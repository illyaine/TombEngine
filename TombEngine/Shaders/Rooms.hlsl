#include "./CBCamera.hlsli"
#include "./CBRoom.hlsli"
#include "./VertexInput.hlsli"
#include "./VertexEffects.hlsli"
#include "./Blending.hlsli"
#include "./Math.hlsli"
#include "./AnimatedTextures.hlsli"
#include "./Shadows.hlsli"
#include "./ShaderLight.hlsli"
#include "./Materials.hlsli"

#define ROOM_LIGHT_COEFF 0.7f

struct PixelShaderInput
{
	float4 Position: SV_POSITION;
	float3 WorldPosition: POSITION0;
	float3 Normal: NORMAL;
	float2 UV: TEXCOORD0;
	float4 Color: COLOR;
	float Sheen : SHEEN;
	float4 PositionCopy : TEXCOORD1;
	float4 FogBulbs : TEXCOORD2;
	float DistanceFog : FOG;
	float3 Tangent: TANGENT;
    float3 Binormal : BINORMAL;
    float3 FaceNormal : TEXCOORD3;
};

Texture2D Texture : register(t0);
SamplerState Sampler : register(s0);

Texture2D NormalTexture : register(t1);
SamplerState NormalTextureSampler : register(s1);

Texture2D CausticsTexture : register(t2);
SamplerState CausticsTextureSampler : register(s2);

struct PixelShaderOutput
{
	float4 Color: SV_TARGET0;
};

PixelShaderInput VS(VertexShaderInput input)
{
	PixelShaderInput output;

	// Setting effect weight on TE side prevents portal vertices from moving.
	// Here we just read weight and decide if we should apply refraction or movement effect.
    float weight = DecodeWeight(input.Effects);

	// Calculate vertex effects
	float wibble = Wibble(input.Effects, DecodeHash(input.AnimationFrameOffsetIndexHash));
	float3 pos = Move(input.Position, input.Effects * weight, wibble);
	float3 col = Glow(input.Color.xyz, input.Effects, wibble);

	// Refraction
	float4 screenPos = mul(float4(pos, 1.0f), ViewProjection);
	float2 clipPos = screenPos.xy / screenPos.w;

	if (CameraUnderwater != Water)
	{
		float factor = (Frame + clipPos.x * 320);
		float xOffset = (sin(factor * PI / 20.0f)) * (screenPos.z / 1024) * 4;
		float yOffset = (cos(factor * PI / 20.0f)) * (screenPos.z / 1024) * 4;
		screenPos.x += xOffset * weight;
		screenPos.y += yOffset * weight;
	}
	
	output.Position = screenPos;
    output.Normal = input.Normal.xyz;
	output.Color = float4(col, input.Color.w);
	output.PositionCopy = screenPos;
    output.UV = GetUVPossiblyAnimated(input.UV, DecodeIndexInPoly(input.Effects), DecodeAnimationFrameOffset(input.AnimationFrameOffsetIndexHash));
	output.WorldPosition = pos;
    output.Tangent = input.Tangent.xyz;
    output.Binormal = cross(input.Normal.xyz, input.Tangent.xyz);
    output.FaceNormal = input.FaceNormal;
	
	output.FogBulbs = DoFogBulbsForVertex(output.WorldPosition);
	output.DistanceFog = DoDistanceFogForVertex(output.WorldPosition);

	return output;
}

PixelShaderOutput PS(PixelShaderInput input)
{
	PixelShaderOutput output;
	
    input.UV = ConvertAnimUV(input.UV);
   
    // Apply parallax mapping
    float3x3 TBNf = float3x3(input.Tangent, input.Binormal, input.FaceNormal);
    input.UV = ParallaxOcclusionMapping(TBNf, input.WorldPosition, input.UV);                	  

    float4 ORSH = ConvertAnimOSRH(ORSHTexture.Sample(ORSHSampler, input.UV));
    float ambientOcclusion = ORSH.x;
    float roughness = ORSH.y;
    float specular = ORSH.z;

    float3 emissive = EmissiveTexture.Sample(EmissiveSampler, input.UV).xyz;
	
    float3x3 TBN = float3x3(input.Tangent, input.Binormal, input.Normal);
    float3 normal = ConvertAnimNormal(UnpackNormalMap(NormalTexture.Sample(NormalTextureSampler, input.UV)));
    normal = EnsureNormal(mul(normal, TBN), input.WorldPosition);

	output.Color = Texture.Sample(Sampler, input.UV);
	DoAlphaTest(output.Color);

    // Material effects
	float3 blendedNormal = normalize(lerp(input.FaceNormal, normal, 0.1f)); // TODO: Make alpha customizable
    output.Color.xyz = CalculateReflections(input.WorldPosition, output.Color.xyz, blendedNormal, specular);

	// Ambient occlusion is an indirect-lighting term. Room vertex colors contain
	// baked room light and the compiled light result for imported geometry.
    float occlusion = CalculateOcclusion(GetSamplePosition(input.PositionCopy), output.Color.w);
    occlusion *= ambientOcclusion;

	float3 indirectLighting = input.Color.xyz;
	float3 directDiffuse = float3(0.0f, 0.0f, 0.0f);
	float3 directSpecular = float3(0.0f, 0.0f, 0.0f);
	
	// Preserve the existing shadow order: room and object shadows affect the
	// baked/indirect term before runtime room lights are added.
	indirectLighting = DoShadow(input.WorldPosition, normal, indirectLighting, -2.5f);
	indirectLighting = DoBlobShadows(input.WorldPosition, indirectLighting);

	bool onlyPointLights = (NumRoomLights & ~LT_MASK) == LT_MASK_POINT;
	int numLights = NumRoomLights & LT_MASK;

	for (int i = 0; i < numLights; i++)
	{
		if (onlyPointLights || RoomLights[i].Type == LT_POINT)
		{
            directDiffuse += DoPointLight(input.WorldPosition, normal, RoomLights[i]) * ROOM_LIGHT_COEFF;
            directSpecular += DoSpecularPoint(input.WorldPosition, normal, RoomLights[i], 0.0f, specular, roughness);
        }
		else if (RoomLights[i].Type == LT_SPOT)
		{
            directDiffuse += DoSpotLight(input.WorldPosition, normal, RoomLights[i]) * ROOM_LIGHT_COEFF;
            directSpecular += DoSpecularSpot(input.WorldPosition, normal, RoomLights[i], 0.0f, specular, roughness);
		}
	}

	// Decals
	if (!Animated && NumRoomDecals > 0 && !(MaterialTypeAndFlags & MATERIAL_FLAG_HEIGHTMAP))
	{
		float decalMask = 0.0f;

		for (int i = 0; i < NumRoomDecals; i++)
		{
			float radius   = RoomDecals[i].Radius;
			float3 pos     = input.WorldPosition - RoomDecals[i].Position;
			float distance = length(pos);

			if (distance > radius * 1.3f)
				continue;
				
			float2 uv = float2(dot(pos, input.Tangent), dot(pos, input.Binormal));
			uv *= (8.0f / (RoomDecals[i].Pattern + 1) * 2.0f / radius);

			float noiseVal = NebularNoise(uv, 1, 0.5f, 0.3f);

			float noisyRadius = radius * (1.0f + 0.25f * (noiseVal * 2.0f - 1.0f));
			float holeRadius = radius / 4.0f;

			float edge = saturate((noisyRadius - distance) / noisyRadius);
			float fade = saturate((radius - distance) / radius);
			float hole = saturate((holeRadius - distance) / (holeRadius * 1.3f)) * (1 - RoomDecals[i].Pattern);

			decalMask = max(decalMask, (edge * fade + hole) * RoomDecals[i].Opacity);
		}

        float decalLighting = 1.0f - decalMask;
		indirectLighting *= decalLighting;
        directDiffuse *= decalLighting;
        directSpecular *= decalLighting;
	}

	// Caustics
    if (Caustics)
    {
        float attenuation = saturate(dot(float3(0.0f, -1.0f, 0.0f), normal));

        float3 blending = abs(normal);
        blending = normalize(max(blending, 0.00001f));
        float b = (blending.x + blending.y + blending.z);
        blending /= float3(b, b, b);

        float3 p = frac(input.WorldPosition.xyz / 2048.0f);
	
		float2 uv_x = CausticsStartUV + float2(p.z, p.y) * CausticsSize;
        float2 uv_y = CausticsStartUV + float2(p.z, p.x) * CausticsSize;
        float2 uv_z = CausticsStartUV + float2(p.y, p.x) * CausticsSize;

        float3 xaxis = CausticsTexture.SampleLevel(CausticsTextureSampler, uv_x, 0).xyz;
        float3 yaxis = CausticsTexture.SampleLevel(CausticsTextureSampler, uv_y, 0).xyz;
        float3 zaxis = CausticsTexture.SampleLevel(CausticsTextureSampler, uv_z, 0).xyz;

        float3 xc = xaxis * blending.x;
        float3 yc = yaxis * blending.y;
        float3 zc = zaxis * blending.z;

        float3 caustics = xc + yc + zc;

        directDiffuse += (caustics * attenuation * 2.0f);
    }

    // Compose room and imported geometry lighting. AO only attenuates the
    // indirect vertex-light term; direct lights and specular remain visible.
    float3 surfaceLighting = (indirectLighting * occlusion) + directDiffuse;
    float3 finalColor = (output.Color.xyz * surfaceLighting) + directSpecular;

	// Fog bulbs retain their legacy darkening before the color blend pass.
	finalColor -= float3(input.FogBulbs.w, input.FogBulbs.w, input.FogBulbs.w);

    // Emissive is self-generated light and must not be suppressed by AO,
    // room shadows or dark vertex colors.
    finalColor += emissive;
	output.Color.xyz = saturate(finalColor);

	output.Color = DoFogBulbsForPixel(output.Color, float4(input.FogBulbs.xyz, 1.0f));
	output.Color = DoDistanceFogForPixel(output.Color, FogColor, input.DistanceFog);

    return output;
}

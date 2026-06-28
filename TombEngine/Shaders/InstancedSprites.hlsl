#include "./CBCamera.hlsli"
#include "./Blending.hlsli"
#include "./VertexInput.hlsli"
#include "./Math.hlsli"
#include "./ShaderLight.hlsli"
#include "./SpriteEffects.hlsli"
#include "./VertexEffects.hlsli"

// NOTE: This shader is used for all opaque or not sorted transparent sprites, that can be instanced for a faster drawing

#define INSTANCED_SPRITES_BUCKET_SIZE 512
#define FADE_FACTOR .789f

#define RENDER_TYPE_HDR_SOURCE_CORE 3
#define RENDER_TYPE_HDR_HALO 4
#define RENDER_TYPE_HDR_GLARE 5

struct PixelShaderInput
{
	float4 Position: SV_POSITION;
	float2 UV: TEXCOORD1;
	float4 Color: COLOR;
	float4 PositionCopy: TEXCOORD2;
	float4 FogBulbs : TEXCOORD3;
	float2 EffectUV : TEXCOORD4;
	float DistanceFog : FOG;
	uint InstanceID : SV_InstanceID;
};

struct InstancedSprite
{
	float4x4 World;
	float4 UV[2];
	float4 Color;
	float IsBillboard;
    float IsSoftParticle;
    int RenderType;
    int PerVertexColor;
};

cbuffer InstancedSpriteBuffer : register(b13)
{
	InstancedSprite Sprites[INSTANCED_SPRITES_BUCKET_SIZE];
};

// HDR effect parameters are isolated from the legacy sprite instances so the
// 512-entry sprite buffer remains exactly within the D3D11 64 KiB limit.
cbuffer HDRSpriteEffectBuffer : register(b9)
{
	float4 HDREffectParams[INSTANCED_SPRITES_BUCKET_SIZE];
};

Texture2D Texture : register(t0);
SamplerState Sampler : register(s0);

Texture2D DepthTexture : register(t6);
SamplerState DepthSampler : register(s6);

PixelShaderInput VS(VertexShaderInput input, uint InstanceID : SV_InstanceID)
{
	PixelShaderInput output;

    InstancedSprite sprite = Sprites[InstanceID];
	
	float4 worldPosition;

    if (sprite.IsBillboard == 1)
	{
        worldPosition = mul(float4(input.Position, 1.0f), sprite.World);
        output.Position = mul(mul(float4(input.Position, 1.0f), sprite.World), ViewProjection);
    }
	else
	{
		worldPosition = float4(input.Position, 1.0f);
		output.Position = mul(float4(input.Position, 1.0f), ViewProjection);
	}
	
    int polyIndex = DecodeIndexInPoly(input.Effects);

	output.PositionCopy = output.Position;
    output.Color = lerp(sprite.Color, input.Color, saturate((float) sprite.PerVertexColor));
    output.UV = float2(sprite.UV[0][polyIndex], sprite.UV[1][polyIndex]);
	output.EffectUV = input.UV;
	output.InstanceID = InstanceID;

	output.FogBulbs = DoFogBulbsForVertex(worldPosition);
	output.DistanceFog = DoDistanceFogForVertex(worldPosition);

	return output;
}

// TODO: From NVIDIA SDK, check if it can be useful instead of linear ramp
float Contrast(float Input, float ContrastPower)
{
#if 1
	//piecewise contrast function
	bool IsAboveHalf = Input > 0.5;
	float ToRaise = saturate(2 * (IsAboveHalf ? 1 - Input : Input));
	float Output = 0.5 * pow(ToRaise, ContrastPower);
	Output = IsAboveHalf ? 1 - Output : Output;
	return Output;
#else
	// another solution to create a kind of contrast function
	return 1.0 - exp2(-2 * pow(2.0 * saturate(Input), ContrastPower));
#endif
}

float4 ApplyHDRLightEffect(float4 color, float2 uv, int renderType, float4 effectParams)
{
	float2 centered = uv * 2.0f - 1.0f;
	float2 absolute = abs(centered);
	float radiusSquared = dot(centered, centered);
	float radius = sqrt(radiusSquared);
	float softness = saturate(effectParams.x);
	float rayCount = max(round(effectParams.y), 2.0f);
	float pulseAmount = saturate(effectParams.z);
	float pulseSpeed = max(effectParams.w, 0.0f);
	float pulse = max(1.0f + sin(Frame * pulseSpeed * 0.05f) * pulseAmount, 0.0f);

	// Fade the generated layer before the billboard boundary. This prevents a
	// visible rectangular edge while retaining HDR values above 1.0 internally.
	float edgeWidth = max(fwidth(radius) * 2.0f, 0.002f);
	float outerFade = 1.0f - smoothstep(1.0f - edgeWidth, 1.0f + edgeWidth, radius);
	float mask = 0.0f;

	if (renderType == RENDER_TYPE_HDR_SOURCE_CORE)
	{
		// A broad luminous body plus a compact white-hot centre reads as an actual
		// emitting surface instead of a uniformly faded billboard. Independent
		// billboard width and height naturally produce bulb, tube and panel shapes.
		float body = exp2(-radiusSquared * lerp(2.5f, 8.0f, softness));
		float hotCenter = exp2(-radiusSquared * lerp(14.0f, 36.0f, softness));
		float sourceBoundary = 1.0f - smoothstep(0.72f, 1.0f, radius);
		mask = (body * 0.72f + hotCenter * 0.48f) * sourceBoundary;

		float peak = max(color.r, max(color.g, color.b));
		float whiteHotAmount = saturate(hotCenter * lerp(0.15f, 0.45f, softness));
		color.rgb = lerp(color.rgb, peak.xxx, whiteHotAmount);
	}
	else if (renderType == RENDER_TYPE_HDR_HALO)
	{
		// Two falloff scales avoid the flat single-Gaussian appearance. The broad
		// lobe provides atmosphere; the tighter lobe keeps the halo attached to its
		// source without flooding the full frame.
		float broadHalo = exp2(-radiusSquared * lerp(1.4f, 4.0f, softness));
		float innerHalo = exp2(-radiusSquared * lerp(5.0f, 12.0f, softness));
		mask = (broadHalo * 0.55f + innerHalo * 0.25f) * outerFade;
	}
	else if (renderType == RENDER_TYPE_HDR_GLARE)
	{
		float angle = atan2(centered.y, centered.x);
		float angularRays = pow(
			abs(cos(angle * rayCount * 0.5f)),
			lerp(14.0f, 42.0f, softness));

		float horizontalCore = exp2(-absolute.y * lerp(22.0f, 72.0f, softness)) *
			exp2(-absolute.x * lerp(1.5f, 3.5f, softness));
		float horizontalFeather = exp2(-absolute.y * lerp(7.0f, 20.0f, softness)) *
			exp2(-absolute.x * lerp(3.0f, 7.0f, softness)) * 0.28f;
		float verticalCore = exp2(-absolute.x * lerp(22.0f, 72.0f, softness)) *
			exp2(-absolute.y * lerp(2.0f, 5.0f, softness)) * 0.55f;
		float radialRays = angularRays * exp2(-radius * lerp(3.0f, 7.0f, softness)) * 0.72f;
		float centralSpark = exp2(-radiusSquared * lerp(24.0f, 56.0f, softness)) * 0.32f;

		mask = max(radialRays, max(horizontalCore + horizontalFeather, verticalCore));
		mask = (mask + centralSpark) * outerFade;
	}

	float energy = max(mask * pulse, 0.0f);
	color.rgb *= energy;
	color.a *= saturate(mask) * saturate(pulse);
	return color;
}

float4 PS(PixelShaderInput input) : SV_TARGET
{
	float4 output = Texture.Sample(Sampler, input.UV) * input.Color;

    InstancedSprite sprite = Sprites[input.InstanceID];
	const bool isHDRLightEffect = sprite.RenderType >= RENDER_TYPE_HDR_SOURCE_CORE &&
		sprite.RenderType <= RENDER_TYPE_HDR_GLARE;

	if (isHDRLightEffect)
		output = ApplyHDRLightEffect(output, input.EffectUV, sprite.RenderType, HDREffectParams[input.InstanceID]);
	
    if (sprite.IsSoftParticle == 1)
	{
		float particleDepth = input.PositionCopy.z / input.PositionCopy.w;
		input.PositionCopy.xy /= input.PositionCopy.w;
		float2 texCoord = 0.5f * (float2(input.PositionCopy.x, -input.PositionCopy.y) + 1);
		float sceneDepth = DepthTexture.Sample(DepthSampler, texCoord).x;

		sceneDepth = LinearizeDepth(sceneDepth, NearPlane, FarPlane);
		particleDepth = LinearizeDepth(particleDepth, NearPlane, FarPlane);

		if (particleDepth - sceneDepth > 0.01f)
		{
			discard;
		}

		float fade = (sceneDepth - particleDepth) * 1024.0f;
		output.w = min(output.w, fade);
	}
	
    if (sprite.RenderType == 1)
    {
        output = DoLaserBarrierEffect(input.Position, output, input.UV, FADE_FACTOR, Frame);
    }

    if (sprite.RenderType == 2)
    {
        output = DoLaserBeamEffect(input.Position, output, input.UV, FADE_FACTOR, Frame);
    }

	output.xyz *= 1.0f - Luma(input.FogBulbs.xyz);
	if (!isHDRLightEffect)
		output.xyz = saturate(output.xyz);

	output = DoDistanceFogForPixel(output, float4(0.0f, 0.0f, 0.0f, 0.0f), input.DistanceFog);

	return output;
}

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
	float4 EffectParams;
	float IsBillboard;
    float IsSoftParticle;
    int RenderType;
    int PerVertexColor;
};

cbuffer InstancedSpriteBuffer : register(b13)
{
	InstancedSprite Sprites[INSTANCED_SPRITES_BUCKET_SIZE];
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
	float radius = length(centered);
	float softness = saturate(effectParams.x);
	float rayCount = max(round(effectParams.y), 2.0f);
	float pulseAmount = saturate(effectParams.z);
	float pulseSpeed = max(effectParams.w, 0.0f);
	float pulse = 1.0f + sin(Frame * pulseSpeed * 0.05f) * pulseAmount;
	float mask = 0.0f;

	if (renderType == RENDER_TYPE_HDR_SOURCE_CORE)
	{
		float core = saturate(1.0f - radius);
		mask = pow(core, lerp(1.0f, 6.0f, softness));
	}
	else if (renderType == RENDER_TYPE_HDR_HALO)
	{
		float haloFalloff = lerp(1.5f, 7.5f, softness);
		mask = exp(-radius * radius * haloFalloff) * saturate(1.0f - radius);
	}
	else if (renderType == RENDER_TYPE_HDR_GLARE)
	{
		float angle = atan2(centered.y, centered.x);
		float angularRays = pow(abs(cos(angle * rayCount * 0.5f)), lerp(10.0f, 32.0f, softness));
		float horizontal = exp(-abs(centered.y) * lerp(14.0f, 42.0f, softness));
		float vertical = exp(-abs(centered.x) * lerp(14.0f, 42.0f, softness));
		float radialFade = pow(saturate(1.0f - radius), 1.5f);
		mask = saturate(max(angularRays * 0.75f, max(horizontal, vertical)) * radialFade);
	}

	color.xyz *= mask * pulse;
	color.w *= mask;
	return color;
}

float4 PS(PixelShaderInput input) : SV_TARGET
{
	float4 output = Texture.Sample(Sampler, input.UV) * input.Color;

    InstancedSprite sprite = Sprites[input.InstanceID];
	const bool isHDRLightEffect = sprite.RenderType >= RENDER_TYPE_HDR_SOURCE_CORE &&
		sprite.RenderType <= RENDER_TYPE_HDR_GLARE;

	if (isHDRLightEffect)
		output = ApplyHDRLightEffect(output, input.EffectUV, sprite.RenderType, sprite.EffectParams);
	
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
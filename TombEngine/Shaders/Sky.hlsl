#include "./CBCamera.hlsli"
#include "./Blending.hlsli"
#include "./VertexInput.hlsli"
#include "./Math.hlsli"
#include "./ShaderLight.hlsli"
#include "./CBSky.hlsli"
#include "./AnimatedTextures.hlsli"
#include "./VertexEffects.hlsli"
#include "./Aurora.hlsli"

struct PixelShaderInput
{
	float4 Position: SV_POSITION;
	float3 Normal: NORMAL;
	float2 UV: TEXCOORD;
	float4 Color: COLOR;
	float4 FogBulbs : TEXCOORD3;
	float3 WorldPosition : TEXCOORD4;
};

Texture2D Texture : register(t0);
SamplerState Sampler : register(s0);

PixelShaderInput VS(VertexShaderInput input)
{
	PixelShaderInput output;

	float4 worldPosition = mul(float4(input.Position, 1.0f), World);

	output.Position = mul(worldPosition, ViewProjection);
	output.Normal = input.Normal.xyz;
	output.Color = input.Color;
	output.UV = GetUVPossiblyAnimated(input.UV, DecodeIndexInPoly(input.Effects), DecodeAnimationFrameOffset(input.AnimationFrameOffsetIndexHash));
	output.FogBulbs = ApplyFogBulbs == 1 ? DoFogBulbsForSky(worldPosition) : 0;
	output.WorldPosition = worldPosition.xyz;

	// Temporary prototype mode. Keep aurora in front of the legacy horizon mesh until it gets its own renderer pass.
	if (Color.w > 1.5f)
		output.Position.z = output.Position.w * 0.0001f;

	return output;
}

float4 PS(PixelShaderInput input) : SV_TARGET
{
	// Temporary prototype mode. RendererDraw.cpp uses Color.w > 1.5 for the standalone aurora pass.
	if (Color.w > 1.5f)
		return float4(DoAuroraScreenWorldBands(input.Position.xy, InterpolatedFrame), 1.0f);

	if (Animated && Type == 1)
		input.UV = CalculateUVRotate(input.UV, 0);

	float4 output = Texture.Sample(Sampler, input.UV);

	DoAlphaTest(output);

	float3 light = saturate(Color.xyz - float3(input.FogBulbs.w, input.FogBulbs.w, input.FogBulbs.w) * 1.4f);
	output.xyz *= light;
	output.xyz += saturate(input.FogBulbs.xyz);
	output.w *= Color.w;

	return output;
}
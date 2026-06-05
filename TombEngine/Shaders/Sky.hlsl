#include "./CBCamera.hlsli"
#include "./Blending.hlsli"
#include "./VertexInput.hlsli"
#include "./Math.hlsli"
#include "./ShaderLight.hlsli"
#include "./CBSky.hlsli"
#include "./AnimatedTextures.hlsli"
#include "./VertexEffects.hlsli"

struct PixelShaderInput
{
	float4 Position: SV_POSITION;
	float3 Normal: NORMAL;
	float2 UV: TEXCOORD;
	float4 Color: COLOR;
	float4 FogBulbs : TEXCOORD3;
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

	return output;
}

float4 PS(PixelShaderInput input) : SV_TARGET
{
	// Keep the temporary aurora prototype marker harmless until the atmosphere layer uses a separate shader/pass.
	if (Color.w > 1.5f && ApplyFogBulbs == 0 && Color.x > 0.99f && Color.y > 0.99f && Color.z > 0.99f)
		return float4(0.0f, 0.0f, 0.0f, 1.0f);

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
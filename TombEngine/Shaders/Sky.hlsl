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

	// Temporary aurora prototype marker. Keep this strict so normal sky/horizon layers
	// are not routed through the aurora shader path.
	if (abs(Color.w - 2.0f) < 0.01f && ApplyFogBulbs == 0 && Color.x > 0.99f && Color.y > 0.99f && Color.z > 0.99f)
	{
		output.Position = float4(input.Position.x / 5120.0f, input.Position.z / 5120.0f, 0.0001f, 1.0f);
		output.FogBulbs = 0;
	}

	return output;
}

float4 PS(PixelShaderInput input) : SV_TARGET
{
	// Temporary aurora prototype marker. This must stay stricter than normal sky colors.
	if (abs(Color.w - 2.0f) < 0.01f && ApplyFogBulbs == 0 && Color.x > 0.99f && Color.y > 0.99f && Color.z > 0.99f)
	{
		float3 aurora = DoAuroraFullscreenDome(input.Position.xy, Frame);
		return float4(aurora * 0.75f, 1.0f);
	}

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
#include "./CBCamera.hlsli"
#include "./Math.hlsli"

struct PixelShaderInput
{
	float4 Position: SV_POSITION;
	float2 UV: TEXCOORD0;
};

PixelShaderInput VS(float3 position : POSITION0, float2 uv : TEXCOORD0)
{
	PixelShaderInput output;
	output.Position = float4(position, 1.0f);
	output.UV = uv;
	return output;
}

float4 PS(PixelShaderInput input) : SV_TARGET
{
	return float4(0.0f, 0.0f, 0.0f, 1.0f);
}

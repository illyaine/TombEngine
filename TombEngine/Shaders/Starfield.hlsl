#include "./CBCamera.hlsli"
#include "./CBStarfield.hlsli"
#include "./Math.hlsli"
#include "./ShaderLight.hlsli"
#include "./VertexInput.hlsli"

struct StarfieldInstance
{
	float3 Direction;
	float Scale;
	float3 Color;
	float Extinction;
};

struct PixelShaderInput
{
	float4 Position : SV_POSITION;
	float2 UV : TEXCOORD1;
	float4 Color : COLOR;
	float4 FogBulbs : TEXCOORD3;
	float DistanceFog : FOG;
};

StructuredBuffer<StarfieldInstance> Stars : register(t14);

Texture2D Texture : register(t0);
SamplerState Sampler : register(s0);

float Hash(uint value)
{
	value ^= value >> 16;
	value *= 0x7feb352du;
	value ^= value >> 15;
	value *= 0x846ca68bu;
	value ^= value >> 16;

	return (value & 0x00ffffffu) / 16777215.0f;
}

float GetTwinkle(uint instanceID)
{
	uint seed = instanceID + 1u;
	seed ^= Frame + 0x9e3779b9u + (seed << 6) + (seed >> 2);

	return lerp(0.5f, 1.0f, Hash(seed));
}

PixelShaderInput VS(VertexShaderInput input, uint instanceID : SV_InstanceID)
{
	PixelShaderInput output;
	StarfieldInstance star = Stars[instanceID];

	const float starDistance = 1024.0f;
	const float starSize = 2.0f * star.Scale;

	float3 cameraUp = normalize(float3(View[0][1], View[1][1], View[2][1]));
	float3 billboardForward = normalize(star.Direction);
	float3 billboardRight = normalize(cross(cameraUp, billboardForward));
	float3 billboardUp = cross(billboardForward, billboardRight);
	float3 center = CamPositionWS.xyz + star.Direction * starDistance;
	float3 worldPosition = center + billboardRight * input.Position.x * starSize + billboardUp * input.Position.y * starSize;

	output.Position = mul(float4(worldPosition, 1.0f), ViewProjection);

	int polyIndex = DecodeIndexInPoly(input.Effects);
	output.UV = float2(StarfieldUV[0][polyIndex], StarfieldUV[1][polyIndex]);
	output.Color = float4(star.Color, GetTwinkle(instanceID) * star.Extinction);
	output.FogBulbs = DoFogBulbsForVertex(float4(worldPosition, 1.0f));
	output.DistanceFog = DoDistanceFogForVertex(float4(worldPosition, 1.0f));

	return output;
}

float4 PS(PixelShaderInput input) : SV_TARGET
{
	float4 output = Texture.Sample(Sampler, input.UV) * input.Color;

	output.xyz *= 1.0f - Luma(input.FogBulbs.xyz);
	output.xyz = saturate(output.xyz);
	output = DoDistanceFogForPixel(output, float4(0.0f, 0.0f, 0.0f, 0.0f), input.DistanceFog);

	return output;
}

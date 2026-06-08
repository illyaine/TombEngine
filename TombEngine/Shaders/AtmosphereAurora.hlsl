#include "./CBAtmosphereAurora.hlsli"

struct PostProcessVertexShaderInput
{
	float3 Position: POSITION0;
	float2 UV: TEXCOORD0;
	float4 Color: COLOR0;
};

struct PixelShaderInput
{
	float4 Position: SV_POSITION;
	float2 UV: TEXCOORD0;
	float4 Color: COLOR0;
};

PixelShaderInput VS(PostProcessVertexShaderInput input)
{
	PixelShaderInput output;

	output.Position = float4(input.Position, 1.0f);
	output.UV = input.UV;
	output.Color = input.Color;

	return output;
}

float AuroraHash(float2 value)
{
	return frac(sin(dot(value, float2(127.1f, 311.7f))) * 43758.5453f);
}

float AuroraNoise(float2 value)
{
	float2 i = floor(value);
	float2 f = frac(value);
	float2 u = f * f * (3.0f - 2.0f * f);

	return lerp(
		lerp(AuroraHash(i + float2(0.0f, 0.0f)), AuroraHash(i + float2(1.0f, 0.0f)), u.x),
		lerp(AuroraHash(i + float2(0.0f, 1.0f)), AuroraHash(i + float2(1.0f, 1.0f)), u.x),
		u.y);
}

float AuroraLayerBand(float2 uv, float height, float width, float waveScale, float waveStrength, float time, float offset)
{
	float x = uv.x;
	float y = uv.y;

	float coarse = AuroraNoise(float2(x * waveScale * 2.7f + time * 0.24f + offset, time * 0.055f + offset));
	float fine = AuroraNoise(float2(x * waveScale * 12.0f - time * 0.18f + offset * 2.0f, y * 8.0f + offset));
	float wave = (coarse - 0.5f) * waveStrength * width;

	float center = saturate(height + wave);
	float distanceFromBand = abs(y - center);
	float band = 1.0f - smoothstep(width * 0.12f, width, distanceFromBand);

	float curtains = pow(saturate(0.25f + fine * 0.90f), 2.2f);
	float streaks = pow(saturate(sin((x + coarse * 0.18f + offset + time * 0.025f) * 74.0f) * 0.5f + 0.5f), 3.0f);
	float skyFade = smoothstep(0.04f, 0.18f, y) * (1.0f - smoothstep(0.70f, 0.96f, y));

	return band * lerp(curtains, streaks, 0.32f) * skyFade;
}

float4 PS(PixelShaderInput input) : SV_Target
{
	float intensity = saturate(AuroraControls.x);
	float speed = max(AuroraControls.y, 0.01f);
	float height = saturate(1.0f - AuroraControls.z);
	float width = max(AuroraControls.w * 0.42f, 0.035f);
	float waveScale = max(AuroraWaves.x, 0.05f);
	float waveStrength = saturate(AuroraWaves.y);
	float transparency = saturate(AuroraWaves.z);
	float time = AuroraTime.x * speed;

	float2 uv = input.UV;
	uv.x = frac(uv.x * 1.18f + time * 0.035f);

	float layerA = AuroraLayerBand(uv, height, width, waveScale, waveStrength, time, 0.0f);
	float layerB = AuroraLayerBand(uv, height + width * 0.35f, width * 0.70f, waveScale * 1.35f, waveStrength * 0.78f, time * 0.85f, 12.7f);
	float layerC = AuroraLayerBand(uv, height - width * 0.30f, width * 0.58f, waveScale * 1.75f, waveStrength * 0.60f, time * 0.68f, 31.4f);

	float3 color = AuroraColorA.rgb * layerA + AuroraColorB.rgb * layerB + AuroraColorC.rgb * layerC;
	float alpha = saturate((layerA + layerB * 0.62f + layerC * 0.42f) * transparency);
	alpha *= intensity > 0.0f ? 1.0f : 0.0f;

	float glow = 0.04f;
	color *= intensity;
	color += color * glow;

	return float4(color, alpha);
}

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
	float4 PositionCopy: TEXCOORD1;
};

PixelShaderInput VS(PostProcessVertexShaderInput input)
{
	PixelShaderInput output;

	output.Position = float4(input.Position, 1.0f);
	output.UV = input.UV;
	output.Color = input.Color;
	output.PositionCopy = output.Position;

	return output;
}

float Hash(float2 value)
{
	return frac(sin(dot(value, float2(127.1f, 311.7f))) * 43758.5453f);
}

float Noise(float2 value)
{
	float2 i = floor(value);
	float2 f = frac(value);
	float2 u = f * f * (3.0f - 2.0f * f);

	return lerp(
		lerp(Hash(i + float2(0.0f, 0.0f)), Hash(i + float2(1.0f, 0.0f)), u.x),
		lerp(Hash(i + float2(0.0f, 1.0f)), Hash(i + float2(1.0f, 1.0f)), u.x),
		u.y);
}

float LayerBand(float2 uv, float height, float width, float waveScale, float waveStrength, float time, float offset)
{
	float x = uv.x;
	float y = uv.y;

	float coarse = Noise(float2(x * waveScale * 3.0f + time * 0.18f + offset, time * 0.05f + offset));
	float fine = Noise(float2(x * waveScale * 13.0f - time * 0.12f + offset * 2.0f, y * 8.0f + offset));
	float wave = (coarse - 0.5f) * waveStrength * width;

	float center = saturate(height + wave);
	float distanceFromBand = abs(y - center);
	float band = 1.0f - smoothstep(width * 0.20f, width, distanceFromBand);

	float curtains = pow(saturate(0.35f + fine * 0.85f), 2.0f);
	float streaks = pow(saturate(sin((x + coarse * 0.22f + offset) * 90.0f) * 0.5f + 0.5f), 3.0f);
	float topFade = smoothstep(0.08f, 0.22f, y) * (1.0f - smoothstep(0.96f, 1.0f, y));

	return band * lerp(curtains, streaks, 0.35f) * topFade;
}

float4 PS(PixelShaderInput input) : SV_Target
{
	float2 uv = input.UV;

	float intensity = saturate(AuroraControls.x);
	float speed = AuroraControls.y;
	float height = saturate(AuroraControls.z);
	float width = max(AuroraControls.w, 0.02f);
	float waveScale = max(AuroraWaves.x, 0.05f);
	float waveStrength = saturate(AuroraWaves.y);
	float transparency = saturate(AuroraWaves.z);
	float time = AuroraTime.x * speed;

	float layerA = LayerBand(uv, height, width, waveScale, waveStrength, time, 0.0f);
	float layerB = LayerBand(uv, height + width * 0.22f, width * 0.78f, waveScale * 1.35f, waveStrength * 0.85f, time * 0.82f, 12.7f);
	float layerC = LayerBand(uv, height - width * 0.18f, width * 0.62f, waveScale * 1.75f, waveStrength * 0.65f, time * 0.68f, 31.4f);

	float3 color = AuroraColorA.rgb * layerA + AuroraColorB.rgb * layerB + AuroraColorC.rgb * layerC;
	float alpha = saturate((layerA + layerB * 0.75f + layerC * 0.55f) * transparency);
	alpha *= intensity > 0.0f ? 1.0f : 0.0f;

	float glow = 0.06f;
	color *= intensity;
	color += color * glow;

	return float4(color, alpha);
}
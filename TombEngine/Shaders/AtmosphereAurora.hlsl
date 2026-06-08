#include "./CBCamera.hlsli"
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

float2 GetSkyUv(PixelShaderInput input)
{
	float2 screenUv = input.Position.xy * InvViewSize;
	float2 clip = screenUv * 2.0f - 1.0f;
	clip.y = -clip.y;

	float4 viewPosition = mul(float4(clip, 1.0f, 1.0f), InverseProjection);
	viewPosition.xyz /= max(abs(viewPosition.w), 0.0001f);

	float3 direction = normalize(mul(float4(viewPosition.xyz, 0.0f), InverseView).xyz);
	float yaw = atan2(direction.x, direction.z) / 6.2831853f;
	float pitch = saturate(direction.y * 0.5f + 0.5f);

	return float2(frac(yaw + 0.5f), pitch);
}

float LayerBand(float2 uv, float height, float width, float waveScale, float waveStrength, float time, float offset)
{
	float x = uv.x;
	float y = uv.y;

	float coarse = AuroraNoise(float2(x * waveScale * 2.4f + time * 0.14f + offset, time * 0.035f + offset));
	float fine = AuroraNoise(float2(x * waveScale * 11.0f - time * 0.10f + offset * 2.0f, y * 7.0f + offset));
	float wave = (coarse - 0.5f) * waveStrength * width;

	float center = saturate(height + wave);
	float distanceFromBand = abs(y - center);
	float band = 1.0f - smoothstep(width * 0.10f, width, distanceFromBand);

	float curtains = pow(saturate(0.26f + fine * 0.86f), 2.6f);
	float streaks = pow(saturate(sin((x + coarse * 0.18f + offset + time * 0.008f) * 72.0f) * 0.5f + 0.5f), 3.2f);
	float skyFade = smoothstep(0.03f, 0.16f, y) * (1.0f - smoothstep(0.62f, 0.88f, y));

	return band * lerp(curtains, streaks, 0.30f) * skyFade;
}

float4 PS(PixelShaderInput input) : SV_Target
{
	float2 uv = GetSkyUv(input);

	float intensity = saturate(AuroraControls.x);
	float speed = AuroraControls.y;
	float height = saturate(1.0f - AuroraControls.z);
	float width = max(AuroraControls.w * 0.35f, 0.018f);
	float waveScale = max(AuroraWaves.x, 0.05f);
	float waveStrength = saturate(AuroraWaves.y);
	float transparency = saturate(AuroraWaves.z);
	float time = AuroraTime.x * speed;

	float layerA = LayerBand(uv, height, width, waveScale, waveStrength, time, 0.0f);
	float layerB = LayerBand(uv, height + width * 0.35f, width * 0.72f, waveScale * 1.35f, waveStrength * 0.78f, time * 0.85f, 12.7f);
	float layerC = LayerBand(uv, height - width * 0.30f, width * 0.58f, waveScale * 1.75f, waveStrength * 0.60f, time * 0.68f, 31.4f);

	float3 color = AuroraColorA.rgb * layerA + AuroraColorB.rgb * layerB + AuroraColorC.rgb * layerC;
	float alpha = saturate((layerA + layerB * 0.62f + layerC * 0.42f) * transparency);
	alpha *= intensity > 0.0f ? 1.0f : 0.0f;

	float glow = 0.035f;
	color *= intensity;
	color += color * glow;

	return float4(color, alpha);
}
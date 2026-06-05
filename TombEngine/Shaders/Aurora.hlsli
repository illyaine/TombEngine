#ifndef AURORA_SHADER
#define AURORA_SHADER

float2 AuroraDebugUv(float2 pixelPosition)
{
	float2 uv = pixelPosition * InvViewSize;
	uv.y = 1.0f - uv.y;
	uv.x = (uv.x - 0.5f) * AspectRatio + 0.5f;
	return uv;
}

float2 AuroraWorldUv(float3 worldPosition)
{
	float3 direction = normalize(worldPosition - CamPositionWS.xyz);
	float yaw = atan2(direction.z, direction.x) / PI2 + 0.5f;
	float height = saturate(direction.y * 0.70f + 0.58f);
	return float2(frac(yaw), height);
}

float AuroraSoftBand(float2 uv, float time, float baseHeight, float curveStrength, float width, float phase)
{
	float x = uv.x + phase;
	float curve = baseHeight;
	curve += sin((x + time * 0.020f) * PI2 * 1.15f) * curveStrength;
	curve += sin((x - time * 0.013f) * PI2 * 2.40f) * curveStrength * 0.40f;
	curve += sin((x + time * 0.008f) * PI2 * 4.10f) * curveStrength * 0.18f;

	float d = abs(uv.y - curve);
	float core = 1.0f - smoothstep(width, width + 0.040f, d);
	float glow = 1.0f - smoothstep(width + 0.035f, width + 0.230f, d);

	float raysA = sin((x + time * 0.018f) * PI2 * 12.0f) * 0.5f + 0.5f;
	float raysB = sin((x - time * 0.011f) * PI2 * 23.0f) * 0.5f + 0.5f;
	float rays = smoothstep(0.18f, 0.95f, raysA * 0.65f + raysB * 0.35f);
	rays = lerp(0.45f, 1.0f, rays);

	float above = smoothstep(curve - 0.040f, curve + 0.130f, uv.y);
	float belowTop = 1.0f - smoothstep(curve + 0.220f, curve + 0.560f, uv.y);
	float curtain = rays * above * belowTop;

	return saturate(core * 0.60f + glow * 0.45f + curtain * 0.34f);
}

float3 AuroraColorFromUv(float2 uv, float frame)
{
	float time = frame / 60.0f;

	float lower = AuroraSoftBand(float2(uv.x - 0.21f, uv.y), time, 0.40f, 0.045f, 0.045f, 0.31f);
	float middle = AuroraSoftBand(uv, time, 0.56f, 0.060f, 0.050f, 0.00f);
	float upper = AuroraSoftBand(float2(uv.x + 0.17f, uv.y), time, 0.70f, 0.045f, 0.055f, 0.67f);

	float3 color = float3(0.08f, 0.78f, 0.36f) * lower;
	color += float3(0.08f, 0.78f, 0.88f) * middle;
	color += float3(0.48f, 0.16f, 0.72f) * upper;

	float horizonFade = smoothstep(0.22f, 0.42f, uv.y);
	float zenithFade = 1.0f - smoothstep(0.96f, 1.0f, uv.y);
	float seamFade = smoothstep(0.00f, 0.035f, uv.x) * (1.0f - smoothstep(0.965f, 1.0f, uv.x));
	seamFade = max(seamFade, 0.30f);

	return color * horizonFade * zenithFade * seamFade * 0.95f;
}

float3 DoAuroraWorldBands(float3 worldPosition, float frame)
{
	return AuroraColorFromUv(AuroraWorldUv(worldPosition), frame);
}

float3 DoAuroraDebugBands(float2 pixelPosition, float frame)
{
	return AuroraColorFromUv(AuroraDebugUv(pixelPosition), frame);
}

// Compatibility helper for old prototype calls.
float3 DoAurora(float3 worldPosition, float frame)
{
	return DoAuroraWorldBands(worldPosition, frame);
}

#endif
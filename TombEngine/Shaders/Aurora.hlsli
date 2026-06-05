#ifndef AURORA_SHADER
#define AURORA_SHADER

float2 AuroraDebugUv(float2 pixelPosition)
{
	float2 uv = pixelPosition * InvViewSize;
	uv.y = 1.0f - uv.y;
	uv.x = (uv.x - 0.5f) * AspectRatio + 0.5f;
	return uv;
}

float2 AuroraDirectionUv(float3 direction)
{
	direction = normalize(direction);
	float yaw = atan2(direction.z, direction.x) / PI2 + 0.5f;
	float height = saturate(direction.y * 0.76f + 0.58f);
	return float2(frac(yaw), height);
}

float3 AuroraWorldDirectionFromScreen(float2 pixelPosition)
{
	float2 uv = pixelPosition * InvViewSize;
	float2 ndc = float2(uv.x * 2.0f - 1.0f, 1.0f - uv.y * 2.0f);
	float4 viewPosition = mul(float4(ndc, 1.0f, 1.0f), InverseProjection);
	viewPosition.xyz /= max(abs(viewPosition.w), EPSILON);
	return normalize(mul(float4(viewPosition.xyz, 0.0f), InverseView).xyz);
}

float2 AuroraWorldUv(float3 worldPosition)
{
	return AuroraDirectionUv(worldPosition - CamPositionWS.xyz);
}

float2 AuroraScreenWorldUv(float2 pixelPosition)
{
	return AuroraDirectionUv(AuroraWorldDirectionFromScreen(pixelPosition));
}

float AuroraSoftBand(float2 uv, float time, float baseHeight, float curveStrength, float width, float phase)
{
	float x = uv.x + phase;
	float curve = baseHeight;
	curve += sin((x + time * 0.018f) * PI2 * 0.90f) * curveStrength;
	curve += sin((x - time * 0.012f) * PI2 * 1.80f) * curveStrength * 0.42f;
	curve += sin((x + time * 0.007f) * PI2 * 3.70f) * curveStrength * 0.22f;

	float d = abs(uv.y - curve);
	float core = 1.0f - smoothstep(width, width + 0.055f, d);
	float glow = 1.0f - smoothstep(width + 0.040f, width + 0.280f, d);

	float raysA = sin((x + time * 0.015f) * PI2 * 8.0f) * 0.5f + 0.5f;
	float raysB = sin((x - time * 0.010f) * PI2 * 17.0f) * 0.5f + 0.5f;
	float rays = smoothstep(0.20f, 0.96f, raysA * 0.60f + raysB * 0.40f);
	rays = lerp(0.35f, 1.0f, rays);

	float above = smoothstep(curve - 0.050f, curve + 0.170f, uv.y);
	float belowTop = 1.0f - smoothstep(curve + 0.230f, curve + 0.620f, uv.y);
	float curtain = rays * above * belowTop;

	return saturate(core * 0.48f + glow * 0.52f + curtain * 0.36f);
}

float3 AuroraColorFromUv(float2 uv, float frame)
{
	float time = frame / 60.0f;

	float lower = AuroraSoftBand(float2(uv.x - 0.18f, uv.y), time, 0.48f, 0.045f, 0.052f, 0.31f);
	float middle = AuroraSoftBand(uv, time, 0.62f, 0.060f, 0.058f, 0.00f);
	float upper = AuroraSoftBand(float2(uv.x + 0.16f, uv.y), time, 0.76f, 0.050f, 0.064f, 0.67f);

	float3 color = float3(0.06f, 0.72f, 0.34f) * lower;
	color += float3(0.07f, 0.72f, 0.82f) * middle;
	color += float3(0.42f, 0.14f, 0.62f) * upper;

	float horizonFade = smoothstep(0.36f, 0.52f, uv.y);
	float zenithFade = 1.0f - smoothstep(0.98f, 1.0f, uv.y);
	float seamFade = smoothstep(0.00f, 0.040f, uv.x) * (1.0f - smoothstep(0.960f, 1.0f, uv.x));
	seamFade = max(seamFade, 0.38f);

	return color * horizonFade * zenithFade * seamFade * 0.90f;
}

float3 DoAuroraWorldBands(float3 worldPosition, float frame)
{
	return AuroraColorFromUv(AuroraWorldUv(worldPosition), frame);
}

float3 DoAuroraScreenWorldBands(float2 pixelPosition, float frame)
{
	return AuroraColorFromUv(AuroraScreenWorldUv(pixelPosition), frame);
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
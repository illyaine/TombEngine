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
	float height = saturate(direction.y * 0.74f + 0.59f);
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
	curve += sin((x + time * 0.15f) * PI2 * 0.90f) * curveStrength;
	curve += sin((x - time * 0.09f) * PI2 * 1.80f) * curveStrength * 0.48f;
	curve += sin((x + time * 0.06f) * PI2 * 3.70f) * curveStrength * 0.28f;

	float d = abs(uv.y - curve);
	float core = 1.0f - smoothstep(width, width + 0.045f, d);
	float glow = 1.0f - smoothstep(width + 0.035f, width + 0.300f, d);

	float raysA = sin((x + time * 0.34f) * PI2 * 10.0f) * 0.5f + 0.5f;
	float raysB = sin((x - time * 0.22f) * PI2 * 24.0f) * 0.5f + 0.5f;
	float rays = smoothstep(0.18f, 0.92f, raysA * 0.58f + raysB * 0.42f);
	rays = pow(saturate(rays), 2.2f);

	float above = smoothstep(curve - 0.045f, curve + 0.160f, uv.y);
	float belowTop = 1.0f - smoothstep(curve + 0.180f, curve + 0.680f, uv.y);
	float curtain = rays * above * belowTop;
	float pulse = 0.86f + sin(time * 1.4f + phase * PI2) * 0.14f;

	return saturate((core * 0.64f + glow * 0.45f + curtain * 0.78f) * pulse);
}

float3 AuroraColorFromUv(float2 uv, float frame)
{
	float time = frame / 60.0f;

	float lower = AuroraSoftBand(float2(uv.x - 0.16f, uv.y), time, 0.46f, 0.055f, 0.043f, 0.31f);
	float middle = AuroraSoftBand(uv, time, 0.61f, 0.074f, 0.047f, 0.00f);
	float upper = AuroraSoftBand(float2(uv.x + 0.14f, uv.y), time, 0.74f, 0.058f, 0.052f, 0.67f);

	float3 color = float3(0.04f, 0.82f, 0.36f) * lower;
	color += float3(0.06f, 0.78f, 0.96f) * middle;
	color += float3(0.48f, 0.18f, 0.78f) * upper;

	float brightness = 0.88f + sin(time * 0.9f + uv.x * PI2 * 1.5f) * 0.12f;
	float horizonFade = smoothstep(0.38f, 0.58f, uv.y);
	float zenithFade = 1.0f - smoothstep(0.98f, 1.0f, uv.y);
	float seamFade = smoothstep(0.00f, 0.040f, uv.x) * (1.0f - smoothstep(0.960f, 1.0f, uv.x));
	float sideFade = smoothstep(0.02f, 0.18f, uv.x) * (1.0f - smoothstep(0.82f, 0.98f, uv.x));
	seamFade = max(seamFade, 0.42f);
	sideFade = max(sideFade, 0.45f);

	return color * horizonFade * zenithFade * seamFade * sideFade * brightness * 1.05f;
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
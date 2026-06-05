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

float AuroraHash(float2 p)
{
	return frac(sin(dot(p, float2(127.1f, 311.7f))) * 43758.5453f);
}

float AuroraNoise(float2 p)
{
	float2 i = floor(p);
	float2 f = frac(p);
	float2 u = f * f * (3.0f - 2.0f * f);

	float a = AuroraHash(i + float2(0.0f, 0.0f));
	float b = AuroraHash(i + float2(1.0f, 0.0f));
	float c = AuroraHash(i + float2(0.0f, 1.0f));
	float d = AuroraHash(i + float2(1.0f, 1.0f));

	return lerp(lerp(a, b, u.x), lerp(c, d, u.x), u.y);
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
	float glow = 1.0f - smoothstep(width + 0.035f, width + 0.360f, d);

	float raysA = sin((x + time * 0.34f) * PI2 * 10.0f) * 0.5f + 0.5f;
	float raysB = sin((x - time * 0.22f) * PI2 * 24.0f) * 0.5f + 0.5f;
	float raysNoise = AuroraNoise(float2(x * 18.0f + time * 0.55f, phase * 4.0f));
	float rays = smoothstep(0.15f, 0.94f, raysA * 0.44f + raysB * 0.36f + raysNoise * 0.20f);
	rays = pow(saturate(rays), 2.0f);

	float above = smoothstep(curve - 0.060f, curve + 0.170f, uv.y);
	float belowTop = 1.0f - smoothstep(curve + 0.220f, curve + 0.760f, uv.y);
	float curtain = rays * above * belowTop;
	float pulse = 0.86f + sin(time * 1.4f + phase * PI2) * 0.14f;

	return saturate((core * 0.56f + glow * 0.48f + curtain * 0.82f) * pulse);
}

float3 AuroraColorFromUv(float2 uv, float frame)
{
	float time = frame / 60.0f;

	float lower = AuroraSoftBand(float2(uv.x - 0.16f, uv.y), time, 0.46f, 0.055f, 0.043f, 0.31f);
	float middle = AuroraSoftBand(uv, time, 0.61f, 0.074f, 0.047f, 0.00f);
	float upper = AuroraSoftBand(float2(uv.x + 0.14f, uv.y), time, 0.74f, 0.058f, 0.052f, 0.67f);

	float colorShift = AuroraNoise(float2(uv.x * 4.0f + time * 0.08f, uv.y * 2.5f));
	float colorWave = sin((uv.x * 1.7f + time * 0.11f) * PI2) * 0.5f + 0.5f;

	float3 green = float3(0.05f, 0.86f, 0.38f);
	float3 cyan = float3(0.06f, 0.78f, 0.98f);
	float3 blue = float3(0.10f, 0.38f, 1.00f);
	float3 violet = float3(0.56f, 0.18f, 0.86f);

	float3 lowerColor = lerp(green, cyan, saturate(colorShift * 0.75f + colorWave * 0.25f));
	float3 middleColor = lerp(cyan, blue, saturate(colorWave * 0.65f + colorShift * 0.35f));
	float3 upperColor = lerp(blue, violet, saturate(colorShift * 0.55f + colorWave * 0.45f));

	float3 color = lowerColor * lower;
	color += middleColor * middle;
	color += upperColor * upper;

	float brightness = 0.88f + sin(time * 0.9f + uv.x * PI2 * 1.5f) * 0.12f;
	float horizonFade = smoothstep(0.32f, 0.58f, uv.y);
	float zenithFade = 1.0f - smoothstep(0.96f, 1.0f, uv.y);
	float seamFade = smoothstep(0.00f, 0.080f, uv.x) * (1.0f - smoothstep(0.920f, 1.0f, uv.x));
	float sideFade = smoothstep(-0.08f, 0.14f, uv.x) * (1.0f - smoothstep(0.86f, 1.08f, uv.x));
	seamFade = max(seamFade, 0.72f);
	sideFade = max(sideFade, 0.78f);

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

float3 DoAuroraFullscreenDome(float2 pixelPosition, float frame)
{
	float2 screenUv = pixelPosition * InvViewSize;
	float edgeFade = smoothstep(-0.02f, 0.10f, screenUv.x) * (1.0f - smoothstep(0.90f, 1.02f, screenUv.x));
	edgeFade *= smoothstep(-0.04f, 0.16f, screenUv.y) * (1.0f - smoothstep(0.88f, 1.04f, screenUv.y));
	edgeFade = max(edgeFade, 0.86f);
	return DoAuroraScreenWorldBands(pixelPosition, frame) * edgeFade;
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
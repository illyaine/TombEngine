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
	float height = saturate(direction.y * 0.62f + 0.72f);
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

float AuroraColumnMask(float x, float time, float phase)
{
	float raysA = sin((x + phase + time * 0.012f) * PI2 * 7.0f) * 0.5f + 0.5f;
	float raysB = sin((x - phase * 0.37f - time * 0.009f) * PI2 * 19.0f) * 0.5f + 0.5f;
	float raysNoise = AuroraNoise(float2(x * 15.0f + phase * 5.0f, time * 0.06f));
	float gaps = AuroraNoise(float2(x * 4.0f - time * 0.018f, phase * 6.0f));
	float mask = smoothstep(0.55f, 0.95f, raysA * 0.42f + raysB * 0.34f + raysNoise * 0.24f);
	mask *= smoothstep(0.18f, 0.78f, gaps);
	return lerp(0.05f, 1.0f, mask);
}

float AuroraSoftBand(float2 uv, float time, float baseHeight, float curveStrength, float width, float phase)
{
	float x = uv.x + phase;
	float curve = baseHeight;
	curve += sin((x + time * 0.018f) * PI2 * 0.80f) * curveStrength;
	curve += sin((x - time * 0.013f) * PI2 * 1.70f) * curveStrength * 0.50f;
	curve += sin((x + time * 0.010f) * PI2 * 3.20f) * curveStrength * 0.30f;

	float d = abs(uv.y - curve);
	float core = 1.0f - smoothstep(width, width + 0.014f, d);
	float localGlow = 1.0f - smoothstep(width + 0.012f, width + 0.085f, d);
	float columnMask = AuroraColumnMask(x, time, phase);

	float lowerCurtain = smoothstep(curve - 0.260f, curve - 0.030f, uv.y);
	float upperCurtain = 1.0f - smoothstep(curve + 0.050f, curve + 0.210f, uv.y);
	float curtainShape = lowerCurtain * upperCurtain;
	float curtain = pow(saturate(columnMask * curtainShape), 1.45f);
	float pulse = 0.97f + sin(time * 0.18f + phase * PI2) * 0.03f;

	return saturate((core * 0.58f + localGlow * 0.07f + curtain * 0.48f) * pulse);
}

float AuroraWideVeil(float2 uv, float time)
{
	float wave = sin((uv.x * 0.82f + time * 0.010f) * PI2) * 0.5f + 0.5f;
	float patches = AuroraNoise(float2(uv.x * 2.8f - time * 0.012f, uv.y * 3.1f));
	float band = smoothstep(0.58f, 0.76f, uv.y) * (1.0f - smoothstep(0.94f, 1.0f, uv.y));
	return band * smoothstep(0.20f, 0.88f, wave * 0.45f + patches * 0.55f) * 0.055f;
}

float3 AuroraColorFromUv(float2 uv, float frame)
{
	float time = frame / 300.0f;

	float lower = AuroraSoftBand(float2(uv.x - 0.17f, uv.y), time, 0.70f, 0.036f, 0.021f, 0.31f);
	float middle = AuroraSoftBand(uv, time, 0.82f, 0.052f, 0.024f, 0.00f);
	float upper = AuroraSoftBand(float2(uv.x + 0.16f, uv.y), time, 0.91f, 0.035f, 0.026f, 0.67f);
	float veil = AuroraWideVeil(uv, time);

	float colorShift = AuroraNoise(float2(uv.x * 4.8f + time * 0.008f, uv.y * 2.6f));
	float colorWave = sin((uv.x * 1.05f + time * 0.012f) * PI2) * 0.5f + 0.5f;

	float3 green = float3(0.06f, 0.84f, 0.30f);
	float3 cyan = float3(0.04f, 0.70f, 0.90f);
	float3 blue = float3(0.08f, 0.25f, 0.86f);
	float3 violet = float3(0.50f, 0.14f, 0.76f);
	float3 magenta = float3(0.68f, 0.12f, 0.46f);

	float3 lowerColor = lerp(green, cyan, saturate(colorShift * 0.35f + colorWave * 0.15f));
	float3 middleColor = lerp(cyan, blue, saturate(colorWave * 0.50f + colorShift * 0.25f));
	float3 upperColor = lerp(violet, magenta, saturate(colorShift * 0.40f + colorWave * 0.32f));
	float3 veilColor = lerp(green, cyan, saturate(colorShift * 0.60f));

	float3 color = lowerColor * lower * 0.74f;
	color += middleColor * middle * 0.68f;
	color += upperColor * upper * 0.54f;
	color += veilColor * veil;

	float brightness = 0.98f + sin(time * 0.11f + uv.x * PI2 * 1.2f) * 0.02f;
	float horizonFade = smoothstep(0.62f, 0.76f, uv.y);
	float zenithFade = 1.0f - smoothstep(0.985f, 1.0f, uv.y);
	float seamFade = smoothstep(0.00f, 0.080f, uv.x) * (1.0f - smoothstep(0.920f, 1.0f, uv.x));
	float sideFade = smoothstep(-0.05f, 0.17f, uv.x) * (1.0f - smoothstep(0.83f, 1.05f, uv.x));
	seamFade = max(seamFade, 0.66f);
	sideFade = max(sideFade, 0.72f);

	return color * horizonFade * zenithFade * seamFade * sideFade * brightness * 0.72f;
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
	edgeFade = max(edgeFade, 0.88f);
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
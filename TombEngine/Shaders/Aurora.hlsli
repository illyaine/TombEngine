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
	float height = saturate(direction.y * 0.70f + 0.66f);
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
	float raysA = sin((x + phase + time * 0.026f) * PI2 * 8.0f) * 0.5f + 0.5f;
	float raysB = sin((x - phase * 0.37f - time * 0.019f) * PI2 * 17.0f) * 0.5f + 0.5f;
	float raysNoise = AuroraNoise(float2(x * 12.0f + phase * 5.0f, time * 0.12f));
	float mask = smoothstep(0.48f, 0.90f, raysA * 0.46f + raysB * 0.34f + raysNoise * 0.20f);
	return lerp(0.24f, 1.0f, mask);
}

float AuroraSoftBand(float2 uv, float time, float baseHeight, float curveStrength, float width, float phase)
{
	float x = uv.x + phase;
	float curve = baseHeight;
	curve += sin((x + time * 0.045f) * PI2 * 0.90f) * curveStrength;
	curve += sin((x - time * 0.032f) * PI2 * 1.80f) * curveStrength * 0.45f;
	curve += sin((x + time * 0.021f) * PI2 * 3.70f) * curveStrength * 0.24f;

	float d = abs(uv.y - curve);
	float core = 1.0f - smoothstep(width, width + 0.020f, d);
	float glow = 1.0f - smoothstep(width + 0.025f, width + 0.180f, d);
	float columnMask = AuroraColumnMask(x, time, phase);

	float above = smoothstep(curve - 0.040f, curve + 0.090f, uv.y);
	float belowTop = 1.0f - smoothstep(curve + 0.170f, curve + 0.520f, uv.y);
	float curtain = pow(saturate(columnMask * above * belowTop), 1.15f);
	float pulse = 0.93f + sin(time * 0.42f + phase * PI2) * 0.07f;

	return saturate((core * 0.78f + glow * 0.22f + curtain * 0.90f) * pulse);
}

float3 AuroraColorFromUv(float2 uv, float frame)
{
	float time = frame / 150.0f;

	float lower = AuroraSoftBand(float2(uv.x - 0.13f, uv.y), time, 0.58f, 0.040f, 0.026f, 0.31f);
	float middle = AuroraSoftBand(uv, time, 0.71f, 0.052f, 0.030f, 0.00f);
	float upper = AuroraSoftBand(float2(uv.x + 0.12f, uv.y), time, 0.84f, 0.040f, 0.034f, 0.67f);

	float colorShift = AuroraNoise(float2(uv.x * 3.2f + time * 0.025f, uv.y * 2.2f));
	float colorWave = sin((uv.x * 1.25f + time * 0.030f) * PI2) * 0.5f + 0.5f;

	float3 green = float3(0.06f, 0.90f, 0.36f);
	float3 cyan = float3(0.05f, 0.78f, 0.95f);
	float3 blue = float3(0.10f, 0.30f, 1.00f);
	float3 violet = float3(0.58f, 0.16f, 0.86f);

	float3 lowerColor = lerp(green, cyan, saturate(colorShift * 0.45f + colorWave * 0.18f));
	float3 middleColor = lerp(cyan, blue, saturate(colorWave * 0.52f + colorShift * 0.20f));
	float3 upperColor = lerp(blue, violet, saturate(colorShift * 0.42f + colorWave * 0.36f));

	float3 color = lowerColor * lower;
	color += middleColor * middle * 0.92f;
	color += upperColor * upper * 0.84f;

	float brightness = 0.96f + sin(time * 0.28f + uv.x * PI2 * 1.2f) * 0.04f;
	float horizonFade = smoothstep(0.50f, 0.67f, uv.y);
	float zenithFade = 1.0f - smoothstep(0.97f, 1.0f, uv.y);
	float seamFade = smoothstep(0.00f, 0.090f, uv.x) * (1.0f - smoothstep(0.910f, 1.0f, uv.x));
	float sideFade = smoothstep(-0.06f, 0.16f, uv.x) * (1.0f - smoothstep(0.84f, 1.06f, uv.x));
	seamFade = max(seamFade, 0.68f);
	sideFade = max(sideFade, 0.74f);

	return color * horizonFade * zenithFade * seamFade * sideFade * brightness * 1.10f;
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
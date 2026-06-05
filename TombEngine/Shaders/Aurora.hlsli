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
	float height = saturate(direction.y * 0.58f + 0.76f);
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

float AuroraColumnMask(float x, float time, float phase, float densityA, float densityB, float gapDensity)
{
	float raysA = sin((x + phase + time * 0.010f) * PI2 * densityA) * 0.5f + 0.5f;
	float raysB = sin((x - phase * 0.37f - time * 0.007f) * PI2 * densityB) * 0.5f + 0.5f;
	float raysNoise = AuroraNoise(float2(x * (densityA + 5.0f) + phase * 5.0f, time * 0.045f));
	float gaps = AuroraNoise(float2(x * gapDensity - time * 0.015f, phase * 6.0f));
	float mask = smoothstep(0.56f, 0.96f, raysA * 0.40f + raysB * 0.34f + raysNoise * 0.26f);
	mask *= smoothstep(0.22f, 0.82f, gaps);
	return lerp(0.02f, 1.0f, mask);
}

float3 AuroraCurtainLayer(
	float2 uv,
	float time,
	float baseHeight,
	float curveStrength,
	float width,
	float phase,
	float densityA,
	float densityB,
	float gapDensity,
	float drift,
	float lowerReach,
	float upperReach,
	float intensity,
	float3 colorA,
	float3 colorB)
{
	float x = uv.x + phase;
	float curve = baseHeight;
	curve += sin((x + time * drift) * PI2 * 0.76f) * curveStrength;
	curve += sin((x - time * drift * 0.70f) * PI2 * 1.65f) * curveStrength * 0.48f;
	curve += sin((x + time * drift * 0.42f) * PI2 * 3.10f) * curveStrength * 0.26f;

	float d = abs(uv.y - curve);
	float core = 1.0f - smoothstep(width, width + 0.012f, d);
	float localGlow = 1.0f - smoothstep(width + 0.010f, width + 0.070f, d);
	float columnMask = AuroraColumnMask(x, time, phase, densityA, densityB, gapDensity);

	float lowerCurtain = smoothstep(curve - lowerReach, curve - width * 0.45f, uv.y);
	float upperCurtain = 1.0f - smoothstep(curve + width * 0.70f, curve + upperReach, uv.y);
	float curtainShape = lowerCurtain * upperCurtain;
	float curtain = pow(saturate(columnMask * curtainShape), 1.42f);

	float localBreakup = AuroraNoise(float2(x * 5.0f + phase * 3.0f, uv.y * 6.0f - time * 0.018f));
	float breakup = lerp(0.35f, 1.0f, smoothstep(0.16f, 0.86f, localBreakup));
	float layer = (core * 0.52f + localGlow * 0.06f + curtain * 0.44f) * breakup;

	float tint = saturate(AuroraNoise(float2(x * 2.8f + time * 0.010f, baseHeight * 8.0f)) * 0.72f + uv.y * 0.18f);
	return lerp(colorA, colorB, tint) * layer * intensity;
}

float3 AuroraVeilLayer(float2 uv, float time, float baseLow, float baseHigh, float phase, float intensity, float3 colorA, float3 colorB)
{
	float wave = sin((uv.x * 0.76f + phase + time * 0.006f) * PI2) * 0.5f + 0.5f;
	float patchesA = AuroraNoise(float2(uv.x * 2.6f + phase * 3.0f - time * 0.009f, uv.y * 3.2f));
	float patchesB = AuroraNoise(float2(uv.x * 5.2f - phase * 2.0f, uv.y * 4.4f + time * 0.006f));
	float vertical = smoothstep(baseLow, baseLow + 0.120f, uv.y) * (1.0f - smoothstep(baseHigh - 0.050f, baseHigh, uv.y));
	float mask = smoothstep(0.26f, 0.90f, wave * 0.36f + patchesA * 0.42f + patchesB * 0.22f);
	float tint = saturate(patchesA * 0.70f + uv.y * 0.12f);
	return lerp(colorA, colorB, tint) * vertical * mask * intensity;
}

float3 AuroraColorFromUv(float2 uv, float frame)
{
	float time = frame / 420.0f;

	float3 green = float3(0.06f, 0.84f, 0.28f);
	float3 cyan = float3(0.04f, 0.66f, 0.88f);
	float3 blue = float3(0.07f, 0.24f, 0.80f);
	float3 violet = float3(0.46f, 0.13f, 0.70f);
	float3 magenta = float3(0.66f, 0.12f, 0.42f);

	float3 color = float3(0.0f, 0.0f, 0.0f);
	color += AuroraVeilLayer(uv, time, 0.62f, 0.98f, 0.17f, 0.032f, green, cyan);
	color += AuroraCurtainLayer(float2(uv.x - 0.18f, uv.y), time, 0.74f, 0.040f, 0.020f, 0.31f, 5.5f, 15.0f, 3.8f, 0.70f, 0.300f, 0.120f, 0.095f, green, cyan);
	color += AuroraCurtainLayer(float2(uv.x + 0.05f, uv.y), time, 0.84f, 0.052f, 0.023f, 0.02f, 7.0f, 20.0f, 4.6f, 0.54f, 0.260f, 0.110f, 0.110f, cyan, blue);
	color += AuroraCurtainLayer(float2(uv.x + 0.18f, uv.y), time, 0.93f, 0.036f, 0.016f, 0.71f, 10.0f, 27.0f, 5.8f, 0.42f, 0.180f, 0.070f, 0.062f, violet, magenta);

	float brightness = 0.99f + sin(time * 0.08f + uv.x * PI2 * 1.2f) * 0.01f;
	float horizonFade = smoothstep(0.65f, 0.78f, uv.y);
	float zenithFade = 1.0f - smoothstep(0.990f, 1.0f, uv.y);
	float seamFade = smoothstep(0.00f, 0.075f, uv.x) * (1.0f - smoothstep(0.925f, 1.0f, uv.x));
	float sideFade = smoothstep(-0.05f, 0.16f, uv.x) * (1.0f - smoothstep(0.84f, 1.05f, uv.x));
	seamFade = max(seamFade, 0.64f);
	sideFade = max(sideFade, 0.70f);

	return color * horizonFade * zenithFade * seamFade * sideFade * brightness;
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
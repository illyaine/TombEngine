#ifndef AURORA_SHADER
#define AURORA_SHADER

float2 AuroraDebugUv(float2 pixelPosition)
{
	float2 uv = pixelPosition * InvViewSize;
	uv.y = 1.0f - uv.y;
	uv.x = (uv.x - 0.5f) * AspectRatio + 0.5f;
	return uv;
}

float AuroraDebugBand(float2 uv, float time, float baseHeight, float curveStrength, float width, float phase)
{
	float curve = baseHeight;
	curve += sin((uv.x + phase + time * 0.035f) * 5.0f) * curveStrength;
	curve += sin((uv.x + phase - time * 0.022f) * 11.0f) * curveStrength * 0.35f;

	float d = abs(uv.y - curve);
	float core = 1.0f - smoothstep(width, width + 0.018f, d);
	float glow = 1.0f - smoothstep(width + 0.020f, width + 0.120f, d);

	float rays = sin((uv.x + phase + time * 0.050f) * 85.0f) * 0.5f + 0.5f;
	rays = pow(saturate(rays), 5.0f);

	float above = smoothstep(curve - 0.025f, curve + 0.200f, uv.y);
	float top = 1.0f - smoothstep(curve + 0.240f, curve + 0.520f, uv.y);
	float curtain = rays * above * top;

	return saturate(core * 1.8f + glow * 0.42f + curtain * 1.25f);
}

float3 DoAuroraDebugBands(float2 pixelPosition, float frame)
{
	float2 uv = AuroraDebugUv(pixelPosition);
	float time = frame / 60.0f;

	float lower = AuroraDebugBand(float2(uv.x - 0.25f, uv.y), time, 0.28f, 0.070f, 0.024f, 2.0f);
	float middle = AuroraDebugBand(uv, time, 0.47f, 0.105f, 0.030f, 0.0f);
	float upper = AuroraDebugBand(float2(uv.x + 0.18f, uv.y), time, 0.66f, 0.080f, 0.032f, 5.0f);

	float3 color = float3(0.0f, 1.45f, 0.28f) * lower;
	color += float3(0.0f, 1.20f, 1.35f) * middle;
	color += float3(0.85f, 0.20f, 1.40f) * upper;

	float skyFade = smoothstep(0.06f, 0.16f, uv.y) * (1.0f - smoothstep(0.94f, 1.0f, uv.y));
	return color * skyFade * 2.6f;
}

// Compatibility helper for old prototype calls.
float3 DoAurora(float3 worldPosition, float frame)
{
	float2 pseudoPixel = float2(worldPosition.x * 0.125f + ViewSize.x * 0.5f, -worldPosition.y * 0.125f + ViewSize.y * 0.5f);
	return DoAuroraDebugBands(pseudoPixel, frame);
}

#endif

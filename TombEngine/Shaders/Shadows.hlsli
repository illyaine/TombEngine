#include "./Blending.hlsli"
#include "./Math.hlsli"
#include "./ShaderLight.hlsli"
#include "./ModernLighting.hlsli"

#define SHADOW_INTENSITY (0.6f)
#define SHADOW_FILTER_TAPS 16

struct Sphere
{
    float3 position;
    float radius;
};

cbuffer ShadowLightBuffer : register(b4)
{
    ShaderLight Light;
    float4x4 LightViewProjections[6];
    int CastShadows;
    int NumSpheres;
    int ShadowMapSize;
    int padding;
    Sphere Spheres[16];
};

Texture2DArray ShadowMap : register(t3);
SamplerComparisonState ShadowMapSampler : register(s3);

static const float2 ShadowPoissonDisk[SHADOW_FILTER_TAPS] =
{
    float2(-0.94201624f, -0.39906216f),
    float2( 0.94558609f, -0.76890725f),
    float2(-0.09418410f, -0.92938870f),
    float2( 0.34495938f,  0.29387760f),
    float2(-0.91588581f,  0.45771432f),
    float2(-0.81544232f, -0.87912464f),
    float2(-0.38277543f,  0.27676845f),
    float2( 0.97484398f,  0.75648379f),
    float2( 0.44323325f, -0.97511554f),
    float2( 0.53742981f, -0.47373420f),
    float2(-0.26496911f, -0.41893023f),
    float2( 0.79197514f,  0.19090188f),
    float2(-0.24188840f,  0.99706507f),
    float2(-0.81409955f,  0.91437590f),
    float2( 0.19984126f,  0.78641367f),
    float2( 0.14383161f, -0.14100790f)
};

float2 TexOffset(float2 offset)
{
    return offset / max((float)ShadowMapSize, 1.0f);
}

float ShadowRotation(float3 worldPosition)
{
    float noise = frac(sin(dot(worldPosition, float3(0.06711056f, 0.00583715f, 0.75324531f))) * 52.9829189f);
    return noise * PI2;
}

float2 RotateShadowOffset(float2 offset, float angle)
{
    float sine = sin(angle);
    float cosine = cos(angle);
    return float2(
        offset.x * cosine - offset.y * sine,
        offset.x * sine + offset.y * cosine);
}

// https://gist.github.com/JuanDiegoMontoya/d8788148dcb9780848ce8bf50f89b7bb
int GetCubeFaceIndex(float3 dir)
{
    float x = abs(dir.x);
    float y = abs(dir.y);
    float z = abs(dir.z);
    if (x > y && x > z)
        return 0 + (dir.x > 0 ? 0 : 1);
    else if (y > z)
        return 2 + (dir.y > 0 ? 0 : 1);
    return 4 + (dir.z > 0 ? 0 : 1);
}

float2 GetCubeUVFromDir(int faceIndex, float3 dir)
{
    float2 uv;
    switch (faceIndex)
    {
    case 0:
        uv = float2(-dir.z, dir.y);
        break;
    case 1:
        uv = float2(dir.z, dir.y);
        break;
    case 2:
        uv = float2(dir.x, dir.z);
        break;
    case 3:
        uv = float2(dir.x, -dir.z);
        break;
    case 4:
        uv = float2(dir.x, dir.y);
        break;
    default:
        uv = float2(-dir.x, dir.y);
        break;
    }
    return uv * 0.5f + 0.5f;
}

float3 DoBlobShadows(float3 worldPos, float3 lighting)
{
    float shadowFactor = 1.0f;

    for (int i = 0; i < NumSpheres; i++)
    {
        Sphere s = Spheres[i];
        float dist = distance(worldPos, s.position);
        float insideSphere = saturate(1.0f - step(s.radius, dist));
        float radiusFactor = dist / max(s.radius, EPSILON);
        float factor = (1.0f - saturate(radiusFactor)) * insideSphere;
        shadowFactor -= factor * shadowFactor;
    }

    shadowFactor = saturate(shadowFactor);
    return lighting * saturate(1.0f - (1.0f - shadowFactor) * (SHADOW_INTENSITY * 0.5f));
}

float3 DoShadow(float3 worldPos, float3 normal, float3 lighting, float bias)
{
    if (!CastShadows)
        return lighting;

    if (BlendMode != BLENDMODE_OPAQUE && BlendMode != BLENDMODE_ALPHATEST && BlendMode != BLENDMODE_ALPHABLEND)
        return lighting;

    float shadowFactor = 1.0f;

    float3 directionToLight = SafeNormalizeLighting(Light.Position - worldPos, normal);
    float normalLight = saturate(dot(normal, directionToLight));
    float facingFactor = saturate((normalLight - bias) / (1.0f - bias + EPSILON));

    float slope = 1.0f - normalLight;
    float receiverBias = lerp(0.00015f, 0.0015f, slope * slope);
    receiverBias += 0.5f / max((float)ShadowMapSize, 1.0f);

    float lightDistance = distance(worldPos, Light.Position);
    float distanceRatio = saturate(lightDistance / max(Light.Out, EPSILON));
    float filterRadius = lerp(1.25f, 2.75f, distanceRatio);
    float rotation = ShadowRotation(worldPos);

    [unroll]
    for (int i = 0; i < 6; i++)
    {
        float4 lightClipSpace = mul(float4(worldPos, 1.0f), LightViewProjections[i]);
        float safeProjectionW = (abs(lightClipSpace.w) > EPSILON) ?
            lightClipSpace.w :
            ((lightClipSpace.w >= 0.0f) ? EPSILON : -EPSILON);
        lightClipSpace.xyz /= safeProjectionW;

        float insideLightBounds =
            step(-1.0f, lightClipSpace.x) * step(lightClipSpace.x, 1.0f) *
            step(-1.0f, lightClipSpace.y) * step(lightClipSpace.y, 1.0f) *
            step( 0.0f, lightClipSpace.z) * step(lightClipSpace.z, 1.0f);

        if (insideLightBounds > 0.0f)
        {
            lightClipSpace.x = lightClipSpace.x * 0.5f + 0.5f;
            lightClipSpace.y = lightClipSpace.y * -0.5f + 0.5f;

            float filteredShadow = 0.0f;
            float comparisonDepth = lightClipSpace.z - receiverBias;

            [unroll]
            for (int tap = 0; tap < SHADOW_FILTER_TAPS; tap++)
            {
                float2 offset = RotateShadowOffset(ShadowPoissonDisk[tap], rotation) * filterRadius;
                filteredShadow += ShadowMap.SampleCmpLevelZero(
                    ShadowMapSampler,
                    float3(lightClipSpace.xy + TexOffset(offset), i),
                    comparisonDepth);
            }

            filteredShadow /= SHADOW_FILTER_TAPS;
            shadowFactor = lerp(shadowFactor, filteredShadow, facingFactor * insideLightBounds);
        }
    }

    float isPoint = step(0.5f, float(Light.Type == LT_POINT));
    float isSpot = step(0.5f, float(Light.Type == LT_SPOT));
    float isOther = 1.0f - (isPoint + isSpot);

    float pointFactor = saturate(Luma(DoModernPointLight(worldPos, normal, Light)));
    float spotFactor = saturate(Luma(DoModernSpotLight(worldPos, normal, Light)));

    float3 pointShadow = lighting * saturate(1.0f - (1.0f - shadowFactor) * SHADOW_INTENSITY * pointFactor);
    float3 spotShadow = lighting * saturate(1.0f - (1.0f - shadowFactor) * SHADOW_INTENSITY * spotFactor);

    return pointShadow * isPoint + spotShadow * isSpot + lighting * isOther;
}

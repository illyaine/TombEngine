#include "./Blending.hlsli"
#include "./Math.hlsli"
#include "./ShaderLight.hlsli"

#define SHADOW_INTENSITY    (0.6f)
#define SHADOW_BLUR         (2)
#define SHADOW_SAMPLE_COUNT (25.0f)

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

int GetCubeFaceIndex(float3 dir)
{
    float x = abs(dir.x);
    float y = abs(dir.y);
    float z = abs(dir.z);

    // Match the shadow render target face order used by RenderTargetCube::forwardVectors.
    if (x > y && x > z)
        return dir.x < 0.0f ? 0 : 1;
    else if (y > z)
        return dir.y < 0.0f ? 2 : 3;

    return dir.z > 0.0f ? 4 : 5;
}

float2 GetCubeUVFromDir(int faceIndex, float3 dir)
{
    float2 uv;
    switch (faceIndex)
    {
    case 0:
        uv = float2(-dir.z, dir.y);
        break; // +X
    case 1:
        uv = float2(dir.z, dir.y);
        break; // -X
    case 2:
        uv = float2(dir.x, dir.z);
        break; // +Y
    case 3:
        uv = float2(dir.x, -dir.z);
        break; // -Y
    case 4:
        uv = float2(dir.x, dir.y);
        break; // +Z
    default:
        uv = float2(-dir.x, dir.y);
        break; // -Z
    }
    return uv * .5 + .5;
}

float3 DoBlobShadows(float3 worldPos, float3 lighting)
{
    float shadowFactor = 1.0f;

    for (int i = 0; i < NumSpheres; i++)
    {
        Sphere s = Spheres[i];
        float dist = distance(worldPos, s.position);
        float insideSphere = saturate(1.0f - step(s.radius, dist));
        float radiusFactor = dist / s.radius;
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

    float3 lightVec = Light.Position - worldPos;
    float lightDistance = length(lightVec);
    float3 dir = normalize(lightVec);
    float ndot = dot(normal, dir);
    float facingFactor = saturate((ndot - bias) / (1.0f - bias + EPSILON));

    // A zero facing contribution leaves the original lighting unchanged, so no shadow-map work is needed.
    if (facingFactor <= 0.0f)
        return lighting;

    // A point can belong to only one cubemap face away from exact seams. Select that face
    // directly instead of transforming every shaded pixel through all six shadow matrices.
    int faceIndex = GetCubeFaceIndex(-dir);
    float4 lightClipSpace = mul(float4(worldPos, 1.0f), LightViewProjections[faceIndex]);
    lightClipSpace.xyz /= lightClipSpace.w;

    float insideLightBounds =
        step(-1.0f, lightClipSpace.x) * step(lightClipSpace.x, 1.0f) *
        step(-1.0f, lightClipSpace.y) * step(lightClipSpace.y, 1.0f) *
        step( 0.0f, lightClipSpace.z) * step(lightClipSpace.z, 1.0f);

    // No selected shadow face contribution means shadowFactor would stay at one.
    if (insideLightBounds <= 0.0f)
        return lighting;

    lightClipSpace.x = lightClipSpace.x / 2 + 0.5;
    lightClipSpace.y = lightClipSpace.y / -2 + 0.5;

    float sum = 0.0f;
    float texelSize = rcp((float)ShadowMapSize);

    // Perform the same 5x5 PCF filtering with one shared texel-size calculation.
    for (int y = -SHADOW_BLUR; y <= SHADOW_BLUR; y++)
    {
        for (int x = -SHADOW_BLUR; x <= SHADOW_BLUR; x++)
        {
            sum += ShadowMap.SampleCmpLevelZero(
                ShadowMapSampler,
                float3(lightClipSpace.xy + float2(x, y) * texelSize, faceIndex),
                lightClipSpace.z);
        }
    }

    float shadowFactor = lerp(1.0f, sum / SHADOW_SAMPLE_COUNT, facingFactor);
    float distanceAttenuation = saturate((Light.Out - lightDistance) / (Light.Out - Light.In));
    float diffuse = saturate(ndot);

    if (Light.Type == LT_POINT)
    {
        float pointFactor = Luma(saturate(Light.Color.xyz * Light.Intensity * distanceAttenuation * diffuse));
        return lighting * saturate(1.0f - (1.0f - shadowFactor) * SHADOW_INTENSITY * pointFactor);
    }

    if (Light.Type == LT_SPOT)
    {
        float cosine = dot(-dir, Light.Direction.xyz);
        float angleAttenuation = saturate((cosine - Light.OutRange) / (Light.InRange - Light.OutRange));
        float spotFactor = Luma(saturate(Light.Color.xyz * Light.Intensity * angleAttenuation * distanceAttenuation * diffuse));
        return lighting * saturate(1.0f - (1.0f - shadowFactor) * SHADOW_INTENSITY * spotFactor);
    }

    return lighting;
}

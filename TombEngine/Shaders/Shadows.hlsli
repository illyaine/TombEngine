#include "./Blending.hlsli"
#include "./Math.hlsli"
#include "./ShaderLight.hlsli"

#define SHADOW_INTENSITY (0.6f)
#define SHADOW_BLUR      (2.0f)

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

float2 TexOffset(int u, int v) 
{
    return float2(u * 1.0f / ShadowMapSize, v * 1.0f / ShadowMapSize);
}

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

    float shadowFactor = 1.0f;

    float3 dir = normalize(Light.Position - worldPos);
    float ndot = dot(normal, dir);
    float facingFactor = saturate((ndot - bias) / (1.0f - bias + EPSILON));

    // A point can belong to only one cubemap face away from exact seams. Select that face
    // directly instead of transforming every shaded pixel through all six shadow matrices.
    int faceIndex = GetCubeFaceIndex(-dir);
    float4 lightClipSpace = mul(float4(worldPos, 1.0f), LightViewProjections[faceIndex]);
    lightClipSpace.xyz /= lightClipSpace.w;

    float insideLightBounds =
        step(-1.0f, lightClipSpace.x) * step(lightClipSpace.x, 1.0f) *
        step(-1.0f, lightClipSpace.y) * step(lightClipSpace.y, 1.0f) *
        step( 0.0f, lightClipSpace.z) * step(lightClipSpace.z, 1.0f);

    if (insideLightBounds > 0.0f)
    {
        lightClipSpace.x = lightClipSpace.x / 2 + 0.5;
        lightClipSpace.y = lightClipSpace.y / -2 + 0.5;

        float sum = 0;
        float samples = 0;

        // Perform basic PCF filtering.
        for (float y = -SHADOW_BLUR; y <= SHADOW_BLUR; y += 1.0)
        {
            for (float x = -SHADOW_BLUR; x <= SHADOW_BLUR; x += 1.0)
            {
                sum += ShadowMap.SampleCmpLevelZero(
                    ShadowMapSampler,
                    float3(lightClipSpace.xy + TexOffset(x, y), faceIndex),
                    lightClipSpace.z);
                samples += 1.0;
            }
        }

        shadowFactor = lerp(shadowFactor, sum / samples, facingFactor);
    }

    if (Light.Type == LT_POINT)
    {
        float pointFactor = Luma(DoPointLight(worldPos, normal, Light));
        return lighting * saturate(1.0f - (1.0f - shadowFactor) * SHADOW_INTENSITY * pointFactor);
    }

    if (Light.Type == LT_SPOT)
    {
        float spotFactor = Luma(DoSpotLight(worldPos, normal, Light));
        return lighting * saturate(1.0f - (1.0f - shadowFactor) * SHADOW_INTENSITY * spotFactor);
    }

    return lighting;
}

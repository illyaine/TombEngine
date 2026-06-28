#ifndef OBJECT_LIGHTING
#define OBJECT_LIGHTING

#include "./ModernLighting.hlsli"

float3 CombineObjectLights(
    float3 ambient,
    float3 ambientTint,
    float3 directTint,
    float3 tex,
    float3 pos,
    float3 normal,
    float sheen,
    const ShaderLight lights[MAX_LIGHTS_PER_ITEM],
    int numLights,
    float fogBulbsDensity,
    float specularIntensity,
    float roughness,
    float occlusion)
{
    float3 diffuse = 0.0f;
    float3 shadow = 0.0f;
    float3 specular = 0.0f;

    int lightTypeMask = (numLights & ~LT_MASK);
    numLights = numLights & LT_MASK;

    for (int i = 0; i < numLights; i++)
    {
        if (lightTypeMask & LT_MASK_SUN)
        {
            float isSun = step(0.5f, float(lights[i].Type == LT_SUN));
            diffuse += isSun * DoModernDirectionalLight(pos, normal, lights[i]);
            specular += isSun * DoModernSpecularSun(pos, normal, lights[i], sheen, specularIntensity, roughness);
        }

        if (lightTypeMask & LT_MASK_POINT)
        {
            float isPoint = step(0.5f, float(lights[i].Type == LT_POINT));
            diffuse += isPoint * DoModernPointLight(pos, normal, lights[i]);
            specular += isPoint * DoModernSpecularPoint(pos, normal, lights[i], sheen, specularIntensity, roughness);
        }

        if (lightTypeMask & LT_MASK_SPOT)
        {
            float isSpot = step(0.5f, float(lights[i].Type == LT_SPOT));
            diffuse += isSpot * DoModernSpotLight(pos, normal, lights[i]);
            specular += isSpot * DoModernSpecularSpot(pos, normal, lights[i], sheen, specularIntensity, roughness);
        }

        if (lightTypeMask & LT_MASK_SHADOW)
        {
            float isShadow = step(0.5f, float(lights[i].Type == LT_SHADOW));
            shadow += isShadow * DoShadowLight(pos, normal, lights[i]);
        }
    }

    ambientTint = saturate(ambientTint);
    directTint = saturate(directTint);

    float resolvedSpecular = ResolveModernSpecular(specularIntensity, sheen);
    float diffuseEnergy = 1.0f - 0.04f * resolvedSpecular;

    float3 ambientTerm = saturate(ambient - saturate(shadow)) * tex * ambientTint * occlusion;
    float3 diffuseTerm = diffuse * tex * directTint * diffuseEnergy;
    float3 combined = ambientTerm + diffuseTerm + specular;

    combined -= float3(fogBulbsDensity, fogBulbsDensity, fogBulbsDensity);
    return max(combined, float3(0.0f, 0.0f, 0.0f));
}

float3 StaticObjectLight(float3 vertex, float3 tex, float fogBulbsDensity, float occlusion)
{
    float3 result = tex * saturate(vertex) * occlusion;
    result -= float3(fogBulbsDensity, fogBulbsDensity, fogBulbsDensity);
    return max(result, float3(0.0f, 0.0f, 0.0f));
}

#endif // OBJECT_LIGHTING

#ifndef OBJECT_LIGHTING
#define OBJECT_LIGHTING

float3 CombineObjectLights(
    float3 ambient,
    float3 vertex,
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
            diffuse += isSun * DoDirectionalLight(pos, normal, lights[i]);
            specular += isSun * DoSpecularSun(normal, lights[i], sheen, specularIntensity, roughness);
        }

        if (lightTypeMask & LT_MASK_POINT)
        {
            float isPoint = step(0.5f, float(lights[i].Type == LT_POINT));
            diffuse += isPoint * DoPointLight(pos, normal, lights[i]);
            specular += isPoint * DoSpecularPoint(pos, normal, lights[i], sheen, specularIntensity, roughness);
        }

        if (lightTypeMask & LT_MASK_SPOT)
        {
            float isSpot = step(0.5f, float(lights[i].Type == LT_SPOT));
            diffuse += isSpot * DoSpotLight(pos, normal, lights[i]);
            specular += isSpot * DoSpecularSpot(pos, normal, lights[i], sheen, specularIntensity, roughness);
        }

        if (lightTypeMask & LT_MASK_SHADOW)
        {
            float isShadow = step(0.5f, float(lights[i].Type == LT_SHADOW));
            shadow += isShadow * DoShadowLight(pos, normal, lights[i]);
        }
    }

    // Vertex colors remain an albedo tint for ambient and diffuse lighting.
    // Ambient occlusion only affects indirect ambient light; it must not erase
    // direct lights or material specular response.
    float3 vertexTint = saturate(vertex);
    float3 ambientTerm = saturate(ambient - saturate(shadow)) * tex * vertexTint * occlusion;
    float3 diffuseTerm = diffuse * tex * vertexTint;
    float3 combined = ambientTerm + diffuseTerm + specular;

    combined -= float3(fogBulbsDensity, fogBulbsDensity, fogBulbsDensity);
    return saturate(combined);
}

float3 StaticObjectLight(float3 vertex, float3 tex, float fogBulbsDensity, float occlusion)
{
    float3 result = tex * saturate(vertex) * occlusion;
    result -= float3(fogBulbsDensity, fogBulbsDensity, fogBulbsDensity);
    return saturate(result);
}

#endif // OBJECT_LIGHTING

#ifndef MODERN_LIGHTING
#define MODERN_LIGHTING

#include "./CBCamera.hlsli"
#include "./Math.hlsli"

float3 SafeNormalizeLighting(float3 value, float3 fallback)
{
    float lengthSquared = dot(value, value);
    return (lengthSquared > EPSILON) ? value * rsqrt(lengthSquared) : fallback;
}

float SmoothRangeAttenuation(float distanceToLight, float innerRange, float outerRange)
{
    innerRange = max(innerRange, 0.0f);
    outerRange = max(outerRange, innerRange + EPSILON);

    float normalizedRange = saturate((distanceToLight - innerRange) / (outerRange - innerRange));
    float smoothRange = 1.0f - smoothstep(0.0f, 1.0f, normalizedRange);

    // Add a mild physically inspired distance response without invalidating
    // the ranges and intensities authored for existing TEN levels.
    float normalizedDistance = saturate(distanceToLight / outerRange);
    float distanceResponse = rcp(1.0f + 2.0f * normalizedDistance * normalizedDistance);

    return smoothRange * lerp(1.0f, distanceResponse, 0.35f);
}

float SmoothSpotAttenuation(float cosine, float innerCosine, float outerCosine)
{
    float coneRange = max(innerCosine - outerCosine, EPSILON);
    float cone = saturate((cosine - outerCosine) / coneRange);
    return cone * cone * (3.0f - 2.0f * cone);
}

float DistributionGGX(float3 normal, float3 halfVector, float roughness)
{
    float alpha = max(roughness * roughness, 0.0025f);
    float alphaSquared = alpha * alpha;
    float normalHalf = saturate(dot(normal, halfVector));
    float denominator = normalHalf * normalHalf * (alphaSquared - 1.0f) + 1.0f;

    return alphaSquared / max(PI * denominator * denominator, EPSILON);
}

float GeometrySchlickGGX(float normalDirection, float roughness)
{
    float r = roughness + 1.0f;
    float k = (r * r) * 0.125f;
    return normalDirection / max(normalDirection * (1.0f - k) + k, EPSILON);
}

float GeometrySmith(float3 normal, float3 viewDirection, float3 lightDirection, float roughness)
{
    float normalView = saturate(dot(normal, viewDirection));
    float normalLight = saturate(dot(normal, lightDirection));
    return GeometrySchlickGGX(normalView, roughness) * GeometrySchlickGGX(normalLight, roughness);
}

float3 FresnelSchlick(float cosine, float3 baseReflectance)
{
    float fresnel = pow(1.0f - saturate(cosine), 5.0f);
    return baseReflectance + (float3(1.0f, 1.0f, 1.0f) - baseReflectance) * fresnel;
}

float ResolveModernRoughness(float roughness, float sheenStrength)
{
    float resolvedRoughness = clamp(roughness, 0.045f, 1.0f);
    float sheenEnabled = step(EPSILON, abs(sheenStrength));
    float sheenExponent = max(abs(sheenStrength) * SPEC_FACTOR, 1.0f);
    float sheenRoughness = clamp(sqrt(2.0f / (sheenExponent + 2.0f)), 0.045f, 1.0f);
    return lerp(resolvedRoughness, sheenRoughness, sheenEnabled);
}

float ApplySpecularAntialiasing(float3 normal, float roughness)
{
    float3 normalDx = ddx(normal);
    float3 normalDy = ddy(normal);
    float normalVariance = max(dot(normalDx, normalDx), dot(normalDy, normalDy));
    float kernelRoughnessSquared = min(normalVariance * 2.0f, 0.18f);

    return clamp(sqrt(roughness * roughness + kernelRoughnessSquared), 0.045f, 1.0f);
}

float ResolveModernSurfaceRoughness(float3 normal, float roughness, float sheenStrength)
{
    float resolvedRoughness = ResolveModernRoughness(roughness, sheenStrength);
    return ApplySpecularAntialiasing(normal, resolvedRoughness);
}

float ResolveModernSpecular(float specularIntensity, float sheenStrength)
{
    float sheenEnabled = step(EPSILON, abs(sheenStrength));
    return lerp(saturate(specularIntensity), 1.0f, sheenEnabled);
}

float3 EvaluateModernSpecular(
    float3 position,
    float3 normal,
    float3 lightDirection,
    float3 radiance,
    float sheenStrength,
    float specularIntensity,
    float resolvedRoughness)
{
    normal = SafeNormalizeLighting(normal, float3(0.0f, 1.0f, 0.0f));
    lightDirection = SafeNormalizeLighting(lightDirection, normal);
    float3 viewDirection = SafeNormalizeLighting(CamPositionWS.xyz - position, -lightDirection);
    float3 halfVector = SafeNormalizeLighting(viewDirection + lightDirection, normal);

    float normalLight = saturate(dot(normal, lightDirection));
    float normalView = saturate(dot(normal, viewDirection));
    if (normalLight <= EPSILON || normalView <= EPSILON)
        return float3(0.0f, 0.0f, 0.0f);

    resolvedRoughness = clamp(resolvedRoughness, 0.045f, 1.0f);

    float resolvedSpecular = ResolveModernSpecular(specularIntensity, sheenStrength);
    if (resolvedSpecular <= EPSILON)
        return float3(0.0f, 0.0f, 0.0f);

    float sheenEnabled = step(EPSILON, abs(sheenStrength));
    float dielectricReflectance = lerp(0.04f, 0.08f, sheenEnabled) * resolvedSpecular;
    float3 baseReflectance = float3(
        dielectricReflectance,
        dielectricReflectance,
        dielectricReflectance);

    float distribution = DistributionGGX(normal, halfVector, resolvedRoughness);
    float geometry = GeometrySmith(normal, viewDirection, lightDirection, resolvedRoughness);
    float3 fresnel = FresnelSchlick(saturate(dot(halfVector, viewDirection)), baseReflectance);

    float denominator = max(4.0f * normalView * normalLight, EPSILON);
    float3 specularBRDF = (distribution * geometry * fresnel) / denominator;
    specularBRDF = min(specularBRDF, float3(16.0f, 16.0f, 16.0f));

    return max(
        radiance * specularBRDF * normalLight,
        float3(0.0f, 0.0f, 0.0f));
}

float3 DoModernPointLight(float3 position, float3 normal, ShaderLight light)
{
    float3 toLight = light.Position.xyz - position;
    float distanceToLight = length(toLight);
    float3 lightDirection = SafeNormalizeLighting(toLight, normal);
    float attenuation = SmoothRangeAttenuation(distanceToLight, light.In, light.Out);
    float normalLight = saturate(dot(normal, lightDirection));

    return max(
        light.Color.xyz * light.Intensity * attenuation * normalLight,
        float3(0.0f, 0.0f, 0.0f));
}

float3 DoModernSpotLight(float3 position, float3 normal, ShaderLight light)
{
    float3 fromLight = position - light.Position.xyz;
    float distanceToLight = length(fromLight);
    float3 coneDirection = SafeNormalizeLighting(fromLight, light.Direction.xyz);
    float3 lightDirection = -coneDirection;

    float distanceAttenuation = SmoothRangeAttenuation(distanceToLight, light.In, light.Out);
    float coneAttenuation = SmoothSpotAttenuation(
        dot(coneDirection, light.Direction.xyz),
        light.InRange,
        light.OutRange);
    float normalLight = saturate(dot(normal, lightDirection));

    return max(
        light.Color.xyz * light.Intensity * distanceAttenuation * coneAttenuation * normalLight,
        float3(0.0f, 0.0f, 0.0f));
}

float3 DoModernDirectionalLight(float3 position, float3 normal, ShaderLight light)
{
    float3 lightDirection = -SafeNormalizeLighting(light.Direction.xyz, normal);
    float normalLight = saturate(dot(normal, lightDirection));

    return max(
        light.Color.xyz * light.Intensity * normalLight,
        float3(0.0f, 0.0f, 0.0f));
}

float3 DoModernSpecularPoint(
    float3 position,
    float3 normal,
    ShaderLight light,
    float sheenStrength,
    float specularIntensity,
    float resolvedRoughness)
{
    float3 toLight = light.Position.xyz - position;
    float distanceToLight = length(toLight);
    float attenuation = SmoothRangeAttenuation(distanceToLight, light.In, light.Out);
    float3 radiance = light.Color.xyz * light.Intensity * attenuation;

    return EvaluateModernSpecular(
        position,
        normal,
        toLight,
        radiance,
        sheenStrength,
        specularIntensity,
        resolvedRoughness);
}

float3 DoModernSpecularSpot(
    float3 position,
    float3 normal,
    ShaderLight light,
    float sheenStrength,
    float specularIntensity,
    float resolvedRoughness)
{
    float3 fromLight = position - light.Position.xyz;
    float distanceToLight = length(fromLight);
    float3 coneDirection = SafeNormalizeLighting(fromLight, light.Direction.xyz);
    float distanceAttenuation = SmoothRangeAttenuation(distanceToLight, light.In, light.Out);
    float coneAttenuation = SmoothSpotAttenuation(
        dot(coneDirection, light.Direction.xyz),
        light.InRange,
        light.OutRange);
    float3 radiance = light.Color.xyz * light.Intensity * distanceAttenuation * coneAttenuation;

    return EvaluateModernSpecular(
        position,
        normal,
        -coneDirection,
        radiance,
        sheenStrength,
        specularIntensity,
        resolvedRoughness);
}

float3 DoModernSpecularSun(
    float3 position,
    float3 normal,
    ShaderLight light,
    float sheenStrength,
    float specularIntensity,
    float resolvedRoughness)
{
    float3 lightDirection = -SafeNormalizeLighting(light.Direction.xyz, normal);
    float3 radiance = light.Color.xyz * light.Intensity;

    return EvaluateModernSpecular(
        position,
        normal,
        lightDirection,
        radiance,
        sheenStrength,
        specularIntensity,
        resolvedRoughness);
}

#endif // MODERN_LIGHTING
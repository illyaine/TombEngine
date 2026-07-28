#ifndef OBJECT_TRANSFORMS
#define OBJECT_TRANSFORMS

float3 TransformObjectNormal(float3 normal, float3x3 transform)
{
    float3 row0 = transform[0];
    float3 row1 = transform[1];
    float3 row2 = transform[2];

    float3 cofactor0 = cross(row1, row2);
    float determinant = dot(row0, cofactor0);

    // Degenerate transforms cannot produce a valid inverse-transpose matrix.
    // Preserve the previous direction transform as a safe fallback.
    if (abs(determinant) <= EPSILON)
        return SafeNormalize(mul(normal, transform));

    // The cofactor matrix is determinant * inverse-transpose. Normalization
    // removes its magnitude; determinant sign preserves mirrored orientation.
    float3x3 cofactor = float3x3(
        cofactor0,
        cross(row2, row0),
        cross(row0, row1));

    float orientation = determinant < 0.0f ? -1.0f : 1.0f;
    return SafeNormalize(mul(normal, cofactor) * orientation);
}

void TransformObjectTangentBasis(
    float3 sourceNormal,
    float3 sourceTangent,
    float3x3 transform,
    out float3 transformedNormal,
    out float3 transformedTangent,
    out float3 transformedBinormal)
{
    transformedNormal = TransformObjectNormal(sourceNormal, transform);

    // Tangents are regular direction vectors. Re-orthogonalize them against
    // the inverse-transpose normal after applying non-uniform scale.
    transformedTangent = mul(sourceTangent, transform);
    transformedTangent -= transformedNormal * dot(transformedTangent, transformedNormal);
    transformedTangent = SafeNormalize(transformedTangent);

    float3 referenceBinormal = mul(cross(sourceNormal, sourceTangent), transform);
    float3 candidateBinormal = cross(transformedNormal, transformedTangent);
    float handedness = dot(candidateBinormal, referenceBinormal) < 0.0f ? -1.0f : 1.0f;
    transformedBinormal = SafeNormalize(candidateBinormal * handedness);
}

#endif // OBJECT_TRANSFORMS

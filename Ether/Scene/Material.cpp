#include "Material.h"

namespace
{
    struct PlanarUV
    {
        Vector2 UV;
        Vector3 Tangent;
        Vector3 Bitangent;
    };

    PlanarUV ProjectDominant(const Vector3& worldPosition, const Vector3& normal, f32 scale)
    {
        Vector3 absNormal = Vector3::Abs(normal);

        if (absNormal.X >= absNormal.Y && absNormal.X >= absNormal.Z)
            return { Vector2(worldPosition.Y * scale, worldPosition.Z * scale), Vector3::UnitY, Vector3::UnitZ };

        if (absNormal.Y >= absNormal.X && absNormal.Y >= absNormal.Z)
            return { Vector2(worldPosition.X * scale, worldPosition.Z * scale), Vector3::UnitX, Vector3::UnitZ };

        return { Vector2(worldPosition.X * scale, worldPosition.Y * scale), Vector3::UnitX, Vector3::UnitY };
    }
}

MaterialSample Material::Evaluate(const Vector3& worldPosition, const Vector3& normal, const Vector2& uvCoordinates, bool hasUV) const
{
    MaterialSample sample { Albedo, Roughness, Metallic, Emission, normal, IOR, Transmission };

    if (!AlbedoMap && !RoughnessMap && !EmissionMap && !BumpMap)
        return sample;

    PlanarUV uv;
    if (hasUV)
    {
        uv.UV = uvCoordinates * TextureScale;

        Vector3 up   = (std::abs(normal.Y) < 0.999f) ? Vector3::UnitY : Vector3::UnitX;
        uv.Tangent   = normal.Cross(up).Normalized();
        uv.Bitangent = normal.Cross(uv.Tangent).Normalized();
    }
    else
    {
        uv = ProjectDominant(worldPosition, normal, TextureScale);
    }

    if (AlbedoMap)    sample.Albedo    = AlbedoMap->Sample(uv.UV);
    if (RoughnessMap) sample.Roughness = RoughnessMap->SampleScalar(uv.UV);
    if (EmissionMap)  sample.Emission  = EmissionMap->Sample(uv.UV);

    if (BumpMap)
    {
        f32 du = 1.0f / static_cast<f32>(BumpMap->Width());
        f32 dv = 1.0f / static_cast<f32>(BumpMap->Height());

        f32 hL = BumpMap->SampleScalar(uv.UV - Vector2(du, 0.0f));
        f32 hR = BumpMap->SampleScalar(uv.UV + Vector2(du, 0.0f));
        f32 hB = BumpMap->SampleScalar(uv.UV - Vector2(0.0f, dv));
        f32 hT = BumpMap->SampleScalar(uv.UV + Vector2(0.0f, dv));

        f32 dHdu = (hR - hL) * 0.5f;
        f32 dHdv = (hT - hB) * 0.5f;

        Vector3 tangent   = (uv.Tangent   - normal * normal.Dot(uv.Tangent)).Normalized();
        Vector3 bitangent = (uv.Bitangent - normal * normal.Dot(uv.Bitangent)).Normalized();

        sample.Normal = (normal - (tangent * dHdu + bitangent * dHdv) * BumpStrength).Normalized();
    }

    return sample;
}
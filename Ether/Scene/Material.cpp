#include "Material.h"

namespace
{
	struct PlanarUV
	{
		f32     U, V;
		Vector3 Tangent;
		Vector3 Bitangent;
	};

	PlanarUV ProjectDominant(const Vector3& worldPosition, const Vector3& normal, f32 scale)
	{
		Vector3 absNormal = Vector3::Abs(normal);

		if (absNormal.X >= absNormal.Y && absNormal.X >= absNormal.Z)
			return { worldPosition.Y * scale, worldPosition.Z * scale, Vector3::UnitY, Vector3::UnitZ };

		if (absNormal.Y >= absNormal.X && absNormal.Y >= absNormal.Z)
			return { worldPosition.X * scale, worldPosition.Z * scale, Vector3::UnitX, Vector3::UnitZ };

		return { worldPosition.X * scale, worldPosition.Y * scale, Vector3::UnitX, Vector3::UnitY };
	}
}

MaterialSample Material::Evaluate(const Vector3& worldPosition, const Vector3& normal) const
{
	MaterialSample sample{ Albedo, Roughness, Metallic, Emission, normal, IOR, Transmission };

	if (!AlbedoMap && !RoughnessMap && !EmissionMap && !BumpMap)
		return sample;

	PlanarUV uv = ProjectDominant(worldPosition, normal, TextureScale);

	if (AlbedoMap)    sample.Albedo    = AlbedoMap->Sample(uv.U, uv.V);
	if (RoughnessMap) sample.Roughness = RoughnessMap->SampleScalar(uv.U, uv.V);
	if (EmissionMap)  sample.Emission  = EmissionMap->Sample(uv.U, uv.V);

	if (BumpMap)
	{
		constexpr f32 Epsilon = 0.01f;
		auto s = [&](f32 u, f32 v){ return BumpMap->SampleScalar(u, v); };

		f32 dHdu = (s(uv.U + Epsilon, uv.V) - s(uv.U - Epsilon, uv.V)) / (2.0f * Epsilon);
		f32 dHdv = (s(uv.U, uv.V + Epsilon) - s(uv.U, uv.V - Epsilon)) / (2.0f * Epsilon);

		Vector3 tangent   = (uv.Tangent   - normal * normal.Dot(uv.Tangent)).Normalized();
		Vector3 bitangent = (uv.Bitangent - normal * normal.Dot(uv.Bitangent)).Normalized();

		sample.Normal = (normal - (tangent * dHdu + bitangent * dHdv) * BumpStrength).Normalized();
	}

	return sample;
}
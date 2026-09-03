#pragma once

#include "../Math/Vector3.h"
#include "Texture.h"

#include <memory>

struct MaterialSample
{
	Vector3 Albedo;
	f32		Roughness;
	f32		Metallic;
	Vector3 Emission;
	Vector3 Normal;
	f32		IOR;
	f32		Transmission;
};

struct Material
{
	Vector3 Albedo = Vector3(0.8f);
	using TexturePtr = std::shared_ptr<Texture>;
	TexturePtr AlbedoMap;

	f32 Roughness = 0.5f;
	TexturePtr RoughnessMap;

	f32 Metallic = 0.0f;

	Vector3 Emission = Vector3::Zero;
	TexturePtr EmissionMap;

	TexturePtr BumpMap;
	f32 BumpStrength = 1.0f;

	f32 TextureScale = 1.0f;

	f32 IOR		     = 1.5f;
	f32 Transmission = 0.0f;

	MaterialSample Evaluate(const Vector3& worldPosition, const Vector3& normal) const;
};
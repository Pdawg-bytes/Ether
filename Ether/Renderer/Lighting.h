#pragma once

#include "PointLight.h"
#include "../Math/Vector3.h"

#include <vector>

class BVH;

struct SurfacePoint
{
	Vector3 Position;
	Vector3 Normal;
	Vector3 Albedo;
	f32		Roughness;
	f32		Metallic;
	Vector3 Emission;
};

class Lighting
{
public:
	void AddPointLight(const PointLight& light);

	Vector3 Shade(const BVH& bvh, const SurfacePoint& surface, const Vector3& viewDir) const;

private:
	bool IsOccluded(const BVH& bvh, const Vector3& origin, const Vector3& direction, f32 maxDistance) const;
	f32 SoftShadow(const BVH& bvh, const Vector3& origin, const Vector3& direction, f32 minT, f32 maxT, f32 k) const;

	std::vector<PointLight> _lights;
};
#pragma once

#include "../Math/Vector3.h"
#include "../Math/Quaternion.h"
#include "../Math/AABB.h"

struct SceneObject;

using SDFDistanceFunc = f32(*)(const SceneObject& object, const Vector3& localPoint);
using SDFNormalFunc   = Vector3(*)(const SceneObject& object, const Vector3& localPoint);

namespace SDF
{
	struct SphereData { f32 Radius; };
	struct BoxData    { Vector3 Extents; };
	struct PlaneData  { Vector3 Normal; f32 Distance; };
}

union PrimitiveData
{
	SDF::SphereData Sphere;
	SDF::BoxData    Box;
	SDF::PlaneData  Plane;
};

struct SceneObject
{
	Vector3 Position;
	Quaternion Rotation = Quaternion::Identity;
	f32 Scale		    = 1.0f;
	bool IsBounded      = true;

	AABB WorldBounds;

	SDFDistanceFunc DistanceFunc = nullptr;
	SDFNormalFunc NormalFunc     = nullptr;
	PrimitiveData Data{};

	f32 Distance(const Vector3& worldPoint) const;
	Vector3 Normal(const Vector3& worldPoint) const;

	static SceneObject CreateSphere(const Vector3& position, f32 radius, f32 scale = 1.0f);
	static SceneObject CreateBox(const Vector3& position, const Quaternion& rotation, const Vector3& extents, f32 scale = 1.0f);
	static SceneObject CreatePlane(const Vector3& normal, f32 distance);

private:
	void UpdateWorldBounds(const AABB& localBounds);
};
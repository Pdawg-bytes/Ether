#pragma once

#include "../Math/Vector3.h"
#include "../Math/Quaternion.h"
#include "../Math/AABB.h"
#include "SDFPrimitives.h"

#include <memory>

class CSGTree;

struct SceneObject
{
	Vector3 Position;
	Quaternion Rotation = Quaternion::Identity;
	Vector3 Scale	    = Vector3(1.0f);
	bool IsBounded      = true;

	AABB WorldBounds;

	f32 BoundingRadius = 0.0f;

	SDFDistanceFunc DistanceFunc = nullptr;
	SDFNormalFunc NormalFunc     = nullptr;
	PrimitiveData Data{};
	std::shared_ptr<CSGTree> Template;

	f32 Distance(const Vector3& worldPoint) const;
	Vector3 Normal(const Vector3& worldPoint) const;

	static SceneObject CreateSphere(const Vector3& position, f32 radius, const Vector3& scale = Vector3(1.0f));
	static SceneObject CreateBox(const Vector3& position, const Quaternion& rotation, const Vector3& extents, const Vector3& scale = Vector3(1.0f));
	static SceneObject CreatePlane(const Vector3& normal, f32 distance);
	static SceneObject CreateFromTemplate(std::shared_ptr<CSGTree> tmpl, const Vector3& position, const Quaternion& rotation = Quaternion::Identity, const Vector3& scale = Vector3(1.0f));

private:
	void UpdateWorldBounds(const AABB& localBounds);
};
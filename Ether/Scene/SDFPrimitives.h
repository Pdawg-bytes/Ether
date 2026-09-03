#pragma once

#include "../Math/Vector3.h"

struct SceneObject;

namespace SDF
{
	f32 SphereDistance(const SceneObject& object, const Vector3& localPoint);
	Vector3 SphereNormal(const SceneObject& object, const Vector3& localPoint);

	f32 BoxDistance(const SceneObject& object, const Vector3& localPoint);
	Vector3 BoxNormal(const SceneObject& object, const Vector3& localPoint);

	f32 PlaneDistance(const SceneObject& object, const Vector3& localPoint);
	Vector3 PlaneNormal(const SceneObject& object, const Vector3& localPoint);
}
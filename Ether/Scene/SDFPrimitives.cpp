#include "SDFPrimitives.h"
#include "SceneObject.h"

#include <algorithm>
#include <cmath>

namespace SDF
{
	f32 SphereDistance(const SceneObject& object, const Vector3& localPoint)
	{
		return localPoint.Length() - object.Data.Sphere.Radius;
	}

	Vector3 SphereNormal(const SceneObject& object, const Vector3& localPoint)
	{
		return localPoint.Normalized();
	}


	f32 BoxDistance(const SceneObject& object, const Vector3& localPoint)
	{
		const Vector3& extents = object.Data.Box.Extents;

		Vector3 q(
			std::abs(localPoint.X) - extents.X,
			std::abs(localPoint.Y) - extents.Y,
			std::abs(localPoint.Z) - extents.Z
		);

		f32 outsideDistance = Vector3::Max(q, Vector3::Zero).Length();
		f32 insideDistance  = std::min(std::max({ q.X, q.Y, q.Z }), 0.0f);

		return outsideDistance + insideDistance;
	}

	Vector3 BoxNormal(const SceneObject& object, const Vector3& localPoint)
	{
		const Vector3& extents = object.Data.Box.Extents;

		Vector3 q(
			std::abs(localPoint.X) - extents.X,
			std::abs(localPoint.Y) - extents.Y,
			std::abs(localPoint.Z) - extents.Z
		);

		if (q.X > q.Y && q.X > q.Z)
			return Vector3(localPoint.X > 0.0f ? 1.0f : -1.0f, 0.0f, 0.0f);

		if (q.Y > q.Z)
			return Vector3(0.0f, localPoint.Y > 0.0f ? 1.0f : -1.0f, 0.0f);

		return Vector3(0.0f, 0.0f, localPoint.Z > 0.0f ? 1.0f : -1.0f);
	}


	f32 PlaneDistance(const SceneObject& object, const Vector3& localPoint)
	{
		return localPoint.Dot(object.Data.Plane.Normal) + object.Data.Plane.Distance;
	}

	Vector3 PlaneNormal(const SceneObject& object, const Vector3& localPoint)
	{
		return object.Data.Plane.Normal;
	}
}
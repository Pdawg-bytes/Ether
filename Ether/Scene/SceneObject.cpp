#include "SceneObject.h"
#include "SDFPrimitives.h"

f32 SceneObject::Distance(const Vector3& worldPoint) const
{
	Vector3 localPoint = Rotation.Conjugate().Rotate(worldPoint - Position) * (1.0f / Scale);
	return DistanceFunc(*this, localPoint) * Scale;
}

Vector3 SceneObject::Normal(const Vector3& worldPoint) const
{
	Vector3 localPoint  = Rotation.Conjugate().Rotate(worldPoint - Position) * (1.0f / Scale);
	Vector3 localNormal = NormalFunc(*this, localPoint);

	return Rotation.Rotate(localNormal).Normalized();
}

void SceneObject::UpdateWorldBounds(const AABB& localBounds)
{
	Vector3 corners[8] =
	{
		Vector3(localBounds.Min.X, localBounds.Min.Y, localBounds.Min.Z),
		Vector3(localBounds.Max.X, localBounds.Min.Y, localBounds.Min.Z),
		Vector3(localBounds.Min.X, localBounds.Max.Y, localBounds.Min.Z),
		Vector3(localBounds.Max.X, localBounds.Max.Y, localBounds.Min.Z),
		Vector3(localBounds.Min.X, localBounds.Min.Y, localBounds.Max.Z),
		Vector3(localBounds.Max.X, localBounds.Min.Y, localBounds.Max.Z),
		Vector3(localBounds.Min.X, localBounds.Max.Y, localBounds.Max.Z),
		Vector3(localBounds.Max.X, localBounds.Max.Y, localBounds.Max.Z),
	};

	AABB worldBounds;
	for (const Vector3& corner : corners)
	{
		Vector3 worldCorner = Position + Rotation.Rotate(corner * Scale);
		worldBounds.Min	    = Vector3::Min(worldBounds.Min, worldCorner);
		worldBounds.Max		= Vector3::Max(worldBounds.Max, worldCorner);
	}

	WorldBounds = worldBounds;
}


SceneObject SceneObject::CreateSphere(const Vector3& position, f32 radius, f32 scale)
{
	SceneObject object;
	object.Position			  = position;
	object.Scale		      = scale;
	object.DistanceFunc		  = SDF::SphereDistance;
	object.NormalFunc		  = SDF::SphereNormal;
	object.Data.Sphere.Radius = radius;

	object.UpdateWorldBounds(AABB(Vector3(-radius), Vector3(radius)));
	return object;
}

SceneObject SceneObject::CreateBox(const Vector3& position, const Quaternion& rotation, const Vector3& extents, f32 scale)
{
	SceneObject object;
	object.Position			= position;
	object.Rotation			= rotation;
	object.Scale            = scale;
	object.DistanceFunc		= SDF::BoxDistance;
	object.NormalFunc		= SDF::BoxNormal;
	object.Data.Box.Extents = extents;

	object.UpdateWorldBounds(AABB(-extents, extents));
	return object;
}

SceneObject SceneObject::CreatePlane(const Vector3& normal, f32 distance)
{
	SceneObject object;
	object.Position				 = Vector3::Zero;
	object.DistanceFunc			 = SDF::PlaneDistance;
	object.NormalFunc			 = SDF::PlaneNormal;
	object.Data.Plane.Normal	 = normal.Normalized();
	object.Data.Plane.Distance	 = distance;
	object.IsBounded			 = false;

	return object;
}
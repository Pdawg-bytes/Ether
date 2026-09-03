#include "SceneObject.h"
#include "SDFPrimitives.h"
#include "CSGTree.h"
#include "../Math/MathUtil.h"

#include <algorithm>

f32 SceneObject::Distance(const Vector3& worldPoint) const
{
    Vector3 localPoint = Rotation.Conjugate().Rotate(worldPoint - Position) / Scale;
    f32 rawDistance    = Template ? Template->Distance(localPoint) : DistanceFunc(Data, localPoint);
    f32 minScale       = std::min(Scale.X, std::min(Scale.Y, Scale.Z));

    return rawDistance * minScale;
}

Vector3 SceneObject::Normal(const Vector3& worldPoint) const
{
    Vector3 localPoint  = Rotation.Conjugate().Rotate(worldPoint - Position) / Scale;
    Vector3 localNormal = Template ? Template->Normal(localPoint) : NormalFunc(Data, localPoint);

    Vector3 correctedNormal(localNormal.X / Scale.X, localNormal.Y / Scale.Y, localNormal.Z / Scale.Z);
    return Rotation.Rotate(correctedNormal).Normalized();
}

s32 SceneObject::ResolveMaterialIndex(const Vector3& worldPoint) const
{
    if (!Template)
        return MaterialIndex;

    Vector3 localPoint = Rotation.Conjugate().Rotate(worldPoint - Position) / Scale;
    s32 leafMaterial    = Template->MaterialIndex(localPoint);

    return leafMaterial >= 0 ? leafMaterial : MaterialIndex;
}

void SceneObject::UpdateWorldBounds(const AABB& localBounds)
{
    Vector3 scaledMin = localBounds.Min * Scale;
    Vector3 scaledMax = localBounds.Max * Scale;
    AABB scaledBounds(scaledMin, scaledMax);
    
    WorldBounds = Math::TransformBounds(scaledBounds, Position, Rotation, 1.0f);
    
    BoundingRadius = 0.0f;
    BoundingRadius = std::max(BoundingRadius, scaledMin.Length());
    BoundingRadius = std::max(BoundingRadius, scaledMax.Length());
    BoundingRadius = std::max(BoundingRadius, Vector3(scaledMin.X, scaledMin.Y, scaledMax.Z).Length());
    BoundingRadius = std::max(BoundingRadius, Vector3(scaledMin.X, scaledMax.Y, scaledMin.Z).Length());
    BoundingRadius = std::max(BoundingRadius, Vector3(scaledMin.X, scaledMax.Y, scaledMax.Z).Length());
    BoundingRadius = std::max(BoundingRadius, Vector3(scaledMax.X, scaledMin.Y, scaledMin.Z).Length());
    BoundingRadius = std::max(BoundingRadius, Vector3(scaledMax.X, scaledMin.Y, scaledMax.Z).Length());
    BoundingRadius = std::max(BoundingRadius, Vector3(scaledMax.X, scaledMax.Y, scaledMin.Z).Length());
}


SceneObject SceneObject::CreateSphere(const Vector3& position, f32 radius, const Vector3& scale, s32 materialIndex)
{
    SceneObject object;
    object.Position			  = position;
    object.Scale		      = scale;
    object.DistanceFunc		  = SDF::SphereDistance;
    object.NormalFunc		  = SDF::SphereNormal;
    object.Data.Sphere.Radius = radius;
    object.MaterialIndex	  = materialIndex;

    object.UpdateWorldBounds(AABB(Vector3(-radius), Vector3(radius)));
    return object;
}

SceneObject SceneObject::CreateBox(const Vector3& position, const Quaternion& rotation, const Vector3& extents, const Vector3& scale, s32 materialIndex)
{
    SceneObject object;
    object.Position			= position;
    object.Rotation			= rotation;
    object.Scale            = scale;
    object.DistanceFunc		= SDF::BoxDistance;
    object.NormalFunc		= SDF::BoxNormal;
    object.Data.Box.Extents = extents;
    object.MaterialIndex	= materialIndex;

    object.UpdateWorldBounds(AABB(-extents, extents));
    return object;
}

SceneObject SceneObject::CreatePlane(const Vector3& normal, f32 distance, s32 materialIndex)
{
    SceneObject object;
    object.Position				 = Vector3::Zero;
    object.DistanceFunc			 = SDF::PlaneDistance;
    object.NormalFunc			 = SDF::PlaneNormal;
    object.Data.Plane.Normal	 = normal.Normalized();
    object.Data.Plane.Distance	 = distance;
    object.IsBounded			 = false;
    object.MaterialIndex		 = materialIndex;

    return object;
}

SceneObject SceneObject::CreateFromTemplate(std::shared_ptr<CSGTree> tmpl, const Vector3& position, const Quaternion& rotation, const Vector3& scale, s32 materialIndex)
{
    SceneObject object;
    object.Position = position;
    object.Rotation = rotation;
    object.Scale	= scale;
    object.Template = std::move(tmpl);
    object.MaterialIndex = materialIndex;

    object.UpdateWorldBounds(object.Template->LocalBounds());
    return object;
}
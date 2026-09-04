#include "SDFPrimitives.h"

#include <algorithm>
#include <cmath>

namespace SDF
{
    PrimitiveDefinition MakeSphere(f32 radius)
    {
        PrimitiveDefinition definition;
        definition.DistanceFunc       = SphereDistance;
        definition.NormalFunc         = SphereNormal;
        definition.Data.Sphere.Radius = radius;
        definition.LocalBounds        = AABB(Vector3(-radius), Vector3(radius));

        return definition;
    }

    PrimitiveDefinition MakeBox(const Vector3& extents)
    {
        PrimitiveDefinition definition;
        definition.DistanceFunc     = BoxDistance;
        definition.NormalFunc       = BoxNormal;
        definition.Data.Box.Extents = extents;
        definition.LocalBounds      = AABB(-extents, extents);

        return definition;
    }

    PrimitiveDefinition MakeTorus(f32 majorRadius, f32 minorRadius)
    {
        PrimitiveDefinition definition;
        definition.DistanceFunc           = TorusDistance;
        definition.NormalFunc             = TorusNormal;
        definition.Data.Torus.MajorRadius = majorRadius;
        definition.Data.Torus.MinorRadius = minorRadius;

        f32 outerRadius        = majorRadius + minorRadius;
        definition.LocalBounds = AABB(Vector3(-outerRadius, -minorRadius, -outerRadius), Vector3(outerRadius, minorRadius, outerRadius));

        return definition;
    }

    PrimitiveDefinition MakeCylinder(f32 radius, f32 halfHeight)
    {
        PrimitiveDefinition definition;
        definition.DistanceFunc             = CylinderDistance;
        definition.NormalFunc               = CylinderNormal;
        definition.Data.Cylinder.Radius     = radius;
        definition.Data.Cylinder.HalfHeight = halfHeight;
        definition.LocalBounds              = AABB(Vector3(-radius, -halfHeight, -radius), Vector3(radius, halfHeight, radius));

        return definition;
    }



    f32 SphereDistance(const PrimitiveData& data, const Vector3& localPoint)
    {
        return localPoint.Length() - data.Sphere.Radius;
    }

    Vector3 SphereNormal(const PrimitiveData& data, const Vector3& localPoint)
    {
        return localPoint.Normalized();
    }


    f32 BoxDistance(const PrimitiveData& data, const Vector3& localPoint)
    {
        const Vector3& extents = data.Box.Extents;

        Vector3 q(
            std::abs(localPoint.X) - extents.X,
            std::abs(localPoint.Y) - extents.Y,
            std::abs(localPoint.Z) - extents.Z
        );

        f32 outsideDistance = Vector3::Max(q, Vector3::Zero).Length();
        f32 insideDistance  = std::min(std::max(q.X, std::max(q.Y, q.Z)), 0.0f);

        return outsideDistance + insideDistance;
    }

    Vector3 BoxNormal(const PrimitiveData& data, const Vector3& localPoint)
    {
        const Vector3& extents = data.Box.Extents;

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


    f32 PlaneDistance(const PrimitiveData& data, const Vector3& localPoint)
    {
        return localPoint.Dot(data.Plane.Normal) + data.Plane.Distance;
    }

    Vector3 PlaneNormal(const PrimitiveData& data, const Vector3& localPoint)
    {
        return data.Plane.Normal;
    }


    f32 TorusDistance(const PrimitiveData& data, const Vector3& localPoint)
    {
        f32 xzLength = std::sqrt(localPoint.X * localPoint.X + localPoint.Z * localPoint.Z);
        Vector3 q(xzLength - data.Torus.MajorRadius, localPoint.Y, 0.0f);

        return q.Length() - data.Torus.MinorRadius;
    }

    Vector3 TorusNormal(const PrimitiveData& data, const Vector3& localPoint)
    {
        Vector3 xz(localPoint.X, 0.0f, localPoint.Z);
        Vector3 ringCenter = xz.Normalized() * data.Torus.MajorRadius;

        return (localPoint - ringCenter).Normalized();
    }


    f32 CylinderDistance(const PrimitiveData& data, const Vector3& localPoint)
    {
        f32 xzLength = std::sqrt(localPoint.X * localPoint.X + localPoint.Z * localPoint.Z);

        f32 dx = std::abs(xzLength) - data.Cylinder.Radius;
        f32 dy = std::abs(localPoint.Y) - data.Cylinder.HalfHeight;

        f32 outsideDistance = Vector3::Max(Vector3(dx, dy, 0.0f), Vector3::Zero).Length();
        f32 insideDistance  = std::min(std::max(dx, dy), 0.0f);

        return outsideDistance + insideDistance;
    }

    Vector3 CylinderNormal(const PrimitiveData& data, const Vector3& localPoint)
    {
        f32 xzLength = std::sqrt(localPoint.X * localPoint.X + localPoint.Z * localPoint.Z);

        f32 dx = xzLength - data.Cylinder.Radius;
        f32 dy = std::abs(localPoint.Y) - data.Cylinder.HalfHeight;

        if (dx > dy)
            return Vector3(localPoint.X, 0.0f, localPoint.Z).Normalized();

        return Vector3(0.0f, localPoint.Y > 0.0f ? 1.0f : -1.0f, 0.0f);
    }
}
#include "Primitives.h"
#include "../Math/MathUtil.h"

#include <algorithm>
#include <cmath>
#include <initializer_list>

namespace Primitives
{
    PrimitiveDefinition MakeSphere(f32 radius)
    {
        PrimitiveDefinition definition;
        definition.Type               = PrimitiveType::Sphere;
        definition.DistanceFunc       = SphereDistance;
        definition.NormalFunc         = SphereNormal;
        definition.IntersectFunc      = SphereIntersect;
        definition.Data.Sphere.Radius = radius;
        definition.LocalBounds        = AABB(Vector3(-radius), Vector3(radius));

        return definition;
    }

    PrimitiveDefinition MakeBox(const Vector3& extents)
    {
        PrimitiveDefinition definition;
        definition.Type             = PrimitiveType::Box;
        definition.DistanceFunc     = BoxDistance;
        definition.NormalFunc       = BoxNormal;
        definition.IntersectFunc    = BoxIntersect;
        definition.Data.Box.Extents = extents;
        definition.LocalBounds      = AABB(-extents, extents);

        return definition;
    }

    PrimitiveDefinition MakePlane(const Vector3& normal, f32 distance)
    {
        PrimitiveDefinition definition;
        definition.Type                = PrimitiveType::Plane;
        definition.DistanceFunc        = PlaneDistance;
        definition.NormalFunc          = PlaneNormal;
        definition.IntersectFunc       = PlaneIntersect;
        definition.Data.Plane.Normal   = normal.Normalized();
        definition.Data.Plane.Distance = distance;
        definition.LocalBounds         = AABB();

        return definition;
    }

    PrimitiveDefinition MakeTorus(f32 majorRadius, f32 minorRadius)
    {
        PrimitiveDefinition definition;
        definition.Type                   = PrimitiveType::Torus;
        definition.DistanceFunc           = TorusDistance;
        definition.NormalFunc             = TorusNormal;
        definition.IntersectFunc          = TorusIntersect;
        definition.Data.Torus.MajorRadius = majorRadius;
        definition.Data.Torus.MinorRadius = minorRadius;

        f32 outerRadius        = majorRadius + minorRadius;
        definition.LocalBounds = AABB(Vector3(-outerRadius, -minorRadius, -outerRadius), Vector3(outerRadius, minorRadius, outerRadius));

        return definition;
    }

    PrimitiveDefinition MakeCylinder(f32 radius, f32 halfHeight)
    {
        PrimitiveDefinition definition;
        definition.Type                     = PrimitiveType::Cylinder;
        definition.DistanceFunc             = CylinderDistance;
        definition.NormalFunc               = CylinderNormal;
        definition.IntersectFunc            = CylinderIntersect;
        definition.Data.Cylinder.Radius     = radius;
        definition.Data.Cylinder.HalfHeight = halfHeight;
        definition.LocalBounds              = AABB(Vector3(-radius, -halfHeight, -radius), Vector3(radius, halfHeight, radius));

        return definition;
    }

    PrimitiveDefinition MakeTriangle(const Vector3& v0, const Vector3& v1, const Vector3& v2,
                                     const Vector2& uv0, const Vector2& uv1, const Vector2& uv2,
                                     bool hasUV)
    {
        PrimitiveDefinition definition;
        definition.Type                   = PrimitiveType::Triangle;
        definition.DistanceFunc           = TriangleDistance;
        definition.NormalFunc             = TriangleNormal;
        definition.IntersectFunc          = TriangleIntersect;
        definition.Data.Triangle.V0       = v0;
        definition.Data.Triangle.V1       = v1;
        definition.Data.Triangle.V2       = v2;
        definition.Data.Triangle.Normal   = (v1 - v0).Cross(v2 - v0).Normalized();
        definition.Data.Triangle.UV0      = uv0;
        definition.Data.Triangle.UV1      = uv1;
        definition.Data.Triangle.UV2      = uv2;
        definition.Data.Triangle.HasUV    = hasUV;

        Vector3 minBounds = Vector3::Min(v0, Vector3::Min(v1, v2));
        Vector3 maxBounds = Vector3::Max(v0, Vector3::Max(v1, v2));
        definition.LocalBounds = AABB(minBounds, maxBounds);

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

    bool SphereIntersect(const PrimitiveData& data, const Ray& localRay, f32 minT, f32 maxT, f32& outT, Vector3& outNormal, Vector2& outUV, bool& outHasUV)
    {
        f32 radius = data.Sphere.Radius;
        Vector3 oc = localRay.Origin;
        f32 a      = localRay.Direction.LengthSquared();
        f32 halfB  = oc.Dot(localRay.Direction);
        f32 c      = oc.LengthSquared() - radius * radius;

        f32 discriminant = halfB * halfB - a * c;

        if (discriminant < 0.0f)
            return false;

        f32 sqrtD = std::sqrt(discriminant);
        f32 root  = (-halfB - sqrtD) / a;

        if (root < minT || root > maxT)
        {
            root = (-halfB + sqrtD) / a;
            if (root < minT || root > maxT)
                return false;
        }

        outT               = root;
        Vector3 localPoint = localRay.At(outT);
        outNormal          = SphereNormal(data, localPoint);

        f32 phi   = std::atan2(localPoint.Z, localPoint.X);
        f32 theta = std::asin(std::clamp(localPoint.Y / radius, -1.0f, 1.0f));

        outUV    = Vector2(0.5f + phi / (2.0f * Math::PI), 0.5f + theta / Math::PI);
        outHasUV = true;

        return true;
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

    bool BoxIntersect(const PrimitiveData& data, const Ray& localRay, f32 minT, f32 maxT, f32& outT, Vector3& outNormal, Vector2& outUV, bool& outHasUV)
    {
        const Vector3& extents = data.Box.Extents;
        AABB localBox(-extents, extents);
        f32 t0, t1;
        if (!localBox.Intersect(localRay, minT, maxT, t0, t1))
            return false;

        f32 hitT = (t0 >= minT) ? t0 : t1;
        if (hitT < minT || hitT > maxT)
            return false;

        outT = hitT;
        Vector3 p = localRay.At(outT);

        Vector3 absP = Vector3::Abs(p);
        Vector3 d(absP.X / extents.X, absP.Y / extents.Y, absP.Z / extents.Z);

        if (d.X >= d.Y && d.X >= d.Z)
        {
            outNormal = Vector3(p.X > 0.0f ? 1.0f : -1.0f, 0.0f, 0.0f);
            outUV = Vector2((p.Z / extents.Z + 1.0f) * 0.5f, (p.Y / extents.Y + 1.0f) * 0.5f);
        }
        else if (d.Y >= d.X && d.Y >= d.Z)
        {
            outNormal = Vector3(0.0f, p.Y > 0.0f ? 1.0f : -1.0f, 0.0f);
            outUV = Vector2((p.X / extents.X + 1.0f) * 0.5f, (p.Z / extents.Z + 1.0f) * 0.5f);
        }
        else
        {
            outNormal = Vector3(0.0f, 0.0f, p.Z > 0.0f ? 1.0f : -1.0f);
            outUV = Vector2((p.X / extents.X + 1.0f) * 0.5f, (p.Y / extents.Y + 1.0f) * 0.5f);
        }

        outHasUV = true;
        return true;
    }


    f32 PlaneDistance(const PrimitiveData& data, const Vector3& localPoint)
    {
        return localPoint.Dot(data.Plane.Normal) + data.Plane.Distance;
    }

    Vector3 PlaneNormal(const PrimitiveData& data, const Vector3& localPoint)
    {
        return data.Plane.Normal;
    }

    bool PlaneIntersect(const PrimitiveData& data, const Ray& localRay, f32 minT, f32 maxT, f32& outT, Vector3& outNormal, Vector2& outUV, bool& outHasUV)
    {
        f32 denom = data.Plane.Normal.Dot(localRay.Direction);
        if (std::abs(denom) < 1e-6f)
            return false;

        f32 t = -(data.Plane.Normal.Dot(localRay.Origin) + data.Plane.Distance) / denom;
        if (t < minT || t > maxT)
            return false;

        outT      = t;
        outNormal = (denom < 0.0f) ? data.Plane.Normal : -data.Plane.Normal;
        Vector3 p = localRay.At(outT);
        outUV     = Vector2(p.X, p.Z);
        outHasUV  = false;

        return true;
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

    bool TorusIntersect(const PrimitiveData& data, const Ray& localRay, f32 minT, f32 maxT, f32& outT, Vector3& outNormal, Vector2& outUV, bool& outHasUV)
    {
        f32 outerRadius = data.Torus.MajorRadius + data.Torus.MinorRadius;

        AABB bounds(Vector3(-outerRadius, -data.Torus.MinorRadius, -outerRadius), Vector3(outerRadius, data.Torus.MinorRadius, outerRadius));

        f32 t0, t1;
        if (!bounds.Intersect(localRay, minT, maxT, t0, t1))
            return false;

        f32 t    = std::max(t0, minT);
        f32 endT = std::min(t1, maxT);

        constexpr s32 MaxSteps = 48;
        constexpr f32 MinDist  = 0.0005f;

        for (s32 i = 0; i < MaxSteps && t <= endT; ++i)
        {
            Vector3 p = localRay.At(t);
            f32 d     = TorusDistance(data, p);

            if (d < MinDist)
            {
                outT      = t;
                outNormal = TorusNormal(data, p);
                outUV     = Vector2(p.X, p.Z);
                outHasUV  = false;

                return true;
            }

            t += std::max(d, 0.001f);
        }

        return false;
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

    bool CylinderIntersect(const PrimitiveData& data, const Ray& localRay, f32 minT, f32 maxT, f32& outT, Vector3& outNormal, Vector2& outUV, bool& outHasUV)
    {
        f32 radius     = data.Cylinder.Radius;
        f32 halfHeight = data.Cylinder.HalfHeight;

        f32 bestT          = maxT + 1.0f;
        Vector3 bestNormal = Vector3::Zero;

        f32 a = localRay.Direction.X * localRay.Direction.X + localRay.Direction.Z * localRay.Direction.Z;
        if (a > 1e-8f)
        {
            f32 b = 2.0f * (localRay.Origin.X * localRay.Direction.X + localRay.Origin.Z * localRay.Direction.Z);
            f32 c = localRay.Origin.X * localRay.Origin.X + localRay.Origin.Z * localRay.Origin.Z - radius * radius;

            f32 disc = b * b - 4.0f * a * c;
            if (disc >= 0.0f)
            {
                f32 sqrtDisc = std::sqrt(disc);
                f32 t1       = (-b - sqrtDisc) / (2.0f * a);
                f32 t2       = (-b + sqrtDisc) / (2.0f * a);

                for (f32 t : { t1, t2 })
                {
                    if (t >= minT && t <= maxT && t < bestT)
                    {
                        f32 y = localRay.Origin.Y + t * localRay.Direction.Y;

                        if (y >= -halfHeight && y <= halfHeight)
                        {
                            bestT      = t;
                            Vector3 p  = localRay.At(t);
                            bestNormal = Vector3(p.X / radius, 0.0f, p.Z / radius).Normalized();
                        }
                    }
                }
            }
        }

        if (std::abs(localRay.Direction.Y) > 1e-8f)
        {
            f32 tTop = (halfHeight - localRay.Origin.Y) / localRay.Direction.Y;
            if (tTop >= minT && tTop <= maxT && tTop < bestT)
            {
                Vector3 p = localRay.At(tTop);

                if (p.X * p.X + p.Z * p.Z <= radius * radius)
                {
                    bestT      = tTop;
                    bestNormal = Vector3(0.0f, 1.0f, 0.0f);
                }
            }

            f32 tBottom = (-halfHeight - localRay.Origin.Y) / localRay.Direction.Y;
            if (tBottom >= minT && tBottom <= maxT && tBottom < bestT)
            {
                Vector3 p = localRay.At(tBottom);

                if (p.X * p.X + p.Z * p.Z <= radius * radius)
                {
                    bestT      = tBottom;
                    bestNormal = Vector3(0.0f, -1.0f, 0.0f);
                }
            }
        }

        if (bestT > maxT)
            return false;

        outT      = bestT;
        outNormal = bestNormal;
        Vector3 p = localRay.At(outT);
        f32 phi   = std::atan2(p.Z, p.X);
        outUV     = Vector2(0.5f + phi / (2.0f * Math::PI), (p.Y + halfHeight) / (2.0f * halfHeight));
        outHasUV  = true;

        return true;
    }


    f32 TriangleDistance(const PrimitiveData& data, const Vector3& p)
    {
        Vector3 v10 = data.Triangle.V1 - data.Triangle.V0;
        Vector3 v20 = data.Triangle.V2 - data.Triangle.V0;
        Vector3 p0  = p - data.Triangle.V0;

        f32 d00 = v10.Dot(v10);
        f32 d01 = v10.Dot(v20);
        f32 d11 = v20.Dot(v20);
        f32 d20 = p0.Dot(v10);
        f32 d21 = p0.Dot(v20);

        f32 denom = d00 * d11 - d01 * d01;
        if (std::abs(denom) > 1e-8f)
        {
            f32 v = (d11 * d20 - d01 * d21) / denom;
            f32 w = (d00 * d21 - d01 * d20) / denom;
            f32 u = 1.0f - v - w;

            if (u >= 0.0f && v >= 0.0f && w >= 0.0f)
            {
                Vector3 proj = data.Triangle.V0 * u + data.Triangle.V1 * v + data.Triangle.V2 * w;
                return (p - proj).Length();
            }
        }

        auto SeqmentDistSq = [](const Vector3& pt, const Vector3& a, const Vector3& b)
        {
            Vector3 ab = b - a;
            f32 t      = std::clamp((pt - a).Dot(ab) / ab.LengthSquared(), 0.0f, 1.0f);

            return (pt - (a + ab * t)).LengthSquared();
        };

        f32 d1 = SeqmentDistSq(p, data.Triangle.V0, data.Triangle.V1);
        f32 d2 = SeqmentDistSq(p, data.Triangle.V1, data.Triangle.V2);
        f32 d3 = SeqmentDistSq(p, data.Triangle.V2, data.Triangle.V0);

        return std::sqrt(std::min(std::min(d1, d2), d3));
    }

    Vector3 TriangleNormal(const PrimitiveData& data, const Vector3& localPoint)
    {
        return data.Triangle.Normal;
    }

    bool TriangleIntersect(const PrimitiveData& data, const Ray& localRay, f32 minT, f32 maxT, f32& outT, Vector3& outNormal, Vector2& outUV, bool& outHasUV)
    {
        const Vector3& v0 = data.Triangle.V0;
        const Vector3& v1 = data.Triangle.V1;
        const Vector3& v2 = data.Triangle.V2;

        Vector3 edge1 = v1 - v0;
        Vector3 edge2 = v2 - v0;
        Vector3 pvec  = localRay.Direction.Cross(edge2);
        f32 det       = edge1.Dot(pvec);

        if (std::abs(det) < 1e-8f)
            return false;

        f32 invDet   = 1.0f / det;
        Vector3 tvec = localRay.Origin - v0;
        f32 u        = tvec.Dot(pvec) * invDet;

        if (u < 0.0f || u > 1.0f)
            return false;

        Vector3 qvec = tvec.Cross(edge1);
        f32 v        = localRay.Direction.Dot(qvec) * invDet;

        if (v < 0.0f || u + v > 1.0f)
            return false;

        f32 t = edge2.Dot(qvec) * invDet;
        if (t < minT || t > maxT)
            return false;

        outT      = t;
        outNormal = (det > 0.0f) ? data.Triangle.Normal : -data.Triangle.Normal;

        if (data.Triangle.HasUV)
        {
            f32 w    = 1.0f - u - v;
            outUV    = data.Triangle.UV0 * w + data.Triangle.UV1 * u + data.Triangle.UV2 * v;
            outHasUV = true;
        }
        else
        {
            outUV    = Vector2(u, v);
            outHasUV = false;
        }

        return true;
    }
}
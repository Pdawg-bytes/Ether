#pragma once

#include "../Math/Vector3.h"
#include "../Math/Vector2.h"
#include "../Math/AABB.h"
#include "../Math/Ray.h"

enum class PrimitiveType
{
    Sphere,
    Box,
    Plane,
    Torus,
    Cylinder,
    Triangle,
    CSG
};

namespace SDF
{
    struct SphereData   { f32 Radius; };
    struct BoxData      { Vector3 Extents; };
    struct PlaneData    { Vector3 Normal; f32 Distance; };
    struct TorusData    { f32 MajorRadius; f32 MinorRadius; };
    struct CylinderData { f32 Radius; f32 HalfHeight; };
    struct TriangleData
    {
        Vector3 V0;
        Vector3 V1;
        Vector3 V2;
        Vector3 Normal;
        Vector2 UV0;
        Vector2 UV1;
        Vector2 UV2;
        bool HasUV;
    };
}

union PrimitiveData
{
    SDF::SphereData   Sphere;
    SDF::BoxData      Box;
    SDF::PlaneData    Plane;
    SDF::TorusData    Torus;
    SDF::CylinderData Cylinder;
    SDF::TriangleData Triangle;
};

using SDFDistanceFunc  = f32(*)(const PrimitiveData& data, const Vector3& localPoint);
using SDFNormalFunc    = Vector3(*)(const PrimitiveData& data, const Vector3& localPoint);
using RayIntersectFunc = bool(*)(const PrimitiveData& data, const Ray& localRay, f32 minT, f32 maxT, f32& outT, Vector3& outNormal, Vector2& outUV, bool& outHasUV);

struct PrimitiveDefinition
{
    PrimitiveType    Type          = PrimitiveType::Sphere;
    SDFDistanceFunc  DistanceFunc  = nullptr;
    SDFNormalFunc    NormalFunc    = nullptr;
    RayIntersectFunc IntersectFunc = nullptr;
    PrimitiveData    Data {};
    AABB             LocalBounds;
};

namespace SDF
{
    PrimitiveDefinition MakeSphere(f32 radius);
    PrimitiveDefinition MakeBox(const Vector3& extents);
    PrimitiveDefinition MakePlane(const Vector3& normal, f32 distance);
    PrimitiveDefinition MakeTorus(f32 majorRadius, f32 minorRadius);
    PrimitiveDefinition MakeCylinder(f32 radius, f32 halfHeight);
    PrimitiveDefinition MakeTriangle(const Vector3& v0, const Vector3& v1, const Vector3& v2,
                                     const Vector2& uv0 = Vector2(0.0f, 0.0f),
                                     const Vector2& uv1 = Vector2(1.0f, 0.0f),
                                     const Vector2& uv2 = Vector2(0.0f, 1.0f),
                                     bool hasUV = false);

    f32 SphereDistance(const PrimitiveData& data, const Vector3& localPoint);
    Vector3 SphereNormal(const PrimitiveData& data, const Vector3& localPoint);
    bool SphereIntersect(const PrimitiveData& data, const Ray& localRay, f32 minT, f32 maxT, f32& outT, Vector3& outNormal, Vector2& outUV, bool& outHasUV);

    f32 BoxDistance(const PrimitiveData& data, const Vector3& localPoint);
    Vector3 BoxNormal(const PrimitiveData& data, const Vector3& localPoint);
    bool BoxIntersect(const PrimitiveData& data, const Ray& localRay, f32 minT, f32 maxT, f32& outT, Vector3& outNormal, Vector2& outUV, bool& outHasUV);

    f32 PlaneDistance(const PrimitiveData& data, const Vector3& localPoint);
    Vector3 PlaneNormal(const PrimitiveData& data, const Vector3& localPoint);
    bool PlaneIntersect(const PrimitiveData& data, const Ray& localRay, f32 minT, f32 maxT, f32& outT, Vector3& outNormal, Vector2& outUV, bool& outHasUV);

    f32 TorusDistance(const PrimitiveData& data, const Vector3& localPoint);
    Vector3 TorusNormal(const PrimitiveData& data, const Vector3& localPoint);
    bool TorusIntersect(const PrimitiveData& data, const Ray& localRay, f32 minT, f32 maxT, f32& outT, Vector3& outNormal, Vector2& outUV, bool& outHasUV);

    f32 CylinderDistance(const PrimitiveData& data, const Vector3& localPoint);
    Vector3 CylinderNormal(const PrimitiveData& data, const Vector3& localPoint);
    bool CylinderIntersect(const PrimitiveData& data, const Ray& localRay, f32 minT, f32 maxT, f32& outT, Vector3& outNormal, Vector2& outUV, bool& outHasUV);

    f32 TriangleDistance(const PrimitiveData& data, const Vector3& localPoint);
    Vector3 TriangleNormal(const PrimitiveData& data, const Vector3& localPoint);
    bool TriangleIntersect(const PrimitiveData& data, const Ray& localRay, f32 minT, f32 maxT, f32& outT, Vector3& outNormal, Vector2& outUV, bool& outHasUV);
}
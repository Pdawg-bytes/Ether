#pragma once

#include "../Math/Vector3.h"
#include "../Math/AABB.h"

namespace SDF
{
    struct SphereData   { f32 Radius; };
    struct BoxData      { Vector3 Extents; };
    struct PlaneData    { Vector3 Normal; f32 Distance; };
    struct TorusData    { f32 MajorRadius; f32 MinorRadius; };
    struct CylinderData { f32 Radius; f32 HalfHeight; };
}

union PrimitiveData
{
    SDF::SphereData   Sphere;
    SDF::BoxData      Box;
    SDF::PlaneData    Plane;
    SDF::TorusData    Torus;
    SDF::CylinderData Cylinder;
};

using SDFDistanceFunc = f32(*)(const PrimitiveData& data, const Vector3& localPoint);
using SDFNormalFunc   = Vector3(*)(const PrimitiveData& data, const Vector3& localPoint);

struct PrimitiveDefinition
{
    SDFDistanceFunc DistanceFunc = nullptr;
    SDFNormalFunc NormalFunc = nullptr;
    PrimitiveData Data{};
    AABB LocalBounds;
};

namespace SDF
{
    PrimitiveDefinition MakeSphere(f32 radius);
    PrimitiveDefinition MakeBox(const Vector3& extents);
    PrimitiveDefinition MakeTorus(f32 majorRadius, f32 minorRadius);
    PrimitiveDefinition MakeCylinder(f32 radius, f32 halfHeight);

    f32 SphereDistance(const PrimitiveData& data, const Vector3& localPoint);
    Vector3 SphereNormal(const PrimitiveData& data, const Vector3& localPoint);

    f32 BoxDistance(const PrimitiveData& data, const Vector3& localPoint);
    Vector3 BoxNormal(const PrimitiveData& data, const Vector3& localPoint);

    f32 PlaneDistance(const PrimitiveData& data, const Vector3& localPoint);
    Vector3 PlaneNormal(const PrimitiveData& data, const Vector3& localPoint);

    f32 TorusDistance(const PrimitiveData& data, const Vector3& localPoint);
    Vector3 TorusNormal(const PrimitiveData& data, const Vector3& localPoint);

    f32 CylinderDistance(const PrimitiveData& data, const Vector3& localPoint);
    Vector3 CylinderNormal(const PrimitiveData& data, const Vector3& localPoint);
}
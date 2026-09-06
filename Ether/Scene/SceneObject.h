#pragma once

#include "../Math/Vector3.h"
#include "../Math/Vector2.h"
#include "../Math/Quaternion.h"
#include "../Math/AABB.h"
#include "../Math/Ray.h"
#include "Primitives.h"

#include <memory>
#include <vector>

class CSGTree;

struct SceneObject
{
    Vector3 Position;
    Quaternion Rotation = Quaternion::Identity;
    Vector3 Scale	    = Vector3(1.0f);
    bool IsBounded      = true;

    AABB WorldBounds;

    f32 BoundingRadius = 0.0f;

    PrimitiveType Type             = PrimitiveType::Sphere;
    SDFDistanceFunc DistanceFunc   = nullptr;
    SDFNormalFunc NormalFunc       = nullptr;
    RayIntersectFunc IntersectFunc = nullptr;
    PrimitiveData Data {};
    std::shared_ptr<CSGTree> Template;

    s32 MaterialIndex = -1;

    bool Intersect(const Ray& ray, RayHit& hit, f32 minT = 0.0001f, f32 maxT = 1e30f) const;
    f32 Distance(const Vector3& worldPoint) const;
    Vector3 Normal(const Vector3& worldPoint) const;
    s32 ResolveMaterialIndex(const Vector3& worldPoint) const;

    static SceneObject CreatePrimitive(const PrimitiveDefinition& definition, const Vector3& position,
                                       const Quaternion& rotation = Quaternion::Identity, const Vector3& scale = Vector3(1.0f), s32 materialIndex = -1);

    static SceneObject CreateSphere(const Vector3& position, f32 radius, const Vector3& scale = Vector3(1.0f), s32 materialIndex = -1);
    static SceneObject CreateBox(const Vector3& position, const Quaternion& rotation, const Vector3& extents, const Vector3& scale = Vector3(1.0f), s32 materialIndex = -1);
    static SceneObject CreateTorus(const Vector3& position, const Quaternion& rotation, f32 majorRadius, f32 minorRadius, f32 scale = 1.0f, s32 materialIndex = -1);
    static SceneObject CreateCylinder(const Vector3& position, const Quaternion& rotation, f32 radius, f32 halfHeight, f32 scale = 1.0f, s32 materialIndex = -1);
    static SceneObject CreatePlane(const Vector3& normal, f32 distance, s32 materialIndex = -1);

    static SceneObject CreateTriangle(const Vector3& v0, const Vector3& v1, const Vector3& v2, s32 materialIndex = -1);
    static SceneObject CreateTriangle(const Vector3& v0, const Vector3& v1, const Vector3& v2,
                                      const Vector2& uv0, const Vector2& uv1, const Vector2& uv2, s32 materialIndex = -1);

    static std::vector<SceneObject> CreateQuad(const Vector3& v0, const Vector3& v1, const Vector3& v2, const Vector3& v3, s32 materialIndex = -1);
    static std::vector<SceneObject> CreateQuad(const Vector3& v0, const Vector3& v1, const Vector3& v2, const Vector3& v3,
                                              const Vector2& uv0, const Vector2& uv1, const Vector2& uv2, const Vector2& uv3, s32 materialIndex = -1);

    static SceneObject CreateFromTemplate(std::shared_ptr<CSGTree> tmpl, const Vector3& position, const Quaternion& rotation = Quaternion::Identity, const Vector3& scale = Vector3(1.0f), s32 materialIndex = -1);

private:
    Vector3 ToLocalPoint(const Vector3& worldPoint) const;
    Ray ToLocalRay(const Ray& worldRay) const;
    Vector3 ToWorldNormal(const Vector3& localNormal) const;
    void UpdateWorldBounds(const AABB& localBounds);
};
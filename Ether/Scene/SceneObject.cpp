#include "SceneObject.h"
#include "Primitives.h"
#include "CSGTree.h"
#include "../Math/MathUtil.h"

#include <algorithm>
#include <cmath>

namespace
{
    constexpr f32 SDFHitEpsilon = 0.0001f;
}

bool SceneObject::Intersect(const Ray& ray, RayHit& hit, f32 minT, f32 maxT) const
{
    f32 effectiveMaxT = std::min(maxT, hit.Distance);
    if (minT > effectiveMaxT)
        return false;

    f32 boxT0 = minT, boxT1 = effectiveMaxT;
    if (IsBounded)
    {
        if (!WorldBounds.Intersect(ray, minT, effectiveMaxT, boxT0, boxT1))
            return false;

        if (boxT0 >= hit.Distance)
            return false;
    }

    Ray localRay = ToLocalRay(ray);

    if (IntersectFunc)
    {
        f32 localT;
        Vector3 localNormal;
        Vector2 uv;

        bool hasUV = false;

        if (!IntersectFunc(Data, localRay, minT, effectiveMaxT, localT, localNormal, uv, hasUV))
            return false;

        if (localT < minT || localT >= hit.Distance)
            return false;

        hit.Distance = localT;
        hit.Point    = ray.At(localT);

        hit.Normal = ToWorldNormal(localNormal);

        hit.UV            = uv;
        hit.HasUV         = hasUV;
        hit.MaterialIndex = ResolveMaterialIndex(hit.Point);

        return true;
    }

    if (Template || DistanceFunc)
    {
        f32 tStart = std::max(boxT0, minT);
        f32 tEnd   = std::min(boxT1, hit.Distance);

        if (Template)
        {
            f32 localT0 = minT, localT1 = effectiveMaxT;
            if (!Template->LocalBounds().Intersect(localRay, minT, effectiveMaxT, localT0, localT1))
                return false;

            tStart = std::max(localT0, minT);
            tEnd   = std::min(localT1, hit.Distance);
        }

        if (tStart > tEnd)
            return false;

        f32 minLocalScale = std::min(Scale.X, std::min(Scale.Y, Scale.Z));
        f32 t             = tStart;

        s32 maxSteps = Template ? 128 : 64;

        for (s32 step = 0; step < maxSteps && t <= tEnd; ++step)
        {
            Vector3 localPoint = localRay.At(t);
            f32 rawDist        = Template ? Template->Distance(localPoint) : DistanceFunc(Data, localPoint);
            f32 worldDist      = rawDist * minLocalScale;

            if (std::abs(worldDist) < SDFHitEpsilon)
            {
                if (t >= minT && t < hit.Distance)
                {
                    hit.Distance      = t;
                    hit.Point         = ray.At(t);
                    hit.Normal        = Normal(hit.Point);
                    hit.MaterialIndex = ResolveMaterialIndex(hit.Point);
                    hit.HasUV         = false;

                    return true;
                }

                return false;
            }

            t += std::max(std::abs(worldDist), SDFHitEpsilon);
        }
    }

    return false;
}

f32 SceneObject::Distance(const Vector3& worldPoint) const
{
    Vector3 localPoint = ToLocalPoint(worldPoint);
    f32 rawDistance    = Template ? Template->Distance(localPoint) : DistanceFunc(Data, localPoint);
    f32 minScale       = std::min(Scale.X, std::min(Scale.Y, Scale.Z));

    return rawDistance * minScale;
}

Vector3 SceneObject::Normal(const Vector3& worldPoint) const
{
    Vector3 localPoint  = ToLocalPoint(worldPoint);
    Vector3 localNormal = Template ? Template->Normal(localPoint) : NormalFunc(Data, localPoint);

    return ToWorldNormal(localNormal);
}

s32 SceneObject::ResolveMaterialIndex(const Vector3& worldPoint) const
{
    if (!Template)
        return MaterialIndex;

    Vector3 localPoint = ToLocalPoint(worldPoint);
    s32 leafMaterial   = Template->MaterialIndex(localPoint);

    return leafMaterial >= 0 ? leafMaterial : MaterialIndex;
}


Vector3 SceneObject::ToLocalPoint(const Vector3& worldPoint) const
{
    return Rotation.Conjugate().Rotate(worldPoint - Position) / Scale;
}

Ray SceneObject::ToLocalRay(const Ray& worldRay) const
{
    Quaternion inverseRotation = Rotation.Conjugate();
    return Ray(inverseRotation.Rotate(worldRay.Origin - Position) / Scale,
               inverseRotation.Rotate(worldRay.Direction) / Scale);
}

Vector3 SceneObject::ToWorldNormal(const Vector3& localNormal) const
{
    Vector3 correctedNormal(localNormal.X / Scale.X, localNormal.Y / Scale.Y, localNormal.Z / Scale.Z);
    return Rotation.Rotate(correctedNormal).Normalized();
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


SceneObject SceneObject::CreatePrimitive(const PrimitiveDefinition& definition, const Vector3& position, const Quaternion& rotation, const Vector3& scale, s32 materialIndex)
{
    SceneObject object;
    object.Position      = position;
    object.Rotation      = rotation;
    object.Scale         = scale;
    object.Type          = definition.Type;
    object.DistanceFunc  = definition.DistanceFunc;
    object.NormalFunc    = definition.NormalFunc;
    object.IntersectFunc = definition.IntersectFunc;
    object.Data          = definition.Data;
    object.MaterialIndex = materialIndex;

    object.UpdateWorldBounds(definition.LocalBounds);

    return object;
}

SceneObject SceneObject::CreateSphere(const Vector3& position, f32 radius, const Vector3& scale, s32 materialIndex)
{
    return CreatePrimitive(Primitives::MakeSphere(radius), position, Quaternion::Identity, scale, materialIndex);
}

SceneObject SceneObject::CreateBox(const Vector3& position, const Quaternion& rotation, const Vector3& extents, const Vector3& scale, s32 materialIndex)
{
    return CreatePrimitive(Primitives::MakeBox(extents), position, rotation, scale, materialIndex);
}

SceneObject SceneObject::CreateTorus(const Vector3& position, const Quaternion& rotation, f32 majorRadius, f32 minorRadius, f32 scale, s32 materialIndex)
{
    return CreatePrimitive(Primitives::MakeTorus(majorRadius, minorRadius), position, rotation, Vector3(scale), materialIndex);
}

SceneObject SceneObject::CreateCylinder(const Vector3& position, const Quaternion& rotation, f32 radius, f32 halfHeight, f32 scale, s32 materialIndex)
{
    return CreatePrimitive(Primitives::MakeCylinder(radius, halfHeight), position, rotation, Vector3(scale), materialIndex);
}

SceneObject SceneObject::CreatePlane(const Vector3& normal, f32 distance, s32 materialIndex)
{
    PrimitiveDefinition def = Primitives::MakePlane(normal, distance);

    SceneObject object;
    object.Position	     = Vector3::Zero;
    object.Type          = def.Type;
    object.DistanceFunc	 = def.DistanceFunc;
    object.NormalFunc	 = def.NormalFunc;
    object.IntersectFunc = def.IntersectFunc;
    object.Data	         = def.Data;
    object.IsBounded	 = false;
    object.MaterialIndex = materialIndex;

    return object;
}

SceneObject SceneObject::CreateTriangle(const Vector3& v0, const Vector3& v1, const Vector3& v2, s32 materialIndex)
{
    return CreatePrimitive(Primitives::MakeTriangle(v0, v1, v2), Vector3::Zero, Quaternion::Identity, Vector3::One, materialIndex);
}

SceneObject SceneObject::CreateTriangle(const Vector3& v0, const Vector3& v1, const Vector3& v2,
                                        const Vector2& uv0, const Vector2& uv1, const Vector2& uv2, s32 materialIndex)
{
    return CreatePrimitive(Primitives::MakeTriangle(v0, v1, v2, uv0, uv1, uv2, true),
                           Vector3::Zero, Quaternion::Identity, Vector3::One, materialIndex);
}

std::vector<SceneObject> SceneObject::CreateQuad(const Vector3& v0, const Vector3& v1, const Vector3& v2, const Vector3& v3, s32 materialIndex)
{
    return
    {
        CreateTriangle(v0, v1, v2, Vector2(0.0f, 0.0f), Vector2(1.0f, 0.0f), Vector2(1.0f, 1.0f), materialIndex),
        CreateTriangle(v0, v2, v3, Vector2(0.0f, 0.0f), Vector2(1.0f, 1.0f), Vector2(0.0f, 1.0f), materialIndex)
    };
}

std::vector<SceneObject> SceneObject::CreateQuad(const Vector3& v0, const Vector3& v1, const Vector3& v2, const Vector3& v3,
                                                const Vector2& uv0, const Vector2& uv1, const Vector2& uv2, const Vector2& uv3, s32 materialIndex)
{
    return
    {
        CreateTriangle(v0, v1, v2, uv0, uv1, uv2, materialIndex),
        CreateTriangle(v0, v2, v3, uv0, uv2, uv3, materialIndex)
    };
}

SceneObject SceneObject::CreateFromTemplate(std::shared_ptr<CSGTree> tmpl, const Vector3& position, const Quaternion& rotation, const Vector3& scale, s32 materialIndex)
{
    SceneObject object;
    object.Position      = position;
    object.Rotation      = rotation;
    object.Scale	     = scale;
    object.Type          = PrimitiveType::CSG;
    object.Template      = std::move(tmpl);
    object.MaterialIndex = materialIndex;

    object.UpdateWorldBounds(object.Template->LocalBounds());
    return object;
}
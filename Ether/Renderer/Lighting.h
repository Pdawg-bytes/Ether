#pragma once

#include "../Math/Vector3.h"

#include <vector>

#define AMBIENT_OCCLUSION

enum class LightType
{
    Point,
    Directional
};

struct Light
{
    LightType Type;
    Vector3 Color = Vector3(1.0f);
    f32 Intensity = 1.0f;
    f32 Radius    = 0.0f;
    
    union
    {
        Vector3 Position;
        Vector3 Direction;
    };

    Light() : Type(LightType::Point), Position(Vector3::Zero) {}
    
    Light(LightType type, const Vector3& color, f32 intensity, f32 radius, const Vector3& posOrDir)
        : Type(type), Color(color), Intensity(intensity), Radius(radius), Position(posOrDir) {}
};

inline Light MakePointLight(const Vector3& position, const Vector3& color, f32 intensity, f32 radius = 0.0f)
{
    return Light(LightType::Point, color, intensity, radius, position);
}

inline Light MakeDirectionalLight(const Vector3& direction, const Vector3& color, f32 intensity, f32 radius = 0.0f)
{
    return Light(LightType::Directional, color, intensity, radius, direction);
}

class BVH;

struct SurfacePoint
{
    Vector3 Position;
    Vector3 Normal;
    Vector3 Albedo;
    f32		Roughness;
    f32		Metallic;
    Vector3 Emission;
};

class Lighting
{
public:
    void AddLight(const Light& light);

    Vector3 Shade(const BVH& bvh, const SurfacePoint& surface, const Vector3& viewDir) const;

private:
    f32 CalculateAO(const BVH& bvh, const Vector3& position, const Vector3& normal) const;

    std::vector<Light> _lights;
};
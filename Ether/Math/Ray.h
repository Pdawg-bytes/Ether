#pragma once

#include "../Math/Vector3.h"
#include "../Math/Vector2.h"
#include <limits>

struct Ray
{
    Vector3 Origin;
    Vector3 Direction;

    Ray() = default;
    Ray(const Vector3& origin, const Vector3& direction) : Origin(origin), Direction(direction) {}

    Vector3 At(f32 distance) const { return Origin + Direction * distance; }
};

struct RayHit
{
    f32 Distance = std::numeric_limits<f32>::max();
    Vector3 Point;
    Vector3 Normal;

    Vector2 UV;
    bool HasUV        = false;
    s32 MaterialIndex = -1;
    s32 ObjectIndex   = -1;
};
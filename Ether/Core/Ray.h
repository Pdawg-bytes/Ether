#pragma once

#include "../Math/Vector3.h"

struct Ray
{
    Vector3 Origin;
    Vector3 Direction;

    Ray() = default;
    Ray(const Vector3& origin, const Vector3& direction) : Origin(origin), Direction(direction) {}

    Vector3 At(f32 distance) const { return Origin + Direction * distance; }
};
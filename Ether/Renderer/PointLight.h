#pragma once

#include "../Math/Vector3.h"

struct PointLight
{
    Vector3 Position;
    Vector3 Color	  = Vector3(1.0f);
    f32 Intensity     = 1.0f;
    f32 Radius        = 0.0f;
};
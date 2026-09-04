#pragma once

#include "Vector3.h"
#include "Quaternion.h"
#include "AABB.h"

namespace Math
{
    constexpr f32 PI = 3.14159265358979323846f;

    inline f32 DegToRad(f32 degrees) { return degrees * (PI / 180.0f); }
    inline f32 RadToDeg(f32 radians) { return radians * (180.0f / PI); }
    inline f32 Lerp(f32 a, f32 b, f32 t) { return a + (b - a) * t; }

    inline AABB TransformBounds(const AABB& bounds, const Vector3& position, const Quaternion& rotation, const Vector3& scale)
    {
        Vector3 corners[8] =
        {
            Vector3(bounds.Min.X, bounds.Min.Y, bounds.Min.Z),
            Vector3(bounds.Max.X, bounds.Min.Y, bounds.Min.Z),
            Vector3(bounds.Min.X, bounds.Max.Y, bounds.Min.Z),
            Vector3(bounds.Max.X, bounds.Max.Y, bounds.Min.Z),
            Vector3(bounds.Min.X, bounds.Min.Y, bounds.Max.Z),
            Vector3(bounds.Max.X, bounds.Min.Y, bounds.Max.Z),
            Vector3(bounds.Min.X, bounds.Max.Y, bounds.Max.Z),
            Vector3(bounds.Max.X, bounds.Max.Y, bounds.Max.Z),
        };

        AABB result;
        for (const Vector3& corner : corners)
        {
            Vector3 worldCorner = position + rotation.Rotate(corner * scale);
            result.Min = Vector3::Min(result.Min, worldCorner);
            result.Max = Vector3::Max(result.Max, worldCorner);
        }

        return result;
    }

    inline AABB TransformBounds(const AABB& bounds, const Vector3& position, const Quaternion& rotation, f32 scale)
    {
        return TransformBounds(bounds, position, rotation, Vector3(scale));
    }
}
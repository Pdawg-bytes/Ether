#pragma once

#include "Vector3.h"
#include <algorithm>
#include <limits>

struct AABB
{
    Vector3 Min;
    Vector3 Max;

    AABB() : Min(Vector3(std::numeric_limits<f32>::max())), Max(Vector3(-std::numeric_limits<f32>::max())) {}
    AABB(const Vector3& min, const Vector3& max) : Min(min), Max(max) {}

    Vector3 Center()  const { return (Min + Max) * 0.5f; }
    Vector3 Extents() const { return (Max - Min) * 0.5f; }

    AABB Union(const AABB& other) const
    {
        return AABB(Vector3::Min(Min, other.Min), Vector3::Max(Max, other.Max));
    }

    f32 DistanceSquared(const Vector3& point) const
    {
        f32 dx = 0.0f;
        if (point.X < Min.X)      dx = Min.X - point.X;
        else if (point.X > Max.X) dx = point.X - Max.X;

        f32 dy = 0.0f;
        if (point.Y < Min.Y)      dy = Min.Y - point.Y;
        else if (point.Y > Max.Y) dy = point.Y - Max.Y;

        f32 dz = 0.0f;
        if (point.Z < Min.Z)      dz = Min.Z - point.Z;
        else if (point.Z > Max.Z) dz = point.Z - Max.Z;

        return dx * dx + dy * dy + dz * dz;
    }

    f32 SurfaceArea() const
    {
        Vector3 d = Max - Min;
        return 2.0f * (d.X * d.Y + d.Y * d.Z + d.Z * d.X);
    }
};
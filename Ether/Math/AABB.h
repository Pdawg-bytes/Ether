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
		f32 dx = std::max({ Min.X - point.X, 0.0f, point.X - Max.X });
		f32 dy = std::max({ Min.Y - point.Y, 0.0f, point.Y - Max.Y });
		f32 dz = std::max({ Min.Z - point.Z, 0.0f, point.Z - Max.Z });

		return dx * dx + dy * dy + dz * dz;
	}

	f32 SurfaceArea() const
	{
		Vector3 d = Max - Min;

		if (d.X < 0.0f || d.Y < 0.0f || d.Z < 0.0f)
			return 0.0f;

		return 2.0f * (d.X * d.Y + d.Y * d.Z + d.Z * d.X);
	}
};
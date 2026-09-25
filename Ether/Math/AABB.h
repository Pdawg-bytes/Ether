#pragma once

#include "Vector3.h"
#include "../Math/Ray.h"
#include <algorithm>
#include <cmath>
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
		f32 dx = std::max(0.0f, std::max(Min.X - point.X, point.X - Max.X));
		f32 dy = std::max(0.0f, std::max(Min.Y - point.Y, point.Y - Max.Y));
		f32 dz = std::max(0.0f, std::max(Min.Z - point.Z, point.Z - Max.Z));
		return dx * dx + dy * dy + dz * dz;
	}

	f32 SurfaceArea() const
	{
		Vector3 d = Max - Min;
		return 2.0f * (d.X * d.Y + d.Y * d.Z + d.Z * d.X);
	}

	bool Intersect(const Vector3& rayOrigin, const Vector3& invDir, f32 tMin, f32 tMax, f32& outT0, f32& outT1) const
	{
		f32 tx1 = (Min.X - rayOrigin.X) * invDir.X;
		f32 tx2 = (Max.X - rayOrigin.X) * invDir.X;
		f32 t0  = std::max(tMin, std::min(tx1, tx2));
		f32 t1  = std::min(tMax, std::max(tx1, tx2));

		f32 ty1 = (Min.Y - rayOrigin.Y) * invDir.Y;
		f32 ty2 = (Max.Y - rayOrigin.Y) * invDir.Y;
		t0	    = std::max(t0, std::min(ty1, ty2));
		t1	    = std::min(t1, std::max(ty1, ty2));

		f32 tz1 = (Min.Z - rayOrigin.Z) * invDir.Z;
		f32 tz2 = (Max.Z - rayOrigin.Z) * invDir.Z;
		t0		= std::max(t0, std::min(tz1, tz2));
		t1		= std::min(t1, std::max(tz1, tz2));

		outT0 = t0;
		outT1 = t1;

		return t0 <= t1;
	}

	bool Intersect(const Ray& ray, f32 tMin, f32 tMax, f32& outT0, f32& outT1) const
	{
		Vector3 invDir(1.0f / ray.Direction.X, 1.0f / ray.Direction.Y, 1.0f / ray.Direction.Z);
		return Intersect(ray.Origin, invDir, tMin, tMax, outT0, outT1);
	}
};
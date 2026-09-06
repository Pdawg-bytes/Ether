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

	bool Intersect(const Ray& ray, f32 tMin, f32 tMax, f32& outT0, f32& outT1) const
	{
		f32 t0 = tMin;
		f32 t1 = tMax;

		if (std::abs(ray.Direction.X) > 1e-8f)
		{
			f32 invD  = 1.0f / ray.Direction.X;
			f32 tNear = (Min.X - ray.Origin.X) * invD;
			f32 tFar  = (Max.X - ray.Origin.X) * invD;

			if (tNear > tFar) 
				std::swap(tNear, tFar);

			t0 = std::max(t0, tNear);
			t1 = std::min(t1, tFar);

			if (t0 > t1) return false;
		}
		else if (ray.Origin.X < Min.X || ray.Origin.X > Max.X)
		{
			return false;
		}

		if (std::abs(ray.Direction.Y) > 1e-8f)
		{
			f32 invD  = 1.0f / ray.Direction.Y;
			f32 tNear = (Min.Y - ray.Origin.Y) * invD;
			f32 tFar  = (Max.Y - ray.Origin.Y) * invD;

			if (tNear > tFar) 
				std::swap(tNear, tFar);

			t0 = std::max(t0, tNear);
			t1 = std::min(t1, tFar);

			if (t0 > t1) return false;
		}
		else if (ray.Origin.Y < Min.Y || ray.Origin.Y > Max.Y)
		{
			return false;
		}

		if (std::abs(ray.Direction.Z) > 1e-8f)
		{
			f32 invD  = 1.0f / ray.Direction.Z;
			f32 tNear = (Min.Z - ray.Origin.Z) * invD;
			f32 tFar  = (Max.Z - ray.Origin.Z) * invD;

			if (tNear > tFar) 
				std::swap(tNear, tFar);

			t0 = std::max(t0, tNear);
			t1 = std::min(t1, tFar);
			
			if (t0 > t1) return false;
		}
		else if (ray.Origin.Z < Min.Z || ray.Origin.Z > Max.Z)
		{
			return false;
		}

		outT0 = t0;
		outT1 = t1;
		return true;
	}

	bool Intersect(const Ray& ray, f32 tMin, f32 tMax) const
	{
		f32 t0, t1;
		return Intersect(ray, tMin, tMax, t0, t1);
	}
};
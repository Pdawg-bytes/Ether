#include "Raymarcher.h"
#include "../Platform/ThreadPool.h"

#include <algorithm>
#include <atomic>
#include <cstdint>

namespace
{
	constexpr s32 MaxSteps         = 128;
	constexpr f32 MinHitDistance   = 0.001f;
	constexpr f32 MaxTraceDistance = 100.0f;

	const Vector3 LightDirection = Vector3(-0.5f, 1.0f, -0.3f).Normalized();

	u32 PackColor(const Vector3& color)
	{
		f32 correctedR = std::pow(std::clamp(color.X, 0.0f, 1.0f), 1.0f / 2.2f);
		f32 correctedG = std::pow(std::clamp(color.Y, 0.0f, 1.0f), 1.0f / 2.2f);
		f32 correctedB = std::pow(std::clamp(color.Z, 0.0f, 1.0f), 1.0f / 2.2f);

		u32 r = (u32)(correctedR * 255.0f);
		u32 g = (u32)(correctedG * 255.0f);
		u32 b = (u32)(correctedB * 255.0f);

		return (255u << 24) | (b << 16) | (g << 8) | r;
	}

	Vector3 Shade(const BVH& bvh, s32 objectIndex, const Vector3& hitPoint)
	{
		Vector3 normal = bvh.GetObject(objectIndex).Normal(hitPoint);
		f32 diffuse    = std::max(normal.Dot(LightDirection), 0.0f);
		f32 intensity  = std::min(0.15f + diffuse, 1.0f);

		return Vector3(intensity);
	}
}

Raymarcher::Raymarcher(Camera& camera, const BVH& bvh, u32 width, u32 height)
	: _camera(camera), _bvh(bvh), _width(width), _height(height)
{
}


MarchResult Raymarcher::Raymarch(const Ray& ray) const
{
	f32 traveled = 0.0f;

	for (s32 step = 0; step < MaxSteps; step++)
	{
		s32 object;
		Vector3 point = ray.At(traveled);
		f32 distance  = _bvh.Distance(point, object);

		if (distance < MinHitDistance)
			return { true, traveled, object, point };

		traveled += distance;
		if (traveled > MaxTraceDistance)
			break;
	}

	return { false, traveled, -1, Vector3::Zero };
}

Vector3 Raymarcher::Trace(const Ray& ray) const
{
	MarchResult result = Raymarcher::Raymarch(ray);

	if (result.Hit)
		return Shade(_bvh, result.ObjectIndex, result.Point);
	else
		return BackgroundColor;
}

void Raymarcher::Render(u32* framebuffer) const
{
#ifdef MULTI_THREADED_RENDERING
	std::atomic<s32> nextRow{ 0 };

	GetSharedThreadPool().Dispatch([this, framebuffer, &nextRow]()
	{
		s32 y = 0;
		while ((y = nextRow.fetch_add(1)) < _height)
		{
			for (s32 x = 0; x < _width; x++)
			{
				Ray ray       = _camera.GetRay(x, y);
				Vector3 color = Trace(ray);

				framebuffer[y * _width + x] = PackColor(color);
			}
		}
	});
#else
	for (s32 y = 0; y < _height; y++)
	{
		for (s32 x = 0; x < _width; x++)
		{
			Ray ray	      = _camera.GetRay(x, y);
			Vector3 color = Trace(ray);

			framebuffer[y * _width + x] = PackColor(color);
		}
	}
#endif
}
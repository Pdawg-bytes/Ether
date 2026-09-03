#include "Raymarcher.h"
#include "../Platform/ThreadPool.h"

#include <algorithm>
#include <array>
#include <atomic>
#include <cstdint>

namespace
{
	constexpr s32 MaxSteps         = 512;
	constexpr f32 MinHitDistance   = 0.0001f;
	constexpr f32 MaxTraceDistance = 100.0f;
	constexpr usize GammaTableSize = 4096;

	const Vector3 LightDirection = Vector3(-0.5f, 1.0f, -0.3f).Normalized();

	u32 PackColor(const Vector3& color)
	{
		static const std::array<u8, GammaTableSize> gammaTable = []
		{
			std::array<u8, GammaTableSize> table{};

			for (usize i = 0; i < GammaTableSize; i++)
				table[i] = (u8)(std::pow((f32)i / (GammaTableSize - 1), 1.0f / 2.2f) * 255.0f);

			return table;
		}();

		auto ToGamma = [](f32 channel) -> u32
		{
			f32 clamped = std::clamp(channel, 0.0f, 1.0f);
			usize index = (usize)(clamped * (GammaTableSize - 1));
			return gammaTable[index];
		};

		u32 r = ToGamma(color.X);
		u32 g = ToGamma(color.Y);
		u32 b = ToGamma(color.Z);

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
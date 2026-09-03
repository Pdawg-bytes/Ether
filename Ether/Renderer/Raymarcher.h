#pragma once

#include "Camera.h"
#include "../Scene/BVH.h"

#define MULTI_THREADED_RENDERING

struct MarchResult
{
	bool    Hit;
	f32     Traveled;
	s32     ObjectIndex;
	Vector3 Point;
};

class Raymarcher
{
public:

	Raymarcher(Camera& camera, const BVH& bvh, u32 width, u32 height);

	MarchResult Raymarch(const Ray& ray) const;
	Vector3 Trace(const Ray& ray) const;
	void Render(u32* framebuffer) const;

private:
	Camera& _camera;
	const BVH& _bvh;
	u32 _width;
	u32 _height;

	static constexpr Vector3 BackgroundColor{ 0.05f, 0.05f, 0.08f };
};
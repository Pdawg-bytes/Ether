#pragma once

#include "Camera.h"
#include "Lighting.h"
#include "../Scene/BVH.h"

//#define MULTI_THREADED_RENDERING

class Raytracer
{
public:
    Raytracer(Camera& camera, const BVH& bvh, const Lighting& lighting, u32 width, u32 height);

    Vector3 Trace(const Ray& ray, s32 depth = 0) const;
    void Render(u32* framebuffer) const;

private:
    Camera&         _camera;
    const BVH&      _bvh;
    const Lighting& _lighting;
    u32             _width;
    u32             _height;

    static constexpr Vector3 BackgroundColor { 0.05f, 0.05f, 0.08f };
};
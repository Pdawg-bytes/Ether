#include "Raytracer.h"
#ifdef MULTI_THREADED_RENDERING
#include "../Platform/ThreadPool.h"
#endif
#include "../Scene/Material.h"
#include "../Scene/MaterialLibrary.h"

#include <algorithm>
#include <array>
#include <atomic>
#include <cmath>
#include <cstdint>

namespace
{
    constexpr f32 MinHitDistance      = 0.0001f;
    constexpr f32 RefractRayBias      = 0.001f;
    constexpr f32 MaxTraceDistance    = 100.0f;
    constexpr s32 MaxBounces          = 2;
    constexpr f32 MinBounceThroughput = 0.02f;
    constexpr usize GammaTableSize    = 4096;

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
}

Raytracer::Raytracer(Camera& camera, const BVH& bvh, const Lighting& lighting, u32 width, u32 height)
    : _camera(camera), _bvh(bvh), _lighting(lighting), _width(width), _height(height)
{
}


Vector3 Raytracer::Trace(const Ray& ray, s32 depth) const
{
    RayHit hit;

    if (!_bvh.Intersect(ray, hit, MinHitDistance, MaxTraceDistance))
        return BackgroundColor;

    const SceneObject& object = _bvh.GetObject(hit.ObjectIndex);
    s32 materialIndex         = hit.MaterialIndex >= 0 ? hit.MaterialIndex : object.MaterialIndex;
    const Material& material  = GetMaterialLibrary().Get(materialIndex);
    MaterialSample surface    = material.Evaluate(hit.Point, hit.Normal, hit.UV, hit.HasUV);

    Vector3 viewDir = -ray.Direction;
    SurfacePoint surfacePoint{ hit.Point, surface.Normal, surface.Albedo, surface.Roughness, surface.Metallic, surface.Emission };
    Vector3 shaded = _lighting.Shade(_bvh, surfacePoint, viewDir);

    Vector3 f0          = Vector3(0.04f) * (1.0f - surface.Metallic) + surface.Albedo * surface.Metallic;
    f32 nDotV           = std::max(surface.Normal.Dot(viewDir), 0.0f);
    Vector3 fresnel     = f0 + (Vector3(1.0f) - f0) * std::pow(1.0f - nDotV, 5.0f);
    f32 smoothness      = 1.0f - std::clamp(surface.Roughness, 0.0f, 1.0f);
    Vector3 reflectance = fresnel * (smoothness * smoothness);

    Vector3 result = shaded * (Vector3(1.0f) - reflectance) * (1.0f - surface.Transmission);

    if (depth >= MaxBounces)
        return result;

    if (surface.Transmission > 0.0f)
    {
        f32 cosI       = std::clamp(surface.Normal.Dot(viewDir), -1.0f, 1.0f);
        bool entering  = cosI > 0.0f;
        f32 eta        = entering ? (1.0f / surface.IOR) : surface.IOR;
        Vector3 n      = entering ? surface.Normal : -surface.Normal;
        f32 cosThetaI  = entering ? cosI : -cosI;
        f32 sin2ThetaT = eta * eta * (1.0f - cosThetaI * cosThetaI);

        if (sin2ThetaT >= 1.0f)
        {
            Vector3 reflectDir = ray.Direction - n * (2.0f * ray.Direction.Dot(n));

            Ray reflectRay(hit.Point + n * RefractRayBias, reflectDir);
            result += Trace(reflectRay, depth + 1) * surface.Transmission;
        }
        else
        {
            f32 cosThetaT      = std::sqrt(1.0f - sin2ThetaT);
            Vector3 refractDir = (ray.Direction * eta + n * (eta * cosThetaI - cosThetaT)).Normalized();

            Ray refractRay(hit.Point - n * RefractRayBias, refractDir);
            result += Trace(refractRay, depth + 1) * surface.Transmission;
        }
    }

    f32 reflectMagnitude = std::max(reflectance.X, std::max(reflectance.Y, reflectance.Z));
    if (reflectMagnitude > MinBounceThroughput)
    {
        Vector3 reflectDir = ray.Direction - surface.Normal * (2.0f * ray.Direction.Dot(surface.Normal));
        f32 normalBias     = reflectDir.Dot(surface.Normal) >= 0.0f ? RefractRayBias : -RefractRayBias;

        Ray reflectRay(hit.Point + surface.Normal * normalBias, reflectDir);
        result += Trace(reflectRay, depth + 1) * reflectance;
    }

    return result;
}

void Raytracer::Render(u32* framebuffer) const
{
#ifdef MULTI_THREADED_RENDERING
    std::atomic<s32> nextRow{ 0 };

    GetSharedThreadPool().Dispatch([this, framebuffer, &nextRow]()
    {
        s32 y = 0;
        while ((y = nextRow.fetch_add(1)) < (s32)_height)
        {
            for (s32 x = 0; x < (s32)_width; x++)
            {
                Ray ray       = _camera.GetRay(x, y);
                Vector3 color = Trace(ray);

                framebuffer[y * _width + x] = PackColor(color);
            }
        }
    });
#else
    for (s32 y = 0; y < (s32)_height; y++)
    {
        for (s32 x = 0; x < (s32)_width; x++)
        {
            Ray ray	      = _camera.GetRay(x, y);
            Vector3 color = Trace(ray);

            framebuffer[y * _width + x] = PackColor(color);
        }
    }
#endif
}
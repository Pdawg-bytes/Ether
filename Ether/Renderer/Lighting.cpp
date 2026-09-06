#include "Lighting.h"
#include "../Scene/BVH.h"
#include "../Math/MathUtil.h"

#include <algorithm>
#include <cmath>

namespace
{
    constexpr f32 AmbientIntensity = 0.03f;
    constexpr f32 ShadowBias       = 0.001f;
    constexpr f32 MaxTraceDistance = 100.0f;
    constexpr s32 AOSteps          = 3;
    constexpr f32 AOStepSize       = 0.1f;
    constexpr f32 AOIntensity      = 2.0f;
}

void Lighting::AddLight(const Light& light)
{
    _lights.push_back(light);
}


f32 Lighting::CalculateAO(const BVH& bvh, const Vector3& position, const Vector3& normal) const
{
    f32 occlusion = 0.0f;
    f32 weight    = 1.0f;

    for (s32 i = 1; i <= AOSteps; ++i)
    {
        f32 stepDist = i * AOStepSize;
        Vector3 p    = position + normal * stepDist;

        s32 dummyObj;
        f32 sdfDist = bvh.Distance(p, dummyObj);

        occlusion += (stepDist - sdfDist) * weight;
        weight    *= 0.5f;
    }

    return std::clamp(1.0f - (occlusion * AOIntensity), 0.0f, 1.0f);
}

Vector3 Lighting::Shade(const BVH& bvh, const SurfacePoint& surface, const Vector3& viewDir) const
{
#ifdef AMBIENT_OCCLUSION
    f32 ao = CalculateAO(bvh, surface.Position, surface.Normal);
#else
    f32 ao = 1.0f;
#endif

    Vector3 result = surface.Albedo * AmbientIntensity * ao + surface.Emission;

    Vector3 f0	  = Vector3(0.04f) * (1.0f - surface.Metallic) + surface.Albedo * surface.Metallic;
    f32 roughness = std::clamp(surface.Roughness, 0.03f, 1.0f);
    f32 alpha	  = roughness * roughness;

    f32 nDotV = std::max(surface.Normal.Dot(viewDir), 0.0f);

    for (const Light& light : _lights)
    {
        Vector3 lightDir;
        f32     maxT;
        bool isPointLight = (light.Type == LightType::Point);

        if (isPointLight)
        {
            Vector3 toLight		= light.Position - surface.Position;
            f32 distanceSquared = toLight.LengthSquared();

            if (distanceSquared < 1e-8f)
                continue;

            f32 distance = std::sqrt(distanceSquared);
            lightDir     = toLight / distance;
            maxT         = std::max(0.0f, distance - ShadowBias * 2.0f);
        }
        else
        {
            lightDir = -light.Direction.Normalized();
            maxT     = MaxTraceDistance;
        }

        f32 nDotL = surface.Normal.Dot(lightDir);
        if (nDotL <= 0.0f)
            continue;

        Vector3 shadowOrigin = surface.Position + surface.Normal * ShadowBias;
        RayHit shadowHit;
        f32 shadow = bvh.Intersect(Ray(shadowOrigin, lightDir), shadowHit, 0.0001f, maxT ) ? 0.0f : 1.0f;

        Vector3 halfVec = (viewDir + lightDir).Normalized();
        f32 nDotH		= std::max(surface.Normal.Dot(halfVec), 0.0f);
        f32 vDotH		= std::max(viewDir.Dot(halfVec), 0.0f);

        f32 denomD = nDotH * nDotH * (alpha - 1.0f) + 1.0f;
        f32 D	   = alpha / (Math::PI * denomD * denomD);

        f32 k  = ((roughness + 1.0f) * (roughness + 1.0f)) / 8.0f;
        f32 gV = nDotV / (nDotV * (1.0f - k) + k);
        f32 gL = nDotL / (nDotL * (1.0f - k) + k);
        f32 G  = gV * gL;

        Vector3 F = f0 + (Vector3(1.0f) - f0) * std::pow(1.0f - vDotH, 5.0f);

        Vector3 specular = F * (D * G / std::max(4.0f * nDotV * nDotL, 1e-4f));
        Vector3 kd		 = (Vector3(1.0f) - F) * (1.0f - surface.Metallic);
        Vector3 diffuse	 = kd * surface.Albedo * (1.0f / Math::PI);

        Vector3 radiance;
        if (isPointLight)
        {
            f32 distanceSquared = (light.Position - surface.Position).LengthSquared();
            f32 attenuation     = light.Intensity / distanceSquared;
            radiance            = light.Color * attenuation;
        }
        else
        {
            radiance = light.Color * light.Intensity;
        }

        result += (diffuse + specular) * radiance * nDotL * shadow;
    }

    return result;
}
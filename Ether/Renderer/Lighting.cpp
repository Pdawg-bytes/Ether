#include "Lighting.h"
#include "../Scene/BVH.h"
#include "../Math/MathUtil.h"

#include <algorithm>
#include <cmath>

namespace
{
    constexpr f32 AmbientIntensity     = 0.03f;
    constexpr f32 ShadowBias		   = 0.001f;
    constexpr f32 ShadowMinHitDistance = 0.0005f;
    constexpr s32 ShadowMaxSteps	   = 64;
    constexpr f32 MaxTraceDistance     = 100.0f;
}

void Lighting::AddLight(const Light& light)
{
    _lights.push_back(light);
}

bool Lighting::IsOccluded(const BVH& bvh, const Vector3& origin, const Vector3& direction, f32 maxDistance) const
{
    if (maxDistance <= 0.0f)
        return false;

    f32 traveled = 0.0f;

    for (s32 step = 0; step < ShadowMaxSteps; step++)
    {
        s32 object;
        Vector3 point = origin + direction * traveled;
        f32 distance  = bvh.Distance(point, object);

        if (distance < ShadowMinHitDistance)
            return true;

        traveled += distance;
        if (traveled >= maxDistance)
            break;
    }

    return false;
}

f32 Lighting::SoftShadow(const BVH& bvh, const Vector3& origin, const Vector3& direction, f32 minT, f32 maxT, f32 k) const
{
    f32 res = 1.0f;
    f32 t   = minT;

    for (s32 i = 0; i < ShadowMaxSteps && t < maxT; ++i)
    {
        s32 object;

        Vector3 p = origin + direction * t;
        f32 h     = bvh.Distance(p, object);

        if (h < ShadowMinHitDistance)
            return 0.0f;

        f32 shadow = std::min(1.0f, k * h / std::max(t, 0.001f));
        res        = std::min(res, shadow);

        t += std::clamp(h, 0.005f, 0.50f);
    }

    return std::clamp(res, 0.0f, 1.0f);
}

Vector3 Lighting::Shade(const BVH& bvh, const SurfacePoint& surface, const Vector3& viewDir) const
{
    Vector3 result = surface.Albedo * AmbientIntensity + surface.Emission;

    Vector3 f0	  = Vector3(0.04f) * (1.0f - surface.Metallic) + surface.Albedo * surface.Metallic;
    f32 roughness = std::clamp(surface.Roughness, 0.03f, 1.0f);
    f32 alpha	  = roughness * roughness;

    f32 nDotV = std::max(surface.Normal.Dot(viewDir), 0.0f);

    for (const Light& light : _lights)
    {
        Vector3 lightDir;
        f32 maxT;
        f32 shadowK;
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
            shadowK      = light.Radius / distance;
        }
        else
        {
            lightDir = -light.Direction.Normalized();
            maxT     = MaxTraceDistance;
            shadowK  = light.Radius;
        }

        f32 nDotL = surface.Normal.Dot(lightDir);
        if (nDotL <= 0.0f)
            continue;

        Vector3 shadowOrigin = surface.Position + surface.Normal * ShadowBias;
        
        f32 shadow = 1.0f;
        if (light.Radius <= 0.0f)
        {
            if (IsOccluded(bvh, shadowOrigin, lightDir, maxT))
                shadow = 0.0f;
        }
        else
        {
            shadow = SoftShadow(bvh, shadowOrigin, lightDir, 0.01f, maxT, shadowK);
            if (shadow <= 0.001f)
                continue;
        }

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
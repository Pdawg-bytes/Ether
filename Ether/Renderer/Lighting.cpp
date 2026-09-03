#include "Lighting.h"
#include "../Scene/BVH.h"
#include "../Math/MathUtil.h"

#include <algorithm>
#include <cmath>

namespace
{
	constexpr f32 AmbientIntensity   = 0.03f;
	constexpr f32 ShadowBias		  = 0.001f;
	constexpr f32 ShadowMinHitDistance = 0.0005f;
	constexpr s32 ShadowMaxSteps	  = 256;
}

void Lighting::AddPointLight(const PointLight& light)
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

		res = std::min(res, k * h / t);
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

	for (const PointLight& light : _lights)
	{
		Vector3 toLight		= light.Position - surface.Position;
		f32 distanceSquared = toLight.LengthSquared();

		if (distanceSquared < 1e-8f)
			continue;

		f32 distance     = std::sqrt(distanceSquared);
		Vector3 lightDir = toLight / distance;

		f32 nDotL = surface.Normal.Dot(lightDir);
		if (nDotL <= 0.0f)
			continue;

		Vector3 shadowOrigin = surface.Position + surface.Normal * ShadowBias;
		f32 maxT			 = std::max(0.0f, distance - ShadowBias * 2.0f);

		f32 shadow = 1.0f;

		if (light.Radius <= 0.0f)
		{
			if (IsOccluded(bvh, shadowOrigin, lightDir, maxT))
				continue;
		}
		else
		{
			f32 k = light.Radius / distance;
			shadow = SoftShadow(bvh, shadowOrigin, lightDir, 0.01f, maxT, k);
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

		f32 attenuation	 = light.Intensity / distanceSquared;
		Vector3 radiance = light.Color * attenuation;

		result += (diffuse + specular) * radiance * nDotL * shadow;
	}

	return result;
}
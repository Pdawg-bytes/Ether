#include "Camera.h"
#include "../Math/MathUtil.h"

#include <cmath>

Camera::Camera(const Vector3& position, u32 width, u32 height, f32 fov)
    : Position(position)
{
    UpdateViewport(width, height, fov);
}

void Camera::UpdateViewport(u32 width, u32 height, f32 fov)
{
    _width  = width;
    _height = height;
    _fov    = fov;

    UpdateView();
}

void Camera::UpdateView()
{
    f32 yawRadians   = Math::DegToRad(Yaw);
    f32 pitchRadians = Math::DegToRad(Pitch);

    Vector3 direction(
        std::cos(yawRadians) * std::cos(pitchRadians),
        std::sin(pitchRadians),
        std::sin(yawRadians) * std::cos(pitchRadians)
    );

    Forward = direction.Normalized();
    Right   = Forward.Cross(Vector3::UnitY).Normalized();
    Up      = Right.Cross(Forward);

    f32 aspectRatio = (f32)_width / (f32)_height;
    f32 halfHeight  = std::tan(Math::DegToRad(_fov) * 0.5f);
    f32 halfWidth   = halfHeight * aspectRatio;

    f32 xStepScale = 2.0f * halfWidth  / _width;
    f32 yStepScale = 2.0f * halfHeight / _height;

    _xStep = Right * xStepScale;
    _yStep = -Up   * yStepScale;

    Vector3 startX = Right * (-halfWidth + xStepScale * 0.5f);
    Vector3 startY = Up    * (halfHeight - yStepScale * 0.5f);

    _topLeftRay = Forward + startX + startY;
}

Ray Camera::GetRay(u32 x, u32 y) const
{
    Vector3 direction = (_topLeftRay + _xStep * x + _yStep * y).Normalized();
    return Ray(Position, direction);
}

bool Camera::ProjectPoint(const Vector3& worldPoint, f32& outNDCX, f32& outNDCY) const
{
    Vector3 rel = worldPoint - Position;
    f32 xCam    = rel.Dot(Right);
    f32 yCam    = rel.Dot(Up);
    f32 zCam    = rel.Dot(Forward);

    if (zCam <= 0.0001f)
        return false;

    f32 halfHeight = std::tan(Math::DegToRad(_fov) * 0.5f);
    f32 halfWidth  = halfHeight * ((f32)_width / (f32)_height);

    outNDCX = xCam / (zCam * halfWidth);
    outNDCY = yCam / (zCam * halfHeight);

    return true;
}
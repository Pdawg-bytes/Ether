#pragma once

#include "../Math/Vector3.h"
#include "../Core/Ray.h"

class Camera
{
public:
    Vector3 Position;

    f32 Yaw   = 90.0f;
    f32 Pitch = 0.0f;

    Vector3 Forward;
    Vector3 Right;
    Vector3 Up;

    Camera(const Vector3& position, u32 width, u32 height, f32 fov = 60.0f);

    void UpdateViewport(u32 width, u32 height, f32 fov);
    void UpdateView();

    f32 GetFOV() const { return _fov; }

    Ray GetRay(u32 x, u32 y) const;
    bool ProjectPoint(const Vector3& worldPoint, f32& outNDCX, f32& outNDCY) const;

private:
    u32 _width;
    u32 _height;
    f32 _fov;

    Vector3 _topLeftRay;
    Vector3 _xStep;
    Vector3 _yStep;
};
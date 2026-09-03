#pragma once

namespace Math
{
    constexpr f32 PI = 3.14159265358979323846f;

    inline f32 DegToRad(f32 degrees) { return degrees * (PI / 180.0f); }
    inline f32 RadToDeg(f32 radians) { return radians * (180.0f / PI); }
    inline f32 Lerp(f32 a, f32 b, f32 t) { return a + (b - a) * t; }
}
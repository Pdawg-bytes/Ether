#pragma once

#include <algorithm>
#include <cmath>

struct Vector2
{
    f32 X, Y;

    constexpr Vector2() : X(0.0f), Y(0.0f) {}
    constexpr Vector2(f32 x, f32 y) : X(x), Y(y) {}
    constexpr explicit Vector2(f32 scalar) : X(scalar), Y(scalar) {}

    constexpr Vector2 operator+(const Vector2& other) const { return Vector2(X + other.X, Y + other.Y); }
    constexpr Vector2 operator-(const Vector2& other) const { return Vector2(X - other.X, Y - other.Y); }
    constexpr Vector2 operator-() const { return Vector2(-X, -Y); }
    constexpr Vector2 operator*(f32 scalar) const { return Vector2(X * scalar, Y * scalar); }
    constexpr Vector2 operator*(const Vector2& other) const { return Vector2(X * other.X, Y * other.Y); }
              Vector2 operator/(f32 scalar) const { f32 inverse = 1.0f / scalar; return Vector2(X * inverse, Y * inverse); }
              Vector2 operator/(const Vector2& other) const { return Vector2(X / other.X, Y / other.Y); }

    Vector2& operator+=(const Vector2& other) { X += other.X; Y += other.Y; return *this; }
    Vector2& operator-=(const Vector2& other) { X -= other.X; Y -= other.Y; return *this; }
    Vector2& operator*=(f32 scalar) { X *= scalar; Y *= scalar; return *this; }

    constexpr bool operator==(const Vector2& other) const { return X == other.X && Y == other.Y; }
    constexpr bool operator!=(const Vector2& other) const { return !(*this == other); }

    constexpr f32 Dot(const Vector2& other) const { return X * other.X + Y * other.Y; }
    constexpr f32 LengthSquared() const { return Dot(*this); }
    f32 Length() const { return std::sqrt(LengthSquared()); }

    Vector2 Normalized() const
    {
        f32 lengthSquared = LengthSquared();
        if (lengthSquared < 1e-12f)
            return Vector2();

        return *this * (1.0f / std::sqrt(lengthSquared));
    }

    static Vector2 Min(const Vector2& a, const Vector2& b) { return Vector2(std::min(a.X, b.X), std::min(a.Y, b.Y)); }
    static Vector2 Max(const Vector2& a, const Vector2& b) { return Vector2(std::max(a.X, b.X), std::max(a.Y, b.Y)); }
    static Vector2 Abs(const Vector2& v)                   { return Vector2(std::fabs(v.X), std::fabs(v.Y)); }

    static const Vector2 Zero;
    static const Vector2 One;
};

inline Vector2 operator*(f32 scalar, const Vector2& vector) { return vector * scalar; }

inline const Vector2 Vector2::Zero(0.0f, 0.0f);
inline const Vector2 Vector2::One(1.0f, 1.0f);
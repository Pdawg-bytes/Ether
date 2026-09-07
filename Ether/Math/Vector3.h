#pragma once

#include <algorithm>
#include <cmath>

struct Vector3
{
    f32 X, Y, Z;

    constexpr Vector3() : X(0.0f), Y(0.0f), Z(0.0f) {}
    constexpr Vector3(f32 x, f32 y, f32 z) : X(x), Y(y), Z(z) {}
    constexpr explicit Vector3(f32 scalar) : X(scalar), Y(scalar), Z(scalar) {}

    constexpr Vector3 operator+(const Vector3& other) const { return Vector3(X + other.X, Y + other.Y, Z + other.Z); }
    constexpr Vector3 operator-(const Vector3& other) const { return Vector3(X - other.X, Y - other.Y, Z - other.Z); }
    constexpr Vector3 operator-() const { return Vector3(-X, -Y, -Z); }
    constexpr Vector3 operator*(f32 scalar) const { return Vector3(X * scalar, Y * scalar, Z * scalar); }
    constexpr Vector3 operator*(const Vector3& other) const { return Vector3(X * other.X, Y * other.Y, Z * other.Z); }
              Vector3 operator/(f32 scalar) const { f32 inverse = 1.0f / scalar; return Vector3(X * inverse, Y * inverse, Z * inverse); }
              Vector3 operator/(const Vector3& other) const { return Vector3(X / other.X, Y / other.Y, Z / other.Z); }

    Vector3& operator+=(const Vector3& other) { X += other.X; Y += other.Y; Z += other.Z; return *this; }
    Vector3& operator-=(const Vector3& other) { X -= other.X; Y -= other.Y; Z -= other.Z; return *this; }
    Vector3& operator*=(f32 scalar) { X *= scalar; Y *= scalar; Z *= scalar; return *this; }

    constexpr bool operator==(const Vector3& other) const { return X == other.X && Y == other.Y && Z == other.Z; }
    constexpr bool operator!=(const Vector3& other) const { return !(*this == other); }

    constexpr f32 Dot(const Vector3& other) const { return X * other.X + Y * other.Y + Z * other.Z; }

    constexpr Vector3 Cross(const Vector3& other) const
    {
        return Vector3(
            Y * other.Z - Z * other.Y,
            Z * other.X - X * other.Z,
            X * other.Y - Y * other.X
        );
    }

    constexpr f32 LengthSquared() const { return Dot(*this); }
    f32 Length() const                  { return std::sqrt(LengthSquared()); }

    Vector3 Normalized() const
    {
        f32 lengthSquared = LengthSquared();
        if (lengthSquared < 1e-12f)
            return Vector3();

        return *this * (1.0f / std::sqrt(lengthSquared));
    }

    static Vector3 Min(const Vector3& a, const Vector3& b) { return Vector3(std::min(a.X, b.X), std::min(a.Y, b.Y), std::min(a.Z, b.Z)); }
    static Vector3 Max(const Vector3& a, const Vector3& b) { return Vector3(std::max(a.X, b.X), std::max(a.Y, b.Y), std::max(a.Z, b.Z)); }
    static Vector3 Abs(const Vector3& v)                   { return Vector3(std::fabs(v.X), std::fabs(v.Y), std::fabs(v.Z)); }

    static const Vector3 Zero;
    static const Vector3 One;
    static const Vector3 UnitX;
    static const Vector3 UnitY;
    static const Vector3 UnitZ;
};

inline Vector3 operator*(f32 scalar, const Vector3& vector) { return vector * scalar; }

inline const Vector3 Vector3::Zero(0.0f, 0.0f, 0.0f);
inline const Vector3 Vector3::One(1.0f, 1.0f, 1.0f);
inline const Vector3 Vector3::UnitX(1.0f, 0.0f, 0.0f);
inline const Vector3 Vector3::UnitY(0.0f, 1.0f, 0.0f);
inline const Vector3 Vector3::UnitZ(0.0f, 0.0f, 1.0f);
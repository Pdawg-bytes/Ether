#pragma once

#include "Vector3.h"
#include <cmath>

struct Quaternion
{
	f32 X, Y, Z, W;

	constexpr Quaternion() : X(0.0f), Y(0.0f), Z(0.0f), W(1.0f) {}
	constexpr Quaternion(f32 x, f32 y, f32 z, f32 w) : X(x), Y(y), Z(z), W(w) {}

	static Quaternion FromAxisAngle(const Vector3& axis, f32 angleRadians)
	{
		f32 halfAngle    = angleRadians * 0.5f;
		f32 sinHalf      = std::sin(halfAngle);
		Vector3 normAxis = axis.Normalized();

		return Quaternion(normAxis.X * sinHalf, normAxis.Y * sinHalf, normAxis.Z * sinHalf, std::cos(halfAngle));
	}

	Quaternion operator*(const Quaternion& other) const
	{
		return Quaternion(
			W * other.X + X * other.W + Y * other.Z - Z * other.Y,
			W * other.Y - X * other.Z + Y * other.W + Z * other.X,
			W * other.Z + X * other.Y - Y * other.X + Z * other.W,
			W * other.W - X * other.X - Y * other.Y - Z * other.Z
		);
	}

	constexpr Quaternion Conjugate() const { return Quaternion(-X, -Y, -Z, W); }

	Vector3 Rotate(const Vector3& vector) const
	{
		Vector3 axis(X, Y, Z);
		Vector3 t = axis.Cross(vector) * 2.0f;

		return vector + t * W + axis.Cross(t);
	}

	Quaternion Normalized() const
	{
		f32 lengthSquared = X * X + Y * Y + Z * Z + W * W;
		if (lengthSquared < 1e-12f)
			return Quaternion();

		f32 inverseLength = 1.0f / std::sqrt(lengthSquared);

		return Quaternion(X * inverseLength, Y * inverseLength, Z * inverseLength, W * inverseLength);
	}

	static const Quaternion Identity;
};

inline const Quaternion Quaternion::Identity(0.0f, 0.0f, 0.0f, 1.0f);
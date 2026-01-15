#pragma once
#include <cmath>

struct Vector3
{
	float x{ 0.0f };
	float y{ 0.0f };
	float z{ 0.0f };

	Vector3 operator-(const Vector3& rhs) const
	{
		return Vector3{ x - rhs.x, y - rhs.y, z - rhs.z };
	}
	Vector3 operator+(const Vector3& rhs) const
	{
		return Vector3{ x + rhs.x, y + rhs.y, z + rhs.z };
	}
	Vector3 operator*(float scalar) const
	{
		return Vector3{ x * scalar, y * scalar, z * scalar };
	}
	Vector3 operator/(float scalar) const
	{
		return Vector3{ x / scalar, y / scalar, z / scalar };
	}

	float Length() const
	{
		return sqrtf(x * x + y * y + z * z);
	}

	Vector3 Normalize() const
	{
		float len = Length();
		if (len > 0)
			return *this / len;
		return *this;
	}

	float DistanceTo(const Vector3& other) const
	{
		float dx = x - other.x;
		float dy = y - other.y;
		float dz = z - other.z;
		return sqrtf(dx * dx + dy * dy + dz * dz);
	}

	static Vector3 Lerp(const Vector3& a, const Vector3& b, float t)
	{
		return a + (b - a) * t;
	}

	static Vector3 Zero() { return Vector3{ 0.f, 0.f, 0.f }; }
	static Vector3 Forward() { return Vector3{ 0.f, 0.f, 1.f }; }
};

struct Vector2
{
	float x{ 0.0f };
	float y{ 0.0f };

	Vector2 operator-(const Vector2& rhs) const
	{
		return Vector2{ x - rhs.x, y - rhs.y };
	}
	float DistanceTo(const Vector2& other) const
	{
		float dx = x - other.x;
		float dy = y - other.y;
		return sqrtf(dx * dx + dy * dy);
	}
};

struct Vector4
{
	float x{ 0.0f };
	float y{ 0.0f };
	float z{ 0.0f };
	float w{ 0.0f };
};

struct Matrix44
{
	float M[4][4]{};
};

struct Quaternion
{
	float x{ 0.0f };
	float y{ 0.0f };
	float z{ 0.0f };
	float w{ 1.0f };

	// Quaternion multiplication (combines rotations)
	Quaternion operator*(const Quaternion& rhs) const
	{
		return Quaternion{
			w * rhs.x + x * rhs.w + y * rhs.z - z * rhs.y,
			w * rhs.y - x * rhs.z + y * rhs.w + z * rhs.x,
			w * rhs.z + x * rhs.y - y * rhs.x + z * rhs.w,
			w * rhs.w - x * rhs.x - y * rhs.y - z * rhs.z
		};
	}

	// Rotate a vector by this quaternion
	// Uses the optimized formula: v' = v + 2w(u x v) + 2(u x (u x v))
	// where u = (x,y,z) and w is the scalar part
	Vector3 Multiply(const Vector3& v) const
	{
		// u = quaternion vector part
		Vector3 u{ x, y, z };
		float s = w;
		
		// Cross products
		Vector3 uv = {
			u.y * v.z - u.z * v.y,
			u.z * v.x - u.x * v.z,
			u.x * v.y - u.y * v.x
		};
		
		Vector3 uuv = {
			u.y * uv.z - u.z * uv.y,
			u.z * uv.x - u.x * uv.z,
			u.x * uv.y - u.y * uv.x
		};
		
		// v' = v + 2*s*uv + 2*uuv
		return Vector3{
			v.x + 2.0f * (s * uv.x + uuv.x),
			v.y + 2.0f * (s * uv.y + uuv.y),
			v.z + 2.0f * (s * uv.z + uuv.z)
		};
	}

	// Direction helpers using Multiply (matches reference implementation)
	Vector3 Down() const { return Multiply(Vector3{ 0.0f, -1.0f, 0.0f }); }
	Vector3 Forward() const { return Multiply(Vector3{ 0.0f, 0.0f, 1.0f }); }
	Vector3 Right() const { return Multiply(Vector3{ 1.0f, 0.0f, 0.0f }); }
	Vector3 Up() const { return Multiply(Vector3{ 0.0f, 1.0f, 0.0f }); }

	// Identity quaternion
	static Quaternion Identity()
	{
		return Quaternion{ 0.0f, 0.0f, 0.0f, 1.0f };
	}
};
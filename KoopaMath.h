#pragma once

struct Vec2
{
	union
	{
		struct { float x, y; };
	};

	Vec2(float x, float y) : x(x), y(y) {}
};

struct Vec3
{
	union
	{
		struct { float x, y, z; };
		struct { float r, g, b; };

	};

	Vec3(float x, float y, float z) : x(x), y(y), z(z) {}
};

struct Vec4
{
	union
	{
		struct { float x, y, z, w; };
		struct { float r, g, b, a; };
	};

	Vec4(float x, float y, float z, float w) : x(x), y(y), z(z), w(w) {}
};

struct Mat3
{
	float m11, m12, m13,
		  m21, m22, m23,
		  m31, m32, m33;
};
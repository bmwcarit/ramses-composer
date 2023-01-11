#pragma once
#include "AliceFloat.h"
namespace Alice{
	class Vector2f{
	public:
		union{
			struct{
				float x, y;
			};
			float v[2];
		};
	public:
		Vector2f(float x, float y);
		Vector2f(const Vector2f &v);
		Vector2f(float *v);
		Vector2f();
	public:
		void Set(const float &xValue, const float &yValue);
		void Set(const float *v);
		bool operator == (const Vector2f& v);
		bool operator != (const Vector2f& v);
		void operator=(const Vector2f &v);
		Vector2f operator+(const Vector2f &v);
		void operator+=(const Vector2f &v);
		Vector2f operator-(const Vector2f &v);
		void operator-=(const Vector2f &v);
		Vector2f operator*(float scalar);
		void operator*=(float scalar);
		float	 operator*(const Vector2f &v);
		float&	 operator[](int index) { return v[index]; };
		Vector2f operator/(float f);
		void operator/=(float f);
		float LengthSquared();
		float Length();
		bool IsPerpendicularTo(Vector2f & v);
		Vector2f ProjectTo(Vector2f&v);
		Vector2f PerpendicularTo(Vector2f&v);
		float Sin(Vector2f&v);
		float Cos(Vector2f&v);
	};
	Vector2f operator*(float scalar, Vector2f & v);
	Vector2f operator-(const Vector2f&r);
	Vector2f Normalize(Vector2f &v);
}

#pragma once
#include "AliceFloat.h"
#define ALICE_VE3_CROSS(dest,v1,v2) do{                 \
          dest[0]=v1[1]*v2[2]-v1[2]*v2[1]; \
          dest[1]=v1[2]*v2[0]-v1[0]*v2[2]; \
          dest[2]=v1[0]*v2[1]-v1[1]*v2[0];}while(0)
#define ALICE_VE3_DOT(v1,v2) (v1[0]*v2[0]+v1[1]*v2[1]+v1[2]*v2[2])
#define ALICE_VE3_SUB(dest,v1,v2) do{       \
          dest[0]=v1[0]-v2[0]; \
          dest[1]=v1[1]-v2[1]; \
          dest[2]=v1[2]-v2[2];}while(0)
namespace Alice{
	class Vector4f;
	class	Vector3f{
	public:
		union{
			struct{
				float x, y, z;
			};
			float v[3];
		};
	public:
		Vector3f(float x, float y, float z):x(x),y(y),z(z) {}
		Vector3f(const Vector3f &v) :x(v.x), y(v.y), z(v.z) {}
		Vector3f(const float *v) :x(v[0]), y(v[1]), z(v[2]) {}
		Vector3f():x(0.0f),y(0.0f),z(0.0f){}
	public:
		Vector3f&operator=(const Vector3f &v);
		void operator=(const Vector4f &v);
		Vector3f operator*(const float scalar);
		float	 operator*(const Vector3f &v);
		Vector3f operator^(const Vector3f &v);
		float&	 operator[](int index);
		bool operator == (const Vector3f& v);
		bool operator != (const Vector3f& v);
		Vector3f& operator += (const Vector3f& inV);
		Vector3f& operator -= (const Vector3f& inV);
		Vector3f& operator *= (float s);
		Vector3f operator - ();
		Vector3f operator / (float s);
		Vector3f& operator /= (float s);
		void Set(float inX, float inY, float inZ);
		void Set(const float* array);
		float	 Length();
		float	 LengthSquared();
		Vector3f Normalize();
		bool IsPerpendicularTo(Vector3f & v);
		Vector3f ProjectTo(Vector3f&v);
		Vector3f PerpendicularTo(Vector3f&v);
		float Sin(Vector3f&v);
		float Cos(Vector3f&v);
		static const float		epsilon;
	};
	Vector3f operator + (const Vector3f& lhs, const Vector3f& rhs);
	Vector3f operator - (const Vector3f& lhs, const Vector3f& rhs);
	Vector3f operator * (const float s, const Vector3f& inV);
	Vector3f minVector3(const Vector3f& lhs, const Vector3f& rhs);
	Vector3f maxVector3(const Vector3f& lhs, const Vector3f& rhs);
	Vector3f Lerp(Vector3f& from, Vector3f& to, float t);
	Vector3f Abs(const Vector3f& v);
	Vector3f Cross(const Vector3f& lhs, const Vector3f& rhs);
	float Dot(const Vector3f& lhs, const Vector3f& rhs);
	float SqrMagnitude(const Vector3f& inV);
	float Magnitude(const Vector3f& inV);
	Vector3f Normalize(Vector3f& inV);
	bool IsFinite(const Vector3f& f);
}
#pragma once
#include "Vector4.h"
#include "Vector2.h"
#include "Vector3.h"
namespace Alice {
	Vector4f::Vector4f(float x, float y, float z, float w) :x(x), y(y), z(z), w(w) {
	}
	Vector4f::Vector4f(const Vector4f &v) : x(v.x), y(v.y), z(v.z), w(v.w) {
	}
	Vector4f::Vector4f(const Vector3f &v, float w) : x(v.x), y(v.y), z(v.z), w(w) {
	}
	Vector4f::Vector4f(const float *buffer) : x(buffer[0]), y(buffer[1]), z(buffer[2]), w(buffer[3]) {
	}
	Vector4f::Vector4f() : x(0.0f), y(0.0f), z(0.0f), w(1.0f) {
	}
	Vector4f&Vector4f::operator=(const Vector4f &v) {
		x = v.x;
		y = v.y;
		z = v.z;
		w = v.w;
		return *this;
	}
	Vector4f&Vector4f::operator=(const Vector2f &v) {
		x = v.x;
		y = v.y;
		z = 0;
		w = 1.0f;
		return *this;
	}
	bool Vector4f::operator == (const Vector4f& v) const { return x == v.x && y == v.y && z == v.z && w == v.w; }
	bool Vector4f::operator != (const Vector4f& v) const { return x != v.x || y != v.y || z != v.z || w != v.w; }
	bool Vector4f::operator == (const float v[4]) const { return x == v[0] && y == v[1] && z == v[2] && w == v[3]; }
	bool Vector4f::operator != (const float v[4]) const { return x != v[0] || y != v[1] || z != v[2] || w != v[3]; }
	Vector4f Vector4f::operator*(float v) { x *= v; y *= v; z *= v; w *= v; return *this; }
	Vector4f Vector4f::operator+(const Vector4f&color) { return Vector4f(x + color.x, y + color.y, z + color.z, w + color.w); }
	Vector4f Vector4f::operator-(const Vector4f&v) { return Vector4f(x - v.x, y - v.y, z - v.z, 1.0f); }
	void Vector4f::Set(float inX, float inY, float inZ, float inW) { x = inX; y = inY; z = inZ; w = inW; }
	Vector4f Vector4f::operator - () const { return Vector4f(-x, -y, -z, -w); }
	float&	 Vector4f::operator[](int index) {
		return v[index];
	} 
	float Vector4f::SqrtMagnitude() {
		return x * x + y * y + z * z;
	}
	float Vector4f::Magnitude() {
		return sqrtf(x*x + y * y + z * z);
	}
};
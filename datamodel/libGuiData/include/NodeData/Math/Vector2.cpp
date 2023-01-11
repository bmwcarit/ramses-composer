#include "Vector2.h"
namespace Alice {
	Vector2f::Vector2f() :x(0.0f), y(0.0f) {
	}
	Vector2f::Vector2f(float *v) : x(v[0]), y(v[1]) {
	}
	Vector2f::Vector2f(float x, float y) : x(x), y(y) {
	}
	Vector2f::Vector2f(const Vector2f &v) : x(v.x), y(v.y) {
	}
	void Vector2f::Set(const float &xValue, const float &yValue) {
		x = xValue;
		y = yValue;
	}
	void Vector2f::Set(const float *v) {
		x = v[0];
		y = v[1];
	}
	bool Vector2f::operator==(const Vector2f& v) {
		return x == v.x &&y == v.y;
	}
	bool Vector2f::operator!=(const Vector2f&v) {
		return x != v.x || y != v.y;
	}
	void Vector2f::operator=(const Vector2f&v) {
		x = v.x;
		y = v.y;
	}
	Vector2f Vector2f::operator+(const Vector2f&r) {
		return Vector2f(x + r.x, y + r.y);
	}
	void Vector2f::operator+=(const Vector2f&r) {
		x += r.x;
		y += r.y;
	}
	Vector2f Vector2f::operator-(const Vector2f&r) {
		return Vector2f(x - r.x, y - r.y);
	}
	void Vector2f::operator-=(const Vector2f&r) {
		x -= r.x;
		y -= r.y;
	}
	Vector2f Vector2f::operator*(float scalar) {
		return Vector2f(x * scalar, y * scalar);
	}
	void Vector2f::operator*=(float scalar) {
		x *= scalar;
		y *= scalar;
	}
	float Vector2f::operator*(const Vector2f&r) {
		return x * r.x + y * r.y;
	}
	Vector2f Vector2f::operator/(float f) {
		if (f != 0) {
			float reciprocal = 1.0f / f;
			return Vector2f(x*reciprocal, y*reciprocal);
		}
		return Vector2f(x, y);
	}
	void Vector2f::operator/=(float f) {
		if (f != 0) {
			float reciprocal = 1.0f / f;
			x *= reciprocal;
			y *= reciprocal;
		}
	}
	float Vector2f::LengthSquared() {
		return ALICE_SQUARE(x) + ALICE_SQUARE(y);
	}
	float Vector2f::Length() {
		float length = ALICE_SQRTF(LengthSquared());
		return length;
	}
	bool Vector2f::IsPerpendicularTo(Vector2f & v) {
		return ((*this)*v == 0);
	}
	Vector2f Vector2f::ProjectTo(Vector2f&v) {
		float lenSquared = v.LengthSquared();
		return (*this)*v*v / LengthSquared();
	}
	Vector2f Vector2f::PerpendicularTo(Vector2f&v) {
		Vector2f projP2Q = ProjectTo(v);
		return (*this) - projP2Q;
	}
	float Vector2f::Sin(Vector2f&v) {
		float cos = Cos(v);
		return ALICE_SQRTF(1-ALICE_SQUARE(cos));
	}
	float Vector2f::Cos(Vector2f&v) {
		return ((*this)*v) / (Length()*v.Length());
	}
	Vector2f operator*(float scalar, Vector2f&r) {
		return  Vector2f(r.x*scalar,r.y*scalar);
	}
	Vector2f operator-(const Vector2f&r) {
		return Vector2f(-r.x, -r.y);
	}
	Vector2f Normalize(Vector2f &v) {
		return v / v.Length();
	}
}
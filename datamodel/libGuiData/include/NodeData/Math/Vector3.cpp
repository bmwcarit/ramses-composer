#include "Vector3.h"
#include "Vector4.h"
namespace Alice{
	const float		Vector3f::epsilon = 0.00001f;
	Vector3f&Vector3f::operator=(const Vector3f &v) {
		x = v.x; y = v.y; z = v.z; return *this;
	}
	void Vector3f::operator=(const Vector4f &v) {
		x = v.x;
		y = v.y;
		z = v.z;
	}
	Vector3f Vector3f::operator*(const float scalar) { 
		return Vector3f(x*scalar, y*scalar, z*scalar); 
	}
	float	 Vector3f::operator*(const Vector3f &v) { 
		return x * v.x + y * v.y + z * v.z; 
	}
	Vector3f Vector3f::operator^(const Vector3f &v) { 
		return Vector3f(y * v.z - v.y * z, v.x * z - x * v.z, x * v.y - v.x * y); 
	}
	float&	 Vector3f::operator[](int index) { 
		return v[index]; 
	}
	bool Vector3f::operator == (const Vector3f& v) { 
		return x == v.x && y == v.y && z == v.z; 
	}
	bool Vector3f::operator != (const Vector3f& v) { 
		return x != v.x || y != v.y || z != v.z; 
	}
	Vector3f& Vector3f::operator += (const Vector3f& inV) { 
		x += inV.x; y += inV.y; z += inV.z; return *this;
	}
	Vector3f& Vector3f::operator -= (const Vector3f& inV) { 
		x -= inV.x; y -= inV.y; z -= inV.z; return *this; 
	}
	Vector3f& Vector3f::operator *= (float s) { 
		x *= s; y *= s; z *= s; return *this; 
	}
	Vector3f Vector3f::operator - () { 
		return Vector3f(-x, -y, -z); 
	}
	Vector3f Vector3f::operator/(float f) {
		if (f != 0.0f) {
			float reciprocal = 1.0f / f;
			return Vector3f(x*reciprocal, y*reciprocal,z*reciprocal);
		}
		return Vector3f(x, y,z);
	}
	Vector3f& Vector3f::operator /= (float s) { 
		x /= s; y /= s; z /= s; return *this; 
	}
	void Vector3f::Set(float inX, float inY, float inZ) { 
		x = inX; y = inY; z = inZ; 
	}
	void Vector3f::Set(const float* array) { 
		x = array[0]; y = array[1]; z = array[2]; 
	}
	float	 Vector3f::Length() { 
		float len = (float)sqrt(x*x + y * y + z * z); 
		return len != 0.0f ? len : 1.0f;
	}
	float	 Vector3f::LengthSquared() { 
		float len = x * x + y * y + z * z; 
		return len != 0.0f ? len : 1.0f; 
	}
	bool Vector3f::IsPerpendicularTo(Vector3f & v) {
		return ((*this)*v == 0);
	}
	Vector3f Vector3f::ProjectTo(Vector3f&v) {
		float lenSquared = v.LengthSquared();
		return (*this)*v*v / LengthSquared();
	}
	Vector3f Vector3f::PerpendicularTo(Vector3f&v) {
		Vector3f projP2Q = ProjectTo(v);
		return (*this) - projP2Q;
	}
	Vector3f Vector3f::Normalize() { 
		float len = Length(); 
		if (EqualApproximately(len,0.0f)){
			x = 0.0f;
			y = 1.0f;
			z = 0.0f;
		}
		else {
			*this = *this * (1.0f / len);
		}
		return *this; 
	}
	float Vector3f::Cos(Vector3f&v) {
		return ((*this)*v) / (Length()*v.Length());
	}
	float Vector3f::Sin(Vector3f&v) {
		Vector3f temp = (*this) ^ v;
		return temp.Length() / (Length()*v.Length());
	}
	Vector3f operator + (const Vector3f& lhs, const Vector3f& rhs) { 
		return Vector3f(lhs.x + rhs.x, lhs.y + rhs.y, lhs.z + rhs.z); 
	}
	Vector3f operator - (const Vector3f& lhs, const Vector3f& rhs) { 
		return Vector3f(lhs.x - rhs.x, lhs.y - rhs.y, lhs.z - rhs.z);
	}
	Vector3f operator * (const float s, const Vector3f& inV) { 
		return Vector3f(inV.x * s, inV.y * s, inV.z * s); 
	}
	Vector3f minVector3(const Vector3f& lhs, const Vector3f& rhs) { 
		return Vector3f(ALICE_MIN(lhs.x, rhs.x), ALICE_MIN(lhs.y, rhs.y), ALICE_MIN(lhs.z, rhs.z)); 
	}
	Vector3f maxVector3(const Vector3f& lhs, const Vector3f& rhs) { 
		return Vector3f(ALICE_MAX(lhs.x, rhs.x), ALICE_MAX(lhs.y, rhs.y), ALICE_MAX(lhs.z, rhs.z)); 
	}
	Vector3f Lerp(Vector3f& from, Vector3f& to, float t) { 
		return from * t + to * (1.0F - t);
	}
	Vector3f Abs(const Vector3f& v) { 
		return Vector3f(ALICE_ABS(v.x), ALICE_ABS(v.y), ALICE_ABS(v.z)); 
	}
	Vector3f Cross(const Vector3f& lhs, const Vector3f& rhs){
		return Vector3f(
			lhs.y * rhs.z - lhs.z * rhs.y,
			lhs.z * rhs.x - lhs.x * rhs.z,
			lhs.x * rhs.y - lhs.y * rhs.x);
	}
	float Dot(const Vector3f& lhs, const Vector3f& rhs) { 
		return lhs.x * rhs.x + lhs.y * rhs.y + lhs.z * rhs.z; 
	}
	float SqrMagnitude(const Vector3f& inV) { 
		return Dot(inV, inV); 
	}
	float Magnitude(const Vector3f& inV) { 
		return ALICE_SQRTF(Dot(inV, inV));
	}
	Vector3f Normalize(Vector3f& inV) { 
		return inV / Magnitude(inV); 
	}
	bool IsFinite(const Vector3f& f){
		return ::IsFinite(f.x) & ::IsFinite(f.y) & ::IsFinite(f.z);
	}
}
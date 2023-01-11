#pragma once
#include <math.h>
#pragma warning(disable:4244)
#define ALICE_PI 3.14159265358979323846264338327950288419716939937510f
#define ALICE_PI_DIV_180 0.017453292519943f
#define ALICE_180_DIV_PI 57.295779513082f
#define ALICE_EPSILON 0.000001f
#define ALICE_DEG2RAD(x) ((x) * ALICE_PI_DIV_180)
#define ALICE_RAD2DEG(x) ((x) * ALICE_180_DIV_PI)
#define ALICE_SQUARE(x) (x*x)
#define ALICE_ABS(x) ((x)>0.0f?(x):-(x))
#define ALICE_SQRTF(x) sqrtf(x)
#define ALICE_INVERSQRTF(x) (1.0f/ALICE_SQRTF((x)))
#define ALICE_COSF(x) cosf(x)
#define ALICE_SINF(x) sinf(x)
#define ALICE_SIGN(x) (x>0.0f?1.0f:-1.0f)
#define ALICE_MAX(x,y) ((x)>(y)?(x):(y))
#define ALICE_MIN(x,y) ((x)<(y)?(x):(y))
bool IsFinite(const float& value);
bool IsFinite(const double& value);
const float sBiggestFloatSmallerThanOne = 0.99999994f;
namespace Alice {
	int FloorfToInt(float f);
	bool EqualApproximately(float f0, float f1, float epsilon = 0.000001f);
	float LinearInterpolate(float from, float to, float t);
	float CopySignf(float x, float y);
	template<typename T>
	T Max(const T & l, const T&r) {
		return l > r ? l : r;
	}
	template<typename T>
	T Min(const T & l, const T&r) {
		return l < r ? l : r;
	}
	template<typename T>
	T Max(const T & a, const T&b, const T&c) {
		return ((a) > (b) ? ((a) > (c) ? (a) : (c)) : ((b) > (c) ? (b) : (c)));
	}
	template<typename T>
	T Min(const T & a, const T&b, const T&c) {
		return ((a) < (b) ? ((a) < (c) ? (a) : (c)) : ((b) < (c) ? (b) : (c)));
	}
	template<typename T>
	T Alice_ABS(const T &a) {
		return a >= 0.0f ? a : -a;
	}
    inline bool IsNAN(float v){
        return v!=v;
    }
    inline bool IsNAN(double v){
        return v!=v;
    }
}

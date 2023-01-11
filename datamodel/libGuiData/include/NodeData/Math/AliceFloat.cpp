#include "AliceFloat.h"
bool IsFinite(const float& value) {
	// Returns false if value is NaN or +/- infinity
	AliceUInt32 intval = *reinterpret_cast<const AliceUInt32*>(&value);
	return (intval & 0x7f800000) != 0x7f800000;
}
bool IsFinite(const double& value) {
	// Returns false if value is NaN or +/- infinity
	AliceUInt64 intval = *reinterpret_cast<const AliceUInt64*>(&value);
	return (intval & 0x7ff0000000000000LL) != 0x7ff0000000000000LL;
}
namespace Alice {
	bool EqualApproximately(float f0, float f1, float epsilon /* = 0.000001f */) {
		float dist = (f0 - f1);
		dist = ALICE_ABS(dist);
		return dist <= epsilon;
	}
	int FloorfToInt(float f) {
		return f >= 0 ? (int)f : (int)(f - sBiggestFloatSmallerThanOne);
	}
	float LinearInterpolate(float from, float to, float t) {
		return to * t + from * (1.0f - t);
	}
	float CopySignf(float x, float y){
		union{
			float f;
			AliceUInt32 i;
		} u, u0, u1;
		u0.f = x; u1.f = y;
		AliceUInt32 a = u0.i;
		AliceUInt32 b = u1.i;
		AliceSInt32 mask = 1 << 31;
		AliceUInt32 sign = b & mask;
		a &= ~mask;
		a |= sign;
		u.i = a;
		return u.f;
	}
}
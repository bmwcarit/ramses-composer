#pragma once
#include "Matrix3x3.h"
#include "AliceMatrix4x4.h"
#include "Vector3.h"
#include <algorithm>
//#include "Runtime/Debugger/Debugger.h"

namespace Alice{
	class Quaternionf{
	public:
		float x, y, z, w;
		Quaternionf() {}
		Quaternionf(float inX, float inY, float inZ, float inW);
		inline friend Quaternionf operator * (const Quaternionf& lhs, const Quaternionf& rhs){
			return Quaternionf(
				lhs.w*rhs.x + lhs.x*rhs.w + lhs.y*rhs.z - lhs.z*rhs.y,
				lhs.w*rhs.y + lhs.y*rhs.w + lhs.z*rhs.x - lhs.x*rhs.z,
				lhs.w*rhs.z + lhs.z*rhs.w + lhs.x*rhs.y - lhs.y*rhs.x,
				lhs.w*rhs.w - lhs.x*rhs.x - lhs.y*rhs.y - lhs.z*rhs.z);
		}
		void Set(float inX, float inY, float inZ, float inW);
	};
	Quaternionf EulerToQuaternion(const Vector3f& euler);
	void QuaternionToMatrix(const Quaternionf& q, Matrix3x3& m);
}

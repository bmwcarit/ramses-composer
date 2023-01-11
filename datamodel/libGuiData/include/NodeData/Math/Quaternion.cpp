#include "Runtime/RuntimePrefix.h"
#include "Quaternion.h"
#include <limits>

namespace Alice
{
	Quaternionf::Quaternionf(float inX, float inY, float inZ, float inW)
	{
		x = inX;
		y = inY;
		z = inZ;
		w = inW;
	}
	void Quaternionf::Set(float inX, float inY, float inZ, float inW)
	{
		x = inX;
		y = inY;
		z = inZ;
		w = inW;
	}
	Quaternionf EulerToQuaternion(const Vector3f& someEulerAngles)
	{
		float cX(cos(someEulerAngles.x / 2.0f));
		float sX(sin(someEulerAngles.x / 2.0f));

		float cY(cos(someEulerAngles.y / 2.0f));
		float sY(sin(someEulerAngles.y / 2.0f));

		float cZ(cos(someEulerAngles.z / 2.0f));
		float sZ(sin(someEulerAngles.z / 2.0f));

		Quaternionf qX(sX, 0.0F, 0.0F, cX);
		Quaternionf qY(0.0f, sY, 0.0F, cY);
		Quaternionf qZ(0.0f, 0.0F, sZ, cZ);

		Quaternionf q = (qY * qX) * qZ;
		return q;
	}

	void QuaternionToMatrix(const Quaternionf& q, Matrix3x3& m)
	{
		float x = q.x * 2.0F;
		float y = q.y * 2.0F;
		float z = q.z * 2.0F;
		float xx = q.x * x;
		float yy = q.y * y;
		float zz = q.z * z;
		float xy = q.x * y;
		float xz = q.x * z;
		float yz = q.y * z;
		float wx = q.w * x;
		float wy = q.w * y;
		float wz = q.w * z;

		// Calculate 3x3 matrix from orthonormal basis
		m.mData[0] = 1.0f - (yy + zz);
		m.mData[1] = xy + wz;
		m.mData[2] = xz - wy;

		m.mData[3] = xy - wz;
		m.mData[4] = 1.0f - (xx + zz);
		m.mData[5] = yz + wx;

		m.mData[6] = xz + wy;
		m.mData[7] = yz - wx;
		m.mData[8] = 1.0f - (xx + yy);
	}
}
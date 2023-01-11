#include "Runtime/RuntimePrefix.h"
#include "Matrix3x3.h"
#include "AliceMatrix4x4.h"
using namespace std;
namespace Alice{
	void Matrix3x3::operator = (const Matrix3x3& other) {
		memcpy(mData, other.mData, sizeof(float) * 9);
	}
	void Matrix3x3::operator = (const Matrix4x4& other){
		mData[0] = other.mData[0];
		mData[1] = other.mData[1];
		mData[2] = other.mData[2];

		mData[3] = other.mData[4];
		mData[4] = other.mData[5];
		mData[5] = other.mData[6];

		mData[6] = other.mData[8];
		mData[7] = other.mData[9];
		mData[8] = other.mData[10];
	}
	Matrix3x3::Matrix3x3(){
		LoadIdentity();
	}
	Matrix3x3::Matrix3x3(const Matrix3x3& r){
		memcpy(mData, r.mData, sizeof(float) * 9);
	}
	Matrix3x3::Matrix3x3(float v) {
		mData[0] = v; mData[1] = 0.0f; mData[2] = 0.0f;
		mData[3] = 0.0f; mData[4] = v; mData[5] = 0.0f;
		mData[6] = 0.0f; mData[7] = 0.0f; mData[8] = v;
	}
	Matrix3x3::Matrix3x3(const Matrix4x4& other){
		mData[0] = other.mData[0];
		mData[1] = other.mData[1];
		mData[2] = other.mData[2];

		mData[3] = other.mData[4];
		mData[4] = other.mData[5];
		mData[5] = other.mData[6];

		mData[6] = other.mData[8];
		mData[7] = other.mData[9];
		mData[8] = other.mData[10];
	}
	void Matrix3x3::LoadIdentity(){
		mData[0] = 1.0f; mData[1] = 0.0f; mData[2] = 0.0f;
		mData[3] = 0.0f; mData[4] = 1.0f; mData[5] = 0.0f;
		mData[6] = 0.0f; mData[7] = 0.0f; mData[8] = 1.0f;
	}
	bool Matrix3x3::IsIdentity(float threshold) {
		if (EqualApproximately(Get(0, 0), 1.0f, threshold) && EqualApproximately(Get(0, 1), 0.0f, threshold) && EqualApproximately(Get(0, 2), 0.0f, threshold) &&
			EqualApproximately(Get(1, 0), 0.0f, threshold) && EqualApproximately(Get(1, 1), 1.0f, threshold) && EqualApproximately(Get(1, 2), 0.0f, threshold) &&
			EqualApproximately(Get(2, 0), 0.0f, threshold) && EqualApproximately(Get(2, 1), 0.0f, threshold) && EqualApproximately(Get(2, 2), 1.0f, threshold))
			return true;
		return false;
	}
	float Matrix3x3::GetDeterminant() const{
		float fCofactor0 = Get(0, 0) * Get(1, 1) * Get(2, 2);
		float fCofactor1 = Get(0, 1) * Get(1, 2) * Get(2, 0);
		float fCofactor2 = Get(0, 2) * Get(1, 0) * Get(2, 1);

		float fCofactor3 = Get(0, 2) * Get(1, 1) * Get(2, 0);
		float fCofactor4 = Get(0, 1) * Get(1, 0) * Get(2, 2);
		float fCofactor5 = Get(0, 0) * Get(1, 2) * Get(2, 1);

		return fCofactor0 + fCofactor1 + fCofactor2 - fCofactor3 - fCofactor4 - fCofactor5;
	}

	Matrix3x3& Matrix3x3::Transpose()
	{
		swap(Get(0, 1), Get(1, 0));
		swap(Get(0, 2), Get(2, 0));
		swap(Get(2, 1), Get(1, 2));
		return *this;
	}

	bool Matrix3x3::Invert(){
		float m11 = mData[0];
		float m12 = mData[1];
		float m13 = mData[2];
		float m21 = mData[3];
		float m22 = mData[4];
		float m23 = mData[5];
		float m31 = mData[6];
		float m32 = mData[7];
		float m33 = mData[8];
		mData[0] = m22 * m33 - m32 * m23;
		mData[1] = m32 * m13 - m12 * m33;
		mData[2] = m12 * m23 - m22 * m13;
		mData[3] = m31 * m23 - m21 * m33;
		mData[4] = m11 * m33 - m31 * m13;
		mData[5] = m21 * m13 - m11 * m23;
		mData[6] = m21 * m32 - m31 * m22;
		mData[7] = m31 * m12 - m11 * m32;
		mData[8] = m11 * m22 - m21 * m12;
		return true;
	}

	void Matrix3x3::InvertTranspose()
	{
		Invert();
		Transpose();
	}

	Matrix3x3& Matrix3x3::operator *= (float f)
	{
		for (int i = 0; i<9; i++)
			mData[i] *= f;
		return *this;
	}

	Matrix3x3& Matrix3x3::operator *= (const Matrix3x3& inM)
	{
		int i;
		for (i = 0; i<3; i++)
		{
			float v[3] = { Get(i, 0), Get(i, 1), Get(i, 2) };
			Get(i, 0) = v[0] * inM.Get(0, 0) + v[1] * inM.Get(1, 0) + v[2] * inM.Get(2, 0);
			Get(i, 1) = v[0] * inM.Get(0, 1) + v[1] * inM.Get(1, 1) + v[2] * inM.Get(2, 1);
			Get(i, 2) = v[0] * inM.Get(0, 2) + v[1] * inM.Get(1, 2) + v[2] * inM.Get(2, 2);
		}
		return *this;
	}

	Matrix3x3& Matrix3x3::operator *= (const Matrix4x4& inM)
	{
		int i;
		for (i = 0; i<3; i++)
		{
			float v[3] = { Get(i, 0), Get(i, 1), Get(i, 2) };
			Get(i, 0) = v[0] * inM.Get(0, 0) + v[1] * inM.Get(1, 0) + v[2] * inM.Get(2, 0);
			Get(i, 1) = v[0] * inM.Get(0, 1) + v[1] * inM.Get(1, 1) + v[2] * inM.Get(2, 1);
			Get(i, 2) = v[0] * inM.Get(0, 2) + v[1] * inM.Get(1, 2) + v[2] * inM.Get(2, 2);
		}
		return *this;
	}

	inline void MakePositive(Vector3f& euler)
	{
		const float negativeFlip = -0.0001F;
		const float positiveFlip = (ALICE_PI * 2.0F) - 0.0001F;

		if (euler.x < negativeFlip)
			euler.x += 2.0 * ALICE_PI;
		else if (euler.x > positiveFlip)
			euler.x -= 2.0 * ALICE_PI;

		if (euler.y < negativeFlip)
			euler.y += 2.0 * ALICE_PI;
		else if (euler.y > positiveFlip)
			euler.y -= 2.0 * ALICE_PI;

		if (euler.z < negativeFlip)
			euler.z += 2.0 * ALICE_PI;
		else if (euler.z > positiveFlip)
			euler.z -= 2.0 * ALICE_PI;
	}

	void EulerToMatrix(const Vector3f& v, Matrix3x3& matrix)
	{
		float cx = cos(v.x);
		float sx = sin(v.x);
		float cy = cos(v.y);
		float sy = sin(v.y);
		float cz = cos(v.z);
		float sz = sin(v.z);

		matrix.Get(0, 0) = cy*cz + sx*sy*sz;
		matrix.Get(0, 1) = cz*sx*sy - cy*sz;
		matrix.Get(0, 2) = cx*sy;

		matrix.Get(1, 0) = cx*sz;
		matrix.Get(1, 1) = cx*cz;
		matrix.Get(1, 2) = -sx;

		matrix.Get(2, 0) = -cz*sy + cy*sx*sz;
		matrix.Get(2, 1) = cy*cz*sx + sy*sz;
		matrix.Get(2, 2) = cx*cy;
	}
	bool MatrixToEuler(const Matrix3x3& matrix, Vector3f& v)
	{
		if (matrix.Get(1, 2) < 0.999F)
		{
			if (matrix.Get(1, 2) > -0.999F)
			{
				v.x = asin(-matrix.Get(1, 2));
				v.y = atan2(matrix.Get(0, 2), matrix.Get(2, 2));
				v.z = atan2(matrix.Get(1, 0), matrix.Get(1, 1));
				MakePositive(v);
				return true;
			}
			else
			{
				v.x = ALICE_PI * 0.5F;
				v.y = atan2(matrix.Get(0, 1), matrix.Get(0, 0));
				v.z = 0.0F;
				MakePositive(v);

				return false;
			}
		}
		else
		{
			v.x = -ALICE_PI * 0.5F;
			v.y = atan2(-matrix.Get(0, 1), matrix.Get(0, 0));
			v.z = 0.0F;
			MakePositive(v);
			return false;
		}
	}
	void RotationMatrix(const Vector3f & from, const Vector3f & to, float *rotation_matrix_3x3) {
		float dot_from_to = Dot(from, to);
	}
	void FromToRotation(const float* from, const float* to, float mtx[3][3])
	{
		float v[3];
		float e, h;
		ALICE_VE3_CROSS(v, from, to);
		e = ALICE_VE3_DOT(from, to);
		if (e>1.0 - ALICE_EPSILON)
		{
			mtx[0][0] = 1.0; mtx[0][1] = 0.0; mtx[0][2] = 0.0;
			mtx[1][0] = 0.0; mtx[1][1] = 1.0; mtx[1][2] = 0.0;
			mtx[2][0] = 0.0; mtx[2][1] = 0.0; mtx[2][2] = 1.0;
		}
		else if (e<-1.0 + ALICE_EPSILON)
		{
			float up[3], left[3];
			float invlen;
			float fxx, fyy, fzz, fxy, fxz, fyz;
			float uxx, uyy, uzz, uxy, uxz, uyz;
			float lxx, lyy, lzz, lxy, lxz, lyz;
			left[0] = 0.0; left[1] = from[2]; left[2] = -from[1];
			if (ALICE_VE3_DOT(left, left)< ALICE_EPSILON)
			{
				left[0] = -from[2]; left[1] = 0.0; left[2] = from[0];
			}
			invlen = 1.0f / sqrt(ALICE_VE3_DOT(left, left));
			left[0] *= invlen;
			left[1] *= invlen;
			left[2] *= invlen;
			ALICE_VE3_CROSS(up, left, from);
			fxx = -from[0] * from[0]; fyy = -from[1] * from[1]; fzz = -from[2] * from[2];
			fxy = -from[0] * from[1]; fxz = -from[0] * from[2]; fyz = -from[1] * from[2];
			uxx = up[0] * up[0]; uyy = up[1] * up[1]; uzz = up[2] * up[2];
			uxy = up[0] * up[1]; uxz = up[0] * up[2]; uyz = up[1] * up[2];
			lxx = -left[0] * left[0]; lyy = -left[1] * left[1]; lzz = -left[2] * left[2];
			lxy = -left[0] * left[1]; lxz = -left[0] * left[2]; lyz = -left[1] * left[2];
			mtx[0][0] = fxx + uxx + lxx; mtx[0][1] = fxy + uxy + lxy; mtx[0][2] = fxz + uxz + lxz;
			mtx[1][0] = mtx[0][1];   mtx[1][1] = fyy + uyy + lyy; mtx[1][2] = fyz + uyz + lyz;
			mtx[2][0] = mtx[0][2];   mtx[2][1] = mtx[1][2];   mtx[2][2] = fzz + uzz + lzz;
		}
		else
		{
			float hvx, hvz, hvxy, hvxz, hvyz;
			h = (1.0f - e) / ALICE_VE3_DOT(v, v);
			hvx = h*v[0];
			hvz = h*v[2];
			hvxy = hvx*v[1];
			hvxz = hvx*v[2];
			hvyz = hvz*v[1];
			mtx[0][0] = e + hvx*v[0]; mtx[0][1] = hvxy - v[2];     mtx[0][2] = hvxz + v[1];
			mtx[1][0] = hvxy + v[2];  mtx[1][1] = e + h*v[1] * v[1]; mtx[1][2] = hvyz - v[0];
			mtx[2][0] = hvxz - v[1];  mtx[2][1] = hvyz + v[0];     mtx[2][2] = e + hvz*v[2];
		}
	}
}


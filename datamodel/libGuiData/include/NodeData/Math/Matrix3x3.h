#pragma once
//#include "Runtime/Base/Object.h"
#include "Vector3.h"
namespace Alice{
	class Matrix4x4;
class Matrix3x3 /*: public Object */ {
	public:
		float mData[9];
		Matrix3x3();
		Matrix3x3(const Matrix3x3& r);
		Matrix3x3(float value);
		Matrix3x3(const Matrix4x4& m);
		float& Get(int row, int column) { return mData[row + (column * 3)]; }
		const float& Get(int row, int column)const { return mData[row + (column * 3)]; }
		void operator= (const Matrix4x4& m);
		void operator=(const Matrix3x3& r);

		Matrix3x3& operator *= (const Matrix3x3& inM);
		Matrix3x3& operator *= (const Matrix4x4& inM);
		friend Matrix3x3 operator * (const Matrix3x3& lhs, const Matrix3x3& rhs) { Matrix3x3 temp(lhs); temp *= rhs; return temp; }
		Vector3f MultiplyVector3(const Vector3f& inV) const;
		void MultiplyVector3(const Vector3f& inV, Vector3f& output) const;

		Vector3f MultiplyPoint3(const Vector3f& inV) const { return MultiplyVector3(inV); }

		Matrix3x3& operator *= (float f);
		Matrix3x3& operator /= (float f) { return *this *= (1.0F / f); }
		float GetDeterminant() const;
		Matrix3x3& Transpose();
		bool Invert();
		void InvertTranspose();
		void LoadIdentity();
		bool IsIdentity(float threshold = 0.00001f);
	};
	void RotationMatrix(const Vector3f & from,const Vector3f & to,float *rotation_matrix_3x3);
	bool MatrixToEuler(const Matrix3x3& matrix, Vector3f& v);
	void EulerToMatrix(const Vector3f& v, Matrix3x3& matrix);
	inline Vector3f Matrix3x3::MultiplyVector3(const Vector3f& v) const{
		Vector3f res;
		res.x = mData[0] * v.x + mData[3] * v.y + mData[6] * v.z;
		res.y = mData[1] * v.x + mData[4] * v.y + mData[7] * v.z;
		res.z = mData[2] * v.x + mData[5] * v.y + mData[8] * v.z;
		return res;
	}
	inline void Matrix3x3::MultiplyVector3(const Vector3f& v, Vector3f& output) const{
		output.x = mData[0] * v.x + mData[3] * v.y + mData[6] * v.z;
		output.y = mData[1] * v.x + mData[4] * v.y + mData[7] * v.z;
		output.z = mData[2] * v.x + mData[5] * v.y + mData[8] * v.z;
	}
	void FromToRotation(const float* from, const float* to, float mtx[3][3]);
}

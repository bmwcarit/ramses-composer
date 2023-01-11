#pragma once
//#include "Runtime/Base/Object.h"
#include "AliceFloat.h"
#include "Math/Vector3.h"
#include "Vector4.h"
#define ALICE_MAT4_DATA(m,r,c) (m)[(c)*4+(r)]
namespace Alice{
	class Matrix3x3;
class Matrix4x4 /*:  public Object */ {
	public:
		float mData[16];
	public:
		Matrix4x4();
		Matrix4x4(const Matrix4x4 &r);
		Matrix4x4(const Matrix3x3 &other);
		Vector3f operator*(Vector3f&vec);
		Vector3f operator*(const Vector4f &v)const;
		float& Get(int row, int column) { return mData[row + (column * 4)]; }
		const float& Get(int row, int column)const { return mData[row + (column * 4)]; }
		Matrix4x4& operator = (const Matrix3x3& m);
		void operator *= (const Matrix4x4& inM);
		Matrix4x4&operator=(const Matrix4x4&inM);
		double GetDeterminant() const;

		Vector3f MultiplyVector3(const Vector3f& inV) const;
		Vector3f MultiplyPoint3(const Vector3f& inV) const;
		void MultiplyPoint3(const Vector3f& inV, Vector3f& output) const;
		bool PerspectiveMultiplyPoint3(const Vector3f& inV, Vector3f& output) const;
		Vector3f InverseMultiplyPoint3Affine(const Vector3f& inV) const;
		Vector3f InverseMultiplyVector3Affine(const Vector3f& inV) const;

		void LookAt(Vector3f &eye,Vector3f &lookAt,Vector3f&up);
		void SetPerspective(float fovy, float aspect, float zNear, float zFar);
		void SetOrtho(float left,float right,float bottom,float top,float zNear,float zFar);

		void LoadIdentity();
		Matrix4x4& LocalTranslate(const Vector3f&trans);
		Matrix4x4& LocalTranslate(float x,float y,float z);

		void Transpose();
		void SetRotationPart(float *data);
		void SetTranslatePart(float x, float y, float z);
		Matrix4x4& Scale(float x, float y, float z);
		Matrix4x4& SetScale(float x, float y, float z);
		Matrix4x4& Rotate(Vector3f&from,Vector3f&to);
		void Rotate(Vector4f v);
		void Dump(const char * hint="matrix");
	};
	void Matrix4x4MultiplyMatrix4x4(const float* left, const float* right, float* result);
	bool InvertMatrix4x4(const float* m, float* out);
}
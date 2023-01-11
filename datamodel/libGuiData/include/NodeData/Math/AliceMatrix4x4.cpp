#include "AliceMatrix4x4.h"
#include "AliceFloat.h"
#include "Matrix3x3.h"
#include "Runtime/Debugger/Log.h"
namespace Alice{
	Matrix4x4::Matrix4x4() :Object(1, 0) {
		LoadIdentity();
	}
	Matrix4x4::Matrix4x4(const Matrix4x4 & r) : Object(1, 0) {
		memcpy(mData, r.mData, sizeof(float)*16);
	}
	Matrix4x4::Matrix4x4(const Matrix3x3 &other) : Object(1, 0) {
		mData[0] = other.mData[0];
		mData[1] = other.mData[1];
		mData[2] = other.mData[2];
		mData[3] = 0.0F;

		mData[4] = other.mData[3];
		mData[5] = other.mData[4];
		mData[6] = other.mData[5];
		mData[7] = 0.0F;

		mData[8] = other.mData[6];
		mData[9] = other.mData[7];
		mData[10] = other.mData[8];
		mData[11] = 0.0F;

		mData[12] = 0.0F;
		mData[13] = 0.0F;
		mData[14] = 0.0F;
		mData[15] = 1.0F;
	}
	double Matrix4x4::GetDeterminant() const{
		double m00 = Get(0, 0);  double m01 = Get(0, 1);  double m02 = Get(0, 2);  double m03 = Get(0, 3);
		double m10 = Get(1, 0);  double m11 = Get(1, 1);  double m12 = Get(1, 2);  double m13 = Get(1, 3);
		double m20 = Get(2, 0);  double m21 = Get(2, 1);  double m22 = Get(2, 2);  double m23 = Get(2, 3);
		double m30 = Get(3, 0);  double m31 = Get(3, 1);  double m32 = Get(3, 2);  double m33 = Get(3, 3);

		double result =
			m03 * m12 * m21 * m30 - m02 * m13 * m21 * m30 - m03 * m11 * m22 * m30 + m01 * m13 * m22 * m30 +
			m02 * m11 * m23 * m30 - m01 * m12 * m23 * m30 - m03 * m12 * m20 * m31 + m02 * m13 * m20 * m31 +
			m03 * m10 * m22 * m31 - m00 * m13 * m22 * m31 - m02 * m10 * m23 * m31 + m00 * m12 * m23 * m31 +
			m03 * m11 * m20 * m32 - m01 * m13 * m20 * m32 - m03 * m10 * m21 * m32 + m00 * m13 * m21 * m32 +
			m01 * m10 * m23 * m32 - m00 * m11 * m23 * m32 - m02 * m11 * m20 * m33 + m01 * m12 * m20 * m33 +
			m02 * m10 * m21 * m33 - m00 * m12 * m21 * m33 - m01 * m10 * m22 * m33 + m00 * m11 * m22 * m33;
		return result;
	}
	void Matrix4x4::operator *= (const Matrix4x4& inM1){
		float result[16];
		Matrix4x4MultiplyMatrix4x4(mData, inM1.mData, result);
		memcpy(mData, result, sizeof(float) * 16);
	}
	Matrix4x4&Matrix4x4::operator=(const Matrix4x4&r) {
		memcpy(mData, r.mData, sizeof(float) * 16);
		return *this;
	}
	Vector3f  Matrix4x4::operator*(Vector3f &v){
		Vector3f res;
		res.x = mData[0] * v.x + mData[4] * v.y + mData[8] * v.z;
		res.y = mData[1] * v.x + mData[5] * v.y + mData[9] * v.z;
		res.z = mData[2] * v.x + mData[6] * v.y + mData[10] * v.z;
		return res;
	}
	Vector3f  Matrix4x4::operator*(const Vector4f &v)const{
		Vector3f res;
		res.x = mData[0] * v.x + mData[4] * v.y + mData[8] * v.z + v.w*mData[12];
		res.y = mData[1] * v.x + mData[5] * v.y + mData[9] * v.z + v.w*mData[13];
		res.z = mData[2] * v.x + mData[6] * v.y + mData[10] * v.z + v.w*mData[14];
		return res;
	}

	Matrix4x4& Matrix4x4::operator = (const Matrix3x3& other)
	{
		mData[0] = other.mData[0];
		mData[1] = other.mData[1];
		mData[2] = other.mData[2];
		mData[3] = 0.0f;

		mData[4] = other.mData[3];
		mData[5] = other.mData[4];
		mData[6] = other.mData[5];
		mData[7] = 0.0f;

		mData[8] = other.mData[6];
		mData[9] = other.mData[7];
		mData[10] = other.mData[8];
		mData[11] = 0.0f;

		mData[12] = 0.0F;
		mData[13] = 0.0F;
		mData[14] = 0.0F;
		mData[15] = 1.0F;
		return *this;
	}

	Vector3f Matrix4x4::MultiplyPoint3(const Vector3f& v) const
	{
		Vector3f res;
		res.x = mData[0] * v.x + mData[4] * v.y + mData[8] * v.z + mData[12];
		res.y = mData[1] * v.x + mData[5] * v.y + mData[9] * v.z + mData[13];
		res.z = mData[2] * v.x + mData[6] * v.y + mData[10] * v.z + mData[14];
		return res;
	}

	void Matrix4x4::MultiplyPoint3(const Vector3f& v, Vector3f& output) const
	{
		output.x = mData[0] * v.x + mData[4] * v.y + mData[8] * v.z + mData[12];
		output.y = mData[1] * v.x + mData[5] * v.y + mData[9] * v.z + mData[13];
		output.z = mData[2] * v.x + mData[6] * v.y + mData[10] * v.z + mData[14];
	}


	Vector3f Matrix4x4::MultiplyVector3(const Vector3f& v) const
	{
		Vector3f res;
		res.x = mData[0] * v.x + mData[4] * v.y + mData[8] * v.z;
		res.y = mData[1] * v.x + mData[5] * v.y + mData[9] * v.z;
		res.z = mData[2] * v.x + mData[6] * v.y + mData[10] * v.z;
		return res;
	}

	bool Matrix4x4::PerspectiveMultiplyPoint3(const Vector3f& v, Vector3f& output) const{
		float w;
		float x = mData[0] * v.x + mData[4] * v.y + mData[8] * v.z + mData[12];
		float y = mData[1] * v.x + mData[5] * v.y + mData[9] * v.z + mData[13];
		float z = mData[2] * v.x + mData[6] * v.y + mData[10] * v.z + mData[14];
		w = mData[3] * v.x + mData[7] * v.y + mData[8] * v.z + mData[15];
		if (ALICE_ABS(w) > 1.0e-7f){
			float invW = 1.0f / w;
			output.x = x * invW;
			output.y = y * invW;
			output.z = z * invW;
			return true;
		}else{
			output.x = 0.0f;
			output.y = 0.0f;
			output.z = 0.0f;
			return false;
		}
	}

	Vector3f Matrix4x4::InverseMultiplyPoint3Affine(const Vector3f& inV) const
	{
		Vector3f v(inV.x - Get(0, 3), inV.y - Get(1, 3), inV.z - Get(2, 3));
		Vector3f res;
		res.x = Get(0, 0) * v.x + Get(1, 0) * v.y + Get(2, 0) * v.z;
		res.y = Get(0, 1) * v.x + Get(1, 1) * v.y + Get(2, 1) * v.z;
		res.z = Get(0, 2) * v.x + Get(1, 2) * v.y + Get(2, 2) * v.z;
		return res;
	}

	Vector3f Matrix4x4::InverseMultiplyVector3Affine(const Vector3f& v) const
	{
		Vector3f res;
		res.x = Get(0, 0) * v.x + Get(1, 0) * v.y + Get(2, 0) * v.z;
		res.y = Get(0, 1) * v.x + Get(1, 1) * v.y + Get(2, 1) * v.z;
		res.z = Get(0, 2) * v.x + Get(1, 2) * v.y + Get(2, 2) * v.z;
		return res;
	}

	void Matrix4x4::LookAt(Vector3f &eye, Vector3f &lookAt, Vector3f&up){
		up.Normalize();
		Vector3f zaxis = (eye - lookAt);
		zaxis.Normalize();
		Vector3f xaxis = Cross(up,zaxis);
		Vector3f yaxis = Cross(zaxis, xaxis);
		mData[0] = xaxis.x;
		mData[4] = xaxis.y;
		mData[8] = xaxis.z;

		mData[1] = yaxis.x;
		mData[5] = yaxis.y;
		mData[9] = yaxis.z;

		mData[2] = zaxis.x;
		mData[6] = zaxis.y;
		mData[10] = zaxis.z;
		mData[12] = -(xaxis*eye);
		mData[13] = -(yaxis*eye);
		mData[14] = -(zaxis*eye);
		mData[15] = 1.0f;
	}

	void Matrix4x4::SetPerspective(float fovy,float aspect,float zNear,float zFar){
		memset(mData, 0, 16 * sizeof(float));
		float deltaZ = zNear - zFar;
		float radian = ALICE_DEG2RAD(fovy / 2.0f );
		float yscale = ALICE_COSF(radian) / ALICE_SINF(radian);
		float xscale = yscale / aspect;
		mData[0] = xscale;
		mData[5] = yscale;
		mData[10] = (zNear + zFar) / deltaZ;
		mData[11] = -1.0f;
		mData[14] = (2.0f*zNear*zFar) / deltaZ;
	}

	void Matrix4x4::LoadIdentity(){
		memset(mData,0,16*sizeof(float));
		mData[0] = 1.0f;
		mData[5] = 1.0f;
		mData[10] = 1.0f;
		mData[15] = 1.0f;
	}
	Matrix4x4&Matrix4x4::LocalTranslate(const Vector3f&trans){
		mData[12] += trans.x;
		mData[13] += trans.y;
		mData[14] += trans.z;
		return *this;
	}
	Matrix4x4&Matrix4x4::LocalTranslate(float x, float y, float z){
		mData[12] += x;
		mData[13] += y;
		mData[14] += z;
		return *this;
	}
	void Matrix4x4::SetOrtho(
		float left,
		float right,
		float bottom,
		float top,
		float zNear,
		float zFar){
		LoadIdentity();
		float deltax = right - left;
		float deltay = top - bottom;
		float deltaz = zFar - zNear;
		Get(0, 0) = 2.0f / deltax;
		Get(0, 3) = -(right + left) / deltax;
		Get(1, 1) = 2.0f / deltay;
		Get(1, 3) = -(top + bottom) / deltay;
		Get(2, 2) = -2.0f / deltaz;
		Get(2, 3) = -(zFar + zNear) / deltaz;
	}
	Matrix4x4&Matrix4x4::SetScale(float x, float y, float z){
		mData[0] = x;
		mData[5] = y;
		mData[10] = z;
		mData[15] = 1.0f;
		return *this;
	}
	Matrix4x4&Matrix4x4::Scale(float x, float y, float z){
		mData[0] *= x;
		mData[1] *= x;
		mData[2] *= x;
		mData[3] *= x;
		mData[4] *= y;
		mData[5] *= y;
		mData[6] *= y;
		mData[7] *= y;
		mData[8] *= z;
		mData[9] *= z;
		mData[10] *= z;
		mData[11] *= z;
		return *this;
	}
	void Matrix4x4::Transpose() {
		std::swap(Get(0, 1), Get(1, 0));
		std::swap(Get(0, 2), Get(2, 0));
		std::swap(Get(0, 3), Get(3, 0));
		std::swap(Get(1, 2), Get(2, 1));
		std::swap(Get(1, 3), Get(3, 1));
		std::swap(Get(2, 3), Get(3, 2));
	}
	void Matrix4x4::SetRotationPart(float *data) {
		mData[0] = data[0];
		mData[1] = data[1];
		mData[2] = data[2];
		mData[3] = 0.0f;
		mData[4] = data[3];
		mData[5] = data[4];
		mData[6] = data[5];
		mData[7] = 0.0f;
		mData[8] = data[6];
		mData[9] = data[7];
		mData[10] = data[8];
		mData[11] = 0.0f;
	}
	void Matrix4x4::SetTranslatePart(float x, float y, float z) {
		mData[12] = x;
		mData[13] = y;
		mData[14] = z;
		mData[15] = 1.0f;
	}
	Matrix4x4&Matrix4x4::Rotate(Vector3f&from,Vector3f&to){
		float mtx[3][3];
		FromToRotation(from.v, to.v, mtx);
		Matrix4x4 rotateMat;
		
		rotateMat.Get(0, 0) = mtx[0][0];	rotateMat.Get(0, 1) = mtx[0][1];	rotateMat.Get(0, 2) = mtx[0][2];	rotateMat.Get(0, 3) = 0.0;
		rotateMat.Get(1, 0) = mtx[1][0];	rotateMat.Get(1, 1) = mtx[1][1];	rotateMat.Get(1, 2) = mtx[1][2];	rotateMat.Get(1, 3) = 0.0;
		rotateMat.Get(2, 0) = mtx[2][0];	rotateMat.Get(2, 1) = mtx[2][1];	rotateMat.Get(2, 2) = mtx[2][2];	rotateMat.Get(2, 3) = 0.0;
		rotateMat.Get(3, 0) = 0.0;		rotateMat.Get(3, 1) = 0.0;		rotateMat.Get(3, 2) = 0.0;		rotateMat.Get(3, 3) = 1.0;

		(*this) *= rotateMat;
		return *this;
	}

	void Matrix4x4::Rotate(Vector4f v){
		float s = sinf( ALICE_DEG2RAD(v.w)),
		c = cosf(ALICE_DEG2RAD(v.w)),
			xx,
			yy,
			zz,
			xy,
			yz,
			zx,
			xs,
			ys,
			zs,
			c1;
		Matrix4x4 mat;
		Vector3f t = { v.x, v.y, v.z };
		if (!v.w || !t.Length()) return;
		xx = t.x * t.x;
		yy = t.y * t.y;
		zz = t.z * t.z;
		xy = t.x * t.y;
		yz = t.y * t.z;
		zx = t.z * t.x;
		xs = t.x * s;
		ys = t.y * s;
		zs = t.z * s;
		c1 = 1.0f - c;
		mat.LoadIdentity();
		mat.Get(0,0)= (c1 * xx) + c;
		mat.Get(0,1) = (c1 * xy) - zs;
		mat.Get(0,2) = (c1 * zx) + ys;

		mat.Get(1,0) = (c1 * xy) + zs;
		mat.Get(1,1) = (c1 * yy) + c;
		mat.Get(1,2) = (c1 * yz) - xs;

		mat.Get(2, 0) = (c1 * zx) - ys;
		mat.Get(2, 1) = (c1 * yz) + xs;
		mat.Get(2, 2) = (c1 * zz) + c;
		(*this) *= mat;
	}

	void Matrix4x4::Dump(const char * hint){
		Info("%s\n%f, %f, %f, %f,\n%f, %f, %f, %f,\n%f, %f, %f, %f,\n%f, %f, %f, %f",hint,
			mData[0], mData[1], mData[2], mData[3],
			mData[4], mData[5], mData[6], mData[7],
			mData[8], mData[9], mData[10], mData[11],
			mData[12], mData[13], mData[14], mData[15]);
	}
	void Matrix4x4MultiplyMatrix4x4(const float* left, const float* right, float* result) {
		for (int i = 0; i < 4; ++i) {
			result[i] = left[i] * right[0] + left[i + 4] * right[1] + left[i + 8] * right[2] + left[i + 12] * right[3];
			result[i + 4] = left[i] * right[4] + left[i + 4] * right[5] + left[i + 8] * right[6] + left[i + 12] * right[7];
			result[i + 8] = left[i] * right[8] + left[i + 4] * right[9] + left[i + 8] * right[10] + left[i + 12] * right[11];
			result[i + 12] = left[i] * right[12] + left[i + 4] * right[13] + left[i + 8] * right[14] + left[i + 12] * right[15];
		}
	}
#define ALICE_RETURN_ZERO_MAT4 do{ \
	memset(out,16*sizeof(float),0);\
	return false; \
}while(0)
#define SWAP_ROWS(a, b) do{ float *tmp = a; (a)=(b); (b)=tmp; }while(0)
	bool InvertMatrix4x4(const float* m, float* out)
	{
		float tmp[4][8];
		float m0, m1, m2, m3, s;
		float *r0, *r1, *r2, *r3;
		r0 = tmp[0];r1 = tmp[1];r2 = tmp[2];r3 = tmp[3];

		r0[0] = ALICE_MAT4_DATA(m, 0, 0); r0[1] = ALICE_MAT4_DATA(m, 0, 1);
		r0[2] = ALICE_MAT4_DATA(m, 0, 2); r0[3] = ALICE_MAT4_DATA(m, 0, 3);
		r0[4] = 1.0; r0[5] = r0[6] = r0[7] = 0.0;
		r1[0] = ALICE_MAT4_DATA(m, 1, 0); r1[1] = ALICE_MAT4_DATA(m, 1, 1);
		r1[2] = ALICE_MAT4_DATA(m, 1, 2); r1[3] = ALICE_MAT4_DATA(m, 1, 3);
		r1[5] = 1.0; r1[4] = r1[6] = r1[7] = 0.0;
		r2[0] = ALICE_MAT4_DATA(m, 2, 0); r2[1] = ALICE_MAT4_DATA(m, 2, 1);
		r2[2] = ALICE_MAT4_DATA(m, 2, 2); r2[3] = ALICE_MAT4_DATA(m, 2, 3);
		r2[6] = 1.0; r2[4] = r2[5] = r2[7] = 0.0;
		r3[0] = ALICE_MAT4_DATA(m, 3, 0); r3[1] = ALICE_MAT4_DATA(m, 3, 1);
		r3[2] = ALICE_MAT4_DATA(m, 3, 2); r3[3] = ALICE_MAT4_DATA(m, 3, 3);
		r3[7] = 1.0; r3[4] = r3[5] = r3[6] = 0.0;

		if (ALICE_ABS(r3[0]) > ALICE_ABS(r2[0])) SWAP_ROWS(r3, r2);
		if (ALICE_ABS(r2[0]) > ALICE_ABS(r1[0])) SWAP_ROWS(r2, r1);
		if (ALICE_ABS(r1[0]) > ALICE_ABS(r0[0])) SWAP_ROWS(r1, r0);

		if (0.0F == r0[0]) ALICE_RETURN_ZERO_MAT4;

		m1 = r1[0] / r0[0]; m2 = r2[0] / r0[0]; m3 = r3[0] / r0[0];
		s = r0[1]; r1[1] -= m1 * s; r2[1] -= m2 * s; r3[1] -= m3 * s;
		s = r0[2]; r1[2] -= m1 * s; r2[2] -= m2 * s; r3[2] -= m3 * s;
		s = r0[3]; r1[3] -= m1 * s; r2[3] -= m2 * s; r3[3] -= m3 * s;
		s = r0[4];
		if (s != 0.0F) { r1[4] -= m1 * s; r2[4] -= m2 * s; r3[4] -= m3 * s; }
		s = r0[5];
		if (s != 0.0F) { r1[5] -= m1 * s; r2[5] -= m2 * s; r3[5] -= m3 * s; }
		s = r0[6];
		if (s != 0.0F) { r1[6] -= m1 * s; r2[6] -= m2 * s; r3[6] -= m3 * s; }
		s = r0[7];
		if (s != 0.0F) { r1[7] -= m1 * s; r2[7] -= m2 * s; r3[7] -= m3 * s; }
		if (ALICE_ABS(r3[1]) > ALICE_ABS(r2[1])) SWAP_ROWS(r3, r2);
		if (ALICE_ABS(r2[1]) > ALICE_ABS(r1[1])) SWAP_ROWS(r2, r1);
		if (0.0F == r1[1]) ALICE_RETURN_ZERO_MAT4;
		m2 = r2[1] / r1[1]; m3 = r3[1] / r1[1];
		r2[2] -= m2 * r1[2]; r3[2] -= m3 * r1[2];
		r2[3] -= m2 * r1[3]; r3[3] -= m3 * r1[3];
		s = r1[4]; if (0.0F != s) { r2[4] -= m2 * s; r3[4] -= m3 * s; }
		s = r1[5]; if (0.0F != s) { r2[5] -= m2 * s; r3[5] -= m3 * s; }
		s = r1[6]; if (0.0F != s) { r2[6] -= m2 * s; r3[6] -= m3 * s; }
		s = r1[7]; if (0.0F != s) { r2[7] -= m2 * s; r3[7] -= m3 * s; }
		if (ALICE_ABS(r3[2]) > ALICE_ABS(r2[2])) SWAP_ROWS(r3, r2);
		if (0.0F == r2[2]) ALICE_RETURN_ZERO_MAT4;
		m3 = r3[2] / r2[2];
		r3[3] -= m3 * r2[3]; r3[4] -= m3 * r2[4];
		r3[5] -= m3 * r2[5]; r3[6] -= m3 * r2[6];
		r3[7] -= m3 * r2[7];

		if (0.0F == r3[3]) ALICE_RETURN_ZERO_MAT4;

		s = 1.0F / r3[3];
		r3[4] *= s; r3[5] *= s; r3[6] *= s; r3[7] *= s;

		m2 = r2[3];
		s = 1.0F / r2[2];
		r2[4] = s * (r2[4] - r3[4] * m2); r2[5] = s * (r2[5] - r3[5] * m2);
		r2[6] = s * (r2[6] - r3[6] * m2); r2[7] = s * (r2[7] - r3[7] * m2);
		m1 = r1[3];
		r1[4] -= r3[4] * m1; r1[5] -= r3[5] * m1;
		r1[6] -= r3[6] * m1; r1[7] -= r3[7] * m1;
		m0 = r0[3];
		r0[4] -= r3[4] * m0; r0[5] -= r3[5] * m0;
		r0[6] -= r3[6] * m0; r0[7] -= r3[7] * m0;
		m1 = r1[2];
		s = 1.0F / r1[1];
		r1[4] = s * (r1[4] - r2[4] * m1); r1[5] = s * (r1[5] - r2[5] * m1);
		r1[6] = s * (r1[6] - r2[6] * m1); r1[7] = s * (r1[7] - r2[7] * m1);
		m0 = r0[2];
		r0[4] -= r2[4] * m0; r0[5] -= r2[5] * m0;
		r0[6] -= r2[6] * m0; r0[7] -= r2[7] * m0;
		m0 = r0[1];
		s = 1.0F / r0[0];
		r0[4] = s * (r0[4] - r1[4] * m0); r0[5] = s * (r0[5] - r1[5] * m0);
		r0[6] = s * (r0[6] - r1[6] * m0); r0[7] = s * (r0[7] - r1[7] * m0);
		ALICE_MAT4_DATA(out, 0, 0) = r0[4]; ALICE_MAT4_DATA(out, 0, 1) = r0[5]; ALICE_MAT4_DATA(out, 0, 2) = r0[6]; ALICE_MAT4_DATA(out, 0, 3) = r0[7];
		ALICE_MAT4_DATA(out, 1, 0) = r1[4]; ALICE_MAT4_DATA(out, 1, 1) = r1[5]; ALICE_MAT4_DATA(out, 1, 2) = r1[6]; ALICE_MAT4_DATA(out, 1, 3) = r1[7];
		ALICE_MAT4_DATA(out, 2, 0) = r2[4]; ALICE_MAT4_DATA(out, 2, 1) = r2[5]; ALICE_MAT4_DATA(out, 2, 2) = r2[6]; ALICE_MAT4_DATA(out, 2, 3) = r2[7];
		ALICE_MAT4_DATA(out, 3, 0) = r3[4]; ALICE_MAT4_DATA(out, 3, 1) = r3[5]; ALICE_MAT4_DATA(out, 3, 2) = r3[6]; ALICE_MAT4_DATA(out, 3, 3) = r3[7];
		return true;
	}
}
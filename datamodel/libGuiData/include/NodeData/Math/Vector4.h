#pragma once
namespace Alice {
	class Vector2f;
	class Vector3f;
	class	Vector4f{
	public:
		union
		{
			struct
			{
				float x, y, z, w;
			};
			float v[4];
		};
	public:
		Vector4f(float x, float y, float z, float w = 1.0f);
		Vector4f(const Vector4f &v);
		Vector4f(const Vector3f &v, float w = 1.0f);
		Vector4f(const float *buffer);
		Vector4f();
	public:
		Vector4f&operator=(const Vector4f &v);
		Vector4f&operator=(const Vector2f &v);
		bool operator == (const Vector4f& v) const;
		bool operator != (const Vector4f& v) const;
		bool operator == (const float v[4]) const;
		bool operator != (const float v[4]) const;
		Vector4f operator*(float v);
		Vector4f operator+(const Vector4f&color);
		Vector4f operator-(const Vector4f&v);
		void Set(float inX, float inY, float inZ, float inW = 1.0f);
		Vector4f operator - () const;
		float&	 operator[](int index);
		float SqrtMagnitude();
		float Magnitude();
	};
	typedef Vector4f Color4f;
	inline Color4f Lerp(Color4f from, Color4f to, float percent) {
		return Color4f(from*percent + to * (1.0f - percent));
	}
}
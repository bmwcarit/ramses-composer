#pragma once
#include <string>

typedef bool BoolType;
typedef double DoubleType;
typedef float FloatType;
typedef int IntType;
typedef std::string stringTpye;
struct SampleTexture{};

struct Vec2{
	float x;
	float y;
};

struct Vec3 {
	float x;
	float y;
	float z;
};

struct Vec4{
	float x;
	float y;
	float z;
	float w;
};
struct mat2 {
	float m00, m01;
	float m10, m11;
};

struct mat3 {
	float m00, m01, m02;
	float m10, m11, m12;
	float m20, m21, m22;
};

struct mat4 {
	float m00, m01, m02, m03;
	float m10, m11, m12, m13;
	float m20, m21, m22, m23;
	float m30, m31, m32, m33;
};

struct Vec2int {
	int x;
	int y;
};

struct Vec3int {
	int x;
	int y;
	int z;
};

struct Vec4int {
	int x;
	int y;
	int z;
	int w;
};
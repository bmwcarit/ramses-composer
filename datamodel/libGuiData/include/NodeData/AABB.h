#pragma once
#include "Math/Vector3.h"
#include "Math/AliceMatrix4x4.h"
#include "Math/Quaternion.h"
namespace Alice{
	class MinMaxAABB;
	class Matrix3x3;
	class AABB{
	public:
		static const AABB zero;
	public:
		Vector3f	mCenter;
		Vector3f	mExtent;
		AABB() {}
		AABB(const Vector3f& c, const Vector3f& e) { mCenter = c; mExtent = e; }
		AABB(const MinMaxAABB& aabb) { FromMinMaxAABB(aabb); }
		AABB& operator = (const MinMaxAABB& aabb) { FromMinMaxAABB(aabb); return *this; }
		bool operator == (const AABB& b)  { return mCenter == b.mCenter && mExtent == b.mExtent; }
		void SetCenterAndExtent(const Vector3f& c, const Vector3f& e) { mCenter = c; mExtent = e; }
		Vector3f&		GetCenter() { return mCenter; }
		Vector3f&		GetExtent() { return mExtent; }
		float&			GetExtent(int i) { return mExtent[i]; }
		Vector3f GetMin() const { return mCenter - mExtent; }
		Vector3f GetMax() const { return mCenter + mExtent; }
		void Expand(float inValue);
		bool IsValid() const;
		bool IsInside(Vector3f& inPoint);
		void GetVertices(Vector3f* outVertices) const;
		void Encapsulate(const Vector3f& inPoint);
	private:
		void FromMinMaxAABB(const MinMaxAABB& aabb);
	};
	MinMaxAABB AddAABB(MinMaxAABB& lhs, MinMaxAABB& rhs);

	void TransformAABB(AABB& aabb, Vector3f& position, Quaternionf& rotation, AABB& result);

	void TransformAABB(AABB& aabb, Matrix4x4& transform, AABB& result);

	float CalculateSqrDistance(Vector3f& rkPoint, AABB& rkBox);

	void CalculateClosestPoint( Vector3f& rkPoint,  AABB& rkBox, Vector3f& outPoint, float& outSqrDistance);

	class MinMaxAABB{
	public:
		Vector3f	mMin;
		Vector3f	mMax;

		MinMaxAABB() { Init(); }
		MinMaxAABB(Vector3f min, Vector3f max) : mMin(min), mMax(max) { };
		MinMaxAABB(AABB& aabb) { FromAABB(aabb); }
		MinMaxAABB& operator = (AABB& aabb) { FromAABB(aabb); return *this; }

		void Init();
		const Vector3f& GetMin() const { return mMin; }
		const Vector3f& GetMax() const { return mMax; }
		Vector3f GetCenter() const { return 0.5F * (mMax + mMin); }
		Vector3f GetExtent() const { return 0.5F * (mMax - mMin); }
		Vector3f GetSize() const { return (mMax - mMin); }

		void Encapsulate(const Vector3f& inPoint);
		void Encapsulate(const Vector4f& inPoint);
		void Encapsulate(AABB& aabb);
		void Encapsulate(const MinMaxAABB& other);

		void Expand(float inValue);
		void Expand(const Vector3f& inOffset);

		bool IsValid();

		bool IsInside(Vector3f& inPoint);

		void GetVertices(Vector3f outVertices[8]) const;

	private:

		void FromAABB(AABB& inAABB);
	};

	inline void AABB::Expand(float inValue)
	{
		mExtent += Vector3f(inValue, inValue, inValue);
	}

	inline void AABB::FromMinMaxAABB(const MinMaxAABB& inAABB)
	{
		mCenter = (inAABB.GetMax() + inAABB.GetMin()) * 0.5F;
		mExtent = (inAABB.GetMax() - inAABB.GetMin()) * 0.5F;
	}

	inline bool AABB::IsValid() const
	{
		return IsFinite(mCenter) && IsFinite(mExtent);
	}

	inline void MinMaxAABB::Encapsulate(const Vector3f& inPoint)
	{
		mMin.x = ALICE_MIN(mMin.x, inPoint.x);
		mMin.y = ALICE_MIN(mMin.y, inPoint.y);
		mMin.z = ALICE_MIN(mMin.z, inPoint.z);
		mMax.x = ALICE_MAX(mMax.x, inPoint.x);
		mMax.y = ALICE_MAX(mMax.y, inPoint.y);
		mMax.z = ALICE_MAX(mMax.z, inPoint.z);
	}

	inline void MinMaxAABB::Encapsulate(const Vector4f& inPoint){
		mMin.x = ALICE_MIN(mMin.x, inPoint.x);
		mMin.y = ALICE_MIN(mMin.y, inPoint.y);
		mMin.z = ALICE_MIN(mMin.z, inPoint.z);
		mMax.x = ALICE_MAX(mMax.x, inPoint.x);
		mMax.y = ALICE_MAX(mMax.y, inPoint.y);
		mMax.z = ALICE_MAX(mMax.z, inPoint.z);
	}

	inline void MinMaxAABB::Encapsulate(AABB& aabb)
	{
		Encapsulate(aabb.GetCenter() + aabb.GetExtent());
		Encapsulate(aabb.GetCenter() - aabb.GetExtent());
	}

	inline void MinMaxAABB::Encapsulate(const MinMaxAABB& other)
	{
		mMin = minVector3(mMin, other.mMin);
		mMax = maxVector3(mMax, other.mMax);
	}

	inline void MinMaxAABB::Expand(float inValue)
	{
		Vector3f offset = Vector3f(inValue, inValue, inValue);
		mMin -= offset;
		mMax += offset;
	}

	inline void MinMaxAABB::Expand(const Vector3f& inOffset)
	{
		mMin -= inOffset;
		mMax += inOffset;
	}
	inline bool MinMaxAABB::IsValid(){
		return !(mMin == Vector3f(std::numeric_limits<float>::infinity(), std::numeric_limits<float>::infinity(), std::numeric_limits<float>::infinity()) || 
			mMax == -Vector3f(std::numeric_limits<float>::infinity(), std::numeric_limits<float>::infinity(), std::numeric_limits<float>::infinity()));
	}
	inline void MinMaxAABB::Init(){
		mMin = Vector3f(std::numeric_limits<float>::infinity(), std::numeric_limits<float>::infinity(), std::numeric_limits<float>::infinity());
		mMax = -Vector3f(std::numeric_limits<float>::infinity(), std::numeric_limits<float>::infinity(), std::numeric_limits<float>::infinity());
	}
	inline void MinMaxAABB::FromAABB(AABB& inAABB){
		mMin = inAABB.GetCenter() - inAABB.GetExtent();
		mMax = inAABB.GetCenter() + inAABB.GetExtent();
	}
}
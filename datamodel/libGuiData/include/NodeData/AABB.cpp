#include "AABB.h"
#include "Math/Matrix3x3.h"
#include "Math/Quaternion.h"
namespace Alice{
	const AABB AABB::zero = AABB(Vector3f(0, 0, 0), Vector3f(0, 0, 0));
	void CalculateClosestPoint(Vector3f& rkPoint, AABB& rkBox, Vector3f& outPoint, float& outSqrDistance)
	{
		Vector3f kClosest = rkPoint - rkBox.GetCenter();
		float fSqrDistance = 0.0f;
		float fDelta;

		for (int i = 0; i < 3; i++)
		{
			if (kClosest[i] < -rkBox.GetExtent(i))
			{
				fDelta = kClosest[i] + rkBox.GetExtent(i);
				fSqrDistance += fDelta * fDelta;
				kClosest[i] = -rkBox.GetExtent(i);
			}
			else if (kClosest[i] > rkBox.GetExtent(i))
			{
				fDelta = kClosest[i] - rkBox.GetExtent(i);
				fSqrDistance += fDelta * fDelta;
				kClosest[i] = rkBox.GetExtent(i);
			}
		}

		if (fSqrDistance == 0.0F)
		{
			outPoint = rkPoint;
			outSqrDistance = 0.0F;
		}
		else
		{
			outPoint = kClosest + rkBox.GetCenter();
			outSqrDistance = fSqrDistance;
		}
	}

	float CalculateSqrDistance(Vector3f& rkPoint, AABB& rkBox)
	{
		Vector3f closest = rkPoint - rkBox.GetCenter();
		float sqrDistance = 0.0f;

		for (int i = 0; i < 3; ++i)
		{
			float clos = closest[i];
			float ext = rkBox.GetExtent(i);
			if (clos < -ext)
			{
				float delta = clos + ext;
				sqrDistance += delta * delta;
				closest[i] = -ext;
			}
			else if (clos > ext)
			{
				float delta = clos - ext;
				sqrDistance += delta * delta;
				closest[i] = ext;
			}
		}

		return sqrDistance;
	}

	void AABB::GetVertices(Vector3f* outVertices) const
	{
		outVertices[0] = mCenter + Vector3f(-mExtent.x, -mExtent.y, -mExtent.z);
		outVertices[1] = mCenter + Vector3f(+mExtent.x, -mExtent.y, -mExtent.z);
		outVertices[2] = mCenter + Vector3f(-mExtent.x, +mExtent.y, -mExtent.z);
		outVertices[3] = mCenter + Vector3f(+mExtent.x, +mExtent.y, -mExtent.z);

		outVertices[4] = mCenter + Vector3f(-mExtent.x, -mExtent.y, +mExtent.z);
		outVertices[5] = mCenter + Vector3f(+mExtent.x, -mExtent.y, +mExtent.z);
		outVertices[6] = mCenter + Vector3f(-mExtent.x, +mExtent.y, +mExtent.z);
		outVertices[7] = mCenter + Vector3f(+mExtent.x, +mExtent.y, +mExtent.z);
	}

	void MinMaxAABB::GetVertices(Vector3f outVertices[8]) const
	{
		outVertices[0].Set(mMin.x, mMin.y, mMin.z);
		outVertices[1].Set(mMax.x, mMin.y, mMin.z);
		outVertices[2].Set(mMax.x, mMax.y, mMin.z);
		outVertices[3].Set(mMin.x, mMax.y, mMin.z);
		outVertices[4].Set(mMin.x, mMin.y, mMax.z);
		outVertices[5].Set(mMax.x, mMin.y, mMax.z);
		outVertices[6].Set(mMax.x, mMax.y, mMax.z);
		outVertices[7].Set(mMin.x, mMax.y, mMax.z);
	}

	bool AABB::IsInside(Vector3f& inPoint)
	{
		if (inPoint[0] < mCenter[0] - mExtent[0])
			return false;
		if (inPoint[0] > mCenter[0] + mExtent[0])
			return false;

		if (inPoint[1] < mCenter[1] - mExtent[1])
			return false;
		if (inPoint[1] > mCenter[1] + mExtent[1])
			return false;

		if (inPoint[2] < mCenter[2] - mExtent[2])
			return false;
		if (inPoint[2] > mCenter[2] + mExtent[2])
			return false;

		return true;
	}

	void AABB::Encapsulate(const Vector3f& inPoint) {
		MinMaxAABB temp = *this;
		temp.Encapsulate(inPoint);
		FromMinMaxAABB(temp);
	}

	bool MinMaxAABB::IsInside(Vector3f& inPoint)
	{
		if (inPoint[0] < mMin[0])
			return false;
		if (inPoint[0] > mMax[0])
			return false;

		if (inPoint[1] < mMin[1])
			return false;
		if (inPoint[1] > mMax[1])
			return false;

		if (inPoint[2] < mMin[2])
			return false;
		if (inPoint[2] > mMax[2])
			return false;

		return true;
	}

	MinMaxAABB AddAABB(MinMaxAABB& lhs,MinMaxAABB& rhs)
	{
		MinMaxAABB minMax;
		if (lhs.IsValid())
			minMax = lhs;

		if (rhs.IsValid())
		{
			minMax.Encapsulate(rhs.GetMax());
			minMax.Encapsulate(rhs.GetMin());
		}

		return minMax;
	}

	inline Vector3f RotateExtents(const Vector3f& extents, const Matrix3x3& rotation)
	{
		Vector3f newExtents;
		for (int i = 0; i < 3; i++)
			newExtents[i] = ALICE_ABS(rotation.Get(i, 0) * extents.x) + ALICE_ABS(rotation.Get(i, 1) * extents.y) + ALICE_ABS(rotation.Get(i, 2) * extents.z);
		return newExtents;
	}

	inline Vector3f RotateExtents(const Vector3f& extents, const Matrix4x4& rotation)
	{
		Vector3f newExtents;
		for (int i = 0; i < 3; i++)
			newExtents[i] = ALICE_ABS(rotation.Get(i, 0) * extents.x) + ALICE_ABS(rotation.Get(i, 1) * extents.y) + ALICE_ABS(rotation.Get(i, 2) * extents.z);
		return newExtents;
	}

	void TransformAABB(AABB& aabb, Vector3f& position, Quaternionf& rotation, AABB& result)
	{
		Matrix3x3 m;
		QuaternionToMatrix(rotation, m);

		Vector3f extents = RotateExtents(aabb.GetExtent(), m);
		Vector3f center = m.MultiplyPoint3(aabb.GetCenter());
		center += position;
		result.SetCenterAndExtent(center, extents);
	}

	void TransformAABB(AABB& aabb, Matrix4x4& transform, AABB& result)
	{
		Vector3f extents = RotateExtents(aabb.GetExtent(), transform);
		Vector3f center = transform.MultiplyPoint3(aabb.GetCenter());
		result.SetCenterAndExtent(center, extents);
	}
}

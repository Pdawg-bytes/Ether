#pragma once

#include "../Math/Vector3.h"
#include "../Math/Quaternion.h"
#include "../Math/AABB.h"
#include "SDFPrimitives.h"

#include <memory>
#include <vector>

enum class CSGOperation
{
	Primitive,
	Union,
	Subtraction,
	Intersection,
	SmoothUnion,
	SmoothSubtraction,
	SmoothIntersection,
};

struct CSGNode
{
	CSGOperation Operation = CSGOperation::Primitive;
	f32 Smoothing		   = 0.0f;

	s32 Left  = -1;
	s32 Right = -1;

	SDFDistanceFunc DistanceFunc = nullptr;
	SDFNormalFunc NormalFunc     = nullptr;

	PrimitiveData Data{};

	Vector3 LocalPosition;

	Quaternion LocalRotation = Quaternion::Identity;
	f32 LocalScale			 = 1.0f;

	AABB LocalBounds;
};

class CSGTree
{
public:
	f32 Distance(const Vector3& localPoint) const;
	Vector3 Normal(const Vector3& localPoint) const;

	const AABB& LocalBounds() const { return _nodes[_rootIndex].LocalBounds; }

private:
	friend class CSGTreeBuilder;

	struct CSGEval
	{
		f32 Distance;
		s32 LeafIndex;
		bool Negated;
		bool Smooth;
	};

	CSGEval EvaluateNode(s32 nodeIndex, const Vector3& localPoint) const;

	std::vector<CSGNode> _nodes;
	s32 _rootIndex = -1;
};

class CSGTreeBuilder
{
public:
	s32 AddSphere(const Vector3& localPosition, f32 radius, f32 localScale = 1.0f);
	s32 AddBox(const Vector3& localPosition, const Quaternion& localRotation, const Vector3& extents, f32 localScale = 1.0f);
	s32 AddTorus(const Vector3& localPosition, const Quaternion& localRotation, f32 majorRadius, f32 minorRadius, f32 localScale = 1.0f);
	s32 AddCylinder(const Vector3& localPosition, const Quaternion& localRotation, f32 radius, f32 halfHeight, f32 localScale = 1.0f);

	s32 Union(s32 left, s32 right);
	s32 Subtraction(s32 left, s32 right);
	s32 Intersection(s32 left, s32 right);
	s32 SmoothUnion(s32 left, s32 right, f32 smoothing);
	s32 SmoothSubtraction(s32 left, s32 right, f32 smoothing);
	s32 SmoothIntersection(s32 left, s32 right, f32 smoothing);

	std::shared_ptr<CSGTree> Build(s32 rootNode);

private:
	s32 AddPrimitive(SDFDistanceFunc distanceFunc, SDFNormalFunc normalFunc, const PrimitiveData& data,
					  const Vector3& localPosition, const Quaternion& localRotation, f32 localScale, const AABB& shapeBounds);
	s32 AddOp(CSGOperation operation, s32 left, s32 right, f32 smoothing);

	std::vector<CSGNode> _nodes;
};
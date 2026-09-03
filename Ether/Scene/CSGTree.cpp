#include "CSGTree.h"
#include "SDFPrimitives.h"
#include "../Math/MathUtil.h"

#include <algorithm>
#include <cmath>

namespace
{
	AABB TransformBounds(const AABB& shapeBounds, const Vector3& position, const Quaternion& rotation, f32 scale)
	{
		Vector3 corners[8] =
		{
			Vector3(shapeBounds.Min.X, shapeBounds.Min.Y, shapeBounds.Min.Z),
			Vector3(shapeBounds.Max.X, shapeBounds.Min.Y, shapeBounds.Min.Z),
			Vector3(shapeBounds.Min.X, shapeBounds.Max.Y, shapeBounds.Min.Z),
			Vector3(shapeBounds.Max.X, shapeBounds.Max.Y, shapeBounds.Min.Z),
			Vector3(shapeBounds.Min.X, shapeBounds.Min.Y, shapeBounds.Max.Z),
			Vector3(shapeBounds.Max.X, shapeBounds.Min.Y, shapeBounds.Max.Z),
			Vector3(shapeBounds.Min.X, shapeBounds.Max.Y, shapeBounds.Max.Z),
			Vector3(shapeBounds.Max.X, shapeBounds.Max.Y, shapeBounds.Max.Z),
		};

		AABB bounds;
		for (const Vector3& corner : corners)
		{
			Vector3 worldCorner = position + rotation.Rotate(corner * scale);
			bounds.Min			= Vector3::Min(bounds.Min, worldCorner);
			bounds.Max			= Vector3::Max(bounds.Max, worldCorner);
		}

		return bounds;
	}
}

f32 CSGTree::Distance(const Vector3& localPoint) const
{
	return EvaluateNode(_rootIndex, localPoint).Distance;
}

Vector3 CSGTree::Normal(const Vector3& localPoint) const
{
	CSGEval eval = EvaluateNode(_rootIndex, localPoint);

	if (!eval.Smooth)
	{
		const CSGNode& leaf = _nodes[eval.LeafIndex];
		Vector3 p			= leaf.LocalRotation.Conjugate().Rotate(localPoint - leaf.LocalPosition) * (1.0f / leaf.LocalScale);
		Vector3 normal		= leaf.NormalFunc(leaf.Data, p);

		if (eval.Negated)
			normal = -normal;

		return leaf.LocalRotation.Rotate(normal).Normalized();
	}

	constexpr f32 Epsilon = 0.0005f;

	const Vector3 k0( 1.0f, -1.0f, -1.0f);
	const Vector3 k1(-1.0f, -1.0f,  1.0f);
	const Vector3 k2(-1.0f,  1.0f, -1.0f);
	const Vector3 k3( 1.0f,  1.0f,  1.0f);

	f32 d0 = Distance(localPoint + k0 * Epsilon);
	f32 d1 = Distance(localPoint + k1 * Epsilon);
	f32 d2 = Distance(localPoint + k2 * Epsilon);
	f32 d3 = Distance(localPoint + k3 * Epsilon);

	Vector3 gradient = k0 * d0 + k1 * d1 + k2 * d2 + k3 * d3;
	return gradient.Normalized();
}

CSGTree::CSGEval CSGTree::EvaluateNode(s32 nodeIndex, const Vector3& localPoint) const
{
	const CSGNode& node = _nodes[nodeIndex];

	if (node.Operation == CSGOperation::Primitive)
	{
		Vector3 p = node.LocalRotation.Conjugate().Rotate(localPoint - node.LocalPosition) * (1.0f / node.LocalScale);
		f32 d	  = node.DistanceFunc(node.Data, p) * node.LocalScale;

		return { d, nodeIndex, false, false };
	}

	if (node.Operation == CSGOperation::Union)
	{
		const CSGNode& left  = _nodes[node.Left];
		const CSGNode& right = _nodes[node.Right];

		f32 leftBoundDistSq  = left.LocalBounds.DistanceSquared(localPoint);
		f32 rightBoundDistSq = right.LocalBounds.DistanceSquared(localPoint);

		if (leftBoundDistSq <= rightBoundDistSq)
		{
			CSGEval a = EvaluateNode(node.Left, localPoint);
			if (a.Distance <= 0.0f || rightBoundDistSq >= a.Distance * a.Distance)
				return a;

			CSGEval b = EvaluateNode(node.Right, localPoint);
			return a.Distance <= b.Distance ? a : b;
		}
		else
		{
			CSGEval b = EvaluateNode(node.Right, localPoint);
			if (b.Distance <= 0.0f || leftBoundDistSq >= b.Distance * b.Distance)
				return b;

			CSGEval a = EvaluateNode(node.Left, localPoint);
			return a.Distance <= b.Distance ? a : b;
		}
	}

	CSGEval a = EvaluateNode(node.Left, localPoint);
	CSGEval b = EvaluateNode(node.Right, localPoint);
	f32 k	  = node.Smoothing;

	switch (node.Operation)
	{
		case CSGOperation::Subtraction:
		{
			f32 negB = -b.Distance;
			if (a.Distance >= negB)
				return a;

			return { negB, b.LeafIndex, !b.Negated, b.Smooth };
		}

		case CSGOperation::Intersection:
			return a.Distance >= b.Distance ? a : b;

		case CSGOperation::SmoothUnion:
		{
			f32 h = std::clamp(0.5f + 0.5f * (b.Distance - a.Distance) / k, 0.0f, 1.0f);
			return { Math::Lerp(b.Distance, a.Distance, h) - k * h * (1.0f - h), -1, false, true };
		}

		case CSGOperation::SmoothSubtraction:
		{
			f32 h = std::clamp(0.5f - 0.5f * (a.Distance + b.Distance) / k, 0.0f, 1.0f);
			return { Math::Lerp(a.Distance, -b.Distance, h) + k * h * (1.0f - h), -1, false, true };
		}

		case CSGOperation::SmoothIntersection:
		{
			f32 h = std::clamp(0.5f - 0.5f * (b.Distance - a.Distance) / k, 0.0f, 1.0f);
			return { Math::Lerp(b.Distance, a.Distance, h) + k * h * (1.0f - h), -1, false, true };
		}

		default:
			return a;
	}
}

s32 CSGTreeBuilder::AddPrimitive(SDFDistanceFunc distanceFunc, SDFNormalFunc normalFunc, const PrimitiveData& data,
								  const Vector3& localPosition, const Quaternion& localRotation, f32 localScale, const AABB& shapeBounds)
{
	CSGNode node;
	node.Operation	   = CSGOperation::Primitive;
	node.DistanceFunc  = distanceFunc;
	node.NormalFunc	   = normalFunc;
	node.Data		   = data;
	node.LocalPosition = localPosition;
	node.LocalRotation = localRotation;
	node.LocalScale	   = localScale;
	node.LocalBounds   = TransformBounds(shapeBounds, localPosition, localRotation, localScale);

	_nodes.push_back(node);
	return (s32)_nodes.size() - 1;
}

s32 CSGTreeBuilder::AddSphere(const Vector3& localPosition, f32 radius, f32 localScale)
{
	PrimitiveData data{};
	data.Sphere.Radius = radius;

	AABB shapeBounds(Vector3(-radius), Vector3(radius));
	return AddPrimitive(SDF::SphereDistance, SDF::SphereNormal, data, localPosition, Quaternion::Identity, localScale, shapeBounds);
}

s32 CSGTreeBuilder::AddBox(const Vector3& localPosition, const Quaternion& localRotation, const Vector3& extents, f32 localScale)
{
	PrimitiveData data{};
	data.Box.Extents = extents;

	AABB shapeBounds(-extents, extents);
	return AddPrimitive(SDF::BoxDistance, SDF::BoxNormal, data, localPosition, localRotation, localScale, shapeBounds);
}

s32 CSGTreeBuilder::AddTorus(const Vector3& localPosition, const Quaternion& localRotation, f32 majorRadius, f32 minorRadius, f32 localScale)
{
	PrimitiveData data{};
	data.Torus.MajorRadius = majorRadius;
	data.Torus.MinorRadius = minorRadius;

	f32 outerRadius = majorRadius + minorRadius;
	AABB shapeBounds(Vector3(-outerRadius, -minorRadius, -outerRadius), Vector3(outerRadius, minorRadius, outerRadius));
	return AddPrimitive(SDF::TorusDistance, SDF::TorusNormal, data, localPosition, localRotation, localScale, shapeBounds);
}

s32 CSGTreeBuilder::AddCylinder(const Vector3& localPosition, const Quaternion& localRotation, f32 radius, f32 halfHeight, f32 localScale)
{
	PrimitiveData data{};
	data.Cylinder.Radius	 = radius;
	data.Cylinder.HalfHeight = halfHeight;

	AABB shapeBounds(Vector3(-radius, -halfHeight, -radius), Vector3(radius, halfHeight, radius));
	return AddPrimitive(SDF::CylinderDistance, SDF::CylinderNormal, data, localPosition, localRotation, localScale, shapeBounds);
}

s32 CSGTreeBuilder::AddOp(CSGOperation operation, s32 left, s32 right, f32 smoothing)
{
	CSGNode node;
	node.Operation = operation;
	node.Left	   = left;
	node.Right	   = right;
	node.Smoothing = smoothing;

	AABB bounds = _nodes[left].LocalBounds.Union(_nodes[right].LocalBounds);
	if (smoothing > 0.0f)
		bounds = AABB(bounds.Min - Vector3(smoothing), bounds.Max + Vector3(smoothing));

	node.LocalBounds = bounds;

	_nodes.push_back(node);
	return (s32)_nodes.size() - 1;
}

s32 CSGTreeBuilder::Union(s32 left, s32 right)					   { return AddOp(CSGOperation::Union, left, right, 0.0f); }
s32 CSGTreeBuilder::Subtraction(s32 left, s32 right)			   { return AddOp(CSGOperation::Subtraction, left, right, 0.0f); }
s32 CSGTreeBuilder::Intersection(s32 left, s32 right)			   { return AddOp(CSGOperation::Intersection, left, right, 0.0f); }
s32 CSGTreeBuilder::SmoothUnion(s32 left, s32 right, f32 k)		   { return AddOp(CSGOperation::SmoothUnion, left, right, k); }
s32 CSGTreeBuilder::SmoothSubtraction(s32 left, s32 right, f32 k)  { return AddOp(CSGOperation::SmoothSubtraction, left, right, k); }
s32 CSGTreeBuilder::SmoothIntersection(s32 left, s32 right, f32 k) { return AddOp(CSGOperation::SmoothIntersection, left, right, k); }

std::shared_ptr<CSGTree> CSGTreeBuilder::Build(s32 rootNode)
{
	auto tree		 = std::make_shared<CSGTree>();
	tree->_nodes	 = std::move(_nodes);
	tree->_rootIndex = rootNode;
	return tree;
}
#include "CSGTree.h"
#include "SDFPrimitives.h"
#include "../Math/MathUtil.h"

#include <algorithm>
#include <cmath>

f32 CSGTree::Distance(const Vector3& localPoint) const
{
    return EvaluateNode(_rootIndex, localPoint).Distance;
}

s32 CSGTree::MaterialIndex(const Vector3& localPoint) const
{
    CSGEval eval = EvaluateNode(_rootIndex, localPoint);
    return eval.LeafIndex >= 0 ? _nodes[eval.LeafIndex].MaterialIndex : -1;
}

Vector3 CSGTree::Normal(const Vector3& localPoint) const
{
    CSGEval eval = EvaluateNode(_rootIndex, localPoint);

    if (!eval.Smooth)
    {
        const CSGNode& leaf = _nodes[eval.LeafIndex];
        Vector3 p			= leaf.LocalRotation.Conjugate().Rotate(localPoint - leaf.LocalPosition) / leaf.LocalScale;
        Vector3 normal		= leaf.NormalFunc(leaf.Data, p);
		normal              = (normal / leaf.LocalScale).Normalized();

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
        Vector3 p = node.LocalRotation.Conjugate().Rotate(localPoint - node.LocalPosition) * node.InvLocalScale;
        f32 d     = node.DistanceFunc(node.Data, p) * node.MinLocalScale;

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

    if (node.Operation == CSGOperation::Subtraction)
    {
        const CSGNode& right = _nodes[node.Right];
        f32 rightBoundDistSq = right.LocalBounds.DistanceSquared(localPoint);

        CSGEval a = EvaluateNode(node.Left, localPoint);

        if (rightBoundDistSq > 0.0f && (a.Distance >= 0.0f || rightBoundDistSq >= a.Distance * a.Distance))
            return a;

        CSGEval b = EvaluateNode(node.Right, localPoint);
        f32 negB  = -b.Distance;

        if (a.Distance >= negB)
            return a;

        return { negB, b.LeafIndex, !b.Negated, b.Smooth };
    }

    CSGEval a = EvaluateNode(node.Left, localPoint);
    CSGEval b = EvaluateNode(node.Right, localPoint);
    f32 k     = node.Smoothing;

    switch (node.Operation)
    {
    case CSGOperation::Intersection:
        return a.Distance >= b.Distance ? a : b;

    case CSGOperation::SmoothUnion:
    {
        f32 h        = std::clamp(0.5f + 0.5f * (b.Distance - a.Distance) / k, 0.0f, 1.0f);
        f32 distance = Math::Lerp(b.Distance, a.Distance, h) - k * h * (1.0f - h);

        const CSGEval& nearest = a.Distance <= b.Distance ? a : b;
        return { distance, nearest.LeafIndex, nearest.Negated, true };
    }

    case CSGOperation::SmoothSubtraction:
    {
        f32 h        = std::clamp(0.5f - 0.5f * (a.Distance + b.Distance) / k, 0.0f, 1.0f);
        f32 distance = Math::Lerp(a.Distance, -b.Distance, h) + k * h * (1.0f - h);
        bool useB    = a.Distance < -b.Distance;

        return { distance, useB ? b.LeafIndex : a.LeafIndex, useB ? !b.Negated : a.Negated, true };
    }

    case CSGOperation::SmoothIntersection:
    {
        f32 h        = std::clamp(0.5f - 0.5f * (b.Distance - a.Distance) / k, 0.0f, 1.0f);
        f32 distance = Math::Lerp(b.Distance, a.Distance, h) + k * h * (1.0f - h);

        const CSGEval& nearest = a.Distance >= b.Distance ? a : b;
        return { distance, nearest.LeafIndex, nearest.Negated, true };
    }

    default:
        return a;
    }
}

s32 CSGTreeBuilder::AddPrimitiveNode(const SceneObject& object)
{
    CSGNode node;
    node.Operation	   = CSGOperation::Primitive;
    node.DistanceFunc  = object.DistanceFunc;
    node.NormalFunc	   = object.NormalFunc;
    node.Data		   = object.Data;
    node.MaterialIndex = object.MaterialIndex;
    node.LocalPosition = object.Position;
    node.LocalRotation = object.Rotation;
    node.LocalBounds   = object.WorldBounds;

    node.SetLocalScale(object.Scale);

    _nodes.push_back(node);
    return (s32)_nodes.size() - 1;
}

s32 CSGTreeBuilder::Add(const SceneObject& object)
{
    if (object.Template || !object.IsBounded || !object.DistanceFunc || !object.NormalFunc)
        return -1;

    return AddPrimitiveNode(object);
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
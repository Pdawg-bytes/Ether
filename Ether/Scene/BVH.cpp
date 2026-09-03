#include "BVH.h"

#include <algorithm>
#include <cmath>
#include <limits>

namespace
{
    constexpr s32 SAHBucketCount = 16;
    constexpr f32 TraversalCost  = 1.0f;

    struct SAHBucket
    {
        s32  Count = 0;
        AABB Bounds;
    };

    inline f32 GetAxisComponent(const Vector3& v, s32 axis)
    {
        return axis == 0 ? v.X : axis == 1 ? v.Y : v.Z;
    }

    inline s32 ComputeBucketIndex(f32 value, f32 axisMin, f32 invAxisRange)
    {
        s32 bucket = (s32)((value - axisMin) * invAxisRange);
        return std::clamp(bucket, 0, SAHBucketCount - 1);
    }
}


void BVH::Build(std::vector<SceneObject> objects)
{
    _objects   = std::move(objects);
    _rootIndex = -1;

    _nodes.clear();
    _unboundedIndices.clear();

    if (_objects.empty())
        return;

    std::vector<s32> boundedIndices;
    for (s32 i = 0; i < (s32)_objects.size(); i++)
    {
        if (_objects[i].IsBounded)
            boundedIndices.push_back(i);
        else
            _unboundedIndices.push_back(i);
    }

    if (!boundedIndices.empty())
    {
        _nodes.reserve(boundedIndices.size() * 2 - 1);
        _rootIndex = BuildRecursive(boundedIndices, 0, (s32)boundedIndices.size());
    }
}

s32 BVH::BuildRecursive(std::vector<s32>& indices, s32 start, s32 end)
{
    s32 count = end - start;

    AABB bounds;
    for (s32 i = start; i < end; i++)
        bounds = bounds.Union(_objects[indices[i]].WorldBounds);

    if (count == 1)
    {
        BVHNode node;
        node.Bounds      = bounds;
        node.ObjectIndex = indices[start];

        _nodes.push_back(node);
        return (s32)_nodes.size() - 1;
    }

    AABB centroidBounds;
    for (s32 i = start; i < end; i++)
    {
        Vector3 center = _objects[indices[i]].WorldBounds.Center();
        centroidBounds = centroidBounds.Union(AABB(center, center));
    }

    SAHBucket buckets[3][SAHBucketCount];

    s32 bestAxis  = -1;
    s32 bestSplit = -1;
    f32 bestCost  = std::numeric_limits<f32>::max();

    for (s32 axis = 0; axis < 3; axis++)
    {
        f32 axisMin   = GetAxisComponent(centroidBounds.Min, axis);
        f32 axisMax   = GetAxisComponent(centroidBounds.Max, axis);
        f32 axisRange = axisMax - axisMin;

        if (axisRange <= 1e-6f)
            continue;

        f32 invAxisRange = (f32)SAHBucketCount / axisRange;

        for (s32 i = start; i < end; i++)
        {
            const AABB& objBounds = _objects[indices[i]].WorldBounds;
            f32 center			  = GetAxisComponent(objBounds.Center(), axis);

            SAHBucket& bucket = buckets[axis][ComputeBucketIndex(center, axisMin, invAxisRange)];
            bucket.Count++;
            bucket.Bounds = bucket.Bounds.Union(objBounds);
        }

        AABB leftBounds[SAHBucketCount - 1];
        s32  leftCount[SAHBucketCount - 1];
        AABB accumBounds;
        s32  accumCount = 0;

        for (s32 i = 0; i < SAHBucketCount - 1; i++)
        {
            accumBounds    = accumBounds.Union(buckets[axis][i].Bounds);
            accumCount    += buckets[axis][i].Count;
            leftBounds[i]  = accumBounds;
            leftCount[i]   = accumCount;
        }

        accumBounds = AABB();
        accumCount  = 0;

        for (s32 i = SAHBucketCount - 1; i > 0; i--)
        {
            accumBounds  = accumBounds.Union(buckets[axis][i].Bounds);
            accumCount  += buckets[axis][i].Count;

            s32 splitIndex = i - 1;
            if (leftCount[splitIndex] == 0 || accumCount == 0)
                continue;

            f32 cost = TraversalCost
                     + leftCount[splitIndex] * leftBounds[splitIndex].SurfaceArea()
                     + accumCount * accumBounds.SurfaceArea();

            if (cost < bestCost)
            {
                bestCost  = cost;
                bestAxis  = axis;
                bestSplit = splitIndex;
            }
        }
    }

    s32 mid;

    if (bestAxis < 0)
    {
        mid = start + count / 2;
    }
    else
    {
        f32 axisMin      = GetAxisComponent(centroidBounds.Min, bestAxis);
        f32 axisMax      = GetAxisComponent(centroidBounds.Max, bestAxis);
        f32 invAxisRange = (f32)SAHBucketCount / (axisMax - axisMin);

        auto middleIt = std::partition(indices.begin() + start, indices.begin() + end,
            [this, bestAxis, axisMin, invAxisRange, bestSplit](s32 index)
            {
                f32 center = GetAxisComponent(_objects[index].WorldBounds.Center(), bestAxis);
                return ComputeBucketIndex(center, axisMin, invAxisRange) <= bestSplit;
            });

        mid = (s32)(middleIt - indices.begin());

        if (mid == start || mid == end)
            mid = start + count / 2;
    }

    s32 leftIndex  = BuildRecursive(indices, start, mid);
    s32 rightIndex = BuildRecursive(indices, mid, end);

    BVHNode node;
    node.Bounds = bounds;
    node.Left   = leftIndex;
    node.Right  = rightIndex;

    _nodes.push_back(node);
    return (s32)_nodes.size() - 1;
}


f32 BVH::Distance(const Vector3& worldPoint, s32& hitObjectIndex) const
{
    f32 bestDistance = std::numeric_limits<f32>::max();
    hitObjectIndex   = -1;

    for (s32 unboundedIndex : _unboundedIndices)
    {
        f32 distance = _objects[unboundedIndex].Distance(worldPoint);
        if (distance < bestDistance)
        {
            bestDistance   = distance;
            hitObjectIndex = unboundedIndex;
        }
    }

    if (_rootIndex < 0)
        return bestDistance;

    s32 stack[64]	   = {};
    s32 stackSize	   = 0;
    stack[stackSize++] = _rootIndex;

    while (stackSize > 0)
    {
        const BVHNode& node = _nodes[stack[--stackSize]];

        if (node.Bounds.DistanceSquared(worldPoint) > bestDistance * bestDistance)
            continue;

        if (node.IsLeaf())
        {
            const SceneObject& object = _objects[node.ObjectIndex];

            f32 sphereDistance = (worldPoint - object.Position).Length() - object.BoundingRadius;
            if (sphereDistance > bestDistance)
                continue;

            f32 distance = object.Distance(worldPoint);
            if (distance < bestDistance)
            {
                bestDistance   = distance;
                hitObjectIndex = node.ObjectIndex;
            }
        }
        else
        {
            const BVHNode& left  = _nodes[node.Left];
            const BVHNode& right = _nodes[node.Right];

            f32 leftDistSq  = left.Bounds.DistanceSquared(worldPoint);
            f32 rightDistSq = right.Bounds.DistanceSquared(worldPoint);

            if (leftDistSq < rightDistSq)
            {
                stack[stackSize++] = node.Right;
                stack[stackSize++] = node.Left;
            }
            else
            {
                stack[stackSize++] = node.Left;
                stack[stackSize++] = node.Right;
            }
        }
    }

    return bestDistance;
}


BVHMetrics BVH::ComputeMetrics() const
{
    BVHMetrics metrics;
    
    if (_rootIndex < 0)
        return metrics;
    
    s32 maxDepth = 0;
    s32 totalDepth = 0;
    s32 nodeCount = 0;
    
    ComputeMetricsRecursive(_rootIndex, 0, maxDepth, totalDepth, nodeCount, metrics);
    
    metrics.SAHCost       = ComputeSAHCost(_rootIndex);
    metrics.MaxDepth      = maxDepth;
    metrics.TotalNodes    = (s32)_nodes.size();
    metrics.AverageDepth  = (f32)totalDepth / metrics.LeafCount;
    metrics.BalanceFactor = (f32)maxDepth / (metrics.TotalNodes > 0 ? (s32)std::log2(metrics.TotalNodes) : 1);
    
    return metrics;
}

void BVH::ComputeMetricsRecursive(s32 nodeIndex, s32 depth, s32& maxDepth, s32& totalDepth, s32& nodeCount, BVHMetrics& metrics) const
{
    if (nodeIndex < 0)
        return;
    
    const BVHNode& node = _nodes[nodeIndex];
    nodeCount++;
    
    if (node.IsLeaf())
    {
        maxDepth = std::max(maxDepth, depth);
        totalDepth += depth;
        metrics.LeafCount++;
    }
    else
    {
        metrics.InternalNodeCount++;
        ComputeMetricsRecursive(node.Left, depth + 1, maxDepth, totalDepth, nodeCount, metrics);
        ComputeMetricsRecursive(node.Right, depth + 1, maxDepth, totalDepth, nodeCount, metrics);
    }
}

f32 BVH::ComputeSAHCost(s32 nodeIndex) const
{
    if (nodeIndex < 0)
        return 0.0f;
    
    const BVHNode& node = _nodes[nodeIndex];
    
    if (node.IsLeaf())
        return node.Bounds.SurfaceArea();
    
    f32 leftCost  = ComputeSAHCost(node.Left);
    f32 rightCost = ComputeSAHCost(node.Right);
    
    return 1.0f + leftCost + rightCost;
}
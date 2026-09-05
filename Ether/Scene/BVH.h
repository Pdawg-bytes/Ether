#pragma once

#include "SceneObject.h"
#include <vector>

struct BVHNode
{
    AABB Bounds;
    s32  Left        = -1;
    s32  Right       = -1;
    s32  ObjectIndex = -1;

    bool IsLeaf() const { return ObjectIndex >= 0; }
};

struct BVHMetrics
{
    f32 SAHCost           = 0.0f;
    s32 MaxDepth          = 0;
    s32 TotalNodes        = 0;
    s32 LeafCount         = 0;
    s32 InternalNodeCount = 0;
    f32 BalanceFactor     = 0.0f;
    f32 AverageDepth      = 0.0f;
};

class BVH
{
public:
    void Build(std::vector<SceneObject> objects);

    bool Intersect(const Ray& ray, RayHit& hit, f32 minT = 0.0001f, f32 maxT = 1e30f) const;
    f32 Distance(const Vector3& worldPoint, s32& hitObjectIndex) const;

    const SceneObject& GetObject(s32 index) const { return _objects[index]; }
    s32 GetObjectCount() const                    { return (s32)_objects.size(); }

    s32 GetNodeCount() const			    { return (s32)_nodes.size(); }
    const BVHNode& GetNode(s32 index) const { return _nodes[index]; }
    
    BVHMetrics ComputeMetrics() const;

private:
    s32 BuildRecursive(std::vector<s32>& indices, s32 start, s32 end);
    void ComputeMetricsRecursive(s32 nodeIndex, s32 depth, s32& maxDepth, s32& totalDepth, s32& nodeCount, BVHMetrics& metrics) const;
    f32 ComputeSAHCost(s32 nodeIndex) const;

    std::vector<SceneObject> _objects;
    std::vector<s32> _unboundedIndices;
    std::vector<BVHNode> _nodes;
    s32 _rootIndex = -1;
};
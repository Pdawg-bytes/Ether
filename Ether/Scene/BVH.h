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

class BVH
{
public:
	void Build(std::vector<SceneObject> objects);

	f32 Distance(const Vector3& worldPoint, s32& hitObjectIndex) const;

	const SceneObject& GetObject(s32 index) const { return _objects[index]; }

	s32 GetNodeCount() const			    { return (s32)_nodes.size(); }
	const BVHNode& GetNode(s32 index) const { return _nodes[index]; }

private:
	s32 BuildRecursive(std::vector<s32>& indices, s32 start, s32 end);

	std::vector<SceneObject> _objects;
	std::vector<s32> _unboundedIndices;
	std::vector<BVHNode> _nodes;
	s32 _rootIndex = -1;
};
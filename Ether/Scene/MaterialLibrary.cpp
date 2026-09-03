#include "MaterialLibrary.h"

s32 MaterialLibrary::Add(Material material)
{
	_materials.push_back(std::move(material));
	return (s32)_materials.size() - 1;
}

const Material& MaterialLibrary::Get(s32 index) const
{
	static const Material DefaultMaterial{};

	if (index < 0 || index >= (s32)_materials.size())
		return DefaultMaterial;

	return _materials[index];
}

MaterialLibrary& GetMaterialLibrary()
{
	static MaterialLibrary library;
	return library;
}
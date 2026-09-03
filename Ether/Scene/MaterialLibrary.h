#pragma once

#include "Material.h"

#include <vector>

class MaterialLibrary
{
public:
    s32 Add(Material material);
    const Material& Get(s32 index) const;

private:
    std::vector<Material> _materials;
};

MaterialLibrary& GetMaterialLibrary();
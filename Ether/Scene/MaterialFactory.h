#pragma once

#include "Material.h"
#include "Texture.h"
#include "MaterialLibrary.h"

#include <memory>

class MaterialFactory
{
public:
    struct BuiltInMaterials
    {
        s32 Checker;
        s32 White;
        s32 Red;
        s32 Green;
        s32 Metal;
        s32 Glass;
        s32 Emissive;
        s32 Mirror;
        s32 Diffuse;
    };

    static BuiltInMaterials RegisterBuiltInMaterials()
    {
        std::shared_ptr<Texture> checkerTexture = Texture::CreateCheckerboard(64, 64, 8, Vector3(0.9f), Vector3(0.08f));

        Material checker;
        checker.AlbedoMap    = checkerTexture;
        checker.Roughness    = 1.0f;
        checker.Metallic     = 0.0f;
        checker.TextureScale = 0.5f;

        Material metal;
        metal.Albedo    = Vector3(0.85f, 0.8f, 0.65f);
        metal.Roughness = 0.25f;
        metal.Metallic  = 1.0f;

        Material glass;
        glass.Albedo       = Vector3(0.9f, 0.95f, 1.0f);
        glass.Roughness    = 0.05f;
        glass.IOR          = 1.5f;
        glass.Transmission = 0.9f;

        Material emissive;
        emissive.Albedo    = Vector3(0.2f, 0.05f, 0.05f);
        emissive.Roughness = 0.4f;
        emissive.Emission  = Vector3(3.0f, 0.6f, 0.2f);

        Material mirror;
        mirror.Albedo    = Vector3::One;
        mirror.Roughness = 0.2f;
        mirror.Metallic  = 1.0f;

        Material diffuse;
        diffuse.Albedo    = Vector3(0.45f, 0.40f, 0.15f);
        diffuse.Roughness = 1.0f;

        Material white;
        white.Albedo    = Vector3(0.75f);
        white.Roughness = 1.0f;

        Material red;
        red.Albedo    = Vector3(0.65f, 0.05f, 0.04f);
        red.Roughness = 1.0f;

        Material green;
        green.Albedo    = Vector3(0.05f, 0.55f, 0.08f);
        green.Roughness = 1.0f;

        MaterialLibrary& library = GetMaterialLibrary();

        BuiltInMaterials materials;
        materials.Checker  = library.Add(checker);
        materials.White    = library.Add(white);
        materials.Red      = library.Add(red);
        materials.Green    = library.Add(green);
        materials.Metal    = library.Add(metal);
        materials.Glass    = library.Add(glass);
        materials.Emissive = library.Add(emissive);
        materials.Mirror   = library.Add(mirror);
        materials.Diffuse  = library.Add(diffuse);
        return materials;
    }
};
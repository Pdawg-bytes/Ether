#include "Platform/Window.h"
#include "Renderer/Camera.h"
#include "Renderer/Raymarcher.h"
#include "Renderer/Lighting.h"
#include "Scene/BVH.h"
#include "Scene/SceneObject.h"
#include "Scene/CSGTree.h"
#include "Scene/Material.h"
#include "Scene/MaterialLibrary.h"
#include "Scene/Texture.h"
#include "Math/MathUtil.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <memory>
#include <string>
#include <vector>

namespace
{
    constexpr u32 Width			   = 200;
    constexpr u32 Height		   = 120;
    constexpr f32 MoveSpeed		   = 3.5f;
    constexpr f32 MouseSensitivity = 0.15f;

    struct SceneMaterials
    {
        s32 Checker;
        s32 Metal;
        s32 Glass;
        s32 Emissive;
        s32 Mirror;
        s32 Diffuse;
    };

    SceneMaterials RegisterMaterials()
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

        MaterialLibrary& library = GetMaterialLibrary();

        SceneMaterials materials;
        materials.Checker  = library.Add(checker);
        materials.Metal    = library.Add(metal);
        materials.Glass    = library.Add(glass);
        materials.Emissive = library.Add(emissive);
        materials.Mirror   = library.Add(mirror);
        materials.Diffuse  = library.Add(diffuse);
        return materials;
    }


    std::shared_ptr<CSGTree> BuildPillarTemplate()
    {
        CSGTreeBuilder builder;

        s32 shaft = builder.AddBox(Vector3(0.0f, 1.5f, 0.0f), Quaternion::Identity, Vector3(0.4f, 1.5f, 0.4f));
        s32 cap   = builder.AddSphere(Vector3(0.0f, 3.0f, 0.0f), 0.55f);
        s32 body  = builder.Union(shaft, cap);

        return builder.Build(body);
    }

    std::shared_ptr<CSGTree> BuildBlobTemplate()
    {
        constexpr f32 Blend = 0.35f;

        CSGTreeBuilder builder;

        s32 a = builder.AddSphere(Vector3(0.0f, 0.0f, 0.0f), 0.6f);
        s32 b = builder.AddSphere(Vector3(0.7f, 0.3f, 0.2f), 0.45f);
        s32 c = builder.AddSphere(Vector3(-0.5f, 0.45f, -0.3f), 0.5f);

        s32 ab   = builder.SmoothUnion(a, b, Blend);
        s32 root = builder.SmoothUnion(ab, c, Blend);

        return builder.Build(root);
    }

    std::shared_ptr<CSGTree> BuildGearTemplate(s32 ringMaterial, s32 toothMaterial)
    {
        constexpr s32 ToothCount = 8;

        CSGTreeBuilder builder;

        s32 ring = builder.AddTorus(Vector3::Zero, Quaternion::Identity, 1.0f, 0.22f, 1.0f, ringMaterial);

        s32 root = ring;
        for (s32 i = 0; i < ToothCount; i++)
        {
            f32 angle		    = (Math::PI * 2.0f) * ((f32)i / (f32)ToothCount);
            Quaternion rotation = Quaternion::FromAxisAngle(Vector3::UnitY, angle);
            Vector3 position    = rotation.Rotate(Vector3(0.0f, 0.0f, 1.0f));

            s32 tooth = builder.AddBox(position, rotation, Vector3(0.15f, 0.15f, 0.35f), 1.0f, toothMaterial);
            root	  = builder.Union(root, tooth);
        }

        s32 axleHole = builder.AddCylinder(Vector3::Zero, Quaternion::Identity, 0.3f, 1.0f);
        root		 = builder.Subtraction(root, axleHole);

        return builder.Build(root);
    }

    BVH BuildScene(const SceneMaterials& materials)
    {
        std::shared_ptr<CSGTree> pillarTemplate = BuildPillarTemplate();
        std::shared_ptr<CSGTree> blobTemplate   = BuildBlobTemplate();
        std::shared_ptr<CSGTree> gearTemplate   = BuildGearTemplate(materials.Emissive, materials.Metal);

        std::vector<SceneObject> objects;

        objects.push_back(SceneObject::CreatePlane(Vector3::UnitY, 0.0f, materials.Checker));

        constexpr s32 PillarCount  = 6;
        constexpr f32 PillarRadius = 6.0f;

        for (s32 i = 0; i < PillarCount; i++)
        {
            f32 angle = (Math::PI * 2.0f) * ((f32)i / (f32)PillarCount);
            Vector3 position(std::sin(angle) * PillarRadius, 0.0f, std::cos(angle) * PillarRadius);

            f32 heightScale = 0.8f + 0.4f * ((f32)i / (f32)PillarCount);
            objects.push_back(SceneObject::CreateFromTemplate(pillarTemplate, position, Quaternion::Identity, Vector3(1.0f, heightScale, 1.0f), materials.Metal));
        }

        objects.push_back(SceneObject::CreateFromTemplate(blobTemplate, Vector3(0.0f, 1.6f, 0.0f), Quaternion::Identity, Vector3::One, materials.Glass));
        objects.push_back(SceneObject::CreateFromTemplate(blobTemplate, Vector3(2.5f, 1.2f, -2.0f), Quaternion::Identity, Vector3(0.7f, 0.5f, 0.7f), materials.Mirror));
        objects.push_back(SceneObject::CreateFromTemplate(blobTemplate, Vector3(-3.0f, 2.0f, 1.5f), Quaternion::FromAxisAngle(Vector3::UnitY, Math::PI / 3.0f)));

        objects.push_back(SceneObject::CreateFromTemplate(
            gearTemplate,
            Vector3(4.0f, 0.3f, 4.0f),
            Quaternion::FromAxisAngle(Vector3::UnitX, Math::PI / 2.0f)
        ));

        objects.push_back(SceneObject::CreateFromTemplate(
            gearTemplate,
            Vector3(-4.0f, 1.3f, -4.0f),
            Quaternion::FromAxisAngle(Vector3::UnitZ, Math::PI / 8.0f),
            Vector3(1.0f, 1.6f, 1.0f)
        ));

        BVH bvh;
        bvh.Build(std::move(objects));
        return bvh;
    }

    BVH BuildCornellBox(const SceneMaterials& materials)
    {
        std::vector<SceneObject> objects;

        objects.push_back(SceneObject::CreatePlane( Vector3::UnitY, 1.0f, materials.Checker));
        objects.push_back(SceneObject::CreatePlane(-Vector3::UnitY, 3.0f));
        objects.push_back(SceneObject::CreatePlane( Vector3::UnitX, 2.0f, materials.Diffuse));
        objects.push_back(SceneObject::CreatePlane(-Vector3::UnitX, 2.0f, materials.Diffuse));
        objects.push_back(SceneObject::CreatePlane(-Vector3::UnitZ, 4.0f));
        objects.push_back(SceneObject::CreatePlane( Vector3::UnitZ, 3.5f));

        objects.push_back(SceneObject::CreateBox(
            Vector3(-0.80f, 0.0f, 2.8f),
            Quaternion::FromAxisAngle(Vector3::UnitY, Math::PI / 4.0f),
            Vector3(0.5f, 1.0f, 0.5f),
            Vector3::One,
            materials.Diffuse
        ));

        objects.push_back(SceneObject::CreateSphere(Vector3(0.75f, -0.35f, 3.15f), 0.55f, Vector3::One, materials.Mirror));

        BVH bvh;
        bvh.Build(std::move(objects));
        return bvh;
    }

    Lighting BuildLighting()
    {
        Lighting lighting;
        //lighting.AddLight(MakePointLight(Vector3(3.0f, 4.5f, -2.0f), Vector3(1.0f, 0.95f, 0.85f), 40.0f, 20.0f));
        //lighting.AddLight(MakePointLight(Vector3(-4.0f, 3.0f, 3.0f), Vector3(0.4f, 0.6f, 1.0f), 25.0f, 10.0f));
        lighting.AddLight(MakePointLight(Vector3(0.0f, 2.98f, 2.2f), Vector3(1.0f, 0.95f, 0.85f), 10.0f, 1.0f));
        return lighting;
    }
}


s32 main()
{
    Window window(Width, Height, L"Ether");
    Camera camera(Vector3(0.0f, 1.0f, -2.0f), Width, Height);

    SceneMaterials materials = RegisterMaterials();
    BVH bvh				     = BuildCornellBox(materials);
    Lighting lighting		 = BuildLighting();

    Raymarcher raymarcher(camera, bvh, lighting, Width, Height);

    std::vector<u32> framebuffer(Width * Height);

    bool showBVH   = false;
    bool prevBDown = false;

    auto lastTime		= std::chrono::steady_clock::now();
    s32 frameCount	    = 0;
    f64 timeAccumulator = 0.0;

    while (window.PollEvents())
    {
        auto currentTime = std::chrono::steady_clock::now();
        f32 deltaTime    = std::chrono::duration<f32>(currentTime - lastTime).count();
        lastTime	     = currentTime;

        f32 mouseDeltaX, mouseDeltaY;
        window.GetMouseDelta(mouseDeltaX, mouseDeltaY);
        bool cameraChanged = mouseDeltaX != 0.0f || mouseDeltaY != 0.0f;

        camera.Yaw   += mouseDeltaX * MouseSensitivity;
        camera.Pitch -= mouseDeltaY * MouseSensitivity;
        camera.Pitch  = std::clamp(camera.Pitch, -89.0f, 89.0f);

        Vector3 movement = Vector3::Zero;
        if (window.IsKeyDown('W')) movement += camera.Forward;
        if (window.IsKeyDown('S')) movement -= camera.Forward;
        if (window.IsKeyDown('A')) movement -= camera.Right;
        if (window.IsKeyDown('D')) movement += camera.Right;
        if (window.IsKeyDown(VK_SPACE)) movement += Vector3::UnitY;
        if (window.IsKeyDown(VK_SHIFT)) movement -= Vector3::UnitY;

        f32 speed = MoveSpeed * deltaTime;
        if (window.IsKeyDown(VK_CONTROL)) speed *= 4.0f;

        if (movement != Vector3::Zero)
        {
            camera.Position += movement.Normalized() * speed;
            cameraChanged = true;
        }

        if (cameraChanged)
            camera.UpdateView();

        bool bDown = window.IsKeyDown('B');
        if (bDown && !prevBDown) showBVH = !showBVH;
        prevBDown = bDown;

        raymarcher.Render(framebuffer.data());
        window.Present(framebuffer.data(), showBVH ? &camera : nullptr, showBVH ? &bvh : nullptr);

        frameCount++;
        timeAccumulator += deltaTime;
        if (timeAccumulator >= 0.5)
        {
            f64 fps			   = frameCount / timeAccumulator;
            std::wstring title = L"Ether | FPS: " + std::to_wstring((s32)fps);
            window.SetTitle(title.c_str());

            frameCount	    = 0;
            timeAccumulator = 0.0;
        }
    }

    return 0;
}
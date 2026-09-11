#include "Platform/Runtime.h"
#include "Renderer/Camera.h"
#include "Renderer/Raytracer.h"
#include "Renderer/Lighting.h"
#include "Scene/BVH.h"
#include "Scene/SceneObject.h"
#include "Scene/CSGTree.h"
#include "Scene/Material.h"
#include "Scene/MaterialLibrary.h"
#include "Scene/MaterialFactory.h"
#include "Scene/Texture.h"
#include "Math/MathUtil.h"
#include "Scene/OBJLoader.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <memory>
#include <string>
#include <vector>

namespace
{
    constexpr u32 Width	          = 200;
    constexpr u32 Height          = 120;
    constexpr f32 MoveSpeed       = 1.5f;
    constexpr f32 LookSensitivity = 0.15f;

    using SceneMaterials = MaterialFactory::BuiltInMaterials;

    std::shared_ptr<CSGTree> BuildPillarTemplate()
    {
        CSGTreeBuilder builder;

        s32 shaft = builder.Add(SceneObject::CreateBox(Vector3(0.0f, 1.5f, 0.0f), Quaternion::Identity, Vector3(0.4f, 1.5f, 0.4f)));
        s32 cap   = builder.Add(SceneObject::CreateSphere(Vector3(0.0f, 3.0f, 0.0f), 0.55f));
        s32 body  = builder.Union(shaft, cap);

        return builder.Build(body);
    }

    std::shared_ptr<CSGTree> BuildBlobTemplate()
    {
        constexpr f32 Blend = 0.35f;

        CSGTreeBuilder builder;

        s32 a = builder.Add(SceneObject::CreateSphere(Vector3(0.0f, 0.0f, 0.0f), 0.6f));
        s32 b = builder.Add(SceneObject::CreateSphere(Vector3(0.7f, 0.3f, 0.2f), 0.45f));
        s32 c = builder.Add(SceneObject::CreateSphere(Vector3(-0.5f, 0.45f, -0.3f), 0.5f));

        s32 ab   = builder.SmoothUnion(a, b, Blend);
        s32 root = builder.SmoothUnion(ab, c, Blend);

        return builder.Build(root);
    }

    std::shared_ptr<CSGTree> BuildGearTemplate(s32 ringMaterial, s32 toothMaterial)
    {
        constexpr s32 ToothCount = 8;

        CSGTreeBuilder builder;

        s32 ring = builder.Add(SceneObject::CreateTorus(Vector3::Zero, Quaternion::Identity, 1.0f, 0.22f, 1.0f, ringMaterial));

        s32 root = ring;
        for (s32 i = 0; i < ToothCount; i++)
        {
            f32 angle		    = (Math::PI * 2.0f) * ((f32)i / (f32)ToothCount);
            Quaternion rotation = Quaternion::FromAxisAngle(Vector3::UnitY, angle);
            Vector3 position    = rotation.Rotate(Vector3(0.0f, 0.0f, 1.0f));

            s32 tooth = builder.Add(SceneObject::CreateBox(position, rotation, Vector3(0.15f, 0.15f, 0.35f), Vector3(1.0f), toothMaterial));
            root	  = builder.Union(root, tooth);
        }

        s32 axleHole = builder.Add(SceneObject::CreateCylinder(Vector3::Zero, Quaternion::Identity, 0.3f, 1.0f));
        root		 = builder.Subtraction(root, axleHole);

        return builder.Build(root);
    }


    BVH BuildCSGScene(const SceneMaterials& materials)
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

        objects.push_back(SceneObject::CreatePlane( Vector3::UnitY, 1.0f, materials.White));
        objects.push_back(SceneObject::CreatePlane(-Vector3::UnitY, 3.0f, materials.White));
        objects.push_back(SceneObject::CreatePlane( Vector3::UnitX, 2.0f, materials.Red));
        objects.push_back(SceneObject::CreatePlane(-Vector3::UnitX, 2.0f, materials.Green));
        objects.push_back(SceneObject::CreatePlane(-Vector3::UnitZ, 4.0f, materials.White));
        objects.push_back(SceneObject::CreatePlane( Vector3::UnitZ, 3.5f, materials.White));

        objects.push_back(SceneObject::CreateBox(
            Vector3(-0.80f, 0.0f, 2.8f),
            Quaternion::FromAxisAngle(Vector3::UnitY, Math::PI / 4.0f),
            Vector3(0.5f, 1.0f, 0.5f),
            Vector3::One,
            materials.White
        ));

        objects.push_back(SceneObject::CreateSphere(Vector3(0.75f, -0.35f, 3.15f), 0.55f, Vector3::One, materials.Mirror));

        BVH bvh;
        bvh.Build(std::move(objects));
        return bvh;
    }

    BVH BuildOBJScene(const SceneMaterials& materials)
    {
        std::vector<SceneObject> rootScene = OBJLoader::LoadFromFile(
            "Monado/monado.obj",
            Vector3(0.0f, 0.1f, 0.0f),
            Quaternion::Identity,
            Vector3::One
        );

        std::vector<SceneObject> teapot = OBJLoader::LoadFromFile(
            "Teapot/teapot.obj",
            Vector3(1.5f, 0.0f, 0.0f),
            Quaternion::Identity,
            Vector3(0.1f)
        );

        rootScene.reserve(rootScene.size() + teapot.size());
        rootScene.insert(rootScene.end(), teapot.begin(), teapot.end());

        rootScene.push_back(SceneObject::CreatePlane(Vector3::UnitY, 0.0f, materials.Mirror));

        BVH bvh;
        bvh.Build(std::move(rootScene));
        return bvh;
    }
    

    Lighting BuildLighting()
    {
        Lighting lighting;
        lighting.AddLight(MakeDirectionalLight(Vector3(0.7, -1.0, 0.5), Vector3(1.0f, 0.89f, 0.71f), 5.0f));
        //lighting.AddLight(MakePointLight(Vector3(3.0f, 4.5f, -2.0f), Vector3(1.0f, 0.95f, 0.85f), 40.0f, 20.0f));
        //lighting.AddLight(MakePointLight(Vector3(-4.0f, 3.0f, 3.0f), Vector3(0.4f, 0.6f, 1.0f), 25.0f, 10.0f));
        //lighting.AddLight(MakePointLight(Vector3(0.0f, 2.98f, 2.2f), Vector3(1.0f, 0.95f, 0.85f), 10.0f, 1.0f));
        return lighting;
    }
}


int main()
{
    Platform::Runtime runtime(Width, Height, "Ether");
    Camera camera(Vector3(0.0f, 1.0f, -2.0f), Width, Height);

    SceneMaterials materials = MaterialFactory::RegisterBuiltInMaterials();
    BVH bvh				     = BuildOBJScene(materials);
    Lighting lighting		 = BuildLighting();

    {
        BVHMetrics metrics = bvh.ComputeMetrics();
        char buffer[256];
        snprintf(buffer, sizeof(buffer), "BVH Metrics");
        runtime.Log(buffer);
        snprintf(buffer, sizeof(buffer), "  Objects:   %ld", (long)bvh.GetObjectCount());
        runtime.Log(buffer);
        snprintf(buffer, sizeof(buffer), "  Nodes:     %ld", (long)bvh.GetNodeCount());
        runtime.Log(buffer);
        snprintf(buffer, sizeof(buffer), "  Max Depth: %ld", (long)metrics.MaxDepth);
        runtime.Log(buffer);
        snprintf(buffer, sizeof(buffer), "  Avg Depth: %.2f", metrics.AverageDepth);
        runtime.Log(buffer);
        snprintf(buffer, sizeof(buffer), "  SAH Cost:  %.2f", metrics.SAHCost);
        runtime.Log(buffer);
        snprintf(buffer, sizeof(buffer), "  Balance:   %.2f", metrics.BalanceFactor);
        runtime.Log(buffer);
    }

    Raytracer raytracer(camera, bvh, lighting, Width, Height);

    std::vector<u32> framebuffer(Width * Height);

    bool showBVH   = false;
    bool prevBDown = false;

    auto lastTime		= std::chrono::steady_clock::now();
    s32 frameCount	    = 0;
    f64 timeAccumulator = 0.0;

    while (runtime.PollEvents())
    {
        auto currentTime = std::chrono::steady_clock::now();
        f32 deltaTime    = std::chrono::duration<f32>(currentTime - lastTime).count();
        lastTime	     = currentTime;

        f32 mouseDeltaX, mouseDeltaY;
        runtime.GetLookDelta(mouseDeltaX, mouseDeltaY);
        bool cameraChanged = mouseDeltaX != 0.0f || mouseDeltaY != 0.0f;

        camera.Yaw   += mouseDeltaX * LookSensitivity;
        camera.Pitch -= mouseDeltaY * LookSensitivity;
        camera.Pitch  = std::clamp(camera.Pitch, -89.0f, 89.0f);

        Vector3 movement = Vector3::Zero;
        if (runtime.IsKeyDown(Platform::Key::Forward))  movement += camera.Forward;
        if (runtime.IsKeyDown(Platform::Key::Backward)) movement -= camera.Forward;
        if (runtime.IsKeyDown(Platform::Key::Left))     movement -= camera.Right;
        if (runtime.IsKeyDown(Platform::Key::Right))    movement += camera.Right;
        if (runtime.IsKeyDown(Platform::Key::Up))       movement += Vector3::UnitY;
        if (runtime.IsKeyDown(Platform::Key::Down))     movement -= Vector3::UnitY;

        f32 speed = MoveSpeed * deltaTime;
        if (runtime.IsKeyDown(Platform::Key::Boost)) speed *= 4.0f;

        if (movement != Vector3::Zero)
        {
            camera.Position += movement.Normalized() * speed;
            cameraChanged    = true;
        }

        if (cameraChanged)
            camera.UpdateView();

        bool bDown = runtime.IsKeyDown(Platform::Key::ToggleBVH);
        if (bDown && !prevBDown) showBVH = !showBVH;
        prevBDown = bDown;

        raytracer.Render(framebuffer.data());
        runtime.Present(framebuffer.data(), showBVH ? &camera : nullptr, showBVH ? &bvh : nullptr);

        frameCount++;
        timeAccumulator += deltaTime;
        if (timeAccumulator >= 0.5)
        {
            f64 fps			  = frameCount / timeAccumulator;
            std::string title = "Ether | FPS: " + std::to_string((s32)fps);
            runtime.SetTitle(title.c_str());

            frameCount      = 0;
            timeAccumulator = 0.0;
        }
    }

    return 0;
}

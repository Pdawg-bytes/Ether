#include "Platform/Window.h"
#include "Renderer/Camera.h"
#include "Renderer/Raymarcher.h"
#include "Scene/BVH.h"
#include "Scene/SceneObject.h"
#include "Scene/CSGTree.h"
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

	std::shared_ptr<CSGTree> BuildPillarTemplate()
	{
		CSGTreeBuilder builder;

		s32 shaft = builder.AddBox(Vector3(0.0f, 1.5f, 0.0f), Quaternion::Identity, Vector3(0.4f, 1.5f, 0.4f));
		s32 cap   = builder.AddSphere(Vector3(0.0f, 3.0f, 0.0f), 0.55f);
		s32 body  = builder.Union(shaft, cap);

		s32 hole = builder.AddCylinder(Vector3(0.0f, 1.5f, 0.0f), Quaternion::Identity, 0.15f, 2.0f);
		s32 root = builder.Subtraction(body, hole);

		return builder.Build(root);
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

	std::shared_ptr<CSGTree> BuildGearTemplate()
	{
		constexpr s32 ToothCount = 8;

		CSGTreeBuilder builder;

		s32 ring = builder.AddTorus(Vector3::Zero, Quaternion::Identity, 1.0f, 0.22f);

		s32 root = ring;
		for (s32 i = 0; i < ToothCount; i++)
		{
			f32 angle		    = (Math::PI * 2.0f) * ((f32)i / (f32)ToothCount);
			Quaternion rotation = Quaternion::FromAxisAngle(Vector3::UnitY, angle);
			Vector3 position    = rotation.Rotate(Vector3(0.0f, 0.0f, 1.0f));

			s32 tooth = builder.AddBox(position, rotation, Vector3(0.15f, 0.15f, 0.35f));
			root	  = builder.Union(root, tooth);
		}

		s32 axleHole = builder.AddCylinder(Vector3::Zero, Quaternion::Identity, 0.3f, 1.0f);
		root		 = builder.Subtraction(root, axleHole);

		return builder.Build(root);
	}

	BVH BuildScene()
	{
		std::shared_ptr<CSGTree> pillarTemplate = BuildPillarTemplate();
		std::shared_ptr<CSGTree> blobTemplate   = BuildBlobTemplate();
		std::shared_ptr<CSGTree> gearTemplate   = BuildGearTemplate();

		std::vector<SceneObject> objects;

		objects.push_back(SceneObject::CreatePlane(Vector3::UnitY, 0.0f));

		constexpr s32 PillarCount  = 6;
		constexpr f32 PillarRadius = 6.0f;

		for (s32 i = 0; i < PillarCount; i++)
		{
			f32 angle = (Math::PI * 2.0f) * ((f32)i / (f32)PillarCount);
			Vector3 position(std::sin(angle) * PillarRadius, 0.0f, std::cos(angle) * PillarRadius);

			f32 heightScale = 0.8f + 0.4f * ((f32)i / (f32)PillarCount);
			objects.push_back(SceneObject::CreateFromTemplate(pillarTemplate, position, Quaternion::Identity, Vector3(1.0f, heightScale, 1.0f)));
		}

		objects.push_back(SceneObject::CreateFromTemplate(blobTemplate, Vector3(0.0f, 1.6f, 0.0f)));
		objects.push_back(SceneObject::CreateFromTemplate(blobTemplate, Vector3(2.5f, 1.2f, -2.0f), Quaternion::Identity, Vector3(0.7f, 0.5f, 0.7f)));
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

	BVH BuildCornellBox()
	{
		std::vector<SceneObject> objects;

		objects.push_back(SceneObject::CreatePlane(Vector3::UnitY,  1.0f));
		objects.push_back(SceneObject::CreatePlane(-Vector3::UnitY, 3.0f));
		objects.push_back(SceneObject::CreatePlane(Vector3::UnitX,  2.0f));
		objects.push_back(SceneObject::CreatePlane(-Vector3::UnitX, 2.0f));
		objects.push_back(SceneObject::CreatePlane(-Vector3::UnitZ, 4.0f));
		objects.push_back(SceneObject::CreatePlane(Vector3::UnitZ,  3.5f));

		objects.push_back(SceneObject::CreateBox(
			Vector3(-0.80f, 0.0f, 2.8f),
			Quaternion::FromAxisAngle(Vector3::UnitY, Math::PI / 4.0f),
			Vector3(0.5f, 1.0f, 0.5f)
		));

		objects.push_back(SceneObject::CreateSphere(Vector3(0.75f, -0.35f, 3.15f), 0.55f));

		objects.push_back(SceneObject::CreateBox(
			Vector3(0.0f, 3.0f, 2.2f),
			Quaternion::Identity,
			Vector3(0.5f, 0.01f, 0.5f)
		));

		BVH bvh;
		bvh.Build(std::move(objects));
		return bvh;
	}
}


s32 main()
{
	Window window(Width, Height, L"Ether");
	Camera camera(Vector3(0.0f, 1.0f, -2.0f), Width, Height);
	BVH bvh = BuildScene();
	Raymarcher raymarcher(camera, bvh, Width, Height);

	std::vector<u32> framebuffer(Width * Height);

	bool showBVH   = true;
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
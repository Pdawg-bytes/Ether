#include "Platform/Window.h"
#include "Renderer/Camera.h"
#include "Renderer/Raymarcher.h"
#include "Scene/BVH.h"
#include "Scene/SceneObject.h"
#include "Math/MathUtil.h"

#include <algorithm>
#include <chrono>
#include <string>
#include <vector>

namespace
{
	constexpr u32 Width			   = 480;
	constexpr u32 Height		   = 270;
	constexpr f32 MoveSpeed		   = 3.5f;
	constexpr f32 MouseSensitivity = 0.15f;

	BVH BuildScene()
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
			camera.Position += movement.Normalized() * speed;

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
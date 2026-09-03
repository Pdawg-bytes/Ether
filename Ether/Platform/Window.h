#pragma once

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#include <GL/gl.h>

class Window
{
public:
	Window(u32 framebufferWidth, u32 framebufferHeight, const wchar_t* title);
	~Window();

	bool PollEvents();
	void Present(const u32* framebuffer, const class Camera* camera = nullptr, const class BVH* bvh = nullptr);

	bool IsKeyDown(s32 virtualKeyCode) const;
	void GetMouseDelta(f32& deltaX, f32& deltaY);

	void SetTitle(const wchar_t* title);

private:
	static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

	HWND _hwnd       = nullptr;
	HDC _hdc	     = nullptr;
	HGLRC _glContext = nullptr;
	GLuint _texture  = 0;

	u32 _framebufferWidth;
	u32 _framebufferHeight;

	bool _running = true;

	POINT _windowCenter{};
	bool _hasFocus = false;
};
#include "Window.h"

#include <cstring>
#include "../Renderer/Camera.h"
#include "../Scene/BVH.h"
#include "../Math/Vector3.h"
#include "../Math/MathUtil.h"
#include <algorithm>
#include <cmath>

namespace
{
	// Column-major, matches GL's glLoadMatrixf layout.
	void BuildPerspectiveMatrix(f32 fovYDegrees, f32 aspect, f32 nearPlane, f32 farPlane, f32 (&outMatrix)[16])
	{
		f32 f = 1.0f / std::tan(Math::DegToRad(fovYDegrees) * 0.5f);

		std::memset(outMatrix, 0, sizeof(outMatrix));
		outMatrix[0]  = f / aspect;
		outMatrix[5]  = f;
		outMatrix[10] = (farPlane + nearPlane) / (nearPlane - farPlane);
		outMatrix[11] = -1.0f;
		outMatrix[14] = (2.0f * farPlane * nearPlane) / (nearPlane - farPlane);
	}

	// Builds a view matrix from the camera's basis; GL camera space looks down -Z.
	void BuildViewMatrix(const Camera& camera, f32 (&outMatrix)[16])
	{
		const Vector3& right   = camera.Right;
		const Vector3& up      = camera.Up;
		const Vector3& forward = camera.Forward;
		const Vector3& pos     = camera.Position;

		outMatrix[0] = right.X;    outMatrix[4] = right.Y;    outMatrix[8]  = right.Z;    outMatrix[12] = -right.Dot(pos);
		outMatrix[1] = up.X;       outMatrix[5] = up.Y;       outMatrix[9]  = up.Z;       outMatrix[13] = -up.Dot(pos);
		outMatrix[2] = -forward.X; outMatrix[6] = -forward.Y; outMatrix[10] = -forward.Z; outMatrix[14] = forward.Dot(pos);
		outMatrix[3] = 0.0f;       outMatrix[7] = 0.0f;       outMatrix[11] = 0.0f;       outMatrix[15] = 1.0f;
	}
}

Window::Window(u32 framebufferWidth, u32 framebufferHeight, const wchar_t* title)
	: _framebufferWidth(framebufferWidth), _framebufferHeight(framebufferHeight)
{
	WNDCLASSW windowClass{};
	windowClass.lpfnWndProc   = WndProc;
	windowClass.hInstance	  = GetModuleHandleW(nullptr);
	windowClass.lpszClassName = L"EtherWindowClass";
	windowClass.hCursor		  = LoadCursorW(nullptr, IDC_ARROW);
	RegisterClassW(&windowClass);

	u32 windowWidth  = framebufferWidth  * 2;
	u32 windowHeight = framebufferHeight * 2;

	RECT rect{ 0, 0, (LONG)windowWidth, (LONG)windowHeight };
	AdjustWindowRect(&rect, WS_OVERLAPPEDWINDOW, FALSE);

	_hwnd = CreateWindowExW(
		0, 
		windowClass.lpszClassName, 
		title, 
		WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT,
		CW_USEDEFAULT, 
		rect.right - rect.left, rect.bottom - rect.top,
		nullptr, 
		nullptr, 
		windowClass.hInstance, 
		nullptr
	);

	SetWindowLongPtrW(_hwnd, GWLP_USERDATA, (LONG_PTR)this);

	_hdc = GetDC(_hwnd);

	PIXELFORMATDESCRIPTOR pfd{};
	pfd.nSize      = sizeof(pfd);
	pfd.nVersion   = 1;
	pfd.dwFlags    = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
	pfd.iPixelType = PFD_TYPE_RGBA;
	pfd.cColorBits = 32;
	pfd.cDepthBits = 24;

	s32 pf = ChoosePixelFormat(_hdc, &pfd);
	SetPixelFormat(_hdc, pf, &pfd);

	_glContext = wglCreateContext(_hdc);
	wglMakeCurrent(_hdc, _glContext);

	using WglSwapIntervalProc        = BOOL(WINAPI*)(int);
	WglSwapIntervalProc swapInterval = reinterpret_cast<WglSwapIntervalProc>(wglGetProcAddress("wglSwapIntervalEXT"));
	if (swapInterval) swapInterval(0);

	glGenTextures(1, &_texture);
	glBindTexture(GL_TEXTURE_2D, _texture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, (GLsizei)framebufferWidth, (GLsizei)framebufferHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

	// Compute before ShowWindow, since it can synchronously trigger WM_SETFOCUS which uses this value.
	RECT clientRect;
	GetClientRect(_hwnd, &clientRect);
	POINT topLeft{ clientRect.left, clientRect.top };
	ClientToScreen(_hwnd, &topLeft);
	_windowCenter.x = topLeft.x + (clientRect.right - clientRect.left) / 2;
	_windowCenter.y = topLeft.y + (clientRect.bottom - clientRect.top) / 2;

	ShowWindow(_hwnd, SW_SHOW);
}

Window::~Window()
{
	ShowCursor(TRUE);
	if (_texture)   { glDeleteTextures(1, &_texture); _texture = 0; }
	if (_glContext) { wglMakeCurrent(nullptr, nullptr); wglDeleteContext(_glContext); _glContext = nullptr; }
	if (_hdc)	    { ReleaseDC(_hwnd, _hdc); _hdc = nullptr; }
	if (_hwnd)		DestroyWindow(_hwnd);
}

LRESULT CALLBACK Window::WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	Window* window = (Window*)GetWindowLongPtrW(hwnd, GWLP_USERDATA);

	switch (msg)
	{
		case WM_SYSCOMMAND:
			if ((wParam & 0xFFF0) == SC_CLOSE)
			{
				if (window) window->_running = false;
				PostQuitMessage(0);
				return 0;
			}
			break;
		case WM_KEYDOWN:
		case WM_SYSKEYDOWN:
			if (window && window->_hasFocus && wParam == VK_ESCAPE)
			{
				SetFocus(nullptr);
				if (window->_hasFocus)
				{
					window->_hasFocus = false;
					ClipCursor(nullptr);
					ReleaseCapture();
					ShowCursor(TRUE);
				}
				return 0;
			}
			break;
		case WM_LBUTTONDOWN:
		case WM_RBUTTONDOWN:
		case WM_MBUTTONDOWN:
			if (window && !window->_hasFocus)
			{
				SetFocus(hwnd);
			}
			break;
		case WM_CLOSE:
		case WM_DESTROY:
			if (window) window->_running = false;
			PostQuitMessage(0);
			return 0;
		case WM_SETFOCUS:
			if (window)
			{
				window->_hasFocus = true;
				ShowCursor(FALSE);

				RECT client;
				GetClientRect(window->_hwnd, &client);
				POINT tl{ client.left, client.top };
				ClientToScreen(window->_hwnd, &tl);
				RECT clip{ tl.x, tl.y, tl.x + (client.right - client.left), tl.y + (client.bottom - client.top) };
				ClipCursor(&clip);
				SetCursorPos(window->_windowCenter.x, window->_windowCenter.y);
				SetCapture(window->_hwnd);
			}
			return 0;
		case WM_KILLFOCUS:
			if (window)
			{
				window->_hasFocus = false;
				ClipCursor(NULL);
				ReleaseCapture();
				ShowCursor(TRUE);
			}
			return 0;
	}

	return DefWindowProcW(hwnd, msg, wParam, lParam);
}

bool Window::PollEvents()
{
	MSG msg;
	while (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE))
	{
		TranslateMessage(&msg);
		DispatchMessageW(&msg);
	}

	return _running;
}

void Window::Present(const u32* framebuffer, const Camera* camera, const BVH* bvh)
{
	if (!_hdc || !_texture) return;

	glBindTexture(GL_TEXTURE_2D, _texture);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, (GLsizei)_framebufferWidth, (GLsizei)_framebufferHeight, GL_RGBA, GL_UNSIGNED_BYTE, framebuffer);

	glViewport(0, 0, (GLsizei)(_framebufferWidth * 2), (GLsizei)(_framebufferHeight * 2));

	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);

	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glLoadIdentity();

	glColor3f(1.0f, 1.0f, 1.0f);
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, _texture);

	glBegin(GL_QUADS);
		glTexCoord2f(0.0f, 1.0f); glVertex2f(-1.0f, -1.0f);
		glTexCoord2f(1.0f, 1.0f); glVertex2f(1.0f, -1.0f);
		glTexCoord2f(1.0f, 0.0f); glVertex2f(1.0f, 1.0f);
		glTexCoord2f(0.0f, 0.0f); glVertex2f(-1.0f, 1.0f);
	glEnd();

	glDisable(GL_TEXTURE_2D);

	if (camera && bvh)
	{
		f32 aspect = (f32)_framebufferWidth / (f32)_framebufferHeight;
		f32 projectionMatrix[16];
		f32 viewMatrix[16];
		BuildPerspectiveMatrix(camera->GetFOV(), aspect, 0.01f, 1000.0f, projectionMatrix);
		BuildViewMatrix(*camera, viewMatrix);

		glMatrixMode(GL_PROJECTION);
		glPushMatrix();
		glLoadMatrixf(projectionMatrix);
		glMatrixMode(GL_MODELVIEW);
		glPushMatrix();
		glLoadMatrixf(viewMatrix);

		glColor3f(0.0f, 1.0f, 0.0f);
		glLineWidth(1.0f);
		glBegin(GL_LINES);
		const s32 nodeCount = bvh->GetNodeCount();
		for (s32 i = 0; i < nodeCount; ++i)
		{
			const BVHNode& node = bvh->GetNode(i);
			Vector3 mins = node.Bounds.Min;
			Vector3 maxs = node.Bounds.Max;
			Vector3 corners[8] = {
				Vector3(mins.X, mins.Y, mins.Z),
				Vector3(maxs.X, mins.Y, mins.Z),
				Vector3(maxs.X, maxs.Y, mins.Z),
				Vector3(mins.X, maxs.Y, mins.Z),
				Vector3(mins.X, mins.Y, maxs.Z),
				Vector3(maxs.X, mins.Y, maxs.Z),
				Vector3(maxs.X, maxs.Y, maxs.Z),
				Vector3(mins.X, maxs.Y, maxs.Z),
			};

			s32 edges[12][2] = {
				{0,1},{1,2},{2,3},{3,0},
				{4,5},{5,6},{6,7},{7,4},
				{0,4},{1,5},{2,6},{3,7}
			};

			for (s32 e = 0; e < 12; ++e)
			{
				const Vector3& a = corners[edges[e][0]];
				const Vector3& b = corners[edges[e][1]];
				glVertex3f(a.X, a.Y, a.Z);
				glVertex3f(b.X, b.Y, b.Z);
			}
		}
		glEnd();

		glPopMatrix();
		glMatrixMode(GL_PROJECTION);
		glPopMatrix();
	}

	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();
	glMatrixMode(GL_PROJECTION);
	glPopMatrix();

	SwapBuffers(_hdc);
}

bool Window::IsKeyDown(s32 virtualKeyCode) const
{
	if (!_hasFocus) return false;
	return (GetAsyncKeyState(virtualKeyCode) & 0x8000) != 0;
}

void Window::GetMouseDelta(f32& deltaX, f32& deltaY)
{
	if (!_hasFocus)
	{
		deltaX = 0.0f;
		deltaY = 0.0f;
		return;
	}

	POINT cursorPos;
	GetCursorPos(&cursorPos);

	deltaX = (f32)(cursorPos.x - _windowCenter.x);
	deltaY = (f32)(cursorPos.y - _windowCenter.y);

	SetCursorPos(_windowCenter.x, _windowCenter.y);
}

void Window::SetTitle(const wchar_t* title)
{
	SetWindowTextW(_hwnd, title);
}
#include "Runtime.h"

#include "Window.h"

#include <string>

namespace
{
    struct WindowsRuntime
    {
        explicit WindowsRuntime(u32 width, u32 height, const char* title)
            : window(width, height, ToWide(title).c_str())
        {
        }

        static std::wstring ToWide(const char* text)
        {
            std::wstring result;
            while (*text)
                result.push_back((wchar_t)*text++);
            return result;
        }

        Window window;
    };
}

namespace Platform
{
    Runtime::Runtime(u32 width, u32 height, const char* title)
        : _implementation(new WindowsRuntime(width, height, title))
    {
    }

    Runtime::~Runtime()
    {
        delete static_cast<WindowsRuntime*>(_implementation);
    }

    bool Runtime::PollEvents()
    {
        return static_cast<WindowsRuntime*>(_implementation)->window.PollEvents();
    }

    bool Runtime::IsKeyDown(Key key) const
    {
        WindowsRuntime* runtime = static_cast<WindowsRuntime*>(_implementation);
        switch (key)
        {
            case Key::Forward:    return runtime->window.IsKeyDown('W');
            case Key::Backward:   return runtime->window.IsKeyDown('S');
            case Key::Left:       return runtime->window.IsKeyDown('A');
            case Key::Right:      return runtime->window.IsKeyDown('D');
            case Key::Up:         return runtime->window.IsKeyDown(VK_SPACE);
            case Key::Down:       return runtime->window.IsKeyDown(VK_SHIFT);
            case Key::Boost:      return runtime->window.IsKeyDown(VK_CONTROL);
            case Key::ToggleBVH:  return runtime->window.IsKeyDown('B');
        }
        return false;
    }

    void Runtime::GetLookDelta(f32& outX, f32& outY)
    {
        static_cast<WindowsRuntime*>(_implementation)->window.GetMouseDelta(outX, outY);
    }

    void Runtime::Present(const u32* framebuffer, const Camera* camera, const BVH* bvh)
    {
        static_cast<WindowsRuntime*>(_implementation)->window.Present(framebuffer, camera, bvh);
    }

    void Runtime::SetTitle(const char* title)
    {
        WindowsRuntime* runtime = static_cast<WindowsRuntime*>(_implementation);
        std::wstring wideTitle  = WindowsRuntime::ToWide(title);
        
        runtime->window.SetTitle(wideTitle.c_str());
    }
}
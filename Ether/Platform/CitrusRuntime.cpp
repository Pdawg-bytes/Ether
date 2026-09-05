#include "Runtime.h"

#include "../Renderer/Camera.h"
#include "../Scene/BVH.h"

#include <3ds.h>

namespace
{
    constexpr u32 ScreenWidth     = 400;
    constexpr u32 ScreenHeight    = 240;
    constexpr u32 BytesPerPixel   = 3;
    constexpr s16 CircleDeadZone  = 10;
    constexpr f32 CircleLookScale = 12.0f / 156.0f;

    struct CitrusRuntime
    {
        CitrusRuntime(u32 width, u32 height)
            : renderWidth(width), renderHeight(height), scale(ScreenHeight / height)
        {
            gfxInitDefault();
            gfxSetDoubleBuffering(GFX_TOP, false);
        }

        ~CitrusRuntime()
        {
            gfxExit();
        }

        u32 renderWidth;
        u32 renderHeight;
        u32 scale;
        u32 heldKeys = 0;
        circlePosition circlePad{};
    };

    void Blit(const CitrusRuntime& runtime, const u32* source, u8* destination)
    {
        u32 imageWidth  = runtime.renderWidth * runtime.scale;
        u32 imageHeight = runtime.renderHeight * runtime.scale;
        u32 leftOffset  = (ScreenWidth - imageWidth) / 2;
        u32 topOffset   = (ScreenHeight - imageHeight) / 2;

        for (u32 y = 0; y < runtime.renderHeight; y++)
        {
            for (u32 x = 0; x < runtime.renderWidth; x++)
            {
                const u32 color = source[y * runtime.renderWidth + x];
                const u8 red    = (u8)(color & 0xff);
                const u8 green  = (u8)((color >> 8) & 0xff);
                const u8 blue   = (u8)((color >> 16) & 0xff);

                for (u32 yScale = 0; yScale < runtime.scale; yScale++)
                {
                    u32 screenY = topOffset + y * runtime.scale + yScale;
                    for (u32 xScale = 0; xScale < runtime.scale; xScale++)
                    {
                        u32 screenX  = leftOffset + x * runtime.scale + xScale;
                        usize offset = ((usize)screenX * ScreenHeight + (ScreenHeight - 1 - screenY)) * BytesPerPixel;
                        destination[offset + 0] = blue;
                        destination[offset + 1] = green;
                        destination[offset + 2] = red;
                    }
                }
            }
        }
    }
}

namespace Platform
{
    Runtime::Runtime(u32 width, u32 height, const char*)
        : _implementation(new CitrusRuntime(width, height))
    {
    }

    Runtime::~Runtime()
    {
        delete static_cast<CitrusRuntime*>(_implementation);
    }

    bool Runtime::PollEvents()
    {
        if (!aptMainLoop())
            return false;

        hidScanInput();
        CitrusRuntime* runtime = static_cast<CitrusRuntime*>(_implementation);
        runtime->heldKeys      = hidKeysHeld();

        hidCircleRead(&runtime->circlePad);
        return (hidKeysDown() & KEY_START) == 0;
    }

    bool Runtime::IsKeyDown(Key key) const
    {
        const CitrusRuntime* runtime = static_cast<const CitrusRuntime*>(_implementation);
        const bool circlePadActive = runtime->circlePad.dx > CircleDeadZone ||
            runtime->circlePad.dx < -CircleDeadZone ||
            runtime->circlePad.dy > CircleDeadZone  ||
            runtime->circlePad.dy < -CircleDeadZone;

        u32 button = 0;
        switch (key)
        {
            case Key::Forward:    button = KEY_UP;    break;
            case Key::Backward:   button = KEY_DOWN;  break;
            case Key::Left:       button = KEY_LEFT;  break;
            case Key::Right:      button = KEY_RIGHT; break;
            case Key::Up:         button = KEY_X;     break;
            case Key::Down:       button = KEY_B;     break;
            case Key::Boost:      button = KEY_R;     break;
            case Key::ToggleBVH:  button = KEY_Y;     break;
        }

        const bool dpadKey = key == Key::Forward || key == Key::Backward || key == Key::Left || key == Key::Right;

        return (!dpadKey || !circlePadActive) && (runtime->heldKeys & button) != 0;
    }

    void Runtime::GetLookDelta(f32& outX, f32& outY)
    {
        const CitrusRuntime* runtime = static_cast<const CitrusRuntime*>(_implementation);

        outX = runtime->circlePad.dx > CircleDeadZone || runtime->circlePad.dx < -CircleDeadZone
            ? runtime->circlePad.dx * CircleLookScale
            : 0.0f;

        outY = runtime->circlePad.dy > CircleDeadZone || runtime->circlePad.dy < -CircleDeadZone
            ? -runtime->circlePad.dy * CircleLookScale
            : 0.0f;
    }

    void Runtime::Present(const u32* framebuffer, const Camera*, const BVH*)
    {
        CitrusRuntime* runtime = static_cast<CitrusRuntime*>(_implementation);
        u8* screen             = gfxGetFramebuffer(GFX_TOP, GFX_LEFT, nullptr, nullptr);

        Blit(*runtime, framebuffer, screen);
        gfxFlushBuffers();
        gfxSwapBuffers();
    }

    void Runtime::SetTitle(const char*)
    {
    }
}
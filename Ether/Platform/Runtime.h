#pragma once

#include "../GlobalTypes.h"

class BVH;
class Camera;

namespace Platform
{
    enum class Key
    {
        Forward,
        Backward,
        Left,
        Right,
        Up,
        Down,
        Boost,
        ToggleBVH,
    };

    class Runtime
    {
    public:
        Runtime(u32 width, u32 height, const char* title);
        ~Runtime();

        bool PollEvents();
        bool IsKeyDown(Key key) const;
        void GetLookDelta(f32& outX, f32& outY);
        void Present(const u32* framebuffer, const Camera* camera, const BVH* bvh);
        void SetTitle(const char* title);

    private:
        void* _implementation;
    };
}
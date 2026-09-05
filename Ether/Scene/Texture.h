#pragma once

#include "../Math/Vector3.h"
#include "../Math/Vector2.h"

#include <memory>
#include <vector>

class Texture
{
public:
    Texture(u32 width, u32 height);

    Vector3 Sample(const Vector2& uv) const;
    f32 SampleScalar(const Vector2& uv) const { return Sample(uv).X; }

    void SetPixel(u32 x, u32 y, const Vector3& color);

    u32 Width()  const { return _width; }
    u32 Height() const { return _height; }

    static std::shared_ptr<Texture> CreateCheckerboard(u32 width, u32 height, u32 checkSize, const Vector3& colorA, const Vector3& colorB);

private:
    u32 _width;
    u32 _height;
    std::vector<Vector3> _pixels;
};
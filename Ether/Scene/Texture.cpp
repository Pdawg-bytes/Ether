#include "Texture.h"

#include "../Platform/FileLoader.h"

#define STB_IMAGE_IMPLEMENTATION
#include "../../Vendor/stb_image.h"

#include <cmath>
#include <limits>

Texture::Texture(u32 width, u32 height)
    : _width(width), _height(height), _pixels(width * height)
{
}

Vector3 Texture::Sample(const Vector2& uv) const
{
    f32 fx = uv.X - std::floor(uv.X);
    f32 fy = uv.Y - std::floor(uv.Y);

    f32 px = fx * (f32)_width - 0.5f;
    f32 py = fy * (f32)_height - 0.5f;

    s32 x0 = (s32)std::floor(px);
    s32 y0 = (s32)std::floor(py);
    f32 tx = px - (f32)x0;
    f32 ty = py - (f32)y0;

    auto Wrap = [](s32 value, s32 size)
    {
        s32 wrapped = value % size;
        return wrapped < 0 ? wrapped + size : wrapped;
    };

    s32 x0w = Wrap(x0, (s32)_width);
    s32 x1w = Wrap(x0 + 1, (s32)_width);
    s32 y0w = Wrap(y0, (s32)_height);
    s32 y1w = Wrap(y0 + 1, (s32)_height);

    const Vector3& c00 = _pixels[y0w * _width + x0w];
    const Vector3& c10 = _pixels[y0w * _width + x1w];
    const Vector3& c01 = _pixels[y1w * _width + x0w];
    const Vector3& c11 = _pixels[y1w * _width + x1w];

    Vector3 top    = c00 * (1.0f - tx) + c10 * tx;
    Vector3 bottom = c01 * (1.0f - tx) + c11 * tx;

    return top * (1.0f - ty) + bottom * ty;
}

void Texture::SetPixel(u32 x, u32 y, const Vector3& color)
{
    _pixels[y * _width + x] = color;
}

std::shared_ptr<Texture> Texture::CreateCheckerboard(u32 width, u32 height, u32 checkSize, const Vector3& colorA, const Vector3& colorB)
{
    auto texture = std::make_shared<Texture>(width, height);
    u32 shift    = static_cast<u32>(std::log2(checkSize));

    for (u32 y = 0; y < height; y++)
    {
        u32 yCheck = y >> shift;

        for (u32 x = 0; x < width; x++)
        {
            bool isA = !(((x >> shift) ^ yCheck) & 1);
            texture->SetPixel(x, y, isA ? colorA : colorB);
        }
    }

    return texture;
}

std::shared_ptr<Texture> Texture::LoadFromFile(const std::string& relativeFilePath)
{
    std::vector<u8> fileData = FileLoader::LoadFile(relativeFilePath);
    if (fileData.empty() || fileData.size() > static_cast<size_t>((std::numeric_limits<int>::max)()))
        return nullptr;

    int width    = 0;
    int height   = 0;
    int channels = 0;

    stbi_set_flip_vertically_on_load(1);
    stbi_uc* pixels = stbi_load_from_memory(fileData.data(), static_cast<int>(fileData.size()), &width, &height, &channels, 4);
    stbi_set_flip_vertically_on_load(0);

    if (!pixels || width <= 0 || height <= 0)
        return nullptr;

    auto texture = std::make_shared<Texture>(static_cast<u32>(width), static_cast<u32>(height));
    for (int y = 0; y < height; ++y)
    {
        for (int x = 0; x < width; ++x)
        {
            const stbi_uc* pixel = pixels + (y * width + x) * 4;
            texture->SetPixel(static_cast<u32>(x), static_cast<u32>(y), Vector3(
                pixel[0] / 255.0f,
                pixel[1] / 255.0f,
                pixel[2] / 255.0f)
            );
        }
    }

    stbi_image_free(pixels);
    return texture;
}
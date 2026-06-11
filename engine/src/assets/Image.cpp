// VyroEngine — Image asset implementation
#include "vyro/assets/Image.hpp"

#define STB_IMAGE_IMPLEMENTATION
#define STBI_NO_THREAD_LOCALS
#include <stb_image.h>

#include <filesystem>
#include <string>

namespace vyro {

std::expected<Image, ImageError> load_image(std::string_view path)
{
    const std::string p(path);
    if (!std::filesystem::exists(p)) {
        return std::unexpected(ImageError::FileNotFound);
    }

    int w = 0;
    int h = 0;
    int channels = 0;
    stbi_uc* data = stbi_load(p.c_str(), &w, &h, &channels, STBI_rgb_alpha);
    if (data == nullptr) {
        return std::unexpected(ImageError::DecodeFailed);
    }

    Image img;
    img.width = static_cast<u32>(w);
    img.height = static_cast<u32>(h);
    img.pixels.assign(data, data + static_cast<usize>(w) * static_cast<usize>(h) * 4);
    stbi_image_free(data);
    return img;
}

Image make_checkerboard(u32 size, u32 cell, u8 r1, u8 g1, u8 b1, u8 r2, u8 g2, u8 b2)
{
    Image img;
    img.width = size;
    img.height = size;
    img.pixels.resize(static_cast<usize>(size) * size * 4);
    for (u32 y = 0; y < size; ++y) {
        for (u32 x = 0; x < size; ++x) {
            const bool a = ((x / cell) + (y / cell)) % 2 == 0;
            u8* px = img.pixels.data() + (static_cast<usize>(y) * size + x) * 4;
            px[0] = a ? r1 : r2;
            px[1] = a ? g1 : g2;
            px[2] = a ? b1 : b2;
            px[3] = 255;
        }
    }
    return img;
}

Image make_solid(u8 r, u8 g, u8 b, u8 a)
{
    Image img;
    img.width = 1;
    img.height = 1;
    img.pixels = {r, g, b, a};
    return img;
}

} // namespace vyro

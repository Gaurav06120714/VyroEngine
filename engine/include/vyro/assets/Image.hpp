// VyroEngine — Image asset (V2.5)
// CPU-side RGBA8 image: loaded from PNG/JPG via stb_image or generated
// procedurally (checkerboard). Uploaded to the GPU through the RHI's
// create_texture.
#pragma once

#include "vyro/core/Types.hpp"

#include <expected>
#include <string_view>
#include <vector>

namespace vyro {

enum class ImageError {
    FileNotFound,
    DecodeFailed,
};

struct Image {
    u32 width = 0;
    u32 height = 0;
    std::vector<u8> pixels; // tightly packed RGBA8

    [[nodiscard]] usize byte_size() const { return pixels.size(); }
    [[nodiscard]] bool empty() const { return pixels.empty(); }

    // Pixel accessor (RGBA, no bounds checking beyond debug assertions).
    [[nodiscard]] const u8* at(u32 x, u32 y) const
    {
        return pixels.data() + (static_cast<usize>(y) * width + x) * 4;
    }
};

// Decode a PNG/JPG/BMP/TGA file into RGBA8.
[[nodiscard]] std::expected<Image, ImageError> load_image(std::string_view path);

// Procedural checkerboard (fallback/default texture).
[[nodiscard]] Image make_checkerboard(u32 size, u32 cell,
                                      u8 r1 = 200, u8 g1 = 200, u8 b1 = 205,
                                      u8 r2 = 90, u8 g2 = 90, u8 b2 = 100);

// 1x1 solid color (neutral texture so untextured materials render unchanged).
[[nodiscard]] Image make_solid(u8 r, u8 g, u8 b, u8 a = 255);

} // namespace vyro

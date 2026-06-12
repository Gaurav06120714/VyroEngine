// VyroEngine — Text geometry (V3.4)
// Builds on-screen text as triangle quads from a built-in 5x7 bitmap font —
// one small quad per lit font pixel, no texture or font file required.
// Coordinates are NDC with (x right, y up); (x, y) is the text's top-left.
// Supports A-Z (lowercase folded), 0-9, space, '-', '!', '.', ':' and '\x03'
// (a heart, for health displays). Unknown characters render as space.
#pragma once

#include "vyro/assets/Mesh.hpp"
#include "vyro/core/Types.hpp"

#include <string_view>
#include <vector>

namespace vyro::text {

// Width of one character cell relative to `size` (5px glyph + 1px spacing).
inline constexpr f32 kAdvance = 6.0f / 7.0f;

// Append quads for `str` into `out`. `size` is the glyph height in NDC units;
// `aspect` (viewport width/height) keeps pixels square.
void build(std::string_view str, f32 x, f32 y, f32 size, f32 aspect, Vec3 color,
           std::vector<Vertex3D>& out);

// Total NDC width the string will occupy (for centering).
[[nodiscard]] f32 measure(std::string_view str, f32 size, f32 aspect);

} // namespace vyro::text

// VyroEngine — Hardware instancing layout (V6.1)
// The contract between the per-instance buffer the game uploads and the
// instanced draw path / shader. Each instance is one column-major Mat4 model
// matrix; the shader reads it as four vec4 attributes (locations 4-7), so
// column c maps to attribute 4+c. Kept here (headless, tested) so the layout
// is documented and verified without a GL context.
#pragma once

#include "vyro/core/Types.hpp"
#include "vyro/math/Mat4.hpp"
#include "vyro/math/Vec.hpp"

namespace vyro::instancing {

// One instance = one Mat4 (column-major, tightly packed).
inline constexpr usize kInstanceStride = sizeof(Mat4);

// Bytes needed for `count` instances.
[[nodiscard]] constexpr usize buffer_bytes(usize count) { return count * kInstanceStride; }

// Column `c` of the matrix, as fed to instance attribute (4 + c).
[[nodiscard]] inline Vec4 column(const Mat4& m, usize c)
{
    return Vec4{m.data[c * 4 + 0], m.data[c * 4 + 1], m.data[c * 4 + 2], m.data[c * 4 + 3]};
}

} // namespace vyro::instancing

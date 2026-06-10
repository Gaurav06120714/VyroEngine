// VyroEngine — Animation blending
// Phase 6.2: a pose is a per-bone list of transforms (translations here).
// Blending linearly interpolates two poses by a weight, the core operation of
// blend trees (e.g. walk/run by speed).
#pragma once

#include "vyro/core/Types.hpp"
#include "vyro/math/Vec.hpp"

#include <vector>

namespace vyro {

using Pose = std::vector<Vec3>;

[[nodiscard]] inline Vec3 lerp(Vec3 a, Vec3 b, f32 t) { return a + (b - a) * t; }

// Blend pose b into pose a by `weight` (0 = a, 1 = b). Bones beyond the shorter
// pose are taken from the longer one unchanged.
[[nodiscard]] inline Pose blend(const Pose& a, const Pose& b, f32 weight)
{
    const f32 w = weight < 0.0f ? 0.0f : (weight > 1.0f ? 1.0f : weight);
    const usize n = a.size() < b.size() ? a.size() : b.size();
    Pose out;
    out.reserve(a.size() > b.size() ? a.size() : b.size());
    for (usize i = 0; i < n; ++i) {
        out.push_back(lerp(a[i], b[i], w));
    }
    for (usize i = n; i < a.size(); ++i) {
        out.push_back(a[i]);
    }
    for (usize i = n; i < b.size(); ++i) {
        out.push_back(b[i]);
    }
    return out;
}

} // namespace vyro

// VyroEngine — Transform gizmos
// Phase 8.5: the math behind viewport manipulators. A gizmo operation applies
// a translate/rotate/scale delta to a transform, optionally snapped to a grid.
// The viewport draws handles; this model applies the edits.
#pragma once

#include "vyro/core/Types.hpp"
#include "vyro/math/Vec.hpp"

#include <cmath>

namespace vyro {

enum class GizmoMode : u8 {
    Translate,
    Rotate,
    Scale,
};

struct TransformComponent {
    Vec3 position{};
    Vec3 rotation{}; // euler radians
    Vec3 scale{1.0f, 1.0f, 1.0f};
};

namespace gizmo {

// Snap a value to the nearest multiple of `step` (step <= 0 disables).
[[nodiscard]] inline f32 snap(f32 value, f32 step)
{
    return step > 0.0f ? std::round(value / step) * step : value;
}

[[nodiscard]] inline Vec3 snap(Vec3 v, f32 step)
{
    return {snap(v.x, step), snap(v.y, step), snap(v.z, step)};
}

// Apply a gizmo drag delta to a transform.
inline void apply(TransformComponent& t, GizmoMode mode, Vec3 delta, f32 snap_step = 0.0f)
{
    switch (mode) {
        case GizmoMode::Translate:
            t.position = snap(t.position + delta, snap_step);
            break;
        case GizmoMode::Rotate:
            t.rotation = snap(t.rotation + delta, snap_step);
            break;
        case GizmoMode::Scale:
            t.scale = snap(t.scale + delta, snap_step);
            // Prevent degenerate/negative scale from drag overshoot.
            t.scale.x = t.scale.x < 0.01f ? 0.01f : t.scale.x;
            t.scale.y = t.scale.y < 0.01f ? 0.01f : t.scale.y;
            t.scale.z = t.scale.z < 0.01f ? 0.01f : t.scale.z;
            break;
    }
}

} // namespace gizmo

} // namespace vyro

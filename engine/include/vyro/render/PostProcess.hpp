// VyroEngine — Post-processing and shadows
// Phase 10.3/10.4: cascaded shadow-map split computation and an ordered
// post-processing stack with reference tone-mapping math.
#pragma once

#include "vyro/core/Types.hpp"
#include "vyro/math/Mat4.hpp"
#include "vyro/math/Vec.hpp"

#include <string>
#include <vector>

namespace vyro {

namespace shadows {

// Practical split scheme: blend between logarithmic and uniform splits.
// Returns `cascades` far-plane distances covering [near, far].
[[nodiscard]] std::vector<f32> cascade_splits(f32 near_z, f32 far_z, u32 cascades,
                                              f32 log_blend = 0.75f);

// Index of the cascade that covers `view_depth`.
[[nodiscard]] u32 select_cascade(const std::vector<f32>& splits, f32 view_depth);

// View-projection for a directional light shadow pass: an orthographic box of
// half-size `radius` centered on `center`, looking along `light_dir`, with the
// light placed `depth` back so the whole scene fits in front of it. Used to
// render the scene's depth from the sun's point of view into a shadow map.
[[nodiscard]] Mat4 light_view_projection(Vec3 light_dir, Vec3 center, f32 radius, f32 depth);

} // namespace shadows

namespace postfx {

// Reinhard tone mapping (HDR -> LDR).
[[nodiscard]] Vec3 tonemap_reinhard(Vec3 hdr);

// ACES filmic approximation (Narkowicz).
[[nodiscard]] Vec3 tonemap_aces(Vec3 hdr);

// Bloom threshold: keep only the component of color above `threshold`.
[[nodiscard]] Vec3 bloom_extract(Vec3 color, f32 threshold);

// Normalized 1D Gaussian weights, length 2*radius+1. sigma defaults to
// radius/2 when zero. The weights always sum to 1 (energy-preserving).
[[nodiscard]] std::vector<f32> gaussian_kernel(u32 radius, f32 sigma = 0.0f);

// Separable Gaussian blur of a row-major RGB image (width*height), edges
// clamped. Runs a horizontal then a vertical pass; result written in place.
void blur_separable(std::vector<Vec3>& image, u32 width, u32 height,
                    const std::vector<f32>& kernel);

// Full bloom: extract pixels above `threshold`, blur them by `radius`, add the
// glow back at `intensity`, then ACES tonemap to LDR. Reference (CPU) pipeline
// that mirrors the GPU pass; deterministic and unit-tested.
[[nodiscard]] std::vector<Vec3> apply_bloom(const std::vector<Vec3>& hdr, u32 width, u32 height,
                                            f32 threshold, u32 radius, f32 intensity);

} // namespace postfx

// Ordered, toggleable effect stack (10.4). The renderer runs enabled effects
// in order; this model owns ordering and enablement.
class PostProcessStack
{
public:
    struct Effect {
        std::string name;
        bool enabled = true;
    };

    void add(std::string name, bool enabled = true);
    bool set_enabled(std::string_view name, bool enabled);
    bool move_before(std::string_view name, std::string_view before);

    [[nodiscard]] std::vector<std::string> active_chain() const;
    [[nodiscard]] usize size() const { return m_effects.size(); }

private:
    std::vector<Effect> m_effects;
};

} // namespace vyro

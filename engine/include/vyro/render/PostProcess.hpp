// VyroEngine — Post-processing and shadows
// Phase 10.3/10.4: cascaded shadow-map split computation and an ordered
// post-processing stack with reference tone-mapping math.
#pragma once

#include "vyro/core/Types.hpp"
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

} // namespace shadows

namespace postfx {

// Reinhard tone mapping (HDR -> LDR).
[[nodiscard]] Vec3 tonemap_reinhard(Vec3 hdr);

// ACES filmic approximation (Narkowicz).
[[nodiscard]] Vec3 tonemap_aces(Vec3 hdr);

// Bloom threshold: keep only the component of color above `threshold`.
[[nodiscard]] Vec3 bloom_extract(Vec3 color, f32 threshold);

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

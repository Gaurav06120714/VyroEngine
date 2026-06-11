// VyroEngine — Physically based shading math
// Phase 10.2: the metallic-roughness BRDF terms (Cook-Torrance with GGX
// distribution, Smith geometry, Schlick Fresnel). These CPU-side reference
// implementations are mirrored by the GPU shaders and validated by tests.
#pragma once

#include "vyro/core/Types.hpp"
#include "vyro/math/Vec.hpp"

namespace vyro::pbr {

inline constexpr f32 kPi = 3.14159265358979323846f;

// GGX/Trowbridge-Reitz normal distribution.
[[nodiscard]] f32 distribution_ggx(f32 n_dot_h, f32 roughness);

// Smith-Schlick geometry (visibility) term.
[[nodiscard]] f32 geometry_smith(f32 n_dot_v, f32 n_dot_l, f32 roughness);

// Schlick Fresnel approximation. f0 is reflectance at normal incidence.
[[nodiscard]] Vec3 fresnel_schlick(f32 cos_theta, Vec3 f0);

// Base reflectance for a metallic-roughness material: dielectrics use 0.04,
// metals use their albedo.
[[nodiscard]] Vec3 base_reflectance(Vec3 albedo, f32 metallic);

// Full Cook-Torrance specular + Lambert diffuse for one light. All vectors
// must be normalized; returns outgoing radiance contribution.
[[nodiscard]] Vec3 shade(Vec3 normal, Vec3 view, Vec3 light, Vec3 albedo, f32 metallic,
                         f32 roughness, Vec3 light_color);

} // namespace vyro::pbr

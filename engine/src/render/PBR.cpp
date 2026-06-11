// VyroEngine — Physically based shading implementation
#include "vyro/render/PBR.hpp"

#include <algorithm>
#include <cmath>

namespace vyro::pbr {

f32 distribution_ggx(f32 n_dot_h, f32 roughness)
{
    const f32 a = roughness * roughness;
    const f32 a2 = a * a;
    const f32 d = n_dot_h * n_dot_h * (a2 - 1.0f) + 1.0f;
    return a2 / (kPi * d * d);
}

f32 geometry_smith(f32 n_dot_v, f32 n_dot_l, f32 roughness)
{
    const f32 r = roughness + 1.0f;
    const f32 k = (r * r) / 8.0f; // direct-lighting remap
    const f32 gv = n_dot_v / (n_dot_v * (1.0f - k) + k);
    const f32 gl = n_dot_l / (n_dot_l * (1.0f - k) + k);
    return gv * gl;
}

Vec3 fresnel_schlick(f32 cos_theta, Vec3 f0)
{
    const f32 t = std::pow(std::clamp(1.0f - cos_theta, 0.0f, 1.0f), 5.0f);
    return f0 + (Vec3{1, 1, 1} - f0) * t;
}

Vec3 base_reflectance(Vec3 albedo, f32 metallic)
{
    const Vec3 dielectric{0.04f, 0.04f, 0.04f};
    return dielectric * (1.0f - metallic) + albedo * metallic;
}

Vec3 shade(Vec3 normal, Vec3 view, Vec3 light, Vec3 albedo, f32 metallic, f32 roughness,
           Vec3 light_color)
{
    const Vec3 halfway = normalize(view + light);
    const f32 n_dot_v = std::max(dot(normal, view), 1e-4f);
    const f32 n_dot_l = std::max(dot(normal, light), 0.0f);
    const f32 n_dot_h = std::max(dot(normal, halfway), 0.0f);
    const f32 h_dot_v = std::max(dot(halfway, view), 0.0f);
    if (n_dot_l <= 0.0f) {
        return Vec3{};
    }

    const Vec3 f0 = base_reflectance(albedo, metallic);
    const f32 ndf = distribution_ggx(n_dot_h, roughness);
    const f32 geo = geometry_smith(n_dot_v, n_dot_l, roughness);
    const Vec3 fresnel = fresnel_schlick(h_dot_v, f0);

    const Vec3 specular = fresnel * (ndf * geo / (4.0f * n_dot_v * n_dot_l));

    // Energy conservation: diffuse is what is not specularly reflected, and
    // metals have no diffuse term.
    const Vec3 kd = (Vec3{1, 1, 1} - fresnel) * (1.0f - metallic);
    const Vec3 diffuse = Vec3{kd.x * albedo.x, kd.y * albedo.y, kd.z * albedo.z} * (1.0f / kPi);

    const Vec3 radiance = diffuse + specular;
    return Vec3{radiance.x * light_color.x, radiance.y * light_color.y,
                radiance.z * light_color.z}
           * n_dot_l;
}

} // namespace vyro::pbr

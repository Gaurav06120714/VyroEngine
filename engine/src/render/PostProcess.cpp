// VyroEngine — Post-processing and shadows implementation
#include "vyro/render/PostProcess.hpp"

#include <algorithm>
#include <cmath>

namespace vyro {

namespace shadows {

std::vector<f32> cascade_splits(f32 near_z, f32 far_z, u32 cascades, f32 log_blend)
{
    std::vector<f32> splits;
    splits.reserve(cascades);
    for (u32 i = 1; i <= cascades; ++i) {
        const f32 p = static_cast<f32>(i) / static_cast<f32>(cascades);
        const f32 logarithmic = near_z * std::pow(far_z / near_z, p);
        const f32 uniform = near_z + (far_z - near_z) * p;
        splits.push_back(log_blend * logarithmic + (1.0f - log_blend) * uniform);
    }
    return splits;
}

u32 select_cascade(const std::vector<f32>& splits, f32 view_depth)
{
    for (u32 i = 0; i < splits.size(); ++i) {
        if (view_depth <= splits[i]) {
            return i;
        }
    }
    return splits.empty() ? 0 : static_cast<u32>(splits.size() - 1);
}

} // namespace shadows

namespace postfx {

Vec3 tonemap_reinhard(Vec3 hdr)
{
    return {hdr.x / (1.0f + hdr.x), hdr.y / (1.0f + hdr.y), hdr.z / (1.0f + hdr.z)};
}

Vec3 tonemap_aces(Vec3 hdr)
{
    auto fit = [](f32 x) {
        constexpr f32 a = 2.51f;
        constexpr f32 b = 0.03f;
        constexpr f32 c = 2.43f;
        constexpr f32 d = 0.59f;
        constexpr f32 e = 0.14f;
        return std::clamp((x * (a * x + b)) / (x * (c * x + d) + e), 0.0f, 1.0f);
    };
    return {fit(hdr.x), fit(hdr.y), fit(hdr.z)};
}

Vec3 bloom_extract(Vec3 color, f32 threshold)
{
    auto cut = [threshold](f32 c) { return c > threshold ? c - threshold : 0.0f; };
    return {cut(color.x), cut(color.y), cut(color.z)};
}

} // namespace postfx

void PostProcessStack::add(std::string name, bool enabled)
{
    m_effects.push_back(Effect{std::move(name), enabled});
}

bool PostProcessStack::set_enabled(std::string_view name, bool enabled)
{
    for (Effect& e : m_effects) {
        if (e.name == name) {
            e.enabled = enabled;
            return true;
        }
    }
    return false;
}

bool PostProcessStack::move_before(std::string_view name, std::string_view before)
{
    const auto src = std::find_if(m_effects.begin(), m_effects.end(),
                                  [&](const Effect& e) { return e.name == name; });
    if (src == m_effects.end()) {
        return false;
    }
    Effect moved = *src;
    m_effects.erase(src);
    const auto dst = std::find_if(m_effects.begin(), m_effects.end(),
                                  [&](const Effect& e) { return e.name == before; });
    if (dst == m_effects.end()) {
        m_effects.push_back(std::move(moved));
        return false;
    }
    m_effects.insert(dst, std::move(moved));
    return true;
}

std::vector<std::string> PostProcessStack::active_chain() const
{
    std::vector<std::string> out;
    for (const Effect& e : m_effects) {
        if (e.enabled) {
            out.push_back(e.name);
        }
    }
    return out;
}

} // namespace vyro

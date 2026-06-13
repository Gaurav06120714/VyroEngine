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

std::vector<f32> gaussian_kernel(u32 radius, f32 sigma)
{
    if (sigma <= 0.0f) {
        sigma = static_cast<f32>(radius) * 0.5f;
        if (sigma <= 0.0f) {
            sigma = 1.0f;
        }
    }
    const usize n = static_cast<usize>(radius) * 2 + 1;
    std::vector<f32> k(n);
    f32 sum = 0.0f;
    for (usize i = 0; i < n; ++i) {
        const f32 x = static_cast<f32>(i) - static_cast<f32>(radius);
        const f32 w = std::exp(-(x * x) / (2.0f * sigma * sigma));
        k[i] = w;
        sum += w;
    }
    for (f32& w : k) {
        w /= sum; // normalize so the weights sum to 1
    }
    return k;
}

namespace {

// Sample with clamped edges.
Vec3 sample_clamped(const std::vector<Vec3>& img, u32 width, u32 height, i32 x, i32 y)
{
    const i32 cx = std::clamp(x, 0, static_cast<i32>(width) - 1);
    const i32 cy = std::clamp(y, 0, static_cast<i32>(height) - 1);
    return img[static_cast<usize>(cy) * width + static_cast<usize>(cx)];
}

} // namespace

void blur_separable(std::vector<Vec3>& image, u32 width, u32 height,
                    const std::vector<f32>& kernel)
{
    if (image.empty() || kernel.empty() || width == 0 || height == 0) {
        return;
    }
    const i32 radius = static_cast<i32>(kernel.size() / 2);

    // Horizontal pass into a temp, then vertical pass back into image.
    std::vector<Vec3> tmp(image.size());
    for (u32 y = 0; y < height; ++y) {
        for (u32 x = 0; x < width; ++x) {
            Vec3 acc{};
            for (i32 t = -radius; t <= radius; ++t) {
                acc = acc + sample_clamped(image, width, height, static_cast<i32>(x) + t,
                                           static_cast<i32>(y))
                                * kernel[static_cast<usize>(t + radius)];
            }
            tmp[static_cast<usize>(y) * width + x] = acc;
        }
    }
    for (u32 y = 0; y < height; ++y) {
        for (u32 x = 0; x < width; ++x) {
            Vec3 acc{};
            for (i32 t = -radius; t <= radius; ++t) {
                acc = acc + sample_clamped(tmp, width, height, static_cast<i32>(x),
                                           static_cast<i32>(y) + t)
                                * kernel[static_cast<usize>(t + radius)];
            }
            image[static_cast<usize>(y) * width + x] = acc;
        }
    }
}

std::vector<Vec3> apply_bloom(const std::vector<Vec3>& hdr, u32 width, u32 height, f32 threshold,
                              u32 radius, f32 intensity)
{
    std::vector<Vec3> bright(hdr.size());
    for (usize i = 0; i < hdr.size(); ++i) {
        bright[i] = bloom_extract(hdr[i], threshold);
    }
    blur_separable(bright, width, height, gaussian_kernel(radius));

    std::vector<Vec3> out(hdr.size());
    for (usize i = 0; i < hdr.size(); ++i) {
        out[i] = tonemap_aces(hdr[i] + bright[i] * intensity);
    }
    return out;
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

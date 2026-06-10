// VyroEngine — Vector math
// Phase 3 (math prereq): compact single-precision Vec2/Vec3/Vec4 with the
// operations rendering and physics need. Self-contained to keep the engine
// dependency-free; can be swapped for GLM behind the same names if desired.
#pragma once

#include "vyro/core/Types.hpp"

#include <cmath>

namespace vyro {

struct Vec2 {
    f32 x = 0.0f;
    f32 y = 0.0f;

    friend constexpr Vec2 operator+(Vec2 a, Vec2 b) { return {a.x + b.x, a.y + b.y}; }
    friend constexpr Vec2 operator-(Vec2 a, Vec2 b) { return {a.x - b.x, a.y - b.y}; }
    friend constexpr Vec2 operator*(Vec2 v, f32 s) { return {v.x * s, v.y * s}; }
    friend constexpr bool operator==(Vec2 a, Vec2 b) { return a.x == b.x && a.y == b.y; }
};

struct Vec3 {
    f32 x = 0.0f;
    f32 y = 0.0f;
    f32 z = 0.0f;

    friend constexpr Vec3 operator+(Vec3 a, Vec3 b) { return {a.x + b.x, a.y + b.y, a.z + b.z}; }
    friend constexpr Vec3 operator-(Vec3 a, Vec3 b) { return {a.x - b.x, a.y - b.y, a.z - b.z}; }
    friend constexpr Vec3 operator*(Vec3 v, f32 s) { return {v.x * s, v.y * s, v.z * s}; }
    friend constexpr Vec3 operator-(Vec3 v) { return {-v.x, -v.y, -v.z}; }
    friend constexpr bool operator==(Vec3 a, Vec3 b) { return a.x == b.x && a.y == b.y && a.z == b.z; }
};

struct Vec4 {
    f32 x = 0.0f;
    f32 y = 0.0f;
    f32 z = 0.0f;
    f32 w = 0.0f;

    friend constexpr Vec4 operator+(Vec4 a, Vec4 b) { return {a.x + b.x, a.y + b.y, a.z + b.z, a.w + b.w}; }
    friend constexpr Vec4 operator*(Vec4 v, f32 s) { return {v.x * s, v.y * s, v.z * s, v.w * s}; }
    friend constexpr bool operator==(Vec4 a, Vec4 b) { return a.x == b.x && a.y == b.y && a.z == b.z && a.w == b.w; }
};

// ── Vec3 free functions ──────────────────────────────────────────────
[[nodiscard]] constexpr f32 dot(Vec3 a, Vec3 b) { return a.x * b.x + a.y * b.y + a.z * b.z; }

[[nodiscard]] constexpr Vec3 cross(Vec3 a, Vec3 b)
{
    return {a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x};
}

[[nodiscard]] inline f32 length(Vec3 v) { return std::sqrt(dot(v, v)); }

[[nodiscard]] inline Vec3 normalize(Vec3 v)
{
    const f32 len = length(v);
    return len > 0.0f ? v * (1.0f / len) : v;
}

} // namespace vyro

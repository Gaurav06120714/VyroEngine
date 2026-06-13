// VyroEngine — Steering behaviors & enemy AI (V5.4)
// Reynolds-style steering so the horde seeks the player while spreading out
// (separation) instead of stacking into a single line, plus a tiny behavior
// state machine (idle/seek/attack) selected by distance. Headless and tested.
#pragma once

#include "vyro/core/Types.hpp"
#include "vyro/math/Vec.hpp"

#include <span>

namespace vyro::ai {

enum class ZombieState : u8 {
    Idle,   // no target in range — shamble in place
    Seek,   // chase the player
    Attack, // close enough to bite
};

// Pick a behavior from the distance to the player.
[[nodiscard]] inline ZombieState select_state(f32 distance, f32 seek_radius, f32 attack_radius)
{
    if (distance <= attack_radius) {
        return ZombieState::Attack;
    }
    if (distance <= seek_radius) {
        return ZombieState::Seek;
    }
    return ZombieState::Idle;
}

// Desired velocity heading straight at `target` at `max_speed`.
[[nodiscard]] inline Vec3 seek(Vec3 pos, Vec3 target, f32 max_speed)
{
    const Vec3 to = target - pos;
    const f32 len = length(to);
    if (len <= 1e-5f) {
        return Vec3{};
    }
    return to * (max_speed / len);
}

// Repulsion away from neighbors within `radius`; closer neighbors push harder.
// Returns a velocity-space vector scaled by `strength`.
[[nodiscard]] inline Vec3 separation(Vec3 pos, std::span<const Vec3> neighbors, f32 radius,
                                     f32 strength)
{
    Vec3 push{};
    u32 count = 0;
    for (const Vec3& n : neighbors) {
        const Vec3 away = pos - n;
        const f32 d = length(away);
        if (d > 1e-4f && d < radius) {
            push = push + away * (1.0f / d); // weight by inverse distance
            ++count;
        }
    }
    if (count == 0) {
        return Vec3{};
    }
    const f32 len = length(push);
    if (len <= 1e-5f) {
        return Vec3{};
    }
    return push * (strength / len);
}

// A round obstacle the horde must flow around (a pillar/cover piece).
struct Obstacle {
    Vec3 center{};
    f32 radius = 1.0f;
};

// Steer away from obstacles whose surface is within `margin` of `pos`; closer
// obstacles push harder. Accounts for each obstacle's radius.
[[nodiscard]] inline Vec3 avoid_obstacles(Vec3 pos, std::span<const Obstacle> obstacles,
                                          f32 margin, f32 strength)
{
    Vec3 push{};
    for (const Obstacle& o : obstacles) {
        const Vec3 away = pos - o.center;
        const f32 d = length(away);
        const f32 surface = d - o.radius; // distance to the obstacle's edge
        if (surface < margin && d > 1e-4f) {
            const f32 w = (margin - surface) / margin; // 0 at margin, 1 at the edge
            push = push + away * ((w > 0.0f ? w : 0.0f) / d);
        }
    }
    const f32 len = length(push);
    if (len <= 1e-5f) {
        return Vec3{};
    }
    return push * (strength / len);
}

// Combined desired velocity: seek the player + spread from the pack, clamped to
// `max_speed`. Keeps the horde surrounding rather than single-file.
[[nodiscard]] inline Vec3 horde_velocity(Vec3 pos, Vec3 target, std::span<const Vec3> neighbors,
                                         f32 max_speed, f32 separation_radius,
                                         f32 separation_strength)
{
    Vec3 v = seek(pos, target, max_speed)
             + separation(pos, neighbors, separation_radius, separation_strength);
    const f32 len = length(v);
    if (len > max_speed && len > 1e-5f) {
        v = v * (max_speed / len);
    }
    return v;
}

} // namespace vyro::ai

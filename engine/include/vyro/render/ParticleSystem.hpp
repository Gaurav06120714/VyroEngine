// VyroEngine — Particle system (V4.1)
// A pooled CPU particle simulator: emitters spawn particles that age, move
// under gravity, and fade out. The simulation is headless and deterministic
// (tests verify it); a renderer turns live particles into camera-facing quads.
#pragma once

#include "vyro/assets/Mesh.hpp"
#include "vyro/core/Types.hpp"
#include "vyro/math/Mat4.hpp"
#include "vyro/math/Vec.hpp"

#include <vector>

namespace vyro {

struct Particle {
    Vec3 position{};
    Vec3 velocity{};
    Vec3 color{1.0f, 1.0f, 1.0f};
    f32 age = 0.0f;
    f32 lifetime = 1.0f;
    f32 size = 0.1f;
    bool alive = false;

    [[nodiscard]] f32 fade() const { return lifetime > 0.0f ? 1.0f - age / lifetime : 0.0f; }
};

// Parameters for a one-shot burst of particles.
struct BurstParams {
    Vec3 origin{};
    Vec3 base_velocity{};  // shared directional push (e.g. recoil)
    f32 speed = 3.0f;      // random radial speed magnitude
    f32 lifetime = 0.5f;
    f32 size = 0.12f;
    Vec3 color{1.0f, 1.0f, 1.0f};
    u32 count = 16;
};

class ParticleSystem
{
public:
    explicit ParticleSystem(usize capacity = 2048);

    // Emit a radial burst. Particles spawn into free pool slots; if the pool
    // is full, the burst is truncated.
    void burst(const BurstParams& params);

    // Advance the simulation: integrate gravity, age particles, retire dead.
    void update(f32 dt);

    void set_gravity(Vec3 gravity) { m_gravity = gravity; }
    void clear();

    [[nodiscard]] u32 alive_count() const { return m_alive; }
    [[nodiscard]] const std::vector<Particle>& particles() const { return m_particles; }

    // Build camera-facing quads for all live particles (two triangles each).
    // `right` and `up` are the camera basis vectors in world space.
    void build_quads(Vec3 right, Vec3 up, std::vector<Vertex3D>& out) const;

private:
    std::vector<Particle> m_particles;
    Vec3 m_gravity{0.0f, -4.0f, 0.0f};
    u32 m_alive = 0;
    u32 m_seed = 0x1234567u;

    [[nodiscard]] f32 rand_unit(); // [-1, 1]
};

} // namespace vyro

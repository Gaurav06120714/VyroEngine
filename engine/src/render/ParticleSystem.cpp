// VyroEngine — Particle system implementation
#include "vyro/render/ParticleSystem.hpp"

namespace vyro {

ParticleSystem::ParticleSystem(usize capacity)
{
    m_particles.resize(capacity);
}

f32 ParticleSystem::rand_unit()
{
    // xorshift32, mapped to [-1, 1] — deterministic for reproducible tests.
    m_seed ^= m_seed << 13;
    m_seed ^= m_seed >> 17;
    m_seed ^= m_seed << 5;
    return (static_cast<f32>(m_seed & 0xFFFFFF) / static_cast<f32>(0xFFFFFF)) * 2.0f - 1.0f;
}

void ParticleSystem::burst(const BurstParams& params)
{
    u32 spawned = 0;
    for (Particle& p : m_particles) {
        if (spawned >= params.count) {
            break;
        }
        if (p.alive) {
            continue;
        }
        const Vec3 dir = normalize(Vec3{rand_unit(), rand_unit(), rand_unit()});
        const f32 speed = params.speed * (0.5f + 0.5f * (rand_unit() * 0.5f + 0.5f));
        p.position = params.origin;
        p.velocity = params.base_velocity + dir * speed;
        p.color = params.color;
        p.age = 0.0f;
        p.lifetime = params.lifetime * (0.7f + 0.3f * (rand_unit() * 0.5f + 0.5f));
        p.size = params.size;
        p.alive = true;
        ++spawned;
        ++m_alive;
    }
}

void ParticleSystem::update(f32 dt)
{
    for (Particle& p : m_particles) {
        if (!p.alive) {
            continue;
        }
        p.age += dt;
        if (p.age >= p.lifetime) {
            p.alive = false;
            --m_alive;
            continue;
        }
        p.velocity = p.velocity + m_gravity * dt;
        p.position = p.position + p.velocity * dt;
    }
}

void ParticleSystem::clear()
{
    for (Particle& p : m_particles) {
        p.alive = false;
    }
    m_alive = 0;
}

void ParticleSystem::build_quads(Vec3 right, Vec3 up, std::vector<Vertex3D>& out) const
{
    const Vec3 normal{0, 0, 1};
    for (const Particle& p : m_particles) {
        if (!p.alive) {
            continue;
        }
        const f32 s = p.size * 0.5f;
        // Fade the color toward black as the particle dies (cheap brightness ramp).
        const Vec3 col = p.color * p.fade();
        const Vec3 r = right * s;
        const Vec3 u = up * s;
        const Vec3 bl = p.position - r - u;
        const Vec3 br = p.position + r - u;
        const Vec3 tr = p.position + r + u;
        const Vec3 tl = p.position - r + u;
        out.push_back({bl, normal, {0, 0}, col});
        out.push_back({br, normal, {1, 0}, col});
        out.push_back({tr, normal, {1, 1}, col});
        out.push_back({bl, normal, {0, 0}, col});
        out.push_back({tr, normal, {1, 1}, col});
        out.push_back({tl, normal, {0, 1}, col});
    }
}

} // namespace vyro

// VyroEngine — Particle system tests (V4.1)
#include "vyro/render/ParticleSystem.hpp"

#include "test_harness.hpp"

#include <cmath>

int main()
{
    using namespace vyro;
    test::Suite suite("particles");

    // Burst_SpawnsRequestedCount
    {
        ParticleSystem ps(256);
        BurstParams b;
        b.count = 20;
        ps.burst(b);
        suite.check(ps.alive_count() == 20, "burst spawns requested particles");
    }

    // Burst_TruncatedAtCapacity
    {
        ParticleSystem ps(8);
        BurstParams b;
        b.count = 50;
        ps.burst(b);
        suite.check(ps.alive_count() == 8, "burst truncates at pool capacity");
    }

    // Update_AgesAndRetires
    {
        ParticleSystem ps(64);
        ps.set_gravity({0, 0, 0});
        BurstParams b;
        b.count = 10;
        b.lifetime = 0.5f;
        ps.burst(b);
        ps.update(0.25f);
        suite.check(ps.alive_count() == 10, "particles alive mid-life");
        ps.update(0.5f); // total 0.75 > max lifetime
        suite.check(ps.alive_count() == 0, "particles retired after lifetime");
    }

    // Update_GravityPullsDown
    {
        ParticleSystem ps(16);
        ps.set_gravity({0, -10, 0});
        BurstParams b;
        b.count = 1;
        b.speed = 0.0f;
        b.base_velocity = {0, 0, 0};
        b.origin = {0, 5, 0};
        b.lifetime = 2.0f;
        ps.burst(b);
        const f32 y0 = ps.particles()[0].position.y;
        ps.update(0.1f);
        suite.check(ps.particles()[0].position.y < y0, "gravity pulls particle down");
    }

    // Fade_DecreasesWithAge
    {
        ParticleSystem ps(8);
        BurstParams b;
        b.count = 1;
        b.lifetime = 1.0f;
        ps.burst(b);
        const f32 f0 = ps.particles()[0].fade();
        ps.update(0.5f);
        suite.check(ps.particles()[0].fade() < f0, "fade decreases over time");
    }

    // BuildQuads_SixVertsPerLiveParticle
    {
        ParticleSystem ps(64);
        BurstParams b;
        b.count = 5;
        ps.burst(b);
        std::vector<Vertex3D> quads;
        ps.build_quads({1, 0, 0}, {0, 1, 0}, quads);
        suite.check(quads.size() == 5 * 6, "six verts per live particle");
    }

    // Determinism
    {
        ParticleSystem a(32);
        ParticleSystem c(32);
        BurstParams b;
        b.count = 8;
        a.burst(b);
        c.burst(b);
        suite.check(a.particles()[0].velocity == c.particles()[0].velocity,
                    "bursts are deterministic");
    }

    return suite.summary();
}

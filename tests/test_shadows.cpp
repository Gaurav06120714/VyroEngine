// VyroEngine — Shadow mapping tests (V5.2)
// The directional light's view-projection centers the scene in shadow-map NDC
// and keeps points within the configured radius inside the [-1,1] box.
#include "vyro/render/PostProcess.hpp"

#include "test_harness.hpp"

#include <cmath>

namespace {
bool approx(vyro::f32 a, vyro::f32 b, vyro::f32 eps = 1e-3f) { return std::fabs(a - b) < eps; }
bool in_ndc(vyro::f32 v) { return v >= -1.0001f && v <= 1.0001f; }
} // namespace

int main()
{
    using namespace vyro;
    test::Suite suite("shadows");

    const Vec3 light_dir{-0.4f, -1.0f, -0.3f};
    const Vec3 center{0.0f, 0.0f, 0.0f};
    const f32 radius = 12.0f;
    const f32 depth = 30.0f;
    const Mat4 lvp = shadows::light_view_projection(light_dir, center, radius, depth);

    // The scene center projects to the middle of the shadow map.
    {
        const Vec3 ndc = transform_point(lvp, center);
        suite.check(approx(ndc.x, 0.0f) && approx(ndc.y, 0.0f), "center maps to shadow-map origin");
        suite.check(in_ndc(ndc.z), "center depth within the light's range");
    }

    // Points within the radius stay inside the shadow-map box.
    {
        const Vec3 a = transform_point(lvp, Vec3{radius * 0.5f, 0.0f, 0.0f});
        const Vec3 b = transform_point(lvp, Vec3{0.0f, 0.0f, radius * 0.5f});
        suite.check(in_ndc(a.x) && in_ndc(a.y), "point inside radius is inside the box (x)");
        suite.check(in_ndc(b.x) && in_ndc(b.y), "point inside radius is inside the box (z)");
    }

    // A point well outside the radius falls outside the box on some axis.
    {
        const Vec3 far_pt = transform_point(lvp, Vec3{radius * 3.0f, 0.0f, 0.0f});
        suite.check(!in_ndc(far_pt.x) || !in_ndc(far_pt.y),
                    "point well beyond radius lands outside the box");
    }

    // Distinct world heights produce distinct shadow-map depths (ordering).
    {
        const Vec3 low = transform_point(lvp, Vec3{0.0f, -2.0f, 0.0f});
        const Vec3 high = transform_point(lvp, Vec3{0.0f, 6.0f, 0.0f});
        suite.check(!approx(low.z, high.z), "different heights have different light-space depth");
    }

    // Slope-scaled bias: base when facing the light, more at grazing, clamped.
    {
        suite.check(approx(shadows::slope_scaled_bias(1.0f, 0.002f, 0.01f), 0.002f),
                    "facing the light uses the base bias");
        suite.check(approx(shadows::slope_scaled_bias(0.0f, 0.002f, 0.01f), 0.012f),
                    "grazing adds the full slope bias");
        suite.check(shadows::slope_scaled_bias(0.5f, 0.002f, 0.01f) > 0.002f
                        && shadows::slope_scaled_bias(0.5f, 0.002f, 0.01f) < 0.012f,
                    "mid angle is between base and max");
        suite.check(approx(shadows::slope_scaled_bias(-5.0f, 0.002f, 0.01f), 0.012f),
                    "n.l clamps below zero");
    }

    return suite.summary();
}

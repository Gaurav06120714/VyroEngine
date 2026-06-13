// VyroEngine — Bloom pipeline tests (V5.1)
// The CPU reference bloom: Gaussian kernels normalize, separable blur spreads
// energy without amplifying it, and apply_bloom only lights up bright pixels.
#include "vyro/render/PostProcess.hpp"

#include "test_harness.hpp"

#include <cmath>

namespace {
bool approx(vyro::f32 a, vyro::f32 b, vyro::f32 eps = 1e-4f) { return std::fabs(a - b) < eps; }
} // namespace

int main()
{
    using namespace vyro;
    test::Suite suite("bloom");

    // Gaussian kernel: correct length, sums to 1, peaks in the center.
    {
        const auto k = postfx::gaussian_kernel(3);
        suite.check(k.size() == 7, "kernel length is 2*radius+1");
        f32 sum = 0.0f;
        for (const f32 w : k) {
            sum += w;
        }
        suite.check(approx(sum, 1.0f), "kernel weights sum to 1");
        suite.check(k[3] >= k[0] && k[3] >= k[6], "center weight is the largest");
    }

    // Separable blur conserves total energy and spreads a single bright pixel.
    {
        const u32 w = 9;
        const u32 h = 9;
        std::vector<Vec3> img(w * h, Vec3{0, 0, 0});
        const usize center = (h / 2) * w + (w / 2);
        img[center] = Vec3{9, 0, 0};
        f32 before = 0.0f;
        for (const auto& p : img) {
            before += p.x;
        }

        postfx::blur_separable(img, w, h, postfx::gaussian_kernel(2));

        f32 after = 0.0f;
        for (const auto& p : img) {
            after += p.x;
        }
        suite.check(approx(before, after, 1e-2f), "blur conserves total energy");
        suite.check(img[center].x < 9.0f && img[center].x > 0.0f, "center dimmed by spreading");
        suite.check(img[center - 1].x > 0.0f, "energy bled to neighbors");
    }

    // apply_bloom: a dark scene with one bright pixel stays mostly dark but the
    // bright pixel blooms into its neighbors; everything is tonemapped to [0,1].
    {
        const u32 w = 8;
        const u32 h = 8;
        std::vector<Vec3> hdr(w * h, Vec3{0.1f, 0.1f, 0.1f});
        const usize center = (h / 2) * w + (w / 2);
        hdr[center] = Vec3{6, 6, 6};

        const auto out = postfx::apply_bloom(hdr, w, h, /*threshold*/ 1.0f, /*radius*/ 2,
                                             /*intensity*/ 1.0f);
        suite.check(out.size() == hdr.size(), "output matches input size");

        bool in_range = true;
        for (const auto& p : out) {
            if (p.x < 0.0f || p.x > 1.0001f) {
                in_range = false;
            }
        }
        suite.check(in_range, "tonemapped output stays in [0,1]");

        const usize corner = 0;
        suite.check(out[center].x > out[corner].x, "bright pixel stays brighter than a corner");
        suite.check(out[center - 1].x > 0.1f, "glow lifts the neighbor above the dim floor");
    }

    return suite.summary();
}

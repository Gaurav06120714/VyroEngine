// VyroEngine — Advanced graphics tests
#include "vyro/render/FrameGraph.hpp"
#include "vyro/render/PBR.hpp"
#include "vyro/render/PostProcess.hpp"

#include "test_harness.hpp"

#include <cmath>

namespace {
bool approx(vyro::f32 a, vyro::f32 b, vyro::f32 eps = 1e-3f) { return std::fabs(a - b) < eps; }
} // namespace

int main()
{
    using namespace vyro;
    test::Suite suite("advgfx");

    // PBR_Fresnel_GrazingAngleReflectsFully (10.2)
    {
        const Vec3 f0{0.04f, 0.04f, 0.04f};
        const Vec3 grazing = pbr::fresnel_schlick(0.0f, f0);
        const Vec3 head_on = pbr::fresnel_schlick(1.0f, f0);
        suite.check(approx(grazing.x, 1.0f), "grazing angle reflects ~100%");
        suite.check(approx(head_on.x, 0.04f), "normal incidence reflects f0");
        suite.check(pbr::base_reflectance(Vec3{1, 0, 0}, 1.0f) == (Vec3{1, 0, 0}),
                    "metal f0 equals albedo");
    }

    // PBR_Shade_LightBehindSurfaceIsBlack (10.2)
    {
        const Vec3 n{0, 1, 0};
        const Vec3 v = normalize(Vec3{0, 1, 1});
        const Vec3 behind{0, -1, 0};
        const Vec3 lit = pbr::shade(n, v, normalize(Vec3{0, 1, 0.2f}), Vec3{0.8f, 0.8f, 0.8f},
                                    0.0f, 0.5f, Vec3{1, 1, 1});
        const Vec3 dark = pbr::shade(n, v, behind, Vec3{0.8f, 0.8f, 0.8f}, 0.0f, 0.5f, Vec3{1, 1, 1});
        suite.check(lit.x > 0.0f, "front light produces radiance");
        suite.check(dark.x == 0.0f && dark.y == 0.0f, "light behind surface is black");
    }

    // Shadows_CascadeSplits_MonotonicAndCovering (10.3)
    {
        const auto splits = shadows::cascade_splits(0.1f, 100.0f, 4);
        suite.check(splits.size() == 4, "four cascades");
        suite.check(splits[0] < splits[1] && splits[1] < splits[2] && splits[2] < splits[3],
                    "splits increase monotonically");
        suite.check(approx(splits[3], 100.0f, 0.5f), "last split reaches far plane");
        suite.check(shadows::select_cascade(splits, 0.5f) == 0, "near depth uses cascade 0");
        suite.check(shadows::select_cascade(splits, 99.0f) == 3, "far depth uses last cascade");
    }

    // PostFX_Tonemap_MapsHDRIntoLDR (10.4)
    {
        const Vec3 hdr{4.0f, 4.0f, 4.0f};
        const Vec3 reinhard = postfx::tonemap_reinhard(hdr);
        const Vec3 aces = postfx::tonemap_aces(hdr);
        suite.check(reinhard.x < 1.0f && reinhard.x > 0.5f, "reinhard compresses into LDR");
        suite.check(aces.x <= 1.0f, "aces clamps to LDR");
        suite.check(postfx::bloom_extract(Vec3{0.5f, 2.0f, 1.0f}, 1.0f)
                        == (Vec3{0.0f, 1.0f, 0.0f}),
                    "bloom keeps only above-threshold energy");
    }

    // PostProcessStack_OrderAndToggle (10.4)
    {
        PostProcessStack stack;
        stack.add("Bloom");
        stack.add("Tonemap");
        stack.add("FXAA");
        suite.check(stack.set_enabled("Bloom", false), "toggle bloom off");
        const auto chain = stack.active_chain();
        suite.check(chain.size() == 2 && chain[0] == "Tonemap", "disabled effect skipped");
        suite.check(stack.move_before("FXAA", "Tonemap"), "reorder FXAA before Tonemap");
        suite.check(stack.active_chain()[0] == "FXAA", "reorder reflected in chain");
    }

    // FrameGraph_Compile_RespectsDependencies (10.1)
    {
        FrameGraph graph;
        const auto post = graph.add_pass("Post", {"lit"}, {"backbuffer"});
        const auto shadow = graph.add_pass("Shadow", {}, {"shadowmap"});
        const auto lighting = graph.add_pass("Lighting", {"gbuffer", "shadowmap"}, {"lit"});
        const auto gbuffer = graph.add_pass("GBuffer", {}, {"gbuffer"});

        const auto order = graph.compile();
        suite.check(order.has_value(), "graph compiles");
        auto pos = [&](FrameGraph::PassId id) {
            for (usize i = 0; i < order->size(); ++i) {
                if ((*order)[i] == id) {
                    return i;
                }
            }
            return usize(99);
        };
        suite.check(pos(shadow) < pos(lighting), "shadow before lighting");
        suite.check(pos(gbuffer) < pos(lighting), "gbuffer before lighting");
        suite.check(pos(lighting) < pos(post), "lighting before post");
    }

    // FrameGraph_Cycle_Detected (10.1)
    {
        FrameGraph graph;
        graph.add_pass("A", {"b"}, {"a"});
        graph.add_pass("B", {"a"}, {"b"});
        const auto order = graph.compile();
        suite.check(!order.has_value(), "cyclic graph rejected");
    }

    return suite.summary();
}

// VyroEngine — Animation blending tests (V4.3)
// Verifies SkinnedModel::pose_blend cross-fades two clips: weight 0 matches
// clip A, weight 1 matches clip B, and an intermediate weight lands between.
#include "vyro/assets/SkinnedModel.hpp"

#include "test_harness.hpp"

#include <cmath>
#include <string>

namespace {

std::expected<vyro::SkinnedModel, vyro::GlbError> load_any(const char* rel)
{
    for (const char* prefix : {"", "../", "../../"}) {
        auto r = vyro::load_glb_skinned(std::string(prefix) + rel);
        if (r.has_value() || r.error() != vyro::GlbError::FileNotFound) {
            return r;
        }
    }
    return std::unexpected(vyro::GlbError::FileNotFound);
}

vyro::f32 max_displacement(const std::vector<vyro::Vertex3D>& a,
                           const std::vector<vyro::Vertex3D>& b)
{
    vyro::f32 best = 0.0f;
    for (vyro::usize i = 0; i < a.size(); ++i) {
        best = std::max(best, vyro::length(a[i].position - b[i].position));
    }
    return best;
}

} // namespace

int main()
{
    using namespace vyro;
    test::Suite suite("pose_blend");

    const auto model = load_any("assets/models/characters/zombie_animated.glb");
    suite.check(model.has_value(), "rigged zombie loads");
    if (!model.has_value()) {
        return suite.summary();
    }
    suite.check(model->clips.size() >= 2, "model has at least two clips to blend");
    if (model->clips.size() < 2) {
        return suite.summary();
    }

    const usize clip_a = 0;
    const usize clip_b = model->clips.size() > 1 ? 1u : 0u;
    const f32 ta = model->clips[clip_a].duration * 0.25f;
    const f32 tb = model->clips[clip_b].duration * 0.6f;

    // Reference poses from the single-clip sampler.
    std::vector<Mat4> only_a;
    std::vector<Mat4> only_b;
    model->pose(clip_a, ta, only_a);
    model->pose(clip_b, tb, only_b);

    std::vector<Vertex3D> skinned_only_a;
    std::vector<Vertex3D> skinned_only_b;
    model->skin(only_a, skinned_only_a);
    model->skin(only_b, skinned_only_b);

    // Blended poses at the extremes and the midpoint.
    std::vector<Mat4> blended;
    std::vector<Vertex3D> skinned_blended;

    model->pose_blend(clip_a, ta, clip_b, tb, 0.0f, blended);
    suite.check(blended.size() == model->joint_nodes.size(), "blend covers all joints");
    model->skin(blended, skinned_blended);
    suite.check(max_displacement(skinned_blended, skinned_only_a) < 1e-3f,
                "weight 0 reproduces clip A");

    model->pose_blend(clip_a, ta, clip_b, tb, 1.0f, blended);
    model->skin(blended, skinned_blended);
    suite.check(max_displacement(skinned_blended, skinned_only_b) < 1e-3f,
                "weight 1 reproduces clip B");

    model->pose_blend(clip_a, ta, clip_b, tb, 0.5f, blended);
    model->skin(blended, skinned_blended);
    const f32 to_a = max_displacement(skinned_blended, skinned_only_a);
    const f32 to_b = max_displacement(skinned_blended, skinned_only_b);
    const f32 a_to_b = max_displacement(skinned_only_a, skinned_only_b);
    suite.check(a_to_b > 1e-3f, "the two clips differ");
    suite.check(to_a < a_to_b && to_b < a_to_b,
                "midpoint blend lies between clip A and clip B");

    // Weights clamp: out-of-range weights match the extremes.
    model->pose_blend(clip_a, ta, clip_b, tb, -0.5f, blended);
    model->skin(blended, skinned_blended);
    suite.check(max_displacement(skinned_blended, skinned_only_a) < 1e-3f,
                "negative weight clamps to clip A");
    model->pose_blend(clip_a, ta, clip_b, tb, 2.0f, blended);
    model->skin(blended, skinned_blended);
    suite.check(max_displacement(skinned_blended, skinned_only_b) < 1e-3f,
                "weight > 1 clamps to clip B");

    return suite.summary();
}

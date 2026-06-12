// VyroEngine — Skeletal animation tests (V3.1)
// Loads the real rigged zombie and verifies skinning + clip sampling.
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
    test::Suite suite("skinned");

    // Quaternion math sanity.
    {
        const Quat identity{};
        const Mat4 m = quat_to_mat4(identity);
        suite.check(m == Mat4::identity(), "identity quat -> identity matrix");
        const Quat half = nlerp(Quat{0, 0, 0, 1}, Quat{0, 0, 0, 1}, 0.5f);
        suite.check(std::fabs(half.w - 1.0f) < 1e-5f, "nlerp of identical quats is stable");
    }

    const auto model = load_any("assets/models/characters/zombie_animated.glb");
    suite.check(model.has_value(), "rigged zombie loads");
    if (!model.has_value()) {
        return suite.summary();
    }

    // Rig structure.
    suite.check(!model->joint_nodes.empty(), "skin has joints");
    suite.check(model->inverse_bind.size() == model->joint_nodes.size(),
                "one inverse bind matrix per joint");
    suite.check(model->joints.size() == model->mesh.vertices.size(),
                "joint indices per vertex");
    suite.check(model->weights.size() == model->mesh.vertices.size(), "weights per vertex");
    suite.check(!model->clips.empty(), "animation clips loaded");

    // Weights are normalized-ish.
    {
        const auto& w = model->weights[0];
        const f32 sum = w[0] + w[1] + w[2] + w[3];
        suite.check(sum > 0.9f && sum < 1.1f, "vertex weights sum to ~1");
    }

    // Sampling the clip at different times produces different poses.
    {
        const SkinClip& clip = model->clips[0];
        suite.check(clip.duration > 0.0f, "clip has duration");

        std::vector<Mat4> pose_a;
        std::vector<Mat4> pose_b;
        model->pose(0, 0.0f, pose_a);
        model->pose(0, clip.duration * 0.5f, pose_b);
        suite.check(pose_a.size() == model->joint_nodes.size(), "pose covers all joints");

        std::vector<Vertex3D> skinned_a;
        std::vector<Vertex3D> skinned_b;
        model->skin(pose_a, skinned_a);
        model->skin(pose_b, skinned_b);
        suite.check(skinned_a.size() == model->mesh.vertices.size(), "skinned all vertices");

        const f32 moved = max_displacement(skinned_a, skinned_b);
        suite.check(moved > 0.01f, "animation moves vertices between sample times");
    }

    // Clip lookup.
    {
        suite.check(model->find_clip(model->clips[0].name) == 0, "find_clip resolves by name");
        suite.check(model->find_clip("not-a-real-clip") == -1, "unknown clip returns -1");
    }

    return suite.summary();
}

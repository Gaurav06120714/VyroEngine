// VyroEngine — Skinned model with skeletal animation (V3.1)
// Loads a rigged GLB: bind-pose geometry with per-vertex joint indices and
// weights, the node hierarchy, the skin's joints + inverse bind matrices, and
// every animation clip (translation/rotation/scale keyframes per node).
//
// Playback is CPU-side: sample a clip at a time to get joint matrices, then
// skin the bind-pose vertices and stream them to the GPU with update_buffer.
#pragma once

#include "vyro/assets/GlbLoader.hpp"
#include "vyro/assets/Image.hpp"
#include "vyro/assets/Mesh.hpp"
#include "vyro/math/Mat4.hpp"

#include <array>
#include <expected>
#include <string>
#include <string_view>
#include <vector>

namespace vyro {

// Quaternion stored as (x, y, z, w).
struct Quat {
    f32 x = 0.0f;
    f32 y = 0.0f;
    f32 z = 0.0f;
    f32 w = 1.0f;
};

[[nodiscard]] Mat4 quat_to_mat4(const Quat& q);
[[nodiscard]] Quat nlerp(const Quat& a, const Quat& b, f32 t);

// A scene-graph node with its rest-pose local transform.
struct SkinNode {
    i32 parent = -1;
    Vec3 translation{};
    Quat rotation{};
    Vec3 scale{1.0f, 1.0f, 1.0f};
};

// Keyframed track for one node within a clip.
struct NodeTrack {
    std::vector<f32> t_times;
    std::vector<Vec3> t_values;
    std::vector<f32> r_times;
    std::vector<Quat> r_values;
    std::vector<f32> s_times;
    std::vector<Vec3> s_values;
};

struct SkinClip {
    std::string name;
    f32 duration = 0.0f;
    std::vector<i32> track_of_node; // node index -> track index or -1
    std::vector<NodeTrack> tracks;
};

class SkinnedModel
{
public:
    MeshData mesh; // bind-pose vertices (untransformed)
    std::vector<std::array<u16, 4>> joints; // per-vertex joint indices (skin-relative)
    std::vector<std::array<f32, 4>> weights;

    std::vector<SkinNode> nodes;
    std::vector<u32> joint_nodes;   // skin joint -> node index
    std::vector<Mat4> inverse_bind; // one per skin joint

    std::vector<SkinClip> clips;
    Image texture;
    bool has_texture = false;

    [[nodiscard]] i32 find_clip(std::string_view name) const;

    // Sample `clip` at time t (wrapping) into per-joint skinning matrices.
    void pose(usize clip, f32 t, std::vector<Mat4>& out_joint_matrices) const;

    // Cross-fade two clips: sample clip_a at ta and clip_b at tb, blend their
    // per-node local transforms by `weight` (0 = a, 1 = b) — lerp translation
    // and scale, nlerp rotation — then resolve into per-joint skinning matrices.
    void pose_blend(usize clip_a, f32 ta, usize clip_b, f32 tb, f32 weight,
                    std::vector<Mat4>& out_joint_matrices) const;

    // Apply skinning matrices to the bind pose. `out` must have mesh.vertices.size().
    void skin(const std::vector<Mat4>& joint_matrices, std::vector<Vertex3D>& out) const;
};

[[nodiscard]] std::expected<SkinnedModel, GlbError> load_glb_skinned(std::string_view path);

} // namespace vyro

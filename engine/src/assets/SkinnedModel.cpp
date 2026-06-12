// VyroEngine — Skinned model implementation
#include "vyro/assets/SkinnedModel.hpp"

#include <nlohmann/json.hpp>
#include <stb_image.h>

#include <cmath>
#include <cstring>
#include <fstream>

namespace vyro {

namespace {

using nlohmann::json;

constexpr u32 kUShort = 5123;
constexpr u32 kUInt = 5125;
constexpr u32 kFloat = 5126;

struct AccessorView {
    const u8* data = nullptr;
    usize count = 0;
    usize stride = 0;
    u32 component_type = 0;
};

AccessorView view_accessor(const json& gltf, const std::vector<u8>& bin, usize accessor_index,
                           usize components)
{
    AccessorView out;
    const json& acc = gltf["accessors"][accessor_index];
    const usize bv_index = acc.value("bufferView", usize(0));
    const json& bv = gltf["bufferViews"][bv_index];
    const usize acc_offset = acc.value("byteOffset", usize(0));
    const usize bv_offset = bv.value("byteOffset", usize(0));
    out.component_type = acc.value("componentType", 0u);
    const usize comp_size = (out.component_type == kFloat || out.component_type == kUInt) ? 4
                            : (out.component_type == kUShort)                             ? 2
                                                                                          : 1;
    out.stride = bv.value("byteStride", comp_size * components);
    out.count = acc.value("count", usize(0));
    out.data = bin.data() + bv_offset + acc_offset;
    return out;
}

Vec3 read_vec3(const AccessorView& v, usize i)
{
    const auto* f = reinterpret_cast<const float*>(v.data + i * v.stride);
    return {f[0], f[1], f[2]};
}

Vec2 read_vec2(const AccessorView& v, usize i)
{
    const auto* f = reinterpret_cast<const float*>(v.data + i * v.stride);
    return {f[0], f[1]};
}

Quat read_quat(const AccessorView& v, usize i)
{
    const auto* f = reinterpret_cast<const float*>(v.data + i * v.stride);
    return {f[0], f[1], f[2], f[3]};
}

f32 read_scalar(const AccessorView& v, usize i)
{
    const auto* f = reinterpret_cast<const float*>(v.data + i * v.stride);
    return f[0];
}

u32 read_uint_comp(const AccessorView& v, usize i, usize comp)
{
    const u8* p = v.data + i * v.stride;
    if (v.component_type == kUShort) {
        u16 value = 0;
        std::memcpy(&value, p + comp * 2, 2);
        return value;
    }
    if (v.component_type == kUInt) {
        u32 value = 0;
        std::memcpy(&value, p + comp * 4, 4);
        return value;
    }
    return p[comp]; // ubyte
}

Mat4 read_mat4(const AccessorView& v, usize i)
{
    Mat4 m;
    const auto* f = reinterpret_cast<const float*>(v.data + i * v.stride);
    for (usize k = 0; k < 16; ++k) {
        m.data[k] = f[k];
    }
    return m;
}

template<typename T>
T sample_track(const std::vector<f32>& times, const std::vector<T>& values, f32 t, T fallback,
               T (*mix)(const T&, const T&, f32))
{
    if (times.empty()) {
        return fallback;
    }
    if (t <= times.front()) {
        return values.front();
    }
    if (t >= times.back()) {
        return values.back();
    }
    for (usize i = 1; i < times.size(); ++i) {
        if (t < times[i]) {
            const f32 span = times[i] - times[i - 1];
            const f32 w = span > 0.0f ? (t - times[i - 1]) / span : 0.0f;
            return mix(values[i - 1], values[i], w);
        }
    }
    return values.back();
}

Vec3 mix_vec3(const Vec3& a, const Vec3& b, f32 t)
{
    return a + (b - a) * t;
}

Quat mix_quat(const Quat& a, const Quat& b, f32 t)
{
    return nlerp(a, b, t);
}

} // namespace

Mat4 quat_to_mat4(const Quat& q)
{
    Mat4 m = Mat4::identity();
    m.at(0, 0) = 1 - 2 * (q.y * q.y + q.z * q.z);
    m.at(0, 1) = 2 * (q.x * q.y - q.z * q.w);
    m.at(0, 2) = 2 * (q.x * q.z + q.y * q.w);
    m.at(1, 0) = 2 * (q.x * q.y + q.z * q.w);
    m.at(1, 1) = 1 - 2 * (q.x * q.x + q.z * q.z);
    m.at(1, 2) = 2 * (q.y * q.z - q.x * q.w);
    m.at(2, 0) = 2 * (q.x * q.z - q.y * q.w);
    m.at(2, 1) = 2 * (q.y * q.z + q.x * q.w);
    m.at(2, 2) = 1 - 2 * (q.x * q.x + q.y * q.y);
    return m;
}

Quat nlerp(const Quat& a, const Quat& b, f32 t)
{
    // Take the short path: flip b when the quaternions point apart.
    const f32 dot = a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;
    const f32 sign = dot < 0.0f ? -1.0f : 1.0f;
    Quat out{a.x + (b.x * sign - a.x) * t, a.y + (b.y * sign - a.y) * t,
             a.z + (b.z * sign - a.z) * t, a.w + (b.w * sign - a.w) * t};
    const f32 len = std::sqrt(out.x * out.x + out.y * out.y + out.z * out.z + out.w * out.w);
    if (len > 0.0f) {
        out.x /= len;
        out.y /= len;
        out.z /= len;
        out.w /= len;
    }
    return out;
}

i32 SkinnedModel::find_clip(std::string_view name) const
{
    for (usize i = 0; i < clips.size(); ++i) {
        if (clips[i].name == name) {
            return static_cast<i32>(i);
        }
    }
    return -1;
}

void SkinnedModel::pose(usize clip_index, f32 t, std::vector<Mat4>& out) const
{
    const SkinClip& clip = clips[clip_index];
    if (clip.duration > 0.0f) {
        t = std::fmod(t, clip.duration);
        if (t < 0.0f) {
            t += clip.duration;
        }
    }

    // Animated local transforms (rest pose where untracked).
    std::vector<Mat4> local(nodes.size());
    for (usize n = 0; n < nodes.size(); ++n) {
        const SkinNode& node = nodes[n];
        Vec3 tr = node.translation;
        Quat ro = node.rotation;
        Vec3 sc = node.scale;
        const i32 track_index = n < clip.track_of_node.size() ? clip.track_of_node[n] : -1;
        if (track_index >= 0) {
            const NodeTrack& track = clip.tracks[static_cast<usize>(track_index)];
            tr = sample_track<Vec3>(track.t_times, track.t_values, t, tr, mix_vec3);
            ro = sample_track<Quat>(track.r_times, track.r_values, t, ro, mix_quat);
            sc = sample_track<Vec3>(track.s_times, track.s_values, t, sc, mix_vec3);
        }
        local[n] = Mat4::translation(tr) * quat_to_mat4(ro) * Mat4::scale(sc);
    }

    // Globals: nodes reference parents by index; resolve iteratively with memo.
    std::vector<Mat4> global(nodes.size());
    std::vector<bool> done(nodes.size(), false);
    for (usize n = 0; n < nodes.size(); ++n) {
        // Walk up to the root collecting the chain, then resolve down.
        std::vector<usize> chain;
        usize cur = n;
        while (!done[cur]) {
            chain.push_back(cur);
            const i32 parent = nodes[cur].parent;
            if (parent < 0) {
                break;
            }
            cur = static_cast<usize>(parent);
        }
        for (auto it = chain.rbegin(); it != chain.rend(); ++it) {
            const i32 parent = nodes[*it].parent;
            global[*it] = (parent >= 0 && done[static_cast<usize>(parent)])
                              ? global[static_cast<usize>(parent)] * local[*it]
                              : (parent >= 0 ? global[static_cast<usize>(parent)] * local[*it]
                                             : local[*it]);
            done[*it] = true;
        }
    }

    out.resize(joint_nodes.size());
    for (usize j = 0; j < joint_nodes.size(); ++j) {
        out[j] = global[joint_nodes[j]] * inverse_bind[j];
    }
}

void SkinnedModel::skin(const std::vector<Mat4>& joint_matrices, std::vector<Vertex3D>& out) const
{
    out.resize(mesh.vertices.size());
    for (usize i = 0; i < mesh.vertices.size(); ++i) {
        const Vertex3D& src = mesh.vertices[i];
        Vec3 pos{};
        Vec3 nrm{};
        for (usize k = 0; k < 4; ++k) {
            const f32 w = weights[i][k];
            if (w <= 0.0f) {
                continue;
            }
            const Mat4& jm = joint_matrices[joints[i][k]];
            pos = pos + transform_point(jm, src.position) * w;
            const Vec3 origin = transform_point(jm, Vec3{});
            nrm = nrm + (transform_point(jm, src.normal) - origin) * w;
        }
        out[i] = src;
        out[i].position = pos;
        out[i].normal = normalize(nrm);
    }
}

std::expected<SkinnedModel, GlbError> load_glb_skinned(std::string_view path)
{
    std::ifstream file{std::string(path), std::ios::binary};
    if (!file.is_open()) {
        return std::unexpected(GlbError::FileNotFound);
    }
    std::vector<u8> raw((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    if (raw.size() < 20 || std::memcmp(raw.data(), "glTF", 4) != 0) {
        return std::unexpected(GlbError::BadHeader);
    }

    json gltf;
    std::vector<u8> bin;
    usize cursor = 12;
    while (cursor + 8 <= raw.size()) {
        u32 len = 0;
        u32 type = 0;
        std::memcpy(&len, raw.data() + cursor, 4);
        std::memcpy(&type, raw.data() + cursor + 4, 4);
        cursor += 8;
        if (cursor + len > raw.size()) {
            break;
        }
        if (type == 0x4E4F534A) {
            gltf = json::parse(raw.begin() + static_cast<long>(cursor),
                               raw.begin() + static_cast<long>(cursor + len), nullptr, false);
        } else if (type == 0x004E4942) {
            bin.assign(raw.begin() + static_cast<long>(cursor),
                       raw.begin() + static_cast<long>(cursor + len));
        }
        cursor += len;
    }
    if (gltf.is_discarded() || gltf.is_null()) {
        return std::unexpected(GlbError::BadJson);
    }

    SkinnedModel model;

    // ── Nodes (rest pose + parents) ──────────────────────────────────
    const json& jnodes = gltf["nodes"];
    model.nodes.resize(jnodes.size());
    for (usize n = 0; n < jnodes.size(); ++n) {
        const json& jn = jnodes[n];
        SkinNode& node = model.nodes[n];
        if (jn.contains("translation")) {
            node.translation = {jn["translation"][0], jn["translation"][1], jn["translation"][2]};
        }
        if (jn.contains("rotation")) {
            node.rotation = {jn["rotation"][0], jn["rotation"][1], jn["rotation"][2],
                             jn["rotation"][3]};
        }
        if (jn.contains("scale")) {
            node.scale = {jn["scale"][0], jn["scale"][1], jn["scale"][2]};
        }
        if (jn.contains("children")) {
            for (const json& c : jn["children"]) {
                model.nodes[c.get<usize>()].parent = static_cast<i32>(n);
            }
        }
    }

    // ── Skin (first skin) ────────────────────────────────────────────
    if (!gltf.contains("skins") || gltf["skins"].empty()) {
        return std::unexpected(GlbError::NoGeometry);
    }
    const json& skin = gltf["skins"][0];
    for (const json& j : skin["joints"]) {
        model.joint_nodes.push_back(j.get<u32>());
    }
    if (skin.contains("inverseBindMatrices")) {
        const AccessorView ibm = view_accessor(gltf, bin, skin["inverseBindMatrices"], 16);
        for (usize i = 0; i < ibm.count; ++i) {
            model.inverse_bind.push_back(read_mat4(ibm, i));
        }
    } else {
        model.inverse_bind.assign(model.joint_nodes.size(), Mat4::identity());
    }

    // ── Skinned geometry (raw bind-pose, with joints/weights) ────────
    for (const json& mesh : gltf["meshes"]) {
        for (const json& prim : mesh["primitives"]) {
            if (!prim.contains("attributes")) {
                continue;
            }
            const json& attrs = prim["attributes"];
            if (!attrs.contains("POSITION") || !attrs.contains("JOINTS_0")
                || !attrs.contains("WEIGHTS_0")) {
                continue;
            }
            const AccessorView pos = view_accessor(gltf, bin, attrs["POSITION"], 3);
            const AccessorView jnt = view_accessor(gltf, bin, attrs["JOINTS_0"], 4);
            const AccessorView wgt = view_accessor(gltf, bin, attrs["WEIGHTS_0"], 4);
            const bool has_normal = attrs.contains("NORMAL");
            const bool has_uv = attrs.contains("TEXCOORD_0");
            const AccessorView nrm = has_normal ? view_accessor(gltf, bin, attrs["NORMAL"], 3)
                                                : AccessorView{};
            const AccessorView uv = has_uv ? view_accessor(gltf, bin, attrs["TEXCOORD_0"], 2)
                                           : AccessorView{};

            Vec3 color{1, 1, 1};
            if (prim.contains("material")) {
                const json& mat = gltf["materials"][prim["material"].get<usize>()];
                if (mat.contains("pbrMetallicRoughness")
                    && mat["pbrMetallicRoughness"].contains("baseColorFactor")) {
                    const json& c = mat["pbrMetallicRoughness"]["baseColorFactor"];
                    color = {c[0], c[1], c[2]};
                }
            }

            const u32 base = static_cast<u32>(model.mesh.vertices.size());
            for (usize i = 0; i < pos.count; ++i) {
                Vertex3D v;
                v.position = read_vec3(pos, i);
                v.normal = has_normal ? read_vec3(nrm, i) : Vec3{0, 1, 0};
                if (has_uv) {
                    v.uv = read_vec2(uv, i);
                }
                v.color = color;
                model.mesh.vertices.push_back(v);

                model.joints.push_back({static_cast<u16>(read_uint_comp(jnt, i, 0)),
                                        static_cast<u16>(read_uint_comp(jnt, i, 1)),
                                        static_cast<u16>(read_uint_comp(jnt, i, 2)),
                                        static_cast<u16>(read_uint_comp(jnt, i, 3))});
                const auto* wf = reinterpret_cast<const float*>(wgt.data + i * wgt.stride);
                model.weights.push_back({wf[0], wf[1], wf[2], wf[3]});
            }
            if (prim.contains("indices")) {
                const AccessorView idx = view_accessor(gltf, bin, prim["indices"], 1);
                for (usize i = 0; i < idx.count; ++i) {
                    model.mesh.indices.push_back(base + read_uint_comp(idx, i, 0));
                }
            }
        }
    }
    if (model.mesh.vertices.empty()) {
        return std::unexpected(GlbError::NoGeometry);
    }

    // ── Animations ───────────────────────────────────────────────────
    if (gltf.contains("animations")) {
        for (const json& anim : gltf["animations"]) {
            SkinClip clip;
            clip.name = anim.value("name", std::string("clip"));
            clip.track_of_node.assign(model.nodes.size(), -1);

            auto track_for = [&](usize node) -> NodeTrack& {
                if (clip.track_of_node[node] < 0) {
                    clip.track_of_node[node] = static_cast<i32>(clip.tracks.size());
                    clip.tracks.emplace_back();
                }
                return clip.tracks[static_cast<usize>(clip.track_of_node[node])];
            };

            for (const json& channel : anim["channels"]) {
                const json& target = channel["target"];
                if (!target.contains("node")) {
                    continue;
                }
                const usize node = target["node"].get<usize>();
                const std::string channel_path = target["path"].get<std::string>();
                const json& sampler = anim["samplers"][channel["sampler"].get<usize>()];
                const AccessorView input = view_accessor(gltf, bin, sampler["input"], 1);
                const usize out_comps = channel_path == "rotation" ? 4 : 3;
                const AccessorView output =
                    view_accessor(gltf, bin, sampler["output"], out_comps);

                NodeTrack& track = track_for(node);
                for (usize i = 0; i < input.count; ++i) {
                    const f32 time = read_scalar(input, i);
                    clip.duration = std::max(clip.duration, time);
                    if (channel_path == "translation") {
                        track.t_times.push_back(time);
                        track.t_values.push_back(read_vec3(output, i));
                    } else if (channel_path == "rotation") {
                        track.r_times.push_back(time);
                        track.r_values.push_back(read_quat(output, i));
                    } else if (channel_path == "scale") {
                        track.s_times.push_back(time);
                        track.s_values.push_back(read_vec3(output, i));
                    }
                }
            }
            model.clips.push_back(std::move(clip));
        }
    }

    // ── First embedded texture ───────────────────────────────────────
    if (gltf.contains("images") && !gltf["images"].empty()
        && gltf["images"][0].contains("bufferView")) {
        const json& bv = gltf["bufferViews"][gltf["images"][0]["bufferView"].get<usize>()];
        const usize offset = bv.value("byteOffset", usize(0));
        const usize length = bv.value("byteLength", usize(0));
        int w = 0;
        int h = 0;
        int n = 0;
        stbi_uc* pixels = stbi_load_from_memory(bin.data() + offset, static_cast<int>(length),
                                                &w, &h, &n, STBI_rgb_alpha);
        if (pixels != nullptr) {
            model.texture.width = static_cast<u32>(w);
            model.texture.height = static_cast<u32>(h);
            model.texture.pixels.assign(pixels,
                                        pixels + static_cast<usize>(w) * static_cast<usize>(h) * 4);
            stbi_image_free(pixels);
            model.has_texture = true;
        }
    }

    return model;
}

} // namespace vyro

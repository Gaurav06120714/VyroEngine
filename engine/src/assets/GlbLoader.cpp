// VyroEngine — GLB loader implementation
#include "vyro/assets/GlbLoader.hpp"

#include "vyro/math/Mat4.hpp"

#include <nlohmann/json.hpp>
#include <stb_image.h>

#include <cmath>
#include <cstring>
#include <fstream>
#include <vector>

namespace vyro {

namespace {

using nlohmann::json;

struct Chunks {
    json gltf;
    std::vector<u8> bin;
};

// glTF componentType constants.
constexpr u32 kUShort = 5123;
constexpr u32 kUInt = 5125;
constexpr u32 kFloat = 5126;

struct AccessorView {
    const u8* data = nullptr;
    usize count = 0;
    usize stride = 0; // bytes between elements
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

u32 read_index(const AccessorView& v, usize i)
{
    const u8* p = v.data + i * v.stride;
    if (v.component_type == kUInt) {
        u32 value = 0;
        std::memcpy(&value, p, 4);
        return value;
    }
    if (v.component_type == kUShort) {
        u16 value = 0;
        std::memcpy(&value, p, 2);
        return value;
    }
    return *p; // unsigned byte
}

// Node-local transform (matrix or TRS). Rotation uses the quaternion directly.
Mat4 node_transform(const json& node)
{
    if (node.contains("matrix")) {
        Mat4 m;
        for (usize i = 0; i < 16; ++i) {
            m.data[i] = node["matrix"][i].get<f32>();
        }
        return m;
    }
    Mat4 m = Mat4::identity();
    if (node.contains("scale")) {
        m = Mat4::scale({node["scale"][0], node["scale"][1], node["scale"][2]}) * m;
    }
    if (node.contains("rotation")) {
        const f32 x = node["rotation"][0];
        const f32 y = node["rotation"][1];
        const f32 z = node["rotation"][2];
        const f32 w = node["rotation"][3];
        Mat4 r = Mat4::identity();
        r.at(0, 0) = 1 - 2 * (y * y + z * z);
        r.at(0, 1) = 2 * (x * y - z * w);
        r.at(0, 2) = 2 * (x * z + y * w);
        r.at(1, 0) = 2 * (x * y + z * w);
        r.at(1, 1) = 1 - 2 * (x * x + z * z);
        r.at(1, 2) = 2 * (y * z - x * w);
        r.at(2, 0) = 2 * (x * z - y * w);
        r.at(2, 1) = 2 * (y * z + x * w);
        r.at(2, 2) = 1 - 2 * (x * x + y * y);
        m = r * m;
    }
    if (node.contains("translation")) {
        m = Mat4::translation({node["translation"][0], node["translation"][1],
                               node["translation"][2]})
            * m;
    }
    return m;
}

Vec3 material_color(const json& gltf, const json& prim)
{
    if (!prim.contains("material")) {
        return {1, 1, 1};
    }
    const json& mat = gltf["materials"][prim["material"].get<usize>()];
    if (mat.contains("pbrMetallicRoughness")
        && mat["pbrMetallicRoughness"].contains("baseColorFactor")) {
        const json& c = mat["pbrMetallicRoughness"]["baseColorFactor"];
        return {c[0], c[1], c[2]};
    }
    return {1, 1, 1};
}

void append_primitive(const json& gltf, const std::vector<u8>& bin, const json& prim,
                      const Mat4& world, MeshData& out)
{
    if (!prim.contains("attributes") || !prim["attributes"].contains("POSITION")) {
        return;
    }
    const json& attrs = prim["attributes"];
    const AccessorView pos = view_accessor(gltf, bin, attrs["POSITION"], 3);
    const bool has_normal = attrs.contains("NORMAL");
    const bool has_uv = attrs.contains("TEXCOORD_0");
    const AccessorView nrm = has_normal ? view_accessor(gltf, bin, attrs["NORMAL"], 3)
                                        : AccessorView{};
    const AccessorView uv = has_uv ? view_accessor(gltf, bin, attrs["TEXCOORD_0"], 2)
                                   : AccessorView{};
    const Vec3 color = material_color(gltf, prim);

    const u32 base = static_cast<u32>(out.vertices.size());
    for (usize i = 0; i < pos.count; ++i) {
        Vertex3D v;
        v.position = transform_point(world, read_vec3(pos, i));
        if (has_normal) {
            // Rotate the normal (translation is irrelevant; assume uniform scale).
            const Vec3 n = read_vec3(nrm, i);
            const Vec3 origin = transform_point(world, Vec3{});
            v.normal = normalize(transform_point(world, n) - origin);
        } else {
            v.normal = {0, 1, 0};
        }
        if (has_uv) {
            v.uv = read_vec2(uv, i);
        }
        v.color = color;
        out.vertices.push_back(v);
    }

    if (prim.contains("indices")) {
        const AccessorView idx = view_accessor(gltf, bin, prim["indices"], 1);
        for (usize i = 0; i < idx.count; ++i) {
            out.indices.push_back(base + read_index(idx, i));
        }
    } else {
        for (usize i = 0; i < pos.count; ++i) {
            out.indices.push_back(base + static_cast<u32>(i));
        }
    }
}

void walk_node(const json& gltf, const std::vector<u8>& bin, usize node_index, const Mat4& parent,
               MeshData& out)
{
    const json& node = gltf["nodes"][node_index];
    const Mat4 world = parent * node_transform(node);

    if (node.contains("mesh")) {
        const json& mesh = gltf["meshes"][node["mesh"].get<usize>()];
        for (const json& prim : mesh["primitives"]) {
            append_primitive(gltf, bin, prim, world, out);
        }
    }
    if (node.contains("children")) {
        for (const json& child : node["children"]) {
            walk_node(gltf, bin, child.get<usize>(), world, out);
        }
    }
}

Image first_texture(const json& gltf, const std::vector<u8>& bin, bool& found)
{
    found = false;
    if (!gltf.contains("images") || gltf["images"].empty()) {
        return {};
    }
    const json& img = gltf["images"][0];
    if (!img.contains("bufferView")) {
        return {};
    }
    const json& bv = gltf["bufferViews"][img["bufferView"].get<usize>()];
    const usize offset = bv.value("byteOffset", usize(0));
    const usize length = bv.value("byteLength", usize(0));

    int w = 0;
    int h = 0;
    int n = 0;
    stbi_uc* pixels = stbi_load_from_memory(bin.data() + offset, static_cast<int>(length), &w, &h,
                                            &n, STBI_rgb_alpha);
    if (pixels == nullptr) {
        return {};
    }
    Image out;
    out.width = static_cast<u32>(w);
    out.height = static_cast<u32>(h);
    out.pixels.assign(pixels, pixels + static_cast<usize>(w) * static_cast<usize>(h) * 4);
    stbi_image_free(pixels);
    found = true;
    return out;
}

} // namespace

std::expected<GlbModel, GlbError> load_glb(std::string_view path)
{
    std::ifstream file{std::string(path), std::ios::binary};
    if (!file.is_open()) {
        return std::unexpected(GlbError::FileNotFound);
    }
    std::vector<u8> raw((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());

    // GLB container: 12-byte header, then chunks (JSON first, BIN second).
    if (raw.size() < 20 || std::memcmp(raw.data(), "glTF", 4) != 0) {
        return std::unexpected(GlbError::BadHeader);
    }

    Chunks chunks;
    usize cursor = 12;
    while (cursor + 8 <= raw.size()) {
        u32 chunk_len = 0;
        u32 chunk_type = 0;
        std::memcpy(&chunk_len, raw.data() + cursor, 4);
        std::memcpy(&chunk_type, raw.data() + cursor + 4, 4);
        cursor += 8;
        if (cursor + chunk_len > raw.size()) {
            break;
        }
        if (chunk_type == 0x4E4F534A) { // 'JSON'
            chunks.gltf = json::parse(raw.begin() + static_cast<long>(cursor),
                                      raw.begin() + static_cast<long>(cursor + chunk_len),
                                      nullptr, false);
        } else if (chunk_type == 0x004E4942) { // 'BIN'
            chunks.bin.assign(raw.begin() + static_cast<long>(cursor),
                              raw.begin() + static_cast<long>(cursor + chunk_len));
        }
        cursor += chunk_len;
    }
    if (chunks.gltf.is_discarded() || chunks.gltf.is_null()) {
        return std::unexpected(GlbError::BadJson);
    }

    GlbModel model;
    const usize scene_index = chunks.gltf.value("scene", usize(0));
    if (chunks.gltf.contains("scenes")
        && chunks.gltf["scenes"][scene_index].contains("nodes")) {
        for (const json& root : chunks.gltf["scenes"][scene_index]["nodes"]) {
            walk_node(chunks.gltf, chunks.bin, root.get<usize>(), Mat4::identity(), model.mesh);
        }
    }
    if (model.mesh.vertices.empty()) {
        return std::unexpected(GlbError::NoGeometry);
    }

    model.texture = first_texture(chunks.gltf, chunks.bin, model.has_texture);
    return model;
}

} // namespace vyro

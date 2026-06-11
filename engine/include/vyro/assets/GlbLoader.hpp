// VyroEngine — GLB (binary glTF) loader
// Loads static geometry from .glb files: positions, normals, UVs, indices,
// node transforms, material base colors, and the first embedded texture
// (decoded via stb_image). Skinned meshes load in their bind pose; animation
// playback is a future milestone — this gets real characters and props from
// asset stores rendering today.
#pragma once

#include "vyro/assets/Image.hpp"
#include "vyro/assets/Mesh.hpp"

#include <expected>
#include <string_view>

namespace vyro {

enum class GlbError {
    FileNotFound,
    BadHeader,
    BadJson,
    NoGeometry,
};

struct GlbModel {
    MeshData mesh;
    Image texture;     // first embedded image, or empty
    bool has_texture = false;
};

[[nodiscard]] std::expected<GlbModel, GlbError> load_glb(std::string_view path);

} // namespace vyro

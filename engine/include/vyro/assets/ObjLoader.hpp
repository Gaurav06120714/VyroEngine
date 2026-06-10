// VyroEngine — Wavefront OBJ loader
// Loads .obj geometry (positions, texture coords, normals, triangulated faces)
// into a MeshData. Missing normals are generated from face geometry. A minimal,
// dependency-free importer for the model showcase; a full pipeline (Assimp /
// glTF) layers in later behind the same MeshData output.
#pragma once

#include "vyro/assets/Mesh.hpp"

#include <expected>
#include <string>
#include <string_view>

namespace vyro {

enum class ObjError {
    FileNotFound,
    Empty,
};

// Load an OBJ file from disk.
[[nodiscard]] std::expected<MeshData, ObjError> load_obj(std::string_view path);

// Parse OBJ text directly (used by the file loader and tests).
[[nodiscard]] std::expected<MeshData, ObjError> parse_obj(std::string_view text);

} // namespace vyro

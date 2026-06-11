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
#include <unordered_map>

namespace vyro {

enum class ObjError {
    FileNotFound,
    Empty,
};

// Material name -> diffuse color (Kd), parsed from a .mtl library.
using MaterialMap = std::unordered_map<std::string, Vec3>;

// Parse .mtl text into material diffuse colors.
[[nodiscard]] MaterialMap parse_mtl(std::string_view text);

// Load an OBJ file from disk. If the file references an .mtl library
// (mtllib), it is loaded from the same directory and material diffuse colors
// are baked into vertex colors on each usemtl group.
[[nodiscard]] std::expected<MeshData, ObjError> load_obj(std::string_view path);

// Parse OBJ text directly (used by the file loader and tests).
[[nodiscard]] std::expected<MeshData, ObjError> parse_obj(std::string_view text,
                                                          const MaterialMap& materials = {});

} // namespace vyro

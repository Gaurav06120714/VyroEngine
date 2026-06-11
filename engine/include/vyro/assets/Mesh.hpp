// VyroEngine — 3D mesh data
// CPU-side mesh: interleaved vertices (position, normal, uv) and an index
// buffer. Produced by the OBJ loader or procedural generators, then uploaded
// to the GPU via the RHI.
#pragma once

#include "vyro/core/Types.hpp"
#include "vyro/math/Vec.hpp"

#include <vector>

namespace vyro {

struct Vertex3D {
    Vec3 position{};
    Vec3 normal{};
    Vec2 uv{};
    Vec3 color{1.0f, 1.0f, 1.0f}; // material diffuse, baked at load time
};

struct MeshData {
    std::vector<Vertex3D> vertices;
    std::vector<u32> indices;

    [[nodiscard]] usize vertex_bytes() const { return vertices.size() * sizeof(Vertex3D); }
    [[nodiscard]] usize index_bytes() const { return indices.size() * sizeof(u32); }
    [[nodiscard]] bool empty() const { return vertices.empty(); }
};

// A unit cube centered at the origin with per-face normals (procedural
// fallback used when no model file is supplied).
[[nodiscard]] MeshData make_cube(f32 size = 1.0f);

} // namespace vyro

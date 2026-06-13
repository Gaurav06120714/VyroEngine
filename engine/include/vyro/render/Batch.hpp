// VyroEngine — Draw-call batching (V5.5)
// Merges many instances of one mesh — each with its own model matrix — into a
// single vertex/index buffer so a whole horde draws in one call instead of N.
// Positions and normals are baked into world space; indices are rebased per
// instance. Headless and unit-tested. (A future RHI addition could push this
// onto the GPU with hardware instancing; this is the CPU batch.)
#pragma once

#include "vyro/assets/Mesh.hpp"
#include "vyro/math/Mat4.hpp"

#include <span>

namespace vyro {

// Append `mesh` transformed by each matrix in `transforms` into `out`.
inline void batch_transform(const MeshData& mesh, std::span<const Mat4> transforms, MeshData& out)
{
    out.vertices.clear();
    out.indices.clear();
    out.vertices.reserve(mesh.vertices.size() * transforms.size());
    out.indices.reserve(mesh.indices.size() * transforms.size());

    for (const Mat4& m : transforms) {
        const u32 base = static_cast<u32>(out.vertices.size());
        const Vec3 origin = transform_point(m, Vec3{}); // for rotating normals
        for (const Vertex3D& v : mesh.vertices) {
            Vertex3D out_v = v;
            out_v.position = transform_point(m, v.position);
            out_v.normal = normalize(transform_point(m, v.normal) - origin);
            out.vertices.push_back(out_v);
        }
        for (const u32 idx : mesh.indices) {
            out.indices.push_back(base + idx);
        }
    }
}

} // namespace vyro

// VyroEngine — Procedural mesh generation
#include "vyro/assets/Mesh.hpp"

namespace vyro {

MeshData make_cube(f32 size)
{
    const f32 h = size * 0.5f;
    MeshData mesh;

    // Six faces, each with its own normal and four corner vertices.
    struct Face {
        Vec3 normal;
        Vec3 corners[4];
    };
    const Face faces[6] = {
        {{0, 0, 1}, {{-h, -h, h}, {h, -h, h}, {h, h, h}, {-h, h, h}}},     // +Z
        {{0, 0, -1}, {{h, -h, -h}, {-h, -h, -h}, {-h, h, -h}, {h, h, -h}}}, // -Z
        {{1, 0, 0}, {{h, -h, h}, {h, -h, -h}, {h, h, -h}, {h, h, h}}},      // +X
        {{-1, 0, 0}, {{-h, -h, -h}, {-h, -h, h}, {-h, h, h}, {-h, h, -h}}}, // -X
        {{0, 1, 0}, {{-h, h, h}, {h, h, h}, {h, h, -h}, {-h, h, -h}}},      // +Y
        {{0, -1, 0}, {{-h, -h, -h}, {h, -h, -h}, {h, -h, h}, {-h, -h, h}}}, // -Y
    };

    const Vec2 uvs[4] = {{0, 0}, {1, 0}, {1, 1}, {0, 1}};
    for (const Face& face : faces) {
        const u32 base = static_cast<u32>(mesh.vertices.size());
        for (int i = 0; i < 4; ++i) {
            mesh.vertices.push_back(Vertex3D{face.corners[i], face.normal, uvs[i]});
        }
        // Two triangles per face.
        mesh.indices.push_back(base + 0);
        mesh.indices.push_back(base + 1);
        mesh.indices.push_back(base + 2);
        mesh.indices.push_back(base + 0);
        mesh.indices.push_back(base + 2);
        mesh.indices.push_back(base + 3);
    }
    return mesh;
}

} // namespace vyro

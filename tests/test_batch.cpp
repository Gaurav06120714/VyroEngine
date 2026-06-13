// VyroEngine — Draw-call batching tests (V5.5)
// Batching N instances of a mesh yields one buffer with N*verts and N*indices,
// vertices baked into world space and indices correctly rebased per instance.
#include "vyro/render/Batch.hpp"

#include "test_harness.hpp"

#include <cmath>
#include <vector>

namespace {
bool approx(vyro::f32 a, vyro::f32 b, vyro::f32 eps = 1e-4f) { return std::fabs(a - b) < eps; }
} // namespace

int main()
{
    using namespace vyro;
    test::Suite suite("batch");

    // A 3-vertex triangle.
    MeshData tri;
    tri.vertices = {
        {{0, 0, 0}, {0, 1, 0}, {0, 0}, {1, 1, 1}},
        {{1, 0, 0}, {0, 1, 0}, {0, 0}, {1, 1, 1}},
        {{0, 0, 1}, {0, 1, 0}, {0, 0}, {1, 1, 1}},
    };
    tri.indices = {0, 1, 2};

    const std::vector<Mat4> xforms = {
        Mat4::translation({10, 0, 0}),
        Mat4::translation({0, 0, -5}),
    };

    MeshData out;
    batch_transform(tri, xforms, out);

    suite.check(out.vertices.size() == tri.vertices.size() * xforms.size(),
                "batched vertex count is verts * instances");
    suite.check(out.indices.size() == tri.indices.size() * xforms.size(),
                "batched index count is indices * instances");

    // First instance translated +10 on x.
    suite.check(approx(out.vertices[0].position.x, 10.0f), "instance 0 vertex translated");
    // Second instance's first vertex translated -5 on z, and its indices rebased
    // to start at the second block (vertex 3).
    suite.check(approx(out.vertices[3].position.z, -5.0f), "instance 1 vertex translated");
    suite.check(out.indices[3] == 3 && out.indices[4] == 4 && out.indices[5] == 5,
                "instance 1 indices rebased past instance 0");

    // Normals survive a pure translation (no rotation).
    suite.check(approx(out.vertices[0].normal.y, 1.0f), "normal preserved under translation");

    return suite.summary();
}

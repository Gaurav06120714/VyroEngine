// VyroEngine — Hardware instancing layout tests (V6.1)
// Verifies the per-instance buffer contract the instanced draw path relies on:
// tight Mat4 stride and column-major attribute extraction.
#include "vyro/render/Instancing.hpp"

#include "test_harness.hpp"

#include <cmath>

namespace {
bool approx(vyro::f32 a, vyro::f32 b, vyro::f32 eps = 1e-4f) { return std::fabs(a - b) < eps; }
} // namespace

int main()
{
    using namespace vyro;
    test::Suite suite("instancing");

    // Stride and buffer sizing.
    suite.check(instancing::kInstanceStride == sizeof(Mat4), "instance stride is one Mat4");
    suite.check(instancing::buffer_bytes(5) == 5 * sizeof(Mat4), "buffer bytes scale with count");
    suite.check(instancing::buffer_bytes(0) == 0, "empty instance set needs no bytes");

    // Column extraction matches Mat4's column-major (row r, col c) = data[c*4+r].
    {
        const Mat4 t = Mat4::translation(Vec3{3, 4, 5});
        const Vec4 c3 = instancing::column(t, 3); // translation lives in column 3
        suite.check(approx(c3.x, 3.0f) && approx(c3.y, 4.0f) && approx(c3.z, 5.0f)
                        && approx(c3.w, 1.0f),
                    "column 3 carries the translation");
        const Vec4 c0 = instancing::column(t, 0);
        suite.check(approx(c0.x, 1.0f) && approx(c0.y, 0.0f), "column 0 is the x basis");
    }

    // Columns reconstruct the matrix elements (row r, col c).
    {
        const Mat4 m = Mat4::scale(Vec3{2, 3, 4});
        suite.check(approx(instancing::column(m, 0).x, m.at(0, 0)), "col0.x == m(0,0)");
        suite.check(approx(instancing::column(m, 1).y, m.at(1, 1)), "col1.y == m(1,1)");
        suite.check(approx(instancing::column(m, 2).z, m.at(2, 2)), "col2.z == m(2,2)");
    }

    return suite.summary();
}

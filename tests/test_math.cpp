// VyroEngine — Math tests
#include "vyro/math/Mat4.hpp"
#include "vyro/math/Vec.hpp"

#include "test_harness.hpp"

#include <cmath>

namespace {

bool approx(vyro::f32 a, vyro::f32 b, vyro::f32 eps = 1e-4f)
{
    return std::fabs(a - b) < eps;
}

bool approx(vyro::Vec3 a, vyro::Vec3 b)
{
    return approx(a.x, b.x) && approx(a.y, b.y) && approx(a.z, b.z);
}

} // namespace

int main()
{
    using namespace vyro;
    test::Suite suite("math");

    // Vec3_DotCross_AreCorrect
    suite.check(approx(dot(Vec3{1, 2, 3}, Vec3{4, 5, 6}), 32.0f), "dot product");
    suite.check(cross(Vec3{1, 0, 0}, Vec3{0, 1, 0}) == Vec3{0, 0, 1}, "cross x*y=z");
    suite.check(approx(length(Vec3{3, 4, 0}), 5.0f), "length 3-4-5");
    suite.check(approx(length(normalize(Vec3{0, 5, 0})), 1.0f), "normalize unit length");

    // Mat4_Identity_IsNeutral
    {
        const Mat4 id = Mat4::identity();
        const Vec3 p{2, 3, 4};
        suite.check(approx(transform_point(id, p), p), "identity leaves point unchanged");
    }

    // Mat4_Translation_MovesPoint
    {
        const Mat4 t = Mat4::translation(Vec3{10, 0, -5});
        suite.check(approx(transform_point(t, Vec3{1, 1, 1}), Vec3{11, 1, -4}),
                    "translation offsets point");
    }

    // Mat4_Scale_ScalesPoint
    {
        const Mat4 s = Mat4::scale(Vec3{2, 3, 4});
        suite.check(approx(transform_point(s, Vec3{1, 1, 1}), Vec3{2, 3, 4}), "scale point");
    }

    // Mat4_Multiply_ComposesTransforms (translate then scale)
    {
        const Mat4 t = Mat4::translation(Vec3{1, 0, 0});
        const Mat4 s = Mat4::scale(Vec3{2, 2, 2});
        // Apply scale first, then translate: (T * S) * p
        const Mat4 ts = t * s;
        suite.check(approx(transform_point(ts, Vec3{1, 1, 1}), Vec3{3, 2, 2}),
                    "composed scale-then-translate");
    }

    // Mat4_Rotation_RotatesAboutZ (90 deg maps +x to +y)
    {
        const Mat4 r = Mat4::rotation(Vec3{0, 0, 1}, 3.14159265f * 0.5f);
        suite.check(approx(transform_point(r, Vec3{1, 0, 0}), Vec3{0, 1, 0}),
                    "90deg z-rotation maps +x to +y");
    }

    // Mat4_LookAt_PlacesEyeAtOrigin
    {
        const Mat4 v = Mat4::look_at(Vec3{0, 0, 5}, Vec3{0, 0, 0}, Vec3{0, 1, 0});
        // The eye position should map to the view-space origin.
        suite.check(approx(transform_point(v, Vec3{0, 0, 5}), Vec3{0, 0, 0}),
                    "look_at maps eye to view origin");
    }

    return suite.summary();
}

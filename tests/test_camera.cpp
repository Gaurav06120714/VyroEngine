// VyroEngine — Camera tests
#include "vyro/render/Camera.hpp"

#include "test_harness.hpp"

#include <cmath>

namespace {

bool approx(vyro::f32 a, vyro::f32 b, vyro::f32 eps = 1e-4f) { return std::fabs(a - b) < eps; }

} // namespace

int main()
{
    vyro::test::Suite suite("camera");

    // Camera_SetView_PlacesEyeAtViewOrigin
    {
        vyro::Camera cam;
        cam.set_view(vyro::Vec3{0, 0, 10}, vyro::Vec3{0, 0, 0});
        suite.check(cam.position() == (vyro::Vec3{0, 0, 10}), "position stored");
        const vyro::Vec3 eye_in_view = vyro::transform_point(cam.view(), vyro::Vec3{0, 0, 10});
        suite.check(approx(eye_in_view.x, 0) && approx(eye_in_view.y, 0) && approx(eye_in_view.z, 0),
                    "eye maps to view-space origin");
    }

    // Camera_Orthographic_MapsCornersToNdc
    {
        vyro::Camera cam;
        cam.set_orthographic(-10, 10, -10, 10, -1, 1);
        suite.check(cam.projection_type() == vyro::ProjectionType::Orthographic, "type is ortho");
        const vyro::Vec3 corner = vyro::transform_point(cam.projection(), vyro::Vec3{10, 10, 0});
        suite.check(approx(corner.x, 1.0f) && approx(corner.y, 1.0f),
                    "right-top corner maps to NDC (1,1)");
    }

    // Camera_Perspective_SetsType
    {
        vyro::Camera cam;
        cam.set_perspective(1.0472f /*60deg*/, 16.0f / 9.0f, 0.1f, 100.0f);
        suite.check(cam.projection_type() == vyro::ProjectionType::Perspective, "type is perspective");
    }

    // Camera_ViewProjection_ComposesProjectionTimesView
    {
        vyro::Camera cam;
        cam.set_orthographic(-1, 1, -1, 1, -1, 1);
        cam.set_view(vyro::Vec3{0, 0, 1}, vyro::Vec3{0, 0, 0});
        const vyro::Mat4 expected = cam.projection() * cam.view();
        suite.check(cam.view_projection() == expected, "view_projection == projection * view");
    }

    return suite.summary();
}

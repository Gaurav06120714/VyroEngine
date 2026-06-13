// VyroEngine — Frustum culling tests (V5.3)
// Planes extracted from a view-projection correctly classify points/volumes
// that are in front of, behind, and to the side of the camera.
#include "vyro/render/Frustum.hpp"

#include "test_harness.hpp"

int main()
{
    using namespace vyro;
    test::Suite suite("frustum");

    // Camera at origin looking down -z (right-handed), 90° fov.
    const Mat4 view = Mat4::look_at(Vec3{0, 0, 0}, Vec3{0, 0, -1}, Vec3{0, 1, 0});
    const Mat4 proj = Mat4::perspective(1.5708f, 1.0f, 0.1f, 100.0f);
    const Frustum f = frustum_from_view_projection(proj * view);

    // A point straight ahead is visible; one behind the camera is not.
    suite.check(intersects_sphere(f, Vec3{0, 0, -10}, 0.5f), "point ahead is visible");
    suite.check(!intersects_sphere(f, Vec3{0, 0, 10}, 0.5f), "point behind is culled");

    // Far past the far plane is culled; very near in front passes.
    suite.check(!intersects_sphere(f, Vec3{0, 0, -500}, 1.0f), "beyond far plane is culled");
    suite.check(intersects_sphere(f, Vec3{0, 0, -1}, 0.5f), "near point in front is visible");

    // Way off to the side (outside the horizontal fov) is culled.
    suite.check(!intersects_sphere(f, Vec3{100, 0, -10}, 0.5f), "far to the side is culled");

    // A large radius rescues an otherwise-outside center (touches the frustum).
    suite.check(intersects_sphere(f, Vec3{0, 0, 8}, 12.0f),
                "big sphere straddling the camera still intersects");

    // AABB tests: a box in front is visible, a box behind is culled.
    suite.check(intersects_aabb(f, Vec3{-1, -1, -11}, Vec3{1, 1, -9}), "box ahead is visible");
    suite.check(!intersects_aabb(f, Vec3{-1, -1, 9}, Vec3{1, 1, 11}), "box behind is culled");

    return suite.summary();
}

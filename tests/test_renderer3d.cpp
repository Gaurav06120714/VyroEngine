// VyroEngine — 3D renderer tests
#include "vyro/render/Camera.hpp"
#include "vyro/render/NullDevice.hpp"
#include "vyro/render/Renderer3D.hpp"

#include "test_harness.hpp"

int main()
{
    vyro::test::Suite suite("renderer3d");

    vyro::Camera cam;
    cam.set_perspective(1.0472f, 16.0f / 9.0f, 0.1f, 100.0f);
    cam.set_view(vyro::Vec3{0, 0, 5}, vyro::Vec3{0, 0, 0});

    // Renderer3D_SubmitMeshes_DrawsEach
    {
        vyro::NullDevice dev;
        auto shader = dev.create_shader({"v", "f"});
        auto vb = dev.create_buffer({vyro::BufferType::Vertex, 1024, nullptr});
        auto ib = dev.create_buffer({vyro::BufferType::Index, 512, nullptr});
        vyro::Renderer3D r3d(dev, shader);

        r3d.begin_scene(cam);
        vyro::MeshSubmission mesh;
        mesh.vertex_buffer = vb;
        mesh.index_buffer = ib;
        mesh.index_count = 36; // a cube
        mesh.model = vyro::Mat4::translation(vyro::Vec3{1, 0, 0});
        r3d.submit(mesh);
        r3d.submit(mesh);
        r3d.submit(mesh);
        r3d.end_scene();

        suite.check(r3d.stats().submitted == 3, "three meshes submitted");
        suite.check(r3d.stats().draw_calls == 3, "three draw calls issued");
        suite.check(dev.stats().draw_calls == 3, "device received three draws");
        suite.check(dev.stats().last_index_count == 36, "index count = 36 (cube)");
    }

    // Renderer3D_ViewProjection_TracksCamera
    {
        vyro::NullDevice dev;
        auto shader = dev.create_shader({"v", "f"});
        vyro::Renderer3D r3d(dev, shader);
        r3d.begin_scene(cam);
        suite.check(r3d.view_projection() == cam.view_projection(),
                    "renderer caches camera view-projection");
    }

    // Renderer3D_BeginScene_ResetsQueue
    {
        vyro::NullDevice dev;
        auto shader = dev.create_shader({"v", "f"});
        vyro::Renderer3D r3d(dev, shader);
        r3d.begin_scene(cam);
        r3d.submit(vyro::MeshSubmission{});
        r3d.end_scene();
        r3d.begin_scene(cam);
        r3d.end_scene();
        suite.check(r3d.stats().submitted == 0, "queue and stats reset each scene");
    }

    return suite.summary();
}

// VyroEngine — 2D renderer tests
#include "vyro/render/Camera.hpp"
#include "vyro/render/NullDevice.hpp"
#include "vyro/render/Renderer2D.hpp"

#include "test_harness.hpp"

int main()
{
    vyro::test::Suite suite("renderer2d");

    vyro::Camera cam;
    cam.set_orthographic(0, 800, 0, 600, -1, 1);

    // Renderer2D_DrawQuads_BatchesIntoOneDrawCall
    {
        vyro::NullDevice dev;
        auto shader = dev.create_shader({"v", "f"});
        vyro::Renderer2D r2d(dev, shader);

        r2d.begin_scene(cam);
        for (int i = 0; i < 10; ++i) {
            r2d.draw_quad(vyro::Vec2{static_cast<float>(i) * 10.0f, 0.0f},
                          vyro::Vec2{8.0f, 8.0f}, vyro::Vec4{1, 1, 1, 1});
        }
        r2d.end_scene();

        suite.check(r2d.stats().quad_count == 10, "ten quads drawn");
        suite.check(r2d.stats().draw_calls == 1, "ten quads batched into one draw call");
        suite.check(dev.stats().last_index_count == 10 * 6, "index count = quads * 6");
    }

    // Renderer2D_ExceedBatch_SplitsIntoMultipleDrawCalls
    {
        vyro::NullDevice dev;
        auto shader = dev.create_shader({"v", "f"});
        vyro::Renderer2D r2d(dev, shader);

        r2d.begin_scene(cam);
        const int count = vyro::Renderer2D::kMaxQuads + 1; // forces a second batch
        for (int i = 0; i < count; ++i) {
            r2d.draw_quad(vyro::Vec2{0, 0}, vyro::Vec2{1, 1}, vyro::Vec4{1, 0, 0, 1});
        }
        r2d.end_scene();

        suite.check(r2d.stats().quad_count == static_cast<vyro::u32>(count), "all quads counted");
        suite.check(r2d.stats().draw_calls == 2, "exceeding the limit splits into two batches");
    }

    // Renderer2D_EmptyScene_IssuesNoDraw
    {
        vyro::NullDevice dev;
        auto shader = dev.create_shader({"v", "f"});
        vyro::Renderer2D r2d(dev, shader);
        r2d.begin_scene(cam);
        r2d.end_scene();
        suite.check(r2d.stats().draw_calls == 0, "empty scene issues no draw call");
    }

    return suite.summary();
}

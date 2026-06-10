// VyroEngine — Renderer tests
#include "vyro/render/NullDevice.hpp"
#include "vyro/render/Renderer.hpp"

#include "test_harness.hpp"

int main()
{
    vyro::test::Suite suite("renderer");

    // Renderer_Frame_ClearsAndDraws
    {
        vyro::NullDevice dev;
        vyro::Renderer renderer(dev);
        auto sh = dev.create_shader({"v", "f"});
        auto vb = dev.create_buffer({vyro::BufferType::Vertex, 64, nullptr});

        renderer.begin_frame(vyro::Vec4{0, 0, 0, 1}, 800, 600);
        vyro::RenderItem item;
        item.shader = sh;
        item.vertex_buffer = vb;
        item.index_count = 3;
        renderer.submit(item);
        renderer.submit(item);
        renderer.end_frame();

        suite.check(renderer.last_frame().submitted == 2, "two items submitted");
        suite.check(renderer.last_frame().draw_calls == 2, "two draw calls issued");
        suite.check(dev.stats().draw_calls == 2, "device received two draws");
        suite.check(dev.stats().frames == 1, "device saw one frame");
    }

    // Renderer_Sort_GroupsStateToMinimizeChanges
    {
        vyro::NullDevice dev;
        vyro::Renderer renderer(dev);
        auto shaderA = dev.create_shader({"a", "a"});
        auto shaderB = dev.create_shader({"b", "b"});

        renderer.begin_frame(vyro::Vec4{}, 100, 100);
        // Interleave A,B,A,B but sort_key groups by shader (A keys < B keys).
        auto make = [](vyro::ShaderHandle sh, vyro::u64 key) {
            vyro::RenderItem it;
            it.shader = sh;
            it.sort_key = key;
            it.vertex_count = 3;
            return it;
        };
        renderer.submit(make(shaderA, 0));
        renderer.submit(make(shaderB, 100));
        renderer.submit(make(shaderA, 1));
        renderer.submit(make(shaderB, 101));
        renderer.end_frame();

        // After sorting: A,A,B,B -> exactly 2 shader binds.
        suite.check(renderer.last_frame().draw_calls == 4, "all four items drawn");
        suite.check(renderer.last_frame().state_changes == 2,
                    "sorting groups state to 2 shader binds");
    }

    // Renderer_BeginFrame_ResetsStats
    {
        vyro::NullDevice dev;
        vyro::Renderer renderer(dev);
        renderer.begin_frame(vyro::Vec4{}, 10, 10);
        renderer.submit(vyro::RenderItem{});
        renderer.end_frame();
        renderer.begin_frame(vyro::Vec4{}, 10, 10);
        renderer.end_frame();
        suite.check(renderer.last_frame().submitted == 0, "stats reset on new frame");
    }

    return suite.summary();
}

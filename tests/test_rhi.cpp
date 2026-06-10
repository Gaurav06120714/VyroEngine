// VyroEngine — RHI / NullDevice tests
#include "vyro/render/NullDevice.hpp"

#include "test_harness.hpp"

int main()
{
    vyro::test::Suite suite("rhi");

    // NullDevice_Backend_IsNull
    {
        vyro::NullDevice dev;
        suite.check(dev.backend() == vyro::RenderBackend::Null, "backend reports Null");
    }

    // NullDevice_CreateResources_ReturnsUniqueValidHandles
    {
        vyro::NullDevice dev;
        auto vb = dev.create_buffer({vyro::BufferType::Vertex, 1024, nullptr});
        auto ib = dev.create_buffer({vyro::BufferType::Index, 512, nullptr});
        auto sh = dev.create_shader({"vs", "fs"});
        suite.check(vb.valid() && ib.valid() && sh.valid(), "handles are valid");
        suite.check(!(vb == ib), "distinct buffers get distinct ids");
        suite.check(dev.stats().buffers_created == 2, "two buffers recorded");
        suite.check(dev.stats().shaders_created == 1, "one shader recorded");
    }

    // NullDevice_FrameAndDraw_RecordsActivity
    {
        vyro::NullDevice dev;
        auto sh = dev.create_shader({"vs", "fs"});
        auto vb = dev.create_buffer({vyro::BufferType::Vertex, 64, nullptr});

        dev.begin_frame();
        suite.check(dev.stats().in_frame, "in_frame true after begin");
        dev.clear(vyro::Vec4{0.1f, 0.2f, 0.3f, 1.0f});

        vyro::DrawCommand cmd;
        cmd.shader = sh;
        cmd.vertex_buffer = vb;
        cmd.index_count = 6;
        dev.draw(cmd);
        dev.draw(cmd);
        dev.end_frame();

        suite.check(!dev.stats().in_frame, "in_frame false after end");
        suite.check(dev.stats().draw_calls == 2, "two draw calls recorded");
        suite.check(dev.stats().last_index_count == 6, "last index count recorded");
        suite.check(dev.stats().last_clear_color == (vyro::Vec4{0.1f, 0.2f, 0.3f, 1.0f}),
                    "clear color recorded");
        suite.check(dev.stats().frames == 1, "one frame counted");
    }

    return suite.summary();
}

// VyroEngine — Renderer implementation
#include "vyro/render/Renderer.hpp"

#include <algorithm>

namespace vyro {

void Renderer::begin_frame(Vec4 clear_color, u32 width, u32 height)
{
    m_queue.clear(); // retains capacity — no per-frame reallocation
    m_stats = FrameStats{};

    m_device->begin_frame();
    m_device->set_viewport(width, height);
    m_device->clear(clear_color);
}

void Renderer::submit(const RenderItem& item)
{
    m_queue.push_back(item);
    ++m_stats.submitted;
}

void Renderer::end_frame()
{
    // Sort to group identical state (material/shader) and front-to-back depth.
    std::sort(m_queue.begin(), m_queue.end(),
              [](const RenderItem& a, const RenderItem& b) { return a.sort_key < b.sort_key; });

    ShaderHandle bound{}; // invalid -> forces first bind
    for (const RenderItem& item : m_queue) {
        if (!(item.shader == bound)) {
            bound = item.shader;
            ++m_stats.state_changes;
        }

        DrawCommand cmd;
        cmd.shader = item.shader;
        cmd.vertex_buffer = item.vertex_buffer;
        cmd.index_buffer = item.index_buffer;
        cmd.index_count = item.index_count;
        cmd.vertex_count = item.vertex_count;
        cmd.topology = item.topology;
        m_device->draw(cmd);
        ++m_stats.draw_calls;
    }

    m_device->end_frame();
}

} // namespace vyro

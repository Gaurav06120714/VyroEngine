// VyroEngine — Renderer
// Phase 3.2: collects render items for a frame, sorts them by a packed sort
// key to group identical state, and submits draw calls to the RHI. Implements
// the extract -> sort -> submit pattern; the queue is reused across frames so
// steady-state rendering does not allocate (rulz/MEMORY_RULES M-3).
#pragma once

#include "vyro/core/Types.hpp"
#include "vyro/render/RHI.hpp"

#include <vector>

namespace vyro {

// A unit of work for a frame. sort_key typically packs material/shader in the
// high bits and depth in the low bits so sorting groups state together.
struct RenderItem {
    u64 sort_key = 0;
    ShaderHandle shader;
    BufferHandle vertex_buffer;
    BufferHandle index_buffer;
    u32 index_count = 0;
    u32 vertex_count = 0;
    PrimitiveTopology topology = PrimitiveTopology::Triangles;
};

class Renderer : NonCopyable
{
public:
    struct FrameStats {
        u32 submitted = 0;     // items submitted this frame
        u32 draw_calls = 0;    // draw calls issued
        u32 state_changes = 0; // shader binds (lower is better)
    };

    explicit Renderer(IRenderDevice& device) : m_device(&device) {}

    void begin_frame(Vec4 clear_color, u32 width, u32 height);
    void submit(const RenderItem& item);
    void end_frame();

    [[nodiscard]] const FrameStats& last_frame() const { return m_stats; }

private:
    IRenderDevice* m_device;
    std::vector<RenderItem> m_queue;
    FrameStats m_stats;
};

} // namespace vyro

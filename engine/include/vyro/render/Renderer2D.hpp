// VyroEngine — 2D renderer
// Phase 3.6: batches quads into a single reused vertex buffer and flushes them
// as one draw call per batch, minimizing draw overhead. A batch breaks when it
// reaches the quad limit. Vertex storage is preallocated so steady-state
// drawing does not allocate (rulz/MEMORY_RULES M-3).
#pragma once

#include "vyro/core/Types.hpp"
#include "vyro/math/Vec.hpp"
#include "vyro/render/Camera.hpp"
#include "vyro/render/RHI.hpp"

#include <vector>

namespace vyro {

struct QuadVertex {
    Vec3 position{};
    Vec4 color{};
    Vec2 uv{};
};

class Renderer2D : NonCopyable
{
public:
    static constexpr u32 kMaxQuads = 1000;
    static constexpr u32 kMaxVertices = kMaxQuads * 4;
    static constexpr u32 kIndicesPerQuad = 6;

    struct Stats {
        u32 quad_count = 0;  // quads drawn this scene
        u32 draw_calls = 0;  // batches flushed this scene
    };

    Renderer2D(IRenderDevice& device, ShaderHandle shader);

    void begin_scene(const Camera& camera);
    void draw_quad(Vec2 position, Vec2 size, Vec4 color);
    void end_scene();

    [[nodiscard]] const Stats& stats() const { return m_stats; }

private:
    void flush();

    IRenderDevice* m_device;
    ShaderHandle m_shader;
    BufferHandle m_vertex_buffer;

    std::vector<QuadVertex> m_vertices; // CPU staging, preallocated
    u32 m_batch_quads = 0;
    Mat4 m_view_projection = Mat4::identity();
    Stats m_stats;
};

} // namespace vyro

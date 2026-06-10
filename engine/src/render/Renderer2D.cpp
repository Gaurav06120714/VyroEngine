// VyroEngine — 2D renderer implementation
#include "vyro/render/Renderer2D.hpp"

namespace vyro {

Renderer2D::Renderer2D(IRenderDevice& device, ShaderHandle shader)
    : m_device(&device), m_shader(shader)
{
    m_vertices.reserve(kMaxVertices);

    BufferDesc desc;
    desc.type = BufferType::Vertex;
    desc.size = static_cast<usize>(kMaxVertices) * sizeof(QuadVertex);
    m_vertex_buffer = m_device->create_buffer(desc);
}

void Renderer2D::begin_scene(const Camera& camera)
{
    m_view_projection = camera.view_projection();
    m_vertices.clear();
    m_batch_quads = 0;
    m_stats = Stats{};
}

void Renderer2D::draw_quad(Vec2 position, Vec2 size, Vec4 color)
{
    if (m_batch_quads >= kMaxQuads) {
        flush();
    }

    const f32 x = position.x;
    const f32 y = position.y;
    const f32 w = size.x;
    const f32 h = size.y;

    // Two triangles' worth as a quad (indices handled by the index buffer in
    // real backends); here we stage four corner vertices.
    m_vertices.push_back(QuadVertex{Vec3{x, y, 0.0f}, color, Vec2{0.0f, 0.0f}});
    m_vertices.push_back(QuadVertex{Vec3{x + w, y, 0.0f}, color, Vec2{1.0f, 0.0f}});
    m_vertices.push_back(QuadVertex{Vec3{x + w, y + h, 0.0f}, color, Vec2{1.0f, 1.0f}});
    m_vertices.push_back(QuadVertex{Vec3{x, y + h, 0.0f}, color, Vec2{0.0f, 1.0f}});

    ++m_batch_quads;
    ++m_stats.quad_count;
}

void Renderer2D::end_scene()
{
    flush();
}

void Renderer2D::flush()
{
    if (m_batch_quads == 0) {
        return;
    }

    DrawCommand cmd;
    cmd.shader = m_shader;
    cmd.vertex_buffer = m_vertex_buffer;
    cmd.vertex_count = m_batch_quads * 4;
    cmd.index_count = m_batch_quads * kIndicesPerQuad;
    cmd.topology = PrimitiveTopology::Triangles;
    m_device->draw(cmd);
    ++m_stats.draw_calls;

    m_vertices.clear();
    m_batch_quads = 0;
}

} // namespace vyro

// VyroEngine — 3D renderer implementation
#include "vyro/render/Renderer3D.hpp"

namespace vyro {

void Renderer3D::begin_scene(const Camera& camera)
{
    m_view_projection = camera.view_projection();
    m_queue.clear();
    m_stats = Stats{};
}

void Renderer3D::submit(const MeshSubmission& mesh)
{
    m_queue.push_back(mesh);
    ++m_stats.submitted;
}

void Renderer3D::end_scene()
{
    for (const MeshSubmission& mesh : m_queue) {
        // Frustum culling integrates here; the Null backend draws everything.
        // The combined transform would be uploaded as a per-object uniform.
        const Mat4 mvp = m_view_projection * mesh.model;
        (void)mvp;

        DrawCommand cmd;
        cmd.shader = m_shader;
        cmd.vertex_buffer = mesh.vertex_buffer;
        cmd.index_buffer = mesh.index_buffer;
        cmd.index_count = mesh.index_count;
        cmd.topology = PrimitiveTopology::Triangles;
        m_device->draw(cmd);
        ++m_stats.draw_calls;
    }
}

} // namespace vyro

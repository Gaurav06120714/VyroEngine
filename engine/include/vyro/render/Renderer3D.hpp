// VyroEngine — 3D renderer
// Phase 3.7: collects mesh submissions for a frame and draws them under a
// camera. Each submission carries its model matrix; the combined MVP is formed
// from the camera's view-projection. Frustum culling hooks in at end_scene.
#pragma once

#include "vyro/core/Types.hpp"
#include "vyro/math/Mat4.hpp"
#include "vyro/render/Camera.hpp"
#include "vyro/render/RHI.hpp"

#include <vector>

namespace vyro {

struct MeshSubmission {
    BufferHandle vertex_buffer;
    BufferHandle index_buffer;
    u32 index_count = 0;
    Mat4 model = Mat4::identity();
};

class Renderer3D : NonCopyable
{
public:
    struct Stats {
        u32 submitted = 0;  // meshes submitted this scene
        u32 draw_calls = 0; // draw calls issued (after culling)
        u32 culled = 0;     // meshes rejected by culling
    };

    Renderer3D(IRenderDevice& device, ShaderHandle shader) : m_device(&device), m_shader(shader) {}

    void begin_scene(const Camera& camera);
    void submit(const MeshSubmission& mesh);
    void end_scene();

    [[nodiscard]] const Stats& stats() const { return m_stats; }
    [[nodiscard]] const Mat4& view_projection() const { return m_view_projection; }

private:
    IRenderDevice* m_device;
    ShaderHandle m_shader;
    std::vector<MeshSubmission> m_queue;
    Mat4 m_view_projection = Mat4::identity();
    Stats m_stats;
};

} // namespace vyro

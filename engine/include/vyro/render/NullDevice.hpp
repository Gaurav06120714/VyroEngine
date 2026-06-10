// VyroEngine — Null render device
// Phase 3.1: a headless IRenderDevice that allocates handles and records
// activity (resource counts, draw calls, clears) without a GPU. Enables
// offline development and deterministic renderer tests until the Vulkan and
// OpenGL backends land.
#pragma once

#include "vyro/render/RHI.hpp"

namespace vyro {

class NullDevice final : public IRenderDevice
{
public:
    struct Stats {
        u32 buffers_created = 0;
        u32 buffers_destroyed = 0;
        u32 textures_created = 0;
        u32 shaders_created = 0;
        u32 frames = 0;
        u32 draw_calls = 0;       // total across all frames
        u32 draw_calls_frame = 0; // within the current frame
        u32 last_index_count = 0;
        Vec4 last_clear_color{};
        bool in_frame = false;
    };

    [[nodiscard]] RenderBackend backend() const override { return RenderBackend::Null; }

    [[nodiscard]] BufferHandle create_buffer(const BufferDesc& desc) override;
    void update_buffer(BufferHandle handle, const void* data, usize size) override;
    void destroy_buffer(BufferHandle handle) override;
    [[nodiscard]] TextureHandle create_texture(const TextureDesc& desc) override;
    void destroy_texture(TextureHandle handle) override;
    [[nodiscard]] ShaderHandle create_shader(const ShaderDesc& desc) override;
    void destroy_shader(ShaderHandle handle) override;

    void begin_frame() override;
    void set_viewport(u32 width, u32 height) override;
    void clear(Vec4 color) override;
    void draw(const DrawCommand& command) override;
    void end_frame() override;

    [[nodiscard]] const Stats& stats() const { return m_stats; }

private:
    Stats m_stats;
    u32 m_next_id = 1; // 0 is reserved as the invalid handle
};

} // namespace vyro

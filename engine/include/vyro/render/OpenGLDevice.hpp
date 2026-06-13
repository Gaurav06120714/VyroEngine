// VyroEngine — OpenGL render device
// The first real GPU backend behind the RHI: an OpenGL 3.3 core implementation
// of IRenderDevice. Assumes the interleaved vertex layout of QuadVertex
// (vec3 position, vec4 color, vec2 uv). Built only when GL is enabled.
#pragma once

#include "vyro/math/Mat4.hpp"
#include "vyro/render/RHI.hpp"

#include <unordered_map>

namespace vyro {

class OpenGLDevice final : public IRenderDevice
{
public:
    OpenGLDevice();
    ~OpenGLDevice() override;

    [[nodiscard]] RenderBackend backend() const override { return RenderBackend::OpenGL; }

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

    // Concrete uniform helpers (used by 3D rendering paths).
    void set_uniform_mat4(ShaderHandle shader, const char* name, const Mat4& value);
    void set_uniform_vec3(ShaderHandle shader, const char* name, Vec3 value);
    void set_uniform_float(ShaderHandle shader, const char* name, f32 value);

    // Toggle depth testing (HUD/overlay passes draw with it off).
    void set_depth_test(bool enabled);

private:
    struct GLBuffer {
        u32 id = 0;
        u32 target = 0; // GL_ARRAY_BUFFER or GL_ELEMENT_ARRAY_BUFFER
    };

    std::unordered_map<u32, GLBuffer> m_buffers; // RHI id -> GL buffer
    std::unordered_map<u32, u32> m_programs;     // RHI id -> GL program
    std::unordered_map<u32, u32> m_textures;     // RHI id -> GL texture
    u32 m_vao = 0;
    u32 m_next_id = 1;
};

} // namespace vyro

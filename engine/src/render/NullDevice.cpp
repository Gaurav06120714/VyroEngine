// VyroEngine — Null render device implementation
#include "vyro/render/NullDevice.hpp"

namespace vyro {

BufferHandle NullDevice::create_buffer(const BufferDesc& /*desc*/)
{
    ++m_stats.buffers_created;
    return BufferHandle{m_next_id++};
}

void NullDevice::destroy_buffer(BufferHandle handle)
{
    if (handle.valid()) {
        ++m_stats.buffers_destroyed;
    }
}

TextureHandle NullDevice::create_texture(const TextureDesc& /*desc*/)
{
    ++m_stats.textures_created;
    return TextureHandle{m_next_id++};
}

void NullDevice::destroy_texture(TextureHandle /*handle*/) {}

ShaderHandle NullDevice::create_shader(const ShaderDesc& /*desc*/)
{
    ++m_stats.shaders_created;
    return ShaderHandle{m_next_id++};
}

void NullDevice::destroy_shader(ShaderHandle /*handle*/) {}

void NullDevice::begin_frame()
{
    m_stats.in_frame = true;
    m_stats.draw_calls_frame = 0;
    ++m_stats.frames;
}

void NullDevice::set_viewport(u32 /*width*/, u32 /*height*/) {}

void NullDevice::clear(Vec4 color)
{
    m_stats.last_clear_color = color;
}

void NullDevice::draw(const DrawCommand& command)
{
    ++m_stats.draw_calls;
    ++m_stats.draw_calls_frame;
    m_stats.last_index_count = command.index_count;
}

void NullDevice::end_frame()
{
    m_stats.in_frame = false;
}

} // namespace vyro

// VyroEngine — Texture system implementation
#include "vyro/render/Texture.hpp"

namespace vyro {

u32 bytes_per_pixel(TextureFormat format)
{
    switch (format) {
        case TextureFormat::R8: return 1;
        case TextureFormat::RGB8: return 3;
        case TextureFormat::RGBA8: return 4;
        case TextureFormat::Depth24Stencil8: return 4;
    }
    return 0;
}

Texture2D Texture2D::create(IRenderDevice& device, const TextureDesc& desc)
{
    const TextureHandle handle = device.create_texture(desc);
    return Texture2D(handle, desc.width, desc.height, desc.format);
}

} // namespace vyro

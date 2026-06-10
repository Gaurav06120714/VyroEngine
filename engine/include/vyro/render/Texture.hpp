// VyroEngine — Texture system
// Phase 3.4: a 2D texture resource over the RHI. Holds the GPU handle plus
// CPU-side dimensions and format. Real backends add mipmapping, BCn
// compression, and streaming; this defines the resource and creation path.
#pragma once

#include "vyro/core/Types.hpp"
#include "vyro/render/RHI.hpp"

namespace vyro {

// Bytes per pixel for an uncompressed format.
[[nodiscard]] u32 bytes_per_pixel(TextureFormat format);

class Texture2D
{
public:
    Texture2D() = default;
    Texture2D(TextureHandle handle, u32 width, u32 height, TextureFormat format)
        : m_handle(handle), m_width(width), m_height(height), m_format(format)
    {
    }

    // Create a texture on the device from a descriptor.
    [[nodiscard]] static Texture2D create(IRenderDevice& device, const TextureDesc& desc);

    [[nodiscard]] TextureHandle handle() const { return m_handle; }
    [[nodiscard]] u32 width() const { return m_width; }
    [[nodiscard]] u32 height() const { return m_height; }
    [[nodiscard]] TextureFormat format() const { return m_format; }
    [[nodiscard]] bool valid() const { return m_handle.valid(); }

    // CPU byte size of one uncompressed level.
    [[nodiscard]] usize size_bytes() const
    {
        return static_cast<usize>(m_width) * m_height * bytes_per_pixel(m_format);
    }

private:
    TextureHandle m_handle;
    u32 m_width = 0;
    u32 m_height = 0;
    TextureFormat m_format = TextureFormat::RGBA8;
};

} // namespace vyro

// VyroEngine — Texture tests
#include "vyro/render/NullDevice.hpp"
#include "vyro/render/Texture.hpp"

#include "test_harness.hpp"

int main()
{
    vyro::test::Suite suite("texture");

    // bytes_per_pixel matches format
    suite.check(vyro::bytes_per_pixel(vyro::TextureFormat::R8) == 1, "R8 = 1 bpp");
    suite.check(vyro::bytes_per_pixel(vyro::TextureFormat::RGB8) == 3, "RGB8 = 3 bpp");
    suite.check(vyro::bytes_per_pixel(vyro::TextureFormat::RGBA8) == 4, "RGBA8 = 4 bpp");

    // Texture2D_Create_StoresDescriptorAndHandle
    {
        vyro::NullDevice dev;
        vyro::TextureDesc desc;
        desc.width = 64;
        desc.height = 32;
        desc.format = vyro::TextureFormat::RGBA8;
        auto tex = vyro::Texture2D::create(dev, desc);

        suite.check(tex.valid(), "created texture is valid");
        suite.check(tex.width() == 64 && tex.height() == 32, "dimensions stored");
        suite.check(tex.format() == vyro::TextureFormat::RGBA8, "format stored");
        suite.check(tex.size_bytes() == 64u * 32u * 4u, "size_bytes computed from format");
        suite.check(dev.stats().textures_created == 1, "device created one texture");
    }

    // Texture2D_Default_IsInvalid
    {
        vyro::Texture2D tex;
        suite.check(!tex.valid(), "default texture is invalid");
        suite.check(tex.size_bytes() == 0, "default texture has zero size");
    }

    return suite.summary();
}

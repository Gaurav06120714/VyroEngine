// VyroEngine — Image asset tests (V2.5)
#include "vyro/assets/Image.hpp"

#include "test_harness.hpp"

int main()
{
    using namespace vyro;
    test::Suite suite("image");

    // Checkerboard_AlternatesCells
    {
        const Image img = make_checkerboard(8, 4, 255, 0, 0, 0, 0, 255);
        suite.check(img.width == 8 && img.height == 8, "8x8 image");
        suite.check(img.byte_size() == 8 * 8 * 4, "RGBA8 size");
        suite.check(img.at(0, 0)[0] == 255, "cell (0,0) is color 1 (red)");
        suite.check(img.at(4, 0)[2] == 255, "cell (4,0) is color 2 (blue)");
        suite.check(img.at(4, 4)[0] == 255, "cell (4,4) back to color 1");
        suite.check(img.at(0, 0)[3] == 255, "alpha opaque");
    }

    // Solid_OnePixel
    {
        const Image img = make_solid(10, 20, 30, 40);
        suite.check(img.width == 1 && img.height == 1, "1x1 image");
        suite.check(img.pixels[0] == 10 && img.pixels[1] == 20 && img.pixels[2] == 30
                        && img.pixels[3] == 40,
                    "solid color stored");
    }

    // LoadImage_MissingFile_Errors
    {
        const auto result = load_image("/no/such/texture.png");
        suite.check(!result.has_value(), "missing file errors");
        suite.check(result.error() == ImageError::FileNotFound, "FileNotFound reported");
    }

    return suite.summary();
}

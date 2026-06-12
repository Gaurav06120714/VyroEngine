// VyroEngine — Text geometry tests (V3.4)
#include "vyro/render/TextGeometry.hpp"

#include "test_harness.hpp"

int main()
{
    using namespace vyro;
    test::Suite suite("text");

    // Known glyphs emit pixel quads (6 verts per lit pixel).
    {
        std::vector<Vertex3D> out;
        text::build("1", -1.0f, 0.0f, 0.1f, 1.0f, {1, 1, 1}, out);
        suite.check(!out.empty(), "digit produces geometry");
        suite.check(out.size() % 6 == 0, "geometry is whole quads");
    }

    // Denser glyphs emit more quads than sparse ones.
    {
        std::vector<Vertex3D> one;
        std::vector<Vertex3D> eight;
        text::build("1", 0, 0, 0.1f, 1.0f, {1, 1, 1}, one);
        text::build("8", 0, 0, 0.1f, 1.0f, {1, 1, 1}, eight);
        suite.check(eight.size() > one.size(), "8 is denser than 1");
    }

    // Lowercase folds to uppercase; unknown chars become space.
    {
        std::vector<Vertex3D> lower;
        std::vector<Vertex3D> upper;
        text::build("wave", 0, 0, 0.1f, 1.0f, {1, 1, 1}, lower);
        text::build("WAVE", 0, 0, 0.1f, 1.0f, {1, 1, 1}, upper);
        suite.check(lower.size() == upper.size(), "lowercase folds to uppercase");

        std::vector<Vertex3D> unknown;
        text::build("~", 0, 0, 0.1f, 1.0f, {1, 1, 1}, unknown);
        suite.check(unknown.empty(), "unknown char renders as space");
    }

    // Measure scales with length and is consistent with advance.
    {
        const f32 w1 = text::measure("A", 0.1f, 1.0f);
        const f32 w3 = text::measure("AAA", 0.1f, 1.0f);
        suite.check(w3 > w1 * 2.9f && w3 < w1 * 3.1f, "measure scales linearly");
    }

    // Color is baked into vertices.
    {
        std::vector<Vertex3D> out;
        text::build("A", 0, 0, 0.1f, 1.0f, {1, 0, 0}, out);
        suite.check(out[0].color == (Vec3{1, 0, 0}), "color applied");
    }

    return suite.summary();
}

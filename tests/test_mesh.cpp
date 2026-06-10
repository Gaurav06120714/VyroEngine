// VyroEngine — Mesh and OBJ loader tests
#include "vyro/assets/Mesh.hpp"
#include "vyro/assets/ObjLoader.hpp"

#include "test_harness.hpp"

int main()
{
    vyro::test::Suite suite("mesh");

    // make_cube produces a closed cube
    {
        const vyro::MeshData cube = vyro::make_cube(2.0f);
        suite.check(cube.vertices.size() == 24, "cube has 24 vertices (4 per face)");
        suite.check(cube.indices.size() == 36, "cube has 36 indices (2 tris * 6 faces)");
        suite.check(!cube.empty(), "cube is not empty");
    }

    // parse_obj loads a triangle with explicit normals
    {
        const char* obj =
            "v 0 0 0\n"
            "v 1 0 0\n"
            "v 0 1 0\n"
            "vn 0 0 1\n"
            "f 1//1 2//1 3//1\n";
        const auto result = vyro::parse_obj(obj);
        suite.check(result.has_value(), "triangle OBJ parses");
        suite.check(result->vertices.size() == 3, "three vertices");
        suite.check(result->indices.size() == 3, "three indices");
        suite.check(result->vertices[0].normal == (vyro::Vec3{0, 0, 1}), "normal applied");
    }

    // parse_obj triangulates a quad face into two triangles
    {
        const char* obj =
            "v 0 0 0\n"
            "v 1 0 0\n"
            "v 1 1 0\n"
            "v 0 1 0\n"
            "f 1 2 3 4\n";
        const auto result = vyro::parse_obj(obj);
        suite.check(result.has_value(), "quad OBJ parses");
        suite.check(result->indices.size() == 6, "quad triangulated into 6 indices");
        suite.check(!(result->vertices[0].normal == (vyro::Vec3{0, 0, 0})),
                    "generated normal is non-zero");
    }

    // Missing file reports an error
    {
        const auto result = vyro::load_obj("/no/such/model.obj");
        suite.check(!result.has_value(), "missing file returns error");
        suite.check(result.error() == vyro::ObjError::FileNotFound, "error is FileNotFound");
    }

    return suite.summary();
}

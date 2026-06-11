// VyroEngine — Wavefront OBJ loader implementation
#include "vyro/assets/ObjLoader.hpp"

#include <charconv>
#include <fstream>
#include <sstream>
#include <unordered_map>

namespace vyro {

namespace {

// Parse a face vertex token "v", "v/vt", "v//vn", or "v/vt/vn". OBJ indices are
// 1-based; returns 0-based, with -1 meaning "absent".
struct FaceRef {
    int v = -1;
    int vt = -1;
    int vn = -1;
};

FaceRef parse_ref(std::string_view token)
{
    FaceRef ref;
    int field = 0;
    usize start = 0;
    for (usize i = 0; i <= token.size(); ++i) {
        if (i == token.size() || token[i] == '/') {
            if (i > start) {
                int value = 0;
                std::from_chars(token.data() + start, token.data() + i, value);
                if (field == 0) {
                    ref.v = value - 1;
                } else if (field == 1) {
                    ref.vt = value - 1;
                } else {
                    ref.vn = value - 1;
                }
            }
            ++field;
            start = i + 1;
        }
    }
    return ref;
}

Vec3 face_normal(Vec3 a, Vec3 b, Vec3 c)
{
    return normalize(cross(b - a, c - a));
}

} // namespace

MaterialMap parse_mtl(std::string_view text)
{
    MaterialMap materials;
    std::string current;

    std::istringstream stream{std::string(text)};
    std::string line;
    while (std::getline(stream, line)) {
        std::istringstream ls(line);
        std::string tag;
        ls >> tag;
        if (tag == "newmtl") {
            ls >> current;
            materials[current] = Vec3{1.0f, 1.0f, 1.0f};
        } else if (tag == "Kd" && !current.empty()) {
            Vec3 kd;
            ls >> kd.x >> kd.y >> kd.z;
            materials[current] = kd;
        }
    }
    return materials;
}

std::expected<MeshData, ObjError> parse_obj(std::string_view text, const MaterialMap& materials)
{
    std::vector<Vec3> positions;
    std::vector<Vec2> texcoords;
    std::vector<Vec3> normals;
    Vec3 current_color{1.0f, 1.0f, 1.0f};

    MeshData mesh;
    std::unordered_map<std::string, u32> unique; // "v/vt/vn/material" -> mesh index

    std::istringstream stream{std::string(text)};
    std::string line;
    while (std::getline(stream, line)) {
        std::istringstream ls(line);
        std::string tag;
        ls >> tag;
        if (tag == "usemtl") {
            std::string name;
            ls >> name;
            const auto it = materials.find(name);
            current_color = (it != materials.end()) ? it->second : Vec3{1.0f, 1.0f, 1.0f};
        } else if (tag == "v") {
            Vec3 p;
            ls >> p.x >> p.y >> p.z;
            positions.push_back(p);
        } else if (tag == "vt") {
            Vec2 t;
            ls >> t.x >> t.y;
            texcoords.push_back(t);
        } else if (tag == "vn") {
            Vec3 n;
            ls >> n.x >> n.y >> n.z;
            normals.push_back(n);
        } else if (tag == "f") {
            std::vector<FaceRef> refs;
            std::string token;
            while (ls >> token) {
                refs.push_back(parse_ref(token));
            }
            if (refs.size() < 3) {
                continue;
            }
            // Triangulate the polygon as a fan.
            for (usize i = 1; i + 1 < refs.size(); ++i) {
                const FaceRef tri[3] = {refs[0], refs[i], refs[i + 1]};

                // Generate a normal if the face has none.
                Vec3 gen_normal{};
                if (tri[0].vn < 0) {
                    const Vec3 a = positions[static_cast<usize>(tri[0].v)];
                    const Vec3 b = positions[static_cast<usize>(tri[1].v)];
                    const Vec3 c = positions[static_cast<usize>(tri[2].v)];
                    gen_normal = face_normal(a, b, c);
                }

                for (const FaceRef& r : tri) {
                    std::string key = std::to_string(r.v) + "/" + std::to_string(r.vt) + "/"
                                      + std::to_string(r.vn) + "/" + std::to_string(current_color.x)
                                      + "," + std::to_string(current_color.y) + ","
                                      + std::to_string(current_color.z);
                    const auto it = unique.find(key);
                    if (it != unique.end()) {
                        mesh.indices.push_back(it->second);
                        continue;
                    }
                    Vertex3D vert;
                    vert.color = current_color;
                    if (r.v >= 0 && static_cast<usize>(r.v) < positions.size()) {
                        vert.position = positions[static_cast<usize>(r.v)];
                    }
                    if (r.vt >= 0 && static_cast<usize>(r.vt) < texcoords.size()) {
                        vert.uv = texcoords[static_cast<usize>(r.vt)];
                    }
                    vert.normal = (r.vn >= 0 && static_cast<usize>(r.vn) < normals.size())
                                      ? normals[static_cast<usize>(r.vn)]
                                      : gen_normal;
                    const u32 index = static_cast<u32>(mesh.vertices.size());
                    mesh.vertices.push_back(vert);
                    unique.emplace(std::move(key), index);
                    mesh.indices.push_back(index);
                }
            }
        }
    }

    if (mesh.vertices.empty()) {
        return std::unexpected(ObjError::Empty);
    }
    return mesh;
}

std::expected<MeshData, ObjError> load_obj(std::string_view path)
{
    std::ifstream file{std::string(path)};
    if (!file.is_open()) {
        return std::unexpected(ObjError::FileNotFound);
    }
    std::stringstream buffer;
    buffer << file.rdbuf();
    const std::string text = buffer.str();

    // Load the referenced material library (if any) from the OBJ's directory.
    // Also try "<obj-stem>.mtl" since archives are often renamed as a pair.
    MaterialMap materials;
    const std::string dir(path.substr(0, path.find_last_of('/') + 1));

    std::istringstream scan(text);
    std::string line;
    while (std::getline(scan, line)) {
        std::istringstream ls(line);
        std::string tag;
        ls >> tag;
        if (tag == "mtllib") {
            std::string name;
            ls >> name;
            std::ifstream mtl(dir + name);
            if (mtl.is_open()) {
                std::stringstream mb;
                mb << mtl.rdbuf();
                materials = parse_mtl(mb.str());
            }
            break;
        }
    }
    if (materials.empty()) {
        std::string stem(path);
        const usize dot = stem.find_last_of('.');
        if (dot != std::string::npos) {
            std::ifstream mtl(stem.substr(0, dot) + ".mtl");
            if (mtl.is_open()) {
                std::stringstream mb;
                mb << mtl.rdbuf();
                materials = parse_mtl(mb.str());
            }
        }
    }

    return parse_obj(text, materials);
}

} // namespace vyro

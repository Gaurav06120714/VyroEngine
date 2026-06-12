// VyroEngine — Scene serialization implementation
#include "vyro/scene/SceneSerializer.hpp"

#include <nlohmann/json.hpp>

#include <fstream>
#include <sstream>

namespace vyro::scene_io {

using nlohmann::json;

std::string to_json(Registry& registry, const SceneSchema& schema)
{
    json scene;
    scene["version"] = 1;
    scene["entities"] = json::array();

    for (const Entity e : registry.alive_entities()) {
        json components = json::object();
        for (const SceneSchema::Entry& entry : schema.entries()) {
            if (entry.has(registry, e)) {
                json fields = json::object();
                for (const auto& [key, value] : entry.save(registry, e)) {
                    fields[key] = value;
                }
                components[entry.name] = std::move(fields);
            }
        }
        json je;
        je["components"] = std::move(components);
        scene["entities"].push_back(std::move(je));
    }
    return scene.dump(2);
}

std::expected<u32, SceneError> from_json(Registry& registry, const SceneSchema& schema,
                                         std::string_view json_text)
{
    const json scene = json::parse(json_text, nullptr, false);
    if (scene.is_discarded() || !scene.contains("entities")) {
        return std::unexpected(SceneError::ParseError);
    }

    u32 created = 0;
    for (const json& je : scene["entities"]) {
        const Entity e = registry.create_entity();
        ++created;
        if (!je.contains("components")) {
            continue;
        }
        for (const SceneSchema::Entry& entry : schema.entries()) {
            if (!je["components"].contains(entry.name)) {
                continue;
            }
            FieldMap fields;
            for (const auto& [key, value] : je["components"][entry.name].items()) {
                fields[key] = value.get<std::string>();
            }
            entry.load(registry, e, fields);
        }
    }
    return created;
}

bool save_file(std::string_view path, Registry& registry, const SceneSchema& schema)
{
    std::ofstream file{std::string(path)};
    if (!file.is_open()) {
        return false;
    }
    file << to_json(registry, schema);
    return file.good();
}

std::expected<u32, SceneError> load_file(std::string_view path, Registry& registry,
                                         const SceneSchema& schema)
{
    std::ifstream file{std::string(path)};
    if (!file.is_open()) {
        return std::unexpected(SceneError::FileNotFound);
    }
    std::stringstream buffer;
    buffer << file.rdbuf();
    return from_json(registry, schema, buffer.str());
}

} // namespace vyro::scene_io

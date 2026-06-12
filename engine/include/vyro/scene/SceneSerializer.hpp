// VyroEngine — Scene serialization (V3.5)
// Saves and loads a Registry's entities as JSON. Component types register a
// name plus save/load functions in a SceneSchema; the serializer walks live
// entities and round-trips every registered component it finds. This is the
// authoring loop: build a scene in the editor, save it, load it in a game.
#pragma once

#include "vyro/core/Types.hpp"
#include "vyro/ecs/Registry.hpp"

#include <expected>
#include <functional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace vyro {

enum class SceneError {
    FileNotFound,
    ParseError,
};

// Type-erased component <-> key/value-map converters. Values are strings;
// component registrars encode/decode their own fields.
using FieldMap = std::unordered_map<std::string, std::string>;

class SceneSchema
{
public:
    template<typename T>
    void register_component(std::string name,
                            std::function<FieldMap(const T&)> save,
                            std::function<T(const FieldMap&)> load)
    {
        Entry entry;
        entry.name = name;
        entry.has = [](Registry& r, Entity e) { return r.has_component<T>(e); };
        entry.save = [save](Registry& r, Entity e) { return save(*r.get_component<T>(e)); };
        entry.load = [load](Registry& r, Entity e, const FieldMap& fields) {
            r.add_component<T>(e, load(fields));
        };
        m_entries.push_back(std::move(entry));
    }

    struct Entry {
        std::string name;
        std::function<bool(Registry&, Entity)> has;
        std::function<FieldMap(Registry&, Entity)> save;
        std::function<void(Registry&, Entity, const FieldMap&)> load;
    };

    [[nodiscard]] const std::vector<Entry>& entries() const { return m_entries; }

private:
    std::vector<Entry> m_entries;
};

namespace scene_io {

// Serialize every live entity's registered components to a JSON string.
[[nodiscard]] std::string to_json(Registry& registry, const SceneSchema& schema);

// Create entities in `registry` from a JSON scene string.
std::expected<u32, SceneError> from_json(Registry& registry, const SceneSchema& schema,
                                         std::string_view json_text);

// File convenience wrappers.
bool save_file(std::string_view path, Registry& registry, const SceneSchema& schema);
std::expected<u32, SceneError> load_file(std::string_view path, Registry& registry,
                                         const SceneSchema& schema);

} // namespace scene_io

} // namespace vyro

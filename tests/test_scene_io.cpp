// VyroEngine — Scene serialization tests (V3.5)
#include "vyro/scene/SceneSerializer.hpp"

#include "test_harness.hpp"

#include <filesystem>
#include <string>

namespace {

struct Name {
    std::string value;
};

struct Health {
    float current = 100.0f;
};

vyro::SceneSchema make_schema()
{
    vyro::SceneSchema schema;
    schema.register_component<Name>(
        "Name", [](const Name& n) { return vyro::FieldMap{{"value", n.value}}; },
        [](const vyro::FieldMap& f) { return Name{f.at("value")}; });
    schema.register_component<Health>(
        "Health",
        [](const Health& h) { return vyro::FieldMap{{"current", std::to_string(h.current)}}; },
        [](const vyro::FieldMap& f) { return Health{std::stof(f.at("current"))}; });
    return schema;
}

} // namespace

int main()
{
    using namespace vyro;
    test::Suite suite("scene_io");
    const SceneSchema schema = make_schema();

    // RoundTrip_PreservesEntitiesAndComponents
    {
        Registry source;
        const auto hero = source.create_entity();
        source.add_component<Name>(hero, Name{"Hero"});
        source.add_component<Health>(hero, Health{42.5f});
        const auto prop = source.create_entity();
        source.add_component<Name>(prop, Name{"Crate"});

        const std::string text = scene_io::to_json(source, schema);
        suite.check(text.find("Hero") != std::string::npos, "JSON contains entity data");

        Registry loaded;
        const auto count = scene_io::from_json(loaded, schema, text);
        suite.check(count.has_value() && count.value() == 2, "two entities loaded");
        suite.check(loaded.entity_count() == 2, "registry holds loaded entities");

        int heroes = 0;
        loaded.view<Name>().for_each_entity([&](Entity e, Name& n) {
            if (n.value == "Hero") {
                ++heroes;
                auto* h = loaded.get_component<Health>(e);
                suite.check(h != nullptr && h->current > 42.4f && h->current < 42.6f,
                            "health survives roundtrip");
            }
        });
        suite.check(heroes == 1, "named entity restored");
    }

    // PartialComponents_OnlyRegisteredOnesPersist
    {
        struct Unregistered {
            int x = 0;
        };
        Registry source;
        const auto e = source.create_entity();
        source.add_component<Name>(e, Name{"OnlyName"});
        source.add_component<Unregistered>(e, Unregistered{7});

        Registry loaded;
        const auto count =
            scene_io::from_json(loaded, schema, scene_io::to_json(source, schema));
        suite.check(count.has_value(), "scene loads");
        loaded.view<Name>().for_each_entity([&](Entity le, Name&) {
            suite.check(!loaded.has_component<Unregistered>(le),
                        "unregistered component not persisted");
        });
    }

    // Files_SaveAndLoad
    {
        const auto path = (std::filesystem::temp_directory_path() / "vyro_test.scene").string();
        Registry source;
        const auto e = source.create_entity();
        source.add_component<Name>(e, Name{"Saved"});
        suite.check(scene_io::save_file(path, source, schema), "scene saved to disk");

        Registry loaded;
        const auto count = scene_io::load_file(path, loaded, schema);
        suite.check(count.has_value() && count.value() == 1, "scene loaded from disk");
        std::filesystem::remove(path);
    }

    // Errors
    {
        Registry r;
        suite.check(!scene_io::from_json(r, schema, "not json").has_value(),
                    "garbage input rejected");
        suite.check(!scene_io::load_file("/no/such.scene", r, schema).has_value(),
                    "missing file rejected");
    }

    return suite.summary();
}

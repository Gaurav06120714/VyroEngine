// VyroEngine — Editor model tests
#include "vyro/editor/AssetBrowser.hpp"
#include "vyro/editor/EditorContext.hpp"
#include "vyro/editor/Gizmo.hpp"
#include "vyro/editor/Inspector.hpp"

#include "test_harness.hpp"

namespace {

// Undoable command that adds a TransformComponent (8.1 test fixture).
class AddTransformCommand final : public vyro::IEditorCommand
{
public:
    explicit AddTransformCommand(vyro::Entity entity) : m_entity(entity) {}
    void execute(vyro::Registry& r) override
    {
        r.add_component<vyro::TransformComponent>(m_entity, vyro::TransformComponent{});
    }
    void undo(vyro::Registry& r) override { r.remove_component<vyro::TransformComponent>(m_entity); }
    std::string_view name() const override { return "AddTransform"; }

private:
    vyro::Entity m_entity;
};

} // namespace

int main()
{
    using namespace vyro;
    test::Suite suite("editor");

    // EditorContext_CommandStack_UndoRedo (8.1)
    {
        Registry reg;
        EditorContext editor(reg);
        const Entity e = reg.create_entity();

        editor.execute(std::make_unique<AddTransformCommand>(e));
        suite.check(reg.has_component<TransformComponent>(e), "command executed");
        suite.check(editor.undo(), "undo succeeds");
        suite.check(!reg.has_component<TransformComponent>(e), "undo reverted component");
        suite.check(editor.redo(), "redo succeeds");
        suite.check(reg.has_component<TransformComponent>(e), "redo re-applied component");
        suite.check(!editor.redo(), "redo stack exhausted");
    }

    // EditorContext_Selection (8.1)
    {
        Registry reg;
        EditorContext editor(reg);
        const Entity e = reg.create_entity();
        suite.check(!editor.has_selection(), "no selection initially");
        editor.select(e);
        suite.check(editor.selected() == e, "entity selected");
        editor.clear_selection();
        suite.check(!editor.has_selection(), "selection cleared");
    }

    // EditorContext_Hierarchy_ReparentAndDetach (8.2)
    {
        Registry reg;
        EditorContext editor(reg);
        const Entity parent = reg.create_entity();
        const Entity child = reg.create_entity();
        const Entity parent2 = reg.create_entity();

        editor.set_parent(child, parent);
        suite.check(editor.parent_of(child) == parent, "child has parent");
        suite.check(editor.children_of(parent).size() == 1, "parent has one child");

        editor.set_parent(child, parent2); // reparent
        suite.check(editor.parent_of(child) == parent2, "child reparented");
        suite.check(editor.children_of(parent).empty(), "old parent lost child");

        editor.detach(child);
        suite.check(editor.parent_of(child).is_null(), "detached child has no parent");
    }

    // Inspector_Reflection_ListsAndEdits (8.3)
    {
        struct Health {
            f32 current = 100.0f;
            f32 maximum = 100.0f;
        };

        Registry reg;
        const Entity e = reg.create_entity();
        const Entity bare = reg.create_entity();
        reg.add_component<Health>(e, Health{});

        Inspector inspector;
        inspector.register_property<Health>("Health", "current", &Health::current);
        inspector.register_property<Health>("Health", "maximum", &Health::maximum);

        const auto props = inspector.properties_of(reg, e);
        suite.check(props.size() == 2, "entity exposes two Health properties");
        suite.check(inspector.properties_of(reg, bare).empty(), "bare entity exposes none");

        props[0]->set(reg, e, 42.0f);
        suite.check(props[0]->get(reg, e) == 42.0f, "property set/get round-trips");
        suite.check(reg.get_component<Health>(e)->current == 42.0f, "edit reached the component");
    }

    // Gizmo_Apply_TranslateAndSnap (8.5)
    {
        TransformComponent t;
        gizmo::apply(t, GizmoMode::Translate, Vec3{1.24f, 0.0f, 0.0f}, 0.5f);
        suite.check(t.position.x == 1.0f, "translate snapped to 0.5 grid");
        gizmo::apply(t, GizmoMode::Scale, Vec3{-5.0f, 0.0f, 0.0f});
        suite.check(t.scale.x == 0.01f, "scale clamped at minimum");
        gizmo::apply(t, GizmoMode::Rotate, Vec3{0.5f, 0.0f, 0.0f});
        suite.check(t.rotation.x == 0.5f, "rotation applied unsnapped");
    }

    // AssetBrowser_ClassifyAndFilter (8.4)
    {
        suite.check(AssetBrowser::classify("ship.obj") == AssetType::Model, "obj is model");
        suite.check(AssetBrowser::classify("hero.PNG") == AssetType::Texture, "png is texture (case-insensitive)");
        suite.check(AssetBrowser::classify("theme.ogg") == AssetType::Audio, "ogg is audio");
        suite.check(AssetBrowser::classify("ai.vs") == AssetType::Script, "vs is script");
        suite.check(AssetBrowser::classify("README") == AssetType::Other, "no extension is other");
    }

    return suite.summary();
}

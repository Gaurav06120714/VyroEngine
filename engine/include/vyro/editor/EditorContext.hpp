// VyroEngine — Editor core
// Phase 8.1/8.2: the editor's headless model layer — selection, an undoable
// command stack, and scene-hierarchy operations. UI panels (ImGui) render this
// state; keeping the model UI-free makes every editor operation testable.
#pragma once

#include "vyro/core/Types.hpp"
#include "vyro/ecs/Entity.hpp"
#include "vyro/ecs/Registry.hpp"

#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace vyro {

// A reversible editor operation (8.1). Concrete commands capture the state
// they need to undo themselves.
class IEditorCommand
{
public:
    virtual ~IEditorCommand() = default;
    virtual void execute(Registry& registry) = 0;
    virtual void undo(Registry& registry) = 0;
    [[nodiscard]] virtual std::string_view name() const = 0;
};

// Hierarchy component: parent/children links between entities (8.2).
struct HierarchyComponent {
    Entity parent = kNullEntity;
    std::vector<Entity> children;
};

class EditorContext : NonCopyable
{
public:
    explicit EditorContext(Registry& registry) : m_registry(registry) {}

    // ── Selection ────────────────────────────────────────────────────
    void select(Entity entity) { m_selection = entity; }
    void clear_selection() { m_selection = kNullEntity; }
    [[nodiscard]] Entity selected() const { return m_selection; }
    [[nodiscard]] bool has_selection() const { return !m_selection.is_null(); }

    // ── Command stack (8.1) ──────────────────────────────────────────
    void execute(std::unique_ptr<IEditorCommand> command);
    bool undo();
    bool redo();
    [[nodiscard]] usize undo_depth() const { return m_undo.size(); }
    [[nodiscard]] usize redo_depth() const { return m_redo.size(); }

    // ── Hierarchy operations (8.2) ───────────────────────────────────
    // Make `child` a child of `parent` (detaching from any previous parent).
    void set_parent(Entity child, Entity parent);
    void detach(Entity child);
    [[nodiscard]] Entity parent_of(Entity entity);
    [[nodiscard]] std::vector<Entity> children_of(Entity entity);

    [[nodiscard]] Registry& registry() { return m_registry; }

private:
    Registry& m_registry;
    Entity m_selection = kNullEntity;
    std::vector<std::unique_ptr<IEditorCommand>> m_undo;
    std::vector<std::unique_ptr<IEditorCommand>> m_redo;
};

} // namespace vyro

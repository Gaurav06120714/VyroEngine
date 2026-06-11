// VyroEngine — Editor core implementation
#include "vyro/editor/EditorContext.hpp"

#include <algorithm>

namespace vyro {

void EditorContext::execute(std::unique_ptr<IEditorCommand> command)
{
    command->execute(m_registry);
    m_undo.push_back(std::move(command));
    m_redo.clear(); // a new action invalidates the redo branch
}

bool EditorContext::undo()
{
    if (m_undo.empty()) {
        return false;
    }
    auto command = std::move(m_undo.back());
    m_undo.pop_back();
    command->undo(m_registry);
    m_redo.push_back(std::move(command));
    return true;
}

bool EditorContext::redo()
{
    if (m_redo.empty()) {
        return false;
    }
    auto command = std::move(m_redo.back());
    m_redo.pop_back();
    command->execute(m_registry);
    m_undo.push_back(std::move(command));
    return true;
}

void EditorContext::set_parent(Entity child, Entity parent)
{
    detach(child);

    if (!m_registry.has_component<HierarchyComponent>(child)) {
        m_registry.add_component<HierarchyComponent>(child, HierarchyComponent{});
    }
    if (!m_registry.has_component<HierarchyComponent>(parent)) {
        m_registry.add_component<HierarchyComponent>(parent, HierarchyComponent{});
    }
    m_registry.get_component<HierarchyComponent>(child)->parent = parent;
    m_registry.get_component<HierarchyComponent>(parent)->children.push_back(child);
}

void EditorContext::detach(Entity child)
{
    auto* h = m_registry.get_component<HierarchyComponent>(child);
    if (h == nullptr || h->parent.is_null()) {
        return;
    }
    auto* p = m_registry.get_component<HierarchyComponent>(h->parent);
    if (p != nullptr) {
        p->children.erase(std::remove(p->children.begin(), p->children.end(), child),
                          p->children.end());
    }
    h->parent = kNullEntity;
}

Entity EditorContext::parent_of(Entity entity)
{
    const auto* h = m_registry.get_component<HierarchyComponent>(entity);
    return h != nullptr ? h->parent : kNullEntity;
}

std::vector<Entity> EditorContext::children_of(Entity entity)
{
    const auto* h = m_registry.get_component<HierarchyComponent>(entity);
    return h != nullptr ? h->children : std::vector<Entity>{};
}

} // namespace vyro

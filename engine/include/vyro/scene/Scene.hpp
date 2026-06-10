// VyroEngine — Scene
// Phase 2.4: a scene bundles an ECS registry with its system manager under a
// name. It is the unit of content the scene manager loads, updates, and
// unloads. Serialization is layered on in a later phase.
#pragma once

#include "vyro/core/Types.hpp"
#include "vyro/ecs/Entity.hpp"
#include "vyro/ecs/Registry.hpp"
#include "vyro/ecs/SystemManager.hpp"

#include <string>
#include <utility>

namespace vyro {

class Scene : NonCopyable
{
public:
    explicit Scene(std::string name) : m_name(std::move(name)), m_systems(m_registry) {}

    [[nodiscard]] Registry& registry() { return m_registry; }
    [[nodiscard]] SystemManager& systems() { return m_systems; }
    [[nodiscard]] const std::string& name() const { return m_name; }

    [[nodiscard]] Entity create_entity() { return m_registry.create_entity(); }
    void destroy_entity(Entity entity) { m_registry.destroy_entity(entity); }

    // Advance the scene by running its systems.
    void update(f32 dt) { m_systems.update(dt); }

    [[nodiscard]] usize entity_count() const { return m_registry.entity_count(); }

private:
    std::string m_name;
    Registry m_registry;          // declared before m_systems (init order)
    SystemManager m_systems;
};

} // namespace vyro

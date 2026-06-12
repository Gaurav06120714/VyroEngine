// VyroEngine — ECS registry (World)
// Phase 2.2: composes the entity manager with per-type component storage and
// exposes a query view. This is the structural heart that holds all
// simulation state; gameplay systems operate over its component data.
#pragma once

#include "vyro/core/Types.hpp"
#include "vyro/ecs/ComponentStorage.hpp"
#include "vyro/ecs/Entity.hpp"
#include "vyro/ecs/EntityManager.hpp"

#include <memory>
#include <tuple>
#include <typeindex>
#include <unordered_map>
#include <utility>

namespace vyro {

class Registry;

// A lazily-evaluated query over entities possessing all of Components...
template<typename... Components>
class View
{
public:
    explicit View(Registry& registry) : m_registry(&registry) {}

    // Invoke f(Components&...) for every matching entity.
    template<typename Func>
    void for_each(Func&& f);

    // Invoke f(Entity, Components&...) for every matching entity.
    template<typename Func>
    void for_each_entity(Func&& f);

private:
    Registry* m_registry;
};

class Registry : NonCopyable
{
public:
    Registry() = default;

    // ── Entities ─────────────────────────────────────────────────────
    [[nodiscard]] Entity create_entity() { return m_entities.create(); }

    void destroy_entity(Entity entity)
    {
        if (!m_entities.is_alive(entity)) {
            return;
        }
        for (auto& [type, pool] : m_pools) {
            pool->remove(entity);
        }
        m_entities.destroy(entity);
    }

    [[nodiscard]] bool is_alive(Entity entity) const { return m_entities.is_alive(entity); }
    [[nodiscard]] usize entity_count() const { return m_entities.alive_count(); }
    [[nodiscard]] std::vector<Entity> alive_entities() const { return m_entities.alive_entities(); }

    // ── Components ───────────────────────────────────────────────────
    template<typename T, typename... Args>
    T& add_component(Entity entity, Args&&... args)
    {
        return storage<T>().emplace(entity, std::forward<Args>(args)...);
    }

    template<typename T>
    [[nodiscard]] T* get_component(Entity entity)
    {
        return storage<T>().get(entity);
    }

    template<typename T>
    [[nodiscard]] bool has_component(Entity entity)
    {
        return storage<T>().has(entity);
    }

    template<typename T>
    void remove_component(Entity entity)
    {
        storage<T>().remove(entity);
    }

    // Lazily create and return the storage pool for component type T.
    template<typename T>
    [[nodiscard]] ComponentStorage<T>& storage()
    {
        const std::type_index key(typeid(T));
        auto it = m_pools.find(key);
        if (it == m_pools.end()) {
            it = m_pools.emplace(key, std::make_unique<ComponentStorage<T>>()).first;
        }
        return *static_cast<ComponentStorage<T>*>(it->second.get());
    }

    // ── Queries ──────────────────────────────────────────────────────
    template<typename... Components>
    [[nodiscard]] View<Components...> view()
    {
        return View<Components...>(*this);
    }

private:
    EntityManager m_entities;
    std::unordered_map<std::type_index, std::unique_ptr<IComponentStorage>> m_pools;
};

// ── View definitions (Registry now complete) ─────────────────────────
template<typename... Components>
template<typename Func>
void View<Components...>::for_each(Func&& f)
{
    using First = std::tuple_element_t<0, std::tuple<Components...>>;
    auto& driver = m_registry->storage<First>();
    // Copy the driver's entity list so the callback may add/remove components
    // without invalidating the iteration (structural-safe).
    const std::vector<Entity> entities(driver.entities().begin(), driver.entities().end());
    for (const Entity e : entities) {
        if ((m_registry->has_component<Components>(e) && ...)) {
            f(*m_registry->get_component<Components>(e)...);
        }
    }
}

template<typename... Components>
template<typename Func>
void View<Components...>::for_each_entity(Func&& f)
{
    using First = std::tuple_element_t<0, std::tuple<Components...>>;
    auto& driver = m_registry->storage<First>();
    const std::vector<Entity> entities(driver.entities().begin(), driver.entities().end());
    for (const Entity e : entities) {
        if ((m_registry->has_component<Components>(e) && ...)) {
            f(e, *m_registry->get_component<Components>(e)...);
        }
    }
}

} // namespace vyro

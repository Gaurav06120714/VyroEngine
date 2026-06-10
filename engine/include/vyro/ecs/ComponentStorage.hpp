// VyroEngine — Component storage
// Phase 2.2: sparse-set storage. Components live in a contiguous dense array
// for cache-friendly iteration (rulz/MEMORY_RULES M-10); a sparse array maps
// entity index -> dense position. Removal is swap-and-pop, O(1).
#pragma once

#include "vyro/core/Types.hpp"
#include "vyro/ecs/Entity.hpp"

#include <span>
#include <utility>
#include <vector>

namespace vyro {

// Type-erased interface so the registry can remove components of any type when
// an entity is destroyed.
class IComponentStorage
{
public:
    virtual ~IComponentStorage() = default;
    virtual void remove(Entity entity) = 0;
    [[nodiscard]] virtual bool has(Entity entity) const = 0;
};

template<typename T>
class ComponentStorage final : public IComponentStorage
{
public:
    static constexpr u32 kNone = 0xFFFFFFFFu;

    // Insert or overwrite the component for `entity`. Returns a reference.
    template<typename... Args>
    T& emplace(Entity entity, Args&&... args)
    {
        const u32 idx = entity.index;
        ensure_sparse(idx);
        if (m_sparse[idx] != kNone) {
            T& existing = m_dense[m_sparse[idx]];
            existing = T(std::forward<Args>(args)...);
            return existing;
        }
        m_sparse[idx] = static_cast<u32>(m_dense.size());
        m_dense.emplace_back(std::forward<Args>(args)...);
        m_entities.push_back(entity);
        return m_dense.back();
    }

    [[nodiscard]] bool has(Entity entity) const override
    {
        return entity.index < m_sparse.size() && m_sparse[entity.index] != kNone;
    }

    [[nodiscard]] T* get(Entity entity)
    {
        if (!has(entity)) {
            return nullptr;
        }
        return &m_dense[m_sparse[entity.index]];
    }

    [[nodiscard]] const T* get(Entity entity) const
    {
        if (!has(entity)) {
            return nullptr;
        }
        return &m_dense[m_sparse[entity.index]];
    }

    void remove(Entity entity) override
    {
        if (!has(entity)) {
            return;
        }
        const u32 pos = m_sparse[entity.index];
        const u32 last = static_cast<u32>(m_dense.size() - 1);

        // Swap-and-pop the removed slot with the last element.
        m_dense[pos] = std::move(m_dense[last]);
        m_entities[pos] = m_entities[last];
        m_sparse[m_entities[pos].index] = pos;

        m_dense.pop_back();
        m_entities.pop_back();
        m_sparse[entity.index] = kNone;
    }

    [[nodiscard]] usize size() const { return m_dense.size(); }

    // Dense, contiguous views for iteration.
    [[nodiscard]] std::span<const Entity> entities() const { return m_entities; }
    [[nodiscard]] std::span<T> data() { return m_dense; }

private:
    void ensure_sparse(u32 index)
    {
        if (index >= m_sparse.size()) {
            m_sparse.resize(index + 1, kNone);
        }
    }

    std::vector<T> m_dense;            // packed components
    std::vector<Entity> m_entities;    // entity per dense slot (parallel)
    std::vector<u32> m_sparse;         // entity index -> dense position
};

} // namespace vyro

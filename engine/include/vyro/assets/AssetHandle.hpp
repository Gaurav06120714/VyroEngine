// VyroEngine — Asset handle
// Phase 1.7: a reference-counted, eviction-aware handle to a managed asset.
// Game code holds handles, never raw asset pointers (rulz/MEMORY_RULES M-9).
#pragma once

#include "vyro/core/Types.hpp"

namespace vyro {

using AssetId = u64;

inline constexpr AssetId kInvalidAsset = 0;

class AssetManager;

template<typename T>
class AssetHandle
{
public:
    AssetHandle() = default;

    AssetHandle(AssetManager* manager, AssetId id, u32 generation)
        : m_manager(manager), m_id(id), m_generation(generation)
    {
    }

    AssetHandle(const AssetHandle& other)
        : m_manager(other.m_manager), m_id(other.m_id), m_generation(other.m_generation)
    {
        add_ref();
    }

    AssetHandle& operator=(const AssetHandle& other)
    {
        if (this != &other) {
            release();
            m_manager = other.m_manager;
            m_id = other.m_id;
            m_generation = other.m_generation;
            add_ref();
        }
        return *this;
    }

    AssetHandle(AssetHandle&& other) noexcept
        : m_manager(other.m_manager), m_id(other.m_id), m_generation(other.m_generation)
    {
        other.m_manager = nullptr;
        other.m_id = kInvalidAsset;
    }

    AssetHandle& operator=(AssetHandle&& other) noexcept
    {
        if (this != &other) {
            release();
            m_manager = other.m_manager;
            m_id = other.m_id;
            m_generation = other.m_generation;
            other.m_manager = nullptr;
            other.m_id = kInvalidAsset;
        }
        return *this;
    }

    ~AssetHandle() { release(); }

    // Resolve the underlying asset. Returns nullptr if the asset was evicted
    // or this handle is stale (generation mismatch).
    [[nodiscard]] T* get() const;

    [[nodiscard]] bool is_valid() const { return get() != nullptr; }

    T* operator->() const { return get(); }
    T& operator*() const { return *get(); }

    [[nodiscard]] AssetId id() const { return m_id; }

private:
    void add_ref() const;
    void release();

    AssetManager* m_manager = nullptr;
    AssetId m_id = kInvalidAsset;
    u32 m_generation = 0;
};

} // namespace vyro

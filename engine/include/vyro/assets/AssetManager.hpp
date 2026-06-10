// VyroEngine — Asset manager
// Phase 1.7: owns loaded assets, reference-counts them via AssetHandle, and
// supports hot-reload by replacing an asset's data in place. Assets are
// type-erased behind a polymorphic storage holder owned by the manager.
#pragma once

#include "vyro/assets/AssetHandle.hpp"
#include "vyro/core/Types.hpp"

#include <memory>
#include <string_view>
#include <typeindex>
#include <unordered_map>
#include <utility>

namespace vyro {

class AssetManager : NonCopyable
{
public:
    AssetManager() = default;
    ~AssetManager() = default;

    // Load (or acquire an existing) asset constructed in place from Args. If
    // the asset already exists, its reference count is incremented and the
    // existing instance is returned.
    template<typename T, typename... Args>
    AssetHandle<T> load(std::string_view name, Args&&... args)
    {
        const AssetId id = hash(name);
        auto it = m_assets.find(id);
        if (it != m_assets.end()) {
            ++it->second.ref_count;
            return AssetHandle<T>(this, id, it->second.generation);
        }

        ControlBlock cb;
        cb.ref_count = 1;
        cb.generation = ++m_generations[id];
        cb.type = std::type_index(typeid(T));
        cb.storage = std::make_unique<Storage<T>>(std::forward<Args>(args)...);
        const u32 generation = cb.generation;
        m_assets.emplace(id, std::move(cb));
        return AssetHandle<T>(this, id, generation);
    }

    // Replace an existing asset's data in place (hot reload). Existing handles
    // remain valid and observe the new data. No-op if the asset is absent.
    template<typename T, typename... Args>
    void reload(std::string_view name, Args&&... args)
    {
        const auto it = m_assets.find(hash(name));
        if (it == m_assets.end()) {
            return;
        }
        it->second.storage = std::make_unique<Storage<T>>(std::forward<Args>(args)...);
        it->second.type = std::type_index(typeid(T));
    }

    [[nodiscard]] usize asset_count() const { return m_assets.size(); }
    [[nodiscard]] bool is_loaded(std::string_view name) const { return m_assets.contains(hash(name)); }

    // FNV-1a hash of an asset name/path.
    [[nodiscard]] static AssetId hash(std::string_view name);

private:
    template<typename>
    friend class AssetHandle;

    struct IStorage {
        virtual ~IStorage() = default;
        [[nodiscard]] virtual void* data() = 0;
    };

    template<typename T>
    struct Storage final : IStorage {
        template<typename... Args>
        explicit Storage(Args&&... args) : value(std::forward<Args>(args)...) {}
        [[nodiscard]] void* data() override { return &value; }
        T value;
    };

    struct ControlBlock {
        u32 ref_count = 0;
        u32 generation = 0;
        std::type_index type = std::type_index(typeid(void));
        std::unique_ptr<IStorage> storage;
    };

    // Handle support (called by AssetHandle).
    void add_ref(AssetId id);
    void release(AssetId id);
    [[nodiscard]] u32 generation(AssetId id) const;
    [[nodiscard]] void* raw_data(AssetId id, u32 expected_generation) const;

    std::unordered_map<AssetId, ControlBlock> m_assets;
    std::unordered_map<AssetId, u32> m_generations;
};

// ── AssetHandle out-of-line definitions (AssetManager now complete) ──
template<typename T>
T* AssetHandle<T>::get() const
{
    if (m_manager == nullptr || m_id == kInvalidAsset) {
        return nullptr;
    }
    return static_cast<T*>(m_manager->raw_data(m_id, m_generation));
}

template<typename T>
void AssetHandle<T>::add_ref() const
{
    if (m_manager != nullptr && m_id != kInvalidAsset) {
        m_manager->add_ref(m_id);
    }
}

template<typename T>
void AssetHandle<T>::release()
{
    if (m_manager != nullptr && m_id != kInvalidAsset) {
        m_manager->release(m_id);
        m_manager = nullptr;
        m_id = kInvalidAsset;
    }
}

} // namespace vyro

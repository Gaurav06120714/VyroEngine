// VyroEngine — Asset manager implementation
#include "vyro/assets/AssetManager.hpp"

namespace vyro {

AssetId AssetManager::hash(std::string_view name)
{
    // FNV-1a 64-bit.
    constexpr u64 kOffset = 14695981039346656037ULL;
    constexpr u64 kPrime = 1099511628211ULL;
    u64 h = kOffset;
    for (const char c : name) {
        h ^= static_cast<u8>(c);
        h *= kPrime;
    }
    // Reserve 0 as the invalid id.
    return h == kInvalidAsset ? 1 : h;
}

void AssetManager::add_ref(AssetId id)
{
    const auto it = m_assets.find(id);
    if (it != m_assets.end()) {
        ++it->second.ref_count;
    }
}

void AssetManager::release(AssetId id)
{
    const auto it = m_assets.find(id);
    if (it == m_assets.end()) {
        return;
    }
    if (--it->second.ref_count == 0) {
        m_assets.erase(it); // evict; m_generations retains the last generation
    }
}

u32 AssetManager::generation(AssetId id) const
{
    const auto it = m_assets.find(id);
    return it != m_assets.end() ? it->second.generation : 0;
}

void* AssetManager::raw_data(AssetId id, u32 expected_generation) const
{
    const auto it = m_assets.find(id);
    if (it == m_assets.end() || it->second.generation != expected_generation) {
        return nullptr;
    }
    return it->second.storage->data();
}

} // namespace vyro

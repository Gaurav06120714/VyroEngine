// VyroEngine — Pool allocator
// Phase 1.6: O(1) allocation/deallocation of fixed-size T objects backed by an
// intrusive free list. Ideal for components, entities, and audio sources
// (rulz/MEMORY_RULES M-2). Slots are recycled in LIFO order.
#pragma once

#include "vyro/core/Types.hpp"

#include <algorithm>
#include <cstdlib>
#include <new>
#include <utility>

namespace vyro::memory {

template<typename T>
class PoolAllocator : NonCopyable
{
public:
    explicit PoolAllocator(usize count)
        : m_count(count)
    {
        // Each slot must hold either a T or a free-list link.
        m_slot_size = std::max(sizeof(T), sizeof(FreeNode));
        const usize alignment = std::max(alignof(T), alignof(FreeNode));
        m_slot_size = (m_slot_size + (alignment - 1)) & ~(alignment - 1);

        m_storage = static_cast<byte*>(std::malloc(m_slot_size * count));

        // Thread every slot onto the free list.
        m_free = nullptr;
        for (usize i = 0; i < count; ++i) {
            byte* slot = m_storage + (count - 1 - i) * m_slot_size;
            auto* node = reinterpret_cast<FreeNode*>(slot);
            node->next = m_free;
            m_free = node;
        }
    }

    ~PoolAllocator()
    {
        std::free(m_storage);
    }

    // Construct a T in a free slot. Returns nullptr if the pool is full.
    template<typename... Args>
    [[nodiscard]] T* allocate(Args&&... args)
    {
        if (m_free == nullptr) {
            return nullptr;
        }
        FreeNode* node = m_free;
        m_free = node->next;
        ++m_allocated;
        return new (node) T(std::forward<Args>(args)...);
    }

    // Destroy a previously allocated object and return its slot to the pool.
    void deallocate(T* ptr)
    {
        if (ptr == nullptr) {
            return;
        }
        ptr->~T();
        auto* node = reinterpret_cast<FreeNode*>(ptr);
        node->next = m_free;
        m_free = node;
        --m_allocated;
    }

    [[nodiscard]] usize capacity() const { return m_count; }
    [[nodiscard]] usize allocated() const { return m_allocated; }
    [[nodiscard]] bool full() const { return m_free == nullptr; }

private:
    struct FreeNode {
        FreeNode* next;
    };

    byte* m_storage = nullptr;
    usize m_count = 0;
    usize m_slot_size = 0;
    usize m_allocated = 0;
    FreeNode* m_free = nullptr;
};

} // namespace vyro::memory

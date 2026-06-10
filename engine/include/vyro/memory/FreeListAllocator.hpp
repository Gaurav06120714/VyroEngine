// VyroEngine — Free-list allocator
// Phase 1.6: general-purpose allocator for arbitrary sizes. First-fit search
// with block splitting and coalescing on free (rulz/MEMORY_RULES M-2). Use as
// the fallback for large one-off allocations.
#pragma once

#include "vyro/core/Types.hpp"

namespace vyro::memory {

class FreeListAllocator : NonCopyable
{
public:
    explicit FreeListAllocator(usize capacity);
    ~FreeListAllocator();

    // Allocate `size` bytes. Alignment is raised to at least 16 so allocation
    // and free-block headers stay aligned. Returns nullptr if no block fits.
    [[nodiscard]] void* allocate(usize size, usize alignment = 16);

    // Return a block previously returned by allocate().
    void deallocate(void* ptr);

    [[nodiscard]] usize capacity() const { return m_capacity; }
    [[nodiscard]] usize used() const { return m_used; }

private:
    struct FreeBlock {
        usize size;       // total bytes of this free region (header included)
        FreeBlock* next;  // next free region, address-ordered
    };

    struct AllocHeader {
        usize size;        // total reserved bytes (adjustment + payload)
        usize adjustment;  // bytes from region start to the returned pointer
    };

    void coalesce(FreeBlock* prev, FreeBlock* block);

    byte* m_base = nullptr;
    usize m_capacity = 0;
    usize m_used = 0;
    FreeBlock* m_free = nullptr;
};

} // namespace vyro::memory

// VyroEngine — Free-list allocator implementation
#include "vyro/memory/FreeListAllocator.hpp"

#include "vyro/memory/Align.hpp"

#include <algorithm>
#include <cstdlib>

namespace vyro::memory {

namespace {
constexpr usize kMinAlignment = 16;
} // namespace

FreeListAllocator::FreeListAllocator(usize capacity)
{
    // Round capacity up so the whole buffer is a clean multiple of alignment.
    m_capacity = align_up(std::max(capacity, sizeof(FreeBlock)), kMinAlignment);
    m_base = static_cast<byte*>(std::malloc(m_capacity));

    // One free region spanning the whole buffer.
    m_free = reinterpret_cast<FreeBlock*>(m_base);
    m_free->size = m_capacity;
    m_free->next = nullptr;
}

FreeListAllocator::~FreeListAllocator()
{
    std::free(m_base);
}

void* FreeListAllocator::allocate(usize size, usize alignment)
{
    alignment = std::max(alignment, kMinAlignment);
    const usize payload = align_up(size, kMinAlignment);
    // The returned pointer sits `sizeof(AllocHeader)` past the region start (sizeof(AllocHeader)
    // is a multiple of 16, so alignment is preserved) and the header lives in
    // the gap before it.
    const usize total = sizeof(AllocHeader) + payload;

    FreeBlock* prev = nullptr;
    FreeBlock* block = m_free;
    while (block != nullptr) {
        if (block->size >= total) {
            break;
        }
        prev = block;
        block = block->next;
    }
    if (block == nullptr) {
        return nullptr; // no fit
    }

    const usize remaining = block->size - total;
    FreeBlock* next_free = block->next;

    byte* region = reinterpret_cast<byte*>(block);
    if (remaining >= sizeof(FreeBlock) + kMinAlignment) {
        // Split: carve a new free block from the tail.
        auto* split = reinterpret_cast<FreeBlock*>(region + total);
        split->size = remaining;
        split->next = next_free;
        next_free = split;
    } else {
        // Absorb the remainder into this allocation.
        const usize absorbed = block->size - total;
        m_used += absorbed;
    }

    // Unlink the consumed block.
    if (prev == nullptr) {
        m_free = next_free;
    } else {
        prev->next = next_free;
    }

    auto* header = reinterpret_cast<AllocHeader*>(region);
    header->size = total;
    header->adjustment = sizeof(AllocHeader);
    m_used += total;
    return region + sizeof(AllocHeader);
}

void FreeListAllocator::deallocate(void* ptr)
{
    if (ptr == nullptr) {
        return;
    }

    auto* raw = static_cast<byte*>(ptr);
    auto* header = reinterpret_cast<AllocHeader*>(raw - sizeof(AllocHeader));
    byte* region = raw - header->adjustment;
    const usize size = header->size;
    m_used -= size;

    auto* block = reinterpret_cast<FreeBlock*>(region);
    block->size = size;

    // Insert address-ordered into the free list.
    FreeBlock* prev = nullptr;
    FreeBlock* cur = m_free;
    while (cur != nullptr && cur < block) {
        prev = cur;
        cur = cur->next;
    }
    block->next = cur;
    if (prev == nullptr) {
        m_free = block;
    } else {
        prev->next = block;
    }

    coalesce(prev, block);
}

void FreeListAllocator::coalesce(FreeBlock* prev, FreeBlock* block)
{
    // Merge with the following block if adjacent.
    auto* block_end = reinterpret_cast<byte*>(block) + block->size;
    if (block->next != nullptr && block_end == reinterpret_cast<byte*>(block->next)) {
        block->size += block->next->size;
        block->next = block->next->next;
    }
    // Merge with the previous block if adjacent.
    if (prev != nullptr) {
        auto* prev_end = reinterpret_cast<byte*>(prev) + prev->size;
        if (prev_end == reinterpret_cast<byte*>(block)) {
            prev->size += block->size;
            prev->next = block->next;
        }
    }
}

} // namespace vyro::memory

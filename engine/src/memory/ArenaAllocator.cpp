// VyroEngine — Arena allocator implementation
#include "vyro/memory/ArenaAllocator.hpp"

#include "vyro/memory/Align.hpp"

#include <cstdlib>

namespace vyro::memory {

// Allocators are the sanctioned boundary to the OS heap; client engine code
// uses these allocators rather than malloc/new directly (rulz/MEMORY_RULES).
ArenaAllocator::ArenaAllocator(usize capacity)
    : m_capacity(capacity)
{
    m_base = static_cast<byte*>(std::malloc(capacity));
}

ArenaAllocator::~ArenaAllocator()
{
    std::free(m_base);
}

void* ArenaAllocator::allocate(usize size, usize alignment)
{
    const usize current = reinterpret_cast<usize>(m_base) + m_offset;
    const usize aligned = align_up(current, alignment);
    const usize padding = aligned - current;

    if (m_offset + padding + size > m_capacity) {
        return nullptr; // exhausted
    }

    m_offset += padding + size;
    return reinterpret_cast<void*>(aligned);
}

void ArenaAllocator::reset()
{
    m_offset = 0;
}

} // namespace vyro::memory

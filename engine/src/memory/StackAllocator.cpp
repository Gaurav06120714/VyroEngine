// VyroEngine — Stack allocator implementation
#include "vyro/memory/StackAllocator.hpp"

#include "vyro/memory/Align.hpp"

#include <cstdlib>

namespace vyro::memory {

StackAllocator::StackAllocator(usize capacity)
    : m_capacity(capacity)
{
    m_base = static_cast<byte*>(std::malloc(capacity));
}

StackAllocator::~StackAllocator()
{
    std::free(m_base);
}

void* StackAllocator::allocate(usize size, usize alignment)
{
    const usize current = reinterpret_cast<usize>(m_base) + m_offset;
    const usize aligned = align_up(current, alignment);
    const usize padding = aligned - current;

    if (m_offset + padding + size > m_capacity) {
        return nullptr;
    }

    m_offset += padding + size;
    return reinterpret_cast<void*>(aligned);
}

void StackAllocator::rewind(Marker marker)
{
    if (marker <= m_offset) {
        m_offset = marker;
    }
}

void StackAllocator::reset()
{
    m_offset = 0;
}

} // namespace vyro::memory

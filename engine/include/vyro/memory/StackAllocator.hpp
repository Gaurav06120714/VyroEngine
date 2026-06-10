// VyroEngine — Stack allocator
// Phase 1.6: LIFO allocation with markers for nested scopes. Allocations are
// freed by rewinding to a previously captured marker (rulz/MEMORY_RULES M-2).
#pragma once

#include "vyro/core/Types.hpp"

#include <new>

namespace vyro::memory {

class StackAllocator : NonCopyable
{
public:
    using Marker = usize;

    explicit StackAllocator(usize capacity);
    ~StackAllocator();

    [[nodiscard]] void* allocate(usize size, usize alignment = alignof(std::max_align_t));

    template<typename T>
    [[nodiscard]] T* allocate_array(usize count)
    {
        return static_cast<T*>(allocate(sizeof(T) * count, alignof(T)));
    }

    // Capture the current top of the stack.
    [[nodiscard]] Marker get_marker() const { return m_offset; }

    // Free everything allocated since `marker` was captured.
    void rewind(Marker marker);

    // Free everything.
    void reset();

    [[nodiscard]] usize used() const { return m_offset; }
    [[nodiscard]] usize capacity() const { return m_capacity; }

private:
    byte* m_base = nullptr;
    usize m_capacity = 0;
    usize m_offset = 0;
};

} // namespace vyro::memory

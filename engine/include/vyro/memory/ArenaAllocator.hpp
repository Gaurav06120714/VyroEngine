// VyroEngine — Arena (linear) allocator
// Phase 1.6: O(1) bump allocation freed all at once via reset(). Ideal for
// per-frame scratch data and bulk scene/asset loads (rulz/MEMORY_RULES M-2).
#pragma once

#include "vyro/core/Types.hpp"

#include <new>
#include <utility>

namespace vyro::memory {

class ArenaAllocator : NonCopyable
{
public:
    explicit ArenaAllocator(usize capacity);
    ~ArenaAllocator();

    // Bump-allocate `size` bytes with the given alignment. Returns nullptr if
    // the arena is exhausted.
    [[nodiscard]] void* allocate(usize size, usize alignment = alignof(std::max_align_t));

    // Allocate (uninitialized) storage for `count` objects of type T.
    template<typename T>
    [[nodiscard]] T* allocate_array(usize count)
    {
        return static_cast<T*>(allocate(sizeof(T) * count, alignof(T)));
    }

    // Allocate and construct a single T in the arena (no destructor is run on
    // reset — use only for trivially destructible or arena-scoped objects).
    template<typename T, typename... Args>
    [[nodiscard]] T* create(Args&&... args)
    {
        void* mem = allocate(sizeof(T), alignof(T));
        return mem ? new (mem) T(std::forward<Args>(args)...) : nullptr;
    }

    // Free everything at once.
    void reset();

    [[nodiscard]] usize used() const { return m_offset; }
    [[nodiscard]] usize capacity() const { return m_capacity; }

private:
    byte* m_base = nullptr;
    usize m_capacity = 0;
    usize m_offset = 0;
};

} // namespace vyro::memory

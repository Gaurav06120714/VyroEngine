// VyroEngine — Memory allocator tests
#include "vyro/memory/Align.hpp"
#include "vyro/memory/ArenaAllocator.hpp"
#include "vyro/memory/FreeListAllocator.hpp"
#include "vyro/memory/PoolAllocator.hpp"
#include "vyro/memory/StackAllocator.hpp"

#include "test_harness.hpp"

#include <cstdint>

namespace {

bool is_aligned(const void* p, vyro::usize a)
{
    return (reinterpret_cast<vyro::usize>(p) % a) == 0;
}

struct Widget {
    int a = 0;
    double b = 0.0;
};

} // namespace

int main()
{
    using namespace vyro;
    test::Suite suite("memory");

    // align_up rounds to the next multiple
    suite.check(memory::align_up(13, 16) == 16, "align_up(13,16)==16");
    suite.check(memory::align_up(16, 16) == 16, "align_up(16,16)==16");
    suite.check(memory::is_power_of_two(64), "64 is power of two");
    suite.check(!memory::is_power_of_two(48), "48 is not power of two");

    // Arena_Allocate_ReturnsAlignedAndAdvances
    {
        memory::ArenaAllocator arena(1024);
        void* a = arena.allocate(10, 16);
        void* b = arena.allocate(10, 16);
        suite.check(a != nullptr && is_aligned(a, 16), "arena alloc aligned");
        suite.check(b > a, "arena bumps forward");
        suite.check(arena.used() > 0, "arena tracks usage");
        arena.reset();
        suite.check(arena.used() == 0, "arena reset frees all");
    }

    // Arena_Exhaustion_ReturnsNull
    {
        memory::ArenaAllocator arena(32);
        suite.check(arena.allocate(1000) == nullptr, "arena returns null when exhausted");
    }

    // Stack_Rewind_ReleasesToMarker
    {
        memory::StackAllocator stack(1024);
        suite.check(stack.allocate(64) != nullptr, "stack allocates first block");
        auto marker = stack.get_marker();
        suite.check(stack.allocate(128) != nullptr, "stack allocates second block");
        suite.check(stack.used() > marker, "stack grew past marker");
        stack.rewind(marker);
        suite.check(stack.used() == marker, "stack rewound to marker");
    }

    // Pool_AllocateDeallocate_ReusesSlot
    {
        memory::PoolAllocator<Widget> pool(2);
        Widget* w1 = pool.allocate(1, 2.0);
        Widget* w2 = pool.allocate(3, 4.0);
        suite.check(w1 != nullptr && w2 != nullptr, "pool allocates two widgets");
        suite.check(w1->a == 1 && w2->a == 3, "pool constructs with args");
        suite.check(pool.full(), "pool full at capacity");
        suite.check(pool.allocate() == nullptr, "pool returns null when full");
        pool.deallocate(w1);
        Widget* w3 = pool.allocate(9, 9.0);
        suite.check(w3 == w1, "pool reuses freed slot");
    }

    // FreeList_AllocateFree_Roundtrips
    {
        memory::FreeListAllocator fl(4096);
        void* a = fl.allocate(100);
        void* b = fl.allocate(200);
        suite.check(a != nullptr && b != nullptr, "freelist allocates two blocks");
        suite.check(is_aligned(a, 16) && is_aligned(b, 16), "freelist blocks aligned");
        const usize used_before = fl.used();
        fl.deallocate(a);
        suite.check(fl.used() < used_before, "freelist usage drops after free");
        // Free b then a-region should coalesce; a large alloc must now fit.
        fl.deallocate(b);
        void* big = fl.allocate(3000);
        suite.check(big != nullptr, "freelist serves large alloc after coalesce");
    }

    return suite.summary();
}

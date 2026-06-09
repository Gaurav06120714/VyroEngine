# VyroEngine Memory Rules

**Version:** 1.0.0

---

## Core Rules

### Rule M-1: No Raw new/delete
Engine code never uses `new` or `delete` directly. Use allocators.

```cpp
// WRONG
Mesh* mesh = new Mesh();  // NO
delete mesh;               // NO

// CORRECT
auto* mesh = pool.allocate<Mesh>();
pool.deallocate(mesh);
```

Exception: `std::unique_ptr` with custom deleters in RAII wrappers is acceptable.

### Rule M-2: Choose the Right Allocator

| Allocator | Use Case | Example |
|-----------|---------|---------|
| `PoolAllocator<T>` | Many same-size objects, frequent alloc/free | Components, entities, audio sources |
| `StackAllocator` | Per-frame temporaries, LIFO order | Render command lists, temp matrices |
| `ArenaAllocator` | Bulk allocation, freed all at once | Scene load, asset batch |
| `FreeListAllocator` | General purpose, arbitrary sizes | Fallback, large one-off allocations |
| `std::pmr::*` | STL containers with custom backing | `pmr::vector<DrawCall>` on frame arena |

### Rule M-3: Zero Per-Frame Heap Allocations in Hot Path
The game loop update and render loop must not allocate from the OS heap. Profile with ASAN + heap tracking to verify.

```cpp
// WRONG: heap allocation every frame
void RenderSystem::OnUpdate() {
    std::vector<DrawCall> calls;  // heap allocation! NO
    ...
}

// CORRECT: frame allocator
void RenderSystem::OnUpdate() {
    auto* calls = m_frame_arena.allocate_array<DrawCall>(max_calls);
    ...
    m_frame_arena.reset(); // free all at end of frame
}
```

### Rule M-4: 64-Byte Alignment for Hot Data
All component arrays and frequently iterated data must be 64-byte aligned (one cache line).

```cpp
struct alignas(64) TransformComponent {
    glm::vec3 position;
    glm::quat rotation;
    glm::vec3 scale;
    // total: 40 bytes — fits in cache line
};
```

### Rule M-5: RAII for All Resources
Every resource that allocates memory must free it in its destructor. No manual cleanup required by callers.

### Rule M-6: Memory Budget Enforcement
Each subsystem has a memory budget. Exceeding it is a compile-time configurable assert in debug.

| Subsystem | Default Budget |
|-----------|---------------|
| ECS component storage | 512 MB |
| Renderer (GPU) | Configured by user |
| Audio cache | 128 MB |
| Asset cache | 256 MB |
| Script VM | 64 MB |
| Editor | 256 MB |

### Rule M-7: Track Allocations in Debug
Every allocator wraps allocations in debug builds:
- Records size, type name (via `typeid`), file, line
- Reports leaks on shutdown via `MemoryTracker::report()`
- Fills freed memory with `0xDEADBEEF` to detect use-after-free

### Rule M-8: No Circular Ownership
Ownership chains must be a DAG (directed acyclic graph). No shared ownership cycles.

```
Engine → Scene → Entity → Component ✓
Asset Manager ← Component (handle/reference only) ✓
Scene → Scene (circular ownership) ✗
```

### Rule M-9: Asset Handles, Not Pointers
Game code never holds raw pointers to assets. Use `AssetHandle<T>` which:
- Reference-counts the asset
- Detects use after asset eviction
- Thread-safe increment/decrement

### Rule M-10: Contiguous Over Pointer-Based
Prefer `std::vector` / arrays over linked lists. Cache misses from pointer chasing in linked lists cost 100ns+.

```cpp
// SLOW: pointer-based list
struct ComponentNode {
    Component* data;
    ComponentNode* next; // pointer chase = cache miss
};

// FAST: contiguous array
std::vector<Component> components; // sequential access = cache warm
```

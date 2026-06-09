# VyroEngine Coding Standards

**Version:** 1.0.0 | **Inspired by:** Godot Engine, LLVM, Google C++ Style

---

## Core Philosophy

1. Code is read 10x more than it is written. Optimize for reading.
2. Explicit is better than implicit.
3. Fail loudly in debug. Trust invariants in release.
4. No magic. Every behavior is explainable.

---

## Naming Conventions

| Entity | Style | Example |
|--------|-------|---------|
| Class | PascalCase | `RigidBody`, `SceneGraph` |
| Struct | PascalCase | `TransformComponent`, `RenderPassDesc` |
| Function / Method | snake_case | `get_entity()`, `submit_batch()` |
| Variable (local) | snake_case | `world_matrix`, `entity_count` |
| Variable (member) | `m_` prefix | `m_position`, `m_entity_id` |
| Variable (static) | `s_` prefix | `s_instance`, `s_pool` |
| Variable (global) | `g_` prefix | `g_engine`, `g_logger` |
| Constant | SCREAMING_SNAKE | `MAX_ENTITIES`, `FIXED_TIMESTEP` |
| Enum class | PascalCase | `RenderBackend`, `PhysicsShape` |
| Enum value | PascalCase | `RenderBackend::Vulkan` |
| Template param | PascalCase | `template<typename ComponentT>` |
| Concept | PascalCase + Concept suffix | `ComponentConcept`, `SystemConcept` |
| Macro | VYRO_ prefix + SCREAMING_SNAKE | `VYRO_ASSERT`, `VYRO_PROFILE_SCOPE` |
| File | snake_case | `rigid_body.cpp`, `scene_graph.hpp` |
| Namespace | snake_case | `vyro::renderer::opengl` |

---

## File Structure

Every `.hpp` file:
```cpp
#pragma once                    // Always, no include guards

#include <vyro/core/Types.hpp>  // Engine includes first
// blank line
#include <string>               // STL includes
#include <vector>
// blank line
#include <glm/glm.hpp>          // Third-party includes

namespace vyro {

class ClassName {
public:
    // 1. Constructors / destructors
    // 2. Public methods
    // 3. Public members (if any — prefer private)

private:
    // 4. Private methods
    // 5. Private members
};

} // namespace vyro
```

Every `.cpp` file:
```cpp
#include "ClassName.hpp"        // Own header first

#include <vyro/core/Logger.hpp> // Engine
#include <vector>               // STL
#include <glad/glad.h>          // Third-party

namespace vyro {

// Implementation

} // namespace vyro
```

---

## C++ Rules

### Language Version
- C++23 required. Use modern features where they improve clarity.
- Use `std::expected<T, E>` instead of exceptions in engine code.
- Use `std::string_view` for non-owning string parameters.
- Use `std::span<T>` for non-owning array views.

### Classes
```cpp
// Good: rule of five or rule of zero
class Buffer {
public:
    Buffer() = default;
    ~Buffer();
    Buffer(const Buffer&) = delete;            // non-copyable
    Buffer& operator=(const Buffer&) = delete;
    Buffer(Buffer&&) noexcept;
    Buffer& operator=(Buffer&&) noexcept;
};

// Bad: implicit copy of GPU resource
class Buffer {
    // ... no copy/move declarations
};
```

### Memory
- Never use raw `new` / `delete` in engine code. Use allocators.
- Never use `malloc` / `free`. Use `vyro::memory::` allocators.
- `std::unique_ptr` is acceptable for single-owner RAII.
- `std::shared_ptr` is forbidden in hot paths (atomic ref count overhead).
- Use `AssetHandle<T>` for assets.

### Error Handling
```cpp
// Good: std::expected for fallible operations
std::expected<Texture, Error> load_texture(std::string_view path);

// Bad: exceptions in engine code
Texture load_texture(std::string_view path); // throws on failure

// Good: VYRO_ASSERT for invariants
VYRO_ASSERT(entity != NULL_ENTITY, "Entity must be valid");

// Good: VYRO_VERIFY for always-on critical checks
VYRO_VERIFY(m_device != nullptr, "Device not initialized");
```

### Const Correctness
- Mark every method `const` that does not mutate state.
- Prefer `const T&` for in-parameters of non-trivial types.
- Prefer `T&&` for sink parameters (move semantics).
- Prefer `T` for trivially copyable types (int, float, POD).

### Lambdas
- Capture only what you need: `[entity_id]` not `[=]` or `[&]`.
- Maximum 3 lines inline; extract longer lambdas to named functions.

### Templates
- Document all template parameters with concepts.
- Do not write template implementations in `.cpp` files.
- Keep template depth < 3 levels.

---

## Comments

Write **why**, not **what**. The code says what. Comments say why.

```cpp
// Good: explains non-obvious invariant
// Bone index 0 is always the root, guaranteed by Assimp importer.
const auto& root = m_bones[0];

// Bad: restates the code
// Get the first bone
const auto& root = m_bones[0];

// Good: documents a workaround
// WORKAROUND: Nvidia driver 531.xx crashes if we bind the UBO before
// calling vkCmdBeginRenderPass. Reorder to avoid the bug.
bind_ubo_after_renderpass();

// Bad: function description (use good names instead)
// This function updates the transform
void update_transform(); // Name already says this
```

No block comment headers inside functions. No "section" dividers. No ASCII art.

---

## Formatting

Enforced by `.clang-format`. Key rules:
- 4-space indent (no tabs)
- Allman braces (opening brace on new line) for functions and classes
- K&R braces for `if`/`for`/`while`
- Max line length: 120 characters
- Always brace single-statement `if` bodies
- Pointer/reference: `T* ptr`, `T& ref` (attached to type)

---

## Includes

Order (enforced by clang-tidy):
1. Own header (`"MyClass.hpp"`)
2. Engine headers (`<vyro/...>`)
3. STL (`<string>`, `<vector>`)
4. Third-party (`<glm/glm.hpp>`)

Use forward declarations instead of includes in headers where possible.

---

## Banned Patterns

```cpp
// BANNED: global mutable state (use dependency injection)
static std::vector<Entity> s_all_entities; // NO

// BANNED: deep inheritance (> 2 levels)
class A {};
class B : public A {};
class C : public B {}; // NO — use composition

// BANNED: std::shared_ptr in hot paths
std::shared_ptr<Mesh> render_mesh; // NO — use AssetHandle<Mesh>

// BANNED: raw loops over entity data (use queries)
for (auto& e : all_entities) { // NO
    if (e.has<Transform>()) ...
}

// GOOD: ECS query
world.query<Transform, Velocity>().for_each([](auto& t, const auto& v) {
    t.position += v.linear * dt;
});
```

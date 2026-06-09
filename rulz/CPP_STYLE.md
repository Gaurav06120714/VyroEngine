# VyroEngine C++ Style Guide

**Version:** 1.0.0 | Enforced by: `.clang-format` + `.clang-tidy`

---

## Quick Reference Card

```cpp
// ─── File Header ────────────────────────────────────────────────────
#pragma once

#include "MyClass.hpp"   // own header
#include <vyro/core.hpp> // engine
#include <string>        // STL
#include <glm/glm.hpp>   // third-party

namespace vyro {         // namespace snake_case

// ─── Class ──────────────────────────────────────────────────────────
class RigidBody          // PascalCase
{
public:
    explicit RigidBody(float mass);    // explicit single-arg ctors
    ~RigidBody() = default;

    void apply_force(glm::vec3 force); // snake_case methods
    [[nodiscard]] float mass() const;  // nodiscard on getters

private:
    float       m_mass{1.0f};          // m_ prefix, init in-class
    glm::vec3   m_velocity{};
};

// ─── Struct (plain data) ─────────────────────────────────────────────
struct PhysicsDesc {
    float mass      = 1.0f;
    float restitution = 0.5f;
    bool  is_kinematic = false;
};

// ─── Enum ────────────────────────────────────────────────────────────
enum class PhysicsShape {
    Sphere,
    Box,
    Capsule,
    ConvexHull,
};

// ─── Free Function ───────────────────────────────────────────────────
[[nodiscard]] glm::vec3 compute_inertia(float mass, float radius);

} // namespace vyro
```

---

## Spacing & Braces

```cpp
// Functions: Allman (brace on new line)
void update_transform(float dt)
{
    // body
}

// Control flow: K&R (brace on same line)
if (condition) {
    do_something();
} else {
    do_other();
}

// Always brace single-statement ifs
if (condition) {   // CORRECT
    return;
}
if (condition) return; // WRONG — no braces

// Switch
switch (shape) {
    case PhysicsShape::Sphere: {
        break;
    }
    default: {
        VYRO_WARN(PHYSICS, "Unknown shape");
        break;
    }
}
```

---

## Initializer Patterns

```cpp
// Prefer in-class initialization
class Timer {
    float m_elapsed{0.0f};    // CORRECT
    float m_elapsed = 0.0f;   // also acceptable
    float m_elapsed;          // WRONG — uninitialized
};

// Prefer designated initializers for structs
PhysicsDesc desc{
    .mass = 5.0f,
    .is_kinematic = true,
};
```

---

## Modern C++23 Patterns to Use

```cpp
// std::expected for error handling
std::expected<Mesh, LoadError> load_mesh(std::string_view path);

// Deducing this (CRTP without CRTP)
struct Base {
    void method(this auto&& self) { self.impl(); }
};

// std::flat_map for small sorted maps
std::flat_map<std::string, int> settings;

// std::print / std::println (prefer VYRO_INFO macros instead)
std::println("Debug: {}", value);

// Concepts
template<typename T>
concept Numeric = std::integral<T> || std::floating_point<T>;

// Range-for with structured bindings
for (auto [id, component] : component_map) { ... }
```

---

## Patterns to Avoid

```cpp
// AVOID: C-style casts
float x = (float)integer_val;       // NO
float x = static_cast<float>(integer_val); // YES

// AVOID: naked pointers for ownership
void* raw = malloc(size);           // NO — use allocators

// AVOID: #define constants
#define MAX_ENTITIES 100000         // NO
constexpr uint32_t MAX_ENTITIES = 100'000; // YES

// AVOID: using namespace in headers
using namespace std;                // NEVER in headers

// AVOID: multiple returns from complex functions
// (acceptable for early guard returns)
std::string process(Data& d) {
    if (!d.valid()) return "";      // early guard — OK
    // ... complex processing ...
    return result;
}

// AVOID: out parameters (prefer return by value or std::expected)
void compute(int input, int* output); // NO
int compute(int input);               // YES
```

---

## Clang-Format Config Summary

Key settings (see `.clang-format` for full config):
```yaml
BasedOnStyle: Google
IndentWidth: 4
ColumnLimit: 120
BreakBeforeBraces: Allman  # for functions/classes
BraceWrapping:
  AfterControlStatement: false  # K&R for if/for/while
PointerAlignment: Left     # int* ptr, not int *ptr
```

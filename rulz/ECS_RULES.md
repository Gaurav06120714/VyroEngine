# VyroEngine ECS Rules

**Version:** 1.0.0 | **Pattern:** Archetype-based ECS

---

## Foundational Rules

### Rule ECS-1: Components are Pure Data
Components must contain ONLY data. No methods, no logic, no virtual functions.

```cpp
// CORRECT
struct TransformComponent {
    glm::vec3 position{0.0f};
    glm::quat rotation{1.0f, 0.0f, 0.0f, 0.0f};
    glm::vec3 scale{1.0f};
};

// WRONG: logic in component
struct TransformComponent {
    glm::vec3 position;
    void Translate(glm::vec3 delta) { position += delta; } // NO
    glm::mat4 GetMatrix() const { ... }                    // NO
};
```

### Rule ECS-2: Systems Contain All Logic
All behavior lives in Systems. Systems query components; components hold state.

```cpp
// CORRECT
class MovementSystem : public ISystem {
    void OnUpdate(float dt) override {
        m_world->query<TransformComponent, VelocityComponent>()
            .for_each([dt](auto& t, const auto& v) {
                t.position += v.linear * dt;
            });
    }
};
```

### Rule ECS-3: No Entity References in Components
Components must not store raw EntityIDs for relationships — use `RelationshipComponent` or indirect lookups.

```cpp
// WRONG: raw entity reference
struct WeaponComponent {
    EntityID owner; // Stale after owner destruction!
};

// CORRECT: use RelationshipComponent for hierarchy
// Use events for cross-entity communication
```

### Rule ECS-4: Tag Components are Zero-Size
Use empty structs as tags. They cost nothing in archetype tables.

```cpp
struct StaticTag {};       // Entity never moves
struct VisibleTag {};      // Entity is rendered
struct PlayerTag {};       // Mark the player
struct NetworkedTag {};    // Entity is replicated
```

### Rule ECS-5: Prefer Iteration Over Single-Entity Lookup
Batch iteration over archetypes is 100x faster than per-entity component lookup.

```cpp
// FAST: iterate all
world.query<TransformComponent>().for_each([](auto& t) { ... });

// SLOW (only when necessary): lookup by entity
auto* t = world.get_component<TransformComponent>(entity_id);
```

### Rule ECS-6: Never Modify the World During Iteration
Adding or removing components during a query loop causes archetype migration. Use a command buffer.

```cpp
// WRONG: modifies world while iterating
world.query<HealthComponent>().for_each([&](EntityID id, auto& health) {
    if (health.value <= 0)
        world.destroy_entity(id); // UNDEFINED BEHAVIOR
});

// CORRECT: defer via CommandBuffer
CommandBuffer cmd;
world.query<HealthComponent>().for_each([&](EntityID id, const auto& h) {
    if (h.value <= 0) cmd.destroy(id);
});
cmd.execute(world);
```

### Rule ECS-7: Component Size Guidelines

| Size | Category | Examples |
|------|---------|---------|
| 0 bytes | Tag | `StaticTag`, `PlayerTag` |
| ≤ 64 bytes | Small — ideal | `TransformComponent`, `VelocityComponent` |
| 64–256 bytes | Medium — acceptable | `MaterialComponent`, `AnimatorComponent` |
| > 256 bytes | Large — use handle | Store data in asset; keep only `AssetHandle<T>` |

Oversized components destroy cache performance. When in doubt, split into smaller components.

### Rule ECS-8: Archetype Stability
Avoid frequently adding/removing components. Each add/remove causes archetype migration (memory copy). If components are toggled often, consider flags inside the component instead.

```cpp
// BAD: toggling component constantly
if (visible) world.add_component<VisibleTag>(e);
else         world.remove_component<VisibleTag>(e);

// BETTER: flag inside component (if VisibleTag is toggled > 10/sec)
struct RenderComponent {
    bool visible = true;
    AssetHandle<Mesh> mesh;
};
```

### Rule ECS-9: Use Separate Components for Separate Concerns
One component = one responsibility.

```cpp
// WRONG: mixing concerns
struct GameObjectComponent {
    glm::vec3 position;
    float health;
    float speed;
    std::string name;
    bool is_enemy;
};

// CORRECT: separate concerns
struct TransformComponent { glm::vec3 position; glm::quat rotation; glm::vec3 scale; };
struct HealthComponent     { float current; float max; };
struct MovementComponent   { float speed; };
struct TagComponent        { std::string name; uint64_t mask; };
struct EnemyTag            {};
```

### Rule ECS-10: EntityID Validation
Always check `world.is_alive(entity_id)` before using a stored EntityID. Generational IDs detect stale references but you must call the check.

---

## Query Rules

### Rule Q-1: Declare Queries Once
Create query objects at system initialization, not every frame.

```cpp
class RenderSystem : public ISystem {
    Query<TransformComponent, MeshComponent> m_query; // cached

    void OnInit() override {
        m_query = m_world->query<TransformComponent, MeshComponent>()
                          .exclude<InvisibleTag>()
                          .build();
    }

    void OnUpdate(float dt) override {
        m_query.for_each([](const auto& t, const auto& m) { ... });
    }
};
```

### Rule Q-2: Use `const` for Read-Only Components
Mark components as `const` in query lambdas when you only read them. Enables future parallelism.

```cpp
world.query<TransformComponent, const VelocityComponent>()
     .for_each([](auto& t, const auto& v) { // v is read-only
         t.position += v.linear * dt;
     });
```

---

## Scene Graph Rules

### Rule SG-1: World Transform is Derived
Never set `world_transform` directly. Set `local_transform` and let the hierarchy system recompute.

### Rule SG-2: Dirty Flags are Mandatory
Every transform change must set the `dirty` flag on the entity and all descendants. The hierarchy system recomputes lazily.

### Rule SG-3: Root Entity Convention
Every scene has exactly one implicit root entity. All top-level entities are children of root.

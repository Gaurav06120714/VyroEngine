# VyroEngine Testing Rules

**Version:** 1.0.0 | **Framework:** Google Test + Google Benchmark

---

## Coverage Requirements

| Module | Minimum Coverage |
|--------|-----------------|
| `vyro_core` (logger, events, UUID) | 90% |
| `vyro_memory` (all allocators) | 95% |
| `vyro_ecs` (registry, archetypes, queries) | 90% |
| `vyro_scene` (serialization, hierarchy) | 85% |
| `vyro_physics` (collision, solver) | 80% |
| `vyro_audio` | 75% |
| `vyro_animation` | 75% |
| `vyro_renderer` (CPU-side only) | 70% |
| `vyro_scripting` | 75% |

---

## Test Types

### Unit Tests
- Test one function or class in isolation
- No real files, no real GPU, no real audio device
- Mock external dependencies
- Must run in < 100ms per test

```cpp
TEST(PoolAllocatorTest, AllocateReturnAlignedPointer) {
    PoolAllocator<Transform, 64> pool;
    auto* t = pool.allocate();
    EXPECT_NE(t, nullptr);
    EXPECT_EQ(reinterpret_cast<uintptr_t>(t) % 16, 0); // 16-byte aligned
    pool.deallocate(t);
}
```

### Integration Tests
- Test multiple systems working together
- May use real file system (temp directories)
- May use headless rendering (software rasterizer or offscreen FBO)
- Must run in < 5 seconds per test

```cpp
TEST(SceneSerializationTest, RoundTripPreservesAllComponents) {
    Scene scene;
    auto entity = scene.create_entity();
    scene.add_component<TransformComponent>(entity, {.position = {1, 2, 3}});
    scene.add_component<TagComponent>(entity, {.name = "TestEntity"});

    auto json = SceneSerializer::serialize(scene);
    Scene loaded;
    SceneSerializer::deserialize(loaded, json);

    auto* t = loaded.get_component<TransformComponent>(entity);
    ASSERT_NE(t, nullptr);
    EXPECT_FLOAT_EQ(t->position.x, 1.0f);
}
```

### Benchmark Tests
- Measure performance of critical paths
- Run with `--benchmark_min_time=5`
- Results stored and compared to baselines (CI regression check)

```cpp
BENCHMARK(BM_ECSQuery_100K_Entities) {
    World world;
    for (int i = 0; i < 100'000; ++i) {
        auto e = world.create_entity();
        world.add_component<TransformComponent>(e, {});
        world.add_component<VelocityComponent>(e, {});
    }

    for (auto _ : state) {
        world.query<TransformComponent, VelocityComponent>()
             .for_each([](auto& t, const auto& v) {
                 t.position += v.linear * 0.016f;
             });
    }
}
BENCHMARK(BM_ECSQuery_100K_Entities)->Unit(benchmark::kMillisecond);
```

---

## Test Rules

### Rule T-1: Test Names Describe Behavior
`ClassName_MethodName_ExpectedBehavior` or `Scenario_WhenCondition_ThenExpected`

```cpp
// Good
TEST(PoolAllocator, Deallocate_ThenAllocate_ReusesMemory)
TEST(EntityRegistry, DestroyEntity_ThenCreate_GenerationIncremented)

// Bad
TEST(PoolAllocator, Test1)
TEST(Registry, Works)
```

### Rule T-2: Arrange-Act-Assert
Every test follows AAA structure:

```cpp
TEST(AudioMixer, SetVolume_AboveMax_ClampsTo1) {
    // Arrange
    AudioMixer mixer;

    // Act
    mixer.set_volume(2.5f); // above max

    // Assert
    EXPECT_FLOAT_EQ(mixer.get_volume(), 1.0f);
}
```

### Rule T-3: One Assertion Concept per Test
Each test should verify one behavior. Use multiple `EXPECT_*` only when they all verify the same logical thing.

### Rule T-4: Tests Must Be Deterministic
Tests that depend on timing, random numbers, or thread scheduling are forbidden unless they are explicitly testing those systems with controlled seeds/mocks.

### Rule T-5: No Test-Only Code in Production
No `#ifdef TESTING` guards in production code. Tests use dependency injection and interfaces, not compiler flags.

### Rule T-6: Sanitizers in CI
All tests run with AddressSanitizer (ASAN) and UndefinedBehaviorSanitizer (UBSAN) in CI. Any sanitizer warning is treated as a test failure.

### Rule T-7: Physics Tests Are Deterministic
Physics tests use fixed seeds, fixed timesteps, and verify final state against known-good values. Not "approximately" stable — exactly stable.

---

## CI Requirements

Every pull request must:
1. Build on Windows + Linux (GitHub Actions matrix)
2. All unit tests pass
3. No ASAN/UBSAN warnings
4. No performance benchmark regression > 10%
5. Code coverage does not drop below minimum thresholds
6. Clang-Format and Clang-Tidy clean

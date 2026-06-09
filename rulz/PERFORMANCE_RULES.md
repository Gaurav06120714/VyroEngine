# VyroEngine Performance Rules

**Version:** 1.0.0 | **Targets:** 60fps minimum, 100K entities, <100MB base RAM

---

## Measurement Rules

### Rule PERF-1: Measure Before Optimizing
Never optimize without a profiler. Every optimization must be motivated by a Tracy or benchmark measurement.

### Rule PERF-2: Establish Baselines
For every major system, a benchmark exists that establishes the baseline. Any change that regresses the baseline by > 5% must be justified or reverted.

### Rule PERF-3: Profile on Minimum Hardware
Performance targets apply to minimum hardware (GTX 1060, 8-core CPU @ 3GHz, 16GB RAM). Profiling only on development hardware (RTX 4090) hides real issues.

---

## CPU Rules

### Rule CPU-1: Cache-Friendly Data Layout
Arrays of structures (AoS) → Structures of arrays (SoA) for hot iteration paths.

```cpp
// AoS — bad for iteration (stride = sizeof(Particle))
struct Particle { glm::vec3 pos; glm::vec3 vel; float life; float size; };
std::vector<Particle> particles;

// SoA — good for iteration (stride = sizeof(float) × 3)
struct ParticleSystem {
    std::vector<glm::vec3> positions;  // iterate only positions → cache hot
    std::vector<glm::vec3> velocities;
    std::vector<float>     lifetimes;
    std::vector<float>     sizes;
};
```

### Rule CPU-2: Branch Prediction Friendly
Sort data before processing to enable branch prediction.

```cpp
// Sort entities by component signature before iterating
// → CPU branch predictor sees same path repeatedly
```

### Rule CPU-3: SIMD for Math Hot Paths
Critical math operations (transform hierarchy, skinning, physics broadphase) use SIMD intrinsics or compiler-friendly patterns that auto-vectorize.

### Rule CPU-4: Job System for Parallelism
Independent work packages must be submitted to the job system, not executed sequentially on the main thread.

```cpp
// WRONG: sequential
for (auto& chunk : chunks) { update_chunk(chunk); }

// CORRECT: parallel
JobBatch batch;
for (auto& chunk : chunks) batch.push([&]{ update_chunk(chunk); });
job_system.submit(batch);
job_system.wait(batch);
```

### Rule CPU-5: Fixed Timestep for Simulation
Physics and deterministic logic use a fixed 60Hz timestep. Variable-rate logic (input, animation, rendering) uses the real frame delta.

---

## GPU Rules

### Rule GPU-1: Minimize State Changes
Render state changes (shader bind, texture bind, blend mode) are expensive. Sort draw calls to minimize them.

Order of sort priority:
1. Render layer / depth bucket
2. Shader / pipeline
3. Material
4. Vertex buffer

### Rule GPU-2: Batch Geometry
Draw calls are expensive. Target < 3,000 draw calls per frame on desktop.
- Use GPU instancing for repeated meshes
- Use sprite batching for 2D

### Rule GPU-3: Avoid GPU Readback
`glReadPixels` / `vkMapMemory` on GPU-only resources stall the pipeline. Use async readback with fences if readback is necessary.

### Rule GPU-4: Mip Maps Always
All textures used in 3D rendering must have mipmaps. Without mipmaps, texture cache efficiency collapses at distance.

### Rule GPU-5: Occlusion Culling
Large scenes use GPU occlusion queries or hierarchical Z-buffer culling to avoid submitting occluded geometry.

---

## Memory Performance Rules

### Rule MP-1: Hot/Cold Data Separation
Frequently accessed data (hot) must not share cache lines with rarely accessed data (cold).

```cpp
// WRONG: hot and cold in same struct
struct Entity {
    // HOT — accessed every frame
    glm::vec3 position;
    glm::vec3 velocity;
    // COLD — accessed rarely
    std::string debug_name;      // cold
    std::vector<std::string> tags; // cold
};

// CORRECT: split into hot/cold
struct EntityHot  { glm::vec3 position; glm::vec3 velocity; };
struct EntityCold { std::string debug_name; std::vector<std::string> tags; };
```

### Rule MP-2: Pool Allocators for Frequent Objects
Any object type created/destroyed more than 100 times per second must use a pool allocator.

---

## Profiling Integration

Every subsystem uses Tracy scopes:

```cpp
#include <tracy/Tracy.hpp>

void PhysicsWorld::step(float dt) {
    ZoneScoped;                          // Function scope
    ZoneNamedN(broadphase, "Broadphase", true);
    run_broadphase();

    ZoneNamedN(narrowphase, "Narrowphase", true);
    run_narrowphase();
}
```

GPU work uses Tracy GPU zones:
```cpp
TracyVkZone(m_tracy_ctx, cmd_buffer, "Shadow Pass");
```

---

## Frame Budget (60fps = 16.67ms)

| System | Budget |
|--------|--------|
| ECS Update | 1.0ms |
| Physics (fixed) | 2.0ms |
| Animation | 0.5ms |
| Scripts | 0.5ms |
| Render CPU (build commands) | 1.0ms |
| Render GPU | 8.0ms |
| Audio | 0.5ms |
| Editor (in editor mode) | 2.0ms |
| Misc / overhead | 1.17ms |
| **Total** | **16.67ms** |

Any system exceeding its budget triggers a Tracy alert in debug builds.

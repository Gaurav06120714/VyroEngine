# VyroEngine 6.0 Roadmap

**Theme:** v1 built the engine, v2 added production backends, v3 made it a
game-maker, v4 made it *feel* good, v5 made it *look* like a world. v6 is **the
production-grade era** — turn the demos into real systems: true GPU-driven
rendering, a proper depth/shadow pipeline, shared multiplayer worlds, authored
levels, and a measured performance budget.

v6 also pays down the two honest debts left by v5:
- **V5.5** batches the horde on the **CPU** (one merged buffer per frame) rather
  than using GPU hardware instancing.
- **V5.2** stores shadow depth in a **color** render target because the RHI has
  no sampleable **depth-texture** attachment yet.

V6.1 and V6.2 close those gaps first; the rest build on top.

## Phases

| Phase | Goal | Tag |
|-------|------|-----|
| ✅ V6.1 — GPU Hardware Instancing | done — `OpenGLDevice::draw_instanced` (`glDrawElementsInstanced`, mat4 instance attribute at locations 4–7, divisor 1) + `render/Instancing.hpp` layout helper. The horde draws in one instanced call from a per-instance transform buffer, replacing the V5.5 CPU batch. | v5.1.0 |
| V6.2 — Depth-Texture RTT & Soft Shadows | Add a sampleable **depth-texture** render target to the RHI (depth attachment, `GL_DEPTH_COMPONENT`), move the shadow pass onto it, and improve shadow quality (slope-scaled bias, wider PCF, optional cascades using the existing `shadows::cascade_splits`). Replaces the V5.2 color-encoded depth. | v5.2.0 |
| V6.3 — Networked Co-op Gameplay | Extend `CoopLink`/`Replication` from avatar-only sync to **shared gameplay**: an authoritative host simulates the horde, score, and waves and replicates them; the joiner renders and contributes hits. Two players fight the same wave. Builds on V4.4. | v5.3.0 |
| V6.4 — Level & Obstacle Pipeline | Author arenas (ground extents, spawn points, obstacles/cover) and load them through the existing `scene/SceneSerializer`. Obstacles occlude (shadows + render), block movement (physics), and are avoided by the horde (obstacle-avoidance steering added to `ai/Steering`). | v5.4.0 |
| V6.5 — Profiling & Optimization | Wire `core/Profiler` into the frame loop with an on-screen frame-time / draw-call graph, set a per-frame budget, and optimize the hot paths surfaced (CPU skinning, batching/instancing, culling). Document before/after numbers. | v5.5.0 |
| v6.0.0 release | All of the above stabilized + packaged (`cpack` tarball + GitHub release) | v6.0.0 |

## Per-phase procedure (unchanged — see AGENT_HANDOFF §4)
1. Implement engine piece(s) as header + .cpp under `engine/`.
2. Add a behavior-named headless `tests/test_<thing>.cpp`.
3. Wire it into the game (`games/vyrostrike/main.cpp`) for a visible payoff.
4. `cmake --build build` → **zero warnings** → `ctest` → **all green**.
5. Commit **per file** with Conventional Commits, **push after each**.
6. Capture a screenshot of the running game to verify.
7. Update README status table; commit; `git tag -a vX.Y.Z`; push tag.

## Rules
Same as always (`rulz/`): Conventional Commits, one logical change per commit,
**no AI co-authors ever**, zero-warning builds under `-Wall -Wextra -Wpedantic
-Wshadow -Wconversion`, deterministic headless tests, optional backends isolated
from the headless core, a screenshot per phase, a tag per phase, and prior
public APIs kept source-compatible.

## Tag scheme
Following v4/v5: V6 phase tags continue the patch series as `v5.1.0` … `v5.5.0`,
and the umbrella release is **`v6.0.0`**.

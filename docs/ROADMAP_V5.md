# VyroEngine 5.0 Roadmap

**Theme:** v1 built the engine, v2 gave it production backends, v3 made it a
game-maker, v4 made it *feel* good. v5 is **the rendering & worlds era** — the
leap from "a good-feeling arena" to "a real-looking world": offscreen HDR
rendering, shadows, bigger streamed scenes, smarter enemies, and the GPU-driven
batching to draw it all fast.

This release also pays down the one honest debt from v4: the RHI has no
offscreen render targets yet, so V4.5's post-FX is a screen-space overlay
rather than a true HDR pass. V5.1 fixes that first, unlocking everything after.

## Phases

| Phase | Goal | Tag |
|-------|------|-----|
| V5.1 — Render Targets & HDR Bloom | Add framebuffer/render-target support to the RHI (OpenGL FBO + Vulkan attachment). Render the scene to an HDR offscreen target, then a real bloom + ACES tonemap post pass — the math already lives in `render/PostProcess.hpp`/`PBR.hpp`, just needs an RTT to run on. | v4.1.0 |
| V5.2 — Real-Time Shadows | A directional shadow-map pass over the arena using the existing `shadows::cascade_splits`/`select_cascade` math. The sun casts the soldier and the horde onto the ground. | v4.2.0 |
| V5.3 — Larger Worlds & Culling | Chunked/streamed scene loading beyond a single flat arena, plus frustum culling so only visible chunks and entities are drawn. Foundations for non-arena levels. | v4.3.0 |
| V5.4 — Gameplay AI | Smarter zombies: steering + flocking so the horde spreads and surrounds, simple obstacle avoidance, and a behavior/state machine driving idle/seek/attack — built on `animation/StateMachine`. | v4.4.0 |
| V5.5 — GPU-Driven Rendering | Instanced rendering of the horde (one draw call for N zombies) and draw-call batching, so larger worlds and bigger hordes stay fast. Profiled against the existing `core/Profiler`. | v4.5.0 |
| v5.0.0 release | All of the above stabilized + packaged (`cpack` tarball + GitHub release) | v5.0.0 |

## Rules
Same as always (see `rulz/`): per-file Conventional Commits, **no AI
co-authors**, zero-warning builds under `-Wall -Wextra -Wpedantic -Wshadow
-Wconversion`, behavior-named headless tests green at every step, the optional
backends stay isolated from the headless core, a screenshot per phase, and a
tag per phase. Prior public APIs stay source-compatible.

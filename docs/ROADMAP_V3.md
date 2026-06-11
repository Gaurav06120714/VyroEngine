# VyroEngine 3.0 Roadmap

**Theme:** v1 built the engine, v2 gave it production backends, v3 makes it a
*game-maker*: characters that move, sound you can hear, UI you can read, and
scenes you can author and save.

## Phases

| Phase | Goal | Tag |
|-------|------|-----|
| V3.1 — Skeletal Animation | GLB skins + animation clips: joint hierarchies, keyframe sampling (translation/rotation/scale), CPU skinning of vertices. Zombies walk. | v2.1.0 |
| V3.2 — Animated Gameplay | VyroStrike Outbreak upgrade: walking zombies, death animations, facing, waves and health | v2.2.0 |
| V3.3 — Real Audio Output | miniaudio device behind the AudioEngine model: gunshots, groans, music | v2.3.0 |
| V3.4 — Text & HUD | On-screen bitmap font rendering: score, health, menus in the viewport | v2.4.0 |
| V3.5 — Scene Authoring | Scene save/load (serialization to disk) + editor create/delete/save workflow | v2.5.0 |
| v3.0.0 release | All of the above stabilized | v3.0.0 |

## Rules
Same as always: per-file conventional commits, no AI co-authors, tests green
at every step, tag per phase, v1/v2 public APIs stay source-compatible.

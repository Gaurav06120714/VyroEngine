# VyroEngine 4.0 Roadmap

**Theme:** v1 built the engine, v2 gave it production backends, v3 made it a
game-maker. v4 is **the juice & performance era** — the polish that separates
a tech demo from a game that *feels* good: particles, real audio, blended
animation, multiplayer, and a camera that sells the action.

## Phases

| Phase | Goal | Tag |
|-------|------|-----|
| V4.1 — Particle System | Pooled CPU particle emitters (spawn/age/fade/gravity) + GPU quad rendering. Muzzle flashes when you shoot, blood bursts when a zombie drops. | v3.1.0 |
| V4.2 — Audio Files & Music | WAV/OGG decoding via miniaudio: load real sound files and loop background music alongside the synthesized SFX. | v3.2.0 |
| V4.3 — Animation Blending | Blend-tree / state-machine playback on the live characters: idle ↔ walk ↔ attack transitions driven by gameplay state. | v3.3.0 |
| V4.4 — Co-op Multiplayer | Two soldiers in Outbreak over the existing UDP transport: a second networked player, replicated. | v3.4.0 |
| V4.5 — Camera & Post-FX | Follow camera with smoothing, screen shake on hits, and a tonemap/bloom post pass wired into the renderer. | v3.5.0 |
| v4.0.0 release | All of the above stabilized | v4.0.0 |

## Rules
Same as always: per-file conventional commits, no AI co-authors, tests green
at every step, tag per phase, all prior public APIs stay source-compatible.

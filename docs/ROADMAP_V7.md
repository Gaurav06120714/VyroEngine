# VyroEngine 7.0 Roadmap

**Theme:** v5 made it look like a world, v6 made it production-grade. v7 is **the
gameplay-depth era** — turn one shambling enemy and one gun into a game with
variety and stakes: a horde that doesn't march in lockstep, multiple enemy and
weapon types, spatial audio, and real objectives.

It opens with the natural follow-on to V6.1 (the whole horde shares one pose),
then builds gameplay systems that are mostly headless-testable engine logic.

## Phases

| Phase | Goal | Tag |
|-------|------|-----|
| V7.1 — Varied Horde Animation | Each zombie animates at its own phase instead of in lockstep, while staying GPU-instanced: bucket instances across K skinned cycle poses (one instanced draw per bucket) and assign each zombie a stable phase from its id. | v6.1.0 |
| V7.2 — Enemy Variety | Multiple enemy archetypes (walker / runner / brute) with per-type speed, health, scale and score, driven by data components — not one hard-coded zombie. | v6.2.0 |
| V7.3 — Weapons & Damage | A weapon model: fire rate, spread, pellet count, damage and reload; pistol / rifle / shotgun loadouts the player can switch. Enemies take damage instead of dying in one hit. | v6.3.0 |
| V7.4 — Spatial Audio Mix | Positional SFX: per-source pan and distance attenuation through a small mixer, so shots and groans come from where they happen. Builds on the V4.2 audio backend. | v6.4.0 |
| V7.5 — Objectives & Game Flow | Timed/target waves with win + lose conditions, an intermission/results screen, and a clear game-state machine — not just "survive forever". | v6.5.0 |
| v7.0.0 release | All of the above stabilized + packaged (`cpack` tarball + GitHub release) | v7.0.0 |

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
from the headless core, a screenshot per phase, a tag per phase, source-compatible
public APIs.

## Tag scheme
Following v4/v5/v6: V7 phase tags are `v6.1.0` … `v6.5.0`; the umbrella release
is **`v7.0.0`**.

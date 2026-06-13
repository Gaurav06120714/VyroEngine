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
| ✅ V7.1 — Varied Horde Animation | done — `animation/CycleVariation.hpp` (`phase_bucket` + `bucket_time`); the horde is split across K phase buckets, each skinned at its own cycle offset and drawn as a separate instanced batch. | v6.1.0 |
| ✅ V7.2 — Enemy Variety | done — `core/Random.hpp` `weighted_index` + walker/runner/brute archetypes with per-type speed/health/scale/score; brutes take multiple hits, runners are faster. | v6.2.0 |
| ✅ V7.3 — Weapons & Damage | done — `game/Weapon.hpp` (fire rate/spread/pellets/damage/reload); pistol/rifle/shotgun on keys 1/2/3, bullets carry damage, HUD shows weapon/ammo. | v6.3.0 |
| ✅ V7.4 — Spatial Audio Mix | done — `audio/Spatial.hpp` (distance attenuation + stereo pan); the game distance-attenuates death groans from the soldier. (Pan is computed/tested for a future stereo device; the current device is mono gain.) | v6.4.0 |
| ✅ V7.5 — Objectives & Game Flow | done — `game/GameFlow.hpp` (wave kill targets, intermission, victory/defeat); HUD shows wave x/total + kill objective, VICTORY/DEFEAT screens. | v6.5.0 |
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

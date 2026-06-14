# VyroEngine 8.0 Roadmap

**Theme:** v7 gave the game depth (enemies, weapons, objectives). v8 is **the
content & persistence era** — the systems that make a run *stick*: saved
progress and settings, pickups, run stats, difficulty modes, and data-driven
wave configs. All headless-testable engine logic.

## Phases

| Phase | Goal | Tag |
|-------|------|-----|
| ✅ V8.1 — Save & Settings | done — `game/SaveData.hpp` (key=value serialize/parse + file IO); the game loads a profile, shows the high score, and saves best score/wave per run. | v7.1.0 |
| ✅ V8.2 — Pickups & Power-ups | done — `game/Pickup.hpp`; kills drop health/ammo/score pickups (weighted), collected on touch (heal/refill/bonus). | v7.2.0 |
| ✅ V8.3 — Run Stats & Summary | done — `game/RunStats.hpp`; tracks shots/hits/kills/time + accuracy, shown on the victory/defeat screen. | v7.3.0 |
| ✅ V8.4 — Difficulty Modes | done — `game/Difficulty.hpp`; easy/normal/hard scale enemy speed/health/spawn + player HP, TAB-cycled and saved. | v7.4.0 
| V8.5 — Data-Driven Waves | Author wave plans (counts, archetype weights, intermissions) as data and load them, instead of the hard-coded ramp. | v7.5.0 |
| v8.0.0 release | All of the above stabilized + packaged (`cpack` tarball + GitHub release) | v8.0.0 |

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
**no AI co-authors ever**, zero-warning builds, deterministic headless tests,
optional backends isolated from the headless core, a screenshot per phase, a tag
per phase, source-compatible public APIs.

## Tag scheme
Following v4–v7: V8 phase tags are `v7.1.0` … `v7.5.0`; the umbrella release is
**`v8.0.0`**.

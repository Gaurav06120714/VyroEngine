# VyroEngine 9.0 Roadmap

**Theme:** v8 made a run *stick*; v9 makes it *grow*. **The progression & bosses
era** — earn currency, spend it on upgrades between waves, fight bosses, chain
kills for a combo multiplier, and unlock medals. All headless-testable systems.

## Phases

| Phase | Goal | Tag |
|-------|------|-----|
| ✅ V9.1 — Currency & Economy | done — `game/Economy.hpp`; kills award credits shown in the HUD (foundation for the shop). | v8.1.0 |
| ✅ V9.2 — Player Upgrades | done — `game/Upgrades.hpp`; between-wave shop (Z/X/C/V) buys tiered damage/fire-rate/health/speed with credits. | v8.2.0 |
| ✅ V9.3 — Boss Enemies | done — `game/BossSchedule.hpp`; a large, tough, high-reward boss spawns every 3rd wave with a HUD warning. | v8.3.0 |
| ✅ V9.4 — Combo Multiplier | done — `game/Combo.hpp`; rapid kills build a capped score/credit multiplier that decays, shown in the HUD. | v8.4.0 |
| ✅ V9.5 — Medals & Milestones | done — `game/Medals.hpp`; run milestones award persisted medals shown on the results screen. | v8.5.0 |
| v9.0.0 release | All of the above stabilized + packaged (`cpack` tarball + GitHub release) | v9.0.0 |

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
Following v4–v8: V9 phase tags are `v8.1.0` … `v8.5.0`; the umbrella release is
**`v9.0.0`**.

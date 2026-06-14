# VyroEngine 10.0 Roadmap

**Theme:** v9 made a run grow; v10 makes each moment varied. **The variety &
polish era** — environmental hazards, boss phases, timed power-ups, unlockable
weapons, and a real title screen. All headless-testable systems.

## Phases

| Phase | Goal | Tag |
|-------|------|-----|
| ✅ V10.1 — Environmental Hazards | done — `game/Hazard.hpp`; periodic fire patches burn enemies in them and fade out. | v9.1.0 |
| ✅ V10.2 — Boss Phases | done — `game/BossPhase.hpp`; bosses enrage/frenzy as health drops, speeding up, with a phase HUD. | v9.2.0 |
| ✅ V10.3 — Weapon Unlocks | done — `game/WeaponUnlock.hpp`; rifle/shotgun unlock by wave, switching gated, HUD shows count. | v9.3.0 |
| ✅ V10.4 — Timed Power-ups | done — `game/Buffs.hpp`; boss/rare buff pickups grant timed rapid-fire & double-damage with HUD countdowns. | v9.4.0 |
| V10.5 — Title & Menu | A title/menu state (start, difficulty, view medals) before the run; a `MenuState` machine drives it. | v9.5.0 |
| v10.0.0 release | All of the above stabilized + packaged (`cpack` tarball + GitHub release) | v10.0.0 |

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
Following v4–v9: V10 phase tags are `v9.1.0` … `v9.5.0`; the umbrella release is
**`v10.0.0`**.

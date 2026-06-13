# VyroEngine — Agent Handoff

A complete context dump so a fresh chat/agent can continue building VyroEngine
without re-deriving anything. Read this first.

---

## 1. What this project is

**VyroEngine** is a cross-platform 3D game engine written from scratch in
**C++23**, plus **VyroStrike: Outbreak**, a zombie-shooter game built on it.

- **Repo:** https://github.com/Gaurav06120714/VyroEngine
- **Local path:** `/Users/gaurav/Desktop/MyProjects/VyroEcosystem/VyroEngine`
- **Author / git identity:** Gaurav (commits are **human-only — never any AI co-author**)
- **Build:** CMake + Ninja, single `build/` dir
- **Platform built/tested on:** macOS (Apple M2 Pro)

### Build & run
```bash
cd /Users/gaurav/Desktop/MyProjects/VyroEcosystem/VyroEngine
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
ctest --test-dir build              # 41 test suites, all green
```
Apps in `build/bin/`:
- `VyroStrike` — the game (A/D move, Space shoot, R restart, Esc quit; `VYRO_AUTOFIRE=1` env = auto-shoot smoke test)
- `VyroEditor` — ImGui editor (hierarchy, inspector, assets, stats; +Entity/Delete/Save/Load scene)
- `vyro_model` — textured 3D model viewer (OBJ)
- `vyro_vulkan` — the model rendered through real Vulkan
- `vyro_window` / `vyro_bounce` — physics demos

---

## 2. Architecture (how it's laid out)

```
engine/include/vyro/   public headers      engine/src/   implementations
  core/      Types, Log, EventBus, Engine, Version, Profiler
  memory/    Arena/Stack/Pool/FreeList allocators
  platform/  Input, KeyCode, Window (GLFW)
  ecs/       Entity, EntityManager, ComponentStorage (sparse set), Registry, System(s), 
  scene/     Scene, SceneManager, SceneSerializer
  math/      Vec, Mat4
  render/    RHI, NullDevice, OpenGLDevice, VulkanDevice, Camera, Renderer2D,
             Shader, Texture, FrameGraph, PBR, PostProcess, ParticleSystem, TextGeometry
  physics/   Collision, RigidBody, PhysicsWorld, Constraint, PhysicsDebug
  audio/     AudioClip, AudioEngine (model), SoundSynth, AudioDevice, AudioFile
  animation/ AnimationClip, Blend, StateMachine
  assets/    Mesh, ObjLoader, GlbLoader, SkinnedModel, Image, AssetManager/Handle
  scripting/ ScriptEngine (VyroScript), ScriptHost, LuaScriptEngine
  net/       Transport (loopback), UdpTransport, Replication
  editor/    EditorContext, Inspector, Gizmo, AssetBrowser

editor/      VyroEditor app        games/vyrostrike/   the game
samples/     demos                 tests/   one test_*.cpp per subsystem
docs/        roadmaps + this file  rulz/    engineering standards (READ THESE)
assets/      models/ (GLB chars+weapons), shaders/, scenes/
engine/third_party/   stb_image, nlohmann/json, miniaudio (all vendored, single-header)
```

### Key design rule: optional backends, headless core
The core `vyro_engine` lib is **headless and dependency-free**. GPU/OS/3rd-party
backends are **separate optional CMake targets**, and their sources live in
`engine/src/{gl,vk,lua,ma}/` (excluded from the core lib via a CMake regex filter):
- `vyro_gl`   — OpenGL + GLFW window + Dear ImGui (`VYRO_ENABLE_GL`)
- `vyro_vk`   — Vulkan via MoltenVK (`VYRO_ENABLE_VULKAN`)
- `vyro_lua`  — Lua 5.x scripting backend (`VYRO_ENABLE_LUA`)
- `vyro_audio`— miniaudio output device + file decode (`VYRO_ENABLE_AUDIO`)

The **RHI** (`render/RHI.hpp`) is the GPU abstraction: `IRenderDevice` has Null,
OpenGL, and Vulkan implementations. Same pattern for audio (model vs device),
scripting (VyroScript vs Lua), and net (loopback vs UDP).

### System dependencies (installed via Homebrew)
glfw, lua, vulkan-headers, vulkan-loader, molten-vk, glslang, cmake, ninja.
MoltenVK ICD lives at `/opt/homebrew/opt/molten-vk/etc/vulkan/icd.d/MoltenVK_icd.json`.

---

## 3. Version history (what's been built)

Each major version is released on GitHub with a packaged tarball.

- **v1.0.0** — 12-phase engine from scratch: ECS, rendering (Null/GL), physics,
  audio model, animation math, scripting (VyroScript), editor model, networking
  (loopback), profiler, packaging.
- **v2.0.0** — production backends: ImGui editor, UDP sockets, real Lua, **real
  Vulkan (MoltenVK)**, GPU textures, GLB/glTF static model loader.
- **v3.0.0** — game-maker era: **skeletal animation + CPU skinning**, animated
  gameplay (homing/dying zombies, waves, health), **real audio output**
  (miniaudio + procedural SFX), **bitmap-font HUD**, **scene serialization**.
- **v4.x (in progress)** — juice & performance:
  - **v3.1.0** ✅ V4.1 Particle System (muzzle flashes, blood bursts)
  - **v3.2.0** ✅ V4.2 Audio Files & Music (WAV/MP3/FLAC decode + looping music)

> Tag scheme note: v4 phase tags are numbered `v3.1.0`, `v3.2.0`… (continuing the
> patch series); the umbrella release will be `v4.0.0`. See README for the table.

---

## 4. WHAT TO DO NEXT  (V4 roadmap — docs/ROADMAP_V4.md)

| Phase | Goal | Tag |
|-------|------|-----|
| ✅ V4.1 Particle System | done | v3.1.0 |
| ✅ V4.2 Audio Files & Music | done | v3.2.0 |
| **▶ V4.3 Animation Blending** | **DO THIS NEXT** — cross-fade idle↔walk↔attack on the zombies. The zombie GLB already loads clips `Zombie|ZombieIdle/Walk/Run/Bite/Crawl` (see logs). Add a blend between two `SkinnedModel` poses (sample clip A and clip B, lerp the joint matrices by a weight) and drive it from gameplay: walk while approaching, **bite when adjacent** to the soldier. `animation/Blend.hpp` already blends *poses*; extend `SkinnedModel` with a `pose_blend(clipA, tA, clipB, tB, weight, out)` and use it in `games/vyrostrike/main.cpp`. | v3.3.0 |
| V4.4 Co-op Multiplayer | a 2nd networked soldier in Outbreak over the existing `UdpTransport` + `Replication` (NetServer/NetClient already exist and are tested). | v3.4.0 |
| V4.5 Camera & Post-FX | follow camera with smoothing, screen shake on hits, a tonemap/bloom post pass (math already in `render/PostProcess.hpp`/`PBR.hpp`). | v3.5.0 |
| v4.0.0 | stabilize + GitHub release with `cpack` tarball | v4.0.0 |

**Per-phase procedure (the loop the agent follows every time):**
1. Implement engine piece(s) as header + .cpp under `engine/`.
2. Add a `tests/test_<thing>.cpp` (behavior-named checks, headless).
3. Wire it into the game (`games/vyrostrike/main.cpp`) for a visible payoff.
4. `cmake --build build` → **zero warnings** → `ctest` → **all green**.
5. Commit **per file** with Conventional Commits, **push after each**.
6. Capture a screenshot of the running game to verify (see §6).
7. Update README status table; commit; `git tag -a vX.Y.Z`; push tag.

---

## 5. THE RULZ (engineering standards the agent MUST follow)

Full text in `rulz/`. The non-negotiables:

- **Commits:** Conventional Commits (`feat(scope): …`, `fix:`, `test:`, `docs:`,
  `build:`, `refactor:`). **One logical change per commit; push after each file.**
- **NO AI CO-AUTHOR, EVER.** No `Co-Authored-By: Claude`, no "Generated with"
  footer. Commits are authored by the human only. (See `rulz/COMMIT_AUTHORSHIP.md`
  — this was an explicit, repeated user requirement.)
- **C++ style** (`rulz/CPP_STYLE.md`, `CODING_STANDARDS.md`):
  - `snake_case` methods/functions/locals, `PascalCase` types, `m_` members,
    `k`-prefixed constants, `VYRO_` macros.
  - **Allman braces for function bodies**, K&R for control flow.
  - C++23. `std::expected<T,E>` for fallible ops (not exceptions). `std::string_view`,
    `std::span` for non-owning params. `[[nodiscard]]` on getters.
  - Header files are `PascalCase.hpp`; build with `-Wall -Wextra -Wpedantic
    -Wshadow -Wconversion` and **fix every warning before committing**.
- **Memory** (`rulz/MEMORY_RULES.md`): no raw `new`/`delete` in client code
  (allocators or `unique_ptr` only); prefer contiguous storage; no per-frame heap
  in hot paths.
- **Testing** (`rulz/TESTING_RULES.md`): behavior-named tests, AAA structure,
  deterministic. (We use a tiny header `tests/test_harness.hpp` — `vyro::test::Suite`
  — not GoogleTest, to stay offline.)
- **Git workflow** (`rulz/GIT_RULES.md`): tag `vMAJOR.MINOR.PATCH` per milestone;
  never commit failing tests; `.gitignore` build outputs, `*.spv`, `imgui.ini`.

The user also prefers: keep momentum (build → test → screenshot → commit → tag),
be honest about scope/limitations, and **make it visible** (always wire engine
work into the game and screenshot the result).

---

## 6. Useful operational notes

- **Screenshotting a running app window** (macOS, the agent used this constantly):
  ```bash
  # find the window id (compile this Swift helper once to /tmp/winid.swift):
  swift /tmp/winid.swift | grep -i vyro          # prints "<id>\t<owner>\t<title>"
  screencapture -o -l <id> /tmp/shot.png         # capture just that window
  ```
  winid.swift uses `CGWindowListCopyWindowInfo`. Game windows are owned by the
  binary name (e.g. `VyroStrike`); pick the row whose title matches.
- **Browser/Chrome** is available (Claude-in-Chrome MCP) and was used to download
  free models from poly.pizza. Models live in `assets/models/{characters,weapons}/`
  (GLB). Downloads land in `~/Downloads`.
- **Retina gotcha:** always use `Window::framebuffer_width/height()` (not logical
  `width/height()`) for `glViewport` and aspect ratio, or rendering lands in the
  lower-left quarter.
- **Vulkan clip space:** flip `proj.at(1,1) *= -1` (Y points down vs OpenGL).
- The zombie GLB has **no death clip** — death is a procedural fall-and-sink in
  the game. It DOES have Idle/Walk/Run/Bite/Crawl clips for V4.3 blending.

---

## 7. Current state at handoff

- Branch `main`, fully pushed, **working tree clean** (after `.gitignore` update).
- Latest tag: **v3.2.0** (V4.2 complete).
- 41 test suites, all green in Release.
- **Next action: implement V4.3 Animation Blending** (see §4).

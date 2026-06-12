# VyroEngine

> A high-performance, cross-platform game engine built from scratch — engineered for the next generation of interactive experiences.

---

## Vision

VyroEngine is an open-architecture, data-driven game engine designed to give developers full control over every layer of the stack — from memory allocators to rendering pipelines. Built with a philosophy of **transparency over magic**, VyroEngine exposes its internals, avoids hidden systems, and prioritizes predictable performance.

Think: the openness of Godot, the performance ceiling of Unreal, and the accessibility of Unity — built ground-up with modern C++23 and Vulkan-first rendering.

---

## Status

> **🎉 v1.0.0 is complete and released** — all 12 development phases (foundation,
> ECS, rendering, physics, audio, animation, scripting, editor, networking,
> advanced graphics, optimization, production) shipped, tested, and packaged.
> See the [v1.0.0 release](https://github.com/Gaurav06120714/VyroEngine/releases/tag/v1.0.0).

**✅ VyroEngine 2.0 is complete and released** — visual editor (ImGui), UDP
networking, Lua scripting, a real Vulkan backend (MoltenVK), texturing, and a
GLB/glTF model loader. See the
[v2.0.0 release](https://github.com/Gaurav06120714/VyroEngine/releases/tag/v2.0.0).

**Now in development: VyroEngine 3.0** — the game-maker era.
Full plan: [docs/ROADMAP_V3.md](docs/ROADMAP_V3.md).

| V3 Phase | Goal | Status | Tag |
|----------|------|--------|-----|
| V3.1 — Skeletal Animation | GLB skins + clips, keyframe sampling, CPU skinning — zombies walk | ✅ Complete | v2.1.0 |
| V3.2 — Animated Gameplay | Death animations, facing, waves & health in Outbreak | ✅ Complete | v2.2.0 |
| V3.3 — Real Audio Output | miniaudio device + synthesized SFX: the game makes sound | ✅ Complete | v2.3.0 |
| V3.4 — Text & HUD | Built-in bitmap font: score, hearts, game-over overlay in the viewport | ✅ Complete | v2.4.0 |
| V3.5 — Scene Authoring | Scene save/load + editor create/delete/save workflow | 🔧 Next | v2.5.0 |
| **v3.0.0** | All of the above stabilized | 📋 Planned | v3.0.0 |

---

## Games built on VyroEngine

| Game | Description | Run |
|------|-------------|-----|
| **VyroStrike: Outbreak** | 3D zombie shooter: you are the soldier, gun down the shambling horde. ECS + Input + collision + GLB character models + GL rendering. | `./build/bin/VyroStrike` |

Controls: **A/D** move · **Space** shoot · **R** restart · **Esc** quit

---

## Documentation Index

| File | Description |
|------|-------------|
| [V1.md](V1.md) | Software Requirements Specification (SRS) |
| [V2.md](V2.md) | Architecture Plan & System Design |
| [V3.md](V3.md) | Development Roadmap — Phases 0–6 |
| [V4.md](V4.md) | Development Roadmap — Phases 7–12 |
| [V5.md](V5.md) | Dependency Graph, Team Structure, Folder Architecture |
| [V6.md](V6.md) | Timelines, Risk Analysis & Final Architecture Diagram |
| [rulz/](rulz/) | Engineering Rules & Coding Standards |

---

## Quick Start (Future)

```bash
git clone https://github.com/Gaurav06120714/VyroEngine.git
cd VyroEngine
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

---

## Tech Stack

| Layer | Technology |
|-------|-----------|
| Language | C++23 |
| Build System | CMake 3.28+ |
| Rendering (Primary) | Vulkan 1.3 |
| Rendering (Fallback) | OpenGL 4.6 |
| Scripting | Lua 5.4 / LuaJIT |
| Physics | Custom + Bullet3 integration |
| Audio | OpenAL / miniaudio |
| UI Framework | Dear ImGui (Editor) |
| Windowing | SDL3 / GLFW |
| Asset Pipeline | Custom + Assimp |

---

## Supported Platforms (Roadmap)

- Windows 10/11 (Primary)
- Linux (Ubuntu 22.04+)
- macOS 13+ (Metal fallback)
- WebAssembly (future)
- Console (future)

---

## Repository Structure

```
VyroEngine/
├── engine/          # Core engine source
├── editor/          # VyroEditor source
├── runtime/         # Game runtime
├── tools/           # Build tools, asset pipeline
├── samples/         # Sample games and demos
├── docs/            # Technical documentation
├── tests/           # Unit and integration tests
├── rulz/            # Engineering standards and rules
├── CMakeLists.txt
└── README.md
```

---

## License

MIT License — see [LICENSE](LICENSE)

---

## Author

**Gaurav** — Principal Architect, VyroEngine  
Part of the [VyroEcosystem](https://github.com/Gaurav06120714)

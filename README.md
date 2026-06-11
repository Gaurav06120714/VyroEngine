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

**Now in development: VyroEngine 2.0** — replacing v1's reference
implementations with production backends, without breaking the public APIs.
Full plan: [docs/ROADMAP_V2.md](docs/ROADMAP_V2.md).

| V2 Phase | Goal | Status | Tag |
|----------|------|--------|-----|
| V2.1 — Visual Editor | Dear ImGui editor (hierarchy, inspector, assets, stats over a live 3D viewport) | ✅ Complete | v1.1.0 |
| V2.2 — UDP Transport | Real socket networking behind `ITransport` | ✅ Complete | v1.2.0 |
| V2.3 — Lua Scripting | Lua 5.4 backend behind `ScriptEngine` | 🔧 In Progress | v1.3.0 |
| V2.4 — Vulkan Backend | `VulkanDevice` RHI via MoltenVK, scheduled by the FrameGraph | 📋 Planned | v1.4.0 |
| V2.5 — Texturing & Materials | PNG loading, GPU textures, textured PBR | 📋 Planned | v1.5.0 |
| **v2.0.0** | All of the above stabilized | 📋 Planned | v2.0.0 |

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

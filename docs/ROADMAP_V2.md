# VyroEngine 2.0 Roadmap

**Theme:** v1.0 proved every subsystem behind clean interfaces. v2.0 replaces
the reference implementations with production backends — without changing the
public APIs that v1 froze.

## Phases

| Phase | Goal | Replaces | Tag |
|-------|------|----------|-----|
| V2.1 — Visual Editor | Dear ImGui editor app: dockable hierarchy, inspector, viewport, asset browser panels driven by the existing `EditorContext`/`Inspector` models | headless editor model only | v1.1.0 |
| V2.2 — UDP Transport | Real socket transport implementing `ITransport`; LAN client-server play | `LoopbackTransport` (kept for tests) | v1.2.0 |
| V2.3 — Lua Scripting | Lua 5.4 backend behind `ScriptEngine`'s interface; VyroScript kept as fallback | VyroScript interpreter | v1.3.0 |
| V2.4 — Vulkan Backend | `VulkanDevice` RHI backend via MoltenVK on macOS, scheduled by the `FrameGraph` | OpenGL as fallback | v1.4.0 |
| V2.5 — Texturing & Materials | Image loading (PNG), GPU texture upload, textured PBR materials in the viewer | flat vertex colors | v1.5.0 |
| V2.0 release | All of the above stabilized | — | v2.0.0 |

## Rules
Same as v1: per-file conventional commits, no AI co-authors, tests green at
every step, tag per phase. Public v1 APIs stay source-compatible.

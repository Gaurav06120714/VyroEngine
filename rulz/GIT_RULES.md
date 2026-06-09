# VyroEngine Git Rules

**Version:** 1.0.0

---

## Branch Rules

### Rule G-1: Branch Naming
```
feature/phase-N-short-description
fix/short-description
hotfix/critical-description
release/vX.Y.Z
docs/what-is-being-documented
```

### Rule G-2: Never Commit to main
All work goes through pull requests. `main` is always releasable.

### Rule G-3: Feature Branches from develop
Create feature branches from `develop`, not `main`.

### Rule G-4: Branch Lifetime
Feature branches must not live longer than 2 weeks. If work spans longer, break into smaller branches and merge incrementally.

---

## Commit Rules

### Rule G-5: Conventional Commits
All commits follow [Conventional Commits](https://www.conventionalcommits.org/):

```
<type>(<scope>): <description>

[optional body]

[optional footer]
```

Types: `feat`, `fix`, `perf`, `refactor`, `test`, `docs`, `chore`, `build`, `ci`

Scopes: `ecs`, `renderer`, `physics`, `audio`, `animation`, `scripting`, `editor`, `memory`, `core`, `platform`, `network`

Examples:
```
feat(ecs): add archetype migration for component add/remove
fix(renderer): resolve shadow map z-fighting with bias correction
perf(physics): optimize broadphase BVH to reduce query time 40%
test(memory): add pool allocator stress test for 1M allocs
docs(scripting): add Lua API reference for Vec3 and Quat
chore(build): update vcpkg baseline to 2026-04-01
```

### Rule G-6: Atomic Commits
One logical change per commit. No "fix stuff" commits. No 10-system omnibus commits.

### Rule G-7: Commit Messages Are Complete
Future contributors must understand what changed and why from the commit message alone.

```
// Bad
fix renderer crash

// Good
fix(renderer): resolve crash when resizing window during texture upload

The GL texture upload was happening on the render thread while the
main thread was resizing the swapchain. Added a fence to synchronize
the two operations. Fixes #142.
```

### Rule G-8: No Commit with Failing Tests
The CI must be green before merging. Never force-push to bypass CI on `main` or `develop`.

---

## Tagging Rules

| Tag | When to Create |
|-----|---------------|
| `v0.1.0` | Phase 1 complete |
| `v0.N.0` | Each phase milestone |
| `v1.0.0` | Phase 12 complete — production release |
| `v1.0.1` | Patch/hotfix |

Tag format: semantic versioning `vMAJOR.MINOR.PATCH`

---

## Pull Request Rules

### Rule PR-1: PR Description Required
Every PR must have:
- What changed (bullet points)
- Why it changed (motivation)
- How to test it (test steps or test names)
- Screenshots (for visual changes)

### Rule PR-2: PR Size Limit
PRs over 500 lines of code must be broken up, except for generated code.

### Rule PR-3: Self-Review Before Requesting Review
Author reviews their own diff before marking PR as ready.

### Rule PR-4: Squash Merge
PRs are squash-merged to keep `main`/`develop` history clean. Feature branch WIP commits are collapsed.

---

## .gitignore Policy

Committed: source, assets, CMakeLists, presets, scripts  
Not committed: build outputs, IDE configs, `.env`, compiled assets, OS files

```gitignore
# Build
build/
out/
cmake-build-*/

# IDE
.vscode/
.idea/
*.user

# Compiled assets
*.spv          # SPIRV shaders
*.ktx          # Compressed textures
*.bc           # Compiled Lua bytecode

# OS
.DS_Store
Thumbs.db
```

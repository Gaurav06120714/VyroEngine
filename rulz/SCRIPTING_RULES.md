# VyroEngine Scripting Rules (Lua)

**Version:** 1.0.0 | **Runtime:** LuaJIT 2.1 + sol3

---

## Script Architecture Rules

### Rule LUA-1: Scripts are Components
Every Lua script is attached to an entity via `ScriptComponent`. Scripts don't exist independently.

### Rule LUA-2: Lifecycle Methods are Conventional
Engine calls these methods if defined:

```lua
-- Called once when the entity is added to the scene
function OnCreate() end

-- Called every frame
function OnUpdate(dt) end

-- Called every fixed physics step (60Hz)
function OnFixedUpdate(dt) end

-- Called when entity is destroyed
function OnDestroy() end

-- Called on collision enter
function OnCollisionEnter(other_entity, contact) end

-- Called on trigger enter
function OnTriggerEnter(other_entity) end
```

Never call these yourself — only define them. The engine calls them.

### Rule LUA-3: No Global State in Scripts
Scripts must not store state in Lua global variables. Use local variables or the entity's component data.

```lua
-- WRONG: global state
player_health = 100  -- global, pollutes all scripts

-- CORRECT: local to script or in component
local health = entity:GetComponent(HealthComponent)
health.current = 100
```

### Rule LUA-4: No Blocking Operations
Scripts must never block. No `io.read()`, no `os.sleep()`. Use coroutines for async-style scripting.

```lua
-- WRONG: blocking loop
while not condition_met() do end

-- CORRECT: coroutine-based waiting
function OnUpdate(dt)
    coroutine.wrap(function()
        Wait(2.0)  -- yields back to engine for 2 seconds
        DoAction()
    end)()
end
```

### Rule LUA-5: Error Handling
Lua errors must not crash the engine. The script engine wraps all calls in `pcall`. Log errors don't terminate execution.

---

## API Usage Rules

### Rule API-1: Cache Component Lookups
Don't call `GetComponent` every frame. Cache the reference at `OnCreate`.

```lua
-- WRONG: lookup every frame
function OnUpdate(dt)
    local t = entity:GetComponent(Transform)  -- called 60/sec
    t.position.y = t.position.y + dt
end

-- CORRECT: cache at OnCreate
local transform

function OnCreate()
    transform = entity:GetComponent(Transform)
end

function OnUpdate(dt)
    transform.position.y = transform.position.y + dt
end
```

### Rule API-2: Vec3 Math
All positions, velocities, directions use `Vec3`. Never use raw tables for spatial data.

```lua
local pos = Vec3(1, 2, 3)
local dir = Vec3.normalize(Vec3(1, 0, 0))
local dot = Vec3.dot(a, b)
```

### Rule API-3: Entity Lifetime
Always check if an entity is alive before using a cached reference. Entities can be destroyed between frames.

```lua
local cached_enemy  -- cached in OnCreate

function OnUpdate(dt)
    if not cached_enemy:IsAlive() then return end
    -- safe to use
end
```

---

## Performance Rules

### Rule PERF-LUA-1: Prefer C++ for Hot Paths
Any logic running > 1000 times per frame belongs in a C++ System, not Lua. Lua is for game logic, not engine loops.

### Rule PERF-LUA-2: LuaJIT Traces
Write Lua code that JIT-compiles well:
- Avoid `pairs()` on hot tables (use indexed arrays)
- Avoid creating tables inside loops
- Avoid polymorphic calls (same call site with different types defeats JIT)

### Rule PERF-LUA-3: No Garbage in Hot Path
Don't create temporary strings, tables, or closures inside `OnUpdate`. Allocate in `OnCreate` and reuse.

---

## Sandboxing Rules

### Rule SEC-LUA-1: Restricted Environment
Scripts run in a sandboxed Lua environment. Forbidden:
- `io.*` — file system access
- `os.*` — OS access (except `os.clock`, `os.time`)
- `debug.*` — debug library
- `loadfile`, `dofile`, `load` — dynamic code loading
- `require` — module loading (use engine's asset system instead)
- `package.*` — package system

### Rule SEC-LUA-2: Script Communication via Events
Scripts don't hold direct references to other scripts. They communicate via the EventBus.

```lua
-- Script A: publish
EventBus.Publish("PlayerDied", { score = 100 })

-- Script B: subscribe
EventBus.Subscribe("PlayerDied", function(data)
    print("Game over, score: " .. data.score)
end)
```

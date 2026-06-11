# VyroEngine Models

Drop your 3D model here to view it in the model viewer.

## How to use your own model

1. Export or download a model in **Wavefront `.obj`** format.
2. Save it in this folder as **`model.obj`**:
   ```
   assets/models/model.obj
   ```
3. Run the viewer:
   ```bash
   ./build/bin/vyro_model
   ```
   It loads `assets/models/model.obj` if present, otherwise falls back to the
   bundled `cube.obj`, otherwise a procedural cube.

You can also pass any path explicitly:
```bash
./build/bin/vyro_model /path/to/your/model.obj
```

## Supported format

- `.obj` with `v` (positions), `vt` (texcoords), `vn` (normals), and `f`
  (faces — triangles or polygons, which are triangulated automatically).
- Missing normals are generated from face geometry.
- The viewer auto-centers and scales any model to fit the view, so size and
  origin do not matter.

## Where to get free `.obj` models

- Poly Pizza (poly.pizza), Sketchfab (downloadable, `.obj`), Quaternius,
  Kenney.nl. Export from Blender via *File → Export → Wavefront (.obj)*.

> Note: `.mtl` materials and textures are not yet applied — the viewer renders
> the geometry with simple directional lighting. Textured/PBR rendering is a
> later phase.

## Bundled model library (from poly.pizza)

### Characters (`characters/`, animated GLB)
| File | Model | Author | Animations |
|------|-------|--------|------------|
| `soldier_shooting.glb` | Soldier | KolosStudios | standing/aiming/shooting |
| `zombie_animated.glb` | Animated Zombie | Quaternius | walk/attack/death |
| `platformer_jump_die.glb` | Animated Platformer Character | Quaternius | run/jump/death |

### Weapons (`weapons/`, GLB)
| File | Model | Author |
|------|-------|--------|
| `pistol_9mm.glb` | 9mm Pistol | Quaternius |
| `assault_rifle.glb` | Assault Rifle | Quaternius |

All CC0/CC-BY from poly.pizza.

> **Format note:** animated models ship as `.glb` (glTF binary) because OBJ
> cannot store rigs or animations. The engine's OBJ loader does not render
> these yet — a glTF loader (mesh + skeleton + clips into the existing
> `MeshData`/`AnimationClip` types) is the next asset-pipeline milestone.

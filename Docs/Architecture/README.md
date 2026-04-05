# Engine Architecture

This section documents how OpenJones3D is structured internally, how the original Jones3D engine runtime is modeled, and where OpenJones3D intentionally extends or modernizes that behavior.

The architecture is easiest to understand as a layered stack:

```text
Jones3D     application flow, menus, HUD, startup, level transitions
  |
sith        gameplay runtime, world state, things, physics, collision, COG, AI
  |
rdroid      renderer-facing scene primitives, cameras, materials, keyframes, raster cache
  |
std         platform abstraction, display/device enumeration, input, files, config
sound       low-level audio backend, banks, channels, handles
wkernel     top-level Win32 process loop and window management
w32util     Windows helpers such as registry access
  |
j3dcore     OpenJones3D-specific hook/trampoline layer used to replace original code
```

OpenJones3D keeps that original layering intact on purpose. The project is not a fresh greenfield engine with a new architecture; it is a reconstruction of the retail runtime layout, with the codebase organized so that original subsystems keep their historical boundaries as much as possible.

## What To Read First

- [Quick-Reference.md](Quick-Reference.md) gives a short mental model of the whole engine, including the layer split, the world/thing model, and why sectors and adjoins matter to both rendering and collision.
- [Boot-And-Lifecycle.md](Boot-And-Lifecycle.md) explains process startup, subsystem startup, world opening, frame execution, and shutdown.
- [World-And-Resources.md](World-And-Resources.md) explains the `SithWorld` container, world loading, section parsing, and resource ownership.
- [Frame-Loop-And-Simulation.md](Frame-Loop-And-Simulation.md) explains per-frame update order, timing, events, the thing system, physics, and collision.
- [Collision-System.md](Collision-System.md) explains movement tracing, hit collection, handler dispatch, line-of-sight queries, floor finding support, and collision response.
- [Control-And-Input.md](Control-And-Input.md) explains the raw input backends, logical gameplay bindings, idle detection, and how player/camera control intent is produced.
- [Event-And-Message-Systems.md](Event-And-Message-Systems.md) explains the deferred event scheduler, COG message dispatch, and the multiplayer/file synchronization message bus.
- [Camera-System.md](Camera-System.md) explains the built-in system cameras, camera interpolation state, camera collision/placement, external-camera rig behavior, and how COG scripts drive cinematic cutscenes.
- [Rendering-And-Visibility.md](Rendering-And-Visibility.md) explains the camera stack, sector visibility, PVS, off-screen thing/light collection, dynamic lighting, and the DX6/DX9 rendering layers.
- [Scripting-AI-And-Audio.md](Scripting-AI-And-Audio.md) explains the COG VM, AI control blocks and awareness propagation, and the audio stack.
- [Sound-System.md](Sound-System.md) explains the low-level sound engine, the gameplay-aware mixer, sound classes, voice playback, and the main OpenJones3D audio extensions.
- [QOL-And-Compatibility.md](QOL-And-Compatibility.md) explains the `J3D_QOL_IMPROVEMENTS` compatibility model and the main places where OpenJones3D intentionally diverges from legacy behavior.

## Source Layout

The main architecture lives in these source trees:

- [`Jones3D/`](../../Jones3D) contains the top-level application layer: startup, menus, HUD, gameplay flow, configuration UI, and Jones-specific helpers.
- [`Libs/sith/`](../../Libs/sith) contains the gameplay runtime: worlds, things, COG, AI, physics, collision, cameras, rendering, save/load, weapons, actors, and related systems.
- [`Libs/rdroid/`](../../Libs/rdroid) contains the renderer-facing primitive and raster layer.
- [`Libs/std/`](../../Libs/std) contains platform-facing services, display/device setup, input, config, file helpers, and the DX6/DX9 rendering backends.
- [`Libs/sound/`](../../Libs/sound) contains the low-level sound engine used by the gameplay-side mixer.
- [`Libs/wkernel/`](../../Libs/wkernel) contains the outer Win32 host loop and window creation/event processing.
- [`Libs/w32util/`](../../Libs/w32util) contains Windows-specific utility helpers, most notably registry access.
- [`Libs/j3dcore/`](../../Libs/j3dcore) contains the reconstruction-specific hook/trampoline infrastructure.

## Original Runtime Versus OpenJones3D

When reading the rest of these notes, it helps to keep three categories in mind:

1. Original architecture preserved

   The subsystem boundary or algorithm matches the retail engine closely. OpenJones3D simply reimplements it in C.

2. Original architecture preserved, implementation improved

   The subsystem still lives in the same layer and serves the same purpose, but the code has fixes, validation, or safer memory handling.

3. Original architecture extended

   OpenJones3D keeps the old path available for compatibility, but adds a new path behind compile-time or runtime configuration. Examples include the DirectX 9 backend, the JSON-backed `Jones.cfg` configuration system, larger sound channel budgets, and the breadth-first culled-sector traversal used for off-screen thing/light collection.

The detailed comparison points live in [QOL-And-Compatibility.md](QOL-And-Compatibility.md), but those differences are also called out in the subsystem pages where they matter.

## Related Documentation

- For the COG language, messages, host functions, flags, and value sets, see [../COG/README.md](../COG/README.md).
- For resource file formats such as `3DO`, `KEY`, `MAT`, `PUP`, `AI`, `SND`, `SPR`, and `UNI`, see [../Formats/README.md](../Formats/README.md).
- For runtime configuration keys, see [../Jones.cfg.md](../Jones.cfg.md).

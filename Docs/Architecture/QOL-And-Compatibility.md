# QOL And Compatibility

This page documents how OpenJones3D preserves the original architecture while still adding modern behavior, fixes, and optional replacements under compile-time or runtime control.

Primary source files:

- [`Libs/j3dcore/j3d.h`](../../Libs/j3dcore/j3d.h)
- [`Libs/j3dcore/j3dhook.h`](../../Libs/j3dcore/j3dhook.h)
- [`CMakeLists.txt`](../../CMakeLists.txt)
- [`Libs/sith/Engine/sithRender.h`](../../Libs/sith/Engine/sithRender.h)
- [`Libs/sith/Engine/sithRender.c`](../../Libs/sith/Engine/sithRender.c)
- [`Libs/std/General/stdConfig.c`](../../Libs/std/General/stdConfig.c)
- [`Libs/std/General/stdPlatform.c`](../../Libs/std/General/stdPlatform.c)
- [`Libs/sith/Engine/sithPhysics.c`](../../Libs/sith/Engine/sithPhysics.c)
- [`Libs/sith/Devices/sithSound.h`](../../Libs/sith/Devices/sithSound.h)
- [`Libs/sith/Devices/sithSoundMixer.c`](../../Libs/sith/Devices/sithSoundMixer.c)
- [`Libs/sith/World/sithMaterial.c`](../../Libs/sith/World/sithMaterial.c)
- [`Libs/sith/Engine/sithPuppet.c`](../../Libs/sith/Engine/sithPuppet.c)

## Reconstruction First, Modernization Second

OpenJones3D is built around a simple principle:

- preserve the original module boundaries and runtime model
- reimplement original behavior faithfully
- add modern replacements as explicit, reviewable extensions

That is why the engine still looks like Jones3D/Sith internally instead of becoming a new engine with the same file formats.

## j3dcore: The Compatibility Layer

The main OpenJones3D-specific architectural addition is [`j3dcore`](../../Libs/j3dcore).

It provides:

- far-address declarations for original globals
- trampolines to original functions
- hook installation for reimplemented functions

[`J3D_HOOKFUNC()`](../../Libs/j3dcore/j3dhook.h#L44) patches the original entry point so execution jumps into the new C implementation. [`J3D_TRAMPOLINE_CALL()`](../../Libs/j3dcore/j3dhook.h#L28) can still invoke the original code when a function is only partially replaced or when original behavior is still needed.

This is not just a development convenience. It is a central architectural strategy:

- original binary behavior remains accessible during reconstruction
- new implementations can replace functions incrementally
- compatibility is maintained even before every subsystem is fully rewritten

## Compile-Time Profiles

The main build-time switches are:

- `JONES3D_USE_DIRECTX9`
- `JONES3D_QOL_IMPROVEMENTS`
- `JONES3D_SPEEDRUN_BUILD`

`J3D_QOL_VALUE(newValue, legacyValue)` is the main mechanism used inside the codebase to keep both the modernized and legacy behavior profiles available.

That means many OpenJones3D enhancements are not scattered as ad hoc one-off hacks. They are encoded as explicit compatibility-profile choices.

## Original Behavior Versus OpenJones3D Extensions

The following are some of the most important architecture-level or runtime-level differences.

| Area | Legacy / Original Direction | OpenJones3D Direction |
| --- | --- | --- |
| Graphics backend | DirectX 6.1c path only | DirectX 9 backend added, legacy DX6 path kept |
| Hook model | Original binary only | Hook/trampoline reconstruction through `j3dcore` |
| Configuration | Windows Registry | `Jones.cfg` JSON with registry fallback/migration |
| Off-screen thing/light traversal | Legacy DFS-like traversal | BFS traversal available and enabled by default under `QOL` |
| Thing/light collection distance | 8.0 / 8.0 default | 18.0 / 18.0 default under `QOL` |
| Timer source | `timeGetTime()` | `QueryPerformanceCounter()` under `QOL` |
| Player fixed-step physics | legacy-tuned 50 Hz behavior | configurable fixed-step rate, default 150 Hz under `QOL` |
| Sound channels | small legacy limit | much larger simultaneous channel budget |
| SW mixer falloff default | linear | logarithmic under `QOL` |
| CND external materials/keyframes | disabled by default | enabled by default under `QOL` |

## BFS Versus Legacy DFS-Like Sector Collection

This is one of the most important render-architecture changes.

Original direction:

- recursive DFS-like walk from the directly visible sectors
- sector marked on first encounter
- traversal order depends on adjoin order
- can under-collect nearby sectors if a worse path reaches them first

OpenJones3D direction:

- queue-based breadth-first walk
- nearer sectors are processed first
- less sensitive to path-order bias
- better at collecting neighboring sectors whose things/lights still affect the visible frame

Why it was added:

- visible geometry can depend on nearby sectors outside the immediate frustum-visible set
- dynamic lights can affect on-screen geometry from off-screen sectors
- some open or winding layouts benefit from a more stable traversal order

Architecturally, this change is still conservative. It does not replace the sector/adjoin model; it only changes how the secondary off-screen collection pass traverses that model.

## Configuration Architecture: Registry To Jones.cfg

OpenJones3D does not simply delete the original registry model. Instead:

1. the registry helper still starts
2. `stdConfig` opens or creates `Jones.cfg`
3. reads first consult JSON
4. missing keys fall back to mapped registry values
5. fallback values are written back into JSON automatically

This makes the engine more modular because subsystems can now read and write structured config keys directly without forcing all modern configuration through registry code.

It also preserves backward compatibility with existing installs that only have the original registry values.

## Timing Precision

Under `QOL`, [`stdPlatform_GetTimeMsec()`](../../Libs/std/General/stdPlatform.c#L95) uses `QueryPerformanceCounter()` instead of `timeGetTime()`.

Why this matters architecturally:

- higher timing precision improves frame-time measurement quality
- the engine keeps the same millisecond-facing timing API, so higher layers do not change
- pause, frame timing, and event scheduling continue to work through the same interfaces

This is a good example of an implementation improvement that preserves the original subsystem boundaries.

## Fixed-Step Player Physics

The player detached-physics path already used fixed-step accumulation logic. OpenJones3D keeps that design, because it is part of the gameplay contract, but changes the default fixed-step frequency under `QOL`.

Why it matters:

- player motion is more sensitive to integration-rate error than many generic physics things
- a higher default fixed-step rate improves precision on modern systems
- the architecture remains compatible because the fixed-step accumulator model is unchanged

So this is an "original architecture preserved, implementation extended" change rather than a rewrite.

## Audio Budget And Falloff

The audio architecture is unchanged, but the defaults are expanded:

- `SITHSOUND_MAXCHANNELS` is much larger under `QOL`
- the software mixer defaults to logarithmic falloff instead of linear

Why this matters:

- the original low sound-channel budget can clip busy scenes
- newer hardware/runtime expectations make a larger channel budget reasonable
- the mixer layer already owns attenuation policy, so switching the default fits cleanly into the existing architecture

## External Resource Loading Defaults

Under `QOL`, external loading of certain CND-embedded resources is enabled by default:

- materials
- keyframes

Architecturally, this keeps the original "embedded resources in the world file" behavior available, but makes loose-file override paths easier to use for modding, testing, and iteration.

## Renderer Backends

The original renderer architecture is preserved as a layered stack, but the lower backend layer now has two implementations:

- DirectX 6.1c
- DirectX 9

The DX9 path adds:

- shader support
- VBO/IBO support
- more modern texture/device capability handling
- support for higher resolutions and features expected on modern systems

The important architectural point is that the upper scene-building logic in `sith` and `rdroid` remains substantially the same. OpenJones3D modernizes the backend under the existing renderer rather than replacing the whole render model.

## Compatibility Philosophy

OpenJones3D generally follows one of these patterns when introducing improvements:

1. Keep the legacy path and add the new one behind a switch.
2. Improve the implementation without changing the public subsystem boundary.
3. Change the default while still allowing the legacy behavior profile.

That is why the project can support:

- a legacy-style DirectX 6.1c renderer
- a DirectX 9 renderer
- a legacy-compatible non-`QOL` build profile
- a modernized default profile

without losing the historical structure of the engine itself.

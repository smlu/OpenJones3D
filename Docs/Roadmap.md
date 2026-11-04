# Roadmap

Where Jones3D EX stands and what comes next. The reasons behind the order are in
[chapter 4 of the architecture docs](arc42/04-solution-strategy.md).

| Milestone | Result | Status |
|---|---|---|
| M0. Toolchain | The engine builds and its tests run outside MSVC | Done |
| M1. Running baseline | A standalone executable runs the game; regression runs against the original | Done |
| M2. Layer design | Platform code separated from engine code, design approved | Done |
| M3. Subsystems behind the layer | Window, input, display, rendering, sound and dialogs behind interfaces; Win32 and DirectX 9 as one backend | Done |
| M4. Null backends and native 64-bit | The game runs headless on 64-bit Linux, deterministic across 32-bit, 64-bit and sanitizer builds | Done |
| Direction | Hard fork, Jones3D EX on the Babylon Engine, these docs | Done |
| M5. Babylon foundation | `Babylon/` with tests, SDL3 window, input, timer and files, Null backends, cvars, adapters. CI on Windows, Linux and macOS | Done |
| M6. Babylon renderer | SDL3 GPU renderer at parity with DirectX 9; then the Win32 and DirectX path is removed | |
| M7. Playable | miniaudio sound, the original dialogs as in-game screens, playable on Windows, Linux and macOS | |
| M8. Release | Packages, game data installation, macOS signing, first Jones3D EX release | |
| M9. Presentation | Widescreen and high-resolution output; bloom and depth of field as post-processing; high frame rates, which first needs an answer to how much of the simulation depends on frame time | |
| M10. Modern lighting | Per-pixel dynamic lighting and screen-space ambient occlusion, with the original vertex lighting kept as the classic mode | |
| M11 and later. Babylon extras | Visible console; modding (loose-file overrides, mod folders, COG); threading last. Open idea: shadows and geometry on the GPU | |

Each milestone ends with a check that can be seen, such as a run of the game or a passing CI job, before the
next one starts.

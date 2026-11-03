# 9. Architecture decisions

Each decision is an ADR in the [Nygard format](https://cognitect.com/blog/2011/11/15/documenting-architecture-decisions):
context, decision, consequences. Decisions aren't edited after the fact; a new ADR supersedes an old one.

| ADR | Decision | Status |
|---|---|---|
| [ADR-0001](ADR-0001-hard-fork.md) | Hard fork from OpenJones3D | Accepted |
| [ADR-0002](ADR-0002-source-port-on-babylon-engine.md) | A Kex-style source port on the Babylon Engine | Accepted |
| [ADR-0003](ADR-0003-fidelity-split.md) | Original simulation, enhanced presentation | Accepted |
| [ADR-0004](ADR-0004-drop-binary-compatibility.md) | Drop binary compatibility with the retail executable | Accepted |
| [ADR-0005](ADR-0005-game-logic-stays-c11.md) | Game logic stays C11 with recovered names | Accepted |
| [ADR-0006](ADR-0006-babylon-engine-in-repo.md) | Babylon Engine lives in this repository, game-agnostic | Accepted |
| [ADR-0007](ADR-0007-adapter-between-game-and-babylon.md) | An adapter between the game and Babylon | Accepted |
| [ADR-0008](ADR-0008-babylon-license.md) | Babylon Engine is MPL-2.0 with permissive dependencies | Accepted |
| [ADR-0009](ADR-0009-sdl3-and-sdl3-gpu.md) | SDL3 for the platform and the SDL3 GPU API for rendering | Accepted |
| [ADR-0010](ADR-0010-miniaudio.md) | miniaudio for audio | Accepted |
| [ADR-0011](ADR-0011-retire-win32-directx.md) | Retire the Win32 and DirectX path after renderer parity | Accepted |
| [ADR-0012](ADR-0012-desktop-first-portable-interfaces.md) | Desktop platforms first, portable interfaces | Accepted |
| [ADR-0013](ADR-0013-cvars-in-foundation.md) | Cvars are part of the Babylon foundation | Accepted |
| [ADR-0014](ADR-0014-arc42-documentation.md) | Architecture documentation in arc42 | Accepted |
| [ADR-0015](ADR-0015-babylon-modules-as-cmake-targets.md) | Babylon modules as CMake targets | Accepted |
| [ADR-0016](ADR-0016-virtual-interfaces-chosen-at-startup.md) | Babylon interfaces are virtual, backends are chosen at startup | Accepted |
| [ADR-0017](ADR-0017-babylon-toolchain-and-dependencies.md) | Babylon builds as C++20 with pinned dependencies and doctest | Accepted |
| [ADR-0018](ADR-0018-replays-record-frame-time-and-raw-input.md) | Replays record frame time and raw input per frame | Accepted |
| [ADR-0019](ADR-0019-user-directory-for-writes.md) | Writes go to a user directory; the game keeps its search order | Accepted |
| [ADR-0020](ADR-0020-cvar-flags.md) | Cvars carry flags, including a simulation flag | Accepted |
| [ADR-0021](ADR-0021-detect-game-data.md) | Detect the installed game data at startup | Accepted |

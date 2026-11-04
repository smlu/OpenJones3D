# Jones3D EX agent guide

Jones3D EX is an enhanced source port of Jones3D, the engine of *Indiana Jones and the Infernal Machine*.
The restored C game logic runs on the **Babylon Engine**, a game-agnostic C++ platform framework, in the
model of the Kex engine. It is a hard fork of OpenJones3D, which reconstructed the engine function by
function; binary compatibility with the retail executable is no longer a goal.

This file is a summary. When it disagrees with these documents, they win:

- [Docs/arc42/README.md](Docs/arc42/README.md): architecture, constraints, decisions (ADRs)
- [CONTRIBUTING.md](CONTRIBUTING.md): workflow, naming, layering, C style, memory, commits
- [Docs/Testing.md](Docs/Testing.md): test workflow, fixtures, build commands, validation report
- [Docs/Roadmap.md](Docs/Roadmap.md): milestones and status
- [Docs/Formats/README.md](Docs/Formats/README.md), [Docs/COG/README.md](Docs/COG/README.md), [Docs/Jones.cfg.md](Docs/Jones.cfg.md): formats, scripting, configuration

Read the owning module, its tests and the relevant doc before changing code.

## Layout

- `Jones3D/`: application layer (game flow, HUD, inventory, config)
- `Libs/sith/`: gameplay, COG VM, physics, collision, AI, savegames
- `Libs/rdroid/`: rendering pipeline, primitives, vector and matrix math
- `Libs/smush/`: cutscene decoding
- `Libs/std/`, `Libs/sound/`, `Libs/wkernel/`, `Libs/w32util/`: platform-facing modules, becoming the adapter onto Babylon
- `Libs/j3dcore/`: lowest support layer (`j3d.h` is generated from `j3d.h.in`)
- `Babylon/`: the Babylon Engine, C++, MPL-2.0, with its own tests (from M5)
- `<area>/Tests/`: tests beside the code they cover; fixtures in `Tests/tv/`
- `game/`: local, untracked game files

Dependencies only flow downward:

```text
Jones3D -> sith -> sound -> rdroid -> std -> j3dcore
game and adapter (std, sound, wkernel) -> Babylon -> SDL3, SDL3 GPU, miniaudio
```

**Babylon is a hard boundary.** Nothing in `Babylon/` includes from `Libs/`, `Jones3D/` or anything else
outside `Babylon/`, or names a Jones3D or Sith concept. No code is copied into Babylon from AGPL or GPL
sources: not from `Libs/`, earlier backends, OpenJones3D branches, Jones3D-Portable or PowerslaveEX. Babylon
dependencies are permissive only (zlib, MIT, BSD, Apache-2.0, public domain). See ADR-0006 to ADR-0008.

## Commands

Native build and unit tests (Clang; GCC needs extra flags, see chapter 11 of the arc42 docs):

```bash
cmake -S . -B build/native -G Ninja -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++ -DCMAKE_BUILD_TYPE=Debug
cmake --build build/native
ctest --test-dir build/native -N -L unit
ctest --test-dir build/native --output-on-failure -L unit
```

Confirm with `-N` that the expected suites were discovered; zero tests validates nothing.

Headless run, the basis for behavior checks:

```bash
build/native/Jones3D/Jones3D-headless --load <save>.nds --frames 1000 --state state.txt
```

The legacy Win32 and DirectX 9 build (until M6) builds with MSVC Win32 or with MinGW through
`cmake/mingw-i686.cmake`; both produce the standalone `Jones3D.exe`. Code that touches it needs a Windows build. Say
so when you couldn't build it, and never describe unbuilt or untested code as validated.

## Fidelity

- **Simulation** (physics, collision, AI, COG, game time, savegames) is measured against retail behavior.
  Two profiles: **Enhanced** (the default) and **Retail**. OpenJones3D's QOL changes are this project's
  enhancements, and new simulation changes join them, on by default. Each is an option the Retail profile
  turns off; with the Retail profile, the retail trace must stay identical.
- **Presentation** (renderer, resolution, audio output, UI) is free to improve.
- Keep the simulation deterministic: no wall-clock time, thread timing, uninitialized memory or pointer
  values in anything that reaches state.
- An unexpected trace difference is investigated first. Never edit expected traces or test vectors to make a
  change pass.
- Pure bug and crash fixes are always on. If one changes the retail trace, it needs the owner's decision.

## Code rules

Game and adapter code (C11):

- Recovered names stay, so code can be compared with the decompilation. Functions are
  `<moduleOrFile>_<ActionObject>`, such as `stdConffile_Open`.
- Locals use the prefixes `p`, `pp`, `a`, `b`, `cur`, `num`, `pf`, `pOut`, `pDst`, `pSrc`.
- Style: four spaces, braces on their own lines, `if ( condition )`, `FunctionName(arg0, arg1)`,
  `char* pName`, `NULL`. Declare locals near first use and align `=` across consecutive initialized
  declarations. Match nearby code.
- Reuse existing helpers (`STD_ARRAYLEN`, `STD_ZEROMEM`, `J3D_QOL_VALUE`, `STDMATH_CLAMP`).
- Use `STDMALLOC`, `STDREALLOC` and `STDFREE`; never mix allocators.
- In-memory struct layouts may change. On-disk formats may not: read and write them through explicit
  serialization, never by reading bytes into a struct.
- In game logic only (`Jones3D/`, `Libs/sith/`, `Libs/rdroid/`, `Libs/smush/`), mark divergence from the original at the
  changed site with `// Fixed:`, `// Added:` or `// Altered:`. Not in the adapter or Babylon.
- TODOs name an actionable problem: `// TODO:`, `// TODO: [BUG]`, `// TODO: [RE]`, `// TODO: [COMPAT]`.

Babylon code (C++):

- Babylon has its own naming and style, set in M5 and documented in `Babylon/`. Interfaces are designed for
  their role, not for one game's calls.
- Every interface has a Null backend that honors its contract without a device.
- Adapter files may be C++ and export the recovered functions as `extern "C"`.
- Babylon modules are CMake targets (`babylon_add_module(<name> DEPS ...)`) with a public
  `include/babylon/<module>/` and a private `src/`. Use only modules you declare; `PUBLIC` only for
  dependencies in public headers; no cycles; no static self-registration. CI checks the target graph
  against an allowed list (ADR-0015).

Everywhere:

- Validate counts and sizes from files, configs, savegames, packets, scripts and the OS where they enter.
- No platform types (Windows, DirectX, SDL) in shared headers above the backends.
- Keep helpers `static` and types private unless another module needs them now. No abstractions without a
  second user.

## Interfaces

These apply when a change adds or changes a function another module calls, especially Babylon interfaces
and the adapter.

- **Every observable behavior of game-facing functions is a contract.** Call order, return values, limits,
  message text, timing and save formats can be relied on by scripts, savegames or speedruns. Moving code
  behind the adapter must not change them.
- **Header first.** Write the header and its comments before any backend. An interface is proven when two
  backends, one of them Null, implement it without `#ifdef`s in the calling code.
- **Say what each function promises:** return value, ownership and lifetime of pointers, units, ranges, and
  when it may be called.
- **Errors:** recovered functions keep their original convention, which isn't uniform (`stdConffile_Open`
  returns 1 on success, `std3D_SetProjection` returns 0). Check the header. New functions return `bool` and
  log failures.
- **Lifecycle:** `_Startup`, `_Open`, `_Close`, `_Shutdown` where they apply. Closing something that isn't
  open is a safe no-op.
- **Don't leak the backend through behavior:** no caller may depend on one backend's buffer layout,
  coordinate convention, error codes or batching.
- **Test through the interface,** including failure paths, against every backend.

## Tests

- Behavior changes need focused tests; fixes need regression tests in the same change. Prove the test fails
  on the old behavior where practical.
- Game code: `<module>Test.c` in the owning `Tests/` directory, Unity Fixture. Babylon: tests inside
  `Babylon/`.
- Check return values, every output, state changes, complete strings, exact serialized bytes. Each test
  passes alone and in any order.
- Test vectors for original behavior come from the original game or the decompilation, with recorded
  provenance. Reconstructed code must not produce its own expectations.
- Permanent tests don't depend on game assets or launch the original executable.

## References

- Retail behavior is the reference for the simulation. The byte-exact decompilation
  (andrewspode/infernal-machine-decomp-public) is the tie-breaker; read it, don't copy it.
- OpenJones3D's `upstream-compatible-final` tag is the last binary-compatible state.
- Never commit game assets or original executables.

## Commits and changes

- Workers never commit or push. The supervising session commits reviewed slices, and pushes only with the
  owner's consent.
- Keep commits scoped, ideally one module and one theme each. Don't mix cleanup with a behavioral fix.
- Subjects are `[<module>] Subject` or `[<module>,<module>] Subject`, for example
  `[std] Harden path and config parsing`, `[Babylon] Add the Null input backend`. General updates need no prefix.
- Update `Docs/` when behavior, configuration, formats or architecture change. Lasting decisions get an ADR
  in `Docs/arc42/09-decisions/`. Keep `CHANGELOG.md` to user-visible changes.
- Change descriptions include the validation report from Docs/Testing.md: passes, failures, ignores and tests
  not run, listed separately.
- Keep prompts, scratch notes and temporary files out of commits. `.claude/` is local and ignored.
- Don't name a specific assistant model in source or project docs.

## Supervisor instructions

The session supervising this repo follows @.claude/supervisor.md. If you were given a brief, the brief is
your task and this doesn't apply to you. Workers don't load the `incremental-implementation` or
`code-review-and-quality` skills: those are for the supervisor.

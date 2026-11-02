# 8. Crosscutting concepts

## 8.1 Fidelity and traces

The simulation is measured against retail behavior ([ADR-0003](../09-decisions/ADR-0003-fidelity-split.md)).

| Term | Meaning |
|---|---|
| Simulation | Physics, collision, AI, COG, game time, events, savegames: everything that decides what happens |
| Presentation | Rendering, resolution, audio output, UI: everything that decides how it is shown |
| Retail profile | All simulation options set to retail behavior |
| Default profile | OpenJones3D's QOL changes on, which is what a player gets |
| Trace | The simulation state written by a headless run, frame by frame |

Rules:

- A change to presentation needs no trace change.
- A change to the simulation needs an option. With the option off, the retail trace stays identical.
- An unexpected trace difference is investigated before anything else changes. Expected traces are never
  edited to make a change pass.
- The simulation stays deterministic: no wall-clock time, thread timing, uninitialized memory or pointer
  values in anything that reaches the state.
- Pure bug and crash fixes are always on. If a fix changes the retail trace, it needs a decision, because
  speedruns may depend on the bug.

[QOL-And-Compatibility.md](QOL-And-Compatibility.md) lists OpenJones3D's QOL changes as they are today:
one compile-time profile, which becomes individual runtime options.

## 8.2 Original names and divergence markers

Game code keeps its recovered names, so it can be compared with the decompilation
([ADR-0005](../09-decisions/ADR-0005-game-logic-stays-c11.md)). Where code deliberately differs from the
original, a short comment at the changed site says why: `// Fixed:`, `// Added:` or `// Altered:`.
Babylon has its own naming and isn't bound by this.

## 8.3 Untrusted input

Everything read from game files, savegames, `Jones.cfg`, COG scripts, network packets or the operating
system is untrusted. Counts, sizes and indexes are checked where they enter, not deep inside. On-disk
structures are read and written by explicit serialization, never by reading bytes into an in-memory struct,
because in-memory layouts differ between 32-bit and 64-bit.

## 8.4 Memory

Game and adapter code allocate through `STDMALLOC`, `STDREALLOC` and `STDFREE`, and never mix allocators.
Babylon uses C++ ownership (RAII, owning smart pointers) inside, and states in each interface who owns what
crosses it.

## 8.5 Configuration

`Jones.cfg` holds the settings inherited from OpenJones3D ([Jones.cfg.md](../../Jones.cfg.md)). New settings
are Babylon cvars from M5 on ([ADR-0013](../09-decisions/ADR-0013-cvars-in-foundation.md)). Cvars that
affect the simulation are written into traces, so a trace can be replayed with the settings that made it.

## 8.6 Errors and logging

Recovered functions keep their original error conventions, which aren't uniform: check the header before
testing a result. New functions return `bool` for success or failure. Failures are logged, through
`STDLOG_*` in game and adapter code and through Babylon's log in Babylon.

## 8.7 Testing

| Level | What | Where |
|---|---|---|
| Unit | One function or module, Unity Fixture in C, Babylon's own framework in C++ | `Tests/` beside the code, `Babylon/` tests inside Babylon |
| Interface | Each Babylon interface's contract, run against every backend including Null | `Babylon/` |
| Trace | Whole-game simulation against the retail and default profiles | Headless runner |
| Visual | The new renderer against DirectX 9, until M6 | Manual and screenshot comparison |

Details are in [Testing.md](../../Testing.md).

## 8.8 Licensing boundary

`Babylon/` is MPL-2.0, everything else AGPL-3.0 ([ADR-0008](../09-decisions/ADR-0008-babylon-license.md)).
No code moves from the AGPL side into Babylon. CI enforces the boundary: Babylon builds alone, an include
check rejects paths outside `Babylon/`, and a license check rejects non-permissive dependencies.

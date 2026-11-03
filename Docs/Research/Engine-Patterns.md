# Patterns from comparable engines

Research notes, 2026-11-03. Which patterns from source ports and platform frameworks suit Jones3D EX, checked
against this codebase. This page records findings and their sources; decisions taken from them are ADRs in
[arc42 chapter 9](../arc42/09-decisions/README.md).

## Projects looked at

| Project | What it is | Why it's comparable |
|---|---|---|
| ScummVM | One executable running many old adventure engines | A platform interface (`OSystem`) under restored game engines |
| libultraship | Framework under Ship of Harkinian and other N64 decompilation ports (MIT) | Unmodified decompiled C, a port layer, a generic framework |
| Doom source ports (Crispy Doom, Odamex) | Ports of the Doom engine that keep demo compatibility | Original simulation, enhanced presentation |
| Quake 3 (id source, ioquake3, Quake3e) | Engine with game modules, cvars and a layered file system | Classic cvar and file system design |
| Kex (Nightdive Studios) | Framework under Nightdive's remasters | The model for Babylon |

## Findings per project

### ScummVM

- `common/system.h` defines `OSystem`, the interface every backend implements. It holds slots, pointers to
  manager objects for events, timers, savefiles and so on. A backend fills the slots in `initBackend()`, and
  `OSystem` creates defaults for slots left empty.
- Each engine plugin provides a `MetaEngine`, a factory with `createInstance(OSystem*, Engine**)`. Detection
  of game data is split into its own class and always built into the executable.
- Every `Engine` is constructed with the `OSystem*` and reaches platform services only through it.
- `main` initializes the `OSystem`, finds the right `MetaEngine` through the plugin manager, which then
  creates the `Engine`.

### libultraship

- Layers in a typical port (the BattleShip port documents them best): decompiled game code, unmodified C;
  a port layer that fixes endianness, owns `Ship::Context` and the resource factories, and bridges the game
  to the framework; the framework itself with SDL input, miniaudio output, a resource manager and rendering.
- Game assets are extracted from the user's ROM into archives (`.otr`, now `.o2r`). At startup the resource
  manager mounts the game's archive and the port's own archive, and code requests resources by path.
- The port's archive stores a version that must match the build.

### Doom source ports

- Doom updates the simulation 35 times per second. Uncapped ports keep that rate and draw more frames in
  between, placing actors, sectors and the view between the last two tics. The simulation is untouched, so
  demos and savegames still work.
- Input is read every frame, and mouse motion is accumulated into the next tic's command (ticcmd), so turning
  shows at once.
- Demos record ticcmds, one small struct of logical actions per tic.
- Criticism: view interpolation can make the mouse feel laggy.

### Quake 3

- The file system searches an ordered list of paths: home path, base path, CD path, each with the game's
  `.pk3` archives and its directory. Archives are searched from the highest number down and take precedence
  over loose files. All writes go to the home path (`~/.q3a` on Unix).
- Cvar flags: `CVAR_ARCHIVE` (saved to the config), `CVAR_ROM` (read-only), `CVAR_INIT` (only from the command
  line), `CVAR_CHEAT`, plus network flags.
- A recent fork fixed a security bug: game modules set cvars through a forced path that skipped `CVAR_INIT`,
  so downloaded modules could change file system path cvars.
- Game modules talk to the engine through versioned import and export tables (`dllEntry`, `vmMain`). This is
  from knowledge of the source, not from the sources below.

### Kex

- Its author describes Kex as "not essentially a game engine, but rather a very generalised framework": only
  platform services such as input, rendering and audio, with the game developed on top.
- Nightdive: "if we have the code for an old game, we can basically plug it into KEX and let the engine
  handle everything else behind the scenes."
- It grew out of the Doom64 EX renderer and is used on all Nightdive projects. Some engines, such as Unreal
  Engine 1, can't be wrapped this way.
- The local reference repository is PowerslaveEX (Kex 3); unlike our plan, it rewrote the game logic and
  formats.

## Checked against Jones3D

### Fixed simulation step with interpolated rendering

- **Jones3D:** mostly variable step. `sithTime_Advance()` measures the frame time, clamped to 1 to 200 ms, and
  the world advances by it once per drawn frame. Only player physics runs in fixed slices: 50 per second in
  retail, 150 by default with the QOL changes (`engine.physics.fixedTimestep`). See
  [Frame-Loop-And-Simulation.md](../arc42/06-runtime/Frame-Loop-And-Simulation.md).
- **Verdict:** not a presentation-only change as in Doom. A fixed world step changes the simulation, so it
  needs an option, off by default, with the retail trace unchanged. Not part of M5; an M9 investigation of
  which systems depend on frame time.
- **Recorded:** as a risk in [chapter 11](../arc42/11-risks-and-technical-debt.md).

### Input as a per-step command record

- **Jones3D:** `stdControl_ReadControls()` samples devices once per frame; `sithControl` pulls key and axis
  values and resolves bindings itself. The headless runner's `--input` script already works per frame at the
  key level.
- `stdControl_ReadKeyAsAxis` scales a held key by the frame's read time, and the step is variable. Input alone
  doesn't reproduce a run.
- **Verdict:** fits at a lower level than Doom's ticcmd, which would change `sithControl`. Record per frame:
  the frame time and the raw key and axis state, at the `stdControl` boundary.
- **Decided:** [ADR-0018](../arc42/09-decisions/ADR-0018-replays-record-frame-time-and-raw-input.md).

### Layered file system

- **Jones3D:** `JonesFile_OpenFile` already searches install directory, working directory, CD and `Resource`,
  each first for a loose file and then in its GOB archives. Unlike Quake 3, loose files win over archives.
- `Jones.cfg` and savegames are written relative to the working directory, next to the game data.
- **Verdict:** the search order stays in game code; mod folders later are one more entry there. Babylon's
  Files module adds a separate, writable user directory.
- **Decided:** [ADR-0019](../arc42/09-decisions/ADR-0019-user-directory-for-writes.md).

### Cvar flags

- **Jones3D:** settings live in `Jones.cfg` ([Jones.cfg.md](../Jones.cfg.md)). COG scripts have no access to
  configuration, so the Quake 3 security bug can't happen today.
- **Verdict:** fits. Flags for saved, read-only and startup-only cvars, plus a simulation flag so that
  simulation cvars are recorded in traces automatically
  ([ADR-0013](../arc42/09-decisions/ADR-0013-cvars-in-foundation.md)). Protecting path cvars becomes a
  requirement once a console or mods can set cvars (M9).
- **Decided:** [ADR-0020](../arc42/09-decisions/ADR-0020-cvar-flags.md).

### Game data detection

- **Jones3D:** nothing checks which data version is installed.
- **Verdict:** fits. Checksum the GOBs at startup, name the version, warn on unknown data. For M8.
- **Decided:** [ADR-0021](../arc42/09-decisions/ADR-0021-detect-game-data.md).

### Already covered

- One context holding all services with Null defaults (ScummVM's `OSystem`, `Ship::Context`): Babylon's
  composition root ([ADR-0016](../arc42/09-decisions/ADR-0016-virtual-interfaces-chosen-at-startup.md)).
  Refinement: the C adapter edge holds one context pointer, not one global per service.
- Unmodified game code, a port layer and a generic framework (libultraship, Kex): the adapter
  ([ADR-0007](../arc42/09-decisions/ADR-0007-adapter-between-game-and-babylon.md)).

### Not adopted

- Versioned import and export tables at the game boundary (Quake 3): they pay off with several games or
  third-party game modules, and there are neither.

## Sources

- ScummVM: [common/system.h](https://github.com/scummvm/scummvm/blob/master/common/system.h),
  [metaengine.h](https://doxygen.scummvm.org/de/d92/metaengine_8h_source.html),
  [overview (DeepWiki)](https://deepwiki.com/alexbevi/scummvm)
- libultraship: [BattleShip port](https://github.com/JRickey/BattleShip),
  [Shipwright overview (DeepWiki)](https://deepwiki.com/HarbourMasters/Shipwright),
  [Ship of Harkinian startup](https://abdess.github.io/retrobios/emulators/soh/),
  [Ship of Harkinian (Wikipedia)](https://en.wikipedia.org/wiki/Ship_of_Harkinian)
- Doom: [Uncapped framerate (Doom Wiki)](https://doomwiki.org/wiki/Uncapped_framerate),
  [Doomworld: techniques for uncapped framerates](https://www.doomworld.com/vb/source-ports/65716-techniques-for-uncapped-framerates),
  [Crispy Doom Crispness menu](https://github.com/fabiangreffrath/crispy-doom/wiki/Crispness-Menu),
  [astrosteveo/DOOM PR #1](https://github.com/astrosteveo/DOOM/pull/1)
- Quake 3: [files.c (id)](https://github.com/id-Software/Quake-III-Arena/blob/master/code/qcommon/files.c),
  [files.c (ioq3)](https://github.com/ioquake/ioq3/blob/351e8f84d25a2f07f51a7ca5bf6c4d4a345bbc7b/code/qcommon/files.c),
  [Quake3e architecture (DeepWiki)](https://deepwiki.com/ec-/Quake3e/2-core-engine-architecture),
  [jm2/Quake-III-Arena PR #407](https://github.com/jm2/Quake-III-Arena/pull/407)
- Kex: [Nightdive interview (Microsoft)](https://developer.microsoft.com/en-us/games/articles/2025/07/art-of-the-remaster-nightdive-studios-video-game-preservation/),
  [Kex Engine (Blood Wiki)](https://blood-wiki.org/index.php/Kex_Engine),
  [PowerslaveEX](https://github.com/svkaiser/PowerslaveEX)

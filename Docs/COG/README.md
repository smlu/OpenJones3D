# COG Scripting Reference

This folder documents the COG scripting system used by OpenJones3D.

The notes here are based on the current implementation in:

- [`Libs/sith/Cog/`](../../Libs/sith/Cog/)
- [`Libs/sith/World/sithVoice.c`](../../Libs/sith/World/sithVoice.c)
- [`Jones3D/Play/jonesCog.c`](../../Jones3D/Play/jonesCog.c)

Older public Jones3D documentation was useful as a cross-check, but the current codebase is the source of truth for the behavior described here.

The host-function pages list each registered verb together with a PascalCase script-facing signature. `->` indicates the value pushed back to the script VM; verbs without `->` do not return a value.

## In This Folder

- [Language](Language.md): script layout, symbol types, parser rules, and execution model.
- [Messages](Messages.md): message ids, sender/source semantics, and message-specific caveats.
- [Flags](Flags.md): central reference for bitmasks and flag enums used by COG verbs.
- [Types and modes](Types-And-Modes.md): central reference for named non-flag value sets used by COG verbs.
- [System host functions](Functions-System.md): core verbs from `sithCogFunction.c`.
- [Thing host functions](Functions-Thing.md): the main gameplay/object verb catalog.
- [AI host functions](Functions-AI.md): AI control, sight, goals, and waypoint verbs.
- [Player host functions](Functions-Player.md): player inventory, local-player state, and MP helpers.
- [Sector host functions](Functions-Sector.md): sector lighting, flags, thrust, and topology helpers.
- [Surface host functions](Functions-Surface.md): surface material, flags, scrolling, and light helpers.
- [Sound host functions](Functions-Sound.md): sound playback, ambient sound, and sound-class verbs.
- [Voice host functions](Functions-Voice.md): subtitle/lip-sync voice helpers from `sithVoice`.
- [Jones host functions](Functions-Jones.md): Indiana Jones specific verbs from `JonesCog`.

## Quick Facts

- COG scripts are loaded from text, parsed into a small bytecode VM, and executed per-cog instance.
- The text parser recognizes `flags`, `symbols`, `code`, and `end` sections.
- The current global symbol table registers:
  - 47 message ids
  - the extra alias `activated` for `activate`
  - `global0` through `global15`
  - 577 host-function names
- COG source is effectively case-insensitive in the current implementation: the config reader lowercases script lines and the lexer lowercases identifiers before lookup.
- COG function calls are written in normal source order, but the runtime pops arguments from right to left.
- Both current-world cogs and static-world cogs participate in broadcast processing.
- Parsed symbol-table layout is savegame-sensitive. Savegame COG state restores values positionally, so global symbol-table differences can change whether an identifier becomes a local placeholder and can make old COG state deserialize into the wrong symbols.

## Registered Host-Function Groups

| Group | Verbs | Source |
| --- | ---: | --- |
| System | 142 | [`Libs/sith/Cog/sithCogFunction.c`](../../Libs/sith/Cog/sithCogFunction.c) |
| Thing | 236 | [`Libs/sith/Cog/sithCogFunctionThing.c`](../../Libs/sith/Cog/sithCogFunctionThing.c) |
| AI | 59 | [`Libs/sith/Cog/sithCogFunctionAI.c`](../../Libs/sith/Cog/sithCogFunctionAI.c) |
| Player | 39 | [`Libs/sith/Cog/sithCogFunctionPlayer.c`](../../Libs/sith/Cog/sithCogFunctionPlayer.c) |
| Sector | 31 | [`Libs/sith/Cog/sithCogFunctionSector.c`](../../Libs/sith/Cog/sithCogFunctionSector.c) |
| Surface | 40 | [`Libs/sith/Cog/sithCogFunctionSurface.c`](../../Libs/sith/Cog/sithCogFunctionSurface.c) |
| Sound | 16 | [`Libs/sith/Cog/sithCogFunctionSound.c`](../../Libs/sith/Cog/sithCogFunctionSound.c) |
| Voice | 5 | [`Libs/sith/World/sithVoice.c`](../../Libs/sith/World/sithVoice.c) |
| Jones | 9 | [`Jones3D/Play/jonesCog.c`](../../Jones3D/Play/jonesCog.c) |

## Devmode And Editor Caveats

- No currently registered COG host function is hidden behind a devmode-only registration path in the current tree.
- What does change under devmode/editor mode is behavior: some handlers are skipped, some helper verbs report editor state, and [`DebugWaitForKey()`](Functions-System.md#debugwaitforkey) only blocks when [`SITHDEBUG_INEDITOR`](Flags.md#debug-mode-flags) is set.
- [`InEditor()`](Functions-System.md#ineditor) mirrors the [`SITHDEBUG_INEDITOR`](Flags.md#debug-mode-flags) debug-mode flag.
- [`DebugWaitForKey()`](Functions-System.md#debugwaitforkey) is the only registered host function in the current tree that explicitly returns immediately when [`SITHDEBUG_INEDITOR`](Flags.md#debug-mode-flags) is not set.
- The older debug-mode gates around [`SelectWeapon()`](Functions-System.md#selectweapon), [`SelectWeaponWait()`](Functions-System.md#selectweaponwait), [`DeselectWeaponWait()`](Functions-System.md#deselectweaponwait), and [`SetWeaponModel()`](Functions-System.md#setweaponmodel) were removed in the current code, so those verbs are not devmode-only anymore.
- Runtime `created` and `initialized` sends triggered by thing creation/initialization are skipped when [`SITHDEBUG_INEDITOR`](Flags.md#debug-mode-flags) is active. The level-open startup pass still walks existing thing cogs and sends both messages.
- Master-cog `user0` notifications triggered by savegame restore are skipped while [`SITHDEBUG_INEDITOR`](Flags.md#debug-mode-flags) is active.

## Recommended Reading Order

1. Start with [Language](Language.md).
2. Read [Messages](Messages.md) next, especially if you are writing gameplay or puzzle logic.
3. Use the host-function pages as a grouped reference while reading or writing scripts.

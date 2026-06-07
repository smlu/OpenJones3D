# Scripting, AI, And Audio

This page documents three subsystems that interact heavily with the simulation but are easier to understand separately: the COG VM, AI control/awareness, and the audio stack.

Primary source files:

- [`Libs/sith/Cog/sithCog.c`](../../Libs/sith/Cog/sithCog.c)
- [`Libs/sith/Cog/sithCogParse.c`](../../Libs/sith/Cog/sithCogParse.c)
- [`Libs/sith/Cog/sithCogExec.c`](../../Libs/sith/Cog/sithCogExec.c)
- [`Libs/sith/Dss/sithDSSCog.c`](../../Libs/sith/Dss/sithDSSCog.c)
- [`Libs/sith/AI/sithAI.c`](../../Libs/sith/AI/sithAI.c)
- [`Libs/sith/AI/sithAIAwareness.c`](../../Libs/sith/AI/sithAIAwareness.c)
- [`Libs/sith/AI/sithAIClass.c`](../../Libs/sith/AI/sithAIClass.c)
- [`Libs/sith/Devices/sithSound.c`](../../Libs/sith/Devices/sithSound.c)
- [`Libs/sith/Devices/sithSoundMixer.c`](../../Libs/sith/Devices/sithSoundMixer.c)
- [`Libs/sound/Sound.c`](../../Libs/sound/Sound.c)
- [`Libs/sith/World/sithVoice.c`](../../Libs/sith/World/sithVoice.c)

## COG Architecture

COG is not bolted on as an external scripting plugin. It is part of the core gameplay runtime.

The runtime is split into several layers:

- script resources
- instantiated cog objects
- symbol tables
- message dispatch/linking
- bytecode execution
- host-function bridge

The detailed language and host-function reference lives in [../COG/README.md](../COG/README.md), but the architectural role of COG is explained here.

## COG Startup And Open

[`sithCog_Startup()`](../../Libs/sith/Cog/sithCog.c#L132) initializes the global COG runtime:

- global symbol table
- script hash table
- timer task integration

When a world opens, [`sithCog_Open()`](../../Libs/sith/Cog/sithCog.c#L179) performs per-world setup:

- resolve symbol references
- bind local and global symbols
- link cogs to things, sectors, and surfaces
- send `LOADING` to each cog

That means there are two separate lifetimes:

- engine-wide COG runtime state
- world-specific cog instance state

## Symbol Tables And Parse-Time Binding

The global COG symbol table is created during `sithCog_Startup()` and filled by the Sith/Jones function and message registration paths. COG scripts then parse against two namespaces:

- the script-local table built from the `symbols` section and any auto-created identifiers
- the global table containing message names, `global0` through `global15`, and host functions

If a code identifier is missing from both tables, the parser creates a new local float symbol with value `0.0`. If the name exists globally, the script references the global symbol instead. This makes the global table part of the parse result, not just a runtime function registry.

After a script is parsed, each `SithCog` instance duplicates that script symbol table. Runtime state such as local variables, message params, suspended execution fields, and heap values then belongs to the instance.

Savegame COG restore depends on that duplicated table having the same shape as it had when the save was written. DSS COG state stores symbol types and values in table order, so a different parse result can corrupt later values even though the bytecode still executes.

## COG Dispatch Model

COGs are invoked mainly by message dispatch rather than by continuous per-frame callbacks.

Messages come from systems such as:

- thing lifecycle
- sector/surface visibility
- timers and pulses
- AI events
- input and player state
- triggers
- explicit script-to-script sends

When a message is delivered, the engine looks up the corresponding handler offset and begins executing the cog's bytecode from that entry point.

## Bytecode VM

[`sithCogExec_Execute()`](../../Libs/sith/Cog/sithCogExec.c#L70) is a bytecode interpreter, not a source interpreter.

The VM supports:

- stack push/pop
- integer and float arithmetic
- vector creation
- comparisons and boolean operations
- jumps and branching
- assignment
- array access
- host-function calls
- returns

Architecturally, this matters because COG execution is cheap to schedule from gameplay systems. The engine does not need to keep reparsing script text or interpret source syntax in the hot path.

## Why COG Is Deeply Integrated

COG has hooks into nearly every important runtime concept:

- things
- sectors
- surfaces
- sound
- AI
- player state
- timers/events
- camera state
- inventory and HUD helpers

That is why the COG system lives in `sith`, not in `Jones3D`. It is part of the engine runtime contract, not an application-layer feature.

## AI Architecture

The AI system is split across several cooperating subsystems:

- `sithAI`: top-level control blocks, update scheduler, state changes, event emission
- `sithAIMove`: movement helpers and AI movement state
- `sithAIInstinct`: instinct implementations
- `sithAIClass`: data-driven AI class loading
- `sithAIAwareness`: propagation of sensory events across the sector graph

This is important because "AI" is not one monolithic function. The engine separates authored class data, per-instance runtime state, movement, and awareness propagation.

## AI Control Blocks

Each AI-driven thing gets a `SithAIControlBlock` attached through [`sithAI_Create()`](../../Libs/sith/AI/sithAI.c#L138).

The control block stores:

- owner thing
- AI class
- home position/orientation
- instinct states
- movement speed/state
- current mode flags
- next-update timing

If the owner has a puppet state, AI creation also pushes the class's armed mode into the animation/puppet layer.

That is a recurring Jones3D pattern: AI, movement, and animation are separate systems, but they synchronize through shared state transitions.

## AI Update Loop

[`sithAI_Process()`](../../Libs/sith/AI/sithAI.c#L203) sweeps all active AI control blocks each frame.

For each valid AI it:

1. skips disabled/dying/destroyed/blocked/sleeping cases
2. processes pending actor state changes
3. updates actor weapon state
4. runs instinct updates when the scheduled time arrives and the AI is not currently in a move state

So the AI scheduler is relatively simple. The interesting behavior lives inside instincts and event reactions.

## Instinct System

Instincts are registered in a global registry and then attached to AI classes. Each instinct has:

- update modes
- block modes
- trigger events
- next-update time
- implementation callback

[`sithAI_InstinctUpdate()`](../../Libs/sith/AI/sithAI.c#L257) loops over active instincts whose update time has elapsed and calls their callbacks. If an instinct changes the AI mode, the scheduler can restart the scan so the new mode takes effect immediately.

Architecturally, this is a rule-driven AI model rather than a behavior tree or GOAP system. AI class data selects and parameterizes instincts; the runtime scheduler repeatedly evaluates them.

## AI Awareness Propagation

[`sithAIAwareness.c`](../../Libs/sith/AI/sithAIAwareness.c) is especially interesting because it is built on the same sector/adjoin graph used by rendering and collision, but for a completely different purpose.

The subsystem:

1. queues transmitting awareness events
2. periodically processes them through an event task
3. recursively propagates them from sector to sector across adjoins
4. attenuates the remaining signal level with distance
5. records the strongest transmission reaching each sector for the current process id
6. emits AI sound events to control blocks whose sectors were reached

The key idea is that awareness is not evaluated with raw Euclidean distance alone. It travels through the level topology, which makes "hearing" follow the actual connected sector graph.

## Audio Stack

Audio is layered in three tiers:

1. `sound/`

   Low-level sound engine: banks, handles, channels, fades, backend I/O, device-facing playback.

2. `sithSoundMixer`

   Gameplay-aware mixer: listener updates, thing-position playback, ambient-sector logic, attenuation and falloff behavior.

3. `sithSound` / `sithSoundClass` / `sithVoice`

   Gameplay-facing asset loading and semantic playback.

This is a strong architectural split. The lowest layer does not know about sectors, things, or players. The middle layer maps gameplay context into audio state. The highest layer exposes engine concepts such as sound classes, world sounds, and voice playback.

## Low-Level Sound Layer

[`Sound.c`](../../Libs/sound/Sound.c) maintains:

- global sound state machine
- static and normal banks
- sound handles and channel handles
- active fades
- cached loaded sounds
- device/backend state

The backend underneath that layer depends on the build:

- [`DriverDX6.c`](../../Libs/sound/DriverDX6.c)
- [`DriverDX9.c`](../../Libs/sound/DriverDX9.c)

The sound layer therefore mirrors the renderer's design: upper engine code talks to a stable interface while the backend changes underneath.

## Gameplay-Aware Sound Mixer

[`sithSoundMixer.c`](../../Libs/sith/Devices/sithSoundMixer.c) is the bridge from gameplay world state to the low-level sound engine.

Its responsibilities include:

- playing a sound on a thing
- playing a sound at a world position
- updating listener position and orientation from the current camera
- fading sector ambient audio as the player/camera changes sectors
- choosing software-mixer attenuation behavior

This is why gameplay code normally does not call the low-level `Sound_*` APIs directly. It uses the mixer or sound-class helpers so playback remains tied to the world and listener state.

## Sound Classes

`sithSoundClass` groups sound semantics by mode. Instead of each gameplay feature hardcoding exact file names, things can refer to a sound class and ask for a mode such as hurt, create, or impact behavior.

That reduces coupling between gameplay logic and exact sound asset selection.

## Voice Layer

[`sithVoice.c`](../../Libs/sith/World/sithVoice.c) adds voice-specific behavior on top of the general sound system:

- voice playback state
- subtitle/voice draw integration
- lip-sync updates for actors/players

Voice is therefore not a separate audio engine. It is a specialized gameplay subsystem layered on the same lower sound foundations.

## Important QOL Audio Changes

The architecture stays the same, but OpenJones3D extends several defaults:

- software-mixer falloff defaults to logarithmic under `QOL`
- maximum simultaneous sound channels increase from the legacy budget to a much larger one
- modern backend fixes exist in both DX6 and DX9 paths

These changes are documented in more detail in [QOL-And-Compatibility.md](QOL-And-Compatibility.md), but they do not replace the original layered audio design.

For the dedicated audio deep dive, including banks, channels, distant-loop restart, ambient-sector crossfading, sound classes, lip sync, and subtitles, see [Sound-System.md](Sound-System.md).

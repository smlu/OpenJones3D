# Frame Loop And Simulation

This page documents the per-frame execution order once a world is open, with special attention to timing, events, thing updates, physics, collision, and how the gameplay runtime is layered.

Primary source files:

- [`Jones3D/Main/JonesMain.c`](../../Jones3D/Main/JonesMain.c)
- [`Libs/sith/Main/sithMain.c`](../../Libs/sith/Main/sithMain.c)
- [`Libs/sith/Gameplay/sithTime.c`](../../Libs/sith/Gameplay/sithTime.c)
- [`Libs/sith/Gameplay/sithEvent.c`](../../Libs/sith/Gameplay/sithEvent.c)
- [`Libs/sith/World/sithThing.c`](../../Libs/sith/World/sithThing.c)
- [`Libs/sith/Engine/sithPhysics.c`](../../Libs/sith/Engine/sithPhysics.c)
- [`Libs/sith/Engine/sithCollision.c`](../../Libs/sith/Engine/sithCollision.c)

## High-Level Frame Shape

The outer gameplay callback is [`JonesMain_ProcessGame()`](../../Jones3D/Main/JonesMain.c#L1214). At a high level it performs:

1. menu/input toggles at the application layer
2. gameplay update through [`sithUpdate()`](../../Libs/sith/Main/sithMain.c#L543)
3. render-scene setup and world draw through [`sithDrawScene()`](../../Libs/sith/Main/sithMain.c#L596)
4. HUD/overlay submission
5. scene end and presentation

The engine therefore divides the frame into two main halves:

- update/simulation
- draw/submission

The `sith` runtime owns both halves, but `JonesMain` still wraps them because it also manages menus, display resets, load/save UI, and window/device presentation.

## Core Update Order

The normal non-sync update path in [`sithUpdate()`](../../Libs/sith/Main/sithMain.c#L543) runs in this order:

1. [`sithSoundMixer_Update()`](../../Libs/sith/Devices/sithSoundMixer.c#L259)
2. if paused: limited control update only
3. otherwise:
   - increment frame number
   - advance render tick
   - [`sithTime_Advance()`](../../Libs/sith/Gameplay/sithTime.c#L34)
   - [`sithEvent_Process()`](../../Libs/sith/Gameplay/sithEvent.c#L135)
   - process pending input/network messages
   - [`sithAI_Process()`](../../Libs/sith/AI/sithAI.c#L203)
   - [`sithAnimate_Update()`](../../Libs/sith/Engine/sithAnimate.c#L934)
   - [`sithControl_Update()`](../../Libs/sith/Devices/sithControl.c#L334)
   - [`sithThing_Update()`](../../Libs/sith/World/sithThing.c#L579)
   - [`sithCog_ProcessCogs()`](../../Libs/sith/Cog/sithCog.c#L2024)
   - [`sithMulti_Update()`](../../Libs/sith/Dss/sithMulti.c#L174)

That order is important:

- time advances before event processing
- events run before AI/control/thing simulation
- thing simulation runs before queued COG resumes are processed
- rendering does not happen until the entire update pass is complete

For the detailed path from raw device reads to gameplay actions, axis values, idle detection, and player/controller consumption, see [Control-And-Input.md](Control-And-Input.md).

## Timing

[`sithTime_Advance()`](../../Libs/sith/Gameplay/sithTime.c#L34) computes:

- frame time in milliseconds
- frame time in seconds (`frameTimeFlex`)
- instantaneous FPS
- cumulative game time in milliseconds and seconds

The frame time is clamped to `1..200` ms in the normal advance path. That prevents excessively large frame deltas from causing catastrophic one-frame simulation jumps after stalls.

OpenJones3D preserves the original timing interface, but under `J3D_QOL_IMPROVEMENTS` the underlying timer comes from `QueryPerformanceCounter()` through [`stdPlatform_GetTimeMsec()`](../../Libs/std/General/stdPlatform.c#L95) rather than `timeGetTime()`. Architecturally the higher layers do not care which timer provides the value; they continue to use the host service and the `sithTime` globals.

## Event Scheduler

[`sithEvent.c`](../../Libs/sith/Gameplay/sithEvent.c) provides a small event/task scheduler used by multiple gameplay systems.

It has two execution styles:

- interval tasks
- queued one-shot events

The scheduler stores:

- a fixed pool of `SithEvent` records
- a free-list of event-buffer indices
- a small task table
- a sorted linked list of queued events

During [`sithEvent_Process()`](../../Libs/sith/Gameplay/sithEvent.c#L135):

1. interval tasks whose frequency has elapsed are executed
2. queued events whose fire time is earlier than the current game time are popped and dispatched

This subsystem is small, but architecturally important because it is how long-running gameplay systems schedule recurring or deferred work without each one needing its own timer queue.

Examples include AI awareness updates and COG timer-driven behavior.

For the deeper scheduler and dispatch architecture, including COG timer wakeups, message delivery, and DSS/network replication, see [Event-And-Message-Systems.md](Event-And-Message-Systems.md).

## Thing Update Is The Main Gameplay Sweep

[`sithThing_Update()`](../../Libs/sith/World/sithThing.c#L579) is the central per-frame object update loop. It iterates over the active thing array in the current world up to `lastThingIdx`.

For each thing it performs, in order:

1. skip free slots
2. remove destroyed things
3. count down life timers and destroy expired things
4. skip disabled things
5. update per-thing COG timers/pulses
6. update control-type-specific behavior
7. update type-specific behavior
8. run optional special callback hook
9. update movement mode
10. apply actual movement through [`sithThing_UpdateMove()`](../../Libs/sith/World/sithThing.c#L717)
11. update puppet/animation state

That means "thing update" is not one narrow system. It is the point where gameplay behavior, movement, animation, AI/path control, sprite logic, weapon logic, and player logic all converge.

## Control-Type Versus Thing-Type

One of the more subtle parts of the architecture is that things have both:

- a thing type
- a control type

Those two axes serve different purposes:

- thing type describes what the object fundamentally is
- control type describes who is driving its behavior

For example:

- AI-controlled actors use the AI control path
- explosion things use explosion-control updates
- particle things use particle-control updates
- weapons, players, actors, and sprites still have their own thing-type-specific update functions afterward

This split lets the engine compose behavior instead of forcing all behavior into a single type enum.

## Movement Update

After control/type updates, the thing system moves the object according to its current movement model:

- `SITH_MT_NONE`
- `SITH_MT_PHYSICS`
- `SITH_MT_PATH`

The update path is:

1. movement subsystem computes velocity or path delta
2. [`sithThing_UpdateMove()`](../../Libs/sith/World/sithThing.c#L717) converts that into spatial motion
3. collision and sector transitions are handled during that movement
4. special attachment/floor-finding updates are applied as needed

The actual move step is therefore split across:

- physics/path update code that computes intended motion
- thing/collision code that applies it to the world

## Physics Branches

[`sithPhysics_UpdateThing()`](../../Libs/sith/Engine/sithPhysics.c#L568) first decides which specialized physics path to run:

- climbing
- jeep
- minecar
- raft
- attached-to-surface or attached-to-thing
- underwater/aetherium
- detached player
- generic detached physics thing

This branching is important architecturally. Jones3D does not have one universal rigid-body solver. Instead it has a family of gameplay-tuned physics handlers chosen from thing flags, attachment state, sector state, and movement state.

That is why vehicle behavior, underwater movement, and climbing all live inside the physics subsystem but still have very different execution paths.

## Detached Physics

For non-player detached objects, [`sithPhysics_UpdateDetachedThingPhysics()`](../../Libs/sith/Engine/sithPhysics.c#L878) applies:

- angular drag and angular thrust
- optional rotation application
- periodic orientation normalization
- air drag
- local thrust transformed into world space
- optional sector thrust
- optional gravity
- final velocity-to-delta integration

The result is written into `deltaVelocity`, which is later consumed by the thing movement path.

This is a good example of the engine's separation of concerns:

- physics computes intended delta
- movement/collision decides how far that delta can actually be applied

## Fixed-Step Player Physics

The player detached-physics path, [`sithPhysics_UpdateDetachedPlayerPhysics()`](../../Libs/sith/Engine/sithPhysics.c#L1005), is special.

Instead of integrating once at the frame delta, it:

1. adds the current frame delta to a rollover accumulator
2. computes how many fixed slices fit in the accumulated time
3. processes those slices one by one
4. stores the remainder back into `physicsRolloverFrames`

Originally this path was effectively tuned around a 50 Hz step. OpenJones3D keeps the fixed-step model, because player movement and gameplay response were authored around it, but under `J3D_QOL_IMPROVEMENTS` the default fixed-step rate becomes configurable and defaults to 150 Hz.

The architectural reason for keeping a fixed-step path is stability and gameplay determinism. The player controller is much more sensitive to frame-rate-dependent drift than generic detached objects.

## Floor Finding And Attachment

[`sithPhysics_FindFloor()`](../../Libs/sith/Engine/sithPhysics.c#L291) is one of the most important bridge functions between physics, collision, and world topology.

Its job is to determine whether a physics thing should attach to:

- a world surface
- a water surface
- a stand-on thing face
- nothing, in which case it detaches and falls

The search uses [`sithCollision_SearchForCollisions()`](../../Libs/sith/Engine/sithCollision.c#L1073) with different flags depending on:

- player or non-player
- raft or non-raft
- wall-stick/floor-stick state
- whether floor-like surfaces are required

This is a strong example of the Jones3D architecture being gameplay-driven rather than purely physically simulated. "Standing," "climbing," "sticking," and "floating" are explicit gameplay states built on top of collision queries.

## Collision System

[`sithCollision.c`](../../Libs/sith/Engine/sithCollision.c) is a graph-search and dispatch system, not just a math helper library.

It contains:

- searched-sector tracking
- collision stacks
- type-vs-type collision handler tables
- type-vs-surface handler tables
- LOS and camera collision queries
- movement collision scanning across adjoining sectors

At startup, the collision system registers pairwise handlers such as:

- actor vs actor
- actor vs player
- player vs player
- weapon vs actor/player/debris/cog
- item vs player

This means collision is architected as:

1. collect possible hits
2. sort/pop the closest hit
3. dispatch the hit to a handler chosen by thing types and hit type

So collision response is data-driven at the handler-table level, even though the search itself is hardcoded.

For the full movement-trace, query, and response pipeline, including adjoin crossing, hit sorting, line-of-sight tests, and handler dispatch, see [Collision-System.md](Collision-System.md).

## Messages, Scripts, And Simulation

The simulation pass does not execute every gameplay consequence immediately inline. It often records or schedules work through:

- COG timers and message delivery
- event tasks
- AI events
- sound/awareness events

That is why [`sithThing_Update()`](../../Libs/sith/World/sithThing.c#L579) is followed by [`sithCog_ProcessCogs()`](../../Libs/sith/Cog/sithCog.c#L2024) later in the frame. Scripts frequently react to simulation outcomes, but the engine still wants a stable main update order.

The dedicated messaging and deferred-work mechanisms behind that handoff are documented in [Event-And-Message-Systems.md](Event-And-Message-Systems.md).

## Rendering Begins After Simulation Completes

Only after [`sithUpdate()`](../../Libs/sith/Main/sithMain.c#L543) finishes does [`sithDrawScene()`](../../Libs/sith/Main/sithMain.c#L596) run. That function:

1. advances render tick
2. updates the current camera
3. renders the scene
4. optionally draws debug overlays

The engine therefore uses a clear update-then-render model. The renderer reads the finished world state for the frame; it does not interleave gameplay updates while drawing.

## Architectural Consequences

The frame model leads to a few recurring design patterns throughout the codebase:

- fixed global arrays and per-frame sweeps instead of fine-grained job systems
- subsystem-specific state machines instead of one general entity-component system
- thing indices and direct pointers instead of opaque handles everywhere
- graph traversal over sectors/adjoins for many systems, not just rendering
- gameplay-tuned physics branches instead of one universal solver

Those characteristics are inherited from the original engine, but OpenJones3D keeps them deliberately because many higher-level systems and scripts implicitly rely on them.

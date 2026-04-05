# Architecture Quick Reference

This page is the short version of the engine architecture. It is meant to give you the mental model first, before you dive into the subsystem-specific pages.

## One-Minute Summary

OpenJones3D keeps the original Jones3D runtime split into layers:

```text
Jones3D   game/application layer
  |
sith      gameplay runtime
  |
rdroid    renderer-facing primitives and caches
  |
std       platform, display, input, files, config
sound     low-level audio engine
  |
wkernel   outer Win32 process and window loop
```

The most important idea is that the engine is both:

- object-based, through `SithThing`
- topology-based, through sectors connected by adjoins

Things drive gameplay behavior. Sectors and adjoins drive world traversal.

## The World Model

A loaded level is represented by a [`SithWorld`](../../Libs/sith/World/sithWorld.c#L351) and contains:

- sectors
- surfaces
- adjoins
- things
- materials, models, keyframes, sounds, scripts, and other world resources

In practice, the world is not just geometry. It is the main runtime ownership object for the whole active level.

There are also two world namespaces:

- the static world for shared resources
- the current world for the active gameplay level

## Sectors, Surfaces, And Adjoins

The level is divided into sectors. A sector is a spatial region of the world that owns:

- a set of surfaces
- a linked list of things currently inside it
- links to neighboring sectors through adjoins

Sectors are expected to be convex volumes. That is an important engine assumption, not just a content convention. If a space is non-convex, it must usually be split into multiple sectors connected by adjoins.

A surface is a polygonal face in a sector. Some surfaces have an attached adjoin. That adjoin connects the surface to another sector.

So the level is not treated as one giant undifferentiated mesh. It is a graph:

- sectors are the nodes
- adjoins are the edges

This graph is one of the most important structural ideas in the engine.

## Why The Sector And Adjoin Graph Exists

The sector/adjoin graph is reused by many systems, each for a different reason.

Rendering:

- starts from the camera sector
- walks through visible adjoins
- clips portal surfaces and builds the visible-sector set
- optionally filters with PVS first

Collision:

- starts from a known sector and a trace origin
- searches world and thing hits in the current sector
- crosses adjoins when the query is allowed to pass through them
- uses the resulting hit list for movement, line-of-sight, floor finding, and helper queries

AI awareness:

- propagates sound/awareness signals through connected sectors instead of relying only on raw distance

Other systems:

- camera placement
- water-sector transitions
- support/floor checks
- off-screen thing/light collection for rendering

So sectors and adjoins are not just a render optimization. They are the engine's general-purpose world traversal structure.

The convex-sector rule is part of why this works well. Rendering, collision, floor finding, and sector-membership tests all rely on the idea that a point inside a sector can be classified consistently against that sector's bounding surfaces.

## The Two Main Runtime Axes

If you want the shortest useful mental model, think of the runtime like this:

1. `SithThing` answers "what is this gameplay object and what is it doing?"
2. sectors/adjoins answer "where is it, what region is it in, and what nearby regions are reachable?"

Most gameplay code ends up combining those two axes.

Examples:

- a player thing moves through sectors
- an actor thing hears an awareness event that propagated through adjoins
- a weapon thing traces collisions across sector boundaries
- a light attached to a thing affects geometry in visible or nearby sectors

## Frame Shape

The frame is broadly:

1. update simulation
2. render the finished world state

During update, the engine advances:

- time
- events
- AI
- animation
- controls
- things
- COG script wakeups

During rendering, the engine:

1. updates the camera
2. builds visible sectors
3. expands nearby off-screen thing/light influence
4. builds dynamic lighting
5. renders sectors
6. renders things
7. renders alpha adjoins and other deferred pieces

The renderer reads finished simulation state. It does not interleave gameplay updates while drawing.

## Key Runtime Terms

`SithWorld`
: The root runtime container for a loaded level or static resource set.

`Sector`
: A region of the level used for visibility, collision, thing ownership, and graph traversal.

`Surface`
: A polygon face in a sector. Surfaces can be collidable, script-linked, climbable, water, floor, sky, and more.

`Adjoin`
: A connection from one surface to a neighboring sector. It defines whether traversal, visibility, or other cross-sector behavior is allowed.

`Thing`
: The core gameplay object type. Players, actors, weapons, items, particles, cameras, and helper objects are all things.

`COG`
: The engine's integrated gameplay scripting VM and message system.

`PVS`
: Precomputed visibility set data used to accelerate visible-sector discovery.

`rdroid`
: The renderer-facing layer that owns meshes, materials, cameras, and raster/cache primitives.

`std`
: The platform layer for input, display, file/config helpers, and graphics backend glue.

## What To Read Next

If you want the architecture in the best order:

- [Boot-And-Lifecycle.md](Boot-And-Lifecycle.md)
- [World-And-Resources.md](World-And-Resources.md)
- [Frame-Loop-And-Simulation.md](Frame-Loop-And-Simulation.md)
- [Rendering-And-Visibility.md](Rendering-And-Visibility.md)
- [Collision-System.md](Collision-System.md)

If you want the shortest path to the sector/adjoin model specifically:

- [World-And-Resources.md](World-And-Resources.md)
- [Rendering-And-Visibility.md](Rendering-And-Visibility.md)
- [Collision-System.md](Collision-System.md)

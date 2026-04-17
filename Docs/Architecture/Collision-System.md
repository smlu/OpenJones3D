# Collision System

This page documents how Jones3D turns intended motion into validated world motion. In this engine, collision is not just a geometric helper. It is a combined system for:

- walking the sector/adjoin graph
- gathering thing and surface hits
- choosing the nearest blocking or crossing result
- dispatching gameplay-specific response handlers
- updating sector membership and script messages

Primary source files:

- [`Libs/sith/Engine/sithCollision.c`](../../Libs/sith/Engine/sithCollision.c)
- [`Libs/sith/Engine/sithIntersect.c`](../../Libs/sith/Engine/sithIntersect.c)
- [`Libs/sith/Engine/sithPhysics.c`](../../Libs/sith/Engine/sithPhysics.c)
- [`Libs/sith/World/sithThing.c`](../../Libs/sith/World/sithThing.c)

## Role In The Frame

The collision system sits between the simulation subsystems that decide where a thing wants to go and the world state that decides where it is allowed to go.

The normal path is:

1. [`sithPhysics_UpdateThing()`](../../Libs/sith/Engine/sithPhysics.c#L568) or path movement computes intended motion.
2. [`sithThing_UpdateMove()`](../../Libs/sith/World/sithThing.c#L717) asks the collision system to apply that motion.
3. [`sithCollision_MoveThing()`](../../Libs/sith/Engine/sithCollision.c#L601) traces through sectors, resolves hits, and updates the thing's final position, sector, and movement remainder.
4. Response handlers may apply bounce, slide, damage, touched/crossed messages, or force movement reset.

That design keeps the gameplay-facing physics/controller code simple. It only needs to produce a desired delta. Collision owns the hard part of applying that delta against world topology and other things.

## Fixed Runtime Data

The collision core uses fixed-size working storage declared near the top of [`sithCollision.c`](../../Libs/sith/Engine/sithCollision.c#L28):

- a four-level nested collision stack via `SITHCOLLISION_STACKSIZE`
- a pairwise thing-vs-thing handler table
- a per-thing-type surface handler table
- up to `256` searched sectors per stack level
- up to `512` queued collision candidates per stack level

This is an original-engine style design choice. Instead of allocating arbitrary temporary containers every trace, the engine uses bounded scratch storage and fails loudly if a pathological case exceeds those limits.

The practical consequences are:

- nested collision queries are allowed, but only up to four active stack frames
- each trace works against a deterministic scratch area
- stack cleanup is explicit through [`sithCollision_DecreaseStackLevel()`](../../Libs/sith/Engine/sithCollision.c#L1141) and [`sithCollision_ResetCurStack()`](../../Libs/sith/Engine/sithCollision.c#L2367)

## Handler Registration

Startup happens in [`sithCollision_Startup()`](../../Libs/sith/Engine/sithCollision.c#L91). It clears the handler tables and installs concrete responses through:

- [`sithCollision_AddCollisionHandler()`](../../Libs/sith/Engine/sithCollision.c#L140)
- [`sithCollision_AddSurfaceCollisionHandler()`](../../Libs/sith/Engine/sithCollision.c#L162)

Important default registrations include:

- actor vs actor and actor vs player through actor-specific handlers
- player vs player and player vs cog through the generic thing handler
- weapon vs actor/player/debris/cog through weapon handlers
- item vs player through the item pickup handler
- actor and weapon surface hits through specialized surface handlers

Everything else falls back to the generic tables or is rejected before the search ever dispatches a response.

Architecturally, this matters because Jones3D separates hit detection from hit semantics. The search phase only finds candidates. The handler table decides whether the result means bounce, damage, pickup, blocking, or nothing.

## The Main Movement Algorithm

[`sithCollision_MoveThing()`](../../Libs/sith/Engine/sithCollision.c#L601) is the heart of the subsystem.

### 1. Pre-processing And Flag Synthesis

Before tracing, the function derives extra behavior from the thing itself:

- non-collidable things force permissive search flags
- path movers force the path-move search path
- player things add player-specific search behavior
- attached children may be moved first so the parent trace can react to crush or blockage

This is why collision behavior is not driven by one flag field alone. The effective trace mode is a combination of the call-site flags, thing type, move type, and attachment state.

### 2. Player-Specific Edge Logic

When the moving thing is the local player and not a vehicle, [`sithCollision_MoveThing()`](../../Libs/sith/Engine/sithCollision.c#L601) performs an additional ledge/floor check before the general trace.

That logic uses [`sithPlayerActions_CheckFloorAtPos()`](../../Libs/sith/Gameplay/sithPlayerActions.c#L3270) to answer a gameplay question rather than a pure physics question:

- can the player keep walking or running here
- should the player stop
- should a leap-forward state begin

This is a good example of the Jones3D approach: collision supports authored movement rules such as ledges, leaps, climb mounts, and walkable-vs-nonwalkable transitions, rather than acting as a fully generic rigid-body solver.

### 3. Iterative Collision Resolution

The main loop in [`sithCollision_MoveThing()`](../../Libs/sith/Engine/sithCollision.c#L601) resolves up to four successive impacts.

Per iteration it:

1. remembers the previous position and sector
2. calls [`sithCollision_SearchForCollisions()`](../../Libs/sith/Engine/sithCollision.c#L1073)
3. repeatedly pops the nearest candidate through [`sithCollision_PopStack()`](../../Libs/sith/Engine/sithCollision.c#L216)
4. advances the thing to the impact distance
5. dispatches the candidate to a thing, surface, or adjoin-cross handler
6. either stops, or converts the remaining displacement into a new `moveDir` and traces again

The four-iteration cap is deliberate. It limits pathological recursive bounce/slide chains and keeps the per-frame trace cost bounded.

### 4. Out-Of-World Recovery

After motion has been resolved, [`sithCollision_MoveThing()`](../../Libs/sith/Engine/sithCollision.c#L601) validates that the final position still belongs to the current sector with [`sithIntersect_IsSphereInSector()`](../../Libs/sith/Engine/sithIntersect.c#L747).

If not, the engine either:

- destroys a temporary physics thing that fell out of the world
- or pushes the player back to the last known-good position

This recovery path is important because the engine allows adjoin crossing, sector changes, bouncing, attachment offsets, and several gameplay-special movement modes in one function. Final validation is the backstop that keeps a bad trace from leaving a thing permanently outside sector ownership.

## Search Pipeline

[`sithCollision_SearchForCollisions()`](../../Libs/sith/Engine/sithCollision.c#L1073) is the orchestration layer for a single collision query. It does not move a thing by itself. Instead it builds one stack level of `SithCollision` results that the caller then consumes through [`sithCollision_PopStack()`](../../Libs/sith/Engine/sithCollision.c#L216).

### Function Contract

The function signature is:

```C
float sithCollision_SearchForCollisions(
    SithSector* pStartSector,
    SithThing* pThing,
    const rdVector3* startPos,
    const rdVector3* moveNorm,
    float moveDist,
    float radius,
    int searchFlags
);
```

Inputs:

- `pStartSector`
  The sector that should contain `startPos`. The function assumes this is already known and valid.
- `pThing`
  The moving or querying thing when the query should use thing-pair collision rules. This can be `NULL` for pure geometry/world queries.
- `startPos`
  The world-space start point of the query.
- `moveNorm`
  The normalized world-space direction of travel. Unlike the helper wrappers, `sithCollision_SearchForCollisions()` does not normalize it for the caller.
- `moveDist`
  The maximum distance to search along `moveNorm`.
- `radius`
  The swept sphere radius. `0.0f` means a point-like trace.
- `searchFlags`
  A bitmask that changes what counts as collidable, whether adjoins can be crossed, whether floor-only surfaces should be considered, and whether neighboring sectors should also contribute thing hits.

Outputs:

- Return value
  The current trace distance horizon after candidate collection. In practice this usually stays equal to the requested `moveDist`, or gets shortened when a nearer thing hit is allowed to clamp later work.
- Collision stack entries
  The main output is the set of `SithCollision` records pushed onto the current collision stack. Each record exposes:
  - `type`
  - `distance`
  - `pThingCollided`
  - `pSurfaceCollided`
  - `pFaceCollided`
  - `pMeshCollided`
  - `hitNorm`

So the return value is only part of the API contract. The real result set lives on the stack and must be drained through [`sithCollision_PopStack()`](../../Libs/sith/Engine/sithCollision.c#L216).

### Stack Ownership And Lifetime

This function increments the collision stack level internally at the start of the call:

- `stackLevel++`
- [`sithCollision_ResetCurStack()`](../../Libs/sith/Engine/sithCollision.c#L2367)

It does not decrement the stack when it returns.

That means the ownership rule is:

- if you call [`sithCollision_SearchForCollisions()`](../../Libs/sith/Engine/sithCollision.c#L1073) directly, you must eventually call [`sithCollision_DecreaseStackLevel()`](../../Libs/sith/Engine/sithCollision.c#L1141) exactly once
- if you call a helper such as [`sithCollision_FindSectorInRadius()`](../../Libs/sith/Engine/sithCollision.c#L238), [`sithCollision_FindSectorAtThing()`](../../Libs/sith/Engine/sithCollision.c#L282), [`sithCollision_FindWaterSector()`](../../Libs/sith/Engine/sithCollision.c#L325), [`sithCollision_CheckLOS()`](../../Libs/sith/Engine/sithCollision.c#L425), or [`sithCollision_CheckThingLOS()`](../../Libs/sith/Engine/sithCollision.c#L458), that helper already drains or inspects the stack and then decrements it internally

This ownership rule is one of the most important practical details of the subsystem.

### Common Direct-Use Pattern

This is the normal direct-use pattern seen throughout gameplay code such as [`sithPlayerActions_Activate()`](../../Libs/sith/Gameplay/sithPlayerActions.c#L103), [`sithPlayerActions_CheckFloorAtPos()`](../../Libs/sith/Gameplay/sithPlayerActions.c#L1638), [`sithPlayerControls_ProcessClimbMove()`](../../Libs/sith/Gameplay/sithPlayerControls.c#L2800), and many weapon or vehicle helpers:

```C
rdVector3 moveNorm = someNormalizedDirection;
float moveDist = 0.11f;
float radius = 0.04f;

sithCollision_SearchForCollisions(
    pStartSector,
    pThing,
    &startPos,
    &moveNorm,
    moveDist,
    radius,
    searchFlags
);

for (SithCollision* pCollision = sithCollision_PopStack();
     pCollision;
     pCollision = sithCollision_PopStack())
{
    if ((pCollision->type & SITHCOLLISION_WORLD) != 0)
    {
        /* inspect pSurfaceCollided, hitNorm, distance */
    }
    else if ((pCollision->type & SITHCOLLISION_THING) != 0)
    {
        /* inspect pThingCollided, pFaceCollided, pMeshCollided */
    }
}

sithCollision_DecreaseStackLevel();
```

### Common Pattern When The Start Point Is Not In The Thing's Current Sector

Many gameplay callers first move the logical trace origin to a derived world-space point such as:

- the eye point
- a grab test point
- a climb ledge test point
- a projectile fire point

In those cases the usual pattern is:

1. use [`sithCollision_FindSectorInRadius()`](../../Libs/sith/Engine/sithCollision.c#L238) to find the sector containing that derived position
2. only then call [`sithCollision_SearchForCollisions()`](../../Libs/sith/Engine/sithCollision.c#L1073) from that position

That pattern appears repeatedly in [`sithPlayerActions.c`](../../Libs/sith/Gameplay/sithPlayerActions.c), for example in [`sithPlayerActions_Activate()`](../../Libs/sith/Gameplay/sithPlayerActions.c#L103), and in projectile setup code such as [`sithWeapon_FireProjectile()`](../../Libs/sith/World/sithWeapon.c#L624).

```C
rdVector3 startPos = someDerivedWorldPosition;
SithSector* pStartSector =
    sithCollision_FindSectorInRadius(pThing->pInSector, &pThing->pos, &startPos, 0.0f);

if (pStartSector)
{
    rdVector3 moveNorm = someNormalizedDirection;

    sithCollision_SearchForCollisions(
        pStartSector,
        pThing,
        &startPos,
        &moveNorm,
        moveDist,
        radius,
        searchFlags
    );

    for (SithCollision* pCollision = sithCollision_PopStack();
         pCollision;
         pCollision = sithCollision_PopStack())
    {
        /* inspect results */
    }

    sithCollision_DecreaseStackLevel();
}
```

### How The Function Works

At a high level the function:

1. pushes a new stack level
2. clears the working arrays for that level
3. builds the initial candidate list in the starting sector
4. expands through adjoins when a candidate represents an allowed cross
5. optionally inspects adjacent sectors for thing collisions even when those sectors were not part of the direct blocking trace

The last point is easy to miss and matters a lot. Jones3D distinguishes between:

- sectors that must be traversed because the move volume can cross their adjoins
- neighboring sectors that should still contribute thing collisions for broad gameplay correctness

That split is what makes collision work well with wide actors, nearby interactables, and movers close to portals without forcing the whole trace to fully recurse through the entire connected level.

### Building A Sector's Candidate List

[`sithCollision_BuildCollisionList()`](../../Libs/sith/Engine/sithCollision.c#L2235) does the per-sector work:

- first reject already-searched sectors through [`sithCollision_CheckSectorSearched()`](../../Libs/sith/Engine/sithCollision.c#L2385)
- add the sector to the current search frontier through [`sithCollision_AddSearchedSector()`](../../Libs/sith/Engine/sithCollision.c#L2373)
- search thing candidates through [`sithCollision_SearchForThingCollisions()`](../../Libs/sith/Engine/sithCollision.c#L1149) unless the trace flags disabled that branch
- search surface/adjoin candidates through [`sithCollision_SearchForSurfaceCollisions()`](../../Libs/sith/Engine/sithCollision.c#L1208)

Thing search can shorten the remaining maximum move distance before surface testing. That is an optimization and a behavior choice: once a closer solid thing hit is known, later surface tests do not need to consider farther distances unless a specific flag requires it.

### Thing Candidate Collection

[`sithCollision_SearchForThingCollisions()`](../../Libs/sith/Engine/sithCollision.c#L1149) iterates `pFirstThingInSector` and applies several layers of filtering before calling the expensive sphere-vs-thing intersection test.

Important filters include:

- global thing eligibility through [`sithCollision_CanThingCollide()`](../../Libs/sith/Engine/sithCollision.c#L2260)
- pairwise eligibility through [`sithCollision_CanThingCollideWithThing()`](../../Libs/sith/Engine/sithCollision.c#L2270)
- special masks for mountable-only, stand-on-only, or filtered actor/player/weapon cases

[`sithCollision_CanThingCollideWithThing()`](../../Libs/sith/Engine/sithCollision.c#L2270) is especially important. It encodes a large amount of gameplay knowledge:

- no self-collision
- no hit if no registered handler exists
- no collisions against dying things
- no shooter-self-damage for protected weapons
- attachment exemptions
- tail/rope-style exemptions
- jeep/minecar runover special cases

This is one of the places where the collision subsystem becomes deeply game-specific.

Successful thing hits are stored through [`sithCollision_PushThingCollision()`](../../Libs/sith/Engine/sithCollision.c#L2398).

### Surface And Adjoin Candidate Collection

[`sithCollision_SearchForSurfaceCollisions()`](../../Libs/sith/Engine/sithCollision.c#L1208) serves three related jobs:

- test solid world surfaces
- test adjoins for pass-through
- test adjoins for zero-radius touch events

The function first uses the sector collision box when present to cheaply reject sectors that cannot possibly matter. It then walks the sector's surfaces and distinguishes between:

- adjoins the trace is allowed to cross, using [`sithCollision_CanCrossAdjoin()`](../../Libs/sith/Engine/sithCollision.c#L2496)
- solid surfaces that should produce world hits

Adjoin results are recorded as either:

- `ADJOINCROSS`: the movement sphere may pass into the neighboring sector
- `ADJOINTOUCH`: the zero-radius center path touched the adjoin plane closely enough to trigger crossing/touched behavior

World surfaces are stored through [`sithCollision_PushSurfaceCollision()`](../../Libs/sith/Engine/sithCollision.c#L2420).

### Choosing The Next Candidate

[`sithCollision_PopClosest()`](../../Libs/sith/Engine/sithCollision.c#L2443) chooses the nearest not-yet-enumerated candidate. If distances tie, it prefers full face hits over edge/vertex hits where possible.

That small tie-break is easy to overlook, but it improves response stability. Sliding or landing against a face is usually more correct than reacting to a neighboring vertex or edge at the same distance.

## Response Dispatch

Once a candidate has been chosen, [`sithCollision_MoveThing()`](../../Libs/sith/Engine/sithCollision.c#L601) dispatches based on collision type.

### Thing Hits

Thing hits are dispatched through the type-pair handler tables, most commonly into [`sithCollision_ThingCollisionHandler()`](../../Libs/sith/Engine/sithCollision.c#L1428) or specialized actor/weapon handlers registered at startup.

[`sithCollision_ThingCollisionHandler()`](../../Libs/sith/Engine/sithCollision.c#L1428) demonstrates the full range of what a "collision response" means in Jones3D:

- send `TOUCHED` COG messages for cog-linked things
- allow player-vs-item pickup flow
- treat jeep-stop blockers specially
- resolve physics-vs-physics impacts with mass and relative velocity
- hand vehicle cases to [`sithCollision_VehicleCollisionHandler()`](../../Libs/sith/Engine/sithCollision.c#L1688)
- apply crush damage or shove the other body when only one side is physics-driven
- play impact/land sounds and inflict impact damage in the player path

So the pairwise handler layer is effectively the gameplay policy layer for collisions.

### Surface Hits

Surface hits either use a thing-type-specific surface handler or fall back to [`sithCollision_HandleThingHitSurface()`](../../Libs/sith/Engine/sithCollision.c#L1311).

[`sithCollision_HandleThingHitSurface()`](../../Libs/sith/Engine/sithCollision.c#L1311) only handles physics movers. It:

- computes impact speed along the hit normal
- delegates bounce/slide response to [`sithCollision_sub_4AA1A0()`](../../Libs/sith/Engine/sithCollision.c#L1979)
- sends `TOUCHED` to cog-linked surfaces with a short time gate
- contains many player and vehicle landing cases
- chooses landing or scrape sounds from material-like surface flags

The key architectural point is that surface response is still not generic. A wall, a floor, a raft contact, a jeep landing, and a player's fall damage path all share the same tracing infrastructure but branch into authored gameplay logic once the hit is known.

### Adjoin Touch And Cross

Adjoin hits are special because they can mean either:

- transition to a new sector
- or a near-plane touch that should still fire gameplay logic

When [`sithCollision_MoveThing()`](../../Libs/sith/Engine/sithCollision.c#L601) sees `ADJOINTOUCH`, it can emit a `CROSSED` message for cog-linked surfaces and then move the thing into the adjoining sector with [`sithThing_SetSector()`](../../Libs/sith/World/sithThing.c#L1851).

That is another recurring Jones3D pattern: topology changes and script triggers are intertwined.

## Query Helpers

Not every collision call is about moving a thing. The collision subsystem also exposes higher-level query helpers that wrap [`sithCollision_SearchForCollisions()`](../../Libs/sith/Engine/sithCollision.c#L1073), inspect the resulting stack internally, and then clean the stack up before returning.

### `sithCollision_FindSectorInRadius()`

[`sithCollision_FindSectorInRadius()`](../../Libs/sith/Engine/sithCollision.c#L238) is the general "what sector should own this end position" helper.

Inputs:

- `pStartSector`
  The known sector containing `startPos`.
- `startPos`
  The current world position.
- `endPos`
  The desired end position. This vector is both input and output.
- `radius`
  The sphere radius to use while checking whether the path can pass through adjoins.

Returns:

- the sector reached by traversing along the path from `startPos` toward `endPos`
- if a blocking hit is found, it also rewrites `endPos` to the blocking point on the trace

How it works:

1. it first checks whether `endPos` already lies in `pStartSector` with [`sithIntersect_IsSphereInSector()`](../../Libs/sith/Engine/sithIntersect.c#L747)
2. if not, it normalizes the vector from `startPos` to `endPos`
3. it calls [`sithCollision_SearchForCollisions()`](../../Libs/sith/Engine/sithCollision.c#L1073) with the geometry-only `0x01` query style
4. it repeatedly pops results
5. every `SITHCOLLISION_ADJOINTOUCH` advances `pSector` into the adjoining sector
6. the first non-adjoin-touch result clamps `endPos` to the hit distance and stops the walk

When to use it:

- before a trace that starts from a derived world position rather than the thing's exact current position
- when moving cameras, weapons, interaction rays, or helper positions through a portal-connected world
- when you need the correct sector for a world-space point but do not want to move a real thing

This is one of the most common "pre-query" helpers in the engine.

### `sithCollision_FindSectorAtThing()`

[`sithCollision_FindSectorAtThing()`](../../Libs/sith/Engine/sithCollision.c#L282) is a more specialized sector-finding helper used when a specific thing should not immediately invalidate the search result.

Inputs:

- `pThing`
  The thing that should be ignored if the trace happens to hit it directly.
- `pStartSector`
  The known sector containing `startPos`.
- `startPos`
  The current start position.
- `endPos`
  The desired end position. This vector may be clamped backward on blocking hits.
- `radius`
  Swept sphere radius.

Returns:

- the best sector for `endPos` after crossing passable adjoins
- if a blocking collision is found, `endPos` is moved back to just before the hit point by subtracting `0.001f`

How it works:

1. it normalizes the path from `startPos` to `endPos`
2. it runs [`sithCollision_SearchForCollisions()`](../../Libs/sith/Engine/sithCollision.c#L1073) with broad geometry/thing testing
3. it advances through `SITHCOLLISION_ADJOINTOUCH` only when the adjoin is both move-enabled and not marked `SITH_ADJOIN_NOPLAYERMOVE`
4. it ignores a thing hit only if that thing is exactly `pThing`
5. any other blocking thing or surface clamps `endPos` slightly backward and stops the search

One subtle detail matters here: the helper does not pass `pThing` into [`sithCollision_SearchForCollisions()`](../../Libs/sith/Engine/sithCollision.c#L1073) as the search owner. Instead it post-filters the popped results and ignores only a direct hit on that exact `pThing`. That makes it a very good fit for "find the fire-point sector while ignoring the shooter if necessary," which is exactly how [`sithWeapon_FireProjectile()`](../../Libs/sith/World/sithWeapon.c#L624) uses it.

### `sithCollision_FindWaterSector()`

[`sithCollision_FindWaterSector()`](../../Libs/sith/Engine/sithCollision.c#L325) is a portal walk specialized for entering underwater sectors through water adjoins.

Inputs:

- `pStartSector`
- `startPos`
- `endPos`
- `radius`

Returns:

- the sector reached by the query
- if the search hits a water adjoin leading into an underwater sector, `endPos` is clamped to that crossing distance and the search stops there

How it works:

1. it follows the same basic pattern as [`sithCollision_FindSectorInRadius()`](../../Libs/sith/Engine/sithCollision.c#L238)
2. while popping collisions, it looks specifically for:
   - an adjoin hit
   - on a water surface
   - whose adjoining sector is marked `SITH_SECTOR_UNDERWATER`
3. when that case is found, it clamps `endPos` to the water crossing point and returns

This helper exists because "the correct sector at the end point" and "the correct underwater transition point" are not quite the same question.

### `sithCollision_FindWaterSurface()`

[`sithCollision_FindWaterSurface()`](../../Libs/sith/Engine/sithCollision.c#L370) is the surface-oriented companion query.

Inputs:

- `pStartSector`
- `startPos`
- `endPos`
- `radius`

Returns:

- the first water surface encountered on the trace, or `NULL`
- if a water surface is found, `endPos` is moved to that hit distance

Use this when the gameplay logic needs the actual water surface rather than just the sector transition.

### `sithCollision_CheckLOS()`

[`sithCollision_CheckLOS()`](../../Libs/sith/Engine/sithCollision.c#L425) is the geometry-only line-of-sight helper.

Inputs:

- `pStartSector`
  Sector containing `startPos`.
- `startPos`
  Viewer or source point.
- `endPos`
  Target point.
- `radius`
  Optional swept radius. `0.0f` gives a thin ray-style LOS test.

Returns:

- `1` if the path can travel from `startPos` to `endPos`
- `0` if a non-passable adjoin or world collision blocks it

How it works:

1. it builds a normalized direction from `startPos` to `endPos`
2. it calls [`sithCollision_SearchForCollisions()`](../../Libs/sith/Engine/sithCollision.c#L1073) with LOS-style flags
3. while popping results, it accepts only `SITHCOLLISION_ADJOINTOUCH` hits whose adjoins are considered traversable for this LOS mode
4. anything else blocks the LOS and returns `0`

Important usage note:

- this is not a renderer visibility test
- this is not a thing-occlusion test

It is best described as a world-geometry line-of-sight check. That matches how callers such as the aiming code in [`sithPlayerControls.c`](../../Libs/sith/Gameplay/sithPlayerControls.c#L2480) use it after first determining the correct start sector with [`sithCollision_FindSectorInRadius()`](../../Libs/sith/Engine/sithCollision.c#L238).

### `sithCollision_CheckThingLOS()`

[`sithCollision_CheckThingLOS()`](../../Libs/sith/Engine/sithCollision.c#L458) is the thing-aware LOS variant.

Inputs:

- `pViewer`
  The viewing thing.
- `pTarget`
  The target thing.
- `a3`
  A mode switch that changes the adjoin-crossing policy. When nonzero, the helper uses a stricter path that no longer ignores move restrictions on adjoins.

Returns:

- `1` if the target is reached without another thing or blocked adjoin intervening
- `0` otherwise

How it works:

1. it traces from `pViewer->pos` toward `pTarget->pos`
2. if the first relevant thing hit is exactly `pTarget`, LOS succeeds
3. if it hits any other thing first, LOS fails
4. if it encounters a non-passable adjoin/world blockage first, LOS fails

This is the gameplay-facing LOS helper used by systems such as:

- aiming and targeting
- pickup checks
- explosion damage filtering

It is the better choice when other things should be allowed to occlude the target.

### Floor Support

Floor support spans both the collision and physics layers.

#### `sithPhysics_FindFloor()`

[`sithPhysics_FindFloor()`](../../Libs/sith/Engine/sithPhysics.c#L291) is not a pure query helper. It is a state-mutating attachment routine used by the physics subsystem.

Inputs:

- `pThing`
  A physics thing whose floor, stand-on, or water attachment state needs to be refreshed.
- `bNoSurfaceImpactUpdate`
  Passed through to the eventual attach routines to control whether surface-impact updates should be suppressed.

Outputs:

- no direct return value
- mutates the thing by:
  - attaching it to a surface
  - attaching it to a stand-on thing face
  - attaching it to a water surface
  - or detaching it and forcing falling/leap behavior if no support is found

How it works:

1. it validates that the thing exists, is physics-driven, and has a sector
2. it handles special environments first:
   - underwater sectors route player things into [`sithPhysics_FindWaterSurface()`](../../Libs/sith/Engine/sithPhysics.c#L514)
   - aetherium sectors force detachment
3. it derives search flags from the physics flags:
   - player movement adds player adjoin restrictions
   - wall-stick searches along `-uvec`
   - raft searches skip normal thing hits
   - regular floor searches request floor-like surfaces or stand-on things
4. it chooses a downward or outward search distance based on thing height, current attachment state, and wall/floor stick state
5. it runs [`sithCollision_SearchForCollisions()`](../../Libs/sith/Engine/sithCollision.c#L1073)
6. it walks the stack looking for:
   - a valid world floor surface
   - a valid stand-on thing face
   - a valid water transition for rafts
7. if nothing is found, it detaches the thing and may switch the player into falling or leap-forward logic

Architecturally, this is one of the strongest examples of why collision is not an isolated math layer. Floor finding is really an authored gameplay-state update built on top of generic trace primitives.

#### `sithCollision_CheckFloorDistance()`

[`sithCollision_CheckFloorDistance()`](../../Libs/sith/Engine/sithCollision.c#L2545) is the lower-level distance query used when the caller needs the nearest support distance rather than a full attach/detach state transition.

Inputs:

- `pThing`
  Thing whose support distance is being queried.
- `moveNorm`
  Direction in which to search for support, usually downward or along the current stick vector.

Returns:

- the nearest distance to:
  - a world surface flagged `SITH_SURFACE_ISFLOOR`
  - or a stand-on thing face with enough collision data to attach against

How it works:

1. it performs a fixed-distance search of `10.0f`
2. it scans the collision stack
3. world hits only count if the surface is marked floor
4. thing hits only count if they provide face/mesh data and are suitable stand-on candidates
5. it tracks and returns the smallest acceptable distance

Use this when the caller needs "how far away is the next valid support surface" rather than "please attach me to support right now."

## Original Architecture And OpenJones3D Changes

The overall collision architecture is preserved closely:

- fixed scratch buffers
- handler tables
- sector/adjoin graph search
- movement trace plus gameplay-specific response

OpenJones3D does not introduce a parallel replacement algorithm for collision search. There is no collision-side equivalent of the renderer's alternate visibility traversal. The core remains Jones3D-style: deterministic scratch buffers, portal-aware search, direct stack ownership by the caller, and heavily authored gameplay responses.

The practical changes in and around this subsystem are more modest:

- correctness and validation fixes in collision helpers and callers
- small gameplay-query tuning changes where collision data is consumed
- higher-frequency player-physics stepping under `J3D_QOL_IMPROVEMENTS`, which increases how often the same collision API is exercised without changing the collision architecture itself

So the important `QOL` point for this subsystem is not a new collision algorithm. It is that the same original collision core is now exercised by a somewhat cleaner, safer, and in some gameplay paths more frequently updated runtime.

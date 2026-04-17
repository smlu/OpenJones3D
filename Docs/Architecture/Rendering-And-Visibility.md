# Rendering And Visibility

This page documents the render-facing architecture: cameras, visible-sector construction, PVS, off-screen thing/light collection, dynamic lighting, render submission, and the split between the legacy DirectX 6.1c backend and the DirectX 9 backend.

Primary source files:

- [`Libs/sith/Engine/sithCamera.c`](../../Libs/sith/Engine/sithCamera.c)
- [`Libs/sith/Engine/sithRender.c`](../../Libs/sith/Engine/sithRender.c)
- [`Libs/sith/Engine/sithRender.h`](../../Libs/sith/Engine/sithRender.h)
- [`Libs/rdroid/Main/rdroid.c`](../../Libs/rdroid/Main/rdroid.c)
- [`Libs/rdroid/Raster/rdCache.c`](../../Libs/rdroid/Raster/rdCache.c)
- [`Libs/std/Win95/std3D.h`](../../Libs/std/Win95/std3D.h)
- [`Libs/std/Win95/DX6/std3DX6.c`](../../Libs/std/Win95/DX6/std3DX6.c)
- [`Libs/std/Win95/DX9/std3DX9.c`](../../Libs/std/Win95/DX9/std3DX9.c)
- [`Libs/std/Win95/DX9/stdShaderDX9.c`](../../Libs/std/Win95/DX9/stdShaderDX9.c)

## Layer Split

Rendering is divided across three layers:

1. `sith`

   Owns gameplay-facing scene traversal, visibility, dynamic light collection, and final high-level scene submission.

2. `rdroid`

   Owns renderer-facing primitives, cameras, materials, keyframes, meshes, and the raster cache used to batch/draw faces.

3. `std3D` / `stdDisplay`

   Own the actual graphics API backend, device state, texture allocation, projection setup, scene begin/end, and buffer presentation.

The renderer is therefore not a single module. `sith` decides what should be drawn, `rdroid` packages it into renderable primitives, and `std3D` submits it to Direct3D.

## Camera System

[`sithCamera.c`](../../Libs/sith/Engine/sithCamera.c) owns the built-in camera objects and the high-level camera behaviors.

Startup creates a fixed set of system cameras, including:

- internal / first-person
- external / third-person
- cinematic
- idle
- auxiliary unknown helper camera
- orbital

Each `SithCamera` stores:

- camera type
- primary and secondary focus things
- orientation
- look position and look angles
- offset and interpolation state
- embedded `rdCamera`

The embedded `rdCamera` is what the lower render layer uses for frustum, projection, light list, and canvas state.

Architecturally, `sithCamera` is the bridge between gameplay semantics like "focus this thing" or "use external camera offsets" and the renderer-facing `rdCamera`.

For the detailed camera architecture, including built-in camera numbers, interpolation state machines, external-camera rigging, collision-limited camera placement, and the COG cutscene workflow built around camera number `2`, see [Camera-System.md](Camera-System.md).

## Per-Frame Camera Use

During [`sithDrawScene()`](../../Libs/sith/Main/sithMain.c#L596):

1. [`sithCamera_Update(curCamera)`](../../Libs/sith/Engine/sithCamera.c#L471) updates camera pose, interpolation, offsets, and focus logic.
2. [`sithCamera_RenderScene()`](../../Libs/sith/Engine/sithCamera.c#L1421) makes the `rdCamera` current and calls [`sithRender_RenderScene()`](../../Libs/sith/Engine/sithRender.c#L299).

That means camera update is part of the draw half of the frame, not the simulation half. The camera reads already-updated gameplay state and converts it into a render view for the current frame.

## Scene Render Entry

[`sithRender_RenderScene()`](../../Libs/sith/Engine/sithRender.c#L299) performs the render-facing setup:

- update sky state
- push fog updates to `std3D` if needed
- adjust fade when paused
- call [`sithRender_Draw()`](../../Libs/sith/Engine/sithRender.c#L332)
- flush voice subtitles and console text

The heavy work happens in [`sithRender_Draw()`](../../Libs/sith/Engine/sithRender.c#L332).

## Main Render Pass Setup

[`sithRender_Draw()`](../../Libs/sith/Engine/sithRender.c#L332) is the top-level render pipeline driver. At the start of that function the engine:

- sets the active lighting mode
- enables ambient camera light in the render options
- applies the underwater FOV/aspect wobble when needed
- resets all counters for visible sectors, visible thing sectors, alpha adjoins, sprites, and collected lights
- clears the active `rdCamera` light list

It then runs the frame in this order:

1. build the visible sector set
2. expand into off-screen thing/light collection
3. build per-vertex dynamic lighting
4. set the projection state
5. render opaque and sky sectors
6. render things
7. render deferred alpha adjoins

That ordering is deliberate. The visible sector pass establishes the spatial window for the rest of the frame, the off-screen pass expands that window for nearby thing/light influence, and only then does actual geometry submission begin.

## High-Level Portal System Overview

At a high level, the visibility system is a portal renderer built on the sector/adjoin graph.

The idea is simple:

- the camera starts in one sector
- each adjoin surface is treated like a portal into a neighboring sector
- the renderer only walks through portals that are both topologically reachable and actually visible on screen
- every successful portal clip produces a tighter screen window for the next sector

That gives the engine a strong "only walk what the camera could plausibly see" property without scanning the whole world every frame.

### Mental model

```text
camera
  |
  v
[Sector A]
    |
    | portal/adjoin visible through current frustum
    v
[Sector B]
    |
    | portal/adjoin still visible after tighter clipping
    v
[Sector C]
```

If the portal from `A` to `B` is not visible, the renderer never considers `B` through that path. If `A -> B` is visible but the portal from `B` to `C` is clipped away, the traversal stops at `B`.

### Non-PVS portal path at a glance

Without PVS, the renderer discovers candidate sectors purely from the live portal graph.

High-level flow:

```text
start at camera sector
  -> inspect its visible adjoins
  -> backface-test each portal surface
  -> transform and clip the portal polygon
  -> if visible, build child clip frustum
  -> recurse into adjoining sector
  -> repeat
```

Pseudo code:

```C
BuildVisibleSectorList(sector, frustum):
    if sector already built for this frame:
        tighten sector frustum if needed
        return

    register sector as visible

    for each visible adjoin in sector:
        if portal surface is backfacing:
            continue

        transform portal surface
        clippedPortal = ClipPortalAgainstFrustum(adjoin.surface, frustum)
        if clippedPortal is empty:
            continue

        childFrustum = BuildChildFrustumFromPortal(clippedPortal, frustum)
        BuildVisibleSectorList(adjoin.neighborSector, childFrustum)
```

This path is simple and exact with respect to the current frame, but it can spend time portal-testing sectors that are only topologically reachable and never realistically visible from the source sector.

### PVS-assisted portal path at a glance

With PVS enabled, the engine still uses the portal renderer, but it first loads a precomputed table that says which adjoins are worth considering from the current source sector.

High-level flow:

```text
start at camera sector
  -> decode that sector's PVS record into an adjoin table
  -> seed visible sector set with the camera sector
  -> enqueue only PVS-approved adjoins
  -> process adjoins by PVS bucket/order
  -> still backface-test and clip portals live
  -> recurse/grow only through surviving portals
```

Pseudo code:

```C
BuildVisibleSectorListPVS(cameraSector, rootFrustum):
    adjoinTable = DecodePVS(cameraSector.pvsIdx)
    clear visible-adjoin buckets

    cameraSector.clipFrustum = rootFrustum
    BuildVisibleSectorPVS(cameraSector)

    for bucket = firstUsedBucket to lastUsedBucket:
        for each adjoin in visibleAdjoinBuckets[bucket]:
            frustum = SelectParentOrRootFrustum(adjoin, adjoinTable)

            if portal surface is backfacing:
                continue

            transform portal surface
            clippedPortal = ClipPortalAgainstFrustumPVS(adjoin.surface, frustum)
            if clippedPortal is empty:
                continue

            nextSector = adjoin.neighborSector
            if nextSector already built:
                tighten nextSector frustum if needed
            else:
                nextSector.clipFrustum = BuildOrReuseFrustum(clippedPortal, frustum)
                BuildVisibleSectorPVS(nextSector)
```

So the PVS path is still a portal renderer. The difference is that it begins with an offline-generated filter over the adjoin graph.

### Non-PVS and PVS side by side

```text
Non-PVS path
camera sector
  -> try every visible adjoin
  -> portal clip decides what survives

PVS path
camera sector
  -> decode offline PVS record
  -> try only PVS-approved adjoins
  -> portal clip still decides what survives
```

The important conclusion is:

- non-PVS = fully live portal discovery
- PVS = offline-pruned portal discovery plus the same live portal tests

That is why the two paths feel architecturally similar in code. PVS is not a second renderer; it is a prefilter for the same sector/portal traversal model.

## Visible Sector Collection

### Pass entry and mode selection

The first decision inside [`sithRender_Draw()`](../../Libs/sith/Engine/sithRender.c#L332) is whether to use:

- [`sithRender_BuildVisibleSectorListPVS()`](../../Libs/sith/Engine/sithRender.c#L544) when PVS culling is enabled and the world has PVS data
- [`sithRender_BuildVisibleSectorList()`](../../Libs/sith/Engine/sithRender.c#L421) otherwise

Both paths ultimately populate the same per-frame state:

- `sithRender_aVisibleSectors`
- each sector's `pClipFrustum`
- the initial visible-thing-sector seed list
- directly visible dynamic lights

So the difference is not in the final data model. The difference is in how candidate sectors and portals are discovered.

### Portal-frustum traversal path

[`sithRender_BuildVisibleSectorList()`](../../Libs/sith/Engine/sithRender.c#L421) is the classic portal/sector path.

For each sector it:

1. ensures the sector itself is registered as visible
2. iterates visible adjoins
3. rejects portals whose surface is backfacing relative to the camera
4. makes sure the portal surface vertices have been transformed for this render tick through [`sithRender_BuildVisibleSurface()`](../../Libs/sith/Engine/sithRender.c#L482)
5. clips the portal polygon against the current frustum
6. either reuses the parent frustum or builds a tighter child frustum with [`sithRender_BuildClipFrustrum()`](../../Libs/sith/Engine/sithRender.c#L505)
7. recurses into the adjoining sector

This is why the renderer is strongly topology-driven. The engine does not start from "all sectors in the world." It starts from the camera sector and walks the sector graph through visible portals.

### PVS-assisted portal path

[`sithRender_BuildVisibleSectorListPVS()`](../../Libs/sith/Engine/sithRender.c#L544) keeps the same portal-clipping idea, but inserts a PVS filter in front of it.

The PVS path:

1. loads the PVS table for the current camera sector
2. builds per-priority visible-adjoin work lists
3. starts from a full-screen frustum
4. clips only those portals that survive the PVS lookup
5. grows each newly accepted sector through [`sithRender_BuildVisibleSectorPVS()`](../../Libs/sith/Engine/sithRender.c#L660)

That means PVS is a coarse reachability accelerator, not a replacement for the portal-frustum tests. The engine still clips portal polygons and still propagates frusta sector by sector.

#### What "PVS" means here

In this engine, PVS is "Potentially Visible Set" data stored per world and keyed per source sector.

At render time, the current camera sector's `pvsIdx` is used to decode a packed visibility record from the world's `aPVS` blob through [`sithPVS_SetTable()`](../../Libs/sith/World/sithPVS.c#L176).

The important architectural detail is that the decoded data is an adjoin table, not a ready-made list of visible sectors. PVS therefore accelerates the existing portal traversal rather than replacing it.

#### Packed-table decode

The first step in [`sithRender_BuildVisibleSectorListPVS()`](../../Libs/sith/Engine/sithRender.c#L544) is:

1. decode the packed PVS record for the current camera sector into `sithRender_aAdjoinTable`
2. clear the temporary visible-adjoin work buckets in `sithRender_aVisibleAdjoins`
3. initialize a full-screen orthographic clip rectangle for the root camera sector
4. seed traversal by calling [`sithRender_BuildVisibleSectorPVS()`](../../Libs/sith/Engine/sithRender.c#L660) on the starting sector

The decode step is handled by [`sithPVS_SetTable()`](../../Libs/sith/World/sithPVS.c#L176), which expands the packed byte stream into one byte per world adjoin using a run-length encoding:

- high bit set: copy the following literal bytes into the adjoin table
- high bit clear: write a run of zeros

This gives the renderer a sparse per-adjoin visibility control table without storing a huge uncompressed array for every sector.

#### Per-adjoin meaning and bucketed traversal

Once decoded, each adjoin's table byte is interpreted this way:

- `0`
  Ignore this adjoin for the current PVS traversal.
- non-zero
  Consider this adjoin.

The non-zero byte then splits into:

- low `7` bits: a bucket/order value
- high bit `0x80`: a flag that changes how the working clip frustum is chosen for that adjoin

[`sithRender_BuildVisibleSectorPVS()`](../../Libs/sith/Engine/sithRender.c#L660) walks the current sector's adjoins and, for each adjoin that is both render-visible and non-zero in the decoded PVS table, inserts it into a linked work list bucket indexed by the low `7` bits.

That is why the PVS traversal uses `sithRender_curPVSIndex` and `sithRender_lastPVSIndex` rather than plain recursion. The renderer is effectively processing adjoins in a bucketed wave order supplied by the PVS data.

#### Root sector and work-list expansion

After the starting sector is registered as visible, the renderer processes the bucketed visible-adjoin lists in increasing order:

1. take the next bucket
2. walk the linked adjoins stored in that bucket
3. clip the portal surface for each adjoin
4. if the portal survives, either tighten or reuse the clip frustum
5. if the neighboring sector has not been seen this frame, register it and enqueue its PVS-approved adjoins

This means PVS is still expanding the same sector/adjoin graph as the classic portal path. It simply prunes most adjoins before the expensive portal-clipping work begins.

#### Why some adjoins restart from the full-screen frustum

When the decoded adjoin-table byte has bit `0x80` set, [`sithRender_BuildVisibleSectorListPVS()`](../../Libs/sith/Engine/sithRender.c#L544) does not inherit the source sector's clip frustum for that portal. Instead it restarts clipping from the full-screen frustum.

In other words, the current implementation treats bit `0x80` as "do not constrain this portal by the parent sector's tighter screen window."

That is a significant difference from the normal portal recursion path. It means the PVS data can choose between:

- ordinary portal-style propagation from an already-tight parent frustum
- a reset-to-root clip test for specific adjoins

The exact offline rule that generated those flags is not documented in the runtime code, but the runtime behavior is explicit.

#### PVS-specific portal clipping

The PVS path uses [`rdClip_ClipFacePVS()`](../../Libs/rdroid/Engine/rdClip.c#L395) instead of the more general clip path.

The important difference is in [`rdClip_Face3WPVS()`](../../Libs/rdroid/Engine/rdClip.c#L403): it intentionally does not clip against the frustum far plane.

That makes sense here because the PVS stage is mainly about portal acceptance and screen-space window tightening. It still needs left/right/top/bottom clipping to refine the portal window, but it does not need to use far-plane rejection as the main coarse filter.

#### Why the portal clip still matters after PVS

PVS only answers a coarse question:

- "from this source sector, which adjoins are even worth considering, and in roughly what processing order?"

It does not answer the finer question:

- "is this portal actually visible through the currently clipped chain of portals on this frame?"

That second question still requires runtime portal clipping.

This is why the PVS path still:

- checks portal-facing direction against the camera
- transforms the portal surface for the current frame
- clips the portal polygon
- builds or tightens per-sector clip frusta

Without that second stage, PVS would over-admit sectors that are only potentially visible in a broad offline sense, not actually visible from the current camera pose and portal chain.

### Sector finalization

Once a sector is accepted by either path, [`sithRender_BuildVisibleSector()`](../../Libs/sith/Engine/sithRender.c#L687) performs the common bookkeeping.

That function:

- stamps the sector with the current render tick
- appends it to `sithRender_aVisibleSectors`
- sends `SIGHTED` to sector COGs the first time the sector is seen
- copies the current clip frustum into the per-frame sector-frustum array
- collects directly visible thing lights and actor/player headlights
- seeds the visible-thing-sector list with the visible sector itself

This is an important architectural detail: directly visible sectors are automatically valid thing-render sectors too. The off-screen thing traversal only extends that set; it does not replace it.

## Off-Screen Thing And Light Collection

### Why there is a second traversal

The visible-sector pass alone is not enough for the full frame.

A sector can fall outside the immediate camera-frustum-visible set and still matter because:

- a thing in that off-screen sector may still be partially visible on screen, for example the sun sprite in the Mero level
- an emitted light may influence geometry that is on screen
- an actor or player headlight may need to reach geometry through nearby adjoining sectors

So after visible sectors are known, [`sithRender_BuildVisibleThingSectorList()`](../../Libs/sith/Engine/sithRender.c#L901) runs a second pass that is specifically about nearby influence, not about direct portal visibility.

### Traversal mode dispatch

[`sithRender_BuildVisibleThingSectorList()`](../../Libs/sith/Engine/sithRender.c#L901) is just a dispatcher. It selects:

- [`sithRender_BuildVisibleThingSectorListDFS()`](../../Libs/sith/Engine/sithRender.c#L917) for the legacy traversal
- [`sithRender_BuildVisibleThingSectorListBFS()`](../../Libs/sith/Engine/sithRender.c#L991) for the OpenJones3D breadth-first path

Both modes write into the same structures:

- `sithRender_aVisibleThingSectors`
- `sithRender_totalVisibleThingSectors`
- the camera light list via collected thing lights

### Legacy DFS-like traversal

[`sithRender_BuildVisibleThingSectorListDFS()`](../../Libs/sith/Engine/sithRender.c#L917) starts from the already visible sectors, walks their visible adjoins, and recursively expands through [`sithRender_CollectVisibleThingSector()`](../../Libs/sith/Engine/sithRender.c#L949).

The legacy recursion does the following:

1. marks the sector on first encounter
2. collects thing lights if the current distance is inside the light range
3. adds the sector to the visible-thing-sector list if the current distance is inside the thing range
4. recurses into visible adjoining sectors while the accumulated path distance remains under the maximum collection range

The reason this can miss useful sectors is that marking happens on first encounter. If the sector is first reached through a worse path, later shorter paths do not get a chance to re-evaluate it.

### OpenJones3D breadth-first traversal

[`sithRender_BuildVisibleThingSectorListBFS()`](../../Libs/sith/Engine/sithRender.c#L991) keeps the same goal but changes the traversal discipline.

The BFS path:

1. seeds a queue from the visible sectors' visible adjoins
2. stores each candidate sector together with its accumulated distance
3. processes the queue breadth-first
4. finalizes each sector only when it is popped for the first time
5. enqueues its visible adjoining sectors while staying under the configured maximum range

That gives the traversal a much more stable "nearest first" behavior. It still walks the same sector/adjoin graph and still obeys the same distance concept, but it is much less sensitive to adjoin order and unlucky early paths.

### Collection distances

The off-screen pass uses two separate limits:

- maximum thing-collection distance
- maximum light-collection distance

Legacy defaults:

- thing collection: `8.0`
- light collection: `8.0`

OpenJones3D `QOL` defaults:

- thing collection: `18.0`
- light collection: `18.0`

The broader `QOL` default exists because the BFS path is meant to gather a more complete neighboring-sector set. In larger spaces, smaller values can still under-collect forward sectors even when the traversal order itself is improved.

### Thing-light collection helper

Both traversal modes eventually rely on [`sithRender_CollectThingLights()`](../../Libs/sith/Engine/sithRender.c#L1085) to extend the camera light list.

That helper collects two kinds of lights:

- thing-emitted dynamic lights from `SITH_TF_EMITLIGHT`
- actor/player headlights

The helper only collects active, non-disabled, non-invisible, non-destroyed emitters. That is important because the off-screen pass is specifically trying to preserve meaningful lighting influence, not just blindly gather every thing in neighboring sectors.

## Dynamic Light Build

After the visible-sector and off-screen collection passes finish, [`sithRender_BuildDynamicLights()`](../../Libs/sith/Engine/sithRender.c#L1127) computes per-vertex dynamic lighting for the currently visible sectors.

Its work is:

1. for each visible sector, find the subset of camera lights whose radius intersects that sector's bounding sphere
2. for each sector vertex touched this frame, accumulate light contribution from the relevant lights
3. clamp the resulting RGB contribution
4. stamp the vertex with the current render tick so it is not recomputed twice

That is why the world keeps:

- `aVertDynamicLights`
- `aVertDynamicLightsRenderTicks`

The render passes that follow then consume those per-vertex light values instead of recalculating light contribution surface by surface.

## Sector Rendering

[`sithRender_RenderSectors()`](../../Libs/sith/Engine/sithRender.c#L752) is the first real submission pass after traversal and light build.

### Opaque pass entry

The sector pass:

- starts from `sithRender_aVisibleSectors`
- prepares shared face-view pointers for transformed vertices, texture vertices, and dynamic-light arrays
- applies a global fog flag if the world has fog enabled

This pass is responsible only for world surfaces. Things and alpha adjoins are handled later.

### Surface filtering and alpha-adjoin deferral

For each visible sector and each surface inside that sector, the renderer first rejects:

- surfaces with `RD_GEOMETRY_NONE`
- surfaces whose normal is backfacing relative to the camera

If a surface has an adjoin and a translucent texture flag, the sector pass does not draw it immediately. Instead it appends the surface to `sithRender_aAlphaAdjoins` for the later alpha-adjoin pass.

That means the main sector pass is structurally an opaque-and-sky pass. Transparent adjoin surfaces are intentionally deferred.

### Sky-surface path

If a surface is marked as ceiling sky or horizon sky, the sector pass:

1. clips the face through [`rdClip_QClipFaceW()`](../../Libs/rdroid/Engine/rdClip.c#L3970)
2. projects the clipped vertices
3. converts the clipped polygon into a sky plane through the sky helpers
4. queues it into the raster cache with [`rdCache_AddProcFace()`](../../Libs/rdroid/Raster/rdCache.c#L191)

This is not normal world-face rendering. Sky faces are treated as inputs to sky-plane generation rather than as literal geometry submission.

### Opaque world-surface path

For non-sky surfaces, the pass:

1. ensures vertices have been transformed for this render tick
2. allocates a raster-cache entry
3. clips the face through [`rdClip_FaceToPlane()`](../../Libs/rdroid/Engine/rdClip.c#L3985)
4. clamps the face lighting mode against the global lighting mode
5. combines surface extra light with sector extra light
6. sets material, cel, and fog-related flags
7. queues the face into the raster cache with [`rdCache_AddProcFace()`](../../Libs/rdroid/Raster/rdCache.c#L191)

After all visible sectors have been processed, the pass flushes the opaque cache through [`rdCache_Flush()`](../../Libs/rdroid/Raster/rdCache.c#L141).

## Thing Rendering

After world surfaces are submitted, [`sithRender_RenderThings()`](../../Libs/sith/Engine/sithRender.c#L1189) walks `sithRender_aVisibleThingSectors`.

### Sector-local thing pass

For each visible thing sector, the function computes a sector ambient light by combining:

- `sector->ambientLight`
- `sector->extraLight`

That ambient light becomes the default ambient contribution for things rendered out of that sector.

### Per-thing filtering and frustum tests

For each thing in that sector, the pass first rejects:

- disabled things
- invisible things
- destroyed things
- the primary focus thing in non-external camera modes

It then transforms the thing position into camera space and determines a culling radius based on the render type:

- models use collision size/movesize and model size
- sprites use sprite radius
- particles use particle radius
- polylines use line length

Polylines skip the sphere-frustum test and are always considered renderable. Other thing types are tested against the sector frustum before they proceed.

### Ambient light versus flat lights

If a thing emits a flat light rather than a dynamic-radius light, the thing pass folds that flat light into the sector ambient light before drawing the thing.

That is different from the earlier dynamic light collection path. Dynamic lights are converted into per-vertex lighting. Flat lights are treated more like local ambient contribution for the current thing draw.

### Rendering the thing wrapper

Once a thing passes culling, [`sithRender_RenderThing()`](../../Libs/sith/Engine/sithRender.c#L1319) handles render-side state around the actual draw call.

It:

- sends `SIGHTED` to a linked thing COG the first time the thing is seen
- wakes sleeping AI by clearing the sleeping mode bit
- stamps the thing with the current render frame
- temporarily copies the thing world position into `orient.dvec`
- delegates the actual draw to [`sithThing_Draw()`](../../Libs/sith/World/sithThing.c#L393)
- optionally renders a shadow through [`sithShadow_RenderThingShadow()`](../../Libs/sith/Engine/sithShadow.c#L37)
- accumulates opaque and alpha thing-poly counters
- restores the orientation translation field afterward

So `sithRender_RenderThing()` is a bridge wrapper. It is not the full draw implementation for every thing type.

### Final thing draw dispatch

[`sithThing_Draw()`](../../Libs/sith/World/sithThing.c#L393) is where the engine switches from general gameplay thing state to the lower render object.

That function performs thing-type-specific preparation such as:

- setting polyline alpha
- setting model alpha color
- temporarily swapping meshes from the thing swap list

It then dispatches by thing type:

- cameras, ghosts, and hints do not draw
- sprites go through [`sithSprite_Draw()`](../../Libs/sith/World/sithSprite.c#L106)
- polylines update their dynamic state and then draw
- most remaining renderable types go through [`rdThing_Draw()`](../../Libs/rdroid/Engine/rdThing.c#L198)

At the end of the pass, [`sithRender_RenderThings()`](../../Libs/sith/Engine/sithRender.c#L1189) flushes the thing geometry through [`rdCache_Flush()`](../../Libs/rdroid/Raster/rdCache.c#L141).

## Alpha-Adjoin Rendering

Alpha adjoins are rendered last through [`sithRender_RenderAlphaAdjoins()`](../../Libs/sith/Engine/sithRender.c#L1374).

### Deferred translucent surface queue

This pass consumes the queue that [`sithRender_RenderSectors()`](../../Libs/sith/Engine/sithRender.c#L752) built earlier in `sithRender_aAlphaAdjoins`.

So alpha adjoins are not discovered independently. They are discovered during opaque sector rendering and then replayed in a separate translucent pass.

### Alpha-adjoin submission path

For each queued surface, the alpha pass:

1. rebuilds sector ambient light from sector ambient plus sector extra light
2. ensures the surface vertices are transformed for the current render tick
3. allocates an alpha raster-cache entry
4. clips the face through [`rdClip_FaceToPlane()`](../../Libs/rdroid/Engine/rdClip.c#L3985)
5. fills lighting, extra light, material, cel, and fog-related flags
6. submits the face through [`rdCache_AddAlphaProcFace()`](../../Libs/rdroid/Raster/rdCache.c#L198)

After all queued alpha adjoins are prepared, the pass flushes the alpha cache through [`rdCache_FlushAlpha()`](../../Libs/rdroid/Raster/rdCache.c#L166).

Architecturally this is a deferred translucent surface pass, not a separate visibility system. The alpha-adjoin renderer depends completely on the earlier sector pass to discover and queue the surfaces it needs to draw.

## rdCache And rdroid

`rdroid` is the lower render layer that packages geometry for the active graphics backend.

Relevant responsibilities:

- `rdCamera` stores camera/frustum/light state
- `rdMaterial`, `rdModel3`, `rdSprite`, `rdParticle`, `rdPolyline`, `rdPuppet` manage renderable asset data
- `rdCache` accumulates processed faces and submits them in batches

The architecture here is important:

- `sithRender` decides *what* to draw
- `rdroid` decides *how* those primitives are represented and flushed
- `std3D` decides *how* the final draw reaches Direct3D

## stdDisplay And std3D

`stdDisplay` manages:

- display devices
- display modes
- primary/back buffer surfaces or swap chains
- presentation

`std3D` manages:

- 3D device enumeration/open/close
- scene begin/end
- texture formats and cache
- projection/fog state
- render-state translation
- low-level draw calls

From the rest of the engine's perspective, `std3D` is the hardware abstraction layer for rendering.

## DirectX 6.1c Backend

The legacy backend lives under [`Libs/std/Win95/DX6/`](../../Libs/std/Win95/DX6).

Architectural characteristics:

- built around older DirectDraw/Direct3D interfaces
- explicit viewport object usage
- fixed-function style pipeline
- legacy texture-format and device enumeration constraints

This path represents the original renderer architecture most closely.

## DirectX 9 Backend

The modern backend lives under [`Libs/std/Win95/DX9/`](../../Libs/std/Win95/DX9).

Architectural extensions introduced there include:

- Direct3D 9 device/runtime management
- optional vertex and index buffer paths
- shader system support through [`stdShaderDX9.c`](../../Libs/std/Win95/DX9/stdShaderDX9.c)
- wider texture-format and device-capability handling
- modern fog/shader constant handling

The shader system owns:

- compiled shader objects
- shader parameter tables
- vertex declarations
- global shader constants such as viewport and fog data

This is not just a backend port. It is a meaningful extension of the renderer architecture while still preserving the same upper-layer scene traversal and submission model.

## Where OpenJones3D Extends The Original Renderer

The biggest architectural extensions are:

- DirectX 9 backend alongside the original DirectX 6.1c backend
- shader layer for the DX9 path
- VBO/IBO support in the DX9 path
- wider texture-format handling, including true-color textures
- breadth-first off-screen thing/light traversal
- longer default collection distances
- modern filtering options, automatic mipmap generation, and MSAA support in the lower graphics layer

Crucially, those changes sit underneath or alongside the original scene-graph/sector-portal logic instead of replacing the entire renderer design. The renderer still thinks in terms of sectors, surfaces, adjoins, clip frusta, thing sectors, and per-vertex dynamic lights.

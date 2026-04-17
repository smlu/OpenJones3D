# World And Resources

This page documents the world container, how level/resource files are loaded, and how ownership is divided across sectors, surfaces, things, and static resources.

Primary source files:

- [`Libs/sith/World/sithWorld.c`](../../Libs/sith/World/sithWorld.c)
- [`Libs/sith/World/sithThing.c`](../../Libs/sith/World/sithThing.c)
- [`Libs/sith/World/sithSector.c`](../../Libs/sith/World/sithSector.c)
- [`Libs/sith/World/sithSurface.c`](../../Libs/sith/World/sithSurface.c)
- [`Libs/sith/World/sithMaterial.c`](../../Libs/sith/World/sithMaterial.c)
- [`Libs/sith/World/sithModel.c`](../../Libs/sith/World/sithModel.c)
- [`Libs/sith/Engine/sithPuppet.c`](../../Libs/sith/Engine/sithPuppet.c)
- [`Libs/sith/Devices/sithSound.c`](../../Libs/sith/Devices/sithSound.c)

## The World Object Is The Main Runtime Container

`SithWorld` is the central ownership object for almost everything that belongs to a loaded level or static resource set.

At a high level, a world owns:

- sounds
- materials
- vertices and texture vertices
- adjoins and surfaces
- sectors
- AI classes
- models
- sprites
- keyframes
- puppet classes
- sound classes
- cog scripts
- cog instances
- thing templates
- things
- PVS data

The world also owns per-runtime render buffers allocated after load:

- transformed vertices
- per-vertex dynamic-light accumulators
- per-vertex render-tick tables

That means `SithWorld` is not just "the level geometry." It is the root object for both spatial data and many higher-level resource tables that gameplay code depends on.

## Static World Versus Current World

The engine keeps two distinct worlds alive:

- `sithWorld_g_pStaticWorld`
- `sithWorld_g_pCurrentWorld`

The static world is used as a shared resource domain for data that should persist independently of the currently loaded gameplay level. A number of systems use static-world indices and resource lookups, especially for shared sounds, scripts, and support data.

The current world is the active gameplay level. It contains the current sectors, things, player placement, AI instances, and render-visible world state.

This split is important architecturally because many APIs must be explicit about which resource namespace they are addressing. Static-world indices are commonly masked or tagged differently from current-world indices so the engine can tell them apart.

## Text NDY Loading Versus Binary CND Loading

[`sithWorld_Load()`](../../Libs/sith/World/sithWorld.c#L351) accepts either text or binary worlds:

- `.ndy` uses [`sithWorld_LoadEntryText()`](../../Libs/sith/World/sithWorld.c#L515)
- `.cnd` uses [`sithWorld_LoadEntryBinary()`](../../Libs/sith/World/sithWorld.c#L1456)

The text path reads a configuration-style file using `stdConffile`, scans `SECTION:` blocks, and dispatches each section to a registered parser.

The binary path uses the packed `CndWorld` layout plus per-section binary parsers.

In both cases, the world-loader architecture is deliberately section-oriented rather than monolithic. Each subsystem registers the parser for the section it owns. That keeps the world loader thin and lets ownership stay with the subsystem that understands the data.

## Section Parser Registration

During [`sithWorld_Startup()`](../../Libs/sith/World/sithWorld.c#L287), the engine registers handlers for the world sections, including:

- `copyright`
- `header`
- `sounds`
- `materials`
- `georesource`
- `sectors`
- `aiclass`
- `models`
- `sprites`
- `keyframes`
- `animclass`
- `soundclass`
- `cogscripts`
- `cogs`
- `templates`
- `things`
- `pvs`

That registration model is one of the cleanest examples of the original engine architecture:

- the world loader owns the parse loop
- subsystem loaders own the actual section semantics

OpenJones3D keeps that division rather than collapsing everything into a single centralized parser.

## World Header State

The world header contains the global environment values that later subsystems consume, such as:

- gravity
- ceiling-sky and horizon parameters
- horizon distance
- LOD distances
- fog enable/state/color/start/end

These values are not render-only. Gravity feeds physics, fog feeds the render pipeline, and sky parameters feed the sky-surface conversion code.

## Post-Load Processing

After the raw data is loaded, [`sithWorld_LoadPostProcess()`](../../Libs/sith/World/sithWorld.c#L400) turns it into an active runtime world.

Important work performed there:

1. allocate runtime render arrays
2. rebuild sector-adjoin linked lists
3. place players
4. find floor/wall attachments for physics things that need it
5. validate required world sections
6. mark the world initialized

This split between "load raw data" and "post-process into runtime state" is important. The raw section readers do not have to finish every cross-link and runtime buffer immediately; post-processing is where the world becomes simulation-ready.

## Adjoins And Sector Graph Structure

Sectors are connected through `SithSurfaceAdjoin` entries referenced from surfaces. During post-load, the world rebuilds each sector's `pFirstAdjoin` chain so later subsystems can traverse the sector graph efficiently.

Each sector is also expected to be convex. In practice that means a sector is treated as a convex volume bounded by its surfaces, and many core systems assume that property when they test whether a point, sphere, or trace is inside the sector or can cross one of its adjoins.

That convexity assumption matters to:

- sector-membership tests
- collision and floor queries
- portal/adjoin traversal
- visible-sector construction

So if level geometry is non-convex, it is normally represented as multiple convex sectors connected together rather than as one large irregular region.

That graph is used by several independent systems:

- rendering sector visibility
- off-screen thing/light collection
- collision searches across adjoining sectors
- AI awareness propagation
- water/floor searches

The same sector/adjoin graph therefore underpins both rendering and gameplay systems, even though each subsystem traverses it differently.

## Things Are The Main Gameplay Object Nodes

`SithThing` is the core polymorphic gameplay object. A thing can represent:

- actors
- players
- weapons
- debris
- items
- explosions
- script helper things
- corpses
- particles
- sprites
- hints
- polylines
- cameras

Each thing is effectively a convergence node where multiple subsystems meet:

- render data
- movement state
- collision state
- attachment state
- AI control block
- puppet/animation state
- sound-class linkage
- COG linkage
- type-specific state unions such as actor, item, weapon, particle, and explosion data

This is one of the most important architectural characteristics of Jones3D/Sith-style engines: the "thing" is not just an entity id. It is a rich object with direct ownership and references into many gameplay/runtime systems.

## Thing Allocation Model

Worlds allocate a fixed thing array. OpenJones3D extends that with configurable extra capacity, but the architecture stays array-based rather than moving to a fully dynamic entity storage model.

The thing lifecycle is:

1. allocate world thing array
2. initialize every slot as `SITH_THING_FREE`
3. hand out indices from the free-index table
4. initialize per-type/per-control/per-render state
5. link the thing into its sector
6. update it every frame
7. free it back to the slot array when destroyed

Because so much code uses stable thing indices and direct slot pointers, the fixed-array model is part of the original engine contract, not just an implementation detail.

## Thing Initialization And Post-Initialization

[`sithThing_Initialize()`](../../Libs/sith/World/sithThing.c#L1419) performs type- and subsystem-specific initialization. Examples include:

- actor/player state-change reset
- actor difficulty application
- actor weapon initialization
- item/explosion/particle/sprite startup
- puppet creation
- AI control-block creation
- sound-class create sounds
- optional initial floor finding
- optional initial `INITIALIZED` cog message

The architecture here is distributed:

- the thing system owns the overall lifecycle
- specialized subsystems own their piece of initialization

## Resource Ownership By Subsystem

Even though the world owns the tables, each subsystem still owns the interpretation and loader/writer logic for its resource type:

- sounds: [`sithSound.c`](../../Libs/sith/Devices/sithSound.c)
- materials: [`sithMaterial.c`](../../Libs/sith/World/sithMaterial.c)
- models: [`sithModel.c`](../../Libs/sith/World/sithModel.c)
- sprites: [`sithSprite.c`](../../Libs/sith/World/sithSprite.c)
- keyframes/puppets: [`sithPuppet.c`](../../Libs/sith/Engine/sithPuppet.c)
- AI classes: [`sithAIClass.c`](../../Libs/sith/AI/sithAIClass.c)
- sound classes: [`sithSoundClass.c`](../../Libs/sith/World/sithSoundClass.c)
- cog scripts and cog instances: [`sithCog.c`](../../Libs/sith/Cog/sithCog.c)
- thing templates: [`sithTemplate.c`](../../Libs/sith/World/sithTemplate.c)

That is why the world loader is mostly an orchestration layer. The actual semantics of "what a model is" or "what a keyframe class means" are intentionally left inside the owning subsystem.

## PVS Ownership

Potentially Visible Set data is stored per world and loaded through the PVS subsystem. When PVS culling is enabled, rendering uses that precomputed visibility data as a coarse visibility accelerator before the finer clip/frustum tests.

The PVS system does not replace normal visibility logic; it narrows the search space that the normal sector/surface visibility code still processes.

### Runtime ownership model

At runtime, PVS lives in two places:

- the world-level PVS blob, `aPVS`, with total size `sizePVS`
- a per-sector `pvsIdx` stored on each `SithSector`

The important detail is that `pvsIdx` is not a sector number. It is an offset into the world's packed PVS blob. When rendering starts from a camera sector, the engine jumps to `&aPVS[pSector->pvsIdx]` and decodes the visibility data from there.

That is why PVS belongs to the world object. The data is authored and stored per level, and each sector carries only the offset needed to find its own visibility record.

### Load paths and serialized form

The text and binary load paths both preserve that ownership model.

Text `.ndy` loading through [`sithPVS_ReadPVSText()`](../../Libs/sith/World/sithPVS.c#L72) reads:

1. `PVS Size`
2. the packed PVS blob as hexadecimal `uint32_t` words
3. one hexadecimal `pvsIdx` value per sector

Binary `.cnd` loading through [`sithPVS_ReadPVSBinary()`](../../Libs/sith/World/sithPVS.c#L155) reads only the packed blob itself. The sector-side `pvsIdx` values are already carried in each sector record and restored when the sector data is loaded.

So the serialized model is:

- one world-wide packed PVS byte stream
- one per-sector offset into that stream

### What the packed table contains

The packed data does not decode directly into "visible sectors." It decodes into an adjoin table.

That decoding is performed by [`sithPVS_SetTable()`](../../Libs/sith/World/sithPVS.c#L176), which expands the packed byte stream into one byte per world adjoin.

The decompression scheme is a simple run-length encoding:

- if the next packed byte has bit `0x80` set, the low `7` bits specify how many following literal bytes should be copied into the adjoin table, with a run length of `value + 1`
- if the next packed byte does not have bit `0x80` set, it describes a zero run and the decoder writes `value + 3` zero bytes into the output table

That makes the packed stream efficient for sparse visibility data, because long stretches of adjoins that are irrelevant to the current source sector can be represented as short zero runs.

### Meaning of the decoded adjoin bytes

After decoding, each adjoin gets one byte in the temporary runtime table:

- `0`
  This adjoin is not part of the PVS-driven traversal from the current source sector.
- non-zero
  This adjoin may be used by the PVS-driven traversal.

The non-zero byte also carries two pieces of control information used by the renderer:

- low `7` bits
  A traversal bucket / ordering value
- high bit `0x80`
  A special flag that makes the PVS path restart clipping from the full-screen frustum for that adjoin instead of inheriting the parent sector frustum

That means the PVS data is richer than a plain yes/no visibility bitset. It gives the renderer both:

- a filtered set of candidate adjoins
- a coarse ordering/grouping for how those candidates should be processed

### Why the data is adjoin-based

This fits the rest of the engine architecture well.

The renderer is fundamentally portal-driven: it traverses the sector graph by walking through adjoins. So the most useful precomputed visibility accelerator is not just "which sectors might be visible," but "which adjoins should even be considered from this source sector, and in what grouping/order."

That is why the PVS data plugs naturally into the sector/adjoin traversal code instead of replacing it with a completely separate visibility system.

### High-level PVS generation overview

The current repository documents how PVS is stored, loaded, decoded, and consumed at runtime. It does not contain an offline PVS builder that computes new PVS data from scratch inside the engine.

So the generation process described here is the high-level pipeline that a level tool or external generator needs to follow.

The goal of PVS generation is:

- for each source sector
- determine which adjoins are worth considering during visibility traversal
- assign the traversal bucket/order metadata expected by the runtime
- pack that per-sector adjoin table into the compressed world PVS blob

Conceptually it looks like this:

```text
for each source sector:
    analyze portal/sector reachability
    decide which adjoins belong in the potential-visibility set
    assign per-adjoin control bytes
    write the sector's packed table into the world PVS blob
    store the resulting blob offset as that sector's pvsIdx
```

### Practical generation inputs

A generator needs at least:

- the sector graph
- the adjoin graph
- sector geometry and portal polygons
- enough geometric testing to decide whether a portal chain can still contribute visibility from the source sector

Because the runtime table is adjoin-based, the natural generation domain is also adjoin-based rather than just sector-based.

### High-level generation algorithm

A practical offline generator would usually:

1. choose one sector as the source sector
2. start from that sector's outgoing visible adjoins
3. propagate through portal chains while maintaining a clipped visibility window
4. mark adjoins that survive this offline portal test as potentially visible from the source sector
5. assign bucket/order values that approximate the preferred traversal order for runtime use
6. optionally mark special adjoins with the `0x80` behavior when the generated data wants runtime clipping to restart from the full-screen root frustum
7. compress the finished per-adjoin byte table with the packed run-length scheme used by [`sithPVS_SetTable()`](../../Libs/sith/World/sithPVS.c#L176)

Pseudo code:

```C
GeneratePVSForSourceSector(sourceSector):
    clear per-adjoin table to zero
    queue = sourceSector.visibleAdjoins with root clip window

    while queue not empty:
        item = pop next queued portal state
        if portal chain is not geometrically visible:
            continue

        mark item.adjoin as potentially visible
        assign bucket/order metadata

        childWindow = ClipWindowThroughPortal(item.window, item.adjoin.surface)
        enqueue visible outgoing adjoins from item.adjoin.neighborSector using childWindow

    return CompressAdjoinTable(perAdjoinTable)
```

Compact C-style sketch:

```C
static size_t PVS_GenerateForSector(
    SithWorld* world,
    SithSector* srcSector,
    uint8_t* adjoinTable,
    float viewportWidth,
    float viewportHeight
)
{
    rdClipFrustum rootFrustum;
    PVS_InitRootFrustum(&rootFrustum, viewportWidth, viewportHeight);
    memset(adjoinTable, 0, world->numAdjoins);

    PVSQueue queue[256];
    size_t head = 0, tail = 0;

    for (SithSurfaceAdjoin* adjoin = srcSector->pFirstAdjoin; adjoin; adjoin = adjoin->pNextAdjoin)
    {
        if ((adjoin->flags & SITH_ADJOIN_VISIBLE) != 0)
        {
            queue[tail++] = (PVSQueue){ adjoin, rootFrustum, 1 };
        }
    }

    while (head < tail)
    {
        PVSQueue item = queue[head++];
        size_t adjoinIdx = (size_t)(item.pAdjoin - world->aAdjoins);

        if (!PVS_PortalChainVisible(&item.frustum, item.pAdjoin->pAdjoinSurface))
        {
            continue;
        }

        adjoinTable[adjoinIdx] = item.bucket;

        rdClipFrustum childFrustum;
        if (!PVS_BuildChildFrustum(&childFrustum, &item.frustum, item.pAdjoin->pAdjoinSurface))
        {
            continue;
        }

        SithSector* nextSector = item.pAdjoin->pAdjoinSector;
        for (SithSurfaceAdjoin* next = nextSector->pFirstAdjoin; next; next = next->pNextAdjoin)
        {
            if ((next->flags & SITH_ADJOIN_VISIBLE) != 0)
            {
                queue[tail++] = (PVSQueue){ next, childFrustum, item.bucket + 1 };
            }
        }
    }

    return PVS_CompressAdjoinTable(world, adjoinTable, srcSector);
}
```

This sketch is intentionally small and readable rather than fully production-ready. It uses the engine's real data model:

- `SithWorld`
- `SithSector`
- `SithSurfaceAdjoin`
- `rdClipFrustum`
- per-sector `pvsIdx`
- the final packed `aPVS` blob

The helper names in that snippet such as `PVS_InitRootFrustum()`, `PVS_PortalChainVisible()`, `PVS_BuildChildFrustum()`, and `PVS_CompressAdjoinTable()` are descriptive stand-ins for the offline steps a generator needs to perform. They are not current exported runtime functions.

One compact source-shaped way to define those helpers in documentation terms is:

```C
#define PVS_MAX_PORTAL_VERTS 64

typedef struct PVSQueue
{
    SithSurfaceAdjoin* pAdjoin;
    rdClipFrustum frustum;
    uint8_t bucket;
} PVSQueue;

static rdPrimit3 g_PVSClippedFace;
static rdVector3 g_PVSClipVerts[PVS_MAX_PORTAL_VERTS];
static rdVector3 g_PVSProjectedVerts[PVS_MAX_PORTAL_VERTS];
static size_t g_PVSProjectedCount;

static void PVS_InitRootFrustum(rdClipFrustum* pFrustum, float width, float height)
{
    *pFrustum = *rdCamera_g_pCurCamera->pFrustum;
    pFrustum->orthoLeftPlane   = 0.0f;
    pFrustum->orthoTopPlane    = 0.0f;
    pFrustum->orthoRightPlane  = width;
    pFrustum->orthoBottomPlane = height;
}

static bool PVS_PortalChainVisible(const rdClipFrustum* pFrustum, const SithSurface* pPortalSurface)
{
    rdPrimit3 src = { 0 };
    rdVector3 lookDir;

    if (!pPortalSurface || pPortalSurface->face.numVertices < 3)
    {
        return false;
    }

    rdVector_Sub3(
        &lookDir,
        &sithCamera_g_pCurCamera->lookPos,
        &sithWorld_g_pCurrentWorld->aVertices[pPortalSurface->face.aVertices[0]]
    );
    if (rdVector_Dot3(&pPortalSurface->face.normal, &lookDir) <= 0.0f)
    {
        return false;
    }

    sithRender_BuildVisibleSurface((SithSurface*)pPortalSurface);

    src.numVertices = pPortalSurface->face.numVertices;
    src.aVertices   = sithWorld_g_pCurrentWorld->aTransformedVertices;
    src.aVertIdxs   = pPortalSurface->face.aVertices;

    g_PVSClippedFace.aVertices = g_PVSClipVerts;
    if (rdClip_ClipFacePVS((rdClipFrustum*)pFrustum, &src, &g_PVSClippedFace) < 3)
    {
        return false;
    }

    g_PVSProjectedCount = g_PVSClippedFace.numVertices;
    rdCamera_g_pCurCamera->pfProjectList(g_PVSProjectedVerts, g_PVSClipVerts, g_PVSProjectedCount);
    return true;
}

static bool PVS_BuildChildFrustum(
    rdClipFrustum* pChild,
    const rdClipFrustum* pParent,
    const SithSurface* pPortalSurface
)
{
    J3D_UNUSED(pPortalSurface);

    if (g_PVSProjectedCount < 3)
    {
        return false;
    }

    *pChild = *pParent;

    for (size_t i = 0; i < g_PVSProjectedCount; ++i)
    {
        float x = g_PVSProjectedVerts[i].x;
        float y = g_PVSProjectedVerts[i].y;

        if (pChild->orthoLeftPlane   > x) pChild->orthoLeftPlane   = x;
        if (pChild->orthoRightPlane  < x) pChild->orthoRightPlane  = x;
        if (pChild->orthoTopPlane    > y) pChild->orthoTopPlane    = y;
        if (pChild->orthoBottomPlane < y) pChild->orthoBottomPlane = y;
    }

    rdCamera_SetFrustrum(
        rdCamera_g_pCurCamera,
        pChild,
        (int)(ceilf(pChild->orthoLeftPlane) + 0.5f),
        (int)(ceilf(pChild->orthoTopPlane) + 0.5f),
        (int)ceilf(pChild->orthoRightPlane),
        (int)ceilf(pChild->orthoBottomPlane)
    );
    return true;
}

static size_t PVS_CompressAdjoinTable(
    SithWorld* pWorld,
    const uint8_t* aAdjoinTable,
    SithSector* pSourceSector
)
{
    size_t offset = pWorld->sizePVS;
    size_t i = 0;

    while (i < pWorld->numAdjoins)
    {
        size_t zeroRun = 0;
        while (i + zeroRun < pWorld->numAdjoins
            && aAdjoinTable[i + zeroRun] == 0
            && zeroRun < (0x7F + 3))
        {
            ++zeroRun;
        }

        if (zeroRun >= 3)
        {
            pWorld->aPVS = (uint8_t*)STDREALLOC(pWorld->aPVS, pWorld->sizePVS + 1);
            pWorld->aPVS[pWorld->sizePVS++] = (uint8_t)(zeroRun - 3);
            i += zeroRun;
            continue;
        }

        size_t litStart = i;
        size_t litRun = 0;
        while (i + litRun < pWorld->numAdjoins && litRun < 0x80)
        {
            if (litRun >= 1
                && i + litRun + 2 < pWorld->numAdjoins
                && aAdjoinTable[i + litRun] == 0
                && aAdjoinTable[i + litRun + 1] == 0
                && aAdjoinTable[i + litRun + 2] == 0)
            {
                break;
            }

            ++litRun;
        }

        pWorld->aPVS = (uint8_t*)STDREALLOC(pWorld->aPVS, pWorld->sizePVS + 1 + litRun);
        pWorld->aPVS[pWorld->sizePVS++] = (uint8_t)(0x80 | (litRun - 1));
        memcpy(&pWorld->aPVS[pWorld->sizePVS], &aAdjoinTable[litStart], litRun);
        pWorld->sizePVS += litRun;
        i += litRun;
    }

    pSourceSector->pvsIdx = (int)offset;
    return offset;
}
```

### What "generate PVS" means in practice today

For OpenJones3D as it currently exists, "generate PVS" means one of these:

- use an external/editor-side tool that computes the packed PVS blob and sector `pvsIdx` values
- import world data that already contains valid PVS
- inspect or rewrite existing PVS through the loader/writer paths in [`sithPVS.c`](../../Libs/sith/World/sithPVS.c)

What the runtime itself can do in-tree today is:

- read PVS from text or binary world data
- write existing PVS back out
- decode the packed tables for rendering
- enable or disable PVS culling at runtime

It does not currently expose a built-in authoring command that derives fresh PVS sets from arbitrary sector geometry.

## World Validation

The load path ends with [`sithWorld_ValidateWorld()`](../../Libs/sith/World/sithWorld.c#L1034), which verifies that the world contains the minimum set of sections and resources the runtime expects.

This validation step matters because many runtime systems assume their tables exist. By validating once during load, the engine avoids scattering missing-resource checks across every frame path.

## World Teardown

[`sithWorld_Free()`](../../Libs/sith/World/sithWorld.c#L589) tears the world down in ownership order. It frees:

- PVS
- things
- sectors
- models
- sprites
- particles
- keyframes
- thing templates
- runtime render arrays
- surfaces
- materials
- sounds
- cogs and cog scripts
- puppet classes
- AI classes
- sound classes

This is another strong sign that `SithWorld` is the root runtime container. If a subsystem's per-world state lives in the active level, the world teardown path is where it is ultimately reclaimed.

## Related Docs

- Resource-format syntax is documented in [../Formats/README.md](../Formats/README.md).
- Runtime update order for things, physics, and collision is documented in [Frame-Loop-And-Simulation.md](Frame-Loop-And-Simulation.md).
- Render-facing visibility and PVS use are documented in [Rendering-And-Visibility.md](Rendering-And-Visibility.md).

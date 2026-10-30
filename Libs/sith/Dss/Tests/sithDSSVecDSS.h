// Cases for the sithDSS.c functions (see sithDSSVec.h).
#ifndef SITHDSSVECDSS_H
#define SITHDSSVECDSS_H

#include "sithDSSVecRun.h"

#define DSSV_A      (&dssv_arena)
#define DSSV_VAR    (cx->c->var)
#define DSSV_IDTO   ((DPID)(0x1100 + (unsigned)DSSV_VAR))
#define DSSV_STREAM ((unsigned)(0x20 + (unsigned)DSSV_VAR))
#define DSSV_MSG    (&cx->inMsg)

// ---------------------------------------------------------------------------------------------------------------
// Surfaces and sectors
// ---------------------------------------------------------------------------------------------------------------
static int dssvc_SurfaceStatus(DssvCtx* cx)
{
    dssv_Begin(cx);
    return DSSV_FN(sithDSS_SurfaceStatus)(&DSSV_A->surfaces[DSSV_VAR], DSSV_IDTO, DSSV_STREAM);
}

static int dssvc_ProcessSurfaceStatus(DssvCtx* cx)
{
    dssv_Begin(cx);
    return DSSV_FN(sithDSS_ProcessSurfaceStatus)(DSSV_MSG);
}

// var 0..3: sector index; 8: sector 2 with every flag set and extreme light and thrust values
static int dssvc_SectorStatus(DssvCtx* cx)
{
    SithSector* sec = &DSSV_A->sectors[DSSV_VAR & 3];
    if ( DSSV_VAR == 8 )
    {
        sec->flags              = (SithSectorFlag)0xFFFFFFFFu;
        sec->ambientLight.red   = dssv_BitsF(DSSV_FLTMAX);
        sec->ambientLight.green = dssv_BitsF(DSSV_NFLTMAX);
        sec->ambientLight.blue  = dssv_BitsF(DSSV_DENORMMIN);
        sec->extraLight.red     = dssv_BitsF(DSSV_NZERO);
        sec->extraLight.green   = dssv_BitsF(DSSV_FLTMIN);
        sec->extraLight.blue    = dssv_BitsF(DSSV_NDENORMMAX);
        sec->thrust.x           = dssv_BitsF(DSSV_NFLTMAX);
        sec->thrust.y           = dssv_BitsF(DSSV_DENORMMIN);
        sec->thrust.z           = dssv_BitsF(DSSV_FLTMAX);
    }
    dssv_Begin(cx);
    return DSSV_FN(sithDSS_SectorStatus)(sec, DSSV_IDTO, DSSV_STREAM);
}

static int dssvc_ProcessSectorStatus(DssvCtx* cx)
{
    dssv_Begin(cx);
    return DSSV_FN(sithDSS_ProcessSectorStatus)(DSSV_MSG);
}

static int dssvc_SectorFlags(DssvCtx* cx)
{
    dssv_Begin(cx);
    return DSSV_FN(sithDSS_SectorFlags)(&DSSV_A->sectors[DSSV_VAR], DSSV_IDTO, DSSV_STREAM);
}

static int dssvc_ProcessSectorFlags(DssvCtx* cx)
{
    dssv_Begin(cx);
    return DSSV_FN(sithDSS_ProcessSectorFlags)(DSSV_MSG);
}

static int dssvc_SyncSeenSectors(DssvCtx* cx)
{
    if ( DSSV_VAR == 1 ) { sithWorld_g_pCurrentWorld = NULL; }
    dssv_Begin(cx);
    return DSSV_FN(sithDSS_SyncSeenSectors)(DSSV_IDTO, DSSV_STREAM);
}

static int dssvc_ProcessSyncSeenSecors(DssvCtx* cx)
{
    if ( DSSV_VAR == 1 ) { sithWorld_g_pCurrentWorld = NULL; }
    dssv_Begin(cx);
    return DSSV_FN(sithDSS_ProcessSyncSeenSecors)(DSSV_MSG);
}

// ---------------------------------------------------------------------------------------------------------------
// AI status
// ---------------------------------------------------------------------------------------------------------------
// var bits: 1 moving, 2 turning, 4 fleeing, 8 no frames, 16 no goal/target thing
static int dssvc_AIStatus(DssvCtx* cx)
{
    SithAIControlBlock* ai = &DSSV_A->aiBlocks[0];
    if ( DSSV_VAR & 1 ) { ai->mode |= SITHAI_MODE_MOVING; }
    if ( DSSV_VAR & 2 ) { ai->mode |= SITHAI_MODE_TURNING; }
    if ( DSSV_VAR & 4 ) { ai->mode |= SITHAI_MODE_FLEEING; }
    if ( DSSV_VAR & 8 ) { ai->numFrames = 0; }
    if ( DSSV_VAR & 16 ) { ai->goalThing = NULL; ai->pTargetThing = NULL; ai->pFleeFromThing = &DSSV_A->things[6]; }
    if ( DSSV_VAR & 32 ) { ai->pClass = &DSSV_A->aiClasses[1]; ai->numInstincts = 5; }
    if ( DSSV_VAR & 64 ) // extreme integer and float fields
    {
        ai->submode         = (SithAISubMode)0x80000000u;
        ai->msecNextUpdate  = 0xFFFFFFFFu;
        ai->msecAttackStart = 0x7FFFFFFF;
        ai->moveSpeed       = dssv_BitsF(DSSV_FLTMAX);
        ai->homePos.x       = dssv_BitsF(DSSV_NFLTMAX);
        ai->homePos.y       = dssv_BitsF(DSSV_DENORMMIN);
        ai->homePos.z       = dssv_BitsF(DSSV_NZERO);
        ai->maxHomeDist     = dssv_BitsF(DSSV_NDENORMMAX);
    }
    dssv_Begin(cx);
    return DSSV_FN(sithDSS_AIStatus)(ai, DSSV_IDTO, DSSV_STREAM);
}

// var 1: the allocation of the frames fails
static int dssvc_ProcessAIStatus(DssvCtx* cx)
{
    if ( DSSV_VAR == 1 ) { dssv_bAllocFail = 1; }
    dssv_Begin(cx);
    return DSSV_FN(sithDSS_ProcessAIStatus)(DSSV_MSG);
}

// ---------------------------------------------------------------------------------------------------------------
// Inventory
// ---------------------------------------------------------------------------------------------------------------
static const struct { unsigned thing, id; } dssv_aInventoryArgs[] = { { 0, 0 }, { 0, 17 }, { 1, 199 }, { 3, 5 }, { 6, 3 }, { 7, 1 }, { 0, 199 } };

// var 6: the last item of the player with extreme amount and status
static int dssvc_Inventory(DssvCtx* cx)
{
    if ( DSSV_VAR == 6 )
    {
        SithInventoryItem* item = &DSSV_A->things[0].thingInfo.actorInfo.pPlayer->aItems[199];
        item->amount = dssv_BitsF(DSSV_NFLTMAX);
        item->status = (SithInventoryItemStatus)0xFFFFFFFFu;
    }
    dssv_Begin(cx);
    return DSSV_FN(sithDSS_Inventory)(&DSSV_A->things[dssv_aInventoryArgs[DSSV_VAR].thing], dssv_aInventoryArgs[DSSV_VAR].id, DSSV_IDTO, DSSV_STREAM);
}

static int dssvc_ProcessInventory(DssvCtx* cx)
{
    dssv_Begin(cx);
    return DSSV_FN(sithDSS_ProcessInventory)(DSSV_MSG);
}

// ---------------------------------------------------------------------------------------------------------------
// Animation status
// ---------------------------------------------------------------------------------------------------------------
static const uint32_t dssv_aAnimFlags[] = {
    SITHANIMATE_THING | SITHANIMATE_THING_MOVE,                                          // 0
    SITHANIMATE_SPRITE | SITHANIMATE_CAMERA_ZOOM,                                        // 1
    SITHANIMATE_THING_MOVEPOS | SITHANIMATE_LOOP,                                        // 2
    SITHANIMATE_PUSHITEM,                                                                // 3
    SITHANIMATE_PULLITEM | SITHANIMATE_NOSYNC,                                           // 4
    SITHANIMATE_SPRITE_SIZE,                                                             // 5
    SITHANIMATE_THING_QUICKTURN,                                                         // 6
    SITHANIMATE_SURFACE,                                                                 // 7
    SITHANIMATE_SCROLL,                                                                  // 8
    SITHANIMATE_PAGEFLIP | SITHANIMATE_LOOP,                                             // 9
    SITHANIMATE_LIGHT,                                                                   // 10
    SITHANIMATE_MATERIAL,                                                                // 11
    SITHANIMATE_SECTOR,                                                                  // 12
    SITHANIMATE_THING_FADE,                                                              // 13
    SITHANIMATE_MATERIAL | SITHANIMATE_SURFACE | SITHANIMATE_SCROLL | SITHANIMATE_PAGEFLIP | SITHANIMATE_LIGHT | SITHANIMATE_SECTOR, // 14
    0,                                                                                   // 15: flags only
    SITHANIMATE_THING | SITHANIMATE_SPRITE | SITHANIMATE_CAMERA_ZOOM | SITHANIMATE_THING_MOVE | SITHANIMATE_THING_MOVEPOS
        | SITHANIMATE_PUSHITEM | SITHANIMATE_PULLITEM | SITHANIMATE_SPRITE_SIZE | SITHANIMATE_THING_QUICKTURN | SITHANIMATE_SURFACE
        | SITHANIMATE_SCROLL | SITHANIMATE_PAGEFLIP | SITHANIMATE_LIGHT | SITHANIMATE_MATERIAL | SITHANIMATE_SECTOR | SITHANIMATE_THING_FADE // 16: all
};

// var: index into dssv_aAnimFlags, slot var % 5. Slot 4 belongs to a sprite thing, slot 5 of the world is the spare for new animations
static int dssvc_AnimStatus(DssvCtx* cx)
{
    SithAnimationSlot* an = &DSSV_A->anims[DSSV_VAR % DSSV_NUMANIMS];
    an->flags = (SithAnimateFlags)dssv_aAnimFlags[DSSV_VAR];
    dssv_Begin(cx);
    return DSSV_FN(sithDSS_AnimStatus)(an, DSSV_IDTO, DSSV_STREAM);
}

static int dssvc_ProcessAnimStatus(DssvCtx* cx)
{
    dssv_Begin(cx);
    return DSSV_FN(sithDSS_ProcessAnimStatus)(DSSV_MSG);
}

// ---------------------------------------------------------------------------------------------------------------
// Puppet status
// ---------------------------------------------------------------------------------------------------------------
// var % 3 selects the thing (0, 1, 4), var / 3 the variant:
//  0 plain, 1 other known callbacks, 2 unknown callback, 3 no puppet state, 4 short track list, 5 no live tracks
static int dssvc_PuppetStatus(DssvCtx* cx)
{
    static const unsigned things[3] = { 0, 1, 4 };
    SithThing* t = &DSSV_A->things[things[DSSV_VAR % 3]];
    rdPuppet* pup = t->renderData.pPuppet;
    switch ( DSSV_VAR / 3 )
    {
        case 1:
            pup->aTracks[1].pfCallback = (rdPuppetTrackCallback)sithPlayerControls_PuppetCallback_ADDR;
            pup->aTracks[4].pfCallback = (rdPuppetTrackCallback)sithVehicleControls_PuppetCallback_ADDR;
            pup->aTracks[6].pfCallback = (rdPuppetTrackCallback)sithWhip_WhipFirePuppetCallback_ADDR;
            break;
        case 2:
            pup->aTracks[4].pfCallback = (rdPuppetTrackCallback)(uintptr_t)0x00ABCDE0;
            break;
        case 3:
            t->pPuppetState = NULL;
            break;
        case 4:
            t->pPuppetState->pFirstTrack->pNextTrack = NULL;
            break;
        case 5:
            for ( unsigned i = 0; i < RDPUPPET_MAX_TRACKS; ++i ) { pup->aTracks[i].status = (rdPuppetTrackStatus)0; }
            break;
    }
    dssv_Begin(cx);
    return DSSV_FN(sithDSS_PuppetStatus)(t, DSSV_IDTO, DSSV_STREAM);
}

// var 1: sithPuppet_NewTrack fails
static int dssvc_ProcessPuppetStatus(DssvCtx* cx)
{
    if ( DSSV_VAR == 1 ) { dssv_newTrackResult = 1; }
    dssv_Begin(cx);
    return DSSV_FN(sithDSS_ProcessPuppetStatus)(DSSV_MSG);
}

// ---------------------------------------------------------------------------------------------------------------
// Task events, cameras, game state, vehicles
// ---------------------------------------------------------------------------------------------------------------
// var 0: event in the future, 1: event time is before the game time, 2: event time equals the game time
static int dssvc_SyncTaskEvents(DssvCtx* cx)
{
    sithTime_g_msecGameTime = 100000;
    DSSV_A->event.msecEventTime = DSSV_VAR == 0 ? 100000 + 2500 : (DSSV_VAR == 1 ? 100000 - 4000 : 100000);
    dssv_Begin(cx);
    return DSSV_FN(sithDSS_SyncTaskEvents)(&DSSV_A->event, DSSV_IDTO, DSSV_STREAM);
}

static int dssvc_ProcessSyncTaskEvents(DssvCtx* cx)
{
    dssv_Begin(cx);
    return DSSV_FN(sithDSS_ProcessSyncTaskEvents)(DSSV_MSG);
}

// var: index of the current camera (0, 3 or 6 for 0..2); 3: camera 6 with extreme cycle number and deltas
static int dssvc_SyncCameras(DssvCtx* cx)
{
    sithCamera_g_pCurCamera = &sithCamera_g_aCameras[DSSV_VAR == 0 ? 0 : (DSSV_VAR == 1 ? 3 : 6)];
    if ( DSSV_VAR == 3 )
    {
        sithCamera_g_curCycleCamNum   = 0x80000000u;
        sithCamera_g_cameraPosDelta   = dssv_BitsF(DSSV_FLTMAX);
        sithCamera_g_cameraAngleDelta = dssv_BitsF(DSSV_DENORMMIN);
        sithCamera_g_aCameras[6].fov  = dssv_BitsF(DSSV_NFLTMAX);
    }
    dssv_Begin(cx);
    return DSSV_FN(sithDSS_SyncCameras)(DSSV_IDTO, DSSV_STREAM);
}

static int dssvc_ProcessSyncCameras(DssvCtx* cx)
{
    dssv_Begin(cx);
    return DSSV_FN(sithDSS_ProcessSyncCameras)(DSSV_MSG);
}

// var 0: everything set, 1: no master cog, ledge model, vehicle or chalk marks, 2: ten chalk marks
static int dssvc_SyncGameState(DssvCtx* cx)
{
    if ( DSSV_VAR == 1 )
    {
        sithCog_g_pMasterCog = NULL;
        sithPlayerActions_g_pCurHitLedgeModel = NULL;
        sithPlayerActions_g_pCurHitLedgeSurface = NULL;
        sithPlayerActions_g_pPlasma = NULL;
        dssv_pVehicleBoarded = NULL;
        sithFX_g_numChalkMarks = 0;
    }
    else if ( DSSV_VAR == 2 )
    {
        sithFX_g_numChalkMarks = 10;
        for ( unsigned i = 0; i < 10; ++i ) { sithFX_g_aChalkMarks[i] = &DSSV_A->things[i % 6]; }
    }
    dssv_Begin(cx);
    return DSSV_FN(sithDSS_SyncGameState)(DSSV_IDTO, DSSV_STREAM);
}

static int dssvc_ProcessSyncGameState(DssvCtx* cx)
{
    dssv_Begin(cx);
    return DSSV_FN(sithDSS_ProcessSyncGameState)(DSSV_MSG);
}

static int dssvc_SyncVehicleControlState(DssvCtx* cx)
{
    dssv_Begin(cx);
    return DSSV_FN(sithDSS_SyncVehicleControlState)(DSSV_IDTO, DSSV_STREAM);
}

static int dssvc_ProcessVehicleControlsState(DssvCtx* cx)
{
    dssv_Begin(cx);
    return DSSV_FN(sithDSS_ProcessVehicleControlsState)(DSSV_MSG);
}

static int dssvc_sub_4B3760(DssvCtx* cx)
{
    dssv_Begin(cx);
    return DSSV_FN(sithDSS_SendEndSection)(DSSV_IDTO, DSSV_STREAM);
}

static int dssvc_sub_4B3790(DssvCtx* cx)
{
    dssv_Begin(cx);
    return DSSV_FN(sithDSS_ProcessEndSection)(DSSV_MSG);
}

// ---------------------------------------------------------------------------------------------------------------
// Case table. Reader cases are fed with the message of the writer case named in src, edited by mut.
// ---------------------------------------------------------------------------------------------------------------
#define DSSV_NOMUT { 0 }
static const DssvCase dssv_aDSSCases[] = {
    // Surfaces: with and without material and adjoin, 3 and 4 vertices
    { "SurfaceStatus_s0", dssvc_SurfaceStatus, 0, NULL, DSSV_NOMUT },
    { "SurfaceStatus_s1", dssvc_SurfaceStatus, 1, NULL, DSSV_NOMUT },
    { "SurfaceStatus_s2", dssvc_SurfaceStatus, 2, NULL, DSSV_NOMUT },
    { "SurfaceStatus_s3_nomat", dssvc_SurfaceStatus, 3, NULL, DSSV_NOMUT },
    { "SurfaceStatus_s4", dssvc_SurfaceStatus, 4, NULL, DSSV_NOMUT },
    { "SurfaceStatus_s5_nomat", dssvc_SurfaceStatus, 5, NULL, DSSV_NOMUT },
    { "ProcessSurfaceStatus_s0", dssvc_ProcessSurfaceStatus, 0, "SurfaceStatus_s0", DSSV_NOMUT },
    { "ProcessSurfaceStatus_s1", dssvc_ProcessSurfaceStatus, 0, "SurfaceStatus_s1", DSSV_NOMUT },
    { "ProcessSurfaceStatus_s2", dssvc_ProcessSurfaceStatus, 0, "SurfaceStatus_s2", DSSV_NOMUT },
    { "ProcessSurfaceStatus_s3_nomat", dssvc_ProcessSurfaceStatus, 0, "SurfaceStatus_s3_nomat", DSSV_NOMUT },
    { "ProcessSurfaceStatus_s4", dssvc_ProcessSurfaceStatus, 0, "SurfaceStatus_s4", DSSV_NOMUT },
    // The writer's material number 0xFFFFFFFF (none) is read back as NULL and the cel becomes -1
    { "ProcessSurfaceStatus_s5_nomat", dssvc_ProcessSurfaceStatus, 0, "SurfaceStatus_s5_nomat", DSSV_NOMUT },
    // Index at and past the end of the surface array
    { "ProcessSurfaceStatus_index6", dssvc_ProcessSurfaceStatus, 0, "SurfaceStatus_s2", { { 0, 2, 6 } } },
    { "ProcessSurfaceStatus_indexFFFF", dssvc_ProcessSurfaceStatus, 0, "SurfaceStatus_s1", { { 0, 2, 0xFFFF } } },
    // Static world material, cel number inside and outside of the material's cels, unset cel
    { "ProcessSurfaceStatus_staticmat", dssvc_ProcessSurfaceStatus, 0, "SurfaceStatus_s0", { { 6, 4, 0x8001 }, { 10, 2, 3 } } },
    { "ProcessSurfaceStatus_celOutOfRange", dssvc_ProcessSurfaceStatus, 0, "SurfaceStatus_s1", { { 10, 2, 7 } } },
    { "ProcessSurfaceStatus_celFFFF", dssvc_ProcessSurfaceStatus, 0, "SurfaceStatus_s2", { { 10, 2, 0xFFFF } } },
    { "ProcessSurfaceStatus_matIndex2", dssvc_ProcessSurfaceStatus, 0, "SurfaceStatus_s4", { { 6, 4, 2 }, { 10, 2, 1 } } },
    { "ProcessSurfaceStatus_matIndex9", dssvc_ProcessSurfaceStatus, 0, "SurfaceStatus_s4", { { 6, 4, 9 } } },
    // Edge values: all flag bits and +-FLT_MAX texture offsets, denormal/-0 light, extreme face modes
    { "ProcessSurfaceStatus_edge_maxflags_maxfloats", dssvc_ProcessSurfaceStatus, 0, "SurfaceStatus_s0", { { 2, 4, 0xFFFFFFFF }, { 12, 4, DSSV_FLTMAX }, { 16, 4, DSSV_NFLTMAX } } },
    { "ProcessSurfaceStatus_edge_denormal_light", dssvc_ProcessSurfaceStatus, 0, "SurfaceStatus_s1", { { 20, 4, DSSV_DENORMMIN }, { 24, 4, DSSV_NZERO }, { 28, 4, DSSV_NDENORMMAX } } },
    { "ProcessSurfaceStatus_edge_facemodes", dssvc_ProcessSurfaceStatus, 0, "SurfaceStatus_s2", { { 32, 4, 0xFFFFFFFF }, { 36, 4, 0x80000000 }, { 40, 4, 0x7FFFFFFF } } },
    // Material index -2, past the static materials, INT_MAX; cel 0 and 0x8000 (read unsigned, beyond the cels)
    { "ProcessSurfaceStatus_edge_matIndexMinus2", dssvc_ProcessSurfaceStatus, 0, "SurfaceStatus_s4", { { 6, 4, 0xFFFFFFFE } } },
    { "ProcessSurfaceStatus_edge_matIndexStaticPastEnd", dssvc_ProcessSurfaceStatus, 0, "SurfaceStatus_s4", { { 6, 4, 0x8002 } } },
    { "ProcessSurfaceStatus_edge_matIndexIntMax", dssvc_ProcessSurfaceStatus, 0, "SurfaceStatus_s4", { { 6, 4, 0x7FFFFFFF } } },
    { "ProcessSurfaceStatus_edge_cel0", dssvc_ProcessSurfaceStatus, 0, "SurfaceStatus_s1", { { 10, 2, 0 } } },
    { "ProcessSurfaceStatus_edge_cel8000", dssvc_ProcessSurfaceStatus, 0, "SurfaceStatus_s1", { { 10, 2, 0x8000 } } },

    // Sectors: thrust and adjoins off flags
    { "SectorStatus_s0_thrust", dssvc_SectorStatus, 0, NULL, DSSV_NOMUT },
    { "SectorStatus_s1_adjoinsoff", dssvc_SectorStatus, 1, NULL, DSSV_NOMUT },
    { "SectorStatus_s2", dssvc_SectorStatus, 2, NULL, DSSV_NOMUT },
    { "SectorStatus_s3_thrust_adjoinsoff", dssvc_SectorStatus, 3, NULL, DSSV_NOMUT },
    // Every flag bit set (thrust included), +-FLT_MAX, denormal and -0 lights and thrust
    { "SectorStatus_edge_s2_allflags_extremes", dssvc_SectorStatus, 8, NULL, DSSV_NOMUT },
    { "ProcessSectorStatus_s0", dssvc_ProcessSectorStatus, 0, "SectorStatus_s0_thrust", DSSV_NOMUT },
    { "ProcessSectorStatus_s1", dssvc_ProcessSectorStatus, 0, "SectorStatus_s1_adjoinsoff", DSSV_NOMUT },
    { "ProcessSectorStatus_s2", dssvc_ProcessSectorStatus, 0, "SectorStatus_s2", DSSV_NOMUT },
    { "ProcessSectorStatus_s3", dssvc_ProcessSectorStatus, 0, "SectorStatus_s3_thrust_adjoinsoff", DSSV_NOMUT },
    { "ProcessSectorStatus_edge_s2_allflags_extremes", dssvc_ProcessSectorStatus, 0, "SectorStatus_edge_s2_allflags_extremes", DSSV_NOMUT },
    { "ProcessSectorStatus_edge_lights", dssvc_ProcessSectorStatus, 0, "SectorStatus_s0_thrust", { { 6, 4, DSSV_FLTMAX }, { 18, 4, DSSV_NDENORMMAX }, { 30, 4, DSSV_NZERO } } },
    // Adjoins switched off (flag 0x80) and on by the message; thrust flag set without the thrust vector in the message would over-read, so
    // the flags are only edited together with the sector they came from
    { "ProcessSectorStatus_hideadjoins", dssvc_ProcessSectorStatus, 0, "SectorStatus_s2", { { 2, 4, 0x80 } } },
    { "ProcessSectorStatus_showadjoins", dssvc_ProcessSectorStatus, 0, "SectorStatus_s1_adjoinsoff", { { 2, 4, 0x00 } } },
    { "ProcessSectorStatus_index4", dssvc_ProcessSectorStatus, 0, "SectorStatus_s2", { { 0, 2, 4 } } },
    { "ProcessSectorStatus_indexFFFF", dssvc_ProcessSectorStatus, 0, "SectorStatus_s2", { { 0, 2, 0xFFFF } } },
    { "SectorFlags_s0", dssvc_SectorFlags, 0, NULL, DSSV_NOMUT },
    { "SectorFlags_s1_adjoinsoff", dssvc_SectorFlags, 1, NULL, DSSV_NOMUT },
    { "SectorFlags_s2", dssvc_SectorFlags, 2, NULL, DSSV_NOMUT },
    { "SectorFlags_s3", dssvc_SectorFlags, 3, NULL, DSSV_NOMUT },
    { "ProcessSectorFlags_s0", dssvc_ProcessSectorFlags, 0, "SectorFlags_s0", DSSV_NOMUT },
    { "ProcessSectorFlags_s1", dssvc_ProcessSectorFlags, 0, "SectorFlags_s1_adjoinsoff", DSSV_NOMUT },
    { "ProcessSectorFlags_hideadjoins", dssvc_ProcessSectorFlags, 0, "SectorFlags_s2", { { 2, 4, 0x80 } } },
    { "ProcessSectorFlags_showadjoins", dssvc_ProcessSectorFlags, 0, "SectorFlags_s1_adjoinsoff", { { 2, 4, 0x10 } } },
    { "ProcessSectorFlags_index4", dssvc_ProcessSectorFlags, 0, "SectorFlags_s2", { { 0, 2, 4 } } },
    { "ProcessSectorFlags_indexFFFF", dssvc_ProcessSectorFlags, 0, "SectorFlags_s2", { { 0, 2, 0xFFFF } } },
    // All flag bits (adjoins switched off) and only the top bit
    { "ProcessSectorFlags_edge_allbits", dssvc_ProcessSectorFlags, 0, "SectorFlags_s2", { { 2, 4, 0xFFFFFFFF } } },
    { "ProcessSectorFlags_edge_topbit", dssvc_ProcessSectorFlags, 0, "SectorFlags_s1_adjoinsoff", { { 2, 4, 0x80000000 } } },
    { "ProcessSectorFlags_edge_lastsector", dssvc_ProcessSectorFlags, 0, "SectorFlags_s2", { { 0, 2, 3 } } },
    { "SyncSeenSectors", dssvc_SyncSeenSectors, 0, NULL, DSSV_NOMUT },
    { "SyncSeenSectors_noworld", dssvc_SyncSeenSectors, 1, NULL, DSSV_NOMUT },
    { "ProcessSyncSeenSecors", dssvc_ProcessSyncSeenSecors, 0, "SyncSeenSectors", DSSV_NOMUT },
    { "ProcessSyncSeenSecors_allseen", dssvc_ProcessSyncSeenSecors, 0, "SyncSeenSectors", { { 0, 4, 0x01010101 } } },
    { "ProcessSyncSeenSecors_noneseen", dssvc_ProcessSyncSeenSecors, 0, "SyncSeenSectors", { { 0, 4, 0 } } },
    // Any nonzero byte marks a sector seen
    { "ProcessSyncSeenSecors_edge_FF", dssvc_ProcessSyncSeenSecors, 0, "SyncSeenSectors", { { 0, 4, 0xFFFFFFFF } } },
    { "ProcessSyncSeenSecors_edge_mixed", dssvc_ProcessSyncSeenSecors, 0, "SyncSeenSectors", { { 0, 4, 0x80FF0002 } } },
    { "ProcessSyncSeenSecors_noworld", dssvc_ProcessSyncSeenSecors, 1, "SyncSeenSectors", DSSV_NOMUT },

    // AI status: every mode flag that adds data, no frames, no goal/target
    { "AIStatus_plain", dssvc_AIStatus, 0, NULL, DSSV_NOMUT },
    { "AIStatus_moving", dssvc_AIStatus, 1, NULL, DSSV_NOMUT },
    { "AIStatus_turning", dssvc_AIStatus, 2, NULL, DSSV_NOMUT },
    { "AIStatus_fleeing", dssvc_AIStatus, 4, NULL, DSSV_NOMUT },
    { "AIStatus_moving_turning_fleeing", dssvc_AIStatus, 7, NULL, DSSV_NOMUT },
    { "AIStatus_noframes", dssvc_AIStatus, 8, NULL, DSSV_NOMUT },
    { "AIStatus_nogoalthing", dssvc_AIStatus, 16, NULL, DSSV_NOMUT },
    { "AIStatus_all_nogoal", dssvc_AIStatus, 23, NULL, DSSV_NOMUT },
    { "AIStatus_class1_5instincts", dssvc_AIStatus, 32, NULL, DSSV_NOMUT },
    { "AIStatus_edge_extremes", dssvc_AIStatus, 64 | 7, NULL, DSSV_NOMUT },
    { "ProcessAIStatus_plain", dssvc_ProcessAIStatus, 0, "AIStatus_plain", DSSV_NOMUT },
    { "ProcessAIStatus_moving", dssvc_ProcessAIStatus, 0, "AIStatus_moving", DSSV_NOMUT },
    { "ProcessAIStatus_turning", dssvc_ProcessAIStatus, 0, "AIStatus_turning", DSSV_NOMUT },
    { "ProcessAIStatus_fleeing", dssvc_ProcessAIStatus, 0, "AIStatus_fleeing", DSSV_NOMUT },
    { "ProcessAIStatus_moving_turning_fleeing", dssvc_ProcessAIStatus, 0, "AIStatus_moving_turning_fleeing", DSSV_NOMUT },
    { "ProcessAIStatus_noframes", dssvc_ProcessAIStatus, 0, "AIStatus_noframes", DSSV_NOMUT },
    { "ProcessAIStatus_nogoalthing", dssvc_ProcessAIStatus, 0, "AIStatus_nogoalthing", DSSV_NOMUT },
    { "ProcessAIStatus_class1_5instincts", dssvc_ProcessAIStatus, 0, "AIStatus_class1_5instincts", DSSV_NOMUT },
    { "ProcessAIStatus_edge_extremes", dssvc_ProcessAIStatus, 0, "AIStatus_edge_extremes", DSSV_NOMUT },
    // Edge values: INT_MIN/UINT_MAX/INT_MAX ints, +-FLT_MAX and denormal floats, goal and target index 0xFFFF,
    // every mode bit on a message that already has the moving, turning and fleeing fields
    { "ProcessAIStatus_edge_ints", dssvc_ProcessAIStatus, 0, "AIStatus_plain", { { 8, 4, 0x80000000 }, { 12, 4, 0xFFFFFFFF }, { 42, 4, 0x7FFFFFFF } } },
    { "ProcessAIStatus_edge_floats", dssvc_ProcessAIStatus, 0, "AIStatus_plain", { { 18, 4, DSSV_FLTMAX }, { 22, 4, DSSV_NFLTMAX }, { 76, 4, DSSV_DENORMMIN } } },
    { "ProcessAIStatus_edge_goaltargetFFFF", dssvc_ProcessAIStatus, 0, "AIStatus_plain", { { 16, 2, 0xFFFF }, { 46, 2, 0xFFFF } } },
    { "ProcessAIStatus_edge_modeallbits", dssvc_ProcessAIStatus, 0, "AIStatus_moving_turning_fleeing", { { 4, 4, 0xFFFFFFFF } } },
    { "ProcessAIStatus_edge_classFFFF", dssvc_ProcessAIStatus, 0, "AIStatus_plain", { { 2, 2, 0xFFFF } } },
    { "ProcessAIStatus_allocfail", dssvc_ProcessAIStatus, 1, "AIStatus_plain", DSSV_NOMUT },
    { "ProcessAIStatus_badthing", dssvc_ProcessAIStatus, 0, "AIStatus_plain", { { 0, 2, 100 } } },
    { "ProcessAIStatus_nonaithing", dssvc_ProcessAIStatus, 0, "AIStatus_plain", { { 0, 2, 0 } } },
    { "ProcessAIStatus_badclass", dssvc_ProcessAIStatus, 0, "AIStatus_plain", { { 2, 2, 2 } } },

    // Inventory
    { "Inventory_player_0", dssvc_Inventory, 0, NULL, DSSV_NOMUT },
    { "Inventory_player_17", dssvc_Inventory, 1, NULL, DSSV_NOMUT },
    { "Inventory_actor_199", dssvc_Inventory, 2, NULL, DSSV_NOMUT },
    { "Inventory_weapon_rejected", dssvc_Inventory, 3, NULL, DSSV_NOMUT },
    { "Inventory_freething_rejected", dssvc_Inventory, 4, NULL, DSSV_NOMUT },
    { "Inventory_corpse_rejected", dssvc_Inventory, 5, NULL, DSSV_NOMUT },
    { "Inventory_edge_player_199_extremes", dssvc_Inventory, 6, NULL, DSSV_NOMUT },
    { "ProcessInventory_player_0", dssvc_ProcessInventory, 0, "Inventory_player_0", DSSV_NOMUT },
    { "ProcessInventory_player_17", dssvc_ProcessInventory, 0, "Inventory_player_17", DSSV_NOMUT },
    { "ProcessInventory_actor_199", dssvc_ProcessInventory, 0, "Inventory_actor_199", DSSV_NOMUT },
    { "ProcessInventory_weapon", dssvc_ProcessInventory, 0, "Inventory_player_0", { { 0, 2, 3 } } },
    { "ProcessInventory_corpse", dssvc_ProcessInventory, 0, "Inventory_player_0", { { 0, 2, 7 } } },
    { "ProcessInventory_badthing", dssvc_ProcessInventory, 0, "Inventory_player_0", { { 0, 2, 200 } } },
    { "ProcessInventory_id200", dssvc_ProcessInventory, 0, "Inventory_player_0", { { 2, 2, 200 } } },
    { "ProcessInventory_idFFFF", dssvc_ProcessInventory, 0, "Inventory_player_0", { { 2, 2, 0xFFFF } } },
    { "ProcessInventory_edge_player_199_extremes", dssvc_ProcessInventory, 0, "Inventory_edge_player_199_extremes", DSSV_NOMUT },
    // Last item id, first invalid id, +FLT_MAX/denormal amounts with INT_MIN status, thing index 0xFFFF
    { "ProcessInventory_edge_id199", dssvc_ProcessInventory, 0, "Inventory_player_0", { { 2, 2, 199 } } },
    { "ProcessInventory_edge_maxamount", dssvc_ProcessInventory, 0, "Inventory_player_17", { { 4, 4, DSSV_FLTMAX }, { 8, 4, 0x80000000 } } },
    { "ProcessInventory_edge_denormalamount", dssvc_ProcessInventory, 0, "Inventory_player_17", { { 4, 4, DSSV_NDENORMMAX }, { 8, 4, 0x7FFFFFFF } } },
    { "ProcessInventory_edge_thingFFFF", dssvc_ProcessInventory, 0, "Inventory_player_0", { { 0, 2, 0xFFFF } } },

    // Animation status: one case per flag group, all flags, no flags
    { "AnimStatus_thing_move", dssvc_AnimStatus, 0, NULL, DSSV_NOMUT },
    { "AnimStatus_sprite_camerazoom", dssvc_AnimStatus, 1, NULL, DSSV_NOMUT },
    { "AnimStatus_thing_movepos", dssvc_AnimStatus, 2, NULL, DSSV_NOMUT },
    { "AnimStatus_pushitem", dssvc_AnimStatus, 3, NULL, DSSV_NOMUT },
    { "AnimStatus_pullitem", dssvc_AnimStatus, 4, NULL, DSSV_NOMUT },
    { "AnimStatus_spritesize", dssvc_AnimStatus, 5, NULL, DSSV_NOMUT },
    { "AnimStatus_quickturn", dssvc_AnimStatus, 6, NULL, DSSV_NOMUT },
    { "AnimStatus_surface", dssvc_AnimStatus, 7, NULL, DSSV_NOMUT },
    { "AnimStatus_scroll", dssvc_AnimStatus, 8, NULL, DSSV_NOMUT },
    { "AnimStatus_pageflip", dssvc_AnimStatus, 9, NULL, DSSV_NOMUT },
    { "AnimStatus_light", dssvc_AnimStatus, 10, NULL, DSSV_NOMUT },
    { "AnimStatus_material", dssvc_AnimStatus, 11, NULL, DSSV_NOMUT },
    { "AnimStatus_sector", dssvc_AnimStatus, 12, NULL, DSSV_NOMUT },
    { "AnimStatus_thing_fade", dssvc_AnimStatus, 13, NULL, DSSV_NOMUT },
    { "AnimStatus_material_surface_scroll_pageflip_light_sector", dssvc_AnimStatus, 14, NULL, DSSV_NOMUT },
    { "AnimStatus_noflags", dssvc_AnimStatus, 15, NULL, DSSV_NOMUT },
    { "AnimStatus_allflags", dssvc_AnimStatus, 16, NULL, DSSV_NOMUT },
    { "ProcessAnimStatus_thing_move", dssvc_ProcessAnimStatus, 0, "AnimStatus_thing_move", DSSV_NOMUT },
    { "ProcessAnimStatus_sprite_camerazoom", dssvc_ProcessAnimStatus, 0, "AnimStatus_sprite_camerazoom", DSSV_NOMUT },
    { "ProcessAnimStatus_thing_movepos", dssvc_ProcessAnimStatus, 0, "AnimStatus_thing_movepos", DSSV_NOMUT },
    { "ProcessAnimStatus_pushitem", dssvc_ProcessAnimStatus, 0, "AnimStatus_pushitem", DSSV_NOMUT },
    { "ProcessAnimStatus_pullitem", dssvc_ProcessAnimStatus, 0, "AnimStatus_pullitem", DSSV_NOMUT },
    { "ProcessAnimStatus_spritesize", dssvc_ProcessAnimStatus, 0, "AnimStatus_spritesize", DSSV_NOMUT },
    { "ProcessAnimStatus_quickturn", dssvc_ProcessAnimStatus, 0, "AnimStatus_quickturn", DSSV_NOMUT },
    { "ProcessAnimStatus_surface", dssvc_ProcessAnimStatus, 0, "AnimStatus_surface", DSSV_NOMUT },
    { "ProcessAnimStatus_scroll", dssvc_ProcessAnimStatus, 0, "AnimStatus_scroll", DSSV_NOMUT },
    { "ProcessAnimStatus_pageflip", dssvc_ProcessAnimStatus, 0, "AnimStatus_pageflip", DSSV_NOMUT },
    { "ProcessAnimStatus_light", dssvc_ProcessAnimStatus, 0, "AnimStatus_light", DSSV_NOMUT },
    { "ProcessAnimStatus_material", dssvc_ProcessAnimStatus, 0, "AnimStatus_material", DSSV_NOMUT },
    { "ProcessAnimStatus_sector", dssvc_ProcessAnimStatus, 0, "AnimStatus_sector", DSSV_NOMUT },
    { "ProcessAnimStatus_thing_fade", dssvc_ProcessAnimStatus, 0, "AnimStatus_thing_fade", DSSV_NOMUT },
    { "ProcessAnimStatus_material_surface_scroll_pageflip_light_sector", dssvc_ProcessAnimStatus, 0, "AnimStatus_material_surface_scroll_pageflip_light_sector", DSSV_NOMUT },
    // flags == 0 stops the animation
    { "ProcessAnimStatus_noflags", dssvc_ProcessAnimStatus, 0, "AnimStatus_noflags", DSSV_NOMUT },
    { "ProcessAnimStatus_allflags", dssvc_ProcessAnimStatus, 0, "AnimStatus_allflags", DSSV_NOMUT },
    // Unknown animation id: a new slot is created
    { "ProcessAnimStatus_newslot", dssvc_ProcessAnimStatus, 0, "AnimStatus_thing_fade", { { 0, 4, 0x777 } } },
    { "ProcessAnimStatus_newslot_stop", dssvc_ProcessAnimStatus, 0, "AnimStatus_noflags", { { 0, 4, 0x777 } } },
    // Thing index and surface index that do not exist
    { "ProcessAnimStatus_badthing", dssvc_ProcessAnimStatus, 0, "AnimStatus_sprite_camerazoom", { { 8, 2, 0x7FFF } } },
    { "ProcessAnimStatus_spritething", dssvc_ProcessAnimStatus, 0, "AnimStatus_sprite_camerazoom", { { 8, 2, 5 } } },
    { "ProcessAnimStatus_badsurface", dssvc_ProcessAnimStatus, 0, "AnimStatus_surface", { { 8, 4, 77 } } },
    // Negative and extreme indices: int16 thing index -1 and -32768, int32 surface/sector/material/thing indices,
    // animation id -1 (a new slot), extreme page flip times and FLT_MAX camera zoom values
    { "ProcessAnimStatus_edge_thingMinus1", dssvc_ProcessAnimStatus, 0, "AnimStatus_sprite_camerazoom", { { 8, 2, 0xFFFF } } },
    { "ProcessAnimStatus_edge_thingMinus32768", dssvc_ProcessAnimStatus, 0, "AnimStatus_sprite_camerazoom", { { 8, 2, 0x8000 } } },
    { "ProcessAnimStatus_edge_zoomfloats", dssvc_ProcessAnimStatus, 0, "AnimStatus_sprite_camerazoom", { { 14, 4, DSSV_FLTMAX }, { 18, 4, DSSV_NFLTMAX }, { 22, 4, DSSV_DENORMMIN } } },
    { "ProcessAnimStatus_edge_surfaceMinus1", dssvc_ProcessAnimStatus, 0, "AnimStatus_surface", { { 8, 4, 0xFFFFFFFF } } },
    { "ProcessAnimStatus_edge_surfaceIntMin", dssvc_ProcessAnimStatus, 0, "AnimStatus_surface", { { 8, 4, 0x80000000 } } },
    { "ProcessAnimStatus_edge_sectorMinus1", dssvc_ProcessAnimStatus, 0, "AnimStatus_sector", { { 8, 4, 0xFFFFFFFF } } },
    { "ProcessAnimStatus_edge_sectorIntMax", dssvc_ProcessAnimStatus, 0, "AnimStatus_sector", { { 8, 4, 0x7FFFFFFF } } },
    { "ProcessAnimStatus_edge_materialMinus1", dssvc_ProcessAnimStatus, 0, "AnimStatus_material", { { 8, 4, 0xFFFFFFFF } } },
    { "ProcessAnimStatus_edge_materialStatic0", dssvc_ProcessAnimStatus, 0, "AnimStatus_material", { { 8, 4, 0x8000 } } },
    { "ProcessAnimStatus_edge_materialIntMax", dssvc_ProcessAnimStatus, 0, "AnimStatus_material", { { 8, 4, 0x7FFFFFFF } } },
    { "ProcessAnimStatus_edge_fadethingMinus1", dssvc_ProcessAnimStatus, 0, "AnimStatus_thing_fade", { { 8, 4, 0xFFFFFFFF } } },
    { "ProcessAnimStatus_edge_fadethingIntMin", dssvc_ProcessAnimStatus, 0, "AnimStatus_thing_fade", { { 8, 4, 0x80000000 } } },
    { "ProcessAnimStatus_edge_pageflipints", dssvc_ProcessAnimStatus, 0, "AnimStatus_pageflip", { { 8, 4, 0xFFFFFFFF }, { 12, 4, 0x80000000 }, { 16, 4, 0x7FFFFFFF } } },
    { "ProcessAnimStatus_edge_animidMinus1", dssvc_ProcessAnimStatus, 0, "AnimStatus_thing_fade", { { 0, 4, 0xFFFFFFFF } } },

    // Puppet status: three things times six variants
    { "PuppetStatus_t0", dssvc_PuppetStatus, 0, NULL, DSSV_NOMUT },
    { "PuppetStatus_t1", dssvc_PuppetStatus, 1, NULL, DSSV_NOMUT },
    { "PuppetStatus_t4", dssvc_PuppetStatus, 2, NULL, DSSV_NOMUT },
    { "PuppetStatus_t0_callbacks", dssvc_PuppetStatus, 3, NULL, DSSV_NOMUT },
    { "PuppetStatus_t1_callbacks", dssvc_PuppetStatus, 4, NULL, DSSV_NOMUT },
    { "PuppetStatus_t4_unknowncallback", dssvc_PuppetStatus, 8, NULL, DSSV_NOMUT },
    { "PuppetStatus_t0_nostate", dssvc_PuppetStatus, 9, NULL, DSSV_NOMUT },
    { "PuppetStatus_t1_shorttracklist", dssvc_PuppetStatus, 13, NULL, DSSV_NOMUT },
    { "PuppetStatus_t0_notracks", dssvc_PuppetStatus, 15, NULL, DSSV_NOMUT },
    { "ProcessPuppetStatus_t0", dssvc_ProcessPuppetStatus, 0, "PuppetStatus_t0", DSSV_NOMUT },
    { "ProcessPuppetStatus_t1", dssvc_ProcessPuppetStatus, 0, "PuppetStatus_t1", DSSV_NOMUT },
    { "ProcessPuppetStatus_t4", dssvc_ProcessPuppetStatus, 0, "PuppetStatus_t4", DSSV_NOMUT },
    { "ProcessPuppetStatus_t0_callbacks", dssvc_ProcessPuppetStatus, 0, "PuppetStatus_t0_callbacks", DSSV_NOMUT },
    { "ProcessPuppetStatus_t1_callbacks", dssvc_ProcessPuppetStatus, 0, "PuppetStatus_t1_callbacks", DSSV_NOMUT },
    { "ProcessPuppetStatus_t4_unknowncallback", dssvc_ProcessPuppetStatus, 0, "PuppetStatus_t4_unknowncallback", DSSV_NOMUT },
    { "ProcessPuppetStatus_t1_shorttracklist", dssvc_ProcessPuppetStatus, 0, "PuppetStatus_t1_shorttracklist", DSSV_NOMUT },
    { "ProcessPuppetStatus_t0_notracks", dssvc_ProcessPuppetStatus, 0, "PuppetStatus_t0_notracks", DSSV_NOMUT },
    { "ProcessPuppetStatus_newtrackfails", dssvc_ProcessPuppetStatus, 1, "PuppetStatus_t0", DSSV_NOMUT },
    { "ProcessPuppetStatus_badthing", dssvc_ProcessPuppetStatus, 0, "PuppetStatus_t0", { { 0, 4, 6 } } },
    { "ProcessPuppetStatus_thingwithoutpuppet", dssvc_ProcessPuppetStatus, 0, "PuppetStatus_t0", { { 0, 4, 2 } } },
    // Track fields at their limits: keyframe -1, priorities 0x7FFF/-32768, +-FLT_MAX and denormal frame values,
    // a denormal playback speed (above zero, kept by both), callback index 255; thing index -1
    { "ProcessPuppetStatus_edge_keyframeMinus1_priorities", dssvc_ProcessPuppetStatus, 0, "PuppetStatus_t0", { { 12, 4, 0xFFFFFFFF }, { 20, 4, 0x80007FFF } } },
    { "ProcessPuppetStatus_edge_floats", dssvc_ProcessPuppetStatus, 0, "PuppetStatus_t0", { { 24, 4, DSSV_FLTMAX }, { 28, 4, DSSV_NFLTMAX }, { 32, 4, DSSV_NDENORMMAX } } },
    { "ProcessPuppetStatus_edge_denormalspeed_callback255", dssvc_ProcessPuppetStatus, 0, "PuppetStatus_t0", { { 16, 4, DSSV_DENORMMIN }, { 40, 1, 0xFF } } },
    { "ProcessPuppetStatus_edge_thingMinus1", dssvc_ProcessPuppetStatus, 0, "PuppetStatus_t0", { { 0, 4, 0xFFFFFFFF } } },
    // The original keeps a playback speed of zero or less, the reconstruction replaces it with 1.0 (known difference)
    { "ProcessPuppetStatus_zerospeed", dssvc_ProcessPuppetStatus, 0, "PuppetStatus_t0", { { 16, 4, 0x00000000 } } },
    { "ProcessPuppetStatus_negativespeed", dssvc_ProcessPuppetStatus, 0, "PuppetStatus_t0", { { 16, 4, 0xBF800000 } } },

    // Task events
    { "SyncTaskEvents_future", dssvc_SyncTaskEvents, 0, NULL, DSSV_NOMUT },
    { "SyncTaskEvents_past", dssvc_SyncTaskEvents, 1, NULL, DSSV_NOMUT },
    { "SyncTaskEvents_edge_now", dssvc_SyncTaskEvents, 2, NULL, DSSV_NOMUT },
    { "ProcessSyncTaskEvents_future", dssvc_ProcessSyncTaskEvents, 0, "SyncTaskEvents_future", DSSV_NOMUT },
    { "ProcessSyncTaskEvents_past", dssvc_ProcessSyncTaskEvents, 0, "SyncTaskEvents_past", DSSV_NOMUT },
    { "ProcessSyncTaskEvents_edge_now", dssvc_ProcessSyncTaskEvents, 0, "SyncTaskEvents_edge_now", DSSV_NOMUT },
    // Delay UINT_MAX, event index INT_MIN, task number 0xFFFF; zero delay with INT_MAX parameters
    { "ProcessSyncTaskEvents_edge_extremes", dssvc_ProcessSyncTaskEvents, 0, "SyncTaskEvents_future", { { 0, 4, 0xFFFFFFFF }, { 4, 4, 0x80000000 }, { 20, 2, 0xFFFF } } },
    { "ProcessSyncTaskEvents_edge_zerodelay", dssvc_ProcessSyncTaskEvents, 0, "SyncTaskEvents_future", { { 0, 4, 0 }, { 8, 4, 0x7FFFFFFF }, { 16, 4, 0xFFFFFFFF } } },

    // Cameras
    { "SyncCameras_cam0", dssvc_SyncCameras, 0, NULL, DSSV_NOMUT },
    { "SyncCameras_cam3", dssvc_SyncCameras, 1, NULL, DSSV_NOMUT },
    { "SyncCameras_cam6", dssvc_SyncCameras, 2, NULL, DSSV_NOMUT },
    { "SyncCameras_edge_extremes", dssvc_SyncCameras, 3, NULL, DSSV_NOMUT },
    { "ProcessSyncCameras_cam0", dssvc_ProcessSyncCameras, 0, "SyncCameras_cam0", DSSV_NOMUT },
    { "ProcessSyncCameras_cam3", dssvc_ProcessSyncCameras, 0, "SyncCameras_cam3", DSSV_NOMUT },
    { "ProcessSyncCameras_cam6", dssvc_ProcessSyncCameras, 0, "SyncCameras_cam6", DSSV_NOMUT },
    // Camera numbers past the last camera are clamped
    { "ProcessSyncCameras_clamp", dssvc_ProcessSyncCameras, 0, "SyncCameras_cam3", { { 0, 2, 9 } } },
    { "ProcessSyncCameras_clampFFFF", dssvc_ProcessSyncCameras, 0, "SyncCameras_cam3", { { 0, 2, 0xFFFF } } },
    { "ProcessSyncCameras_edge_extremes", dssvc_ProcessSyncCameras, 0, "SyncCameras_edge_extremes", DSSV_NOMUT },
    // Focus thing indices INT_MIN/INT_MAX, FLT_MAX field of view; all bits in the camera set flag, -0/denormal deltas
    { "ProcessSyncCameras_edge_focusindices", dssvc_ProcessSyncCameras, 0, "SyncCameras_cam3", { { 42, 4, 0x80000000 }, { 46, 4, 0x7FFFFFFF }, { 50, 4, DSSV_FLTMAX } } },
    { "ProcessSyncCameras_edge_globals", dssvc_ProcessSyncCameras, 0, "SyncCameras_cam0", { { 2, 4, 0xFFFFFFFF }, { 34, 4, DSSV_NZERO }, { 38, 4, DSSV_NDENORMMAX } } },

    // Game state
    { "SyncGameState_full", dssvc_SyncGameState, 0, NULL, DSSV_NOMUT },
    { "SyncGameState_empty", dssvc_SyncGameState, 1, NULL, DSSV_NOMUT },
    { "SyncGameState_tenchalkmarks", dssvc_SyncGameState, 2, NULL, DSSV_NOMUT },
    { "ProcessSyncGameState_full", dssvc_ProcessSyncGameState, 0, "SyncGameState_full", DSSV_NOMUT },
    { "ProcessSyncGameState_empty", dssvc_ProcessSyncGameState, 0, "SyncGameState_empty", DSSV_NOMUT },
    { "ProcessSyncGameState_tenchalkmarks", dssvc_ProcessSyncGameState, 0, "SyncGameState_tenchalkmarks", DSSV_NOMUT },
    // Master cog index INT_MAX/INT_MIN/-2 with extreme current item and weapon ids
    { "ProcessSyncGameState_edge_cogIntMax", dssvc_ProcessSyncGameState, 0, "SyncGameState_full", { { 0, 4, 0x7FFFFFFF }, { 4, 4, 0x80000000 }, { 8, 4, 0xFFFFFFFF } } },
    { "ProcessSyncGameState_edge_cogIntMin", dssvc_ProcessSyncGameState, 0, "SyncGameState_empty", { { 0, 4, 0x80000000 }, { 4, 4, 0x7FFFFFFF } } },
    { "ProcessSyncGameState_edge_cogMinus2", dssvc_ProcessSyncGameState, 0, "SyncGameState_full", { { 0, 4, 0xFFFFFFFE } } },

    // Vehicle controls
    { "SyncVehicleControlState", dssvc_SyncVehicleControlState, 0, NULL, DSSV_NOMUT },
    { "ProcessVehicleControlsState", dssvc_ProcessVehicleControlsState, 0, "SyncVehicleControlState", DSSV_NOMUT },
    // +-FLT_MAX, denormal and -0 floats, INT_MIN/UINT_MAX states
    { "ProcessVehicleControlsState_edge_minecar", dssvc_ProcessVehicleControlsState, 0, "SyncVehicleControlState", { { 0, 4, DSSV_FLTMAX }, { 4, 4, 0x80000000 }, { 8, 4, DSSV_DENORMMIN } } },
    { "ProcessVehicleControlsState_edge_raft", dssvc_ProcessVehicleControlsState, 0, "SyncVehicleControlState", { { 44, 4, 0xFFFFFFFF }, { 52, 4, DSSV_NFLTMAX }, { 92, 4, DSSV_NZERO } } },

    // End of section message
    { "sub_4B3760", dssvc_sub_4B3760, 0, NULL, DSSV_NOMUT },
    { "sub_4B3790", dssvc_sub_4B3790, 0, "sub_4B3760", DSSV_NOMUT },
};
#define DSSV_NUMDSSCASES (sizeof(dssv_aDSSCases) / sizeof(dssv_aDSSCases[0]))

#endif // SITHDSSVECDSS_H

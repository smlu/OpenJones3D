// Engine call stubs for sithDSS and the case runner shared by the vector harness and the tests.
#ifndef SITHDSSVECRUN_H
#define SITHDSSVECRUN_H

#include "sithDSSVecWorld.h"

// ---------------------------------------------------------------------------------------------------------------
// Case descriptor and runner
// ---------------------------------------------------------------------------------------------------------------
struct DssvCtx;

// Overwrites len (1..4) bytes at data offset off of the message a reader case is fed
typedef struct
{
    uint16_t off, len;
    uint32_t val;
} DssvMut;

typedef struct
{
    const char* name;
    int (*pfn)(struct DssvCtx*);
    int var;          // selects the state/argument variant inside the case function
    const char* src;  // reader cases: name of the writer case whose message is fed to the reader
    DssvMut mut[3];   // reader cases: edits applied to that message
} DssvCase;

typedef struct DssvCtx
{
    const DssvCase* c;
    SithMessage inMsg; // message handed to readers
    int bReader;
    DssvBuf pre;
} DssvCtx;

// Message handed to a reader case: type, length and payload (stored in the fixtures)
typedef struct
{
    uint16_t type;
    uint32_t length;
    const uint8_t* data;
} DssvMsgIn;

// ---------------------------------------------------------------------------------------------------------------
// Stubs of the engine functions the DSS functions call. In the capture harness the original functions are patched to
// jump to these, the tests rename the calls of the tested sources to them (see seams.def).
// ---------------------------------------------------------------------------------------------------------------
static void dssv_EvWords(DssvEvent id, const void* p, size_t size)
{
    dssvB_u8(&dssv_ev, (uint32_t)id);
    for ( size_t k = 0; k + 4 <= size; k += 4 )
    {
        uint32_t w;
        memcpy(&w, (const uint8_t*)p + k, 4);
        dssvB_u32(&dssv_ev, dssv_NormWord(w));
    }
}

DSSV_STUB int J3DAPI dssv_sithMessage_SendMessage(SithMessage* pMsg, DPID idTo, SithMessageStream outstream, uint32_t dwDPFlags)
{
#ifdef DSSV_TRACE
    DSSV_TRACE("SendMessage(%p type=%u len=%u idTo=%x stream=%x flags=%x)\n", (void*)pMsg, pMsg->type, pMsg->length, (unsigned)idTo, (unsigned)outstream, dwDPFlags);
#endif
    dssvB_u8(&dssv_ev, DSSV_EV_SEND);
    dssvB_u16(&dssv_ev, pMsg->type);
    dssvB_u32(&dssv_ev, pMsg->length);
    dssvB_u32(&dssv_ev, (uint32_t)idTo);
    dssvB_u32(&dssv_ev, (uint32_t)outstream);
    dssvB_u32(&dssv_ev, dwDPFlags);

    // The writers must not touch anything behind the payload
    int bClean = 1;
    if ( pMsg->length <= sizeof(pMsg->data) )
    {
        for ( size_t i = pMsg->length; i < sizeof(pMsg->data); ++i )
        {
            if ( pMsg->data[i] != (uint8_t)dssv_fill ) { bClean = 0; break; }
        }
    }
    dssvB_u8(&dssv_ev, (uint32_t)bClean);
    dssv_msgTailClean = bClean;
    if ( pMsg->length <= sizeof(pMsg->data) )
    {
        dssvB_put(&dssv_ev, pMsg->data, pMsg->length);
        dssv_lastMsg.valid  = 1;
        dssv_lastMsg.type   = pMsg->type;
        dssv_lastMsg.length = pMsg->length;
        memcpy(dssv_lastMsg.data, pMsg->data, pMsg->length);
    }
    return dssv_sendResult;
}

DSSV_STUB DPID J3DAPI dssv_sithMessage_GetSenderID(void) { return dssv_senderId; }
DSSV_STUB int J3DAPI dssv_stdComm_IsGameHost(void) { return dssv_bGameHost; }
DSSV_STUB int J3DAPI dssv_stdComm_IsGameActive(void) { return dssv_bGameActive; }

DSSV_STUB void J3DAPI dssv_sithSector_HideSectorAdjoins(SithSector* pSector) { dssv_Ev(DSSV_EV_HIDEADJOINS, 1, DSSV_P(pSector)); }
DSSV_STUB void J3DAPI dssv_sithSector_ShowSectorAdjoins(SithSector* pSector) { dssv_Ev(DSSV_EV_SHOWADJOINS, 1, DSSV_P(pSector)); }

DSSV_STUB const SithMineCarControlsState* J3DAPI dssv_sithVehicleControls_GetMineCarState(void) { return &dssv_arena.mineCar; }
DSSV_STUB const SithRaftControlsState* J3DAPI dssv_sithVehicleControls_GetRaftState(void) { return &dssv_arena.raft; }
DSSV_STUB void J3DAPI dssv_sithVehicleControls_SetMineCarState(const SithMineCarControlsState* pState) { dssv_EvWords(DSSV_EV_SETMINECAR, pState, sizeof(*pState)); }
DSSV_STUB void J3DAPI dssv_sithVehicleControls_SetRaftState(const SithRaftControlsState* pState) { dssv_EvWords(DSSV_EV_SETRAFT, pState, sizeof(*pState)); }

static SithThing* dssv_pVehicleBoarded;
DSSV_STUB SithThing* J3DAPI dssv_sithPlayerControls_GetVehicleBoardedThing() { return dssv_pVehicleBoarded; }
DSSV_STUB void J3DAPI dssv_sithPlayerControls_SetVehicleBoardedThing(SithThing* pThing)
{
    dssv_pVehicleBoarded = pThing;
    dssv_Ev(DSSV_EV_SETVEHICLEBOARDED, 1, DSSV_P(pThing));
}

static int dssv_newTrackResult;
DSSV_STUB void J3DAPI dssv_sithPuppet_SetMoveMode(SithThing* pThing, SithPuppetMoveMode newMode) { dssv_Ev(DSSV_EV_SETMOVEMODE, 2, DSSV_P(pThing), (uint32_t)newMode); }
DSSV_STUB void J3DAPI dssv_sithPuppet_SetArmedMode(SithThing* pThing, unsigned int newMode) { dssv_Ev(DSSV_EV_SETARMEDMODE, 2, DSSV_P(pThing), newMode); }
DSSV_STUB int J3DAPI dssv_sithPuppet_NewTrack(SithThing* pThing, SithPuppetClassSubmode* pSubmode, int trackNum, SithPuppetSubMode submode)
{
    dssv_Ev(DSSV_EV_NEWTRACK, 4, DSSV_P(pThing), (uint32_t)(pSubmode - &dssv_arena.puppetClass.aModes[0][0]), (uint32_t)trackNum, (uint32_t)submode);
    return dssv_newTrackResult;
}
DSSV_STUB void J3DAPI dssv_rdPuppet_NewEntry(rdPuppet* pPuppet, rdThing* pThing) { dssv_Ev(DSSV_EV_PUPPETNEWENTRY, 2, DSSV_P(pPuppet), DSSV_P(pThing)); }

DSSV_STUB int J3DAPI dssv_sithEvent_CreateEvent(size_t taskId, SithEventParams* params, uint32_t when)
{
    dssv_Ev(DSSV_EV_CREATEEVENT, 6, (uint32_t)taskId, (uint32_t)params->idx, (uint32_t)params->param1, (uint32_t)params->param2, (uint32_t)params->param3, when);
    return 0x77;
}

DSSV_STUB SithAnimationSlot* J3DAPI dssv_sithAnimate_GetAnim(int animID)
{
    for ( unsigned i = 0; i < DSSV_NUMANIMS; ++i )
    {
        if ( dssv_arena.anims[i].animID == animID ) { return &dssv_arena.anims[i]; }
    }
    return NULL;
}

DSSV_STUB SithAnimationSlot* dssv_sithAnimate_Create(void)
{
    dssv_Ev(DSSV_EV_ANIMCREATE, 0);
    if ( dssv_bAllocFail ) { return NULL; }
    SithAnimationSlot* p = &dssv_arena.anims[DSSV_NUMANIMS];
    memset(p, 0, sizeof(*p));
    p->animID = 0x5555;
    return p;
}

DSSV_STUB void J3DAPI dssv_sithAnimate_Stop(SithAnimationSlot* pAnim) { dssv_Ev(DSSV_EV_ANIMSTOP, 1, DSSV_P(pAnim)); }

DSSV_STUB void* J3DAPI dssv_stdMemory_Malloc(size_t size, const char* pFilename, size_t line)
{
    (void)pFilename; (void)line;
    dssv_Ev(DSSV_EV_MALLOC, 1, (uint32_t)size);
    ++dssv_numMallocs;
    if ( dssv_bAllocFail || dssv_arena.poolUsed + size > sizeof(dssv_arena.pool) ) { return NULL; }
    void* p = &dssv_arena.pool[dssv_arena.poolUsed];
    dssv_arena.poolUsed += (size + 7) & ~(size_t)7;
    memset(p, (int)dssv_fill, size);
    return p;
}

DSSV_STUB void J3DAPI dssv_stdMemory_Free(void* pBytes) { dssv_Ev(DSSV_EV_FREE, 1, DSSV_P(pBytes)); }

// sithRender_bResetCameraAspect is an int of the exe but a bool of the reconstruction (the global moved to the DLL)
#ifdef DSSV_HARNESS
#define DSSV_RESETCAMERAASPECT (*(int*)sithRender_bResetCameraAspect_ADDR)
#else
#define DSSV_RESETCAMERAASPECT sithRender_bResetCameraAspect
#endif
static int dssv_mirrorResetCameraAspect; // watched copy of the variable above

// ---------------------------------------------------------------------------------------------------------------
// Global state of the engine that the DSS functions read or write. Set to deterministic values for every case.
// ---------------------------------------------------------------------------------------------------------------
static void dssv_BuildGlobals(void)
{
    DssvArena* a = &dssv_arena;

    memset(&sithMulti_g_message, 0, sizeof(sithMulti_g_message));
    sithMulti_g_serverId   = 0x7001;
    sithTime_g_msecGameTime = dssv_Rnd();
    sithMain_g_frameNumber = dssv_Rnd();
    memset(&sithMain_g_sith_mode, 0, sizeof(sithMain_g_sith_mode));

    memset(&sithCamera_g_aCameras[0], 0, sizeof(sithCamera_g_aCameras));
    for ( unsigned i = 0; i < 7; ++i )
    {
        SithCamera* c = &sithCamera_g_aCameras[i];
        RF(c->fov);
        c->pPrimaryFocusThing   = (i & 1) ? &a->things[(i + 1) % 6] : NULL;
        c->pSecondaryFocusThing = (i % 3 == 0) ? NULL : &a->things[i % 6];
        c->pSector              = &a->sectors[i % DSSV_NUMSECTORS];
    }
    sithCamera_g_pCurCamera = &sithCamera_g_aCameras[3];
    RU(sithCamera_g_bCurCameraSet);
    RU(sithCamera_g_curCycleCamNum);
    RV3(sithCamera_g_vecCameraPosOffset);
    RV3(sithCamera_g_vecCameraAngleOffset);
    RF(sithCamera_g_cameraPosDelta);
    RF(sithCamera_g_cameraAngleDelta);

    for ( unsigned i = 0; i < 20; ++i )
    {
        sithInventory_g_aPowerKeybinds[i].bEnabled = dssv_Rnd();
        sithInventory_g_aPowerKeybinds[i].binding = dssv_Rnd();
        sithInventory_g_aPowerKeybinds[i].bActivated = dssv_Rnd();
    }
    RU(sithInventory_g_bSendDeactivateMessage);
    RU(sithInventory_g_bInitInventory);
    RU(sithInventory_g_dword_56B750);
    RU(sithInventory_g_dword_56B754);
    for ( unsigned i = 0; i < 12; ++i )
    {
        sithInventory_g_aTypes[i].pHolsterModel = (i & 1) ? &a->models[i % DSSV_NUMMODELS] : NULL;
    }

    DSSV_RESETCAMERAASPECT = dssv_RndI(2);
    sithPlayerActions_g_pCurCollidedSurface       = &a->surfaces[2];
    sithPlayerActions_g_pCurCollidedModel    = &a->models[1];
    sithPlayerActions_g_pCurCollidedModelFace = &a->modelFaces[1][2];
    sithPlayerActions_g_pPlasma                = &a->things[3];
    RU(sithPlayer_g_bPlayerInPor);
    RF(sithPlayer_g_impState);
    sithPlayer_g_bInAetheriumSector = dssv_RndI(2);
    sithPlayer_g_bGuybrush          = dssv_RndI(2);
    sithPlayer_g_impFireType        = dssv_RndI(1000);
    sithPlayer_g_curLevelNum        = dssv_RndI(30000);
    sithPlayer_g_bBonusMapBought    = dssv_RndI(2);
    sithPlayerActions_g_jewelFlyingPuppetTrackNum = dssv_RndI(30000);
    sithPlayerActions_g_bJewelFlying     = dssv_RndI(2);
    sithPlayerActions_g_bPlayerInvisible = dssv_RndI(2);
    sithPlayerControls_g_bCutsceneMode   = dssv_RndI(2);
    sithPuppet_g_bPlayerLeapForward      = dssv_RndI(2);

    sithFX_g_numChalkMarks    = 3;
    sithFX_g_lastChalkMarkNum = dssv_RndI(100);
    for ( unsigned i = 0; i < 10; ++i )
    {
        sithFX_g_aChalkMarks[i] = i < 3 ? &a->things[i + 1] : NULL;
    }
    sithCog_g_pMasterCog = &a->cogs[2];

    dssv_pVehicleBoarded = &a->things[4];
}

// ---------------------------------------------------------------------------------------------------------------
// Runner
// ---------------------------------------------------------------------------------------------------------------
#define DSSV_W(x) dssv_Watch(&(x), sizeof(x))

// Everything a DSS function may write is watched in every case: the synthetic world and the engine globals
static void dssv_WatchAll(void)
{
    dssv_numWatch = 0;
    DSSV_W(dssv_arena);
    DSSV_W(sithCamera_g_aCameras);
    dssv_Watch(&sithPlayer_g_aPlayers[0], sizeof(sithPlayer_g_aPlayers[0]) * 2);
    DSSV_W(sithInventory_g_aPowerKeybinds);
    dssv_Watch(&sithInventory_g_aTypes[0], sizeof(sithInventory_g_aTypes[0]) * 12);
    DSSV_W(sithCamera_g_pCurCamera);
    DSSV_W(sithCamera_g_bCurCameraSet);
    DSSV_W(sithCamera_g_curCycleCamNum);
    DSSV_W(sithCamera_g_vecCameraPosOffset);
    DSSV_W(sithCamera_g_vecCameraAngleOffset);
    DSSV_W(sithCamera_g_cameraPosDelta);
    DSSV_W(sithCamera_g_cameraAngleDelta);
    DSSV_W(sithInventory_g_bSendDeactivateMessage);
    DSSV_W(sithInventory_g_bInitInventory);
    DSSV_W(sithInventory_g_dword_56B750);
    DSSV_W(sithInventory_g_dword_56B754);
    DSSV_W(dssv_mirrorResetCameraAspect);
    DSSV_W(sithPlayerActions_g_pCurCollidedSurface);
    DSSV_W(sithPlayerActions_g_pCurCollidedModel);
    DSSV_W(sithPlayerActions_g_pCurCollidedModelFace);
    DSSV_W(sithPlayerActions_g_pPlasma);
    DSSV_W(sithPlayer_g_bPlayerInPor);
    DSSV_W(sithPlayer_g_impState);
    DSSV_W(sithPlayer_g_bInAetheriumSector);
    DSSV_W(sithPlayer_g_bGuybrush);
    DSSV_W(sithPlayer_g_impFireType);
    DSSV_W(sithPlayer_g_curLevelNum);
    DSSV_W(sithPlayer_g_bBonusMapBought);
    DSSV_W(sithPlayerActions_g_jewelFlyingPuppetTrackNum);
    DSSV_W(sithPlayerActions_g_bJewelFlying);
    DSSV_W(sithPlayerActions_g_bPlayerInvisible);
    DSSV_W(sithPlayerControls_g_bCutsceneMode);
    DSSV_W(sithPuppet_g_bPlayerLeapForward);
    DSSV_W(sithFX_g_numChalkMarks);
    DSSV_W(sithFX_g_lastChalkMarkNum);
    DSSV_W(sithFX_g_aChalkMarks);
    DSSV_W(sithCog_g_pMasterCog);
    DSSV_W(dssv_pVehicleBoarded);
}

// Called by a case function right before it calls the function under test
static void dssv_Begin(DssvCtx* cx)
{
    memset(&sithMulti_g_message, (int)dssv_fill, sizeof(sithMulti_g_message));
    dssv_WatchAll();
    dssv_mirrorResetCameraAspect = DSSV_RESETCAMERAASPECT;
#ifdef DSSV_TRACE
    for ( unsigned i = 0; i < dssv_numWatch; ++i ) { DSSV_TRACE("watch %u: %p size %u\n", i, dssv_aWatch[i].p, (unsigned)dssv_aWatch[i].size); }
#endif
    dssv_Capture(&cx->pre);
}

static const char* dssv_Name(const DssvCtx* cx) { return cx->c->name; }

// Runs a case and appends its record to rec (cleared first). in is NULL for writer cases.
static void dssv_RunCase(const DssvCase* c, uint32_t fill, const DssvMsgIn* in, DssvBuf* rec)
{
    DssvCtx cx;
    memset(&cx, 0, sizeof(cx));
    cx.c = c;

    dssv_fill        = fill;
    dssv_numAsserts  = dssv_numErrors = dssv_numStatus = dssv_numMallocs = 0;
    dssv_ev.size     = 0;
    dssv_lastMsg.valid = 0;
    dssv_sendResult  = 0x5151;
    dssv_senderId    = 0x7002;
    dssv_bGameHost   = dssv_bGameActive = 0;
    dssv_bAllocFail  = 0;
    dssv_newTrackResult = 0;
    dssv_msgTailClean = 1;

#ifdef DSSV_TRACE
    DSSV_TRACE("run %s fill %02x\n", c->name, fill);
#endif
    dssv_BuildWorld(dssv_Hash(c->name));
#ifdef DSSV_TRACE
    DSSV_TRACE(" world built\n");
#endif
    dssv_BuildGlobals();
#ifdef DSSV_TRACE
    DSSV_TRACE(" globals built\n");
#endif

    memset(&cx.inMsg, (int)fill, sizeof(cx.inMsg));
    if ( in )
    {
        cx.bReader      = 1;
        cx.inMsg.type   = in->type;
        cx.inMsg.length = in->length;
        memcpy(cx.inMsg.data, in->data, in->length);
    }

    int ret = c->pfn(&cx);

    DssvBuf post = { 0 };
    dssv_mirrorResetCameraAspect = DSSV_RESETCAMERAASPECT;
    dssv_Capture(&post);
    if ( cx.pre.size != post.size ) { printf("dssv: watch size changed\n"); }

    rec->size = 0;
    dssvB_u32(rec, (uint32_t)ret);
    dssvB_u16(rec, dssv_numAsserts);
    dssvB_u16(rec, dssv_numErrors);
    dssvB_u16(rec, dssv_numStatus);
    dssvB_u16(rec, dssv_numMallocs);
    dssvB_u32(rec, (uint32_t)dssv_ev.size);
    dssvB_put(rec, dssv_ev.p, dssv_ev.size);
    dssv_Delta(rec, &cx.pre, &post);

    dssvB_free(&cx.pre);
    dssvB_free(&post);
}

#endif // SITHDSSVECRUN_H

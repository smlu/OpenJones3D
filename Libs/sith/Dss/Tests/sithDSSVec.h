// Shared core of the sithDSS / sithDSSThing original-engine test vectors.
//
// This header is compiled twice from the same source:
//  * by the capture harness (DSSV_HARNESS defined), which is injected into the retail v1.0 Indy3D.exe and runs the
//    ORIGINAL functions at their v1.0 addresses, and
//  * by sithDSSTests (Libs/sith/Dss/Tests), which runs the reconstructed functions.
// Both build the same synthetic engine state, route the engine calls the DSS functions make through the same stubs
// and produce a byte record per case. The harness stores the records in the .tv fixtures, the tests compare them.
//
// Record layout of a case (all little endian):
//   u32 return value
//   u16 numAsserts, u16 numErrors, u16 numStatus, u16 numMallocs   (host service calls made during the case)
//   u32 eventsSize, events[]      (calls of the stubbed engine functions, see DssvEvent)
//   u32 numDeltas, delta[]        (u32 offset, u16 size, bytes: changed bytes of the watched memory, pointers normalized)
#ifndef SITHDSSVEC_H
#define SITHDSSVEC_H

#include <j3dcore/j3d.h>

#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <rdroid/Engine/rdPuppet.h>
#include <rdroid/Math/rdMatrix.h>
#include <rdroid/Math/rdVector.h>
#include <rdroid/types.h>

#include <sith/AI/sithAIMove.h>
#include <sith/Cog/sithCog.h>
#include <sith/Devices/sithComm.h>
#include <sith/Devices/sithSoundMixer.h>
#include <sith/Dss/sithDSS.h>
#include <sith/Dss/sithDSSThing.h>
#include <sith/Dss/sithMulti.h>
#include <sith/Engine/sithAnimate.h>
#include <sith/Engine/sithCamera.h>
#include <sith/Engine/sithPathMove.h>
#include <sith/Engine/sithPuppet.h>
#include <sith/Engine/sithRender.h>
#include <sith/Gameplay/sithEvent.h>
#include <sith/Gameplay/sithFX.h>
#include <sith/Gameplay/sithInventory.h>
#include <sith/Gameplay/sithPlayer.h>
#include <sith/Gameplay/sithPlayerActions.h>
#include <sith/Gameplay/sithPlayerControls.h>
#include <sith/Gameplay/sithTime.h>
#include <sith/Gameplay/sithVehicleControls.h>
#include <sith/Gameplay/sithWhip.h>
#include <sith/Main/sithMain.h>
#include <sith/RTI/addresses.h>
#include <sith/RTI/symbols.h>
#include <sith/World/sithActor.h>
#include <sith/World/sithItem.h>
#include <sith/World/sithMaterial.h>
#include <sith/World/sithModel.h>
#include <sith/World/sithSector.h>
#include <sith/World/sithSoundClass.h>
#include <sith/World/sithSurface.h>
#include <sith/World/sithTemplate.h>
#include <sith/World/sithThing.h>
#include <sith/World/sithWeapon.h>
#include <sith/World/sithWorld.h>

#include <sound/Sound.h>

#include <std/General/stdMemory.h>
#include <std/Win95/stdComm.h>

#ifdef DSSV_HARNESS
#define DSSV_STUB static
#else
#define DSSV_STUB
#endif

#ifdef DSSV_HARNESS
#define DSSV_FN(fn) ((fn##_TYPE)fn##_ADDR)
#else
#define DSSV_FN(fn) fn
#endif

// ---------------------------------------------------------------------------------------------------------------
// Growable byte buffer
// ---------------------------------------------------------------------------------------------------------------
typedef struct
{
    uint8_t* p;
    size_t size, cap;
} DssvBuf;

static void dssvB_put(DssvBuf* b, const void* d, size_t n)
{
    if ( b->size + n > b->cap )
    {
        b->cap = (b->size + n) * 2 + 256;
        b->p   = (uint8_t*)realloc(b->p, b->cap);
    }
    if ( d ) { memcpy(b->p + b->size, d, n); }
    else { memset(b->p + b->size, 0, n); }
    b->size += n;
}
static void dssvB_u8(DssvBuf* b, uint32_t v) { uint8_t x = (uint8_t)v; dssvB_put(b, &x, 1); }
static void dssvB_u16(DssvBuf* b, uint32_t v) { uint16_t x = (uint16_t)v; dssvB_put(b, &x, 2); }
static void dssvB_u32(DssvBuf* b, uint32_t v) { dssvB_put(b, &v, 4); }
static void dssvB_free(DssvBuf* b) { free(b->p); b->p = NULL; b->size = b->cap = 0; }

// ---------------------------------------------------------------------------------------------------------------
// Deterministic data
// ---------------------------------------------------------------------------------------------------------------
static uint32_t dssv_rng;
static uint32_t dssv_fill; // garbage byte of the current run (the harness probes every case with two values)

static uint32_t dssv_Hash(const char* s)
{
    uint32_t h = 2166136261u;
    while ( *s ) { h = (h ^ (uint8_t)*s++) * 16777619u; }
    return h;
}

static uint32_t dssv_Rnd(void)
{
    dssv_rng = dssv_rng * 1664525u + 1013904223u;
    return dssv_rng >> 8; // 24 bits
}

// Floats are multiples of 1/16 so every value is exact and has a distinctive bit pattern
static float dssv_RndF(void) { return (float)((int)(dssv_Rnd() % 16384u) - 8192) / 16.0f; }
static int dssv_RndI(unsigned n) { return (int)(dssv_Rnd() % n); }
static void dssv_RndVec3(rdVector3* v) { v->x = dssv_RndF(); v->y = dssv_RndF(); v->z = dssv_RndF(); }
static void dssv_RndVec4(rdVector4* v) { v->x = dssv_RndF(); v->y = dssv_RndF(); v->z = dssv_RndF(); v->w = dssv_RndF(); }

// ---------------------------------------------------------------------------------------------------------------
// Host services (sith_g_pHS / std_g_pHS). Calls are only counted, text is not compared.
// ---------------------------------------------------------------------------------------------------------------
static unsigned dssv_numAsserts, dssv_numErrors, dssv_numStatus, dssv_numMallocs;
static int dssv_bAllocFail; // when set stdMemory_Malloc returns NULL

#define DSSV_MAXALLOCS 64
static void* dssv_apAllocs[DSSV_MAXALLOCS];
static unsigned dssv_numAllocs;

static void J3DAPI dssv_hsAssert(const char* pCond, const char* pFile, int line) { (void)pCond; (void)pFile; (void)line; ++dssv_numAsserts; }
static void dssv_hsStatus(const char* pFmt, ...) { (void)pFmt; ++dssv_numStatus; }
static void dssv_hsMessage(const char* pFmt, ...) { (void)pFmt; }
static void dssv_hsWarning(const char* pFmt, ...) { (void)pFmt; }
static void dssv_hsError(const char* pFmt, ...) { (void)pFmt; ++dssv_numErrors; }
static void dssv_hsDebug(const char* pFmt, ...) { (void)pFmt; }

static tHostServices dssv_hostServices;

static void dssv_InitHostServices(void)
{
    memset(&dssv_hostServices, 0, sizeof(dssv_hostServices));
    dssv_hostServices.pAssert       = (tAssertFunc)dssv_hsAssert;
    dssv_hostServices.pStatusPrint  = (tPrintfFunc)dssv_hsStatus;
    dssv_hostServices.pMessagePrint = (tPrintfFunc)dssv_hsMessage;
    dssv_hostServices.pWarningPrint = (tPrintfFunc)dssv_hsWarning;
    dssv_hostServices.pErrorPrint   = (tPrintfFunc)dssv_hsError;
    dssv_hostServices.pDebugPrint   = (tPrintfFunc)dssv_hsDebug;
}

// ---------------------------------------------------------------------------------------------------------------
// Synthetic engine state
// ---------------------------------------------------------------------------------------------------------------
#define DSSV_NUMSECTORS   4u
#define DSSV_CAPSECTORS   8u
#define DSSV_NUMSURFACES  6u
#define DSSV_CAPSURFACES  10u
#define DSSV_NUMADJOINS   2u
#define DSSV_NUMMATS      3u
#define DSSV_CAPMATS      6u
#define DSSV_NUMSMATS     2u
#define DSSV_NUMTHINGS    8u
#define DSSV_CAPTHINGS    12u
#define DSSV_NUMTEMPLATES 3u
#define DSSV_CAPTEMPLATES 6u
#define DSSV_NUMMODELS    3u
#define DSSV_CAPMODELS    6u
#define DSSV_NUMKEYS      4u
#define DSSV_CAPKEYS      6u
#define DSSV_NUMCOGS      3u
#define DSSV_CAPCOGS      6u
#define DSSV_NUMAICLASSES 2u
#define DSSV_CAPAICLASSES 4u
#define DSSV_NUMANIMS     5u
#define DSSV_NUMSOUNDS    5u

// Guid of thing i
#define DSSV_GUID(i) (0x1000 + (int)(i) * 0x11)

typedef struct
{
    SithWorld world, staticWorld;
    SithSector sectors[DSSV_CAPSECTORS], staticSectors[2];
    SithSurface surfaces[DSSV_CAPSURFACES];
    SithSurfaceAdjoin adjoins[DSSV_NUMADJOINS + 2];
    rdVector4 intens[DSSV_CAPSURFACES][4];
    int faceVerts[DSSV_CAPSURFACES][4];
    rdMaterial mats[DSSV_CAPMATS], staticMats[4];
    SithThing things[DSSV_CAPTHINGS];
    SithThing templates[DSSV_CAPTEMPLATES], staticTemplates[2];
    rdModel3 models[DSSV_CAPMODELS], staticModels[3];
    rdModel3Mesh modelMeshes[DSSV_CAPMODELS + 3];
    rdFace modelFaces[DSSV_CAPMODELS + 3][4];
    rdKeyframe keys[DSSV_CAPKEYS], staticKeys[2];
    SithCog cogs[DSSV_CAPCOGS], staticCogs[2];
    SithAIClass aiClasses[DSSV_CAPAICLASSES];
    SithAIControlBlock aiBlocks[2];
    rdPuppet puppets[DSSV_CAPTHINGS];
    SithPuppetState puppetStates[DSSV_CAPTHINGS];
    SithPuppetTrack puppetTracks[DSSV_CAPTHINGS][4];
    SithPuppetClass puppetClass;
    SithThingSwapEntry swaps[DSSV_CAPTHINGS][3];
    SithAnimationSlot anims[DSSV_NUMANIMS + 1];
    uint8_t pool[2048]; // stdMemory_Malloc allocates from here
    size_t poolUsed;
    SithEvent event;
    SithMineCarControlsState mineCar;
    SithRaftControlsState raft;
    rdSprite3 sprite;
    rdVector3 aiFrames[4];
    SithPathFrame pathFrames[3];
    uint32_t tail[4]; // keeps the arena end distinct
} DssvArena;

static DssvArena dssv_arena;

// The puppet track callbacks the DSS puppet status message can name. The original stores the exe addresses, the
// reconstructed code may store its own function addresses; both are normalized to the callback number.
static const uintptr_t dssv_aCallbackAddrs[7][2] = {
#ifndef DSSV_HARNESS
    { sithPuppet_DefaultCallback_ADDR, (uintptr_t)&sithPuppet_DefaultCallback },
    { sithPlayerControls_PuppetCallback_ADDR, (uintptr_t)&sithPlayerControls_PuppetCallback },
    { sithVehicleControls_PuppetCallback_ADDR, (uintptr_t)&sithVehicleControls_PuppetCallback },
    { sithAIMove_PuppetCallback_ADDR, (uintptr_t)&sithAIMove_PuppetCallback },
    { sithWhip_WhipClimbPuppetCallback_ADDR, (uintptr_t)&sithWhip_WhipClimbPuppetCallback },
    { sithWhip_ClimbDismountPuppetCallback_ADDR, (uintptr_t)&sithWhip_ClimbDismountPuppetCallback },
    { sithWhip_WhipFirePuppetCallback_ADDR, (uintptr_t)&sithWhip_WhipFirePuppetCallback }
#else
    { sithPuppet_DefaultCallback_ADDR, 0 },
    { sithPlayerControls_PuppetCallback_ADDR, 0 },
    { sithVehicleControls_PuppetCallback_ADDR, 0 },
    { sithAIMove_PuppetCallback_ADDR, 0 },
    { sithWhip_WhipClimbPuppetCallback_ADDR, 0 },
    { sithWhip_ClimbDismountPuppetCallback_ADDR, 0 },
    { sithWhip_WhipFirePuppetCallback_ADDR, 0 }
#endif
};

// Normalizes values that are addresses of synthetic state so records do not depend on where the state lives.
// All generated integers and floats are far away from these addresses (checked by dssv_CheckAddressSpace).
static uint32_t dssv_NormWord(uint32_t w)
{
    uintptr_t base = (uintptr_t)&dssv_arena;
    if ( w >= base && w < base + sizeof(dssv_arena) )
    {
        return 0xA5000000u | (uint32_t)(w - base);
    }

    base = (uintptr_t)&sithCamera_g_aCameras[0];
    if ( w >= base && w < base + sizeof(sithCamera_g_aCameras) )
    {
        return 0xA6000000u | (uint32_t)(w - base);
    }

    base = (uintptr_t)&sithPlayer_g_aPlayers[0];
    if ( w >= base && w < base + sizeof(sithPlayer_g_aPlayers) )
    {
        return 0xA7000000u | (uint32_t)(w - base);
    }

    for ( uint32_t i = 0; i < 7; ++i )
    {
        if ( w == dssv_aCallbackAddrs[i][0] || (dssv_aCallbackAddrs[i][1] && w == dssv_aCallbackAddrs[i][1]) )
        {
            return 0xCB000000u | i;
        }
    }
    return w;
}

#define DSSV_MAXWATCH 64
static struct { const void* p; size_t size; } dssv_aWatch[DSSV_MAXWATCH];
static unsigned dssv_numWatch;

static void dssv_Watch(const void* p, size_t size)
{
    dssv_aWatch[dssv_numWatch].p    = p;
    dssv_aWatch[dssv_numWatch].size = size;
    ++dssv_numWatch;
}

// Captures the watched regions (size is padded to 4 and compared as normalized words)
static void dssv_Capture(DssvBuf* out)
{
    out->size = 0;
    for ( unsigned i = 0; i < dssv_numWatch; ++i )
    {
        const uint8_t* p = (const uint8_t*)dssv_aWatch[i].p;
        size_t n = dssv_aWatch[i].size;
        size_t k = 0;
        for ( ; k + 4 <= n; k += 4 )
        {
            uint32_t w;
            memcpy(&w, p + k, 4);
            w = dssv_NormWord(w);
            dssvB_put(out, &w, 4);
        }
        if ( k < n ) { dssvB_put(out, p + k, n - k); }
    }
}

static void dssv_Delta(DssvBuf* rec, const DssvBuf* pre, const DssvBuf* post)
{
    DssvBuf d = { 0 };
    uint32_t numDeltas = 0;
    for ( size_t i = 0; i < post->size; )
    {
        if ( pre->p[i] == post->p[i] ) { ++i; continue; }
        size_t j = i;
        while ( j < post->size && pre->p[j] != post->p[j] && j - i < 0xFFF0 ) { ++j; }
        dssvB_u32(&d, (uint32_t)i);
        dssvB_u16(&d, (uint32_t)(j - i));
        dssvB_put(&d, post->p + i, j - i);
        ++numDeltas;
        i = j;
    }
    dssvB_u32(rec, numDeltas);
    dssvB_put(rec, d.p, d.size);
    dssvB_free(&d);
}

// Tail elements of the arrays hold garbage (the probe fill); fixed up by dssv_BuildWorld
static void dssv_FillTail(void* pArray, size_t elemSize, size_t numUsed, size_t cap)
{
    memset((uint8_t*)pArray + elemSize * numUsed, (int)dssv_fill, elemSize * (cap - numUsed));
}

// Calls recorded by the stubs
static DssvBuf dssv_ev;

typedef enum
{
    DSSV_EV_SEND = 1,
    DSSV_EV_HIDEADJOINS,
    DSSV_EV_SHOWADJOINS,
    DSSV_EV_SETRAFT,
    DSSV_EV_SETMINECAR,
    DSSV_EV_SETVEHICLEBOARDED,
    DSSV_EV_SETMOVEMODE,
    DSSV_EV_SETARMEDMODE,
    DSSV_EV_NEWTRACK,
    DSSV_EV_PUPPETNEWENTRY,
    DSSV_EV_CREATEEVENT,
    DSSV_EV_ANIMSTOP,
    DSSV_EV_ANIMCREATE,
    DSSV_EV_MALLOC,
    DSSV_EV_FREE,
    DSSV_EV_SETSECTOR,
    DSSV_EV_DETACH,
    DSSV_EV_RESETSWAPLIST,
    DSSV_EV_ADDSWAPENTRY,
    DSSV_EV_SETTHINGMODEL,
    DSSV_EV_MODELLOAD,
    DSSV_EV_PLAYSOUNDTHING,
    DSSV_EV_PLAYSOUNDPOS,
    DSSV_EV_PLAYSOUND,
    DSSV_EV_STOPSOUND,
    DSSV_EV_FADEVOLUME,
    DSSV_EV_SOUNDCLASSMODE,
    DSSV_EV_SOUNDCLASSRANDOM,
    DSSV_EV_PUPPETPLAYKEY,
    DSSV_EV_PUPPETPLAYMODE,
    DSSV_EV_PUPPETSTOPKEY,
    DSSV_EV_FIREPROJECTILE,
    DSSV_EV_KILLACTOR,
    DSSV_EV_KILLPLAYER,
    DSSV_EV_DAMAGETHING,
    DSSV_EV_PATHMOVETOFRAME,
    DSSV_EV_PATHMOVESKIP,
    DSSV_EV_PATHMOVEFINISH,
    DSSV_EV_SETITEMTAKEN,
    DSSV_EV_CREATETHING,
    DSSV_EV_CREATETHINGATPOS,
    DSSV_EV_DESTROYTHING,
    DSSV_EV_SETHEADPYR,
    DSSV_EV_PLAYERACTION,
    DSSV_EV_STRAFE
} DssvEvent;

static void dssv_Ev(DssvEvent id, unsigned n, ...)
{
    va_list va;
    va_start(va, n);
    dssvB_u8(&dssv_ev, (uint32_t)id);
    for ( unsigned i = 0; i < n; ++i ) { dssvB_u32(&dssv_ev, dssv_NormWord(va_arg(va, uint32_t))); }
    va_end(va);
}

// ---------------------------------------------------------------------------------------------------------------
// Stub state
// ---------------------------------------------------------------------------------------------------------------
static int dssv_sendResult;         // value returned by sithMessage_SendMessage
static DPID dssv_senderId;          // value returned by sithMessage_GetSenderID
static int dssv_bGameHost, dssv_bGameActive;

// Copy of the last message handed to sithMessage_SendMessage (the harness feeds it to the readers)
static struct
{
    int valid;
    uint16_t type;
    uint32_t length;
    uint8_t data[sizeof(((SithMessage*)0)->data)];
} dssv_lastMsg;

static int dssv_msgTailClean;

// ---------------------------------------------------------------------------------------------------------------
// Lookup helper stubs. They reproduce the original lookups on the synthetic world. In the harness the originals run
// unpatched and the stubs are verified against them.
// ---------------------------------------------------------------------------------------------------------------
DSSV_STUB SithThing* J3DAPI dssv_sithThing_GetThingByIndex(int idx)
{
    SithWorld* pW = sithWorld_g_pCurrentWorld;
    if ( idx < 0 || idx >= (int)pW->numThings || !pW->aThings[idx].type ) { return NULL; }
    return &pW->aThings[idx];
}

DSSV_STUB int J3DAPI dssv_sithThing_GetThingIndex(const SithThing* pThing)
{
    SithWorld* pW = sithWorld_g_pCurrentWorld;
    if ( !pThing ) { return -1; }
    if ( pThing < pW->aThings || pThing >= &pW->aThings[pW->numThings] ) { return -1; }
    if ( pThing->type == SITH_THING_FREE ) { return -1; }
    return pThing->idx;
}

DSSV_STUB SithThing* J3DAPI dssv_sithThing_GetGuidThing(int guid)
{
    SithWorld* pW = sithWorld_g_pCurrentWorld;
    if ( guid < 0 ) { return NULL; }
    if ( (guid & 0xFFFF0000) == 0 && guid < (int)pW->numThings && pW->aThings[guid].type ) { return &pW->aThings[guid]; }
    for ( int i = 0; i <= pW->lastThingIdx; ++i )
    {
        if ( pW->aThings[i].guid == guid && pW->aThings[i].type ) { return &pW->aThings[i]; }
    }
    return NULL;
}

DSSV_STUB SithSector* J3DAPI dssv_sithSector_GetSectorEx(const SithWorld* pWorld, int idx)
{
    if ( !pWorld || idx < 0 || idx >= (int)pWorld->numSectors ) { return NULL; }
    return &pWorld->aSectors[idx];
}

DSSV_STUB int J3DAPI dssv_sithSector_GetSectorIndex(const SithSector* pSector)
{
    SithWorld* pW = sithWorld_g_pCurrentWorld;
    if ( pSector >= pW->aSectors && pSector <= &pW->aSectors[pW->numSectors] ) { return (int)(pSector - pW->aSectors); }
    return -1;
}

DSSV_STUB SithSurface* J3DAPI dssv_sithSurface_GetSurfaceEx(const SithWorld* pWorld, int idx)
{
    if ( !pWorld || idx < 0 || idx >= (int)pWorld->numSurfaces ) { return NULL; }
    return &pWorld->aSurfaces[idx];
}

DSSV_STUB int J3DAPI dssv_sithSurface_GetSurfaceIndex(const SithSurface* pSurf)
{
    SithWorld* pW = sithWorld_g_pCurrentWorld;
    if ( pSurf >= pW->aSurfaces && pSurf <= &pW->aSurfaces[pW->numSurfaces] ) { return (int)(pSurf - pW->aSurfaces); }
    return -1;
}

DSSV_STUB rdModel3* J3DAPI dssv_sithModel_GetModelByIndex(size_t idx)
{
    int16_t modelIdx = (int16_t)idx; // the original takes an int16_t
    SithWorld* pW = sithWorld_g_pCurrentWorld;
    if ( (modelIdx & SITH_STATICRESOURCE_INDEXMASK) != 0 )
    {
        pW = sithWorld_g_pStaticWorld;
        modelIdx &= ~SITH_STATICRESOURCE_INDEXMASK;
    }
    if ( !pW || modelIdx < 0 || modelIdx >= (int)pW->numModels ) { return NULL; }
    return &pW->aModels[modelIdx];
}

DSSV_STUB int J3DAPI dssv_sithModel_GetModelIndex(const rdModel3* pModel)
{
    SithWorld* pW = sithWorld_g_pCurrentWorld;
    SithWorld* pS = sithWorld_g_pStaticWorld;
    int index;
    if ( pModel >= pW->aModels && pModel < &pW->aModels[pW->numModels] ) { index = (int)(pModel - pW->aModels); }
    else if ( pModel >= pS->aModels && pModel < &pS->aModels[pS->numModels] ) { index = (int)(pModel - pS->aModels) | SITH_STATICRESOURCE_INDEXMASK; }
    else { index = -1; }
    return (int16_t)index; // the original returns an int16_t
}

DSSV_STUB rdMaterial* J3DAPI dssv_sithMaterial_GetMaterialByIndex(int index)
{
    SithWorld* pW = sithWorld_g_pCurrentWorld;
    if ( (index & SITH_STATICRESOURCE_INDEXMASK) != 0 )
    {
        pW = sithWorld_g_pStaticWorld;
        index &= ~SITH_STATICRESOURCE_INDEXMASK;
    }
    if ( !pW || index < 0 || index >= (int)pW->numMaterials ) { return NULL; }
    return &pW->aMaterials[index];
}

DSSV_STUB SithThing* J3DAPI dssv_sithTemplate_GetTemplateByIndex(int idx)
{
    SithWorld* pW = sithWorld_g_pCurrentWorld;
    if ( (idx & SITH_STATICRESOURCE_INDEXMASK) != 0 )
    {
        pW = sithWorld_g_pStaticWorld;
        idx &= ~SITH_STATICRESOURCE_INDEXMASK;
    }
    if ( !pW ) { return NULL; }
    if ( idx < 0 || idx >= (int)pW->numThingTemplates ) { return NULL; }
    return &pW->aThingTemplates[idx];
}

DSSV_STUB int J3DAPI dssv_sithTemplate_GetTemplateIndex(const SithThing* pTemplate)
{
    SithWorld* pW = sithWorld_g_pCurrentWorld;
    if ( !pTemplate ) { return -1; }
    if ( pTemplate < pW->aThingTemplates || pTemplate >= &pW->aThingTemplates[pW->numThingTemplates] ) { return -1; }
    if ( !pTemplate->type ) { return -1; }
    return pTemplate->idx;
}

DSSV_STUB SithCog* J3DAPI dssv_sithCog_GetCogByIndex(int idx)
{
    SithWorld* pW = sithWorld_g_pCurrentWorld;
    if ( (idx & SITH_STATICRESOURCE_INDEXMASK) != 0 )
    {
        pW = sithWorld_g_pStaticWorld;
        idx &= ~SITH_STATICRESOURCE_INDEXMASK;
    }
    if ( !pW || idx < 0 || idx >= (int)pW->numCogs ) { return NULL; }
    return &pW->aCogs[idx];
}

DSSV_STUB rdKeyframe* J3DAPI dssv_sithPuppet_GetKeyframeByIndex(int index)
{
    SithWorld* pW = sithWorld_g_pCurrentWorld;
    if ( (index & SITH_STATICRESOURCE_INDEXMASK) != 0 )
    {
        pW = sithWorld_g_pStaticWorld;
        index &= ~SITH_STATICRESOURCE_INDEXMASK;
    }
    if ( index < 0 || index >= (int)pW->numKeyframes ) { return NULL; }
    return &pW->aKeyframes[index];
}

// Stable id of a pointer into the arena (kind in the high byte, index + 1 in the low bytes), 0 for NULL
static uint32_t dssv_Id(const void* p)
{
    DssvArena* a = &dssv_arena;
#define DSSV_ID(kind, arr, cnt) \
    if ( (const uint8_t*)p >= (const uint8_t*)&a->arr[0] && (const uint8_t*)p < (const uint8_t*)&a->arr[cnt] ) \
        return ((kind) << 24) | (uint32_t)(((const uint8_t*)p - (const uint8_t*)&a->arr[0]) / sizeof(a->arr[0]) + 1)
    if ( !p ) { return 0; }
    DSSV_ID(1u, things, DSSV_CAPTHINGS);
    DSSV_ID(2u, templates, DSSV_CAPTEMPLATES);
    DSSV_ID(3u, staticTemplates, 2);
    DSSV_ID(4u, sectors, DSSV_CAPSECTORS);
    DSSV_ID(5u, surfaces, DSSV_CAPSURFACES);
    DSSV_ID(6u, models, DSSV_CAPMODELS);
    DSSV_ID(7u, staticModels, 3);
    DSSV_ID(8u, mats, DSSV_CAPMATS);
    DSSV_ID(9u, staticMats, 4);
    DSSV_ID(10u, keys, DSSV_CAPKEYS);
    DSSV_ID(11u, staticKeys, 2);
    DSSV_ID(12u, cogs, DSSV_CAPCOGS);
    DSSV_ID(13u, staticCogs, 2);
    DSSV_ID(14u, puppets, DSSV_CAPTHINGS);
    DSSV_ID(15u, anims, DSSV_NUMANIMS + 1);
    DSSV_ID(16u, aiBlocks, 2);
#undef DSSV_ID
    return 0xDEAD0000u;
}
#define DSSV_P(p) ((unsigned)dssv_Id(p))

#endif // SITHDSSVEC_H

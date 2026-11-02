// Shared core of the sithMulti original-engine test vectors.
//
// Like the DSS vectors (sithDSSVec.h) this header is compiled by the capture harness (DSSV_HARNESS defined, injected
// into the retail v1.0 Indy3D.exe, runs the ORIGINAL functions at their v1.0 addresses) and by sithMultiTests (runs
// the reconstructed functions). It reuses the synthetic world, the record format and the message stub of the DSS
// vectors and adds the players, the module state of sithMulti and the stubs of the engine functions sithMulti calls
// (sithMultiSeams.def).
//
// Record layout of a case (all little endian):
//   u32 return value
//   u16 numAsserts, u16 numErrors, u16 numStatus, u16 numWarnings   (host service calls made during the case)
//   u32 eventsSize, events[]      (calls of the stubbed engine functions, DssvEvent / MsvEvent ids)
//   u32 numDeltas, delta[]        (u32 offset, u16 size, bytes: changed bytes of the watched memory, pointers normalized)
#ifndef SITHMULTIVEC_H
#define SITHMULTIVEC_H

#include "sithDSSVecRun.h"
#include "sithMultiVecState.h"

#include <sith/Devices/sithConsole.h>
#include <sith/Engine/sithCollision.h>
#include <sith/Main/sithString.h>

#include <std/General/stdPlatform.h>

#include <wchar.h>

#define MSV_NUMPLAYERS SITHPLAYER_MAX_PLAYERS

// Net ids: player i has MSV_NETID(i); the host is player 0
#define MSV_NETID(i)   ((DPID)(0x2000u + (unsigned)(i)))
#define MSV_GAMETIME   1000000u
#define MSV_CLOCKTIME  2000000u
#define MSV_CHECKSUM   0x1234ABCDu
#define MSV_SYNCTIME   0x00ABCDEFu // returned by stdPlatform_GetTimeMsec
#define MSV_NEWPLAYER  0x2777u     // net id of a player that is joining

typedef enum
{
    MSV_EV_CLOSEGAME = 100,
    MSV_EV_CHECKMEMBERSHIP,
    MSV_EV_STOPPROCESSMSGS,
    MSV_EV_UPDATEPLAYERS,
    MSV_EV_VERIFYPLAYER,
    MSV_EV_GETTIME,
    MSV_EV_REGISTERTASK,
    MSV_EV_DSSPOS,
    MSV_EV_DSSUPDATESTATE,
    MSV_EV_DSSFULLDESC,
    MSV_EV_DSSDESTROY,
    MSV_EV_DSSSURFACE,
    MSV_EV_DSSSECTOR,
    MSV_EV_DSSSECTORFLAGS,
    MSV_EV_GETSTRING,
    MSV_EV_PRINT,
    MSV_EV_PRINTW,
    MSV_EV_STOPSOUNDTHING,
    MSV_EV_HIDEPLAYER,
    MSV_EV_SHOWPLAYER,
    MSV_EV_SETLOCALPLAYER,
    MSV_EV_RESETPLAYER,
    MSV_EV_GETPLAYERNUM,
    MSV_EV_BROADCAST,
    MSV_EV_LOADPOSTPROCESS,
    MSV_EV_SETTHINGMODEL,
    MSV_EV_SYNCTHINGS,
    MSV_EV_SYNCSURFACES,
    MSV_EV_SYNCSECTORS,
    MSV_EV_CANSYNC,
    MSV_EV_MODELLOAD,
    MSV_EV_SEARCHCOLL,
    MSV_EV_POPCOLL,
    MSV_EV_DECSTACK,
    MSV_EV_RAND,
    MSV_EV_JOINEDCALLBACK,
    MSV_EV_INMSGTYPE
} MsvEvent;

#define MSV_EV(id, n, ...) dssv_Ev((DssvEvent)(id), n, __VA_ARGS__)
#define MSV_EV0(id) dssv_Ev((DssvEvent)(id), 0)

// ---------------------------------------------------------------------------------------------------------------
// Stub state
// ---------------------------------------------------------------------------------------------------------------
static int msv_updatePlayersResult, msv_verifyPlayerResult;
static uint32_t msv_canSyncMask; // bit i: sithThing_CanSyncThing returns 1 for thing i
static rdModel3* msv_pLoadedModel; // returned by sithModel_Load

#define MSV_MAXRAND 8
static int msv_aRand[MSV_MAXRAND];
static unsigned msv_numRand, msv_randPos;

#define MSV_NUMCOLLS 4
#define MSV_MAXSEARCH 8
static SithCollision msv_aColls[MSV_NUMCOLLS];
static int8_t msv_aCollScript[MSV_MAXSEARCH][MSV_NUMCOLLS + 1]; // per search: indices into msv_aColls, -1 ends
static unsigned msv_numSearches, msv_popPos;

#define MSV_MAXSTRINGS 16
static struct { char aKey[64]; wchar_t awText[64]; } msv_aStrings[MSV_MAXSTRINGS];
static unsigned msv_numStrings;

// ---------------------------------------------------------------------------------------------------------------
// Stubs
// ---------------------------------------------------------------------------------------------------------------
DSSV_STUB int J3DAPI msv_sithMessage_SendMessage(SithMessage* pMsg, DPID idTo, SithMessageStream outstream, uint32_t dwDPFlags)
{
    return dssv_sithMessage_SendMessage(pMsg, idTo, outstream, dwDPFlags);
}

DSSV_STUB DPID J3DAPI msv_sithMessage_GetSenderID(void) { return dssv_senderId; }
DSSV_STUB void msv_sithMessage_CloseGame(void) { MSV_EV0(MSV_EV_CLOSEGAME); }
DSSV_STUB void J3DAPI msv_sithMessage_CheckMembershipList(DPID idTo) { MSV_EV(MSV_EV_CHECKMEMBERSHIP, 1, (uint32_t)idTo); }
DSSV_STUB void J3DAPI msv_sithMessage_StopProcessMessages() { MSV_EV0(MSV_EV_STOPPROCESSMSGS); }
DSSV_STUB int J3DAPI msv_stdComm_IsGameHost(void) { return dssv_bGameHost; }
DSSV_STUB int J3DAPI msv_stdComm_IsGameActive(void) { return dssv_bGameActive; }

DSSV_STUB int J3DAPI msv_stdComm_UpdatePlayers(size_t gameNum)
{
    MSV_EV(MSV_EV_UPDATEPLAYERS, 1, (uint32_t)gameNum);
    return msv_updatePlayersResult;
}

DSSV_STUB int J3DAPI msv_stdComm_VerifyPlayer(DPID id)
{
    MSV_EV(MSV_EV_VERIFYPLAYER, 1, (uint32_t)id);
    return msv_verifyPlayerResult;
}

DSSV_STUB tStdTime msv_stdPlatform_GetTimeMsec(void)
{
    MSV_EV0(MSV_EV_GETTIME);
    return MSV_SYNCTIME;
}

// The task callback is normalized: 1 for sithMulti_CheckPlayers, 0 for NULL
static uint32_t msv_NormTask(SithEventProcess pf)
{
    if ( !pf ) { return 0; }
#ifdef DSSV_HARNESS
    if ( (uintptr_t)pf == sithMulti_CheckPlayers_ADDR ) { return 1; }
#else
    if ( pf == sithMulti_CheckPlayers ) { return 1; }
#endif
    return MSV_CALLBACK_OTHER;
}

DSSV_STUB int J3DAPI msv_sithEvent_RegisterTask(size_t taskId, SithEventProcess pfProcess, uint32_t frequency, SithEventTaskMode startMode)
{
    MSV_EV(MSV_EV_REGISTERTASK, 4, (uint32_t)taskId, msv_NormTask(pfProcess), frequency, (uint32_t)startMode);
    return 1;
}

DSSV_STUB int J3DAPI msv_sithDSSThing_Pos(const SithThing* pThing, DPID toID, unsigned int dpFlags)
{
    MSV_EV(MSV_EV_DSSPOS, 3, DSSV_P(pThing), (uint32_t)toID, dpFlags);
    return 0x66;
}

DSSV_STUB int J3DAPI msv_sithDSSThing_UpdateState(const SithThing* pThing, DPID toID, SithMessageStream outstream)
{
    MSV_EV(MSV_EV_DSSUPDATESTATE, 3, DSSV_P(pThing), (uint32_t)toID, (uint32_t)outstream);
    return 0x66;
}

DSSV_STUB int J3DAPI msv_sithDSSThing_ThingFullDescription(const SithThing* pThing, DPID idTo, SithMessageStream outstream)
{
    MSV_EV(MSV_EV_DSSFULLDESC, 3, DSSV_P(pThing), (uint32_t)idTo, (uint32_t)outstream);
    return 0x66;
}

DSSV_STUB int J3DAPI msv_sithDSSThing_DestroyThing(int guid, DPID idTo)
{
    MSV_EV(MSV_EV_DSSDESTROY, 2, (uint32_t)guid, (uint32_t)idTo);
    return 0x66;
}

DSSV_STUB int J3DAPI msv_sithDSS_SurfaceStatus(const SithSurface* pSurf, DPID idTo, unsigned int outstream)
{
    MSV_EV(MSV_EV_DSSSURFACE, 3, DSSV_P(pSurf), (uint32_t)idTo, outstream);
    return 0x66;
}

DSSV_STUB int J3DAPI msv_sithDSS_SectorStatus(const SithSector* pSector, DPID idTo, unsigned int outstream)
{
    MSV_EV(MSV_EV_DSSSECTOR, 3, DSSV_P(pSector), (uint32_t)idTo, outstream);
    return 0x66;
}

DSSV_STUB int J3DAPI msv_sithDSS_SectorFlags(const SithSector* pSector, DPID idTo, unsigned int outstream)
{
    MSV_EV(MSV_EV_DSSSECTORFLAGS, 3, DSSV_P(pSector), (uint32_t)idTo, outstream);
    return 0x66;
}

// Returns the key itself as the (wide) text, so the formats of the keys with %s work as in the game's string table
DSSV_STUB wchar_t* J3DAPI msv_sithString_GetString(const char* pStr)
{
    MSV_EV(MSV_EV_GETSTRING, 1, dssv_Hash(pStr));
    for ( unsigned i = 0; i < msv_numStrings; ++i )
    {
        if ( strcmp(msv_aStrings[i].aKey, pStr) == 0 ) { return msv_aStrings[i].awText; }
    }
    if ( msv_numStrings == MSV_MAXSTRINGS ) { return NULL; }
    unsigned k = msv_numStrings++;
    size_t n = 0;
    for ( ; pStr[n] && n < 63; ++n )
    {
        msv_aStrings[k].aKey[n]   = pStr[n];
        msv_aStrings[k].awText[n] = (wchar_t)(uint8_t)pStr[n];
    }
    msv_aStrings[k].aKey[n]   = 0;
    msv_aStrings[k].awText[n] = 0;
    return msv_aStrings[k].awText;
}

// The printed text is recorded up to its terminator (at most 1024 characters; 0xFFFF marks a longer or NULL text)
DSSV_STUB void J3DAPI msv_sithConsole_PrintString(const char* pString)
{
    MSV_EV0(MSV_EV_PRINT);
    size_t n = 0;
    while ( pString && n < 1024 && pString[n] ) { ++n; }
    if ( !pString || n == 1024 ) { dssvB_u16(&dssv_ev, 0xFFFF); return; }
    dssvB_u16(&dssv_ev, (uint32_t)n);
    dssvB_put(&dssv_ev, pString, n);
}

DSSV_STUB void J3DAPI msv_sithConsole_PrintWString(const wchar_t* pwString)
{
    MSV_EV0(MSV_EV_PRINTW);
    size_t n = 0;
    while ( pwString && n < 1024 && pwString[n] ) { ++n; }
    if ( !pwString || n == 1024 ) { dssvB_u16(&dssv_ev, 0xFFFF); return; }
    dssvB_u16(&dssv_ev, (uint32_t)n);
    for ( size_t i = 0; i < n; ++i ) { dssvB_u16(&dssv_ev, (uint32_t)pwString[i]); }
}

DSSV_STUB void J3DAPI msv_sithSoundMixer_StopSoundThing(const SithThing* pThing, tSoundHandleType handle)
{
    MSV_EV(MSV_EV_STOPSOUNDTHING, 2, DSSV_P(pThing), (uint32_t)handle);
}

DSSV_STUB void J3DAPI msv_sithPlayer_HidePlayer(size_t playerNum) { MSV_EV(MSV_EV_HIDEPLAYER, 1, (uint32_t)playerNum); }

DSSV_STUB int J3DAPI msv_sithPlayer_ShowPlayer(size_t playerNum, DPID id)
{
    MSV_EV(MSV_EV_SHOWPLAYER, 2, (uint32_t)playerNum, (uint32_t)id);
    return 1;
}

DSSV_STUB void J3DAPI msv_sithPlayer_SetLocalPlayer(size_t playerNum) { MSV_EV(MSV_EV_SETLOCALPLAYER, 1, (uint32_t)playerNum); }
DSSV_STUB void J3DAPI msv_sithPlayer_Reset(size_t playerNum) { MSV_EV(MSV_EV_RESETPLAYER, 1, (uint32_t)playerNum); }

DSSV_STUB int J3DAPI msv_sithPlayer_GetPlayerNum(DPID playerId)
{
    MSV_EV(MSV_EV_GETPLAYERNUM, 1, (uint32_t)playerId);
    for ( size_t i = 0; i < sithPlayer_g_numPlayers && i < MSV_NUMPLAYERS; ++i )
    {
        if ( sithPlayer_g_aPlayers[i].playerNetId == playerId ) { return (int)i; }
    }
    return -1;
}

DSSV_STUB void J3DAPI msv_sithCog_BroadcastMessage(SithCogMsgType msgType, SithCogSymbolRefType senderType, int senderIdx, SithCogSymbolRefType srcType, int srcIdx)
{
    MSV_EV(MSV_EV_BROADCAST, 5, (uint32_t)msgType, (uint32_t)senderType, (uint32_t)senderIdx, (uint32_t)srcType, (uint32_t)srcIdx);
}

DSSV_STUB void J3DAPI msv_sithThing_LoadPostProcess(SithWorld* pWorld)
{
    MSV_EV(MSV_EV_LOADPOSTPROCESS, 1, pWorld == &dssv_arena.world ? 1u : (pWorld ? 0xBADu : 0u));
}

DSSV_STUB int J3DAPI msv_sithThing_SetThingModel(SithThing* pThing, rdModel3* pModel)
{
    MSV_EV(MSV_EV_SETTHINGMODEL, 2, DSSV_P(pThing), DSSV_P(pModel));
    return 1;
}

DSSV_STUB void msv_sithThing_SyncThings(void) { MSV_EV0(MSV_EV_SYNCTHINGS); }
DSSV_STUB void msv_sithSurface_SyncSurfaces(void) { MSV_EV0(MSV_EV_SYNCSURFACES); }

DSSV_STUB size_t msv_sithSector_SyncSectors(void)
{
    MSV_EV0(MSV_EV_SYNCSECTORS);
    return 0;
}

DSSV_STUB int J3DAPI msv_sithThing_CanSyncThing(const SithThing* pThing)
{
    MSV_EV(MSV_EV_CANSYNC, 1, DSSV_P(pThing));
    ptrdiff_t idx = pThing - &dssv_arena.things[0];
    return idx >= 0 && idx < 32 && ((msv_canSyncMask >> idx) & 1) != 0;
}

DSSV_STUB rdModel3* J3DAPI msv_sithModel_Load(const char* pName, int bSkipDefault)
{
    MSV_EV(MSV_EV_MODELLOAD, 2, dssv_Hash(pName), (uint32_t)bSkipDefault);
    return msv_pLoadedModel;
}

static uint32_t msv_FloatBits(float f)
{
    uint32_t u;
    memcpy(&u, &f, sizeof(u));
    return u;
}

DSSV_STUB float J3DAPI msv_sithCollision_SearchForCollisions(SithSector* pStartSector, SithThing* pThing, const rdVector3* startPos, const rdVector3* moveNorm, float moveDist, float radius, int searchFlags)
{
    MSV_EV(MSV_EV_SEARCHCOLL, 9, DSSV_P(pStartSector), DSSV_P(pThing), dssv_NormWord((uint32_t)(uintptr_t)startPos),
        msv_FloatBits(moveNorm->x), msv_FloatBits(moveNorm->y), msv_FloatBits(moveNorm->z), msv_FloatBits(moveDist), msv_FloatBits(radius), (uint32_t)searchFlags);
    msv_popPos = 0;
    ++msv_numSearches;
    return 0.0f;
}

DSSV_STUB SithCollision* msv_sithCollision_PopStack(void)
{
    unsigned search = msv_numSearches ? msv_numSearches - 1 : 0;
    if ( search >= MSV_MAXSEARCH ) { search = MSV_MAXSEARCH - 1; }
    int idx = msv_popPos <= MSV_NUMCOLLS ? msv_aCollScript[search][msv_popPos] : -1;
    MSV_EV(MSV_EV_POPCOLL, 1, (uint32_t)idx);
    if ( idx < 0 ) { return NULL; }
    ++msv_popPos;
    return &msv_aColls[idx];
}

DSSV_STUB void msv_sithCollision_DecreaseStackLevel(void) { MSV_EV0(MSV_EV_DECSTACK); }

// The CRT rand of SITH_RAND(): returns the scripted values of the case, then 0x1234
DSSV_STUB int msv_rand(void)
{
    int r = msv_randPos < msv_numRand ? msv_aRand[msv_randPos] : 0x1234;
    ++msv_randPos;
    MSV_EV(MSV_EV_RAND, 1, (uint32_t)r);
    return r;
}

DSSV_STUB void msv_NewPlayerJoinedCallback(void) { MSV_EV0(MSV_EV_JOINEDCALLBACK); }

// ---------------------------------------------------------------------------------------------------------------
// Module state
// ---------------------------------------------------------------------------------------------------------------
#ifdef DSSV_HARNESS
#define MSV_EXEVAR(name) (*(uint32_t*)sithMulti_##name##_ADDR)

static void msv_GetState(MsvState* s)
{
    s->tickRate           = MSV_EXEVAR(tickRate);
    s->numUpdatedSurfaces = MSV_EXEVAR(numUpdatedSurfaces);
    s->lastUpdateIdx      = MSV_EXEVAR(lastUpdateIdx);
    s->playerWelcomeState = MSV_EXEVAR(playerWelcomeState);
    s->numUpdatedSectors  = MSV_EXEVAR(numUpdatedSectors);
    memcpy(s->aRemovedStaticThings, (const void*)sithMulti_aRemovedStaticThings_ADDR, sizeof(s->aRemovedStaticThings));
    s->curWelcomePlayerNum = MSV_EXEVAR(curWelcomePlayerNum);
    s->numUpdatedThings    = MSV_EXEVAR(numUpdatedThings);
    s->quitGameState       = MSV_EXEVAR(quitGameState);
    s->checksum            = MSV_EXEVAR(checksum);
    s->bWelcomingPlayer    = MSV_EXEVAR(bWelcomingPlayer) ? 1u : 0u;
    s->newPlayerId         = MSV_EXEVAR(newPlayerId);
    s->bSyncScores         = MSV_EXEVAR(bSyncScores) ? 1u : 0u;
    uintptr_t pf = (uintptr_t)MSV_EXEVAR(pfNewPlayerJoinedCallback);
    s->pfNewPlayerJoinedCallback = !pf ? 0u : (pf == (uintptr_t)&msv_NewPlayerJoinedCallback ? 1u : MSV_CALLBACK_OTHER);
    s->msecPingStartTime         = MSV_EXEVAR(msecPingStartTime);
    s->msecQuitGameTime          = MSV_EXEVAR(msecQuitGameTime);
    s->numRemovedStaticThings    = MSV_EXEVAR(numRemovedStaticThings);
    s->msecLastSyncScoreTime     = MSV_EXEVAR(msecLastSyncScoreTime);
    s->msecWelcomeUpdateInterval = MSV_EXEVAR(msecWelcomeUpdateInterval);
    s->msecSyncTime              = MSV_EXEVAR(msecSyncTime);
}

static void msv_SetState(const MsvState* s)
{
    MSV_EXEVAR(tickRate)           = s->tickRate;
    MSV_EXEVAR(numUpdatedSurfaces) = s->numUpdatedSurfaces;
    MSV_EXEVAR(lastUpdateIdx)      = s->lastUpdateIdx;
    MSV_EXEVAR(playerWelcomeState) = s->playerWelcomeState;
    MSV_EXEVAR(numUpdatedSectors)  = s->numUpdatedSectors;
    memcpy((void*)sithMulti_aRemovedStaticThings_ADDR, s->aRemovedStaticThings, sizeof(s->aRemovedStaticThings));
    MSV_EXEVAR(curWelcomePlayerNum)       = s->curWelcomePlayerNum;
    MSV_EXEVAR(numUpdatedThings)          = s->numUpdatedThings;
    MSV_EXEVAR(quitGameState)             = s->quitGameState;
    MSV_EXEVAR(checksum)                  = s->checksum;
    MSV_EXEVAR(bWelcomingPlayer)          = s->bWelcomingPlayer;
    MSV_EXEVAR(newPlayerId)               = s->newPlayerId;
    MSV_EXEVAR(bSyncScores)               = s->bSyncScores;
    MSV_EXEVAR(pfNewPlayerJoinedCallback) = s->pfNewPlayerJoinedCallback ? (uint32_t)(uintptr_t)&msv_NewPlayerJoinedCallback : 0u;
    MSV_EXEVAR(msecPingStartTime)         = s->msecPingStartTime;
    MSV_EXEVAR(msecQuitGameTime)          = s->msecQuitGameTime;
    MSV_EXEVAR(numRemovedStaticThings)    = s->numRemovedStaticThings;
    MSV_EXEVAR(msecLastSyncScoreTime)     = s->msecLastSyncScoreTime;
    MSV_EXEVAR(msecWelcomeUpdateInterval) = s->msecWelcomeUpdateInterval;
    MSV_EXEVAR(msecSyncTime)              = s->msecSyncTime;
}
#else
#define msv_GetState sithMultiTest_GetState
#define msv_SetState sithMultiTest_SetState
#endif

static MsvState msv_state;       // state the case sets up (applied by msv_Begin)
static MsvState msv_mirror;      // watched copy of the module state
static uint32_t msv_mirrorServerId;

static void msv_Mirror(void)
{
    msv_GetState(&msv_mirror);
    msv_mirrorServerId = (uint32_t)sithMulti_g_serverId;
}

// ---------------------------------------------------------------------------------------------------------------
// Players
// ---------------------------------------------------------------------------------------------------------------
static const unsigned msv_aPlayerThings[7] = { 0, 1, 2, 3, 4, 5, 7 };

static const wchar_t msv_awName15[] = L"FifteenCharName";
static const wchar_t msv_awName16[] = L"SixteenCharNameX";

// Sets the name of player i: name (at most 63 characters and the terminator) followed by zeros, or by '#' with bStale
static void msv_SetName(unsigned i, const wchar_t* pName, int bStale)
{
    SithPlayer* p = &sithPlayer_g_aPlayers[i];
    for ( size_t k = 0; k < STD_ARRAYLEN(p->awName); ++k ) { p->awName[k] = bStale ? L'#' : 0; }
    size_t n = 0;
    for ( ; pName[n] && n < STD_ARRAYLEN(p->awName) - 1; ++n ) { p->awName[n] = pName[n]; }
    p->awName[n] = 0;
}

// A name of len characters ('A'..'Z' repeated, starting at first)
static void msv_MakeName(wchar_t* pOut, size_t len, wchar_t first)
{
    for ( size_t k = 0; k < len; ++k ) { pOut[k] = (wchar_t)(L'A' + (first - L'A' + k) % 26); }
    pOut[len] = 0;
}

// Builds numBuilt players (the rest of the array holds the probe garbage) and numPlayers players in the game
static void msv_BuildPlayers(unsigned numBuilt, unsigned numPlayers)
{
    DssvArena* a = &dssv_arena;
    for ( unsigned i = 0; i < MSV_NUMPLAYERS; ++i )
    {
        SithPlayer* p = &sithPlayer_g_aPlayers[i];
        if ( i >= numBuilt )
        {
            memset(p, (int)dssv_fill, sizeof(*p));
            continue;
        }

        memset(p, 0, sizeof(*p));
        wchar_t awName[16];
        swprintf(awName, 16, L"Player%u", i);
        msv_SetName(i, awName, 0);
        p->flags       = SITH_PLAYER_JOINEDGAME | SITH_PLAYER_PLACED | SITH_PLAYER_UNKNOWN_04;
        p->playerNetId = MSV_NETID(i);
        p->pThing      = &a->things[msv_aPlayerThings[i % 7]];
        RV3(p->orient.rvec);
        RV3(p->orient.lvec);
        RV3(p->orient.uvec);
        RV3(p->orient.dvec);
        p->pInSector        = &a->sectors[i % DSSV_NUMSECTORS];
        p->respawnMask      = 0;
        p->msecLastCommTime = MSV_GAMETIME - 500u * i;
        RU(p->curItemID);
        RU(p->curWeaponID);
    }
    sithPlayer_g_numPlayers        = numPlayers;
    sithPlayer_g_pLocalPlayerThing = &a->things[0];
    sithPlayer_g_pLocalPlayer      = &sithPlayer_g_aPlayers[0];
    a->things[0].thingInfo.actorInfo.pPlayer = &sithPlayer_g_aPlayers[0];
    a->things[1].thingInfo.actorInfo.pPlayer = &sithPlayer_g_aPlayers[1];
}

// Makes the local player the client player 1 (the server stays player 0)
static void msv_SetClient(void)
{
    dssv_bGameHost                 = 0;
    sithMessage_g_localPlayerId    = MSV_NETID(1);
    sithPlayer_g_pLocalPlayerThing = sithPlayer_g_aPlayers[1].pThing;
    sithPlayer_g_pLocalPlayer      = &sithPlayer_g_aPlayers[1];
}

// ---------------------------------------------------------------------------------------------------------------
// Hand-built input messages
// ---------------------------------------------------------------------------------------------------------------
typedef struct
{
    SithMessage* pMsg;
    uint8_t* p;
} MsvMsgOut;

static MsvMsgOut msv_MsgStart(DssvCtx* cx, unsigned type)
{
    MsvMsgOut m;
    m.pMsg       = &cx->inMsg;
    m.pMsg->type = (uint16_t)type;
    m.p          = &m.pMsg->data[0];
    cx->bReader  = 1;
    return m;
}

static void msv_Put(MsvMsgOut* m, const void* p, size_t n) { memcpy(m->p, p, n); m->p += n; }
static void msv_Put16(MsvMsgOut* m, uint32_t v) { uint16_t x = (uint16_t)v; msv_Put(m, &x, 2); }
static void msv_Put32(MsvMsgOut* m, uint32_t v) { msv_Put(m, &v, 4); }
static void msv_PutStr(MsvMsgOut* m, const char* s, size_t size) // size bytes: s zero padded (not terminated if s is long)
{
    size_t n = strlen(s);
    for ( size_t k = 0; k < size; ++k ) { *m->p++ = k < n ? (uint8_t)s[k] : 0; }
}
static void msv_PutWStr(MsvMsgOut* m, const wchar_t* s, size_t num)
{
    size_t n = wcslen(s);
    for ( size_t k = 0; k < num; ++k ) { msv_Put16(m, k < n ? (uint32_t)s[k] : 0); }
}
static void msv_MsgEnd(MsvMsgOut* m) { m->pMsg->length = (uint32_t)(m->p - &m->pMsg->data[0]); }

// ---------------------------------------------------------------------------------------------------------------
// Runner
// ---------------------------------------------------------------------------------------------------------------
static void msv_DefaultState(MsvState* s)
{
    memset(s, 0, sizeof(*s));
    s->tickRate           = 70;
    s->playerWelcomeState = SITHMULTI_WELCOME_SURFACES;
    s->checksum           = MSV_CHECKSUM;
    s->numRemovedStaticThings  = 3;
    s->aRemovedStaticThings[0] = DSSV_GUID(2);
    s->aRemovedStaticThings[1] = 0x10055;
    s->aRemovedStaticThings[2] = 0x7FFF0001;
    for ( unsigned i = 3; i < MSV_NUMREMOVED; ++i ) { s->aRemovedStaticThings[i] = 0x5EED0000u + i; }
}

// Called by a case function right before it calls the function under test: applies msv_state and takes the snapshot
static void msv_Begin(DssvCtx* cx)
{
    msv_SetState(&msv_state);
    memset(&sithMulti_g_message, (int)dssv_fill, sizeof(sithMulti_g_message));
    memset(&std_g_genBuffer[0], (int)dssv_fill, sizeof(std_g_genBuffer));

    dssv_WatchAll();
    dssv_Watch(&sithPlayer_g_aPlayers[2], sizeof(SithPlayer) * (MSV_NUMPLAYERS - 2));
    DSSV_W(msv_mirror);
    DSSV_W(msv_mirrorServerId);
    DSSV_W(sithPlayer_g_numPlayers);
    DSSV_W(sithPlayer_g_pLocalPlayerThing);
    DSSV_W(sithPlayer_g_pLocalPlayer);
    DSSV_W(sithMessage_g_localPlayerId);
    DSSV_W(sithMessage_g_outputstream);
    DSSV_W(sithMessage_g_inputstream);
    DSSV_W(sithMain_g_sith_mode);
    DSSV_W(sithTime_g_msecGameTime);
    DSSV_W(sithTime_g_clockTime);

    dssv_mirrorResetCameraAspect = DSSV_RESETCAMERAASPECT;
    msv_Mirror();
    dssv_Capture(&cx->pre);
}

// Runs a case and appends its record to rec (cleared first). in is NULL unless the case reads a writer's message.
static void msv_RunCase(const DssvCase* c, uint32_t fill, const DssvMsgIn* in, DssvBuf* rec)
{
    DssvCtx cx;
    memset(&cx, 0, sizeof(cx));
    cx.c = c;

    dssv_fill       = fill;
    dssv_numAsserts = dssv_numErrors = dssv_numStatus = dssv_numMallocs = dssv_numWarnings = 0;
    dssv_ev.size    = 0;
    dssv_lastMsg.valid = 0;
    dssv_sendResult = 0x5151;
    dssv_senderId   = MSV_NETID(3);
    dssv_bGameHost  = 1;
    dssv_bGameActive = 1;
    dssv_bAllocFail = 0;
    dssv_msgTailClean = 1;

    msv_updatePlayersResult = 0;
    msv_verifyPlayerResult  = 0;
    msv_canSyncMask         = 0xBF; // every built thing but the free thing 6
    msv_numRand = msv_randPos = 0;
    msv_numSearches = msv_popPos = 0;
    memset(msv_aCollScript, 0xFF, sizeof(msv_aCollScript));

    dssv_BuildWorld(dssv_Hash(c->name));
    dssv_BuildGlobals();
    msv_pLoadedModel = &dssv_arena.models[2];
    snprintf(dssv_arena.world.aName, sizeof(dssv_arena.world.aName), "level%u.ndy", 3u);

    // Collisions: 0 a live player, 1 an actor, 2 a dying player (thing 5 made a player), 3 a surface
    memset(msv_aColls, 0, sizeof(msv_aColls));
    msv_aColls[0].type           = SITHCOLLISION_THING;
    msv_aColls[0].pThingCollided = &dssv_arena.things[0];
    msv_aColls[1].type           = SITHCOLLISION_THING;
    msv_aColls[1].pThingCollided = &dssv_arena.things[1];
    msv_aColls[2].type           = SITHCOLLISION_THING;
    msv_aColls[2].pThingCollided = &dssv_arena.things[5];
    msv_aColls[3].type           = SITHCOLLISION_FACE;
    msv_aColls[3].pSurfaceCollided = &dssv_arena.surfaces[1];
    dssv_arena.things[0].flags &= ~(SITH_TF_DYING | SITH_TF_DESTROYED);

    msv_BuildPlayers(4, 4);
    sithMulti_g_serverId        = MSV_NETID(0);
    sithMessage_g_localPlayerId = MSV_NETID(0);
    sithMessage_g_outputstream  = (SithMessageStream)0xFF;
    sithMessage_g_inputstream   = (SithMessageStream)0xFF;
    sithTime_g_msecGameTime     = MSV_GAMETIME;
    sithTime_g_clockTime        = MSV_CLOCKTIME;
    msv_DefaultState(&msv_state);

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
    msv_Mirror();
    dssv_Capture(&post);
    if ( cx.pre.size != post.size ) { printf("msv: watch size changed\n"); }

    rec->size = 0;
    dssvB_u32(rec, (uint32_t)ret);
    dssvB_u16(rec, dssv_numAsserts);
    dssvB_u16(rec, dssv_numErrors);
    dssvB_u16(rec, dssv_numStatus);
    dssvB_u16(rec, dssv_numWarnings);
    dssvB_u32(rec, (uint32_t)dssv_ev.size);
    dssvB_put(rec, dssv_ev.p, dssv_ev.size);
    dssv_Delta(rec, &cx.pre, &post);

    dssvB_free(&cx.pre);
    dssvB_free(&post);
}

#define MSV_A   (&dssv_arena)
#define MSV_VAR (cx->c->var)
#define MSV_MSG (&cx->inMsg)
#define MSV_S   (&msv_state)
#define MSV_P(i) (&sithPlayer_g_aPlayers[i])

#include "sithMultiVecCases.h"

#endif // SITHMULTIVEC_H

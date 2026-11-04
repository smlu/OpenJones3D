// Compiles sithMulti.c for the original-engine vector tests (see tv/sithMulti/README.md).
//
// sithMulti.c keeps its module state in file statics. The vector cases set that state before they call a function and
// compare it afterwards, so the source is compiled in this translation unit together with the accessors below. The
// engine calls of sithMulti.c are renamed to the stubs of sithMultiVec.h by CMakeLists.txt (sithMultiSeams.def);
// the CRT rand of SITH_RAND() is renamed here so the cases control the random numbers.
#include <stdlib.h>

int msv_rand(void);
// The force-included address headers reach j3dcrt.h, which on the Null platform macros rand to
// j3dcrt_rand; drop that first so this capture redirect wins (no-op on Windows, where j3dcrt.h
// compiles out and rand is not a macro).
#undef rand
#define rand msv_rand

#include "../sithMulti.c"

#include "sithMultiVecState.h"

void sithMultiTest_GetState(MsvState* pState)
{
    pState->tickRate           = (uint32_t)sithMulti_tickRate;
    pState->numUpdatedSurfaces = (uint32_t)sithMulti_numUpdatedSurfaces;
    pState->lastUpdateIdx      = (uint32_t)sithMulti_lastUpdateIdx;
    pState->playerWelcomeState = (uint32_t)sithMulti_playerWelcomeState;
    pState->numUpdatedSectors  = (uint32_t)sithMulti_numUpdatedSectors;
    for ( size_t i = 0; i < MSV_NUMREMOVED; ++i )
    {
        pState->aRemovedStaticThings[i] = (uint32_t)sithMulti_aRemovedStaticThings[i];
    }

    pState->curWelcomePlayerNum = sithMulti_curWelcomePlayerNum;
    pState->numUpdatedThings    = (uint32_t)sithMulti_numUpdatedThings;
    pState->quitGameState       = (uint32_t)sithMulti_quitGameState;
    pState->checksum            = (uint32_t)sithMulti_checksum;
    pState->bWelcomingPlayer    = sithMulti_bWelcomingPlayer ? 1u : 0u;
    pState->newPlayerId         = (uint32_t)sithMulti_newPlayerId;
    pState->bSyncScores         = sithMulti_bSyncScores ? 1u : 0u;

    if ( !sithMulti_pfNewPlayerJoinedCallback )
    {
        pState->pfNewPlayerJoinedCallback = 0;
    }
    else if ( sithMulti_pfNewPlayerJoinedCallback == msv_NewPlayerJoinedCallback )
    {
        pState->pfNewPlayerJoinedCallback = 1;
    }
    else
    {
        pState->pfNewPlayerJoinedCallback = MSV_CALLBACK_OTHER;
    }

    pState->msecPingStartTime         = sithMulti_msecPingStartTime;
    pState->msecQuitGameTime          = sithMulti_msecQuitGameTime;
    pState->numRemovedStaticThings    = (uint32_t)sithMulti_numRemovedStaticThings;
    pState->msecLastSyncScoreTime     = sithMulti_msecLastSyncScoreTime;
    pState->msecWelcomeUpdateInterval = sithMulti_msecWelcomeUpdateInterval;
    pState->msecSyncTime              = (uint32_t)sithMulti_msecSyncTime;
}

void sithMultiTest_SetState(const MsvState* pState)
{
    sithMulti_tickRate           = pState->tickRate;
    sithMulti_numUpdatedSurfaces = (int)pState->numUpdatedSurfaces;
    sithMulti_lastUpdateIdx      = (int)pState->lastUpdateIdx;
    sithMulti_playerWelcomeState = (SithMultiWelcomeState)pState->playerWelcomeState;
    sithMulti_numUpdatedSectors  = (int)pState->numUpdatedSectors;
    for ( size_t i = 0; i < MSV_NUMREMOVED; ++i )
    {
        sithMulti_aRemovedStaticThings[i] = (int)pState->aRemovedStaticThings[i];
    }

    sithMulti_curWelcomePlayerNum       = pState->curWelcomePlayerNum;
    sithMulti_numUpdatedThings          = (int)pState->numUpdatedThings;
    sithMulti_quitGameState             = (SithMultiQuitState)pState->quitGameState;
    sithMulti_checksum                  = (int)pState->checksum;
    sithMulti_bWelcomingPlayer          = pState->bWelcomingPlayer != 0;
    sithMulti_newPlayerId               = pState->newPlayerId;
    sithMulti_bSyncScores               = pState->bSyncScores != 0;
    sithMulti_pfNewPlayerJoinedCallback = pState->pfNewPlayerJoinedCallback ? msv_NewPlayerJoinedCallback : NULL;
    sithMulti_msecPingStartTime         = pState->msecPingStartTime;
    sithMulti_msecQuitGameTime          = pState->msecQuitGameTime;
    sithMulti_numRemovedStaticThings    = pState->numRemovedStaticThings;
    sithMulti_msecLastSyncScoreTime     = pState->msecLastSyncScoreTime;
    sithMulti_msecWelcomeUpdateInterval = pState->msecWelcomeUpdateInterval;
    sithMulti_msecSyncTime              = (int)pState->msecSyncTime;
}

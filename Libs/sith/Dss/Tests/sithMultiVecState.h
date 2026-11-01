// Module state of sithMulti.c as seen by the original-engine vector cases (see sithMultiVec.h).
//
// The original keeps the state in .data/.bss variables of the executable, the reconstruction in file statics of
// sithMulti.c (some with other types, e.g. bool instead of int). The cases set and read the state through this
// structure: the capture harness accesses the variables of the executable at their v1.0 addresses, the tests call the
// accessors that sithMultiTested.c compiles together with sithMulti.c.
#ifndef SITHMULTIVECSTATE_H
#define SITHMULTIVECSTATE_H

#include <stdint.h>

#define MSV_NUMREMOVED 256u

typedef struct
{
    uint32_t tickRate;
    uint32_t numUpdatedSurfaces;
    uint32_t lastUpdateIdx;
    uint32_t playerWelcomeState;
    uint32_t numUpdatedSectors;
    uint32_t aRemovedStaticThings[MSV_NUMREMOVED];
    uint32_t curWelcomePlayerNum;
    uint32_t numUpdatedThings;
    uint32_t quitGameState;
    uint32_t checksum;
    uint32_t bWelcomingPlayer;
    uint32_t newPlayerId;
    uint32_t bSyncScores;
    uint32_t pfNewPlayerJoinedCallback; // 0: NULL, 1: msv_NewPlayerJoinedCallback, 0xBAD0BAD0: anything else
    uint32_t msecPingStartTime;
    uint32_t msecQuitGameTime;
    uint32_t numRemovedStaticThings;
    uint32_t msecLastSyncScoreTime;
    uint32_t msecWelcomeUpdateInterval;
    uint32_t msecSyncTime;
} MsvState;

#define MSV_CALLBACK_OTHER 0xBAD0BAD0u

#ifndef DSSV_HARNESS
// Defined in sithMultiTested.c
void sithMultiTest_GetState(MsvState* pState);
void sithMultiTest_SetState(const MsvState* pState);
// Defined in sithMultiTest.c (sithMultiVec.h)
void msv_NewPlayerJoinedCallback(void);
#endif

#endif // SITHMULTIVECSTATE_H

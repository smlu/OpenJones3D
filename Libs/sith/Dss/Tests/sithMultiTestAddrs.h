// Force-included into the sithMulti tests and sithMultiTested.c (see CMakeLists.txt).
//
// Defines the <global>_ADDR redirects used by the multiplayer vector cases, in addition to the redirects of the
// DSS tests (sithDSSTestAddrs.h, whose state builder the sithMulti cases reuse). sithMulti.c itself reaches the
// module globals of the reconstruction directly; the redirects are read only by the historical harness-era case
// code and are kept until that is removed in a separate cleanup. The capture harness does not use this header.
#ifndef SITHMULTITESTADDRS_H
#define SITHMULTITESTADDRS_H

#include "sithDSSTestAddrs.h"

#include <sith/Devices/sithComm.h>

typedef struct
{
    size_t numPlayers;
    SithThing* pLocalPlayerThing;
    DPID localPlayerId;
    SithMessageStream outputstream;
    SithMessageStream inputstream;
    uint32_t clockTime;
    char genBuffer[2048];
} SithMultiTestGlobals;

extern SithMultiTestGlobals sithMultiTest_g;

#undef sithPlayer_g_numPlayers_ADDR
#undef sithPlayer_g_pLocalPlayerThing_ADDR
#undef sithMessage_g_localPlayerId_ADDR
#undef sithMessage_g_outputstream_ADDR
#undef sithMessage_g_inputstream_ADDR
#undef sithTime_g_clockTime_ADDR
#undef std_g_genBuffer_ADDR

#define sithPlayer_g_numPlayers_ADDR ((uintptr_t)&sithMultiTest_g.numPlayers)
#define sithPlayer_g_pLocalPlayerThing_ADDR ((uintptr_t)&sithMultiTest_g.pLocalPlayerThing)
#define sithMessage_g_localPlayerId_ADDR ((uintptr_t)&sithMultiTest_g.localPlayerId)
#define sithMessage_g_outputstream_ADDR ((uintptr_t)&sithMultiTest_g.outputstream)
#define sithMessage_g_inputstream_ADDR ((uintptr_t)&sithMultiTest_g.inputstream)
#define sithTime_g_clockTime_ADDR ((uintptr_t)&sithMultiTest_g.clockTime)
#define std_g_genBuffer_ADDR ((uintptr_t)&sithMultiTest_g.genBuffer)

#endif // SITHMULTITESTADDRS_H

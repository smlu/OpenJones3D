// Force-included into the sithMulti tests and sithMultiTested.c (see CMakeLists.txt).
//
// Redirects the engine globals of the original executable that sithMulti.c reaches through J3D_DECL_FAR_VAR to static
// memory, in addition to the globals of the DSS tests (sithDSSTestAddrs.h, whose state builder the sithMulti cases
// reuse). The capture harness does not use this header.
#ifndef SITHMULTITESTADDRS_H
#define SITHMULTITESTADDRS_H

#include "sithDSSTestAddrs.h"

#include <sith/Devices/sithComm.h>
#include <std/RTI/addresses.h>

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

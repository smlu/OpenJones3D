#ifndef SITH_COG_TIMING_TEST_SEAMS_H
#define SITH_COG_TIMING_TEST_SEAMS_H

#include "sithEngineTestAddrs.h"
#include <sith/types.h>

// The COG timing tests run the real sleep and pulse code of sithCog.c, sithCogFunction.c and sithCogFunctionThing.c.
// Only popping the script arguments and running the script are replaced: Sleep, SetPulse and SetThingPulse get the
// interval the test sets, and a woken sleep loop sleeps again. The stubs are in sithFrameRateVec.h.
float J3DAPI sithFrameRateVec_PopFlex(SithCog* pCog);
#define sithCogExec_PopFlex sithFrameRateVec_PopFlex

SithThing* J3DAPI sithFrameRateVec_PopThing(SithCog* pCog);
#define sithCogExec_PopThing sithFrameRateVec_PopThing

void J3DAPI sithFrameRateVec_Execute(SithCog* pCog);
#define sithCogExec_Execute sithFrameRateVec_Execute

#endif

#ifndef SITH_SITHVEHICLECONTROLS_H
#define SITH_SITHVEHICLECONTROLS_H
#include <j3dcore/j3d.h>
#include <rdroid/types.h>
#include <sith/types.h>
#include <sith/Main/sithMain.h>
#include <std/types.h>

J3D_EXTERN_C_START

const SithMineCarControlsState* sithVehicleControls_GetMineCarState(void);
void J3DAPI sithVehicleControls_SetMineCarState(const SithMineCarControlsState* pState);

const SithRaftControlsState* sithVehicleControls_GetRaftState(void);
void J3DAPI sithVehicleControls_SetRaftState(const SithRaftControlsState* pState);

void J3DAPI sithVehicleControls_PuppetCallback(SithThing* pThing, int track, rdKeyMarkerType markerType);

void J3DAPI sithVehicleControls_ProcessMineCarMove(SithThing* pThing, float secDeltaTime);
void J3DAPI sithVehicleControls_ProcessJeepMove(SithThing* pThing, float secDeltaTime);
void J3DAPI sithVehicleControls_ProcessRaftMove(SithThing* pThing, float secDeltaTime);

void J3DAPI sithVehicleControls_StartBoardCutscene(SithThing* pThing);
void J3DAPI sithVehicleControls_EndBoardCutscene(SithThing* pThing);

// Helper hooking functions
void sithVehicleControls_ResetGlobals(void);

J3D_EXTERN_C_END
#endif // SITH_SITHVEHICLECONTROLS_H

#include "sithPhysics.h"
#include <j3dcore/j3dhook.h>
#include <sith/RTI/symbols.h>

#define sithPhysics_bMineCarEngineRunFx J3D_DECL_FAR_VAR(sithPhysics_bMineCarEngineRunFx, int)
#define sithPhysics_bMineCarEngineRumbleFx J3D_DECL_FAR_VAR(sithPhysics_bMineCarEngineRumbleFx, int)
#define sithPhysics_bMineCarSparksFx J3D_DECL_FAR_VAR(sithPhysics_bMineCarSparksFx, int)
#define sithPhysics_bMineCarClatterFx J3D_DECL_FAR_VAR(sithPhysics_bMineCarClatterFx, int)
#define sithPhysics_bMineCarRailClackFx J3D_DECL_FAR_VAR(sithPhysics_bMineCarRailClackFx, int)
#define sithPhysics_vecMinecarSparkPosLeft J3D_DECL_FAR_VAR(sithPhysics_vecMinecarSparkPosLeft, rdVector3)
#define sithPhysics_vecMinecarSparkPosRight J3D_DECL_FAR_VAR(sithPhysics_vecMinecarSparkPosRight, rdVector3)
#define sithPhysics_bUpdateTrackJeepEngineFx J3D_DECL_FAR_VAR(sithPhysics_bUpdateTrackJeepEngineFx, int)
#define sithPhysics_truckMinSndPitch J3D_DECL_FAR_VAR(sithPhysics_truckMinSndPitch, float)
#define sithPhysics_truckMaxSndPitch J3D_DECL_FAR_VAR(sithPhysics_truckMaxSndPitch, float)
#define sithPhysics_truckMinSndVolume J3D_DECL_FAR_VAR(sithPhysics_truckMinSndVolume, float)
#define sithPhysics_bUpdateTrackTruckEngineFx J3D_DECL_FAR_VAR(sithPhysics_bUpdateTrackTruckEngineFx, int)
#define sithPhysics_flt_538D04 J3D_DECL_FAR_VAR(sithPhysics_flt_538D04, float)
#define sithPhysics_dword_538D2C J3D_DECL_FAR_VAR(sithPhysics_dword_538D2C, int)
#define sithPhysics_dword_538D30 J3D_DECL_FAR_VAR(sithPhysics_dword_538D30, int)
#define sithPhysics_bJeepMoveFx J3D_DECL_FAR_VAR(sithPhysics_bJeepMoveFx, int)
#define sithPhysics_dword_538D38 J3D_DECL_FAR_VAR(sithPhysics_dword_538D38, int)
#define sithPhysics_dword_58540C J3D_DECL_FAR_VAR(sithPhysics_dword_58540C, int)
#define sithPhysics_flt_585410 J3D_DECL_FAR_VAR(sithPhysics_flt_585410, float)

void sithPhysics_InstallHooks(void)
{
    // Uncomment only lines for functions that have full definition and doesn't call original function (non-thunk functions)

    // J3D_HOOKFUNC(sithPhysics_FindFloor);
    // J3D_HOOKFUNC(sithPhysics_FindWaterSurface);
    // J3D_HOOKFUNC(sithPhysics_UpdateThing);
    // J3D_HOOKFUNC(sithPhysics_ApplyForce);
    // J3D_HOOKFUNC(sithPhysics_SetThingLook);
    // J3D_HOOKFUNC(sithPhysics_ApplyDrag);
    // J3D_HOOKFUNC(sithPhysics_ParseArg);
    // J3D_HOOKFUNC(sithPhysics_ResetThingMovement);
    // J3D_HOOKFUNC(sithPhysics_GetThingHeight);
    // J3D_HOOKFUNC(sithPhysics_UpdateThingPhysics);
    // J3D_HOOKFUNC(sithPhysics_UpdatePlayerPhysics);
    // J3D_HOOKFUNC(sithPhysics_UpdateUnderwaterThingPhysics);
    // J3D_HOOKFUNC(sithPhysics_UpdateClimbingThingPhysics);
    // J3D_HOOKFUNC(sithPhysics_UpdateAttachedThingPhysics);
    // J3D_HOOKFUNC(sithPhysics_CreateMineCarUserBlock);
    // J3D_HOOKFUNC(sithPhysics_InitMineCarFxState);
    // J3D_HOOKFUNC(sithPhysics_InitVehicleFxState);
    // J3D_HOOKFUNC(sithPhysics_InitDefaultMineCarInfo);
    // J3D_HOOKFUNC(sithPhysics_InitMineCarChassis);
    // J3D_HOOKFUNC(sithPhysics_InitTrackTruckChassis);
    // J3D_HOOKFUNC(sithPhysics_InitTrackJeepChassis);
    // J3D_HOOKFUNC(sithPhysics_InitMineCarExhaust);
    // J3D_HOOKFUNC(sithPhysics_InitJeepExhaust);
    // J3D_HOOKFUNC(sithPhysics_InitTrackTruckExhaust);
    // J3D_HOOKFUNC(sithPhysics_InitMineCarState);
    // J3D_HOOKFUNC(sithPhysics_InitTrackTruckState);
    // J3D_HOOKFUNC(sithPhysics_UpdateMineCarPhysics);
    // J3D_HOOKFUNC(sithPhysics_UpdateTrackVehicleFx);
    // J3D_HOOKFUNC(sithPhysics_UpdateMineCarFx);
    // J3D_HOOKFUNC(sithPhysics_UpdateTrackJeepFx);
    // J3D_HOOKFUNC(sithPhysics_UpdateTrackTruckFx);
    // J3D_HOOKFUNC(sithPhysics_UpdateVehicleChassis);
    // J3D_HOOKFUNC(sithPhysics_UpdateExhaustFx);
    // J3D_HOOKFUNC(sithPhysics_UpdateRaftPhysics);
    // J3D_HOOKFUNC(sithPhysics_GetWaterNormalAtPos);
    // J3D_HOOKFUNC(sithPhysics_GetWaterThrust);
    // J3D_HOOKFUNC(sithPhysics_ProcessMineCarTrackMove);
    // J3D_HOOKFUNC(sithPhysics_ProcessTrackFace);
    // J3D_HOOKFUNC(sithPhysics_CheckForPointOnTrack);
    // J3D_HOOKFUNC(sithPhysics_sub_487EC0);
    // J3D_HOOKFUNC(sithPhysics_UpdateJeepPhysics);
    // J3D_HOOKFUNC(sithPhysics_Jeep_sub_4892E0);
    // J3D_HOOKFUNC(sithPhysics_sub_48A970);
    // J3D_HOOKFUNC(sithPhysics_sub_48AD20);
    // J3D_HOOKFUNC(sithPhysics_CreateJeepUserBlock);
    // J3D_HOOKFUNC(sithPhysics_Jeep_sub_48B4D0);
    // J3D_HOOKFUNC(sithPhysics_UpdateJeepFx);
}

void sithPhysics_ResetGlobals(void)
{
    int sithPhysics_bMineCarEngineRunFx_tmp = 1;
    memcpy(&sithPhysics_bMineCarEngineRunFx, &sithPhysics_bMineCarEngineRunFx_tmp, sizeof(sithPhysics_bMineCarEngineRunFx));

    int sithPhysics_bMineCarEngineRumbleFx_tmp = 1;
    memcpy(&sithPhysics_bMineCarEngineRumbleFx, &sithPhysics_bMineCarEngineRumbleFx_tmp, sizeof(sithPhysics_bMineCarEngineRumbleFx));

    int sithPhysics_bMineCarSparksFx_tmp = 1;
    memcpy(&sithPhysics_bMineCarSparksFx, &sithPhysics_bMineCarSparksFx_tmp, sizeof(sithPhysics_bMineCarSparksFx));

    int sithPhysics_bMineCarClatterFx_tmp = 1;
    memcpy(&sithPhysics_bMineCarClatterFx, &sithPhysics_bMineCarClatterFx_tmp, sizeof(sithPhysics_bMineCarClatterFx));

    int sithPhysics_bMineCarRailClackFx_tmp = 1;
    memcpy(&sithPhysics_bMineCarRailClackFx, &sithPhysics_bMineCarRailClackFx_tmp, sizeof(sithPhysics_bMineCarRailClackFx));

    rdVector3 sithPhysics_vecMinecarSparkPosLeft_tmp = { { 0.050000001f }, { -0.039999999f }, { -0.090000004f } };
    memcpy(&sithPhysics_vecMinecarSparkPosLeft, &sithPhysics_vecMinecarSparkPosLeft_tmp, sizeof(sithPhysics_vecMinecarSparkPosLeft));

    rdVector3 sithPhysics_vecMinecarSparkPosRight_tmp = { { -0.050000001f }, { -0.039999999f }, { -0.090000004f } };
    memcpy(&sithPhysics_vecMinecarSparkPosRight, &sithPhysics_vecMinecarSparkPosRight_tmp, sizeof(sithPhysics_vecMinecarSparkPosRight));

    int sithPhysics_bUpdateTrackJeepEngineFx_tmp = 1;
    memcpy(&sithPhysics_bUpdateTrackJeepEngineFx, &sithPhysics_bUpdateTrackJeepEngineFx_tmp, sizeof(sithPhysics_bUpdateTrackJeepEngineFx));

    float sithPhysics_truckMinSndPitch_tmp = 1.0f;
    memcpy(&sithPhysics_truckMinSndPitch, &sithPhysics_truckMinSndPitch_tmp, sizeof(sithPhysics_truckMinSndPitch));

    float sithPhysics_truckMaxSndPitch_tmp = 2.0f;
    memcpy(&sithPhysics_truckMaxSndPitch, &sithPhysics_truckMaxSndPitch_tmp, sizeof(sithPhysics_truckMaxSndPitch));

    float sithPhysics_truckMinSndVolume_tmp = 0.60000002f;
    memcpy(&sithPhysics_truckMinSndVolume, &sithPhysics_truckMinSndVolume_tmp, sizeof(sithPhysics_truckMinSndVolume));

    int sithPhysics_bUpdateTrackTruckEngineFx_tmp = 1;
    memcpy(&sithPhysics_bUpdateTrackTruckEngineFx, &sithPhysics_bUpdateTrackTruckEngineFx_tmp, sizeof(sithPhysics_bUpdateTrackTruckEngineFx));

    float sithPhysics_flt_538D04_tmp = 0.60000002f;
    memcpy(&sithPhysics_flt_538D04, &sithPhysics_flt_538D04_tmp, sizeof(sithPhysics_flt_538D04));

    int sithPhysics_dword_538D2C_tmp = 1;
    memcpy(&sithPhysics_dword_538D2C, &sithPhysics_dword_538D2C_tmp, sizeof(sithPhysics_dword_538D2C));

    int sithPhysics_dword_538D30_tmp = 1;
    memcpy(&sithPhysics_dword_538D30, &sithPhysics_dword_538D30_tmp, sizeof(sithPhysics_dword_538D30));

    int sithPhysics_bJeepMoveFx_tmp = 1;
    memcpy(&sithPhysics_bJeepMoveFx, &sithPhysics_bJeepMoveFx_tmp, sizeof(sithPhysics_bJeepMoveFx));

    int sithPhysics_dword_538D38_tmp = 1;
    memcpy(&sithPhysics_dword_538D38, &sithPhysics_dword_538D38_tmp, sizeof(sithPhysics_dword_538D38));

    memset(&sithPhysics_dword_58540C, 0, sizeof(sithPhysics_dword_58540C));
    memset(&sithPhysics_flt_585410, 0, sizeof(sithPhysics_flt_585410));
}

void J3DAPI sithPhysics_FindFloor(SithThing* pThing, int bNoThingStateUpdate)
{
    J3D_TRAMPOLINE_CALL(sithPhysics_FindFloor, pThing, bNoThingStateUpdate);
}

void J3DAPI sithPhysics_FindWaterSurface(SithThing* pThing)
{
    J3D_TRAMPOLINE_CALL(sithPhysics_FindWaterSurface, pThing);
}

void J3DAPI sithPhysics_UpdateThing(SithThing* pThing, float secDeltaTime)
{
    J3D_TRAMPOLINE_CALL(sithPhysics_UpdateThing, pThing, secDeltaTime);
}

void J3DAPI sithPhysics_ApplyForce(SithThing* pThing, const rdVector3* force)
{
    J3D_TRAMPOLINE_CALL(sithPhysics_ApplyForce, pThing, force);
}

void J3DAPI sithPhysics_SetThingLook(SithThing* pThing, const rdVector3* pNormal, float secDeltaTime)
{
    J3D_TRAMPOLINE_CALL(sithPhysics_SetThingLook, pThing, pNormal, secDeltaTime);
}

void J3DAPI sithPhysics_ApplyDrag(rdVector3* pVelocity, float drag, float mag, float secDeltaTime)
{
    J3D_TRAMPOLINE_CALL(sithPhysics_ApplyDrag, pVelocity, drag, mag, secDeltaTime);
}

int J3DAPI sithPhysics_ParseArg(const StdConffileArg* pArg, SithThing* pThing, int adjNum)
{
    return J3D_TRAMPOLINE_CALL(sithPhysics_ParseArg, pArg, pThing, adjNum);
}

void J3DAPI sithPhysics_ResetThingMovement(SithThing* pThing)
{
    J3D_TRAMPOLINE_CALL(sithPhysics_ResetThingMovement, pThing);
}

float J3DAPI sithPhysics_GetThingHeight(const SithThing* pThing)
{
    return J3D_TRAMPOLINE_CALL(sithPhysics_GetThingHeight, pThing);
}

// Updates physics include gravity effect on thing
void J3DAPI sithPhysics_UpdateThingPhysics(SithThing* pThing, float secDeltaTime)
{
    J3D_TRAMPOLINE_CALL(sithPhysics_UpdateThingPhysics, pThing, secDeltaTime);
}

// update player physics in the air or water/aeterium
// local variable allocation has failed, the output may be wrong!
void J3DAPI sithPhysics_UpdatePlayerPhysics(SithThing* pThing, float msDeltaTime)
{
    J3D_TRAMPOLINE_CALL(sithPhysics_UpdatePlayerPhysics, pThing, msDeltaTime);
}

void J3DAPI sithPhysics_UpdateUnderwaterThingPhysics(SithThing* pThing, float secDeltaTime)
{
    J3D_TRAMPOLINE_CALL(sithPhysics_UpdateUnderwaterThingPhysics, pThing, secDeltaTime);
}

void J3DAPI sithPhysics_UpdateClimbingThingPhysics(SithThing* pThing, float secDeltaTime)
{
    J3D_TRAMPOLINE_CALL(sithPhysics_UpdateClimbingThingPhysics, pThing, secDeltaTime);
}

void J3DAPI sithPhysics_UpdateAttachedThingPhysics(SithThing* pThing, float secDeltaTime)
{
    J3D_TRAMPOLINE_CALL(sithPhysics_UpdateAttachedThingPhysics, pThing, secDeltaTime);
}

int J3DAPI sithPhysics_CreateMineCarUserBlock(SithThing* pMineCar)
{
    return J3D_TRAMPOLINE_CALL(sithPhysics_CreateMineCarUserBlock, pMineCar);
}

void J3DAPI sithPhysics_InitMineCarFxState(SithThing* pThing, SithMineCarFxState* pFxState)
{
    J3D_TRAMPOLINE_CALL(sithPhysics_InitMineCarFxState, pThing, pFxState);
}

void J3DAPI sithPhysics_InitVehicleFxState(SithThing* pThing, SithVehicleEngineFxState* pFxState)
{
    J3D_TRAMPOLINE_CALL(sithPhysics_InitVehicleFxState, pThing, pFxState);
}

void J3DAPI sithPhysics_InitDefaultMineCarInfo(SithThing* pThing, void* pInfo)
{
    J3D_TRAMPOLINE_CALL(sithPhysics_InitDefaultMineCarInfo, pThing, pInfo);
}

void J3DAPI sithPhysics_InitMineCarChassis(SithThing* pThing, SithVehicleChassisInfo* pChassisInfo)
{
    J3D_TRAMPOLINE_CALL(sithPhysics_InitMineCarChassis, pThing, pChassisInfo);
}

void J3DAPI sithPhysics_InitTrackTruckChassis(SithThing* pThing, SithVehicleChassisInfo* pChassisInfo)
{
    J3D_TRAMPOLINE_CALL(sithPhysics_InitTrackTruckChassis, pThing, pChassisInfo);
}

void J3DAPI sithPhysics_InitTrackJeepChassis(SithThing* pThing, SithVehicleChassisInfo* pChassisInfo)
{
    J3D_TRAMPOLINE_CALL(sithPhysics_InitTrackJeepChassis, pThing, pChassisInfo);
}

void J3DAPI sithPhysics_InitMineCarExhaust(SithThing* pThing, SithVehicleExhaustInfo* pExhaustInfo)
{
    J3D_TRAMPOLINE_CALL(sithPhysics_InitMineCarExhaust, pThing, pExhaustInfo);
}

void J3DAPI sithPhysics_InitJeepExhaust(SithThing* pThing, SithVehicleExhaustInfo* pExhaustInfo)
{
    J3D_TRAMPOLINE_CALL(sithPhysics_InitJeepExhaust, pThing, pExhaustInfo);
}

void J3DAPI sithPhysics_InitTrackTruckExhaust(SithThing* pThing, SithVehicleExhaustInfo* pExhaustInfo)
{
    J3D_TRAMPOLINE_CALL(sithPhysics_InitTrackTruckExhaust, pThing, pExhaustInfo);
}

void J3DAPI sithPhysics_InitMineCarState(SithThing* pThing, SithMineCarState* pState)
{
    J3D_TRAMPOLINE_CALL(sithPhysics_InitMineCarState, pThing, pState);
}

void J3DAPI sithPhysics_InitTrackTruckState(SithThing* pThing, SithMineCarState* pState)
{
    J3D_TRAMPOLINE_CALL(sithPhysics_InitTrackTruckState, pThing, pState);
}

void J3DAPI sithPhysics_UpdateMineCarPhysics(SithThing* pThing, float secDeltaTime)
{
    J3D_TRAMPOLINE_CALL(sithPhysics_UpdateMineCarPhysics, pThing, secDeltaTime);
}

void J3DAPI sithPhysics_UpdateTrackVehicleFx(SithThing* pThing, SithMineCarUserBlock* pMineCarUserBlock, float secDeltaTime)
{
    J3D_TRAMPOLINE_CALL(sithPhysics_UpdateTrackVehicleFx, pThing, pMineCarUserBlock, secDeltaTime);
}

void J3DAPI sithPhysics_UpdateMineCarFx(SithThing* pThing, SithMineCarFxState* pFxState, float secDeltaTime)
{
    J3D_TRAMPOLINE_CALL(sithPhysics_UpdateMineCarFx, pThing, pFxState, secDeltaTime);
}

void J3DAPI sithPhysics_UpdateTrackJeepFx(SithThing* pThing, SithVehicleEngineFxState* pFxState, float secDeltaTime)
{
    J3D_TRAMPOLINE_CALL(sithPhysics_UpdateTrackJeepFx, pThing, pFxState, secDeltaTime);
}

void J3DAPI sithPhysics_UpdateTrackTruckFx(SithThing* pThing, SithVehicleEngineFxState* pFxState, float secDeltaTime)
{
    J3D_TRAMPOLINE_CALL(sithPhysics_UpdateTrackTruckFx, pThing, pFxState, secDeltaTime);
}

void J3DAPI sithPhysics_UpdateVehicleChassis(SithThing* pThing, SithVehicleChassisInfo* pChasisInfo, float secDeltaTime)
{
    J3D_TRAMPOLINE_CALL(sithPhysics_UpdateVehicleChassis, pThing, pChasisInfo, secDeltaTime);
}

void J3DAPI sithPhysics_UpdateExhaustFx(SithThing* pThing, SithVehicleExhaustInfo* pExhaustInfo, float secDeltaTime)
{
    J3D_TRAMPOLINE_CALL(sithPhysics_UpdateExhaustFx, pThing, pExhaustInfo, secDeltaTime);
}

// update physics
void J3DAPI sithPhysics_UpdateRaftPhysics(SithThing* pThing, float secDeltaTime)
{
    J3D_TRAMPOLINE_CALL(sithPhysics_UpdateRaftPhysics, pThing, secDeltaTime);
}

int J3DAPI sithPhysics_GetWaterNormalAtPos(const SithThing* pThing, float* pOutDistance, rdVector3* pOutNormal, const rdVector3* pPos, float secDeltaTime)
{
    return J3D_TRAMPOLINE_CALL(sithPhysics_GetWaterNormalAtPos, pThing, pOutDistance, pOutNormal, pPos, secDeltaTime);
}

void J3DAPI sithPhysics_GetWaterThrust(SithThing* pThing, rdVector3* a2, rdVector3* pOutThrust, float msDeltaTime)
{
    J3D_TRAMPOLINE_CALL(sithPhysics_GetWaterThrust, pThing, a2, pOutThrust, msDeltaTime);
}

void J3DAPI sithPhysics_ProcessMineCarTrackMove(SithThing* pThing, float secDeltaTime, float a3, rdVector3* a4)
{
    J3D_TRAMPOLINE_CALL(sithPhysics_ProcessMineCarTrackMove, pThing, secDeltaTime, a3, a4);
}

int J3DAPI sithPhysics_ProcessTrackFace(SithThing* pThing, rdFace* pFace, rdVector3* a3, rdVector3* a4, rdVector3* a5, void* pData)
{
    return J3D_TRAMPOLINE_CALL(sithPhysics_ProcessTrackFace, pThing, pFace, a3, a4, a5, pData);
}

int J3DAPI sithPhysics_CheckForPointOnTrack(SithThing* pThing, rdVector3* a2, rdFace** ppOutFoundFace, int bUpdateState, rdFace* pPrevFace)
{
    return J3D_TRAMPOLINE_CALL(sithPhysics_CheckForPointOnTrack, pThing, a2, ppOutFoundFace, bUpdateState, pPrevFace);
}

int J3DAPI sithPhysics_sub_487EC0(SithThing* pThing, void* pData, float* secDeltaTime, rdVector3* a4, rdVector3* a5, float a6)
{
    return J3D_TRAMPOLINE_CALL(sithPhysics_sub_487EC0, pThing, pData, secDeltaTime, a4, a5, a6);
}

void J3DAPI sithPhysics_UpdateJeepPhysics(SithThing* pThing, float secDeltaTime)
{
    J3D_TRAMPOLINE_CALL(sithPhysics_UpdateJeepPhysics, pThing, secDeltaTime);
}

int J3DAPI sithPhysics_Jeep_sub_4892E0(SithThing* pThing, void* a2, float secDeltaTime)
{
    return J3D_TRAMPOLINE_CALL(sithPhysics_Jeep_sub_4892E0, pThing, a2, secDeltaTime);
}

// function might check for jeep surface and thing face collision
void J3DAPI sithPhysics_sub_48A970(SithThing* pThing, void* a2, const rdVector3* a3)
{
    J3D_TRAMPOLINE_CALL(sithPhysics_sub_48A970, pThing, a2, a3);
}

int J3DAPI sithPhysics_sub_48AD20(SithThing* pThing, void* a2, rdVector3* a3, float secDeltaTime)
{
    return J3D_TRAMPOLINE_CALL(sithPhysics_sub_48AD20, pThing, a2, a3, secDeltaTime);
}

signed int J3DAPI sithPhysics_CreateJeepUserBlock(SithThing* pThing)
{
    return J3D_TRAMPOLINE_CALL(sithPhysics_CreateJeepUserBlock, pThing);
}

void J3DAPI sithPhysics_Jeep_sub_48B4D0(SithThing* pThing, float* a2)
{
    J3D_TRAMPOLINE_CALL(sithPhysics_Jeep_sub_48B4D0, pThing, a2);
}

void J3DAPI sithPhysics_UpdateJeepFx(SithThing* pThing, SithVehicleEngineFxState* pFxState, float a3)
{
    J3D_TRAMPOLINE_CALL(sithPhysics_UpdateJeepFx, pThing, pFxState, a3);
}

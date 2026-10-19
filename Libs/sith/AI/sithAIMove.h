#ifndef SITH_SITHAIMOVE_H
#define SITH_SITHAIMOVE_H
#include <j3dcore/j3d.h>
#include <rdroid/types.h>
#include <sith/types.h>
#include <sith/Main/sithMain.h>
#include <sith/RTI/addresses.h>
#include <std/types.h>

J3D_EXTERN_C_START

#define sithAIMove_g_flt_585464 J3D_DECL_FAR_VAR(sithAIMove_g_flt_585464, float)
// extern float sithAIMove_g_flt_585464;

#define sithAIMove_g_flt_585468 J3D_DECL_FAR_VAR(sithAIMove_g_flt_585468, float)
// extern float sithAIMove_g_flt_585468;

#define sithAIMove_g_flt_58546C J3D_DECL_FAR_VAR(sithAIMove_g_flt_58546C, float)
// extern float sithAIMove_g_flt_58546C;

#define sithAIMove_g_flt_585470 J3D_DECL_FAR_VAR(sithAIMove_g_flt_585470, float)
// extern float sithAIMove_g_flt_585470;

void J3DAPI sithAIMove_Update(SithThing* pThing, float secDeltatTime);
void J3DAPI sithAIMove_UpdateMineCar(SithAIControlBlock* pLocal, float secDeltaTime);
void J3DAPI sithAIMove_sub_4958B0(SithAIControlBlock* pLocal, float secDeltatTime);
void J3DAPI sithAIMove_sub_495CD0(SithAIControlBlock* pLocal, float secDeltaTime);
void J3DAPI sithAIMove_sub_4961A0(SithAIControlBlock* pLocal);
double J3DAPI sithAIMove_sub_496200(SithAIControlBlock* pLocal, rdVector3* a2, rdVector3* a3);
int J3DAPI sithAIMove_sub_496550(SithAIControlBlock* pLocal, float secDeltaTime);
int J3DAPI sithAIMove_sub_4966D0(SithAIControlBlock* pLocal, float secDeltaTime);
int J3DAPI sithAIMove_sub_496820(SithAIControlBlock* pLocal, float secDeltaTime);

int J3DAPI sithAIMove_AIGetMoveState(const SithAIControlBlock* pLocal);
int J3DAPI sithAIMove_AISpecialTurn(SithAIControlBlock* pLocal, float angle);

float J3DAPI sithAIMove_UpdateAIMove(SithAIControlBlock* pLocal);

void J3DAPI sithAIMove_GetAIMoveModes(const SithThing* pThing, float moveSpeed, int moveDirection, SithThingMoveStatus* pOutMoveStatus, SithPuppetSubMode* pOutSubmode);
void J3DAPI sithAIMove_SetSubMode(SithAIControlBlock* pLocal, SithThingMoveStatus moveStatus, SithPuppetSubMode submode);
void J3DAPI sithAIMove_PuppetCallback(SithThing* pThing, int trackNum, rdKeyMarkerType marker);
void J3DAPI sithAIMove_AISetLookThing(SithAIControlBlock* pLocal, const SithThing* pThing);
void J3DAPI sithAIMove_AISetLookPos(SithAIControlBlock* pLocal, const rdVector3* targetPos);
void J3DAPI sithAIMove_AISetLookPosEyeLevel(SithAIControlBlock* pLocal, const rdVector3* targetPos);
int J3DAPI sithAIMove_AISetMovePos(SithAIControlBlock* pLocal, const rdVector3* moveToPos, float moveSpeed);
int J3DAPI sithAIMove_AISetMoveTargetPos(SithAIControlBlock* pLocal, const rdVector3* moveToPos, float moveSpeed);
int J3DAPI sithAIMove_AISetMoveTargetPos2(SithAIControlBlock* pLocal, const rdVector3* moveToPos, float moveSpeed); // Added from debug
int J3DAPI sithAIMove_AISpecialMove(SithAIControlBlock* pLocal, SithActorSpecialMoveFlags moveFlags);
void J3DAPI sithAIMove_AIJump(SithAIControlBlock* pLocal, rdVector3* movePos, float a3);
void J3DAPI sithAIMove_AIStop(SithAIControlBlock* pLocal);
void J3DAPI sithAIMove_SetGoalReached(SithAIControlBlock* pLocal);
void J3DAPI sithAIMove_Unreachable(SithAIControlBlock* pLocal);
void J3DAPI sithAIMove_StopAIMovement(SithAIControlBlock* pLocal);
void J3DAPI sithAIMove_ResetAILook(SithAIControlBlock* pLocal);

void J3DAPI sithAIMove_UpdateBoss(SithAIControlBlock* pLocal, float secDeltaTime);
void J3DAPI sithAIMove_sub_499090(SithAIControlBlock* pLocal, float secDeltaTime);
void J3DAPI sithAIMove_sub_4996C0(SithAIControlBlock* pLocal, float a2);
void J3DAPI sithAIMove_sub_499A80(SithAIControlBlock* pLocal, float* pDestAngle, float* a3, float* a4, float* a5);
void J3DAPI sithAIMove_sub_499CA0(SithAIControlBlock* pLocal, float secDeltaTime);
void J3DAPI sithAIMove_sub_49A020(SithAIControlBlock* pLocal, float* angle, float* a3, float* a4, float* a5);
void J3DAPI sithAIMove_sub_49A1B0(SithAIControlBlock* pLocal, float secDeltaTime);
void J3DAPI sithAIMove_sub_49A450(SithAIControlBlock* pLocal, float secDeltaTime);
void J3DAPI sithAIMove_sub_49A630(SithAIControlBlock* pLocal, float secDeltaTime);
void J3DAPI sithAIMove_sub_49A810(SithAIControlBlock* pLocal, float secDeltaTime);
void J3DAPI sithAIMove_UpdateQuetzTail(SithAIControlBlock* pLocal, float secDeltaTime);
void J3DAPI sithAIMove_sub_49AA60(SithAIControlBlock* pLocal);
void J3DAPI sithAIMove_sub_49AB80(SithAIControlBlock* pLocal);
void J3DAPI sithAIMove_sub_49AC50(SithAIControlBlock* pLocal);
void J3DAPI sithAIMove_sub_49AF80(SithAIControlBlock* pLocal);
void J3DAPI sithAIMove_sub_49B1B0(SithAIControlBlock* pLocal);
void J3DAPI sithAIMove_UpdateMardukTail(SithThing* pThing, float secDeltaTime);

// Helper hooking functions
void sithAIMove_InstallHooks(void);
void sithAIMove_ResetGlobals(void);

J3D_EXTERN_C_END
#endif // SITH_SITHAIMOVE_H

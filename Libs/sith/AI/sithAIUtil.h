#ifndef SITH_SITHAIUTIL_H
#define SITH_SITHAIUTIL_H
#include <j3dcore/j3d.h>
#include <rdroid/types.h>
#include <sith/types.h>
#include <sith/Main/sithMain.h>
#include <sith/RTI/addresses.h>
#include <std/types.h>

J3D_EXTERN_C_START

#define sithAIUtil_g_bRenderAIWpnts J3D_DECL_FAR_VAR(sithAIUtil_g_bRenderAIWpnts, int)
// extern int sithAIUtil_g_bRenderAIWpnts;

void J3DAPI sithAIUtil_UpdateTargetSight(SithAIControlBlock* pLocal, SithAIUtilFireFlags flags);
void J3DAPI sithAIUtil_UpdateGoalSight(SithAIControlBlock* pLocal);
// returns target sight state:
// 0 - target in sight
// 1 - target too far away (dist > sightDistance)
// 2 - target out of FOV view
// 3 - target not in sight
int J3DAPI sithAIUtil_GetDistanceToTarget(SithThing* pViewer, rdVector3* startPos, const SithThing* pTarget, float fov, float sightDistance, float collisionSize, rdVector3* toTarget, float* pDist);
int J3DAPI sithAIUtil_GetDistanceToTargetPos(SithThing* pViewer, rdVector3* startPos, rdVector3* endPos, float fov, float sightDistance, float collisionSize, rdVector3* toTarget, float* pDist);
int J3DAPI sithAIUtil_AIFire(SithAIControlBlock* pLocal, float minDist, float maxDist, float fireDot, float aimError, int weaponNum, SithAIUtilFireFlags flags, int burstCount);
void J3DAPI sithAIUtil_AIPauseMove(SithAIControlBlock* pLocal, int msecPause);
void sithAIUtil_AIResetWaypoints(void);
int J3DAPI sithAIUtil_SyncWpnts(DPID idTo, unsigned int outstream);
int J3DAPI sithAIUtil_ProcessSyncWpnts(const SithMessage* pMsg);
void J3DAPI sithAIUtil_AISetWpnt(SithThing* pGhost, unsigned int wpntIdx);
SithThing* J3DAPI sithAIUtil_AIGetWpntThing(int wpntIdx);
void J3DAPI sithAIUtil_AISetWpntRank(unsigned int wpntIdx, unsigned int rank);
void J3DAPI sithAIUtil_AISetWpntFlags(unsigned int wpntIdx, SithAIWaypointLayerFlag flags);
void J3DAPI sithAIUtil_AIClearWpntFlags(unsigned int wpntIdx, SithAIWaypointLayerFlag flags);
void J3DAPI sithAIUtil_AISetActiveWpntLayer(SithAIWaypointLayerFlag layer);
void J3DAPI sithAIUtil_AIConnectWpnts(unsigned int wpntIdx1, unsigned int wpntIdx2, int bConnectWpnt1To2);
int J3DAPI sithAIUtil_AIMoveToNextWpnt(SithAIControlBlock* pLocal, float moveSpeed, float degTurn, int mode);
void J3DAPI sithAIUtil_AITraverseWpnts(SithAIControlBlock* pLocal, int wpntIdx, float moveSpeed, float degTurn, int mode);
int J3DAPI sithAIUtil_AIAdvanceToNextWpnt(SithAIControlBlock* pLocal, int bNotify);
int J3DAPI sithAIUtil_SelectWpntLink(SithAIControlBlock* pLocal, unsigned int numLinks, int* aWypointLinks, int linkNum, int wpntOwnerNum);
void J3DAPI sithAIUtil_CheckWpntMoveAlign(SithAIControlBlock* pLocal);
int J3DAPI sithAIUtil_AIFindNearestWpnt(SithThing* pThing, int bCheckPath);
void J3DAPI sithAIUtil_RankWpntsFromPlayer();
void J3DAPI sithAIUtil_RankWpntsFromThing(SithThing* pThing);
int J3DAPI sithAIUtil_CreateAIWaypoint(SithAIControlBlock* pLocal, float degTurn);
void J3DAPI sithAIUtil_ClearAIWaypoint(SithAIControlBlock* pLocal);
int J3DAPI sithAIUtil_GetAIWaypointNum(SithAIControlBlock* pLocal);
int J3DAPI sithAIUtil_CompareAIWpntDistances(const SithAIWaypointDistance* pDist1, const SithAIWaypointDistance* pDist2);
void sithAIUtil_RenderAIWaypoints(void);
int J3DAPI sithAIUtil_CheckPathToPoint(SithThing* pViewer, const rdVector3* pTarget, float radius, float* pDistance, rdVector3* pHitNorm, int bDetectThings, int bSkipFloor);
int J3DAPI sithAIUtil_CheckPosition(const SithAIControlBlock* pLocal, rdVector3* endPos, int* pbNoCollision);
int J3DAPI sithAIUtil_CheckFloorAtPos(const SithAIControlBlock* pLocal, const rdVector3* startPos, SithSector* pStartSector, int* pbNoCollision);
int J3DAPI sithAIUtil_MakeRandPoint(SithAIControlBlock* pLocal, rdVector3* pPos, int flags, float angle, float* pDownCos, rdVector3* pDir, rdVector3* pDestPoint);
int J3DAPI sithAIUtil_RetryMakeRandPoint(SithAIControlBlock* pLocal, rdVector3* pPos, float downCos, rdVector3* pDirection, rdVector3* pDestPoint);
int J3DAPI sithAIUtil_CheckPathToPos(SithAIControlBlock* pLocal, SithSector* pStartSector, rdVector3* startPos, int flags, float minMoveDist, float moveDist, float radius, rdVector3* pDirection, rdVector3* pEndPos);
int J3DAPI sithAIUtil_SweepMoveToPathPos(SithAIControlBlock* pLocal, rdVector3* pPos, float angle, float maxAngle, float deltaAngle, int flags);
int J3DAPI sithAIUtil_MakeRoamPos(SithAIControlBlock* pLocal, rdVector3* pPos, float minDist, float moveDist, int bAllow3D, int maxRetries, rdVector3* pDestPos);
int J3DAPI sithAIUtil_MakePathPos(SithAIControlBlock* pLocal, rdVector3* pPos, float minDist, float moveDist, float angle, int flags, rdVector3* pDirection, rdVector3* pDestPos);
int J3DAPI sithAIUtil_IsLineOfFireClear(const SithAIControlBlock* pLocal, const rdVector3* pPos);
void J3DAPI sithAIUtil_AIPlaySoundMode(SithAIControlBlock* pLocal, SithSoundClassMode mode);
void J3DAPI sithAIUtil_AISetMode(SithAIControlBlock* pLocal, SithAIMode mode);
void J3DAPI sithAIUtil_AIClearMode(SithAIControlBlock* pLocal, SithAIMode mode);
void J3DAPI sithAIUtil_SetWeaponFireFlags(int weaponNum, SithAIUtilFireFlags* pFlags);
int J3DAPI sithAIUtil_AIGetMovePos(const SithAIControlBlock* pLocal, int flags, rdVector3* pDestPos);
signed int J3DAPI sithAIUtil_GetXYHeadingVector(const SithThing* pThing, rdVector3* pDest);
signed int J3DAPI sithAIUtil_GetXYZHeadingVector(const SithThing* pThing, rdVector3* pDest);
int J3DAPI sithAIUtil_CanAttackTarget(const SithThing* pThing);
int J3DAPI sithAIUtil_IsThingMoving(const SithThing* pThing);
void J3DAPI sithAIUtil_ApplyForce(SithAIControlBlock* pLocal, const rdVector3* pDirection, float force);
void J3DAPI sithAIUtil_RandomRotate(rdVector3* pOutPYR, float scalar);
size_t J3DAPI sithAIUtil_GetThingsInView(SithSector* pStartSector, const rdMatrix34* orient, float fovX, float fovY, int sizeResult, SithThing** pResult, int thingTypeMask, float maxDistance);
void J3DAPI sithAIUtil_GetNextThingsInView(SithSector* pSector, const rdMatrix34* pOrient, float distance);
int J3DAPI sithAIUtil_CanSeeTarget(const SithAIControlBlock* pLocal, SithThing* pTarget, float distance);

// Helper hooking functions
void sithAIUtil_InstallHooks(void);
void sithAIUtil_ResetGlobals(void);

J3D_EXTERN_C_END
#endif // SITH_SITHAIUTIL_H

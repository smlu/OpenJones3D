#include "sithAIUtil.h"
#include <j3dcore/j3dhook.h>
#include <sith/RTI/symbols.h>

#include <sith/AI/sithAI.h>
#include <sith/AI/sithAIMove.h>
#include <sith/Cog/sithCog.h>
#include <sith/Devices/sithComm.h>
#include <sith/Dss/sithDSS.h>
#include <sith/Dss/sithMulti.h>
#include <sith/Engine/sithCollision.h>
#include <sith/Engine/sithPhysics.h>
#include <sith/Engine/sithPuppet.h>
#include <sith/Gameplay/sithFX.h>
#include <sith/Gameplay/sithInventory.h>
#include <sith/Gameplay/sithPlayer.h>
#include <sith/Gameplay/sithTime.h>
#include <sith/Main/sithMain.h>
#include <sith/World/sithActor.h>
#include <sith/World/sithSoundClass.h>
#include <sith/World/sithWeapon.h>
#include <sith/World/sithWorld.h>

#include <rdroid/Engine/rdLight.h>
#include <rdroid/Math/rdMath.h>
#include <rdroid/Math/rdMatrix.h>
#include <rdroid/Math/rdVector.h>

#include <std/General/std.h>
#include <std/General/stdMath.h>
#include <std/General/stdUtil.h>

#include <float.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define sithAIUtil_thingsInViewFovY J3D_DECL_FAR_VAR(sithAIUtil_thingsInViewFovY, float)
#define sithAIUtil_numVisitedSectors J3D_DECL_FAR_VAR(sithAIUtil_numVisitedSectors, size_t)
#define sithAIUtil_activeWpntLayer J3D_DECL_FAR_VAR(sithAIUtil_activeWpntLayer, SithAIWaypointLayerFlag)
#define sithAIUtil_numThingsInView J3D_DECL_FAR_VAR(sithAIUtil_numThingsInView, size_t)
#define sithAIUtil_mkPointCurDir J3D_DECL_FAR_VAR(sithAIUtil_mkPointCurDir, rdVector3)
#define sithAIUtil_pMkPointCurLocal J3D_DECL_FAR_VAR(sithAIUtil_pMkPointCurLocal, SithAIControlBlock*)
#define sithAIUtil_mkPointCurPYR J3D_DECL_FAR_VAR(sithAIUtil_mkPointCurPYR, rdVector3)
#define sithAIUtil_thingsInViewFovX J3D_DECL_FAR_VAR(sithAIUtil_thingsInViewFovX, float)
#define sithAIUtil_aWpntOwners J3D_DECL_FAR_ARRAYVAR(sithAIUtil_aWpntOwners, SithAIWaypointOwner(*)[10])
#define sithAIUtil_aThingsInView J3D_DECL_FAR_VAR(sithAIUtil_aThingsInView, SithThing**)
#define sithAIUtil_aWpntDistances J3D_DECL_FAR_ARRAYVAR(sithAIUtil_aWpntDistances, SithAIWaypointDistance(*)[60])
#define sithAIUtil_sizeThingsInView J3D_DECL_FAR_VAR(sithAIUtil_sizeThingsInView, size_t)
#define sithAIUtil_mkPointCurRVec J3D_DECL_FAR_VAR(sithAIUtil_mkPointCurRVec, rdVector3)
#define sithAIUtil_maxDistanceToThingsInView J3D_DECL_FAR_VAR(sithAIUtil_maxDistanceToThingsInView, float)
#define sithAIUtil_thingInViewTypeMask J3D_DECL_FAR_VAR(sithAIUtil_thingInViewTypeMask, int)
#define sithAIUtil_aAIWpnts J3D_DECL_FAR_ARRAYVAR(sithAIUtil_aAIWpnts, SithAIWaypoint(*)[60])
#define sithAIUtil_secLastUpdate J3D_DECL_FAR_VAR(sithAIUtil_secLastUpdate, float)
#define sithAIUtil_mkPoinCurFlags J3D_DECL_FAR_VAR(sithAIUtil_mkPoinCurFlags, int)

void sithAIUtil_InstallHooks(void)
{
    J3D_HOOKFUNC(sithAIUtil_UpdateTargetSight);
    J3D_HOOKFUNC(sithAIUtil_UpdateGoalSight);
    J3D_HOOKFUNC(sithAIUtil_GetDistanceToTarget);
    J3D_HOOKFUNC(sithAIUtil_GetDistanceToTargetPos);
    J3D_HOOKFUNC(sithAIUtil_AIFire);
    J3D_HOOKFUNC(sithAIUtil_AIPauseMove);
    J3D_HOOKFUNC(sithAIUtil_AIResetWaypoints);
    J3D_HOOKFUNC(sithAIUtil_SyncWpnts);
    J3D_HOOKFUNC(sithAIUtil_ProcessSyncWpnts);
    J3D_HOOKFUNC(sithAIUtil_AISetWpnt);
    J3D_HOOKFUNC(sithAIUtil_AIGetWpntThing);
    J3D_HOOKFUNC(sithAIUtil_AISetWpntRank);
    J3D_HOOKFUNC(sithAIUtil_AISetWpntFlags);
    J3D_HOOKFUNC(sithAIUtil_AIClearWpntFlags);
    J3D_HOOKFUNC(sithAIUtil_AISetActiveWpntLayer);
    J3D_HOOKFUNC(sithAIUtil_AIConnectWpnts);
    J3D_HOOKFUNC(sithAIUtil_AIMoveToNextWpnt);
    J3D_HOOKFUNC(sithAIUtil_AITraverseWpnts);
    J3D_HOOKFUNC(sithAIUtil_AIAdvanceToNextWpnt);
    J3D_HOOKFUNC(sithAIUtil_SelectWpntLink);
    J3D_HOOKFUNC(sithAIUtil_CheckWpntMoveAlign);
    J3D_HOOKFUNC(sithAIUtil_AIFindNearestWpnt);
    J3D_HOOKFUNC(sithAIUtil_RankWpntsFromPlayer);
    J3D_HOOKFUNC(sithAIUtil_RankWpntsFromThing);
    J3D_HOOKFUNC(sithAIUtil_CreateAIWaypoint);
    J3D_HOOKFUNC(sithAIUtil_ClearAIWaypoint);
    J3D_HOOKFUNC(sithAIUtil_GetAIWaypointNum);
    J3D_HOOKFUNC(sithAIUtil_CompareAIWpntDistances);
    J3D_HOOKFUNC(sithAIUtil_RenderAIWaypoints);
    J3D_HOOKFUNC(sithAIUtil_CheckPathToPoint);
    J3D_HOOKFUNC(sithAIUtil_CheckPosition);
    J3D_HOOKFUNC(sithAIUtil_CheckFloorAtPos);
    J3D_HOOKFUNC(sithAIUtil_MakeRandPoint);
    J3D_HOOKFUNC(sithAIUtil_RetryMakeRandPoint);
    J3D_HOOKFUNC(sithAIUtil_CheckPathToPos);
    J3D_HOOKFUNC(sithAIUtil_SweepMoveToPathPos);
    J3D_HOOKFUNC(sithAIUtil_MakeRoamPos);
    J3D_HOOKFUNC(sithAIUtil_MakePathPos);
    J3D_HOOKFUNC(sithAIUtil_IsLineOfFireClear);
    J3D_HOOKFUNC(sithAIUtil_AIPlaySoundMode);
    J3D_HOOKFUNC(sithAIUtil_AISetMode);
    J3D_HOOKFUNC(sithAIUtil_AIClearMode);
    J3D_HOOKFUNC(sithAIUtil_SetWeaponFireFlags);
    J3D_HOOKFUNC(sithAIUtil_AIGetMovePos);
    J3D_HOOKFUNC(sithAIUtil_GetXYHeadingVector);
    J3D_HOOKFUNC(sithAIUtil_GetXYZHeadingVector);
    J3D_HOOKFUNC(sithAIUtil_CanAttackTarget);
    J3D_HOOKFUNC(sithAIUtil_IsThingMoving);
    J3D_HOOKFUNC(sithAIUtil_ApplyForce);
    J3D_HOOKFUNC(sithAIUtil_RandomRotate);
    J3D_HOOKFUNC(sithAIUtil_GetThingsInView);
    J3D_HOOKFUNC(sithAIUtil_GetNextThingsInView);
    J3D_HOOKFUNC(sithAIUtil_CanSeeTarget);
}

void sithAIUtil_ResetGlobals(void)
{
    memset(&sithAIUtil_thingsInViewFovY, 0, sizeof(sithAIUtil_thingsInViewFovY));
    memset(&sithAIUtil_numVisitedSectors, 0, sizeof(sithAIUtil_numVisitedSectors));
    memset(&sithAIUtil_g_bRenderAIWpnts, 0, sizeof(sithAIUtil_g_bRenderAIWpnts));
    memset(&sithAIUtil_activeWpntLayer, 0, sizeof(sithAIUtil_activeWpntLayer));
    memset(&sithAIUtil_numThingsInView, 0, sizeof(sithAIUtil_numThingsInView));
    memset(&sithAIUtil_mkPointCurDir, 0, sizeof(sithAIUtil_mkPointCurDir));
    memset(&sithAIUtil_pMkPointCurLocal, 0, sizeof(sithAIUtil_pMkPointCurLocal));
    memset(&sithAIUtil_mkPointCurPYR, 0, sizeof(sithAIUtil_mkPointCurPYR));
    memset(&sithAIUtil_thingsInViewFovX, 0, sizeof(sithAIUtil_thingsInViewFovX));
    memset(&sithAIUtil_aWpntOwners, 0, sizeof(sithAIUtil_aWpntOwners));
    memset(&sithAIUtil_aThingsInView, 0, sizeof(sithAIUtil_aThingsInView));
    memset(&sithAIUtil_aWpntDistances, 0, sizeof(sithAIUtil_aWpntDistances));
    memset(&sithAIUtil_sizeThingsInView, 0, sizeof(sithAIUtil_sizeThingsInView));
    memset(&sithAIUtil_mkPointCurRVec, 0, sizeof(sithAIUtil_mkPointCurRVec));
    memset(&sithAIUtil_maxDistanceToThingsInView, 0, sizeof(sithAIUtil_maxDistanceToThingsInView));
    memset(&sithAIUtil_thingInViewTypeMask, 0, sizeof(sithAIUtil_thingInViewTypeMask));
    memset(&sithAIUtil_aAIWpnts, 0, sizeof(sithAIUtil_aAIWpnts));
    memset(&sithAIUtil_secLastUpdate, 0, sizeof(sithAIUtil_secLastUpdate));
    memset(&sithAIUtil_mkPoinCurFlags, 0, sizeof(sithAIUtil_mkPoinCurFlags));
}

void J3DAPI sithAIUtil_UpdateTargetSight(SithAIControlBlock* pLocal, SithAIUtilFireFlags flags)
{
    int sightState = pLocal->targetSightState;
    SithThing* pOwner = pLocal->pOwner;
    float curcollideSize = 0.0f;
    if ( (flags & (SITHAIUTIL_FIRE_LOB | SITHAIUTIL_FIRE_PRIMARY | SITHAIUTIL_FIRE_ALT)) == 0 && (size_t)pLocal->unknown124 == sithMain_g_frameNumber )
    {
        return;
    }

    pLocal->unknown124 = sithMain_g_frameNumber;
    if ( sithWeapon_HasWeaponSelected(pOwner) )
    {
        curcollideSize = sithWeapon_GetWeaponCollideSize(pOwner->thingInfo.actorInfo.weaponInfo.curWeaponID);
    }
    else if ( pOwner->thingInfo.actorInfo.pWeaponTemplate )
    {
        curcollideSize = pOwner->thingInfo.actorInfo.pWeaponTemplate->collide.movesize;
    }

    rdMatrix_TransformVector34(&pLocal->weaponFirePos, &pOwner->thingInfo.actorInfo.fireOffset, &pOwner->orient);
    rdVector_Add3Acc(&pLocal->weaponFirePos, &pOwner->pos);
    rdVector3 curstartPos;
    rdVector_Copy3(&curstartPos, &pLocal->weaponFirePos);

    if ( (pLocal->submode & SITHAI_SUBMODE_FIREADDEYEOFFSET) != 0
        && (pOwner->thingInfo.actorInfo.eyeOffset.x != 0.0f || pOwner->thingInfo.actorInfo.eyeOffset.y != 0.0f || pOwner->thingInfo.actorInfo.eyeOffset.z != 0.0f) )
    {
        curstartPos.z = pOwner->thingInfo.actorInfo.eyeOffset.z * 0.5f + curstartPos.z;
    }

    pLocal->mode &= ~SITHAI_MODE_TARGETVISIBLE;
    SithThing* pTarget = pLocal->pTargetThing;
    if ( pTarget )
    {
        if ( (pLocal->pOwner->thingInfo.actorInfo.flags & SITH_AF_SEEINVISIBLE) == 0 )
        {
            if ( (pTarget->type == SITH_THING_ACTOR || pTarget->type == SITH_THING_PLAYER) && (pTarget->thingInfo.actorInfo.flags & SITH_AF_INVISIBLE) != 0 )
            {
                sightState = 3;
            }

            if ( (pLocal->pOwner->thingInfo.actorInfo.flags & SITH_AF_BLIND) != 0 )
            {
                sightState = 3;
            }
        }

        rdVector_Copy3(&pLocal->targetPos, &pTarget->pos);
        pLocal->targetSightState = sithAIUtil_GetDistanceToTarget(pOwner, &curstartPos, pTarget, pLocal->pClass->fov, pLocal->pClass->sightDistance, curcollideSize, &pLocal->toTarget, &pLocal->distance);
        if ( pLocal->targetSightState == 0 )
        {
            if ( sightState && !sithAIUtil_CanSeeTarget(pLocal, pTarget, pLocal->distance) )
            {
                pLocal->targetSightState = 3;
                return;
            }

            rdVector_Copy3(&pLocal->vecUnknown122, &pTarget->pos);
            pLocal->msecAttackStart = sithTime_g_msecGameTime;
            pLocal->mode |= SITHAI_MODE_TARGETVISIBLE;
        }
    }
    else
    {
        pLocal->targetSightState = sithAIUtil_GetDistanceToTargetPos(pOwner, &curstartPos, &pLocal->targetPos, pLocal->pClass->fov, pLocal->pClass->sightDistance, curcollideSize, &pLocal->toTarget, &pLocal->distance);
        if ( pLocal->targetSightState == 0 )
        {
            rdVector_Copy3(&pLocal->vecUnknown122, &pLocal->targetPos);
            pLocal->msecAttackStart = sithTime_g_msecGameTime;
            pLocal->mode |= SITHAI_MODE_TARGETVISIBLE;
        }
    }

    if ( (pLocal->submode & SITHAI_SUBMODE_FIREADDEYEOFFSET) != 0
        && (pOwner->thingInfo.actorInfo.eyeOffset.x != 0.0f || pOwner->thingInfo.actorInfo.eyeOffset.y != 0.0f || pOwner->thingInfo.actorInfo.eyeOffset.z != 0.0f) )
    {
        rdVector_Sub3(&pLocal->toTarget, &pLocal->targetPos, &pLocal->weaponFirePos);
        if ( pTarget )
        {
            pLocal->distance = rdVector_Normalize3Acc(&pLocal->toTarget) - pTarget->collide.size;
        }
        else
        {
            pLocal->distance = rdVector_Normalize3Acc(&pLocal->toTarget);
        }

        pLocal->distance = J3DMAX(pLocal->distance, 0.0f);
    }
}

void J3DAPI sithAIUtil_UpdateGoalSight(SithAIControlBlock* pLocal)
{
    int curSightState = pLocal->targetSightState2;
    SithThing* pOwner = pLocal->pOwner;
    if ( (size_t)pLocal->unknown141 == sithMain_g_frameNumber )
    {
        return;
    }

    pLocal->unknown141 = sithMain_g_frameNumber;
    SithThing* pTarget = pLocal->goalThing;
    if ( pTarget )
    {
        if ( (pOwner->thingInfo.actorInfo.flags & SITH_AF_SEEINVISIBLE) == 0 )
        {
            if ( (pTarget->type == SITH_THING_ACTOR || pTarget->type == SITH_THING_PLAYER) && (pTarget->thingInfo.actorInfo.flags & SITH_AF_INVISIBLE) != 0 )
            {
                curSightState = 3;
            }

            if ( (pOwner->thingInfo.actorInfo.flags & SITH_AF_BLIND) != 0 )
            {
                curSightState = 3;
            }
        }

        pLocal->targetSightState2 = sithAIUtil_GetDistanceToTarget(pOwner, &pOwner->pos, pTarget, -1.0f, pLocal->pClass->sightDistance, 0.0f, &pLocal->vecUnknown0, &pLocal->targetDistance);
        if ( pLocal->targetSightState2 == 0 )
        {
            if ( curSightState && !sithAIUtil_CanSeeTarget(pLocal, pTarget, pLocal->targetDistance) )
            {
                pLocal->targetSightState2 = 3;
                return;
            }

            rdVector_Copy3(&pLocal->vecUnknown5, &pTarget->pos);
            pLocal->unknown150 = sithTime_g_msecGameTime;
        }
    }
    else
    {
        rdVector_Copy3(&pLocal->vecUnknown6, &pLocal->movePos);
        pLocal->targetSightState2 = sithAIUtil_GetDistanceToTargetPos(pOwner, &pOwner->pos, &pLocal->vecUnknown6, -1.0f, pLocal->pClass->sightDistance, 0.0f, &pLocal->vecUnknown0, &pLocal->targetDistance);
        if ( pLocal->targetSightState2 == 0 )
        {
            rdVector_Copy3(&pLocal->vecUnknown5, &pLocal->vecUnknown6);
            pLocal->unknown150 = sithTime_g_msecGameTime;
        }
    }
}

// returns target sight state:
// 0 - target in sight
// 1 - target too far away (dist > sightDistance)
// 2 - target out of FOV view
// 3 - target not in sight
int J3DAPI sithAIUtil_GetDistanceToTarget(SithThing* pViewer, rdVector3* startPos, const SithThing* pTarget, float fov, float sightDistance, float collisionSize, rdVector3* toTarget, float* pDist)
{
    J3D_UNUSED(collisionSize);

    SITH_ASSERTREL(toTarget && pDist);
    SITH_ASSERTREL(pViewer && pTarget && startPos);

    rdVector_Sub3(toTarget, &pTarget->pos, startPos);
    *pDist = rdVector_Normalize3Acc(toTarget) - pTarget->collide.size;
    *pDist = *pDist > 0.0f ? *pDist : 0.0f;

    if ( (pViewer->thingInfo.actorInfo.flags & SITH_AF_SEEINVISIBLE) == 0 )
    {
        if ( (pTarget->type == SITH_THING_ACTOR || pTarget->type == SITH_THING_PLAYER) && (pTarget->thingInfo.actorInfo.flags & SITH_AF_INVISIBLE) != 0 )
        {
            return 3;
        }

        if ( (pViewer->thingInfo.actorInfo.flags & SITH_AF_BLIND) != 0 )
        {
            return 3;
        }
    }

    if ( (pViewer->flags & SITH_TF_SUBMERGED) == 0 && (pTarget->flags & SITH_TF_SUBMERGED) != 0 && (pTarget->moveType != SITH_MT_PHYSICS || (pTarget->moveInfo.physics.flags & SITH_PF_ONWATERSURFACE) == 0) )
    {
        return 3;
    }

    if ( (pViewer->flags & SITH_TF_SUBMERGED) != 0 && (pTarget->flags & SITH_TF_SUBMERGED) == 0 )
    {
        return 3;
    }

    if ( *pDist - pTarget->collide.size > sightDistance )
    {
        return 1;
    }

    if ( fov > -1.0f )
    {
        float curdotL = rdVector_Dot3(toTarget, &pViewer->orient.lvec);
        float dotR = fabsf(rdVector_Dot3(toTarget, &pViewer->orient.rvec));

        if ( fov >= 0.0f && (curdotL < 0.0f || dotR > (1.0f - fov)) )
        {
            return 2;
        }

        if ( fov < 0.0f && curdotL < 0.0f && dotR < (fov + 1.0f) )
        {
            return 2;
        }
    }

    if ( (pViewer->thingInfo.actorInfo.flags & SITH_AF_SEEINVISIBLE) != 0 )
    {
        return 0;
    }

    SithSector* pSector = sithCollision_FindSectorInRadius(pViewer->pInSector, &pViewer->pos, startPos, 0.0f);
    if ( sithCollision_CheckLOS(pSector, startPos, &pTarget->pos, 0.0f) )
    {
        return 0;
    }

    return 3;
}

int J3DAPI sithAIUtil_GetDistanceToTargetPos(SithThing* pViewer, rdVector3* startPos, rdVector3* endPos, float fov, float sightDistance, float collisionSize, rdVector3* toTarget, float* pDist)
{
    J3D_UNUSED(collisionSize);

    SITH_ASSERTREL(toTarget && pDist);
    SITH_ASSERTREL(pViewer && startPos);

    rdVector_Sub3(toTarget, endPos, startPos);
    *pDist = rdVector_Normalize3Acc(toTarget);
    if ( *pDist > sightDistance )
    {
        return 1;
    }

    if ( fov > -1.0f )
    {
        float curdotL = rdVector_Dot3(toTarget, &pViewer->orient.lvec);
        float dotR = fabsf(rdVector_Dot3(toTarget, &pViewer->orient.rvec));

        if ( fov >= 0.0f && (curdotL < 0.0f || dotR > (1.0f - fov)) )
        {
            return 2;
        }

        if ( fov < 0.0f && curdotL < 0.0f && dotR < (fov + 1.0f) )
        {
            return 2;
        }
    }

    if ( (pViewer->thingInfo.actorInfo.flags & SITH_AF_SEEINVISIBLE) != 0 )
    {
        return 0;
    }

    SithSector* pSector = sithCollision_FindSectorInRadius(pViewer->pInSector, &pViewer->pos, startPos, 0.0f);
    if ( sithCollision_CheckLOS(pSector, startPos, endPos, 0.0f) )
    {
        return 0;
    }

    return 3;
}

int J3DAPI sithAIUtil_AIFire(SithAIControlBlock* pLocal, float minDist, float maxDist, float fireDot, float aimError, int weaponNum, SithAIUtilFireFlags flags, int burstCount)
{
    SithPuppetSubMode curSubMode = 0;
    SithFireProjectileFlags projFlags = 0;
    float lobFactor = 1.0f;
    int bHasFired = 0;
    rdVector3 targetDir;
    rdVector3 predictVec;
    float speed;

    SITH_ASSERTREL(pLocal->pOwner && pLocal->pClass);

    SithThing* pAttacker = pLocal->pOwner;
    if ( (sithMain_g_sith_mode.debugModeFlags & SITHDEBUG_AIDISABLED) != 0 )
    {
        return 0;
    }

    if ( (pAttacker->flags & (SITH_TF_DESTROYED | SITH_TF_DYING)) != 0 )
    {
        return 0;
    }

    if ( !pLocal->pTargetThing )
    {
        return 0;
    }

    if ( (pAttacker->pInSector->flags & SITH_SECTOR_UNDERWATER) != 0 && (pAttacker->thingInfo.actorInfo.flags & SITH_AF_NOUNDERWATERFIRE) != 0 )
    {
        return 0;
    }

    sithAI_EmitEvent(pLocal, SITHAI_EVENT_PREFIRE, (void*)flags);

    SithThing* pShotTemplate = pAttacker->thingInfo.actorInfo.pWeaponTemplate;
    SithWeaponId weaponIdx = pAttacker->thingInfo.actorInfo.weaponInfo.curWeaponID;
    if ( !pShotTemplate )
    {
        if ( !((unsigned int)weaponIdx >= SITHWEAPON_COMFISTS && (unsigned int)weaponIdx <= SITHWEAPON_COMSHOTGUN) )
        {
            goto postfire;
        }

        speed = 5.0f;
    }
    else
    {
        speed = pShotTemplate->moveInfo.physics.velocity.y;
    }

    sithAIUtil_UpdateTargetSight(pLocal, flags);
    rdVector_Copy3(&targetDir, &pLocal->toTarget);

    if ( (flags & SITHAIUTIL_FIRE_UNKNOWN_8) == 0 )
    {
        if ( pLocal->msecFireWaitTime > sithTime_g_msecGameTime )
        {
            goto postfire;
        }

        if ( pLocal->targetSightState || !(pLocal->distance >= minDist && pLocal->distance <= maxDist) )
        {
            goto postfire;
        }

        if ( (pLocal->submode & SITHAI_SUBMODE_SKIPCHECKFIREFOV) == 0 && rdVector_Dot3(&pAttacker->orient.lvec, &targetDir) < fireDot )
        {
            goto postfire;
        }

        if ( (flags & SITHAIUTIL_FIRE_UNKNOWN_40) != 0 )
        {
            curSubMode = SITHPUPPETSUBMODE_FIRE;
        }
        else if ( (flags & SITHAIUTIL_FIRE_UNKNOWN_80) != 0 )
        {
            curSubMode = SITHPUPPETSUBMODE_FIRE2;
        }
        else if ( (flags & SITHAIUTIL_FIRE_UNKNOWN_100) != 0 )
        {
            curSubMode = SITHPUPPETSUBMODE_FIRE3;
        }
        else if ( (flags & SITHAIUTIL_FIRE_UNKNOWN_200) != 0 )
        {
            curSubMode = SITHPUPPETSUBMODE_FIRE4;
        }

        if ( (flags & SITHAIUTIL_FIRE_STOP_ON_FIRE) != 0 )
        {
            sithAIMove_AIStop(pLocal);
        }
    }

    bool bDelayFire = false;
    if ( weaponNum >= 0 )
    {
        pLocal->fireDot = fireDot;
        pLocal->minFireDist = minDist;
        pLocal->maxFireDist = maxDist;
        bDelayFire = (flags & SITHAIUTIL_FIRE_UNKNOWN_8) == 0 && (pAttacker->thingInfo.actorInfo.flags & SITH_AF_DELAYFIRE) != 0;
    }

    if ( bDelayFire )
    {
        pLocal->fireFlags = flags | SITHAIUTIL_FIRE_UNKNOWN_8;
        pLocal->aimError = aimError;
        pLocal->fireWeaponNum = weaponNum;
        sithPuppet_PlayMode(pAttacker, curSubMode, NULL);
        sithAI_EmitEvent(pLocal, SITHAI_EVENT_FIRE, pLocal->pTargetThing);
        bHasFired = 1;
    }
    else
    {
        if ( (flags & SITHAIUTIL_FIRE_LEAD) != 0 && pLocal->pTargetThing->moveType == SITH_MT_PHYSICS
            && (pLocal->pTargetThing->moveInfo.physics.velocity.x != 0.0f || pLocal->pTargetThing->moveInfo.physics.velocity.y != 0.0f || pLocal->pTargetThing->moveInfo.physics.velocity.z != 0.0f) )
        {
            rdVector_Scale3(&predictVec, &targetDir, speed);
            rdVector_Add3Acc(&predictVec, &pLocal->pTargetThing->moveInfo.physics.velocity);
            rdVector_Normalize3Acc(&predictVec);
            if ( rdVector_Dot3(&predictVec, &targetDir) > 0.5f )
            {
                rdVector_Copy3(&targetDir, &predictVec);
            }
        }

        if ( (flags & SITHAIUTIL_FIRE_LOB) != 0 )
        {
            rdVector_Scale3(&predictVec, &pLocal->toTarget, speed);
            float leadFlightTime = pLocal->distance / speed;
            predictVec.z += leadFlightTime * 0.5f * sithWorld_g_pCurrentWorld->gravity;
            lobFactor = rdVector_Normalize3(&targetDir, &predictVec) / speed;
            projFlags |= 1;
        }

        float accuracy = pLocal->pClass->accurancy * sithGetHitAccuarancyScalar();
        if ( aimError != 0.0f && pLocal->distance != 0.0f && SITH_RAND() > accuracy )
        {
            aimError = sithGetCombatDamageScalar() * aimError;
            sithAIUtil_RandomRotate(&targetDir, aimError);
        }

        if ( pShotTemplate )
        {
            if ( pAttacker->pAttachedThing && (pAttacker->pAttachedThing->attach.flags & SITH_ATTACH_TAIL) != 0 )
            {
                sithAIMove_StartQuetzStrike(pLocal);
            }

            if ( weaponNum < 0 )
            {
                weaponNum = 0;
            }

            sithSoundClass_PlayModeRandom(pAttacker, SITHSOUNDCLASS_FIRE1 + weaponNum);
            rdVector_Copy3(&pAttacker->thingInfo.actorInfo.weaponInfo.vecUnknown0, &targetDir);
            SithThing* pBullet = sithWeapon_WeaponFire(pAttacker, pShotTemplate, &targetDir, &pLocal->weaponFirePos, 0, curSubMode, lobFactor, projFlags, 0.0f);
            if ( pBullet )
            {
                sithCog_ThingSendMessageEx(pAttacker, pBullet, SITHCOG_MSG_FIRE, (int)pShotTemplate->thingInfo.weaponInfo.damage, pShotTemplate->thingInfo.weaponInfo.damageType, weaponNum, 0);
                if ( (flags & SITHAIUTIL_FIRE_UNKNOWN_8) == 0 )
                {
                    sithAI_EmitEvent(pLocal, SITHAI_EVENT_FIRE, pLocal->pTargetThing);
                }

                bHasFired = 1;
                goto postfire;
            }
        }
        else
        {
            SithInventoryType* pActiveWeaponType = sithInventory_GetType(pAttacker->thingInfo.actorInfo.weaponInfo.curWeaponID);
            if ( pActiveWeaponType && pActiveWeaponType->pCog )
            {
                rdVector_Sub3(&pAttacker->thingInfo.actorInfo.weaponInfo.vecUnknown0, &targetDir, &pLocal->toTarget);
                if ( sithWeapon_HasWeaponSelected(pAttacker) && !sithWeapon_IsAiming(pAttacker) )
                {
                    goto postfire;
                }

                if ( !sithWeapon_IsMountingWeapon(pAttacker) )
                {
                    sithCog_SendMessageEx(pActiveWeaponType->pCog, SITHCOG_MSG_ACTIVATE, SITHCOG_SYM_REF_NONE, 0, SITHCOG_SYM_REF_THING, pAttacker->idx, 0, burstCount, 0, 0, 0);
                    if ( (flags & SITHAIUTIL_FIRE_UNKNOWN_8) == 0 )
                    {
                        sithAI_EmitEvent(pLocal, SITHAI_EVENT_FIRE, pLocal->pTargetThing);
                    }

                    bHasFired = 1;
                }
            }

            goto postfire;
        }

        return 0;
    }

postfire:
    sithAI_EmitEvent(pLocal, SITHAI_EVENT_POSTFIRE, (void*)flags);
    return bHasFired;
}

void J3DAPI sithAIUtil_AIPauseMove(SithAIControlBlock* pLocal, int msecPause)
{
    pLocal->msecPauseMoveUntil = sithTime_g_msecGameTime + msecPause;
}

void sithAIUtil_AIResetWaypoints(void)
{
    for ( int i = 0; i < 60; ++i )
    {
        sithAIUtil_aAIWpnts[i].thingNum = -1;
        sithAIUtil_aAIWpnts[i].flags = SITH_AIWPNT_LAYER0;
        sithAIUtil_aAIWpnts[i].numUsedLinks = 0;
        sithAIUtil_aAIWpnts[i].rank = 0;
    }

    for ( int i = 0; i < 10; ++i )
    {
        sithAIUtil_aWpntOwners[i].thingNum = -1;
        sithAIUtil_aWpntOwners[i].waypointNum = 0;
        sithAIUtil_aWpntOwners[i].cosTurnAlignAngle = 0.0f;
    }

    sithAIUtil_activeWpntLayer = SITH_AIWPNT_LAYER0;
    sithAIUtil_g_bRenderAIWpnts = 0;
}

int J3DAPI sithAIUtil_SyncWpnts(DPID idTo, unsigned int outstream)
{
    SITHDSS_STARTOUT(SITHDSS_SYNCWPNT);

    for ( int wpntNum = 0; wpntNum < 60; ++wpntNum )
    {
        SITHDSS_PUSHINT32(sithAIUtil_aAIWpnts[wpntNum].thingNum);
        SITHDSS_PUSHINT32(sithAIUtil_aAIWpnts[wpntNum].flags);
        SITHDSS_PUSHINT32(sithAIUtil_aAIWpnts[wpntNum].numUsedLinks);
        for ( int j = 0; j < 8; ++j )
        {
            SITHDSS_PUSHINT32(sithAIUtil_aAIWpnts[wpntNum].aWpntLinks[j]);
        }

        SITHDSS_PUSHINT32(sithAIUtil_aAIWpnts[wpntNum].rank);
    }

    for ( int wpntNum = 0; wpntNum < 10; ++wpntNum )
    {
        SITHDSS_PUSHINT32(sithAIUtil_aWpntOwners[wpntNum].thingNum);
        SITHDSS_PUSHINT32(sithAIUtil_aWpntOwners[wpntNum].waypointNum);
        SITHDSS_PUSHFLOAT(sithAIUtil_aWpntOwners[wpntNum].cosTurnAlignAngle);
    }

    SITHDSS_PUSHINT32(sithAIUtil_activeWpntLayer);

    SITHDSS_ENDOUT;
    return sithMessage_SendMessage(&sithMulti_g_message, idTo, outstream, 1u);
}

int J3DAPI sithAIUtil_ProcessSyncWpnts(const SithMessage* pMsg)
{
    SITH_ASSERTREL(pMsg);

    SITHDSS_STARTIN(pMsg);

    for ( int idx = 0; idx < 60; ++idx )
    {
        sithAIUtil_aAIWpnts[idx].thingNum = SITHDSS_POPINT32();
        sithAIUtil_aAIWpnts[idx].flags = SITHDSS_POPINT32();
        sithAIUtil_aAIWpnts[idx].numUsedLinks = SITHDSS_POPINT32();
        for ( int j = 0; j < 8; ++j )
        {
            sithAIUtil_aAIWpnts[idx].aWpntLinks[j] = SITHDSS_POPINT32();
        }

        sithAIUtil_aAIWpnts[idx].rank = SITHDSS_POPINT32();
    }

    for ( int idx = 0; idx < 10; ++idx )
    {
        sithAIUtil_aWpntOwners[idx].thingNum = SITHDSS_POPINT32();
        sithAIUtil_aWpntOwners[idx].waypointNum = SITHDSS_POPINT32();
        sithAIUtil_aWpntOwners[idx].cosTurnAlignAngle = SITHDSS_POPFLOAT();
    }

    sithAIUtil_activeWpntLayer = SITHDSS_POPINT32();
    SITHDSS_ENDIN;
    return 1;
}

void J3DAPI sithAIUtil_AISetWpnt(SithThing* pGhost, unsigned int wpntIdx)
{
    if ( wpntIdx >= STD_ARRAYLEN(sithAIUtil_aAIWpnts) )
    {
        SITHLOG_ERROR("AISetWpnt: Index (%d) out of allowed range (0 - %d).\n", wpntIdx, STD_ARRAYLEN(sithAIUtil_aAIWpnts));
        return;
    }

    if ( !pGhost || pGhost->type != SITH_THING_GHOST )
    {
        SITHLOG_ERROR("AISetWpnt: Non-GHOST type THING passed as waypoint.\n");
        return;
    }

    sithAIUtil_aAIWpnts[wpntIdx].thingNum = pGhost->idx;
    sithAIUtil_aAIWpnts[wpntIdx].flags = SITH_AIWPNT_LAYER0;
    sithAIUtil_aAIWpnts[wpntIdx].numUsedLinks = 0;
}

SithThing* J3DAPI sithAIUtil_AIGetWpntThing(int wpntIdx)
{
    return &sithWorld_g_pCurrentWorld->aThings[sithAIUtil_aAIWpnts[wpntIdx].thingNum];
}

void J3DAPI sithAIUtil_AISetWpntRank(unsigned int wpntIdx, unsigned int rank)
{
    if ( wpntIdx >= STD_ARRAYLEN(sithAIUtil_aAIWpnts) )
    {
        SITHLOG_ERROR("AISetWpnt: Index (%d) out of allowed range (0 - %d).\n", wpntIdx, STD_ARRAYLEN(sithAIUtil_aAIWpnts));
        return;
    }

    rank = STDMATH_CLAMP(rank, 0, 0xFF);

    sithAIUtil_aAIWpnts[wpntIdx].flags &= ~0xFF;
    sithAIUtil_aAIWpnts[wpntIdx].flags |= rank;
}

void J3DAPI sithAIUtil_AISetWpntFlags(unsigned int wpntIdx, unsigned int flags)
{
    if ( wpntIdx >= STD_ARRAYLEN(sithAIUtil_aAIWpnts) )
    {
        SITHLOG_ERROR("AISetWpnt: Index (%d) out of allowed range (0 - %d).\n", wpntIdx, STD_ARRAYLEN(sithAIUtil_aAIWpnts));
        return;
    }

    flags = STDMATH_CLAMP(flags, 0x100, 0xFFFFFF00);

    sithAIUtil_aAIWpnts[wpntIdx].flags |= flags;
}

void J3DAPI sithAIUtil_AIClearWpntFlags(unsigned int wpntIdx, unsigned int flags)
{
    if ( wpntIdx >= STD_ARRAYLEN(sithAIUtil_aAIWpnts) )
    {
        SITHLOG_ERROR("AISetWpnt: Index (%d) out of allowed range (0 - %d).\n", wpntIdx, STD_ARRAYLEN(sithAIUtil_aAIWpnts));
        return;
    }

    flags = STDMATH_CLAMP(flags, 0x100, 0xFFFFFF00);

    sithAIUtil_aAIWpnts[wpntIdx].flags &= ~flags;
}

void J3DAPI sithAIUtil_AISetActiveWpntLayer(SithAIWaypointLayerFlag layer)
{
    if ( (layer & 0x3F000) == 0 )
    {
        SITHLOG_ERROR("AISetWpntLayer: Invalid layer flag (%x).\n", layer);
        return;
    }

    sithAIUtil_activeWpntLayer = layer;
}

void J3DAPI sithAIUtil_AIConnectWpnts(unsigned int wpntIdx1, unsigned int wpntIdx2, int bConnectWpnt1To2)
{
    if ( wpntIdx1 >= STD_ARRAYLEN(sithAIUtil_aAIWpnts) || wpntIdx2 >= STD_ARRAYLEN(sithAIUtil_aAIWpnts) )
    {
        SITHLOG_ERROR("AISetWpnt: Index (%d or %d) out of allowed range (0 - %d).\n", wpntIdx1, wpntIdx2, STD_ARRAYLEN(sithAIUtil_aAIWpnts));
        return;
    }

    if ( sithAIUtil_aAIWpnts[wpntIdx1].thingNum == -1 )
    {
        SITHLOG_ERROR("AIConnectWpnts: Invalid waypoint (#%d).\n", wpntIdx1);
        return;
    }

    if ( sithAIUtil_aAIWpnts[wpntIdx2].thingNum == -1 )
    {
        SITHLOG_ERROR("AIConnectWpnts: Invalid waypoint (#%d).\n", wpntIdx2);
        return;
    }

    if ( (unsigned int)sithAIUtil_aAIWpnts[wpntIdx1].numUsedLinks < STD_ARRAYLEN(sithAIUtil_aAIWpnts[wpntIdx1].aWpntLinks) )
    {
        sithAIUtil_aAIWpnts[wpntIdx1].aWpntLinks[sithAIUtil_aAIWpnts[wpntIdx1].numUsedLinks] = wpntIdx2;
        sithAIUtil_aAIWpnts[wpntIdx1].numUsedLinks++;
    }
    else
    {
        SITHLOG_ERROR("AIConnectWpnts: Cant connect (#%d) to (#%d). Too many links\n", wpntIdx2, wpntIdx1);
    }

    if ( bConnectWpnt1To2 == 1 )
    {
        if ( (unsigned int)sithAIUtil_aAIWpnts[wpntIdx2].numUsedLinks < STD_ARRAYLEN(sithAIUtil_aAIWpnts[wpntIdx2].aWpntLinks) )
        {
            sithAIUtil_aAIWpnts[wpntIdx2].aWpntLinks[sithAIUtil_aAIWpnts[wpntIdx2].numUsedLinks] = wpntIdx1;
            sithAIUtil_aAIWpnts[wpntIdx2].numUsedLinks++;
        }
        else
        {
            SITHLOG_ERROR("AIConnectWpnts: Cannot connect (#%d) to (#%d). Too many links.\n", wpntIdx1, wpntIdx2);
        }
    }
}

int J3DAPI sithAIUtil_AIMoveToNextWpnt(SithAIControlBlock* pLocal, float moveSpeed, float degTurn, int mode)
{
    int wpntIdx = sithAIUtil_AIFindNearestWpnt(pLocal->pOwner, 1);
    if ( wpntIdx > -1 )
    {
        if ( (pLocal->submode & SITHAI_SUBMODE_UNKNOWN_100) != 0 )
        {
            degTurn = 180.0f;
        }

        sithAIUtil_AITraverseWpnts(pLocal, wpntIdx, moveSpeed, degTurn, mode);
        return 1;
    }

    return 0;
}

void J3DAPI sithAIUtil_AITraverseWpnts(SithAIControlBlock* pLocal, unsigned int wpntIdx, float moveSpeed, float degTurn, unsigned int mode)
{
    SithWorld* pCurWorld = sithWorld_g_pCurrentWorld;

    SITH_ASSERTREL(pLocal);

    if ( !pCurWorld || wpntIdx >= STD_ARRAYLEN(sithAIUtil_aAIWpnts) )
    {
        SITHLOG_ERROR("AITraverseWpnts: Index out of allowed range.\n");
        return;
    }

    int thingNum = sithAIUtil_aAIWpnts[wpntIdx].thingNum;
    if ( thingNum > pCurWorld->lastThingIdx || pCurWorld->aThings[thingNum].type != SITH_THING_GHOST )
    {
        SITHLOG_ERROR("AITraverseWpnts: Invalid waypoint (#%d).\n", wpntIdx);
        return;
    }

    if ( degTurn == 0.0f )
    {
        degTurn = pLocal->pClass->degTurnAlign;
        if ( degTurn == 0.0f )
        {
            degTurn = 45.0f;
        }
    }

    int ownerNum = sithAIUtil_CreateAIWaypoint(pLocal, degTurn);
    if ( ownerNum == -1 )
    {
        SITHLOG_ERROR("AITraverseWpnts: Too many AI's using waypoints (max = %d).\n", STD_ARRAYLEN(sithAIUtil_aWpntOwners));
        sithAIMove_AIStop(pLocal);
        return;
    }

    pLocal->mode |= SITHAI_MODE_TRAVERSEWPNTS;
    if ( mode == 1 )
    {
        pLocal->submode |= SITHAI_SUBMODE_SEMICONTINUOUSWPNTMOTION;
    }
    else if ( mode > 1 )
    {
        pLocal->submode |= SITHAI_SUBMODE_CONTINUOUSWPNTMOTION;
    }

    sithAIUtil_aWpntOwners[ownerNum].waypointNum = (sithAIUtil_aWpntOwners[ownerNum].waypointNum & ~0xFF00) | (wpntIdx << 8);

    if ( sithAIMove_AISetMovePos(pLocal, &pCurWorld->aThings[thingNum].pos, moveSpeed) )
    {
        sithAIMove_AISetLookPosEyeLevel(pLocal, &pCurWorld->aThings[thingNum].pos);
        sithAIUtil_CheckWpntMoveAlign(pLocal);
        return;
    }

    pLocal->moveSpeed = moveSpeed;
    sithAIUtil_AIAdvanceToNextWpnt(pLocal, 0);
}

int J3DAPI sithAIUtil_AIAdvanceToNextWpnt(SithAIControlBlock* pLocal, int bNotify)
{
    SithWorld* pCurLevel = sithWorld_g_pCurrentWorld;

    SITH_ASSERTREL(pLocal && pLocal->pOwner);

    int ownerIdx = sithAIUtil_GetAIWaypointNum(pLocal);
    if ( ownerIdx == -1 )
    {
        SITHLOG_ERROR("AIAdvanceToNextWpnt: AI '%s' not in waypoint tracking list.\n", pLocal->pOwner->aName);
    }
    else
    {
        int wpntIdx = (sithAIUtil_aWpntOwners[ownerIdx].waypointNum & 0xFF00) >> 8;
        sithCog_ThingSendMessageEx(pLocal->pOwner, NULL, SITHCOG_MSG_UPDATEWPNTS, 0, 0, 0, 0);

        if ( wpntIdx < 60 )
        {
            if ( sithAIUtil_aAIWpnts[wpntIdx].numUsedLinks > 0 )
            {
                int* aLinks = sithAIUtil_aAIWpnts[wpntIdx].aWpntLinks;
                int bLinkUsable = 0;
                int nextWpntNum;
                int i;
                for ( i = 0; i < (int)sithAIUtil_aAIWpnts[wpntIdx].numUsedLinks; ++i )
                {
                    nextWpntNum = aLinks[i];
                    if ( (sithAIUtil_aAIWpnts[nextWpntNum].flags & sithAIUtil_activeWpntLayer) != 0 && (sithAIUtil_aAIWpnts[nextWpntNum].flags & SITH_AIWPNT_DISABLED) == 0 )
                    {
                        bLinkUsable = 1;
                        break;
                    }
                }

                bool bSkipSingleLink = sithAIUtil_aAIWpnts[wpntIdx].numUsedLinks == 1 && aLinks[0] == (sithAIUtil_aWpntOwners[ownerIdx].waypointNum & 0xFF) && (pLocal->submode & (SITHAI_SUBMODE_CONTINUOUSWPNTMOTION | SITHAI_SUBMODE_SEMICONTINUOUSWPNTMOTION)) == 0;
                if ( bLinkUsable && !bSkipSingleLink )
                {
                    nextWpntNum = sithAIUtil_SelectWpntLink(pLocal, sithAIUtil_aAIWpnts[wpntIdx].numUsedLinks, aLinks, i, ownerIdx);
                    int thingNum = sithAIUtil_aAIWpnts[nextWpntNum].thingNum;
                    if ( sithAIMove_AISetMovePos(pLocal, &pCurLevel->aThings[thingNum].pos, pLocal->moveSpeed) )
                    {
                        sithAIMove_AISetLookPosEyeLevel(pLocal, &pCurLevel->aThings[thingNum].pos);
                        sithAIUtil_CheckWpntMoveAlign(pLocal);

                        sithAIUtil_aWpntOwners[ownerIdx].waypointNum |= 0xFF;
                        if ( wpntIdx != -1 )
                        {
                            sithAIUtil_aWpntOwners[ownerIdx].waypointNum = (sithAIUtil_aWpntOwners[ownerIdx].waypointNum & ~0xFF) | wpntIdx;
                        }

                        sithAIUtil_aWpntOwners[ownerIdx].waypointNum |= 0xFF00;
                        sithAIUtil_aWpntOwners[ownerIdx].waypointNum = (sithAIUtil_aWpntOwners[ownerIdx].waypointNum & ~0xFF00) | (nextWpntNum << 8);
                        if ( bNotify )
                        {
                            sithCog_ThingSendMessageEx(pLocal->pOwner, NULL, SITHCOG_MSG_ARRIVEDWPNT, wpntIdx, nextWpntNum, 0, 0);
                        }

                        return 1;
                    }
                }
            }
        }

        sithCog_ThingSendMessageEx(pLocal->pOwner, NULL, SITHCOG_MSG_ARRIVEDWPNT, wpntIdx, -1, 0, 0);
        sithCog_ThingSendMessage(pLocal->pOwner, NULL, SITHCOG_MSG_ARRIVED);
    }

    sithAIUtil_AIClearMode(pLocal, SITHAI_MODE_TRAVERSEWPNTS);
    return 0;
}

int J3DAPI sithAIUtil_SelectWpntLink(SithAIControlBlock* pLocal, unsigned int numLinks, int* aWypointLinks, int linkNum, int wpntOwnerNum)
{
    SithWorld* pWorld = sithWorld_g_pCurrentWorld;
    if ( (pLocal->mode & SITHAI_MODE_HUNTING) != 0 )
    {
        sithAIUtil_RankWpntsFromPlayer();
    }
    else if ( (pLocal->mode & SITHAI_MODE_FLEEING) != 0 )
    {
        if ( (pLocal->mode & SITHAI_MODE_FLEEINGTOWAYPOINT) != 0 )
        {
            sithAIUtil_RankWpntsFromThing(pLocal->pFleeFromThing);
        }
        else
        {
            sithAIUtil_RankWpntsFromPlayer();
        }
    }

    int leadWpnt = aWypointLinks[linkNum];
    for ( unsigned int idx = linkNum + 1; idx < numLinks; ++idx )
    {
        int nextWpnt = aWypointLinks[idx];
        if ( (sithAIUtil_aAIWpnts[nextWpnt].flags & SITH_AIWPNT_DISABLED) != 0 || (sithAIUtil_aAIWpnts[nextWpnt].flags & sithAIUtil_activeWpntLayer) == 0 )
        {
            continue;
        }

        int bestPriority = sithAIUtil_aAIWpnts[leadWpnt].flags & 0xFF;
        int curRank = sithAIUtil_aAIWpnts[nextWpnt].flags & 0xFF;

        if ( (pLocal->mode & SITHAI_MODE_FLEEING) != 0 && pLocal->pFleeFromThing && (pLocal->mode & SITHAI_MODE_FLEEINGTOWAYPOINT) == 0 )
        {
            if ( sithAIUtil_aAIWpnts[leadWpnt].rank == 0xFF )
            {
                bestPriority -= 14;
            }
            else if ( sithAIUtil_aAIWpnts[nextWpnt].rank == 0xFF )
            {
                curRank -= 14;
            }

            rdVector3 targetDir;
            rdVector3 wpntDir;
            rdVector_Sub3(&targetDir, &pLocal->pFleeFromThing->pos, &pLocal->pOwner->pos);
            float playerDistance = rdVector_Normalize3Acc(&targetDir);
            float slopeStep = (0.984f - 0.923f) / (1.5f - 0.5f);
            float threshold = (playerDistance - 0.5f) * slopeStep + 0.923f;
            threshold = STDMATH_CLAMP(threshold, 0.923f, 0.984f);

            int idxThing = sithAIUtil_aAIWpnts[leadWpnt].thingNum;
            rdVector_Sub3(&wpntDir, &pWorld->aThings[idxThing].pos, &pLocal->pOwner->pos);
            rdVector_Normalize3Acc(&wpntDir);
            float fleeFacing = rdVector_Dot3(&targetDir, &wpntDir);
            if ( fleeFacing > threshold )
            {
                bestPriority -= 10;
            }

            idxThing = sithAIUtil_aAIWpnts[nextWpnt].thingNum;
            rdVector_Sub3(&wpntDir, &pWorld->aThings[idxThing].pos, &pLocal->pOwner->pos);
            rdVector_Normalize3Acc(&wpntDir);
            fleeFacing = rdVector_Dot3(&targetDir, &wpntDir);
            if ( fleeFacing > threshold )
            {
                curRank -= 10;
            }

            int rankLead = sithAIUtil_aAIWpnts[nextWpnt].rank - sithAIUtil_aAIWpnts[leadWpnt].rank;
            if ( rankLead < 0 )
            {
                curRank += -rankLead * 3;
            }
            else if ( rankLead > 0 )
            {
                bestPriority += rankLead * 3;
            }

            if ( wpntOwnerNum != -1 )
            {
                if ( (sithAIUtil_aWpntOwners[wpntOwnerNum].waypointNum & 0xFF) < 60 )
                {
                    if ( nextWpnt == (sithAIUtil_aWpntOwners[wpntOwnerNum].waypointNum & 0xFF) )
                    {
                        --curRank;
                    }

                    if ( leadWpnt == (sithAIUtil_aWpntOwners[wpntOwnerNum].waypointNum & 0xFF) )
                    {
                        --bestPriority;
                    }
                }
            }
        }
        else if ( (pLocal->mode & SITHAI_MODE_FLEEING) != 0 && pLocal->pFleeFromThing && (pLocal->mode & SITHAI_MODE_FLEEINGTOWAYPOINT) != 0 )
        {
            bestPriority = 0;
            curRank = 0;
            if ( wpntOwnerNum != -1 )
            {
                if ( (sithAIUtil_aWpntOwners[wpntOwnerNum].waypointNum & 0xFF) < 60 )
                {
                    if ( nextWpnt == (sithAIUtil_aWpntOwners[wpntOwnerNum].waypointNum & 0xFF) )
                    {
                        curRank -= 6;
                    }

                    if ( leadWpnt == (sithAIUtil_aWpntOwners[wpntOwnerNum].waypointNum & 0xFF) )
                    {
                        bestPriority -= 6;
                    }
                }
            }

            if ( sithAIUtil_aAIWpnts[nextWpnt].rank > sithAIUtil_aAIWpnts[leadWpnt].rank )
            {
                curRank += 7;
            }
            else if ( sithAIUtil_aAIWpnts[nextWpnt].rank < sithAIUtil_aAIWpnts[leadWpnt].rank )
            {
                bestPriority += 7;
            }
        }
        else if ( (pLocal->mode & SITHAI_MODE_HUNTING) != 0 && pLocal->pTargetThing )
        {
            bestPriority = 0;
            curRank = 0;
            if ( wpntOwnerNum != -1 )
            {
                if ( (sithAIUtil_aWpntOwners[wpntOwnerNum].waypointNum & 0xFF) < 60 )
                {
                    if ( nextWpnt == (sithAIUtil_aWpntOwners[wpntOwnerNum].waypointNum & 0xFF) )
                    {
                        curRank -= 6;
                    }

                    if ( leadWpnt == (sithAIUtil_aWpntOwners[wpntOwnerNum].waypointNum & 0xFF) )
                    {
                        bestPriority -= 6;
                    }
                }
            }

            if ( sithAIUtil_aAIWpnts[nextWpnt].rank > sithAIUtil_aAIWpnts[leadWpnt].rank )
            {
                curRank += 7;
            }
            else if ( sithAIUtil_aAIWpnts[nextWpnt].rank < sithAIUtil_aAIWpnts[leadWpnt].rank )
            {
                bestPriority += 7;
            }
        }
        else if ( (pLocal->mode & (SITHAI_MODE_ATTACKING | SITHAI_MODE_SEARCHING)) != 0 )
        {
            if ( wpntOwnerNum != -1 )
            {
                if ( (sithAIUtil_aWpntOwners[wpntOwnerNum].waypointNum & 0xFF) < 60 )
                {
                    if ( nextWpnt == (sithAIUtil_aWpntOwners[wpntOwnerNum].waypointNum & 0xFF) )
                    {
                        curRank -= 6;
                    }

                    if ( leadWpnt == (sithAIUtil_aWpntOwners[wpntOwnerNum].waypointNum & 0xFF) )
                    {
                        bestPriority -= 6;
                    }
                }
            }

            if ( curRank < bestPriority && SITH_RAND() > (bestPriority - curRank) * 0.1f + 0.5f )
            {
                bestPriority = -bestPriority;
            }
        }

        if ( curRank > bestPriority || (curRank == bestPriority && SITH_RAND() > 0.5f) )
        {
            leadWpnt = nextWpnt;
        }
    }

    return leadWpnt;
}

void J3DAPI sithAIUtil_CheckWpntMoveAlign(SithAIControlBlock* pLocal)
{
    int wpntNumber = sithAIUtil_GetAIWaypointNum(pLocal);
    if ( wpntNumber > -1 )
    {
        float dist = rdMath_DistancePointToPlane(&pLocal->goalLVec, &pLocal->pOwner->orient.uvec, &rdroid_g_zeroVector3);
        rdVector3 dir;
        rdVector_ScaleAdd3(&dir, &pLocal->pOwner->orient.uvec, -dist, &pLocal->goalLVec);
        rdVector_Normalize3Acc(&dir);

        float dot;
        if ( (pLocal->mode & SITHAI_MODE_WALLCRAWLING) != 0 )
        {
            dot = pLocal->pOwner->orient.lvec.x * dir.x + pLocal->pOwner->orient.lvec.y * dir.y + pLocal->pOwner->orient.lvec.z * dir.z;
        }
        else
        {
            dot = pLocal->pOwner->orient.lvec.x * dir.x + pLocal->pOwner->orient.lvec.y * dir.y;
        }

        if ( dot >= sithAIUtil_aWpntOwners[wpntNumber].cosTurnAlignAngle || (pLocal->mode & SITHAI_MODE_TURNING) == 0 )
        {
            pLocal->submode &= ~SITHAI_SUBMODE_UNKNOWN_10;
        }
        else
        {
            memset(&pLocal->pOwner->moveInfo.physics.velocity, 0, sizeof(rdVector3));
        }
    }
    else
    {
        sithAIUtil_AIClearMode(pLocal, SITHAI_MODE_TRAVERSEWPNTS);
    }
}

int J3DAPI sithAIUtil_AIFindNearestWpnt(SithThing* pThing, int bCheckPath)
{
    SithWorld* pWorld = sithWorld_g_pCurrentWorld;
    int idWpnt = 0;
    int found = 0;
    if ( pThing != sithPlayer_g_pLocalPlayerThing )
    {
        sithCog_ThingSendMessageEx(pThing, NULL, SITHCOG_MSG_UPDATEWPNTS, 0, 0, 0, 0);
    }

    int numDist = 0;
    int i;
    for ( i = 0; i < 60; ++i )
    {
        if ( (sithAIUtil_aAIWpnts[i].flags & SITH_AIWPNT_DISABLED) != 0 || (sithAIUtil_aAIWpnts[i].flags & sithAIUtil_activeWpntLayer) == 0 )
        {
            continue;
        }

        int idxThing = sithAIUtil_aAIWpnts[i].thingNum;
        if ( idxThing == -1 )
        {
            continue;
        }

        if ( idxThing > pWorld->lastThingIdx || pWorld->aThings[idxThing].type != SITH_THING_GHOST )
        {
            SITHLOG_ERROR("AIFindNearestWpnt: Invalid waypoint (#%d).\n", i);
            continue;
        }

        found = 1;
        sithAIUtil_aWpntDistances[numDist].num = i;
        sithAIUtil_aWpntDistances[numDist].distance = (pThing->pos.x - pWorld->aThings[idxThing].pos.x) * (pThing->pos.x - pWorld->aThings[idxThing].pos.x) + (pThing->pos.y - pWorld->aThings[idxThing].pos.y) * (pThing->pos.y - pWorld->aThings[idxThing].pos.y) + (pThing->pos.z - pWorld->aThings[idxThing].pos.z) * (pThing->pos.z - pWorld->aThings[idxThing].pos.z);
        if ( sithAIUtil_aWpntDistances[numDist].distance <= (pThing->collide.movesize + 0.03f) * (pThing->collide.movesize + 0.03f) )
        {
            return i;
        }

        ++numDist;
    }

    if ( !found )
    {
        return -1;
    }

    qsort(sithAIUtil_aWpntDistances, numDist, sizeof(SithAIWaypointDistance), sithAIUtil_CompareAIWpntDistances);
    idWpnt = sithAIUtil_aWpntDistances[0].num;
    float pathDist;
    rdVector3 hitNorm;
    if ( bCheckPath )
    {
        found = 0;
        for ( i = 0; i < numDist; ++i )
        {
            idWpnt = sithAIUtil_aWpntDistances[i].num;
            int idxThing = sithAIUtil_aAIWpnts[idWpnt].thingNum;
            if ( !sithAIUtil_CheckPathToPoint(pThing, &pWorld->aThings[idxThing].pos, pThing->collide.movesize, &pathDist, &hitNorm, 1, 0) )
            {
                found = 1;
                break;
            }
        }

        if ( !found )
        {
            return -1;
        }
    }

    if ( pThing == sithPlayer_g_pLocalPlayerThing )
    {
        return idWpnt;
    }

    int aLinks[9];
    aLinks[0] = idWpnt;
    unsigned int nLinks = 1;
    for ( ++i; i < numDist; ++i )
    {
        for ( unsigned int linkId = 0; linkId < sithAIUtil_aAIWpnts[idWpnt].numUsedLinks; ++linkId )
        {
            if ( sithAIUtil_aAIWpnts[idWpnt].aWpntLinks[linkId] == sithAIUtil_aWpntDistances[i].num )
            {
                int idxThing = sithAIUtil_aAIWpnts[sithAIUtil_aWpntDistances[i].num].thingNum;
                if ( !sithAIUtil_CheckPathToPoint(pThing, &pWorld->aThings[idxThing].pos, pThing->collide.movesize, &pathDist, &hitNorm, 1, 0) )
                {
                    aLinks[nLinks++] = sithAIUtil_aWpntDistances[i].num;
                    break;
                }
            }
        }

        if ( nLinks >= sithAIUtil_aAIWpnts[idWpnt].numUsedLinks + 1 )
        {
            break;
        }
    }

    idWpnt = sithAIUtil_SelectWpntLink(pThing->controlInfo.aiControl.pLocal, nLinks, aLinks, 0, -1);
    return idWpnt;
}

void J3DAPI sithAIUtil_RankWpntsFromPlayer()
{
    if ( !sithPlayer_g_pLocalPlayerThing )
    {
        return;
    }

    if ( (sithPlayer_g_pLocalPlayerThing->thingInfo.actorInfo.flags & SITH_AF_INVISIBLE) != 0 )
    {
        return;
    }

    for ( unsigned int idx = 0; idx < 60; ++idx )
    {
        sithAIUtil_aAIWpnts[idx].rank = 0;
    }

    int wpntNum = sithAIUtil_AIFindNearestWpnt(sithPlayer_g_pLocalPlayerThing, 0);
    if ( wpntNum == -1 )
    {
        return;
    }

    unsigned int rankNum = 255;
    int aStack[60];
    int top = 0;
    aStack[0] = wpntNum;
    sithAIUtil_aAIWpnts[wpntNum].rank = rankNum;
    while ( top >= 0 )
    {
        int curWpnt = aStack[top--];
        rankNum = sithAIUtil_aAIWpnts[curWpnt].rank - 1;
        for ( unsigned int idx = 0; idx < sithAIUtil_aAIWpnts[curWpnt].numUsedLinks; ++idx )
        {
            int linkCount = sithAIUtil_aAIWpnts[curWpnt].aWpntLinks[idx];
            if ( (sithAIUtil_aAIWpnts[linkCount].flags & SITH_AIWPNT_DISABLED) != 0 || (sithAIUtil_aAIWpnts[linkCount].flags & sithAIUtil_activeWpntLayer) == 0 )
            {
                continue;
            }

            if ( sithAIUtil_aAIWpnts[linkCount].rank < rankNum )
            {
                sithAIUtil_aAIWpnts[linkCount].rank = rankNum;
                if ( sithAIUtil_aAIWpnts[linkCount].numUsedLinks > 1 )
                {
                    aStack[++top] = linkCount;
                }
            }
        }
    }
}

void J3DAPI sithAIUtil_RankWpntsFromThing(SithThing* pThing)
{
    for ( int idx = 0; idx < 60; ++idx )
    {
        sithAIUtil_aAIWpnts[idx].rank = 0;
    }

    int wpntNum = 0;
    int bFound = 0;
    for ( unsigned int idx = 0; idx < 60; ++idx )
    {
        if ( sithAIUtil_aAIWpnts[idx].thingNum == pThing->idx )
        {
            bFound = 1;
            wpntNum = idx;
            break;
        }
    }

    if ( !bFound )
    {
        return;
    }

    unsigned int rankNum = 255;
    int aStack[60];
    int top = 0;
    aStack[0] = wpntNum;
    sithAIUtil_aAIWpnts[wpntNum].rank = rankNum;
    while ( top >= 0 )
    {
        int curWpnt = aStack[top--];
        rankNum = sithAIUtil_aAIWpnts[curWpnt].rank - 1;
        for ( unsigned int idx = 0; idx < sithAIUtil_aAIWpnts[curWpnt].numUsedLinks; ++idx )
        {
            int linkCount = sithAIUtil_aAIWpnts[curWpnt].aWpntLinks[idx];
            if ( (sithAIUtil_aAIWpnts[linkCount].flags & SITH_AIWPNT_DISABLED) != 0 || (sithAIUtil_aAIWpnts[linkCount].flags & sithAIUtil_activeWpntLayer) == 0 )
            {
                continue;
            }

            if ( sithAIUtil_aAIWpnts[linkCount].rank < rankNum )
            {
                sithAIUtil_aAIWpnts[linkCount].rank = rankNum;
                if ( sithAIUtil_aAIWpnts[linkCount].numUsedLinks > 1 )
                {
                    aStack[++top] = linkCount;
                }
            }
        }
    }
}

int J3DAPI sithAIUtil_CreateAIWaypoint(SithAIControlBlock* pLocal, float degTurn)
{
    int wpntNum = -1;
    for ( int ownerNum = 0; ownerNum < 10; ++ownerNum )
    {
        if ( sithAIUtil_aWpntOwners[ownerNum].thingNum == -1 && wpntNum == -1 )
        {
            wpntNum = ownerNum;
        }

        if ( sithAIUtil_aWpntOwners[ownerNum].thingNum == pLocal->pOwner->idx )
        {
            wpntNum = ownerNum;
            break;
        }
    }

    if ( wpntNum > -1 )
    {
        degTurn = STDMATH_CLAMP(degTurn, 5.0f, 180.0f);
        float sinAngle;
        float cosAngle;
        stdMath_SinCos(degTurn, &sinAngle, &cosAngle);
        sithAIUtil_aWpntOwners[wpntNum].thingNum = pLocal->pOwner->idx;
        sithAIUtil_aWpntOwners[wpntNum].cosTurnAlignAngle = cosAngle;
    }

    return wpntNum;
}

void J3DAPI sithAIUtil_ClearAIWaypoint(SithAIControlBlock* pLocal)
{
    int wpntNum = sithAIUtil_GetAIWaypointNum(pLocal);
    if ( wpntNum > -1 )
    {
        sithAIUtil_aWpntOwners[wpntNum].thingNum = -1;
        sithAIUtil_aWpntOwners[wpntNum].waypointNum = 0;
        sithAIUtil_aWpntOwners[wpntNum].cosTurnAlignAngle = 0.0f;
    }
}

int J3DAPI sithAIUtil_GetAIWaypointNum(SithAIControlBlock* pLocal)
{
    int i;

    for ( i = 0; i < 10; ++i )
    {
        if ( sithAIUtil_aWpntOwners[i].thingNum == pLocal->pOwner->idx )
        {
            break;
        }
    }

    if ( i < 10 )
    {
        return i;
    }

    return -1;
}

int J3DAPI sithAIUtil_CompareAIWpntDistances(const SithAIWaypointDistance* pDist1, const SithAIWaypointDistance* pDist2)
{
    if ( pDist1 && pDist2 )
    {
        if ( pDist1->distance < pDist2->distance )
        {
            return -1;
        }

        if ( pDist1->distance > pDist2->distance )
        {
            return 1;
        }
    }

    return 0;
}

void sithAIUtil_RenderAIWaypoints(void)
{
    SithWorld* pWorld = sithWorld_g_pCurrentWorld;
    if ( !sithAIUtil_g_bRenderAIWpnts )
    {
        sithAIUtil_secLastUpdate = 0.0f;
        return;
    }

    if ( sithTime_g_secGameTime - sithAIUtil_secLastUpdate < 1.0f )
    {
        return;
    }

    for ( unsigned int wpntIdx = 0; wpntIdx < 60; ++wpntIdx )
    {
        int thingNum = sithAIUtil_aAIWpnts[wpntIdx].thingNum;
        if ( (sithAIUtil_aAIWpnts[wpntIdx].flags & SITH_AIWPNT_DISABLED) != 0 || (sithAIUtil_aAIWpnts[wpntIdx].flags & sithAIUtil_activeWpntLayer) == 0 )
        {
            continue;
        }

        for ( unsigned int j = 0; j < sithAIUtil_aAIWpnts[wpntIdx].numUsedLinks; ++j )
        {
            int toWpntIdx = sithAIUtil_aAIWpnts[wpntIdx].aWpntLinks[j];
            if ( (unsigned int)toWpntIdx < wpntIdx )
            {
                continue;
            }

            if ( (sithAIUtil_aAIWpnts[toWpntIdx].flags & SITH_AIWPNT_DISABLED) != 0 || (sithAIUtil_aAIWpnts[toWpntIdx].flags & sithAIUtil_activeWpntLayer) == 0 )
            {
                continue;
            }

            int linkGhostIdx = sithAIUtil_aAIWpnts[toWpntIdx].thingNum;
            rdVector3 offset = { 0.0f, 0.0f, 0.1f };
            rdVector3 laserEnd;
            rdVector_Copy3(&laserEnd, &pWorld->aThings[linkGhostIdx].pos);
            laserEnd.z += 0.1f;
            sithFX_CreateLaserThing(&pWorld->aThings[thingNum], &offset, &laserEnd, 0.05f, 0.05f, 1.1f);
            sithAIUtil_secLastUpdate = sithTime_g_secGameTime;
        }
    }
}

int J3DAPI sithAIUtil_CheckPathToPoint(SithThing* pViewer, const rdVector3* pTarget, float radius, float* pDistance, rdVector3* pHitNorm, int bDetectThings, int bSkipFloor)
{
    rdVector3 curdir;
    SithCollision* pCurCollision;

    int bBlocked = 0;
    SITH_ASSERTREL(pViewer && pTarget);

    rdVector_Sub3(&curdir, pTarget, &pViewer->pos);
    float distance = rdVector_Normalize3Acc(&curdir);
    *pDistance = distance;

    int flags = 0x102;
    if ( !bDetectThings )
    {
        flags |= 0x1;
    }

    sithCollision_SearchForCollisions(pViewer->pInSector, pViewer, &pViewer->pos, &curdir, distance, radius, flags);
    while ( (pCurCollision = sithCollision_PopStack()) != NULL )
    {
        if ( !bSkipFloor && (pCurCollision->type & SITHCOLLISION_WORLD) != 0 && (pCurCollision->pSurfaceCollided->flags & SITH_SURFACE_ISFLOOR) != 0 )
        {
            continue;
        }

        if ( (pCurCollision->type & (SITHCOLLISION_ADJOINTOUCH | SITHCOLLISION_ADJOINCROSS)) != 0 && (pCurCollision->pSurfaceCollided->pAdjoin->flags & SITH_ADJOIN_MOVE) != 0 )
        {
            continue;
        }

        bBlocked = 1;
        *pDistance = pCurCollision->distance;
        rdVector_Copy3(pHitNorm, &pCurCollision->hitNorm);
        break;
    }

    sithCollision_DecreaseStackLevel();
    return bBlocked;
}

int J3DAPI sithAIUtil_CheckPosition(const SithAIControlBlock* pLocal, rdVector3* endPos, int* pbNoCollision)
{
    SithThing* pThing = pLocal->pOwner;
    SITH_ASSERTREL(pThing);

    SithSector* pSector = sithCollision_FindSectorInRadius(pThing->pInSector, &pThing->pos, endPos, 0.0f);
    if ( !pSector )
    {
        return 0;
    }

    if ( (pSector->flags & SITH_SECTOR_NOACTORENTER) != 0 )
    {
        return 0;
    }

    if ( (pThing->pInSector->flags & SITH_SECTOR_UNDERWATER) == 0 && (pSector->flags & SITH_SECTOR_UNDERWATER) != 0 && (pThing->flags & SITH_TF_WATERDESTROYED) != 0 )
    {
        return 0;
    }

    return sithAIUtil_CheckFloorAtPos(pLocal, endPos, pSector, pbNoCollision);
}

int J3DAPI sithAIUtil_CheckFloorAtPos(const SithAIControlBlock* pLocal, const rdVector3* startPos, SithSector* pStartSector, int* pbNoCollision)
{
    int result = 8;
    SithThing* pThing = pLocal->pOwner;
    SITH_ASSERTREL(pThing);

    rdVector3 downDir;
    if ( (pLocal->mode & SITHAI_MODE_WALLCRAWLING) != 0 )
    {
        rdVector_Neg3(&downDir, &pThing->orient.uvec);
    }
    else if ( rdVector_Dot3(&pThing->orient.uvec, &rdroid_g_zVector3) > -0.707f )
    {
        rdVector_Neg3(&downDir, &rdroid_g_zVector3);
    }
    else
    {
        rdVector_Copy3(&downDir, &rdroid_g_zVector3);
    }

    sithCollision_SearchForCollisions(pStartSector, pThing, startPos, &downDir, sithPhysics_GetThingHeight(pThing) + pLocal->pClass->maxStep, 0.001f, 0);
    SithCollision* pCollision;
    while ( (pCollision = sithCollision_PopStack()) != NULL )
    {
        if ( (pCollision->type & (SITHCOLLISION_ADJOINTOUCH | SITHCOLLISION_ADJOINCROSS)) != 0 )
        {
            if ( (pCollision->pSurfaceCollided->pAdjoin->pAdjoinSector->flags & SITH_SECTOR_NOACTORENTER) != 0 )
            {
                result = 0;
                break;
            }

            if ( (pThing->pInSector->flags & SITH_SECTOR_UNDERWATER) == 0 && (pCollision->pSurfaceCollided->pAdjoin->pAdjoinSector->flags & SITH_SECTOR_UNDERWATER) != 0 && (pThing->flags & SITH_TF_WATERDESTROYED) != 0 )
            {
                result = 0;
                break;
            }
        }

        if ( (pCollision->type & SITHCOLLISION_WORLD) != 0 )
        {
            SITH_ASSERTREL(pCollision->pSurfaceCollided);
            if ( (pCollision->pSurfaceCollided->flags & SITH_SURFACE_NOAIMOVE) != 0 )
            {
                result = 0;
                break;
            }

            if ( pLocal->allowedSurfaceTypes )
            {
                if ( (pCollision->pSurfaceCollided->flags & pLocal->allowedSurfaceTypes) == 0 )
                {
                    result = 0;
                    break;
                }
            }
            else if ( (pCollision->pSurfaceCollided->flags & 0x200000) != 0 )
            {
                result = 0;
                break;
            }

            if ( (pCollision->pSurfaceCollided->flags & SITH_SURFACE_ISFLOOR) != 0 )
            {
                result = 1;
                if ( (pThing->moveInfo.physics.flags & SITH_PF_ALIGNUP) != 0 && (pThing->attach.flags & SITH_ATTACH_SURFACE) != 0 )
                {
                    SithSurface* pAttachedSurface = pThing->attach.attachedToStructure.pSurfaceAttached;
                    SithSurface* pSurface = pCollision->pSurfaceCollided;
                    if ( pAttachedSurface != pSurface )
                    {
                        float dot = rdVector_Dot3(&pSurface->face.normal, &rdroid_g_zVector3);
                        if ( dot < 0.788f )
                        {
                            result = 2;
                        }
                    }
                }
            }
            else
            {
                result = 2;
            }

            if ( pbNoCollision )
            {
                if ( (pThing->attach.flags & SITH_ATTACH_SURFACE) == 0 || pThing->attach.attachedToStructure.pSurfaceAttached != pCollision->pSurfaceCollided )
                {
                    *pbNoCollision = 1;
                }
                else
                {
                    *pbNoCollision = 0;
                }
            }

            break;
        }

        if ( (pCollision->type & SITHCOLLISION_THING) != 0 )
        {
            SITH_ASSERTREL(pCollision->pThingCollided);
            if ( (pCollision->pThingCollided->attach.flags & SITH_ATTACH_THING) != 0 && pCollision->pThingCollided->attach.attachedToStructure.pThingAttached == pThing )
            {
                result = 1;
            }
            else if ( pThing->pAttachedThing && (pThing->pAttachedThing->attach.flags & SITH_ATTACH_TAIL) != 0 )
            {
                result = 1;
            }
            else if ( (pCollision->pThingCollided->flags & SITH_TF_STANDON) == 0 && (pCollision->pThingCollided->type & 0xA) == 0 )
            {
                result = 4;
                break;
            }

            result = 1;
            if ( pbNoCollision )
            {
                if ( (pThing->attach.flags & SITH_ATTACH_THINGFACE) == 0 || pThing->attach.attachedToStructure.pThingAttached != pCollision->pThingCollided )
                {
                    *pbNoCollision = 1;
                }
                else
                {
                    *pbNoCollision = 0;
                }
            }

            break;
        }
    }

    sithCollision_DecreaseStackLevel();
    return result;
}

int J3DAPI sithAIUtil_MakeRandPoint(SithAIControlBlock* pLocal, rdVector3* pPos, int flags, float angle, float* pDownCos, rdVector3* pDir, rdVector3* pDestPoint)
{
    rdVector3 newDir;
    rdVector3 rotPyr;
    rdVector3 toMovePos;

    int bValid = 0;
    SITH_ASSERTREL(pLocal && pLocal->pClass && pLocal->pOwner);
    SithThing* pActor = pLocal->pOwner;

    if ( !pPos || !pDir || !pDestPoint )
    {
        SITHLOG_ERROR("sithAIUtil_MakeRandPoint: Bad parameters.\n");
        return 0;
    }

    sithAIUtil_mkPoinCurFlags = 0;
    sithAIUtil_pMkPointCurLocal = NULL;

    if ( (flags & 4) == 0 )
    {
        pDir->z = 0.0f;
        rdVector_Normalize3Acc(pDir);
    }

    if ( (flags & 6) != 0 )
    {
        float pitch = (flags & 4) != 0 ? angle : 0.0f;
        float yaw = (flags & 2) != 0 ? angle : 0.0f;

        if ( (flags & 0x4000) != 0 )
        {
            if ( pitch != 0.0f )
            {
                pitch = SITH_RANDF() * pitch;
            }

            if ( yaw != 0.0f )
            {
                yaw = SITH_RANDF() * yaw;
            }
        }

        rotPyr.x = pitch;
        rotPyr.y = yaw;
        rotPyr.z = 0.0f;
        if ( (flags & 0x8000) != 0 && SITH_RAND() > 0.5f )
        {
            rdVector_Neg3Acc(&rotPyr);
        }

        rdVector_Rotate3(&newDir, pDir, &rotPyr);
        bValid = 1;
    }
    else if ( (flags & 1) != 0 )
    {
        rdVector_Cross3(&newDir, &pActor->orient.uvec, pDir);

        if ( (flags & 0x800) != 0 )
        {
            if ( rdVector_Dot3(&pLocal->moveDirection, &newDir) < 0.0f )
            {
                rdVector_Neg3Acc(&newDir);
            }
        }
        else
        {
            if ( (flags & 0x8000) != 0 && SITH_RAND() > 0.5f )
            {
                rdVector_Neg3Acc(&newDir);
            }

            if ( (flags & 0x4000) != 0 )
            {
                rotPyr.x = 0.0f;
                rotPyr.y = SITH_RANDF() * angle;
                rotPyr.z = 0.0f;
                if ( SITH_RAND() > 0.5f )
                {
                    rdVector_Neg3Acc(&rotPyr);
                }

                rdVector_Rotate3Acc(&newDir, &rotPyr);
            }
        }

        bValid = 1;
    }

    if ( bValid )
    {
        if ( (flags & 0x101700) != 0 )
        {
            rdVector3 movePos;
            sithAIUtil_AIGetMovePos(pLocal, flags, &movePos);
            rdVector_Sub3(&toMovePos, &movePos, &pActor->pos);
            rdVector_Normalize3Acc(&toMovePos);

            int isInFront = rdVector_Dot3(&newDir, &toMovePos) > 0.0f;
            if ( ((flags & 0x100500) != 0 && !isInFront) || ((flags & 0x1200) != 0 && isInFront) )
            {
                if ( (flags & 2) != 0 )
                {
                    rdVector_Neg3Acc(&rotPyr);
                    rdVector_Rotate3(&newDir, pDir, &rotPyr);
                }
                else
                {
                    rdVector_Neg3Acc(&newDir);
                }
            }

            if ( (flags & 0x10000) != 0 )
            {
                *pDownCos = -rdMath_DistancePointToPlane(pPos, &newDir, &movePos);
            }
        }

        if ( stdMath_ClipNearZero(*pDownCos) != 0.0f )
        {
            sithAIUtil_mkPoinCurFlags = flags;
            sithAIUtil_pMkPointCurLocal = pLocal;
            rdVector_Copy3(&sithAIUtil_mkPointCurDir, pDir);
            if ( (flags & 2) != 0 )
            {
                rdVector_Copy3(&sithAIUtil_mkPointCurPYR, &rotPyr);
            }
            else
            {
                rdVector_Copy3(&sithAIUtil_mkPointCurRVec, &newDir);
            }

            rdVector_ScaleAdd3(pDestPoint, &newDir, *pDownCos, pPos);
            rdVector_Copy3(pDir, &newDir);
            *pDownCos = fabsf(*pDownCos);
        }
        else
        {
            bValid = 0;
        }
    }

    return bValid;
}

int J3DAPI sithAIUtil_RetryMakeRandPoint(SithAIControlBlock* pLocal, rdVector3* pPos, float downCos, rdVector3* pDirection, rdVector3* pDestPoint)
{
    rdVector3 dir;

    if ( !sithAIUtil_mkPoinCurFlags || !sithAIUtil_pMkPointCurLocal || !pLocal )
    {
        return 0;
    }

    if ( pLocal != sithAIUtil_pMkPointCurLocal || !pPos || !pDirection || !pDestPoint )
    {
        SITHLOG_ERROR("sithAIUtil_RetryMakeRandPoint: Bad parameters.\n");
        return 0;
    }

    if ( (sithAIUtil_mkPoinCurFlags & 0x10400) != 0 )
    {
        sithAIUtil_mkPoinCurFlags = 0;
        sithAIUtil_pMkPointCurLocal = NULL;
        return 0;
    }

    if ( (sithAIUtil_mkPoinCurFlags & 6) != 0 )
    {
        rdVector_Neg3Acc(&sithAIUtil_mkPointCurPYR);
        rdVector_Rotate3(&dir, &sithAIUtil_mkPointCurDir, &sithAIUtil_mkPointCurPYR);
        sithAIUtil_mkPoinCurFlags &= ~6;
    }
    else if ( (sithAIUtil_mkPoinCurFlags & 1) != 0 )
    {
        rdVector_Neg3(&dir, &sithAIUtil_mkPointCurRVec);
        sithAIUtil_mkPoinCurFlags &= ~1;
    }
    else
    {
        sithAIUtil_mkPoinCurFlags = 0;
        sithAIUtil_pMkPointCurLocal = NULL;
        return 0;
    }

    rdVector_ScaleAdd3(pDestPoint, &dir, downCos, pPos);
    rdVector_Copy3(pDirection, &dir);
    return 1;
}

int J3DAPI sithAIUtil_CheckPathToPos(SithAIControlBlock* pLocal, SithSector* pStartSector, rdVector3* startPos, int flags, float minMoveDist, float moveDist, float radius, rdVector3* pDirection, rdVector3* pEndPos)
{
    SITH_ASSERTREL(pLocal && pLocal->pOwner);
    SithThing* pOwner = pLocal->pOwner;

    if ( !pDirection )
    {
        SITHLOG_ERROR("sithAIUtil_CheckPathToPos: Bad direction vector.\n");
        return 0;
    }

    if ( radius == 0.0f )
    {
        radius = pOwner->collide.movesize;
    }

    if ( (flags & 0x10) != 0 )
    {
        int result = 1;
        rdVector3 endPos;
        rdVector_ScaleAdd3(&endPos, pDirection, moveDist, startPos);

        sithCollision_SearchForCollisions(pStartSector, pOwner, startPos, pDirection, moveDist, radius, 0x102);
        SithCollision* pCollision;
        while ( (pCollision = sithCollision_PopStack()) != NULL )
        {
            if ( (flags & 0x20000) != 0 && (pCollision->type & SITHCOLLISION_THING) != 0 && pCollision->pThingCollided->type == SITH_THING_PLAYER )
            {
                continue;
            }

            if ( (flags & 0x40000) != 0 && (pCollision->type & SITHCOLLISION_THING) != 0 )
            {
                continue;
            }

            if ( (flags & 0x80000) != 0 && (pCollision->type & SITHCOLLISION_WORLD) != 0 && (pCollision->pSurfaceCollided->flags & 1) != 0 )
            {
                continue;
            }

            if ( pCollision->distance >= minMoveDist )
            {
                rdVector_ScaleAdd3(&endPos, pDirection, pCollision->distance, startPos);
            }
            else
            {
                result = 0;
                break;
            }

            if ( (pCollision->type & SITHCOLLISION_THING) != 0 )
            {
                result = 4;
                break;
            }

            if ( (pCollision->type & SITHCOLLISION_WORLD) != 0 )
            {
                result = 2;
                break;
            }
        }

        sithCollision_DecreaseStackLevel();
        if ( result )
        {
            float targetDist;
            rdVector3 toTarget;

            SithThing* pTarget = NULL;
            if ( (flags & 0x2000) != 0 && pLocal->goalThing )
            {
                pTarget = pLocal->goalThing;
            }

            if ( (flags & 0x800000) != 0 && pLocal->pTargetThing )
            {
                pTarget = pLocal->pTargetThing;
            }

            if ( pTarget && sithAIUtil_GetDistanceToTargetPos(pTarget, &pTarget->pos, &endPos, -1.0f, pLocal->pClass->sightDistance, 0.0f, &toTarget, &targetDist) == 3 )
            {
                return 0;
            }

            if ( pEndPos )
            {
                rdVector_Copy3(pEndPos, &endPos);
            }
        }

        return result;
    }

    if ( (flags & 0x20) != 0 )
    {
        int aScores[2] = { 0, 0 };
        int aResults[2] = { 0, 0 };
        rdVector3 aEndPos[2];

        int side = 1;
        int bSideDone = 0;
        rdVector3 perp;
        rdVector_Cross3(&perp, &pLocal->pOwner->orient.uvec, pDirection);
        perp.x = stdMath_ClipNearZero(perp.x);
        perp.y = stdMath_ClipNearZero(perp.y);
        perp.z = stdMath_ClipNearZero(perp.z);
        if ( rdVector_Normalize3Acc(&perp) == 0.0f )
        {
            rdVector_Cross3(&perp, pDirection, &pLocal->pOwner->orient.rvec);
            perp.x = stdMath_ClipNearZero(perp.x);
            perp.y = stdMath_ClipNearZero(perp.y);
            perp.z = stdMath_ClipNearZero(perp.z);
            rdVector_Normalize3Acc(&perp);
        }

        float distance = minMoveDist;
        while ( side >= 0 )
        {
            if ( bSideDone )
            {
                if ( --side < 0 )
                {
                    break;
                }

                bSideDone = 0;
                distance = minMoveDist;
                rdVector_Neg3Acc(&perp);
            }

            distance += 0.03f;
            if ( distance > moveDist )
            {
                bSideDone = 1;
                continue;
            }

            rdVector_ScaleAdd3(&aEndPos[side], &perp, distance, startPos);
            SithSector* pSector = sithCollision_FindSectorInRadius(pStartSector, startPos, &aEndPos[side], radius);
            if ( !pSector )
            {
                bSideDone = 1;
                continue;
            }

            sithCollision_SearchForCollisions(pStartSector, pOwner, startPos, &perp, distance, radius, 0x102);
            SithCollision* pCollision = sithCollision_PopStack();
            sithCollision_DecreaseStackLevel();
            if ( pCollision )
            {
                rdVector_ScaleAdd3(&aEndPos[side], &perp, pCollision->distance, startPos);
                if ( (pCollision->type & SITHCOLLISION_THING) != 0 )
                {
                    aResults[side] = 4;
                }
                else if ( (pCollision->type & SITHCOLLISION_WORLD) != 0 )
                {
                    aResults[side] = 2;
                }

                bSideDone = 1;
            }
            else
            {
                aResults[side] = sithAIUtil_CheckPathToPos(pLocal, pSector, &aEndPos[side], 0x80010, pOwner->collide.movesize + 0.03f, radius, 0.0f, pDirection, NULL);
                if ( aResults[side] == 1 )
                {
                    bSideDone = 1;
                }
            }
        }

        if ( pLocal->goalThing )
        {
            for ( int i = 0; i < 2; ++i )
            {
                int bVisible = sithAIUtil_GetDistanceToTargetPos(pLocal->goalThing, &pLocal->goalThing->pos, &aEndPos[i], -1.0f, pLocal->pClass->sightDistance, 0.0f, &pLocal->vecUnknown0, &pLocal->targetDistance) != 3;
                if ( bVisible )
                {
                    ++aScores[i];
                }
                else if ( (flags & 0x2000) != 0 )
                {
                    --aScores[i];
                }
            }

            float goalDist = sqrtf((aEndPos[0].x - pLocal->goalThing->pos.x) * (aEndPos[0].x - pLocal->goalThing->pos.x) + (aEndPos[0].y - pLocal->goalThing->pos.y) * (aEndPos[0].y - pLocal->goalThing->pos.y));
            if ( goalDist < sqrtf((aEndPos[1].x - pLocal->goalThing->pos.x) * (aEndPos[1].x - pLocal->goalThing->pos.x) + (aEndPos[1].y - pLocal->goalThing->pos.y) * (aEndPos[1].y - pLocal->goalThing->pos.y)) )
            {
                if ( (flags & 0x100) != 0 )
                {
                    ++aScores[0];
                }
                else if ( (flags & 0x200) != 0 )
                {
                    --aScores[0];
                }
            }
            else
            {
                if ( (flags & 0x100) != 0 )
                {
                    ++aScores[1];
                }
                else if ( (flags & 0x200) != 0 )
                {
                    --aScores[1];
                }
            }
        }

        int i = aScores[0] < aScores[1];
        if ( pEndPos )
        {
            rdVector_Copy3(pEndPos, &aEndPos[i]);
        }

        return aResults[i];
    }

    return 0;
}

int J3DAPI sithAIUtil_SweepMoveToPathPos(SithAIControlBlock* pLocal, rdVector3* pPos, float angle, float maxAngle, float deltaAngle, int flags)
{
    rdVector3 toTargetDir;
    rdVector_Sub3(&toTargetDir, pPos, &pLocal->pOwner->pos);
    toTargetDir.z = 0.0f;
    float pathDist = rdVector_Normalize3Acc(&toTargetDir);

    flags &= 0x0FFFFF00;
    int randFlags = flags | 0x8002;
    float curAngle = angle;
    while ( curAngle <= maxAngle )
    {
        rdVector3 randDir;
        rdVector_Copy3(&randDir, &toTargetDir);
        rdVector3 destPos;
        if ( !sithAIUtil_MakeRandPoint(pLocal, &pLocal->pOwner->pos, randFlags, curAngle, &pathDist, &randDir, &destPos) )
        {
            return 0;
        }

        int bFound;
        float hitDist;
        rdVector3 surfaceNorm;
        while ( (bFound = sithAIUtil_CheckPathToPoint(pLocal->pOwner, &destPos, pLocal->pOwner->collide.movesize, &hitDist, &surfaceNorm, 1, 0)) == 0
            && curAngle != 0.0f
            && curAngle != 180.0f
            && sithAIUtil_RetryMakeRandPoint(pLocal, &pLocal->pOwner->pos, pathDist, &toTargetDir, &destPos) )
        {
        }

        if ( bFound )
        {
            sithAIMove_StopAIMovement(pLocal);
            sithAIMove_AISetLookPosEyeLevel(pLocal, &destPos);
            sithAIMove_AISetMoveTargetPos(pLocal, &destPos, pLocal->moveSpeed);
            return 1;
        }

        curAngle += deltaAngle;
    }

    return 0;
}

int J3DAPI sithAIUtil_MakeRoamPos(SithAIControlBlock* pLocal, rdVector3* pPos, float minDist, float moveDist, int bAllow3D, int maxRetries, rdVector3* pDestPos)
{
    size_t numRetries = 0;
    rdVector3 dir;
    rdVector_Copy3(&dir, &pLocal->pOwner->orient.lvec);

    int currentflags;
retry:
    currentflags = 0xC002;
    if ( bAllow3D )
    {
        currentflags |= 4;
    }

    if ( !sithAIUtil_MakeRandPoint(pLocal, pPos, currentflags, 360.0f, &moveDist, &dir, pDestPos) )
    {
        return 0;
    }

    int curresult;
check:
    if ( bAllow3D )
    {
        currentflags = 0x10;
    }
    else
    {
        currentflags = 0x80010;
    }

    curresult = sithAIUtil_CheckPathToPos(pLocal, pLocal->pOwner->pInSector, &pLocal->pOwner->pos, currentflags, minDist, moveDist, 0.0f, &dir, pDestPos);
    if ( !curresult )
    {
        if ( sithAIUtil_RetryMakeRandPoint(pLocal, &pLocal->pOwner->pos, moveDist, &dir, pDestPos) )
        {
            goto check;
        }

        if ( ++numRetries < (size_t)maxRetries )
        {
            goto retry;
        }
    }

    if ( curresult )
    {
        return 1;
    }

    return 0;
}

int J3DAPI sithAIUtil_MakePathPos(SithAIControlBlock* pLocal, rdVector3* pPos, float minDist, float moveDist, float angle, int flags, rdVector3* pDirection, rdVector3* pDestPos)
{
    if ( !sithAIUtil_MakeRandPoint(pLocal, pPos, flags, angle, &moveDist, pDirection, pDestPos) )
    {
        return 0;
    }

    int pathFlags = flags & 0x8E2000;
    if ( (flags & 4) != 0 )
    {
        pathFlags |= 0x10;
    }
    else
    {
        pathFlags |= 0x80010;
    }

    while ( sithAIUtil_CheckPathToPos(pLocal, pLocal->pOwner->pInSector, &pLocal->pOwner->pos, pathFlags, minDist, moveDist, 0.0f, pDirection, pDestPos) == 0 )
    {
        if ( sithAIUtil_RetryMakeRandPoint(pLocal, &pLocal->pOwner->pos, moveDist, pDirection, pDestPos) )
        {
            continue;
        }

        return 0;
    }

    return 1;
}

int J3DAPI sithAIUtil_IsLineOfFireClear(const SithAIControlBlock* pLocal, const rdVector3* pPos)
{
    rdVector3 curdir;

    if ( !pLocal || !pLocal->pOwner || pLocal->pOwner->controlType != SITH_CT_AI )
    {
        return 1;
    }

    if ( !sithWeapon_HasWeaponSelected(pLocal->pOwner) )
    {
        return 1;
    }

    if ( !pLocal->pTargetThing )
    {
        return 1;
    }

    rdVector_Sub3(&curdir, &pLocal->pTargetThing->pos, pPos);
    float dist = rdVector_Normalize3Acc(&curdir);

    SithAIControlBlock* cur = sithAI_g_aControlBlocks;
    for ( int idx = 0; idx <= sithAI_g_lastUsedAIIndex; ++idx, ++cur )
    {
        if ( !cur || !cur->pOwner || cur->pOwner->controlType != SITH_CT_AI )
        {
            continue;
        }

        if ( (cur->pOwner->thingInfo.actorInfo.flags & SITH_AF_HUMAN) == 0 )
        {
            continue;
        }

        if ( (cur->mode & SITHAI_MODE_TARGETVISIBLE) == 0 )
        {
            continue;
        }

        if ( cur == pLocal )
        {
            continue;
        }

        float dot = rdVector_Dot3(&cur->toTarget, &curdir);
        if ( dot >= 0.966f && dist > cur->distance )
        {
            return 0;
        }
    }

    return 1;
}

void J3DAPI sithAIUtil_AIPlaySoundMode(SithAIControlBlock* pLocal, unsigned int mode)
{
    if ( !pLocal || !pLocal->pOwner )
    {
        return;
    }

    if ( mode >= SITHSOUNDCLASS_CURIOUS && mode <= SITHSOUNDCLASS_RESERVED8 && sithAI_HasInstinct(pLocal, "talk") )
    {
        sithAI_EmitEvent(pLocal, SITHAI_EVENT_TALK, (void*)mode);
    }
    else
    {
        sithSoundClass_PlayModeRandom(pLocal->pOwner, (SithSoundClassMode)mode);
    }
}

void J3DAPI sithAIUtil_AISetMode(SithAIControlBlock* pLocal, SithAIMode mode)
{
    switch ( mode )
    {
        case SITHAI_MODE_MOVING:
            pLocal->mode |= SITHAI_MODE_MOVING;
            pLocal->submode |= SITHAI_SUBMODE_UNKNOWN_4000;
            break;

        case SITHAI_MODE_SEARCHING:
            pLocal->mode |= SITHAI_MODE_SEARCHING;
            pLocal->mode &= ~(SITHAI_MODE_CHASE_GOAL | SITHAI_MODE_ACTIVE);
            pLocal->mode &= ~(SITHAI_MODE_CIRCLESTRAFING | SITHAI_MODE_ATTACKING);
            pLocal->mode &= ~(SITHAI_MODE_UNKNOWN_1000000 | SITHAI_MODE_TARGETVISIBLE);
            pLocal->mode &= ~(SITHAI_MODE_FLEEINGTOWAYPOINT | SITHAI_MODE_FLEEING);
            pLocal->mode &= ~(SITHAI_MODE_BLOCK | SITHAI_MODE_NOCHECKFORCLIFF);
            sithActor_SetHeadPYR(pLocal->pOwner, &rdroid_g_zeroVector3);
            break;

        case SITHAI_MODE_ATTACKING:
            pLocal->mode |= (SITHAI_MODE_ACTIVE | SITHAI_MODE_ATTACKING);
            pLocal->mode &= ~(SITHAI_MODE_CHASE_GOAL | SITHAI_MODE_SEARCHING);
            pLocal->mode &= ~(SITHAI_MODE_FLEEINGTOWAYPOINT | SITHAI_MODE_FLEEING);
            pLocal->mode &= ~(SITHAI_MODE_BLOCK | SITHAI_MODE_NOCHECKFORCLIFF);
            break;

        case SITHAI_MODE_HUNTING:
            pLocal->mode |= SITHAI_MODE_HUNTING;
            pLocal->mode &= ~(SITHAI_MODE_FLEEINGTOWAYPOINT | SITHAI_MODE_FLEEING);
            pLocal->mode &= ~(SITHAI_MODE_BLOCK | SITHAI_MODE_NOCHECKFORCLIFF);
            pLocal->mode &= ~SITHAI_MODE_SLEEPING;
            break;

        case SITHAI_MODE_FLEEINGTOWAYPOINT:
            pLocal->mode |= SITHAI_MODE_FLEEING;
            pLocal->mode |= SITHAI_MODE_FLEEINGTOWAYPOINT;
            pLocal->mode &= ~SITHAI_MODE_SEARCHING;
            pLocal->mode &= ~(SITHAI_MODE_CHASE_GOAL | SITHAI_MODE_ACTIVE);
            pLocal->mode &= ~(SITHAI_MODE_CIRCLESTRAFING | SITHAI_MODE_ATTACKING);
            pLocal->mode &= ~(SITHAI_MODE_UNKNOWN_1000000 | SITHAI_MODE_TARGETVISIBLE);
            pLocal->mode &= ~(SITHAI_MODE_BLOCK | SITHAI_MODE_NOCHECKFORCLIFF);
            pLocal->mode &= ~SITHAI_MODE_SLEEPING;
            pLocal->mode &= ~SITHAI_MODE_HUNTING;
            break;

        case SITHAI_MODE_FLEEING:
            pLocal->mode |= SITHAI_MODE_FLEEING;
            pLocal->mode &= ~SITHAI_MODE_SEARCHING;
            pLocal->mode &= ~(SITHAI_MODE_CHASE_GOAL | SITHAI_MODE_ACTIVE);
            pLocal->mode &= ~(SITHAI_MODE_CIRCLESTRAFING | SITHAI_MODE_ATTACKING);
            pLocal->mode &= ~(SITHAI_MODE_UNKNOWN_1000000 | SITHAI_MODE_TARGETVISIBLE);
            pLocal->mode &= ~(SITHAI_MODE_BLOCK | SITHAI_MODE_NOCHECKFORCLIFF);
            pLocal->mode &= ~SITHAI_MODE_SLEEPING;
            pLocal->submode &= ~SITHAI_SUBMODE_UNKNOWN_1;
            break;

        case SITHAI_MODE_BLOCK:
            pLocal->mode |= (SITHAI_MODE_BLOCK | SITHAI_MODE_NOCHECKFORCLIFF);
            pLocal->mode |= SITHAI_MODE_SEARCHING;
            pLocal->mode &= ~(SITHAI_MODE_CHASE_GOAL | SITHAI_MODE_ACTIVE);
            pLocal->mode &= ~(SITHAI_MODE_CIRCLESTRAFING | SITHAI_MODE_ATTACKING);
            pLocal->mode &= ~(SITHAI_MODE_UNKNOWN_1000000 | SITHAI_MODE_TARGETVISIBLE);
            break;

        case SITHAI_MODE_WALLCRAWLING:
            pLocal->mode |= SITHAI_MODE_WALLCRAWLING;
            pLocal->submode |= SITHAI_SUBMODE_UNKNOWN_100;
            break;
    }
}

void J3DAPI sithAIUtil_AIClearMode(SithAIControlBlock* pLocal, SithAIMode mode)
{
    switch ( mode )
    {
        case SITHAI_MODE_MOVING:
            pLocal->mode &= ~SITHAI_MODE_MOVING;
            pLocal->mode &= ~SITHAI_MODE_UNKNOWN_10;
            pLocal->submode &= ~(SITHAI_SUBMODE_UNKNOWN_1 | SITHAI_SUBMODE_UNKNOWN_2);
            pLocal->submode &= ~(SITHAI_SUBMODE_UNKNOWN_4000 | SITHAI_SUBMODE_UNKNOWN_4);
            pLocal->submode &= ~(SITHAI_SUBMODE_UNKNOWN_8 | SITHAI_SUBMODE_UNKNOWN_10);
            break;

        case SITHAI_MODE_TURNING:
            pLocal->mode &= ~SITHAI_MODE_TURNING;
            pLocal->mode &= ~SITHAI_MODE_UNKNOWN_80;
            pLocal->submode &= ~SITHAI_SUBMODE_UNKNOWN_10;
            break;

        case SITHAI_MODE_TRAVERSEWPNTS:
            pLocal->mode &= ~SITHAI_MODE_TRAVERSEWPNTS;
            pLocal->mode &= ~SITHAI_MODE_HUNTING;
            pLocal->mode &= ~SITHAI_MODE_FLEEINGTOWAYPOINT;
            pLocal->submode &= ~SITHAI_SUBMODE_SEMICONTINUOUSWPNTMOTION;
            sithAIUtil_ClearAIWaypoint(pLocal);
            break;

        case SITHAI_MODE_HUNTING:
            pLocal->mode &= ~SITHAI_MODE_HUNTING;
            if ( (pLocal->submode & SITHAI_SUBMODE_CONTINUOUSWPNTMOTION) == 0 )
            {
                pLocal->mode &= ~SITHAI_MODE_TRAVERSEWPNTS;
                pLocal->mode &= ~SITHAI_MODE_FLEEINGTOWAYPOINT;
                pLocal->submode &= ~SITHAI_SUBMODE_SEMICONTINUOUSWPNTMOTION;
                sithAIUtil_ClearAIWaypoint(pLocal);
            }
            break;

        case SITHAI_MODE_WALLCRAWLING:
            pLocal->mode &= ~SITHAI_MODE_WALLCRAWLING;
            pLocal->submode &= ~SITHAI_SUBMODE_UNKNOWN_100;
            break;
    }
}

void J3DAPI sithAIUtil_SetWeaponFireFlags(int weaponNum, SithAIUtilFireFlags* pFlags)
{
    switch ( weaponNum )
    {
        case 0:
            *pFlags |= SITHAIUTIL_FIRE_UNKNOWN_40;
            break;

        case 1:
            *pFlags |= SITHAIUTIL_FIRE_UNKNOWN_80;
            break;

        case 2:
            *pFlags |= SITHAIUTIL_FIRE_UNKNOWN_100;
            break;

        case 3:
            *pFlags |= SITHAIUTIL_FIRE_UNKNOWN_200;
            break;
    }
}

int J3DAPI sithAIUtil_AIGetMovePos(const SithAIControlBlock* pLocal, int flags, rdVector3* pDestPos)
{
    if ( (flags & 0x100000) != 0 )
    {
        rdVector_Copy3(pDestPos, &pLocal->pOwner->pos);
        return 1;
    }

    if ( (flags & 0x400) != 0 )
    {
        rdVector_Copy3(pDestPos, &pLocal->movePos);
        return 1;
    }

    if ( (flags & 0x200000) != 0 )
    {
        rdVector_Copy3(pDestPos, &pLocal->homePos);
        return 1;
    }

    if ( (flags & 0x300) != 0 )
    {
        if ( pLocal->goalThing )
        {
            rdVector_Copy3(pDestPos, &pLocal->goalThing->pos);
        }
        else
        {
            rdVector_Copy3(pDestPos, &pLocal->vecUnknown6);
        }

        return 1;
    }

    if ( (flags & 0x1000) != 0 )
    {
        if ( pLocal->pFleeFromThing )
        {
            rdVector_ScaleAdd3(pDestPos, &pLocal->pFleeFromThing->moveInfo.physics.velocity, sithTime_g_frameTimeFlex, &pLocal->pFleeFromThing->pos);
        }
    }

    return 0;
}

signed int J3DAPI sithAIUtil_GetXYHeadingVector(const SithThing* pThing, rdVector3* pDest)
{
    if ( !pThing || !pDest )
    {
        SITHLOG_ERROR("Bad args passed to sithAIUtil_GetXYHeadingVector().\n");
        return 0;
    }

    if ( pThing->moveInfo.physics.velocity.x != 0.0f || pThing->moveInfo.physics.velocity.y != 0.0f || pThing->moveInfo.physics.velocity.z != 0.0f )
    {
        rdVector_Copy3(pDest, &pThing->moveInfo.physics.velocity);
    }
    else
    {
        rdVector_Copy3(pDest, &pThing->orient.lvec);
    }

    pDest->z = 0.0f;
    pDest->x = stdMath_ClipNearZero(pDest->x);
    pDest->y = stdMath_ClipNearZero(pDest->y);
    pDest->z = stdMath_ClipNearZero(pDest->z);
    rdVector_Normalize3Acc(pDest);
    return 1;
}

signed int J3DAPI sithAIUtil_GetXYZHeadingVector(const SithThing* pThing, rdVector3* pDest)
{
    if ( !pThing || !pDest )
    {
        SITHLOG_ERROR("Bad args passed to sithAIUtil_GetXYHeadingVector().\n");
        return 0;
    }

    if ( pThing->moveInfo.physics.velocity.x != 0.0f || pThing->moveInfo.physics.velocity.y != 0.0f || pThing->moveInfo.physics.velocity.z != 0.0f )
    {
        rdVector_Copy3(pDest, &pThing->moveInfo.physics.velocity);
        pDest->x = stdMath_ClipNearZero(pDest->x);
        pDest->y = stdMath_ClipNearZero(pDest->y);
        pDest->z = stdMath_ClipNearZero(pDest->z);
        rdVector_Normalize3Acc(pDest);
    }
    else
    {
        rdVector_Copy3(pDest, &pThing->orient.lvec);
    }

    return 1;
}

int J3DAPI sithAIUtil_CanAttackTarget(const SithThing* pThing)
{
    if ( !pThing )
    {
        return 0;
    }

    if ( (pThing->flags & (SITH_TF_DISABLED | SITH_TF_DYING | SITH_TF_DESTROYED)) != 0 )
    {
        return 0;
    }

    if ( pThing->type == SITH_THING_PLAYER && (pThing->thingInfo.actorInfo.flags & SITH_AF_CONTROLSDISABLED) != 0 )
    {
        return 0;
    }

    return 1;
}

int J3DAPI sithAIUtil_IsThingMoving(const SithThing* pThing)
{
    int bMoving = 0;
    if ( !pThing )
    {
        return 0;
    }

    switch ( pThing->type )
    {
        case SITH_THING_FREE:
            break;

        case SITH_THING_ACTOR:
            if ( !pThing->controlInfo.aiControl.pLocal )
            {
                return 0;
            }

            bMoving = pThing->controlInfo.aiControl.pLocal->mode & SITHAI_MODE_MOVING;
            break;

        default:
            if ( pThing->moveType == SITH_MT_PHYSICS )
            {
                bMoving = (pThing->moveInfo.physics.velocity.x != 0.0f || pThing->moveInfo.physics.velocity.y != 0.0f || pThing->moveInfo.physics.velocity.z != 0.0f)
                    | (pThing->moveInfo.physics.angularVelocity.x != 0.0f || pThing->moveInfo.physics.angularVelocity.y != 0.0f || pThing->moveInfo.physics.angularVelocity.z != 0.0f);
            }
            else if ( pThing->moveType == SITH_MT_PATH )
            {
                bMoving = pThing->moveInfo.pathMovement.mode & 1;
            }
            break;
    }

    return bMoving;
}

void J3DAPI sithAIUtil_ApplyForce(SithAIControlBlock* pLocal, const rdVector3* pDirection, float force)
{
    rdVector3 vecForce;

    rdVector_Scale3(&vecForce, pDirection, pLocal->pOwner->moveInfo.physics.mass * force);
    sithPhysics_ApplyForce(pLocal->pOwner, &vecForce);
}

void J3DAPI sithAIUtil_RandomRotate(rdVector3* pOutPYR, float scalar)
{
    pOutPYR->x = (float)((SITH_RAND() - 0.5f) * scalar + pOutPYR->x);
    pOutPYR->y = (float)((SITH_RAND() - 0.5f) * scalar + pOutPYR->y);
    pOutPYR->z = (float)((SITH_RAND() - 0.5f) * scalar + pOutPYR->z);
    rdVector_Normalize3Acc(pOutPYR);
}

size_t J3DAPI sithAIUtil_GetThingsInView(SithSector* pStartSector, const rdMatrix34* orient, float fovX, float fovY, int sizeResult, SithThing** pResult, int thingTypeMask, float maxDistance)
{
    float sinFov;

    SITH_ASSERTREL(pStartSector!=NULL);

    if ( fovX < 0.0f || fovY < 0.0f )
    {
        return 0;
    }

    sithAIUtil_sizeThingsInView = sizeResult;
    sithAIUtil_thingInViewTypeMask = thingTypeMask;
    sithAIUtil_maxDistanceToThingsInView = maxDistance;
    sithAIUtil_aThingsInView = pResult;

    float angleX = 90.0f - (fovX / 2.0f);
    float curangleY = 90.0f - (fovY / 2.0f);
    stdMath_SinCos(angleX, &sinFov, &sithAIUtil_thingsInViewFovX);
    stdMath_SinCos(curangleY, &sinFov, &sithAIUtil_thingsInViewFovY);

    sithAdvanceRenderTick();
    sithAIUtil_numThingsInView = 0;
    sithAIUtil_numVisitedSectors = 0;
    sithAIUtil_GetNextThingsInView(pStartSector, orient, 0.0f);
    return sithAIUtil_numThingsInView;
}

void J3DAPI sithAIUtil_GetNextThingsInView(SithSector* pSector, const rdMatrix34* pOrient, float distance)
{
    rdVector3 relPosDir;
    rdVector3 upDir;
    rdVector3 rightNorm;
    rdVector3 forwardVec;

    if ( pSector->renderTick != sithMain_g_curRenderTick )
    {
        pSector->renderTick = sithMain_g_curRenderTick;
        if ( sithAIUtil_numVisitedSectors >= 128 )
        {
            return;
        }

        ++sithAIUtil_numVisitedSectors;

        SithThing* pCurThing = pSector->pFirstThingInSector;
        while ( pCurThing && sithAIUtil_numThingsInView < sithAIUtil_sizeThingsInView )
        {
            if ( ((1 << pCurThing->type) & sithAIUtil_thingInViewTypeMask) != 0 && (pCurThing->flags & (SITH_TF_DISABLED | SITH_TF_DYING | SITH_TF_DESTROYED)) == 0 )
            {
                rdVector_Sub3(&relPosDir, &pCurThing->pos, &pOrient->dvec);
                rdVector_Normalize3Acc(&relPosDir);

                rdVector_Normalize3(&upDir, &pOrient->uvec);
                rdVector_Normalize3(&rightNorm, &pOrient->rvec);
                rdVector_Normalize3(&forwardVec, &pOrient->lvec);

                float dotUpDist = rdVector_Dot3(&upDir, &relPosDir);
                float dotRightDist = rdVector_Dot3(&rightNorm, &relPosDir);
                float dotAheadDist = rdVector_Dot3(&forwardVec, &relPosDir);

                if ( dotUpDist <= sithAIUtil_thingsInViewFovY && dotUpDist >= -sithAIUtil_thingsInViewFovY
                    && dotRightDist <= sithAIUtil_thingsInViewFovX && dotRightDist >= -sithAIUtil_thingsInViewFovX
                    && dotAheadDist >= 0.0f )
                {
                    if ( sithAIUtil_numThingsInView < sithAIUtil_sizeThingsInView )
                    {
                        sithAIUtil_aThingsInView[sithAIUtil_numThingsInView++] = pCurThing;
                    }
                    else
                    {
                        return;
                    }
                }
            }

            pCurThing = pCurThing->pNextThingInSector;
        }

        if ( distance > sithAIUtil_maxDistanceToThingsInView )
        {
            return;
        }

        SithSurfaceAdjoin* pAdjoin = pSector->pFirstAdjoin;
        while ( pAdjoin )
        {
            SithSurface* pSurface = pAdjoin->pAdjoinSurface;
            rdFace* pFace = &pSurface->face;
            if ( (pAdjoin->flags & SITH_ADJOIN_VISIBLE) != 0
                && (!pFace->pMaterial || !pFace->geometryMode || (pFace->flags & RD_FF_TEX_TRANSLUCENT) != 0 || pFace->pMaterial->formatType != STDCOLOR_FORMAT_RGB)
                && rdVector_Dot3(&pFace->normal, &pOrient->lvec) < 0.0f )
            {
                float adjoinDist = distance + pAdjoin->distance + pAdjoin->pMirrorAdjoin->distance;
                sithAIUtil_GetNextThingsInView(pAdjoin->pAdjoinSector, pOrient, adjoinDist);
            }

            pAdjoin = pAdjoin->pNextAdjoin;
        }
    }
}

int J3DAPI sithAIUtil_CanSeeTarget(const SithAIControlBlock* pLocal, SithThing* pTarget, float distance)
{
    SithThing* pOwner = pLocal->pOwner;
    float chance = 1.0f;

    if ( !pTarget || pTarget->type != SITH_THING_ACTOR && pTarget->type != SITH_THING_PLAYER )
    {
        return 1;
    }

    if ( distance < 2.0f )
    {
        goto check_invisible;
    }

    if ( (pLocal->mode & SITHAI_MODE_ACTIVE) == 0 )
    {
        chance *= 0.5f;
    }

    if ( (pTarget->thingInfo.actorInfo.flags & SITH_AF_HEADLIGHT) != 0 || (pTarget->unknownFlags & 1) != 0 )
    {
        goto check_invisible;
    }

    float distFactor = (distance - 2.0f) * 0.1f;
    distFactor = STDMATH_CLAMP(distFactor, 0.0f, 0.6f);
    chance = (1.0f - distFactor) * chance;

    if ( (pOwner->thingInfo.actorInfo.flags & SITH_AF_SEEINDARK) == 0 && rdLight_GetIntensity(&pTarget->pInSector->ambientLight) < 0.5f )
    {
        chance = (rdLight_GetIntensity(&pTarget->pInSector->ambientLight) + 0.2f) * chance;
    }

    if ( pTarget->moveType == SITH_MT_PHYSICS )
    {
        if ( (pTarget->moveInfo.physics.flags & SITH_PF_CROUCHING) != 0 )
        {
            chance *= 0.75f;
        }

        if ( pTarget->moveInfo.physics.velocity.x == 0.0f && pTarget->moveInfo.physics.velocity.y == 0.0f && pTarget->moveInfo.physics.velocity.z == 0.0f )
        {
            chance *= 0.5f;
        }
    }

check_invisible:
    if ( (pOwner->thingInfo.actorInfo.flags & SITH_AF_SEEINVISIBLE) == 0 )
    {
        if ( (pTarget->thingInfo.actorInfo.flags & SITH_AF_INVISIBLE) != 0 )
        {
            chance *= 0.05f;
        }

        if ( (pOwner->thingInfo.actorInfo.flags & SITH_AF_BLIND) != 0 )
        {
            chance *= 0.05f;
        }
    }

    chance = STDMATH_CLAMP(chance, 0.05f, 1.0f);
    if ( SITH_RAND() < chance )
    {
        return 1;
    }

    return 0;
}

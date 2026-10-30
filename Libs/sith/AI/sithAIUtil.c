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

#define SITHAIUTIL_MKPOINT_ROTATE       (SITHAIUTIL_MKPOINT_YAW | SITHAIUTIL_MKPOINT_PITCH)
#define SITHAIUTIL_MKPOINT_TOWARDREF    (SITHAIUTIL_MKPOINT_TOWARDGOAL | SITHAIUTIL_MKPOINT_TOWARDMOVEPOS | SITHAIUTIL_MKPOINT_REFOWNER)
#define SITHAIUTIL_MKPOINT_AWAYFROMREF  (SITHAIUTIL_MKPOINT_AWAYFROMGOAL | SITHAIUTIL_MKPOINT_AWAYFROMFLEE)
#define SITHAIUTIL_MKPOINT_ANYREF       (SITHAIUTIL_MKPOINT_TOWARDREF | SITHAIUTIL_MKPOINT_AWAYFROMREF)
#define SITHAIUTIL_MKPOINT_NORETRY      (SITHAIUTIL_MKPOINT_TARGETDIST | SITHAIUTIL_MKPOINT_TOWARDMOVEPOS)

// Flags of sithAIUtil_MakePathPos that are passed over to the path check
#define SITHAIUTIL_MAKEPATH_PATHFLAGS (SITHAIUTIL_PATHFLAG_GOALVISIBLE | SITHAIUTIL_PATHFLAG_IGNOREPLAYER | SITHAIUTIL_PATHFLAG_IGNORETHINGS | SITHAIUTIL_PATHFLAG_IGNOREFLOORS | SITHAIUTIL_PATHFLAG_TARGETVISIBLE)

// Flags of sithAIUtil_SweepMoveToPathPos that are cleared, i.e. the passed flags are masked with 0x0FFF7F00 (~0xF00080FF) before the point creation
#define SITHAIUTIL_SWEEP_CLEARFLAGS (SITHAIUTIL_MKPOINT_SIDEWAYS | SITHAIUTIL_MKPOINT_YAW | SITHAIUTIL_MKPOINT_PITCH | 0xF8u | SITHAIUTIL_MKPOINT_RANDSIDE | 0xF0000000u)

#define SITHAIUTIL_MAXTHINGSINVIEWSECTORS 128u

// Score adjustments of the waypoint link selection
#define SITHAIUTIL_WPNTSCORE_FLEEMAXRANK    14 // Penalty for the waypoint with the max rank, i.e. the waypoint the AI flees from
#define SITHAIUTIL_WPNTSCORE_FLEEFACING     10 // Penalty for the waypoint in the direction of the flee-from thing
#define SITHAIUTIL_WPNTSCORE_FLEERANKSTEP   3  // Score per rank difference when fleeing
#define SITHAIUTIL_WPNTSCORE_BACKTRACK      6  // Penalty for the waypoint the AI came from
#define SITHAIUTIL_WPNTSCORE_FLEEBACKTRACK  1
#define SITHAIUTIL_WPNTSCORE_RANKBONUS      7  // Bonus for the waypoint with the better rank

//
// Waypoint link selection vars
//
static float sithAIUtil_fleeFacingMinDot  = 0.923f;
static float sithAIUtil_fleeFacingMaxDot  = 0.984f;
static float sithAIUtil_fleeFacingMinDist = 0.5f;
static float sithAIUtil_fleeFacingMaxDist = 1.5f;

static float sithAIUtil_wpntRenderInterval = 1.0f; // In seconds

static SithAIWaypoint sithAIUtil_aAIWpnts[SITHAI_MAXWPNTS]                 = { 0 };
static SithAIWaypointOwner sithAIUtil_aWpntOwners[SITHAI_MAXWPNTOWNERS]    = { 0 };
static SithAIWaypointDistance sithAIUtil_aWpntDistances[SITHAI_MAXWPNTS]   = { 0 };
static SithAIWaypointLayerFlag sithAIUtil_activeWpntLayer                  = 0;
static float sithAIUtil_secLastUpdate                                      = 0.0f;

//
// Make point vars
//
static SithAIUtilMkPointFlags sithAIUtil_mkPointCurFlags = 0;
static SithAIControlBlock* sithAIUtil_pMkPointCurLocal   = NULL;
static rdVector3 sithAIUtil_mkPointCurDir                = { 0 };
static rdVector3 sithAIUtil_mkPointCurPYR                = { 0 };
static rdVector3 sithAIUtil_mkPointCurRVec               = { 0 };

//
// Things in view vars
//
static float sithAIUtil_thingsInViewFovX                = 0.0f;
static float sithAIUtil_thingsInViewFovY                = 0.0f;
static size_t sithAIUtil_numVisitedSectors              = 0;
static size_t sithAIUtil_numThingsInView                = 0;
static size_t sithAIUtil_sizeThingsInView               = 0;
static SithThing** sithAIUtil_aThingsInView             = NULL;
static float sithAIUtil_maxDistanceToThingsInView       = 0.0f;
static int sithAIUtil_thingInViewTypeMask               = 0;

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
    memset(&sithAIUtil_g_bRenderAIWpnts, 0, sizeof(sithAIUtil_g_bRenderAIWpnts));
}

void J3DAPI sithAIUtil_UpdateTargetSight(SithAIControlBlock* pLocal, SithAIUtilFireFlags flags)
{
    SithAISightState sightState = pLocal->targetSightState;
    SithThing* pOwner           = pLocal->pOwner;
    float curCollideSize        = 0.0f;
    if ( (flags & (SITHAIUTIL_FIRE_LOB | SITHAIUTIL_FIRE_PRIMARY | SITHAIUTIL_FIRE_ALT)) == 0 && (size_t)pLocal->targetSightFrame == sithMain_g_frameNumber )
    {
        return;
    }

    pLocal->targetSightFrame = sithMain_g_frameNumber;
    if ( sithWeapon_HasWeaponSelected(pOwner) )
    {
        curCollideSize = sithWeapon_GetWeaponCollideSize(pOwner->thingInfo.actorInfo.weaponInfo.curWeaponID);
    }
    else if ( pOwner->thingInfo.actorInfo.pWeaponTemplate )
    {
        curCollideSize = pOwner->thingInfo.actorInfo.pWeaponTemplate->collide.movesize;
    }

    rdMatrix_TransformVector34(&pLocal->weaponFirePos, &pOwner->thingInfo.actorInfo.fireOffset, &pOwner->orient);
    rdVector_Add3Acc(&pLocal->weaponFirePos, &pOwner->pos);

    rdVector3 curStartPos = pLocal->weaponFirePos;

    if ( (pLocal->submode & SITHAI_SUBMODE_FIREADDEYEOFFSET) != 0
        && !rdVector_IsZero3(&pOwner->thingInfo.actorInfo.eyeOffset) )
    {
        curStartPos.z = pOwner->thingInfo.actorInfo.eyeOffset.z * 0.5f + curStartPos.z;
    }

    pLocal->mode &= ~SITHAI_MODE_TARGETVISIBLE;
    SithThing* pTarget = pLocal->pTargetThing;
    if ( pTarget )
    {
        if ( (pLocal->pOwner->thingInfo.actorInfo.flags & SITH_AF_SEEINVISIBLE) == 0 )
        {
            if ( (pTarget->type == SITH_THING_ACTOR || pTarget->type == SITH_THING_PLAYER) && (pTarget->thingInfo.actorInfo.flags & SITH_AF_INVISIBLE) != 0 )
            {
                sightState = SITHAI_SIGHT_HIDDEN;
            }

            if ( (pLocal->pOwner->thingInfo.actorInfo.flags & SITH_AF_BLIND) != 0 )
            {
                sightState = SITHAI_SIGHT_HIDDEN;
            }
        }

        pLocal->targetPos        = pTarget->pos;
        pLocal->targetSightState = sithAIUtil_GetDistanceToTarget(pOwner, &curStartPos, pTarget, pLocal->pClass->fov, pLocal->pClass->sightDistance, curCollideSize, &pLocal->toTarget, &pLocal->distance);

        if ( pLocal->targetSightState == SITHAI_SIGHT_VISIBLE )
        {
            if ( sightState != SITHAI_SIGHT_VISIBLE && !sithAIUtil_CanSeeTarget(pLocal, pTarget, pLocal->distance) )
            {
                pLocal->targetSightState = SITHAI_SIGHT_HIDDEN;
                return;
            }

            pLocal->lastTargetSeenPos  = pTarget->pos;
            pLocal->msecAttackStart = sithTime_g_msecGameTime;
            pLocal->mode |= SITHAI_MODE_TARGETVISIBLE;
        }
    }
    else
    {
        pLocal->targetSightState = sithAIUtil_GetDistanceToTargetPos(pOwner, &curStartPos, &pLocal->targetPos, pLocal->pClass->fov, pLocal->pClass->sightDistance, curCollideSize, &pLocal->toTarget, &pLocal->distance);

        if ( pLocal->targetSightState == SITHAI_SIGHT_VISIBLE )
        {
            pLocal->lastTargetSeenPos   = pLocal->targetPos;
            pLocal->msecAttackStart = sithTime_g_msecGameTime;
            pLocal->mode |= SITHAI_MODE_TARGETVISIBLE;
        }
    }

    if ( (pLocal->submode & SITHAI_SUBMODE_FIREADDEYEOFFSET) != 0
        && !rdVector_IsZero3(&pOwner->thingInfo.actorInfo.eyeOffset) )
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
    SithAISightState curSightState = pLocal->targetSightState2;
    SithThing* pOwner              = pLocal->pOwner;
    if ( (size_t)pLocal->goalSightFrame == sithMain_g_frameNumber )
    {
        return;
    }

    pLocal->goalSightFrame = sithMain_g_frameNumber;
    SithThing* pTarget = pLocal->goalThing;
    if ( pTarget )
    {
        if ( (pOwner->thingInfo.actorInfo.flags & SITH_AF_SEEINVISIBLE) == 0 )
        {
            if ( (pTarget->type == SITH_THING_ACTOR || pTarget->type == SITH_THING_PLAYER) && (pTarget->thingInfo.actorInfo.flags & SITH_AF_INVISIBLE) != 0 )
            {
                curSightState = SITHAI_SIGHT_HIDDEN;
            }

            if ( (pOwner->thingInfo.actorInfo.flags & SITH_AF_BLIND) != 0 )
            {
                curSightState = SITHAI_SIGHT_HIDDEN;
            }
        }

        pLocal->targetSightState2 = sithAIUtil_GetDistanceToTarget(pOwner, &pOwner->pos, pTarget, -1.0f, pLocal->pClass->sightDistance, 0.0f, &pLocal->toGoal, &pLocal->targetDistance);

        if ( pLocal->targetSightState2 == SITHAI_SIGHT_VISIBLE )
        {
            if ( curSightState != SITHAI_SIGHT_VISIBLE && !sithAIUtil_CanSeeTarget(pLocal, pTarget, pLocal->targetDistance) )
            {
                pLocal->targetSightState2 = SITHAI_SIGHT_HIDDEN;
                return;
            }

            pLocal->lastGoalSeenPos = pTarget->pos;
            pLocal->msecGoalSeen  = sithTime_g_msecGameTime;
        }
    }
    else
    {
        pLocal->goalPos       = pLocal->movePos;
        pLocal->targetSightState2 = sithAIUtil_GetDistanceToTargetPos(pOwner, &pOwner->pos, &pLocal->goalPos, -1.0f, pLocal->pClass->sightDistance, 0.0f, &pLocal->toGoal, &pLocal->targetDistance);

        if ( pLocal->targetSightState2 == SITHAI_SIGHT_VISIBLE )
        {
            pLocal->lastGoalSeenPos = pLocal->goalPos;
            pLocal->msecGoalSeen  = sithTime_g_msecGameTime;
        }
    }
}

SithAISightState J3DAPI sithAIUtil_GetDistanceToTarget(SithThing* pViewer, rdVector3* startPos, const SithThing* pTarget, float fov, float sightDistance, float collisionSize, rdVector3* toTarget, float* pDist)
{
    J3D_UNUSED(collisionSize);

    SITH_ASSERTREL(toTarget && pDist);
    SITH_ASSERTREL(pViewer && pTarget && startPos);

    rdVector_Sub3(toTarget, &pTarget->pos, startPos);

    *pDist = rdVector_Normalize3Acc(toTarget) - pTarget->collide.size;
    *pDist = J3DMAX(*pDist, 0.0f);

    if ( (pViewer->thingInfo.actorInfo.flags & SITH_AF_SEEINVISIBLE) == 0 )
    {
        if ( (pTarget->type == SITH_THING_ACTOR || pTarget->type == SITH_THING_PLAYER) && (pTarget->thingInfo.actorInfo.flags & SITH_AF_INVISIBLE) != 0 )
        {
            return SITHAI_SIGHT_HIDDEN;
        }

        if ( (pViewer->thingInfo.actorInfo.flags & SITH_AF_BLIND) != 0 )
        {
            return SITHAI_SIGHT_HIDDEN;
        }
    }

    if ( (pViewer->flags & SITH_TF_SUBMERGED) == 0 && (pTarget->flags & SITH_TF_SUBMERGED) != 0 && (pTarget->moveType != SITH_MT_PHYSICS || (pTarget->moveInfo.physics.flags & SITH_PF_ONWATERSURFACE) == 0) )
    {
        return SITHAI_SIGHT_HIDDEN;
    }

    if ( (pViewer->flags & SITH_TF_SUBMERGED) != 0 && (pTarget->flags & SITH_TF_SUBMERGED) == 0 )
    {
        return SITHAI_SIGHT_HIDDEN;
    }

    if ( *pDist - pTarget->collide.size > sightDistance )
    {
        return SITHAI_SIGHT_TOOFAR;
    }

    if ( fov > -1.0f )
    {
        float curdotL = rdVector_Dot3(toTarget, &pViewer->orient.lvec);
        float dotR = fabsf(rdVector_Dot3(toTarget, &pViewer->orient.rvec));

        if ( fov >= 0.0f && (curdotL < 0.0f || dotR > (1.0f - fov)) )
        {
            return SITHAI_SIGHT_OUTOFFOV;
        }

        if ( fov < 0.0f && curdotL < 0.0f && dotR < (fov + 1.0f) )
        {
            return SITHAI_SIGHT_OUTOFFOV;
        }
    }

    if ( (pViewer->thingInfo.actorInfo.flags & SITH_AF_SEEINVISIBLE) != 0 )
    {
        return SITHAI_SIGHT_VISIBLE;
    }

    SithSector* pSector = sithCollision_FindSectorInRadius(pViewer->pInSector, &pViewer->pos, startPos, 0.0f);

    if ( sithCollision_CheckLOS(pSector, startPos, &pTarget->pos, 0.0f) )
    {
        return SITHAI_SIGHT_VISIBLE;
    }

    return SITHAI_SIGHT_HIDDEN;
}

SithAISightState J3DAPI sithAIUtil_GetDistanceToTargetPos(SithThing* pViewer, rdVector3* startPos, rdVector3* endPos, float fov, float sightDistance, float collisionSize, rdVector3* toTarget, float* pDist)
{
    J3D_UNUSED(collisionSize);

    SITH_ASSERTREL(toTarget && pDist);
    SITH_ASSERTREL(pViewer && startPos);

    rdVector_Sub3(toTarget, endPos, startPos);

    *pDist = rdVector_Normalize3Acc(toTarget);

    if ( *pDist > sightDistance )
    {
        return SITHAI_SIGHT_TOOFAR;
    }

    if ( fov > -1.0f )
    {
        float curdotL = rdVector_Dot3(toTarget, &pViewer->orient.lvec);
        float dotR = fabsf(rdVector_Dot3(toTarget, &pViewer->orient.rvec));

        if ( fov >= 0.0f && (curdotL < 0.0f || dotR > (1.0f - fov)) )
        {
            return SITHAI_SIGHT_OUTOFFOV;
        }

        if ( fov < 0.0f && curdotL < 0.0f && dotR < (fov + 1.0f) )
        {
            return SITHAI_SIGHT_OUTOFFOV;
        }
    }

    if ( (pViewer->thingInfo.actorInfo.flags & SITH_AF_SEEINVISIBLE) != 0 )
    {
        return SITHAI_SIGHT_VISIBLE;
    }

    SithSector* pSector = sithCollision_FindSectorInRadius(pViewer->pInSector, &pViewer->pos, startPos, 0.0f);

    if ( sithCollision_CheckLOS(pSector, startPos, endPos, 0.0f) )
    {
        return SITHAI_SIGHT_VISIBLE;
    }

    return SITHAI_SIGHT_HIDDEN;
}

int J3DAPI sithAIUtil_AIFire(SithAIControlBlock* pLocal, float minDist, float maxDist, float fireDot, float aimError, int weaponNum, SithAIUtilFireFlags flags, int burstCount)
{
    SithPuppetSubMode curSubMode      = 0;
    SithFireProjectileFlags projFlags = 0;
    float lobFactor                   = 1.0f;
    bool bHasFired                    = false;
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

    rdVector3 targetDir = pLocal->toTarget;

    if ( (flags & SITHAIUTIL_FIRE_DELAYED) == 0 )
    {
        if ( pLocal->msecFireWaitTime > sithTime_g_msecGameTime )
        {
            goto postfire;
        }

        if ( pLocal->targetSightState != SITHAI_SIGHT_VISIBLE || !(pLocal->distance >= minDist && pLocal->distance <= maxDist) )
        {
            goto postfire;
        }

        if ( (pLocal->submode & SITHAI_SUBMODE_SKIPCHECKFIREFOV) == 0 && rdVector_Dot3(&pAttacker->orient.lvec, &targetDir) < fireDot )
        {
            goto postfire;
        }

        if ( (flags & SITHAIUTIL_FIRE_WEAPON1) != 0 )
        {
            curSubMode = SITHPUPPETSUBMODE_FIRE;
        }
        else if ( (flags & SITHAIUTIL_FIRE_WEAPON2) != 0 )
        {
            curSubMode = SITHPUPPETSUBMODE_FIRE2;
        }
        else if ( (flags & SITHAIUTIL_FIRE_WEAPON3) != 0 )
        {
            curSubMode = SITHPUPPETSUBMODE_FIRE3;
        }
        else if ( (flags & SITHAIUTIL_FIRE_WEAPON4) != 0 )
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
        pLocal->fireDot     = fireDot;
        pLocal->minFireDist = minDist;
        pLocal->maxFireDist = maxDist;
        bDelayFire          = (flags & SITHAIUTIL_FIRE_DELAYED) == 0 && (pAttacker->thingInfo.actorInfo.flags & SITH_AF_DELAYFIRE) != 0;
    }

    if ( bDelayFire )
    {
        pLocal->fireFlags     = flags | SITHAIUTIL_FIRE_DELAYED;
        pLocal->aimError      = aimError;
        pLocal->fireWeaponNum = weaponNum;
        sithPuppet_PlayMode(pAttacker, curSubMode, NULL);
        sithAI_EmitEvent(pLocal, SITHAI_EVENT_FIRE, pLocal->pTargetThing);

        bHasFired = true;
    }
    else
    {
        if ( (flags & SITHAIUTIL_FIRE_LEAD) != 0
            && pLocal->pTargetThing->moveType == SITH_MT_PHYSICS
            && !rdVector_IsZero3(&pLocal->pTargetThing->moveInfo.physics.velocity) )
        {
            rdVector3 predictVec;
            rdVector_Scale3(&predictVec, &targetDir, speed);
            rdVector_Add3Acc(&predictVec, &pLocal->pTargetThing->moveInfo.physics.velocity);
            rdVector_Normalize3Acc(&predictVec);

            if ( rdVector_Dot3(&predictVec, &targetDir) > 0.5f )
            {
                targetDir = predictVec;
            }
        }

        if ( (flags & SITHAIUTIL_FIRE_LOB) != 0 )
        {
            rdVector3 predictVec;
            rdVector_Scale3(&predictVec, &pLocal->toTarget, speed);

            float leadFlightTime = pLocal->distance / speed;
            predictVec.z += leadFlightTime * 0.5f * sithWorld_g_pCurrentWorld->gravity;
            lobFactor = rdVector_Normalize3(&targetDir, &predictVec) / speed;
            projFlags |= SITHFIREPROJECTILE_SCALE_VELOCITY;
        }

        float accuracy = pLocal->pClass->accurancy * sithGetHitAccuarancyScalar();

        if ( aimError != 0.0f && pLocal->distance != 0.0f && SITH_RANDF() > accuracy )
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

            pAttacker->thingInfo.actorInfo.weaponInfo.vecUnknown0 = targetDir;
            SithThing* pBullet = sithWeapon_WeaponFire(pAttacker, pShotTemplate, &targetDir, &pLocal->weaponFirePos, 0, curSubMode, lobFactor, projFlags, 0.0f);

            if ( pBullet )
            {
                sithCog_ThingSendMessageEx(pAttacker, pBullet, SITHCOG_MSG_FIRE, (int)pShotTemplate->thingInfo.weaponInfo.damage, pShotTemplate->thingInfo.weaponInfo.damageType, weaponNum, 0);

                if ( (flags & SITHAIUTIL_FIRE_DELAYED) == 0 )
                {
                    sithAI_EmitEvent(pLocal, SITHAI_EVENT_FIRE, pLocal->pTargetThing);
                }

                bHasFired = true;
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

                    if ( (flags & SITHAIUTIL_FIRE_DELAYED) == 0 )
                    {
                        sithAI_EmitEvent(pLocal, SITHAI_EVENT_FIRE, pLocal->pTargetThing);
                    }

                    bHasFired = true;
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
    for ( size_t i = 0; i < STD_ARRAYLEN(sithAIUtil_aAIWpnts); ++i )
    {
        sithAIUtil_aAIWpnts[i].thingNum     = -1;
        sithAIUtil_aAIWpnts[i].flags        = SITH_AIWPNT_LAYER0;
        sithAIUtil_aAIWpnts[i].numUsedLinks = 0;
        sithAIUtil_aAIWpnts[i].rank         = 0;
    }

    for ( size_t i = 0; i < STD_ARRAYLEN(sithAIUtil_aWpntOwners); ++i )
    {
        sithAIUtil_aWpntOwners[i].thingNum          = -1;
        sithAIUtil_aWpntOwners[i].waypointNum       = 0;
        sithAIUtil_aWpntOwners[i].cosTurnAlignAngle = 0.0f;
    }

    sithAIUtil_activeWpntLayer = SITH_AIWPNT_LAYER0;
    sithAIUtil_g_bRenderAIWpnts = 0;
}

int J3DAPI sithAIUtil_SyncWpnts(DPID idTo, unsigned int outstream)
{
    SITHDSS_STARTOUT(SITHDSS_SYNCWPNT);

    for ( size_t wpntNum = 0; wpntNum < STD_ARRAYLEN(sithAIUtil_aAIWpnts); ++wpntNum )
    {
        SITHDSS_PUSHINT32(sithAIUtil_aAIWpnts[wpntNum].thingNum);
        SITHDSS_PUSHINT32(sithAIUtil_aAIWpnts[wpntNum].flags);
        SITHDSS_PUSHINT32(sithAIUtil_aAIWpnts[wpntNum].numUsedLinks);

        for ( size_t j = 0; j < STD_ARRAYLEN(sithAIUtil_aAIWpnts[wpntNum].aWpntLinks); ++j )
        {
            SITHDSS_PUSHINT32(sithAIUtil_aAIWpnts[wpntNum].aWpntLinks[j]);
        }

        SITHDSS_PUSHINT32(sithAIUtil_aAIWpnts[wpntNum].rank);
    }

    for ( size_t wpntNum = 0; wpntNum < STD_ARRAYLEN(sithAIUtil_aWpntOwners); ++wpntNum )
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

    for ( size_t idx = 0; idx < STD_ARRAYLEN(sithAIUtil_aAIWpnts); ++idx )
    {
        sithAIUtil_aAIWpnts[idx].thingNum     = SITHDSS_POPINT32();
        sithAIUtil_aAIWpnts[idx].flags        = SITHDSS_POPINT32();
        sithAIUtil_aAIWpnts[idx].numUsedLinks = SITHDSS_POPINT32();

        for ( size_t j = 0; j < STD_ARRAYLEN(sithAIUtil_aAIWpnts[idx].aWpntLinks); ++j )
        {
            sithAIUtil_aAIWpnts[idx].aWpntLinks[j] = SITHDSS_POPINT32();
        }

        sithAIUtil_aAIWpnts[idx].rank = SITHDSS_POPINT32();
    }

    for ( size_t idx = 0; idx < STD_ARRAYLEN(sithAIUtil_aWpntOwners); ++idx )
    {
        sithAIUtil_aWpntOwners[idx].thingNum          = SITHDSS_POPINT32();
        sithAIUtil_aWpntOwners[idx].waypointNum       = SITHDSS_POPINT32();
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

    sithAIUtil_aAIWpnts[wpntIdx].thingNum     = pGhost->idx;
    sithAIUtil_aAIWpnts[wpntIdx].flags        = SITH_AIWPNT_LAYER0;
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

    rank = STDMATH_CLAMP(rank, 0, SITHAI_WPNT_MAXRANK);

    sithAIUtil_aAIWpnts[wpntIdx].flags &= ~SITHAI_WPNT_RANKMASK;
    sithAIUtil_aAIWpnts[wpntIdx].flags |= rank;
}

void J3DAPI sithAIUtil_AISetWpntFlags(unsigned int wpntIdx, SithAIWaypointLayerFlag flags)
{
    if ( wpntIdx >= STD_ARRAYLEN(sithAIUtil_aAIWpnts) )
    {
        SITHLOG_ERROR("AISetWpnt: Index (%d) out of allowed range (0 - %d).\n", wpntIdx, STD_ARRAYLEN(sithAIUtil_aAIWpnts));
        return;
    }

    unsigned int uFlags = flags;
    uFlags = STDMATH_CLAMP(uFlags, SITH_AIWPNT_DISABLED, SITHAI_WPNT_FLAGSMASK);

    sithAIUtil_aAIWpnts[wpntIdx].flags |= uFlags;
}

void J3DAPI sithAIUtil_AIClearWpntFlags(unsigned int wpntIdx, SithAIWaypointLayerFlag flags)
{
    if ( wpntIdx >= STD_ARRAYLEN(sithAIUtil_aAIWpnts) )
    {
        SITHLOG_ERROR("AISetWpnt: Index (%d) out of allowed range (0 - %d).\n", wpntIdx, STD_ARRAYLEN(sithAIUtil_aAIWpnts));
        return;
    }

    unsigned int uFlags = flags;
    uFlags = STDMATH_CLAMP(uFlags, SITH_AIWPNT_DISABLED, SITHAI_WPNT_FLAGSMASK);

    sithAIUtil_aAIWpnts[wpntIdx].flags &= ~uFlags;
}

void J3DAPI sithAIUtil_AISetActiveWpntLayer(SithAIWaypointLayerFlag layer)
{
    if ( (layer & SITH_AIWPNT_ALLLAYERS) == 0 )
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
        if ( (pLocal->submode & SITHAI_SUBMODE_MOVEFORWARD) != 0 )
        {
            degTurn = 180.0f;
        }

        sithAIUtil_AITraverseWpnts(pLocal, wpntIdx, moveSpeed, degTurn, mode);
        return 1;
    }

    return 0;
}

void J3DAPI sithAIUtil_AITraverseWpnts(SithAIControlBlock* pLocal, int wpntIdx, float moveSpeed, float degTurn, int mode)
{
    SithWorld* pCurWorld = sithWorld_g_pCurrentWorld;

    SITH_ASSERTREL(pLocal);

    if ( !pCurWorld || (unsigned int)wpntIdx >= STD_ARRAYLEN(sithAIUtil_aAIWpnts) )
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
    else if ( (unsigned int)mode > 1 )
    {
        pLocal->submode |= SITHAI_SUBMODE_CONTINUOUSWPNTMOTION;
    }

    sithAIUtil_aWpntOwners[ownerNum].waypointNum = SITHAI_WPNTOWNER_PACKNEXT(sithAIUtil_aWpntOwners[ownerNum].waypointNum, wpntIdx);

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
        int wpntIdx = SITHAI_WPNTOWNER_GETNEXT(sithAIUtil_aWpntOwners[ownerIdx].waypointNum);
        sithCog_ThingSendMessageEx(pLocal->pOwner, NULL, SITHCOG_MSG_UPDATEWPNTS, 0, 0, 0, 0);

        if ( (size_t)wpntIdx < STD_ARRAYLEN(sithAIUtil_aAIWpnts) )
        {
            if ( sithAIUtil_aAIWpnts[wpntIdx].numUsedLinks > 0 )
            {
                int* aLinks      = sithAIUtil_aAIWpnts[wpntIdx].aWpntLinks;
                bool bLinkUsable = false;
                int nextWpntNum;
                int i;
                for ( i = 0; i < (int)sithAIUtil_aAIWpnts[wpntIdx].numUsedLinks; ++i )
                {
                    nextWpntNum = aLinks[i];
                    if ( (sithAIUtil_aAIWpnts[nextWpntNum].flags & sithAIUtil_activeWpntLayer) != 0
                        && (sithAIUtil_aAIWpnts[nextWpntNum].flags & SITH_AIWPNT_DISABLED) == 0 )
                    {
                        bLinkUsable = true;
                        break;
                    }
                }

                bool bSkipSingleLink = sithAIUtil_aAIWpnts[wpntIdx].numUsedLinks == 1
                    && (unsigned int)aLinks[0] == SITHAI_WPNTOWNER_GETPREV(sithAIUtil_aWpntOwners[ownerIdx].waypointNum)
                    && (pLocal->submode & (SITHAI_SUBMODE_CONTINUOUSWPNTMOTION | SITHAI_SUBMODE_SEMICONTINUOUSWPNTMOTION)) == 0;
                if ( bLinkUsable && !bSkipSingleLink )
                {
                    nextWpntNum = sithAIUtil_SelectWpntLink(pLocal, sithAIUtil_aAIWpnts[wpntIdx].numUsedLinks, aLinks, i, ownerIdx);
                    int thingNum = sithAIUtil_aAIWpnts[nextWpntNum].thingNum;
                    if ( sithAIMove_AISetMovePos(pLocal, &pCurLevel->aThings[thingNum].pos, pLocal->moveSpeed) )
                    {
                        sithAIMove_AISetLookPosEyeLevel(pLocal, &pCurLevel->aThings[thingNum].pos);
                        sithAIUtil_CheckWpntMoveAlign(pLocal);

                        sithAIUtil_aWpntOwners[ownerIdx].waypointNum = SITHAI_WPNTOWNER_PACKPREV(sithAIUtil_aWpntOwners[ownerIdx].waypointNum, wpntIdx);
                        sithAIUtil_aWpntOwners[ownerIdx].waypointNum = SITHAI_WPNTOWNER_PACKNEXT(sithAIUtil_aWpntOwners[ownerIdx].waypointNum, nextWpntNum);
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
        if ( (sithAIUtil_aAIWpnts[nextWpnt].flags & SITH_AIWPNT_DISABLED) != 0
            || (sithAIUtil_aAIWpnts[nextWpnt].flags & sithAIUtil_activeWpntLayer) == 0 )
        {
            continue;
        }

        int bestPriority = sithAIUtil_aAIWpnts[leadWpnt].flags & SITHAI_WPNT_RANKMASK;
        int curRank      = sithAIUtil_aAIWpnts[nextWpnt].flags & SITHAI_WPNT_RANKMASK;

        // Waypoint the AI came from
        int prevWpnt = -1;
        if ( wpntOwnerNum != -1 && SITHAI_WPNTOWNER_GETPREV(sithAIUtil_aWpntOwners[wpntOwnerNum].waypointNum) < STD_ARRAYLEN(sithAIUtil_aAIWpnts) )
        {
            prevWpnt = SITHAI_WPNTOWNER_GETPREV(sithAIUtil_aWpntOwners[wpntOwnerNum].waypointNum);
        }

        if ( (pLocal->mode & SITHAI_MODE_FLEEING) != 0
            && pLocal->pFleeFromThing
            && (pLocal->mode & SITHAI_MODE_FLEEINGTOWAYPOINT) == 0 )
        {
            if ( sithAIUtil_aAIWpnts[leadWpnt].rank == SITHAI_WPNT_MAXRANK )
            {
                bestPriority -= SITHAIUTIL_WPNTSCORE_FLEEMAXRANK;
            }
            else if ( sithAIUtil_aAIWpnts[nextWpnt].rank == SITHAI_WPNT_MAXRANK )
            {
                curRank -= SITHAIUTIL_WPNTSCORE_FLEEMAXRANK;
            }

            rdVector3 targetDir;
            rdVector3 wpntDir;
            rdVector_Sub3(&targetDir, &pLocal->pFleeFromThing->pos, &pLocal->pOwner->pos);

            float playerDistance = rdVector_Normalize3Acc(&targetDir);
            float slopeStep      = (sithAIUtil_fleeFacingMaxDot - sithAIUtil_fleeFacingMinDot) / (sithAIUtil_fleeFacingMaxDist - sithAIUtil_fleeFacingMinDist);
            float threshold      = (playerDistance - sithAIUtil_fleeFacingMinDist) * slopeStep + sithAIUtil_fleeFacingMinDot;
            threshold = STDMATH_CLAMP(threshold, sithAIUtil_fleeFacingMinDot, sithAIUtil_fleeFacingMaxDot);

            int idxThing = sithAIUtil_aAIWpnts[leadWpnt].thingNum;
            rdVector_Sub3(&wpntDir, &pWorld->aThings[idxThing].pos, &pLocal->pOwner->pos);
            rdVector_Normalize3Acc(&wpntDir);
            float fleeFacing = rdVector_Dot3(&targetDir, &wpntDir);

            if ( fleeFacing > threshold )
            {
                bestPriority -= SITHAIUTIL_WPNTSCORE_FLEEFACING;
            }

            idxThing = sithAIUtil_aAIWpnts[nextWpnt].thingNum;
            rdVector_Sub3(&wpntDir, &pWorld->aThings[idxThing].pos, &pLocal->pOwner->pos);
            rdVector_Normalize3Acc(&wpntDir);
            fleeFacing = rdVector_Dot3(&targetDir, &wpntDir);

            if ( fleeFacing > threshold )
            {
                curRank -= SITHAIUTIL_WPNTSCORE_FLEEFACING;
            }

            int rankLead = sithAIUtil_aAIWpnts[nextWpnt].rank - sithAIUtil_aAIWpnts[leadWpnt].rank;
            if ( rankLead < 0 )
            {
                curRank += -rankLead * SITHAIUTIL_WPNTSCORE_FLEERANKSTEP;
            }
            else if ( rankLead > 0 )
            {
                bestPriority += rankLead * SITHAIUTIL_WPNTSCORE_FLEERANKSTEP;
            }

            if ( nextWpnt == prevWpnt )
            {
                curRank -= SITHAIUTIL_WPNTSCORE_FLEEBACKTRACK;
            }

            if ( leadWpnt == prevWpnt )
            {
                bestPriority -= SITHAIUTIL_WPNTSCORE_FLEEBACKTRACK;
            }
        }
        else if ( (pLocal->mode & SITHAI_MODE_FLEEING) != 0
            && pLocal->pFleeFromThing
            && (pLocal->mode & SITHAI_MODE_FLEEINGTOWAYPOINT) != 0 )
        {
            bestPriority = 0;
            curRank      = 0;

            if ( nextWpnt == prevWpnt )
            {
                curRank -= SITHAIUTIL_WPNTSCORE_BACKTRACK;
            }

            if ( leadWpnt == prevWpnt )
            {
                bestPriority -= SITHAIUTIL_WPNTSCORE_BACKTRACK;
            }

            if ( sithAIUtil_aAIWpnts[nextWpnt].rank > sithAIUtil_aAIWpnts[leadWpnt].rank )
            {
                curRank += SITHAIUTIL_WPNTSCORE_RANKBONUS;
            }
            else if ( sithAIUtil_aAIWpnts[nextWpnt].rank < sithAIUtil_aAIWpnts[leadWpnt].rank )
            {
                bestPriority += SITHAIUTIL_WPNTSCORE_RANKBONUS;
            }
        }
        else if ( (pLocal->mode & SITHAI_MODE_HUNTING) != 0 && pLocal->pTargetThing )
        {
            bestPriority = 0;
            curRank      = 0;

            if ( nextWpnt == prevWpnt )
            {
                curRank -= SITHAIUTIL_WPNTSCORE_BACKTRACK;
            }

            if ( leadWpnt == prevWpnt )
            {
                bestPriority -= SITHAIUTIL_WPNTSCORE_BACKTRACK;
            }

            if ( sithAIUtil_aAIWpnts[nextWpnt].rank > sithAIUtil_aAIWpnts[leadWpnt].rank )
            {
                curRank += SITHAIUTIL_WPNTSCORE_RANKBONUS;
            }
            else if ( sithAIUtil_aAIWpnts[nextWpnt].rank < sithAIUtil_aAIWpnts[leadWpnt].rank )
            {
                bestPriority += SITHAIUTIL_WPNTSCORE_RANKBONUS;
            }
        }
        else if ( (pLocal->mode & (SITHAI_MODE_ATTACKING | SITHAI_MODE_SEARCHING)) != 0 )
        {
            if ( nextWpnt == prevWpnt )
            {
                curRank -= SITHAIUTIL_WPNTSCORE_BACKTRACK;
            }

            if ( leadWpnt == prevWpnt )
            {
                bestPriority -= SITHAIUTIL_WPNTSCORE_BACKTRACK;
            }

            if ( curRank < bestPriority && SITH_RANDF() > (bestPriority - curRank) * 0.1f + 0.5f )
            {
                bestPriority = -bestPriority;
            }
        }

        if ( curRank > bestPriority || (curRank == bestPriority && SITH_RANDF() > 0.5f) )
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
        rdVector3 dir;
        rdMath_ProjectPointOntoPlaneNormalized(&dir, &pLocal->goalLVec, &pLocal->pOwner->orient.uvec, &rdroid_g_zeroVector3);

        float dot;
        if ( (pLocal->mode & SITHAI_MODE_WALLCRAWLING) != 0 )
        {
            dot = rdVector_Dot3(&pLocal->pOwner->orient.lvec, &dir);
        }
        else
        {
            dot = pLocal->pOwner->orient.lvec.x * dir.x + pLocal->pOwner->orient.lvec.y * dir.y;
        }

        if ( dot >= sithAIUtil_aWpntOwners[wpntNumber].cosTurnAlignAngle || (pLocal->mode & SITHAI_MODE_TURNING) == 0 )
        {
            pLocal->submode &= ~SITHAI_SUBMODE_ALIGNING;
        }
        else
        {
            rdVector_Zero3(&pLocal->pOwner->moveInfo.physics.velocity);
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
    bool bFound = false;
    if ( pThing != sithPlayer_g_pLocalPlayerThing )
    {
        sithCog_ThingSendMessageEx(pThing, NULL, SITHCOG_MSG_UPDATEWPNTS, 0, 0, 0, 0);
    }

    size_t numDist = 0;
    size_t i;
    for ( i = 0; i < STD_ARRAYLEN(sithAIUtil_aAIWpnts); ++i )
    {
        if ( (sithAIUtil_aAIWpnts[i].flags & SITH_AIWPNT_DISABLED) != 0
            || (sithAIUtil_aAIWpnts[i].flags & sithAIUtil_activeWpntLayer) == 0 )
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
            SITHLOG_ERROR("AIFindNearestWpnt: Invalid waypoint (#%zu).\n", i);
            continue;
        }

        bFound = true;

        rdVector3 toWpnt;
        rdVector_Sub3(&toWpnt, &pThing->pos, &pWorld->aThings[idxThing].pos);

        sithAIUtil_aWpntDistances[numDist].num      = (int)i;
        sithAIUtil_aWpntDistances[numDist].distance = rdVector_Dot3(&toWpnt, &toWpnt);

        if ( sithAIUtil_aWpntDistances[numDist].distance <= (pThing->collide.movesize + 0.03f) * (pThing->collide.movesize + 0.03f) )
        {
            return (int)i;
        }

        ++numDist;
    }

    if ( !bFound )
    {
        return -1;
    }

    // Step 1: Sort the waypoints by their squared distance to the thing, nearest first
    qsort(sithAIUtil_aWpntDistances, numDist, sizeof(SithAIWaypointDistance), sithAIUtil_CompareAIWpntDistances);
    idWpnt = sithAIUtil_aWpntDistances[0].num;
    float pathDist;
    rdVector3 hitNorm;
    if ( bCheckPath )
    {
        bFound = false;
        for ( i = 0; i < numDist; ++i )
        {
            idWpnt = sithAIUtil_aWpntDistances[i].num;
            int idxThing = sithAIUtil_aAIWpnts[idWpnt].thingNum;
            if ( !sithAIUtil_CheckPathToPoint(pThing, &pWorld->aThings[idxThing].pos, pThing->collide.movesize, &pathDist, &hitNorm, 1, 0) )
            {
                bFound = true;
                break;
            }
        }

        if ( !bFound )
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

    for ( size_t idx = 0; idx < STD_ARRAYLEN(sithAIUtil_aAIWpnts); ++idx )
    {
        sithAIUtil_aAIWpnts[idx].rank = 0;
    }

    int wpntNum = sithAIUtil_AIFindNearestWpnt(sithPlayer_g_pLocalPlayerThing, 0);

    if ( wpntNum == -1 )
    {
        return;
    }

    unsigned int rankNum = SITHAI_WPNT_MAXRANK;
    int aStack[SITHAI_MAXWPNTS];
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
            if ( (sithAIUtil_aAIWpnts[linkCount].flags & SITH_AIWPNT_DISABLED) != 0
                || (sithAIUtil_aAIWpnts[linkCount].flags & sithAIUtil_activeWpntLayer) == 0 )
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
    for ( size_t idx = 0; idx < STD_ARRAYLEN(sithAIUtil_aAIWpnts); ++idx )
    {
        sithAIUtil_aAIWpnts[idx].rank = 0;
    }

    int wpntNum = 0;
    int bFound  = 0;
    for ( size_t idx = 0; idx < STD_ARRAYLEN(sithAIUtil_aAIWpnts); ++idx )
    {
        if ( sithAIUtil_aAIWpnts[idx].thingNum == pThing->idx )
        {
            bFound  = 1;
            wpntNum = idx;
            break;
        }
    }

    if ( !bFound )
    {
        return;
    }

    unsigned int rankNum = SITHAI_WPNT_MAXRANK;
    int aStack[SITHAI_MAXWPNTS];
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
            if ( (sithAIUtil_aAIWpnts[linkCount].flags & SITH_AIWPNT_DISABLED) != 0
                || (sithAIUtil_aAIWpnts[linkCount].flags & sithAIUtil_activeWpntLayer) == 0 )
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
    for ( size_t ownerNum = 0; ownerNum < STD_ARRAYLEN(sithAIUtil_aWpntOwners); ++ownerNum )
    {
        if ( sithAIUtil_aWpntOwners[ownerNum].thingNum == -1 && wpntNum == -1 )
        {
            wpntNum = (int)ownerNum;
        }

        if ( sithAIUtil_aWpntOwners[ownerNum].thingNum == pLocal->pOwner->idx )
        {
            wpntNum = (int)ownerNum;
            break;
        }
    }

    if ( wpntNum > -1 )
    {
        degTurn = STDMATH_CLAMP(degTurn, 5.0f, 180.0f);
        float sinAngle;
        float cosAngle;
        stdMath_SinCos(degTurn, &sinAngle, &cosAngle);

        sithAIUtil_aWpntOwners[wpntNum].thingNum          = pLocal->pOwner->idx;
        sithAIUtil_aWpntOwners[wpntNum].cosTurnAlignAngle = cosAngle;
    }

    return wpntNum;
}

void J3DAPI sithAIUtil_ClearAIWaypoint(SithAIControlBlock* pLocal)
{
    int wpntNum = sithAIUtil_GetAIWaypointNum(pLocal);

    if ( wpntNum > -1 )
    {
        sithAIUtil_aWpntOwners[wpntNum].thingNum          = -1;
        sithAIUtil_aWpntOwners[wpntNum].waypointNum       = 0;
        sithAIUtil_aWpntOwners[wpntNum].cosTurnAlignAngle = 0.0f;
    }
}

int J3DAPI sithAIUtil_GetAIWaypointNum(SithAIControlBlock* pLocal)
{
    for ( size_t i = 0; i < STD_ARRAYLEN(sithAIUtil_aWpntOwners); ++i )
    {
        if ( sithAIUtil_aWpntOwners[i].thingNum == pLocal->pOwner->idx )
        {
            return (int)i;
        }
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

    if ( sithTime_g_secGameTime - sithAIUtil_secLastUpdate < sithAIUtil_wpntRenderInterval )
    {
        return;
    }

    for ( size_t wpntIdx = 0; wpntIdx < STD_ARRAYLEN(sithAIUtil_aAIWpnts); ++wpntIdx )
    {
        int thingNum = sithAIUtil_aAIWpnts[wpntIdx].thingNum;
        if ( (sithAIUtil_aAIWpnts[wpntIdx].flags & SITH_AIWPNT_DISABLED) != 0
            || (sithAIUtil_aAIWpnts[wpntIdx].flags & sithAIUtil_activeWpntLayer) == 0 )
        {
            continue;
        }

        for ( unsigned int j = 0; j < sithAIUtil_aAIWpnts[wpntIdx].numUsedLinks; ++j )
        {
            int toWpntIdx = sithAIUtil_aAIWpnts[wpntIdx].aWpntLinks[j];
            if ( (size_t)toWpntIdx < wpntIdx )
            {
                continue;
            }

            if ( (sithAIUtil_aAIWpnts[toWpntIdx].flags & SITH_AIWPNT_DISABLED) != 0
                || (sithAIUtil_aAIWpnts[toWpntIdx].flags & sithAIUtil_activeWpntLayer) == 0 )
            {
                continue;
            }

            int linkGhostIdx = sithAIUtil_aAIWpnts[toWpntIdx].thingNum;
            rdVector3 offset = { 0.0f, 0.0f, 0.1f };
            rdVector3 laserEnd = pWorld->aThings[linkGhostIdx].pos;
            laserEnd.z += 0.1f;
            sithFX_CreateLaserThing(&pWorld->aThings[thingNum], &offset, &laserEnd, 0.05f, 0.05f, sithAIUtil_wpntRenderInterval + 0.1f);

            sithAIUtil_secLastUpdate = sithTime_g_secGameTime;
        }
    }
}

int J3DAPI sithAIUtil_CheckPathToPoint(SithThing* pViewer, const rdVector3* pTarget, float radius, float* pDistance, rdVector3* pHitNorm, int bDetectThings, int bSkipFloor)
{
    SITH_ASSERTREL(pViewer && pTarget);

    bool bBlocked = false;

    rdVector3 curDir;
    rdVector_Sub3(&curDir, pTarget, &pViewer->pos);

    float distance = rdVector_Normalize3Acc(&curDir);
    *pDistance     = distance;

    int searchFlags = 0x102;
    if ( !bDetectThings )
    {
        searchFlags |= 0x1;
    }

    sithCollision_SearchForCollisions(pViewer->pInSector, pViewer, &pViewer->pos, &curDir, distance, radius, searchFlags);

    SithCollision* pCurCollision;
    while ( (pCurCollision = sithCollision_PopStack()) != NULL )
    {
        if ( !bSkipFloor
            && (pCurCollision->type & SITHCOLLISION_WORLD) != 0
            && (pCurCollision->pSurfaceCollided->flags & SITH_SURFACE_ISFLOOR) != 0 )
        {
            continue;
        }

        if ( (pCurCollision->type & (SITHCOLLISION_ADJOINTOUCH | SITHCOLLISION_ADJOINCROSS)) != 0
            && (pCurCollision->pSurfaceCollided->pAdjoin->flags & SITH_ADJOIN_MOVE) != 0 )
        {
            continue;
        }

        bBlocked   = true;
        *pDistance = pCurCollision->distance;
        *pHitNorm  = pCurCollision->hitNorm;
        break;
    }

    sithCollision_DecreaseStackLevel();
    return bBlocked;
}

SithAIUtilFloorCheck J3DAPI sithAIUtil_CheckPosition(const SithAIControlBlock* pLocal, rdVector3* endPos, int* pbNoCollision)
{
    SithThing* pThing = pLocal->pOwner;
    SITH_ASSERTREL(pThing);

    SithSector* pSector = sithCollision_FindSectorInRadius(pThing->pInSector, &pThing->pos, endPos, 0.0f);

    if ( !pSector )
    {
        return SITHAIUTIL_FLOOR_BLOCKED;
    }

    if ( (pSector->flags & SITH_SECTOR_NOACTORENTER) != 0 )
    {
        return SITHAIUTIL_FLOOR_BLOCKED;
    }

    if ( (pThing->pInSector->flags & SITH_SECTOR_UNDERWATER) == 0
        && (pSector->flags & SITH_SECTOR_UNDERWATER) != 0
        && (pThing->flags & SITH_TF_WATERDESTROYED) != 0 )
    {
        return SITHAIUTIL_FLOOR_BLOCKED;
    }

    return sithAIUtil_CheckFloorAtPos(pLocal, endPos, pSector, pbNoCollision);
}

SithAIUtilFloorCheck J3DAPI sithAIUtil_CheckFloorAtPos(const SithAIControlBlock* pLocal, const rdVector3* startPos, SithSector* pStartSector, int* pbNoCollision)
{
    SithAIUtilFloorCheck result = SITHAIUTIL_FLOOR_NONE;
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
        downDir = rdroid_g_zVector3;
    }

    sithCollision_SearchForCollisions(pStartSector, pThing, startPos, &downDir, sithPhysics_GetThingHeight(pThing) + pLocal->pClass->maxStep, 0.001f, 0);

    SithCollision* pCollision;
    while ( (pCollision = sithCollision_PopStack()) != NULL )
    {
        if ( (pCollision->type & (SITHCOLLISION_ADJOINTOUCH | SITHCOLLISION_ADJOINCROSS)) != 0 )
        {
            if ( (pCollision->pSurfaceCollided->pAdjoin->pAdjoinSector->flags & SITH_SECTOR_NOACTORENTER) != 0 )
            {
                result = SITHAIUTIL_FLOOR_BLOCKED;
                break;
            }

            if ( (pThing->pInSector->flags & SITH_SECTOR_UNDERWATER) == 0
                && (pCollision->pSurfaceCollided->pAdjoin->pAdjoinSector->flags & SITH_SECTOR_UNDERWATER) != 0
                && (pThing->flags & SITH_TF_WATERDESTROYED) != 0 )
            {
                result = SITHAIUTIL_FLOOR_BLOCKED;
                break;
            }
        }

        if ( (pCollision->type & SITHCOLLISION_WORLD) != 0 )
        {
            SITH_ASSERTREL(pCollision->pSurfaceCollided);
            if ( (pCollision->pSurfaceCollided->flags & SITH_SURFACE_NOAIMOVE) != 0 )
            {
                result = SITHAIUTIL_FLOOR_BLOCKED;
                break;
            }

            if ( pLocal->allowedSurfaceTypes )
            {
                if ( (pCollision->pSurfaceCollided->flags & pLocal->allowedSurfaceTypes) == 0 )
                {
                    result = SITHAIUTIL_FLOOR_BLOCKED;
                    break;
                }
            }
            else if ( (pCollision->pSurfaceCollided->flags & SITH_SURFACE_LAVA) != 0 )
            {
                result = SITHAIUTIL_FLOOR_BLOCKED;
                break;
            }

            if ( (pCollision->pSurfaceCollided->flags & SITH_SURFACE_ISFLOOR) != 0 )
            {
                result = SITHAIUTIL_FLOOR_OK;
                if ( (pThing->moveInfo.physics.flags & SITH_PF_ALIGNUP) != 0 && (pThing->attach.flags & SITH_ATTACH_SURFACE) != 0 )
                {
                    SithSurface* pAttachedSurface = pThing->attach.attachedToStructure.pSurfaceAttached;
                    SithSurface* pSurface         = pCollision->pSurfaceCollided;

                    if ( pAttachedSurface != pSurface )
                    {
                        float dot = rdVector_Dot3(&pSurface->face.normal, &rdroid_g_zVector3);

                        if ( dot < 0.788f )
                        {
                            result = SITHAIUTIL_FLOOR_WALL;
                        }
                    }
                }
            }
            else
            {
                result = SITHAIUTIL_FLOOR_WALL;
            }

            if ( pbNoCollision )
            {
                *pbNoCollision = (pThing->attach.flags & SITH_ATTACH_SURFACE) == 0
                    || pThing->attach.attachedToStructure.pSurfaceAttached != pCollision->pSurfaceCollided;
            }

            break;
        }

        if ( (pCollision->type & SITHCOLLISION_THING) != 0 )
        {
            SITH_ASSERTREL(pCollision->pThingCollided);
            if ( (pCollision->pThingCollided->attach.flags & SITH_ATTACH_THING) != 0 && pCollision->pThingCollided->attach.attachedToStructure.pThingAttached == pThing )
            {
                result = SITHAIUTIL_FLOOR_OK;
            }
            else if ( pThing->pAttachedThing && (pThing->pAttachedThing->attach.flags & SITH_ATTACH_TAIL) != 0 )
            {
                result = SITHAIUTIL_FLOOR_OK;
            }
            // TODO: [BUG] SITH_THING_PLAYER is 10 (0xA), so the original test (type & SITH_THING_PLAYER) == 0 treats the thing type as a bit mask:
            //       every type having bit 2 or 8 set (not only players) is skipped. The intended check is probably that the thing is not invisible,
            //       i.e. (pThingCollided->flags & SITH_TF_INVISIBLE) == 0. Kept as in the original.
            else if ( (pCollision->pThingCollided->flags & SITH_TF_STANDON) == 0
                && (pCollision->pThingCollided->type & SITH_THING_PLAYER) == 0 )
            {
                result = SITHAIUTIL_FLOOR_THING;
                break;
            }

            result = SITHAIUTIL_FLOOR_OK;
            if ( pbNoCollision )
            {
                *pbNoCollision = (pThing->attach.flags & SITH_ATTACH_THINGFACE) == 0
                    || pThing->attach.attachedToStructure.pThingAttached != pCollision->pThingCollided;
            }

            break;
        }
    }

    sithCollision_DecreaseStackLevel();
    return result;
}

int J3DAPI sithAIUtil_MakeRandPoint(SithAIControlBlock* pLocal, rdVector3* pPos, SithAIUtilMkPointFlags flags, float angle, float* pDownCos, rdVector3* pDir, rdVector3* pDestPoint)
{
    rdVector3 newDir = { 0 };
    rdVector3 rotPyr = { 0 };
    rdVector3 toMovePos;

    bool bValid = false;
    SITH_ASSERTREL(pLocal && pLocal->pClass && pLocal->pOwner);
    SithThing* pActor = pLocal->pOwner;

    if ( !pPos || !pDir || !pDestPoint )
    {
        SITHLOG_ERROR("sithAIUtil_MakeRandPoint: Bad parameters.\n");
        return 0;
    }

    sithAIUtil_mkPointCurFlags  = 0;
    sithAIUtil_pMkPointCurLocal = NULL;

    if ( (flags & SITHAIUTIL_MKPOINT_PITCH) == 0 )
    {
        pDir->z = 0.0f;
        rdVector_Normalize3Acc(pDir);
    }

    if ( (flags & SITHAIUTIL_MKPOINT_ROTATE) != 0 )
    {
        float pitch = (flags & SITHAIUTIL_MKPOINT_PITCH) != 0 ? angle : 0.0f;
        float yaw   = (flags & SITHAIUTIL_MKPOINT_YAW) != 0 ? angle : 0.0f;

        if ( (flags & SITHAIUTIL_MKPOINT_RANDANGLE) != 0 )
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
        if ( (flags & SITHAIUTIL_MKPOINT_RANDSIDE) != 0 && SITH_RANDF() > 0.5f )
        {
            rdVector_Neg3Acc(&rotPyr);
        }

        rdVector_Rotate3(&newDir, pDir, &rotPyr);

        bValid = true;
    }
    else if ( (flags & SITHAIUTIL_MKPOINT_SIDEWAYS) != 0 )
    {
        rdVector_Cross3(&newDir, &pActor->orient.uvec, pDir);

        if ( (flags & SITHAIUTIL_MKPOINT_ALONGMOVEDIR) != 0 )
        {
            if ( rdVector_Dot3(&pLocal->moveDirection, &newDir) < 0.0f )
            {
                rdVector_Neg3Acc(&newDir);
            }
        }
        else
        {
            if ( (flags & SITHAIUTIL_MKPOINT_RANDSIDE) != 0 && SITH_RANDF() > 0.5f )
            {
                rdVector_Neg3Acc(&newDir);
            }

            if ( (flags & SITHAIUTIL_MKPOINT_RANDANGLE) != 0 )
            {
                rotPyr.x = 0.0f;
                rotPyr.y = SITH_RANDF() * angle;
                rotPyr.z = 0.0f;
                if ( SITH_RANDF() > 0.5f )
                {
                    rdVector_Neg3Acc(&rotPyr);
                }

                rdVector_Rotate3Acc(&newDir, &rotPyr);
            }
        }

        bValid = true;
    }

    if ( bValid )
    {
        if ( (flags & SITHAIUTIL_MKPOINT_ANYREF) != 0 )
        {
            // TODO: [BUG] The original leaves movePos uninitialized when sithAIUtil_AIGetMovePos finds no reference position (flee-from thing is missing)
            rdVector3 movePos;
            sithAIUtil_AIGetMovePos(pLocal, flags, &movePos);
            rdVector_Sub3(&toMovePos, &movePos, &pActor->pos);
            rdVector_Normalize3Acc(&toMovePos);

            bool bInFront = rdVector_Dot3(&newDir, &toMovePos) > 0.0f;
            if ( ((flags & SITHAIUTIL_MKPOINT_TOWARDREF) != 0 && !bInFront)
                || ((flags & SITHAIUTIL_MKPOINT_AWAYFROMREF) != 0 && bInFront) )
            {
                if ( (flags & SITHAIUTIL_MKPOINT_YAW) != 0 )
                {
                    rdVector_Neg3Acc(&rotPyr);
                    rdVector_Rotate3(&newDir, pDir, &rotPyr);
                }
                else
                {
                    rdVector_Neg3Acc(&newDir);
                }
            }

            if ( (flags & SITHAIUTIL_MKPOINT_TARGETDIST) != 0 )
            {
                *pDownCos = -rdMath_DistancePointToPlane(pPos, &newDir, &movePos);
            }
        }

        if ( stdMath_ClipNearZero(*pDownCos) != 0.0f )
        {
            sithAIUtil_mkPointCurFlags  = flags;
            sithAIUtil_pMkPointCurLocal = pLocal;
            sithAIUtil_mkPointCurDir    = *pDir;

            if ( (flags & SITHAIUTIL_MKPOINT_YAW) != 0 )
            {
                sithAIUtil_mkPointCurPYR = rotPyr;
            }
            else
            {
                sithAIUtil_mkPointCurRVec = newDir;
            }

            rdVector_ScaleAdd3(pDestPoint, &newDir, *pDownCos, pPos);

            *pDir     = newDir;
            *pDownCos = fabsf(*pDownCos);
        }
        else
        {
            bValid = false;
        }
    }

    return bValid;
}

int J3DAPI sithAIUtil_RetryMakeRandPoint(SithAIControlBlock* pLocal, rdVector3* pPos, float downCos, rdVector3* pDirection, rdVector3* pDestPoint)
{
    rdVector3 dir;

    if ( !sithAIUtil_mkPointCurFlags || !sithAIUtil_pMkPointCurLocal || !pLocal )
    {
        return 0;
    }

    if ( pLocal != sithAIUtil_pMkPointCurLocal || !pPos || !pDirection || !pDestPoint )
    {
        SITHLOG_ERROR("sithAIUtil_RetryMakeRandPoint: Bad parameters.\n");
        return 0;
    }

    if ( (sithAIUtil_mkPointCurFlags & SITHAIUTIL_MKPOINT_NORETRY) != 0 )
    {
        sithAIUtil_mkPointCurFlags  = 0;
        sithAIUtil_pMkPointCurLocal = NULL;
        return 0;
    }

    if ( (sithAIUtil_mkPointCurFlags & SITHAIUTIL_MKPOINT_ROTATE) != 0 )
    {
        rdVector_Neg3Acc(&sithAIUtil_mkPointCurPYR);
        rdVector_Rotate3(&dir, &sithAIUtil_mkPointCurDir, &sithAIUtil_mkPointCurPYR);

        sithAIUtil_mkPointCurFlags &= ~SITHAIUTIL_MKPOINT_ROTATE;
    }
    else if ( (sithAIUtil_mkPointCurFlags & SITHAIUTIL_MKPOINT_SIDEWAYS) != 0 )
    {
        rdVector_Neg3(&dir, &sithAIUtil_mkPointCurRVec);

        sithAIUtil_mkPointCurFlags &= ~SITHAIUTIL_MKPOINT_SIDEWAYS;
    }
    else
    {
        sithAIUtil_mkPointCurFlags  = 0;
        sithAIUtil_pMkPointCurLocal = NULL;
        return 0;
    }

    rdVector_ScaleAdd3(pDestPoint, &dir, downCos, pPos);

    *pDirection = dir;
    return 1;
}

SithAIUtilPathResult J3DAPI sithAIUtil_CheckPathToPos(SithAIControlBlock* pLocal, SithSector* pStartSector, rdVector3* startPos, SithAIUtilPathFlags flags, float minMoveDist, float moveDist, float radius, rdVector3* pDirection, rdVector3* pEndPos)
{
    SITH_ASSERTREL(pLocal && pLocal->pOwner);
    SithThing* pOwner = pLocal->pOwner;

    if ( !pDirection )
    {
        SITHLOG_ERROR("sithAIUtil_CheckPathToPos: Bad direction vector.\n");
        return SITHAIUTIL_PATH_BLOCKED;
    }

    if ( radius == 0.0f )
    {
        radius = pOwner->collide.movesize;
    }

    if ( (flags & SITHAIUTIL_PATHFLAG_STRAIGHT) != 0 )
    {
        SithAIUtilPathResult result = SITHAIUTIL_PATH_CLEAR;
        rdVector3 endPos;
        rdVector_ScaleAdd3(&endPos, pDirection, moveDist, startPos);

        sithCollision_SearchForCollisions(pStartSector, pOwner, startPos, pDirection, moveDist, radius, 0x102);

        SithCollision* pCollision;
        while ( (pCollision = sithCollision_PopStack()) != NULL )
        {
            if ( (flags & SITHAIUTIL_PATHFLAG_IGNOREPLAYER) != 0
                && (pCollision->type & SITHCOLLISION_THING) != 0
                && pCollision->pThingCollided->type == SITH_THING_PLAYER )
            {
                continue;
            }

            if ( (flags & SITHAIUTIL_PATHFLAG_IGNORETHINGS) != 0 && (pCollision->type & SITHCOLLISION_THING) != 0 )
            {
                continue;
            }

            if ( (flags & SITHAIUTIL_PATHFLAG_IGNOREFLOORS) != 0
                && (pCollision->type & SITHCOLLISION_WORLD) != 0
                && (pCollision->pSurfaceCollided->flags & SITH_SURFACE_ISFLOOR) != 0 )
            {
                continue;
            }

            if ( pCollision->distance >= minMoveDist )
            {
                rdVector_ScaleAdd3(&endPos, pDirection, pCollision->distance, startPos);
            }
            else
            {
                result = SITHAIUTIL_PATH_BLOCKED;
                break;
            }

            if ( (pCollision->type & SITHCOLLISION_THING) != 0 )
            {
                result = SITHAIUTIL_PATH_THING;
                break;
            }

            if ( (pCollision->type & SITHCOLLISION_WORLD) != 0 )
            {
                result = SITHAIUTIL_PATH_WORLD;
                break;
            }
        }

        sithCollision_DecreaseStackLevel();

        if ( result != SITHAIUTIL_PATH_BLOCKED )
        {
            float targetDist;
            rdVector3 toTarget;

            SithThing* pTarget = NULL;
            if ( (flags & SITHAIUTIL_PATHFLAG_GOALVISIBLE) != 0 && pLocal->goalThing )
            {
                pTarget = pLocal->goalThing;
            }

            if ( (flags & SITHAIUTIL_PATHFLAG_TARGETVISIBLE) != 0 && pLocal->pTargetThing )
            {
                pTarget = pLocal->pTargetThing;
            }

            if ( pTarget
                && sithAIUtil_GetDistanceToTargetPos(pTarget, &pTarget->pos, &endPos, -1.0f, pLocal->pClass->sightDistance, 0.0f, &toTarget, &targetDist) == SITHAI_SIGHT_HIDDEN )
            {
                return SITHAIUTIL_PATH_BLOCKED;
            }

            if ( pEndPos )
            {
                *pEndPos = endPos;
            }
        }

        return result;
    }

    if ( (flags & SITHAIUTIL_PATHFLAG_SIDES) != 0 )
    {
        int aScores[2]                   = { 0, 0 };
        SithAIUtilPathResult aResults[2] = { SITHAIUTIL_PATH_BLOCKED, SITHAIUTIL_PATH_BLOCKED };
        rdVector3 aEndPos[2];

        int side       = 1;
        bool bSideDone = false;
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

                bSideDone = false;
                distance  = minMoveDist;
                rdVector_Neg3Acc(&perp);
            }

            distance += 0.03f;
            if ( distance > moveDist )
            {
                bSideDone = true;
                continue;
            }

            rdVector_ScaleAdd3(&aEndPos[side], &perp, distance, startPos);

            SithSector* pSector = sithCollision_FindSectorInRadius(pStartSector, startPos, &aEndPos[side], radius);

            if ( !pSector )
            {
                bSideDone = true;
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
                    aResults[side] = SITHAIUTIL_PATH_THING;
                }
                else if ( (pCollision->type & SITHCOLLISION_WORLD) != 0 )
                {
                    aResults[side] = SITHAIUTIL_PATH_WORLD;
                }

                bSideDone = true;
            }
            else
            {
                aResults[side] = sithAIUtil_CheckPathToPos(pLocal, pSector, &aEndPos[side], SITHAIUTIL_PATHFLAG_IGNOREFLOORS | SITHAIUTIL_PATHFLAG_STRAIGHT, pOwner->collide.movesize + 0.03f, radius, 0.0f, pDirection, NULL);

                if ( aResults[side] == SITHAIUTIL_PATH_CLEAR )
                {
                    bSideDone = true;
                }
            }
        }

        if ( pLocal->goalThing )
        {
            for ( size_t i = 0; i < STD_ARRAYLEN(aEndPos); ++i )
            {
                bool bVisible = sithAIUtil_GetDistanceToTargetPos(pLocal->goalThing, &pLocal->goalThing->pos, &aEndPos[i], -1.0f, pLocal->pClass->sightDistance, 0.0f, &pLocal->toGoal, &pLocal->targetDistance) != SITHAI_SIGHT_HIDDEN;
                if ( bVisible )
                {
                    ++aScores[i];
                }
                else if ( (flags & SITHAIUTIL_PATHFLAG_GOALVISIBLE) != 0 )
                {
                    --aScores[i];
                }
            }

            float goalDist = rdVector_Dist2((const rdVector2*)&aEndPos[0], (const rdVector2*)&pLocal->goalThing->pos);

            if ( goalDist < rdVector_Dist2((const rdVector2*)&aEndPos[1], (const rdVector2*)&pLocal->goalThing->pos) )
            {
                if ( (flags & SITHAIUTIL_PATHFLAG_PREFERGOAL) != 0 )
                {
                    ++aScores[0];
                }
                else if ( (flags & SITHAIUTIL_PATHFLAG_AVOIDGOAL) != 0 )
                {
                    --aScores[0];
                }
            }
            else
            {
                if ( (flags & SITHAIUTIL_PATHFLAG_PREFERGOAL) != 0 )
                {
                    ++aScores[1];
                }
                else if ( (flags & SITHAIUTIL_PATHFLAG_AVOIDGOAL) != 0 )
                {
                    --aScores[1];
                }
            }
        }

        int i = aScores[0] < aScores[1];
        if ( pEndPos )
        {
            *pEndPos = aEndPos[i];
        }

        return aResults[i];
    }

    return SITHAIUTIL_PATH_BLOCKED;
}

int J3DAPI sithAIUtil_SweepMoveToPathPos(SithAIControlBlock* pLocal, rdVector3* pPos, float angle, float maxAngle, float deltaAngle, SithAIUtilMkPointFlags flags)
{
    rdVector3 toTargetDir;
    rdVector_Sub3(&toTargetDir, pPos, &pLocal->pOwner->pos);

    toTargetDir.z = 0.0f;
    float pathDist = rdVector_Normalize3Acc(&toTargetDir);

    flags &= ~SITHAIUTIL_SWEEP_CLEARFLAGS;
    SithAIUtilMkPointFlags randFlags = flags | SITHAIUTIL_MKPOINT_RANDSIDE | SITHAIUTIL_MKPOINT_YAW;
    for ( float curAngle = angle; curAngle <= maxAngle; curAngle += deltaAngle )
    {
        rdVector3 randDir = toTargetDir;
        rdVector3 destPos;
        if ( !sithAIUtil_MakeRandPoint(pLocal, &pLocal->pOwner->pos, randFlags, curAngle, &pathDist, &randDir, &destPos) )
        {
            return 0;
        }

        int bFound;
        float hitDist;
        rdVector3 surfaceNorm;
        do
        {
            bFound = sithAIUtil_CheckPathToPoint(pLocal->pOwner, &destPos, pLocal->pOwner->collide.movesize, &hitDist, &surfaceNorm, 1, 0);
        } while ( !bFound
            && curAngle != 0.0f
            && curAngle != 180.0f
            && sithAIUtil_RetryMakeRandPoint(pLocal, &pLocal->pOwner->pos, pathDist, &toTargetDir, &destPos) );

        if ( bFound )
        {
            sithAIMove_StopAIMovement(pLocal);
            sithAIMove_AISetLookPosEyeLevel(pLocal, &destPos);
            sithAIMove_AISetMoveTargetPos(pLocal, &destPos, pLocal->moveSpeed);
            return 1;
        }
    }

    return 0;
}

int J3DAPI sithAIUtil_MakeRoamPos(SithAIControlBlock* pLocal, rdVector3* pPos, float minDist, float moveDist, int bAllow3D, int maxRetries, rdVector3* pDestPos)
{
    size_t numRetries = 0;
    rdVector3 dir     = pLocal->pOwner->orient.lvec;

    SithAIUtilPathResult curResult;
    do
    {
        SithAIUtilMkPointFlags mkFlags = SITHAIUTIL_MKPOINT_RANDSIDE | SITHAIUTIL_MKPOINT_RANDANGLE | SITHAIUTIL_MKPOINT_YAW;
        if ( bAllow3D )
        {
            mkFlags |= SITHAIUTIL_MKPOINT_PITCH;
        }

        if ( !sithAIUtil_MakeRandPoint(pLocal, pPos, mkFlags, 360.0f, &moveDist, &dir, pDestPos) )
        {
            return 0;
        }

        do
        {
            SithAIUtilPathFlags pathFlags;
            if ( bAllow3D )
            {
                pathFlags = SITHAIUTIL_PATHFLAG_STRAIGHT;
            }
            else
            {
                pathFlags = SITHAIUTIL_PATHFLAG_IGNOREFLOORS | SITHAIUTIL_PATHFLAG_STRAIGHT;
            }

            curResult = sithAIUtil_CheckPathToPos(pLocal, pLocal->pOwner->pInSector, &pLocal->pOwner->pos, pathFlags, minDist, moveDist, 0.0f, &dir, pDestPos);
        } while ( curResult == SITHAIUTIL_PATH_BLOCKED
            && sithAIUtil_RetryMakeRandPoint(pLocal, &pLocal->pOwner->pos, moveDist, &dir, pDestPos) );
    } while ( curResult == SITHAIUTIL_PATH_BLOCKED && ++numRetries < (size_t)maxRetries );

    return curResult != SITHAIUTIL_PATH_BLOCKED;
}

int J3DAPI sithAIUtil_MakePathPos(SithAIControlBlock* pLocal, rdVector3* pPos, float minDist, float moveDist, float angle, int flags, rdVector3* pDirection, rdVector3* pDestPos)
{
    if ( !sithAIUtil_MakeRandPoint(pLocal, pPos, flags, angle, &moveDist, pDirection, pDestPos) )
    {
        return 0;
    }

    SithAIUtilPathFlags pathFlags = flags & SITHAIUTIL_MAKEPATH_PATHFLAGS;
    if ( (flags & SITHAIUTIL_MKPOINT_PITCH) != 0 )
    {
        pathFlags |= SITHAIUTIL_PATHFLAG_STRAIGHT;
    }
    else
    {
        pathFlags |= SITHAIUTIL_PATHFLAG_IGNOREFLOORS | SITHAIUTIL_PATHFLAG_STRAIGHT;
    }

    while ( sithAIUtil_CheckPathToPos(pLocal, pLocal->pOwner->pInSector, &pLocal->pOwner->pos, pathFlags, minDist, moveDist, 0.0f, pDirection, pDestPos) == SITHAIUTIL_PATH_BLOCKED )
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

    rdVector3 curDir;
    rdVector_Sub3(&curDir, &pLocal->pTargetThing->pos, pPos);

    float dist = rdVector_Normalize3Acc(&curDir);

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

        float dot = rdVector_Dot3(&cur->toTarget, &curDir);

        if ( dot >= 0.966f && dist > cur->distance )
        {
            return 0;
        }
    }

    return 1;
}

void J3DAPI sithAIUtil_AIPlaySoundMode(SithAIControlBlock* pLocal, SithSoundClassMode mode)
{
    if ( !pLocal || !pLocal->pOwner )
    {
        return;
    }

    if ( (unsigned int)mode >= SITHSOUNDCLASS_CURIOUS && (unsigned int)mode <= SITHSOUNDCLASS_RESERVED8 && sithAI_HasInstinct(pLocal, "talk") )
    {
        sithAI_EmitEvent(pLocal, SITHAI_EVENT_TALK, (void*)mode);
    }
    else
    {
        sithSoundClass_PlayModeRandom(pLocal->pOwner, mode);
    }
}

void J3DAPI sithAIUtil_AISetMode(SithAIControlBlock* pLocal, SithAIMode mode)
{
    switch ( mode )
    {
        case SITHAI_MODE_MOVING:
            pLocal->mode |= SITHAI_MODE_MOVING;
            pLocal->submode |= SITHAI_SUBMODE_NEWMOVE;
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
            pLocal->submode &= ~SITHAI_SUBMODE_MOVETARGETLOCKED;
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
            pLocal->submode |= SITHAI_SUBMODE_MOVEFORWARD;
            break;
    }
}

void J3DAPI sithAIUtil_AIClearMode(SithAIControlBlock* pLocal, SithAIMode mode)
{
    switch ( mode )
    {
        case SITHAI_MODE_MOVING:
            pLocal->mode &= ~SITHAI_MODE_MOVING;
            pLocal->mode &= ~SITHAI_MODE_DODGING;
            pLocal->submode &= ~(SITHAI_SUBMODE_MOVETARGETLOCKED | SITHAI_SUBMODE_UNKNOWN_2);
            pLocal->submode &= ~(SITHAI_SUBMODE_NEWMOVE | SITHAI_SUBMODE_UNKNOWN_4);
            pLocal->submode &= ~(SITHAI_SUBMODE_UNKNOWN_8 | SITHAI_SUBMODE_ALIGNING);
            break;

        case SITHAI_MODE_TURNING:
            pLocal->mode &= ~SITHAI_MODE_TURNING;
            pLocal->mode &= ~SITHAI_MODE_LOOKING;
            pLocal->submode &= ~SITHAI_SUBMODE_ALIGNING;
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
            pLocal->submode &= ~SITHAI_SUBMODE_MOVEFORWARD;
            break;
    }
}

void J3DAPI sithAIUtil_SetWeaponFireFlags(int weaponNum, SithAIUtilFireFlags* pFlags)
{
    switch ( weaponNum )
    {
        case 0:
            *pFlags |= SITHAIUTIL_FIRE_WEAPON1;
            break;

        case 1:
            *pFlags |= SITHAIUTIL_FIRE_WEAPON2;
            break;

        case 2:
            *pFlags |= SITHAIUTIL_FIRE_WEAPON3;
            break;

        case 3:
            *pFlags |= SITHAIUTIL_FIRE_WEAPON4;
            break;
    }
}

int J3DAPI sithAIUtil_AIGetMovePos(const SithAIControlBlock* pLocal, SithAIUtilMkPointFlags flags, rdVector3* pDestPos)
{
    if ( (flags & SITHAIUTIL_MKPOINT_REFOWNER) != 0 )
    {
        *pDestPos = pLocal->pOwner->pos;
        return 1;
    }

    if ( (flags & SITHAIUTIL_MKPOINT_TOWARDMOVEPOS) != 0 )
    {
        *pDestPos = pLocal->movePos;
        return 1;
    }

    if ( (flags & SITHAIUTIL_MKPOINT_REFHOME) != 0 )
    {
        *pDestPos = pLocal->homePos;
        return 1;
    }

    if ( (flags & (SITHAIUTIL_MKPOINT_TOWARDGOAL | SITHAIUTIL_MKPOINT_AWAYFROMGOAL)) != 0 )
    {
        if ( pLocal->goalThing )
        {
            *pDestPos = pLocal->goalThing->pos;
        }
        else
        {
            *pDestPos = pLocal->goalPos;
        }

        return 1;
    }

    if ( (flags & SITHAIUTIL_MKPOINT_AWAYFROMFLEE) != 0 )
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

    if ( !rdVector_IsZero3(&pThing->moveInfo.physics.velocity) )
    {
        *pDest = pThing->moveInfo.physics.velocity;
    }
    else
    {
        *pDest = pThing->orient.lvec;
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

    if ( !rdVector_IsZero3(&pThing->moveInfo.physics.velocity) )
    {
        *pDest = pThing->moveInfo.physics.velocity;
        pDest->x = stdMath_ClipNearZero(pDest->x);
        pDest->y = stdMath_ClipNearZero(pDest->y);
        pDest->z = stdMath_ClipNearZero(pDest->z);
        rdVector_Normalize3Acc(pDest);
    }
    else
    {
        *pDest = pThing->orient.lvec;
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
                bMoving = !rdVector_IsZero3(&pThing->moveInfo.physics.velocity)
                    | !rdVector_IsZero3(&pThing->moveInfo.physics.angularVelocity);
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
    pOutPYR->x = (SITH_RANDF() - 0.5f) * scalar + pOutPYR->x;
    pOutPYR->y = (SITH_RANDF() - 0.5f) * scalar + pOutPYR->y;
    pOutPYR->z = (SITH_RANDF() - 0.5f) * scalar + pOutPYR->z;
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
        if ( sithAIUtil_numVisitedSectors >= SITHAIUTIL_MAXTHINGSINVIEWSECTORS )
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

    if ( !(distance < 2.0f) ) // Note, not distance >= 2.0f so that NaN distance is handled as in the original
    {
        if ( (pLocal->mode & SITHAI_MODE_ACTIVE) == 0 )
        {
            chance *= 0.5f;
        }

        if ( (pTarget->thingInfo.actorInfo.flags & SITH_AF_HEADLIGHT) == 0 && (pTarget->unknownFlags & 1) == 0 )
        {
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

                if ( rdVector_IsZero3(&pTarget->moveInfo.physics.velocity) )
                {
                    chance *= 0.5f;
                }
            }
        }
    }

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
    if ( SITH_RANDF() < chance )
    {
        return 1;
    }

    return 0;
}

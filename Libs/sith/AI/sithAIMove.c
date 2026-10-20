#include "sithAIMove.h"
#include <j3dcore/j3dhook.h>

#include <math.h>
#include <string.h>

#include <rdroid/Math/rdMath.h>
#include <rdroid/Math/rdMatrix.h>
#include <rdroid/Math/rdVector.h>

#include <sith/AI/sithAI.h>
#include <sith/AI/sithAIUtil.h>
#include <sith/Cog/sithCog.h>
#include <sith/Engine/sithCollision.h>
#include <sith/Engine/sithPuppet.h>
#include <sith/Engine/sithPhysics.h>
#include <sith/Gameplay/sithPlayerActions.h>
#include <sith/Gameplay/sithPlayerControls.h>
#include <sith/Gameplay/sithTime.h>
#include <sith/RTI/symbols.h>
#include <sith/World/sithActor.h>
#include <sith/World/sithSoundClass.h>
#include <sith/World/sithTemplate.h>
#include <sith/World/sithThing.h>
#include <sith/World/sithWeapon.h>
#include <sith/World/sithWorld.h>

#include <std/General/stdMath.h>
#include <std/General/stdUtil.h>

static float sithAIMove_maxTurnSpeed = 0.0049999999f;
static float sithAIMove_maxWalkSpeed = 0.27000001f;
static float sithAIMove_minWalkSpeed = 0.001f;

// Debug vars, for debugging AISetMoveToPos
static SithThing* sithAIMove_pMoveToDebugMarkThing      = NULL;
static SithThing* sithAIMove_pMoveToPosGroundMarkThing  = NULL;;
static bool sithAIMove_bDebugMoveToPo = false;

void J3DAPI sithAIMove_AIFinalizeSpecialMove(SithAIControlBlock* pLocal, SithActorSpecialMoveFlags type);

void sithAIMove_InstallHooks(void)
{
    // Uncomment only lines for functions that have full definition and doesn't call original function (non-thunk functions)

    J3D_HOOKFUNC(sithAIMove_Update);
    J3D_HOOKFUNC(sithAIMove_UpdateMineCar);
    J3D_HOOKFUNC(sithAIMove_UpdateTurn);
    J3D_HOOKFUNC(sithAIMove_UpdateMove);
    J3D_HOOKFUNC(sithAIMove_CheckMoveAlign);
    J3D_HOOKFUNC(sithAIMove_GetGoalTolerance);
    J3D_HOOKFUNC(sithAIMove_CheckWalkMove);
    J3D_HOOKFUNC(sithAIMove_CheckFlyMove);
    J3D_HOOKFUNC(sithAIMove_CheckSwimMove);
    J3D_HOOKFUNC(sithAIMove_AIGetMoveState);
    J3D_HOOKFUNC(sithAIMove_AISpecialTurn);
    J3D_HOOKFUNC(sithAIMove_UpdateAIMove);
    J3D_HOOKFUNC(sithAIMove_GetAIMoveModes);
    J3D_HOOKFUNC(sithAIMove_SetSubMode);
    J3D_HOOKFUNC(sithAIMove_PuppetCallback);
    J3D_HOOKFUNC(sithAIMove_AISetLookThing);
    J3D_HOOKFUNC(sithAIMove_AISetLookPos);
    J3D_HOOKFUNC(sithAIMove_AISetLookPosEyeLevel);
    J3D_HOOKFUNC(sithAIMove_AISetMovePos);
    J3D_HOOKFUNC(sithAIMove_AISetMoveTargetPos);
    J3D_HOOKFUNC(sithAIMove_AISpecialMove);
    J3D_HOOKFUNC(sithAIMove_AIFinalizeSpecialMove);
    J3D_HOOKFUNC(sithAIMove_AIJump);
    J3D_HOOKFUNC(sithAIMove_AIStop);
    J3D_HOOKFUNC(sithAIMove_SetGoalReached);
    J3D_HOOKFUNC(sithAIMove_Unreachable);
    J3D_HOOKFUNC(sithAIMove_StopAIMovement);
    J3D_HOOKFUNC(sithAIMove_ResetAILook);
    J3D_HOOKFUNC(sithAIMove_UpdateBoss);
    J3D_HOOKFUNC(sithAIMove_UpdateHeadTrackTarget);
    J3D_HOOKFUNC(sithAIMove_UpdateHeadTrackLookPos);
    J3D_HOOKFUNC(sithAIMove_GetQuetzStrikeAngles);
    J3D_HOOKFUNC(sithAIMove_UpdateQuetzStrike);
    J3D_HOOKFUNC(sithAIMove_GetQuetzAnim2Angles);
    J3D_HOOKFUNC(sithAIMove_UpdateQuetzAnim2);
    J3D_HOOKFUNC(sithAIMove_UpdateQuetzAnim3);
    J3D_HOOKFUNC(sithAIMove_UpdateQuetzAnim5);
    J3D_HOOKFUNC(sithAIMove_UpdateQuetzAnim4);
    J3D_HOOKFUNC(sithAIMove_UpdateQuetzTail);
    J3D_HOOKFUNC(sithAIMove_StartQuetzStrike);
    J3D_HOOKFUNC(sithAIMove_StartQuetzAnim2);
    J3D_HOOKFUNC(sithAIMove_StartQuetzAnim3);
    J3D_HOOKFUNC(sithAIMove_StartQuetzAnim5);
    J3D_HOOKFUNC(sithAIMove_StartQuetzAnim4);
    J3D_HOOKFUNC(sithAIMove_UpdateMardukTail);
}

void sithAIMove_ResetGlobals(void)
{
    memset(&sithAIMove_g_flt_585464, 0, sizeof(sithAIMove_g_flt_585464));
    memset(&sithAIMove_g_flt_585468, 0, sizeof(sithAIMove_g_flt_585468));
    memset(&sithAIMove_g_flt_58546C, 0, sizeof(sithAIMove_g_flt_58546C));
    memset(&sithAIMove_g_flt_585470, 0, sizeof(sithAIMove_g_flt_585470));
}

void J3DAPI sithAIMove_Update(SithThing* pThing, float secDeltatTime)
{
    SITH_ASSERTREL(pThing);
    SITH_ASSERTREL(pThing->controlType == SITH_CT_AI);

    if ( pThing->type != SITH_THING_ACTOR )
    {
        return;
    }

    if ( pThing->thingInfo.actorInfo.health <= 0.0f )
    {
        return;
    }

    SithAIControlBlock* pLocal = pThing->controlInfo.aiControl.pLocal;
    SITH_ASSERTREL(pLocal);

    if ( (pLocal->mode & SITHAI_MODE_SLEEPING) != 0 && (pLocal->mode & SITHAI_MODE_BLOCK) == 0 && (pThing->thingInfo.actorInfo.flags & SITH_AF_NOSLOPEMOVE) == 0 )
    {
        return;
    }

    if ( (pThing->flags & (SITH_TF_DISABLED | SITH_TF_DYING | SITH_TF_DESTROYED)) != 0 )
    {
        return;
    }

    if ( (pLocal->submode & SITHAI_SUBMODE_UNKNOWN_100) != 0 && (pLocal->mode & SITHAI_MODE_MOVING) != 0 && (pLocal->mode & SITHAI_MODE_DISABLED) == 0 )
    {
        rdVector3 lookPos;
        rdVector_Copy3(&lookPos, &pLocal->movePos);
        lookPos.z += pLocal->pOwner->thingInfo.actorInfo.eyeOffset.z;
        sithAIMove_AISetLookPos(pLocal, &lookPos);
    }

    if ( (pLocal->mode & SITHAI_MODE_TURNING) != 0 || (pLocal->submode & SITHAI_SUBMODE_BODYTRACKINGMOTION) != 0 )
    {
        if ( (pThing->thingInfo.actorInfo.flags & SITH_AF_BOSS) != 0 )
        {
            sithAIMove_UpdateBoss(pLocal, secDeltatTime);
        }
        else if ( pLocal->pOwner->moveType == SITH_MT_PHYSICS && (pThing->moveInfo.physics.flags & SITH_PF_MINECAR) != 0 )
        {
            sithAIMove_UpdateMineCar(pLocal, secDeltatTime);
        }
        else
        {
            if ( (pLocal->submode & SITHAI_SUBMODE_BODYTRACKINGMOTION) != 0 && pLocal->pTargetThing )
            {
                if ( (pLocal->mode & SITHAI_MODE_TURNING) != 0 || (pLocal->submode & SITHAI_SUBMODE_UNKNOWN_400) == 0 )
                {
                    sithAIMove_AISetLookThing(pLocal, pLocal->pTargetThing);
                }
                else if ( fabsf(pThing->thingInfo.actorInfo.headPYR.y) > 20.0f )
                {
                    sithAIMove_AISetLookThing(pLocal, pLocal->pTargetThing);
                }
            }

            sithAIMove_UpdateTurn(pLocal, secDeltatTime);
        }
    }

    if ( (pLocal->submode & SITHAI_SUBMODE_UNKNOWN_10) != 0 )
    {
        if ( (pLocal->mode & SITHAI_MODE_TRAVERSEWPNTS) != 0 )
        {
            sithAIUtil_CheckWpntMoveAlign(pLocal);
        }
        else
        {
            sithAIMove_CheckMoveAlign(pLocal);
        }
    }

    if ( (pLocal->mode & SITHAI_MODE_MOVING) != 0 && (pLocal->submode & SITHAI_SUBMODE_UNKNOWN_10) == 0 )
    {
        sithAIMove_UpdateMove(pLocal, secDeltatTime);
    }

    if ( sithWeapon_HasWeaponSelected(pLocal->pOwner) )
    {
        sithPlayerControls_ProcessWeaponAim(pLocal->pOwner, secDeltatTime);
    }

    if ( !sithWeapon_IsAiming(pLocal->pOwner)
        && ((pLocal->mode & (SITHAI_MODE_UNKNOWN_80 | SITHAI_MODE_TURNING)) != 0 || (pLocal->submode & SITHAI_SUBMODE_UNKNOWN_400) != 0) )
    {
        if ( ((pLocal->mode & SITHAI_MODE_BLOCK) != 0 || (pThing->thingInfo.actorInfo.flags & SITH_AF_NOSLOPEMOVE) != 0)
            && (pLocal->submode & SITHAI_SUBMODE_UNKNOWN_400) == 0 )
        {
            sithAIMove_UpdateHeadTrackLookPos(pLocal, secDeltatTime);
        }
        else
        {
            sithAIMove_UpdateHeadTrackTarget(pLocal, secDeltatTime);
        }
    }

    if ( pLocal->pOwner->pAttachedThing && (pLocal->pOwner->pAttachedThing->attach.flags & SITH_ATTACH_TAIL) != 0 )
    {
        sithAIMove_UpdateQuetzTail(pLocal, secDeltatTime);
    }
    else if ( strncmp(pThing->aName, "marduk", 6) == 0 )
    {
        sithAIMove_UpdateMardukTail(pThing, secDeltatTime);
    }
}

void J3DAPI sithAIMove_UpdateMineCar(SithAIControlBlock* pLocal, float secDeltaTime)
{
    SithThing* pThing = pLocal->pOwner;
    SithActorInfo* pActorInfo = &pThing->thingInfo.actorInfo;
    if ( (pActorInfo->flags & SITH_AF_IMMOBILE) != 0 )
    {
        return;
    }

    int meshIdx = sithThing_GetThingMeshIndex(pThing, "redarmy");

    rdVector3 jointPos;
    rdVector_Copy3(&jointPos, &pThing->renderData.paJointMatrices[meshIdx].dvec);

    rdVector3 toLook;
    rdVector_Sub3(&toLook, &pLocal->lookPos, &jointPos);

    rdVector3 lvec;
    rdVector_Rotate3(&lvec, &pThing->orient.lvec, &pThing->renderData.apTweakedAngles[meshIdx]);
    rdVector_Normalize3Acc(&lvec);

    float dist = rdMath_DistancePointToPlane(&lvec, &pThing->orient.uvec, &rdroid_g_zeroVector3);

    rdVector3 aimDir;
    rdVector_ScaleAdd3(&aimDir, &pThing->orient.uvec, -dist, &lvec);
    rdVector_Normalize3Acc(&aimDir);

    rdVector3 rvec;
    rdVector_Cross3(&rvec, &aimDir, &pThing->orient.uvec);
    rdVector_Normalize3Acc(&rvec);

    // TODO: [BUG] The original never assigns the local moveState (sithAIMove_UpdateTurn sets it from sithAIMove_AIGetMoveState).
    //       The compiler reuses the stack slot of the pLocal argument for both moveState and angle: when angle is computed the slot
    //       holds angle's bits, so moveState is zero only if angle is +0.0f; when the actor looks straight up or down angle is never
    //       stored and the slot still holds the never-NULL pLocal pointer, so moveState is nonzero.
    int moveState = 1;

    float upDot = fabsf(rdVector_Dot3(&pThing->orient.uvec, &toLook));
    if ( upDot > 0.999f )
    {
        if ( (pThing->moveInfo.physics.flags & SITH_PF_FLY) == 0 )
        {
            SITHLOG_ERROR("Strange... AI '%s' is looking straight up or down.\n", pThing->aName);
        }
    }
    else
    {
        dist = rdMath_DistancePointToPlane(&toLook, &pThing->orient.uvec, &rdroid_g_zeroVector3);

        rdVector3 lookDir;
        rdVector_ScaleAdd3(&lookDir, &pThing->orient.uvec, -dist, &toLook);
        rdVector_Normalize3Acc(&lookDir);

        float angle = rdMath_DeltaAngleNormalized(&lookDir, &aimDir, &pThing->orient.uvec);
        memcpy(&moveState, &angle, sizeof(moveState));

        float turnRate = pActorInfo->maxRotVelocity * secDeltaTime;
        float maxStep  = pActorInfo->maxRotVelocity * 0.07f;
        if ( fabsf(angle) <= maxStep && fabsf(angle) >= 1.0f )
        {
            turnRate = (fabsf(angle) / maxStep) * turnRate;
        }

        float turnLimit = turnRate * 1.05f;
        if ( fabsf(angle) <= turnLimit )
        {
            float fwdDot   = rdVector_Dot3(&pThing->orient.lvec, &lookDir);
            float rightDot = rdVector_Dot3(&pThing->orient.rvec, &lookDir);
            pThing->renderData.apTweakedAngles[meshIdx].y = 90.0f - stdMath_ArcSin3(fwdDot);
            if ( rightDot > 0.0f )
            {
                pThing->renderData.apTweakedAngles[meshIdx].y = -pThing->renderData.apTweakedAngles[meshIdx].y;
            }
        }
        else
        {
            rdVector3 pyr;
            pyr.x = 0.0f;
            pyr.y = -((angle < 0.0f ? -1.0f : 1.0f) * turnRate);
            pyr.z = 0.0f;
            rdVector_Rotate3Acc(&aimDir, &pyr);

            float fwdDot   = rdVector_Dot3(&pThing->orient.lvec, &aimDir);
            float rightDot = rdVector_Dot3(&pThing->orient.rvec, &aimDir);
            pThing->renderData.apTweakedAngles[meshIdx].y = 90.0f - stdMath_ArcSin3(fwdDot);
            if ( rightDot > 0.0f )
            {
                pThing->renderData.apTweakedAngles[meshIdx].y = -pThing->renderData.apTweakedAngles[meshIdx].y;
            }

            if ( moveState == 0 )
            {
                if ( angle < 0.0f )
                {
                    pThing->moveStatus = SITHPLAYERMOVE_TURNING_LEFT;
                    if ( angle < -45.0f )
                    {
                        pThing->moveStatus = SITHPLAYERMOVE_UNKNOWN_105;
                    }
                }
                else
                {
                    pThing->moveStatus = SITHPLAYERMOVE_TURNING_RIGHT;
                    if ( angle > 45.0f )
                    {
                        pThing->moveStatus = SITHPLAYERMOVE_UNKNOWN_106;
                    }
                }
            }

            return;
        }
    }

    pLocal->mode &= ~SITHAI_MODE_TURNING;

    if ( moveState == 0 )
    {
        pThing->moveStatus = SITHPLAYERMOVE_STILL;
    }
}

void J3DAPI sithAIMove_UpdateTurn(SithAIControlBlock* pLocal, float secDeltatTime)
{
    SithThing* pOwner = pLocal->pOwner;
    SithActorInfo* pActorInfo = &pOwner->thingInfo.actorInfo;

    rdVector3 goalLVec;
    rdVector_Copy3(&goalLVec, &pLocal->goalLVec);

    if ( (pActorInfo->flags & SITH_AF_IMMOBILE) != 0 )
    {
        return;
    }

    int moveState = sithAIMove_AIGetMoveState(pLocal);
    if ( moveState == 2 )
    {
        return;
    }

    float upDot = fabsf(rdVector_Dot3(&pOwner->orient.uvec, &goalLVec));
    if ( upDot > 0.999f )
    {
        if ( (pOwner->moveInfo.physics.flags & SITH_PF_FLY) == 0 )
        {
            SITHLOG_ERROR("Strange... AI '%s' is looking straight up or down.\n", pOwner->aName);
        }
    }
    else
    {
        float dist = rdMath_DistancePointToPlane(&goalLVec, &pOwner->orient.uvec, &rdroid_g_zeroVector3);

        rdVector3 lvec;
        rdVector_ScaleAdd3(&lvec, &pOwner->orient.uvec, -dist, &goalLVec);
        rdVector_Normalize3Acc(&lvec);

        float angle = rdMath_DeltaAngleNormalized(&lvec, &pOwner->orient.lvec, &pOwner->orient.uvec);
        if ( sithAIMove_AISpecialTurn(pLocal, angle) )
        {
            return;
        }

        float turnRate = pActorInfo->maxRotVelocity * secDeltatTime;
        float maxStep  = pActorInfo->maxRotVelocity * 0.07f;
        if ( fabsf(angle) <= maxStep && fabsf(angle) >= 1.0f )
        {
            turnRate = (fabsf(angle) / maxStep) * turnRate;
        }

        float turnLimit = turnRate * 1.05f;
        if ( fabsf(angle) <= turnLimit )
        {
            rdVector_Copy3(&pOwner->orient.lvec, &lvec);
            rdVector_Cross3(&pOwner->orient.rvec, &pOwner->orient.lvec, &pOwner->orient.uvec);
            rdVector_Normalize3Acc(&pOwner->orient.rvec);
            rdVector_Cross3(&pOwner->orient.uvec, &pOwner->orient.rvec, &pOwner->orient.lvec);
        }
        else
        {
            rdVector3 pyr;
            pyr.x = 0.0f;
            pyr.y = -((angle < 0.0f ? -1.0f : 1.0f) * turnRate);
            pyr.z = 0.0f;
            rdMatrix_PreRotate34(&pOwner->orient, &pyr);
            rdMatrix_Normalize34(&pOwner->orient);

            if ( moveState == 0 )
            {
                if ( angle < 0.0f )
                {
                    pOwner->moveStatus = SITHPLAYERMOVE_TURNING_LEFT;
                    if ( angle < -45.0f )
                    {
                        pOwner->moveStatus = SITHPLAYERMOVE_UNKNOWN_105;
                    }
                }
                else
                {
                    pOwner->moveStatus = SITHPLAYERMOVE_TURNING_RIGHT;
                    if ( angle > 45.0f )
                    {
                        pOwner->moveStatus = SITHPLAYERMOVE_UNKNOWN_106;
                    }
                }
            }

            return;
        }
    }

    pLocal->mode &= ~SITHAI_MODE_TURNING;
    if ( moveState == 0 )
    {
        pOwner->moveStatus = SITHPLAYERMOVE_STILL;
    }
}

void J3DAPI sithAIMove_UpdateMove(SithAIControlBlock* pLocal, float secDeltaTime)
{
    int bGoalReached = 0;

    SITH_ASSERTREL(pLocal);

    SithThing* pThing = pLocal->pOwner;
    SITH_ASSERTREL(pThing && (pThing->type == SITH_THING_ACTOR));

    if ( (pThing->flags & (SITH_TF_DISABLED | SITH_TF_DYING | SITH_TF_DESTROYED)) != 0 )
    {
        return;
    }

    if ( (pThing->thingInfo.actorInfo.flags & SITH_AF_IMMOBILE) != 0 )
    {
        return;
    }

    if ( sithTime_g_msecGameTime < pLocal->msecPauseMoveUntil )
    {
        return;
    }

    pLocal->submode &= ~(SITHAI_SUBMODE_UNKNOWN_4000 | SITHAI_SUBMODE_UNKNOWN_4);

    float moveDist = (pThing->thingInfo.actorInfo.maxThrust * secDeltaTime) * pLocal->moveSpeed;

    rdVector3 pos;
    rdVector_Copy3(&pos, &pThing->pos);

    rdVector3 movePos;
    rdVector_Copy3(&movePos, &pLocal->movePos);

    float goalDist = (float)sithAIMove_GetGoalTolerance(pLocal, &pos, &movePos);

    rdVector_Sub3(&pLocal->moveDirection, &movePos, &pos);
    pLocal->moveDistance = rdVector_Normalize3Acc(&pLocal->moveDirection);

    rdVector3 vel;
    if ( (pLocal->submode & SITHAI_SUBMODE_UNKNOWN_100) == 0 )
    {
        rdVector_Scale3(&vel, &pLocal->moveDirection, moveDist);
    }
    else
    {
        float dot = rdVector_Dot3(&pThing->orient.lvec, &pLocal->moveDirection);
        dot = STDMATH_CLAMP(dot, 0.3f, 1.0f);
        moveDist *= dot;
        rdVector_Scale3(&vel, &pThing->orient.lvec, moveDist);
    }

    vel.x = stdMath_ClipNearZero(vel.x);
    vel.y = stdMath_ClipNearZero(vel.y);
    vel.z = stdMath_ClipNearZero(vel.z);

    if ( (pLocal->mode & SITHAI_MODE_UNKNOWN_8000) != 0 && pLocal->goalThing )
    {
        rdVector3 goalDir;
        rdVector_Copy3(&goalDir, &pLocal->goalThing->moveInfo.physics.velocity);
        float goalSpeed = rdVector_Normalize3Acc(&goalDir);

        rdVector3 dir;
        rdVector_Copy3(&dir, &pThing->moveInfo.physics.velocity);
        rdVector_Normalize3Acc(&dir);
        rdVector_Scale3(&pThing->moveInfo.physics.velocity, &dir, goalSpeed);
    }
    else
    {
        rdVector_Add3Acc(&pThing->moveInfo.physics.velocity, &vel);
    }

    if ( sithAIMove_AIGetMoveState(pLocal) == 2 )
    {
        memset(&pLocal->pOwner->moveInfo.physics.velocity, 0, sizeof(rdVector3));
    }

    if ( (pLocal->submode & SITHAI_SUBMODE_UNKNOWN_100) != 0 )
    {
        float speed = rdVector_Len3(&pThing->moveInfo.physics.velocity);
        rdVector_Scale3(&pThing->moveInfo.physics.velocity, &pThing->orient.lvec, speed);
    }

    if ( (pLocal->mode & SITHAI_MODE_NOCHECKFORCLIFF) == 0
        && (pThing->moveInfo.physics.velocity.x != 0.0f || pThing->moveInfo.physics.velocity.y != 0.0f || pThing->moveInfo.physics.velocity.z != 0.0f) )
    {
        if ( (pThing->moveInfo.physics.flags & SITH_PF_FLY) == 0 && pThing->attach.flags )
        {
            bGoalReached = sithAIMove_CheckWalkMove(pLocal, secDeltaTime);
        }
        else if ( (pThing->moveInfo.physics.flags & SITH_PF_FLY) != 0 && (pThing->thingInfo.actorInfo.flags & SITH_AF_BREATHEUNDERWATER) == 0 )
        {
            bGoalReached = sithAIMove_CheckFlyMove(pLocal, secDeltaTime);
        }
        else if ( (pThing->flags & (SITH_TF_AIRDESTROYED | SITH_TF_SUBMERGED)) != 0 )
        {
            bGoalReached = sithAIMove_CheckSwimMove(pLocal, secDeltaTime);
        }

        if ( (pLocal->mode & SITHAI_MODE_MOVING) == 0 )
        {
            return;
        }
    }

    if ( pThing->collide.movesize * 3.0f >= pLocal->moveDistance )
    {
        sithAI_EmitEvent(pLocal, SITHAI_EVENT_GOAL_SET, NULL);
    }

    if ( !bGoalReached && pLocal->moveDistance <= goalDist )
    {
        bGoalReached = 1;
    }

    if ( bGoalReached )
    {
        if ( (pLocal->submode & SITHAI_SUBMODE_UNKNOWN_2) != 0 )
        {
            sithAIMove_StopAIMovement(pLocal);
            sithAIMove_AISetMovePos(pLocal, &pLocal->vecUnknown3, pLocal->moveSpeed);
            return;
        }

        if ( (pLocal->submode & SITHAI_SUBMODE_UNKNOWN_8) != 0 )
        {
            int flags = 0;
            if ( (pLocal->mode & SITHAI_MODE_ACTIVE) != 0 )
            {
                flags |= 0x100;
            }

            if ( sithAIUtil_SweepMoveToPathPos(pLocal, &pLocal->vecUnknown3, 45.0f, 90.0f, 45.0f, flags) )
            {
                return;
            }
        }

        if ( (pLocal->mode & SITHAI_MODE_TRAVERSEWPNTS) != 0 && sithAIUtil_AIAdvanceToNextWpnt(pLocal, 1) )
        {
            return;
        }

        sithAIMove_SetGoalReached(pLocal);
    }
}

void J3DAPI sithAIMove_CheckMoveAlign(SithAIControlBlock* pLocal)
{
    if ( (pLocal->submode & SITHAI_SUBMODE_NOMOVEBACKWARDS) != 0 )
    {
        if ( rdVector_Dot3(&pLocal->goalLVec, &pLocal->pOwner->orient.lvec) >= 0.0f )
        {
            pLocal->submode &= ~SITHAI_SUBMODE_UNKNOWN_10;
        }
    }
    else
    {
        pLocal->submode &= ~SITHAI_SUBMODE_UNKNOWN_10;
    }
}

double J3DAPI sithAIMove_GetGoalTolerance(SithAIControlBlock* pLocal, rdVector3* pPos, rdVector3* pMovePos)
{
    SithThing* pThing = pLocal->pOwner;
    SITH_ASSERTREL(pThing);

    if ( (pThing->pInSector->flags & SITH_SECTOR_UNDERWATER) != 0 || (pThing->moveInfo.physics.flags & SITH_PF_FLY) != 0 )
    {
        return J3DMIN(pThing->collide.movesize, 0.05f);
    }

    if ( (pLocal->mode & SITHAI_MODE_WALLCRAWLING) != 0 )
    {
        int numAligned = 0;
        int bOnSurface = 0;
        float distance = 0.0f;
        float minDist  = J3DMIN(pThing->collide.movesize, 0.03f);
        if ( pThing->moveStatus == SITHPLAYERMOVE_MOUNTING_WALL )
        {
            return minDist;
        }

        rdVector3 delta;
        rdVector_Sub3(&delta, pMovePos, pPos);
        if ( fabsf(delta.x) <= minDist )
        {
            ++numAligned;
            pMovePos->x = pPos->x;
            delta.x = 0.0f;
        }

        if ( fabsf(delta.y) <= minDist )
        {
            ++numAligned;
            pMovePos->y = pPos->y;
            delta.y = 0.0f;
        }

        if ( fabsf(delta.z) <= minDist )
        {
            ++numAligned;
            pMovePos->z = pPos->z;
            delta.z = 0.0f;
        }

        if ( (pThing->attach.flags & SITH_ATTACH_SURFACE) != 0 )
        {
            distance = rdVector_Normalize3Acc(&delta);
            float dot = rdVector_Dot3(&pThing->attach.attachedToStructure.pSurfaceAttached->face.normal, &delta);
            if ( fabsf(dot) < 0.996f )
            {
                bOnSurface = 1;
            }
        }

        if ( !bOnSurface )
        {
            if ( numAligned == 2 || distance <= pThing->collide.movesize * 2.0f )
            {
                rdVector_Copy3(pMovePos, pPos);
            }
            else
            {
                sithAIMove_Unreachable(pLocal);
            }
        }

        return minDist;
    }

    float minDist = J3DMIN(pThing->collide.movesize, 0.03f);
    pPos->z = pThing->pos.z;
    pMovePos->z = pThing->pos.z;
    if ( (pLocal->submode & SITHAI_SUBMODE_UNKNOWN_100) != 0 )
    {
        rdVector3 pos;
        rdVector_ScaleAdd3(&pos, &pThing->orient.rvec, 0.05f, pPos);
        float dist1 = stdMath_Dist2D1(pos.x - pMovePos->x, pos.y - pMovePos->y);

        rdVector3 pos2;
        rdVector_ScaleAdd3(&pos2, &pThing->orient.rvec, -0.05f, pPos);
        float dist2 = stdMath_Dist2D1(pos2.x - pMovePos->x, pos2.y - pMovePos->y);

        if ( dist1 < 0.05f || dist2 < 0.05f )
        {
            rdVector_Copy3(pMovePos, pPos);
        }
    }

    return minDist;
}

int J3DAPI sithAIMove_CheckWalkMove(SithAIControlBlock* pLocal, float secDeltaTime)
{
    rdVector3 endPos;
    rdVector_ScaleAdd3(&endPos, &pLocal->pOwner->moveInfo.physics.velocity, secDeltaTime, &pLocal->pOwner->pos);

    if ( ((pLocal->pOwner->idx + sithMain_g_frameNumber) & 1) != 0 )
    {
        rdVector3 direction;
        float offset;
        if ( (pLocal->mode & SITHAI_MODE_WALLCRAWLING) != 0 )
        {
            sithAIUtil_GetXYZHeadingVector(pLocal->pOwner, &direction);
            offset = pLocal->pOwner->collide.movesize;
            if ( offset - 0.03f > 0.0f )
            {
                offset -= 0.03f;
            }
        }
        else
        {
            sithAIUtil_GetXYHeadingVector(pLocal->pOwner, &direction);
            offset = pLocal->pOwner->collide.movesize + 0.03f;
        }

        rdVector_ScaleAdd3(&endPos, &direction, offset, &endPos);
    }

    switch ( sithAIUtil_CheckPosition(pLocal, &endPos, NULL) )
    {
        case 1:
            break;

        case 0:
        case 8:
            sithAI_EmitEvent(pLocal, SITHAI_EVENT_HIT_CLIFF, NULL);
            break;

        case 2:
            if ( !sithAI_HasInstinct(pLocal, "wallcrawl") )
            {
                sithAI_EmitEvent(pLocal, SITHAI_EVENT_HIT_CLIFF, NULL);
            }

            break;

        case 4:
            if ( (pLocal->submode & SITHAI_SUBMODE_ALLOWSTEPTHING) == 0 )
            {
                sithAI_EmitEvent(pLocal, SITHAI_EVENT_HIT_THING, NULL);
            }

            break;

        default:
            sithAIMove_Unreachable(pLocal);
            break;
    }

    return 0;
}

int J3DAPI sithAIMove_CheckFlyMove(SithAIControlBlock* pLocal, float secDeltaTime)
{
    SithThing* pThing = pLocal->pOwner;
    if ( (pThing->flags & SITH_TF_SUBMERGED) != 0 )
    {
        sithAIMove_StopAIMovement(pLocal);
        pThing->moveInfo.physics.velocity.z += 0.5f;
        pThing->moveStatus = SITHPLAYERMOVE_WALKING;
    }
    else
    {
        rdVector3 endPosition;
        rdVector_ScaleAdd3(&endPosition, &pThing->moveInfo.physics.velocity, secDeltaTime, &pThing->pos);

        rdVector3 curdir;
        sithAIUtil_GetXYHeadingVector(pThing, &curdir);
        rdVector_ScaleAdd3(&endPosition, &curdir, pThing->collide.movesize + 0.05f, &endPosition);

        SithSector* pSector = sithCollision_FindSectorInRadius(pThing->pInSector, &pThing->pos, &endPosition, 0.0f);
        if ( !pSector )
        {
            sithAIMove_Unreachable(pLocal);
            return 1;
        }

        if ( (pSector->flags & SITH_SECTOR_NOACTORENTER) != 0 )
        {
            sithAIMove_Unreachable(pLocal);
            return 1;
        }

        if ( pSector && (pSector->flags & SITH_SECTOR_UNDERWATER) != 0 )
        {
            sithAIMove_StopAIMovement(pLocal);
            pThing->moveStatus = SITHPLAYERMOVE_WALKING;
            pThing->moveInfo.physics.velocity.z += 0.5f;
            return 1;
        }
    }

    return 0;
}

int J3DAPI sithAIMove_CheckSwimMove(SithAIControlBlock* pLocal, float secDeltaTime)
{
    SithThing* pThing = pLocal->pOwner;
    rdVector3 endPosition;
    SithSector* pSector;

    if ( pThing->moveInfo.physics.velocity.z != 0.0f )
    {
        rdVector_Copy3(&endPosition, &pThing->pos);
        if ( (pLocal->submode & SITHAI_SUBMODE_SWIMNEARSURFACE) != 0 )
        {
            endPosition.z = pThing->collide.movesize * 0.2f + endPosition.z;
        }
        else
        {
            endPosition.z = pThing->collide.movesize * 0.5f + endPosition.z;
        }

        pSector = sithCollision_FindSectorInRadius(pThing->pInSector, &pThing->pos, &endPosition, 0.0f);
        if ( pSector && (pSector->flags & SITH_SECTOR_UNDERWATER) == 0 )
        {
            pThing->moveInfo.physics.velocity.z = -0.05f;
            if ( pLocal->movePos.z > pThing->pos.z )
            {
                pLocal->movePos.z = pThing->pos.z - 0.05f;
            }
        }
    }

    rdVector_ScaleAdd3(&endPosition, &pThing->moveInfo.physics.velocity, secDeltaTime, &pThing->pos);

    rdVector3 curdir;
    sithAIUtil_GetXYHeadingVector(pLocal->pOwner, &curdir);
    rdVector_ScaleAdd3(&endPosition, &curdir, pLocal->pOwner->collide.movesize + 0.03f, &endPosition);

    pSector = sithCollision_FindSectorInRadius(pThing->pInSector, &pThing->pos, &endPosition, 0.0f);
    if ( !pSector )
    {
        sithAIMove_Unreachable(pLocal);
        return 0;
    }

    if ( (pSector->flags & SITH_SECTOR_NOACTORENTER) != 0 || (pSector->flags & SITH_SECTOR_UNDERWATER) == 0 )
    {
        sithAIMove_Unreachable(pLocal);
    }

    return 0;
}

int J3DAPI sithAIMove_AIGetMoveState(const SithAIControlBlock* pLocal)
{
    int result;

    switch ( pLocal->pOwner->moveStatus )
    {
        case SITHPLAYERMOVE_MOUNTING_WALL:
        case SITHPLAYERMOVE_TURNING_LEFT_45_DEGREES:
        case SITHPLAYERMOVE_TURNING_RIGHT_45_DEGREES:
        case SITHPLAYERMOVE_TURNING_LEFT_90_DEGREES:
        case SITHPLAYERMOVE_TURNING_RIGHT_90_DEGREES:
        case SITHPLAYERMOVE_TURNING_LEFT_135_DEGREES:
        case SITHPLAYERMOVE_TURNING_RIGHT_135_DEGREES:
        case SITHPLAYERMOVE_TURNING_180_DEGREES:
            result = 2;
            break;

        case SITHPLAYERMOVE_ROLLING_LEFT:
        case SITHPLAYERMOVE_ROLLING_RIGHT:
        case SITHPLAYERMOVE_STRAFING_LEFT:
        case SITHPLAYERMOVE_STRAFING_RIGHT:
        case SITHPLAYERMOVE_SLIDEDOWNFORWARD:
        case SITHPLAYERMOVE_STAND_TO_CRAWL:
        case SITHPLAYERMOVE_CRAWL_TO_STAND:
        case SITHPLAYERMOVE_WALK2STAND:
        case SITHPLAYERMOVE_STAND2WALK:
        case SITHPLAYERMOVE_UNKNOWN_78:
        case SITHPLAYERMOVE_STAND2RUN:
        case SITHPLAYERMOVE_UNKNOWN_80:
        case SITHPLAYERMOVE_UNKNOWN_81:
        case SITHPLAYERMOVE_SLIDEDOWNBACK:
        case SITHPLAYERMOVE_KNOCKEDOUT:
        case SITHPLAYERMOVE_RUNOVER:
            result = 1;
            break;

        default:
            result = 0;
            break;
    }

    return result;
}

int J3DAPI sithAIMove_AISpecialTurn(SithAIControlBlock* pLocal, float angle)
{
    BOOL bTurn90;
    BOOL bTurn135;
    SithActorSpecialMoveFlags moveFlags;
    float normAngle;
    int bTurnSuccess;

    bTurnSuccess = 0;

    if ( (pLocal->submode & SITHAI_SUBMODE_SPECIALTURNS) == 0 )
    {
        return 0;
    }

    if ( sithAIMove_AIGetMoveState(pLocal) )
    {
        return 1;
    }

    if ( angle < 0.0f )
    {
        normAngle = -angle;
    }
    else
    {
        normAngle = angle;
    }

    if ( normAngle < 22.5f )
    {
        return 0;
    }

    if ( angle >= 0.0f )
    {
        moveFlags = SITHACTORSPECIALMOVE_DIR_RIGHT;
    }
    else
    {
        moveFlags = SITHACTORSPECIALMOVE_DIR_LEFT;
    }

    if ( normAngle >= 112.5f && normAngle <= 180.0f )
    {
        bTurn135 = normAngle >= 112.5f && normAngle <= 157.5f;
        if ( bTurn135 && sithAIMove_AISpecialMove(pLocal, (SithActorSpecialMoveFlags)(moveFlags | SITHACTORSPECIALMOVE_TURN135)) )
        {
            bTurnSuccess = 1;
        }

        if ( !bTurnSuccess )
        {
            bTurnSuccess = sithAIMove_AISpecialMove(pLocal, SITHACTORSPECIALMOVE_TURN180) != 0;
        }
    }
    else
    {
        bTurn90 = normAngle >= 67.5f && normAngle <= 112.5f;
        if ( bTurn90 && sithAIMove_AISpecialMove(pLocal, (SithActorSpecialMoveFlags)(moveFlags | SITHACTORSPECIALMOVE_TURN90)) )
        {
            bTurnSuccess = 1;
        }
    }

    if ( !bTurnSuccess )
    {
        return sithAIMove_AISpecialMove(pLocal, (SithActorSpecialMoveFlags)(moveFlags | SITHACTORSPECIALMOVE_TURN45)) != 0;
    }

    return bTurnSuccess;
}

float J3DAPI sithAIMove_UpdateAIMove(SithAIControlBlock* pLocal)
{
    int v2;
    float x;
    float v4;
    float v5;
    float v6;
    float y;
    float z;
    float absVelRight;
    float absVelFwd;
    float absVelUp;
    SithThing* pThing;
    rdVector3 velocity;
    SithPuppetSubMode newSubmode;
    SithThingMoveStatus newMoveStatus;
    float moveSpeed;
    SithPhysicsInfo* pPhysics;
    float minFlyMoveSpeed;
    int axis;

    pPhysics = (SithPhysicsInfo*)&pLocal->pOwner->moveInfo;
    pThing = pLocal->pOwner;
    moveSpeed = 0.0f;
    newSubmode = SITHPUPPETSUBMODE_STAND;
    newMoveStatus = SITHPLAYERMOVE_STILL;
    minFlyMoveSpeed = 0.0099999998f;

    SITH_ASSERTREL(pThing);
    if ( pThing->pInSector
        && (pPhysics->velocity.x != 0.0f || pPhysics->velocity.y != 0.0f || pPhysics->velocity.z != 0.0f || (pLocal->mode & SITHAI_MODE_TURNING) != 0)
        && (pThing->thingInfo.actorInfo.flags & SITH_AF_IMMOBILE) == 0 )
    {
        // Transform to world velocity
        rdMatrix_TransformVectorOrtho34(&velocity, &pPhysics->velocity, &pThing->orient);

        if ( (pPhysics->flags & SITH_PF_FLY) != 0
            && (velocity.z >= 0.0f ? (absVelUp = velocity.z) : (absVelUp = -velocity.z),
                velocity.y >= 0.0f ? (absVelFwd = velocity.y) : (absVelFwd = -velocity.y),
                absVelUp > (double)absVelFwd
                && (velocity.z >= 0.0f ? (absVelUp = velocity.z) : (absVelUp = -velocity.z),
                    velocity.x >= 0.0f ? (absVelRight = velocity.x) : (absVelRight = -velocity.x),
                    absVelUp > (double)absVelRight)) )
        {
            moveSpeed = velocity.z;
            if ( velocity.z > (double)minFlyMoveSpeed )
            {
                newMoveStatus = SITHPLAYERMOVE_UNKNOWN_103;
                newSubmode = SITHPUPPETSUBMODE_RISING;
            }

            else if ( -minFlyMoveSpeed > moveSpeed )
            {
                newMoveStatus = SITHPLAYERMOVE_UNKNOWN_104;
                newSubmode = SITHPUPPETSUBMODE_FALLFORWARD;
            }
        }
        else                                    // not flying
        {
            if ( velocity.z >= 0.0f )
            {
                z = velocity.z;
            }
            else
            {
                z = -velocity.z;
            }

            if ( velocity.y >= 0.0f )
            {
                y = velocity.y;
            }
            else
            {
                y = -velocity.y;
            }

            if ( z > (double)y
                && (velocity.z >= 0.0f ? (v6 = velocity.z) : (v6 = -velocity.z), velocity.x >= 0.0f ? (v5 = velocity.x) : (v5 = -velocity.x), v6 > (double)v5) )
            {
                moveSpeed = velocity.z;
                axis = 2;
            }
            else
            {
                if ( velocity.y < 0.0f )
                {
                    v4 = -velocity.y;
                }
                else
                {
                    v4 = velocity.y;
                }

                if ( velocity.x < 0.0f )
                {
                    x = -velocity.x;
                }
                else
                {
                    x = velocity.x;
                }

                if ( v4 <= (double)x )
                {
                    moveSpeed = velocity.x;
                    axis = 0;
                }
                else
                {
                    moveSpeed = velocity.y;
                    axis = 1;
                }
            }

            sithAIMove_GetAIMoveModes(pThing, moveSpeed, axis, &newMoveStatus, &newSubmode);
        }
    }

    if ( pThing->thingInfo.actorInfo.bControlsDisabled || pThing->thingInfo.actorInfo.bForceMovePlay )
    {
        return moveSpeed;
    }

    v2 = sithAIMove_AIGetMoveState(pLocal);
    switch ( v2 )
    {
        case 0:                                 // turning
            break;

        case 1:                                 // moving
            return moveSpeed;

        case 2:
            return 0.0f;
    }

    if ( pThing->pPuppetState->submode != newSubmode )
    {
        sithAIMove_SetSubMode(pLocal, newMoveStatus, newSubmode);
    }

    return moveSpeed;
}

void J3DAPI sithAIMove_GetAIMoveModes(const SithThing* pThing, float moveSpeed, int axis, SithThingMoveStatus* pOutMoveStatus, SithPuppetSubMode* pOutSubmode)
{
    float v5;
    float v6;
    float v7;
    float v8;
    float absMoveSpeed;
    float maxTurnSpeed;
    SithAIControlBlock* pLocal;
    float minWalkSpeed;
    float maxWalkSpeed;

    pLocal = pThing->controlInfo.aiControl.pLocal;

    minWalkSpeed = 0.001f;
    maxWalkSpeed = 0.27000001f;
    maxTurnSpeed = 0.0049999999f;

    SITH_ASSERTREL(pLocal);
    if ( (pThing->thingInfo.actorInfo.flags & SITH_AF_FASTMOVE15) != 0 )
    {
        minWalkSpeed = sithAIMove_minWalkSpeed * 1.15f;
        maxWalkSpeed = sithAIMove_maxWalkSpeed * 1.15f;
    }

    else if ( (pThing->thingInfo.actorInfo.flags & SITH_AF_FASTMOVE10) != 0 )
    {
        minWalkSpeed = sithAIMove_minWalkSpeed * 1.1f;
        maxWalkSpeed = sithAIMove_maxWalkSpeed * 1.1f;
    }

    if ( (pLocal->mode & SITHAI_MODE_TURNING) == 0 )
    {
        goto LABEL_25;
    }

    if ( (pThing->thingInfo.actorInfo.flags & SITH_AF_FLYERMOVE) != 0 )
    {
        if ( pThing->moveStatus == SITHPLAYERMOVE_UNKNOWN_105 )
        {
            *pOutMoveStatus = SITHPLAYERMOVE_TURNING_LEFT;
            *pOutSubmode = SITHPUPPETSUBMODE_TURNLEFT;
            return;
        }

        if ( pThing->moveStatus == SITHPLAYERMOVE_UNKNOWN_106 )
        {
            *pOutMoveStatus = SITHPLAYERMOVE_TURNING_RIGHT;
            *pOutSubmode = SITHPUPPETSUBMODE_TURNRIGHT;
            return;
        }
    }

    if ( (pThing->thingInfo.actorInfo.flags & SITH_AF_FASTMOVE15) != 0 )
    {
        maxTurnSpeed = sithAIMove_maxTurnSpeed * 1.15f;
    }

    else if ( (pThing->thingInfo.actorInfo.flags & SITH_AF_FASTMOVE10) != 0 )
    {
        maxTurnSpeed = sithAIMove_maxTurnSpeed * 1.1f;
    }

    absMoveSpeed = moveSpeed >= 0.0f ? moveSpeed : -moveSpeed;
    if ( absMoveSpeed < (double)maxTurnSpeed )
    {
        if ( pThing->moveStatus == SITHPLAYERMOVE_TURNING_LEFT || pThing->moveStatus == SITHPLAYERMOVE_UNKNOWN_105 )
        {
            *pOutMoveStatus = SITHPLAYERMOVE_TURNING_LEFT;
            *pOutSubmode = SITHPUPPETSUBMODE_TURNLEFT;
        }
        else
        {
            *pOutMoveStatus = SITHPLAYERMOVE_TURNING_RIGHT;
            *pOutSubmode = SITHPUPPETSUBMODE_TURNRIGHT;
        }
    }
    else
    {
    LABEL_25:
        if ( !axis ) // x - right
        {
            if ( moveSpeed < 0.0f )
            {
                v8 = -moveSpeed;
            }
            else
            {
                v8 = moveSpeed;
            }

            if ( v8 >= (double)maxWalkSpeed )
            {
                *pOutMoveStatus = SITHPLAYERMOVE_RUNNING;
                *pOutSubmode = SITHPUPPETSUBMODE_RUN;
            }
            else
            {
                if ( moveSpeed < 0.0f )
                {
                    v7 = -moveSpeed;
                }
                else
                {
                    v7 = moveSpeed;
                }

                if ( v7 >= (double)minWalkSpeed && (moveSpeed >= 0.0f ? (v6 = moveSpeed) : (v6 = -moveSpeed), v6 < (double)maxWalkSpeed) )
                {
                    *pOutMoveStatus = SITHPLAYERMOVE_WALKING;
                    *pOutSubmode = SITHPUPPETSUBMODE_WALK;
                }
                else
                {
                    *pOutMoveStatus = SITHPLAYERMOVE_STILL;
                    *pOutSubmode = SITHPUPPETSUBMODE_STAND;
                }
            }
        }
        else
        {
            if ( axis == 1 ) // y - forward
            {
                goto LABEL_54;
            }

            if ( axis != 2 ) // z - up
            {
                *pOutMoveStatus = SITHPLAYERMOVE_STILL;
                *pOutSubmode = SITHPUPPETSUBMODE_STAND;
                return;
            }

            // axis should be z - up
            if ( !pThing->attach.flags )
            {
                if ( -minWalkSpeed <= moveSpeed && moveSpeed <= (double)minWalkSpeed )
                {
                    *pOutMoveStatus = SITHPLAYERMOVE_STILL;
                    *pOutSubmode = SITHPUPPETSUBMODE_STAND;
                }
                else
                {
                    if ( moveSpeed >= 0.0f )
                    {
                        v5 = moveSpeed;
                    }
                    else
                    {
                        v5 = -moveSpeed;
                    }

                    if ( v5 > (double)minWalkSpeed )
                    {
                        *pOutMoveStatus = SITHPLAYERMOVE_FALLING;
                        *pOutSubmode = SITHPUPPETSUBMODE_FALL;
                    }
                }
            }
            else
            {
            LABEL_54:
                if ( moveSpeed >= (double)maxWalkSpeed )
                {
                    *pOutMoveStatus = SITHPLAYERMOVE_RUNNING;
                    *pOutSubmode = SITHPUPPETSUBMODE_RUN;
                }

                else if ( moveSpeed >= (double)minWalkSpeed && moveSpeed < (double)maxWalkSpeed )
                {
                    *pOutMoveStatus = SITHPLAYERMOVE_WALKING;
                    *pOutSubmode = SITHPUPPETSUBMODE_WALK;
                }

                else if ( -minWalkSpeed < moveSpeed && moveSpeed < (double)minWalkSpeed )
                {
                    *pOutMoveStatus = SITHPLAYERMOVE_STILL;
                    *pOutSubmode = SITHPUPPETSUBMODE_STAND;
                }

                else if ( -minWalkSpeed >= moveSpeed )
                {
                    *pOutMoveStatus = SITHPLAYERMOVE_WALKING;
                    *pOutSubmode = SITHPUPPETSUBMODE_WALKBACK;
                }
            }
        }
    }
}

void J3DAPI sithAIMove_SetSubMode(SithAIControlBlock* pLocal, SithThingMoveStatus moveStatus, SithPuppetSubMode submode)
{
    SithThing* pThing;
    const rdKeyframe* pKeyframe;
    int majorMode;
    void (J3DAPI * pCallback)(SithThing*, int, rdKeyMarkerType);
    SithPuppetTrack* pTrack;
    SithPuppetSubMode bdSubmode;
    SithThingMoveStatus bdModeStatus;
    SithPuppetSubMode curSubmode;


    pCallback    = NULL;
    pThing       = pLocal->pOwner;
    bdSubmode    = 0;
    bdModeStatus = 0;

    if ( !pLocal->pOwner || !pThing->pPuppetClass || !pThing->pPuppetState )
    {
        SITHLOG_ERROR("sithAIMove_SetSubMode: Bad puppet data for AI '%s'.\n", pThing->aName); // TODO: potential null pointer dereference, pThing can be null here
    }
    else
    {
        majorMode  = pThing->pPuppetState->majorMode;
        curSubmode = pThing->pPuppetState->submode;

        switch ( curSubmode )
        {
            case SITHPUPPETSUBMODE_STAND:
            case SITHPUPPETSUBMODE_TURNLEFT:
            case SITHPUPPETSUBMODE_TURNRIGHT:
                if ( (unsigned int)submode >= SITHPUPPETSUBMODE_WALK && (unsigned int)submode <= SITHPUPPETSUBMODE_WALKBACK )
                {
                    bdSubmode = SITHPUPPETSUBMODE_STAND2WALK;
                    bdModeStatus = SITHPLAYERMOVE_STAND2WALK;
                }

                break;

            case SITHPUPPETSUBMODE_WALK:
                if ( submode == SITHPUPPETSUBMODE_STAND
                    || (unsigned int)submode > SITHPUPPETSUBMODE_STRAFERIGHT && (unsigned int)submode <= SITHPUPPETSUBMODE_TURNRIGHT )
                {
                    bdSubmode = SITHPUPPETSUBMODE_WALK2STAND;
                    bdModeStatus = SITHPLAYERMOVE_WALK2STAND;
                }

                break;

            case SITHPUPPETSUBMODE_RUN:
                if ( submode == SITHPUPPETSUBMODE_STAND
                    || (unsigned int)submode > SITHPUPPETSUBMODE_STRAFERIGHT && (unsigned int)submode <= SITHPUPPETSUBMODE_TURNRIGHT )
                {
                    bdSubmode = SITHPUPPETSUBMODE_WALK2STAND;
                    bdModeStatus = SITHPLAYERMOVE_WALK2STAND;
                }

                break;

            case SITHPUPPETSUBMODE_FALL:
                if ( submode == SITHPUPPETSUBMODE_STAND && sithPuppet_PlayMode(pThing, SITHPUPPETSUBMODE_LAND, sithAIMove_PuppetCallback) > -1 )
                {
                    moveStatus = SITHPLAYERMOVE_LAND;
                }

                break;

            default:
                break;
        }

        // Play transition animation
        if ( bdSubmode )
        {
            if ( pThing->pPuppetClass->aModes[majorMode][bdSubmode].pKeyframe )
            {
                sithPuppet_ClearMode(pThing, curSubmode);

                moveStatus = bdModeStatus;
                submode    = bdSubmode;
                pCallback = sithAIMove_PuppetCallback;

                if ( bdSubmode == SITHPUPPETSUBMODE_STAND2WALK )
                {
                    memset(&pThing->moveInfo.physics.velocity, 0, sizeof(pThing->moveInfo.physics.velocity));
                    sithSoundClass_PlayModeFirst(pThing, SITHSOUNDCLASS_STAND2WALK);
                }
                else
                {
                    sithSoundClass_PlayModeFirst(pThing, SITHSOUNDCLASS_WALK2STAND);
                }
            }
        }

        pKeyframe = pThing->pPuppetClass->aModes[majorMode][submode].pKeyframe;
        if ( pKeyframe )
        {
            pTrack = sithPuppet_FindActiveTrack(pThing, pKeyframe);
            if ( pTrack )
            {
                sithPuppet_SynchMode(pThing, pTrack->submode, submode, -1.0f, 0);
            }
            else
            {
                sithPuppet_SetSubMode(pThing, submode, pCallback);
            }
        }

        pThing->pPuppetState->submode = submode;
        pThing->moveStatus = moveStatus;
    }
}

void J3DAPI sithAIMove_PuppetCallback(SithThing* pThing, int trackNum, rdKeyMarkerType marker)
{
    SithAIMode mode;
    float a;
    float aa;
    SithThingMoveStatus moveStatus;
    SithPhysicsInfo* pPhysics;
    SithAIControlBlock* pLocal;

    pPhysics = (SithPhysicsInfo*)&pThing->moveInfo;
    SITH_ASSERTREL(pThing);
    pLocal = pThing->controlInfo.aiControl.pLocal;
    if ( !pLocal )
    {
    LABEL_48:
        sithPuppet_DefaultCallback(pThing, trackNum, marker);
    }
    else
    {
        switch ( marker )
        {
            case 0:
                sithPuppet_FreeTrackByIndex(pThing, trackNum);
                if ( (pThing->flags & (SITH_TF_DYING | SITH_TF_DESTROYED)) == 0 )
                {
                    switch ( pThing->moveStatus )
                    {
                        case SITHPLAYERMOVE_LAND:
                            pThing->moveStatus = SITHPLAYERMOVE_STILL;
                            goto LABEL_29;

                        case SITHPLAYERMOVE_MOUNTING_WALL:
                            sithAIMove_AIFinalizeSpecialMove(pLocal, SITHACTORSPECIALMOVE_MOUNTWALL);
                            pThing->moveStatus = SITHPLAYERMOVE_WALKING;
                            goto LABEL_29;

                        case SITHPLAYERMOVE_ROLLING_LEFT:
                        case SITHPLAYERMOVE_ROLLING_RIGHT:
                            sithAIMove_AIFinalizeSpecialMove(pLocal, SITHACTORSPECIALMOVE_ROLL);
                            pThing->moveStatus = SITHPLAYERMOVE_STILL;
                            sithAIMove_SetGoalReached(pLocal);
                            goto LABEL_29;

                        case SITHPLAYERMOVE_STRAFING_LEFT:
                        case SITHPLAYERMOVE_STRAFING_RIGHT:
                            pThing->moveStatus = SITHPLAYERMOVE_STILL;
                            sithAIMove_SetGoalReached(pLocal);
                            goto LABEL_29;

                        case SITHPLAYERMOVE_WALK2STAND:
                        case SITHPLAYERMOVE_UNKNOWN_78:
                            pThing->moveStatus = SITHPLAYERMOVE_STILL;
                            goto LABEL_29;

                        case SITHPLAYERMOVE_STAND2WALK:
                        case SITHPLAYERMOVE_STAND2RUN:
                            if ( pThing->moveStatus == SITHPLAYERMOVE_STAND2WALK )
                            {
                                pThing->moveStatus = SITHPLAYERMOVE_WALKING;
                            }
                            else
                            {
                                pThing->moveStatus = SITHPLAYERMOVE_RUNNING;
                            }

                            goto LABEL_29;

                        case SITHPLAYERMOVE_KNOCKEDOUT:
                            pThing->thingInfo.actorInfo.bControlsDisabled = 0;
                            pThing->moveStatus = SITHPLAYERMOVE_STILL;
                            pThing->collide.type = SITH_COLLIDE_FACE;
                            mode = pThing->controlInfo.aiControl.pLocal->mode;
                            mode &= ~0x2000;// 0x2000
                            pThing->controlInfo.aiControl.pLocal->mode = mode;
                            pThing->thingInfo.actorInfo.flags &= ~(SITH_AF_IMMOBILE | SITH_AF_INVULNERABLE);
                            goto LABEL_29;

                        case SITHPLAYERMOVE_TURNING_LEFT_45_DEGREES:
                        case SITHPLAYERMOVE_TURNING_RIGHT_45_DEGREES:
                            sithAIMove_AIFinalizeSpecialMove(pLocal, SITHACTORSPECIALMOVE_TURN45);
                            pThing->moveStatus = SITHPLAYERMOVE_WALKING;
                            goto LABEL_29;

                        case SITHPLAYERMOVE_TURNING_LEFT_90_DEGREES:
                        case SITHPLAYERMOVE_TURNING_RIGHT_90_DEGREES:
                            sithAIMove_AIFinalizeSpecialMove(pLocal, SITHACTORSPECIALMOVE_TURN90);
                            pThing->moveStatus = SITHPLAYERMOVE_WALKING;
                            goto LABEL_29;

                        case SITHPLAYERMOVE_TURNING_LEFT_135_DEGREES:
                        case SITHPLAYERMOVE_TURNING_RIGHT_135_DEGREES:
                            sithAIMove_AIFinalizeSpecialMove(pLocal, SITHACTORSPECIALMOVE_TURN135);
                            pThing->moveStatus = SITHPLAYERMOVE_WALKING;
                            goto LABEL_29;

                        case SITHPLAYERMOVE_TURNING_180_DEGREES:
                            sithAIMove_AIFinalizeSpecialMove(pLocal, SITHACTORSPECIALMOVE_TURN180);
                            pThing->moveStatus = SITHPLAYERMOVE_WALKING;

                        LABEL_29:
                            moveStatus = pThing->moveStatus;
                            switch ( moveStatus )
                            {
                                case SITHPLAYERMOVE_STILL:
                                    sithPuppet_SetSubMode(pThing, SITHPUPPETSUBMODE_STAND, 0);
                                    break;

                                case SITHPLAYERMOVE_WALKING:
                                    if ( pThing->moveInfo.physics.velocity.x == 0.0f
                                        && pThing->moveInfo.physics.velocity.y == 0.0f
                                        && pThing->moveInfo.physics.velocity.z == 0.0f )
                                    {
                                        pThing->moveInfo.physics.velocity.y = 0.059999999f;
                                    }
                                    else
                                    {
                                        a = rdVector_Dot3(&pPhysics->velocity, &pPhysics->velocity);
                                        if ( sqrtf(a) < 0.059999999f )
                                        {
                                            pThing->moveInfo.physics.velocity.x = pThing->moveInfo.physics.velocity.x * 1.0599999f;
                                            pThing->moveInfo.physics.velocity.y = pThing->moveInfo.physics.velocity.y * 1.0599999f;
                                            pThing->moveInfo.physics.velocity.z = pThing->moveInfo.physics.velocity.z * 1.0599999f;
                                        }
                                    }

                                    sithPuppet_SetSubMode(pThing, SITHPUPPETSUBMODE_WALK, 0);
                                    break;

                                case SITHPLAYERMOVE_RUNNING:
                                    if ( pThing->moveInfo.physics.velocity.x == 0.0f
                                        && pThing->moveInfo.physics.velocity.y == 0.0f
                                        && pThing->moveInfo.physics.velocity.z == 0.0f )
                                    {
                                        pThing->moveInfo.physics.velocity.y = 0.27000001f;
                                    }

                                    aa = rdVector_Dot3(&pPhysics->velocity, &pPhysics->velocity);
                                    if ( sqrtf(aa) < 0.27000001f )
                                    {
                                        pThing->moveInfo.physics.velocity.x = pThing->moveInfo.physics.velocity.x * 1.27f;
                                        pThing->moveInfo.physics.velocity.y = pThing->moveInfo.physics.velocity.y * 1.27f;
                                        pThing->moveInfo.physics.velocity.z = pThing->moveInfo.physics.velocity.z * 1.27f;
                                    }

                                    sithPuppet_SetSubMode(pThing, SITHPUPPETSUBMODE_RUN, 0);
                                    break;
                            }

                            break;

                        default:
                            return;
                    }
                }

                return;

            case RDKEYMARKER_LEFTFOOT:
                if ( pThing->moveStatus != SITHPLAYERMOVE_STRAFING_RIGHT )
                {
                    goto LABEL_48;
                }

                goto LABEL_6;

            case RDKEYMARKER_RIGHTFOOT:
                if ( pThing->moveStatus != SITHPLAYERMOVE_STRAFING_LEFT )
                {
                    goto LABEL_48;
                }

            LABEL_6:
                sithAIMove_AIFinalizeSpecialMove(pLocal, SITHACTORSPECIALMOVE_STRAFE);
                break;

            case RDKEYMARKER_DIED:
                switch ( pThing->moveStatus )
                {
                    case SITHPLAYERMOVE_ROLLING_LEFT:
                    case SITHPLAYERMOVE_ROLLING_RIGHT:
                        sithSoundClass_PlayModeRandom(pThing, SITHSOUNDCLASS_RESERVED3);
                        break;

                    case SITHPLAYERMOVE_KNOCKEDOUT:
                        sithSoundClass_PlayModeRandom(pThing, SITHSOUNDCLASS_RESERVED1);
                        break;

                    case SITHPLAYERMOVE_RUNOVER:
                        sithSoundClass_PlayModeRandom(pThing, SITHSOUNDCLASS_RESERVED2);
                        break;

                    default:
                        goto LABEL_48;
                }

                break;

            default:
                goto LABEL_48;
        }
    }
}

void J3DAPI sithAIMove_AISetLookThing(SithAIControlBlock* pLocal, const SithThing* pThing)
{
    rdVector3 eyePos;
    rdVector3 dest;

    SITH_ASSERTREL(pLocal && pLocal->pOwner && pLocal->pClass);
    SITH_ASSERTREL(pThing);
    if ( pThing->type == SITH_THING_ACTOR || pThing->type == SITH_THING_PLAYER )
    {
        rdMatrix_TransformVector34(&dest, &pThing->thingInfo.actorInfo.eyeOffset, &pThing->orient);

        eyePos.x = pThing->pos.x + dest.x;
        eyePos.y = pThing->pos.y + dest.y;
        eyePos.z = pThing->pos.z + dest.z;
    }
    else
    {
        memcpy(&eyePos, &pThing->pos, sizeof(eyePos));
    }

    sithAIMove_AISetLookPos(pLocal, &eyePos);
}

void J3DAPI sithAIMove_AISetLookPos(SithAIControlBlock* pLocal, const rdVector3* targetPos)
{
    double dist;
    SithThing* pOwner;
    float v5;
    rdVector3 eyePos;
    SithActorInfo* pActor;

    SITH_ASSERTREL(pLocal && pLocal->pOwner && pLocal->pClass);
    SITH_ASSERTREL(targetPos);
    if ( (pLocal->submode & SITHAI_SUBMODE_UNKNOWN_1) == 0 )
    {
        if ( (pLocal->mode & SITHAI_MODE_DISABLED) != 0 && (sithMain_g_sith_mode.debugModeFlags & SITHDEBUG_INEDITOR) == 0 )
        {
            SITHLOG_ERROR("WARNING!  Look target issued to '%s'. AI is DISABLED.\n", pLocal->pOwner->aName);
        }

        pOwner = pLocal->pOwner;
        pActor = &pLocal->pOwner->thingInfo.actorInfo;
        memcpy(&eyePos, &pOwner->pos, sizeof(eyePos));
        if ( pOwner->type == SITH_THING_ACTOR || pOwner->type == SITH_THING_PLAYER )
        {
            eyePos.z = eyePos.z + pActor->eyeOffset.z;
        }

        pLocal->goalLVec.x = targetPos->x - eyePos.x;
        pLocal->goalLVec.y = targetPos->y - eyePos.y;
        pLocal->goalLVec.z = targetPos->z - eyePos.z;
        dist = rdVector_Normalize3Acc(&pLocal->goalLVec);

        v5 = dist;
        if ( _isnan(dist) || v5 < 0.001f )
        {
            memcpy(&pLocal->goalLVec, &pOwner->orient.lvec, sizeof(pLocal->goalLVec));
            v5 = 0.0f;
        }

        memcpy(&pLocal->lookPos, targetPos, sizeof(pLocal->lookPos));
        if ( v5 != 0.0f )
        {
            pLocal->mode |= SITHAI_MODE_TURNING;
            pLocal->submode |= SITHAI_SUBMODE_UNKNOWN_10;
        }

        pLocal->mode |= SITHAI_MODE_UNKNOWN_80;
        if ( (pLocal->mode & SITHAI_MODE_WANTALLEVENTS) != 0 )
        {
            sithAI_EmitEvent(pLocal, SITHAI_EVENT_UNKNOWN_40000, 0);
        }
    }
}

void J3DAPI sithAIMove_AISetLookPosEyeLevel(SithAIControlBlock* pLocal, const rdVector3* targetPos)
{
    rdVector3 eyePos;
    SithThing* pOwner;
    rdVector3 lookPos;
    float dist;

    pOwner = pLocal->pOwner;

    memcpy(&lookPos, targetPos, sizeof(lookPos));
    eyePos.x = 0.0f;
    eyePos.y = 0.0f;
    eyePos.z = pOwner->thingInfo.actorInfo.eyeOffset.z;
    rdMatrix_TransformVector34Acc(&eyePos, &pOwner->orient);

    eyePos.x = eyePos.x + pOwner->pos.x;
    eyePos.y = eyePos.y + pOwner->pos.y;
    eyePos.z = eyePos.z + pOwner->pos.z;

    // rdMath_DistPointToPlane
    dist = (lookPos.x - eyePos.x) * pOwner->orient.uvec.x + (lookPos.y - eyePos.y) * pOwner->orient.uvec.y + (lookPos.z - eyePos.z) * pOwner->orient.uvec.z;
    lookPos.x = pOwner->orient.uvec.x * -dist + lookPos.x;
    lookPos.y = pOwner->orient.uvec.y * -dist + lookPos.y;
    lookPos.z = pOwner->orient.uvec.z * -dist + lookPos.z;
    sithAIMove_AISetLookPos(pLocal, &lookPos);
}

int J3DAPI sithAIMove_AISetMovePos(SithAIControlBlock* pLocal, const rdVector3* moveToPos, float moveSpeed)
{
    SITH_ASSERTREL(pLocal && pLocal->pOwner && pLocal->pClass);
    SITH_ASSERTREL(moveToPos);

    if ( (pLocal->submode & SITHAI_SUBMODE_UNKNOWN_1) != 0 )
    {
        return 0;
    }

    if ( (pLocal->mode & SITHAI_MODE_DISABLED) != 0 && (sithMain_g_sith_mode.debugModeFlags & SITHDEBUG_INEDITOR) == 0 )
    {
        SITHLOG_ERROR("WARNING!  Move target issued to '%s'. AI is DISABLED.\n", pLocal->pOwner->aName);
    }

    rdVector3 newPos = *moveToPos;
    rdVector3 curPos = pLocal->pOwner->pos;

    if ( pLocal->maxHomeDist > 0.0f )
    {
        rdVector3 dir;
        dir.x = moveToPos->x - pLocal->homePos.x;
        dir.y = moveToPos->y - pLocal->homePos.y;
        dir.z = moveToPos->z - pLocal->homePos.z;
        float dist = rdVector_Normalize3Acc(&dir);
        if ( dist > (double)pLocal->maxHomeDist )
        {
            newPos.x = pLocal->maxHomeDist * dir.x + pLocal->homePos.x;
            newPos.y = pLocal->maxHomeDist * dir.y + pLocal->homePos.y;
            newPos.z = pLocal->maxHomeDist * dir.z + pLocal->homePos.z;
        }
    }

    float moveSize = sithAIMove_GetGoalTolerance(pLocal, &curPos, &newPos);
    if ( rdVector_Dist3(moveToPos, &pLocal->pOwner->pos) <= moveSize )
    {
        return 0;
    }

    pLocal->moveSpeed = moveSpeed;
    memcpy(&pLocal->movePos, moveToPos, sizeof(pLocal->movePos));
    memcpy(&pLocal->vecUnknown, &pLocal->pOwner->pos, sizeof(pLocal->vecUnknown));

    sithSoundClass_PlayModeFirst(pLocal->pOwner, SITHSOUNDCLASS_MOVING);
    sithAIUtil_AISetMode(pLocal, SITHAI_MODE_MOVING);

    if ( (pLocal->mode & SITHAI_MODE_WANTALLEVENTS) != 0 )
    {
        sithAI_EmitEvent(pLocal, SITHAI_EVENT_MOVE_TARGET_CHANGED, NULL);
    }

    // Found in debug version
#ifdef J3D_DEBUG
    // TODO: Find a way to null the 2 pointers on level close to prevent crashes due to dangling pointers
    if ( sithAIMove_pMoveToDebugMarkThing )
    {
        sithThing_RemoveThing(sithWorld_g_pCurrentWorld, sithAIMove_pMoveToDebugMarkThing);
        sithAIMove_pMoveToDebugMarkThing = NULL;
    }

    if ( sithAIMove_pMoveToPosGroundMarkThing )
    {
        sithThing_RemoveThing(sithWorld_g_pCurrentWorld, sithAIMove_pMoveToPosGroundMarkThing);
        sithAIMove_pMoveToPosGroundMarkThing = NULL;
    }

    if ( !sithAIMove_bDebugMoveToPo )
    {
        return 1;
    }

    SithThing* pMarkTpl = sithTemplate_GetTemplate("+x_mark");
    if ( !pMarkTpl )
    {
        return 1;
    }

    rdVector3 look = rdroid_g_zVector3;
    rdMatrix34 markOrient;
    rdMatrix_BuildFromLook34(&markOrient, &look);

    rdVector3 markPos = *moveToPos;
    markPos.z += 0.001f; // Added: z offset
    sithAIMove_pMoveToDebugMarkThing = sithThing_CreateThingAtPos(pMarkTpl, &markPos, &markOrient, pLocal->pOwner->pInSector, NULL);

    float thingHeight = sithPhysics_GetThingHeight(pLocal->pOwner);
    markPos.z = markPos.z - thingHeight + 0.001f;
    sithAIMove_pMoveToPosGroundMarkThing = sithThing_CreateThingAtPos(pMarkTpl, &markPos, &markOrient, pLocal->pOwner->pInSector, NULL);
#endif

    return 1;
}

int J3DAPI sithAIMove_AISetMoveTargetPos(SithAIControlBlock* pLocal, const rdVector3* moveToPos, float moveSpeed)
{
    SITH_ASSERTREL(pLocal);
    SITH_ASSERTREL(moveToPos);
    if ( (pLocal->mode & SITHAI_MODE_DISABLED) != 0 && (sithMain_g_sith_mode.debugModeFlags & SITHDEBUG_INEDITOR) == 0 )
    {
        SITHLOG_ERROR("WARNING!  Move target issued to '%s'. AI is DISABLED.\n", pLocal->pOwner->aName);
    }

    pLocal->submode &= ~SITHAI_SUBMODE_UNKNOWN_1;
    if ( !sithAIMove_AISetMovePos(pLocal, moveToPos, moveSpeed) )
    {
        return 0;
    }

    pLocal->submode |= SITHAI_SUBMODE_UNKNOWN_1;
    return 1;
}

int J3DAPI sithAIMove_AISetMoveTargetPos2(SithAIControlBlock* pLocal, const rdVector3* moveToPos, float moveSpeed)
{
    SITH_ASSERTREL(pLocal);
    SITH_ASSERTREL(moveToPos);
    if ( (pLocal->submode & SITHAI_SUBMODE_UNKNOWN_1) != 0 )
    {
        return 0;
    }

    if ( (pLocal->mode & SITHAI_MODE_DISABLED) != 0 && (sithMain_g_sith_mode.debugModeFlags & SITHDEBUG_INEDITOR) == 0 )
    {
        SITHLOG_ERROR("WARNING!  Move target issued to '%s'. AI is DISABLED.\n", pLocal->pOwner->aName);
    }

    if ( (pLocal->submode & SITHAI_SUBMODE_UNKNOWN_2) != 0 )
    {
        return 0;
    }

    memcpy(&pLocal->vecUnknown3, &pLocal->movePos, sizeof(pLocal->vecUnknown3));
    if ( !sithAIMove_AISetMoveTargetPos(pLocal, moveToPos, moveSpeed) )
    {
        return 0;
    }

    pLocal->submode |= SITHAI_SUBMODE_UNKNOWN_2;
    return 1;
}

int J3DAPI sithAIMove_AISpecialMove(SithAIControlBlock* pLocal, SithActorSpecialMoveFlags moveFlags)
{
    double v3;
    double v4;
    float v5;
    BOOL v6;
    float v7;
    float v8;
    BOOL v9;
    float v10;
    float v11;
    float v12;
    float v13;
    float v14;
    SithThing* pThing;
    SithPuppetSubMode rightTurnMode;
    BOOL v17;
    rdVector3 pHitNorm;
    int v19;
    SithPuppetSubMode leftTurnMode;
    int v21;

    rdVector3 newUVec;
    rdVector3 endPos;
    int v25;

    rdVector3 pTarget;

    v17 = 0;

    SITH_ASSERTREL(pLocal && pLocal->pOwner);

    pThing = pLocal->pOwner;

    rightTurnMode = 0;
    if ( (moveFlags & SITHACTORSPECIALMOVE_ROLL) != 0 )
    {
        if ( rdVector_Dot3(&pThing->orient.lvec, &pLocal->toTarget) <= 0.40000001f )
        {
            return 0;
        }

        leftTurnMode = SITHPUPPETSUBMODE_HOPLEFT;
        rightTurnMode = SITHPUPPETSUBMODE_HOPRIGHT;
    }
    else if ( (moveFlags & SITHACTORSPECIALMOVE_STRAFE) != 0 )
    {
        if ( rdVector_Dot3(&pThing->orient.lvec, &pLocal->toTarget) <= 0.40000001f )
        {
            return 0;
        }

        leftTurnMode = SITHPUPPETSUBMODE_STRAFELEFT;
        rightTurnMode = SITHPUPPETSUBMODE_STRAFERIGHT;
    }
    else if ( (moveFlags & SITHACTORSPECIALMOVE_TURN45) != 0 )
    {
        leftTurnMode = SITHPUPPETSUBMODE_WHIPCLIMBUP;
        rightTurnMode = SITHPUPPETSUBMODE_WHIPCLIMBDOWN;
    }
    else if ( (moveFlags & SITHACTORSPECIALMOVE_TURN90) != 0 )
    {
        leftTurnMode = SITHPUPPETSUBMODE_WHIPCLIMBLEFT;
        rightTurnMode = SITHPUPPETSUBMODE_WHIPCLIMBRIGHT;
    }
    else if ( (moveFlags & SITHACTORSPECIALMOVE_TURN135) != 0 )
    {
        leftTurnMode = SITHPUPPETSUBMODE_WHIPCLIMBDISMOUNT;
        rightTurnMode = SITHPUPPETSUBMODE_WHIPSWINGMOUNT;
    }
    else if ( (moveFlags & SITHACTORSPECIALMOVE_TURN180) != 0 )
    {
        leftTurnMode = SITHPUPPETSUBMODE_WHIPCLIMBMOUNT;
        v17 = 1;
    }
    else
    {
        if ( (moveFlags & SITHACTORSPECIALMOVE_MOUNTWALL) == 0 )
        {
            return 0;
        }

        leftTurnMode = SITHPUPPETSUBMODE_MOUNTWALL;
        v17 = 1;
    }

    if ( !v17 )
    {
        v17 = pThing->pPuppetClass->aModes[pThing->pPuppetState->majorMode][rightTurnMode].pKeyframe != 0;
    }

    if ( !pThing->pPuppetClass->aModes[pThing->pPuppetState->majorMode][leftTurnMode].pKeyframe || !v17 )
    {
        return 0;
    }

    bool bRight = false;
    if ( (moveFlags & (SITHACTORSPECIALMOVE_STRAFE | SITHACTORSPECIALMOVE_ROLL)) != 0 )
    {
        if ( (moveFlags & SITHACTORSPECIALMOVE_ROLL) != 0 )
        {
            v12 = 0.21000001f;
        }
        else
        {
            v12 = 0.079999998f;
        }

        float distance = v12;

        bRight = true;
        if ( (moveFlags & SITHACTORSPECIALMOVE_DIR_RANDOM) != 0 )
        {
            bRight = SITH_RAND() >= 0.5f;
        }

        else if ( (moveFlags & SITHACTORSPECIALMOVE_DIR_LEFT) != 0 )// left
        {
            bRight = 0;
        }

        if ( bRight )
        {
            memcpy(&newUVec, &pThing->orient, sizeof(newUVec));
        }
        else
        {
            newUVec.x = -pThing->orient.rvec.x;
            newUVec.y = -pThing->orient.rvec.y;
            newUVec.z = -pThing->orient.rvec.z;
        }

        endPos.x = distance * 0.5f * newUVec.x + pThing->pos.x;
        endPos.y = distance * 0.5f * newUVec.y + pThing->pos.y;
        endPos.z = distance * 0.5f * newUVec.z + pThing->pos.z;

        pTarget.x = newUVec.x * distance + pThing->pos.x;
        pTarget.y = newUVec.y * distance + pThing->pos.y;
        pTarget.z = newUVec.z * distance + pThing->pos.z;

        v25 = 0;
        while ( 1 )
        {
            v19 = sithAIUtil_CheckPathToPoint(pThing, &pTarget, pThing->collide.movesize, &distance, &pHitNorm, 1, 0);
            if ( !v19 || (moveFlags & (SITHACTORSPECIALMOVE_STRAFE | SITHACTORSPECIALMOVE_ROLL)) == 0 )
            {
                break;
            }

            if ( ++v25 >= 2 )
            {
                return 0;
            }

            bRight = 1 - bRight;
            newUVec.x = -newUVec.x;
            newUVec.y = -newUVec.y;
            newUVec.z = -newUVec.z;
        }

        if ( (moveFlags & SITHACTORSPECIALMOVE_ROLL) != 0 )
        {
            v21 = sithAIUtil_CheckPosition(pLocal, &endPos, 0);
            if ( v21 != 1 )
            {
                return 0;
            }
        }

        if ( (moveFlags & (SITHACTORSPECIALMOVE_STRAFE | SITHACTORSPECIALMOVE_ROLL)) != 0 )
        {
            v21 = sithAIUtil_CheckPosition(pLocal, &pTarget, 0);
            if ( v21 != 1 )
            {
                return 0;
            }
        }
    }
    else if ( (moveFlags & (SITHACTORSPECIALMOVE_TURN135 | SITHACTORSPECIALMOVE_TURN90 | SITHACTORSPECIALMOVE_TURN45)) != 0 )
    {
        bRight = (moveFlags & SITHACTORSPECIALMOVE_DIR_LEFT) == 0;
    }

    if ( (moveFlags & SITHACTORSPECIALMOVE_STRAFE) != 0 )
    {
        sithAIMove_StopAIMovement(pLocal);
        if ( bRight )
        {
            pLocal->pOwner->moveStatus = SITHPLAYERMOVE_STRAFING_RIGHT;
            sithPuppet_SetSubMode(pLocal->pOwner, SITHPUPPETSUBMODE_STRAFERIGHT, sithAIMove_PuppetCallback);
        }
        else
        {
            pLocal->pOwner->moveStatus = SITHPLAYERMOVE_STRAFING_LEFT;
            sithPuppet_SetSubMode(pLocal->pOwner, SITHPUPPETSUBMODE_STRAFELEFT, sithAIMove_PuppetCallback);
        }

        if ( pLocal->pTargetThing )
        {
            sithAIMove_AISetLookThing(pLocal, pLocal->pTargetThing);
        }

        sithAIMove_AISetMoveTargetPos(pLocal, &pTarget, 1.0f);
        return 1;
    }

    else if ( (moveFlags & SITHACTORSPECIALMOVE_ROLL) != 0 )
    {
        sithAIMove_StopAIMovement(pLocal);
        sithAIMove_ResetAILook(pLocal);
        if ( bRight )
        {
            pLocal->pOwner->moveStatus = SITHPLAYERMOVE_ROLLING_RIGHT;
            sithPuppet_SetSubMode(pLocal->pOwner, SITHPUPPETSUBMODE_HOPRIGHT, sithAIMove_PuppetCallback);
        }
        else
        {
            pLocal->pOwner->moveStatus = SITHPLAYERMOVE_ROLLING_LEFT;
            sithPuppet_SetSubMode(pLocal->pOwner, SITHPUPPETSUBMODE_HOPLEFT, sithAIMove_PuppetCallback);
        }

        sithAIMove_AISetMoveTargetPos(pLocal, &pTarget, 2.0f);
        return 1;
    }

    else if ( (moveFlags & SITHACTORSPECIALMOVE_TURN45) != 0 )
    {
        if ( bRight )
        {
            pLocal->pOwner->moveStatus = SITHPLAYERMOVE_TURNING_RIGHT_45_DEGREES;
            sithPuppet_SetSubMode(pLocal->pOwner, SITHPUPPETSUBMODE_WHIPCLIMBDOWN, sithAIMove_PuppetCallback);
        }
        else
        {
            pLocal->pOwner->moveStatus = SITHPLAYERMOVE_TURNING_LEFT_45_DEGREES;
            sithPuppet_SetSubMode(pLocal->pOwner, SITHPUPPETSUBMODE_WHIPCLIMBUP, sithAIMove_PuppetCallback);
        }

        return 1;
    }

    else if ( (moveFlags & SITHACTORSPECIALMOVE_TURN90) != 0 )
    {
        if ( bRight )
        {
            pLocal->pOwner->moveStatus = SITHPLAYERMOVE_TURNING_RIGHT_90_DEGREES;
            sithPuppet_SetSubMode(pLocal->pOwner, SITHPUPPETSUBMODE_WHIPCLIMBRIGHT, sithAIMove_PuppetCallback);
        }
        else
        {
            pLocal->pOwner->moveStatus = SITHPLAYERMOVE_TURNING_LEFT_90_DEGREES;
            sithPuppet_SetSubMode(pLocal->pOwner, SITHPUPPETSUBMODE_WHIPCLIMBLEFT, sithAIMove_PuppetCallback);
        }

        return 1;
    }

    else if ( (moveFlags & SITHACTORSPECIALMOVE_TURN135) != 0 )
    {
        if ( bRight )
        {
            pLocal->pOwner->moveStatus = SITHPLAYERMOVE_TURNING_RIGHT_135_DEGREES;
            sithPuppet_SetSubMode(pLocal->pOwner, SITHPUPPETSUBMODE_WHIPSWINGMOUNT, sithAIMove_PuppetCallback);
        }
        else
        {
            pLocal->pOwner->moveStatus = SITHPLAYERMOVE_TURNING_LEFT_135_DEGREES;
            sithPuppet_SetSubMode(pLocal->pOwner, SITHPUPPETSUBMODE_WHIPCLIMBDISMOUNT, sithAIMove_PuppetCallback);
        }

        return 1;
    }
    else                                        // wall mount
    {
        if ( (moveFlags & SITHACTORSPECIALMOVE_TURN180) != 0 )
        {
            pLocal->pOwner->moveStatus = SITHPLAYERMOVE_TURNING_180_DEGREES;
            sithPuppet_SetSubMode(pLocal->pOwner, SITHPUPPETSUBMODE_WHIPCLIMBMOUNT, sithAIMove_PuppetCallback);
            return 1;
        }

        if ( (moveFlags & SITHACTORSPECIALMOVE_MOUNTWALL) == 0 )
        {
            return 0;
        }

        v3 = rdMath_DeltaAngleNormalized(&pLocal->vecUnknown3, &pThing->orient.uvec, &pThing->orient.lvec);
        v14 = v3;
        if ( v3 < 0.0f )
        {
            v11 = -v14;
        }
        else
        {
            v11 = v3;
        }

        if ( v11 < 80.0f )
        {
            v9 = 0;
        }
        else
        {
            if ( v14 < 0.0f )
            {
                v10 = -v14;
            }
            else
            {
                v10 = v3;
            }

            v9 = v10 <= 100.0f;
        }

        if ( v9 )
        {
            pLocal->pOwner->moveStatus = SITHPLAYERMOVE_MOUNTING_WALL;
            sithPuppet_SetSubMode(pLocal->pOwner, SITHPUPPETSUBMODE_MOUNTWALL, sithAIMove_PuppetCallback);
            return 1;
        }

        if ( v14 < 0.0f )
        {
            v8 = -v14;
        }
        else
        {
            v8 = v3;
        }

        if ( v8 < 60.0f )
        {
            v6 = 0;
        }
        else
        {
            if ( v14 < 0.0f )
            {
                v7 = -v14;
            }
            else
            {
                v7 = v3;
            }

            v6 = v7 <= 79.0f;
        }

        if ( v6 )
        {
            newUVec.x = pLocal->vecUnknown3.x * -0.2f + pThing->orient.uvec.x;
            newUVec.y = pLocal->vecUnknown3.y * -0.2f + pThing->orient.uvec.y;
            newUVec.z = pLocal->vecUnknown3.z * -0.2f + pThing->orient.uvec.z;
            rdVector_Normalize3Acc(&newUVec);

            v4 = (pThing->orient.lvec.x - 0.0f) * newUVec.x + (pThing->orient.lvec.y - 0.0f) * newUVec.y + (pThing->orient.lvec.z - 0.0f) * newUVec.z;
            v13 = v4;
            pTarget.x = -v4 * newUVec.x + pThing->orient.lvec.x;
            pTarget.y = -v13 * newUVec.y + pThing->orient.lvec.y;
            pTarget.z = -v13 * newUVec.z + pThing->orient.lvec.z;
            rdVector_Normalize3Acc(&pTarget);

            memcpy(&pThing->orient.uvec, &newUVec, sizeof(pThing->orient.uvec));

            pThing->orient.rvec.x = pThing->orient.lvec.y * pThing->orient.uvec.z - pThing->orient.lvec.z * pThing->orient.uvec.y;
            pThing->orient.rvec.y = pThing->orient.lvec.z * pThing->orient.uvec.x - pThing->orient.lvec.x * pThing->orient.uvec.z;
            pThing->orient.rvec.z = pThing->orient.lvec.x * pThing->orient.uvec.y - pThing->orient.lvec.y * pThing->orient.uvec.x;
            rdVector_Normalize3Acc(&pThing->orient.rvec);

            pThing->orient.lvec.x = pThing->orient.uvec.y * pThing->orient.rvec.z - pThing->orient.uvec.z * pThing->orient.rvec.y;
            pThing->orient.lvec.y = pThing->orient.uvec.z * pThing->orient.rvec.x - pThing->orient.uvec.x * pThing->orient.rvec.z;
            pThing->orient.lvec.z = pThing->orient.uvec.x * pThing->orient.rvec.y - pThing->orient.uvec.y * pThing->orient.rvec.x;

            pLocal->pOwner->moveStatus = SITHPLAYERMOVE_MOUNTING_WALL;
            sithPuppet_SetSubMode(pLocal->pOwner, SITHPUPPETSUBMODE_MOUNTWALL, sithAIMove_PuppetCallback);
            return 1;
        }

        v5 = v14 >= 0.0f ? v3 : -v14;
        if ( v5 <= 40.0f )
        {
            return 0;
        }
        else
        {
            sithAIMove_AIFinalizeSpecialMove(pLocal, SITHACTORSPECIALMOVE_MOUNTWALL);
            return 1;
        }
    }
}

void J3DAPI sithAIMove_AIFinalizeSpecialMove(SithAIControlBlock* pLocal, SithActorSpecialMoveFlags moveFlags)
{
    SithThing* pThing;
    rdVector3 pyr;

    pThing = pLocal->pOwner;
    switch ( moveFlags )
    {
        case SITHACTORSPECIALMOVE_STRAFE:
            sithAIMove_StopAIMovement(pLocal);
            sithAIUtil_ApplyForce(pLocal, &pLocal->moveDirection, -0.050000001f);
            break;

        case SITHACTORSPECIALMOVE_TURN45:
        case SITHACTORSPECIALMOVE_TURN90:
        case SITHACTORSPECIALMOVE_TURN135:
        case SITHACTORSPECIALMOVE_TURN180:
            if ( pThing->moveStatus == SITHPLAYERMOVE_TURNING_LEFT_45_DEGREES )
            {
                pyr.x = 0.0f;
                pyr.y = 45.0f;
                pyr.z = 0.0f;
            }

            if ( pThing->moveStatus == SITHPLAYERMOVE_TURNING_RIGHT_45_DEGREES )
            {
                pyr.x = 0.0f;
                pyr.y = -45.0f;
                pyr.z = 0.0f;
            }

            if ( pThing->moveStatus == SITHPLAYERMOVE_TURNING_LEFT_90_DEGREES )
            {
                pyr.x = 0.0f;
                pyr.y = 90.0f;
                pyr.z = 0.0f;
            }

            if ( pThing->moveStatus == SITHPLAYERMOVE_TURNING_RIGHT_90_DEGREES )
            {
                pyr.x = 0.0f;
                pyr.y = -90.0f;
                pyr.z = 0.0f;
            }

            if ( pThing->moveStatus == SITHPLAYERMOVE_TURNING_LEFT_135_DEGREES )
            {
                pyr.x = 0.0f;
                pyr.y = 135.0f;
                pyr.z = 0.0f;
            }

            if ( pThing->moveStatus == SITHPLAYERMOVE_TURNING_RIGHT_135_DEGREES )
            {
                pyr.x = 0.0f;
                pyr.y = -135.0f;
                pyr.z = 0.0f;
            }

            if ( pThing->moveStatus == SITHPLAYERMOVE_TURNING_180_DEGREES )
            {
                pyr.x = 0.0f;
                pyr.y = 180.0f;
                pyr.z = 0.0f;
            }

            rdMatrix_PreRotate34(&pThing->orient, &pyr);
            rdMatrix_Normalize34(&pThing->orient);
            break;

        case SITHACTORSPECIALMOVE_MOUNTWALL:
            sithThing_DetachThing(pThing);

            memcpy(&pThing->orient.uvec, &pLocal->vecUnknown3, sizeof(pThing->orient.uvec));

            pThing->orient.lvec.x = pThing->orient.uvec.y * pThing->orient.rvec.z - pThing->orient.uvec.z * pThing->orient.rvec.y;
            pThing->orient.lvec.y = pThing->orient.uvec.z * pThing->orient.rvec.x - pThing->orient.uvec.x * pThing->orient.rvec.z;
            pThing->orient.lvec.z = pThing->orient.uvec.x * pThing->orient.rvec.y - pThing->orient.uvec.y * pThing->orient.rvec.x;
            rdVector_Normalize3Acc(&pThing->orient.lvec);

            sithPhysics_FindFloor(pThing, 1);
            if ( pThing->moveStatus == SITHPLAYERMOVE_MOUNTING_WALL )
            {
                sithCollision_MoveThing(pThing, &pThing->orient.lvec, pThing->collide.movesize, 0);
            }

            break;

        default:
            return;
    }
}

void J3DAPI sithAIMove_AIJump(SithAIControlBlock* pLocal, rdVector3* movePos, float jumpDirection)
{
    memcpy(&pLocal->movePos, movePos, sizeof(pLocal->movePos));
    pLocal->moveSpeed = 2.0f;
    if ( sithPuppet_PlayMode(pLocal->pOwner, SITHPUPPETSUBMODE_JUMPUP, 0) < 0 )
    {
        sithPlayerActions_Jump(pLocal->pOwner, jumpDirection, 0);
    }

    pLocal->msecPauseMoveUntil = sithTime_g_msecGameTime + 2000;
    pLocal->mode |= SITHAI_MODE_MOVING;
}

void J3DAPI sithAIMove_AIStop(SithAIControlBlock* pLocal)
{
    SITH_ASSERTREL(pLocal && pLocal->pOwner);
    sithAIMove_StopAIMovement(pLocal);
    sithAIMove_ResetAILook(pLocal);

    memcpy(&pLocal->movePos, &pLocal->pOwner->pos, sizeof(pLocal->movePos));

    sithAI_EmitEvent(pLocal, SITHAI_EVENT_GOAL_UNREACHABLE, NULL);
    sithAIMove_UpdateAIMove(pLocal);
}

void J3DAPI sithAIMove_SetGoalReached(SithAIControlBlock* pLocal)
{
    SITH_ASSERTREL(pLocal && pLocal->pOwner);
    sithAI_EmitEvent(pLocal, SITHAI_EVENT_GOAL_REACHED, NULL);
    if ( (pLocal->mode & SITHAI_MODE_BLOCK) != 0 )
    {
        sithCog_ThingSendMessage(pLocal->pOwner, NULL, SITHCOG_MSG_ARRIVED);
    }

    if ( (pLocal->submode & SITHAI_SUBMODE_UNKNOWN_4000) == 0 && (pLocal->mode & SITHAI_MODE_TRAVERSEWPNTS) == 0 )
    {
        sithAIMove_StopAIMovement(pLocal);
        if ( !sithAIMove_AIGetMoveState(pLocal) )
        {
            sithAIMove_UpdateAIMove(pLocal);
        }
    }
}

void J3DAPI sithAIMove_Unreachable(SithAIControlBlock* pLocal)
{
    SITH_ASSERTREL(pLocal && pLocal->pOwner);
    if ( (pLocal->mode & SITHAI_MODE_BLOCK) != 0 )
    {
        sithCog_ThingSendMessage(pLocal->pOwner, NULL, SITHCOG_MSG_BLOCKED);
    }
    else
    {
        sithAIMove_StopAIMovement(pLocal);
        sithAI_EmitEvent(pLocal, SITHAI_EVENT_GOAL_UNREACHABLE, NULL);
        sithAIMove_UpdateAIMove(pLocal);
    }
}

void J3DAPI sithAIMove_StopAIMovement(SithAIControlBlock* pLocal)
{
    sithAIUtil_AIClearMode(pLocal, SITHAI_MODE_MOVING);
    sithAIUtil_AIClearMode(pLocal, SITHAI_MODE_TRAVERSEWPNTS);

    if ( (pLocal->submode & SITHAI_SUBMODE_CONTINUOUSMOTION) != 0 )
    {
        pLocal->mode |= SITHAI_MODE_MOVING;
    }
    else
    {
        sithSoundClass_StopMode(pLocal->pOwner, SITHSOUNDCLASS_MOVING);
        memset(&pLocal->pOwner->moveInfo.physics.velocity, 0, sizeof(pLocal->pOwner->moveInfo.physics.velocity));
    }
}

void J3DAPI sithAIMove_ResetAILook(SithAIControlBlock* pLocal)
{
    sithActor_SetHeadPYR(pLocal->pOwner, &rdroid_g_zeroVector3);
    memcpy(&pLocal->goalLVec, &pLocal->pOwner->orient.lvec, sizeof(pLocal->goalLVec));
    sithAIUtil_AIClearMode(pLocal, SITHAI_MODE_TURNING);
}

void J3DAPI sithAIMove_UpdateBoss(SithAIControlBlock* pLocal, float secDeltaTime)
{
    SithThing* pOwner = pLocal->pOwner;
    SithActorInfo* pActorInfo = &pOwner->thingInfo.actorInfo;

    rdVector3 goal;
    rdVector_Copy3(&goal, &pLocal->goalLVec);

    float dot = fabsf(rdVector_Dot3(&pOwner->orient.uvec, &goal));
    if ( dot > 0.99f )
    {
        pLocal->mode &= ~SITHAI_MODE_TURNING;
        SITHLOG_ERROR("WARNING! AI '%s' looking straight up or down.\n", pOwner->aName);
        return;
    }

    float dist = rdMath_DistancePointToPlane(&goal, &pOwner->orient.uvec, &rdroid_g_zeroVector3);

    rdVector3 dir;
    rdVector_ScaleAdd3(&dir, &pOwner->orient.uvec, -dist, &goal);
    rdVector_Normalize3Acc(&dir);

    float angle    = rdMath_DeltaAngleNormalized(&dir, &pOwner->orient.lvec, &pOwner->orient.uvec);
    float rotStep  = (pActorInfo->maxRotVelocity * 1.0f) * secDeltaTime;
    float rotLimit = rotStep * 1.05f;
    if ( fabsf(angle) <= rotLimit )
    {
        rdVector_Copy3(&pOwner->orient.lvec, &dir);
        rdVector_Cross3(&pOwner->orient.rvec, &pOwner->orient.lvec, &pOwner->orient.uvec);
        rdVector_Normalize3Acc(&pOwner->orient.rvec);
        rdVector_Cross3(&pOwner->orient.uvec, &pOwner->orient.rvec, &pOwner->orient.lvec);
        pLocal->mode &= ~SITHAI_MODE_TURNING;
    }
    else
    {
        rdVector3 pyr;
        pyr.x = 0.0f;
        pyr.y = -((angle < 0.0f ? -1.0f : 1.0f) * rotStep);
        pyr.z = 0.0f;
        rdMatrix_PreRotate34(&pOwner->orient, &pyr);
        rdMatrix_Normalize34(&pOwner->orient);
    }
}

void J3DAPI sithAIMove_UpdateHeadTrackTarget(SithAIControlBlock* pLocal, float secDeltaTime)
{
    SITH_ASSERTREL(pLocal && pLocal->pOwner);

    SithThing* pOwner = pLocal->pOwner;
    SithActorInfo* pActorInfo = &pOwner->thingInfo.actorInfo;
    SithThing* pTarget = pLocal->pTargetThing;

    rdVector3 headPYR;
    rdVector_Copy3(&headPYR, &pActorInfo->headPYR);

    if ( (pActorInfo->flags & SITH_AF_IMMOBILE) != 0 )
    {
        return;
    }

    if ( (pActorInfo->flags & SITH_AF_CANROTATEHEAD) == 0 )
    {
        return;
    }

    rdVector3 eyePos;
    eyePos.x = 0.0f;
    eyePos.y = 0.0f;
    eyePos.z = pOwner->thingInfo.actorInfo.eyeOffset.z;
    rdMatrix_TransformVector34Acc(&eyePos, &pOwner->orient);
    rdVector_Add3Acc(&eyePos, &pOwner->pos);

    if ( pTarget && (pActorInfo->flags & SITH_AF_SEEINVISIBLE) == 0 )
    {
        if ( (pTarget->type == SITH_THING_ACTOR || pTarget->type == SITH_THING_PLAYER) && (pTarget->thingInfo.actorInfo.flags & SITH_AF_INVISIBLE) != 0 )
        {
            pTarget = NULL;
        }

        if ( (pActorInfo->flags & SITH_AF_BLIND) != 0 )
        {
            pTarget = NULL;
        }
    }

    float dist;
    rdVector3 targetPos;
    rdVector3 targetDir;
    rdVector3 projTargetDir;
    if ( pTarget )
    {
        if ( pTarget->type == SITH_THING_ACTOR || pTarget->type == SITH_THING_PLAYER )
        {
            rdMatrix_TransformVector34(&targetPos, &pTarget->thingInfo.actorInfo.eyeOffset, &pTarget->orient);
            rdVector_Add3Acc(&targetPos, &pTarget->pos);
        }
        else
        {
            rdVector_Copy3(&targetPos, &pTarget->pos);
        }

        rdVector_Sub3(&targetDir, &targetPos, &eyePos);
        rdVector_Normalize3Acc(&targetDir);

        dist = rdMath_DistancePointToPlane(&targetDir, &pOwner->orient.uvec, &rdroid_g_zeroVector3);
        rdVector_ScaleAdd3(&projTargetDir, &pOwner->orient.uvec, -dist, &targetDir);
        rdVector_Normalize3Acc(&projTargetDir);
    }

    rdVector_Copy3(&targetPos, &pLocal->lookPos);

    rdVector3 lookDir;
    rdVector_Sub3(&lookDir, &targetPos, &eyePos);
    rdVector_Normalize3Acc(&lookDir);

    dist = rdMath_DistancePointToPlane(&lookDir, &pOwner->orient.uvec, &rdroid_g_zeroVector3);

    rdVector3 projLookDir;
    rdVector_ScaleAdd3(&projLookDir, &pOwner->orient.uvec, -dist, &lookDir);
    rdVector_Normalize3Acc(&projLookDir);

    float lowThreshold  = -0.2f;
    float highThreshold = sithAIMove_g_flt_58546C;
    if ( !pTarget )
    {
        pLocal->submode &= ~SITHAI_SUBMODE_UNKNOWN_800;
    }
    else if ( (pActorInfo->flags & SITH_AF_NOSLOPEMOVE) != 0 || (pLocal->mode & SITHAI_MODE_BLOCK) != 0 )
    {
        pLocal->submode |= SITHAI_SUBMODE_UNKNOWN_800;
        lowThreshold  = -1.0f;
        highThreshold = 1.0f;
    }
    else
    {
        float dot = rdVector_Dot3(&pOwner->orient.lvec, &projTargetDir);
        if ( (pLocal->submode & SITHAI_SUBMODE_UNKNOWN_800) != 0 )
        {
            if ( dot < lowThreshold )
            {
                pLocal->submode &= ~SITHAI_SUBMODE_UNKNOWN_800;
            }
        }
        else if ( dot > highThreshold )
        {
            pLocal->submode |= SITHAI_SUBMODE_UNKNOWN_800;
        }
    }

    rdVector3 aimDir;
    rdVector3 pitchDir;
    if ( (pLocal->submode & SITHAI_SUBMODE_UNKNOWN_800) != 0 )
    {
        rdVector_Copy3(&aimDir, &projTargetDir);
        rdVector_Copy3(&pitchDir, &targetDir);
    }
    else
    {
        rdVector_Copy3(&aimDir, &projLookDir);
        rdVector_Copy3(&pitchDir, &lookDir);
    }

    float yawRate   = pActorInfo->maxHeadVelocity * secDeltaTime;
    float pitchRate = (pActorInfo->maxHeadVelocity * 0.5f) * secDeltaTime;

    float dot = rdVector_Dot3(&pOwner->orient.lvec, &aimDir);

    float yaw = 0.0f;
    if ( dot >= lowThreshold )
    {
        yaw = rdMath_DeltaAngleNormalized(&pOwner->orient.lvec, &aimDir, &pOwner->orient.uvec);
    }

    float deltaYaw = yaw - pActorInfo->headPYR.y;
    deltaYaw = STDMATH_CLAMP(deltaYaw, -yawRate, yawRate);
    headPYR.y += deltaYaw;
    if ( headPYR.y <= -pActorInfo->maxHeadYaw || headPYR.y >= pActorInfo->maxHeadYaw )
    {
        headPYR.y = pActorInfo->maxHeadYaw * (headPYR.y < 0.0f ? -1.0f : 1.0f);
        pLocal->submode &= ~SITHAI_SUBMODE_HEADTRACKINGMOTION;
    }
    else
    {
        pLocal->submode |= SITHAI_SUBMODE_HEADTRACKINGMOTION;
    }

    float pitch = 0.0f;
    if ( dot > 0.0f )
    {
        pitch = rdMath_DeltaAngleNormalized(&aimDir, &pitchDir, &pOwner->orient.rvec);
    }

    float deltaPitch = pitch - pActorInfo->headPYR.x;
    deltaPitch = STDMATH_CLAMP(deltaPitch, -pitchRate, pitchRate);
    headPYR.x = STDMATH_CLAMP(headPYR.x + deltaPitch, pActorInfo->minHeadPitch, pActorInfo->maxHeadPitch);

    sithActor_SetHeadPYR(pLocal->pOwner, &headPYR);
}

void J3DAPI sithAIMove_UpdateHeadTrackLookPos(SithAIControlBlock* pLocal, float secDeltaTime)
{
    SITH_ASSERTREL(pLocal && pLocal->pOwner);

    SithThing* pOwner = pLocal->pOwner;
    SithActorInfo* pActorInfo = &pOwner->thingInfo.actorInfo;

    rdVector3 headPYR;
    rdVector_Copy3(&headPYR, &pActorInfo->headPYR);

    rdVector3 lookPos;
    rdVector_Copy3(&lookPos, &pLocal->lookPos);

    if ( (pActorInfo->flags & SITH_AF_CANROTATEHEAD) == 0 )
    {
        return;
    }

    if ( (pOwner->thingInfo.actorInfo.flags & SITH_AF_IMMOBILE) != 0 )
    {
        return;
    }

    rdVector3 eyePos;
    eyePos.x = 0.0f;
    eyePos.y = 0.0f;
    eyePos.z = pOwner->thingInfo.actorInfo.eyeOffset.z;
    rdMatrix_TransformVector34Acc(&eyePos, &pOwner->orient);
    rdVector_Add3Acc(&eyePos, &pOwner->pos);

    rdVector3 lookDir;
    rdVector_Sub3(&lookDir, &lookPos, &eyePos);
    rdVector_Normalize3Acc(&lookDir);

    float dist = rdMath_DistancePointToPlane(&lookDir, &pOwner->orient.uvec, &rdroid_g_zeroVector3);

    rdVector3 projDir;
    rdVector_ScaleAdd3(&projDir, &pOwner->orient.uvec, -dist, &lookDir);
    rdVector_Normalize3Acc(&projDir);

    float yawRate   = pActorInfo->maxHeadVelocity * secDeltaTime;
    float pitchRate = (pActorInfo->maxHeadVelocity * 0.5f) * secDeltaTime;

    float yaw = rdMath_DeltaAngleNormalized(&pOwner->orient.lvec, &projDir, &pOwner->orient.uvec);
    float deltaYaw = yaw - pActorInfo->headPYR.y;
    deltaYaw = STDMATH_CLAMP(deltaYaw, -yawRate, yawRate);
    headPYR.y = STDMATH_CLAMP(headPYR.y + deltaYaw, -pActorInfo->maxHeadYaw, pActorInfo->maxHeadYaw);

    float pitch = 0.0f;
    float dot = rdVector_Dot3(&pOwner->orient.lvec, &projDir);
    if ( dot > 0.0f )
    {
        pitch = rdMath_DeltaAngleNormalized(&projDir, &lookDir, &pOwner->orient.rvec);
    }

    float deltaPitch = pitch - pActorInfo->headPYR.x;
    deltaPitch = STDMATH_CLAMP(deltaPitch, -pitchRate, pitchRate);
    headPYR.x = STDMATH_CLAMP(headPYR.x + deltaPitch, pActorInfo->minHeadPitch, pActorInfo->maxHeadPitch);

    sithActor_SetHeadPYR(pLocal->pOwner, &headPYR);

    if ( fabsf(deltaYaw) <= 0.5f && fabsf(deltaPitch) <= 0.5f )
    {
        pLocal->mode &= ~SITHAI_MODE_UNKNOWN_80;
    }
}

void J3DAPI sithAIMove_GetQuetzStrikeAngles(SithAIControlBlock* pLocal, float* pYaw, float* pPitch, float* pHeadYaw, float* pHeadPitch)
{
    SithThing* pOwner  = pLocal->pOwner;
    SithThing* pTarget = pLocal->pTargetThing;

    rdVector3 ownerEye;
    rdVector_Copy3(&ownerEye, &pOwner->pos);
    ownerEye.z += pOwner->thingInfo.actorInfo.eyeOffset.z;

    rdVector3 targetEye;
    rdMatrix_TransformVector34(&targetEye, &pTarget->thingInfo.actorInfo.eyeOffset, &pTarget->orient);
    rdVector_Add3Acc(&targetEye, &pTarget->pos);
    if ( (pTarget->moveInfo.physics.flags & SITH_PF_CROUCHING) != 0 )
    {
        targetEye.z -= 0.1f;
    }

    rdVector3 dir;
    rdVector_Sub3(&dir, &targetEye, &ownerEye);
    rdVector_Normalize3Acc(&dir);

    float dist = rdMath_DistancePointToPlane(&dir, &pOwner->orient.uvec, &rdroid_g_zeroVector3);

    rdVector3 lookDir;
    rdVector_ScaleAdd3(&lookDir, &pOwner->orient.uvec, -dist, &dir);
    rdVector_Normalize3Acc(&lookDir);

    float yaw = rdMath_DeltaAngleNormalized(&pOwner->orient.lvec, &lookDir, &pOwner->orient.uvec);
    *pYaw     = yaw;
    *pHeadYaw = -pOwner->renderData.apTweakedAngles[2].y;

    float distance = sqrtf((pOwner->pos.x - pTarget->pos.x) * (pOwner->pos.x - pTarget->pos.x) + (pOwner->pos.y - pTarget->pos.y) * (pOwner->pos.y - pTarget->pos.y));
    distance = STDMATH_CLAMP(distance, 0.3f, 0.5f);

    float pitchScale = (-60.0f - -15.0f) / (0.5f - 0.3f);
    *pPitch = (distance - 0.3f) * pitchScale + -15.0f;

    float headPitchScale = (60.0f - sithAIMove_g_flt_585470) / (0.5f - 0.3f);
    *pHeadPitch = (distance - 0.3f) * headPitchScale + sithAIMove_g_flt_585470 - pOwner->renderData.apTweakedAngles[2].x;
}

void J3DAPI sithAIMove_UpdateQuetzStrike(SithAIControlBlock* pLocal, float secDeltaTime)
{
    SithThing* pThing = pLocal->pOwner;
    if ( !sithThing_CreateQuetzUserBlock(pThing) )
    {
        return;
    }

    // Strike: unknown0 = mode, unknown1 = curTime, unknown3 = numSnapShots; snapshot: unknown0 = time
    SithQuetzStrike* pStrike = &pThing->userblock.pQuetz->strike;
    if ( pStrike->unknown0 != 1 )
    {
        return;
    }

    SithQuetzSnapShot* pSnapShot = &pStrike->aSnapShots[pStrike->snapShot];
    float remainder = pSnapShot->unknown0 - pStrike->unknown1;

    if ( pStrike->snapShot == 1 )
    {
        float angle;
        float pitchAngle;
        float headYaw;
        float angleHeadPitch;
        sithAIMove_GetQuetzStrikeAngles(pLocal, &angle, &pitchAngle, &headYaw, &angleHeadPitch);
        pStrike->aSnapShots[1].angle1 = 20.0f;
        pStrike->aSnapShots[1].angle2 = angle * 0.5f;
        pStrike->aSnapShots[1].angle3 = pThing->renderData.apTweakedAngles[2].pitch - 20.0f;
        pStrike->aSnapShots[1].angle4 = pThing->renderData.apTweakedAngles[2].yaw + headYaw * 0.5f;
        pStrike->aSnapShots[1].angle5 = pStrike->aSnapShots[0].angle5;
    }
    else if ( pStrike->snapShot == 2 )
    {
        float angle;
        float pitchAngle;
        float headYaw;
        float angleHeadPitch;
        sithAIMove_GetQuetzStrikeAngles(pLocal, &angle, &pitchAngle, &headYaw, &angleHeadPitch);
        pStrike->aSnapShots[2].angle1 = pitchAngle * 0.9f;
        pStrike->aSnapShots[2].angle2 = angle * 0.9f;
        pStrike->aSnapShots[2].angle3 = pThing->renderData.apTweakedAngles[2].pitch + angleHeadPitch * 0.9f;
        pStrike->aSnapShots[2].angle4 = pThing->renderData.apTweakedAngles[2].yaw + headYaw * 0.9f;
        pStrike->aSnapShots[2].angle5 = -30.0f;
    }
    else if ( pStrike->snapShot == 3 )
    {
        float angle;
        float pitchAngle;
        float headYaw;
        float angleHeadPitch;
        sithAIMove_GetQuetzStrikeAngles(pLocal, &angle, &pitchAngle, &headYaw, &angleHeadPitch);
        pStrike->aSnapShots[3].angle1 = pitchAngle;
        pStrike->aSnapShots[3].angle2 = angle;
        pStrike->aSnapShots[3].angle3 = pThing->renderData.apTweakedAngles[2].pitch + angleHeadPitch;
        pStrike->aSnapShots[3].angle4 = pThing->renderData.apTweakedAngles[2].yaw + headYaw;
        pStrike->aSnapShots[3].angle5 = 20.0f;
    }
    else if ( pStrike->snapShot == 4 )
    {
        float angle;
        float pitchAngle;
        float headYaw;
        float angleHeadPitch;
        sithAIMove_GetQuetzStrikeAngles(pLocal, &angle, &pitchAngle, &headYaw, &angleHeadPitch);
        pStrike->aSnapShots[4].angle1 = 10.0f;
        pStrike->aSnapShots[4].angle2 = angle * 0.4f;
        pStrike->aSnapShots[4].angle3 = pThing->renderData.apTweakedAngles[2].pitch - 10.0f;
        pStrike->aSnapShots[4].angle4 = pThing->renderData.apTweakedAngles[2].yaw + headYaw * 0.4f;
        pStrike->aSnapShots[4].angle5 = pStrike->aSnapShots[0].angle5;
    }
    else if ( pStrike->snapShot == pStrike->unknown3 - 1 )
    {
        pSnapShot->angle3 = pThing->renderData.apTweakedAngles[2].pitch;
        pSnapShot->angle4 = pThing->renderData.apTweakedAngles[2].yaw;
    }

    if ( remainder >= secDeltaTime )
    {
        SITH_ASSERTREL(pStrike->snapShot != 0);
        SithQuetzSnapShot* pPrevSnapShot = &pStrike->aSnapShots[pStrike->snapShot - 1];
        pStrike->unknown1 += secDeltaTime;

        float length      = pSnapShot->unknown0 - pPrevSnapShot->unknown0;
        float timeElapsed = pStrike->unknown1 - pPrevSnapShot->unknown0;
        float ratio       = timeElapsed / length;
        ratio = STDMATH_CLAMP(ratio, 0.0f, 1.0f);

        pThing->renderData.apTweakedAngles[1].pitch  = (pSnapShot->angle1 - pPrevSnapShot->angle1) * ratio + pPrevSnapShot->angle1;
        pThing->renderData.apTweakedAngles[1].yaw    = (pSnapShot->angle2 - pPrevSnapShot->angle2) * ratio + pPrevSnapShot->angle2;
        pThing->renderData.apTweakedAngles[2].pitch  = (pSnapShot->angle3 - pPrevSnapShot->angle3) * ratio + pPrevSnapShot->angle3;
        pThing->renderData.apTweakedAngles[2].yaw    = (pSnapShot->angle4 - pPrevSnapShot->angle4) * ratio + pPrevSnapShot->angle4;
        pThing->renderData.apTweakedAngles[28].pitch = (pSnapShot->angle5 - pPrevSnapShot->angle5) * ratio + pPrevSnapShot->angle5;
    }
    else
    {
        pStrike->snapShot++;
        pStrike->unknown1 = pSnapShot->unknown0;
        pThing->renderData.apTweakedAngles[1].pitch  = pSnapShot->angle1;
        pThing->renderData.apTweakedAngles[1].yaw    = pSnapShot->angle2;
        pThing->renderData.apTweakedAngles[2].pitch  = pSnapShot->angle3;
        pThing->renderData.apTweakedAngles[2].yaw    = pSnapShot->angle4;
        pThing->renderData.apTweakedAngles[28].pitch = pSnapShot->angle5;
        if ( pStrike->snapShot == pStrike->unknown3 )
        {
            pStrike->unknown0 = 0;
        }
        else
        {
            sithAIMove_UpdateQuetzStrike(pLocal, secDeltaTime - remainder);
        }
    }
}

void J3DAPI sithAIMove_GetQuetzAnim2Angles(SithAIControlBlock* pLocal, float* pYaw, float* pPitch, float* pHeadYaw, float* pHeadPitch)
{
    SithThing* pOwner  = pLocal->pOwner;
    SithThing* pTarget = pLocal->pTargetThing;

    rdVector3 pos;
    rdVector_Copy3(&pos, &pOwner->pos);
    pos.z += pOwner->thingInfo.actorInfo.eyeOffset.z;

    rdVector3 targetPos;
    rdMatrix_TransformVector34(&targetPos, &pTarget->thingInfo.actorInfo.eyeOffset, &pTarget->orient);
    rdVector_Add3Acc(&targetPos, &pTarget->pos);

    rdVector3 dir;
    rdVector_Sub3(&dir, &targetPos, &pos);
    rdVector_Normalize3Acc(&dir);

    float dist = rdMath_DistancePointToPlane(&dir, &pOwner->orient.uvec, &rdroid_g_zeroVector3);

    rdVector3 planeDir;
    rdVector_ScaleAdd3(&planeDir, &pOwner->orient.uvec, -dist, &dir);
    rdVector_Normalize3Acc(&planeDir);

    float yaw = rdMath_DeltaAngleNormalized(&pOwner->orient.lvec, &planeDir, &pOwner->orient.uvec);
    *pYaw       = yaw;
    *pHeadYaw   = -pOwner->renderData.apTweakedAngles[2].yaw;
    *pHeadPitch = 50.0f;
    *pPitch     = -30.0f;
}

void J3DAPI sithAIMove_UpdateQuetzAnim2(SithAIControlBlock* pLocal, float secDeltaTime)
{
    SithThing* pThing = pLocal->pOwner;
    if ( !sithThing_CreateQuetzUserBlock(pThing) )
    {
        return;
    }

    SithQuetzStrike* pStrike = &pThing->userblock.pQuetz->strike;
    if ( pStrike->unknown0 != 2 )
    {
        return;
    }

    SithQuetzSnapShot* pSnapShot = &pStrike->aSnapShots[pStrike->snapShot];
    float remainder = pSnapShot->unknown0 - pStrike->unknown1;

    if ( pStrike->snapShot == 1 )
    {
        float angle;
        float pitchAngle;
        float headYaw;
        float angleHeadPitch;
        sithAIMove_GetQuetzAnim2Angles(pLocal, &angle, &pitchAngle, &headYaw, &angleHeadPitch);
        pStrike->aSnapShots[1].angle1 = 20.0f;
        pStrike->aSnapShots[1].angle2 = angle * 0.5f;
        pStrike->aSnapShots[1].angle3 = pThing->renderData.apTweakedAngles[2].pitch + 10.0f;
        pStrike->aSnapShots[1].angle4 = pThing->renderData.apTweakedAngles[2].yaw + headYaw * 0.5f;
        pStrike->aSnapShots[1].angle5 = pStrike->aSnapShots[0].angle5;
    }
    else if ( pStrike->snapShot == 2 )
    {
        float angle;
        float pitchAngle;
        float headYaw;
        float angleHeadPitch;
        sithAIMove_GetQuetzAnim2Angles(pLocal, &angle, &pitchAngle, &headYaw, &angleHeadPitch);
        pStrike->aSnapShots[2].angle1 = pitchAngle;
        pStrike->aSnapShots[2].angle2 = angle;
        pStrike->aSnapShots[2].angle3 = pThing->renderData.apTweakedAngles[2].pitch + angleHeadPitch;
        pStrike->aSnapShots[2].angle4 = pThing->renderData.apTweakedAngles[2].yaw + headYaw;
        pStrike->aSnapShots[2].angle5 = -70.0f;
    }
    else if ( pStrike->snapShot == pStrike->unknown3 - 1 )
    {
        pSnapShot->angle3 = pThing->renderData.apTweakedAngles[2].pitch;
        pSnapShot->angle4 = pThing->renderData.apTweakedAngles[2].yaw;
    }

    if ( remainder >= secDeltaTime )
    {
        SITH_ASSERTREL(pStrike->snapShot != 0);
        SithQuetzSnapShot* pPrevSnapShot = &pStrike->aSnapShots[pStrike->snapShot - 1];
        pStrike->unknown1 += secDeltaTime;

        float length      = pSnapShot->unknown0 - pPrevSnapShot->unknown0;
        float timeElapsed = pStrike->unknown1 - pPrevSnapShot->unknown0;
        float ratio       = timeElapsed / length;
        ratio = STDMATH_CLAMP(ratio, 0.0f, 1.0f);

        pThing->renderData.apTweakedAngles[1].pitch  = (pSnapShot->angle1 - pPrevSnapShot->angle1) * ratio + pPrevSnapShot->angle1;
        pThing->renderData.apTweakedAngles[1].yaw    = (pSnapShot->angle2 - pPrevSnapShot->angle2) * ratio + pPrevSnapShot->angle2;
        pThing->renderData.apTweakedAngles[2].pitch  = (pSnapShot->angle3 - pPrevSnapShot->angle3) * ratio + pPrevSnapShot->angle3;
        pThing->renderData.apTweakedAngles[2].yaw    = (pSnapShot->angle4 - pPrevSnapShot->angle4) * ratio + pPrevSnapShot->angle4;
        pThing->renderData.apTweakedAngles[28].pitch = (pSnapShot->angle5 - pPrevSnapShot->angle5) * ratio + pPrevSnapShot->angle5;
    }
    else
    {
        pStrike->snapShot++;
        pStrike->unknown1 = pSnapShot->unknown0;
        pThing->renderData.apTweakedAngles[1].pitch  = pSnapShot->angle1;
        pThing->renderData.apTweakedAngles[1].yaw    = pSnapShot->angle2;
        pThing->renderData.apTweakedAngles[2].pitch  = pSnapShot->angle3;
        pThing->renderData.apTweakedAngles[2].yaw    = pSnapShot->angle4;
        pThing->renderData.apTweakedAngles[28].pitch = pSnapShot->angle5;
        if ( pStrike->snapShot == pStrike->unknown3 )
        {
            pStrike->unknown0 = 0;
        }
        else
        {
            sithAIMove_UpdateQuetzAnim2(pLocal, secDeltaTime - remainder);
        }
    }
}

void J3DAPI sithAIMove_UpdateQuetzAnim3(SithAIControlBlock* pLocal, float secDeltaTime)
{
    SithThing* pThing = pLocal->pOwner;
    if ( !sithThing_CreateQuetzUserBlock(pThing) )
    {
        return;
    }

    SithQuetzStrike* pStrike = &pThing->userblock.pQuetz->strike;
    if ( pStrike->unknown0 != 3 )
    {
        return;
    }

    SithQuetzSnapShot* pSnapShot = &pStrike->aSnapShots[pStrike->snapShot];
    float timeLeft = pSnapShot->unknown0 - pStrike->unknown1;

    if ( pStrike->snapShot == pStrike->unknown3 - 1 )
    {
        pSnapShot->angle3 = pThing->renderData.apTweakedAngles[2].pitch;
        pSnapShot->angle4 = pThing->renderData.apTweakedAngles[2].yaw;
    }

    if ( timeLeft >= secDeltaTime )
    {
        SITH_ASSERTREL(pStrike->snapShot != 0);
        SithQuetzSnapShot* pPrevSnapShot = &pStrike->aSnapShots[pStrike->snapShot - 1];
        pStrike->unknown1 += secDeltaTime;

        float period      = pSnapShot->unknown0 - pPrevSnapShot->unknown0;
        float timeElapsed = pStrike->unknown1 - pPrevSnapShot->unknown0;
        float ratio       = timeElapsed / period;
        ratio = STDMATH_CLAMP(ratio, 0.0f, 1.0f);

        pThing->renderData.apTweakedAngles[1].pitch  = (pSnapShot->angle1 - pPrevSnapShot->angle1) * ratio + pPrevSnapShot->angle1;
        pThing->renderData.apTweakedAngles[1].yaw    = (pSnapShot->angle2 - pPrevSnapShot->angle2) * ratio + pPrevSnapShot->angle2;
        pThing->renderData.apTweakedAngles[2].pitch  = (pSnapShot->angle3 - pPrevSnapShot->angle3) * ratio + pPrevSnapShot->angle3;
        pThing->renderData.apTweakedAngles[2].yaw    = (pSnapShot->angle4 - pPrevSnapShot->angle4) * ratio + pPrevSnapShot->angle4;
        pThing->renderData.apTweakedAngles[28].pitch = (pSnapShot->angle5 - pPrevSnapShot->angle5) * ratio + pPrevSnapShot->angle5;
    }
    else
    {
        pStrike->snapShot++;
        pStrike->unknown1 = pSnapShot->unknown0;
        pThing->renderData.apTweakedAngles[1].pitch  = pSnapShot->angle1;
        pThing->renderData.apTweakedAngles[1].yaw    = pSnapShot->angle2;
        pThing->renderData.apTweakedAngles[2].pitch  = pSnapShot->angle3;
        pThing->renderData.apTweakedAngles[2].yaw    = pSnapShot->angle4;
        pThing->renderData.apTweakedAngles[28].pitch = pSnapShot->angle5;
        if ( pStrike->snapShot == pStrike->unknown3 )
        {
            pStrike->unknown0 = 0;
        }
        else
        {
            sithAIMove_UpdateQuetzAnim3(pLocal, secDeltaTime - timeLeft);
        }
    }
}

void J3DAPI sithAIMove_UpdateQuetzAnim5(SithAIControlBlock* pLocal, float secDeltaTime)
{
    SithThing* pThing = pLocal->pOwner;
    if ( !sithThing_CreateQuetzUserBlock(pThing) )
    {
        return;
    }

    SithQuetzStrike* pStrike = &pThing->userblock.pQuetz->strike;
    if ( pStrike->unknown0 != 5 )
    {
        return;
    }

    SithQuetzSnapShot* pSnapShot = &pStrike->aSnapShots[pStrike->snapShot];
    float timeLeft = pSnapShot->unknown0 - pStrike->unknown1;

    if ( pStrike->snapShot == pStrike->unknown3 - 1 )
    {
        pSnapShot->angle3 = pThing->renderData.apTweakedAngles[2].pitch;
        pSnapShot->angle4 = pThing->renderData.apTweakedAngles[2].yaw;
    }

    if ( timeLeft >= secDeltaTime )
    {
        SITH_ASSERTREL(pStrike->snapShot != 0);
        SithQuetzSnapShot* pPrevSnapShot = &pStrike->aSnapShots[pStrike->snapShot - 1];
        pStrike->unknown1 += secDeltaTime;

        float period      = pSnapShot->unknown0 - pPrevSnapShot->unknown0;
        float timeElapsed = pStrike->unknown1 - pPrevSnapShot->unknown0;
        float ratio       = timeElapsed / period;
        ratio = STDMATH_CLAMP(ratio, 0.0f, 1.0f);

        pThing->renderData.apTweakedAngles[1].pitch  = (pSnapShot->angle1 - pPrevSnapShot->angle1) * ratio + pPrevSnapShot->angle1;
        pThing->renderData.apTweakedAngles[1].yaw    = (pSnapShot->angle2 - pPrevSnapShot->angle2) * ratio + pPrevSnapShot->angle2;
        pThing->renderData.apTweakedAngles[2].pitch  = (pSnapShot->angle3 - pPrevSnapShot->angle3) * ratio + pPrevSnapShot->angle3;
        pThing->renderData.apTweakedAngles[2].yaw    = (pSnapShot->angle4 - pPrevSnapShot->angle4) * ratio + pPrevSnapShot->angle4;
        pThing->renderData.apTweakedAngles[28].pitch = (pSnapShot->angle5 - pPrevSnapShot->angle5) * ratio + pPrevSnapShot->angle5;
    }
    else
    {
        pStrike->snapShot++;
        pStrike->unknown1 = pSnapShot->unknown0;
        pThing->renderData.apTweakedAngles[1].pitch  = pSnapShot->angle1;
        pThing->renderData.apTweakedAngles[1].yaw    = pSnapShot->angle2;
        pThing->renderData.apTweakedAngles[2].pitch  = pSnapShot->angle3;
        pThing->renderData.apTweakedAngles[2].yaw    = pSnapShot->angle4;
        pThing->renderData.apTweakedAngles[28].pitch = pSnapShot->angle5;
        if ( pStrike->snapShot == pStrike->unknown3 )
        {
            pStrike->unknown0 = 0;
        }
        else
        {
            sithAIMove_UpdateQuetzAnim5(pLocal, secDeltaTime - timeLeft);
        }
    }
}

void J3DAPI sithAIMove_UpdateQuetzAnim4(SithAIControlBlock* pLocal, float secDeltaTime)
{
    SithThing* pThing = pLocal->pOwner;
    if ( !sithThing_CreateQuetzUserBlock(pThing) )
    {
        return;
    }

    SithQuetzStrike* pStrike = &pThing->userblock.pQuetz->strike;
    if ( pStrike->unknown0 != 4 )
    {
        return;
    }

    SithQuetzSnapShot* pSnapShot = &pStrike->aSnapShots[pStrike->snapShot];
    float timeLeft = pSnapShot->unknown0 - pStrike->unknown1;

    if ( timeLeft >= secDeltaTime )
    {
        SITH_ASSERTREL(pStrike->snapShot != 0);
        SithQuetzSnapShot* pPrevSnapShot = &pStrike->aSnapShots[pStrike->snapShot - 1];
        pStrike->unknown1 += secDeltaTime;

        float period      = pSnapShot->unknown0 - pPrevSnapShot->unknown0;
        float timeElapsed = pStrike->unknown1 - pPrevSnapShot->unknown0;
        float ratio       = timeElapsed / period;
        ratio = STDMATH_CLAMP(ratio, 0.0f, 1.0f);

        pThing->renderData.apTweakedAngles[1].pitch  = (pSnapShot->angle1 - pPrevSnapShot->angle1) * ratio + pPrevSnapShot->angle1;
        pThing->renderData.apTweakedAngles[1].yaw    = (pSnapShot->angle2 - pPrevSnapShot->angle2) * ratio + pPrevSnapShot->angle2;
        pThing->renderData.apTweakedAngles[2].pitch  = (pSnapShot->angle3 - pPrevSnapShot->angle3) * ratio + pPrevSnapShot->angle3;
        pThing->renderData.apTweakedAngles[2].yaw    = (pSnapShot->angle4 - pPrevSnapShot->angle4) * ratio + pPrevSnapShot->angle4;
        pThing->renderData.apTweakedAngles[28].pitch = (pSnapShot->angle5 - pPrevSnapShot->angle5) * ratio + pPrevSnapShot->angle5;
    }
    else
    {
        pStrike->snapShot++;
        pStrike->unknown1 = pSnapShot->unknown0;
        pThing->renderData.apTweakedAngles[1].pitch  = pSnapShot->angle1;
        pThing->renderData.apTweakedAngles[1].yaw    = pSnapShot->angle2;
        pThing->renderData.apTweakedAngles[2].pitch  = pSnapShot->angle3;
        pThing->renderData.apTweakedAngles[2].yaw    = pSnapShot->angle4;
        pThing->renderData.apTweakedAngles[28].pitch = pSnapShot->angle5;
        if ( pStrike->snapShot == pStrike->unknown3 )
        {
            pStrike->unknown0 = 0;
        }
        else
        {
            sithAIMove_UpdateQuetzAnim4(pLocal, secDeltaTime - timeLeft);
        }
    }
}

void J3DAPI sithAIMove_UpdateQuetzTail(SithAIControlBlock* pLocal, float secDeltaTime)
{
    SithThing* pThing = pLocal->pOwner;
    if ( !sithThing_CreateQuetzUserBlock(pThing) )
    {
        return;
    }

    SithQuetzStrike* pStrike = &pThing->userblock.pQuetz->strike;
    switch ( pStrike->unknown0 )
    {
        case 1:
            sithAIMove_UpdateQuetzStrike(pLocal, secDeltaTime);
            break;

        case 2:
            sithAIMove_UpdateQuetzAnim2(pLocal, secDeltaTime);
            break;

        case 3:
            sithAIMove_UpdateQuetzAnim3(pLocal, secDeltaTime);
            break;

        case 5:
            sithAIMove_UpdateQuetzAnim5(pLocal, secDeltaTime);
            break;

        case 4:
            sithAIMove_UpdateQuetzAnim4(pLocal, secDeltaTime);
            break;
    }
}

void J3DAPI sithAIMove_StartQuetzStrike(SithAIControlBlock* pLocal)
{
    SithThing* pThing = pLocal->pOwner;
    if ( !sithThing_CreateQuetzUserBlock(pThing) )
    {
        return;
    }

    SithQuetzStrike* pStrike = &pThing->userblock.pQuetz->strike;
    float angle;
    float pitchAngle;
    float headYaw;
    float neckPitch;
    sithAIMove_GetQuetzStrikeAngles(pLocal, &angle, &pitchAngle, &headYaw, &neckPitch);

    pStrike->unknown1 = 0.0f;
    pStrike->snapShot = 0;
    pStrike->aSnapShots[0].unknown0 = 0.0f;
    pStrike->aSnapShots[0].angle1 = pThing->renderData.apTweakedAngles[1].pitch;
    pStrike->aSnapShots[0].angle2 = pThing->renderData.apTweakedAngles[1].yaw;
    pStrike->aSnapShots[0].angle3 = pThing->renderData.apTweakedAngles[2].pitch;
    pStrike->aSnapShots[0].angle4 = pThing->renderData.apTweakedAngles[2].yaw;
    pStrike->aSnapShots[0].angle5 = pThing->renderData.apTweakedAngles[28].pitch;

    pStrike->aSnapShots[1].unknown0 = 0.11f;
    pStrike->aSnapShots[2].unknown0 = 0.24f;
    pStrike->aSnapShots[3].unknown0 = 0.3f;
    pStrike->aSnapShots[4].unknown0 = 0.5f;

    pStrike->aSnapShots[5].unknown0 = 0.7f;
    pStrike->aSnapShots[5].angle1 = pThing->renderData.apTweakedAngles[1].pitch;
    pStrike->aSnapShots[5].angle2 = pThing->renderData.apTweakedAngles[1].yaw;
    pStrike->aSnapShots[5].angle3 = pThing->renderData.apTweakedAngles[2].pitch;
    pStrike->aSnapShots[5].angle4 = pThing->renderData.apTweakedAngles[2].yaw;
    pStrike->aSnapShots[5].angle5 = pThing->renderData.apTweakedAngles[28].pitch;

    pStrike->unknown0 = 1;
    pStrike->unknown3 = 6;
}

void J3DAPI sithAIMove_StartQuetzAnim2(SithAIControlBlock* pLocal)
{
    SithThing* pThing = pLocal->pOwner;
    if ( !sithThing_CreateQuetzUserBlock(pThing) )
    {
        return;
    }

    SithQuetzStrike* pStrike = &pThing->userblock.pQuetz->strike;
    pStrike->unknown1 = 0.0f;
    pStrike->snapShot = 0;

    pStrike->aSnapShots[0].unknown0 = 0.0f;
    pStrike->aSnapShots[0].angle1 = pThing->renderData.apTweakedAngles[1].pitch;
    pStrike->aSnapShots[0].angle2 = pThing->renderData.apTweakedAngles[1].yaw;
    pStrike->aSnapShots[0].angle3 = pThing->renderData.apTweakedAngles[2].pitch;
    pStrike->aSnapShots[0].angle4 = pThing->renderData.apTweakedAngles[2].yaw;
    pStrike->aSnapShots[0].angle5 = pThing->renderData.apTweakedAngles[28].pitch;

    pStrike->aSnapShots[1].unknown0 = 0.11f;

    pStrike->aSnapShots[2].unknown0 = 0.24f;

    pStrike->aSnapShots[3].unknown0 = 0.5f;
    pStrike->aSnapShots[3].angle1 = pThing->renderData.apTweakedAngles[1].pitch;
    pStrike->aSnapShots[3].angle2 = pThing->renderData.apTweakedAngles[1].yaw;
    pStrike->aSnapShots[3].angle3 = pThing->renderData.apTweakedAngles[2].pitch;
    pStrike->aSnapShots[3].angle4 = pThing->renderData.apTweakedAngles[2].yaw;
    pStrike->aSnapShots[3].angle5 = pThing->renderData.apTweakedAngles[28].pitch;

    pStrike->unknown0 = 2;
    pStrike->unknown3 = 4;
}

void J3DAPI sithAIMove_StartQuetzAnim3(SithAIControlBlock* pLocal)
{
    SithThing* pThing = pLocal->pOwner;
    if ( !sithThing_CreateQuetzUserBlock(pThing) )
    {
        return;
    }

    SithQuetzStrike* pStrike = &pThing->userblock.pQuetz->strike;
    pStrike->unknown1 = 0.0f;
    pStrike->snapShot = 0;

    pStrike->aSnapShots[0].unknown0 = 0.0f;
    pStrike->aSnapShots[0].angle1 = pThing->renderData.apTweakedAngles[1].pitch;
    pStrike->aSnapShots[0].angle2 = pThing->renderData.apTweakedAngles[1].yaw;
    pStrike->aSnapShots[0].angle3 = pThing->renderData.apTweakedAngles[2].pitch;
    pStrike->aSnapShots[0].angle4 = pThing->renderData.apTweakedAngles[2].yaw;
    pStrike->aSnapShots[0].angle5 = pThing->renderData.apTweakedAngles[28].pitch;

    pStrike->aSnapShots[1].unknown0 = 0.1f;
    pStrike->aSnapShots[1].angle1 = -10.0f;
    pStrike->aSnapShots[1].angle2 = 0.0f;
    pStrike->aSnapShots[1].angle3 = 50.0f;
    pStrike->aSnapShots[1].angle4 = 0.0f;
    pStrike->aSnapShots[1].angle5 = -40.0f;

    pStrike->aSnapShots[2].unknown0 = 0.3f;
    pStrike->aSnapShots[2].angle1 = -15.0f;
    pStrike->aSnapShots[2].angle2 = 30.0f;
    pStrike->aSnapShots[2].angle3 = 60.0f;
    pStrike->aSnapShots[2].angle4 = 50.0f;
    pStrike->aSnapShots[2].angle5 = -60.0f;

    pStrike->aSnapShots[3].unknown0 = 0.6f;
    pStrike->aSnapShots[3].angle1 = -12.0f;
    pStrike->aSnapShots[3].angle2 = -30.0f;
    pStrike->aSnapShots[3].angle3 = 55.0f;
    pStrike->aSnapShots[3].angle4 = -50.0f;
    pStrike->aSnapShots[3].angle5 = -50.0f;

    pStrike->aSnapShots[4].unknown0 = 0.9f;
    pStrike->aSnapShots[4].angle1 = -15.0f;
    pStrike->aSnapShots[4].angle2 = 30.0f;
    pStrike->aSnapShots[4].angle3 = 60.0f;
    pStrike->aSnapShots[4].angle4 = 50.0f;
    pStrike->aSnapShots[4].angle5 = -35.0f;

    pStrike->aSnapShots[5].unknown0 = 1.2f;
    pStrike->aSnapShots[5].angle1 = -12.0f;
    pStrike->aSnapShots[5].angle2 = -30.0f;
    pStrike->aSnapShots[5].angle3 = 55.0f;
    pStrike->aSnapShots[5].angle4 = -50.0f;
    pStrike->aSnapShots[5].angle5 = -50.0f;

    pStrike->aSnapShots[6].unknown0 = 1.5f;
    pStrike->aSnapShots[6].angle1 = -15.0f;
    pStrike->aSnapShots[6].angle2 = 30.0f;
    pStrike->aSnapShots[6].angle3 = 60.0f;
    pStrike->aSnapShots[6].angle4 = 50.0f;
    pStrike->aSnapShots[6].angle5 = -60.0f;

    pStrike->aSnapShots[7].unknown0 = 1.8f;
    pStrike->aSnapShots[7].angle1 = -12.0f;
    pStrike->aSnapShots[7].angle2 = -30.0f;
    pStrike->aSnapShots[7].angle3 = 55.0f;
    pStrike->aSnapShots[7].angle4 = -50.0f;
    pStrike->aSnapShots[7].angle5 = -50.0f;

    pStrike->aSnapShots[8].unknown0 = 2.1f;
    pStrike->aSnapShots[8].angle1 = -15.0f;
    pStrike->aSnapShots[8].angle2 = 30.0f;
    pStrike->aSnapShots[8].angle3 = 60.0f;
    pStrike->aSnapShots[8].angle4 = 50.0f;
    pStrike->aSnapShots[8].angle5 = -35.0f;

    pStrike->aSnapShots[9].unknown0 = 2.4f;
    pStrike->aSnapShots[9].angle1 = -12.0f;
    pStrike->aSnapShots[9].angle2 = -30.0f;
    pStrike->aSnapShots[9].angle3 = 55.0f;
    pStrike->aSnapShots[9].angle4 = -50.0f;
    pStrike->aSnapShots[9].angle5 = -50.0f;

    pStrike->aSnapShots[10].unknown0 = 2.7f;
    pStrike->aSnapShots[10].angle1 = -15.0f;
    pStrike->aSnapShots[10].angle2 = 30.0f;
    pStrike->aSnapShots[10].angle3 = 60.0f;
    pStrike->aSnapShots[10].angle4 = 50.0f;
    pStrike->aSnapShots[10].angle5 = -60.0f;

    pStrike->aSnapShots[11].unknown0 = 3.0f;
    pStrike->aSnapShots[11].angle1 = -12.0f;
    pStrike->aSnapShots[11].angle2 = -30.0f;
    pStrike->aSnapShots[11].angle3 = 55.0f;
    pStrike->aSnapShots[11].angle4 = -50.0f;
    pStrike->aSnapShots[11].angle5 = -50.0f;

    pStrike->aSnapShots[12].unknown0 = 3.3f;
    pStrike->aSnapShots[12].angle1 = -10.0f;
    pStrike->aSnapShots[12].angle2 = 0.0f;
    pStrike->aSnapShots[12].angle3 = 20.0f;
    pStrike->aSnapShots[12].angle4 = 0.0f;
    pStrike->aSnapShots[12].angle5 = -15.0f;

    pStrike->aSnapShots[13].unknown0 = 3.7f;
    pStrike->aSnapShots[13].angle1 = 0.0f;
    pStrike->aSnapShots[13].angle2 = 0.0f;
    pStrike->aSnapShots[13].angle3 = 5.0f;
    pStrike->aSnapShots[13].angle4 = 0.0f;
    pStrike->aSnapShots[13].angle5 = -10.0f;

    pStrike->aSnapShots[14].unknown0 = 5.0f;
    pStrike->aSnapShots[14].angle1 = pThing->renderData.apTweakedAngles[1].pitch;
    pStrike->aSnapShots[14].angle2 = pThing->renderData.apTweakedAngles[1].yaw;
    pStrike->aSnapShots[14].angle3 = pThing->renderData.apTweakedAngles[2].pitch;
    pStrike->aSnapShots[14].angle4 = pThing->renderData.apTweakedAngles[2].yaw;
    pStrike->aSnapShots[14].angle5 = pThing->renderData.apTweakedAngles[28].pitch;

    pStrike->unknown0 = 3;
    pStrike->unknown3 = 15;
}

void J3DAPI sithAIMove_StartQuetzAnim5(SithAIControlBlock* pLocal)
{
    SithThing* pThing = pLocal->pOwner;
    if ( !sithThing_CreateQuetzUserBlock(pThing) )
    {
        return;
    }

    SithQuetzStrike* pStrike = &pThing->userblock.pQuetz->strike;
    pStrike->unknown1 = 0.0f;
    pStrike->snapShot = 0;

    pStrike->aSnapShots[0].unknown0 = 0.0f;
    pStrike->aSnapShots[0].angle1 = pThing->renderData.apTweakedAngles[1].pitch;
    pStrike->aSnapShots[0].angle2 = pThing->renderData.apTweakedAngles[1].yaw;
    pStrike->aSnapShots[0].angle3 = pThing->renderData.apTweakedAngles[2].pitch;
    pStrike->aSnapShots[0].angle4 = pThing->renderData.apTweakedAngles[2].yaw;
    pStrike->aSnapShots[0].angle5 = pThing->renderData.apTweakedAngles[28].pitch;

    pStrike->aSnapShots[1].unknown0 = 0.1f;
    pStrike->aSnapShots[1].angle1 = -10.0f;
    pStrike->aSnapShots[1].angle2 = 0.0f;
    pStrike->aSnapShots[1].angle3 = 50.0f;
    pStrike->aSnapShots[1].angle4 = 0.0f;
    pStrike->aSnapShots[1].angle5 = -40.0f;

    pStrike->aSnapShots[2].unknown0 = 0.3f;
    pStrike->aSnapShots[2].angle1 = -15.0f;
    pStrike->aSnapShots[2].angle2 = 30.0f;
    pStrike->aSnapShots[2].angle3 = 60.0f;
    pStrike->aSnapShots[2].angle4 = 50.0f;
    pStrike->aSnapShots[2].angle5 = -60.0f;

    pStrike->aSnapShots[3].unknown0 = 0.6f;
    pStrike->aSnapShots[3].angle1 = -12.0f;
    pStrike->aSnapShots[3].angle2 = -30.0f;
    pStrike->aSnapShots[3].angle3 = 55.0f;
    pStrike->aSnapShots[3].angle4 = -50.0f;
    pStrike->aSnapShots[3].angle5 = -50.0f;

    pStrike->aSnapShots[4].unknown0 = 0.9f;
    pStrike->aSnapShots[4].angle1 = -15.0f;
    pStrike->aSnapShots[4].angle2 = 30.0f;
    pStrike->aSnapShots[4].angle3 = 60.0f;
    pStrike->aSnapShots[4].angle4 = 50.0f;
    pStrike->aSnapShots[4].angle5 = -35.0f;

    pStrike->aSnapShots[5].unknown0 = 1.2f;
    pStrike->aSnapShots[5].angle1 = -12.0f;
    pStrike->aSnapShots[5].angle2 = -30.0f;
    pStrike->aSnapShots[5].angle3 = 55.0f;
    pStrike->aSnapShots[5].angle4 = -50.0f;
    pStrike->aSnapShots[5].angle5 = -50.0f;

    pStrike->aSnapShots[6].unknown0 = 1.5f;
    pStrike->aSnapShots[6].angle1 = -15.0f;
    pStrike->aSnapShots[6].angle2 = 30.0f;
    pStrike->aSnapShots[6].angle3 = 60.0f;
    pStrike->aSnapShots[6].angle4 = 50.0f;
    pStrike->aSnapShots[6].angle5 = -60.0f;

    pStrike->aSnapShots[7].unknown0 = 1.8f;
    pStrike->aSnapShots[7].angle1 = -12.0f;
    pStrike->aSnapShots[7].angle2 = -30.0f;
    pStrike->aSnapShots[7].angle3 = 55.0f;
    pStrike->aSnapShots[7].angle4 = -50.0f;
    pStrike->aSnapShots[7].angle5 = -50.0f;

    pStrike->aSnapShots[8].unknown0 = 2.1f;
    pStrike->aSnapShots[8].angle1 = -15.0f;
    pStrike->aSnapShots[8].angle2 = 30.0f;
    pStrike->aSnapShots[8].angle3 = 60.0f;
    pStrike->aSnapShots[8].angle4 = 50.0f;
    pStrike->aSnapShots[8].angle5 = -35.0f;

    pStrike->aSnapShots[9].unknown0 = 3.0f;
    pStrike->aSnapShots[9].angle1 = pThing->renderData.apTweakedAngles[1].pitch;
    pStrike->aSnapShots[9].angle2 = pThing->renderData.apTweakedAngles[1].yaw;
    pStrike->aSnapShots[9].angle3 = pThing->renderData.apTweakedAngles[2].pitch;
    pStrike->aSnapShots[9].angle4 = pThing->renderData.apTweakedAngles[2].yaw;
    pStrike->aSnapShots[9].angle5 = pThing->renderData.apTweakedAngles[28].pitch;

    pStrike->unknown0 = 5;
    pStrike->unknown3 = 10;
}

void J3DAPI sithAIMove_StartQuetzAnim4(SithAIControlBlock* pLocal)
{
    SithThing* pThing = pLocal->pOwner;
    if ( !sithThing_CreateQuetzUserBlock(pThing) )
    {
        return;
    }

    SithQuetzStrike* pStrike = &pThing->userblock.pQuetz->strike;
    pStrike->unknown1 = 0.0f;
    pStrike->snapShot = 0;

    pStrike->aSnapShots[0].unknown0 = 0.0f;
    pStrike->aSnapShots[0].angle1 = pThing->renderData.apTweakedAngles[1].pitch;
    pStrike->aSnapShots[0].angle2 = pThing->renderData.apTweakedAngles[1].yaw;
    pStrike->aSnapShots[0].angle3 = pThing->renderData.apTweakedAngles[2].pitch;
    pStrike->aSnapShots[0].angle4 = pThing->renderData.apTweakedAngles[2].yaw;
    pStrike->aSnapShots[0].angle5 = pThing->renderData.apTweakedAngles[28].pitch;

    pStrike->aSnapShots[1].unknown0 = 1.6f;
    pStrike->aSnapShots[1].angle1 = 0.0f;
    pStrike->aSnapShots[1].angle2 = 0.0f;
    pStrike->aSnapShots[1].angle3 = 0.0f;
    pStrike->aSnapShots[1].angle4 = 0.0f;
    pStrike->aSnapShots[1].angle5 = -60.0f;

    pStrike->aSnapShots[2].unknown0 = 5.0f;
    pStrike->aSnapShots[2].angle1 = 0.0f;
    pStrike->aSnapShots[2].angle2 = 0.0f;
    pStrike->aSnapShots[2].angle3 = 0.0f;
    pStrike->aSnapShots[2].angle4 = 0.0f;
    pStrike->aSnapShots[2].angle5 = -60.0f;

    pStrike->unknown0 = 4;
    pStrike->unknown3 = 3;
}

void J3DAPI sithAIMove_UpdateMardukTail(SithThing* pThing, float secDeltaTime)
{
    if ( stdUtil_StrCmp(pThing->aName, "marduk") == 0 || stdUtil_StrCmp(pThing->aName, "marduk2") == 0 )
    {
        int jointIndex = sithThing_GetThingJointIndex(pThing, "wmabdomen");
        if ( jointIndex != -1 )
        {
            float angle = stdMath_NormalizeAngle(720.0f * secDeltaTime);
            pThing->renderData.apTweakedAngles[jointIndex].yaw += angle;
        }
    }
}

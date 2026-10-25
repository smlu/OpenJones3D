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

//
// Body tracking vars
//
static float sithAIMove_bodyTrackMaxHeadYaw = 20.0f; // Max head yaw before the body turns again to face the target

//
// Mine car turn vars
//
static float sithAIMove_mineCarMaxUpDot         = 0.99900001f;
static float sithAIMove_mineCarTurnLimitScale   = 1.05f;
static float sithAIMove_mineCarMinSlowTurnAngle = 1.0f;

//
// Turn vars
//
static float sithAIMove_turnMaxUpDot         = 0.99900001f;
static float sithAIMove_turnLimitScale       = 1.05f;
static float sithAIMove_turnMinSlowTurnAngle = 1.0f;

//
// Move vars
//
static float sithAIMove_moveMinFacingDot  = 0.30000001f; // Min move speed scale when moving in facing direction
static float sithAIMove_goalSideTolerance = 0.050000001f;

//
// Boss turn vars
//
static float sithAIMove_bossMaxUpDot       = 0.99000001f;
static float sithAIMove_bossTurnLimitScale = 1.05f;

//
// Head tracking vars
//
static float sithAIMove_headTrackTargetStopDot  = -0.2f;
static float sithAIMove_headTrackTargetStartDot = 0.0f;
static float sithAIMove_headTrackDoneAngle      = 0.5f;

//
// Quetzalcoatl strike vars
//
static float sithAIMove_quetzStrikeMinDist       = 0.30000001f;
static float sithAIMove_quetzStrikeMaxDist       = 0.5f;
static float sithAIMove_quetzStrikeNearPitch     = -15.0f;
static float sithAIMove_quetzStrikeFarPitch      = -60.0f;
static float sithAIMove_quetzStrikeNearHeadPitch = 0.0f;
static float sithAIMove_quetzStrikeFarHeadPitch  = 60.0f;

static float sithAIMove_mardukTailYawSpeed = 720.0f; // In degrees per sec

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
{}

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

    if ( (pLocal->mode & SITHAI_MODE_SLEEPING) != 0
        && (pLocal->mode & SITHAI_MODE_BLOCK) == 0
        && (pThing->thingInfo.actorInfo.flags & SITH_AF_NOSLOPEMOVE) == 0 )
    {
        return;
    }

    if ( (pThing->flags & (SITH_TF_DISABLED | SITH_TF_DYING | SITH_TF_DESTROYED)) != 0 )
    {
        return;
    }

    if ( (pLocal->submode & SITHAI_SUBMODE_UNKNOWN_100) != 0
        && (pLocal->mode & SITHAI_MODE_MOVING) != 0
        && (pLocal->mode & SITHAI_MODE_DISABLED) == 0 )
    {
        rdVector3 lookPos = pLocal->movePos;
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
                else if ( fabsf(pThing->thingInfo.actorInfo.headPYR.y) > sithAIMove_bodyTrackMaxHeadYaw )
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
    SithThing* pThing         = pLocal->pOwner;
    SithActorInfo* pActorInfo = &pThing->thingInfo.actorInfo;
    if ( (pActorInfo->flags & SITH_AF_IMMOBILE) != 0 )
    {
        return;
    }

    int meshIdx = sithThing_GetThingMeshIndex(pThing, "redarmy");

    rdVector3 jointPos = pThing->renderData.paJointMatrices[meshIdx].dvec;

    rdVector3 toLook;
    rdVector_Sub3(&toLook, &pLocal->lookPos, &jointPos);

    rdVector3 lvec;
    rdVector_Rotate3(&lvec, &pThing->orient.lvec, &pThing->renderData.apTweakedAngles[meshIdx]);
    rdVector_Normalize3Acc(&lvec);

    rdVector3 aimDir;
    rdMath_ProjectPointOntoPlaneNormalized(&aimDir, &lvec, &pThing->orient.uvec, &rdroid_g_zeroVector3);

    rdVector3 rvec;
    rdVector_Cross3(&rvec, &aimDir, &pThing->orient.uvec);
    rdVector_Normalize3Acc(&rvec);

    // TODO: [BUG] The original never assigns the local moveState (sithAIMove_UpdateTurn sets it from sithAIMove_AIGetMoveState).
    //       In the debug build its own stack slot is read but never written (garbage). In retail the compiler shares the slot
    //       with the pLocal argument, which later holds angle: moveState is zero only when angle is +0.0f, and nonzero (the
    //       pLocal pointer) when the actor looks straight up or down and angle is never computed. Emulated as retail.
    int moveState = 1;

    float upDot = fabsf(rdVector_Dot3(&pThing->orient.uvec, &toLook));
    if ( upDot > sithAIMove_mineCarMaxUpDot )
    {
        if ( (pThing->moveInfo.physics.flags & SITH_PF_FLY) == 0 )
        {
            SITHLOG_ERROR("Strange... AI '%s' is looking straight up or down.\n", pThing->aName);
        }
    }
    else
    {
        rdVector3 lookDir;
        rdMath_ProjectPointOntoPlaneNormalized(&lookDir, &toLook, &pThing->orient.uvec, &rdroid_g_zeroVector3);

        float angle = rdMath_DeltaAngleNormalized(&lookDir, &aimDir, &pThing->orient.uvec);
        moveState   = angle == 0.0f && !signbit(angle) ? SITHAIMOVE_STATE_NONE : 1;

        float turnRate = pActorInfo->maxRotVelocity * secDeltaTime;
        float maxStep  = pActorInfo->maxRotVelocity * 0.07f;
        if ( fabsf(angle) <= maxStep && fabsf(angle) >= sithAIMove_mineCarMinSlowTurnAngle )
        {
            turnRate = (fabsf(angle) / maxStep) * turnRate;
        }

        float turnLimit = turnRate * sithAIMove_mineCarTurnLimitScale;
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
            rdVector3 pyr = { .pitch = 0.0f, .yaw = -((angle < 0.0f ? -1.0f : 1.0f) * turnRate), .roll = 0.0f };
            rdVector_Rotate3Acc(&aimDir, &pyr);

            float fwdDot   = rdVector_Dot3(&pThing->orient.lvec, &aimDir);
            float rightDot = rdVector_Dot3(&pThing->orient.rvec, &aimDir);
            pThing->renderData.apTweakedAngles[meshIdx].y = 90.0f - stdMath_ArcSin3(fwdDot);
            if ( rightDot > 0.0f )
            {
                pThing->renderData.apTweakedAngles[meshIdx].y = -pThing->renderData.apTweakedAngles[meshIdx].y;
            }

            if ( moveState == SITHAIMOVE_STATE_NONE )
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

    if ( moveState == SITHAIMOVE_STATE_NONE )
    {
        pThing->moveStatus = SITHPLAYERMOVE_STILL;
    }
}

void J3DAPI sithAIMove_UpdateTurn(SithAIControlBlock* pLocal, float secDeltatTime)
{
    SithThing* pOwner         = pLocal->pOwner;
    SithActorInfo* pActorInfo = &pOwner->thingInfo.actorInfo;
    rdVector3 goalLVec        = pLocal->goalLVec;

    if ( (pActorInfo->flags & SITH_AF_IMMOBILE) != 0 )
    {
        return;
    }

    SithAIMoveState moveState = sithAIMove_AIGetMoveState(pLocal);
    if ( moveState == SITHAIMOVE_STATE_STATIONARY )
    {
        return;
    }

    float upDot = fabsf(rdVector_Dot3(&pOwner->orient.uvec, &goalLVec));
    if ( upDot > sithAIMove_turnMaxUpDot )
    {
        if ( (pOwner->moveInfo.physics.flags & SITH_PF_FLY) == 0 )
        {
            SITHLOG_ERROR("Strange... AI '%s' is looking straight up or down.\n", pOwner->aName);
        }
    }
    else
    {
        rdVector3 lvec;
        rdMath_ProjectPointOntoPlaneNormalized(&lvec, &goalLVec, &pOwner->orient.uvec, &rdroid_g_zeroVector3);

        float angle = rdMath_DeltaAngleNormalized(&lvec, &pOwner->orient.lvec, &pOwner->orient.uvec);
        if ( sithAIMove_AISpecialTurn(pLocal, angle) )
        {
            return;
        }

        float turnRate = pActorInfo->maxRotVelocity * secDeltatTime;
        float maxStep  = pActorInfo->maxRotVelocity * 0.07f;
        if ( fabsf(angle) <= maxStep && fabsf(angle) >= sithAIMove_turnMinSlowTurnAngle )
        {
            turnRate = (fabsf(angle) / maxStep) * turnRate;
        }

        float turnLimit = turnRate * sithAIMove_turnLimitScale;
        if ( fabsf(angle) <= turnLimit )
        {
            pOwner->orient.lvec = lvec;
            rdVector_Cross3(&pOwner->orient.rvec, &pOwner->orient.lvec, &pOwner->orient.uvec);
            rdVector_Normalize3Acc(&pOwner->orient.rvec);
            rdVector_Cross3(&pOwner->orient.uvec, &pOwner->orient.rvec, &pOwner->orient.lvec);
        }
        else
        {
            rdVector3 pyr = { .pitch = 0.0f, .yaw = -((angle < 0.0f ? -1.0f : 1.0f) * turnRate), .roll = 0.0f };
            rdMatrix_PreRotate34(&pOwner->orient, &pyr);
            rdMatrix_Normalize34(&pOwner->orient);

            if ( moveState == SITHAIMOVE_STATE_NONE )
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

    if ( moveState == SITHAIMOVE_STATE_NONE )
    {
        pOwner->moveStatus = SITHPLAYERMOVE_STILL;
    }
}

void J3DAPI sithAIMove_UpdateMove(SithAIControlBlock* pLocal, float secDeltaTime)
{
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

    float moveDist    = (pThing->thingInfo.actorInfo.maxThrust * secDeltaTime) * pLocal->moveSpeed;
    rdVector3 pos     = pThing->pos;
    rdVector3 movePos = pLocal->movePos;

    float goalDist = sithAIMove_GetGoalTolerance(pLocal, &pos, &movePos);

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
        dot = STDMATH_CLAMP(dot, sithAIMove_moveMinFacingDot, 1.0f);
        moveDist *= dot;
        rdVector_Scale3(&vel, &pThing->orient.lvec, moveDist);
    }

    vel.x = stdMath_ClipNearZero(vel.x);
    vel.y = stdMath_ClipNearZero(vel.y);
    vel.z = stdMath_ClipNearZero(vel.z);

    if ( (pLocal->mode & SITHAI_MODE_UNKNOWN_8000) != 0 && pLocal->goalThing )
    {
        rdVector3 goalDir = pLocal->goalThing->moveInfo.physics.velocity;
        float goalSpeed   = rdVector_Normalize3Acc(&goalDir);

        rdVector3 dir = pThing->moveInfo.physics.velocity;
        rdVector_Normalize3Acc(&dir);
        rdVector_Scale3(&pThing->moveInfo.physics.velocity, &dir, goalSpeed);
    }
    else
    {
        rdVector_Add3Acc(&pThing->moveInfo.physics.velocity, &vel);
    }

    if ( sithAIMove_AIGetMoveState(pLocal) == SITHAIMOVE_STATE_STATIONARY )
    {
        rdVector_Zero3(&pLocal->pOwner->moveInfo.physics.velocity);
    }

    if ( (pLocal->submode & SITHAI_SUBMODE_UNKNOWN_100) != 0 )
    {
        float speed = rdVector_Len3(&pThing->moveInfo.physics.velocity);
        rdVector_Scale3(&pThing->moveInfo.physics.velocity, &pThing->orient.lvec, speed);
    }

    bool bGoalReached = false;
    if ( (pLocal->mode & SITHAI_MODE_NOCHECKFORCLIFF) == 0 && !rdVector_IsZero3(&pThing->moveInfo.physics.velocity) )
    {
        if ( (pThing->moveInfo.physics.flags & SITH_PF_FLY) == 0 && pThing->attach.flags )
        {
            bGoalReached = sithAIMove_CheckWalkMove(pLocal, secDeltaTime);
        }
        else if ( (pThing->moveInfo.physics.flags & SITH_PF_FLY) != 0
            && (pThing->thingInfo.actorInfo.flags & SITH_AF_BREATHEUNDERWATER) == 0 )
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
        bGoalReached = true;
    }

    if ( !bGoalReached )
    {
        return;
    }

    if ( (pLocal->submode & SITHAI_SUBMODE_UNKNOWN_2) != 0 )
    {
        sithAIMove_StopAIMovement(pLocal);
        sithAIMove_AISetMovePos(pLocal, &pLocal->vecUnknown3, pLocal->moveSpeed);
        return;
    }

    if ( (pLocal->submode & SITHAI_SUBMODE_UNKNOWN_8) != 0 )
    {
        SithAIUtilMkPointFlags pointFlags = 0;
        if ( (pLocal->mode & SITHAI_MODE_ACTIVE) != 0 )
        {
            pointFlags |= SITHAIUTIL_MKPOINT_TOWARDGOAL;
        }

        if ( sithAIUtil_SweepMoveToPathPos(pLocal, &pLocal->vecUnknown3, 45.0f, 90.0f, 45.0f, pointFlags) )
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

float J3DAPI sithAIMove_GetGoalTolerance(SithAIControlBlock* pLocal, rdVector3* pPos, rdVector3* pMovePos)
{
    SithThing* pThing = pLocal->pOwner;
    SITH_ASSERTREL(pThing);

    if ( (pThing->pInSector->flags & SITH_SECTOR_UNDERWATER) != 0 || (pThing->moveInfo.physics.flags & SITH_PF_FLY) != 0 )
    {
        return J3DMIN(pThing->collide.movesize, 0.05f);
    }

    if ( (pLocal->mode & SITHAI_MODE_WALLCRAWLING) != 0 )
    {
        float minDist = J3DMIN(pThing->collide.movesize, 0.03f);
        if ( pThing->moveStatus == SITHPLAYERMOVE_MOUNTING_WALL )
        {
            return minDist;
        }

        int numAligned = 0;

        rdVector3 delta;
        rdVector_Sub3(&delta, pMovePos, pPos);
        if ( fabsf(delta.x) <= minDist )
        {
            ++numAligned;
            pMovePos->x = pPos->x;
            delta.x     = 0.0f;
        }

        if ( fabsf(delta.y) <= minDist )
        {
            ++numAligned;
            pMovePos->y = pPos->y;
            delta.y     = 0.0f;
        }

        if ( fabsf(delta.z) <= minDist )
        {
            ++numAligned;
            pMovePos->z = pPos->z;
            delta.z     = 0.0f;
        }

        float distance  = 0.0f;
        bool bOnSurface = false;
        if ( (pThing->attach.flags & SITH_ATTACH_SURFACE) != 0 )
        {
            distance = rdVector_Normalize3Acc(&delta);

            float dot = rdVector_Dot3(&pThing->attach.attachedToStructure.pSurfaceAttached->face.normal, &delta);
            if ( fabsf(dot) < 0.996f )
            {
                bOnSurface = true;
            }
        }

        if ( !bOnSurface )
        {
            if ( numAligned == 2 || distance <= pThing->collide.movesize * 2.0f )
            {
                *pMovePos = *pPos;
            }
            else
            {
                sithAIMove_Unreachable(pLocal);
            }
        }

        return minDist;
    }

    float minDist = J3DMIN(pThing->collide.movesize, 0.03f);
    pPos->z       = pThing->pos.z;
    pMovePos->z   = pThing->pos.z;

    if ( (pLocal->submode & SITHAI_SUBMODE_UNKNOWN_100) != 0 )
    {
        rdVector3 rightPos;
        rdVector_ScaleAdd3(&rightPos, &pThing->orient.rvec, sithAIMove_goalSideTolerance, pPos);
        float rightDist = stdMath_Dist2D1(rightPos.x - pMovePos->x, rightPos.y - pMovePos->y);

        rdVector3 leftPos;
        rdVector_ScaleAdd3(&leftPos, &pThing->orient.rvec, -sithAIMove_goalSideTolerance, pPos);
        float leftDist = stdMath_Dist2D1(leftPos.x - pMovePos->x, leftPos.y - pMovePos->y);

        if ( rightDist < sithAIMove_goalSideTolerance || leftDist < sithAIMove_goalSideTolerance )
        {
            *pMovePos = *pPos;
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
        case SITHAIUTIL_FLOOR_OK:
            break;

        case SITHAIUTIL_FLOOR_BLOCKED:
        case SITHAIUTIL_FLOOR_NONE:
            sithAI_EmitEvent(pLocal, SITHAI_EVENT_HIT_CLIFF, NULL);
            break;

        case SITHAIUTIL_FLOOR_WALL:
            if ( !sithAI_HasInstinct(pLocal, "wallcrawl") )
            {
                sithAI_EmitEvent(pLocal, SITHAI_EVENT_HIT_CLIFF, NULL);
            }

            break;

        case SITHAIUTIL_FLOOR_THING:
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
        return 0;
    }

    rdVector3 endPos;
    rdVector_ScaleAdd3(&endPos, &pThing->moveInfo.physics.velocity, secDeltaTime, &pThing->pos);

    rdVector3 curDir;
    sithAIUtil_GetXYHeadingVector(pThing, &curDir);
    rdVector_ScaleAdd3(&endPos, &curDir, pThing->collide.movesize + 0.05f, &endPos);

    SithSector* pSector = sithCollision_FindSectorInRadius(pThing->pInSector, &pThing->pos, &endPos, 0.0f);
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

    return 0;
}

int J3DAPI sithAIMove_CheckSwimMove(SithAIControlBlock* pLocal, float secDeltaTime)
{
    SithThing* pThing = pLocal->pOwner;

    if ( pThing->moveInfo.physics.velocity.z != 0.0f )
    {
        rdVector3 surfacePos = pThing->pos;
        if ( (pLocal->submode & SITHAI_SUBMODE_SWIMNEARSURFACE) != 0 )
        {
            surfacePos.z += pThing->collide.movesize * 0.2f;
        }
        else
        {
            surfacePos.z += pThing->collide.movesize * 0.5f;
        }

        SithSector* pSurfaceSector = sithCollision_FindSectorInRadius(pThing->pInSector, &pThing->pos, &surfacePos, 0.0f);
        if ( pSurfaceSector && (pSurfaceSector->flags & SITH_SECTOR_UNDERWATER) == 0 )
        {
            pThing->moveInfo.physics.velocity.z = -0.05f;
            if ( pLocal->movePos.z > pThing->pos.z )
            {
                pLocal->movePos.z = pThing->pos.z - 0.05f;
            }
        }
    }

    rdVector3 endPos;
    rdVector_ScaleAdd3(&endPos, &pThing->moveInfo.physics.velocity, secDeltaTime, &pThing->pos);

    rdVector3 curDir;
    sithAIUtil_GetXYHeadingVector(pLocal->pOwner, &curDir);
    rdVector_ScaleAdd3(&endPos, &curDir, pLocal->pOwner->collide.movesize + 0.03f, &endPos);

    SithSector* pSector = sithCollision_FindSectorInRadius(pThing->pInSector, &pThing->pos, &endPos, 0.0f);
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

SithAIMoveState J3DAPI sithAIMove_AIGetMoveState(const SithAIControlBlock* pLocal)
{
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
            return SITHAIMOVE_STATE_STATIONARY;

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
            return SITHAIMOVE_STATE_ANIMATED;

        default:
            return SITHAIMOVE_STATE_NONE;
    }
}

int J3DAPI sithAIMove_AISpecialTurn(SithAIControlBlock* pLocal, float angle)
{
    if ( (pLocal->submode & SITHAI_SUBMODE_SPECIALTURNS) == 0 )
    {
        return 0;
    }

    if ( sithAIMove_AIGetMoveState(pLocal) != SITHAIMOVE_STATE_NONE )
    {
        return 1;
    }

    float absAngle = fabsf(angle);
    if ( absAngle < 22.5f )
    {
        return 0;
    }

    SithActorSpecialMoveFlags dirFlag = angle >= 0.0f ? SITHACTORSPECIALMOVE_DIR_RIGHT : SITHACTORSPECIALMOVE_DIR_LEFT;

    bool bTurned = false;
    if ( absAngle >= 112.5f && absAngle <= 180.0f )
    {
        bool bTurn135 = absAngle >= 112.5f && absAngle <= 157.5f;
        if ( bTurn135 && sithAIMove_AISpecialMove(pLocal, (SithActorSpecialMoveFlags)(dirFlag | SITHACTORSPECIALMOVE_TURN135)) )
        {
            bTurned = true;
        }

        if ( !bTurned )
        {
            bTurned = sithAIMove_AISpecialMove(pLocal, SITHACTORSPECIALMOVE_TURN180) != 0;
        }
    }
    else
    {
        bool bTurn90 = absAngle >= 67.5f && absAngle <= 112.5f;
        if ( bTurn90 && sithAIMove_AISpecialMove(pLocal, (SithActorSpecialMoveFlags)(dirFlag | SITHACTORSPECIALMOVE_TURN90)) )
        {
            bTurned = true;
        }
    }

    if ( !bTurned )
    {
        return sithAIMove_AISpecialMove(pLocal, (SithActorSpecialMoveFlags)(dirFlag | SITHACTORSPECIALMOVE_TURN45)) != 0;
    }

    return 1;
}

float J3DAPI sithAIMove_UpdateAIMove(SithAIControlBlock* pLocal)
{
    SithPhysicsInfo* pPhysics = &pLocal->pOwner->moveInfo.physics;
    SithThing* pThing         = pLocal->pOwner;

    float moveSpeed                   = 0.0f;
    SithPuppetSubMode newSubmode      = SITHPUPPETSUBMODE_STAND;
    SithThingMoveStatus newMoveStatus = SITHPLAYERMOVE_STILL;
    float minFlyMoveSpeed             = 0.0099999998f;

    SITH_ASSERTREL(pThing);

    if ( pThing->pInSector
        && (!rdVector_IsZero3(&pPhysics->velocity) || (pLocal->mode & SITHAI_MODE_TURNING) != 0)
        && (pThing->thingInfo.actorInfo.flags & SITH_AF_IMMOBILE) == 0 )
    {
        // Transform to local velocity
        rdVector3 velocity;
        rdMatrix_TransformVectorOrtho34(&velocity, &pPhysics->velocity, &pThing->orient);

        if ( (pPhysics->flags & SITH_PF_FLY) != 0
            && fabsf(velocity.z) > fabsf(velocity.y)
            && fabsf(velocity.z) > fabsf(velocity.x) )
        {
            moveSpeed = velocity.z;
            if ( moveSpeed > minFlyMoveSpeed )
            {
                newMoveStatus = SITHPLAYERMOVE_UNKNOWN_103;
                newSubmode    = SITHPUPPETSUBMODE_RISING;
            }
            else if ( moveSpeed < -minFlyMoveSpeed )
            {
                newMoveStatus = SITHPLAYERMOVE_UNKNOWN_104;
                newSubmode    = SITHPUPPETSUBMODE_FALLFORWARD;
            }
        }
        else
        {
            int moveDirection;
            if ( fabsf(velocity.z) > fabsf(velocity.y) && fabsf(velocity.z) > fabsf(velocity.x) )
            {
                moveSpeed     = velocity.z;
                moveDirection = 2; // z - up
            }
            else if ( fabsf(velocity.y) > fabsf(velocity.x) )
            {
                moveSpeed     = velocity.y;
                moveDirection = 1; // y - forward
            }
            else
            {
                moveSpeed     = velocity.x;
                moveDirection = 0; // x - right
            }

            sithAIMove_GetAIMoveModes(pThing, moveSpeed, moveDirection, &newMoveStatus, &newSubmode);
        }
    }

    if ( pThing->thingInfo.actorInfo.bControlsDisabled || pThing->thingInfo.actorInfo.bForceMovePlay )
    {
        return moveSpeed;
    }

    switch ( sithAIMove_AIGetMoveState(pLocal) )
    {
        case SITHAIMOVE_STATE_NONE:
            break;

        case SITHAIMOVE_STATE_ANIMATED:
            return moveSpeed;

        case SITHAIMOVE_STATE_STATIONARY:
            return 0.0f;
    }

    if ( pThing->pPuppetState->submode != newSubmode )
    {
        sithAIMove_SetSubMode(pLocal, newMoveStatus, newSubmode);
    }

    return moveSpeed;
}

void J3DAPI sithAIMove_GetAIMoveModes(const SithThing* pThing, float moveSpeed, int moveDirection, SithThingMoveStatus* pOutMoveStatus, SithPuppetSubMode* pOutSubmode)
{
    SithAIControlBlock* pLocal = pThing->controlInfo.aiControl.pLocal;

    float minWalkSpeed = 0.001f;
    float maxWalkSpeed = 0.27000001f;
    float maxTurnSpeed = 0.0049999999f;

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

    if ( (pLocal->mode & SITHAI_MODE_TURNING) != 0 )
    {
        if ( (pThing->thingInfo.actorInfo.flags & SITH_AF_FLYERMOVE) != 0 )
        {
            if ( pThing->moveStatus == SITHPLAYERMOVE_UNKNOWN_105 )
            {
                *pOutMoveStatus = SITHPLAYERMOVE_TURNING_LEFT;
                *pOutSubmode    = SITHPUPPETSUBMODE_TURNLEFT;
                return;
            }

            if ( pThing->moveStatus == SITHPLAYERMOVE_UNKNOWN_106 )
            {
                *pOutMoveStatus = SITHPLAYERMOVE_TURNING_RIGHT;
                *pOutSubmode    = SITHPUPPETSUBMODE_TURNRIGHT;
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

        if ( fabsf(moveSpeed) < maxTurnSpeed )
        {
            if ( pThing->moveStatus == SITHPLAYERMOVE_TURNING_LEFT || pThing->moveStatus == SITHPLAYERMOVE_UNKNOWN_105 )
            {
                *pOutMoveStatus = SITHPLAYERMOVE_TURNING_LEFT;
                *pOutSubmode    = SITHPUPPETSUBMODE_TURNLEFT;
            }
            else
            {
                *pOutMoveStatus = SITHPLAYERMOVE_TURNING_RIGHT;
                *pOutSubmode    = SITHPUPPETSUBMODE_TURNRIGHT;
            }

            return;
        }
    }

    switch ( moveDirection )
    {
        case 0: // x - right
            if ( fabsf(moveSpeed) >= maxWalkSpeed )
            {
                *pOutMoveStatus = SITHPLAYERMOVE_RUNNING;
                *pOutSubmode    = SITHPUPPETSUBMODE_RUN;
            }
            else if ( fabsf(moveSpeed) >= minWalkSpeed && fabsf(moveSpeed) < maxWalkSpeed )
            {
                *pOutMoveStatus = SITHPLAYERMOVE_WALKING;
                *pOutSubmode    = SITHPUPPETSUBMODE_WALK;
            }
            else
            {
                *pOutMoveStatus = SITHPLAYERMOVE_STILL;
                *pOutSubmode    = SITHPUPPETSUBMODE_STAND;
            }

            break;

        case 2: // z - up
            if ( !pThing->attach.flags )
            {
                if ( moveSpeed >= -minWalkSpeed && moveSpeed <= minWalkSpeed )
                {
                    *pOutMoveStatus = SITHPLAYERMOVE_STILL;
                    *pOutSubmode    = SITHPUPPETSUBMODE_STAND;
                }
                else if ( fabsf(moveSpeed) > minWalkSpeed )
                {
                    *pOutMoveStatus = SITHPLAYERMOVE_FALLING;
                    *pOutSubmode    = SITHPUPPETSUBMODE_FALL;
                }

                break;
            }

            // Fall through

        case 1: // y - forward
            if ( moveSpeed >= maxWalkSpeed )
            {
                *pOutMoveStatus = SITHPLAYERMOVE_RUNNING;
                *pOutSubmode    = SITHPUPPETSUBMODE_RUN;
            }
            else if ( moveSpeed >= minWalkSpeed && moveSpeed < maxWalkSpeed )
            {
                *pOutMoveStatus = SITHPLAYERMOVE_WALKING;
                *pOutSubmode    = SITHPUPPETSUBMODE_WALK;
            }
            else if ( moveSpeed > -minWalkSpeed && moveSpeed < minWalkSpeed )
            {
                *pOutMoveStatus = SITHPLAYERMOVE_STILL;
                *pOutSubmode    = SITHPUPPETSUBMODE_STAND;
            }
            else if ( moveSpeed <= -minWalkSpeed )
            {
                *pOutMoveStatus = SITHPLAYERMOVE_WALKING;
                *pOutSubmode    = SITHPUPPETSUBMODE_WALKBACK;
            }

            break;

        default:
            *pOutMoveStatus = SITHPLAYERMOVE_STILL;
            *pOutSubmode    = SITHPUPPETSUBMODE_STAND;
            break;
    }
}

void J3DAPI sithAIMove_SetSubMode(SithAIControlBlock* pLocal, SithThingMoveStatus moveStatus, SithPuppetSubMode submode)
{
    SithThing* pThing = pLocal->pOwner;
    if ( !pLocal->pOwner || !pThing->pPuppetClass || !pThing->pPuppetState )
    {
        SITHLOG_ERROR("sithAIMove_SetSubMode: Bad puppet data for AI '%s'.\n", pThing->aName); // TODO: potential null pointer dereference, pThing can be null here
        return;
    }

    int majorMode                = pThing->pPuppetState->majorMode;
    SithPuppetSubMode curSubmode = pThing->pPuppetState->submode;

    SithPuppetSubMode transitionSubmode      = 0;
    SithThingMoveStatus transitionMoveStatus = 0;
    switch ( curSubmode )
    {
        case SITHPUPPETSUBMODE_STAND:
        case SITHPUPPETSUBMODE_TURNLEFT:
        case SITHPUPPETSUBMODE_TURNRIGHT:
            if ( (unsigned int)submode >= SITHPUPPETSUBMODE_WALK && (unsigned int)submode <= SITHPUPPETSUBMODE_WALKBACK )
            {
                transitionSubmode    = SITHPUPPETSUBMODE_STAND2WALK;
                transitionMoveStatus = SITHPLAYERMOVE_STAND2WALK;
            }

            break;

        case SITHPUPPETSUBMODE_WALK:
            if ( submode == SITHPUPPETSUBMODE_STAND
                || (unsigned int)submode > SITHPUPPETSUBMODE_STRAFERIGHT && (unsigned int)submode <= SITHPUPPETSUBMODE_TURNRIGHT )
            {
                transitionSubmode    = SITHPUPPETSUBMODE_WALK2STAND;
                transitionMoveStatus = SITHPLAYERMOVE_WALK2STAND;
            }

            break;

        case SITHPUPPETSUBMODE_RUN:
            if ( submode == SITHPUPPETSUBMODE_STAND
                || (unsigned int)submode > SITHPUPPETSUBMODE_STRAFERIGHT && (unsigned int)submode <= SITHPUPPETSUBMODE_TURNRIGHT )
            {
                transitionSubmode    = SITHPUPPETSUBMODE_WALK2STAND;
                transitionMoveStatus = SITHPLAYERMOVE_WALK2STAND;
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
    rdPuppetTrackCallback pfCallback = NULL;
    if ( transitionSubmode && pThing->pPuppetClass->aModes[majorMode][transitionSubmode].pKeyframe )
    {
        sithPuppet_ClearMode(pThing, curSubmode);

        moveStatus = transitionMoveStatus;
        submode    = transitionSubmode;
        pfCallback = sithAIMove_PuppetCallback;

        if ( transitionSubmode == SITHPUPPETSUBMODE_STAND2WALK )
        {
            rdVector_Zero3(&pThing->moveInfo.physics.velocity);
            sithSoundClass_PlayModeFirst(pThing, SITHSOUNDCLASS_STAND2WALK);
        }
        else
        {
            sithSoundClass_PlayModeFirst(pThing, SITHSOUNDCLASS_WALK2STAND);
        }
    }

    const rdKeyframe* pKeyframe = pThing->pPuppetClass->aModes[majorMode][submode].pKeyframe;
    if ( pKeyframe )
    {
        SithPuppetTrack* pTrack = sithPuppet_FindActiveTrack(pThing, pKeyframe);
        if ( pTrack )
        {
            sithPuppet_SynchMode(pThing, pTrack->submode, submode, -1.0f, 0);
        }
        else
        {
            sithPuppet_SetSubMode(pThing, submode, pfCallback);
        }
    }

    pThing->pPuppetState->submode = submode;
    pThing->moveStatus            = moveStatus;
}

void J3DAPI sithAIMove_PuppetCallback(SithThing* pThing, int trackNum, rdKeyMarkerType marker)
{
    SITH_ASSERTREL(pThing);

    SithAIControlBlock* pLocal = pThing->controlInfo.aiControl.pLocal;
    if ( !pLocal )
    {
        sithPuppet_DefaultCallback(pThing, trackNum, marker);
        return;
    }

    switch ( marker )
    {
        case (rdKeyMarkerType)0: // Finish playing
            sithPuppet_FreeTrackByIndex(pThing, trackNum);
            if ( (pThing->flags & (SITH_TF_DYING | SITH_TF_DESTROYED)) != 0 )
            {
                return;
            }

            switch ( pThing->moveStatus )
            {
                case SITHPLAYERMOVE_LAND:
                    pThing->moveStatus = SITHPLAYERMOVE_STILL;
                    break;

                case SITHPLAYERMOVE_MOUNTING_WALL:
                    sithAIMove_AIFinalizeSpecialMove(pLocal, SITHACTORSPECIALMOVE_MOUNTWALL);
                    pThing->moveStatus = SITHPLAYERMOVE_WALKING;
                    break;

                case SITHPLAYERMOVE_ROLLING_LEFT:
                case SITHPLAYERMOVE_ROLLING_RIGHT:
                    sithAIMove_AIFinalizeSpecialMove(pLocal, SITHACTORSPECIALMOVE_ROLL);
                    pThing->moveStatus = SITHPLAYERMOVE_STILL;
                    sithAIMove_SetGoalReached(pLocal);
                    break;

                case SITHPLAYERMOVE_STRAFING_LEFT:
                case SITHPLAYERMOVE_STRAFING_RIGHT:
                    pThing->moveStatus = SITHPLAYERMOVE_STILL;
                    sithAIMove_SetGoalReached(pLocal);
                    break;

                case SITHPLAYERMOVE_WALK2STAND:
                case SITHPLAYERMOVE_UNKNOWN_78:
                    pThing->moveStatus = SITHPLAYERMOVE_STILL;
                    break;

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

                    break;

                case SITHPLAYERMOVE_KNOCKEDOUT:
                    pThing->thingInfo.actorInfo.bControlsDisabled = 0;
                    pThing->moveStatus                            = SITHPLAYERMOVE_STILL;
                    pThing->collide.type                          = SITH_COLLIDE_FACE;

                    pThing->controlInfo.aiControl.pLocal->mode &= ~SITHAI_MODE_DISABLED;
                    pThing->thingInfo.actorInfo.flags          &= ~(SITH_AF_IMMOBILE | SITH_AF_INVULNERABLE);
                    break;

                case SITHPLAYERMOVE_TURNING_LEFT_45_DEGREES:
                case SITHPLAYERMOVE_TURNING_RIGHT_45_DEGREES:
                    sithAIMove_AIFinalizeSpecialMove(pLocal, SITHACTORSPECIALMOVE_TURN45);
                    pThing->moveStatus = SITHPLAYERMOVE_WALKING;
                    break;

                case SITHPLAYERMOVE_TURNING_LEFT_90_DEGREES:
                case SITHPLAYERMOVE_TURNING_RIGHT_90_DEGREES:
                    sithAIMove_AIFinalizeSpecialMove(pLocal, SITHACTORSPECIALMOVE_TURN90);
                    pThing->moveStatus = SITHPLAYERMOVE_WALKING;
                    break;

                case SITHPLAYERMOVE_TURNING_LEFT_135_DEGREES:
                case SITHPLAYERMOVE_TURNING_RIGHT_135_DEGREES:
                    sithAIMove_AIFinalizeSpecialMove(pLocal, SITHACTORSPECIALMOVE_TURN135);
                    pThing->moveStatus = SITHPLAYERMOVE_WALKING;
                    break;

                case SITHPLAYERMOVE_TURNING_180_DEGREES:
                    sithAIMove_AIFinalizeSpecialMove(pLocal, SITHACTORSPECIALMOVE_TURN180);
                    pThing->moveStatus = SITHPLAYERMOVE_WALKING;
                    break;

                default:
                    return;
            }

            // Continue in the mode the finished animation leads to
            switch ( pThing->moveStatus )
            {
                case SITHPLAYERMOVE_STILL:
                    sithPuppet_SetSubMode(pThing, SITHPUPPETSUBMODE_STAND, NULL);
                    break;

                case SITHPLAYERMOVE_WALKING:
                    if ( rdVector_IsZero3(&pThing->moveInfo.physics.velocity) )
                    {
                        pThing->moveInfo.physics.velocity.y = 0.059999999f;
                    }
                    else if ( rdVector_Len3(&pThing->moveInfo.physics.velocity) < 0.059999999f )
                    {
                        rdVector_Scale3Acc(&pThing->moveInfo.physics.velocity, 1.0599999f);
                    }

                    sithPuppet_SetSubMode(pThing, SITHPUPPETSUBMODE_WALK, NULL);
                    break;

                case SITHPLAYERMOVE_RUNNING:
                    if ( rdVector_IsZero3(&pThing->moveInfo.physics.velocity) )
                    {
                        pThing->moveInfo.physics.velocity.y = 0.27000001f;
                    }

                    if ( rdVector_Len3(&pThing->moveInfo.physics.velocity) < 0.27000001f )
                    {
                        rdVector_Scale3Acc(&pThing->moveInfo.physics.velocity, 1.27f);
                    }

                    sithPuppet_SetSubMode(pThing, SITHPUPPETSUBMODE_RUN, NULL);
                    break;
            }

            return;

        case RDKEYMARKER_LEFTFOOT:
            if ( pThing->moveStatus != SITHPLAYERMOVE_STRAFING_RIGHT )
            {
                sithPuppet_DefaultCallback(pThing, trackNum, marker);
                return;
            }

            sithAIMove_AIFinalizeSpecialMove(pLocal, SITHACTORSPECIALMOVE_STRAFE);
            return;

        case RDKEYMARKER_RIGHTFOOT:
            if ( pThing->moveStatus != SITHPLAYERMOVE_STRAFING_LEFT )
            {
                sithPuppet_DefaultCallback(pThing, trackNum, marker);
                return;
            }

            sithAIMove_AIFinalizeSpecialMove(pLocal, SITHACTORSPECIALMOVE_STRAFE);
            return;

        case RDKEYMARKER_DIED:
            switch ( pThing->moveStatus )
            {
                case SITHPLAYERMOVE_ROLLING_LEFT:
                case SITHPLAYERMOVE_ROLLING_RIGHT:
                    sithSoundClass_PlayModeRandom(pThing, SITHSOUNDCLASS_RESERVED3);
                    return;

                case SITHPLAYERMOVE_KNOCKEDOUT:
                    sithSoundClass_PlayModeRandom(pThing, SITHSOUNDCLASS_RESERVED1);
                    return;

                case SITHPLAYERMOVE_RUNOVER:
                    sithSoundClass_PlayModeRandom(pThing, SITHSOUNDCLASS_RESERVED2);
                    return;

                default:
                    sithPuppet_DefaultCallback(pThing, trackNum, marker);
                    return;
            }

        default:
            sithPuppet_DefaultCallback(pThing, trackNum, marker);
            return;
    }
}

void J3DAPI sithAIMove_AISetLookThing(SithAIControlBlock* pLocal, const SithThing* pThing)
{
    SITH_ASSERTREL(pLocal && pLocal->pOwner && pLocal->pClass);
    SITH_ASSERTREL(pThing);

    rdVector3 eyePos;
    if ( pThing->type == SITH_THING_ACTOR || pThing->type == SITH_THING_PLAYER )
    {
        rdVector3 eyeOffset;
        rdMatrix_TransformVector34(&eyeOffset, &pThing->thingInfo.actorInfo.eyeOffset, &pThing->orient);
        rdVector_Add3(&eyePos, &pThing->pos, &eyeOffset);
    }
    else
    {
        eyePos = pThing->pos;
    }

    sithAIMove_AISetLookPos(pLocal, &eyePos);
}

void J3DAPI sithAIMove_AISetLookPos(SithAIControlBlock* pLocal, const rdVector3* targetPos)
{
    SITH_ASSERTREL(pLocal && pLocal->pOwner && pLocal->pClass);
    SITH_ASSERTREL(targetPos);

    if ( (pLocal->submode & SITHAI_SUBMODE_UNKNOWN_1) != 0 )
    {
        return;
    }

    if ( (pLocal->mode & SITHAI_MODE_DISABLED) != 0 && (sithMain_g_sith_mode.debugModeFlags & SITHDEBUG_INEDITOR) == 0 )
    {
        SITHLOG_ERROR("WARNING!  Look target issued to '%s'. AI is DISABLED.\n", pLocal->pOwner->aName);
    }

    SithThing* pOwner = pLocal->pOwner;
    rdVector3 eyePos  = pOwner->pos;
    if ( pOwner->type == SITH_THING_ACTOR || pOwner->type == SITH_THING_PLAYER )
    {
        eyePos.z += pOwner->thingInfo.actorInfo.eyeOffset.z;
    }

    rdVector_Sub3(&pLocal->goalLVec, targetPos, &eyePos);
    float lookDist = rdVector_Normalize3Acc(&pLocal->goalLVec);
    if ( _isnan(lookDist) || lookDist < 0.001f )
    {
        pLocal->goalLVec = pOwner->orient.lvec;
        lookDist         = 0.0f;
    }

    pLocal->lookPos = *targetPos;
    if ( lookDist != 0.0f )
    {
        pLocal->mode    |= SITHAI_MODE_TURNING;
        pLocal->submode |= SITHAI_SUBMODE_UNKNOWN_10;
    }

    pLocal->mode |= SITHAI_MODE_UNKNOWN_80;
    if ( (pLocal->mode & SITHAI_MODE_WANTALLEVENTS) != 0 )
    {
        sithAI_EmitEvent(pLocal, SITHAI_EVENT_UNKNOWN_40000, NULL);
    }
}

void J3DAPI sithAIMove_AISetLookPosEyeLevel(SithAIControlBlock* pLocal, const rdVector3* targetPos)
{
    SithThing* pOwner = pLocal->pOwner;
    rdVector3 lookPos = *targetPos;

    rdVector3 eyePos = { .x = 0.0f, .y = 0.0f, .z = pOwner->thingInfo.actorInfo.eyeOffset.z };
    rdMatrix_TransformVector34Acc(&eyePos, &pOwner->orient);
    rdVector_Add3Acc(&eyePos, &pOwner->pos);

    // Project look pos onto the plane at eye level
    float dist = rdMath_DistancePointToPlane(&lookPos, &pOwner->orient.uvec, &eyePos);
    rdVector_MultAcc3(&lookPos, &pOwner->orient.uvec, -dist);

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
        rdVector3 homeDir;
        rdVector_Sub3(&homeDir, moveToPos, &pLocal->homePos);

        float homeDist = rdVector_Normalize3Acc(&homeDir);
        if ( homeDist > pLocal->maxHomeDist )
        {
            rdVector_ScaleAdd3(&newPos, &homeDir, pLocal->maxHomeDist, &pLocal->homePos);
        }
    }

    float goalDist = sithAIMove_GetGoalTolerance(pLocal, &curPos, &newPos);
    if ( rdVector_Dist3(moveToPos, &pLocal->pOwner->pos) <= goalDist )
    {
        return 0;
    }

    pLocal->moveSpeed  = moveSpeed;
    pLocal->movePos    = *moveToPos;
    pLocal->vecUnknown = pLocal->pOwner->pos;

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

    pLocal->vecUnknown3 = pLocal->movePos;
    if ( !sithAIMove_AISetMoveTargetPos(pLocal, moveToPos, moveSpeed) )
    {
        return 0;
    }

    pLocal->submode |= SITHAI_SUBMODE_UNKNOWN_2;
    return 1;
}

int J3DAPI sithAIMove_AISpecialMove(SithAIControlBlock* pLocal, SithActorSpecialMoveFlags moveFlags)
{
    SITH_ASSERTREL(pLocal && pLocal->pOwner);

    SithThing* pThing = pLocal->pOwner;

    SithPuppetSubMode leftSubmode;
    SithPuppetSubMode rightSubmode = 0;
    bool bHasRightMode             = false;
    if ( (moveFlags & SITHACTORSPECIALMOVE_ROLL) != 0 )
    {
        if ( rdVector_Dot3(&pThing->orient.lvec, &pLocal->toTarget) <= 0.40000001f )
        {
            return 0;
        }

        leftSubmode  = SITHPUPPETSUBMODE_HOPLEFT;
        rightSubmode = SITHPUPPETSUBMODE_HOPRIGHT;
    }
    else if ( (moveFlags & SITHACTORSPECIALMOVE_STRAFE) != 0 )
    {
        if ( rdVector_Dot3(&pThing->orient.lvec, &pLocal->toTarget) <= 0.40000001f )
        {
            return 0;
        }

        leftSubmode  = SITHPUPPETSUBMODE_STRAFELEFT;
        rightSubmode = SITHPUPPETSUBMODE_STRAFERIGHT;
    }
    else if ( (moveFlags & SITHACTORSPECIALMOVE_TURN45) != 0 )
    {
        leftSubmode  = SITHPUPPETSUBMODE_WHIPCLIMBUP;
        rightSubmode = SITHPUPPETSUBMODE_WHIPCLIMBDOWN;
    }
    else if ( (moveFlags & SITHACTORSPECIALMOVE_TURN90) != 0 )
    {
        leftSubmode  = SITHPUPPETSUBMODE_WHIPCLIMBLEFT;
        rightSubmode = SITHPUPPETSUBMODE_WHIPCLIMBRIGHT;
    }
    else if ( (moveFlags & SITHACTORSPECIALMOVE_TURN135) != 0 )
    {
        leftSubmode  = SITHPUPPETSUBMODE_WHIPCLIMBDISMOUNT;
        rightSubmode = SITHPUPPETSUBMODE_WHIPSWINGMOUNT;
    }
    else if ( (moveFlags & SITHACTORSPECIALMOVE_TURN180) != 0 )
    {
        leftSubmode   = SITHPUPPETSUBMODE_WHIPCLIMBMOUNT;
        bHasRightMode = true;
    }
    else
    {
        if ( (moveFlags & SITHACTORSPECIALMOVE_MOUNTWALL) == 0 )
        {
            return 0;
        }

        leftSubmode   = SITHPUPPETSUBMODE_MOUNTWALL;
        bHasRightMode = true;
    }

    if ( !bHasRightMode )
    {
        bHasRightMode = pThing->pPuppetClass->aModes[pThing->pPuppetState->majorMode][rightSubmode].pKeyframe != NULL;
    }

    if ( !pThing->pPuppetClass->aModes[pThing->pPuppetState->majorMode][leftSubmode].pKeyframe || !bHasRightMode )
    {
        return 0;
    }

    bool bRight = false;
    rdVector3 targetPos;
    if ( (moveFlags & (SITHACTORSPECIALMOVE_STRAFE | SITHACTORSPECIALMOVE_ROLL)) != 0 )
    {
        float distance = (moveFlags & SITHACTORSPECIALMOVE_ROLL) != 0 ? 0.21000001f : 0.079999998f;

        bRight = true;
        if ( (moveFlags & SITHACTORSPECIALMOVE_DIR_RANDOM) != 0 )
        {
            bRight = SITH_RAND() >= 0.5f;
        }
        else if ( (moveFlags & SITHACTORSPECIALMOVE_DIR_LEFT) != 0 )
        {
            bRight = false;
        }

        rdVector3 moveDir;
        if ( bRight )
        {
            moveDir = pThing->orient.rvec;
        }
        else
        {
            moveDir = RDVECTOR_NEG3(pThing->orient.rvec);
        }

        rdVector3 halfPos;
        rdVector_ScaleAdd3(&halfPos, &moveDir, distance * 0.5f, &pThing->pos);
        rdVector_ScaleAdd3(&targetPos, &moveDir, distance, &pThing->pos);

        // TODO: [BUG] The retry flips the direction but checks the same targetPos again
        int numTries = 0;
        rdVector3 hitNormal;
        while ( sithAIUtil_CheckPathToPoint(pThing, &targetPos, pThing->collide.movesize, &distance, &hitNormal, 1, 0)
            && (moveFlags & (SITHACTORSPECIALMOVE_STRAFE | SITHACTORSPECIALMOVE_ROLL)) != 0 )
        {
            if ( ++numTries >= 2 )
            {
                return 0;
            }

            bRight = !bRight;
            rdVector_Neg3Acc(&moveDir);
        }

        if ( (moveFlags & SITHACTORSPECIALMOVE_ROLL) != 0 && sithAIUtil_CheckPosition(pLocal, &halfPos, NULL) != SITHAIUTIL_FLOOR_OK )
        {
            return 0;
        }

        if ( (moveFlags & (SITHACTORSPECIALMOVE_STRAFE | SITHACTORSPECIALMOVE_ROLL)) != 0
            && sithAIUtil_CheckPosition(pLocal, &targetPos, NULL) != SITHAIUTIL_FLOOR_OK )
        {
            return 0;
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

        sithAIMove_AISetMoveTargetPos(pLocal, &targetPos, 1.0f);
        return 1;
    }

    if ( (moveFlags & SITHACTORSPECIALMOVE_ROLL) != 0 )
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

        sithAIMove_AISetMoveTargetPos(pLocal, &targetPos, 2.0f);
        return 1;
    }

    if ( (moveFlags & SITHACTORSPECIALMOVE_TURN45) != 0 )
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

    if ( (moveFlags & SITHACTORSPECIALMOVE_TURN90) != 0 )
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

    if ( (moveFlags & SITHACTORSPECIALMOVE_TURN135) != 0 )
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

    // Wall mount
    float angle    = rdMath_DeltaAngleNormalized(&pLocal->vecUnknown3, &pThing->orient.uvec, &pThing->orient.lvec);
    float absAngle = fabsf(angle);
    if ( absAngle >= 80.0f && absAngle <= 100.0f )
    {
        pLocal->pOwner->moveStatus = SITHPLAYERMOVE_MOUNTING_WALL;
        sithPuppet_SetSubMode(pLocal->pOwner, SITHPUPPETSUBMODE_MOUNTWALL, sithAIMove_PuppetCallback);
        return 1;
    }

    if ( absAngle >= 60.0f && absAngle <= 79.0f )
    {
        rdVector3 newUVec;
        rdVector_ScaleAdd3(&newUVec, &pLocal->vecUnknown3, -0.2f, &pThing->orient.uvec);
        rdVector_Normalize3Acc(&newUVec);

        // TODO: [BUG] The projected lvec is computed but never used
        rdVector3 newLVec;
        rdMath_ProjectPointOntoPlaneNormalized(&newLVec, &pThing->orient.lvec, &newUVec, &rdroid_g_zeroVector3);

        pThing->orient.uvec = newUVec;
        rdVector_Cross3(&pThing->orient.rvec, &pThing->orient.lvec, &pThing->orient.uvec);
        rdVector_Normalize3Acc(&pThing->orient.rvec);
        rdVector_Cross3(&pThing->orient.lvec, &pThing->orient.uvec, &pThing->orient.rvec);

        pLocal->pOwner->moveStatus = SITHPLAYERMOVE_MOUNTING_WALL;
        sithPuppet_SetSubMode(pLocal->pOwner, SITHPUPPETSUBMODE_MOUNTWALL, sithAIMove_PuppetCallback);
        return 1;
    }

    if ( absAngle <= 40.0f )
    {
        return 0;
    }

    sithAIMove_AIFinalizeSpecialMove(pLocal, SITHACTORSPECIALMOVE_MOUNTWALL);
    return 1;
}

void J3DAPI sithAIMove_AIFinalizeSpecialMove(SithAIControlBlock* pLocal, SithActorSpecialMoveFlags moveFlags)
{
    SithThing* pThing = pLocal->pOwner;
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
        {
            rdVector3 pyr;
            if ( pThing->moveStatus == SITHPLAYERMOVE_TURNING_LEFT_45_DEGREES )
            {
                rdVector_Set3(&pyr, 0.0f, 45.0f, 0.0f);
            }

            if ( pThing->moveStatus == SITHPLAYERMOVE_TURNING_RIGHT_45_DEGREES )
            {
                rdVector_Set3(&pyr, 0.0f, -45.0f, 0.0f);
            }

            if ( pThing->moveStatus == SITHPLAYERMOVE_TURNING_LEFT_90_DEGREES )
            {
                rdVector_Set3(&pyr, 0.0f, 90.0f, 0.0f);
            }

            if ( pThing->moveStatus == SITHPLAYERMOVE_TURNING_RIGHT_90_DEGREES )
            {
                rdVector_Set3(&pyr, 0.0f, -90.0f, 0.0f);
            }

            if ( pThing->moveStatus == SITHPLAYERMOVE_TURNING_LEFT_135_DEGREES )
            {
                rdVector_Set3(&pyr, 0.0f, 135.0f, 0.0f);
            }

            if ( pThing->moveStatus == SITHPLAYERMOVE_TURNING_RIGHT_135_DEGREES )
            {
                rdVector_Set3(&pyr, 0.0f, -135.0f, 0.0f);
            }

            if ( pThing->moveStatus == SITHPLAYERMOVE_TURNING_180_DEGREES )
            {
                rdVector_Set3(&pyr, 0.0f, 180.0f, 0.0f);
            }

            rdMatrix_PreRotate34(&pThing->orient, &pyr);
            rdMatrix_Normalize34(&pThing->orient);
            break;
        }

        case SITHACTORSPECIALMOVE_MOUNTWALL:
            sithThing_DetachThing(pThing);

            pThing->orient.uvec = pLocal->vecUnknown3;
            rdVector_Cross3(&pThing->orient.lvec, &pThing->orient.uvec, &pThing->orient.rvec);
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
    pLocal->movePos   = *movePos;
    pLocal->moveSpeed = 2.0f;
    if ( sithPuppet_PlayMode(pLocal->pOwner, SITHPUPPETSUBMODE_JUMPUP, NULL) < 0 )
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

    pLocal->movePos = pLocal->pOwner->pos;

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
        if ( sithAIMove_AIGetMoveState(pLocal) == SITHAIMOVE_STATE_NONE )
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
        rdVector_Zero3(&pLocal->pOwner->moveInfo.physics.velocity);
    }
}

void J3DAPI sithAIMove_ResetAILook(SithAIControlBlock* pLocal)
{
    sithActor_SetHeadPYR(pLocal->pOwner, &rdroid_g_zeroVector3);
    pLocal->goalLVec = pLocal->pOwner->orient.lvec;
    sithAIUtil_AIClearMode(pLocal, SITHAI_MODE_TURNING);
}

void J3DAPI sithAIMove_UpdateBoss(SithAIControlBlock* pLocal, float secDeltaTime)
{
    SithThing* pOwner         = pLocal->pOwner;
    SithActorInfo* pActorInfo = &pOwner->thingInfo.actorInfo;
    rdVector3 goal            = pLocal->goalLVec;

    float dot = fabsf(rdVector_Dot3(&pOwner->orient.uvec, &goal));
    if ( dot > sithAIMove_bossMaxUpDot )
    {
        pLocal->mode &= ~SITHAI_MODE_TURNING;
        SITHLOG_ERROR("WARNING! AI '%s' looking straight up or down.\n", pOwner->aName);
        return;
    }

    rdVector3 dir;
    rdMath_ProjectPointOntoPlaneNormalized(&dir, &goal, &pOwner->orient.uvec, &rdroid_g_zeroVector3);

    float angle    = rdMath_DeltaAngleNormalized(&dir, &pOwner->orient.lvec, &pOwner->orient.uvec);
    float rotStep  = (pActorInfo->maxRotVelocity * 1.0f) * secDeltaTime;
    float rotLimit = rotStep * sithAIMove_bossTurnLimitScale;
    if ( fabsf(angle) <= rotLimit )
    {
        pOwner->orient.lvec = dir;
        rdVector_Cross3(&pOwner->orient.rvec, &pOwner->orient.lvec, &pOwner->orient.uvec);
        rdVector_Normalize3Acc(&pOwner->orient.rvec);
        rdVector_Cross3(&pOwner->orient.uvec, &pOwner->orient.rvec, &pOwner->orient.lvec);

        pLocal->mode &= ~SITHAI_MODE_TURNING;
    }
    else
    {
        rdVector3 pyr = { .pitch = 0.0f, .yaw = -((angle < 0.0f ? -1.0f : 1.0f) * rotStep), .roll = 0.0f };
        rdMatrix_PreRotate34(&pOwner->orient, &pyr);
        rdMatrix_Normalize34(&pOwner->orient);
    }
}

void J3DAPI sithAIMove_UpdateHeadTrackTarget(SithAIControlBlock* pLocal, float secDeltaTime)
{
    SITH_ASSERTREL(pLocal && pLocal->pOwner);

    SithThing* pOwner         = pLocal->pOwner;
    SithActorInfo* pActorInfo = &pOwner->thingInfo.actorInfo;
    SithThing* pTarget        = pLocal->pTargetThing;
    rdVector3 headPYR         = pActorInfo->headPYR;

    if ( (pActorInfo->flags & SITH_AF_IMMOBILE) != 0 )
    {
        return;
    }

    if ( (pActorInfo->flags & SITH_AF_CANROTATEHEAD) == 0 )
    {
        return;
    }

    rdVector3 eyePos = { .x = 0.0f, .y = 0.0f, .z = pOwner->thingInfo.actorInfo.eyeOffset.z };
    rdMatrix_TransformVector34Acc(&eyePos, &pOwner->orient);
    rdVector_Add3Acc(&eyePos, &pOwner->pos);

    if ( pTarget && (pActorInfo->flags & SITH_AF_SEEINVISIBLE) == 0 )
    {
        if ( (pTarget->type == SITH_THING_ACTOR || pTarget->type == SITH_THING_PLAYER)
            && (pTarget->thingInfo.actorInfo.flags & SITH_AF_INVISIBLE) != 0 )
        {
            pTarget = NULL;
        }

        if ( (pActorInfo->flags & SITH_AF_BLIND) != 0 )
        {
            pTarget = NULL;
        }
    }

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
            targetPos = pTarget->pos;
        }

        rdVector_Sub3(&targetDir, &targetPos, &eyePos);
        rdVector_Normalize3Acc(&targetDir);

        rdMath_ProjectPointOntoPlaneNormalized(&projTargetDir, &targetDir, &pOwner->orient.uvec, &rdroid_g_zeroVector3);
    }

    targetPos = pLocal->lookPos;

    rdVector3 lookDir;
    rdVector_Sub3(&lookDir, &targetPos, &eyePos);
    rdVector_Normalize3Acc(&lookDir);

    rdVector3 projLookDir;
    rdMath_ProjectPointOntoPlaneNormalized(&projLookDir, &lookDir, &pOwner->orient.uvec, &rdroid_g_zeroVector3);

    float lowThreshold  = sithAIMove_headTrackTargetStopDot;
    float highThreshold = sithAIMove_headTrackTargetStartDot;
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
        rdVector_Copy3(&aimDir, &projTargetDir); // Set when pTarget is set, which SITHAI_SUBMODE_UNKNOWN_800 requires
        rdVector_Copy3(&pitchDir, &targetDir);
    }
    else
    {
        aimDir   = projLookDir;
        pitchDir = lookDir;
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

    SithThing* pOwner         = pLocal->pOwner;
    SithActorInfo* pActorInfo = &pOwner->thingInfo.actorInfo;
    rdVector3 headPYR         = pActorInfo->headPYR;
    rdVector3 lookPos         = pLocal->lookPos;

    if ( (pActorInfo->flags & SITH_AF_CANROTATEHEAD) == 0 )
    {
        return;
    }

    if ( (pOwner->thingInfo.actorInfo.flags & SITH_AF_IMMOBILE) != 0 )
    {
        return;
    }

    rdVector3 eyePos = { .x = 0.0f, .y = 0.0f, .z = pOwner->thingInfo.actorInfo.eyeOffset.z };
    rdMatrix_TransformVector34Acc(&eyePos, &pOwner->orient);
    rdVector_Add3Acc(&eyePos, &pOwner->pos);

    rdVector3 lookDir;
    rdVector_Sub3(&lookDir, &lookPos, &eyePos);
    rdVector_Normalize3Acc(&lookDir);

    rdVector3 projDir;
    rdMath_ProjectPointOntoPlaneNormalized(&projDir, &lookDir, &pOwner->orient.uvec, &rdroid_g_zeroVector3);

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

    if ( fabsf(deltaYaw) <= sithAIMove_headTrackDoneAngle && fabsf(deltaPitch) <= sithAIMove_headTrackDoneAngle )
    {
        pLocal->mode &= ~SITHAI_MODE_UNKNOWN_80;
    }
}

void J3DAPI sithAIMove_GetQuetzStrikeAngles(SithAIControlBlock* pLocal, float* pYaw, float* pPitch, float* pHeadYaw, float* pHeadPitch)
{
    SithThing* pOwner  = pLocal->pOwner;
    SithThing* pTarget = pLocal->pTargetThing;

    rdVector3 ownerEye = pOwner->pos;
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

    rdVector3 lookDir;
    rdMath_ProjectPointOntoPlaneNormalized(&lookDir, &dir, &pOwner->orient.uvec, &rdroid_g_zeroVector3);

    float yaw = rdMath_DeltaAngleNormalized(&pOwner->orient.lvec, &lookDir, &pOwner->orient.uvec);
    *pYaw     = yaw;
    *pHeadYaw = -pOwner->renderData.apTweakedAngles[2].y;

    // Horizontal distance
    float distance = rdVector_Dist2((const rdVector2*)&pOwner->pos, (const rdVector2*)&pTarget->pos);
    distance = STDMATH_CLAMP(distance, sithAIMove_quetzStrikeMinDist, sithAIMove_quetzStrikeMaxDist);

    float pitchScale = (sithAIMove_quetzStrikeFarPitch - sithAIMove_quetzStrikeNearPitch) / (sithAIMove_quetzStrikeMaxDist - sithAIMove_quetzStrikeMinDist);
    *pPitch = (distance - sithAIMove_quetzStrikeMinDist) * pitchScale + sithAIMove_quetzStrikeNearPitch;

    float headPitchScale = (sithAIMove_quetzStrikeFarHeadPitch - sithAIMove_quetzStrikeNearHeadPitch) / (sithAIMove_quetzStrikeMaxDist - sithAIMove_quetzStrikeMinDist);
    *pHeadPitch = (distance - sithAIMove_quetzStrikeMinDist) * headPitchScale + sithAIMove_quetzStrikeNearHeadPitch - pOwner->renderData.apTweakedAngles[2].x;
}

void J3DAPI sithAIMove_UpdateQuetzStrike(SithAIControlBlock* pLocal, float secDeltaTime)
{
    SithThing* pThing = pLocal->pOwner;
    if ( !sithThing_CreateQuetzUserBlock(pThing) )
    {
        return;
    }

    SithQuetzStrike* pStrike = &pThing->userblock.pQuetz->strike;
    if ( pStrike->mode != SITHQUETZSTRIKE_MODE_STRIKE )
    {
        return;
    }

    SithQuetzSnapShot* pSnapShot = &pStrike->aSnapShots[pStrike->snapShot];
    float remainder = pSnapShot->time - pStrike->curTime;

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
    else if ( pStrike->snapShot == pStrike->numSnapShots - 1 )
    {
        pSnapShot->angle3 = pThing->renderData.apTweakedAngles[2].pitch;
        pSnapShot->angle4 = pThing->renderData.apTweakedAngles[2].yaw;
    }

    if ( remainder >= secDeltaTime )
    {
        SITH_ASSERTREL(pStrike->snapShot != 0);
        SithQuetzSnapShot* pPrevSnapShot = &pStrike->aSnapShots[pStrike->snapShot - 1];
        pStrike->curTime += secDeltaTime;

        float length      = pSnapShot->time - pPrevSnapShot->time;
        float timeElapsed = pStrike->curTime - pPrevSnapShot->time;
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
        pStrike->curTime = pSnapShot->time;
        pThing->renderData.apTweakedAngles[1].pitch  = pSnapShot->angle1;
        pThing->renderData.apTweakedAngles[1].yaw    = pSnapShot->angle2;
        pThing->renderData.apTweakedAngles[2].pitch  = pSnapShot->angle3;
        pThing->renderData.apTweakedAngles[2].yaw    = pSnapShot->angle4;
        pThing->renderData.apTweakedAngles[28].pitch = pSnapShot->angle5;
        if ( pStrike->snapShot == pStrike->numSnapShots )
        {
            pStrike->mode = SITHQUETZSTRIKE_MODE_NONE;
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

    rdVector3 pos = pOwner->pos;
    pos.z += pOwner->thingInfo.actorInfo.eyeOffset.z;

    rdVector3 targetPos;
    rdMatrix_TransformVector34(&targetPos, &pTarget->thingInfo.actorInfo.eyeOffset, &pTarget->orient);
    rdVector_Add3Acc(&targetPos, &pTarget->pos);

    rdVector3 dir;
    rdVector_Sub3(&dir, &targetPos, &pos);
    rdVector_Normalize3Acc(&dir);

    rdVector3 planeDir;
    rdMath_ProjectPointOntoPlaneNormalized(&planeDir, &dir, &pOwner->orient.uvec, &rdroid_g_zeroVector3);

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
    if ( pStrike->mode != SITHQUETZSTRIKE_MODE_ANIM2 )
    {
        return;
    }

    SithQuetzSnapShot* pSnapShot = &pStrike->aSnapShots[pStrike->snapShot];
    float remainder = pSnapShot->time - pStrike->curTime;

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
    else if ( pStrike->snapShot == pStrike->numSnapShots - 1 )
    {
        pSnapShot->angle3 = pThing->renderData.apTweakedAngles[2].pitch;
        pSnapShot->angle4 = pThing->renderData.apTweakedAngles[2].yaw;
    }

    if ( remainder >= secDeltaTime )
    {
        SITH_ASSERTREL(pStrike->snapShot != 0);
        SithQuetzSnapShot* pPrevSnapShot = &pStrike->aSnapShots[pStrike->snapShot - 1];
        pStrike->curTime += secDeltaTime;

        float length      = pSnapShot->time - pPrevSnapShot->time;
        float timeElapsed = pStrike->curTime - pPrevSnapShot->time;
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
        pStrike->curTime = pSnapShot->time;
        pThing->renderData.apTweakedAngles[1].pitch  = pSnapShot->angle1;
        pThing->renderData.apTweakedAngles[1].yaw    = pSnapShot->angle2;
        pThing->renderData.apTweakedAngles[2].pitch  = pSnapShot->angle3;
        pThing->renderData.apTweakedAngles[2].yaw    = pSnapShot->angle4;
        pThing->renderData.apTweakedAngles[28].pitch = pSnapShot->angle5;
        if ( pStrike->snapShot == pStrike->numSnapShots )
        {
            pStrike->mode = SITHQUETZSTRIKE_MODE_NONE;
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
    if ( pStrike->mode != SITHQUETZSTRIKE_MODE_ANIM3 )
    {
        return;
    }

    SithQuetzSnapShot* pSnapShot = &pStrike->aSnapShots[pStrike->snapShot];
    float timeLeft = pSnapShot->time - pStrike->curTime;

    if ( pStrike->snapShot == pStrike->numSnapShots - 1 )
    {
        pSnapShot->angle3 = pThing->renderData.apTweakedAngles[2].pitch;
        pSnapShot->angle4 = pThing->renderData.apTweakedAngles[2].yaw;
    }

    if ( timeLeft >= secDeltaTime )
    {
        SITH_ASSERTREL(pStrike->snapShot != 0);
        SithQuetzSnapShot* pPrevSnapShot = &pStrike->aSnapShots[pStrike->snapShot - 1];
        pStrike->curTime += secDeltaTime;

        float period      = pSnapShot->time - pPrevSnapShot->time;
        float timeElapsed = pStrike->curTime - pPrevSnapShot->time;
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
        pStrike->curTime = pSnapShot->time;
        pThing->renderData.apTweakedAngles[1].pitch  = pSnapShot->angle1;
        pThing->renderData.apTweakedAngles[1].yaw    = pSnapShot->angle2;
        pThing->renderData.apTweakedAngles[2].pitch  = pSnapShot->angle3;
        pThing->renderData.apTweakedAngles[2].yaw    = pSnapShot->angle4;
        pThing->renderData.apTweakedAngles[28].pitch = pSnapShot->angle5;
        if ( pStrike->snapShot == pStrike->numSnapShots )
        {
            pStrike->mode = SITHQUETZSTRIKE_MODE_NONE;
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
    if ( pStrike->mode != SITHQUETZSTRIKE_MODE_ANIM5 )
    {
        return;
    }

    SithQuetzSnapShot* pSnapShot = &pStrike->aSnapShots[pStrike->snapShot];
    float timeLeft = pSnapShot->time - pStrike->curTime;

    if ( pStrike->snapShot == pStrike->numSnapShots - 1 )
    {
        pSnapShot->angle3 = pThing->renderData.apTweakedAngles[2].pitch;
        pSnapShot->angle4 = pThing->renderData.apTweakedAngles[2].yaw;
    }

    if ( timeLeft >= secDeltaTime )
    {
        SITH_ASSERTREL(pStrike->snapShot != 0);
        SithQuetzSnapShot* pPrevSnapShot = &pStrike->aSnapShots[pStrike->snapShot - 1];
        pStrike->curTime += secDeltaTime;

        float period      = pSnapShot->time - pPrevSnapShot->time;
        float timeElapsed = pStrike->curTime - pPrevSnapShot->time;
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
        pStrike->curTime = pSnapShot->time;
        pThing->renderData.apTweakedAngles[1].pitch  = pSnapShot->angle1;
        pThing->renderData.apTweakedAngles[1].yaw    = pSnapShot->angle2;
        pThing->renderData.apTweakedAngles[2].pitch  = pSnapShot->angle3;
        pThing->renderData.apTweakedAngles[2].yaw    = pSnapShot->angle4;
        pThing->renderData.apTweakedAngles[28].pitch = pSnapShot->angle5;
        if ( pStrike->snapShot == pStrike->numSnapShots )
        {
            pStrike->mode = SITHQUETZSTRIKE_MODE_NONE;
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
    if ( pStrike->mode != SITHQUETZSTRIKE_MODE_ANIM4 )
    {
        return;
    }

    SithQuetzSnapShot* pSnapShot = &pStrike->aSnapShots[pStrike->snapShot];
    float timeLeft = pSnapShot->time - pStrike->curTime;

    if ( timeLeft >= secDeltaTime )
    {
        SITH_ASSERTREL(pStrike->snapShot != 0);
        SithQuetzSnapShot* pPrevSnapShot = &pStrike->aSnapShots[pStrike->snapShot - 1];
        pStrike->curTime += secDeltaTime;

        float period      = pSnapShot->time - pPrevSnapShot->time;
        float timeElapsed = pStrike->curTime - pPrevSnapShot->time;
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
        pStrike->curTime = pSnapShot->time;
        pThing->renderData.apTweakedAngles[1].pitch  = pSnapShot->angle1;
        pThing->renderData.apTweakedAngles[1].yaw    = pSnapShot->angle2;
        pThing->renderData.apTweakedAngles[2].pitch  = pSnapShot->angle3;
        pThing->renderData.apTweakedAngles[2].yaw    = pSnapShot->angle4;
        pThing->renderData.apTweakedAngles[28].pitch = pSnapShot->angle5;
        if ( pStrike->snapShot == pStrike->numSnapShots )
        {
            pStrike->mode = SITHQUETZSTRIKE_MODE_NONE;
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
    switch ( pStrike->mode )
    {
        case SITHQUETZSTRIKE_MODE_STRIKE:
            sithAIMove_UpdateQuetzStrike(pLocal, secDeltaTime);
            break;

        case SITHQUETZSTRIKE_MODE_ANIM2:
            sithAIMove_UpdateQuetzAnim2(pLocal, secDeltaTime);
            break;

        case SITHQUETZSTRIKE_MODE_ANIM3:
            sithAIMove_UpdateQuetzAnim3(pLocal, secDeltaTime);
            break;

        case SITHQUETZSTRIKE_MODE_ANIM5:
            sithAIMove_UpdateQuetzAnim5(pLocal, secDeltaTime);
            break;

        case SITHQUETZSTRIKE_MODE_ANIM4:
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

    pStrike->curTime  = 0.0f;
    pStrike->snapShot = 0;

    pStrike->aSnapShots[0].time   = 0.0f;
    pStrike->aSnapShots[0].angle1 = pThing->renderData.apTweakedAngles[1].pitch;
    pStrike->aSnapShots[0].angle2 = pThing->renderData.apTweakedAngles[1].yaw;
    pStrike->aSnapShots[0].angle3 = pThing->renderData.apTweakedAngles[2].pitch;
    pStrike->aSnapShots[0].angle4 = pThing->renderData.apTweakedAngles[2].yaw;
    pStrike->aSnapShots[0].angle5 = pThing->renderData.apTweakedAngles[28].pitch;

    pStrike->aSnapShots[1].time = 0.11f;
    pStrike->aSnapShots[2].time = 0.24f;
    pStrike->aSnapShots[3].time = 0.3f;
    pStrike->aSnapShots[4].time = 0.5f;

    pStrike->aSnapShots[5].time   = 0.7f;
    pStrike->aSnapShots[5].angle1 = pThing->renderData.apTweakedAngles[1].pitch;
    pStrike->aSnapShots[5].angle2 = pThing->renderData.apTweakedAngles[1].yaw;
    pStrike->aSnapShots[5].angle3 = pThing->renderData.apTweakedAngles[2].pitch;
    pStrike->aSnapShots[5].angle4 = pThing->renderData.apTweakedAngles[2].yaw;
    pStrike->aSnapShots[5].angle5 = pThing->renderData.apTweakedAngles[28].pitch;

    pStrike->mode         = SITHQUETZSTRIKE_MODE_STRIKE;
    pStrike->numSnapShots = 6;
}

void J3DAPI sithAIMove_StartQuetzAnim2(SithAIControlBlock* pLocal)
{
    SithThing* pThing = pLocal->pOwner;
    if ( !sithThing_CreateQuetzUserBlock(pThing) )
    {
        return;
    }

    SithQuetzStrike* pStrike = &pThing->userblock.pQuetz->strike;
    pStrike->curTime  = 0.0f;
    pStrike->snapShot = 0;

    pStrike->aSnapShots[0].time   = 0.0f;
    pStrike->aSnapShots[0].angle1 = pThing->renderData.apTweakedAngles[1].pitch;
    pStrike->aSnapShots[0].angle2 = pThing->renderData.apTweakedAngles[1].yaw;
    pStrike->aSnapShots[0].angle3 = pThing->renderData.apTweakedAngles[2].pitch;
    pStrike->aSnapShots[0].angle4 = pThing->renderData.apTweakedAngles[2].yaw;
    pStrike->aSnapShots[0].angle5 = pThing->renderData.apTweakedAngles[28].pitch;

    pStrike->aSnapShots[1].time = 0.11f;

    pStrike->aSnapShots[2].time = 0.24f;

    pStrike->aSnapShots[3].time   = 0.5f;
    pStrike->aSnapShots[3].angle1 = pThing->renderData.apTweakedAngles[1].pitch;
    pStrike->aSnapShots[3].angle2 = pThing->renderData.apTweakedAngles[1].yaw;
    pStrike->aSnapShots[3].angle3 = pThing->renderData.apTweakedAngles[2].pitch;
    pStrike->aSnapShots[3].angle4 = pThing->renderData.apTweakedAngles[2].yaw;
    pStrike->aSnapShots[3].angle5 = pThing->renderData.apTweakedAngles[28].pitch;

    pStrike->mode         = SITHQUETZSTRIKE_MODE_ANIM2;
    pStrike->numSnapShots = 4;
}

void J3DAPI sithAIMove_StartQuetzAnim3(SithAIControlBlock* pLocal)
{
    SithThing* pThing = pLocal->pOwner;
    if ( !sithThing_CreateQuetzUserBlock(pThing) )
    {
        return;
    }

    SithQuetzStrike* pStrike = &pThing->userblock.pQuetz->strike;
    pStrike->curTime  = 0.0f;
    pStrike->snapShot = 0;

    pStrike->aSnapShots[0].time   = 0.0f;
    pStrike->aSnapShots[0].angle1 = pThing->renderData.apTweakedAngles[1].pitch;
    pStrike->aSnapShots[0].angle2 = pThing->renderData.apTweakedAngles[1].yaw;
    pStrike->aSnapShots[0].angle3 = pThing->renderData.apTweakedAngles[2].pitch;
    pStrike->aSnapShots[0].angle4 = pThing->renderData.apTweakedAngles[2].yaw;
    pStrike->aSnapShots[0].angle5 = pThing->renderData.apTweakedAngles[28].pitch;

    pStrike->aSnapShots[1].time   = 0.1f;
    pStrike->aSnapShots[1].angle1 = -10.0f;
    pStrike->aSnapShots[1].angle2 = 0.0f;
    pStrike->aSnapShots[1].angle3 = 50.0f;
    pStrike->aSnapShots[1].angle4 = 0.0f;
    pStrike->aSnapShots[1].angle5 = -40.0f;

    pStrike->aSnapShots[2].time   = 0.3f;
    pStrike->aSnapShots[2].angle1 = -15.0f;
    pStrike->aSnapShots[2].angle2 = 30.0f;
    pStrike->aSnapShots[2].angle3 = 60.0f;
    pStrike->aSnapShots[2].angle4 = 50.0f;
    pStrike->aSnapShots[2].angle5 = -60.0f;

    pStrike->aSnapShots[3].time   = 0.6f;
    pStrike->aSnapShots[3].angle1 = -12.0f;
    pStrike->aSnapShots[3].angle2 = -30.0f;
    pStrike->aSnapShots[3].angle3 = 55.0f;
    pStrike->aSnapShots[3].angle4 = -50.0f;
    pStrike->aSnapShots[3].angle5 = -50.0f;

    pStrike->aSnapShots[4].time   = 0.9f;
    pStrike->aSnapShots[4].angle1 = -15.0f;
    pStrike->aSnapShots[4].angle2 = 30.0f;
    pStrike->aSnapShots[4].angle3 = 60.0f;
    pStrike->aSnapShots[4].angle4 = 50.0f;
    pStrike->aSnapShots[4].angle5 = -35.0f;

    pStrike->aSnapShots[5].time   = 1.2f;
    pStrike->aSnapShots[5].angle1 = -12.0f;
    pStrike->aSnapShots[5].angle2 = -30.0f;
    pStrike->aSnapShots[5].angle3 = 55.0f;
    pStrike->aSnapShots[5].angle4 = -50.0f;
    pStrike->aSnapShots[5].angle5 = -50.0f;

    pStrike->aSnapShots[6].time   = 1.5f;
    pStrike->aSnapShots[6].angle1 = -15.0f;
    pStrike->aSnapShots[6].angle2 = 30.0f;
    pStrike->aSnapShots[6].angle3 = 60.0f;
    pStrike->aSnapShots[6].angle4 = 50.0f;
    pStrike->aSnapShots[6].angle5 = -60.0f;

    pStrike->aSnapShots[7].time   = 1.8f;
    pStrike->aSnapShots[7].angle1 = -12.0f;
    pStrike->aSnapShots[7].angle2 = -30.0f;
    pStrike->aSnapShots[7].angle3 = 55.0f;
    pStrike->aSnapShots[7].angle4 = -50.0f;
    pStrike->aSnapShots[7].angle5 = -50.0f;

    pStrike->aSnapShots[8].time   = 2.1f;
    pStrike->aSnapShots[8].angle1 = -15.0f;
    pStrike->aSnapShots[8].angle2 = 30.0f;
    pStrike->aSnapShots[8].angle3 = 60.0f;
    pStrike->aSnapShots[8].angle4 = 50.0f;
    pStrike->aSnapShots[8].angle5 = -35.0f;

    pStrike->aSnapShots[9].time   = 2.4f;
    pStrike->aSnapShots[9].angle1 = -12.0f;
    pStrike->aSnapShots[9].angle2 = -30.0f;
    pStrike->aSnapShots[9].angle3 = 55.0f;
    pStrike->aSnapShots[9].angle4 = -50.0f;
    pStrike->aSnapShots[9].angle5 = -50.0f;

    pStrike->aSnapShots[10].time   = 2.7f;
    pStrike->aSnapShots[10].angle1 = -15.0f;
    pStrike->aSnapShots[10].angle2 = 30.0f;
    pStrike->aSnapShots[10].angle3 = 60.0f;
    pStrike->aSnapShots[10].angle4 = 50.0f;
    pStrike->aSnapShots[10].angle5 = -60.0f;

    pStrike->aSnapShots[11].time   = 3.0f;
    pStrike->aSnapShots[11].angle1 = -12.0f;
    pStrike->aSnapShots[11].angle2 = -30.0f;
    pStrike->aSnapShots[11].angle3 = 55.0f;
    pStrike->aSnapShots[11].angle4 = -50.0f;
    pStrike->aSnapShots[11].angle5 = -50.0f;

    pStrike->aSnapShots[12].time   = 3.3f;
    pStrike->aSnapShots[12].angle1 = -10.0f;
    pStrike->aSnapShots[12].angle2 = 0.0f;
    pStrike->aSnapShots[12].angle3 = 20.0f;
    pStrike->aSnapShots[12].angle4 = 0.0f;
    pStrike->aSnapShots[12].angle5 = -15.0f;

    pStrike->aSnapShots[13].time   = 3.7f;
    pStrike->aSnapShots[13].angle1 = 0.0f;
    pStrike->aSnapShots[13].angle2 = 0.0f;
    pStrike->aSnapShots[13].angle3 = 5.0f;
    pStrike->aSnapShots[13].angle4 = 0.0f;
    pStrike->aSnapShots[13].angle5 = -10.0f;

    pStrike->aSnapShots[14].time   = 5.0f;
    pStrike->aSnapShots[14].angle1 = pThing->renderData.apTweakedAngles[1].pitch;
    pStrike->aSnapShots[14].angle2 = pThing->renderData.apTweakedAngles[1].yaw;
    pStrike->aSnapShots[14].angle3 = pThing->renderData.apTweakedAngles[2].pitch;
    pStrike->aSnapShots[14].angle4 = pThing->renderData.apTweakedAngles[2].yaw;
    pStrike->aSnapShots[14].angle5 = pThing->renderData.apTweakedAngles[28].pitch;

    pStrike->mode         = SITHQUETZSTRIKE_MODE_ANIM3;
    pStrike->numSnapShots = 15;
}

void J3DAPI sithAIMove_StartQuetzAnim5(SithAIControlBlock* pLocal)
{
    SithThing* pThing = pLocal->pOwner;
    if ( !sithThing_CreateQuetzUserBlock(pThing) )
    {
        return;
    }

    SithQuetzStrike* pStrike = &pThing->userblock.pQuetz->strike;
    pStrike->curTime  = 0.0f;
    pStrike->snapShot = 0;

    pStrike->aSnapShots[0].time   = 0.0f;
    pStrike->aSnapShots[0].angle1 = pThing->renderData.apTweakedAngles[1].pitch;
    pStrike->aSnapShots[0].angle2 = pThing->renderData.apTweakedAngles[1].yaw;
    pStrike->aSnapShots[0].angle3 = pThing->renderData.apTweakedAngles[2].pitch;
    pStrike->aSnapShots[0].angle4 = pThing->renderData.apTweakedAngles[2].yaw;
    pStrike->aSnapShots[0].angle5 = pThing->renderData.apTweakedAngles[28].pitch;

    pStrike->aSnapShots[1].time   = 0.1f;
    pStrike->aSnapShots[1].angle1 = -10.0f;
    pStrike->aSnapShots[1].angle2 = 0.0f;
    pStrike->aSnapShots[1].angle3 = 50.0f;
    pStrike->aSnapShots[1].angle4 = 0.0f;
    pStrike->aSnapShots[1].angle5 = -40.0f;

    pStrike->aSnapShots[2].time   = 0.3f;
    pStrike->aSnapShots[2].angle1 = -15.0f;
    pStrike->aSnapShots[2].angle2 = 30.0f;
    pStrike->aSnapShots[2].angle3 = 60.0f;
    pStrike->aSnapShots[2].angle4 = 50.0f;
    pStrike->aSnapShots[2].angle5 = -60.0f;

    pStrike->aSnapShots[3].time   = 0.6f;
    pStrike->aSnapShots[3].angle1 = -12.0f;
    pStrike->aSnapShots[3].angle2 = -30.0f;
    pStrike->aSnapShots[3].angle3 = 55.0f;
    pStrike->aSnapShots[3].angle4 = -50.0f;
    pStrike->aSnapShots[3].angle5 = -50.0f;

    pStrike->aSnapShots[4].time   = 0.9f;
    pStrike->aSnapShots[4].angle1 = -15.0f;
    pStrike->aSnapShots[4].angle2 = 30.0f;
    pStrike->aSnapShots[4].angle3 = 60.0f;
    pStrike->aSnapShots[4].angle4 = 50.0f;
    pStrike->aSnapShots[4].angle5 = -35.0f;

    pStrike->aSnapShots[5].time   = 1.2f;
    pStrike->aSnapShots[5].angle1 = -12.0f;
    pStrike->aSnapShots[5].angle2 = -30.0f;
    pStrike->aSnapShots[5].angle3 = 55.0f;
    pStrike->aSnapShots[5].angle4 = -50.0f;
    pStrike->aSnapShots[5].angle5 = -50.0f;

    pStrike->aSnapShots[6].time   = 1.5f;
    pStrike->aSnapShots[6].angle1 = -15.0f;
    pStrike->aSnapShots[6].angle2 = 30.0f;
    pStrike->aSnapShots[6].angle3 = 60.0f;
    pStrike->aSnapShots[6].angle4 = 50.0f;
    pStrike->aSnapShots[6].angle5 = -60.0f;

    pStrike->aSnapShots[7].time   = 1.8f;
    pStrike->aSnapShots[7].angle1 = -12.0f;
    pStrike->aSnapShots[7].angle2 = -30.0f;
    pStrike->aSnapShots[7].angle3 = 55.0f;
    pStrike->aSnapShots[7].angle4 = -50.0f;
    pStrike->aSnapShots[7].angle5 = -50.0f;

    pStrike->aSnapShots[8].time   = 2.1f;
    pStrike->aSnapShots[8].angle1 = -15.0f;
    pStrike->aSnapShots[8].angle2 = 30.0f;
    pStrike->aSnapShots[8].angle3 = 60.0f;
    pStrike->aSnapShots[8].angle4 = 50.0f;
    pStrike->aSnapShots[8].angle5 = -35.0f;

    pStrike->aSnapShots[9].time   = 3.0f;
    pStrike->aSnapShots[9].angle1 = pThing->renderData.apTweakedAngles[1].pitch;
    pStrike->aSnapShots[9].angle2 = pThing->renderData.apTweakedAngles[1].yaw;
    pStrike->aSnapShots[9].angle3 = pThing->renderData.apTweakedAngles[2].pitch;
    pStrike->aSnapShots[9].angle4 = pThing->renderData.apTweakedAngles[2].yaw;
    pStrike->aSnapShots[9].angle5 = pThing->renderData.apTweakedAngles[28].pitch;

    pStrike->mode         = SITHQUETZSTRIKE_MODE_ANIM5;
    pStrike->numSnapShots = 10;
}

void J3DAPI sithAIMove_StartQuetzAnim4(SithAIControlBlock* pLocal)
{
    SithThing* pThing = pLocal->pOwner;
    if ( !sithThing_CreateQuetzUserBlock(pThing) )
    {
        return;
    }

    SithQuetzStrike* pStrike = &pThing->userblock.pQuetz->strike;
    pStrike->curTime  = 0.0f;
    pStrike->snapShot = 0;

    pStrike->aSnapShots[0].time   = 0.0f;
    pStrike->aSnapShots[0].angle1 = pThing->renderData.apTweakedAngles[1].pitch;
    pStrike->aSnapShots[0].angle2 = pThing->renderData.apTweakedAngles[1].yaw;
    pStrike->aSnapShots[0].angle3 = pThing->renderData.apTweakedAngles[2].pitch;
    pStrike->aSnapShots[0].angle4 = pThing->renderData.apTweakedAngles[2].yaw;
    pStrike->aSnapShots[0].angle5 = pThing->renderData.apTweakedAngles[28].pitch;

    pStrike->aSnapShots[1].time   = 1.6f;
    pStrike->aSnapShots[1].angle1 = 0.0f;
    pStrike->aSnapShots[1].angle2 = 0.0f;
    pStrike->aSnapShots[1].angle3 = 0.0f;
    pStrike->aSnapShots[1].angle4 = 0.0f;
    pStrike->aSnapShots[1].angle5 = -60.0f;

    pStrike->aSnapShots[2].time   = 5.0f;
    pStrike->aSnapShots[2].angle1 = 0.0f;
    pStrike->aSnapShots[2].angle2 = 0.0f;
    pStrike->aSnapShots[2].angle3 = 0.0f;
    pStrike->aSnapShots[2].angle4 = 0.0f;
    pStrike->aSnapShots[2].angle5 = -60.0f;

    pStrike->mode         = SITHQUETZSTRIKE_MODE_ANIM4;
    pStrike->numSnapShots = 3;
}

void J3DAPI sithAIMove_UpdateMardukTail(SithThing* pThing, float secDeltaTime)
{
    if ( stdUtil_StrCmp(pThing->aName, "marduk") == 0 || stdUtil_StrCmp(pThing->aName, "marduk2") == 0 )
    {
        int jointIndex = sithThing_GetThingJointIndex(pThing, "wmabdomen");
        if ( jointIndex != -1 )
        {
            float angle = stdMath_NormalizeAngle(sithAIMove_mardukTailYawSpeed * secDeltaTime);
            pThing->renderData.apTweakedAngles[jointIndex].yaw += angle;
        }
    }
}

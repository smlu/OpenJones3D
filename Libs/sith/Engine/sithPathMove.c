#include "sithPathMove.h"
#include <j3dcore/j3dhook.h>

#include <rdroid/Math/rdMatrix.h>
#include <rdroid/Math/rdVector.h>

#include <sith/Cog/sithCog.h>
#include <sith/Engine/sithCollision.h>
#include <sith/World/sithSoundClass.h>
#include <sith/RTI/symbols.h>
#include <sith/World/sithThing.h>
#include <sith/World/sithWorld.h>

#include <std/General/stdConffile.h>
#include <std/General/stdMath.h>
#include <std/General/stdMemory.h>


void J3DAPI sithPathMove_FollowNextPath(SithThing* pThing);
void J3DAPI sithPathMove_SendMessageBlocked(SithThing* pThing);
void J3DAPI sithPathMove_UpdatePath(SithThing* pThing);

void J3DAPI sithPathMove_UpdateRotate(SithThing* pThing, float simTime); // Added: From debug version
void J3DAPI sithPathMove_UpdateMove(SithThing* pThing, float simTime); // Added: From debug version

void sithPathMove_InstallHooks(void)
{
    J3D_HOOKFUNC(sithPathMove_SkipToFrame);
    J3D_HOOKFUNC(sithPathMove_MoveToFrame);
    J3D_HOOKFUNC(sithPathMove_RotatePivot);
    J3D_HOOKFUNC(sithPathMove_FollowNextPath);
    J3D_HOOKFUNC(sithPathMove_MoveToPos);
    J3D_HOOKFUNC(sithPathMove_RotateToPYR);
    J3D_HOOKFUNC(sithPathMove_Update);
    J3D_HOOKFUNC(sithPathMove_ParseArg);
    J3D_HOOKFUNC(sithPathMove_SendMessageBlocked);
    J3D_HOOKFUNC(sithPathMove_UpdatePath);
    J3D_HOOKFUNC(sithPathMove_Finish);
    J3D_HOOKFUNC(sithPathMove_PathMovePause);
    J3D_HOOKFUNC(sithPathMove_PathMoveResume);
    J3D_HOOKFUNC(sithPathMove_Create);
}

void sithPathMove_ResetGlobals(void)
{}

//void J3DAPI sithPathMove_SkipToFrame(SithThing* pThing, int frame, float velocity)
//{
//    J3D_TRAMPOLINE_CALL(sithPathMove_SkipToFrame, pThing, frame, velocity);
//}
//
//int J3DAPI sithPathMove_MoveToFrame(SithThing* pThing, int frame, float velocity)
//{
//    return J3D_TRAMPOLINE_CALL(sithPathMove_MoveToFrame, pThing, frame, velocity);
//}
//
//void J3DAPI sithPathMove_RotatePivot(SithThing* pThing, const rdVector3* pOffset, const rdVector3* pRot, float rotTime)
//{
//    J3D_TRAMPOLINE_CALL(sithPathMove_RotatePivot, pThing, pOffset, pRot, rotTime);
//}
//
//void J3DAPI sithPathMove_FollowNextPath(SithThing* pThing)
//{
//    J3D_TRAMPOLINE_CALL(sithPathMove_FollowNextPath, pThing);
//}
//
//double J3DAPI sithPathMove_MoveToPos(SithThing* pThing, SithPathFrame* pos)
//{
//    return J3D_TRAMPOLINE_CALL(sithPathMove_MoveToPos, pThing, pos);
//}
//
//void J3DAPI sithPathMove_RotateToPYR(SithThing* pThing, rdVector3* pyr, float time)
//{
//    J3D_TRAMPOLINE_CALL(sithPathMove_RotateToPYR, pThing, pyr, time);
//}
//
//void J3DAPI sithPathMove_Update(SithThing* pThing, float simTime)
//{
//    J3D_TRAMPOLINE_CALL(sithPathMove_Update, pThing, simTime);
//}
//
//signed int J3DAPI sithPathMove_ParseArg(StdConffileArg* pArg, SithThing* pThing, int adjNum)
//{
//    return J3D_TRAMPOLINE_CALL(sithPathMove_ParseArg, pArg, pThing, adjNum);
//}
//
//void J3DAPI sithPathMove_SendMessageBlocked(SithThing* pThing)
//{
//    J3D_TRAMPOLINE_CALL(sithPathMove_SendMessageBlocked, pThing);
//}
//
//void J3DAPI sithPathMove_UpdatePath(SithThing* pThing)
//{
//    J3D_TRAMPOLINE_CALL(sithPathMove_UpdatePath, pThing);
//}
//
//void J3DAPI sithPathMove_Finish(SithThing* pThing)
//{
//    J3D_TRAMPOLINE_CALL(sithPathMove_Finish, pThing);
//}
//
//signed int J3DAPI sithPathMove_PathMovePause(SithThing* pThing)
//{
//    return J3D_TRAMPOLINE_CALL(sithPathMove_PathMovePause, pThing);
//}
//
//signed int J3DAPI sithPathMove_PathMoveResume(SithThing* pThing)
//{
//    return J3D_TRAMPOLINE_CALL(sithPathMove_PathMoveResume, pThing);
//}
//
//void J3DAPI sithPathMove_Create(SithThing* pNewThing, const SithThing* pMarker, const rdVector3* pOffset)
//{
//    J3D_TRAMPOLINE_CALL(sithPathMove_Create, pNewThing, pMarker, pOffset);
//}

void J3DAPI sithPathMove_SkipToFrame(SithThing* pThing, size_t frame, float velocity)
{
    SITH_ASSERTREL(sithThing_ValidateThingPointer(sithWorld_g_pCurrentWorld, pThing));
    SITH_ASSERTREL(pThing->moveType == SITH_MT_PATH);
    SITH_ASSERTREL(velocity > 0.0f);

    if ( frame >= pThing->moveInfo.pathMovement.numFrames )
    {
        SITHLOG_ERROR("SkipToFrame: illegal frame #%d.\n", frame);
        return;
    }

    pThing->moveInfo.pathMovement.mode     &= ~SITH_PATHMOVE_FOLLOWPATH;
    pThing->moveInfo.pathMovement.goalFrame = frame;
    pThing->moveInfo.pathMovement.moveVel   = velocity;

    sithSoundClass_PlayModeFirst(pThing, SITHSOUNDCLASS_STARTMOVE);
    sithSoundClass_PlayModeFirst(pThing, SITHSOUNDCLASS_MOVING);

    sithPathMove_MoveToPos(pThing, &pThing->moveInfo.pathMovement.aFrames[pThing->moveInfo.pathMovement.goalFrame]);
}

void J3DAPI sithPathMove_MoveToFrame(SithThing* pThing, size_t frame, float velocity)
{
    SITH_ASSERTREL(sithThing_ValidateThingPointer(sithWorld_g_pCurrentWorld, pThing));
    SITH_ASSERTREL(pThing->moveType == SITH_MT_PATH);
    SITH_ASSERTREL(velocity > 0.0f);

    if ( frame >= pThing->moveInfo.pathMovement.numFrames )
    {
        SITHLOG_ERROR("MoveToFrame: illegal frame #%d.\n", frame);
        return;
    }

    pThing->moveInfo.pathMovement.mode     |= SITH_PATHMOVE_FOLLOWPATH;
    pThing->moveInfo.pathMovement.moveVel   = velocity;
    pThing->moveInfo.pathMovement.goalFrame = frame;

    sithSoundClass_PlayModeFirst(pThing, SITHSOUNDCLASS_STARTMOVE);
    sithSoundClass_PlayModeFirst(pThing, SITHSOUNDCLASS_MOVING);

    sithPathMove_FollowNextPath(pThing);
}

void J3DAPI sithPathMove_RotatePivot(SithThing* pThing, const rdVector3* pOffset, const rdVector3* pRot, float rotTime)
{
    SITH_ASSERTREL(pThing);
    SITH_ASSERTREL(pThing->moveType == SITH_MT_PATH);
    SITH_ASSERTREL(rotTime > 0.0f);

    pThing->moveInfo.pathMovement.mode |= SITH_PATHMOVE_ROTATE_PIVOT | SITH_PATHMOVE_ROTATE;

    sithSoundClass_PlayModeFirst(pThing, SITHSOUNDCLASS_STARTMOVE);
    sithSoundClass_PlayModeFirst(pThing, SITHSOUNDCLASS_MOVING);

    rdVector_Copy3(&pThing->moveInfo.pathMovement.rotOffset, pOffset);
    rdMatrix_Copy34(&pThing->moveInfo.pathMovement.curOrient, &pThing->orient);
    rdVector_Sub3(&pThing->moveInfo.pathMovement.curOrient.dvec, &pThing->pos, &pThing->moveInfo.pathMovement.rotOffset);

    rdVector_Copy3(&pThing->moveInfo.pathMovement.goalPYR, pRot);

    pThing->moveInfo.pathMovement.rotDeltaTime    = 1.0f / rotTime;
    pThing->moveInfo.pathMovement.rotDelta        = 0.0f;
    pThing->moveInfo.pathMovement.numBlockedMoves = 0;
    pThing->moveInfo.pathMovement.nextFrame       = pThing->moveInfo.pathMovement.currentFrame;
    pThing->moveInfo.pathMovement.currentFrame    = -1;
}

void J3DAPI sithPathMove_FollowNextPath(SithThing* pThing)
{
    SITH_ASSERTREL(pThing && (pThing->moveType == SITH_MT_PATH));

    SithPathMoveInfo* pPath = &pThing->moveInfo.pathMovement;
    SITH_ASSERTREL(((pPath->mode) & (SITH_PATHMOVE_FOLLOWPATH)));

    float time;
    size_t nextFrame;
    if ( pPath->currentFrame == pPath->goalFrame )
    {
        time = sithPathMove_MoveToPos(pThing, &pPath->aFrames[pPath->currentFrame]);
        if ( time == 0.0f )
        {
            SITHLOG_ERROR("Thing %s (index %d) reached goal frame %d.\n", pThing->aName, pThing->idx, pPath->goalFrame);
            sithPathMove_Finish(pThing);
            return;
        }

        nextFrame = pPath->goalFrame;
    }
    else
    {
        nextFrame = pPath->goalFrame;
        if ( pPath->currentFrame >= 0 )
        {
            nextFrame = (size_t)((((int)pPath->goalFrame - pPath->currentFrame) >= 0 ? 1 : -1) + pPath->currentFrame);
        }

        SITH_ASSERTREL(nextFrame < pPath->sizeFrames);
        pPath->nextFrame = nextFrame;
        time = sithPathMove_MoveToPos(pThing, &pPath->aFrames[nextFrame]);
    }

    if ( time < 1.0f )
    {
        time = 1.0f;
    }

    sithPathMove_RotateToPYR(pThing, &pPath->aFrames[nextFrame].pyr, time);
}

float J3DAPI sithPathMove_MoveToPos(SithThing* pThing, SithPathFrame* pos)
{
    SITH_ASSERTREL(sithThing_ValidateThingPointer(sithWorld_g_pCurrentWorld, pThing));
    SITH_ASSERTREL(pThing->moveType == SITH_MT_PATH);
    SITH_ASSERTREL(pos);

    SithPathMoveInfo* pPath = &pThing->moveInfo.pathMovement;
    SITH_ASSERTREL(pPath->moveVel > 0.0f);

    rdVector_Sub3(&pPath->vecDeltaPos, &pos->pos, &pThing->pos);
    float dist = rdVector_Normalize3Acc(&pPath->vecDeltaPos);
    if ( dist == 0.0f )
    {
        return pPath->moveTimeRemaining;
    }

    pPath->mode |= SITH_PATHMOVE_MOVE;
    pPath->numBlockedMoves = 0;

    pPath->moveTimeRemaining = dist / pPath->moveVel;
    SITH_ASSERTREL(pPath->moveTimeRemaining > 0.0f);
    return pPath->moveTimeRemaining;
}

void J3DAPI sithPathMove_RotateToPYR(SithThing* pThing, rdVector3* pyr, float time)
{
    SITH_ASSERTREL(pThing && (pThing->moveType == SITH_MT_PATH));
    SITH_ASSERTREL(pyr);
    SITH_ASSERTREL(time > 0.0f);

    SithPathMoveInfo* pPath = &pThing->moveInfo.pathMovement;

    rdVector3 curPYR;
    rdMatrix_ExtractAngles34(&pThing->orient, &curPYR);

    rdVector3 nexGoalPYR;
    rdVector_Sub3(&nexGoalPYR, pyr, &curPYR);

    nexGoalPYR.pitch = stdMath_NormalizeAngleAcute(nexGoalPYR.pitch);
    nexGoalPYR.yaw   = stdMath_NormalizeAngleAcute(nexGoalPYR.yaw);
    nexGoalPYR.roll  = stdMath_NormalizeAngleAcute(nexGoalPYR.roll);

    if ( fabsf(nexGoalPYR.x) < 2.5f )
    {
        nexGoalPYR.x = 0.0f;
    }

    if ( fabsf(nexGoalPYR.y) < 2.5f )
    {
        nexGoalPYR.y = 0.0f;
    }

    if ( fabsf(nexGoalPYR.z) < 2.5f )
    {
        nexGoalPYR.z = 0.0f;
    }

    if ( nexGoalPYR.x != 0.0f || nexGoalPYR.y != 0.0f || nexGoalPYR.z != 0.0f )
    {
        pPath->rotDeltaTime = 1.0f / time;

        rdMatrix_Copy34(&pPath->curOrient, &pThing->orient);
        rdVector_Copy3(&pPath->curOrient.dvec, &rdroid_g_zeroVector3);
        rdVector_Copy3(&pPath->goalPYR, &nexGoalPYR);
        rdVector_Copy3(&pPath->rotateToPYR, pyr);

        pPath->mode |= SITH_PATHMOVE_ROTATE;
        pPath->mode &= ~SITH_PATHMOVE_ROTATE_PIVOT;
        pPath->rotDelta = 0.0f;
    }
}

void J3DAPI sithPathMove_Update(SithThing* pThing, float simTime)
{
    SITH_ASSERTREL(sithThing_ValidateThingPointer(sithWorld_g_pCurrentWorld, pThing));
    SITH_ASSERTREL(pThing->moveType == SITH_MT_PATH);
    SITH_ASSERTREL(simTime > 0.0f);

    if ( simTime != 0.0f && pThing->moveInfo.pathMovement.mode && (pThing->moveInfo.pathMovement.mode & SITH_PATHMOVE_PAUSED) == 0 )
    {
        if ( (pThing->moveInfo.pathMovement.mode & SITH_PATHMOVE_ROTATE) != 0 )
        {
            sithPathMove_UpdateRotate(pThing, simTime);
        }

        if ( (pThing->moveInfo.pathMovement.mode & SITH_PATHMOVE_MOVE) != 0 )
        {
            sithPathMove_UpdateMove(pThing, simTime);
        }

        if ( (pThing->moveInfo.pathMovement.mode & (SITH_PATHMOVE_ROTATE | SITH_PATHMOVE_MOVE)) == 0 )
        {
            sithPathMove_UpdatePath(pThing);
        }
    }
}

int J3DAPI sithPathMove_ParseArg(const StdConffileArg* pArg, SithThing* pThing, int adjNum)
{
    SITH_ASSERTREL((pArg != NULL) && (pThing != NULL));

    switch ( adjNum )
    {
        case SITHTHING_ARG_FRAME:
        {
            if ( pThing->moveInfo.pathMovement.numFrames >= pThing->moveInfo.pathMovement.sizeFrames )
            {
                SITHLOG_ERROR("Too many frames for thing %d.\n", pThing->idx);
                return 1;
            }

            rdVector3 pos, pyr;
            if ( sscanf_s(pArg->argValue, "(%f/%f/%f:%f/%f/%f)", &pos.x, &pos.y, &pos.z, &pyr.pitch, &pyr.yaw, &pyr.roll) != 6 )
            {
                SITHLOG_ERROR("Bad argument %s=%s line %d.\n", pArg->argName, pArg->argValue, stdConffile_GetLineNumber());
                return 0;
            }

            const size_t frameNum = pThing->moveInfo.pathMovement.numFrames;
            rdVector_Copy3(&pThing->moveInfo.pathMovement.aFrames[frameNum].pos, &pos);
            rdVector_Copy3(&pThing->moveInfo.pathMovement.aFrames[frameNum].pyr, &pyr);
            pThing->moveInfo.pathMovement.numFrames = frameNum + 1;
            return 1;
        }
        case SITHTHING_ARG_NUMFRAMES:
        {
            size_t sizeFrames;
            if ( pThing->moveInfo.pathMovement.sizeFrames || (sizeFrames = atoi(pArg->argValue), sizeFrames < 1) )
            {
                SITHLOG_ERROR("Bad argument %s=%s line %d.\n", pArg->argName, pArg->argValue, stdConffile_GetLineNumber());
                return 0;
            }

            SITH_ASSERTREL(pThing->moveInfo.pathMovement.aFrames == NULL);

            pThing->moveInfo.pathMovement.aFrames = (SithPathFrame*)STDMALLOC(sizeof(SithPathFrame) * sizeFrames);
            if ( !pThing->moveInfo.pathMovement.aFrames )
            {
                SITHLOG_ERROR("Memory allocation error, line %d.\n", stdConffile_GetLineNumber());
                return 0;
            }

            memset(pThing->moveInfo.pathMovement.aFrames, 0, sizeof(SithPathFrame) * sizeFrames);
            pThing->moveInfo.pathMovement.sizeFrames = sizeFrames;
            pThing->moveInfo.pathMovement.numFrames  = 0;
            return 1;
        }
    }

    return 0;
}

void J3DAPI sithPathMove_SendMessageBlocked(SithThing* pThing)
{
    SITH_ASSERTREL(pThing->moveType == SITH_MT_PATH);
    if ( (pThing->flags & SITH_TF_COGLINKED) != 0 && (pThing->flags & SITH_TF_REMOTE) == 0 )
    {
        sithCog_ThingSendMessage(pThing, NULL, SITHCOG_MSG_BLOCKED);
    }
}

void J3DAPI sithPathMove_UpdatePath(SithThing* pThing)
{
    if ( (pThing->moveInfo.pathMovement.mode & SITH_PATHMOVE_FOLLOWPATH) != 0 )
    {
        pThing->moveInfo.pathMovement.currentFrame = pThing->moveInfo.pathMovement.nextFrame;
        if ( pThing->moveInfo.pathMovement.currentFrame == pThing->moveInfo.pathMovement.goalFrame )
        {
            sithPathMove_Finish(pThing);
        }
        else
        {
            sithPathMove_FollowNextPath(pThing);
        }
    }
    else
    {
        pThing->moveInfo.pathMovement.currentFrame = pThing->moveInfo.pathMovement.nextFrame;
        sithPathMove_Finish(pThing);
    }
}

void J3DAPI sithPathMove_Finish(SithThing* pThing)
{
    SITH_ASSERTREL(pThing != NULL);

    pThing->moveInfo.pathMovement.mode           &= ~(SITH_PATHMOVE_ROTATE_PIVOT | SITH_PATHMOVE_FOLLOWPATH | SITH_PATHMOVE_ROTATE | SITH_PATHMOVE_MOVE);
    pThing->moveInfo.pathMovement.moveVel         = 0.0f;
    pThing->moveInfo.pathMovement.goalFrame       = 0;
    pThing->moveInfo.pathMovement.nextFrame       = 0;
    pThing->moveInfo.pathMovement.numBlockedMoves = 0;

    sithSoundClass_StopMode(pThing, SITHSOUNDCLASS_MOVING);
    sithSoundClass_PlayModeFirst(pThing, SITHSOUNDCLASS_STOPMOVE);

    if ( (pThing->flags & SITH_TF_COGLINKED) != 0 && (pThing->flags & SITH_TF_REMOTE) == 0 )
    {
        sithCog_ThingSendMessage(pThing, NULL, SITHCOG_MSG_ARRIVED);
    }
}

int J3DAPI sithPathMove_PathMovePause(SithThing* pThing)
{
    SITH_ASSERTREL(pThing);
    SITH_ASSERTREL(pThing->moveType == SITH_MT_PATH);

    if ( (pThing->moveInfo.pathMovement.mode & (SITH_PATHMOVE_ROTATE | SITH_PATHMOVE_MOVE)) == 0 )
    {
        return 0;
    }

    sithSoundClass_StopMode(pThing, SITHSOUNDCLASS_MOVING);
    pThing->moveInfo.pathMovement.mode |= SITH_PATHMOVE_PAUSED;
    return 1;
}

int J3DAPI sithPathMove_PathMoveResume(SithThing* pThing)
{
    SITH_ASSERTREL(pThing);
    SITH_ASSERTREL(pThing->moveType == SITH_MT_PATH);

    if ( (pThing->moveInfo.pathMovement.mode & SITH_PATHMOVE_PAUSED) == 0 )
    {
        return 0;
    }

    sithSoundClass_PlayModeFirst(pThing, SITHSOUNDCLASS_MOVING);
    pThing->moveInfo.pathMovement.mode  &= ~SITH_PATHMOVE_PAUSED;
    return 1;
}

void J3DAPI sithPathMove_Create(SithThing* pNewThing, const SithThing* pMarker, const rdVector3* pOffset)
{
    SITH_ASSERTREL(pNewThing && pMarker);
    SITH_ASSERTREL((pNewThing->moveType == SITH_MT_PATH) && (pMarker->moveType == SITH_MT_PATH));
    SITH_ASSERTREL(!pNewThing->moveInfo.pathMovement.aFrames);

    const size_t sizeFrames = sizeof(SithPathFrame) * pMarker->moveInfo.pathMovement.sizeFrames;
    pNewThing->moveInfo.pathMovement.aFrames = (SithPathFrame*)STDMALLOC(sizeFrames);
    memcpy(pNewThing->moveInfo.pathMovement.aFrames, pMarker->moveInfo.pathMovement.aFrames, sizeFrames);

    pNewThing->moveInfo.pathMovement.sizeFrames = pMarker->moveInfo.pathMovement.sizeFrames;
    pNewThing->moveInfo.pathMovement.numFrames  = pMarker->moveInfo.pathMovement.numFrames;

    for ( size_t i = 0; i < pNewThing->moveInfo.pathMovement.numFrames; ++i )
    {
        SithPathFrame* pFrame = &pNewThing->moveInfo.pathMovement.aFrames[i];

        rdVector3 offset;
        rdVector_Rotate3(&offset, pOffset, &pFrame->pyr);
        rdVector_Add3Acc(&pFrame->pos, &offset);
    }
}

void J3DAPI sithPathMove_UpdateRotate(SithThing* pThing, float simTime)
{
    SithPathMoveInfo* pPath = &pThing->moveInfo.pathMovement;

    float deltaTime = pPath->rotDeltaTime * simTime;
    float rotDelta  = pPath->rotDelta + deltaTime;
    if ( rotDelta <= 1.0f )
    {
        if ( rotDelta < -1.0f )
        {
            deltaTime = -1.0f - pPath->rotDelta;
            rotDelta = -1.0f;
        }
    }
    else
    {
        deltaTime = 1.0f - pPath->rotDelta;
        rotDelta = 1.0f;
    }

    rdVector3 deltaPYR;
    rdVector_Scale3(&deltaPYR, &pPath->goalPYR, rotDelta);

    rdMatrix34 curOrient;
    rdMatrix_Copy34(&curOrient, &pPath->curOrient);
    rdMatrix_PostRotate34(&curOrient, &deltaPYR);

    if ( (pPath->mode & SITH_PATHMOVE_ROTATE_PIVOT) != 0 )
    {
        rdVector3 moveNorm;
        rdVector_Add3(&moveNorm, &pPath->rotOffset, &curOrient.dvec);
        rdVector_Sub3Acc(&moveNorm, &pThing->pos);

        if ( moveNorm.x != 0.0f || moveNorm.y != 0.0f || moveNorm.z != 0.0f )
        {
            float moveDist = stdMath_ClipNearZero(rdVector_Normalize3Acc(&moveNorm));
            if ( moveDist != 0.0f )
            {
                float movedDist = sithCollision_MoveThing(pThing, &moveNorm, moveDist, 0x44);
                if ( movedDist < moveDist )
                {
                    rotDelta = movedDist / moveDist * deltaTime + pPath->rotDelta;
                    rdVector_Scale3(&deltaPYR, &pPath->goalPYR, rotDelta);
                    rdMatrix_Copy34(&curOrient, &pPath->curOrient);
                    rdMatrix_PreRotate34(&curOrient, &deltaPYR);
                }
            }
        }
    }

    pPath->rotDelta = rotDelta;
    rdVector_Copy3(&pThing->orient.dvec, &rdroid_g_zeroVector3);
    sithCollision_ApplyThingOrient(pThing, &curOrient);

    if ( pPath->rotDelta >= 1.0f || pPath->rotDelta <= -1.0f )
    {
        if ( (pPath->mode & SITH_PATHMOVE_ROTATE_PIVOT) == 0 )
        {
            rdMatrix_BuildRotate34(&curOrient, &pPath->rotateToPYR);
            rdVector_Copy3(&pThing->orient.dvec, &rdroid_g_zeroVector3);
            sithCollision_ApplyThingOrient(pThing, &curOrient);
        }

        pPath->mode &= ~(SITH_PATHMOVE_ROTATE_PIVOT | SITH_PATHMOVE_ROTATE);
    }
}

void J3DAPI sithPathMove_UpdateMove(SithThing* pThing, float simTime)
{
    SithPathMoveInfo* pPath = &pThing->moveInfo.pathMovement;

    float timeDelta = simTime;
    if ( pPath->moveTimeRemaining <= (double)simTime )
    {
        timeDelta = pPath->moveTimeRemaining;
    }

    float moveDist = stdMath_ClipNearZero(pPath->moveVel * timeDelta);
    if ( moveDist != 0.0f )
    {
        float movedDist = sithCollision_MoveThing(pThing, &pPath->vecDeltaPos, moveDist, 0x44);
        if ( movedDist < moveDist )
        {
            if ( stdMath_ClipNearZero(movedDist) > 0.0f )
            {
                timeDelta = movedDist / moveDist * timeDelta;
            }
            else
            {
                timeDelta = 0.0f;
            }

            ++pPath->numBlockedMoves;
        }
    }

    pPath->moveTimeRemaining -= timeDelta;
    if ( stdMath_ClipNearZero(pPath->moveTimeRemaining) == 0.0f )
    {
        pPath->mode &= ~SITH_PATHMOVE_MOVE;
    }
    else if ( pPath->numBlockedMoves > 2u )
    {
        sithPathMove_SendMessageBlocked(pThing);
    }
}

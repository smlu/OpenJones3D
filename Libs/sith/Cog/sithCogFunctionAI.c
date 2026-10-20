#include "sithCogFunctionAI.h"
#include <j3dcore/j3dhook.h>

#include <rdroid/Math/rdMatrix.h>
#include <rdroid/Math/rdVector.h>

#include <sith/AI/sithAI.h>
#include <sith/AI/sithAIMove.h>
#include <sith/AI/sithAIUtil.h>
#include <sith/Cog/sithCog.h>
#include <sith/Cog/sithCogExec.h>
#include <sith/Engine/sithPuppet.h>
#include <sith/Gameplay/sithEvent.h>
#include <sith/Gameplay/sithInventory.h>
#include <sith/Gameplay/sithPlayer.h>
#include <sith/Gameplay/sithTime.h>
#include <sith/RTI/symbols.h>
#include <sith/World/sithSoundClass.h>

#include <std/General/stdMath.h>
#include <std/General/stdUtil.h>

static SithThing* sithCogFunctionAI_aThingsInView[32];
static int sithCogFunctionAI_numThingsInView;
static int sithCogFunctionAI_curThingInView;

void J3DAPI sithCogFunctionAI_AISetMoveSpeed(SithCog* pCog)
{
    SITH_ASSERTREL(pCog);

    float speed = sithCogExec_PopFlex(pCog);
    SithThing* pThing = sithCogExec_PopThing(pCog);

    if ( !pThing || pThing->controlType != SITH_CT_AI || !pThing->controlInfo.aiControl.pLocal )
    {
        SITHLOG_ERROR("Cog %s: Bad thing in AISetMoveSpeed()\n", pCog->aName);
        return;
    }

    if ( speed < 0.0f || speed > 10.0f )
    {
        SITHLOG_ERROR("Cog %s: Bad move speed in AISetMoveSpeed().  Capping at 10.f\n", pCog->aName);
        speed = STDMATH_CLAMP(speed, 0.0f, 10.0f);
    }

    pThing->controlInfo.aiControl.pLocal->moveSpeed = speed;
}

void J3DAPI sithCogFunctionAI_AISetMovePos(SithCog* pCog)
{
    SITH_ASSERTREL(pCog);

    int bWait = sithCogExec_PopInt(pCog);

    SithThing* pThing;
    rdVector3 pos;
    if ( !sithCogExec_PopVector(pCog, &pos)
        || (pThing = sithCogExec_PopThing(pCog)) == 0
        || pThing->controlType != SITH_CT_AI
        || !pThing->controlInfo.aiControl.pLocal )
    {
        SITHLOG_ERROR("Cog %s: Bad thing or vector for AISetMovePos\n", pCog->aName);
        return;
    }

    if ( (pThing->controlInfo.aiControl.pLocal->submode & (SITHAI_SUBMODE_UNKNOWN_2 | SITHAI_SUBMODE_UNKNOWN_1)) != 0 )
    {
        sithAIMove_StopAIMovement(pThing->controlInfo.aiControl.pLocal);
    }

    if ( sithAIMove_AISetMovePos(pThing->controlInfo.aiControl.pLocal, &pos, pThing->controlInfo.aiControl.pLocal->moveSpeed) )
    {
        if ( bWait )
        {
            pCog->status = SITHCOG_STATUS_WAITING_AI_TO_STOP;
            pCog->statusParams[0] = pThing->idx;
        }
    }
    else
    {
        sithCog_ThingSendMessage(pThing->controlInfo.aiControl.pLocal->pOwner, NULL, SITHCOG_MSG_ARRIVED);
    }
}

void J3DAPI sithCogFunctionAI_AIGetHomePos(SithCog* pCog)
{
    SITH_ASSERTREL(pCog);

    SithThing* pThing = sithCogExec_PopThing(pCog);
    if ( !pThing )
    {
        SITHLOG_ERROR("Cog %s: Bad thing passed to AIGetHomePos\n", pCog->aName);
        sithCogExec_PushInt(pCog, -1);
        return;
    }

    int bPushZero = 0;
    if ( pThing->controlType != SITH_CT_AI || !pThing->controlInfo.aiControl.pLocal )
    {
        SITHLOG_ERROR("Cog %s: thing %s is not AI controlled in AIGetHomePos\n", pCog->aName, pThing->aName);
        bPushZero = 1;
    }

    if ( bPushZero )
    {
        sithCogExec_PushVector(pCog, &rdroid_g_zeroVector3);
    }
    else
    {
        sithCogExec_PushVector(pCog, &pThing->controlInfo.aiControl.pLocal->homePos);
    }
}

void J3DAPI sithCogFunctionAI_AISetMaxHomeDist(SithCog* pCog)
{
    float dist = sithCogExec_PopFlex(pCog);
    SithThing* pThing = sithCogExec_PopThing(pCog);

    if ( !pThing )
    {
        SITHLOG_ERROR("Cog %s: Invalid thing passed to AISetMaxHomeDist()\n", pCog->aName);
    }

    else if ( pThing->controlType != SITH_CT_AI || !pThing->controlInfo.aiControl.pLocal )
    {
        SITHLOG_ERROR("Cog %s: THING '%s' is not AI controlled in AISetMaxHomeDist\n", pCog->aName, pThing->aName);
    }
    else
    {
        pThing->controlInfo.aiControl.pLocal->maxHomeDist = dist;
    }
}

void J3DAPI sithCogFunctionAI_AIGetGoalThing(SithCog* pCog)
{
    SITH_ASSERTREL(pCog);

    SithThing* pThing = sithCogExec_PopThing(pCog);
    if ( !pThing )
    {
        SITHLOG_ERROR("Cog %s: Bad thing passed to AIGetGoalThing\n", pCog->aName);
        sithCogExec_PushInt(pCog, -1); // Fixed: Push -1 on error (original Indy3D.exe v1.0 0x004691C0)
        return;
    }

    if ( pThing->controlType != SITH_CT_AI || !pThing->controlInfo.aiControl.pLocal )
    {
        SITHLOG_ERROR("Cog %s: thing %s is not AI controlled in AIGetGoalThing\n", pCog->aName, pThing->aName);
        sithCogExec_PushInt(pCog, -1); // Fixed: Push -1 on error (original Indy3D.exe v1.0 0x004691C0)
        return;
    }

    if ( !pThing->controlInfo.aiControl.pLocal->goalThing )
    {
        SITHLOG_ERROR("NOTE: Cog %s: thing %s has no goal thing in AIGetGoalThing\n", pCog->aName, pThing->aName);
        sithCogExec_PushInt(pCog, -1);
        return;
    }

    sithCogExec_PushInt(pCog, pThing->controlInfo.aiControl.pLocal->goalThing->idx);
}

void J3DAPI sithCogFunctionAI_AIGetGoalLVec(SithCog* pCog)
{
    SITH_ASSERTREL(pCog);

    SithThing* pThing = sithCogExec_PopThing(pCog);
    if ( !pThing )
    {
        SITHLOG_ERROR("Cog %s: Bad THING passed to AIGetGoalLVec\n", pCog->aName);
        return;
    }

    if ( pThing->controlType == SITH_CT_AI )
    {
        if ( !pThing->controlInfo.aiControl.pLocal )
        {
            SITHLOG_ERROR("Cog %s: THING %s is not AI controlled in AIGetGoalLVec\n", pCog->aName, pThing->aName);
            sithCogExec_PushVector(pCog, &rdroid_g_zeroVector3);
            return;
        }
    }

    sithCogExec_PushVector(pCog, &pThing->controlInfo.aiControl.pLocal->goalLVec);
}

void J3DAPI sithCogFunctionAI_AIJump(SithCog* pCog)
{
    SITH_ASSERTREL(pCog);

    rdVector3 movePos;
    float a3 = sithCogExec_PopFlex(pCog);
    int bVec = sithCogExec_PopVector(pCog, &movePos);
    SithThing* pThing = sithCogExec_PopThing(pCog);

    if ( !pThing
        || !bVec
        || !pThing->attach.flags
        || pThing->controlType != SITH_CT_AI
        || !pThing->controlInfo.aiControl.pLocal )
    {
        SITHLOG_ERROR("Bad params in aijump.\n");
    }
    else
    {
        sithAIMove_AIJump(pThing->controlInfo.aiControl.pLocal, &movePos, a3);
    }
}

void J3DAPI sithCogFunctionAI_AISetMoveFrame(SithCog* pCog)
{
    SITH_ASSERTREL(pCog);
    size_t frame = sithCogExec_PopInt(pCog);
    SithThing* pThing = sithCogExec_PopThing(pCog);

    if ( !pThing || pThing->controlType != SITH_CT_AI || !pThing->controlInfo.aiControl.pLocal )
    {
        SITHLOG_ERROR("Cog %s: Bad thing in AISetMoveFrame\n", pCog->aName);
        return;
    }

    if ( !pThing->controlInfo.aiControl.pLocal->aFrames || frame >= pThing->controlInfo.aiControl.pLocal->numFrames )
    {
        SITHLOG_ERROR("Cog %s: Bad frame in AISetMoveFrame\n", pCog->aName);
        return;
    }

    sithAIMove_AISetMovePos(
        pThing->controlInfo.aiControl.pLocal,
        &pThing->controlInfo.aiControl.pLocal->aFrames[frame],
        pThing->controlInfo.aiControl.pLocal->moveSpeed
    );
}

void J3DAPI sithCogFunctionAI_AISetMoveThing(SithCog* pCog)
{
    SITH_ASSERTREL(pCog);
    int bWait = sithCogExec_PopInt(pCog);
    SithThing* pGoalThing = sithCogExec_PopThing(pCog);
    SithThing* pThing = sithCogExec_PopThing(pCog);

    if ( !pThing
        || !pGoalThing
        || pThing->controlType != SITH_CT_AI
        || !pThing->controlInfo.aiControl.pLocal )
    {
        SITHLOG_ERROR("Cog %s: Bad args for AISetMoveThing\n", pCog->aName);
    }
    else
    {
        pThing->controlInfo.aiControl.pLocal->goalThing = pGoalThing;
        sithAIMove_AISetMovePos(
            pThing->controlInfo.aiControl.pLocal,
            &pGoalThing->pos,
            pThing->controlInfo.aiControl.pLocal->moveSpeed
        );

        if ( bWait )
        {
            pCog->status = SITHCOG_STATUS_WAITING_AI_TO_STOP;
            pCog->statusParams[0] = pThing->idx;
        }
    }
}

void J3DAPI sithCogFunctionAI_AISetGoalThing(SithCog* pCog)
{
    SITH_ASSERTREL(pCog);
    SithThing* pGoalThing = sithCogExec_PopThing(pCog);
    SithThing* pThing = sithCogExec_PopThing(pCog);

    if ( !pThing
        || !pGoalThing
        || pThing->controlType != SITH_CT_AI
        || !pThing->controlInfo.aiControl.pLocal )
    {
        SITHLOG_ERROR("Cog %s: Bad args for AISetGoalThing\n", pCog->aName);
    }
    else
    {
        pThing->controlInfo.aiControl.pLocal->goalThing = pGoalThing;
    }
}

void J3DAPI sithCogFunctionAI_AIEnableHeadTracking(SithCog* pCog)
{
    SITH_ASSERTREL(pCog);
    SithThing* pTarget = sithCogExec_PopThing(pCog);
    SithThing* pThing = sithCogExec_PopThing(pCog);

    if ( !pThing
        || !pTarget
        || pThing->controlType != SITH_CT_AI
        || !pThing->controlInfo.aiControl.pLocal )
    {
        SITHLOG_ERROR("Cog %s: Bad args for AIEnableHeadTracking\n", pCog->aName);
    }
    else
    {
        pThing->controlInfo.aiControl.pLocal->pTargetThing = pTarget;
        pThing->controlInfo.aiControl.pLocal->submode |= SITHAI_SUBMODE_HEADTRACKINGMOTION | SITHAI_SUBMODE_UNKNOWN_800 | SITHAI_SUBMODE_UNKNOWN_400;
    }
}

void J3DAPI sithCogFunctionAI_AIDisableHeadTracking(SithCog* pCog)
{
    SITH_ASSERTREL(pCog);
    SithThing* pThing = sithCogExec_PopThing(pCog);
    if ( !pThing || pThing->controlType != SITH_CT_AI || !pThing->controlInfo.aiControl.pLocal )
    {
        SITHLOG_ERROR("Cog %s: Bad args for AIDisableHeadTracking\n", pCog->aName);
    }
    else
    {
        pThing->controlInfo.aiControl.pLocal->submode &= ~(SITHAI_SUBMODE_HEADTRACKINGMOTION | SITHAI_SUBMODE_UNKNOWN_400); // Note, SITHAI_SUBMODE_UNKNOWN_800 not being cleared
    }
}

void J3DAPI sithCogFunctionAI_AIEnableBodyTracking(SithCog* pCog)
{
    SITH_ASSERTREL(pCog);
    SithThing* pTarget = sithCogExec_PopThing(pCog);
    SithThing* pThing = sithCogExec_PopThing(pCog);

    if ( !pThing
        || !pTarget
        || pThing->controlType != SITH_CT_AI
        || !pThing->controlInfo.aiControl.pLocal )
    {
        SITHLOG_ERROR("Cog %s: Bad args for AIEnableBodyTracking\n", pCog->aName);
    }
    else
    {
        pThing->controlInfo.aiControl.pLocal->pTargetThing = pTarget;
        pThing->controlInfo.aiControl.pLocal->submode |= SITHAI_SUBMODE_BODYTRACKINGMOTION;
    }
}

void J3DAPI sithCogFunctionAI_AIDisableBodyTracking(SithCog* pCog)
{
    SITH_ASSERTREL(pCog);
    SithThing* pThing = sithCogExec_PopThing(pCog);
    if ( !pThing || pThing->controlType != SITH_CT_AI || !pThing->controlInfo.aiControl.pLocal )
    {
        SITHLOG_ERROR("Cog %s: Bad args for AIDisableBodyTracking\n", pCog->aName);
    }
    else
    {
        pThing->controlInfo.aiControl.pLocal->submode &= ~SITHAI_SUBMODE_BODYTRACKINGMOTION;
    }
}

void J3DAPI sithCogFunctionAI_AISetLookPos(SithCog* pCog)
{
    SITH_ASSERTREL(pCog);
    SithThing* pThing;
    rdVector3 targetPos;
    if ( !sithCogExec_PopVector(pCog, &targetPos)
        || (pThing = sithCogExec_PopThing(pCog)) == NULL
        || pThing->controlType != SITH_CT_AI
        || !pThing->controlInfo.aiControl.pLocal )
    {
        SITHLOG_ERROR("Cog %s: Bad thing or vector for AISetLookPos\n", pCog->aName);
    }
    else
    {
        sithAIMove_AISetLookPos(pThing->controlInfo.aiControl.pLocal, &targetPos);
    }
}

void J3DAPI sithCogFunctionAI_AISetLookFrame(SithCog* pCog)
{
    SITH_ASSERTREL(pCog);
    size_t frameNum = sithCogExec_PopInt(pCog);
    SithThing* pThing = sithCogExec_PopThing(pCog);
    if ( !pThing
        || pThing->controlType != SITH_CT_AI
        || !pThing->controlInfo.aiControl.pLocal
        || !pThing->controlInfo.aiControl.pLocal->aFrames
        || frameNum >= pThing->controlInfo.aiControl.pLocal->numFrames )
    {
        SITHLOG_ERROR("Cog %s: Bad thing or frame for AISetLookFrame\n", pCog->aName);
    }
    else
    {
        sithAIMove_AISetLookPos(
            pThing->controlInfo.aiControl.pLocal,
            &pThing->controlInfo.aiControl.pLocal->aFrames[frameNum]
        );
    }
}

void J3DAPI sithCogFunctionAI_AIGetMovePos(SithCog* pCog)
{
    SITH_ASSERTREL(pCog);
    SithThing* pThing = sithCogExec_PopThing(pCog);
    if ( !pThing || pThing->controlType != SITH_CT_AI || !pThing->controlInfo.aiControl.pLocal )
    {
        SITHLOG_ERROR("Cog %s: Invalid AI THING passed to AIGetMovePos.\n", pCog->aName);
        // TODO: Shouldn't return zero vector?
    }
    else
    {
        sithCogExec_PushVector(pCog, &pThing->controlInfo.aiControl.pLocal->movePos);
    }
}

void J3DAPI sithCogFunctionAI_AIPauseMove(SithCog* pCog)
{
    SITH_ASSERTREL(pCog);
    int msecPause = sithCogExec_PopInt(pCog);
    SithThing* pThing = sithCogExec_PopThing(pCog);
    if ( !pThing || pThing->controlType != SITH_CT_AI || !pThing->controlInfo.aiControl.pLocal )
    {
        SITHLOG_ERROR("Cog %s: Invalid AI THING passed to AIPauseMove.\n", pCog->aName);
    }
    else
    {
        sithAIUtil_AIPauseMove(pThing->controlInfo.aiControl.pLocal, msecPause);
    }
}

void J3DAPI sithCogFunctionAI_AISetMode(SithCog* pCog)
{
    SITH_ASSERTREL(pCog);
    SithAIMode newMode = sithCogExec_PopInt(pCog);
    SithThing* pThing = sithCogExec_PopThing(pCog);
    if ( !pThing || pThing->controlType != SITH_CT_AI || !pThing->controlInfo.aiControl.pLocal )
    {
        SITHLOG_ERROR("Cog %s: Bad thing passed to AISetMode.\n", pCog->aName);
    }
    else
    {
        SithAIMode oldmode = pThing->controlInfo.aiControl.pLocal->mode;
        pThing->controlInfo.aiControl.pLocal->mode |= newMode;
        if ( oldmode != pThing->controlInfo.aiControl.pLocal->mode )
        {
            sithAI_EmitEvent(pThing->controlInfo.aiControl.pLocal, SITHAI_EVENT_MODECHANGED, (void*)oldmode);
        }
    }
}

void J3DAPI sithCogFunctionAI_AIGetMode(SithCog* pCog)
{
    SITH_ASSERTREL(pCog);
    SithThing* pThing = sithCogExec_PopThing(pCog);
    if ( !pThing || pThing->controlType != SITH_CT_AI || !pThing->controlInfo.aiControl.pLocal )
    {
        SITHLOG_ERROR("Cog %s: Bad thing passed to AIGetMode.\n", pCog->aName);
        sithCogExec_PushInt(pCog, -1);
    }
    else
    {
        sithCogExec_PushInt(pCog, pThing->controlInfo.aiControl.pLocal->mode);
    }
}

void J3DAPI sithCogFunctionAI_AIClearMode(SithCog* pCog)
{
    SITH_ASSERTREL(pCog);
    SithAIMode mode = sithCogExec_PopInt(pCog);
    SithThing* pThing = sithCogExec_PopThing(pCog);
    if ( !pThing || pThing->controlType != SITH_CT_AI || !pThing->controlInfo.aiControl.pLocal )
    {
        SITHLOG_ERROR("Cog %s: Bad thing passed to AIClearMode.\n", pCog->aName);
    }
    else
    {
        SithAIMode oldmode = pThing->controlInfo.aiControl.pLocal->mode;
        pThing->controlInfo.aiControl.pLocal->mode &= ~mode;
        if ( oldmode != pThing->controlInfo.aiControl.pLocal->mode )
        {
            sithAI_EmitEvent(pThing->controlInfo.aiControl.pLocal, SITHAI_EVENT_MODECHANGED, (void*)oldmode);
        }
    }
}

void J3DAPI sithCogFunctionAI_AIGetSubMode(SithCog* pCog)
{
    SITH_ASSERTREL(pCog);
    SithThing* pThing = sithCogExec_PopThing(pCog);
    if ( !pThing || pThing->controlType != SITH_CT_AI || !pThing->controlInfo.aiControl.pLocal )
    {
        SITHLOG_ERROR("Cog %s: Bad thing passed to AIGetSubMode.\n", pCog->aName);
        sithCogExec_PushInt(pCog, -1);
    }
    else
    {
        sithCogExec_PushInt(pCog, pThing->controlInfo.aiControl.pLocal->submode);
    }
}

void J3DAPI sithCogFunctionAI_AISetSubMode(SithCog* pCog)
{
    SITH_ASSERTREL(pCog);
    SithAISubMode submode = sithCogExec_PopInt(pCog);
    SithThing* pThing = sithCogExec_PopThing(pCog);
    if ( !pThing || pThing->controlType != SITH_CT_AI || !pThing->controlInfo.aiControl.pLocal )
    {
        SITHLOG_ERROR("Cog %s: Bad thing passed to AISetSubMode.\n", pCog->aName);
    }
    else
    {
        pThing->controlInfo.aiControl.pLocal->submode |= submode;
    }
}

void J3DAPI sithCogFunctionAI_AIClearSubMode(SithCog* pCog)
{
    SITH_ASSERTREL(pCog);
    SithAISubMode submode = sithCogExec_PopInt(pCog);
    SithThing* pThing = sithCogExec_PopThing(pCog);
    if ( !pThing || pThing->controlType != SITH_CT_AI || !pThing->controlInfo.aiControl.pLocal )
    {
        SITHLOG_ERROR("Cog %s: Bad thing passed to AIClearSubMode.\n", pCog->aName);
    }
    else
    {
        pThing->controlInfo.aiControl.pLocal->submode &= ~submode;
    }
}

// Gets a thing in the specified view from a thing. 
// Use : 
//     FirstThingInView(ThingThatFinds, FOV(in angle), Distance(jku), WhatToLookForFlag);
//     Flag is the thing type value. The Type is ThingTypeMask
//     For ie if flag is 0x4, then actor thing should be returned
//     Returns the thing found.
// 
// From:    
//     https://web.archive.org/web/20011212235446/http://www.sfc.keio.ac.jp:80/~t00214ho/millennium/cogverb/
// 
// Forum:
//     https://web.archive.org/web/20200114134644/https://forums.massassi.net/vb3/archive/index.php/t-14421.html
void J3DAPI sithCogFunctionAI_FirstThingInView(SithCog* pCog)
{
    int thingTypeMask = sithCogExec_PopInt(pCog);
    float distance    = sithCogExec_PopFlex(pCog);
    float fovX        = sithCogExec_PopFlex(pCog);
    SithThing* pThing = sithCogExec_PopThing(pCog);

    if ( !pThing )
    {
        SITHLOG_ERROR("Cog %s: Bad thing passed to FirstThingInView.\n", pCog->aName);
        sithCogExec_PushInt(pCog, -1);
        return;
    }

    rdMatrix34 orient = pThing->orient;
    if ( pThing->type == SITH_THING_ACTOR || pThing->type == SITH_THING_PLAYER )
    {
        rdMatrix_PreRotate34(&orient, &pThing->thingInfo.actorInfo.headPYR);
        rdMatrix_PostTranslate34(&orient, &pThing->pos);
        rdMatrix_PreTranslate34(&orient, &pThing->thingInfo.actorInfo.eyeOffset);
    }
    else
    {
        rdMatrix_PostTranslate34(&orient, &pThing->pos);
    }

    sithCogFunctionAI_numThingsInView = sithAIUtil_GetThingsInView(
        pThing->pInSector,
        &orient,
        fovX,
        fovX,
        STD_ARRAYLEN(sithCogFunctionAI_aThingsInView),
        sithCogFunctionAI_aThingsInView,
        thingTypeMask,
        distance
    );

    sithCogFunctionAI_curThingInView = 0;
    if ( sithCogFunctionAI_numThingsInView > 0 && sithCogFunctionAI_aThingsInView[0] )
    {
        sithCogExec_PushInt(pCog, sithCogFunctionAI_aThingsInView[0]->idx);
    }
    else
    {
        sithCogExec_PushInt(pCog, -1);
    }
}

void J3DAPI sithCogFunctionAI_NextThingInView(SithCog* pCog)
{
    size_t num = ++sithCogFunctionAI_curThingInView;
    SithThing* pThing = NULL;
    if ( sithCogFunctionAI_curThingInView < sithCogFunctionAI_numThingsInView
        && (pThing = sithCogFunctionAI_aThingsInView[num]) != NULL )
    {
        sithCogExec_PushInt(pCog, pThing->idx);
    }
    else
    {
        sithCogExec_PushInt(pCog, -1);
    }
}

void J3DAPI sithCogFunctionAI_ThingViewDot(SithCog* pCog)
{
    SithThing* pThing2 = sithCogExec_PopThing(pCog);
    SithThing* pThing1 = sithCogExec_PopThing(pCog);
    if ( !pThing2 || !pThing1 )
    {
        SITHLOG_ERROR("Cog %s: Bad thing or player passed to ThingViewDot.\n", pCog->aName);
        sithCogExec_PushFlex(pCog, -1000.0f);
        return;
    }

    rdMatrix34 orient1 = pThing1->orient;
    if ( pThing1->type == SITH_THING_ACTOR || pThing1->type == SITH_THING_PLAYER )
    {
        rdMatrix_PreRotate34(&orient1, &pThing1->thingInfo.actorInfo.headPYR);
    }

    rdVector3 lvec =  orient1.lvec;
    rdVector_Normalize3Acc(&lvec);

    rdVector3 dir;
    rdVector_Sub3(&dir, &pThing2->pos, &pThing1->pos);
    rdVector_Normalize3Acc(&dir);

    float dot = rdVector_Dot3(&lvec, &dir);
    sithCogExec_PushFlex(pCog, dot);
}

void J3DAPI sithCogFunctionAI_AISetFireTarget(SithCog* pCog)
{
    SithThing* pTarget = sithCogExec_PopThing(pCog);
    SithThing* pThing = sithCogExec_PopThing(pCog);
    if ( !pThing
        || pThing->controlType != SITH_CT_AI
        || !pThing->controlInfo.aiControl.pLocal
        || !pTarget )
    {
        SITHLOG_ERROR("Bad parameters to SetFireTarget.\n");
    }
    else
    {
        SithAIMode oldmode = pThing->controlInfo.aiControl.pLocal->mode;

        pThing->controlInfo.aiControl.pLocal->pTargetThing    = pTarget;
        pThing->controlInfo.aiControl.pLocal->goalThing       = pTarget;
        pThing->controlInfo.aiControl.pLocal->msecAttackStart = sithTime_g_msecGameTime;
        sithAIUtil_AISetMode(pThing->controlInfo.aiControl.pLocal, SITHAI_MODE_ATTACKING);

        if ( (pThing->controlInfo.aiControl.pLocal->submode & SITHAI_SUBMODE_CONTINUOUSWPNTMOTION) == 0 )
        {
            sithAIUtil_AIClearMode(pThing->controlInfo.aiControl.pLocal, SITHAI_MODE_TRAVERSEWPNTS);
        }

        sithAI_ForceInstinctUpdate(pThing->controlInfo.aiControl.pLocal, "primaryfire", 0);
        sithAI_ForceInstinctUpdate(pThing->controlInfo.aiControl.pLocal, "lobfire", 0);
        sithAI_ForceInstinctUpdate(pThing->controlInfo.aiControl.pLocal, "alternatefire", 0);

        if ( pThing->controlInfo.aiControl.pLocal->mode != oldmode )
        {
            sithAI_EmitEvent(pThing->controlInfo.aiControl.pLocal, SITHAI_EVENT_MODECHANGED, (void*)oldmode);
        }
    }
}

void J3DAPI sithCogFunctionAI_AIEnableInstinct(SithCog* pCog)
{
    int bEnable = sithCogExec_PopInt(pCog);
    const char* pInstinctName = sithCogExec_PopString(pCog);
    SithThing* pThing = sithCogExec_PopThing(pCog);
    if ( !pThing
        || !pInstinctName
        || pThing->controlType != SITH_CT_AI
        || !pThing->controlInfo.aiControl.pLocal )
    {
        SITHLOG_ERROR("Bad AI THING in EnableInstinct().\n");
        sithCogExec_PushInt(pCog, 0);
    }
    else
    {
        int bSuccess = sithAI_EnableInstinct(pThing->controlInfo.aiControl.pLocal, pInstinctName, bEnable);
        sithCogExec_PushInt(pCog, bSuccess);
    }
}

void J3DAPI sithCogFunctionAI_AISetCutsceneMode(SithCog* pCog)
{
    SithThing* pThing = sithCogExec_PopThing(pCog);
    if ( !pThing )
    {
        SITHLOG_ERROR("Cog %s: Bad AI THING in AISetCutsceneMode().\n", pCog->aName);
        return;
    }

    if ( pThing->controlType != SITH_CT_AI || !pThing->controlInfo.aiControl.pLocal )
    {
        SITHLOG_ERROR("Cog %s: Thing %s is not AI controlled in AISetCutsceneMode().\n", pCog->aName, pThing->aName);
        return;
    }

    sithAIUtil_AISetMode(pThing->controlInfo.aiControl.pLocal, SITHAI_MODE_BLOCK);
}

void J3DAPI sithCogFunctionAI_AIClearCutsceneMode(SithCog* pCog)
{
    SithThing* pThing = sithCogExec_PopThing(pCog);
    if ( !pThing )
    {
        SITHLOG_ERROR("Cog %s: Bad AI THING in AIClearCutsceneMode().\n", pCog->aName);
        return;
    }

    if ( pThing->controlType != SITH_CT_AI || !pThing->controlInfo.aiControl.pLocal )
    {
        SITHLOG_ERROR("Cog %s: Thing %s is not AI controlled in AIClearCutsceneMode().\n", pCog->aName, pThing->aName);
        return;
    }

    sithAIUtil_AISetMode(pThing->controlInfo.aiControl.pLocal, SITHAI_MODE_SEARCHING);
}

void J3DAPI sithCogFunctionAI_IsAITargetInSight(SithCog* pCog)
{
    SithThing* pThing = sithCogExec_PopThing(pCog);
    if ( !pThing
        || pThing->type != SITH_THING_ACTOR
        || pThing->controlType != SITH_CT_AI
        || !pThing->controlInfo.aiControl.pLocal )
    {
        SITHLOG_ERROR("Cog %s: Invalid AI THING in IsAITargetInSight.\n", pCog->aName);
        return;
    }

    if ( pThing->controlInfo.aiControl.pLocal->pTargetThing && pThing->controlInfo.aiControl.pLocal->targetSightState == 0 )// 0 - target in sight
    {
        sithCogExec_PushInt(pCog, 1);
        return;
    }

    sithCogExec_PushInt(pCog, 0);
}

void J3DAPI sithCogFunctionAI_AIFlee(SithCog* pCog)
{
    SithThing* pFleeFromThing = sithCogExec_PopThing(pCog);
    SithThing* pThing = sithCogExec_PopThing(pCog);
    if ( !pFleeFromThing
        || !pThing
        || pThing->type != SITH_THING_ACTOR
        || pThing->controlType != SITH_CT_AI
        || !pThing->controlInfo.aiControl.pLocal )
    {
        SITHLOG_ERROR("Cog %s: Invalid AI thing in AIFlee.\n", pCog->aName);
    }
    else
    {
        pThing->controlInfo.aiControl.pLocal->pFleeFromThing = pFleeFromThing;
        SithAIMode oldmode = pThing->controlInfo.aiControl.pLocal->mode;
        if ( (oldmode & SITHAI_MODE_FLEEING) == 0 )
        {
            sithAIUtil_AISetMode(pThing->controlInfo.aiControl.pLocal, SITHAI_MODE_FLEEING);
            sithAI_EmitEvent(pThing->controlInfo.aiControl.pLocal, SITHAI_EVENT_MODECHANGED, (void*)oldmode);
        }
    }
}

void J3DAPI sithCogFunctionAI_AIStopFlee(SithCog* pCog)
{
    SithThing* pThing = sithCogExec_PopThing(pCog);
    if ( !pThing )
    {
        SITHLOG_ERROR("Cog %s: Invalid thing in AIStopFlee.\n", pCog->aName);
        return;
    }

    if ( pThing->type != SITH_THING_ACTOR
        || pThing->controlType != SITH_CT_AI
        || !pThing->controlInfo.aiControl.pLocal )
    {
        SITHLOG_ERROR("Cog %s: Thing %s is not AI controlled in AIStopFlee.\n", pCog->aName, pThing->aName);
        return;
    }

    SithAIMode oldmode = pThing->controlInfo.aiControl.pLocal->mode;
    if ( (oldmode & SITHAI_MODE_FLEEING) != 0 )
    {
        sithAIUtil_AISetMode(pThing->controlInfo.aiControl.pLocal, SITHAI_MODE_SEARCHING);
        sithAI_EmitEvent(pThing->controlInfo.aiControl.pLocal, SITHAI_EVENT_MODECHANGED, (void*)oldmode);
        pThing->controlInfo.aiControl.pLocal->pFleeFromThing = NULL;
    }
}

void J3DAPI sithCogFunctionAI_AISetClass(SithCog* pCog)
{
    SithAIClass* pClass = sithCogExec_PopAIClass(pCog);
    SithThing* pThing = sithCogExec_PopThing(pCog);
    if ( !pClass || !pThing || pThing->controlType != SITH_CT_AI || !pThing->controlInfo.aiControl.pLocal )
    {
        SITHLOG_ERROR("Cog %s: Bad arguments for AISetClass.\n", pCog->aName);
        return;
    }

    pThing->controlInfo.aiControl.pClass = pClass;
    pThing->controlInfo.aiControl.pLocal->pClass = pClass;
    pThing->controlInfo.aiControl.pLocal->numInstincts = pClass->numInstincts;
}

void J3DAPI sithCogFunctionAI_AISetLookThing(SithCog* pCog)
{
    SITH_ASSERTREL(pCog);
    SithThing* pTarget = sithCogExec_PopThing(pCog);
    SithThing* pThing = sithCogExec_PopThing(pCog);
    if ( !pThing )
    {
        SITHLOG_ERROR("Cog %s: Bad thing in AISetLookThing()\n", pCog->aName);
    }
    else if ( pThing->controlType != SITH_CT_AI || !pThing->controlInfo.aiControl.pLocal )
    {
        SITHLOG_ERROR("Cog %s: thing %s is not AI controlled in AISetLookThing()\n", pCog->aName, pThing->aName);
    }
    else if ( !pTarget )
    {
        SITHLOG_ERROR("Cog %s: Bad target in AISetLookThing()\n", pCog->aName);
    }
    else
    {
        sithAIMove_AISetLookThing(pThing->controlInfo.aiControl.pLocal, pTarget);
    }
}

void J3DAPI sithCogFunctionAI_AISetLookThingEyeLevel(SithCog* pCog)
{
    SITH_ASSERTREL(pCog);
    SithThing* pTarget = sithCogExec_PopThing(pCog);
    SithThing* pThing = sithCogExec_PopThing(pCog);
    if ( !pThing )
    {
        SITHLOG_ERROR("Cog %s: Bad thing in AISetLookThingEyeLevel()\n", pCog->aName);
    }
    else if ( pThing->controlType != SITH_CT_AI || !pThing->controlInfo.aiControl.pLocal )
    {
        SITHLOG_ERROR("Cog %s: thing %s is not AI controlled in AISetLookThingEyeLevel()\n", pCog->aName, pThing->aName);
    }
    else if ( !pTarget )
    {
        SITHLOG_ERROR("Cog %s: Bad target in AISetLookThingEyeLevel()\n", pCog->aName);
    }
    else
    {
        sithAIMove_AISetLookPosEyeLevel(pThing->controlInfo.aiControl.pLocal, &pTarget->pos);
    }
}

void J3DAPI sithCogFunctionAI_AIWaitForStop(SithCog* pCog)
{
    SITH_ASSERTREL(pCog);
    SithThing* pThing = sithCogExec_PopThing(pCog);
    if ( !pThing || pThing->controlType != SITH_CT_AI || !pThing->controlInfo.aiControl.pLocal )
    {
        SITHLOG_ERROR("Cog %s: Bad AI THING in AIWaitForStop()\n", pCog->aName);
    }
    else if ( (pThing->controlInfo.aiControl.pLocal->mode & (SITHAI_MODE_TURNING | SITHAI_MODE_MOVING)) == 0 )
    {
        SITHLOG_ERROR("Cog %s: '%s' is neither moving nor turning in AIWaitForStop().  Execution continuing.\n", pCog->aName, pThing->aName);
    }
    else
    {
        pCog->status = SITHCOG_STATUS_WAITING_AI_TO_STOP;
        pCog->statusParams[0] = pThing->idx;
    }
}

void J3DAPI sithCogFunctionAI_AIWaitForHeadTracking(SithCog* pCog)
{
    SITH_ASSERTREL(pCog);
    SithThing* pThing = sithCogExec_PopThing(pCog);
    if ( !pThing || pThing->controlType != SITH_CT_AI || !pThing->controlInfo.aiControl.pLocal )
    {
        SITHLOG_ERROR("Cog %s: Bad AI THING in AIWaitForHeadTracking()\n", pCog->aName);
    }
    else if ( (pThing->controlInfo.aiControl.pLocal->submode & SITHAI_SUBMODE_HEADTRACKINGMOTION) == 0 )
    {
        SITHLOG_ERROR("Cog %s: '%s' is not head tracking in AIWaitForHeadTracking().  Execution continuing.\n", pCog->aName, pThing->aName);
    }
    else
    {
        pCog->status = SITHCOG_STATUS_WAITING_AI_HEAD_TRACK;
        pCog->statusParams[0] = pThing->idx;
    }
}

void J3DAPI sithCogFunctionAI_AIGetArmedMode(SithCog* pCog)
{
    SithThing* pThing = sithCogExec_PopThing(pCog);
    if ( !pThing )
    {
        SITHLOG_ERROR("Cog %s: Bad thing in AIGetArmedMode().\n", pCog->aName);
        sithCogExec_PushInt(pCog, -1);
        return;
    }

    if ( !pThing->pPuppetState )
    {
        SITHLOG_ERROR("Cog %s: AIGetArmedMode(): Puppet state not available.\n", pCog->aName);
        sithCogExec_PushInt(pCog, -1);
    }
    else
    {
        sithCogExec_PushInt(pCog, pThing->pPuppetState->armedMode);
    }
}

void J3DAPI sithCogFunctionAI_AIKnockout(SithCog* pCog)
{
    int timerId = sithCogExec_PopInt(pCog);
    float secDuration = sithCogExec_PopFlex(pCog);
    SithThing* pThing = sithCogExec_PopThing(pCog);

    SITH_ASSERTREL(pThing);
    SITH_ASSERTREL(pThing->renderData.pPuppet);
    SITH_ASSERTREL(pThing->controlType == SITH_CT_AI);
    SITH_ASSERTREL(pThing->controlInfo.aiControl.pLocal);

    if ( secDuration <= 0.0f )
    {
        sithPuppet_PlayMode(pThing, SITHPUPPETSUBMODE_ACTIVATEHIGH, sithAIMove_PuppetCallback);
        sithPuppet_StopMode(pThing, SITHPUPPETSUBMODE_ACTIVATE, 0.0f);
        sithPuppet_PlayMode(pThing, SITHPUPPETSUBMODE_DRAWWEAPON, NULL);

        pThing->flags |= SITH_TF_SHADOW;
    }
    else
    {
        sithPuppet_RemoveAllTracks(pThing);

        pThing->thingInfo.actorInfo.flags |= SITH_AF_IMMOBILE | SITH_AF_INVULNERABLE;
        sithPuppet_PlayMode(pThing, SITHPUPPETSUBMODE_ACTIVATE, sithAIMove_PuppetCallback); // activate - knockout

        pThing->thingInfo.actorInfo.bControlsDisabled = 1;
        pThing->moveStatus = SITHPLAYERMOVE_KNOCKEDOUT;

        SithEventParams params;
        params.idx    = pCog->idx;
        params.param3 = 0;
        params.param2 = pThing->idx;
        params.param1 = timerId;
        sithEvent_CreateEvent(SITHCOG_TASKID, &params, (uint32_t)(secDuration * 1000.0f));

        pThing->controlInfo.aiControl.pLocal->mode |= SITHAI_MODE_DISABLED;
        pThing->flags &= ~SITH_TF_SHADOW;
        pThing->collide.type = SITH_COLLIDE_NONE;
    }
}

void J3DAPI sithCogFunctionAI_AIRunOver(SithCog* pCog)
{
    int timerId            = sithCogExec_PopInt(pCog);
    float duration    = sithCogExec_PopFlex(pCog);
    SithThing* pThing = sithCogExec_PopThing(pCog);

    SITH_ASSERTREL(pThing);
    SITH_ASSERTREL(pThing->renderData.pPuppet);
    SITH_ASSERTREL(pThing->controlType == SITH_CT_AI);
    SITH_ASSERTREL(pThing->controlInfo.aiControl.pLocal);
    SITH_ASSERTREL(duration);

    sithPuppet_RemoveAllTracks(pThing);

    pThing->thingInfo.actorInfo.flags |= SITH_AF_IMMOBILE | SITH_AF_INVULNERABLE;
    sithPuppet_PlayMode(pThing, SITHPUPPETSUBMODE_WHIPSWING, sithAIMove_PuppetCallback); // whipswing - runover mode

    pThing->thingInfo.actorInfo.bControlsDisabled = 1;
    pThing->moveStatus = SITHPLAYERMOVE_RUNOVER;

    sithSoundClass_PlayModeRandom(pThing, SITHSOUNDCLASS_DEATH2);

    SithEventParams params;
    params.idx    = pCog->idx;
    params.param1 = timerId;
    params.param2 = pThing->idx;
    params.param3 = 0;
    sithEvent_CreateEvent(SITHCOG_TASKID, &params, (uint32_t)(duration * 1000.0f));

    SithCog* pItemCog = sithInventory_GetType(pThing->thingInfo.actorInfo.weaponInfo.curWeaponID)->pCog;
    if ( pItemCog )
    {
        sithCog_SendMessage(pItemCog, SITHCOG_MSG_DESELECTED, SITHCOG_SYM_REF_NONE, 0, SITHCOG_SYM_REF_THING, pThing->idx, 0);
    }

    pThing->controlInfo.aiControl.pLocal->mode |= SITHAI_MODE_DISABLED;
    pThing->controlInfo.aiControl.pLocal->mode &= ~(SITHAI_MODE_ACTIVE | SITHAI_MODE_UNKNOWN_80 | SITHAI_MODE_TURNING | SITHAI_MODE_MOVING);
}

void J3DAPI sithCogFunctionAI_AISetAllowedSurfaceType(SithCog* pCog)
{
    int surftypes = sithCogExec_PopInt(pCog);
    SithThing* pThing = sithCogExec_PopThing(pCog);
    if ( !pThing || pThing->controlType != SITH_CT_AI || !pThing->controlInfo.aiControl.pLocal )
    {
        SITHLOG_ERROR("Cog %s: Bad thing in AISetAllowedSurfaceType()\n", pCog->aName);
    }
    else
    {
        pThing->controlInfo.aiControl.pLocal->allowedSurfaceTypes |= surftypes;
    }
}

void J3DAPI sithCogFunctionAI_AIClearAllowedSurfaceType(SithCog* pCog)
{
    int surftypes = sithCogExec_PopInt(pCog);
    SithThing* pThing = sithCogExec_PopThing(pCog);
    if ( !pThing || pThing->controlType != SITH_CT_AI || !pThing->controlInfo.aiControl.pLocal )
    {
        SITHLOG_ERROR("Cog %s: Bad thing in AIClearAllowedSurfaceType()\n", pCog->aName);
    }
    else
    {
        pThing->controlInfo.aiControl.pLocal->allowedSurfaceTypes &= ~surftypes;
    }
}

void J3DAPI sithCogFunctionAI_AISetWpnt(SithCog* pCog)
{
    int wpntIdx = sithCogExec_PopInt(pCog);
    SithThing* pThing = sithCogExec_PopThing(pCog);
    if ( !pThing )
    {
        SITHLOG_ERROR("Invalid GHOST THING in AISetWpnt().\n");
        return;
    }

    sithAIUtil_AISetWpnt(pThing, wpntIdx);
}

void J3DAPI sithCogFunctionAI_AISetWpntRank(SithCog* pCog)
{
    int rank    = sithCogExec_PopInt(pCog);
    int wpntNum = sithCogExec_PopInt(pCog);
    sithAIUtil_AISetWpntRank(wpntNum, rank);
}

void J3DAPI sithCogFunctionAI_AISetWpntFlags(SithCog* pCog)
{
    SithAIWaypointLayerFlag flags = sithCogExec_PopInt(pCog);
    int wpntNum = sithCogExec_PopInt(pCog);
    sithAIUtil_AISetWpntFlags(wpntNum, flags);
}

void J3DAPI sithCogFunctionAI_AIClearWpntFlags(SithCog* pCog)
{
    SithAIWaypointLayerFlag flags = sithCogExec_PopInt(pCog);
    int wpntIdx = sithCogExec_PopInt(pCog);
    sithAIUtil_AIClearWpntFlags(wpntIdx, flags);
}

void J3DAPI sithCogFunctionAI_AISetActiveWpntLayer(SithCog* pCog)
{
    int layer = sithCogExec_PopInt(pCog);
    sithAIUtil_AISetActiveWpntLayer(layer);
}

void J3DAPI sithCogFunctionAI_AIConnectWpnts(SithCog* pCog)
{
    int wpntIdx2 = sithCogExec_PopInt(pCog);
    int wpntIdx1 = sithCogExec_PopInt(pCog);
    sithAIUtil_AIConnectWpnts(wpntIdx1, wpntIdx2, /*bConnectWpnt1To2=*/1);
}

void J3DAPI sithCogFunctionAI_AIConnectWpntsOneWay(SithCog* pCog)
{
    int wpntIdx2 = sithCogExec_PopInt(pCog);
    int wpntIdx1 = sithCogExec_PopInt(pCog);
    sithAIUtil_AIConnectWpnts(wpntIdx1, wpntIdx2, /*bConnectWpnt1To2=*/0);
}

void J3DAPI sithCogFunctionAI_AITraverseWpnts(SithCog* pCog)
{
    int mode          = sithCogExec_PopInt(pCog);
    float degTurn     = sithCogExec_PopFlex(pCog);
    float moveSpeed   = sithCogExec_PopFlex(pCog);
    int wpntIdx       = sithCogExec_PopInt(pCog);
    SithThing* pThing = sithCogExec_PopThing(pCog);

    if ( !pThing || pThing->controlType != SITH_CT_AI || !pThing->controlInfo.aiControl.pLocal )
    {
        SITHLOG_ERROR("Invalid AI THING in AITraverseWpnts().\n");
        sithCogExec_PushInt(pCog, -1);
        return;
    }

    sithAIUtil_AITraverseWpnts(pThing->controlInfo.aiControl.pLocal, wpntIdx, moveSpeed, degTurn, mode);
    sithCogExec_PushInt(pCog, 1);
}

void J3DAPI sithCogFunctionAI_AIClearTraverseWpnts(SithCog* pCog)
{
    SithThing* pThing = sithCogExec_PopThing(pCog);
    if ( !pThing || pThing->controlType != SITH_CT_AI || !pThing->controlInfo.aiControl.pLocal )
    {
        SITHLOG_ERROR("Invalid AI THING in AIClearTraverseWpnts().\n");
    }
    else
    {
        sithAIUtil_AIClearMode(pThing->controlInfo.aiControl.pLocal, SITHAI_MODE_TRAVERSEWPNTS);
    }
}

void J3DAPI sithCogFunctionAI_AIFindNearestWpnt(SithCog* pCog)
{
    SithThing* pThing = sithCogExec_PopThing(pCog);
    if ( !pThing
        || pThing->type != SITH_THING_PLAYER
        && (pThing->controlType != SITH_CT_AI || !pThing->controlInfo.aiControl.pLocal) )
    {
        SITHLOG_ERROR("Invalid AI THING in AIFindNearestWpnt().\n");
        sithCogExec_PushInt(pCog, -1);
        return;
    }

    int wpntNum = sithAIUtil_AIFindNearestWpnt(pThing, 1);
    sithCogExec_PushInt(pCog, wpntNum);
}

void J3DAPI sithCogFunctionAI_AIWpntHuntTarget(SithCog* pCog)
{
    float degTurn = sithCogExec_PopFlex(pCog);
    float moveSpeed = sithCogExec_PopFlex(pCog);
    SithThing* pThing = sithCogExec_PopThing(pCog);
    if ( !pThing || pThing->controlType != SITH_CT_AI || !pThing->controlInfo.aiControl.pLocal )
    {
        SITHLOG_ERROR("Invalid AI THING in AIWpntHuntTarget().\n");

        sithCogExec_PushInt(pCog, -1);
    }
    else
    {
        pThing->controlInfo.aiControl.pLocal->pTargetThing = sithPlayer_g_pLocalPlayerThing;
        SITH_ASSERTREL(pThing->controlInfo.aiControl.pLocal->pTargetThing);

        SithAIMode oldmode = pThing->controlInfo.aiControl.pLocal->mode;
        sithAIUtil_AISetMode(pThing->controlInfo.aiControl.pLocal, SITHAI_MODE_HUNTING);

        int wpntIdx = sithAIUtil_AIFindNearestWpnt(pThing, 1);
        if ( wpntIdx <= -1 )
        {
            sithAIUtil_AISetMode(pThing->controlInfo.aiControl.pLocal, SITHAI_MODE_SEARCHING);
            sithCogExec_PushInt(pCog, -1);
            SITHLOG_ERROR("AIWpntHuntTarget: Could not initiate hunt.\n");
        }
        else
        {
            sithCogExec_PushInt(pCog, wpntIdx);
            sithAIUtil_AITraverseWpnts(pThing->controlInfo.aiControl.pLocal, wpntIdx, moveSpeed, degTurn, 0);
        }

        sithAI_EmitEvent(pThing->controlInfo.aiControl.pLocal, SITHAI_EVENT_MODECHANGED, (void*)oldmode);
    }
}

void J3DAPI sithCogFunctionAI_AISetInstinctWpntMode(SithCog* pCog)
{
    SithThing* pThing = sithCogExec_PopThing(pCog);
    if ( !pThing || pThing->controlType != SITH_CT_AI || !pThing->controlInfo.aiControl.pLocal )
    {
        SITHLOG_ERROR("Invalid AI THING in AISetInstinctWpntMode().\n");
    }
    else
    {
        pThing->controlInfo.aiControl.pLocal->mode |= SITHAI_MODE_INSTINCTUSEWPNTS;
    }
}

void J3DAPI sithCogFunctionAI_AIClearInstinctWpntMode(SithCog* pCog)
{
    SithThing* pThing = sithCogExec_PopThing(pCog);
    if ( !pThing || pThing->controlType != SITH_CT_AI || !pThing->controlInfo.aiControl.pLocal )
    {
        SITHLOG_ERROR("Invalid AI THING in AIClearInstinctWpntMode().\n");
    }
    else
    {
        pThing->controlInfo.aiControl.pLocal->mode &= ~SITHAI_MODE_INSTINCTUSEWPNTS;
    }
}

void J3DAPI sithCogFunctionAI_AIFleeToWpnt(SithCog* pCog)
{
    int idx = sithCogExec_PopInt(pCog);
    SithThing* pThing = sithCogExec_PopThing(pCog);
    if ( !pThing
        || pThing->type != SITH_THING_ACTOR
        || pThing->controlType != SITH_CT_AI
        || !pThing->controlInfo.aiControl.pLocal )
    {
        SITHLOG_ERROR("Cog %s: Invalid AI thing in AIFlee.\n", pCog->aName);
        return;
    }

    SithAIMode oldmode = pThing->controlInfo.aiControl.pLocal->mode;
    pThing->controlInfo.aiControl.pLocal->pFleeFromThing = sithAIUtil_AIGetWpntThing(idx);
    sithAIUtil_AISetMode(pThing->controlInfo.aiControl.pLocal, SITHAI_MODE_FLEEINGTOWAYPOINT);

    sithAI_EmitEvent(pThing->controlInfo.aiControl.pLocal, SITHAI_EVENT_MODECHANGED, (void*)oldmode);
}

void J3DAPI sithCogFunctionAI_AISpat(SithCog* pCog)
{
    int timerId       = sithCogExec_PopInt(pCog);
    float duration    = sithCogExec_PopFlex(pCog);
    SithThing* pThing = sithCogExec_PopThing(pCog);

    SITH_ASSERTREL(pThing);
    SITH_ASSERTREL(pThing->renderData.pPuppet);
    SITH_ASSERTREL(pThing->controlType == SITH_CT_AI);
    SITH_ASSERTREL(pThing->controlInfo.aiControl.pLocal);

    if ( duration <= 0.0f )
    {
        pThing->controlInfo.aiControl.pLocal->mode &= ~SITHAI_MODE_DISABLED;
        pThing->thingInfo.actorInfo.bControlsDisabled = 0;
    }
    else
    {
        sithPuppet_RemoveAllTracks(pThing);
        sithPuppet_PlayMode(pThing, SITHPUPPETSUBMODE_RUN, NULL);

        pThing->thingInfo.actorInfo.bControlsDisabled = 1;
        pThing->moveStatus = SITHPLAYERMOVE_RUNNING;

        SithEventParams param;
        param.idx = pCog->idx;
        param.param1 = timerId;
        param.param2 = pThing->idx;
        param.param3 = 0;
        sithEvent_CreateEvent(SITHCOG_TASKID, &param, (uint32_t)(duration * 1000.0f));

        pThing->controlInfo.aiControl.pLocal->mode |= SITHAI_MODE_DISABLED;
    }
}

void sithCogFunctionAI_InstallHooks(void)
{
    J3D_HOOKFUNC(sithCogFunctionAI_RegisterFunctions);
    J3D_HOOKFUNC(sithCogFunctionAI_AISetMoveSpeed);
    J3D_HOOKFUNC(sithCogFunctionAI_AISetMovePos);
    J3D_HOOKFUNC(sithCogFunctionAI_AIGetHomePos);
    J3D_HOOKFUNC(sithCogFunctionAI_AISetMaxHomeDist);
    J3D_HOOKFUNC(sithCogFunctionAI_AIGetGoalThing);
    J3D_HOOKFUNC(sithCogFunctionAI_AIGetGoalLVec);
    J3D_HOOKFUNC(sithCogFunctionAI_AIJump);
    J3D_HOOKFUNC(sithCogFunctionAI_AISetMoveFrame);
    J3D_HOOKFUNC(sithCogFunctionAI_AISetMoveThing);
    J3D_HOOKFUNC(sithCogFunctionAI_AISetGoalThing);
    J3D_HOOKFUNC(sithCogFunctionAI_AIEnableHeadTracking);
    J3D_HOOKFUNC(sithCogFunctionAI_AIDisableHeadTracking);
    J3D_HOOKFUNC(sithCogFunctionAI_AIEnableBodyTracking);
    J3D_HOOKFUNC(sithCogFunctionAI_AIDisableBodyTracking);
    J3D_HOOKFUNC(sithCogFunctionAI_AISetLookPos);
    J3D_HOOKFUNC(sithCogFunctionAI_AISetLookFrame);
    J3D_HOOKFUNC(sithCogFunctionAI_AIGetMovePos);
    J3D_HOOKFUNC(sithCogFunctionAI_AIPauseMove);
    J3D_HOOKFUNC(sithCogFunctionAI_AISetMode);
    J3D_HOOKFUNC(sithCogFunctionAI_AIGetMode);
    J3D_HOOKFUNC(sithCogFunctionAI_AIClearMode);
    J3D_HOOKFUNC(sithCogFunctionAI_AIGetSubMode);
    J3D_HOOKFUNC(sithCogFunctionAI_AISetSubMode);
    J3D_HOOKFUNC(sithCogFunctionAI_AIClearSubMode);
    J3D_HOOKFUNC(sithCogFunctionAI_FirstThingInView);
    J3D_HOOKFUNC(sithCogFunctionAI_NextThingInView);
    J3D_HOOKFUNC(sithCogFunctionAI_ThingViewDot);
    J3D_HOOKFUNC(sithCogFunctionAI_AISetFireTarget);
    J3D_HOOKFUNC(sithCogFunctionAI_AIEnableInstinct);
    J3D_HOOKFUNC(sithCogFunctionAI_AISetCutsceneMode);
    J3D_HOOKFUNC(sithCogFunctionAI_AIClearCutsceneMode);
    J3D_HOOKFUNC(sithCogFunctionAI_IsAITargetInSight);
    J3D_HOOKFUNC(sithCogFunctionAI_AIFlee);
    J3D_HOOKFUNC(sithCogFunctionAI_AIStopFlee);
    J3D_HOOKFUNC(sithCogFunctionAI_AISetClass);
    J3D_HOOKFUNC(sithCogFunctionAI_AISetLookThing);
    J3D_HOOKFUNC(sithCogFunctionAI_AISetLookThingEyeLevel);
    J3D_HOOKFUNC(sithCogFunctionAI_AIWaitForStop);
    J3D_HOOKFUNC(sithCogFunctionAI_AIWaitForHeadTracking);
    J3D_HOOKFUNC(sithCogFunctionAI_AIGetArmedMode);
    J3D_HOOKFUNC(sithCogFunctionAI_AIKnockout);
    J3D_HOOKFUNC(sithCogFunctionAI_AIRunOver);
    J3D_HOOKFUNC(sithCogFunctionAI_AISetAllowedSurfaceType);
    J3D_HOOKFUNC(sithCogFunctionAI_AIClearAllowedSurfaceType);
    J3D_HOOKFUNC(sithCogFunctionAI_AISetWpnt);
    J3D_HOOKFUNC(sithCogFunctionAI_AISetWpntRank);
    J3D_HOOKFUNC(sithCogFunctionAI_AISetWpntFlags);
    J3D_HOOKFUNC(sithCogFunctionAI_AIClearWpntFlags);
    J3D_HOOKFUNC(sithCogFunctionAI_AISetActiveWpntLayer);
    J3D_HOOKFUNC(sithCogFunctionAI_AIConnectWpnts);
    J3D_HOOKFUNC(sithCogFunctionAI_AIConnectWpntsOneWay);
    J3D_HOOKFUNC(sithCogFunctionAI_AITraverseWpnts);
    J3D_HOOKFUNC(sithCogFunctionAI_AIClearTraverseWpnts);
    J3D_HOOKFUNC(sithCogFunctionAI_AIFindNearestWpnt);
    J3D_HOOKFUNC(sithCogFunctionAI_AIWpntHuntTarget);
    J3D_HOOKFUNC(sithCogFunctionAI_AISetInstinctWpntMode);
    J3D_HOOKFUNC(sithCogFunctionAI_AIClearInstinctWpntMode);
    J3D_HOOKFUNC(sithCogFunctionAI_AIFleeToWpnt);
    J3D_HOOKFUNC(sithCogFunctionAI_AISpat);
}

void sithCogFunctionAI_ResetGlobals(void)
{}

int J3DAPI sithCogFunctionAI_RegisterFunctions(SithCogSymbolTable* pTable)
{
    return sithCog_RegisterFunction(pTable, sithCogFunctionAI_AIGetMode, "aigetmode")
        || sithCog_RegisterFunction(pTable, sithCogFunctionAI_AISetMode, "aisetmode")
        || sithCog_RegisterFunction(pTable, sithCogFunctionAI_AIClearMode, "aiclearmode")
        || sithCog_RegisterFunction(pTable, sithCogFunctionAI_AIGetSubMode, "aigetsubmode")
        || sithCog_RegisterFunction(pTable, sithCogFunctionAI_AISetSubMode, "aisetsubmode")
        || sithCog_RegisterFunction(pTable, sithCogFunctionAI_AIClearSubMode, "aiclearsubmode")
        || sithCog_RegisterFunction(pTable, sithCogFunctionAI_AIGetMovePos, "aigetmovepos")
        || sithCog_RegisterFunction(pTable, sithCogFunctionAI_AISetMovePos, "aisetmovepos")
        || sithCog_RegisterFunction(pTable, sithCogFunctionAI_AIGetHomePos, "aigethomepos")
        || sithCog_RegisterFunction(pTable, sithCogFunctionAI_AIPauseMove, "aipausemove")
        || sithCog_RegisterFunction(pTable, sithCogFunctionAI_AISetMaxHomeDist, "aisetmaxhomedist")
        || sithCog_RegisterFunction(pTable, sithCogFunctionAI_AIGetGoalThing, "aigetgoalthing")
        || sithCog_RegisterFunction(pTable, sithCogFunctionAI_AIGetGoalLVec, "aigetgoallvec")
        || sithCog_RegisterFunction(pTable, sithCogFunctionAI_FirstThingInView, "firstthinginview")
        || sithCog_RegisterFunction(pTable, sithCogFunctionAI_NextThingInView, "nextthinginview")
        || sithCog_RegisterFunction(pTable, sithCogFunctionAI_ThingViewDot, "thingviewdot")
        || sithCog_RegisterFunction(pTable, sithCogFunctionAI_AISetFireTarget, "aisetfiretarget")
        || sithCog_RegisterFunction(pTable, sithCogFunctionAI_AISetMoveThing, "aisetmovething")
        || sithCog_RegisterFunction(pTable, sithCogFunctionAI_AISetLookPos, "aisetlookpos")
        || sithCog_RegisterFunction(pTable, sithCogFunctionAI_AISetMoveSpeed, "aisetmovespeed")
        || sithCog_RegisterFunction(pTable, sithCogFunctionAI_AISetLookFrame, "aisetlookframe")
        || sithCog_RegisterFunction(pTable, sithCogFunctionAI_AISetMoveFrame, "aisetmoveframe")
        || sithCog_RegisterFunction(pTable, sithCogFunctionAI_IsAITargetInSight, "isaitargetinsight")
        || sithCog_RegisterFunction(pTable, sithCogFunctionAI_AIFlee, "aiflee")
        || sithCog_RegisterFunction(pTable, sithCogFunctionAI_AIStopFlee, "aistopflee")
        || sithCog_RegisterFunction(pTable, sithCogFunctionAI_AISetClass, "aisetclass")
        || sithCog_RegisterFunction(pTable, sithCogFunctionAI_AIJump, "aijump")
        || sithCog_RegisterFunction(pTable, sithCogFunctionAI_AIEnableInstinct, "aienableinstinct")
        || sithCog_RegisterFunction(pTable, sithCogFunctionAI_AISetLookThing, "aisetlookthing")
        || sithCog_RegisterFunction(pTable, sithCogFunctionAI_AISetLookThingEyeLevel, "aisetlookthingeyelevel")
        || sithCog_RegisterFunction(pTable, sithCogFunctionAI_AIWaitForStop, "aiwaitforstop")
        || sithCog_RegisterFunction(pTable, sithCogFunctionAI_AIWaitForHeadTracking, "aiwaitforheadtracking")
        || sithCog_RegisterFunction(pTable, sithCogFunctionAI_AIGetArmedMode, "aigetarmedmode")
        || sithCog_RegisterFunction(pTable, sithCogFunctionAI_AIKnockout, "aiknockout")
        || sithCog_RegisterFunction(pTable, sithCogFunctionAI_AIRunOver, "airunover")
        || sithCog_RegisterFunction(pTable, sithCogFunctionAI_AISetAllowedSurfaceType, "aisetallowedsurfacetype")
        || sithCog_RegisterFunction(pTable, sithCogFunctionAI_AIClearAllowedSurfaceType, "aiclearallowedsurfacetype")
        || sithCog_RegisterFunction(pTable, sithCogFunctionAI_AISetCutsceneMode, "aisetcutscenemode")
        || sithCog_RegisterFunction(pTable, sithCogFunctionAI_AIClearCutsceneMode, "aiclearcutscenemode")
        || sithCog_RegisterFunction(pTable, sithCogFunctionAI_AISetGoalThing, "aisetgoalthing")
        || sithCog_RegisterFunction(pTable, sithCogFunctionAI_AIEnableHeadTracking, "aienableheadtracking")
        || sithCog_RegisterFunction(pTable, sithCogFunctionAI_AIDisableHeadTracking, "aidisableheadtracking")
        || sithCog_RegisterFunction(pTable, sithCogFunctionAI_AIEnableBodyTracking, "aienablebodytracking")
        || sithCog_RegisterFunction(pTable, sithCogFunctionAI_AIDisableBodyTracking, "aidisablebodytracking")
        || sithCog_RegisterFunction(pTable, sithCogFunctionAI_AISetWpnt, "aisetwpnt")
        || sithCog_RegisterFunction(pTable, sithCogFunctionAI_AISetWpntRank, "aisetwpntrank")
        || sithCog_RegisterFunction(pTable, sithCogFunctionAI_AISetWpntFlags, "aisetwpntflags")
        || sithCog_RegisterFunction(pTable, sithCogFunctionAI_AIClearWpntFlags, "aiclearwpntflags")
        || sithCog_RegisterFunction(pTable, sithCogFunctionAI_AISetActiveWpntLayer, "aisetactivewpntlayer")
        || sithCog_RegisterFunction(pTable, sithCogFunctionAI_AIConnectWpnts, "aiconnectwpnts")
        || sithCog_RegisterFunction(pTable, sithCogFunctionAI_AIConnectWpntsOneWay, "aiconnectwpntsoneway")
        || sithCog_RegisterFunction(pTable, sithCogFunctionAI_AITraverseWpnts, "aitraversewpnts")
        || sithCog_RegisterFunction(pTable, sithCogFunctionAI_AIClearTraverseWpnts, "aicleartraversewpnts")
        || sithCog_RegisterFunction(pTable, sithCogFunctionAI_AIFindNearestWpnt, "aifindnearestwpnt")
        || sithCog_RegisterFunction(pTable, sithCogFunctionAI_AIWpntHuntTarget, "aiwpnthunttarget")
        || sithCog_RegisterFunction(pTable, sithCogFunctionAI_AISetInstinctWpntMode, "aisetinstinctwpntmode")
        || sithCog_RegisterFunction(pTable, sithCogFunctionAI_AIClearInstinctWpntMode, "aiclearinstinctwpntmode")
        || sithCog_RegisterFunction(pTable, sithCogFunctionAI_AISpat, "aispat")
        || sithCog_RegisterFunction(pTable, sithCogFunctionAI_AIFleeToWpnt, "aifleetowpnt");
}

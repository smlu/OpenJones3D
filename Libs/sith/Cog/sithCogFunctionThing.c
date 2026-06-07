#include "sithCogFunctionThing.h"
#include <j3dcore/j3dhook.h>

#include <sith/AI/sithAIMove.h>
#include <sith/AI/sithAIUtil.h>
#include <sith/Cog/sithCog.h>
#include <sith/Cog/sithCogExec.h>
#include <sith/Devices/sithComm.h>
#include <sith/Devices/sithConsole.h>
#include <sith/Dss/sithDSSThing.h>
#include <sith/Engine/sithAnimate.h>
#include <sith/Engine/sithCamera.h>
#include <sith/Engine/sithCollision.h>
#include <sith/Engine/sithPathMove.h>
#include <sith/Engine/sithPhysics.h>
#include <sith/Engine/sithPuppet.h>
#include <sith/Gameplay/sithFX.h>
#include <sith/Gameplay/sithInventory.h>
#include <sith/Gameplay/sithPlayer.h>
#include <sith/Gameplay/sithPlayerActions.h>
#include <sith/Gameplay/sithPlayerControls.h>
#include <sith/Gameplay/sithTime.h>
#include <sith/RTI/symbols.h>
#include <sith/World/sithActor.h>
#include <sith/World/sithItem.h>
#include <sith/World/sithSector.h>
#include <sith/World/sithWorld.h>

#include <rdroid/Engine/rdPuppet.h>
#include <rdroid/Math/rdMatrix.h>
#include <rdroid/Math/rdVector.h>
#include <rdroid/Primitives/rdModel3.h>

#include <std/General/stdMath.h>
#include <std/Win95/stdComm.h>

void J3DAPI sithCogFunctionThing_IsGhostVisible(SithCog* pCog)
{
    float angle            = sithCogExec_PopFlex(pCog);
    SithThing* pGhostThing = sithCogExec_PopThing(pCog);
    SithThing* pThing      = sithCogExec_PopThing(pCog);
    if ( !pGhostThing || !pThing )
    {
        STDLOG_ERROR("Cog %s: Invalid thing ref in IsGhostVisible()!\n", pCog->aName);
        sithCogExec_PushInt(pCog, -1);
        return;
    }

    if ( angle < 1.0 || angle > 90.0 )
    {
        STDLOG_ERROR("Cog %s: Invalid range angle in IsGhostVisible()!\n", pCog->aName);
        sithCogExec_PushInt(pCog, -1);
        return;
    }

    float retSin, retCos;
    stdMath_SinCos(angle, &retSin, &retCos);

    rdVector3 dir;
    rdVector_Sub3(&dir, &pGhostThing->pos, &pThing->pos);
    rdVector_Normalize3Acc(&dir);

    if ( rdVector_Dot3(&pThing->orient.lvec, &dir) < retCos )
    {
        sithCogExec_PushInt(pCog, 0);
    }
    else
    {
        sithCogExec_PushInt(pCog, 1);
    }
}

void J3DAPI sithCogFunctionThing_SetThingAlpha(SithCog* pCog)
{
    float alpha       = sithCogExec_PopFlex(pCog);
    SithThing* pThing = sithCogExec_PopThing(pCog);

    if ( !pThing )
    {
        STDLOG_ERROR("Cog %s: Invalid thing ref in SetThingAlpha()!\n", pCog->aName);
        sithCogExec_PushFlex(pCog, -1.0);
        return;
    }

    alpha = STDMATH_CLAMP(alpha, 0.0f, 1.0f);

    if ( pThing->type == SITH_THING_POLYLINE )
    {
        pThing->renderData.data.pPolyline->face.extraLight.alpha = alpha;
    }

    pThing->alpha = alpha;
    sithCogExec_PushFlex(pCog, alpha);
}

void J3DAPI sithCogFunctionThing_GetThingAlpha(SithCog* pCog)
{
    SithThing* pThing = sithCogExec_PopThing(pCog);
    if ( !pThing )
    {
        STDLOG_ERROR("Cog %s: Invalid thing ref in SetThingAlpha()!\n", pCog->aName);
        sithCogExec_PushFlex(pCog, -1.0);
        return; // Fixed: Added return statement to avoid accessing null pointer
    }

    sithCogExec_PushFlex(pCog, pThing->alpha);
}

void J3DAPI sithCogFunctionThing_CreatePolylineThing(SithCog* pCog)
{
    rdVector3 endPos;
    float duration        = sithCogExec_PopFlex(pCog);
    float tipRadius       = sithCogExec_PopFlex(pCog);
    float baseRadius      = sithCogExec_PopFlex(pCog);
    rdMaterial* pMaterial = sithCogExec_PopMaterial(pCog);
    int bVec              = sithCogExec_PopVector(pCog, &endPos);
    SithThing* pDestThing = sithCogExec_TryPopThing(pCog);
    SithThing* pSrcThing  = sithCogExec_PopThing(pCog);

    if ( !bVec )
    {
        STDLOG_ERROR("Cog %s: Invalid vector in CreatePolyLineThing!\n", pCog->aName);
        sithCogExec_PushInt(pCog, -1);
        return;
    }

    if ( !pSrcThing )
    {
        STDLOG_ERROR("Cog %s: Invalid source thing in CreatePolyLineThing!\n", pCog->aName);
        sithCogExec_PushInt(pCog, -1);
        return;
    }

    SithThing* pPolyline = sithFX_CreatePolylineThing(pSrcThing, pDestThing, &endPos, pMaterial, baseRadius, tipRadius, duration);
    if ( !pPolyline )
    {
        STDLOG_ERROR("Cog %s: Unable to create poly line from thing %s to thing %s CreatePolyLineThing!\n",
            pCog->aName,
            pSrcThing->aName,
            pDestThing ? pDestThing->aName : "<none>" // Fixed: Add pDestThing null check
        );
        sithCogExec_PushInt(pCog, -1);
        return;
    }

    sithCogExec_PushInt(pCog, pPolyline->idx);
    return;
}

void J3DAPI sithCogFunctionThing_GetMoveStatus(SithCog* pCog)
{
    SithThing* pThing = sithCogExec_PopThing(pCog);

    if ( pThing )
    {
        sithCogExec_PushInt(pCog, pThing->moveStatus);
    }
    else
    {
        STDLOG_ERROR("Cog %s: Invalid thing ref if GetMoveStatus()!\n", pCog->aName);
        sithCogExec_PushInt(pCog, -1);
    }
}

void J3DAPI sithCogFunctionThing_CopyOrient(SithCog* pCog)
{
    SithThing* pDestThing = sithCogExec_PopThing(pCog);
    SithThing* pSrcThing  = sithCogExec_PopThing(pCog); // Fixed: Moved popping of the src thing here to ensure execution stack is cleared of this function's arguments

    if ( !pDestThing )
    {
        STDLOG_ERROR("Cog %s: Invalid destination thing in CopyOrient!\n", pCog->aName);
        return;
    }

    if ( !pSrcThing )
    {
        STDLOG_ERROR("Cog %s: Invalid source thing in CopyOrient!\n", pCog->aName);
        return;
    }

    // Now copy orient
    pDestThing->orient = pSrcThing->orient;
}

void J3DAPI sithCogFunctionThing_CopyOrientAndPos(SithCog* pCog)
{
    SithThing* pDstThing = sithCogExec_PopThing(pCog);
    SithThing* pSrcThing = sithCogExec_PopThing(pCog);

    if ( !pDstThing )
    {
        STDLOG_ERROR("Cog %s: Invalid destination thing in CopyOrientAndPos!\n", pCog->aName);
        return;
    }

    if ( !pSrcThing )
    {
        STDLOG_ERROR("Cog %s: Invalid source thing in CopyOrientAndPos!\n", pCog->aName);
        return;
    }

    if ( !pSrcThing->pInSector )
    {
        STDLOG_ERROR("Cog %s: Source thing %s isn't in any sector in CopyOrientAndPos!\n", pCog->aName, pSrcThing->aName); // Fixed: typo in function name
        return;
    }

    pDstThing->orient = pSrcThing->orient;
    pDstThing->pos    = pSrcThing->pos;

    sithThing_ExitSector(pDstThing);
    sithThing_EnterSector(pDstThing, pSrcThing->pInSector, /*bNoWaterSplash=*/1, /*bNoNotify=*/1);
}

void J3DAPI sithCogFunctionThing_CreateLaser(SithCog* pCog)
{
    bool bArgError = false;

    float duration   = sithCogExec_PopFlex(pCog);
    float tipRadius  = sithCogExec_PopFlex(pCog);
    float baseRadius = sithCogExec_PopFlex(pCog);

    rdVector3 endPos;
    if ( !sithCogExec_PopVector(pCog, &endPos) )
    {
        STDLOG_ERROR("Cog %s: Invalid end pos vector in CreateLaser!\n", pCog->aName);
        bArgError = true; // Fixed: Set bArgError to true so we don't proceed with creating laser thing
    }

    rdVector3 offset;
    if ( !sithCogExec_PopVector(pCog, &offset) )
    {
        STDLOG_ERROR("Cog %s: Invalid offset vector in CreateLaser!\n", pCog->aName);
        bArgError = true;
    }

    SithThing* pSrcThing = sithCogExec_PopThing(pCog);
    if ( !pSrcThing )
    {
        STDLOG_ERROR("Cog %s: Invalid source thing in CreateLaser!\n", pCog->aName);
        bArgError = true;
    }

    if ( bArgError )
    {
        sithCogExec_PushInt(pCog, -1);
        return;
    }

    SithThing* pLaser = sithFX_CreateLaserThing(pSrcThing, &offset, &endPos, baseRadius, tipRadius, duration);
    if ( !pLaser )
    {
        STDLOG_ERROR("Cog %s: Unable to create laser from thing %s in CreateLaser!\n", pCog->aName, pSrcThing->aName);
        sithCogExec_PushInt(pCog, -1);
        return;
    }

    // Return laser thing
    sithCogExec_PushInt(pCog, pLaser->idx);
}

void J3DAPI sithCogFunctionThing_CreateLightning(SithCog* pCog)
{
    bool bArgError = false;

    float duration   = sithCogExec_PopFlex(pCog);
    float tipRadius  = sithCogExec_PopFlex(pCog);
    float baseRadius = sithCogExec_PopFlex(pCog);

    rdVector3 endPos;
    if ( !sithCogExec_PopVector(pCog, &endPos) )
    {
        STDLOG_ERROR("Cog %s: Invalid end pos vector in CreateLightning!\n", pCog->aName);
        bArgError = true;
    }

    rdVector3 offset;
    if ( !sithCogExec_PopVector(pCog, &offset) )
    {
        STDLOG_ERROR("Cog %s: Invalid offset vector in CreateLightning!\n", pCog->aName);
        bArgError = true;
    }

    SithThing* pSourceThing = sithCogExec_PopThing(pCog);
    if ( !pSourceThing )
    {
        STDLOG_ERROR("Cog %s: Invalid source thing in CreateLightning!\n", pCog->aName);
        bArgError = true;
    }

    if ( bArgError )
    {
        sithCogExec_PushInt(pCog, -1);
        return;
    }

    SithThing* pLightning = sithFX_CreateLightningThing(pSourceThing, &offset, &endPos, baseRadius, tipRadius, duration);
    if ( !pLightning )
    {
        STDLOG_ERROR("Cog %s: Unable to create lightning from thing %s in CreateLightning!\n", pCog->aName, pSourceThing->aName);
        sithCogExec_PushInt(pCog, -1);
        return;
    }

    // Return lightening thing
    sithCogExec_PushInt(pCog, pLightning->idx);
}

void J3DAPI sithCogFunctionThing_AnimateSpriteSize(SithCog* pCog)
{
    rdVector3 vecEnd;
    rdVector3 vecStart;

    float deltaTime    = sithCogExec_PopFlex(pCog);
    int bVecEnd        = sithCogExec_PopVector(pCog, &vecEnd);
    int bVecStart      = sithCogExec_PopVector(pCog, &vecStart);
    SithThing* pSprite = sithCogExec_PopThing(pCog);

    if ( !bVecStart )
    {
        STDLOG_ERROR("Cog %s: Invalid start vector in AnimateSpriteSize!\n", pCog->aName);
        STDLOG_ERROR("Cog %s: Unable to create animation for AnimateSpriteSize().\n", pCog->aName);
        sithCogExec_PushInt(pCog, -1);
        return;
    }

    if ( !bVecEnd )
    {
        STDLOG_ERROR("Cog %s: Invalid end vector in AnimateSpriteSize!\n", pCog->aName);
        STDLOG_ERROR("Cog %s: Unable to create animation for AnimateSpriteSize().\n", pCog->aName);
        sithCogExec_PushInt(pCog, -1);
        return;
    }

    if ( !pSprite )
    {
        STDLOG_ERROR("Cog %s: Invalid thing in AnimateSpriteSize!\n", pCog->aName);
        STDLOG_ERROR("Cog %s: Unable to create animation for AnimateSpriteSize().\n", pCog->aName);
        sithCogExec_PushInt(pCog, -1);
        return;
    }

    // Altered: Added check for deltaTime to avoid unnecessary animation creation and spare animation slot
    if ( deltaTime > 0.0 )
    {
        SithAnimationSlot* pAnim = sithAnimate_StartAnimateSpriteSize(pSprite, &vecStart, &vecEnd, deltaTime);
        if ( !pAnim )
        {
            STDLOG_ERROR("Cog %s: Unable to create animation for AnimateSpriteSize().\n", pCog->aName);
            sithCogExec_PushInt(pCog, -1);
            return;
        }

        sithCogExec_PushInt(pCog, pAnim->animID);
    }
    else
    {
        // Added: Set sprite size instantly when deltaTime is 0
        pSprite->thingInfo.spriteInfo.width  = vecEnd.x;
        pSprite->thingInfo.spriteInfo.height = vecEnd.y;
        pSprite->thingInfo.spriteInfo.alpha  = vecEnd.z;
        sithCogExec_PushInt(pCog, -1);
    }
}

void J3DAPI sithCogFunctionThing_GetThingType(SithCog* pCog)
{
    SithThing* pThing = sithCogExec_PopThing(pCog);
    if ( pThing )
    {
        sithCogExec_PushInt(pCog, pThing->type);
    }
    else
    {
        sithCogExec_PushInt(pCog, -1);
    }
}

void J3DAPI sithCogFunctionThing_IsThingAutoAiming(SithCog* pCog)
{
    J3D_UNUSED(pCog);
    // TODO: redundant function
}

void J3DAPI sithCogFunctionThing_PlayForceMoveMode(SithCog* pCog)
{
    SithPuppetSubMode submode = sithCogExec_PopInt(pCog);
    SithThing* pThing         = sithCogExec_PopThing(pCog);

    if ( !pThing )
    {
        STDLOG_ERROR("Cog %s: Bad thing in PlayForceMoveMode()\n", pCog->aName); // Fixed: Updated format to show cog name
        sithCogExec_PushInt(pCog, -1);
        return;
    }

    // Set initial forcemove position
    pThing->forceMoveStartPos = pThing->pos;

    int trackNum      = -1; // Fixed: Init to -1 for player action moves
    bool bAnimPlaying = false;

    switch ( submode )
    {
        case SITHPUPPETSUBMODE_HOPBACK:
        {
            sithPlayerActions_HopBack(pThing);
            bAnimPlaying = true;
            break;
        }
        case SITHPUPPETSUBMODE_HOPLEFT:
        {
            sithPlayerActions_HopLeft(pThing);
            bAnimPlaying = true;
            break;
        }
        case SITHPUPPETSUBMODE_HOPRIGHT:
        {
            sithPlayerActions_HopRight(pThing);
            bAnimPlaying = true;
            break;
        }
        case SITHPUPPETSUBMODE_LEAP:
        {
            pThing->moveStatus        = SITHPLAYERMOVE_JUMPFWD;
            pThing->forceMoveStartPos = pThing->pos; // TODO: forceMoveStartPos already set, why set it here again

            sithThing_DetachThing(pThing);

            trackNum = sithPuppet_PlayForceMoveMode(pThing, SITHPUPPETSUBMODE_LEAP, /*pCallback=*/NULL);// TODO: set new var for submode to tracknum
            bAnimPlaying = true;
            break;
        }
        case SITHPUPPETSUBMODE_JUMPUP:
        {
            sithPlayerActions_Jump(pThing, 0.6f, 0);
            bAnimPlaying = true;
            break;
        }
        case SITHPUPPETSUBMODE_JUMPFWD:
        {
            sithPlayerActions_Jump(pThing, 0.77f, 1);
            bAnimPlaying = true;
            break;
        }
        case SITHPUPPETSUBMODE_PUSHPULLREADY:
        case SITHPUPPETSUBMODE_WHIPSWING:
        {
            if ( pThing->type == SITH_THING_PLAYER )
            {
                sithCogExec_PushInt(pCog, -1);
                return;
            }

            pThing->forceMoveStartPos = pThing->pos; // TODO: forceMoveStartPos already set, why set it here again
            break;
        }
        case SITHPUPPETSUBMODE_PUSHITEM:
        {
            pThing->moveStatus = SITHPLAYERMOVE_PUSHING;
            break;
        }
        case SITHPUPPETSUBMODE_PULLITEM:
        {
            pThing->moveStatus = SITHPLAYERMOVE_PULLING;
            break;
        }
        case SITHPUPPETSUBMODE_HANGSHIMLEFT:
        case SITHPUPPETSUBMODE_HANGSHIMRIGHT:
        {
            pThing->moveStatus = SITHPLAYERMOVE_HANGING;
            break;
        }
        case SITHPUPPETSUBMODE_CLIMBWALLUP:
        case SITHPUPPETSUBMODE_CLIMBWALLDOWN:
        case SITHPUPPETSUBMODE_CLIMBWALLLEFT:
        case SITHPUPPETSUBMODE_CLIMBWALLRIGHT:
        {
            pThing->moveStatus = SITHPLAYERMOVE_CLIMBIDLE;
            break;
        }
        case SITHPUPPETSUBMODE_CLIMBPULLINGUP:
        case SITHPUPPETSUBMODE_MOUNTFROMWATER:
        {
            pThing->moveStatus = SITHPLAYERMOVE_PULLINGUP;
            break;
        }
        case SITHPUPPETSUBMODE_DIVEFROMSURFACE:
        {
            pThing->moveStatus = SITHPLAYERMOVE_SWIMIDLE;
            break;
        }
        case SITHPUPPETSUBMODE_MOUNT1MSTEP:
        {
            sithPlayerActions_ClimbOn1m(pThing);
            bAnimPlaying = true;
            break;
        }
        case SITHPUPPETSUBMODE_MOUNT2MLEDGE:
        {
            sithPlayerActions_ClimbOn2m(pThing);
            bAnimPlaying = true;
            break;
        }
        case SITHPUPPETSUBMODE_JUMPROLLBACK:
        {
            sithPlayerActions_JumpRollBack(pThing);
            bAnimPlaying = true;
            break;
        }
        case SITHPUPPETSUBMODE_JUMPROLLFWD:
        {
            sithPlayerActions_JumpRollForward(pThing);
            bAnimPlaying = true;
            break;
        }
        default:
        {
            sithCogExec_PushInt(pCog, -1);
            return;
        }
    }

    if ( !bAnimPlaying )
    {
        trackNum = sithPuppet_PlayForceMoveMode(pThing, submode, /*pCallback*/NULL);
    }

    sithCogExec_PushInt(pCog, trackNum);
}

void J3DAPI sithCogFunctionThing_SetMoveMode(SithCog* pCog)
{
    SithPuppetMoveMode newMode = sithCogExec_PopInt(pCog);
    SithThing* pThing          = sithCogExec_PopThing(pCog);

    // TODO: function returns -1 on error and 0 on success. Should maybe rather return 1?

    if ( !pThing )
    {
        STDLOG_ERROR("Cog %s: Bad thing in SetMoveMode()\n", pCog->aName); // Fixed: Updated format to show cog name
        sithCogExec_PushInt(pCog, -1);
        return;
    }

    if ( (size_t)newMode >= SITH_PUPPET_NUMMOVEMODES ) //107 )     // ?? shouldnt be less than SITH_PUPPET_NUMMOVEMODES which is 3 => newMode > -1 && newMode <  SITH_PUPPET_NUMMOVEMODES
    {
        STDLOG_ERROR("Cog %s: Bad move mode for thing %s in SetMoveMode()\n", pCog->aName, pThing->aName); // Fixed: Updated format to show cog name
        sithCogExec_PushInt(pCog, -1);
        return;
    }

    sithPuppet_SetMoveMode(pThing, (SithPuppetMoveMode)newMode);
    sithCogExec_PushInt(pCog, 0);
    return;
}

void J3DAPI sithCogFunctionThing_CreateThing(SithCog* pCog)
{
    SithThing* pMarker   = sithCogExec_PopThing(pCog);
    SithThing* pTemplate = sithCogExec_PopTemplate(pCog);

    if ( !pMarker )
    {
        STDLOG_ERROR("Cog %s: Bad thing reference in CreateThing.\n", pCog->aName);
        sithCogExec_PushInt(pCog, -1);
        return;
    }

    if ( pMarker->type == SITH_THING_FREE )
    {
        STDLOG_ERROR("Cog %s: Thing %s has been deleted from the world in CreateThing.\n", pCog->aName, pMarker->aName);
        sithCogExec_PushInt(pCog, -1);
        return;
    }

    if ( !pMarker->pInSector )
    {
        STDLOG_ERROR("Cog %s: Thing %s is not in any sector in CreateThing.\n", pCog->aName, pMarker->aName);
        sithCogExec_PushInt(pCog, -1);
        return;
    }

    if ( !pTemplate )
    {
        STDLOG_ERROR("Cog %s: Bad template reference in CreateThing.\n", pCog->aName);
        sithCogExec_PushInt(pCog, -1);
        return;
    }

    SithThing* pNewThing = sithThing_CreateThing(pTemplate, pMarker);
    if ( !pNewThing )
    {
        STDLOG_ERROR("Cog %s: CreateThing failed.\n", pCog->aName);
        sithCogExec_PushInt(pCog, -1);
        return;
    }

    if ( (pCog->flags & SITHCOG_NOSYNC) == 0 )
    {
        if ( pCog->execMsgType != SITHCOG_MSG_STARTUP && pCog->execMsgType != SITHCOG_MSG_SHUTDOWN )
        {
            sithDSSThing_CreateThing(pTemplate, pNewThing, pMarker, /*pNewSector=*/NULL, /*pos=*/NULL, /*pyr=*/NULL, 0xFFu, DPSEND_GUARANTEED); // make encrypted send
        }
    }

    pNewThing->moveStatus = SITHPLAYERMOVE_STILL;
    sithCogExec_PushInt(pCog, pNewThing->idx);
}

void J3DAPI sithCogFunctionThing_CreateThingAtPos(SithCog* pCog)
{
    rdVector3 pos, pyr;
    int bPyrVec          = sithCogExec_PopVector(pCog, &pyr);
    int bPosVec          = sithCogExec_PopVector(pCog, &pos);
    SithSector* pSector  = sithCogExec_PopSector(pCog);
    SithThing* pTemplate = sithCogExec_PopTemplate(pCog);

    if ( !bPyrVec || !bPosVec )
    {
        STDLOG_ERROR("Cog %s: Bad vector in CreateThingAtPos.\n", pCog->aName);
        sithCogExec_PushInt(pCog, -1);
        return;
    }

    if ( !pTemplate )
    {
        STDLOG_ERROR("Cog %s: Bad template in CreateThingAtPos.\n", pCog->aName);
        sithCogExec_PushInt(pCog, -1);
        return;
    }

    if ( !pSector )
    {
        STDLOG_ERROR("Cog %s: Bad sector in CreateThingAtPos.\n", pCog->aName);
        sithCogExec_PushInt(pCog, -1);
        return;
    }

    rdVector3 offset = { 0 };
    switch ( pTemplate->renderData.type )
    {
        case RD_THING_MODEL3:
            offset = pTemplate->renderData.data.pModel3->insertOffset;
            break;
        case RD_THING_SPRITE3:
            offset = pTemplate->renderData.data.pSprite3->offset;
            break;
    }

    rdMatrix34 orient;
    rdMatrix_BuildRotate34(&orient, &pyr);

    rdMatrix_TransformVector34Acc(&offset, &orient);
    rdVector_Add3Acc(&pos, &offset);

    SithThing* pNewThing = sithThing_CreateThingAtPos(pTemplate, &pos, &orient, pSector, /*pParent=*/NULL);
    if ( !pNewThing )
    {
        STDLOG_ERROR("Cog %s: Unable to create thing in CreateThingAtPos.\n", pCog->aName);
        sithCogExec_PushInt(pCog, -1);
        return;
    }

    if ( (pCog->flags & SITHCOG_NOSYNC) == 0 )
    {
        if ( pCog->execMsgType != SITHCOG_MSG_STARTUP && pCog->execMsgType != SITHCOG_MSG_SHUTDOWN )
        {
            sithDSSThing_CreateThing(pTemplate, pNewThing, /*pMarker=*/NULL, pSector, &pos, &pyr, 0xFFu, DPSEND_GUARANTEED);
        }
    }

    sithCogExec_PushInt(pCog, pNewThing->idx);
}

void J3DAPI sithCogFunctionThing_DamageThing(SithCog* pCog)
{
    SithThing* pDamageThing    = sithCogExec_PopThing(pCog);
    SithDamageType damageclass = sithCogExec_PopInt(pCog);
    float damage               = sithCogExec_PopFlex(pCog);
    SithThing* pThing          = sithCogExec_PopThing(pCog);

    if ( damage <= 0.0 || !pThing )
    {
        sithCogExec_PushInt(pCog, -1);
        return;
    }

    if ( !pDamageThing )
    {
        pDamageThing = pThing;
    }

    if ( (pCog->flags & SITHCOG_NOSYNC) == 0 )
    {
        if ( pCog->execMsgType != SITHCOG_MSG_STARTUP && pCog->execMsgType != SITHCOG_MSG_SHUTDOWN )
        {
            if ( stdComm_IsGameHost() )
            {
                sithDSSThing_DamageThing(pThing, pDamageThing, damage, damageclass, SITHMESSAGE_SENDTOJOINEDPLAYERS, SITHMESSAGE_STREAM_NET);
            }
        }
    }

    float newDamage = sithThing_DamageThing(pThing, pDamageThing, damage, damageclass);
    sithCogExec_PushFlex(pCog, newDamage);
}

void J3DAPI sithCogFunctionThing_HealThing(SithCog* pCog)
{
    float health      = sithCogExec_PopFlex(pCog);
    SithThing* pThing = sithCogExec_PopThing(pCog);

    if ( health > 0.0 )
    {
        if ( pThing )
        {
            if ( pThing->type == SITH_THING_ACTOR || pThing->type == SITH_THING_PLAYER )
            {
                pThing->thingInfo.actorInfo.health += health;
                if ( pThing->thingInfo.actorInfo.health > pThing->thingInfo.actorInfo.maxHealth )
                {
                    pThing->thingInfo.actorInfo.health = pThing->thingInfo.actorInfo.maxHealth;
                }
            }
        }
    }
}

void J3DAPI sithCogFunctionThing_GetHealth(SithCog* pCog)
{
    SithThing* pThing = sithCogExec_PopThing(pCog);

    if ( !pThing )
    {
        STDLOG_ERROR("Cog %s: Bad thing in GetHealth.\n", pCog->aName);
        sithCogExec_PushFlex(pCog, -1.0);
        return;
    }

    if ( pThing->type != SITH_THING_ACTOR && pThing->type != SITH_THING_PLAYER )
    {
        STDLOG_ERROR("Cog %s: Thing %s is not an actor or player in GetHealth.\n", pCog->aName, pThing->aName);
        sithCogExec_PushFlex(pCog, -1.0);
        return;
    }

    sithCogExec_PushFlex(pCog, pThing->thingInfo.actorInfo.health);
    return;
}

void J3DAPI sithCogFunctionThing_GetMaxHealth(SithCog* pCog)
{
    SithThing* pThing = sithCogExec_PopThing(pCog);

    if ( !pThing )
    {
        STDLOG_ERROR("Cog %s: Bad thing in GetMaxHealth.\n", pCog->aName);
        sithCogExec_PushFlex(pCog, -1.0);
        return;
    }

    if ( pThing->type != SITH_THING_ACTOR && pThing->type != SITH_THING_PLAYER )
    {
        STDLOG_ERROR("Cog %s: Thing %s is not an actor or player in GetMaxHealth.\n", pCog->aName, pThing->aName);
        sithCogExec_PushFlex(pCog, -1.0);
        return;
    }

    sithCogExec_PushFlex(pCog, pThing->thingInfo.actorInfo.maxHealth);
    return;
}

void J3DAPI sithCogFunctionThing_SetHealth(SithCog* pCog)
{
    float health      = sithCogExec_PopFlex(pCog);
    SithThing* pThing = sithCogExec_PopThing(pCog);

    if ( pThing )
    {
        if ( pThing->type == SITH_THING_ACTOR || pThing->type == SITH_THING_PLAYER )
        {
            pThing->thingInfo.actorInfo.health = health;
        }
    }
}

void J3DAPI sithCogFunctionThing_DestroyThing(SithCog* pCog)
{
    SithThing* pThing = sithCogExec_PopThing(pCog);

    if ( pThing )
    {
        if ( (pCog->flags & SITHCOG_NOSYNC) == 0 )
        {
            if ( pCog->execMsgType != SITHCOG_MSG_STARTUP && pCog->execMsgType != SITHCOG_MSG_SHUTDOWN )
            {
                sithDSSThing_DestroyThing(pThing->guid, SITHMESSAGE_SENDTOJOINEDPLAYERS);
            }
        }

        sithThing_DestroyThing(pThing);
    }
}

void J3DAPI sithCogFunctionThing_JumpToFrame(SithCog* pCog)
{
    SithSector* pNewSector = sithCogExec_PopSector(pCog);
    size_t frame           = sithCogExec_PopInt(pCog);
    SithThing* pThing      = sithCogExec_PopThing(pCog);

    if ( !pThing || !pNewSector || pThing->moveType != SITH_MT_PATH || frame >= pThing->moveInfo.pathMovement.numFrames )
    {
        STDLOG_ERROR("Cog %s: Illegal params in JumpToFrame.\n", pCog->aName);
        return;
    }

    if ( pThing->pInSector && pNewSector != pThing->pInSector )
    {
        sithThing_ExitSector(pThing);
    }

    if ( pThing->attach.flags )
    {
        sithThing_DetachThing(pThing);
    }

    rdMatrix_BuildRotate34(&pThing->orient, &pThing->moveInfo.pathMovement.aFrames[frame].pyr);
    pThing->pos = pThing->moveInfo.pathMovement.aFrames[frame].pos;

    if ( !pThing->pInSector )
    {
        sithThing_EnterSector(pThing, pNewSector, /*bNoWaterSplash=*/1, /*bNoNotify=*/0);
    }
}

void J3DAPI sithCogFunctionThing_MoveToFrame(SithCog* pCog)
{
    float speed       = sithCogExec_PopFlex(pCog) * 0.1f;
    size_t frame      = sithCogExec_PopInt(pCog);
    SithThing* pThing = sithCogExec_PopThing(pCog);

    if ( !pThing )
    {
        STDLOG_ERROR("Cog %s: Thing pointer is NULL in MoveToFrame\n", pCog->aName);
        return;
    }

    if ( pThing->moveType != SITH_MT_PATH )
    {
        STDLOG_ERROR("Cog %s: Thing %s is not a path move thing in MoveToFrame\n", pCog->aName, pThing->aName);
        return;
    }

    if ( frame >= pThing->moveInfo.pathMovement.numFrames )
    {
        STDLOG_ERROR("Cog %s: Thing %s, moveFrame (%d) > path's frames (%d)\n", pCog->aName, pThing->aName, frame, pThing->moveInfo.pathMovement.numFrames);
        return;
    }

    if ( speed == 0.0f )
    {
        STDLOG_ERROR("Cog %s: Bad movement speed set for movetoframe.  Set=0.5\n", pCog->aName);
        speed = 0.5;
    }

    sithPathMove_MoveToFrame(pThing, frame, speed);

    if ( (pCog->flags & SITHCOG_NOSYNC) == 0 )
    {
        if ( pCog->execMsgType != SITHCOG_MSG_STARTUP && pCog->execMsgType != SITHCOG_MSG_SHUTDOWN )
        {
            sithDSSThing_PathMove(pThing, frame, speed, 0, SITHMESSAGE_SENDTOJOINEDPLAYERS, SITHMESSAGE_STREAM_ALL);
        }
    }
}

void J3DAPI sithCogFunctionThing_SkipToFrame(SithCog* pCog)
{
    float speed       = sithCogExec_PopFlex(pCog) * 0.1f;
    size_t frame      = sithCogExec_PopInt(pCog);
    SithThing* pThing = sithCogExec_PopThing(pCog);

    if ( !pThing || pThing->moveType != SITH_MT_PATH || frame >= pThing->moveInfo.pathMovement.numFrames )
    {
        STDLOG_ERROR("Cog %s: Illegal params in MoveToFrame call from COG.\n", pCog->aName);
        return;
    }

    if ( speed == 0.0 )
    {
        STDLOG_ERROR("Cog %s: Bad movement speed set for movetoframe.  Set=0.5\n", pCog->aName);
        speed = 0.5;
    }

    sithPathMove_SkipToFrame(pThing, frame, speed);

    if ( (pCog->flags & SITHCOG_NOSYNC) == 0 )
    {
        if ( pCog->execMsgType != SITHCOG_MSG_STARTUP && pCog->execMsgType != SITHCOG_MSG_SHUTDOWN )
        {
            sithDSSThing_PathMove(pThing, frame, speed, 1, SITHMESSAGE_SENDTOJOINEDPLAYERS, SITHMESSAGE_STREAM_ALL);
        }
    }
}

void J3DAPI sithCogFunctionThing_RotatePivot(SithCog* pCog)
{
    float rotTime     = sithCogExec_PopFlex(pCog);
    size_t frame      = sithCogExec_PopInt(pCog);
    SithThing* pThing = sithCogExec_PopThing(pCog);

    if ( rotTime == 0.0 )
    {
        rotTime = 1.0;
    }

    if ( !pThing || pThing->moveType != SITH_MT_PATH || frame >= pThing->moveInfo.pathMovement.numFrames )

    {
        STDLOG_ERROR("Cog %s: Illegal parameters in Rotate call from COG.\n", pCog->aName);
        return;
    }

    SithPathFrame* pFrame = &pThing->moveInfo.pathMovement.aFrames[frame];
    if ( rotTime <= 0.0 )
    {
        rdVector3 rot;
        rdVector_Neg3(&rot, &pFrame->pyr);
        sithPathMove_RotatePivot(pThing, &pFrame->pos, &rot, -rotTime);
    }
    else
    {
        sithPathMove_RotatePivot(pThing, &pFrame->pos, &pFrame->pyr, rotTime);
    }
}

void J3DAPI sithCogFunctionThing_Rotate(SithCog* pCog)
{
    float rotTime     = sithCogExec_PopFlex(pCog);
    size_t axis       = sithCogExec_PopInt(pCog);
    float degrees     = sithCogExec_PopFlex(pCog);
    SithThing* pThing = sithCogExec_PopThing(pCog);

    if ( !pThing )
    {
        STDLOG_ERROR("Cog %s: Invalid thing in Rotate.\n", pCog->aName);
        return;
    }

    if ( pThing->moveType != SITH_MT_PATH )
    {
        STDLOG_ERROR("Cog %s: Thing %s is not a pathmove object in Rotate.\n", pCog->aName, pThing->aName);
        return;
    }

    if ( axis >= 3 )
    {
        STDLOG_ERROR("Cog %s: Thing %s invalid rotation axis %d in Rotate.\n", pCog->aName, pThing->aName, axis);
        return;
    }

    if ( degrees == 0.0f )
    {
        STDLOG_ERROR("NOTE: Cog %s: Thing %s being rotated 0 degrees in Rotate.\n", pCog->aName, pThing->aName);
        return;
    }

    if ( rotTime == 0.0f )
    {
        STDLOG_ERROR("Cog %s: Thing %s given rotation time of 0 in Rotate.\n", pCog->aName, pThing->aName);
        return;
    }

    rdVector3 offset = { 0 };

    if ( pThing->renderData.data.pModel3 )
    {
        offset =  pThing->renderData.data.pModel3->insertOffset;
    }

    rdMatrix_TransformVector34Acc(&offset, &pThing->orient);
    rdVector_Sub3(&offset, &pThing->pos, &offset);

    rdVector3 pyrRot = { 0 };
    switch ( axis )
    {
        case 0:
            pyrRot.pitch = -degrees;
            break;
        case 1:
            pyrRot.yaw = -degrees;
            break;
        case 2:
            pyrRot.roll = -degrees;
            break;
    }

    sithPathMove_RotatePivot(pThing, &offset, &pyrRot, rotTime);;
}

void J3DAPI sithCogFunctionThing_RotateToPYR(SithCog* pCog)
{
    rdVector3 pyr;
    float time        = sithCogExec_PopFlex(pCog);
    int bPyr          = sithCogExec_PopVector(pCog, &pyr);
    SithThing* pThing = sithCogExec_PopThing(pCog);

    if ( !pThing )
    {
        STDLOG_ERROR("Cog %s: Invalid thing in RotateToPYR.\n", pCog->aName);
        return;
    }

    if ( pThing->moveType != SITH_MT_PATH )
    {
        STDLOG_ERROR("Cog %s: Thing %s is not a pathmove object in RotateToPYR.\n", pCog->aName, pThing->aName);
        return;
    }

    if ( !bPyr )
    {
        STDLOG_ERROR("Cog %s: Invalid vector applied to thing %s in RotateToPYR call from COG.\n", pCog->aName, pThing->aName);
        return;
    }

    if ( time <= 0.0 )
    {
        STDLOG_ERROR("Cog %s: Time must be greater than zero for thing %s in RotateToPYR call from COG.\n", pCog->aName, pThing->aName);
        return;
    }

    sithPathMove_RotateToPYR(pThing, &pyr, time);
}

void J3DAPI sithCogFunctionThing_GetThingLight(SithCog* pCog)
{
    SithThing* pThing = sithCogExec_PopThing(pCog);

    rdVector3 color = { .red=-1.0f, .green=-1.0f, .blue=-1.0f };
    if ( pThing )
    {
        color = *(rdVector3*)&pThing->light.color;
    }
    else
    {
        STDLOG_ERROR("Cog %s: Illegal call to GetThingLight().\n", pCog->aName);
    }

    sithCogExec_PushVector(pCog, &color);
}

void J3DAPI sithCogFunctionThing_SetExtCamOffset(SithCog* pCog)
{
    rdVector3 vec;
    sithCogExec_PopVector(pCog, &vec);

    sithCamera_SetExtCameraOffset(&vec);
}

void J3DAPI sithCogFunctionThing_SetExtCamOffsetToThing(SithCog* pCog)
{
    SithThing* pThing = sithCogExec_PopThing(pCog);
    if ( !pThing )
    {
        STDLOG_ERROR("Cog %s: Bad THING in SetExtCamOffsetToThing() call.\n", pCog->aName);
        return;
    }

    rdVector3 offset;
    rdVector_Sub3(&offset, &pThing->pos, &sithPlayer_g_pLocalPlayerThing->pos);

    rdVector3 pyr;
    rdMatrix_ExtractAngles34(&sithPlayer_g_pLocalPlayerThing->orient, &pyr);

    pyr.pitch = 0.0;
    pyr.yaw   = -pyr.y;
    pyr.roll  = 0.0;
    rdVector_Rotate3Acc(&offset, &pyr);

    sithCamera_SetExtCameraOffset(&offset);
}

void J3DAPI sithCogFunctionThing_SetExtCamLookOffset(SithCog* pCog)
{
    rdVector3 vec;
    sithCogExec_PopVector(pCog, &vec);

    sithCamera_SetExtCameraLookOffset(&vec);
}

void J3DAPI sithCogFunctionThing_SetExtCamLookOffsetToThing(SithCog* pCog)
{
    SithThing* pThing = sithCogExec_PopThing(pCog);

    if ( !pThing )
    {
        STDLOG_ERROR("Cog %s: Bad THING in SetExtCamLookOffsetToThing() call.\n", pCog->aName);
        return;
    }

    rdVector3 offset;
    rdVector_Sub3(&offset, &pThing->pos, &sithPlayer_g_pLocalPlayerThing->pos);

    rdVector3 pyr;
    rdMatrix_ExtractAngles34(&sithPlayer_g_pLocalPlayerThing->orient, &pyr);

    pyr.pitch = 0.0;
    pyr.yaw   = -pyr.y;
    pyr.roll  = 0.0;
    rdVector_Rotate3Acc(&offset, &pyr);

    sithCamera_SetExtCameraLookOffset(&offset);
}

void J3DAPI sithCogFunctionThing_MakeCamera2LikeCamera1(SithCog* pCog)
{
    SithThing* pCam1Look = sithCogExec_PopThing(pCog);
    SithThing* pCam1Pos  = sithCogExec_PopThing(pCog);

    if ( !sithPlayer_g_pLocalPlayerThing || !sithPlayer_g_pLocalPlayerThing->pInSector )
    {
        STDLOG_ERROR("Cog %s: no local player or player not in valid sector!\n", pCog->aName);
        sithCogExec_PushInt(pCog, 0);
        return;
    }

    if ( !pCam1Look || !pCam1Pos )
    {
        STDLOG_ERROR("Cog &s: Bad THING in MakeCamera2LikeCamera1() call.\n");
        sithCogExec_PushInt(pCog, 0);
        return;
    }

    rdVector3 pos  = sithCamera_g_camSpot;
    rdVector3 look = sithCamera_g_camLookSpot;

    SithSector* pPosSector = sithCollision_FindSectorInRadius(sithPlayer_g_pLocalPlayerThing->pInSector, &sithPlayer_g_pLocalPlayerThing->pos, &pos, 0.0);
    if ( !pPosSector )
    {
        STDLOG_ERROR("Cog %s: pos thing %s endpoint not in valid sector in MakeCamera2LikeCamera1() call.\n", pCog->aName); // Fixed: Fixed typo "Cog &s"

        sithCogExec_PushInt(pCog, 0);
        return;
    }

    SithSector* pLookSector = sithCollision_FindSectorInRadius(sithPlayer_g_pLocalPlayerThing->pInSector, &sithPlayer_g_pLocalPlayerThing->pos, &look, 0.0);
    if ( !pLookSector )
    {
        STDLOG_ERROR("Cog %s: look thing %s endpoint not in valid sector in MakeCamera2LikeCamera1() call.\n", pCog->aName); // Fixed: Fixed typo "Cog &s"
        sithCogExec_PushInt(pCog, 0);
        return;
    }

    pCam1Pos->pos  = pos;
    pCam1Look->pos = look;

    if ( pPosSector != pCam1Pos->pInSector )
    {
        if ( pCam1Pos->pInSector )
        {
            sithThing_ExitSector(pCam1Pos);
        }

        sithThing_EnterSector(pCam1Pos, pPosSector, /*bNoWaterSplash=*/1, /*bNoNotify=*/1);
    }

    if ( pLookSector != pCam1Look->pInSector )
    {
        if ( pCam1Look->pInSector )
        {
            sithThing_ExitSector(pCam1Look);
        }

        sithThing_EnterSector(pCam1Look, pLookSector, /*bNoWaterSplash=*/1, /*bNoNotify=*/1);
    }

    sithCogExec_PushInt(pCog, 1);
}

void J3DAPI sithCogFunctionThing_RestoreExtCam(SithCog* pCog)
{
    J3D_UNUSED(pCog);
    sithCamera_RestoreExtCamera();
}

void J3DAPI sithCogFunctionThing_SetCameraPosition(SithCog* pCog)
{
    rdVector3 vec;
    int bVec      = sithCogExec_PopVector(pCog, &vec);
    size_t camNum = sithCogExec_PopInt(pCog);

    if ( bVec && camNum < STD_ARRAYLEN(sithCamera_g_aCameras) ) // Fixed: Add check for bVec
    {
        sithCamera_SetCameraPosition(&sithCamera_g_aCameras[camNum], &vec);
    }
}

void J3DAPI sithCogFunctionThing_SetCameraLookInterp(SithCog* pCog)
{
    int bEnable   = sithCogExec_PopInt(pCog); // enable pan & tilt to lock on 2nd target
    size_t camNum = sithCogExec_PopInt(pCog);

    if ( camNum >= STD_ARRAYLEN(sithCamera_g_aCameras) )
    {
        STDLOG_ERROR("Cog %s: Invalid camera in SetCameraLookInterp().\n", pCog->aName);
        return;
    }

    SithCamera* pCamera = &sithCamera_g_aCameras[camNum];
    if ( bEnable != pCamera->bLookInterp )
    {
        pCamera->lookInterpState = -1;
    }

    pCamera->bLookInterp = bEnable;
}

// Function enables/disables dolly mode for specified camera.
//
// Dolly mode: when camera primary focus is changed the camera will interpolate move to the new focused thing instead of teleport to it.
void J3DAPI sithCogFunctionThing_SetCameraPosInterp(SithCog* pCog)
{
    int bDollyMode = sithCogExec_PopInt(pCog);
    size_t camNum = sithCogExec_PopInt(pCog);

    if ( camNum >= STD_ARRAYLEN(sithCamera_g_aCameras) )
    {
        STDLOG_ERROR("Cog %s: Invalid camera in SetCameraPosInterp().\n", pCog->aName);
        return;
    }

    SithCamera* pCamera = &sithCamera_g_aCameras[camNum];
    if ( bDollyMode != pCamera->bPosInterp )
    {
        pCamera->posInterpState = -1;
    }

    pCamera->bPosInterp = bDollyMode;
}

void J3DAPI sithCogFunctionThing_GetCameraFOV(SithCog* pCog)
{
    sithCogExec_PushFlex(pCog, sithCamera_g_pCurCamera->fov);
}

void J3DAPI sithCogFunctionThing_GetCameraPosition(SithCog* pCog)
{
    size_t camNum = sithCogExec_PopInt(pCog);

    rdVector3 pos = { 0 };
    if ( camNum >= STD_ARRAYLEN(sithCamera_g_aCameras) )
    {
        STDLOG_ERROR("Cog %s: Invalid camera number %d in GetCameraPosition().\n", pCog->aName, camNum);
    }
    else
    {
        sithCamera_GetCameraPosition(&sithCamera_g_aCameras[camNum], &pos);
    }

    sithCogExec_PushVector(pCog, &pos);
}

void J3DAPI sithCogFunctionThing_SetCameraInterpSpeed(SithCog* pCog)
{
    float speed   = sithCogExec_PopFlex(pCog);
    size_t camNum = sithCogExec_PopInt(pCog);

    if ( camNum >= STD_ARRAYLEN(sithCamera_g_aCameras) )
    {
        STDLOG_ERROR("Cog %s: Invalid camera in SetCameraInterpSpeed().\n", pCog->aName);
        return;
    }

    sithCamera_g_aCameras[camNum].interpSpeed = speed;
}

void J3DAPI sithCogFunctionThing_SetCameraFOV(SithCog* pCog)
{
    float timeDelta = sithCogExec_PopFlex(pCog);
    int bInterp     = sithCogExec_PopInt(pCog);
    float fov       = sithCogExec_PopFlex(pCog);

    if ( bInterp )
    {
        if ( timeDelta > 0.0 )
        {
            sithAnimate_CameraZoom(sithCamera_g_pCurCamera, fov, timeDelta);
            return;
        }

        STDLOG_ERROR("Cog %s: Invalid interp time in SetCameraFOV().\n", pCog->aName);
    }

    sithCamera_SetCameraFOV(sithCamera_g_pCurCamera, fov);
}

void J3DAPI sithCogFunctionThing_ResetCameraFOV(SithCog* pCog)
{
    float time  = sithCogExec_PopFlex(pCog);
    int bInterp = sithCogExec_PopInt(pCog);

    if ( bInterp )
    {
        if ( time > 0.0 )
        {
            sithAnimate_CameraZoom(sithCamera_g_pCurCamera, SITHCAMERA_FOVDEFAULT, time);
            return;
        }

        STDLOG_ERROR("Cog %s: Invalid interp time in ResetCameraFOV().\n", pCog->aName);
    }

    sithCamera_SetCameraFOV(sithCamera_g_pCurCamera, SITHCAMERA_FOVDEFAULT);
}

void J3DAPI sithCogFunctionThing_SetCameraFadeThing(SithCog* pCog)
{
    int bBackside                   = sithCogExec_PopInt(pCog);   // Flip fadeplate to backside (i.e: frontside black & backside white color)
    SithThing* pPrimaryFocusThing   = sithCogExec_PopThing(pCog);
    SithThing* pSecondaryFocusThing = sithCogExec_PopThing(pCog);
    size_t camNum                   = sithCogExec_PopInt(pCog);

    if ( !pSecondaryFocusThing )
    {
        STDLOG_ERROR("Cog %s: Bad fade thing in SetCameraFadeThing() call.\n", pCog->aName);
        return;
    }

    if ( camNum >= STD_ARRAYLEN(sithCamera_g_aCameras) )
    {
        STDLOG_ERROR("Cog %s: Bad camera number in SetCameraFadeThing() call.\n", pCog->aName);
        return;
    }

    SithCamera* pCamera = &sithCamera_g_aCameras[camNum];
    if ( pCamera->bPosInterp )
    {
        pCamera->posInterpState = -1;
    }
    pCamera->bPosInterp = 0;

    if ( pCamera->bLookInterp )
    {
        pCamera->lookInterpState = -1;
    }
    pCamera->bLookInterp = 0;

    sithCamera_SetCameraPosition(pCamera, &sithCamera_g_aCameras[SITHCAMERA_EXTCAMERANUM].lookPos); // EXT camera
    sithCamera_SetCameraFOV(pCamera, sithCamera_g_aCameras[SITHCAMERA_EXTCAMERANUM].fov);

    pCamera->orient = sithCamera_g_aCameras[SITHCAMERA_EXTCAMERANUM].orient;

    pSecondaryFocusThing->orient = pCamera->orient;
    rdVector_ScaleAdd3(&pSecondaryFocusThing->pos, &pSecondaryFocusThing->orient.lvec, 0.02f, &sithCamera_g_aCameras[SITHCAMERA_EXTCAMERANUM].lookPos);

    SithSector* pSector = sithCollision_FindSectorInRadius(sithCamera_g_aCameras[SITHCAMERA_EXTCAMERANUM].pSector, &sithCamera_g_aCameras[SITHCAMERA_EXTCAMERANUM].lookPos, &pSecondaryFocusThing->pos, 0.0);
    sithThing_SetSector(pSecondaryFocusThing, pSector, /*bNotify=*/0);

    pPrimaryFocusThing->orient = pSecondaryFocusThing->orient;
    pPrimaryFocusThing->pos    = sithCamera_g_aCameras[SITHCAMERA_EXTCAMERANUM].lookPos;

    sithThing_SetSector(pPrimaryFocusThing, sithCamera_g_aCameras[SITHCAMERA_EXTCAMERANUM].pSector, 0);
    if ( !bBackside )
    {
        rdVector3 pyr = { 0 };
        pyr.yaw = 180.0;
        rdMatrix_PreRotate34(&pSecondaryFocusThing->orient, &pyr);
    }

    sithCamera_SetCameraFocus(pCamera, pPrimaryFocusThing, pSecondaryFocusThing);
}

void J3DAPI sithCogFunctionThing_ThingLight(SithCog* pCog)
{
    rdVector4 color;
    float litupTime   = sithCogExec_PopFlex(pCog);
    float range       = sithCogExec_PopFlex(pCog);
    int bVec          = sithCogExec_PopVector(pCog, (rdVector3*)&color);
    SithThing* pThing = sithCogExec_PopThing(pCog);

    if ( !bVec || !pThing )
    {
        STDLOG_ERROR("Cog %s: Illegal call to ThingLight().\n", pCog->aName);
        return;
    }

    color.alpha = range;
    if ( litupTime == 0.0 )
    {
        pThing->light.color     = color; // Fixed: Assigned range to alpha, fixes light flickering bug when camera is near light object.
        pThing->light.minRadius = range;
        pThing->light.maxRadius = range * 1.1f; // TODO: It would be good to make it dynamic, consider making new cog function for this param.
                                                // Note, same way new max light range is calculated in sithAnimate,
                                                // probably there should be a utility function to calculate new range.

        if ( color.red > 0.0 || color.green > 0.0 || color.blue > 0.0 )
        {
            pThing->flags |= SITH_TF_EMITLIGHT;
        }
    }
    else
    {
        sithAnimate_StartThingLightAnim(pThing, &color, litupTime, (SithAnimateFlags)0);
    }
}

void J3DAPI sithCogFunctionThing_ThingLightAnim(SithCog* pCog)
{
    rdVector4 startColor, endColor;
    float speed       = sithCogExec_PopFlex(pCog);
    float endRadius   = sithCogExec_PopFlex(pCog);
    int bEndColor     = sithCogExec_PopVector(pCog, (rdVector3*)&endColor);
    float startRadius = sithCogExec_PopFlex(pCog);
    int bColor        = sithCogExec_PopVector(pCog, (rdVector3*)&startColor);
    SithThing* pThing = sithCogExec_PopThing(pCog);

    if ( !pThing || !bEndColor || !bColor || speed <= 0.0 )
    {
        STDLOG_ERROR("Cog %s: Bad parameters to ThingLightAnim.\n", pCog->aName);
        sithCogExec_PushInt(pCog, -1);
        return;
    }

    startColor.alpha = startRadius;
    endColor.alpha   = endRadius;

    pThing->light.color       = startColor;
    pThing->light.minRadius   = startRadius;
    pThing->light.maxRadius   = startRadius * 1.1f; // Note, same way new max light range is calculated in sithAnimate.

    SithAnimationSlot* pAnim = sithAnimate_StartThingLightAnim(pThing, &endColor, speed / 2, SITHANIMATE_LOOP); // speed / 2 -> half time in start - end interpol and half time in end - start interpol
    if ( pAnim )
    {
        sithCogExec_PushInt(pCog, pAnim->animID);
        return;
    }
}

void J3DAPI sithCogFunctionThing_ThingFadeAnim(SithCog* pCog)
{
    rdVector4 startColor;
    rdVector_Set4(&startColor, 1.0f, 1.0f, 1.0f, 1.0f);

    rdVector4 endColor;
    rdVector_Set4(&endColor, 1.0f, 1.0f, 1.0f, 1.0f);

    int bLoop         = sithCogExec_PopInt(pCog);
    float timeDelta   = sithCogExec_PopFlex(pCog);
    float endAlpha    = sithCogExec_PopFlex(pCog);
    float startAlpha  = sithCogExec_PopFlex(pCog);
    SithThing* pThing = sithCogExec_PopThing(pCog);

    if ( !pThing || timeDelta <= 0.0 )
    {
        STDLOG_ERROR("Cog %s: Bad parameters to sithCogFunctionThing_FadeThingAnim.\n", pCog->aName);
        goto error;
    }

    if ( pThing->type == SITH_THING_POLYLINE )
    {
        startColor = pThing->renderData.data.pPolyline->face.extraLight;
        startColor.alpha = startAlpha;

        endColor = pThing->renderData.data.pPolyline->face.extraLight;
    }
    else
    {
        startColor.alpha = startAlpha;
        rdModel3_SetThingColor(&pThing->renderData, &startColor);
    }

    endColor.alpha = endAlpha;

    SithAnimationSlot* pAnim;
    if ( bLoop )
    {
        pAnim = sithAnimate_StartThingFadeAnim(pThing, &startColor, &endColor, timeDelta * 0.5f, SITHANIMATE_LOOP);
    }
    else
    {
        pAnim = sithAnimate_StartThingFadeAnim(pThing, &startColor, &endColor, timeDelta, (SithAnimateFlags)0);
    }

    if ( pAnim )
    {
        sithCogExec_PushInt(pCog, pAnim->animID);
        return;
    }

error:
    STDLOG_ERROR("Cog %s: Unable to create animation for sithCogFunctionThing_FadeThingAnim().\n", pCog->aName);
    sithCogExec_PushInt(pCog, -1);
}

void J3DAPI sithCogFunctionThing_WaitForStop(SithCog* pCog)
{
    SithThing* pThing = sithCogExec_PopThing(pCog);
    if ( !pThing )
    {
        STDLOG_ERROR("Cog %s: Illegal thingref in WaitForStop call.\n", pCog->aName);
        return;
    }

    if ( pThing->moveType != SITH_MT_PATH )
    {
        STDLOG_ERROR("Cog %s: thing %s is not a path move thing in WaitForStop call.\n", pCog->aName, pThing->aName);
        return;
    }

    if ( (pThing->moveInfo.pathMovement.mode & (SITH_PATHMOVE_MOVE | SITH_PATHMOVE_ROTATE)) != 0 )
    {
        pCog->status          = SITHCOG_STATUS_WAITING_THING_TO_STOP;
        pCog->statusParams[0] = pThing->idx;

        if ( (pCog->flags & SITHCOG_DEBUG) != 0 )
        {
            SITHCONSOLE_PRINTF("Cog %s: Waiting for stop on object %d.\n", pCog->aName, pThing->idx);
        }
    }
}

void J3DAPI sithCogFunctionThing_WaitForAnimStop(SithCog* pCog)
{
    int animID = sithCogExec_PopInt(pCog);
    if ( !sithAnimate_GetAnim(animID) )
    {
        STDLOG_ERROR("Cog %s: Anim ID %d is not a valid ID in WaitForAnimStop call.\n", pCog->aName, animID);
        return;
    }

    pCog->status          = SITHCOG_STATUS_WAITING_ANIMATION_TO_STOP;
    pCog->statusParams[0] = animID;

    if ( (pCog->flags & SITHCOG_DEBUG) != 0 )
    {
        SITHCONSOLE_PRINTF("Cog %s: Waiting for anim %d to stop.\n", pCog->aName, animID);
    }
}

void J3DAPI sithCogFunctionThing_GetThingSector(SithCog* pCog)
{
    SITH_ASSERTREL(pCog);

    SithThing* pThing = sithCogExec_PopThing(pCog);
    if ( !pThing || !pThing->pInSector )
    {
        STDLOG_ERROR("Cog %s: Bad thing ref in GetThingSector().\n", pCog->aName);
        sithCogExec_PushInt(pCog, -1);
        return;
    }

    int sectorIdx = sithSector_GetSectorIndex(pThing->pInSector);
    sithCogExec_PushInt(pCog, sectorIdx);
}

void J3DAPI sithCogFunctionThing_GetCurFrame(SithCog* pCog)
{
    SITH_ASSERTREL(pCog);

    SithThing* pThing = sithCogExec_PopThing(pCog);
    if ( !pThing )
    {
        STDLOG_ERROR("Cog %s: Bad thingRef in GetCurFrame.\n", pCog->aName);
        sithCogExec_PushInt(pCog, 0); // TODO: should -1 be returned?
        return;
    }

    if ( pThing->moveType != SITH_MT_PATH )
    {
        STDLOG_ERROR("Cog %s: Thing %s is not a path move type thing.\n", pCog->aName, pThing->aName);
        sithCogExec_PushInt(pCog, 0); // TODO: should -1 be returned?
        return;
    }

    sithCogExec_PushInt(pCog, pThing->moveInfo.pathMovement.currentFrame);
    return;
}

void J3DAPI sithCogFunctionThing_GetGoalFrame(SithCog* pCog)
{
    SITH_ASSERTREL(pCog);

    SithThing* pThing = sithCogExec_PopThing(pCog);
    if ( !pThing || pThing->moveType != SITH_MT_PATH )
    {
        STDLOG_ERROR("Cog %s: Bad thingRef in GetGoalFrame.\n",
            pCog->aName);
        sithCogExec_PushInt(pCog, 0);
        return;
    }

    sithCogExec_PushInt(pCog, pThing->moveInfo.pathMovement.goalFrame);
}

void J3DAPI sithCogFunctionThing_StopThing(SithCog* pCog)
{
    SithThing* pThing = sithCogExec_PopThing(pCog);
    if ( !pThing )
    {
        // TODO: Maybe add error log?
        return;
    }

    switch ( pThing->moveType )
    {
        case SITH_MT_PHYSICS:
        {
            sithPhysics_ResetThingMovement(pThing);
            if ( pThing->controlType == SITH_CT_AI )
            {
                SithAIControlBlock* pLocal = pThing->controlInfo.aiControl.pLocal;
                if ( pLocal )
                {
                    sithAIMove_AIStop(pLocal);
                }
            }
        } break;
        case SITH_MT_PATH:
        {
            sithPathMove_Finish(pThing);
            if ( (pCog->flags & SITHCOG_NOSYNC) == 0 )
            {
                if ( pCog->execMsgType != SITHCOG_MSG_STARTUP && pCog->execMsgType != SITHCOG_MSG_SHUTDOWN )
                {
                    sithDSSThing_PathMove(pThing, 0, 0.0, 2, SITHMESSAGE_SENDTOJOINEDPLAYERS, SITHMESSAGE_STREAM_ALL); // 2 - finish moving
                }
            }
        } break;
    };
}

void J3DAPI sithCogFunctionThing_IsThingMoving(SithCog* pCog)
{
    SITH_ASSERTREL(pCog);

    SithThing* pThing = sithCogExec_PopThing(pCog);
    if ( !pThing || pThing->type == SITH_THING_FREE )
    {
        STDLOG_ERROR("Cog %s: Bad parameters in IsMoving()\n", pCog->aName);
        sithCogExec_PushInt(pCog, 0);
        return;
    }

    SithAIControlBlock* pLocal;
    if ( pThing->controlType == SITH_CT_AI && (pLocal = pThing->controlInfo.aiControl.pLocal) != NULL )
    {
        sithCogExec_PushInt(pCog, pLocal->mode & (SITHAI_MODE_TURNING | SITHAI_MODE_MOVING));
    }
    else
    {
        if ( pThing->moveType == SITH_MT_PHYSICS )
        {
            int moveState = !rdVector_IsZero3(&pThing->moveInfo.physics.velocity);
            int rotState  = !rdVector_IsZero3(&pThing->moveInfo.physics.angularVelocity);
            sithCogExec_PushInt(pCog, moveState || rotState);
        }
        else if ( pThing->moveType == SITH_MT_PATH )
        {
            sithCogExec_PushInt(pCog, pThing->moveInfo.pathMovement.mode & (SITH_PATHMOVE_ROTATE | SITH_PATHMOVE_MOVE));
        }
        else
        {
            STDLOG_ERROR("Cog %s: Unknown movement type for thingRef, assuming not moving.\n", pCog->aName);
            sithCogExec_PushInt(pCog, 0);
        }
    }
}

void J3DAPI sithCogFunctionThing_SetThingPulse(SithCog* pCog)
{
    float secPulse    = sithCogExec_PopFlex(pCog);
    SithThing* pThing = sithCogExec_PopThing(pCog);

    if ( !pThing )
    {
        STDLOG_ERROR("Cog %s: Illegal parameters to SetThingPulse.\n", pCog->aName);
        return;
    }

    if ( secPulse == 0.0 )
    {
        // Stop pulse
        pThing->flags &= ~SITH_TF_PULSESET;
        pThing->msecNextPulseTime = 0;
        pThing->msecPulseInterval = 0;
    }
    else
    {
        // Start pulse
        pThing->flags            |= SITH_TF_PULSESET;
        pThing->msecPulseInterval = (uint32_t)(secPulse * 1000.0);
        pThing->msecNextPulseTime = pThing->msecPulseInterval + sithTime_g_msecGameTime;
    }
}

void J3DAPI sithCogFunctionThing_SetThingTimer(SithCog* pCog)
{
    float secTimer    = sithCogExec_PopFlex(pCog);
    SithThing* pThing = sithCogExec_PopThing(pCog);

    if ( !pThing )
    {
        STDLOG_ERROR("Cog %s: Illegal parameters to SetThingTimer.\n", pCog->aName);
        return;
    }

    if ( secTimer == 0.0 )
    {
        // Stop timer
        pThing->msecTimerTime = 0;
        pThing->flags &= ~SITH_TF_TIMERSET;
    }
    else
    {
        // Start timer
        pThing->flags |= SITH_TF_TIMERSET;
        pThing->msecTimerTime = sithTime_g_msecGameTime + (uint32_t)(secTimer * 1000.0);
    }
}

void J3DAPI sithCogFunctionThing_CaptureThing(SithCog* pCog)
{
    SithThing* pThing = sithCogExec_PopThing(pCog);

    if ( pThing )
    {
        pThing->flags      |= SITH_TF_COGLINKED;
        pThing->pCaptureCog = pCog;
    }
}

void J3DAPI sithCogFunctionThing_ReleaseThing(SithCog* pCog)
{
    SithThing* pThing = sithCogExec_PopThing(pCog);

    if ( pThing )
    {
        pThing->pCaptureCog = NULL;
        if ( !pThing->pCog && !sithCog_IsThingLinked(pThing) )
        {
            pThing->flags &= ~SITH_TF_COGLINKED;
        }
    }
}

void J3DAPI sithCogFunctionThing_GetThingParent(SithCog* pCog)
{
    SithThing* pThing = sithCogExec_PopThing(pCog);

    SithThing* pParent;
    if ( pThing && (pParent = sithThing_GetThingParent(pThing)) != NULL )
    {
        sithCogExec_PushInt(pCog, pParent->idx);
    }
    else
    {
        sithCogExec_PushInt(pCog, -1);
    }
}

void J3DAPI sithCogFunctionThing_GetThingPos(SithCog* pCog)
{
    SithThing* pThing = sithCogExec_PopThing(pCog);

    if ( pThing )
    {
        sithCogExec_PushVector(pCog, &pThing->pos);
    }
    else
    {
        sithCogExec_PushVector(pCog, &rdroid_g_zeroVector3);
    }
}

void J3DAPI sithCogFunctionThing_SetThingPos(SithCog* pCog)
{
    rdVector3 newPos;
    int bVect         = sithCogExec_PopVector(pCog, &newPos);
    SithThing* pThing = sithCogExec_PopThing(pCog);

    if ( !pThing || !bVect ) // Fixed: Added check for bVec
    {
        STDLOG_ERROR("Cog %s: Bad parameters in SetThingPos()\n", pCog->aName); // Added
        sithCogExec_PushInt(pCog, 0);
        return;
    }

    pThing->pos = newPos;

    if ( (pCog->flags & SITHCOG_NOSYNC) == 0 )
    {
        if ( pCog->execMsgType != SITHCOG_MSG_STARTUP && pCog->execMsgType != SITHCOG_MSG_SHUTDOWN )
        {
            sithDSSThing_Pos(pThing, SITHMESSAGE_SENDTOJOINEDPLAYERS, DPSEND_GUARANTEED);
        }
    }

    sithCogExec_PushInt(pCog, 1);
}

void J3DAPI sithCogFunctionThing_GetInventory(SithCog* pCog)
{
    size_t typeId     = sithCogExec_PopInt(pCog);
    SithThing* pThing = sithCogExec_PopThing(pCog);

    if ( !pThing || pThing->type != SITH_THING_PLAYER || !pThing->thingInfo.actorInfo.pPlayer )
    {
        STDLOG_ERROR("Cog %s: Called GetInventory for illegal thing.\n", pCog->aName);
        sithCogExec_PushFlex(pCog, 0.0f);
        return;
    }

    if ( typeId >= SITHINVENTORY_MAXTYPES )
    {
        STDLOG_ERROR("Cog %s: Called GetInventory for illegal type %d.\n", pCog->aName, typeId);
        sithCogExec_PushFlex(pCog, 0.0f);
        return;
    }

    float val = sithInventory_GetInventory(pThing, typeId);
    sithCogExec_PushFlex(pCog, val);
    return;
}

void J3DAPI sithCogFunctionThing_SetInventory(SithCog* pCog)
{
    float amount      = sithCogExec_PopFlex(pCog);
    size_t typeId     = sithCogExec_PopInt(pCog);
    SithThing* pThing = sithCogExec_PopThing(pCog);

    if ( !pThing || pThing->type != SITH_THING_PLAYER || !pThing->thingInfo.actorInfo.pPlayer )
    {
        STDLOG_ERROR("Cog %s: Called SetInventory for illegal thing.\n", pCog->aName);
        return;
    }

    if ( typeId > SITHINVENTORY_MAXTYPES )
    {
        STDLOG_ERROR("Cog %s: Called SetInventory for illegal type %d.\n", pCog->aName, typeId);
        return;
    }

    sithInventory_SetInventory(pThing, typeId, amount);
}

void J3DAPI sithCogFunctionThing_ChangeInventory(SithCog* pCog)
{
    float amount      = sithCogExec_PopFlex(pCog);
    size_t typeId     = sithCogExec_PopInt(pCog);
    SithThing* pThing = sithCogExec_PopThing(pCog);

    if ( !pThing || pThing->type != SITH_THING_PLAYER || !pThing->thingInfo.actorInfo.pPlayer )
    {
        STDLOG_ERROR("Cog %s: Called ChangeInventory for illegal thing.\n", pCog->aName);
        sithCogExec_PushFlex(pCog, 0.0f);
        return;
    }

    if ( typeId >= SITHINVENTORY_MAXTYPES )
    {
        STDLOG_ERROR("Cog %s: Called ChangeInventory for illegal type %d.\n", pCog->aName, typeId);
        sithCogExec_PushFlex(pCog, 0.0f);
        return;
    }

    float newAmount = sithInventory_ChangeInventory(pThing, typeId, amount);
    sithCogExec_PushFlex(pCog, newAmount);
}

void J3DAPI sithCogFunctionThing_GetInventoryCog(SithCog* pCog)
{
    size_t typeId     = sithCogExec_PopInt(pCog);
    SithThing* pThing = sithCogExec_PopThing(pCog);

    if ( !pThing || pThing->type != SITH_THING_PLAYER || !pThing->thingInfo.actorInfo.pPlayer )
    {
        STDLOG_ERROR("Cog %s: Called GetInventoryCog for illegal thing.\n", pCog->aName);
        sithCogExec_PushInt(pCog, -1);
        return;
    }

    // Fixed: Added typeId check before call to sithInventory_GetInventoryType which can assert on invalid typeId
    if ( typeId >= SITHINVENTORY_MAXTYPES )
    {
        STDLOG_ERROR("Cog %s: Called GetInventoryCog for illegal type %d.\n", pCog->aName, typeId);
        sithCogExec_PushInt(pCog, -1);
        return;
    }

    SithInventoryType* pInventory = sithInventory_GetInventoryType(pThing, typeId);
    if ( !pInventory )
    {
        STDLOG_ERROR("Cog %s: Called GetInventoryCog for illegal type %d.\n", pCog->aName, typeId);
        sithCogExec_PushInt(pCog, -1);
        return;
    }

    if ( pInventory->pCog )
    {
        sithCogExec_PushInt(pCog, pInventory->pCog->idx);
        return;
    }
}

void J3DAPI sithCogFunctionThing_GetThingVelocity(SithCog* pCog)
{
    SithThing* pThing = sithCogExec_PopThing(pCog);

    if ( !pThing )
    {
        STDLOG_ERROR("Cog %s: Bad thing reference in GetThingVelocity()\n", pCog->aName); // Added
        sithCogExec_PushVector(pCog, &rdroid_g_zeroVector3);
        return;
    }

    rdVector3 vec = rdroid_g_zeroVector3;
    if ( pThing->moveType == SITH_MT_PHYSICS )
    {
        vec = pThing->moveInfo.physics.velocity;
    }
    else if ( pThing->moveType == SITH_MT_PATH )
    {
        rdVector_Scale3(&vec, &pThing->moveInfo.pathMovement.vecDeltaPos, pThing->moveInfo.pathMovement.moveVel);
    }

    sithCogExec_PushVector(pCog, &vec);
}

void J3DAPI sithCogFunctionThing_SetThingVel(SithCog* pCog)
{
    rdVector3 vel;
    int bVec          = sithCogExec_PopVector(pCog, &vel);
    SithThing* pThing = sithCogExec_PopThing(pCog);

    if ( !pThing || !bVec ) // Fixed: Added bVec check
    {
        STDLOG_ERROR("Cog %s: Bad parameters in SetThingVel()\n", pCog->aName); // Added
        return;
    };

    if ( pThing->moveType != SITH_MT_PHYSICS )
    {
        STDLOG_ERROR("Cog %s: Cannot set velocity on a movepath thing.", pCog->aName);
        return;
    }

    pThing->moveInfo.physics.velocity = vel;

    if ( (pCog->flags & SITHCOG_NOSYNC) == 0 )
    {
        if ( pCog->execMsgType != SITHCOG_MSG_STARTUP && pCog->execMsgType != SITHCOG_MSG_SHUTDOWN )
        {
            sithThing_SyncThing(pThing, SITHTHING_SYNC_POS);
        }
    }
}

void J3DAPI sithCogFunctionThing_ApplyForce(SithCog* pCog)
{
    rdVector3 force;
    int bVec          = sithCogExec_PopVector(pCog, &force);
    SithThing* pThing = sithCogExec_PopThing(pCog);

    if ( pThing && bVec ) // Fixed: Added check for bVec
    {
        if ( pThing->moveType == SITH_MT_PHYSICS )
        {
            sithPhysics_ApplyForce(pThing, &force);

            if ( (pCog->flags & SITHCOG_NOSYNC) == 0 )
            {
                if ( pCog->execMsgType != SITHCOG_MSG_STARTUP && pCog->execMsgType != SITHCOG_MSG_SHUTDOWN )
                {
                    sithThing_SyncThing(pThing, SITHTHING_SYNC_POS);
                }
            }
        }
    }
}

void J3DAPI sithCogFunctionThing_AddThingVel(SithCog* pCog)
{
    rdVector3 vel;
    int bVec         = sithCogExec_PopVector(pCog, &vel);
    SithThing* pThing = sithCogExec_PopThing(pCog);

    if ( !pThing || !bVec ) // Added: check for bVec
    {
        STDLOG_ERROR("Cog %s: Bad parameters in AddThingVel()\n", pCog->aName); // Added
        return;
    }

    if ( pThing->moveType != SITH_MT_PHYSICS )
    {
        STDLOG_ERROR("Cog %s: Cannot add velocity on a non-physics thing.", pCog->aName);
        return;
    }

    rdVector_Add3Acc(&pThing->moveInfo.physics.velocity, &vel);

    if ( (pCog->flags & SITHCOG_NOSYNC) == 0 )
    {
        if ( pCog->execMsgType != SITHCOG_MSG_STARTUP && pCog->execMsgType != SITHCOG_MSG_SHUTDOWN )
        {
            sithThing_SyncThing(pThing, SITHTHING_SYNC_POS);
        }
    }
}

void J3DAPI sithCogFunctionThing_GetThingLVec(SithCog* pCog)
{
    SithThing* pThing = sithCogExec_PopThing(pCog);

    if ( pThing )
    {
        sithCogExec_PushVector(pCog, &pThing->orient.lvec);
    }
    else
    {
        STDLOG_ERROR("Cog %s: Bad thing reference in GetThingLVec()\n", pCog->aName); // Added
        sithCogExec_PushVector(pCog, &rdroid_g_zeroVector3);
    }
}

void J3DAPI sithCogFunctionThing_GetThingUVec(SithCog* pCog)
{
    SithThing* pThing = sithCogExec_PopThing(pCog);

    if ( pThing )
    {
        sithCogExec_PushVector(pCog, &pThing->orient.uvec);
    }
    else
    {
        STDLOG_ERROR("Cog %s: Bad thing reference in GetThingUVec()\n", pCog->aName); // Added
        sithCogExec_PushVector(pCog, &rdroid_g_zeroVector3);
    }
}

void J3DAPI sithCogFunctionThing_GetThingRVec(SithCog* pCog)
{
    SithThing* pThing = sithCogExec_PopThing(pCog);

    if ( pThing )
    {
        sithCogExec_PushVector(pCog, &pThing->orient.rvec);
    }
    else
    {
        STDLOG_ERROR("Cog %s: Bad parathing referencemeters in GetThingRVec()\n", pCog->aName); // Added
        sithCogExec_PushVector(pCog, &rdroid_g_zeroVector3);
    }
}

void J3DAPI sithCogFunctionThing_GetActorHeadPYR(SithCog* pCog)
{
    SithThing* pThing = sithCogExec_PopThing(pCog);

    if ( pThing && (pThing->type == SITH_THING_ACTOR || pThing->type == SITH_THING_PLAYER) )
    {
        sithCogExec_PushVector(pCog, &pThing->thingInfo.actorInfo.headPYR);
    }
    else
    {
        STDLOG_ERROR("Cog %s: Bad thing reference in GetActorHeadPYR()\n", pCog->aName); // Added
        sithCogExec_PushVector(pCog, &rdroid_g_zeroVector3);
    }
}

void J3DAPI sithCogFunctionThing_DetachThing(SithCog* pCog)
{
    SithThing* pThing = sithCogExec_PopThing(pCog);

    if ( pThing )
    {
        sithThing_DetachThing(pThing);

        if ( (pCog->flags & SITHCOG_NOSYNC) == 0 )
        {
            if ( pCog->execMsgType != SITHCOG_MSG_STARTUP && pCog->execMsgType != SITHCOG_MSG_SHUTDOWN )
            {
                sithDSSThing_Attachment(pThing, SITHMESSAGE_SENDTOJOINEDPLAYERS, SITHMESSAGE_STREAM_ALL, DPSEND_GUARANTEED);
            }
        }
    }
}

void J3DAPI sithCogFunctionThing_GetThingAttachFlags(SithCog* pCog)
{
    SithThing* pThing = sithCogExec_PopThing(pCog);

    if ( pThing )
    {
        sithCogExec_PushInt(pCog, pThing->attach.flags);
    }
    else
    {
        STDLOG_ERROR("Cog %s: Bad thing reference in GetThingAttachFlags()\n", pCog->aName); // Added
        sithCogExec_PushInt(pCog, -1);
    }
}

void J3DAPI sithCogFunctionThing_AttachThingToSurf(SithCog* pCog)
{
    SithSurface* pSurf = sithCogExec_PopSurface(pCog);
    SithThing* pThing  = sithCogExec_PopThing(pCog);

    if ( !pThing )
    {
        STDLOG_ERROR("Cog %s: Bad thing in AttachThingToSurf().\n", pCog->aName);
        return;
    }

    if ( !pSurf )
    {
        STDLOG_ERROR("Cog %s: Bad sector in AttachThingToSurf().\n", pCog->aName);
        return;
    }

    sithThing_AttachThingToSurface(pThing, pSurf, /*bNoImpactUpdate=*/1);

    if ( (pCog->flags & SITHCOG_NOSYNC) == 0 )
    {
        if ( pCog->execMsgType != SITHCOG_MSG_STARTUP && pCog->execMsgType != SITHCOG_MSG_SHUTDOWN )
        {
            sithDSSThing_Attachment(pThing, SITHMESSAGE_SENDTOJOINEDPLAYERS, SITHMESSAGE_STREAM_ALL, DPSEND_GUARANTEED);
        }
    }
}

void J3DAPI sithCogFunctionThing_AttachThingToThing(SithCog* pCog)
{
    SithThing* pAttachThing = sithCogExec_PopThing(pCog);
    SithThing* pThing       = sithCogExec_PopThing(pCog);

    if ( !pThing )
    {
        STDLOG_ERROR("Cog %s: Bad thing in AttachThingToThing().\n", pCog->aName);
        return;
    }

    if ( !pAttachThing )
    {
        STDLOG_ERROR("Cog %s: Bad attach thing in AttachThingToThing().\n", pCog->aName);
        return;
    }

    sithThing_AttachThingToThing(pThing, pAttachThing);

    if ( (pCog->flags & SITHCOG_NOSYNC) == 0 )
    {
        if ( pCog->execMsgType != SITHCOG_MSG_STARTUP && pCog->execMsgType != SITHCOG_MSG_SHUTDOWN )
        {
            sithDSSThing_Attachment(pThing, SITHMESSAGE_SENDTOJOINEDPLAYERS, SITHMESSAGE_STREAM_ALL, DPSEND_GUARANTEED);
        }
    }
}

void J3DAPI sithCogFunctionThing_AttachThingToThingEx(SithCog* pCog)
{
    SithAttachFlag attflags = sithCogExec_PopInt(pCog);
    SithThing* pAttachThing = sithCogExec_PopThing(pCog);
    SithThing* pThing       = sithCogExec_PopThing(pCog);

    if ( !pThing )
    {
        STDLOG_ERROR("Cog %s: Invalid thing in AttachThingToThingEx.\n", pCog->aName);
        return;
    }

    if ( !pAttachThing )
    {
        STDLOG_ERROR("Cog %s: Invalid attach thing in AttachThingToThingEx.\n", pCog->aName);
        return;
    }

    sithThing_AttachThingToThing(pThing, pAttachThing);
    pThing->attach.flags |= attflags;

    if ( (pCog->flags & SITHCOG_NOSYNC) == 0 )
    {
        if ( pCog->execMsgType != SITHCOG_MSG_STARTUP && pCog->execMsgType != SITHCOG_MSG_SHUTDOWN )
        {
            sithDSSThing_Attachment(pThing, SITHMESSAGE_SENDTOJOINEDPLAYERS, SITHMESSAGE_STREAM_ALL, DPSEND_GUARANTEED);
        }
    }
}

void J3DAPI sithCogFunctionThing_GetThingAttachedThing(SithCog* pCog)
{
    int attype        = sithCogExec_PopInt(pCog); // if -1, then root attached thing is returned
    SithThing* pThing = sithCogExec_PopThing(pCog);

    if ( !pThing )
    {
        STDLOG_ERROR("Cog %s: Invalid THING in GetThingAttachedThing().\n", pCog->aName);
        sithCogExec_PushInt(pCog, -1);
        return;
    }

    int thingNum = -1;
    for ( SithThing* pAttachedThing = pThing->pAttachedThing; pAttachedThing; pAttachedThing = pAttachedThing->pNextAttachedThing )
    {
        if ( attype == -1 || pAttachedThing->type == attype )
        {
            thingNum = sithThing_GetThingIndex(pAttachedThing);
            break;
        }
    }

    sithCogExec_PushInt(pCog, thingNum);
}

void J3DAPI sithCogFunctionThing_WaitMode(SithCog* pCog)
{
    SithPuppetSubMode mode   = sithCogExec_PopInt(pCog);
    SithThing* pThing        = sithCogExec_PopThing(pCog);

    if ( !pThing )
    {
        STDLOG_ERROR("Cog %s: Invalid thing in WaitMode().\n", pCog->aName);
        sithCogExec_PushInt(pCog, -1);
        return;
    }

    SithPuppetTrack* pTrack = sithPuppet_GetModeTrack(pThing, mode);
    if ( pTrack )
    {
        pCog->status          = SITHCOG_STATUS_WAITING_KEYFRAME_TO_STOP;
        pCog->statusParams[0] = pThing->renderData.pPuppet->aTracks[pTrack->trackNum].guid;
        pCog->statusParams[1] = pThing->idx;
        sithCogExec_PushInt(pCog, 1);
    }
    else
    {
        sithCogExec_PushInt(pCog, 0);
    }
}

void J3DAPI sithCogFunctionThing_PlayMode(SithCog* pCog)
{
    int bWait                = sithCogExec_PopInt(pCog);
    SithPuppetSubMode mode   = sithCogExec_PopInt(pCog);
    SithThing* pThing        = sithCogExec_PopThing(pCog);

    if ( !pThing )
    {
        STDLOG_ERROR("Cog %s: Bad thing in PlayMode().\n", pCog->aName);
        sithCogExec_PushInt(pCog, -1);
        return;
    }

    int trackNum = sithPuppet_PlayMode(pThing, mode, /*pCallback=*/NULL);
    if ( trackNum == -1 )
    {
        STDLOG_ERROR("Cog %s: No space to add keyframe mode %d to thing %s in PlayMode().\n", pCog->aName, mode, pThing->aName);
        sithCogExec_PushInt(pCog, -1);
        return;
    }

    sithCogExec_PushInt(pCog, trackNum);

    if ( (pCog->flags & SITHCOG_NOSYNC) == 0 )
    {
        if ( pCog->execMsgType != SITHCOG_MSG_STARTUP && pCog->execMsgType != SITHCOG_MSG_SHUTDOWN && trackNum >= 0 )
        {
            sithDSSThing_PlayKeyMode(pThing, mode, pThing->renderData.pPuppet->aTracks[trackNum].guid, SITHMESSAGE_SENDTOJOINEDPLAYERS, SITHMESSAGE_STREAM_ALL);
        }
    }

    if ( bWait )
    {
        if ( trackNum >= 0 )
        {
            pCog->status          = SITHCOG_STATUS_WAITING_KEYFRAME_TO_STOP;
            pCog->statusParams[0] = pThing->renderData.pPuppet->aTracks[trackNum].guid;
            pCog->statusParams[1] = pThing->idx;
        }
    }
}

void J3DAPI sithCogFunctionThing_StopMode(SithCog* pCog)
{
    float fadeTime         = sithCogExec_PopFlex(pCog);
    SithPuppetSubMode mode = sithCogExec_PopInt(pCog);
    SithThing* pThing      = sithCogExec_PopThing(pCog);

    if ( !pThing )
    {
        STDLOG_ERROR("Cog %s: Bad thing in StopMode().\n", pCog->aName);
        return;
    }

    sithPuppet_StopMode(pThing, mode, fadeTime);
}

void J3DAPI sithCogFunctionThing_SynchMode(SithCog* pCog)
{
    int bReverse              = sithCogExec_PopInt(pCog);
    float unk                 = sithCogExec_PopFlex(pCog);
    SithPuppetSubMode newMode = sithCogExec_PopInt(pCog);
    SithPuppetSubMode oldMode = sithCogExec_PopInt(pCog);
    SithThing* pThing         = sithCogExec_PopThing(pCog);

    if ( !pThing )
    {
        STDLOG_ERROR("Cog %s: Bad thing in SynchMode().\n", pCog->aName);
        return;
    }

    if ( sithPuppet_SynchMode(pThing, oldMode, newMode, unk, bReverse) < 0 )
    {
        STDLOG_ERROR("Cog %s: SynchMode() - Unable to sync mode %d to mode %d.\n", pCog->aName, oldMode, newMode);
    }
}

void J3DAPI sithCogFunctionThing_IsModePlaying(SithCog* pCog)
{
    size_t mode       = sithCogExec_PopInt(pCog);
    SithThing* pThing = sithCogExec_PopThing(pCog);

    if ( !pThing || !pThing->pPuppetClass || !pThing->renderData.pPuppet ) // Added: check for null pPuppetClass & pPuppet
    {
        STDLOG_ERROR("Cog %s: Bad thing in IsModePlaying().\n", pCog->aName);
        sithCogExec_PushInt(pCog, -1);
        return;
    }

    // Fixed: Added OOB check
    if ( mode >= SITH_PUPPET_NUMSUBMODES )
    {
        STDLOG_ERROR("Cog %s: Bad mode in IsModePlaying().\n", pCog->aName);
        sithCogExec_PushInt(pCog, -1);
        return;
    }

    rdKeyframe* pKeyframe = pThing->pPuppetClass->aModes[pThing->pPuppetState->majorMode][mode].pKeyframe;
    if ( !pKeyframe )
    {
        sithCogExec_PushInt(pCog, 0);
        return;
    }

    size_t trackNum = 0;
    for ( ; trackNum < RDPUPPET_MAX_TRACKS && pThing->renderData.pPuppet->aTracks[trackNum].pKFTrack != pKeyframe; ++trackNum )
    {
        ;
    }

    if ( trackNum == RDPUPPET_MAX_TRACKS )
    {
        sithCogExec_PushInt(pCog, 0);
    }
    else if ( (pThing->renderData.pPuppet->aTracks[trackNum].status & RDPUPPET_TRACK_PAUSED) != 0 )
    {
        sithCogExec_PushInt(pCog, 2);
    }
    else
    {
        sithCogExec_PushInt(pCog, 1);
    }
}

void J3DAPI sithCogFunctionThing_PauseMode(SithCog* pCog)
{
    size_t mode       = sithCogExec_PopInt(pCog);
    SithThing* pThing = sithCogExec_PopThing(pCog);

    if ( !pThing || !pThing->pPuppetClass || !pThing->renderData.pPuppet ) // Added: check for null pPuppetClass & pPuppet
    {
        STDLOG_ERROR("Cog %s: Bad thing in PauseMode().\n", pCog->aName);
        sithCogExec_PushInt(pCog, -1);
        return;
    }

    // Fixed: Added OOB check
    if ( mode >= SITH_PUPPET_NUMSUBMODES )
    {
        STDLOG_ERROR("Cog %s: Bad mode in PauseMode().\n", pCog->aName);
        sithCogExec_PushInt(pCog, -1);
        return;
    }

    rdKeyframe* pKeyframe = pThing->pPuppetClass->aModes[pThing->pPuppetState->majorMode][mode].pKeyframe;
    if ( !pKeyframe )
    {
        sithCogExec_PushInt(pCog, 0);
        return;
    }

    size_t trackNum = 0;
    for ( ; trackNum < RDPUPPET_MAX_TRACKS && pThing->renderData.pPuppet->aTracks[trackNum].pKFTrack != pKeyframe; ++trackNum )
    {
        ;
    }

    if ( trackNum == RDPUPPET_MAX_TRACKS )
    {
        sithCogExec_PushInt(pCog, 0);
    }
    else
    {
        rdPuppetTrack* pTrack = &pThing->renderData.pPuppet->aTracks[trackNum];
        if ( (pTrack->status & RDPUPPET_TRACK_PAUSED) != 0 )
        {
            sithCogExec_PushInt(pCog, 2);
        }
        else
        {
            pTrack->status |= RDPUPPET_TRACK_PAUSED;
            sithCogExec_PushInt(pCog, 1);
        }
    }
}

void J3DAPI sithCogFunctionThing_ResumeMode(SithCog* pCog)
{
    size_t mode       = sithCogExec_PopInt(pCog);
    SithThing* pThing = sithCogExec_PopThing(pCog);

    if ( !pThing || !pThing->pPuppetClass || !pThing->renderData.pPuppet ) // Added: check for null pPuppetClass & pPuppet
    {
        STDLOG_ERROR("Cog %s: Bad thing in ResumeMode().\n", pCog->aName);
        sithCogExec_PushInt(pCog, -1);
        return;
    }

    // Fixed: Added OOB check
    if ( mode >= SITH_PUPPET_NUMSUBMODES )
    {
        STDLOG_ERROR("Cog %s: Bad mode in ResumeMode().\n", pCog->aName);
        sithCogExec_PushInt(pCog, -1);
        return;
    }

    rdKeyframe* pKeyframe = pThing->pPuppetClass->aModes[pThing->pPuppetState->majorMode][mode].pKeyframe;
    if ( !pKeyframe )
    {
        sithCogExec_PushInt(pCog, 0);
        return;
    }

    size_t trackNum = 0;
    for ( ; trackNum < RDPUPPET_MAX_TRACKS && pThing->renderData.pPuppet->aTracks[trackNum].pKFTrack != pKeyframe; ++trackNum )
    {
        ;
    }

    if ( trackNum == RDPUPPET_MAX_TRACKS )
    {
        sithCogExec_PushInt(pCog, 0);
    }
    else
    {
        rdPuppetTrack* pTrack = &pThing->renderData.pPuppet->aTracks[trackNum];
        if ( (pTrack->status & RDPUPPET_TRACK_PAUSED) != 0 )
        {
            pTrack->status &= ~RDPUPPET_TRACK_PAUSED;
            sithCogExec_PushInt(pCog, 1);
        }
        else
        {
            sithCogExec_PushInt(pCog, 2);
        }
    }
}

void J3DAPI sithCogFunctionThing_TrackToMode(SithCog* pCog)
{
    size_t trackNum   = sithCogExec_PopInt(pCog);
    SithThing* pThing = sithCogExec_PopThing(pCog);

    if ( !pThing || !pThing->pPuppetClass || !pThing->renderData.pPuppet ) // Added: check for null pPuppetClass & pPuppet
    {
        STDLOG_ERROR("Cog %s: Bad thing in TrackToMode().\n", pCog->aName);
        sithCogExec_PushInt(pCog, -1);
    }

    if ( trackNum >= RDPUPPET_MAX_TRACKS )
    {
        // TODO: maybe add log
        sithCogExec_PushInt(pCog, -1);
        return;
    }

    rdKeyframe* pKFTrack = pThing->renderData.pPuppet->aTracks[trackNum].pKFTrack;
    if ( !pKFTrack )
    {
        sithCogExec_PushInt(pCog, -1);
        return;
    }

    size_t mode = 0;
    for ( ; mode < SITH_PUPPET_NUMSUBMODES && pThing->pPuppetClass->aModes[pThing->pPuppetState->majorMode][mode].pKeyframe != pKFTrack; ++mode )
    {
        ;
    }

    if ( mode != SITH_PUPPET_NUMSUBMODES )
    {
        sithCogExec_PushInt(pCog, mode);
    }
    else
    {
        sithCogExec_PushInt(pCog, -1);
    }
}

void J3DAPI sithCogFunctionThing_PlayKey(SithCog* pCog)
{
    int bWait               = sithCogExec_PopInt(pCog);
    rdKeyframeFlags kfflags = sithCogExec_PopInt(pCog);
    int lowPriority         = sithCogExec_PopInt(pCog);
    rdKeyframe* pKfTrack    = sithCogExec_PopKeyframe(pCog);
    SithThing* pThing       = sithCogExec_PopThing(pCog);

    if ( !pThing )
    {
        STDLOG_ERROR("Cog %s: No thing to play keyframe anim on in PlayKey().\n", pCog->aName);
        sithCogExec_PushInt(pCog, -1);
        return;
    }

    // Fixed: Moved thich check here to avoid potential null pointer access in !pPuppet scope
    if ( !pKfTrack )
    {
        STDLOG_ERROR("Cog %s: Bad keyframe reference in PlayKey().\n", pCog->aName);
        sithCogExec_PushInt(pCog, -1);
        return;
    }

    rdPuppet* pPuppet = pThing->renderData.pPuppet;
    if ( !pPuppet )
    {
        STDLOG_ERROR("Cog %s: tried to play keyframe anim %s on thing %s - no puppet in PlayKey().\n", pCog->aName, pKfTrack->aName, pThing->aName);
        sithCogExec_PushInt(pCog, -1);
        return;
    }

    int trackNum = sithPuppet_PlayKey(pPuppet, pKfTrack, lowPriority, lowPriority + 2, kfflags, /*pCallback=*/NULL);
    if ( trackNum < 0 )
    {
        STDLOG_ERROR("Cog %s: No space to add keyframe %s to thing %s.\n", pCog->aName, pKfTrack, pThing->aName);
        sithCogExec_PushInt(pCog, -1);
        return;
    }

    sithCogExec_PushInt(pCog, trackNum);

    if ( pThing->moveType == SITH_MT_PATH )
    {
        if ( pThing->moveInfo.pathMovement.mode )
        {
            sithPathMove_Finish(pThing);
        }

        pThing->moveInfo.pathMovement.curOrient.dvec = pThing->pos;
    }

    if ( (pCog->flags & SITHCOG_NOSYNC) == 0 )
    {
        if ( pCog->execMsgType != SITHCOG_MSG_STARTUP && pCog->execMsgType != SITHCOG_MSG_SHUTDOWN )
        {
            sithDSSThing_PlayKey(pThing, pKfTrack, kfflags, lowPriority, pThing->renderData.pPuppet->aTracks[trackNum].guid, SITHMESSAGE_SENDTOJOINEDPLAYERS, SITHMESSAGE_STREAM_ALL);
        }
    }

    if ( bWait )
    {
        pCog->status          = SITHCOG_STATUS_WAITING_KEYFRAME_TO_STOP;
        pCog->statusParams[0] = pThing->renderData.pPuppet->aTracks[trackNum].guid;
        pCog->statusParams[1] = pThing->idx;
    }
}

void J3DAPI sithCogFunctionThing_PlayKeyEx(SithCog* pCog)
{
    int bWait               = sithCogExec_PopInt(pCog);
    rdKeyframeFlags kfflags = sithCogExec_PopInt(pCog);
    int highPriority        = sithCogExec_PopInt(pCog);
    int lowPriority         = sithCogExec_PopInt(pCog);
    rdKeyframe* pKfTrack    = sithCogExec_PopKeyframe(pCog);
    SithThing* pThing       = sithCogExec_PopThing(pCog);

    if ( !pThing )
    {
        STDLOG_ERROR("Cog %s: No thing to play keyframe anim on in PlayKeyEx().\n", pCog->aName);
        sithCogExec_PushInt(pCog, -1);
        return;
    }

    if ( !pKfTrack )
    {
        STDLOG_ERROR("Cog %s: Bad keyframe reference in PlayKeyEx().\n", pCog->aName);
        sithCogExec_PushInt(pCog, -1);
        return;
    }

    // Added: Null check for pPuppet
    rdPuppet* pPuppet = pThing->renderData.pPuppet;
    if ( !pPuppet )
    {
        STDLOG_ERROR("Cog %s: tried to play keyframe anim %s on thing %s - no puppet in PlayKeyEx().\n", pCog->aName, pKfTrack->aName, pThing->aName);
        sithCogExec_PushInt(pCog, -1);
        return;
    }

    int trackNum = sithPuppet_PlayKey(pPuppet, pKfTrack, lowPriority, highPriority, kfflags, /*pCallback=*/NULL);
    if ( trackNum < 0 )
    {
        STDLOG_ERROR("Cog %s: No space to add keyframe %s to thing %s in PLayKeyEx().\n", pCog->aName, pKfTrack->aName, pThing->aName);
        sithCogExec_PushInt(pCog, trackNum);
        return;
    }

    sithCogExec_PushInt(pCog, trackNum);

    if ( pThing->moveType == SITH_MT_PATH )
    {
        if ( pThing->moveInfo.pathMovement.mode )
        {
            sithPathMove_Finish(pThing);
        }

        pThing->moveInfo.pathMovement.curOrient.dvec = pThing->pos;
    }

    // TODO: Are we missing sithDSSThing sync?

    if ( bWait )
    {
        pCog->status          = SITHCOG_STATUS_WAITING_KEYFRAME_TO_STOP;
        pCog->statusParams[0] = pThing->renderData.pPuppet->aTracks[trackNum].guid;
        pCog->statusParams[1] = pThing->idx;
    }
}

void J3DAPI sithCogFunctionThing_PauseKey(SithCog* pCog)
{
    size_t trackNum   = sithCogExec_PopInt(pCog);
    SithThing* pThing = sithCogExec_PopThing(pCog);

    if ( !pThing )
    {
        STDLOG_ERROR("Cog %s: Bad thing in PauseKey().\n", pCog->aName);
        sithCogExec_PushInt(pCog, -1);
        return;
    }

    if ( !pThing->pPuppetClass )
    {
        STDLOG_ERROR("Cog %s: Thing %s has no puppet in PauseKey().\n", pCog->aName, pThing->aName);
        sithCogExec_PushInt(pCog, -1);
        return;
    }

    if ( trackNum >= RDPUPPET_MAX_TRACKS )
    {
        STDLOG_ERROR("Cog %s: Invalid track number %d for thing %s in PauseKey().\n", pCog->aName, trackNum, pThing->aName);
        sithCogExec_PushInt(pCog, -1);
        return;
    }

    rdPuppetTrack* pTrack = &pThing->renderData.pPuppet->aTracks[trackNum];
    if ( pTrack->status == 0 )
    {
        STDLOG_ERROR("Cog %s: Track number %d does not exist for thing %s in PauseKey().\n", pCog->aName, trackNum, pThing->aName);
        sithCogExec_PushInt(pCog, -1);
        return;
    }

    pTrack->status |= RDPUPPET_TRACK_PAUSED;
    sithCogExec_PushInt(pCog, 1);

    // TODO: should we also do sithDSSThing sync?
    return;
}

void J3DAPI sithCogFunctionThing_ResumeKey(SithCog* pCog)
{
    size_t trackNum   = sithCogExec_PopInt(pCog);
    SithThing* pThing = sithCogExec_PopThing(pCog);

    if ( !pThing )
    {
        STDLOG_ERROR("Cog %s: Bad thing in ResumeKey().\n", pCog->aName);
        sithCogExec_PushInt(pCog, -1);
        return;
    }

    if ( !pThing->pPuppetClass )
    {
        STDLOG_ERROR("Cog %s: Thing %s has no puppet in ResumeKey().\n", pCog->aName, pThing->aName);
        sithCogExec_PushInt(pCog, -1);
        return;
    }

    if ( trackNum >= RDPUPPET_MAX_TRACKS )
    {
        STDLOG_ERROR("Cog %s: Invalid track number %d for thing %s in ResumeKey().\n", pCog->aName, trackNum, pThing->aName);
        sithCogExec_PushInt(pCog, -1);
        return;
    }

    rdPuppetTrack* pTrack = &pThing->renderData.pPuppet->aTracks[trackNum];
    if ( pTrack->status == 0 )
    {
        STDLOG_ERROR("Cog %s: Track number %d does not exist for thing %s in ResumeKey().\n", pCog->aName, trackNum, pThing->aName);
        sithCogExec_PushInt(pCog, -1);
        return;
    }

    pTrack->status &= ~RDPUPPET_TRACK_PAUSED;
    sithCogExec_PushInt(pCog, 1);

    // TODO: should we also do sithDSSThing sync?
    return;
}

void J3DAPI sithCogFunctionThing_StopKey(SithCog* pCog)
{
    float fadeTime    = sithCogExec_PopFlex(pCog);
    size_t trackNum   = sithCogExec_PopInt(pCog);
    SithThing* pThing = sithCogExec_PopThing(pCog);

    if ( !pThing )
    {
        STDLOG_ERROR("Cog %s: No thing to stop keyframe anim on.\n", pCog->aName);
        return;
    }

    rdPuppet* pPuppet = pThing->renderData.pPuppet;
    if ( !pPuppet )
    {
        STDLOG_ERROR("Cog %s: cannot stop puppet track %d, not puppet.\n", pCog->aName, trackNum);
        return;
    }

    if ( trackNum >= RDPUPPET_MAX_TRACKS )
    {
        STDLOG_ERROR("Cog %s: can't stop track %d, out of range.\n", pCog->aName, trackNum);
        return;
    }

    if ( fadeTime < 0.0 )
    {
        STDLOG_ERROR("Cog %s: fadeTime %g, is illegal.\n", pCog->aName, fadeTime);
        return;
    }

    int trackGUID = pPuppet->aTracks[trackNum].guid;
    if ( sithPuppet_StopKey(pPuppet, trackNum, fadeTime) )
    {
        if ( (pCog->flags & SITHCOG_NOSYNC) == 0 )
        {
            if ( pCog->execMsgType != SITHCOG_MSG_STARTUP && pCog->execMsgType != SITHCOG_MSG_SHUTDOWN )
            {
                sithDSSThing_StopKey(pThing, trackGUID, fadeTime, SITHMESSAGE_SENDTOJOINEDPLAYERS, SITHMESSAGE_STREAM_ALL);
            }
        }
    }
}

void J3DAPI sithCogFunctionThing_SetThingMesh(SithCog* pCog)
{
    const int meshNumSrc = sithCogExec_PopInt(pCog);
    rdModel3* pModel     = sithCogExec_PopModel3(pCog);
    const int meshNum    = sithCogExec_PopInt(pCog);
    SithThing* pThing    = sithCogExec_PopThing(pCog);

    if ( !pThing )
    {
        STDLOG_ERROR("Cog %s: Bad thing in SetThingMesh.\n", pCog->aName);
        sithCogExec_PushInt(pCog, -1); // Fixed: Added return value pushed to execution stack
        return;
    }
    if ( !pModel )
    {
        STDLOG_ERROR("Cog %s: Bad model in SetThingMesh.\n", pCog->aName);
        sithCogExec_PushInt(pCog, -1); // Fixed: Added return value pushed to execution stack
        return;
    }

    const int refNum = sithThing_AddSwapEntry(pThing, meshNum, pModel, meshNumSrc);
    sithCogExec_PushInt(pCog, refNum);

    if ( (pCog->flags & SITHCOG_NOSYNC) == 0 )
    {
        if ( pCog->execMsgType != SITHCOG_MSG_STARTUP && pCog->execMsgType != SITHCOG_MSG_SHUTDOWN )
        {
            sithThing_SyncThing(pThing, SITHTHING_SYNC_STATE);
        }
    }
}

void J3DAPI sithCogFunctionThing_RestoreThingMesh(SithCog* pCog)
{
    const int  refnum = sithCogExec_PopInt(pCog);
    SithThing* pThing = sithCogExec_PopThing(pCog);

    if ( !pThing )
    {
        STDLOG_ERROR("Cog %s: Bad thing in RestoreThingMesh.\n", pCog->aName);
        return;
    }
    if ( refnum == -99 )
    {
        sithThing_ResetSwapList(pThing);
    }
    else
    {
        sithThing_RemoveSwapEntry(pThing, refnum);
    }

    if ( (pCog->flags & SITHCOG_NOSYNC) == 0 )
    {
        if ( pCog->execMsgType != SITHCOG_MSG_STARTUP && pCog->execMsgType != SITHCOG_MSG_SHUTDOWN )
        {
            sithThing_SyncThing(pThing, SITHTHING_SYNC_STATE);
        }
    }
}

void J3DAPI sithCogFunctionThing_SetThingModel(SithCog* pCog)
{
    rdModel3* pModel  = sithCogExec_PopModel3(pCog);
    SithThing* pThing = sithCogExec_PopThing(pCog);

    if ( !pThing )
    {
        STDLOG_ERROR("Cog %s: SetThingModel - No thing to set.\n", pCog->aName);
        sithCogExec_PushInt(pCog, -1);
        return;
    }

    if ( !pModel )
    {
        STDLOG_ERROR("Cog %s: SetThingModel - No model to set.\n", pCog->aName);
        sithCogExec_PushInt(pCog, -1);
        return;
    }

    const rdModel3* pCurModel = pThing->renderData.data.pModel3;
    int prevModelIdx = pCurModel ? pCurModel->num : -1;
    if ( !pCurModel )
    {
        STDLOG_WARNING("Cog %s: SetThingModel - Thing had no model.\n", pCog->aName); // Altered: Changed error to warning
    }

    if ( !sithThing_SetThingModel(pThing, pModel) )
    {
        STDLOG_ERROR("Cog %s: SetThingModel - Failed to set model.\n", pCog->aName);
        sithCogExec_PushInt(pCog, -1);
        return;
    }

    sithCogExec_PushInt(pCog, prevModelIdx);

    if ( (pCog->flags & SITHCOG_NOSYNC) == 0 )
    {
        if ( pCog->execMsgType != SITHCOG_MSG_STARTUP && pCog->execMsgType != SITHCOG_MSG_SHUTDOWN )
        {
            sithDSSThing_SetModel(pThing, SITHMESSAGE_SENDTOJOINEDPLAYERS);
        }
    }
}

void J3DAPI sithCogFunctionThing_GetThingModel(SithCog* pCog)
{
    const SithThing* pThing = sithCogExec_PopThing(pCog);

    if ( !pThing )
    {
        STDLOG_ERROR("Cog %s: GetThingModel - No thing to set.\n", pCog->aName);
        sithCogExec_PushInt(pCog, -1);
        return;
    }

    if ( pThing->renderData.type != RD_THING_MODEL3 )
    {
        STDLOG_ERROR("Cog %s: GetThingModel - Thing is not model.\n", pCog->aName);
        sithCogExec_PushInt(pCog, -1);
        return;
    }

    const rdModel3* pModel = pThing->renderData.data.pModel3;
    if ( !pModel )
    {
        STDLOG_ERROR("Cog %s: GetThingModel - No model on thing.\n", pCog->aName);
        sithCogExec_PushInt(pCog, -1);
        return;
    }

    sithCogExec_PushInt(pCog, pModel->num);
}

void J3DAPI sithCogFunctionThing_SetArmedMode(SithCog* pCog)
{
    const size_t armedMode = sithCogExec_PopInt(pCog);
    SithThing* pThing      = sithCogExec_PopThing(pCog);

    if ( !pThing )
    {
        STDLOG_ERROR("Cog %s: Trying to set armed mode on illegal thing.\n", pCog->aName);
        return;
    }
    if ( armedMode >= SITH_PUPPET_NUMARMEDMODES )
    {
        STDLOG_ERROR("Cog %s: Trying to set invalid armed mode '%d' on thing '%s'.\n", pCog->aName, armedMode, pThing->aName);
        return;
    }

    sithPuppet_SetArmedMode(pThing, armedMode);

    if ( (pCog->flags & SITHCOG_NOSYNC) == 0 )
    {
        if ( pCog->execMsgType != SITHCOG_MSG_STARTUP && pCog->execMsgType != SITHCOG_MSG_SHUTDOWN )
        {
            sithDSSThing_UpdateState(pThing, SITHMESSAGE_SENDTOJOINEDPLAYERS, SITHMESSAGE_STREAM_ALL); // TODO: why not using sithThing_Sync function?
        }
    }
}

void J3DAPI sithCogFunctionThing_GetThingFlags(SithCog* pCog)
{
    SithThing* pThing = sithCogExec_PopThing(pCog);

    if ( !pThing )
    {
        STDLOG_ERROR("Cog %s: Bad thing reference in GetThingFlags().\n", pCog->aName); // Added: Error log
        sithCogExec_PushInt(pCog, -1);
        return;
    }

    sithCogExec_PushInt(pCog, pThing->flags);
}

void J3DAPI sithCogFunctionThing_SetThingFlags(SithCog* pCog)
{
    const int  flags  = sithCogExec_PopInt(pCog);
    SithThing* pThing = sithCogExec_PopThing(pCog);

    if ( !pThing || flags == 0 )
    {
        STDLOG_ERROR("Cog %s: Cannot set thing flags.\n", pCog->aName);
        return;
    }

    pThing->flags |= flags;

    if ( (pCog->flags & SITHCOG_NOSYNC) == 0 )
    {
        if ( pCog->execMsgType != SITHCOG_MSG_STARTUP && pCog->execMsgType != SITHCOG_MSG_SHUTDOWN )
        {
            sithThing_SyncThing(pThing, SITHTHING_SYNC_STATE);
        }
    }
}

void J3DAPI sithCogFunctionThing_ClearThingFlags(SithCog* pCog)
{
    const int  flags  = sithCogExec_PopInt(pCog);
    SithThing* pThing = sithCogExec_PopThing(pCog);

    if ( !pThing || !flags )
    {
        STDLOG_ERROR("Cog %s: Cannot clear thing flags.\n", pCog->aName);
        return;
    }

    pThing->flags &= ~flags;

    if ( (pCog->flags & SITHCOG_NOSYNC) == 0 )
    {
        if ( pCog->execMsgType != SITHCOG_MSG_STARTUP && pCog->execMsgType != SITHCOG_MSG_SHUTDOWN )
        {
            sithThing_SyncThing(pThing, SITHTHING_SYNC_STATE);
        }
    }
}

void J3DAPI sithCogFunctionThing_TeleportThing(SithCog* pCog)
{
    SithThing* pSrcThing  = sithCogExec_PopThing(pCog);
    SithThing* pDestThing = sithCogExec_PopThing(pCog);

    if ( !pSrcThing || !pDestThing )
    {
        STDLOG_ERROR("Cog %s: Illegal params in TeleportThing.\n", pCog->aName);
        return;
    }

    if ( pDestThing->attach.flags != 0 )
    {
        sithThing_DetachThing(pDestThing);
    }

    // Copy pos and orient
    pDestThing->pos    = pSrcThing->pos;
    pDestThing->orient = pSrcThing->orient;

    // Set sector
    sithThing_SetSector(pDestThing, pSrcThing->pInSector, /*bNotify=*/0);

    // Stick to floor
    if ( pDestThing->moveType == SITH_MT_PHYSICS && (pDestThing->moveInfo.physics.flags & SITH_PF_FLOORSTICK) != 0 )
    {
        sithPhysics_FindFloor(pDestThing, /*bNoThingStateUpdate=*/1);
    }

    if ( pDestThing == sithPlayer_g_pLocalPlayerThing )
    {
        sithCamera_Update(sithCamera_g_pCurCamera);
    }

    if ( (pCog->flags & SITHCOG_NOSYNC) == 0 )
    {
        if ( pCog->execMsgType != SITHCOG_MSG_STARTUP && pCog->execMsgType != SITHCOG_MSG_SHUTDOWN )
        {
            sithDSSThing_Pos(pDestThing, SITHMESSAGE_SENDTOJOINEDPLAYERS, DPSEND_GUARANTEED);
        }
    }
}

void J3DAPI sithCogFunctionThing_SetThingType(SithCog* pCog)
{
    int type         = sithCogExec_PopInt(pCog);
    SithThing* pThing = sithCogExec_PopThing(pCog);

    if ( !pThing )
    {
        STDLOG_ERROR("Cog %s: Bad thing reference in SetThingType.\n", pCog->aName); // Added: Error log
        return;
    }

    // Fixed: Added check for valid type
    if ( type < 0 || type >= SITH_THING_NUMTYPES )
    {
        STDLOG_ERROR("Cog %s: Invalid thing type %d in SetThingType.\n", pCog->aName, type);
        return;
    }

    pThing->type = type;

    // TODO: Synce thing?
}

void J3DAPI sithCogFunctionThing_GetCollideType(SithCog* pCog)
{
    SithThing* pThing = sithCogExec_PopThing(pCog);

    if ( !pThing )
    {
        STDLOG_ERROR("Cog %s: Cannot get thing collide type.\n", pCog->aName); // Added: Error log
        sithCogExec_PushInt(pCog, -1);
        return;
    }

    sithCogExec_PushInt(pCog, pThing->collide.type);
}

void J3DAPI sithCogFunctionThing_SetCollideType(SithCog* pCog)
{
    SithCollideType type  = sithCogExec_PopInt(pCog);
    SithThing* pThing      = sithCogExec_PopThing(pCog);

    if ( !pThing || (unsigned int)type >= SITH_COLLIDE_NUMTYPES )
    {
        STDLOG_ERROR("Cog %s: Cannot set thing collide type.\n", pCog->aName);
        return;
    }

    pThing->collide.type = type;

    if ( (pCog->flags & SITHCOG_NOSYNC) == 0 )
    {
        if ( pCog->execMsgType != SITHCOG_MSG_STARTUP && pCog->execMsgType != SITHCOG_MSG_SHUTDOWN )
        {
            sithThing_SyncThing(pThing, SITHTHING_SYNC_STATE);
        }
    }
}

void J3DAPI sithCogFunctionThing_FirstThingInSector(SithCog* pCog)
{
    SithSector* pSector = sithCogExec_PopSector(pCog);

    if ( !pSector )
    {
        STDLOG_ERROR("Cog %s: Cannot get first thing in sector. Invalid sector.\n", pCog->aName);
        sithCogExec_PushInt(pCog, -1);
        //sithCogExec_PushInt(pCog, -1); // Fixed: Removed redundant push.
                                         // Original code was missing return statement here so it continue to executing,
                                         // reaching case where !pFirstThing and that resulted in another -1 push to stack.
        return;
    }

    SithThing* pFirstThing = pSector->pFirstThingInSector;
    sithCogExec_PushInt(pCog, pFirstThing ? pFirstThing->idx : -1);
}

void J3DAPI sithCogFunctionThing_NextThingInSector(SithCog* pCog)
{
    SithThing* pThing = sithCogExec_PopThing(pCog);

    if ( !pThing )
    {
        STDLOG_ERROR("Cog %s: No thing passed to NextThingInSector.\n", pCog->aName);
        sithCogExec_PushInt(pCog, -1);
        return;
    }

    SithThing* pNextThing = pThing->pNextThingInSector;
    sithCogExec_PushInt(pCog, pNextThing ? pNextThing->idx : -1);
}

void J3DAPI sithCogFunctionThing_PrevThingInSector(SithCog* pCog)
{
    SithThing* pThing = sithCogExec_PopThing(pCog);

    if ( !pThing )
    {
        STDLOG_ERROR("Cog %s: No thing passed to PrevThingInSector().\n", pCog->aName);
        sithCogExec_PushInt(pCog, -1);
        return;
    }

    SithThing* pPrevThing = pThing->pPrevThingInSector;
    sithCogExec_PushInt(pCog, pPrevThing ? pPrevThing->idx : -1);
}

void J3DAPI sithCogFunctionThing_GetInventoryMinimum(SithCog* pCog)
{
    const int  bin    = sithCogExec_PopInt(pCog);
    SithThing* pThing = sithCogExec_PopThing(pCog);

    if ( !pThing || pThing->type != SITH_THING_PLAYER || !pThing->thingInfo.actorInfo.pPlayer )
    {
        STDLOG_ERROR("Cog %s: No thing passed to GetInventoryMinimum.\n", pCog->aName);
        sithCogExec_PushFlex(pCog, -1.0f);
        return;
    }

    float minVal = sithInventory_GetInventoryMinimum(pThing, bin);
    sithCogExec_PushFlex(pCog, minVal);
}

void J3DAPI sithCogFunctionThing_GetInventoryMaximum(SithCog* pCog)
{
    const int  bin    = sithCogExec_PopInt(pCog);
    SithThing* pThing = sithCogExec_PopThing(pCog);

    if ( !pThing || pThing->type != SITH_THING_PLAYER || !pThing->thingInfo.actorInfo.pPlayer )
    {
        STDLOG_ERROR("Cog %s: No thing passed to GetInventoryMaximum.\n", pCog->aName);
        sithCogExec_PushFlex(pCog, -1.0f);
        return;
    }

    float maxVal = sithInventory_GetInventoryMaximum(pThing, bin);
    sithCogExec_PushFlex(pCog, maxVal);
}

void J3DAPI sithCogFunctionThing_PathMovePause(SithCog* pCog)
{
    SithThing* pThing  = sithCogExec_PopThing(pCog);

    if ( !pThing || pThing->moveType != SITH_MT_PATH )
    {
        STDLOG_ERROR("Cog %s: Bad parameters in PathMovePause().\n", pCog->aName); // Added: Error log
        sithCogExec_PushInt(pCog, -1);
        return;
    }

    SITH_ASSERTREL(sithThing_ValidateThingPointer(sithWorld_g_pCurrentWorld, pThing));

    int bPaused = sithPathMove_PathMovePause(pThing);
    sithCogExec_PushInt(pCog, (bPaused == 1) ? pThing->idx : -1);
}

void J3DAPI sithCogFunctionThing_SetHeadLightIntensity(SithCog* pCog)
{
    rdVector3 color;
    int bVec          = sithCogExec_PopVector(pCog, &color);
    SithThing* pThing = sithCogExec_PopThing(pCog);

    if ( !pThing || ((pThing->type != SITH_THING_ACTOR) && (pThing->type != SITH_THING_PLAYER)) || !bVec ) // Fixed: Added check for bVec
    {
        STDLOG_ERROR("Cog %s: Bad parameters in SetHeadLightIntensity().\n", pCog->aName); // Added: Error log
        sithCogExec_PushVector(pCog, &(const rdVector3){ -1.0f, -1.0f, -1.0f });
        return;
    }

    pThing->thingInfo.actorInfo.headLightIntensity.red   = color.red;
    pThing->thingInfo.actorInfo.headLightIntensity.green = color.green;
    pThing->thingInfo.actorInfo.headLightIntensity.blue  = color.blue;
    pThing->thingInfo.actorInfo.headLightIntensity.alpha = 0.0f; // TODO: set alpha to > 0.0f to enable headlight
    sithCogExec_PushVector(pCog, &color);
}

void J3DAPI sithCogFunctionThing_GetHeadLightIntensity(SithCog* pCog)
{
    rdVector3 dummy;
    sithCogExec_PopVector(pCog, &dummy); // TODO:This pop must be a bug

    SithThing* pThing = sithCogExec_PopThing(pCog);

    if ( !pThing || ((pThing->type != SITH_THING_ACTOR) && (pThing->type != SITH_THING_PLAYER)) )
    {
        STDLOG_ERROR("Cog %s: Bad parameters in GetHeadLightIntensity().\n", pCog->aName); // Added: Error log
        sithCogExec_PushVector(pCog, &(const rdVector3){ -1.0f, -1.0f, -1.0f });
        return;
    }

    sithCogExec_PushVector(pCog, (rdVector3*)&pThing->thingInfo.actorInfo.headLightIntensity);
}

void J3DAPI sithCogFunctionThing_IsThingVisible(SithCog* pCog)
{
    SithThing* pThing = sithCogExec_PopThing(pCog);

    if ( !pThing )
    {
        STDLOG_ERROR("Cog %s: Bad parameters in IsThingVisible().\n", pCog->aName);
        sithCogExec_PushInt(pCog, 0);
        return;
    }

    sithCogExec_PushInt(pCog, (pThing->renderFrame + 1 >= sithMain_g_frameNumber));
}

void J3DAPI sithCogFunctionThing_PathMoveResume(SithCog* pCog)
{
    SithThing* pThing = sithCogExec_PopThing(pCog);

    if ( !pThing || pThing->moveType != SITH_MT_PATH )
    {
        STDLOG_ERROR("Cog %s: Bad parameters in PathMoveResume().\n", pCog->aName); // Added: Error log
        sithCogExec_PushInt(pCog, -1);
        return;
    }

    SITH_ASSERTREL(sithThing_ValidateThingPointer(sithWorld_g_pCurrentWorld, pThing));

    int bResumed = sithPathMove_PathMoveResume(pThing);
    sithCogExec_PushInt(pCog, (bResumed == 1) ? pThing->idx : -1);
}

void J3DAPI sithCogFunctionThing_SetThingCurLightMode(SithCog* pCog)
{
    rdLightMode mode  = sithCogExec_PopInt(pCog);
    SithThing* pThing = sithCogExec_PopThing(pCog);

    if ( !pThing )
    {
        STDLOG_ERROR("Cog %s: Bad thing in SetThingCurLightMode().\n", pCog->aName);
        return;
    }

    pThing->renderData.lightMode = mode;
}

void J3DAPI sithCogFunctionThing_GetThingCurLightMode(SithCog* pCog)
{
    SithThing* pThing = sithCogExec_PopThing(pCog);

    if ( !pThing )
    {
        STDLOG_ERROR("Cog %s: Bad thing in GetThingCurLightMode().\n", pCog->aName);
        return;
    }

    sithCogExec_PushInt(pCog, pThing->renderData.lightMode);
}

void J3DAPI sithCogFunctionThing_SetActorExtraSpeed(SithCog* pCog)
{
    const float speed  = sithCogExec_PopFlex(pCog);
    SithThing* pThing  = sithCogExec_PopThing(pCog);

    if ( !pThing || ((pThing->type != SITH_THING_ACTOR) && (pThing->type != SITH_THING_PLAYER)) ) // Added: Check for valid thing type
    {
        STDLOG_ERROR("Cog %s: Bad parameters in SetActorExtraSpeed().\n", pCog->aName);
        return;
    }

    pThing->thingInfo.actorInfo.extraSpeed = speed;
}

void J3DAPI sithCogFunctionThing_GetThingTemplate(SithCog* pCog)
{
    SithThing* pThing = sithCogExec_PopThing(pCog);

    if ( !pThing )
    {
        STDLOG_ERROR("Cog %s: Bad thing in GetThingTemplate().\n", pCog->aName);
        sithCogExec_PushInt(pCog, -1);
        return;
    }

    if ( !pThing->pTemplate )
    {
        STDLOG_ERROR("Cog %s: No template for thing in GetThingTemplate().\n", pCog->aName);
        sithCogExec_PushInt(pCog, -1);
        return;
    }

    sithCogExec_PushInt(pCog, pThing->pTemplate->idx);
}

void J3DAPI sithCogFunctionThing_SetLifeleft(SithCog* pCog)
{
    const float secLeft = sithCogExec_PopFlex(pCog);
    SithThing* pThing   = sithCogExec_PopThing(pCog);

    if ( !pThing )
    {
        STDLOG_ERROR("Cog %s: Bad thing in SetLifeleft().\n", pCog->aName);
        return;
    }

    if ( secLeft < 0.0f )
    {
        STDLOG_ERROR("Cog %s: Seconds must be positive for SetLifeleft().\n", pCog->aName);
        return;
    }

    pThing->msecLifeLeft = (uint32_t)(secLeft * 1000.0f);

    if ( (pCog->flags & SITHCOG_NOSYNC) == 0 )
    {
        if ( pCog->execMsgType != SITHCOG_MSG_STARTUP && pCog->execMsgType != SITHCOG_MSG_SHUTDOWN )
        {
            sithThing_SyncThing(pThing, SITHTHING_SYNC_STATE);
        }
    }
}

void J3DAPI sithCogFunctionThing_GetLifeleft(SithCog* pCog)
{
    SithThing* pThing = sithCogExec_PopThing(pCog);

    if ( !pThing )
    {
        STDLOG_ERROR("Cog %s: Bad thing in GetLifeleft().\n", pCog->aName);
        return;
    }

    float secLeft = (float)pThing->msecLifeLeft / 1000.0f;
    sithCogExec_PushFlex(pCog, secLeft);
}

void J3DAPI sithCogFunctionThing_SetThingThrust(SithCog* pCog)
{
    rdVector3  newThrust;
    int bValidThrust  = sithCogExec_PopVector(pCog, &newThrust);
    SithThing* pThing = sithCogExec_PopThing(pCog);

    if ( !pThing || pThing->moveType != SITH_MT_PHYSICS )
    {
        STDLOG_ERROR("Cog %s: Bad thing or no physics in SetThingThrust().\n", pCog->aName);
        return;
    }

    if ( !bValidThrust )
    {
        STDLOG_ERROR("Cog %s: Bad vector in SetThingThrust().\n", pCog->aName);
        return;
    }

    // Return previous newThrust
    sithCogExec_PushVector(pCog, &pThing->moveInfo.physics.thrust);

    // Now set new newThrust
    pThing->moveInfo.physics.thrust = newThrust;
}

void J3DAPI sithCogFunctionThing_GetThingThrust(SithCog* pCog)
{
    SithThing* pThing = sithCogExec_PopThing(pCog);

    if ( !pThing || pThing->moveType != SITH_MT_PHYSICS )
    {
        STDLOG_ERROR("Cog %s: Bad thing or no physics in GetThingThrust().\n", pCog->aName);
        return;
    }

    sithCogExec_PushVector(pCog, &pThing->moveInfo.physics.thrust);
}

void J3DAPI sithCogFunctionThing_AmputateJoint(SithCog* pCog)
{
    const size_t joint = sithCogExec_PopInt(pCog);
    SithThing* pThing  = sithCogExec_PopThing(pCog);

    if ( !pThing )
    {
        STDLOG_ERROR("Cog %s: Bad thing in AmputateJoint().\n", pCog->aName);
        return;
    }

    if ( !pThing->renderData.data.pModel3 )
    {
        SITH_ASSERTREL(pThing->renderData.data.pModel3 != NULL);
    }

    if ( joint > pThing->renderData.data.pModel3->numHNodes )
    {
        STDLOG_ERROR("Cog %s: Bad joint passed to AmputateJoint().\n", pCog->aName);
        return;
    }

    pThing->renderData.paJointAmputationFlags[joint] = 1;
}

void J3DAPI sithCogFunctionThing_SetActorWeapon(SithCog* pCog)
{
    SithThing* pWeapon = sithCogExec_PopTemplate(pCog);
    SithThing* pActor  = sithCogExec_PopThing(pCog);

    if ( !pActor || ((pActor->type != SITH_THING_ACTOR) && (pActor->type != SITH_THING_PLAYER)) )
    {
        STDLOG_ERROR("Cog %s: Bad thing or weapon in SetActorWeapon().\n", pCog->aName);
        return;
    }

    pActor->thingInfo.actorInfo.pWeaponTemplate = pWeapon;
}

void J3DAPI sithCogFunctionThing_GetActorWeapon(SithCog* pCog)
{
    SithThing* pActor = sithCogExec_PopThing(pCog);

    if ( !pActor || ((pActor->type != SITH_THING_ACTOR) && (pActor->type != SITH_THING_PLAYER)) )
    {
        STDLOG_ERROR("Cog %s: Bad thing or weapon in GetActorWeapon().\n", pCog->aName);
        return;
    }

    SithThing* pWeaponTemplate = pActor->thingInfo.actorInfo.pWeaponTemplate;
    sithCogExec_PushInt(pCog, pWeaponTemplate ? pWeaponTemplate->idx : -1);
}

void J3DAPI sithCogFunctionThing_GetPhysicsFlags(SithCog* pCog)
{
    SithThing* pThing = sithCogExec_PopThing(pCog);

    if ( !pThing || pThing->moveType != SITH_MT_PHYSICS )
    {
        STDLOG_ERROR("Cog %s: Bad arguments in GetPhysicsFlags().\n", pCog->aName); // Added: Error log
        sithCogExec_PushInt(pCog, -1);
        return;
    }

    sithCogExec_PushInt(pCog, pThing->moveInfo.physics.flags);
}

void J3DAPI sithCogFunctionThing_SetPhysicsFlags(SithCog* pCog)
{
    const int  physflags = sithCogExec_PopInt(pCog);
    SithThing* pThing    = sithCogExec_PopThing(pCog);

    if ( !pThing || physflags == 0 || pThing->moveType != SITH_MT_PHYSICS )
    {
        STDLOG_ERROR("Cog %s: Cannot set Physics flags.\n", pCog->aName);
        return;
    }

    pThing->moveInfo.physics.flags |= physflags;

    if ( (pCog->flags & SITHCOG_NOSYNC) == 0 )
    {
        if ( pCog->execMsgType != SITHCOG_MSG_STARTUP && pCog->execMsgType != SITHCOG_MSG_SHUTDOWN )
        {
            sithThing_SyncThing(pThing, SITHTHING_SYNC_STATE);
        }
    }
}

void J3DAPI sithCogFunctionThing_ClearPhysicsFlags(SithCog* pCog)
{
    const int  physflags = sithCogExec_PopInt(pCog);
    SithThing* pThing    = sithCogExec_PopThing(pCog);

    if ( !pThing || physflags == 0 || pThing->moveType != SITH_MT_PHYSICS )
    {
        STDLOG_ERROR("Cog %s: Cannot clear Physics flags.\n", pCog->aName);
        return;
    }

    pThing->moveInfo.physics.flags &= ~physflags;
}

void J3DAPI sithCogFunctionThing_ParseArg(SithCog* pCog)
{
    const char* pArgs = sithCogExec_PopString(pCog);
    SithThing* pThing = sithCogExec_PopThing(pCog);

    if ( !pThing || !pArgs )
    {
        goto error;
    }

    STD_STRCPY(std_g_genBuffer, pArgs);

    if ( stdConffile_ReadArgsFromStr(std_g_genBuffer) )
    {
        goto error;
    }

    for ( size_t i = 0; i < stdConffile_g_entry.numArgs; ++i )
    {
        StdConffileArg* pArg = &stdConffile_g_entry.aArgs[i];
        if ( sithThing_ParseArg(pArg, sithWorld_g_pCurrentWorld, pThing) )
        {
            STDLOG_ERROR("Cog %s: Argument %s=%s parse failed.\n", pCog->aName, pArg->argName, pArg->argValue);
        }
    }

    return;  // Success

error:
    STDLOG_ERROR("Cog %s: Bad arguments to ParseArg verb.\n", pCog->aName);
}

void J3DAPI sithCogFunctionThing_SetThingRotVel(SithCog* pCog)
{
    rdVector3 vel;
    int bVec          = sithCogExec_PopVector(pCog, &vel);
    SithThing* pThing = sithCogExec_PopThing(pCog);

    if ( !pThing || pThing->moveType != SITH_MT_PHYSICS || !bVec ) // Fixed: Added check for bVec
    {
        STDLOG_ERROR("Cog %s: No thing, or non-physics thing in SetRotVel.\n", pCog->aName);
        return;
    }

    pThing->moveInfo.physics.angularVelocity = vel;

    if ( (pCog->flags & SITHCOG_NOSYNC) == 0 )
    {
        if ( pCog->execMsgType != SITHCOG_MSG_STARTUP && pCog->execMsgType != SITHCOG_MSG_SHUTDOWN )
        {
            sithThing_SyncThing(pThing, SITHTHING_SYNC_POS);
        }
    }
}

void J3DAPI sithCogFunctionThing_GetThingRotVel(SithCog* pCog)
{
    SithThing* pThing = sithCogExec_PopThing(pCog);

    if ( !pThing || pThing->moveType != SITH_MT_PHYSICS )
    {
        STDLOG_ERROR("Cog %s: No thing, or non-physics thing in GetThingRotVel.\n", pCog->aName);
        sithCogExec_PushVector(pCog, &rdroid_g_zeroVector3);
    }

    sithCogExec_PushVector(pCog, &pThing->moveInfo.physics.angularVelocity);
}

void J3DAPI sithCogFunctionThing_SetRotThrust(SithCog* pCog)
{
    rdVector3  thrust;
    int bVec          = sithCogExec_PopVector(pCog, &thrust);
    SithThing* pThing = sithCogExec_PopThing(pCog);

    if ( !pThing || pThing->moveType != SITH_MT_PHYSICS )
    {
        STDLOG_ERROR("Cog %s: No thing, or non-physics thing in SetRotThrust.\n", pCog->aName);
        return;
    }

    // Added: Check for valid vector
    if ( !bVec )
    {
        STDLOG_ERROR("Cog %s: Bad thrust vector in SetRotThrust.\n", pCog->aName);
        return;
    }

    pThing->moveInfo.physics.rotThrust = thrust;

    if ( (pCog->flags & SITHCOG_NOSYNC) == 0 )
    {
        if ( pCog->execMsgType != SITHCOG_MSG_STARTUP && pCog->execMsgType != SITHCOG_MSG_SHUTDOWN )
        {
            sithThing_SyncThing(pThing, SITHTHING_SYNC_POS);
        }
    }
}

void J3DAPI sithCogFunctionThing_GetRotThrust(SithCog* pCog)
{
    SithThing* pThing = sithCogExec_PopThing(pCog);

    if ( !pThing || pThing->moveType != SITH_MT_PHYSICS )
    {
        STDLOG_ERROR("Cog %s: No thing, or non-physics thing in GetRotThrust.\n", pCog->aName);
        sithCogExec_PushVector(pCog, &rdroid_g_zeroVector3);
        return;
    }

    sithCogExec_PushVector(pCog, &pThing->moveInfo.physics.rotThrust);
}

void J3DAPI sithCogFunctionThing_SetThingLook(SithCog* pCog)
{
    rdVector3 look;
    int bValidVec     = sithCogExec_PopVector(pCog, &look);
    SithThing* pThing = sithCogExec_PopThing(pCog);

    if ( !pThing || !bValidVec )
    {
        STDLOG_ERROR("Cog %s: Could not set thing orientation from look vector for SetThingLook().\n", pCog->aName);
        return;
    }

    rdVector_Normalize3Acc(&look);
    rdMatrix_BuildFromLook34(&pThing->orient, &look);

    if ( (pCog->flags & SITHCOG_NOSYNC) == 0 )
    {
        if ( pCog->execMsgType != SITHCOG_MSG_STARTUP && pCog->execMsgType != SITHCOG_MSG_SHUTDOWN )
        {
            sithThing_SyncThing(pThing, SITHTHING_SYNC_POS);
        }
    }
}

int J3DAPI sithCogFunctionThing_SetThingHeadOrientation(SithThing* pThing, const rdVector3* pLook)
{
    if ( !pThing || !pLook )
    {
        return 0;
    }

    if ( pThing->type != SITH_THING_ACTOR && pThing->type != SITH_THING_PLAYER )
    {
        STDLOG_ERROR("Illegal attempt to set thing %s head orientation: thing is not an actor.\n", pThing->aName); // Fixed: Bad var 'Cog %s:' in format
        return 0;
    }

    if ( (pThing->thingInfo.actorInfo.flags & SITH_AF_CANROTATEHEAD) == 0 )
    {
        return 0;
    }

    // Calc Pitch
    rdVector3 dir;
    rdVector_Sub3(&dir, pLook, &pThing->pos);
    rdVector_Normalize3Acc(&dir);

    float pitch = STDMATH_CLAMP(stdMath_ArcSin3(dir.z), pThing->thingInfo.actorInfo.minHeadPitch, pThing->thingInfo.actorInfo.maxHeadPitch);

    // Calc Yaw
    dir.z = 0.0f;
    rdVector_Normalize3Acc(&dir);
    float dot = rdVector_Dot3(&pThing->orient.lvec, &dir);
    float yaw = 90.0f - stdMath_ArcSin3(dot);

    if ( rdVector_Dot3(&pThing->orient.rvec, &dir) > 0 )
    {
        yaw = -yaw;
    }

    yaw = STDMATH_CLAMP(yaw, -35.0f, 35.0f);

    if ( pitch != pThing->thingInfo.actorInfo.headPYR.pitch || yaw != pThing->thingInfo.actorInfo.headPYR.yaw )
    {
        pThing->thingInfo.actorInfo.headPYR.pitch = pitch;
        pThing->thingInfo.actorInfo.headPYR.yaw   = yaw;
        sithActor_SetHeadPYR(pThing, &pThing->thingInfo.actorInfo.headPYR);
        return 1;
    }

    return 0;
}

void J3DAPI sithCogFunctionThing_SetThingHeadLookPos(SithCog* pCog)
{
    rdVector3  look;
    int bValidVec     = sithCogExec_PopVector(pCog, &look);
    SithThing* pThing = sithCogExec_PopThing(pCog);

    if ( !pThing )
    {
        STDLOG_ERROR("Cog %s: Bad thing reference passed to SetThingHeadLook().\n", pCog->aName);
        return;
    }

    if ( !bValidVec )
    {
        STDLOG_ERROR("Cog %s: Bad look vector passed for thing %s in SetThingHeadLook().\n", pCog->aName, pThing->aName);
        return;
    }

    if ( !sithCogFunctionThing_SetThingHeadOrientation(pThing, &look) )
    {
        STDLOG_ERROR("Cog %s: illegal attempt to set thing %s head orientation SetThingHeadLook().\n", pCog->aName, pThing->aName);
    }

    if ( (pCog->flags & SITHCOG_NOSYNC) == 0 )
    {
        if ( pCog->execMsgType != SITHCOG_MSG_STARTUP && pCog->execMsgType != SITHCOG_MSG_SHUTDOWN )
        {
            sithThing_SyncThing(pThing, SITHTHING_SYNC_POS);
        }
    }
}

void J3DAPI sithCogFunctionThing_SetThingHeadLookThing(SithCog* pCog)
{
    SithThing* pLookThing = sithCogExec_PopThing(pCog);
    SithThing* pThing     = sithCogExec_PopThing(pCog);

    if ( !pThing )
    {
        STDLOG_ERROR("Cog %s: Bad thing reference passed to SetThingHeadLookThing().\n", pCog->aName); // Fixed: Fixed function name in error message
        return;
    }

    if ( !pLookThing )
    {
        STDLOG_ERROR("Cog %s: Bad look thing reference passed for thing %s in SetThingHeadLookThing().\n", pCog->aName, pThing->aName); // Fixed: Fixed message mentioning look thing and correct function name
        return;
    }

    if ( !sithCogFunctionThing_SetThingHeadOrientation(pThing, &pLookThing->pos) )
    {
        STDLOG_ERROR("Cog %s: illegal attempt to set thing %s head orientation SetThingHeadLookThing().\n", pCog->aName, pThing->aName); // Fixed: Fixed function name in error message
    }

    if ( (pCog->flags & SITHCOG_NOSYNC) == 0 )
    {
        if ( pCog->execMsgType != SITHCOG_MSG_STARTUP && pCog->execMsgType != SITHCOG_MSG_SHUTDOWN )
        {
            sithThing_SyncThing(pThing, SITHTHING_SYNC_POS);
        }
    }
}

void J3DAPI sithCogFunctionThing_IsThingCrouching(SithCog* pCog)
{
    SithThing* pThing = sithCogExec_PopThing(pCog);
    if ( !pThing || pThing->moveType != SITH_MT_PHYSICS )
    {
        STDLOG_ERROR("Cog %s: Bad thing reference passed to IsThingCrouching().\n", pCog->aName); // Added: Error message
        sithCogExec_PushInt(pCog, -1);
        return; // Fixed: Was missing return, fixes potential dereferencing NULL pointer
    }

    if ( (pThing->moveInfo.physics.flags & SITH_PF_CROUCHING) != 0 )
    {
        sithCogExec_PushInt(pCog, 1);
    }
    else
    {
        sithCogExec_PushInt(pCog, 0);
    }
}

void J3DAPI sithCogFunctionThing_GetThingClassCog(SithCog* pCog)
{
    SithThing* pThing = sithCogExec_PopThing(pCog);

    if ( !pThing || !pThing->pCog )
    {
        // Added: Error message
        if ( !pThing )
        {
            STDLOG_ERROR("Cog %s: Bad thing reference passed to GetThingClassCog().\n", pCog->aName);
        }

        sithCogExec_PushInt(pCog, -1);
        return;
    }

    sithCogExec_PushInt(pCog, pThing->pCog->idx);
}

void J3DAPI sithCogFunctionThing_SetThingClassCog(SithCog* pCog)
{
    SithCog* pThingCog = sithCogExec_PopCog(pCog);
    SithThing* pThing  = sithCogExec_PopThing(pCog);

    if ( !pThing )
    {
        STDLOG_ERROR("Cog %s: No thing, in SetThingClassCog.\n", pCog->aName);
        return;
    }

    if ( !pThingCog )
    {
        STDLOG_ERROR("Cog %s: No cog, in SetThingClassCog.\n", pCog->aName);
        return;
    }

    pThing->pCog = pThingCog;
}

void J3DAPI sithCogFunctionThing_GetThingCaptureCog(SithCog* pCog)
{
    SithThing* pThing = sithCogExec_PopThing(pCog);

    if ( !pThing || !pThing->pCaptureCog )
    {
         // Added: Error message
        if ( !pThing )
        {
            STDLOG_ERROR("Cog %s: Bad thing reference passed to GetThingCaptureCog().\n", pCog->aName);
        }
        sithCogExec_PushInt(pCog, -1);
        return;
    }

    sithCogExec_PushInt(pCog, pThing->pCaptureCog->idx);
}

void J3DAPI sithCogFunctionThing_SetThingCaptureCog(SithCog* pCog)
{
    SithCog* pCaptureCog = sithCogExec_PopCog(pCog);
    SithThing* pThing    = sithCogExec_PopThing(pCog);

    if ( !pThing )
    {
        STDLOG_ERROR("Cog %s: No thing, in SetThingCaptureCog.\n", pCog->aName);
        return;
    }

    if ( !pCaptureCog )
    {
        STDLOG_ERROR("Cog %s: No cog, in SetThingCaptureCog.\n", pCog->aName);
        return;
    }

    pThing->pCaptureCog = pCaptureCog;
}

void J3DAPI sithCogFunctionThing_GetThingRespawn(SithCog* pCog)
{
    SithThing* pThing = sithCogExec_PopThing(pCog);

    if ( !pThing || pThing->type != SITH_THING_ITEM )
    {
        STDLOG_ERROR("Cog %s: Illegal thing, in GetThingRespawn.\n", pCog->aName);
        sithCogExec_PushFlex(pCog, -1.0f); // Fixed: Added return value pushed to execution stack
        return;
    }

    sithCogExec_PushFlex(pCog, pThing->thingInfo.itemInfo.secRespawnInterval);
}

void J3DAPI sithCogFunctionThing_GetThingSignature(SithCog* pCog)
{
    SithThing* pThing = sithCogExec_PopThing(pCog);

    if ( !pThing )
    {
        STDLOG_ERROR("Cog %s: Bad thing in GetThingSignature.\n", pCog->aName); // Added: Error message
        sithCogExec_PushInt(pCog, -1);
        return;
    }

    sithCogExec_PushInt(pCog, pThing->signature);
}

void J3DAPI sithCogFunctionThing_SetThingAttachFlags(SithCog* pCog)
{
    int attflags      = sithCogExec_PopInt(pCog);
    SithThing* pThing = sithCogExec_PopThing(pCog);

    if ( !pThing || attflags == 0 )
    {
        STDLOG_ERROR("Cog %s: Cannot set thing attach flags.\n", pCog->aName);
        return;
    }

    pThing->attach.flags |= attflags;

    if ( (pCog->flags & SITHCOG_NOSYNC) == 0 )
    {
        if ( pCog->execMsgType != SITHCOG_MSG_STARTUP && pCog->execMsgType != SITHCOG_MSG_SHUTDOWN )
        {
            sithDSSThing_Attachment(pThing, SITHMESSAGE_SENDTOJOINEDPLAYERS, SITHMESSAGE_STREAM_ALL, DPSEND_GUARANTEED);
        }
    }
}

void J3DAPI sithCogFunctionThing_ClearThingAttachFlags(SithCog* pCog)
{
    int attflags      = sithCogExec_PopInt(pCog);
    SithThing* pThing = sithCogExec_PopThing(pCog);

    if ( !pThing || attflags == 0 )
    {
        STDLOG_ERROR("Cog %s: Cannot clear thing attach flags.\n", pCog->aName);
        return;
    }

    pThing->attach.flags &= ~attflags;

    if ( (pCog->flags & SITHCOG_NOSYNC) == 0 )
    {
        if ( pCog->execMsgType != SITHCOG_MSG_STARTUP && pCog->execMsgType != SITHCOG_MSG_SHUTDOWN )
        {
            sithDSSThing_Attachment(pThing, SITHMESSAGE_SENDTOJOINEDPLAYERS, SITHMESSAGE_STREAM_ALL, DPSEND_GUARANTEED);
        }
    }
}

void J3DAPI sithCogFunctionThing_GetParticleSize(SithCog* pCog)
{
    SithThing* pThing = sithCogExec_PopThing(pCog);

    if ( !pThing )
    {
        STDLOG_ERROR("Cog %s: Bad thing in GetParticleSize.\n", pCog->aName);
        sithCogExec_PushFlex(pCog, -1.0f);
        return;
    }

    if ( pThing->type != SITH_THING_PARTICLE )
    {
        STDLOG_ERROR("Cog %s: Cannot get thing Particle size for non particle things.\n", pCog->aName);
        sithCogExec_PushFlex(pCog, -1.0f);
        return;
    }

    sithCogExec_PushFlex(pCog, pThing->thingInfo.particleInfo.size);
}

void J3DAPI sithCogFunctionThing_SetParticleSize(SithCog* pCog)
{
    float size        = sithCogExec_PopFlex(pCog);
    SithThing* pThing = sithCogExec_PopThing(pCog);

    if ( !pThing )
    {
        STDLOG_ERROR("Cog %s: Bad thing in SetParticleSize.\n", pCog->aName);
        return;
    }

    if ( pThing->type != SITH_THING_PARTICLE )
    {
        STDLOG_ERROR("Cog %s: Cannot set thing particle size for non particle things.\n", pCog->aName);
        return;
    }

    pThing->thingInfo.particleInfo.size = size;
}

void J3DAPI sithCogFunctionThing_GetParticleGrowthSpeed(SithCog* pCog)
{
    SithThing* pThing = sithCogExec_PopThing(pCog);

    if ( !pThing )
    {
        STDLOG_ERROR("Cog %s: Bad thing in GetParticleGrowthSpeed.\n", pCog->aName);
        sithCogExec_PushFlex(pCog, -1.0f);
        return;
    }

    if ( pThing->type != SITH_THING_PARTICLE )
    {
        STDLOG_ERROR("Cog %s: Cannot get thing particle GrowthSpeed for non particle things.\n", pCog->aName);
        sithCogExec_PushFlex(pCog, -1.0f);
        return;
    }

    sithCogExec_PushFlex(pCog, pThing->thingInfo.particleInfo.growthSpeed);
}

void J3DAPI sithCogFunctionThing_SetParticleGrowthSpeed(SithCog* pCog)
{
    float speed       = sithCogExec_PopFlex(pCog);
    SithThing* pThing = sithCogExec_PopThing(pCog);

    if ( !pThing )
    {
        STDLOG_ERROR("Cog %s: Bad thing in SetParticleGrowthSpeed.\n", pCog->aName);
        return;
    }

    if ( pThing->type != SITH_THING_PARTICLE )
    {
        STDLOG_ERROR("Cog %s: Cannot set thing particle growth speed for non particle things.\n", pCog->aName); // Fixed: Fixed typo in error message
        return;
    }

    pThing->thingInfo.particleInfo.growthSpeed = speed;
}

void J3DAPI sithCogFunctionThing_GetParticleTimeoutRate(SithCog* pCog)
{
    SithThing* pThing = sithCogExec_PopThing(pCog);

    if ( !pThing )
    {
        STDLOG_ERROR("Cog %s: Bad thing in GetParticleTimeoutRate.\n", pCog->aName);
        sithCogExec_PushFlex(pCog, -1.0f);
        return;
    }

    if ( pThing->type != SITH_THING_PARTICLE )
    {
        STDLOG_ERROR("Cog %s: Cannot get thing particle timeout rate for non particle things.\n", pCog->aName); // Fixed: Fixed typo in error message
        sithCogExec_PushFlex(pCog, -1.0f);
        return;
    }

    sithCogExec_PushFlex(pCog, pThing->thingInfo.particleInfo.timeoutRate);
}

void J3DAPI sithCogFunctionThing_SetParticleTimeoutRate(SithCog* pCog)
{
    float timeoutRate = sithCogExec_PopFlex(pCog);
    SithThing* pThing = sithCogExec_PopThing(pCog);

    if ( !pThing )
    {
        STDLOG_ERROR("Cog %s: Bad thing in SetParticleTimeoutRate.\n", pCog->aName);
        return;
    }

    if ( pThing->type != SITH_THING_PARTICLE )
    {
        STDLOG_ERROR("Cog %s: Cannot set thing particle timeout rate for non particle things.\n", pCog->aName); // Fixed: Fixed typo in error message
        return;
    }

    pThing->thingInfo.particleInfo.timeoutRate = timeoutRate;
}

void J3DAPI sithCogFunctionThing_GetTypeFlags(SithCog* pCog)
{
    SithThing* pThing = sithCogExec_PopThing(pCog);

    if ( !pThing )
    {
        STDLOG_ERROR("Cog %s: Bad thing in GetTypeFlags.\n", pCog->aName);
        sithCogExec_PushInt(pCog, -1);
        return;
    }

    switch ( pThing->type )
    {
        case SITH_THING_ACTOR:
        case SITH_THING_WEAPON:
        case SITH_THING_ITEM:
        case SITH_THING_EXPLOSION:
        case SITH_THING_PLAYER:
        case SITH_THING_PARTICLE:
            sithCogExec_PushInt(pCog, pThing->thingInfo.actorInfo.flags);
            break;

        default:
            STDLOG_ERROR("Cog %s: Cannot get thing type flags for this type of thing.\n", pCog->aName);
            sithCogExec_PushInt(pCog, -1);
            break;
    }
}

void J3DAPI sithCogFunctionThing_SetTypeFlags(SithCog* pCog)
{
    int typeflags     = sithCogExec_PopInt(pCog);
    SithThing* pThing = sithCogExec_PopThing(pCog);

    if ( !pThing || typeflags == 0 )
    {
        STDLOG_ERROR("Cog %s: Bad thing in SetTypeFlags.\n", pCog->aName);
        return;
    }

    switch ( pThing->type )
    {
        case SITH_THING_ACTOR:
        case SITH_THING_WEAPON:
        case SITH_THING_ITEM:
        case SITH_THING_EXPLOSION:
        case SITH_THING_PLAYER:
        case SITH_THING_PARTICLE:
            pThing->thingInfo.actorInfo.flags |= typeflags;
            break;

        default:
            STDLOG_ERROR("Cog %s: Cannot set thing type flags for this type of thing.\n", pCog->aName);
            return; // Fixed: Added return to prevent further execution
    }

    if ( (pCog->flags & SITHCOG_NOSYNC) == 0 )
    {
        if ( pCog->execMsgType != SITHCOG_MSG_STARTUP && pCog->execMsgType != SITHCOG_MSG_SHUTDOWN )
        {
            sithThing_SyncThing(pThing, SITHTHING_SYNC_STATE);
        }
    }
}

void J3DAPI sithCogFunctionThing_ClearTypeFlags(SithCog* pCog)
{
    int typeflags     = sithCogExec_PopInt(pCog);
    SithThing* pThing = sithCogExec_PopThing(pCog);

    if ( !pThing || !typeflags )
    {
        STDLOG_ERROR("Cog %s: Bad thing in ClearTypeFlags.\n", pCog->aName);
        return;
    }

    switch ( pThing->type )
    {
        case SITH_THING_ACTOR:
        case SITH_THING_WEAPON:
        case SITH_THING_ITEM:
        case SITH_THING_EXPLOSION:
        case SITH_THING_PLAYER:
        case SITH_THING_PARTICLE:
            pThing->thingInfo.actorInfo.flags &= ~typeflags;
            break;

        default:
            STDLOG_ERROR("Cog %s: Cannot clear thing type flags for this type of thing.\n", pCog->aName);
            return; // Fixed: Added return to prevent further execution
    }

    if ( (pCog->flags & SITHCOG_NOSYNC) == 0 )
    {
        if ( pCog->execMsgType != SITHCOG_MSG_STARTUP && pCog->execMsgType != SITHCOG_MSG_SHUTDOWN )
        {
            sithThing_SyncThing(pThing, SITHTHING_SYNC_STATE);
        }
    }
}

void J3DAPI sithCogFunctionThing_TakeItem(SithCog* pCog)
{
    SithThing* pThing = sithCogExec_PopThing(pCog);
    SithThing* pItem  = sithCogExec_PopThing(pCog);

    if ( !pItem || !pThing || pItem->type != SITH_THING_ITEM )
    {
        STDLOG_ERROR("Cog %s: Bad parameters in TakeItem.\n", pCog->aName); // Added: Error message
        return;
    }

    sithItem_SetItemTaken(pItem, pThing, /*bNoMultiSync=*/0);
}

void J3DAPI sithCogFunctionThing_HasLOS(SithCog* pCog)
{
    SithThing* pTarget = sithCogExec_PopThing(pCog);
    SithThing* pViewer = sithCogExec_PopThing(pCog);

    if ( !pViewer || !pTarget )
    {
        STDLOG_ERROR("Cog %s: Bad thing in HasLOS.\n", pCog->aName);
        sithCogExec_PushInt(pCog, -1);
        return;
    }

    if ( sithCollision_CheckThingLOS(pViewer, pTarget, 0) )
    {
        sithCogExec_PushInt(pCog, 1);
    }
    else
    {
        sithCogExec_PushInt(pCog, 0);
    }
}

void J3DAPI sithCogFunctionThing_GetFireOffset(SithCog* pCog)
{
    SithThing* pThing = sithCogExec_PopThing(pCog);

    if ( !pThing )
    {
        STDLOG_ERROR("Cog %s: Bad thing in GetFireOffset.\n", pCog->aName);
        sithCogExec_PushVector(pCog, &rdroid_g_zeroVector3);
        return;
    }

    sithCogExec_PushVector(pCog, &pThing->thingInfo.actorInfo.fireOffset);
}

void J3DAPI sithCogFunctionThing_SetFireOffset(SithCog* pCog)
{
    rdVector3 offset;
    int bVec          = sithCogExec_PopVector(pCog, &offset);
    SithThing* pThing = sithCogExec_PopThing(pCog);

    if ( !pThing )
    {
        STDLOG_ERROR("Cog %s: Bad thing in SetFireOffset.\n", pCog->aName);
        return;
    }

    // Added: bVec check
    if ( !bVec )
    {
        STDLOG_ERROR("Cog %s: Bad vector in SetFireOffset.\n", pCog->aName);
        return;
    }

    pThing->thingInfo.actorInfo.fireOffset = offset;
}

void J3DAPI sithCogFunctionThing_GetThingUserData(SithCog* pCog)
{
    SithThing* pThing = sithCogExec_PopThing(pCog);

    if ( !pThing )
    {
        STDLOG_ERROR("Cog %s: Bad thing in GetThingUserData.\n", pCog->aName);
        sithCogExec_PushFlex(pCog, -1.0f);
        return;
    }

    sithCogExec_PushFlex(pCog, pThing->userval);
}

void J3DAPI sithCogFunctionThing_SetThingUserData(SithCog* pCog)
{
    float userval     = sithCogExec_PopFlex(pCog);
    SithThing* pThing = sithCogExec_PopThing(pCog);

    if ( !pThing )
    {
        STDLOG_ERROR("Cog %s: Bad thing in SetThingUserData.\n", pCog->aName);
        return;
    }

    pThing->userval = userval;
}

void J3DAPI sithCogFunctionThing_GetThingCollideSize(SithCog* pCog)
{
    SithThing* pThing = sithCogExec_PopThing(pCog);

    if ( !pThing )
    {
        STDLOG_ERROR("Cog %s: Bad thing in GetThingCollideSize.\n", pCog->aName);
        sithCogExec_PushFlex(pCog, -1.0f);
        return;
    }

    sithCogExec_PushFlex(pCog, pThing->collide.size);
}

void J3DAPI sithCogFunctionThing_SetThingCollideSize(SithCog* pCog)
{
    float size        = sithCogExec_PopFlex(pCog);
    SithThing* pThing = sithCogExec_PopThing(pCog);

    if ( !pThing )
    {
        STDLOG_ERROR("Cog %s: Bad thing in SetThingCollideSize.\n", pCog->aName);
        return;
    }
    pThing->collide.size = size;
}

void J3DAPI sithCogFunctionThing_GetThingMoveSize(SithCog* pCog)
{
    SithThing* pThing = sithCogExec_PopThing(pCog);

    if ( !pThing )
    {
        STDLOG_ERROR("Cog %s: Bad thing in GetThingMoveSize.\n", pCog->aName);
        sithCogExec_PushFlex(pCog, -1.0f);
        return;
    }

    sithCogExec_PushFlex(pCog, pThing->collide.movesize);
}

void J3DAPI sithCogFunctionThing_SetThingMoveSize(SithCog* pCog)
{
    float movesize      = sithCogExec_PopFlex(pCog);
    SithThing* pThing   = sithCogExec_PopThing(pCog);

    if ( !pThing )
    {
        STDLOG_ERROR("Cog %s: Bad thing in SetThingMoveSize.\n", pCog->aName);
        return;
    }

    pThing->collide.movesize = movesize;
}

void J3DAPI sithCogFunctionThing_GetThingMass(SithCog* pCog)
{
    SithThing* pThing = sithCogExec_PopThing(pCog);

    if ( !pThing )
    {
        STDLOG_ERROR("Cog %s: Bad thing in GetThingMass.\n", pCog->aName); // Added: Error message
        sithCogExec_PushFlex(pCog, 0.0f); // Fixed: Added return value pushed to execution stack
        return;
    }

    if ( pThing->moveType != SITH_MT_PHYSICS )
    {
        STDLOG_ERROR("Cog %s: Non physics thing in GetThingMass.\n", pCog->aName);
        sithCogExec_PushFlex(pCog, 0.0f);
        return;
    }

    sithCogExec_PushFlex(pCog, pThing->moveInfo.physics.mass);
}

void J3DAPI sithCogFunctionThing_SetThingMass(SithCog* pCog)
{
    float mass        = sithCogExec_PopFlex(pCog);
    SithThing* pThing = sithCogExec_PopThing(pCog);

    if ( !pThing )
    {
        STDLOG_ERROR("Cog %s: Bad thing in SetThingMass.\n", pCog->aName); // Added: Error message
        return;
    }

    if ( pThing->moveType != SITH_MT_PHYSICS )
    {
        STDLOG_ERROR("Cog %s: Non physics thing in SetThingMass.\n", pCog->aName);
        return;
    }

    pThing->moveInfo.physics.mass = mass;

    if ( (pCog->flags & SITHCOG_NOSYNC) == 0 )
    {
        if ( pCog->execMsgType != SITHCOG_MSG_STARTUP && pCog->execMsgType != SITHCOG_MSG_SHUTDOWN )
        {
            sithThing_SyncThing(pThing, SITHTHING_SYNC_STATE);
        }
    }
}

void J3DAPI sithCogFunctionThing_SyncThingPos(SithCog* pCog)
{
    SithThing* pThing = sithCogExec_PopThing(pCog);

    if ( !pThing )
    {
        STDLOG_ERROR("Cog %s: Bad thing in SyncThingPos().\n", pCog->aName);
        return;
    }

    sithThing_SyncThing(pThing, SITHTHING_SYNC_POS);
}

void J3DAPI sithCogFunctionThing_SyncThingAttachment(SithCog* pCog)
{
    SithThing* pThing = sithCogExec_PopThing(pCog);

    if ( !pThing )
    {
        STDLOG_ERROR("Cog %s: Bad thing in SyncThingAttachment().\n", pCog->aName);
        return;
    }

    sithDSSThing_Attachment(pThing, SITHMESSAGE_SENDTOJOINEDPLAYERS, SITHMESSAGE_STREAM_ALL, 0x00);
}

void J3DAPI sithCogFunctionThing_SyncThingState(SithCog* pCog)
{
    SithThing* pThing = sithCogExec_PopThing(pCog);

    if ( !pThing )
    {
        STDLOG_ERROR("Cog %s: Bad thing in SyncThingState().\n", pCog->aName);
        return;
    }

    sithThing_SyncThing(pThing, SITHTHING_SYNC_STATE);
}

void J3DAPI sithCogFunctionThing_GetMajorMode(SithCog* pCog)
{
    SithThing* pThing = sithCogExec_PopThing(pCog);

    if ( !pThing )
    {
        STDLOG_ERROR("Cog %s: No thing in GetMajorMode().\n", pCog->aName);
        sithCogExec_PushInt(pCog, -1);
        return;
    }

    if ( !pThing->pPuppetClass || !pThing->renderData.pPuppet )
    {
        STDLOG_ERROR("Cog %s: No puppet on thing %s for GetMajorMode().\n", pCog->aName, pThing->aName);
        sithCogExec_PushInt(pCog, -1);
        return;
    }

    sithCogExec_PushInt(pCog, pThing->pPuppetState->majorMode);
}

void J3DAPI sithCogFunctionThing_SetThingPosEx(SithCog* pCog)
{
    rdVector3 newPos;
    SithSector* pSector = sithCogExec_PopSector(pCog);
    int bVec            = sithCogExec_PopVector(pCog, &newPos);
    SithThing* pThing   = sithCogExec_PopThing(pCog);

    if ( !pThing || !bVec ) // Fixed: Add check for bVec, and moved pThing check above of searching for sector
    {
        sithCogExec_PushInt(pCog, 0);
        return;
    }

    if ( !pSector )
    {
        pSector = sithSector_FindSectorAtPos(sithWorld_g_pCurrentWorld, &newPos);
    }

    // Set thing position now
    pThing->pos = newPos;
    sithThing_SetSector(pThing, pSector, /*bNotify=*/0);

    if ( pThing->moveType == SITH_MT_PHYSICS && (pThing->moveInfo.physics.flags & SITH_PF_FLOORSTICK) != 0 )
    {
        sithPhysics_FindFloor(pThing, /*bNoThingStateUpdate=*/1);
    }

    if ( pThing == sithPlayer_g_pLocalPlayerThing )
    {
        sithCamera_Update(sithCamera_g_pCurCamera);
    }

    if ( (pCog->flags & SITHCOG_NOSYNC) == 0 )
    {
        if ( pCog->execMsgType != SITHCOG_MSG_STARTUP && pCog->execMsgType != SITHCOG_MSG_SHUTDOWN )
        {
            sithDSSThing_Pos(pThing, SITHMESSAGE_SENDTOJOINEDPLAYERS, DPSEND_GUARANTEED);
        }
    }

    sithCogExec_PushInt(pCog, 1);
}

void J3DAPI sithCogFunctionThing_GetThingGuid(SithCog* pCog)
{
    SithThing* pThing = sithCogExec_PopThing(pCog);

    if ( !pThing )
    {
        sithCogExec_PushInt(pCog, -1);
        return;
    }

    sithCogExec_PushInt(pCog, pThing->guid);
}

void J3DAPI sithCogFunctionThing_GetGuidThing(SithCog* pCog)
{
    int guid = sithCogExec_PopInt(pCog);

    SithThing* pThing = sithThing_GetGuidThing(guid);
    if ( !pThing )
    {
        STDLOG_ERROR("Cog %s: Invalid GUID in GetGuidThing!\n", pCog->aName);
        sithCogExec_PushInt(pCog, -1);
        return;
    }

    sithCogExec_PushInt(pCog, pThing->idx);
}

void J3DAPI sithCogFunctionThing_GetThingMaxVel(SithCog* pCog)
{
    SithThing* pThing = sithCogExec_PopThing(pCog);

    if ( !pThing )
    {
        STDLOG_ERROR("Cog %s: Bad thing in GetThingMaxVel!\n", pCog->aName); // Added: Error log
        sithCogExec_PushFlex(pCog, 0.0f); // Fixed: Added missing push of return value to stack
        return;
    }

    if ( pThing->moveType == SITH_MT_PHYSICS )
    {
        sithCogExec_PushFlex(pCog, pThing->moveInfo.physics.maxVelocity);
    }
    else
    {
        STDLOG_ERROR("Cog %s: Non physics thing in GetThingMaxVel!\n", pCog->aName);
        sithCogExec_PushFlex(pCog, 0.0f);
    }
}

void J3DAPI sithCogFunctionThing_SetThingMaxVel(SithCog* pCog)
{
    float maxSpeed    = sithCogExec_PopFlex(pCog);
    SithThing* pThing = sithCogExec_PopThing(pCog);

    if ( !pThing )
    {
        STDLOG_ERROR("Cog %s: Bad thing in SetThingMaxVel!\n", pCog->aName); // Added: Error log
        return;
    }

    if ( pThing->moveType == SITH_MT_PHYSICS )
    {
        pThing->moveInfo.physics.maxVelocity = maxSpeed;
    }
    else
    {
        STDLOG_ERROR("Cog %s: Non physics thing in SetThingMaxVel!\n", pCog->aName);
    }
}

void J3DAPI sithCogFunctionThing_GetThingMaxAngVel(SithCog* pCog)
{
    SithThing* pThing = sithCogExec_PopThing(pCog);

    if ( !pThing )
    {
        STDLOG_ERROR("Cog %s: Bad thing in GetThingMaxAngVel!\n", pCog->aName); // Added: Error log
        sithCogExec_PushFlex(pCog, 0.0f); // Fixed: Added missing push of return value to stack
        return;
    }

    if ( pThing->moveType == SITH_MT_PHYSICS )
    {
        sithCogExec_PushFlex(pCog, pThing->moveInfo.physics.maxRotationVelocity);
    }
    else
    {
        STDLOG_ERROR("Cog %s: Non physics thing in GetThingMaxAngVel!\n", pCog->aName);
        sithCogExec_PushFlex(pCog, 0.0f);
    }
}

void J3DAPI sithCogFunctionThing_SetThingMaxAngVel(SithCog* pCog)
{
    float maxRotSpeed = sithCogExec_PopFlex(pCog);
    SithThing* pThing = sithCogExec_PopThing(pCog);

    if ( !pThing )
    {
        STDLOG_ERROR("Cog %s: Bad thing in SetThingMaxAngVel!\n", pCog->aName); // Added: Error log
        return;
    }

    if ( pThing->moveType == SITH_MT_PHYSICS )
    {
        pThing->moveInfo.physics.maxRotationVelocity = maxRotSpeed;
    }
    else
    {
        STDLOG_ERROR("Cog %s: Non physics thing in SetThingMaxAngVel!\n", pCog->aName);
    }
}

void J3DAPI sithCogFunctionThing_SetJointAngle(SithCog* pCog)
{
    float angle       = sithCogExec_PopFlex(pCog);
    size_t jointNum   = sithCogExec_PopInt(pCog);
    SithThing* pThing = sithCogExec_PopThing(pCog);

    if ( !pThing )
    {
        STDLOG_ERROR("Cog %s: Bad thing in SetJointAngle.\n", pCog->aName);
        return;
    }

    SITH_ASSERTREL(pThing && ((pThing->type == SITH_THING_ACTOR) || (pThing->type == SITH_THING_PLAYER) || (pThing->type == SITH_THING_CORPSE)));

    if ( !pThing->pPuppetClass )
    {
        STDLOG_ERROR("Cog %s: Thing %s has no puppet in SetJointAngle.\n", pCog->aName, pThing->aName); // Added: Error log
        return;
    }

    if ( pThing->renderData.type != RD_THING_MODEL3 )
    {
        STDLOG_ERROR("Cog %s: Thing %s has no model in SetJointAngle.\n", pCog->aName, pThing->aName); // Added: Error log
        return;
    }

    if ( !pThing->renderData.apTweakedAngles )
    {
        STDLOG_ERROR("Cog %s: Bad thing %s in SetJointAngle.\n", pCog->aName, pThing->aName); // Added: Error log
        return;
    }

    // Fixed: Added check for jointNum to prevent OOB access
    if ( jointNum >= STD_ARRAYLEN(pThing->pPuppetClass->aJoints) )
    {
        STDLOG_ERROR("Cog %s: Bad joint number in SetJointAngle.\n", pCog->aName);
        return;
    }

    int nodeNum = pThing->pPuppetClass->aJoints[jointNum];
    if ( nodeNum >= 0 && nodeNum < pThing->renderData.data.pModel3->numHNodes )
    {
        pThing->renderData.apTweakedAngles[nodeNum].pitch = angle;
    }
}

void J3DAPI sithCogFunctionThing_GetJointAngle(SithCog* pCog)
{
    size_t jointNum   = sithCogExec_PopInt(pCog);
    SithThing* pThing = sithCogExec_PopThing(pCog);

    if ( !pThing )
    {
        STDLOG_ERROR("Cog %s: Bad thing in GetJointAngle.\n", pCog->aName);
        sithCogExec_PushFlex(pCog, -1.0f); // Fixed: Added missing push of return value to stack
        return;
    }

    SITH_ASSERTREL(pThing && ((pThing->type == SITH_THING_ACTOR) || (pThing->type == SITH_THING_PLAYER) || (pThing->type == SITH_THING_CORPSE)));

    if ( !pThing->pPuppetClass )
    {
        STDLOG_ERROR("Cog %s: Thing %s has no puppet in GetJointAngle.\n", pCog->aName, pThing->aName); // Added: Error log
        sithCogExec_PushFlex(pCog, -1.0f);
        return;
    }

    if ( pThing->renderData.type != RD_THING_MODEL3 )
    {
        STDLOG_ERROR("Cog %s: Thing %s has no model in GetJointAngle.\n", pCog->aName, pThing->aName); // Added: Error log
        sithCogExec_PushFlex(pCog, -1.0f);
        return;
    }

    if ( !pThing->renderData.apTweakedAngles )
    {
        STDLOG_ERROR("Cog %s: Bad thing %s in GetJointAngle.\n", pCog->aName, pThing->aName); // Added: Error log
        sithCogExec_PushFlex(pCog, -1.0f);
        return;
    }

    // Fixed: Added check for jointNum to prevent OOB access
    if ( jointNum >= STD_ARRAYLEN(pThing->pPuppetClass->aJoints) )
    {
        STDLOG_ERROR("Cog %s: Bad joint number in GetJointAngle.\n", pCog->aName);
        sithCogExec_PushFlex(pCog, -1.0f);
        return;
    }

    int nodeNum = pThing->pPuppetClass->aJoints[jointNum];
    if ( nodeNum >= 0 && nodeNum < pThing->renderData.data.pModel3->numHNodes )
    {
        float pitch = pThing->renderData.apTweakedAngles[nodeNum].pitch;
        sithCogExec_PushFlex(pCog, pitch);
    }
    else
    {
        sithCogExec_PushFlex(pCog, -1.0f);
    }
}

void J3DAPI sithCogFunctionThing_SetMaxHeadPitch(SithCog* pCog)
{
    float maxPitch    = sithCogExec_PopFlex(pCog);
    SithThing* pThing = sithCogExec_PopThing(pCog);

    if ( !pThing )
    {
        STDLOG_ERROR("Cog %s: Bad thing in SetMaxHeadPitch.\n", pCog->aName);
        sithCogExec_PushFlex(pCog, -1.0f); // Fixed: Added missing push of return value to stack
        return;
    }

    sithCogExec_PushFlex(pCog, pThing->thingInfo.actorInfo.maxHeadPitch);
    pThing->thingInfo.actorInfo.maxHeadPitch = maxPitch;
}

void J3DAPI sithCogFunctionThing_SetMinHeadPitch(SithCog* pCog)
{
    float minPitch    = sithCogExec_PopFlex(pCog);
    SithThing* pThing = sithCogExec_PopThing(pCog);

    if ( !pThing )
    {
        STDLOG_ERROR("Cog %s: Bad thing in SetMinHeadPitch.\n", pCog->aName);
        sithCogExec_PushFlex(pCog, -1.0f); // Fixed: Added missing push of return value to stack
        return;
    }

    sithCogExec_PushFlex(pCog, pThing->thingInfo.actorInfo.minHeadPitch);
    pThing->thingInfo.actorInfo.minHeadPitch = minPitch;
}

void J3DAPI sithCogFunctionThing_GetThingMaxHeadPitch(SithCog* pCog)
{
    SithThing* pThing = sithCogExec_PopThing(pCog);

    if ( !pThing )
    {
        STDLOG_ERROR("Cog %s: Ain't no thing in GetThingMaxHeadPitch().\n", pCog->aName);
        sithCogExec_PushFlex(pCog, -1.0f);
        return;
    }

    if ( pThing->type != SITH_THING_PLAYER && pThing->type != SITH_THING_ACTOR )
    {
        STDLOG_ERROR("Cog %s: Bad thing type in GetThingMaxHeadPitch().\n", pCog->aName);
        sithCogExec_PushFlex(pCog, -1.0f);
        return;
    }

    sithCogExec_PushFlex(pCog, pThing->thingInfo.actorInfo.maxHeadPitch);
}

void J3DAPI sithCogFunctionThing_GetThingMinHeadPitch(SithCog* pCog)
{
    SithThing* pThing = sithCogExec_PopThing(pCog);

    if ( !pThing )
    {
        STDLOG_ERROR("Cog %s: Ain't no thing in GetThingMinHeadPitch().\n", pCog->aName);
        sithCogExec_PushFlex(pCog, -1.0);
        return;
    }

    if ( pThing->type != SITH_THING_PLAYER && pThing->type != SITH_THING_ACTOR )
    {
        STDLOG_ERROR("Cog %s: Bad thing type in GetThingMinHeadPitch().\n", pCog->aName);
        sithCogExec_PushFlex(pCog, -1.0);
        return;
    }

    sithCogExec_PushFlex(pCog, pThing->thingInfo.actorInfo.minHeadPitch);
}

void J3DAPI sithCogFunctionThing_SetMaxHeadYaw(SithCog* pCog)
{
    float maxYaw      = sithCogExec_PopFlex(pCog);
    SithThing* pThing = sithCogExec_PopThing(pCog);

    if ( !pThing )
    {
        STDLOG_ERROR("Cog %s: Bad thing in SetMaxHeadYaw.\n", pCog->aName);
        sithCogExec_PushFlex(pCog, -1.0f); // Fixed: Added missing push of return value to stack
        return;
    }

    sithCogExec_PushFlex(pCog, pThing->thingInfo.actorInfo.maxHeadYaw);
    pThing->thingInfo.actorInfo.maxHeadYaw = maxYaw;
}

void J3DAPI sithCogFunctionThing_GetThingMaxHeadYaw(SithCog* pCog)
{
    SithThing* pThing = sithCogExec_PopThing(pCog);

    if ( !pThing )
    {
        STDLOG_ERROR("Cog %s: Ain't no thing in GetThingMaxHeadYaw().\n", pCog->aName);
        sithCogExec_PushFlex(pCog, -1.0);
        return;
    }

    if ( pThing->type != SITH_THING_PLAYER && pThing->type != SITH_THING_ACTOR )
    {
        STDLOG_ERROR("Cog %s: Bad thing type in GetThingMaxHeadYaw().\n", pCog->aName);
        sithCogExec_PushFlex(pCog, -1.0);
        return;
    }

    sithCogExec_PushFlex(pCog, pThing->thingInfo.actorInfo.maxHeadYaw);
}

void J3DAPI sithCogFunctionThing_InterpolatePYR(SithCog* pCog)
{
    rdVector3 axisX, axisY, axisZ;

    float angle = sithCogExec_PopFlex(pCog);
    sithCogExec_PopVector(pCog, &axisZ);
    sithCogExec_PopVector(pCog, &axisY);
    sithCogExec_PopVector(pCog, &axisX);

    // TODO: Check for errors

    // Get pyr1
    rdVector3 look, pyr1;
    rdVector_Sub3(&look, &axisY, &axisX);
    rdVector_Normalize3Acc(&look);

    rdMatrix34 orient;
    rdMatrix_BuildFromLook34(&orient, &look);
    rdMatrix_ExtractAngles34(&orient, &pyr1);

    // Get pyr2
    rdVector3 pyr2;
    rdVector_Sub3(&look, &axisZ, &axisX);
    rdVector_Normalize3Acc(&look);

    rdMatrix_BuildFromLook34(&orient, &look);
    rdMatrix_ExtractAngles34(&orient, &pyr2);

    rdVector_Lerp3(&look, &pyr1, &pyr2, angle);
    sithCogExec_PushVector(pCog, &look);
}

void J3DAPI sithCogFunctionThing_GetThingLVecPYR(SithCog* pCog)
{
    SithThing* pThing = sithCogExec_PopThing(pCog);

    if ( !pThing )
    {
        STDLOG_ERROR("Cog %s: Bad thing in SetMaxHeadYaw.\n", pCog->aName); // Added: Error log
        sithCogExec_PushVector(pCog, &rdroid_g_zeroVector3);
        return;
    }

    rdVector3 pyr;
    rdMatrix_ExtractAngles34(&pThing->orient, &pyr);
    sithCogExec_PushVector(pCog, &pyr);
}

void J3DAPI sithCogFunctionThing_SetThingLVec(SithCog* pCog)
{
    rdVector3 lvec;
    int bVec          = sithCogExec_PopVector(pCog, &lvec);
    SithThing* pThing = sithCogExec_PopThing(pCog);

    if ( !pThing || !bVec )
    {
        STDLOG_ERROR("Cog %s: Could not set thing orientation from look vector for SetThingLVec().\n", pCog->aName);
        return;
    }

    rdMatrix34 orient;
    rdMatrix_BuildRotate34(&orient, &lvec);
    rdVector_Normalize3Acc(&orient.lvec);
    rdMatrix_BuildFromLook34(&pThing->orient, &orient.lvec);

    if ( (pCog->flags & SITHCOG_NOSYNC) == 0 )
    {
        if ( pCog->execMsgType != SITHCOG_MSG_STARTUP && pCog->execMsgType != SITHCOG_MSG_SHUTDOWN )
        {
            sithThing_SyncThing(pThing, SITHTHING_SYNC_POS);
        }
    }
}

void J3DAPI sithCogFunctionThing_SetHeadPYR(SithCog* pCog)
{
    rdVector3 pyr;
    int bVec          = sithCogExec_PopVector(pCog, &pyr);
    SithThing* pThing = sithCogExec_PopThing(pCog);

    if ( !pThing )
    {
        STDLOG_ERROR("Cog %s: Bad thing in SetHeadPYR.\n", pCog->aName);
        return;
    }

    if ( pThing->type != SITH_THING_ACTOR && pThing->type != SITH_THING_PLAYER )
    {
        STDLOG_ERROR("Cog %s: Thing %s is not an actor or player in SetHeadPYR.\n", pCog->aName, pThing->aName);
        return;
    }

    if ( (pThing->thingInfo.actorInfo.flags & SITH_AF_CANROTATEHEAD) == 0 )
    {
        STDLOG_ERROR("Cog %s: Thing %s cannot change head orientation in SetHeadPYR.\n", pCog->aName, pThing->aName);
        return;
    }

    // Fixed: Added check for bVec to prevent orientation from being set to invalid values
    if ( !bVec )
    {
        STDLOG_ERROR("Cog %s: Could not set thing head orientation for SetHeadPYR.\n", pCog->aName);
        return;
    }

    pyr.pitch = STDMATH_CLAMP(pyr.pitch, pThing->thingInfo.actorInfo.minHeadPitch, pThing->thingInfo.actorInfo.maxHeadPitch);
    pyr.yaw   = STDMATH_CLAMP(pyr.yaw, -pThing->thingInfo.actorInfo.maxHeadYaw, pThing->thingInfo.actorInfo.maxHeadYaw);

    sithActor_SetHeadPYR(pThing, &pyr);
}

void J3DAPI sithCogFunctionThing_SetThingAirDrag(SithCog* pCog)
{
    float drag        = sithCogExec_PopFlex(pCog);
    SithThing* pThing = sithCogExec_PopThing(pCog);

    if ( !pThing || pThing->moveType != SITH_MT_PHYSICS )
    {
        STDLOG_ERROR("Cog %s: Bad thing in SetThingAirDrag.\n", pCog->aName);
        return;
    }

    pThing->moveInfo.physics.airDrag = drag;
}

void J3DAPI sithCogFunctionThing_GetThingJointPos(SithCog* pCog)
{
    size_t jointNum   = sithCogExec_PopInt(pCog);
    SithThing* pThing = sithCogExec_PopThing(pCog);

    if ( !pThing || !pThing->renderData.pPuppet )
    {
        sithCogExec_PushVector(pCog, &rdroid_g_zeroVector3);
        STDLOG_ERROR("Cog %s: Bad thing in GetThingJointPos.\n", pCog->aName);
        return;
    }

    if ( jointNum >= pThing->renderData.data.pModel3->numHNodes )
    {
        sithCogExec_PushVector(pCog, &rdroid_g_zeroVector3);
        STDLOG_ERROR("Cog %s: Invalid joint number in GetThingJointPos.\n", pCog->aName);
        return;
    }

    sithCogExec_PushVector(pCog, &pThing->renderData.paJointMatrices[jointNum].dvec);
}

void J3DAPI sithCogFunctionThing_IsThingModelName(SithCog* pCog)
{
    const char* pModelName = sithCogExec_PopString(pCog);
    SithThing* pThing      = sithCogExec_PopThing(pCog);

    if ( !pThing || !pThing->renderData.data.pModel3 )
    {
        STDLOG_ERROR("Cog %s: Bad Thing in IsThingModelName().", pCog->aName);
        sithCogExec_PushInt(pCog, 0);
        return;
    }

    if ( !pModelName )
    {
        STDLOG_ERROR("Cog %s: Bad args in IsThingModelName().", pCog->aName);
        sithCogExec_PushInt(pCog, 0);
        return;
    }

    if ( streqi(pThing->renderData.data.pModel3->aName, pModelName) )
    {
        sithCogExec_PushInt(pCog, 1);
        return;
    }

    sithCogExec_PushInt(pCog, 0);
}

void J3DAPI sithCogFunctionThing_GetMeshByName(SithCog* pCog)
{
    const char* pMeshName = sithCogExec_PopString(pCog);
    SithThing* pThing     = sithCogExec_PopThing(pCog);

    if ( !pThing )
    {
        STDLOG_ERROR("Cog %s: Bad Thing in GetMeshByName().", pCog->aName);
        sithCogExec_PushInt(pCog, -1);
        return;
    }

    int meshNum = sithThing_GetThingMeshIndex(pThing, pMeshName);
    sithCogExec_PushInt(pCog, meshNum);
}

void J3DAPI sithCogFunctionThing_GetNodeByName(SithCog* pCog)
{
    const char* pNodeName = sithCogExec_PopString(pCog);
    SithThing* pThing     = sithCogExec_PopThing(pCog);

    if ( !pThing )
    {
        STDLOG_ERROR("Cog %s: Bad Thing in GetNodeByName().", pCog->aName);
        sithCogExec_PushInt(pCog, -1);
        return;
    }

    int nodeNum = sithThing_GetThingJointIndex(pThing, pNodeName);
    sithCogExec_PushInt(pCog, nodeNum);
}

void J3DAPI sithCogFunctionThing_AttachThingToThingMesh(SithCog* pCog)
{
    int meshNum             = sithCogExec_PopInt(pCog);
    SithThing* pTemplate    = sithCogExec_PopTemplate(pCog);
    SithThing* pAttachThing = sithCogExec_PopThing(pCog);

    if ( !pAttachThing || !pAttachThing->renderData.pPuppet || !pAttachThing->renderData.data.pModel3 )
    {
        STDLOG_ERROR("Cog %s: Bad thing in AttachThingToThingMesh().\n", pCog->aName);
        sithCogExec_PushInt(pCog, -1);
        return;
    }

    if ( !pTemplate )
    {
        STDLOG_ERROR("Cog %s: Bad template in AttachThingToThingMesh().\n", pCog->aName);
        sithCogExec_PushInt(pCog, -1);
        return;
    }

    if ( meshNum < 0 )
    {
        STDLOG_ERROR("Cog %s: Bad mesh number in AttachThingToThingMesh().\n", pCog->aName);
        sithCogExec_PushInt(pCog, -1);
        return;
    }

    SithThing* pThing = sithThing_CreateThingAtPos(pTemplate, &pAttachThing->pos, &pAttachThing->orient, pAttachThing->pInSector, /*pParent=*/NULL);
    if ( !pThing )
    {
        STDLOG_ERROR("Cog %s: Unable to create attached thing in AttachThingToThingMesh().\n", pCog->aName);
        sithCogExec_PushInt(pCog, -1);
        return;
    }

    if ( pThing->type == SITH_THING_EXPLOSION )
    {
        pThing->pParent = pAttachThing;
    }

    sithThing_AttachThingToThingMesh(pThing, pAttachThing, meshNum);
    sithCogExec_PushInt(pCog, pThing->idx);
}

void J3DAPI sithCogFunctionThing_DetachThingMesh(SithCog* pCog)
{
    SithThing* pThing = sithCogExec_PopThing(pCog);

    if ( !pThing )
    {
        STDLOG_ERROR("Cog %s: Bad thing reference in DetachThingMesh().\n", pCog->aName);
        return;
    }

    sithThing_DestroyThing(pThing);
}

void J3DAPI sithCogFunctionThing_GetThingMaxRotVel(SithCog* pCog)
{
    SithThing* pThing = sithCogExec_PopThing(pCog);

    if ( !pThing )
    {
        STDLOG_ERROR("Cog %s: Ain't no thing in GetThingMaxRotVel().\n", pCog->aName);
        sithCogExec_PushFlex(pCog, -1.0f);
        return;
    }

    if ( pThing->type != SITH_THING_PLAYER && pThing->type != SITH_THING_ACTOR )
    {
        STDLOG_ERROR("Cog %s: Bad thing type in GetThingMaxRotVel().\n", pCog->aName);
        sithCogExec_PushFlex(pCog, -1.0f);
        return;
    }

    sithCogExec_PushFlex(pCog, pThing->thingInfo.actorInfo.maxRotVelocity);
}

void J3DAPI sithCogFunctionThing_SetThingMaxRotVel(SithCog* pCog)
{
    float vel         = sithCogExec_PopFlex(pCog);
    SithThing* pThing = sithCogExec_PopThing(pCog);

    if ( !pThing )
    {
        STDLOG_ERROR("Cog %s: Ain't no thing in SetThingMaxRotVel().\n", pCog->aName);
        return;
    }

    if ( pThing->type != SITH_THING_PLAYER && pThing->type != SITH_THING_ACTOR )
    {
        STDLOG_ERROR("Cog %s: Bad thing type in SetThingMaxRotVel().\n", pCog->aName);
        return;
    }

    pThing->thingInfo.actorInfo.maxRotVelocity = vel;
}

void J3DAPI sithCogFunctionThing_GetThingMaxHeadVel(SithCog* pCog)
{
    SithThing* pThing = sithCogExec_PopThing(pCog);

    if ( !pThing )
    {
        STDLOG_ERROR("Cog %s: Ain't no thing in GetThingMaxHeadVel().\n", pCog->aName);
        sithCogExec_PushFlex(pCog, -1.0f);
        return;
    }

    if ( pThing->type != SITH_THING_PLAYER && pThing->type != SITH_THING_ACTOR )
    {
        STDLOG_ERROR("Cog %s: Bad thing type in GetThingMaxHeadVel().\n", pCog->aName);
        sithCogExec_PushFlex(pCog, -1.0f);
        return;
    }

    sithCogExec_PushFlex(pCog, pThing->thingInfo.actorInfo.maxHeadVelocity);
}

void J3DAPI sithCogFunctionThing_SetThingMaxHeadVel(SithCog* pCog)
{
    float vel         = sithCogExec_PopFlex(pCog);
    SithThing* pThing = sithCogExec_PopThing(pCog);

    if ( !pThing )
    {
        STDLOG_ERROR("Cog %s: Ain't no thing in SetThingMaxHeadVel().\n", pCog->aName);
    }

    if ( pThing->type != SITH_THING_PLAYER && pThing->type != SITH_THING_ACTOR )
    {
        STDLOG_ERROR("Cog %s: Bad thing type in SetThingMaxHeadVel().\n", pCog->aName);
        return;
    }

    pThing->thingInfo.actorInfo.maxHeadVelocity = vel;
}

void J3DAPI sithCogFunctionThing_SetPuppetModeFPS(SithCog* pCog)
{
    float fps              = sithCogExec_PopFlex(pCog);
    unsigned int submode   = sithCogExec_PopInt(pCog);
    unsigned int majorMode = sithCogExec_PopInt(pCog);
    SithThing* pThing      = sithCogExec_PopThing(pCog);

    if ( !pThing || !pThing->pPuppetClass )
    {
        STDLOG_ERROR("Cog %s: Bad thing in SetPuppetModeFPS().\n", pCog->aName);
        sithCogExec_PushFlex(pCog, -1.0f);
        return;
    }

    if ( majorMode >= SITH_PUPPET_MAXMODES )
    {
        STDLOG_ERROR("Cog %s: Bad major mode number in SetPuppetModeFPS().\n", pCog->aName);
        sithCogExec_PushFlex(pCog, -1.0f);
        return;
    }

    if ( submode >= SITH_PUPPET_NUMSUBMODES )
    {
        STDLOG_ERROR("Cog %s: Bad mode number in SetPuppetModeFPS().\n", pCog->aName);
        sithCogExec_PushFlex(pCog, -1.0f);
        return;
    }

    rdKeyframe* pKfTrack = pThing->pPuppetClass->aModes[majorMode][submode].pKeyframe;
    if ( !pKfTrack )
    {
        STDLOG_ERROR("Cog %s: THING %s doesn't possess Submode %d for SetPuppetModeFPS().\n", pCog->aName, pThing->aName, submode);
        sithCogExec_PushFlex(pCog, -1.0f);
        return;
    }

    sithCogExec_PushFlex(pCog, pKfTrack->fps);
    pKfTrack->fps = fps;
}

void J3DAPI sithCogFunctionThing_ResetThing(SithCog* pCog)
{
    SithThing* pThing = sithCogExec_PopThing(pCog);

    if ( !pThing || !pThing->renderData.pPuppet )
    {
        STDLOG_ERROR("Cog %s: Bad thing in ResetThing().\n", pCog->aName);
        return;
    }

    sithPhysics_ResetThingMovement(pThing);
    sithPuppet_RemoveAllTracks(pThing);
    pThing->moveStatus = SITHPLAYERMOVE_STILL;
}

void J3DAPI sithCogFunctionThing_GetThingInsertOffset(SithCog* pCog)
{
    SithThing* pThing = sithCogExec_PopThing(pCog);

    if ( !pThing )
    {
        STDLOG_ERROR("Cog %s: Bad thing in GetThingInsertOffset().\n", pCog->aName);
        return;
    }

    rdModel3* pModel3 = pThing->renderData.data.pModel3;
    if ( !pModel3 )
    {
        STDLOG_ERROR("Cog %s: No thing model in GetThingInsertOffset().\n", pCog->aName);
        return;
    }

    sithCogExec_PushVector(pCog, &pModel3->insertOffset);
}

void J3DAPI sithCogFunctionThing_SetThingInsertOffset(SithCog* pCog)
{
    rdVector3 offset;
    int bVec          = sithCogExec_PopVector(pCog, &offset);
    SithThing* pThing = sithCogExec_PopThing(pCog);

    if ( !pThing )
    {
        STDLOG_ERROR("Cog %s: Bad thing in SetThingInsertOffset().\n", pCog->aName);
        sithCogExec_PushVector(pCog, &rdroid_g_zeroVector3); // Fixed: Fix returning 0 vector on error
        return;
    }

    if ( !bVec )
    {
        STDLOG_ERROR("Cog %s: Bad vector in SetThingInsertOffset().\n", pCog->aName);
        sithCogExec_PushVector(pCog, &rdroid_g_zeroVector3); // Fixed: Fix returning 0 vector on error
        return;
    }

    rdModel3* pModel3 = pThing->renderData.data.pModel3;
    if ( !pModel3 )
    {
        STDLOG_ERROR("Cog %s: No thing model in SetThingInsertOffset().\n", pCog->aName);
        sithCogExec_PushVector(pCog, &rdroid_g_zeroVector3); // Fixed: Fix returning 0 vector on error
        return;
    }

    pModel3->insertOffset = offset;
    sithCogExec_PushVector(pCog, &pThing->renderData.data.pModel3->insertOffset); // Note, this looks right, it should return the new offset.
                                                                                  // tem_lavaboss.cog is the only script using this function
}

void J3DAPI sithCogFunctionThing_GetThingEyeOffset(SithCog* pCog)
{
    SithThing* pThing = sithCogExec_PopThing(pCog);

    if ( !pThing )
    {
        STDLOG_ERROR("Cog %s: Invalid thing in GetThingEyeOffset().\n", pCog->aName);
        return;
    }

    if ( pThing->type == SITH_THING_PLAYER || pThing->type == SITH_THING_ACTOR )
    {
        sithCogExec_PushVector(pCog, &pThing->thingInfo.actorInfo.eyeOffset);
    }
    else
    {
        sithCogExec_PushVector(pCog, &rdroid_g_zeroVector3);
    }
}

void J3DAPI sithCogFunctionThing_CheckFloorDistance(SithCog* pCog)
{
    SithThing* pThing = sithCogExec_PopThing(pCog);

    if ( !pThing )
    {
        STDLOG_ERROR("Cog %s: Bad thing in CheckFloorDistance().\n", pCog->aName);
        sithCogExec_PushFlex(pCog, -1.0f);
        return;
    }

    rdVector3 moveNorm;
    rdVector_Neg3(&moveNorm, &rdroid_g_zVector3);

    const float distance = sithCollision_CheckFloorDistance(pThing, &moveNorm);
    sithCogExec_PushFlex(pCog, distance);
}

void J3DAPI sithCogFunctionThing_CheckPathToPoint(SithCog* pCog)
{
    rdVector3 vecTarget;
    const int bSkipFloor    = sithCogExec_PopInt(pCog) == 1;
    const int bDetectThings = sithCogExec_PopInt(pCog) == 1;
    const int bVec          = sithCogExec_PopVector(pCog, &vecTarget);
    SithThing* pViewer      = sithCogExec_PopThing(pCog);

    if ( !pViewer )
    {
        STDLOG_ERROR("Cog %s: Bad THING in CheckPathToPoint() call.\n", pCog->aName);
        sithCogExec_PushFlex(pCog, 0.0f);
        return;
    }

    if ( !bVec )
    {
        sithCogExec_PushFlex(pCog, 0.0f);
        return;
    }

    rdVector3 hitNorm;
    float distance;
    sithAIUtil_CheckPathToPoint(pViewer, &vecTarget, pViewer->collide.movesize, &distance, &hitNorm, bDetectThings, bSkipFloor);
    sithCogExec_PushFlex(pCog, distance);
}

void J3DAPI sithCogFunctionThing_MoveThing(SithCog* pCog)
{
    rdVector3 vecDirection;
    const float timeDelta  = sithCogExec_PopFlex(pCog);
    const float moveDist   = sithCogExec_PopFlex(pCog);
    const int bVec         = sithCogExec_PopVector(pCog, &vecDirection);
    SithThing* pThing      = sithCogExec_PopThing(pCog);

    if ( !pThing )
    {
        STDLOG_ERROR("Cog %s: Bad thing in MoveThing().\n", pCog->aName);
        sithCogExec_PushInt(pCog, -1); // Fixed: Added return -1 on error, previously it was returning nothing
        return;
    }

    if ( !bVec )
    {
        STDLOG_ERROR("Cog %s: Bad vector in MoveThing().\n", pCog->aName);
        sithCogExec_PushInt(pCog, -1); // Fixed: Added return -1 on error, previously it was returning nothing
        return;
    }

    if ( moveDist <= 0.0 )
    {
        STDLOG_ERROR("Cog %s: Bad move distance in MoveThing().\n", pCog->aName);
        sithCogExec_PushInt(pCog, -1); // Fixed: Added return -1 on error, previously it was returning nothing
        return;
    }

    if ( timeDelta > 0.0 )
    {
        SithAnimationSlot* pAnim = sithAnimate_StartThingMove(pThing, &vecDirection, moveDist, timeDelta);
        if ( pAnim )
        {
            sithCogExec_PushInt(pCog, pAnim->animID);
        }
        else
        {
            STDLOG_ERROR("Cog %s: Failed to make thing move animation in MoveThing().\n", pCog->aName); // Added: Error log
            sithCogExec_PushInt(pCog, -1);
        }
    }
    else
    {
        sithCollision_MoveThing(pThing, &vecDirection, moveDist, 0x0);
        sithCogExec_PushInt(pCog, -1);
    }
}

void J3DAPI sithCogFunctionThing_MoveThingToPos(SithCog* pCog)
{
    rdVector3 pos;
    const float time  = sithCogExec_PopFlex(pCog);
    const int bVec    = sithCogExec_PopVector(pCog, &pos);
    SithThing* pThing = sithCogExec_PopThing(pCog);

    if ( !pThing )
    {
        STDLOG_ERROR("Cog %s: Bad thing in MoveThingToPos().\n", pCog->aName);
        return;
    }

    if ( !bVec )
    {
        STDLOG_ERROR("Cog %s: Bad vector in MoveThingToPos().\n", pCog->aName);
        return;
    }

    if ( time > 0.0 )
    {
        const SithAnimationSlot* pAnim = sithAnimate_StartThingMoveToPos(pThing, &pos, time);
        sithCogExec_PushInt(pCog, pAnim ? pAnim->animID : -1);
    }
    else
    {
        rdVector3 moveNorm;
        rdVector_Sub3(&moveNorm, &pos, &pThing->pos);
        float distance = rdVector_Normalize3Acc(&moveNorm);

        sithCollision_MoveThing(pThing, &moveNorm, distance, 0x0);
        sithCogExec_PushInt(pCog, -1);
    }
}

void J3DAPI sithCogFunctionThing_SetThingStateChange(SithCog* pCog)
{
    int type          = sithCogExec_PopInt(pCog);
    int state         = sithCogExec_PopInt(pCog);
    SithThing* pThing = sithCogExec_PopThing(pCog);

    if ( !pThing )
    {
        STDLOG_ERROR("Cog %s: Bad thing in SetThingStateChange.\n", pCog->aName);
        return;
    }

    if ( pThing->type != SITH_THING_ACTOR && pThing->type != SITH_THING_PLAYER )
    {
        STDLOG_ERROR("Cog %s: Thing %s is not an actor or player in SetThingStateChange.\n", pCog->aName, pThing->aName);
        return;
    }

    pThing->thingInfo.actorInfo.stateChange.type             = type;
    pThing->thingInfo.actorInfo.stateChange.params.moveFlags = (SithActorSpecialMoveFlags)state; // Note could be also armedMode, depending on the type
}

void J3DAPI sithCogFunctionThing_StartQuetzAnim(SithCog* pCog)
{
    const int mode          = sithCogExec_PopInt(pCog);
    const SithThing* pThing = sithCogExec_PopThing(pCog);

    if ( !pThing )
    {
        STDLOG_ERROR("Bad thing in StartQuetzAnim()\n");
        sithCogExec_PushInt(pCog, -1);
        return;
    }

    if ( mode > 6 )
    {
        STDLOG_ERROR("Bad mode for thing %s in StartQuetzAnim()\n", pThing->aName);
        sithCogExec_PushInt(pCog, -1);
        return;
    }

    switch ( mode )
    {
        case 2:
            sithAIMove_sub_49AB80(pThing->controlInfo.aiControl.pLocal);
            break;
        case 3:
            sithAIMove_sub_49AC50(pThing->controlInfo.aiControl.pLocal);
            break;
        case 4:
            sithAIMove_sub_49B1B0(pThing->controlInfo.aiControl.pLocal);
            break;
        case 5:
            sithAIMove_sub_49AF80(pThing->controlInfo.aiControl.pLocal);
            break;
        default:
            return;
    }
}

void J3DAPI sithCogFunctionThing_BoardVehicle(SithCog* pCog)
{
    SithThing* pThing = sithCogExec_PopThing(pCog);

    if ( !pThing )
    {
        STDLOG_ERROR("Cog %s: Bad thing in Board Vehicle.\n", pCog->aName);
        return;
    }

    if ( pThing->type != SITH_THING_PLAYER )
    {
        STDLOG_ERROR("Cog %s: Thing %s is not player in Board Vehicle.\n", pCog->aName, pThing->aName);
        return;
    }

    const int bBoarded = sithPlayerControls_BoardVehicle(pThing, /*bBoard=*/1);
    sithCogExec_PushInt(pCog, bBoarded);
}

void J3DAPI sithCogFunctionThing_FadeInTrack(SithCog* pCog)
{
    const float speed = sithCogExec_PopFlex(pCog);
    const int track   = sithCogExec_PopInt(pCog);
    SithThing* pThing = sithCogExec_PopThing(pCog);

    if ( !pThing )
    {
        STDLOG_ERROR("Cog %s: Bad thing in FadeInMode().\n", pCog->aName);
        sithCogExec_PushInt(pCog, -1);
        return;
    }

    const int bSuccess = rdPuppet_FadeInTrack(pThing->renderData.pPuppet, track, speed);
    sithCogExec_PushInt(pCog, bSuccess);
}

void J3DAPI sithCogFunctionThing_MakeFairyDust(SithCog* pCog)
{
    rdVector3 pos;
    int bPos          = sithCogExec_PopVector(pCog, &pos);
    SithThing* pThing = sithCogExec_PopThing(pCog);

    if ( !pThing || !pThing->pInSector | !bPos )
    {
        STDLOG_ERROR("Cog %s: Bad args in MakeFairyDust().\n", pCog->aName);
        return;
    }

    sithFX_CreateFairyDust(&pos, pThing->pInSector);
}

void J3DAPI sithCogFunctionThing_MakeFairyDustDeluxe(SithCog* pCog)
{
    rdVector3 pos;
    int bPos          = sithCogExec_PopVector(pCog, &pos);
    SithThing* pThing = sithCogExec_PopThing(pCog);

    if ( !pThing || !bPos )
    {
        STDLOG_ERROR("Cog %s: Bad args in MakeFairyDustDeluxe().\n", pCog->aName);
        return;
    }

    if ( rdVector_IsZero3(&pos) )
    {
        sithFX_DestroyFairyDustDeluxe(pThing);
    }
    else
    {
        sithFX_CreateFairyDustDelux(pThing, &pos);
    }
}

void sithCogFunctionThing_InstallHooks(void)
{
    J3D_HOOKFUNC(sithCogFunctionThing_RegisterFunctions);
    J3D_HOOKFUNC(sithCogFunctionThing_IsGhostVisible);
    J3D_HOOKFUNC(sithCogFunctionThing_SetThingAlpha);
    J3D_HOOKFUNC(sithCogFunctionThing_GetThingAlpha);
    J3D_HOOKFUNC(sithCogFunctionThing_CreatePolylineThing);
    J3D_HOOKFUNC(sithCogFunctionThing_GetMoveStatus);
    J3D_HOOKFUNC(sithCogFunctionThing_CopyOrient);
    J3D_HOOKFUNC(sithCogFunctionThing_CopyOrientAndPos);
    J3D_HOOKFUNC(sithCogFunctionThing_CreateLaser);
    J3D_HOOKFUNC(sithCogFunctionThing_CreateLightning);
    J3D_HOOKFUNC(sithCogFunctionThing_AnimateSpriteSize);
    J3D_HOOKFUNC(sithCogFunctionThing_GetThingType);
    J3D_HOOKFUNC(sithCogFunctionThing_PlayForceMoveMode);
    J3D_HOOKFUNC(sithCogFunctionThing_SetMoveMode);
    J3D_HOOKFUNC(sithCogFunctionThing_CreateThing);
    J3D_HOOKFUNC(sithCogFunctionThing_CreateThingAtPos);
    J3D_HOOKFUNC(sithCogFunctionThing_DamageThing);
    J3D_HOOKFUNC(sithCogFunctionThing_HealThing);
    J3D_HOOKFUNC(sithCogFunctionThing_GetHealth);
    J3D_HOOKFUNC(sithCogFunctionThing_GetMaxHealth);
    J3D_HOOKFUNC(sithCogFunctionThing_SetHealth);
    J3D_HOOKFUNC(sithCogFunctionThing_DestroyThing);
    J3D_HOOKFUNC(sithCogFunctionThing_JumpToFrame);
    J3D_HOOKFUNC(sithCogFunctionThing_MoveToFrame);
    J3D_HOOKFUNC(sithCogFunctionThing_SkipToFrame);
    J3D_HOOKFUNC(sithCogFunctionThing_RotatePivot);
    J3D_HOOKFUNC(sithCogFunctionThing_Rotate);
    J3D_HOOKFUNC(sithCogFunctionThing_RotateToPYR);
    J3D_HOOKFUNC(sithCogFunctionThing_GetThingLight);
    J3D_HOOKFUNC(sithCogFunctionThing_SetExtCamOffset);
    J3D_HOOKFUNC(sithCogFunctionThing_SetExtCamOffsetToThing);
    J3D_HOOKFUNC(sithCogFunctionThing_SetExtCamLookOffset);
    J3D_HOOKFUNC(sithCogFunctionThing_SetExtCamLookOffsetToThing);
    J3D_HOOKFUNC(sithCogFunctionThing_MakeCamera2LikeCamera1);
    J3D_HOOKFUNC(sithCogFunctionThing_SetCameraPosition);
    J3D_HOOKFUNC(sithCogFunctionThing_SetCameraLookInterp);
    J3D_HOOKFUNC(sithCogFunctionThing_SetCameraPosInterp);
    J3D_HOOKFUNC(sithCogFunctionThing_GetCameraFOV);
    J3D_HOOKFUNC(sithCogFunctionThing_GetCameraPosition);
    J3D_HOOKFUNC(sithCogFunctionThing_SetCameraInterpSpeed);
    J3D_HOOKFUNC(sithCogFunctionThing_SetCameraFOV);
    J3D_HOOKFUNC(sithCogFunctionThing_ResetCameraFOV);
    J3D_HOOKFUNC(sithCogFunctionThing_SetCameraFadeThing);
    J3D_HOOKFUNC(sithCogFunctionThing_ThingLight);
    J3D_HOOKFUNC(sithCogFunctionThing_ThingLightAnim);
    J3D_HOOKFUNC(sithCogFunctionThing_ThingFadeAnim);
    J3D_HOOKFUNC(sithCogFunctionThing_WaitForStop);
    J3D_HOOKFUNC(sithCogFunctionThing_WaitForAnimStop);
    J3D_HOOKFUNC(sithCogFunctionThing_GetThingSector);
    J3D_HOOKFUNC(sithCogFunctionThing_GetCurFrame);
    J3D_HOOKFUNC(sithCogFunctionThing_GetGoalFrame);
    J3D_HOOKFUNC(sithCogFunctionThing_StopThing);
    J3D_HOOKFUNC(sithCogFunctionThing_IsThingMoving);
    J3D_HOOKFUNC(sithCogFunctionThing_SetThingPulse);
    J3D_HOOKFUNC(sithCogFunctionThing_SetThingTimer);
    J3D_HOOKFUNC(sithCogFunctionThing_CaptureThing);
    J3D_HOOKFUNC(sithCogFunctionThing_ReleaseThing);
    J3D_HOOKFUNC(sithCogFunctionThing_GetThingParent);
    J3D_HOOKFUNC(sithCogFunctionThing_GetThingPos);
    J3D_HOOKFUNC(sithCogFunctionThing_SetThingPos);
    J3D_HOOKFUNC(sithCogFunctionThing_GetInventory);
    J3D_HOOKFUNC(sithCogFunctionThing_SetInventory);
    J3D_HOOKFUNC(sithCogFunctionThing_ChangeInventory);
    J3D_HOOKFUNC(sithCogFunctionThing_GetInventoryCog);
    J3D_HOOKFUNC(sithCogFunctionThing_GetThingVelocity);
    J3D_HOOKFUNC(sithCogFunctionThing_SetThingVel);
    J3D_HOOKFUNC(sithCogFunctionThing_ApplyForce);
    J3D_HOOKFUNC(sithCogFunctionThing_AddThingVel);
    J3D_HOOKFUNC(sithCogFunctionThing_GetThingLVec);
    J3D_HOOKFUNC(sithCogFunctionThing_GetThingUVec);
    J3D_HOOKFUNC(sithCogFunctionThing_GetThingRVec);
    J3D_HOOKFUNC(sithCogFunctionThing_GetActorHeadPYR);
    J3D_HOOKFUNC(sithCogFunctionThing_DetachThing);
    J3D_HOOKFUNC(sithCogFunctionThing_GetThingAttachFlags);
    J3D_HOOKFUNC(sithCogFunctionThing_AttachThingToSurf);
    J3D_HOOKFUNC(sithCogFunctionThing_AttachThingToThing);
    J3D_HOOKFUNC(sithCogFunctionThing_AttachThingToThingEx);
    J3D_HOOKFUNC(sithCogFunctionThing_GetThingAttachedThing);
    J3D_HOOKFUNC(sithCogFunctionThing_WaitMode);
    J3D_HOOKFUNC(sithCogFunctionThing_PlayMode);
    J3D_HOOKFUNC(sithCogFunctionThing_StopMode);
    J3D_HOOKFUNC(sithCogFunctionThing_SynchMode);
    J3D_HOOKFUNC(sithCogFunctionThing_IsModePlaying);
    J3D_HOOKFUNC(sithCogFunctionThing_PauseMode);
    J3D_HOOKFUNC(sithCogFunctionThing_ResumeMode);
    J3D_HOOKFUNC(sithCogFunctionThing_TrackToMode);
    J3D_HOOKFUNC(sithCogFunctionThing_PlayKey);
    J3D_HOOKFUNC(sithCogFunctionThing_PlayKeyEx);
    J3D_HOOKFUNC(sithCogFunctionThing_PauseKey);
    J3D_HOOKFUNC(sithCogFunctionThing_ResumeKey);
    J3D_HOOKFUNC(sithCogFunctionThing_StopKey);
    J3D_HOOKFUNC(sithCogFunctionThing_SetThingMesh);
    J3D_HOOKFUNC(sithCogFunctionThing_RestoreThingMesh);
    J3D_HOOKFUNC(sithCogFunctionThing_SetThingModel);
    J3D_HOOKFUNC(sithCogFunctionThing_GetThingModel);
    J3D_HOOKFUNC(sithCogFunctionThing_SetArmedMode);
    J3D_HOOKFUNC(sithCogFunctionThing_GetThingFlags);
    J3D_HOOKFUNC(sithCogFunctionThing_SetThingFlags);
    J3D_HOOKFUNC(sithCogFunctionThing_ClearThingFlags);
    J3D_HOOKFUNC(sithCogFunctionThing_TeleportThing);
    J3D_HOOKFUNC(sithCogFunctionThing_SetThingType);
    J3D_HOOKFUNC(sithCogFunctionThing_GetCollideType);
    J3D_HOOKFUNC(sithCogFunctionThing_SetCollideType);
    J3D_HOOKFUNC(sithCogFunctionThing_FirstThingInSector);
    J3D_HOOKFUNC(sithCogFunctionThing_NextThingInSector);
    J3D_HOOKFUNC(sithCogFunctionThing_PrevThingInSector);
    J3D_HOOKFUNC(sithCogFunctionThing_GetInventoryMinimum);
    J3D_HOOKFUNC(sithCogFunctionThing_GetInventoryMaximum);
    J3D_HOOKFUNC(sithCogFunctionThing_PathMovePause);
    J3D_HOOKFUNC(sithCogFunctionThing_SetHeadLightIntensity);
    J3D_HOOKFUNC(sithCogFunctionThing_GetHeadLightIntensity);
    J3D_HOOKFUNC(sithCogFunctionThing_IsThingVisible);
    J3D_HOOKFUNC(sithCogFunctionThing_PathMoveResume);
    J3D_HOOKFUNC(sithCogFunctionThing_SetThingCurLightMode);
    J3D_HOOKFUNC(sithCogFunctionThing_GetThingCurLightMode);
    J3D_HOOKFUNC(sithCogFunctionThing_SetActorExtraSpeed);
    J3D_HOOKFUNC(sithCogFunctionThing_GetThingTemplate);
    J3D_HOOKFUNC(sithCogFunctionThing_SetLifeleft);
    J3D_HOOKFUNC(sithCogFunctionThing_GetLifeleft);
    J3D_HOOKFUNC(sithCogFunctionThing_SetThingThrust);
    J3D_HOOKFUNC(sithCogFunctionThing_GetThingThrust);
    J3D_HOOKFUNC(sithCogFunctionThing_AmputateJoint);
    J3D_HOOKFUNC(sithCogFunctionThing_SetActorWeapon);
    J3D_HOOKFUNC(sithCogFunctionThing_GetActorWeapon);
    J3D_HOOKFUNC(sithCogFunctionThing_GetPhysicsFlags);
    J3D_HOOKFUNC(sithCogFunctionThing_SetPhysicsFlags);
    J3D_HOOKFUNC(sithCogFunctionThing_ClearPhysicsFlags);
    J3D_HOOKFUNC(sithCogFunctionThing_ParseArg);
    J3D_HOOKFUNC(sithCogFunctionThing_SetThingRotVel);
    J3D_HOOKFUNC(sithCogFunctionThing_GetThingRotVel);
    J3D_HOOKFUNC(sithCogFunctionThing_SetRotThrust);
    J3D_HOOKFUNC(sithCogFunctionThing_GetRotThrust);
    J3D_HOOKFUNC(sithCogFunctionThing_SetThingLook);
    J3D_HOOKFUNC(sithCogFunctionThing_SetThingHeadOrientation);
    J3D_HOOKFUNC(sithCogFunctionThing_SetThingHeadLookPos);
    J3D_HOOKFUNC(sithCogFunctionThing_SetThingHeadLookThing);
    J3D_HOOKFUNC(sithCogFunctionThing_IsThingCrouching);
    J3D_HOOKFUNC(sithCogFunctionThing_GetThingClassCog);
    J3D_HOOKFUNC(sithCogFunctionThing_SetThingClassCog);
    J3D_HOOKFUNC(sithCogFunctionThing_GetThingCaptureCog);
    J3D_HOOKFUNC(sithCogFunctionThing_SetThingCaptureCog);
    J3D_HOOKFUNC(sithCogFunctionThing_GetThingRespawn);
    J3D_HOOKFUNC(sithCogFunctionThing_GetThingSignature);
    J3D_HOOKFUNC(sithCogFunctionThing_SetThingAttachFlags);
    J3D_HOOKFUNC(sithCogFunctionThing_ClearThingAttachFlags);
    J3D_HOOKFUNC(sithCogFunctionThing_GetParticleSize);
    J3D_HOOKFUNC(sithCogFunctionThing_SetParticleSize);
    J3D_HOOKFUNC(sithCogFunctionThing_GetParticleGrowthSpeed);
    J3D_HOOKFUNC(sithCogFunctionThing_SetParticleGrowthSpeed);
    J3D_HOOKFUNC(sithCogFunctionThing_GetParticleTimeoutRate);
    J3D_HOOKFUNC(sithCogFunctionThing_SetParticleTimeoutRate);
    J3D_HOOKFUNC(sithCogFunctionThing_GetTypeFlags);
    J3D_HOOKFUNC(sithCogFunctionThing_SetTypeFlags);
    J3D_HOOKFUNC(sithCogFunctionThing_ClearTypeFlags);
    J3D_HOOKFUNC(sithCogFunctionThing_TakeItem);
    J3D_HOOKFUNC(sithCogFunctionThing_HasLOS);
    J3D_HOOKFUNC(sithCogFunctionThing_GetFireOffset);
    J3D_HOOKFUNC(sithCogFunctionThing_SetFireOffset);
    J3D_HOOKFUNC(sithCogFunctionThing_GetThingUserData);
    J3D_HOOKFUNC(sithCogFunctionThing_SetThingUserData);
    J3D_HOOKFUNC(sithCogFunctionThing_GetThingCollideSize);
    J3D_HOOKFUNC(sithCogFunctionThing_SetThingCollideSize);
    J3D_HOOKFUNC(sithCogFunctionThing_GetThingMoveSize);
    J3D_HOOKFUNC(sithCogFunctionThing_SetThingMoveSize);
    J3D_HOOKFUNC(sithCogFunctionThing_GetThingMass);
    J3D_HOOKFUNC(sithCogFunctionThing_SetThingMass);
    J3D_HOOKFUNC(sithCogFunctionThing_SyncThingPos);
    J3D_HOOKFUNC(sithCogFunctionThing_SyncThingAttachment);
    J3D_HOOKFUNC(sithCogFunctionThing_SyncThingState);
    J3D_HOOKFUNC(sithCogFunctionThing_GetMajorMode);
    J3D_HOOKFUNC(sithCogFunctionThing_SetThingPosEx);
    J3D_HOOKFUNC(sithCogFunctionThing_GetThingGuid);
    J3D_HOOKFUNC(sithCogFunctionThing_GetGuidThing);
    J3D_HOOKFUNC(sithCogFunctionThing_GetThingMaxVel);
    J3D_HOOKFUNC(sithCogFunctionThing_SetThingMaxVel);
    J3D_HOOKFUNC(sithCogFunctionThing_GetThingMaxAngVel);
    J3D_HOOKFUNC(sithCogFunctionThing_SetThingMaxAngVel);
    J3D_HOOKFUNC(sithCogFunctionThing_SetJointAngle);
    J3D_HOOKFUNC(sithCogFunctionThing_GetJointAngle);
    J3D_HOOKFUNC(sithCogFunctionThing_SetMaxHeadPitch);
    J3D_HOOKFUNC(sithCogFunctionThing_SetMinHeadPitch);
    J3D_HOOKFUNC(sithCogFunctionThing_GetThingMaxHeadPitch);
    J3D_HOOKFUNC(sithCogFunctionThing_GetThingMinHeadPitch);
    J3D_HOOKFUNC(sithCogFunctionThing_SetMaxHeadYaw);
    J3D_HOOKFUNC(sithCogFunctionThing_GetThingMaxHeadYaw);
    J3D_HOOKFUNC(sithCogFunctionThing_InterpolatePYR);
    J3D_HOOKFUNC(sithCogFunctionThing_GetThingLVecPYR);
    J3D_HOOKFUNC(sithCogFunctionThing_SetThingLVec);
    J3D_HOOKFUNC(sithCogFunctionThing_SetHeadPYR);
    J3D_HOOKFUNC(sithCogFunctionThing_SetThingAirDrag);
    J3D_HOOKFUNC(sithCogFunctionThing_GetThingJointPos);
    J3D_HOOKFUNC(sithCogFunctionThing_IsThingModelName);
    J3D_HOOKFUNC(sithCogFunctionThing_GetMeshByName);
    J3D_HOOKFUNC(sithCogFunctionThing_GetNodeByName);
    J3D_HOOKFUNC(sithCogFunctionThing_AttachThingToThingMesh);
    J3D_HOOKFUNC(sithCogFunctionThing_DetachThingMesh);
    J3D_HOOKFUNC(sithCogFunctionThing_GetThingMaxRotVel);
    J3D_HOOKFUNC(sithCogFunctionThing_SetThingMaxRotVel);
    J3D_HOOKFUNC(sithCogFunctionThing_GetThingMaxHeadVel);
    J3D_HOOKFUNC(sithCogFunctionThing_SetThingMaxHeadVel);
    J3D_HOOKFUNC(sithCogFunctionThing_SetPuppetModeFPS);
    J3D_HOOKFUNC(sithCogFunctionThing_ResetThing);
    J3D_HOOKFUNC(sithCogFunctionThing_GetThingInsertOffset);
    J3D_HOOKFUNC(sithCogFunctionThing_SetThingInsertOffset);
    J3D_HOOKFUNC(sithCogFunctionThing_GetThingEyeOffset);
    J3D_HOOKFUNC(sithCogFunctionThing_CheckFloorDistance);
    J3D_HOOKFUNC(sithCogFunctionThing_CheckPathToPoint);
    J3D_HOOKFUNC(sithCogFunctionThing_MoveThing);
    J3D_HOOKFUNC(sithCogFunctionThing_MoveThingToPos);
    J3D_HOOKFUNC(sithCogFunctionThing_SetThingStateChange);
    J3D_HOOKFUNC(sithCogFunctionThing_StartQuetzAnim);
    J3D_HOOKFUNC(sithCogFunctionThing_BoardVehicle);
    J3D_HOOKFUNC(sithCogFunctionThing_FadeInTrack);
    J3D_HOOKFUNC(sithCogFunctionThing_MakeFairyDust);
    J3D_HOOKFUNC(sithCogFunctionThing_MakeFairyDustDeluxe);
}

void sithCogFunctionThing_ResetGlobals(void)
{}

int J3DAPI sithCogFunctionThing_RegisterFunctions(SithCogSymbolTable* pTable)
{
    return sithCog_RegisterFunction(pTable, sithCogFunctionThing_WaitForStop, "waitforstop")
        || sithCog_RegisterFunction(pTable, sithCogFunctionThing_WaitForAnimStop, "waitforanimstop")
        || sithCog_RegisterFunction(pTable, sithCogFunctionThing_StopThing, "stopthing")
        || sithCog_RegisterFunction(pTable, sithCogFunctionThing_DestroyThing, "destroything")
        || sithCog_RegisterFunction(pTable, sithCogFunctionThing_GetHealth, "getthinghealth")
        || sithCog_RegisterFunction(pTable, sithCogFunctionThing_GetMaxHealth, "getthingmaxhealth")
        || sithCog_RegisterFunction(pTable, sithCogFunctionThing_GetHealth, "gethealth")
        || sithCog_RegisterFunction(pTable, sithCogFunctionThing_HealThing, "healthing")
        || sithCog_RegisterFunction(pTable, sithCogFunctionThing_GetThingLight, "getthinglight")
        || sithCog_RegisterFunction(pTable, sithCogFunctionThing_ThingLight, "setthinglight")
        || sithCog_RegisterFunction(pTable, sithCogFunctionThing_ThingLight, "thinglight")
        || sithCog_RegisterFunction(pTable, sithCogFunctionThing_ThingLightAnim, "thinglightanim")
        || sithCog_RegisterFunction(pTable, sithCogFunctionThing_ThingFadeAnim, "thingfadeanim")
        || sithCog_RegisterFunction(pTable, sithCogFunctionThing_CreateThing, "creatething")
        || sithCog_RegisterFunction(pTable, sithCogFunctionThing_CreateThingAtPos, "createthingatpos")
        || sithCog_RegisterFunction(pTable, sithCogFunctionThing_CaptureThing, "capturething")
        || sithCog_RegisterFunction(pTable, sithCogFunctionThing_ReleaseThing, "releasething")
        || sithCog_RegisterFunction(pTable, sithCogFunctionThing_SetThingVel, "setthingvel")
        || sithCog_RegisterFunction(pTable, sithCogFunctionThing_AddThingVel, "addthingvel")
        || sithCog_RegisterFunction(pTable, sithCogFunctionThing_ApplyForce, "applyforce")
        || sithCog_RegisterFunction(pTable, sithCogFunctionThing_DetachThing, "detachthing")
        || sithCog_RegisterFunction(pTable, sithCogFunctionThing_GetThingAttachFlags, "getattachflags")
        || sithCog_RegisterFunction(pTable, sithCogFunctionThing_GetThingAttachFlags, "getthingattachflags")
        || sithCog_RegisterFunction(pTable, sithCogFunctionThing_AttachThingToSurf, "attachthingtosurf")
        || sithCog_RegisterFunction(pTable, sithCogFunctionThing_AttachThingToThing, "attachthingtothing")
        || sithCog_RegisterFunction(pTable, sithCogFunctionThing_SetArmedMode, "setarmedmode")
        || sithCog_RegisterFunction(pTable, sithCogFunctionThing_SetThingFlags, "setthingflags")
        || sithCog_RegisterFunction(pTable, sithCogFunctionThing_ClearThingFlags, "clearthingflags")
        || sithCog_RegisterFunction(pTable, sithCogFunctionThing_TeleportThing, "teleportthing")
        || sithCog_RegisterFunction(pTable, sithCogFunctionThing_SetThingType, "setthingtype")
        || sithCog_RegisterFunction(pTable, sithCogFunctionThing_SetCollideType, "setcollidetype")
        || sithCog_RegisterFunction(pTable, sithCogFunctionThing_SetHeadLightIntensity, "setheadlightintensity")
        || sithCog_RegisterFunction(pTable, sithCogFunctionThing_GetThingCurLightMode, "getthingcurlightmode")
        || sithCog_RegisterFunction(pTable, sithCogFunctionThing_SetThingCurLightMode, "setthingcurlightmode")
        || sithCog_RegisterFunction(pTable, sithCogFunctionThing_SetActorExtraSpeed, "setactorextraspeed")
        || sithCog_RegisterFunction(pTable, sithCogFunctionThing_SetThingPosEx, "setthingposex")
        || sithCog_RegisterFunction(pTable, sithCogFunctionThing_SetThingMaxVel, "setthingmaxvel")
        || sithCog_RegisterFunction(pTable, sithCogFunctionThing_SetThingMaxAngVel, "setthingmaxangvel")
        || sithCog_RegisterFunction(pTable, sithCogFunctionThing_SetJointAngle, "setthingjointangle")
        || sithCog_RegisterFunction(pTable, sithCogFunctionThing_SetMaxHeadPitch, "setthingmaxheadpitch")
        || sithCog_RegisterFunction(pTable, sithCogFunctionThing_SetMinHeadPitch, "setthingminheadpitch")
        || sithCog_RegisterFunction(pTable, sithCogFunctionThing_GetThingMaxHeadPitch, "getthingmaxheadpitch")
        || sithCog_RegisterFunction(pTable, sithCogFunctionThing_GetThingMinHeadPitch, "getthingminheadpitch")
        || sithCog_RegisterFunction(pTable, sithCogFunctionThing_SetMaxHeadYaw, "setthingmaxheadyaw")
        || sithCog_RegisterFunction(pTable, sithCogFunctionThing_GetThingMaxHeadYaw, "getthingmaxheadyaw")
        || sithCog_RegisterFunction(pTable, sithCogFunctionThing_SetThingLVec, "setthinglvecpyr")
        || sithCog_RegisterFunction(pTable, sithCogFunctionThing_SetHeadPYR, "setactorheadpyr")
        || sithCog_RegisterFunction(pTable, sithCogFunctionThing_SetThingAirDrag, "setthingairdrag")
        || sithCog_RegisterFunction(pTable, sithCogFunctionThing_SetThingMaxRotVel, "setthingmaxrotvel")
        || sithCog_RegisterFunction(pTable, sithCogFunctionThing_SetThingMaxHeadVel, "setthingmaxheadvel")
        || sithCog_RegisterFunction(pTable, sithCogFunctionThing_ResetThing, "resetthing")
        || sithCog_RegisterFunction(pTable, sithCogFunctionThing_MoveThing, "movething")
        || sithCog_RegisterFunction(pTable, sithCogFunctionThing_MoveThingToPos, "movethingtopos")
        || sithCog_RegisterFunction(pTable, sithCogFunctionThing_GetThingType, "getthingtype")
        || sithCog_RegisterFunction(pTable, sithCogFunctionThing_IsThingMoving, "isthingmoving")
        || sithCog_RegisterFunction(pTable, sithCogFunctionThing_IsThingMoving, "ismoving")
        || sithCog_RegisterFunction(pTable, sithCogFunctionThing_GetCurFrame, "getcurframe")
        || sithCog_RegisterFunction(pTable, sithCogFunctionThing_GetGoalFrame, "getgoalframe")
        || sithCog_RegisterFunction(pTable, sithCogFunctionThing_GetThingParent, "getthingparent")
        || sithCog_RegisterFunction(pTable, sithCogFunctionThing_GetThingSector, "getthingsector")
        || sithCog_RegisterFunction(pTable, sithCogFunctionThing_GetThingPos, "getthingpos")
        || sithCog_RegisterFunction(pTable, sithCogFunctionThing_SetThingPos, "setthingpos")
        || sithCog_RegisterFunction(pTable, sithCogFunctionThing_GetThingVelocity, "getthingvel")
        || sithCog_RegisterFunction(pTable, sithCogFunctionThing_GetThingUVec, "getthinguvec")
        || sithCog_RegisterFunction(pTable, sithCogFunctionThing_GetThingLVec, "getthinglvec")
        || sithCog_RegisterFunction(pTable, sithCogFunctionThing_GetThingRVec, "getthingrvec")
        || sithCog_RegisterFunction(pTable, sithCogFunctionThing_GetThingFlags, "getthingflags")
        || sithCog_RegisterFunction(pTable, sithCogFunctionThing_GetCollideType, "getcollidetype")
        || sithCog_RegisterFunction(pTable, sithCogFunctionThing_GetHeadLightIntensity, "getheadlightintensity")
        || sithCog_RegisterFunction(pTable, sithCogFunctionThing_IsThingVisible, "isthingvisible")
        || sithCog_RegisterFunction(pTable, sithCogFunctionThing_GetThingGuid, "getthingguid")
        || sithCog_RegisterFunction(pTable, sithCogFunctionThing_GetGuidThing, "getguidthing")
        || sithCog_RegisterFunction(pTable, sithCogFunctionThing_GetThingMaxVel, "getthingmaxvel")
        || sithCog_RegisterFunction(pTable, sithCogFunctionThing_GetThingMaxAngVel, "getthingmaxangvel")
        || sithCog_RegisterFunction(pTable, sithCogFunctionThing_GetJointAngle, "getthingjointangle")
        || sithCog_RegisterFunction(pTable, sithCogFunctionThing_InterpolatePYR, "interpolatepyr")
        || sithCog_RegisterFunction(pTable, sithCogFunctionThing_GetThingLVecPYR, "getthinglvecpyr")
        || sithCog_RegisterFunction(pTable, sithCogFunctionThing_GetActorHeadPYR, "getactorheadpyr")
        || sithCog_RegisterFunction(pTable, sithCogFunctionThing_GetThingJointPos, "getthingjointpos")
        || sithCog_RegisterFunction(pTable, sithCogFunctionThing_IsThingModelName, "isthingmodelname")
        || sithCog_RegisterFunction(pTable, sithCogFunctionThing_GetThingMaxRotVel, "getthingmaxrotvel")
        || sithCog_RegisterFunction(pTable, sithCogFunctionThing_GetThingMaxHeadVel, "getthingmaxheadvel")
        || sithCog_RegisterFunction(pTable, sithCogFunctionThing_CopyOrient, "copyorient")
        || sithCog_RegisterFunction(pTable, sithCogFunctionThing_CopyOrientAndPos, "copyorientandpos")
        || sithCog_RegisterFunction(pTable, sithCogFunctionThing_GetThingInsertOffset, "getthinginsertoffset")
        || sithCog_RegisterFunction(pTable, sithCogFunctionThing_SetThingInsertOffset, "setthinginsertoffset")
        || sithCog_RegisterFunction(pTable, sithCogFunctionThing_GetThingEyeOffset, "getthingeyeoffset")
        || sithCog_RegisterFunction(pTable, sithCogFunctionThing_SetThingPulse, "setthingpulse")
        || sithCog_RegisterFunction(pTable, sithCogFunctionThing_SetThingTimer, "setthingtimer")
        || sithCog_RegisterFunction(pTable, sithCogFunctionThing_GetInventory, "getinv")
        || sithCog_RegisterFunction(pTable, sithCogFunctionThing_SetInventory, "setinv")
        || sithCog_RegisterFunction(pTable, sithCogFunctionThing_ChangeInventory, "changeinv")
        || sithCog_RegisterFunction(pTable, sithCogFunctionThing_GetInventoryCog, "getinvcog")
        || sithCog_RegisterFunction(pTable, sithCogFunctionThing_GetInventoryMinimum, "getinvmin")
        || sithCog_RegisterFunction(pTable, sithCogFunctionThing_GetInventoryMaximum, "getinvmax")
        || sithCog_RegisterFunction(pTable, sithCogFunctionThing_PlayKey, "playkey")
        || sithCog_RegisterFunction(pTable, sithCogFunctionThing_PlayKeyEx, "playkeyex")
        || sithCog_RegisterFunction(pTable, sithCogFunctionThing_StopKey, "stopkey")
        || sithCog_RegisterFunction(pTable, sithCogFunctionThing_PauseKey, "pausekey")
        || sithCog_RegisterFunction(pTable, sithCogFunctionThing_ResumeKey, "resumekey")
        || sithCog_RegisterFunction(pTable, sithCogFunctionThing_SetThingModel, "setthingmodel")
        || sithCog_RegisterFunction(pTable, sithCogFunctionThing_GetThingModel, "getthingmodel")
        || sithCog_RegisterFunction(pTable, sithCogFunctionThing_PlayMode, "playmode")
        || sithCog_RegisterFunction(pTable, sithCogFunctionThing_StopMode, "stopmode")
        || sithCog_RegisterFunction(pTable, sithCogFunctionThing_SynchMode, "synchmode")
        || sithCog_RegisterFunction(pTable, sithCogFunctionThing_IsModePlaying, "ismodeplaying")
        || sithCog_RegisterFunction(pTable, sithCogFunctionThing_PauseMode, "pausemode")
        || sithCog_RegisterFunction(pTable, sithCogFunctionThing_ResumeMode, "resumemode")
        || sithCog_RegisterFunction(pTable, sithCogFunctionThing_TrackToMode, "tracktomode")
        || sithCog_RegisterFunction(pTable, sithCogFunctionThing_WaitMode, "waitmode")
        || sithCog_RegisterFunction(pTable, sithCogFunctionThing_GetMajorMode, "getmajormode")
        || sithCog_RegisterFunction(pTable, sithCogFunctionThing_FirstThingInSector, "firstthinginsector")
        || sithCog_RegisterFunction(pTable, sithCogFunctionThing_NextThingInSector, "nextthinginsector")
        || sithCog_RegisterFunction(pTable, sithCogFunctionThing_PrevThingInSector, "prevthinginsector")
        || sithCog_RegisterFunction(pTable, sithCogFunctionThing_MoveToFrame, "movetoframe")
        || sithCog_RegisterFunction(pTable, sithCogFunctionThing_SkipToFrame, "skiptoframe")
        || sithCog_RegisterFunction(pTable, sithCogFunctionThing_JumpToFrame, "jumptoframe")
        || sithCog_RegisterFunction(pTable, sithCogFunctionThing_PathMovePause, "pathmovepause")
        || sithCog_RegisterFunction(pTable, sithCogFunctionThing_PathMoveResume, "pathmoveresume")
        || sithCog_RegisterFunction(pTable, sithCogFunctionThing_Rotate, "rotate")
        || sithCog_RegisterFunction(pTable, sithCogFunctionThing_RotatePivot, "rotatepivot")
        || sithCog_RegisterFunction(pTable, sithCogFunctionThing_RotateToPYR, "rotatetopyr")
        || sithCog_RegisterFunction(pTable, sithCogFunctionThing_GetThingTemplate, "getthingtemplate")
        || sithCog_RegisterFunction(pTable, sithCogFunctionThing_DamageThing, "damagething")
        || sithCog_RegisterFunction(pTable, sithCogFunctionThing_SetLifeleft, "setlifeleft")
        || sithCog_RegisterFunction(pTable, sithCogFunctionThing_GetLifeleft, "getlifeleft")
        || sithCog_RegisterFunction(pTable, sithCogFunctionThing_SetThingThrust, "setthingthrust")
        || sithCog_RegisterFunction(pTable, sithCogFunctionThing_GetThingThrust, "getthingthrust")
        || sithCog_RegisterFunction(pTable, sithCogFunctionThing_SetHealth, "setthinghealth")
        || sithCog_RegisterFunction(pTable, sithCogFunctionThing_SetHealth, "sethealth")
        || sithCog_RegisterFunction(pTable, sithCogFunctionThing_AmputateJoint, "amputatejoint")
        || sithCog_RegisterFunction(pTable, sithCogFunctionThing_SetActorWeapon, "setactorweapon")
        || sithCog_RegisterFunction(pTable, sithCogFunctionThing_GetActorWeapon, "getactorweapon")
        || sithCog_RegisterFunction(pTable, sithCogFunctionThing_GetPhysicsFlags, "getphysicsflags")
        || sithCog_RegisterFunction(pTable, sithCogFunctionThing_SetPhysicsFlags, "setphysicsflags")
        || sithCog_RegisterFunction(pTable, sithCogFunctionThing_ClearPhysicsFlags, "clearphysicsflags")
        || sithCog_RegisterFunction(pTable, sithCogFunctionThing_ParseArg, "parsearg")
        || sithCog_RegisterFunction(pTable, sithCogFunctionThing_GetThingRotVel, "getthingrotvel")
        || sithCog_RegisterFunction(pTable, sithCogFunctionThing_SetThingRotVel, "setthingrotvel")
        || sithCog_RegisterFunction(pTable, sithCogFunctionThing_GetRotThrust, "getthingrotthrust")
        || sithCog_RegisterFunction(pTable, sithCogFunctionThing_SetRotThrust, "setthingrotthrust")
        || sithCog_RegisterFunction(pTable, sithCogFunctionThing_SetThingLook, "setthinglook")
        || sithCog_RegisterFunction(pTable, sithCogFunctionThing_SetThingHeadLookPos, "setthingheadlookpos")
        || sithCog_RegisterFunction(pTable, sithCogFunctionThing_SetThingHeadLookThing, "setthingheadlookthing")
        || sithCog_RegisterFunction(pTable, sithCogFunctionThing_IsThingCrouching, "isthingcrouching")
        || sithCog_RegisterFunction(pTable, sithCogFunctionThing_IsThingCrouching, "iscrouching")
        || sithCog_RegisterFunction(pTable, sithCogFunctionThing_GetThingClassCog, "getthingclasscog")
        || sithCog_RegisterFunction(pTable, sithCogFunctionThing_SetThingClassCog, "setthingclasscog")
        || sithCog_RegisterFunction(pTable, sithCogFunctionThing_GetThingCaptureCog, "getthingcapturecog")
        || sithCog_RegisterFunction(pTable, sithCogFunctionThing_SetThingCaptureCog, "setthingcapturecog")
        || sithCog_RegisterFunction(pTable, sithCogFunctionThing_GetThingRespawn, "getthingrespawn")
        || sithCog_RegisterFunction(pTable, sithCogFunctionThing_GetThingSignature, "getthingsignature")
        || sithCog_RegisterFunction(pTable, sithCogFunctionThing_SetThingAttachFlags, "setthingattachflags")
        || sithCog_RegisterFunction(pTable, sithCogFunctionThing_ClearThingAttachFlags, "clearthingattachflags")
        || sithCog_RegisterFunction(pTable, sithCogFunctionThing_GetParticleSize, "getparticlesize")
        || sithCog_RegisterFunction(pTable, sithCogFunctionThing_SetParticleSize, "setparticlesize")
        || sithCog_RegisterFunction(pTable, sithCogFunctionThing_GetParticleGrowthSpeed, "getparticlegrowthspeed")
        || sithCog_RegisterFunction(pTable, sithCogFunctionThing_SetParticleGrowthSpeed, "setparticlegrowthspeed")
        || sithCog_RegisterFunction(pTable, sithCogFunctionThing_GetParticleTimeoutRate, "getparticletimeoutrate")
        || sithCog_RegisterFunction(pTable, sithCogFunctionThing_SetParticleTimeoutRate, "setparticletimeoutrate")
        || sithCog_RegisterFunction(pTable, sithCogFunctionThing_GetTypeFlags, "gettypeflags")
        || sithCog_RegisterFunction(pTable, sithCogFunctionThing_SetTypeFlags, "settypeflags")
        || sithCog_RegisterFunction(pTable, sithCogFunctionThing_ClearTypeFlags, "cleartypeflags")
        || sithCog_RegisterFunction(pTable, sithCogFunctionThing_GetTypeFlags, "getactorflags")
        || sithCog_RegisterFunction(pTable, sithCogFunctionThing_SetTypeFlags, "setactorflags")
        || sithCog_RegisterFunction(pTable, sithCogFunctionThing_ClearTypeFlags, "clearactorflags")
        || sithCog_RegisterFunction(pTable, sithCogFunctionThing_GetTypeFlags, "getweaponflags")
        || sithCog_RegisterFunction(pTable, sithCogFunctionThing_SetTypeFlags, "setweaponflags")
        || sithCog_RegisterFunction(pTable, sithCogFunctionThing_ClearTypeFlags, "clearweaponflags")
        || sithCog_RegisterFunction(pTable, sithCogFunctionThing_GetTypeFlags, "getexplosionflags")
        || sithCog_RegisterFunction(pTable, sithCogFunctionThing_SetTypeFlags, "setexplosionflags")
        || sithCog_RegisterFunction(pTable, sithCogFunctionThing_ClearTypeFlags, "clearexplosionflags")
        || sithCog_RegisterFunction(pTable, sithCogFunctionThing_GetTypeFlags, "getitemflags")
        || sithCog_RegisterFunction(pTable, sithCogFunctionThing_SetTypeFlags, "setitemflags")
        || sithCog_RegisterFunction(pTable, sithCogFunctionThing_ClearTypeFlags, "clearitemflags")
        || sithCog_RegisterFunction(pTable, sithCogFunctionThing_GetTypeFlags, "getparticleflags")
        || sithCog_RegisterFunction(pTable, sithCogFunctionThing_SetTypeFlags, "setparticleflags")
        || sithCog_RegisterFunction(pTable, sithCogFunctionThing_ClearTypeFlags, "clearparticleflags")
        || sithCog_RegisterFunction(pTable, sithCogFunctionThing_TakeItem, "takeitem")
        || sithCog_RegisterFunction(pTable, sithCogFunctionThing_HasLOS, "haslos")
        || sithCog_RegisterFunction(pTable, sithCogFunctionThing_GetFireOffset, "getthingfireoffset")
        || sithCog_RegisterFunction(pTable, sithCogFunctionThing_SetFireOffset, "setthingfireoffset")
        || sithCog_RegisterFunction(pTable, sithCogFunctionThing_GetThingUserData, "getthinguserdata")
        || sithCog_RegisterFunction(pTable, sithCogFunctionThing_SetThingUserData, "setthinguserdata")
        || sithCog_RegisterFunction(pTable, sithCogFunctionThing_GetThingCollideSize, "getthingcollidesize")
        || sithCog_RegisterFunction(pTable, sithCogFunctionThing_SetThingCollideSize, "setthingcollidesize")
        || sithCog_RegisterFunction(pTable, sithCogFunctionThing_GetThingMoveSize, "getthingmovesize")
        || sithCog_RegisterFunction(pTable, sithCogFunctionThing_SetThingMoveSize, "setthingmovesize")
        || sithCog_RegisterFunction(pTable, sithCogFunctionThing_GetThingMass, "getthingmass")
        || sithCog_RegisterFunction(pTable, sithCogFunctionThing_SetThingMass, "setthingmass")
        || sithCog_RegisterFunction(pTable, sithCogFunctionThing_SyncThingPos, "syncthingpos")
        || sithCog_RegisterFunction(pTable, sithCogFunctionThing_SyncThingAttachment, "syncthingattachment")
        || sithCog_RegisterFunction(pTable, sithCogFunctionThing_SyncThingState, "syncthingstate")
        || sithCog_RegisterFunction(pTable, sithCogFunctionThing_AttachThingToThingEx, "attachthingtothingex")
        || sithCog_RegisterFunction(pTable, sithCogFunctionThing_GetThingAttachedThing, "getthingattachedthing")
        || sithCog_RegisterFunction(pTable, sithCogFunctionThing_GetMeshByName, "getmeshbyname")
        || sithCog_RegisterFunction(pTable, sithCogFunctionThing_GetNodeByName, "getnodebyname")
        || sithCog_RegisterFunction(pTable, sithCogFunctionThing_AttachThingToThingMesh, "attachthingtothingmesh")
        || sithCog_RegisterFunction(pTable, sithCogFunctionThing_DetachThingMesh, "detachthingmesh")
        || sithCog_RegisterFunction(pTable, sithCogFunctionThing_SetThingMesh, "setthingmesh")
        || sithCog_RegisterFunction(pTable, sithCogFunctionThing_RestoreThingMesh, "restorethingmesh")
        || sithCog_RegisterFunction(pTable, sithCogFunctionThing_GetThingAlpha, "getthingalpha")
        || sithCog_RegisterFunction(pTable, sithCogFunctionThing_SetThingAlpha, "setthingalpha")
        || sithCog_RegisterFunction(pTable, sithCogFunctionThing_GetCameraFOV, "getcamerafov")
        || sithCog_RegisterFunction(pTable, sithCogFunctionThing_SetCameraFOV, "setcamerafov")
        || sithCog_RegisterFunction(pTable, sithCogFunctionThing_ResetCameraFOV, "resetcamerafov")
        || sithCog_RegisterFunction(pTable, sithCogFunctionThing_SetCameraLookInterp, "setcameralookinterp")
        || sithCog_RegisterFunction(pTable, sithCogFunctionThing_SetCameraPosInterp, "setcameraposinterp")
        || sithCog_RegisterFunction(pTable, sithCogFunctionThing_SetCameraInterpSpeed, "setcamerainterpspeed")
        || sithCog_RegisterFunction(pTable, sithCogFunctionThing_SetCameraPosition, "setcameraposition")
        || sithCog_RegisterFunction(pTable, sithCogFunctionThing_AnimateSpriteSize, "animatespritesize")
        || sithCog_RegisterFunction(pTable, sithCogFunctionThing_GetCameraPosition, "getcameraposition")
        || sithCog_RegisterFunction(pTable, sithCogFunctionThing_SetCameraFadeThing, "setcamerafadething")
        || sithCog_RegisterFunction(pTable, sithCogFunctionThing_SetExtCamOffset, "setextcamoffset")
        || sithCog_RegisterFunction(pTable, sithCogFunctionThing_SetExtCamOffsetToThing, "setextcamoffsettothing")
        || sithCog_RegisterFunction(pTable, sithCogFunctionThing_SetExtCamLookOffsetToThing, "setextcamlookoffsettothing")
        || sithCog_RegisterFunction(pTable, sithCogFunctionThing_SetExtCamLookOffset, "setextcamlookoffset")
        || sithCog_RegisterFunction(pTable, sithCogFunctionThing_RestoreExtCam, "restoreextcam")
        || sithCog_RegisterFunction(pTable, sithCogFunctionThing_PlayForceMoveMode, "playforcemovemode")
        || sithCog_RegisterFunction(pTable, sithCogFunctionThing_IsThingAutoAiming, "isthingautoaiming") // TODO: redundant function
        || sithCog_RegisterFunction(pTable, sithCogFunctionThing_GetMoveStatus, "getmovestatus")
        || sithCog_RegisterFunction(pTable, sithCogFunctionThing_CreateLaser, "createlaser")
        || sithCog_RegisterFunction(pTable, sithCogFunctionThing_CreateLightning, "createlightning")
        || sithCog_RegisterFunction(pTable, sithCogFunctionThing_MakeFairyDust, "makefairydust")
        || sithCog_RegisterFunction(pTable, sithCogFunctionThing_MakeFairyDustDeluxe, "makefairydustdeluxe")
        || sithCog_RegisterFunction(pTable, sithCogFunctionThing_SetPuppetModeFPS, "setpuppetmodefps")
        || sithCog_RegisterFunction(pTable, sithCogFunctionThing_SetMoveMode, "setmovemode")
        || sithCog_RegisterFunction(pTable, sithCogFunctionThing_CheckFloorDistance, "checkfloordistance")
        || sithCog_RegisterFunction(pTable, sithCogFunctionThing_CheckPathToPoint, "checkpathtopoint")
        || sithCog_RegisterFunction(pTable, sithCogFunctionThing_SetThingStateChange, "setthingstatechange")
        || sithCog_RegisterFunction(pTable, sithCogFunctionThing_CreatePolylineThing, "createpolylinething")
        || sithCog_RegisterFunction(pTable, sithCogFunctionThing_StartQuetzAnim, "startquetzanim")
        || sithCog_RegisterFunction(pTable, sithCogFunctionThing_BoardVehicle, "boardvehicle")
        || sithCog_RegisterFunction(pTable, sithCogFunctionThing_FadeInTrack, "fadeintrack")
        || sithCog_RegisterFunction(pTable, sithCogFunctionThing_IsGhostVisible, "isghostvisible")
        || sithCog_RegisterFunction(pTable, sithCogFunctionThing_MakeCamera2LikeCamera1, "makecamera2likecamera1");
}
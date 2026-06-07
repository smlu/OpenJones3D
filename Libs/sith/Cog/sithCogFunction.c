#include "sithCogFunction.h"
#include <j3dcore/j3dhook.h>

#include <rdroid/Math/rdMatrix.h>
#include <rdroid/Math/rdVector.h>
#include <rdroid/types.h>

#include <sith/Cog/sithCog.h>
#include <sith/Cog/sithCogExec.h>
#include <sith/Devices/sithComm.h>
#include <sith/Devices/sithConsole.h>
#include <sith/Devices/sithControl.h>
#include <sith/Dss/sithDSS.h>
#include <sith/Dss/sithDSSCog.h>
#include <sith/Dss/sithGamesave.h>
#include <sith/Engine/sithAnimate.h>
#include <sith/Engine/sithCamera.h>
#include <sith/Engine/sithCollision.h>
#include <sith/Engine/sithPuppet.h>
#include <sith/Gameplay/sithEvent.h>
#include <sith/Gameplay/sithInventory.h>
#include <sith/Gameplay/sithOverlayMap.h>
#include <sith/Gameplay/sithPlayer.h>
#include <sith/Gameplay/sithTime.h>
#include <sith/Main/sithMain.h>
#include <sith/RTI/symbols.h>
#include <sith/World/sithModel.h>
#include <sith/World/sithSector.h>
#include <sith/World/sithTemplate.h>
#include <sith/World/sithThing.h>
#include <sith/World/sithWeapon.h>
#include <sith/World/sithWorld.h>

#include <std/General/stdFnames.h>
#include <std/General/stdMath.h>
#include <std/General/stdMemory.h>
#include <std/General/stdUtil.h>
#include <std/Win95/stdComm.h>
#include <stdbool.h>

static bool bPrintEnabled = false; // Added

void J3DAPI sithCogFunction_EnablePrint(bool bEnable)
{
    bPrintEnabled = bEnable;
}

bool sithCogFunction_IsPrintEnabled(void)
{
    return bPrintEnabled;
}

void J3DAPI EnablePrint(SithCog* pCog)
{
    int curPrint = bPrintEnabled ? 1 : 0;
    sithCogFunction_EnablePrint(sithCogExec_PopInt(pCog) != 0);
    sithCogExec_PushInt(pCog, curPrint);
}

void J3DAPI IsPrintEnabled(SithCog* pCog)
{
    sithCogExec_PushInt(pCog, bPrintEnabled ? 1 : 0);
}

void J3DAPI sithCogFunction_GetSenderID(SithCog* pCog)
{
    sithCogExec_PushInt(pCog, pCog->linkId);
}

void J3DAPI sithCogFunction_GetSenderRef(SithCog* pCog)
{
    sithCogExec_PushInt(pCog, pCog->senderIdx);
}

void J3DAPI sithCogFunction_GetSenderType(SithCog* pCog)
{
    sithCogExec_PushInt(pCog, pCog->senderType);
}

void J3DAPI sithCogFunction_GetSourceRef(SithCog* pCog)
{
    sithCogExec_PushInt(pCog, pCog->sourceIdx);
}

void J3DAPI sithCogFunction_GetSourceType(SithCog* pCog)
{
    sithCogExec_PushInt(pCog, pCog->sourceType);
}

void J3DAPI sithCogFunction_GetSysDate(SithCog* pCog)
{
    SYSTEMTIME date;
    GetLocalTime(&date);

    rdVector3 vec;
    vec.x = (float)date.wYear;
    vec.y = (float)date.wMonth;
    vec.z = (float)date.wDay;
    sithCogExec_PushVector(pCog, &vec);
}

void J3DAPI sithCogFunction_GetSysTime(SithCog* pCog)
{
    SYSTEMTIME time;
    GetLocalTime(&time);

    rdVector3 vec;
    vec.x = (float)time.wHour;
    vec.y = (float)time.wMinute;
    vec.z = (float)time.wSecond;
    sithCogExec_PushVector(pCog, &vec);
}

void J3DAPI sithCogFunction_Sleep(SithCog* pCog)
{
    float secWait = sithCogExec_PopFlex(pCog);
    if ( secWait <= 0.0f )
    {
        SITHLOG_ERROR("Cog %s: Wait time less than 0 in Sleep(); assuming .1 seconds\n", pCog->aName);
        secWait = 0.1f;
    }

    if ( (pCog->flags & SITHCOG_DEBUG) != 0 )
    {
        SITHCONSOLE_PRINTF("Cog %s: Sleeping for %f seconds.\n", pCog->aName, secWait);
    }

    pCog->status = SITHCOG_STATUS_SLEEPING;
    pCog->statusParams[0] = (sithTime_g_msecGameTime + (uint32_t)(secWait * 1000.0f));
}

void J3DAPI sithCogFunction_Print(SithCog* pCog)
{
    const char* pStr = sithCogExec_PopString(pCog);

    // Added: from debug version but modified to require explicit enable
    if ( bPrintEnabled && pStr )
    {
        sithConsole_PrintString(pStr);
    }
}

void J3DAPI sithCogFunction_PrintInt(SithCog* pCog)
{
    char aVecStr[64] = { 0 }; // Added: Init to 0
    int val = sithCogExec_PopInt(pCog);
    STD_FORMAT(aVecStr, "%d", val);

    // Added: from debug version but modified to require explicit enable
    if ( bPrintEnabled )
    {
        sithConsole_PrintString(aVecStr);
    }
}

void J3DAPI sithCogFunction_PrintVector(SithCog* pCog)
{
    rdVector3 vec;
    char aVecStr[64] = { 0 }; // Added: Init to 0
    if ( !sithCogExec_PopVector(pCog, &vec) )
    {
        STD_FORMAT(aVecStr, "Bad vector");
    }
    else
    {
        STD_FORMAT(aVecStr, "<%f %f %f>", vec.x, vec.y, vec.z);
    }

    // Added: from debug version but modified to require explicit enable
    if ( bPrintEnabled )
    {
        sithConsole_PrintString(aVecStr);
    }
}

void J3DAPI sithCogFunction_PrintFlex(SithCog* pCog)
{
    char aFltStr[64] = { 0 }; // Added: Init to 0
    STD_FORMAT(aFltStr, "%f", sithCogExec_PopFlex(pCog));

    // Added: from debug version but modified to require explicit enable
    if ( bPrintEnabled )
    {
        sithConsole_PrintString(aFltStr);
    }
}

void J3DAPI sithCogFunction_PrintHex(SithCog* pCog)
{
    char aHexStr[64] = { 0 }; // Added: Init to 0
    STD_FORMAT(aHexStr, "0x%x", sithCogExec_PopInt(pCog));

    // Added: from debug version but modified to require explicit enable
    if ( bPrintEnabled )
    {
        sithConsole_PrintString(aHexStr);
    }
}

void J3DAPI sithCogFunction_SurfaceAnim(SithCog* pCog)
{
    SithAnimateFlags flags = sithCogExec_PopInt(pCog);
    float speed            = sithCogExec_PopFlex(pCog);
    SithSurface* pSurf     = sithCogExec_PopSurface(pCog);

    if ( !pSurf )
    {
        SITHLOG_ERROR("Cog %s: Bad surface index.\n", pCog->aName);
        sithCogExec_PushInt(pCog, -1);
        return;
    }

    if ( speed <= 0.0f )
    {
        SITHLOG_ERROR("Cog %s: Illegal anim speed %f, changed to 15fps.\n", pCog->aName, speed);
        speed = 15.0f;
    }

    SithAnimationSlot* pAnim = sithAnimate_StartSurfaceAnim(pSurf, speed, flags);
    if ( pAnim )
    {
        sithCogExec_PushInt(pCog, pAnim->animID);
    }
    else
    {
        sithCogExec_PushInt(pCog, -1);
    }
}

void J3DAPI sithCogFunction_MaterialAnim(SithCog* pCog)
{
    int flags        = sithCogExec_PopInt(pCog);
    float fps        = sithCogExec_PopFlex(pCog);
    rdMaterial* pMat = sithCogExec_PopMaterial(pCog);

    if ( !pMat )
    {
        SITHLOG_ERROR("Cog %s: Bad material index.\n", pCog->aName);
        sithCogExec_PushInt(pCog, -1);
        return;
    }

    if ( fps <= 0.0f )
    {
        SITHLOG_ERROR("Cog %s: Illegal anim speed %f, changed to 15fps.\n", pCog->aName, fps);
        fps = 15.0f;
    }

    SithAnimationSlot* pAnim = sithAnimate_StartMaterialAnim(pMat, fps, (SithAnimateFlags)flags);
    if ( pAnim )
    {
        sithCogExec_PushInt(pCog, pAnim->animID);
    }
    else
    {
        sithCogExec_PushInt(pCog, -1);
    }
}

void J3DAPI sithCogFunction_StopMaterialAnim(SithCog* pCog)
{
    rdMaterial* pMaterial = sithCogExec_PopMaterial(pCog);
    if ( !pMaterial )
    {
        SITHLOG_ERROR("Cog %s: Bad material in StopMaterialAnim.\n", pCog->aName);
        return;
    }

    SithAnimationSlot* pAnim = sithAnimate_GetMaterialAnim(pMaterial);
    if ( pAnim )
    {
        sithAnimate_Stop(pAnim);
    }
}

void J3DAPI sithCogFunction_StopAnim(SithCog* pCog)
{
    int animID               = sithCogExec_PopInt(pCog);
    SithAnimationSlot* pAnim = sithAnimate_GetAnim(animID);
    if ( pAnim )
    {
        sithAnimate_Stop(pAnim);
        if ( sithMessage_g_outputstream )
        {
            sithDSS_AnimStatus(pAnim, SITHMESSAGE_SENDTOJOINEDPLAYERS, SITHMESSAGE_STREAM_ALL);
        }
    }
}

void J3DAPI sithCogFunction_StopSurfaceAnim(SithCog* pCog)
{
    SithSurface* pSurf = sithCogExec_PopSurface(pCog);
    if ( !pSurf )
    {
        SITHLOG_ERROR("Cog %s: Trying to call StopSurfaceAnim on an invalid surface.\n", pCog->aName);
        return;
    }

    SithAnimationSlot* pAnim = sithAnimate_GetSurfaceAnim(pSurf);
    if ( !pAnim )
    {
        SITHLOG_ERROR("Tried to stop anim on surface: no anim\n");
        return;
    }

    sithAnimate_Stop(pAnim);
    if ( sithMessage_g_outputstream )
    {
        sithDSS_AnimStatus(pAnim, SITHMESSAGE_SENDTOJOINEDPLAYERS, SITHMESSAGE_STREAM_ALL);
    }
}

void J3DAPI sithCogFunction_GetSurfaceAnim(SithCog* pCog)
{
    SithSurface* pSurf = sithCogExec_PopSurface(pCog);
    if ( !pSurf )
    {
        SITHLOG_ERROR("Cog %s: Trying to call GetSurfaceAnim on an invalid surface.\n", pCog->aName);
        sithCogExec_PushInt(pCog, -1);
        return;
    }

    int animNum = sithAnimate_GetSurfaceAnimIndex(pSurf);
    sithCogExec_PushInt(pCog, animNum);
}

void J3DAPI sithCogFunction_LoadTemplate(SithCog* pCog)
{
    const char* pTemplateName = sithCogExec_PopString(pCog);

    SithThing* pTemplate;
    if ( !pTemplateName || (pTemplate = sithTemplate_GetTemplate(pTemplateName)) == NULL )
    {
        sithCogExec_PushInt(pCog, -1);
    }
    else
    {
        sithCogExec_PushInt(pCog, pTemplate->idx);
    }
}

void J3DAPI sithCogFunction_LoadKeyframe(SithCog* pCog)
{
    const char* pKeyFilename = sithCogExec_PopString(pCog);

    rdKeyframe* pKeyframe;
    if ( !pKeyFilename || (pKeyframe = sithPuppet_LoadKeyframe(pKeyFilename)) == NULL )
    {
        sithCogExec_PushInt(pCog, -1);
    }
    else
    {
        sithCogExec_PushInt(pCog, pKeyframe->idx);
    }
}

void J3DAPI sithCogFunction_LoadModel(SithCog* pCog)
{
    const char* pFilename = sithCogExec_PopString(pCog);

    rdModel3* pModel;
    if ( !pFilename || (pModel = sithModel_Load(pFilename, 1)) == NULL )
    {
        sithCogExec_PushInt(pCog, -1);
    }
    else
    {
        sithCogExec_PushInt(pCog, pModel->num);
    }
}

void J3DAPI sithCogFunction_SetPulse(SithCog* pCog)
{
    float interval = sithCogExec_PopFlex(pCog);

    if ( interval <= 0.0f )
    {
        if ( (pCog->flags & SITHCOG_DEBUG) != 0 )
        {
            SITHCONSOLE_PRINTF("Cog %s: Pulse disabled.\n", pCog->aName);
        }
        pCog->flags &= ~SITHCOG_PULSE_SET;
    }
    else
    {
        if ( (pCog->flags & SITHCOG_DEBUG) != 0 )
        {
            SITHCONSOLE_PRINTF("Cog %s: Pulse set to %f seconds.\n", pCog->aName, interval);
        }

        pCog->flags |= SITHCOG_PULSE_SET;
        pCog->msecPulseInterval = (uint32_t)(interval * 1000.0f);
        pCog->msecNextPulseTime = pCog->msecPulseInterval + sithTime_g_msecGameTime;
    }
}

void J3DAPI sithCogFunction_SetTimer(SithCog* pCog)
{
    float when = sithCogExec_PopFlex(pCog);

    if ( when <= 0.0f )
    {
        if ( (pCog->flags & SITHCOG_DEBUG) != 0 )
        {
            SITHCONSOLE_PRINTF("Cog %s: Timer cancelled.\n", pCog->aName);
        }

        pCog->flags &=  ~SITHCOG_TIMER_SET;
    }
    else
    {
        if ( (pCog->flags & SITHCOG_DEBUG) != 0 )
        {
            SITHCONSOLE_PRINTF("Cog %s: Timer set for %f seconds.\n", pCog->aName, when);
        }

        pCog->flags |= SITHCOG_TIMER_SET;
        pCog->msecTimerTimeout = sithTime_g_msecGameTime + (uint32_t)(when * 1000.0f);
    }
}

void J3DAPI sithCogFunction_SetTimerEx(SithCog* pCog)
{
    SithEventParams params;
    params.param3 = sithCogExec_PopInt(pCog);
    params.param2 = sithCogExec_PopInt(pCog);
    params.param1 = sithCogExec_PopInt(pCog);
    params.idx = pCog->idx;
    int msWhen = (int)(sithCogExec_PopFlex(pCog) * 1000.0f);

    if ( msWhen >= 0 )
    {
        sithEvent_CreateEvent(4u, &params, msWhen);
    }
}

void J3DAPI sithCogFunction_KillTimerEx(SithCog* pCog)
{
    int timerId = sithCogExec_PopInt(pCog);
    if ( timerId > 0 )
    {
        SithEvent* pPrevEvent = NULL;
        SithEvent* pNextEvent = NULL;
        for ( SithEvent* pCurEvent = sithEvent_g_pFirstQueuedEvent; pCurEvent; pCurEvent = pNextEvent )
        {
            pNextEvent = pCurEvent->pNextEvent;
            if ( pCurEvent->taskNum == SITHCOG_TASKID && pCurEvent->params.idx == pCog->idx && pCurEvent->params.param1 == timerId )
            {
                if ( pPrevEvent )
                {
                    pPrevEvent->pNextEvent = pNextEvent;
                }
                else
                {
                    sithEvent_g_pFirstQueuedEvent = pCurEvent->pNextEvent;
                }

                sithEvent_FreeEvent(pCurEvent);
                pCurEvent = pPrevEvent;
            }

            pPrevEvent = pCurEvent;
        }
    }
}

void J3DAPI sithCogFunction_Reset(SithCog* pCog)
{
    pCog->callDepth = 0;
}

void J3DAPI sithCogFunction_VectorSet(SithCog* pCog)
{
    rdVector3 vec;
    vec.z = sithCogExec_PopFlex(pCog);
    vec.y = sithCogExec_PopFlex(pCog);
    vec.x = sithCogExec_PopFlex(pCog);
    sithCogExec_PushVector(pCog, &vec);
}

void J3DAPI sithCogFunction_VectorAdd(SithCog* pCog)
{
    rdVector3 b;
    sithCogExec_PopVector(pCog, &b);

    rdVector3 a;
    sithCogExec_PopVector(pCog, &a);

    rdVector3 c;
    rdVector_Add3(&c, &a, &b);
    sithCogExec_PushVector(pCog, &c);
}

void J3DAPI sithCogFunction_VectorSub(SithCog* pCog)
{
    rdVector3 b;
    sithCogExec_PopVector(pCog, &b);

    rdVector3 a;
    sithCogExec_PopVector(pCog, &a);

    rdVector3 c;
    rdVector_Sub3(&c, &a, &b);
    sithCogExec_PushVector(pCog, &c);
}

void J3DAPI sithCogFunction_VectorDot(SithCog* pCog)
{
    rdVector3 b;
    sithCogExec_PopVector(pCog, &b);

    rdVector3 a;
    sithCogExec_PopVector(pCog, &a);

    float c = rdVector_Dot3(&a, &b);
    sithCogExec_PushFlex(pCog, c);
}

void J3DAPI sithCogFunction_VectorCross(SithCog* pCog)
{
    rdVector3 b;
    sithCogExec_PopVector(pCog, &b);

    rdVector3 a;
    sithCogExec_PopVector(pCog, &a);

    rdVector3 c;
    rdVector_Cross3(&c, &a, &b); // Note: in debug version is 'a cross b' but in release version 'b cross a'
    sithCogExec_PushVector(pCog, &c);
}

void J3DAPI sithCogFunction_VectorLen(SithCog* pCog)
{
    rdVector3 vec;
    sithCogExec_PopVector(pCog, &vec);
    sithCogExec_PushFlex(pCog, rdVector_Len3(&vec));
}

void J3DAPI sithCogFunction_VectorScale(SithCog* pCog)
{
    rdVector3 a;
    float scalar = sithCogExec_PopFlex(pCog);
    sithCogExec_PopVector(pCog, &a);

    rdVector3 b;
    rdVector_Scale3(&b, &a, scalar);
    sithCogExec_PushVector(pCog, &b);
}

void J3DAPI sithCogFunction_VectorDist(SithCog* pCog)
{
    rdVector3 b;
    sithCogExec_PopVector(pCog, &b);

    rdVector3 a;
    sithCogExec_PopVector(pCog, &a);

    float dist = rdVector_Dist3(&a, &b);
    sithCogExec_PushFlex(pCog, dist);
}

void J3DAPI sithCogFunction_VectorEqual(SithCog* pCog)
{
    rdVector3 b;
    sithCogExec_PopVector(pCog, &b);

    rdVector3 a;
    sithCogExec_PopVector(pCog, &a);

    if ( a.x == b.x && a.y == b.y && a.z == b.z )
    {
        sithCogExec_PushInt(pCog, 1);
    }
    else
    {
        sithCogExec_PushInt(pCog, 0);
    }
}

void J3DAPI sithCogFunction_VectorRotate(SithCog* pCog)
{
    rdVector3 pyr;
    sithCogExec_PopVector(pCog, &pyr);

    rdVector3 vec;
    sithCogExec_PopVector(pCog, &vec);

    rdVector_Rotate3Acc(&vec, &pyr);
    sithCogExec_PushVector(pCog, &vec);
}

void J3DAPI sithCogFunction_VectorTransformToOrient(SithCog* pCog)
{
    rdVector3 vec;
    int bVec = sithCogExec_PopVector(pCog, &vec);
    SithThing* pThing = sithCogExec_PopThing(pCog);
    if ( !pThing || !bVec )
    {
        SITHLOG_ERROR("Cog %s: Bad parameters in VectorTransformToOrient() call.\n", pCog->aName);
        sithCogExec_PushVector(pCog, &rdroid_g_zeroVector3);
        return;
    }

    rdMatrix_TransformVector34Acc(&vec, &pThing->orient);
    sithCogExec_PushVector(pCog, &vec);
}

void J3DAPI sithCogFunction_SendMessage(SithCog* pCog)
{
    SithCogMsgType msgType = sithCogExec_PopInt(pCog);
    SithCog* pDstCog  = sithCogExec_PopCog(pCog);
    if ( !pDstCog )
    {
        SITHLOG_ERROR("Cog %s: Trying to send message to invalid cog.\n", pCog->aName); // Fixed: Removed %s from the string at the end
        return;
    }

    if ( (unsigned int)msgType > SITHCOG_MSG_UPDATEWPNTS )
    {
        SITHLOG_ERROR("Cog %s: Trying to send invalid message %d\n", pCog->aName, msgType);
        return;
    }

    sithCog_SendMessage(pDstCog, msgType, SITHCOG_SYM_REF_COG, pCog->idx, pCog->sourceType, pCog->sourceIdx, 0);
}

void J3DAPI sithCogFunction_SendMessageEx(SithCog* pCog)
{
    int param3 = sithCogExec_PopInt(pCog);
    int param2 = sithCogExec_PopInt(pCog);
    int param1 = sithCogExec_PopInt(pCog);
    int param0 = sithCogExec_PopInt(pCog);

    SithCogMsgType msg = sithCogExec_PopInt(pCog);
    SithCog* pDstCog   = sithCogExec_PopCog(pCog);

    if ( !pDstCog )
    {
        SITHLOG_ERROR("Cog %s: Trying to send message to invalid cog.\n", pCog->aName);
        return;
    }

    if ( msg > (unsigned int)SITHCOG_MSG_UPDATEWPNTS )
    {
        SITHLOG_ERROR("Cog %s: Trying to send invalid message %d\n", pCog->aName, msg);
        return;
    }

    int result = sithCog_SendMessageEx(pDstCog, msg, SITHCOG_SYM_REF_COG, pCog->idx, pCog->sourceType, pCog->sourceIdx, 0, param0, param1, param2, param3);
    sithCogExec_PushInt(pCog, result);
}

void J3DAPI sithCogFunction_GetKeyLen(SithCog* pCog)
{
    rdKeyframe* pKeyframe = sithCogExec_PopKeyframe(pCog);
    if ( !pKeyframe || pKeyframe->fps == 0.0f )
    {
        SITHLOG_ERROR("Cog %s: Bad Track reference passed to GetKeyLen.\n", pCog->aName);
        sithCogExec_PushFlex(pCog, 0.0f);
        return;
    }

    SITHLOG_DEBUG("Cog %s: GetKeyLen %s %d %f.\n", pCog->aName, pKeyframe->aName, pKeyframe->numFrames, pKeyframe->fps);
    float len = (float)pKeyframe->numFrames / pKeyframe->fps;
    sithCogExec_PushFlex(pCog, len);
}

void J3DAPI sithCogFunction_GetSithMode(SithCog* pCog)
{
    sithCogExec_PushInt(pCog, sithMain_g_sith_mode.masterMode);
}

void J3DAPI sithCogFunction_GetGameTime(SithCog* pCog)
{
    sithCogExec_PushInt(pCog, sithTime_g_msecGameTime);
}

void J3DAPI sithCogFunction_GetFlexGameTime(SithCog* pCog)
{
    sithCogExec_PushFlex(pCog, sithTime_g_secGameTime);
}

void J3DAPI sithCogFunction_GetDifficulty(SithCog* pCog)
{
    sithCogExec_PushInt(pCog, sithMain_g_sith_mode.difficulty);
}

void J3DAPI sithCogFunction_SetSubModeFlags(SithCog* pCog)
{
    sithMain_g_sith_mode.subModeFlags |= sithCogExec_PopInt(pCog);
}

void J3DAPI sithCogFunction_ClearSubModeFlags(SithCog* pCog)
{
    sithMain_g_sith_mode.subModeFlags &= ~sithCogExec_PopInt(pCog);
}

void J3DAPI sithCogFunction_GetSubModeFlags(SithCog* pCog)
{
    sithCogExec_PushInt(pCog, sithMain_g_sith_mode.subModeFlags);
}

void J3DAPI sithCogFunction_SetDebugModeFlags(SithCog* pCog)
{
    sithMain_g_sith_mode.debugModeFlags |= sithCogExec_PopInt(pCog);
}

void J3DAPI sithCogFunction_ClearDebugModeFlags(SithCog* pCog)
{
    sithMain_g_sith_mode.debugModeFlags &= ~sithCogExec_PopInt(pCog);
}

void J3DAPI sithCogFunction_GetDebugModeFlags(SithCog* pCog)
{
    sithCogExec_PushInt(pCog, sithMain_g_sith_mode.debugModeFlags);
}

void J3DAPI sithCogFunction_BitSet(SithCog* pCog)
{
    int mask  = sithCogExec_PopInt(pCog);
    int flags = sithCogExec_PopInt(pCog);
    sithCogExec_PushInt(pCog, flags | mask);
}

void J3DAPI sithCogFunction_BitTest(SithCog* pCog)
{
    int mask = sithCogExec_PopInt(pCog);
    int flags = sithCogExec_PopInt(pCog);
    sithCogExec_PushInt(pCog, flags & mask);
}

void J3DAPI sithCogFunction_BitClear(SithCog* pCog)
{
    int mask = sithCogExec_PopInt(pCog);
    int flags = sithCogExec_PopInt(pCog);
    sithCogExec_PushInt(pCog, flags & ~mask);
}

void J3DAPI sithCogFunction_GetLevelTime(SithCog* pCog)
{
    float secTime = (float)sithTime_g_msecGameTime * 0.001f;
    sithCogExec_PushFlex(pCog, secTime);
}

void J3DAPI sithCogFunction_GetThingCount(SithCog* pCog)
{
    sithCogExec_PushInt(pCog, sithWorld_g_pCurrentWorld->numThings);
}

void J3DAPI sithCogFunction_GetThingTemplateCount(SithCog* pCog)
{
    SithThing* pTemplate  = sithCogExec_PopTemplate(pCog);
    if ( !pTemplate )
    {
        SITHLOG_ERROR("Cog %s: Bad template reference passed to GetThingTemplateCount.\n", pCog->aName);
        return;
    }

    size_t count = 0;
    for ( int i = 0; i < sithWorld_g_pCurrentWorld->lastThingIdx; ++i )
    {
        SithThing* pThing = &sithWorld_g_pCurrentWorld->aThings[i];
        if ( pThing->type && pThing->type != SITH_THING_CORPSE && pThing->pTemplate == pTemplate )
        {
            count++;
        }
    }

    sithCogExec_PushInt(pCog, count);
}

void J3DAPI sithCogFunction_GetGravity(SithCog* pCog)
{
    sithCogExec_PushFlex(pCog, sithWorld_g_pCurrentWorld->gravity);
}

void J3DAPI sithCogFunction_SetGravity(SithCog* pCog)
{
    sithWorld_g_pCurrentWorld->gravity = sithCogExec_PopFlex(pCog);
}

void J3DAPI sithCogFunction_ReturnEx(SithCog* pCog)
{
    pCog->returnValue = sithCogExec_PopInt(pCog);
}

void J3DAPI sithCogFunction_GetParam(SithCog* pCog)
{
    size_t idx = sithCogExec_PopInt(pCog);
    if ( idx >= STD_ARRAYLEN(pCog->params) )
    {
        sithCogExec_PushInt(pCog, -9999);
    }
    else
    {
        sithCogExec_PushInt(pCog, pCog->params[idx]);
    }
}

void J3DAPI sithCogFunction_SetParam(SithCog* pCog)
{
    int val    = sithCogExec_PopInt(pCog);
    size_t num = sithCogExec_PopInt(pCog);
    if ( num < STD_ARRAYLEN(pCog->params) )
    {
        pCog->params[num] = val;
    }
}

void J3DAPI sithCogFunction_VectorX(SithCog* pCog)
{
    rdVector3 vec;

    sithCogExec_PopVector(pCog, &vec);
    sithCogExec_PushFlex(pCog, vec.x);
}

void J3DAPI sithCogFunction_VectorY(SithCog* pCog)
{
    rdVector3 vec;

    sithCogExec_PopVector(pCog, &vec);
    sithCogExec_PushFlex(pCog, vec.y);
}

void J3DAPI sithCogFunction_VectorZ(SithCog* pCog)
{
    rdVector3 vec;

    sithCogExec_PopVector(pCog, &vec);
    sithCogExec_PushFlex(pCog, vec.z);
}

void J3DAPI sithCogFunction_VectorNorm(SithCog* pCog)
{
    rdVector3 vec;
    rdVector3 dest;

    sithCogExec_PopVector(pCog, &vec);
    rdVector_Normalize3(&dest, &vec);
    sithCogExec_PushVector(pCog, &dest);
}

void J3DAPI sithCogFunction_SetMaterialCel(SithCog* pCog)
{
    int celNum       = sithCogExec_PopInt(pCog);
    rdMaterial* pMat = sithCogExec_PopMaterial(pCog);

    if ( !pMat || celNum < 0 || (size_t)celNum >= pMat->numCels )
    {
        sithCogExec_PushInt(pCog, -1);
        return;
    }

    int curCelNum = pMat->curCelNum;
    pMat->curCelNum = celNum;
    sithCogExec_PushInt(pCog, curCelNum);
}

void J3DAPI sithCogFunction_GetMaterialCel(SithCog* pCog)
{
    sithCogExec_PopMaterial(pCog);
    sithCogExec_PushInt(pCog, -1); // TODO: ?? why material cell number is not returned
}

void J3DAPI sithCogFunction_SetInvFlags(SithCog* pCog)
{
    int flags  = sithCogExec_PopInt(pCog);
    size_t bin = sithCogExec_PopInt(pCog);
    SithThing* pThing = sithCogExec_PopThing(pCog);

    if ( !pThing || pThing->type != SITH_THING_PLAYER || !pThing->thingInfo.itemInfo.aBackpackItems[14].typeId )
    {
        SITHLOG_ERROR("Cog %s: Invalid thing in SetInvFlags.\n", pCog->aName);
        return;
    }

    if ( bin >= SITHINVENTORY_MAXTYPES )
    {
        SITHLOG_ERROR("Cog %s: Invalid inventory id in SetInvFlags.\n", pCog->aName);
        return;
    }

    sithInventory_SetInventoryFlags(pThing, bin, (SithInventoryTypeFlag)flags);
}

void J3DAPI sithCogFunction_SetMapModeFlags(SithCog* pCog)
{
    sithMain_g_sith_mode.mapModeFlags |= sithCogExec_PopInt(pCog);
}

void J3DAPI sithCogFunction_GetMapModelFlags(SithCog* pCog)
{
    sithCogExec_PushInt(pCog, sithMain_g_sith_mode.mapModeFlags);
}

void J3DAPI sithCogFunction_ClearMapModeFlags(SithCog* pCog)
{
    sithMain_g_sith_mode.mapModeFlags &= ~sithCogExec_PopInt(pCog);
}

void J3DAPI sithCogFunction_SetCameraFocus(SithCog* pCog)
{
    SithThing* pThing = sithCogExec_PopThing(pCog);
    size_t camNum     = sithCogExec_PopInt(pCog);

    if ( camNum >= STD_ARRAYLEN(sithCamera_g_aCameras) || !pThing )
    {
        SITHLOG_ERROR("Cog %s: Invalid camera or focus in SetCameraFocus().\n", pCog->aName);
        return;
    }

    SithCamera* pCamera = &sithCamera_g_aCameras[camNum];
    if ( camNum == SITHCAMERA_CINEMACAMERANUM && pCamera->bPosInterp )
    {
        if ( pCamera->pPrimaryFocusThing )
        {
            pCamera->pos = pCamera->pPrimaryFocusThing->pos;
            pCamera->posInterpState = 0;
        }
    }

    sithCamera_SetCameraFocus(pCamera, pThing, NULL);
}

void J3DAPI sithCogFunction_SetCameraSecondaryFocus(SithCog* pCog)
{
    SithThing* pFocus = sithCogExec_PopThing(pCog);
    size_t camNum     = sithCogExec_PopInt(pCog);

    if ( camNum >= STD_ARRAYLEN(sithCamera_g_aCameras) || !pFocus )
    {
        SITHLOG_ERROR("Cog %s: Invalid camera or focus in SetCameraFocus().\n", pCog->aName);
        return;
    }
    sithCamera_SetCameraFocus(&sithCamera_g_aCameras[camNum], 0, pFocus);
}

void J3DAPI sithCogFunction_GetPrimaryFocus(SithCog* pCog)
{
    size_t camNum = sithCogExec_PopInt(pCog);
    if ( camNum >= STD_ARRAYLEN(sithCamera_g_aCameras) )
    {
        sithCogExec_PushInt(pCog, -1);
        SITHLOG_ERROR("Cog %s: Invalid camera or focus in GetPrimaryFocus().\n", pCog->aName);
        return;
    }

    SithThing* pFocus = sithCamera_GetPrimaryFocus(&sithCamera_g_aCameras[camNum]);
    if ( pFocus )
    {
        sithCogExec_PushInt(pCog, pFocus->idx);
    }
    else
    {
        sithCogExec_PushInt(pCog, -1);
    }
}

void J3DAPI sithCogFunction_GetSecondaryFocus(SithCog* pCog)
{
    size_t camNum = sithCogExec_PopInt(pCog);
    if ( camNum >= STD_ARRAYLEN(sithCamera_g_aCameras) )
    {
        sithCogExec_PushInt(pCog, -1);
        SITHLOG_ERROR("Cog %s: Invalid camera or focus in GetSecondaryFocus().\n", pCog->aName);
        return;
    }

    SithThing* pFocus = sithCamera_GetSecondaryFocus(&sithCamera_g_aCameras[camNum]);
    if ( pFocus )
    {
        sithCogExec_PushInt(pCog, pFocus->idx);
    }
    else
    {
        sithCogExec_PushInt(pCog, -1);
    }
}

void J3DAPI sithCogFunction_SetCameraMode(SithCog* pCog)
{
    size_t camNum = sithCogExec_PopInt(pCog);
    if ( camNum >= STD_ARRAYLEN(sithCamera_g_aCameras) )
    {
        SITHLOG_ERROR("Cog %s: Invalid camera in SetCameraMode().\n", pCog->aName);
        return;
    }

    sithCamera_SetCurrentCamera(&sithCamera_g_aCameras[camNum]);
}

void J3DAPI sithCogFunction_GetCameraMode(SithCog* pCog)
{
    if ( !sithCamera_g_pCurCamera || sithCamera_g_pCurCamera - sithCamera_g_aCameras >= STD_ARRAYLEN(sithCamera_g_aCameras) )
    {
        sithCogExec_PushInt(pCog, -1);
    }
    else
    {
        sithCogExec_PushInt(pCog, sithCamera_g_pCurCamera - sithCamera_g_aCameras);
    }
}

void J3DAPI sithCogFunction_CycleCamera(SithCog* pCog)
{
    J3D_UNUSED(pCog);
    sithCamera_CycleCamera();
}

void J3DAPI sithCogFunction_SetPOVShake(SithCog* pCog)
{
    float angDelta = sithCogExec_PopFlex(pCog);
    float posDelta = sithCogExec_PopFlex(pCog);

    rdVector3 angleOffset;
    if ( !sithCogExec_PopVector(pCog, &angleOffset) )
    {
        SITHLOG_ERROR("Cog %s: Invalid vector in SetPOVShake().\n", pCog->aName);
        return;
    }

    rdVector3 posOffset;
    if ( !sithCogExec_PopVector(pCog, &posOffset) )
    {
        SITHLOG_ERROR("Cog %s: Invalid vector in SetPOVShake().\n", pCog->aName);
        return;
    }

    sithCamera_SetPOVShake(&posOffset, &angleOffset, posDelta, angDelta);
}

void J3DAPI sithCogFunction_HeapNew(SithCog* pCog)
{
    int size = sithCogExec_PopInt(pCog);
    if ( size > 0 )
    {
        if ( pCog->aHeap )
        {
            STDFREE(pCog->aHeap);
            pCog->heapSize = 0;
        }

        pCog->aHeap = (SithCogSymbolValue*)STDMALLOC(sizeof(SithCogSymbolValue) * size);
        STD_ZEROMEM(pCog->aHeap, sizeof(SithCogSymbolValue) * size);
        pCog->heapSize = size;
    }
}

void J3DAPI sithCogFunction_HeapSet(SithCog* pCog)
{
    SithCogSymbolValue val;
    int bValidSym = sithCogExec_PopSymbol(pCog, &val);
    size_t idx = sithCogExec_PopInt(pCog);

    if ( bValidSym && idx < pCog->heapSize )
    {
        pCog->aHeap[idx] = val;
    }
}

void J3DAPI sithCogFunction_HeapGet(SithCog* pCog)
{
    size_t num = sithCogExec_PopInt(pCog);
    if ( num >= pCog->heapSize )
    {
        SITHLOG_ERROR("HeapGet: index %d out of range.\n", (int)num);
        sithCogExec_PushInt(pCog, 0);
        return;
    }

    SITH_ASSERTREL(pCog->aHeap);
    SithCogSymbolValue value = pCog->aHeap[num]; // TODO: Don't deep copy, just use the pointer
    sithCogExec_PushStack(pCog, &value);
}

void J3DAPI sithCogFunction_HeapFree(SithCog* pCog)
{
    if ( pCog->aHeap )
    {
        STDFREE(pCog->aHeap);
        pCog->heapSize = 0;
    }
}

void J3DAPI sithCogFunction_GetSelfCog(SithCog* pCog)
{
    sithCogExec_PushInt(pCog, pCog->idx);
}

void J3DAPI sithCogFunction_GetMasterCog(SithCog* pCog)
{
    if ( sithCog_g_pMasterCog )
    {
        sithCogExec_PushInt(pCog, sithCog_g_pMasterCog->idx);
    }
    else
    {
        sithCogExec_PushInt(pCog, -1);
    }
}

void J3DAPI sithCogFunction_SetMasterCog(SithCog* pCog)
{
    sithCog_g_pMasterCog = sithCogExec_PopCog(pCog);
}

void J3DAPI sithCogFunction_GetCogByIndex(SithCog* pCog)
{
    // Function retrieves cog from static resources (jones3Dstatic)
    int idx = sithCogExec_PopInt(pCog);
    idx     = SITHWORLD_STATICINDEX(idx);

    SithCog* pRetCog = sithCog_GetCogByIndex(idx);
    if ( pRetCog )
    {
        sithCogExec_PushInt(pCog, pRetCog->idx);
    }
    else
    {
        sithCogExec_PushInt(pCog, -1);
    }
}

void J3DAPI sithCogFunction_FireProjectile(SithCog* pCog)
{
    float autoAimFovZ = sithCogExec_PopFlex(pCog);
    float autoAimFovX = sithCogExec_PopFlex(pCog);
    SithFireProjectileFlags flags = sithCogExec_PopInt(pCog);
    float extra = sithCogExec_PopFlex(pCog);

    rdVector3 fireError;
    sithCogExec_PopVector(pCog, &fireError);

    rdVector3 fireOffset;
    sithCogExec_PopVector(pCog, &fireOffset);

    SithPuppetSubMode submode      = sithCogExec_PopInt(pCog);
    tSoundHandle hFireSnd          = sithCogExec_PopSound(pCog);
    SithThing* pProjectileTemplate = sithCogExec_PopTemplate(pCog);
    SithThing* pShooter            = sithCogExec_PopThing(pCog);

    SithThing* pProjectile;
    if ( !pShooter
        || (pProjectile = sithWeapon_FireProjectile(pShooter, pProjectileTemplate, hFireSnd, submode, &fireOffset, &fireError, extra, flags, autoAimFovX, autoAimFovZ)) == NULL )
    {
        sithCogExec_PushInt(pCog, -1);
    }
    else
    {
        sithCogExec_PushInt(pCog, pProjectile->idx);
    }
}

void J3DAPI sithCogFunction_SendTrigger(SithCog* pCog)
{
    int param3 = sithCogExec_PopInt(pCog);
    int param2 = sithCogExec_PopInt(pCog);
    int param1 = sithCogExec_PopInt(pCog);
    int param0 = sithCogExec_PopInt(pCog);

    int srcIdx = sithCogExec_PopInt(pCog);
    SithThing* pThing = sithCogExec_PopThing(pCog);

    if ( pThing )
    {
        if ( pThing->type == SITH_THING_PLAYER
            && pThing->thingInfo.actorInfo.pPlayer
            && (pThing->thingInfo.actorInfo.pPlayer->flags & SITH_PLAYER_JOINEDGAME) != 0 )
        {
            if ( pThing == sithPlayer_g_pLocalPlayerThing )
            {
                sithCog_BroadcastMessageEx(SITHCOG_MSG_TRIGGER, SITHCOG_SYM_REF_THING, sithPlayer_g_pLocalPlayerThing->idx, SITHCOG_SYM_REF_NONE, srcIdx, param0, param1, param2, param3);
            }
            else
            {
                sithDSSCog_SendMessage(
                    NULL,
                    SITHCOG_MSG_TRIGGER,
                    SITHCOG_SYM_REF_THING,
                    sithPlayer_g_pLocalPlayerThing->idx,
                    SITHCOG_SYM_REF_NONE,
                    srcIdx,
                    0,
                    param0,
                    param1,
                    param2,
                    param3,
                    pThing->thingInfo.actorInfo.pPlayer->playerNetId
                );
            }
        }
    }
    else
    {
        sithDSSCog_SendMessage(
            NULL,
            SITHCOG_MSG_TRIGGER,
            SITHCOG_SYM_REF_THING,
            sithPlayer_g_pLocalPlayerThing->idx,
            SITHCOG_SYM_REF_NONE,
            srcIdx,
            0,
            param0,
            param1,
            param2,
            param3,
            SITHMESSAGE_SENDTOJOINEDPLAYERS
        );

        sithCog_BroadcastMessageEx(SITHCOG_MSG_TRIGGER, SITHCOG_SYM_REF_THING, sithPlayer_g_pLocalPlayerThing->idx, SITHCOG_SYM_REF_NONE, srcIdx, param0, param1, param2, param3);
    }
}

void J3DAPI sithCogFunction_ActivateWeapon(SithCog* pCog)
{
    float timeToWait  = sithCogExec_PopFlex(pCog);
    SithThing* pThing = sithCogExec_PopThing(pCog);

    if ( !pThing || timeToWait < 0.0f )
    {
        SITHLOG_ERROR("Cog %s: Bad call to ActivateWeapon.\n", pCog->aName);
    }
    else
    {
        sithWeapon_ActivateWeapon(pThing, pCog, timeToWait);
    }
}

void J3DAPI sithCogFunction_DeactivateWeapon(SithCog* pCog)
{
    SithThing* pThing = sithCogExec_PopThing(pCog);
    if ( !pThing )
    {
        SITHLOG_ERROR("Cog %s: Bad call to DeactivateWeapon.\n", pCog->aName);

        sithCogExec_PushFlex(pCog, -1.0f);
    }
    else
    {
        float secTime = sithWeapon_DeactivateWeapon(pThing);
        sithCogExec_PushFlex(pCog, secTime);
    }
}

void J3DAPI sithCogFunction_DeactivateCurrentWeapon(SithCog* pCog)
{
    SithThing* pThing = sithCogExec_PopThing(pCog);
    if ( pThing )
    {
        sithWeapon_DeactivateCurrentWeapon(pThing);
    }
}

void J3DAPI sithCogFunction_SetFireWait(SithCog* pCog)
{
    float waitTime = sithCogExec_PopFlex(pCog);
    SithThing* pThing = sithCogExec_PopThing(pCog);

    if ( !pThing || pThing->type != SITH_THING_PLAYER && pThing->type != SITH_THING_ACTOR || waitTime < -1.0f )
    {
        SITHLOG_ERROR("Cog %s: Bad call to SetFireWait.\n", pCog->aName);
    }
    else
    {
        sithWeapon_SetFireWait(pThing, waitTime);
    }
}

void J3DAPI sithCogFunction_SetMountWait(SithCog* pCog)
{
    float timeToWait  = sithCogExec_PopFlex(pCog);
    SithThing* pThing = sithCogExec_PopThing(pCog);

    SITH_ASSERTREL(pThing != NULL);
    SITH_ASSERTREL(pThing->type == SITH_THING_PLAYER);
    SITH_ASSERTREL(timeToWait >= -((float)1.0));

    sithWeapon_SetMountWait(timeToWait);
}

void J3DAPI sithCogFunction_SetAimWait(SithCog* pCog)
{
    float timeToWait  = sithCogExec_PopFlex(pCog);
    SITH_ASSERTREL(timeToWait >= -1.0f); // Changed: Moved up here

    SithThing* pThing = sithCogExec_PopThing(pCog);
    SITH_ASSERTREL(pThing != NULL);
    SITH_ASSERTREL((pThing->type == SITH_THING_PLAYER) || (pThing->type == SITH_THING_ACTOR));

    sithWeapon_SetAimWait(pThing, timeToWait);
}

void J3DAPI sithCogFunction_SelectWeapon(SithCog* pCog)
{
    int weaponID       = sithCogExec_PopInt(pCog);
    SithThing* pThing  = sithCogExec_PopThing(pCog);
    SITH_ASSERTREL(pThing);
    SITH_ASSERTREL((pThing->type == SITH_THING_PLAYER) || (pThing->type == SITH_THING_ACTOR));

    // Removed the debug mode check
    // TODO: remove this commented scope
    // if ( (sithMain_g_sith_mode.debugModeFlags & SITHDEBUG_INEDITOR) != 0 )
    // {
    //     sithCogExec_PushInt(pCog, 1);
    //     return;
    // }

    if ( !sithInventory_IsWeapon(weaponID) )
    {
        SITHLOG_ERROR("Cog %s: Bad weapon index in SelectWeapon().\n", pCog->aName);
        sithCogExec_PushInt(pCog, 0);
        return;
    }

    if ( sithWeapon_SelectWeapon(pThing, (SithWeaponId)weaponID) )
    {
        sithCogExec_PushInt(pCog, 1);
        return;
    }

    sithCogExec_PushInt(pCog, 0);
}

void J3DAPI sithCogFunction_SelectWeaponWait(SithCog* pCog)
{
    int typeID        = sithCogExec_PopInt(pCog);
    SithThing* pThing = sithCogExec_PopThing(pCog);

    // Removed the debug mode check
    // TODO: remove this commented scope
    // if ( (sithMain_g_sith_mode.debugModeFlags & SITHDEBUG_INEDITOR) != 0 )
    // {
    //     sithCogExec_PushInt(pCog, -1);
    //     return;
    // }

    if ( !pThing || pThing->type != SITH_THING_PLAYER )
    {
        SITHLOG_ERROR("Cog %s: Bad thing in SelectWeaponWait().\n", pCog->aName);
        sithCogExec_PushInt(pCog, -1);
        return;
    }

    if ( !sithInventory_IsWeapon(typeID) )
    {
        SITHLOG_ERROR("Cog %s: Bad weapon index in SelectWeaponWait().\n", pCog->aName);
        sithCogExec_PushInt(pCog, -1);
        return;
    }

    // Return the current weapon ID
    sithCogExec_PushInt(pCog, pThing->thingInfo.actorInfo.weaponInfo.curWeaponID);

    if ( !sithWeapon_SelectWeapon(pThing, (SithWeaponId)typeID) )
    {
        pCog->status = SITHCOG_STATUS_WAITING_ACTOR_WEAPON_SELECT_FINISH;
        pCog->statusParams[0] = pThing->idx;
        pCog->statusParams[1] = typeID;
    }
}

void J3DAPI sithCogFunction_DeselectWeapon(SithCog* pCog)
{
    SithThing* pThing = sithCogExec_PopThing(pCog);
    if ( !pThing )
    {
        SITHLOG_ERROR("Cog %s: Bad thing in DeselectWeapon.\n", pCog->aName);
        sithCogExec_PushInt(pCog, -1);
        return;
    }
    sithCogExec_PushInt(pCog, pThing->thingInfo.actorInfo.weaponInfo.curWeaponID);
    sithWeapon_DeselectWeapon(pThing);
}

void J3DAPI sithCogFunction_DeselectWeaponWait(SithCog* pCog)
{
    SithThing* pThing = sithCogExec_PopThing(pCog);

    // Removed the debug mode check
    // TODO: remove this commented scope
    // if ( (sithMain_g_sith_mode.debugModeFlags & SITHDEBUG_INEDITOR) != 0 )
    // {
    //     sithCogExec_PushInt(pCog, -1);
    //     return;
    // }

    if ( !pThing || pThing->type != SITH_THING_PLAYER )
    {
        SITHLOG_ERROR("Cog %s: Bad thing in DeselectWeaponWait().\n", pCog->aName);
        sithCogExec_PushInt(pCog, -1);
        return;
    }

    int weaponID = pThing->thingInfo.actorInfo.weaponInfo.curWeaponID;
    if ( weaponID == SITHWEAPON_GRENADE )
    {
        sithCog_SendMessage(sithInventory_g_aTypes[SITHWEAPON_GRENADE].pCog, SITHCOG_MSG_BLOCKED, SITHCOG_SYM_REF_NONE, 0, SITHCOG_SYM_REF_THING, pThing->idx, 0);
    }
    else if ( weaponID == SITHWEAPON_NO_WEAPON )
    {
        if ( !sithWeapon_IsMountingWeapon(pThing) )
        {
            // No weapon to deselect
            sithCogExec_PushInt(pCog, 0);
            return;
        }

        weaponID = sithWeapon_GetLastWeapon();
    }

    // Make the cog wait until the weapon is deselected
    pCog->status          = SITHCOG_STATUS_WAITING_ACTOR_WEAPON_DESELECT_FINISH;
    pCog->statusParams[0] = pThing->idx;
    pCog->statusParams[1] = weaponID;

    sithCogExec_PushInt(pCog, weaponID); // Return the weapon ID of the weapon being deselected
    sithWeapon_DeselectWeapon(pThing);   // Deslect the weapon
}

void J3DAPI sithCogFunction_SetCurWeapon(SithCog* pCog)
{
    int weaponID      = sithCogExec_PopInt(pCog);
    SithThing* pThing = sithCogExec_PopThing(pCog);

    if ( !pThing || pThing->type != SITH_THING_PLAYER )
    {
        SITHLOG_ERROR("Cog %s: Invalid thing called from SetCurWeapon.\n", pCog->aName);
    }
    else
    {
        sithInventory_SetCurrentWeapon(pThing, (SithWeaponId)weaponID);
    }
}

void J3DAPI sithCogFunction_GetCurWeapon(SithCog* pCog)
{
    SithThing* pThing = sithCogExec_PopThing(pCog);
    if ( !pThing || pThing->type != SITH_THING_PLAYER )
    {
        SITHLOG_ERROR("Cog %s: Invalid thing called from GetCurWeapon.\n", pCog->aName);
        sithCogExec_PushInt(pCog, -1);
        return;
    }

    int weaponID = sithInventory_GetCurrentWeapon(pThing);
    sithCogExec_PushInt(pCog, weaponID);
}

void J3DAPI sithCogFunction_GetCameraStateFlags(SithCog* pCog)
{
    int flags = sithCamera_GetCameraStateFlags();
    sithCogExec_PushInt(pCog, flags);
}

void J3DAPI sithCogFunction_SetCameraStateFlags(SithCog* pCog)
{
    int flags = sithCogExec_PopInt(pCog);
    sithCamera_SetCameraStateFlags((SithCameraState)flags);
}

void J3DAPI sithCogFunction_IsMulti(SithCog* pCog)
{
    if ( stdComm_IsGameActive() )
    {
        sithCogExec_PushInt(pCog, 1);
    }
    else
    {
        sithCogExec_PushInt(pCog, 0);
    }
}

void J3DAPI sithCogFunction_IsServer(SithCog* pCog)
{
    if ( stdComm_IsGameHost() )
    {
        sithCogExec_PushInt(pCog, 1);
    }
    else
    {
        sithCogExec_PushInt(pCog, 0);
    }
}

void J3DAPI sithCogFunction_AutoSavegame(SithCog* pCog)
{
    J3D_UNUSED(pCog);
    char aSavePath[128];
    const char* pFilename = sithGetCurrentWorldSaveName();
    const char* pPrefix   = sithGetAutoSaveFilePrefix();
    STD_FORMAT(aSavePath, "%s%s", pPrefix, pFilename);

    // Fixed: Use bounded extension replacement for the fixed-size path buffer.
    stdFnames_ChangeExtEx(aSavePath, STD_ARRAYLEN(aSavePath), "nds");
    sithGamesave_Save(aSavePath, /*bOverride=*/1);
}

void J3DAPI sithCogFunction_GetHintSolved(SithCog* pCog)
{
    SithThing* pThing= sithCogExec_PopThing(pCog);
    if ( !pThing || pThing->type != SITH_THING_HINT )
    {
        sithCogExec_PushInt(pCog, -1);
        SITHLOG_ERROR("Cog %s: Bad call to GetHintSolved.\n", pCog->aName);
        return;
    }

    if ( ((SithHintFlags)pThing->userval & SITH_HINT_SOLVED) != 0 )
    {
        sithCogExec_PushInt(pCog, 1);
    }
    else
    {
        sithCogExec_PushInt(pCog, 0);
    }
}

void J3DAPI sithCogFunction_SetHintSolved(SithCog* pCog)
{
    SithThing* pThing = sithCogExec_PopThing(pCog);
    if ( !pThing )
    {
        SITHLOG_ERROR("Cog %s: Bad thingRef in SetHintSolved().\n", pCog->aName);
        return;
    }

    if ( pThing->type != SITH_THING_HINT )
    {
        SITHLOG_ERROR("Cog %s: Thing %s is not a hint in SetHintSolved().\n", pCog->aName, pThing->aName);
        return;
    }

    sithOverlayMap_SetHintSolved(pThing);
}

void J3DAPI sithCogFunction_SetHintUnsolved(SithCog* pCog)
{
    SithThing* pThing = sithCogExec_PopThing(pCog);
    if ( !pThing )
    {
        SITHLOG_ERROR("Cog %s: Bad thingRef in SetHintUnsolved().\n", pCog->aName);
        return;
    }

    if ( pThing->type != SITH_THING_HINT )
    {
        SITHLOG_ERROR("Cog %s: Thing %s is not a hint in SetHintUnsolved().\n", pCog->aName, pThing->aName);
        return;
    }

    sithOverlayMap_SetHintUnsolved(pThing);
}

void J3DAPI sithCogFunction_Rand(SithCog* pCog)
{
    sithCogExec_PushFlex(pCog, SITH_RANDF());
}

void J3DAPI sithCogFunction_RandBetween(SithCog* pCog)
{
    int max = sithCogExec_PopInt(pCog);
    int min = sithCogExec_PopInt(pCog);
    if ( min > max )
    {
        SITHLOG_ERROR("Cog %s: Illegal params in RandBetween.\n", pCog->aName);
        return;
    }

    sithCogExec_PushInt(pCog, min + rand() % (max - min + 1));
}

void J3DAPI sithCogFunction_RandVec(SithCog* pCog)
{
    rdVector3 vec;
    vec.x = SITH_RANDF();
    vec.y = SITH_RANDF();
    vec.z = SITH_RANDF();
    sithCogExec_PushVector(pCog, &vec);
}

void J3DAPI sithCogFunction_Round(SithCog* pCog)
{
    float val = sithCogExec_PopFlex(pCog);
    float rval = floorf(val);
    if ( val - rval >= 0.5f )
    {
        sithCogExec_PushFlex(pCog, ceilf(val));
    }
    else
    {
        sithCogExec_PushFlex(pCog, rval);
    }
}

void J3DAPI sithCogFunction_Truncate(SithCog* pCog)
{
    float val = sithCogExec_PopFlex(pCog);
    val = val < 0.0f ? ceilf(val) : floorf(val);
    sithCogExec_PushFlex(pCog, val);
}

void J3DAPI sithCogFunction_Abs(SithCog* pCog)
{
    float val = sithCogExec_PopFlex(pCog);
    if ( val < 0.0f )
    {
        val = -val;
    }

    sithCogExec_PushFlex(pCog, val);
}

void J3DAPI sithCogFunction_Pow(SithCog* pCog)
{
    float exp  = sithCogExec_PopFlex(pCog);
    float base = sithCogExec_PopFlex(pCog);

    if ( exp == 0.0f && base == 0.0f )
    {
        SITHLOG_ERROR("Cog %s: 0 to the 0 power is undefined\n", pCog->aName);
        sithCogExec_PushFlex(pCog, 0.0f);
        return;
    }

    float val = powf(base, exp);
    sithCogExec_PushFlex(pCog, val);
}

void J3DAPI sithCogFunction_Sin(SithCog* pCog)
{
    float angle = sithCogExec_PopFlex(pCog);

    float sin, cos;
    stdMath_SinCos(angle, &sin, &cos);
    sithCogExec_PushFlex(pCog, sin);
}

void J3DAPI sithCogFunction_Cos(SithCog* pCog)
{
    float angle = sithCogExec_PopFlex(pCog);

    float sin, cos;
    stdMath_SinCos(angle, &sin, &cos);
    sithCogExec_PushFlex(pCog, cos);
}

void J3DAPI sithCogFunction_ArcTan(SithCog* pCog)
{
    float y = sithCogExec_PopFlex(pCog);
    float x = sithCogExec_PopFlex(pCog);

    float atan = stdMath_ArcTan4(x, y);
    sithCogExec_PushFlex(pCog, atan);
}

void J3DAPI sithCogFunction_SetWeaponModel(SithCog* pCog)
{
    int typeId = sithCogExec_PopInt(pCog);
    SithThing* pThing = sithCogExec_PopThing(pCog);
    SITH_ASSERTREL(pThing);
    SITH_ASSERTREL((pThing->type == SITH_THING_PLAYER) || (pThing->type == SITH_THING_ACTOR));

    // Removed the debug mode check
    // TODO: Remove this commented scope
    //if ( (sithMain_g_sith_mode.debugModeFlags & SITHDEBUG_INEDITOR) == 0 || pThing->type == SITH_THING_PLAYER )
    //{
    //    if ( !sithInventory_g_aTypes[typeId].pModel )
    //    {
    //        SITHLOG_ERROR("Cog %s: No model available in SetWeaponModel().\n", pCog->aName);
    //        return;
    //    }
    //
    //    sithWeapon_SetWeaponModel(pThing, (SithWeaponId)typeId);
    //    if ( (pCog->flags & SITHCOG_NOSYNC) == 0 && pCog->execMsgType != SITHCOG_MSG_STARTUP && pCog->execMsgType != SITHCOG_MSG_SHUTDOWN )
    //    {
    //        sithThing_SyncThing(pThing, SITHTHING_SYNC_STATE);
    //    }
    //}

    if ( !sithInventory_g_aTypes[typeId].pModel )
    {
        SITHLOG_ERROR("Cog %s: No model available in SetWeaponModel().\n", pCog->aName);
        return;
    }

    sithWeapon_SetWeaponModel(pThing, (SithWeaponId)typeId);

    if ( (pCog->flags & SITHCOG_NOSYNC) == 0 && pCog->execMsgType != SITHCOG_MSG_STARTUP && pCog->execMsgType != SITHCOG_MSG_SHUTDOWN )
    {
        sithThing_SyncThing(pThing, SITHTHING_SYNC_STATE);
    }
}

void J3DAPI sithCogFunction_ResetWeaponModel(SithCog* pCog)
{
    SithThing* pThing = sithCogExec_PopThing(pCog);
    SITH_ASSERTREL(pThing);
    SITH_ASSERTREL((pThing->type == SITH_THING_PLAYER) || (pThing->type == SITH_THING_ACTOR));

    sithWeapon_ResetWeaponModel(pThing);

    if ( (pCog->flags & SITHCOG_NOSYNC) == 0 && pCog->execMsgType != SITHCOG_MSG_STARTUP && pCog->execMsgType != SITHCOG_MSG_SHUTDOWN )
    {
        sithThing_SyncThing(pThing, SITHTHING_SYNC_STATE);
    }
}

void J3DAPI sithCogFunction_LoadHolsterModel(SithCog* pCog)
{
    const char* pModelFilename = sithCogExec_PopString(pCog);
    SithInventoryType* pType = &sithInventory_g_aTypes[sithCogExec_PopInt(pCog)];

    rdModel3* pModel = NULL; // Fixed: Init. to NULL
    if ( pModelFilename )
    {
        pModel = sithModel_GetModel(pModelFilename);
    }

    if ( !pModel || !pType )
    {
        SITHLOG_ERROR("Cog %s: No model loaded in LoadHolsterModel().\n", pCog->aName);
        sithCogExec_PushInt(pCog, -1);
        return;
    }

    pType->pHolsterModel = pModel;
    sithCogExec_PushInt(pCog, pModel->num);
}

void J3DAPI sithCogFunction_SetHolsterModel(SithCog* pCog)
{
    int meshNum       = sithCogExec_PopInt(pCog);
    size_t weaponId   = sithCogExec_PopInt(pCog);
    SithThing* pThing = sithCogExec_PopThing(pCog);

    if ( !pThing )
    {
        SITHLOG_ERROR("Cog %s: Bad thing in SetHolsterModel().\n", pCog->aName);
        return;
    }

    if ( !weaponId || weaponId > SITHWEAPON_COMSHOTGUN )
    {
        SITHLOG_ERROR("Cog %s: Bad weaponID SetHolsterModel().\n", pCog->aName);
        return;
    }

    if ( !sithInventory_g_aTypes[weaponId].pHolsterModel )
    {
        SITHLOG_ERROR("Cog %s: No model available in SetHolsterModel().\n", pCog->aName);
        return;
    }

    sithWeapon_SetHolsterModel(pThing, (SithWeaponId)weaponId, meshNum);
    if ( (pCog->flags & SITHCOG_NOSYNC) == 0 && pCog->execMsgType != SITHCOG_MSG_STARTUP && pCog->execMsgType != SITHCOG_MSG_SHUTDOWN )
    {
        sithThing_SyncThing(pThing, SITHTHING_SYNC_STATE);
    }
}

void J3DAPI sithCogFunction_ResetHolsterModel(SithCog* pCog)
{
    size_t holsterNum = sithCogExec_PopInt(pCog);
    SithThing* pThing = sithCogExec_PopThing(pCog);

    if ( !pThing )
    {
        SITHLOG_ERROR("Cog %s: Bad thing in ResetHolsterModel().\n", pCog->aName);
        return;
    }

    if ( holsterNum == 0 || holsterNum > SITHWEAPON_HOLSTERBACK )
    {
        SITHLOG_ERROR("Cog %s: Bad holster number in ResetHolsterModel().\n", pCog->aName);
        return;
    }

    sithWeapon_ResetHolsterModel(pThing, holsterNum);

    if ( (pCog->flags & SITHCOG_NOSYNC) == 0 && pCog->execMsgType != SITHCOG_MSG_STARTUP && pCog->execMsgType != SITHCOG_MSG_SHUTDOWN )
    {
        sithThing_SyncThing(pThing, SITHTHING_SYNC_STATE);
    }
}

void J3DAPI sithCogFunction_GetLastPistol(SithCog* pCog)
{
    SithThing* pThing = sithCogExec_PopThing(pCog);
    if ( !pThing || pThing->type != SITH_THING_PLAYER )
    {
        SITHLOG_ERROR("Cog %s: Bad thing in GetLastPistol().\n", pCog->aName);
    }
    else
    {
        sithCogExec_PushInt(pCog, pThing->thingInfo.actorInfo.pPlayer->lastPistolID);
    }
}

void J3DAPI sithCogFunction_GetLastRifle(SithCog* pCog)
{
    SithThing* pThing = sithCogExec_PopThing(pCog);
    if ( !pThing || pThing->type != SITH_THING_PLAYER )
    {
        SITHLOG_ERROR("Cog %s: Bad thing in GetLastRifle().\n", pCog->aName);
    }
    else
    {
        sithCogExec_PushInt(pCog, pThing->thingInfo.actorInfo.pPlayer->lastRifleID);
    }
}

void J3DAPI sithCogFunction_InEditor(SithCog* pCog)
{
    if ( (sithMain_g_sith_mode.debugModeFlags & SITHDEBUG_INEDITOR) != 0 )
    {
        sithCogExec_PushInt(pCog, 1);
    }
    else
    {
        sithCogExec_PushInt(pCog, 0);
    }
}

void J3DAPI sithCogFunction_CopyPlayerHolsters(SithCog* pCog)
{
    SithThing* pDestThing = sithCogExec_PopThing(pCog);
    SithThing* pSourceThing = sithCogExec_PopThing(pCog);

    if ( !pSourceThing || pSourceThing->type != SITH_THING_PLAYER || !pDestThing )
    {
        SITHLOG_ERROR("Cog %s: Bad thing in CopyPlayerHolsters().\n", pCog->aName);
        return;
    }

    SITH_ASSERTREL(pSourceThing->renderData.data.pModel3 != NULL);
    SITH_ASSERTREL(pDestThing->renderData.data.pModel3 != NULL);

    sithThing_ResetSwapList(pDestThing);

    SithThingSwapEntry* pSwapList = pSourceThing->pSwapList;
    while ( pSwapList )
    {
        int meshNum = sithThing_GetThingMeshIndex(pDestThing, pSourceThing->renderData.data.pModel3->aGeos[0].aMeshes[pSwapList->meshNum].name);
        if ( meshNum != -1 )
        {
            sithThing_AddSwapEntry(pDestThing, meshNum, pSwapList->pSrcModel, pSwapList->srcMeshNum);
        }
        pSwapList = pSwapList->pNextEntry; // Fixed: Infinite loop when meshNum == -1 by moving outside the if statement's scope
    }
}

void J3DAPI sithCogFunction_GetPerformanceLevel(SithCog* pCog)
{
    sithCogExec_PushInt(pCog, sithGetPerformanceLevel());
}

void J3DAPI sithCogFunction_IsLevelName(SithCog* pCog)
{
    const char* pName = sithCogExec_PopString(pCog);
    if ( !pName )
    {
        SITHLOG_ERROR("Cog %s: Bad string in IsLevelName().", pCog->aName);
        sithCogExec_PushInt(pCog, 0);
        return;
    }

    if ( streqi(sithWorld_g_pCurrentWorld->aName, pName) )
    {
        sithCogExec_PushInt(pCog, 1);
    }
    else
    {
        sithCogExec_PushInt(pCog, 0);
    }
}

void J3DAPI sithCogFunction_SetFog(SithCog* pCog)
{
    rdVector3 vecColor;
    float end   = sithCogExec_PopFlex(pCog);
    float start = sithCogExec_PopFlex(pCog);
    int bVec    = sithCogExec_PopVector(pCog, &vecColor);
    int bEnable = sithCogExec_PopInt(pCog);

    if ( !bVec )
    {
        SITHLOG_ERROR("Cog %s: Bad call to SetFog().\n", pCog->aName);
        return;
    }

    sithWorld_g_pCurrentWorld->fog.bEnabled    = bEnable;
    sithWorld_g_pCurrentWorld->fog.color.red   = vecColor.red;
    sithWorld_g_pCurrentWorld->fog.color.green = vecColor.green;
    sithWorld_g_pCurrentWorld->fog.color.blue  = vecColor.blue;
    sithWorld_g_pCurrentWorld->fog.startDepth  = start;
    sithWorld_g_pCurrentWorld->fog.endDepth    = end;
    sithWorld_g_pCurrentWorld->state |= SITH_WORLD_STATE_UPDATE_FOG;
}

void J3DAPI sithCogFunction_FindNewSector(SithCog* pCog)
{
    rdVector3 endPos;
    sithCogExec_PopVector(pCog, &endPos);

    SithSector* pSector = sithCogExec_PopSector(pCog);

    rdVector3 startPos;
    sithCogExec_PopVector(pCog, &startPos);

    SithSector* pFoundSector = sithCollision_FindSectorInRadius(pSector, &startPos, &endPos, 0.0f);
    if ( pFoundSector )
    {
        sithCogExec_PushInt(pCog, sithSector_GetSectorIndex(pFoundSector));
    }
    else
    {
        sithCogExec_PushInt(pCog, -1);
    }
}

void J3DAPI sithCogFunction_FindNewSectorFromThing(SithCog* pCog)
{
    rdVector3 endPos;
    sithCogExec_PopVector(pCog, &endPos);
    SithThing* pThing = sithCogExec_PopThing(pCog);

    if ( !pThing || !pThing->pInSector )
    {
        SITHLOG_ERROR("BAD THING REF IN FindNewSectorFromThing\n");
        return;
    }

    SithSector* pFoundSector = sithCollision_FindSectorInRadius(pThing->pInSector, &pThing->pos, &endPos, 0.0f);
    if ( pFoundSector )
    {
        sithCogExec_PushInt(pCog, sithSector_GetSectorIndex(pFoundSector));
        return;
    }

    sithCogExec_PushInt(pCog, -1);
}

void J3DAPI sithCogFunction_DebugLocalSymbols(SithCog* pCog)
{
    const char* pHeaderText = sithCogExec_PopString(pCog);

    STD_FORMAT(std_g_genBuffer, "%s.\n", pHeaderText);
    SITHLOG_ERROR(std_g_genBuffer);

    if ( pCog->pScript && pCog->pSymbolTable )
    {
        size_t symNum = 0;
        SithCogSymbol* pCurSym = pCog->pSymbolTable->aSymbols;

        while ( symNum < pCog->pSymbolTable->numUsedSymbols )
        {
            bool bPrint = symNum < pCog->pScript->numHandlers;
            if ( symNum >= pCog->pScript->numHandlers )
            {
                size_t symRefNum = 0;
                SithCogSymbolRef* pCurSymRef = pCog->pScript->aSymRefs;
                while ( symRefNum < pCog->pScript->numSymbolRefs )
                {
                    if ( pCurSymRef->symbolId == symNum )
                    {
                        if ( (pCurSymRef->bLocal & 1) == 0 )
                        {
                            bPrint = true;
                        }

                        break;
                    }

                    ++symRefNum;
                    ++pCurSymRef;
                }
            }

            if ( !bPrint )
            {
                switch ( pCurSym->value.type )
                {
                    case SITHCOG_VALUE_FLOAT:
                    {
                        const char* pValStr = pCurSym->pName ? pCurSym->pName : "<null>";
                        STD_FORMAT(std_g_genBuffer, "  Symbol %d: '%s' ", pCurSym->id, pValStr);

                        size_t endOffs = strlen(std_g_genBuffer);
                        stdUtil_Format(&std_g_genBuffer[endOffs], STD_ARRAYLEN(std_g_genBuffer) - strlen(std_g_genBuffer), " = %f\n", pCurSym->value.val.floatValue);
                        SITHLOG_DEBUG(std_g_genBuffer); // Changed: Changed to debug from error
                    } break;
                    case SITHCOG_VALUE_VECTOR:
                    {
                        const char* pValStr = pCurSym->pName ? pCurSym->pName : "<null>";
                        STD_FORMAT(std_g_genBuffer, "  Symbol %d: '%s' ", pCurSym->id, pValStr);

                        size_t endOffs = strlen(std_g_genBuffer);
                        stdUtil_Format(&std_g_genBuffer[endOffs], STD_ARRAYLEN(std_g_genBuffer) - strlen(std_g_genBuffer), " = %f %f %f\n", pCurSym->value.val.floatValue, pCurSym->value.val.vecValue.y, pCurSym->value.val.vecValue.z);
                        SITHLOG_DEBUG(std_g_genBuffer); // Changed: Changed to debug from error
                    } break;
                    case SITHCOG_VALUE_INT:
                    {
                        const char* pValStr = pCurSym->pName ? pCurSym->pName : "<null>";
                        STD_FORMAT(std_g_genBuffer, "  Symbol %d: '%s' ", pCurSym->id, pValStr);

                        size_t endOffs = strlen(std_g_genBuffer);
                        stdUtil_Format(&std_g_genBuffer[endOffs], STD_ARRAYLEN(std_g_genBuffer) - strlen(std_g_genBuffer), " = %d\n", pCurSym->value.val.intValue);
                        SITHLOG_DEBUG(std_g_genBuffer); // Changed: Changed to debug from error
                    } break;
                }
            }

            ++symNum;
            ++pCurSym;
        }
    }

    sithCogExec_PushInt(pCog, 1);
}

void J3DAPI sithCogFunction_DebugPrint(SithCog* pCog)
{
    const char* pStr = sithCogExec_PopString(pCog);
    if ( pStr )
    {
        SITHLOG_DEBUG("%s\n", pStr);
    }
}

void J3DAPI sithCogFunction_DebugFlex(SithCog* pCog)
{
    const float val  = sithCogExec_PopFlex(pCog);
    const char* pStr = sithCogExec_PopString(pCog);
    if ( pStr )
    {
        SITHLOG_DEBUG("%s (%f)\n", pStr, val);
    }

    sithCogExec_PushFlex(pCog, val);
}

void J3DAPI sithCogFunction_DebugInt(SithCog* pCog)
{
    const int val    = sithCogExec_PopInt(pCog);
    const char* pStr = sithCogExec_PopString(pCog);
    if ( pStr )
    {
        SITHLOG_DEBUG("%s (%d)\n", pStr, val);
    }

    sithCogExec_PushInt(pCog, val);
}

void J3DAPI sithCogFunction_DebugVector(SithCog* pCog)
{
    rdVector3 vec = { 0 };
    char aVecStr[64] = { 0 }; // Added: Init to 0
    if ( !sithCogExec_PopVector(pCog, &vec) )
    {
        STD_FORMAT(aVecStr, "Bad vector");
    }
    else
    {
        STD_FORMAT(aVecStr, "<%f %f %f>", vec.x, vec.y, vec.z);
    }

    const char* pStr = sithCogExec_PopString(pCog);
    if ( pStr )
    {
        SITHLOG_DEBUG("%s %s\n", pStr, aVecStr);
    }
    sithCogExec_PushVector(pCog, &vec);
}

void J3DAPI sithCogFunction_DebugWaitForKey(SithCog* pCog)
{
    J3D_UNUSED(pCog);

    // Added: This scope was not present in the original debug version
    if ( (sithMain_g_sith_mode.debugModeFlags & SITHDEBUG_INEDITOR) == 0 )
    {
        return;
    }

    SITHLOG_ERROR("Waiting for keypress: ");

    int bPressed = 0;
    while ( !bPressed )
    {
        sithControl_ReadControls();
        bPressed = sithControl_GetKey(SITHCONTROL_DEBUG, NULL);
        sithControl_FinishRead();
    }

    SITHLOG_ERROR("\n");
}

int J3DAPI sithCogFunction_RegisterFunctions(SithCogSymbolTable* pTable)
{
    return sithCog_RegisterFunction(pTable, sithCogFunction_GetSenderRef, "getsenderref")
        || sithCog_RegisterFunction(pTable, sithCogFunction_GetSenderType, "getsendertype")
        || sithCog_RegisterFunction(pTable, sithCogFunction_GetSenderID, "getsenderid")
        || sithCog_RegisterFunction(pTable, sithCogFunction_GetSourceType, "getsourcetype")
        || sithCog_RegisterFunction(pTable, sithCogFunction_GetSourceRef, "getsourceref")
        || sithCog_RegisterFunction(pTable, sithCogFunction_GetSysDate, "getsysdate")
        || sithCog_RegisterFunction(pTable, sithCogFunction_GetSysTime, "getsystime")
        || sithCog_RegisterFunction(pTable, sithCogFunction_InEditor, "ineditor")
        || sithCog_RegisterFunction(pTable, sithCogFunction_GetThingCount, "getthingcount")
        || sithCog_RegisterFunction(pTable, sithCogFunction_GetGravity, "getgravity")
        || sithCog_RegisterFunction(pTable, sithCogFunction_SetGravity, "setgravity")
        || sithCog_RegisterFunction(pTable, sithCogFunction_GetLevelTime, "getleveltime")
        || sithCog_RegisterFunction(pTable, sithCogFunction_GetGameTime, "getgametime")
        || sithCog_RegisterFunction(pTable, sithCogFunction_GetFlexGameTime, "getflexgametime")
        || sithCog_RegisterFunction(pTable, sithCogFunction_GetThingTemplateCount, "getthingtemplatecount")
        || sithCog_RegisterFunction(pTable, sithCogFunction_SetFog, "setfog")
        || sithCog_RegisterFunction(pTable, sithCogFunction_FindNewSector, "findnewsector")
        || sithCog_RegisterFunction(pTable, sithCogFunction_FindNewSectorFromThing, "findnewsectorfromthing")
        || sithCog_RegisterFunction(pTable, sithCogFunction_Sleep, "sleep")
        || sithCog_RegisterFunction(pTable, sithCogFunction_SetPulse, "setpulse")
        || sithCog_RegisterFunction(pTable, sithCogFunction_SetTimer, "settimer")
        || sithCog_RegisterFunction(pTable, sithCogFunction_SetTimerEx, "settimerex")
        || sithCog_RegisterFunction(pTable, sithCogFunction_KillTimerEx, "killtimerex")
        || sithCog_RegisterFunction(pTable, sithCogFunction_Reset, "reset")
        || sithCog_RegisterFunction(pTable, sithCogFunction_MaterialAnim, "materialanim")
        || sithCog_RegisterFunction(pTable, sithCogFunction_StopMaterialAnim, "stopmaterialanim")
        || sithCog_RegisterFunction(pTable, sithCogFunction_StopAnim, "stopanim")
        || sithCog_RegisterFunction(pTable, sithCogFunction_StopSurfaceAnim, "stopsurfaceanim")
        || sithCog_RegisterFunction(pTable, sithCogFunction_GetSurfaceAnim, "getsurfaceanim")
        || sithCog_RegisterFunction(pTable, sithCogFunction_SurfaceAnim, "surfaceanim")
        || sithCog_RegisterFunction(pTable, sithCogFunction_GetKeyLen, "getkeylen")
        || sithCog_RegisterFunction(pTable, sithCogFunction_LoadTemplate, "loadtemplate")
        || sithCog_RegisterFunction(pTable, sithCogFunction_LoadKeyframe, "loadkeyframe")
        || sithCog_RegisterFunction(pTable, sithCogFunction_LoadModel, "loadmodel")
        || sithCog_RegisterFunction(pTable, EnablePrint, "enableprint")
        || sithCog_RegisterFunction(pTable, IsPrintEnabled, "isprintenabled")
        || sithCog_RegisterFunction(pTable, sithCogFunction_Print, "print")
        || sithCog_RegisterFunction(pTable, sithCogFunction_PrintInt, "printint")
        || sithCog_RegisterFunction(pTable, sithCogFunction_PrintFlex, "printflex")
        || sithCog_RegisterFunction(pTable, sithCogFunction_PrintVector, "printvector")
        || sithCog_RegisterFunction(pTable, sithCogFunction_PrintHex, "printhex")
        || sithCog_RegisterFunction(pTable, sithCogFunction_VectorAdd, "vectoradd")
        || sithCog_RegisterFunction(pTable, sithCogFunction_VectorSub, "vectorsub")
        || sithCog_RegisterFunction(pTable, sithCogFunction_VectorDot, "vectordot")
        || sithCog_RegisterFunction(pTable, sithCogFunction_VectorCross, "vectorcross")
        || sithCog_RegisterFunction(pTable, sithCogFunction_VectorSet, "vectorset")
        || sithCog_RegisterFunction(pTable, sithCogFunction_VectorLen, "vectorlen")
        || sithCog_RegisterFunction(pTable, sithCogFunction_VectorScale, "vectorscale")
        || sithCog_RegisterFunction(pTable, sithCogFunction_VectorDist, "vectordist")
        || sithCog_RegisterFunction(pTable, sithCogFunction_VectorX, "vectorx")
        || sithCog_RegisterFunction(pTable, sithCogFunction_VectorY, "vectory")
        || sithCog_RegisterFunction(pTable, sithCogFunction_VectorZ, "vectorz")
        || sithCog_RegisterFunction(pTable, sithCogFunction_VectorNorm, "vectornorm")
        || sithCog_RegisterFunction(pTable, sithCogFunction_VectorEqual, "vectorequal")
        || sithCog_RegisterFunction(pTable, sithCogFunction_VectorRotate, "vectorrotate")
        || sithCog_RegisterFunction(pTable, sithCogFunction_VectorTransformToOrient, "vectortransformtoorient")
        || sithCog_RegisterFunction(pTable, sithCogFunction_GetSithMode, "getsithmode")
        || sithCog_RegisterFunction(pTable, sithCogFunction_GetDifficulty, "getdifficulty")
        || sithCog_RegisterFunction(pTable, sithCogFunction_SetSubModeFlags, "setsubmodeflags")
        || sithCog_RegisterFunction(pTable, sithCogFunction_GetSubModeFlags, "getsubmodeflags")
        || sithCog_RegisterFunction(pTable, sithCogFunction_ClearSubModeFlags, "clearsubmodeflags")
        || sithCog_RegisterFunction(pTable, sithCogFunction_SetDebugModeFlags, "setdebugmodeflags")
        || sithCog_RegisterFunction(pTable, sithCogFunction_GetDebugModeFlags, "getdebugmodeflags")
        || sithCog_RegisterFunction(pTable, sithCogFunction_ClearDebugModeFlags, "cleardebugmodeflags")
        || sithCog_RegisterFunction(pTable, sithCogFunction_BitSet, "bitset")
        || sithCog_RegisterFunction(pTable, sithCogFunction_BitTest, "bittest")
        || sithCog_RegisterFunction(pTable, sithCogFunction_BitClear, "bitclear")
        || sithCog_RegisterFunction(pTable, sithCogFunction_FireProjectile, "fireprojectile")
        || sithCog_RegisterFunction(pTable, sithCogFunction_ActivateWeapon, "activateweapon")
        || sithCog_RegisterFunction(pTable, sithCogFunction_DeactivateWeapon, "deactivateweapon")
        || sithCog_RegisterFunction(pTable, sithCogFunction_DeactivateCurrentWeapon, "deactivatecurweapon")
        || sithCog_RegisterFunction(pTable, sithCogFunction_SetMountWait, "setmountwait")
        || sithCog_RegisterFunction(pTable, sithCogFunction_SetFireWait, "setfirewait")
        || sithCog_RegisterFunction(pTable, sithCogFunction_SetAimWait, "setaimwait")
        || sithCog_RegisterFunction(pTable, sithCogFunction_SelectWeapon, "selectweapon")
        || sithCog_RegisterFunction(pTable, sithCogFunction_SelectWeaponWait, "selectweaponwait")
        || sithCog_RegisterFunction(pTable, sithCogFunction_DeselectWeapon, "deselectweapon")
        || sithCog_RegisterFunction(pTable, sithCogFunction_DeselectWeaponWait, "deselectweaponwait")
        || sithCog_RegisterFunction(pTable, sithCogFunction_SetCurWeapon, "setcurweapon")
        || sithCog_RegisterFunction(pTable, sithCogFunction_GetCurWeapon, "getcurweapon")
        || sithCog_RegisterFunction(pTable, sithCogFunction_SetWeaponModel, "setweaponmodel")
        || sithCog_RegisterFunction(pTable, sithCogFunction_ResetWeaponModel, "resetweaponmodel")
        || sithCog_RegisterFunction(pTable, sithCogFunction_LoadHolsterModel, "loadholstermodel")
        || sithCog_RegisterFunction(pTable, sithCogFunction_SetHolsterModel, "setholstermodel")
        || sithCog_RegisterFunction(pTable, sithCogFunction_ResetHolsterModel, "resetholstermodel")
        || sithCog_RegisterFunction(pTable, sithCogFunction_GetLastPistol, "getlastpistol")
        || sithCog_RegisterFunction(pTable, sithCogFunction_GetLastRifle, "getlastrifle")
        || sithCog_RegisterFunction(pTable, sithCogFunction_CopyPlayerHolsters, "copyplayerholsters")
        || sithCog_RegisterFunction(pTable, sithCogFunction_SendMessage, "sendmessage")
        || sithCog_RegisterFunction(pTable, sithCogFunction_SendMessageEx, "sendmessageex")
        || sithCog_RegisterFunction(pTable, sithCogFunction_ReturnEx, "returnex")
        || sithCog_RegisterFunction(pTable, sithCogFunction_GetParam, "getparam")
        || sithCog_RegisterFunction(pTable, sithCogFunction_SetParam, "setparam")
        || sithCog_RegisterFunction(pTable, sithCogFunction_SetInvFlags, "setinvflags")
        || sithCog_RegisterFunction(pTable, sithCogFunction_SetMapModeFlags, "setmapmodeflags")
        || sithCog_RegisterFunction(pTable, sithCogFunction_GetMapModelFlags, "getmapmodeflags")
        || sithCog_RegisterFunction(pTable, sithCogFunction_ClearMapModeFlags, "clearmapmodeflags")
        || sithCog_RegisterFunction(pTable, sithCogFunction_GetMaterialCel, "getmaterialcel")
        || sithCog_RegisterFunction(pTable, sithCogFunction_SetMaterialCel, "setmaterialcel")
        || sithCog_RegisterFunction(pTable, sithCogFunction_SetCameraFocus, "setcamerafocus")
        || sithCog_RegisterFunction(pTable, sithCogFunction_GetPrimaryFocus, "getprimaryfocus")
        || sithCog_RegisterFunction(pTable, sithCogFunction_GetSecondaryFocus, "getsecondaryfocus")
        || sithCog_RegisterFunction(pTable, sithCogFunction_SetCameraMode, "setcurrentcamera")
        || sithCog_RegisterFunction(pTable, sithCogFunction_GetCameraMode, "getcurrentcamera")
        || sithCog_RegisterFunction(pTable, sithCogFunction_CycleCamera, "cyclecamera")
        || sithCog_RegisterFunction(pTable, sithCogFunction_SetPOVShake, "setpovshake")
        || sithCog_RegisterFunction(pTable, sithCogFunction_SetCameraStateFlags, "setcamerastateflags")
        || sithCog_RegisterFunction(pTable, sithCogFunction_GetCameraStateFlags, "getcamerastateflags")
        || sithCog_RegisterFunction(pTable, sithCogFunction_SetCameraSecondaryFocus, "setcamerasecondaryfocus")
        || sithCog_RegisterFunction(pTable, sithCogFunction_HeapNew, "heapnew")
        || sithCog_RegisterFunction(pTable, sithCogFunction_HeapSet, "heapset")
        || sithCog_RegisterFunction(pTable, sithCogFunction_HeapGet, "heapget")
        || sithCog_RegisterFunction(pTable, sithCogFunction_HeapFree, "heapfree")
        || sithCog_RegisterFunction(pTable, sithCogFunction_GetSelfCog, "getselfcog")
        || sithCog_RegisterFunction(pTable, sithCogFunction_GetMasterCog, "getmastercog")
        || sithCog_RegisterFunction(pTable, sithCogFunction_SetMasterCog, "setmastercog")
        || sithCog_RegisterFunction(pTable, sithCogFunction_GetCogByIndex, "getcogbyindex")
        || sithCog_RegisterFunction(pTable, sithCogFunction_IsMulti, "ismulti")
        || sithCog_RegisterFunction(pTable, sithCogFunction_IsServer, "isserver")
        || sithCog_RegisterFunction(pTable, sithCogFunction_SendTrigger, "sendtrigger")
        || sithCog_RegisterFunction(pTable, sithCogFunction_AutoSavegame, "autosavegame")
        || sithCog_RegisterFunction(pTable, sithCogFunction_GetHintSolved, "gethintsolved")
        || sithCog_RegisterFunction(pTable, sithCogFunction_SetHintSolved, "sethintsolved")
        || sithCog_RegisterFunction(pTable, sithCogFunction_SetHintUnsolved, "sethintunsolved")
        || sithCog_RegisterFunction(pTable, sithCogFunction_Rand, "rand")
        || sithCog_RegisterFunction(pTable, sithCogFunction_RandBetween, "randbetween")
        || sithCog_RegisterFunction(pTable, sithCogFunction_RandVec, "randvec")
        || sithCog_RegisterFunction(pTable, sithCogFunction_Round, "round")
        || sithCog_RegisterFunction(pTable, sithCogFunction_Truncate, "truncate")
        || sithCog_RegisterFunction(pTable, sithCogFunction_Abs, "abs")
        || sithCog_RegisterFunction(pTable, sithCogFunction_Pow, "pow")
        || sithCog_RegisterFunction(pTable, sithCogFunction_Sin, "sin")
        || sithCog_RegisterFunction(pTable, sithCogFunction_Cos, "cos")
        || sithCog_RegisterFunction(pTable, sithCogFunction_ArcTan, "arctan")
        || sithCog_RegisterFunction(pTable, sithCogFunction_GetPerformanceLevel, "getperformancelevel")
        || sithCog_RegisterFunction(pTable, sithCogFunction_IsLevelName, "islevelname")
        || sithCog_RegisterFunction(pTable, sithCogFunction_DebugPrint, "debugprint") // Added: From debug version
        || sithCog_RegisterFunction(pTable, sithCogFunction_DebugFlex, "debugflex")   // Added: From debug version
        || sithCog_RegisterFunction(pTable, sithCogFunction_DebugInt, "debugint")     // Added: From debug version
        || sithCog_RegisterFunction(pTable, sithCogFunction_DebugVector, "debugvector") // Added
        || sithCog_RegisterFunction(pTable, sithCogFunction_DebugLocalSymbols, "debuglocalsymbols") // Added: From debug version
        || sithCog_RegisterFunction(pTable, sithCogFunction_DebugWaitForKey, "debugwaitforkey"); // Added: From debug version
}

void sithCogFunction_InstallHooks(void)
{
    J3D_HOOKFUNC(sithCogFunction_RegisterFunctions);
    J3D_HOOKFUNC(sithCogFunction_GetSenderID);
    J3D_HOOKFUNC(sithCogFunction_GetSenderRef);
    J3D_HOOKFUNC(sithCogFunction_GetSenderType);
    J3D_HOOKFUNC(sithCogFunction_GetSourceRef);
    J3D_HOOKFUNC(sithCogFunction_GetSourceType);
    J3D_HOOKFUNC(sithCogFunction_GetSysDate);
    J3D_HOOKFUNC(sithCogFunction_GetSysTime);
    J3D_HOOKFUNC(sithCogFunction_Sleep);
    J3D_HOOKFUNC(sithCogFunction_Print);
    J3D_HOOKFUNC(sithCogFunction_PrintVector);
    J3D_HOOKFUNC(sithCogFunction_PrintFlex);
    J3D_HOOKFUNC(sithCogFunction_PrintInt);
    J3D_HOOKFUNC(sithCogFunction_SurfaceAnim);
    J3D_HOOKFUNC(sithCogFunction_MaterialAnim);
    J3D_HOOKFUNC(sithCogFunction_StopMaterialAnim);
    J3D_HOOKFUNC(sithCogFunction_StopAnim);
    J3D_HOOKFUNC(sithCogFunction_StopSurfaceAnim);
    J3D_HOOKFUNC(sithCogFunction_GetSurfaceAnim);
    J3D_HOOKFUNC(sithCogFunction_LoadTemplate);
    J3D_HOOKFUNC(sithCogFunction_LoadKeyframe);
    J3D_HOOKFUNC(sithCogFunction_LoadModel);
    J3D_HOOKFUNC(sithCogFunction_SetPulse);
    J3D_HOOKFUNC(sithCogFunction_SetTimer);
    J3D_HOOKFUNC(sithCogFunction_SetTimerEx);
    J3D_HOOKFUNC(sithCogFunction_KillTimerEx);
    J3D_HOOKFUNC(sithCogFunction_Reset);
    J3D_HOOKFUNC(sithCogFunction_VectorSet);
    J3D_HOOKFUNC(sithCogFunction_VectorAdd);
    J3D_HOOKFUNC(sithCogFunction_VectorSub);
    J3D_HOOKFUNC(sithCogFunction_VectorDot);
    J3D_HOOKFUNC(sithCogFunction_VectorCross);
    J3D_HOOKFUNC(sithCogFunction_VectorLen);
    J3D_HOOKFUNC(sithCogFunction_VectorScale);
    J3D_HOOKFUNC(sithCogFunction_VectorDist);
    J3D_HOOKFUNC(sithCogFunction_VectorEqual);
    J3D_HOOKFUNC(sithCogFunction_VectorRotate);
    J3D_HOOKFUNC(sithCogFunction_VectorTransformToOrient);
    J3D_HOOKFUNC(sithCogFunction_SendMessage);
    J3D_HOOKFUNC(sithCogFunction_SendMessageEx);
    J3D_HOOKFUNC(sithCogFunction_GetKeyLen);
    J3D_HOOKFUNC(sithCogFunction_GetSithMode);
    J3D_HOOKFUNC(sithCogFunction_GetGameTime);
    J3D_HOOKFUNC(sithCogFunction_GetFlexGameTime);
    J3D_HOOKFUNC(sithCogFunction_GetDifficulty);
    J3D_HOOKFUNC(sithCogFunction_SetSubModeFlags);
    J3D_HOOKFUNC(sithCogFunction_ClearSubModeFlags);
    J3D_HOOKFUNC(sithCogFunction_GetSubModeFlags);
    J3D_HOOKFUNC(sithCogFunction_SetDebugModeFlags);
    J3D_HOOKFUNC(sithCogFunction_ClearDebugModeFlags);
    J3D_HOOKFUNC(sithCogFunction_GetDebugModeFlags);
    J3D_HOOKFUNC(sithCogFunction_BitSet);
    J3D_HOOKFUNC(sithCogFunction_BitTest);
    J3D_HOOKFUNC(sithCogFunction_BitClear);
    J3D_HOOKFUNC(sithCogFunction_GetLevelTime);
    J3D_HOOKFUNC(sithCogFunction_GetThingCount);
    J3D_HOOKFUNC(sithCogFunction_GetThingTemplateCount);
    J3D_HOOKFUNC(sithCogFunction_GetGravity);
    J3D_HOOKFUNC(sithCogFunction_SetGravity);
    J3D_HOOKFUNC(sithCogFunction_ReturnEx);
    J3D_HOOKFUNC(sithCogFunction_GetParam);
    J3D_HOOKFUNC(sithCogFunction_SetParam);
    J3D_HOOKFUNC(sithCogFunction_VectorX);
    J3D_HOOKFUNC(sithCogFunction_VectorY);
    J3D_HOOKFUNC(sithCogFunction_VectorZ);
    J3D_HOOKFUNC(sithCogFunction_VectorNorm);
    J3D_HOOKFUNC(sithCogFunction_SetMaterialCel);
    J3D_HOOKFUNC(sithCogFunction_GetMaterialCel);
    J3D_HOOKFUNC(sithCogFunction_SetInvFlags);
    J3D_HOOKFUNC(sithCogFunction_SetMapModeFlags);
    J3D_HOOKFUNC(sithCogFunction_GetMapModelFlags);
    J3D_HOOKFUNC(sithCogFunction_ClearMapModeFlags);
    J3D_HOOKFUNC(sithCogFunction_SetCameraFocus);
    J3D_HOOKFUNC(sithCogFunction_SetCameraSecondaryFocus);
    J3D_HOOKFUNC(sithCogFunction_GetPrimaryFocus);
    J3D_HOOKFUNC(sithCogFunction_GetSecondaryFocus);
    J3D_HOOKFUNC(sithCogFunction_SetCameraMode);
    J3D_HOOKFUNC(sithCogFunction_GetCameraMode);
    J3D_HOOKFUNC(sithCogFunction_SetPOVShake);
    J3D_HOOKFUNC(sithCogFunction_HeapNew);
    J3D_HOOKFUNC(sithCogFunction_HeapSet);
    J3D_HOOKFUNC(sithCogFunction_HeapGet);
    J3D_HOOKFUNC(sithCogFunction_HeapFree);
    J3D_HOOKFUNC(sithCogFunction_GetSelfCog);
    J3D_HOOKFUNC(sithCogFunction_GetMasterCog);
    J3D_HOOKFUNC(sithCogFunction_SetMasterCog);
    J3D_HOOKFUNC(sithCogFunction_GetCogByIndex);
    J3D_HOOKFUNC(sithCogFunction_FireProjectile);
    J3D_HOOKFUNC(sithCogFunction_SendTrigger);
    J3D_HOOKFUNC(sithCogFunction_ActivateWeapon);
    J3D_HOOKFUNC(sithCogFunction_DeactivateWeapon);
    J3D_HOOKFUNC(sithCogFunction_DeactivateCurrentWeapon);
    J3D_HOOKFUNC(sithCogFunction_SetFireWait);
    J3D_HOOKFUNC(sithCogFunction_SetMountWait);
    J3D_HOOKFUNC(sithCogFunction_SetAimWait);
    J3D_HOOKFUNC(sithCogFunction_SelectWeapon);
    J3D_HOOKFUNC(sithCogFunction_SelectWeaponWait);
    J3D_HOOKFUNC(sithCogFunction_DeselectWeapon);
    J3D_HOOKFUNC(sithCogFunction_DeselectWeaponWait);
    J3D_HOOKFUNC(sithCogFunction_SetCurWeapon);
    J3D_HOOKFUNC(sithCogFunction_GetCurWeapon);
    J3D_HOOKFUNC(sithCogFunction_GetCameraStateFlags);
    J3D_HOOKFUNC(sithCogFunction_SetCameraStateFlags);
    J3D_HOOKFUNC(sithCogFunction_IsMulti);
    J3D_HOOKFUNC(sithCogFunction_IsServer);
    J3D_HOOKFUNC(sithCogFunction_AutoSavegame);
    J3D_HOOKFUNC(sithCogFunction_GetHintSolved);
    J3D_HOOKFUNC(sithCogFunction_SetHintSolved);
    J3D_HOOKFUNC(sithCogFunction_SetHintUnsolved);
    J3D_HOOKFUNC(sithCogFunction_Rand);
    J3D_HOOKFUNC(sithCogFunction_RandBetween);
    J3D_HOOKFUNC(sithCogFunction_RandVec);
    J3D_HOOKFUNC(sithCogFunction_Round);
    J3D_HOOKFUNC(sithCogFunction_Truncate);
    J3D_HOOKFUNC(sithCogFunction_Abs);
    J3D_HOOKFUNC(sithCogFunction_Pow);
    J3D_HOOKFUNC(sithCogFunction_Sin);
    J3D_HOOKFUNC(sithCogFunction_Cos);
    J3D_HOOKFUNC(sithCogFunction_ArcTan);
    J3D_HOOKFUNC(sithCogFunction_SetWeaponModel);
    J3D_HOOKFUNC(sithCogFunction_ResetWeaponModel);
    J3D_HOOKFUNC(sithCogFunction_LoadHolsterModel);
    J3D_HOOKFUNC(sithCogFunction_SetHolsterModel);
    J3D_HOOKFUNC(sithCogFunction_ResetHolsterModel);
    J3D_HOOKFUNC(sithCogFunction_GetLastPistol);
    J3D_HOOKFUNC(sithCogFunction_GetLastRifle);
    J3D_HOOKFUNC(sithCogFunction_InEditor);
    J3D_HOOKFUNC(sithCogFunction_CopyPlayerHolsters);
    J3D_HOOKFUNC(sithCogFunction_GetPerformanceLevel);
    J3D_HOOKFUNC(sithCogFunction_IsLevelName);
    J3D_HOOKFUNC(sithCogFunction_SetFog);
    J3D_HOOKFUNC(sithCogFunction_FindNewSector);
    J3D_HOOKFUNC(sithCogFunction_FindNewSectorFromThing);
}

void sithCogFunction_ResetGlobals(void)
{}
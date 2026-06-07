#include "sithDSSCog.h"
#include <j3dcore/j3dhook.h>

#include <sith/Cog/sithCog.h>
#include <sith/Dss/sithDss.h>
#include <sith/Dss/sithMulti.h>
#include <sith/RTI/symbols.h>
#include <sith/World/sithThing.h>
#include <sith/World/sithWorld.h>

void sithDSSCog_InstallHooks(void)
{
    J3D_HOOKFUNC(sithDSSCog_SendMessage);
    J3D_HOOKFUNC(sithDSSCog_ProcessMessage);
    J3D_HOOKFUNC(sithDSSCog_SyncCogState);
    J3D_HOOKFUNC(sithDSSCog_ProcessCogState);
}

void sithDSSCog_ResetGlobals(void)
{}

int J3DAPI sithDSSCog_SendMessage(const SithCog* pCog, SithCogMsgType msgType, SithCogSymbolRefType senderType, int senderIdx, SithCogSymbolRefType srcType, int srcIdx, int linkId, int param0, int param1, int param2, int param3, DPID idTo)
{
    SITHDSS_STARTOUT(SITHDSS_COGMESSAGE);
    SITHDSS_PUSHINT32(linkId);

   /**(uint32_t*)sithMulti_g_message.data =*/

    if ( pCog )
    {
        SITHDSS_PUSHINT16(pCog->idx);
        //*(uint16_t*)&sithMulti_g_message.data[4] =
    }
    else
    {
        SITHDSS_PUSHINT16(-1);
        //*(uint16_t*)&sithMulti_g_message.data[4] = -1;
    }

    SITHDSS_PUSHUINT8(senderType);
    SITHDSS_PUSHUINT8(srcType);

    //sithMulti_g_message.data[6] = senderType;
    //sithMulti_g_message.data[7] = srcType;

    SithThing* pSenderThing;
    if ( senderType == SITHCOG_SYM_REF_THING && (pSenderThing = sithThing_GetThingByIndex(senderIdx)) != NULL )
    {
        SITHDSS_PUSHINT32(pSenderThing->guid);
        //*(uint32_t*)&sithMulti_g_message.data[8] = pSenderThing->guid;
    }
    else
    {
        SITHDSS_PUSHINT32(senderIdx);
        //*(uint32_t*)&sithMulti_g_message.data[8] = senderIdx;
    }

    SithThing* pSrcThing;
    if ( srcType == SITHCOG_SYM_REF_THING && (pSrcThing = sithThing_GetThingByIndex(srcIdx)) != NULL )
    {
        SITHDSS_PUSHINT32(pSrcThing->guid);
        //*(uint32_t*)&sithMulti_g_message.data[12] = pSrcThing->guid;
    }
    else
    {
        SITHDSS_PUSHINT32(srcIdx);
        //*(uint32_t*)&sithMulti_g_message.data[12] = srcIdx;
    }

    SITHDSS_PUSHUINT16((uint8_t)msgType); // TODO: Why is msgType cast to uint8_t and then pushed as uint16_t?
    SITHDSS_PUSHINT32(param0);
    SITHDSS_PUSHINT32(param1);
    SITHDSS_PUSHINT32(param2);
    SITHDSS_PUSHINT32(param3);

    //*(uint16_t*)&sithMulti_g_message.data[16] = (uint8_t)msgType;
    //*(uint32_t*)&sithMulti_g_message.data[18] = param0;
    //*(uint32_t*)&sithMulti_g_message.data[22] = param1;
    //*(uint32_t*)&sithMulti_g_message.data[26] = param2;
    //*(uint32_t*)&sithMulti_g_message.data[30] = param3;

    /*sithMulti_g_message.type = SITHDSS_COGMESSAGE;
    sithMulti_g_message.length = 34;*/

    SITHDSS_ENDOUT;
    SITH_ASSERT(sithMulti_g_message.length == 34); // Added: Just sanity check

    //unsigned int sendflags = DPSEND_GUARANTEED;
    //if ( msgType == SITHCOG_MSG_TOUCHED )
    //{
    //    sendflags = 0;
    //}

    return sithMessage_SendMessage(&sithMulti_g_message, idTo, SITHMESSAGE_STREAM_NET, msgType == SITHCOG_MSG_TOUCHED ? 0 : DPSEND_GUARANTEED);
}

int J3DAPI sithDSSCog_ProcessMessage(const SithMessage* pMsg)
{
    SITH_ASSERTREL(pMsg);
    SITHDSS_STARTIN(pMsg);

    int linkId    = SITHDSS_POPINT32(); //*(uint32_t*)pMsg->data;
    SithCog* pCog = sithCog_GetCogByIndex(SITHDSS_POPUINT16()); // Deserialize as unsigned to avoid sign-extension issues

    SithCogSymbolRefType senderType = SITHDSS_POPUINT8(); //pMsg->data[6];
    SithCogSymbolRefType srcType    = SITHDSS_POPUINT8(); //pMsg->data[7];

    int senderIdx = SITHDSS_POPINT32(); //*(uint32_t*)&pMsg->data[8];
    int srcIdx    = SITHDSS_POPINT32(); //*(uint32_t*)&pMsg->data[12];

    if ( senderType == SITHCOG_SYM_REF_THING )
    {
        SithThing* pSenderThing = sithThing_GetGuidThing(senderIdx); // senderIdx is a guid
        if ( pSenderThing )
        {
            senderIdx = pSenderThing->idx;
        }
    }

    if ( srcType == SITHCOG_SYM_REF_THING )
    {
        SithThing* pSrcThing = sithThing_GetGuidThing(srcIdx); // srcIdx is a guid
        if ( pSrcThing )
        {
            srcIdx = pSrcThing->idx;
        }
    }

    SithCogMsgType msgType = SITHDSS_POPUINT16(); //*(uint16_t*)&pMsg->data[16];
    int param0 = SITHDSS_POPINT32(); //*(uint32_t*)&pMsg->data[18];
    int param1 = SITHDSS_POPINT32(); //*(uint32_t*)&pMsg->data[22];
    int param2 = SITHDSS_POPINT32(); //*(uint32_t*)&pMsg->data[26];
    int param3 = SITHDSS_POPINT32(); //*(uint32_t*)&pMsg->data[30];

    SITHDSS_ENDIN;

    if ( pCog )
    {
        SithThing* pSenderThing;
        if ( senderType != SITHCOG_SYM_REF_THING || (pSenderThing = sithThing_GetThingByIndex(senderIdx)) != NULL && (pSenderThing->flags & SITH_TF_COGLINKED) != 0 )
        {
            sithCog_SendMessageEx(pCog, msgType, senderType, senderIdx, srcType, srcIdx, linkId, param0, param1, param2, param3);
            return 1;
        }

        return 1;
    }

    // If the message isn't for a specific cog, broadcast it to all cogs
    sithCog_BroadcastMessageEx(msgType, senderType, senderIdx, srcType, srcIdx, param0, param1, param2, param3);
    return 1;
}

int J3DAPI sithDSSCog_SyncCogState(const SithCog* pCog, DPID idTO, SithMessageStream outstream)
{
    SITH_ASSERTREL(pCog);
    SITHDSS_STARTOUT(SITHDSS_COGSTATE);

    SITHDSS_PUSHINT32(pCog->idx);
    SITHDSS_PUSHINT32(pCog->status);
    SITHDSS_PUSHINT32(pCog->flags);

    /**(uint32_t*)sithMulti_g_message.data = pCog->idx;
    *(uint32_t*)&sithMulti_g_message.data[4] = pCog->status;
    *(uint32_t*)&sithMulti_g_message.data[8] = pCog->flags;*/

    // uint8_t* pCurOut = &sithMulti_g_message.data[12];
    if ( pCog->status )
    {
        SITHDSS_PUSHINT32(pCog->statusParams[0]);
        SITHDSS_PUSHINT32(pCog->statusParams[1]);
        SITHDSS_PUSHUINT32(pCog->execPos);
        SITHDSS_PUSHINT32(pCog->linkId);
        SITHDSS_PUSHINT32(pCog->senderIdx);
        SITHDSS_PUSHINT32(pCog->senderType);
        SITHDSS_PUSHINT32(pCog->sourceIdx);
        SITHDSS_PUSHINT32(pCog->sourceType);
        SITH_ASSERT(SITHDSS_CURPOS() == 44);

        //*(uint32_t*)pCurOut = pCog->statusParams[0];
        //*(uint32_t*)&sithMulti_g_message.data[16] = pCog->statusParams[1];
        //*(uint32_t*)&sithMulti_g_message.data[20] = pCog->execPos;
        //*(uint32_t*)&sithMulti_g_message.data[24] = pCog->linkId;
        //*(uint32_t*)&sithMulti_g_message.data[28] = pCog->senderIdx;
        //*(uint32_t*)&sithMulti_g_message.data[32] = pCog->senderType;
        //*(uint32_t*)&sithMulti_g_message.data[36] = pCog->sourceIdx;
        //*(uint32_t*)&sithMulti_g_message.data[40] = pCog->sourceType;
        //pCurOut = &sithMulti_g_message.data[44];
    }

    if ( (pCog->flags & SITHCOG_PULSE_SET) != 0 )
    {
        SITHDSS_PUSHUINT32(pCog->msecPulseInterval);
        /**(uint32_t*)pCurOut = pCog->msecPulseInterval;
        pCurOut += 4;*/

        SITHDSS_PUSHUINT32(pCog->msecNextPulseTime);
        /**(uint32_t*)pCurOut = pCog->msecNextPulseTime;
        pCurOut += 4;*/
    }

    if ( (pCog->flags & SITHCOG_TIMER_SET) != 0 )
    {
        SITHDSS_PUSHUINT32(pCog->msecTimerTimeout);
        /**(uint32_t*)pCurOut = pCog->msecTimerTimeout;
        pCurOut += 4;*/
    }

    for ( size_t i = 0; i < pCog->pSymbolTable->numUsedSymbols; ++i )
    {
        SITHDSS_PUSHUINT8(pCog->pSymbolTable->aSymbols[i].value.type);
        //*pCurOut++ = pCog->pSymbolTable->aSymbols[i].value.type;
    }

    for ( size_t i = 0; i < pCog->pSymbolTable->numUsedSymbols; ++i )
    {
        SithCogValueType type = pCog->pSymbolTable->aSymbols[i].value.type;
        if ( type == SITHCOG_VALUE_FLOAT )
        {
            SITHDSS_PUSHFLOAT(pCog->pSymbolTable->aSymbols[i].value.val.floatValue);
            /**(float*)pCurOut = pCog->pSymbolTable->aSymbols[i].value.val.floatValue;
            pCurOut += 4;*/
        }

        else if ( type != SITHCOG_VALUE_STRING )
        {
            if ( type == SITHCOG_VALUE_VECTOR )
            {
                SITHDSS_PUSHVEC3(&pCog->pSymbolTable->aSymbols[i].value.val.vecValue);
                /*memcpy(pCurOut, &pCog->pSymbolTable->aSymbols[i].value.val, sizeof(rdVector3));
                pCurOut += 12;*/
            }
            else
            {
                SITHDSS_PUSHINT32(pCog->pSymbolTable->aSymbols[i].value.val.intValue);
                /**(uint32_t*)pCurOut = pCog->pSymbolTable->aSymbols[i].value.val.intValue;
                pCurOut += 4;*/
            }
        }
    }

    SITHDSS_ENDOUT;
    /*sithMulti_g_message.type = SITHDSS_COGSTATE;
    sithMulti_g_message.length = pCurOut - sithMulti_g_message.data;*/

    return sithMessage_SendMessage(&sithMulti_g_message, idTO, (SithMessageStream)outstream, DPSEND_GUARANTEED);
}

int J3DAPI sithDSSCog_ProcessCogState(const SithMessage* pMsg)
{
    SITH_ASSERTREL(pMsg && sithWorld_g_pCurrentWorld);
    SITHDSS_STARTIN(pMsg);

    SithCog* pCog = sithCog_GetCogByIndex(SITHDSS_POPINT32()); // sithCog_GetCogByIndex(*(uint32_t*)pMsg->data);
    if ( !pCog )
    {
        SITHDSS_ENDIN;
        return 0;
    }

    pCog->status = SITHDSS_POPINT32(); //*(uint32_t*)&pMsg->data[4];
    pCog->flags  = SITHDSS_POPINT32(); // *(uint32_t*)&pMsg->data[8];

    //uint8_t* pCurIn = &pMsg->data[12];
    if ( pCog->status )
    {
        pCog->statusParams[0] = SITHDSS_POPINT32();  // *(uint32_t*)pCurIn;
        pCog->statusParams[1] = SITHDSS_POPINT32();  //*(uint32_t*)&pMsg->data[16];
        pCog->execPos         = SITHDSS_POPUINT32(); //*(uint32_t*)&pMsg->data[20];
        pCog->linkId          = SITHDSS_POPINT32();  // *(uint32_t*)&pMsg->data[24];
        pCog->senderIdx       = SITHDSS_POPINT32();  //*(uint32_t*)&pMsg->data[28];
        pCog->senderType      = SITHDSS_POPINT32();  //*(uint32_t*)&pMsg->data[32];
        pCog->sourceIdx       = SITHDSS_POPINT32();  //*(uint32_t*)&pMsg->data[36];
        pCog->sourceType      = SITHDSS_POPINT32();  //*(uint32_t*)&pMsg->data[40];
        //pCurIn = &pMsg->data[44];
        SITH_ASSERT(SITHDSS_CURPOS() == 44);
    }

    if ( (pCog->flags & SITHCOG_PULSE_SET) != 0 )
    {
        pCog->msecPulseInterval = SITHDSS_POPUINT32(); //*(uint32_t*)pCurIn;
        //pCurIn += 4;
        pCog->msecNextPulseTime = SITHDSS_POPUINT32(); //*(uint32_t*)pCurIn;
        //pCurIn += 4;
    }

    if ( (pCog->flags & SITHCOG_TIMER_SET) != 0 )
    {
        pCog->msecTimerTimeout = SITHDSS_POPUINT32(); //*(uint32_t*)pCurIn;
        //pCurIn += 4;
    }

    for ( size_t i = 0; i < pCog->pSymbolTable->numUsedSymbols; ++i )
    {
        pCog->pSymbolTable->aSymbols[i].value.type = SITHDSS_POPUINT8(); //*pCurIn++;
    }

    for ( size_t i = 0; i < pCog->pSymbolTable->numUsedSymbols; ++i )
    {
        SithCogValueType type = pCog->pSymbolTable->aSymbols[i].value.type;
        if ( type == SITHCOG_VALUE_FLOAT )
        {
            pCog->pSymbolTable->aSymbols[i].value.val.floatValue = SITHDSS_POPFLOAT(); //*(float*)pCurIn;
            //pCurIn += 4;
        }
        else if ( type != SITHCOG_VALUE_STRING )
        {
            if ( type == SITHCOG_VALUE_VECTOR )
            {
                SITHDSS_POPVEC3(&pCog->pSymbolTable->aSymbols[i].value.val.vecValue);
                /*memcpy(&pCog->pSymbolTable->aSymbols[i].value.val, pCurIn, sizeof(pCog->pSymbolTable->aSymbols[i].value.val));
                pCurIn += 12;*/
            }
            else
            {
                pCog->pSymbolTable->aSymbols[i].value.val.intValue = SITHDSS_POPINT32(); //*(uint32_t*)pCurIn;
                //pCurIn += 4;
            }
        }
    }

    SITHDSS_ENDIN;
    return 1;
}
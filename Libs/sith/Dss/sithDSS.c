#include "sithDSS.h"
#include <j3dcore/j3dhook.h>

#include <rdroid/Engine/rdPuppet.h>

#include <sith/AI/sithAIMove.h>
#include <sith/Devices/sithComm.h>
#include <sith/Dss/sithMulti.h>
#include <sith/Engine/sithPuppet.h>
#include <sith/Gameplay/sithPlayerControls.h>
#include <sith/Gameplay/sithVehicleControls.h>
#include <sith/Gameplay/sithWhip.h>
#include <sith/RTI/symbols.h>
#include <sith/World/sithThing.h>
#include <sith/World/sithWorld.h>

#include <std/General/stdUtil.h>

#define VANILLACB(callback) ((rdPuppetTrackCallback)(callback##_ADDR))

static const rdPuppetTrackCallback sithDSS_aPuppetCallbacks[7][2] = {
    { &sithPuppet_DefaultCallback,           VANILLACB(sithPuppet_DefaultCallback)           },
    { &sithPlayerControls_PuppetCallback,    VANILLACB(sithPlayerControls_PuppetCallback)    },
    { &sithVehicleControls_PuppetCallback,   VANILLACB(sithVehicleControls_PuppetCallback)   },
    { &sithAIMove_PuppetCallback,            VANILLACB(sithAIMove_PuppetCallback)            },
    { &sithWhip_WhipClimbPuppetCallback,     VANILLACB(sithWhip_WhipClimbPuppetCallback)     },
    { &sithWhip_ClimbDismountPuppetCallback, VANILLACB(sithWhip_ClimbDismountPuppetCallback) },
    { &sithWhip_WhipFirePuppetCallback,      VANILLACB(sithWhip_WhipFirePuppetCallback)      }
};

// TODO: When all functions that are using following callbacks are defined, uncomment below const and remove previous definition above
//static const rdPuppetTrackCallback sithDSS_aPuppetCallbacks[7] = {
//    &sithPuppet_DefaultCallback,
//    &sithPlayerControls_PuppetCallback,
//    &sithVehicleControls_PuppetCallback,
//    &sithAIMove_PuppetCallback,
//    &sithWhip_WhipClimbPuppetCallback,
//    &sithWhip_ClimbDismountPuppetCallback,
//    &sithWhip_WhipFirePuppetCallback
//};

void sithDSS_InstallHooks(void)
{
    // Uncomment only lines for functions that have full definition and doesn't call original function (non-thunk functions)

    // J3D_HOOKFUNC(sithDSS_SurfaceStatus);
    // J3D_HOOKFUNC(sithDSS_ProcessSurfaceStatus);
    // J3D_HOOKFUNC(sithDSS_SectorStatus);
    // J3D_HOOKFUNC(sithDSS_SyncSeenSectors);
    // J3D_HOOKFUNC(sithDSS_ProcessSyncSeenSecors);
    // J3D_HOOKFUNC(sithDSS_ProcessSectorStatus);
    // J3D_HOOKFUNC(sithDSS_SectorFlags);
    // J3D_HOOKFUNC(sithDSS_ProcessSectorFlags);
    // J3D_HOOKFUNC(sithDSS_AIStatus);
    // J3D_HOOKFUNC(sithDSS_ProcessAIStatus);
    // J3D_HOOKFUNC(sithDSS_Inventory);
    // J3D_HOOKFUNC(sithDSS_ProcessInventory);
    // J3D_HOOKFUNC(sithDSS_AnimStatus);
    // J3D_HOOKFUNC(sithDSS_ProcessAnimStatus);
    J3D_HOOKFUNC(sithDSS_PuppetStatus);
    J3D_HOOKFUNC(sithDSS_ProcessPuppetStatus);
    // J3D_HOOKFUNC(sithDSS_SyncTaskEvents);
    // J3D_HOOKFUNC(sithDSS_ProcessSyncTaskEvents);
    // J3D_HOOKFUNC(sithDSS_SyncCameras);
    // J3D_HOOKFUNC(sithDSS_ProcessSyncCameras);
    // J3D_HOOKFUNC(sithDSS_SyncGameState);
    // J3D_HOOKFUNC(sithDSS_ProcessSyncGameState);
    // J3D_HOOKFUNC(sithDSS_SyncVehicleControlState);
    // J3D_HOOKFUNC(sithDSS_ProcessVehicleControlsState);
    // J3D_HOOKFUNC(sithDSS_sub_4B3760);
    // J3D_HOOKFUNC(sithDSS_sub_4B3790);
}

void sithDSS_ResetGlobals(void)
{}

int J3DAPI sithDSS_SurfaceStatus(const SithSurface* pSurf, DPID idTo, unsigned int outstream)
{
    return J3D_TRAMPOLINE_CALL(sithDSS_SurfaceStatus, pSurf, idTo, outstream);
}

int J3DAPI sithDSS_ProcessSurfaceStatus(const SithMessage* pMsg)
{
    return J3D_TRAMPOLINE_CALL(sithDSS_ProcessSurfaceStatus, pMsg);
}

int J3DAPI sithDSS_SectorStatus(const SithSector* pSector, DPID a2, unsigned int outstream)
{
    return J3D_TRAMPOLINE_CALL(sithDSS_SectorStatus, pSector, a2, outstream);
}

int J3DAPI sithDSS_SyncSeenSectors(DPID idTo, unsigned int outstream)
{
    return J3D_TRAMPOLINE_CALL(sithDSS_SyncSeenSectors, idTo, outstream);
}

int J3DAPI sithDSS_ProcessSyncSeenSecors(const SithMessage* pMsg)
{
    return J3D_TRAMPOLINE_CALL(sithDSS_ProcessSyncSeenSecors, pMsg);
}

int J3DAPI sithDSS_ProcessSectorStatus(const SithMessage* pMsg)
{
    return J3D_TRAMPOLINE_CALL(sithDSS_ProcessSectorStatus, pMsg);
}

int J3DAPI sithDSS_SectorFlags(const SithSector* pSector, DPID idTo, unsigned int outstream)
{
    return J3D_TRAMPOLINE_CALL(sithDSS_SectorFlags, pSector, idTo, outstream);
}

int J3DAPI sithDSS_ProcessSectorFlags(const SithMessage* pMsg)
{
    return J3D_TRAMPOLINE_CALL(sithDSS_ProcessSectorFlags, pMsg);
}

int J3DAPI sithDSS_AIStatus(const SithAIControlBlock* pLocal, DPID idTo, signed int outstream)
{
    return J3D_TRAMPOLINE_CALL(sithDSS_AIStatus, pLocal, idTo, outstream);
}

int J3DAPI sithDSS_ProcessAIStatus(const SithMessage* pMsg)
{
    return J3D_TRAMPOLINE_CALL(sithDSS_ProcessAIStatus, pMsg);
}

int J3DAPI sithDSS_Inventory(const SithThing* pThing, unsigned int inventoryId, DPID idTo, unsigned int outstream)
{
    return J3D_TRAMPOLINE_CALL(sithDSS_Inventory, pThing, inventoryId, idTo, outstream);
}

int J3DAPI sithDSS_ProcessInventory(const SithMessage* pMsg)
{
    return J3D_TRAMPOLINE_CALL(sithDSS_ProcessInventory, pMsg);
}

int J3DAPI sithDSS_AnimStatus(const SithAnimationSlot* pAnim, DPID idTo, unsigned int outstream)
{
    return J3D_TRAMPOLINE_CALL(sithDSS_AnimStatus, pAnim, idTo, outstream);
}

int J3DAPI sithDSS_ProcessAnimStatus(const SithMessage* pMsg)
{
    return J3D_TRAMPOLINE_CALL(sithDSS_ProcessAnimStatus, pMsg);
}

int J3DAPI sithDSS_PuppetStatus(const SithThing* pThing, DPID idTo, unsigned int outstream)
{
    SITH_ASSERTREL(pThing && pThing->renderData.pPuppet);
    rdPuppet* pPuppet = pThing->renderData.pPuppet;

    SITHDSS_STARTOUT(SITHDSS_PUPPETSTATUS);

    SITHDSS_PUSHINT32(sithThing_GetThingIndex(pThing));
    //*(int32_t*)sithMulti_g_message.data = sithThing_GetThingIndex(pThing);

    //uint8_t* pCurOut = &sithMulti_g_message.data[4];

    size_t trackCount = 0;
    for ( size_t i = 0; i < STD_ARRAYLEN(pPuppet->aTracks); ++i )
    {
        SITHDSS_PUSHINT32(pPuppet->aTracks[i].status);
        /**(int32_t*)pCurOut = pPuppet->aTracks[i].status;
        pCurOut += 4;*/

        if ( pPuppet->aTracks[i].status )
        {
            SITH_ASSERTREL(pPuppet->aTracks[i].pKFTrack);

            SITHDSS_PUSHINT32(pPuppet->aTracks[i].pKFTrack->idx);
            /**(int32_t*)pCurOut = pPuppet->aTracks[i].pKFTrack->idx;
            pCurOut += 4;*/

            // Altered: Changed to serialize 32bit float type, as struct field was repurposed.
            //          Originally this field was not used but was serialized as 32 bit integer
            SITHDSS_PUSHFLOAT(pPuppet->aTracks[i].playbackSpeed);
           // SITHDSS_PUSHINT32(pPuppet->aTracks[i].unknown0);
            /**(int32_t*)pCurOut = pPuppet->aTracks[i].unknown0;
            pCurOut += 4;*/

            SITHDSS_PUSHINT16(pPuppet->aTracks[i].lowPriority);
            /**(int16_t*)pCurOut = (uint16_t)pPuppet->aTracks[i].lowPriority;
            pCurOut += 2;*/

            SITHDSS_PUSHINT16(pPuppet->aTracks[i].highPriority);

            /**(int16_t*)pCurOut = (uint16_t)pPuppet->aTracks[i].highPriority;
            pCurOut += 2;*/

            SITHDSS_PUSHFLOAT(pPuppet->aTracks[i].fps);
            /**(float*)pCurOut = pPuppet->aTracks[i].fps;
            pCurOut += 4;*/

            SITHDSS_PUSHFLOAT(pPuppet->aTracks[i].blendWeight);
            /* *(float*)pCurOut = pPuppet->aTracks[i].blendWeight;
             pCurOut += 4;*/

            SITHDSS_PUSHFLOAT(pPuppet->aTracks[i].curFrame);
            /**(float*)pCurOut = pPuppet->aTracks[i].curFrame;
            pCurOut += 4;*/

            SITHDSS_PUSHFLOAT(pPuppet->aTracks[i].prevFrame);
            /**(float*)pCurOut = pPuppet->aTracks[i].prevFrame;
            pCurOut += 4;*/

            if ( pPuppet->aTracks[i].pfCallback )
            {
                size_t cbIdx = 0;
                for ( ; cbIdx < STD_ARRAYLEN(sithDSS_aPuppetCallbacks); ++cbIdx )
                {
                    if ( pPuppet->aTracks[i].pfCallback == sithDSS_aPuppetCallbacks[cbIdx][0]
                        || pPuppet->aTracks[i].pfCallback == sithDSS_aPuppetCallbacks[cbIdx][1] )
                    {
                        SITHDSS_PUSHUINT8(cbIdx);
                        //*pCurOut++ = (uint8_t)cbIdx;
                        break;
                    }
                }

                if ( cbIdx == STD_ARRAYLEN(sithDSS_aPuppetCallbacks) )
                {
                    SITHLOG_ERROR("Saving Unknown callback for %s\n", pPuppet->aTracks[i].pKFTrack->aName);
                    SITHDSS_PUSHUINT8(STD_ARRAYLEN(sithDSS_aPuppetCallbacks)); // mark there is no callback
                    //*pCurOut++ = STD_ARRAYLEN(sithDSS_aPuppetCallbacks);
                }
            }
            else // mark there is no callback
            {
                SITHDSS_PUSHUINT8(STD_ARRAYLEN(sithDSS_aPuppetCallbacks));
                //*pCurOut++ = STD_ARRAYLEN(sithDSS_aPuppetCallbacks);
            }

            ++trackCount;
        }
    }

    if ( pThing->pPuppetState )
    {
        SITHDSS_PUSHINT16(pThing->pPuppetState->armedMode);
        /**(int16_t*)pCurOut = (int16_t)pThing->pPuppetState->armedMode;
        pCurOut += 2;*/

        SITHDSS_PUSHINT16(pThing->pPuppetState->moveMode);
        /* *(int16_t*)pCurOut = (int16_t)pThing->pPuppetState->moveMode;
         pCurOut += 2;*/

        SITHDSS_PUSHINT16(trackCount);

        /**(int16_t*)pCurOut = (int16_t)trackCount;
        pCurOut += 2;*/

        SithPuppetTrack* pCurTrack = pThing->pPuppetState->pFirstTrack;
        while ( trackCount )
        {
            if ( pCurTrack )
            {
                SITHDSS_PUSHINT32(pCurTrack->trackNum);
                /**(int32_t*)pCurOut = pCurTrack->trackNum;
                pCurOut += 4;*/

                SITHDSS_PUSHUINT32(pCurTrack->submode);
                /**(uint32_t*)pCurOut = pCurTrack->submode;
                pCurOut += 4;*/

                pCurTrack = pCurTrack->pNextTrack;
            }
            else // If no track
            {
                SITHDSS_PUSHUINT32(STD_ARRAYLEN(pPuppet->aTracks)); // mark null track
                /**(uint32_t*)pCurOut = STD_ARRAYLEN(pPuppet->aTracks);
                pCurOut += 4;*/

                SITHDSS_PUSHUINT32(SITH_PUPPET_NUMSUBMODES); // mark no mode
                /**(uint32_t*)pCurOut = SITH_PUPPET_NUMSUBMODES;
                pCurOut += 4;*/
            }

            --trackCount;
        }
    }

    /*sithMulti_g_message.type = SITHDSS_PUPPETSTATUS;
    sithMulti_g_message.length = pCurOut - sithMulti_g_message.data;*/
    SITHDSS_ENDOUT;
    return sithMessage_SendMessage(&sithMulti_g_message, idTo, outstream, 1u);
}

int J3DAPI sithDSS_ProcessPuppetStatus(const SithMessage* pMsg)
{
    SITH_ASSERTREL(pMsg);

    SITHDSS_STARTIN(pMsg);

    SithThing* pThing = sithThing_GetThingByIndex(SITHDSS_POPINT32());
    //uint8_t* pCurIn   = (uint8_t*)&pMsg->data[4];
    //SithThing* pThing = sithThing_GetThingByIndex(*(int32_t*)pMsg->data);

    if ( !pThing || !pThing->pPuppetClass || !pThing->pPuppetState || !pThing->renderData.pPuppet )
    {
        SITHLOG_ERROR("Error in DSS::PuppetStatus message format.\n");
        SITHDSS_ENDIN;
        return 0;
    }

    rdPuppet* pPuppet = pThing->renderData.pPuppet;
    rdPuppet_NewEntry(pPuppet, &pThing->renderData);

    for ( size_t i = 0; i < STD_ARRAYLEN(pPuppet->aTracks); ++i )
    {
        pPuppet->aTracks[i].status = SITHDSS_POPINT32();
        /*pPuppet->aTracks[i].status = *(int32_t*)pCurIn;
        pCurIn += 4;*/

        if ( pPuppet->aTracks[i].status )
        {
            int kfIdx = SITHDSS_POPINT32();
            /*int kfIdx = *(int32_t*)pCurIn;
            pCurIn += 4;*/

            pPuppet->aTracks[i].pKFTrack = sithPuppet_GetKeyframeByIndex(kfIdx);

            // Altered: Changed to deserialize 32bit float type, as struct field was repurposed.
            //          Originally this field was not used but was desrialized as 32 bit integer
            pPuppet->aTracks[i].playbackSpeed = SITHDSS_POPFLOAT();
            if ( pPuppet->aTracks[i].playbackSpeed <= 0.0f ) // Added: Added check for 0, and in this case set it to 1.0f
            {
                pPuppet->aTracks[i].playbackSpeed = 1.0f;
            }

           // pPuppet->aTracks[i].unknown0 = SITHDSS_POPINT32();
            /*pPuppet->aTracks[i].unknown0 = *(int32_t*)pCurIn;
            pCurIn += 4;*/

            pPuppet->aTracks[i].lowPriority = SITHDSS_POPINT16();
            /*pPuppet->aTracks[i].lowPriority = *(int16_t*)pCurIn;
            pCurIn += 2;*/

            pPuppet->aTracks[i].highPriority = SITHDSS_POPINT16();
            /* pPuppet->aTracks[i].highPriority = *(int16_t*)pCurIn;
             pCurIn += 2;*/

            pPuppet->aTracks[i].fps = SITHDSS_POPFLOAT();
            /*pPuppet->aTracks[i].fps = *(float*)pCurIn;
            pCurIn += 4;*/

            pPuppet->aTracks[i].blendWeight = SITHDSS_POPFLOAT();
            /*pPuppet->aTracks[i].blendWeight = *(float*)pCurIn;
            pCurIn += 4;*/

            pPuppet->aTracks[i].curFrame = SITHDSS_POPFLOAT();
            /*pPuppet->aTracks[i].curFrame = *(float*)pCurIn;
            pCurIn += 4;*/

            pPuppet->aTracks[i].prevFrame = SITHDSS_POPFLOAT();
            /*pPuppet->aTracks[i].prevFrame = *(float*)pCurIn;
            pCurIn += 4;*/

            size_t cbIdx = SITHDSS_POPUINT8();
            //size_t cbIdx = *pCurIn++;
            if ( cbIdx < STD_ARRAYLEN(sithDSS_aPuppetCallbacks) )
            {
                pPuppet->aTracks[i].pfCallback = sithDSS_aPuppetCallbacks[cbIdx][0];
            }
            else
            {
                pPuppet->aTracks[i].pfCallback = NULL;
            }
        }
    }

    if ( !pThing->pPuppetState )
    {
        SITHDSS_ENDIN;
        return 1;
    }

    uint32_t armMode = SITHDSS_POPINT16();
    /*uint32_t armMode = *(int16_t*)pCurIn;
    pCurIn += 2;*/

    // Fixed: Reject save/network puppet modes before they reach mode-indexed arrays.
    if ( armMode >= SITH_PUPPET_NUMARMEDMODES )
    {
        SITHLOG_ERROR("DSS::PuppetStatus received invalid armed mode %d.\n", armMode);
        SITHDSS_ENDIN;
        return 0;
    }

    sithPuppet_SetArmedMode(pThing, armMode);

    SithPuppetMoveMode moveMode = SITHDSS_POPUINT16();
    /*SithPuppetMoveMode moveMode = *(uint16_t*)pCurIn;
    pCurIn += 2;*/

    // Fixed: Reject save/network puppet move modes before sithPuppet_SetMoveMode asserts or indexes with them.
    if ( moveMode >= SITH_PUPPET_NUMMOVEMODES )
    {
        SITHLOG_ERROR("DSS::PuppetStatus received invalid move mode %d.\n", moveMode);
        SITHDSS_ENDIN;
        return 0;
    }

    int bControlsDisabled = false;
    if ( pThing->type == SITH_THING_PLAYER || pThing->type == SITH_THING_ACTOR )
    {
        bControlsDisabled = pThing->thingInfo.actorInfo.bControlsDisabled;
    }

    sithPuppet_SetMoveMode(pThing, moveMode);
    if ( pThing->type == SITH_THING_PLAYER || pThing->type == SITH_THING_ACTOR )
    {
        pThing->thingInfo.actorInfo.bControlsDisabled = bControlsDisabled;
    }

    size_t trackCount = SITHDSS_POPUINT16();
    /*size_t trackCount = *(uint16_t*)pCurIn;
    pCurIn += 2;*/

    // Fixed: The stream only writes one entry per puppet track, so larger counts are corrupt.
    if ( trackCount > STD_ARRAYLEN(pPuppet->aTracks) )
    {
        SITHLOG_ERROR("DSS::PuppetStatus received invalid track count %d.\n", (int)trackCount);
        SITHDSS_ENDIN;
        return 0;
    }

    // Fixed: Prevent restored state from indexing outside the puppet mode table.
    if ( (size_t)pThing->pPuppetState->majorMode >= SITH_PUPPET_MAXMODES )
    {
        SITHLOG_ERROR("DSS::PuppetStatus received invalid major mode %d.\n", pThing->pPuppetState->majorMode);
        SITHDSS_ENDIN;
        return 0;
    }

    while ( trackCount )
    {
        size_t trackNum = SITHDSS_POPINT32();
        SithPuppetSubMode submode = SITHDSS_POPUINT32();

        // Note: trackNum == STD_ARRAYLEN(pPuppet->aTracks) && submode == SITH_PUPPET_NUMSUBMODES is used to mark null track,
        //       so these values are valid but indicate no track rather than a real track index or submode.
        if ( trackNum < STD_ARRAYLEN(pPuppet->aTracks)
            && submode < SITH_PUPPET_NUMSUBMODES // Fixed: Added submode bounds check to prevent indexing outside the mode table
            && sithPuppet_NewTrack(pThing, &pThing->pPuppetClass->aModes[pThing->pPuppetState->majorMode][submode], trackNum, submode) )
        {
            SITHDSS_ENDIN;
            return 0;
        }

        --trackCount;
    }

    SITHDSS_ENDIN;
    return 1;
}

int J3DAPI sithDSS_SyncTaskEvents(const SithEvent* pEvent, DPID idTo, unsigned int outstream)
{
    return J3D_TRAMPOLINE_CALL(sithDSS_SyncTaskEvents, pEvent, idTo, outstream);
}

int J3DAPI sithDSS_ProcessSyncTaskEvents(const SithMessage* pMsg)
{
    return J3D_TRAMPOLINE_CALL(sithDSS_ProcessSyncTaskEvents, pMsg);
}

int J3DAPI sithDSS_SyncCameras(DPID idTo, unsigned int outstream)
{
    return J3D_TRAMPOLINE_CALL(sithDSS_SyncCameras, idTo, outstream);
}

int J3DAPI sithDSS_ProcessSyncCameras(const SithMessage* pMsg)
{
    return J3D_TRAMPOLINE_CALL(sithDSS_ProcessSyncCameras, pMsg);
}

int J3DAPI sithDSS_SyncGameState(DPID idTo, unsigned int outstream)
{
    return J3D_TRAMPOLINE_CALL(sithDSS_SyncGameState, idTo, outstream);
}

int J3DAPI sithDSS_ProcessSyncGameState(const SithMessage* pMsg)
{
    return J3D_TRAMPOLINE_CALL(sithDSS_ProcessSyncGameState, pMsg);
}

int J3DAPI sithDSS_SyncVehicleControlState(DPID idTo, unsigned int outstream)
{
    return J3D_TRAMPOLINE_CALL(sithDSS_SyncVehicleControlState, idTo, outstream);
}

int J3DAPI sithDSS_ProcessVehicleControlsState(const SithMessage* pMsg)
{
    return J3D_TRAMPOLINE_CALL(sithDSS_ProcessVehicleControlsState, pMsg);
}

int J3DAPI sithDSS_sub_4B3760(DPID idTo, unsigned int outstream)
{
    return J3D_TRAMPOLINE_CALL(sithDSS_sub_4B3760, idTo, outstream);
}

int J3DAPI sithDSS_sub_4B3790(const SithMessage* pMsg)
{
    return J3D_TRAMPOLINE_CALL(sithDSS_sub_4B3790, pMsg);
}
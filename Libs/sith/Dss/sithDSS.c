#include "sithDSS.h"
#include <j3dcore/j3dhook.h>

#include <rdroid/Engine/rdPuppet.h>

#include <sith/AI/sithAIMove.h>
#include <sith/Cog/sithCog.h>
#include <sith/Devices/sithComm.h>
#include <sith/Dss/sithMulti.h>
#include <sith/Engine/sithAnimate.h>
#include <sith/Engine/sithCamera.h>
#include <sith/Engine/sithPuppet.h>
#include <sith/Engine/sithRender.h>
#include <sith/Gameplay/sithEvent.h>
#include <sith/Gameplay/sithFX.h>
#include <sith/Gameplay/sithInventory.h>
#include <sith/Gameplay/sithPlayer.h>
#include <sith/Gameplay/sithPlayerActions.h>
#include <sith/Gameplay/sithPlayerControls.h>
#include <sith/Gameplay/sithTime.h>
#include <sith/Gameplay/sithVehicleControls.h>
#include <sith/Gameplay/sithWhip.h>
#include <sith/RTI/symbols.h>
#include <sith/World/sithMaterial.h>
#include <sith/World/sithModel.h>
#include <sith/World/sithSector.h>
#include <sith/World/sithSurface.h>
#include <sith/World/sithThing.h>
#include <sith/World/sithWorld.h>

#include <std/General/stdMath.h>
#include <std/General/stdMemory.h>
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

    J3D_HOOKFUNC(sithDSS_SurfaceStatus);
    J3D_HOOKFUNC(sithDSS_ProcessSurfaceStatus);
    J3D_HOOKFUNC(sithDSS_SectorStatus);
    J3D_HOOKFUNC(sithDSS_SyncSeenSectors);
    J3D_HOOKFUNC(sithDSS_ProcessSyncSeenSecors);
    J3D_HOOKFUNC(sithDSS_ProcessSectorStatus);
    J3D_HOOKFUNC(sithDSS_SectorFlags);
    J3D_HOOKFUNC(sithDSS_ProcessSectorFlags);
    J3D_HOOKFUNC(sithDSS_AIStatus);
    J3D_HOOKFUNC(sithDSS_ProcessAIStatus);
    J3D_HOOKFUNC(sithDSS_Inventory);
    J3D_HOOKFUNC(sithDSS_ProcessInventory);
    J3D_HOOKFUNC(sithDSS_AnimStatus);
    J3D_HOOKFUNC(sithDSS_ProcessAnimStatus);
    J3D_HOOKFUNC(sithDSS_PuppetStatus);
    J3D_HOOKFUNC(sithDSS_ProcessPuppetStatus);
    J3D_HOOKFUNC(sithDSS_SyncTaskEvents);
    J3D_HOOKFUNC(sithDSS_ProcessSyncTaskEvents);
    J3D_HOOKFUNC(sithDSS_SyncCameras);
    J3D_HOOKFUNC(sithDSS_ProcessSyncCameras);
    J3D_HOOKFUNC(sithDSS_SyncGameState);
    J3D_HOOKFUNC(sithDSS_ProcessSyncGameState);
    J3D_HOOKFUNC(sithDSS_SyncVehicleControlState);
    J3D_HOOKFUNC(sithDSS_ProcessVehicleControlsState);
    J3D_HOOKFUNC(sithDSS_SendEndSection);
    J3D_HOOKFUNC(sithDSS_ProcessEndSection);
}

void sithDSS_ResetGlobals(void)
{}

int J3DAPI sithDSS_SurfaceStatus(const SithSurface* pSurf, DPID idTo, unsigned int outstream)
{
    SITH_ASSERTREL(pSurf);

    SITHDSS_STARTOUT(SITHDSS_SURFACESTATUS);

    SITHDSS_PUSHINT16((int16_t)(sithSurface_GetSurfaceIndex(pSurf)));
    SITHDSS_PUSHINT32(pSurf->flags);
    if ( pSurf->face.pMaterial )
    {
        SITHDSS_PUSHINT32(pSurf->face.pMaterial->num);
    }
    else
    {
        SITHDSS_PUSHINT32(-1);
    }

    SITHDSS_PUSHINT16((int16_t)pSurf->face.matCelNum);
    SITHDSS_PUSHFLOAT(pSurf->face.texVertOffset.x);
    SITHDSS_PUSHFLOAT(pSurf->face.texVertOffset.y);
    SITHDSS_PUSHFLOAT(pSurf->face.extraLight.red);
    SITHDSS_PUSHFLOAT(pSurf->face.extraLight.green);
    SITHDSS_PUSHFLOAT(pSurf->face.extraLight.blue);
    SITHDSS_PUSHINT32(pSurf->face.flags);
    SITHDSS_PUSHINT32(pSurf->face.geometryMode);
    SITHDSS_PUSHINT32(pSurf->face.lightingMode);
    if ( pSurf->pAdjoin )
    {
        SITHDSS_PUSHINT32(pSurf->pAdjoin->flags);
    }

    for ( size_t i = 0; i < pSurf->face.numVertices; ++i )
    {
        SITHDSS_PUSHFLOAT(pSurf->aIntensities[i].alpha);
    }

    SITHDSS_ENDOUT;
    return sithMessage_SendMessage(&sithMulti_g_message, idTo, outstream, DPSEND_GUARANTEED);
}

int J3DAPI sithDSS_ProcessSurfaceStatus(const SithMessage* pMsg)
{
    SITH_ASSERTREL(pMsg && sithWorld_g_pCurrentWorld);

    SITHDSS_STARTIN(pMsg);

    size_t index = SITHDSS_POPUINT16();
    if ( index >= sithWorld_g_pCurrentWorld->numSurfaces )
    {
        SITHLOG_ERROR("DSS::SurfaceStatus..Surface %d out of range.\n", index);
        SITHDSS_ENDIN;
        return 0;
    }

    SithSurface* pSurf = &sithWorld_g_pCurrentWorld->aSurfaces[index];
    pSurf->flags = SITHDSS_POPINT32();

    // Note: Material index -1 (no material) results in NULL material
    pSurf->face.pMaterial = sithMaterial_GetMaterialByIndex(SITHDSS_POPINT32());

    int curCelNum = SITHDSS_POPUINT16();
    if ( curCelNum != -1 && pSurf->face.pMaterial && curCelNum < (int)pSurf->face.pMaterial->numCels )
    {
        pSurf->face.matCelNum = curCelNum;
    }
    else
    {
        pSurf->face.matCelNum = -1;
    }

    pSurf->face.texVertOffset.x  = SITHDSS_POPFLOAT();
    pSurf->face.texVertOffset.y  = SITHDSS_POPFLOAT();
    pSurf->face.extraLight.red   = SITHDSS_POPFLOAT();
    pSurf->face.extraLight.green = SITHDSS_POPFLOAT();
    pSurf->face.extraLight.blue  = SITHDSS_POPFLOAT();
    pSurf->face.flags            = SITHDSS_POPINT32();
    pSurf->face.geometryMode     = SITHDSS_POPINT32();
    pSurf->face.lightingMode     = SITHDSS_POPINT32();

    if ( pSurf->pAdjoin )
    {
        pSurf->pAdjoin->flags = SITHDSS_POPINT32();
    }

    for ( size_t i = 0; i < pSurf->face.numVertices; ++i )
    {
        pSurf->aIntensities[i].alpha = SITHDSS_POPFLOAT();
    }

    SITHDSS_ENDIN;
    return 1;
}

int J3DAPI sithDSS_SectorStatus(const SithSector* pSector, DPID a2, unsigned int outstream)
{
    SITH_ASSERTREL(pSector);

    SITHDSS_STARTOUT(SITHDSS_SECTORSTATUS);

    SITHDSS_PUSHINT16((int16_t)(sithSector_GetSectorIndex(pSector)));
    SITHDSS_PUSHINT32(pSector->flags);
    SITHDSS_PUSHFLOAT(pSector->ambientLight.red);
    SITHDSS_PUSHFLOAT(pSector->ambientLight.green);
    SITHDSS_PUSHFLOAT(pSector->ambientLight.blue);
    SITHDSS_PUSHFLOAT(pSector->extraLight.red);
    SITHDSS_PUSHFLOAT(pSector->extraLight.green);
    SITHDSS_PUSHFLOAT(pSector->extraLight.blue);
    if ( (pSector->flags & SITH_SECTOR_USETHRUST) != 0 )
    {
        SITHDSS_PUSHVEC3(&pSector->thrust);
    }

    SITHDSS_ENDOUT;
    return sithMessage_SendMessage(&sithMulti_g_message, a2, outstream, DPSEND_GUARANTEED);
}

int J3DAPI sithDSS_SyncSeenSectors(DPID idTo, unsigned int outstream)
{
    if ( !sithWorld_g_pCurrentWorld )
    {
        return 1;
    }

    SITHDSS_STARTOUT(SITHDSS_SYNCSEENSECTORS);

    for ( size_t i = 0; i < sithWorld_g_pCurrentWorld->numSectors; ++i )
    {
        const SithSector* pSector = &sithWorld_g_pCurrentWorld->aSectors[i];
        uint8_t bSeen = (pSector->flags & SITH_SECTOR_SEEN) != 0 ? 1 : 0;
        SITHDSS_PUSHUINT8((uint8_t)bSeen);
    }

    SITHDSS_ENDOUT;
    return sithMessage_SendMessage(&sithMulti_g_message, idTo, outstream, DPSEND_GUARANTEED);
}

int J3DAPI sithDSS_ProcessSyncSeenSecors(const SithMessage* pMsg)
{
    SITHDSS_STARTIN(pMsg);

    if ( !sithWorld_g_pCurrentWorld )
    {
        SITHDSS_ENDIN;
        return 0;
    }

    for ( size_t i = 0; i < sithWorld_g_pCurrentWorld->numSectors; ++i )
    {
        SithSector* pSector = &sithWorld_g_pCurrentWorld->aSectors[i];
        uint8_t bSeen = SITHDSS_POPUINT8();
        if ( bSeen )
        {
            pSector->flags |= SITH_SECTOR_SEEN;
        }
        else
        {
            pSector->flags &= ~SITH_SECTOR_SEEN;
        }
    }

    SITHDSS_ENDIN;
    return 1;
}

int J3DAPI sithDSS_ProcessSectorStatus(const SithMessage* pMsg)
{
    SITHDSS_STARTIN(pMsg);

    size_t sectorIdx = SITHDSS_POPUINT16();
    if ( sectorIdx >= sithWorld_g_pCurrentWorld->numSectors )
    {
        SITHLOG_ERROR("Sector %d out of range for DSS:SectorStatus.\n", sectorIdx);
        SITHDSS_ENDIN;
        return 0;
    }

    SithSector* pSector = &sithWorld_g_pCurrentWorld->aSectors[sectorIdx];

    SithSectorFlag oldFlags = pSector->flags;
    pSector->flags = SITHDSS_POPINT32();
    if ( (pSector->flags & SITH_SECTOR_ADJOINSOFF) != 0 && (oldFlags & SITH_SECTOR_ADJOINSOFF) == 0 )
    {
        sithSector_HideSectorAdjoins(pSector);
    }
    else if ( (oldFlags & SITH_SECTOR_ADJOINSOFF) != 0 && (pSector->flags & SITH_SECTOR_ADJOINSOFF) == 0 )
    {
        sithSector_ShowSectorAdjoins(pSector);
    }

    pSector->ambientLight.red   = SITHDSS_POPFLOAT();
    pSector->ambientLight.green = SITHDSS_POPFLOAT();
    pSector->ambientLight.blue  = SITHDSS_POPFLOAT();
    pSector->ambientLight.alpha = 0.0f;

    pSector->extraLight.red   = SITHDSS_POPFLOAT();
    pSector->extraLight.green = SITHDSS_POPFLOAT();
    pSector->extraLight.blue  = SITHDSS_POPFLOAT();
    pSector->extraLight.alpha = 0.0f;

    if ( (pSector->flags & SITH_SECTOR_USETHRUST) != 0 )
    {
        SITHDSS_POPVEC3(&pSector->thrust);
    }
    else
    {
        memset(&pSector->thrust, 0, sizeof(pSector->thrust));
    }

    SITHDSS_ENDIN;
    return 1;
}

int J3DAPI sithDSS_SectorFlags(const SithSector* pSector, DPID idTo, unsigned int outstream)
{
    SITH_ASSERTREL(pSector);

    SITHDSS_STARTOUT(SITHDSS_SECTORFLAGS);

    SITHDSS_PUSHINT16((int16_t)(sithSector_GetSectorIndex(pSector)));
    SITHDSS_PUSHINT32(pSector->flags);
    SITHDSS_ENDOUT;

    if ( (pSector->flags & SITH_SECTOR_ADJOINSOFF) != 0 )
    {
        return sithMessage_SendMessage(&sithMulti_g_message, idTo, outstream, 0);
    }

    return sithMessage_SendMessage(&sithMulti_g_message, idTo, outstream, DPSEND_GUARANTEED);
}

int J3DAPI sithDSS_ProcessSectorFlags(const SithMessage* pMsg)
{
    SITHDSS_STARTIN(pMsg);

    int sectorIdx = SITHDSS_POPUINT16();
    SithSector* pSector = sithSector_GetSectorEx(sithWorld_g_pCurrentWorld, sectorIdx);
    if ( !pSector )
    {
        SITHLOG_ERROR("Sector %d out of range for DSS:SectorStatus.\n", sectorIdx);
        SITHDSS_ENDIN;
        return 0;
    }

    SithSectorFlag oldFlags = pSector->flags;
    pSector->flags = SITHDSS_POPUINT32();
    if ( (pSector->flags & SITH_SECTOR_ADJOINSOFF) != 0 && (oldFlags & SITH_SECTOR_ADJOINSOFF) == 0 )
    {
        sithSector_HideSectorAdjoins(pSector);
    }
    else if ( (oldFlags & SITH_SECTOR_ADJOINSOFF) != 0 && (pSector->flags & SITH_SECTOR_ADJOINSOFF) == 0 )
    {
        sithSector_ShowSectorAdjoins(pSector);
    }

    SITHDSS_ENDIN;
    return 1;
}

int J3DAPI sithDSS_AIStatus(const SithAIControlBlock* pLocal, DPID idTo, signed int outstream)
{
    SITH_ASSERTREL(pLocal && pLocal->pOwner && pLocal->pClass);

    SITHDSS_STARTOUT(SITHDSS_AISTATUS);

    SITHDSS_PUSHINT16((int16_t)pLocal->pOwner->idx);
    SITHDSS_PUSHINT16((int16_t)(pLocal->pClass - sithWorld_g_pCurrentWorld->aAIClasses));
    SITHDSS_PUSHINT32(pLocal->mode);
    SITHDSS_PUSHINT32(pLocal->submode);
    SITHDSS_PUSHINT32(pLocal->msecNextUpdate);

    SITHDSS_PUSHINT16((int16_t)(sithThing_GetThingIndex(pLocal->goalThing)));
    SITHDSS_PUSHVEC3(&pLocal->vecUnknown6);
    SITHDSS_PUSHVEC3(&pLocal->vecUnknown5);
    SITHDSS_PUSHINT32(pLocal->unknown150);

    SITHDSS_PUSHINT16((int16_t)(sithThing_GetThingIndex(pLocal->pTargetThing)));
    SITHDSS_PUSHVEC3(&pLocal->targetPos);
    SITHDSS_PUSHVEC3(&pLocal->vecUnknown122);
    SITHDSS_PUSHINT32(pLocal->msecAttackStart);
    SITHDSS_PUSHFLOAT(pLocal->moveSpeed);

    if ( (pLocal->mode & SITHAI_MODE_MOVING) != 0 )
    {
        SITHDSS_PUSHVEC3(&pLocal->movePos);
    }

    if ( (pLocal->mode & SITHAI_MODE_TURNING) != 0 )
    {
        SITHDSS_PUSHVEC3(&pLocal->goalLVec);
    }

    if ( (pLocal->mode & SITHAI_MODE_FLEEING) != 0 )
    {
        SITHDSS_PUSHINT16((int16_t)(sithThing_GetThingIndex(pLocal->pFleeFromThing)));
    }

    SITHDSS_PUSHVEC3(&pLocal->homePos);
    SITHDSS_PUSHVEC3(&pLocal->homeOrient);

    for ( size_t i = 0; i < pLocal->numInstincts; ++i )
    {
        SITHDSS_PUSHINT32(pLocal->aInstinctStates[i].flags);
        SITHDSS_PUSHINT32(pLocal->aInstinctStates[i].msecNextUpdate);
        for ( size_t j = 0; j < STD_ARRAYLEN(pLocal->aInstinctStates[i].aParams); ++j )
        {
            SITHDSS_PUSHFLOAT(pLocal->aInstinctStates[i].aParams[j]);
        }
    }

    SITHDSS_PUSHINT32(pLocal->msecFireWaitTime);
    SITHDSS_PUSHINT32(pLocal->msecPauseMoveUntil);
    SITHDSS_PUSHINT32(pLocal->numFrames);
    for ( size_t i = 0; i < pLocal->numFrames; ++i )
    {
        SITHDSS_PUSHVEC3(&pLocal->aFrames[i]);
    }

    SITHDSS_PUSHINT32(pLocal->allowedSurfaceTypes);
    SITHDSS_PUSHVEC3(&pLocal->vecUnknown3);
    SITHDSS_PUSHVEC3(&pLocal->vecUnknown);
    SITHDSS_PUSHFLOAT(pLocal->maxHomeDist);

    SITHDSS_ENDOUT;
    return sithMessage_SendMessage(&sithMulti_g_message, idTo, outstream, DPSEND_GUARANTEED);
}

int J3DAPI sithDSS_ProcessAIStatus(const SithMessage* pMsg)
{
    SithWorld* pWorld = sithWorld_g_pCurrentWorld;
    SITH_ASSERTREL(pWorld && pMsg);

    SITHDSS_STARTIN(pMsg);

    size_t thingIdx = SITHDSS_POPUINT16();
    SithThing* pThing = sithThing_GetThingByIndex(thingIdx);
    if ( !pThing )
    {
        SITHLOG_ERROR("DSS::AiStatus - Bad thing index %d.\n", thingIdx);
        SITHDSS_ENDIN;
        return 0;
    }

    if ( pThing->controlType != SITH_CT_AI || !pThing->controlInfo.aiControl.pLocal )
    {
        SITHLOG_ERROR("DSS::AiStatus - Non-AI thing index %d.\n", thingIdx);
        SITHDSS_ENDIN;
        return 0;
    }

    SithAIControlBlock* pLocal = pThing->controlInfo.aiControl.pLocal;
    size_t classIdx = SITHDSS_POPUINT16();
    if ( classIdx >= pWorld->numAIClasses )
    {
        SITHDSS_ENDIN;
        return 0;
    }

    pLocal->pClass         = &pWorld->aAIClasses[classIdx];
    pLocal->numInstincts   = pLocal->pClass->numInstincts;
    pLocal->mode           = SITHDSS_POPINT32();
    pLocal->submode        = SITHDSS_POPINT32();
    pLocal->msecNextUpdate = SITHDSS_POPINT32();

    thingIdx = SITHDSS_POPUINT16();
    pLocal->goalThing = sithThing_GetThingByIndex(thingIdx);
    SITHDSS_POPVEC3(&pLocal->vecUnknown6);
    SITHDSS_POPVEC3(&pLocal->vecUnknown5);
    pLocal->unknown150 = SITHDSS_POPINT32();
    pLocal->unknown141 = 0;

    thingIdx = SITHDSS_POPUINT16();
    pLocal->pTargetThing = sithThing_GetThingByIndex(thingIdx);
    SITHDSS_POPVEC3(&pLocal->targetPos);
    SITHDSS_POPVEC3(&pLocal->vecUnknown122);
    pLocal->msecAttackStart = SITHDSS_POPINT32();
    pLocal->unknown124      = 0;
    pLocal->moveSpeed       = SITHDSS_POPFLOAT();

    if ( (pLocal->mode & SITHAI_MODE_MOVING) != 0 )
    {
        SITHDSS_POPVEC3(&pLocal->movePos);
    }

    if ( (pLocal->mode & SITHAI_MODE_TURNING) != 0 )
    {
        SITHDSS_POPVEC3(&pLocal->goalLVec);
    }

    if ( (pLocal->mode & SITHAI_MODE_FLEEING) != 0 )
    {
        thingIdx = SITHDSS_POPUINT16();
        pLocal->pFleeFromThing = sithThing_GetThingByIndex(thingIdx);
    }

    SITHDSS_POPVEC3(&pLocal->homePos);
    SITHDSS_POPVEC3(&pLocal->homeOrient);

    for ( size_t i = 0; i < pLocal->numInstincts; ++i )
    {
        pLocal->aInstinctStates[i].flags          = SITHDSS_POPINT32();
        pLocal->aInstinctStates[i].msecNextUpdate = SITHDSS_POPINT32();
        for ( size_t j = 0; j < STD_ARRAYLEN(pLocal->aInstinctStates[i].aParams); ++j )
        {
            pLocal->aInstinctStates[i].aParams[j] = SITHDSS_POPFLOAT();
        }
    }

    pLocal->msecFireWaitTime   = SITHDSS_POPINT32();
    pLocal->msecPauseMoveUntil = SITHDSS_POPINT32();
    pLocal->numFrames          = SITHDSS_POPINT32();

    // TODO: [BUG] Previously allocated aFrames is not freed, it leaks
    if ( pLocal->numFrames && (pLocal->aFrames = (rdVector3*)STDMALLOC(sizeof(rdVector3) * pLocal->numFrames)) != NULL )
    {
        pLocal->sizeFrames = pLocal->numFrames;
        for ( size_t i = 0; i < pLocal->numFrames; ++i )
        {
            SITHDSS_POPVEC3(&pLocal->aFrames[i]);
        }
    }
    else
    {
        pLocal->sizeFrames = 0;
        pLocal->numFrames  = 0;
    }

    pLocal->allowedSurfaceTypes = SITHDSS_POPINT32();
    SITHDSS_POPVEC3(&pLocal->vecUnknown3);
    SITHDSS_POPVEC3(&pLocal->vecUnknown);
    pLocal->maxHomeDist = SITHDSS_POPFLOAT();

    SITHDSS_ENDIN;
    return 1;
}

int J3DAPI sithDSS_Inventory(const SithThing* pThing, unsigned int inventoryId, DPID idTo, unsigned int outstream)
{
    SITH_ASSERTREL(pThing);
    SITH_ASSERTREL(inventoryId < SITHINVENTORY_MAXTYPES);

    if ( pThing->type != SITH_THING_PLAYER && pThing->type != SITH_THING_ACTOR )
    {
        return 0;
    }

    if ( !pThing->thingInfo.actorInfo.pPlayer )
    {
        return 0;
    }

    const SithInventoryItem* pCurItems = pThing->thingInfo.actorInfo.pPlayer->aItems;

    SITHDSS_STARTOUT(SITHDSS_INVENTORY);

    SITHDSS_PUSHINT16((int16_t)(sithThing_GetThingIndex(pThing)));
    SITHDSS_PUSHINT16((int16_t)inventoryId);
    SITHDSS_PUSHFLOAT(pCurItems[inventoryId].amount);
    SITHDSS_PUSHINT32(pCurItems[inventoryId].status);
    SITHDSS_ENDOUT;
    return sithMessage_SendMessage(&sithMulti_g_message, idTo, outstream, DPSEND_GUARANTEED);
}

int J3DAPI sithDSS_ProcessInventory(const SithMessage* pMsg)
{
    SITH_ASSERTREL(pMsg);

    SITHDSS_STARTIN(pMsg);

    int n = SITHDSS_POPUINT16();
    SithThing* pThing = sithThing_GetThingByIndex(n);
    if ( !pThing
        || (pThing->type != SITH_THING_ACTOR && pThing->type != SITH_THING_PLAYER)
        || !pThing->thingInfo.actorInfo.pPlayer )
    {
        goto error;
    }

    SithInventoryItem* pCurItems = pThing->thingInfo.actorInfo.pPlayer->aItems;
    n = SITHDSS_POPUINT16();
    if ( n < 0 || n >= (int)SITHINVENTORY_MAXTYPES )
    {
        goto error;
    }

    pCurItems[n].amount = SITHDSS_POPFLOAT();
    pCurItems[n].status = SITHDSS_POPINT32();

    SITHDSS_ENDIN;
    return 1;

error:
    SITHLOG_ERROR("Error processing DSS::Inventory message.\n");
    SITHDSS_ENDIN;
    return 0;
}

int J3DAPI sithDSS_AnimStatus(const SithAnimationSlot* pAnim, DPID idTo, unsigned int outstream)
{
    SITH_ASSERTREL(pAnim);

    SITHDSS_STARTOUT(SITHDSS_ANIMSTATUS);

    SITHDSS_PUSHINT32(pAnim->animID);
    SITHDSS_PUSHINT32(pAnim->flags);

    if ( (pAnim->flags & (SITHANIMATE_THING | SITHANIMATE_SPRITE)) != 0 )
    {
        SITHDSS_PUSHINT16((int16_t)(sithThing_GetThingIndex(pAnim->pThing)));
        SITHDSS_PUSHINT32(pAnim->thingSignature);
    }

    if ( (pAnim->flags & SITHANIMATE_CAMERA_ZOOM) != 0 )
    {
        SITHDSS_PUSHFLOAT(pAnim->deltaValue);
        SITHDSS_PUSHFLOAT(pAnim->endValue);
        SITHDSS_PUSHFLOAT(pAnim->curValue);
    }

    if ( (pAnim->flags & SITHANIMATE_THING_MOVE) != 0 )
    {
        SITHDSS_PUSHINT32(pAnim->thingSignature);
        SITHDSS_PUSHFLOAT(pAnim->deltaValue);
        SITHDSS_PUSHFLOAT(pAnim->endValue);
        SITHDSS_PUSHFLOAT(pAnim->curValue);
    }

    if ( (pAnim->flags & SITHANIMATE_THING_MOVEPOS) != 0 )
    {
        SITHDSS_PUSHINT32(pAnim->thingSignature);
        SITHDSS_PUSHFLOAT(pAnim->secTimeRemaining);
        SITHDSS_PUSHVEC3(&pAnim->thingEndPosition);
    }

    if ( (pAnim->flags & (SITHANIMATE_PUSHITEM | SITHANIMATE_PULLITEM)) != 0 )
    {
        SITHDSS_PUSHINT16((int16_t)(sithThing_GetThingIndex(pAnim->pItemThing)));
        SITHDSS_PUSHINT32(pAnim->thingSignature);
        SITHDSS_PUSHFLOAT(pAnim->deltaValue);
        SITHDSS_PUSHFLOAT(pAnim->endValue);
        SITHDSS_PUSHFLOAT(pAnim->curValue);
        SITHDSS_PUSHINT32(pAnim->curFrame);
        SITHDSS_PUSHVEC4(&pAnim->deltaVector);
        SITHDSS_PUSHVEC4(&pAnim->startVector);
        SITHDSS_PUSHVEC4(&pAnim->endVector);
        SITHDSS_PUSHINT32(pAnim->trackNum);
        SITHDSS_PUSHINT32(pAnim->msecPerFrame);
        SITHDSS_PUSHINT32(sithSurface_GetSurfaceIndex(pAnim->pSurface));
        SITHDSS_PUSHVEC3(&pAnim->direction3);
    }

    if ( (pAnim->flags & SITHANIMATE_SPRITE_SIZE) != 0 )
    {
        SITHDSS_PUSHVEC4(&pAnim->startVector);
        SITHDSS_PUSHVEC4(&pAnim->curVector);
        SITHDSS_PUSHVEC4(&pAnim->deltaVector);
        SITHDSS_PUSHVEC4(&pAnim->endVector);
    }

    if ( (pAnim->flags & SITHANIMATE_THING_QUICKTURN) != 0 )
    {
        SITHDSS_PUSHFLOAT(pAnim->curValue);
        SITHDSS_PUSHFLOAT(pAnim->endValue);
        SITHDSS_PUSHFLOAT(pAnim->deltaValue);
    }

    if ( (pAnim->flags & SITHANIMATE_SURFACE) != 0 )
    {
        SITH_ASSERTREL(pAnim->pSurface);
        SITHDSS_PUSHINT32(sithSurface_GetSurfaceIndex(pAnim->pSurface));
    }

    if ( (pAnim->flags & SITHANIMATE_SCROLL) != 0 )
    {
        SITHDSS_PUSHVEC3(&pAnim->direction3);
        SITHDSS_PUSHFLOAT(pAnim->direction2.x);
        SITHDSS_PUSHFLOAT(pAnim->direction2.y);
    }

    if ( (pAnim->flags & SITHANIMATE_PAGEFLIP) != 0 )
    {
        SITHDSS_PUSHINT32(pAnim->msecNextFrameTime);
        SITHDSS_PUSHINT32(pAnim->msecPerFrame);
        SITHDSS_PUSHINT32(pAnim->curFrame);
    }

    if ( (pAnim->flags & SITHANIMATE_LIGHT) != 0 )
    {
        SITHDSS_PUSHVEC4(&pAnim->deltaVector);
        SITHDSS_PUSHVEC4(&pAnim->startVector);
        SITHDSS_PUSHVEC4(&pAnim->endVector);
        SITHDSS_PUSHVEC4(&pAnim->curVector);
    }

    if ( (pAnim->flags & SITHANIMATE_MATERIAL) != 0 )
    {
        SITHDSS_PUSHINT32(pAnim->pMaterial->num);
    }

    if ( (pAnim->flags & SITHANIMATE_SECTOR) != 0 )
    {
        SITHDSS_PUSHINT32(sithSector_GetSectorIndex(pAnim->pSector));
    }

    if ( (pAnim->flags & SITHANIMATE_THING_FADE) != 0 )
    {
        SITHDSS_PUSHINT32(sithThing_GetThingIndex(pAnim->pThing));
        SITHDSS_PUSHINT32(pAnim->thingSignature);
        SITHDSS_PUSHVEC4(&pAnim->startVector);
        SITHDSS_PUSHVEC4(&pAnim->curVector);
        SITHDSS_PUSHVEC4(&pAnim->deltaVector);
        SITHDSS_PUSHVEC4(&pAnim->endVector);
    }

    SITHDSS_ENDOUT;
    return sithMessage_SendMessage(&sithMulti_g_message, idTo, outstream, DPSEND_GUARANTEED);
}

int J3DAPI sithDSS_ProcessAnimStatus(const SithMessage* pMsg)
{
    SITHDSS_STARTIN(pMsg);

    int animIdx = SITHDSS_POPINT32();
    SithAnimationSlot* pAnim = sithAnimate_GetAnim(animIdx);
    if ( !pAnim )
    {
        pAnim = sithAnimate_Create();
    }

    if ( !pAnim )
    {
        SITHDSS_ENDIN;
        return 0;
    }

    pAnim->flags = SITHDSS_POPINT32();
    if ( !pAnim->flags )
    {
        sithAnimate_Stop(pAnim);
        SITHDSS_ENDIN;
        return 1;
    }

    if ( (pAnim->flags & (SITHANIMATE_THING | SITHANIMATE_SPRITE)) != 0 )
    {
        int16_t idx16 = SITHDSS_POPINT16();
        pAnim->pThing = sithThing_GetThingByIndex(idx16);
        if ( pAnim->pThing && pAnim->pThing->renderData.type == RD_THING_SPRITE3 )
        {
            pAnim->pMaterial = pAnim->pThing->renderData.data.pSprite3->face.pMaterial;
        }

        pAnim->thingSignature = SITHDSS_POPINT32();
    }

    if ( (pAnim->flags & SITHANIMATE_CAMERA_ZOOM) != 0 )
    {
        pAnim->deltaValue = SITHDSS_POPFLOAT();
        pAnim->endValue   = SITHDSS_POPFLOAT();
        pAnim->curValue   = SITHDSS_POPFLOAT();
    }

    if ( (pAnim->flags & SITHANIMATE_THING_MOVE) != 0 )
    {
        pAnim->thingSignature = SITHDSS_POPINT32();
        pAnim->deltaValue     = SITHDSS_POPFLOAT();
        pAnim->endValue       = SITHDSS_POPFLOAT();
        pAnim->curValue       = SITHDSS_POPFLOAT();
    }

    if ( (pAnim->flags & SITHANIMATE_THING_MOVEPOS) != 0 )
    {
        pAnim->thingSignature   = SITHDSS_POPINT32();
        pAnim->secTimeRemaining = SITHDSS_POPFLOAT();
        SITHDSS_POPVEC3(&pAnim->thingEndPosition);
    }

    if ( (pAnim->flags & (SITHANIMATE_PUSHITEM | SITHANIMATE_PULLITEM)) != 0 )
    {
        int16_t idx16 = SITHDSS_POPINT16();
        pAnim->pItemThing     = sithThing_GetThingByIndex(idx16);
        pAnim->thingSignature = SITHDSS_POPINT32();
        pAnim->deltaValue     = SITHDSS_POPFLOAT();
        pAnim->endValue       = SITHDSS_POPFLOAT();
        pAnim->curValue       = SITHDSS_POPFLOAT();
        pAnim->curFrame       = SITHDSS_POPINT32();
        SITHDSS_POPVEC4(&pAnim->deltaVector);
        SITHDSS_POPVEC4(&pAnim->startVector);
        SITHDSS_POPVEC4(&pAnim->endVector);
        pAnim->trackNum     = SITHDSS_POPINT32();
        pAnim->msecPerFrame = SITHDSS_POPINT32();
        animIdx = SITHDSS_POPINT32();
        pAnim->pSurface     = sithSurface_GetSurfaceEx(sithWorld_g_pCurrentWorld, animIdx);
        SITHDSS_POPVEC3(&pAnim->direction3);
    }

    if ( (pAnim->flags & SITHANIMATE_SPRITE_SIZE) != 0 )
    {
        SITHDSS_POPVEC4(&pAnim->startVector);
        SITHDSS_POPVEC4(&pAnim->curVector);
        SITHDSS_POPVEC4(&pAnim->deltaVector);
        SITHDSS_POPVEC4(&pAnim->endVector);
    }

    if ( (pAnim->flags & SITHANIMATE_THING_QUICKTURN) != 0 )
    {
        pAnim->curValue   = SITHDSS_POPFLOAT();
        pAnim->endValue   = SITHDSS_POPFLOAT();
        pAnim->deltaValue = SITHDSS_POPFLOAT();
    }

    if ( (pAnim->flags & SITHANIMATE_SURFACE) != 0 )
    {
        animIdx = SITHDSS_POPINT32();
        pAnim->pSurface = sithSurface_GetSurfaceEx(sithWorld_g_pCurrentWorld, animIdx);
        if ( pAnim->pSurface )
        {
            pAnim->pMaterial = pAnim->pSurface->face.pMaterial;
        }
    }

    if ( (pAnim->flags & SITHANIMATE_SCROLL) != 0 )
    {
        SITHDSS_POPVEC3(&pAnim->direction3);
        pAnim->direction2.x = SITHDSS_POPFLOAT();
        pAnim->direction2.y = SITHDSS_POPFLOAT();
    }

    if ( (pAnim->flags & SITHANIMATE_PAGEFLIP) != 0 )
    {
        pAnim->msecNextFrameTime = SITHDSS_POPINT32();
        pAnim->msecPerFrame      = SITHDSS_POPINT32();
        pAnim->curFrame          = SITHDSS_POPINT32();
    }

    if ( (pAnim->flags & SITHANIMATE_LIGHT) != 0 )
    {
        SITHDSS_POPVEC4(&pAnim->deltaVector);
        SITHDSS_POPVEC4(&pAnim->startVector);
        SITHDSS_POPVEC4(&pAnim->endVector);
        SITHDSS_POPVEC4(&pAnim->curVector);
    }

    if ( (pAnim->flags & SITHANIMATE_MATERIAL) != 0 )
    {
        animIdx = SITHDSS_POPINT32();
        pAnim->pMaterial = sithMaterial_GetMaterialByIndex(animIdx);
    }

    if ( (pAnim->flags & SITHANIMATE_SECTOR) != 0 )
    {
        animIdx = SITHDSS_POPINT32();
        pAnim->pSector = sithSector_GetSectorEx(sithWorld_g_pCurrentWorld, animIdx);
    }

    if ( (pAnim->flags & SITHANIMATE_THING_FADE) != 0 )
    {
        animIdx = SITHDSS_POPINT32();
        pAnim->pThing         = sithThing_GetThingByIndex(animIdx);
        pAnim->thingSignature = SITHDSS_POPINT32();
        SITHDSS_POPVEC4(&pAnim->startVector);
        SITHDSS_POPVEC4(&pAnim->curVector);
        SITHDSS_POPVEC4(&pAnim->deltaVector);
        SITHDSS_POPVEC4(&pAnim->endVector);
    }

    SITHDSS_ENDIN;
    return 1;
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
    SITHDSS_STARTOUT(SITHDSS_SYNCTASKEVENTS);

    SITHDSS_PUSHINT32(pEvent->msecEventTime - sithTime_g_msecGameTime);
    SITHDSS_PUSHINT32(pEvent->params.idx);
    SITHDSS_PUSHINT32(pEvent->params.param1);
    SITHDSS_PUSHINT32(pEvent->params.param2);
    SITHDSS_PUSHINT32(pEvent->params.param3);
    SITHDSS_PUSHUINT16((uint16_t)pEvent->taskNum);

    SITHDSS_ENDOUT;
    return sithMessage_SendMessage(&sithMulti_g_message, idTo, outstream, DPSEND_GUARANTEED);
}

int J3DAPI sithDSS_ProcessSyncTaskEvents(const SithMessage* pMsg)
{
    SITH_ASSERTREL(pMsg);

    SITHDSS_STARTIN(pMsg);

    uint32_t msecDelay = SITHDSS_POPINT32();

    SithEventParams params;
    params.idx    = SITHDSS_POPINT32();
    params.param1 = SITHDSS_POPINT32();
    params.param2 = SITHDSS_POPINT32();
    params.param3 = SITHDSS_POPINT32();

    size_t taskNum = SITHDSS_POPUINT16();
    sithEvent_CreateEvent(taskNum, &params, msecDelay);

    SITHDSS_ENDIN;
    return 1;
}

int J3DAPI sithDSS_SyncCameras(DPID idTo, unsigned int outstream)
{
    SITHDSS_STARTOUT(SITHDSS_SYNCCAMERAS);

    SITHDSS_PUSHINT16((int16_t)(sithCamera_g_pCurCamera - sithCamera_g_aCameras));
    SITHDSS_PUSHINT32(sithCamera_g_bCurCameraSet);
    SITHDSS_PUSHINT32(sithCamera_g_curCycleCamNum);
    SITHDSS_PUSHVEC3(&sithCamera_g_vecCameraPosOffset);
    SITHDSS_PUSHVEC3(&sithCamera_g_vecCameraAngleOffset);
    SITHDSS_PUSHFLOAT(sithCamera_g_cameraPosDelta);
    SITHDSS_PUSHFLOAT(sithCamera_g_cameraAngleDelta);

    for ( size_t i = 0; i < STD_ARRAYLEN(sithCamera_g_aCameras); ++i )
    {
        const SithCamera* pCamera = &sithCamera_g_aCameras[i];
        SITHDSS_PUSHINT32(sithThing_GetThingIndex(pCamera->pPrimaryFocusThing));
        SITHDSS_PUSHINT32(sithThing_GetThingIndex(pCamera->pSecondaryFocusThing));
        SITHDSS_PUSHFLOAT(pCamera->fov);
    }

    SITH_ASSERTREL(sithPlayer_g_pLocalPlayer);

    SITHDSS_ENDOUT;
    return sithMessage_SendMessage(&sithMulti_g_message, idTo, outstream, DPSEND_GUARANTEED);
}

int J3DAPI sithDSS_ProcessSyncCameras(const SithMessage* pMsg)
{
    SITH_ASSERTREL(pMsg);

    SITHDSS_STARTIN(pMsg);

    size_t cameraNum = SITHDSS_POPUINT16();
    cameraNum = STDMATH_CLAMP(cameraNum, 0, 6);
    sithCamera_g_pCurCamera       = &sithCamera_g_aCameras[cameraNum];
    sithCamera_g_bCurCameraSet    = SITHDSS_POPINT32();
    sithCamera_g_curCycleCamNum   = SITHDSS_POPINT32();
    SITHDSS_POPVEC3(&sithCamera_g_vecCameraPosOffset);
    SITHDSS_POPVEC3(&sithCamera_g_vecCameraAngleOffset);
    sithCamera_g_cameraPosDelta   = SITHDSS_POPFLOAT();
    sithCamera_g_cameraAngleDelta = SITHDSS_POPFLOAT();

    for ( size_t i = 0; i < STD_ARRAYLEN(sithCamera_g_aCameras); ++i )
    {
        SithCamera* pCamera = &sithCamera_g_aCameras[i];
        pCamera->pSector = NULL;
        pCamera->pPrimaryFocusThing   = sithThing_GetThingByIndex(SITHDSS_POPINT32());
        pCamera->pSecondaryFocusThing = sithThing_GetThingByIndex(SITHDSS_POPINT32());
        pCamera->fov = SITHDSS_POPFLOAT();
    }

    SITHDSS_ENDIN;
    return 1;
}

int J3DAPI sithDSS_SyncGameState(DPID idTo, unsigned int outstream)
{
    SITH_ASSERTREL(sithWorld_g_pStaticWorld && sithWorld_g_pCurrentWorld);

    SITHDSS_STARTOUT(SITHDSS_SYNCGAMESTATE);

    if ( sithCog_g_pMasterCog )
    {
        SITHDSS_PUSHINT32(sithCog_g_pMasterCog->idx);
    }
    else
    {
        SITHDSS_PUSHINT32(-1);
    }

    SITH_ASSERTREL(sithPlayer_g_pLocalPlayer);
    SITHDSS_PUSHINT32(sithPlayer_g_pLocalPlayer->curItemID);
    SITHDSS_PUSHINT32(sithPlayer_g_pLocalPlayer->curWeaponID);

    for ( int n = 0; n < (int)STD_ARRAYLEN(sithInventory_g_aPowerKeybinds); ++n )
    {
        SITHDSS_PUSHINT32(sithInventory_g_aPowerKeybinds[n].bActivated);
    }

    SITHDSS_PUSHINT32(sithInventory_g_bSendDeactivateMessage);
    SITHDSS_PUSHINT32(sithInventory_g_bInitInventory);
    SITHDSS_PUSHINT32(sithInventory_g_dword_56B750);
    SITHDSS_PUSHINT32(sithInventory_g_dword_56B754);

    for ( int n = 2; n < 11; ++n )
    {
        const SithInventoryType* pType = &sithInventory_g_aTypes[n];
        if ( pType->pHolsterModel )
        {
            SITHDSS_PUSHINT16((int16_t)(sithModel_GetModelIndex(pType->pHolsterModel)));
        }
        else
        {
            SITHDSS_PUSHINT16(-1);
        }
    }

    SITHDSS_PUSHINT32(sithRender_bResetCameraAspect);
    SITHDSS_PUSHINT32(sithThing_GetThingIndex(sithPlayerControls_GetVehicleBoardedThing()));

    // Note: Cur collided surface, model and model face are named as cur ledge surface, ledge thing model and model face
    SITHDSS_PUSHINT16((int16_t)(sithSurface_GetSurfaceIndex(sithPlayerActions_g_pCurCollidedSurface)));
    SITHDSS_PUSHINT16((int16_t)(sithModel_GetModelIndex(sithPlayerActions_g_pCurCollidedModel)));
    if ( sithPlayerActions_g_pCurCollidedModel )
    {
        SITHDSS_PUSHINT16((int16_t)(sithPlayerActions_g_pCurCollidedModelFace - sithPlayerActions_g_pCurCollidedModel->aGeos[0].aMeshes->aFaces));
    }

    SITHDSS_PUSHINT32(sithPlayer_g_bPlayerInPor);
    SITHDSS_PUSHFLOAT(sithPlayer_g_impState);
    SITHDSS_PUSHUINT8((uint8_t)sithPlayer_g_bInAetheriumSector);
    SITHDSS_PUSHUINT8((uint8_t)sithPlayer_g_bGuybrush);
    SITHDSS_PUSHINT16((int16_t)sithPlayer_g_impFireType);
    SITHDSS_PUSHINT16((int16_t)sithPlayer_g_curLevelNum);
    SITHDSS_PUSHUINT8((uint8_t)sithPlayer_g_bBonusMapBought);
    SITHDSS_PUSHINT32(sithThing_GetThingIndex(sithPlayerActions_g_pPlasma));
    SITHDSS_PUSHINT16((int16_t)sithPlayerActions_g_jewelFlyingPuppetTrackNum);
    SITHDSS_PUSHUINT8((uint8_t)sithPlayerActions_g_bJewelFlying);
    SITHDSS_PUSHUINT8((uint8_t)sithPlayerActions_g_bPlayerInvisible);
    SITHDSS_PUSHUINT8((uint8_t)sithPlayerControls_g_bCutsceneMode);
    SITHDSS_PUSHUINT8((uint8_t)sithPuppet_g_bPlayerLeapForward);

    SITHDSS_PUSHINT32(sithFX_g_numChalkMarks);
    SITHDSS_PUSHINT32(sithFX_g_lastChalkMarkNum);
    for ( int n = 0; n < (int)sithFX_g_numChalkMarks; ++n )
    {
        SITHDSS_PUSHINT32(sithThing_GetThingIndex(sithFX_g_aChalkMarks[n]));
    }

    const SithWorld* pWorld = sithWorld_g_pStaticWorld;
    for ( int n = 0; n < (int)pWorld->numMaterials; ++n )
    {
        if ( pWorld->aMaterials[n].numCels > 1 )
        {
            SITHDSS_PUSHUINT8((uint8_t)(pWorld->aMaterials[n].curCelNum));
        }
    }

    pWorld = sithWorld_g_pCurrentWorld;
    for ( int n = 0; n < (int)pWorld->numMaterials; ++n )
    {
        if ( pWorld->aMaterials[n].numCels > 1 )
        {
            SITHDSS_PUSHUINT8((uint8_t)(pWorld->aMaterials[n].curCelNum));
        }
    }

    SITHDSS_ENDOUT;
    return sithMessage_SendMessage(&sithMulti_g_message, idTo, outstream, DPSEND_GUARANTEED);
}

int J3DAPI sithDSS_ProcessSyncGameState(const SithMessage* pMsg)
{
    SITH_ASSERTREL(pMsg && sithWorld_g_pStaticWorld && sithWorld_g_pCurrentWorld);

    SITHDSS_STARTIN(pMsg);

    int cogIdx = SITHDSS_POPINT32();
    sithCog_g_pMasterCog = sithCog_GetCogByIndex(cogIdx);

    SITH_ASSERTREL(sithPlayer_g_pLocalPlayer);
    sithPlayer_g_pLocalPlayer->curItemID   = SITHDSS_POPINT32();
    sithPlayer_g_pLocalPlayer->curWeaponID = SITHDSS_POPINT32();

    for ( int i = 0; i < (int)STD_ARRAYLEN(sithInventory_g_aPowerKeybinds); ++i )
    {
        sithInventory_g_aPowerKeybinds[i].bActivated = SITHDSS_POPINT32();
    }

    sithInventory_g_bSendDeactivateMessage = SITHDSS_POPINT32();
    sithInventory_g_bInitInventory         = SITHDSS_POPINT32();
    sithInventory_g_dword_56B750           = SITHDSS_POPINT32();
    sithInventory_g_dword_56B754           = SITHDSS_POPINT32();

    for ( int i = 2; i < 11; ++i )
    {
        SithInventoryType* pInventoryType = &sithInventory_g_aTypes[i];
        int16_t idx16 = SITHDSS_POPINT16();
        if ( idx16 != -1 )
        {
            pInventoryType->pHolsterModel = sithModel_GetModelByIndex(idx16);
        }
        else
        {
            pInventoryType->pHolsterModel = NULL;
        }
    }

    sithRender_bResetCameraAspect = SITHDSS_POPINT32();
    cogIdx = SITHDSS_POPINT32();
    sithPlayerControls_SetVehicleBoardedThing(sithThing_GetThingByIndex(cogIdx));

    // Note: Cur collided surface, model and model face are named as cur ledge surface, ledge thing model and model face
    int16_t idx16 = SITHDSS_POPINT16();
    sithPlayerActions_g_pCurCollidedSurface = sithSurface_GetSurfaceEx(sithWorld_g_pCurrentWorld, idx16);
    idx16 = SITHDSS_POPINT16();
    sithPlayerActions_g_pCurCollidedModel = sithModel_GetModelByIndex(idx16);
    if ( sithPlayerActions_g_pCurCollidedModel )
    {
        idx16 = SITHDSS_POPINT16();
        sithPlayerActions_g_pCurCollidedModelFace = &sithPlayerActions_g_pCurCollidedModel->aGeos[0].aMeshes->aFaces[idx16];
    }

    sithPlayer_g_bPlayerInPor       = SITHDSS_POPINT32();
    sithPlayer_g_impState           = SITHDSS_POPFLOAT();
    sithPlayer_g_bInAetheriumSector = SITHDSS_POPUINT8();
    sithPlayer_g_bGuybrush          = SITHDSS_POPUINT8();
    sithPlayer_g_impFireType        = SITHDSS_POPUINT16();
    sithPlayer_g_curLevelNum        = SITHDSS_POPUINT16();
    sithPlayer_g_bBonusMapBought    = SITHDSS_POPUINT8();
    cogIdx = SITHDSS_POPINT32();
    sithPlayerActions_g_pPlasma     = sithThing_GetThingByIndex(cogIdx);
    sithPlayerActions_g_jewelFlyingPuppetTrackNum = SITHDSS_POPUINT16();
    sithPlayerActions_g_bJewelFlying     = SITHDSS_POPUINT8();
    sithPlayerActions_g_bPlayerInvisible = SITHDSS_POPUINT8();
    sithPlayerControls_g_bCutsceneMode   = SITHDSS_POPUINT8();
    sithPuppet_g_bPlayerLeapForward      = SITHDSS_POPUINT8();

    sithFX_g_numChalkMarks    = SITHDSS_POPINT32();
    sithFX_g_lastChalkMarkNum = SITHDSS_POPINT32();

    // TODO: [BUG] Number of chalk marks read from the stream is not checked against the size of aChalkMarks
    for ( int i = 0; i < (int)sithFX_g_numChalkMarks; ++i )
    {
        cogIdx = SITHDSS_POPINT32();
        if ( cogIdx >= 0 && cogIdx < (int)sithWorld_g_pCurrentWorld->numThings )
        {
            sithFX_g_aChalkMarks[i] = &sithWorld_g_pCurrentWorld->aThings[cogIdx];
        }
    }

    SithWorld* pWorld = sithWorld_g_pStaticWorld;
    for ( int i = 0; i < (int)pWorld->numMaterials; ++i )
    {
        if ( pWorld->aMaterials[i].numCels > 1 )
        {
            pWorld->aMaterials[i].curCelNum = SITHDSS_POPUINT8();
        }
    }

    pWorld = sithWorld_g_pCurrentWorld;
    for ( int i = 0; i < (int)pWorld->numMaterials; ++i )
    {
        if ( pWorld->aMaterials[i].numCels > 1 )
        {
            pWorld->aMaterials[i].curCelNum = SITHDSS_POPUINT8();
        }
    }

    SITHDSS_ENDIN;
    return 1;
}

int J3DAPI sithDSS_SyncVehicleControlState(DPID idTo, unsigned int outstream)
{
    const SithMineCarControlsState* pMineCarState = sithVehicleControls_GetMineCarState();
    const SithRaftControlsState* pRaftState       = sithVehicleControls_GetRaftState();

    SITHDSS_STARTOUT(SITHDSS_VEHICLECONTROLSTATE);

    SITHDSS_PUSHFLOAT(pMineCarState->secElapsedStoppingTime);
    SITHDSS_PUSHINT32(pMineCarState->moveState);
    SITHDSS_PUSHFLOAT(pMineCarState->secDuckTime);
    SITHDSS_PUSHINT32(pMineCarState->curDuckPuppetTrackNum);
    SITHDSS_PUSHINT32(pMineCarState->bCanDuck);
    SITHDSS_PUSHINT32(pMineCarState->leanState);
    SITHDSS_PUSHINT32(pMineCarState->curPuppetTrack);
    SITHDSS_PUSHINT32(pMineCarState->hBrakeSnd);
    SITHDSS_PUSHFLOAT(pMineCarState->secElapsedBrakingTime);
    SITHDSS_PUSHFLOAT(pMineCarState->secUnboardingElapsedTime);
    SITHDSS_PUSHINT32(pMineCarState->unboardState);

    SITHDSS_PUSHINT32(pRaftState->nextMoveStatus);
    SITHDSS_PUSHINT32(pRaftState->nextPuppetMode);
    SITHDSS_PUSHVEC3(&pRaftState->unboardPos);
    SITHDSS_PUSHVEC3(&pRaftState->unboardNorm);
    SITHDSS_PUSHFLOAT(pRaftState->moveSize);
    SITHDSS_PUSHFLOAT(pRaftState->wakeTimer);
    SITHDSS_PUSHINT32(pRaftState->bRowing);
    SITHDSS_PUSHFLOAT(pRaftState->secRowStartTime);
    SITHDSS_PUSHFLOAT(pRaftState->secUnboardTime);

    SITHDSS_ENDOUT;
    return sithMessage_SendMessage(&sithMulti_g_message, idTo, outstream, DPSEND_GUARANTEED);
}

int J3DAPI sithDSS_ProcessVehicleControlsState(const SithMessage* pMsg)
{
    SITH_ASSERTREL(pMsg);

    SITHDSS_STARTIN(pMsg);

    SithMineCarControlsState mineCarState;
    mineCarState.secElapsedStoppingTime  = SITHDSS_POPFLOAT();
    mineCarState.moveState               = SITHDSS_POPINT32();
    mineCarState.secDuckTime             = SITHDSS_POPFLOAT();
    mineCarState.curDuckPuppetTrackNum   = SITHDSS_POPINT32();
    mineCarState.bCanDuck                = SITHDSS_POPINT32();
    mineCarState.leanState               = SITHDSS_POPINT32();
    mineCarState.curPuppetTrack          = SITHDSS_POPINT32();
    mineCarState.hBrakeSnd               = SITHDSS_POPINT32();
    mineCarState.secElapsedBrakingTime   = SITHDSS_POPFLOAT();
    mineCarState.secUnboardingElapsedTime = SITHDSS_POPFLOAT();
    mineCarState.unboardState            = SITHDSS_POPINT32();

    SithRaftControlsState raftState;
    raftState.nextMoveStatus = SITHDSS_POPINT32();
    raftState.nextPuppetMode = SITHDSS_POPINT32();
    SITHDSS_POPVEC3(&raftState.unboardPos);
    SITHDSS_POPVEC3(&raftState.unboardNorm);
    raftState.moveSize        = SITHDSS_POPFLOAT();
    raftState.wakeTimer       = SITHDSS_POPFLOAT();
    raftState.bRowing         = SITHDSS_POPINT32();
    raftState.secRowStartTime = SITHDSS_POPFLOAT();
    raftState.secUnboardTime  = SITHDSS_POPFLOAT();

    sithVehicleControls_SetMineCarState(&mineCarState);
    sithVehicleControls_SetRaftState(&raftState);

    SITHDSS_ENDIN;
    return 1;
}

int J3DAPI sithDSS_SendEndSection(DPID idTo, unsigned int outstream)
{
    SITHDSS_STARTOUT(SITHDSS_ENDSECTION);
    SITHDSS_ENDOUT;
    return sithMessage_SendMessage(&sithMulti_g_message, idTo, outstream, DPSEND_GUARANTEED);
}

int J3DAPI sithDSS_ProcessEndSection(const SithMessage* pMsg)
{
    SITH_ASSERTREL(pMsg);
    return 1;
}

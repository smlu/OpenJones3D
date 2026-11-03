#include "sithAIAwareness.h"
#include <j3dcore/j3dhook.h>

#include <rdroid/Math/rdMath.h>
#include <rdroid/Math/rdVector.h>

#include <sith/AI/sithAI.h>
#include <sith/Engine/sithIntersect.h>
#include <sith/Gameplay/sithEvent.h>
#include <sith/RTI/symbols.h>
#include <sith/World/sithSector.h>
#include <sith/World/sithWorld.h>

#include <std/General/stdMemory.h>
#include <std/General/stdMath.h>
#include <std/General/stdUtil.h>

#include <float.h>

// Added: Standalone builds have no retail executable to hold these. *_ResetGlobals sets the starting values.
SithAIAwarenessSector* sithAIAwareness_g_aSectors;

static bool bOpened = false; // Added: Init to false

static size_t numEvents;
static SithAIAwarenessEvent aEvents[SITHAIAWARENESS_MAXEVENTS];

static size_t curProcessID = 0; // Added: Init to 0

void sithAIAwareness_ProcessEvents(void);
int J3DAPI sithAIAwareness_Update(int msecTime, SithEventParams* pParams);
void J3DAPI sithAIAwareness_ProcessEvent(const SithAIAwarenessEvent* pEvent, const SithSector* pSector, const rdVector3* startPos, rdVector3* endPos, float levelAtTransmittingPos, const SithSurfaceAdjoin* pTargetAdjoin, SithThing* pThing);
float J3DAPI sithAIAwareness_CheckAdjoinDistance(const SithSurfaceAdjoin* pAdjoin, const rdVector3* startPos, rdVector3* pHitPos, float moveDistance);
int J3DAPI sithAIAwareness_CreateEvent(SithSector* pSector, const rdVector3* pos, int type, float levelAtTransmittingPos, SithThing* pThing);

void sithAIAwareness_ResetGlobals(void)
{
    memset(&sithAIAwareness_g_aSectors, 0, sizeof(sithAIAwareness_g_aSectors));
}

int sithAIAwareness_Open(void)
{
    SITH_ASSERTREL(sithWorld_g_pCurrentWorld);
    if ( bOpened )
    {
        return 0;
    }

    sithAIAwareness_g_aSectors = (SithAIAwarenessSector*)STDMALLOC(sizeof(SithAIAwarenessSector) * sithWorld_g_pCurrentWorld->numSectors);
    if ( !sithAIAwareness_g_aSectors )
    {
        return 1;
    }
    memset(sithAIAwareness_g_aSectors, 0, sizeof(SithAIAwarenessSector) * sithWorld_g_pCurrentWorld->numSectors);

    numEvents = 0;
    if ( !sithEvent_RegisterTask(SITHAIAWARENESS_TASKID, sithAIAwareness_Update, 500u, SITHEVENT_TASKINTERVAL) )
    {
        return 1;
    }

    bOpened = true;
    return 0;
}

void sithAIAwareness_Close(void)
{
    if ( bOpened )
    {
        SITH_ASSERTREL(sithAIAwareness_g_aSectors);
        STDFREE(sithAIAwareness_g_aSectors);
        sithAIAwareness_g_aSectors = NULL;

        // Remove event task
        sithEvent_RegisterTask(SITHAIAWARENESS_TASKID, NULL, 0, SITHEVENT_TASKDISABLED);
        bOpened = false;
    }
}

int J3DAPI sithAIAwareness_CreateTransmittingEvent(SithSector* pSector, const rdVector3* pos, int type, float transmittingLevel, SithThing* pThing)
{
    if ( !sithAI_g_bOpen )
    {
        return 0;
    }

    SITH_ASSERTREL(type < SITHAIAWARENESS_NUMTYPES);
    return sithAIAwareness_CreateEvent(pSector, pos, type, transmittingLevel, pThing);
}

void sithAIAwareness_ProcessEvents(void)
{
    for ( size_t eventNum = 0; eventNum < numEvents; ++eventNum )
    {
        SithAIAwarenessEvent* pEvent = &aEvents[eventNum];
        sithAIAwareness_ProcessEvent(pEvent, pEvent->pSector, &pEvent->pos, &pEvent->pos, pEvent->levelAtTransmittingPos, NULL, pEvent->pThing);
    }
}

int J3DAPI sithAIAwareness_Update(int msecTime, SithEventParams* pParams)
{
    J3D_UNUSED(msecTime);
    J3D_UNUSED(pParams);

    ++curProcessID;
    if ( !numEvents )
    {
        return 1;
    }

    // Process events
    sithAIAwareness_ProcessEvents();

    // Send sound events
    for ( int aiNum = 0; aiNum <= sithAI_g_lastUsedAIIndex; ++aiNum )
    {
        SithAIControlBlock* pLocal = &sithAI_g_aControlBlocks[aiNum];
        if ( pLocal->pClass && pLocal->pOwner && (pLocal->pOwner->flags & (SITH_TF_DYING | SITH_TF_DESTROYED)) == 0 )
        {
            SithSector* pSector = pLocal->pOwner->pInSector;
            if ( pSector && sithAIAwareness_g_aSectors[sithSector_GetSectorIndex(pSector)].processID == curProcessID ) // TODO: sithSector_GetSectorIndex might return -1
            {
                sithAI_EmitEvent(pLocal, SITHAI_EVENT_SOUND, 0);
            }
        }
    }

    numEvents = 0;
    return 1;
}

int J3DAPI sithAIAwareness_IsInTransmittingRange(const SithAIAwarenessSector* pAISector, int type, const SithThing* pThing)
{
    rdVector3 vec;
    rdVector_Sub3(&vec, &pThing->pos, &pAISector->aEndPos[type]);
    float distance = stdMath_ClipNearZero(rdVector_Len3(&vec));
    return distance <= (double)pAISector->aLevelAtTransmittingPos[type];
}

void J3DAPI sithAIAwareness_ProcessEvent(const SithAIAwarenessEvent* pEvent, const SithSector* pSector, const rdVector3* startPos, rdVector3* endPos, float levelAtTransmittingPos, const SithSurfaceAdjoin* pTargetAdjoin, SithThing* pThing)
{
    SITH_ASSERTREL(pSector && pEvent);
    SITH_ASSERTREL(levelAtTransmittingPos > 0.0f);

    SithAIAwarenessSector* pAISector = &sithAIAwareness_g_aSectors[sithSector_GetSectorIndex(pSector)]; // TODO: sithSector_GetSectorIndex might return -1
    if ( pAISector->processID != curProcessID )
    {
        memset(pAISector, 0, sizeof(SithAIAwarenessSector));
        pAISector->processID = curProcessID;
    }

    if ( pAISector->aLevelAtTransmittingPos[pEvent->type] < (double)levelAtTransmittingPos )
    {
        pAISector->aLevelAtTransmittingPos[pEvent->type] = levelAtTransmittingPos;
        pAISector->aTransmittingThing[pEvent->type]      = pThing;

        rdVector_Copy3(&pAISector->aStartPos[pEvent->type], startPos);
        rdVector_Copy3(&pAISector->aEndPos[pEvent->type], endPos);

        for ( SithSurfaceAdjoin* pAdjoin = pSector->pFirstAdjoin; pAdjoin; pAdjoin = pAdjoin->pNextAdjoin )
        {
            if ( pAdjoin != pTargetAdjoin )
            {
                rdVector3 hitPos;
                float dist = sithAIAwareness_CheckAdjoinDistance(pAdjoin, endPos, &hitPos, levelAtTransmittingPos);
                float dTransmittingPos = levelAtTransmittingPos - dist;
                if ( dTransmittingPos > 0.0f )
                {
                    sithAIAwareness_ProcessEvent(pEvent, pAdjoin->pAdjoinSector, startPos, &hitPos, dTransmittingPos, pAdjoin->pMirrorAdjoin, pThing);
                }
            }
        }
    }
}

float J3DAPI sithAIAwareness_CheckAdjoinDistance(const SithSurfaceAdjoin* pAdjoin, const rdVector3* startPos, rdVector3* pHitPos, float moveDistance)
{
    SithSurface* pSurface = pAdjoin->pAdjoinSurface;
    SITH_ASSERTREL(pSurface);

    rdFace* pFace = &pSurface->face;
    SITH_ASSERTREL(pFace);

    rdVector3 moveNorm;
    rdVector_Neg3(&moveNorm, &pFace->normal);

    float hitDist;
    if ( sithIntersect_CheckSphereFaceIntersectionEx(startPos, &moveNorm, moveDistance, 0.0f, &pSurface->face, sithWorld_g_pCurrentWorld->aVertices, &hitDist, NULL, 0) )
    {
        rdVector_ScaleAdd3(pHitPos, &moveNorm, hitDist, startPos);
    }
    else
    {
        hitDist = FLT_MAX; // OG FLT_MAX was defined as 3.4028235e38f;
        for ( size_t i = 0; i < pSurface->face.numVertices; ++i )
        {
            rdVector3 vec;
            rdVector_Sub3(&vec, startPos, &sithWorld_g_pCurrentWorld->aVertices[pSurface->face.aVertices[i]]);
            float distance = stdMath_ClipNearZero(rdVector_Len3(&vec));
            if ( distance < (double)hitDist )
            {
                hitDist = distance;
                rdVector_Copy3(pHitPos, &sithWorld_g_pCurrentWorld->aVertices[pSurface->face.aVertices[i]]);
            }
        }
    }

    return hitDist;
}

int J3DAPI sithAIAwareness_CreateEvent(SithSector* pSector, const rdVector3* pos, int type, float levelAtTransmittingPos, SithThing* pThing)
{
    if ( numEvents == STD_ARRAYLEN(aEvents) )
    {
        return 0;
    }

    rdVector_Copy3(&aEvents[numEvents].pos, pos);
    aEvents[numEvents].pSector = pSector;
    aEvents[numEvents].type    = type;
    aEvents[numEvents].levelAtTransmittingPos = levelAtTransmittingPos;
    aEvents[numEvents].pThing = pThing;
    numEvents++;
    return 1;
}
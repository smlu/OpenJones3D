#include "sithItem.h"
#include <j3dcore/j3dhook.h>

#include <rdroid/Math/rdVector.h>

#include <sith/Cog/sithCog.h>
#include <sith/Devices/sithComm.h>
#include <sith/Dss/sithDSSThing.h>
#include <sith/Engine/sithCollision.h>
#include <sith/Gameplay/sithPlayer.h>
#include <sith/Gameplay/sithTime.h>
#include <sith/World/sithThing.h>
#include <sith/RTI/symbols.h>

#include <std/Win95/stdComm.h>

void sithItem_InstallHooks(void)
{
    J3D_HOOKFUNC(sithItem_PlayerCollisionHandler);
    J3D_HOOKFUNC(sithItem_Initialize);
    J3D_HOOKFUNC(sithItem_SetItemTaken);
    J3D_HOOKFUNC(sithItem_DestroyItem);
    J3D_HOOKFUNC(sithItem_ParseArg);
}

void sithItem_ResetGlobals(void)
{}

int J3DAPI sithItem_PlayerCollisionHandler(SithThing* pItem, SithThing* pPlayer, SithCollision* pCollision, int a5)
{
    J3D_UNUSED(a5);
    J3D_UNUSED(pCollision);

    SITH_ASSERTREL(pItem && pPlayer);
    SITH_ASSERTREL(pItem->type == SITH_THING_ITEM);
    SITH_ASSERTREL(pPlayer->type == SITH_THING_PLAYER);

    if ( stdComm_IsGameActive() && (pPlayer->flags & SITH_TF_REMOTE) != 0 )
    {
        return 0;
    }

    if ( !sithCollision_CheckThingLOS(pPlayer, pItem, 0) )
    {
        return 0;
    }

    if ( pItem->thingInfo.itemInfo.msecLastTouchTime < sithTime_g_msecGameTime )
    {
        pItem->thingInfo.itemInfo.msecLastTouchTime = sithTime_g_msecGameTime + 500;
    }

    return 0;
}

void J3DAPI sithItem_Initialize(SithThing* pThing)
{
    SITH_ASSERTREL(pThing);
    rdVector_Copy3(&pThing->thingInfo.itemInfo.pos, &pThing->pos);
    pThing->thingInfo.itemInfo.pInSector = pThing->pInSector;
}

void J3DAPI sithItem_SetItemTaken(SithThing* pItem, const SithThing* pSrcThing, int bNoMultiSync)
{
    if ( stdComm_IsGameActive() && !bNoMultiSync )
    {
        sithDSSThing_Take(pItem, pSrcThing, 0xFFu);
        return;
    }

    if ( pSrcThing == sithPlayer_g_pLocalPlayerThing )
    {
        sithCog_ThingSendMessage(pItem, pSrcThing, SITHCOG_MSG_TAKEN);
    }

    if ( (pItem->thingInfo.itemInfo.flags & SITH_ITEM_RESPAWN_SP) != 0 && !stdComm_IsGameActive()
        || (pItem->thingInfo.itemInfo.flags & SITH_ITEM_RESPAWN_MP) != 0 && stdComm_IsGameActive() )
    {
        // Re-spawn enabled, hide item for the time of respawn interval
        // Fyi, re-spawning handled in sithItem_DestroyItem
        pItem->flags |= SITH_TF_DISABLED; // hide item
        pItem->msecLifeLeft = (uint32_t)(pItem->thingInfo.itemInfo.secRespawnInterval * 1000.0f);
    }
    else
    {
        sithThing_DestroyThing(pItem);
    }
}

void J3DAPI sithItem_DestroyItem(SithThing* pItem)
{
    if ( stdComm_IsGameActive() && !stdComm_IsGameHost() )
    {
        pItem->msecLifeLeft = 0;
        return;
    }

    if ( pItem->thingInfo.itemInfo.pInSector
        && (stdComm_IsGameActive() && (pItem->thingInfo.itemInfo.flags & SITH_ITEM_RESPAWN_MP) != 0
            || !stdComm_IsGameActive() && (pItem->thingInfo.itemInfo.flags & SITH_ITEM_RESPAWN_SP) != 0) )
    {
        // Re-spawn item

        pItem->moveInfo.physics.velocity.x = 0.0f;
        pItem->moveInfo.physics.velocity.y = 0.0f;
        pItem->moveInfo.physics.velocity.z = 0.0f;

        sithThing_ExitSector(pItem);
        sithThing_SetPositionAndOrient(pItem, &pItem->thingInfo.itemInfo.pos, &pItem->orient);
        sithThing_SetSector(pItem, pItem->thingInfo.itemInfo.pInSector, 1);

        pItem->flags &= ~SITH_TF_DISABLED;
        pItem->msecLifeLeft = 0;

        sithCog_ThingSendMessage(pItem, pItem, SITHCOG_MSG_RESPAWN);
    }
    else if ( pItem->renderFrame + 1 == sithMain_g_frameNumber )
    {
        // Item is in camera view extend life for 3 sec
        pItem->msecLifeLeft = 3000;
    }
    else
    {
        sithThing_DestroyThing(pItem);
    }

    if ( sithMessage_g_outputstream )
    {
        sithDSSThing_UpdateState(pItem, SITHMESSAGE_SENDTOJOINEDPLAYERS, SITHMESSAGE_STREAM_ALL);
        sithDSSThing_Pos(pItem, SITHMESSAGE_SENDTOJOINEDPLAYERS, DPSEND_GUARANTEED);// TODO: make separate global var for dpflags and do secure connection
    }
}

int J3DAPI sithItem_ParseArg(const StdConffileArg* pArg, SithThing* pThing, int adjNum)
{
    switch ( adjNum )
    {
        case SITHTHING_ARG_TYPEFLAGS:
        {
            SithItemFlag iflags;
            if ( sscanf_s(pArg->argValue, "%x", &iflags) != 1 ) {
                return 0; // error
            }
            pThing->thingInfo.itemInfo.flags = iflags;
            return 1;
        }

        case SITHTHING_ARG_RESPAWN:
        {
            errno = 0; // Fixed: Reset errno, a stale ERANGE rejected valid values
            float spawnInterval = strtof(pArg->argValue, NULL); // Changed: Use strtof instead atof
            // Fixed: Only reject overflow, the original (float)atof accepted underflowing values
            if ( errno == ERANGE && fabsf(spawnInterval) > 1.0f ) { // Added
                return 0; // error
            }

            pThing->thingInfo.itemInfo.secRespawnInterval = spawnInterval;
            return 1;
        }
    };

    return 0; // error
}
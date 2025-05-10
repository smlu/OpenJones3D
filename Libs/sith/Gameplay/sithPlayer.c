#include "sithPlayer.h"
#include <j3dcore/j3dhook.h>

#include <rdroid/Math/rdMatrix.h>
#include <rdroid/Math/rdVector.h>

#include <sith/Cog/sithCog.h>
#include <sith/Devices/sithComm.h>
#include <sith/Devices/sithSoundMixer.h>
#include <sith/Dss/sithMulti.h>
#include <sith/Dss/sithDSSThing.h>
#include <sith/Engine/sithCamera.h>
#include <sith/Engine/sithPhysics.h>
#include <sith/Engine/sithPuppet.h>
#include <sith/Gameplay/sithInventory.h>
#include <sith/Gameplay/sithPlayerActions.h>
#include <sith/RTI/symbols.h>
#include <sith/World/sithActor.h>
#include <sith/World/sithThing.h>
#include <sith/World/sithWeapon.h>
#include <sith/World/sithWorld.h>

#include <std/General/stdUtil.h>
#include <std/Win95/stdComm.h>

static wchar_t sithPlayer_awLocalPlayerName[64] = { 0 }; // Added: Init to 0

void sithPlayer_InstallHooks(void)
{
    J3D_HOOKFUNC(sithPlayer_Open);
    J3D_HOOKFUNC(sithPlayer_Close);
    J3D_HOOKFUNC(sithPlayer_GetLocalPlayerName);
    J3D_HOOKFUNC(sithPlayer_PlacePlayers);
    J3D_HOOKFUNC(sithPlayer_HidePlayer);
    J3D_HOOKFUNC(sithPlayer_ShowPlayer);
    J3D_HOOKFUNC(sithPlayer_SetLocalPlayer);
    J3D_HOOKFUNC(sithPlayer_Reset);
    J3D_HOOKFUNC(sithPlayer_Update);
    J3D_HOOKFUNC(sithPlayer_NewPlayer);
    J3D_HOOKFUNC(sithPlayer_GetPlayerNum);
    J3D_HOOKFUNC(sithPlayer_PlayerKilledAction);
    J3D_HOOKFUNC(sithPlayer_KillPlayer);
    J3D_HOOKFUNC(sithPlayer_GetThingPlayerNum);
    J3D_HOOKFUNC(sithPlayer_ToggleGuybrush);
    J3D_HOOKFUNC(sithPlayer_IMPStartFiring);
    J3D_HOOKFUNC(sithPlayer_IMPEndFiring);
}

void sithPlayer_ResetGlobals(void)
{
    int sithPlayer_g_bPlayerInPor_tmp = 1; // Must be set to 1
    memcpy(&sithPlayer_g_bPlayerInPor, &sithPlayer_g_bPlayerInPor_tmp, sizeof(sithPlayer_g_bPlayerInPor));

    int sithPlayer_g_impFireType_tmp = -1;
    memcpy(&sithPlayer_g_impFireType, &sithPlayer_g_impFireType_tmp, sizeof(sithPlayer_g_impFireType));

    memset(&sithPlayer_g_playerNum, 0, sizeof(sithPlayer_g_playerNum));
    memset(&sithPlayer_g_numPlayers, 0, sizeof(sithPlayer_g_numPlayers));
    memset(&sithPlayer_g_pLocalPlayerThing, 0, sizeof(sithPlayer_g_pLocalPlayerThing));
    memset(&sithPlayer_g_pLocalPlayer, 0, sizeof(sithPlayer_g_pLocalPlayer));
    memset(&sithPlayer_g_bPlayerInvulnerable, 0, sizeof(sithPlayer_g_bPlayerInvulnerable));
    memset(&sithPlayer_g_bInAetheriumSector, 0, sizeof(sithPlayer_g_bInAetheriumSector));
    memset(&sithPlayer_g_bGuybrush, 0, sizeof(sithPlayer_g_bGuybrush));
    memset(&sithPlayer_g_curLevelNum, 0, sizeof(sithPlayer_g_curLevelNum));
    memset(&sithPlayer_g_bBonusMapBought, 0, sizeof(sithPlayer_g_bBonusMapBought));
    memset(&sithPlayer_g_aPlayers, 0, sizeof(sithPlayer_g_aPlayers));
    memset(&sithPlayer_g_impState, 0, sizeof(sithPlayer_g_impState));
}

void J3DAPI sithPlayer_Open(const wchar_t* awName)
{
    // TODO: maybe track open/close state like in other modules
    STD_WSTRCPY(sithPlayer_awLocalPlayerName, awName);
    sithPlayer_g_impState = 0.0f;
}

void sithPlayer_Close(void)
{
    // TODO: maybe track open/close state like in other modules

    // Fixed: Reset IMP state (in case IMP is active).
    //        This is required to avoid IMP being activated in the next level, as the state is not reset on open.
    sithPlayer_g_impFireType = -1;

    // Fixed: Reset player state to visible as it is not reset on open, nor in sithPlayerActions module and it will be present in the next level.
    sithPlayerActions_g_bPlayerInvisible = 0;

    // Fixed: Disable jewel flying (in case active) which resets global vars sithPlayerActions_g_bJewelFlying and sithPlayerActions_g_pPlasma
    sithPlayerActions_DisableJewelFlying();

    // Fixed: Disable swimming inventory, which could be set by jewel flying system or by cog script.
    sithInventory_SetSwimmingInventory(sithPlayer_g_pLocalPlayerThing, /*bItemsAvailable=*/1);

    sithPlayer_g_pLocalPlayerThing = NULL;
    sithPlayer_g_pLocalPlayer      = NULL;
}

const wchar_t* sithPlayer_GetLocalPlayerName(void)
{
    return sithPlayer_awLocalPlayerName;
}

void J3DAPI sithPlayer_PlacePlayers(SithWorld* pWorld)
{
    SITH_ASSERTREL(pWorld != NULL);

    size_t numPlayers = 0;
    if ( pWorld->lastThingIdx >= 0 )
    {
        for ( size_t thNum = 0; thNum <= (size_t)pWorld->lastThingIdx; thNum++ )
        {
            SithThing* pThing = &pWorld->aThings[thNum];
            if ( pThing->type == SITH_THING_PLAYER )
            {
                if ( numPlayers >= STD_ARRAYLEN(sithPlayer_g_aPlayers) )
                {
                    SITHLOG_ERROR("Too many player objects placed.\n");
                    // TODO: maybe should jump out of loop here
                }
                else
                {
                    SithPlayer* pPlayer = &sithPlayer_g_aPlayers[numPlayers];
                    pPlayer->flags    |= SITH_PLAYER_PLACED;
                    pPlayer->pThing    = pThing;
                    pPlayer->pInSector = pThing->pInSector;

                    pThing->flags |= SITH_TF_REMOTE;
                    pThing->thingInfo.actorInfo.pPlayer = pPlayer;

                    rdMatrix_Copy34(&pPlayer->orient, &pThing->orient);
                    rdVector_Copy3(&pPlayer->orient.dvec, &pThing->pos);

                    ++numPlayers;
                }
            }
        }
    }

    sithPlayer_g_numPlayers = numPlayers;

    for ( size_t playerNum = numPlayers; playerNum < STD_ARRAYLEN(sithPlayer_g_aPlayers); ++playerNum )
    {
        sithPlayer_g_aPlayers[playerNum].pThing    = NULL;
        sithPlayer_g_aPlayers[playerNum].pInSector = NULL;
    }
}

void J3DAPI sithPlayer_HidePlayer(size_t playerNum)
{
    SITH_ASSERTREL(playerNum < STD_ARRAYLEN(sithPlayer_g_aPlayers));

    SithPlayer* pPlayer = &sithPlayer_g_aPlayers[playerNum]; // Fixed: Moved after asser bound check
    pPlayer->flags &= ~SITH_PLAYER_JOINEDGAME;
    pPlayer->playerNetId = 0;

    if ( pPlayer->pThing )
    {
        if ( sithWorld_g_pCurrentWorld )
        {
            sithThing_SetThingModel(pPlayer->pThing, pPlayer->pThing->pTemplate->renderData.data.pModel3);
            pPlayer->pThing->flags |= SITH_TF_DISABLED;
        }
    }
}

int J3DAPI sithPlayer_ShowPlayer(size_t playerNum, DPID id)
{
    SITH_ASSERTREL(playerNum < sithPlayer_g_numPlayers);

    SithPlayer* pPlayer = &sithPlayer_g_aPlayers[playerNum];
    SithThing* pThing = pPlayer->pThing;
    if ( !pThing )
    {
        return 0;
    }

    pPlayer->flags |= SITH_PLAYER_UNKNOWN_04 | SITH_PLAYER_JOINEDGAME;
    pPlayer->playerNetId = id;
    pThing->flags &= ~SITH_TF_DISABLED;
    return 1;
}

void J3DAPI sithPlayer_SetLocalPlayer(size_t playerNum)
{
    SITH_ASSERTREL(playerNum < sithPlayer_g_numPlayers);
    SITH_ASSERTREL(sithWorld_g_pCurrentWorld);

    SithPlayer* pPlayer = &sithPlayer_g_aPlayers[playerNum];
    SITH_ASSERTREL(pPlayer->pThing);

    sithWorld_g_pCurrentWorld->pLocalPlayer = pPlayer->pThing;
    sithWorld_g_pCurrentWorld->pCameraFocusThing = sithWorld_g_pCurrentWorld->pLocalPlayer;
    sithPlayer_g_pLocalPlayer = &sithPlayer_g_aPlayers[playerNum];

    sithPlayer_g_pLocalPlayerThing = pPlayer->pThing;
    sithPlayer_g_pLocalPlayerThing->flags &= ~SITH_TF_REMOTE;

    sithPlayer_g_playerNum = playerNum;

    STD_WSTRCPY(sithPlayer_g_pLocalPlayer->awName, sithPlayer_awLocalPlayerName);

    for ( size_t i = 0; i < sithPlayer_g_numPlayers; ++i )
    {
        if ( sithPlayer_g_aPlayers[i].pThing )
        {
            if ( i != playerNum )
            {
                sithPlayer_g_aPlayers[i].pThing->flags |= SITH_TF_REMOTE;
            }
        }
    }
}

void J3DAPI sithPlayer_Reset(size_t playerNum)
{
    if ( playerNum >= STD_ARRAYLEN(sithPlayer_g_aPlayers) )
    {
        SITHLOG_ERROR("Invalid playerNum %d in sithPlayer_Reset.\n", playerNum);
        return;
    }

    SithPlayer* pPlayer = &sithPlayer_g_aPlayers[playerNum]; // Fixed: assign pointer after bounds check
    pPlayer->respawnMask = 0;
    pPlayer->playerNetId = 0;
    pPlayer->awName[0]   = 0;

    if ( pPlayer->pThing )
    {
        if ( sithWorld_g_pCurrentWorld )
        {
            sithInventory_InitInventory(pPlayer->pThing);
        }
    }

    pPlayer->flags &= ~(SITH_PLAYER_UNKNOWN_04 | SITH_PLAYER_JOINEDGAME);
}

void J3DAPI sithPlayer_Update(SithPlayer* pPlayer, float secDetaTime)
{
    if ( pPlayer != sithPlayer_g_pLocalPlayer )
    {
        return;
    }

    SITH_ASSERTREL(pPlayer);

    SithThing* pThing = pPlayer->pThing;
    SITH_ASSERTREL(pThing);
    sithWeapon_UpdateActorWeaponState(pThing);

    // Did thing fall into falldeth sector?
    if ( !pThing->attach.flags
        && (pThing->thingInfo.actorInfo.flags & SITH_AF_FALLKILLED) == 0
        && pThing->moveType == SITH_MT_PHYSICS
        && pThing->pInSector
        && (pThing->pInSector->flags & SITH_SECTOR_FALLDEATH) != 0
        && (pThing->thingInfo.actorInfo.flags & SITH_AF_INVULNERABLE) == 0
        && (sithMain_g_sith_mode.debugModeFlags & SITHDEBUG_INEDITOR) == 0 )
    {
        pThing->thingInfo.actorInfo.flags |= SITH_AF_FALLKILLED;
    }

    if ( (pThing->thingInfo.actorInfo.flags & SITH_AF_FALLKILLED) != 0 && (pThing->flags & SITH_TF_DYING) == 0 )
    {
        // Player fell into pit, kill him
        pThing->flags |= SITH_TF_DYING; // Note, this will prevent sithActor_Kill function to do any action
        sithPlayer_KillPlayer(pThing);
    }

    // Update IMP fire state
    bool bImpStateUpdated = false;
    if ( sithPlayer_g_impFireType != -1 )
    {
        switch ( sithPlayer_g_impFireType )
        {
            case SITHWEAPON_IMP1:
            case SITHWEAPON_IMP4:
            {
                sithPlayer_g_impState = sithGetIMPDamageScalar() * 160.0f + sithPlayer_g_impState;
                bImpStateUpdated = true;

                sithPlayer_g_impFireType = -1; // Note: fire type has to be reset for Urgon's part and Azerim's part as it's single shot 
                if ( sithPlayer_g_impState > 180.0f )
                {
                    float damage = sithGetIMPDamageScalar() * ((sithPlayer_g_impState - 180.0f) * 2.0f); // Don't move
                    sithPlayer_g_impState = 180.0f;
                    sithThing_DamageThing(pThing, pThing, damage, SITH_DAMAGE_IMP_BLAST);
                }

            } break;

            case SITHWEAPON_IMP2:
            case SITHWEAPON_IMP3:
            case SITHWEAPON_IMP5:
            {
                sithPlayer_g_impState = sithGetIMPDamageScalar() * 10.0f * secDetaTime + sithPlayer_g_impState;
                bImpStateUpdated = true;

                if ( sithPlayer_g_impState > 180.0f )
                {
                    sithPlayer_g_impState = 180.0f;
                    float damage = sithGetIMPDamageScalar() * secDetaTime * 40.0f;
                    // Fixed: Set min damage to 1.0f as sithThing_DamageThing cuts off damages lower than 1.0f. ( >40 fps)
                #ifdef J3D_QOL_IMPROVEMENTS
                    damage = J3DMAX(damage, 1.0f);
                #endif
                    sithThing_DamageThing(pThing, pThing, damage, SITH_DAMAGE_IMP_BLAST);
                }

            } break;

            default:
                break;
        }
    }

    // Update state in Aetherium or POR
    if ( (pThing->pInSector->flags & SITH_SECTOR_AETHERIUM) != 0 )
    {
        sithPlayer_g_impState = sithGetIMPDamageScalar() * secDetaTime + sithPlayer_g_impState;
        if ( sithPlayer_g_impState >= 180.0f )
        {
            sithPlayer_g_impState = 180.0f;
            float damage = sithGetIMPDamageScalar() * secDetaTime * 180.0f;;
            sithThing_DamageThing(pThing, pThing, damage, SITH_DAMAGE_IMP_BLAST);
        }
    }
    else if ( sithPlayer_g_impState != 0.0f && (pThing->flags & (SITH_TF_DYING | SITH_TF_DESTROYED)) == 0 )
    {
        if ( sithPlayer_g_bPlayerInPor )
        {
            if ( !bImpStateUpdated )
            {
                // Player is safe In POR, increase aetherium stamina
                sithPlayer_g_impState -= secDetaTime * 8.0f;
                if ( sithPlayer_g_impState < 0.0f )
                {
                    sithPlayer_g_impState = 0.0f;
                }
            }
        }
    }
}

void J3DAPI sithPlayer_NewPlayer(SithThing* pPlayer)
{
    SITH_ASSERTREL(pPlayer);
    SITH_ASSERTREL((pPlayer->type == SITH_THING_PLAYER) || (pPlayer->type == SITH_THING_CORPSE));
    SITH_ASSERTREL(pPlayer->thingInfo.actorInfo.pPlayer);

    if ( pPlayer->renderData.pPuppet && pPlayer->pPuppetState )
    {
        sithPuppet_RemoveAllTracks(pPlayer);
    }

    if ( !stdComm_IsGameActive() || (pPlayer->flags & SITH_TF_REMOTE) == 0 )
    {
        SITH_ASSERTREL(pPlayer->pTemplate);
        pPlayer->thingInfo.actorInfo.health = pPlayer->pTemplate->thingInfo.actorInfo.health;

        if ( (pPlayer->pTemplate->moveInfo.physics.flags & SITH_PF_ALIGNUP) != 0 ) // TODO: This must be a bug, probably should be check for not set SITH_PF_ALIGNUP flag
        {
            pPlayer->moveInfo.physics.flags|= SITH_PF_ALIGNUP;
            pPlayer->moveInfo.physics.flags &= ~(SITH_PF_ALIGNED | SITH_PF_ALIGNSURFACE);
        }

        sithActor_SetHeadPYR(pPlayer, &rdroid_g_zeroVector3);

        if ( pPlayer == sithPlayer_g_pLocalPlayerThing )
        {
            sithCamera_SetCameraFocus(&sithCamera_g_aCameras[SITHCAMERA_INTCAMERANUM], pPlayer, NULL);
            sithCamera_SetCameraFocus(&sithCamera_g_aCameras[SITHCAMERA_EXTCAMERANUM], pPlayer, NULL);
            sithCamera_RestoreExtCamera();
            sithCamera_SetCurrentToCycleCamera();
        }

        pPlayer->flags &= ~(SITH_TF_DYING | SITH_TF_DESTROYED);
        pPlayer->thingInfo.actorInfo.flags &= ~SITH_AF_FALLKILLED;
        pPlayer->msecLifeLeft = 0;

        if ( stdComm_IsGameActive() && pPlayer == sithPlayer_g_pLocalPlayerThing )
        {
            size_t spawnPlayerNum = sithMulti_Respawn(pPlayer);
            sithThing_ExitSector(pPlayer);
            sithThing_SetPositionAndOrient(pPlayer, &sithPlayer_g_aPlayers[spawnPlayerNum].orient.dvec, &sithPlayer_g_aPlayers[spawnPlayerNum].orient);
            sithThing_EnterSector(pPlayer, sithPlayer_g_aPlayers[spawnPlayerNum].pInSector, 1, 0);

            sithCamera_Update(sithCamera_g_pCurCamera);

            pPlayer->thingInfo.actorInfo.flags &= ~SITH_AF_CONTROLSDISABLED;

            sithPhysics_ResetThingMovement(pPlayer);
            sithWeapon_DeactivateCurrentWeapon(pPlayer);
            sithCog_BroadcastMessage(SITHCOG_MSG_NEWPLAYER, SITHCOG_SYM_REF_THING, pPlayer->idx, SITHCOG_SYM_REF_THING, pPlayer->idx);
            if ( sithMessage_g_outputstream )
            {
                sithDSSThing_UpdateState(pPlayer, SITHMESSAGE_SENDTOJOINEDPLAYERS, SITHMESSAGE_STREAM_ALL);
            }
        }
    }
}

int J3DAPI sithPlayer_GetPlayerNumByName(wchar_t* pwName)
{
    if ( !pwName )
    {
        return -1;
    }

    for ( size_t i = 0; i < sithPlayer_g_numPlayers; ++i )
    {
        if ( (sithPlayer_g_aPlayers[i].flags & SITH_PLAYER_JOINEDGAME) != 0 && wstreqi(sithPlayer_g_aPlayers[i].awName, pwName) )
        {
            return i;
        }
    }

    return -1;
}

int J3DAPI sithPlayer_GetPlayerNum(DPID playerId)
{
    if ( !playerId )
    {
        return -1;
    }

    for ( size_t i = 0; i < sithPlayer_g_numPlayers; ++i )
    {
        if ( playerId == sithPlayer_g_aPlayers[i].playerNetId )
        {
            return i;
        }
    }

    return -1;
}

void J3DAPI sithPlayer_PlayerKilledAction(SithThing* pPlayerThing, const SithThing* pSrcThing)
{
    SithPlayer* pPlayer = pPlayerThing->thingInfo.actorInfo.pPlayer;
    SITH_ASSERTREL(pPlayer);

    pPlayerThing->moveInfo.physics.flags |= (SITH_PF_ALIGNSURFACE | SITH_PF_USEGRAVITY);
    pPlayerThing->moveInfo.physics.flags &= ~(SITH_PF_ALIGNED | SITH_PF_ALIGNUP);

    pPlayerThing->flags |= SITH_TF_DYING;

    if ( (pPlayerThing->moveInfo.physics.flags & (SITH_PF_JEEP | SITH_PF_MINECAR)) != 0 )
    {
        memset(&pPlayerThing->moveInfo.physics.rotThrust, 0, sizeof(pPlayerThing->moveInfo.physics.rotThrust));
        memset(&pPlayerThing->moveInfo.physics.thrust, 0, sizeof(pPlayerThing->moveInfo.physics.thrust));
    }
    else
    {
        sithPhysics_ResetThingMovement(pPlayerThing);
    }

    sithWeapon_DeactivateCurrentWeapon(pPlayerThing);
    sithInventory_BroadcastKilledMessage(pPlayerThing, pSrcThing);

    if ( stdComm_IsGameActive() )
    {
        sithMulti_ProcessKilledPlayer(pPlayer, pPlayerThing, pSrcThing);
    }
}

void J3DAPI sithPlayer_KillPlayer(SithThing* pPlayerThing)
{
    SithPlayer* pPlayer = pPlayerThing->thingInfo.actorInfo.pPlayer;

    SITH_ASSERTREL(pPlayer);

    if ( pPlayerThing == sithPlayer_g_pLocalPlayerThing )
    {
        sithDSSThing_Death(pPlayerThing, pPlayerThing, 1, SITHMESSAGE_SENDTOJOINEDPLAYERS, SITHMESSAGE_STREAM_ALL);
    }

    if ( (pPlayerThing->flags & SITH_TF_COGLINKED) == 0
        || (sithCog_ThingSendMessage(pPlayerThing, pPlayerThing, SITHCOG_MSG_KILLED), (pPlayerThing->flags & SITH_TF_DESTROYED) == 0) )
    {
        sithSoundMixer_StopSoundThing(pPlayerThing, 0);
        sithThing_DetachAttachedThings(pPlayerThing);
        sithActor_SetHeadPYR(pPlayerThing, &rdroid_g_zeroVector3);

        pPlayerThing->moveInfo.physics.flags |= SITH_PF_ALIGNSURFACE | SITH_PF_USEGRAVITY;
        pPlayerThing->moveInfo.physics.flags &= ~(SITH_PF_ALIGNUP | SITH_PF_ALIGNED);

        sithActor_KillActor(pPlayerThing, NULL, SITH_DAMAGE_DROWN); // TODO: why drown damage ?
        sithWeapon_DeactivateCurrentWeapon(pPlayerThing);

        if ( stdComm_IsGameActive() )
        {
            sithMulti_ProcessKilledPlayer(pPlayer, pPlayerThing, pPlayerThing);
        }
    }
}

float J3DAPI sithPlayer_GetInvItemAmount(size_t binIndex)
{
    SITH_ASSERTREL((((sithPlayer_g_playerNum) >= (0)) ? (((sithPlayer_g_playerNum) <= (sithPlayer_g_numPlayers - 1)) ? 1 : 0) : 0));
    SITH_ASSERTREL(binIndex < STD_ARRAYLEN(sithPlayer_g_aPlayers[sithPlayer_g_playerNum].aItems));
    return sithPlayer_g_aPlayers[sithPlayer_g_playerNum].aItems[binIndex].amount;
}

void J3DAPI sithPlayer_SetInvItemAmount(size_t binIndex, float amount)
{
    SITH_ASSERTREL((((sithPlayer_g_playerNum) >= (0)) ? (((sithPlayer_g_playerNum) <= (sithPlayer_g_numPlayers - 1)) ? 1 : 0) : 0));
    SITH_ASSERTREL(binIndex < STD_ARRAYLEN(sithPlayer_g_aPlayers[sithPlayer_g_playerNum].aItems));
    sithPlayer_g_aPlayers[sithPlayer_g_playerNum].aItems[binIndex].amount = amount;
}

void J3DAPI sithPlayer_SetInvItemAvailable(size_t binIndex, int bAvailable)
{
    SITH_ASSERTREL((((sithPlayer_g_playerNum) >= (0)) ? (((sithPlayer_g_playerNum) <= (sithPlayer_g_numPlayers - 1)) ? 1 : 0) : 0));
    SITH_ASSERTREL(binIndex < STD_ARRAYLEN(sithPlayer_g_aPlayers[sithPlayer_g_playerNum].aItems));

    SithInventoryItem* pItem = &sithPlayer_g_aPlayers[sithPlayer_g_playerNum].aItems[binIndex];
    if ( bAvailable )
    {
        pItem->status |= SITHINVENTORY_ITEM_AVAILABLE;
    }
    else
    {
        pItem->status &= ~SITHINVENTORY_ITEM_AVAILABLE;
    }
}

bool J3DAPI sithPlayer_IsInvItemAvailable(size_t binIndex)
{
    SITH_ASSERTREL((((sithPlayer_g_playerNum) >= (0)) ? (((sithPlayer_g_playerNum) <= (sithPlayer_g_numPlayers - 1)) ? 1 : 0) : 0));
    SITH_ASSERTREL(binIndex < STD_ARRAYLEN(sithPlayer_g_aPlayers[sithPlayer_g_playerNum].aItems));
    return (sithPlayer_g_aPlayers[sithPlayer_g_playerNum].aItems[binIndex].status & SITHINVENTORY_ITEM_AVAILABLE) != 0;
}


int J3DAPI sithPlayer_GetThingPlayerNum(const SithThing* pThing)
{
    if ( !pThing || pThing->type != SITH_THING_PLAYER )
    {
        return -1;
    }

    if ( !stdComm_IsGameActive() )
    {
        return 0;
    }

    for ( size_t i = 0; i < sithPlayer_g_numPlayers; ++i )
    {
        if ( (sithPlayer_g_aPlayers[i].flags & SITH_PLAYER_JOINEDGAME) != 0 && sithPlayer_g_aPlayers[i].pThing == pThing )
        {
            return i;
        }
    }

    return -1;
}

int J3DAPI sithPlayer_GetThingPlayerNumByIndex(int thingIdx)
{
    if ( !stdComm_IsGameActive() )
    {
        return 0;
    }

    for ( size_t i = 0; i < sithPlayer_g_numPlayers; ++i )
    {
        if ( (sithPlayer_g_aPlayers[i].flags & SITH_PLAYER_JOINEDGAME) != 0 && sithPlayer_g_aPlayers[i].pThing->idx == thingIdx )
        {
            return i;
        }
    }

    return -1;
}

int sithPlayer_ToggleGuybrush(void)
{
    if ( !sithPlayer_g_pLocalPlayerThing )
    {
        return 0;
    }

    if ( (sithPlayer_g_pLocalPlayerThing->moveInfo.physics.flags & (SITH_PF_JEEP | SITH_PF_RAFT | SITH_PF_MINECAR)) != 0 )
    {
        return 0;
    }

    if ( sithPlayer_g_pLocalPlayerThing->pCog )
    {
        if ( sithPlayer_g_bGuybrush )
        {
            sithCog_SendMessage(sithPlayer_g_pLocalPlayerThing->pCog, SITHCOG_MSG_USER1, SITHCOG_SYM_REF_NONE, 0, SITHCOG_SYM_REF_THING, sithPlayer_g_pLocalPlayerThing->idx, 0);
            sithPlayer_g_bGuybrush = 0;
        }
        else
        {
            sithCog_SendMessage(sithPlayer_g_pLocalPlayerThing->pCog, SITHCOG_MSG_USER0, SITHCOG_SYM_REF_NONE, 0, SITHCOG_SYM_REF_THING, sithPlayer_g_pLocalPlayerThing->idx, 0);
            sithPlayer_g_bGuybrush = 1;
        }
    }

    for ( SithWeaponId i = SITHWEAPON_SATCHEL; i < SITHWEAPON_IMP4; ++i )
    {
        SithCog* pCog = sithCog_GetCogByIndex(SITHWORLD_STATICINDEX(i));
        if ( pCog )
        {
            sithCog_SendMessage(pCog, SITHCOG_MSG_STARTUP, SITHCOG_SYM_REF_INT, 9999, SITHCOG_SYM_REF_NONE, 0, 0);
        }
    }

    return 1;
}

void J3DAPI sithPlayer_IMPStartFiring(int fireType)
{
    sithPlayer_g_impFireType = fireType;
}

void J3DAPI sithPlayer_IMPEndFiring(int fireType)
{
    if ( fireType == sithPlayer_g_impFireType )
    {
        sithPlayer_g_impFireType = -1;
    }
}
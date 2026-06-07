#include "sithCommand.h"

#include <j3dcore/j3dhook.h>

#include <sith/AI/sithAI.h>
#include <sith/AI/sithAIUtil.h>
#include <sith/Cog/sithCog.h>
#include <sith/Cog/sithCogFunction.h>
#include <sith/Devices/sithConsole.h>
#include <sith/Devices/sithSoundMixer.h>
#include <sith/Dss/sithMulti.h>
#include <sith/Engine/sithCamera.h>
#include <sith/Engine/sithIntersect.h>
#include <sith/Engine/sithPhysics.h>
#include <sith/Engine/sithPuppet.h>
#include <sith/Gameplay/sithEvent.h>
#include <sith/Gameplay/sithInventory.h>
#include <sith/Gameplay/sithPlayer.h>
#include <sith/Gameplay/sithPlayerActions.h>
#include <sith/Main/sithMain.h>
#include <sith/Main/sithString.h>
#include <sith/RTI/symbols.h>
#include <sith/World/sithMaterial.h>
#include <sith/World/sithSector.h>
#include <sith/World/sithThing.h>
#include <sith/World/sithWorld.h>

#include <rdroid/Engine/rdMaterial.h>
#include <rdroid/Math/rdMatrix.h>

#include <std/General/stdMemory.h>
#include <std/General/stdUtil.h>
#include <std/Win95/stdComm.h>

typedef struct sSithCommandMatInfo
{
    size_t num;
    size_t surfCount;
    size_t memPerSurf;
    size_t memUsage;
} SithCommandMatInfo;

static char aCipherTextBuf[256] = { 0 }; // Added: Init to 0

int J3DAPI sithCommand_DebugMode(const SithConsoleCommand* pFunc, const char* pArg)
{
    SITH_ASSERTREL(pFunc);

    // Added: Init to null and 0
    int* pFlags  = NULL;
    int newFlags = 0;
    int cmdFlags = pFunc->flags & ~SITHCONSOLE_DEVMODE; // Added: Remove devmode flag
    switch ( cmdFlags )
    {
        case 0:
            pFlags = &sithMain_g_sith_mode.debugModeFlags;
            newFlags = SITHDEBUG_AIEVENTS_DISABLED;
            break;

        case 1:
            pFlags = &sithMain_g_sith_mode.debugModeFlags;
            newFlags = SITHDEBUG_PUPPETSYSTEM_DISABLED;
            break;

        case 2:
            pFlags = &sithMain_g_sith_mode.debugModeFlags;
            newFlags = SITHDEBUG_TRACKSHOTS;
            break;

        case 3:
            pFlags = &sithMain_g_sith_mode.debugModeFlags;
            newFlags = SITHDEBUG_AIDISABLED;
            break;

        case 4:  // invul
            if ( !sithWorld_g_pCurrentWorld || !sithWorld_g_pCurrentWorld->pLocalPlayer || sithWorld_g_pCurrentWorld->pLocalPlayer->type != SITH_THING_PLAYER )
            {
                return 0;
            }

            pFlags = (int*)&sithWorld_g_pCurrentWorld->pLocalPlayer->thingInfo.actorInfo.flags;
            newFlags = SITH_AF_INVULNERABLE;
            break;

        case 5:
            pFlags = &sithMain_g_sith_mode.debugModeFlags;
            newFlags = SITHDEBUG_AINOTARGET;
            break;

        case 6:
            pFlags = &sithMain_g_sith_mode.debugModeFlags;
            newFlags = SITHDEBUG_SLOWMODE;
            break;

        default:
            return 0;
    }

    // Set/Clear flag
    if ( pArg )
    {
        bool bON;
        if ( sithCommand_ParseBool(pArg, &bON) )
        {
            if ( bON )
            {
                *pFlags |= newFlags;
                if ( cmdFlags == 4 ) // invul
                {
                    sithPlayer_g_bPlayerInvulnerable = 1;
                }
            }
            else
            {
                *pFlags &= ~newFlags;
                if ( cmdFlags == 4 )    // invul
                {
                    sithPlayer_g_bPlayerInvulnerable = 0;
                }
            }
        }
    }

    // Print command state to console
    wchar_t awComName[32];
    STD_TOWSTR(awComName, pFunc->aName);

    const wchar_t* pwState;
    if ( (newFlags & *pFlags) != 0 )
    {
        pwState = sithString_GetString("ON");
    }
    else
    {
        pwState = sithString_GetString("OFF");
    }

    wchar_t awText[128];
    const wchar_t* pwFormat = sithString_GetString("%s_IS_%s"); // Note, bug calling sithString_GetString will override pwState due to stdStrTable internal cache
    STD_WFORMAT(awText, pwFormat, awComName, pwState);
    sithConsole_PrintWString(awText);

    return 1;
}

int J3DAPI sithCommand_Wuss(const SithConsoleCommand* pFunc, const char* pArg)
{
    J3D_UNUSED(pFunc);
    J3D_UNUSED(pArg);

    if ( !sithPlayer_g_pLocalPlayerThing )
    {
        return 0;
    }

    SithCog* pCog = sithCog_GetCogByIndex(SITHWORLD_STATICINDEX(31)); //0x801F
    if ( !pCog )
    {
        return 0;
    }

    sithCog_SendMessage(pCog, SITHCOG_MSG_SELECTED, SITHCOG_SYM_REF_NONE, 0, SITHCOG_SYM_REF_THING, sithPlayer_g_pLocalPlayerThing->idx, 0);
    return 1;
}

int J3DAPI sithCommand_Fixme(const SithConsoleCommand* pFunc, const char* pArg)
{
    J3D_UNUSED(pFunc);
    J3D_UNUSED(pArg);

    SithThing* pThing = sithPlayer_g_pLocalPlayerThing;
    if ( !sithPlayer_g_pLocalPlayerThing )
    {
        return 0;
    }

#ifndef J3D_SPEEDRUN_BUILD
    // Fixed: Fixes missing floorstick flag,
    //        stopping correctly all currently played forcemove tracks,
    //        and detaches thing from any attached surface/object
    pThing->moveInfo.physics.flags |= SITH_PF_FLOORSTICK;
    if ( pThing->thingInfo.actorInfo.bForceMovePlay )
    {
        sithPuppet_FinishForceMove(pThing, /*bStopTracks=*/1);
    }
    sithThing_DetachThing(pThing);
#endif

    pThing->moveStatus       = SITHPLAYERMOVE_STILL;
    pThing->collide.movesize = 0.04f;

    pThing->thingInfo.actorInfo.bControlsDisabled = 0;
    pThing->thingInfo.actorInfo.bForceMovePlay    = 0;

    pThing->moveInfo.physics.flags &= ~SITH_PF_ALIGNSURFACE;
    pThing->moveInfo.physics.flags |= SITH_PF_ALIGNUP;
    pThing->moveInfo.physics.height = 0.09f;

    sithPuppet_SetMoveMode(pThing, SITHPUPPET_MOVEMODE_NORMAL);

    return 1;
}

int J3DAPI sithCommand_Ouch(const SithConsoleCommand* pFunc, const char* pArg)
{
    J3D_UNUSED(pFunc);
    J3D_UNUSED(pArg);

    if ( !sithPlayer_g_pLocalPlayerThing )
    {
        return 0;
    }

    SithCog* pCog = sithCog_GetCogByIndex(SITHWORLD_STATICINDEX(32)); // 0x8020
    if ( !pCog )
    {
        return 0;
    }

    sithCog_SendMessage(pCog, SITHCOG_MSG_SELECTED, SITHCOG_SYM_REF_NONE, 0, SITHCOG_SYM_REF_THING, sithPlayer_g_pLocalPlayerThing->idx, 0);
    return 1;
}

int J3DAPI sithCommand_MakeMePirate(const SithConsoleCommand* pFunc, const char* pArg)
{
    J3D_UNUSED(pFunc);
    J3D_UNUSED(pArg);

    return sithPlayer_ToggleGuybrush();
}

int J3DAPI sithCommand_Coords(const SithConsoleCommand* pFunc, const char* pArg)
{
    J3D_UNUSED(pFunc);
    J3D_UNUSED(pArg);

    if ( !sithWorld_g_pCurrentWorld || !sithWorld_g_pCurrentWorld->pLocalPlayer )
    {
        sithConsole_PrintString("No world.");
        return 0;
    }

    SithThing* pPlayer = sithWorld_g_pCurrentWorld->pLocalPlayer;
    if ( !pPlayer->pInSector )
    {
        sithConsole_PrintString("Thing is not in world");
    }
    else
    {
        rdVector3 pyr;
        rdMatrix_ExtractAngles34(&pPlayer->orient, &pyr);
        int secIdx = sithSector_GetSectorIndex(pPlayer->pInSector);

        SITHCONSOLE_PRINTF(
            "Pos: (%.2f, %.2f, %.2f) PYR: (%.2f, %.2f, %.2f) Sector: %d.",
            pPlayer->pos.x,
            pPlayer->pos.y,
            pPlayer->pos.z,
            pyr.pitch,
            pyr.yaw,
            pyr.roll,
            secIdx
        );
    }

    return 1;
}

int J3DAPI sithCommand_Warp(const SithConsoleCommand* pFunc, const char* pArg)
{
    J3D_UNUSED(pFunc);

    SithWorld* pWorld = sithWorld_g_pCurrentWorld;
    if ( !sithWorld_g_pCurrentWorld || !pWorld->pLocalPlayer )
    {
        sithConsole_PrintString("No world.");
        return 0;
    }

    if ( !pArg )
    {
        sithConsole_PrintString("Format: WARP x y z");
        return 0;
    }

    rdVector3 pos, pyr;
    int numParsed = sscanf_s(pArg, "%f %f %f %f %f %f", &pos.x, &pos.y, &pos.z, &pyr.pitch, &pyr.yaw, &pyr.roll);
    if ( numParsed < 3 )
    {
        return 0;
    }

    rdMatrix34 mat;
    if ( numParsed == 6 )
    {
        rdMatrix_BuildRotate34(&mat, &pyr);
    }
    else
    {
        rdMatrix_Identity34(&mat);
    }

    // Get sector for position
    size_t numSecs = 0;
    SithSector* pSector = pWorld->aSectors;
    for ( ; numSecs < pWorld->numSectors && !sithIntersect_IsSphereInSector(pWorld, &pos, 0.0f, pSector); ++pSector )
    {
        ++numSecs;
    }

    if ( numSecs == pWorld->numSectors )
    {
        sithConsole_PrintString("Position not in world");
        return 0;
    }

    SithThing* pThing = pWorld->pLocalPlayer;
    sithThing_DetachThing(pThing);
    sithThing_ExitSector(pThing);
    sithThing_SetPositionAndOrient(pThing, &pos, &mat);
    sithThing_EnterSector(pThing, pSector, 1, 0);
    return 1;
}

int J3DAPI sithCommand_Jump(const SithConsoleCommand* pFunc, const char* pArg)
{
    J3D_UNUSED(pFunc);

    if ( !pArg )
    {
        return 0;
    }

    size_t playerNum;
    if ( sscanf_s(pArg, "%d", &playerNum) == 0 )
    {
        return 0;
    }

    sithPlayerActions_MoveToPlayerPosition(sithPlayer_g_pLocalPlayerThing, playerNum - 1);
    return 1;
}

int J3DAPI sithCommand_Memory(const SithConsoleCommand* pFunc, const char* pArg)
{
    J3D_UNUSED(pFunc);

    const SithWorld* pWorld = sithWorld_g_pCurrentWorld;
    if ( pArg )
    {
        pWorld = sithWorld_g_pStaticWorld;
    }

    if ( !pWorld )
    {
        sithConsole_PrintString("No world.");
        return 0;
    }

    size_t aMemUsed[17] = { 0 }; // Added: Init to 0
    size_t aCount[17]   = { 0 }; // Added: Init to 0
    sithWorld_GetMemoryUsage(pWorld, &aMemUsed, &aCount);

    size_t totalUsed = 0;

    SITHCONSOLE_PRINTF("%5d Materials        %8d bytes.", aCount[0], aMemUsed[0]);
    totalUsed += aMemUsed[0];

    SITHCONSOLE_PRINTF("%5d Models           %8d bytes.", aCount[10], aMemUsed[10]);
    totalUsed += aMemUsed[10];

    SITHCONSOLE_PRINTF("%5d Keyframes        %8d bytes.", aCount[11], aMemUsed[11]);
    totalUsed += aMemUsed[11];

    SITHCONSOLE_PRINTF("%5d World Vertices   %8d bytes.", aCount[1], aMemUsed[1]);
    totalUsed += aMemUsed[1];

    SITHCONSOLE_PRINTF("%5d World TexVerts   %8d bytes.", aCount[2], aMemUsed[2]);
    totalUsed += aMemUsed[2];

    SITHCONSOLE_PRINTF("%5d Surfaces         %8d bytes.", aCount[3], aMemUsed[3]);
    totalUsed += aMemUsed[3];

    SITHCONSOLE_PRINTF("%5d Sectors          %8d bytes.", aCount[5], aMemUsed[5]);
    totalUsed += aMemUsed[5];

    SITHCONSOLE_PRINTF("%5d Cog Scripts\t\t%8d bytes.", aCount[8], aMemUsed[8]);
    totalUsed += aMemUsed[8];

    SITHCONSOLE_PRINTF("%5d Cogs             %8d bytes.", aCount[7], aMemUsed[7]);
    totalUsed += aMemUsed[7];

    SITHCONSOLE_PRINTF("%5d Adjoins          %8d bytes.", aCount[4], aMemUsed[4]);
    totalUsed += aMemUsed[4];

    SITHCONSOLE_PRINTF("Total Memory Used:   %8d bytes.", totalUsed);
    return 1;
}

int J3DAPI sithCommand_DynamicMemory(const SithConsoleCommand* pFunc, const char* pArg)
{
    J3D_UNUSED(pFunc);
    J3D_UNUSED(pArg);

    SITHCONSOLE_PRINTF(
        "NumAllocs: %d TotalMemAlloc: %d bytes MaxMemAlloc: %d bytes",
        stdMemory_g_curState.totalAllocs,
        stdMemory_g_curState.totalBytes,
        stdMemory_g_curState.maxBytes
    );
    return 1;
}

int J3DAPI sithCommand_MemoryDump(const SithConsoleCommand* pFunc, const char* pArg)
{
    J3D_UNUSED(pFunc);
    J3D_UNUSED(pArg);

    tFileHandle fh = sith_g_pHS->pFileOpen("memdump.txt", "w+");
    if ( !fh )
    {
        return 0;
    }

    sith_g_pHS->pFilePrintf(
        fh,
        "Total Memory allocated: %d bytes   # Allocations: %d  Max Memory used: %d bytes\n\n",
        stdMemory_g_curState.totalBytes,
        stdMemory_g_curState.totalAllocs,
        stdMemory_g_curState.maxBytes
    );

    for ( tMemoryHeader* pMemHeader = stdMemory_g_curState.header.pNext; pMemHeader; pMemHeader = pMemHeader->pNext )
    {
        sith_g_pHS->pFilePrintf(fh, "%25s:  line %3d   %8d bytes   #%d\n", pMemHeader->pFilename, pMemHeader->line, pMemHeader->size, pMemHeader->number);
    }

    sith_g_pHS->pFileClose(fh);
    sithConsole_PrintString("Memory dump file 'memdump.txt' written.");
    return 1;
}

int J3DAPI sithCommand_Players(const SithConsoleCommand* pFunc, const char* pArg)
{
    J3D_UNUSED(pFunc);
    J3D_UNUSED(pArg);

    SITHCONSOLE_PRINTF("Maxplayers = %d", sithPlayer_g_numPlayers);
    for ( size_t i = 0; i < sithPlayer_g_numPlayers; ++i )
    {
        char aString[64];
        STD_TOSTR(aString, sithPlayer_g_aPlayers[i].awName);
        SITHCONSOLE_PRINTF("Player %d:  Name: %s  Flags: %x  ID: %x", i, aString, sithPlayer_g_aPlayers[i].flags, sithPlayer_g_aPlayers[i].playerNetId);
    }

    return 1;
}

int J3DAPI sithCommand_PingPlayer(const SithConsoleCommand* pFunc, const char* pName)
{
    J3D_UNUSED(pFunc);

    if ( !stdComm_IsGameActive() )
    {
        sithConsole_PrintString("Game not active.");
        return 0;
    }

    int num = 0;
    if ( pName )
    {
        wchar_t awName[64];
        STD_TOWSTR(awName, pName);
        num = sithPlayer_GetPlayerNumByName(awName);
        if ( num < 0 )
        {
            SITHCONSOLE_PRINTF("Player %s not found", (const char*)awName);
        }
    }

    sithMulti_Ping(sithPlayer_g_aPlayers[num].playerNetId);
    return 1;
}

int J3DAPI sithCommand_Kick(const SithConsoleCommand* pFunc, const char* pArg)
{
    J3D_UNUSED(pFunc);

    if ( !stdComm_IsGameActive() || !stdComm_IsGameHost() )
    {
        sithConsole_PrintString("Game not active.");
        return 0;
    }

    if ( !pArg )
    {
        sithConsole_PrintString("Format: kick <playername>");
        return 0;
    }

    wchar_t aPlayerName[64];
    STD_TOWSTR(aPlayerName, pArg);
    for ( size_t i = 0; i < sithPlayer_g_numPlayers; ++i )
    {
        if ( (sithPlayer_g_aPlayers[i].flags & SITH_PLAYER_JOINEDGAME) != 0 && wstreqi(sithPlayer_g_aPlayers[i].awName, aPlayerName) )
        {
            SITHCONSOLE_PRINTF("Kicked %S", sithPlayer_g_aPlayers[i].awName);
            sithMulti_QuitPlayer(sithPlayer_g_aPlayers[i].playerNetId);
        }
    }

    return 1;
}

int J3DAPI sithCommand_Tick(const SithConsoleCommand* pFunc, const char* pStrTickRate)
{
    J3D_UNUSED(pFunc);

    if ( !stdComm_IsGameActive() )
    {
        sithConsole_PrintString("Game not active.");
        return 0;
    }

    size_t newRate;
    if ( pStrTickRate && (newRate = atoi(pStrTickRate)) != 0 )
    {
        if ( newRate >= 50 && newRate <= 300 )
        {
            sithMulti_SetTickRate(newRate);
            sithEvent_RegisterTask(SITHMULTI_CHECKPLAYER_TASKID, sithMulti_CheckPlayers, newRate, SITHEVENT_TASKINTERVAL);
            return 1;
        }

        sithConsole_PrintString("New tick is out of range");
        return 0;
    }

    size_t curRate = sithMulti_GetTickRate();
    SITHCONSOLE_PRINTF("Current tick rate is %d msec", curRate);
    return 1;
}

int J3DAPI sithCommand_Session(const SithConsoleCommand* pFunc, const char* pArg)
{
    J3D_UNUSED(pFunc);
    J3D_UNUSED(pArg);

    if ( !stdComm_IsGameActive() )
    {
        sithConsole_PrintString("Game not active.");
        return 0;
    }

    if ( stdComm_UpdatePlayers(0) )
    {
        SITHLOG_WARNING("Unable to UpdatePlayers List.\n");
        return 0;
    }

    size_t numPlayers = stdComm_GetNumPlayers();
    SITHCONSOLE_PRINTF("%d players in session", numPlayers);

    for ( size_t playerNum = 0; playerNum < numPlayers; ++playerNum )
    {
        DPID playerID = stdComm_GetPlayerID(playerNum);
        int idx = sithMulti_GetPlayerIndexByID(playerID);

        SITHCONSOLE_PRINTF("Player %x (%S) is in the session", playerID, sithPlayer_g_aPlayers[idx].awName);
    }

    return 1;
}

int J3DAPI sithCommand_Fly(const SithConsoleCommand* pFunc, const char* pArg)
{
    J3D_UNUSED(pFunc);
    J3D_UNUSED(pArg);

    if ( !sithWorld_g_pCurrentWorld || !sithWorld_g_pCurrentWorld->pLocalPlayer )
    {
        sithConsole_PrintString("No world.");
        return 0;
    }

    SithThing* pPlayer = sithWorld_g_pCurrentWorld->pLocalPlayer;
    if ( pPlayer->moveType != SITH_MT_PHYSICS )
    {
        sithConsole_PrintString("Not physics thing.");
        return 0;
    }

    if ( !pArg )
    {
        SITHCONSOLE_PRINTF("fly %s", (pPlayer->moveInfo.physics.flags & SITH_PF_FLY) != 0 ? "on" : "off");
        return 0;
    }

    bool bEnable;
    if ( !sithCommand_ParseBool(pArg, &bEnable) )
    {
        sithConsole_PrintString("Invalid argument!");
        return 0;
    }

    if ( bEnable )
    {
        pPlayer->moveInfo.physics.flags |= SITH_PF_FLY;
        pPlayer->moveInfo.physics.flags &= ~SITH_PF_USEGRAVITY;
    }
    else
    {
        pPlayer->moveInfo.physics.flags &= ~SITH_PF_FLY;
        pPlayer->moveInfo.physics.flags |= SITH_PF_USEGRAVITY;
    }

    return 1;
}

int J3DAPI sithCommand_NoClip(const SithConsoleCommand* pFunc, const char* pArg)
{
    J3D_UNUSED(pFunc);
    J3D_UNUSED(pArg);

    if ( !sithWorld_g_pCurrentWorld || !sithWorld_g_pCurrentWorld->pLocalPlayer )
    {
        sithConsole_PrintString("No world.");
        return 0;
    }

    SithThing* pPlayer = sithWorld_g_pCurrentWorld->pLocalPlayer;

    if ( !pArg )
    {
        SITHCONSOLE_PRINTF("noclip %s", pPlayer->collide.type == 0 ? "on" : "off");
        return 0;
    }

    bool bEnable;
    if ( !sithCommand_ParseBool(pArg, &bEnable) )
    {
        sithConsole_PrintString("Invalid argument!");
        return 0;
    }

    if ( bEnable )
    {
        pPlayer->collide.type = 0;
    }
    else
    {
        pPlayer->collide.type = 1;
    }

    return 1;
}

int J3DAPI sithCommand_IMP1(const SithConsoleCommand* pFunc, const char* pArg)
{
    J3D_UNUSED(pFunc);
    J3D_UNUSED(pArg);

    if ( !sithPlayer_g_pLocalPlayerThing )
    {
        return 0;
    }

    sithInventory_SetInventory(sithPlayer_g_pLocalPlayerThing, SITHWEAPON_IMP1, 1.0f);
    sithInventory_SetInventoryAvailable(sithPlayer_g_pLocalPlayerThing, SITHWEAPON_IMP1, 1);
    return 1;
}

int J3DAPI sithCommand_IMP2(const SithConsoleCommand* pFunc, const char* pArg)
{
    J3D_UNUSED(pFunc);
    J3D_UNUSED(pArg);

    if ( !sithPlayer_g_pLocalPlayerThing )
    {
        return 0;
    }

    sithInventory_SetInventory(sithPlayer_g_pLocalPlayerThing, SITHWEAPON_IMP2, 1.0f);
    sithInventory_SetInventoryAvailable(sithPlayer_g_pLocalPlayerThing, SITHWEAPON_IMP2, 1);
    return 1;
}

int J3DAPI sithCommand_IMP3(const SithConsoleCommand* pFunc, const char* pArg)
{
    J3D_UNUSED(pFunc);
    J3D_UNUSED(pArg);

    if ( !sithPlayer_g_pLocalPlayerThing )
    {
        return 0;
    }

    sithInventory_SetInventory(sithPlayer_g_pLocalPlayerThing, SITHWEAPON_IMP3, 1.0f);
    sithInventory_SetInventoryAvailable(sithPlayer_g_pLocalPlayerThing, SITHWEAPON_IMP3, 1);
    return 1;
}

int J3DAPI sithCommand_IMP4(const SithConsoleCommand* pFunc, const char* pArg)
{
    J3D_UNUSED(pFunc);
    J3D_UNUSED(pArg);

    if ( !sithPlayer_g_pLocalPlayerThing )
    {
        return 0;
    }

    sithInventory_SetInventory(sithPlayer_g_pLocalPlayerThing, SITHWEAPON_IMP4, 1.0f);
    sithInventory_SetInventoryAvailable(sithPlayer_g_pLocalPlayerThing, SITHWEAPON_IMP4, 1);
    return 1;
}

int J3DAPI sithCommand_IMP5(const SithConsoleCommand* pFunc, const char* pArg)
{
    J3D_UNUSED(pFunc);
    J3D_UNUSED(pArg);

    if ( !sithPlayer_g_pLocalPlayerThing )
    {
        return 0;
    }

    sithInventory_SetInventory(sithPlayer_g_pLocalPlayerThing, SITHWEAPON_IMP5, 1.0f);
    sithInventory_SetInventoryAvailable(sithPlayer_g_pLocalPlayerThing, SITHWEAPON_IMP5, 1);
    return 1;
}

int J3DAPI sithCommand_ElectroMan(const SithConsoleCommand* pFunc, const char* pArg)
{
    J3D_UNUSED(pFunc);
    J3D_UNUSED(pArg);

    if ( !sithPlayer_g_pLocalPlayerThing )
    {
        return 0;
    }

    sithPlayer_g_pLocalPlayerThing->thingInfo.actorInfo.flags |= SITH_AF_ELECTRICWHIP;
    sithInventory_SetInventory(sithPlayer_g_pLocalPlayerThing, SITHWEAPON_WHIP, 25.0f);
    rdMaterial* pMat = sithMaterial_GetMaterialByIndex(SITHWORLD_STATICINDEX(326)); // 0x8146
    if ( pMat )
    {
        pMat->curCelNum = 1;
    }

    return 1;
}

int J3DAPI sithCommand_CameraInfo(const SithConsoleCommand* pFunc, const char* pArg)
{
    J3D_UNUSED(pFunc);
    J3D_UNUSED(pArg);

    if ( !sithCamera_g_pCurCamera )
    {
        sithConsole_PrintString("No current camera.");
        return 0;
    }

    SithCamera* pCam = sithCamera_g_pCurCamera;
    SITHCONSOLE_PRINTF(
        "POS: X %2.4f Y %2.4f Z %2.4f FOV %2.2f",
        sithCamera_g_pCurCamera->orient.dvec.x,
        sithCamera_g_pCurCamera->orient.dvec.y,
        sithCamera_g_pCurCamera->orient.dvec.z,
        sithCamera_g_pCurCamera->fov
    );

    SITHCONSOLE_PRINTF("ORIENT: P %2.4f Y %2.4f R %2.4f", pCam->lookPYR.x, pCam->lookPYR.y, pCam->lookPYR.z);
    return 1;
}

int J3DAPI sithCommand_FOV(const SithConsoleCommand* pFunc, const char* pArg)
{
    J3D_UNUSED(pFunc);

    if ( !pArg )
    {
        SITHCONSOLE_PRINTF("Current FOV: %2.2f", sithCamera_g_pCurCamera->fov);
        return 0;
    }

    float fov;
    if ( sscanf_s(pArg, "%f", &fov) != 1 )
    {
        sithConsole_PrintString("Invalid FOV.");
        return 0;
    }

    sithCamera_SetCameraFOV(sithCamera_g_pCurCamera, fov);
    return 1;
}

int J3DAPI sithCommand_TwistJoint(const SithConsoleCommand* pFunc, const char* pArg)
{
    J3D_UNUSED(pFunc);

    if ( !pArg )
    {
        sithConsole_PrintString("Format: tj JOINTNUM AXIS DEGREES");
        return 0;
    }

    size_t joinNum;
    size_t axis;
    float degrees;
    if ( sscanf_s(pArg, "%d %d %f", &joinNum, &axis, &degrees) < 3 )
    {
        sithConsole_PrintString("Format: tj JOINTNUM AXIS DEGREES");
        return 0;
    }

    if ( sithPlayer_g_pLocalPlayerThing && axis < 3
        && joinNum < sithPlayer_g_pLocalPlayerThing->renderData.data.pModel3->numHNodes )
    {
        *(&sithPlayer_g_pLocalPlayerThing->renderData.apTweakedAngles[joinNum].x + axis) = degrees;
        return 1;
    }

    return 0;
}

int J3DAPI sithCommand_EnableInventoryWeapons(const SithConsoleCommand* pFunc, const char* pArg)
{
    J3D_UNUSED(pFunc);
    J3D_UNUSED(pArg);

    if ( !sithPlayer_g_pLocalPlayerThing )
    {
        return 0;
    }

    sithInventory_SetSwimmingInventory(sithPlayer_g_pLocalPlayerThing, 1);
    return 1;
}

int J3DAPI sithCommand_CogTrace(const SithConsoleCommand* pFunc, const char* pStrId)
{
    J3D_UNUSED(pFunc);
    if ( !sithWorld_g_pCurrentWorld )
    {
        sithConsole_PrintString("no world open.");
        return 0;
    }

    if ( !pStrId )
    {
        sithConsole_PrintString("syntax error.");
        return 0;
    }

    size_t id = atoi(pStrId);
    if ( id >= sithWorld_g_pCurrentWorld->numCogs )
    {
        sithConsole_PrintString("cog id out of range.");
        return 0;
    }

    SithCog* pCog = &sithWorld_g_pCurrentWorld->aCogs[id];
    if ( (pCog->flags & SITHCOG_DEBUG) != 0 )
    {
        sithConsole_PrintString("Cog trace disabled.");
        pCog->flags &= ~SITHCOG_DEBUG;
    }
    else
    {
        sithConsole_PrintString("Cog trace enabled.");
        pCog->flags |= SITHCOG_DEBUG;
    }

    return 1;
}

int J3DAPI sithCommand_CogList(const SithConsoleCommand* pFunc, const char* pArg)
{
    J3D_UNUSED(pFunc);
    J3D_UNUSED(pArg);

    if ( !sithWorld_g_pCurrentWorld )
    {
        sithConsole_PrintString("No world.");
        return 0;
    }

    SITHCONSOLE_PRINTF("World cogs = %d.", sithWorld_g_pCurrentWorld->numCogs);

    for ( size_t cogNum = 0; cogNum < sithWorld_g_pCurrentWorld->numCogs; ++cogNum )
    {
        SithCog* pCog = &sithWorld_g_pCurrentWorld->aCogs[cogNum];
        STD_FORMAT(std_g_genBuffer, "%d: %-16s %-16s ", cogNum, pCog->aName, pCog->pScript->aName);

        size_t len = 0;
        if ( (pCog->flags & SITHCOG_DISABLED) != 0 )
        {
            len = strlen(std_g_genBuffer);
            stdUtil_Format(&std_g_genBuffer[len], STD_ARRAYLEN(std_g_genBuffer) - len, "(paused) ");
        }

        if ( (pCog->flags & SITHCOG_DEBUG) != 0 )
        {
            len = strlen(std_g_genBuffer);
            stdUtil_Format(&std_g_genBuffer[len], STD_ARRAYLEN(std_g_genBuffer) - len, "(trace)  ");
        }

        //len = strlen(std_g_genBuffer);
        //stdUtil_Format(&std_g_genBuffer[len], STD_ARRAYLEN(std_g_genBuffer) - len, byte_62B018);
        sithConsole_PrintString(std_g_genBuffer);
    }

    return 1;
}

int J3DAPI sithCommand_CogPause(const SithConsoleCommand* pFunc, const char* pArg)
{
    J3D_UNUSED(pFunc);

    if ( !sithWorld_g_pCurrentWorld )
    {
        sithConsole_PrintString("No world.");
        return 0;
    }

    if ( !pArg )
    {
        sithConsole_PrintString("syntax error.");
        return 0;
    }

    size_t idx = atoi(pArg);
    if ( idx >= sithWorld_g_pCurrentWorld->numCogs )
    {
        sithConsole_PrintString("cog id out of range.");
        return 0;
    }

    SithCog* pCog = &sithWorld_g_pCurrentWorld->aCogs[idx];
    if ( (pCog->flags & SITHCOG_DISABLED) != 0 )
    {
        sithConsole_PrintString("Cog enabled.");
        pCog->flags &= ~SITHCOG_DISABLED;
    }
    else
    {
        sithConsole_PrintString("Cog disabled.");
        pCog->flags |= SITHCOG_DISABLED;
    }

    return 1;
}

int J3DAPI sithCommand_PauseAllCogs(const SithConsoleCommand* pFunc, const char* pArg)
{
    J3D_UNUSED(pFunc);

    if ( !sithWorld_g_pCurrentWorld )
    {
        sithConsole_PrintString("No world.");
        return 0;
    }

    if ( !pArg )
    {
        sithConsole_PrintString("On or off?");
        return 0;
    }

    bool bDisable;
    if ( sithCommand_ParseBool(pArg, &bDisable) )
    {
        if ( bDisable )
        {
            sithConsole_PrintString("Cogs disabled.");
            for ( size_t i = 0; i < sithWorld_g_pCurrentWorld->numCogs; ++i )
            {
                sithWorld_g_pCurrentWorld->aCogs[i].flags |= SITHCOG_DISABLED;
            }
        }
        else
        {
            sithConsole_PrintString("Cogs enabled.");
            for ( size_t i = 0; i < sithWorld_g_pCurrentWorld->numCogs; ++i )
            {
                sithWorld_g_pCurrentWorld->aCogs[i].flags &= ~SITHCOG_DISABLED;
            }
        }
    }

    return 1;
}

int J3DAPI sithCommand_CompareMatInfos(const SithCommandMatInfo* pMatInfo1, const SithCommandMatInfo* pMatInfo2)
{
    return pMatInfo2->memPerSurf - pMatInfo1->memPerSurf;
}

int J3DAPI sithCommand_MatList(const SithConsoleCommand* pFunc, const char* pArg)
{
    J3D_UNUSED(pFunc);
    J3D_UNUSED(pArg);

    SithWorld* pWorld = sithWorld_g_pCurrentWorld;
    if ( !sithWorld_g_pCurrentWorld )
    {
        sithConsole_PrintString("No world.");
        return 0;
    }

    SithCommandMatInfo* aMatInfo = (SithCommandMatInfo*)STDMALLOC(sizeof(SithCommandMatInfo) * sithWorld_g_pCurrentWorld->numMaterials);
    SITH_ASSERTREL(aMatInfo);

    for ( size_t i = 0; i < pWorld->numMaterials; ++i )
    {
        aMatInfo[i].num = i;
        aMatInfo[i].surfCount = 0;
    }

    for ( size_t i = 0; i < pWorld->numSurfaces; ++i )
    {
        SithSurface* pSurf = &pWorld->aSurfaces[i];
        if ( pSurf->face.pMaterial )
        {
            size_t idx = pSurf->face.pMaterial - pWorld->aMaterials;
            ++aMatInfo[idx].surfCount;
        }
    }

    for ( size_t i = 0; i < pWorld->numMaterials; ++i )
    {
        if ( aMatInfo[i].surfCount )
        {
            aMatInfo[i].memUsage   = rdMaterial_GetMaterialMemUsage(&pWorld->aMaterials[aMatInfo[i].num]);
            aMatInfo[i].memPerSurf = aMatInfo[i].memUsage / aMatInfo[i].surfCount;
        }
        else
        {
            aMatInfo[i].memPerSurf = 0;
            aMatInfo[i].memUsage   = 0;
        }
    }

    qsort(aMatInfo, pWorld->numMaterials, sizeof(SithCommandMatInfo), sithCommand_CompareMatInfos);
    for ( int i = pWorld->numMaterials - 1; i >= 0; --i )
    {
        if ( aMatInfo[i].memPerSurf )
        {
            SITHCONSOLE_PRINTF(
                "%-16s: %d faces, %d bytes, %d bytes/face.",
                pWorld->aMaterials[aMatInfo[i].num].aName,
                aMatInfo[i].surfCount,
                aMatInfo[i].memUsage,
                aMatInfo[i].memPerSurf
            );
        }
    }

    return 1;
}

int J3DAPI sithCommand_Activate(const SithConsoleCommand* pFunc, const char* pArg)
{
    J3D_UNUSED(pFunc);

    SithWorld* pWorld = sithWorld_g_pCurrentWorld;
    if ( !sithWorld_g_pCurrentWorld || !pWorld->pLocalPlayer )
    {
        sithConsole_PrintString("No world");
        return 0;
    }

    int bin;
    if ( sscanf_s(pArg, "%d", &bin) < 1 )
    {
        return 0;
    }
    else if ( bin < 0 )
    {
        return 0;
    }

    SithThing* pThing = pWorld->pLocalPlayer;
    SithInventoryType* pItem = &sithInventory_g_aTypes[bin];
    if ( pItem->pCog && sithInventory_IsInventoryAvailable(pThing, bin) )
    {
        sithCog_SendMessage(sithInventory_g_aTypes[bin].pCog, SITHCOG_MSG_ACTIVATE, SITHCOG_SYM_REF_NONE, bin, SITHCOG_SYM_REF_THING, pThing->idx, 0);
        return 1;
    }

    return 0;
}

int J3DAPI sithCommand_StopSounds(const SithConsoleCommand* pFunc, const char* pArg)
{
    J3D_UNUSED(pFunc);
    J3D_UNUSED(pArg);

    if ( !sithWorld_g_pCurrentWorld )
    {
        sithConsole_PrintString("No world open.");
        return 0;
    }

    sithSoundMixer_StopAll();
    return 1;
}

int J3DAPI sithCommand_WpDraw(const SithConsoleCommand* pFunc, const char* pArg)
{
    J3D_UNUSED(pFunc);
    J3D_UNUSED(pArg);

    sithAIUtil_g_bRenderAIWpnts = sithAIUtil_g_bRenderAIWpnts == 0;
    return 1;
}

int J3DAPI sithCommand_Print(const SithConsoleCommand* pFunc, const char* pArg)
{
    J3D_UNUSED(pFunc);
    if ( pArg )
    {
        bool bEnable;
        if ( !sithCommand_ParseBool(pArg, &bEnable) )
        {
            sithConsole_PrintString("Invalid argument!");
            return 0;
        }

        sithCogFunction_EnablePrint(bEnable);
    }
    else
    {
        SITHCONSOLE_PRINTF("print %s", sithCogFunction_IsPrintEnabled() ? "on" : "off");
    }
    return 1;
}

const char* J3DAPI sithCommand_CipherText(const char* pText)
{
    // Function goes over all characters of text and xor each char with 34 (22h)
    // def decrypt_cheat_text(enc_cheat):
    //      cheat = ""
    //      for c in enc_cheat:
    //          cheat += chr(ord(c) ^ 34)
    //      return cheat

    size_t len = strlen(pText);
    size_t i = 0;
    for ( ; i < len; ++i )
    {
        aCipherTextBuf[i] = pText[i] ^ 34;
    }
    aCipherTextBuf[i] = 0;

    return aCipherTextBuf;
}

void sithCommand_InstallHooks(void)
{
    J3D_HOOKFUNC(sithCommand_RegisterCommands);
    J3D_HOOKFUNC(sithCommand_DebugMode);
    J3D_HOOKFUNC(sithCommand_Wuss);
    J3D_HOOKFUNC(sithCommand_Fixme);
    J3D_HOOKFUNC(sithCommand_Ouch);
    J3D_HOOKFUNC(sithCommand_Coords);
    J3D_HOOKFUNC(sithCommand_Memory);
    J3D_HOOKFUNC(sithCommand_DynamicMemory);
    J3D_HOOKFUNC(sithCommand_MemoryDump);
    J3D_HOOKFUNC(sithCommand_CipherText);
}

void sithCommand_ResetGlobals(void)
{}

void sithCommand_RegisterCommands(void)
{
    // taklit_marion
    sithConsole_RegisterCommand(sithCommand_DebugMode, sithCommand_CipherText("VCINKV}OCPKML"), 4);

    // urgon_elsa
    sithConsole_RegisterCommand(sithCommand_Wuss, sithCommand_CipherText("WPEML}GNQC"), 0);

    // azerim_sophia
    sithConsole_RegisterCommand(sithCommand_Ouch, sithCommand_CipherText("CXGPKO}QMRJKC"), 0);

    sithConsole_RegisterCommand(sithCommand_Fixme, "fixme", 0);
    sithConsole_RegisterCommand(sithCommand_DebugMode, "invul", 4 | SITHCONSOLE_DEVMODE); // Added: From debug version
    sithConsole_RegisterCommand(sithCommand_Wuss, "wuss", SITHCONSOLE_DEVMODE); // Added: From debug version
    sithConsole_RegisterCommand(sithCommand_Ouch, "ouch", SITHCONSOLE_DEVMODE); // Added: From debug version
    sithConsole_RegisterCommand(sithCommand_Warp, "warp", SITHCONSOLE_DEVMODE); // Added: From debug version

    sithConsole_RegisterCommand(sithCommand_Jump, "jump", SITHCONSOLE_DEVMODE); // Added: From debug version
    sithConsole_RegisterCommand(sithCommand_MakeMePirate, "makemeapirate", 0);
    sithConsole_RegisterCommand(sithCommand_Coords, "coords", 0);

    sithConsole_RegisterCommand(sithCommand_Memory, "mem", 0);
    sithConsole_RegisterCommand(sithCommand_DynamicMemory, "dynamicmem", 0);
    sithConsole_RegisterCommand(sithCommand_MemoryDump, "memdump", 0);

    sithConsole_RegisterCommand(sithCommand_IMP1, "imp1", SITHCONSOLE_DEVMODE); // Added: From debug version
    sithConsole_RegisterCommand(sithCommand_IMP2, "imp2", SITHCONSOLE_DEVMODE); // Added: From debug version
    sithConsole_RegisterCommand(sithCommand_IMP3, "imp3", SITHCONSOLE_DEVMODE); // Added: From debug version
    sithConsole_RegisterCommand(sithCommand_IMP4, "imp4", SITHCONSOLE_DEVMODE); // Added: From debug version
    sithConsole_RegisterCommand(sithCommand_IMP5, "imp5", SITHCONSOLE_DEVMODE); // Added: From debug version

    sithConsole_RegisterCommand(sithCommand_Fly, "fly", SITHCONSOLE_DEVMODE); // Added: From debug version
    sithConsole_RegisterCommand(sithCommand_NoClip, "noclip", SITHCONSOLE_DEVMODE); // Added

    sithConsole_RegisterCommand(sithCommand_Players, "players", SITHCONSOLE_DEVMODE); // Added: From debug version
    sithConsole_RegisterCommand(sithCommand_PingPlayer, "ping", SITHCONSOLE_DEVMODE); // Added: From debug version
    sithConsole_RegisterCommand(sithCommand_Kick, "kick", SITHCONSOLE_DEVMODE); // Added: From debug version
    sithConsole_RegisterCommand(sithCommand_Tick, "tick", SITHCONSOLE_DEVMODE); // Added: From debug version
    sithConsole_RegisterCommand(sithCommand_Session, "session", SITHCONSOLE_DEVMODE); // Added: From debug version

    sithConsole_RegisterCommand(sithCommand_ElectroMan, "electroman", SITHCONSOLE_DEVMODE); // Added: From debug version
    sithConsole_RegisterCommand(sithCommand_DebugMode, "trackshots", 2 | SITHCONSOLE_DEVMODE); // Added: From debug version

    sithConsole_RegisterCommand(sithCommand_CameraInfo, "caminfo", SITHCONSOLE_DEVMODE); // Added: From debug version
    sithConsole_RegisterCommand(sithCommand_FOV, "fov", SITHCONSOLE_DEVMODE); // Added: From debug version

    sithConsole_RegisterCommand(sithCommand_TwistJoint, "tj", SITHCONSOLE_DEVMODE); // Added: From debug version
    sithConsole_RegisterCommand(sithCommand_EnableInventoryWeapons, "weapons", SITHCONSOLE_DEVMODE); // Added: From debug version

    sithConsole_RegisterCommand(sithConsole_Help, "help", 0); // Added: From debug version

    sithConsole_RegisterCommand(sithCommand_DebugMode, "disableai", 0 | SITHCONSOLE_DEVMODE); // Added: From debug version
    sithConsole_RegisterCommand(sithCommand_DebugMode, "notarget", 5 | SITHCONSOLE_DEVMODE); // Added: From debug version
    sithConsole_RegisterCommand(sithCommand_DebugMode, "disablepuppet", 1 | SITHCONSOLE_DEVMODE); // Added: From debug version

    sithConsole_RegisterCommand(sithCommand_CogTrace, "cogtrace", SITHCONSOLE_DEVMODE); // Added: From debug version
    sithConsole_RegisterCommand(sithCommand_CogList, "coglist", SITHCONSOLE_DEVMODE); // Added: From debug version
    sithConsole_RegisterCommand(sithCog_CogStatus, "cogstatus", SITHCONSOLE_DEVMODE); // Added: From debug version

    sithConsole_RegisterCommand(sithCommand_DebugMode, "noaishots", 3 | SITHCONSOLE_DEVMODE); // Added: From debug version
    sithConsole_RegisterCommand(sithAI_AIStatus, "aistatus", SITHCONSOLE_DEVMODE); // Added: From debug version
    sithConsole_RegisterCommand(sithAI_AIList, "ailist", SITHCONSOLE_DEVMODE); // Added: From debug version

    sithConsole_RegisterCommand(sithCommand_CogPause, "cogpause", SITHCONSOLE_DEVMODE); // Added: From debug version
    sithConsole_RegisterCommand(sithCommand_PauseAllCogs, "pauseallcogs", SITHCONSOLE_DEVMODE); // Added: From debug version

    sithConsole_RegisterCommand(sithCommand_MatList, "matlist", SITHCONSOLE_DEVMODE); // Added: From debug version
    sithConsole_RegisterCommand(sithCommand_Activate, "activate", SITHCONSOLE_DEVMODE); // Added: From debug version
    sithConsole_RegisterCommand(sithCommand_DebugMode, "slowmo", 6 | SITHCONSOLE_DEVMODE); // Added: From debug version
    sithConsole_RegisterCommand(sithCommand_StopSounds, "stopsounds", SITHCONSOLE_DEVMODE); // Added: From debug version
    sithConsole_RegisterCommand(sithCommand_WpDraw, "wpdraw", SITHCONSOLE_DEVMODE); // Added: From debug version
    sithConsole_RegisterCommand(sithCommand_Print, "print", SITHCONSOLE_DEVMODE); // Added: New command
}
#include "sithMulti.h"
#include <j3dcore/j3dhook.h>
#include <sith/RTI/symbols.h>

#include <rdroid/Math/rdVector.h>

#include <sith/Cog/sithCog.h>
#include <sith/Devices/sithComm.h>
#include <sith/Devices/sithConsole.h>
#include <sith/Devices/sithSoundMixer.h>
#include <sith/Dss/sithDSS.h>
#include <sith/Dss/sithDSSThing.h>
#include <sith/Engine/sithCollision.h>
#include <sith/Gameplay/sithEvent.h>
#include <sith/Gameplay/sithPlayer.h>
#include <sith/Gameplay/sithTime.h>
#include <sith/Main/sithMain.h>
#include <sith/Main/sithString.h>
#include <sith/World/sithModel.h>
#include <sith/World/sithSector.h>
#include <sith/World/sithSurface.h>
#include <sith/World/sithThing.h>
#include <sith/World/sithWorld.h>

#include <std/General/stdPlatform.h>
#include <std/General/stdUtil.h>
#include <std/Win95/stdComm.h>

#define sithMulti_tickRate J3D_DECL_FAR_VAR(sithMulti_tickRate, size_t)
#define sithMulti_numUpdatedSurfaces J3D_DECL_FAR_VAR(sithMulti_numUpdatedSurfaces, int)
#define sithMulti_lastUpdateIdx J3D_DECL_FAR_VAR(sithMulti_lastUpdateIdx, int)
#define sithMulti_playerWelcomeState J3D_DECL_FAR_VAR(sithMulti_playerWelcomeState, int)
#define sithMulti_numUpdatedSectors J3D_DECL_FAR_VAR(sithMulti_numUpdatedSectors, int)
#define sithMulti_aRemovedStaticThings J3D_DECL_FAR_ARRAYVAR(sithMulti_aRemovedStaticThings, int(*)[256])
#define sithMulti_curWelcomePlayerNum J3D_DECL_FAR_VAR(sithMulti_curWelcomePlayerNum, unsigned int)
#define sithMulti_numUpdatedThings J3D_DECL_FAR_VAR(sithMulti_numUpdatedThings, int)
#define sithMulti_quitGameState J3D_DECL_FAR_VAR(sithMulti_quitGameState, int)
#define sithMulti_checksum J3D_DECL_FAR_VAR(sithMulti_checksum, int)
#define sithMulti_bWelcomingPlayer J3D_DECL_FAR_VAR(sithMulti_bWelcomingPlayer, int)
#define sithMulti_newPlayerId J3D_DECL_FAR_VAR(sithMulti_newPlayerId, DPID)
#define sithMulti_bSyncScores J3D_DECL_FAR_VAR(sithMulti_bSyncScores, int)
#define sithMulti_pfNewPlayerJoinedCallback J3D_DECL_FAR_VAR(sithMulti_pfNewPlayerJoinedCallback, SithMultiNewPlayerJoinedCallback)
#define sithMulti_msecPingStartTime J3D_DECL_FAR_VAR(sithMulti_msecPingStartTime, unsigned int)
#define sithMulti_msecQuitGameTime J3D_DECL_FAR_VAR(sithMulti_msecQuitGameTime, unsigned int)
#define sithMulti_numRemovedStaticThings J3D_DECL_FAR_VAR(sithMulti_numRemovedStaticThings, size_t)
#define sithMulti_msecLastSyncScoreTime J3D_DECL_FAR_VAR(sithMulti_msecLastSyncScoreTime, unsigned int)
#define sithMulti_msecWelcomeUpdateInterval J3D_DECL_FAR_VAR(sithMulti_msecWelcomeUpdateInterval, unsigned int)
#define sithMulti_dword_17F10EC J3D_DECL_FAR_VAR(sithMulti_dword_17F10EC, int)

static void sithMulti_FinishWelcomingPlayer(void);

void sithMulti_InstallHooks(void)
{
    // Uncomment only lines for functions that have full definition and doesn't call original function (non-thunk functions)

    J3D_HOOKFUNC(sithMulti_CloseGame);
    J3D_HOOKFUNC(sithMulti_CheckPlayers);
    J3D_HOOKFUNC(sithMulti_ProcessPlayerLost);
    J3D_HOOKFUNC(sithMulti_RemovePlayer);
    J3D_HOOKFUNC(sithMulti_SendWelcome);
    J3D_HOOKFUNC(sithMulti_ProcessWelcome);
    J3D_HOOKFUNC(sithMulti_ProcessPlayerJoin);
    J3D_HOOKFUNC(sithMulti_SyncPlayers);
    J3D_HOOKFUNC(sithMulti_ProcessSyncPlayers);
    J3D_HOOKFUNC(sithMulti_ProcessJoinRequest);
    J3D_HOOKFUNC(sithMulti_FinishJoining);
    J3D_HOOKFUNC(sithMulti_ProcessChat);
    J3D_HOOKFUNC(sithMulti_ProcessPing);
    J3D_HOOKFUNC(sithMulti_ProcessPong);
    J3D_HOOKFUNC(sithMulti_QuitPlayer);
    J3D_HOOKFUNC(sithMulti_ProcessQuit);
    J3D_HOOKFUNC(sithMulti_Update);
    J3D_HOOKFUNC(sithMulti_SyncScores);
    J3D_HOOKFUNC(sithMulti_GetPlayerIndexByID);
    J3D_HOOKFUNC(sithMulti_ProcessKilledPlayer);
    J3D_HOOKFUNC(sithMulti_QuitGame);
    J3D_HOOKFUNC(sithMulti_Respawn);
    J3D_HOOKFUNC(sithMulti_RemoveStaticThing);
    J3D_HOOKFUNC(sithMulti_UpdateSurfaces);
    J3D_HOOKFUNC(sithMulti_UpdateSectors);
    J3D_HOOKFUNC(sithMulti_UpdateThings);
    J3D_HOOKFUNC(sithMulti_UpdateRemovals);
    J3D_HOOKFUNC(sithMulti_StartWelcomingPlayer);
    J3D_HOOKFUNC(sithMulti_StopWelcomingPlayer);
}

void sithMulti_ResetGlobals(void)
{
    int sithMulti_tickRate_tmp = 70;
    memcpy(&sithMulti_tickRate, &sithMulti_tickRate_tmp, sizeof(sithMulti_tickRate));

    memset(&sithMulti_numUpdatedSurfaces, 0, sizeof(sithMulti_numUpdatedSurfaces));
    memset(&sithMulti_lastUpdateIdx, 0, sizeof(sithMulti_lastUpdateIdx));
    memset(&sithMulti_playerWelcomeState, 0, sizeof(sithMulti_playerWelcomeState));
    memset(&sithMulti_numUpdatedSectors, 0, sizeof(sithMulti_numUpdatedSectors));
    memset(&sithMulti_aRemovedStaticThings, 0, sizeof(sithMulti_aRemovedStaticThings));
    memset(&sithMulti_curWelcomePlayerNum, 0, sizeof(sithMulti_curWelcomePlayerNum));
    memset(&sithMulti_numUpdatedThings, 0, sizeof(sithMulti_numUpdatedThings));
    memset(&sithMulti_quitGameState, 0, sizeof(sithMulti_quitGameState));
    memset(&sithMulti_checksum, 0, sizeof(sithMulti_checksum));
    memset(&sithMulti_bWelcomingPlayer, 0, sizeof(sithMulti_bWelcomingPlayer));
    memset(&sithMulti_newPlayerId, 0, sizeof(sithMulti_newPlayerId));
    memset(&sithMulti_bSyncScores, 0, sizeof(sithMulti_bSyncScores));
    memset(&sithMulti_pfNewPlayerJoinedCallback, 0, sizeof(sithMulti_pfNewPlayerJoinedCallback));
    memset(&sithMulti_msecPingStartTime, 0, sizeof(sithMulti_msecPingStartTime));
    memset(&sithMulti_msecQuitGameTime, 0, sizeof(sithMulti_msecQuitGameTime));
    memset(&sithMulti_numRemovedStaticThings, 0, sizeof(sithMulti_numRemovedStaticThings));
    memset(&sithMulti_msecLastSyncScoreTime, 0, sizeof(sithMulti_msecLastSyncScoreTime));
    memset(&sithMulti_msecWelcomeUpdateInterval, 0, sizeof(sithMulti_msecWelcomeUpdateInterval));
    memset(&sithMulti_g_serverId, 0, sizeof(sithMulti_g_serverId));
    memset(&sithMulti_dword_17F10EC, 0, sizeof(sithMulti_dword_17F10EC));
    memset(&sithMulti_g_message, 0, sizeof(sithMulti_g_message));
}

void sithMulti_CloseGame()
{
    if ( stdComm_IsGameHost() && sithMulti_bWelcomingPlayer )
    {
        sithMulti_StopWelcomingPlayer(1);
    }

    sithMessage_g_outputstream &= ~SITHMESSAGE_STREAM_NET;
    sithMessage_g_inputstream &= ~SITHMESSAGE_STREAM_NET;
    sithEvent_RegisterTask(SITHMULTI_CHECKPLAYER_TASKID, NULL, 0, SITHEVENT_TASKDISABLED);
    sithMessage_CloseGame();

    memset(&sithMulti_g_serverId, 0, sizeof(sithMulti_g_serverId));
    memset(&sithMulti_dword_17F10EC, 0, sizeof(sithMulti_dword_17F10EC));
}

int J3DAPI sithMulti_CheckPlayers(int msecTime, SithEventParams* pParam)
{
    J3D_UNUSED(msecTime);
    J3D_UNUSED(pParam);

    if ( sithWorld_g_pCurrentWorld && sithPlayer_g_pLocalPlayerThing && (sithMain_g_sith_mode.subModeFlags & SITH_SUBMODE_SYNC) == 0 )
    {
        sithDSSThing_Pos(sithPlayer_g_pLocalPlayerThing, SITHMESSAGE_SENDTOJOINEDPLAYERS, 0);
    }

    if ( stdComm_IsGameHost() )
    {
        for ( size_t i = 0; i < sithPlayer_g_numPlayers; ++i )
        {
            if ( sithPlayer_g_aPlayers[i].playerNetId != sithMessage_g_localPlayerId
                && (sithPlayer_g_aPlayers[i].flags & SITH_PLAYER_JOINEDGAME) != 0
                && sithTime_g_msecGameTime > sithPlayer_g_aPlayers[i].msecLastCommTime + 45000 )
            {
                SITHLOG_ERROR("Haven't heard from player %d, kicking them out of the game.\n", i);
                sithMulti_QuitPlayer(sithPlayer_g_aPlayers[i].playerNetId);
                sithMulti_RemovePlayer(i);
            }
        }

        if ( sithMulti_msecLastSyncScoreTime + 10000 < sithTime_g_clockTime )
        {
            sithMulti_msecLastSyncScoreTime = sithTime_g_clockTime;
            sithMulti_SyncScores();
        }
    }
    else if ( sithTime_g_msecGameTime > sithPlayer_g_aPlayers[0].msecLastCommTime + 45000 )
    {
        sithPlayer_g_aPlayers[0].msecLastCommTime = sithTime_g_msecGameTime;
        SITHLOG_ERROR("Haven't heard from server, exiting the game.\n");
        sithMulti_RemovePlayer(0);
    }

    return 1;
}

void J3DAPI sithMulti_ProcessPlayerLost(DPID idPlayer)
{
    if ( sithMulti_bWelcomingPlayer && idPlayer == sithMulti_newPlayerId )
    {
        sithMulti_StopWelcomingPlayer(1);
    }

    int playerNum = sithMulti_GetPlayerIndexByID(idPlayer);
    SITHLOG_ERROR("Player Lost: id=%x, playerNum=%d.\n", idPlayer, playerNum);
    if ( playerNum < 0 )
    {
        return;
    }

    sithMulti_RemovePlayer(playerNum);
}

void J3DAPI sithMulti_RemovePlayer(unsigned int playerNum)
{
    SITH_ASSERTREL(playerNum < sithPlayer_g_numPlayers);
    SITHLOG_ERROR("Removing player %d.\n", playerNum);

    wchar_t awText[128];
    STD_WFORMAT(awText, sithString_GetString("%s_HAS_LEFT_THE_GAME"), sithPlayer_g_aPlayers[playerNum].awName);
    sithConsole_PrintWString(awText);

    if ( sithPlayer_g_aPlayers[playerNum].playerNetId == sithMulti_g_serverId )
    {
        sithConsole_PrintWString(sithString_GetString("SERVER_LEFT_GAME"));
        sithMulti_g_serverId = 0;
    }

    sithSoundMixer_StopSoundThing(sithPlayer_g_aPlayers[playerNum].pThing, 0);
    sithPlayer_HidePlayer(playerNum);

    if ( stdComm_IsGameHost() )
    {
        sithCog_BroadcastMessage(SITHCOG_MSG_LEAVE, SITHCOG_SYM_REF_THING, sithPlayer_g_aPlayers[playerNum].pThing->idx, SITHCOG_SYM_REF_NONE, playerNum);
    }
}

signed int J3DAPI sithMulti_SendWelcome(DPID idPlayer, int playerNum, DPID idTo)
{
    SITHDSS_STARTOUT(SITHDSS_WELCOME);
    SITHDSS_PUSHINT32(playerNum);
    SITHDSS_PUSHINT32(idPlayer);

    SITHLOG_STATUS("Sending game welcome of player %d to playerID %x.\n", playerNum, idTo);

    memset(pCurDSSMsgData_, 0, 32);
    stdUtil_WStringNumCopy((wchar_t*)pCurDSSMsgData_, 16, sithPlayer_g_aPlayers[playerNum].awName, 15);
    pCurDSSMsgData_ += 32;

    SITHDSS_ENDOUT;
    return sithMessage_SendMessage(&sithMulti_g_message, idTo, SITHMESSAGE_STREAM_NET, 1);
}

int J3DAPI sithMulti_ProcessWelcome(const SithMessage* pMsg)
{
    SITHDSS_STARTIN(pMsg);
    int playerNum = SITHDSS_POPINT32();
    DPID playerId = SITHDSS_POPUINT32();

    SITHLOG_STATUS("Processing welcome from player %d (id=%x).\n", playerNum, playerId);

    // TODO: [BUG] playerNum is read from the message and used as an index into sithPlayer_g_aPlayers without a range check
    stdUtil_WStringNumCopy(sithPlayer_g_aPlayers[playerNum].awName, STD_ARRAYLEN(sithPlayer_g_aPlayers[playerNum].awName), (const wchar_t*)pCurDSSMsgData_, 15);
    sithPlayer_g_aPlayers[playerNum].awName[15] = 0;
    pCurDSSMsgData_ += 32;

    if ( playerId == sithMessage_g_localPlayerId )
    {
        SITHLOG_STATUS("DSS::welcome packet addressed to me!.\n");
        if ( (sithMain_g_sith_mode.subModeFlags & SITH_SUBMODE_SYNC) != 0 )
        {
            SITHLOG_STATUS("Joining the game loop.\n");

            for ( size_t i = 0; i < sithPlayer_g_numPlayers; ++i )
            {
                sithPlayer_g_aPlayers[i].msecLastCommTime = sithTime_g_msecGameTime;
            }

            sithMain_g_sith_mode.subModeFlags &= ~SITH_SUBMODE_SYNC;
            sithThing_LoadPostProcess(sithWorld_g_pCurrentWorld);
            sithPlayer_ShowPlayer(playerNum, playerId);
            sithPlayer_SetLocalPlayer(playerNum);

            SITH_ASSERTREL(sithMulti_tickRate);
            sithEvent_RegisterTask(SITHMULTI_CHECKPLAYER_TASKID, sithMulti_CheckPlayers, sithMulti_tickRate, SITHEVENT_TASKINTERVAL);
            sithMessage_StopProcessMessages();
        }
        else
        {
            SITHLOG_STATUS("Got duplicate Welcome packet from host, ignored.\n");
        }
    }
    else
    {
        if ( (sithPlayer_g_aPlayers[playerNum].flags & SITH_PLAYER_JOINEDGAME) != 0 )
        {
            SITHLOG_STATUS("Received duplicate welcome packet from player %d (id=%x).\n", playerNum, playerId);
            return 1;
        }

        sithPlayer_ShowPlayer(playerNum, playerId);
        sithMulti_ProcessPlayerJoin(playerNum);
        if ( stdComm_IsGameHost() )
        {
            sithMulti_SyncScores();
        }
    }

    return 1;
}

void J3DAPI sithMulti_ProcessPlayerJoin(int playerNum)
{
    wchar_t awText[128];
    STD_WFORMAT(awText, sithString_GetString("%s_HAS_JOINED_THE_GAME"), sithPlayer_g_aPlayers[playerNum].awName);
    sithConsole_PrintWString(awText);

    sithPlayer_g_aPlayers[playerNum].msecLastCommTime = sithTime_g_msecGameTime;
    if ( stdComm_IsGameHost() )
    {
        sithCog_BroadcastMessage(SITHCOG_MSG_JOIN, SITHCOG_SYM_REF_THING, sithPlayer_g_aPlayers[playerNum].pThing->idx, SITHCOG_SYM_REF_NONE, playerNum);
    }

    if ( sithMulti_pfNewPlayerJoinedCallback )
    {
        sithMulti_pfNewPlayerJoinedCallback();
    }

    sithDSSThing_UpdateState(sithPlayer_g_pLocalPlayerThing, SITHMESSAGE_SENDTOJOINEDPLAYERS, SITHMESSAGE_STREAM_ALL);
}

void J3DAPI sithMulti_SyncPlayers(DPID idTo, uint32_t dpFlags)
{
    SITHDSS_STARTOUT(SITHDSS_SYNCPLAYERS);
    SITHDSS_PUSHUINT32(sithMulti_g_serverId);
    SITHDSS_PUSHUINT16((uint16_t)sithPlayer_g_numPlayers);

    for ( size_t i = 0; i < sithPlayer_g_numPlayers; ++i )
    {
        SITHDSS_PUSHUINT32(sithPlayer_g_aPlayers[i].flags);
        if ( (sithPlayer_g_aPlayers[i].flags & SITH_PLAYER_UNKNOWN_04) != 0 )
        {
            SITHDSS_PUSHUINT32(sithPlayer_g_aPlayers[i].playerNetId);

            char aName[64];
            stdUtil_ToAStringEx(aName, sithPlayer_g_aPlayers[i].awName, 15);
            aName[15] = 0;

            memset(pCurDSSMsgData_, 0, 16);
            stdUtil_StringNumCopy((char*)pCurDSSMsgData_, 16, aName, 15);
            pCurDSSMsgData_ += 16;
        }
    }

    SITHDSS_ENDOUT;
    sithMessage_SendMessage(&sithMulti_g_message, idTo, SITHMESSAGE_STREAM_NET, dpFlags);

    for ( size_t i = 0; i < sithPlayer_g_numPlayers; ++i )
    {
        if ( sithPlayer_g_aPlayers[i].playerNetId && sithMessage_g_localPlayerId && sithPlayer_g_aPlayers[i].playerNetId != sithMessage_g_localPlayerId )
        {
            rdModel3* pModel = sithModel_Load("gen_vi.3do", 1);
            if ( !pModel )
            {
                SITHLOG_ERROR("Model %s not found, using default.\n", "gen_vi.3do");
            }

            sithThing_SetThingModel(sithPlayer_g_aPlayers[i].pThing, pModel);
        }
    }
}

signed int J3DAPI sithMulti_ProcessSyncPlayers(const SithMessage* pMsg)
{
    SITHDSS_STARTIN(pMsg);

    memset(&sithMulti_g_serverId, 0, sizeof(sithMulti_g_serverId));
    memset(&sithMulti_dword_17F10EC, 0, sizeof(sithMulti_dword_17F10EC));
    sithMulti_dword_17F10EC = stdPlatform_GetTimeMsec();

    sithMulti_g_serverId = SITHDSS_POPUINT32();

    // TODO: [BUG] numPlayers is read from the message and not limited to SITHPLAYER_MAX_PLAYERS, the loop below can write past sithPlayer_g_aPlayers
    sithPlayer_g_numPlayers = SITHDSS_POPUINT16();

    for ( size_t i = 0; i < sithPlayer_g_numPlayers; ++i )
    {
        SithPlayerFlag prevFlags = sithPlayer_g_aPlayers[i].flags;
        sithPlayer_g_aPlayers[i].flags = SITHDSS_POPUINT32();
        if ( (sithPlayer_g_aPlayers[i].flags & SITH_PLAYER_UNKNOWN_04) != 0 )
        {
            sithPlayer_g_aPlayers[i].playerNetId = SITHDSS_POPUINT32();

            if ( (sithPlayer_g_aPlayers[i].flags & SITH_PLAYER_JOINEDGAME) != 0 && (prevFlags & SITH_PLAYER_JOINEDGAME) == 0 && (sithMain_g_sith_mode.subModeFlags & SITH_SUBMODE_SYNC) == 0 )
            {
                SITHLOG_STATUS("Got a new player but we missed the welcome packet, apparently.\n");
                sithMulti_ProcessPlayerJoin(i);
            }
            else if ( !sithPlayer_g_aPlayers[i].playerNetId && (prevFlags & SITH_PLAYER_JOINEDGAME) != 0 && (sithMain_g_sith_mode.subModeFlags & SITH_SUBMODE_SYNC) == 0 )
            {
                SITHLOG_STATUS("Noticed that player %d was removed by the server.  Removing locally.\n", i);
                sithMulti_RemovePlayer(i);
            }

            char aName[64];
            STD_STRNCPY(aName, (const char*)pCurDSSMsgData_, 15);
            aName[15] = 0;
            pCurDSSMsgData_ += 16;

            stdUtil_ToWStringEx(sithPlayer_g_aPlayers[i].awName, aName, 15);
            sithPlayer_g_aPlayers[i].awName[15] = 0;
        }

        if ( sithPlayer_g_aPlayers[i].playerNetId && sithMessage_g_localPlayerId && sithPlayer_g_aPlayers[i].playerNetId != sithMessage_g_localPlayerId )
        {
            rdModel3* pModel = sithModel_Load("gen_vi.3do", 1);
            if ( !pModel )
            {
                SITHLOG_ERROR("Model %s not found, using default.\n", "gen_vi.3do");
                return 1;
            }

            sithThing_SetThingModel(sithPlayer_g_aPlayers[i].pThing, pModel);
        }
    }

    return 1;
}

int J3DAPI sithMulti_ProcessJoinRequest(const SithMessage* pMsg)
{
    SITHDSS_STARTIN(pMsg);
    DPID playerId = sithMessage_GetSenderID();

    if ( !stdComm_IsGameHost() )
    {
        SITHLOG_ERROR("Ignoring JoinRequest from player %d, ignoring.\n", playerId);
        return 1;
    }

    char aLevelName[64];
    STD_STRNCPY(aLevelName, (const char*)pCurDSSMsgData_, 63);
    aLevelName[63] = 0;
    pCurDSSMsgData_ += 64;

    if ( _strcmpi(aLevelName, sithWorld_g_pCurrentWorld->aName) )
    {
        SITHLOG_ERROR("Got join request but its for the wrong level %s.\n", aLevelName);
        sithMulti_FinishJoining(SITHMULTI_JOIN_WRONGLEVEL, 0.0f, playerId);
        return 1;
    }

    size_t i;
    for ( i = 0; i < sithPlayer_g_numPlayers; ++i )
    {
        if ( sithPlayer_g_aPlayers[i].playerNetId == playerId )
        {
            break;
        }
    }

    if ( i < sithPlayer_g_numPlayers )
    {
        SITHLOG_STATUS("Received knock from player %d, already in game.\n", playerId);
        sithMulti_SendWelcome(playerId, i, playerId);
        return 1;
    }

    sithMessage_CheckMembershipList(playerId);
    if ( sithMulti_quitGameState )
    {
        sithMulti_FinishJoining(SITHMULTI_JOIN_CANCEL, 0.0f, playerId);
        return 1;
    }

    if ( sithMulti_bWelcomingPlayer )
    {
        if ( sithMulti_newPlayerId != playerId )
        {
            SITHLOG_STATUS("Deferring join request from player %x, busy already.\n", playerId);
            sithMulti_FinishJoining(SITHMULTI_JOIN_BUSY, 0.0f, playerId);
        }
        else
        {
            sithMulti_FinishJoining(SITHMULTI_JOIN_OK, 0.5f, playerId);
        }

        return 1;
    }

    for ( i = 0; i < sithPlayer_g_numPlayers; ++i )
    {
        if ( (sithPlayer_g_aPlayers[i].flags & SITH_PLAYER_PLACED) != 0 && !sithPlayer_g_aPlayers[i].playerNetId )
        {
            break;
        }
    }

    if ( i == sithPlayer_g_numPlayers )
    {
        SITHLOG_STATUS("Received join request from player %d, but the game is full.. sucks for him.\n", playerId);
        sithMulti_FinishJoining(SITHMULTI_JOIN_GAMEFULL, 0.0f, playerId);
        return 1;
    }

    sithMulti_curWelcomePlayerNum = i;
    if ( stdComm_UpdatePlayers(0) )
    {
        SITHLOG_WARNING("Unable to UpdatePlayers List.\n");
        return 1;
    }

    if ( stdComm_VerifyPlayer(playerId) )
    {
        SITHLOG_ERROR("Got a Knock message, but the dude isn't enumerating.\n");
        return 1;
    }

    sithPlayer_Reset(sithMulti_curWelcomePlayerNum);
    stdUtil_WStringNumCopy(sithPlayer_g_aPlayers[sithMulti_curWelcomePlayerNum].awName, STD_ARRAYLEN(sithPlayer_g_aPlayers[sithMulti_curWelcomePlayerNum].awName), (const wchar_t*)pCurDSSMsgData_, 15);
    sithPlayer_g_aPlayers[sithMulti_curWelcomePlayerNum].awName[15] = 0;
    pCurDSSMsgData_ += 32;

    int clientChecksum = SITHDSS_POPINT32();
    SITHLOG_STATUS("My checksum=%x, client=%x.\n", sithMulti_checksum, clientChecksum);
    if ( sithMulti_checksum != clientChecksum )
    {
        SITHLOG_ERROR("Got join request with wrong checksum, %x != %x.\n", sithMulti_checksum, clientChecksum);
        sithMulti_FinishJoining(SITHMULTI_JOIN_WRONGCHECKSUM, 0.0f, playerId);
        return 1;
    }

    sithMulti_FinishJoining(SITHMULTI_JOIN_OK, 0.25f, playerId);
    sithMulti_SyncPlayers(playerId, 0);
    sithMulti_StartWelcomingPlayer(playerId);
    SITHLOG_ERROR("Starting welcome of player %d.\n", playerId);
    return 1;
}

signed int J3DAPI sithMulti_FinishJoining(SithMultiJoinStatus code, float arg4, int playerId)
{
    SITHDSS_STARTOUT(SITHDSS_JOINSTATUS);
    SITHDSS_PUSHINT32(code);
    SITHDSS_PUSHFLOAT(arg4);

    SITHDSS_ENDOUT;
    return sithMessage_SendMessage(&sithMulti_g_message, playerId, SITHMESSAGE_STREAM_NET, 0);
}

int J3DAPI sithMulti_ProcessChat(const SithMessage* pMsg)
{
    SITHDSS_STARTIN(pMsg);
    SITHDSS_POPINT32(); // sender count, unused
    int playerCount = SITHDSS_POPINT32();
    size_t len = SITHDSS_POPUINT32();
    len = len < 128 ? len : 128;

    // TODO: [BUG] len can be 0: len - 1 wraps; the original strncpy then overruns aText (here the copy is clamped) and aText[len - 1] writes before aText
    char aText[132];
    STD_STRNCPY(aText, (const char*)pCurDSSMsgData_, len - 1);
    aText[len - 1] = 0;
    pCurDSSMsgData_ += len;
    aText[len + 1] = 0;

    // TODO: [BUG] playerCount is only checked for being negative, not against sithPlayer_g_numPlayers
    if ( playerCount >= 0 )
    {
        SITHCONSOLE_PRINTF("%S says '%s'", sithPlayer_g_aPlayers[playerCount].awName, aText);
    }
    else
    {
        SITHCONSOLE_PRINTF("%s", aText);
    }

    return 1;
}

int J3DAPI sithMulti_ProcessPing(const SithMessage* pMsg)
{
    ((SithMessage*)pMsg)->type = SITHDSS_PONG;
    sithMessage_SendMessage(&sithMulti_g_message, sithMessage_GetSenderID(), SITHMESSAGE_STREAM_NET, 0);
    return 1;
}

int J3DAPI sithMulti_ProcessPong(const SithMessage* pMsg)
{
    DPID senderId = sithMessage_GetSenderID();

    SITHDSS_STARTIN(pMsg);
    unsigned int msecStart = SITHDSS_POPUINT32();
    if ( msecStart == sithMulti_msecPingStartTime )
    {
        for ( size_t i = 0; i < sithPlayer_g_numPlayers; ++i )
        {
            if ( sithPlayer_g_aPlayers[i].playerNetId == senderId )
            {
                unsigned int msecPing = sithTime_g_msecGameTime - sithMulti_msecPingStartTime;
                SITHCONSOLE_PRINTF("Ping time to %S is %d msec", sithPlayer_g_aPlayers[i].awName, msecPing);
                SITHLOG_STATUS("Ping reply from player %d - %d msec.\n", i, msecPing);
                break;
            }
        }
    }

    return 1;
}

void J3DAPI sithMulti_QuitPlayer(DPID id)
{
    if ( !stdComm_IsGameHost() )
    {
        return;
    }

    SITHDSS_STARTOUT(SITHDSS_QUIT);
    SITHDSS_PUSHUINT32(id);

    SITHDSS_ENDOUT;
    sithMessage_SendMessage(&sithMulti_g_message, id, SITHMESSAGE_STREAM_NET, 1);
}

int J3DAPI sithMulti_ProcessQuit(const SithMessage* pMsg)
{
    SITHDSS_STARTIN(pMsg);
    DPID senderId = sithMessage_GetSenderID();
    DPID playerId = SITHDSS_POPUINT32();
    if ( senderId != sithMulti_g_serverId )
    {
        SITHLOG_ERROR("Got QUIT message from non-server, ignored.\n");
        return 0;
    }

    if ( playerId == sithMessage_g_localPlayerId )
    {
        if ( sithMulti_quitGameState == 2 )
        {
            return 1;
        }

        sithConsole_PrintWString(sithString_GetString("MULTI_EJECTED"));
        sithMulti_QuitGame(sithTime_g_msecGameTime + 5000, 2);
    }
    else
    {
        int playerNum = sithPlayer_GetPlayerNum(playerId);
        if ( playerNum >= 0 )
        {
            sithMulti_RemovePlayer(playerNum);
        }
    }

    return 1;
}

void J3DAPI sithMulti_Update(unsigned int msecDeltaTime)
{
    if ( !stdComm_IsGameActive() )
    {
        return;
    }

    sithThing_SyncThings();
    sithSurface_SyncSurfaces();
    sithSector_SyncSectors();

    if ( sithMulti_quitGameState && sithTime_g_msecGameTime > sithMulti_msecQuitGameTime )
    {
        switch ( sithMulti_quitGameState )
        {
            case 2:
                sithMulti_quitGameState = 0;
                break;

            case 1:
                sithMulti_quitGameState = 0;
                break;
        }

        return;
    }

    if ( !stdComm_IsGameHost() )
    {
        return;
    }

    if ( sithMulti_bSyncScores )
    {
        sithMulti_bSyncScores = 0;
        sithMulti_SyncPlayers(SITHMESSAGE_SENDTOJOINEDPLAYERS, 0);
    }

    if ( !sithMulti_bWelcomingPlayer )
    {
        return;
    }

    if ( sithMulti_quitGameState )
    {
        sithMulti_StopWelcomingPlayer(1);
        return;
    }

    msecDeltaTime += sithMulti_msecWelcomeUpdateInterval;
    unsigned int numUpdates = msecDeltaTime / 60;
    sithMulti_msecWelcomeUpdateInterval = msecDeltaTime - 60 * numUpdates;

    for ( unsigned int i = 0; i < numUpdates; ++i )
    {
        switch ( sithMulti_playerWelcomeState )
        {
            case 2:
                sithMulti_UpdateSurfaces();
                ++sithMulti_numUpdatedSurfaces;
                break;

            case 1:
                sithMulti_UpdateSectors();
                ++sithMulti_numUpdatedSectors;
                break;

            case 3:
                sithMulti_UpdateThings();
                ++sithMulti_numUpdatedThings;
                break;

            case 4:
                sithMulti_UpdateRemovals();
                break;

            default:
                return;
        }
    }
}

void J3DAPI sithMulti_SyncScores()
{
    sithMulti_bSyncScores = 1;
}

//int J3DAPI sithMulti_GetPlayerIndexByID(DPID idPlayer)
//{
//    return J3D_TRAMPOLINE_CALL(sithMulti_GetPlayerIndexByID, idPlayer);
//}

void J3DAPI sithMulti_ProcessKilledPlayer(const SithPlayer* pPlayer, const SithThing* pPlayerThing, const SithThing* pKiller)
{
    wchar_t awMessage[128];
    if ( pKiller && pKiller->type == SITH_THING_PLAYER )
    {
        SITH_ASSERTREL(pKiller->thingInfo.actorInfo.pPlayer);
        if ( pKiller == pPlayerThing )
        {
            float rnd = SITH_RANDF();
            rnd = rnd * 4.0f;
            if ( rnd < 1.0f )
            {
                STD_WFORMAT(awMessage, sithString_GetString("%s_COMMITTED_SUICIDE0"), pPlayer->awName);
            }
            else if ( rnd < 2.0f )
            {
                STD_WFORMAT(awMessage, sithString_GetString("%s_COMMITTED_SUICIDE1"), pPlayer->awName);
            }
            else if ( rnd < 3.0f )
            {
                STD_WFORMAT(awMessage, sithString_GetString("%s_COMMITTED_SUICIDE2"), pPlayer->awName);
            }
            else
            {
                STD_WFORMAT(awMessage, sithString_GetString("%s_COMMITTED_SUICIDE3"), pPlayer->awName);
            }

            sithConsole_PrintWString(awMessage);
        }
        else
        {
            STD_WFORMAT(awMessage, sithString_GetString("%s_WAS_KILLED_BY_%s"), pPlayer->awName, pKiller->thingInfo.actorInfo.pPlayer->awName);
            sithConsole_PrintWString(awMessage);
        }
    }
    else
    {
        STD_WFORMAT(awMessage, sithString_GetString("%s_DIED"), pPlayer->awName);
        sithConsole_PrintWString(awMessage);
    }
}

void J3DAPI sithMulti_QuitGame(unsigned int msecTime, int state)
{
    if ( sithMulti_quitGameState == state && msecTime >= sithMulti_msecQuitGameTime )
    {
        return;
    }

    sithMulti_quitGameState = state;
    sithMulti_msecQuitGameTime = msecTime;
}

size_t J3DAPI sithMulti_Respawn(SithThing* pPlayer)
{
    SITH_ASSERTREL(pPlayer && (pPlayer->type == SITH_THING_PLAYER));

    size_t numFound = 0;
    size_t aPlayerNums[SITHPLAYER_MAX_PLAYERS];
    for ( size_t i = 0; i < sithPlayer_g_numPlayers; ++i )
    {
        if ( (pPlayer->thingInfo.actorInfo.pPlayer->respawnMask & (1 << i)) == 0 )
        {
            aPlayerNums[numFound++] = i;
        }
    }

    if ( !numFound )
    {
        SITHLOG_ERROR("No respawn points unmasked for thing %s, using respawn 0.\n", pPlayer->aName);
        return 0;
    }

    if ( numFound == 1 )
    {
        return aPlayerNums[0];
    }

    size_t spawnIdx = (size_t)(SITH_RANDF() * numFound);
    spawnIdx = STDMATH_CLAMP(spawnIdx, 0, numFound - 1);
    size_t playerNum = aPlayerNums[spawnIdx];

    size_t numTries = 0;
    while ( 1 )
    {
        sithCollision_SearchForCollisions(sithPlayer_g_aPlayers[playerNum].pInSector, NULL, &sithPlayer_g_aPlayers[playerNum].orient.dvec, &rdroid_g_zeroVector3, 0.0f, pPlayer->collide.movesize, 0x482);

        SithCollision* pCollision;
        while ( (pCollision = sithCollision_PopStack()) != NULL )
        {
            if ( (pCollision->type & SITHCOLLISION_THING) != 0
                && pCollision->pThingCollided->type == SITH_THING_PLAYER
                && (pCollision->pThingCollided->flags & (SITH_TF_DYING | SITH_TF_DESTROYED)) == 0 )
            {
                break;
            }
        }

        sithCollision_DecreaseStackLevel();
        if ( pCollision )
        {
            SITHLOG_STATUS("Tried to use respawn %d but was occupied by %s.\n", playerNum, pCollision->pThingCollided->aName);
            if ( numTries < numFound )
            {
                ++numTries;
                spawnIdx = (spawnIdx + 1) % numFound;
                playerNum = aPlayerNums[spawnIdx];
                continue;
            }

            SITHLOG_STATUS("All available respawn positions occupied, using %d anyway.\n", playerNum);
        }

        break;
    }

    return playerNum;
}

void J3DAPI sithMulti_RemoveStaticThing(int guid)
{
    if ( sithMulti_numRemovedStaticThings < 256 )
    {
        sithMulti_aRemovedStaticThings[sithMulti_numRemovedStaticThings++] = guid;
    }
    else
    {
        SITHLOG_ERROR("Limit %d on # of removed static things exceeded!", 256);
    }
}

void J3DAPI sithMulti_UpdateSurfaces()
{
    while ( (size_t)sithMulti_lastUpdateIdx < sithWorld_g_pCurrentWorld->numSurfaces )
    {
        SithSurface* pSurf = &sithWorld_g_pCurrentWorld->aSurfaces[sithMulti_lastUpdateIdx++];
        if ( (pSurf->flags & SITH_SURFACE_SYNC) != 0 )
        {
            sithDSS_SurfaceStatus(pSurf, sithMulti_newPlayerId, SITHMESSAGE_STREAM_NET);
            break;
        }
    }

    if ( (size_t)sithMulti_lastUpdateIdx >= sithWorld_g_pCurrentWorld->numSurfaces )
    {
        SITHLOG_ERROR("Welcome of player %d switching to SECTORS.\n", sithMulti_newPlayerId);
        sithMulti_playerWelcomeState = 1;
        sithMulti_lastUpdateIdx = 0;
    }
}

void J3DAPI sithMulti_UpdateSectors()
{
    while ( (size_t)sithMulti_lastUpdateIdx < sithWorld_g_pCurrentWorld->numSectors )
    {
        SithSector* pSector = &sithWorld_g_pCurrentWorld->aSectors[sithMulti_lastUpdateIdx++];
        if ( (pSector->flags & SITH_SECTOR_SYNC) != 0 )
        {
            sithDSS_SectorStatus(pSector, sithMulti_newPlayerId, SITHMESSAGE_STREAM_NET);
            break;
        }

        if ( (pSector->flags & SITH_SECTOR_ADJOINSOFF) != 0 )
        {
            sithDSS_SectorFlags(pSector, sithMulti_newPlayerId, SITHMESSAGE_STREAM_NET);
            break;
        }
    }

    if ( (size_t)sithMulti_lastUpdateIdx >= sithWorld_g_pCurrentWorld->numSectors )
    {
        SITHLOG_ERROR("Welcome of player %d switching to THINGS.\n", sithMulti_newPlayerId);
        sithMulti_playerWelcomeState = 3;
        sithMulti_lastUpdateIdx = 0;
    }
}

void J3DAPI sithMulti_UpdateThings()
{
    while ( sithMulti_lastUpdateIdx <= sithWorld_g_pCurrentWorld->lastThingIdx )
    {
        SithThing* pThing = &sithWorld_g_pCurrentWorld->aThings[sithMulti_lastUpdateIdx++];
        if ( sithThing_CanSyncThing(pThing) && pThing->type != SITH_THING_WEAPON && pThing->type != SITH_THING_EXPLOSION )
        {
            if ( (pThing->guid & 0xFFFF0000) == 0 )
            {
                sithDSSThing_UpdateState(pThing, sithMulti_newPlayerId, SITHMESSAGE_STREAM_NET);
            }
            else
            {
                sithDSSThing_ThingFullDescription(pThing, sithMulti_newPlayerId, SITHMESSAGE_STREAM_NET);
            }

            sithDSSThing_Pos(pThing, sithMulti_newPlayerId, 0);
            break;
        }
    }

    if ( sithMulti_lastUpdateIdx > sithWorld_g_pCurrentWorld->lastThingIdx )
    {
        SITHLOG_ERROR("Welcome of player %d switching to REMOVALS.\n", sithMulti_newPlayerId);
        sithMulti_playerWelcomeState = 4;
        sithMulti_lastUpdateIdx = 0;
    }
}

void J3DAPI sithMulti_UpdateRemovals()
{
    if ( (size_t)sithMulti_lastUpdateIdx < sithMulti_numRemovedStaticThings )
    {
        sithDSSThing_DestroyThing(sithMulti_aRemovedStaticThings[sithMulti_lastUpdateIdx], sithMulti_newPlayerId);
        ++sithMulti_lastUpdateIdx;
    }

    if ( (size_t)sithMulti_lastUpdateIdx >= sithMulti_numRemovedStaticThings )
    {
        sithMulti_FinishWelcomingPlayer();
    }
}

// Retail function 0x004564F0, not yet in upstream's function list or RTI; called only from sithMulti_UpdateRemovals
static void sithMulti_FinishWelcomingPlayer(void)
{
    SITHLOG_ERROR("Finished welcoming player %d, sending the welcome packet.\n", sithMulti_newPlayerId);
    SITHLOG_ERROR("Update totals: %d surfaces, %d sectors, %d things.\n", sithMulti_numUpdatedSurfaces, sithMulti_numUpdatedSectors, sithMulti_numUpdatedThings);

    sithPlayer_g_aPlayers[sithMulti_curWelcomePlayerNum].playerNetId = sithMulti_newPlayerId;
    sithMulti_SyncPlayers(sithMulti_newPlayerId, 1);
    sithMulti_SendWelcome(sithMulti_newPlayerId, sithMulti_curWelcomePlayerNum, sithMulti_newPlayerId);
    sithMulti_StopWelcomingPlayer(0);
    sithMulti_SyncScores();
}

void J3DAPI sithMulti_StartWelcomingPlayer(DPID playerId)
{
    SITH_ASSERTREL(!sithMulti_bWelcomingPlayer);
    SITH_ASSERTREL(playerId);
    SITHLOG_ERROR("Starting to welcome player %d.\n", playerId);

    sithMulti_bWelcomingPlayer = 1;
    sithMulti_newPlayerId = playerId;
    sithMulti_playerWelcomeState = 2;
    sithMulti_lastUpdateIdx = 0;
    sithMulti_numUpdatedThings = sithMulti_numUpdatedSectors = sithMulti_numUpdatedSurfaces = 0;
}

void J3DAPI sithMulti_StopWelcomingPlayer(int bError)
{
    SITH_ASSERTREL(sithMulti_bWelcomingPlayer);
    SITHLOG_ERROR("Stopped welcoming player %d.\n", sithMulti_newPlayerId);
    if ( bError && sithMulti_newPlayerId )
    {
        sithMulti_FinishJoining(SITHMULTI_JOIN_CANCEL, 0.0f, sithMulti_newPlayerId);
    }

    sithMulti_bWelcomingPlayer = 0;
    sithMulti_newPlayerId = 0;
    sithMulti_playerWelcomeState = 2;
    sithMulti_lastUpdateIdx = 0;
}

void sithMulti_OpenGame(void)
{
    // TODO: Add implementation
    SITH_ASSERT(0);
}

size_t J3DAPI sithMulti_GetTickRate(void)
{
    return sithMulti_tickRate;
}

void J3DAPI sithMulti_SetTickRate(size_t tickRate)
{
    sithMulti_tickRate = tickRate;
}

int J3DAPI sithMulti_Ping(DPID idTo)
{
    sithMulti_msecPingStartTime = sithTime_g_msecGameTime;

    SITHDSS_STARTOUT(SITHDSS_PING);
    SITHDSS_PUSHUINT32(sithTime_g_msecGameTime);
    SITHDSS_ENDOUT;
    return sithMessage_SendMessage(&sithMulti_g_message, idTo, 1u, 0);
}

int J3DAPI sithMulti_GetPlayerIndexByID(DPID playerID)
{
    for ( size_t i = 0; i < sithPlayer_g_numPlayers; ++i )
    {
        if ( playerID == sithPlayer_g_aPlayers[i].playerNetId )
        {
            return i;
        }
    }

    return -1;
}
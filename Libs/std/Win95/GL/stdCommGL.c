#include "../stdComm.h"

#include <j3dcore/j3dhook.h>
#include <std/General/std.h>
#include <std/General/stdMemory.h>
#include <std/General/stdUtil.h>
#include <std/RTI/symbols.h>
#include <std/Win95/stdWin95.h>

static bool bGameHost            = false;
static bool bGameActive          = false;

static size_t stdComm_numPlayers = 0;
static StdCommPlayerInfo stdComm_aPlayers[STDCOMM_MAX_PLAYERS] = { 0 };

static size_t stdComm_numGames = 0;
StdCommGame stdComm_aGames[STDCOMM_MAX_GAMES] = { 0 };


static const char* J3DAPI stdComm_DPGetStatus(HRESULT code);

void stdComm_InstallHooks(void)
{
    J3D_HOOKFUNC(stdComm_Send);
    J3D_HOOKFUNC(stdComm_Receive);
    J3D_HOOKFUNC(stdComm_CloseGame);
    J3D_HOOKFUNC(stdComm_RejoinSession);
    J3D_HOOKFUNC(stdComm_GetSessionSettings);
    J3D_HOOKFUNC(stdComm_IsGameActive);
    J3D_HOOKFUNC(stdComm_IsGameHost);
    J3D_HOOKFUNC(stdComm_UpdatePlayers);
    J3D_HOOKFUNC(stdComm_CreatePlayer);
    J3D_HOOKFUNC(stdComm_DestroyPlayer);
    J3D_HOOKFUNC(stdComm_GetNumPlayers);
    J3D_HOOKFUNC(stdComm_VerifyPlayer);
    J3D_HOOKFUNC(stdComm_GetPlayerID);
    //J3D_HOOKFUNC(stdComm_EnumPlayersCallback);
    //J3D_HOOKFUNC(stdComm_ProcessSystemMessage);
    //J3D_HOOKFUNC(stdComm_SessionToSettings);
    J3D_HOOKFUNC(stdComm_DPGetStatus);
}

void stdComm_ResetGlobals(void)
{}

int32_t J3DAPI stdComm_Send(DPID idFrom, DPID idTo, const void* pData, uint32_t size, uint32_t flags)
{
    J3D_UNUSED(idFrom);
    J3D_UNUSED(idTo);
    J3D_UNUSED(pData);
    J3D_UNUSED(size);
    J3D_UNUSED(flags);

    STDLOG_FATAL("stdComm_Send not implemented!");
    return 0;
}

int32_t J3DAPI stdComm_Receive(DPID* pSender, void* pData, size_t* pLength)
{
    J3D_UNUSED(pLength);
    STD_ASSERTREL(pData && pLength && pSender);
    STDLOG_FATAL("stdComm_Receive not implemented!");
    return -1;
}

int J3DAPI stdComm_GetGame(size_t gameNum, StdCommGame* pGame)
{
    if ( gameNum > stdComm_numGames )
    {
        return 1;
    }

    memcpy(pGame, &stdComm_aGames[gameNum], sizeof(StdCommGame));
    return 0;
}

HRESULT J3DAPI stdComm_CreateGame(const StdCommGame* pSettings)
{
    STD_ASSERTREL(pSettings);
    STDLOG_FATAL("stdComm_CreateGame not implemented!");
    return 0; // success
}

HRESULT J3DAPI stdComm_SetGameParams(StdCommGame* pSettings)
{
    J3D_UNUSED(pSettings);
    STD_ASSERTREL(bGameActive && bGameHost);
    STDLOG_FATAL("stdComm_SetGameParams not implemented!");
    return 0; // Success
}

int J3DAPI stdComm_JoinGame(size_t gameNum, const wchar_t* pPassword)
{
    J3D_UNUSED(pPassword);

    STD_ASSERTREL(!bGameActive);
    STD_ASSERTREL(gameNum < stdComm_numGames);
    STDLOG_FATAL("stdComm_JoinGame not implemented!");
    return 0;
}

void stdComm_CloseGame(void)
{
    STDLOG_FATAL("stdComm_CloseGame not implemented!");
    bGameActive = false;
    bGameHost   = false;
}

int J3DAPI stdComm_RejoinSession(DPID* pPlayerId, const wchar_t* pPlayerName)
{
    J3D_UNUSED(pPlayerId);
    J3D_UNUSED(pPlayerName);

    STD_ASSERTREL(bGameActive);
    STDLOG_FATAL("stdComm_RejoinSession not implemented!");
    bGameActive = true;
    bGameHost   = false;
    return 0;
}

int J3DAPI stdComm_GetSessionSettings(StdCommGame* pSettings)
{
    STD_ASSERTREL(bGameActive);
    STD_ASSERTREL(pSettings);
    STDLOG_FATAL("stdComm_GetSessionSettings not implemented!");
    return 0;
}

int J3DAPI stdComm_IsGameActive()
{
    return bGameActive;
}

int J3DAPI stdComm_IsGameHost(void)
{
    return bGameHost;
}

int J3DAPI stdComm_UpdatePlayers(size_t gameNum)
{
    J3D_UNUSED(gameNum);

    STDLOG_FATAL("stdComm_UpdatePlayers not implemented!");
    return -1;
}

DPID J3DAPI stdComm_CreatePlayer(const wchar_t* pPlayerName)
{
    J3D_UNUSED(pPlayerName);

    STD_ASSERTREL(bGameActive);
    STDLOG_FATAL("stdComm_CreatePlayer not implemented!");
    return 0;
}

void J3DAPI stdComm_DestroyPlayer(DPID playerId)
{
    STD_ASSERTREL(playerId > 0);
    STDLOG_FATAL("stdComm_DestroyPlayer not implemented!");
}

size_t stdComm_GetNumPlayers(void)
{
    return stdComm_numPlayers;
}

int J3DAPI stdComm_VerifyPlayer(DPID id)
{
    for ( size_t i = 0; i < stdComm_numPlayers; ++i )
    {
        if ( id == stdComm_aPlayers[i].id ) {
            return 0;
        }
    }

    return 1;
}

DPID J3DAPI stdComm_GetPlayerID(size_t playerNum)
{
    return stdComm_aPlayers[playerNum].id;
}

const char* J3DAPI stdComm_DPGetStatus(HRESULT code)
{
    J3D_UNUSED(code);
    return "Unknown Error";
}

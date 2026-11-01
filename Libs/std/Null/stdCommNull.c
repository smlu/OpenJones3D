// Added: Null version of std/Win95/stdComm.c (DirectPlay). There is no network on this
// platform: every query answers with "no active game and no players", state-changing calls
// keep the original stubs' success conventions from std/Win95/stdComm.c, but the fatal
// "not implemented" logs are dropped because an inactive session is the expected state here.
#include <j3dcore/j3d.h>

#include <std/Win95/stdComm.h>

#include <std/General/std.h>

static bool bGameActive = false;
static bool bGameHost   = false;

static size_t stdComm_numPlayers = 0;
static StdCommPlayerInfo stdComm_aPlayers[STDCOMM_MAX_PLAYERS] = { 0 };

int32_t J3DAPI stdComm_Send(DPID idFrom, DPID idTo, const void* pData, uint32_t size, uint32_t flags)
{
    J3D_UNUSED(idFrom);
    J3D_UNUSED(idTo);
    J3D_UNUSED(pData);
    J3D_UNUSED(size);
    J3D_UNUSED(flags);
    return 0;
}

int32_t J3DAPI stdComm_Receive(DPID* pSender, void* pData, size_t* pLength)
{
    J3D_UNUSED(pData);
    J3D_UNUSED(pLength);
    STD_ASSERTREL(pSender && pData && pLength);

    // Returns -1 when no message is waiting, like the Win95 stub.
    return -1;
}

// Returns 0 (stored success answer of the Win95 stub); with no session service there is
// nothing to create, and callers only drive multiplayer while IsGameActive reports TRUE.
int J3DAPI stdComm_CreateGame(const StdCommGame* pSettings)
{
    STD_ASSERTREL(pSettings);
    return 0;
}

int J3DAPI stdComm_SetGameParams(StdCommGame* pSettings)
{
    STD_ASSERTREL(bGameActive && bGameHost);
    return 0;
}

int J3DAPI stdComm_JoinGame(size_t gameNum, const wchar_t* pPassword)
{
    J3D_UNUSED(pPassword);
    STD_ASSERTREL(!bGameActive);
    return 0;
}

void stdComm_CloseGame(void)
{
    bGameActive = false;
    bGameHost   = false;
}

int J3DAPI stdComm_RejoinSession(DPID* pPlayerId, const wchar_t* pPlayerName)
{
    J3D_UNUSED(pPlayerId);
    J3D_UNUSED(pPlayerName);
    STD_ASSERTREL(bGameActive);
    return 0;
}

// Added: 1 when no game session is open (the "current settings" answer does not exist).
int J3DAPI stdComm_GetSessionSettings(StdCommGame* pSettings)
{
    J3D_UNUSED(pSettings);
    STD_ASSERTREL(pSettings);
    STD_ASSERTREL(bGameActive);
    return 1;
}

int J3DAPI stdComm_IsGameActive(void)
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
    return -1;
}

DPID J3DAPI stdComm_CreatePlayer(const wchar_t* pPlayerName)
{
    J3D_UNUSED(pPlayerName);
    STD_ASSERTREL(bGameActive);
    return 0;
}

void J3DAPI stdComm_DestroyPlayer(DPID playerId)
{
    STD_ASSERTREL(playerId > 0);
    J3D_UNUSED(playerId);
}

size_t stdComm_GetNumPlayers(void)
{
    return stdComm_numPlayers;
}

int J3DAPI stdComm_VerifyPlayer(DPID id)
{
    for ( size_t i = 0; i < stdComm_numPlayers; ++i )
    {
        if ( id == stdComm_aPlayers[i].id )
        {
            return 0;
        }
    }

    return 1;
}

DPID J3DAPI stdComm_GetPlayerID(size_t playerNum)
{
    STD_ASSERTREL(playerNum < STDCOMM_MAX_PLAYERS);
    return stdComm_aPlayers[playerNum].id;
}

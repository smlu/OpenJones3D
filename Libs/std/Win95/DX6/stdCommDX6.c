#include <std/Win95/stdComm.h>

#include <j3dcore/j3dhook.h>
#include <std/General/std.h>
#include <std/General/stdMemory.h>
#include <std/General/stdUtil.h>
#include <std/RTI/symbols.h>
#include <std/Win95/stdWin95.h>

static bool bGameHost            = false;
static bool bGameActive          = false;
static LPDIRECTPLAY4 pDirectPlay = NULL;

static size_t stdComm_numPlayers = 0;
static StdCommPlayerInfo stdComm_aPlayers[STDCOMM_MAX_PLAYERS] = { 0 };

static size_t stdComm_numGames = 0;
StdCommGame stdComm_aGames[STDCOMM_MAX_GAMES] = { 0 };

static const DXStatus aDPStatusTbl[60] =
{
    { DP_OK,                         "DP_OK"                        },
    { DPERR_ALREADYINITIALIZED,      "DPERR_ALREADYINITIALIZED"     },
    { DPERR_ACCESSDENIED,            "DPERR_ACCESSDENIED"           },
    { DPERR_ACTIVEPLAYERS,           "DPERR_ACTIVEPLAYERS"          },
    { DPERR_BUFFERTOOSMALL,          "DPERR_BUFFERTOOSMALL"         },
    { DPERR_CANTADDPLAYER,           "DPERR_CANTADDPLAYER"          },
    { DPERR_CANTCREATEGROUP,         "DPERR_CANTCREATEGROUP"        },
    { DPERR_CANTCREATEPLAYER,        "DPERR_CANTCREATEPLAYER"       },
    { DPERR_CANTCREATESESSION,       "DPERR_CANTCREATESESSION"      },
    { DPERR_CAPSNOTAVAILABLEYET,     "DPERR_CAPSNOTAVAILABLEYET"    },
    { DPERR_EXCEPTION,               "DPERR_EXCEPTION"              },
    { DPERR_GENERIC,                 "DPERR_GENERIC"                },
    { DPERR_INVALIDFLAGS,            "DPERR_INVALIDFLAGS"           },
    { DPERR_INVALIDOBJECT,           "DPERR_INVALIDOBJECT"          },
    { DPERR_INVALIDPARAM,            "DPERR_INVALIDPARAM"           },
    { DPERR_INVALIDPARAMS,           "DPERR_INVALIDPARAMS"          },
    { DPERR_INVALIDPLAYER,           "DPERR_INVALIDPLAYER"          },
    { DPERR_INVALIDGROUP,            "DPERR_INVALIDGROUP"           },
    { DPERR_NOCAPS,                  "DPERR_NOCAPS"                 },
    { DPERR_NOCONNECTION,            "DPERR_NOCONNECTION"           },
    { DPERR_NOMEMORY,                "DPERR_NOMEMORY"               },
    { DPERR_OUTOFMEMORY,             "DPERR_OUTOFMEMORY"            },
    { DPERR_NOMESSAGES,              "DPERR_NOMESSAGES"             },
    { DPERR_NONAMESERVERFOUND,       "DPERR_NONAMESERVERFOUND"      },
    { DPERR_NOPLAYERS,               "DPERR_NOPLAYERS"              },
    { DPERR_NOSESSIONS,              "DPERR_NOSESSIONS"             },
    { DPERR_PENDING,                 "DPERR_PENDING"                },
    { DPERR_SENDTOOBIG,              "DPERR_SENDTOOBIG"             },
    { DPERR_TIMEOUT,                 "DPERR_TIMEOUT"                },
    { DPERR_UNAVAILABLE,             "DPERR_UNAVAILABLE"            },
    { DPERR_UNSUPPORTED,             "DPERR_UNSUPPORTED"            },
    { DPERR_BUSY,                    "DPERR_BUSY"                   },
    { DPERR_USERCANCEL,              "DPERR_USERCANCEL"             },
    { DPERR_NOINTERFACE,             "DPERR_NOINTERFACE"            },
    { DPERR_CANNOTCREATESERVER,      "DPERR_CANNOTCREATESERVER"     },
    { DPERR_PLAYERLOST,              "DPERR_PLAYERLOST"             },
    { DPERR_SESSIONLOST,             "DPERR_SESSIONLOST"            },
    { DPERR_UNINITIALIZED,           "DPERR_UNINITIALIZED"          },
    { DPERR_NONEWPLAYERS,            "DPERR_NONEWPLAYERS"           },
    { DPERR_INVALIDPASSWORD,         "DPERR_INVALIDPASSWORD"        },
    { DPERR_CONNECTING,              "DPERR_CONNECTING"             },
    { DPERR_BUFFERTOOLARGE,          "DPERR_BUFFERTOOLARGE"         },
    { DPERR_CANTCREATEPROCESS,       "DPERR_CANTCREATEPROCESS"      },
    { DPERR_APPNOTSTARTED,           "DPERR_APPNOTSTARTED"          },
    { DPERR_INVALIDINTERFACE,        "DPERR_INVALIDINTERFACE"       },
    { DPERR_NOSERVICEPROVIDER,       "DPERR_NOSERVICEPROVIDER"      },
    { DPERR_UNKNOWNAPPLICATION,      "DPERR_UNKNOWNAPPLICATION"     },
    { DPERR_NOTLOBBIED,              "DPERR_NOTLOBBIED"             },
    { DPERR_SERVICEPROVIDERLOADED,   "DPERR_SERVICEPROVIDERLOADED"  },
    { DPERR_ALREADYREGISTERED,       "DPERR_ALREADYREGISTERED"      },
    { DPERR_NOTREGISTERED,           "DPERR_NOTREGISTERED"          },
    { DPERR_AUTHENTICATIONFAILED,    "DPERR_AUTHENTICATIONFAILED"   },
    { DPERR_CANTLOADSSPI,            "DPERR_CANTLOADSSPI"           },
    { DPERR_ENCRYPTIONFAILED,        "DPERR_ENCRYPTIONFAILED"       },
    { DPERR_SIGNFAILED,              "DPERR_SIGNFAILED"             },
    { DPERR_CANTLOADSECURITYPACKAGE, "DPERR_CANTLOADSECURITYPACKAGE"},
    { DPERR_ENCRYPTIONNOTSUPPORTED,  "DPERR_ENCRYPTIONNOTSUPPORTED" },
    { DPERR_CANTLOADCAPI,            "DPERR_CANTLOADCAPI"           },
    { DPERR_NOTLOGGEDIN,             "DPERR_NOTLOGGEDIN"            },
    { DPERR_LOGONDENIED,             "DPERR_LOGONDENIED"            }
};

static int J3DAPI stdComm_ProcessSystemMessage(LPDPMSG_GENERIC pMessage, DPID* pSender);
static const char* J3DAPI stdComm_DPGetStatus(HRESULT code);
static void J3DAPI stdComm_SessionToSettings(LPDPSESSIONDESC2 pSession, StdCommGame* pSettings);
static BOOL PASCAL stdComm_EnumPlayersCallback(DPID playerId, DWORD dwPlayerType, LPCDPNAME lpName, DWORD dwFlags, LPVOID lpContext);

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
    J3D_HOOKFUNC(stdComm_EnumPlayersCallback);
    J3D_HOOKFUNC(stdComm_ProcessSystemMessage);
    J3D_HOOKFUNC(stdComm_SessionToSettings);
    J3D_HOOKFUNC(stdComm_DPGetStatus);
}

void stdComm_ResetGlobals(void)
{}

int32_t J3DAPI stdComm_Send(DPID idFrom, DPID idTo, const void* pData, uint32_t size, uint32_t flags)
{
    STD_ASSERTREL(pDirectPlay);
    STD_ASSERTREL(bGameActive);

    HRESULT hr = IDirectPlayX_Send(pDirectPlay, idFrom, idTo, flags, (LPVOID)pData, size);
    if ( hr < DP_OK ) {
        STDLOG_ERROR("DirectPlay->send returned %s.\n", stdComm_DPGetStatus(hr));
    }

    return hr;
}

int32_t J3DAPI stdComm_Receive(DPID* pSender, void* pData, size_t* pLength)
{
    STD_ASSERTREL(pDirectPlay && bGameActive);
    STD_ASSERTREL(pData && pLength && pSender);

    HRESULT hr;
    DPID idTo = 0; // Fixed: Keep error logging deterministic if DirectPlay fails before writing the recipient id.
    while ( 1 )
    {
        DWORD length = *pLength > UINT32_MAX ? UINT32_MAX : (DWORD)*pLength; // Fixed: DirectPlay receives a DWORD byte count, not a size_t pointer.
        hr = IDirectPlayX_Receive(pDirectPlay, pSender, &idTo, DPRECEIVE_ALL, pData, &length);
        *pLength = length;
        if ( hr < DP_OK ) {
            break;
        }

        if ( *pSender != 0 ) {
            return 0;
        }

        int result = stdComm_ProcessSystemMessage((LPDPMSG_GENERIC)pData, pSender);
        if ( result != -1 ) {
            return result;
        }
    }

    if ( hr == DPERR_NOMESSAGES ) {
        return -1;
    }

    STDLOG_ERROR("DirectPlay->Receive from id %x returned %s.\n", idTo, stdComm_DPGetStatus(hr));
    return -2;
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
    STD_ASSERTREL(pDirectPlay);
    STD_ASSERTREL(pSettings);

    DPSESSIONDESC2 sesdesc = { 0 };
    sesdesc.dwSize = sizeof(DPSESSIONDESC2);


    sesdesc.dwFlags |= DPSESSION_DIRECTPLAYPROTOCOL;
    sesdesc.dwFlags |= DPSESSION_KEEPALIVE;
    sesdesc.dwFlags |= DPSESSION_NODATAMESSAGES | DPSESSION_KEEPALIVE;
    sesdesc.dwFlags |= DPSESSION_NOPRESERVEORDER;
    sesdesc.dwFlags |= DPSESSION_OPTIMIZELATENCY;
    sesdesc.dwFlags |= DPSESSION_JOINDISABLED | DPSESSION_MIGRATEHOST;

    sesdesc.guidApplication = *stdWin95_GetGuid();
    sesdesc.dwMaxPlayers = pSettings->maxPlayers;

    wchar_t aSessionName[128];
    STD_WFORMAT(aSessionName, L"%s:%S", pSettings->aSessionName, pSettings->aSomething); // TODO: pSettings->aSomething is of type char!!
    sesdesc.lpszSessionName = aSessionName;
    if ( wcslen(pSettings->aPassword) )
    {
        sesdesc.lpszPassword = (LPWSTR)pSettings->aPassword;
    }

    sesdesc.dwUser1 = pSettings->opt1;
    sesdesc.dwUser3 = pSettings->opt2;
    sesdesc.dwUser4 = pSettings->opt3;

    HRESULT hr = IDirectPlayX_Open(pDirectPlay, &sesdesc, DPOPEN_CREATE);
    if ( hr < DP_OK )
    {
        STDLOG_ERROR("DirectPlay->Open returned %s.\n", stdComm_DPGetStatus(hr));
        IDirectPlayX_Close(pDirectPlay);
        bGameHost   = false;
        bGameActive = false;
        return hr;
    }

    bGameHost = true;
    bGameActive = true; // Fixed: Mark newly created host sessions active instead of writing the host flag twice.
    return 0; // success
}

HRESULT J3DAPI stdComm_SetGameParams(StdCommGame* pSettings)
{
    STD_ASSERTREL(pDirectPlay);
    STD_ASSERTREL(bGameActive && bGameHost);

    DWORD size;
    HRESULT hr = IDirectPlayX_GetSessionDesc(pDirectPlay, NULL, &size);
    if ( hr != DPERR_BUFFERTOOSMALL )           // ????
    {
        STDLOG_ERROR("DirectPlay::GetSessionDesc size query returned %s.\n", stdComm_DPGetStatus(hr));
        return hr;
    }

    LPDPSESSIONDESC2 pSesDesc = (LPDPSESSIONDESC2)STDMALLOC(size);
    hr = IDirectPlayX_GetSessionDesc(pDirectPlay, pSesDesc, &size);
    if ( hr < DP_OK )
    {
        STDLOG_ERROR("GetSessionDesc returned %s.\n", stdComm_DPGetStatus(hr));
        return hr;
    }
    pSesDesc->dwMaxPlayers = pSettings->maxPlayers;

    wchar_t aSessionName[128] = { 0 }; // Added: Init to 0
    STD_WFORMAT(aSessionName, L"%s:%S", pSettings->aSessionName, pSettings->aSomething); // TODO: pSettings->aSomething is of type char!!

    pSesDesc->lpszSessionName = aSessionName;
    if ( wcslen(pSettings->aPassword) )
    {
        pSesDesc->lpszPassword = pSettings->aPassword;
    }

    pSesDesc->dwUser1 = pSettings->opt1;
    pSesDesc->dwUser3 = pSettings->opt2;
    pSesDesc->dwUser4 = pSettings->opt3;

    pSesDesc->dwFlags |= DPSESSION_DIRECTPLAYPROTOCOL;
    pSesDesc->dwFlags |= DPSESSION_KEEPALIVE;
    pSesDesc->dwFlags |= DPSESSION_NODATAMESSAGES;
    pSesDesc->dwFlags |= DPSESSION_NOPRESERVEORDER;
    pSesDesc->dwFlags |= DPSESSION_OPTIMIZELATENCY;
    pSesDesc->dwFlags |= DPSESSION_MIGRATEHOST;;

    hr = IDirectPlayX_SetSessionDesc(pDirectPlay, pSesDesc, 0);
    if ( hr < DP_OK )
    {
        STDLOG_ERROR("SetSessionDesc returned %s (flags=%x).\n", stdComm_DPGetStatus(hr), pSesDesc->dwFlags);
        return hr;
    }

    return 0; // Success
}

int J3DAPI stdComm_JoinGame(size_t gameNum, const wchar_t* pPassword)
{
    STD_ASSERTREL(pDirectPlay);
    STD_ASSERTREL(!bGameActive);
    STD_ASSERTREL(gameNum < stdComm_numGames);

    DPSESSIONDESC2 dpsdesc = { 0 };
    dpsdesc.dwSize = sizeof(DPSESSIONDESC2);
    dpsdesc.guidInstance = stdComm_aGames[gameNum].guid;
    if ( pPassword && wcslen(pPassword) )
    {
        dpsdesc.lpszPassword = (LPWSTR)pPassword;
    }

    HRESULT hr = IDirectPlayX_Open(pDirectPlay, &dpsdesc, DPOPEN_JOIN);
    if ( hr >= DP_OK )
    {
        bGameHost   = false;
        bGameActive = true;
        return 0;
    }

    STDLOG_ERROR("DirectPlay->Open returned %s.\n", stdComm_DPGetStatus(hr));
    IDirectPlayX_Close(pDirectPlay);
    return hr;
}

void stdComm_CloseGame(void)
{
    STD_ASSERTREL(pDirectPlay != NULL);
    if ( bGameActive ) {
        IDirectPlayX_Close(pDirectPlay);
    }

    bGameActive = false;
    bGameHost   = false;
}

int J3DAPI stdComm_RejoinSession(DPID* pPlayerId, const wchar_t* pPlayerName)
{
    STD_ASSERTREL(pDirectPlay != NULL);
    STD_ASSERTREL(bGameActive);

    StdCommGame settings;
    int result = stdComm_GetSessionSettings(&settings);
    if ( result < 0 ) {
        return result;
    }

    HRESULT hr = IDirectPlayX_Close(pDirectPlay);
    if ( hr < DP_OK )
    {
        STDLOG_ERROR("DirectPlay->Close returned %s.\n", stdComm_DPGetStatus(hr));
        return hr;
    }

    DPSESSIONDESC2 sessiondesc;
    memset(&sessiondesc, 0, sizeof(sessiondesc));

    sessiondesc.guidInstance = settings.guid;
    sessiondesc.lpszPassword = settings.aPassword;
    sessiondesc.dwSize = sizeof(DPSESSIONDESC2);
    hr = IDirectPlayX_Open(pDirectPlay, &sessiondesc, DPOPEN_JOIN);
    if ( hr < DP_OK )
    {
        STDLOG_ERROR("DirectPlay->Open returned %s.\n", stdComm_DPGetStatus(hr));
        return hr;
    }

    *pPlayerId = stdComm_CreatePlayer(pPlayerName);
    if ( *pPlayerId == 0 )
    {
        STDLOG_ERROR("Failed to create new player after rejoining the session.\n");
        return DPERR_GENERIC;
    }

    bGameActive = true;
    bGameHost   = false;
    return 0;
}

int J3DAPI stdComm_GetSessionSettings(StdCommGame* pSettings)
{
    STD_ASSERTREL(pDirectPlay);
    STD_ASSERTREL(bGameActive);
    STD_ASSERTREL(pSettings);

    DWORD size = 0;
    HRESULT hr = IDirectPlayX_GetSessionDesc(pDirectPlay, NULL, &size);
    if ( hr != DPERR_BUFFERTOOSMALL )
    {
        STDLOG_ERROR("DirectPlay::GetSessionDesc size query returned %s.\n", stdComm_DPGetStatus(hr));
        return hr;
    }

    LPDPSESSIONDESC2 pSessionDesc = (LPDPSESSIONDESC2)STDMALLOC(size);
    hr = IDirectPlayX_GetSessionDesc(pDirectPlay, pSessionDesc, &size);
    if ( hr < DP_OK )
    {
        STDLOG_ERROR("GetSessionDesc returned %s.\n", stdComm_DPGetStatus(hr));
        return hr;
    }

    stdComm_SessionToSettings(pSessionDesc, pSettings);
    stdMemory_Free(pSessionDesc);
    return hr;
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
    STD_ASSERTREL((pDirectPlay != NULL));

    stdComm_numPlayers = 0;
    memset(stdComm_aPlayers, 0, sizeof(stdComm_aPlayers));

    HRESULT hr;
    if ( bGameActive ) {
        hr = IDirectPlayX_EnumPlayers(pDirectPlay, 0, stdComm_EnumPlayersCallback, NULL, 0);
    }
    else
    {
        if ( gameNum >= stdComm_numGames ) {
            return DPERR_GENERIC;
        }

        hr = IDirectPlayX_EnumPlayers(pDirectPlay, &stdComm_aGames[gameNum].guid, stdComm_EnumPlayersCallback, NULL, DPENUMPLAYERS_SESSION);
    }

    if ( hr >= DP_OK ) {
        return hr;
    }

    STDLOG_ERROR("UpdatePlayers returned %s.\n", stdComm_DPGetStatus(hr));
    return hr;
}

DPID J3DAPI stdComm_CreatePlayer(const wchar_t* pPlayerName)
{
    STD_ASSERTREL(pDirectPlay);
    STD_ASSERTREL(bGameActive);

    DPNAME dpname;
    dpname.lpszShortName = (LPWSTR)pPlayerName;
    dpname.dwSize        = sizeof(DPNAME);
    dpname.dwFlags       = 0;
    dpname.lpszLongName  = 0;

    DPID id = 0; // Fixed: Return a deterministic invalid player id when DirectPlay fails.
    HRESULT hr = IDirectPlayX_CreatePlayer(pDirectPlay, &id, &dpname, NULL, NULL, 0, 0);
    if ( hr >= DP_OK ) {
        return id;
    }

    STDLOG_STATUS("CreatePlayer returned %s.\n", stdComm_DPGetStatus(hr));
    return id;
}

void J3DAPI stdComm_DestroyPlayer(DPID playerId)
{
    STD_ASSERTREL(pDirectPlay != NULL);
    STD_ASSERTREL(playerId > 0);
    HRESULT  hr = IDirectPlayX_DestroyPlayer(pDirectPlay, playerId);
    if ( hr >= DP_OK ) {
        STDLOG_STATUS("Player %d destroyed.\n", playerId);
    }
    else {
        STDLOG_ERROR("DestroyPlayer returned %s.\n", stdComm_DPGetStatus(hr));
    }
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

BOOL PASCAL stdComm_EnumPlayersCallback(DPID playerId, DWORD dwPlayerType, LPCDPNAME lpName, DWORD dwFlags, LPVOID lpContext)
{
    J3D_UNUSED(dwFlags);
    J3D_UNUSED(lpContext);
    if ( dwPlayerType != DPPLAYERTYPE_PLAYER ) {
        return TRUE;
    }

    if ( stdComm_numPlayers < STDCOMM_MAX_PLAYERS )
    {
        memset(&stdComm_aPlayers[stdComm_numPlayers], 0, sizeof(StdCommPlayerInfo));
        if ( lpName->lpszShortName ) {
            stdUtil_WStringCopy(stdComm_aPlayers[stdComm_numPlayers].aName, STD_ARRAYLEN(stdComm_aPlayers[stdComm_numPlayers].aName), lpName->lpszShortName);
        }

        stdComm_aPlayers[stdComm_numPlayers++].id = playerId;
        return TRUE;
    }
    else
    {
        STDLOG_ERROR("Too many players.  Player %d not enumerated.\n", playerId);
        return TRUE;
    }
}

int J3DAPI stdComm_ProcessSystemMessage(LPDPMSG_GENERIC pMessage, DPID* pSender)
{
    STD_ASSERTREL(bGameActive && pDirectPlay);
    STD_ASSERTREL(pMessage);
    switch ( pMessage->dwType )
    {
        case DPSYS_CREATEPLAYERORGROUP:
            *pSender = ((LPDPMSG_CREATEPLAYERORGROUP)pMessage)->dpId;
            if ( ((LPDPMSG_CREATEPLAYERORGROUP)pMessage)->dwPlayerType == DPPLAYERTYPE_PLAYER ) {
                return 5;
            }
            return -1;

        case DPSYS_DESTROYPLAYERORGROUP:
            *pSender = ((LPDPMSG_DESTROYPLAYERORGROUP)pMessage)->dpId;
            return 2;

        case DPSYS_SESSIONLOST:
            return 1;

        case DPSYS_HOST: // the current host has left the session, we're now the host
            bGameHost = 1;
            return 8;
    }

    return -1;
}

void J3DAPI stdComm_SessionToSettings(LPDPSESSIONDESC2 pSession, StdCommGame* pSettings)
{
    wchar_t aSessionName[128] = { 0 };

    STD_ASSERTREL(pSession && pSettings);
    stdUtil_WStringCopy(aSessionName, STD_ARRAYLEN(aSessionName), pSession->lpszSessionName);

    wchar_t* pntok = NULL;
    wchar_t* pSessionName = wcstok(aSessionName, L":\n", &pntok);
    if ( pSessionName ) {
        stdUtil_WStringCopy(pSettings->aSessionName, STD_ARRAYLEN(pSettings->aSessionName), pSessionName);
    }

    wchar_t* v3 = wcstok(0, L"\n", &pntok);
    if ( v3 )
    {
        STD_TOSTR(pSettings->aSomething, v3);
    }

    pSettings->guid              = pSession->guidInstance;
    pSettings->maxPlayers        = pSession->dwMaxPlayers;
    pSettings->numCurrentPlayers = pSession->dwCurrentPlayers;
    pSettings->opt1              = pSession->dwUser1;
    pSettings->opt2              = pSession->dwUser3;
    pSettings->opt3              = pSession->dwUser4;
}

const char* J3DAPI stdComm_DPGetStatus(HRESULT code)
{

    for ( size_t i = 0; i < STD_ARRAYLEN(aDPStatusTbl); i++ )
    {
        const DXStatus* pCurStatus = &aDPStatusTbl[i];
        if ( pCurStatus->code == code ) {
            return pCurStatus->text;
        }
    }
    return "Unknown Error";
}

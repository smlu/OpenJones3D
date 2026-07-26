#include <unity_fixture.h>

#include <string.h>

#include <j3dcore/Tests/j3dTest.h>
#include <std/General/stdUtil.h>
#include <std/General/stdMemory.h>
#include <std/Win95/stdComm.h>
#include <std/Win95/stdWin95.h>

#include "stdGeneralTest.h"

#define STDCOMMDX6TEST_MAX_MESSAGES 8u
#define STDCOMMDX6TEST_MAX_FAKE_PLAYERS 32u

typedef struct sStdCommDX6TestMessage
{
    DPID sender;
    DPID recipient;
    uint8_t aData[128];
    DWORD size;
} StdCommDX6TestMessage;

typedef struct sStdCommDX6TestFakeDirectPlay
{
    IDirectPlay4 iface;
    IDirectPlay4Vtbl vtbl;
    HRESULT openResult;
    HRESULT closeResult;
    HRESULT sendResult;
    HRESULT receiveResult;
    HRESULT enumPlayersResult;
    HRESULT createPlayerResult;
    HRESULT destroyPlayerResult;
    HRESULT getSessionDescQueryResult;
    HRESULT getSessionDescResult;
    HRESULT setSessionDescResult;
    DWORD lastOpenFlags;
    DPID lastSendFrom;
    DPID lastSendTo;
    DWORD lastSendFlags;
    uint8_t aLastSendData[128];
    DWORD lastSendSize;
    DPID lastDestroyedPlayer;
    DWORD closeCount;
    DWORD setSessionDescCount;
    DWORD lastSetSessionDescFlags;
    DPID nextPlayerId;
    DPSESSIONDESC2 sessionDesc;
    wchar_t aSessionName[128];
    wchar_t aPassword[64];
    StdCommDX6TestMessage aMessages[STDCOMMDX6TEST_MAX_MESSAGES];
    size_t numMessages;
    size_t nextMessage;
    DPID aPlayerIds[STDCOMMDX6TEST_MAX_FAKE_PLAYERS];
    wchar_t aPlayerNames[STDCOMMDX6TEST_MAX_FAKE_PLAYERS][20];
    size_t numPlayers;
} StdCommDX6TestFakeDirectPlay;

extern StdCommGame stdComm_aGames[STDCOMM_MAX_GAMES];
void stdComm_TestSetDirectPlayState(LPDIRECTPLAY4 pDirectPlayArg, bool bActive, bool bHost, size_t numGames);

TEST_GROUP(stdCommDX6);

static GUID stdCommDX6Test_guid =
{
    0xD36132B2u,
    0x580Cu,
    0x4F11u,
    { 0x94u, 0x7Du, 0x2Eu, 0xA2u, 0x40u, 0x1Du, 0x51u, 0x9Fu }
};

static StdCommDX6TestFakeDirectPlay* stdCommDX6TestFake_FromInterface(LPDIRECTPLAY4 pDirectPlay)
{
    return (StdCommDX6TestFakeDirectPlay*)pDirectPlay;
}

static HRESULT STDMETHODCALLTYPE stdCommDX6TestFake_Close(LPDIRECTPLAY4 pDirectPlay)
{
    StdCommDX6TestFakeDirectPlay* pFake = stdCommDX6TestFake_FromInterface(pDirectPlay);

    ++pFake->closeCount;
    return pFake->closeResult;
}

static HRESULT STDMETHODCALLTYPE stdCommDX6TestFake_CreatePlayer(LPDIRECTPLAY4 pDirectPlay, LPDPID pId, LPDPNAME pName, HANDLE event, LPVOID pData, DWORD dataSize, DWORD flags)
{
    StdCommDX6TestFakeDirectPlay* pFake = stdCommDX6TestFake_FromInterface(pDirectPlay);

    J3D_UNUSED(event);
    J3D_UNUSED(pData);
    J3D_UNUSED(dataSize);
    J3D_UNUSED(flags);

    if ( pFake->createPlayerResult < DP_OK )
    {
        return pFake->createPlayerResult;
    }

    *pId = pFake->nextPlayerId++;
    if ( pFake->numPlayers < STDCOMMDX6TEST_MAX_FAKE_PLAYERS )
    {
        pFake->aPlayerIds[pFake->numPlayers] = *pId;
        if ( pName && pName->lpszShortName )
        {
            stdUtil_WStringCopy(pFake->aPlayerNames[pFake->numPlayers], STD_ARRAYLEN(pFake->aPlayerNames[pFake->numPlayers]), pName->lpszShortName);
        }

        ++pFake->numPlayers;
    }

    return DP_OK;
}

static HRESULT STDMETHODCALLTYPE stdCommDX6TestFake_DestroyPlayer(LPDIRECTPLAY4 pDirectPlay, DPID playerId)
{
    StdCommDX6TestFakeDirectPlay* pFake = stdCommDX6TestFake_FromInterface(pDirectPlay);

    pFake->lastDestroyedPlayer = playerId;
    return pFake->destroyPlayerResult;
}

static HRESULT STDMETHODCALLTYPE stdCommDX6TestFake_EnumPlayers(LPDIRECTPLAY4 pDirectPlay, LPGUID pSessionGuid, LPDPENUMPLAYERSCALLBACK2 pfCallback, LPVOID pContext, DWORD flags)
{
    StdCommDX6TestFakeDirectPlay* pFake = stdCommDX6TestFake_FromInterface(pDirectPlay);

    J3D_UNUSED(pSessionGuid);
    J3D_UNUSED(flags);

    if ( pFake->enumPlayersResult < DP_OK )
    {
        return pFake->enumPlayersResult;
    }

    for ( size_t i = 0; i < pFake->numPlayers; ++i )
    {
        DPNAME name;

        STD_ZEROMEM(&name, sizeof(name));
        name.dwSize        = sizeof(name);
        name.lpszShortName = pFake->aPlayerNames[i];
        if ( !pfCallback(pFake->aPlayerIds[i], DPPLAYERTYPE_PLAYER, &name, 0u, pContext) )
        {
            break;
        }
    }

    return DP_OK;
}

static HRESULT STDMETHODCALLTYPE stdCommDX6TestFake_GetSessionDesc(LPDIRECTPLAY4 pDirectPlay, LPVOID pSessionDesc, LPDWORD pSize)
{
    StdCommDX6TestFakeDirectPlay* pFake = stdCommDX6TestFake_FromInterface(pDirectPlay);

    if ( !pSessionDesc )
    {
        *pSize = sizeof(DPSESSIONDESC2);
        return pFake->getSessionDescQueryResult;
    }

    if ( pFake->getSessionDescResult < DP_OK )
    {
        return pFake->getSessionDescResult;
    }

    if ( *pSize < sizeof(DPSESSIONDESC2) )
    {
        *pSize = sizeof(DPSESSIONDESC2);
        return DPERR_BUFFERTOOSMALL;
    }

    STD_COPYMEM(pSessionDesc, &pFake->sessionDesc, sizeof(DPSESSIONDESC2));
    return DP_OK;
}

static HRESULT STDMETHODCALLTYPE stdCommDX6TestFake_Open(LPDIRECTPLAY4 pDirectPlay, LPDPSESSIONDESC2 pSessionDesc, DWORD flags)
{
    StdCommDX6TestFakeDirectPlay* pFake = stdCommDX6TestFake_FromInterface(pDirectPlay);

    pFake->lastOpenFlags = flags;
    if ( pFake->openResult < DP_OK )
    {
        return pFake->openResult;
    }

    if ( pSessionDesc )
    {
        pFake->sessionDesc = *pSessionDesc;
        if ( pSessionDesc->lpszSessionName )
        {
            stdUtil_WStringCopy(pFake->aSessionName, STD_ARRAYLEN(pFake->aSessionName), pSessionDesc->lpszSessionName);
            pFake->sessionDesc.lpszSessionName = pFake->aSessionName;
        }

        if ( pSessionDesc->lpszPassword )
        {
            stdUtil_WStringCopy(pFake->aPassword, STD_ARRAYLEN(pFake->aPassword), pSessionDesc->lpszPassword);
            pFake->sessionDesc.lpszPassword = pFake->aPassword;
        }
    }

    return DP_OK;
}

static HRESULT STDMETHODCALLTYPE stdCommDX6TestFake_Receive(LPDIRECTPLAY4 pDirectPlay, LPDPID pSender, LPDPID pRecipient, DWORD flags, LPVOID pData, LPDWORD pSize)
{
    StdCommDX6TestFakeDirectPlay* pFake = stdCommDX6TestFake_FromInterface(pDirectPlay);

    J3D_UNUSED(flags);

    if ( pFake->receiveResult < DP_OK )
    {
        *pRecipient = 0u;
        return pFake->receiveResult;
    }

    if ( pFake->nextMessage >= pFake->numMessages )
    {
        return DPERR_NOMESSAGES;
    }

    const StdCommDX6TestMessage* pMessage = &pFake->aMessages[pFake->nextMessage];
    if ( *pSize < pMessage->size )
    {
        *pSize = pMessage->size;
        return DPERR_BUFFERTOOSMALL;
    }

    *pSender    = pMessage->sender;
    *pRecipient = pMessage->recipient;
    *pSize      = pMessage->size;
    STD_COPYMEM(pData, pMessage->aData, pMessage->size);
    ++pFake->nextMessage;
    return DP_OK;
}

static HRESULT STDMETHODCALLTYPE stdCommDX6TestFake_Send(LPDIRECTPLAY4 pDirectPlay, DPID idFrom, DPID idTo, DWORD flags, LPVOID pData, DWORD size)
{
    StdCommDX6TestFakeDirectPlay* pFake = stdCommDX6TestFake_FromInterface(pDirectPlay);

    pFake->lastSendFrom  = idFrom;
    pFake->lastSendTo    = idTo;
    pFake->lastSendFlags = flags;
    pFake->lastSendSize  = J3DMIN(size, sizeof(pFake->aLastSendData));
    STD_COPYMEM(pFake->aLastSendData, pData, pFake->lastSendSize);
    return pFake->sendResult;
}

static HRESULT STDMETHODCALLTYPE stdCommDX6TestFake_SetSessionDesc(LPDIRECTPLAY4 pDirectPlay, LPDPSESSIONDESC2 pSessionDesc, DWORD flags)
{
    StdCommDX6TestFakeDirectPlay* pFake = stdCommDX6TestFake_FromInterface(pDirectPlay);

    pFake->lastSetSessionDescFlags = flags;

    ++pFake->setSessionDescCount;
    if ( pFake->setSessionDescResult < DP_OK )
    {
        return pFake->setSessionDescResult;
    }

    pFake->sessionDesc = *pSessionDesc;
    if ( pSessionDesc->lpszSessionName )
    {
        stdUtil_WStringCopy(pFake->aSessionName, STD_ARRAYLEN(pFake->aSessionName), pSessionDesc->lpszSessionName);
        pFake->sessionDesc.lpszSessionName = pFake->aSessionName;
    }

    if ( pSessionDesc->lpszPassword )
    {
        stdUtil_WStringCopy(pFake->aPassword, STD_ARRAYLEN(pFake->aPassword), pSessionDesc->lpszPassword);
        pFake->sessionDesc.lpszPassword = pFake->aPassword;
    }

    return DP_OK;
}

static void stdCommDX6TestFake_Init(StdCommDX6TestFakeDirectPlay* pFake)
{
    STD_ZEROMEM(pFake, sizeof(*pFake));
    pFake->iface.lpVtbl               = &pFake->vtbl;
    pFake->vtbl.Close                 = stdCommDX6TestFake_Close;
    pFake->vtbl.CreatePlayer          = stdCommDX6TestFake_CreatePlayer;
    pFake->vtbl.DestroyPlayer         = stdCommDX6TestFake_DestroyPlayer;
    pFake->vtbl.EnumPlayers           = stdCommDX6TestFake_EnumPlayers;
    pFake->vtbl.GetSessionDesc        = stdCommDX6TestFake_GetSessionDesc;
    pFake->vtbl.Open                  = stdCommDX6TestFake_Open;
    pFake->vtbl.Receive               = stdCommDX6TestFake_Receive;
    pFake->vtbl.Send                  = stdCommDX6TestFake_Send;
    pFake->vtbl.SetSessionDesc        = stdCommDX6TestFake_SetSessionDesc;
    pFake->openResult                 = DP_OK;
    pFake->closeResult                = DP_OK;
    pFake->sendResult                 = DP_OK;
    pFake->receiveResult              = DP_OK;
    pFake->enumPlayersResult          = DP_OK;
    pFake->createPlayerResult         = DP_OK;
    pFake->destroyPlayerResult        = DP_OK;
    pFake->getSessionDescQueryResult  = DPERR_BUFFERTOOSMALL;
    pFake->getSessionDescResult       = DP_OK;
    pFake->setSessionDescResult       = DP_OK;
    pFake->nextPlayerId               = 1000u;
    pFake->sessionDesc.dwSize         = sizeof(DPSESSIONDESC2);
    pFake->sessionDesc.guidInstance   = stdCommDX6Test_guid;
    pFake->sessionDesc.dwMaxPlayers   = 8u;
    pFake->sessionDesc.dwUser1        = 11u;
    pFake->sessionDesc.dwUser3        = 22u;
    pFake->sessionDesc.dwUser4        = 33u;
    STD_WSTRCPY(pFake->aSessionName, L"DX6 Local:arena01");
    pFake->sessionDesc.lpszSessionName = pFake->aSessionName;
}

static void stdCommDX6TestFake_QueueMessage(StdCommDX6TestFakeDirectPlay* pFake, DPID sender, const void* pData, DWORD size)
{
    TEST_ASSERT_TRUE(pFake->numMessages < STDCOMMDX6TEST_MAX_MESSAGES);

    StdCommDX6TestMessage* pMessage = &pFake->aMessages[pFake->numMessages++];
    pMessage->sender                = sender;
    pMessage->recipient             = 0u;
    pMessage->size                  = J3DMIN(size, sizeof(pMessage->aData));
    STD_COPYMEM(pMessage->aData, pData, pMessage->size);
}

static void stdCommDX6TestFake_AddPlayer(StdCommDX6TestFakeDirectPlay* pFake, DPID playerId, const wchar_t* pName)
{
    TEST_ASSERT_TRUE(pFake->numPlayers < STDCOMMDX6TEST_MAX_FAKE_PLAYERS);

    pFake->aPlayerIds[pFake->numPlayers] = playerId;
    stdUtil_WStringCopy(pFake->aPlayerNames[pFake->numPlayers], STD_ARRAYLEN(pFake->aPlayerNames[pFake->numPlayers]), pName);
    ++pFake->numPlayers;
}

TEST_SETUP(stdCommDX6)
{
    StdGeneralTest_Startup();
    stdComm_ResetGlobals();
    STD_ZEROMEM(stdComm_aGames, sizeof(StdCommGame) * STDCOMM_MAX_GAMES);
    stdWin95_SetGuid(&stdCommDX6Test_guid);
    J3DTest_ResetAssertCapture();
}

TEST_TEAR_DOWN(stdCommDX6)
{
    stdComm_ResetGlobals();
    J3DTest_ResetAssertCapture();
    StdGeneralTest_Shutdown();
}

TEST(stdCommDX6, TestPublicLimitsAndInactiveState)
{
    TEST_ASSERT_EQUAL_UINT32(24u, STDCOMM_MAX_PLAYERS);
    TEST_ASSERT_EQUAL_UINT32(32u, STDCOMM_MAX_GAMES);
    TEST_ASSERT_EQUAL_size_t(44u, sizeof(StdCommPlayerInfo));
    TEST_ASSERT_EQUAL_size_t(356u, sizeof(StdCommGame));
    TEST_ASSERT_FALSE(stdComm_IsGameActive());
    TEST_ASSERT_FALSE(stdComm_IsGameHost());
    TEST_ASSERT_EQUAL_size_t(0u, stdComm_GetNumPlayers());
    TEST_ASSERT_EQUAL_INT(1, stdComm_VerifyPlayer((DPID)1u));
    TEST_ASSERT_EQUAL_UINT32(0u, stdComm_GetPlayerID(0u));
}

TEST(stdCommDX6, TestGetGameBoundsAndSlotZeroCopy)
{
    StdCommGame game;
    StdCommGame outGame;

    STD_ZEROMEM(&game, sizeof(game));
    StdCommGame unchanged;
    STD_FILLMEM(&unchanged, 0xCD, sizeof(unchanged));
    STD_FILLMEM(&outGame, 0xCD, sizeof(outGame));

    game.maxPlayers        = 8;
    game.numCurrentPlayers = 2;
    game.opt1              = 11;
    game.opt2              = 22;
    game.opt3              = 33;
    STD_WSTRCPY(game.aSessionName, L"std dx6 test game");
    STD_WSTRCPY(game.aPassword, L"secret");
    STD_COPYMEM(&stdComm_aGames[0], &game, sizeof(game));

    TEST_ASSERT_EQUAL_INT(0, stdComm_GetGame(0u, &outGame));
    TEST_ASSERT_EQUAL_MEMORY(&game, &outGame, sizeof(game));

    STD_FILLMEM(&outGame, 0xCD, sizeof(outGame));
    TEST_ASSERT_EQUAL_INT(1, stdComm_GetGame(1u, &outGame));
    TEST_ASSERT_EQUAL_MEMORY(&unchanged, &outGame, sizeof(outGame));

    STD_FILLMEM(&outGame, 0xCD, sizeof(outGame));
    TEST_ASSERT_EQUAL_INT(1, stdComm_GetGame(STDCOMM_MAX_GAMES, &outGame));
    TEST_ASSERT_EQUAL_MEMORY(&unchanged, &outGame, sizeof(outGame));
}

TEST(stdCommDX6, TestCloseGameWithoutDirectPlayAssertsAndLeavesInactiveState)
{
    stdComm_CloseGame();

    TEST_ASSERT_FALSE(stdComm_IsGameActive());
    TEST_ASSERT_FALSE(stdComm_IsGameHost());
    TEST_ASSERT_EQUAL_INT(1, J3DTest_GetAssertCount());
    StdGeneralTest_AssertLastAssert("pDirectPlay != NULL", "stdCommDX6.c");
}

TEST(stdCommDX6, TestUpdatePlayersWithoutDirectPlayRejectsMissingGame)
{
    TEST_ASSERT_EQUAL_INT(DPERR_GENERIC, stdComm_UpdatePlayers(0u));
    TEST_ASSERT_EQUAL_size_t(0u, stdComm_GetNumPlayers());
    TEST_ASSERT_EQUAL_INT(1, J3DTest_GetAssertCount());
    StdGeneralTest_AssertLastAssert("(pDirectPlay != NULL)", "stdCommDX6.c");
}

TEST(stdCommDX6, TestFakeDirectPlayCreateGameSessionSettingsAndClose)
{
    StdCommDX6TestFakeDirectPlay fake;
    StdCommGame settings;
    StdCommGame outSettings;

    stdCommDX6TestFake_Init(&fake);
    stdComm_TestSetDirectPlayState(&fake.iface, false, false, 0u);

    STD_ZEROMEM(&settings, sizeof(settings));
    settings.maxPlayers = 6;
    settings.opt1       = 101;
    settings.opt2       = 202;
    settings.opt3       = 303;
    STD_WSTRCPY(settings.aSessionName, L"CodexLab");
    STD_STRCPY(settings.aSomething, "sector7");
    STD_WSTRCPY(settings.aPassword, L"pw");

    TEST_ASSERT_EQUAL_INT(DP_OK, stdComm_CreateGame(&settings));
    TEST_ASSERT_TRUE(stdComm_IsGameActive());
    TEST_ASSERT_TRUE(stdComm_IsGameHost());
    TEST_ASSERT_EQUAL_UINT32(DPOPEN_CREATE, fake.lastOpenFlags);
    TEST_ASSERT_EQUAL_INT(0, wcscmp(L"CodexLab:sector7", fake.sessionDesc.lpszSessionName));
    TEST_ASSERT_EQUAL_INT(0, wcscmp(L"pw", fake.sessionDesc.lpszPassword));
    TEST_ASSERT_EQUAL_UINT32(6u, fake.sessionDesc.dwMaxPlayers);

    STD_ZEROMEM(&outSettings, sizeof(outSettings));
    TEST_ASSERT_EQUAL_INT(DP_OK, stdComm_GetSessionSettings(&outSettings));
    TEST_ASSERT_EQUAL_INT(0, wcscmp(L"CodexLab", outSettings.aSessionName));
    TEST_ASSERT_EQUAL_STRING("sector7", outSettings.aSomething);
    TEST_ASSERT_EQUAL_INT(101, outSettings.opt1);
    TEST_ASSERT_EQUAL_INT(202, outSettings.opt2);
    TEST_ASSERT_EQUAL_INT(303, outSettings.opt3);

    settings.maxPlayers = 4;
    settings.opt1       = 404;
    TEST_ASSERT_EQUAL_INT(DP_OK, stdComm_SetGameParams(&settings));
    TEST_ASSERT_EQUAL_UINT32(1u, fake.setSessionDescCount);
    TEST_ASSERT_EQUAL_UINT32(4u, fake.sessionDesc.dwMaxPlayers);
    TEST_ASSERT_EQUAL_UINT32(404u, fake.sessionDesc.dwUser1);

    stdComm_CloseGame();
    TEST_ASSERT_FALSE(stdComm_IsGameActive());
    TEST_ASSERT_FALSE(stdComm_IsGameHost());
    TEST_ASSERT_EQUAL_UINT32(1u, fake.closeCount);
}

TEST(stdCommDX6, TestFakeDirectPlayCreateGameFailureClearsState)
{
    StdCommDX6TestFakeDirectPlay fake;
    StdCommGame settings;

    stdCommDX6TestFake_Init(&fake);
    fake.openResult = DPERR_CANTCREATESESSION;
    stdComm_TestSetDirectPlayState(&fake.iface, false, false, 0u);

    STD_ZEROMEM(&settings, sizeof(settings));
    settings.maxPlayers = 2;
    STD_WSTRCPY(settings.aSessionName, L"Broken");

    TEST_ASSERT_EQUAL_INT(DPERR_CANTCREATESESSION, stdComm_CreateGame(&settings));
    TEST_ASSERT_FALSE(stdComm_IsGameActive());
    TEST_ASSERT_FALSE(stdComm_IsGameHost());
    TEST_ASSERT_EQUAL_UINT32(1u, fake.closeCount);
}

TEST(stdCommDX6, TestFakeDirectPlaySendReceiveAndSystemMessages)
{
    StdCommDX6TestFakeDirectPlay fake;
    uint8_t aPayload[] = { 0x10u, 0x20u, 0x30u, 0x40u };
    uint8_t aReceived[64];
    DPID sender;
    size_t length;

    stdCommDX6TestFake_Init(&fake);
    stdComm_TestSetDirectPlayState(&fake.iface, true, false, 0u);

    TEST_ASSERT_EQUAL_INT(DP_OK, stdComm_Send(10u, 20u, aPayload, sizeof(aPayload), DPSEND_GUARANTEED));
    TEST_ASSERT_EQUAL_UINT32(10u, fake.lastSendFrom);
    TEST_ASSERT_EQUAL_UINT32(20u, fake.lastSendTo);
    TEST_ASSERT_EQUAL_UINT32(DPSEND_GUARANTEED, fake.lastSendFlags);
    TEST_ASSERT_EQUAL_MEMORY(aPayload, fake.aLastSendData, sizeof(aPayload));
    TEST_ASSERT_EQUAL_UINT32(sizeof(aPayload), fake.lastSendSize);

    stdCommDX6TestFake_QueueMessage(&fake, 20u, aPayload, sizeof(aPayload));
    STD_ZEROMEM(aReceived, sizeof(aReceived));
    sender = 0u;
    length = sizeof(aReceived);
    TEST_ASSERT_EQUAL_INT(0, stdComm_Receive(&sender, aReceived, &length));
    TEST_ASSERT_EQUAL_UINT32(20u, sender);
    TEST_ASSERT_EQUAL_size_t(sizeof(aPayload), length);
    TEST_ASSERT_EQUAL_MEMORY(aPayload, aReceived, sizeof(aPayload));

    length = sizeof(aReceived);
    TEST_ASSERT_EQUAL_INT(-1, stdComm_Receive(&sender, aReceived, &length));

    DPMSG_CREATEPLAYERORGROUP createMessage;
    STD_ZEROMEM(&createMessage, sizeof(createMessage));
    createMessage.dwType       = DPSYS_CREATEPLAYERORGROUP;
    createMessage.dpId         = 77u;
    createMessage.dwPlayerType = DPPLAYERTYPE_PLAYER;
    stdCommDX6TestFake_QueueMessage(&fake, 0u, &createMessage, sizeof(createMessage));

    sender = 0u;
    length = sizeof(aReceived);
    TEST_ASSERT_EQUAL_INT(5, stdComm_Receive(&sender, aReceived, &length));
    TEST_ASSERT_EQUAL_UINT32(77u, sender);

    DPMSG_GENERIC hostMessage;
    STD_ZEROMEM(&hostMessage, sizeof(hostMessage));
    hostMessage.dwType = DPSYS_HOST;
    stdCommDX6TestFake_QueueMessage(&fake, 0u, &hostMessage, sizeof(hostMessage));

    sender = 0u;
    length = sizeof(aReceived);
    TEST_ASSERT_EQUAL_INT(8, stdComm_Receive(&sender, aReceived, &length));
    TEST_ASSERT_TRUE(stdComm_IsGameHost());

    fake.receiveResult = DPERR_GENERIC;
    length             = sizeof(aReceived);
    TEST_ASSERT_EQUAL_INT(-2, stdComm_Receive(&sender, aReceived, &length));
}

TEST(stdCommDX6, TestFakeDirectPlayPlayersAndErrorPaths)
{
    StdCommDX6TestFakeDirectPlay fake;

    stdCommDX6TestFake_Init(&fake);
    stdCommDX6TestFake_AddPlayer(&fake, 100u, L"Indy");
    stdCommDX6TestFake_AddPlayer(&fake, 200u, L"Sophia");
    stdComm_TestSetDirectPlayState(&fake.iface, true, false, 0u);

    TEST_ASSERT_EQUAL_INT(DP_OK, stdComm_UpdatePlayers(0u));
    TEST_ASSERT_EQUAL_size_t(2u, stdComm_GetNumPlayers());
    TEST_ASSERT_EQUAL_INT(0, stdComm_VerifyPlayer(100u));
    TEST_ASSERT_EQUAL_INT(0, stdComm_VerifyPlayer(200u));
    TEST_ASSERT_EQUAL_INT(1, stdComm_VerifyPlayer(300u));
    TEST_ASSERT_EQUAL_UINT32(100u, stdComm_GetPlayerID(0u));
    TEST_ASSERT_EQUAL_UINT32(200u, stdComm_GetPlayerID(1u));

    DPID createdId = stdComm_CreatePlayer(L"Marcus");
    TEST_ASSERT_EQUAL_UINT32(1000u, createdId);
    stdComm_DestroyPlayer(createdId);
    TEST_ASSERT_EQUAL_UINT32(createdId, fake.lastDestroyedPlayer);

    fake.createPlayerResult = DPERR_CANTCREATEPLAYER;
    TEST_ASSERT_EQUAL_UINT32(0u, stdComm_CreatePlayer(L"Failed"));

    fake.enumPlayersResult = DPERR_GENERIC;
    TEST_ASSERT_EQUAL_INT(DPERR_GENERIC, stdComm_UpdatePlayers(0u));
    TEST_ASSERT_EQUAL_size_t(0u, stdComm_GetNumPlayers());

    fake.sendResult = DPERR_SENDTOOBIG;
    TEST_ASSERT_EQUAL_INT(DPERR_SENDTOOBIG, stdComm_Send(1u, 2u, "too big", 7u, 0u));
}

TEST(stdCommDX6, TestFakeDirectPlayInactiveGameEnumerationAndSessionErrors)
{
    StdCommDX6TestFakeDirectPlay fake;
    StdCommGame outSettings;

    stdCommDX6TestFake_Init(&fake);
    stdCommDX6TestFake_AddPlayer(&fake, 55u, L"Short Round");
    stdComm_aGames[0].guid = stdCommDX6Test_guid;
    stdComm_TestSetDirectPlayState(&fake.iface, false, false, 1u);

    TEST_ASSERT_EQUAL_INT(DP_OK, stdComm_UpdatePlayers(0u));
    TEST_ASSERT_EQUAL_size_t(1u, stdComm_GetNumPlayers());
    TEST_ASSERT_EQUAL_UINT32(55u, stdComm_GetPlayerID(0u));
    TEST_ASSERT_EQUAL_INT(DPERR_GENERIC, stdComm_UpdatePlayers(1u));

    stdComm_TestSetDirectPlayState(&fake.iface, true, true, 0u);
    fake.getSessionDescQueryResult = DPERR_GENERIC;
    TEST_ASSERT_EQUAL_INT(DPERR_GENERIC, stdComm_GetSessionSettings(&outSettings));

    fake.getSessionDescQueryResult = DPERR_BUFFERTOOSMALL;
    fake.getSessionDescResult      = DPERR_GENERIC;
    TEST_ASSERT_EQUAL_INT(DPERR_GENERIC, stdComm_GetSessionSettings(&outSettings));
}

TEST(stdCommDX6, TestGetSessionDescriptorFailureCleansMemoryAndPreservesOutputs)
{
    StdCommDX6TestFakeDirectPlay fake;
    stdCommDX6TestFake_Init(&fake);
    stdComm_TestSetDirectPlayState(&fake.iface, true, false, 0u);
    StdCommGame settings;
    StdCommGame unchanged;
    STD_FILLMEM(&unchanged, 0xCD, sizeof(unchanged));
    settings = unchanged;

    // A failed descriptor read must release its buffer and leave every caller output unchanged.
    fake.getSessionDescResult = DPERR_GENERIC;
    TEST_ASSERT_EQUAL_INT(DPERR_GENERIC, stdComm_GetSessionSettings(&settings));
    TEST_ASSERT_EQUAL_MEMORY(&unchanged, &settings, sizeof(settings));
    TEST_ASSERT_EQUAL_size_t(0u, stdMemory_g_curState.totalAllocs);
    TEST_ASSERT_EQUAL_size_t(0u, stdMemory_g_curState.totalBytes);

    // A failed size query and allocation failure must also preserve the output and allocate nothing.
    fake.getSessionDescResult = DP_OK;
    fake.getSessionDescQueryResult = DPERR_GENERIC;
    TEST_ASSERT_EQUAL_INT(DPERR_GENERIC, stdComm_GetSessionSettings(&settings));
    TEST_ASSERT_EQUAL_MEMORY(&unchanged, &settings, sizeof(settings));
    TEST_ASSERT_EQUAL_size_t(0u, stdMemory_g_curState.totalAllocs);
    fake.getSessionDescQueryResult = DPERR_BUFFERTOOSMALL;
    StdGeneralTest_FailNextAllocations(0);
    TEST_ASSERT_EQUAL_INT(DPERR_OUTOFMEMORY, stdComm_GetSessionSettings(&settings));
    StdGeneralTest_ClearAllocationFailures();
    TEST_ASSERT_EQUAL_MEMORY(&unchanged, &settings, sizeof(settings));
    TEST_ASSERT_EQUAL_size_t(0u, stdMemory_g_curState.totalAllocs);
    TEST_ASSERT_EQUAL_size_t(0u, stdMemory_g_curState.totalBytes);

    // A successful retry must populate all settings and release the temporary descriptor.
    STD_ZEROMEM(&settings, sizeof(settings));
    StdCommGame expected;
    STD_ZEROMEM(&expected, sizeof(expected));
    expected.guid = stdCommDX6Test_guid;
    expected.maxPlayers = 8;
    expected.opt1 = 11;
    expected.opt2 = 22;
    expected.opt3 = 33;
    STD_WSTRCPY(expected.aSessionName, L"DX6 Local");
    // Set only the narrow string and terminator; conversion leaves the remaining capacity unchanged.
    memcpy(expected.aSomething, "arena01", sizeof("arena01"));
    TEST_ASSERT_EQUAL_INT(DP_OK, stdComm_GetSessionSettings(&settings));
    TEST_ASSERT_EQUAL_MEMORY(&expected, &settings, sizeof(settings));
    TEST_ASSERT_EQUAL_size_t(0u, stdMemory_g_curState.totalAllocs);
    TEST_ASSERT_EQUAL_size_t(0u, stdMemory_g_curState.totalBytes);
}

TEST(stdCommDX6, TestSetSessionDescriptorSuccessAndFailuresReleaseMemory)
{
    StdCommDX6TestFakeDirectPlay fake;
    stdCommDX6TestFake_Init(&fake);
    stdComm_TestSetDirectPlayState(&fake.iface, true, true, 0u);
    StdCommGame settings;
    STD_ZEROMEM(&settings, sizeof(settings));
    settings.maxPlayers = 4;
    settings.opt1 = 101;
    settings.opt2 = 202;
    settings.opt3 = 303;
    STD_WSTRCPY(settings.aSessionName, L"Session");
    STD_STRCPY(settings.aSomething, "arena");
    STD_WSTRCPY(settings.aPassword, L"secret");

    DPSESSIONDESC2 expectedDescriptor = fake.sessionDesc;
    expectedDescriptor.dwMaxPlayers = 4u;
    expectedDescriptor.dwUser1 = 101u;
    expectedDescriptor.dwUser3 = 202u;
    expectedDescriptor.dwUser4 = 303u;
    expectedDescriptor.dwFlags |= DPSESSION_DIRECTPLAYPROTOCOL | DPSESSION_KEEPALIVE
        | DPSESSION_NODATAMESSAGES | DPSESSION_NOPRESERVEORDER
        | DPSESSION_OPTIMIZELATENCY | DPSESSION_MIGRATEHOST;
    expectedDescriptor.lpszSessionName = fake.aSessionName;
    expectedDescriptor.lpszPassword = fake.aPassword;
    fake.lastSetSessionDescFlags = UINT32_MAX;

    // Verify the complete descriptor delivered through the DirectPlay function pointer and its cleanup.
    TEST_ASSERT_EQUAL_INT(DP_OK, stdComm_SetGameParams(&settings));
    TEST_ASSERT_EQUAL_size_t(0u, stdMemory_g_curState.totalAllocs);
    TEST_ASSERT_EQUAL_size_t(0u, stdMemory_g_curState.totalBytes);
    TEST_ASSERT_EQUAL_UINT32(1u, fake.setSessionDescCount);
    TEST_ASSERT_EQUAL_UINT32(0u, fake.lastSetSessionDescFlags);
    TEST_ASSERT_EQUAL_MEMORY(&expectedDescriptor, &fake.sessionDesc, sizeof(expectedDescriptor));
    TEST_ASSERT_EQUAL(0, wcscmp(L"Session:arena", fake.sessionDesc.lpszSessionName));
    TEST_ASSERT_EQUAL(0, wcscmp(L"secret", fake.sessionDesc.lpszPassword));
    TEST_ASSERT_EQUAL_UINT32(4u, fake.sessionDesc.dwMaxPlayers);
    TEST_ASSERT_EQUAL_UINT32(101u, fake.sessionDesc.dwUser1);
    TEST_ASSERT_EQUAL_UINT32(202u, fake.sessionDesc.dwUser3);
    TEST_ASSERT_EQUAL_UINT32(303u, fake.sessionDesc.dwUser4);
    TEST_ASSERT_EQUAL_MEMORY(&stdCommDX6Test_guid, &fake.sessionDesc.guidInstance, sizeof(GUID));

    // Reject query and read failures before calling SetSessionDesc, without leaking allocations.
    fake.getSessionDescQueryResult = DPERR_GENERIC;
    TEST_ASSERT_EQUAL_INT(DPERR_GENERIC, stdComm_SetGameParams(&settings));
    TEST_ASSERT_EQUAL_UINT32(1u, fake.setSessionDescCount);
    TEST_ASSERT_EQUAL_size_t(0u, stdMemory_g_curState.totalAllocs);
    fake.getSessionDescQueryResult = DPERR_BUFFERTOOSMALL;
    fake.getSessionDescResult = DPERR_GENERIC;
    TEST_ASSERT_EQUAL_INT(DPERR_GENERIC, stdComm_SetGameParams(&settings));
    TEST_ASSERT_EQUAL_UINT32(1u, fake.setSessionDescCount);
    TEST_ASSERT_EQUAL_size_t(0u, stdMemory_g_curState.totalAllocs);
    TEST_ASSERT_EQUAL_size_t(0u, stdMemory_g_curState.totalBytes);
    fake.getSessionDescResult = DP_OK;
    // A failed SetSessionDesc call must release the same temporary buffer.
    fake.setSessionDescResult = DPERR_GENERIC;
    TEST_ASSERT_EQUAL_INT(DPERR_GENERIC, stdComm_SetGameParams(&settings));
    TEST_ASSERT_EQUAL_size_t(0u, stdMemory_g_curState.totalAllocs);
    TEST_ASSERT_EQUAL_size_t(0u, stdMemory_g_curState.totalBytes);
    TEST_ASSERT_EQUAL_UINT32(2u, fake.setSessionDescCount);

    // Allocation failure must not call SetSessionDesc; a later retry must still succeed.
    StdGeneralTest_FailNextAllocations(0);
    TEST_ASSERT_EQUAL_INT(DPERR_OUTOFMEMORY, stdComm_SetGameParams(&settings));
    StdGeneralTest_ClearAllocationFailures();
    TEST_ASSERT_EQUAL_size_t(0u, stdMemory_g_curState.totalAllocs);
    TEST_ASSERT_EQUAL_size_t(0u, stdMemory_g_curState.totalBytes);
    TEST_ASSERT_EQUAL_UINT32(2u, fake.setSessionDescCount);
    // An empty requested password preserves the descriptor password, as in the existing API.
    settings.aPassword[0] = L'\0';
    fake.setSessionDescResult = DP_OK;
    TEST_ASSERT_EQUAL_INT(DP_OK, stdComm_SetGameParams(&settings));
    TEST_ASSERT_EQUAL(0, wcscmp(L"secret", fake.sessionDesc.lpszPassword));
    TEST_ASSERT_EQUAL_MEMORY(&expectedDescriptor, &fake.sessionDesc, sizeof(expectedDescriptor));
    TEST_ASSERT_EQUAL_size_t(0u, stdMemory_g_curState.totalAllocs);
    TEST_ASSERT_EQUAL_size_t(0u, stdMemory_g_curState.totalBytes);
}

TEST_GROUP_RUNNER(stdCommDX6)
{
    RUN_TEST_CASE(stdCommDX6, TestSetSessionDescriptorSuccessAndFailuresReleaseMemory);
    RUN_TEST_CASE(stdCommDX6, TestGetSessionDescriptorFailureCleansMemoryAndPreservesOutputs);
    RUN_TEST_CASE(stdCommDX6, TestPublicLimitsAndInactiveState);
    RUN_TEST_CASE(stdCommDX6, TestGetGameBoundsAndSlotZeroCopy);
    RUN_TEST_CASE(stdCommDX6, TestCloseGameWithoutDirectPlayAssertsAndLeavesInactiveState);
    RUN_TEST_CASE(stdCommDX6, TestUpdatePlayersWithoutDirectPlayRejectsMissingGame);
    RUN_TEST_CASE(stdCommDX6, TestFakeDirectPlayCreateGameSessionSettingsAndClose);
    RUN_TEST_CASE(stdCommDX6, TestFakeDirectPlayCreateGameFailureClearsState);
    RUN_TEST_CASE(stdCommDX6, TestFakeDirectPlaySendReceiveAndSystemMessages);
    RUN_TEST_CASE(stdCommDX6, TestFakeDirectPlayPlayersAndErrorPaths);
    RUN_TEST_CASE(stdCommDX6, TestFakeDirectPlayInactiveGameEnumerationAndSessionErrors);
}

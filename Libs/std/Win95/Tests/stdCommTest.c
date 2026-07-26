#include <unity_fixture.h>

#include <string.h>

#include <j3dcore/Tests/j3dTest.h>
#include <std/General/std.h>
#include <std/General/stdUtil.h>
#include <std/Win95/stdComm.h>

#include "stdGeneralTest.h"

extern StdCommGame stdComm_aGames[STDCOMM_MAX_GAMES];

TEST_GROUP(stdComm);

TEST_SETUP(stdComm)
{
    StdGeneralTest_Startup();
    memset(stdComm_aGames, 0, sizeof(stdComm_aGames));
    stdComm_CloseGame();
    J3DTest_ResetAssertCapture();
}

TEST_TEAR_DOWN(stdComm)
{
    stdComm_CloseGame();
    J3DTest_ResetAssertCapture();
    StdGeneralTest_Shutdown();
}

TEST(stdComm, TestInactiveStateAndEmptyPlayerTable)
{
    TEST_ASSERT_EQUAL_UINT32(24u, STDCOMM_MAX_PLAYERS);
    TEST_ASSERT_EQUAL_UINT32(32u, STDCOMM_MAX_GAMES);
    TEST_ASSERT_EQUAL_size_t(44u, sizeof(StdCommPlayerInfo));
    TEST_ASSERT_EQUAL_size_t(356u, sizeof(StdCommGame));
    TEST_ASSERT_FALSE(stdComm_IsGameActive());
    TEST_ASSERT_FALSE(stdComm_IsGameHost());
    TEST_ASSERT_EQUAL_size_t(0u, stdComm_GetNumPlayers());
    TEST_ASSERT_EQUAL_INT(1, stdComm_VerifyPlayer((DPID)1u));
}

TEST(stdComm, TestGetGameBoundsAndSlotZeroCopy)
{
    StdCommGame game;
    StdCommGame outGame;

    STD_ZEROMEM(&game, sizeof(game));
    STD_FILLMEM(&outGame, 0xCD, sizeof(outGame));

    game.maxPlayers        = 8;
    game.numCurrentPlayers = 2;
    game.opt1              = 11;
    game.opt2              = 22;
    game.opt3              = 33;
    STD_WSTRCPY(game.aSessionName, L"std test game");
    STD_WSTRCPY(game.aPassword, L"secret");
    STD_COPYMEM(&stdComm_aGames[0], &game, sizeof(game));

    TEST_ASSERT_EQUAL_INT(0, stdComm_GetGame(0u, &outGame));
    TEST_ASSERT_EQUAL_MEMORY(&game, &outGame, sizeof(game));

    STD_FILLMEM(&outGame, 0xCD, sizeof(outGame));
    TEST_ASSERT_EQUAL_INT(1, stdComm_GetGame(1u, &outGame));
    TEST_ASSERT_EQUAL_HEX8(0xCDu, ((uint8_t*)&outGame)[0]);

    STD_FILLMEM(&outGame, 0xCD, sizeof(outGame));
    TEST_ASSERT_EQUAL_INT(1, stdComm_GetGame(STDCOMM_MAX_GAMES, &outGame));
    TEST_ASSERT_EQUAL_HEX8(0xCDu, ((uint8_t*)&outGame)[0]);
}

TEST(stdComm, TestStubbedOperationsAssertAndReturnLegacyValues)
{
    DPID sender = 0;
    uint8_t data[8];
    size_t length = sizeof(data);

    J3DTest_ResetAssertCapture();
    TEST_ASSERT_EQUAL_INT(0, stdComm_Send(1u, 2u, data, sizeof(data), 0u));
    TEST_ASSERT_EQUAL_INT(1, J3DTest_GetAssertCount());
    J3DTest_AssertLastAssert("stdComm_Send not implemented!", "stdComm.c");

    J3DTest_ResetAssertCapture();
    TEST_ASSERT_EQUAL_INT(-1, stdComm_Receive(&sender, data, &length));
    TEST_ASSERT_EQUAL_INT(1, J3DTest_GetAssertCount());
    J3DTest_AssertLastAssert("stdComm_Receive not implemented!", "stdComm.c");

    J3DTest_ResetAssertCapture();
    TEST_ASSERT_EQUAL_INT(0, stdComm_CreatePlayer(L"Player"));
    TEST_ASSERT_EQUAL_INT(2, J3DTest_GetAssertCount());
    J3DTest_AssertLastAssert("stdComm_CreatePlayer not implemented!", "stdComm.c");
}

TEST(stdComm, TestStubbedNullArgumentsAssertBeforeLegacyReturn)
{
    J3DTest_ResetAssertCapture();
    TEST_ASSERT_EQUAL_INT(-1, stdComm_Receive(NULL, NULL, NULL));
    TEST_ASSERT_EQUAL_INT(2, J3DTest_GetAssertCount());
    J3DTest_AssertLastAssert("stdComm_Receive not implemented!", "stdComm.c");

    J3DTest_ResetAssertCapture();
    TEST_ASSERT_EQUAL_INT(0, stdComm_CreateGame(NULL));
    TEST_ASSERT_EQUAL_INT(2, J3DTest_GetAssertCount());
    J3DTest_AssertLastAssert("stdComm_CreateGame not implemented!", "stdComm.c");

    J3DTest_ResetAssertCapture();
    TEST_ASSERT_EQUAL_INT(0, stdComm_GetSessionSettings(NULL));
    TEST_ASSERT_EQUAL_INT(3, J3DTest_GetAssertCount());
    J3DTest_AssertLastAssert("stdComm_GetSessionSettings not implemented!", "stdComm.c");
}

TEST(stdComm, TestStubbedGameAndSessionOperations)
{
    StdCommGame game;
    DPID playerId = 0;

    STD_ZEROMEM(&game, sizeof(game));

    J3DTest_ResetAssertCapture();
    TEST_ASSERT_EQUAL_INT(0, stdComm_CreateGame(&game));
    TEST_ASSERT_EQUAL_INT(1, J3DTest_GetAssertCount());
    J3DTest_AssertLastAssert("stdComm_CreateGame not implemented!", "stdComm.c");

    J3DTest_ResetAssertCapture();
    TEST_ASSERT_EQUAL_INT(0, stdComm_SetGameParams(&game));
    TEST_ASSERT_EQUAL_INT(2, J3DTest_GetAssertCount());
    J3DTest_AssertLastAssert("stdComm_SetGameParams not implemented!", "stdComm.c");

    J3DTest_ResetAssertCapture();
    TEST_ASSERT_EQUAL_INT(0, stdComm_JoinGame(0u, L""));
    TEST_ASSERT_EQUAL_INT(2, J3DTest_GetAssertCount());
    J3DTest_AssertLastAssert("stdComm_JoinGame not implemented!", "stdComm.c");

    J3DTest_ResetAssertCapture();
    TEST_ASSERT_EQUAL_INT(0, stdComm_RejoinSession(&playerId, L"Player"));
    TEST_ASSERT_EQUAL_INT(2, J3DTest_GetAssertCount());
    J3DTest_AssertLastAssert("stdComm_RejoinSession not implemented!", "stdComm.c");
    TEST_ASSERT_TRUE(stdComm_IsGameActive());
    TEST_ASSERT_FALSE(stdComm_IsGameHost());

    J3DTest_ResetAssertCapture();
    TEST_ASSERT_EQUAL_INT(0, stdComm_GetSessionSettings(&game));
    TEST_ASSERT_EQUAL_INT(1, J3DTest_GetAssertCount());
    J3DTest_AssertLastAssert("stdComm_GetSessionSettings not implemented!", "stdComm.c");
}

TEST(stdComm, TestRejoinSessionChangesCreatePlayerAssertPath)
{
    DPID playerId = 0;

    J3DTest_ResetAssertCapture();
    TEST_ASSERT_EQUAL_INT(0, stdComm_RejoinSession(&playerId, L"Player"));
    TEST_ASSERT_EQUAL_INT(2, J3DTest_GetAssertCount());
    J3DTest_AssertLastAssert("stdComm_RejoinSession not implemented!", "stdComm.c");
    TEST_ASSERT_TRUE(stdComm_IsGameActive());

    J3DTest_ResetAssertCapture();
    TEST_ASSERT_EQUAL_UINT32(0u, stdComm_CreatePlayer(L"Player"));
    TEST_ASSERT_EQUAL_INT(1, J3DTest_GetAssertCount());
    J3DTest_AssertLastAssert("stdComm_CreatePlayer not implemented!", "stdComm.c");

    J3DTest_ResetAssertCapture();
    TEST_ASSERT_EQUAL_INT(0, stdComm_RejoinSession(&playerId, L"Player"));
    TEST_ASSERT_EQUAL_INT(1, J3DTest_GetAssertCount());
    J3DTest_AssertLastAssert("stdComm_RejoinSession not implemented!", "stdComm.c");
}

TEST(stdComm, TestStubbedPlayerHelpers)
{
    J3DTest_ResetAssertCapture();
    TEST_ASSERT_EQUAL_INT(-1, stdComm_UpdatePlayers(0u));
    TEST_ASSERT_EQUAL_INT(1, J3DTest_GetAssertCount());
    J3DTest_AssertLastAssert("stdComm_UpdatePlayers not implemented!", "stdComm.c");

    J3DTest_ResetAssertCapture();
    stdComm_DestroyPlayer(0u);
    TEST_ASSERT_EQUAL_INT(2, J3DTest_GetAssertCount());
    J3DTest_AssertLastAssert("stdComm_DestroyPlayer not implemented!", "stdComm.c");

    J3DTest_ResetAssertCapture();
    stdComm_DestroyPlayer(1u);
    TEST_ASSERT_EQUAL_INT(1, J3DTest_GetAssertCount());
    J3DTest_AssertLastAssert("stdComm_DestroyPlayer not implemented!", "stdComm.c");

    TEST_ASSERT_EQUAL_UINT32(0u, stdComm_GetPlayerID(0u));
}

TEST(stdComm, TestCloseGameClearsStateAndReportsStub)
{
    J3DTest_ResetAssertCapture();
    stdComm_CloseGame();

    TEST_ASSERT_FALSE(stdComm_IsGameActive());
    TEST_ASSERT_FALSE(stdComm_IsGameHost());
    TEST_ASSERT_EQUAL_INT(1, J3DTest_GetAssertCount());
    J3DTest_AssertLastAssert("stdComm_CloseGame not implemented!", "stdComm.c");
}

TEST_GROUP_RUNNER(stdComm)
{
    RUN_TEST_CASE(stdComm, TestInactiveStateAndEmptyPlayerTable);
    RUN_TEST_CASE(stdComm, TestGetGameBoundsAndSlotZeroCopy);
    RUN_TEST_CASE(stdComm, TestStubbedOperationsAssertAndReturnLegacyValues);
    RUN_TEST_CASE(stdComm, TestStubbedNullArgumentsAssertBeforeLegacyReturn);
    RUN_TEST_CASE(stdComm, TestStubbedGameAndSessionOperations);
    RUN_TEST_CASE(stdComm, TestRejoinSessionChangesCreatePlayerAssertPath);
    RUN_TEST_CASE(stdComm, TestStubbedPlayerHelpers);
    RUN_TEST_CASE(stdComm, TestCloseGameClearsStateAndReportsStub);
}

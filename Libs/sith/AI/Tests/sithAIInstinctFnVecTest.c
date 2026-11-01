#include <unity_fixture.h>

#include "sithAIInstinctFnVec.h"

#include "sithVecTest.h"

// Original-engine test vectors for the instinct functions of sithAIInstinct.c (everything but
// sithAIInstinct_InitInstincts). The fixture in tv/sithAIInstinctFn holds, for every case of sithAIFnVec_aCases, the result
// of the retail v1.0 function: its return value, the engine calls it made (with their arguments) and every change of the
// AI state (control block, things, instinct, instinct state and the snake statics) after every call. The tests rebuild the
// same state, run the reconstructed function and compare. See tv/sithAIInstinctFn/README.md.

// Engine globals the instincts read (redirected, see sithAIInstinctFnVecAddrs.h)
SITHVEC_REDIRECT_DEFINE(uint32_t, sithTime_g_msecGameTime);
SITHVEC_REDIRECT_DEFINE(float, sithTime_g_secGameTime);
SITHVEC_REDIRECT_DEFINE(size_t, sithMain_g_frameNumber);
SITHVEC_REDIRECT_DEFINE(SithMode, sithMain_g_sith_mode);
SITHVEC_REDIRECT_DEFINE(SithThing*, sithPlayer_g_pLocalPlayerThing);
SITHVEC_REDIRECT_DEFINE(int, sithPlayerControls_g_bCutsceneMode);
SITHVEC_REDIRECT_DEFINE(SithAIAwarenessSector*, sithAIAwareness_g_aSectors);

static SithVecTestFixture sithAIInstinctFnVecTest_fixture;
static char sithAIInstinctFnVecTest_aFailText[8192];

TEST_GROUP(sithAIInstinctFn);

TEST_SETUP(sithAIInstinctFn)
{
    sithVecTest_AssertArenaPlacement();
    sith_g_pHS = sithVec_InitHostServices();
    sithVecTest_LoadFixture(&sithAIInstinctFnVecTest_fixture, SITHVEC_TV_DIR, sithAIFnVec_suite.pName);
}

TEST_TEAR_DOWN(sithAIInstinctFn)
{
    sithVecTest_FreeFixture(&sithAIInstinctFnVecTest_fixture);
    sith_g_pHS = NULL;
}

// Every case of the table is in the fixture and the other way around
TEST(sithAIInstinctFn, TestFixtureCoversAllCases)
{
    sithVecTest_AssertFixtureCoversSuite(&sithAIInstinctFnVecTest_fixture, &sithAIFnVec_suite);
}

#define SITHAIFN_TEST(fn)                                                                                                     \
    TEST(sithAIInstinctFn, TestOriginal##fn)                                                                                  \
    {                                                                                                                         \
        unsigned int numFailed = sithVecTest_RunCases(&sithAIFnVec_suite, &sithAIInstinctFnVecTest_fixture, #fn "_",          \
            sithAIInstinctFnVecTest_aFailText, sizeof(sithAIInstinctFnVecTest_aFailText));                                    \
        TEST_ASSERT_EQUAL_UINT_MESSAGE(0, numFailed, sithAIInstinctFnVecTest_aFailText);                                      \
    }

SITHAIFN_TEST(Listen)
SITHAIFN_TEST(LookForTarget)
SITHAIFN_TEST(SenseDanger)
SITHAIFN_TEST(FearGunshot)
SITHAIFN_TEST(Hop)
SITHAIFN_TEST(HoverDrift)
SITHAIFN_TEST(Talk)
SITHAIFN_TEST(RandomTurn)
SITHAIFN_TEST(OpenDoors)
SITHAIFN_TEST(Jump)
SITHAIFN_TEST(LobFire)
SITHAIFN_TEST(PrimaryFire)
SITHAIFN_TEST(AlternateFire)
SITHAIFN_TEST(CircleStrafe)
SITHAIFN_TEST(Dodge)
SITHAIFN_TEST(HitAndRun)
SITHAIFN_TEST(Retreat)
SITHAIFN_TEST(RandomMove)
SITHAIFN_TEST(HumanCombatMove)
SITHAIFN_TEST(Roam)
SITHAIFN_TEST(ReturnHome)
SITHAIFN_TEST(Flee)
SITHAIFN_TEST(BasicFallow)
SITHAIFN_TEST(WallCrawl)
SITHAIFN_TEST(SnakeFollow)
SITHAIFN_TEST(SnakeFindMovePos)
SITHAIFN_TEST(SnakeMungeTestCheck)
SITHAIFN_TEST(SnakePlaySlitherSound)

TEST_GROUP_RUNNER(sithAIInstinctFn)
{
    RUN_TEST_CASE(sithAIInstinctFn, TestFixtureCoversAllCases);
    RUN_TEST_CASE(sithAIInstinctFn, TestOriginalListen);
    RUN_TEST_CASE(sithAIInstinctFn, TestOriginalLookForTarget);
    RUN_TEST_CASE(sithAIInstinctFn, TestOriginalSenseDanger);
    RUN_TEST_CASE(sithAIInstinctFn, TestOriginalFearGunshot);
    RUN_TEST_CASE(sithAIInstinctFn, TestOriginalHop);
    RUN_TEST_CASE(sithAIInstinctFn, TestOriginalHoverDrift);
    RUN_TEST_CASE(sithAIInstinctFn, TestOriginalTalk);
    RUN_TEST_CASE(sithAIInstinctFn, TestOriginalRandomTurn);
    RUN_TEST_CASE(sithAIInstinctFn, TestOriginalOpenDoors);
    RUN_TEST_CASE(sithAIInstinctFn, TestOriginalJump);
    RUN_TEST_CASE(sithAIInstinctFn, TestOriginalLobFire);
    RUN_TEST_CASE(sithAIInstinctFn, TestOriginalPrimaryFire);
    RUN_TEST_CASE(sithAIInstinctFn, TestOriginalAlternateFire);
    RUN_TEST_CASE(sithAIInstinctFn, TestOriginalCircleStrafe);
    RUN_TEST_CASE(sithAIInstinctFn, TestOriginalDodge);
    RUN_TEST_CASE(sithAIInstinctFn, TestOriginalHitAndRun);
    RUN_TEST_CASE(sithAIInstinctFn, TestOriginalRetreat);
    RUN_TEST_CASE(sithAIInstinctFn, TestOriginalRandomMove);
    RUN_TEST_CASE(sithAIInstinctFn, TestOriginalHumanCombatMove);
    RUN_TEST_CASE(sithAIInstinctFn, TestOriginalRoam);
    RUN_TEST_CASE(sithAIInstinctFn, TestOriginalReturnHome);
    RUN_TEST_CASE(sithAIInstinctFn, TestOriginalFlee);
    RUN_TEST_CASE(sithAIInstinctFn, TestOriginalBasicFallow);
    RUN_TEST_CASE(sithAIInstinctFn, TestOriginalWallCrawl);
    RUN_TEST_CASE(sithAIInstinctFn, TestOriginalSnakeFollow);
    RUN_TEST_CASE(sithAIInstinctFn, TestOriginalSnakeFindMovePos);
    RUN_TEST_CASE(sithAIInstinctFn, TestOriginalSnakeMungeTestCheck);
    RUN_TEST_CASE(sithAIInstinctFn, TestOriginalSnakePlaySlitherSound);
}

void SithVecTest_RunAllTests(void)
{
    RUN_TEST_GROUP(sithAIInstinctFn);
}

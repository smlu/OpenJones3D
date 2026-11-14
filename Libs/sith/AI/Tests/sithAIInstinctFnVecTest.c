#include <unity_fixture.h>

#include "sithAIInstinctFnVec.h"

#include "sithVecTest.h"
#include "sithAIFixTest.h"

// Original-engine test vectors for the instinct functions of sithAIInstinct.c (everything but
// sithAIInstinct_InitInstincts). The fixture in tv/sithAIInstinctFn holds, for every case of sithAIFnVec_aCases, the result
// of the retail v1.0 function: its return value, the engine calls it made (with their arguments) and every change of the
// AI state (control block, things, instinct, instinct state and the snake statics) after every call. The tests rebuild the
// same state, run the reconstructed function and compare. See tv/sithAIInstinctFn/README.md.

// Engine globals the instincts read (redirected, see sithAIInstinctFnVecAddrs.h)
SITHVEC_REDIRECT_DEFINE(uint32_t, sithTime_g_msecGameTime);
// The frame time of a 30 FPS frame: the original vectors were captured without one, and sithTime_CalcDeadline puts
// deadlines on 30 FPS frames only for shorter frames, so the deadlines stay the original now + interval
SITHVEC_REDIRECT_DEFINE(uint32_t, sithTime_g_frameTime) = 1000u / 30u;
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

#ifdef J3D_QOL_IMPROVEMENTS
#include <sith/Gameplay/sithTime.h>

// Runs a case and returns the deadline it leaves: the jump and combat-move instincts keep it in state param 0 (float msec),
// lob fire in the AI's fire wait time
static float sithAIInstinctFnVecTest_RunForDeadline(const SithVecCase* pCase, uint32_t msecFrameTime, int bFireWait)
{
    sithVecTest_g_sithTime_g_frameTime = msecFrameTime;
    SithVecBuf rec = { 0 };
    char aWhy[128];
    const int bStable = sithVec_RunCase(pCase, &rec, aWhy, sizeof(aWhy));
    sithVecB_Free(&rec);
    TEST_ASSERT_TRUE_MESSAGE(bStable, pCase->pName);
    return bFireWait ? (float)SITHAIFN_BLOCK->msecFireWaitTime : SITHAIFN_USER->state.aParams[0];
}

TEST(sithAIInstinctFn, TestDeadlinesLandOn30FpsFramesAtHigherFrameRates)
{
    // At 144 FPS the next jump time, the next combat move time and the lob-fire wait the real instincts set lie on the start
    // of a 30 FPS frame, within a 30 FPS frame of the deadline they set at 30 FPS (where the vectors prove them original).
    static const struct { const char* pPrefix; int bFireWait; } aChecks[] = { { "Jump_", 0 }, { "HumanCombatMove_", 0 }, { "LobFire_", 1 } };
    unsigned int numChanged = 0;
    for ( size_t c = 0; c < STD_ARRAYLEN(aChecks); ++c )
    {
        for ( size_t k = 0; k < sithAIFnVec_suite.numCases; ++k )
        {
            const SithVecCase* pCase = &sithAIFnVec_suite.aCases[k];
            if ( strncmp(pCase->pName, aChecks[c].pPrefix, strlen(aChecks[c].pPrefix)) != 0 )
            {
                continue;
            }

            const float at30Fps  = sithAIInstinctFnVecTest_RunForDeadline(pCase, 1000u / 30u, aChecks[c].bFireWait);
            const float at144Fps = sithAIInstinctFnVecTest_RunForDeadline(pCase, 7u, aChecks[c].bFireWait);
            if ( at30Fps == at144Fps )
            {
                continue; // The case didn't set the deadline, or set a whole number of 30 FPS frames
            }

            ++numChanged;
            TEST_ASSERT_FLOAT_WITHIN_MESSAGE(34.0f, at30Fps, at144Fps, pCase->pName);
            const uint32_t msecDeadline = (uint32_t)at144Fps;
            TEST_ASSERT_EQUAL_FLOAT_MESSAGE((float)msecDeadline, at144Fps, pCase->pName); // a whole msec
            TEST_ASSERT_EQUAL_UINT32_MESSAGE(sithTime_GetReferenceFrameTime(sithTime_GetReferenceFrame(msecDeadline)), msecDeadline, pCase->pName); // a 30 FPS frame start
        }
    }

    sithVecTest_g_sithTime_g_frameTime = 1000u / 30u;
    TEST_ASSERT_TRUE(numChanged > 0);
}
#endif // J3D_QOL_IMPROVEMENTS
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
#ifdef J3D_QOL_IMPROVEMENTS
    RUN_TEST_CASE(sithAIInstinctFn, TestDeadlinesLandOn30FpsFramesAtHigherFrameRates);
#endif
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

// The original fixture intentionally masks undefined outputs. These tests state
// the fixed contract independently, without changing the original captures.
TEST_GROUP(sithAIInstinctFix);

TEST_SETUP(sithAIInstinctFix)
{
    sithVecTest_AssertArenaPlacement();
    sith_g_pHS = sithVec_InitHostServices();
}

TEST_TEAR_DOWN(sithAIInstinctFix)
{
    sithAIFixTest_Shutdown();
}

static void sithAIInstinctFix_Prepare(const SithAIFnParams* pParams)
{
    SithVecCase testCase = { "fixed instinct behavior", NULL, 0, pParams };
    sithAIFixTest_Prepare(&sithAIFnVec_suite, &testCase);
}

TEST(sithAIInstinctFix, RandomTurnReportsHandledVisiblePoint)
{
    SithAIFnParams params = { 0 };
    params.fn = SITHAIFN_RANDOMTURN;
    params.mode = SITHAI_MODE_SEARCHING;
    params.args[0] = 2000.0f;
    params.args[1] = 5.0f;
    params.rnd = "M";
    params.stubs.sightPos = SITHAIFN_ALL(SITHAI_SIGHT_VISIBLE);
    sithAIInstinctFix_Prepare(&params);

    TEST_ASSERT_EQUAL_INT(1, sithAIInstinct_RandomTurn(SITHAIFN_BLOCK, &SITHAIFN_USER->instinct, &SITHAIFN_USER->state, 0, NULL));
    TEST_ASSERT_EQUAL_UINT(1, sithAIFnVec_observed.numLookEye);
    TEST_ASSERT_EQUAL_PTR(SITHAIFN_BLOCK, sithAIFnVec_observed.pLookLocal);
    TEST_ASSERT_EQUAL_MEMORY(&sithAIFnVec_observed.sightEndPos, &sithAIFnVec_observed.lookEyePos, sizeof(rdVector3));
    TEST_ASSERT_EQUAL_UINT32(102000, SITHAIFN_USER->state.msecNextUpdate);
    TEST_ASSERT_EQUAL_INT(SITHAI_MODE_SEARCHING, SITHAIFN_BLOCK->mode);
}

TEST(sithAIInstinctFix, RandomTurnPreservesOtherHandledResults)
{
    SithAIFnParams params = { 0 };
    params.fn = SITHAIFN_RANDOMTURN;
    params.mode = SITHAI_MODE_ATTACKING;
    params.args[1] = 5.0f;
    sithAIInstinctFix_Prepare(&params);
    TEST_ASSERT_EQUAL_INT(0, sithAIInstinct_RandomTurn(SITHAIFN_BLOCK, &SITHAIFN_USER->instinct, &SITHAIFN_USER->state, 0, NULL));
    TEST_ASSERT_EQUAL_UINT(0, sithAIFnVec_ctr.sightPos);
    TEST_ASSERT_EQUAL_UINT(0, sithAIFnVec_observed.numLookEye);
    TEST_ASSERT_EQUAL_UINT32(105000, SITHAIFN_USER->state.msecNextUpdate);

    static const SithAISightState aSight[] = { SITHAI_SIGHT_TOOFAR, SITHAI_SIGHT_OUTOFFOV, SITHAI_SIGHT_HIDDEN };
    for ( size_t i = 0; i < SITHVEC_ARRAYLEN(aSight); ++i )
    {
        params.mode = SITHAI_MODE_SEARCHING;
        params.stubs.sightPos = SITHAIFN_ALL(aSight[i]);
        sithAIInstinctFix_Prepare(&params);
        TEST_ASSERT_EQUAL_INT(aSight[i], sithAIInstinct_RandomTurn(SITHAIFN_BLOCK, &SITHAIFN_USER->instinct, &SITHAIFN_USER->state, 0, NULL));
        TEST_ASSERT_EQUAL_UINT(1, sithAIFnVec_ctr.sightPos);
        TEST_ASSERT_EQUAL_UINT(0, sithAIFnVec_observed.numLookEye);
    }
}

TEST(sithAIInstinctFix, PrimaryFireBerserkBurstBoundaries)
{
    // +/-6 previously divides by zero. +/-5 has divisor one, so the neighboring
    // randomized result is exactly +/-25, independent of the random seed.
    static const int aAuthored[] = { 6, -6, 5, -5 };
    static const int aExpected[] = { 6, -6, 25, -25 };
    for ( size_t i = 0; i < SITHVEC_ARRAYLEN(aAuthored); ++i )
    {
        SithAIFnParams params = { 0 };
        params.fn = SITHAIFN_PRIMARYFIRE;
        params.mode = SITHAI_MODE_ATTACKING | SITHAI_MODE_BERSERK;
        params.targetRole = SITHAIFN_R_TARGET;
        params.args[1] = 0.75f;
        params.args[2] = 30.0f;
        params.args[3] = 0.125f;
        params.args[4] = 1.0f;
        params.args[8] = (float)aAuthored[i];
        params.args[11] = 2.0f;
        params.args[16] = 1.0f;
        params.stubs.canAttack = SITHAIFN_ALL(1);
        params.stubs.fire = SITHAIFN_ALL(1);
        sithAIInstinctFix_Prepare(&params);

        TEST_ASSERT_EQUAL_INT(0, sithAIInstinct_PrimaryFire(SITHAIFN_BLOCK, &SITHAIFN_USER->instinct, &SITHAIFN_USER->state, 0, NULL));
        TEST_ASSERT_EQUAL_UINT(1, sithAIFnVec_observed.numFire);
        TEST_ASSERT_EQUAL_INT(aExpected[i], sithAIFnVec_observed.burstCount);
        TEST_ASSERT_EQUAL_INT(2, sithAIFnVec_observed.weaponNum);
        TEST_ASSERT_EQUAL_FLOAT(1.0f, sithAIFnVec_observed.minFireDist);
        TEST_ASSERT_EQUAL_FLOAT(30.0f, sithAIFnVec_observed.maxFireDist);
        TEST_ASSERT_EQUAL_FLOAT(0.75f, sithAIFnVec_observed.fireDot);
        TEST_ASSERT_EQUAL_FLOAT(0.125f, sithAIFnVec_observed.aimError);
        TEST_ASSERT_EQUAL_UINT32(SITHAIUTIL_FIRE_PRIMARY | (2u << 16), sithAIFnVec_observed.fireFlags);
        TEST_ASSERT_EQUAL_UINT32(100000, SITHAIFN_USER->state.msecNextUpdate);
    }
}

#ifdef J3D_QOL_IMPROVEMENTS
TEST(sithAIInstinctFix, ListenHuntingUsesTransmittedLoudness)
{
    static const SithAIMode aModes[] = { SITHAI_MODE_HUNTING, SITHAI_MODE_INSTINCTUSEWPNTS };
    static const float aKnown[] = { 0.5f, 3.0f };
    static const float aExpected[] = { 2.0f, 3.0f };
    for ( size_t i = 0; i < SITHVEC_ARRAYLEN(aModes); ++i )
    {
        for ( size_t j = 0; j < SITHVEC_ARRAYLEN(aKnown); ++j )
        {
            SithAIFnParams params = { 0 };
            params.fn = SITHAIFN_LISTEN;
            params.mode = SITHAI_MODE_SEARCHING | aModes[i];
            params.args[0] = params.args[1] = 1.0f;
            params.st[0] = aKnown[j];
            params.aw.level[SITHAIAWARENESS_NOISE] = 2.0f;
            params.aw.thing[SITHAIAWARENESS_NOISE] = SITHAIFN_R_OTHER;
            params.stubs.inRange = SITHAIFN_ALL(1);
            params.stubs.sightPos = SITHAIFN_ALL(SITHAI_SIGHT_HIDDEN);
            params.stubs.dist[0] = params.stubs.dist[1] = 5.0f;
            params.rnd = "LL";
            sithAIInstinctFix_Prepare(&params);

            TEST_ASSERT_EQUAL_INT(0, sithAIInstinct_Listen(SITHAIFN_BLOCK, &SITHAIFN_USER->instinct, &SITHAIFN_USER->state, SITHAI_EVENT_SOUND, NULL));
            TEST_ASSERT_EQUAL_FLOAT(aExpected[j], SITHAIFN_USER->state.aParams[0]);
            TEST_ASSERT_EQUAL_PTR(sithPlayer_g_pLocalPlayerThing, SITHAIFN_BLOCK->pTargetThing);
            TEST_ASSERT_EQUAL_UINT32(SITHAI_MODE_SEARCHING | aModes[i] | SITHAI_MODE_HUNTING, SITHAIFN_BLOCK->mode);
            static const uint32_t aCallbacks[] = { SITHAIFN_EV_PLAYSOUNDMODE, SITHAIFN_EV_SETMODE, SITHAIFN_EV_NEXTWPNT };
            TEST_ASSERT_EQUAL_UINT(3, sithAIFnVec_observed.numCallbacks);
            TEST_ASSERT_EQUAL_MEMORY(aCallbacks, sithAIFnVec_observed.aCallbacks, sizeof(aCallbacks));
            TEST_ASSERT_EQUAL_FLOAT(1.7f, sithAIFnVec_observed.waypointSpeed);
            TEST_ASSERT_EQUAL_INT(0, sithAIFnVec_observed.waypointMode);
            TEST_ASSERT_EQUAL_UINT(0, sithAIFnVec_observed.numLookEye);
            TEST_ASSERT_EQUAL_UINT(2, sithAIFnVec_ctr.dist);
        }
    }
}

TEST(sithAIInstinctFix, SnakeMungePassesOnlyIntendedPathFlags)
{
    SithAIFnParams params = { 0 };
    params.fn = SITHAIFN_SNAKEMUNGETESTCHECK;
    params.quetz = 1;
    params.stubs.quetz = SITHAIFN_ALL(1);
    params.stubs.checkPath = SITHAIFN_ALL(1);
    params.stubs.height = 0.125f;
    sithAIInstinctFix_Prepare(&params);
    rdVector3 direction = { 1.0f, 0.0f, 0.0f };
    rdVector3 output = { -1.0f, -2.0f, -3.0f };
    TEST_ASSERT_EQUAL_INT(1, sithAIInstinct_SnakeMungeTestCheck(SITHAIFN_BLOCK, &direction, 2.0f, &output));
    TEST_ASSERT_EQUAL_UINT(1, sithAIFnVec_observed.numPath);
    TEST_ASSERT_EQUAL_UINT32(SITHAIUTIL_PATHFLAG_IGNOREPLAYER | SITHAIUTIL_PATHFLAG_IGNORETHINGS | SITHAIUTIL_PATHFLAG_GOALVISIBLE | SITHAIUTIL_PATHFLAG_STRAIGHT, sithAIFnVec_observed.pathFlags);
    TEST_ASSERT_EQUAL_MEMORY(&direction, &sithAIFnVec_observed.pathDirection, sizeof(direction));
    TEST_ASSERT_EQUAL_FLOAT(0.0f, sithAIFnVec_observed.minMoveDist);
    TEST_ASSERT_EQUAL_FLOAT(2.05f, sithAIFnVec_observed.moveDist);
    TEST_ASSERT_EQUAL_FLOAT(0.3f, sithAIFnVec_observed.radius);
    TEST_ASSERT_EQUAL_FLOAT(0.177f, sithAIFnVec_observed.pathStart.z);
    rdVector3 expected = { 2.0f, 0.0f, 0.0f };
    TEST_ASSERT_EQUAL_MEMORY(&expected, &output, sizeof(output));
}
#endif

TEST_GROUP_RUNNER(sithAIInstinctFix)
{
    RUN_TEST_CASE(sithAIInstinctFix, RandomTurnReportsHandledVisiblePoint);
    RUN_TEST_CASE(sithAIInstinctFix, RandomTurnPreservesOtherHandledResults);
    RUN_TEST_CASE(sithAIInstinctFix, PrimaryFireBerserkBurstBoundaries);
#ifdef J3D_QOL_IMPROVEMENTS
    RUN_TEST_CASE(sithAIInstinctFix, ListenHuntingUsesTransmittedLoudness);
    RUN_TEST_CASE(sithAIInstinctFix, SnakeMungePassesOnlyIntendedPathFlags);
#endif
}

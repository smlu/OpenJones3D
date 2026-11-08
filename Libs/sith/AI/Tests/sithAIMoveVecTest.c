#include <unity_fixture.h>

#include "sithAIMoveVec.h"

#include "sithVecTest.h"
#include "sithAIFixTest.h"

// Original-engine test vectors for the sithAIMove functions. The fixture in tv/sithAIMove holds, for every case of
// sithAIMoveVec_aCases, the result of the retail v1.0 functions (return values, the engine calls they made and the
// bytes they changed in the synthetic state, call by call). The tests rebuild the same state, run the reconstructed
// functions and compare. See tv/sithAIMove/README.md.

SITHVEC_REDIRECT_DEFINE(uint32_t, sithTime_g_msecGameTime);
SITHVEC_REDIRECT_DEFINE(size_t, sithMain_g_frameNumber);
SITHVEC_REDIRECT_DEFINE(SithMode, sithMain_g_sith_mode);

static SithVecTestFixture sithAIMoveVecTest_fixture;
static char sithAIMoveVecTest_aFailText[8192];

TEST_GROUP(sithAIMove);

TEST_SETUP(sithAIMove)
{
    sithVecTest_AssertArenaPlacement();
    sith_g_pHS = sithVec_InitHostServices();
    sithVecTest_LoadFixture(&sithAIMoveVecTest_fixture, SITHVEC_TV_DIR, sithAIMoveVec_suite.pName);
}

TEST_TEAR_DOWN(sithAIMove)
{
    sithVecTest_FreeFixture(&sithAIMoveVecTest_fixture);
    sith_g_pHS = NULL;
}

// Every case of the table is in the fixture and the other way around
TEST(sithAIMove, TestFixtureCoversAllCases)
{
    sithVecTest_AssertFixtureCoversSuite(&sithAIMoveVecTest_fixture, &sithAIMoveVec_suite);
}

static void sithAIMoveVecTest_CheckResult(unsigned int numFailed)
{
#ifdef J3D_QOL_IMPROVEMENTS
    // The QOL fix of sithAIMove_UpdateMineCar queries the actor move state, changing its resulting status and state/calls;
    // the legacy build matches the original, see tv/sithAIMove/README.md
    static const char* const aKnown[] = {
        "Update_turn_minecar",
        "UpdateMineCar_turn_right",
        "UpdateMineCar_turn_left",
        "UpdateMineCar_turn_behind",
        "UpdateMineCar_slowzone",
        "UpdateMineCar_turn_45",
        "UpdateMineCar_mesh0",
        "UpdateMineCar_tilted",
        "UpdateMineCar_random_tweak_right",
        "UpdateMineCar_random_tweak_left",
        "UpdateMineCar_edge_limit_below",
        "UpdateMineCar_zero_dt",
        "UpdateMineCar_zero_dt_behind",
        "UpdateMineCar_negative_rate",
    };
    sithVecTest_ExpectKnownDifferences(numFailed, sithAIMoveVecTest_aFailText, aKnown, (unsigned int)SITHVEC_ARRAYLEN(aKnown),
        "sithAIMove_UpdateMineCar queries the move state (QOL); see tv/sithAIMove/README.md.");
#else
    TEST_ASSERT_EQUAL_UINT_MESSAGE(0, numFailed, sithAIMoveVecTest_aFailText);
#endif
}

#define SITHAIMOVEVEC_TEST(name, prefix)                                                                                       \
    TEST(sithAIMove, TestOriginal##name)                                                                                       \
    {                                                                                                                          \
        unsigned int numFailed = sithVecTest_RunCases(&sithAIMoveVec_suite, &sithAIMoveVecTest_fixture, prefix,                \
            sithAIMoveVecTest_aFailText, sizeof(sithAIMoveVecTest_aFailText));                                                 \
        sithAIMoveVecTest_CheckResult(numFailed);                                                                              \
    }

SITHAIMOVEVEC_TEST_LIST(SITHAIMOVEVEC_TEST)

TEST_GROUP_RUNNER(sithAIMove)
{
    RUN_TEST_CASE(sithAIMove, TestFixtureCoversAllCases);
#define SITHAIMOVEVEC_RUN(name, prefix) RUN_TEST_CASE(sithAIMove, TestOriginal##name);
    SITHAIMOVEVEC_TEST_LIST(SITHAIMOVEVEC_RUN)
}

#ifdef J3D_QOL_IMPROVEMENTS
TEST_GROUP(sithAIMoveFix);

TEST_SETUP(sithAIMoveFix)
{
    sithVecTest_AssertArenaPlacement();
    sith_g_pHS = sithVec_InitHostServices();
}

TEST_TEAR_DOWN(sithAIMoveFix)
{
    sithAIFixTest_Shutdown();
}

static void sithAIMoveFix_Prepare(SithThingMoveStatus status, float lookX)
{
    SithAIMoveVecParams params = { 0 };
    params.mode = SITHAI_MODE_TURNING | SITHAI_MODE_SEARCHING;
    params.moveStatus = status;
    params.zeroTweak = 1;
    params.lookPos.x = lookX;
    params.lookPos.z = 0.125f; // level with joint zero; removes unrelated elevation
    SithVecCase testCase = { "fixed mine-car behavior", NULL, 0, &params };
    sithAIFixTest_Prepare(&sithAIMoveVec_suite, &testCase);
}

static void sithAIMoveFix_AssertMeshCall(const SithThing* pOwner)
{
    // The query is the only external callback on these paths. Unexpected puppet,
    // sound, collision or AI callbacks must not be hidden by a fixture ignore.
    uint8_t expected[9] = { SITHAIMOVEVEC_EV_MESHINDEX };
    uint32_t args[2] = { SITHVEC_P(pOwner), sithVec_Hash("redarmy") };
    memcpy(expected + 1, args, sizeof(args));
    TEST_ASSERT_EQUAL_size_t(sizeof(expected), sithVec_ev.size);
    TEST_ASSERT_EQUAL_MEMORY(expected, sithVec_ev.p, sizeof(expected));
}

static void sithAIMoveFix_AssertAngles(float yaw)
{
    const rdVector3* aAngles = SITHAIMOVEVEC_USER->aAngles;
    for ( size_t i = 0; i < 32; ++i )
    {
        TEST_ASSERT_EQUAL_FLOAT(0.0f, aAngles[i].pitch);
        TEST_ASSERT_EQUAL_FLOAT(0.0f, aAngles[i].roll);
        // This is the geometric angle in degrees. The deterministic trig stubs
        // evaluate sin/asin separately, so allow 0.001 degree roundoff.
        TEST_ASSERT_FLOAT_WITHIN(0.001f, i ? 0.0f : yaw, aAngles[i].yaw);
    }
}

TEST(sithAIMoveFix, MineCarFreeActorTurnsBothWaysAndCompletes)
{
    for ( int side = -1; side <= 1; side += 2 )
    {
        sithAIMoveFix_Prepare(SITHPLAYERMOVE_WALKING, (float)side * 4.0f);
        SithAIControlBlock* pLocal = &sithVec_arena.aiBlocks[0];
        SithThing* pOwner = pLocal->pOwner;
        rdMatrix34 beforeOrient = pOwner->orient;

        TEST_ASSERT_EQUAL_INT(SITHAIMOVE_STATE_NONE, sithAIMove_AIGetMoveState(pLocal));
        sithAIMove_UpdateMineCar(pLocal, 0.25f);
        TEST_ASSERT_EQUAL_INT(side < 0 ? SITHPLAYERMOVE_UNKNOWN_105 : SITHPLAYERMOVE_UNKNOWN_106, pOwner->moveStatus);
        TEST_ASSERT_EQUAL_UINT32(SITHAI_MODE_TURNING | SITHAI_MODE_SEARCHING, pLocal->mode);
        sithAIMoveFix_AssertAngles((float)-side * 22.5f);
        sithAIMoveFix_AssertMeshCall(pOwner);
        TEST_ASSERT_EQUAL_MEMORY(&beforeOrient, &pOwner->orient, sizeof(beforeOrient));

        // A subsequent update can reach the remaining turn. The free actor
        // becomes still and only TURNING is cleared.
        sithVec_ev.size = 0;
        sithAIMove_UpdateMineCar(pLocal, 1.0f);
        TEST_ASSERT_EQUAL_INT(SITHPLAYERMOVE_STILL, pOwner->moveStatus);
        TEST_ASSERT_EQUAL_UINT32(SITHAI_MODE_SEARCHING, pLocal->mode);
        sithAIMoveFix_AssertAngles((float)-side * 90.0f);
        sithAIMoveFix_AssertMeshCall(pOwner);
        TEST_ASSERT_EQUAL_MEMORY(&beforeOrient, &pOwner->orient, sizeof(beforeOrient));
    }
}

TEST(sithAIMoveFix, MineCarPreservesBusyActorStatus)
{
    static const SithThingMoveStatus aStatus[] = { SITHPLAYERMOVE_ROLLING_LEFT, SITHPLAYERMOVE_MOUNTING_WALL };
    static const SithAIMoveState aState[] = { SITHAIMOVE_STATE_ANIMATED, SITHAIMOVE_STATE_STATIONARY };
    for ( size_t i = 0; i < SITHVEC_ARRAYLEN(aStatus); ++i )
    {
        for ( int side = -1; side <= 1; side += 2 )
        {
            sithAIMoveFix_Prepare(aStatus[i], (float)side * 4.0f);
            SithAIControlBlock* pLocal = &sithVec_arena.aiBlocks[0];
            TEST_ASSERT_EQUAL_INT(aState[i], sithAIMove_AIGetMoveState(pLocal));
            sithAIMove_UpdateMineCar(pLocal, 0.25f);
            TEST_ASSERT_EQUAL_INT(aStatus[i], pLocal->pOwner->moveStatus);
            TEST_ASSERT_EQUAL_UINT32(SITHAI_MODE_TURNING | SITHAI_MODE_SEARCHING, pLocal->mode);
            sithAIMoveFix_AssertAngles((float)-side * 22.5f);
            sithAIMoveFix_AssertMeshCall(pLocal->pOwner);

            sithVec_ev.size = 0;
            sithAIMove_UpdateMineCar(pLocal, 1.0f);
            TEST_ASSERT_EQUAL_INT(aStatus[i], pLocal->pOwner->moveStatus);
            TEST_ASSERT_EQUAL_UINT32(SITHAI_MODE_SEARCHING, pLocal->mode);
            sithAIMoveFix_AssertAngles((float)-side * 90.0f);
            sithAIMoveFix_AssertMeshCall(pLocal->pOwner);
        }
    }
}

TEST_GROUP_RUNNER(sithAIMoveFix)
{
    RUN_TEST_CASE(sithAIMoveFix, MineCarFreeActorTurnsBothWaysAndCompletes);
    RUN_TEST_CASE(sithAIMoveFix, MineCarPreservesBusyActorStatus);
}
#endif

void SithVecTest_RunAllTests(void)
{
    RUN_TEST_GROUP(sithAIMove);
#ifdef J3D_QOL_IMPROVEMENTS
    RUN_TEST_GROUP(sithAIMoveFix);
#endif
}

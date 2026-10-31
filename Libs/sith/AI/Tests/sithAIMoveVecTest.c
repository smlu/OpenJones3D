#include <unity_fixture.h>

#include "sithAIMoveVec.h"

#include "sithVecTest.h"

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
    // The QOL fix of sithAIMove_UpdateMineCar queries the move state of the actor (one more AI call and a defined result);
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

void SithVecTest_RunAllTests(void)
{
    RUN_TEST_GROUP(sithAIMove);
}

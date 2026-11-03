#include <unity_fixture.h>

#include "sithAIInstinctVec.h"

#include "sithVecTest.h"

// Original-engine test vectors for sithAIInstinct_InitInstincts and the instinct registry functions of sithAI.c it uses
// (sithAI_RegisterInstinct, sithAI_FindInstinct). The fixture in tv/sithAIInstinct holds, for every case of
// sithAIVec_aInstinctCases, the result of the retail v1.0 functions (the registry entries and count after every call,
// return values and lookup results). The tests rebuild the same registry, run the reconstructed functions and
// compare. See tv/sithAIInstinct/README.md.

static SithVecTestFixture sithAIInstinctVecTest_fixture;
static char sithAIInstinctVecTest_aFailText[8192];

TEST_GROUP(sithAIInstinct);

TEST_SETUP(sithAIInstinct)
{
    sithVecTest_AssertArenaPlacement();
    sith_g_pHS = sithVec_InitHostServices();
    sithVecTest_LoadFixture(&sithAIInstinctVecTest_fixture, SITHVEC_TV_DIR, sithAIVec_instinctSuite.pName);
}

TEST_TEAR_DOWN(sithAIInstinct)
{
    sithVecTest_FreeFixture(&sithAIInstinctVecTest_fixture);
    sith_g_pHS = NULL;
}

// Every case of the table is in the fixture and the other way around
TEST(sithAIInstinct, TestFixtureCoversAllCases)
{
    sithVecTest_AssertFixtureCoversSuite(&sithAIInstinctVecTest_fixture, &sithAIVec_instinctSuite);
}

// Registration into empty, partly used, exactly filled, full and overfull registries, repeated initialization, and
// lookups of every registered name afterwards
TEST(sithAIInstinct, TestOriginalInitInstincts)
{
    unsigned int numFailed = sithVecTest_RunCases(&sithAIVec_instinctSuite, &sithAIInstinctVecTest_fixture, "InitInstincts_",
        sithAIInstinctVecTest_aFailText, sizeof(sithAIInstinctVecTest_aFailText));
    TEST_ASSERT_EQUAL_UINT_MESSAGE(0, numFailed, sithAIInstinctVecTest_aFailText);
}

// Direct registrations: all name lengths, mode masks, a NULL function, the last free entry and a full registry
TEST(sithAIInstinct, TestOriginalRegisterInstinct)
{
    unsigned int numFailed = sithVecTest_RunCases(&sithAIVec_instinctSuite, &sithAIInstinctVecTest_fixture, "RegisterInstinct_",
        sithAIInstinctVecTest_aFailText, sizeof(sithAIInstinctVecTest_aFailText));
    TEST_ASSERT_EQUAL_UINT_MESSAGE(0, numFailed, sithAIInstinctVecTest_aFailText);
}

// Lookups in an empty registry and in one with only earlier registrations
TEST(sithAIInstinct, TestOriginalFindInstinct)
{
    unsigned int numFailed = sithVecTest_RunCases(&sithAIVec_instinctSuite, &sithAIInstinctVecTest_fixture, "FindInstinct_",
        sithAIInstinctVecTest_aFailText, sizeof(sithAIInstinctVecTest_aFailText));
    TEST_ASSERT_EQUAL_UINT_MESSAGE(0, numFailed, sithAIInstinctVecTest_aFailText);
}

TEST_GROUP_RUNNER(sithAIInstinct)
{
    RUN_TEST_CASE(sithAIInstinct, TestFixtureCoversAllCases);
    RUN_TEST_CASE(sithAIInstinct, TestOriginalInitInstincts);
    RUN_TEST_CASE(sithAIInstinct, TestOriginalRegisterInstinct);
    RUN_TEST_CASE(sithAIInstinct, TestOriginalFindInstinct);
}

void SithVecTest_RunAllTests(void)
{
    RUN_TEST_GROUP(sithAIInstinct);
}

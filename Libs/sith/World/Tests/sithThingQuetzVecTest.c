#include <unity_fixture.h>

#include "sithThingVecQuetz.h"

#include "sithVecTest.h"

// Original-engine test vectors for sithThing_CreateQuetzUserBlock and sithThing_UpdateQuetzUserBlock. The fixture in
// tv/sithThingQuetz holds, for every case of sithThingVec_aQuetzCases, the result of the retail v1.0 functions (return
// value, the engine calls they made and the bytes they changed in the synthetic state, frame by frame). The tests
// rebuild the same state, run the reconstructed functions and compare. See tv/sithThingQuetz/README.md.

SITHVEC_REDIRECT_DEFINE(float, sithTime_g_fps);

static SithVecTestFixture sithThingQuetzVecTest_fixture;
static char sithThingQuetzVecTest_aFailText[8192];

TEST_GROUP(sithThingQuetz);

TEST_SETUP(sithThingQuetz)
{
    sithVecTest_AssertArenaPlacement();
    sith_g_pHS = sithVec_InitHostServices();
    sithVecTest_LoadFixture(&sithThingQuetzVecTest_fixture, SITHVEC_TV_DIR, sithThingVec_quetzSuite.pName);
}

TEST_TEAR_DOWN(sithThingQuetz)
{
    sithVecTest_FreeFixture(&sithThingQuetzVecTest_fixture);
    sith_g_pHS = NULL;
}

// Every case of the table is in the fixture and the other way around
TEST(sithThingQuetz, TestFixtureCoversAllCases)
{
    sithVecTest_AssertFixtureCoversSuite(&sithThingQuetzVecTest_fixture, &sithThingVec_quetzSuite);
}

// Segment counts 0..34, other attachments, an existing block, a failed allocation, pitched and vertical heads
TEST(sithThingQuetz, TestOriginalCreateQuetzUserBlock)
{
    unsigned int numFailed = sithVecTest_RunCases(&sithThingVec_quetzSuite, &sithThingQuetzVecTest_fixture, "Create_",
        sithThingQuetzVecTest_aFailText, sizeof(sithThingQuetzVecTest_aFailText));
    TEST_ASSERT_EQUAL_UINT_MESSAGE(0, numFailed, sithThingQuetzVecTest_aFailText);
}

// Frame scripts: straight, gentle, sharp and alternating turns, resets, sharp turning carried over, long and negative
// steps, velocity directions, sector changes and blocked moves, attachment changes, standing still, pitched heads,
// failed allocation, existing blocks and frame rates
TEST(sithThingQuetz, TestOriginalUpdateQuetzUserBlock)
{
    unsigned int numFailed = sithVecTest_RunCases(&sithThingVec_quetzSuite, &sithThingQuetzVecTest_fixture, "Update_",
        sithThingQuetzVecTest_aFailText, sizeof(sithThingQuetzVecTest_aFailText));
    TEST_ASSERT_EQUAL_UINT_MESSAGE(0, numFailed, sithThingQuetzVecTest_aFailText);
}

TEST_GROUP_RUNNER(sithThingQuetz)
{
    RUN_TEST_CASE(sithThingQuetz, TestFixtureCoversAllCases);
    RUN_TEST_CASE(sithThingQuetz, TestOriginalCreateQuetzUserBlock);
    RUN_TEST_CASE(sithThingQuetz, TestOriginalUpdateQuetzUserBlock);
}

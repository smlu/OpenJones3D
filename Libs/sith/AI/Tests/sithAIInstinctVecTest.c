#include <unity_fixture.h>

#include "sithAIInstinctVec.h"

#include "sithVecTest.h"

#include <sith/Gameplay/sithTime.h>

SITHVEC_REDIRECT_DEFINE(uint32_t, sithTime_g_msecGameTime);
// The frame time of a 30 FPS frame: the original vectors were captured without one, and sithTime_CalcDeadline puts
// deadlines on 30 FPS frames only for shorter frames, so the deadlines stay the original now + interval
SITHVEC_REDIRECT_DEFINE(uint32_t, sithTime_g_frameTime) = 1000u / 30u;

void J3DAPI sithAI_InstinctUpdate(SithAIControlBlock* pLocal); // sithAI.c, no header declaration

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

static unsigned sithAIInstinctVecTest_scanCalls;
static unsigned sithAIInstinctVecTest_aScanOrder[4];
static int sithAIInstinctVecTest_bScanHandled;

static int J3DAPI sithAIInstinctVecTest_ScanInstinct(SithAIControlBlock* pLocal, SithAIInstinct* pInstinct, SithAIInstinctState* pState, SithAIEventType event, void* pObject)
{
    J3D_UNUSED(pState);
    J3D_UNUSED(event);
    J3D_UNUSED(pObject);
    unsigned index = (unsigned)(pInstinct - pLocal->pClass->aInstincts);
    if ( sithAIInstinctVecTest_scanCalls < SITHVEC_ARRAYLEN(sithAIInstinctVecTest_aScanOrder) )
    {
        sithAIInstinctVecTest_aScanOrder[sithAIInstinctVecTest_scanCalls] = index;
    }
    ++sithAIInstinctVecTest_scanCalls;
    if ( index == 1 )
    {
        // Enabling an earlier instinct makes scan restart observable without
        // changing the actor's mode or invoking unrelated mode-change events.
        pLocal->aInstinctStates[0].flags = 0;
        return sithAIInstinctVecTest_bScanHandled;
    }
    return 0;
}

TEST(sithAIInstinct, HandledInstinctRestartsScan)
{
    for ( int handled = 0; handled <= 1; ++handled )
    {
        SithAIControlBlock local = { 0 };
        SithAIClass aiClass = { 0 };
        local.pClass = &aiClass;
        local.numInstincts = 3;
        local.mode = SITHAI_MODE_SEARCHING;
        for ( size_t i = 0; i < local.numInstincts; ++i )
        {
            aiClass.aInstincts[i].pfInstinct = sithAIInstinctVecTest_ScanInstinct;
            aiClass.aInstincts[i].updateModes = SITHAI_MODE_SEARCHING;
        }
        local.aInstinctStates[0].flags = SITHAI_INSTINCT_DISABLED;
        sithTime_g_msecGameTime = 100000;
        sithAIInstinctVecTest_scanCalls = 0;
        memset(sithAIInstinctVecTest_aScanOrder, 0xFF, sizeof(sithAIInstinctVecTest_aScanOrder));
        sithAIInstinctVecTest_bScanHandled = handled;

        sithAI_InstinctUpdate(&local);
        static const unsigned aRestarted[] = { 1, 0, 2 };
        static const unsigned aContinued[] = { 1, 2 };
        TEST_ASSERT_EQUAL_UINT(handled ? 3 : 2, sithAIInstinctVecTest_scanCalls);
        TEST_ASSERT_EQUAL_MEMORY(handled ? aRestarted : aContinued, sithAIInstinctVecTest_aScanOrder, (handled ? 3 : 2) * sizeof(unsigned));
        TEST_ASSERT_EQUAL_UINT32(101000, local.msecNextUpdate);
        TEST_ASSERT_EQUAL_UINT32(handled ? 101000 : 0, local.aInstinctStates[0].msecNextUpdate);
        TEST_ASSERT_EQUAL_UINT32(SITHAI_MODE_SEARCHING, local.mode);
    }
}

TEST_GROUP_RUNNER(sithAIInstinct)
{
    RUN_TEST_CASE(sithAIInstinct, TestFixtureCoversAllCases);
    RUN_TEST_CASE(sithAIInstinct, TestOriginalInitInstincts);
    RUN_TEST_CASE(sithAIInstinct, TestOriginalRegisterInstinct);
    RUN_TEST_CASE(sithAIInstinct, TestOriginalFindInstinct);
    RUN_TEST_CASE(sithAIInstinct, HandledInstinctRestartsScan);
}

#include <unity_fixture.h>

#include "sithAIUtilVec.h"

#include "sithVecTest.h"

// Original-engine test vectors for the sithAIUtil functions: waypoint tables, sight and distance, firing, path checks and
// point creation, and the things-in-view search. The fixture in tv/sithAIUtil holds, for every case of
// sithAIUtilVec_aCases, the result of the retail v1.0 functions (return value, outputs, the file statics of sithAIUtil.c
// and the AI control blocks after every call, and the engine calls made through the seams). The tests rebuild the same
// state, run the reconstructed functions and compare. See tv/sithAIUtil/README.md.

// Engine globals of the original executable used by the tested functions (see sithAIUtilVecAddrs.h)
SITHVEC_REDIRECT_DEFINE(size_t, sithMain_g_frameNumber);
SITHVEC_REDIRECT_DEFINE(size_t, sithMain_g_curRenderTick);
SITHVEC_REDIRECT_DEFINE(SithMode, sithMain_g_sith_mode);
SITHVEC_REDIRECT_DEFINE(uint32_t, sithTime_g_msecGameTime);
SITHVEC_REDIRECT_DEFINE(float, sithTime_g_secGameTime);
SITHVEC_REDIRECT_DEFINE(float, sithTime_g_frameTimeFlex);
SITHVEC_REDIRECT_DEFINE(SithThing*, sithPlayer_g_pLocalPlayerThing);
SITHVEC_REDIRECT_DEFINE(SithAIControlBlock, sithAI_g_aControlBlocks[256]);
SITHVEC_REDIRECT_DEFINE(int, sithAI_g_lastUsedAIIndex);
SITHVEC_REDIRECT_DEFINE(int, sithAIUtil_g_bRenderAIWpnts);

static SithVecTestFixture sithAIUtilVecTest_fixture;
static char sithAIUtilVecTest_aFailText[32768];

TEST_GROUP(sithAIUtil);

TEST_SETUP(sithAIUtil)
{
    sithVecTest_AssertArenaPlacement();
    sith_g_pHS = sithVec_InitHostServices();
    sithVecTest_LoadFixture(&sithAIUtilVecTest_fixture, SITHVEC_TV_DIR, sithAIUtilVec_suite.pName);
}

TEST_TEAR_DOWN(sithAIUtil)
{
    sithVecTest_FreeFixture(&sithAIUtilVecTest_fixture);
    sith_g_pHS = NULL;
}

// Every case of the table is in the fixture and the other way around
TEST(sithAIUtil, TestFixtureCoversAllCases)
{
    sithVecTest_AssertFixtureCoversSuite(&sithAIUtilVecTest_fixture, &sithAIUtilVec_suite);
}


TEST(sithAIUtil, TestOriginalSetWeaponFireFlags)
{
    unsigned int numFailed = sithVecTest_RunCases(&sithAIUtilVec_suite, &sithAIUtilVecTest_fixture, "SetWeaponFireFlags_",
        sithAIUtilVecTest_aFailText, sizeof(sithAIUtilVecTest_aFailText));
    TEST_ASSERT_EQUAL_UINT_MESSAGE(0, numFailed, sithAIUtilVecTest_aFailText);
}


TEST(sithAIUtil, TestOriginalCanAttackTarget)
{
    unsigned int numFailed = sithVecTest_RunCases(&sithAIUtilVec_suite, &sithAIUtilVecTest_fixture, "CanAttackTarget_",
        sithAIUtilVecTest_aFailText, sizeof(sithAIUtilVecTest_aFailText));
    TEST_ASSERT_EQUAL_UINT_MESSAGE(0, numFailed, sithAIUtilVecTest_aFailText);
}


TEST(sithAIUtil, TestOriginalIsThingMoving)
{
    unsigned int numFailed = sithVecTest_RunCases(&sithAIUtilVec_suite, &sithAIUtilVecTest_fixture, "IsThingMoving_",
        sithAIUtilVecTest_aFailText, sizeof(sithAIUtilVecTest_aFailText));
    TEST_ASSERT_EQUAL_UINT_MESSAGE(0, numFailed, sithAIUtilVecTest_aFailText);
}


TEST(sithAIUtil, TestOriginalAIPauseMove)
{
    unsigned int numFailed = sithVecTest_RunCases(&sithAIUtilVec_suite, &sithAIUtilVecTest_fixture, "AIPauseMove_",
        sithAIUtilVecTest_aFailText, sizeof(sithAIUtilVecTest_aFailText));
    TEST_ASSERT_EQUAL_UINT_MESSAGE(0, numFailed, sithAIUtilVecTest_aFailText);
}


TEST(sithAIUtil, TestOriginalAISetMode)
{
    unsigned int numFailed = sithVecTest_RunCases(&sithAIUtilVec_suite, &sithAIUtilVecTest_fixture, "AISetMode_",
        sithAIUtilVecTest_aFailText, sizeof(sithAIUtilVecTest_aFailText));
    TEST_ASSERT_EQUAL_UINT_MESSAGE(0, numFailed, sithAIUtilVecTest_aFailText);
}


TEST(sithAIUtil, TestOriginalAIClearMode)
{
    unsigned int numFailed = sithVecTest_RunCases(&sithAIUtilVec_suite, &sithAIUtilVecTest_fixture, "AIClearMode_",
        sithAIUtilVecTest_aFailText, sizeof(sithAIUtilVecTest_aFailText));
    TEST_ASSERT_EQUAL_UINT_MESSAGE(0, numFailed, sithAIUtilVecTest_aFailText);
}


TEST(sithAIUtil, TestOriginalAIGetMovePos)
{
    unsigned int numFailed = sithVecTest_RunCases(&sithAIUtilVec_suite, &sithAIUtilVecTest_fixture, "AIGetMovePos_",
        sithAIUtilVecTest_aFailText, sizeof(sithAIUtilVecTest_aFailText));
    TEST_ASSERT_EQUAL_UINT_MESSAGE(0, numFailed, sithAIUtilVecTest_aFailText);
}


TEST(sithAIUtil, TestOriginalGetXYHeadingVector)
{
    unsigned int numFailed = sithVecTest_RunCases(&sithAIUtilVec_suite, &sithAIUtilVecTest_fixture, "GetXYHeadingVector_",
        sithAIUtilVecTest_aFailText, sizeof(sithAIUtilVecTest_aFailText));
    TEST_ASSERT_EQUAL_UINT_MESSAGE(0, numFailed, sithAIUtilVecTest_aFailText);
}


TEST(sithAIUtil, TestOriginalGetXYZHeadingVector)
{
    unsigned int numFailed = sithVecTest_RunCases(&sithAIUtilVec_suite, &sithAIUtilVecTest_fixture, "GetXYZHeadingVector_",
        sithAIUtilVecTest_aFailText, sizeof(sithAIUtilVecTest_aFailText));
    TEST_ASSERT_EQUAL_UINT_MESSAGE(0, numFailed, sithAIUtilVecTest_aFailText);
}


TEST(sithAIUtil, TestOriginalApplyForce)
{
    unsigned int numFailed = sithVecTest_RunCases(&sithAIUtilVec_suite, &sithAIUtilVecTest_fixture, "ApplyForce_",
        sithAIUtilVecTest_aFailText, sizeof(sithAIUtilVecTest_aFailText));
    TEST_ASSERT_EQUAL_UINT_MESSAGE(0, numFailed, sithAIUtilVecTest_aFailText);
}


TEST(sithAIUtil, TestOriginalRandomRotate)
{
    unsigned int numFailed = sithVecTest_RunCases(&sithAIUtilVec_suite, &sithAIUtilVecTest_fixture, "RandomRotate_",
        sithAIUtilVecTest_aFailText, sizeof(sithAIUtilVecTest_aFailText));
    TEST_ASSERT_EQUAL_UINT_MESSAGE(0, numFailed, sithAIUtilVecTest_aFailText);
}


TEST(sithAIUtil, TestOriginalAIPlaySoundMode)
{
    unsigned int numFailed = sithVecTest_RunCases(&sithAIUtilVec_suite, &sithAIUtilVecTest_fixture, "AIPlaySoundMode_",
        sithAIUtilVecTest_aFailText, sizeof(sithAIUtilVecTest_aFailText));
    TEST_ASSERT_EQUAL_UINT_MESSAGE(0, numFailed, sithAIUtilVecTest_aFailText);
}


TEST(sithAIUtil, TestOriginalAISetWpnt)
{
    unsigned int numFailed = sithVecTest_RunCases(&sithAIUtilVec_suite, &sithAIUtilVecTest_fixture, "AISetWpnt_",
        sithAIUtilVecTest_aFailText, sizeof(sithAIUtilVecTest_aFailText));
    TEST_ASSERT_EQUAL_UINT_MESSAGE(0, numFailed, sithAIUtilVecTest_aFailText);
}


TEST(sithAIUtil, TestOriginalAISetWpntRank)
{
    unsigned int numFailed = sithVecTest_RunCases(&sithAIUtilVec_suite, &sithAIUtilVecTest_fixture, "AISetWpntRank_",
        sithAIUtilVecTest_aFailText, sizeof(sithAIUtilVecTest_aFailText));
    TEST_ASSERT_EQUAL_UINT_MESSAGE(0, numFailed, sithAIUtilVecTest_aFailText);
}


TEST(sithAIUtil, TestOriginalAISetWpntFlags)
{
    unsigned int numFailed = sithVecTest_RunCases(&sithAIUtilVec_suite, &sithAIUtilVecTest_fixture, "AISetWpntFlags_",
        sithAIUtilVecTest_aFailText, sizeof(sithAIUtilVecTest_aFailText));
    TEST_ASSERT_EQUAL_UINT_MESSAGE(0, numFailed, sithAIUtilVecTest_aFailText);
}


TEST(sithAIUtil, TestOriginalAIClearWpntFlags)
{
    unsigned int numFailed = sithVecTest_RunCases(&sithAIUtilVec_suite, &sithAIUtilVecTest_fixture, "AIClearWpntFlags_",
        sithAIUtilVecTest_aFailText, sizeof(sithAIUtilVecTest_aFailText));
    TEST_ASSERT_EQUAL_UINT_MESSAGE(0, numFailed, sithAIUtilVecTest_aFailText);
}


TEST(sithAIUtil, TestOriginalAISetActiveWpntLayer)
{
    unsigned int numFailed = sithVecTest_RunCases(&sithAIUtilVec_suite, &sithAIUtilVecTest_fixture, "AISetActiveWpntLayer_",
        sithAIUtilVecTest_aFailText, sizeof(sithAIUtilVecTest_aFailText));
    TEST_ASSERT_EQUAL_UINT_MESSAGE(0, numFailed, sithAIUtilVecTest_aFailText);
}


TEST(sithAIUtil, TestOriginalAIConnectWpnts)
{
    unsigned int numFailed = sithVecTest_RunCases(&sithAIUtilVec_suite, &sithAIUtilVecTest_fixture, "AIConnectWpnts_",
        sithAIUtilVecTest_aFailText, sizeof(sithAIUtilVecTest_aFailText));
    TEST_ASSERT_EQUAL_UINT_MESSAGE(0, numFailed, sithAIUtilVecTest_aFailText);
}


TEST(sithAIUtil, TestOriginalCreateAIWaypoint)
{
    unsigned int numFailed = sithVecTest_RunCases(&sithAIUtilVec_suite, &sithAIUtilVecTest_fixture, "CreateAIWaypoint_",
        sithAIUtilVecTest_aFailText, sizeof(sithAIUtilVecTest_aFailText));
    TEST_ASSERT_EQUAL_UINT_MESSAGE(0, numFailed, sithAIUtilVecTest_aFailText);
}


TEST(sithAIUtil, TestOriginalClearAIWaypoint)
{
    unsigned int numFailed = sithVecTest_RunCases(&sithAIUtilVec_suite, &sithAIUtilVecTest_fixture, "ClearAIWaypoint_",
        sithAIUtilVecTest_aFailText, sizeof(sithAIUtilVecTest_aFailText));
    TEST_ASSERT_EQUAL_UINT_MESSAGE(0, numFailed, sithAIUtilVecTest_aFailText);
}


TEST(sithAIUtil, TestOriginalGetAIWaypointNum)
{
    unsigned int numFailed = sithVecTest_RunCases(&sithAIUtilVec_suite, &sithAIUtilVecTest_fixture, "GetAIWaypointNum_",
        sithAIUtilVecTest_aFailText, sizeof(sithAIUtilVecTest_aFailText));
    TEST_ASSERT_EQUAL_UINT_MESSAGE(0, numFailed, sithAIUtilVecTest_aFailText);
}


TEST(sithAIUtil, TestOriginalCompareAIWpntDistances)
{
    unsigned int numFailed = sithVecTest_RunCases(&sithAIUtilVec_suite, &sithAIUtilVecTest_fixture, "CompareAIWpntDistances_",
        sithAIUtilVecTest_aFailText, sizeof(sithAIUtilVecTest_aFailText));
    TEST_ASSERT_EQUAL_UINT_MESSAGE(0, numFailed, sithAIUtilVecTest_aFailText);
}


TEST(sithAIUtil, TestOriginalAIGetWpntThing)
{
    unsigned int numFailed = sithVecTest_RunCases(&sithAIUtilVec_suite, &sithAIUtilVecTest_fixture, "AIGetWpntThing_",
        sithAIUtilVecTest_aFailText, sizeof(sithAIUtilVecTest_aFailText));
    TEST_ASSERT_EQUAL_UINT_MESSAGE(0, numFailed, sithAIUtilVecTest_aFailText);
}


TEST(sithAIUtil, TestOriginalRankWpntsFromThing)
{
    unsigned int numFailed = sithVecTest_RunCases(&sithAIUtilVec_suite, &sithAIUtilVecTest_fixture, "RankWpntsFromThing_",
        sithAIUtilVecTest_aFailText, sizeof(sithAIUtilVecTest_aFailText));
    TEST_ASSERT_EQUAL_UINT_MESSAGE(0, numFailed, sithAIUtilVecTest_aFailText);
}


TEST(sithAIUtil, TestOriginalRankWpntsFromPlayer)
{
    unsigned int numFailed = sithVecTest_RunCases(&sithAIUtilVec_suite, &sithAIUtilVecTest_fixture, "RankWpntsFromPlayer_",
        sithAIUtilVecTest_aFailText, sizeof(sithAIUtilVecTest_aFailText));
    TEST_ASSERT_EQUAL_UINT_MESSAGE(0, numFailed, sithAIUtilVecTest_aFailText);
}


TEST(sithAIUtil, TestOriginalSelectWpntLink)
{
    unsigned int numFailed = sithVecTest_RunCases(&sithAIUtilVec_suite, &sithAIUtilVecTest_fixture, "SelectWpntLink_",
        sithAIUtilVecTest_aFailText, sizeof(sithAIUtilVecTest_aFailText));
    TEST_ASSERT_EQUAL_UINT_MESSAGE(0, numFailed, sithAIUtilVecTest_aFailText);
}


TEST(sithAIUtil, TestOriginalCheckPathToPoint)
{
    unsigned int numFailed = sithVecTest_RunCases(&sithAIUtilVec_suite, &sithAIUtilVecTest_fixture, "CheckPathToPoint_",
        sithAIUtilVecTest_aFailText, sizeof(sithAIUtilVecTest_aFailText));
    TEST_ASSERT_EQUAL_UINT_MESSAGE(0, numFailed, sithAIUtilVecTest_aFailText);
}


TEST(sithAIUtil, TestOriginalCheckFloorAtPos)
{
    unsigned int numFailed = sithVecTest_RunCases(&sithAIUtilVec_suite, &sithAIUtilVecTest_fixture, "CheckFloorAtPos_",
        sithAIUtilVecTest_aFailText, sizeof(sithAIUtilVecTest_aFailText));
    TEST_ASSERT_EQUAL_UINT_MESSAGE(0, numFailed, sithAIUtilVecTest_aFailText);
}


TEST(sithAIUtil, TestOriginalCheckPosition)
{
    unsigned int numFailed = sithVecTest_RunCases(&sithAIUtilVec_suite, &sithAIUtilVecTest_fixture, "CheckPosition_",
        sithAIUtilVecTest_aFailText, sizeof(sithAIUtilVecTest_aFailText));
    TEST_ASSERT_EQUAL_UINT_MESSAGE(0, numFailed, sithAIUtilVecTest_aFailText);
}


TEST(sithAIUtil, TestOriginalAIFindNearestWpnt)
{
    unsigned int numFailed = sithVecTest_RunCases(&sithAIUtilVec_suite, &sithAIUtilVecTest_fixture, "AIFindNearestWpnt_",
        sithAIUtilVecTest_aFailText, sizeof(sithAIUtilVecTest_aFailText));
    TEST_ASSERT_EQUAL_UINT_MESSAGE(0, numFailed, sithAIUtilVecTest_aFailText);
}


TEST(sithAIUtil, TestOriginalAIMoveToNextWpnt)
{
    unsigned int numFailed = sithVecTest_RunCases(&sithAIUtilVec_suite, &sithAIUtilVecTest_fixture, "AIMoveToNextWpnt_",
        sithAIUtilVecTest_aFailText, sizeof(sithAIUtilVecTest_aFailText));
    TEST_ASSERT_EQUAL_UINT_MESSAGE(0, numFailed, sithAIUtilVecTest_aFailText);
}


TEST(sithAIUtil, TestOriginalAITraverseWpnts)
{
    unsigned int numFailed = sithVecTest_RunCases(&sithAIUtilVec_suite, &sithAIUtilVecTest_fixture, "AITraverseWpnts_",
        sithAIUtilVecTest_aFailText, sizeof(sithAIUtilVecTest_aFailText));
    TEST_ASSERT_EQUAL_UINT_MESSAGE(0, numFailed, sithAIUtilVecTest_aFailText);
}


TEST(sithAIUtil, TestOriginalAIAdvanceToNextWpnt)
{
    unsigned int numFailed = sithVecTest_RunCases(&sithAIUtilVec_suite, &sithAIUtilVecTest_fixture, "AIAdvanceToNextWpnt_",
        sithAIUtilVecTest_aFailText, sizeof(sithAIUtilVecTest_aFailText));
    TEST_ASSERT_EQUAL_UINT_MESSAGE(0, numFailed, sithAIUtilVecTest_aFailText);
}


TEST(sithAIUtil, TestOriginalCheckWpntMoveAlign)
{
    unsigned int numFailed = sithVecTest_RunCases(&sithAIUtilVec_suite, &sithAIUtilVecTest_fixture, "CheckWpntMoveAlign_",
        sithAIUtilVecTest_aFailText, sizeof(sithAIUtilVecTest_aFailText));
    TEST_ASSERT_EQUAL_UINT_MESSAGE(0, numFailed, sithAIUtilVecTest_aFailText);
}


TEST(sithAIUtil, TestOriginalRenderAIWaypoints)
{
    unsigned int numFailed = sithVecTest_RunCases(&sithAIUtilVec_suite, &sithAIUtilVecTest_fixture, "RenderAIWaypoints_",
        sithAIUtilVecTest_aFailText, sizeof(sithAIUtilVecTest_aFailText));
    TEST_ASSERT_EQUAL_UINT_MESSAGE(0, numFailed, sithAIUtilVecTest_aFailText);
}


TEST(sithAIUtil, TestOriginalGetDistanceToTarget)
{
    unsigned int numFailed = sithVecTest_RunCases(&sithAIUtilVec_suite, &sithAIUtilVecTest_fixture, "GetDistanceToTarget_",
        sithAIUtilVecTest_aFailText, sizeof(sithAIUtilVecTest_aFailText));
    TEST_ASSERT_EQUAL_UINT_MESSAGE(0, numFailed, sithAIUtilVecTest_aFailText);
}


TEST(sithAIUtil, TestOriginalGetDistanceToTargetPos)
{
    unsigned int numFailed = sithVecTest_RunCases(&sithAIUtilVec_suite, &sithAIUtilVecTest_fixture, "GetDistanceToTargetPos_",
        sithAIUtilVecTest_aFailText, sizeof(sithAIUtilVecTest_aFailText));
    TEST_ASSERT_EQUAL_UINT_MESSAGE(0, numFailed, sithAIUtilVecTest_aFailText);
}


// Known difference: with a NaN distance the original treats the distance as below 2 (the compare of the x87 code is
// true for unordered operands), the reconstruction does not; no caller can produce a NaN distance
TEST(sithAIUtil, TestOriginalCanSeeTarget)
{
    static const char* const aKnown[] = { "CanSeeTarget_edge_nan_distance", "CanSeeTarget_sweeprand_edge_nan_distance" };
    unsigned int numFailed = sithVecTest_RunCases(&sithAIUtilVec_suite, &sithAIUtilVecTest_fixture, "CanSeeTarget_",
        sithAIUtilVecTest_aFailText, sizeof(sithAIUtilVecTest_aFailText));
    sithVecTest_ExpectKnownDifferences(numFailed, sithAIUtilVecTest_aFailText, aKnown, (unsigned int)SITHVEC_ARRAYLEN(aKnown),
        "A NaN distance: the original skips the distance dependent part (x87 compare of a NaN counts as less), the reconstruction enters it; see tv/sithAIUtil/README.md.");
}


TEST(sithAIUtil, TestOriginalUpdateTargetSight)
{
    unsigned int numFailed = sithVecTest_RunCases(&sithAIUtilVec_suite, &sithAIUtilVecTest_fixture, "UpdateTargetSight_",
        sithAIUtilVecTest_aFailText, sizeof(sithAIUtilVecTest_aFailText));
    TEST_ASSERT_EQUAL_UINT_MESSAGE(0, numFailed, sithAIUtilVecTest_aFailText);
}


TEST(sithAIUtil, TestOriginalUpdateGoalSight)
{
    unsigned int numFailed = sithVecTest_RunCases(&sithAIUtilVec_suite, &sithAIUtilVecTest_fixture, "UpdateGoalSight_",
        sithAIUtilVecTest_aFailText, sizeof(sithAIUtilVecTest_aFailText));
    TEST_ASSERT_EQUAL_UINT_MESSAGE(0, numFailed, sithAIUtilVecTest_aFailText);
}


TEST(sithAIUtil, TestOriginalIsLineOfFireClear)
{
    unsigned int numFailed = sithVecTest_RunCases(&sithAIUtilVec_suite, &sithAIUtilVecTest_fixture, "IsLineOfFireClear_",
        sithAIUtilVecTest_aFailText, sizeof(sithAIUtilVecTest_aFailText));
    TEST_ASSERT_EQUAL_UINT_MESSAGE(0, numFailed, sithAIUtilVecTest_aFailText);
}


TEST(sithAIUtil, TestOriginalAIFire)
{
    unsigned int numFailed = sithVecTest_RunCases(&sithAIUtilVec_suite, &sithAIUtilVecTest_fixture, "AIFire_",
        sithAIUtilVecTest_aFailText, sizeof(sithAIUtilVecTest_aFailText));
    TEST_ASSERT_EQUAL_UINT_MESSAGE(0, numFailed, sithAIUtilVecTest_aFailText);
}


TEST(sithAIUtil, TestOriginalGetThingsInView)
{
    unsigned int numFailed = sithVecTest_RunCases(&sithAIUtilVec_suite, &sithAIUtilVecTest_fixture, "GetThingsInView_",
        sithAIUtilVecTest_aFailText, sizeof(sithAIUtilVecTest_aFailText));
    TEST_ASSERT_EQUAL_UINT_MESSAGE(0, numFailed, sithAIUtilVecTest_aFailText);
}


TEST(sithAIUtil, TestOriginalGetNextThingsInView)
{
    unsigned int numFailed = sithVecTest_RunCases(&sithAIUtilVec_suite, &sithAIUtilVecTest_fixture, "GetNextThingsInView_",
        sithAIUtilVecTest_aFailText, sizeof(sithAIUtilVecTest_aFailText));
    TEST_ASSERT_EQUAL_UINT_MESSAGE(0, numFailed, sithAIUtilVecTest_aFailText);
}


// Known difference: the SITH_RAND macro computes in double precision, the original in single precision (one ulp)
TEST(sithAIUtil, TestOriginalMakeRandPoint)
{
    static const char* const aKnown[] = { "MakeRandPoint_edge_rand_ulp" };
    unsigned int numFailed = sithVecTest_RunCases(&sithAIUtilVec_suite, &sithAIUtilVecTest_fixture, "MakeRandPoint_",
        sithAIUtilVecTest_aFailText, sizeof(sithAIUtilVecTest_aFailText));
    sithVecTest_ExpectKnownDifferences(numFailed, sithAIUtilVecTest_aFailText, aKnown, (unsigned int)SITHVEC_ARRAYLEN(aKnown),
        "SITH_RAND of the reconstruction differs from the original by one ulp for some random numbers; see tv/sithAIUtil/README.md.");
}


TEST(sithAIUtil, TestOriginalRetryMakeRandPoint)
{
    unsigned int numFailed = sithVecTest_RunCases(&sithAIUtilVec_suite, &sithAIUtilVecTest_fixture, "RetryMakeRandPoint_",
        sithAIUtilVecTest_aFailText, sizeof(sithAIUtilVecTest_aFailText));
    TEST_ASSERT_EQUAL_UINT_MESSAGE(0, numFailed, sithAIUtilVecTest_aFailText);
}


TEST(sithAIUtil, TestOriginalCheckPathToPos)
{
    unsigned int numFailed = sithVecTest_RunCases(&sithAIUtilVec_suite, &sithAIUtilVecTest_fixture, "CheckPathToPos_",
        sithAIUtilVecTest_aFailText, sizeof(sithAIUtilVecTest_aFailText));
    TEST_ASSERT_EQUAL_UINT_MESSAGE(0, numFailed, sithAIUtilVecTest_aFailText);
}


TEST(sithAIUtil, TestOriginalSweepMoveToPathPos)
{
    unsigned int numFailed = sithVecTest_RunCases(&sithAIUtilVec_suite, &sithAIUtilVecTest_fixture, "SweepMoveToPathPos_",
        sithAIUtilVecTest_aFailText, sizeof(sithAIUtilVecTest_aFailText));
    TEST_ASSERT_EQUAL_UINT_MESSAGE(0, numFailed, sithAIUtilVecTest_aFailText);
}


// Known difference: the SITH_RAND macro computes in double precision, the original in single precision (one ulp)
TEST(sithAIUtil, TestOriginalMakeRoamPos)
{
    static const char* const aKnown[] = { "MakeRoamPos_edge_rand_ulp" };
    unsigned int numFailed = sithVecTest_RunCases(&sithAIUtilVec_suite, &sithAIUtilVecTest_fixture, "MakeRoamPos_",
        sithAIUtilVecTest_aFailText, sizeof(sithAIUtilVecTest_aFailText));
    sithVecTest_ExpectKnownDifferences(numFailed, sithAIUtilVecTest_aFailText, aKnown, (unsigned int)SITHVEC_ARRAYLEN(aKnown),
        "SITH_RAND of the reconstruction differs from the original by one ulp for some random numbers; see tv/sithAIUtil/README.md.");
}


TEST(sithAIUtil, TestOriginalMakePathPos)
{
    unsigned int numFailed = sithVecTest_RunCases(&sithAIUtilVec_suite, &sithAIUtilVecTest_fixture, "MakePathPos_",
        sithAIUtilVecTest_aFailText, sizeof(sithAIUtilVecTest_aFailText));
    TEST_ASSERT_EQUAL_UINT_MESSAGE(0, numFailed, sithAIUtilVecTest_aFailText);
}


TEST(sithAIUtil, TestOriginalAIResetWaypoints)
{
    unsigned int numFailed = sithVecTest_RunCases(&sithAIUtilVec_suite, &sithAIUtilVecTest_fixture, "AIResetWaypoints_",
        sithAIUtilVecTest_aFailText, sizeof(sithAIUtilVecTest_aFailText));
    TEST_ASSERT_EQUAL_UINT_MESSAGE(0, numFailed, sithAIUtilVecTest_aFailText);
}


TEST_GROUP_RUNNER(sithAIUtil)
{
    RUN_TEST_CASE(sithAIUtil, TestFixtureCoversAllCases);
    RUN_TEST_CASE(sithAIUtil, TestOriginalSetWeaponFireFlags);
    RUN_TEST_CASE(sithAIUtil, TestOriginalCanAttackTarget);
    RUN_TEST_CASE(sithAIUtil, TestOriginalIsThingMoving);
    RUN_TEST_CASE(sithAIUtil, TestOriginalAIPauseMove);
    RUN_TEST_CASE(sithAIUtil, TestOriginalAISetMode);
    RUN_TEST_CASE(sithAIUtil, TestOriginalAIClearMode);
    RUN_TEST_CASE(sithAIUtil, TestOriginalAIGetMovePos);
    RUN_TEST_CASE(sithAIUtil, TestOriginalGetXYHeadingVector);
    RUN_TEST_CASE(sithAIUtil, TestOriginalGetXYZHeadingVector);
    RUN_TEST_CASE(sithAIUtil, TestOriginalApplyForce);
    RUN_TEST_CASE(sithAIUtil, TestOriginalRandomRotate);
    RUN_TEST_CASE(sithAIUtil, TestOriginalAIPlaySoundMode);
    RUN_TEST_CASE(sithAIUtil, TestOriginalAISetWpnt);
    RUN_TEST_CASE(sithAIUtil, TestOriginalAISetWpntRank);
    RUN_TEST_CASE(sithAIUtil, TestOriginalAISetWpntFlags);
    RUN_TEST_CASE(sithAIUtil, TestOriginalAIClearWpntFlags);
    RUN_TEST_CASE(sithAIUtil, TestOriginalAISetActiveWpntLayer);
    RUN_TEST_CASE(sithAIUtil, TestOriginalAIConnectWpnts);
    RUN_TEST_CASE(sithAIUtil, TestOriginalCreateAIWaypoint);
    RUN_TEST_CASE(sithAIUtil, TestOriginalClearAIWaypoint);
    RUN_TEST_CASE(sithAIUtil, TestOriginalGetAIWaypointNum);
    RUN_TEST_CASE(sithAIUtil, TestOriginalCompareAIWpntDistances);
    RUN_TEST_CASE(sithAIUtil, TestOriginalAIGetWpntThing);
    RUN_TEST_CASE(sithAIUtil, TestOriginalRankWpntsFromThing);
    RUN_TEST_CASE(sithAIUtil, TestOriginalRankWpntsFromPlayer);
    RUN_TEST_CASE(sithAIUtil, TestOriginalSelectWpntLink);
    RUN_TEST_CASE(sithAIUtil, TestOriginalCheckPathToPoint);
    RUN_TEST_CASE(sithAIUtil, TestOriginalCheckFloorAtPos);
    RUN_TEST_CASE(sithAIUtil, TestOriginalCheckPosition);
    RUN_TEST_CASE(sithAIUtil, TestOriginalAIFindNearestWpnt);
    RUN_TEST_CASE(sithAIUtil, TestOriginalAIMoveToNextWpnt);
    RUN_TEST_CASE(sithAIUtil, TestOriginalAITraverseWpnts);
    RUN_TEST_CASE(sithAIUtil, TestOriginalAIAdvanceToNextWpnt);
    RUN_TEST_CASE(sithAIUtil, TestOriginalCheckWpntMoveAlign);
    RUN_TEST_CASE(sithAIUtil, TestOriginalRenderAIWaypoints);
    RUN_TEST_CASE(sithAIUtil, TestOriginalGetDistanceToTarget);
    RUN_TEST_CASE(sithAIUtil, TestOriginalGetDistanceToTargetPos);
    RUN_TEST_CASE(sithAIUtil, TestOriginalCanSeeTarget);
    RUN_TEST_CASE(sithAIUtil, TestOriginalUpdateTargetSight);
    RUN_TEST_CASE(sithAIUtil, TestOriginalUpdateGoalSight);
    RUN_TEST_CASE(sithAIUtil, TestOriginalIsLineOfFireClear);
    RUN_TEST_CASE(sithAIUtil, TestOriginalAIFire);
    RUN_TEST_CASE(sithAIUtil, TestOriginalGetThingsInView);
    RUN_TEST_CASE(sithAIUtil, TestOriginalGetNextThingsInView);
    RUN_TEST_CASE(sithAIUtil, TestOriginalMakeRandPoint);
    RUN_TEST_CASE(sithAIUtil, TestOriginalRetryMakeRandPoint);
    RUN_TEST_CASE(sithAIUtil, TestOriginalCheckPathToPos);
    RUN_TEST_CASE(sithAIUtil, TestOriginalSweepMoveToPathPos);
    RUN_TEST_CASE(sithAIUtil, TestOriginalMakeRoamPos);
    RUN_TEST_CASE(sithAIUtil, TestOriginalMakePathPos);
    RUN_TEST_CASE(sithAIUtil, TestOriginalAIResetWaypoints);
}

void SithVecTest_RunAllTests(void)
{
    RUN_TEST_GROUP(sithAIUtil);
}

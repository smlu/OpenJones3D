#include <unity_fixture.h>

#include "sithAIUtilVec.h"

#include "sithVecTest.h"
#include "sithAIFixTest.h"

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
// The frame time of a 30 FPS frame: the original vectors were captured without one, and sithTime_CalcDeadline puts
// deadlines on 30 FPS frames only for shorter frames, so the deadlines stay the original now + interval
SITHVEC_REDIRECT_DEFINE(uint32_t, sithTime_g_frameTime) = 1000u / 30u;
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


TEST(sithAIUtil, TestOriginalCanSeeTarget)
{
    unsigned int numFailed = sithVecTest_RunCases(&sithAIUtilVec_suite, &sithAIUtilVecTest_fixture, "CanSeeTarget_",
        sithAIUtilVecTest_aFailText, sizeof(sithAIUtilVecTest_aFailText));
    TEST_ASSERT_EQUAL_UINT_MESSAGE(0, numFailed, sithAIUtilVecTest_aFailText);
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


TEST(sithAIUtil, TestOriginalMakeRandPoint)
{
    unsigned int numFailed = sithVecTest_RunCases(&sithAIUtilVec_suite, &sithAIUtilVecTest_fixture, "MakeRandPoint_",
        sithAIUtilVecTest_aFailText, sizeof(sithAIUtilVecTest_aFailText));
    TEST_ASSERT_EQUAL_UINT_MESSAGE(0, numFailed, sithAIUtilVecTest_aFailText);
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


TEST(sithAIUtil, TestOriginalMakeRoamPos)
{
    unsigned int numFailed = sithVecTest_RunCases(&sithAIUtilVec_suite, &sithAIUtilVecTest_fixture, "MakeRoamPos_",
        sithAIUtilVecTest_aFailText, sizeof(sithAIUtilVecTest_aFailText));
    TEST_ASSERT_EQUAL_UINT_MESSAGE(0, numFailed, sithAIUtilVecTest_aFailText);
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

TEST_GROUP(sithAIUtilFix);

TEST_SETUP(sithAIUtilFix)
{
    sithVecTest_AssertArenaPlacement();
    sith_g_pHS = sithVec_InitHostServices();
}

TEST_TEAR_DOWN(sithAIUtilFix)
{
    sithAIFixTest_Shutdown();
}

TEST(sithAIUtilFix, UntestedSidesKeepStartPosition)
{
    // A side step adds 0.03 to minMoveDist before checking moveDist. Neither
    // side is reached for these distances; output and goal scoring must use the
    // original start position, never uninitialized side-position locals.
    static const float aMaxDistance[] = { 0.25f, 0.26f };
    for ( size_t i = 0; i < SITHVEC_ARRAYLEN(aMaxDistance); ++i )
    {
        for ( int withGoal = 0; withGoal <= 1; ++withGoal )
        {
            SithVecCase testCase = { "untested path sides", NULL, 0, NULL };
            sithAIFixTest_Prepare(&sithAIUtilVec_suite, &testCase);
            SithAIControlBlock* pLocal = UV_LOCAL;
            pLocal->goalThing = withGoal ? UV_GOAL : NULL;
            rdVector3 start = { 1.0f, 2.0f, 3.0f };
            rdVector3 direction = { 0.0f, 1.0f, 0.0f };
            rdVector3 output = { -100.0f, -200.0f, -300.0f };
            SithAIUtilPathFlags flags = SITHAIUTIL_PATHFLAG_SIDES;
            if ( withGoal ) flags |= SITHAIUTIL_PATHFLAG_PREFERGOAL;
            TEST_ASSERT_EQUAL_INT(SITHAIUTIL_PATH_BLOCKED, sithAIUtil_CheckPathToPos(pLocal, UV_SECTOR(0), &start, flags, 0.25f, aMaxDistance[i], 0.125f, &direction, &output));
            TEST_ASSERT_EQUAL_MEMORY(&start, &output, sizeof(output));
            TEST_ASSERT_EQUAL_UINT(0, sithAIUtilVec_cfg.numSearches);
            TEST_ASSERT_EQUAL_UINT(withGoal ? 2 : 0, sithVec_numFindSectorCalls);
            TEST_ASSERT_EQUAL_UINT(withGoal ? 2 : 0, sithAIUtilVec_cfg.numLos);
            if ( withGoal )
            {
                // Both goal-visibility checks consume the initialized start
                // position. Goal is (-4,3,0), so the displacement is (5,-1,3).
                TEST_ASSERT_FLOAT_WITHIN(0.00001f, sqrtf(35.0f), pLocal->targetDistance);
                TEST_ASSERT_FLOAT_WITHIN(0.00001f, 5.0f / sqrtf(35.0f), pLocal->toGoal.x);
                TEST_ASSERT_FLOAT_WITHIN(0.00001f, -1.0f / sqrtf(35.0f), pLocal->toGoal.y);
                TEST_ASSERT_FLOAT_WITHIN(0.00001f, 3.0f / sqrtf(35.0f), pLocal->toGoal.z);
            }
        }
    }
}

TEST_GROUP_RUNNER(sithAIUtilFix)
{
    RUN_TEST_CASE(sithAIUtilFix, UntestedSidesKeepStartPosition);
}

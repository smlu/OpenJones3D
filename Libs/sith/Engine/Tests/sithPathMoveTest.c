#include <unity_fixture.h>
#include <math.h>
#include <string.h>

#include <sith/Engine/sithCollision.h>
#include <sith/Engine/sithPathMove.h>

#include "sithEngineTest.h"

// UpdateRotate is a debug-derived helper, inlined in the retail Update function.
void J3DAPI sithPathMove_UpdateRotate(SithThing* pThing, float simTime);

static SithThing sithPathMoveTest_thing;
static size_t sithPathMoveTest_numPostRotates;
static size_t sithPathMoveTest_numBuilds;
static size_t sithPathMoveTest_numApplies;
static size_t sithPathMoveTest_numMoves;
static size_t sithPathMoveTest_numNormalizes;
static rdVector3 sithPathMoveTest_postPYR;
static rdVector3 sithPathMoveTest_buildPYR;
static int sithPathMoveTest_bClearedTranslation;

static const rdMatrix34 sithPathMoveTest_identity = {
    { 1.0f, 0.0f, 0.0f }, { 0.0f, 1.0f, 0.0f },
    { 0.0f, 0.0f, 1.0f }, { 0.0f, 0.0f, 0.0f }
};

void J3DAPI sithPathMoveTest_PostRotate34(rdMatrix34* pMatrix, const rdVector3* pPYR)
{
    (void)pMatrix;
    ++sithPathMoveTest_numPostRotates;
    sithPathMoveTest_postPYR = *pPYR;
}

void J3DAPI sithPathMoveTest_PreRotate34(rdMatrix34* pMatrix, const rdVector3* pPYR)
{
    (void)pMatrix;
    (void)pPYR;
    TEST_FAIL_MESSAGE("Unexpected blocked pivot rotation");
}

void J3DAPI sithPathMoveTest_BuildRotate34(rdMatrix34* pMatrix, const rdVector3* pPYR)
{
    ++sithPathMoveTest_numBuilds;
    sithPathMoveTest_buildPYR = *pPYR;
    *pMatrix = sithPathMoveTest_identity;
}

void J3DAPI sithPathMoveTest_ApplyThingOrient(SithThing* pThing, rdMatrix34* pMatrix)
{
    ++sithPathMoveTest_numApplies;
    sithPathMoveTest_bClearedTranslation &= pThing->orient.dvec.x == 0.0f
        && pThing->orient.dvec.y == 0.0f && pThing->orient.dvec.z == 0.0f;
    pThing->orient = *pMatrix;
}

float sithPathMoveTest_moveReturnScale = 1.0f; // 0 blocks the mover (the frame-rate tests)

float J3DAPI sithPathMoveTest_MoveThing(SithThing* pThing, const rdVector3* pDirection, float distance, int flags)
{
    (void)pThing;
    (void)pDirection;
    (void)flags;
    ++sithPathMoveTest_numMoves;
    return distance * sithPathMoveTest_moveReturnScale;
}

float J3DAPI sithPathMoveTest_Normalize3Acc(rdVector3* pVector)
{
    (void)pVector;
    ++sithPathMoveTest_numNormalizes;
    return 0.0f;
}

static void sithPathMoveTest_InitThing(void)
{
    memset(&sithPathMoveTest_thing, 0, sizeof(sithPathMoveTest_thing));
    sithPathMoveTest_thing.moveType = SITH_MT_PATH;
    sithPathMoveTest_thing.orient = sithPathMoveTest_identity;
    sithPathMoveTest_thing.orient.dvec = (rdVector3){ 9.0f, 8.0f, 7.0f };
    SithPathMoveInfo* pPath = &sithPathMoveTest_thing.moveInfo.pathMovement;
    pPath->mode = SITH_PATHMOVE_MOVE | SITH_PATHMOVE_ROTATE;
    pPath->curOrient = sithPathMoveTest_identity;
    pPath->goalPYR = (rdVector3){ 2.0f, 4.0f, 8.0f };
    pPath->rotateToPYR = (rdVector3){ 10.0f, 20.0f, 30.0f };
    sithPathMoveTest_numPostRotates = sithPathMoveTest_numBuilds = sithPathMoveTest_numApplies = 0;
    sithPathMoveTest_numMoves = sithPathMoveTest_numNormalizes = 0;
    sithPathMoveTest_bClearedTranslation = 1;
}

TEST_GROUP(sithPathMove);
TEST_SETUP(sithPathMove)
{
    SithEngineTest_Startup();
    sithPathMoveTest_InitThing();
}
TEST_TEAR_DOWN(sithPathMove) { SithEngineTest_Shutdown(); }

static void sithPathMoveTest_AssertCompleted(float delta, int bPivot)
{
    const SithPathMoveInfo* pPath = &sithPathMoveTest_thing.moveInfo.pathMovement;
    TEST_ASSERT_EQUAL_FLOAT(delta, pPath->rotDelta);
    TEST_ASSERT_EQUAL_INT(SITH_PATHMOVE_MOVE, pPath->mode); // Preserve the unrelated move flag.
    TEST_ASSERT_EQUAL_size_t(1u, sithPathMoveTest_numPostRotates);
    TEST_ASSERT_EQUAL_FLOAT(2.0f * delta, sithPathMoveTest_postPYR.x);
    TEST_ASSERT_EQUAL_FLOAT(4.0f * delta, sithPathMoveTest_postPYR.y);
    TEST_ASSERT_EQUAL_FLOAT(8.0f * delta, sithPathMoveTest_postPYR.z);
    TEST_ASSERT_EQUAL_size_t(bPivot ? 0u : 1u, sithPathMoveTest_numBuilds);
    TEST_ASSERT_EQUAL_size_t(bPivot ? 1u : 2u, sithPathMoveTest_numApplies);
    if ( !bPivot )
    {
        TEST_ASSERT_EQUAL_MEMORY(&pPath->rotateToPYR, &sithPathMoveTest_buildPYR, sizeof(rdVector3));
    }
    TEST_ASSERT_TRUE(sithPathMoveTest_bClearedTranslation);
    TEST_ASSERT_EQUAL_size_t(0u, sithPathMoveTest_numMoves);
    TEST_ASSERT_EQUAL_size_t(0u, sithPathMoveTest_numNormalizes);
    TEST_ASSERT_EQUAL_MEMORY(&sithPathMoveTest_identity, &sithPathMoveTest_thing.orient, sizeof(rdMatrix34));
}

TEST(sithPathMove, TestRotationWithinBoundsRemainsActive)
{
    SithPathMoveInfo* pPath = &sithPathMoveTest_thing.moveInfo.pathMovement;
    pPath->rotDelta = 0.25f;
    pPath->rotDeltaTime = 0.5f;
    sithPathMove_UpdateRotate(&sithPathMoveTest_thing, 0.5f);
    TEST_ASSERT_EQUAL_FLOAT(0.5f, pPath->rotDelta);
    TEST_ASSERT_EQUAL_INT(SITH_PATHMOVE_MOVE | SITH_PATHMOVE_ROTATE, pPath->mode);
    TEST_ASSERT_EQUAL_size_t(1u, sithPathMoveTest_numPostRotates);
    TEST_ASSERT_EQUAL_FLOAT(1.0f, sithPathMoveTest_postPYR.x);
    TEST_ASSERT_EQUAL_FLOAT(2.0f, sithPathMoveTest_postPYR.y);
    TEST_ASSERT_EQUAL_FLOAT(4.0f, sithPathMoveTest_postPYR.z);
    TEST_ASSERT_EQUAL_size_t(1u, sithPathMoveTest_numApplies);
    TEST_ASSERT_EQUAL_size_t(0u, sithPathMoveTest_numBuilds);
    TEST_ASSERT_TRUE(sithPathMoveTest_bClearedTranslation);
}

TEST(sithPathMove, TestRotationBoundsComplete)
{
    // Equality completes too; infinities select their corresponding finite bound.
    const float aDeltas[] = { 1.0f, -1.0f, 2.0f, -2.0f, INFINITY, -INFINITY };
    for ( size_t i = 0; i < sizeof(aDeltas) / sizeof(aDeltas[0]); ++i )
    {
        sithPathMoveTest_InitThing();
        sithPathMoveTest_thing.moveInfo.pathMovement.rotDelta = aDeltas[i];
        sithPathMove_UpdateRotate(&sithPathMoveTest_thing, 1.0f);
        sithPathMoveTest_AssertCompleted(aDeltas[i] > 0.0f ? 1.0f : -1.0f, 0);
    }
}

TEST(sithPathMove, TestNaNRotationClampsAndCompletes)
{
    // NaN can originate from stored progress, rate or the timestep. Original retail/debug
    // select the lower clamp, then finish the rotation; passing NaN through leaves it active.
    for ( int source = 0; source < 3; ++source )
    {
        sithPathMoveTest_InitThing();
        SithPathMoveInfo* pPath = &sithPathMoveTest_thing.moveInfo.pathMovement;
        pPath->rotDelta = source == 0 ? NAN : 0.0f;
        pPath->rotDeltaTime = source == 1 ? NAN : 1.0f;
        sithPathMove_UpdateRotate(&sithPathMoveTest_thing, source == 2 ? NAN : 1.0f);
        sithPathMoveTest_AssertCompleted(-1.0f, 0);
    }
}

TEST(sithPathMove, TestPivotNaNCompletesWithoutFinalBuild)
{
    SithPathMoveInfo* pPath = &sithPathMoveTest_thing.moveInfo.pathMovement;
    pPath->mode |= SITH_PATHMOVE_ROTATE_PIVOT;
    pPath->rotDelta = NAN;
    // Coincident pivot/position isolates completion without invoking collision movement.
    sithPathMove_UpdateRotate(&sithPathMoveTest_thing, 1.0f);
    sithPathMoveTest_AssertCompleted(-1.0f, 1);
}

TEST_GROUP_RUNNER(sithPathMove)
{
    RUN_TEST_CASE(sithPathMove, TestRotationWithinBoundsRemainsActive);
    RUN_TEST_CASE(sithPathMove, TestRotationBoundsComplete);
    RUN_TEST_CASE(sithPathMove, TestNaNRotationClampsAndCompletes);
    RUN_TEST_CASE(sithPathMove, TestPivotNaNCompletesWithoutFinalBuild);
}

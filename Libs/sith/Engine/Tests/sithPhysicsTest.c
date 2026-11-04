#include <unity_fixture.h>
#include <string.h>

#include <rdroid/Math/rdVector.h>
#include <sith/Engine/sithPhysics.h>

#include "sithEngineTest.h"

static SithThing sithPhysicsTest_thing;
static size_t sithPhysicsTest_numSteps;
static size_t sithPhysicsTest_maxSteps;

void J3DAPI sithPhysicsTest_MultAcc3(rdVector3* pAcc, const rdVector3* pVector, float scale)
{
    TEST_ASSERT_EQUAL_PTR(&sithPhysicsTest_thing.moveInfo.physics.deltaVelocity, pAcc);
    ++sithPhysicsTest_numSteps;
    TEST_ASSERT_TRUE_MESSAGE(sithPhysicsTest_numSteps <= sithPhysicsTest_maxSteps,
        "Unexpected integration step (negative accumulated time must not wrap)");
    TEST_ASSERT_EQUAL_FLOAT(0.02f, scale);
    rdVector_MultAcc3(pAcc, pVector, scale);
}

static void sithPhysicsTest_InitThing(void)
{
    memset(&sithPhysicsTest_thing, 0, sizeof(sithPhysicsTest_thing));
    sithPhysicsTest_thing.type = SITH_THING_PLAYER;
    sithPhysicsTest_thing.moveType = SITH_MT_PHYSICS;
    sithPhysicsTest_thing.orient.rvec.x = 1.0f;
    sithPhysicsTest_thing.orient.lvec.y = 1.0f;
    sithPhysicsTest_thing.orient.uvec.z = 1.0f;
    // No raft, angular thrust, drag, mass or sector forces: observe the actual
    // fixed-step integrator without loading world/device globals.
    sithPhysicsTest_thing.moveInfo.physics.flags = SITH_PF_FLY;
    sithPhysicsTest_thing.moveInfo.physics.velocity = (rdVector3){ 1.0f, 2.0f, 3.0f };
    sithPhysicsTest_numSteps = 0;
    sithPhysicsTest_maxSteps = 0;
}

static void sithPhysicsTest_AssertVector(const rdVector3* pExpected, const rdVector3* pActual)
{
    TEST_ASSERT_FLOAT_WITHIN(0.000001f, pExpected->x, pActual->x);
    TEST_ASSERT_FLOAT_WITHIN(0.000001f, pExpected->y, pActual->y);
    TEST_ASSERT_FLOAT_WITHIN(0.000001f, pExpected->z, pActual->z);
}

TEST_GROUP(sithPhysics);
TEST_SETUP(sithPhysics)
{
    SithEngineTest_Startup();
    sithPhysicsTest_InitThing();
}
TEST_TEAR_DOWN(sithPhysics) { SithEngineTest_Shutdown(); }

TEST(sithPhysics, TestNegativeAccumulatedTimeDoesNotIntegrate)
{
    static const struct { float timestep; float rollover; } aCases[] = {
        { -0.02f, 0.0f }, { -0.005f, 0.0f }, { 0.01f, -0.03f }
    };
    for ( size_t i = 0; i < sizeof(aCases) / sizeof(aCases[0]); ++i )
    {
        sithPhysicsTest_InitThing();
        SithPhysicsInfo* pPhysics = &sithPhysicsTest_thing.moveInfo.physics;
        pPhysics->physicsRolloverFrames = aCases[i].rollover;
        pPhysics->deltaVelocity = (rdVector3){ 4.0f, 5.0f, 6.0f };
        pPhysics->gravityForce = (rdVector3){ 7.0f, 8.0f, 9.0f };
        const rdVector3 velocity = pPhysics->velocity;
        const rdVector3 delta = pPhysics->deltaVelocity;
        const rdVector3 zero = { 0 };

        sithPhysics_UpdateDetachedPlayerPhysics(&sithPhysicsTest_thing, aCases[i].timestep);

        TEST_ASSERT_EQUAL_size_t(0, sithPhysicsTest_numSteps);
        TEST_ASSERT_EQUAL_FLOAT(aCases[i].timestep + aCases[i].rollover, pPhysics->physicsRolloverFrames);
        TEST_ASSERT_EQUAL_MEMORY(&velocity, &pPhysics->velocity, sizeof(velocity));
        TEST_ASSERT_EQUAL_MEMORY(&delta, &pPhysics->deltaVelocity, sizeof(delta));
        TEST_ASSERT_EQUAL_MEMORY(&zero, &pPhysics->gravityForce, sizeof(zero));
    }
}

TEST(sithPhysics, TestZeroAndFractionalStepsPreserveRemainder)
{
    SithPhysicsInfo* pPhysics = &sithPhysicsTest_thing.moveInfo.physics;
    sithPhysics_UpdateDetachedPlayerPhysics(&sithPhysicsTest_thing, 0.0f);
    TEST_ASSERT_EQUAL_size_t(0, sithPhysicsTest_numSteps);
    TEST_ASSERT_EQUAL_FLOAT(0.0f, pPhysics->physicsRolloverFrames);

    pPhysics->physicsRolloverFrames = -0.005f;
    sithPhysics_UpdateDetachedPlayerPhysics(&sithPhysicsTest_thing, 0.01f);
    TEST_ASSERT_EQUAL_size_t(0, sithPhysicsTest_numSteps);
    TEST_ASSERT_FLOAT_WITHIN(0.000001f, 0.005f, pPhysics->physicsRolloverFrames);

    // After repaying the negative remainder above, the next frame takes one
    // 20 ms step; it must not discard the remaining 5 ms.
    sithPhysicsTest_maxSteps = 1;
    sithPhysics_UpdateDetachedPlayerPhysics(&sithPhysicsTest_thing, 0.02f);
    const rdVector3 expectedDelta = { 0.02f, 0.04f, 0.06f };
    TEST_ASSERT_EQUAL_size_t(1, sithPhysicsTest_numSteps);
    TEST_ASSERT_FLOAT_WITHIN(0.000001f, 0.005f, pPhysics->physicsRolloverFrames);
    sithPhysicsTest_AssertVector(&expectedDelta, &pPhysics->deltaVelocity);
}

TEST(sithPhysics, TestPositiveTimeIntegratesOnlyCompleteSteps)
{
    static const struct { float timestep; float rollover; size_t steps; float remainder; } aCases[] = {
        { 0.02f, 0.0f, 1, 0.0f }, { 0.05f, 0.0f, 2, 0.01f }, { 0.02f, 0.01f, 1, 0.01f }
    };
    for ( size_t i = 0; i < sizeof(aCases) / sizeof(aCases[0]); ++i )
    {
        sithPhysicsTest_InitThing();
        SithPhysicsInfo* pPhysics = &sithPhysicsTest_thing.moveInfo.physics;
        pPhysics->physicsRolloverFrames = aCases[i].rollover;
        sithPhysicsTest_maxSteps = aCases[i].steps;
        sithPhysics_UpdateDetachedPlayerPhysics(&sithPhysicsTest_thing, aCases[i].timestep);
        const float time = (float)aCases[i].steps * 0.02f;
        const rdVector3 expectedDelta = { time, 2.0f * time, 3.0f * time };
        TEST_ASSERT_EQUAL_size_t(aCases[i].steps, sithPhysicsTest_numSteps);
        TEST_ASSERT_FLOAT_WITHIN(0.000001f, aCases[i].remainder, pPhysics->physicsRolloverFrames);
        sithPhysicsTest_AssertVector(&expectedDelta, &pPhysics->deltaVelocity);
    }
}

TEST(sithPhysics, TestThrustIsAppliedForEachFixedStep)
{
    SithPhysicsInfo* pPhysics = &sithPhysicsTest_thing.moveInfo.physics;
    pPhysics->flags |= SITH_PF_USETHRUST;
    pPhysics->velocity = (rdVector3){ 0 };
    pPhysics->thrust = (rdVector3){ 10.0f, 0.0f, 0.0f };
    sithPhysicsTest_maxSteps = 2;

    sithPhysics_UpdateDetachedPlayerPhysics(&sithPhysicsTest_thing, 0.05f);

    const rdVector3 velocity = { 0.4f, 0.0f, 0.0f };
    const rdVector3 delta = { 0.012f, 0.0f, 0.0f };
    TEST_ASSERT_EQUAL_size_t(2, sithPhysicsTest_numSteps);
    TEST_ASSERT_FLOAT_WITHIN(0.000001f, 0.01f, pPhysics->physicsRolloverFrames);
    sithPhysicsTest_AssertVector(&velocity, &pPhysics->velocity);
    sithPhysicsTest_AssertVector(&delta, &pPhysics->deltaVelocity);
}

TEST_GROUP_RUNNER(sithPhysics)
{
    RUN_TEST_CASE(sithPhysics, TestNegativeAccumulatedTimeDoesNotIntegrate);
    RUN_TEST_CASE(sithPhysics, TestZeroAndFractionalStepsPreserveRemainder);
    RUN_TEST_CASE(sithPhysics, TestPositiveTimeIntegratesOnlyCompleteSteps);
    RUN_TEST_CASE(sithPhysics, TestThrustIsAppliedForEachFixedStep);
}

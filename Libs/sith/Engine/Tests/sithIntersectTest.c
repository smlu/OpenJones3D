#include <unity_fixture.h>
#include <math.h>

#include <sith/Engine/sithIntersect.h>

#include "sithEngineTest.h"

TEST_GROUP(sithIntersect);

TEST_SETUP(sithIntersect) { SithEngineTest_Startup(); }
TEST_TEAR_DOWN(sithIntersect) { SithEngineTest_Shutdown(); }

// Expected branches/results were independently checked against retail v1.2 CheckSphereHit.
// The non-finite cases exercise
// the original unordered comparisons; the permanent suite calls only reconstructed code.
TEST(sithIntersect, TestSphereHitTowardPlane)
{
    const rdVector3 start = { 2.0f, 0.0f, 0.0f };
    const rdVector3 move = { -1.0f, 0.0f, 0.0f };
    const rdVector3 normal = { 1.0f, 0.0f, 0.0f };
    const rdVector3 point = { 0.0f, 0.0f, 0.0f };
    float distance = 17.0f;

    TEST_ASSERT_EQUAL_INT(1, sithIntersect_CheckSphereHit(&start, &move, 10.0f, 0.5f, &normal, &point, &distance, 0));
    TEST_ASSERT_EQUAL_FLOAT(1.5f, distance);
}

TEST(sithIntersect, TestSphereMissPreservesDistance)
{
    static const struct
    {
        rdVector3 move;
        float moveDistance;
    } aCases[] = {
        { { 1.0f, 0.0f, 0.0f }, 10.0f }, // Moving away.
        { { 0.0f, 1.0f, 0.0f }, 10.0f }, // Parallel to the plane.
        { { -1.0f, 0.0f, 0.0f }, 1.0f }, // Cannot reach the sphere/plane contact.
    };
    const rdVector3 start = { 2.0f, 0.0f, 0.0f };
    const rdVector3 normal = { 1.0f, 0.0f, 0.0f };
    const rdVector3 point = { 0.0f, 0.0f, 0.0f };
    for ( size_t i = 0; i < sizeof(aCases) / sizeof(aCases[0]); ++i )
    {
        float distance = 17.0f;
        TEST_ASSERT_EQUAL_INT(0, sithIntersect_CheckSphereHit(&start, &aCases[i].move, aCases[i].moveDistance, 0.5f, &normal, &point, &distance, 0));
        TEST_ASSERT_EQUAL_FLOAT(17.0f, distance);
    }
}

TEST(sithIntersect, TestSphereOverlapFlags)
{
    const rdVector3 start = { 0.25f, 0.0f, 0.0f };
    const rdVector3 move = { -1.0f, 0.0f, 0.0f };
    const rdVector3 normal = { 1.0f, 0.0f, 0.0f };
    const rdVector3 point = { 0.0f, 0.0f, 0.0f };
    float distance = 17.0f;
    TEST_ASSERT_EQUAL_INT(1, sithIntersect_CheckSphereHit(&start, &move, 10.0f, 0.5f, &normal, &point, &distance, 0));
    TEST_ASSERT_EQUAL_FLOAT(0.0f, distance);

    // Flag 0x400 adds the radius to the supplied distance instead of clearing it.
    distance = 17.0f;
    TEST_ASSERT_EQUAL_INT(1, sithIntersect_CheckSphereHit(&start, &move, 10.0f, 0.5f, &normal, &point, &distance, 0x400));
    TEST_ASSERT_EQUAL_FLOAT(17.5f, distance);
}

TEST(sithIntersect, TestNaNDirectionRejectedWithoutWritingDistance)
{
    const rdVector3 start = { 2.0f, 0.0f, 0.0f };
    const rdVector3 move = { NAN, 0.0f, 0.0f };
    const rdVector3 normal = { 1.0f, 0.0f, 0.0f };
    const rdVector3 point = { 0.0f, 0.0f, 0.0f };
    float distance = 17.0f;

    // A <= 0 rejection lets NaN reach division and the release assertion.
    TEST_ASSERT_EQUAL_INT(0, sithIntersect_CheckSphereHit(&start, &move, 10.0f, 0.0f, &normal, &point, &distance, 0));
    TEST_ASSERT_EQUAL_FLOAT(17.0f, distance);
}

TEST(sithIntersect, TestUnorderedReachRejectedWithoutWritingDistance)
{
    const rdVector3 start = { 2.0f, 0.0f, 0.0f };
    const rdVector3 move = { -1.0f, 0.0f, 0.0f };
    const rdVector3 infiniteMove = { -INFINITY, 0.0f, 0.0f };
    const rdVector3 normal = { 1.0f, 0.0f, 0.0f };
    const rdVector3 point = { 0.0f, 0.0f, 0.0f };
    float distance = 17.0f;
    TEST_ASSERT_EQUAL_INT(0, sithIntersect_CheckSphereHit(&start, &move, NAN, 0.0f, &normal, &point, &distance, 0));
    TEST_ASSERT_EQUAL_FLOAT(17.0f, distance);

    // Infinity * zero makes the reach unordered even when its individual inputs are not NaN.
    TEST_ASSERT_EQUAL_INT(0, sithIntersect_CheckSphereHit(&start, &infiniteMove, 0.0f, 2.0f, &normal, &point, &distance, 0));
    TEST_ASSERT_EQUAL_FLOAT(17.0f, distance);
}

TEST(sithIntersect, TestUnorderedSphereDistanceUsesOriginalOverlapPath)
{
    const rdVector3 start = { 2.0f, 0.0f, 0.0f };
    const rdVector3 move = { -1.0f, 0.0f, 0.0f };
    const rdVector3 normal = { 1.0f, 0.0f, 0.0f };
    const rdVector3 point = { 0.0f, 0.0f, 0.0f };
    float distance = 17.0f;

    // This preserves the original NaN-radius branch; it is not a claim that the input is valid game state.
    TEST_ASSERT_EQUAL_INT(1, sithIntersect_CheckSphereHit(&start, &move, 10.0f, NAN, &normal, &point, &distance, 0));
    TEST_ASSERT_EQUAL_FLOAT(0.0f, distance);
    distance = 17.0f;
    TEST_ASSERT_EQUAL_INT(1, sithIntersect_CheckSphereHit(&start, &move, 10.0f, NAN, &normal, &point, &distance, 0x400));
    TEST_ASSERT_TRUE(isnan(distance));
}

TEST_GROUP_RUNNER(sithIntersect)
{
    RUN_TEST_CASE(sithIntersect, TestSphereHitTowardPlane);
    RUN_TEST_CASE(sithIntersect, TestSphereMissPreservesDistance);
    RUN_TEST_CASE(sithIntersect, TestSphereOverlapFlags);
    RUN_TEST_CASE(sithIntersect, TestNaNDirectionRejectedWithoutWritingDistance);
    RUN_TEST_CASE(sithIntersect, TestUnorderedReachRejectedWithoutWritingDistance);
    RUN_TEST_CASE(sithIntersect, TestUnorderedSphereDistanceUsesOriginalOverlapPath);
}

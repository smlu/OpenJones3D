#include <unity_fixture.h>

#include <stdint.h>
#include <string.h>

#include <sith/Engine/sithPhysics.h>
#include <std/General/stdUtil.h>

// These helpers are not exposed by the header
void J3DAPI sithPhysics_sub_487EC0(SithThing* pThing, SithPhysicsTrackFaceData* pData, float* secDeltaTime, rdVector3* a4, rdVector3* a5, float a6);
void J3DAPI sithPhysics_Jeep_sub_48B4D0(SithThing* pThing, SithJeepState* a2);

TEST_GROUP(sithPhysics);

TEST_SETUP(sithPhysics)
{}

TEST_TEAR_DOWN(sithPhysics)
{}

// Move distance is shorter than the distance to the face front, the position ends on the track and no time is left.
TEST(sithPhysics, MoveToTrackFaceFrontStaysInFace)
{
    SithPhysicsTrackFaceData data;
    STD_ZEROMEM(&data, sizeof(data));
    data.forward.y       = 1.0f;
    data.front.y         = 2.0f;
    data.distanceToFront = 1.0f;

    float secDeltaTime = 0.5f;
    rdVector3 startPos = { 0 };
    rdVector3 outPos   = { 0 };

    sithPhysics_sub_487EC0(NULL, &data, &secDeltaTime, &startPos, &outPos, 1.0f);

    TEST_ASSERT_EQUAL_FLOAT(0.0f, secDeltaTime);
    TEST_ASSERT_EQUAL_FLOAT(0.0f, outPos.x);
    TEST_ASSERT_EQUAL_FLOAT(1.5f, outPos.y); // front + forward * (moveDist - distanceToFront)
    TEST_ASSERT_EQUAL_FLOAT(0.0f, outPos.z);
}

// Move distance passes the face front, the position is clamped to the front and the unused time is returned.
TEST(sithPhysics, MoveToTrackFaceFrontOvershoot)
{
    SithPhysicsTrackFaceData data;
    STD_ZEROMEM(&data, sizeof(data));
    data.forward.y       = 1.0f;
    data.front.y         = 2.0f;
    data.distanceToFront = 1.0f;

    float secDeltaTime = 2.0f;
    rdVector3 startPos = { 0 };
    rdVector3 outPos   = { 0 };

    sithPhysics_sub_487EC0(NULL, &data, &secDeltaTime, &startPos, &outPos, 1.0f);

    TEST_ASSERT_EQUAL_FLOAT(0.0f, outPos.x);
    TEST_ASSERT_EQUAL_FLOAT(2.0f, outPos.y);
    TEST_ASSERT_EQUAL_FLOAT(0.0f, outPos.z);
    TEST_ASSERT_EQUAL_FLOAT(1.0f, secDeltaTime); // 2 - (1 / 2) * 2
}

TEST(sithPhysics, ResetJeepWheelsSetsDefaults)
{
    SithJeepState state;
    memset(&state, 0xCD, sizeof(state));

    sithPhysics_Jeep_sub_48B4D0(NULL, &state);

    TEST_ASSERT_EQUAL_FLOAT(0.14f, state.width);
    TEST_ASSERT_EQUAL_FLOAT(0.22f, state.length);
    TEST_ASSERT_EQUAL_FLOAT(0.18f, state.height);
    for ( size_t i = 0; i < 4; ++i )
    {
        TEST_ASSERT_EQUAL_FLOAT(0.01f, state.aWheels[i].width);
        TEST_ASSERT_EQUAL_INT(0xCDCDCDCD, state.aWheels[i].contactState);
        TEST_ASSERT_EQUAL_HEX32(0xCDCDCDCD, *(uint32_t*)&state.aWheels[i].compressionDist);
        TEST_ASSERT_EQUAL_HEX32(0xCDCDCDCD, *(uint32_t*)&state.aWheels[i].initPos.x);
    }
    TEST_ASSERT_EQUAL_HEX32(0xCDCDCDCD, *(uint32_t*)&state.centerOfMass.x);
    TEST_ASSERT_EQUAL_INT(0xCDCDCDCD, state.bSkidding);
}

TEST_GROUP_RUNNER(sithPhysics)
{
    RUN_TEST_CASE(sithPhysics, MoveToTrackFaceFrontStaysInFace);
    RUN_TEST_CASE(sithPhysics, MoveToTrackFaceFrontOvershoot);
    RUN_TEST_CASE(sithPhysics, ResetJeepWheelsSetsDefaults);
}

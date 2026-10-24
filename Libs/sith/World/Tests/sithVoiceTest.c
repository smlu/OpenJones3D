#include <unity_fixture.h>

#include <stdint.h>

#include <sith/World/sithVoice.h>
#include <sound/Sound.h>
#include <std/General/stdUtil.h>

#define SITHVOICE_TEST_CHANNEL_HANDLE 0x40000001u
#define SITHVOICE_TEST_HEAD_MESH_NUM  7
#define SITHVOICE_TEST_SWAP_REF_NUM   73

static SithThing sithVoiceTest_thing;
static SithThing sithVoiceTest_otherThing;
static SithThingSwapEntry sithVoiceTest_unrelatedSwap;
static rdModel3 sithVoiceTest_aHeadModels[4];

TEST_GROUP(sithVoice);

static SithActorVoiceInfo* sithVoiceTest_GetVoiceInfo(SithThing* pThing)
{
    return &pThing->thingInfo.actorInfo.voiceInfo;
}

static void sithVoiceTest_InitThing(SithThing* pThing)
{
    STD_ZEROMEM(pThing, sizeof(*pThing));
    pThing->type = SITH_THING_ACTOR;

    SithActorVoiceInfo* pVoiceInfo = sithVoiceTest_GetVoiceInfo(pThing);
    pVoiceInfo->hSndChannel                  = SITHVOICE_TEST_CHANNEL_HANDLE;
    pVoiceInfo->voiceHeadInfo.headMeshNum    = SITHVOICE_TEST_HEAD_MESH_NUM;
    pVoiceInfo->voiceHeadInfo.headSwapRefNum = -1;

    for ( size_t i = 0; i < STD_ARRAYLEN(pVoiceInfo->voiceHeadInfo.apSoundHeadModels); ++i )
    {
        pVoiceInfo->voiceHeadInfo.apSoundHeadModels[i] = &sithVoiceTest_aHeadModels[i];
    }
}

static void sithVoiceTest_SetShippedTable(void)
{
    for ( size_t mouthYLevel = 0; mouthYLevel < 4; ++mouthYLevel )
    {
        for ( size_t mouthXLevel = 0; mouthXLevel < 4; ++mouthXLevel )
        {
            sithVoice_TestSetHeadTableEntry(mouthYLevel, mouthXLevel, (uint8_t)mouthYLevel);
        }
    }
}

static SithVoiceTestState sithVoiceTest_Update(SithThing* pThing, float secGameTime, uint8_t mouthPosX, uint8_t mouthPosY)
{
    sithVoice_TestSetGameTime(secGameTime);
    sithVoice_TestSetLipSyncResult(1, mouthPosX, mouthPosY);
    sithVoice_UpdateLipSync(pThing);

    SithVoiceTestState state;
    sithVoice_TestGetState(&state);
    return state;
}

TEST_SETUP(sithVoice)
{
    STD_ZEROMEM(sithVoiceTest_aHeadModels, sizeof(sithVoiceTest_aHeadModels));
    STD_ZEROMEM(&sithVoiceTest_unrelatedSwap, sizeof(sithVoiceTest_unrelatedSwap));
    sithVoice_TestResetState();
    sithVoiceTest_InitThing(&sithVoiceTest_thing);
    sithVoiceTest_InitThing(&sithVoiceTest_otherThing);
}

TEST_TEAR_DOWN(sithVoice)
{
}

TEST(sithVoice, TestAllTableCells)
{
    for ( size_t mouthYLevel = 0; mouthYLevel < 4; ++mouthYLevel )
    {
        for ( size_t mouthXLevel = 0; mouthXLevel < 4; ++mouthXLevel )
        {
            uint8_t expectedHeadSlot = (uint8_t)((mouthYLevel * 3 + mouthXLevel) % 4);

            sithVoice_TestResetState();
            sithVoiceTest_InitThing(&sithVoiceTest_thing);
            sithVoice_TestSetHeadTableEntry(mouthYLevel, mouthXLevel, expectedHeadSlot);

            SithVoiceTestState state = sithVoiceTest_Update(
                &sithVoiceTest_thing,
                1.0f,
                (uint8_t)(mouthXLevel * SOUND_LIPSYNC_MOUTH_LEVEL_STEP),
                (uint8_t)(mouthYLevel * SOUND_LIPSYNC_MOUTH_LEVEL_STEP)
            );

            TEST_ASSERT_EQUAL_INT(expectedHeadSlot, state.curHeadSlot);
            TEST_ASSERT_EQUAL_PTR(&sithVoiceTest_aHeadModels[expectedHeadSlot], state.pLastAddedModel);
            TEST_ASSERT_EQUAL_INT(SITHVOICE_TEST_HEAD_MESH_NUM, state.lastAddedMeshNum);
            TEST_ASSERT_EQUAL_INT(0, state.lastAddedSrcMeshNum);
        }
    }
}

TEST(sithVoice, TestShippedTableIgnoresMouthX)
{
    sithVoiceTest_SetShippedTable();

    for ( size_t mouthYLevel = 0; mouthYLevel < 4; ++mouthYLevel )
    {
        for ( size_t mouthXLevel = 0; mouthXLevel < 4; ++mouthXLevel )
        {
            sithVoice_TestSetSelectionState(-1, 0, 0.0f);

            SithVoiceTestState state = sithVoiceTest_Update(
                &sithVoiceTest_thing,
                1.0f,
                (uint8_t)(mouthXLevel * SOUND_LIPSYNC_MOUTH_LEVEL_STEP),
                (uint8_t)(mouthYLevel * SOUND_LIPSYNC_MOUTH_LEVEL_STEP)
            );

            TEST_ASSERT_EQUAL_INT((int)mouthYLevel, state.curHeadSlot);
            TEST_ASSERT_EQUAL_PTR(&sithVoiceTest_aHeadModels[mouthYLevel], state.pLastAddedModel);
        }
    }
}

TEST(sithVoice, TestRepeatedHeadVariation)
{
    static const int aExpectedHeads[4][4] = {
        { 0, 0, 0, 0 },
        { 1, 1, 3, 1 },
        { 2, 2, 0, 2 },
        { 3, 3, 1, 3 }
    };

    for ( size_t headSlot = 0; headSlot < STD_ARRAYLEN(aExpectedHeads); ++headSlot )
    {
        sithVoice_TestResetState();
        sithVoiceTest_InitThing(&sithVoiceTest_thing);
        sithVoice_TestSetHeadTableEntry(0, 0, (uint8_t)headSlot);

        for ( size_t updateNum = 0; updateNum < STD_ARRAYLEN(aExpectedHeads[headSlot]); ++updateNum )
        {
            SithVoiceTestState state = sithVoiceTest_Update(&sithVoiceTest_thing, 1.0f + (float)updateNum * 0.2f, 0, 0);

            TEST_ASSERT_EQUAL_INT(aExpectedHeads[headSlot][updateNum], state.curHeadSlot);
            TEST_ASSERT_EQUAL_PTR(&sithVoiceTest_aHeadModels[aExpectedHeads[headSlot][updateNum]], state.pLastAddedModel);
        }
    }
}

TEST(sithVoice, TestMissingMHeadRemovesOnlyTrackedSwap)
{
    SithActorVoiceInfo* pVoiceInfo = sithVoiceTest_GetVoiceInfo(&sithVoiceTest_thing);
    pVoiceInfo->voiceHeadInfo.apSoundHeadModels[0] = NULL;
    pVoiceInfo->voiceHeadInfo.headSwapRefNum       = SITHVOICE_TEST_SWAP_REF_NUM;
    sithVoiceTest_thing.pSwapList                  = &sithVoiceTest_unrelatedSwap;

    SithVoiceTestState state = sithVoiceTest_Update(&sithVoiceTest_thing, 1.0f, 0, 0);

    TEST_ASSERT_EQUAL_size_t(0u, state.numAddSwapCalls);
    TEST_ASSERT_EQUAL_size_t(1u, state.numRemoveSwapCalls);
    TEST_ASSERT_EQUAL_INT(SITHVOICE_TEST_SWAP_REF_NUM, state.lastRemovedRefNum);
    TEST_ASSERT_EQUAL_PTR(&sithVoiceTest_unrelatedSwap, sithVoiceTest_thing.pSwapList);
    TEST_ASSERT_EQUAL_INT(-1, pVoiceInfo->voiceHeadInfo.headSwapRefNum);
}

TEST(sithVoice, TestTerminationRestoresExistingMHead)
{
    SithActorVoiceInfo* pVoiceInfo = sithVoiceTest_GetVoiceInfo(&sithVoiceTest_thing);
    pVoiceInfo->voiceHeadInfo.headSwapRefNum = SITHVOICE_TEST_SWAP_REF_NUM;
    sithVoice_TestSetThingHasSwapHead(1);
    sithVoice_TestSetLipSyncResult(0, 0, 0);

    sithVoice_UpdateLipSync(&sithVoiceTest_thing);

    SithVoiceTestState state;
    sithVoice_TestGetState(&state);
    TEST_ASSERT_EQUAL_size_t(1u, state.numAddSwapCalls);
    TEST_ASSERT_EQUAL_size_t(0u, state.numRemoveSwapCalls);
    TEST_ASSERT_EQUAL_PTR(&sithVoiceTest_aHeadModels[0], state.pLastAddedModel);
    TEST_ASSERT_FALSE(state.bThingHasSwapHead);
    TEST_ASSERT_EQUAL_UINT32(SOUND_INVALIDHANDLE, pVoiceInfo->hSndChannel);
    TEST_ASSERT_EQUAL_INT(-1, pVoiceInfo->voiceHeadInfo.headSwapRefNum);
}

TEST(sithVoice, TestTerminationRemovesTrackedSwap)
{
    SithActorVoiceInfo* pVoiceInfo = sithVoiceTest_GetVoiceInfo(&sithVoiceTest_thing);
    pVoiceInfo->voiceHeadInfo.headSwapRefNum = SITHVOICE_TEST_SWAP_REF_NUM;
    sithVoiceTest_thing.pSwapList            = &sithVoiceTest_unrelatedSwap;
    sithVoice_TestSetLipSyncResult(0, 0, 0);

    sithVoice_UpdateLipSync(&sithVoiceTest_thing);

    SithVoiceTestState state;
    sithVoice_TestGetState(&state);
    TEST_ASSERT_EQUAL_size_t(0u, state.numAddSwapCalls);
    TEST_ASSERT_EQUAL_size_t(1u, state.numRemoveSwapCalls);
    TEST_ASSERT_EQUAL_INT(SITHVOICE_TEST_SWAP_REF_NUM, state.lastRemovedRefNum);
    TEST_ASSERT_EQUAL_PTR(&sithVoiceTest_unrelatedSwap, sithVoiceTest_thing.pSwapList);
    TEST_ASSERT_EQUAL_UINT32(SOUND_INVALIDHANDLE, pVoiceInfo->hSndChannel);
}

TEST(sithVoice, TestDyingThingStopsVoiceChannel)
{
    sithVoiceTest_thing.flags |= SITH_TF_DYING;

    SithVoiceTestState state = sithVoiceTest_Update(&sithVoiceTest_thing, 1.0f, 0, 0);

    TEST_ASSERT_EQUAL_size_t(1u, state.numStopCalls);
    TEST_ASSERT_EQUAL_UINT32(SITHVOICE_TEST_CHANNEL_HANDLE, state.hLastStoppedChannel);
}

TEST(sithVoice, TestHeadUpdateWaitsForGlobalDeadline)
{
    sithVoice_TestSetHeadTableEntry(0, 0, 1);
    sithVoice_TestSetSelectionState(-1, 0, 5.0f);

    SithVoiceTestState state = sithVoiceTest_Update(&sithVoiceTest_thing, 4.0f, 0, 0);
    TEST_ASSERT_EQUAL_size_t(0u, state.numAddSwapCalls);

    state = sithVoiceTest_Update(&sithVoiceTest_thing, 5.0f, 0, 0);
    TEST_ASSERT_EQUAL_size_t(0u, state.numAddSwapCalls);

    state = sithVoiceTest_Update(&sithVoiceTest_thing, 5.001f, 0, 0);
    TEST_ASSERT_EQUAL_size_t(1u, state.numAddSwapCalls);
    TEST_ASSERT_EQUAL_INT(1, state.curHeadSlot);
}

TEST(sithVoice, TestGlobalDeadlineIsSharedBySpeakers)
{
    sithVoice_TestSetHeadTableEntry(0, 0, 1);

    SithVoiceTestState state = sithVoiceTest_Update(&sithVoiceTest_thing, 1.0f, 0, 0);
    TEST_ASSERT_EQUAL_size_t(1u, state.numAddSwapCalls);

    state = sithVoiceTest_Update(&sithVoiceTest_otherThing, 1.05f, 0, 0);
    TEST_ASSERT_EQUAL_size_t(1u, state.numAddSwapCalls);

    // NOTE: This characterizes the original global scheduler; it is not per-speaker state.
    state = sithVoiceTest_Update(&sithVoiceTest_otherThing, 1.11f, 0, 0);
    TEST_ASSERT_EQUAL_size_t(2u, state.numAddSwapCalls);
    TEST_ASSERT_EQUAL_PTR(&sithVoiceTest_otherThing, state.pLastSwapThing);
}

TEST(sithVoice, TestExistingHeadStateIsSharedBySpeakers)
{
    SithActorVoiceInfo* pVoiceInfo = sithVoiceTest_GetVoiceInfo(&sithVoiceTest_otherThing);
    pVoiceInfo->voiceHeadInfo.headSwapRefNum = SITHVOICE_TEST_SWAP_REF_NUM;
    sithVoice_TestSetThingHasSwapHead(1);
    sithVoice_TestSetLipSyncResult(0, 0, 0);

    sithVoice_UpdateLipSync(&sithVoiceTest_otherThing);

    SithVoiceTestState state;
    sithVoice_TestGetState(&state);

    // NOTE: This characterizes the original global flag; it does not identify which speaker owned the existing head.
    TEST_ASSERT_EQUAL_size_t(1u, state.numAddSwapCalls);
    TEST_ASSERT_EQUAL_size_t(0u, state.numRemoveSwapCalls);
    TEST_ASSERT_EQUAL_PTR(&sithVoiceTest_otherThing, state.pLastSwapThing);
    TEST_ASSERT_EQUAL_PTR(&sithVoiceTest_aHeadModels[0], state.pLastAddedModel);
    TEST_ASSERT_FALSE(state.bThingHasSwapHead);
}

TEST_GROUP_RUNNER(sithVoice)
{
    RUN_TEST_CASE(sithVoice, TestAllTableCells);
    RUN_TEST_CASE(sithVoice, TestShippedTableIgnoresMouthX);
    RUN_TEST_CASE(sithVoice, TestRepeatedHeadVariation);
    RUN_TEST_CASE(sithVoice, TestMissingMHeadRemovesOnlyTrackedSwap);
    RUN_TEST_CASE(sithVoice, TestTerminationRestoresExistingMHead);
    RUN_TEST_CASE(sithVoice, TestTerminationRemovesTrackedSwap);
    RUN_TEST_CASE(sithVoice, TestDyingThingStopsVoiceChannel);
    RUN_TEST_CASE(sithVoice, TestHeadUpdateWaitsForGlobalDeadline);
    RUN_TEST_CASE(sithVoice, TestGlobalDeadlineIsSharedBySpeakers);
    RUN_TEST_CASE(sithVoice, TestExistingHeadStateIsSharedBySpeakers);
}

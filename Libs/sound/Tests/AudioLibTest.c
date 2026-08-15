#include <unity_fixture.h>

#include <limits.h>
#include <stdint.h>
#include <string.h>

#include <sound/AudioLib.h>
#include <std/General/stdUtil.h>

#define AUDIOLIB_TEST_HEADER_SIZE       8
#define AUDIOLIB_TEST_ENTRY_SIZE        4
#define AUDIOLIB_TEST_TIME_MASK         0xFFFF0000u
#define AUDIOLIB_TEST_MOUTH_MASK        0x7Fu
#define AUDIOLIB_TEST_TIME_UNITS_PER_SECOND 4096000u
#define AUDIOLIB_TEST_ANALYSIS_RATE     60u
#define AUDIOLIB_TEST_SAMPLE_RATE       8000
#define AUDIOLIB_TEST_FRAME_SIZE        (2 * AUDIOLIB_TEST_SAMPLE_RATE / (int)AUDIOLIB_TEST_ANALYSIS_RATE)
#define AUDIOLIB_TEST_OUTPUT_SIZE       8192
#define AUDIOLIB_TEST_MAX_PCM_SIZE      16000
#define AUDIOLIB_TEST_INPUT_PADDING     8

TEST_GROUP(AudioLib);

TEST_SETUP(AudioLib)
{
}

TEST_TEAR_DOWN(AudioLib)
{
}

static uint32_t AudioLibTest_ReadU32LE(const uint8_t* pData)
{
    return (uint32_t)pData[0]
        | ((uint32_t)pData[1] << 8)
        | ((uint32_t)pData[2] << 16)
        | ((uint32_t)pData[3] << 24);
}

static void AudioLibTest_WriteU32LE(uint8_t* pData, uint32_t value)
{
    pData[0] = (uint8_t)value;
    pData[1] = (uint8_t)(value >> 8);
    pData[2] = (uint8_t)(value >> 16);
    pData[3] = (uint8_t)(value >> 24);
}

static void AudioLibTest_WriteS16LE(uint8_t* pData, int16_t value)
{
    uint16_t bits = (uint16_t)value;

    pData[0] = (uint8_t)bits;
    pData[1] = (uint8_t)(bits >> 8);
}

static void AudioLibTest_InitBlock(uint8_t* pData, int numEntries)
{
    memcpy(pData, "SYNC", 4);
    AudioLibTest_WriteU32LE(pData + 4, (uint32_t)numEntries);
}

static void AudioLibTest_SetEntry(uint8_t* pData, int index, uint32_t timeKey, uint8_t mouthX, uint8_t mouthY)
{
    uint32_t entry = (timeKey & AUDIOLIB_TEST_TIME_MASK)
        | ((uint32_t)(mouthX & AUDIOLIB_TEST_MOUTH_MASK) << 8)
        | (mouthY & AUDIOLIB_TEST_MOUTH_MASK);

    AudioLibTest_WriteU32LE(pData + AUDIOLIB_TEST_HEADER_SIZE + index * AUDIOLIB_TEST_ENTRY_SIZE, entry);
}

static void AudioLibTest_AssertMouth(uint8_t* pData, int msecTime, uint8_t expectedX, uint8_t expectedY)
{
    uint8_t mouthX = UINT8_MAX;
    uint8_t mouthY = UINT8_MAX;

    TEST_ASSERT_EQUAL_INT(0, AudioLib_GetMouthPosition(pData, msecTime, &mouthX, &mouthY));
    TEST_ASSERT_EQUAL_UINT8(expectedX, mouthX);
    TEST_ASSERT_EQUAL_UINT8(expectedY, mouthY);
}

static void AudioLibTest_AssertGeneratedTimeline(uint8_t* pLipSyncData)
{
    uint32_t numEntries = AudioLibTest_ReadU32LE(pLipSyncData + 4);

    TEST_ASSERT_GREATER_THAN_UINT32(0u, numEntries);

    uint32_t finalEntry = AudioLibTest_ReadU32LE(pLipSyncData + AUDIOLIB_TEST_HEADER_SIZE + (numEntries - 1u) * AUDIOLIB_TEST_ENTRY_SIZE);
    int finalMsec       = (int)((finalEntry & AUDIOLIB_TEST_TIME_MASK) >> 12);

    for ( int msecTime = 0; msecTime <= finalMsec + 16; ++msecTime )
    {
        uint32_t targetTimeKey = ((uint32_t)msecTime << 12) & AUDIOLIB_TEST_TIME_MASK;
        uint32_t expectedIndex = 0;

        if ( targetTimeKey > (finalEntry & AUDIOLIB_TEST_TIME_MASK) )
        {
            expectedIndex = numEntries - 1u;
        }
        else
        {
            for ( uint32_t entryIndex = 1; entryIndex < numEntries; ++entryIndex )
            {
                uint32_t entry = AudioLibTest_ReadU32LE(pLipSyncData + AUDIOLIB_TEST_HEADER_SIZE + entryIndex * AUDIOLIB_TEST_ENTRY_SIZE);
                uint32_t timeKey = entry & AUDIOLIB_TEST_TIME_MASK;

                if ( targetTimeKey < timeKey )
                {
                    break;
                }

                if ( targetTimeKey == timeKey )
                {
                    // NOTE: Preserve the separately documented exact-final-timestamp bug.
                    expectedIndex = entryIndex + 1u == numEntries ? entryIndex - 1u : entryIndex;
                    break;
                }

                expectedIndex = entryIndex;
            }
        }

        uint32_t expectedEntry = AudioLibTest_ReadU32LE(pLipSyncData + AUDIOLIB_TEST_HEADER_SIZE + expectedIndex * AUDIOLIB_TEST_ENTRY_SIZE);

        AudioLibTest_AssertMouth(
            pLipSyncData,
            msecTime,
            (uint8_t)(expectedEntry >> 8) & AUDIOLIB_TEST_MOUTH_MASK,
            (uint8_t)expectedEntry & AUDIOLIB_TEST_MOUTH_MASK
        );
    }
}

static void AudioLibTest_FillPcm(uint8_t* pData, int sndDataSize, int pattern, uint32_t seed)
{
    int numSamples = sndDataSize / (int)sizeof(int16_t);

    for ( int i = 0; i < numSamples; ++i )
    {
        int16_t sample;

        switch ( pattern )
        {
            case 0:
                sample = 0;
                break;

            case 1:
                sample = 12000;
                break;

            case 2:
                sample = (i & 1) ? 15000 : -15000;
                break;

            case 3:
                seed = seed * 1664525u + 1013904223u;
                sample = (int16_t)(seed >> 16);
                break;

            case 4:
                sample = i % 17 == 0 ? 30000 : (i % 3 == 0 ? -500 : 0);
                break;

            default:
            {
                static const int16_t aBoundarySamples[] = {
                    INT16_MIN, INT16_MAX, -1, 0, 1, -20000, 20000, 0
                };

                sample = aBoundarySamples[i % (int)STD_ARRAYLEN(aBoundarySamples)];
                break;
            }
        }

        AudioLibTest_WriteS16LE(pData + i * (int)sizeof(int16_t), sample);
    }
}

static void AudioLibTest_FillTransientPcm(uint8_t* pData, int frameSize, int numFrames)
{
    int samplesPerFrame = frameSize / (int)sizeof(int16_t);

    for ( int frameIndex = 0; frameIndex < numFrames; ++frameIndex )
    {
        for ( int sampleIndex = 0; sampleIndex < samplesPerFrame; ++sampleIndex )
        {
            int16_t sample;

            if ( frameIndex == 0 )
            {
                sample = (sampleIndex & 1) ? 16000 : -16000;
            }
            else
            {
                sample = sampleIndex == 1 ? 16000 : 0;
            }

            AudioLibTest_WriteS16LE(pData + (frameIndex * samplesPerFrame + sampleIndex) * (int)sizeof(int16_t), sample);
        }
    }
}

static void AudioLibTest_FillConstantFrames(uint8_t* pData, int frameSize, const int16_t* aAmplitudes, int numFrames)
{
    int samplesPerFrame = frameSize / (int)sizeof(int16_t);

    for ( int frameIndex = 0; frameIndex < numFrames; ++frameIndex )
    {
        for ( int sampleIndex = 0; sampleIndex < samplesPerFrame; ++sampleIndex )
        {
            AudioLibTest_WriteS16LE(
                pData + (frameIndex * samplesPerFrame + sampleIndex) * (int)sizeof(int16_t),
                aAmplitudes[frameIndex]
            );
        }
    }
}

static void AudioLibTest_FillCrossingFrames(uint8_t* pData, int frameSize, int bAtFrameBoundary)
{
    int samplesPerFrame = frameSize / (int)sizeof(int16_t);

    for ( int frameIndex = 0; frameIndex < 2; ++frameIndex )
    {
        for ( int sampleIndex = 0; sampleIndex < samplesPerFrame; ++sampleIndex )
        {
            int16_t sample;

            if ( bAtFrameBoundary )
            {
                sample = frameIndex == 0 ? -16000 : 16000;
            }
            else
            {
                sample = sampleIndex < samplesPerFrame / 2 ? -16000 : 16000;
            }

            AudioLibTest_WriteS16LE(pData + (frameIndex * samplesPerFrame + sampleIndex) * (int)sizeof(int16_t), sample);
        }
    }
}

static void AudioLibTest_FillChangingFrames(uint8_t* pData, int frameSize, int numFrames)
{
    int samplesPerFrame = frameSize / (int)sizeof(int16_t);

    for ( int frameIndex = 0; frameIndex < numFrames; ++frameIndex )
    {
        for ( int sampleIndex = 0; sampleIndex < samplesPerFrame; ++sampleIndex )
        {
            int16_t sample = 0;

            if ( frameIndex & 1 )
            {
                sample = (sampleIndex & 1) ? 16000 : -16000;
            }

            AudioLibTest_WriteS16LE(pData + (frameIndex * samplesPerFrame + sampleIndex) * (int)sizeof(int16_t), sample);
        }
    }
}

TEST(AudioLib, TestMouthTimeline)
{
    uint8_t aLipSyncData[AUDIOLIB_TEST_HEADER_SIZE + 4 * AUDIOLIB_TEST_ENTRY_SIZE];

    AudioLibTest_InitBlock(aLipSyncData, 4);
    AudioLibTest_SetEntry(aLipSyncData, 0, 0x00000000u, 1, 2);
    AudioLibTest_SetEntry(aLipSyncData, 1, 0x00010000u, 3, 4);
    AudioLibTest_SetEntry(aLipSyncData, 2, 0x00030000u, 5, 6);
    AudioLibTest_SetEntry(aLipSyncData, 3, 0x00050000u, 7, 8);

    AudioLibTest_AssertMouth(aLipSyncData, 0, 1, 2);
    AudioLibTest_AssertMouth(aLipSyncData, 15, 1, 2);
    AudioLibTest_AssertMouth(aLipSyncData, 16, 3, 4);
    AudioLibTest_AssertMouth(aLipSyncData, 31, 3, 4);
    AudioLibTest_AssertMouth(aLipSyncData, 32, 3, 4);
    AudioLibTest_AssertMouth(aLipSyncData, 47, 3, 4);
    AudioLibTest_AssertMouth(aLipSyncData, 48, 5, 6);
    AudioLibTest_AssertMouth(aLipSyncData, 64, 5, 6);
    AudioLibTest_AssertMouth(aLipSyncData, 96, 7, 8);

    TEST_ASSERT_EQUAL_INT(0, AudioLib_GetMouthPosition(aLipSyncData, 48, NULL, NULL));
}

TEST(AudioLib, TestMouthBoundaries)
{
    uint8_t aSingleEntry[AUDIOLIB_TEST_HEADER_SIZE + AUDIOLIB_TEST_ENTRY_SIZE];
    uint8_t aTimeline[AUDIOLIB_TEST_HEADER_SIZE + 2 * AUDIOLIB_TEST_ENTRY_SIZE];

    AudioLibTest_InitBlock(aSingleEntry, 1);
    AudioLibTest_SetEntry(aSingleEntry, 0, 0, 12, 34);

    AudioLibTest_AssertMouth(aSingleEntry, 0, 12, 34);
    AudioLibTest_AssertMouth(aSingleEntry, 12345, 12, 34);

    uint8_t mouthX = UINT8_MAX;
    uint8_t mouthY = UINT8_MAX;

    TEST_ASSERT_EQUAL_INT(0, AudioLib_GetMouthPosition(aSingleEntry, 0, NULL, &mouthY));
    TEST_ASSERT_EQUAL_UINT8(34, mouthY);
    TEST_ASSERT_EQUAL_INT(0, AudioLib_GetMouthPosition(aSingleEntry, 0, &mouthX, NULL));
    TEST_ASSERT_EQUAL_UINT8(12, mouthX);

    AudioLibTest_InitBlock(aTimeline, 2);
    AudioLibTest_SetEntry(aTimeline, 0, 0x00000000u, 1, 2);
    AudioLibTest_SetEntry(aTimeline, 1, 0xFFFE0000u, 126, 127);

    // NOTE: 0xFFFFF is the highest millisecond value accepted by the original mask check.
    AudioLibTest_AssertMouth(aTimeline, 0x000FFFFF, 126, 127);
}

TEST(AudioLib, TestMouthSearchShapes)
{
    static const int aEntryCounts[] = { 3, 4, 5, 8, 17 };
    uint8_t aLipSyncData[AUDIOLIB_TEST_HEADER_SIZE + 17 * AUDIOLIB_TEST_ENTRY_SIZE];

    for ( size_t caseIndex = 0; caseIndex < STD_ARRAYLEN(aEntryCounts); ++caseIndex )
    {
        int numEntries = aEntryCounts[caseIndex];

        AudioLibTest_InitBlock(aLipSyncData, numEntries);
        for ( int entryIndex = 0; entryIndex < numEntries; ++entryIndex )
        {
            AudioLibTest_SetEntry(
                aLipSyncData,
                entryIndex,
                (uint32_t)entryIndex * 0x00020000u,
                (uint8_t)(entryIndex + 1),
                (uint8_t)(127 - entryIndex)
            );
        }

        int middleIndex    = (numEntries - 1) / 2;
        int penultimateIdx = numEntries - 2;

        AudioLibTest_AssertMouth(aLipSyncData, 0, 1, 127);
        AudioLibTest_AssertMouth(aLipSyncData, middleIndex * 32, (uint8_t)(middleIndex + 1), (uint8_t)(127 - middleIndex));
        AudioLibTest_AssertMouth(aLipSyncData, middleIndex * 32 + 16, (uint8_t)(middleIndex + 1), (uint8_t)(127 - middleIndex));
        AudioLibTest_AssertMouth(aLipSyncData, penultimateIdx * 32, (uint8_t)(penultimateIdx + 1), (uint8_t)(127 - penultimateIdx));
        AudioLibTest_AssertMouth(aLipSyncData, numEntries * 32, (uint8_t)numEntries, (uint8_t)(128 - numEntries));
    }

    AudioLibTest_InitBlock(aLipSyncData, 1);
    AudioLibTest_WriteU32LE(aLipSyncData + AUDIOLIB_TEST_HEADER_SIZE, 0x0000FFFFu);

    // NOTE: Bit 7 in each packed mouth byte is reserved and must not reach the caller.
    AudioLibTest_AssertMouth(aLipSyncData, 0, 127, 127);
}

TEST(AudioLib, TestFinalTimestampBug)
{
    uint8_t aLipSyncData[AUDIOLIB_TEST_HEADER_SIZE + 2 * AUDIOLIB_TEST_ENTRY_SIZE];

    AudioLibTest_InitBlock(aLipSyncData, 2);
    AudioLibTest_SetEntry(aLipSyncData, 0, 0x00000000u, 1, 2);
    AudioLibTest_SetEntry(aLipSyncData, 1, 0x00010000u, 3, 4);

    // NOTE: The original adjacent-range exit selects the preceding entry at the final timestamp.
    AudioLibTest_AssertMouth(aLipSyncData, 16, 1, 2);
    AudioLibTest_AssertMouth(aLipSyncData, 32, 3, 4);
}

TEST(AudioLib, TestZeroEntryBug)
{
    uint8_t aLipSyncData[AUDIOLIB_TEST_HEADER_SIZE + AUDIOLIB_TEST_ENTRY_SIZE];

    AudioLibTest_InitBlock(aLipSyncData, 0);
    AudioLibTest_SetEntry(aLipSyncData, 0, 0, 12, 34);

    // NOTE: The original accepts a zero count and reads a phantom entry after the header at time zero.
    AudioLibTest_AssertMouth(aLipSyncData, 0, 12, 34);

    // NOTE: A later lookup first reads the zero count as the entry before the array.
    AudioLibTest_AssertMouth(aLipSyncData, 16, 0, 0);
}

TEST(AudioLib, TestMouthRejections)
{
    uint8_t aLipSyncData[AUDIOLIB_TEST_HEADER_SIZE + AUDIOLIB_TEST_ENTRY_SIZE];
    uint8_t mouthX = UINT8_MAX;
    uint8_t mouthY = UINT8_MAX;

    AudioLibTest_InitBlock(aLipSyncData, 1);
    AudioLibTest_SetEntry(aLipSyncData, 0, 0, 12, 34);
    aLipSyncData[0] = 'X';

    TEST_ASSERT_EQUAL_INT(1, AudioLib_GetMouthPosition(aLipSyncData, 0, &mouthX, &mouthY));
    TEST_ASSERT_EQUAL_UINT8(0, mouthX);
    TEST_ASSERT_EQUAL_UINT8(0, mouthY);

    aLipSyncData[0] = 'S';
    mouthX = UINT8_MAX;
    mouthY = UINT8_MAX;
    TEST_ASSERT_EQUAL_INT(1, AudioLib_GetMouthPosition(aLipSyncData, 0x00100000, &mouthX, &mouthY));
    TEST_ASSERT_EQUAL_UINT8(0, mouthX);
    TEST_ASSERT_EQUAL_UINT8(0, mouthY);

    mouthX = UINT8_MAX;
    mouthY = UINT8_MAX;
    TEST_ASSERT_EQUAL_INT(1, AudioLib_GetMouthPosition(aLipSyncData, -1, &mouthX, &mouthY));
    TEST_ASSERT_EQUAL_UINT8(0, mouthX);
    TEST_ASSERT_EQUAL_UINT8(0, mouthY);
}

TEST(AudioLib, TestGenerateRejections)
{
    static const uint32_t aInvalidBitDepths[] = { UINT32_MAX, 0u, 8u, 15u, 24u, 32u };
    static const uint32_t aInvalidChannels[]  = { UINT32_MAX, 0u, 2u, 4u };
    uint8_t aOutData[32];
    uint8_t aSndData[8] = { 0 };
    uint8_t aExpected[STD_ARRAYLEN(aOutData)];

    memset(aExpected, 0xA5, sizeof(aExpected));

    memcpy(aOutData, aExpected, sizeof(aOutData));
    TEST_ASSERT_EQUAL_INT(0, AudioLib_GenerateLipSyncBlock(aOutData, aSndData, 60, 4, 4, 8000, 16, 1, 0, 0));
    TEST_ASSERT_EQUAL_MEMORY(aExpected, aOutData, sizeof(aOutData));

    memcpy(aOutData, aExpected, sizeof(aOutData));
    TEST_ASSERT_EQUAL_INT(0, AudioLib_GenerateLipSyncBlock(aOutData, NULL, 60, 4, 4, 8000, 16, 1, 0, sizeof(aSndData)));
    TEST_ASSERT_EQUAL_MEMORY(aExpected, aOutData, sizeof(aOutData));

    for ( size_t caseIndex = 0; caseIndex < STD_ARRAYLEN(aInvalidBitDepths); ++caseIndex )
    {
        memcpy(aOutData, aExpected, sizeof(aOutData));
        TEST_ASSERT_EQUAL_INT(
            0,
            AudioLib_GenerateLipSyncBlock(aOutData, aSndData, 60, 4, 4, 8000, aInvalidBitDepths[caseIndex], 1, 0, sizeof(aSndData))
        );
        TEST_ASSERT_EQUAL_MEMORY(aExpected, aOutData, sizeof(aOutData));
    }

    for ( size_t caseIndex = 0; caseIndex < STD_ARRAYLEN(aInvalidChannels); ++caseIndex )
    {
        memcpy(aOutData, aExpected, sizeof(aOutData));
        TEST_ASSERT_EQUAL_INT(
            0,
            AudioLib_GenerateLipSyncBlock(aOutData, aSndData, 60, 4, 4, 8000, 16, aInvalidChannels[caseIndex], 0, sizeof(aSndData))
        );
        TEST_ASSERT_EQUAL_MEMORY(aExpected, aOutData, sizeof(aOutData));
    }
}

TEST(AudioLib, TestGenerateCapacity)
{
    uint8_t aSndData[1600 + AUDIOLIB_TEST_INPUT_PADDING] = { 0 };
    uint8_t aOutData[64];

    size_t maxBlockSize = AudioLib_GetLipSyncBlockMaxSize(60u, 8000, 16, 1, 1600);

    TEST_ASSERT_EQUAL_size_t(36u, maxBlockSize);
    TEST_ASSERT_EQUAL_size_t(0u, AudioLib_GetLipSyncBlockMaxSize(0, 8000, 16, 1, 1600));
    TEST_ASSERT_EQUAL_size_t(0u, AudioLib_GetLipSyncBlockMaxSize(60, 0, 16, 1, 1600));
    TEST_ASSERT_EQUAL_size_t(0u, AudioLib_GetLipSyncBlockMaxSize(60, 8000, 8, 1, 1600));
    TEST_ASSERT_EQUAL_size_t(0u, AudioLib_GetLipSyncBlockMaxSize(60, 8000, 16, 2, 1600));
    TEST_ASSERT_EQUAL_size_t(0u, AudioLib_GetLipSyncBlockMaxSize(60, 8000, 16, 1, 0));
    TEST_ASSERT_EQUAL_size_t(0u, AudioLib_GetLipSyncBlockMaxSize(60, 8000, 16, 1, 1599));
    TEST_ASSERT_EQUAL_size_t(0u, AudioLib_GetLipSyncBlockMaxSize(4096001u, 8000, 16, 1, 1600));
    TEST_ASSERT_EQUAL_size_t(0u, AudioLib_GetLipSyncBlockMaxSize(16001u, 8000, 16, 1, 1600));
    TEST_ASSERT_EQUAL_size_t(0u, AudioLib_GetLipSyncBlockMaxSize(60, INT_MAX, 16, 1, 1600));
    TEST_ASSERT_EQUAL_size_t(0u, AudioLib_GetLipSyncBlockMaxSize(60, UINT32_MAX, 16, 1, 1600));
    TEST_ASSERT_EQUAL_size_t(0u, AudioLib_GetLipSyncBlockMaxSize(60, 8000, 16, 1, INT_MAX - 1));
    TEST_ASSERT_EQUAL_size_t(0u, AudioLib_GetLipSyncBlockMaxSize(60, 8000, 16, 1, UINT32_MAX));

    size_t largeBlockSize = AudioLib_GetLipSyncBlockMaxSize(
        AUDIOLIB_TEST_ANALYSIS_RATE,
        AUDIOLIB_TEST_SAMPLE_RATE,
        16,
        1,
        2048 * AUDIOLIB_TEST_FRAME_SIZE
    );

    TEST_ASSERT_GREATER_THAN_size_t(8192u, largeBlockSize);

    memset(aOutData, 0xA5, sizeof(aOutData));
    TEST_ASSERT_EQUAL_INT(0, AudioLib_GenerateLipSyncBlockEx(aOutData, maxBlockSize - 1u, aSndData, sizeof(aSndData), 60, 4, 4, 8000, 16, 1, 0, 1600));
    TEST_ASSERT_EACH_EQUAL_UINT8(0xA5u, aOutData, sizeof(aOutData));

    TEST_ASSERT_EQUAL_INT(0, AudioLib_GenerateLipSyncBlockEx(NULL, maxBlockSize, aSndData, sizeof(aSndData), 60, 4, 4, 8000, 16, 1, 0, 1600));
    TEST_ASSERT_EQUAL_INT(0, AudioLib_GenerateLipSyncBlockEx(aOutData, maxBlockSize, NULL, sizeof(aSndData), 60, 4, 4, 8000, 16, 1, 0, 1600));
    TEST_ASSERT_EQUAL_INT(0, AudioLib_GenerateLipSyncBlockEx(aOutData, maxBlockSize, aSndData, 1599, 60, 4, 4, 8000, 16, 1, 0, 1600));
    TEST_ASSERT_EQUAL_INT(0, AudioLib_GenerateLipSyncBlockEx(aOutData, maxBlockSize, aSndData, 1600, 60, 4, 4, 8000, 16, 1, 1, 1600));

    size_t oddFrameBlockSize = AudioLib_GetLipSyncBlockMaxSize(60, 16000, 16, 1, 1066);
    TEST_ASSERT_GREATER_THAN_size_t(0u, oddFrameBlockSize);
    TEST_ASSERT_EQUAL_INT(0, AudioLib_GenerateLipSyncBlockEx(aOutData, sizeof(aOutData), aSndData, 1066, 60, 4, 4, 16000, 16, 1, 0, 1066));
    TEST_ASSERT_GREATER_THAN_INT(0, AudioLib_GenerateLipSyncBlockEx(aOutData, sizeof(aOutData), aSndData, 1067, 60, 4, 4, 16000, 16, 1, 0, 1066));
    TEST_ASSERT_EQUAL_INT(0, AudioLib_GenerateLipSyncBlockEx(aOutData, sizeof(aOutData), aSndData, 1067, 60, 4, 4, 16000, 16, 1, 1, 1066));
    TEST_ASSERT_GREATER_THAN_INT(0, AudioLib_GenerateLipSyncBlockEx(aOutData, sizeof(aOutData), aSndData, 1068, 60, 4, 4, 16000, 16, 1, 1, 1066));

    int result = AudioLib_GenerateLipSyncBlockEx(aOutData, maxBlockSize, aSndData, sizeof(aSndData), 60, 4, 4, 8000, 16, 1, 0, 1600);

    TEST_ASSERT_EQUAL_INT(16, result);
    TEST_ASSERT_EACH_EQUAL_UINT8(0xA5u, aOutData + maxBlockSize, sizeof(aOutData) - maxBlockSize);
}

TEST(AudioLib, TestGenerateSilence)
{
    static const uint8_t aExpected[] = {
        'S', 'Y', 'N', 'C', 0x02, 0x00, 0x00, 0x00,
        0x00, 0x20, 0x00, 0x00,
        0x00, 0x00, 0x06, 0x00
    };
    uint8_t aSndData[1600] = { 0 };
    uint8_t aOutData[64];

    memset(aOutData, 0xA5, sizeof(aOutData));

    int result = AudioLib_GenerateLipSyncBlock(aOutData, aSndData, 60, 4, 4, 8000, 16, 1, 0, sizeof(aSndData));

    TEST_ASSERT_EQUAL_INT((int)sizeof(aExpected), result);
    TEST_ASSERT_EQUAL_HEX8_ARRAY(aExpected, aOutData, sizeof(aExpected));
    TEST_ASSERT_EACH_EQUAL_UINT8(0xA5u, aOutData + result, sizeof(aOutData) - (size_t)result);
}

TEST(AudioLib, TestGenerateWaveform)
{
    static const uint8_t aExpected[] = {
        'S', 'Y', 'N', 'C', 0x04, 0x00, 0x00, 0x00,
        0x40, 0x20, 0x00, 0x00,
        0x40, 0x40, 0x01, 0x00,
        0x40, 0x20, 0x02, 0x00,
        0x00, 0x00, 0x06, 0x00
    };
    uint8_t aSndData[1600];
    uint8_t aOutData[64];

    AudioLibTest_FillPcm(aSndData, sizeof(aSndData), 3, 0x9E3779B9u);
    memset(aOutData, 0xA5, sizeof(aOutData));

    int result = AudioLib_GenerateLipSyncBlock(aOutData, aSndData, 60, 4, 4, 8000, 16, 1, 0, sizeof(aSndData));

    TEST_ASSERT_EQUAL_INT((int)sizeof(aExpected), result);
    TEST_ASSERT_EQUAL_HEX8_ARRAY(aExpected, aOutData, sizeof(aExpected));
    TEST_ASSERT_EACH_EQUAL_UINT8(0xA5u, aOutData + result, sizeof(aOutData) - (size_t)result);
}

TEST(AudioLib, TestOffsetAndQuantization)
{
    static const struct
    {
        int8_t mouthXLevels;
        int8_t mouthYLevels;
        uint8_t mouthXMask;
        uint8_t mouthYMask;
    } aQuantizationCases[] = {
        { 1, 16, 0x00, 0x78 },
        { 2,  8, 0x40, 0x70 },
        { 4,  4, 0x60, 0x60 },
        { 8,  2, 0x70, 0x40 },
        { 16, 1, 0x78, 0x00 },
        { 32, 64, 0x7C, 0x7E },
        { 64, 32, 0x7E, 0x7C }
    };
    uint8_t aAligned[AUDIOLIB_TEST_MAX_PCM_SIZE + AUDIOLIB_TEST_INPUT_PADDING];
    uint8_t aOffset[AUDIOLIB_TEST_MAX_PCM_SIZE + AUDIOLIB_TEST_INPUT_PADDING];
    uint8_t aAlignedOut[AUDIOLIB_TEST_OUTPUT_SIZE];
    uint8_t aOffsetOut[AUDIOLIB_TEST_OUTPUT_SIZE];
    const int sndDataSize = 3200;

    AudioLibTest_FillPcm(aAligned, sndDataSize, 3, 0x12345678u);
    aOffset[0] = 0xA5;
    memcpy(aOffset + 1, aAligned, sndDataSize);

    int alignedSize = AudioLib_GenerateLipSyncBlock(aAlignedOut, aAligned, 60, 4, 4, 16000, 16, 1, 0, sndDataSize);
    int offsetSize  = AudioLib_GenerateLipSyncBlock(aOffsetOut, aOffset, 60, 4, 4, 16000, 16, 1, 1, sndDataSize);

    TEST_ASSERT_EQUAL_INT(alignedSize, offsetSize);
    TEST_ASSERT_EQUAL_MEMORY(aAlignedOut, aOffsetOut, (size_t)alignedSize);

    for ( size_t caseIndex = 0; caseIndex < STD_ARRAYLEN(aQuantizationCases); ++caseIndex )
    {
        int blockSize = AudioLib_GenerateLipSyncBlock(
            aAlignedOut,
            aAligned,
            60,
            aQuantizationCases[caseIndex].mouthXLevels,
            aQuantizationCases[caseIndex].mouthYLevels,
            16000,
            16,
            1,
            0,
            sndDataSize
        );
        uint32_t numEntries = AudioLibTest_ReadU32LE(aAlignedOut + 4);

        TEST_ASSERT_EQUAL_INT(AUDIOLIB_TEST_HEADER_SIZE + AUDIOLIB_TEST_ENTRY_SIZE * (int)numEntries, blockSize);
        for ( uint32_t entryIndex = 0; entryIndex < numEntries; ++entryIndex )
        {
            uint32_t entry = AudioLibTest_ReadU32LE(aAlignedOut + AUDIOLIB_TEST_HEADER_SIZE + entryIndex * AUDIOLIB_TEST_ENTRY_SIZE);
            uint8_t mouthX = (uint8_t)(entry >> 8) & AUDIOLIB_TEST_MOUTH_MASK;
            uint8_t mouthY = (uint8_t)entry & AUDIOLIB_TEST_MOUTH_MASK;

            TEST_ASSERT_EQUAL_UINT8(0, mouthX & (uint8_t)~aQuantizationCases[caseIndex].mouthXMask);
            TEST_ASSERT_EQUAL_UINT8(0, mouthY & (uint8_t)~aQuantizationCases[caseIndex].mouthYMask);
        }
    }

    AudioLibTest_FillPcm(aOffsetOut, sndDataSize, 3, 0x12345678u);
    TEST_ASSERT_EQUAL_MEMORY(aOffsetOut, aAligned, sndDataSize);
    TEST_ASSERT_EQUAL_UINT8(0xA5u, aOffset[0]);
    TEST_ASSERT_EQUAL_MEMORY(aOffsetOut, aOffset + 1, sndDataSize);
}

TEST(AudioLib, TestGenerateRates)
{
    static const struct
    {
        unsigned int analysisRateHz;
        int sampleRate;
        int sndDataSize;
    } aRateCases[] = {
        { 1, 8000, 16000 },
        { 30, 11025, 2204 },
        { 60, 22050, 4410 },
        { 75, 16000, 3200 },
        { 100, 44100, 8820 },
        { 120, 8000, 2048 },
        { 8000, 8000, 16 }
    };
    uint8_t aSndData[AUDIOLIB_TEST_MAX_PCM_SIZE + AUDIOLIB_TEST_INPUT_PADDING];
    uint8_t aFirstOut[AUDIOLIB_TEST_OUTPUT_SIZE];
    uint8_t aSecondOut[AUDIOLIB_TEST_OUTPUT_SIZE];

    for ( size_t caseIndex = 0; caseIndex < STD_ARRAYLEN(aRateCases); ++caseIndex )
    {
        const unsigned int analysisRateHz = aRateCases[caseIndex].analysisRateHz;
        const int sampleRate              = aRateCases[caseIndex].sampleRate;
        const int sndDataSize             = aRateCases[caseIndex].sndDataSize;

        // NOTE: Padding makes the original odd-frame lookahead deterministic.
        memset(aSndData, 0xA5, sizeof(aSndData));
        AudioLibTest_FillPcm(aSndData, sndDataSize, (int)(caseIndex % 5), 0xC001D00Du + (uint32_t)caseIndex);
        memset(aFirstOut, 0xA5, sizeof(aFirstOut));
        memset(aSecondOut, 0x5A, sizeof(aSecondOut));

        int firstSize  = AudioLib_GenerateLipSyncBlock(aFirstOut, aSndData, analysisRateHz, 2, 8, sampleRate, 16, 1, 0, sndDataSize);
        int secondSize = AudioLib_GenerateLipSyncBlock(aSecondOut, aSndData, analysisRateHz, 2, 8, sampleRate, 16, 1, 0, sndDataSize);

        TEST_ASSERT_EQUAL_INT(firstSize, secondSize);
        TEST_ASSERT_GREATER_THAN_INT(AUDIOLIB_TEST_HEADER_SIZE, firstSize);
        TEST_ASSERT_EQUAL_MEMORY(aFirstOut, aSecondOut, (size_t)firstSize);
        TEST_ASSERT_EQUAL_MEMORY("SYNC", aFirstOut, 4);

        uint32_t numEntries = AudioLibTest_ReadU32LE(aFirstOut + 4);
        int bytesPerFrame   = (int)(2u * (unsigned int)sampleRate / analysisRateHz);
        uint32_t numFrames  = (uint32_t)(sndDataSize / bytesPerFrame);
        uint32_t timeStep   = 4096000u / analysisRateHz;
        uint32_t finalEntry = AudioLibTest_ReadU32LE(aFirstOut + firstSize - AUDIOLIB_TEST_ENTRY_SIZE);

        TEST_ASSERT_EQUAL_INT(AUDIOLIB_TEST_HEADER_SIZE + AUDIOLIB_TEST_ENTRY_SIZE * (int)numEntries, firstSize);
        TEST_ASSERT_EQUAL_HEX32((numFrames * timeStep) & AUDIOLIB_TEST_TIME_MASK, finalEntry);
        TEST_ASSERT_EACH_EQUAL_UINT8(0xA5u, aFirstOut + firstSize, sizeof(aFirstOut) - (size_t)firstSize);
        TEST_ASSERT_EACH_EQUAL_UINT8(0x5Au, aSecondOut + secondSize, sizeof(aSecondOut) - (size_t)secondSize);

        uint32_t previousTimeKey = 0;
        for ( uint32_t entryIndex = 0; entryIndex < numEntries; ++entryIndex )
        {
            uint32_t entry   = AudioLibTest_ReadU32LE(aFirstOut + AUDIOLIB_TEST_HEADER_SIZE + entryIndex * AUDIOLIB_TEST_ENTRY_SIZE);
            uint32_t timeKey = entry & AUDIOLIB_TEST_TIME_MASK;

            TEST_ASSERT_GREATER_OR_EQUAL_HEX32(previousTimeKey, timeKey);
            previousTimeKey = timeKey;
        }
    }
}

TEST(AudioLib, TestGenerateFrameBoundaries)
{
    static const struct
    {
        int sndDataSize;
        uint32_t numFrames;
    } aBoundaryCases[] = {
        { 2, 0 },
        { AUDIOLIB_TEST_FRAME_SIZE - 2, 0 },
        { AUDIOLIB_TEST_FRAME_SIZE, 1 },
        { AUDIOLIB_TEST_FRAME_SIZE + 2, 1 },
        { 2 * AUDIOLIB_TEST_FRAME_SIZE, 2 },
        { 2 * AUDIOLIB_TEST_FRAME_SIZE + 2, 2 }
    };
    uint8_t aSndData[2 * AUDIOLIB_TEST_FRAME_SIZE + 2] = { 0 };
    uint8_t aOutData[64];

    for ( size_t caseIndex = 0; caseIndex < STD_ARRAYLEN(aBoundaryCases); ++caseIndex )
    {
        memset(aOutData, 0xA5, sizeof(aOutData));

        int result = AudioLib_GenerateLipSyncBlock(
            aOutData,
            aSndData,
            AUDIOLIB_TEST_ANALYSIS_RATE,
            4,
            4,
            AUDIOLIB_TEST_SAMPLE_RATE,
            16,
            1,
            0,
            aBoundaryCases[caseIndex].sndDataSize
        );
        uint32_t expectedEntries = aBoundaryCases[caseIndex].numFrames == 0 ? 1u : 2u;
        uint32_t numEntries      = AudioLibTest_ReadU32LE(aOutData + 4);
        uint32_t finalEntry      = AudioLibTest_ReadU32LE(aOutData + result - AUDIOLIB_TEST_ENTRY_SIZE);
        uint32_t expectedTime    = aBoundaryCases[caseIndex].numFrames
            * (AUDIOLIB_TEST_TIME_UNITS_PER_SECOND / AUDIOLIB_TEST_ANALYSIS_RATE);

        TEST_ASSERT_EQUAL_UINT32(expectedEntries, numEntries);
        TEST_ASSERT_EQUAL_INT(AUDIOLIB_TEST_HEADER_SIZE + (int)expectedEntries * AUDIOLIB_TEST_ENTRY_SIZE, result);
        TEST_ASSERT_EQUAL_HEX32(expectedTime & AUDIOLIB_TEST_TIME_MASK, finalEntry);
        TEST_ASSERT_EACH_EQUAL_UINT8(0xA5u, aOutData + result, sizeof(aOutData) - (size_t)result);

        if ( aBoundaryCases[caseIndex].numFrames != 0 )
        {
            TEST_ASSERT_EQUAL_HEX32(0x00002000u, AudioLibTest_ReadU32LE(aOutData + AUDIOLIB_TEST_HEADER_SIZE));
        }
    }
}

TEST(AudioLib, TestRollingWindowWrap)
{
    static const uint8_t aExpected[] = {
        'S', 'Y', 'N', 'C', 0x0A, 0x00, 0x00, 0x00,
        0x40, 0x20, 0x00, 0x00,
        0x40, 0x40, 0x04, 0x00,
        0x40, 0x20, 0x06, 0x00,
        0x40, 0x40, 0x07, 0x00,
        0x40, 0x20, 0x08, 0x00,
        0x40, 0x40, 0x09, 0x00,
        0x40, 0x20, 0x0A, 0x00,
        0x40, 0x40, 0x0F, 0x00,
        0x40, 0x20, 0x10, 0x00,
        0x00, 0x00, 0x12, 0x00
    };
    uint8_t aSndData[18 * AUDIOLIB_TEST_FRAME_SIZE];
    uint8_t aOutData[64];

    AudioLibTest_FillPcm(aSndData, sizeof(aSndData), 3, 0x31415926u);
    memset(aOutData, 0xA5, sizeof(aOutData));

    int result = AudioLib_GenerateLipSyncBlock(aOutData, aSndData, AUDIOLIB_TEST_ANALYSIS_RATE, 4, 4, AUDIOLIB_TEST_SAMPLE_RATE, 16, 1, 0, sizeof(aSndData));

    // NOTE: Eighteen frames force the original 15-slot rolling maximum index to wrap.
    TEST_ASSERT_EQUAL_INT((int)sizeof(aExpected), result);
    TEST_ASSERT_EQUAL_HEX8_ARRAY(aExpected, aOutData, sizeof(aExpected));
    TEST_ASSERT_EACH_EQUAL_UINT8(0xA5u, aOutData + result, sizeof(aOutData) - (size_t)result);
}

TEST(AudioLib, TestTransientDecay)
{
    static const uint8_t aExpected[] = {
        'S', 'Y', 'N', 'C', 0x07, 0x00, 0x00, 0x00,
        0x40, 0x60, 0x00, 0x00,
        0x20, 0x60, 0x01, 0x00,
        0x20, 0x40, 0x02, 0x00,
        0x00, 0x40, 0x03, 0x00,
        0x00, 0x20, 0x04, 0x00,
        0x00, 0x00, 0x06, 0x00,
        0x00, 0x00, 0x0A, 0x00
    };
    uint8_t aSndData[10 * AUDIOLIB_TEST_FRAME_SIZE];
    uint8_t aOutData[64];

    AudioLibTest_FillTransientPcm(aSndData, AUDIOLIB_TEST_FRAME_SIZE, 10);
    memset(aOutData, 0xA5, sizeof(aOutData));

    int result = AudioLib_GenerateLipSyncBlock(aOutData, aSndData, AUDIOLIB_TEST_ANALYSIS_RATE, 4, 4, AUDIOLIB_TEST_SAMPLE_RATE, 16, 1, 0, sizeof(aSndData));

    // NOTE: A sustained frame followed by sparse impulses exercises both decay thresholds.
    TEST_ASSERT_EQUAL_INT((int)sizeof(aExpected), result);
    TEST_ASSERT_EQUAL_HEX8_ARRAY(aExpected, aOutData, sizeof(aExpected));
    TEST_ASSERT_EACH_EQUAL_UINT8(0xA5u, aOutData + result, sizeof(aOutData) - (size_t)result);
}

TEST(AudioLib, TestSignalClamp)
{
    static const int16_t aAmplitudes[] = {
        1000, 1000, 1000, 1000, 1000,
        1000, 1000, 1000, 1000, 1000,
        1000, 1000, 1000, 1000, 1000,
        5000, 30000
    };
    static const uint8_t aExpected[] = {
        'S', 'Y', 'N', 'C', 0x09, 0x00, 0x00, 0x00,
        0x02, 0x2A, 0x00, 0x00,
        0x02, 0x28, 0x06, 0x00,
        0x04, 0x26, 0x08, 0x00,
        0x06, 0x24, 0x0A, 0x00,
        0x06, 0x22, 0x0B, 0x00,
        0x0A, 0x1E, 0x0C, 0x00,
        0x32, 0x00, 0x0F, 0x00,
        0x7E, 0x00, 0x10, 0x00,
        0x00, 0x00, 0x11, 0x00
    };
    uint8_t aSndData[STD_ARRAYLEN(aAmplitudes) * AUDIOLIB_TEST_FRAME_SIZE];
    uint8_t aOutData[64];

    AudioLibTest_FillConstantFrames(aSndData, AUDIOLIB_TEST_FRAME_SIZE, aAmplitudes, STD_ARRAYLEN(aAmplitudes));
    memset(aOutData, 0xA5, sizeof(aOutData));

    int result = AudioLib_GenerateLipSyncBlock(aOutData, aSndData, AUDIOLIB_TEST_ANALYSIS_RATE, 64, 64, AUDIOLIB_TEST_SAMPLE_RATE, 16, 1, 0, sizeof(aSndData));

    // NOTE: The last two frames separately exercise the rolling floor and mouth-Y saturation.
    TEST_ASSERT_EQUAL_INT((int)sizeof(aExpected), result);
    TEST_ASSERT_EQUAL_HEX8_ARRAY(aExpected, aOutData, sizeof(aExpected));
    TEST_ASSERT_EACH_EQUAL_UINT8(0xA5u, aOutData + result, sizeof(aOutData) - (size_t)result);
}

TEST(AudioLib, TestZeroCrossings)
{
    static const uint8_t aInternalExpected[] = {
        'S', 'Y', 'N', 'C', 0x02, 0x00, 0x00, 0x00,
        0x4C, 0x1E, 0x00, 0x00,
        0x00, 0x00, 0x02, 0x00
    };
    static const uint8_t aBoundaryExpected[] = {
        'S', 'Y', 'N', 'C', 0x02, 0x00, 0x00, 0x00,
        0x4C, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x02, 0x00
    };
    uint8_t aSndData[2 * AUDIOLIB_TEST_FRAME_SIZE];
    uint8_t aOutData[64];

    AudioLibTest_FillCrossingFrames(aSndData, AUDIOLIB_TEST_FRAME_SIZE, 0);
    memset(aOutData, 0xA5, sizeof(aOutData));

    int result = AudioLib_GenerateLipSyncBlock(aOutData, aSndData, AUDIOLIB_TEST_ANALYSIS_RATE, 64, 64, AUDIOLIB_TEST_SAMPLE_RATE, 16, 1, 0, sizeof(aSndData));

    TEST_ASSERT_EQUAL_INT((int)sizeof(aInternalExpected), result);
    TEST_ASSERT_EQUAL_HEX8_ARRAY(aInternalExpected, aOutData, sizeof(aInternalExpected));
    TEST_ASSERT_EACH_EQUAL_UINT8(0xA5u, aOutData + result, sizeof(aOutData) - (size_t)result);

    AudioLibTest_FillCrossingFrames(aSndData, AUDIOLIB_TEST_FRAME_SIZE, 1);
    memset(aOutData, 0xA5, sizeof(aOutData));

    result = AudioLib_GenerateLipSyncBlock(aOutData, aSndData, AUDIOLIB_TEST_ANALYSIS_RATE, 64, 64, AUDIOLIB_TEST_SAMPLE_RATE, 16, 1, 0, sizeof(aSndData));

    // NOTE: The original frame analysis does not count a crossing between adjacent frames.
    TEST_ASSERT_EQUAL_INT((int)sizeof(aBoundaryExpected), result);
    TEST_ASSERT_EQUAL_HEX8_ARRAY(aBoundaryExpected, aOutData, sizeof(aBoundaryExpected));
    TEST_ASSERT_EACH_EQUAL_UINT8(0xA5u, aOutData + result, sizeof(aOutData) - (size_t)result);
}

TEST(AudioLib, TestFrameFirstSample)
{
    static const uint8_t aFirstExpected[] = {
        'S', 'Y', 'N', 'C', 0x02, 0x00, 0x00, 0x00,
        0x00, 0x2E, 0x00, 0x00,
        0x00, 0x00, 0x02, 0x00
    };
    static const uint8_t aSecondExpected[] = {
        'S', 'Y', 'N', 'C', 0x03, 0x00, 0x00, 0x00,
        0x00, 0x2E, 0x00, 0x00,
        0x00, 0x00, 0x01, 0x00,
        0x00, 0x00, 0x02, 0x00
    };
    uint8_t aSndData[2 * AUDIOLIB_TEST_FRAME_SIZE];
    uint8_t aOutData[64];

    STD_ZEROMEM(aSndData, sizeof(aSndData));
    AudioLibTest_WriteS16LE(aSndData + AUDIOLIB_TEST_FRAME_SIZE, 30000);
    memset(aOutData, 0xA5, sizeof(aOutData));

    int result = AudioLib_GenerateLipSyncBlock(aOutData, aSndData, AUDIOLIB_TEST_ANALYSIS_RATE, 64, 64, AUDIOLIB_TEST_SAMPLE_RATE, 16, 1, 0, sizeof(aSndData));

    TEST_ASSERT_EQUAL_INT((int)sizeof(aFirstExpected), result);
    TEST_ASSERT_EQUAL_HEX8_ARRAY(aFirstExpected, aOutData, sizeof(aFirstExpected));

    STD_ZEROMEM(aSndData, sizeof(aSndData));
    AudioLibTest_WriteS16LE(aSndData + AUDIOLIB_TEST_FRAME_SIZE + (int)sizeof(int16_t), 30000);
    memset(aOutData, 0xA5, sizeof(aOutData));

    result = AudioLib_GenerateLipSyncBlock(aOutData, aSndData, AUDIOLIB_TEST_ANALYSIS_RATE, 64, 64, AUDIOLIB_TEST_SAMPLE_RATE, 16, 1, 0, sizeof(aSndData));

    // NOTE: Each frame's first sample is excluded, while its second sample participates in analysis.
    TEST_ASSERT_EQUAL_INT((int)sizeof(aSecondExpected), result);
    TEST_ASSERT_EQUAL_HEX8_ARRAY(aSecondExpected, aOutData, sizeof(aSecondExpected));
    TEST_ASSERT_EACH_EQUAL_UINT8(0xA5u, aOutData + result, sizeof(aOutData) - (size_t)result);
}

TEST(AudioLib, TestOutputGrowth)
{
    enum { NUM_FRAMES = 20 };
    uint8_t aSndData[NUM_FRAMES * AUDIOLIB_TEST_FRAME_SIZE];
    uint8_t aOutData[128];

    AudioLibTest_FillChangingFrames(aSndData, AUDIOLIB_TEST_FRAME_SIZE, NUM_FRAMES);
    memset(aOutData, 0xA5, sizeof(aOutData));

    int result          = AudioLib_GenerateLipSyncBlock(aOutData, aSndData, AUDIOLIB_TEST_ANALYSIS_RATE, 64, 64, AUDIOLIB_TEST_SAMPLE_RATE, 16, 1, 0, sizeof(aSndData));
    uint32_t numEntries = AudioLibTest_ReadU32LE(aOutData + 4);

    TEST_ASSERT_EQUAL_UINT32(NUM_FRAMES + 1, numEntries);
    TEST_ASSERT_EQUAL_INT(AUDIOLIB_TEST_HEADER_SIZE + (NUM_FRAMES + 1) * AUDIOLIB_TEST_ENTRY_SIZE, result);

    uint32_t previousMouth = UINT32_MAX;
    uint32_t timeStep      = AUDIOLIB_TEST_TIME_UNITS_PER_SECOND / AUDIOLIB_TEST_ANALYSIS_RATE;

    for ( int frameIndex = 0; frameIndex < NUM_FRAMES; ++frameIndex )
    {
        uint32_t entry = AudioLibTest_ReadU32LE(aOutData + AUDIOLIB_TEST_HEADER_SIZE + frameIndex * AUDIOLIB_TEST_ENTRY_SIZE);
        uint32_t mouth = entry & 0x0000FFFFu;

        TEST_ASSERT_EQUAL_HEX32(((uint32_t)frameIndex * timeStep) & AUDIOLIB_TEST_TIME_MASK, entry & AUDIOLIB_TEST_TIME_MASK);
        TEST_ASSERT_NOT_EQUAL_UINT32(previousMouth, mouth);
        previousMouth = mouth;
    }

    uint32_t finalEntry = AudioLibTest_ReadU32LE(aOutData + AUDIOLIB_TEST_HEADER_SIZE + NUM_FRAMES * AUDIOLIB_TEST_ENTRY_SIZE);

    TEST_ASSERT_EQUAL_HEX32(((uint32_t)NUM_FRAMES * timeStep) & AUDIOLIB_TEST_TIME_MASK, finalEntry);
    TEST_ASSERT_EACH_EQUAL_UINT8(0xA5u, aOutData + result, sizeof(aOutData) - (size_t)result);
}

TEST(AudioLib, TestRandomReferenceVectors)
{
    static const uint8_t aExpectedA[] = {
        'S', 'Y', 'N', 'C', 0x03, 0x00, 0x00, 0x00,
        0x40, 0x20, 0x00, 0x00,
        0x40, 0x40, 0x01, 0x00,
        0x00, 0x00, 0x04, 0x00
    };
    static const uint8_t aExpectedB[] = {
        'S', 'Y', 'N', 'C', 0x05, 0x00, 0x00, 0x00,
        0x4A, 0x3C, 0x00, 0x00,
        0x4C, 0x44, 0x00, 0x00,
        0x48, 0x34, 0x01, 0x00,
        0x4C, 0x38, 0x02, 0x00,
        0x00, 0x00, 0x03, 0x00
    };
    static const uint8_t aExpectedC[] = {
        'S', 'Y', 'N', 'C', 0x04, 0x00, 0x00, 0x00,
        0x48, 0x3E, 0x00, 0x00,
        0x4C, 0x38, 0x01, 0x00,
        0x48, 0x40, 0x01, 0x00,
        0x00, 0x00, 0x02, 0x00
    };
    static const struct
    {
        unsigned int analysisRateHz;
        int8_t mouthXLevels;
        int8_t mouthYLevels;
        int sampleRate;
        int sndDataSize;
        uint32_t seed;
        const uint8_t* pExpected;
        size_t expectedSize;
    } aCases[] = {
        { 60, 4, 4, 8000, 1064, 0x13579BDFu, aExpectedA, sizeof(aExpectedA) },
        { 75, 32, 64, 16000, 1704, 0x2468ACE0u, aExpectedB, sizeof(aExpectedB) },
        { 100, 64, 32, 44100, 3528, 0xC0FFEE11u, aExpectedC, sizeof(aExpectedC) }
    };
    uint8_t aSndData[3528];
    uint8_t aOutData[64];

    for ( size_t caseIndex = 0; caseIndex < STD_ARRAYLEN(aCases); ++caseIndex )
    {
        AudioLibTest_FillPcm(aSndData, aCases[caseIndex].sndDataSize, 3, aCases[caseIndex].seed);
        memset(aOutData, 0xA5, sizeof(aOutData));

        int result = AudioLib_GenerateLipSyncBlock(
            aOutData,
            aSndData,
            aCases[caseIndex].analysisRateHz,
            aCases[caseIndex].mouthXLevels,
            aCases[caseIndex].mouthYLevels,
            aCases[caseIndex].sampleRate,
            16,
            1,
            0,
            aCases[caseIndex].sndDataSize
        );

        // NOTE: These deterministic goldens were generated by the standalone 1:1 reference.
        TEST_ASSERT_EQUAL_INT((int)aCases[caseIndex].expectedSize, result);
        TEST_ASSERT_EQUAL_HEX8_ARRAY(aCases[caseIndex].pExpected, aOutData, aCases[caseIndex].expectedSize);
        TEST_ASSERT_EACH_EQUAL_UINT8(0xA5u, aOutData + result, sizeof(aOutData) - (size_t)result);
    }
}

TEST(AudioLib, TestGeneratedRoundTrip)
{
    static const struct
    {
        unsigned int analysisRateHz;
        int8_t mouthXLevels;
        int8_t mouthYLevels;
        int sampleRate;
        int sndDataSize;
        int pattern;
        uint32_t seed;
        int bCrossSample;
    } aCases[] = {
        { 30, 4, 4, 8000, 3200, 0, 0, 0 },
        { 60, 64, 32, 8000, 1600, 2, 0, 0 },
        { 60, 32, 64, 16000, 6400, 3, 0x89ABCDEFu, 1 },
        { 30, 8, 16, 22050, 8820, 4, 0, 0 },
        { 60, 16, 8, 44100, 8820, 5, 0, 0 }
    };
    uint8_t aSndData[AUDIOLIB_TEST_MAX_PCM_SIZE + AUDIOLIB_TEST_INPUT_PADDING + 1];
    uint8_t aLipSyncData[AUDIOLIB_TEST_OUTPUT_SIZE];

    for ( size_t caseIndex = 0; caseIndex < STD_ARRAYLEN(aCases); ++caseIndex )
    {
        memset(aSndData, 0xA5, sizeof(aSndData));

        uint8_t* pPcmData = aSndData + (aCases[caseIndex].bCrossSample != 0);
        AudioLibTest_FillPcm(pPcmData, aCases[caseIndex].sndDataSize, aCases[caseIndex].pattern, aCases[caseIndex].seed);

        size_t lipSyncCapacity = AudioLib_GetLipSyncBlockMaxSize(
            aCases[caseIndex].analysisRateHz,
            aCases[caseIndex].sampleRate,
            16,
            1,
            aCases[caseIndex].sndDataSize
        );
        int blockSize = AudioLib_GenerateLipSyncBlockEx(
            aLipSyncData,
            lipSyncCapacity,
            aSndData,
            sizeof(aSndData),
            aCases[caseIndex].analysisRateHz,
            aCases[caseIndex].mouthXLevels,
            aCases[caseIndex].mouthYLevels,
            aCases[caseIndex].sampleRate,
            16,
            1,
            aCases[caseIndex].bCrossSample,
            aCases[caseIndex].sndDataSize
        );
        uint32_t numEntries = AudioLibTest_ReadU32LE(aLipSyncData + 4);

        TEST_ASSERT_GREATER_THAN_INT(AUDIOLIB_TEST_HEADER_SIZE, blockSize);
        TEST_ASSERT_EQUAL_INT(AUDIOLIB_TEST_HEADER_SIZE + AUDIOLIB_TEST_ENTRY_SIZE * (int)numEntries, blockSize);
        TEST_ASSERT_TRUE((size_t)blockSize <= lipSyncCapacity);
        AudioLibTest_AssertGeneratedTimeline(aLipSyncData);
    }
}

TEST_GROUP_RUNNER(AudioLib)
{
    RUN_TEST_CASE(AudioLib, TestMouthTimeline);
    RUN_TEST_CASE(AudioLib, TestMouthBoundaries);
    RUN_TEST_CASE(AudioLib, TestMouthSearchShapes);
    RUN_TEST_CASE(AudioLib, TestFinalTimestampBug);
    RUN_TEST_CASE(AudioLib, TestZeroEntryBug);
    RUN_TEST_CASE(AudioLib, TestMouthRejections);
    RUN_TEST_CASE(AudioLib, TestGenerateRejections);
    RUN_TEST_CASE(AudioLib, TestGenerateCapacity);
    RUN_TEST_CASE(AudioLib, TestGenerateSilence);
    RUN_TEST_CASE(AudioLib, TestGenerateWaveform);
    RUN_TEST_CASE(AudioLib, TestOffsetAndQuantization);
    RUN_TEST_CASE(AudioLib, TestGenerateRates);
    RUN_TEST_CASE(AudioLib, TestGenerateFrameBoundaries);
    RUN_TEST_CASE(AudioLib, TestRollingWindowWrap);
    RUN_TEST_CASE(AudioLib, TestTransientDecay);
    RUN_TEST_CASE(AudioLib, TestSignalClamp);
    RUN_TEST_CASE(AudioLib, TestZeroCrossings);
    RUN_TEST_CASE(AudioLib, TestFrameFirstSample);
    RUN_TEST_CASE(AudioLib, TestOutputGrowth);
    RUN_TEST_CASE(AudioLib, TestRandomReferenceVectors);
    RUN_TEST_CASE(AudioLib, TestGeneratedRoundTrip);
}

#include <unity_fixture.h>

#include <limits.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <sound/AudioLib.h>
#include <sound/Sound.h>
#include <std/General/stdUtil.h>
#include <std/types.h>

#define SOUND_TEST_SOUND_HANDLE       1234u
#define SOUND_TEST_CHANNEL_HANDLE     1235u
#define SOUND_TEST_DATA_OFFSET        64u
#define SOUND_TEST_PCM_SIZE           1600u
#define SOUND_TEST_FRAME_SIZE         266u
#define SOUND_TEST_LARGE_FRAME_COUNT  2048u
#define SOUND_TEST_CACHE_SIZE         600000u
#define SOUND_TEST_MAX_ALLOCATIONS    8u

typedef struct SoundTestAllocation
{
    void* pData;
    size_t size;
} SoundTestAllocation;

static tHostServices SoundTest_hostServices;
static SoundInfo SoundTest_soundInfo;
static tSoundChannel SoundTest_channel;
static uint8_t SoundTest_aSoundCache[SOUND_TEST_CACHE_SIZE];
static SoundTestAllocation SoundTest_aAllocations[SOUND_TEST_MAX_ALLOCATIONS];
static size_t SoundTest_numMallocCalls;
static size_t SoundTest_numFreeCalls;
static size_t SoundTest_numActiveAllocations;
static size_t SoundTest_failMallocCall;
static size_t SoundTest_maxAllocationSize;
static size_t SoundTest_numReallocCalls;
static size_t SoundTest_numUncompressCalls;
static unsigned int SoundTest_lastUncompressSize;

TEST_GROUP(Sound);

static int J3DAPI SoundTest_Print(const char* pFormat, ...)
{
    J3D_UNUSED(pFormat);
    return 0;
}

static void J3DAPI SoundTest_Assert(const char* pText, const char* pFile, int line)
{
    J3D_UNUSED(pText);
    J3D_UNUSED(pFile);
    J3D_UNUSED(line);
}

static void* J3DAPI SoundTest_Malloc(size_t size)
{
    ++SoundTest_numMallocCalls;
    if ( SoundTest_numMallocCalls == SoundTest_failMallocCall )
    {
        return NULL;
    }

    void* pData = malloc(size);
    TEST_ASSERT_NOT_NULL(pData);
    TEST_ASSERT_TRUE(SoundTest_numActiveAllocations < STD_ARRAYLEN(SoundTest_aAllocations));

    SoundTest_aAllocations[SoundTest_numActiveAllocations].pData = pData;
    SoundTest_aAllocations[SoundTest_numActiveAllocations].size  = size;
    ++SoundTest_numActiveAllocations;

    if ( size > SoundTest_maxAllocationSize )
    {
        SoundTest_maxAllocationSize = size;
    }

    return pData;
}

static void J3DAPI SoundTest_Free(void* pData)
{
    if ( !pData )
    {
        return;
    }

    size_t allocationIndex = SoundTest_numActiveAllocations;
    for ( size_t i = 0; i < SoundTest_numActiveAllocations; ++i )
    {
        if ( SoundTest_aAllocations[i].pData == pData )
        {
            allocationIndex = i;
            break;
        }
    }

    TEST_ASSERT_TRUE(allocationIndex < SoundTest_numActiveAllocations);
    free(pData);
    ++SoundTest_numFreeCalls;

    --SoundTest_numActiveAllocations;
    SoundTest_aAllocations[allocationIndex] = SoundTest_aAllocations[SoundTest_numActiveAllocations];
    SoundTest_aAllocations[SoundTest_numActiveAllocations].pData = NULL;
    SoundTest_aAllocations[SoundTest_numActiveAllocations].size  = 0;
}

static void* J3DAPI SoundTest_Realloc(void* pData, size_t size)
{
    J3D_UNUSED(pData);
    J3D_UNUSED(size);
    ++SoundTest_numReallocCalls;
    return NULL;
}

static void SoundTest_WriteU32LE(uint8_t* pData, uint32_t value)
{
    pData[0] = (uint8_t)value;
    pData[1] = (uint8_t)(value >> 8);
    pData[2] = (uint8_t)(value >> 16);
    pData[3] = (uint8_t)(value >> 24);
}

static uint32_t SoundTest_ReadU32LE(const uint8_t* pData)
{
    return (uint32_t)pData[0]
        | ((uint32_t)pData[1] << 8)
        | ((uint32_t)pData[2] << 16)
        | ((uint32_t)pData[3] << 24);
}

static void SoundTest_SetLipSyncEntry(uint8_t* pData, int index, uint32_t timeKey, uint8_t mouthX, uint8_t mouthY)
{
    uint32_t entry = (timeKey & 0xFFFF0000u) | ((uint32_t)(mouthX & 0x7Fu) << 8) | (mouthY & 0x7Fu);
    SoundTest_WriteU32LE(pData + 8 + index * 4, entry);
}

static void SoundTest_ResetAllocationState(void)
{
    TEST_ASSERT_EQUAL_size_t(0u, SoundTest_numActiveAllocations);

    STD_ZEROMEM(SoundTest_aAllocations, sizeof(SoundTest_aAllocations));
    SoundTest_numMallocCalls       = 0;
    SoundTest_numFreeCalls         = 0;
    SoundTest_numActiveAllocations = 0;
    SoundTest_failMallocCall       = SIZE_MAX;
    SoundTest_maxAllocationSize    = 0;
}

static void SoundTest_ConfigureDefaultFixture(void)
{
    STD_ZEROMEM(&SoundTest_soundInfo, sizeof(SoundTest_soundInfo));
    STD_ZEROMEM(&SoundTest_channel, sizeof(SoundTest_channel));
    STD_ZEROMEM(SoundTest_aSoundCache, sizeof(SoundTest_aSoundCache));

    TEST_ASSERT_TRUE(stdUtil_StringCopy(
        (char*)SoundTest_aSoundCache,
        STD_ARRAYLEN(SoundTest_aSoundCache),
        "voice_test.wav"
    ));
    STD_ZEROMEM(&SoundTest_aSoundCache[SOUND_TEST_DATA_OFFSET], SOUND_TEST_PCM_SIZE + sizeof(int16_t));
    SoundTest_aSoundCache[SOUND_TEST_DATA_OFFSET] = 0xA5;

    SoundTest_soundInfo.hSnd           = SOUND_TEST_SOUND_HANDLE;
    SoundTest_soundInfo.bankNum        = 0;
    SoundTest_soundInfo.filePathOffset = 0;
    SoundTest_soundInfo.dataOffset     = SOUND_TEST_DATA_OFFSET;
    SoundTest_soundInfo.sampleRate     = 8000;
    SoundTest_soundInfo.sempleBitSize  = 16;
    SoundTest_soundInfo.numChannels    = 1;
    SoundTest_soundInfo.dataSize       = SOUND_TEST_PCM_SIZE;
    SoundTest_soundInfo.bCompressed    = 0;

    SoundTest_channel.handle        = SOUND_TEST_CHANNEL_HANDLE;
    SoundTest_channel.hSnd          = SOUND_TEST_SOUND_HANDLE;
    SoundTest_channel.pDSoundBuffer = (tSysSoundBuffer*)(uintptr_t)1;

    Sound_TestSetChannels(&SoundTest_channel, 1);
    Sound_TestSetSoundBank(0, &SoundTest_soundInfo, 1, SoundTest_aSoundCache, sizeof(SoundTest_aSoundCache));
    Sound_TestSetCurrentPosition(0);
}

static void SoundTest_ConfigureCompressedFixture(void)
{
    SoundTest_soundInfo.bCompressed = 1;
    SoundTest_soundInfo.dataSize    = 16;
    SoundTest_WriteU32LE(&SoundTest_aSoundCache[SOUND_TEST_DATA_OFFSET], SOUND_TEST_PCM_SIZE);
}

static void J3DAPI SoundTest_Uncompress(tAudioCompressorState* pCompressorState, uint8_t* pOutSndData, const uint8_t* pCompressedData, unsigned int size)
{
    J3D_UNUSED(pCompressorState);
    J3D_UNUSED(pCompressedData);

    ++SoundTest_numUncompressCalls;
    SoundTest_lastUncompressSize = size;
    STD_ZEROMEM(pOutSndData, size);
}

static int SoundTest_IsTrackedAllocation(const void* pData)
{
    for ( size_t i = 0; i < SoundTest_numActiveAllocations; ++i )
    {
        if ( SoundTest_aAllocations[i].pData == pData )
        {
            return 1;
        }
    }

    return 0;
}

static void SoundTest_AssertLipSyncFailure(int msecLipSyncOffset)
{
    uint8_t mouthX = UINT8_MAX;
    uint8_t mouthY = UINT8_MAX;

    TEST_ASSERT_EQUAL_INT(0, Sound_GenerateLipSync(SOUND_TEST_CHANNEL_HANDLE, &mouthX, &mouthY, msecLipSyncOffset));
    TEST_ASSERT_EQUAL_UINT8(0, mouthX);
    TEST_ASSERT_EQUAL_UINT8(0, mouthY);
}

TEST_SETUP(Sound)
{
    STD_ZEROMEM(&SoundTest_hostServices, sizeof(SoundTest_hostServices));
    SoundTest_hostServices.pStatusPrint  = SoundTest_Print;
    SoundTest_hostServices.pMessagePrint = SoundTest_Print;
    SoundTest_hostServices.pWarningPrint = SoundTest_Print;
    SoundTest_hostServices.pErrorPrint   = SoundTest_Print;
    SoundTest_hostServices.pDebugPrint   = SoundTest_Print;
    SoundTest_hostServices.pAssert       = SoundTest_Assert;
    SoundTest_hostServices.pMalloc       = SoundTest_Malloc;
    SoundTest_hostServices.pFree         = SoundTest_Free;
    SoundTest_hostServices.pRealloc      = SoundTest_Realloc;

    SoundTest_numActiveAllocations = 0;
    SoundTest_ResetAllocationState();
    SoundTest_numReallocCalls    = 0;
    SoundTest_numUncompressCalls = 0;
    SoundTest_lastUncompressSize = 0;

    Sound_TestResetLipSyncState();
    TEST_ASSERT_EQUAL_INT(0, Sound_Initialize(&SoundTest_hostServices));
    SoundTest_ConfigureDefaultFixture();
    Sound_TestSetUncompressFunc(SoundTest_Uncompress);
}

TEST_TEAR_DOWN(Sound)
{
    if ( SoundTest_soundInfo.pLipSyncData && SoundTest_IsTrackedAllocation(SoundTest_soundInfo.pLipSyncData) )
    {
        SoundTest_Free(SoundTest_soundInfo.pLipSyncData);
        SoundTest_soundInfo.pLipSyncData = NULL;
    }

    size_t numLeakedAllocations = SoundTest_numActiveAllocations;
    while ( SoundTest_numActiveAllocations > 0 )
    {
        SoundTest_Free(SoundTest_aAllocations[SoundTest_numActiveAllocations - 1].pData);
    }

    Sound_Uninitialize();
    Sound_TestResetLipSyncState();
    TEST_ASSERT_EQUAL_size_t(0u, SoundTest_numReallocCalls);
    TEST_ASSERT_EQUAL_size_t(0u, numLeakedAllocations);
}

TEST(Sound, TestFailures)
{
    uint8_t mouthX = UINT8_MAX;
    uint8_t mouthY = UINT8_MAX;

    TEST_ASSERT_EQUAL_INT(0, Sound_GenerateLipSync(SOUND_INVALIDHANDLE, &mouthX, &mouthY, 0));
    TEST_ASSERT_EQUAL_UINT8(0, mouthX);
    TEST_ASSERT_EQUAL_UINT8(0, mouthY);

    SoundTest_channel.hSnd = SOUND_TEST_SOUND_HANDLE + 2u;
    mouthX = UINT8_MAX;
    mouthY = UINT8_MAX;
    TEST_ASSERT_EQUAL_INT(0, Sound_GenerateLipSync(SOUND_TEST_CHANNEL_HANDLE, &mouthX, &mouthY, 0));
    TEST_ASSERT_EQUAL_UINT8(0, mouthX);
    TEST_ASSERT_EQUAL_UINT8(0, mouthY);
    SoundTest_channel.hSnd = SOUND_TEST_SOUND_HANDLE;

    static const struct
    {
        uint32_t sampleRate;
        uint32_t bitsPerSample;
        uint32_t numChannels;
    } aInvalidFormats[] = {
        { 0, 16, 1 },
        { 8000, 8, 1 },
        { 8000, 24, 1 },
        { 8000, 16, 0 },
        { 8000, 16, 2 }
    };

    for ( size_t caseIndex = 0; caseIndex < STD_ARRAYLEN(aInvalidFormats); ++caseIndex )
    {
        SoundTest_soundInfo.sampleRate    = aInvalidFormats[caseIndex].sampleRate;
        SoundTest_soundInfo.sempleBitSize = aInvalidFormats[caseIndex].bitsPerSample;
        SoundTest_soundInfo.numChannels   = aInvalidFormats[caseIndex].numChannels;
        mouthX = UINT8_MAX;
        mouthY = UINT8_MAX;

        TEST_ASSERT_EQUAL_INT(0, Sound_GenerateLipSync(SOUND_TEST_CHANNEL_HANDLE, &mouthX, &mouthY, 0));
        TEST_ASSERT_EQUAL_UINT8(0, mouthX);
        TEST_ASSERT_EQUAL_UINT8(0, mouthY);
    }

    SoundTest_soundInfo.sampleRate    = 8000;
    SoundTest_soundInfo.sempleBitSize = 16;
    SoundTest_soundInfo.numChannels   = 1;
    Sound_TestSetNoLipSync(1);
    TEST_ASSERT_EQUAL_INT(0, Sound_GenerateLipSync(SOUND_TEST_CHANNEL_HANDLE, &mouthX, &mouthY, 0));
    TEST_ASSERT_EQUAL_size_t(0u, SoundTest_numMallocCalls);

}

TEST(Sound, TestUncompressedCache)
{
    uint8_t aOriginalPcm[SOUND_TEST_PCM_SIZE];
    memcpy(aOriginalPcm, &SoundTest_aSoundCache[SOUND_TEST_DATA_OFFSET + 1u], sizeof(aOriginalPcm));
    TEST_ASSERT_EACH_EQUAL_UINT8(0, aOriginalPcm, sizeof(aOriginalPcm));

    uint8_t mouthX = UINT8_MAX;
    uint8_t mouthY = UINT8_MAX;

    TEST_ASSERT_EQUAL_INT(1, Sound_GenerateLipSync(SOUND_TEST_CHANNEL_HANDLE, &mouthX, &mouthY, 0));
    TEST_ASSERT_NOT_NULL(SoundTest_soundInfo.pLipSyncData);
    TEST_ASSERT_EQUAL_MEMORY("SYNC", SoundTest_soundInfo.pLipSyncData, 4);
    TEST_ASSERT_EQUAL_HEX32(0x00002000u, SoundTest_ReadU32LE(SoundTest_soundInfo.pLipSyncData + 8));
    TEST_ASSERT_EQUAL_UINT8(32, mouthX);
    TEST_ASSERT_EQUAL_UINT8(0, mouthY);
    TEST_ASSERT_EQUAL_size_t(1u, SoundTest_numMallocCalls);
    TEST_ASSERT_EQUAL_size_t(1u, SoundTest_numActiveAllocations);
    TEST_ASSERT_EQUAL_MEMORY(aOriginalPcm, &SoundTest_aSoundCache[SOUND_TEST_DATA_OFFSET + 1u], sizeof(aOriginalPcm));

    uint8_t* pCachedData = SoundTest_soundInfo.pLipSyncData;
    Sound_TestSetNoLipSync(1);
    Sound_TestSetCurrentPosition(532);

    TEST_ASSERT_EQUAL_INT(1, Sound_GenerateLipSync(SOUND_TEST_CHANNEL_HANDLE, &mouthX, &mouthY, 0));
    TEST_ASSERT_EQUAL_PTR(pCachedData, SoundTest_soundInfo.pLipSyncData);
    TEST_ASSERT_EQUAL_size_t(1u, SoundTest_numMallocCalls);
}

TEST(Sound, TestPlaybackPosition)
{
    uint8_t aLipSyncData[8 + 3 * 4];
    memcpy(aLipSyncData, "SYNC", 4);
    SoundTest_WriteU32LE(aLipSyncData + 4, 3);
    SoundTest_SetLipSyncEntry(aLipSyncData, 0, 0x00000000u, 1, 2);
    SoundTest_SetLipSyncEntry(aLipSyncData, 1, 0x00020000u, 3, 4);
    SoundTest_SetLipSyncEntry(aLipSyncData, 2, 0x00030000u, 5, 6);
    SoundTest_soundInfo.pLipSyncData = aLipSyncData;

    uint8_t mouthX = UINT8_MAX;
    uint8_t mouthY = UINT8_MAX;

    Sound_TestSetCurrentPosition(0);
    TEST_ASSERT_EQUAL_INT(1, Sound_GenerateLipSync(SOUND_TEST_CHANNEL_HANDLE, &mouthX, &mouthY, 0));
    TEST_ASSERT_EQUAL_UINT8(1, mouthX);
    TEST_ASSERT_EQUAL_UINT8(2, mouthY);

    Sound_TestSetCurrentPosition(512);
    TEST_ASSERT_EQUAL_INT(1, Sound_GenerateLipSync(SOUND_TEST_CHANNEL_HANDLE, &mouthX, &mouthY, 0));
    TEST_ASSERT_EQUAL_UINT8(3, mouthX);
    TEST_ASSERT_EQUAL_UINT8(4, mouthY);

    Sound_TestSetCurrentPosition(0);
    TEST_ASSERT_EQUAL_INT(1, Sound_GenerateLipSync(SOUND_TEST_CHANNEL_HANDLE, NULL, &mouthY, 16));
    TEST_ASSERT_EQUAL_UINT8(2, mouthY);
    TEST_ASSERT_EQUAL_INT(1, Sound_GenerateLipSync(SOUND_TEST_CHANNEL_HANDLE, &mouthX, NULL, 16));
    TEST_ASSERT_EQUAL_UINT8(1, mouthX);

    Sound_TestSetCurrentPosition(SOUND_TEST_PCM_SIZE - 400u);
    mouthX = UINT8_MAX;
    mouthY = UINT8_MAX;
    TEST_ASSERT_EQUAL_INT(0, Sound_GenerateLipSync(SOUND_TEST_CHANNEL_HANDLE, &mouthX, &mouthY, 0));
    TEST_ASSERT_EQUAL_UINT8(0, mouthX);
    TEST_ASSERT_EQUAL_UINT8(0, mouthY);

    Sound_TestSetCurrentPosition(SOUND_TEST_PCM_SIZE - 401u);
    TEST_ASSERT_EQUAL_INT(1, Sound_GenerateLipSync(SOUND_TEST_CHANNEL_HANDLE, &mouthX, &mouthY, 0));
    TEST_ASSERT_EQUAL_UINT8(5, mouthX);
    TEST_ASSERT_EQUAL_UINT8(6, mouthY);

    Sound_TestSetCurrentPosition(0);
    TEST_ASSERT_EQUAL_INT(1, Sound_GenerateLipSync(SOUND_TEST_CHANNEL_HANDLE, NULL, NULL, 0));

    SoundTest_AssertLipSyncFailure(-1);

    Sound_TestSetCurrentPosition(512);
    SoundTest_AssertLipSyncFailure(INT_MAX);

    Sound_TestSetCurrentPosition(SOUND_TEST_PCM_SIZE);
    SoundTest_AssertLipSyncFailure(0);

    aLipSyncData[0] = 'X';
    Sound_TestSetCurrentPosition(0);
    SoundTest_AssertLipSyncFailure(0);
}

TEST(Sound, TestCompressedCache)
{
    SoundTest_ConfigureCompressedFixture();

    uint8_t mouthX = UINT8_MAX;
    uint8_t mouthY = UINT8_MAX;

    TEST_ASSERT_EQUAL_INT(1, Sound_GenerateLipSync(SOUND_TEST_CHANNEL_HANDLE, &mouthX, &mouthY, 0));
    TEST_ASSERT_EQUAL_UINT8(32, mouthX);
    TEST_ASSERT_EQUAL_UINT8(0, mouthY);
    TEST_ASSERT_EQUAL_size_t(1u, SoundTest_numUncompressCalls);
    TEST_ASSERT_EQUAL_UINT32(SOUND_TEST_PCM_SIZE, SoundTest_lastUncompressSize);
    TEST_ASSERT_EQUAL_size_t(2u, SoundTest_numMallocCalls);
    TEST_ASSERT_EQUAL_size_t(1u, SoundTest_numFreeCalls);
    TEST_ASSERT_EQUAL_size_t(1u, SoundTest_numActiveAllocations);
    TEST_ASSERT_EQUAL_size_t(SOUND_TEST_PCM_SIZE + sizeof(int16_t), SoundTest_maxAllocationSize);

    TEST_ASSERT_EQUAL_INT(1, Sound_GenerateLipSync(SOUND_TEST_CHANNEL_HANDLE, &mouthX, &mouthY, 0));
    TEST_ASSERT_EQUAL_size_t(1u, SoundTest_numUncompressCalls);
    TEST_ASSERT_EQUAL_size_t(2u, SoundTest_numMallocCalls);
}

TEST(Sound, TestAllocationFailures)
{
    uint8_t mouthX = UINT8_MAX;
    uint8_t mouthY = UINT8_MAX;

    SoundTest_failMallocCall = 1;
    TEST_ASSERT_EQUAL_INT(0, Sound_GenerateLipSync(SOUND_TEST_CHANNEL_HANDLE, &mouthX, &mouthY, 0));
    TEST_ASSERT_NULL(SoundTest_soundInfo.pLipSyncData);
    TEST_ASSERT_EQUAL_size_t(0u, SoundTest_numActiveAllocations);

    SoundTest_ResetAllocationState();
    SoundTest_ConfigureCompressedFixture();
    SoundTest_failMallocCall = 1;
    TEST_ASSERT_EQUAL_INT(0, Sound_GenerateLipSync(SOUND_TEST_CHANNEL_HANDLE, &mouthX, &mouthY, 0));
    TEST_ASSERT_EQUAL_size_t(0u, SoundTest_numUncompressCalls);
    TEST_ASSERT_EQUAL_size_t(0u, SoundTest_numActiveAllocations);

    SoundTest_ResetAllocationState();
    SoundTest_failMallocCall = 2;
    TEST_ASSERT_EQUAL_INT(0, Sound_GenerateLipSync(SOUND_TEST_CHANNEL_HANDLE, &mouthX, &mouthY, 0));
    TEST_ASSERT_EQUAL_size_t(1u, SoundTest_numUncompressCalls);
    TEST_ASSERT_EQUAL_size_t(1u, SoundTest_numFreeCalls);
    TEST_ASSERT_EQUAL_size_t(0u, SoundTest_numActiveAllocations);
    TEST_ASSERT_NULL(SoundTest_soundInfo.pLipSyncData);
}

TEST(Sound, TestLargeLipSyncBuffer)
{
    SoundTest_soundInfo.dataSize = SOUND_TEST_LARGE_FRAME_COUNT * SOUND_TEST_FRAME_SIZE;
    STD_ZEROMEM(&SoundTest_aSoundCache[SOUND_TEST_DATA_OFFSET + 1u], SoundTest_soundInfo.dataSize);

    uint8_t mouthX = UINT8_MAX;
    uint8_t mouthY = UINT8_MAX;

    TEST_ASSERT_EQUAL_INT(1, Sound_GenerateLipSync(SOUND_TEST_CHANNEL_HANDLE, &mouthX, &mouthY, 0));
    TEST_ASSERT_GREATER_THAN_size_t(8192u, SoundTest_maxAllocationSize);
    TEST_ASSERT_NOT_NULL(SoundTest_soundInfo.pLipSyncData);
    TEST_ASSERT_EQUAL_MEMORY("SYNC", SoundTest_soundInfo.pLipSyncData, 4);
}

TEST(Sound, TestMouthLevelMacro)
{
    for ( uint32_t mouthPosition = 0; mouthPosition < AUDIOLIB_LIPSYNC_MOUTH_POSITION_COUNT; ++mouthPosition )
    {
        uint32_t expectedLevel = mouthPosition / SOUND_LIPSYNC_MOUTH_LEVEL_STEP;
        TEST_ASSERT_EQUAL_UINT32(expectedLevel, SOUND_LIPSYNC_GETMOUTHLEVEL(mouthPosition));
    }
}

TEST_GROUP_RUNNER(Sound)
{
    RUN_TEST_CASE(Sound, TestFailures);
    RUN_TEST_CASE(Sound, TestUncompressedCache);
    RUN_TEST_CASE(Sound, TestPlaybackPosition);
    RUN_TEST_CASE(Sound, TestCompressedCache);
    RUN_TEST_CASE(Sound, TestAllocationFailures);
    RUN_TEST_CASE(Sound, TestLargeLipSyncBuffer);
    RUN_TEST_CASE(Sound, TestMouthLevelMacro);
}

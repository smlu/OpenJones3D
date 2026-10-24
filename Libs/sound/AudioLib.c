#include "AudioLib.h"
#include <j3dcore/j3d.h>
#include <j3dcore/j3dhook.h>
#include <sound/RTI/symbols.h>
#include <std/General/stdUtil.h>
#include <limits.h>

#define AUDIOLIB_LIPSYNC_MAGIC                       "SYNC"
#define AUDIOLIB_LIPSYNC_MAGIC_LENGTH                ((int)sizeof(AUDIOLIB_LIPSYNC_MAGIC) - 1)
#define AUDIOLIB_LIPSYNC_HEADER_SIZE                 (AUDIOLIB_LIPSYNC_MAGIC_LENGTH + (int)sizeof(uint32_t))
#define AUDIOLIB_LIPSYNC_ENTRY_SIZE                  ((int)sizeof(uint32_t))
#define AUDIOLIB_BITS_PER_BYTE                       CHAR_BIT
#define AUDIOLIB_LIPSYNC_SAMPLE_SIZE                 ((int)sizeof(int16_t))
#define AUDIOLIB_LIPSYNC_BITS_PER_SAMPLE             (AUDIOLIB_LIPSYNC_SAMPLE_SIZE * AUDIOLIB_BITS_PER_BYTE)
#define AUDIOLIB_LIPSYNC_NUM_CHANNELS                1
#define AUDIOLIB_LIPSYNC_MOUTH_X_SHIFT               AUDIOLIB_BITS_PER_BYTE
#define AUDIOLIB_LIPSYNC_TIME_SHIFT                  12u
#define AUDIOLIB_LIPSYNC_TIME_SCALE                  (1u << AUDIOLIB_LIPSYNC_TIME_SHIFT)
#define AUDIOLIB_LIPSYNC_TIME_UNITS_PER_SECOND       (AUDIOLIB_LIPSYNC_TIME_SCALE * 1000u) // 4096 fixed-point lip-sync time units per millisecond, scaled to one second.
#define AUDIOLIB_LIPSYNC_TIME_MASK                   0xFFFF0000u
#define AUDIOLIB_LIPSYNC_MSEC_OVERFLOW_MASK          0xFFF00000u
#define AUDIOLIB_LIPSYNC_MOUTH_MASK                  0x7Fu
#define AUDIOLIB_LIPSYNC_MASK_BIT                    1u
#define AUDIOLIB_LIPSYNC_MOUTH_MAX                   127
#define AUDIOLIB_LIPSYNC_PERCENT_SCALE               100
#define AUDIOLIB_LIPSYNC_ROLLING_MAX_COUNT           15
#define AUDIOLIB_LIPSYNC_ROLLING_MIN_DIVISOR         4
#define AUDIOLIB_LIPSYNC_MOUTH_DECAY_STEP            16
#define AUDIOLIB_LIPSYNC_MOUTH_X_DECAY_MIN           47
#define AUDIOLIB_LIPSYNC_MOUTH_Y_DECAY_MIN           15
#define AUDIOLIB_LIPSYNC_QUIET_THRESHOLD             25
#define AUDIOLIB_LIPSYNC_NEUTRAL_MOUTH_X             37
#define AUDIOLIB_LIPSYNC_MOUTH_X_SCALE               50
#define AUDIOLIB_LIPSYNC_MOUTH_Y_SCALE               60
#define AUDIOLIB_LIPSYNC_ZERO_CROSSING_BIAS          1u
#define AUDIOLIB_LIPSYNC_TRANSIENT_THRESHOLD_PERCENT 30

#define AUDIOLIB_PACK_LIPSYNC_ENTRY(timeKey, mouthX, mouthY) (((uint32_t)(mouthY) & AUDIOLIB_LIPSYNC_MOUTH_MASK) | (((uint32_t)(mouthX) & AUDIOLIB_LIPSYNC_MOUTH_MASK) << 8) | ((timeKey) & AUDIOLIB_LIPSYNC_TIME_MASK))

#define AudioLib_aStepTable J3D_DECL_FAR_ARRAYVAR(AudioLib_aStepTable, const int16_t(*)[89])
#define AudioLib_aStepBits J3D_DECL_FAR_ARRAYVAR(AudioLib_aStepBits, const uint8_t(*)[89])
#define AudioLib_aIndexTableTable J3D_DECL_FAR_ARRAYVAR(AudioLib_aIndexTableTable, const int8_t*(*)[8])
#define AudioLib_aDeltaTable J3D_DECL_FAR_ARRAYVAR(AudioLib_aDeltaTable, int16_t(*)[64])
#define AudioLib_word_14E4928 J3D_DECL_FAR_ARRAYVAR(AudioLib_word_14E4928, int16_t(*)[5632])
#define AudioLib_bDeltaTableInitialized J3D_DECL_FAR_VAR(AudioLib_bDeltaTableInitialized, int)

void AudioLib_InstallHooks(void)
{
    // Uncomment only lines for functions that have full definition and doesn't call original function (non-thunk functions)

    // J3D_HOOKFUNC(AudioLib_ParseWaveFileHeader);
    // J3D_HOOKFUNC(AudioLib_Compress);
    // J3D_HOOKFUNC(AudioLib_ResetCompressor);
    // J3D_HOOKFUNC(AudioLib_Uncompress);
    J3D_HOOKFUNC(AudioLib_GetMouthPosition);
    J3D_HOOKFUNC(AudioLib_GenerateLipSyncBlock);
   // J3D_HOOKFUNC(AudioLib_CompressBlock);
   // J3D_HOOKFUNC(AudioLib_UncompressBlock);
    J3D_HOOKFUNC(AudioLib_WVSMCompressBlock);
    // J3D_HOOKFUNC(AudioLib_WVSMUncompressBlock);
}

void AudioLib_ResetGlobals(void)
{
    const int16_t AudioLib_aStepTable_tmp[89] = {
      7,
      8,
      9,
      10,
      11,
      12,
      13,
      14,
      16,
      17,
      19,
      21,
      23,
      25,
      28,
      31,
      34,
      37,
      41,
      45,
      50,
      55,
      60,
      66,
      73,
      80,
      88,
      97,
      107,
      118,
      130,
      143,
      157,
      173,
      190,
      209,
      230,
      253,
      279,
      307,
      337,
      371,
      408,
      449,
      494,
      544,
      598,
      658,
      724,
      796,
      876,
      963,
      1060,
      1166,
      1282,
      1411,
      1552,
      1707,
      1878,
      2066,
      2272,
      2499,
      2749,
      3024,
      3327,
      3660,
      4026,
      4428,
      4871,
      5358,
      5894,
      6484,
      7132,
      7845,
      8630,
      9493,
      10442,
      11487,
      12635,
      13899,
      15289,
      16818,
      18500,
      20350,
      22385,
      24623,
      27086,
      29794,
      32767
    };
    memcpy((int16_t*)&AudioLib_aStepTable, &AudioLib_aStepTable_tmp, sizeof(AudioLib_aStepTable));

    const uint8_t AudioLib_aStepBits_tmp[89] = {
      4u,
      4u,
      4u,
      4u,
      4u,
      4u,
      4u,
      4u,
      4u,
      4u,
      4u,
      4u,
      4u,
      4u,
      4u,
      4u,
      4u,
      4u,
      4u,
      4u,
      4u,
      4u,
      4u,
      4u,
      4u,
      4u,
      4u,
      4u,
      4u,
      4u,
      4u,
      4u,
      4u,
      4u,
      4u,
      4u,
      4u,
      4u,
      4u,
      4u,
      4u,
      4u,
      4u,
      4u,
      4u,
      5u,
      5u,
      5u,
      5u,
      5u,
      5u,
      5u,
      5u,
      5u,
      5u,
      5u,
      5u,
      5u,
      5u,
      6u,
      6u,
      6u,
      6u,
      6u,
      6u,
      6u,
      6u,
      6u,
      6u,
      6u,
      6u,
      6u,
      6u,
      6u,
      7u,
      7u,
      7u,
      7u,
      7u,
      7u,
      7u,
      7u,
      7u,
      7u,
      7u,
      7u,
      7u,
      7u,
      7u
    };
    memcpy((uint8_t*)&AudioLib_aStepBits, &AudioLib_aStepBits_tmp, sizeof(AudioLib_aStepBits));

    /* const int8_t *AudioLib_aIndexTableTable_tmp[8] = {
       NULL,
       NULL,
       &AudioLib_aIndex2Bit,
       &AudioLib_aIndex3Bit,
       &AudioLib_aIndex4Bit,
       &AudioLib_aIndex5Bit,
       &AudioLib_aIndex6Bit,
       &AudioLib_aIndex7Bit
     };
     memcpy((int8_t **)&AudioLib_aIndexTableTable, &AudioLib_aIndexTableTable_tmp, sizeof(AudioLib_aIndexTableTable));*/

    memset(&AudioLib_aDeltaTable, 0, sizeof(AudioLib_aDeltaTable));
    memset(&AudioLib_word_14E4928, 0, sizeof(AudioLib_word_14E4928));
    memset(&AudioLib_bDeltaTableInitialized, 0, sizeof(AudioLib_bDeltaTableInitialized));
}

// Returns pointer to snd data
const uint8_t* J3DAPI AudioLib_ParseWaveFileHeader(const uint8_t* pData, int* pType, uint32_t* pSampleRate, uint32_t* pBitsPerSample, uint32_t* pNumChannels, uint32_t* pExtraInfo, uint32_t* pSoundDataSize, uint32_t* pExtraDataOffset, uint32_t* pSoundDataOffset)
{
    return J3D_TRAMPOLINE_CALL(AudioLib_ParseWaveFileHeader, pData, pType, pSampleRate, pBitsPerSample, pNumChannels, pExtraInfo, pSoundDataSize, pExtraDataOffset, pSoundDataOffset);
}

int J3DAPI AudioLib_Compress(tAudioCompressorState* pCompressorState, uint8_t* pOutBuffer, const uint8_t* pInBuffer, int size, unsigned int numChannels)
{
    return J3D_TRAMPOLINE_CALL(AudioLib_Compress, pCompressorState, pOutBuffer, pInBuffer, size, numChannels);
}

void J3DAPI AudioLib_ResetCompressor(tAudioCompressorState* pState)
{
    J3D_TRAMPOLINE_CALL(AudioLib_ResetCompressor, pState);
}

void J3DAPI AudioLib_Uncompress(tAudioCompressorState* pCompressorState, uint8_t* pOutSndData, const uint8_t* pCompressedData, unsigned int size)
{
    J3D_TRAMPOLINE_CALL(AudioLib_Uncompress, pCompressorState, pOutSndData, pCompressedData, size);
}

//int J3DAPI AudioLib_GetMouthPosition(uint8_t* pData, int a2, uint8_t* pMouthPosX, uint8_t* pMouthPosY)
//{
//    return J3D_TRAMPOLINE_CALL(AudioLib_GetMouthPosition, pData, a2, pMouthPosX, pMouthPosY);
//}
//
//int J3DAPI AudioLib_GenerateLipSyncBlock(uint8_t* pOutData, const uint8_t* pSndData, uint32_t a3, int8_t a4, int8_t a5, uint32_t sampleRate, uint32_t bitsPerSample, uint32_t numChannels, int a9, uint32_t sndDataSize)
//{
//    return J3D_TRAMPOLINE_CALL(AudioLib_GenerateLipSyncBlock, pOutData, pSndData, a3, a4, a5, sampleRate, bitsPerSample, numChannels, a9, sndDataSize);
//}

// Added helper functions for readability; original code used direct little-endian casts.
// Reads a 32-bit little-endian value from the lip-sync block.
static inline uint32_t AudioLib_ReadU32LE(const uint8_t* pData)
{
    return (uint32_t)pData[0]
        | ((uint32_t)pData[1] << AUDIOLIB_BITS_PER_BYTE)
        | ((uint32_t)pData[2] << (AUDIOLIB_BITS_PER_BYTE * 2))
        | ((uint32_t)pData[3] << (AUDIOLIB_BITS_PER_BYTE * 3));
}

// Writes a 32-bit little-endian value to the lip-sync block.
static inline void AudioLib_WriteU32LE(uint8_t* pData, uint32_t value)
{
    pData[0] = (uint8_t)value;
    pData[1] = (uint8_t)(value >> AUDIOLIB_BITS_PER_BYTE);
    pData[2] = (uint8_t)(value >> (AUDIOLIB_BITS_PER_BYTE * 2));
    pData[3] = (uint8_t)(value >> (AUDIOLIB_BITS_PER_BYTE * 3));
}

// Reads a signed 16-bit little-endian PCM sample.
static inline int16_t AudioLib_ReadS16LE(const uint8_t* pData)
{
    uint16_t value = (uint16_t)pData[0] | ((uint16_t)pData[1] << AUDIOLIB_BITS_PER_BYTE);
    return (int16_t)value;
}

// Matches the original abs() use on 16-bit PCM samples.
static inline int AudioLib_AbsS16(int16_t value)
{
    return value < 0 ? -(int)value : (int)value;
}

int J3DAPI AudioLib_GetMouthPosition(uint8_t* pLipSyncData, int msecTime, uint8_t* pMouthPosX, uint8_t* pMouthPosY)
{
    uint8_t mouthX = 0;
    uint8_t mouthY = 0;
    int result = 0;

    if ( memeq(pLipSyncData, AUDIOLIB_LIPSYNC_MAGIC, AUDIOLIB_LIPSYNC_MAGIC_LENGTH) )
    {
        int numEntries = (int)AudioLib_ReadU32LE(pLipSyncData + AUDIOLIB_LIPSYNC_MAGIC_LENGTH);
        const uint8_t* pEntries = pLipSyncData + AUDIOLIB_LIPSYNC_HEADER_SIZE;

        if ( (msecTime & AUDIOLIB_LIPSYNC_MSEC_OVERFLOW_MASK) != 0 )
        {
            result = 1;
        }
        else
        {
            uint32_t targetTimeKey = ((uint32_t)msecTime << AUDIOLIB_LIPSYNC_TIME_SHIFT) & AUDIOLIB_LIPSYNC_TIME_MASK;
            int first = 0;
            int last = numEntries - 1;
            int entryIndex = 0;
            // TODO: [BUG] Reject zero-entry blocks; the original reads before the entry array when numEntries is zero.
            uint32_t lastEntry = AudioLib_ReadU32LE(pEntries + AUDIOLIB_LIPSYNC_ENTRY_SIZE * numEntries - AUDIOLIB_LIPSYNC_ENTRY_SIZE);

            // TODO: [BUG] An exact final timestamp selects the preceding entry; only a later timestamp selects the last entry.
            if ( targetTimeKey > lastEntry )
            {
                entryIndex = last;
                targetTimeKey = 0;
            }

            while ( targetTimeKey != 0 )
            {
                int middle;
                uint32_t entry;

                if ( first + 1 >= last )
                {
                    entryIndex = first + (last - first) / 2;
                    break;
                }

                middle = first + (last - first) / 2;
                entry = AudioLib_ReadU32LE(pEntries + AUDIOLIB_LIPSYNC_ENTRY_SIZE * middle);

                if ( targetTimeKey == (entry & AUDIOLIB_LIPSYNC_TIME_MASK) )
                {
                    entryIndex = middle;
                    break;
                }

                if ( targetTimeKey <= entry )
                {
                    last = middle;
                }
                else
                {
                    first = middle;
                }
            }

            uint32_t entry = AudioLib_ReadU32LE(pEntries + AUDIOLIB_LIPSYNC_ENTRY_SIZE * entryIndex);
            mouthX = (uint8_t)((entry >> AUDIOLIB_LIPSYNC_MOUTH_X_SHIFT) & AUDIOLIB_LIPSYNC_MOUTH_MASK);
            mouthY = (uint8_t)(entry & AUDIOLIB_LIPSYNC_MOUTH_MASK);
        }
    }
    else
    {
        result = 1;
    }

    if ( pMouthPosX )
    {
        *pMouthPosX = mouthX;
    }

    if ( pMouthPosY )
    {
        *pMouthPosY = mouthY;
    }

    return result;
}

// Added: Calculate the worst-case generated block size so checked callers can allocate safely.
size_t J3DAPI AudioLib_GetLipSyncBlockMaxSize(uint32_t analysisRateHz, uint32_t sampleRate, uint32_t bitsPerSample, uint32_t numChannels, uint32_t sndDataSize)
{
    if ( analysisRateHz == 0u
        || analysisRateHz > AUDIOLIB_LIPSYNC_TIME_UNITS_PER_SECOND
        || sampleRate == 0u
        || sampleRate > (uint32_t)INT_MAX / AUDIOLIB_LIPSYNC_SAMPLE_SIZE
        || bitsPerSample != AUDIOLIB_LIPSYNC_BITS_PER_SAMPLE
        || numChannels != AUDIOLIB_LIPSYNC_NUM_CHANNELS
        || sndDataSize < (uint32_t)AUDIOLIB_LIPSYNC_SAMPLE_SIZE
        || sndDataSize % AUDIOLIB_LIPSYNC_SAMPLE_SIZE != 0
        || (size_t)sampleRate > SIZE_MAX / AUDIOLIB_LIPSYNC_SAMPLE_SIZE )
    {
        return 0;
    }

    size_t bytesPerAnalysisFrame = (size_t)AUDIOLIB_LIPSYNC_SAMPLE_SIZE * (size_t)sampleRate / analysisRateHz;
    if ( bytesPerAnalysisFrame < AUDIOLIB_LIPSYNC_SAMPLE_SIZE
        || bytesPerAnalysisFrame / AUDIOLIB_LIPSYNC_SAMPLE_SIZE > INT_MAX / ((int)INT16_MAX + 1)
        || (size_t)sndDataSize > (size_t)INT_MAX - bytesPerAnalysisFrame )
    {
        return 0;
    }

    size_t maxZeroCrossings = (size_t)sndDataSize / (AUDIOLIB_LIPSYNC_SAMPLE_SIZE * 2u);
    if ( maxZeroCrossings > 0 && bytesPerAnalysisFrame > (size_t)INT_MAX / maxZeroCrossings )
    {
        return 0;
    }

    size_t numFrames = (size_t)sndDataSize / bytesPerAnalysisFrame;
    if ( numFrames > (SIZE_MAX - AUDIOLIB_LIPSYNC_HEADER_SIZE) / AUDIOLIB_LIPSYNC_ENTRY_SIZE - 1u )
    {
        return 0;
    }

    size_t maxBlockSize = AUDIOLIB_LIPSYNC_HEADER_SIZE + (numFrames + 1u) * AUDIOLIB_LIPSYNC_ENTRY_SIZE;
    return maxBlockSize <= INT_MAX ? maxBlockSize : 0;
}

// Added: Reject missing or undersized buffers before entering the original unchecked generator.
int J3DAPI AudioLib_GenerateLipSyncBlockEx(uint8_t* pOutData, size_t outDataSize, const uint8_t* pSndData, size_t sndDataCapacity, uint32_t analysisRateHz, int8_t mouthXLevels, int8_t mouthYLevels, uint32_t sampleRate, uint32_t bitsPerSample, uint32_t numChannels, int bCrossSample, uint32_t sndDataSize)
{
    size_t maxBlockSize          = AudioLib_GetLipSyncBlockMaxSize(analysisRateHz, sampleRate, bitsPerSample, numChannels, sndDataSize);
    size_t bytesPerAnalysisFrame = maxBlockSize != 0 ? (size_t)AUDIOLIB_LIPSYNC_SAMPLE_SIZE * (size_t)sampleRate / analysisRateHz : 0;
    size_t inputPadding          = (bCrossSample != 0) + (bytesPerAnalysisFrame % AUDIOLIB_LIPSYNC_SAMPLE_SIZE != 0);
    size_t requiredSndDataSize   = sndDataSize > 0u && (size_t)sndDataSize <= SIZE_MAX - inputPadding ? (size_t)sndDataSize + inputPadding : 0;

    if ( !pOutData
        || !pSndData
        || maxBlockSize == 0
        || outDataSize < maxBlockSize
        || requiredSndDataSize == 0
        || sndDataCapacity < requiredSndDataSize )
    {
        return 0;
    }

    return AudioLib_GenerateLipSyncBlock(
        pOutData,
        pSndData,
        analysisRateHz,
        mouthXLevels,
        mouthYLevels,
        sampleRate,
        bitsPerSample,
        numChannels,
        bCrossSample,
        sndDataSize
    );
}

// Altered: Use explicit signed-byte mouth levels and unsigned audio metadata and counters.
int J3DAPI AudioLib_GenerateLipSyncBlock(uint8_t* pOutData, const uint8_t* pSndData, uint32_t analysisRateHz, int8_t mouthXLevels, int8_t mouthYLevels, uint32_t sampleRate, uint32_t bitsPerSample, uint32_t numChannels, int bCrossSample, uint32_t sndDataSize)
{
    uint8_t* pCurOut = pOutData;
    char aDebugBuffer[200];
    int aRollingMax[AUDIOLIB_LIPSYNC_ROLLING_MAX_COUNT];
    int rollingMaxIndex = 0;
    int maxAmplitude = 0;
    unsigned int absAmplitude = 0;
    int totalZeroCrossings = 0;
    int previousMouthX = -1;
    int previousMouthY = -1;
    int previousRawMouthX = 0;
    int previousRawMouthY = 0;
    int mouthXMask = 0;
    int mouthYMask = 0;
    uint32_t bytesPerAnalysisFrame;
    unsigned int zeroCrossingsPerFrame;
    uint32_t timeStep;
    uint32_t timeKey = 0;
    uint32_t processedBytes = 0;
    uint8_t mouthXLevelsMinusOne;
    uint8_t mouthYLevelsMinusOne;

    if ( !sndDataSize )
    {
        return 0;
    }

    if ( !pSndData || bitsPerSample != AUDIOLIB_LIPSYNC_BITS_PER_SAMPLE || numChannels != AUDIOLIB_LIPSYNC_NUM_CHANNELS )
    {
        return 0;
    }

    if ( bCrossSample )
    {
        ++pSndData;
    }

    strcpy_s((char*)pOutData, sizeof(AUDIOLIB_LIPSYNC_MAGIC), AUDIOLIB_LIPSYNC_MAGIC);
    pCurOut += AUDIOLIB_LIPSYNC_HEADER_SIZE;

    mouthXLevelsMinusOne = (uint8_t)(mouthXLevels - 1);
    mouthYLevelsMinusOne = (uint8_t)(mouthYLevels - 1);
    for ( int i = 0; i < (int)AUDIOLIB_LIPSYNC_MOUTH_POSITION_BITS; ++i )
    {
        mouthXMask *= 2;
        mouthYMask *= 2;

        if ( mouthXLevelsMinusOne )
        {
            mouthXMask |= AUDIOLIB_LIPSYNC_MASK_BIT;
        }

        if ( mouthYLevelsMinusOne )
        {
            mouthYMask |= AUDIOLIB_LIPSYNC_MASK_BIT;
        }

        mouthXLevelsMinusOne >>= 1;
        mouthYLevelsMinusOne >>= 1;
    }

    // TODO: [BUG] Validate the rate and complete sample/frame sizes; the original can divide by zero, stall, or read past odd boundaries.
    timeStep = AUDIOLIB_LIPSYNC_TIME_UNITS_PER_SECOND / analysisRateHz;

    for ( uint32_t byteOffset = AUDIOLIB_LIPSYNC_SAMPLE_SIZE; byteOffset < sndDataSize; byteOffset += AUDIOLIB_LIPSYNC_SAMPLE_SIZE )
    {
        int16_t previousSample = AudioLib_ReadS16LE(pSndData + byteOffset - AUDIOLIB_LIPSYNC_SAMPLE_SIZE);
        int16_t sample = AudioLib_ReadS16LE(pSndData + byteOffset);
        int absSample = AudioLib_AbsS16(sample);

        if ( sample >= 0 && previousSample < 0 )
        {
            ++totalZeroCrossings;
        }

        if ( absSample > maxAmplitude )
        {
            maxAmplitude = absSample;
        }

        absAmplitude += (unsigned int)absSample;
    }

    /*
     * The original formats "MaxAmp: %d  Avg: %d" into a local buffer here, but
     * the text is never used. Keep the average computation documented instead.
     */
    absAmplitude /= (unsigned int)sndDataSize / (unsigned int)AUDIOLIB_LIPSYNC_SAMPLE_SIZE;
    sprintf_s(aDebugBuffer, STD_ARRAYLEN(aDebugBuffer), "MaxAmp: %d  Avg: %d", maxAmplitude, absAmplitude); // Altered: Replaced sprintf with sprintf_s for safety

    for ( size_t i = 0; i < STD_ARRAYLEN(aRollingMax); ++i )
    {
        aRollingMax[i] = maxAmplitude;
    }

    bytesPerAnalysisFrame = AUDIOLIB_LIPSYNC_SAMPLE_SIZE * sampleRate / analysisRateHz;
    zeroCrossingsPerFrame = bytesPerAnalysisFrame * (uint32_t)totalZeroCrossings / sndDataSize;

    while ( processedBytes + bytesPerAnalysisFrame <= sndDataSize )
    {
        int frameMaxAmplitude = 0;
        int frameZeroCrossings = 0;
        int frameAbsSum = 0;
        int frameAvgAmplitude;
        int rollingMaxSum = 0;
        int rollingAvgMax;
        int rawMouthX;
        int rawMouthY;
        int mouthXPercent;
        int mouthYPercent;
        int mouthX;
        int mouthY;

        for ( uint32_t byteOffset = AUDIOLIB_LIPSYNC_SAMPLE_SIZE; byteOffset < bytesPerAnalysisFrame; byteOffset += AUDIOLIB_LIPSYNC_SAMPLE_SIZE )
        {
            int16_t previousSample = AudioLib_ReadS16LE(pSndData + byteOffset - AUDIOLIB_LIPSYNC_SAMPLE_SIZE);
            int16_t sample = AudioLib_ReadS16LE(pSndData + byteOffset);
            int absSample = AudioLib_AbsS16(sample);

            if ( sample >= 0 && previousSample < 0 )
            {
                ++frameZeroCrossings;
            }

            if ( absSample > frameMaxAmplitude )
            {
                frameMaxAmplitude = absSample;
            }

            frameAbsSum += absSample;
        }

        frameAvgAmplitude = frameAbsSum / (int)(bytesPerAnalysisFrame / AUDIOLIB_LIPSYNC_SAMPLE_SIZE);

        for ( int i = rollingMaxIndex; i < rollingMaxIndex + (int)STD_ARRAYLEN(aRollingMax); ++i )
        {
            rollingMaxSum += aRollingMax[i % (int)STD_ARRAYLEN(aRollingMax)];
        }

        rollingAvgMax = rollingMaxSum / (int)STD_ARRAYLEN(aRollingMax);
        if ( rollingAvgMax == 0 )
        {
            rollingAvgMax = 1;
        }

        if ( rollingAvgMax < maxAmplitude / AUDIOLIB_LIPSYNC_ROLLING_MIN_DIVISOR )
        {
            rollingAvgMax = maxAmplitude / AUDIOLIB_LIPSYNC_ROLLING_MIN_DIVISOR;
        }

        aRollingMax[rollingMaxIndex] = frameMaxAmplitude;
        rollingMaxIndex = (rollingMaxIndex + 1) % (int)STD_ARRAYLEN(aRollingMax);

        rawMouthY = AUDIOLIB_LIPSYNC_MOUTH_Y_SCALE * frameMaxAmplitude / rollingAvgMax;
        rawMouthX = AUDIOLIB_LIPSYNC_MOUTH_X_SCALE * frameZeroCrossings / (int)(zeroCrossingsPerFrame + AUDIOLIB_LIPSYNC_ZERO_CROSSING_BIAS);

        if ( rawMouthY < AUDIOLIB_LIPSYNC_QUIET_THRESHOLD )
        {
            rawMouthX = (AUDIOLIB_LIPSYNC_NEUTRAL_MOUTH_X * (AUDIOLIB_LIPSYNC_QUIET_THRESHOLD - rawMouthY) + rawMouthX * rawMouthY) / AUDIOLIB_LIPSYNC_QUIET_THRESHOLD;
        }

        if ( rawMouthX >= AUDIOLIB_LIPSYNC_PERCENT_SCALE )
        {
            mouthXPercent = AUDIOLIB_LIPSYNC_PERCENT_SCALE;
        }
        else
        {
            mouthXPercent = rawMouthX;
        }

        rawMouthX = AUDIOLIB_LIPSYNC_MOUTH_MAX * mouthXPercent / AUDIOLIB_LIPSYNC_PERCENT_SCALE;
        if ( rawMouthY >= AUDIOLIB_LIPSYNC_PERCENT_SCALE )
        {
            mouthYPercent = AUDIOLIB_LIPSYNC_PERCENT_SCALE;
        }
        else
        {
            mouthYPercent = rawMouthY;
        }

        rawMouthY = AUDIOLIB_LIPSYNC_MOUTH_MAX * mouthYPercent / AUDIOLIB_LIPSYNC_PERCENT_SCALE;

        if ( frameAvgAmplitude < AUDIOLIB_LIPSYNC_TRANSIENT_THRESHOLD_PERCENT * frameMaxAmplitude / AUDIOLIB_LIPSYNC_PERCENT_SCALE )
        {
            if ( previousRawMouthX <= AUDIOLIB_LIPSYNC_MOUTH_X_DECAY_MIN )
            {
                rawMouthX = 0;
            }
            else
            {
                rawMouthX = previousRawMouthX - AUDIOLIB_LIPSYNC_MOUTH_DECAY_STEP;
            }

            if ( previousRawMouthY <= AUDIOLIB_LIPSYNC_MOUTH_Y_DECAY_MIN )
            {
                rawMouthY = 0;
            }
            else
            {
                rawMouthY = previousRawMouthY - AUDIOLIB_LIPSYNC_MOUTH_DECAY_STEP;
            }
        }

        previousRawMouthX = rawMouthX;
        previousRawMouthY = rawMouthY;

        mouthX = rawMouthX & mouthXMask;
        mouthY = rawMouthY & mouthYMask;

        if ( mouthX != previousMouthX || mouthY != previousMouthY )
        {
            AudioLib_WriteU32LE(pCurOut, AUDIOLIB_PACK_LIPSYNC_ENTRY(timeKey, mouthX, mouthY));
            pCurOut += AUDIOLIB_LIPSYNC_ENTRY_SIZE;
            previousMouthX = mouthX;
            previousMouthY = mouthY;
        }

        timeKey += timeStep;
        processedBytes += bytesPerAnalysisFrame;
        pSndData += bytesPerAnalysisFrame;
    }

    AudioLib_WriteU32LE(pCurOut, timeKey & AUDIOLIB_LIPSYNC_TIME_MASK);
    pCurOut += AUDIOLIB_LIPSYNC_ENTRY_SIZE;

    {
        int numEntries = (int)(((pCurOut - pOutData) / AUDIOLIB_LIPSYNC_ENTRY_SIZE) - (AUDIOLIB_LIPSYNC_HEADER_SIZE / AUDIOLIB_LIPSYNC_ENTRY_SIZE));
        AudioLib_WriteU32LE(pOutData + AUDIOLIB_LIPSYNC_MAGIC_LENGTH, numEntries);
        return AUDIOLIB_LIPSYNC_ENTRY_SIZE * numEntries + AUDIOLIB_LIPSYNC_HEADER_SIZE;
    }
}

// ADPCM compression
int J3DAPI AudioLib_CompressBlock(tAudioCompressorState* pCompressorState, uint8_t* pOutBuffer, int16_t* a3, int a4, unsigned int numChannels, int a6, int bStateInitialized)
{
    return J3D_TRAMPOLINE_CALL(AudioLib_CompressBlock, pCompressorState, pOutBuffer, a3, a4, numChannels, a6, bStateInitialized);
}

// ADPCM decompress
void J3DAPI AudioLib_UncompressBlock(tAudioCompressorState* pCompressorState, uint8_t* pOutData, const uint8_t* pInData, int size, unsigned int numChannels, int bStateInited)
{
    J3D_TRAMPOLINE_CALL(AudioLib_UncompressBlock, pCompressorState, pOutData, pInData, size, numChannels, bStateInited);
}

//int J3DAPI AudioLib_WVSMCompressBlock(uint8_t* pOutBuffer, const uint8_t* pInBuffer, int blockSize, FILE* pFile)
//{
//    return J3D_TRAMPOLINE_CALL(AudioLib_WVSMCompressBlock, pOutBuffer, pInBuffer, blockSize, pFile);
//}

int J3DAPI AudioLib_WVSMUncompressBlock(uint8_t* pOutBuffer, const uint8_t* pInBuffer, int blockSize)
{
    return J3D_TRAMPOLINE_CALL(AudioLib_WVSMUncompressBlock, pOutBuffer, pInBuffer, blockSize);
}

int J3DAPI AudioLib_WVSMCompressBlock(uint8_t* pOutBuffer, const uint8_t* pInBuffer, int blockSize, FILE* pFile)
{
    const uint8_t* pInBuffer_1;
    int size_1;
    unsigned int sampleCount;
    int curLeftSamp;
    const uint8_t* v8;
    int i;
    int curRightSamp;
    int j;
    int kRight_1;
    int* pLeft;
    int kLeft;
    int* pRight;
    int rsavedBits;
    int lsavedBits_1;
    int numSlots;
    int* pRight_1;
    int* pLeft_1;
    int v21;
    int leftOffset;
    char kRight_2;
    int* v24;
    int v25;
    int* v26;
    int v27;
    const int16_t* pCurIn;
    uint8_t* pCurOut;
    int curLeftSample;
    const int16_t* pRightIn;
    int leftSampleOffseted;
    int leftSampeCompressed;
    int v34;
    uint8_t* v35;
    int curRightSampe;
    uint8_t* pRightOut;
    int rightSampleOffseted;
    int rightSampleCompressed;
    int v40;
    uint8_t* v41;
    int compressedSize;
    int kRight;
    int v44;
    unsigned int kLeft_1;
    int lsavedBits;
    int v47;
    int rightOffset;
    int leftBits[17];
    int rightBits[17];
    int blockSizea;

    pInBuffer_1 = pInBuffer;

    memset(leftBits, 0, sizeof(leftBits));

    size_1 = blockSize >> 1;
    blockSizea = size_1;

    memset(rightBits, 0, sizeof(rightBits));

    if ( size_1 > 0 )
    {
        int outOfBoundsLeft = 0;
        int outOfBoundsRight = 0;
        sampleCount = (unsigned int)(size_1 + 1) >> 1;

        // Note in debug version of Indy3D.exe the loop is the same only the loop range is different.
        // In debug version the loop range is defined as counter = 0; while(counter < size_1) {... counter += 2; }
        // This change still causes the rightBits or leftBits to be rad out of bounds,
        // so the change must be somewhere else in order for the debug version to run normally
        do
        {
            curLeftSamp = *(int16_t*)pInBuffer_1;
            v8 = pInBuffer_1 + 2;
            if ( curLeftSamp < 0 )
            {
                curLeftSamp = -curLeftSamp;
            }

            for ( i = 0; curLeftSamp; ++i ) // [ADD] Fixed init i value (orig. i = 1) which caused out of bounds access
            {
                curLeftSamp >>= 1;
            }

            pInBuffer_1 = v8 + 2;

            if ( i > 16 ) // [ADD] bound check fix
            {
                i = 16;
                outOfBoundsLeft++;
            }
            ++leftBits[i];

            curRightSamp = *((int16_t*)pInBuffer_1 - 1);
            if ( curRightSamp < 0 )
            {
                curRightSamp = -curRightSamp;
            }

            for ( j = 0; curRightSamp; ++j ) // [ADD] Fixed init j value (orig. j = 1)
            {
                curRightSamp >>= 1;
            }

            --sampleCount;

            if ( j > 16 ) // [ADD] bound check fix
            {
                j = 16;
                outOfBoundsRight++;
            }
            ++rightBits[j];
        } while ( sampleCount );

        if ( outOfBoundsLeft || outOfBoundsRight )
        {
            // TODO: make separate log function var for audio lib
          /*  if (Sound_pHS) {
                Sound_pHS->logWarning("AudioLib_WVSMCompressBlock: %d left bits out of bounds, %d right bits out of bounds\n", outOfBoundsLeft, outOfBoundsRight);
            }*/
        }
    }

    kRight_1 = 8;

    pLeft = &leftBits[16];
    for ( kLeft = 8; kLeft > 0; --kLeft )
    {
        if ( *pLeft )
        {
            break;
        }

        --pLeft;
    }

    v44 = kLeft;

    pRight = &rightBits[16];
    do
    {
        if ( *pRight )
        {
            break;
        }

        --kRight_1;
        --pRight;
    } while ( kRight_1 > 0 );

    rsavedBits = 0;
    lsavedBits_1 = 0;

    kRight = kRight_1;
    numSlots = 192;
    pRight_1 = &rightBits[kRight_1 + 8];
    pLeft_1 = &leftBits[kLeft + 8];
    do
    {
        if ( kLeft <= 0 )
        {
            if ( kRight <= 0 )
            {
                break;
            }

            v21 = 0;
        }

        else if ( kRight <= 0 )
        {
            v21 = 1;
        }
        else
        {
            v21 = *pLeft_1 < *pRight_1;
            kLeft = v44;
        }

        if ( v21 )
        {
            if ( *pLeft_1 > numSlots )
            {
                break;
            }

            numSlots -= *pLeft_1;
            --kLeft;
            --pLeft_1;
            v44 = kLeft;
            ++lsavedBits_1;
        }
        else
        {
            if ( *pRight_1 > numSlots )
            {
                break;
            }

            numSlots -= *pRight_1--;
            --kRight;
            ++rsavedBits;
        }
    } while ( numSlots );

    lsavedBits = lsavedBits_1;
    if ( kLeft )
    {
        leftOffset = 1 << (kLeft - 1);
        v47 = leftOffset;
    }
    else
    {
        v47 = 0;
        leftOffset = 0;
    }

    kRight_2 = kRight;
    if ( kRight )
    {
        rightOffset = 1 << (kRight - 1);
    }
    else
    {
        rightOffset = 0;
    }

    if ( pFile )
    {
        fprintf(pFile, "L = %4d:", kLeft);
        v24 = leftBits;
        v25 = 16;
        do
        {
            fprintf(pFile, "%4d,", *v24++);
            --v25;
        } while ( v25 );

        fprintf(pFile, "%4d\n", leftBits[16]);

        fprintf(pFile, "R = %4d:", kRight);
        v26 = rightBits;
        v27 = 16;
        do
        {
            fprintf(pFile, "%4d,", *v26++);
            --v27;
        } while ( v27 );

        fprintf(pFile, "%4d\n", rightBits[16]);

        fprintf(pFile, "%d escape slots used, %dL/%dR bits saved\n\n", 192 - numSlots, lsavedBits, rsavedBits);
        kLeft |= (v44 & 0xff);
        leftOffset = v47;
        kRight_2 = kRight;
    }

    pCurIn = (const int16_t*)pInBuffer;
    *pOutBuffer = 0;
    pOutBuffer[2] = kRight_2 + 16 * kLeft;      // assign sample expander kRight_2 | (kLeft << 4)
    pCurOut = pOutBuffer + 3;

    // compressing block
    if ( blockSizea > 0 )
    {
        kLeft_1 = (unsigned int)(blockSizea + 1) >> 1;// TODO: make new int var
        while ( 1 )
        {
            curLeftSample = *pCurIn;
            pRightIn = pCurIn + 1;
            if ( curLeftSample >= 0 )
            {
                leftSampleOffseted = leftOffset + curLeftSample;
            }
            else
            {
                leftSampleOffseted = curLeftSample - leftOffset;
            }

            if ( leftSampleOffseted < -32767 )
            {
                leftSampleOffseted = -32767;
            }

            if ( leftSampleOffseted > 32767 )
            {
                leftSampleOffseted = 32767;
            }

            leftSampeCompressed = leftSampleOffseted >> kLeft;
            v34 = leftSampeCompressed << kLeft; // leftSampleOffseted could be used instead
            if ( leftSampeCompressed > 127 || leftSampeCompressed < -127 )
            {
                *pCurOut = 0x80;
                v35 = pCurOut + 1;
                *v35 = (v34 >> 8) & 0xff;
                pCurOut = v35 + 1;
                *pCurOut = v34;
            }
            else
            {
                *pCurOut = leftSampeCompressed;
            }

            curRightSampe = *pRightIn;
            pRightOut = pCurOut + 1;
            pCurIn = pRightIn + 1;
            if ( curRightSampe >= 0 )
            {
                rightSampleOffseted = rightOffset + curRightSampe;
            }
            else
            {
                rightSampleOffseted = curRightSampe - rightOffset;
            }

            if ( rightSampleOffseted < -32767 )
            {
                rightSampleOffseted = -32767;
            }

            if ( rightSampleOffseted > 32767 )
            {
                rightSampleOffseted = 32767;
            }

            rightSampleCompressed = rightSampleOffseted >> kRight;
            v40 = rightSampleCompressed << kRight;// rightSampleOffseted could be used instead
            if ( rightSampleCompressed > 127 || rightSampleCompressed < -127 )
            {
                *pRightOut = 0x80;
                v41 = pRightOut + 1;
                *v41 = (v40 >> 8) & 0xff;
                pRightOut = v41 + 1;
                *pRightOut = v40;
            }
            else
            {
                *pRightOut = rightSampleCompressed;
            }

            pCurOut = pRightOut + 1;
            if ( !--kLeft_1 )
            {
                break;
            }

            leftOffset = v47;
        }
    }

    compressedSize = pCurOut - pOutBuffer;

    // assign be compressed size to the begin of out block
    *pOutBuffer = (uint16_t)((uint16_t)pCurOut - (uint16_t)pOutBuffer - 2) >> 8;
    pOutBuffer[1] = (uint8_t)pCurOut - (uint8_t)pOutBuffer - 2;

    return compressedSize;
}
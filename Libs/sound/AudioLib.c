#include "AudioLib.h"
#include <j3dcore/j3d.h>
#include <j3dcore/j3dhook.h>
#include <sound/RTI/symbols.h>
#include <std/General/stdMath.h>
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

// ADPCM codec constants
#define AUDIOLIB_NUM_STEPS                  89
#define AUDIOLIB_MAX_STEP_INDEX             (AUDIOLIB_NUM_STEPS - 1)
#define AUDIOLIB_DELTA_CODE_BITS            6
#define AUDIOLIB_DELTA_CODE_COUNT           (1 << AUDIOLIB_DELTA_CODE_BITS)
#define AUDIOLIB_DELTA_TABLE_SIZE           (AUDIOLIB_NUM_STEPS * AUDIOLIB_DELTA_CODE_COUNT)
#define AUDIOLIB_MAX_STEP_BITS              7
#define AUDIOLIB_BITBUFFER_BITS             16
#define AUDIOLIB_ADPCM_CHANNEL_BIAS         2 // Compress: a channel argument above 2 selects ADPCM and is the channel count plus 2.
#define AUDIOLIB_SAMPLE_MIN                 (-32768)
#define AUDIOLIB_SAMPLE_MAX                 32767

// WVSM codec constants
#define AUDIOLIB_WVSM_MAGIC                 "WVSM"
#define AUDIOLIB_WVSM_BLOCK_SIZE            0x1000
#define AUDIOLIB_WVSM_ESCAPE                0x80 // Marks a raw 16-bit sample in a WVSM block.
#define AUDIOLIB_WVSM_BLOCK_HEADER_SIZE     2    // Compressed block size, big endian.
#define AUDIOLIB_WVSM_STEREO_STEP_INDEX     0x64
#define AUDIOLIB_WVSM_STEREO_PREV_SAMPLE0   0x1111
#define AUDIOLIB_WVSM_STEREO_PREV_SAMPLE1   0x2222

// Sound file types returned by AudioLib_ParseWaveFileHeader
#define AUDIOLIB_FILETYPE_UNKNOWN           0
#define AUDIOLIB_FILETYPE_IMUS              1
#define AUDIOLIB_FILETYPE_IMUS_COMPRESSED   2
#define AUDIOLIB_FILETYPE_WAV               3
#define AUDIOLIB_FILETYPE_WAV_COMPRESSED    4
#define AUDIOLIB_FILETYPE_AIFF              5
#define AUDIOLIB_FILETYPE_INDYWV            6

// Sound file format constants
#define AUDIOLIB_MCMP_MAGIC                 "MCMP"
#define AUDIOLIB_MCMP_ENTRY_SIZE            9
#define AUDIOLIB_INDYWV_MAGIC               "INDYWV"
#define AUDIOLIB_RIFF_MAGIC                 "RIFF"
#define AUDIOLIB_RIFF_WAVEFMT_MAGIC         "WAVEfmt "
#define AUDIOLIB_RIFF_SYNC_CHUNK            "sync"
#define AUDIOLIB_RIFF_DATA_CHUNK            "data"
#define AUDIOLIB_RIFF_FORMAT_PCM            1
#define AUDIOLIB_IMUS_MAGIC                 "iMUS"
#define AUDIOLIB_IMUS_FRMT_MAGIC            "FRMT"
#define AUDIOLIB_AIFF_MAGIC                 "FORM"
#define AUDIOLIB_AIFF_COMM_MAGIC            "AIFFCOMM"
#define AUDIOLIB_AIFF_SSND_CHUNK            "SSND"
#define AUDIOLIB_AIFF_RATE_EXP_44100        14 // Low byte of the 80-bit extended sample rate exponent
#define AUDIOLIB_AIFF_RATE_EXP_11025        12
#define AUDIOLIB_AIFF_RATE_44100            44100
#define AUDIOLIB_AIFF_RATE_22050            22050
#define AUDIOLIB_AIFF_RATE_11025            11025

// Compressed data header of the stereo WVSM stream written by AudioLib_Compress
static const uint8_t AudioLib_aWVSMHeader[10] = { 0xE4, 0x11, 0x11, 0x64, 0x22, 0x22, 'W', 'V', 'S', 'M' };

static const int16_t AudioLib_aStepTable[AUDIOLIB_NUM_STEPS] = {
    7, 8, 9, 10, 11, 12, 13, 14, 16, 17,
    19, 21, 23, 25, 28, 31, 34, 37, 41, 45,
    50, 55, 60, 66, 73, 80, 88, 97, 107, 118,
    130, 143, 157, 173, 190, 209, 230, 253, 279, 307,
    337, 371, 408, 449, 494, 544, 598, 658, 724, 796,
    876, 963, 1060, 1166, 1282, 1411, 1552, 1707, 1878, 2066,
    2272, 2499, 2749, 3024, 3327, 3660, 4026, 4428, 4871, 5358,
    5894, 6484, 7132, 7845, 8630, 9493, 10442, 11487, 12635, 13899,
    15289, 16818, 18500, 20350, 22385, 24623, 27086, 29794, 32767
};

// Number of bits of an encoded sample per step index
static const uint8_t AudioLib_aStepBits[AUDIOLIB_NUM_STEPS] = {
    4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
    4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
    4, 4, 4, 4, 4, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 6,
    6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 7, 7, 7, 7, 7, 7,
    7, 7, 7, 7, 7, 7, 7, 7, 7
};

// Step index adjustments indexed by the encoded magnitude, one table per sample bit count
static const int8_t AudioLib_aIndex2Bit[4] = { -1, 4, -1, 4 };
static const int8_t AudioLib_aIndex3Bit[8] = { -1, -1, 2, 6, -1, -1, 2, 6 };
static const int8_t AudioLib_aIndex4Bit[16] = { -1, -1, -1, -1, 1, 2, 4, 6, -1, -1, -1, -1, 1, 2, 4, 6 };
static const int8_t AudioLib_aIndex5Bit[32] = {
    -1, -1, -1, -1, -1, -1, -1, -1, 1, 1, 1, 2, 2, 4, 5, 6,
    -1, -1, -1, -1, -1, -1, -1, -1, 1, 1, 1, 2, 2, 4, 5, 6
};
static const int8_t AudioLib_aIndex6Bit[64] = {
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    1, 1, 1, 1, 1, 2, 2, 2, 2, 4, 4, 4, 5, 5, 6, 6,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    1, 1, 1, 1, 1, 2, 2, 2, 2, 4, 4, 4, 5, 5, 6, 6
};
static const int8_t AudioLib_aIndex7Bit[128] = {
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2,
    2, 2, 4, 4, 4, 4, 4, 4, 5, 5, 5, 5, 6, 6, 6, 6,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2,
    2, 2, 4, 4, 4, 4, 4, 4, 5, 5, 5, 5, 6, 6, 6, 6
};

static const int8_t* const AudioLib_aIndexTableTable[AUDIOLIB_MAX_STEP_BITS + 1] = {
    NULL,
    NULL,
    AudioLib_aIndex2Bit,
    AudioLib_aIndex3Bit,
    AudioLib_aIndex4Bit,
    AudioLib_aIndex5Bit,
    AudioLib_aIndex6Bit,
    AudioLib_aIndex7Bit
};

// Step delta lookup table indexed by [stepIndex * 64 + code], built on first use by AudioLib_UncompressBlock
static uint16_t AudioLib_aDeltaTable[AUDIOLIB_DELTA_TABLE_SIZE];
static int AudioLib_bDeltaTableInitialized = 0;

void AudioLib_InstallHooks(void)
{
    // Uncomment only lines for functions that have full definition and doesn't call original function (non-thunk functions)

    J3D_HOOKFUNC(AudioLib_ParseWaveFileHeader);
    J3D_HOOKFUNC(AudioLib_Compress);
    J3D_HOOKFUNC(AudioLib_ResetCompressor);
    J3D_HOOKFUNC(AudioLib_Uncompress);
    J3D_HOOKFUNC(AudioLib_GetMouthPosition);
    J3D_HOOKFUNC(AudioLib_GenerateLipSyncBlock);
    J3D_HOOKFUNC(AudioLib_CompressBlock);
    J3D_HOOKFUNC(AudioLib_UncompressBlock);
    J3D_HOOKFUNC(AudioLib_WVSMCompressBlock);
    J3D_HOOKFUNC(AudioLib_WVSMUncompressBlock);
}

void AudioLib_ResetGlobals(void)
{}

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

// Reads a 16-bit big-endian value.
static inline uint16_t AudioLib_ReadU16BE(const uint8_t* pData)
{
    return (uint16_t)((pData[0] << AUDIOLIB_BITS_PER_BYTE) | pData[1]);
}

// Reads a 32-bit big-endian value.
static inline uint32_t AudioLib_ReadU32BE(const uint8_t* pData)
{
    return ((uint32_t)pData[0] << (AUDIOLIB_BITS_PER_BYTE * 3))
        | ((uint32_t)pData[1] << (AUDIOLIB_BITS_PER_BYTE * 2))
        | ((uint32_t)pData[2] << AUDIOLIB_BITS_PER_BYTE)
        | (uint32_t)pData[3];
}

// Writes a 16-bit big-endian value.
static inline void AudioLib_WriteU16BE(uint8_t* pData, uint16_t value)
{
    pData[0] = (uint8_t)(value >> AUDIOLIB_BITS_PER_BYTE);
    pData[1] = (uint8_t)value;
}

// Swaps the bytes of a 16-bit value.
static inline uint16_t AudioLib_SwapU16(uint16_t value)
{
    return (uint16_t)((value << AUDIOLIB_BITS_PER_BYTE) | (value >> AUDIOLIB_BITS_PER_BYTE));
}

// Returns pointer to snd data
const uint8_t* J3DAPI AudioLib_ParseWaveFileHeader(const uint8_t* pData, int* pType, uint32_t* pSampleRate, uint32_t* pBitsPerSample, uint32_t* pNumChannels, uint32_t* pExtraInfo, uint32_t* pSoundDataSize, uint32_t* pExtraDataOffset, uint32_t* pSoundDataOffset)
{
    bool bCompressed = false;
    if ( pType )
    {
        *pType = AUDIOLIB_FILETYPE_UNKNOWN;
    }

    if ( !pData )
    {
        return NULL;
    }

    if ( pExtraDataOffset )
    {
        *pExtraDataOffset = 0;
    }

    if ( pSoundDataOffset )
    {
        *pSoundDataOffset = 0;
    }

    // Skip the MCMP container header: the index of entries followed by a size prefixed block
    if ( memcmp(pData, AUDIOLIB_MCMP_MAGIC, sizeof(AUDIOLIB_MCMP_MAGIC) - 1) == 0 )
    {
        uint16_t count = AudioLib_ReadU16BE(pData + 4);
        pData += (AUDIOLIB_MCMP_ENTRY_SIZE * count) + 6;

        count = AudioLib_ReadU16BE(pData);
        pData += count + 2;
        bCompressed = true;
    }

    if ( memcmp(pData, AUDIOLIB_INDYWV_MAGIC, sizeof(AUDIOLIB_INDYWV_MAGIC) - 1) == 0 )
    {
        if ( pType )
        {
            *pType = AUDIOLIB_FILETYPE_INDYWV;
        }

        if ( pSampleRate )
        {
            *pSampleRate = AudioLib_ReadU32LE(pData + 6);
        }

        if ( pBitsPerSample )
        {
            *pBitsPerSample = AudioLib_ReadU32LE(pData + 10);
        }

        if ( pNumChannels )
        {
            *pNumChannels = AudioLib_ReadU32LE(pData + 14);
        }

        if ( pExtraInfo )
        {
            *pExtraInfo = AudioLib_ReadU32LE(pData + 18);
        }

        if ( pSoundDataSize )
        {
            *pSoundDataSize = AudioLib_ReadU32LE(pData + 18);
        }

        uint32_t extraDataSize = 0;
        if ( pExtraDataOffset && pSoundDataOffset )
        {
            extraDataSize = AudioLib_ReadU32LE(pData + 22);
            *pSoundDataOffset = extraDataSize;
            if ( *pSoundDataOffset )
            {
                *pExtraDataOffset = (uint32_t)(uintptr_t)(pData + 26);
            }
            else
            {
                *pExtraDataOffset = 0;
            }
        }

        return pData + 26 + extraDataSize;
    }

    if ( memcmp(pData, AUDIOLIB_RIFF_MAGIC, sizeof(AUDIOLIB_RIFF_MAGIC) - 1) == 0 )
    {
        const uint8_t* pEnd = pData + AudioLib_ReadU32LE(pData + 4) + 8;
        if ( memcmp(pData + 8, AUDIOLIB_RIFF_WAVEFMT_MAGIC, sizeof(AUDIOLIB_RIFF_WAVEFMT_MAGIC) - 1) != 0 )
        {
            return NULL;
        }

        if ( AudioLib_ReadS16LE(pData + 20) != AUDIOLIB_RIFF_FORMAT_PCM )
        {
            return NULL;
        }

        if ( pType )
        {
            *pType = bCompressed ? AUDIOLIB_FILETYPE_WAV_COMPRESSED : AUDIOLIB_FILETYPE_WAV;
        }

        pData += 16;
        uint32_t size = AudioLib_ReadU32LE(pData);
        if ( pSampleRate )
        {
            *pSampleRate = AudioLib_ReadU32LE(pData + 8);
        }

        if ( pBitsPerSample )
        {
            *pBitsPerSample = AudioLib_ReadS16LE(pData + 18);
        }

        if ( pNumChannels )
        {
            *pNumChannels = AudioLib_ReadS16LE(pData + 6);
        }

        if ( pExtraInfo )
        {
            *pExtraInfo = 1;
        }

        // Walk the chunks until the data chunk, remembering the sync chunk
        do
        {
            pData += size + 8;
            size = AudioLib_ReadU32LE(pData);
            if ( memcmp(pData - 4, AUDIOLIB_RIFF_SYNC_CHUNK, sizeof(AUDIOLIB_RIFF_SYNC_CHUNK) - 1) == 0 && pExtraDataOffset && pSoundDataOffset )
            {
                *pExtraDataOffset = (uint32_t)(uintptr_t)(pData + 4);
                *pSoundDataOffset = size;
            }
        } while ( pData < pEnd && memcmp(pData - 4, AUDIOLIB_RIFF_DATA_CHUNK, sizeof(AUDIOLIB_RIFF_DATA_CHUNK) - 1) != 0 );

        if ( pData < pEnd )
        {
            if ( pSoundDataSize )
            {
                *pSoundDataSize = size;
            }

            return pData + 4;
        }

        return NULL;
    }

    if ( memcmp(pData, AUDIOLIB_IMUS_MAGIC, sizeof(AUDIOLIB_IMUS_MAGIC) - 1) == 0 )
    {
        uint32_t size = AudioLib_ReadU32BE(pData + 12);
        if ( memcmp(pData + 16, AUDIOLIB_IMUS_FRMT_MAGIC, sizeof(AUDIOLIB_IMUS_FRMT_MAGIC) - 1) != 0 )
        {
            return NULL;
        }

        if ( pType )
        {
            *pType = bCompressed ? AUDIOLIB_FILETYPE_IMUS_COMPRESSED : AUDIOLIB_FILETYPE_IMUS;
        }

        pData += 32;
        if ( pBitsPerSample )
        {
            *pBitsPerSample = AudioLib_ReadU32BE(pData);
        }

        if ( pSampleRate )
        {
            *pSampleRate = AudioLib_ReadU32BE(pData + 4);
        }

        if ( pNumChannels )
        {
            *pNumChannels = AudioLib_ReadU32BE(pData + 8);
        }

        if ( pExtraInfo )
        {
            *pExtraInfo = 1;
        }

        if ( pSoundDataSize )
        {
            *pSoundDataSize = AudioLib_ReadU32BE(&pData[size - 12]);
        }

        return pData + size - 8;
    }

    if ( bCompressed )
    {
        return NULL;
    }

    if ( memcmp(pData, AUDIOLIB_AIFF_MAGIC, sizeof(AUDIOLIB_AIFF_MAGIC) - 1) == 0 )
    {
        const uint8_t* pEnd = pData + AudioLib_ReadU32BE(pData + 4) + 8;
        if ( memcmp(pData + 8, AUDIOLIB_AIFF_COMM_MAGIC, sizeof(AUDIOLIB_AIFF_COMM_MAGIC) - 1) != 0 )
        {
            return NULL;
        }

        if ( pType )
        {
            *pType = AUDIOLIB_FILETYPE_AIFF;
        }

        pData += 16;
        uint32_t size = AudioLib_ReadU32BE(pData);
        if ( pSampleRate )
        {
            // Only the exponent byte of the extended sample rate is examined
            if ( pData[13] == AUDIOLIB_AIFF_RATE_EXP_44100 )
            {
                *pSampleRate = AUDIOLIB_AIFF_RATE_44100;
            }
            else if ( pData[13] == AUDIOLIB_AIFF_RATE_EXP_11025 )
            {
                *pSampleRate = AUDIOLIB_AIFF_RATE_11025;
            }
            else
            {
                *pSampleRate = AUDIOLIB_AIFF_RATE_22050;
            }
        }

        if ( pBitsPerSample )
        {
            *pBitsPerSample = pData[11];
        }

        if ( pNumChannels )
        {
            *pNumChannels = pData[5];
        }

        if ( pExtraInfo )
        {
            *pExtraInfo = 0;
        }

        // Walk the chunks until the sound data chunk
        do
        {
            pData += size + 8;
            size = AudioLib_ReadU32BE(pData);
        } while ( pData < pEnd && memcmp(pData - 4, AUDIOLIB_AIFF_SSND_CHUNK, sizeof(AUDIOLIB_AIFF_SSND_CHUNK) - 1) != 0 );

        if ( pData < pEnd )
        {
            if ( pSoundDataSize )
            {
                *pSoundDataSize = size - 8;
            }

            return pData + 12;
        }

        return NULL;
    }

    return NULL;
}

int J3DAPI AudioLib_Compress(tAudioCompressorState* pCompressorState, uint8_t* pOutBuffer, const uint8_t* pInBuffer, int size, unsigned int numChannels)
{
    if ( numChannels > AUDIOLIB_ADPCM_CHANNEL_BIAS )
    {
        // ADPCM stream: the compressor state precedes the compressed block
        numChannels -= AUDIOLIB_ADPCM_CHANNEL_BIAS;

        *pOutBuffer++ = numChannels > 1 ? ~pCompressorState->aStepIndex[0] : pCompressorState->aStepIndex[0]; // Negative step index marks stereo
        AudioLib_WriteU16BE(pOutBuffer, (uint16_t)pCompressorState->aPrevSample[0]);
        pOutBuffer += sizeof(uint16_t);

        if ( numChannels > 1 )
        {
            *pOutBuffer++ = pCompressorState->aStepIndex[1];
            AudioLib_WriteU16BE(pOutBuffer, (uint16_t)pCompressorState->aPrevSample[1]);
            pOutBuffer += sizeof(uint16_t);
        }

        return (3 * numChannels) + AudioLib_CompressBlock(pCompressorState, pOutBuffer, (int16_t*)pInBuffer, size / (2 * numChannels), numChannels, 1, 1);
    }

    // WVSM stream
    uint8_t* pStart = pOutBuffer;
    memcpy(pOutBuffer, AudioLib_aWVSMHeader, sizeof(AudioLib_aWVSMHeader));
    pOutBuffer += sizeof(AudioLib_aWVSMHeader);

    while ( size )
    {
        unsigned int blockSize = AUDIOLIB_WVSM_BLOCK_SIZE;
        if ( (unsigned int)size > AUDIOLIB_WVSM_BLOCK_SIZE )
        {
            size -= AUDIOLIB_WVSM_BLOCK_SIZE;
        }
        else
        {
            blockSize = size;
            size = 0;
        }

        pOutBuffer += AudioLib_WVSMCompressBlock(pOutBuffer, pInBuffer, blockSize, NULL);
        pInBuffer += AUDIOLIB_WVSM_BLOCK_SIZE;
    }

    // TODO: [BUG] The header size is added to the size of the output which already includes the header, so the returned size is 10 bytes too large.
    return (int)(pOutBuffer + sizeof(AudioLib_aWVSMHeader) - pStart);
}

void J3DAPI AudioLib_ResetCompressor(tAudioCompressorState* pState)
{
    pState->aStepIndex[0] = pState->aStepIndex[1] = 0;
    pState->aPrevSample[0] = pState->aPrevSample[1] = 0;
}

void J3DAPI AudioLib_Uncompress(tAudioCompressorState* pCompressorState, uint8_t* pOutSndData, const uint8_t* pCompressedData, unsigned int size)
{
    unsigned int numChannels = 1;
    pCompressorState->aStepIndex[0] = *pCompressedData++;
    if ( pCompressorState->aStepIndex[0] < 0 )
    {
        pCompressorState->aStepIndex[0] = ~pCompressorState->aStepIndex[0];
        ++numChannels;
    }

    pCompressorState->aPrevSample[0] = AudioLib_ReadU16BE(pCompressedData);
    pCompressedData += sizeof(uint16_t);

    if ( numChannels > 1 )
    {
        pCompressorState->aStepIndex[1] = *pCompressedData++;
        pCompressorState->aPrevSample[1] = AudioLib_ReadU16BE(pCompressedData);
        pCompressedData += sizeof(uint16_t);
    }

    // Stereo header with the fixed WVSM signature values
    if ( numChannels == 2
        && pCompressorState->aPrevSample[0] == AUDIOLIB_WVSM_STEREO_PREV_SAMPLE0
        && pCompressorState->aStepIndex[1] == AUDIOLIB_WVSM_STEREO_STEP_INDEX
        && pCompressorState->aPrevSample[1] == AUDIOLIB_WVSM_STEREO_PREV_SAMPLE1
        && memcmp(pCompressedData, AUDIOLIB_WVSM_MAGIC, sizeof(AUDIOLIB_WVSM_MAGIC) - 1) == 0 )
    {
        pCompressedData += sizeof(AUDIOLIB_WVSM_MAGIC) - 1;
        while ( size )
        {
            unsigned int blockSize = AUDIOLIB_WVSM_BLOCK_SIZE;
            if ( size > AUDIOLIB_WVSM_BLOCK_SIZE )
            {
                size -= AUDIOLIB_WVSM_BLOCK_SIZE;
            }
            else
            {
                blockSize = size;
                size = 0;
            }

            pCompressedData += AudioLib_WVSMUncompressBlock(pOutSndData, pCompressedData, blockSize);
            pOutSndData += AUDIOLIB_WVSM_BLOCK_SIZE;
        }
    }
    else
    {
        AudioLib_UncompressBlock(pCompressorState, pOutSndData, pCompressedData, size / (2 * numChannels), numChannels, 1);
    }
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
int J3DAPI AudioLib_CompressBlock(tAudioCompressorState* pCompressorState, uint8_t* pOutBuffer, int16_t* pInBuffer, int numSamples, unsigned int numChannels, int bNativeByteOrder, int bStateInitialized)
{
    uint8_t* pCurOut   = pOutBuffer;
    int bitPos         = 0;
    uint16_t bitBuffer = 0;

    if ( !bStateInitialized )
    {
        AudioLib_ResetCompressor(pCompressorState);
    }

    for ( unsigned int channel = 0; channel < numChannels; ++channel )
    {
        int16_t* pSample = &pInBuffer[channel];
        int index        = pCompressorState->aStepIndex[channel];
        int predSample   = pCompressorState->aPrevSample[channel];

        for ( unsigned int i = 0; i < (unsigned int)numSamples; ++i, pSample += numChannels )
        {
            // TODO: [BUG] The sample is not sign-extended when the input is byte swapped (bNativeByteOrder == 0), so negative samples are coded as large positive ones.
            int sample = bNativeByteOrder ? *pSample : AudioLib_SwapU16(*(uint16_t*)pSample);

            int step        = AudioLib_aStepTable[index];
            int diff        = sample - predSample;
            uint8_t numBits = AudioLib_aStepBits[index];
            int signBit     = 1 << (numBits - 1);
            uint8_t escape  = (uint8_t)(signBit - 1);

            uint8_t sign = 0;
            if ( diff < 0 )
            {
                sign = (uint8_t)signBit;
                diff = -diff;
            }

            // Quantize the difference to the step, one bit at a time
            uint8_t code = 0;
            int delta    = 0;
            for ( int bit = signBit >> 1; bit != 0; bit >>= 1 )
            {
                if ( diff >= step )
                {
                    code |= bit;
                    diff -= step;
                    delta += step;
                }

                step >>= 1;
            }

            if ( code )
            {
                delta += step;
            }

            uint8_t freeBits = 8 - (bitPos & 7);
            bitBuffer <<= numBits;
            bitBuffer |= code | sign;
            bitPos += numBits;
            if ( numBits >= freeBits )
            {
                *pCurOut++ = (uint8_t)(bitBuffer >> (numBits - freeBits));
            }

            if ( code == escape )
            {
                // Escape code: store the raw 16-bit sample
                predSample = sample;
                bitBuffer <<= 8;
                bitBuffer |= (predSample >> 8) & 0xFF;
                *pCurOut++ = (uint8_t)(bitBuffer >> (bitPos & 7));
                bitBuffer <<= 8;
                bitBuffer |= predSample & 0xFF;
                *pCurOut++ = (uint8_t)(bitBuffer >> (bitPos & 7));
            }
            else
            {
                predSample += sign ? -delta : delta;
                predSample = STDMATH_CLAMP(predSample, AUDIOLIB_SAMPLE_MIN, AUDIOLIB_SAMPLE_MAX);
            }

            index += AudioLib_aIndexTableTable[numBits][code];
            index = STDMATH_CLAMP(index, 0, AUDIOLIB_MAX_STEP_INDEX);
        }

        pCompressorState->aStepIndex[channel]  = (int8_t)index;
        pCompressorState->aPrevSample[channel] = (int16_t)predSample;
    }

    if ( bitPos & 7 )
    {
        bitBuffer <<= 8 - (bitPos & 7);
        *pCurOut++ = bitBuffer & 0xFF;
    }

    return (int)(pCurOut - pOutBuffer);
}

static void AudioLib_InitDeltaTable(void)
{
    // For each step and code, sum the step fractions selected by the code bits
    for ( unsigned int code = 0; code < AUDIOLIB_DELTA_CODE_COUNT; ++code )
    {
        for ( unsigned int stepIdx = 0; stepIdx < AUDIOLIB_NUM_STEPS; ++stepIdx )
        {
            uint16_t step = AudioLib_aStepTable[stepIdx];
            int delta     = 0;
            for ( unsigned int bit = AUDIOLIB_DELTA_CODE_COUNT >> 1; bit != 0; bit >>= 1 )
            {
                if ( code & bit )
                {
                    delta += step;
                }

                step >>= 1;
            }

            AudioLib_aDeltaTable[code + stepIdx * AUDIOLIB_DELTA_CODE_COUNT] = (uint16_t)delta;
        }
    }

    AudioLib_bDeltaTableInitialized = 1;
}

// ADPCM decompress
void J3DAPI AudioLib_UncompressBlock(tAudioCompressorState* pCompressorState, uint8_t* pOutData, const uint8_t* pInData, int numSamples, unsigned int numChannels, int bStateInitialized)
{
    if ( !AudioLib_bDeltaTableInitialized )
    {
        AudioLib_InitDeltaTable();
    }

    if ( !bStateInitialized )
    {
        AudioLib_ResetCompressor(pCompressorState);
    }

    const uint8_t* pIn = pInData;
    int bitPos         = 0;

    // The bit buffer holds the 16 bits around the current read position
    uint16_t bitBuffer = *pIn++;
    bitBuffer <<= 8;
    bitBuffer |= *pIn++;

    // Channels are stored one after another and decoded into interleaved output
    for ( unsigned int channel = 0; channel < numChannels; ++channel )
    {
        int index      = pCompressorState->aStepIndex[channel];
        int predSample = pCompressorState->aPrevSample[channel];
        int16_t* pOut  = (int16_t*)pOutData + channel;

        for ( int numSamplesLeft = numSamples; numSamplesLeft != 0; --numSamplesLeft )
        {
            uint8_t numBits = AudioLib_aStepBits[index];
            int signBit     = 1 << (numBits - 1);
            uint8_t escape  = (uint8_t)(signBit - 1);

            bitPos += numBits;
            unsigned int code = (bitBuffer >> (AUDIOLIB_BITBUFFER_BITS - bitPos)) & ((1u << numBits) - 1u);
            if ( bitPos > 7 )
            {
                bitPos -= 8;
                bitBuffer = (uint16_t)((bitBuffer << 8) | *pIn++);
            }

            bool bNegative = (code & signBit) != 0;
            if ( bNegative )
            {
                code ^= signBit;
            }

            int sample;
            if ( code == escape )
            {
                // Escape code: the raw 16-bit sample follows in the stream
                uint16_t nextBuffer = (uint16_t)((bitBuffer << 8) | *pIn++);
                uint8_t lowByte     = (uint8_t)(nextBuffer >> (8 - bitPos));
                sample              = (int16_t)(((bitBuffer << bitPos) & 0xFF00u) | lowByte);
                bitBuffer           = (uint16_t)((nextBuffer << 8) | *pIn++);
            }
            else
            {
                int delta = AudioLib_aDeltaTable[(code << (AUDIOLIB_MAX_STEP_BITS - numBits)) | (index << AUDIOLIB_DELTA_CODE_BITS)];
                if ( code )
                {
                    delta += AudioLib_aStepTable[index] >> (numBits - 1);
                }

                sample = bNegative ? predSample - delta : predSample + delta;
                sample = STDMATH_CLAMP(sample, AUDIOLIB_SAMPLE_MIN, AUDIOLIB_SAMPLE_MAX);
            }

            predSample = sample;
            *pOut      = (int16_t)sample;
            pOut += numChannels;

            index += AudioLib_aIndexTableTable[numBits][code];
            index = STDMATH_CLAMP(index, 0, AUDIOLIB_MAX_STEP_INDEX);
        }

        pCompressorState->aStepIndex[channel]  = (int8_t)index;
        pCompressorState->aPrevSample[channel] = (int16_t)predSample;
    }
}

int J3DAPI AudioLib_WVSMUncompressBlock(uint8_t* pOutBuffer, const uint8_t* pInBuffer, int blockSize)
{
    const uint8_t* pIn = pInBuffer;
    int16_t* pOut      = (int16_t*)pOutBuffer;
    int numSamples     = (blockSize & ~1) >> 1;

    pIn += AUDIOLIB_WVSM_BLOCK_HEADER_SIZE; // Skip compressed block size

    // Per channel shifts applied to the stored 8-bit samples
    int shifts     = *pIn++;
    int leftShift  = shifts >> 4;
    int rightShift = shifts & 0xF;

    for ( int i = 0; i < numSamples; i += 2 )
    {
        int left = *pIn++;
        if ( left == AUDIOLIB_WVSM_ESCAPE )
        {
            int value = *pIn++ << 8;
            *pOut++   = (int16_t)(value + *pIn++);
        }
        else
        {
            left    = (int8_t)left;
            *pOut++ = (int16_t)(left * (1 << leftShift));
        }

        if ( i + 1 == numSamples )
        {
            break;
        }

        int right = *pIn++;
        if ( right == AUDIOLIB_WVSM_ESCAPE )
        {
            int value = *pIn++ << 8;
            *pOut++   = (int16_t)(value + *pIn++);
        }
        else
        {
            right   = (int8_t)right;
            *pOut++ = (int16_t)(right * (1 << rightShift));
        }
    }

    return (int)(pIn - pInBuffer);
}

//int J3DAPI AudioLib_WVSMCompressBlock(uint8_t* pOutBuffer, const uint8_t* pInBuffer, int blockSize, FILE* pFile)
//{
//    return J3D_TRAMPOLINE_CALL(AudioLib_WVSMCompressBlock, pOutBuffer, pInBuffer, blockSize, pFile);
//}

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
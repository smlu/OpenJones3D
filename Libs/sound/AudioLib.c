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
#define AUDIOLIB_WVSM_MAX_SHIFT             8    // Largest sample shift of a channel.
#define AUDIOLIB_WVSM_NUM_BIT_LENGTHS       17   // Entries of the sample bit length histogram of the original.
#define AUDIOLIB_WVSM_HISTOGRAM_SIZE        (AUDIOLIB_WVSM_NUM_BIT_LENGTHS + 1)
#define AUDIOLIB_WVSM_MAX_ESCAPES           192  // Number of escaped samples allowed per block when lowering the shifts.
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
#define AUDIOLIB_INDYWV_SAMPLERATE_OFFSET   6  // INDYWV header layout: sample rate, bits per sample, channels, sound data size (also the extra info), extra data size
#define AUDIOLIB_INDYWV_BITS_OFFSET         10
#define AUDIOLIB_INDYWV_CHANNELS_OFFSET     14
#define AUDIOLIB_INDYWV_DATASIZE_OFFSET     18
#define AUDIOLIB_INDYWV_EXTRASIZE_OFFSET    22
#define AUDIOLIB_INDYWV_HEADER_SIZE         26 // Extra data follows the header, then the sound data
#define AUDIOLIB_RIFF_MAGIC                 "RIFF"
#define AUDIOLIB_RIFF_WAVEFMT_MAGIC         "WAVEfmt "
#define AUDIOLIB_RIFF_SYNC_CHUNK            "sync"
#define AUDIOLIB_RIFF_DATA_CHUNK            "data"
#define AUDIOLIB_RIFF_FORMAT_PCM            1
#define AUDIOLIB_RIFF_CHUNK_HEADER_SIZE     8  // Chunk id and chunk size
#define AUDIOLIB_RIFF_FMT_CHUNK_OFFSET      16 // Offset of the 'fmt ' chunk size from the start of the file
#define AUDIOLIB_RIFF_FMT_FORMAT_OFFSET     4  // 'fmt ' chunk layout, from the chunk size: format tag, channels, sample rate, bits per sample
#define AUDIOLIB_RIFF_FMT_CHANNELS_OFFSET   6
#define AUDIOLIB_RIFF_FMT_SAMPLERATE_OFFSET 8
#define AUDIOLIB_RIFF_FMT_BITS_OFFSET       18
#define AUDIOLIB_IMUS_MAGIC                 "iMUS"
#define AUDIOLIB_IMUS_FRMT_MAGIC            "FRMT"
#define AUDIOLIB_IMUS_SIZE_OFFSET           12 // Offset of the big endian size field used to locate the sound data
#define AUDIOLIB_IMUS_FRMT_OFFSET           16 // Offset of the 'FRMT' tag
#define AUDIOLIB_IMUS_FORMAT_OFFSET         32 // Offset of the format fields: bits per sample, sample rate and number of channels
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
    if ( memeq(pData, AUDIOLIB_MCMP_MAGIC, sizeof(AUDIOLIB_MCMP_MAGIC) - 1) )
    {
        uint16_t count = AudioLib_ReadU16BE(pData + 4);
        pData += (AUDIOLIB_MCMP_ENTRY_SIZE * count) + 6;

        count = AudioLib_ReadU16BE(pData);
        pData += count + 2;
        bCompressed = true;
    }

    if ( memeq(pData, AUDIOLIB_INDYWV_MAGIC, sizeof(AUDIOLIB_INDYWV_MAGIC) - 1) )
    {
        if ( pType )
        {
            *pType = AUDIOLIB_FILETYPE_INDYWV;
        }

        if ( pSampleRate )
        {
            *pSampleRate = AudioLib_ReadU32LE(pData + AUDIOLIB_INDYWV_SAMPLERATE_OFFSET);
        }

        if ( pBitsPerSample )
        {
            *pBitsPerSample = AudioLib_ReadU32LE(pData + AUDIOLIB_INDYWV_BITS_OFFSET);
        }

        if ( pNumChannels )
        {
            *pNumChannels = AudioLib_ReadU32LE(pData + AUDIOLIB_INDYWV_CHANNELS_OFFSET);
        }

        if ( pExtraInfo )
        {
            *pExtraInfo = AudioLib_ReadU32LE(pData + AUDIOLIB_INDYWV_DATASIZE_OFFSET);
        }

        if ( pSoundDataSize )
        {
            *pSoundDataSize = AudioLib_ReadU32LE(pData + AUDIOLIB_INDYWV_DATASIZE_OFFSET);
        }

        uint32_t extraDataSize = 0;
        if ( pExtraDataOffset && pSoundDataOffset )
        {
            extraDataSize = AudioLib_ReadU32LE(pData + AUDIOLIB_INDYWV_EXTRASIZE_OFFSET);
            *pSoundDataOffset = extraDataSize;
            if ( *pSoundDataOffset )
            {
                *pExtraDataOffset = (uint32_t)(uintptr_t)(pData + AUDIOLIB_INDYWV_HEADER_SIZE);
            }
            else
            {
                *pExtraDataOffset = 0;
            }
        }

        return pData + AUDIOLIB_INDYWV_HEADER_SIZE + extraDataSize;
    }

    if ( memeq(pData, AUDIOLIB_RIFF_MAGIC, sizeof(AUDIOLIB_RIFF_MAGIC) - 1) )
    {
        const uint8_t* pEnd = pData + AudioLib_ReadU32LE(pData + 4) + 8;
        if ( !memeq(pData + 8, AUDIOLIB_RIFF_WAVEFMT_MAGIC, sizeof(AUDIOLIB_RIFF_WAVEFMT_MAGIC) - 1) )
        {
            return NULL;
        }

        if ( AudioLib_ReadS16LE(pData + AUDIOLIB_RIFF_FMT_CHUNK_OFFSET + AUDIOLIB_RIFF_FMT_FORMAT_OFFSET) != AUDIOLIB_RIFF_FORMAT_PCM )
        {
            return NULL;
        }

        if ( pType )
        {
            *pType = bCompressed ? AUDIOLIB_FILETYPE_WAV_COMPRESSED : AUDIOLIB_FILETYPE_WAV;
        }

        pData += AUDIOLIB_RIFF_FMT_CHUNK_OFFSET;
        uint32_t size = AudioLib_ReadU32LE(pData);
        if ( pSampleRate )
        {
            *pSampleRate = AudioLib_ReadU32LE(pData + AUDIOLIB_RIFF_FMT_SAMPLERATE_OFFSET);
        }

        if ( pBitsPerSample )
        {
            *pBitsPerSample = AudioLib_ReadS16LE(pData + AUDIOLIB_RIFF_FMT_BITS_OFFSET);
        }

        if ( pNumChannels )
        {
            *pNumChannels = AudioLib_ReadS16LE(pData + AUDIOLIB_RIFF_FMT_CHANNELS_OFFSET);
        }

        if ( pExtraInfo )
        {
            *pExtraInfo = 1;
        }

        // Walk the chunks until the data chunk, remembering the sync chunk
        do
        {
            pData += size + AUDIOLIB_RIFF_CHUNK_HEADER_SIZE;
            size = AudioLib_ReadU32LE(pData);
            if ( memeq(pData - 4, AUDIOLIB_RIFF_SYNC_CHUNK, sizeof(AUDIOLIB_RIFF_SYNC_CHUNK) - 1) && pExtraDataOffset && pSoundDataOffset )
            {
                *pExtraDataOffset = (uint32_t)(uintptr_t)(pData + 4);
                *pSoundDataOffset = size;
            }
        } while ( pData < pEnd && !memeq(pData - 4, AUDIOLIB_RIFF_DATA_CHUNK, sizeof(AUDIOLIB_RIFF_DATA_CHUNK) - 1) );

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

    if ( memeq(pData, AUDIOLIB_IMUS_MAGIC, sizeof(AUDIOLIB_IMUS_MAGIC) - 1) )
    {
        uint32_t size = AudioLib_ReadU32BE(pData + AUDIOLIB_IMUS_SIZE_OFFSET);
        if ( !memeq(pData + AUDIOLIB_IMUS_FRMT_OFFSET, AUDIOLIB_IMUS_FRMT_MAGIC, sizeof(AUDIOLIB_IMUS_FRMT_MAGIC) - 1) )
        {
            return NULL;
        }

        if ( pType )
        {
            *pType = bCompressed ? AUDIOLIB_FILETYPE_IMUS_COMPRESSED : AUDIOLIB_FILETYPE_IMUS;
        }

        pData += AUDIOLIB_IMUS_FORMAT_OFFSET;
        if ( pBitsPerSample )
        {
            *pBitsPerSample = AudioLib_ReadU32BE(pData);
        }

        if ( pSampleRate )
        {
            *pSampleRate = AudioLib_ReadU32BE(pData + sizeof(uint32_t));
        }

        if ( pNumChannels )
        {
            *pNumChannels = AudioLib_ReadU32BE(pData + 2 * sizeof(uint32_t));
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

    if ( memeq(pData, AUDIOLIB_AIFF_MAGIC, sizeof(AUDIOLIB_AIFF_MAGIC) - 1) )
    {
        const uint8_t* pEnd = pData + AudioLib_ReadU32BE(pData + 4) + 8;
        if ( !memeq(pData + 8, AUDIOLIB_AIFF_COMM_MAGIC, sizeof(AUDIOLIB_AIFF_COMM_MAGIC) - 1) )
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
        } while ( pData < pEnd && !memeq(pData - 4, AUDIOLIB_AIFF_SSND_CHUNK, sizeof(AUDIOLIB_AIFF_SSND_CHUNK) - 1) );

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
        && memeq(pCompressedData, AUDIOLIB_WVSM_MAGIC, sizeof(AUDIOLIB_WVSM_MAGIC) - 1) )
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
//
// IMA-ADPCM style codec with a variable code width. Every channel is coded on its own, one after another (the channels
// of the interleaved input are not interleaved in the output), and starts from the step index and predicted sample
// stored in the compressor state.
//
// For each sample:
//  1. The step size is looked up from the step index (AudioLib_aStepTable) and also the width of the code
//     (AudioLib_aStepBits, 4 to 7 bits including the sign bit). The width grows with the step so loud passages get more bits.
//  2. The difference to the predicted sample is split into a sign and a magnitude.
//  3. The magnitude is quantized by successive approximation: starting with the most significant magnitude bit, the
//     step is subtracted from the difference if it fits, the bit is set in the code and the step is halved for the next bit.
//     The subtracted steps add up to the delta the decoder will reconstruct. A non-zero code gets one more halved step (rounding).
//  4. The code (sign bit on top) is written to the bit stream, most significant bit first.
//  5. If all magnitude bits are set the code is the escape code and the raw 16-bit sample follows in the stream, and the
//     prediction is set to that sample. Otherwise the signed delta is added to the prediction and the result is clamped to 16 bits.
//  6. The step index is adjusted by a table indexed by the code width and the code (AudioLib_aIndexTableTable): small codes
//     lower the index by one and large codes raise it by up to six, then it is clamped to the step table range.
//
// The state is written back after each channel so a stream can be compressed in multiple calls. Returns the size of the written data.
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
        // Load the state of the channel; samples of the channel are interleaved in the input
        int16_t* pSample = &pInBuffer[channel];
        int index        = pCompressorState->aStepIndex[channel];
        int predSample   = pCompressorState->aPrevSample[channel];

        for ( unsigned int i = 0; i < (unsigned int)numSamples; ++i, pSample += numChannels )
        {
            // TODO: [BUG] The sample is not sign-extended when the input is byte swapped (bNativeByteOrder == 0), so negative samples are coded as large positive ones.
            int sample = bNativeByteOrder ? *pSample : AudioLib_SwapU16(*(uint16_t*)pSample);

            // Step 1: step size and code width of the current step index. The escape code has all magnitude bits set.
            int step        = AudioLib_aStepTable[index];
            int diff        = sample - predSample;
            uint8_t numBits = AudioLib_aStepBits[index];
            int signBit     = 1 << (numBits - 1);
            uint8_t escape  = (uint8_t)(signBit - 1);

            // Step 2: split the difference to the prediction into sign and magnitude
            uint8_t sign = 0;
            if ( diff < 0 )
            {
                sign = (uint8_t)signBit;
                diff = -diff;
            }

            // Step 3: quantize the magnitude to the step, one bit at a time starting with the most significant magnitude bit
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

            // Rounding: a non-zero code also gets the last halved step
            if ( code )
            {
                delta += step;
            }

            // Step 4: append the code and the sign to the bit stream. A byte is written once the bit buffer holds a complete byte.
            uint8_t freeBits = 8 - (bitPos & 7);
            bitBuffer <<= numBits;
            bitBuffer |= code | sign;
            bitPos += numBits;
            if ( numBits >= freeBits )
            {
                *pCurOut++ = (uint8_t)(bitBuffer >> (numBits - freeBits));
            }

            // Step 5: update the prediction
            if ( code == escape )
            {
                // Escape code: store the raw 16-bit sample, big endian, behind the code and use it as the new prediction
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
                // The decoder reconstructs the same delta
                predSample += sign ? -delta : delta;
                predSample = STDMATH_CLAMP(predSample, AUDIOLIB_SAMPLE_MIN, AUDIOLIB_SAMPLE_MAX);
            }

            // Step 6: adapt the step index to the size of the code
            index += AudioLib_aIndexTableTable[numBits][code];
            index = STDMATH_CLAMP(index, 0, AUDIOLIB_MAX_STEP_INDEX);
        }

        // Save the state of the channel
        pCompressorState->aStepIndex[channel]  = (int8_t)index;
        pCompressorState->aPrevSample[channel] = (int16_t)predSample;
    }

    // Flush the remaining bits padded with zeros to a full byte
    if ( bitPos & 7 )
    {
        bitBuffer <<= 8 - (bitPos & 7);
        *pCurOut++ = bitBuffer & 0xFF;
    }

    return (int)(pCurOut - pOutBuffer);
}

// Builds the lookup table of the decoder: for every step index and every 6-bit code the sum of the step halves
// selected by the code bits, the most significant bit selecting the whole step. This is the delta the encoder builds up bit by bit.
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
//
// Inverse of AudioLib_CompressBlock. Every channel is decoded on its own from the step index and the previous sample of the
// compressor state, and written to the interleaved output.
//
// For each sample:
//  1. The code width (4 to 7 bits including the sign bit) is taken from the step index and the code is read from the bit stream,
//     most significant bit first.
//  2. The sign bit is split off the code.
//  3. If the magnitude bits are all set (escape code) the raw 16-bit sample follows in the stream and is used as is.
//  4. Otherwise the delta is the sum of step fractions looked up in the delta table (built by AudioLib_InitDeltaTable), plus
//     a rounding step for non-zero codes. It is subtracted from or added to the previous sample depending on the sign
//     and the result is clamped to 16 bits.
//  5. The sample becomes the new prediction and the step index is adapted by the same table as in the encoder.
//
// The state is written back after each channel.
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
        // Load the state of the channel; the channel is written interleaved to the output
        int index      = pCompressorState->aStepIndex[channel];
        int predSample = pCompressorState->aPrevSample[channel];
        int16_t* pOut  = (int16_t*)pOutData + channel;

        for ( int numSamplesLeft = numSamples; numSamplesLeft != 0; --numSamplesLeft )
        {
            // Step 1: read the code from the bit buffer. The buffer holds 16 bits and is refilled by a byte when a byte was consumed.
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

            // Step 2: split off the sign bit
            bool bNegative = (code & signBit) != 0;
            if ( bNegative )
            {
                code ^= signBit;
            }

            int sample;
            if ( code == escape )
            {
                // Step 3: escape code, the raw 16-bit sample follows in the stream, not byte aligned
                uint16_t nextBuffer = (uint16_t)((bitBuffer << 8) | *pIn++);
                uint8_t lowByte     = (uint8_t)(nextBuffer >> (8 - bitPos));
                sample              = (int16_t)(((bitBuffer << bitPos) & 0xFF00u) | lowByte);
                bitBuffer           = (uint16_t)((nextBuffer << 8) | *pIn++);
            }
            else
            {
                // Step 4: delta from the delta table, the code is aligned to the 6 table bits
                int delta = AudioLib_aDeltaTable[(code << (AUDIOLIB_MAX_STEP_BITS - numBits)) | (index << AUDIOLIB_DELTA_CODE_BITS)];
                if ( code )
                {
                    delta += AudioLib_aStepTable[index] >> (numBits - 1);
                }

                sample = bNegative ? predSample - delta : predSample + delta;
                sample = STDMATH_CLAMP(sample, AUDIOLIB_SAMPLE_MIN, AUDIOLIB_SAMPLE_MAX);
            }

            // Step 5: output the sample, use it as the prediction and adapt the step index
            predSample = sample;
            *pOut      = (int16_t)sample;
            pOut += numChannels;

            index += AudioLib_aIndexTableTable[numBits][code];
            index = STDMATH_CLAMP(index, 0, AUDIOLIB_MAX_STEP_INDEX);
        }

        // Save the state of the channel
        pCompressorState->aStepIndex[channel]  = (int8_t)index;
        pCompressorState->aPrevSample[channel] = (int16_t)predSample;
    }
}

// WVSM decompression of one block of up to AUDIOLIB_WVSM_BLOCK_SIZE bytes of stereo 16-bit PCM.
//
// The compressed block holds a big endian size (skipped here), a shift byte (left channel shift in the high nibble, right
// channel shift in the low nibble) and then the samples alternating left and right. A sample is a signed byte which is scaled
// back by the shift of its channel, or the escape byte 0x80 followed by the raw 16-bit sample, big endian, for samples that
// did not fit. Returns the number of consumed bytes.
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
        // Left sample
        int left = *pIn++;
        if ( left == AUDIOLIB_WVSM_ESCAPE )
        {
            // Escape: raw 16-bit sample
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

        // Right sample
        int right = *pIn++;
        if ( right == AUDIOLIB_WVSM_ESCAPE )
        {
            // Escape: raw 16-bit sample
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

// WVSM compression of one block of up to AUDIOLIB_WVSM_BLOCK_SIZE bytes of stereo 16-bit PCM, inverse of AudioLib_WVSMUncompressBlock.
//
// The block is written as: big endian compressed size (without the size field), a shift byte (left channel shift in the
// high nibble, right channel shift in the low nibble) and the samples alternating left and right. Every sample is stored
// as a signed byte, the 16-bit sample divided by the shift of its channel (rounded away from zero), or as the escape byte
// 0x80 followed by the big endian 16-bit sample, with the low bits lost to the shift already cleared, if it did not fit into the byte.
//
// Algorithm:
//  1. Histogram: for each channel count the samples by bit length of their magnitude. The histogram index is the bit length plus one.
//  2. Start with the largest shift (8) and lower it while no sample needs it, i.e. while no sample of the channel has a
//     magnitude of k + 7 bits, so every sample fits into a byte. Samples with k + 7 bits fit exactly.
//  3. Lowering a shift further gives the channel one more bit of precision but all samples of the highest bit length become
//     escapes (3 bytes instead of one). With a budget of 192 escapes per block, repeatedly lower the shift of the channel
//     whose escapes cost less until the budget is exhausted.
//  4. Write the block, optionally logging the histograms to pFile.
//
// Returns the size of the written block including the size field.
int J3DAPI AudioLib_WVSMCompressBlock(uint8_t* pOutBuffer, const uint8_t* pInBuffer, int blockSize, FILE* pFile)
{
    // Fixed: Histograms have one more entry than the original (17), the sample -32768 has a bit length of 17 and wrote past the end of the arrays
    int leftBits[AUDIOLIB_WVSM_HISTOGRAM_SIZE]  = { 0 };
    int rightBits[AUDIOLIB_WVSM_HISTOGRAM_SIZE] = { 0 };

    int numValues = blockSize >> 1; // Number of 16-bit samples of both channels

    // Step 1: histogram of the bit length of the magnitude of the left and right samples
    const int16_t* pIn = (const int16_t*)pInBuffer;
    for ( int i = 0; i < numValues; i += 2 )
    {
        int sample = *pIn++;
        sample = sample < 0 ? -sample : sample;

        int numBits = 1;
        for ( ; sample; sample >>= 1 )
        {
            ++numBits;
        }

        ++leftBits[numBits];

        sample = *pIn++;
        sample = sample < 0 ? -sample : sample;

        numBits = 1;
        for ( ; sample; sample >>= 1 )
        {
            ++numBits;
        }

        ++rightBits[numBits];
    }

    // Step 2: the smallest shift of each channel for which all samples fit into a byte
    int kLeft  = AUDIOLIB_WVSM_MAX_SHIFT;
    int kRight = AUDIOLIB_WVSM_MAX_SHIFT;
    while ( kLeft > 0 && !leftBits[kLeft + AUDIOLIB_WVSM_MAX_SHIFT] )
    {
        --kLeft;
    }

    while ( kRight > 0 && !rightBits[kRight + AUDIOLIB_WVSM_MAX_SHIFT] )
    {
        --kRight;
    }

    // Step 3: spend the escape budget on lowering the shifts
    int numSlotsLeft = AUDIOLIB_WVSM_MAX_ESCAPES;
    int leftSaved    = 0;
    int rightSaved   = 0;
    while ( numSlotsLeft )
    {
        bool bLeft;
        if ( kLeft > 0 && kRight > 0 )
        {
            // Both shifts can be lowered, take the channel with fewer escapes
            bLeft = leftBits[kLeft + AUDIOLIB_WVSM_MAX_SHIFT] < rightBits[kRight + AUDIOLIB_WVSM_MAX_SHIFT];
        }
        else if ( kLeft > 0 )
        {
            bLeft = true;
        }
        else if ( kRight > 0 )
        {
            bLeft = false;
        }
        else
        {
            break;
        }

        if ( bLeft )
        {
            if ( leftBits[kLeft + AUDIOLIB_WVSM_MAX_SHIFT] > numSlotsLeft )
            {
                break;
            }

            numSlotsLeft -= leftBits[kLeft + AUDIOLIB_WVSM_MAX_SHIFT];
            --kLeft;
            ++leftSaved;
        }
        else
        {
            if ( rightBits[kRight + AUDIOLIB_WVSM_MAX_SHIFT] > numSlotsLeft )
            {
                break;
            }

            numSlotsLeft -= rightBits[kRight + AUDIOLIB_WVSM_MAX_SHIFT];
            --kRight;
            ++rightSaved;
        }
    }

    // Rounding offset of each channel, half of the shifted out range
    int leftOffset  = kLeft ? 1 << (kLeft - 1) : 0;
    int rightOffset = kRight ? 1 << (kRight - 1) : 0;

    // Step 4: log the histograms
    if ( pFile )
    {
        fprintf(pFile, "L = %4d:", kLeft);
        for ( int i = 0; i < AUDIOLIB_WVSM_NUM_BIT_LENGTHS - 1; ++i )
        {
            fprintf(pFile, "%4d,", leftBits[i]);
        }

        fprintf(pFile, "%4d\n", leftBits[AUDIOLIB_WVSM_NUM_BIT_LENGTHS - 1]);

        fprintf(pFile, "R = %4d:", kRight);
        for ( int i = 0; i < AUDIOLIB_WVSM_NUM_BIT_LENGTHS - 1; ++i )
        {
            fprintf(pFile, "%4d,", rightBits[i]);
        }

        fprintf(pFile, "%4d\n", rightBits[AUDIOLIB_WVSM_NUM_BIT_LENGTHS - 1]);

        fprintf(pFile, "%d escape slots used, %dL/%dR bits saved\n\n", AUDIOLIB_WVSM_MAX_ESCAPES - numSlotsLeft, leftSaved, rightSaved);
    }

    // Block header: the size is written last, followed by the shifts
    uint8_t* pCurOut = pOutBuffer;
    *pCurOut++ = 0;
    *pCurOut++ = 0;
    *pCurOut++ = (uint8_t)((kLeft << 4) + kRight);

    // Write the samples
    pIn = (const int16_t*)pInBuffer;
    for ( int i = 0; i < numValues; i += 2 )
    {
        // Left sample: round away from zero, clamp, shift
        int sample = *pIn++;
        sample = sample < 0 ? sample - leftOffset : sample + leftOffset;
        sample = sample < -32767 ? -32767 : sample;
        sample = sample > 32767 ? 32767 : sample;
        sample >>= kLeft;

        int value = sample * (1 << kLeft); // Sample with the shifted out bits cleared
        if ( sample > 127 || sample < -127 )
        {
            // Escape: raw 16-bit sample, big endian
            *pCurOut++ = AUDIOLIB_WVSM_ESCAPE;
            *pCurOut++ = (uint8_t)(value >> 8);
            *pCurOut++ = (uint8_t)value;
        }
        else
        {
            *pCurOut++ = (uint8_t)sample;
        }

        // Right sample
        sample = *pIn++;
        sample = sample < 0 ? sample - rightOffset : sample + rightOffset;
        sample = sample < -32767 ? -32767 : sample;
        sample = sample > 32767 ? 32767 : sample;
        sample >>= kRight;

        value = sample * (1 << kRight);
        if ( sample > 127 || sample < -127 )
        {
            *pCurOut++ = AUDIOLIB_WVSM_ESCAPE;
            *pCurOut++ = (uint8_t)(value >> 8);
            *pCurOut++ = (uint8_t)value;
        }
        else
        {
            *pCurOut++ = (uint8_t)sample;
        }
    }

    // Store the compressed size, without the size field, big endian at the start of the block
    int compressedSize = (int)(pCurOut - pOutBuffer);
    pOutBuffer[0] = (uint8_t)((compressedSize - AUDIOLIB_WVSM_BLOCK_HEADER_SIZE) >> 8);
    pOutBuffer[1] = (uint8_t)(compressedSize - AUDIOLIB_WVSM_BLOCK_HEADER_SIZE);

    return compressedSize;
}

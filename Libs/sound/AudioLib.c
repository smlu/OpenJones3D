#include "AudioLib.h"
#include <j3dcore/j3dhook.h>
#include <sound/RTI/symbols.h>

#define AudioLib_aStepTable J3D_DECL_FAR_ARRAYVAR(AudioLib_aStepTable, const int16_t(*)[89])
#define AudioLib_aStepBits J3D_DECL_FAR_ARRAYVAR(AudioLib_aStepBits, const uint8_t(*)[89])
#define AudioLib_aIndexTableTable J3D_DECL_FAR_ARRAYVAR(AudioLib_aIndexTableTable, const int8_t*(*)[8])
#define AudioLib_aDeltaTable J3D_DECL_FAR_ARRAYVAR(AudioLib_aDeltaTable, int16_t(*)[64])
#define AudioLib_word_14E4928 J3D_DECL_FAR_ARRAYVAR(AudioLib_word_14E4928, int16_t(*)[5632])
#define AudioLib_bDeltaTableInitialized J3D_DECL_FAR_VAR(AudioLib_bDeltaTableInitialized, int)

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

static uint32_t AudioLib_ReadLE32(const uint8_t* p)
{
    uint32_t v;
    memcpy(&v, p, sizeof(v)); // x86: little endian
    return v;
}

static uint32_t AudioLib_ReadBE32(const uint8_t* p)
{
    return ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16) | ((uint32_t)p[2] << 8) | p[3];
}

static uint32_t AudioLib_ReadBE16(const uint8_t* p)
{
    return ((uint32_t)p[0] << 8) | p[1];
}

// Parses the header of a sound file (verified against the original v1.2 code).
// Returns the start of the sound data (NULL if the format isn't recognised) and sets *pType:
//   1 iMUSE (iMUS/FRMT, big endian), 2 the same inside MCMP, 3 RIFF WAVE PCM, 4 the same inside MCMP,
//   5 AIFF (FORM/AIFF), 6 IndyWV (the game's own format).
// pExtraDataOffset receives a *pointer* to the lip-sync data (RIFF "sync" chunk, IndyWV extra data) and
// pSoundDataOffset its size; both only when both are given. Quirks kept from the original: IndyWV fills pExtraInfo
// and pSoundDataSize from the same field, and the AIFF sample rate comes from the exponent byte of the 80-bit float
// (only 11025, 22050 and 44100 Hz).
const uint8_t* J3DAPI AudioLib_ParseWaveFileHeader(const uint8_t* pData, int* pType, uint32_t* pSampleRate, uint32_t* pBitsPerSample, uint32_t* pNumChannels, uint32_t* pExtraInfo, uint32_t* pSoundDataSize, uint32_t* pExtraDataOffset, uint32_t* pSoundDataOffset)
{
    if ( pType )
    {
        *pType = 0;
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

    // MCMP container: skip its header (a table of 9-byte entries, then a variable part)
    bool bMcmp = memcmp(pData, "MCMP", 4) == 0;
    if ( bMcmp )
    {
        uint32_t numEntries = AudioLib_ReadBE16(pData + 4);
        pData += AudioLib_ReadBE16(pData + numEntries * 9 + 6) + numEntries * 9 + 8;
    }

    if ( memcmp(pData, "INDYWV", 6) == 0 )
    {
        if ( pType )
        {
            *pType = 6;
        }
        if ( pSampleRate )
        {
            *pSampleRate = AudioLib_ReadLE32(pData + 6);
        }
        if ( pBitsPerSample )
        {
            *pBitsPerSample = AudioLib_ReadLE32(pData + 10);
        }
        if ( pNumChannels )
        {
            *pNumChannels = AudioLib_ReadLE32(pData + 14);
        }
        if ( pExtraInfo )
        {
            *pExtraInfo = AudioLib_ReadLE32(pData + 18);
        }
        if ( pSoundDataSize )
        {
            *pSoundDataSize = AudioLib_ReadLE32(pData + 18);
        }
        if ( pExtraDataOffset && pSoundDataOffset )
        {
            uint32_t extraSize = AudioLib_ReadLE32(pData + 22);
            *pSoundDataOffset  = extraSize;
            if ( extraSize )
            {
                *pExtraDataOffset = (uint32_t)(uintptr_t)(pData + 26);
                return pData + extraSize + 26;
            }
            *pExtraDataOffset = 0;
        }
        return pData + 26;
    }

    if ( memcmp(pData, "RIFF", 4) == 0 )
    {
        uint32_t riffSize = AudioLib_ReadLE32(pData + 4);
        int16_t format;
        memcpy(&format, pData + 20, sizeof(format));
        if ( memcmp(pData + 8, "WAVEfmt ", 8) != 0 || format != 1 ) // PCM only
        {
            return NULL;
        }

        if ( pType )
        {
            *pType = bMcmp ? 4 : 3;
        }
        if ( pSampleRate )
        {
            *pSampleRate = AudioLib_ReadLE32(pData + 24);
        }
        if ( pBitsPerSample )
        {
            int16_t bits;
            memcpy(&bits, pData + 34, sizeof(bits));
            *pBitsPerSample = (uint32_t)(int32_t)bits;
        }
        if ( pNumChannels )
        {
            int16_t channels;
            memcpy(&channels, pData + 22, sizeof(channels));
            *pNumChannels = (uint32_t)(int32_t)channels;
        }
        if ( pExtraInfo )
        {
            *pExtraInfo = 1;
        }

        // walk the chunks after "fmt " up to "data"; pSize points at the size field of the current chunk
        const uint8_t* pEnd  = pData + riffSize + 8;
        const uint8_t* pSize = pData + 16;
        uint32_t chunkSize   = AudioLib_ReadLE32(pSize);
        for ( ;; )
        {
            pSize    += chunkSize + 8;
            chunkSize = AudioLib_ReadLE32(pSize);
            if ( memcmp(pSize - 4, "sync", 4) == 0 && pExtraDataOffset && pSoundDataOffset )
            {
                *pExtraDataOffset = (uint32_t)(uintptr_t)(pSize + 4);
                *pSoundDataOffset = chunkSize;
            }
            if ( pSize >= pEnd )
            {
                return NULL;
            }
            if ( memcmp(pSize - 4, "data", 4) == 0 )
            {
                break;
            }
        }

        if ( pSoundDataSize )
        {
            *pSoundDataSize = chunkSize;
        }
        return pSize + 4;
    }

    if ( memcmp(pData, "iMUS", 4) == 0 )
    {
        uint32_t frmtEnd = AudioLib_ReadBE32(pData + 12);
        if ( memcmp(pData + 16, "FRMT", 4) != 0 )
        {
            return NULL;
        }

        if ( pType )
        {
            *pType = bMcmp ? 2 : 1;
        }
        if ( pBitsPerSample )
        {
            *pBitsPerSample = AudioLib_ReadBE32(pData + 32);
        }
        if ( pSampleRate )
        {
            *pSampleRate = AudioLib_ReadBE32(pData + 36);
        }
        if ( pNumChannels )
        {
            *pNumChannels = AudioLib_ReadBE32(pData + 40);
        }
        if ( pExtraInfo )
        {
            *pExtraInfo = 1;
        }
        if ( pSoundDataSize )
        {
            *pSoundDataSize = AudioLib_ReadBE32(pData + frmtEnd + 20);
        }
        return pData + frmtEnd + 24;
    }

    if ( !bMcmp && memcmp(pData, "FORM", 4) == 0 && memcmp(pData + 8, "AIFFCOMM", 8) == 0 )
    {
        const uint8_t* pEnd = pData + AudioLib_ReadBE32(pData + 4) + 8;
        if ( pType )
        {
            *pType = 5;
        }
        if ( pSampleRate )
        {
            uint8_t exponent = pData[0x1D];
            *pSampleRate = exponent == 0x0E ? 44100 : exponent == 0x0C ? 11025 : 22050;
        }
        if ( pBitsPerSample )
        {
            *pBitsPerSample = pData[0x1B];
        }
        if ( pNumChannels )
        {
            *pNumChannels = pData[0x15];
        }
        if ( pExtraInfo )
        {
            *pExtraInfo = 0;
        }

        // walk the chunks after "COMM" up to "SSND"; pSize points at the size field of the current chunk
        const uint8_t* pSize = pData + 16;
        uint32_t chunkSize   = AudioLib_ReadBE32(pSize);
        do
        {
            pSize    += chunkSize + 8;
            chunkSize = AudioLib_ReadBE32(pSize);
            if ( pSize >= pEnd )
            {
                return NULL;
            }
        } while ( memcmp(pSize - 4, "SSND", 4) != 0 );

        if ( pSoundDataSize )
        {
            *pSoundDataSize = chunkSize - 8;
        }
        return pSize + 12; // after the size, SSND's offset and block size fields
    }

    return NULL;
}

// Compressed sound layout (verified against the original v1.2 code):
//   ADPCM ("numChannels" 3 = mono, 4 = stereo):
//     byte 0: step index of channel 0, inverted (~) for stereo; bytes 1-2: prediction of channel 0 (big endian);
//     stereo: byte 3: step index of channel 1, bytes 4-5: its prediction; then the AudioLib_CompressBlock bit stream
//   WVSM ("numChannels" 1 or 2): fixed header E4 11 11 64 22 22 'W' 'V' 'S' 'M' (an impossible ADPCM state), then
//     AudioLib_WVSMCompressBlock blocks of 0x1000 input bytes each
static const uint8_t AudioLib_aWVSMHeader[10] = { 0xE4, 0x11, 0x11, 0x64, 0x22, 0x22, 'W', 'V', 'S', 'M' };
#define AUDIOLIB_WVSM_BLOCKSIZE 0x1000

int J3DAPI AudioLib_Compress(tAudioCompressorState* pCompressorState, uint8_t* pOutBuffer, const uint8_t* pInBuffer, int size, unsigned int numChannels)
{
    if ( numChannels > 2 )
    {
        // ADPCM
        numChannels -= 2;
        pOutBuffer[0] = numChannels > 1 ? (uint8_t)~pCompressorState->aStepIndex[0] : pCompressorState->aStepIndex[0];
        pOutBuffer[1] = (uint8_t)((uint16_t)pCompressorState->aPrediction[0] >> 8);
        pOutBuffer[2] = (uint8_t)pCompressorState->aPrediction[0];
        uint8_t* pData = pOutBuffer + 3;
        if ( numChannels > 1 )
        {
            pData[0] = pCompressorState->aStepIndex[1];
            pData[1] = (uint8_t)((uint16_t)pCompressorState->aPrediction[1] >> 8);
            pData[2] = (uint8_t)pCompressorState->aPrediction[1];
            pData   += 3;
        }

        int numSamples = (int)((unsigned int)size / (numChannels * 2));
        return AudioLib_CompressBlock(pCompressorState, pData, (int16_t*)pInBuffer, numSamples, numChannels, 1, 1) + numChannels * 3;
    }

    // WVSM
    memcpy(pOutBuffer, AudioLib_aWVSMHeader, sizeof(AudioLib_aWVSMHeader));
    uint8_t* pOut = pOutBuffer + sizeof(AudioLib_aWVSMHeader);
    unsigned int remaining = (unsigned int)size;
    while ( remaining )
    {
        unsigned int blockSize = remaining > AUDIOLIB_WVSM_BLOCKSIZE ? AUDIOLIB_WVSM_BLOCKSIZE : remaining;
        pOut      += AudioLib_WVSMCompressBlock(pOut, pInBuffer, (int)blockSize, NULL);
        pInBuffer += AUDIOLIB_WVSM_BLOCKSIZE;
        remaining -= blockSize;
    }
    return (int)(pOut - pOutBuffer);
}

void J3DAPI AudioLib_ResetCompressor(tAudioCompressorState* pState)
{
    pState->aStepIndex[0]  = 0;
    pState->aStepIndex[1]  = 0;
    pState->aPrediction[0] = 0;
    pState->aPrediction[1] = 0;
}

void J3DAPI AudioLib_Uncompress(tAudioCompressorState* pCompressorState, uint8_t* pOutSndData, const uint8_t* pCompressedData, unsigned int size)
{
    unsigned int numChannels = 1;
    pCompressorState->aStepIndex[0] = pCompressedData[0];
    if ( (int8_t)pCompressedData[0] < 0 )
    {
        pCompressorState->aStepIndex[0] = (uint8_t)~pCompressedData[0];
        numChannels = 2;
    }
    pCompressorState->aPrediction[0] = (int16_t)((pCompressedData[1] << 8) | pCompressedData[2]);

    const uint8_t* pData = pCompressedData + 3;
    if ( numChannels > 1 )
    {
        pCompressorState->aStepIndex[1]  = pData[0];
        pCompressorState->aPrediction[1] = (int16_t)((pData[1] << 8) | pData[2]);
        pData += 3;
    }

    // The WVSM header reads as a stereo ADPCM state that the encoder never writes
    if ( numChannels == 2 && pCompressorState->aPrediction[0] == 0x1111 && pCompressorState->aStepIndex[1] == 100
        && pCompressorState->aPrediction[1] == 0x2222 && memcmp(pData, &AudioLib_aWVSMHeader[6], 4) == 0 )
    {
        pData += 4;
        while ( size )
        {
            unsigned int blockSize = size > AUDIOLIB_WVSM_BLOCKSIZE ? AUDIOLIB_WVSM_BLOCKSIZE : size;
            pData       += AudioLib_WVSMUncompressBlock(pOutSndData, pData, (int)blockSize);
            pOutSndData += AUDIOLIB_WVSM_BLOCKSIZE;
            size        -= blockSize;
        }
        return;
    }

    AudioLib_UncompressBlock(pCompressorState, pOutSndData, pData, (int)(size / (numChannels * 2)), numChannels, 1);
}

// Lip-sync data ("SYNC" block, verified against the original v1.2 code):
//   "SYNC", uint32 number of entries, then uint32 entries sorted by time: bits 16-31 the time in units of 16 ms
//   (milliseconds << 12, rounded down), bits 8-14 the mouth width (X), bits 0-6 the opening (Y); the last entry only
//   holds the end time.
static const uint8_t AudioLib_aSyncMagic[4] = { 'S', 'Y', 'N', 'C' };

// Mouth position at a sound position in milliseconds (below 2^20). Returns 0 on success, 1 if the data isn't a SYNC
// block or the position is out of range (then both outputs are 0).
int J3DAPI AudioLib_GetMouthPosition(uint8_t* pData, int position, uint8_t* pMouthPosX, uint8_t* pMouthPosY)
{
    uint8_t x = 0, y = 0;
    int result = 1;

    if ( memcmp(pData, AudioLib_aSyncMagic, 4) == 0 && ((uint32_t)position & 0xFFF00000) == 0 )
    {
        result = 0;
        int32_t numEntries;
        memcpy(&numEntries, pData + 4, sizeof(numEntries));
        const uint8_t* pEntries = pData + 8;
        #define AUDIOLIB_SYNC_ENTRY(i) AudioLib_ReadLE32(pEntries + (i) * 4)

        uint32_t key = ((uint32_t)position & ~0xFu) << 12;
        int hi       = numEntries - 1;
        int index    = 0;
        if ( AUDIOLIB_SYNC_ENTRY(numEntries - 1) < key )
        {
            key   = 0; // past the end: last entry
            index = hi;
        }

        // binary search for the entry with this time, or the last one before it
        int lo = 0;
        if ( key != 0 )
        {
            for ( ;; )
            {
                index = (hi - lo) / 2 + lo;
                if ( !(lo + 1 < hi && key != (AUDIOLIB_SYNC_ENTRY(index) & 0xFFFF0000)) )
                {
                    break;
                }
                if ( AUDIOLIB_SYNC_ENTRY(index) < key )
                {
                    lo = index;
                }
                else
                {
                    hi = index;
                }
            }
        }

        uint32_t entry = AUDIOLIB_SYNC_ENTRY(index);
        x = (uint8_t)((entry >> 8) & 0x7F);
        y = (uint8_t)(entry & 0x7F);
        #undef AUDIOLIB_SYNC_ENTRY
    }

    if ( pMouthPosX )
    {
        *pMouthPosX = x;
    }
    if ( pMouthPosY )
    {
        *pMouthPosY = y;
    }
    return result;
}

static const uint8_t* AudioLib_pSndDataEnd; // AudioLib_GenerateLipSyncBlock: end of the sound data

// A 16-bit sample (the data may be misaligned: bByteOffset). Fixed: the loops read the next sample, up to two bytes past
// the end of the data; there the original read whatever followed (it crashes where the buffer ends at a page boundary,
// e.g. on Android): missing bytes count as 0.
static int16_t AudioLib_ReadSample(const uint8_t* p)
{
    if ( AudioLib_pSndDataEnd && p + sizeof(int16_t) > AudioLib_pSndDataEnd )
    {
        return p < AudioLib_pSndDataEnd ? (int16_t)p[0] : 0;
    }
    int16_t v;
    memcpy(&v, p, sizeof(v));
    return v;
}

// Generates lip-sync data from 16-bit mono sound by amplitude analysis: per window of 1/updateRate s, the peak
// (relative to a moving average of the last 15 windows) opens the mouth (Y) and the zero-crossing rate widens it
// (X); quiet windows let both decay. Positions are quantised to numXPositions/numYPositions steps of the 7-bit range,
// and an entry is written only when they change. bByteOffset reads the data shifted by one byte (Sound.c passes 1:
// each "sample" is then the high byte of one sample and the low byte of the next). Returns the block size, 0 if the
// sound isn't 16-bit mono. Note: the original also formatted an unused debug string (which divided by zero for a
// 1-byte sound); left out.
int J3DAPI AudioLib_GenerateLipSyncBlock(uint8_t* pOutData, const uint8_t* pSndData, unsigned int updateRate, char numXPositions, char numYPositions, int sampleRate, int bitsPerSample, int numChannels, int bByteOffset, int sndDataSize)
{
    unsigned int dataSize = (unsigned int)sndDataSize;
    if ( dataSize == 0 || !pSndData || bitsPerSample != 16 || numChannels != 1 )
    {
        return 0;
    }

    AudioLib_pSndDataEnd = pSndData + dataSize;
    if ( bByteOffset )
    {
        pSndData += 1;
    }

    memcpy(pOutData, AudioLib_aSyncMagic, 4);
    pOutData[4] = 0;
    uint8_t* pEntry = pOutData + 8;

    // quantisation masks: the top bits of the 7-bit range, as many as numPositions - 1 needs
    unsigned int maskX = 0, maskY = 0;
    uint8_t restX = (uint8_t)(numXPositions - 1), restY = (uint8_t)(numYPositions - 1);
    for ( int i = 0; i < 7; i++ )
    {
        maskX = (maskX << 1) | (restX != 0);
        maskY = (maskY << 1) | (restY != 0);
        restX >>= 1;
        restY >>= 1;
    }

    // whole sound: zero crossings (rising) and peak
    int numCrossings = 0, peak = 0;
    if ( (int)dataSize > 2 )
    {
        for ( unsigned int i = 0; i < (dataSize - 1) >> 1; i++ )
        {
            int16_t cur = AudioLib_ReadSample(pSndData + i * 2), next = AudioLib_ReadSample(pSndData + i * 2 + 2);
            int amplitude = next < 0 ? -next : next;
            if ( next >= 0 && cur < 0 )
            {
                numCrossings++;
            }
            if ( amplitude > peak )
            {
                peak = amplitude;
            }
        }
    }

    int aRecentPeaks[15];
    for ( int i = 0; i < 15; i++ )
    {
        aRecentPeaks[i] = peak;
    }

    unsigned int windowSize   = (unsigned int)(sampleRate * 2) / updateRate; // bytes
    unsigned int crossingNorm = windowSize * (unsigned int)numCrossings;
    int recentIndex           = 0;
    uint32_t time             = 0;
    int processed             = 0;
    unsigned int prevX = 0, prevY = 0;
    unsigned int lastX = 0xFFFFFFFF, lastY = 0xFFFFFFFF;

    for ( unsigned int end = windowSize; (int)end <= (int)dataSize; end = windowSize + (unsigned int)processed )
    {
        int windowCrossings = 0, windowPeak = 0, windowSum = 0;
        if ( (int)windowSize > 2 )
        {
            for ( unsigned int i = 0; i < (windowSize - 1) >> 1; i++ )
            {
                int16_t cur = AudioLib_ReadSample(pSndData + i * 2), next = AudioLib_ReadSample(pSndData + i * 2 + 2);
                int amplitude = next < 0 ? -next : next;
                if ( next >= 0 && cur < 0 )
                {
                    windowCrossings++;
                }
                if ( amplitude > windowPeak )
                {
                    windowPeak = amplitude;
                }
                windowSum += amplitude;
            }
        }

        int average = 0;
        for ( int i = recentIndex; i < recentIndex + 15; i++ )
        {
            average += aRecentPeaks[i % 15];
        }
        average /= 15;
        if ( average == 0 )
        {
            average = 1;
        }
        if ( average < peak / 4 )
        {
            average = peak / 4;
        }
        aRecentPeaks[recentIndex] = windowPeak;

        int opening = (windowPeak * 60) / average;
        int width   = (windowCrossings * 50) / (int)(crossingNorm / dataSize + 1);
        if ( opening < 25 )
        {
            width = ((width - 37) * opening + 925) / 25;
        }
        if ( width > 99 )
        {
            width = 100;
        }
        unsigned int x = (unsigned int)((width * 127) / 100);
        if ( opening > 99 )
        {
            opening = 100;
        }
        unsigned int y = (unsigned int)((opening * 127) / 100);

        if ( windowSum / ((int)windowSize >> 1) < (windowPeak * 30) / 100 ) // quiet: close slowly
        {
            x = (int)prevX < 48 ? 0 : prevX - 16;
            y = (int)prevY < 16 ? 0 : prevY - 16;
        }

        unsigned int qx = x & maskX, qy = y & maskY;
        if ( qx != lastX || qy != lastY )
        {
            uint32_t entry = (time & 0xFFFF0000) | ((qx & 0x7F) << 8) | (qy & 0x7F);
            memcpy(pEntry, &entry, 4);
            pEntry += 4;
            lastX = qx;
            lastY = qy;
        }

        time        += (uint32_t)(int)(0x3E8000ull / updateRate);
        processed   += (int)windowSize;
        pSndData    += windowSize;
        recentIndex  = (recentIndex + 1) % 15;
        prevX        = x;
        prevY        = y;
    }

    uint32_t endEntry = time & 0xFFFF0000;
    memcpy(pEntry, &endEntry, 4);
    pEntry += 4;

    int32_t numEntries = (int32_t)((pEntry - pOutData) >> 2) - 2;
    memcpy(pOutData + 4, &numEntries, 4);
    AudioLib_pSndDataEnd = NULL;
    return numEntries * 4 + 8;
}

// ADPCM compression (verified against the original v1.2 code); the inverse of
// AudioLib_UncompressBlock. numSamples per channel, interleaved 16-bit input; bNativeOrder 1: little-endian samples,
// 0: big-endian samples, which the original reads as unsigned values (kept as is; AudioLib_Compress always passes 1).
int J3DAPI AudioLib_CompressBlock(tAudioCompressorState* pCompressorState, uint8_t* pOutBuffer, int16_t* pSamples, int numSamples, unsigned int numChannels, int bNativeOrder, int bStateInitialized)
{
    if ( !bStateInitialized )
    {
        AudioLib_ResetCompressor(pCompressorState);
    }

    uint8_t* pOut          = pOutBuffer;
    unsigned int bitBuffer = 0; // pending output bits in the low bits
    unsigned int numBits   = 0; // bits written so far

    for ( unsigned int channel = 0; channel < numChannels; channel++ )
    {
        int stepIndex       = (int8_t)pCompressorState->aStepIndex[channel];
        int prediction      = pCompressorState->aPrediction[channel];
        const uint16_t* pIn = (const uint16_t*)pSamples + channel;

        for ( int n = numSamples; n != 0; n--, pIn += numChannels )
        {
            int sample = bNativeOrder ? (int16_t)*pIn : (int)(uint16_t)((*pIn >> 8) | (*pIn << 8));

            unsigned int codeBits = AudioLib_aStepBits[stepIndex];
            unsigned int signBit  = 1u << (codeBits - 1);
            int step              = (uint16_t)AudioLib_aStepTable[stepIndex];

            int diff          = sample - prediction;
            unsigned int sign = 0;
            if ( diff < 0 )
            {
                diff = -diff;
                sign = signBit;
            }

            // magnitude: binary search over halving step sizes; delta is what the decoder will add
            unsigned int code = 0;
            int delta         = 0;
            for ( unsigned int bit = signBit >> 1; bit; bit >>= 1, step >>= 1 )
            {
                if ( step <= diff )
                {
                    diff  -= step;
                    code  |= bit;
                    delta += step;
                }
            }
            if ( code != 0 )
            {
                delta += step;
            }

            unsigned int freeBits = 8 - (numBits & 7);
            bitBuffer = (bitBuffer << codeBits) | sign | code;
            numBits  += codeBits;
            if ( freeBits <= codeBits )
            {
                *pOut++ = (uint8_t)((bitBuffer & 0xFFFF) >> (codeBits - freeBits));
            }

            if ( code == (uint8_t)(signBit - 1) )
            {
                // escape: the sample itself follows as 16 bits
                prediction = bNativeOrder ? (int16_t)*pIn : (int)(uint16_t)((*pIn >> 8) | (*pIn << 8));
                unsigned int bitPos = numBits & 7;
                pOut[0]   = (uint8_t)((((bitBuffer & 0xFF) << 8) | ((prediction >> 8) & 0xFF)) >> bitPos);
                bitBuffer = (unsigned int)prediction & 0xFFFF;
                pOut[1]   = (uint8_t)((uint16_t)prediction >> bitPos);
                pOut     += 2;
            }
            else
            {
                prediction += sign ? -delta : delta;
                if ( prediction < -0x8000 )
                {
                    prediction = -0x8000;
                }
                else if ( prediction > 0x7FFF )
                {
                    prediction = 0x7FFF;
                }
            }

            stepIndex += AudioLib_aIndexTableTable[codeBits][code];
            if ( stepIndex < 0 )
            {
                stepIndex = 0;
            }
            if ( stepIndex > 88 )
            {
                stepIndex = 88;
            }
        }

        pCompressorState->aStepIndex[channel]  = (uint8_t)stepIndex;
        pCompressorState->aPrediction[channel] = (int16_t)prediction;
    }

    if ( (numBits & 7) != 0 )
    {
        *pOut++ = (uint8_t)(bitBuffer << (8 - (numBits & 7)));
    }
    return (int)(pOut - pOutBuffer);
}

// ADPCM delta table: aDeltas[step][code] is the change encoded by a 6-bit magnitude code at a step size,
// sum over the code's bits (32, 16, ... 1) of step >> (bit position from the top). Note: own copy (the exe builds
// the same table in AudioLib_aDeltaTable), so the decoder no longer depends on the exe's data.
static int16_t AudioLib_aDeltas[89][64];
static bool AudioLib_bDeltasInitialized = false;

static void AudioLib_InitDeltas(void)
{
    for ( unsigned int code = 0; code < 64; code++ )
    {
        for ( size_t step = 0; step < 89; step++ )
        {
            uint16_t stepSize = (uint16_t)AudioLib_aStepTable[step];
            int16_t delta     = 0;
            for ( unsigned int bit = 0x20; bit; bit >>= 1, stepSize >>= 1 )
            {
                if ( (code & bit) != 0 )
                {
                    delta += stepSize;
                }
            }
            AudioLib_aDeltas[step][code] = delta;
        }
    }
    AudioLib_bDeltasInitialized = true;
}

// ADPCM decompress (verified against the original v1.2 code).
// numSamples per channel; the channels are stored one after another in the bit stream, and interleaved in the output.
// Each sample is a code of AudioLib_aStepBits[step index] bits, most significant bit first: the top bit is the sign,
// the rest the magnitude. A magnitude with all bits set is an escape: the next 16 bits are the sample itself.
void J3DAPI AudioLib_UncompressBlock(tAudioCompressorState* pCompressorState, uint8_t* pOutData, const uint8_t* pInData, int numSamples, unsigned int numChannels, int bStateInited)
{
    if ( !AudioLib_bDeltasInitialized )
    {
        AudioLib_InitDeltas();
    }

    if ( !bStateInited )
    {
        AudioLib_ResetCompressor(pCompressorState);
    }

    unsigned int bitBuffer = (pInData[0] << 8) | pInData[1]; // next 16 bits of the stream
    const uint8_t* pIn     = pInData + 2;
    int bitPos             = 0;                               // bits of bitBuffer already used

    for ( unsigned int channel = 0; channel < numChannels; channel++ )
    {
        int stepIndex = (int8_t)pCompressorState->aStepIndex[channel];
        int sample    = pCompressorState->aPrediction[channel];
        int16_t* pOut = (int16_t*)pOutData + channel;

        for ( int n = numSamples; n != 0; n-- )
        {
            unsigned int numBits = AudioLib_aStepBits[stepIndex];
            unsigned int signBit = 1u << (numBits - 1);
            uint8_t escape       = (uint8_t)(signBit - 1);
            unsigned int mask    = (signBit & ~0xFFu) | ((signBit | escape) & 0xFFu);

            bitPos += numBits;
            unsigned int code = (bitBuffer >> (16 - bitPos)) & mask;
            if ( bitPos > 7 )
            {
                bitPos   -= 8;
                bitBuffer = ((bitBuffer << 8) | *pIn++) & 0xFFFF;
            }

            bool bNegative = (code & signBit) != 0;
            if ( bNegative )
            {
                code ^= signBit;
            }

            if ( (uint8_t)code == escape )
            {
                uint8_t hi = (uint8_t)(((bitBuffer << bitPos) & 0xFFFF) >> 8);
                uint8_t lo = (uint8_t)((((bitBuffer & 0xFF) << 8) | pIn[0]) >> (8 - bitPos));
                sample     = (int16_t)((hi << 8) | lo);
                bitBuffer  = (pIn[0] << 8) | pIn[1];
                pIn       += 2;
            }
            else
            {
                unsigned int delta = (uint16_t)AudioLib_aDeltas[stepIndex][code << (7 - numBits)];
                if ( code != 0 )
                {
                    delta += (unsigned int)AudioLib_aStepTable[stepIndex] >> (numBits - 1);
                }

                if ( !bNegative )
                {
                    sample += (int)delta;
                    if ( sample > 0x7FFE )
                    {
                        sample = 0x7FFF;
                    }
                }
                else
                {
                    sample -= (int)delta;
                    if ( sample < -0x7FFF )
                    {
                        sample = -0x8000;
                    }
                }
            }

            *pOut = (int16_t)sample;
            pOut += numChannels;

            stepIndex += AudioLib_aIndexTableTable[numBits][code];
            if ( stepIndex < 1 )
            {
                stepIndex = 0;
            }
            else if ( stepIndex > 87 )
            {
                stepIndex = 88;
            }
        }

        pCompressorState->aStepIndex[channel]  = (uint8_t)stepIndex;
        pCompressorState->aPrediction[channel] = (int16_t)sample;
    }
}

//int J3DAPI AudioLib_WVSMCompressBlock(uint8_t* pOutBuffer, const uint8_t* pInBuffer, int blockSize, FILE* pFile)
//{
//    return J3D_TRAMPOLINE_CALL(AudioLib_WVSMCompressBlock, pOutBuffer, pInBuffer, blockSize, pFile);
//}

// WVSM block decompress (verified against the original v1.2 code).
// Byte 2 of a block holds the shifts: high nibble for even (left) samples, low nibble for odd (right) ones. Each
// sample is a signed byte shifted left by its shift; 0x80 is an escape followed by the sample as big-endian 16 bits.
// Returns the number of input bytes used.
int J3DAPI AudioLib_WVSMUncompressBlock(uint8_t* pOutBuffer, const uint8_t* pInBuffer, int blockSize)
{
    int16_t* pOut      = (int16_t*)pOutBuffer;
    uint8_t shifts     = pInBuffer[2];
    const uint8_t* pIn = pInBuffer + 3;
    int numSamples     = blockSize >> 1;

    for ( int i = 0; i < numSamples; i++ )
    {
        uint8_t code = *pIn++;
        if ( code == 0x80 )
        {
            pOut[i] = (int16_t)((pIn[0] << 8) + pIn[1]);
            pIn    += 2;
        }
        else
        {
            unsigned int shift = (i & 1) ? (shifts & 0x0F) : (shifts >> 4);
            pOut[i] = (int16_t)(uint16_t)((uint32_t)(int32_t)(int8_t)code << shift); // shifted as unsigned: well defined
        }
    }
    return (int)(pIn - pInBuffer);
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

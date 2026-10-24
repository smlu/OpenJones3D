#ifndef SOUND_AUDIOLIB_H
#define SOUND_AUDIOLIB_H
#include <j3dcore/j3d.h>
#include <sound/types.h>
#include <sound/RTI/addresses.h>
#include <stdio.h>

J3D_EXTERN_C_START

// Added: Expose the seven-bit coordinate range stored in each lip-sync mouth axis.
#define AUDIOLIB_LIPSYNC_MOUTH_POSITION_BITS  7u
#define AUDIOLIB_LIPSYNC_MOUTH_POSITION_COUNT (1u << AUDIOLIB_LIPSYNC_MOUTH_POSITION_BITS)

// Returns pointer to snd data
const uint8_t* J3DAPI AudioLib_ParseWaveFileHeader(const uint8_t* pData, int* pType, uint32_t* pSampleRate, uint32_t* pBitsPerSample, uint32_t* pNumChannels, uint32_t* pExtraInfo, uint32_t* pSoundDataSize, uint32_t* pExtraDataOffset, uint32_t* pSoundDataOffset);

int J3DAPI AudioLib_Compress(tAudioCompressorState* pCompressorState, uint8_t* pOutBuffer, const uint8_t* pInBuffer, int size, unsigned int numChannels);
void J3DAPI AudioLib_ResetCompressor(tAudioCompressorState* pState);
void J3DAPI AudioLib_Uncompress(tAudioCompressorState* pCompressorState, uint8_t* pOutSndData, const uint8_t* pCompressedData, unsigned int size);
int J3DAPI AudioLib_GetMouthPosition(uint8_t* pData, int a2, uint8_t* pMouthPosX, uint8_t* pMouthPosY);

/**
 * Generates a lip-sync block from 16-bit mono PCM data.
 *
 * @param bCrossSample When nonzero, decoding starts one byte into the PCM stream. For adjacent little-endian
 *                     samples `{lowN, highN}` and `{lowNPlus1, highNPlus1}`, a read at that position produces
 *                     `{highN, lowNPlus1}`: the current sample's high byte becomes the new low byte, and the
 *                     adjacent sample's low byte becomes the new high byte. This creates synthetic sample
 *                     amplitudes and zero crossings; it does not interpolate or mix the source samples.
 * @return The generated block size in bytes, or 0 on failure.
 */
// Altered: Make the recovered signed-byte mouth levels and non-negative audio metadata explicit.
int J3DAPI AudioLib_GenerateLipSyncBlock(uint8_t* pOutData, const uint8_t* pSndData, uint32_t analysisRateHz, int8_t mouthXLevels, int8_t mouthYLevels, uint32_t sampleRate, uint32_t bitsPerSample, uint32_t numChannels, int bCrossSample, uint32_t sndDataSize);

// Added: Capacity-aware helpers for callers that cannot rely on the original unchecked output contract.
size_t J3DAPI AudioLib_GetLipSyncBlockMaxSize(uint32_t analysisRateHz, uint32_t sampleRate, uint32_t bitsPerSample, uint32_t numChannels, uint32_t sndDataSize);

/**
 * Generates a lip-sync block while validating the input and output buffer capacities.
 *
 * @param bCrossSample When nonzero, decoding starts one byte into the PCM stream. For adjacent little-endian
 *                     samples `{lowN, highN}` and `{lowNPlus1, highNPlus1}`, a read at that position produces
 *                     `{highN, lowNPlus1}`: the current sample's high byte becomes the new low byte, and the
 *                     adjacent sample's low byte becomes the new high byte. This creates synthetic sample
 *                     amplitudes and zero crossings; it does not interpolate or mix the source samples.
 * @return The generated block size in bytes, or 0 on failure.
 */
int J3DAPI AudioLib_GenerateLipSyncBlockEx(uint8_t* pOutData, size_t outDataSize, const uint8_t* pSndData, size_t sndDataCapacity, uint32_t analysisRateHz, int8_t mouthXLevels, int8_t mouthYLevels, uint32_t sampleRate, uint32_t bitsPerSample, uint32_t numChannels, int bCrossSample, uint32_t sndDataSize);

// ADPCM compression
int J3DAPI AudioLib_CompressBlock(tAudioCompressorState* pCompressorState, uint8_t* pOutBuffer, int16_t* a3, int a4, unsigned int numChannels, int a6, int bStateInitialized);
// ADPCM decompress
void J3DAPI AudioLib_UncompressBlock(tAudioCompressorState* pCompressorState, uint8_t* pOutData, const uint8_t* pInData, int size, unsigned int numChannels, int bStateInited);
int J3DAPI AudioLib_WVSMCompressBlock(uint8_t* pOutBuffer, const uint8_t* pInBuffer, int blockSize, FILE* pFile);
int J3DAPI AudioLib_WVSMUncompressBlock(uint8_t* pOutBuffer, const uint8_t* pInBuffer, int blockSize);

// Helper hooking functions
void AudioLib_InstallHooks(void);
void AudioLib_ResetGlobals(void);

J3D_EXTERN_C_END
#endif // SOUND_AUDIOLIB_H

#ifndef SOUND_DRIVER_H
#define SOUND_DRIVER_H
#include <j3dcore/j3d.h>
#include <sound/types.h>
#include <sound/RTI/addresses.h>

J3D_EXTERN_C_START

#if defined (J3D_DIRECTX6)
typedef IDirectSound tDirectSound;
#elif defined(J3D_DIRECTX9) || defined(J3D_OPENGL)
typedef IDirectSound8 tDirectSound;
#else
#error "Unsuported system sound API. Please define J3D_DIRECTX6 or J3D_DIRECTX9 in your project."
#endif

typedef uint8_t* (J3DAPI* SoundDriverGetSoundBufferDataFunc)(tSysSoundBuffer* pDSBuf, uint32_t* pSoundDataSize, uint32_t* pbCompressed);

int J3DAPI SoundDriver_Open(int bNoSound3D, int bGlobalFocus, HWND hwnd, tDirectSound* pDirectSound, SoundCalcListenerSoundMixFunc pfCalcListenerSoundMix, SoundDriverGetSoundBufferDataFunc pfGetSoundBufferData, tPrintfFunc pfLogError);
void SoundDriver_Close(void);

void J3DAPI SoundDriver_SetMaxVolume(float volume);
tSysSoundBuffer* J3DAPI SoundDriver_CreateAndPlay(size_t samplesPerSec, size_t nBitsPerSample, size_t numChannels, const uint8_t* pSoundData, size_t dataSize, tSoundChannelFlag* pFlags, tSysSoundBuffer* pSBuffer);
void J3DAPI SoundDriver_Release(tSysSoundBuffer* pDSBuf);
void J3DAPI SoundDriver_Play(tSysSoundBuffer* pDSBuf, int bLoop);
void J3DAPI SoundDriver_Stop(tSysSoundBuffer* pDSBuf);

void J3DAPI SoundDriver_SetVolume(tSysSoundBuffer* pDSBuf, float volume);
float J3DAPI SoundDriver_GetVolume(tSysSoundBuffer* pDSBuf);

void J3DAPI SoundDriver_SetPan(tSysSoundBuffer* pDSoundBuf, float pan);
float J3DAPI SoundDriver_GetPan(tSysSoundBuffer* pDSBuf); // Added: From debug

void J3DAPI SoundDriver_SetFrequency(tSysSoundBuffer* pDSBuf, float freq);
float J3DAPI SoundDriver_GetFrequency(tSysSoundBuffer* pDSBuf);

size_t J3DAPI SoundDriver_GetCurrentPosition(tSysSoundBuffer* pDSoundBuf);
tSoundChannelFlag J3DAPI SoundDriver_GetStatusAndCaps(tSysSoundBuffer* pDSBuffer);

void J3DAPI SoundDriver_SetListenerPosition(const rdVector3* pPos, const rdVector3* pVelocity, const rdVector3* pTopOrient, const rdVector3* pFrontOrient);
void SoundDriver_ListenerCommitDeferred(void);

void J3DAPI SoundDriver_SetPosAndVelocity(tSysSoundBuffer* pBuffer, float x, float y, float z, float velX, float velY, float velZ, float minDistance, float maxDistance);

int J3DAPI SoundDriver_Update3DSound(tSysSoundBuffer** ppDSBuf, float x, float y, float z, float volume, float pitch, tSoundChannelFlag* pChannelFlags, float minRadius, float maxRadius, SoundEnvFlags envflags);

void J3DAPI SoundDriver_SetGlobals(float distanceFactor, float minDistance, float maxDistance, float rolloffFactor, float dopplerFactor);

int SoundDriver_Has3DHW(void);
int SoundDriver_Use3DCaps(void); // Added: From debug version
tDirectSound* SoundDriver_GetDSound(void);

tDirectSound* J3DAPI SoundDriver_CreateDirectSound(int bNoSound3D);
void J3DAPI SoundDriver_ReleaseDirectSound(tDirectSound* pDSound);

void SoundDriver_CreateListener(void);
void SoundDriver_ReleaseListener(void);

void J3DAPI SoundDriver_SetOuputFormat(tDirectSound* pDSound, uint32_t nSamplesPerSec, uint32_t nBitsPerSample, uint32_t numChannels);

tSysSoundBuffer* J3DAPI SoundDriver_GetPrimaryDSBuffer(tDirectSound* pDSound);
void J3DAPI SoundDriver_ReleasePrimaryDSBuffer();

int J3DAPI SoundDriver_GetDecibelVolume(float volume);
int J3DAPI SoundDriver_GetIntPan(float pan);

float J3DAPI SoundDriver_GetVolumeFromDecibels(int dB);
float J3DAPI SoundDriver_GetPanFromTable(int pan); // Added From debug

uint32_t SoundDriver_GetTimeMsec(void);
void J3DAPI SoundDriver_Sleep(unsigned int dwMilliseconds);

// Helper hooking functions
void Driver_InstallHooks(void);
void Driver_ResetGlobals(void);

J3D_EXTERN_C_END
#endif // SOUND_DRIVER_H

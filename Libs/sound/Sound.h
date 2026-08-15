#ifndef SOUND_SOUND_H
#define SOUND_SOUND_H
#include <j3dcore/j3d.h>
#include <sound/AudioLib.h>
#include <sound/types.h>
#include <sound/RTI/addresses.h>

J3D_EXTERN_C_START

#define SOUND_INVALIDHANDLE       0u
#define SOUND_INVALIDGUID         0u

#define SOUND_NOSOUNDINDEX          0x12345678u // Index or ID marking sound not belonging to any specific entity
#define SOUND_INVALISOUNDINFOINDEX  0u

#define SOUND_ALLTHINGSOUNDHANDLE   0u // Handle for all thing sounds, used in functions to modify thing sounds, e.g. Sound_SetVolumeThing

// Added: Describe the four-level subset of AudioLib's seven-bit mouth coordinates returned by Sound_GenerateLipSync.
#define SOUND_LIPSYNC_MOUTH_LEVELS     4u
#define SOUND_LIPSYNC_MOUTH_LEVEL_STEP (AUDIOLIB_LIPSYNC_MOUTH_POSITION_COUNT / SOUND_LIPSYNC_MOUTH_LEVELS)
#define SOUND_LIPSYNC_GETMOUTHLEVEL(mouthPosition) ((mouthPosition) / SOUND_LIPSYNC_MOUTH_LEVEL_STEP)

int J3DAPI Sound_Initialize(tHostServices* pHS);
void Sound_Uninitialize(void);

int Sound_Startup(void);
void Sound_Shutdown(void);

int J3DAPI Sound_Open(SoundOpenFlags flags, size_t maxSoundChannels, SoundGetThingInfoCallback pfGetThingInfo, SoundCalcListenerSoundMixFunc pfCalcListenerSoundMix, HWND hwnd);
void Sound_Close(void);

int J3DAPI Sound_Save(tFileHandle fh);
int J3DAPI Sound_Restore(tFileHandle fh);

void Sound_Pause(void);
void Sound_Resume(void);

void J3DAPI Sound_Set3DGlobals(float distanceFactor, float minDistance, float maxDistance, float folloverFactor, float dopplerFactor);

int Sound_Has3DHW(void);
void J3DAPI Sound_Set3DHWState(int bNo3DSound);
int Sound_Get3DHWState(void);

size_t Sound_GetMemoryUsage(void);
tSoundHandle J3DAPI Sound_Load(const char* filePath, uint32_t* sndIdx); // sndIdx is probably ID that is assign to loaded sound. When sound was previously already loaded then it's index is returned via sndIdx.
tSoundHandle J3DAPI Sound_LoadStatic(const char* pFilename, uint32_t* sndIdx); // sndIdx is probably ID that is assign to loaded sound. When sound was previously already loaded then it's index is returned via sndIdx.

void J3DAPI Sound_Reset(int bClearAll);
void J3DAPI Sound_EnableLoad(int bEnable);

size_t J3DAPI Sound_ExportStaticBank(tFileHandle fh);
int J3DAPI Sound_ImportStaticBank(tFileHandle fh);

size_t J3DAPI Sound_ExportNormalBank(tFileHandle fh);
int J3DAPI Sound_ImportNormalBank(tFileHandle fh);

int J3DAPI Sound_SkipSoundFileSection(tFileHandle fh);

tSoundHandle J3DAPI Sound_GetSoundHandle(uint32_t idx);
size_t J3DAPI Sound_GetSoundIndex(tSoundHandle hSnd);

tSoundChannelHandle J3DAPI Sound_GetChannelHandle(int guid);
int J3DAPI Sound_GetChannelGUID(tSoundChannelHandle hChannel);

const char* J3DAPI Sound_GetSoundFilename(tSoundHandleType handle); // handle is either tSoundChannelHandle or tSoundHandle
size_t J3DAPI Sound_GetLengthMsec(tSoundHandleType handle); // handle is either tSoundChannelHandle or tSoundHandle

void J3DAPI Sound_SetMaxVolume(float maxVolume);
float Sound_GetMaxVolume(void);

tSoundChannelHandle J3DAPI Sound_Play(tSoundHandle hSnd, float volume, float pan, int priority, tSoundChannelFlag flags, int guid);
tSoundChannelHandle J3DAPI Sound_PlayPos(tSoundHandle hSnd, float volume, int priority, tSoundChannelFlag channelflags, float x, float y, float z, int guid, float minRadius, float maxRadius, SoundEnvFlags envflags);
tSoundChannelHandle J3DAPI Sound_PlayThing(tSoundHandle hSnd, float volume, int priority, tSoundChannelFlag flags, int thingID, int guid, float minRadius, float maxRadius);

void J3DAPI Sound_StopChannel(tSoundChannelHandle hChannel);
void Sound_StopAllSounds(void);

void J3DAPI Sound_StopThing(int thingId, tSoundHandleType handle); // handle is either tSoundChannelHandle, tSoundHandle, or SOUND_ALLTHINGSOUNDHANDLE in which case will cause to stop all thing sounds
void J3DAPI Sound_SetVolume(tSoundChannelHandle hChannel, float volume);
void J3DAPI Sound_FadeVolume(tSoundChannelHandle hChannel, float volume, float secFadeTime);

void J3DAPI Sound_SetVolumeThing(int thingId, tSoundHandleType handle, float volume); // handle is either tSoundChannelHandle, tSoundHandle, or SOUND_ALLTHINGSOUNDHANDLE in which case will cause to set volume to all thing sounds
void J3DAPI Sound_FadeVolumeThing(int thingId, tSoundHandleType handle, float startVolume, float endVolume); // handle is either tSoundChannelHandle, or SOUND_ALLTHINGSOUNDHANDLE in which case will cause to fade all thing sounds
int J3DAPI Sound_IsThingFadingVol(int thingId, tSoundHandleType handle); // handle is either tSoundChannelHandle, tSoundHandle, or SOUND_ALLTHINGSOUNDHANDLE in which case will cause to check if any of thing sounds is fading volume

void J3DAPI Sound_SetPan(tSoundChannelHandle hChannel, float volume); // Added: From debug version
float J3DAPI Sound_GetPan(tSoundChannelHandle hChannel); // Added: From debug version

void J3DAPI Sound_SetPitch(tSoundChannelHandle hChannel, float pitch);
float J3DAPI Sound_GetPitch(tSoundChannelHandle hChannel);
void J3DAPI Sound_FadePitch(tSoundChannelHandle hChannel, float pitch, float secFadeTime);

void J3DAPI Sound_SetPitchThing(int thingId, tSoundHandleType handle, float pitch); // handle is either tSoundChannelHandle, tSoundHandle, or SOUND_ALLTHINGSOUNDHANDLE in which case will cause to set pitch to all thing sounds
bool J3DAPI Sound_IsThingFadingPitch(int thingId, tSoundHandleType handle); // handle is either tSoundChannelHandle, tSoundHandle, or SOUND_ALLTHINGSOUNDHANDLE in which case will cause to check if any of thing sounds is fading pitch

tSoundChannelFlag J3DAPI Sound_GetChannelFlags(tSoundChannelHandle hChannel);
bool J3DAPI Sound_GetChannelPlayProgress(tSoundChannelHandle hChannel, float* pProgress); // Added: Get sound channel play progress in percentage. false is returned on when sound is not played or channel doesn't exist

void J3DAPI Sound_Update(const rdVector3* pPos, const rdVector3* pVelocity, const rdVector3* pTopOrient, const rdVector3* pFrontOrient);
int J3DAPI Sound_GenerateLipSync(tSoundChannelHandle hChannel, uint8_t* pMouthPosX, uint8_t* pMouthPosY, int a4);

size_t J3DAPI Sound_GetAllInstanceInfo(SoundInstanceInfo* pCurInstance, size_t sizeInstances); // Added: From debug version
void J3DAPI Sound_SetReverseSound(int bReverse);
void Sound_SoundDump(void); // Added: From debug version

SoundInfo* J3DAPI Sound_GetSoundInfo(tSoundHandle hSnd);
tSoundChannel* J3DAPI Sound_GetChannel(tSoundChannelHandle hChannel);
tSoundChannel* J3DAPI Sound_GetChannelBySoundHandle(tSoundHandle hSnd);

#if defined(J3D_TEST)
typedef void (J3DAPI* SoundTestUncompressFunc)(tAudioCompressorState* pCompressorState, uint8_t* pOutSndData, const uint8_t* pCompressedData, unsigned int size);

// Added: Test-only state and dependency controls for deterministic lip-sync unit tests.
void Sound_TestResetLipSyncState(void);
void Sound_TestSetChannels(tSoundChannel* pChannels, size_t numChannels);
void Sound_TestSetSoundBank(size_t bankNum, SoundInfo* pSoundInfos, size_t numSounds, uint8_t* pSoundCache, size_t cacheSize);
void Sound_TestSetNoLipSync(int bNoLipSync);
void Sound_TestSetCurrentPosition(size_t position);
void Sound_TestSetUncompressFunc(SoundTestUncompressFunc pfUncompress);
#endif

// Helper hooking functions
void Sound_InstallHooks(void);
void Sound_ResetGlobals(void);

J3D_EXTERN_C_END
#endif // SOUND_SOUND_H

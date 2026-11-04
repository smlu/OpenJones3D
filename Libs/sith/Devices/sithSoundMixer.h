#ifndef SITH_SITHSOUNDMIXER_H
#define SITH_SITHSOUNDMIXER_H
#include <j3dcore/j3d.h>
#include <rdroid/types.h>
#include <sith/types.h>
#include <sith/Main/sithMain.h>
#include <std/types.h>

J3D_EXTERN_C_START

#define SITHSOUNDMIXER_CFG_SWMIXER_FALLOFFMODE "sound.swmixer.falloff"

// Added
typedef enum
{
    SITHSOUNDMIXER_SW_FALLOFF_LINEAR      = 0,
    SITHSOUNDMIXER_SW_FALLOFF_EXPONENTIAL = 1,
    SITHSOUNDMIXER_SW_FALLOFF_LOGARITHMIC = 2,

} SithSoundMixerSwFalloffMode;

int sithSoundMixer_Startup(void);
void J3DAPI sithSoundMixer_Shutdown();

void J3DAPI sithSoundMixer_ClearAmbientSector();
void sithSoundMixer_Pause(void);  // Added
void sithSoundMixer_Resume(void); // Added

tSoundChannelHandle J3DAPI sithSoundMixer_PlaySound(tSoundHandle hSnd, float volume, float pan, SoundPlayFlag playflags);
tSoundChannelHandle J3DAPI sithSoundMixer_PlaySoundPos(tSoundHandle hSnd, rdVector3* pos, SithSector* pSector, float volume, float minRadius, float maxRadius, SoundPlayFlag playflags);
tSoundChannelHandle J3DAPI sithSoundMixer_PlaySoundThing(tSoundHandle hSnd, const SithThing* pThing, float volume, float minRadius, float maxRadius, SoundPlayFlag playflags);

void J3DAPI sithSoundMixer_StopSound(tSoundChannelHandle hChannel);
void sithSoundMixer_StopAll(void);

/**
 * Fades sound volume. If new volume is 0.0f the sound will stop playing and be removed from playing channel list
 * @param hChannel
 * @param volume
 * @param secFadeTime
 */
void J3DAPI sithSoundMixer_FadeVolume(tSoundChannelHandle hChannel, float volume, float secFadeTime);
void J3DAPI sithSoundMixer_SetVolume(tSoundChannelHandle hChannel, float volume); // Added

void J3DAPI sithSoundMixer_FadePitch(tSoundChannelHandle hSndChannel, float pitch, float secFadeTime);
void J3DAPI sithSoundMixer_SetPitch(tSoundChannelHandle hChannel, float pitch); // Added

void J3DAPI sithSoundMixer_SetPitchThing(const SithThing* pThing, tSoundHandleType handle, float pitch); // handle is either tSoundChannelHandle, tSoundHandle, or SOUND_ALLTHINGSOUNDHANDLE in which case will cause to set pitch to all thing sounds
void J3DAPI sithSoundMixer_SetVolumeThing(const SithThing* pThing, tSoundHandleType handle, float volume); // handle is either tSoundChannelHandle, tSoundHandle, or SOUND_ALLTHINGSOUNDHANDLE in which case will cause to set volume to all thing sounds
void J3DAPI sithSoundMixer_FadeVolumeThing(const SithThing* pThing, tSoundHandleType handle, float startVolume, float endVolume); // handle is either tSoundChannelHandle, tSoundHandle, or SOUND_ALLTHINGSOUNDHANDLE in which case will cause to fade pitch all thing sounds

int J3DAPI sithSoundMixer_IsThingFadingVol(const SithThing* pThing, tSoundHandleType handle); // handle is either tSoundChannelHandle, tSoundHandle, or SOUND_ALLTHINGSOUNDHANDLE in which case it will check if any of thing sound is fading volume
bool J3DAPI sithSoundMixer_IsThingFadingPitch(SithThing* pThing, tSoundHandleType handle); // Added: From debug version; // handle is either tSoundChannelHandle, tSoundHandle, or SOUND_ALLTHINGSOUNDHANDLE in which case it will check if any of thing sound is fading pitch

void sithSoundMixer_Update(void);

int J3DAPI sithSoundMixer_GetThingInfo(int thingId, SoundThingInfo* pThingInfo);
void J3DAPI sithSoundMixer_CalcCameraRelativeSoundMix(const SoundSpatialInfo* pSpatialInfo, float* volume, float* pan, float* pitch);

void J3DAPI sithSoundMixer_StopAllSoundsThing(const SithThing* pThing);
void J3DAPI sithSoundMixer_StopSoundThing(const SithThing* pThing, tSoundHandleType handle); // handle is either tSoundChannelHandle, tSoundHandle, or SOUND_ALLTHINGSOUNDHANDLE in which case will stop all thing sounds

tSoundChannelHandle J3DAPI sithSoundMixer_GetChannelHandle(int guid);

void J3DAPI sithSoundMixer_SetSectorAmbientSound(SithSector* pSector, tSoundHandle hSnd, float volume);

int J3DAPI sithSoundMixer_GameSave(tFileHandle fh);
signed int J3DAPI sithSoundMixer_GameRestore(tFileHandle fh);

// Helper hooking functions
void sithSoundMixer_ResetGlobals(void);

J3D_EXTERN_C_END
#endif // SITH_SITHSOUNDMIXER_H

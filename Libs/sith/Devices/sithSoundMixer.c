#include "sithSoundMixer.h"
#include <j3dcore/j3dhook.h>

#include <rdroid/Math/rdVector.h>

#include <sith/Engine/sithCamera.h>
#include <sith/Gameplay/sithPlayer.h>
#include <sith/Main/sithMain.h>
#include <sith/RTI/symbols.h>
#include <sith/World/sithThing.h>

#include <sound/Sound.h>

#include <std/General/stdConfig.h>
#include <std/General/stdMath.h>
#include <std/General/stdMemory.h>
#include <std/Win95/stdComm.h>

static bool sithSoundMixer_bStartup = false; // Added: Init. to false
static uint16_t sithSoundMixer_channelGUIDSeed = 0; // Added: Init to 0

static SithSector* sithSoundMixer_pCurSector = NULL; // Added: Init to null
tSoundChannelHandle sithSoundMixer_hCurAmbientChannel;

SithSoundMixerSwFalloffMode sithSoundMixer_swfalloff;

int sithSoundMixer_GetNextChannelGUID(void);
tSoundChannelFlag J3DAPI sithSoundMixer_PlayFlagsToChannelFlags(SoundPlayFlag flags);
int J3DAPI sithSoundMixer_GetPlayPriority(SoundPlayFlag playflags);
int J3DAPI sithSoundMixer_GetThingID(int idx);
int J3DAPI sithSoundMixer_GetThingIndex(int thingID);

void sithSoundMixer_ResetGlobals(void)
{}

int sithSoundMixer_Startup(void)
{
    if ( sithSoundMixer_bStartup )
    {
        SITHLOG_ERROR("Multiple open in soundmix.\n");
        return 1;
    }

    // Added: Added config for sw mixer volume falloff mode
    sithSoundMixer_swfalloff = stdConfig_GetInt(SITHSOUNDMIXER_CFG_SWMIXER_FALLOFFMODE, J3D_QOL_VALUE(SITHSOUNDMIXER_SW_FALLOFF_LOGARITHMIC, SITHSOUNDMIXER_SW_FALLOFF_LINEAR));
    sithSoundMixer_swfalloff = STDMATH_CLAMP(sithSoundMixer_swfalloff, SITHSOUNDMIXER_SW_FALLOFF_LINEAR, SITHSOUNDMIXER_SW_FALLOFF_LOGARITHMIC);
    if ( !stdConfig_Contains(SITHSOUNDMIXER_CFG_SWMIXER_FALLOFFMODE) )
    {
        stdConfig_SetInt(SITHSOUNDMIXER_CFG_SWMIXER_FALLOFFMODE, sithSoundMixer_swfalloff);
    }

    sithSoundMixer_ClearAmbientSector();
    sithSoundMixer_bStartup = true;
    return 0;
}

void sithSoundMixer_Shutdown(void)
{
    if ( sithSoundMixer_bStartup )
    {
        sithSoundMixer_StopAll();
        sithSoundMixer_ClearAmbientSector();
        sithSoundMixer_bStartup = false;
    }
}

void sithSoundMixer_ClearAmbientSector(void)
{
    sithSoundMixer_pCurSector         = NULL;
    sithSoundMixer_hCurAmbientChannel = SOUND_INVALIDHANDLE;
}

void sithSoundMixer_Pause(void)
{
    if ( !stdComm_IsGameActive() )
    {
        Sound_Pause();
    }
}

void sithSoundMixer_Resume(void)
{
    if ( !stdComm_IsGameActive() )
    {
        Sound_Resume();
    }
}

tSoundChannelHandle J3DAPI sithSoundMixer_PlaySound(tSoundHandle hSnd, float volume, float pan, SoundPlayFlag playflags)
{
    if ( !sithSoundMixer_bStartup )
    {
        return 0;
    }

    int guid                = sithSoundMixer_GetNextChannelGUID();
    tSoundChannelFlag flags = sithSoundMixer_PlayFlagsToChannelFlags(playflags);
    int priority            = sithSoundMixer_GetPlayPriority(playflags);
    return Sound_Play(hSnd, volume, pan, priority, flags, guid);
}

tSoundChannelHandle J3DAPI sithSoundMixer_PlaySoundPos(tSoundHandle hSnd, rdVector3* pos, SithSector* pSector, float volume, float minRadius, float maxRadius, SoundPlayFlag playflags)
{
    if ( !sithSoundMixer_bStartup )
    {
        return 0;
    }

    if ( !sithCamera_g_pCurCamera )
    {
        return 0;
    }

    if ( (playflags & SOUNDPLAY_LOOP) == 0 )
    {
        rdVector3 dvec;
        rdVector_Sub3(&dvec, pos, &sithCamera_g_pCurCamera->lookPos);
        if ( rdVector_Normalize3QuickAcc(&dvec) > maxRadius )
        {
            return 0;
        }
    }

    SoundEnvFlags envflags = 0;
    if ( pSector && (pSector->flags & SITH_SECTOR_UNDERWATER) != 0 )
    {
        envflags |= SOUND_ENV_UNDERWATER;
    }

    int guid                = sithSoundMixer_GetNextChannelGUID();
    tSoundChannelFlag flags = sithSoundMixer_PlayFlagsToChannelFlags(playflags);
    int priority            = sithSoundMixer_GetPlayPriority(playflags);
    return Sound_PlayPos(hSnd, volume, priority, flags, pos->x, pos->y, pos->z, guid, minRadius, maxRadius, envflags);
}

tSoundChannelHandle J3DAPI sithSoundMixer_PlaySoundThing(tSoundHandle hSnd, const SithThing* pThing, float volume, float minRadius, float maxRadius, SoundPlayFlag playflags)
{
    SITH_ASSERTREL(pThing);
    if ( !sithSoundMixer_bStartup )
    {
        return 0;
    }

    if ( !sithCamera_g_pCurCamera )
    {
        return 0;
    }

    if ( (playflags & SOUNDPLAY_LOOP) == 0 )
    {
        rdVector3 dvec;
        rdVector_Sub3(&dvec, &pThing->pos, &sithCamera_g_pCurCamera->lookPos);
        if ( rdVector_Normalize3QuickAcc(&dvec) > maxRadius )
        {
            return 0;
        }
    }

    int guid                = sithSoundMixer_GetNextChannelGUID();
    int thingID             = sithSoundMixer_GetThingID(pThing->idx);
    tSoundChannelFlag flags = sithSoundMixer_PlayFlagsToChannelFlags(playflags);
    int priority            = sithSoundMixer_GetPlayPriority(playflags);
    return Sound_PlayThing(hSnd, volume, priority, flags, thingID, guid, minRadius, maxRadius);
}

void J3DAPI sithSoundMixer_StopSound(tSoundChannelHandle hChannel)
{
    Sound_StopChannel(hChannel);
}

void sithSoundMixer_StopAll(void)
{
    Sound_StopAllSounds();
    sithSoundMixer_ClearAmbientSector();
}

void J3DAPI sithSoundMixer_FadeVolume(tSoundChannelHandle hChannel, float volume, float secFadeTime)
{
    Sound_FadeVolume(hChannel, volume, secFadeTime);
}

void J3DAPI sithSoundMixer_SetVolume(tSoundChannelHandle hChannel, float volume)
{
    Sound_SetVolume(hChannel, volume);
}

void J3DAPI sithSoundMixer_FadePitch(tSoundChannelHandle hSndChannel, float pitch, float secFadeTime)
{
    Sound_FadePitch(hSndChannel, pitch, secFadeTime);
}

void J3DAPI sithSoundMixer_SetPitch(tSoundChannelHandle hChannel, float pitch)
{
    Sound_SetPitch(hChannel, pitch);
}

void J3DAPI sithSoundMixer_SetPitchThing(const SithThing* pThing, tSoundHandleType handle, float pitch)
{
    int thingID = sithSoundMixer_GetThingID(pThing->idx);
    Sound_SetPitchThing(thingID, handle, pitch);
}

void J3DAPI sithSoundMixer_SetVolumeThing(const SithThing* pThing, tSoundHandleType handle, float volume)
{
    int thingID = sithSoundMixer_GetThingID(pThing->idx);
    Sound_SetVolumeThing(thingID, handle, volume);
}

void J3DAPI sithSoundMixer_FadeVolumeThing(const SithThing* pThing, tSoundHandleType handle, float startVolume, float endVolume)
{
    int thingID = sithSoundMixer_GetThingID(pThing->idx);
    Sound_FadeVolumeThing(thingID, handle, startVolume, endVolume);
}

int J3DAPI sithSoundMixer_IsThingFadingVol(const SithThing* pThing, tSoundHandleType handle)
{
    int thingID = sithSoundMixer_GetThingID(pThing->idx);
    return Sound_IsThingFadingVol(thingID, handle);
}

bool J3DAPI sithSoundMixer_IsThingFadingPitch(SithThing* pThing, tSoundHandleType handle)
{
    int thingID = sithSoundMixer_GetThingID(pThing->idx);
    return Sound_IsThingFadingPitch(thingID, handle);
}

void sithSoundMixer_Update(void)
{
    if ( !sithCamera_g_pCurCamera || !sithCamera_g_pCurCamera->pSector )
    {
        Sound_Update(NULL, NULL, NULL, NULL);
        return;
    }

    if ( sithCamera_g_pCurCamera->pSector != sithSoundMixer_pCurSector )
    {
        tSoundHandle hSndPrev = SOUND_INVALIDHANDLE;
        float sndVolPrev      = 0.0f;
        if ( sithSoundMixer_pCurSector )
        {
            hSndPrev = sithSoundMixer_pCurSector->hAmbientSound;
            sndVolPrev = sithSoundMixer_pCurSector->ambientSoundVolume;
        }

        sithSoundMixer_pCurSector = sithCamera_g_pCurCamera->pSector;
        tSoundHandle hSnd = sithSoundMixer_pCurSector->hAmbientSound;
        float sndVol = sithSoundMixer_pCurSector->ambientSoundVolume;
        if ( sithSoundMixer_pCurSector->ambientSoundVolume == 0.0f )
        {
            hSnd = SOUND_INVALIDHANDLE;
        }

        if ( hSnd == hSndPrev )
        {
            if ( sndVol != sndVolPrev )
            {
                sithSoundMixer_FadeVolume(sithSoundMixer_hCurAmbientChannel, sndVol, 0.5f);
            }
        }
        else
        {
            if ( sithSoundMixer_hCurAmbientChannel )
            {
                sithSoundMixer_FadeVolume(sithSoundMixer_hCurAmbientChannel, 0.0f, 0.5f);
                sithSoundMixer_hCurAmbientChannel = SOUND_INVALIDHANDLE;
            }

            if ( hSnd )
            {
                sithSoundMixer_hCurAmbientChannel = sithSoundMixer_PlaySound(hSnd, 0.0f, 0.0f, SOUNDPLAY_LOOP);
                if ( sithSoundMixer_hCurAmbientChannel )
                {
                    sithSoundMixer_FadeVolume(sithSoundMixer_hCurAmbientChannel, sndVol, 0.5f);
                }
            }
        }
    }

    rdVector3 fwdDir;
    rdVector_Neg3(&fwdDir, &sithCamera_g_pCurCamera->orient.lvec);
    Sound_Update(&sithCamera_g_pCurCamera->lookPos, NULL, &sithCamera_g_pCurCamera->orient.uvec, &fwdDir);
}

int J3DAPI sithSoundMixer_GetThingInfo(int thingId, SoundThingInfo* pThingInfo)
{
    int thidx = sithSoundMixer_GetThingIndex(thingId);
    SithThing* pThing = sithThing_GetThingByIndex(thidx);
    if ( !pThing )
    {
        return 0;
    }

    pThingInfo->pos = pThing->pos;
    if ( pThing->moveType == SITH_MT_PHYSICS )
    {
        pThingInfo->velocity = pThing->moveInfo.physics.velocity;
    }
    else
    {
        rdVector_Zero3(&pThingInfo->velocity);
    }

    SoundEnvFlags envflags = pThingInfo->envflags;
    if ( (pThing->pInSector->flags & SITH_SECTOR_UNDERWATER) != 0 )
    {
        envflags |= SOUND_ENV_UNDERWATER;
    }
    else
    {
        envflags &= ~SOUND_ENV_UNDERWATER;
    }

    pThingInfo->envflags = envflags;
    return 1;
}

float Atten_Inverse(float distance, float minRadius, float maxRadius, float baseVol)
{
    if ( distance <= minRadius )
        return baseVol;
    if ( distance >= maxRadius )
        return 0.0f;

    // map distance so we avoid volume blow-up
    float d = distance - minRadius;
    float denom = minRadius + d;
    float newVolume = baseVol * (minRadius / denom);
    return newVolume;
}

void J3DAPI sithSoundMixer_CalcCameraRelativeSoundMix(const SoundSpatialInfo* pSpatialInfo, float* volume, float* pan, float* pitch)
{
    float newVolume = pSpatialInfo->volume;
    SoundEnvFlags flags = pSpatialInfo->flags;

    if ( sithCamera_g_pCurCamera )
    {
        rdVector3 pos, dir;
        rdVector_Set3(&pos, pSpatialInfo->pos.x, pSpatialInfo->pos.y, pSpatialInfo->pos.z);
        rdVector_Sub3(&dir, &pos, &sithCamera_g_pCurCamera->lookPos);
        float dist = rdVector_Normalize3QuickAcc(&dir);

        // Behind-listener attenuation
        float dot = rdVector_Dot3(&sithCamera_g_pCurCamera->orient.lvec, &dir);
        if ( dot < 0.0f )
        {
            newVolume = (1.0f - -dot * 0.3f) * newVolume;
        }

        if ( pan )
        {
            *pan = rdVector_Dot3(&sithCamera_g_pCurCamera->orient.rvec, &dir);
        }

        if ( dist < pSpatialInfo->maxRadius && sithCamera_g_pCurCamera->pSector )
        {
            if ( (flags & SOUND_ENV_UNDERWATER) != 0 && (sithCamera_g_pCurCamera->pSector->flags & SITH_SECTOR_UNDERWATER) == 0 )
            {
                dist *= 1.5; // Sound travels 50% "farther" underwater
            }
            else if ( (flags & SOUND_ENV_UNDERWATER) == 0 && (sithCamera_g_pCurCamera->pSector->flags & SITH_SECTOR_UNDERWATER) != 0 )
            {
                dist *= 1.5; // Sound travels 50% "farther" underwater
            }
        }

        if ( volume )
        {
            // Fixed: When minDistDelta <= 0 set the out volume to already modulated `vol`.
            //        OG was assigning pSpatialInfo->volume in this case
            float minDistDelta = dist - pSpatialInfo->minRadius;
            if ( minDistDelta > 0.0 )
            {
                // Altered: Added exponential & logarithmic falloff. OG was only linear
                switch ( sithSoundMixer_swfalloff )
                {
                    case SITHSOUNDMIXER_SW_FALLOFF_LINEAR: // OG
                    {
                        float dminmax = 0.0f;
                        if ( pSpatialInfo->minRadius != pSpatialInfo->maxRadius )
                        {
                            dminmax = 1.0f / (pSpatialInfo->maxRadius - pSpatialInfo->minRadius);
                        }

                        float volLev = STDMATH_CLAMP(minDistDelta * dminmax, 0.0f, 1.0f);
                        newVolume = (1.0f - volLev) * newVolume;
                    } break;
                    case SITHSOUNDMIXER_SW_FALLOFF_EXPONENTIAL:
                    {
                        float range = pSpatialInfo->maxRadius - pSpatialInfo->minRadius;
                        if ( range > 0.0f )
                        {
                            float normalizedDist = minDistDelta / range;
                            normalizedDist = STDMATH_CLAMP(normalizedDist, 0.0f, 1.0f);

                            // Exponential falloff (adjust exponent for steepness: 2.0-4.0 typical)
                            float attenuation = powf(1.0f - normalizedDist, 3.0f);
                            newVolume = attenuation * newVolume;
                        }
                    } break;
                    default: // logarithmic
                    {
                        float range = pSpatialInfo->maxRadius - pSpatialInfo->minRadius;
                        if ( range > 0.0f )
                        {
                            float normalizedDist = minDistDelta / range;
                            normalizedDist = STDMATH_CLAMP(normalizedDist, 0.0f, 1.0f);

                            // Logarithmic falloff (simulates decibels)
                            // -40dB at max distance (barely audible)
                            float dbAttenuation = -42.0f * normalizedDist;
                            float attenuation = powf(10.0f, dbAttenuation / 20.0f);
                            newVolume = attenuation * newVolume;
                        }
                    } break;
                };
            }

            *volume = newVolume;
        }

        if ( pitch )
        {
            *pitch = 1.0f;
        }
    }
}

void J3DAPI sithSoundMixer_StopAllSoundsThing(const SithThing* pThing)
{
    int thingID = sithSoundMixer_GetThingID(pThing->idx);
    Sound_StopThing(thingID, SOUND_ALLTHINGSOUNDHANDLE);
}

tSoundChannelHandle J3DAPI sithSoundMixer_GetChannelHandle(int guid)
{
    if ( guid )
    {
        return Sound_GetChannelHandle(guid);
    }

    return 0;
}

void J3DAPI sithSoundMixer_StopSoundThing(const SithThing* pThing, tSoundHandleType handle)
{
    int thingID = sithSoundMixer_GetThingID(pThing->idx);
    Sound_StopThing(thingID, handle);
}

void J3DAPI sithSoundMixer_SetSectorAmbientSound(SithSector* pSector, tSoundHandle hSnd, float volume)
{
    SITH_ASSERTREL(pSector);

    pSector->hAmbientSound      = hSnd;
    pSector->ambientSoundVolume = volume;
    if ( sithSoundMixer_pCurSector == pSector )
    {
        sithSoundMixer_pCurSector = NULL;
    }
}

int J3DAPI sithSoundMixer_GameSave(tFileHandle fh)
{
    J3D_ASSERT_SIZE32(sizeof(sithSoundMixer_hCurAmbientChannel) == 4, "sizeof(sithSoundMixer_hCurAmbientChannel) == 4");
    return sith_g_pHS->pFileWrite(fh, &sithSoundMixer_hCurAmbientChannel, sizeof(sithSoundMixer_hCurAmbientChannel)) != sizeof(sithSoundMixer_hCurAmbientChannel)
        || Sound_Save(fh) == 0;
}

int J3DAPI sithSoundMixer_GameRestore(tFileHandle fh)
{
    sithSoundMixer_StopAll();

    tSoundChannelHandle hAmbChannel;
    if ( sith_g_pHS->pFileRead(fh, &hAmbChannel, sizeof(sithSoundMixer_hCurAmbientChannel)) != sizeof(sithSoundMixer_hCurAmbientChannel) )
    {
        return 1;
    }

    sithSoundMixer_ClearAmbientSector();

    if ( !Sound_Restore(fh) )
    {
        return 1;
    }

    Sound_StopChannel(hAmbChannel); // TODO: Why hAmbChannel is not set to sithSoundMixer_hCurAmbientChannel?
    return 0;
}

int sithSoundMixer_GetNextChannelGUID(void)
{
    if ( !sithSoundMixer_channelGUIDSeed )
    {
        sithSoundMixer_channelGUIDSeed = 1;
    }

    return sithSoundMixer_channelGUIDSeed++ | ((sithPlayer_g_playerNum + 1) << 16);
}

tSoundChannelFlag J3DAPI sithSoundMixer_PlayFlagsToChannelFlags(SoundPlayFlag flags)
{
    tSoundChannelFlag chflags = (flags & SOUNDPLAY_LOOP) != 0 ? SOUND_CHANNEL_LOOP | SOUND_CHANNEL_PLAYING : SOUND_CHANNEL_PLAYING;
    if ( (flags & SOUNDPLAY_PLAYONCE) != 0 )
    {
        chflags |= SOUND_CHANNEL_PLAYONCE;
    }

    if ( (flags & SOUNDPLAY_PLAYTHINGONCE) != 0 )
    {
        chflags |= SOUND_CHANNEL_PLAYTHINGONCE;
    }

    return chflags;
}

int J3DAPI sithSoundMixer_GetPlayPriority(SoundPlayFlag playflags)
{
    switch ( playflags & (SOUNDPLAY_HIGHEST_PRIORITY | SOUNDPLAY_HIGH_PRIORITY) )
    {
        case SOUNDPLAY_HIGH_PRIORITY:
            return 500;

        case SOUNDPLAY_HIGHEST_PRIORITY:
            return 1000;

        case SOUNDPLAY_HIGHEST_PRIORITY | SOUNDPLAY_HIGH_PRIORITY:
            return 2000;
    }

    return 0;
}

int J3DAPI sithSoundMixer_GetThingID(int idx)
{
    return idx + 1;
}

int J3DAPI sithSoundMixer_GetThingIndex(int thingID)
{
    return thingID - 1;
}

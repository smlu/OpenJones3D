// Added: Null version of Libs/sound/DriverDX9.c (the DirectSound driver). There is no sound
// device: SoundDriver_Open succeeds, CreateAndPlay hands out a small malloc'd handle that
// Release frees, playback never starts (Stop/Play are no-ops) and GetCurrentPosition stays
// 0, so every sound "finishes" immediately. The settable state (volume, pan, frequency) is
// kept per handle like the DirectSound buffer state, and the listener mix path mirrors
// DriverDX9.c's software path because there is never 3D hardware here.
#include <j3dcore/j3d.h>

#include <math.h>

#include <sound/Driver.h>
#include <sound/AudioLib.h>

#include <rdroid/Math/rdVector.h>

#include <std/General/std.h>
#include <std/General/stdMemory.h>
#include <std/General/stdPlatform.h>

#define SOUNDDRIVER_FARVOLUMETHRESHOLD 0.01f
#define SOUNDDRIVER_DEFAULT_FREQUENCY  22050.0f

// Added: The opaque backend types for this backend: small malloc'd blocks instead of
// DirectSound COM buffers, with the per-buffer state the engine queries back.
struct sSysSoundBuffer
{
    float volume;     // 0 dB = 1.0f, like a fresh DirectSoundBuffer
    float pan;
    float frequency;  // SOUNDDRIVER_DEFAULT_FREQUENCY on a fresh buffer
};

struct sSysSoundDevice
{
    int unused;
};

static tDirectSound* SoundDriver_pDirectSound          = NULL;
static bool SoundDriver_bDSoundNotCreated              = false;
static float SoundDriver_maxVolume                     = 1.0f;
static rdVector3 SoundDriver_curListenerPos            = { 0 };
static float SoundDriver_frontOrientATan               = 0.0f;
static SoundCalcListenerSoundMixFunc SoundDriver_pfCalcListenerSoundMix;
static SoundDriverGetSoundBufferDataFunc SoundDriver_pfGetSoundBufferData;
static tPrintfFunc SoundDriver_pfLogError;

// Added: The listener mix callback bridge of DriverDX9.c's CalcListeneSoundMix: the engine's
// callback decides volume/pan/pitch; without one (the engine always installs one) the
// original values pass through unchanged.
static void J3DAPI SoundDriver_CalcListeneSoundMix(float x, float y, float z, float volume, float pitch, float* newVolume, float* pan, float* newPitch, float minRadius, float maxRadius, SoundEnvFlags envflags)
{
    J3D_UNUSED(x);
    J3D_UNUSED(y);
    J3D_UNUSED(z);
    J3D_UNUSED(SoundDriver_frontOrientATan);

    if ( !newVolume || !pan || !newPitch )
    {
        return;
    }

    *newVolume = volume;
    *pan       = 0.0f;
    *newPitch  = pitch;

    if ( !SoundDriver_pfCalcListenerSoundMix )
    {
        return;
    }

    SoundSpatialInfo spatialInfo = { 0 };
    spatialInfo.pos.x     = x;
    spatialInfo.pos.y     = y;
    spatialInfo.pos.z     = z;
    spatialInfo.volume    = volume;
    spatialInfo.pitch     = pitch;
    spatialInfo.minRadius = minRadius;
    spatialInfo.maxRadius = maxRadius;

    // Added: DriverDX9.c fills envflags/flags here; SoundSpatialInfo owns both spellings.
    spatialInfo.flags = envflags;
    SoundDriver_pfCalcListenerSoundMix(&spatialInfo, newVolume, pan, newPitch);
}

int J3DAPI SoundDriver_Open(int bNoSound3D, int bGlobalFocus, tSysWindow hwnd, tDirectSound* pDirectSound, SoundCalcListenerSoundMixFunc pfCalcListenerSoundMix, SoundDriverGetSoundBufferDataFunc pfGetSoundBufferData, tPrintfFunc pfLogError)
{
    J3D_UNUSED(bNoSound3D);
    J3D_UNUSED(bGlobalFocus);
    J3D_UNUSED(hwnd);

    SoundDriver_pfLogError             = pfLogError;
    SoundDriver_pfCalcListenerSoundMix = pfCalcListenerSoundMix;
    SoundDriver_pfGetSoundBufferData   = pfGetSoundBufferData;

    if ( pDirectSound )
    {
        // Added: Same ownership convention as DriverDX9.c: a passed-in device is not owned.
        SoundDriver_pDirectSound      = pDirectSound;
        SoundDriver_bDSoundNotCreated = true;
    }
    else
    {
        SoundDriver_pDirectSound = (tDirectSound*)STDMALLOC(sizeof(struct sSysSoundDevice));
        if ( !SoundDriver_pDirectSound )
        {
            return 0;
        }

        SoundDriver_bDSoundNotCreated = false;
    }

    // Returns the same "device is usable" answer DriverDX9.c returns on success.
    return SoundDriver_pDirectSound != NULL;
}

void SoundDriver_Close(void)
{
    if ( SoundDriver_pDirectSound && !SoundDriver_bDSoundNotCreated )
    {
        STDFREE(SoundDriver_pDirectSound);
    }

    SoundDriver_pDirectSound      = NULL;
    SoundDriver_bDSoundNotCreated = false;
}

void SoundDriver_SetMaxVolume(float volume)
{
    J3D_UNUSED(volume);
}

tSysSoundBuffer* J3DAPI SoundDriver_CreateAndPlay(size_t samplesPerSec, size_t nBitsPerSample, size_t numChannels, const uint8_t* pSoundData, size_t dataSize, tSoundChannelFlag* pFlags, tSysSoundBuffer* pSBuffer)
{
    J3D_UNUSED(samplesPerSec);
    J3D_UNUSED(nBitsPerSample);
    J3D_UNUSED(numChannels);
    J3D_UNUSED(dataSize);
    J3D_UNUSED(pSoundData);

    if ( !pFlags )
    {
        return NULL;
    }

    // Added: DriverDX9.c duplicates a passed-in pSBuffer; there are no hardware buffers to
    // duplicate on this platform, so a fresh software handle is created either way.
    J3D_UNUSED(pSBuffer);

    tSysSoundBuffer* pBuf = (tSysSoundBuffer*)STDMALLOC(sizeof(struct sSysSoundBuffer));
    if ( !pBuf )
    {
        return NULL;
    }

    pBuf->volume    = 1.0f; // 0 dB, the fresh DirectSoundBuffer volume
    pBuf->pan       = 0.0f;
    pBuf->frequency = SOUNDDRIVER_DEFAULT_FREQUENCY;

    // Added: The buffer is always software-mixed here and never starts playing;
    // GetStatusAndCaps reports it stopped, so the engine ends the channel at once.
    *pFlags |= SOUND_CHANNEL_SWMIXING;
    return pBuf;
}

void J3DAPI SoundDriver_Release(tSysSoundBuffer* pDSBuf)
{
    if ( pDSBuf )
    {
        STDFREE(pDSBuf);
    }
}

void J3DAPI SoundDriver_Play(tSysSoundBuffer* pDSBuf, int bLoop)
{
    J3D_UNUSED(pDSBuf);
    J3D_UNUSED(bLoop);
}

void J3DAPI SoundDriver_Stop(tSysSoundBuffer* pDSBuf)
{
    J3D_UNUSED(pDSBuf);
}

void J3DAPI SoundDriver_SetVolume(tSysSoundBuffer* pDSBuf, float volume)
{
    if ( pDSBuf )
    {
        pDSBuf->volume = volume;
    }
}

float J3DAPI SoundDriver_GetVolume(tSysSoundBuffer* pDSBuf)
{
    if ( !pDSBuf )
    {
        return 1.0f;
    }

    return pDSBuf->volume;
}

void J3DAPI SoundDriver_SetPan(tSysSoundBuffer* pDSoundBuf, float pan)
{
    if ( pDSoundBuf )
    {
        pDSoundBuf->pan = pan;
    }
}

float J3DAPI SoundDriver_GetPan(tSysSoundBuffer* pDSBuf)
{
    if ( !pDSBuf )
    {
        return 0.0f;
    }

    return pDSBuf->pan;
}

void J3DAPI SoundDriver_SetFrequency(tSysSoundBuffer* pDSBuf, float freq)
{
    if ( pDSBuf )
    {
        // Added: Same frequency clamp as DriverDX9.c's SetFrequency.
        pDSBuf->frequency = freq < SOUNDDRIVER_DEFAULT_FREQUENCY ? SOUNDDRIVER_DEFAULT_FREQUENCY : freq;
    }
}

float J3DAPI SoundDriver_GetFrequency(tSysSoundBuffer* pDSBuf)
{
    if ( !pDSBuf )
    {
        return SOUNDDRIVER_DEFAULT_FREQUENCY;
    }

    return pDSBuf->frequency;
}

size_t J3DAPI SoundDriver_GetCurrentPosition(tSysSoundBuffer* pDSoundBuf)
{
    J3D_UNUSED(pDSoundBuf);
    return 0;
}

tSoundChannelFlag J3DAPI SoundDriver_GetStatusAndCaps(tSysSoundBuffer* pDSBuffer)
{
    if ( !pDSBuffer )
    {
        return (tSoundChannelFlag)0;
    }

    // Added: Same caps flags DriverDX9.c derives: always software-mixed and the stored
    // global-focus state, but never PLAYING/LOOPING: the buffer is always stopped.
    tSoundChannelFlag flags = SOUND_CHANNEL_SWMIXING | SOUND_CHANNEL_GLOBALFOCUS;
    return flags;
}

void J3DAPI SoundDriver_SetListenerPosition(const rdVector3* pPos, const rdVector3* pVelocity, const rdVector3* pTopOrient, const rdVector3* pFrontOrient)
{
    J3D_UNUSED(pVelocity);
    J3D_UNUSED(pTopOrient);

    if ( pPos )
    {
        rdVector_Copy3(&SoundDriver_curListenerPos, pPos);
    }

    // Added: Same fallback pan-orientation math as DriverDX9.c for the software mix.
    if ( !SoundDriver_pfCalcListenerSoundMix && pFrontOrient )
    {
        SoundDriver_frontOrientATan = pFrontOrient->x == 0.0f ? 0.0f : atan2f(pFrontOrient->z, pFrontOrient->x);
    }
}

void SoundDriver_ListenerCommitDeferred(void)
{
}

void J3DAPI SoundDriver_SetPosAndVelocity(tSysSoundBuffer* pBuffer, float x, float y, float z, float velX, float velY, float velZ, float minDistance, float maxDistance)
{
    J3D_UNUSED(pBuffer);
    J3D_UNUSED(x);
    J3D_UNUSED(y);
    J3D_UNUSED(z);
    J3D_UNUSED(velX);
    J3D_UNUSED(velY);
    J3D_UNUSED(velZ);
    J3D_UNUSED(minDistance);
    J3D_UNUSED(maxDistance);
}

int J3DAPI SoundDriver_Update3DSound(tSysSoundBuffer** ppDSBuf, float x, float y, float z, float volume, float pitch, tSoundChannelFlag* pChannelFlags, float minRadius, float maxRadius, SoundEnvFlags envflags)
{
    tSysSoundBuffer* pDSBuf = ppDSBuf ? *ppDSBuf : NULL;

    // Added: No 3D hardware, so always the software mix path of DriverDX9.c Update3DSound.
    float newVolume = volume;
    float pan       = 0.0f;
    float newPitch  = pitch;

    SoundDriver_CalcListeneSoundMix(x, y, z, volume, pitch, &newVolume, &pan, &newPitch, minRadius, maxRadius, envflags);

    if ( newVolume < SOUNDDRIVER_FARVOLUMETHRESHOLD
        && (*pChannelFlags & (SOUND_CHANNEL_LOOP | SOUND_CHANNEL_PLAYING)) != (SOUND_CHANNEL_LOOP | SOUND_CHANNEL_PLAYING) )
    {
        return 0;
    }

    SoundDriver_SetVolume(pDSBuf, newVolume * SoundDriver_maxVolume);
    SoundDriver_SetPan(pDSBuf, pan);
    if ( newPitch != 1.0f )
    {
        float freq = SoundDriver_GetFrequency(pDSBuf);
        freq *= newPitch / pitch;
        SoundDriver_SetFrequency(pDSBuf, freq);
    }

    // Added: The DriverDX9.c 3D-channel stop branches all require SOUND_CHANNEL_3DSOUND,
    // which this backend never sets, so the software path ends with the far-restart check.
    if ( (*pChannelFlags & SOUND_CHANNEL_FAR) != 0 && newVolume > SOUNDDRIVER_FARVOLUMETHRESHOLD )
    {
        *pChannelFlags |= SOUND_CHANNEL_RESTART;
        return 1;
    }

    return 1;
}

void J3DAPI SoundDriver_SetGlobals(float distanceFactor, float minDistance, float maxDistance, float rolloffFactor, float dopplerFactor)
{
    J3D_UNUSED(distanceFactor);
    J3D_UNUSED(minDistance);
    J3D_UNUSED(maxDistance);
    J3D_UNUSED(rolloffFactor);
    J3D_UNUSED(dopplerFactor);
}

int SoundDriver_Has3DHW(void)
{
    return 0;
}

int SoundDriver_Use3DCaps(void)
{
    return 0;
}

tDirectSound* SoundDriver_GetDSound(void)
{
    return SoundDriver_pDirectSound;
}

uint32_t SoundDriver_GetTimeMsec(void)
{
    return stdPlatform_GetTimeMsec();
}

void J3DAPI SoundDriver_Sleep(unsigned int dwMilliseconds)
{
    // Added: DriverDX9.c sleeps on the Windows Sleep; the Null platform layer owns the wait.
    stdPlatform_Sleep(dwMilliseconds);
}

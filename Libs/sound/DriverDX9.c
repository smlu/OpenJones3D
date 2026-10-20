#include "Driver.h"
#include <j3dcore/j3dhook.h>

#include <rdroid/Math/rdVector.h>

#include <sound/RTI/symbols.h>

#include <sound/AudioLib.h>

#include <std/General/stdMath.h>
#include <std/General/stdPlatform.h>
#include <std/General/stdUtil.h>

#include <stdarg.h>
#include <stdio.h>

#define SOUNDDRIVER_FARVOLUMETHRESHOLD 0.01f

static float SoundDriver_maxVolume = 1.0f;
static float Sound_defaultDistanceFactor = 0.30000001f;
static float SoundDriver_defaultRolloffFactor = 1.0f;
static float SoundDriver_defaultDopplerFactor = 1.0f;

static float SoundDriver_minDistance = 15.0f;
static float SoundDriver_maxDistance = 10000.0f;
static float SoundDriver_defaultDistanceFactor = 0.30000001f;

static HWND SoundDriver_hwnd;
static tDirectSound* SoundDriver_pDirectSound = NULL; // Added: Init to NULL
static tSysSoundBuffer* SoundDriver_pDSBuffer;

static int SoundDriver_aPanTable[256];

static int SoundDriver_bNoDSound3D;
static int SoundDriver_bUseGlobalFocusBuf;

static int SoundDriver_bDSoundNotCreated;

static tPrintfFunc SoundDriver_pfLogError;
static SoundCalcListenerSoundMixFunc SoundDriver_pfCalcListenerSoundMix;
static SoundDriverGetSoundBufferDataFunc SoundDriver_pfGetSoundBufferData;

static LPDIRECTSOUND3DLISTENER8 SoundDriver_pDS3DListener;
static rdVector3 SoundDriver_curListenerPos;
static float SoundDriver_frontOrientATan;

static int SoundDriver_bUse3DCaps;
static int SoundDriver_bHas3DHW;

static const int SoundDriver_aDBTable[256] = {
    -9640, -4810, -4210, -3850, -3600, -3410, -3250, -3120, -3000, -2900, -2810, -2730, -2650, -2580, -2520, -2460,
    -2400, -2350, -2300, -2250, -2210, -2160, -2120, -2080, -2050, -2010, -1980, -1950, -1910, -1880, -1850, -1830,
    -1800, -1770, -1750, -1720, -1700, -1670, -1650, -1630, -1600, -1580, -1560, -1540, -1520, -1500, -1480, -1460,
    -1450, -1430, -1410, -1390, -1380, -1360, -1340, -1330, -1310, -1300, -1280, -1270, -1250, -1240, -1220, -1210,
    -1200, -1180, -1170, -1160, -1140, -1130, -1120, -1110, -1090, -1080, -1070, -1060, -1050, -1040, -1020, -1010,
    -1000,  -990,  -980,  -970,  -960,  -950,  -940,  -930,  -920,  -910,  -900,  -890,  -880,  -870,  -860,  -850,
     -840,  -830,  -830,  -820,  -810,  -800,  -790,  -780,  -770,  -770,  -760,  -750,  -740,  -730,  -730,  -720,
     -710,  -700,  -690,  -690,  -680,  -670,  -660,  -660,  -650,  -640,  -640,  -630,  -620,  -610,  -610,  -600,
     -590,  -590,  -580,  -570,  -570,  -560,  -550,  -550,  -540,  -530,  -530,  -520,  -520,  -510,  -500,  -500,
     -490,  -490,  -480,  -470,  -470,  -460,  -460,  -450,  -440,  -440,  -430,  -430,  -420,  -420,  -410,  -410,
     -400,  -390,  -390,  -380,  -380,  -370,  -370,  -360,  -360,  -350,  -350,  -340,  -340,  -330,  -330,  -320,
     -320,  -310,  -310,  -300,  -300,  -290,  -290,  -280,  -280,  -270,  -270,  -260,  -260,  -260,  -250,  -250,
     -240,  -240,  -230,  -230,  -220,  -220,  -210,  -210,  -210,  -200,  -200,  -190,  -190,  -180,  -180,  -180,
     -170,  -170,  -160,  -160,  -160,  -150,  -150,  -140,  -140,  -140,  -130,  -130,  -120,  -120,  -120,  -110,
     -110,  -100,  -100,  -100,   -90,   -90,   -80,   -80,   -80,   -70,   -70,   -70,   -60,   -60,   -50,   -50,
      -50,   -40,   -40,   -40,   -30,   -30,   -30,   -20,   -20,   -20,   -10,   -10,   -10,   -10,   -10,   -10
};

const DXStatus SoundDriver_aDStatusTbl[16] =
{
  { DSERR_ALLOCATED         , "DSERR_ALLOCATED"          },
  { DSERR_ALREADYINITIALIZED, "DSERR_ALREADYINITIALIZED" },
  { DSERR_BADFORMAT         , "DSERR_BADFORMAT"          },
  { DSERR_BUFFERLOST        , "DSERR_BUFFERLOST"         },
  { DSERR_CONTROLUNAVAIL    , "DSERR_CONTROLUNAVAIL"     },
  { DSERR_GENERIC           , "DSERR_GENERIC"            },
  { DSERR_INVALIDCALL       , "DSERR_INVALIDCALL"        },
  { DSERR_INVALIDPARAM      , "DSERR_INVALIDPARAM"       },
  { DSERR_NOAGGREGATION     , "DSERR_NOAGGREGATION"      },
  { DSERR_NODRIVER          , "DSERR_NODRIVER"           },
  { DSERR_OTHERAPPHASPRIO   , "DSERR_OTHERAPPHASPRIO"    },
  { DSERR_OUTOFMEMORY       , "DSERR_OUTOFMEMORY"        },
  { DSERR_PRIOLEVELNEEDED   , "DSERR_PRIOLEVELNEEDED"    },
  { DSERR_UNINITIALIZED     , "DSERR_UNINITIALIZED"      },
  { DSERR_UNSUPPORTED       , "DSERR_UNSUPPORTED"        },
  { 0                       , "Unknown error code."      }
};

static void SoundDriver_InitPanTable(void);
static HRESULT J3DAPI SoundDriver_DSCheckStatus(HRESULT code, size_t codeLine);

static void J3DAPI SoundDriver_CalcListeneSoundMix(float x, float y, float z, float volume, float pitch, float* newVolume, float* pan, float* newPitch, float minRadius, float maxRadius, SoundEnvFlags envflags);
static double J3DAPI SoundDriver_CalcDistToListener(float x1, float y1, float z1, float x2, float y2, float z2);

static void J3DAPI SoundDriver_RestoreSoundBufferData(tSysSoundBuffer* pDSBuf);

static HRESULT J3DAPI SoundDriver_CreateSoundBuffer(tDirectSound* pDSound, const DSBUFFERDESC* dsbd, tSysSoundBuffer** pSndBuf);

void SoundDriver_InstallHooks(void)
{
    J3D_HOOKFUNC(SoundDriver_Open);
    J3D_HOOKFUNC(SoundDriver_Close);
    J3D_HOOKFUNC(SoundDriver_SetMaxVolume);
    J3D_HOOKFUNC(SoundDriver_CreateAndPlay);
    J3D_HOOKFUNC(SoundDriver_Release);
    J3D_HOOKFUNC(SoundDriver_Play);
    J3D_HOOKFUNC(SoundDriver_Stop);
    J3D_HOOKFUNC(SoundDriver_SetVolume);
    J3D_HOOKFUNC(SoundDriver_GetVolume);
    J3D_HOOKFUNC(SoundDriver_SetPan);
    J3D_HOOKFUNC(SoundDriver_SetFrequency);
    J3D_HOOKFUNC(SoundDriver_GetFrequency);
    J3D_HOOKFUNC(SoundDriver_GetCurrentPosition);
    J3D_HOOKFUNC(SoundDriver_GetStatusAndCaps);
    J3D_HOOKFUNC(SoundDriver_SetListenerPosition);
    J3D_HOOKFUNC(SoundDriver_ListenerCommitDeferred);
    J3D_HOOKFUNC(SoundDriver_SetPosAndVelocity);
    J3D_HOOKFUNC(SoundDriver_Update3DSound);
    J3D_HOOKFUNC(SoundDriver_SetGlobals);
    J3D_HOOKFUNC(SoundDriver_Use3DCaps);
    J3D_HOOKFUNC(SoundDriver_GetDSound);
    J3D_HOOKFUNC(SoundDriver_CreateDirectSound);
    J3D_HOOKFUNC(SoundDriver_ReleaseDirectSound);
    J3D_HOOKFUNC(SoundDriver_CreateListener);
    J3D_HOOKFUNC(SoundDriver_ReleaseListener);
    J3D_HOOKFUNC(SoundDriver_SetOuputFormat);
    J3D_HOOKFUNC(SoundDriver_GetPrimaryDSBuffer);
    J3D_HOOKFUNC(SoundDriver_ReleasePrimaryDSBuffer);
    J3D_HOOKFUNC(SoundDriver_GetDecibelVolume);
    J3D_HOOKFUNC(SoundDriver_GetIntPan);
    J3D_HOOKFUNC(SoundDriver_GetVolumeFromDecibels);
    J3D_HOOKFUNC(SoundDriver_InitPanTable);
    J3D_HOOKFUNC(SoundDriver_DSCheckStatus);
    J3D_HOOKFUNC(SoundDriver_CalcListeneSoundMix);
    J3D_HOOKFUNC(SoundDriver_CalcDistToListener);
    J3D_HOOKFUNC(SoundDriver_RestoreSoundBufferData);
    J3D_HOOKFUNC(SoundDriver_Sleep);
}

void SoundDriver_ResetGlobals(void)
{}

static void SoundDriver_LogError(const char* pFormat, ...)
{
    if ( SoundDriver_pfLogError )
    {
        // Fixed: Format the message into a buffer, a va_list was passed as a variadic argument of the log function (undefined output or crash)
        char aMessage[1024];

        va_list args;
        va_start(args, pFormat);
        vsnprintf(aMessage, sizeof(aMessage), pFormat, args);
        va_end(args);

        aMessage[sizeof(aMessage) - 1] = '\0';
        SoundDriver_pfLogError("%s", aMessage);
    }
}

HRESULT J3DAPI SoundDriver_CreateSoundBuffer(tDirectSound* pDSound, const DSBUFFERDESC* dsbd, tSysSoundBuffer** pSndBuf)
{
    LPDIRECTSOUNDBUFFER pTmpBuf;
    HRESULT hr = IDirectSound8_CreateSoundBuffer(pDSound, dsbd, &pTmpBuf, NULL);
    if ( FAILED(hr) || !pTmpBuf )
    {
        pSndBuf = NULL;
        return hr;
    }

    hr = IDirectSoundBuffer_QueryInterface(pTmpBuf, &IID_IDirectSoundBuffer8, (LPVOID*)pSndBuf);
    IDirectSoundBuffer_Release(pTmpBuf);

    if ( FAILED(hr) )
    {
        pSndBuf = NULL;
    }

    return hr;
}

HRESULT J3DAPI SoundDriver_DuplicateSoundBuffer(tDirectSound* pDSound, tSysSoundBuffer* pSrcSndBuf, tSysSoundBuffer** pDestSndBuf)
{
    LPDIRECTSOUNDBUFFER pTmpBuf;
    HRESULT hr = IDirectSound8_DuplicateSoundBuffer(pDSound, (LPDIRECTSOUNDBUFFER)pSrcSndBuf, &pTmpBuf);
    if ( FAILED(hr) || !pTmpBuf )
    {
        pDestSndBuf = NULL;
        return hr;
    }

    hr = IDirectSoundBuffer_QueryInterface(pTmpBuf, &IID_IDirectSoundBuffer8, (LPVOID*)pDestSndBuf);
    IDirectSoundBuffer_Release(pTmpBuf);

    if ( FAILED(hr) )
    {
        pDestSndBuf = NULL;
    }

    return hr;
}

int J3DAPI SoundDriver_Open(int bNoSound3D, int bGlobalFocus, HWND hwnd, tDirectSound* pDirectSound, SoundCalcListenerSoundMixFunc pfCalcListenerSoundMix, SoundDriverGetSoundBufferDataFunc pfGetSoundBufferData, tPrintfFunc pfLogError)
{
    SoundDriver_pfLogError         = pfLogError;
    SoundDriver_bNoDSound3D        = bNoSound3D;
    SoundDriver_bUseGlobalFocusBuf = bGlobalFocus;
    SoundDriver_hwnd               = hwnd;

    SoundDriver_InitPanTable();

    if ( pDirectSound )
    {
        SoundDriver_pDirectSound      = pDirectSound;
        SoundDriver_bDSoundNotCreated = 1;
    }
    else
    {
        SoundDriver_pDirectSound = SoundDriver_CreateDirectSound(SoundDriver_bNoDSound3D);
    }

    if ( SoundDriver_pDirectSound )
    {
        SoundDriver_CreateListener();
        SoundDriver_SetOuputFormat(SoundDriver_pDirectSound, 22050u, 16u, 2);
    }

    SoundDriver_pfCalcListenerSoundMix = pfCalcListenerSoundMix;
    SoundDriver_pfGetSoundBufferData   = pfGetSoundBufferData;
    return SoundDriver_pDirectSound != NULL;
}

void SoundDriver_Close(void)
{
    SoundDriver_ReleaseListener();
    if ( !SoundDriver_bDSoundNotCreated )
    {
        SoundDriver_ReleaseDirectSound(SoundDriver_pDirectSound);
    }

    SoundDriver_pDirectSound = NULL;
}

void J3DAPI SoundDriver_SetMaxVolume(float volume)
{
    SoundDriver_maxVolume = volume;
}

tSysSoundBuffer* J3DAPI SoundDriver_CreateAndPlay(size_t samplesPerSec, size_t nBitsPerSample, size_t numChannels, const uint8_t* pSoundData, size_t dataSize, tSoundChannelFlag* pFlags, tSysSoundBuffer* pSBuffer)
{
    LPDIRECTSOUNDBUFFER8 pDSBufferNew = NULL;

    if ( pSBuffer )
    {
        //HRESULT dsres = IDirectSound8_DuplicateSoundBuffer(SoundDriver_pDirectSound, (LPDIRECTSOUNDBUFFER)pSBuffer, (LPDIRECTSOUNDBUFFER*)&pDSBufferNew);
        HRESULT dsres = SoundDriver_DuplicateSoundBuffer(SoundDriver_pDirectSound, pSBuffer, &pDSBufferNew);
        if ( SoundDriver_DSCheckStatus(dsres, __LINE__) == DS_OK )
        {
            dsres = IDirectSoundBuffer8_SetCurrentPosition(pDSBufferNew, 0);
            SoundDriver_DSCheckStatus(dsres, __LINE__);
            if ( SoundDriver_bUse3DCaps )
            {
                DSBCAPS dsbcaps;
                dsbcaps.dwSize = sizeof(DSBCAPS);
                dsres = IDirectSoundBuffer8_GetCaps(pDSBufferNew, &dsbcaps);
                if ( SoundDriver_DSCheckStatus(dsres, __LINE__) != DS_OK )
                {
                    if ( pDSBufferNew )
                    {
                        IDirectSoundBuffer8_Release(pDSBufferNew);
                    }

                    return NULL;
                }

                if ( (dsbcaps.dwFlags & DSBCAPS_LOCHARDWARE) == 0 )
                {
                    if ( pDSBufferNew )
                    {
                        IDirectSoundBuffer8_Release(pDSBufferNew);
                    }

                    return NULL;
                }

                *pFlags |= SOUND_CHANNEL_HWMIXING;
            }

            *pFlags |= SOUND_CHANNEL_UNKNOWN_800000;
            goto play;
        }
    }

    if ( (*pFlags & SOUND_CHANNEL_COMPRESSED) != 0 )
    {
        dataSize = ((tAudioCompressedData*)pSoundData)->uncompressedSize;
    }

    WAVEFORMATEX wavefmt;
    wavefmt.wFormatTag      = WAVE_FORMAT_PCM;
    wavefmt.nChannels       = numChannels;
    wavefmt.nSamplesPerSec  = samplesPerSec;
    wavefmt.nAvgBytesPerSec = numChannels * (nBitsPerSample / 8) * samplesPerSec;
    wavefmt.nBlockAlign     = numChannels * (nBitsPerSample / 8);
    wavefmt.wBitsPerSample  = nBitsPerSample;
    wavefmt.cbSize          = 0;

    DSBUFFERDESC dsbufd = { 0 };
    dsbufd.dwSize = sizeof(DSBUFFERDESC);
    dsbufd.dwFlags = DSBCAPS_GETCURRENTPOSITION2 | DSBCAPS_CTRLVOLUME | DSBCAPS_CTRLPAN | DSBCAPS_CTRLFREQUENCY;

    if ( SoundDriver_bUseGlobalFocusBuf || (*pFlags & SOUND_CHANNEL_GLOBALFOCUS) != 0 )
    {
        dsbufd.dwFlags |= DSBCAPS_GLOBALFOCUS;
        *pFlags |= SOUND_CHANNEL_GLOBALFOCUS;
    }

    if ( SoundDriver_bUse3DCaps && (*pFlags & SOUND_CHANNEL_USE3DCAPS) == 0 )
    {
        *pFlags |= SOUND_CHANNEL_3DSOUND;
        if ( numChannels == 2 )
        {
            *pFlags |= SOUND_CHANNEL_UNKNOWN_8000000;
        }
        else
        {
            dsbufd.dwFlags |= DSBCAPS_CTRL3D;
        }
    }

    dsbufd.dwBufferBytes = dataSize;
    dsbufd.lpwfxFormat = (LPWAVEFORMATEX)&wavefmt;

    HRESULT dsres = SoundDriver_CreateSoundBuffer(SoundDriver_pDirectSound, &dsbufd, &pDSBufferNew);
    dsres = SoundDriver_DSCheckStatus(dsres, __LINE__);
    if ( dsres == DS_OK )
    {
        DSBCAPS dsbcaps;
        dsbcaps.dwSize = sizeof(DSBCAPS);
        dsres = IDirectSoundBuffer8_GetCaps(pDSBufferNew, (LPDSBCAPS)&dsbcaps);
        if ( SoundDriver_DSCheckStatus(dsres, __LINE__) != DS_OK )
        {
            if ( pDSBufferNew )
            {
                IDirectSoundBuffer8_Release(pDSBufferNew);
            }

            return NULL;
        }

        if ( SoundDriver_bUse3DCaps )
        {
            if ( (dsbcaps.dwFlags & DSBCAPS_LOCHARDWARE) == 0 )
            {
                if ( pDSBufferNew )
                {
                    IDirectSoundBuffer8_Release(pDSBufferNew);
                }

                return NULL;
            }

            *pFlags |= SOUND_CHANNEL_HWMIXING;
        }
        else
        {
            *pFlags &= ~SOUND_CHANNEL_HWMIXING;
        }
    }

    if ( dsres == DS_OK )
    {
        if ( (*pFlags & SOUND_CHANNEL_RESTART) != 0 )
        {
            dsres = IDirectSoundBuffer8_SetVolume(pDSBufferNew, (LONG)(SoundDriver_maxVolume * (float)DSBVOLUME_MIN)); // silence
            SoundDriver_DSCheckStatus(dsres, __LINE__);
        }
        else
        {
            dsres = IDirectSoundBuffer8_SetVolume(pDSBufferNew, (LONG)(SoundDriver_maxVolume * (float)DS3D_DEFAULTCONEOUTSIDEVOLUME)); // max i.e. no attenuation
            SoundDriver_DSCheckStatus(dsres, __LINE__);
        }

        if ( (*pFlags & SOUND_CHANNEL_COMPRESSED) != 0 )
        {
            //DWORD dataSite = ((tAudioCompressedData*)pSoundData)->uncompressedSize;//*(DWORD*)pSoundData;

            // Note, using dataSize as it should be already set to uncompressed size here
            DWORD dwAudioBytes1, dwAudioBytes2;
            void* lpvAudioPtr1;
            void* lpvAudioPtr2;
            dsres = IDirectSoundBuffer8_Lock(pDSBufferNew, 0, dataSize, &lpvAudioPtr1, &dwAudioBytes1, &lpvAudioPtr2, &dwAudioBytes2, 0);// last arg is flags DSBLOCK_FROMWRITECURSOR or DSBLOCK_ENTIREBUFFER 
            dsres = SoundDriver_DSCheckStatus(dsres, __LINE__);
            if ( dsres == DSERR_BUFFERLOST )
            {
                dsres = IDirectSoundBuffer8_Restore(pDSBufferNew);
                dsres = SoundDriver_DSCheckStatus(dsres, __LINE__);
                if ( dsres == DS_OK )
                {
                    dsres = IDirectSoundBuffer8_Lock(pDSBufferNew, 0, dataSize, &lpvAudioPtr1, &dwAudioBytes1, &lpvAudioPtr2, &dwAudioBytes2, 0);
                    dsres = SoundDriver_DSCheckStatus(dsres, __LINE__);
                }
            }

            if ( dsres == DS_OK )
            {
                tAudioCompressorState compressor;
                AudioLib_ResetCompressor(&compressor);

                if ( lpvAudioPtr2 )
                {
                    uint8_t* pUncompressedData = (uint8_t*)malloc(dataSize);
                    if ( pUncompressedData ) // Fixed: Added check for null ptr
                    {
                        AudioLib_Uncompress(&compressor, pUncompressedData, ((tAudioCompressedData*)pSoundData)->compressedData, dataSize);
                        memcpy(lpvAudioPtr1, pUncompressedData, dwAudioBytes1);
                        memcpy(lpvAudioPtr2, &pUncompressedData[dwAudioBytes1], dwAudioBytes2);
                        free(pUncompressedData);
                    }
                    else
                    {
                        SoundDriver_LogError("SoundDriver: SoundDriver_CreateAndPlay Failed to allocate memory to uncompress data.");
                    }
                }
                else
                {
                    AudioLib_Uncompress(&compressor, (uint8_t*)lpvAudioPtr1, ((tAudioCompressedData*)pSoundData)->compressedData, dwAudioBytes1);
                }

                dsres = IDirectSoundBuffer8_Unlock(pDSBufferNew, lpvAudioPtr1, dwAudioBytes1, lpvAudioPtr2, dwAudioBytes2);
                dsres = SoundDriver_DSCheckStatus(dsres, __LINE__);
            }
        }
        else
        {
            DWORD dwAudioBytes1, dwAudioBytes2;
            void* lpvAudioPtr1;
            void* lpvAudioPtr2;
            dsres = IDirectSoundBuffer8_Lock(pDSBufferNew, 0, dataSize, &lpvAudioPtr1, &dwAudioBytes1, &lpvAudioPtr2, &dwAudioBytes2, 0);
            dsres = SoundDriver_DSCheckStatus(dsres, __LINE__);
            if ( dsres == DSERR_BUFFERLOST )
            {
                dsres = IDirectSoundBuffer8_Restore(pDSBufferNew);
                dsres = SoundDriver_DSCheckStatus(dsres, __LINE__);
                if ( dsres == DS_OK )
                {
                    dsres = IDirectSoundBuffer8_Lock(pDSBufferNew, 0, dataSize, &lpvAudioPtr1, &dwAudioBytes1, &lpvAudioPtr2, &dwAudioBytes2, 0);
                    dsres = SoundDriver_DSCheckStatus(dsres, __LINE__);
                }
            }

            if ( dsres == DS_OK )
            {
                memcpy(lpvAudioPtr1, pSoundData, dwAudioBytes1);
                if ( lpvAudioPtr2 )
                {
                    memcpy(lpvAudioPtr2, &pSoundData[dwAudioBytes1], dwAudioBytes2);
                }
            }

            dsres = IDirectSoundBuffer8_Unlock(pDSBufferNew, lpvAudioPtr1, dwAudioBytes1, lpvAudioPtr2, dwAudioBytes2);
            dsres = SoundDriver_DSCheckStatus(dsres, __LINE__);
        }

    play:
        if ( (*pFlags & SOUND_CHANNEL_PLAYING) == 0 )
        {
            return pDSBufferNew;
        }

        dsres = IDirectSoundBuffer8_Play(pDSBufferNew, 0, 0, (*pFlags & (SOUND_CHANNEL_LOOP | SOUND_CHANNEL_PLAYING)) == (SOUND_CHANNEL_LOOP | SOUND_CHANNEL_PLAYING) ? DSBPLAY_LOOPING : 0);
        dsres = SoundDriver_DSCheckStatus(dsres, __LINE__);
        if ( dsres != DSERR_BUFFERLOST )
        {
            return pDSBufferNew;
        }

        dsres = IDirectSoundBuffer8_Restore(pDSBufferNew);
        dsres = SoundDriver_DSCheckStatus(dsres, __LINE__);
        if ( dsres != DS_OK )
        {
            return pDSBufferNew;
        }

        SoundDriver_RestoreSoundBufferData(pDSBufferNew);
        dsres = IDirectSoundBuffer8_Play(pDSBufferNew, 0, 0, (*pFlags & (SOUND_CHANNEL_LOOP | SOUND_CHANNEL_PLAYING)) == (SOUND_CHANNEL_LOOP | SOUND_CHANNEL_PLAYING) ? DSBPLAY_LOOPING : 0);
        SoundDriver_DSCheckStatus(dsres, __LINE__);
        return pDSBufferNew;
    }

    if ( pDSBufferNew )
    {
        IDirectSoundBuffer8_Release(pDSBufferNew);
    }

    return NULL;
}

void J3DAPI SoundDriver_Release(tSysSoundBuffer* pDSBuf)
{
    if ( pDSBuf )
    {
        IDirectSoundBuffer8_Release(pDSBuf);
    }
}

void J3DAPI SoundDriver_Play(tSysSoundBuffer* pDSBuf, int bLoop)
{
    if ( pDSBuf )
    {
        HRESULT dsres = IDirectSoundBuffer8_Play(pDSBuf, 0, 0, bLoop ? DSBPLAY_LOOPING : 0);
        if ( SoundDriver_DSCheckStatus(dsres, __LINE__) == DSERR_BUFFERLOST )
        {
            dsres = IDirectSoundBuffer8_Restore(pDSBuf);
            if ( SoundDriver_DSCheckStatus(dsres, __LINE__) == DS_OK )
            {
                SoundDriver_RestoreSoundBufferData(pDSBuf);
                dsres = IDirectSoundBuffer8_Play(pDSBuf, 0, 0, bLoop ? DSBPLAY_LOOPING : 0);
                SoundDriver_DSCheckStatus(dsres, __LINE__);
            }
        }
    }
}

void J3DAPI SoundDriver_Stop(tSysSoundBuffer* pDSBuf)
{
    if ( pDSBuf )
    {
        HRESULT dsres = IDirectSoundBuffer8_Stop(pDSBuf);
        SoundDriver_DSCheckStatus(dsres, __LINE__);
    }
}

void J3DAPI SoundDriver_SetVolume(tSysSoundBuffer* pDSBuf, float volume)
{
    if ( pDSBuf )
    {
        int dbVol = SoundDriver_GetDecibelVolume(volume);
        HRESULT dsres = IDirectSoundBuffer8_SetVolume(pDSBuf, dbVol);
        SoundDriver_DSCheckStatus(dsres, __LINE__);
    }
}

float J3DAPI SoundDriver_GetVolume(tSysSoundBuffer* pDSBuf)
{
    if ( !pDSBuf )
    {
        return 1.0f;
    }

    LONG dbVol;
    HRESULT dsres = IDirectSoundBuffer8_GetVolume(pDSBuf, &dbVol);
    if ( SoundDriver_DSCheckStatus(dsres, __LINE__) != DS_OK )
    {
        return 1.0f;
    }

    return SoundDriver_GetVolumeFromDecibels(dbVol);
}

void J3DAPI SoundDriver_SetPan(tSysSoundBuffer* pDSoundBuf, float pan)
{
    if ( pDSoundBuf )
    {
        int lPan = SoundDriver_GetIntPan(pan);
        HRESULT dsres = IDirectSoundBuffer8_SetPan(pDSoundBuf, lPan);
        SoundDriver_DSCheckStatus(dsres, __LINE__);
    }
}

float J3DAPI SoundDriver_GetPan(tSysSoundBuffer* pDSBuf)
{
    if ( !pDSBuf )
    {
        return 0.0f;
    }

    LONG lPan;
    HRESULT dsres = IDirectSoundBuffer8_GetPan(pDSBuf, &lPan);
    if ( SoundDriver_DSCheckStatus(dsres, __LINE__) != DS_OK )
    {
        return 0.0f;
    }

    return SoundDriver_GetPanFromTable(lPan);
}

void J3DAPI SoundDriver_SetFrequency(tSysSoundBuffer* pDSBuf, float freq)
{
    if ( pDSBuf )
    {
        HRESULT dsres = IDirectSoundBuffer8_SetFrequency(pDSBuf, (int32_t)freq);
        SoundDriver_DSCheckStatus(dsres, __LINE__);
    }
}

float J3DAPI SoundDriver_GetFrequency(tSysSoundBuffer* pDSBuf)
{
    if ( !pDSBuf )
    {
        return 22050.0f;
    }

    DWORD freq;
    HRESULT dsres = IDirectSoundBuffer8_GetFrequency(pDSBuf, &freq);
    if ( SoundDriver_DSCheckStatus(dsres, __LINE__) != DS_OK )
    {
        return 22050.0f;
    }

    return (float)(int)freq;
}

size_t J3DAPI SoundDriver_GetCurrentPosition(tSysSoundBuffer* pDSoundBuf)
{
    if ( !pDSoundBuf )
    {
        return 0;
    }

    DWORD dwCurPlayPosition;
    HRESULT dsres = IDirectSoundBuffer8_GetCurrentPosition(pDSoundBuf, &dwCurPlayPosition, NULL);
    if ( SoundDriver_DSCheckStatus(dsres, __LINE__) != DS_OK )
    {
        return 0;
    }

    return dwCurPlayPosition;
}

tSoundChannelFlag J3DAPI SoundDriver_GetStatusAndCaps(tSysSoundBuffer* pDSBuffer)
{
    if ( !pDSBuffer )
    {
        return (tSoundChannelFlag)0;
    }

    DWORD dwStatus;
    HRESULT dsres = IDirectSoundBuffer8_GetStatus(pDSBuffer, &dwStatus);
    if ( SoundDriver_DSCheckStatus(dsres, __LINE__) != DS_OK )
    {
        return 0;
    }

    tSoundChannelFlag flags = 0;
    if ( (dwStatus & DSBSTATUS_PLAYING) != 0 )
    {
        flags |= SOUND_CHANNEL_PLAYING;
    }

    if ( (dwStatus & DSBSTATUS_LOOPING) != 0 )
    {
        flags |= SOUND_CHANNEL_LOOP | SOUND_CHANNEL_PLAYING;
    }

    DSBCAPS dsbcaps = { 0 }; // Added: Init to 0
    dsbcaps.dwSize = sizeof(DSBCAPS);
    dsres = IDirectSoundBuffer8_GetCaps(pDSBuffer, &dsbcaps);
    if ( SoundDriver_DSCheckStatus(dsres, __LINE__) != DS_OK )
    {
        return (tSoundChannelFlag)0;
    }

    if ( (dsbcaps.dwFlags & DSBCAPS_CTRL3D) != 0 )
    {
        flags |= SOUND_CHANNEL_3DSOUND;
    }

    if ( (dsbcaps.dwFlags & DSBCAPS_LOCSOFTWARE) != 0 )
    {
        flags |= SOUND_CHANNEL_SWMIXING;
    }

    if ( (dsbcaps.dwFlags & DSBCAPS_LOCHARDWARE) != 0 )
    {
        flags |= SOUND_CHANNEL_HWMIXING;
    }

    if ( (dsbcaps.dwFlags & DSBCAPS_GLOBALFOCUS) != 0 )
    {
        flags |= SOUND_CHANNEL_GLOBALFOCUS;
    }

    return flags;
}

void J3DAPI SoundDriver_SetListenerPosition(const rdVector3* pPos, const rdVector3* pVelocity, const rdVector3* pTopOrient, const rdVector3* pFrontOrient)
{
    if ( pPos )
    {
        rdVector_Copy3(&SoundDriver_curListenerPos, pPos);
    }

    if ( !SoundDriver_pfCalcListenerSoundMix && pTopOrient && pFrontOrient )
    {
        if ( pFrontOrient->x == 0.0f )
        {
            SoundDriver_frontOrientATan = 0.0f;
        }
        else
        {
            SoundDriver_frontOrientATan = atan2f(pFrontOrient->z, pFrontOrient->x);
            if ( errno == EDOM )
            {
                SoundDriver_frontOrientATan = 0.0f;
            }
        }
    }

    if ( SoundDriver_bUse3DCaps && SoundDriver_pDS3DListener )
    {
        if ( pPos )
        {
            HRESULT dsres = IDirectSound3DListener_SetPosition(SoundDriver_pDS3DListener, pPos->x, pPos->y, pPos->z, DS3D_DEFERRED);
            SoundDriver_DSCheckStatus(dsres, __LINE__);
        }

        if ( pVelocity )
        {
            HRESULT dsres = IDirectSound3DListener_SetVelocity(SoundDriver_pDS3DListener, pVelocity->x, pVelocity->y, pVelocity->z, DS3D_DEFERRED);
            SoundDriver_DSCheckStatus(dsres, __LINE__);
        }

        if ( pFrontOrient )
        {
            if ( pTopOrient )
            {
                HRESULT dsres = IDirectSound3DListener_SetOrientation(SoundDriver_pDS3DListener, pFrontOrient->x, pFrontOrient->y, pFrontOrient->z, pTopOrient->x, pTopOrient->y, pTopOrient->z, DS3D_DEFERRED);
                SoundDriver_DSCheckStatus(dsres, __LINE__);
            }
        }
    }
}

void SoundDriver_ListenerCommitDeferred(void)
{
    if ( SoundDriver_bUse3DCaps )
    {
        if ( SoundDriver_pDS3DListener )
        {
            HRESULT dress = IDirectSound3DListener_CommitDeferredSettings(SoundDriver_pDS3DListener);
            SoundDriver_DSCheckStatus(dress, __LINE__);
        }
    }
}

void J3DAPI SoundDriver_SetPosAndVelocity(tSysSoundBuffer* pBuffer, float x, float y, float z, float velX, float velY, float velZ, float minDistance, float maxDistance)
{
    if ( pBuffer )
    {
        if ( SoundDriver_bUse3DCaps )
        {
            LPDIRECTSOUND3DBUFFER8 pDS3DBuffer;
            HRESULT dsres = IDirectSoundBuffer8_QueryInterface(pBuffer, &IID_IDirectSound3DBuffer8, &pDS3DBuffer);
            if ( SoundDriver_DSCheckStatus(dsres, __LINE__) == DS_OK )
            {
                dsres = IDirectSound3DBuffer_SetPosition(pDS3DBuffer, x, y, z, DS3D_DEFERRED);
                SoundDriver_DSCheckStatus(dsres, __LINE__);

                dsres = IDirectSound3DBuffer_SetVelocity(pDS3DBuffer, velX, velY, velZ, DS3D_DEFERRED);
                SoundDriver_DSCheckStatus(dsres, __LINE__);

                if ( minDistance < 0.0f )
                {
                    minDistance = SoundDriver_minDistance;
                }

                if ( maxDistance < 0.0f )
                {
                    maxDistance = SoundDriver_maxDistance;
                }

                dsres = IDirectSound3DBuffer_SetMinDistance(pDS3DBuffer, minDistance, DS3D_DEFERRED);
                SoundDriver_DSCheckStatus(dsres, __LINE__);

                dsres = IDirectSound3DBuffer_SetMaxDistance(pDS3DBuffer, maxDistance, DS3D_DEFERRED);
                SoundDriver_DSCheckStatus(dsres, __LINE__);

                IDirectSound3DBuffer_Release(pDS3DBuffer);
            }
        }
    }
}

int J3DAPI SoundDriver_Update3DSound(tSysSoundBuffer** ppDSBuf, float x, float y, float z, float volume, float pitch, tSoundChannelFlag* pChannelFlags, float minRadius, float maxRadius, SoundEnvFlags envflags)
{
    tSysSoundBuffer* pDSBuf = NULL;
    if ( ppDSBuf )
    {
        pDSBuf = *ppDSBuf;
    }

    bool b3DControl = false;
    if ( (*pChannelFlags & SOUND_CHANNEL_USE3DCAPS) != 0 )
    {
        b3DControl = true;
    }
    else if ( SoundDriver_Use3DCaps() && (*pChannelFlags & SOUND_CHANNEL_UNKNOWN_8000000) == 0 )
    {
        float distSquare = (SoundDriver_curListenerPos.x - x) * (SoundDriver_curListenerPos.x - x)
            + (SoundDriver_curListenerPos.y - y) * (SoundDriver_curListenerPos.y - y)
            + (SoundDriver_curListenerPos.z - z) * (SoundDriver_curListenerPos.z - z);
        b3DControl = distSquare <= 0.25f;
    }

    if ( (*pChannelFlags & SOUND_CHANNEL_DISABLEHWMIXING) != 0 )
    {
        b3DControl = false;
    }

    float newVolume = volume;
    float pan       = 0.0f;
    float newPitch  = pitch;
    if ( !b3DControl )
    {
        SoundDriver_CalcListeneSoundMix(x, y, z, volume, pitch, &newVolume, &pan, &newPitch, minRadius, maxRadius, envflags);
        if ( newVolume < SOUNDDRIVER_FARVOLUMETHRESHOLD
            && (*pChannelFlags & (SOUND_CHANNEL_LOOP | SOUND_CHANNEL_PLAYING)) != (SOUND_CHANNEL_LOOP | SOUND_CHANNEL_PLAYING)
        #ifdef J3D_QOL_IMPROVEMENTS
            && (*pChannelFlags & (SOUND_CHANNEL_FAR)) != (SOUND_CHANNEL_FAR)  // Fixed: Added check for channel being played far away. 
                                                                              //        This prevents removing a looping sound in the Sound_Update at far distance that is out of hearing range.
        #endif
            )
        {
            return 0;
        }
    }

    if ( SoundDriver_Use3DCaps() )
    {
        if ( (((*pChannelFlags & SOUND_CHANNEL_UNKNOWN_8000000) == 0) & *pChannelFlags) == 0 )
        {
            if ( pDSBuf )
            {
                LPDIRECTSOUND3DBUFFER8 pDS3DBuffer;
                HRESULT dsres = IDirectSoundBuffer8_QueryInterface(pDSBuf, &IID_IDirectSound3DBuffer8, &pDS3DBuffer);
                if ( SoundDriver_DSCheckStatus(dsres, __LINE__) == DS_OK )
                {
                    IDirectSound3DBuffer_SetMode(pDS3DBuffer, b3DControl ? DS3DMODE_DISABLE : DS3DMODE_NORMAL, DS3D_IMMEDIATE);
                    IDirectSound3DBuffer_Release(pDS3DBuffer);
                }
            }
        }
    }

    // Note: No need to check for null pDSBuf because all underlying functions do that
    if ( (*pChannelFlags & SOUND_CHANNEL_HWMIXING) == 0 || (*pChannelFlags & SOUND_CHANNEL_DISABLEHWMIXING) != 0 )
    {
        float newVol = newVolume * SoundDriver_maxVolume;
        SoundDriver_SetVolume(pDSBuf, newVol);
        SoundDriver_SetPan(pDSBuf, pan);
        if ( newPitch != 1.0f )
        {
            float freq = SoundDriver_GetFrequency(pDSBuf);
            freq *= newPitch / pitch;
            SoundDriver_SetFrequency(pDSBuf, freq);
        }
    }

    if ( (*pChannelFlags & SOUND_CHANNEL_FAR) != 0 )
    {
        if ( newVolume > SOUNDDRIVER_FARVOLUMETHRESHOLD )
        {
            *pChannelFlags |= SOUND_CHANNEL_RESTART;
            return 1;
        }
    }

#ifdef J3D_QOL_IMPROVEMENTS
    // Fixed: Always stop currently looping far sound when volume drops below SOUNDDRIVER_FARVOLUMETHESHOLD.
    //        Originally, far marked looping sound was not stopped due to else if branch here.
    if ( (*pChannelFlags & (SOUND_CHANNEL_LOOP | SOUND_CHANNEL_PLAYING)) == (SOUND_CHANNEL_LOOP | SOUND_CHANNEL_PLAYING)
        && (*pChannelFlags & SOUND_CHANNEL_3DSOUND) == SOUND_CHANNEL_3DSOUND
        && newVolume < SOUNDDRIVER_FARVOLUMETHRESHOLD )
    #else

    else if ( (*pChannelFlags & (SOUND_CHANNEL_LOOP | SOUND_CHANNEL_PLAYING)) == (SOUND_CHANNEL_LOOP | SOUND_CHANNEL_PLAYING)
        && (*pChannelFlags & SOUND_CHANNEL_3DSOUND) == SOUND_CHANNEL_3DSOUND
        && newVolume < SOUNDDRIVER_FARVOLUMETHRESHOLD )
    #endif
    {
        SoundDriver_Stop(pDSBuf);
        if ( pDSBuf )
        {
            IDirectSoundBuffer8_Release(pDSBuf);
            *ppDSBuf = NULL;
        }

        *pChannelFlags &= ~(SOUND_CHANNEL_LOOP | SOUND_CHANNEL_PLAYING);
        *pChannelFlags |= SOUND_CHANNEL_FAR;
    }

    return 1;
}

void J3DAPI SoundDriver_SetGlobals(float distanceFactor, float minDistance, float maxDistance, float rolloffFactor, float dopplerFactor)
{
    SoundDriver_defaultDistanceFactor = distanceFactor / 10.0f;

    SoundDriver_minDistance = minDistance;
    SoundDriver_maxDistance = maxDistance;

    SoundDriver_defaultRolloffFactor = rolloffFactor;
    SoundDriver_defaultDopplerFactor = dopplerFactor;

    if ( SoundDriver_bUse3DCaps )
    {
        if ( SoundDriver_pDS3DListener )
        {
            HRESULT dsres = IDirectSound3DListener_SetDistanceFactor(SoundDriver_pDS3DListener, SoundDriver_defaultDistanceFactor, DS3D_IMMEDIATE);
            SoundDriver_DSCheckStatus(dsres, __LINE__);

            dsres = IDirectSound3DListener_SetRolloffFactor(SoundDriver_pDS3DListener, SoundDriver_defaultRolloffFactor, DS3D_IMMEDIATE);
            SoundDriver_DSCheckStatus(dsres, __LINE__);

            dsres = IDirectSound3DListener_SetDopplerFactor(SoundDriver_pDS3DListener, SoundDriver_defaultDopplerFactor, DS3D_IMMEDIATE);
            SoundDriver_DSCheckStatus(dsres, __LINE__);
        }
    }
}

int SoundDriver_Has3DHW(void)
{
    return SoundDriver_bHas3DHW;
}

int SoundDriver_Use3DCaps(void)
{
    return SoundDriver_bUse3DCaps;
}

tDirectSound* SoundDriver_GetDSound(void)
{
    return SoundDriver_pDirectSound;
}

tDirectSound* J3DAPI SoundDriver_CreateDirectSound(int bNoSound3D)
{
    J3D_UNUSED(bNoSound3D);

    tDirectSound* pDSound = NULL;
    if ( DirectSoundCreate8(NULL, &pDSound, NULL) != DS_OK )
    {
        SoundDriver_LogError("DirectSound: Original DS fails\n");
        return NULL;
    }

    HRESULT dsres = IDirectSound8_SetCooperativeLevel(pDSound, SoundDriver_hwnd, DSSCL_PRIORITY);
    if ( SoundDriver_DSCheckStatus(dsres, __LINE__) != DS_OK )
    {
        if ( pDSound )
        {
            IDirectSound8_Release(pDSound);
        }

        SoundDriver_LogError("DirectSound: Set co-operative level fails!\n");
        SoundDriver_bUse3DCaps = 0;
        return NULL;
    }

    DSCAPS dscaps = { 0 };
    dscaps.dwSize = sizeof(DSCAPS);
    dsres = IDirectSound8_GetCaps(pDSound, &dscaps);
    SoundDriver_DSCheckStatus(dsres, __LINE__);

    SoundDriver_bUse3DCaps = 0;

    if ( !dscaps.dwMaxHw3DAllBuffers )
    {
        return pDSound;
    }

    if ( !SoundDriver_bNoDSound3D )
    {
        SoundDriver_bUse3DCaps = 1;
    }

    SoundDriver_bHas3DHW = 1;
    return pDSound;
}

void J3DAPI SoundDriver_ReleaseDirectSound(tDirectSound* pDSound)
{
    SoundDriver_ReleasePrimaryDSBuffer();
    if ( pDSound )
    {
        IDirectSound8_Release(pDSound);
    }
}
void SoundDriver_CreateListener(void)
{
    if ( SoundDriver_bUse3DCaps )
    {
        tSysSoundBuffer* pBuffer = SoundDriver_GetPrimaryDSBuffer(SoundDriver_pDirectSound);
        HRESULT dsres = IDirectSoundBuffer8_QueryInterface(pBuffer, &IID_IDirectSound3DListener8, &SoundDriver_pDS3DListener);
        if ( SoundDriver_DSCheckStatus(dsres, __LINE__) != DS_OK )
        {
            SoundDriver_pDS3DListener = NULL;
            return;
        }

        dsres = IDirectSound3DListener_SetDistanceFactor(SoundDriver_pDS3DListener, SoundDriver_defaultDistanceFactor, DS3D_IMMEDIATE);
        SoundDriver_DSCheckStatus(dsres, __LINE__);

        dsres = IDirectSound3DListener_SetRolloffFactor(SoundDriver_pDS3DListener, SoundDriver_defaultRolloffFactor, DS3D_IMMEDIATE);
        SoundDriver_DSCheckStatus(dsres, __LINE__);

        dsres = IDirectSound3DListener_SetDopplerFactor(SoundDriver_pDS3DListener, SoundDriver_defaultDopplerFactor, DS3D_IMMEDIATE);
        SoundDriver_DSCheckStatus(dsres, __LINE__);
    }
}

void SoundDriver_ReleaseListener(void)
{
    if ( SoundDriver_pDS3DListener )
    {
        IDirectSound3DListener_Release(SoundDriver_pDS3DListener);
        SoundDriver_pDS3DListener = NULL;
    }
}

void J3DAPI SoundDriver_SetOuputFormat(tDirectSound* pDSound, uint32_t nSamplesPerSec, uint32_t nBitsPerSample, uint32_t numChannels)
{
    tSysSoundBuffer* pBuffer = SoundDriver_GetPrimaryDSBuffer(pDSound);
    if ( !pBuffer )
    {
        SoundDriver_LogError("Sound.DLL: SetOuputFormat: Grabbing primary buffer fails!\n");
        return;
    }

    WAVEFORMATEX fmt = { 0 }; // Added: Init to 0
    fmt.wFormatTag      = WAVE_FORMAT_PCM;
    fmt.nChannels       = numChannels;
    fmt.nSamplesPerSec  = nSamplesPerSec;
    fmt.nAvgBytesPerSec = numChannels * (nBitsPerSample / 8) * nSamplesPerSec;
    fmt.nBlockAlign     = numChannels * (nBitsPerSample / 8);
    fmt.wBitsPerSample  = nBitsPerSample;
    fmt.cbSize          = 0;

    HRESULT dsres = IDirectSoundBuffer8_SetFormat(pBuffer, &fmt);
    SoundDriver_DSCheckStatus(dsres, __LINE__);
    SoundDriver_ReleasePrimaryDSBuffer();
}

tSysSoundBuffer* J3DAPI SoundDriver_GetPrimaryDSBuffer(tDirectSound* pDSound)
{
    if ( !pDSound )
    {
        return NULL;
    }

    if ( SoundDriver_pDSBuffer )
    {
        return SoundDriver_pDSBuffer;
    }

    DSBUFFERDESC dsbd = { 0 }; // Added: Init to 0
    dsbd.dwSize = sizeof(DSBUFFERDESC);
    dsbd.dwFlags = DSBCAPS_CTRLVOLUME | DSBCAPS_PRIMARYBUFFER;

    if ( SoundDriver_bUse3DCaps )
    {
        dsbd.dwFlags |= DSBCAPS_CTRL3D;
    }

    if ( SoundDriver_bUseGlobalFocusBuf )
    {
        dsbd.dwFlags |= DSBCAPS_GLOBALFOCUS;
    }

    HRESULT hr = SoundDriver_CreateSoundBuffer(pDSound, &dsbd, &SoundDriver_pDSBuffer);
    if ( SoundDriver_DSCheckStatus(hr, __LINE__) != DS_OK )
    {
        if ( SoundDriver_pDSBuffer )
        {
            SoundDriver_pDSBuffer = NULL;
        }
    }

    return SoundDriver_pDSBuffer;
}

void SoundDriver_ReleasePrimaryDSBuffer(void)
{
    if ( SoundDriver_pDSBuffer )
    {
        IDirectSoundBuffer8_Release(SoundDriver_pDSBuffer);
        SoundDriver_pDSBuffer = NULL;
    }
}

int J3DAPI SoundDriver_GetDecibelVolume(float volume)
{
    int idx = (int)(volume * 255.0f);
    if ( idx < 0 )
    {
        idx = 0;
    }

    if ( idx < STD_ARRAYLEN(SoundDriver_aDBTable) )
    {
        return SoundDriver_aDBTable[idx];
    }

    return 0;
}

int J3DAPI SoundDriver_GetIntPan(float pan)
{
    int idx = (int)(pan * 128.0f + 128.0f);
    if ( idx < 0 )
    {
        idx = 0;
    }

    if ( idx < STD_ARRAYLEN(SoundDriver_aPanTable) )
    {
        return SoundDriver_aPanTable[idx];
    }

    return 0;
}

float J3DAPI SoundDriver_GetVolumeFromDecibels(int dB)
{
    int v1;
    int idx = STD_ARRAYLEN(SoundDriver_aDBTable);
    do
    {
        v1 = idx--;
    } while ( v1 && SoundDriver_aDBTable[idx] > dB );

    return (float)idx * 1.0f / 255.0f;
}

float J3DAPI SoundDriver_GetPanFromTable(int pan)
{
    int v1;
    int idx = STD_ARRAYLEN(SoundDriver_aPanTable);
    do
    {
        v1 = idx--;
    } while ( v1 && SoundDriver_aPanTable[idx] > pan );

    return (float)(idx - 128) * 1.0f / 128.0f;
}

void SoundDriver_InitPanTable(void)
{
    int i;

    for ( i = 0; i < (STD_ARRAYLEN(SoundDriver_aPanTable) / 2); ++i )
    {
        SoundDriver_aPanTable[i + 128] = -SoundDriver_aDBTable[255 - 2 * i];
        SoundDriver_aPanTable[127 - i] = SoundDriver_aDBTable[255 - 2 * i];
    }
}

HRESULT J3DAPI SoundDriver_DSCheckStatus(HRESULT code, size_t codeLine)
{
    if ( code == DS_OK )
    {
        return 0;
    }

    const DXStatus* pEntry;
    for ( pEntry = SoundDriver_aDStatusTbl; pEntry->code != 0 && pEntry->code != code; ++pEntry )
    {
        ;
    }

    // TODO: maybe log unknown error
    if ( pEntry->code == 0 )
    {
        return code;
    }

    SoundDriver_LogError("Sound.DLL Driver.C:%d Return value: %s\n", codeLine, pEntry->text);
    return code;
}

void J3DAPI SoundDriver_CalcListeneSoundMix(float x, float y, float z, float volume, float pitch, float* newVolume, float* pan, float* newPitch, float minRadius, float maxRadius, SoundEnvFlags envflags)
{
    if ( SoundDriver_pfCalcListenerSoundMix )
    {
        SoundSpatialInfo spatialInfo = { 0 };
        spatialInfo.pos.x     = x;
        spatialInfo.pos.y     = y;
        spatialInfo.pos.z     = z;
        spatialInfo.volume    = volume;
        spatialInfo.pitch     = pitch;
        spatialInfo.minRadius = minRadius;
        spatialInfo.maxRadius = maxRadius;
        spatialInfo.flags     = envflags;

        if ( SoundDriver_curListenerPos.x < 100000.0f )
        {
            SoundDriver_pfCalcListenerSoundMix(&spatialInfo, newVolume, pan, newPitch);
        }
        else
        {
            if ( newVolume )
            {
                *newVolume = 0.0f;
            }

            if ( pan )
            {
                *pan = 0.0f;
            }
        }
    }
    else
    {
        if ( !newVolume )
        {
            return;
        }

        *newVolume = volume;
        if ( pan )
        {
            *pan = 0.0f;
        }

        if ( newPitch )
        {
            *newPitch = 1.0f;
        }

        double dist = SoundDriver_CalcDistToListener(x, y, z, SoundDriver_curListenerPos.x, SoundDriver_curListenerPos.y, SoundDriver_curListenerPos.z);
        if ( dist > 500.0 )
        {
            *newVolume = 0.0f;
            return;
        }

        if ( dist >= 200.0 )
        {
            *newVolume = 0.0f;
        }
        else
        {
            if ( dist < 30.0 )
            {
                *newVolume = 1.0f;
                return;
            }

            if ( dist >= 200.0 ) // this makes no sense
            {
                dist = 200.0f;
            }

            *newVolume = (float)((*newVolume - 0.0) * (170.0 - (dist - 30.0)) / 170.0 + 0.0); // 0.0f looks like to be some constant vars
        }

        if ( pan )
        {
            float dx = x - SoundDriver_curListenerPos.x;
            float dz = z - SoundDriver_curListenerPos.z;
            if ( dx <= -0.5f || dx >= 0.5f )
            {
                if ( dz <= -0.5f || dz >= 0.5f )
                {
                    float atan = atan2f(dz, dx);
                    if ( errno == EDOM )
                    {
                        atan = 0.0f;
                    }

                    *pan = -sinf(atan - SoundDriver_frontOrientATan);
                }
                else if ( dx <= 0.0f )
                {
                    *pan = -1.0f;
                }
                else
                {
                    *pan = 1.0f;
                }
            }
        }
    }
}

double J3DAPI SoundDriver_CalcDistToListener(float x1, float y1, float z1, float x2, float y2, float z2)
{
    float dx  = fabsf(x2 - x1);
    float dz  = fabsf(z2 - z1);
    float hdx = (dx > (double)dz ? dz : dx) / 2.0f;
    float dy  = fabsf(y2 - y1);
    return dx + dz - hdx + dy;
}

void J3DAPI SoundDriver_RestoreSoundBufferData(tSysSoundBuffer* pDSBuf)
{
    if ( SoundDriver_pfGetSoundBufferData )
    {
        uint32_t dataSize = 0;  // Added: Init to 0
        unsigned int bCompressed = 0; // Added: Init to 0
        uint8_t* pSoundData = SoundDriver_pfGetSoundBufferData(pDSBuf, &dataSize, &bCompressed);
        if ( bCompressed )
        {
            DWORD uncompressedSize = ((tAudioCompressedData*)pSoundData)->uncompressedSize;

            DWORD dwAudioBytes1, dwAudioBytes2;
            void* lpvAudioPtr1 = NULL;
            void* lpvAudioPtr2 = NULL;

            HRESULT dsres = IDirectSoundBuffer8_Lock(pDSBuf, 0, uncompressedSize, &lpvAudioPtr1, &dwAudioBytes1, &lpvAudioPtr2, &dwAudioBytes2, 0); // last param is flags, can be DSBLOCK_FROMWRITECURSOR or DSBLOCK_ENTIREBUFFER 
            dsres = SoundDriver_DSCheckStatus(dsres, __LINE__);

            if ( dsres == DSERR_BUFFERLOST )
            {
                dsres = IDirectSoundBuffer8_Restore(pDSBuf);
                dsres = SoundDriver_DSCheckStatus(dsres, __LINE__);
                if ( dsres == DS_OK )
                {
                    dsres = IDirectSoundBuffer8_Lock(pDSBuf, 0, uncompressedSize, &lpvAudioPtr1, &dwAudioBytes1, &lpvAudioPtr2, &dwAudioBytes2, 0);
                    dsres = SoundDriver_DSCheckStatus(dsres, __LINE__);
                }
            }

            if ( dsres == DS_OK )
            {
                tAudioCompressorState compresor;
                AudioLib_ResetCompressor(&compresor);

                if ( lpvAudioPtr2 )
                {
                    uint8_t* pUncompressedData = (uint8_t*)malloc(uncompressedSize);
                    if ( pUncompressedData ) // Fixed: Added check for null ptr 
                    {
                        AudioLib_Uncompress(&compresor, pUncompressedData, ((tAudioCompressedData*)pSoundData)->compressedData, uncompressedSize);
                        memcpy(lpvAudioPtr1, pUncompressedData, dwAudioBytes1);
                        memcpy(lpvAudioPtr2, &pUncompressedData[dwAudioBytes1], dwAudioBytes2);
                        free(pUncompressedData);
                    }
                    else
                    {
                        SoundDriver_LogError("SoundDriver: SoundDriver_RestoreSoundBufferData Failed to allocate memory to uncompress data.");
                    }
                }
                else
                {
                    AudioLib_Uncompress(&compresor, (uint8_t*)lpvAudioPtr1, ((tAudioCompressedData*)pSoundData)->compressedData, dwAudioBytes1);
                }

                dsres = IDirectSoundBuffer8_Unlock(pDSBuf, lpvAudioPtr1, dwAudioBytes1, lpvAudioPtr2, dwAudioBytes2);
                dsres = SoundDriver_DSCheckStatus(dsres, __LINE__);
            }
        }
        else
        {
            DWORD dwAudioBytes1, dwAudioBytes2;
            void* lpvAudioPtr1 = NULL;
            void* lpvAudioPtr2 = NULL;

            HRESULT dsres = IDirectSoundBuffer8_Lock(pDSBuf, 0, dataSize, &lpvAudioPtr1, &dwAudioBytes1, &lpvAudioPtr2, &dwAudioBytes2, 0);
            dsres = SoundDriver_DSCheckStatus(dsres, __LINE__);
            if ( dsres == DSERR_BUFFERLOST )
            {
                dsres = IDirectSoundBuffer8_Restore(pDSBuf);
                dsres = SoundDriver_DSCheckStatus(dsres, __LINE__);
                if ( dsres == DS_OK )
                {
                    dsres = IDirectSoundBuffer8_Lock(pDSBuf, 0, dataSize, &lpvAudioPtr1, &dwAudioBytes1, &lpvAudioPtr2, &dwAudioBytes2, 0);
                    dsres = SoundDriver_DSCheckStatus(dsres, __LINE__);
                }
            }

            if ( dsres == DS_OK )
            {
                memcpy(lpvAudioPtr1, pSoundData, dwAudioBytes1);
                if ( lpvAudioPtr2 )
                {
                    memcpy(lpvAudioPtr2, &pSoundData[dwAudioBytes1], dwAudioBytes2);
                }
            }

            dsres = IDirectSoundBuffer8_Unlock(pDSBuf, lpvAudioPtr1, dwAudioBytes1, lpvAudioPtr2, dwAudioBytes2);
            SoundDriver_DSCheckStatus(dsres, __LINE__);
        }
    }
}

uint32_t SoundDriver_GetTimeMsec(void)
{
    return stdPlatform_GetTimeMsec();
}

void J3DAPI SoundDriver_Sleep(unsigned int dwMilliseconds)
{
    Sleep(dwMilliseconds);
}

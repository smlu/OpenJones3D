// Dsnd.c: the DirectSound driver (`SNDDRIVER=insane_dsnd`) (original 0x004E74B0-0x004E7D60).
#include <smush/insane/Sound/Dsnd.h>
#include <j3dcore/j3d.h>
#include <j3dcore/j3dhook.h>
#include <smush/RTI/symbols.h>

#include <stdarg.h>
#include <stddef.h>
#include <string.h>

#include <Windows.h>
#include <mmsystem.h>
#include <Mmreg.h>
#if defined (J3D_DIRECTX6)
#include <DirectX6/dsound.h>
#elif defined (J3D_DIRECTX9)
#include <dsound.h>
#else
#error "Unsuported system sound API. Please define J3D_DIRECTX6 or J3D_DIRECTX9 in your project."
#endif

#include <smush/insane/Sound/Engine.h>
#include <smush/insane/Sound/Mixer.h>
#include <smush/insane/Timer/Timer.h>
#include <smush/SmushPlay.h>

static LecModule dsnd_module = { dsnd_Register, dsnd_Unregister, dsnd_Startup, dsnd_Shutdown };
static SndDriver dsnd_interface = { dsnd_Start, dsnd_Stop, dsnd_SetVolume, dsnd_GetVolume, (int (*)(int))sound_Null, dsnd_GetRate };

static int dsnd_lastBlock = -1;
static int dsnd_rate = 22050;
static int dsnd_volume = 127;
static int dsnd_savedVolume = -5000;
static WAVEFORMATEX dsnd_primaryFormat = { WAVE_FORMAT_PCM, 2, 22050, 88200, 4, 16, 0 };
static WAVEFORMATEX dsnd_streamFormat  = { WAVE_FORMAT_PCM, 2, 22050, 88200, 4, 16, 0 };

static void* dsnd_pMixerTable = NULL;
static int dsnd_cfg3 = 0;
static int dsnd_volTable[128];
static void (*dsnd_pfnFill)(int, SoundFeed) = NULL;
static int dsnd_cfg1 = 0;
static int dsnd_writePos = 0;
static LecShare* dsnd_pShare = NULL;
static int dsnd_numBlocks = 0;
static TimerInterface* dsnd_pTimer = NULL;
static int dsnd_playPos = 0;
static int dsnd_cfg4 = 0;
static void* dsnd_pParam = NULL;
static char dsnd_scratch[DSND_BLOCKSIZE]; // One block of 0x600 stereo 16-bit frames, the size the callback mixes into it
static int dsnd_cfg2 = 0;
static LPDIRECTSOUND dsnd_pDS = NULL;
static LPDIRECTSOUNDBUFFER dsnd_pPrimary = NULL;
static LPDIRECTSOUNDBUFFER dsnd_pSecondary = NULL;
static int dsnd_busy = 0;
static int dsnd_ownDS = 0;
static int dsnd_hTimer = 0;
static int dsnd_tablesReady = 0;

int J3DAPI dsnd_Register(LecShare* pShare, void* pParam)
{
    dsnd_pShare = pShare;
    dsnd_pParam = pParam;
    pShare->Publish("LECMODULE=insane_dsnd", &dsnd_module);
    dsnd_pShare->Publish("SNDDRIVER=insane_dsnd", &dsnd_interface);
    dsnd_cfg1 = -1;
    dsnd_cfg3 = -1;
    dsnd_cfg2 = -1;
    dsnd_cfg4 = -1;
    return 0;
}

int J3DAPI dsnd_Configure(int tag, ...)
{
    va_list args;
    va_start(args, tag);
    while ( tag )
    {
        switch ( tag )
        {
            case 4:
                dsnd_cfg4 = va_arg(args, int);
                break;

            case 2:
                dsnd_cfg2 = va_arg(args, int);
                break;

            case 3:
                dsnd_cfg3 = va_arg(args, int);
                break;

            case 1:
                dsnd_cfg1 = va_arg(args, int);
                break;

            case 5:
                dsnd_pDS = va_arg(args, LPDIRECTSOUND); // The DirectSound object of the game (IDirectSound8 in DX9 builds, the vtables share the used methods)
                dsnd_ownDS = 0;
                break;

            default:
                va_end(args);
                return -1;
        }

        tag = va_arg(args, int);
    }

    va_end(args);
    return 0;
}

void dsnd_Unregister(void)
{
    dsnd_pShare->Unpublish("SNDDRIVER=insane_dsnd");
    dsnd_pShare->Unpublish("LECMODULE=insane_dsnd");
}

int dsnd_Startup(void)
{
    dsnd_pTimer = dsnd_pShare->Subscribe("TIMER=clock", dsnd_pParam);
    if ( dsnd_pTimer == NULL )
    {
        dsnd_pShare->ReportError("DirectSoundDriver Module: Cannot allocate timer services.");
        return -1;
    }

    dsnd_pMixerTable = dsnd_pShare->Subscribe("MEMORY.mixertable", dsnd_pParam);
    if ( dsnd_pMixerTable == NULL )
    {
        dsnd_pShare->ReportError("DirectSoundDriver Module: Cannot allocate timer services.");
        return -1;
    }

    return 0;
}

void dsnd_Shutdown(void)
{
    dsnd_pShare->Unsubscribe(dsnd_pTimer);
    dsnd_pShare->Unsubscribe(dsnd_pMixerTable);
}

int J3DAPI dsnd_Start(int rate, int a2, void (*pfnFill)(int, SoundFeed))
{
    J3D_UNUSED(a2);
    if ( rate < 22050 ) {
        rate = 22050;
    }

    if ( dsnd_Init() ) {
        return -1;
    }

    if ( mixer_Init(dsnd_pMixerTable, 4, 0x400, 0, 1, 1) ) {
        return -1;
    }

    dsnd_pfnFill = pfnFill;
    dsnd_rate = rate;
    dsnd_hTimer = dsnd_pTimer->Alloc(dsnd_Callback, 1193180 / (rate / 1024), 2, 0);
    return 0;
}

void dsnd_Stop(void)
{
    if ( dsnd_hTimer ) {
        dsnd_pTimer->Free(dsnd_hTimer);
    }

    dsnd_hTimer = 0;
    mixer_Done(dsnd_pMixerTable);
    dsnd_Release();
}

void J3DAPI dsnd_SetVolume(int volume)
{
    if ( !dsnd_tablesReady )
    {
        dsnd_volTable[0] = -10000;
        dsnd_volTable[1] = -7000;
        dsnd_volTable[2] = -6000;
        dsnd_volTable[3] = -5200;
        dsnd_volTable[4] = -5000;
        dsnd_volTable[5] = -4500;
        dsnd_volTable[6] = -4200;
        dsnd_volTable[7] = -4100;
        dsnd_volTable[8] = -4000;

        int* pEntry = &dsnd_volTable[8];
        for ( int v = 0; v < 48000; v += 2000 ) {
            *pEntry++ = v / 23 - 4000;
        }

        pEntry = &dsnd_volTable[32];
        for ( int v = 0; v < 32000; v += 1000 ) {
            *pEntry++ = v / 31 - 2000;
        }

        pEntry = &dsnd_volTable[64];
        for ( int v = 0; v < 64000; v += 1000 ) {
            *pEntry++ = v / 63 - 1000;
        }

        dsnd_tablesReady = 1;
    }

    dsnd_volume = volume & 0x7F;
    IDirectSoundBuffer_SetVolume(dsnd_pSecondary, dsnd_volTable[dsnd_volume]);
}

int dsnd_GetVolume(void)
{
    return dsnd_volume;
}

int dsnd_GetRate(void)
{
    return 22050;
}

void J3DAPI dsnd_Callback(int index, int param)
{
    J3D_UNUSED(index);
    J3D_UNUSED(param);

    int cur = dsnd_GetPlayPos();
    if ( cur != -1 )
    {
        int w = dsnd_lastBlock;
        int n = dsnd_numBlocks;
        if ( cur != (w + 1) % n )
        {
            if ( w < 0 ) {
                w = cur;
            }

            dsnd_lastBlock = (w + 1) % n;

            LPVOID pPtr1, pPtr2;
            DWORD len1, len2;
            if ( IDirectSoundBuffer_Lock(dsnd_pSecondary, dsnd_lastBlock * DSND_BLOCKSIZE, DSND_BLOCKSIZE, &pPtr1, &len1, &pPtr2, &len2, 0) == 0 )
            {
                int chunk;
                char* pDst;
                if ( pPtr2 && len2 )
                {
                    chunk = 0x600;
                    pDst = dsnd_scratch;
                }
                else
                {
                    chunk = (int)len1 / 4;
                    pDst = pPtr1;
                }

                mixer_Begin(chunk);
                dsnd_pfnFill(chunk, mixer_Feed);
                mixer_Mix((int16_t*)pDst, chunk);
                if ( pPtr2 && len2 )
                {
                    memcpy(pPtr1, dsnd_scratch, len1);
                    memcpy(pPtr2, dsnd_scratch + len1, len2);
                }

                IDirectSoundBuffer_Unlock(dsnd_pSecondary, pPtr1, len1, pPtr2, len2);
            }
        }
    }

    dsnd_pTimer->SetInterval(dsnd_hTimer, 1193180 / (dsnd_rate / 1024));
}

int dsnd_Init(void)
{
    if ( dsnd_pDS != NULL )
    {
        dsnd_OpenSecondary(3);
        return 0;
    }

    HRESULT hr = DirectSoundCreate(NULL, &dsnd_pDS, NULL);
    if ( hr )
    {
        dsnd_pDS = NULL;
        dsnd_Print("Cannot create DirectSound object (%d)", hr);
        return -1;
    }

    hr = IDirectSound_SetCooperativeLevel(dsnd_pDS, (HWND)smushplay_GetHwnd(), DSSCL_EXCLUSIVE);
    if ( hr )
    {
        dsnd_Print("DirectSound: Error %08X while setting cooperative level", hr);
        return -1;
    }

    DSBUFFERDESC desc;
    memset(&desc, 0, sizeof(desc));
    desc.dwSize = sizeof(desc);
    desc.dwFlags = DSBCAPS_PRIMARYBUFFER | DSBCAPS_CTRLVOLUME;
    desc.dwBufferBytes = 0;
    desc.dwReserved = 0;
    desc.lpwfxFormat = NULL;
    hr = IDirectSound_CreateSoundBuffer(dsnd_pDS, &desc, &dsnd_pPrimary, NULL);
    if ( hr )
    {
        dsnd_Print("DirectSound: Error %08X while creating primary buffer", hr);
        return -1;
    }

    if ( IDirectSoundBuffer_SetFormat(dsnd_pPrimary, &dsnd_primaryFormat) ) {
        return -1;
    }

    if ( IDirectSoundBuffer_GetVolume(dsnd_pPrimary, (LPLONG)&dsnd_savedVolume) ) {
        return -1;
    }

    if ( IDirectSoundBuffer_SetVolume(dsnd_pPrimary, 0) ) {
        return -1;
    }

    dsnd_OpenSecondary(3);
    dsnd_ownDS = 1;
    return 0;
}

// The driver's error print: an empty body (linker-folded onto 0x0046D9E0 in the original); never hook.
void J3DAPI dsnd_Print(const char* pFormat, ...)
{
    J3D_UNUSED(pFormat);
}

void J3DAPI dsnd_OpenSecondary(int blocks)
{
    dsnd_busy = 1;
    if ( dsnd_pSecondary ) {
        IDirectSoundBuffer_Release(dsnd_pSecondary);
    }

    dsnd_pSecondary = NULL;
    if ( blocks < 3 ) {
        blocks = 3;
    }
    else if ( blocks > 16 ) {
        blocks = 16;
    }

    dsnd_numBlocks = blocks;

    DSBUFFERDESC desc;
    memset(&desc, 0, sizeof(desc));
    desc.dwSize = sizeof(desc);
    desc.dwBufferBytes = blocks * DSND_BLOCKSIZE;
    desc.dwFlags = DSBCAPS_GETCURRENTPOSITION2 | DSBCAPS_CTRLVOLUME;
    desc.dwReserved = 0;
    desc.lpwfxFormat = &dsnd_streamFormat;
    if ( IDirectSound_CreateSoundBuffer(dsnd_pDS, &desc, &dsnd_pSecondary, NULL) == 0 )
    {
        LPVOID pPtr1, pPtr2;
        DWORD len1, len2;

        IDirectSoundBuffer_SetVolume(dsnd_pSecondary, 0);
        if ( IDirectSoundBuffer_Lock(dsnd_pSecondary, 0, dsnd_numBlocks * DSND_BLOCKSIZE, &pPtr1, &len1, &pPtr2, &len2, 0) == 0 )
        {
            memset(pPtr1, 0, len1);
            if ( pPtr2 ) {
                memset(pPtr2, 0, len2);
            }

            IDirectSoundBuffer_Unlock(dsnd_pSecondary, pPtr1, len1, pPtr2, len2);
        }

        IDirectSoundBuffer_Play(dsnd_pSecondary, 0, 0, DSBPLAY_LOOPING);
    }

    dsnd_busy = 0;
}

void dsnd_Release(void)
{
    if ( dsnd_pSecondary ) {
        IDirectSoundBuffer_Release(dsnd_pSecondary);
    }

    dsnd_pSecondary = NULL;
    if ( dsnd_pPrimary && dsnd_ownDS )
    {
        IDirectSoundBuffer_SetVolume(dsnd_pPrimary, dsnd_savedVolume);
        IDirectSoundBuffer_Release(dsnd_pPrimary);
        dsnd_pPrimary = NULL;
        dsnd_ownDS = 0;
    }

    // TODO: [BUG] dsnd_ownDS was cleared above, so the DirectSound object the driver created itself is never released.
    if ( dsnd_pDS && dsnd_ownDS ) {
        IDirectSound_Release(dsnd_pDS);
    }

    dsnd_pDS = NULL;
}

int dsnd_GetPlayPos(void)
{
    if ( dsnd_pSecondary )
    {
        if ( IDirectSoundBuffer_GetCurrentPosition(dsnd_pSecondary, (LPDWORD)&dsnd_playPos, (LPDWORD)&dsnd_writePos) == 0 ) {
            return (dsnd_playPos / DSND_BLOCKSIZE) % dsnd_numBlocks;
        }
    }

    return -1;
}

#if defined(J3D_TEST)
// Added: Test-only reset of the driver's state.
void Dsnd_TestReset(void);
void Dsnd_TestReset(void)
{
    dsnd_lastBlock = -1;
    dsnd_rate = 22050;
    dsnd_volume = 127;
    dsnd_pDS = NULL;
    dsnd_pPrimary = NULL;
    dsnd_pSecondary = NULL;
    dsnd_busy = 0;
    dsnd_ownDS = 0;
    dsnd_hTimer = 0;
    dsnd_numBlocks = 0;
}
#endif

void Dsnd_InstallHooks(void)
{
    J3D_HOOKFUNC(dsnd_Register);
    J3D_HOOKFUNC(dsnd_Configure);
    J3D_HOOKFUNC(dsnd_Unregister);
    J3D_HOOKFUNC(dsnd_Startup);
    J3D_HOOKFUNC(dsnd_Shutdown);
    J3D_HOOKFUNC(dsnd_Start);
    J3D_HOOKFUNC(dsnd_Stop);
    J3D_HOOKFUNC(dsnd_SetVolume);
    J3D_HOOKFUNC(dsnd_GetVolume);
    J3D_HOOKFUNC(dsnd_GetRate);
    J3D_HOOKFUNC(dsnd_Callback);
    J3D_HOOKFUNC(dsnd_Init);
    J3D_HOOKFUNC(dsnd_OpenSecondary);
    J3D_HOOKFUNC(dsnd_Release);
    J3D_HOOKFUNC(dsnd_GetPlayPos);
}

void Dsnd_ResetGlobals(void)
{}

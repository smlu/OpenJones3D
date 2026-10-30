// SmushPlay.c: the glue between the game and the INSANE movie-player library (original 0x004E30B0-0x004E33F0).
#include "SmushPlay.h"
#include <j3dcore/j3d.h>
#include <j3dcore/j3dhook.h>
#include <smush/RTI/symbols.h>
#include <string.h>

#include <smush/insane/animplay.h>

#include <std/General/stdUtil.h>

#include <smush/insane/Animate/Frame.h>
#include <smush/insane/Animate/Pcplay.h>
#include <smush/insane/Bitmap/Bitmap.h>
#include <smush/insane/Bitmap/Blit.h>
#include <smush/insane/Bitmap/BlitAsm.h>
#include <smush/insane/Bitmap/MemCopy.h>
#include <smush/insane/Bitmap/MemFill.h>
#include <smush/insane/Decode/Decode.h>
#include <smush/insane/Decode/DecodeAsm.h>
#include <smush/insane/File/Cdstream.h>
#include <smush/insane/File/Framestream.h>
#include <smush/insane/File/Stdfile.h>
#include <smush/insane/File/Stdio.h>
#include <smush/insane/File/Zfilio/Zfilio.h>
#include <smush/insane/Flobject/Bloc16Asm.h>
#include <smush/insane/Flobject/D_bloc16.h>
#include <smush/insane/Flobject/D_wave.h>
#include <smush/insane/Image/Readauto.h>
#include <smush/insane/Lec/LecShare.h>
#include <smush/insane/Memory/Memory.h>
#include <smush/insane/Sound/Dsnd.h>
#include <smush/insane/Sound/Engine.h>
#include <smush/insane/Sound/Mixer.h>
#include <smush/insane/Sound/MixerAsm.h>
#include <smush/insane/Timer/Timer.h>
#include <smush/insane/Util/List.h>

#define SMUSHPLAY_MAX_PATH     260
#define SMUSHPLAY_DEFAULT_RATE 15   // Frames per second used if the caller passes 0
#define SMUSHPLAY_MAX_VOLUME   127
#define SMUSHPLAY_PLAYFLAGS    (ANIMPLAY_FLAG_SYNC | ANIMPLAY_FLAG_NOCLEAR | ANIMPLAY_FLAG_FIXEDRATE) // Flags always set for the game's movies

static void* smushplay_pMemory;
static SoundInterface* smushplay_pSound;
static void* smushplay_pFile;
static void* smushplay_pAnimFrame;
static LecShare* smushplay_pShare;
static AnimPlayTable* smushplay_pAnimPlay;
static BitmapInterface* smushplay_pBitmap;
static void* smushplay_pTimer;

static LecBatch smushplay_aServices[] = {
    { &smushplay_pMemory, "MEMORY.app", NULL },
    { &smushplay_pTimer, "TIMER.app", NULL },
    { &smushplay_pFile, "FILE.app", NULL },
    { (void**)&smushplay_pBitmap, "BITMAP.app", NULL },
    { (void**)&smushplay_pSound, "SOUND.app", NULL },
    { &smushplay_pAnimFrame, "ANIMFRAME.app", NULL },
    { (void**)&smushplay_pAnimPlay, "ANIMPLAY.app", NULL },
    { NULL, NULL, NULL }
};

static LecModuleEntry smushplay_aModules[] = {
    { memory_Register, NULL },
    { timer_Register, NULL },
    { bitmap_Register, NULL },
    { stdio_Register, NULL },
    { zfilio_Register, "FILIO=insane_stdio" },
    { stdfile_Register, NULL },
    { dbloc16_Register, NULL },
    { dwave_Register, NULL },
    { dsnd_Register, NULL },
    { sound_Register, NULL },
    { streamer_Register, NULL },
    { animframe_Register, NULL },
    { animplay_Register, NULL },
    { readauto_Register, NULL },
    { NULL, NULL }
};

static int smushplay_volume = SMUSHPLAY_MAX_VOLUME;

static HWND smushplay_hWnd;
static AnimPlay* smushplay_pPlay;
static int smushplay_bAbort;
static int(__cdecl* smushplay_pfBlit)(const SmushBitmap* pBitmap, int a2);

int J3DAPI SmushPlay_SysStartup(HWND hwnd, tDirectSound* pDSound)
{
    smushplay_hWnd = hwnd;
    smushplay_pShare = LecShare_Init(lec_Log, smushplay_aModules, "READANIM=insane_auto FILIO=insane_zfilio", "");
    if ( smushplay_pShare )
    {
        zfilio_Configure(1, ".snm", 0);
        zfilio_Configure(1, ".znm", 0);
        dsnd_Configure(5, pDSound);
        if ( LecShare_Startup() == 0 )
        {
            if ( ((int (*)(LecBatch*, const char*))smushplay_pShare->pfn10)(smushplay_aServices, NULL) == 0 )
            {
                return 1;
            }

            LecShare_Shutdown();
        }

        LecShare_UnregisterAll();
    }

    return 0;
}

int J3DAPI SmushPlay_PlayMovie(const char* pFilename, int rate, int flags, int a4, int a5, int width, int height, int a8, int a9, int(__cdecl* pBlit)(const SmushBitmap* pBitmap, int a2), int loops, int ringSize, int frameSize)
{
    J3D_UNUSED(a4);
    J3D_UNUSED(a5);

    char aPath[SMUSHPLAY_MAX_PATH];
    uint8_t aPalette[ANIMPLAY_PALETTE_SIZE]; // Persists between frames

    // Fixed: The original copies the whole file name into the 260 byte buffer without a bound. Truncate the name to the buffer
    STD_STRCPY(aPath, pFilename);

    if ( rate == 0 )
    {
        rate = SMUSHPLAY_DEFAULT_RATE;
    }

    LecBitmapHeader* pBitmap;
    if ( smushplay_pBitmap->Create(&pBitmap, width, height, LECBITMAP_TYPE_RGB565) )
    {
        return 0;
    }

    // A zero loop count becomes -1, for which the loop below does not run
    if ( loops == 0 )
    {
        loops = -1;
    }

    smushplay_pSound->SetVolume(SOUND_NOTRACK, smushplay_volume);
    smushplay_pfBlit = pBlit;
    smushplay_bAbort = 0;
    dbloc16_Configure(1, smushplay_Blit);

    // TODO: [BUG] The original returns an uninitialized rc if the loop doesn't run (loops is -1 if 0 is passed in), here it is 0.
    int rc = 0; // Fixed: Init to 0
    for ( int i = 0; i < loops; ++i )
    {
        rc = smushplay_pAnimPlay->Create(&smushplay_pPlay, aPath, rate, ringSize, frameSize, flags | SMUSHPLAY_PLAYFLAGS);
        if ( rc == 0 )
        {
            int r;
            do
            {
                int outA = 0;
                int outB = 0;
                r = smushplay_pAnimPlay->Pace(smushplay_pPlay, pBitmap, 0, 0, 0, aPalette, &outA, &outB, a8, a9);
                if ( smushplay_bAbort )
                {
                    i = loops;
                    break;
                }
            } while ( r >= 0 && r != ANIMPLAY_STATE_END );

            smushplay_pAnimPlay->Destroy(&smushplay_pPlay);
        }
    }

    smushplay_pPlay = NULL;
    smushplay_pBitmap->Destroy(&pBitmap);
    return rc == 0;
}

int J3DAPI smushplay_Blit(int a1, int a2, int a3, int a4, void* pBitmap)
{
    J3D_UNUSED(a1);
    J3D_UNUSED(a2);
    J3D_UNUSED(a3);
    J3D_UNUSED(a4);

    if ( smushplay_pfBlit )
    {
        smushplay_bAbort = smushplay_pfBlit(pBitmap, smushplay_pPlay->frame);
    }

    return 0;
}

void SmushPlay_SysShutdown(void)
{
    LecShare_Shutdown();
    LecShare_UnregisterAll();
    ((int (*)(LecBatch*))smushplay_pShare->pfn14)(smushplay_aServices);
}

void J3DAPI SmushPlay_SetGlobalVolume(size_t volume)
{
    if ( (int)volume >= 0 && (int)volume <= SMUSHPLAY_MAX_VOLUME ) // Original compares signed
    {
        smushplay_volume = (int)volume;
    }
}

void* smushplay_GetHwnd(void)
{
    return smushplay_hWnd;
}

void SmushPlay_InstallHooks(void)
{
    // Uncomment only lines for functions that have full definition and doesn't call original function (non-thunk functions)

    J3D_HOOKFUNC(SmushPlay_SysStartup);
    J3D_HOOKFUNC(SmushPlay_PlayMovie);
    J3D_HOOKFUNC(smushplay_Blit);
    J3D_HOOKFUNC(SmushPlay_SysShutdown);
    J3D_HOOKFUNC(SmushPlay_SetGlobalVolume);
    J3D_HOOKFUNC(smushplay_GetHwnd);
}

void SmushPlay_ResetGlobals(void)
{
    smushplay_pMemory    = NULL;
    smushplay_pSound     = NULL;
    smushplay_pFile      = NULL;
    smushplay_pAnimFrame = NULL;
    smushplay_pShare     = NULL;
    smushplay_pAnimPlay  = NULL;
    smushplay_pBitmap    = NULL;
    smushplay_pTimer     = NULL;
    smushplay_volume     = SMUSHPLAY_MAX_VOLUME;
    smushplay_hWnd       = NULL;
    smushplay_pPlay      = NULL;
    smushplay_bAbort     = 0;
    smushplay_pfBlit     = NULL;
}

void Smush_InstallHooks(void)
{
    SmushPlay_InstallHooks();
    Frame_InstallHooks();
    Pcplay_InstallHooks();
    Bitmap_InstallHooks();
    Blit_InstallHooks();
    BlitAsm_InstallHooks();
    MemCopy_InstallHooks();
    MemFill_InstallHooks();
    Decode_InstallHooks();
    DecodeAsm_InstallHooks();
    Cdstream_InstallHooks();
    Framestream_InstallHooks();
    Stdfile_InstallHooks();
    Stdio_InstallHooks();
    Zfilio_InstallHooks();
    Bloc16Asm_InstallHooks();
    D_bloc16_InstallHooks();
    D_wave_InstallHooks();
    Readauto_InstallHooks();
    LecShare_InstallHooks();
    Memory_InstallHooks();
    Dsnd_InstallHooks();
    Engine_InstallHooks();
    Mixer_InstallHooks();
    MixerAsm_InstallHooks();
    Timer_InstallHooks();
    List_InstallHooks();
}

void Smush_ResetGlobals(void)
{
    SmushPlay_ResetGlobals();
    Frame_ResetGlobals();
    Pcplay_ResetGlobals();
    Bitmap_ResetGlobals();
    Blit_ResetGlobals();
    BlitAsm_ResetGlobals();
    MemCopy_ResetGlobals();
    MemFill_ResetGlobals();
    Decode_ResetGlobals();
    DecodeAsm_ResetGlobals();
    Cdstream_ResetGlobals();
    Framestream_ResetGlobals();
    Stdfile_ResetGlobals();
    Stdio_ResetGlobals();
    Zfilio_ResetGlobals();
    Bloc16Asm_ResetGlobals();
    D_bloc16_ResetGlobals();
    D_wave_ResetGlobals();
    Readauto_ResetGlobals();
    LecShare_ResetGlobals();
    Memory_ResetGlobals();
    Dsnd_ResetGlobals();
    Engine_ResetGlobals();
    Mixer_ResetGlobals();
    MixerAsm_ResetGlobals();
    Timer_ResetGlobals();
    List_ResetGlobals();

    // zfilio_pFilio is the only INSANE datum that lives in the executable (read by its zlib)
#ifndef J3D_LOCAL_RTI
    memset((char*)&zfilio_pFilio, 0, sizeof(zfilio_pFilio));
#endif
}

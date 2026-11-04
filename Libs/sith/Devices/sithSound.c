#include "sithSound.h"

#include <sith/Devices/sithSoundMixer.h>
#include <sith/World/sithSoundClass.h>
#include <sith/World/sithWorld.h>

#include <sound/Sound.h>

#include <std/General/stdConffile.h>
#include <std/General/stdMemory.h>
#include <std/General/stdUtil.h>
#include <std/Win95/stdWin95.h>

#include <stdlib.h>

static bool bStartup = false; // Added: Init to false
static bool bSkipRestoringSounds;

void sithSound_ResetGlobals(void)
{}

int J3DAPI sithSound_Initialize(tHostServices* pHS)
{
    return Sound_Initialize(pHS);
}

void sithSound_Uninitialize(void)
{
    Sound_Uninitialize();
}

int sithSound_StartupSound(void)
{
    return Sound_Startup();
}

void sithSound_ShutdownSound(void)
{
    Sound_Shutdown();
}

int J3DAPI sithSound_Startup(int bSound3D)
{
    SITH_ASSERTREL(!bStartup);

    SoundOpenFlags flags = 0;
    if ( !bSound3D )
    {
        flags = SOUNDOPEN_NO3DSOUND;
    }

    const tSysWindow hwnd = stdWin95_GetWindow();
    Sound_Open(flags, SITHSOUND_MAXCHANNELS, sithSoundMixer_GetThingInfo, sithSoundMixer_CalcCameraRelativeSoundMix, hwnd);

    Sound_Set3DGlobals(10.0f, 0.5f, 2.5f, 1.0f, 1.0f);
    Sound_EnableLoad(1);

    if ( sithSoundMixer_Startup() || sithSoundClass_Startup() )
    {
        SITHLOG_ERROR("stdSound_Startup failed (no hardware?).\n");
        sithSound_Shutdown();
        return 1;
    }

    bSkipRestoringSounds = false;
    bStartup = true;
    return 0;
}

void sithSound_Shutdown(void)
{
    sithSoundClass_Shutdown();
    sithSoundMixer_Shutdown();
    Sound_Close();

    bSkipRestoringSounds = false;
    bStartup = false;
}

int J3DAPI sithSound_WriteSoundsListText(const SithWorld* pWorld)
{
    if ( stdConffile_WriteLine("###### Sound information  ######\n")
        || stdConffile_WriteLine("SECTION: SOUNDS\n\n")
        || stdConffile_Printf("World sounds %d\n\n", pWorld->numSounds + 80) )
    {
        return 1;
    }

    for ( uint32_t idx = 0; idx < pWorld->numSounds; ++idx )
    {
        tSoundHandle hSnd = Sound_GetSoundHandle(idx);
        const char* pFilename = Sound_GetSoundFilename(hSnd);
        if ( pFilename && stdConffile_Printf("%s\n", pFilename) )
        {
            return 1;
        }
    }

    return stdConffile_WriteLine("end\n") || stdConffile_WriteLine("################################\n\n\n");
}

int J3DAPI sithSound_ReadSoundsListText(SithWorld* pWorld, int bSkip)
{
    SITH_ASSERTREL(pWorld);

    if ( bSkip )
    {
        return 1;
    }

    sithWorld_UpdateLoadProgress(0.0f);

    if ( !stdConffile_ReadArgs() || !streq(stdConffile_g_entry.aArgs[0].argValue, "world") || !streq(stdConffile_g_entry.aArgs[1].argValue, "sounds") )
    {
        sithSound_FreeWorldSounds(pWorld);
        return 1;
    }

    size_t sizeSounds = atoi(stdConffile_g_entry.aArgs[2].argValue);
    if ( !sizeSounds )
    {
        return 0;
    }

    pWorld->numSounds = 0; // TODO: should this one be set to 0 before parsing size?

    // Fixed: Refactored to load sounds directly instead of first reading filenames into a cache list.
    //        This also fixes a memory leak where the cache list wasn't freed when bSkipRestoringSounds = true.
    while ( stdConffile_ReadArgs() && !streq(stdConffile_g_entry.aArgs[0].argValue, "end") )
    {
        if ( !bSkipRestoringSounds )
        {
            sithSound_Load(pWorld, stdConffile_g_entry.aArgs[0].argValue);
        }
    }

    if ( bSkipRestoringSounds )
    {
        bSkipRestoringSounds = false;
    }
    else
    {
        SITHLOG_STATUS("%d sounds loaded.\n", pWorld->numSounds);
    }

    return 0;
}


int J3DAPI sithSound_WriteSoundsListBinary(tFileHandle fh, const SithWorld* pWorld)
{
    if ( (pWorld->state & SITH_WORLD_STATE_STATIC) != 0 )
    {
        if ( !Sound_ExportStaticBank(fh) )
        {
            return 1;
        }
    }
    else if ( !Sound_ExportNormalBank(fh) )
    {
        return 1;
    }

    return 0;
}

int J3DAPI sithSound_ReadSoundsListBinary(tFileHandle fh, SithWorld* pWorld)
{
    if ( bSkipRestoringSounds )
    {
        bSkipRestoringSounds = false;
        return Sound_SkipSoundFileSection(fh) == 0;
    }

    if ( (pWorld->state & SITH_WORLD_STATE_STATIC) != 0 )
    {
        if ( !Sound_ImportStaticBank(fh) )
        {
            return 1;
        }
    }
    else if ( !Sound_ImportNormalBank(fh) )
    {
        return 1;
    }

    return 0;
}

void J3DAPI sithSound_FreeWorldSounds(SithWorld* pWorld)
{
    SITH_ASSERTREL(pWorld != NULL);

    if ( bSkipRestoringSounds )
    {
        Sound_StopAllSounds();
    }
    else
    {
        Sound_Reset(pWorld->state & SITH_WORLD_STATE_STATIC);
        pWorld->numSounds = 0;
    }
}

tSoundHandle J3DAPI sithSound_Load(SithWorld* pWorld, const char* filename)
{
    SITH_ASSERTREL(filename != NULL);
    SITH_ASSERTREL(pWorld);

    if ( !bStartup )
    {
        return 0;
    }

    if ( !filename )
    {
        return 0;
    }

    if ( streq(filename, "none") )
    {
        return 0;
    }

    char aPath[128];
    STD_FORMAT(aPath, "%s%c%s", "sound", '\\', filename);

    uint32_t sndIdx = pWorld->numSounds;
    tSoundHandle hSnd;
    if ( (pWorld->state & SITH_WORLD_STATE_STATIC) != 0 )
    {
        sndIdx = SITHWORLD_STATICINDEX(sndIdx);
        hSnd = Sound_LoadStatic(aPath, &sndIdx);
    }
    else
    {
        hSnd = Sound_Load(aPath, &sndIdx);
    }

    if ( !hSnd )
    {
        SITHLOG_ERROR("Could not open sound file '%s'.\n", aPath);
        return 0;
    }

    sndIdx = SITHWORLD_FROM_STATICINDEX(sndIdx);
    if ( sndIdx == pWorld->numSounds )
    {
        ++pWorld->numSounds;
    }

    return hSnd;
}

void J3DAPI sithSound_SetSkipRestoringSounds(int bSkip)
{
    bSkipRestoringSounds = bSkip != 0;
}
#include "sithSoundClass.h"
#include <j3dcore/j3dhook.h>

#include <sith/AI/sithAIAwareness.h>
#include <sith/Devices/sithSound.h>
#include <sith/Devices/sithSoundMixer.h>
#include <sith/Gameplay/sithPlayer.h>
#include <sith/RTI/symbols.h>
#include <sith/World/sithVoice.h>

#include <sound/Sound.h>

#include <std/General/stdConffile.h>
#include <std/General/stdHashtbl.h>
#include <std/General/stdMemory.h>
#include <std/General/stdUtil.h>
#include <std/Win95/stdComm.h>

static tHashTable* sithSoundClass_pHashtblModes;
static tHashTable* sithSoundClass_pHashTable;

static size_t sithSoundClass_lastVoiceEntryNum = 1000u;

static const char* sithSoundClass_aSoundModeNames[SITHSOUNDCLASS_MAXMODES] =
{
    "--reserved--", // used so the modeNum can start at 1, required due to stdHashtbl_Find returns 0 when mode is not found
    "create",
    "activate",
    "startmove",
    "stopmove",
    "moving",
    "lwalkhard",
    "rwalkhard",
    "lrunhard",
    "rrunhard",
    "lwalkmetal",
    "rwalkmetal",
    "lrunmetal",
    "rrunmetal",
    "lwalkwater",
    "rwalkwater",
    "lrunwater",
    "rrunwater",
    "lwalkpuddle",
    "rwalkpuddle",
    "lrunpuddle",
    "rrunpuddle",
    "lwalkearth",
    "rwalkearth",
    "lrunearth",
    "rrunearth",
    "lwalksnow",
    "rwalksnow",
    "lrunsnow",
    "rrunsnow",
    "lwalkwood",
    "rwalkwood",
    "lrunwood",
    "rrunwood",
    "lwalkhardecho",
    "rwalkhardecho",
    "lrunhardecho",
    "rrunhardecho",
    "lwalkwoodecho",
    "rwalkwoodecho",
    "lrunwoodecho",
    "rrunwoodecho",
    "lwalkearthecho",
    "rwalkearthecho",
    "lrunearthecho",
    "rrunearthecho",
    "lwalkaet",
    "rwalkaet",
    "lrunaet",
    "rrunaet",
    "enterwater",
    "enterwaterslow",
    "exitwater",
    "exitwaterslow",
    "lswimsurface",
    "rswimsurface",
    "treadsurface",
    "lswimunder",
    "rswimunder",
    "treadunder",
    "jump",
    "jumpmetal",
    "jumpwater",
    "jumpearth",
    "jumpsnow",
    "jumpwood",
    "jumphardecho",
    "jumpwoodecho",
    "jumpearthecho",
    "jumpaet",
    "landhard",
    "landmetal",
    "landwater",
    "landpuddle",
    "landearth",
    "landsnow",
    "landwood",
    "landhardecho",
    "landwoodecho",
    "landearthecho",
    "landaet",
    "landhurt",
    "hithard",
    "hitmetal",
    "hitearth",
    "deflected",
    "scrapehard",
    "scrapemetal",
    "scrapeearth",
    "hitdamaged",
    "falling",
    "corpsehit",
    "hurtimpact",
    "hurtenergy",
    "hurtfire",
    "hurtfists",
    "hurtmachete",
    "drowning",
    "death1",
    "death2",
    "deathunder",
    "drowned",
    "splattered",
    "pant",
    "breath",
    "gasp",
    "fire1",
    "fire2",
    "fire3",
    "fire4",
    "curious",
    "alert",
    "idle",
    "gloat",
    "fear",
    "boast",
    "slide1",
    "victory",
    "help",
    "flee",
    "search",
    "calm",
    "surprise",
    "reserved1",
    "reserved2",
    "reserved3",
    "reserved4",
    "reserved5",
    "reserved6",
    "reserved7",
    "reserved8",
    "stand2walk",
    "walk2stand",
    "climbhandleft",
    "climbhandright",
    "climbhoriz",
    "climbonto",
    "climbdownonto",
    "lswimaet",
    "rswimaet",
    "treadaet"
};

void J3DAPI sithSoundClass_Free(SithSoundClass* pClass);
tSoundChannelHandle J3DAPI sithSoundClass_PlayModeEntry(SithThing* pThing, SithSoundClassMode mode, SithSoundClassEntry* pEntry, float volume);
SithSoundClass* J3DAPI sithSoundClass_CacheFind(const char* pName);
void J3DAPI sithSoundClass_CacheAdd(SithSoundClass* pClass);
void J3DAPI sithSoundClass_CacheRemove(const SithSoundClass* pClass);

void sithSoundClass_InstallHooks(void)
{
    J3D_HOOKFUNC(sithSoundClass_Startup);
    J3D_HOOKFUNC(sithSoundClass_Shutdown);
    J3D_HOOKFUNC(sithSoundClass_ReadSoundClassesListText);
    J3D_HOOKFUNC(sithSoundClass_WriteSoundClassesListBinary);
    J3D_HOOKFUNC(sithSoundClass_ReadSoundClassesListBinary);
    J3D_HOOKFUNC(sithSoundClass_Load);
    J3D_HOOKFUNC(sithSoundClass_LoadEntry);
    J3D_HOOKFUNC(sithSoundClass_AllocSoundClasses);
    J3D_HOOKFUNC(sithSoundClass_FreeWorldSoundClasses);
    J3D_HOOKFUNC(sithSoundClass_Free);
    J3D_HOOKFUNC(sithSoundClass_PlayModeFirstEx);
    J3D_HOOKFUNC(sithSoundClass_PlayModeFirst);
    J3D_HOOKFUNC(sithSoundClass_PlayModeRandom);
    J3D_HOOKFUNC(sithSoundClass_PlayVoiceModeRandom);
    J3D_HOOKFUNC(sithSoundClass_PlayPlayerVoiceModeRandom);
    J3D_HOOKFUNC(sithSoundClass_PlayMode);
    J3D_HOOKFUNC(sithSoundClass_StopMode);
    J3D_HOOKFUNC(sithSoundClass_SetModePitch);
    J3D_HOOKFUNC(sithSoundClass_SetModeVolume);
    J3D_HOOKFUNC(sithSoundClass_FadeModeVolume);
    J3D_HOOKFUNC(sithSoundClass_IsModeFadingVol);
    J3D_HOOKFUNC(sithSoundClass_PlayModeEntry);
    J3D_HOOKFUNC(sithSoundClass_CacheFind);
    J3D_HOOKFUNC(sithSoundClass_CacheAdd);
    J3D_HOOKFUNC(sithSoundClass_CacheRemove);
}

void sithSoundClass_ResetGlobals(void)
{}

int sithSoundClass_Startup(void)
{
    sithSoundClass_pHashtblModes = stdHashtbl_New(64u);
    sithSoundClass_pHashTable    = stdHashtbl_New(282u);
    if ( !sithSoundClass_pHashtblModes || !sithSoundClass_pHashTable )
    {
        SITHLOG_ERROR("Could not allocate hashtable for soundclasses.\n");
        return 1;
    }

    for ( size_t modeNum = 1; modeNum < STD_ARRAYLEN(sithSoundClass_aSoundModeNames); ++modeNum )
    {
        stdHashtbl_Add(sithSoundClass_pHashTable, sithSoundClass_aSoundModeNames[modeNum], (void*)modeNum);
    }

    return 0;
}

void sithSoundClass_Shutdown(void)
{
    if ( sithSoundClass_pHashtblModes )
    {
        stdHashtbl_Free(sithSoundClass_pHashtblModes);
    }

    sithSoundClass_pHashtblModes = NULL;

    if ( sithSoundClass_pHashTable )
    {
        stdHashtbl_Free(sithSoundClass_pHashTable);
    }

    sithSoundClass_pHashTable = NULL;
}

int J3DAPI sithSoundClass_WriteSoundClassesListText(const SithWorld* pWorld)
{
    if ( stdConffile_WriteLine("#### Sound (foley) Classes #####\n")
        || stdConffile_WriteLine("Section: Soundclass\n\n")
        || stdConffile_Printf("World soundclasses %d\n", pWorld->numSoundClasses) )
    {
        return 1;
    }

    for ( size_t i = 0; i < pWorld->numSoundClasses; ++i )
    {
        if ( stdConffile_Printf("%d:\t%s\n", i, pWorld->aSoundClasses[i].aName) )
        {
            return 1;
        }
    }

    return stdConffile_WriteLine("end\n") || stdConffile_WriteLine("################################\n\n\n");
}

int J3DAPI sithSoundClass_ReadSoundClassesListText(SithWorld* pWorld, int bSkip)
{
    if ( bSkip )
    {
        return 1;
    }

    SITH_ASSERTREL(pWorld && !pWorld->aSoundClasses);
    SITH_ASSERTREL(!pWorld->sizeSoundClasses && !pWorld->numSoundClasses);

    stdConffile_ReadArgs();
    if ( !streq(stdConffile_g_entry.aArgs[0].argValue, "world") || !streq(stdConffile_g_entry.aArgs[1].argValue, "soundclasses") )
    {
        SITHLOG_ERROR("Parse error reading soundclass list line %d.\n", stdConffile_GetLineNumber());
        return 1;
    }

    size_t numClasses = atoi(stdConffile_g_entry.aArgs[2].argValue);
    if ( numClasses <= 0 )
    {
        return 0;
    }

    if ( stdComm_IsGameActive() )
    {
        // Add additional buffer size
        numClasses += 32;
    }

    if ( sithSoundClass_AllocSoundClasses(pWorld, numClasses) )
    {
        SITHLOG_ERROR("Memory error while reading soundclasses, line %d.\n", stdConffile_GetLineNumber());
        return 1;
    }

    while ( stdConffile_ReadArgs() && !streq(stdConffile_g_entry.aArgs[0].argValue, "end") )
    {
        sithSoundClass_Load(pWorld, stdConffile_g_entry.aArgs[1].argValue);
    }

    SITHLOG_STATUS("Soundclasses read.  Soundclass array space %d, used %d.\n", pWorld->sizeSoundClasses, pWorld->numSoundClasses);
    SITH_ASSERTREL(pWorld->numSoundClasses <= pWorld->sizeSoundClasses);
    return 0;
}

int J3DAPI sithSoundClass_WriteSoundClassesListBinary(tFileHandle fh, const SithWorld* pWorld)
{
    for ( size_t i = 0; i < pWorld->numSoundClasses; ++i )
    {
        static_assert(sizeof(pWorld->aSoundClasses[i].aName) == 64, "sizeof(pWorld->aSoundClasses[i].aName)");
        if ( sith_g_pHS->pFileWrite(fh, &pWorld->aSoundClasses[i].aName, sizeof(pWorld->aSoundClasses[i].aName)) != sizeof(pWorld->aSoundClasses[i].aName) )
        {
            return 1;
        }
    }

    return 0;
}

int J3DAPI sithSoundClass_ReadSoundClassesListBinary(tFileHandle fh, SithWorld* pWorld)
{
    size_t numClasses = pWorld->numSoundClasses;
    if ( !numClasses )
    {
        return 0;
    }

    if ( sithSoundClass_AllocSoundClasses(pWorld, pWorld->sizeSoundClasses) )
    {
        return 1;
    }

    for ( size_t i = 0; i < numClasses; ++i )
    {
        char aFilename[64];
        if ( sith_g_pHS->pFileRead(fh, aFilename, sizeof(aFilename)) != sizeof(aFilename) )
        {
            return 1;
        }

        sithSoundClass_Load(pWorld, aFilename);
    }

    return 0;
}

SithSoundClass* J3DAPI sithSoundClass_Load(SithWorld* pWorld, const char* pName)
{
    SITH_ASSERTREL(pWorld && pName);
    SITH_ASSERTREL(strlen(pName) < 64);

    if ( !sithSoundClass_pHashtblModes )
    {
        return 0;
    }

    if ( streq(pName, "none") || !*pName )
    {
        return 0;
    }

    if ( !pWorld->aSoundClasses )
    {
        SITHLOG_ERROR("Soundclasses array not allocated for this world.\n");
        return 0;
    }

    SithSoundClass* pClass = sithSoundClass_CacheFind(pName);
    if ( pClass )
    {
        return pClass;
    }

    if ( pWorld->numSoundClasses == pWorld->sizeSoundClasses )
    {
        SITHLOG_ERROR("Too many sound classes loaded, line %d.\n", stdConffile_GetLineNumber());
        return 0;
    }

    pClass = &pWorld->aSoundClasses[pWorld->numSoundClasses];
    STD_STRCPY(pClass->aName, pName);

    char aPath[128];
    STD_FORMAT(aPath, "%s%c%s", "misc\\snd", '\\', pName); // Changed: Moved down here
    if ( sithSoundClass_LoadEntry(pWorld, pClass, aPath) )
    {
        SITHLOG_ERROR("Failed to load soundclass '%s'.\n", aPath);
        return 0;
    }

    ++pWorld->numSoundClasses;
    sithSoundClass_CacheAdd(pClass);
    return pClass;
}

int J3DAPI sithSoundClass_LoadEntry(SithWorld* pWorld, SithSoundClass* pClass, const char* pPath)
{

    SITH_ASSERTREL(pClass && pPath);

    if ( !stdConffile_Open(pPath) )
    {
        return 1;
    }

    SoundPlayFlag flags = SOUNDPLAY_ABSOLUTE_POS;
    while ( stdConffile_ReadArgs() )
    {
        if ( stdConffile_g_entry.numArgs < 2u )
        {
            SITHLOG_ERROR("Short line in file %s, line %d.\n", pPath, stdConffile_GetLineNumber());
            continue;
        }

        size_t modeNum = (size_t)stdHashtbl_Find(sithSoundClass_pHashTable, stdConffile_g_entry.aArgs[0].argValue);
        if ( !modeNum || modeNum >= STD_ARRAYLEN(sithSoundClass_aSoundModeNames) )
        {
            SITHLOG_ERROR("Mode %s not recognized in file %s.\n", stdConffile_g_entry.aArgs[0].argValue, pPath);
            continue;
        }

        if ( stdConffile_g_entry.numArgs > 2u )
        {
            sscanf_s(stdConffile_g_entry.aArgs[2].argValue, "%x", &flags);
        }

        tSoundHandle hSnd = 0;
        if ( !streq(stdConffile_g_entry.aArgs[1].argValue, "none") != 0 )
        {

            hSnd = sithSound_Load(pWorld, stdConffile_g_entry.aArgs[1].argValue);
            if ( !hSnd )
            {
                SITHLOG_ERROR("Mode %d sound %s could not be loaded.\n", modeNum, stdConffile_g_entry.aArgs[1].argValue);
                continue;
            }
        }

        SithSoundClassEntry* pEntry = (SithSoundClassEntry*)STDMALLOC(sizeof(SithSoundClassEntry));
        if ( !pEntry )
        {
            SITHLOG_ERROR("Malloc error loading soundclass entry in %s.\n", pPath);
            continue;
        }

        STD_ZEROMEM(pEntry, sizeof(SithSoundClassEntry));
        pEntry->hSnd      = hSnd;
        pEntry->playflags = flags;
        pEntry->minRadius = 0.5f;
        pEntry->maxRadius = 2.5f;
        pEntry->maxVolume = 1.0f;

        if ( stdConffile_g_entry.numArgs > 3u )
        {
            pEntry->minRadius =  atof(stdConffile_g_entry.aArgs[3].argValue);
        }

        if ( stdConffile_g_entry.numArgs > 4u )
        {
            pEntry->maxRadius =  atof(stdConffile_g_entry.aArgs[4].argValue);
        }

        if ( stdConffile_g_entry.numArgs > 5u )
        {
            pEntry->maxVolume = atof(stdConffile_g_entry.aArgs[5].argValue);
        }

        if ( !pClass->aEntries[modeNum] )
        {
            pClass->aEntries[modeNum] = pEntry;
            pEntry->numEntries = 1;
        }
        else
        {
            size_t count = 1;
            SithSoundClassEntry* pPrevEntry = pClass->aEntries[modeNum];
            while ( pPrevEntry->pNextMode )
            {
                pPrevEntry = pPrevEntry->pNextMode;
                ++count;
            }

            pPrevEntry->pNextMode = pEntry;
            pClass->aEntries[modeNum]->numEntries = count + 1;
        }

        // TODO: Should the flags be reset to SOUNDPLAY_ABSOLUTE_POS here?
    }

    stdConffile_Close();
    return 0;
}

int J3DAPI sithSoundClass_AllocSoundClasses(SithWorld* pWorld, size_t size)
{
    SITH_ASSERTREL(pWorld);
    SITH_ASSERTREL(size > 0);

    pWorld->aSoundClasses = (SithSoundClass*)STDMALLOC(sizeof(SithSoundClass) * size);
    if ( !pWorld->aSoundClasses )
    {
        SITHLOG_ERROR("Memory failure allocating %d soundclasses.\n", size);
        return 1;
    }

    pWorld->sizeSoundClasses = size;
    pWorld->numSoundClasses  = 0;
    STD_ZEROMEM(pWorld->aSoundClasses, sizeof(SithSoundClass) * size);
    return 0;
}

void J3DAPI sithSoundClass_FreeWorldSoundClasses(SithWorld* pWorld)
{
    SITH_ASSERTREL(pWorld);
    if ( pWorld->sizeSoundClasses )
    {
        SITH_ASSERTREL(pWorld->aSoundClasses);
        for ( size_t i = 0; i < pWorld->numSoundClasses; ++i )
        {
            sithSoundClass_Free(&pWorld->aSoundClasses[i]);
        }

        stdMemory_Free(pWorld->aSoundClasses);
        pWorld->aSoundClasses    = NULL;
        pWorld->sizeSoundClasses = 0;
        pWorld->numSoundClasses  = 0;
    }

    SITH_ASSERTREL(!pWorld->aSoundClasses);
}

void J3DAPI sithSoundClass_Free(SithSoundClass* pClass)
{
    sithSoundClass_CacheRemove(pClass);
    for ( size_t i = 0; i < STD_ARRAYLEN(sithSoundClass_aSoundModeNames); ++i )
    {
        SithSoundClassEntry* pNextMode;
        for ( SithSoundClassEntry* pCurMode = pClass->aEntries[i]; pCurMode; pCurMode = pNextMode )
        {
            pNextMode = pCurMode->pNextMode;
            stdMemory_Free(pCurMode);
        }
    }
}

tSoundChannelHandle J3DAPI sithSoundClass_PlayModeFirstEx(SithThing* pThing, SithSoundClassMode mode, float volume)
{
    SITH_ASSERTREL(pThing);
    if ( !pThing->pSoundClass )
    {
        return SOUND_INVALIDHANDLE;
    }

    if ( (size_t)mode >= STD_ARRAYLEN(sithSoundClass_aSoundModeNames) )
    {
        SITHLOG_ERROR("Sound mode %d out of range.\n", mode);
        return SOUND_INVALIDHANDLE;
    }

    if ( !pThing->pSoundClass->aEntries[mode] )
    {
        return SOUND_INVALIDHANDLE;
    }

    return sithSoundClass_PlayModeEntry(pThing, mode, pThing->pSoundClass->aEntries[mode], volume);
}

tSoundChannelHandle J3DAPI sithSoundClass_PlayModeFirst(SithThing* pThing, SithSoundClassMode mode)
{
    SITH_ASSERTREL(pThing);
    if ( !pThing->pSoundClass )
    {
        return SOUND_INVALIDHANDLE;
    }

    if ( (size_t)mode >= STD_ARRAYLEN(sithSoundClass_aSoundModeNames) )
    {
        SITHLOG_ERROR("Sound mode %d out of range.\n", mode);
        return SOUND_INVALIDHANDLE;
    }

    if ( !pThing->pSoundClass->aEntries[mode] )
    {
        return SOUND_INVALIDHANDLE;
    }

    return sithSoundClass_PlayModeEntry(pThing, mode, pThing->pSoundClass->aEntries[mode], 1.0f);
}

tSoundChannelHandle J3DAPI sithSoundClass_PlayModeRandom(SithThing* pThing, SithSoundClassMode mode)
{
    if ( !pThing )
    {
        SITHLOG_ERROR("NULL thing in sithSoundClass_PlayModeRandom()");
        return SOUND_INVALIDHANDLE;
    }

    if ( !pThing->pSoundClass )
    {
        return SOUND_INVALIDHANDLE;
    }

    if ( (size_t)mode >= STD_ARRAYLEN(sithSoundClass_aSoundModeNames) )
    {
        SITHLOG_ERROR("Sound modes out of range.\n");
        return SOUND_INVALIDHANDLE;
    }

    SithSoundClassEntry* pEntry = pThing->pSoundClass->aEntries[mode];
    if ( !pEntry )
    {
        return SOUND_INVALIDHANDLE;
    }

    if ( pEntry->numEntries <= 1u )
    {
        return sithSoundClass_PlayModeEntry(pThing, mode, pEntry, 1.0f);
    }

    size_t entryNum = (size_t)(SITH_RAND() * (double)pEntry->numEntries);
    if ( entryNum > pEntry->numEntries - 1 )
    {
        entryNum = pEntry->numEntries - 1;
    }

    for ( size_t i = 0; i < entryNum; ++i )
    {
        pEntry = pEntry->pNextMode;
    }

    return sithSoundClass_PlayModeEntry(pThing, mode, pEntry, 1.0f);
}

tSoundChannelHandle J3DAPI sithSoundClass_PlayVoiceModeRandom(SithThing* pThing, SithSoundClassMode mode)
{
    if ( !pThing )
    {
        SITHLOG_ERROR("NULL thing in sithSoundClass_PlayModeRandom()");
        return SOUND_INVALIDHANDLE;
    }

    if ( !pThing->pSoundClass )
    {
        return SOUND_INVALIDHANDLE;
    }

    if ( (size_t)mode >= STD_ARRAYLEN(sithSoundClass_aSoundModeNames) )
    {
        SITHLOG_ERROR("Sound modes out of range.\n");
        return SOUND_INVALIDHANDLE;
    }

    SithSoundClassEntry* pModeEntry = pThing->pSoundClass->aEntries[mode];
    if ( !pModeEntry )
    {
        return SOUND_INVALIDHANDLE;
    }

    if ( pModeEntry->numEntries <= 1u )
    {
        return sithVoice_PlayThingVoice(pThing, pModeEntry->hSnd, pModeEntry->maxVolume);
    }

    size_t entryNum = (int32_t)(SITH_RAND() * (double)pModeEntry->numEntries);
    if ( entryNum > pModeEntry->numEntries - 1 )
    {
        entryNum = pModeEntry->numEntries - 1;
    }

    for ( size_t i = 0; i < entryNum; ++i )
    {
        pModeEntry = pModeEntry->pNextMode;
    }

    return sithVoice_PlayThingVoice(pThing, pModeEntry->hSnd, pModeEntry->maxVolume);
}

tSoundChannelHandle J3DAPI sithSoundClass_PlayPlayerVoiceModeRandom(SithThing* pThing, SithSoundClassMode mode)
{
    SITH_ASSERTREL(pThing->type == SITH_THING_PLAYER);
    if ( !pThing )
    {
        SITHLOG_ERROR("NULL thing in sithSoundClass_PlayModeRandom()");
        return SOUND_INVALIDHANDLE;
    }

    if ( !pThing->pSoundClass )
    {
        return SOUND_INVALIDHANDLE;
    }

    if ( (size_t)mode >= STD_ARRAYLEN(sithSoundClass_aSoundModeNames) )
    {
        SITHLOG_ERROR("Sound modes out of range.\n");
        return SOUND_INVALIDHANDLE;
    }

    SithSoundClassEntry* pModeEntry = pThing->pSoundClass->aEntries[mode];
    if ( !pModeEntry )
    {
        return SOUND_INVALIDHANDLE;
    }

    if ( pModeEntry->numEntries <= 1u )
    {
        return sithVoice_PlayThingVoice(pThing, pModeEntry->hSnd, pModeEntry->maxVolume);
    }

    size_t entryNum = (size_t)(SITH_RAND() * (double)pModeEntry->numEntries);
    if ( entryNum > pModeEntry->numEntries - 1 )
    {
        entryNum = pModeEntry->numEntries - 1;
    }

    if ( entryNum == sithSoundClass_lastVoiceEntryNum && ++entryNum >= pModeEntry->numEntries )
    {
        entryNum = 0;
    }

    SITHLOG_STATUS("%d %d\n", entryNum, sithSoundClass_lastVoiceEntryNum);
    sithSoundClass_lastVoiceEntryNum = entryNum;

    for ( size_t i = 0; i < entryNum; ++i )
    {
        pModeEntry = pModeEntry->pNextMode;
    }

    return sithVoice_PlayThingVoice(pThing, pModeEntry->hSnd, pModeEntry->maxVolume);
}

tSoundChannelHandle J3DAPI sithSoundClass_PlayMode(SithThing* pThing, SithSoundClassMode mode, float selectionRatio)
{
    SITH_ASSERTREL(pThing);
    if ( !pThing->pSoundClass )
    {
        return SOUND_INVALIDHANDLE;
    }

    if ( (size_t)mode >= STD_ARRAYLEN(sithSoundClass_aSoundModeNames) )
    {
        SITHLOG_ERROR("Sound modes out of range.\n");
        return SOUND_INVALIDHANDLE;
    }

    SithSoundClassEntry* pEntry = pThing->pSoundClass->aEntries[mode];
    if ( !pEntry )
    {
        return SOUND_INVALIDHANDLE;
    }

    if ( pEntry->numEntries <= 1 )
    {
        return sithSoundClass_PlayModeEntry(pThing, mode, pEntry, 1.0f);
    }

    size_t numeEntries = (size_t)((double)pEntry->numEntries * selectionRatio);
    if ( numeEntries > pEntry->numEntries - 1 )
    {
        numeEntries = pEntry->numEntries - 1;
    }

    for ( size_t i = 0; i < numeEntries; ++i )
    {
        pEntry = pEntry->pNextMode;
    }

    return sithSoundClass_PlayModeEntry(pThing, mode, pEntry, 1.0f);
}

void J3DAPI sithSoundClass_StopMode(SithThing* pThing, SithSoundClassMode mode)
{
    if ( pThing->pSoundClass && (size_t)mode < STD_ARRAYLEN(sithSoundClass_aSoundModeNames) )
    {
        if ( pThing->pSoundClass->aEntries[mode] )
        {
            sithSoundMixer_StopSoundThing(pThing, pThing->pSoundClass->aEntries[mode]->hSnd);
        }
    }
}

void J3DAPI sithSoundClass_SetModePitch(SithThing* pThing, SithSoundClassMode mode, float pitch)
{
    if ( pThing->pSoundClass )
    {
        if ( (size_t)mode < STD_ARRAYLEN(sithSoundClass_aSoundModeNames) )
        {
            SithSoundClassEntry* pEntry = pThing->pSoundClass->aEntries[mode];
            if ( pEntry )
            {
                const size_t numEntries = pEntry->numEntries;
                for ( size_t i = 0; i < numEntries; ++i )
                {
                    sithSoundMixer_SetPitchThing(pThing, pEntry->hSnd, pitch);
                    pEntry = pEntry->pNextMode;
                }
            }
        }
    }
}

void J3DAPI sithSoundClass_SetModeVolume(SithThing* pThing, SithSoundClassMode mode, float volume)
{
    if ( pThing->pSoundClass )
    {
        if ( (size_t)mode < STD_ARRAYLEN(sithSoundClass_aSoundModeNames) )
        {
            SithSoundClassEntry* pEntry = pThing->pSoundClass->aEntries[mode];
            if ( pEntry )
            {
                const size_t numEntries = pEntry->numEntries;
                for ( size_t i = 0; i < numEntries; ++i )
                {
                    sithSoundMixer_SetVolumeThing(pThing, pEntry->hSnd, volume);
                    pEntry = pEntry->pNextMode;
                }
            }
        }
    }
}

void J3DAPI sithSoundClass_FadeModeVolume(SithThing* pThing, SithSoundClassMode mode, float startVolume, float endVolume)
{
    if ( pThing->pSoundClass )
    {
        if ( (size_t)mode < STD_ARRAYLEN(sithSoundClass_aSoundModeNames) )
        {
            SithSoundClassEntry* pEntry = pThing->pSoundClass->aEntries[mode];
            if ( pEntry )
            {
                const size_t numEntries = pEntry->numEntries;
                for ( size_t i = 0; i < numEntries; ++i )
                {
                    sithSoundMixer_FadeVolumeThing(pThing, pEntry->hSnd, startVolume, endVolume);
                    pEntry = pEntry->pNextMode;
                }
            }
        }
    }
}

int J3DAPI sithSoundClass_IsModeFadingVol(SithThing* pThing, SithSoundClassMode mode)
{
    if ( !pThing->pSoundClass || (size_t)mode >= STD_ARRAYLEN(sithSoundClass_aSoundModeNames) )
    {
        return 0;
    }

    SithSoundClassEntry* pEntry = pThing->pSoundClass->aEntries[mode];
    if ( !pEntry )
    {
        return 0;
    }

    return sithSoundMixer_IsThingFadingVol(pThing, pEntry->hSnd);
}

bool J3DAPI sithSoundClass_IsModePitchFading(SithThing* pThing, SithSoundClassMode mode)
{
    if ( !pThing->pSoundClass || (size_t)mode >= STD_ARRAYLEN(sithSoundClass_aSoundModeNames) )
    {
        return false;
    }

    SithSoundClassEntry* pEntry = pThing->pSoundClass->aEntries[mode];
    if ( !pEntry )
    {
        return false;
    }

    return sithSoundMixer_IsThingFadingPitch(pThing, pEntry->hSnd);
}

tSoundChannelHandle J3DAPI sithSoundClass_PlayModeEntry(SithThing* pThing, SithSoundClassMode mode, SithSoundClassEntry* pEntry, float volume)
{
    if ( !pEntry->hSnd )
    {
        return SOUND_INVALIDHANDLE;
    }

    float playVolume = pEntry->maxVolume * volume;

    if ( (pEntry->playflags & SOUNDPLAY_ABSOLUTE_POS) != 0 )
    {
        return sithSoundMixer_PlaySoundPos(pEntry->hSnd, &pThing->pos, pThing->pInSector, playVolume, pEntry->minRadius, pEntry->maxRadius, pEntry->playflags);
    }

    bool bMakeAIEvent = false;
    if ( pThing == sithPlayer_g_pLocalPlayerThing )
    {
        switch ( mode )
        {
            case SITHSOUNDCLASS_LWALKHARD:
            case SITHSOUNDCLASS_RWALKHARD:
            case SITHSOUNDCLASS_LWALKMETAL:
            case SITHSOUNDCLASS_RWALKMETAL:
            case SITHSOUNDCLASS_LWALKWATER:
            case SITHSOUNDCLASS_RWALKWATER:
            case SITHSOUNDCLASS_LWALKPUDDLE:
            case SITHSOUNDCLASS_RWALKPUDDLE:
            case SITHSOUNDCLASS_LWALKEARTH:
            case SITHSOUNDCLASS_RWALKEARTH:
            case SITHSOUNDCLASS_LWALKSNOW:
            case SITHSOUNDCLASS_RWALKSNOW:
            case SITHSOUNDCLASS_LWALKWOOD:
            case SITHSOUNDCLASS_RWALKWOOD:
            case SITHSOUNDCLASS_LWALKHARDECHO:
            case SITHSOUNDCLASS_RWALKHARDECHO:
            case SITHSOUNDCLASS_LWALKWOODECHO:
            case SITHSOUNDCLASS_RWALKWOODECHO:
            case SITHSOUNDCLASS_LWALKEARTHECHO:
            case SITHSOUNDCLASS_RWALKEARTHECHO:
            case SITHSOUNDCLASS_LWALKAET:
            case SITHSOUNDCLASS_RWALKAET:
            case SITHSOUNDCLASS_HURTIMPACT:
            case SITHSOUNDCLASS_HURTENERGY:
            case SITHSOUNDCLASS_HURTFIRE:
                break;

            default:
                bMakeAIEvent = true;
                break;
        }
    }

    if ( pThing->moveType == SITH_MT_PATH )
    {
        bMakeAIEvent = true;
    }

    if ( !bMakeAIEvent )
    {
        return sithSoundMixer_PlaySoundThing(pEntry->hSnd, pThing, playVolume, pEntry->minRadius, pEntry->maxRadius, pEntry->playflags);
    }

    float transmittingLevel = pEntry->maxRadius * 0.60000002f;
    sithAIAwareness_CreateTransmittingEvent(pThing->pInSector, &pThing->pos, /*type=*/0, transmittingLevel, pThing);
    return sithSoundMixer_PlaySoundThing(pEntry->hSnd, pThing, playVolume, pEntry->minRadius, pEntry->maxRadius, pEntry->playflags);
}

SithSoundClass* J3DAPI sithSoundClass_CacheFind(const char* pName)
{
    SITH_ASSERTREL(pName != NULL);
    if ( !sithSoundClass_pHashtblModes )
    {
        return NULL;
    }

    return (SithSoundClass*)stdHashtbl_Find(sithSoundClass_pHashtblModes, pName);
}

void J3DAPI sithSoundClass_CacheAdd(SithSoundClass* pClass)
{
    SITH_ASSERTREL(pClass != NULL);
    SITH_ASSERTREL(strlen(pClass->aName) > 0);

    if ( sithSoundClass_pHashtblModes )
    {
        stdHashtbl_Add(sithSoundClass_pHashtblModes, pClass->aName, pClass);
    }
}

void J3DAPI sithSoundClass_CacheRemove(const SithSoundClass* pClass)
{
    SITH_ASSERTREL(pClass != NULL);
    SITH_ASSERTREL(strlen(pClass->aName) > 0);

    if ( sithSoundClass_pHashtblModes )
    {
        stdHashtbl_Remove(sithSoundClass_pHashtblModes, pClass->aName);
    }
}

int J3DAPI sithSoundClass_SetThingClass(SithThing* pThing, SithSoundClass* pSoundClass)
{
    SITH_ASSERTREL(pThing && pSoundClass);
    if ( pThing->pSoundClass == pSoundClass )
    {
        return 0;
    }

    pThing->pSoundClass = pSoundClass;
    return 1;
}
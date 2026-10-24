#include "Sound.h"
#include <j3dcore/j3dhook.h>

#include <rdroid/Math/rdVector.h>
#include <rdroid/Math/rdMatrix.h>

#include "AudioLib.h"
#include "Driver.h"
#include <sound/RTI/symbols.h>

#include <std/General/stdMath.h>
#include <std/General/stdUtil.h>

#include <limits.h>

// General module constants
#define SOUND_INIT_SOUNDINFO_BUFFERSIZE 16

// Sound bank constants
#define SOUNDBANK_STATIC_NUM 0
#define SOUNDBANK_NORMAL_NUM 1
#define SOUNDBANK_NUMBANKS   2

// Sound playback constants
#define SOUND_MAXCREATERETRIES 4

// Lip-sync constants
#define SOUND_LIPSYNC_ANALYSIS_RATE_HZ    60u
#define SOUND_LIPSYNC_END_MARGIN_MSEC     25u
#define SOUND_MILLISECONDS_PER_SECOND     1000u

// Sound load constants and macros
#define SOUND_LOADSTATICIDXMASK 0x12344321 // Mask for static sound index

// Sound handle constants & helper macros
#define SOUND_HANDLE_ENTROPY_MAX   1111111u // Note, max 555555 handles per handle type (sound or channel)
#define SOUND_HANDLE_MIN           1234u
#define SOUND_HANDLE_MAX           (SOUND_HANDLE_MIN + SOUND_HANDLE_ENTROPY_MAX)

#define SOUND_IS_SOUNDINFOHANDLE(handle) ( handle >= SOUND_HANDLE_MIN && handle < SOUND_HANDLE_MAX && (handle & 1) == 0 )
#define SOUND_IS_CHANNELHANDLE(handle) ( handle >= SOUND_HANDLE_MIN && handle < SOUND_HANDLE_MAX && (handle & 1) != 0 )

// Logging macros
#define SOUNDLOG_DEBUG(format, ...) \
    Sound_pHS->pDebugPrint(format, ##__VA_ARGS__);
    //J3DLOG_DEBUG(Sound_pHS, format, ##__VA_ARGS__)

#define SOUNDLOG_STATUS(format, ...) \
    Sound_pHS->pStatusPrint(format, ##__VA_ARGS__);
    //J3DLOG_STATUS(Sound_pHS, format, ##__VA_ARGS__)

#define SOUNDLOG_MESSAGE(format, ...) \
    Sound_pHS->pMessagePrint(format, ##__VA_ARGS__);
    //J3DLOG_MESSAGE(Sound_pHS, format, ##__VA_ARGS__)

#define SOUNDLOG_WARNING(format, ...) \
    Sound_pHS->pWarningPrint(format, ##__VA_ARGS__);
    //J3DLOG_WARNING(Sound_pHS, format, ##__VA_ARGS__)

#define SOUNDLOG_ERROR(format, ...) \
    Sound_pHS->pErrorPrint(format, ##__VA_ARGS__);
    //J3DLOG_ERROR(Sound_pHS, format, ##__VA_ARGS__)

#define SOUNDLOG_FATAL(message) \
    Sound_pHS->pAssert(message, J3D_FILE, __LINE__);
    //J3DLOG_FATAL(Sound_pHS, message)

#define SOUND_ASSERT(condition) \
    J3D_ASSERT(condition, Sound_pHS)

#define SOUND_ASSERTREL(condition) \
    J3D_ASSERTREL(condition, Sound_pHS)

// Sound module state enum
typedef enum eSoundModuleState
{
    SOUNDSTATE_NONE        = 0,
    SOUNDSTATE_INITIALIZED = 1,
    SOUNDSTATE_STARTUP     = 2,
    SOUNDSTATE_OPEN        = 3,
} SoundModuleState;

// Note don't change the int types of vars that are being serialized on save/restore

// Module state
static SoundModuleState Sound_state        = SOUNDSTATE_NONE; // Added: Init to none
static tHostServices* Sound_pHS;
static int Sound_bLoadEnabled;
static int Sound_pausedRefCount;
static uint32_t Sound_msecPauseStartTime;
static uint32_t Sound_msecElapsed;

// Module options
static int Sound_bNoSound3D;
static int Sound_bReverseSound;
static int Sound_bGlobalFocusBuf;
static float Sound_maxVolume          = 1.0f;
static int Sound_bNoSoundCompression;
static int Sound_bNoLipSync;

// Misc vars
static HWND Sound_hwnd;
static tDirectSound* Sound_pDirectSound;

// 3D sound vars
static rdVector3 Sound_curUpdatePos;
static SoundCalcListenerSoundMixFunc Sound_pfCalcListenerSoundMix;
static SoundGetThingInfoCallback Sound_pfGetThingInfoCallback;

// Save/Restore vars
static const uint32_t Sound_serMagic        = 0x12345678u;
static const uint32_t Sound_serSoundListEnd = 0u;
static char Sound_sndFilename[256]          = { 0 }; // Added: Init to 0

// Sound and soundbank load vars
static size_t soundbank_lastImportedBankNum;
static uint8_t Sound_loadBuffer[10000];

// Soundbank vars
static size_t Sound_handleEntropy;

// Loaded sound data buffer vars
static size_t Sound_cacheBlockSize          = 0x200000;
static size_t soundbank_aCacheSizes[2];
static uint8_t* soundbank_apSoundCache[2];
static size_t soundbank_aUsedCacheSizes[2];

// Loaded sound info vars
static size_t soundbank_aNumSounds[2];
static size_t soundbank_aSizeSounds[2];
static SoundInfo* soundbank_apSoundInfos[2];

// Playing sound vars
static size_t Sound_maxChannels;
static size_t Sound_numChannels;
static size_t Sound_sizeChannels;
static tSoundChannel* Sound_apChannels;

// Sound fade vars
static SoundFade Sound_aFades[48];

// In-memory file buffer vars
static uint8_t* Sound_pMemfileBuf;
static size_t Sound_memfileSize;
static size_t Sound_memfilePos;

#if defined(J3D_TEST)
static int Sound_test_bCurrentPositionSet;
static size_t Sound_test_currentPosition;
static SoundTestUncompressFunc Sound_test_pfUncompress;
#endif

tSoundHandle Sound_GenerateSoundHandle(void);
tSoundChannelHandle Sound_GenerateChannelHandle(void);
uint32_t Sound_GetEntropyFromHandle(tSoundHandleType handle, bool bChannelHandle); // Added
uint32_t Sound_GetEntropyFromSoundHandle(tSoundHandle handle); // Added
uint32_t Sound_GetEntropyFromChannelHandle(tSoundChannelHandle handle); // Added

size_t J3DAPI Sound_GetFreeCache(size_t bankNum, size_t requiredSize);
void J3DAPI Sound_IncreaseFreeCache(size_t bankNum, size_t nSize);
size_t J3DAPI Sound_WriteSoundFilepathToBank(size_t bankNum, const char* pName, size_t len);
size_t Sound_GetDeltaTime(void);

uint8_t* J3DAPI Sound_GetSoundBufferData(tSysSoundBuffer* pSndBuf, size_t* pDataSize, uint32_t* pbCompressed);
int J3DAPI Sound_MemFileSeek(tFileHandle fh, int offset, int origin);
size_t J3DAPI Sound_MemFileRead(tFileHandle fh, void* pOutData, size_t nRead);
size_t J3DAPI Sound_MemFileWrite(tFileHandle fh, const void* pData, size_t size);
tFileHandle J3DAPI Sound_MemFileReset();
void J3DAPI Sound_MemFileFree(void);

void Sound_StopAllNonStaticSounds(void);

static size_t Sound_GetLipSyncCurrentPosition(tSysSoundBuffer* pSoundBuffer);
static void Sound_ResetLipSyncCompressor(tAudioCompressorState* pCompressorState);
static void Sound_UncompressLipSyncData(tAudioCompressorState* pCompressorState, uint8_t* pOutSndData, const uint8_t* pCompressedData, unsigned int size);

int J3DAPI Sound_ExportBank(tFileHandle fh, size_t bankNum);
int J3DAPI Sound_ImportBank(tFileHandle fh, size_t bankNum);
int J3DAPI Sound_ReadFile(tFileHandle fh, void* pData, size_t size);

void Sound_InstallHooks(void)
{
    J3D_HOOKFUNC(Sound_Initialize);
    J3D_HOOKFUNC(Sound_Uninitialize);
    J3D_HOOKFUNC(Sound_Startup);
    J3D_HOOKFUNC(Sound_Shutdown);
    J3D_HOOKFUNC(Sound_Open);
    J3D_HOOKFUNC(Sound_Close);
    J3D_HOOKFUNC(Sound_Save);
    J3D_HOOKFUNC(Sound_Restore);
    J3D_HOOKFUNC(Sound_Pause);
    J3D_HOOKFUNC(Sound_Resume);
    J3D_HOOKFUNC(Sound_Set3DGlobals);
    J3D_HOOKFUNC(Sound_Has3DHW);
    J3D_HOOKFUNC(Sound_Set3DHWState);
    J3D_HOOKFUNC(Sound_Get3DHWState);
    J3D_HOOKFUNC(Sound_GetMemoryUsage);
    J3D_HOOKFUNC(Sound_Load);
    J3D_HOOKFUNC(Sound_LoadStatic);
    J3D_HOOKFUNC(Sound_Reset);
    J3D_HOOKFUNC(Sound_EnableLoad);
    J3D_HOOKFUNC(Sound_ExportStaticBank);
    J3D_HOOKFUNC(Sound_ImportStaticBank);
    J3D_HOOKFUNC(Sound_ExportNormalBank);
    J3D_HOOKFUNC(Sound_ImportNormalBank);
    J3D_HOOKFUNC(Sound_SkipSoundFileSection);
    J3D_HOOKFUNC(Sound_GetSoundHandle);
    J3D_HOOKFUNC(Sound_GetSoundIndex);
    J3D_HOOKFUNC(Sound_GetChannelHandle);
    J3D_HOOKFUNC(Sound_GetChannelGUID);
    J3D_HOOKFUNC(Sound_GetSoundFilename);
    J3D_HOOKFUNC(Sound_GetLengthMsec);
    J3D_HOOKFUNC(Sound_SetMaxVolume);
    J3D_HOOKFUNC(Sound_GetMaxVolume);
    J3D_HOOKFUNC(Sound_Play);
    J3D_HOOKFUNC(Sound_PlayPos);
    J3D_HOOKFUNC(Sound_PlayThing);
    J3D_HOOKFUNC(Sound_StopChannel);
    J3D_HOOKFUNC(Sound_StopAllSounds);
    J3D_HOOKFUNC(Sound_StopThing);
    J3D_HOOKFUNC(Sound_SetVolume);
    J3D_HOOKFUNC(Sound_FadeVolume);
    J3D_HOOKFUNC(Sound_SetVolumeThing);
    J3D_HOOKFUNC(Sound_FadeVolumeThing);
    J3D_HOOKFUNC(Sound_IsThingFadingVol);
    J3D_HOOKFUNC(Sound_SetPitch);
    J3D_HOOKFUNC(Sound_GetPitch);
    J3D_HOOKFUNC(Sound_FadePitch);
    J3D_HOOKFUNC(Sound_SetPitchThing);
    J3D_HOOKFUNC(Sound_GetChannelFlags);
    J3D_HOOKFUNC(Sound_Update);
    J3D_HOOKFUNC(Sound_GenerateLipSync);
    J3D_HOOKFUNC(Sound_SetReverseSound);
    J3D_HOOKFUNC(Sound_GenerateSoundHandle);
    J3D_HOOKFUNC(Sound_GenerateChannelHandle);
    J3D_HOOKFUNC(Sound_GetFreeCache);
    J3D_HOOKFUNC(Sound_IncreaseFreeCache);
    J3D_HOOKFUNC(Sound_WriteSoundFilepathToBank);
    J3D_HOOKFUNC(Sound_GetDeltaTime);
    J3D_HOOKFUNC(Sound_GetSoundInfo);
    J3D_HOOKFUNC(Sound_GetChannel);
    J3D_HOOKFUNC(Sound_GetChannelBySoundHandle);
    J3D_HOOKFUNC(Sound_GetSoundBufferData);
    J3D_HOOKFUNC(Sound_MemFileRead);
    J3D_HOOKFUNC(Sound_MemFileWrite);
    J3D_HOOKFUNC(Sound_MemFileReset);
    J3D_HOOKFUNC(Sound_MemFileFree);
    J3D_HOOKFUNC(Sound_StopAllNonStaticSounds);
    J3D_HOOKFUNC(Sound_ExportBank);
    J3D_HOOKFUNC(Sound_ImportBank);
    J3D_HOOKFUNC(Sound_ReadFile);
}

static void Sound_Release(tSoundChannel* pChannel) // Added
{
    if ( !pChannel )
    {
        return;
    }

    SoundDriver_Release(pChannel->pDSoundBuffer);
    pChannel->pDSoundBuffer = NULL;
    pChannel->handle        = SOUND_INVALIDHANDLE;
    pChannel->hSnd          = SOUND_INVALIDHANDLE;
    pChannel->thingId       = 0;
}

void Sound_ResetGlobals(void)
{}

// Added: Keep external audio-driver dependencies replaceable in deterministic unit tests.
static size_t Sound_GetLipSyncCurrentPosition(tSysSoundBuffer* pSoundBuffer)
{
#if defined(J3D_TEST)
    if ( Sound_test_bCurrentPositionSet )
    {
        return Sound_test_currentPosition;
    }
#endif

    return SoundDriver_GetCurrentPosition(pSoundBuffer);
}

static void Sound_ResetLipSyncCompressor(tAudioCompressorState* pCompressorState)
{
#if defined(J3D_TEST)
    if ( Sound_test_pfUncompress )
    {
        STD_ZEROMEM(pCompressorState, sizeof(*pCompressorState));
        return;
    }
#endif

    AudioLib_ResetCompressor(pCompressorState);
}

static void Sound_UncompressLipSyncData(tAudioCompressorState* pCompressorState, uint8_t* pOutSndData, const uint8_t* pCompressedData, unsigned int size)
{
#if defined(J3D_TEST)
    if ( Sound_test_pfUncompress )
    {
        Sound_test_pfUncompress(pCompressorState, pOutSndData, pCompressedData, size);
        return;
    }
#endif

    AudioLib_Uncompress(pCompressorState, pOutSndData, pCompressedData, size);
}

#if defined(J3D_TEST)
void Sound_TestResetLipSyncState(void)
{
    Sound_state                    = SOUNDSTATE_NONE;
    Sound_pHS                      = NULL;
    Sound_bNoLipSync               = 0;
    Sound_apChannels               = NULL;
    Sound_numChannels              = 0;
    Sound_sizeChannels             = 0;
    Sound_test_bCurrentPositionSet = 0;
    Sound_test_currentPosition     = 0;
    Sound_test_pfUncompress        = NULL;

    STD_ZEROMEM(soundbank_apSoundCache, sizeof(soundbank_apSoundCache));
    STD_ZEROMEM(soundbank_aCacheSizes, sizeof(soundbank_aCacheSizes));
    STD_ZEROMEM(soundbank_aUsedCacheSizes, sizeof(soundbank_aUsedCacheSizes));
    STD_ZEROMEM(soundbank_apSoundInfos, sizeof(soundbank_apSoundInfos));
    STD_ZEROMEM(soundbank_aNumSounds, sizeof(soundbank_aNumSounds));
    STD_ZEROMEM(soundbank_aSizeSounds, sizeof(soundbank_aSizeSounds));
}

void Sound_TestSetChannels(tSoundChannel* pChannels, size_t numChannels)
{
    Sound_apChannels   = pChannels;
    Sound_numChannels  = numChannels;
    Sound_sizeChannels = numChannels;
}

void Sound_TestSetSoundBank(size_t bankNum, SoundInfo* pSoundInfos, size_t numSounds, uint8_t* pSoundCache, size_t cacheSize)
{
    if ( bankNum >= SOUNDBANK_NUMBANKS )
    {
        return;
    }

    soundbank_apSoundInfos[bankNum]    = pSoundInfos;
    soundbank_aNumSounds[bankNum]      = numSounds;
    soundbank_aSizeSounds[bankNum]     = numSounds;
    soundbank_apSoundCache[bankNum]    = pSoundCache;
    soundbank_aCacheSizes[bankNum]     = cacheSize;
    soundbank_aUsedCacheSizes[bankNum] = cacheSize;
}

void Sound_TestSetNoLipSync(int bNoLipSync)
{
    Sound_bNoLipSync = bNoLipSync != 0;
}

void Sound_TestSetCurrentPosition(size_t position)
{
    Sound_test_bCurrentPositionSet = 1;
    Sound_test_currentPosition     = position;
}

void Sound_TestSetUncompressFunc(SoundTestUncompressFunc pfUncompress)
{
    Sound_test_pfUncompress = pfUncompress;
}
#endif

int J3DAPI Sound_Initialize(tHostServices* pHS)
{
    // Fixed: Added check for pHS, as module requires it to be initialized
    if ( !pHS )
    {
        return 1;
    }

    Sound_pHS = pHS;
    if ( Sound_state != SOUNDSTATE_NONE )
    {
        SOUNDLOG_ERROR("Sound_Initialize: module already in initialized state.");
        return 1;
    }

    Sound_state = SOUNDSTATE_INITIALIZED;
    return 0;
}

void Sound_Uninitialize(void)
{
    if ( Sound_state == SOUNDSTATE_INITIALIZED )
    {
        Sound_state = SOUNDSTATE_NONE;
    }

    Sound_pHS = NULL;
}

int Sound_Startup(void)
{
    if ( Sound_state == SOUNDSTATE_STARTUP )
    {
        SOUNDLOG_ERROR("Sound_Startup: module already in startup state.");
        return 0;
    }

    if ( Sound_state != SOUNDSTATE_INITIALIZED )
    {
        if ( Sound_pHS )
        {
            SOUNDLOG_ERROR("Sound_Startup: module hasn't been initialized, yet.");
        }

        return 0;
    }

    Sound_state = SOUNDSTATE_STARTUP;
    return 1;
}

void Sound_Shutdown(void)
{
    if ( Sound_state == SOUNDSTATE_STARTUP )
    {
        Sound_state = SOUNDSTATE_INITIALIZED;
    }
}

int J3DAPI Sound_Open(SoundOpenFlags flags, size_t maxSoundChannels, SoundGetThingInfoCallback pfGetThingInfo, SoundCalcListenerSoundMixFunc pfCalcListenerSoundMix, HWND hwnd)
{
    if ( Sound_state == SOUNDSTATE_OPEN )
    {
        SOUNDLOG_ERROR("Sound_Open: module already in open state.");
        return 0;
    }

    if ( Sound_state != SOUNDSTATE_STARTUP )
    {
        SOUNDLOG_ERROR("Sound_Open: module not in startup state.");
        return 0;
    }

    // Open sound driver
#ifdef J3D_DIRECTX9
    SOUNDLOG_STATUS("Sound_Open: Opening sound driver with DirectSound8 as backend...\n");
#endif

    Sound_bNoSound3D             = (flags & SOUNDOPEN_NO3DSOUND) != 0;
    Sound_bGlobalFocusBuf        = (flags & SOUNDOPEN_GLOBALFOCUS) != 0;
    Sound_hwnd                   = hwnd;
    Sound_pDirectSound           = (flags & SOUNDOPEN_HWNDISDIRECTSOUND) != 0 ? (tDirectSound*)hwnd : NULL;
    Sound_pfCalcListenerSoundMix = pfCalcListenerSoundMix;

    if ( !SoundDriver_Open(Sound_bNoSound3D, Sound_bGlobalFocusBuf, hwnd, Sound_pDirectSound, pfCalcListenerSoundMix, Sound_GetSoundBufferData, Sound_pHS->pErrorPrint) )
    {
        SOUNDLOG_ERROR("Sound_Open: Failed to open sound driver!"); // Added log
        return 0;
    }

    Sound_msecElapsed = SoundDriver_GetTimeMsec();
    memset(Sound_aFades, 0, sizeof(Sound_aFades));

    Sound_pausedRefCount         = 0;
    Sound_handleEntropy          = 0;
    Sound_maxChannels            = maxSoundChannels;
    Sound_pfGetThingInfoCallback = pfGetThingInfo;
    Sound_bNoSoundCompression    = (flags & SOUNDOPEN_NOSOUNDCOMPRESSION) != 0;
    Sound_bNoLipSync             = (flags & SOUNDOPEN_NOLIPSYNC) != 0;

    Sound_Reset(1);
    Sound_EnableLoad(1);

    Sound_state = SOUNDSTATE_OPEN;
    return 1;
}

void Sound_Close(void)
{
    if ( Sound_state == SOUNDSTATE_OPEN )
    {
        Sound_Reset(1);
        SoundDriver_Close();

        if ( Sound_apChannels )
        {
            Sound_pHS->pFree(Sound_apChannels);
            Sound_apChannels = NULL;
        }

        Sound_sizeChannels = 0;
        Sound_numChannels  = 0;
        Sound_state = SOUNDSTATE_STARTUP;
    }
}

int J3DAPI Sound_Save(tFileHandle fh)
{
    if ( Sound_state < SOUNDSTATE_INITIALIZED )
    {
        SOUNDLOG_ERROR("Sound_Save: module is not initialized.\n");
        return 0;
    }

    // section marker (header magic)
    if ( Sound_pHS->pFileWrite(fh, &Sound_serMagic, sizeof(Sound_serMagic)) != sizeof(Sound_serMagic) )
    {
        static_assert(sizeof(Sound_serMagic) == 4, "sizeof(Sound_sermagic) == 4");
        return 0;
    }

    if ( Sound_pHS->pFileWrite(fh, &Sound_pausedRefCount, sizeof(Sound_pausedRefCount)) != sizeof(Sound_pausedRefCount) )
    {
        static_assert(sizeof(Sound_pausedRefCount) == 4, "sizeof(Sound_pausedRefCount) == 4");
        return 0;
    }

    const uint32_t curDetlatTime = Sound_GetDeltaTime();
    if ( Sound_pHS->pFileWrite(fh, &curDetlatTime, sizeof(curDetlatTime)) != sizeof(curDetlatTime) )
    {
        static_assert(sizeof(curDetlatTime) == 4, "sizeof(curDetlatTime) == 4");
        return 0;
    }

    const uint32_t curDetlatPauseTime = SoundDriver_GetTimeMsec() - Sound_msecPauseStartTime;
    if ( Sound_pHS->pFileWrite(fh, &curDetlatPauseTime, sizeof(curDetlatPauseTime)) != sizeof(curDetlatPauseTime) )
    {
        static_assert(sizeof(curDetlatPauseTime) == 4, "sizeof(curDetlatPauseTime) == 4");
        return 0;
    }

    int channelNum = Sound_numChannels;
    tSoundChannel* pCurChannel = &Sound_apChannels[Sound_numChannels];

    while ( 1 )
    {
        --pCurChannel;
        if ( !channelNum-- )
        {
            break;
        }

        // TODO: Save also 2D sounds
        if ( pCurChannel->handle != SOUND_INVALIDHANDLE
            && ((pCurChannel->flags & (SOUND_CHANNEL_LOOP | SOUND_CHANNEL_PLAYING)) == (SOUND_CHANNEL_LOOP | SOUND_CHANNEL_PLAYING)
                || (pCurChannel->flags & SOUND_CHANNEL_FAR) != 0)
            && (pCurChannel->flags & SOUND_CHANNEL_3DSOUND) != 0 )
        {
            SoundInfo* pSndInfo = Sound_GetSoundInfo(pCurChannel->hSnd);
            if ( pSndInfo )
            {
                uint8_t* aSndCache = soundbank_apSoundCache[pSndInfo->bankNum];

                // TODO: Make sure name max len should be less or equal than STD_ARRAYLEN(Sound_sndFilename)
                const uint32_t nameLen = strlen((const char*)&aSndCache[pSndInfo->filePathOffset]) + 1;
                if ( Sound_pHS->pFileWrite(fh, &nameLen, sizeof(nameLen)) != sizeof(nameLen) )
                {
                    static_assert(sizeof(nameLen) == 4, "sizeof(nameLen) == 4");
                    return 0;
                }

                // Write filename
                if ( Sound_pHS->pFileWrite(fh, &aSndCache[pSndInfo->filePathOffset], nameLen) != nameLen )
                {
                    return 0;
                }

                // Read sound bank num
                const uint32_t bankNum = pSndInfo->bankNum;
                if ( Sound_pHS->pFileWrite(fh, &bankNum, sizeof(bankNum)) != sizeof(bankNum) )
                {
                    static_assert(sizeof(bankNum) == 4, "sizeof(bankNum) == 4");
                    return 0;
                }

                // Read sound index
                const uint32_t sndIdx = pSndInfo->idx;
                if ( Sound_pHS->pFileWrite(fh, &sndIdx, sizeof(sndIdx)) != sizeof(sndIdx) )
                {
                    static_assert(sizeof(sndIdx) == 4, "sizeof(sndIdx) == 4");
                    return 0;
                }

                const uint32_t hSnd = pSndInfo->hSnd;
                if ( Sound_pHS->pFileWrite(fh, &hSnd, sizeof(hSnd)) != sizeof(hSnd) )
                {
                    static_assert(sizeof(hSnd) == 4, "sizeof(hSnd) == 4");
                    return 0;
                }

                if ( Sound_pHS->pFileWrite(fh, &pCurChannel->handle, sizeof(pCurChannel->handle)) != sizeof(pCurChannel->handle) )
                {
                    static_assert(sizeof(pCurChannel->handle) == 4, "sizeof(pCurChannel->handle) == 4");
                    return 0;
                }

                if ( Sound_pHS->pFileWrite(fh, &pCurChannel->priority, sizeof(pCurChannel->priority)) != sizeof(pCurChannel->priority) )
                {
                    static_assert(sizeof(pCurChannel->priority) == 4, "sizeof(pCurChannel->handle) == 4");

                    return 0;
                }

                if ( Sound_pHS->pFileWrite(fh, &pCurChannel->thingId, sizeof(pCurChannel->thingId)) != sizeof(pCurChannel->thingId) )
                {
                    static_assert(sizeof(pCurChannel->thingId) == 4, "sizeof(pCurChannel->thingId) == 4");
                    return 0;
                }

                if ( Sound_pHS->pFileWrite(fh, &pCurChannel->playPos.x, sizeof(pCurChannel->playPos.x)) != sizeof(pCurChannel->playPos.x) )
                {
                    static_assert(sizeof(pCurChannel->playPos.x) == 4, "sizeof(pCurChannel->playPos.x) == 4");
                    return 0;
                }

                if ( Sound_pHS->pFileWrite(fh, &pCurChannel->playPos.y, sizeof(pCurChannel->playPos.y)) != sizeof(pCurChannel->playPos.y) )
                {
                    static_assert(sizeof(pCurChannel->playPos.y) == 4, "sizeof(pCurChannel->playPos.y) == 4");
                    return 0;
                }

                if ( Sound_pHS->pFileWrite(fh, &pCurChannel->playPos.z, sizeof(pCurChannel->playPos.z)) != sizeof(pCurChannel->playPos.z) )
                {
                    static_assert(sizeof(pCurChannel->playPos.z) == 4, "sizeof(pCurChannel->playPos.z) == 4");
                    return 0;
                }

                if ( Sound_pHS->pFileWrite(fh, &pCurChannel->volume, sizeof(pCurChannel->volume)) != sizeof(pCurChannel->volume) )
                {
                    static_assert(sizeof(pCurChannel->volume) == 4, "sizeof(pCurChannel->volume) == 4");
                    return 0;
                }

                if ( Sound_pHS->pFileWrite(fh, &pCurChannel->pitch, sizeof(pCurChannel->pitch)) != sizeof(pCurChannel->pitch) )
                {
                    static_assert(sizeof(pCurChannel->pitch) == 4, "sizeof(pCurChannel->pitch) == 4");
                    return 0;
                }

                if ( Sound_pHS->pFileWrite(fh, &pCurChannel->sampleRate, sizeof(pCurChannel->sampleRate)) != sizeof(pCurChannel->sampleRate) )
                {
                    static_assert(sizeof(pCurChannel->sampleRate) == 4, "sizeof(pCurChannel->sampleRate) == 4");
                    return 0;
                }

                if ( Sound_pHS->pFileWrite(fh, &pCurChannel->flags, sizeof(pCurChannel->flags)) != sizeof(pCurChannel->flags) )
                {
                    static_assert(sizeof(pCurChannel->flags) == 4, "sizeof(pCurChannel->flags) == 4");
                    return 0;
                }

                if ( Sound_pHS->pFileWrite(fh, &pCurChannel->guid, sizeof(pCurChannel->guid)) != sizeof(pCurChannel->guid) )
                {
                    static_assert(sizeof(pCurChannel->guid) == 4, "sizeof(pCurChannel->guid) == 4");
                    return 0;
                }

                if ( Sound_pHS->pFileWrite(fh, &pCurChannel->minRadius, sizeof(pCurChannel->minRadius)) != sizeof(pCurChannel->minRadius) )
                {
                    static_assert(sizeof(pCurChannel->minRadius) == 4, "sizeof(pCurChannel->minRadius) == 4");
                    return 0;
                }

                if ( Sound_pHS->pFileWrite(fh, &pCurChannel->maxRadius, sizeof(pCurChannel->maxRadius)) != sizeof(pCurChannel->maxRadius) )
                {
                    static_assert(sizeof(pCurChannel->maxRadius) == 4, "sizeof(pCurChannel->maxRadius) == 4");
                    return 0;
                }

                if ( Sound_pHS->pFileWrite(fh, &pCurChannel->envflags, sizeof(pCurChannel->envflags)) != sizeof(pCurChannel->envflags) )
                {
                    static_assert(sizeof(pCurChannel->envflags) == 4, "sizeof(pCurChannel->envflags) == 4");
                    return 0;
                }
            }
        }
    }

    // Marks end of list
    if ( Sound_pHS->pFileWrite(fh, &Sound_serSoundListEnd, sizeof(Sound_serSoundListEnd)) != sizeof(Sound_serSoundListEnd) )
    {
        static_assert(sizeof(Sound_serSoundListEnd) == 4, "sizeof(Sound_serSoundListEnd) == 4");
        return 0;
    }

    static_assert(sizeof(Sound_aFades) == 1152, "sizeof(Sound_aFades) == 1152");
    return Sound_pHS->pFileWrite(fh, &Sound_handleEntropy, sizeof(uint32_t)) == sizeof(uint32_t)
        && Sound_pHS->pFileWrite(fh, Sound_aFades, sizeof(Sound_aFades)) == sizeof(Sound_aFades);
}

int J3DAPI Sound_Restore(tFileHandle fh)
{
    if ( Sound_state != SOUNDSTATE_OPEN )
    {
        // Sound module is not opened, read out sound list anyway so the file read offset is moved to the end of sound section
        SOUNDLOG_ERROR("Sound_Restore: module is not open.\n");
    }

    uint32_t magic;
    if ( Sound_pHS->pFileRead(fh, &magic, sizeof(magic)) != sizeof(magic) )
    {
        return 0;
    }

    if ( magic != Sound_serMagic )
    {
        // We're not at the ser sound position roll back position for sizeof(magic)
        Sound_pHS->pFileSeek(fh, -(int)sizeof(magic), 1);
        return 0;
    }

    // Set initial listener position
    if ( Sound_state == SOUNDSTATE_OPEN )
    {
        rdVector3 listnerPos;
        listnerPos.x = 100000.0f;
        listnerPos.y = 100000.0f;
        listnerPos.z = 100000.0f;
        Sound_StopAllSounds();
        Sound_Update(&listnerPos, NULL, NULL, NULL);
    }

    if ( Sound_pHS->pFileRead(fh, &Sound_pausedRefCount, sizeof(uint32_t)) != sizeof(uint32_t) )
    {
        return 0;
    }

    Sound_pausedRefCount = 0; // ???

    uint32_t pauseDeltaTime;
    if ( Sound_pHS->pFileRead(fh, &pauseDeltaTime, sizeof(pauseDeltaTime)) != sizeof(pauseDeltaTime) )
    {
        return 0;
    }

    Sound_msecElapsed = SoundDriver_GetTimeMsec() - pauseDeltaTime;

    if ( Sound_pHS->pFileRead(fh, &Sound_msecPauseStartTime, sizeof(uint32_t)) != sizeof(uint32_t) )
    {
        return 0;
    }

    Sound_msecPauseStartTime += SoundDriver_GetTimeMsec();

    // Read sound file name length
    uint32_t filenameLen;
    if ( Sound_pHS->pFileRead(fh, &filenameLen, sizeof(filenameLen)) != sizeof(filenameLen) )
    {
        return 0;
    }

    // Added: Added tracking of max entropy value
    size_t maxEntropy = 0;

    while ( filenameLen != Sound_serSoundListEnd )
    {
        if ( filenameLen <= STD_ARRAYLEN(Sound_sndFilename) )
        {
            // Read filename
            if ( Sound_pHS->pFileRead(fh, Sound_sndFilename, filenameLen) != filenameLen )
            {
                return 0;
            }

            uint32_t bankNum;
            if ( Sound_pHS->pFileRead(fh, &bankNum, sizeof(bankNum)) != sizeof(bankNum) )
            {
                return 0;
            }

            // Read sound index
            uint32_t sndIdx;
            if ( Sound_pHS->pFileRead(fh, &sndIdx, sizeof(sndIdx)) != sizeof(sndIdx) )
            {
                return 0;
            }

            // Read sound handle
            if ( Sound_pHS->pFileRead(fh, &Sound_handleEntropy, sizeof(uint32_t)) != sizeof(uint32_t) )
            {
                return 0;
            }

            // Fixed: Calculate correct entropy from sound info handle
            Sound_handleEntropy = Sound_GetEntropyFromSoundHandle(Sound_handleEntropy);
            if ( maxEntropy < Sound_handleEntropy ) // Added
            {
                maxEntropy = Sound_handleEntropy;
            }

            tSoundHandle hSnd = SOUND_INVALIDHANDLE;
            if ( Sound_state == SOUNDSTATE_OPEN )
            {
                if ( bankNum == SOUNDBANK_STATIC_NUM )
                {
                    hSnd = Sound_LoadStatic(Sound_sndFilename, &sndIdx);
                }
                else
                {
                    hSnd = Sound_Load(Sound_sndFilename, &sndIdx);
                }
            }

            // Read channel handle
            if ( Sound_pHS->pFileRead(fh, &Sound_handleEntropy, sizeof(uint32_t)) != sizeof(uint32_t) )
            {
                return 0;
            }

            // Fixed: Calculate correct entropy from channel handle
            Sound_handleEntropy = Sound_GetEntropyFromChannelHandle(Sound_handleEntropy);
            if ( maxEntropy < Sound_handleEntropy ) // Added
            {
                maxEntropy = Sound_handleEntropy;
            }

            int32_t priority;
            if ( Sound_pHS->pFileRead(fh, &priority, sizeof(priority)) != sizeof(priority) )
            {
                return 0;
            }

            int32_t thingId;
            if ( Sound_pHS->pFileRead(fh, &thingId, sizeof(thingId)) != sizeof(thingId) )
            {
                return 0;
            }

            float posX;
            if ( Sound_pHS->pFileRead(fh, &posX, sizeof(posX)) != sizeof(posX) )
            {
                return 0;
            }

            float posY;
            if ( Sound_pHS->pFileRead(fh, &posY, sizeof(posY)) != sizeof(posY) )
            {
                return 0;
            }

            float posZ;
            if ( Sound_pHS->pFileRead(fh, &posZ, sizeof(posY)) != sizeof(posY) )
            {
                return 0;
            }

            float volume;
            if ( Sound_pHS->pFileRead(fh, &volume, sizeof(volume)) != sizeof(volume) )
            {
                return 0;
            }

            float pitch;
            if ( Sound_pHS->pFileRead(fh, &pitch, sizeof(pitch)) != sizeof(pitch) )
            {
                return 0;
            }

            float sampleRate;
            if ( Sound_pHS->pFileRead(fh, &sampleRate, sizeof(sampleRate)) != sizeof(sampleRate) )
            {
                return 0;
            }

            tSoundChannelFlag chflags;
            if ( Sound_pHS->pFileRead(fh, &chflags, sizeof(int32_t)) != sizeof(int32_t) )
            {
                return 0;
            }

            int32_t guid;
            if ( Sound_pHS->pFileRead(fh, &guid, sizeof(guid)) != sizeof(guid) )
            {
                return 0;
            }

            float minRadius;
            if ( Sound_pHS->pFileRead(fh, &minRadius, sizeof(minRadius)) != sizeof(minRadius) )
            {
                return 0;
            }

            float maxRadius;
            if ( Sound_pHS->pFileRead(fh, &maxRadius, sizeof(maxRadius)) != sizeof(maxRadius) )
            {
                return 0;
            }

            SoundEnvFlags envflags;
            if ( Sound_pHS->pFileRead(fh, &envflags, sizeof(int32_t)) != sizeof(int32_t) )
            {
                return 0;
            }

            if ( Sound_state == SOUNDSTATE_OPEN )
            {
                // Fixed: Mark far sound as loop play sound so it's pushed to the sound channel list. Otherwise, it won't be played.
            #ifdef J3D_QOL_IMPROVEMENTS
                if ( (chflags & SOUND_CHANNEL_FAR) != 0 )
                {
                    chflags |= (SOUND_CHANNEL_LOOP | SOUND_CHANNEL_PLAYING);
                }
            #endif

                if ( (chflags & (SOUND_CHANNEL_THING | SOUND_CHANNEL_3DSOUND)) == (SOUND_CHANNEL_THING | SOUND_CHANNEL_3DSOUND) )
                {
                    tSoundChannelHandle hChannel = Sound_PlayPos(hSnd, volume, priority, chflags, posX, posY, posZ, guid, minRadius, maxRadius, envflags);
                    tSoundChannel* pChannel = Sound_GetChannel(hChannel);
                    if ( pChannel )
                    {
                        pChannel->thingId = thingId;
                        pChannel->flags |= SOUND_CHANNEL_THING | SOUND_CHANNEL_3DSOUND;
                    }
                }
                else if ( (chflags & SOUND_CHANNEL_3DSOUND) != 0 )
                {
                    chflags |= SOUND_CHANNEL_LOOP | SOUND_CHANNEL_PLAYING;
                    Sound_PlayPos(hSnd, volume, priority, chflags, posX, posY, posZ, guid, minRadius, maxRadius, envflags);
                }
            }
        }

        // Read next sound filename
        if ( Sound_pHS->pFileRead(fh, &filenameLen, sizeof(filenameLen)) != sizeof(filenameLen) )
        {
            return 0;
        }
    }

    // Read handle entropy and fades.
    if ( Sound_pHS->pFileRead(fh, &Sound_handleEntropy, sizeof(uint32_t)) != sizeof(uint32_t) )
    {
        return 0;
    }

    // Fixed: Added check to ensure Sound_handleEntropy is never smaller than the max entropy value.
    //        This prevents possible sound handle collisions when loading or playing new sounds.
    //        Especially the case where playing sounds get switched due to handle collisions,
    //        e.g.: When firing a gun and due to handle collision the sound of colliding handle is played instead of the gun sound.
    //              And when the colliding sound is looping, it'll be play indefinitely at player's position.
    if ( Sound_handleEntropy < maxEntropy )
    {
        SOUNDLOG_WARNING("Sound_Restore: Restored handle entropy #%d is smaller than max handle entropy #%d. Set to max handle entropy.\n", Sound_handleEntropy, maxEntropy);
        Sound_handleEntropy = maxEntropy;
    }

    // Read fades
    // Note, what's the point to read fades to system in case system is not opened?
    return Sound_pHS->pFileRead(fh, Sound_aFades, sizeof(Sound_aFades)) == sizeof(Sound_aFades);
}

void Sound_Pause(void)
{
    if ( Sound_state != SOUNDSTATE_OPEN )
    {
        SOUNDLOG_ERROR("Sound_Pause: module is not open.\n");
        return;
    }

    if ( Sound_pausedRefCount == 0 )
    {
        // Pause all channels

        Sound_msecPauseStartTime = SoundDriver_GetTimeMsec();

        for ( size_t i = Sound_numChannels; i > 0; i-- )
        {
            tSoundChannel* pChannel = &Sound_apChannels[i - 1];

            if ( pChannel->handle != SOUND_INVALIDHANDLE )
            {
                if ( (pChannel->flags & SOUND_CHANNEL_FAR) == 0 )
                {
                    pChannel->flags |= SOUND_CHANNEL_PAUSED;
                    SoundDriver_Stop(pChannel->pDSoundBuffer);
                }
            }
        }
    }

    // Increment pause reference count
    ++Sound_pausedRefCount;
}

void Sound_Resume(void)
{
    if ( Sound_state != SOUNDSTATE_OPEN )
    {
        SOUNDLOG_ERROR("Sound_Resume: module is not open.\n");
        return;
    }

    if ( Sound_pausedRefCount == 0 )
    {
        return;
    }

    if ( --Sound_pausedRefCount <= 0 )
    {
        Sound_msecElapsed += SoundDriver_GetTimeMsec() - Sound_msecPauseStartTime;

        // Resume all paused channels
        for ( size_t i = Sound_numChannels; i > 0; i-- )
        {
            tSoundChannel* pChannel = &Sound_apChannels[i - 1];
            if ( pChannel->handle != SOUND_INVALIDHANDLE )
            {
                if ( (pChannel->flags & SOUND_CHANNEL_PAUSED) != 0 )
                {
                    pChannel->flags &= ~SOUND_CHANNEL_PAUSED;
                    SoundDriver_Play(
                        pChannel->pDSoundBuffer,
                        (pChannel->flags & (SOUND_CHANNEL_LOOP | SOUND_CHANNEL_PLAYING)) == (SOUND_CHANNEL_LOOP | SOUND_CHANNEL_PLAYING)
                    );
                }
            }
        }
    }
}

void J3DAPI Sound_Set3DGlobals(float distanceFactor, float minDistance, float maxDistance, float folloverFactor, float dopplerFactor)
{
    if ( Sound_state != SOUNDSTATE_OPEN )
    {
        SOUNDLOG_ERROR("Sound_Set3DGlobals: module is not open.\n");
        return;
    }

    SoundDriver_SetGlobals(distanceFactor, minDistance, maxDistance, folloverFactor, dopplerFactor);
}

int Sound_Has3DHW(void)
{
    if ( Sound_state == SOUNDSTATE_OPEN )
    {
        return SoundDriver_Has3DHW();
    }

    SOUNDLOG_ERROR("Sound_Has3DHW: module is not open.\n");
    return 0;
}

void J3DAPI Sound_Set3DHWState(int bNo3DSound)
{
    if ( Sound_state != SOUNDSTATE_OPEN )
    {
        SOUNDLOG_ERROR("Sound_Set3DHWState: module is not open.\n");
        return;
    }

    if ( Sound_Has3DHW() && (!bNo3DSound || Sound_bNoSound3D) && (bNo3DSound || !Sound_bNoSound3D) )
    {
        // Store original file functions
        tFileReadFunc pFileRead   = Sound_pHS->pFileRead;
        tFileWriteFunc pFileWrite = Sound_pHS->pFileWrite;
        tFileSeekFunc pFileSeek   = Sound_pHS->pFileSeek;

        // Toggle 3D sound state
        Sound_bNoSound3D ^= 1u;

        // Set write functions to memory file
        Sound_pHS->pFileWrite = Sound_MemFileWrite;

        // Save current sound state to memory file
        tFileHandle fh = Sound_MemFileReset();
        Sound_Save(fh);

        // Restore original file functions
        Sound_pHS->pFileWrite = pFileWrite;

        // Reopen sound driver
        Sound_StopAllSounds();
        SoundDriver_Close();

        SoundDriver_Open(
            Sound_bNoSound3D,
            Sound_bGlobalFocusBuf,
            Sound_hwnd,
            Sound_pDirectSound,
            Sound_pfCalcListenerSoundMix,
            Sound_GetSoundBufferData,
            Sound_pHS->pErrorPrint
        );

        // Set read functions to memory file
        Sound_pHS->pFileRead = Sound_MemFileRead;
        Sound_pHS->pFileSeek = Sound_MemFileSeek;

        // Restore sound state from memory file
        fh = Sound_MemFileReset();
        Sound_Restore(fh);

        // Restore original file functions
        Sound_pHS->pFileRead = pFileRead;
        Sound_pHS->pFileSeek = pFileSeek;
        Sound_MemFileFree();
    }
}

int Sound_Get3DHWState(void)
{
    if ( Sound_state != SOUNDSTATE_OPEN )
    {
        SOUNDLOG_ERROR("Sound_Get3DHWState: module is not open.\n");
        return 0;
    }

    return Sound_Has3DHW() && Sound_bNoSound3D == 0;
}

size_t Sound_GetMemoryUsage(void)
{
    return (Sound_sizeChannels * sizeof(tSoundChannel))
        + sizeof(SoundInfo) * (soundbank_aSizeSounds[SOUNDBANK_NORMAL_NUM] + soundbank_aSizeSounds[SOUNDBANK_STATIC_NUM])
        + soundbank_aCacheSizes[SOUNDBANK_NORMAL_NUM]
        + soundbank_aCacheSizes[SOUNDBANK_STATIC_NUM];
}

tSoundHandle J3DAPI Sound_Load(const char* pFilepath, uint32_t* sndIdx)
{
    // Set bank number
    size_t bankNum = SOUNDBANK_NORMAL_NUM;
    if ( sndIdx && *sndIdx == SOUND_LOADSTATICIDXMASK )
    {
        sndIdx = (uint32_t*)sndIdx[1]; // The actual index is stored in the second parameter
        bankNum = SOUNDBANK_STATIC_NUM;
    }

    // Extract filename from file path
    size_t curPathPos     = strlen(pFilepath);
    const char* pFilename = &pFilepath[curPathPos];
    do
    {
        --pFilename;
        --curPathPos;
    } while ( curPathPos && *pFilename != '/' && *pFilename != '\\' );

    // Skip '/' or '\'
    ++pFilename;

    // Search for sound in the soundbank
    for ( size_t i = 0; i < SOUNDBANK_NUMBANKS; ++i )
    {
        if ( !soundbank_apSoundCache[i] )
        {
            continue;
        }

        for ( size_t j = soundbank_aNumSounds[i]; j > 0; --j )
        {
            SoundInfo* pSndInfo = &soundbank_apSoundInfos[i][j - 1];
            if ( streqi(pFilename, (const char*)&soundbank_apSoundCache[i][pSndInfo->nameOffset]) )
            {
                // Found sound in the bank, return it's handle and index
                if ( sndIdx )
                {
                    *sndIdx = pSndInfo->idx;
                }
                return pSndInfo->hSnd;
            }
        }
    }

    // Ok not found, let's load it

    if ( !Sound_bLoadEnabled )
    {
        SOUNDLOG_ERROR("Sound_Load: Can't load %s because loads are not currently allowed.\n", pFilepath);
        return SOUND_INVALIDHANDLE;
    }

    tFileHandle fh = Sound_pHS->pFileOpen(pFilepath, "rb");
    if ( !fh )
    {
        SOUNDLOG_ERROR("Sound_Load: Can't load %s.\n", pFilepath);
        return SOUND_INVALIDHANDLE;
    }

    // Read WAV header into buffer
    size_t nRead = Sound_pHS->pFileRead(fh, Sound_loadBuffer, STD_ARRAYLEN(Sound_loadBuffer));
    if ( (int)nRead < 50 ) // 50 - is WAV header size?
    {
        SOUNDLOG_ERROR("Sound_Load: Can't load %s because loads are not currently allowed.\n", pFilepath);
        Sound_pHS->pFileClose(fh); // Fixed: Close fh
        return SOUND_INVALIDHANDLE;
    }

    // Allocate sound info array if not already allocated or if we need to increase the size
    while ( soundbank_aNumSounds[bankNum] >= soundbank_aSizeSounds[bankNum] )
    {
        if ( soundbank_aSizeSounds[bankNum] )
        {
            soundbank_aSizeSounds[bankNum] *= 2; // double size
        }
        else
        {
            soundbank_aSizeSounds[bankNum] = SOUND_INIT_SOUNDINFO_BUFFERSIZE;
        }

        SoundInfo* aSndInfos = (SoundInfo*)Sound_pHS->pRealloc(soundbank_apSoundInfos[bankNum], sizeof(SoundInfo) * soundbank_aSizeSounds[bankNum]);
        if ( !aSndInfos )
        {
            SOUNDLOG_ERROR("Sound_Load: Couldn't realloc %d bytes.", sizeof(SoundInfo) * soundbank_aSizeSounds[bankNum]);
            Sound_Reset(1);

            Sound_pHS->pFileClose(fh); // Fixed: Close fh
            return SOUND_INVALIDHANDLE;
        }

        soundbank_apSoundInfos[bankNum] = aSndInfos;
    }

    // Parse WAV header
    int wavType;
    uint32_t extraDataOffset, soundDataOffset;
    SoundInfo* pSndInfo = &soundbank_apSoundInfos[bankNum][soundbank_aNumSounds[bankNum]];

    const uint8_t* pSoundData = AudioLib_ParseWaveFileHeader(
        Sound_loadBuffer,
        &wavType,
        &pSndInfo->sampleRate,
        &pSndInfo->sempleBitSize,
        &pSndInfo->numChannels,
        NULL,
        &pSndInfo->dataSize,
        &extraDataOffset,
        &soundDataOffset
    );

    if ( wavType != 3 && wavType != 6 && wavType != 5 ) // 6  - IndyWV,  3 - WAV, 5 - FORM AIFFCOM
    {
        // Unsupported WAV type
        SOUNDLOG_ERROR("Sound_Load: %s is not a WAV or WV file.\n", pFilepath);
        Sound_pHS->pFileClose(fh);
        return SOUND_INVALIDHANDLE;
    }

    // Set last imported bank number.. TODO: This is kind of hacky shouldn't we set it to soundbank_lastImportedBankNum = bankNum?
    if ( soundbank_lastImportedBankNum == SOUNDBANK_STATIC_NUM )
    {
        soundbank_lastImportedBankNum = SOUNDBANK_NORMAL_NUM;
    }

    // Assign sound info
    pSndInfo->hSnd           = Sound_GenerateSoundHandle();
    pSndInfo->bankNum        = bankNum;
    pSndInfo->filePathOffset = Sound_WriteSoundFilepathToBank(pSndInfo->bankNum, pFilepath, strlen(pFilepath) + 1); // + 1 is terminating zero

    // Check if we have enough space in the sound cache to write the sound data
    if ( !pSndInfo->filePathOffset )
    {
        Sound_pHS->pFileClose(fh);
        return SOUND_INVALIDHANDLE;
    }

    // Assign name offset & compressed flag
    pSndInfo->nameOffset  = pFilename - pFilepath + pSndInfo->filePathOffset;
    pSndInfo->bCompressed = wavType == 6; // 6 - IndyWV

    // Move file pointer to sound data offset
    Sound_pHS->pFileSeek(fh, pSoundData - Sound_loadBuffer, 0);

    if ( pSndInfo->bCompressed || Sound_bNoSoundCompression )
    {
        // Compressed Sound data read

        pSndInfo->dataOffset = Sound_GetFreeCache(pSndInfo->bankNum, pSndInfo->dataSize);
        if ( !pSndInfo->dataOffset )
        {
            Sound_pHS->pFileClose(fh);
            return SOUND_INVALIDHANDLE;
        }

        nRead = Sound_pHS->pFileRead(fh, &soundbank_apSoundCache[pSndInfo->bankNum][pSndInfo->dataOffset], pSndInfo->dataSize);
        if ( nRead != pSndInfo->dataSize )
        {
            SOUNDLOG_ERROR("Sound_Load: Failed to read sound data from %s.\n", pFilepath); // Added: Log read error
            Sound_pHS->pFileClose(fh); // Fixed: Close fh
            return SOUND_INVALIDHANDLE;
        }

        // We're done close file and do final load post process
        Sound_pHS->pFileClose(fh);
        pSndInfo->pLipSyncData = NULL;

        if ( sndIdx )
        {
            pSndInfo->idx = *sndIdx;
        }
        else
        {
            pSndInfo->idx = SOUND_NOSOUNDINDEX; // default idx as sound idx was not provided
        }

        ++soundbank_aNumSounds[bankNum];
        return pSndInfo->hSnd;
    }
    else
    {
        // Uncompressed sound data read

        size_t dataSize = pSndInfo->dataSize;
        pSndInfo->dataOffset =  Sound_GetFreeCache(pSndInfo->bankNum, 2 * dataSize); // Reserve space for original data size + compressed data size
        if ( !pSndInfo->dataOffset )
        {
            Sound_pHS->pFileClose(fh);
            return SOUND_INVALIDHANDLE;
        }

        // Read uncompressed data
        nRead = Sound_pHS->pFileRead(fh, &soundbank_apSoundCache[pSndInfo->bankNum][pSndInfo->dataOffset + dataSize], pSndInfo->dataSize);
        if ( nRead != pSndInfo->dataSize )
        {
            SOUNDLOG_ERROR("Sound_Load: Failed to read sound data from %s.\n", pFilepath); // Added: Log read error
            Sound_pHS->pFileClose(fh); // Fixed: Close fh
            return SOUND_INVALIDHANDLE;
        }

        // Compress read data
        tAudioCompressorState compressor;
        AudioLib_ResetCompressor(&compressor);
        size_t compressedSize = AudioLib_Compress(
            &compressor,
            ((tAudioCompressedData*)&soundbank_apSoundCache[pSndInfo->bankNum][pSndInfo->dataOffset])->compressedData,
            &soundbank_apSoundCache[pSndInfo->bankNum][pSndInfo->dataOffset + dataSize],
            pSndInfo->dataSize,
            pSndInfo->numChannels
        ) + sizeof(uint32_t);     //  sizeof((uint32_t) is size of tAudioCompressedData::uncompressedSize

        // Write original data size before compressed data to the beginning of the sound data in cache
        //*(uint32_t*)&soundbank_apSoundCache[pSndInfo->bankNum][pSndInfo->dataOffset] = pSndInfo->dataSize;
        ((tAudioCompressedData*)&soundbank_apSoundCache[pSndInfo->bankNum][pSndInfo->dataOffset])->uncompressedSize = pSndInfo->dataSize;

        // Now assign compressed data size
        pSndInfo->dataSize = compressedSize;

        // Increase free cache size to size of data size
        Sound_IncreaseFreeCache(pSndInfo->bankNum, dataSize + pSndInfo->dataSize - compressedSize);
        pSndInfo->bCompressed = 1;

         // We're done close file and do final load post process
        Sound_pHS->pFileClose(fh);
        pSndInfo->pLipSyncData = NULL;

        if ( sndIdx )
        {
            pSndInfo->idx = *sndIdx;
        }
        else
        {
            pSndInfo->idx = SOUND_NOSOUNDINDEX; // default idx as sound idx was not provided
        }

        ++soundbank_aNumSounds[bankNum];
        return pSndInfo->hSnd;
    }
}

tSoundHandle J3DAPI Sound_LoadStatic(const char* pFilename, uint32_t* sndIdx)
{
    uint32_t aSndIdx[2];
    aSndIdx[0] = SOUND_LOADSTATICIDXMASK;
    aSndIdx[1] = (uint32_t)sndIdx;

    tSoundHandle hSnd = Sound_Load(pFilename, aSndIdx);
    soundbank_lastImportedBankNum = SOUNDBANK_STATIC_NUM;
    return hSnd;
}

void J3DAPI Sound_Reset(int bResetAllBanks)
{
    if ( bResetAllBanks )
    {
        Sound_StopAllSounds();

        for ( size_t bankNum = 0; bankNum < SOUNDBANK_NUMBANKS; ++bankNum )
        {
            if ( soundbank_apSoundInfos[bankNum] )
            {
                for ( size_t i = 0; i < soundbank_aNumSounds[bankNum]; ++i )
                {
                    if ( soundbank_apSoundInfos[bankNum][i].pLipSyncData )
                    {
                        Sound_pHS->pFree(soundbank_apSoundInfos[bankNum][i].pLipSyncData);
                    }
                }

                Sound_pHS->pFree(soundbank_apSoundInfos[bankNum]);
                soundbank_apSoundInfos[bankNum] = NULL;
            }

            soundbank_aSizeSounds[bankNum] = 0;
            soundbank_aNumSounds[bankNum]  = 0;

            if ( soundbank_apSoundCache[bankNum] )
            {
                Sound_pHS->pFree(soundbank_apSoundCache[bankNum]);
                soundbank_apSoundCache[bankNum] = NULL;
            }

            soundbank_aCacheSizes[bankNum]     = 0;
            soundbank_aUsedCacheSizes[bankNum] = 0;
        }

        Sound_handleEntropy = 0;
    }
    else
    {
        Sound_StopAllNonStaticSounds();

        if ( soundbank_apSoundInfos[SOUNDBANK_NORMAL_NUM] )
        {
            for ( size_t i = 0; i < soundbank_aNumSounds[SOUNDBANK_NORMAL_NUM]; ++i )
            {
                if ( soundbank_apSoundInfos[SOUNDBANK_NORMAL_NUM][i].pLipSyncData )
                {
                    Sound_pHS->pFree(soundbank_apSoundInfos[SOUNDBANK_NORMAL_NUM][i].pLipSyncData);
                }
            }

            Sound_pHS->pFree(soundbank_apSoundInfos[SOUNDBANK_NORMAL_NUM]);
            soundbank_apSoundInfos[SOUNDBANK_NORMAL_NUM] = NULL;
        }

        soundbank_aSizeSounds[SOUNDBANK_NORMAL_NUM] = 0;
        soundbank_aNumSounds[SOUNDBANK_NORMAL_NUM]  = 0;

        if ( soundbank_apSoundCache[SOUNDBANK_NORMAL_NUM] )
        {
            Sound_pHS->pFree(soundbank_apSoundCache[SOUNDBANK_NORMAL_NUM]);
            soundbank_apSoundCache[SOUNDBANK_NORMAL_NUM] = NULL;
        }

        soundbank_aCacheSizes[SOUNDBANK_NORMAL_NUM]     = 0;
        soundbank_aUsedCacheSizes[SOUNDBANK_NORMAL_NUM] = 0;
    }

    soundbank_lastImportedBankNum = SOUNDBANK_STATIC_NUM;
    Sound_bLoadEnabled = 1;
}

void J3DAPI Sound_EnableLoad(int bEnable)
{
    Sound_bLoadEnabled = bEnable;
}

size_t J3DAPI Sound_ExportStaticBank(tFileHandle fh)
{
    return Sound_ExportBank(fh, SOUNDBANK_STATIC_NUM);
}

int J3DAPI Sound_ImportStaticBank(tFileHandle fh)
{
    return Sound_ImportBank(fh, SOUNDBANK_STATIC_NUM);
}

size_t J3DAPI Sound_ExportNormalBank(tFileHandle fh)
{
    return Sound_ExportBank(fh, SOUNDBANK_NORMAL_NUM);
}

int J3DAPI Sound_ImportNormalBank(tFileHandle fh)
{
    return Sound_ImportBank(fh, SOUNDBANK_NORMAL_NUM);
}

int J3DAPI Sound_SkipSoundFileSection(tFileHandle fh)
{
    uint32_t numSounds;
    if ( Sound_pHS->pFileRead(fh, &numSounds, sizeof(uint32_t)) != sizeof(uint32_t) )
    {
        return 0;
    }

    uint32_t sizeData;
    if ( Sound_pHS->pFileRead(fh, &sizeData, sizeof(uint32_t)) != sizeof(uint32_t) )
    {
        return 0;
    }

    Sound_pHS->pFileSeek(fh, sizeof(SoundInfo) * numSounds, 1); // skip sound info list
    Sound_pHS->pFileSeek(fh, sizeData, 1);         // skip sound data list
    Sound_pHS->pFileSeek(fh, sizeof(uint32_t), 1); // skip next handle
    return 1;
}

tSoundHandle J3DAPI Sound_GetSoundHandle(uint32_t idx)
{
    for ( size_t bankNum = 0; bankNum < SOUNDBANK_NUMBANKS; ++bankNum )
    {
        size_t numInfos = soundbank_aNumSounds[bankNum];
        for ( size_t i = numInfos; i > 0; i-- )
        {
            SoundInfo* pSndInfo = &soundbank_apSoundInfos[bankNum][i - 1];
            if ( pSndInfo->hSnd && pSndInfo->idx == idx )
            {
                return pSndInfo->hSnd;
            }
        }
    }

    return SOUND_INVALIDHANDLE;
}

size_t J3DAPI Sound_GetSoundIndex(tSoundHandle hSnd)
{
    for ( size_t bankNum = 0; bankNum < SOUNDBANK_NUMBANKS; ++bankNum )
    {
        size_t numInfos = soundbank_aNumSounds[bankNum];
        for ( size_t i = numInfos; i > 0; i-- )
        {
            SoundInfo* pSndInfo = &soundbank_apSoundInfos[bankNum][i - 1];
            if ( pSndInfo->hSnd == hSnd )
            {
                return pSndInfo->idx;
            }
        }
    }

    return SOUND_INVALISOUNDINFOINDEX;
}

tSoundChannelHandle J3DAPI Sound_GetChannelHandle(int guid)
{
    for ( size_t i = Sound_numChannels; i > 0; i-- )
    {
        tSoundChannel* pChannel = &Sound_apChannels[i - 1];
        if ( pChannel->handle != SOUND_INVALIDHANDLE && pChannel->guid == guid )
        {
            return pChannel->handle;
        }
    }

    return SOUND_INVALIDHANDLE;
}

int J3DAPI Sound_GetChannelGUID(tSoundChannelHandle hChannel)
{
    if ( !hChannel )
    {
        return SOUND_INVALIDGUID;
    }

    for ( size_t i = Sound_numChannels; i > 0; i-- )
    {
        tSoundChannel* pChannel = &Sound_apChannels[i - 1];
        if ( pChannel->handle == hChannel )
        {
            return pChannel->guid;
        }
    }

    return SOUND_INVALIDGUID;
}

void J3DAPI Sound_SetChannelGUID(tSoundChannelHandle hChannel, int guid)
{
    if ( hChannel )
    {
        // Iterate through channels to find matching handle and update GUID
        for ( size_t i = Sound_numChannels; i > 0; i-- )
        {
            tSoundChannel* pChannel = &Sound_apChannels[i - 1];
            if ( pChannel->handle == hChannel )
            {
                pChannel->guid = guid;
                return;
            }
        }
    }
}

const char* J3DAPI Sound_GetSoundFilename(tSoundHandleType handle)
{
    SoundInfo* pSndInfo;
    tSoundChannel* pChannel = Sound_GetChannel(handle);
    if ( pChannel )
    {
        pSndInfo = Sound_GetSoundInfo(pChannel->hSnd);
    }
    else
    {
        pSndInfo = Sound_GetSoundInfo(handle);
    }

    if ( pSndInfo )
    {
        return (const char*)&soundbank_apSoundCache[pSndInfo->bankNum][pSndInfo->nameOffset];
    }

    return NULL;
}

size_t J3DAPI Sound_GetLengthMsec(tSoundHandleType handle)
{
    SoundInfo* pSndInfo;
    tSoundChannel* pChannel = Sound_GetChannel(handle);
    if ( pChannel )
    {
        pSndInfo = Sound_GetSoundInfo(pChannel->hSnd);
    }
    else
    {
        pSndInfo = Sound_GetSoundInfo(handle);
    }

    if ( !pSndInfo )
    {
        return 0;
    }

    uint32_t dataSize = pSndInfo->dataSize;
    if ( pSndInfo->bCompressed )
    {
        dataSize = ((tAudioCompressedData*)&soundbank_apSoundCache[pSndInfo->bankNum][pSndInfo->dataOffset])->uncompressedSize; // decompressed size is stored in the first 4 bytes of the compressed data
    }

    if ( pSndInfo->sampleRate && pSndInfo->sempleBitSize >= 8 && pSndInfo->numChannels )
    {
        return 1000 * dataSize / (pSndInfo->numChannels * (pSndInfo->sempleBitSize >> 3)) / pSndInfo->sampleRate;
    }

    SOUNDLOG_ERROR("Sound_GetLengthMsec: Bad header data for handle: %p (rate:%d bits:%d chans:%d)\n", handle, pSndInfo->sampleRate, pSndInfo->sempleBitSize, pSndInfo->numChannels);
    return 0;
}

void J3DAPI Sound_SetMaxVolume(float maxVolume)
{
    maxVolume = STDMATH_CLAMP(maxVolume, 0.0f, 1.0f);
    if ( maxVolume != Sound_maxVolume )
    {
        Sound_maxVolume = maxVolume;
        SoundDriver_SetMaxVolume(maxVolume);

        for ( size_t i = Sound_numChannels; i > 0; i-- )
        {
            tSoundChannel* pChannel = &Sound_apChannels[i - 1];
            if ( pChannel->handle != SOUND_INVALIDHANDLE )
            {
                float newVolume = pChannel->volume * Sound_maxVolume;
                SoundDriver_SetVolume(pChannel->pDSoundBuffer, newVolume);
            }
        }
    }
}

float Sound_GetMaxVolume(void)
{
    return Sound_maxVolume;
}

tSoundChannelHandle J3DAPI Sound_Play(tSoundHandle hSnd, float volume, float pan, int priority, tSoundChannelFlag flags, int guid)
{
    if ( Sound_state != SOUNDSTATE_OPEN
        || Sound_pausedRefCount && (flags & (SOUND_CHANNEL_LOOP | SOUND_CHANNEL_PLAYING)) == (SOUND_CHANNEL_LOOP | SOUND_CHANNEL_PLAYING) )
    {
        return SOUND_INVALIDHANDLE;
    }

    if ( !SOUND_IS_SOUNDINFOHANDLE(hSnd) )
    {
        return SOUND_INVALIDHANDLE;
    }

    if ( SoundDriver_Use3DCaps() && (flags & SOUND_CHANNEL_3DSOUND) == 0 )
    {
        flags |= SOUND_CHANNEL_USE3DCAPS;
    }

    SoundInfo* pSndInfo = Sound_GetSoundInfo(hSnd);
    if ( !pSndInfo )
    {
        return SOUND_INVALIDHANDLE;
    }

    if ( (flags & SOUND_CHANNEL_PLAYONCE) != 0 && Sound_GetChannelBySoundHandle(hSnd) )
    {
        return SOUND_INVALIDHANDLE;
    }

    // Allocate sound channel list if not already allocated
    if ( !Sound_apChannels )
    {
        Sound_apChannels = (tSoundChannel*)Sound_pHS->pMalloc(Sound_maxChannels * sizeof(tSoundChannel));
        if ( !Sound_apChannels )
        {
            return SOUND_INVALIDHANDLE;
        }

        memset(Sound_apChannels, 0, Sound_maxChannels * sizeof(tSoundChannel));
        Sound_sizeChannels = Sound_maxChannels;
        Sound_numChannels  = Sound_maxChannels;
    }

    // First scan for a free channel while tracking lowest priority
    tSoundChannel* pChannel       = Sound_apChannels;
    tSoundChannel* pLowPriChannel = NULL;
    for ( size_t i = 0; i < Sound_numChannels; ++i )
    {
         // Track lowest priority channel seen so far
        if ( pChannel->priority < priority )
        {
            pLowPriChannel = pChannel;
        }

        // Found a free channel - use it immediately
        if ( pChannel->handle == SOUND_INVALIDHANDLE )
        {
            break;
        }

        ++pChannel;
    }

    // If we couldn't find a free channel, try to reuse lowest priority one
    if ( pChannel->handle != SOUND_INVALIDHANDLE )
    {
        // No free channel found, check if we have a lower priority one to reuse
        if ( !pLowPriChannel )
        {
            return SOUND_INVALIDHANDLE;
        }

        Sound_StopChannel(pLowPriChannel->handle);
        pChannel = pLowPriChannel;
    }

    tSoundChannel* pExistingSndChannel = Sound_GetChannelBySoundHandle(hSnd);
    if ( pExistingSndChannel && (pExistingSndChannel->flags & SOUND_CHANNEL_GLOBALFOCUS) != 0 )
    {
        flags |= SOUND_CHANNEL_GLOBALFOCUS;
    }

    if ( pSndInfo->bCompressed )
    {
        flags |= SOUND_CHANNEL_COMPRESSED;
    }

    pChannel->hSnd       = hSnd;
    pChannel->thingId    = 0;
    pChannel->priority   = priority;
    pChannel->volume     = volume;
    pChannel->pitch      = 1.0f;
    pChannel->sampleRate = (float)pSndInfo->sampleRate;
    pChannel->flags      = flags;
    pChannel->guid       = guid;
    pChannel->minRadius  = 0.0f;
    pChannel->maxRadius  = 0.0f;
    pChannel->envflags   = 0;

    // If start far playing then don't play sound
    if ( (flags & SOUND_CHANNEL_STARTFAR) != 0 )
    {
        pChannel->flags &= ~SOUND_CHANNEL_STARTFAR;
        pChannel->flags |= SOUND_CHANNEL_FAR;
        pChannel->pDSoundBuffer = NULL;
    }
    else
    {
        // Create sound buffer and play it
        size_t nRetries = 0;
        do
        {
            // Try to create and play buffer, else remove low priority channel and re-try, max SOUND_MAXCREATERETRIES times
            pChannel->pDSoundBuffer = SoundDriver_CreateAndPlay(pSndInfo->sampleRate, pSndInfo->sempleBitSize, pSndInfo->numChannels, &soundbank_apSoundCache[pSndInfo->bankNum][pSndInfo->dataOffset], pSndInfo->dataSize, &pChannel->flags, NULL);
            if ( !pChannel->pDSoundBuffer )
            {
                tSoundChannel* pChannelToRemove = NULL;
                int curPriority = priority;

                for ( size_t i = Sound_numChannels; i > 0; i-- )
                {
                    tSoundChannel* pCurChannel = &Sound_apChannels[i - 1];
                    if ( pCurChannel->handle != SOUND_INVALIDHANDLE )
                    {
                        if ( pCurChannel->priority < curPriority )
                        {
                            curPriority      = pCurChannel->priority;
                            pChannelToRemove = pCurChannel;
                        }
                    }
                }

                if ( pChannelToRemove )
                {
                    Sound_Release(pChannelToRemove);
                    ++nRetries;
                }
                else
                {
                    // No channels to remove, so just break out of the loop
                    nRetries = SOUND_MAXCREATERETRIES;
                }
            }
        } while ( !pChannel->pDSoundBuffer && nRetries < SOUND_MAXCREATERETRIES );

        // If we still don't have a buffer, give up
        if ( nRetries >= SOUND_MAXCREATERETRIES )
        {
            // Added: Log error message
            SOUNDLOG_ERROR("Sound_Play: Couldn't create sound buffer after %d retries.\n", SOUND_MAXCREATERETRIES);

            // Couldn't create sound buffer
            pChannel->handle  = SOUND_INVALIDHANDLE;
            pChannel->hSnd    = SOUND_INVALIDHANDLE;
            pChannel->thingId = 0;
            return SOUND_INVALIDHANDLE;
        }

        // We have a buffer, set the volume and pan
        float newVolume = volume * Sound_maxVolume;
        SoundDriver_SetVolume(pChannel->pDSoundBuffer, newVolume);
        SoundDriver_SetPan(pChannel->pDSoundBuffer, pan);
    }

    // Set the sound handle
    pChannel->handle = Sound_GenerateChannelHandle();
    return pChannel->handle;
}

tSoundChannelHandle J3DAPI Sound_PlayPos(tSoundHandle hSnd, float volume, int priority, tSoundChannelFlag channelflags, float x, float y, float z, int guid, float minRadius, float maxRadius, SoundEnvFlags envflags)
{
    if ( Sound_state != SOUNDSTATE_OPEN || Sound_pausedRefCount )
    {
        return SOUND_INVALIDHANDLE;
    }

    tSoundChannelFlag newFlags = 0;
    if ( !SoundDriver_Update3DSound(NULL, x, y, z, volume, 1.0f, &newFlags, minRadius, maxRadius, envflags) )
    {
        if ( (channelflags & (SOUND_CHANNEL_LOOP | SOUND_CHANNEL_PLAYING)) != (SOUND_CHANNEL_LOOP | SOUND_CHANNEL_PLAYING) )
        {
            return SOUND_INVALIDHANDLE;
        }

        channelflags |= SOUND_CHANNEL_STARTFAR;
    }

    if ( SoundDriver_Use3DCaps()
        && (channelflags & (SOUND_CHANNEL_LOOP | SOUND_CHANNEL_PLAYING)) != (SOUND_CHANNEL_LOOP | SOUND_CHANNEL_PLAYING)
        && (Sound_curUpdatePos.x - x) * (Sound_curUpdatePos.x - x)
        + (Sound_curUpdatePos.y - y) * (Sound_curUpdatePos.y - y)
        + (Sound_curUpdatePos.z - z) * (Sound_curUpdatePos.z - z) <= 0.25f )
    {
        channelflags |= SOUND_CHANNEL_USE3DCAPS;
    }

    tSoundChannelHandle hChannel = Sound_Play(hSnd, volume, 0.0f, priority, (tSoundChannelFlag)(channelflags | SOUND_CHANNEL_3DSOUND), guid);
    tSoundChannel* pChannel      = Sound_GetChannel(hChannel);
    if ( !hChannel )
    {
        return SOUND_INVALIDHANDLE;
    }

    SoundDriver_SetPosAndVelocity(pChannel->pDSoundBuffer, x, y, z, 0.0f, 0.0f, 0.0f, minRadius, maxRadius);

    pChannel->playPos.x = x;
    pChannel->playPos.y = y;
    pChannel->playPos.z = z;
    pChannel->envflags  = envflags;
    pChannel->minRadius = minRadius;
    pChannel->maxRadius = maxRadius;

    if ( !SoundDriver_Update3DSound(&pChannel->pDSoundBuffer, x, y, z, volume, 1.0f, &pChannel->flags, minRadius, maxRadius, pChannel->envflags) )
    {
        Sound_StopChannel(hChannel);
    }

    return hChannel;
}

tSoundChannelHandle J3DAPI Sound_PlayThing(tSoundHandle hSnd, float volume, int priority, tSoundChannelFlag flags, int thingID, int guid, float minRadius, float maxRadius)
{
    if ( Sound_state != SOUNDSTATE_OPEN || Sound_pausedRefCount )
    {
        return SOUND_INVALIDHANDLE;
    }

    if ( !SOUND_IS_SOUNDINFOHANDLE(hSnd) )
    {
        return SOUND_INVALIDHANDLE;
    }

    SoundThingInfo thingInfo = { 0 };
    if ( Sound_pfGetThingInfoCallback )
    {
        Sound_pfGetThingInfoCallback(thingID, &thingInfo);
    }

    // Check if playonce flag is set and if so, check if the sound is already playing
    if ( (flags & SOUND_CHANNEL_PLAYTHINGONCE) != 0 )
    {
        for ( size_t i = Sound_numChannels; i > 0; i-- )
        {
            tSoundChannel* pChannel = &Sound_apChannels[i - 1];
            if ( pChannel->hSnd == hSnd && pChannel->thingId == thingID )
            {
                return SOUND_INVALIDHANDLE;
            }
        }
    }

    tSoundChannelFlag newFlags = 0;
    if ( !SoundDriver_Update3DSound(NULL, thingInfo.pos.x, thingInfo.pos.y, thingInfo.pos.z, volume, 1.0f, &newFlags, minRadius, maxRadius, thingInfo.envflags) )
    {
        if ( (flags & (SOUND_CHANNEL_LOOP | SOUND_CHANNEL_PLAYING)) != (SOUND_CHANNEL_LOOP | SOUND_CHANNEL_PLAYING) )
        {
            return SOUND_INVALIDHANDLE;
        }

        flags |= SOUND_CHANNEL_STARTFAR;
    }

    if ( SoundDriver_Use3DCaps() )
    {
        if ( (Sound_curUpdatePos.x - thingInfo.pos.x) * (Sound_curUpdatePos.x - thingInfo.pos.x)
            + (Sound_curUpdatePos.y - thingInfo.pos.y) * (Sound_curUpdatePos.y - thingInfo.pos.y)
            + (Sound_curUpdatePos.z - thingInfo.pos.z) * (Sound_curUpdatePos.z - thingInfo.pos.z) <= 0.25f )
        {
            flags |= SOUND_CHANNEL_USE3DCAPS;
            if ( (flags & (SOUND_CHANNEL_LOOP | SOUND_CHANNEL_PLAYING)) == (SOUND_CHANNEL_LOOP | SOUND_CHANNEL_PLAYING) )
            {
                flags |= SOUND_CHANNEL_DISABLEHWMIXING;
            }
        }
    }

    tSoundHandle hChannel = Sound_Play(hSnd, volume, 0.0f, priority, (tSoundChannelFlag)(flags | SOUND_CHANNEL_THING | SOUND_CHANNEL_3DSOUND), guid);
    if ( !hChannel )
    {
        return SOUND_INVALIDHANDLE;
    }

    tSoundChannel* pChannel = Sound_GetChannel(hChannel);
    pChannel->thingId       = thingID;
    pChannel->minRadius     = minRadius;
    pChannel->maxRadius     = maxRadius;
    pChannel->playPos       = thingInfo.pos;
    pChannel->envflags      = thingInfo.envflags;

    SoundDriver_SetPosAndVelocity(pChannel->pDSoundBuffer, thingInfo.pos.x, thingInfo.pos.y, thingInfo.pos.z, thingInfo.velocity.x, thingInfo.velocity.y, thingInfo.velocity.z, minRadius, maxRadius);

    if ( !SoundDriver_Update3DSound(&pChannel->pDSoundBuffer, thingInfo.pos.x, thingInfo.pos.y, thingInfo.pos.z, volume, 1.0f, &pChannel->flags, minRadius, maxRadius, pChannel->envflags) )
    {
        Sound_StopChannel(hChannel);
    }

    return hChannel;
}

void J3DAPI Sound_StopChannel(tSoundChannelHandle hChannel)
{
    if ( Sound_apChannels && hChannel )
    {
        for ( size_t i = 0; i < Sound_numChannels; ++i )
        {
            tSoundChannel* pChannel = &Sound_apChannels[i];
            if ( pChannel->handle == hChannel )
            {
                Sound_Release(pChannel);
                return;
            }
        }
    }
}

void Sound_StopAllSounds(void)
{
    if ( Sound_apChannels )
    {
        for ( size_t i = 0; i < Sound_numChannels; ++i )
        {
            tSoundChannel* pChannel = &Sound_apChannels[i];
            if ( pChannel->handle != SOUND_INVALIDHANDLE )
            {
                Sound_Release(pChannel);
            }
        }
    }
}

void J3DAPI Sound_StopThing(int thingId, tSoundHandleType handle)
{
    if ( SOUND_IS_CHANNELHANDLE(handle) )
    {
        Sound_StopChannel(handle);
        return;
    }

    if ( handle != SOUND_ALLTHINGSOUNDHANDLE && !SOUND_IS_SOUNDINFOHANDLE(handle) )
    {
        SOUNDLOG_ERROR("Sound_StopThing: Don't know what 'sound' is: %p\n", handle);
        return;
    }

    // Iterate channels in reverse order and stop all channels with specific sound handle and thing id
    for ( size_t i = Sound_numChannels; i > 0; i-- )
    {
        tSoundChannel* pChannel = &Sound_apChannels[i - 1];
        if ( pChannel->handle != SOUND_INVALIDHANDLE
            && (pChannel->flags & (SOUND_CHANNEL_THING | SOUND_CHANNEL_3DSOUND)) != 0
            && pChannel->thingId == thingId
            && (handle == SOUND_ALLTHINGSOUNDHANDLE || pChannel->hSnd == handle) )
        {
            Sound_Release(pChannel);
        }
    }
}

void J3DAPI Sound_SetVolume(tSoundChannelHandle hChannel, float volume)
{
    tSoundChannel* pChannel = Sound_GetChannel(hChannel);
    if ( pChannel )
    {
        if ( volume <= 0.0f )
        {
            Sound_StopChannel(hChannel);
        }
        else
        {
            pChannel->volume = volume;
            if ( (pChannel->flags & (SOUND_CHANNEL_3DSOUND | SOUND_CHANNEL_HWMIXING)) != SOUND_CHANNEL_3DSOUND )
            {
                float newVolume = volume * Sound_maxVolume;
                SoundDriver_SetVolume(pChannel->pDSoundBuffer, newVolume);
            }
        }
    }
}

float J3DAPI Sound_GetVolume(tSoundHandle hSnd)
{
    tSoundChannel* pChannel = Sound_GetChannel(hSnd);
    if ( pChannel )
    {
        return SoundDriver_GetVolume(pChannel->pDSoundBuffer);
    }

    return 0.0f;
}

void J3DAPI Sound_FadeVolume(tSoundChannelHandle hChannel, float volume, float secFadeTime)
{
    tSoundChannel* pChannel = Sound_GetChannel(hChannel);
    if ( !pChannel )
    {
        return;
    }

    uint32_t curTime      = Sound_GetDeltaTime();
    uint32_t msecFadeTime = (uint32_t)(secFadeTime * 1000.0f);
    for ( size_t i = STD_ARRAYLEN(Sound_aFades); i > 0; i-- )
    {
        SoundFade* pFade = &Sound_aFades[i - 1];
        if ( !pFade->hChannel )
        {
            pFade->hChannel      = hChannel;
            pFade->bPitch        = 0;
            pFade->msecStartTime = curTime;
            pFade->msecEndTime   = msecFadeTime + curTime;
            pFade->startValue    = SoundDriver_GetVolume(pChannel->pDSoundBuffer);
            pFade->endValue      = volume;

            pChannel->flags |= SOUND_CHANNEL_VOLUMEFADE;
            return;
        }
    }

    // No free fades, so find the one with the longest time remaining
    SoundFade* pFade = NULL;
    uint32_t nRemaining = 999999999;
    for ( size_t i = STD_ARRAYLEN(Sound_aFades); i > 0; i-- )
    {
        SoundFade* pCurFade = &Sound_aFades[i - 1];
        if ( !pCurFade->hChannel )
        {
            pFade = pCurFade;
            break;
        }

        if ( (pCurFade->msecEndTime - curTime) > nRemaining )
        {
            nRemaining = pCurFade->msecEndTime - curTime;
            pFade = pCurFade;
        }
    }

    if ( !pFade )
    {
        SOUNDLOG_ERROR("Sound_FadeVolume: No free fades!\n");
        return;
    }

    // Stop the fade on the channel that was fading
    tSoundChannel* pStopChannel = Sound_GetChannel(pFade->hChannel);
    pStopChannel->flags &= ~SOUND_CHANNEL_VOLUMEFADE;
    Sound_SetVolume(pFade->hChannel, pFade->endValue);

    // Set up the new fade
    pFade->hChannel      = hChannel;
    pFade->bPitch        = 0;
    pFade->msecStartTime = curTime;
    pFade->msecEndTime   = msecFadeTime + curTime;
    pFade->startValue    = SoundDriver_GetVolume(pChannel->pDSoundBuffer);
    pFade->endValue      = volume;

    pChannel->flags |= SOUND_CHANNEL_VOLUMEFADE;
}

void J3DAPI Sound_SetVolumeThing(int thingId, tSoundHandleType handle, float volume)
{
    if ( SOUND_IS_CHANNELHANDLE(handle) )
    {
        Sound_SetVolume(handle, volume);
        return;
    }

    if ( handle != SOUND_ALLTHINGSOUNDHANDLE && !SOUND_IS_SOUNDINFOHANDLE(handle) )
    {
        SOUNDLOG_ERROR("Sound_SetVolumeThing: Don't know what 'sound' is: %p\n", handle);
        return;
    }

    // Iterate channels in reverse order to apply new volume
    for ( size_t i = Sound_numChannels; i > 0; i-- )
    {
        tSoundChannel* pChannel = &Sound_apChannels[i - 1];
        if ( pChannel->handle != SOUND_INVALIDHANDLE
            && (pChannel->flags & (SOUND_CHANNEL_THING | SOUND_CHANNEL_3DSOUND)) != 0
            && pChannel->thingId == thingId
            && (handle == SOUND_ALLTHINGSOUNDHANDLE || pChannel->hSnd == handle) )
        {
            Sound_SetVolume(pChannel->handle, volume);
        }
    }
}

void J3DAPI Sound_FadeVolumeThing(int thingId, tSoundHandleType handle, float startVolume, float endVolume)
{
    if ( SOUND_IS_CHANNELHANDLE(handle) )
    {
        Sound_FadeVolume(handle, startVolume, endVolume);
        return;
    }

    if ( handle != SOUND_ALLTHINGSOUNDHANDLE && !SOUND_IS_SOUNDINFOHANDLE(handle) )
    {
        SOUNDLOG_ERROR("Sound_FadeVolumeThing: Don't know what 'sound' is: %p\n", handle);
        return;
    }

    // Iterate channels in reverse order to apply volume fade
    for ( size_t i = Sound_numChannels; i > 0; i-- )
    {
        tSoundChannel* pChannel = &Sound_apChannels[i - 1];
        if ( pChannel->handle != SOUND_INVALIDHANDLE
            && (pChannel->flags & (SOUND_CHANNEL_THING | SOUND_CHANNEL_3DSOUND)) != 0
            && pChannel->thingId == thingId
            && (handle == SOUND_ALLTHINGSOUNDHANDLE || pChannel->hSnd == handle) )
        {
            Sound_FadeVolume(pChannel->handle, startVolume, endVolume);
        }
    }
}

int J3DAPI Sound_IsThingFadingVol(int thingId, tSoundHandleType handle)
{
    if ( SOUND_IS_CHANNELHANDLE(handle) )
    {
        return (Sound_GetChannelFlags(handle) & SOUND_CHANNEL_VOLUMEFADE) != 0;
    }

    if ( handle != SOUND_ALLTHINGSOUNDHANDLE && !SOUND_IS_SOUNDINFOHANDLE(handle) )
    {
        SOUNDLOG_ERROR("Sound_IsThingFadingVol: Don't know what 'sound' is: %p\n", handle);
        return 0;
    }

    // Iterate through channels in reverse order to find matching sound with volume fade
    for ( size_t i = Sound_numChannels; i > 0; i-- )
    {
        tSoundChannel* pChannel = &Sound_apChannels[i - 1];
        if ( pChannel->handle != SOUND_INVALIDHANDLE
            && (pChannel->flags & (SOUND_CHANNEL_THING | SOUND_CHANNEL_3DSOUND)) != 0
            && pChannel->thingId == thingId
            && (handle == SOUND_ALLTHINGSOUNDHANDLE || pChannel->hSnd == handle)
            && (pChannel->flags & SOUND_CHANNEL_VOLUMEFADE) != 0 )
        {
            return 1;
        }
    }

    return 0;
}

void J3DAPI Sound_SetPan(tSoundChannelHandle hChannel, float volume)
{
    tSoundChannel* pChannel = Sound_GetChannel(hChannel);
    if ( pChannel )
    {
        SoundDriver_SetPan(pChannel->pDSoundBuffer, volume);
    }
}

float J3DAPI Sound_GetPan(tSoundChannelHandle hChannel)
{
    tSoundChannel* pChannel = Sound_GetChannel(hChannel);
    if ( pChannel )
    {
        return SoundDriver_GetPan(pChannel->pDSoundBuffer);
    }

    return 0.0f;
}

void J3DAPI Sound_SetPitch(tSoundChannelHandle hChannel, float pitch)
{
    tSoundChannel* pChannel = Sound_GetChannel(hChannel);
    if ( pChannel )
    {
        pChannel->pitch = pitch;
        float freq = pitch * pChannel->sampleRate;
        SoundDriver_SetFrequency(pChannel->pDSoundBuffer, freq);
    }
}

float J3DAPI Sound_GetPitch(tSoundChannelHandle hChannel)
{
    tSoundChannel* pChannel = Sound_GetChannel(hChannel);
    if ( pChannel )
    {
        return pChannel->pitch;
    }

    return 0.0f;
}

void J3DAPI Sound_FadePitch(tSoundChannelHandle hChannel, float pitch, float secFadeTime)
{
    tSoundChannel* pChannel = Sound_GetChannel(hChannel);
    if ( !pChannel )
    {
        return;
    }

    uint32_t curTime      = Sound_GetDeltaTime();
    uint32_t msecFadeTime = (uint32_t)(secFadeTime * 1000.0f);
    for ( size_t i = STD_ARRAYLEN(Sound_aFades); i > 0; i-- )
    {
        SoundFade* pFade = &Sound_aFades[i - 1];
        if ( !pFade->hChannel )
        {
            pFade->hChannel      = hChannel;
            pFade->bPitch        = 1;
            pFade->msecStartTime = curTime;
            pFade->msecEndTime   = msecFadeTime + curTime;
            pFade->startValue    = Sound_GetPitch(hChannel);
            pFade->endValue      = pitch;

            pChannel->flags |= SOUND_CHANNEL_PITCHFADE;
            return;
        }
    }

    // No free fades, so find the one with the longest time remaining
    SoundFade* pFade = NULL;
    uint32_t nRemaining = 999999999;
    for ( size_t i = STD_ARRAYLEN(Sound_aFades); i > 0; i-- )
    {
        SoundFade* pCurFade = &Sound_aFades[i - 1];
        if ( !pCurFade->hChannel )
        {
            pFade = pCurFade;
            break;
        }

        if ( (pCurFade->msecEndTime - curTime) > nRemaining )
        {
            nRemaining = pCurFade->msecEndTime - curTime;
            pFade = pCurFade;
        }
    }

    if ( !pFade )
    {
        SOUNDLOG_ERROR("Sound_FadeVolume: No free fades!\n");
        return;
    }

    // Stop the fade on the channel that was fading
    tSoundChannel* pStopChannel = Sound_GetChannel(pFade->hChannel);
    pStopChannel->flags &= ~SOUND_CHANNEL_PITCHFADE;
    Sound_SetPitch(pFade->hChannel, pFade->endValue);

    // Set up the new fade
    pFade->hChannel      = hChannel;
    pFade->bPitch        = 1;
    pFade->msecStartTime = curTime;
    pFade->msecEndTime   = msecFadeTime + curTime;
    pFade->startValue    = Sound_GetPitch(hChannel);
    pFade->endValue      = pitch;

    pChannel->flags |= SOUND_CHANNEL_PITCHFADE;
}

void J3DAPI Sound_SetPitchThing(int thingId, tSoundHandleType handle, float pitch)
{
    if ( SOUND_IS_CHANNELHANDLE(handle) )
    {
        Sound_SetPitch(handle, pitch);
        return;
    }

    if ( handle != SOUND_ALLTHINGSOUNDHANDLE && !SOUND_IS_SOUNDINFOHANDLE(handle) )
    {
        SOUNDLOG_ERROR("Sound_SetPitchThing: Don't know what 'sound' is: %p\n", handle);
        return;
    }

    // Iterate through channels in reverse order
    for ( size_t i = Sound_numChannels; i > 0; i-- )
    {
        tSoundChannel* pChannel = &Sound_apChannels[i - 1];
        if ( pChannel->handle != SOUND_INVALIDHANDLE
            && (pChannel->flags & (SOUND_CHANNEL_THING | SOUND_CHANNEL_3DSOUND)) != 0
            && pChannel->thingId == thingId
            && (handle == SOUND_ALLTHINGSOUNDHANDLE || pChannel->hSnd == handle) )
        {
            Sound_SetPitch(pChannel->handle, pitch);
        }
    }
}

bool J3DAPI Sound_IsThingFadingPitch(int thingId, tSoundHandleType handle)
{
    if ( SOUND_IS_CHANNELHANDLE(handle) )
    {
        return (Sound_GetChannelFlags(handle) & SOUND_CHANNEL_PITCHFADE) != 0;
    }

    if ( handle != SOUND_ALLTHINGSOUNDHANDLE && !SOUND_IS_SOUNDINFOHANDLE(handle) )
    {
        SOUNDLOG_ERROR("Sound_IsThingFadingPitch: Don't know what 'sound' is: %p\n", handle);
        return false;
    }

    // Look through all channels for matching thingId/handle with pitch fade
    for ( size_t i = Sound_numChannels; i > 0; i-- )
    {
        tSoundChannel* pChannel = &Sound_apChannels[i - 1];
        if ( pChannel->handle != SOUND_INVALIDHANDLE
            && (pChannel->flags & (SOUND_CHANNEL_THING | SOUND_CHANNEL_3DSOUND)) != 0
            && pChannel->thingId == thingId
            && (handle == SOUND_ALLTHINGSOUNDHANDLE || pChannel->hSnd == handle)
            && (pChannel->flags & SOUND_CHANNEL_PITCHFADE) != 0 )
        {
            return true;
        }
    }

    return false;
}

tSoundChannelFlag J3DAPI Sound_GetChannelFlags(tSoundChannelHandle hChannel)
{
    tSoundChannel* pChannel = Sound_GetChannel(hChannel);
    if ( pChannel )
    {
        return pChannel->flags;
    }

    return 0;
}

bool J3DAPI Sound_GetChannelPlayProgress(tSoundChannelHandle hChannel, float* pProgress)
{
    SOUND_ASSERT(pProgress);

    tSoundChannel* pChannel = Sound_GetChannel(hChannel);
    if ( !pChannel || !pChannel->pDSoundBuffer )
    {
        *pProgress = 0.0;
        return false;
    }

    if ( (pChannel->flags & SOUND_CHANNEL_PLAYING) == 0 )
    {
        *pProgress = 0.0;
        return false;
    }

    SoundInfo* pSndInfo = Sound_GetSoundInfo(pChannel->hSnd);
    if ( !pSndInfo )
    {
        *pProgress = 0.0;
        return false;
    }

    size_t dataSize = pSndInfo->dataSize;
    if ( pSndInfo->bCompressed )
    {
        dataSize = ((tAudioCompressedData*)&soundbank_apSoundCache[pSndInfo->bankNum][pSndInfo->dataOffset])->uncompressedSize;
    }

    size_t curPos = SoundDriver_GetCurrentPosition(pChannel->pDSoundBuffer);
    *pProgress = (float)curPos / (float)dataSize * 100.0f;
    return true;
}

static void Sound_UpdateFades(void) // Added
{
    for ( size_t i = 0; i < STD_ARRAYLEN(Sound_aFades); i++ )
    {
        SoundFade* pFade = &Sound_aFades[i];
        if ( Sound_aFades[i].hChannel )
        {
            int dtime = Sound_GetDeltaTime();
            tSoundChannel* pChannel = Sound_GetChannel(pFade->hChannel);
            if ( !pChannel )
            {
                pFade->hChannel = SOUND_INVALIDHANDLE;
                continue;
            }

            if ( dtime < (int)pFade->msecEndTime )
            {
                double newValue = (pFade->endValue - pFade->startValue)
                    * (double)(dtime - pFade->msecStartTime)
                    / (double)(pFade->msecEndTime - pFade->msecStartTime)
                    + pFade->startValue;

                if ( newValue != 0.0 )
                {
                    if ( pFade->bPitch )
                    {
                        Sound_SetPitch(pFade->hChannel, (float)newValue);
                    }
                    else
                    {
                        Sound_SetVolume(pFade->hChannel, (float)newValue);
                    }

                    if ( (float)newValue == pFade->endValue )
                    {
                        pFade->hChannel = SOUND_INVALIDHANDLE;
                    }
                }
            }
            else
            {
                if ( pFade->bPitch )
                {
                    Sound_SetPitch(pFade->hChannel, pFade->endValue);
                    pChannel->flags &= ~SOUND_CHANNEL_PITCHFADE;
                }
                else if ( pFade->endValue == 0.0f )
                {
                    Sound_StopChannel(pFade->hChannel);
                }
                else
                {
                    Sound_SetVolume(pFade->hChannel, pFade->endValue);
                    pChannel->flags &= ~SOUND_CHANNEL_VOLUMEFADE;
                }

                pFade->hChannel = SOUND_INVALIDHANDLE;
            }
        }
    }
}

void J3DAPI Sound_Update(const rdVector3* pPos, const rdVector3* pVelocity, const rdVector3* pTopOrient, const rdVector3* pFrontOrient)
{
    // Was changed to function call
    Sound_UpdateFades();

    // Fixed: Fixed negating orient by not modifying the original pointer var
    rdVector3 topOrient;
    if ( pTopOrient && !Sound_bReverseSound )
    {
        rdVector_Neg3(&topOrient, pTopOrient);
        pTopOrient = &topOrient;
    }

    // Fixed: Fixed negating orient by not modifying the original pointer var
    rdVector3 frontOrient;
    if ( pFrontOrient && !Sound_bReverseSound )
    {
        rdVector_Neg3(&frontOrient, pFrontOrient);
        pFrontOrient = &frontOrient;
    }

    if ( pPos )
    {
        Sound_curUpdatePos = *pPos;
    }

    SoundDriver_SetListenerPosition(pPos, pVelocity, pTopOrient, pFrontOrient);

    int count = Sound_numChannels;
    tSoundChannel* pCurChannel = &Sound_apChannels[Sound_numChannels];

    while ( 1 )
    {
        --pCurChannel;
        if ( !count-- )
        {
            break;
        }

        if ( pCurChannel->handle != SOUND_INVALIDHANDLE && (pCurChannel->flags & SOUND_CHANNEL_3DSOUND) != 0 )
        {
            SoundThingInfo thngSndInfo;
            if ( (pCurChannel->flags & (SOUND_CHANNEL_THING | SOUND_CHANNEL_3DSOUND)) != 0
                && Sound_pfGetThingInfoCallback
                && Sound_pfGetThingInfoCallback(pCurChannel->thingId, &thngSndInfo) )
            {
                pCurChannel->playPos  = thngSndInfo.pos;
                pCurChannel->envflags = thngSndInfo.envflags;

                SoundDriver_SetPosAndVelocity(
                    pCurChannel->pDSoundBuffer,
                    thngSndInfo.pos.x,
                    thngSndInfo.pos.y,
                    thngSndInfo.pos.z,
                    thngSndInfo.velocity.x,
                    thngSndInfo.velocity.y,
                    thngSndInfo.velocity.z,
                    pCurChannel->minRadius,
                    pCurChannel->maxRadius
                );
            }

            if ( !SoundDriver_Update3DSound(
                &pCurChannel->pDSoundBuffer,
                pCurChannel->playPos.x,
                pCurChannel->playPos.y,
                pCurChannel->playPos.z,
                pCurChannel->volume,
                pCurChannel->pitch,
                &pCurChannel->flags,
                pCurChannel->minRadius,
                pCurChannel->maxRadius,
                pCurChannel->envflags) )
            {
                Sound_StopChannel(pCurChannel->handle); // Fixed: Fixed bug to use correct channel handle; OG used count var
            }
            else if ( (pCurChannel->flags & SOUND_CHANNEL_RESTART) != 0 )
            {
                pCurChannel->flags |= SOUND_CHANNEL_LOOP | SOUND_CHANNEL_PLAYING;
                pCurChannel->flags &= ~SOUND_CHANNEL_FAR;

                if ( (pCurChannel->flags & (SOUND_CHANNEL_THING | SOUND_CHANNEL_3DSOUND)) == (SOUND_CHANNEL_THING | SOUND_CHANNEL_3DSOUND) )
                {
                    tSoundChannel channel = *pCurChannel;
                    pCurChannel->handle   = SOUND_INVALIDHANDLE;
                    pCurChannel->hSnd     = SOUND_INVALIDHANDLE;
                    pCurChannel->thingId  = 0;
                    if ( pCurChannel->pDSoundBuffer )
                    {
                        SoundDriver_Release(pCurChannel->pDSoundBuffer);
                        pCurChannel->pDSoundBuffer = NULL;
                    }

                    tSoundChannelHandle hChannel = Sound_PlayThing(channel.hSnd, channel.volume, channel.priority, channel.flags, channel.thingId, channel.guid, channel.minRadius, channel.maxRadius);
                    if ( !hChannel )
                    {
                        SOUNDLOG_ERROR("Sound_Update: Error restarting distant sound.\n");
                    }
                    else
                    {
                        for ( size_t i = Sound_numChannels; i > 0; --i )
                        {
                            tSoundChannel* pChannel = &Sound_apChannels[i - 1];
                            if ( pChannel->handle == hChannel )
                            {
                                pChannel->flags &= ~SOUND_CHANNEL_RESTART;
                                pChannel->handle = channel.handle;
                                break; // Should continue the while loop
                            }
                        }
                    }
                }
                else
                {
                    tSoundChannel channel = *pCurChannel;
                    pCurChannel->handle = SOUND_INVALIDHANDLE;
                    if ( pCurChannel->pDSoundBuffer )
                    {
                        SoundDriver_Release(pCurChannel->pDSoundBuffer);
                        pCurChannel->pDSoundBuffer = NULL;
                    }

                    tSoundChannelHandle hChannel = Sound_PlayPos(
                        channel.hSnd,
                        channel.volume,
                        channel.priority,
                        channel.flags,
                        channel.playPos.x,
                        channel.playPos.y,
                        channel.playPos.z,
                        channel.guid,
                        channel.minRadius,
                        channel.maxRadius,
                        channel.envflags
                    );

                    if ( !hChannel )
                    {
                        SOUNDLOG_ERROR("Sound_Update: Error restarting distant sound.\n");
                    }
                    else
                    {
                        for ( size_t i = Sound_numChannels; i > 0; --i )
                        {
                            tSoundChannel* pChannel = &Sound_apChannels[i - 1];
                            if ( pChannel->handle == hChannel )
                            {
                                pChannel->flags &= ~SOUND_CHANNEL_RESTART;
                                pChannel->handle = channel.handle;
                                break; // Should continue the while loop
                            }
                        }
                    }
                }
            }
        }
    }

    SoundDriver_ListenerCommitDeferred();

    // Remove channels that have finished playing
    if ( Sound_apChannels )
    {
        for ( size_t i = 0; i < Sound_numChannels; ++i )
        {
            pCurChannel = &Sound_apChannels[i];
            if ( pCurChannel->handle != SOUND_INVALIDHANDLE
                && (SoundDriver_GetStatusAndCaps(pCurChannel->pDSoundBuffer) & SOUND_CHANNEL_PLAYING) == 0
                && (pCurChannel->flags & SOUND_CHANNEL_PAUSED) == 0
                && (pCurChannel->flags & SOUND_CHANNEL_FAR) == 0 )
            {
                Sound_Release(pCurChannel);
            }
        }
    }
}

// Added: Build and commit cached lip-sync data only after every allocation and generation step succeeds.
static int Sound_CacheLipSyncData(SoundInfo* pSndInfo, uint8_t* pSoundCache, size_t soundCacheCapacity, const char* pFilename)
{
    const tAudioCompressedData* pCompressedData = NULL;
    const uint8_t* pSndData                     = NULL;

    uint8_t* pAllocatedSndData = NULL;
    uint32_t sndDataSize;

    if ( pSndInfo->bCompressed )
    {
        pCompressedData = (const tAudioCompressedData*)&pSoundCache[pSndInfo->dataOffset];
        sndDataSize      = pCompressedData->uncompressedSize;
    }
    else
    {
        sndDataSize = pSndInfo->dataSize;
        pSndData    = &pSoundCache[pSndInfo->dataOffset];
    }

    size_t lipSyncCapacity = AudioLib_GetLipSyncBlockMaxSize(
        SOUND_LIPSYNC_ANALYSIS_RATE_HZ,
        pSndInfo->sampleRate,
        pSndInfo->sempleBitSize,
        pSndInfo->numChannels,
        sndDataSize
    );
    if ( lipSyncCapacity == 0 )
    {
        SOUNDLOG_ERROR("Sound_GenerateLipSync: Unsupported lip-sync generation parameters for %s\n", pFilename);
        return 0;
    }

    size_t sndDataCapacity;
    if ( pSndInfo->bCompressed )
    {
        // Fixed: The original allocated only sndDataSize bytes although AudioLib starts reading at pSndData + 1.
        //        A 16-bit read can reach one byte past the data, or two when an analysis frame has an odd byte length.
        sndDataCapacity = (size_t)sndDataSize + sizeof(int16_t);

        // Fixed: Check the temporary decompression allocation before resetting or using the output buffer.
        pAllocatedSndData = (uint8_t*)Sound_pHS->pMalloc(sndDataCapacity);
        if ( !pAllocatedSndData )
        {
            SOUNDLOG_ERROR("Sound_GenerateLipSync: Couldn't allocate decompression buffer for %s\n", pFilename);
            return 0;
        }

        // Fixed: Initialize the decompression buffer so a short decode cannot feed indeterminate bytes to lip-sync generation.
        STD_ZEROMEM(pAllocatedSndData, sndDataCapacity);

        tAudioCompressorState compressor;
        Sound_ResetLipSyncCompressor(&compressor);
        Sound_UncompressLipSyncData(&compressor, pAllocatedSndData, pCompressedData->compressedData, (unsigned int)sndDataSize);
        pSndData = pAllocatedSndData;
    }
    else
    {
        sndDataCapacity = soundCacheCapacity - pSndInfo->dataOffset;
    }

    // Fixed: Replace the original fixed 8192-byte stack scratch buffer with a checked worst-case allocation.
    //        The 8-byte header, one 4-byte change entry per 60 Hz frame, and one terminal entry limited
    //        the old buffer to 2045 changing frames, or about 34.1 seconds in the worst case. Audio with
    //        repeated mouth states could be longer, but highly variable audio could overrun the unchecked
    //        buffer. The stack scratch avoided a temporary output allocation and allowed an exact-sized
    //        persistent copy after successful generation; this path allocates the checked capacity directly.
    uint8_t* pGeneratedData = (uint8_t*)Sound_pHS->pMalloc(lipSyncCapacity);
    if ( !pGeneratedData )
    {
        SOUNDLOG_ERROR("Sound_GenerateLipSync: Couldn't allocate lip-sync data for %s\n", pFilename);
        if ( pAllocatedSndData )
        {
            Sound_pHS->pFree(pAllocatedSndData);
        }

        return 0;
    }

    int generatedSize = AudioLib_GenerateLipSyncBlockEx(
        pGeneratedData,
        lipSyncCapacity,
        pSndData,
        sndDataCapacity,
        SOUND_LIPSYNC_ANALYSIS_RATE_HZ,
        SOUND_LIPSYNC_MOUTH_LEVELS,
        SOUND_LIPSYNC_MOUTH_LEVELS,
        pSndInfo->sampleRate,
        pSndInfo->sempleBitSize,
        pSndInfo->numChannels,
        /*bCrossSample=*/1,
        sndDataSize
    );

    if ( pAllocatedSndData )
    {
        Sound_pHS->pFree(pAllocatedSndData);
    }

    if ( generatedSize == 0 )
    {
        SOUNDLOG_ERROR("Sound_GenerateLipSync: Failed to generate lip-sync data for %s\n", pFilename);
        Sound_pHS->pFree(pGeneratedData);
        return 0;
    }

    // Fixed: Publish the cache pointer only after a complete block has been generated.
    pSndInfo->pLipSyncData = pGeneratedData;
    return 1;
}

int J3DAPI Sound_GenerateLipSync(tSoundChannelHandle hChannel, uint8_t* pMouthPosX, uint8_t* pMouthPosY, int msecLipSyncOffset)
{
    tSoundChannel* pChannel = Sound_GetChannel(hChannel);
    if ( !pChannel )
    {
        goto error;
    }

    SoundInfo* pSndInfo = Sound_GetSoundInfo(pChannel->hSnd);
    // Fixed: Preserve the original sound/sample-rate checks and match the generator's actual 16-bit mono PCM contract.
    if ( !pSndInfo
        || pSndInfo->sampleRate == 0
        || pSndInfo->sempleBitSize != 16
        || pSndInfo->numChannels != 1 )
    {
        goto error;
    }

    // TODO: [BUG] Validate loaded and imported sound-bank metadata before exposing its offsets and sizes to runtime paths.
    uint8_t* pSoundCache = soundbank_apSoundCache[pSndInfo->bankNum];

    if ( !pSndInfo->pLipSyncData )
    {
        if ( Sound_bNoLipSync )
        {
            goto error;
        }

        const char* pFilename       = (const char*)&pSoundCache[pSndInfo->filePathOffset];
        size_t soundCacheCapacity   = soundbank_aCacheSizes[pSndInfo->bankNum];
        SOUNDLOG_DEBUG("Sound_GenerateLipSync: Generating lip sync for file %s\n", pFilename);
        if ( !Sound_CacheLipSyncData(pSndInfo, pSoundCache, soundCacheCapacity, pFilename) )
        {
            goto error;
        }
    }

    size_t dataSize = pSndInfo->dataSize;
    if ( pSndInfo->bCompressed )
    {
        dataSize = ((const tAudioCompressedData*)&pSoundCache[pSndInfo->dataOffset])->uncompressedSize;
    }

    // Fixed: Use wide intermediates for playback-window and millisecond calculations.
    uint64_t bytesPerSecond = (uint64_t)pSndInfo->sampleRate * (pSndInfo->sempleBitSize >> 3) * pSndInfo->numChannels;
    size_t ampSize          = (size_t)(bytesPerSecond * SOUND_LIPSYNC_END_MARGIN_MSEC / SOUND_MILLISECONDS_PER_SECOND);
    size_t curPos           = Sound_GetLipSyncCurrentPosition(pChannel->pDSoundBuffer);

    int msecLipSyncTime = -1;
    // Altered: Express the original end-of-data check without overflowing ampSize + curPos.
    if ( curPos < dataSize && ampSize < dataSize - curPos )
    {
        uint64_t channelMsec = SOUND_MILLISECONDS_PER_SECOND * (uint64_t)curPos / bytesPerSecond;
        int64_t adjustedMsec = (int64_t)msecLipSyncOffset + (int64_t)channelMsec;
        if ( adjustedMsec >= INT_MIN && adjustedMsec <= INT_MAX )
        {
            msecLipSyncTime = (int)adjustedMsec;
        }
    }

    if ( !AudioLib_GetMouthPosition(pSndInfo->pLipSyncData, msecLipSyncTime, pMouthPosX, pMouthPosY) )
    {
        return 1; // success
    }

error:
    if ( pMouthPosX )
    {
        *pMouthPosX = 0;
    }

    if ( pMouthPosY )
    {
        *pMouthPosY = 0;
    }

    return 0;
}

size_t J3DAPI Sound_GetAllInstanceInfo(SoundInstanceInfo* pCurInstance, size_t sizeInstances)
{
    size_t numSounds   = 0;
    size_t numChannels = Sound_numChannels;
    tSoundChannel* pCurChannel = &Sound_apChannels[Sound_numChannels];
    while ( 1 )
    {
        --pCurChannel;
        if ( !numChannels-- )
        {
            break;
        }

        if ( pCurChannel->handle != SOUND_INVALIDHANDLE )
        {
            if ( numSounds >= sizeInstances )
            {
                SOUNDLOG_ERROR("Sound_GetAllInstanceInfo: Return value not big enough!\n");
                return sizeInstances;
            }

            SoundInfo* pSndInfo = Sound_GetSoundInfo(pCurChannel->hSnd);
            if ( pSndInfo )
            {
                pCurInstance->pName = (const char*)&soundbank_apSoundCache[pSndInfo->bankNum][pSndInfo->nameOffset];
            }
            else
            {
                SOUNDLOG_ERROR("Sound_GetAllInstanceInfo: Unknown sound data for playing sound.\n");
                pCurInstance->pName = ">ERR:Unknown<";
            }

            pCurInstance->handle  = pCurChannel->handle;
            pCurInstance->thingId = pCurChannel->thingId;
            pCurInstance->flags   = pCurChannel->flags;
            ++pCurInstance;
            ++numSounds;
        }
    }

    return numSounds;
}

void J3DAPI Sound_SetReverseSound(int bReverse)
{
    Sound_bReverseSound = bReverse;
}

void Sound_SoundDump(void)
{
    // TODO: make this array to the size of maxSoundChannels var
    SoundInstanceInfo aInfos[J3D_QOL_VALUE(128, 32) * 2];

    size_t numSounds = Sound_GetAllInstanceInfo(aInfos, STD_ARRAYLEN(aInfos));
    SOUNDLOG_STATUS("Sound Dump of %d sounds\n", numSounds);

    for ( size_t i = 0; i < numSounds; ++i )
    {
        char aPlayingText[32];
        if ( (aInfos[i].flags & SOUND_CHANNEL_PLAYING) != 0 )
        {
            STD_FORMAT(aPlayingText, "Playing,");
        }
        else
        {
            STD_FORMAT(aPlayingText, "NotPlay,");
        }

        char aPusedText[32];
        if ( (aInfos[i].flags & SOUND_CHANNEL_PAUSED) != 0 )
        {
            STD_FORMAT(aPusedText, "Paused,");
        }
        else
        {
            STD_FORMAT(aPusedText, "Not Paused,");
        }

        char aDistanceText[32];
        if ( (aInfos[i].flags & SOUND_CHANNEL_FAR) != 0 )
        {
            STD_FORMAT(aDistanceText, "Far,");
        }
        else
        {
            STD_FORMAT(aDistanceText, "   ,");
        }

        char aLoopingText[32];
        if ( (aInfos[i].flags & SOUND_CHANNEL_LOOP) != 0 )
        {
            STD_FORMAT(aLoopingText, "Looping,");
        }
        else
        {
            STD_FORMAT(aLoopingText, "       ,");
        }

        char aSpacialText[32];
        if ( (aInfos[i].flags & SOUND_CHANNEL_3DSOUND) != 0 )
        {
            STD_FORMAT(aSpacialText, "3D,");
        }
        else
        {
            STD_FORMAT(aSpacialText, "2D,");
        }

        char aThingText[32];
        if ( (aInfos[i].flags & (SOUND_CHANNEL_THING | SOUND_CHANNEL_3DSOUND)) != 0 )
        {
            STD_FORMAT(aThingText, "Thing,");
        }
        else
        {
            STD_FORMAT(aThingText, "     ,");
        }

        char aVoldFadeText[32];
        if ( (aInfos[i].flags & SOUND_CHANNEL_VOLUMEFADE) != 0 )
        {
            STD_FORMAT(aVoldFadeText, "volFade,");
        }
        else
        {
            STD_FORMAT(aVoldFadeText, "       ,");
        }

        char aPitchText[32];
        if ( (aInfos[i].flags & SOUND_CHANNEL_PITCHFADE) != 0 )
        {
            STD_FORMAT(aPitchText, "pitchFade,");
        }
        else
        {
            STD_FORMAT(aPitchText, "         ,");
        }

        SOUNDLOG_STATUS("%s %d\n \t%s %s %s %s %s %s %s %s\n", aInfos[i].pName, aInfos[i].handle, aPlayingText, aDistanceText, aLoopingText, aSpacialText, aThingText, aVoldFadeText, aPitchText, aPusedText);
    }

    // Print final line break to separate from next log
    SOUNDLOG_STATUS("\n");
}

tSoundHandle Sound_GenerateSoundHandle(void)
{
    uint32_t hNext = Sound_handleEntropy;
    if ( (Sound_handleEntropy & 1) != 0 )
    {
        hNext = (Sound_handleEntropy + 1) % SOUND_HANDLE_ENTROPY_MAX;
    }

    Sound_handleEntropy = (hNext + 1) % SOUND_HANDLE_ENTROPY_MAX;
    return hNext + SOUND_HANDLE_MIN;
}

tSoundChannelHandle Sound_GenerateChannelHandle(void)
{
    uint32_t hNext = Sound_handleEntropy;
    if ( (Sound_handleEntropy & 1) == 0 )
    {
        hNext = (Sound_handleEntropy + 1) % SOUND_HANDLE_ENTROPY_MAX;
    }

    Sound_handleEntropy = (hNext + 1) % SOUND_HANDLE_ENTROPY_MAX;
    return hNext + SOUND_HANDLE_MIN;
}

uint32_t Sound_GetEntropyFromHandle(tSoundHandleType handle, bool bChannelHandle)
{
    // Subtract SOUND_HANDLE_MIN to get hNext
    uint32_t hNext = handle - SOUND_HANDLE_MIN;

    if ( bChannelHandle )
    {
        if ( hNext % 2 == 1 )
        {
            // If hNext is odd, Sound_handleEntropy was likely odd too
            return hNext;
        }
        else
        {
            // If hNext is even, it might have been (Sound_handleEntropy + 1)
            // We need to subtract 1 and handle wrap-around
            return (hNext > 0) ? hNext - 1 : SOUND_HANDLE_ENTROPY_MAX - 1;
        }
    }
    else
    {
        // Sound info handles
        if ( hNext % 2 == 0 )
        {
            // If hNext is even, Sound_handleEntropy was likely even too
            return hNext;
        }
        else
        {
            // If hNext is odd, it might have been (Sound_handleEntropy + 1)
            // We need to subtract 1 and handle wrap-around
            return (hNext > 0) ? hNext - 1 : SOUND_HANDLE_ENTROPY_MAX - 1;
        }
    }
}

uint32_t Sound_GetEntropyFromSoundHandle(tSoundHandle handle)
{
    return Sound_GetEntropyFromHandle(handle, false);
}

uint32_t Sound_GetEntropyFromChannelHandle(tSoundChannelHandle handle)
{
    return Sound_GetEntropyFromHandle(handle, true);
}

size_t J3DAPI Sound_GetFreeCache(size_t bankNum, size_t requiredSize)
{
    size_t prevCacheSize = soundbank_aUsedCacheSizes[bankNum];
    if ( prevCacheSize == 0 ) // increment size to avoid 0 size for cache alignment
    {
        ++soundbank_aUsedCacheSizes[bankNum];
    }

    while ( (soundbank_aUsedCacheSizes[bankNum] & 3) != 0 )// // align cache to 8 bytes
    {
        ++soundbank_aUsedCacheSizes[bankNum];
    }

    size_t cacheSize = soundbank_aUsedCacheSizes[bankNum];
    soundbank_aUsedCacheSizes[bankNum] = requiredSize + cacheSize;
    if ( soundbank_aUsedCacheSizes[bankNum] < soundbank_aCacheSizes[bankNum] )
    {
        return cacheSize;
    }

    while ( soundbank_aUsedCacheSizes[bankNum] >= soundbank_aCacheSizes[bankNum] )
    {
        soundbank_aCacheSizes[bankNum] += Sound_cacheBlockSize;
    }

    uint8_t* pNewCache = (uint8_t*)Sound_pHS->pRealloc(soundbank_apSoundCache[bankNum], soundbank_aCacheSizes[bankNum]);
    if ( !pNewCache )
    {
        soundbank_aUsedCacheSizes[bankNum] = prevCacheSize;
        soundbank_aCacheSizes[bankNum]     = prevCacheSize;
        SOUNDLOG_ERROR(
            "Sound_Load: Sound cache out of memory! (cacheSize:%d or 0x%08X)\n",
            soundbank_aCacheSizes[bankNum],
            soundbank_aCacheSizes[bankNum]
        );
        return 0;
    }

    soundbank_apSoundCache[bankNum] = pNewCache;
    return cacheSize;
}

void J3DAPI Sound_IncreaseFreeCache(size_t bankNum, size_t size)
{
    if ( size <= soundbank_aUsedCacheSizes[bankNum] )
    {
        soundbank_aUsedCacheSizes[bankNum] -= size;
    }
    else
    {
        soundbank_aUsedCacheSizes[bankNum] = 0;
    }
}

size_t J3DAPI Sound_WriteSoundFilepathToBank(size_t bankNum, const char* pFilepath, size_t len)
{
    size_t writeOffset = Sound_GetFreeCache(bankNum, len);
    if ( writeOffset )
    {
        memcpy(&soundbank_apSoundCache[bankNum][writeOffset], pFilepath, len);
    }

    return writeOffset;
}

uint32_t Sound_GetDeltaTime(void)
{
    return SoundDriver_GetTimeMsec() - Sound_msecElapsed;
}

SoundInfo* J3DAPI Sound_GetSoundInfo(tSoundHandle hSnd)
{
    if ( !SOUND_IS_SOUNDINFOHANDLE(hSnd) )
    {
        return NULL;
    }

    for ( size_t bankNum = 0; bankNum < SOUNDBANK_NUMBANKS; ++bankNum )
    {
        size_t numSounds = soundbank_aNumSounds[bankNum];
        for ( size_t i = numSounds; i > 0; --i )
        {
            SoundInfo* pCurSndInfo = &soundbank_apSoundInfos[bankNum][i - 1];
            if ( pCurSndInfo->hSnd == hSnd )
            {
                return pCurSndInfo;
            }
        }
    }

    return NULL;
}

tSoundChannel* J3DAPI Sound_GetChannel(tSoundChannelHandle hChannel)
{
    if ( !SOUND_IS_CHANNELHANDLE(hChannel) )
    {
        return NULL;
    }

    for ( size_t i = Sound_numChannels; i > 0; --i )
    {
        tSoundChannel* pChannel = &Sound_apChannels[i - 1];
        if ( pChannel->handle == hChannel )
        {
            return pChannel;
        }
    }

    return NULL;
}

tSoundChannel* J3DAPI Sound_GetChannelBySoundHandle(tSoundHandle hSnd)
{
    if ( !SOUND_IS_SOUNDINFOHANDLE(hSnd) )
    {
        return NULL;
    }

    for ( size_t i = Sound_numChannels; i > 0; --i )
    {
        tSoundChannel* pChannel = &Sound_apChannels[i - 1];
        if ( pChannel->handle != SOUND_INVALIDHANDLE && pChannel->hSnd == hSnd )
        {
            return pChannel;
        }
    }

    return NULL;
}

uint8_t* J3DAPI Sound_GetSoundBufferData(tSysSoundBuffer* pSndBuf, size_t* pDataSize, uint32_t* pbCompressed)
{
    SoundInfo* pSoundInfo = NULL;
    for ( size_t i = Sound_numChannels; i > 0; --i )
    {
        tSoundChannel* pChannel = &Sound_apChannels[i - 1];
        if ( pChannel->handle != SOUND_INVALIDHANDLE && pChannel->pDSoundBuffer == pSndBuf )
        {
            pSoundInfo = Sound_GetSoundInfo(pChannel->hSnd);
            if ( pSoundInfo )
            {
                break;
            }
        }
    }

    if ( !pSoundInfo )
    {
        return NULL;
    }

    if ( pDataSize )
    {
        *pDataSize = pSoundInfo->dataSize;
    }

    if ( pbCompressed )
    {
        *pbCompressed = pSoundInfo->bCompressed;
    }

    return &soundbank_apSoundCache[pSoundInfo->bankNum][pSoundInfo->dataOffset];
}

int J3DAPI Sound_MemFileSeek(tFileHandle fh, int offset, int origin)
{
    J3D_UNUSED(fh);
    J3D_UNUSED(offset);
    J3D_UNUSED(origin);
    return 0;
}

size_t J3DAPI Sound_MemFileRead(tFileHandle fh, void* pOutData, size_t nRead)
{
    J3D_UNUSED(fh);

    if ( !Sound_pMemfileBuf || Sound_memfilePos >= Sound_memfileSize )
    {
        return 0;
    }

    if ( nRead + Sound_memfilePos <= Sound_memfileSize )
    {
        memcpy(pOutData, &Sound_pMemfileBuf[Sound_memfilePos], nRead);
        Sound_memfilePos += nRead;
        return nRead;
    }
    else
    {
        memcpy(pOutData, &Sound_pMemfileBuf[Sound_memfilePos], Sound_memfileSize - Sound_memfilePos);
        Sound_memfilePos = Sound_memfileSize;
        return 0;
    }
}

size_t J3DAPI Sound_MemFileWrite(tFileHandle fh, const void* pData, size_t size)
{
    J3D_UNUSED(fh);

    if ( size + Sound_memfilePos > Sound_memfileSize )
    {
        Sound_memfileSize += Sound_cacheBlockSize;
        uint8_t* pBufData = (uint8_t*)Sound_pHS->pRealloc(Sound_pMemfileBuf, Sound_memfileSize);
        if ( !pBufData )
        {
            SOUNDLOG_ERROR("Sound.MemFileWrite: Couldn't realloc %d.\n", Sound_memfileSize);
            Sound_MemFileFree();
            Sound_MemFileReset();
            return 0;
        }

        Sound_pMemfileBuf = pBufData;
    }

    memcpy(&Sound_pMemfileBuf[Sound_memfilePos], pData, size);
    Sound_memfilePos += size;
    return size;
}

tFileHandle Sound_MemFileReset(void)
{
    Sound_memfilePos = 0;
    return 1;
}

void Sound_MemFileFree(void)
{
    if ( Sound_pMemfileBuf )
    {
        Sound_pHS->pFree(Sound_pMemfileBuf);
        Sound_pMemfileBuf = NULL;
    }

    Sound_memfileSize = 0;
    Sound_memfilePos  = 0;
}

void Sound_StopAllNonStaticSounds(void)
{
    if ( !Sound_apChannels )
    {
        return;
    }

    for ( size_t i = 0; i < Sound_numChannels; ++i )
    {
        tSoundChannel* pChannel = &Sound_apChannels[i];
        if ( pChannel->handle != SOUND_INVALIDHANDLE )
        {
            SoundInfo* pSndInfo = Sound_GetSoundInfo(pChannel->hSnd);
            if ( pSndInfo )
            {
                if ( pSndInfo->bankNum != SOUNDBANK_STATIC_NUM ) // if not static
                {
                    Sound_Release(pChannel);
                }
            }
        }
    }
}

int J3DAPI Sound_ExportBank(tFileHandle fh, size_t bankNum)
{
    // Write num sound infos
    if ( Sound_pHS->pFileWrite(fh, &soundbank_aNumSounds[bankNum], sizeof(uint32_t)) != sizeof(uint32_t) )
    {
        return 0;
    }

    // Write sound data size
    if ( Sound_pHS->pFileWrite(fh, &soundbank_aUsedCacheSizes[bankNum], sizeof(uint32_t)) != sizeof(uint32_t) )
    {
        return 0;
    }

    // Write sound infos
    if ( Sound_pHS->pFileWrite(fh, soundbank_apSoundInfos[bankNum], sizeof(SoundInfo) * soundbank_aNumSounds[bankNum]) != sizeof(SoundInfo) * soundbank_aNumSounds[bankNum] )
    {
        return 0;
    }

    // Write sound data
    if ( Sound_pHS->pFileWrite(fh, soundbank_apSoundCache[bankNum], soundbank_aUsedCacheSizes[bankNum]) != soundbank_aUsedCacheSizes[bankNum] )
    {
        return 0;
    }

    // Write next handle
    return Sound_pHS->pFileWrite(fh, &Sound_handleEntropy, sizeof(uint32_t)) == sizeof(uint32_t);
}

int J3DAPI Sound_ImportBank(tFileHandle fh, size_t bankNum)
{
    Sound_Reset(bankNum == SOUNDBANK_STATIC_NUM);

    // Read num sound infos
    Sound_ReadFile(fh, &soundbank_aNumSounds[bankNum], sizeof(uint32_t));

    // Allocate sound info cache
    if ( soundbank_aSizeSounds[bankNum] < soundbank_aNumSounds[bankNum] )
    {
        // Realloc sound info cache to import size
        soundbank_aSizeSounds[bankNum] = soundbank_aNumSounds[bankNum];
        if ( soundbank_apSoundInfos[bankNum] )
        {
            Sound_pHS->pFree(soundbank_apSoundInfos[bankNum]);
        }

        soundbank_apSoundInfos[bankNum] = (SoundInfo*)Sound_pHS->pMalloc(sizeof(SoundInfo) * soundbank_aSizeSounds[bankNum]);
        if ( !soundbank_apSoundInfos[bankNum] )
        {
            SOUNDLOG_ERROR("Sound:ImportBank: Failed to allocate %d bytes.\n", sizeof(SoundInfo) * soundbank_aSizeSounds[bankNum]);
            return 0;
        }
    }

    // Read sound data size
    Sound_ReadFile(fh, &soundbank_aUsedCacheSizes[bankNum], sizeof(uint32_t));

    // Allocate sound data cache
    if ( soundbank_aCacheSizes[bankNum] < soundbank_aUsedCacheSizes[bankNum] )
    {
        soundbank_aCacheSizes[bankNum] = soundbank_aUsedCacheSizes[bankNum];
        if ( soundbank_apSoundCache[bankNum] )
        {
            Sound_pHS->pFree(soundbank_apSoundCache[bankNum]);
        }

        soundbank_apSoundCache[bankNum] = (uint8_t*)Sound_pHS->pMalloc(soundbank_aCacheSizes[bankNum]);
        if ( !soundbank_apSoundCache[bankNum] )
        {
            SOUNDLOG_ERROR("Sound:ImportBank: Failed to allocate %d bytes.\n", soundbank_aCacheSizes[bankNum]);
            return 0;
        }
    }

    // Read sound infos, sound data and next handle
    Sound_ReadFile(fh, soundbank_apSoundInfos[bankNum], sizeof(SoundInfo) * soundbank_aNumSounds[bankNum]);
    Sound_ReadFile(fh, soundbank_apSoundCache[bankNum], soundbank_aUsedCacheSizes[bankNum]);
    Sound_ReadFile(fh, &Sound_handleEntropy, sizeof(uint32_t));

    soundbank_lastImportedBankNum = bankNum;
    return 1;
}

int J3DAPI Sound_ReadFile(tFileHandle fh, void* pData, size_t size)
{
    while ( size )
    {
        size_t nRemaining = size;
        if ( size >= 0x40000 )
        {
            nRemaining = 0x40000;
        }

        if ( Sound_pHS->pFileRead(fh, pData, nRemaining) != nRemaining )
        {
            return 0;
        }

        SoundDriver_Sleep(20u);

        pData = (char*)pData + nRemaining;
        size -= nRemaining;
    }

    return 1;
}
#include <unity_fixture.h>

#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <j3dcore/j3d.h>
#include <Windows.h>
#include <mmsystem.h>
#include <Mmreg.h>
#if defined (J3D_DIRECTX6)
#include <DirectX6/dsound.h>
#elif defined (J3D_DIRECTX9)
#include <dsound.h>
#endif

#include <smush/insane/Memory/Memory.h>
#include <smush/insane/Sound/Dsnd.h>
#include <smush/insane/Sound/Engine.h>
#include <smush/insane/Sound/Mixer.h>
#include <smush/insane/Timer/Timer.h>

// Unit tests of the smush Sound area (Engine, Mixer, Dsnd) with synthetic data and mocked services: the LEC share table,
// the debug allocator, the timer, the sound driver and a pure C IDirectSound/IDirectSoundBuffer. MixerAsm is tested by its own lane.

// Test-only resets of the private statics (Engine.c, Dsnd.c under J3D_TEST)
void Engine_TestReset(void);
void Dsnd_TestReset(void);
void Dsnd_TestSetOwned(LPDIRECTSOUNDBUFFER pPrimary);

// ---------------------------------------------------------------------------------------------------------------------
// Mocks
// ---------------------------------------------------------------------------------------------------------------------

#define SST_MAX_ALLOCS 16
#define SST_MAX_NAMES 8
#define SST_MAX_FEEDS 128

typedef struct SstMemory
{
    MemoryDebugInterface iface;
    int numAllocs;
    int numFrees;
    int sizes[SST_MAX_ALLOCS];
    int failFrom; // 1-based number of the first failing Alloc call (0: never)
    int failOnly; // fail only this call (0: off)
} SstMemory;

static SstMemory SstMem;

static void* SstMemAlloc(int size, const char* pFile, int line)
{
    J3D_UNUSED(pFile); J3D_UNUSED(line);
    int call = ++SstMem.numAllocs;
    if ( call <= SST_MAX_ALLOCS ) {
        SstMem.sizes[call - 1] = size;
    }

    if ( (SstMem.failFrom && call >= SstMem.failFrom) || (SstMem.failOnly && call == SstMem.failOnly) ) {
        return NULL;
    }

    return calloc(1, (size_t)size + 16);
}

static void SstMemFree(void* pBlock, const char* pFile, int line)
{
    J3D_UNUSED(pFile); J3D_UNUSED(line);
    SstMem.numFrees++;
    free(pBlock);
}

static void SstMemReset(void)
{
    memset(&SstMem, 0, sizeof(SstMem));
    SstMem.iface.Alloc = SstMemAlloc;
    SstMem.iface.Free = SstMemFree;
}

typedef struct SstTimer
{
    TimerInterface iface;
    int numAllocs;
    TimerCallback pfnAlloc;
    int allocInterval;
    uint32_t allocFlags;
    int allocParam;
    int numFrees;
    int lastFreed;
    int numSync;
    int lastSync;
    int numSetInterval;
    int lastSetIndex;
    int lastSetInterval;
    int ticks;
} SstTimer;

static SstTimer SstTim;

static int SstTimerAlloc(TimerCallback pfn, int interval, uint32_t flags, int param)
{
    SstTim.numAllocs++;
    SstTim.pfnAlloc = pfn;
    SstTim.allocInterval = interval;
    SstTim.allocFlags = flags;
    SstTim.allocParam = param;
    return 7;
}

static void SstTimerFree(int index)
{
    SstTim.numFrees++;
    SstTim.lastFreed = index;
}

static void SstTimerSetInterval(int index, int interval)
{
    SstTim.numSetInterval++;
    SstTim.lastSetIndex = index;
    SstTim.lastSetInterval = interval;
}

static int SstTimerGetTicks(void)
{
    return SstTim.ticks;
}

static void SstTimerSync(int ticks)
{
    SstTim.numSync++;
    SstTim.lastSync = ticks;
}

static void SstTimerReset(void)
{
    memset(&SstTim, 0, sizeof(SstTim));
    SstTim.iface.Alloc = SstTimerAlloc;
    SstTim.iface.Free = SstTimerFree;
    SstTim.iface.SetInterval = SstTimerSetInterval;
    SstTim.iface.GetTicks = SstTimerGetTicks;
    SstTim.iface.Sync = SstTimerSync;
}

typedef struct SstShare
{
    LecShare iface;
    const char* aPublishedNames[SST_MAX_NAMES];
    void* aPublished[SST_MAX_NAMES];
    int numPublished;
    int numUnpublished;
    const char* aSubscribeNames[SST_MAX_NAMES];
    int numSubscribed;
    void* aUnsubscribed[SST_MAX_NAMES];
    int numUnsubscribed;
    const char* aErrors[SST_MAX_NAMES];
    int numErrors;
    void* pMemory;   // MEMORY.workspace
    void* pMixerMem; // MEMORY.mixertable
    void* pTimerExt; // TIMER.external_sync
    void* pTimerClk; // TIMER=clock
    void* pDriver;   // SNDDRIVER.driver
} SstShare;

static SstShare SstSh;

static int SstSharePublish(const char* pName, void* pInterface)
{
    if ( SstSh.numPublished < SST_MAX_NAMES )
    {
        SstSh.aPublishedNames[SstSh.numPublished] = pName;
        SstSh.aPublished[SstSh.numPublished] = pInterface;
    }

    SstSh.numPublished++;
    return 0;
}

static void SstShareUnpublish(const char* pName)
{
    J3D_UNUSED(pName);
    SstSh.numUnpublished++;
}

static void* SstShareSubscribe(const char* pName, void* pParam)
{
    J3D_UNUSED(pParam);
    if ( SstSh.numSubscribed < SST_MAX_NAMES ) {
        SstSh.aSubscribeNames[SstSh.numSubscribed] = pName;
    }

    SstSh.numSubscribed++;
    if ( strcmp(pName, "MEMORY.workspace") == 0 ) return SstSh.pMemory;
    if ( strcmp(pName, "MEMORY.mixertable") == 0 ) return SstSh.pMixerMem;
    if ( strcmp(pName, "TIMER.external_sync") == 0 ) return SstSh.pTimerExt;
    if ( strcmp(pName, "TIMER=clock") == 0 ) return SstSh.pTimerClk;
    if ( strcmp(pName, "SNDDRIVER.driver") == 0 ) return SstSh.pDriver;
    return NULL;
}

static void SstShareUnsubscribe(void* pInterface)
{
    if ( SstSh.numUnsubscribed < SST_MAX_NAMES ) {
        SstSh.aUnsubscribed[SstSh.numUnsubscribed] = pInterface;
    }

    SstSh.numUnsubscribed++;
}

static void SstShareReportError(const char* pFormat, ...)
{
    if ( SstSh.numErrors < SST_MAX_NAMES ) {
        SstSh.aErrors[SstSh.numErrors] = pFormat;
    }

    SstSh.numErrors++;
}

static void SstShareReset(void)
{
    memset(&SstSh, 0, sizeof(SstSh));
    SstSh.iface.Publish = SstSharePublish;
    SstSh.iface.Unpublish = SstShareUnpublish;
    SstSh.iface.Subscribe = SstShareSubscribe;
    SstSh.iface.Unsubscribe = SstShareUnsubscribe;
    SstSh.iface.ReportError = SstShareReportError;
}

static void* SstSharePublished(const char* pName)
{
    for ( int i = 0; i < SstSh.numPublished && i < SST_MAX_NAMES; i++ )
    {
        if ( strcmp(SstSh.aPublishedNames[i], pName) == 0 ) {
            return SstSh.aPublished[i];
        }
    }

    return NULL;
}

// Mock sound driver (what the engine subscribes to as SNDDRIVER.driver)
typedef struct SstDriver
{
    SndDriver iface;
    int startResult;
    int numStart;
    int startRate;
    int startTracks;
    void (*pfnFill)(int, SoundFeed);
    int numStop;
    int volume;
    int rate;
    int numSetRate;
} SstDriver;

static SstDriver SstDrv;

static int SstDriverStart(int rate, int tracks, void (*pfnFill)(int, SoundFeed))
{
    SstDrv.numStart++;
    SstDrv.startRate = rate;
    SstDrv.startTracks = tracks;
    SstDrv.pfnFill = pfnFill;
    return SstDrv.startResult;
}

static void SstDriverStop(void)
{
    SstDrv.numStop++;
}

static void SstDriverSetVolume(int volume)
{
    SstDrv.volume = volume;
}

static int SstDriverGetVolume(void)
{
    return SstDrv.volume;
}

static int SstDriverSetRate(int rate)
{
    SstDrv.numSetRate++;
    SstDrv.rate = rate;
    return 0;
}

static int SstDriverGetRate(void)
{
    return SstDrv.rate;
}

static void SstDriverReset(void)
{
    memset(&SstDrv, 0, sizeof(SstDrv));
    SstDrv.iface.Start = SstDriverStart;
    SstDrv.iface.Stop = SstDriverStop;
    SstDrv.iface.SetVolume = SstDriverSetVolume;
    SstDrv.iface.GetVolume = SstDriverGetVolume;
    SstDrv.iface.SetRate = SstDriverSetRate;
    SstDrv.iface.GetRate = SstDriverGetRate;
    SstDrv.rate = 22050;
    SstDrv.volume = 99;
}

// Records the engine's feed calls
typedef struct SstFeed
{
    int offset;
    int n;
    int bytes;
    int a5;
    int volume;
    int pan;
    int format;
    uint8_t head[8];
} SstFeed;

static SstFeed SstFeeds[SST_MAX_FEEDS];
static int SstNumFeeds;

static void SstFeedFn(void* pSrc, int offset, int n, int bytes, int a5, int volume, int pan, int format)
{
    if ( SstNumFeeds < SST_MAX_FEEDS )
    {
        SstFeed* pFeed = &SstFeeds[SstNumFeeds];
        pFeed->offset = offset;
        pFeed->n = n;
        pFeed->bytes = bytes;
        pFeed->a5 = a5;
        pFeed->volume = volume;
        pFeed->pan = pan;
        pFeed->format = format;
        memcpy(pFeed->head, pSrc, bytes < 8 ? (size_t)bytes : 8);
    }

    SstNumFeeds++;
}

// ---------------------------------------------------------------------------------------------------------------------
// Engine tests
// ---------------------------------------------------------------------------------------------------------------------

TEST_GROUP(SmushSoundEngine);

static LecModule* SstEngModule;
static SoundInterface* SstSnd;
static int SstEngStarted;

static int SstEngineStartup(void)
{
    TEST_ASSERT_EQUAL_INT(0, sound_Register(&SstSh.iface, NULL));
    SstEngModule = SstSharePublished("LECMODULE=insane_sound");
    SstSnd = SstSharePublished("SOUND=insane");
    TEST_ASSERT_NOT_NULL(SstEngModule);
    TEST_ASSERT_NOT_NULL(SstSnd);
    int res = SstEngModule->Startup();
    SstEngStarted = res == 0;
    return res;
}

TEST_SETUP(SmushSoundEngine)
{
    Engine_TestReset();
    SstShareReset();
    SstMemReset();
    SstTimerReset();
    SstDriverReset();
    SstSh.pMemory = &SstMem.iface;
    SstSh.pTimerExt = &SstTim.iface;
    SstSh.pDriver = &SstDrv.iface;
    SstNumFeeds = 0;
    SstEngStarted = 0;
    SstEngModule = NULL;
    SstSnd = NULL;
}

TEST_TEAR_DOWN(SmushSoundEngine)
{
    if ( SstEngModule )
    {
        SstEngModule->Shutdown();
        SstEngModule->Unregister();
    }
}

// Writes a big-endian value
static void SstPutBE32(uint8_t* p, uint32_t v)
{
    p[0] = (uint8_t)(v >> 24);
    p[1] = (uint8_t)(v >> 16);
    p[2] = (uint8_t)(v >> 8);
    p[3] = (uint8_t)v;
}

static void SstPutBE16(uint8_t* p, uint32_t v)
{
    p[0] = (uint8_t)(v >> 8);
    p[1] = (uint8_t)v;
}

// Builds a `Crea` block whose sample data follows its header. The retail code reads the 24-bit block length from the byte at +1
// three times, so the only non-zero length a caller can express is lenByte * 0x010101 (65793 samples for 1), or 0.
#define SST_VOC_OFFSET 0x1A
#define SST_VOC_LEN    0x010101
static uint8_t* SstMakeVoc(int lenByte, int* pNumSamples)
{
    int numSamples = lenByte * 0x010101;
    uint8_t* pData = calloc(1, 0x40 + (size_t)numSamples);
    memcpy(pData, "Crea", 4);
    pData[0x14] = SST_VOC_OFFSET;
    pData[0x15] = 0;
    pData[SST_VOC_OFFSET + 1] = (uint8_t)lenByte;
    for ( int i = 0; i < numSamples; i++ ) {
        pData[SST_VOC_OFFSET + 6 + i] = (uint8_t)(i * 7 + 3);
    }

    *pNumSamples = numSamples;
    return pData;
}

TEST(SmushSoundEngine, RegisterPublishesModuleAndInterface)
{
    TEST_ASSERT_EQUAL_INT(0, sound_Register(&SstSh.iface, NULL));
    SstEngModule = SstSh.aPublished[0];
    TEST_ASSERT_EQUAL_INT(2, SstSh.numPublished);
    TEST_ASSERT_EQUAL_STRING("LECMODULE=insane_sound", SstSh.aPublishedNames[0]);
    TEST_ASSERT_EQUAL_STRING("SOUND=insane", SstSh.aPublishedNames[1]);

    LecModule* pModule = SstSh.aPublished[0];
    TEST_ASSERT_TRUE(pModule->Register == sound_Register);
    TEST_ASSERT_TRUE(pModule->Unregister == sound_Unregister);
    TEST_ASSERT_TRUE(pModule->Startup == sound_Startup);
    TEST_ASSERT_TRUE(pModule->Shutdown == sound_Shutdown);

    SoundInterface* pSnd = SstSh.aPublished[1];
    TEST_ASSERT_TRUE(pSnd->Play == sound_Play);
    TEST_ASSERT_TRUE(pSnd->Stop == sound_Stop);
    TEST_ASSERT_TRUE(pSnd->Pause == sound_Pause);
    TEST_ASSERT_TRUE(pSnd->Resume == sound_Resume);
    TEST_ASSERT_TRUE(pSnd->StopAll == sound_StopAll);
    TEST_ASSERT_TRUE(pSnd->GetVolume == sound_GetVolume);
    TEST_ASSERT_TRUE(pSnd->SetVolume == sound_SetVolume);
    TEST_ASSERT_TRUE(pSnd->GetPan == sound_GetPan);
    TEST_ASSERT_TRUE(pSnd->SetPan == sound_SetPan);
    TEST_ASSERT_TRUE(pSnd->GetMasterVolume == sound_GetMasterVolume);
    TEST_ASSERT_TRUE(pSnd->SetMasterVolume == sound_SetMasterVolume);
    TEST_ASSERT_TRUE(pSnd->GetRate == sound_GetRate);
    TEST_ASSERT_TRUE(pSnd->SetRate == sound_SetRate);
    TEST_ASSERT_TRUE(pSnd->GetPosition == sound_GetPosition);
    TEST_ASSERT_TRUE(pSnd->Query == sound_Query);
    TEST_ASSERT_TRUE(pSnd->Control == sound_Control);

    // Without a driver the master volume and rate report -1 and setting them does nothing
    TEST_ASSERT_EQUAL_INT(-1, sound_GetMasterVolume());
    TEST_ASSERT_EQUAL_INT(-1, sound_GetRate());
    sound_SetMasterVolume(5);
    sound_SetRate(8000);
    TEST_ASSERT_EQUAL_INT(99, SstDrv.volume);
    TEST_ASSERT_EQUAL_INT(0, SstDrv.numSetRate);

    sound_Unregister();
    TEST_ASSERT_EQUAL_INT(2, SstSh.numUnpublished);
    SstEngModule = NULL; // Not started: nothing to shut down
}

TEST(SmushSoundEngine, StartupOpensOneTrackAndStartsDriver)
{
    TEST_ASSERT_EQUAL_INT(0, SstEngineStartup());
    TEST_ASSERT_EQUAL_INT(3, SstSh.numSubscribed);
    TEST_ASSERT_EQUAL_STRING("MEMORY.workspace", SstSh.aSubscribeNames[0]);
    TEST_ASSERT_EQUAL_STRING("TIMER.external_sync", SstSh.aSubscribeNames[1]);
    TEST_ASSERT_EQUAL_STRING("SNDDRIVER.driver", SstSh.aSubscribeNames[2]);

    // One track: the 0x1800-byte fade buffer and the ring buffer of the registered 500000 bytes
    TEST_ASSERT_EQUAL_INT(2, SstMem.numAllocs);
    TEST_ASSERT_EQUAL_INT(0x1800, SstMem.sizes[0]);
    TEST_ASSERT_EQUAL_INT(500000, SstMem.sizes[1]);

    TEST_ASSERT_EQUAL_INT(1, SstDrv.numStart);
    TEST_ASSERT_EQUAL_INT(22050, SstDrv.startRate);
    TEST_ASSERT_EQUAL_INT(1, SstDrv.startTracks);
    TEST_ASSERT_TRUE(SstDrv.pfnFill == sound_Callback);
    TEST_ASSERT_EQUAL_INT(1, SstTim.numSync);
    TEST_ASSERT_EQUAL_INT(0, SstTim.lastSync);
    TEST_ASSERT_EQUAL_INT(22050, sound_GetRate());
}

TEST(SmushSoundEngine, StartupReportsMissingServices)
{
    SstSh.pMemory = NULL;
    TEST_ASSERT_EQUAL_INT(-1, SstEngineStartup());
    TEST_ASSERT_EQUAL_INT(1, SstSh.numErrors);
    TEST_ASSERT_EQUAL_STRING("Sound Module: Cannot allocate memory services.", SstSh.aErrors[0]);

    SstSh.pMemory = &SstMem.iface;
    SstSh.pTimerExt = NULL;
    TEST_ASSERT_EQUAL_INT(-1, SstEngModule->Startup());
    TEST_ASSERT_EQUAL_STRING("Sound Module: Cannot allocate timer services.", SstSh.aErrors[1]);

    SstSh.pTimerExt = &SstTim.iface;
    SstSh.pDriver = NULL;
    TEST_ASSERT_EQUAL_INT(-1, SstEngModule->Startup());
    TEST_ASSERT_EQUAL_STRING("Sound Module: Cannot allocate snddriver services.", SstSh.aErrors[2]);
    TEST_ASSERT_EQUAL_INT(0, SstDrv.numStart);
}

TEST(SmushSoundEngine, StartupSurvivesRingBufferAllocationFailure)
{
    // The first attempt at the ring buffer fails: the engine retries and then shrinks the track by the minimum size
    SstMem.failOnly = 2;
    TEST_ASSERT_EQUAL_INT(0, SstEngineStartup());
    TEST_ASSERT_EQUAL_INT(4, SstMem.numAllocs);
    TEST_ASSERT_EQUAL_INT(0x1800, SstMem.sizes[0]);
    TEST_ASSERT_EQUAL_INT(500000, SstMem.sizes[1]);
    TEST_ASSERT_EQUAL_INT(500000, SstMem.sizes[2]);
    TEST_ASSERT_EQUAL_INT(500000 - 64000, SstMem.sizes[3]);
    TEST_ASSERT_EQUAL_INT(1, SstMem.numFrees); // The block of the retry
    TEST_ASSERT_EQUAL_INT(1, SstDrv.numStart);
}

TEST(SmushSoundEngine, StartupFailsWhenNoRingBufferCanBeAllocated)
{
    SstMem.failFrom = 2;
    TEST_ASSERT_EQUAL_INT(-1, SstEngineStartup());
    TEST_ASSERT_EQUAL_INT(1, SstMem.numFrees); // The fade buffer
    TEST_ASSERT_EQUAL_INT(0, SstDrv.numStart);
    TEST_ASSERT_EQUAL_INT(0, SstDrv.numStop);
}

TEST(SmushSoundEngine, StartupFailsWhenFadeBufferCannotBeAllocated)
{
    SstMem.failFrom = 1;
    TEST_ASSERT_EQUAL_INT(-1, SstEngineStartup());
    TEST_ASSERT_EQUAL_INT(1, SstMem.numAllocs);
    TEST_ASSERT_EQUAL_INT(0, SstDrv.numStart);
}

TEST(SmushSoundEngine, ShutdownReleasesServicesAndMemory)
{
    TEST_ASSERT_EQUAL_INT(0, SstEngineStartup());
    SstEngModule->Shutdown();
    TEST_ASSERT_EQUAL_INT(3, SstSh.numUnsubscribed);
    TEST_ASSERT_TRUE(SstSh.aUnsubscribed[0] == &SstMem.iface);
    TEST_ASSERT_TRUE(SstSh.aUnsubscribed[1] == &SstDrv.iface);
    TEST_ASSERT_TRUE(SstSh.aUnsubscribed[2] == &SstTim.iface);
    TEST_ASSERT_EQUAL_INT(1, SstDrv.numStop);
    TEST_ASSERT_EQUAL_INT(2, SstMem.numFrees);
}

TEST(SmushSoundEngine, CloseReturnsNumberOfCallbacks)
{
    TEST_ASSERT_EQUAL_INT(0, SstEngineStartup());
    sound_Callback(100, SstFeedFn);
    sound_Callback(100, SstFeedFn);
    sound_Callback(100, SstFeedFn);
    TEST_ASSERT_EQUAL_INT(3, sound_Close());
    TEST_ASSERT_EQUAL_INT(1, SstDrv.numStop);
    // A second close finds nothing to stop or free
    TEST_ASSERT_EQUAL_INT(3, sound_Close());
    TEST_ASSERT_EQUAL_INT(1, SstDrv.numStop);
    TEST_ASSERT_EQUAL_INT(2, SstMem.numFrees);
}

TEST(SmushSoundEngine, CallbackAdvancesTimerBySampleCount)
{
    TEST_ASSERT_EQUAL_INT(0, SstEngineStartup());
    int numSync = SstTim.numSync;
    sound_Callback(22050, SstFeedFn); // One second of samples = 1193180 ticks
    TEST_ASSERT_EQUAL_INT(numSync + 1, SstTim.numSync);
    TEST_ASSERT_EQUAL_INT(1193180, SstTim.lastSync);
    sound_Callback(11025, SstFeedFn);
    TEST_ASSERT_EQUAL_INT(596590, SstTim.lastSync);
}

TEST(SmushSoundEngine, SyncTimerCompensatesForClockDrift)
{
    TEST_ASSERT_EQUAL_INT(0, SstEngineStartup());
    SstTim.ticks = 1000;
    sound_SyncTimer();
    SstTim.ticks = 1600; // 600 ticks later than the stored sync point: the engine's clock is pulled back by 600
    sound_Callback(22050, SstFeedFn);
    TEST_ASSERT_EQUAL_INT(1193180 - 600, SstTim.lastSync);
    sound_Callback(22050, SstFeedFn); // The sync point is cleared
    TEST_ASSERT_EQUAL_INT(1193180, SstTim.lastSync);
}

TEST(SmushSoundEngine, PlayVocFeedsSamplesToMixer)
{
    TEST_ASSERT_EQUAL_INT(0, SstEngineStartup());
    int numSamples;
    uint8_t* pVoc = SstMakeVoc(1, &numSamples);
    TEST_ASSERT_EQUAL_INT(SST_VOC_LEN, numSamples);

    SstSnd->Play(pVoc, 5, 100, -20, 0);
    TEST_ASSERT_EQUAL_INT(0, SstNumFeeds);
    TEST_ASSERT_EQUAL_INT(100, sound_GetVolume(5));
    TEST_ASSERT_EQUAL_INT(-20, sound_GetPan(5));
    TEST_ASSERT_EQUAL_INT(1, sound_GetPosition(5)); // Starting: the position is still 0

    sound_Callback(100, SstFeedFn);
    TEST_ASSERT_EQUAL_INT(1, SstNumFeeds);
    TEST_ASSERT_EQUAL_INT(0, SstFeeds[0].offset);
    TEST_ASSERT_EQUAL_INT(100, SstFeeds[0].n);
    TEST_ASSERT_EQUAL_INT(100, SstFeeds[0].bytes);
    TEST_ASSERT_EQUAL_INT(1, SstFeeds[0].a5);
    TEST_ASSERT_EQUAL_INT(100, SstFeeds[0].volume);
    TEST_ASSERT_EQUAL_INT(-20, SstFeeds[0].pan);
    TEST_ASSERT_EQUAL_INT(0, SstFeeds[0].format);
    TEST_ASSERT_EQUAL_MEMORY(pVoc + SST_VOC_OFFSET + 6, SstFeeds[0].head, 8);

    TEST_ASSERT_EQUAL_INT(101, sound_GetPosition(5));
    TEST_ASSERT_EQUAL_INT(101, sound_GetPosition(SOUND_NOTRACK)); // Any playing track
    TEST_ASSERT_EQUAL_INT(101, sound_GetPosition(-1));            // Track index 0
    TEST_ASSERT_EQUAL_INT(0, sound_GetPosition(777));

    sound_Callback(100, SstFeedFn);
    TEST_ASSERT_EQUAL_INT(2, SstNumFeeds);
    TEST_ASSERT_EQUAL_INT(100, SstFeeds[1].bytes);
    TEST_ASSERT_EQUAL_MEMORY(pVoc + SST_VOC_OFFSET + 6 + 100, SstFeeds[1].head, 8);
    TEST_ASSERT_EQUAL_INT(201, sound_GetPosition(5));

    // The master volume (the variable 0xFF) scales the track volume
    sound_SetVolume(SOUND_NOTRACK, 64);
    TEST_ASSERT_EQUAL_INT(64, sound_GetVolume(SOUND_NOTRACK));
    sound_Callback(100, SstFeedFn);
    TEST_ASSERT_EQUAL_INT(100 * 64 / 127, SstFeeds[2].volume);

    sound_SetVolume(5, 40);
    sound_SetPan(5, 33);
    TEST_ASSERT_EQUAL_INT(40, sound_GetVolume(5));
    TEST_ASSERT_EQUAL_INT(33, sound_GetPan(5));
    TEST_ASSERT_EQUAL_INT(-1, sound_GetVolume(6));
    TEST_ASSERT_EQUAL_INT(-1, sound_GetPan(6));

    sound_Stop(5);
    TEST_ASSERT_EQUAL_INT(0, sound_GetPosition(5));
    TEST_ASSERT_EQUAL_INT(-1, sound_GetVolume(5));
    TEST_ASSERT_EQUAL_INT(0, sound_GetPosition(SOUND_NOTRACK));
    sound_Callback(100, SstFeedFn);
    TEST_ASSERT_EQUAL_INT(3, SstNumFeeds);
    free(pVoc);
}

TEST(SmushSoundEngine, EmptyVocStopsTheTrack)
{
    TEST_ASSERT_EQUAL_INT(0, SstEngineStartup());
    int numSamples;
    uint8_t* pVoc = SstMakeVoc(0, &numSamples);
    SstSnd->Play(pVoc, 9, 100, 0, 0);
    TEST_ASSERT_EQUAL_INT(100, sound_GetVolume(9));
    sound_Callback(100, SstFeedFn);
    TEST_ASSERT_EQUAL_INT(0, SstNumFeeds);
    TEST_ASSERT_EQUAL_INT(-1, sound_GetVolume(9)); // The block list ended: state 0, no id
    free(pVoc);
}

TEST(SmushSoundEngine, PlayIgnoredWhenDriverDidNotStart)
{
    SstDrv.startResult = -1;
    TEST_ASSERT_EQUAL_INT(0, SstEngineStartup());
    int numSamples;
    uint8_t* pVoc = SstMakeVoc(1, &numSamples);
    SstSnd->Play(pVoc, 5, 100, 0, 0);
    TEST_ASSERT_EQUAL_INT(-1, sound_GetVolume(5));
    free(pVoc);
}

TEST(SmushSoundEngine, PlayIgnoresUnknownTag)
{
    TEST_ASSERT_EQUAL_INT(0, SstEngineStartup());
    uint8_t data[0x40] = { 'X', 'Y', 'Z', 'W' };
    SstSnd->Play(data, 5, 100, 0, 0);
    TEST_ASSERT_EQUAL_INT(-1, sound_GetVolume(5));
}

TEST(SmushSoundEngine, StopAllClearsEveryTrack)
{
    TEST_ASSERT_EQUAL_INT(0, SstEngineStartup());
    int numSamples;
    uint8_t* pVoc = SstMakeVoc(1, &numSamples);
    SstSnd->Play(pVoc, 5, 100, 0, 0);
    TEST_ASSERT_EQUAL_INT(100, sound_GetVolume(5));
    SstSnd->StopAll();
    TEST_ASSERT_EQUAL_INT(-1, sound_GetVolume(5));
    free(pVoc);
}

TEST(SmushSoundEngine, PauseSuppressesFeedUntilResumed)
{
    TEST_ASSERT_EQUAL_INT(0, SstEngineStartup());
    int numSamples;
    uint8_t* pVoc = SstMakeVoc(1, &numSamples);
    SstSnd->Play(pVoc, 5, 100, 0, 0);

    SstSnd->Pause();
    SstSnd->Pause();
    sound_Callback(100, SstFeedFn);
    TEST_ASSERT_EQUAL_INT(0, SstNumFeeds);
    TEST_ASSERT_EQUAL_INT(2, SstTim.numSync); // The clock still runs

    SstSnd->Resume();
    sound_Callback(100, SstFeedFn);
    TEST_ASSERT_EQUAL_INT(0, SstNumFeeds); // Still paused once

    SstSnd->Resume();
    SstSnd->Resume(); // Resume without pause does nothing
    sound_Callback(100, SstFeedFn);
    TEST_ASSERT_EQUAL_INT(1, SstNumFeeds);
    free(pVoc);
}

TEST(SmushSoundEngine, MasterVolumeAndRateGoThroughDriver)
{
    TEST_ASSERT_EQUAL_INT(0, SstEngineStartup());
    SstSnd->SetMasterVolume(42);
    TEST_ASSERT_EQUAL_INT(42, SstDrv.volume);
    TEST_ASSERT_EQUAL_INT(42, SstSnd->GetMasterVolume());
    SstSnd->SetRate(11025);
    TEST_ASSERT_EQUAL_INT(1, SstDrv.numSetRate);
    TEST_ASSERT_EQUAL_INT(11025, SstDrv.rate);
    TEST_ASSERT_EQUAL_INT(11025, SstSnd->GetRate());
    // The engine's own sample rate follows SetRate: the callback's time scale is 1193180 ticks per second of samples
    sound_Callback(11025, SstFeedFn);
    TEST_ASSERT_EQUAL_INT(1193180, SstTim.lastSync);
}

TEST(SmushSoundEngine, ControlAndQueryVariables)
{
    TEST_ASSERT_EQUAL_INT(0, SstEngineStartup());

    TEST_ASSERT_EQUAL_INT(0, SstSnd->Control(0, 1, 3, 77));
    int value = -1;
    TEST_ASSERT_EQUAL_INT(0, SstSnd->Query(0, 1, &value, (int*)(uintptr_t)3));
    TEST_ASSERT_EQUAL_INT(77, value);
    TEST_ASSERT_EQUAL_INT(77, sound_GetVar(3));
    TEST_ASSERT_EQUAL_INT(0, sound_GetVar(4));

    // The variable 0xFF is the master volume (127 after opening)
    TEST_ASSERT_EQUAL_INT(0, SstSnd->Query(0, 1, &value, (int*)(uintptr_t)0xFF));
    TEST_ASSERT_EQUAL_INT(127, value);
    TEST_ASSERT_EQUAL_INT(0, SstSnd->Control(0, 1, 0xFF, 50));
    TEST_ASSERT_EQUAL_INT(50, sound_GetVolume(SOUND_NOTRACK));
    TEST_ASSERT_EQUAL_INT(50, sound_GetVar(0xFF));

    int a = 0, b = 0;
    TEST_ASSERT_EQUAL_INT(0, SstSnd->Query(0, 2, &a, &b));
    TEST_ASSERT_EQUAL_INT(0x40, a);
    TEST_ASSERT_EQUAL_INT(0x100, b);
    TEST_ASSERT_EQUAL_INT(0, SstSnd->Control(0, 2, 0x20, 0)); // 0 leaves the value alone
    TEST_ASSERT_EQUAL_INT(0, SstSnd->Query(0, 2, &a, &b));
    TEST_ASSERT_EQUAL_INT(0x20, a);
    TEST_ASSERT_EQUAL_INT(0x100, b);
    TEST_ASSERT_EQUAL_INT(0, SstSnd->Control(0, 2, 0, 0x80));
    TEST_ASSERT_EQUAL_INT(0, SstSnd->Query(0, 2, &a, &b));
    TEST_ASSERT_EQUAL_INT(0x20, a);
    TEST_ASSERT_EQUAL_INT(0x80, b);

    TEST_ASSERT_EQUAL_INT(0, SstSnd->Query(0, 3, &a, NULL));
    TEST_ASSERT_EQUAL_INT(1, a);
    TEST_ASSERT_EQUAL_INT(0, SstSnd->Control(0, 3, 0, 0));
    TEST_ASSERT_EQUAL_INT(0, SstSnd->Query(0, 3, &a, NULL));
    TEST_ASSERT_EQUAL_INT(0, a);

    TEST_ASSERT_EQUAL_INT(0, SstSnd->Query(0, 4, &a, NULL));
    TEST_ASSERT_EQUAL_INT(1, a);
    TEST_ASSERT_EQUAL_INT(0, SstSnd->Control(0, 5, 0, 0));
    TEST_ASSERT_EQUAL_INT(0, SstSnd->Query(0, 5, &a, NULL));
    TEST_ASSERT_EQUAL_INT(0, a);
    TEST_ASSERT_EQUAL_INT(0, SstSnd->Query(0, 3, NULL, NULL)); // Null outputs are skipped

    TEST_ASSERT_EQUAL_INT(-1, SstSnd->Control(0, 9, 0, 0));
    TEST_ASSERT_EQUAL_INT(-1, SstSnd->Query(0, 9, &a, &b));
}

TEST(SmushSoundEngine, DisabledCategoryIsNotFed)
{
    TEST_ASSERT_EQUAL_INT(0, SstEngineStartup());
    int numSamples;
    uint8_t* pVoc = SstMakeVoc(1, &numSamples);
    SstSnd->Play(pVoc, 5, 100, 0, 0x80);                   // Effects
    TEST_ASSERT_EQUAL_INT(0, SstSnd->Control(0, 4, 0, 0)); // Effects off
    sound_Callback(100, SstFeedFn);
    TEST_ASSERT_EQUAL_INT(0, SstNumFeeds);
    TEST_ASSERT_EQUAL_INT(101, sound_GetPosition(5));      // The track still advances
    TEST_ASSERT_EQUAL_INT(0, SstSnd->Control(0, 4, 1, 0));
    sound_Callback(100, SstFeedFn);
    TEST_ASSERT_EQUAL_INT(1, SstNumFeeds);
    free(pVoc);
}

TEST(SmushSoundEngine, EffectsDuckMusic)
{
    TEST_ASSERT_EQUAL_INT(0, SstEngineStartup());
    sound_Close();
    TEST_ASSERT_EQUAL_INT(0, sound_Open(22050, 64000, 100000, 100000, 0));
    TEST_ASSERT_EQUAL_INT(2, SstDrv.startTracks);

    int numSamples;
    uint8_t* pFx = SstMakeVoc(1, &numSamples);
    uint8_t* pMusic = SstMakeVoc(1, &numSamples);
    SstSnd->Play(pFx, 1, 127, 10, 0x80);
    SstSnd->Play(pMusic, 2, 127, -10, 0x40);

    // Unducked: the music plays at full volume, then the effect pulls the level down to its floor
    sound_Callback(8192, SstFeedFn);
    int firstMusic = -1;
    for ( int i = 0; i < SstNumFeeds && i < SST_MAX_FEEDS; i++ )
    {
        if ( SstFeeds[i].pan == -10 )
        {
            firstMusic = SstFeeds[i].volume;
            break;
        }
    }

    TEST_ASSERT_EQUAL_INT(127, firstMusic);

    SstNumFeeds = 0;
    sound_Callback(8192, SstFeedFn);
    int secondMusic = -1;
    for ( int i = 0; i < SstNumFeeds && i < SST_MAX_FEEDS; i++ )
    {
        if ( SstFeeds[i].pan == -10 )
        {
            secondMusic = SstFeeds[i].volume;
            break;
        }
    }

    TEST_ASSERT_EQUAL_INT((0x40 * 127) >> 8, secondMusic);

    // Without effects the level recovers
    SstSnd->StopAll();
    sound_Callback(22050, SstFeedFn);
    sound_Callback(22050, SstFeedFn);
    free(pFx);
    free(pMusic);
}

// Builds a stream chunk: SAUD header, one type 1 block (10 bytes) and a sample chunk of `numSamples` bytes. `tag` is the sample chunk tag.
#define SST_STRM_HDR 10
static uint8_t* SstMakeStream(const char* pTag, int numSamples, int* pTotalLen)
{
    int len = 0x10 + SST_STRM_HDR + 8 + numSamples;
    uint8_t* pData = calloc(1, (size_t)len + 16);
    memcpy(pData, "SAUD", 4);
    SstPutBE32(pData + 4, (uint32_t)(len - 8));
    memcpy(pData + 8, "STRK", 4);
    SstPutBE32(pData + 0xC, SST_STRM_HDR);
    // Block type 1: [1][8] pos(4) avail(4)
    pData[0x10] = 1;
    pData[0x11] = 8;
    SstPutBE32(pData + 0x12, 0);
    SstPutBE32(pData + 0x16, (uint32_t)numSamples);
    // Sample chunk
    memcpy(pData + 0x10 + SST_STRM_HDR, pTag, 4);
    SstPutBE32(pData + 0x10 + SST_STRM_HDR + 4, (uint32_t)numSamples);
    for ( int i = 0; i < numSamples; i++ ) {
        pData[0x10 + SST_STRM_HDR + 8 + i] = (uint8_t)(i * 5 + 1);
    }

    *pTotalLen = len;
    return pData;
}

TEST(SmushSoundEngine, PlaySaudStreamUsesCallerBuffer)
{
    TEST_ASSERT_EQUAL_INT(0, SstEngineStartup());
    int len;
    uint8_t* pStream = SstMakeStream("SD8S", 200, &len); // Stereo 8-bit
    SstSnd->Play(pStream, 3, 64, 5, 0);

    sound_Callback(50, SstFeedFn); // 50 stereo frames = 100 bytes
    TEST_ASSERT_EQUAL_INT(1, SstNumFeeds);
    TEST_ASSERT_EQUAL_INT(50, SstFeeds[0].n);
    TEST_ASSERT_EQUAL_INT(100, SstFeeds[0].bytes);
    TEST_ASSERT_EQUAL_INT(64, SstFeeds[0].volume);
    TEST_ASSERT_EQUAL_INT(5, SstFeeds[0].pan);
    TEST_ASSERT_EQUAL_INT(1, SstFeeds[0].format);
    TEST_ASSERT_EQUAL_MEMORY(pStream + 0x10 + SST_STRM_HDR + 8, SstFeeds[0].head, 8);

    sound_Callback(50, SstFeedFn);
    TEST_ASSERT_EQUAL_INT(2, SstNumFeeds);
    TEST_ASSERT_EQUAL_MEMORY(pStream + 0x10 + SST_STRM_HDR + 8 + 100, SstFeeds[1].head, 8);
    TEST_ASSERT_EQUAL_INT(201, sound_GetPosition(3));

    // All samples are used up; the next block is not a block: the track ends
    sound_Callback(50, SstFeedFn);
    TEST_ASSERT_EQUAL_INT(2, SstNumFeeds);
    TEST_ASSERT_EQUAL_INT(-1, sound_GetVolume(3));
    free(pStream);
}

TEST(SmushSoundEngine, StreamFormatFollowsChunkTag)
{
    TEST_ASSERT_EQUAL_INT(0, SstEngineStartup());
    static const struct { const char* pTag; int format; int frameSize; } aFormats[] = {
        { "S6SZ", 5, 4 }, { "S6MZ", 4, 2 }, { "SD6M", 2, 2 }, { "SD2S", 1, 2 }, { "SD8S", 1, 2 }, { "SD6S", 3, 4 }, { "ZZZZ", 0, 1 },
    };

    for ( size_t i = 0; i < sizeof(aFormats) / sizeof(aFormats[0]); i++ )
    {
        int len;
        uint8_t* pStream = SstMakeStream(aFormats[i].pTag, 200, &len);
        SstNumFeeds = 0;
        SstSnd->Play(pStream, 3, 64, 5, 0);
        sound_Callback(10, SstFeedFn);
        TEST_ASSERT_EQUAL_INT(1, SstNumFeeds);
        TEST_ASSERT_EQUAL_INT(aFormats[i].format, SstFeeds[0].format);
        TEST_ASSERT_EQUAL_INT(10 * aFormats[i].frameSize, SstFeeds[0].bytes);
        SstSnd->StopAll();
        sound_Callback(1, SstFeedFn); // Lets the channel see the stopped track (a new sound on a playing track would cross-fade)
        free(pStream);
    }
}

TEST(SmushSoundEngine, MultiPartPsadStreamIsAssembledInRing)
{
    TEST_ASSERT_EQUAL_INT(0, SstEngineStartup());

    // Part 0 carries the stream header and the first 100 bytes, part 1 the remaining 100
    int innerLen;
    uint8_t* pInner = SstMakeStream("SD8S", 200, &innerLen);
    int firstLen = 0x10 + SST_STRM_HDR + 8 + 100;

    uint8_t* pPacket0 = calloc(1, 0x14 + (size_t)firstLen + 16);
    memcpy(pPacket0, "PSAD", 4);
    SstPutBE32(pPacket0 + 4, (uint32_t)(firstLen + 0xC));
    SstPutBE16(pPacket0 + 0xA, 77); // Stream id
    SstPutBE16(pPacket0 + 0xE, 0);  // Part
    SstPutBE16(pPacket0 + 0x12, 2); // Parts
    memcpy(pPacket0 + 0x14, pInner, (size_t)firstLen);

    int secondLen = 100;
    uint8_t* pPacket1 = calloc(1, 0x14 + (size_t)secondLen + 16);
    memcpy(pPacket1, "PSAD", 4);
    SstPutBE32(pPacket1 + 4, (uint32_t)(secondLen + 0xC));
    SstPutBE16(pPacket1 + 0xA, 77);
    SstPutBE16(pPacket1 + 0xE, 1);
    SstPutBE16(pPacket1 + 0x12, 2);
    memcpy(pPacket1 + 0x14, pInner + 0x10 + SST_STRM_HDR + 8 + 100, (size_t)secondLen);

    SstSnd->Play(pPacket0, 4, 100, 0, 0);

    // A part with a wrong sequence number is dropped
    SstPutBE16(pPacket1 + 0xE, 5);
    SstSnd->Play(pPacket1, 4, 100, 0, 0);
    SstPutBE16(pPacket1 + 0xE, 1);
    SstSnd->Play(pPacket1, 4, 100, 0, 0);

    sound_Callback(50, SstFeedFn);
    TEST_ASSERT_EQUAL_INT(1, SstNumFeeds);
    TEST_ASSERT_EQUAL_INT(100, SstFeeds[0].bytes);
    TEST_ASSERT_EQUAL_MEMORY(pInner + 0x10 + SST_STRM_HDR + 8, SstFeeds[0].head, 8);
    sound_Callback(50, SstFeedFn);
    TEST_ASSERT_EQUAL_INT(2, SstNumFeeds);
    TEST_ASSERT_EQUAL_INT(100, SstFeeds[1].bytes);
    TEST_ASSERT_EQUAL_MEMORY(pInner + 0x10 + SST_STRM_HDR + 8 + 100, SstFeeds[1].head, 8);

    free(pInner);
    free(pPacket0);
    free(pPacket1);
}

TEST(SmushSoundEngine, NullReturnsZero)
{
    TEST_ASSERT_EQUAL_INT(0, sound_Null());
}

TEST_GROUP_RUNNER(SmushSoundEngine)
{
    RUN_TEST_CASE(SmushSoundEngine, RegisterPublishesModuleAndInterface);
    RUN_TEST_CASE(SmushSoundEngine, StartupOpensOneTrackAndStartsDriver);
    RUN_TEST_CASE(SmushSoundEngine, StartupReportsMissingServices);
    RUN_TEST_CASE(SmushSoundEngine, StartupSurvivesRingBufferAllocationFailure);
    RUN_TEST_CASE(SmushSoundEngine, StartupFailsWhenNoRingBufferCanBeAllocated);
    RUN_TEST_CASE(SmushSoundEngine, StartupFailsWhenFadeBufferCannotBeAllocated);
    RUN_TEST_CASE(SmushSoundEngine, ShutdownReleasesServicesAndMemory);
    RUN_TEST_CASE(SmushSoundEngine, CloseReturnsNumberOfCallbacks);
    RUN_TEST_CASE(SmushSoundEngine, CallbackAdvancesTimerBySampleCount);
    RUN_TEST_CASE(SmushSoundEngine, SyncTimerCompensatesForClockDrift);
    RUN_TEST_CASE(SmushSoundEngine, PlayVocFeedsSamplesToMixer);
    RUN_TEST_CASE(SmushSoundEngine, EmptyVocStopsTheTrack);
    RUN_TEST_CASE(SmushSoundEngine, PlayIgnoredWhenDriverDidNotStart);
    RUN_TEST_CASE(SmushSoundEngine, PlayIgnoresUnknownTag);
    RUN_TEST_CASE(SmushSoundEngine, StopAllClearsEveryTrack);
    RUN_TEST_CASE(SmushSoundEngine, PauseSuppressesFeedUntilResumed);
    RUN_TEST_CASE(SmushSoundEngine, MasterVolumeAndRateGoThroughDriver);
    RUN_TEST_CASE(SmushSoundEngine, ControlAndQueryVariables);
    RUN_TEST_CASE(SmushSoundEngine, DisabledCategoryIsNotFed);
    RUN_TEST_CASE(SmushSoundEngine, EffectsDuckMusic);
    RUN_TEST_CASE(SmushSoundEngine, PlaySaudStreamUsesCallerBuffer);
    RUN_TEST_CASE(SmushSoundEngine, StreamFormatFollowsChunkTag);
    RUN_TEST_CASE(SmushSoundEngine, MultiPartPsadStreamIsAssembledInRing);
    RUN_TEST_CASE(SmushSoundEngine, NullReturnsZero);
}

// ---------------------------------------------------------------------------------------------------------------------
// Mixer tests
// ---------------------------------------------------------------------------------------------------------------------

TEST_GROUP(SmushSoundMixer);

#define SST_MIX_FRAMES 64
static int16_t SstMixOut[SST_MIX_FRAMES * 2];
static uint32_t SstSrcStore[64]; // 256 bytes, 4-byte aligned
#define SstSrc ((uint8_t*)SstSrcStore)

typedef enum { SST_M8, SST_S8, SST_M16, SST_S16, SST_MU16, SST_SU16, SST_MSW, SST_SSW } SstKind;

TEST_SETUP(SmushSoundMixer)
{
    SstMemReset();
    TEST_ASSERT_EQUAL_INT(0, mixer_Init(&SstMem.iface, 4, 0x400, 0, 1, 1));

    // Deterministic source bytes
    uint32_t x = 0x12345678;
    for ( int i = 0; i < 256; i++ )
    {
        x ^= x << 13;
        x ^= x >> 17;
        x ^= x << 5;
        SstSrc[i] = (uint8_t)(x >> 11);
    }

    memset(SstMixOut, 0x55, sizeof(SstMixOut));
}

TEST_TEAR_DOWN(SmushSoundMixer)
{
    mixer_Done(&SstMem.iface);
}

static int SstClip(int v)
{
    v >>= 7;
    return v < -0x8000 ? -0x8000 : (v > 0x7FFF ? 0x7FFF : v);
}

// The value an element of the source contributes before the gain
static int SstRef(SstKind kind, const uint8_t* pSrc, int idx)
{
    switch ( kind )
    {
        case SST_M8:
        case SST_S8:
            return (pSrc[idx] ^ 0x80) << 8;

        case SST_M16:
        case SST_S16:
            return ((const int16_t*)pSrc)[idx];

        case SST_MU16:
        case SST_SU16:
            return (int16_t)(((const uint16_t*)pSrc)[idx] ^ 0x8000);

        default:
        {
            uint16_t v = ((const uint16_t*)pSrc)[idx];
            return ((v & 0xFF) << 8) | (v >> 8);
        }
    }
}

static int SstIsStereo(SstKind kind)
{
    return kind == SST_S8 || kind == SST_S16 || kind == SST_SU16 || kind == SST_SSW;
}

static int SstIs8(SstKind kind)
{
    return kind == SST_M8 || kind == SST_S8;
}

// Expected accumulator values of one kernel on a cleared buffer: out[2 * frame + channel] before the final shift and clip
static void SstExpect(SstKind kind, const uint8_t* pSrc, int count, int n, int left, int right, int mult, int* pOut)
{
    int stereo = SstIsStereo(kind);
    uint32_t step = ((uint32_t)count << 16) / (uint32_t)n;
    if ( step == 0x8000 )
    {
        // Doubling: each source sample is followed by the mean with its neighbour; the mono kernels take the sample two ahead
        // instead of the next one (TODO: [BUG] preserved from the original), the last sample is repeated.
        for ( int f = 0; f < n; f++ )
        {
            int k = f / 2;
            int bMean = (f & 1) && k < count - 1;
            for ( int c = 0; c < (stereo ? 2 : 1); c++ )
            {
                int idx = stereo ? 2 * k + c : k;
                int idxNext = stereo ? 2 * k + 2 + c : k + 2;
                int v;
                if ( !bMean ) {
                    v = SstRef(kind, pSrc, idx);
                }
                else if ( SstIs8(kind) ) {
                    v = ((pSrc[idx] + pSrc[idxNext]) << 7) ^ 0x8000;
                }
                else {
                    v = (SstRef(kind, pSrc, idx) + SstRef(kind, pSrc, idxNext)) >> 1;
                }

                if ( stereo ) {
                    pOut[2 * f + c] = v * mult;
                }
                else
                {
                    pOut[2 * f] = v * left;
                    pOut[2 * f + 1] = v * right;
                }
            }
        }

        return;
    }

    // Other ratios: a 16.16 fixed point walk through the source (stereo kernels advance two elements per frame at the unit ratio)
    uint32_t frac = 0;
    int idx = 0;
    uint32_t stepEl = stereo ? step << 1 : step;
    for ( int f = 0; f < n; f++ )
    {
        if ( stereo )
        {
            pOut[2 * f] = SstRef(kind, pSrc, idx) * mult;
            pOut[2 * f + 1] = SstRef(kind, pSrc, idx + 1) * mult;
        }
        else
        {
            int v = SstRef(kind, pSrc, idx);
            pOut[2 * f] = v * left;
            pOut[2 * f + 1] = v * right;
        }

        frac += stepEl;
        idx += (int)frac >> 16;
        frac &= 0xFFFF;
    }
}

// Mixes through mixer_Add and compares with the model. Volume 127 and pan 63 (centre) give the gains (102, 85) for mono.
static void SstCheckKernel(SstKind kind, int count, int n)
{
    int bits = SstIs8(kind) ? 8 : 16;
    int channels = SstIsStereo(kind) ? 2 : 1;
    int isSigned = !(kind == SST_MU16 || kind == SST_SU16);
    int isSwapped = kind == SST_MSW || kind == SST_SSW;
    int volume = channels == 1 ? 127 : 100;
    int mult = volume + 1;

    mixer_Begin(n);
    mixer_Add(SstSrc, (uint32_t)count, bits, channels, (uint32_t)n, 0, volume, 63, isSigned, isSwapped);
    TEST_ASSERT_EQUAL_INT(0, mixer_Mix(SstMixOut, n));

    int expect[SST_MIX_FRAMES * 2];
    SstExpect(kind, SstSrc, count, n, 102, 85, mult, expect);
    for ( int i = 0; i < n * 2; i++ )
    {
        char msg[64];
        sprintf_s(msg, sizeof(msg), "kernel %d count %d n %d index %d", (int)kind, count, n, i);
        TEST_ASSERT_EQUAL_INT_MESSAGE(SstClip(expect[i]), SstMixOut[i], msg);
    }
}

TEST(SmushSoundMixer, InitAllocatesBuffersAndFreesThem)
{
    TEST_ASSERT_EQUAL_INT(2, SstMem.numAllocs);
    TEST_ASSERT_EQUAL_INT(0x4000, SstMem.sizes[0]); // The accumulation buffer (the size is multiplied twice by 4)
    TEST_ASSERT_EQUAL_INT(4 << 16, SstMem.sizes[1]);
    mixer_Done(&SstMem.iface);
    TEST_ASSERT_EQUAL_INT(2, SstMem.numFrees);
    TEST_ASSERT_EQUAL_INT(0, mixer_Free(&SstMem.iface)); // Freeing twice is harmless
    TEST_ASSERT_EQUAL_INT(2, SstMem.numFrees);

    // Initialise again so that the tear down finds something to free
    SstMemReset();
    TEST_ASSERT_EQUAL_INT(0, mixer_Init(&SstMem.iface, 4, 0x400, 0, 1, 1));
}

TEST(SmushSoundMixer, SetupReportsAllocationFailures)
{
    mixer_Done(&SstMem.iface);

    SstMemReset();
    SstMem.failFrom = 1;
    TEST_ASSERT_EQUAL_INT(-1, mixer_Init(&SstMem.iface, 4, 0x400, 0, 1, 1));

    SstMemReset();
    SstMem.failFrom = 2;
    TEST_ASSERT_EQUAL_INT(-1, mixer_Init(&SstMem.iface, 4, 0x400, 0, 1, 1));
    mixer_Done(&SstMem.iface);

    SstMemReset();
    TEST_ASSERT_EQUAL_INT(0, mixer_Init(&SstMem.iface, 4, 0x400, 0, 1, 1));
}

TEST(SmushSoundMixer, MixRejectsEmptyArguments)
{
    TEST_ASSERT_EQUAL_INT(-1, mixer_Mix(NULL, 4));
    TEST_ASSERT_EQUAL_INT(-1, mixer_Mix(SstMixOut, 0));
    TEST_ASSERT_EQUAL_INT(0, mixer_Mix(SstMixOut, 4));
}

TEST(SmushSoundMixer, ClearedBufferMixesToSilence)
{
    mixer_Feed(SstSrc, 0, 4, 4, 1, 127, 0, 0);
    TEST_ASSERT_EQUAL_INT(0, mixer_Begin(4));
    TEST_ASSERT_EQUAL_INT(0, mixer_Mix(SstMixOut, 8));
    for ( int i = 0; i < 16; i++ ) {
        TEST_ASSERT_EQUAL_INT(0, SstMixOut[i]);
    }
}

TEST(SmushSoundMixer, PanTableSetsChannelGains)
{
    // Mono 8-bit, 4 samples, full volume, centred (pan 0 -> 63): gains 102 (left) and 85 (right)
    static const uint8_t src[4] = { 0x00, 0x80, 0xFF, 0x7F };
    mixer_Begin(4);
    mixer_Feed((void*)src, 0, 4, 4, 1, 127, 0, 0);
    TEST_ASSERT_EQUAL_INT(0, mixer_Mix(SstMixOut, 4));
    TEST_ASSERT_EQUAL_INT(26112, SstMixOut[0]); // 0x8000 * 102 >> 7
    TEST_ASSERT_EQUAL_INT(21760, SstMixOut[1]); // 0x8000 * 85 >> 7
    TEST_ASSERT_EQUAL_INT(0, SstMixOut[2]);
    TEST_ASSERT_EQUAL_INT(0, SstMixOut[3]);
    TEST_ASSERT_EQUAL_INT(25908, SstMixOut[4]);
    TEST_ASSERT_EQUAL_INT(21590, SstMixOut[5]);
    TEST_ASSERT_EQUAL_INT(32767, SstMixOut[6]); // Clipped
    TEST_ASSERT_EQUAL_INT(32767, SstMixOut[7]);

    // Hard left (pan -127 -> 0): gains (131, 4)
    mixer_Begin(1);
    mixer_Feed((void*)src, 0, 1, 1, 1, 127, -127, 0);
    mixer_Mix(SstMixOut, 1);
    TEST_ASSERT_EQUAL_INT(32767, SstMixOut[0]);
    TEST_ASSERT_EQUAL_INT(SstClip(0x8000 * 4), SstMixOut[1]);

    // Volume 0 still leaves the gain 4 of the table's first row
    mixer_Begin(1);
    mixer_Feed((void*)src, 0, 1, 1, 1, 0, 0, 0);
    mixer_Mix(SstMixOut, 1);
    TEST_ASSERT_EQUAL_INT(SstClip(0x8000 * 4), SstMixOut[0]);
    TEST_ASSERT_EQUAL_INT(SstClip(0x8000 * 4), SstMixOut[1]);
}

TEST(SmushSoundMixer, FeedOffsetSkipsFrames)
{
    static const uint8_t src[2] = { 0x00, 0x00 };
    mixer_Begin(5);
    mixer_Feed((void*)src, 3, 2, 2, 1, 127, 0, 0);
    mixer_Mix(SstMixOut, 5);
    for ( int i = 0; i < 6; i++ ) {
        TEST_ASSERT_EQUAL_INT(0, SstMixOut[i]);
    }

    TEST_ASSERT_EQUAL_INT(26112, SstMixOut[6]);
    TEST_ASSERT_EQUAL_INT(21760, SstMixOut[7]);
    TEST_ASSERT_EQUAL_INT(26112, SstMixOut[8]);
    TEST_ASSERT_EQUAL_INT(21760, SstMixOut[9]);
}

TEST(SmushSoundMixer, FeedFormatsSelectKernels)
{
    // format 4: mono signed 16-bit, 2 samples
    static const int16_t s16[2] = { 1000, -2000 };
    mixer_Begin(2);
    mixer_Feed((void*)s16, 0, 2, 4, 1, 127, 0, 4);
    mixer_Mix(SstMixOut, 2);
    TEST_ASSERT_EQUAL_INT(SstClip(1000 * 102), SstMixOut[0]);
    TEST_ASSERT_EQUAL_INT(SstClip(1000 * 85), SstMixOut[1]);
    TEST_ASSERT_EQUAL_INT(SstClip(-2000 * 102), SstMixOut[2]);

    // format 5: stereo signed 16-bit: multiplier volume + 1
    static const int16_t st[4] = { 1000, -2000, 3000, 4000 };
    mixer_Begin(2);
    mixer_Feed((void*)st, 0, 2, 8, 1, 63, 0, 5);
    mixer_Mix(SstMixOut, 2);
    TEST_ASSERT_EQUAL_INT(SstClip(1000 * 64), SstMixOut[0]);
    TEST_ASSERT_EQUAL_INT(SstClip(-2000 * 64), SstMixOut[1]);
    TEST_ASSERT_EQUAL_INT(SstClip(3000 * 64), SstMixOut[2]);
    TEST_ASSERT_EQUAL_INT(SstClip(4000 * 64), SstMixOut[3]);

    // format 2: mono "unsigned" 16-bit (sign bit flipped)
    static const uint16_t u16[2] = { 0x8000, 0x0000 };
    mixer_Begin(2);
    mixer_Feed((void*)u16, 0, 2, 4, 1, 127, 0, 2);
    mixer_Mix(SstMixOut, 2);
    TEST_ASSERT_EQUAL_INT(0, SstMixOut[0]);
    TEST_ASSERT_EQUAL_INT(SstClip(-32768 * 102), SstMixOut[2]);
}

// 1:1, 2:1, 5:8 and 7:3 (the generic walk) resampling ratios
#define SST_RATIO_CASES(kind)   \
    do {                        \
        SstCheckKernel(kind, 8, 8);  \
        SstCheckKernel(kind, 16, 8); \
        SstCheckKernel(kind, 5, 8);  \
        SstCheckKernel(kind, 7, 3);  \
    } while ( 0 )

TEST(SmushSoundMixer, Mono8Ratios)
{
    SST_RATIO_CASES(SST_M8);
    SstCheckKernel(SST_M8, 4, 8); // Doubling with interpolation
}

TEST(SmushSoundMixer, Stereo8Ratios)
{
    SST_RATIO_CASES(SST_S8);
    SstCheckKernel(SST_S8, 4, 8);
}

TEST(SmushSoundMixer, Mono16Ratios)
{
    SST_RATIO_CASES(SST_M16);
    SstCheckKernel(SST_M16, 4, 8);
}

TEST(SmushSoundMixer, Stereo16Ratios)
{
    SST_RATIO_CASES(SST_S16);
    SstCheckKernel(SST_S16, 4, 8);
}

TEST(SmushSoundMixer, MonoU16Ratios)
{
    SST_RATIO_CASES(SST_MU16);
    SstCheckKernel(SST_MU16, 4, 8);
}

TEST(SmushSoundMixer, StereoU16Ratios)
{
    SST_RATIO_CASES(SST_SU16);
    SstCheckKernel(SST_SU16, 4, 8);
}

TEST(SmushSoundMixer, MonoSwap16Ratios)
{
    // The doubling path of the swapped kernels converts through a signed 16-bit value and is not modelled here
    SstCheckKernel(SST_MSW, 8, 8);
    SstCheckKernel(SST_MSW, 16, 8);
    SstCheckKernel(SST_MSW, 5, 8);
    SstCheckKernel(SST_MSW, 7, 3);
}

TEST(SmushSoundMixer, StereoSwap16Ratios)
{
    SstCheckKernel(SST_SSW, 16, 8);
    SstCheckKernel(SST_SSW, 5, 8);
    SstCheckKernel(SST_SSW, 7, 3);
}

TEST(SmushSoundMixer, StereoSwap16UnitRatioReadsFirstSampleOnly)
{
    // The retail kernel for the 1:1 case never advances its source pointer (it is not reachable through mixer_Feed)
    static const uint16_t src[8] = { 0x0102, 0x0304, 0x0506, 0x0708, 0, 0, 0, 0 };
    mixer_Begin(4);
    mixer_Add((void*)src, 4, 16, 2, 4, 0, 99, 63, 1, 1);
    mixer_Mix(SstMixOut, 4);
    for ( int i = 0; i < 8; i++ ) {
        TEST_ASSERT_EQUAL_INT(SstClip(0x0201 * 100), SstMixOut[i]);
    }
}

TEST(SmushSoundMixer, AddIgnoresEmptyInput)
{
    mixer_Begin(4);
    mixer_Add(NULL, 4, 8, 1, 4, 0, 127, 63, 1, 0);
    mixer_Add(SstSrc, 0, 8, 1, 4, 0, 127, 63, 1, 0);
    mixer_Mix(SstMixOut, 4);
    for ( int i = 0; i < 8; i++ ) {
        TEST_ASSERT_EQUAL_INT(0, SstMixOut[i]);
    }
}

TEST(SmushSoundMixer, SourcesAccumulate)
{
    static const uint8_t src[2] = { 0x00, 0x00 };
    mixer_Begin(2);
    mixer_Feed((void*)src, 0, 2, 2, 1, 127, 0, 0);
    mixer_Feed((void*)src, 0, 2, 2, 1, 127, 0, 0);
    mixer_Mix(SstMixOut, 2);
    TEST_ASSERT_EQUAL_INT(SstClip(2 * 0x8000 * 102), SstMixOut[0]);
    TEST_ASSERT_EQUAL_INT(SstClip(2 * 0x8000 * 85), SstMixOut[1]);
}

TEST(SmushSoundMixer, BlitOutsideClipDrawsNothing)
{
    uint8_t dstPixels[16];
    uint8_t srcPixels[16];
    memset(dstPixels, 0x11, sizeof(dstPixels));
    memset(srcPixels, 0x22, sizeof(srcPixels));

    LecBitmapHeader dst;
    LecBitmapHeader src;
    memset(&dst, 0, sizeof(dst));
    memset(&src, 0, sizeof(src));
    dst.pPixels = dstPixels; dst.width = 4; dst.height = 4; dst.pixelSize = 1; dst.bpp = 8; dst.pitch = 4;
    src.pPixels = srcPixels; src.width = 4; src.height = 4; src.pixelSize = 1; src.bpp = 8; src.pitch = 4;

    mixer_Blit(&dst, NULL, 4, 0, &src, NULL, NULL);  // Entirely right of the destination
    mixer_Blit(&dst, NULL, -4, 0, &src, NULL, NULL); // Entirely left
    mixer_Blit(&dst, NULL, 0, 4, &src, NULL, NULL);  // Below
    MixerRect clip = { 1, 1, 0, 2 };
    mixer_Blit(&dst, &clip, 0, 0, &src, NULL, NULL); // Empty clip rectangle
    for ( int i = 0; i < 16; i++ ) {
        TEST_ASSERT_EQUAL_UINT8(0x11, dstPixels[i]);
    }
}

TEST_GROUP_RUNNER(SmushSoundMixer)
{
    RUN_TEST_CASE(SmushSoundMixer, InitAllocatesBuffersAndFreesThem);
    RUN_TEST_CASE(SmushSoundMixer, SetupReportsAllocationFailures);
    RUN_TEST_CASE(SmushSoundMixer, MixRejectsEmptyArguments);
    RUN_TEST_CASE(SmushSoundMixer, ClearedBufferMixesToSilence);
    RUN_TEST_CASE(SmushSoundMixer, PanTableSetsChannelGains);
    RUN_TEST_CASE(SmushSoundMixer, FeedOffsetSkipsFrames);
    RUN_TEST_CASE(SmushSoundMixer, FeedFormatsSelectKernels);
    RUN_TEST_CASE(SmushSoundMixer, Mono8Ratios);
    RUN_TEST_CASE(SmushSoundMixer, Stereo8Ratios);
    RUN_TEST_CASE(SmushSoundMixer, Mono16Ratios);
    RUN_TEST_CASE(SmushSoundMixer, Stereo16Ratios);
    RUN_TEST_CASE(SmushSoundMixer, MonoU16Ratios);
    RUN_TEST_CASE(SmushSoundMixer, StereoU16Ratios);
    RUN_TEST_CASE(SmushSoundMixer, MonoSwap16Ratios);
    RUN_TEST_CASE(SmushSoundMixer, StereoSwap16Ratios);
    RUN_TEST_CASE(SmushSoundMixer, StereoSwap16UnitRatioReadsFirstSampleOnly);
    RUN_TEST_CASE(SmushSoundMixer, AddIgnoresEmptyInput);
    RUN_TEST_CASE(SmushSoundMixer, SourcesAccumulate);
    RUN_TEST_CASE(SmushSoundMixer, BlitOutsideClipDrawsNothing);
}

// ---------------------------------------------------------------------------------------------------------------------
// Dsnd tests (mock IDirectSound / IDirectSoundBuffer, no real audio device)
// ---------------------------------------------------------------------------------------------------------------------

TEST_GROUP(SmushSoundDsnd);

#define SST_BLOCK 0x1800

typedef struct SstDsBuffer
{
    IDirectSoundBuffer base;
    uint8_t mem[16 * SST_BLOCK];
    int numLock;
    DWORD lockOffset;
    DWORD lockBytes;
    DWORD splitAt; // Non-zero: Lock returns two regions, the first of this size
    int numUnlock;
    void* unlockPtr1;
    DWORD unlockLen1;
    void* unlockPtr2;
    DWORD unlockLen2;
    int numPlay;
    DWORD playFlags;
    int numSetVolume;
    LONG lastVolume;
    int numRelease;
    DWORD playCursor;
    DWORD writeCursor;
    HRESULT positionResult;
} SstDsBuffer;

typedef struct SstDs
{
    IDirectSound base;
    int numCreateBuffer;
    DWORD descFlags;
    DWORD descBytes;
    WAVEFORMATEX format;
    int hasFormat;
    int numRelease;
} SstDs;

static SstDsBuffer SstBuf;
static SstDs SstDS;

static ULONG STDMETHODCALLTYPE SstBufRelease(IDirectSoundBuffer* This)
{
    J3D_UNUSED(This);
    SstBuf.numRelease++;
    return 0;
}

static HRESULT STDMETHODCALLTYPE SstBufGetCurrentPosition(IDirectSoundBuffer* This, LPDWORD pdwPlay, LPDWORD pdwWrite)
{
    J3D_UNUSED(This);
    if ( SstBuf.positionResult != DS_OK ) {
        return SstBuf.positionResult;
    }

    *pdwPlay = SstBuf.playCursor;
    *pdwWrite = SstBuf.writeCursor;
    return DS_OK;
}

static HRESULT STDMETHODCALLTYPE SstBufLock(IDirectSoundBuffer* This, DWORD offset, DWORD bytes, LPVOID* ppv1, LPDWORD pLen1, LPVOID* ppv2, LPDWORD pLen2, DWORD flags)
{
    J3D_UNUSED(This); J3D_UNUSED(flags);
    SstBuf.numLock++;
    SstBuf.lockOffset = offset;
    SstBuf.lockBytes = bytes;
    *ppv1 = SstBuf.mem + offset;
    if ( SstBuf.splitAt && SstBuf.splitAt < bytes )
    {
        *pLen1 = SstBuf.splitAt;
        *ppv2 = SstBuf.mem;
        *pLen2 = bytes - SstBuf.splitAt;
    }
    else
    {
        *pLen1 = bytes;
        *ppv2 = NULL;
        *pLen2 = 0;
    }

    return DS_OK;
}

static HRESULT STDMETHODCALLTYPE SstBufUnlock(IDirectSoundBuffer* This, LPVOID pv1, DWORD len1, LPVOID pv2, DWORD len2)
{
    J3D_UNUSED(This);
    SstBuf.numUnlock++;
    SstBuf.unlockPtr1 = pv1;
    SstBuf.unlockLen1 = len1;
    SstBuf.unlockPtr2 = pv2;
    SstBuf.unlockLen2 = len2;
    return DS_OK;
}

static HRESULT STDMETHODCALLTYPE SstBufPlay(IDirectSoundBuffer* This, DWORD reserved1, DWORD priority, DWORD flags)
{
    J3D_UNUSED(This); J3D_UNUSED(reserved1); J3D_UNUSED(priority);
    SstBuf.numPlay++;
    SstBuf.playFlags = flags;
    return DS_OK;
}

static HRESULT STDMETHODCALLTYPE SstBufSetVolume(IDirectSoundBuffer* This, LONG volume)
{
    J3D_UNUSED(This);
    SstBuf.numSetVolume++;
    SstBuf.lastVolume = volume;
    return DS_OK;
}

static IDirectSoundBufferVtbl SstBufVtbl;

static ULONG STDMETHODCALLTYPE SstDsRelease(IDirectSound* This)
{
    J3D_UNUSED(This);
    SstDS.numRelease++;
    return 0;
}

static HRESULT STDMETHODCALLTYPE SstDsCreateSoundBuffer(IDirectSound* This, LPCDSBUFFERDESC pDesc, LPDIRECTSOUNDBUFFER* ppBuffer, LPUNKNOWN pOuter)
{
    J3D_UNUSED(This); J3D_UNUSED(pOuter);
    SstDS.numCreateBuffer++;
    SstDS.descFlags = pDesc->dwFlags;
    SstDS.descBytes = pDesc->dwBufferBytes;
    SstDS.hasFormat = pDesc->lpwfxFormat != NULL;
    if ( pDesc->lpwfxFormat ) {
        memcpy(&SstDS.format, pDesc->lpwfxFormat, sizeof(WAVEFORMATEX));
    }

    memset(SstBuf.mem, 0xAA, sizeof(SstBuf.mem));
    *ppBuffer = &SstBuf.base;
    return DS_OK;
}

static IDirectSoundVtbl SstDsVtbl;

static void SstDsReset(void)
{
    memset(&SstBuf, 0, sizeof(SstBuf));
    memset(&SstDS, 0, sizeof(SstDS));
    memset(&SstBufVtbl, 0, sizeof(SstBufVtbl));
    memset(&SstDsVtbl, 0, sizeof(SstDsVtbl));
    SstBufVtbl.Release = SstBufRelease;
    SstBufVtbl.GetCurrentPosition = SstBufGetCurrentPosition;
    SstBufVtbl.Lock = SstBufLock;
    SstBufVtbl.Unlock = SstBufUnlock;
    SstBufVtbl.Play = SstBufPlay;
    SstBufVtbl.SetVolume = SstBufSetVolume;
    SstDsVtbl.Release = SstDsRelease;
    SstDsVtbl.CreateSoundBuffer = SstDsCreateSoundBuffer;
    SstBuf.base.lpVtbl = &SstBufVtbl;
    SstDS.base.lpVtbl = &SstDsVtbl;
}

static LecModule* SstDsndModule;
static SndDriver* SstDsndDrv;
static int SstDsndStarted;
static int SstFillCalls;
static int SstFillChunk;
static SoundFeed SstFillFeed;
static int SstFillOffset;

// The engine's fill callback of the tests: feeds a known 4-sample block at the given frame offset
static void SstFillFn(int n, SoundFeed pfnFeed)
{
    static const uint8_t src[4] = { 0x00, 0x80, 0xFF, 0x7F };
    SstFillCalls++;
    SstFillChunk = n;
    SstFillFeed = pfnFeed;
    pfnFeed((void*)src, SstFillOffset, 4, 4, 1, 127, 0, 0);
}

TEST_SETUP(SmushSoundDsnd)
{
    Dsnd_TestReset();
    SstShareReset();
    SstMemReset();
    SstTimerReset();
    SstDsReset();
    SstSh.pTimerClk = &SstTim.iface;
    SstSh.pMixerMem = &SstMem.iface;
    SstFillCalls = 0;
    SstFillChunk = 0;
    SstFillFeed = NULL;
    SstFillOffset = 0;
    SstDsndStarted = 0;
    TEST_ASSERT_EQUAL_INT(0, dsnd_Register(&SstSh.iface, NULL));
    SstDsndModule = SstSharePublished("LECMODULE=insane_dsnd");
    SstDsndDrv = SstSharePublished("SNDDRIVER=insane_dsnd");
    TEST_ASSERT_NOT_NULL(SstDsndModule);
    TEST_ASSERT_NOT_NULL(SstDsndDrv);
}

TEST_TEAR_DOWN(SmushSoundDsnd)
{
    if ( SstDsndStarted ) {
        SstDsndDrv->Stop();
    }

    SstDsndModule->Shutdown();
    SstDsndModule->Unregister();
    Dsnd_TestReset();
}

static void SstDsndStart(void)
{
    TEST_ASSERT_EQUAL_INT(0, dsnd_Configure(5, &SstDS.base, 0));
    TEST_ASSERT_EQUAL_INT(0, SstDsndModule->Startup());
    TEST_ASSERT_EQUAL_INT(0, SstDsndDrv->Start(22050, 1, SstFillFn));
    SstDsndStarted = 1;
}

TEST(SmushSoundDsnd, RegisterPublishesModuleAndDriver)
{
    TEST_ASSERT_EQUAL_INT(2, SstSh.numPublished);
    TEST_ASSERT_EQUAL_STRING("LECMODULE=insane_dsnd", SstSh.aPublishedNames[0]);
    TEST_ASSERT_EQUAL_STRING("SNDDRIVER=insane_dsnd", SstSh.aPublishedNames[1]);
    TEST_ASSERT_TRUE(SstDsndModule->Register == dsnd_Register);
    TEST_ASSERT_TRUE(SstDsndModule->Unregister == dsnd_Unregister);
    TEST_ASSERT_TRUE(SstDsndModule->Startup == dsnd_Startup);
    TEST_ASSERT_TRUE(SstDsndModule->Shutdown == dsnd_Shutdown);
    TEST_ASSERT_TRUE(SstDsndDrv->Start == dsnd_Start);
    TEST_ASSERT_TRUE(SstDsndDrv->Stop == dsnd_Stop);
    TEST_ASSERT_TRUE(SstDsndDrv->SetVolume == dsnd_SetVolume);
    TEST_ASSERT_TRUE(SstDsndDrv->GetVolume == dsnd_GetVolume);
    TEST_ASSERT_TRUE(SstDsndDrv->GetRate == dsnd_GetRate);
    TEST_ASSERT_TRUE(SstDsndDrv->SetRate == (int (*)(int))sound_Null);
    TEST_ASSERT_EQUAL_INT(0, SstDsndDrv->SetRate(11025));
    TEST_ASSERT_EQUAL_INT(22050, SstDsndDrv->GetRate());
}

TEST(SmushSoundDsnd, ConfigureAcceptsKnownTagsOnly)
{
    TEST_ASSERT_EQUAL_INT(0, dsnd_Configure(0));
    TEST_ASSERT_EQUAL_INT(0, dsnd_Configure(1, 10, 2, 20, 3, 30, 4, 40, 0));
    TEST_ASSERT_EQUAL_INT(-1, dsnd_Configure(9, 1, 0));
}

TEST(SmushSoundDsnd, StartupSubscribesToServices)
{
    TEST_ASSERT_EQUAL_INT(0, SstDsndModule->Startup());
    TEST_ASSERT_EQUAL_STRING("TIMER=clock", SstSh.aSubscribeNames[0]);
    TEST_ASSERT_EQUAL_STRING("MEMORY.mixertable", SstSh.aSubscribeNames[1]);
    SstDsndModule->Shutdown();
    TEST_ASSERT_EQUAL_INT(2, SstSh.numUnsubscribed);
    TEST_ASSERT_TRUE(SstSh.aUnsubscribed[0] == &SstTim.iface);
    TEST_ASSERT_TRUE(SstSh.aUnsubscribed[1] == &SstMem.iface);
}

TEST(SmushSoundDsnd, StartupReportsMissingServices)
{
    SstSh.pTimerClk = NULL;
    TEST_ASSERT_EQUAL_INT(-1, SstDsndModule->Startup());
    TEST_ASSERT_EQUAL_STRING("DirectSoundDriver Module: Cannot allocate timer services.", SstSh.aErrors[0]);
    SstSh.pTimerClk = &SstTim.iface;
    SstSh.pMixerMem = NULL;
    TEST_ASSERT_EQUAL_INT(-1, SstDsndModule->Startup());
    TEST_ASSERT_EQUAL_INT(2, SstSh.numErrors);
    SstSh.pMixerMem = &SstMem.iface;
    TEST_ASSERT_EQUAL_INT(0, SstDsndModule->Startup());
}

TEST(SmushSoundDsnd, StartCreatesLoopingSecondaryBufferOnGivenObject)
{
    SstDsndStart();
    TEST_ASSERT_EQUAL_INT(1, SstDS.numCreateBuffer);
    TEST_ASSERT_EQUAL_UINT32(DSBCAPS_GETCURRENTPOSITION2 | DSBCAPS_CTRLVOLUME, SstDS.descFlags);
    TEST_ASSERT_EQUAL_UINT32(3 * SST_BLOCK, SstDS.descBytes);
    TEST_ASSERT_TRUE(SstDS.hasFormat);
    TEST_ASSERT_EQUAL_UINT16(WAVE_FORMAT_PCM, SstDS.format.wFormatTag);
    TEST_ASSERT_EQUAL_UINT16(2, SstDS.format.nChannels);
    TEST_ASSERT_EQUAL_UINT32(22050, SstDS.format.nSamplesPerSec);
    TEST_ASSERT_EQUAL_UINT16(16, SstDS.format.wBitsPerSample);
    TEST_ASSERT_EQUAL_UINT16(4, SstDS.format.nBlockAlign);

    // The buffer is silenced and started looping at full volume
    TEST_ASSERT_EQUAL_INT(1, SstBuf.numLock);
    TEST_ASSERT_EQUAL_UINT32(0, SstBuf.lockOffset);
    TEST_ASSERT_EQUAL_UINT32(3 * SST_BLOCK, SstBuf.lockBytes);
    TEST_ASSERT_EQUAL_INT(1, SstBuf.numUnlock);
    for ( int i = 0; i < 3 * SST_BLOCK; i++ )
    {
        if ( SstBuf.mem[i] != 0 ) {
            TEST_FAIL_MESSAGE("secondary buffer not silenced");
        }
    }

    TEST_ASSERT_EQUAL_UINT8(0xAA, SstBuf.mem[3 * SST_BLOCK]);
    TEST_ASSERT_EQUAL_INT(1, SstBuf.numPlay);
    TEST_ASSERT_EQUAL_UINT32(DSBPLAY_LOOPING, SstBuf.playFlags);
    TEST_ASSERT_EQUAL_INT(1, SstBuf.numSetVolume);
    TEST_ASSERT_EQUAL_INT32(0, SstBuf.lastVolume);

    // The mixer got its buffers and the timer a callback of 1193180 / (22050 / 1024) ticks
    TEST_ASSERT_EQUAL_INT(2, SstMem.numAllocs);
    TEST_ASSERT_EQUAL_INT(1, SstTim.numAllocs);
    TEST_ASSERT_TRUE(SstTim.pfnAlloc == dsnd_Callback);
    TEST_ASSERT_EQUAL_INT(1193180 / 21, SstTim.allocInterval);
    TEST_ASSERT_EQUAL_UINT32(2, SstTim.allocFlags);
    TEST_ASSERT_EQUAL_INT(0, SstTim.allocParam);
}

TEST(SmushSoundDsnd, StartRaisesTooLowRate)
{
    TEST_ASSERT_EQUAL_INT(0, dsnd_Configure(5, &SstDS.base, 0));
    TEST_ASSERT_EQUAL_INT(0, SstDsndModule->Startup());
    TEST_ASSERT_EQUAL_INT(0, SstDsndDrv->Start(8000, 1, SstFillFn));
    SstDsndStarted = 1;
    TEST_ASSERT_EQUAL_INT(1193180 / 21, SstTim.allocInterval);
}

TEST(SmushSoundDsnd, StartFailsWhenMixerCannotAllocate)
{
    SstMem.failFrom = 1;
    TEST_ASSERT_EQUAL_INT(0, dsnd_Configure(5, &SstDS.base, 0));
    TEST_ASSERT_EQUAL_INT(0, SstDsndModule->Startup());
    TEST_ASSERT_EQUAL_INT(-1, SstDsndDrv->Start(22050, 1, SstFillFn));
    TEST_ASSERT_EQUAL_INT(0, SstTim.numAllocs);
    TEST_ASSERT_EQUAL_INT(1, SstDS.numCreateBuffer); // The secondary buffer was created before the mixer failed
    dsnd_Release();
    TEST_ASSERT_EQUAL_INT(1, SstBuf.numRelease);
}

TEST(SmushSoundDsnd, SetVolumeMapsToDecibelTable)
{
    SstDsndStart();
    static const struct { int volume; LONG expect; } aCases[] = {
        { 0, -10000 }, { 1, -7000 }, { 2, -6000 }, { 3, -5200 }, { 4, -5000 }, { 5, -4500 }, { 6, -4200 }, { 7, -4100 }, { 8, -4000 },
        { 9, 2000 / 23 - 4000 }, { 31, 46000 / 23 - 4000 }, { 32, -2000 }, { 33, 1000 / 31 - 2000 }, { 63, 31000 / 31 - 2000 },
        { 64, -1000 }, { 65, 1000 / 63 - 1000 }, { 127, 0 }, { 128, -10000 }, { 255, 0 },
    };

    for ( size_t i = 0; i < sizeof(aCases) / sizeof(aCases[0]); i++ )
    {
        dsnd_SetVolume(aCases[i].volume);
        char msg[48];
        sprintf_s(msg, sizeof(msg), "volume %d", aCases[i].volume);
        TEST_ASSERT_EQUAL_INT32_MESSAGE(aCases[i].expect, SstBuf.lastVolume, msg);
        TEST_ASSERT_EQUAL_INT_MESSAGE(aCases[i].volume & 0x7F, SstDsndDrv->GetVolume(), msg);
    }
}

// The first frame of the fill callback's block: left and right of the mono 8-bit sample 0x00 at the centre (see PanTableSetsChannelGains)
#define SST_FRAME0_L 26112
#define SST_FRAME0_R 21760

TEST(SmushSoundDsnd, CallbackMixesNextBlockIntoLockedRegion)
{
    SstDsndStart();
    int lockBefore = SstBuf.numLock;

    // Play cursor in block 1: the first callback writes block 2 (the block after the play cursor)
    SstBuf.playCursor = 1 * SST_BLOCK + 5;
    SstTim.pfnAlloc(7, 0);
    TEST_ASSERT_EQUAL_INT(lockBefore + 1, SstBuf.numLock);
    TEST_ASSERT_EQUAL_UINT32(2 * SST_BLOCK, SstBuf.lockOffset);
    TEST_ASSERT_EQUAL_UINT32(SST_BLOCK, SstBuf.lockBytes);
    TEST_ASSERT_EQUAL_INT(1, SstFillCalls);
    TEST_ASSERT_EQUAL_INT(0x600, SstFillChunk);
    TEST_ASSERT_TRUE(SstFillFeed == mixer_Feed);
    TEST_ASSERT_TRUE(SstBuf.unlockPtr1 == SstBuf.mem + 2 * SST_BLOCK);
    TEST_ASSERT_EQUAL_UINT32(SST_BLOCK, SstBuf.unlockLen1);
    TEST_ASSERT_NULL(SstBuf.unlockPtr2);

    const int16_t* pOut = (const int16_t*)(SstBuf.mem + 2 * SST_BLOCK);
    TEST_ASSERT_EQUAL_INT16(SST_FRAME0_L, pOut[0]);
    TEST_ASSERT_EQUAL_INT16(SST_FRAME0_R, pOut[1]);
    TEST_ASSERT_EQUAL_INT16(0, pOut[2]);
    TEST_ASSERT_EQUAL_INT16(32767, pOut[6]);
    for ( int i = 8; i < 0xC00; i++ ) {
        TEST_ASSERT_EQUAL_INT16(0, pOut[i]);
    }

    TEST_ASSERT_EQUAL_INT(1, SstTim.numSetInterval);
    TEST_ASSERT_EQUAL_INT(7, SstTim.lastSetIndex);
    TEST_ASSERT_EQUAL_INT(1193180 / 21, SstTim.lastSetInterval);

    // The same play position: the next block (0) is due
    SstTim.pfnAlloc(7, 0);
    TEST_ASSERT_EQUAL_UINT32(0, SstBuf.lockOffset);

    // The play cursor reaches the block after the last one written: nothing to do
    SstBuf.playCursor = 1 * SST_BLOCK;
    int locks = SstBuf.numLock;
    SstTim.pfnAlloc(7, 0);
    TEST_ASSERT_EQUAL_INT(locks, SstBuf.numLock);
    TEST_ASSERT_EQUAL_INT(3, SstTim.numSetInterval); // The interval is still re-armed
}

TEST(SmushSoundDsnd, CallbackSkipsWhenPlayPositionUnavailable)
{
    SstDsndStart();
    int locks = SstBuf.numLock;
    SstBuf.positionResult = DSERR_GENERIC;
    SstTim.pfnAlloc(7, 0);
    TEST_ASSERT_EQUAL_INT(locks, SstBuf.numLock);
    TEST_ASSERT_EQUAL_INT(0, SstFillCalls);
    TEST_ASSERT_EQUAL_INT(1, SstTim.numSetInterval);
    TEST_ASSERT_EQUAL_INT(-1, dsnd_GetPlayPos());
}

TEST(SmushSoundDsnd, CallbackCopiesWrappedRegionsThroughScratch)
{
    SstDsndStart();
    SstBuf.playCursor = 1 * SST_BLOCK;
    SstBuf.splitAt = 0x1000; // The locked block wraps: 0x1000 bytes at the offset, 0x800 bytes at the start of the buffer
    SstFillOffset = 0x400;   // Frame 0x400 is the first one of the second region (0x1000 / 4)
    SstTim.pfnAlloc(7, 0);

    TEST_ASSERT_EQUAL_INT(0x600, SstFillChunk);
    TEST_ASSERT_EQUAL_UINT32(0x1000, SstBuf.unlockLen1);
    TEST_ASSERT_EQUAL_UINT32(0x800, SstBuf.unlockLen2);
    TEST_ASSERT_TRUE(SstBuf.unlockPtr2 == SstBuf.mem);

    const int16_t* pFirst = (const int16_t*)(SstBuf.mem + 2 * SST_BLOCK);
    for ( int i = 0; i < 0x800; i++ ) {
        TEST_ASSERT_EQUAL_INT16(0, pFirst[i]);
    }

    const int16_t* pSecond = (const int16_t*)SstBuf.mem;
    TEST_ASSERT_EQUAL_INT16(SST_FRAME0_L, pSecond[0]);
    TEST_ASSERT_EQUAL_INT16(SST_FRAME0_R, pSecond[1]);
    TEST_ASSERT_EQUAL_INT16(32767, pSecond[6]);
    TEST_ASSERT_EQUAL_INT16(0, pSecond[8]);
}

TEST(SmushSoundDsnd, GetPlayPosReturnsBlockIndex)
{
    SstDsndStart();
    SstBuf.playCursor = 2 * SST_BLOCK + 100;
    TEST_ASSERT_EQUAL_INT(2, dsnd_GetPlayPos());
    SstBuf.playCursor = 5 * SST_BLOCK; // Beyond the 3 blocks: wraps by the block count
    TEST_ASSERT_EQUAL_INT(2, dsnd_GetPlayPos());
}

TEST(SmushSoundDsnd, StopFreesTimerMixerAndBuffer)
{
    SstDsndStart();
    SstDsndDrv->Stop();
    SstDsndStarted = 0;
    TEST_ASSERT_EQUAL_INT(1, SstTim.numFrees);
    TEST_ASSERT_EQUAL_INT(7, SstTim.lastFreed);
    TEST_ASSERT_EQUAL_INT(2, SstMem.numFrees);
    TEST_ASSERT_EQUAL_INT(1, SstBuf.numRelease);
    TEST_ASSERT_EQUAL_INT(0, SstDS.numRelease);   // The game owns its DirectSound object
    TEST_ASSERT_EQUAL_INT(-1, dsnd_GetPlayPos()); // No buffer any more
}

TEST(SmushSoundDsnd, ReleaseFreesDirectSoundCreatedByTheDriver_Fixed)
{
    TEST_ASSERT_EQUAL_INT(0, dsnd_Configure(5, &SstDS.base, 0));
    Dsnd_TestSetOwned(&SstBuf.base); // As after dsnd_Init created the DirectSound object and the primary buffer

    // Fix: the original cleared the ownership flag with the primary buffer and never released the DirectSound object of the driver
    dsnd_Release();
    TEST_ASSERT_EQUAL_INT(1, SstBuf.numRelease);
    TEST_ASSERT_EQUAL_INT(1, SstDS.numRelease);

    // Released only once
    dsnd_Release();
    TEST_ASSERT_EQUAL_INT(1, SstDS.numRelease);
}

TEST_GROUP_RUNNER(SmushSoundDsnd)
{
    RUN_TEST_CASE(SmushSoundDsnd, RegisterPublishesModuleAndDriver);
    RUN_TEST_CASE(SmushSoundDsnd, ConfigureAcceptsKnownTagsOnly);
    RUN_TEST_CASE(SmushSoundDsnd, StartupSubscribesToServices);
    RUN_TEST_CASE(SmushSoundDsnd, StartupReportsMissingServices);
    RUN_TEST_CASE(SmushSoundDsnd, StartCreatesLoopingSecondaryBufferOnGivenObject);
    RUN_TEST_CASE(SmushSoundDsnd, StartRaisesTooLowRate);
    RUN_TEST_CASE(SmushSoundDsnd, StartFailsWhenMixerCannotAllocate);
    RUN_TEST_CASE(SmushSoundDsnd, SetVolumeMapsToDecibelTable);
    RUN_TEST_CASE(SmushSoundDsnd, CallbackMixesNextBlockIntoLockedRegion);
    RUN_TEST_CASE(SmushSoundDsnd, CallbackSkipsWhenPlayPositionUnavailable);
    RUN_TEST_CASE(SmushSoundDsnd, CallbackCopiesWrappedRegionsThroughScratch);
    RUN_TEST_CASE(SmushSoundDsnd, GetPlayPosReturnsBlockIndex);
    RUN_TEST_CASE(SmushSoundDsnd, StopFreesTimerMixerAndBuffer);
    RUN_TEST_CASE(SmushSoundDsnd, ReleaseFreesDirectSoundCreatedByTheDriver_Fixed);
}

TEST_GROUP_RUNNER(SmushSound)
{
    RUN_TEST_GROUP(SmushSoundEngine);
    RUN_TEST_GROUP(SmushSoundMixer);
    RUN_TEST_GROUP(SmushSoundDsnd);
}

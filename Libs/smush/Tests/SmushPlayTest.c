#include <unity_fixture.h>

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#if defined(_DEBUG)
#include <crtdbg.h>
#endif

#include <std/General/stdUtil.h>

#include <smush/SmushPlay.h>
#include <smush/insane/animplay.h>
#include <smush/insane/Lec/LecShare.h>
#include <smush/insane/Sound/Dsnd.h>

#define SMUSHPLAYTEST_WIDTH       8
#define SMUSHPLAYTEST_HEIGHT      8
#define SMUSHPLAYTEST_FRAMES      3
#define SMUSHPLAYTEST_CALLBACKS   (SMUSHPLAYTEST_FRAMES + 1)
#define SMUSHPLAYTEST_FRAME_BYTES (SMUSHPLAYTEST_WIDTH * SMUSHPLAYTEST_HEIGHT * 2)
#define SMUSHPLAYTEST_CHILD_MS    5000u

static const char* smushPlayTest_apFiles[] = { "SANM.snm", "SANM.san", "ZNM.znm", "expected-rgb565.bin" };
static char smushPlayTest_aDirectory[MAX_PATH];

// These objects outlive every real mixer/timer callback until SysShutdown joins them.
static IDirectSound smushPlayTest_device;
static IDirectSoundBuffer smushPlayTest_buffer;
static IDirectSoundVtbl smushPlayTest_deviceVtable;
static IDirectSoundBufferVtbl smushPlayTest_bufferVtable;
static uint8_t smushPlayTest_aSound[3 * DSND_BLOCKSIZE];
static DWORD smushPlayTest_soundBytes;
static DWORD smushPlayTest_soundStatus;
static volatile LONG smushPlayTest_numCreates, smushPlayTest_numBufferReleases;
static volatile LONG smushPlayTest_numDeviceReleases, smushPlayTest_numSoundErrors;

static ULONG STDMETHODCALLTYPE SmushPlayTest_ReleaseBuffer(IDirectSoundBuffer* pThis)
{
    J3D_UNUSED(pThis);
    InterlockedIncrement(&smushPlayTest_numBufferReleases);
    return 0;
}

static ULONG STDMETHODCALLTYPE SmushPlayTest_ReleaseDevice(IDirectSound* pThis)
{
    J3D_UNUSED(pThis);
    InterlockedIncrement(&smushPlayTest_numDeviceReleases);
    return 0;
}

static HRESULT STDMETHODCALLTYPE SmushPlayTest_Lock(IDirectSoundBuffer* pThis, DWORD offset, DWORD size,
    LPVOID* ppFirst, LPDWORD pFirstSize, LPVOID* ppSecond, LPDWORD pSecondSize, DWORD flags)
{
    J3D_UNUSED(pThis);
    J3D_UNUSED(flags);
    if ( offset > smushPlayTest_soundBytes || size > smushPlayTest_soundBytes - offset )
    {
        InterlockedIncrement(&smushPlayTest_numSoundErrors);
        return DSERR_INVALIDPARAM;
    }
    *ppFirst = smushPlayTest_aSound + offset;
    *pFirstSize = size;
    *ppSecond = NULL;
    *pSecondSize = 0;
    return DS_OK;
}

static HRESULT STDMETHODCALLTYPE SmushPlayTest_Unlock(IDirectSoundBuffer* pThis, LPVOID pFirst, DWORD firstSize, LPVOID pSecond, DWORD secondSize)
{
    J3D_UNUSED(pThis);
    J3D_UNUSED(pFirst);
    J3D_UNUSED(firstSize);
    J3D_UNUSED(pSecond);
    J3D_UNUSED(secondSize);
    return DS_OK;
}

static HRESULT STDMETHODCALLTYPE SmushPlayTest_PlayBuffer(IDirectSoundBuffer* pThis, DWORD reserved, DWORD priority, DWORD flags)
{
    J3D_UNUSED(pThis);
    J3D_UNUSED(reserved);
    J3D_UNUSED(priority);
    J3D_UNUSED(flags);
    smushPlayTest_soundStatus = DSBSTATUS_PLAYING | DSBSTATUS_LOOPING;
    return DS_OK;
}

static HRESULT STDMETHODCALLTYPE SmushPlayTest_SetVolume(IDirectSoundBuffer* pThis, LONG volume)
{
    J3D_UNUSED(pThis);
    J3D_UNUSED(volume);
    return DS_OK;
}

static HRESULT STDMETHODCALLTYPE SmushPlayTest_GetPosition(IDirectSoundBuffer* pThis, LPDWORD pPlay, LPDWORD pWrite)
{
    J3D_UNUSED(pThis);
    *pPlay = *pWrite = 0;
    return DS_OK;
}

static HRESULT STDMETHODCALLTYPE SmushPlayTest_GetStatus(IDirectSoundBuffer* pThis, LPDWORD pStatus)
{
    J3D_UNUSED(pThis);
    *pStatus = smushPlayTest_soundStatus;
    return DS_OK;
}

static HRESULT STDMETHODCALLTYPE SmushPlayTest_CreateBuffer(IDirectSound* pThis, LPCDSBUFFERDESC pDesc,
    LPDIRECTSOUNDBUFFER* ppBuffer, LPUNKNOWN pOuter)
{
    J3D_UNUSED(pThis);
    J3D_UNUSED(pOuter);
    InterlockedIncrement(&smushPlayTest_numCreates);
    if ( !pDesc->lpwfxFormat || pDesc->dwBufferBytes > sizeof(smushPlayTest_aSound) )
    {
        InterlockedIncrement(&smushPlayTest_numSoundErrors);
        return DSERR_INVALIDPARAM;
    }
    smushPlayTest_soundBytes = pDesc->dwBufferBytes;
    *ppBuffer = &smushPlayTest_buffer;
    return DS_OK;
}

static void SmushPlayTest_ResetSound(void)
{
    memset(&smushPlayTest_deviceVtable, 0, sizeof(smushPlayTest_deviceVtable));
    memset(&smushPlayTest_bufferVtable, 0, sizeof(smushPlayTest_bufferVtable));
    smushPlayTest_deviceVtable.Release = SmushPlayTest_ReleaseDevice;
    smushPlayTest_deviceVtable.CreateSoundBuffer = SmushPlayTest_CreateBuffer;
    smushPlayTest_bufferVtable.Release = SmushPlayTest_ReleaseBuffer;
    smushPlayTest_bufferVtable.Lock = SmushPlayTest_Lock;
    smushPlayTest_bufferVtable.Unlock = SmushPlayTest_Unlock;
    smushPlayTest_bufferVtable.Play = SmushPlayTest_PlayBuffer;
    smushPlayTest_bufferVtable.SetVolume = SmushPlayTest_SetVolume;
    smushPlayTest_bufferVtable.GetCurrentPosition = SmushPlayTest_GetPosition;
    smushPlayTest_bufferVtable.GetStatus = SmushPlayTest_GetStatus;
    smushPlayTest_device.lpVtbl = &smushPlayTest_deviceVtable;
    smushPlayTest_buffer.lpVtbl = &smushPlayTest_bufferVtable;
    smushPlayTest_numCreates = smushPlayTest_numBufferReleases = 0;
    smushPlayTest_numDeviceReleases = smushPlayTest_numSoundErrors = 0;
    smushPlayTest_soundBytes = smushPlayTest_soundStatus = 0;
}

static uint8_t smushPlayTest_aExpected[SMUSHPLAYTEST_FRAMES * SMUSHPLAYTEST_FRAME_BYTES];
static int smushPlayTest_numErrors, smushPlayTest_numCallbacks, smushPlayTest_abortAfter;
static int smushPlayTest_numEnds, smushPlayTest_lastState, smushPlayTest_bStarted;
static AnimPlayTable* smushPlayTest_pPlayer;
static int (*smushPlayTest_pfPace)(AnimPlay*, void*, int, int, int, void*, int*, int*, int, int);

static int SmushPlayTest_Check(int bCondition, const char* pMessage)
{
    if ( !bCondition )
    {
        fprintf(stderr, "%s\n", pMessage);
        ++smushPlayTest_numErrors;
    }
    return bCondition;
}

static int SmushPlayTest_Pace(AnimPlay* pPlay, void* pBitmap, int a3, int a4, int a5, void* pPalette,
    int* pOutA, int* pOutB, int a9, int a10)
{
    int state = smushPlayTest_pfPace(pPlay, pBitmap, a3, a4, a5, pPalette, pOutA, pOutB, a9, a10);
    smushPlayTest_lastState = state;
    if ( state == ANIMPLAY_STATE_END )
    {
        ++smushPlayTest_numEnds;
    }
    return state;
}

static int __cdecl SmushPlayTest_Blit(const SmushBitmap* pBitmap, int frame)
{
    static const int aRgb565[] = { 5, 6, 5, 11, 5, 0, 3, 2, 3 };
    // The END tick increments the index and presents the final decoded frame once more.
    int expectedFrame = smushPlayTest_numCallbacks % SMUSHPLAYTEST_CALLBACKS;
    int pixelFrame    = expectedFrame < SMUSHPLAYTEST_FRAMES ? expectedFrame : SMUSHPLAYTEST_FRAMES - 1;
    if ( !SmushPlayTest_Check(frame == expectedFrame, "Unexpected callback frame index.")
        || !SmushPlayTest_Check(pBitmap->pPixels && pBitmap->width == SMUSHPLAYTEST_WIDTH
            && pBitmap->height == SMUSHPLAYTEST_HEIGHT && pBitmap->pixelSize == 2 && pBitmap->bpp == 16
            && pBitmap->pitch == SMUSHPLAYTEST_WIDTH * 2 && pBitmap->colorMode == STDCOLOR_RGB
            && pBitmap->unknown6 == 0 && pBitmap->unknown7 == 0, "Unexpected bitmap metadata.")
        || !SmushPlayTest_Check(memcmp(&pBitmap->format, aRgb565, sizeof(aRgb565)) == 0, "Unexpected RGB565 channel layout.") )
    {
        return 1;
    }
    const uint8_t* pExpected = smushPlayTest_aExpected + pixelFrame * SMUSHPLAYTEST_FRAME_BYTES;
    for ( int row = 0; row < SMUSHPLAYTEST_HEIGHT; ++row )
    {
        if ( !SmushPlayTest_Check(memcmp((const uint8_t*)pBitmap->pPixels + row * pBitmap->pitch,
            pExpected + row * SMUSHPLAYTEST_WIDTH * 2, SMUSHPLAYTEST_WIDTH * 2) == 0, "Decoded frame pixels differ.") )
        {
            return 1;
        }
    }
    ++smushPlayTest_numCallbacks;
    return smushPlayTest_abortAfter && smushPlayTest_numCallbacks >= smushPlayTest_abortAfter;
}

static int SmushPlayTest_Start(void)
{
    SmushPlayTest_ResetSound();
    if ( !SmushPlayTest_Check(SmushPlay_SysStartup(NULL, (tDirectSound*)&smushPlayTest_device) == 1, "Player startup failed.") )
    {
        return 0;
    }
    smushPlayTest_bStarted = 1;
    smushPlayTest_pPlayer = LecShare_Subscribe("ANIMPLAY", NULL);
    if ( !SmushPlayTest_Check(smushPlayTest_pPlayer != NULL, "Player service is missing.") )
    {
        return 0;
    }
    smushPlayTest_pfPace = smushPlayTest_pPlayer->Pace;
    smushPlayTest_pPlayer->Pace = SmushPlayTest_Pace;
    return 1;
}

static void SmushPlayTest_Stop(void)
{
    if ( smushPlayTest_pPlayer )
    {
        smushPlayTest_pPlayer->Pace = smushPlayTest_pfPace;
        LecShare_Unsubscribe(smushPlayTest_pPlayer);
        smushPlayTest_pPlayer = NULL;
    }
    if ( smushPlayTest_bStarted )
    {
        SmushPlay_SysShutdown();
        smushPlayTest_bStarted = 0;
        SmushPlayTest_Check(smushPlayTest_numCreates == 1 && smushPlayTest_numBufferReleases == 1,
            "Secondary buffer ownership is unbalanced.");
        SmushPlayTest_Check(smushPlayTest_numDeviceReleases == 0, "Borrowed DirectSound device was released.");
        SmushPlayTest_Check(smushPlayTest_numSoundErrors == 0, "Unexpected DirectSound operation.");
    }
}

static void SmushPlayTest_Movie(const char* pDirectory, const char* pName, int loops, int abortAfter,
    int expectedReturn, int expectedCallbacks, int expectedEnds)
{
    char aPath[MAX_PATH];
    snprintf(aPath, sizeof(aPath), "%s\\%s", pDirectory, pName);
    smushPlayTest_numCallbacks = smushPlayTest_numEnds = 0;
    smushPlayTest_lastState = -100;
    smushPlayTest_abortAfter = abortAfter;
    int result = SmushPlay_PlayMovie(aPath, 100, ANIMPLAY_FLAG_NOSKIP, 0, 0, SMUSHPLAYTEST_WIDTH,
        SMUSHPLAYTEST_HEIGHT, -1, 0, SmushPlayTest_Blit, loops, 8192, 4096);
    SmushPlayTest_Check(result == expectedReturn, "Unexpected PlayMovie return value.");
    SmushPlayTest_Check(smushPlayTest_numCallbacks == expectedCallbacks, "Unexpected complete-frame count.");
    SmushPlayTest_Check(smushPlayTest_numEnds == expectedEnds, "Unexpected natural-END count.");
    if ( expectedEnds )
    {
        SmushPlayTest_Check(smushPlayTest_lastState == ANIMPLAY_STATE_END, "Playback did not finish with END.");
    }
    fprintf(stderr, "movie=%s loops=%d abort=%d return=%d frames=%d ends=%d terminal=%d\n",
        pName, loops, abortAfter, result, smushPlayTest_numCallbacks, smushPlayTest_numEnds, smushPlayTest_lastState);
}

int SmushPlayTest_RunChild(const char* pScenario, const char* pDirectory)
{
    SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX | SEM_NOOPENFILEERRORBOX);
    FILE* pLog = NULL;
    if ( freopen_s(&pLog, "child.log", "w", stderr) != 0 ) return 2;
    setvbuf(stderr, NULL, _IONBF, 0);
#if defined(_DEBUG)
    _CrtSetReportMode(_CRT_ASSERT, _CRTDBG_MODE_FILE);
    _CrtSetReportFile(_CRT_ASSERT, _CRTDBG_FILE_STDERR);
#endif
    char aPath[MAX_PATH];
    snprintf(aPath, sizeof(aPath), "%s\\expected-rgb565.bin", pDirectory);
    FILE* pExpected = NULL;
    if ( fopen_s(&pExpected, aPath, "rb") != 0 ) return 3;
    size_t size = fread(smushPlayTest_aExpected, 1, sizeof(smushPlayTest_aExpected), pExpected);
    int trailing = fgetc(pExpected);
    fclose(pExpected);
    if ( size != sizeof(smushPlayTest_aExpected) || trailing != EOF ) return 4;

    Smush_ResetGlobals();
    int cycles = strcmp(pScenario, "restart") == 0 ? 2 : 1;
    for ( int cycle = 0; cycle < cycles; ++cycle )
    {
        if ( !SmushPlayTest_Start() ) break;
        if ( strcmp(pScenario, "plain") == 0 )
            SmushPlayTest_Movie(pDirectory, "SANM.snm", 1, 0, 1, 4, 1);
        else if ( strcmp(pScenario, "gzip") == 0 )
            SmushPlayTest_Movie(pDirectory, "ZNM.znm", 1, 0, 1, 4, 1);
        else if ( strcmp(pScenario, "unregistered") == 0 )
            SmushPlayTest_Movie(pDirectory, "SANM.san", 1, 0, 1, 4, 1);
        else if ( strcmp(pScenario, "abort-replay") == 0 )
        {
            SmushPlayTest_Movie(pDirectory, "SANM.snm", 2, 1, 1, 1, 0);
            SmushPlayTest_Movie(pDirectory, "SANM.snm", 1, 0, 1, 4, 1);
        }
        else if ( strcmp(pScenario, "restart") == 0 )
            SmushPlayTest_Movie(pDirectory, cycle ? "ZNM.znm" : "SANM.snm", 1, 0, 1, 4, 1);
        else if ( strcmp(pScenario, "loops") == 0 )
            SmushPlayTest_Movie(pDirectory, "SANM.snm", 2, 0, 1, 8, 2);
        else if ( strcmp(pScenario, "zero-loops") == 0 )
            SmushPlayTest_Movie(pDirectory, "SANM.snm", 0, 0, 1, 0, 0);
        else if ( strcmp(pScenario, "missing-replay") == 0 )
        {
            SmushPlayTest_Movie(pDirectory, "absent.snm", 1, 0, 0, 0, 0);
            SmushPlayTest_Movie(pDirectory, "ZNM.znm", 1, 0, 1, 4, 1);
        }
        else
            SmushPlayTest_Check(0, "Unknown lifecycle scenario.");
        SmushPlayTest_Stop();
    }
    SmushPlayTest_Stop();
    fprintf(stderr, "scenario=%s errors=%d\n", pScenario, smushPlayTest_numErrors);
    return smushPlayTest_numErrors ? 1 : 0;
}

TEST_GROUP(SmushPlay);

TEST_SETUP(SmushPlay)
{
    smushPlayTest_aDirectory[0] = '\0';
    char aTempPath[MAX_PATH];
    DWORD size = GetTempPathA(MAX_PATH, aTempPath);
    TEST_ASSERT_TRUE(size > 0 && size < MAX_PATH);
    TEST_ASSERT_NOT_EQUAL_UINT(0u, GetTempFileNameA(aTempPath, "OJP", 0, smushPlayTest_aDirectory));
    TEST_ASSERT_TRUE(DeleteFileA(smushPlayTest_aDirectory));
    TEST_ASSERT_TRUE(CreateDirectoryA(smushPlayTest_aDirectory, NULL));
    for ( size_t i = 0; i < STD_ARRAYLEN(smushPlayTest_apFiles); ++i )
    {
        char aSource[MAX_PATH], aTarget[MAX_PATH];
        snprintf(aSource, sizeof(aSource), "%s/%s", SMUSH_PLAY_TV_DIR, smushPlayTest_apFiles[i]);
        snprintf(aTarget, sizeof(aTarget), "%s\\%s", smushPlayTest_aDirectory, smushPlayTest_apFiles[i]);
        TEST_ASSERT_TRUE(CopyFileA(aSource, aTarget, FALSE));
    }
}

TEST_TEAR_DOWN(SmushPlay)
{
    if ( smushPlayTest_aDirectory[0] )
    {
        char aPath[MAX_PATH];
        for ( size_t i = 0; i < STD_ARRAYLEN(smushPlayTest_apFiles); ++i )
        {
            snprintf(aPath, sizeof(aPath), "%s\\%s", smushPlayTest_aDirectory, smushPlayTest_apFiles[i]);
            DeleteFileA(aPath);
        }
        snprintf(aPath, sizeof(aPath), "%s\\child.log", smushPlayTest_aDirectory);
        DeleteFileA(aPath);
        DeleteFileA(smushPlayTest_aDirectory);
        TEST_ASSERT_TRUE(RemoveDirectoryA(smushPlayTest_aDirectory));
        smushPlayTest_aDirectory[0] = '\0';
    }
}

static void SmushPlayTest_Run(const char* pScenario)
{
    char aExecutable[MAX_PATH], aCommand[MAX_PATH * 3];
    DWORD size = GetModuleFileNameA(NULL, aExecutable, MAX_PATH);
    TEST_ASSERT_TRUE(size > 0 && size < MAX_PATH);
    snprintf(aCommand, sizeof(aCommand), "\"%s\" --smush-play-child %s \"%s\"", aExecutable, pScenario, smushPlayTest_aDirectory);
    STARTUPINFOA startup = { 0 };
    PROCESS_INFORMATION process = { 0 };
    startup.cb = sizeof(startup);
    BOOL bCreated = CreateProcessA(aExecutable, aCommand, NULL, NULL, FALSE, CREATE_NO_WINDOW,
        NULL, smushPlayTest_aDirectory, &startup, &process);
    TEST_ASSERT_TRUE(bCreated);
    DWORD wait = WaitForSingleObject(process.hProcess, SMUSHPLAYTEST_CHILD_MS);
    DWORD reaped = WAIT_OBJECT_0;
    if ( wait != WAIT_OBJECT_0 )
    {
        TerminateProcess(process.hProcess, ERROR_TIMEOUT);
        reaped = WaitForSingleObject(process.hProcess, 1000u);
    }
    DWORD exitCode = 0;
    BOOL bGotExit = GetExitCodeProcess(process.hProcess, &exitCode);
    CloseHandle(process.hThread);
    CloseHandle(process.hProcess);

    char aLogPath[MAX_PATH], aLog[2048] = { 0 };
    snprintf(aLogPath, sizeof(aLogPath), "%s\\child.log", smushPlayTest_aDirectory);
    FILE* pLog = NULL;
    if ( fopen_s(&pLog, aLogPath, "rb") == 0 )
    {
        fread(aLog, 1, sizeof(aLog) - 1, pLog);
        fclose(pLog);
    }
    TEST_ASSERT_EQUAL_UINT32_MESSAGE(WAIT_OBJECT_0, reaped, aLog);
    TEST_ASSERT_EQUAL_UINT32_MESSAGE(WAIT_OBJECT_0, wait, aLog);
    TEST_ASSERT_TRUE(bGotExit);
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, (int)exitCode, aLog);
}

TEST(SmushPlay, PlainMovieReachesNaturalEndWithEveryPixel) { SmushPlayTest_Run("plain"); }
TEST(SmushPlay, GzipMovieReachesNaturalEndWithEveryPixel) { SmushPlayTest_Run("gzip"); }
TEST(SmushPlay, UnregisteredPlainExtensionKeepsEveryFrame) { SmushPlayTest_Run("unregistered"); }
TEST(SmushPlay, CallbackAbortThenReplayResetsPlayerState) { SmushPlayTest_Run("abort-replay"); }
TEST(SmushPlay, RepeatedStartupPlaybackAndShutdown) { SmushPlayTest_Run("restart"); }
TEST(SmushPlay, TwoLoopsRepeatCompleteFramesInOrder) { SmushPlayTest_Run("loops"); }
TEST(SmushPlay, ZeroLoopsDoesNotPresentFrames) { SmushPlayTest_Run("zero-loops"); }
TEST(SmushPlay, MissingMovieThenReplayCleansUp) { SmushPlayTest_Run("missing-replay"); }

TEST_GROUP_RUNNER(SmushPlay)
{
    RUN_TEST_CASE(SmushPlay, PlainMovieReachesNaturalEndWithEveryPixel);
    RUN_TEST_CASE(SmushPlay, GzipMovieReachesNaturalEndWithEveryPixel);
    RUN_TEST_CASE(SmushPlay, UnregisteredPlainExtensionKeepsEveryFrame);
    RUN_TEST_CASE(SmushPlay, CallbackAbortThenReplayResetsPlayerState);
    RUN_TEST_CASE(SmushPlay, RepeatedStartupPlaybackAndShutdown);
    RUN_TEST_CASE(SmushPlay, TwoLoopsRepeatCompleteFramesInOrder);
    RUN_TEST_CASE(SmushPlay, ZeroLoopsDoesNotPresentFrames);
    RUN_TEST_CASE(SmushPlay, MissingMovieThenReplayCleansUp);
}

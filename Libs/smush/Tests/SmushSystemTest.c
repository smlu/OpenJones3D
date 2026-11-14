#include <unity_fixture.h>

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <smush/SmushPlay.h>
#include <smush/insane/animplay.h>
#include <smush/insane/Lec/LecShare.h>
#include <smush/insane/Sound/Dsnd.h>

#define SMUSHSYSTEM_WIDTH       8
#define SMUSHSYSTEM_HEIGHT      8
#define SMUSHSYSTEM_FRAME_BYTES (SMUSHSYSTEM_WIDTH * SMUSHSYSTEM_HEIGHT * 2)

static HWND smushSystem_hwnd;
static int smushSystem_bStarted;
static uint8_t smushSystem_aExpected[3 * SMUSHSYSTEM_FRAME_BYTES];
static int smushSystem_numFrames;
static int smushSystem_numEnds;
static int smushSystem_lastState;
static const char* smushSystem_pError;
static AnimPlayTable* smushSystem_pPlayer;
static int (*smushSystem_pfPace)(AnimPlay*, void*, int, int, int, void*, int*, int*, int, int);

static int SmushSystem_Check(int bCondition, const char* pMessage)
{
    if ( !bCondition && !smushSystem_pError ) smushSystem_pError = pMessage;
    return bCondition;
}

static int SmushSystem_Pace(AnimPlay* pPlay, void* pBitmap, int a3, int a4, int a5,
    void* pPalette, int* pOutA, int* pOutB, int a9, int a10)
{
    int state = smushSystem_pfPace(pPlay, pBitmap, a3, a4, a5, pPalette, pOutA, pOutB, a9, a10);
    smushSystem_lastState = state;
    if ( state == ANIMPLAY_STATE_END ) ++smushSystem_numEnds;
    SmushSystem_Check(state >= 0, "Decoder returned a failure state.");
    return state;
}

static int __cdecl SmushSystem_Blit(const SmushBitmap* pBitmap, int frame)
{
    if ( !SmushSystem_Check(smushSystem_numFrames < 4 && frame == smushSystem_numFrames,
            "Unexpected native-player callback sequence.")
        || !SmushSystem_Check(pBitmap && pBitmap->pPixels && pBitmap->width == SMUSHSYSTEM_WIDTH
            && pBitmap->height == SMUSHSYSTEM_HEIGHT && pBitmap->pixelSize == 2 && pBitmap->bpp == 16
            && pBitmap->pitch == SMUSHSYSTEM_WIDTH * 2 && pBitmap->colorMode == STDCOLOR_RGB,
            "Unexpected native-player bitmap metadata.") )
    {
        return 1;
    }

    // A normal END presents the retained final image again with callback index 3.
    int image = frame < 3 ? frame : 2;
    const uint8_t* pExpected = smushSystem_aExpected + image * SMUSHSYSTEM_FRAME_BYTES;
    for ( int row = 0; row < SMUSHSYSTEM_HEIGHT; ++row )
    {
        if ( !SmushSystem_Check(memcmp((const uint8_t*)pBitmap->pPixels + row * pBitmap->pitch,
            pExpected + row * SMUSHSYSTEM_WIDTH * 2, SMUSHSYSTEM_WIDTH * 2) == 0,
            "Decoded native-player pixels differ from the authored golden.") )
        {
            return 1;
        }
    }
    ++smushSystem_numFrames;
    return 0;
}

static void SmushSystem_Stop(void)
{
    if ( smushSystem_pPlayer )
    {
        smushSystem_pPlayer->Pace = smushSystem_pfPace;
        LecShare_Unsubscribe(smushSystem_pPlayer);
        smushSystem_pPlayer = NULL;
    }
    if ( smushSystem_bStarted )
    {
        SmushPlay_SysShutdown();
        smushSystem_bStarted = 0;
    }
}

static void SmushSystem_RequireNativeDevice(void)
{
    tDirectSound* pDevice = NULL;
#if defined(J3D_DIRECTX9)
    HRESULT hr = DirectSoundCreate8(NULL, &pDevice, NULL);
#else
    HRESULT hr = DirectSoundCreate(NULL, &pDevice, NULL);
#endif
    if ( SUCCEEDED(hr) ) hr = pDevice->lpVtbl->SetCooperativeLevel(pDevice, smushSystem_hwnd, DSSCL_EXCLUSIVE);
    if ( pDevice ) pDevice->lpVtbl->Release(pDevice);
    if ( FAILED(hr) )
    {
        char aReason[128];
        snprintf(aReason, sizeof(aReason), "Native DirectSound device/cooperative level unavailable (0x%08lX).", (unsigned long)hr);
        TEST_IGNORE_MESSAGE(aReason);
    }
}

TEST_GROUP(SmushSystem);

TEST_SETUP(SmushSystem)
{
    smushSystem_hwnd = NULL;
    smushSystem_bStarted = 0;
    smushSystem_pPlayer = NULL;
    Smush_ResetGlobals();

    FILE* pFile = NULL;
    TEST_ASSERT_EQUAL_INT(0, fopen_s(&pFile, SMUSH_PLAY_TV_DIR "/expected-rgb565.bin", "rb"));
    size_t bytes = fread(smushSystem_aExpected, 1, sizeof(smushSystem_aExpected), pFile);
    int trailing = fgetc(pFile);
    fclose(pFile);
    TEST_ASSERT_EQUAL_UINT(sizeof(smushSystem_aExpected), bytes);
    TEST_ASSERT_EQUAL_INT(EOF, trailing);

    smushSystem_hwnd = CreateWindowExA(0, "STATIC", "SMUSH native sound test", WS_OVERLAPPEDWINDOW,
        0, 0, 64, 64, NULL, NULL, GetModuleHandleA(NULL), NULL);
    if ( !smushSystem_hwnd ) TEST_IGNORE_MESSAGE("A hidden Win32 window is unavailable.");
    SmushSystem_RequireNativeDevice();
}

TEST_TEAR_DOWN(SmushSystem)
{
    SmushSystem_Stop();
    if ( smushSystem_hwnd ) DestroyWindow(smushSystem_hwnd);
    smushSystem_hwnd = NULL;
}

TEST(SmushSystem, OwnedConfiguredDevicePlaysSyntheticMoviesAcrossRestart)
{
    const char* apMovies[] = { SMUSH_PLAY_TV_DIR "/SANM.snm", SMUSH_PLAY_TV_DIR "/ZNM.znm" };
    for ( int cycle = 0; cycle < 2; ++cycle )
    {
        smushSystem_numFrames = smushSystem_numEnds = 0;
        smushSystem_lastState = -1;
        smushSystem_pError = NULL;
        smushSystem_bStarted = SmushPlay_SysStartup(smushSystem_hwnd, NULL);
        int bStarted = smushSystem_bStarted;
        // The sound engine can continue silently after driver failure; startup alone is not proof of a real buffer.
        int bHasNativeBuffer = bStarted && dsnd_GetPlayPos() >= 0;
        int result = 0;
        int bHasPlayer = 0;
        if ( bHasNativeBuffer )
        {
            smushSystem_pPlayer = LecShare_Subscribe("ANIMPLAY", NULL);
            bHasPlayer = smushSystem_pPlayer && smushSystem_pPlayer->Pace;
            if ( smushSystem_pPlayer ) smushSystem_pfPace = smushSystem_pPlayer->Pace;
            if ( bHasPlayer )
            {
                smushSystem_pPlayer->Pace = SmushSystem_Pace;
                result = SmushPlay_PlayMovie(apMovies[cycle], 100, ANIMPLAY_FLAG_NOSKIP,
                    0, 0, SMUSHSYSTEM_WIDTH, SMUSHSYSTEM_HEIGHT, -1, 0, SmushSystem_Blit, 1, 8192, 4096);
            }
        }
        SmushSystem_Stop();
        int bReleasedNativeBuffer = dsnd_GetPlayPos() == -1;

        TEST_ASSERT_TRUE_MESSAGE(bStarted, "SMUSH startup failed after native capability preflight succeeded.");
        TEST_ASSERT_TRUE_MESSAGE(bHasNativeBuffer, "SMUSH did not create a working native DirectSound buffer.");
        TEST_ASSERT_TRUE_MESSAGE(bHasPlayer, "The production animation player service is unavailable.");
        TEST_ASSERT_NULL_MESSAGE(smushSystem_pError, smushSystem_pError ? smushSystem_pError : "");
        TEST_ASSERT_EQUAL_INT(1, result);
        TEST_ASSERT_EQUAL_INT(4, smushSystem_numFrames);
        TEST_ASSERT_EQUAL_INT(1, smushSystem_numEnds);
        TEST_ASSERT_EQUAL_INT(ANIMPLAY_STATE_END, smushSystem_lastState);
        TEST_ASSERT_TRUE_MESSAGE(bReleasedNativeBuffer, "SMUSH shutdown retained its native buffer.");
    }
}

TEST_GROUP_RUNNER(SmushSystem)
{
    SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX | SEM_NOOPENFILEERRORBOX);
    RUN_TEST_CASE(SmushSystem, OwnedConfiguredDevicePlaysSyntheticMoviesAcrossRestart);
}

#include <unity_fixture.h>

#include <stdbool.h>
#include <stdint.h>

#include <std/General/stdUtil.h>
#include <std/Win95/std3D.h>
#include <std/Win95/stdDisplay.h>

#include "std3DSystemTestSupport.h"
#include "stdGeneralTest.h"
#include "stdWin95SystemTestSupport.h"

TEST_GROUP(stdDisplayDX6DisruptiveSystem);

TEST_SETUP(stdDisplayDX6DisruptiveSystem)
{
    StdWin95SystemTest_Startup();
}

TEST_TEAR_DOWN(stdDisplayDX6DisruptiveSystem)
{
    StdWin95SystemTest_Shutdown();
}

static bool stdDisplayDX6DisruptiveSystemTest_FindFullscreenMode(size_t* pModeNum, StdVideoMode* pMode)
{
    uint64_t bestArea = UINT64_MAX;
    size_t bestMode   = SIZE_MAX;

    for ( size_t i = 0u; i < stdDisplay_GetNumVideoModes(); ++i )
    {
        StdVideoMode mode;

        STD_ZEROMEM(&mode, sizeof(mode));
        if ( stdDisplay_GetVideoMode(i, &mode) )
        {
            continue;
        }

        uint32_t width  = mode.rasterInfo.width;
        uint32_t height = mode.rasterInfo.height;
        uint32_t bpp    = mode.rasterInfo.colorInfo.bpp;
        if ( width < 320u || height < 240u )
        {
            continue;
        }

        if ( bpp != 16u && bpp != 24u && bpp != 32u )
        {
            continue;
        }

        uint64_t area = (uint64_t)width * height;
        if ( area < bestArea )
        {
            bestArea = area;
            bestMode = i;
            *pMode   = mode;
        }
    }

    if ( bestMode == SIZE_MAX )
    {
        return false;
    }

    *pModeNum = bestMode;
    return true;
}

static bool stdDisplayDX6DisruptiveSystemTest_FindFullscreenModeForBpp(uint32_t targetBpp, size_t* pModeNum, StdVideoMode* pMode)
{
    uint64_t bestArea = UINT64_MAX;
    size_t bestMode   = SIZE_MAX;

    for ( size_t i = 0u; i < stdDisplay_GetNumVideoModes(); ++i )
    {
        StdVideoMode mode;

        STD_ZEROMEM(&mode, sizeof(mode));
        if ( stdDisplay_GetVideoMode(i, &mode)
            || mode.rasterInfo.width < 320u
            || mode.rasterInfo.height < 240u
            || mode.rasterInfo.colorInfo.bpp != targetBpp )
        {
            continue;
        }

        uint64_t area = (uint64_t)mode.rasterInfo.width * mode.rasterInfo.height;
        if ( area < bestArea )
        {
            bestArea = area;
            bestMode = i;
            *pMode   = mode;
        }
    }

    if ( bestMode == SIZE_MAX )
    {
        return false;
    }

    *pModeNum = bestMode;
    return true;
}

static void stdDisplayDX6DisruptiveSystemTest_Open3D(void)
{
    // Exclusive rendering requires a visible foreground window before creating the 3D device.
    if ( stdDisplay_IsFullscreen() )
    {
        StdWin95SystemTest_RequireForegroundWindow();
    }

    TEST_ASSERT_TRUE(std3D_Startup());
    StdWin95SystemTest_SetStd3DStarted(true);
    TEST_ASSERT_TRUE(std3D_Open(0u));
}

static void stdDisplayDX6DisruptiveSystemTest_Close3D(void)
{
    std3D_Close();
    std3D_Shutdown();
    StdWin95SystemTest_SetStd3DStarted(false);
}

static void stdDisplayDX6DisruptiveSystemTest_RenderPrimitiveAfterModeTransition(void)
{
    uint8_t red   = 0u;
    uint8_t green = 0u;
    uint8_t blue  = 0u;

    StdGeneralTest_ResetDebugOutput();
    for ( size_t attempt = 0u; attempt < 4u; ++attempt )
    {
        Std3DSystemTest_RenderPrimitiveHarnessScene();
        Std3DSystemTest_ReadBackBufferPixel(160u, 120u, &red, &green, &blue);
        if ( red <= 32u && green >= 223u && blue <= 32u )
        {
            return;
        }

        if ( attempt + 1u < 4u )
        {
            TEST_ASSERT_EQUAL_INT(0, stdDisplay_Update());
            Sleep(16u);
        }
    }

    TEST_MESSAGE(StdGeneralTest_GetDebugOutput());
    Std3DSystemTest_AssertBackBufferPixelNear(160u, 120u, 0u, 255u, 0u, 32u);
}

static void stdDisplayDX6DisruptiveSystemTest_AssertBackBufferUsable(uint32_t expectedWidth, uint32_t expectedHeight)
{
    void* pSurface;
    uint32_t width;
    uint32_t height;
    int32_t pitch;

    stdDisplay_GetBackBufferSize(&width, &height);
    TEST_ASSERT_EQUAL_UINT32(expectedWidth, width);
    TEST_ASSERT_EQUAL_UINT32(expectedHeight, height);
    TEST_ASSERT_EQUAL_INT(0, stdDisplay_BackBufferFill(0u, NULL));
    TEST_ASSERT_EQUAL_INT(0, stdDisplay_Update());
    TEST_ASSERT_EQUAL_INT(0, stdDisplay_LockBackBufferReadOnly(&pSurface, &width, &height, &pitch));
    TEST_ASSERT_NOT_NULL(pSurface);
    TEST_ASSERT_EQUAL_UINT32(expectedWidth, width);
    TEST_ASSERT_EQUAL_UINT32(expectedHeight, height);
    TEST_ASSERT_NOT_EQUAL_INT32(0, pitch);
    stdDisplay_UnlockBackBuffer();
}

static void stdDisplayDX6DisruptiveSystemTest_RestoreWindowedMode(void)
{
    stdDisplay_SetDefaultResolution(320u, 240u);
    StdGeneralTest_ResetDebugOutput();

    int result = stdDisplay_SetMode(0u, 0, 1u);
    if ( result )
    {
        TEST_MESSAGE(StdGeneralTest_GetDebugOutput());
    }
    TEST_ASSERT_EQUAL_INT(0, result);
}

static void stdDisplayDX6DisruptiveSystemTest_AssertZBuffer(const tSysPixelFormat* pFormat)
{
    DDSCAPS2 caps = { 0 };
    LPDIRECTDRAWSURFACE4 pSurface = NULL;
    DDSURFACEDESC2 desc = { 0 };

    caps.dwCaps = DDSCAPS_ZBUFFER;
    TEST_ASSERT_EQUAL_INT(DD_OK, IDirectDrawSurface4_GetAttachedSurface(
        stdDisplay_g_backBuffer.surface.pSysSurface, &caps, &pSurface));
    TEST_ASSERT_NOT_NULL(pSurface);
    desc.dwSize = sizeof(desc);
    HRESULT result = IDirectDrawSurface4_GetSurfaceDesc(pSurface, &desc);
    IDirectDrawSurface4_Release(pSurface);
    TEST_ASSERT_EQUAL_INT(DD_OK, result);
    TEST_ASSERT_EQUAL_UINT32(stdDisplay_g_backBuffer.rasterInfo.width, desc.dwWidth);
    TEST_ASSERT_EQUAL_UINT32(stdDisplay_g_backBuffer.rasterInfo.height, desc.dwHeight);
    TEST_ASSERT_EQUAL_MEMORY(pFormat, &desc.ddpfPixelFormat, sizeof(*pFormat));
}

static void stdDisplayDX6DisruptiveSystemTest_ExerciseFullscreenBpp(uint32_t targetBpp)
{
    size_t modeNum;
    StdVideoMode mode;

    StdWin95SystemTest_RequireHiddenWindow();
    StdWin95SystemTest_RequireWindowedDisplayMode();

    STD_ZEROMEM(&mode, sizeof(mode));
    if ( !stdDisplayDX6DisruptiveSystemTest_FindFullscreenModeForBpp(targetBpp, &modeNum, &mode) )
    {
        TEST_IGNORE_MESSAGE("No matching DirectX 6 fullscreen color depth was enumerated.");
    }

    if ( stdDisplay_SetMode(modeNum, 1, 1u) )
    {
        stdDisplayDX6DisruptiveSystemTest_RestoreWindowedMode();
        TEST_IGNORE_MESSAGE("Requested DirectX 6 fullscreen color depth is unavailable on this host.");
    }

    StdVideoMode currentMode;
    STD_ZEROMEM(&currentMode, sizeof(currentMode));
    TEST_ASSERT_EQUAL_INT(0, stdDisplay_GetCurrentVideoMode(&currentMode));
    TEST_ASSERT_EQUAL_UINT32(targetBpp, currentMode.rasterInfo.colorInfo.bpp);
    stdDisplayDX6DisruptiveSystemTest_AssertBackBufferUsable(mode.rasterInfo.width, mode.rasterInfo.height);

    stdDisplayDX6DisruptiveSystemTest_Open3D();
    stdDisplayDX6DisruptiveSystemTest_RenderPrimitiveAfterModeTransition();
    stdDisplayDX6DisruptiveSystemTest_Close3D();

    stdDisplayDX6DisruptiveSystemTest_RestoreWindowedMode();
    stdDisplayDX6DisruptiveSystemTest_AssertBackBufferUsable(320u, 240u);
}

TEST(stdDisplayDX6DisruptiveSystem, TestRefreshKeepsWindowedBackBufferUsable)
{
    StdWin95SystemTest_RequireHiddenWindow();
    StdWin95SystemTest_RequireWindowedDisplayMode();

    stdDisplayDX6DisruptiveSystemTest_AssertBackBufferUsable(320u, 240u);
    stdDisplay_Refresh(1);
    stdDisplayDX6DisruptiveSystemTest_AssertBackBufferUsable(320u, 240u);
}

TEST(stdDisplayDX6DisruptiveSystem, TestFullscreenModeSwitchRecreatesBackBuffer)
{
    size_t modeNum;
    StdVideoMode mode;

    StdWin95SystemTest_RequireHiddenWindow();
    StdWin95SystemTest_RequireWindowedDisplayMode();

    STD_ZEROMEM(&mode, sizeof(mode));
    if ( !stdDisplayDX6DisruptiveSystemTest_FindFullscreenMode(&modeNum, &mode) )
    {
        TEST_IGNORE_MESSAGE("No usable DirectX 6 fullscreen video mode was enumerated.");
    }

    if ( stdDisplay_SetMode(modeNum, 1, 1u) )
    {
        stdDisplayDX6DisruptiveSystemTest_RestoreWindowedMode();
        TEST_IGNORE_MESSAGE("DirectX 6 fullscreen mode switch unavailable on this host.");
    }

    TEST_ASSERT_TRUE(stdDisplay_IsFullscreen());
    stdDisplayDX6DisruptiveSystemTest_AssertBackBufferUsable(mode.rasterInfo.width, mode.rasterInfo.height);

    stdDisplayDX6DisruptiveSystemTest_RestoreWindowedMode();
    TEST_ASSERT_FALSE(stdDisplay_IsFullscreen());
    stdDisplayDX6DisruptiveSystemTest_AssertBackBufferUsable(320u, 240u);
}

TEST(stdDisplayDX6DisruptiveSystem, TestModeSwitchRecreatesZBuffer)
{
    size_t modeNum;
    StdVideoMode mode;
    DDSCAPS2 caps = { 0 };
    LPDIRECTDRAWSURFACE4 pSurface = NULL;
    DDSURFACEDESC2 desc = { 0 };

    StdWin95SystemTest_RequireHiddenWindow();
    StdWin95SystemTest_RequireWindowedDisplayMode();

    STD_ZEROMEM(&mode, sizeof(mode));
    if ( !stdDisplayDX6DisruptiveSystemTest_FindFullscreenMode(&modeNum, &mode) )
    {
        TEST_IGNORE_MESSAGE("No usable DirectX 6 fullscreen video mode was enumerated.");
    }

    // Use the complete supported pixel format selected by the rendering backend.
    stdDisplayDX6DisruptiveSystemTest_Open3D();
    caps.dwCaps = DDSCAPS_ZBUFFER;
    TEST_ASSERT_EQUAL_INT(DD_OK, IDirectDrawSurface4_GetAttachedSurface(
        stdDisplay_g_backBuffer.surface.pSysSurface, &caps, &pSurface));
    TEST_ASSERT_NOT_NULL(pSurface);
    desc.dwSize = sizeof(desc);
    HRESULT result = IDirectDrawSurface4_GetSurfaceDesc(pSurface, &desc);
    IDirectDrawSurface4_Release(pSurface);
    TEST_ASSERT_EQUAL_INT(DD_OK, result);
    int bSystemMemory = (desc.ddsCaps.dwCaps & DDSCAPS_SYSTEMMEMORY) != 0;
    stdDisplayDX6DisruptiveSystemTest_Close3D();

    TEST_ASSERT_EQUAL_INT(0, stdDisplay_CreateZBuffer(&desc.ddpfPixelFormat, bSystemMemory));
    stdDisplayDX6DisruptiveSystemTest_AssertZBuffer(&desc.ddpfPixelFormat);

    if ( stdDisplay_SetMode(modeNum, 1, 1u) )
    {
        stdDisplayDX6DisruptiveSystemTest_RestoreWindowedMode();
        TEST_IGNORE_MESSAGE("DirectX 6 fullscreen mode switch unavailable on this host.");
    }

    TEST_ASSERT_EQUAL_INT(0, stdDisplay_CreateZBuffer(&desc.ddpfPixelFormat, bSystemMemory));
    stdDisplayDX6DisruptiveSystemTest_AssertZBuffer(&desc.ddpfPixelFormat);
    stdDisplayDX6DisruptiveSystemTest_AssertBackBufferUsable(mode.rasterInfo.width, mode.rasterInfo.height);

    stdDisplayDX6DisruptiveSystemTest_RestoreWindowedMode();
    TEST_ASSERT_EQUAL_INT(0, stdDisplay_CreateZBuffer(&desc.ddpfPixelFormat, bSystemMemory));
    stdDisplayDX6DisruptiveSystemTest_AssertZBuffer(&desc.ddpfPixelFormat);
    stdDisplayDX6DisruptiveSystemTest_AssertBackBufferUsable(320u, 240u);
}

TEST(stdDisplayDX6DisruptiveSystem, TestFullscreenUsesTwoBackBuffers)
{
    size_t modeNum;
    StdVideoMode mode;

    StdWin95SystemTest_RequireHiddenWindow();
    StdWin95SystemTest_RequireWindowedDisplayMode();

    STD_ZEROMEM(&mode, sizeof(mode));
    if ( !stdDisplayDX6DisruptiveSystemTest_FindFullscreenMode(&modeNum, &mode) )
    {
        TEST_IGNORE_MESSAGE("No usable DirectX 6 fullscreen video mode was enumerated.");
    }

    if ( stdDisplay_SetMode(modeNum, 1, 2u) )
    {
        stdDisplayDX6DisruptiveSystemTest_RestoreWindowedMode();
        TEST_IGNORE_MESSAGE("DirectX 6 fullscreen double buffering is unavailable on this host.");
    }

    TEST_ASSERT_EQUAL_UINT32(2u, stdDisplay_g_frontBuffer.surface.desc.dwBackBufferCount);
    for ( uint32_t frame = 0u; frame < 6u; ++frame )
    {
        TEST_ASSERT_EQUAL_INT(1, stdDisplay_VBufferFill(&stdDisplay_g_backBuffer, frame, NULL));
        TEST_ASSERT_EQUAL_INT(0, stdDisplay_Update());
    }

    stdDisplayDX6DisruptiveSystemTest_RestoreWindowedMode();
    stdDisplayDX6DisruptiveSystemTest_AssertBackBufferUsable(320u, 240u);
}

TEST(stdDisplayDX6DisruptiveSystem, TestFullscreen16BppModeRendersAndRestores)
{
    stdDisplayDX6DisruptiveSystemTest_ExerciseFullscreenBpp(16u);
}

TEST(stdDisplayDX6DisruptiveSystem, TestFullscreen32BppModeRendersAndRestores)
{
    stdDisplayDX6DisruptiveSystemTest_ExerciseFullscreenBpp(32u);
}

TEST(stdDisplayDX6DisruptiveSystem, TestWindowResizeRecreatesUsableBackBuffer)
{
    StdWin95SystemTest_RequireHiddenWindow();
    StdWin95SystemTest_RequireWindowedDisplayMode();

    StdWin95SystemTest_SizeWindowClient(512u, 320u);
    StdWin95SystemTest_PumpMessages();
    stdDisplay_SetDefaultResolution(512u, 320u);
    TEST_ASSERT_EQUAL_INT(0, stdDisplay_SetMode(0u, 0, 1u));
    stdDisplayDX6DisruptiveSystemTest_AssertBackBufferUsable(512u, 320u);

    stdDisplayDX6DisruptiveSystemTest_Open3D();
    Std3DSystemTest_RenderPrimitiveHarnessScene();
    Std3DSystemTest_AssertBackBufferPixelNear(160u, 120u, 0u, 255u, 0u, 32u);
    stdDisplayDX6DisruptiveSystemTest_Close3D();

    StdWin95SystemTest_SizeWindowClient(320u, 240u);
    stdDisplayDX6DisruptiveSystemTest_RestoreWindowedMode();
}

TEST(stdDisplayDX6DisruptiveSystem, TestFullscreenMinimizeRestoreRecoversLive3DResources)
{
    size_t modeNum;
    StdVideoMode mode;

    StdWin95SystemTest_RequireHiddenWindow();
    StdWin95SystemTest_RequireWindowedDisplayMode();

    STD_ZEROMEM(&mode, sizeof(mode));
    if ( !stdDisplayDX6DisruptiveSystemTest_FindFullscreenMode(&modeNum, &mode) )
    {
        TEST_IGNORE_MESSAGE("No usable DirectX 6 fullscreen video mode was enumerated.");
    }

    if ( stdDisplay_SetMode(modeNum, 1, 1u) )
    {
        stdDisplayDX6DisruptiveSystemTest_RestoreWindowedMode();
        TEST_IGNORE_MESSAGE("DirectX 6 fullscreen mode switch unavailable on this host.");
    }

    stdDisplayDX6DisruptiveSystemTest_Open3D();
    stdDisplayDX6DisruptiveSystemTest_RenderPrimitiveAfterModeTransition();

    HWND hwnd = StdWin95SystemTest_GetWindow();
    TEST_ASSERT_NOT_NULL(hwnd);
    ShowWindow(hwnd, SW_MINIMIZE);
    StdWin95SystemTest_PumpMessages();
    Sleep(100u);
    HRESULT lostState = IDirectDrawSurface4_IsLost(stdDisplay_g_frontBuffer.surface.pSysSurface);
    TEST_ASSERT_TRUE(lostState == DD_OK || lostState == DDERR_SURFACELOST);

    ShowWindow(hwnd, SW_RESTORE);
    SetForegroundWindow(hwnd);
    StdWin95SystemTest_PumpMessages();
    Sleep(100u);
    // Restore foreground ownership before resetting the active display resources.
    StdWin95SystemTest_RequireForegroundWindow();
    // Follow application activation order: reset texture bindings before restoring DirectDraw surfaces.
    std3D_ResetTextureCache();
    stdDisplay_Refresh(1);

    TEST_ASSERT_EQUAL_HEX32(DD_OK, IDirectDrawSurface4_IsLost(stdDisplay_g_frontBuffer.surface.pSysSurface));
    TEST_ASSERT_EQUAL_HEX32(DD_OK, IDirectDrawSurface4_IsLost(stdDisplay_g_backBuffer.surface.pSysSurface));
    stdDisplayDX6DisruptiveSystemTest_RenderPrimitiveAfterModeTransition();
    stdDisplayDX6DisruptiveSystemTest_Close3D();

    stdDisplayDX6DisruptiveSystemTest_RestoreWindowedMode();
    stdDisplayDX6DisruptiveSystemTest_AssertBackBufferUsable(320u, 240u);
}

TEST_GROUP_RUNNER(stdDisplayDX6DisruptiveSystem)
{
    RUN_TEST_CASE(stdDisplayDX6DisruptiveSystem, TestRefreshKeepsWindowedBackBufferUsable);
    RUN_TEST_CASE(stdDisplayDX6DisruptiveSystem, TestFullscreenModeSwitchRecreatesBackBuffer);
    RUN_TEST_CASE(stdDisplayDX6DisruptiveSystem, TestModeSwitchRecreatesZBuffer);
    RUN_TEST_CASE(stdDisplayDX6DisruptiveSystem, TestFullscreenUsesTwoBackBuffers);
    RUN_TEST_CASE(stdDisplayDX6DisruptiveSystem, TestFullscreen16BppModeRendersAndRestores);
    RUN_TEST_CASE(stdDisplayDX6DisruptiveSystem, TestFullscreen32BppModeRendersAndRestores);
    RUN_TEST_CASE(stdDisplayDX6DisruptiveSystem, TestWindowResizeRecreatesUsableBackBuffer);
    RUN_TEST_CASE(stdDisplayDX6DisruptiveSystem, TestFullscreenMinimizeRestoreRecoversLive3DResources);
}

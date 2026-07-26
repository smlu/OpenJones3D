#include <unity_fixture.h>

#include <stdbool.h>
#include <stdint.h>

#include <std/General/stdConfig.h>
#include <std/General/stdUtil.h>
#include <std/Win95/std3D.h>
#include <std/Win95/stdDisplay.h>

#include "std3DSystemTestSupport.h"
#include "stdWin95SystemTestSupport.h"

TEST_GROUP(stdDisplayDX9DisruptiveSystem);

static const D3DFORMAT stdDisplayDX9DisruptiveSystemTest_aDepthFormats[] =
{
    D3DFMT_D24S8,
    D3DFMT_D24X4S4,
    D3DFMT_D24X8,
    D3DFMT_D32,
    D3DFMT_D15S1,
    D3DFMT_D16
};

static size_t stdDisplayDX9DisruptiveSystemTest_numReleaseCallbacks;

static void stdDisplayDX9DisruptiveSystemTest_OnDeviceRelease(tSysDevice3D* pDevice)
{
    TEST_ASSERT_NOT_NULL(pDevice);
    ++stdDisplayDX9DisruptiveSystemTest_numReleaseCallbacks;
}

TEST_SETUP(stdDisplayDX9DisruptiveSystem)
{
    StdWin95SystemTest_Startup();
    stdDisplayDX9DisruptiveSystemTest_numReleaseCallbacks = 0u;
}

TEST_TEAR_DOWN(stdDisplayDX9DisruptiveSystem)
{
    StdWin95SystemTest_Shutdown();
}

static bool stdDisplayDX9DisruptiveSystemTest_FindWindowedDepthFormat(UINT* pAdapter, D3DDISPLAYMODE* pDesktopMode, D3DFORMAT* pDepthFormat)
{
    LPDIRECT3D9 pD3D = stdDisplay_GetDirect3D();
    StdDisplayDevice displayDevice;
    if ( !pD3D || stdDisplay_GetCurrentDevice(&displayDevice) )
    {
        return false;
    }

    UINT adapter = displayDevice.caps.AdapterOrdinal;
    if ( FAILED(IDirect3D9_GetAdapterDisplayMode(pD3D, adapter, pDesktopMode)) )
    {
        return false;
    }

    for ( size_t i = 0u; i < STD_ARRAYLEN(stdDisplayDX9DisruptiveSystemTest_aDepthFormats); ++i )
    {
        D3DFORMAT depthFormat = stdDisplayDX9DisruptiveSystemTest_aDepthFormats[i];
        if ( SUCCEEDED(IDirect3D9_CheckDeviceFormat(
                pD3D,
                adapter,
                D3DDEVTYPE_HAL,
                pDesktopMode->Format,
                D3DUSAGE_DEPTHSTENCIL,
                D3DRTYPE_SURFACE,
                depthFormat
            ))
            && SUCCEEDED(IDirect3D9_CheckDepthStencilMatch(
                pD3D,
                adapter,
                D3DDEVTYPE_HAL,
                pDesktopMode->Format,
                pDesktopMode->Format,
                depthFormat
            )) )
        {
            *pAdapter     = adapter;
            *pDepthFormat = depthFormat;
            return true;
        }
    }

    return false;
}

static bool stdDisplayDX9DisruptiveSystemTest_FindFullscreenMode(size_t* pModeNum, StdVideoMode* pMode)
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

static bool stdDisplayDX9DisruptiveSystemTest_FindFullscreenModeForBpp(uint32_t targetBpp, size_t* pModeNum, StdVideoMode* pMode)
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

static D3DPRESENT_PARAMETERS stdDisplayDX9DisruptiveSystemTest_GetPresentParameters(void)
{
    LPDIRECT3DDEVICE9 pDevice = stdDisplay_GetSystemDevice();
    TEST_ASSERT_NOT_NULL(pDevice);

    LPDIRECT3DSWAPCHAIN9 pSwapChain = NULL;
    TEST_ASSERT_EQUAL_HEX32(D3D_OK, IDirect3DDevice9_GetSwapChain(pDevice, 0u, &pSwapChain));
    TEST_ASSERT_NOT_NULL(pSwapChain);

    D3DPRESENT_PARAMETERS params;
    STD_ZEROMEM(&params, sizeof(params));
    TEST_ASSERT_EQUAL_HEX32(D3D_OK, IDirect3DSwapChain9_GetPresentParameters(pSwapChain, &params));
    IDirect3DSwapChain9_Release(pSwapChain);
    return params;
}

static void stdDisplayDX9DisruptiveSystemTest_Open3D(void)
{
    TEST_ASSERT_TRUE(std3D_Startup());
    StdWin95SystemTest_SetStd3DStarted(true);
    TEST_ASSERT_TRUE(std3D_Open(0u));
}

static void stdDisplayDX9DisruptiveSystemTest_Close3D(void)
{
    std3D_Close();
    std3D_Shutdown();
    StdWin95SystemTest_SetStd3DStarted(false);
}

static void stdDisplayDX9DisruptiveSystemTest_AssertBackBufferUsable(uint32_t expectedWidth, uint32_t expectedHeight)
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

static void stdDisplayDX9DisruptiveSystemTest_RequireWindowedMode(void)
{
    stdDisplay_SetDefaultResolution(320u, 240u);
    TEST_ASSERT_EQUAL_INT(0, stdDisplay_SetMode(0u, 0, 1u));
    TEST_ASSERT_FALSE(stdDisplay_IsFullscreen());
    stdDisplayDX9DisruptiveSystemTest_AssertBackBufferUsable(320u, 240u);
}

static void stdDisplayDX9DisruptiveSystemTest_RestoreWindowedMode(void)
{
    stdDisplay_SetDefaultResolution(320u, 240u);
    TEST_ASSERT_EQUAL_INT(0, stdDisplay_SetMode(0u, 0, 1u));
}

static void stdDisplayDX9DisruptiveSystemTest_CreateZBuffer(void)
{
    UINT adapter;
    D3DDISPLAYMODE desktopMode;
    tSysPixelFormat zFormat;

    TEST_ASSERT_TRUE(stdDisplayDX9DisruptiveSystemTest_FindWindowedDepthFormat(&adapter, &desktopMode, &zFormat));
    TEST_ASSERT_EQUAL_INT(0, stdDisplay_CreateZBuffer(&zFormat, 0));
}

static void stdDisplayDX9DisruptiveSystemTest_ExerciseFullscreenBpp(uint32_t targetBpp)
{
    size_t modeNum;
    StdVideoMode mode;

    StdWin95SystemTest_RequireHiddenWindow();
    StdWin95SystemTest_RequireWindowedDisplayMode();

    STD_ZEROMEM(&mode, sizeof(mode));
    if ( !stdDisplayDX9DisruptiveSystemTest_FindFullscreenModeForBpp(targetBpp, &modeNum, &mode) )
    {
        TEST_IGNORE_MESSAGE("No matching DirectX 9 fullscreen color depth was enumerated.");
    }

    if ( stdDisplay_SetMode(modeNum, 1, 1u) )
    {
        stdDisplayDX9DisruptiveSystemTest_RestoreWindowedMode();
        TEST_IGNORE_MESSAGE("Requested DirectX 9 fullscreen color depth is unavailable on this host.");
    }

    StdVideoMode currentMode;
    STD_ZEROMEM(&currentMode, sizeof(currentMode));
    TEST_ASSERT_EQUAL_INT(0, stdDisplay_GetCurrentVideoMode(&currentMode));
    D3DFORMAT requestedFormat = targetBpp == 16u ? D3DFMT_R5G6B5 : D3DFMT_X8R8G8B8;
    D3DPRESENT_PARAMETERS retainedParams;
    stdDisplay_TestGetPresentParameters(&retainedParams);
    TEST_ASSERT_FALSE(retainedParams.Windowed);
    TEST_ASSERT_EQUAL_INT(requestedFormat, retainedParams.BackBufferFormat);

    D3DPRESENT_PARAMETERS params = stdDisplayDX9DisruptiveSystemTest_GetPresentParameters();
    TEST_ASSERT_FALSE(params.Windowed);
    uint32_t actualBpp = targetBpp;
    // Windows may promote an RGB565 fullscreen request to a 32-bit swap chain.
    if ( targetBpp == 16u && params.BackBufferFormat == D3DFMT_X8R8G8B8 )
    {
        actualBpp = 32u;
    }
    else
    {
        TEST_ASSERT_EQUAL_INT(requestedFormat, params.BackBufferFormat);
    }
    TEST_ASSERT_EQUAL_UINT32(actualBpp, currentMode.rasterInfo.colorInfo.bpp);
    TEST_ASSERT_EQUAL_INT(params.BackBufferFormat, stdDisplay_g_frontBuffer.surface.desc.Format);
    TEST_ASSERT_EQUAL_INT(params.BackBufferFormat, stdDisplay_g_backBuffer.surface.desc.Format);

    StdVideoMode enumeratedMode;
    TEST_ASSERT_EQUAL_INT(0, stdDisplay_GetVideoMode(modeNum, &enumeratedMode));
    TEST_ASSERT_EQUAL_MEMORY(&mode, &enumeratedMode, sizeof(mode));
    stdDisplayDX9DisruptiveSystemTest_AssertBackBufferUsable(mode.rasterInfo.width, mode.rasterInfo.height);

    stdDisplayDX9DisruptiveSystemTest_Open3D();
    Std3DSystemTest_RenderPrimitiveHarnessScene();
    Std3DSystemTest_AssertBackBufferPixelNear(160u, 120u, 0u, 255u, 0u, 32u);
    stdDisplayDX9DisruptiveSystemTest_Close3D();

    stdDisplayDX9DisruptiveSystemTest_RestoreWindowedMode();
    stdDisplayDX9DisruptiveSystemTest_AssertBackBufferUsable(320u, 240u);
    TEST_ASSERT_EQUAL_INT(0, stdDisplay_GetVideoMode(modeNum, &enumeratedMode));
    TEST_ASSERT_EQUAL_MEMORY(&mode, &enumeratedMode, sizeof(mode));
}

static void stdDisplayDX9DisruptiveSystemTest_ExerciseWindowedMSAA(int requestedSamples, D3DMULTISAMPLE_TYPE requestedType)
{
    StdWin95SystemTest_RequireHiddenWindow();
    StdWin95SystemTest_RequireDisplayConfig();
    TEST_ASSERT_TRUE(stdConfig_SetBool(STD3D_CFG_MSAAENABLED, true));
    TEST_ASSERT_TRUE(stdConfig_SetInt(STD3D_CFG_MSAASAMPLES, requestedSamples));
    StdWin95SystemTest_RequireWindowedDisplayMode();

    D3DPRESENT_PARAMETERS params = stdDisplayDX9DisruptiveSystemTest_GetPresentParameters();
    TEST_ASSERT_TRUE(params.Windowed);
    TEST_ASSERT_EQUAL_INT(D3DMULTISAMPLE_NONE, params.MultiSampleType);
    TEST_ASSERT_EQUAL_UINT32(0u, params.MultiSampleQuality);
    TEST_ASSERT_NOT_EQUAL_UINT32(0u, params.Flags & D3DPRESENTFLAG_LOCKABLE_BACKBUFFER);
    TEST_ASSERT_EQUAL_INT(D3DMULTISAMPLE_NONE, stdDisplay_g_frontBuffer.surface.desc.MultiSampleType);

    D3DMULTISAMPLE_TYPE renderSampleType = stdDisplay_g_backBuffer.surface.desc.MultiSampleType;
    DWORD renderSampleQuality = stdDisplay_g_backBuffer.surface.desc.MultiSampleQuality;
    TEST_ASSERT_TRUE((int)renderSampleType <= requestedSamples);

    LPDIRECT3D9 pD3D = stdDisplay_GetDirect3D();
    TEST_ASSERT_NOT_NULL(pD3D);

    UINT adapter;
    D3DDISPLAYMODE desktopMode;
    D3DFORMAT depthFormat;
    STD_ZEROMEM(&desktopMode, sizeof(desktopMode));
    TEST_ASSERT_TRUE(stdDisplayDX9DisruptiveSystemTest_FindWindowedDepthFormat(&adapter, &desktopMode, &depthFormat));

    DWORD colorQualityLevels = 0u;
    HRESULT colorSupportResult = IDirect3D9_CheckDeviceMultiSampleType(
        pD3D,
        adapter,
        D3DDEVTYPE_HAL,
        desktopMode.Format,
        TRUE,
        requestedType,
        &colorQualityLevels
    );

    DWORD depthQualityLevels = 0u;
    HRESULT depthSupportResult = IDirect3D9_CheckDeviceMultiSampleType(
        pD3D,
        adapter,
        D3DDEVTYPE_HAL,
        depthFormat,
        TRUE,
        requestedType,
        &depthQualityLevels
    );
    if ( SUCCEEDED(colorSupportResult)
        && SUCCEEDED(depthSupportResult)
        && colorQualityLevels > 0u
        && depthQualityLevels > 0u )
    {
        TEST_ASSERT_EQUAL_INT(requestedType, renderSampleType);
        TEST_ASSERT_TRUE(renderSampleQuality < colorQualityLevels);
        TEST_ASSERT_TRUE(renderSampleQuality < depthQualityLevels);
    }
    else if ( renderSampleType != D3DMULTISAMPLE_NONE )
    {
        colorQualityLevels = 0u;
        TEST_ASSERT_EQUAL_HEX32(
            D3D_OK,
            IDirect3D9_CheckDeviceMultiSampleType(
                pD3D,
                adapter,
                D3DDEVTYPE_HAL,
                desktopMode.Format,
                TRUE,
                renderSampleType,
                &colorQualityLevels
            )
        );

        depthQualityLevels = 0u;
        TEST_ASSERT_EQUAL_HEX32(
            D3D_OK,
            IDirect3D9_CheckDeviceMultiSampleType(
                pD3D,
                adapter,
                D3DDEVTYPE_HAL,
                depthFormat,
                TRUE,
                renderSampleType,
                &depthQualityLevels
            )
        );
        TEST_ASSERT_TRUE(renderSampleQuality < colorQualityLevels);
        TEST_ASSERT_TRUE(renderSampleQuality < depthQualityLevels);
    }

    stdDisplayDX9DisruptiveSystemTest_Open3D();
    Std3DSystemTest_RenderPrimitiveHarnessScene();
    Std3DSystemTest_AssertBackBufferPixelNear(160u, 120u, 0u, 255u, 0u, 32u);
    stdDisplayDX9DisruptiveSystemTest_Close3D();
}

TEST(stdDisplayDX9DisruptiveSystem, TestRefreshKeepsWindowedBackBufferUsable)
{
    StdWin95SystemTest_RequireHiddenWindow();
    StdWin95SystemTest_RequireWindowedDisplayMode();

    stdDisplayDX9DisruptiveSystemTest_CreateZBuffer();
    stdDisplayDX9DisruptiveSystemTest_AssertBackBufferUsable(320u, 240u);

    stdDisplay_Refresh(1);
    stdDisplayDX9DisruptiveSystemTest_AssertBackBufferUsable(320u, 240u);
}

TEST(stdDisplayDX9DisruptiveSystem, TestFullscreenModeSwitchRecreatesBackBuffer)
{
    size_t modeNum;
    StdVideoMode mode;

    StdWin95SystemTest_RequireHiddenWindow();
    StdWin95SystemTest_RequireWindowedDisplayMode();

    STD_ZEROMEM(&mode, sizeof(mode));
    if ( !stdDisplayDX9DisruptiveSystemTest_FindFullscreenMode(&modeNum, &mode) )
    {
        TEST_IGNORE_MESSAGE("No usable DirectX 9 fullscreen video mode was enumerated.");
    }

    stdDisplay_RegisterDeviceReleaseCallback(stdDisplayDX9DisruptiveSystemTest_OnDeviceRelease);
    if ( stdDisplay_SetMode(modeNum, 1, 1u) )
    {
        stdDisplayDX9DisruptiveSystemTest_RestoreWindowedMode();
        TEST_IGNORE_MESSAGE("DirectX 9 fullscreen mode switch unavailable on this host.");
    }

    TEST_ASSERT_TRUE(stdDisplay_IsFullscreen());
    TEST_ASSERT_EQUAL_size_t(1u, stdDisplayDX9DisruptiveSystemTest_numReleaseCallbacks);
    stdDisplayDX9DisruptiveSystemTest_AssertBackBufferUsable(mode.rasterInfo.width, mode.rasterInfo.height);

    stdDisplayDX9DisruptiveSystemTest_RestoreWindowedMode();
    TEST_ASSERT_FALSE(stdDisplay_IsFullscreen());
    TEST_ASSERT_EQUAL_size_t(2u, stdDisplayDX9DisruptiveSystemTest_numReleaseCallbacks);
    stdDisplayDX9DisruptiveSystemTest_AssertBackBufferUsable(320u, 240u);
}

TEST(stdDisplayDX9DisruptiveSystem, TestModeSwitchRecreatesZBuffer)
{
    size_t modeNum;
    StdVideoMode mode;

    StdWin95SystemTest_RequireHiddenWindow();
    StdWin95SystemTest_RequireWindowedDisplayMode();

    STD_ZEROMEM(&mode, sizeof(mode));
    if ( !stdDisplayDX9DisruptiveSystemTest_FindFullscreenMode(&modeNum, &mode) )
    {
        TEST_IGNORE_MESSAGE("No usable DirectX 9 fullscreen video mode was enumerated.");
    }

    stdDisplayDX9DisruptiveSystemTest_CreateZBuffer();
    if ( stdDisplay_SetMode(modeNum, 1, 1u) )
    {
        stdDisplayDX9DisruptiveSystemTest_RestoreWindowedMode();
        TEST_IGNORE_MESSAGE("DirectX 9 fullscreen mode switch unavailable on this host.");
    }

    stdDisplayDX9DisruptiveSystemTest_CreateZBuffer();
    stdDisplayDX9DisruptiveSystemTest_AssertBackBufferUsable(mode.rasterInfo.width, mode.rasterInfo.height);

    stdDisplayDX9DisruptiveSystemTest_RestoreWindowedMode();
    stdDisplayDX9DisruptiveSystemTest_CreateZBuffer();
    stdDisplayDX9DisruptiveSystemTest_AssertBackBufferUsable(320u, 240u);
}

TEST(stdDisplayDX9DisruptiveSystem, TestFullscreenUsesTwoBackBuffers)
{
    size_t modeNum;
    StdVideoMode mode;

    StdWin95SystemTest_RequireHiddenWindow();
    StdWin95SystemTest_RequireWindowedDisplayMode();

    STD_ZEROMEM(&mode, sizeof(mode));
    if ( !stdDisplayDX9DisruptiveSystemTest_FindFullscreenMode(&modeNum, &mode) )
    {
        TEST_IGNORE_MESSAGE("No usable DirectX 9 fullscreen video mode was enumerated.");
    }

    if ( stdDisplay_SetMode(modeNum, 1, 2u) )
    {
        stdDisplayDX9DisruptiveSystemTest_RestoreWindowedMode();
        TEST_IGNORE_MESSAGE("DirectX 9 fullscreen double buffering is unavailable on this host.");
    }

    D3DPRESENT_PARAMETERS params = stdDisplayDX9DisruptiveSystemTest_GetPresentParameters();
    TEST_ASSERT_FALSE(params.Windowed);
    TEST_ASSERT_EQUAL_UINT32(2u, params.BackBufferCount);
    for ( uint32_t frame = 0u; frame < 6u; ++frame )
    {
        TEST_ASSERT_EQUAL_INT(0, stdDisplay_BackBufferFill(0x00010101u * frame, NULL));
        TEST_ASSERT_EQUAL_INT(0, stdDisplay_Update());
    }

    stdDisplayDX9DisruptiveSystemTest_RestoreWindowedMode();
    stdDisplayDX9DisruptiveSystemTest_AssertBackBufferUsable(320u, 240u);
}

TEST(stdDisplayDX9DisruptiveSystem, TestFullscreen16BppModeRendersAndRestores)
{
    stdDisplayDX9DisruptiveSystemTest_ExerciseFullscreenBpp(16u);
}

TEST(stdDisplayDX9DisruptiveSystem, TestFullscreen32BppModeRendersAndRestores)
{
    stdDisplayDX9DisruptiveSystemTest_ExerciseFullscreenBpp(32u);
}

TEST(stdDisplayDX9DisruptiveSystem, TestWindowed2xMSAAOrSupportedFallback)
{
    stdDisplayDX9DisruptiveSystemTest_ExerciseWindowedMSAA(2, D3DMULTISAMPLE_2_SAMPLES);
}

TEST(stdDisplayDX9DisruptiveSystem, TestWindowed4xMSAAOrSupportedFallback)
{
    stdDisplayDX9DisruptiveSystemTest_ExerciseWindowedMSAA(4, D3DMULTISAMPLE_4_SAMPLES);
}

TEST(stdDisplayDX9DisruptiveSystem, TestWindowed8xMSAAOrSupportedFallback)
{
    stdDisplayDX9DisruptiveSystemTest_ExerciseWindowedMSAA(8, D3DMULTISAMPLE_8_SAMPLES);
}

TEST(stdDisplayDX9DisruptiveSystem, TestWindowed16xMSAAOrSupportedFallback)
{
    stdDisplayDX9DisruptiveSystemTest_ExerciseWindowedMSAA(16, D3DMULTISAMPLE_16_SAMPLES);
}

TEST(stdDisplayDX9DisruptiveSystem, TestWindowResizeRecreatesUsableBackBuffer)
{
    StdWin95SystemTest_RequireHiddenWindow();
    StdWin95SystemTest_RequireWindowedDisplayMode();

    StdWin95SystemTest_SizeWindowClient(512u, 320u);
    StdWin95SystemTest_PumpMessages();
    stdDisplay_SetDefaultResolution(512u, 320u);
    TEST_ASSERT_EQUAL_INT(0, stdDisplay_SetMode(0u, 0, 1u));
    stdDisplayDX9DisruptiveSystemTest_AssertBackBufferUsable(512u, 320u);

    stdDisplayDX9DisruptiveSystemTest_Open3D();
    Std3DSystemTest_RenderPrimitiveHarnessScene();
    Std3DSystemTest_AssertBackBufferPixelNear(160u, 120u, 0u, 255u, 0u, 32u);
    stdDisplayDX9DisruptiveSystemTest_Close3D();

    StdWin95SystemTest_SizeWindowClient(320u, 240u);
    stdDisplayDX9DisruptiveSystemTest_RestoreWindowedMode();
}

TEST(stdDisplayDX9DisruptiveSystem, TestFullscreenMinimizeRestoreRecoversLive3DResources)
{
    size_t modeNum;
    StdVideoMode mode;

    StdWin95SystemTest_RequireHiddenWindow();
    StdWin95SystemTest_RequireWindowedDisplayMode();

    STD_ZEROMEM(&mode, sizeof(mode));
    if ( !stdDisplayDX9DisruptiveSystemTest_FindFullscreenMode(&modeNum, &mode) )
    {
        TEST_IGNORE_MESSAGE("No usable DirectX 9 fullscreen video mode was enumerated.");
    }

    if ( stdDisplay_SetMode(modeNum, 1, 1u) )
    {
        stdDisplayDX9DisruptiveSystemTest_RestoreWindowedMode();
        TEST_IGNORE_MESSAGE("DirectX 9 fullscreen mode switch unavailable on this host.");
    }

    stdDisplayDX9DisruptiveSystemTest_Open3D();
    Std3DSystemTest_RenderPrimitiveHarnessScene();

    HWND hwnd = StdWin95SystemTest_GetWindow();
    TEST_ASSERT_NOT_NULL(hwnd);
    ShowWindow(hwnd, SW_MINIMIZE);
    StdWin95SystemTest_PumpMessages();
    Sleep(100u);
    TEST_ASSERT_EQUAL_INT(0, stdDisplay_Update());

    ShowWindow(hwnd, SW_RESTORE);
    SetForegroundWindow(hwnd);
    StdWin95SystemTest_PumpMessages();

    LPDIRECT3DDEVICE9 pDevice = stdDisplay_GetSystemDevice();
    TEST_ASSERT_NOT_NULL(pDevice);
    HRESULT deviceState = D3DERR_DEVICELOST;
    for ( size_t attempt = 0u; attempt < 100u; ++attempt )
    {
        deviceState = IDirect3DDevice9_TestCooperativeLevel(pDevice);
        if ( deviceState == D3DERR_DEVICENOTRESET )
        {
            stdDisplay_Refresh(1);
            deviceState = IDirect3DDevice9_TestCooperativeLevel(pDevice);
        }

        if ( deviceState == D3D_OK )
        {
            break;
        }

        Sleep(20u);
        StdWin95SystemTest_PumpMessages();
    }

    TEST_ASSERT_EQUAL_HEX32(D3D_OK, deviceState);
    Std3DSystemTest_RenderPrimitiveHarnessScene();
    Std3DSystemTest_AssertBackBufferPixelNear(160u, 120u, 0u, 255u, 0u, 32u);
    stdDisplayDX9DisruptiveSystemTest_Close3D();

    stdDisplayDX9DisruptiveSystemTest_RestoreWindowedMode();
    stdDisplayDX9DisruptiveSystemTest_AssertBackBufferUsable(320u, 240u);
}

TEST_GROUP_RUNNER(stdDisplayDX9DisruptiveSystem)
{
    RUN_TEST_CASE(stdDisplayDX9DisruptiveSystem, TestRefreshKeepsWindowedBackBufferUsable);
    RUN_TEST_CASE(stdDisplayDX9DisruptiveSystem, TestFullscreenModeSwitchRecreatesBackBuffer);
    RUN_TEST_CASE(stdDisplayDX9DisruptiveSystem, TestModeSwitchRecreatesZBuffer);
    RUN_TEST_CASE(stdDisplayDX9DisruptiveSystem, TestFullscreenUsesTwoBackBuffers);
    RUN_TEST_CASE(stdDisplayDX9DisruptiveSystem, TestFullscreen16BppModeRendersAndRestores);
    RUN_TEST_CASE(stdDisplayDX9DisruptiveSystem, TestFullscreen32BppModeRendersAndRestores);
    RUN_TEST_CASE(stdDisplayDX9DisruptiveSystem, TestWindowed2xMSAAOrSupportedFallback);
    RUN_TEST_CASE(stdDisplayDX9DisruptiveSystem, TestWindowed4xMSAAOrSupportedFallback);
    RUN_TEST_CASE(stdDisplayDX9DisruptiveSystem, TestWindowed8xMSAAOrSupportedFallback);
    RUN_TEST_CASE(stdDisplayDX9DisruptiveSystem, TestWindowed16xMSAAOrSupportedFallback);
    RUN_TEST_CASE(stdDisplayDX9DisruptiveSystem, TestWindowResizeRecreatesUsableBackBuffer);
    RUN_TEST_CASE(stdDisplayDX9DisruptiveSystem, TestFullscreenMinimizeRestoreRecoversLive3DResources);
}

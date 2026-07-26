#include <unity_fixture.h>

#include <stdint.h>

#include <std/General/std.h>
#include <std/General/stdColor.h>
#include <std/General/stdConfig.h>
#include <std/General/stdUtil.h>
#include <std/Win95/std3D.h>
#include <std/Win95/stdDisplay.h>

#include "std3DSystemTestSupport.h"
#include "stdGeneralTest.h"
#include "stdWin95SystemTestSupport.h"

TEST_GROUP(stdDisplayDX9System);

static const D3DFORMAT stdDisplayDX9SystemTest_aDepthFormats[] =
{
    D3DFMT_D24S8,
    D3DFMT_D24X4S4,
    D3DFMT_D24X8,
    D3DFMT_D32,
    D3DFMT_D15S1,
    D3DFMT_D16
};

static size_t stdDisplayDX9SystemTest_numPreResetCallbacks;
static size_t stdDisplayDX9SystemTest_numPostResetCallbacks;
static size_t stdDisplayDX9SystemTest_numReleaseCallbacks;
static bool stdDisplayDX9SystemTest_bPostResetSucceeds;

static void stdDisplayDX9SystemTest_OnDevicePreReset(tSysDevice3D* pDevice)
{
    TEST_ASSERT_NOT_NULL(pDevice);
    TEST_ASSERT_EQUAL_PTR(stdDisplay_GetSystemDevice(), pDevice);
    TEST_ASSERT_EQUAL_size_t(stdDisplayDX9SystemTest_numPostResetCallbacks, stdDisplayDX9SystemTest_numPreResetCallbacks);
    ++stdDisplayDX9SystemTest_numPreResetCallbacks;
}

static bool stdDisplayDX9SystemTest_OnDevicePostReset(tSysDevice3D* pDevice)
{
    TEST_ASSERT_NOT_NULL(pDevice);
    TEST_ASSERT_EQUAL_PTR(stdDisplay_GetSystemDevice(), pDevice);
    TEST_ASSERT_EQUAL_size_t(stdDisplayDX9SystemTest_numPostResetCallbacks + 1u, stdDisplayDX9SystemTest_numPreResetCallbacks);
    ++stdDisplayDX9SystemTest_numPostResetCallbacks;
    return stdDisplayDX9SystemTest_bPostResetSucceeds;
}

static void stdDisplayDX9SystemTest_OnDeviceRelease(tSysDevice3D* pDevice)
{
    TEST_ASSERT_NOT_NULL(pDevice);
    TEST_ASSERT_EQUAL_PTR(stdDisplay_GetSystemDevice(), pDevice);
    ++stdDisplayDX9SystemTest_numReleaseCallbacks;
}

TEST_SETUP(stdDisplayDX9System)
{
    StdWin95SystemTest_Startup();
    stdDisplayDX9SystemTest_numPreResetCallbacks  = 0u;
    stdDisplayDX9SystemTest_numPostResetCallbacks = 0u;
    stdDisplayDX9SystemTest_numReleaseCallbacks   = 0u;
    stdDisplayDX9SystemTest_bPostResetSucceeds = true;
}

TEST_TEAR_DOWN(stdDisplayDX9System)
{
    StdWin95SystemTest_Shutdown();
}

static uint32_t stdDisplayDX9SystemTest_ExpectedRGB565(uint16_t pixel, const ColorInfo* pColorInfo)
{
    uint8_t red   = (uint8_t)(((pixel >> 11) & 0x1Fu) << 3);
    uint8_t green = (uint8_t)(((pixel >> 5) & 0x3Fu) << 2);
    uint8_t blue  = (uint8_t)((pixel & 0x1Fu) << 3);

    red   |= (uint8_t)(red >> 5);
    green |= (uint8_t)(green >> 6);
    blue  |= (uint8_t)(blue >> 5);

    return stdColor_EncodeRGB(pColorInfo, red, green, blue);
}

static void stdDisplayDX9SystemTest_AssertVBufferPixelNear(tVBuffer* pVBuffer, uint32_t x, uint32_t y, uint8_t expectedRed, uint8_t expectedGreen, uint8_t expectedBlue)
{
    uint32_t pixel = 0u;
    size_t bytesPerPixel = pVBuffer->rasterInfo.colorInfo.bpp / 8u;
    const uint8_t* pSrc = &pVBuffer->pPixels[(size_t)y * pVBuffer->rasterInfo.rowSize + (size_t)x * bytesPerPixel];

    TEST_ASSERT_TRUE(bytesPerPixel > 0u && bytesPerPixel <= sizeof(pixel));
    STD_COPYMEM(&pixel, pSrc, bytesPerPixel);

    uint8_t red;
    uint8_t green;
    uint8_t blue;
    stdColor_DecodeRGB(pixel, &pVBuffer->rasterInfo.colorInfo, &red, &green, &blue);
    TEST_ASSERT_UINT8_WITHIN(8u, expectedRed, red);
    TEST_ASSERT_UINT8_WITHIN(8u, expectedGreen, green);
    TEST_ASSERT_UINT8_WITHIN(8u, expectedBlue, blue);
}

static void stdDisplayDX9SystemTest_ExerciseHardwareVBuffer(int bUseVideoMemory)
{
    tRasterInfo rasterInfo;
    StdRect rect = { 4, 3, 8, 4 };

    STD_ZEROMEM(&rasterInfo, sizeof(rasterInfo));
    rasterInfo.width     = 32u;
    rasterInfo.height    = 16u;
    rasterInfo.colorInfo = stdColor_cfRGB8888;

    tVBuffer* pVBuffer = stdDisplay_VBufferNew(&rasterInfo, 1, bUseVideoMemory);
    TEST_ASSERT_NOT_NULL(pVBuffer);
    TEST_ASSERT_EQUAL_INT(VBUFFER_HARDWARE, pVBuffer->type);
    TEST_ASSERT_EQUAL_INT(bUseVideoMemory, pVBuffer->bVideoMemory);
    TEST_ASSERT_EQUAL_INT(bUseVideoMemory ? D3DPOOL_DEFAULT : D3DPOOL_SYSTEMMEM, pVBuffer->surface.desc.Pool);

    uint32_t blue = stdColor_EncodeRGB(&pVBuffer->rasterInfo.colorInfo, 0u, 0u, 255u);
    uint32_t green = stdColor_EncodeRGB(&pVBuffer->rasterInfo.colorInfo, 0u, 255u, 0u);
    TEST_ASSERT_TRUE(stdDisplay_VBufferFill(pVBuffer, blue, NULL));
    TEST_ASSERT_TRUE(stdDisplay_VBufferFill(pVBuffer, green, &rect));

    TEST_ASSERT_TRUE(stdDisplay_VBufferLock(pVBuffer));
    uint8_t* pFirstLock = pVBuffer->pPixels;
    TEST_ASSERT_NOT_NULL(pFirstLock);
    TEST_ASSERT_TRUE(stdDisplay_VBufferLock(pVBuffer));
    TEST_ASSERT_EQUAL_PTR(pFirstLock, pVBuffer->pPixels);
    TEST_ASSERT_EQUAL_size_t(2u, pVBuffer->lockRefCount);

    stdDisplayDX9SystemTest_AssertVBufferPixelNear(pVBuffer, 1u, 1u, 0u, 0u, 255u);
    stdDisplayDX9SystemTest_AssertVBufferPixelNear(pVBuffer, 5u, 4u, 0u, 255u, 0u);

    TEST_ASSERT_EQUAL_INT(1, stdDisplay_VBufferUnlock(pVBuffer));
    TEST_ASSERT_EQUAL_size_t(1u, pVBuffer->lockRefCount);
    TEST_ASSERT_EQUAL_PTR(pFirstLock, pVBuffer->pPixels);
    TEST_ASSERT_EQUAL_INT(0, stdDisplay_VBufferUnlock(pVBuffer));
    TEST_ASSERT_EQUAL_size_t(0u, pVBuffer->lockRefCount);
    TEST_ASSERT_NULL(pVBuffer->pPixels);

    stdDisplay_VBufferFree(pVBuffer);
}

static void stdDisplayDX9SystemTest_RequireDisplayOpenWithMSAA(bool bEnabled, int samples)
{
    StdWin95SystemTest_RequireHiddenWindow();
    StdWin95SystemTest_RequireDisplayConfig();

    TEST_ASSERT_TRUE(stdConfig_SetBool(STD3D_CFG_MSAAENABLED, bEnabled));
    TEST_ASSERT_TRUE(stdConfig_SetInt(STD3D_CFG_MSAASAMPLES, samples));

    StdWin95SystemTest_RequireDisplayDeviceOpen();
}

static bool stdDisplayDX9SystemTest_FindWindowedDepthFormat(UINT* pAdapter, D3DDISPLAYMODE* pDesktopMode, D3DFORMAT* pDepthFormat)
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

    for ( size_t i = 0u; i < STD_ARRAYLEN(stdDisplayDX9SystemTest_aDepthFormats); ++i )
    {
        D3DFORMAT depthFormat = stdDisplayDX9SystemTest_aDepthFormats[i];
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

static bool stdDisplayDX9SystemTest_IsWindowedMSAASupported(D3DMULTISAMPLE_TYPE sampleType)
{
    LPDIRECT3D9 pD3D = stdDisplay_GetDirect3D();
    if ( !pD3D )
    {
        return false;
    }

    UINT adapter;
    D3DDISPLAYMODE desktopMode;
    D3DFORMAT depthFormat;
    if ( !stdDisplayDX9SystemTest_FindWindowedDepthFormat(&adapter, &desktopMode, &depthFormat) )
    {
        return false;
    }

    DWORD colorQualityLevels = 0u;
    HRESULT colorResult = IDirect3D9_CheckDeviceMultiSampleType(
        pD3D,
        adapter,
        D3DDEVTYPE_HAL,
        desktopMode.Format,
        TRUE,
        sampleType,
        &colorQualityLevels
    );

    DWORD depthQualityLevels = 0u;
    HRESULT depthResult = IDirect3D9_CheckDeviceMultiSampleType(
        pD3D,
        adapter,
        D3DDEVTYPE_HAL,
        depthFormat,
        TRUE,
        sampleType,
        &depthQualityLevels
    );

    return SUCCEEDED(colorResult)
        && SUCCEEDED(depthResult)
        && colorQualityLevels > 0u
        && depthQualityLevels > 0u;
}

static D3DPRESENT_PARAMETERS stdDisplayDX9SystemTest_GetPresentParameters(void)
{
    IDirect3DSwapChain9* pSwapChain = NULL;
    TEST_ASSERT_EQUAL_HEX32(D3D_OK, IDirect3DDevice9_GetSwapChain(stdDisplay_GetSystemDevice(), 0u, &pSwapChain));
    TEST_ASSERT_NOT_NULL(pSwapChain);

    D3DPRESENT_PARAMETERS params;
    STD_ZEROMEM(&params, sizeof(params));
    TEST_ASSERT_EQUAL_HEX32(D3D_OK, IDirect3DSwapChain9_GetPresentParameters(pSwapChain, &params));
    IDirect3DSwapChain9_Release(pSwapChain);
    return params;
}

static int stdDisplayDX9SystemTest_HighestSupportedWindowedMSAASampleCount(void)
{
    static const struct
    {
        int sampleCount;
        D3DMULTISAMPLE_TYPE sampleType;
    } aSampleTypes[] =
    {
        { 16, D3DMULTISAMPLE_16_SAMPLES },
        { 8,  D3DMULTISAMPLE_8_SAMPLES  },
        { 4,  D3DMULTISAMPLE_4_SAMPLES  },
        { 2,  D3DMULTISAMPLE_2_SAMPLES  }
    };

    for ( size_t i = 0u; i < STD_ARRAYLEN(aSampleTypes); ++i )
    {
        if ( stdDisplayDX9SystemTest_IsWindowedMSAASupported(aSampleTypes[i].sampleType) )
        {
            return aSampleTypes[i].sampleCount;
        }
    }

    return 0;
}

TEST(stdDisplayDX9System, TestDisplayStartupOpenAndCloseDevice)
{
    StdDisplayDevice device;

    StdWin95SystemTest_RequireHiddenWindow();
    StdWin95SystemTest_RequireDisplayDeviceOpen();

    STD_ZEROMEM(&device, sizeof(device));
    TEST_ASSERT_TRUE(stdDisplay_IsOpen());
    TEST_ASSERT_TRUE(stdDisplay_GetNumDevices() > 0u);
    TEST_ASSERT_EQUAL_INT(0, stdDisplay_GetCurrentDevice(&device));
    TEST_ASSERT_TRUE(stdDisplay_GetNumVideoModes() > 0u);
    TEST_ASSERT_NOT_NULL(stdDisplay_GetDirect3D());
    TEST_ASSERT_NULL(stdDisplay_GetSystemDevice());

    stdDisplay_Close();
    StdWin95SystemTest_SetDisplayOpen(false);
    TEST_ASSERT_FALSE(stdDisplay_IsOpen());
}

TEST(stdDisplayDX9System, TestWindowedDisplayModeCreatesBackBuffer)
{
    uint32_t width  = 0u;
    uint32_t height = 0u;

    StdWin95SystemTest_RequireHiddenWindow();
    StdWin95SystemTest_RequireWindowedDisplayMode();

    TEST_ASSERT_FALSE(stdDisplay_IsFullscreen());
    stdDisplay_GetBackBufferSize(&width, &height);
    TEST_ASSERT_EQUAL_UINT32(320u, width);
    TEST_ASSERT_EQUAL_UINT32(240u, height);
    TEST_ASSERT_NOT_NULL(stdDisplay_GetSystemDevice());
    TEST_ASSERT_EQUAL_INT(D3DMULTISAMPLE_NONE, stdDisplay_g_frontBuffer.surface.desc.MultiSampleType);
    TEST_ASSERT_EQUAL_INT(D3DMULTISAMPLE_NONE, stdDisplay_g_backBuffer.surface.desc.MultiSampleType);
    TEST_ASSERT_EQUAL_PTR(
        stdDisplay_g_frontBuffer.surface.pSysSurface,
        stdDisplay_g_backBuffer.surface.pSysSurface
    );
    TEST_ASSERT_EQUAL_UINT32(width, stdDisplay_g_backBuffer.rasterInfo.rowWidth);
    TEST_ASSERT_EQUAL_UINT32(
        width * (stdDisplay_g_backBuffer.rasterInfo.colorInfo.bpp / 8u),
        stdDisplay_g_backBuffer.rasterInfo.rowSize
    );
    TEST_ASSERT_EQUAL_UINT32(
        stdDisplay_g_backBuffer.rasterInfo.rowSize * height,
        stdDisplay_g_backBuffer.rasterInfo.size
    );
    TEST_ASSERT_EQUAL_INT(0, stdDisplay_BackBufferFill(0u, NULL));
}

TEST(stdDisplayDX9System, TestTwoBackBufferWindowedRequestPresentsRepeatedly)
{
    StdWin95SystemTest_RequireHiddenWindow();
    StdWin95SystemTest_RequireDisplayDeviceOpen();

    stdDisplay_SetDefaultResolution(320u, 240u);
    TEST_ASSERT_EQUAL_INT(0, stdDisplay_SetMode(0u, 0, 2u));

    IDirect3DSwapChain9* pSwapChain = NULL;
    TEST_ASSERT_EQUAL_HEX32(D3D_OK, IDirect3DDevice9_GetSwapChain(stdDisplay_GetSystemDevice(), 0u, &pSwapChain));
    TEST_ASSERT_NOT_NULL(pSwapChain);

    D3DPRESENT_PARAMETERS params;
    STD_ZEROMEM(&params, sizeof(params));
    TEST_ASSERT_EQUAL_HEX32(D3D_OK, IDirect3DSwapChain9_GetPresentParameters(pSwapChain, &params));
    // Windowed mode intentionally uses a single swap-chain backbuffer; fullscreen coverage verifies the requested count.
    TEST_ASSERT_EQUAL_UINT32(1u, params.BackBufferCount);
    IDirect3DSwapChain9_Release(pSwapChain);

    for ( uint32_t frame = 0u; frame < 6u; ++frame )
    {
        uint32_t color = frame & 1u ? 0x000000FFu : 0x0000FF00u;
        TEST_ASSERT_EQUAL_INT(0, stdDisplay_BackBufferFill(color, NULL));
        TEST_ASSERT_EQUAL_INT(0, stdDisplay_Update());
    }
}

TEST(stdDisplayDX9System, TestWindowedDisplayModeHandlesSupportedMSAAReadback)
{
    stdDisplayDX9SystemTest_RequireDisplayOpenWithMSAA(true, 2);

    if ( !stdDisplayDX9SystemTest_IsWindowedMSAASupported(D3DMULTISAMPLE_2_SAMPLES) )
    {
        TEST_IGNORE_MESSAGE("Windowed 2x MSAA is unavailable on this host.");
    }

    stdDisplay_SetDefaultResolution(320u, 240u);
    if ( stdDisplay_SetMode(0u, 0, 1u) )
    {
        TEST_IGNORE_MESSAGE("Windowed DX9 MSAA display mode unavailable on this host.");
    }

    D3DPRESENT_PARAMETERS params = stdDisplayDX9SystemTest_GetPresentParameters();
    TEST_ASSERT_EQUAL_INT(D3DMULTISAMPLE_NONE, params.MultiSampleType);
    TEST_ASSERT_EQUAL_UINT32(0u, params.MultiSampleQuality);
    TEST_ASSERT_NOT_EQUAL_UINT32(0u, params.Flags & D3DPRESENTFLAG_LOCKABLE_BACKBUFFER);
    TEST_ASSERT_EQUAL_INT(D3DMULTISAMPLE_NONE, stdDisplay_g_frontBuffer.surface.desc.MultiSampleType);
    TEST_ASSERT_EQUAL_INT(D3DMULTISAMPLE_2_SAMPLES, stdDisplay_g_backBuffer.surface.desc.MultiSampleType);
    TEST_ASSERT_TRUE(
        stdDisplay_g_frontBuffer.surface.pSysSurface
        != stdDisplay_g_backBuffer.surface.pSysSurface
    );

    IDirect3DSurface9* pPresentationBuffer = NULL;
    TEST_ASSERT_EQUAL_HEX32(
        D3D_OK,
        IDirect3DDevice9_GetBackBuffer(
            stdDisplay_GetSystemDevice(),
            0u,
            0u,
            D3DBACKBUFFER_TYPE_MONO,
            &pPresentationBuffer
        )
    );
    TEST_ASSERT_EQUAL_PTR(stdDisplay_g_frontBuffer.surface.pSysSurface, pPresentationBuffer);
    IDirect3DSurface9_Release(pPresentationBuffer);

    UINT adapter;
    D3DDISPLAYMODE desktopMode;
    tSysPixelFormat zFormat;
    TEST_ASSERT_TRUE(stdDisplayDX9SystemTest_FindWindowedDepthFormat(&adapter, &desktopMode, &zFormat));
    TEST_ASSERT_EQUAL_INT(0, stdDisplay_CreateZBuffer(&zFormat, 0));
    TEST_ASSERT_EQUAL_INT(0, stdDisplay_BackBufferFill(0u, NULL));
    TEST_ASSERT_EQUAL_INT(0, stdDisplay_Update());

    StdRect rect = { 32, 24, 96, 72 };
    StdVideoMode mode;
    STD_ZEROMEM(&mode, sizeof(mode));
    TEST_ASSERT_EQUAL_INT(0, stdDisplay_GetCurrentVideoMode(&mode));

    uint32_t red = stdColor_EncodeRGB(&mode.rasterInfo.colorInfo, 255u, 0u, 0u);
    TEST_ASSERT_EQUAL_INT(0, stdDisplay_BackBufferFill(red, &rect));
    Std3DSystemTest_AssertBackBufferRectNear(40u, 32u, 16u, 16u, 255u, 0u, 0u, 8u);
}

TEST(stdDisplayDX9System, TestWindowedMSAACpuWritesRemainOnPresentationPath)
{
    stdDisplayDX9SystemTest_RequireDisplayOpenWithMSAA(true, 2);

    if ( !stdDisplayDX9SystemTest_IsWindowedMSAASupported(D3DMULTISAMPLE_2_SAMPLES) )
    {
        TEST_IGNORE_MESSAGE("Windowed 2x MSAA is unavailable on this host.");
    }

    stdDisplay_SetDefaultResolution(320u, 240u);
    if ( stdDisplay_SetMode(0u, 0, 1u) )
    {
        TEST_IGNORE_MESSAGE("Windowed DX9 MSAA display mode unavailable on this host.");
    }

    StdVideoMode mode;
    STD_ZEROMEM(&mode, sizeof(mode));
    TEST_ASSERT_EQUAL_INT(0, stdDisplay_GetCurrentVideoMode(&mode));

    void* pSurface = NULL;
    uint32_t width = 0u;
    uint32_t height = 0u;
    int32_t pitch = 0;
    TEST_ASSERT_EQUAL_INT(0, stdDisplay_LockBackBuffer(&pSurface, &width, &height, &pitch));
    TEST_ASSERT_NOT_NULL(pSurface);
    TEST_ASSERT_TRUE(width > 12u && height > 12u);

    size_t bytesPerPixel = mode.rasterInfo.colorInfo.bpp / 8u;
    TEST_ASSERT_TRUE(bytesPerPixel > 0u && bytesPerPixel <= sizeof(uint32_t));

    uint32_t expectedPixel = stdColor_EncodeRGB(&mode.rasterInfo.colorInfo, 17u, 83u, 201u);
    size_t rowPitch = pitch < 0 ? (size_t)-pitch : (size_t)pitch;
    uint8_t* pRow = pitch < 0
        ? (uint8_t*)pSurface + (height - 1u - 12u) * rowPitch
        : (uint8_t*)pSurface + 12u * rowPitch;
    STD_COPYMEM(&pRow[12u * bytesPerPixel], &expectedPixel, bytesPerPixel);
    stdDisplay_UnlockBackBuffer();

    pSurface = NULL;
    TEST_ASSERT_EQUAL_INT(0, stdDisplay_LockBackBuffer(&pSurface, &width, &height, &pitch));
    rowPitch = pitch < 0 ? (size_t)-pitch : (size_t)pitch;
    pRow = pitch < 0
        ? (uint8_t*)pSurface + (height - 1u - 12u) * rowPitch
        : (uint8_t*)pSurface + 12u * rowPitch;

    uint32_t actualPixel = 0u;
    STD_COPYMEM(&actualPixel, &pRow[12u * bytesPerPixel], bytesPerPixel);
    TEST_ASSERT_EQUAL_HEX32(expectedPixel, actualPixel);
    stdDisplay_UnlockBackBuffer();

    TEST_ASSERT_EQUAL_INT(0, stdDisplay_Update());

    uint32_t green = stdColor_EncodeRGB(&mode.rasterInfo.colorInfo, 0u, 255u, 0u);
    TEST_ASSERT_EQUAL_INT(0, stdDisplay_BackBufferFill(green, NULL));
    Std3DSystemTest_AssertBackBufferPixelNear(12u, 12u, 0u, 255u, 0u, 8u);
}

TEST(stdDisplayDX9System, TestWindowedMSAAUnsupportedSampleFallsBack)
{
    stdDisplayDX9SystemTest_RequireDisplayOpenWithMSAA(true, 16);

    if ( stdDisplayDX9SystemTest_IsWindowedMSAASupported(D3DMULTISAMPLE_16_SAMPLES) )
    {
        TEST_IGNORE_MESSAGE("Windowed 16x MSAA is supported on this host, so fallback is not exercised.");
    }

    int expectedFallbackSamples = stdDisplayDX9SystemTest_HighestSupportedWindowedMSAASampleCount();
    if ( expectedFallbackSamples == 0 )
    {
        TEST_IGNORE_MESSAGE("No windowed MSAA fallback sample count is supported on this host.");
    }

    stdDisplay_SetDefaultResolution(320u, 240u);
    if ( stdDisplay_SetMode(0u, 0, 1u) )
    {
        TEST_IGNORE_MESSAGE("Windowed DX9 display mode unavailable while testing MSAA fallback.");
    }

    TEST_ASSERT_EQUAL_INT(expectedFallbackSamples, stdConfig_GetInt(STD3D_CFG_MSAASAMPLES, 16));
    TEST_ASSERT_EQUAL_INT(0, stdDisplay_BackBufferFill(0u, NULL));
    TEST_ASSERT_EQUAL_INT(0, stdDisplay_Update());
}

TEST(stdDisplayDX9System, TestEachEnumeratedDisplayDeviceOpensWindowedMode)
{
    StdWin95SystemTest_RequireHiddenWindow();
    StdWin95SystemTest_RequireDisplayConfig();

    TEST_ASSERT_TRUE(stdConfig_SetBool(STD3D_CFG_MSAAENABLED, false));
    StdWin95SystemTest_RequireDisplayStartup();

    size_t numOpenedDevices = 0u;
    size_t numModeSetDevices = 0u;
    for ( size_t deviceNum = 0u; deviceNum < stdDisplay_GetNumDevices(); ++deviceNum )
    {
        if ( !stdDisplay_Open(deviceNum) )
        {
            continue;
        }

        StdWin95SystemTest_SetDisplayOpen(true);
        ++numOpenedDevices;

        stdDisplay_SetDefaultResolution(320u, 240u);
        if ( !stdDisplay_SetMode(0u, 0, 1u) )
        {
            TEST_ASSERT_EQUAL_INT(0, stdDisplay_BackBufferFill(0u, NULL));
            TEST_ASSERT_EQUAL_INT(0, stdDisplay_Update());
            ++numModeSetDevices;
        }

        stdDisplay_Close();
        StdWin95SystemTest_SetDisplayOpen(false);
    }

    TEST_ASSERT_TRUE(numOpenedDevices > 0u);
    TEST_ASSERT_TRUE(numModeSetDevices > 0u);
}

TEST(stdDisplayDX9System, TestDisableVSyncResetsDeviceAndKeepsBackBufferUsable)
{
    StdWin95SystemTest_RequireHiddenWindow();
    StdWin95SystemTest_RequireWindowedDisplayMode();

    stdDisplay_RegisterDevicePreResetCallback(stdDisplayDX9SystemTest_OnDevicePreReset);
    stdDisplay_RegisterDevicePostResetCallback(stdDisplayDX9SystemTest_OnDevicePostReset);

    D3DPRESENT_PARAMETERS retainedParams;
    STD_ZEROMEM(&retainedParams, sizeof(retainedParams));
    stdDisplay_TestGetPresentParameters(&retainedParams);

    stdDisplay_DisableVSync(true);
    TEST_ASSERT_EQUAL_size_t(1u, stdDisplayDX9SystemTest_numPreResetCallbacks);
    TEST_ASSERT_EQUAL_size_t(1u, stdDisplayDX9SystemTest_numPostResetCallbacks);
    TEST_ASSERT_EQUAL_INT(0, stdDisplay_BackBufferFill(0u, NULL));
    TEST_ASSERT_EQUAL_INT(0, stdDisplay_Update());

    D3DPRESENT_PARAMETERS immediateParams;
    STD_ZEROMEM(&immediateParams, sizeof(immediateParams));
    stdDisplay_TestGetPresentParameters(&immediateParams);

    D3DPRESENT_PARAMETERS expectedParams = retainedParams;
    expectedParams.PresentationInterval = D3DPRESENT_INTERVAL_IMMEDIATE;
    TEST_ASSERT_EQUAL_MEMORY(&expectedParams, &immediateParams, sizeof(expectedParams));

    stdDisplay_DisableVSync(false);
    TEST_ASSERT_EQUAL_size_t(2u, stdDisplayDX9SystemTest_numPreResetCallbacks);
    TEST_ASSERT_EQUAL_size_t(2u, stdDisplayDX9SystemTest_numPostResetCallbacks);
    TEST_ASSERT_EQUAL_INT(0, stdDisplay_BackBufferFill(0u, NULL));
    TEST_ASSERT_EQUAL_INT(0, stdDisplay_Update());

    D3DPRESENT_PARAMETERS restoredParams;
    STD_ZEROMEM(&restoredParams, sizeof(restoredParams));
    stdDisplay_TestGetPresentParameters(&restoredParams);
    TEST_ASSERT_EQUAL_MEMORY(&retainedParams, &restoredParams, sizeof(retainedParams));
}

TEST(stdDisplayDX9System, TestDeviceResetRejectsHeldLockWithoutChangingSettings)
{
    void* pSurface;
    uint32_t width;
    uint32_t height;
    int32_t pitch;

    StdWin95SystemTest_RequireHiddenWindow();
    StdWin95SystemTest_RequireWindowedDisplayMode();

    D3DPRESENT_PARAMETERS retainedParams;
    STD_ZEROMEM(&retainedParams, sizeof(retainedParams));
    stdDisplay_TestGetPresentParameters(&retainedParams);

    bool bDisable = retainedParams.PresentationInterval != D3DPRESENT_INTERVAL_IMMEDIATE;
    stdDisplay_RegisterDevicePreResetCallback(stdDisplayDX9SystemTest_OnDevicePreReset);
    stdDisplay_RegisterDevicePostResetCallback(stdDisplayDX9SystemTest_OnDevicePostReset);

    TEST_ASSERT_EQUAL_INT(0, stdDisplay_LockBackBufferReadOnly(&pSurface, &width, &height, &pitch));
    stdDisplay_DisableVSync(bDisable);
    TEST_ASSERT_EQUAL_size_t(0u, stdDisplayDX9SystemTest_numPreResetCallbacks);
    TEST_ASSERT_EQUAL_size_t(0u, stdDisplayDX9SystemTest_numPostResetCallbacks);

    D3DPRESENT_PARAMETERS rejectedParams;
    STD_ZEROMEM(&rejectedParams, sizeof(rejectedParams));
    stdDisplay_TestGetPresentParameters(&rejectedParams);
    TEST_ASSERT_EQUAL_MEMORY(&retainedParams, &rejectedParams, sizeof(retainedParams));
    stdDisplay_UnlockBackBuffer();

    stdDisplay_DisableVSync(bDisable);
    TEST_ASSERT_EQUAL_size_t(1u, stdDisplayDX9SystemTest_numPreResetCallbacks);
    TEST_ASSERT_EQUAL_size_t(1u, stdDisplayDX9SystemTest_numPostResetCallbacks);

    stdDisplay_DisableVSync(!bDisable);
    TEST_ASSERT_EQUAL_size_t(2u, stdDisplayDX9SystemTest_numPreResetCallbacks);
    TEST_ASSERT_EQUAL_size_t(2u, stdDisplayDX9SystemTest_numPostResetCallbacks);
}

TEST(stdDisplayDX9System, TestHardwareVBuffersRemainUsableAcrossDeviceReset)
{
    tRasterInfo rasterInfo;

    StdWin95SystemTest_RequireHiddenWindow();
    StdWin95SystemTest_RequireWindowedDisplayMode();

    STD_ZEROMEM(&rasterInfo, sizeof(rasterInfo));
    rasterInfo.width     = 32u;
    rasterInfo.height    = 16u;
    rasterInfo.colorInfo = stdColor_cfRGB8888;

    tVBuffer* pSystemVBuffer = stdDisplay_VBufferNew(&rasterInfo, 1, 0);
    tVBuffer* pVideoVBuffer  = stdDisplay_VBufferNew(&rasterInfo, 1, 1);
    TEST_ASSERT_NOT_NULL(pSystemVBuffer);
    TEST_ASSERT_NOT_NULL(pVideoVBuffer);

    IDirect3DSurface9* pSystemSurface = pSystemVBuffer->surface.pSysSurface;
    uint32_t blue = stdColor_EncodeRGB(&rasterInfo.colorInfo, 0u, 0u, 255u);
    TEST_ASSERT_TRUE(stdDisplay_VBufferFill(pSystemVBuffer, blue, NULL));
    TEST_ASSERT_TRUE(stdDisplay_VBufferFill(pVideoVBuffer, blue, NULL));

    D3DPRESENT_PARAMETERS retainedParams;
    STD_ZEROMEM(&retainedParams, sizeof(retainedParams));
    stdDisplay_TestGetPresentParameters(&retainedParams);
    bool bDisable = retainedParams.PresentationInterval != D3DPRESENT_INTERVAL_IMMEDIATE;
    stdDisplay_DisableVSync(bDisable);

    TEST_ASSERT_EQUAL_PTR(pSystemSurface, pSystemVBuffer->surface.pSysSurface);
    TEST_ASSERT_EQUAL_INT(D3DPOOL_SYSTEMMEM, pSystemVBuffer->surface.desc.Pool);
    TEST_ASSERT_NOT_NULL(pVideoVBuffer->surface.pSysSurface);
    TEST_ASSERT_EQUAL_INT(D3DPOOL_DEFAULT, pVideoVBuffer->surface.desc.Pool);

    uint32_t green = stdColor_EncodeRGB(&rasterInfo.colorInfo, 0u, 255u, 0u);
    TEST_ASSERT_TRUE(stdDisplay_VBufferFill(pSystemVBuffer, green, NULL));
    TEST_ASSERT_TRUE(stdDisplay_VBufferFill(pVideoVBuffer, green, NULL));
    TEST_ASSERT_TRUE(stdDisplay_VBufferLock(pVideoVBuffer));
    stdDisplayDX9SystemTest_AssertVBufferPixelNear(pVideoVBuffer, 1u, 1u, 0u, 255u, 0u);
    TEST_ASSERT_EQUAL_INT(0, stdDisplay_VBufferUnlock(pVideoVBuffer));

    stdDisplay_VBufferFree(pVideoVBuffer);
    stdDisplay_VBufferFree(pSystemVBuffer);
    stdDisplay_DisableVSync(!bDisable);
}

TEST(stdDisplayDX9System, TestBackBufferLockSaveScreenAndMemoryQueries)
{
    const char* pScreenPath = "stdDisplayDX9SystemTest_screen.bmp";
    void* pSurface;
    void* pSecondSurface;
    uint32_t width;
    uint32_t height;
    uint32_t secondWidth;
    uint32_t secondHeight;
    int32_t pitch;
    int32_t secondPitch;
    size_t totalMemory;
    size_t freeMemory;
    StdRect rect = { 2, 3, 8, 7 };

    StdGeneralTest_DeleteFile(pScreenPath);

    StdWin95SystemTest_RequireHiddenWindow();
    StdWin95SystemTest_RequireWindowedDisplayMode();

    TEST_ASSERT_EQUAL_INT(0, stdDisplay_GetTextureMemory(&totalMemory, &freeMemory));
    TEST_ASSERT_TRUE(totalMemory > 0u);
    TEST_ASSERT_TRUE(freeMemory > 0u);
    TEST_ASSERT_EQUAL_INT(0, stdDisplay_GetTotalMemory(&totalMemory, &freeMemory));
    TEST_ASSERT_TRUE(totalMemory > 0u);

    TEST_ASSERT_EQUAL_INT(0, stdDisplay_BackBufferFill(0u, &rect));
    TEST_ASSERT_EQUAL_INT(0, stdDisplay_Update());
    stdDisplay_Refresh(0);
    stdDisplay_Refresh(1);

    TEST_ASSERT_EQUAL_INT(0, stdDisplay_LockBackBufferReadOnly(&pSurface, &width, &height, &pitch));
    TEST_ASSERT_NOT_NULL(pSurface);
    TEST_ASSERT_EQUAL_UINT32(320u, width);
    TEST_ASSERT_EQUAL_UINT32(240u, height);
    TEST_ASSERT_NOT_EQUAL_INT32(0, pitch);

    // A D3DLOCK_READONLY lock cannot be upgraded in place without violating the surface contract.
    TEST_ASSERT_EQUAL_INT(1, stdDisplay_LockBackBuffer(&pSecondSurface, &secondWidth, &secondHeight, &secondPitch));

    TEST_ASSERT_EQUAL_INT(0, stdDisplay_LockBackBufferReadOnly(&pSecondSurface, &secondWidth, &secondHeight, &secondPitch));
    TEST_ASSERT_EQUAL_PTR(pSurface, pSecondSurface);
    TEST_ASSERT_EQUAL_UINT32(width, secondWidth);
    TEST_ASSERT_EQUAL_UINT32(height, secondHeight);
    TEST_ASSERT_EQUAL_INT32(pitch, secondPitch);
    stdDisplay_UnlockBackBuffer();
    stdDisplay_UnlockBackBuffer();

    TEST_ASSERT_EQUAL_INT(0, stdDisplay_SaveScreen(pScreenPath));
    TEST_ASSERT_TRUE(stdFileSize(pScreenPath) > 0u);
    StdGeneralTest_DeleteFile(pScreenPath);
}

TEST(stdDisplayDX9System, TestBackBufferFillPixelsAndRectReadback)
{
    StdVideoMode mode;
    StdRect rect = { 40, 30, 60, 50 };

    StdWin95SystemTest_RequireHiddenWindow();
    StdWin95SystemTest_RequireWindowedDisplayMode();

    STD_ZEROMEM(&mode, sizeof(mode));
    TEST_ASSERT_EQUAL_INT(0, stdDisplay_GetCurrentVideoMode(&mode));

    uint32_t blue  = stdColor_EncodeRGB(&mode.rasterInfo.colorInfo, 0u, 0u, 255u);
    uint32_t green = stdColor_EncodeRGB(&mode.rasterInfo.colorInfo, 0u, 255u, 0u);
    TEST_ASSERT_EQUAL_INT(0, stdDisplay_BackBufferFill(blue, NULL));
    TEST_ASSERT_EQUAL_INT(0, stdDisplay_BackBufferFill(green, &rect));

    Std3DSystemTest_AssertBackBufferPixelNear(8u, 8u, 0u, 0u, 255u, 8u);
    Std3DSystemTest_AssertBackBufferRectNear(45u, 35u, 10u, 10u, 0u, 255u, 0u, 8u);
    Std3DSystemTest_AssertBackBufferMatchesBmpMask("display_fill_rect_mask.bmp", 8u);
}

TEST(stdDisplayDX9System, TestHardwareVBufferPoolsFillAndNestedLocks)
{
    StdWin95SystemTest_RequireHiddenWindow();
    StdWin95SystemTest_RequireWindowedDisplayMode();

    stdDisplayDX9SystemTest_ExerciseHardwareVBuffer(0);
    stdDisplayDX9SystemTest_ExerciseHardwareVBuffer(1);
}

TEST(stdDisplayDX9System, TestRepeatedDisplayLifecycle)
{
    StdWin95SystemTest_RequireHiddenWindow();
    StdWin95SystemTest_RequireDisplayConfig();

    for ( size_t iteration = 0u; iteration < 4u; ++iteration )
    {
        TEST_ASSERT_TRUE(stdDisplay_Startup());
        TEST_ASSERT_TRUE(stdDisplay_GetNumDevices() > 0u);
        TEST_ASSERT_TRUE(stdDisplay_Open(0u));
        stdDisplay_SetDefaultResolution(320u, 240u);
        TEST_ASSERT_EQUAL_INT(0, stdDisplay_SetMode(0u, 0, 1u));
        TEST_ASSERT_EQUAL_INT(0, stdDisplay_BackBufferFill((uint32_t)iteration, NULL));
        TEST_ASSERT_EQUAL_INT(0, stdDisplay_Update());
        stdDisplay_ClearMode();
        stdDisplay_Close();
        stdDisplay_Shutdown();
        TEST_ASSERT_FALSE(stdDisplay_IsOpen());
    }
}

TEST(stdDisplayDX9System, TestEnumerationRejectsInvalidIndexes)
{
    StdDisplayDevice device;
    StdVideoMode mode;

    StdWin95SystemTest_RequireHiddenWindow();
    StdWin95SystemTest_RequireDisplayDeviceOpen();

    STD_ZEROMEM(&device, sizeof(device));
    STD_ZEROMEM(&mode, sizeof(mode));

    TEST_ASSERT_NOT_NULL(stdDisplay_GetAllDevices());
    TEST_ASSERT_EQUAL_INT(1, stdDisplay_GetDevice(stdDisplay_GetNumDevices(), &device));
    TEST_ASSERT_EQUAL_INT(1, stdDisplay_GetVideoMode(stdDisplay_GetNumVideoModes(), &mode));
    TEST_ASSERT_EQUAL_INT(1, stdDisplay_GetCurrentVideoMode(&mode));
}

TEST(stdDisplayDX9System, TestZBufferCreationAndGdiReadbackHelpers)
{
    UINT adapter;
    D3DDISPLAYMODE desktopMode;
    tSysPixelFormat zFormat;
    HDC hdcBack;

    StdWin95SystemTest_RequireHiddenWindow();
    StdWin95SystemTest_RequireWindowedDisplayMode();

    TEST_ASSERT_TRUE(stdDisplayDX9SystemTest_FindWindowedDepthFormat(&adapter, &desktopMode, &zFormat));
    TEST_ASSERT_EQUAL_INT(0, stdDisplay_CreateZBuffer(&zFormat, 0));

    TEST_ASSERT_EQUAL_INT(0, stdDisplay_BackBufferFill(0u, NULL));
    hdcBack = stdDisplay_GetBackBufferDC();
    if ( hdcBack )
    {
        COLORREF pixel = GetPixel(hdcBack, 1, 1);
        TEST_ASSERT_NOT_EQUAL(CLR_INVALID, pixel);
        stdDisplay_ReleaseBackBufferDC(hdcBack);
    }
}

TEST(stdDisplayDX9System, TestDeviceReleaseCallbackRunsWhenModeIsCleared)
{
    StdWin95SystemTest_RequireHiddenWindow();
    StdWin95SystemTest_RequireWindowedDisplayMode();

    stdDisplay_RegisterDeviceReleaseCallback(stdDisplayDX9SystemTest_OnDeviceRelease);
    stdDisplay_ClearMode();
    TEST_ASSERT_EQUAL_size_t(1u, stdDisplayDX9SystemTest_numReleaseCallbacks);
}

TEST(stdDisplayDX9System, TestWindowedClipperAndGdiCompatibilityHelpers)
{
    HDC hdcFront;
    int canRenderWindowed;

    StdWin95SystemTest_RequireHiddenWindow();
    StdWin95SystemTest_RequireWindowedDisplayMode();

    canRenderWindowed = stdDisplay_CanRenderWindowed();
    TEST_ASSERT_TRUE(canRenderWindowed == 0 || canRenderWindowed == 1);
    TEST_ASSERT_EQUAL_INT(1, stdDisplay_SetBufferClipper(1));
    TEST_ASSERT_EQUAL_INT(S_OK, stdDisplay_RemoveBufferClipper(1));
    TEST_ASSERT_EQUAL_INT(1, stdDisplay_SetBufferClipper(0));
    TEST_ASSERT_EQUAL_INT(S_OK, stdDisplay_RemoveBufferClipper(0));
    TEST_ASSERT_EQUAL_INT(S_OK, stdDisplay_FlipToGDISurface());

    hdcFront = stdDisplay_GetFrontBufferDC();
    if ( hdcFront )
    {
        TEST_ASSERT_NOT_EQUAL(CLR_INVALID, GetPixel(hdcFront, 0, 0));
        stdDisplay_ReleaseFrontBufferDC(hdcFront);
    }
}

TEST(stdDisplayDX9System, TestEncodeFromRGB565UsesCurrentVideoModeFormat)
{
    static const uint16_t aPixels[] =
    {
        0x0000u,
        0xF800u,
        0x07E0u,
        0x001Fu,
        0xFFFFu,
        0x8410u
    };
    StdVideoMode mode;

    StdWin95SystemTest_RequireHiddenWindow();
    StdWin95SystemTest_RequireWindowedDisplayMode();

    STD_ZEROMEM(&mode, sizeof(mode));
    TEST_ASSERT_EQUAL_INT(0, stdDisplay_GetCurrentVideoMode(&mode));

    for ( size_t i = 0; i < STD_ARRAYLEN(aPixels); ++i )
    {
        uint32_t expected = stdDisplayDX9SystemTest_ExpectedRGB565(aPixels[i], &mode.rasterInfo.colorInfo);
        TEST_ASSERT_EQUAL_HEX32(expected, stdDisplay_EncodeFromRGB565(aPixels[i]));
    }
}

TEST(stdDisplayDX9System, TestPostResetCallbackFailureRollsBackSettingsAndCanRetry)
{
    StdWin95SystemTest_RequireHiddenWindow();
    StdWin95SystemTest_RequireWindowedDisplayMode();
    D3DPRESENT_PARAMETERS original;
    STD_ZEROMEM(&original, sizeof(original));
    stdDisplay_TestGetPresentParameters(&original);
    bool bDisable = original.PresentationInterval != D3DPRESENT_INTERVAL_IMMEDIATE;

    // A rejected post-reset callback must run in order and roll back all presentation settings.
    stdDisplay_RegisterDevicePreResetCallback(stdDisplayDX9SystemTest_OnDevicePreReset);
    stdDisplay_RegisterDevicePostResetCallback(stdDisplayDX9SystemTest_OnDevicePostReset);
    stdDisplayDX9SystemTest_bPostResetSucceeds = false;
    stdDisplay_DisableVSync(bDisable);
    TEST_ASSERT_EQUAL_size_t(1u, stdDisplayDX9SystemTest_numPreResetCallbacks);
    TEST_ASSERT_EQUAL_size_t(1u, stdDisplayDX9SystemTest_numPostResetCallbacks);

    D3DPRESENT_PARAMETERS rejected;
    STD_ZEROMEM(&rejected, sizeof(rejected));
    stdDisplay_TestGetPresentParameters(&rejected);
    TEST_ASSERT_EQUAL_MEMORY(&original, &rejected, sizeof(original));

    // A successful retry must apply the requested settings and leave the back buffer usable.
    stdDisplayDX9SystemTest_bPostResetSucceeds = true;
    stdDisplay_DisableVSync(bDisable);
    TEST_ASSERT_EQUAL_size_t(2u, stdDisplayDX9SystemTest_numPreResetCallbacks);
    TEST_ASSERT_EQUAL_size_t(2u, stdDisplayDX9SystemTest_numPostResetCallbacks);
    D3DPRESENT_PARAMETERS recovered;
    STD_ZEROMEM(&recovered, sizeof(recovered));
    stdDisplay_TestGetPresentParameters(&recovered);
    D3DPRESENT_PARAMETERS expected = original;
    expected.PresentationInterval = bDisable ? D3DPRESENT_INTERVAL_IMMEDIATE : D3DPRESENT_INTERVAL_DEFAULT;
    TEST_ASSERT_EQUAL_MEMORY(&expected, &recovered, sizeof(expected));
    TEST_ASSERT_EQUAL_INT(0, stdDisplay_BackBufferFill(0u, NULL));
    TEST_ASSERT_EQUAL_INT(0, stdDisplay_Update());

    // VSync preference survives display shutdown; restore it for following fixtures.
    stdDisplay_DisableVSync(!bDisable);
    D3DPRESENT_PARAMETERS restored;
    STD_ZEROMEM(&restored, sizeof(restored));
    stdDisplay_TestGetPresentParameters(&restored);
    TEST_ASSERT_EQUAL_MEMORY(&original, &restored, sizeof(original));
    TEST_ASSERT_EQUAL_size_t(3u, stdDisplayDX9SystemTest_numPreResetCallbacks);
    TEST_ASSERT_EQUAL_size_t(3u, stdDisplayDX9SystemTest_numPostResetCallbacks);
}

TEST_GROUP_RUNNER(stdDisplayDX9System)
{
    RUN_TEST_CASE(stdDisplayDX9System, TestPostResetCallbackFailureRollsBackSettingsAndCanRetry);
    RUN_TEST_CASE(stdDisplayDX9System, TestDisplayStartupOpenAndCloseDevice);
    RUN_TEST_CASE(stdDisplayDX9System, TestWindowedDisplayModeCreatesBackBuffer);
    RUN_TEST_CASE(stdDisplayDX9System, TestTwoBackBufferWindowedRequestPresentsRepeatedly);
    RUN_TEST_CASE(stdDisplayDX9System, TestWindowedDisplayModeHandlesSupportedMSAAReadback);
    RUN_TEST_CASE(stdDisplayDX9System, TestWindowedMSAACpuWritesRemainOnPresentationPath);
    RUN_TEST_CASE(stdDisplayDX9System, TestWindowedMSAAUnsupportedSampleFallsBack);
    RUN_TEST_CASE(stdDisplayDX9System, TestEachEnumeratedDisplayDeviceOpensWindowedMode);
    RUN_TEST_CASE(stdDisplayDX9System, TestDisableVSyncResetsDeviceAndKeepsBackBufferUsable);
    RUN_TEST_CASE(stdDisplayDX9System, TestDeviceResetRejectsHeldLockWithoutChangingSettings);
    RUN_TEST_CASE(stdDisplayDX9System, TestHardwareVBuffersRemainUsableAcrossDeviceReset);
    RUN_TEST_CASE(stdDisplayDX9System, TestBackBufferLockSaveScreenAndMemoryQueries);
    RUN_TEST_CASE(stdDisplayDX9System, TestBackBufferFillPixelsAndRectReadback);
    RUN_TEST_CASE(stdDisplayDX9System, TestHardwareVBufferPoolsFillAndNestedLocks);
    RUN_TEST_CASE(stdDisplayDX9System, TestRepeatedDisplayLifecycle);
    RUN_TEST_CASE(stdDisplayDX9System, TestEnumerationRejectsInvalidIndexes);
    RUN_TEST_CASE(stdDisplayDX9System, TestZBufferCreationAndGdiReadbackHelpers);
    RUN_TEST_CASE(stdDisplayDX9System, TestDeviceReleaseCallbackRunsWhenModeIsCleared);
    RUN_TEST_CASE(stdDisplayDX9System, TestWindowedClipperAndGdiCompatibilityHelpers);
    RUN_TEST_CASE(stdDisplayDX9System, TestEncodeFromRGB565UsesCurrentVideoModeFormat);
}

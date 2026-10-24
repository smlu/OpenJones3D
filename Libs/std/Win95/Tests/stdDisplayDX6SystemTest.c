#include <unity_fixture.h>

#include <stdint.h>

#include <std/General/std.h>
#include <std/General/stdColor.h>
#include <std/General/stdUtil.h>
#include <std/Win95/stdDisplay.h>

#include "std3DSystemTestSupport.h"
#include "stdGeneralTest.h"
#include "stdWin95SystemTestSupport.h"

TEST_GROUP(stdDisplayDX6System);

TEST_SETUP(stdDisplayDX6System)
{
    StdWin95SystemTest_Startup();
}

TEST_TEAR_DOWN(stdDisplayDX6System)
{
    StdWin95SystemTest_Shutdown();
}

static uint32_t stdDisplayDX6SystemTest_ExpectedRGB565(uint16_t pixel, const ColorInfo* pColorInfo)
{
#ifdef J3D_QOL_IMPROVEMENTS
    uint8_t red   = (uint8_t)(((pixel >> 11) & 0x1Fu) << 3);
    uint8_t green = (uint8_t)(((pixel >> 5) & 0x3Fu) << 2);
    uint8_t blue  = (uint8_t)((pixel & 0x1Fu) << 3);

    red   |= (uint8_t)(red >> 5);
    green |= (uint8_t)(green >> 6);
    blue  |= (uint8_t)(blue >> 5);
#else
    uint8_t red = 8u * (pixel >> 11);
    if ( (red & 8u) != 0u )
    {
        red |= 7u;
    }

    uint8_t green = 4u * (pixel >> 5);
    if ( (green & 4u) != 0u )
    {
        green |= 3u;
    }

    uint8_t blue = 8u * pixel;
    if ( (blue & 8u) != 0u )
    {
        blue |= 7u;
    }
#endif

    return stdColor_EncodeRGB(pColorInfo, red, green, blue);
}

static void stdDisplayDX6SystemTest_AssertVBufferPixelNear(tVBuffer* pVBuffer, uint32_t x, uint32_t y, uint8_t expectedRed, uint8_t expectedGreen, uint8_t expectedBlue)
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

static void stdDisplayDX6SystemTest_ExerciseHardwareVBuffer(int bUseVideoMemory)
{
    StdVideoMode mode;
    tRasterInfo rasterInfo;
    StdRect rect = { 4, 3, 8, 4 };

    STD_ZEROMEM(&mode, sizeof(mode));
    TEST_ASSERT_EQUAL_INT(0, stdDisplay_GetCurrentVideoMode(&mode));

    STD_ZEROMEM(&rasterInfo, sizeof(rasterInfo));
    rasterInfo.width     = 32u;
    rasterInfo.height    = 16u;
    rasterInfo.colorInfo = mode.rasterInfo.colorInfo;

    tVBuffer* pVBuffer = stdDisplay_VBufferNew(&rasterInfo, 1, bUseVideoMemory);
    TEST_ASSERT_NOT_NULL(pVBuffer);
    TEST_ASSERT_EQUAL_INT(VBUFFER_HARDWARE, pVBuffer->type);
    if ( !bUseVideoMemory )
    {
        TEST_ASSERT_FALSE(pVBuffer->bVideoMemory);
        TEST_ASSERT_NOT_EQUAL_UINT32(0u, pVBuffer->surface.desc.ddsCaps.dwCaps & DDSCAPS_SYSTEMMEMORY);
    }

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

    stdDisplayDX6SystemTest_AssertVBufferPixelNear(pVBuffer, 1u, 1u, 0u, 0u, 255u);
    stdDisplayDX6SystemTest_AssertVBufferPixelNear(pVBuffer, 5u, 4u, 0u, 255u, 0u);

    TEST_ASSERT_EQUAL_INT(1, stdDisplay_VBufferUnlock(pVBuffer));
    TEST_ASSERT_EQUAL_size_t(1u, pVBuffer->lockRefCount);
    TEST_ASSERT_EQUAL_PTR(pFirstLock, pVBuffer->pPixels);
    TEST_ASSERT_EQUAL_INT(0, stdDisplay_VBufferUnlock(pVBuffer));
    TEST_ASSERT_EQUAL_size_t(0u, pVBuffer->lockRefCount);
    TEST_ASSERT_NULL(pVBuffer->pPixels);

    stdDisplay_VBufferFree(pVBuffer);
}

TEST(stdDisplayDX6System, TestDisplayStartupOpenAndCloseDevice)
{
    StdDisplayDevice device;

    StdWin95SystemTest_RequireHiddenWindow();
    StdWin95SystemTest_RequireDisplayDeviceOpen();

    STD_ZEROMEM(&device, sizeof(device));
    TEST_ASSERT_TRUE(stdDisplay_IsOpen());
    TEST_ASSERT_TRUE(stdDisplay_GetNumDevices() > 0u);
    TEST_ASSERT_EQUAL_INT(0, stdDisplay_GetCurrentDevice(&device));
    TEST_ASSERT_TRUE(stdDisplay_GetNumVideoModes() > 0u);
    TEST_ASSERT_NOT_NULL(stdDisplay_GetSystemDevice());

    stdDisplay_Close();
    StdWin95SystemTest_SetDisplayOpen(false);
    TEST_ASSERT_FALSE(stdDisplay_IsOpen());
}

TEST(stdDisplayDX6System, TestWindowedDisplayModeCreatesBackBuffer)
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
    TEST_ASSERT_EQUAL_INT(0, stdDisplay_BackBufferFill(0u, NULL));
}

TEST(stdDisplayDX6System, TestTwoBackBufferWindowedRequestPresentsRepeatedly)
{
    StdWin95SystemTest_RequireHiddenWindow();
    StdWin95SystemTest_RequireDisplayDeviceOpen();

    stdDisplay_SetDefaultResolution(320u, 240u);
    TEST_ASSERT_EQUAL_INT(0, stdDisplay_SetMode(0u, 0, 2u));
    for ( uint32_t frame = 0u; frame < 6u; ++frame )
    {
        uint32_t color = frame & 1u ? 0x001Fu : 0x07E0u;
        TEST_ASSERT_EQUAL_INT(0, stdDisplay_BackBufferFill(color, NULL));
        TEST_ASSERT_EQUAL_INT(0, stdDisplay_Update());
    }
}

TEST(stdDisplayDX6System, TestDisableVSyncToggleKeepsBackBufferUsable)
{
    StdWin95SystemTest_RequireHiddenWindow();
    StdWin95SystemTest_RequireWindowedDisplayMode();

    stdDisplay_DisableVSync(true);
    TEST_ASSERT_EQUAL_INT(0, stdDisplay_BackBufferFill(0u, NULL));
    TEST_ASSERT_EQUAL_INT(0, stdDisplay_Update());

    stdDisplay_DisableVSync(false);
    TEST_ASSERT_EQUAL_INT(0, stdDisplay_BackBufferFill(0u, NULL));
    TEST_ASSERT_EQUAL_INT(0, stdDisplay_Update());
}

TEST(stdDisplayDX6System, TestBackBufferLockSaveScreenAndMemoryQueries)
{
    const char* pScreenPath = "stdDisplayDX6SystemTest_screen.bmp";
    void* pSurface;
    uint32_t width;
    uint32_t height;
    int32_t pitch;
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
    stdDisplay_UnlockBackBuffer();

    TEST_ASSERT_EQUAL_INT(0, stdDisplay_SaveScreen(pScreenPath));
    TEST_ASSERT_TRUE(stdFileSize(pScreenPath) > 0u);
    StdGeneralTest_DeleteFile(pScreenPath);
}

TEST(stdDisplayDX6System, TestBackBufferFillPixelsAndRectReadback)
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

TEST(stdDisplayDX6System, TestHardwareVBufferPoolsFillAndNestedLocks)
{
    StdWin95SystemTest_RequireHiddenWindow();
    StdWin95SystemTest_RequireWindowedDisplayMode();

    stdDisplayDX6SystemTest_ExerciseHardwareVBuffer(0);
    stdDisplayDX6SystemTest_ExerciseHardwareVBuffer(1);
}

TEST(stdDisplayDX6System, TestRepeatedDisplayLifecycle)
{
    StdWin95SystemTest_RequireHiddenWindow();

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

TEST(stdDisplayDX6System, TestEnumerationRejectsInvalidIndexes)
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

TEST(stdDisplayDX6System, TestOpenRejectsFirstIndexPastDeviceArray)
{
    StdWin95SystemTest_RequireHiddenWindow();
    StdWin95SystemTest_RequireDisplayConfig();
    StdWin95SystemTest_RequireDisplayStartup();

    TEST_ASSERT_TRUE(stdDisplay_GetNumDevices() > 0u);
    TEST_ASSERT_FALSE(stdDisplay_Open(stdDisplay_GetNumDevices()));
    TEST_ASSERT_FALSE(stdDisplay_IsOpen());
}

TEST(stdDisplayDX6System, TestWindowedClipperAndGdiHelpers)
{
    HDC hdcFront;
    HDC hdcBack;
    int canRenderWindowed;

    StdWin95SystemTest_RequireHiddenWindow();
    StdWin95SystemTest_RequireWindowedDisplayMode();

    canRenderWindowed = stdDisplay_CanRenderWindowed();
    TEST_ASSERT_TRUE(canRenderWindowed == 0 || canRenderWindowed > 0);
    TEST_ASSERT_EQUAL_INT(0, stdDisplay_SetBufferClipper(1));
    TEST_ASSERT_EQUAL_INT(0, stdDisplay_RemoveBufferClipper(1));
    TEST_ASSERT_EQUAL_INT(0, stdDisplay_SetBufferClipper(0));
    TEST_ASSERT_EQUAL_INT(0, stdDisplay_RemoveBufferClipper(0));

    hdcFront = stdDisplay_GetFrontBufferDC();
    if ( hdcFront )
    {
        stdDisplay_ReleaseFrontBufferDC(hdcFront);
    }

    hdcBack = stdDisplay_GetBackBufferDC();
    TEST_ASSERT_NOT_NULL(hdcBack);
    TEST_ASSERT_NOT_EQUAL(CLR_INVALID, GetPixel(hdcBack, 0, 0));
    stdDisplay_ReleaseBackBufferDC(hdcBack);
}

TEST(stdDisplayDX6System, TestEncodeFromRGB565UsesCurrentVideoModeFormat)
{
    StdVideoMode mode;

    StdWin95SystemTest_RequireHiddenWindow();
    StdWin95SystemTest_RequireWindowedDisplayMode();

    STD_ZEROMEM(&mode, sizeof(mode));
    TEST_ASSERT_EQUAL_INT(0, stdDisplay_GetCurrentVideoMode(&mode));

    for ( uint32_t pixel = 0u; pixel <= UINT16_MAX; ++pixel )
    {
        uint32_t expected = stdDisplayDX6SystemTest_ExpectedRGB565((uint16_t)pixel, &mode.rasterInfo.colorInfo);
        TEST_ASSERT_EQUAL_HEX32(expected, stdDisplay_EncodeFromRGB565((uint16_t)pixel));
    }
}

TEST_GROUP_RUNNER(stdDisplayDX6System)
{
    RUN_TEST_CASE(stdDisplayDX6System, TestDisplayStartupOpenAndCloseDevice);
    RUN_TEST_CASE(stdDisplayDX6System, TestWindowedDisplayModeCreatesBackBuffer);
    RUN_TEST_CASE(stdDisplayDX6System, TestTwoBackBufferWindowedRequestPresentsRepeatedly);
    RUN_TEST_CASE(stdDisplayDX6System, TestDisableVSyncToggleKeepsBackBufferUsable);
    RUN_TEST_CASE(stdDisplayDX6System, TestBackBufferLockSaveScreenAndMemoryQueries);
    RUN_TEST_CASE(stdDisplayDX6System, TestBackBufferFillPixelsAndRectReadback);
    RUN_TEST_CASE(stdDisplayDX6System, TestHardwareVBufferPoolsFillAndNestedLocks);
    RUN_TEST_CASE(stdDisplayDX6System, TestRepeatedDisplayLifecycle);
    RUN_TEST_CASE(stdDisplayDX6System, TestEnumerationRejectsInvalidIndexes);
    RUN_TEST_CASE(stdDisplayDX6System, TestOpenRejectsFirstIndexPastDeviceArray);
    RUN_TEST_CASE(stdDisplayDX6System, TestWindowedClipperAndGdiHelpers);
    RUN_TEST_CASE(stdDisplayDX6System, TestEncodeFromRGB565UsesCurrentVideoModeFormat);
}

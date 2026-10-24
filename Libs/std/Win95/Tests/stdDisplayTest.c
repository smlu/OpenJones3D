#include <unity_fixture.h>

#include <stdint.h>
#include <string.h>

#include <std/General/std.h>
#include <std/General/stdColor.h>
#include <std/General/stdUtil.h>
#include <std/Win95/stdDisplay.h>

#include "stdGeneralTest.h"

static int stdDisplayTest_numPreResetCallbacks;
static int stdDisplayTest_numPostResetCallbacks;
static int stdDisplayTest_numReleaseCallbacks;

static void stdDisplayTest_PreResetCallback(tSysDevice3D* pDevice)
{
    TEST_ASSERT_NULL(pDevice);
    ++stdDisplayTest_numPreResetCallbacks;
}

static bool stdDisplayTest_PostResetCallback(tSysDevice3D* pDevice)
{
    TEST_ASSERT_NULL(pDevice);
    ++stdDisplayTest_numPostResetCallbacks;
    return true;
}

static void stdDisplayTest_ReleaseCallback(tSysDevice3D* pDevice)
{
    TEST_ASSERT_NULL(pDevice);
    ++stdDisplayTest_numReleaseCallbacks;
}

TEST_GROUP(stdDisplay);

TEST_SETUP(stdDisplay)
{
    StdGeneralTest_Startup();
    stdDisplay_Shutdown();

    stdDisplayTest_numPreResetCallbacks  = 0;
    stdDisplayTest_numPostResetCallbacks = 0;
    stdDisplayTest_numReleaseCallbacks   = 0;
}

TEST_TEAR_DOWN(stdDisplay)
{
    stdDisplay_Shutdown();
    StdGeneralTest_Shutdown();
}

static tRasterInfo stdDisplayTest_MakeRasterInfo(uint32_t width, uint32_t height, const ColorInfo* pColorInfo)
{
    tRasterInfo rasterInfo;

    STD_ZEROMEM(&rasterInfo, sizeof(rasterInfo));
    rasterInfo.width     = width;
    rasterInfo.height    = height;
    rasterInfo.colorInfo = *pColorInfo;
    return rasterInfo;
}

static void stdDisplayTest_AssertAllPixels16(const tVBuffer* pVBuffer, uint16_t pixel)
{
    const uint16_t* pPixels = (const uint16_t*)pVBuffer->pPixels;
    size_t numPixels        = pVBuffer->rasterInfo.size / sizeof(*pPixels);

    for ( size_t i = 0; i < numPixels; ++i )
    {
        TEST_ASSERT_EQUAL_HEX16(pixel, pPixels[i]);
    }
}

static void stdDisplayTest_AssertAllPixels32(const tVBuffer* pVBuffer, uint32_t pixel)
{
    const uint32_t* pPixels = (const uint32_t*)pVBuffer->pPixels;
    size_t numPixels        = pVBuffer->rasterInfo.size / sizeof(*pPixels);

    for ( size_t i = 0; i < numPixels; ++i )
    {
        TEST_ASSERT_EQUAL_HEX32(pixel, pPixels[i]);
    }
}

TEST(stdDisplay, TestCallbackTypesAreCallable)
{
    tDisplayDevicePreResetCallback pPreResetCallback   = stdDisplayTest_PreResetCallback;
    tDisplayDevicePostResetCallback pPostResetCallback = stdDisplayTest_PostResetCallback;
    tDisplayDeviceReleaseCallback pReleaseCallback     = stdDisplayTest_ReleaseCallback;

    pPreResetCallback(NULL);
    pPostResetCallback(NULL);
    pReleaseCallback(NULL);

    TEST_ASSERT_EQUAL_INT(1, stdDisplayTest_numPreResetCallbacks);
    TEST_ASSERT_EQUAL_INT(1, stdDisplayTest_numPostResetCallbacks);
    TEST_ASSERT_EQUAL_INT(1, stdDisplayTest_numReleaseCallbacks);
}

TEST(stdDisplay, TestDisplayModeStructuresExposeExpectedFields)
{
    StdVideoMode mode = { 0 };

    StdDisplayDevice dev = { 0 };

    StdDisplayInfo info = { 0 };

    StdDisplayEnvironment env = { 0 };

    mode.aspectRatio              = 4.0f / 3.0f;
    mode.rasterInfo.width         = 640u;
    mode.rasterInfo.height        = 480u;
    mode.rasterInfo.colorInfo.bpp = 16u;
    mode.refreshRate              = 60u;

    dev.bHAL             = 1;
    dev.bGuidNotSet      = 0;
    dev.totalVideoMemory = 8u * 1024u * 1024u;
    dev.freeVideoMemory  = 4u * 1024u * 1024u;

    info.displayDevice = dev;
    info.numModes      = 1u;
    info.aModes        = &mode;
    env.numInfos       = 1u;
    env.aDisplayInfos  = &info;

    TEST_ASSERT_FLOAT_WITHIN(0.0001f, 1.3333334f, env.aDisplayInfos[0].aModes[0].aspectRatio);
    TEST_ASSERT_EQUAL_UINT32(640u, env.aDisplayInfos[0].aModes[0].rasterInfo.width);
    TEST_ASSERT_EQUAL_UINT32(480u, env.aDisplayInfos[0].aModes[0].rasterInfo.height);
    TEST_ASSERT_EQUAL_INT(16, env.aDisplayInfos[0].aModes[0].rasterInfo.colorInfo.bpp);
    TEST_ASSERT_EQUAL_UINT32(60u, env.aDisplayInfos[0].aModes[0].refreshRate);
    TEST_ASSERT_EQUAL_INT(1, env.aDisplayInfos[0].displayDevice.bHAL);
    TEST_ASSERT_EQUAL_size_t(1u, env.numInfos);
}

TEST(stdDisplay, TestTextureFormatAndVBufferStructuresExposeExpectedFields)
{
    StdTextureFormat textureFormat = { 0 };

    tVBuffer vbuffer = { 0 };

    textureFormat.ci.colorMode = STDCOLOR_RGBA;
    textureFormat.ci.bpp       = 32u;
    textureFormat.ci.redBPP    = 8u;
    textureFormat.ci.greenBPP  = 8u;
    textureFormat.ci.blueBPP   = 8u;
    textureFormat.ci.alphaBPP  = 8u;
    textureFormat.bColorKey    = 1;

    vbuffer.type                 = VBUFFER_SOFTWARE;
    vbuffer.lockRefCount         = 2u;
    vbuffer.bVideoMemory         = 0;
    vbuffer.rasterInfo.width     = 320u;
    vbuffer.rasterInfo.height    = 200u;
    vbuffer.rasterInfo.rowWidth  = 320u;
    vbuffer.rasterInfo.rowSize   = 640u;
    vbuffer.rasterInfo.size      = 128000u;
    vbuffer.rasterInfo.colorInfo = textureFormat.ci;

    TEST_ASSERT_EQUAL_INT(STDCOLOR_RGBA, textureFormat.ci.colorMode);
    TEST_ASSERT_EQUAL_UINT32(32u, textureFormat.ci.bpp);
    TEST_ASSERT_EQUAL_INT(1, textureFormat.bColorKey);
    TEST_ASSERT_EQUAL_INT(VBUFFER_SOFTWARE, vbuffer.type);
    TEST_ASSERT_EQUAL_size_t(2u, vbuffer.lockRefCount);
    TEST_ASSERT_EQUAL_UINT32(320u, vbuffer.rasterInfo.width);
    TEST_ASSERT_EQUAL_UINT32(200u, vbuffer.rasterInfo.height);
    TEST_ASSERT_EQUAL_size_t(640u, vbuffer.rasterInfo.rowSize);
    TEST_ASSERT_EQUAL_size_t(128000u, vbuffer.rasterInfo.size);
    TEST_ASSERT_EQUAL_UINT32(32u, vbuffer.rasterInfo.colorInfo.bpp);
}

TEST(stdDisplay, TestClosedDisplayStateAndSafeNoDeviceCalls)
{
    StdDisplayDevice device;
    StdVideoMode mode;
    const StdDisplayDevice* pDevices;
    uint32_t width  = 1u;
    uint32_t height = 1u;
    void* pSurface  = (void*)(uintptr_t)1u;
    int32_t pitch   = 123;

    STD_ZEROMEM(&device, sizeof(device));
    STD_ZEROMEM(&mode, sizeof(mode));

    TEST_ASSERT_FALSE(stdDisplay_IsOpen());
    TEST_ASSERT_EQUAL_size_t(0u, stdDisplay_GetNumDevices());
    TEST_ASSERT_EQUAL_INT(1, stdDisplay_GetDevice(0u, &device));
    TEST_ASSERT_EQUAL_INT(1, stdDisplay_GetCurrentDevice(&device));

    pDevices = stdDisplay_GetAllDevices();
    TEST_ASSERT_NOT_NULL(pDevices);
    TEST_ASSERT_EQUAL_MEMORY(&device, &pDevices[0], sizeof(device));
    TEST_ASSERT_NULL(stdDisplay_GetSystemDevice());

#ifdef J3D_DIRECTX9
    TEST_ASSERT_NULL(stdDisplay_GetDirect3D());
    TEST_ASSERT_EQUAL_INT(1, stdDisplay_Update());
    TEST_ASSERT_EQUAL_INT(1, stdDisplay_BackBufferFill(0u, NULL));
    TEST_ASSERT_EQUAL_INT(1, stdDisplay_SaveScreen("stdDisplayTest_no_device.bmp"));
    TEST_ASSERT_EQUAL_INT(S_OK, stdDisplay_FlipToGDISurface());
#endif

    TEST_ASSERT_EQUAL_INT(1, stdDisplay_CreateZBuffer(NULL, 0));
    TEST_ASSERT_EQUAL_size_t(0u, stdDisplay_GetNumVideoModes());
    TEST_ASSERT_EQUAL_INT(1, stdDisplay_GetVideoMode(0u, &mode));
    TEST_ASSERT_EQUAL_INT(1, stdDisplay_GetCurrentVideoMode(&mode));

    stdDisplay_GetBackBufferSize(&width, &height);
    TEST_ASSERT_EQUAL_UINT32(0u, width);
    TEST_ASSERT_EQUAL_UINT32(0u, height);

    TEST_ASSERT_NULL(stdDisplay_GetFrontBufferDC());
    TEST_ASSERT_NULL(stdDisplay_GetBackBufferDC());
    TEST_ASSERT_EQUAL_INT(-1, stdDisplay_CanRenderWindowed());
#ifdef J3D_DIRECTX9
    TEST_ASSERT_EQUAL_INT(1, stdDisplay_SetBufferClipper(1));
    TEST_ASSERT_EQUAL_INT(S_OK, stdDisplay_RemoveBufferClipper(1));
#else
    TEST_ASSERT_EQUAL_INT(0, stdDisplay_SetBufferClipper(1));
    TEST_ASSERT_EQUAL_INT(0, stdDisplay_RemoveBufferClipper(1));
#endif
    TEST_ASSERT_EQUAL_INT(0, stdDisplay_IsFullscreen());
    TEST_ASSERT_EQUAL_INT(1, stdDisplay_LockBackBuffer(&pSurface, &width, &height, &pitch));
    TEST_ASSERT_EQUAL_INT(1, stdDisplay_LockBackBufferReadOnly(&pSurface, &width, &height, &pitch));
    TEST_ASSERT_EQUAL_PTR((void*)(uintptr_t)1u, pSurface);
    TEST_ASSERT_EQUAL_UINT32(0u, width);
    TEST_ASSERT_EQUAL_UINT32(0u, height);
    TEST_ASSERT_EQUAL_INT32(123, pitch);

    stdDisplay_RegisterDevicePreResetCallback(stdDisplayTest_PreResetCallback);
    stdDisplay_RegisterDevicePostResetCallback(stdDisplayTest_PostResetCallback);
    TEST_ASSERT_EQUAL_INT(0, stdDisplayTest_numPreResetCallbacks);
    TEST_ASSERT_EQUAL_INT(0, stdDisplayTest_numPostResetCallbacks);
}

TEST(stdDisplay, TestSoftwareVBufferNewLockFillAndFree)
{
    static const uint8_t aExpected8[] =
    {
        0x11u, 0x11u, 0x11u,
        0x11u, 0x22u, 0x22u
    };
    tRasterInfo raster8  = stdDisplayTest_MakeRasterInfo(3u, 2u, &stdColor_cfRGB555);
    tRasterInfo raster16 = stdDisplayTest_MakeRasterInfo(3u, 2u, &stdColor_cfRGB565);
    tRasterInfo raster32 = stdDisplayTest_MakeRasterInfo(2u, 2u, &stdColor_cfRGB8888);
    tVBuffer* pVBuffer8;
    tVBuffer* pVBuffer16;
    tVBuffer* pVBuffer32;

    raster8.colorInfo.bpp = 8u;
    pVBuffer8 = stdDisplay_VBufferNew(&raster8, 0, 0);
    TEST_ASSERT_NOT_NULL(pVBuffer8);
    TEST_ASSERT_EQUAL_INT(VBUFFER_SOFTWARE, pVBuffer8->type);
    TEST_ASSERT_EQUAL_size_t(1u, pVBuffer8->lockRefCount);
    TEST_ASSERT_EQUAL_size_t(3u, pVBuffer8->rasterInfo.rowSize);
    TEST_ASSERT_EQUAL_size_t(3u, pVBuffer8->rasterInfo.rowWidth);
    TEST_ASSERT_EQUAL_size_t(sizeof(aExpected8), pVBuffer8->rasterInfo.size);

    TEST_ASSERT_EQUAL_INT(1, stdDisplay_VBufferLock(pVBuffer8));
    TEST_ASSERT_EQUAL_size_t(2u, pVBuffer8->lockRefCount);
    TEST_ASSERT_EQUAL_INT(1, stdDisplay_VBufferUnlock(pVBuffer8));
    TEST_ASSERT_EQUAL_size_t(1u, pVBuffer8->lockRefCount);
    TEST_ASSERT_EQUAL_INT(1, stdDisplay_VBufferFill(pVBuffer8, 0x11u, NULL));

    StdRect rect8 = { 1, 1, 2, 1 };
    TEST_ASSERT_EQUAL_INT(1, stdDisplay_VBufferFill(pVBuffer8, 0x22u, &rect8));
    TEST_ASSERT_EQUAL_MEMORY(aExpected8, pVBuffer8->pPixels, sizeof(aExpected8));

    pVBuffer16 = stdDisplay_VBufferNew(&raster16, 0, 0);
    TEST_ASSERT_NOT_NULL(pVBuffer16);
    TEST_ASSERT_EQUAL_size_t(6u, pVBuffer16->rasterInfo.rowSize);
    TEST_ASSERT_EQUAL_size_t(3u, pVBuffer16->rasterInfo.rowWidth);
    TEST_ASSERT_EQUAL_size_t(12u, pVBuffer16->rasterInfo.size);
    TEST_ASSERT_EQUAL_INT(1, stdDisplay_VBufferFill(pVBuffer16, 0x1234u, NULL));
    stdDisplayTest_AssertAllPixels16(pVBuffer16, 0x1234u);

    pVBuffer32 = stdDisplay_VBufferNew(&raster32, 0, 0);
    TEST_ASSERT_NOT_NULL(pVBuffer32);
    TEST_ASSERT_EQUAL_size_t(8u, pVBuffer32->rasterInfo.rowSize);
    TEST_ASSERT_EQUAL_size_t(2u, pVBuffer32->rasterInfo.rowWidth);
    TEST_ASSERT_EQUAL_size_t(16u, pVBuffer32->rasterInfo.size);
    TEST_ASSERT_EQUAL_INT(1, stdDisplay_VBufferFill(pVBuffer32, 0x89ABCDEFu, NULL));
    stdDisplayTest_AssertAllPixels32(pVBuffer32, 0x89ABCDEFu);

    stdDisplay_VBufferFree(pVBuffer32);
    stdDisplay_VBufferFree(pVBuffer16);
    stdDisplay_VBufferFree(pVBuffer8);
}

TEST(stdDisplay, TestSoftwareVBufferFillCoversOddEvenAndRectangularPixels)
{
    tRasterInfo raster16Odd  = stdDisplayTest_MakeRasterInfo(5u, 1u, &stdColor_cfRGB565);
    tRasterInfo raster16Rect = stdDisplayTest_MakeRasterInfo(4u, 3u, &stdColor_cfRGB565);
    tRasterInfo raster32Rect = stdDisplayTest_MakeRasterInfo(4u, 2u, &stdColor_cfRGB8888);
    tVBuffer* pVBuffer16Odd  = stdDisplay_VBufferNew(&raster16Odd, 0, 0);
    tVBuffer* pVBuffer16Rect = stdDisplay_VBufferNew(&raster16Rect, 0, 0);
    tVBuffer* pVBuffer32Rect = stdDisplay_VBufferNew(&raster32Rect, 0, 0);

    TEST_ASSERT_NOT_NULL(pVBuffer16Odd);
    TEST_ASSERT_NOT_NULL(pVBuffer16Rect);
    TEST_ASSERT_NOT_NULL(pVBuffer32Rect);

    TEST_ASSERT_EQUAL_INT(1, stdDisplay_VBufferFill(pVBuffer16Odd, 0xA55Au, NULL));
    stdDisplayTest_AssertAllPixels16(pVBuffer16Odd, 0xA55Au);

    TEST_ASSERT_EQUAL_INT(1, stdDisplay_VBufferFill(pVBuffer16Rect, 0x1111u, NULL));
    StdRect rect16 = { 1, 1, 2, 1 };
    TEST_ASSERT_EQUAL_INT(1, stdDisplay_VBufferFill(pVBuffer16Rect, 0xBEEFu, &rect16));

    const uint16_t* pPixels16 = (const uint16_t*)pVBuffer16Rect->pPixels;
    for ( size_t y = 0; y < 3u; ++y )
    {
        for ( size_t x = 0; x < 4u; ++x )
        {
            uint16_t expected = (y == 1u && x >= 1u && x <= 2u) ? 0xBEEFu : 0x1111u;
            TEST_ASSERT_EQUAL_HEX16(expected, pPixels16[y * 4u + x]);
        }
    }

    TEST_ASSERT_EQUAL_INT(1, stdDisplay_VBufferFill(pVBuffer32Rect, 0x01020304u, NULL));
    StdRect rect32 = { 2, 0, 2, 2 };
    TEST_ASSERT_EQUAL_INT(1, stdDisplay_VBufferFill(pVBuffer32Rect, 0xA1B2C3D4u, &rect32));

    const uint32_t* pPixels32 = (const uint32_t*)pVBuffer32Rect->pPixels;
    for ( size_t y = 0; y < 2u; ++y )
    {
        for ( size_t x = 0; x < 4u; ++x )
        {
            uint32_t expected = x >= 2u ? 0xA1B2C3D4u : 0x01020304u;
            TEST_ASSERT_EQUAL_HEX32(expected, pPixels32[y * 4u + x]);
        }
    }

    stdDisplay_VBufferFree(pVBuffer32Rect);
    stdDisplay_VBufferFree(pVBuffer16Rect);
    stdDisplay_VBufferFree(pVBuffer16Odd);
}

TEST(stdDisplay, TestSoftwareVBufferAllocationFailures)
{
    tRasterInfo rasterInfo = stdDisplayTest_MakeRasterInfo(2u, 2u, &stdColor_cfRGB8888);

    StdGeneralTest_FailNextAllocations(0);
    TEST_ASSERT_NULL(stdDisplay_VBufferNew(&rasterInfo, 0, 0));

    StdGeneralTest_FailNextAllocations(1);
    TEST_ASSERT_NULL(stdDisplay_VBufferNew(&rasterInfo, 0, 0));

    StdGeneralTest_ClearAllocationFailures();
}

TEST(stdDisplay, TestSoftwareVBufferColorConversion)
{
    tRasterInfo rasterInfo = stdDisplayTest_MakeRasterInfo(2u, 1u, &stdColor_cfRGB8888);
    tVBuffer* pSource = stdDisplay_VBufferNew(&rasterInfo, 0, 0);

    TEST_ASSERT_NOT_NULL(pSource);
    ((uint32_t*)pSource->pPixels)[0] = stdColor_EncodeRGB(&stdColor_cfRGB8888, 255u, 0u, 0u);
    ((uint32_t*)pSource->pPixels)[1] = stdColor_EncodeRGB(&stdColor_cfRGB8888, 0u, 255u, 0u);

    tVBuffer* pSame = stdDisplay_VBufferConvertColorFormat(&stdColor_cfRGB8888, pSource, 0, NULL);
    TEST_ASSERT_EQUAL_PTR(pSource, pSame);

    tVBuffer* pConverted = stdDisplay_VBufferConvertColorFormat(&stdColor_cfRGB565, pSource, 0, NULL);
    TEST_ASSERT_NOT_NULL(pConverted);
    TEST_ASSERT_NOT_EQUAL(pSource, pConverted);
    TEST_ASSERT_EQUAL_UINT32(16u, pConverted->rasterInfo.colorInfo.bpp);
    TEST_ASSERT_EQUAL_size_t(4u, pConverted->rasterInfo.size);
    TEST_ASSERT_EQUAL_HEX16(0xF800u, ((uint16_t*)pConverted->pPixels)[0]);
    TEST_ASSERT_EQUAL_HEX16(0x07E0u, ((uint16_t*)pConverted->pPixels)[1]);

    stdDisplay_VBufferFree(pConverted);
}

TEST_GROUP_RUNNER(stdDisplay)
{
    RUN_TEST_CASE(stdDisplay, TestCallbackTypesAreCallable);
    RUN_TEST_CASE(stdDisplay, TestDisplayModeStructuresExposeExpectedFields);
    RUN_TEST_CASE(stdDisplay, TestTextureFormatAndVBufferStructuresExposeExpectedFields);
    RUN_TEST_CASE(stdDisplay, TestClosedDisplayStateAndSafeNoDeviceCalls);
    RUN_TEST_CASE(stdDisplay, TestSoftwareVBufferNewLockFillAndFree);
    RUN_TEST_CASE(stdDisplay, TestSoftwareVBufferFillCoversOddEvenAndRectangularPixels);
    RUN_TEST_CASE(stdDisplay, TestSoftwareVBufferAllocationFailures);
    RUN_TEST_CASE(stdDisplay, TestSoftwareVBufferColorConversion);
}

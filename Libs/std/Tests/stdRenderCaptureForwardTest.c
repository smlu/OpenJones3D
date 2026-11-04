// Added: M6.0 render capture forwarding-layer tests: the std3D_/stdDisplay_ forwarders record
// every call through the RCAP v1 writer while a capture is open. Built only on the Null platform
// columns (it forwards to the Null backends); expected bytes are hand-written from the encoding
// in std3DCapture.c / stdDisplayCapture.c, never produced by the code under test.

#include <unity_fixture.h>

#include <stdio.h>
#include <string.h>

#include <std/General/stdRenderCapture.h>
#include <std/General/stdRenderCaptureRecord.h>
#include <std/General/stdUtil.h>

#include <std/Win95/std3D.h>
#include <std/Win95/stdDisplay.h>
#include <std/Win95/stdDisplayCapture.h>

#include "stdGeneralTest.h"

// The 20-byte RCAP v1 header for the 4-column column name "null".
static const uint8_t stdRenderCaptureForward_aHeaderNull[20] = {
    0x4A, 0x33, 0x44, 0x52, 0x43, 0x41, 0x50, 0x00, // "J3DRCAP\0"
    0x01, 0x00, 0x00, 0x00,                         // version 1
    0x04, 0x00, 0x00, 0x00,                         // columnLen 4
    0x6E, 0x75, 0x6C, 0x6C,                         // "null"
};

// GLOBAL 1 with the IEEE-754 bits of the fog density (std3D_ResetGlobals sets 1.0f).
static const uint8_t stdRenderCaptureForward_aGlobalFog1[16] = {
    0xF1, 0xFF, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00,
    0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x3F, // id 1, 1.0f = 0x3F800000
};

// GLOBAL 1 with 2.0f.
static const uint8_t stdRenderCaptureForward_aGlobalFog2[16] = {
    0xF1, 0xFF, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00,
    0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x40, // id 1, 2.0f = 0x40000000
};

static size_t stdRenderCaptureForward_ReadFile(const char* pPath, uint8_t* pBuf, size_t bufSize)
{
    FILE* pFile = fopen(pPath, "rb");
    long size;
    size_t numRead;

    if ( !pFile )
    {
        return (size_t)-1;
    }

    fseek(pFile, 0, SEEK_END);
    size = ftell(pFile);
    fseek(pFile, 0, SEEK_SET);
    if ( size < 0 || (size_t)size > bufSize )
    {
        fclose(pFile);
        return (size_t)-1;
    }

    numRead = fread(pBuf, 1u, (size_t)size, pFile);
    fclose(pFile);
    TEST_ASSERT_EQUAL_size_t((size_t)size, numRead);
    return (size_t)size;
}

// Compares the whole capture file against the hand-written expected bytes, then removes it.
static void stdRenderCaptureForward_AssertFileBytes(const char* pPath, const uint8_t* pExpected, size_t expectedSize)
{
    uint8_t aFile[1024];
    size_t size = stdRenderCaptureForward_ReadFile(pPath, aFile, sizeof(aFile));

    TEST_ASSERT_EQUAL_size_t(expectedSize, size);
    TEST_ASSERT_EQUAL_MEMORY(pExpected, aFile, expectedSize);
    remove(pPath);
}

// The 16-bit 5:6:5 ColorInfo fields in record order, used by the VBufferNew records.
#define STDRENCAPTUREFWD_COLORINFO_16BPP_BYTES                                                    \
    0x01, 0x00, 0x00, 0x00, /* colorMode   = STDCOLOR_RGB */                                      \
    0x10, 0x00, 0x00, 0x00, /* bpp         = 16 */                                                \
    0x05, 0x00, 0x00, 0x00, /* redBPP      = 5 */                                                 \
    0x06, 0x00, 0x00, 0x00, /* greenBPP    = 6 */                                                 \
    0x05, 0x00, 0x00, 0x00, /* blueBPP     = 5 */                                                 \
    0x0B, 0x00, 0x00, 0x00, /* redPosShift = 11 */                                                \
    0x05, 0x00, 0x00, 0x00, /* greenPosShift = 5 */                                               \
    0x00, 0x00, 0x00, 0x00, /* bluePosShift = 0 */                                                \
    0x03, 0x00, 0x00, 0x00, /* redPosShiftRight = 3 */                                            \
    0x02, 0x00, 0x00, 0x00, /* greenPosShiftRight = 2 */                                          \
    0x03, 0x00, 0x00, 0x00, /* bluePosShiftRight = 3 */                                           \
    0x00, 0x00, 0x00, 0x00, /* alphaBPP = 0 */                                                    \
    0x00, 0x00, 0x00, 0x00, /* alphaPosShift = 0 */                                               \
    0x00, 0x00, 0x00, 0x00  /* alphaPosShiftRight = 0 */

static const uint8_t stdRenderCaptureForward_aColorInfo16[56] = {
    STDRENCAPTUREFWD_COLORINFO_16BPP_BYTES
};

static void stdRenderCaptureForward_FillColorInfo16(ColorInfo* pColorInfo)
{
    pColorInfo->colorMode           = STDCOLOR_RGB;
    pColorInfo->bpp                 = 16u;
    pColorInfo->redBPP              = 5u;
    pColorInfo->greenBPP            = 6u;
    pColorInfo->blueBPP             = 5u;
    pColorInfo->redPosShift         = 11;
    pColorInfo->greenPosShift       = 5;
    pColorInfo->bluePosShift        = 0;
    pColorInfo->redPosShiftRight    = 3;
    pColorInfo->greenPosShiftRight  = 2;
    pColorInfo->bluePosShiftRight   = 3;
    pColorInfo->alphaBPP            = 0u;
    pColorInfo->alphaPosShift       = 0;
    pColorInfo->alphaPosShiftRight  = 0;
}

TEST_GROUP(stdRenderCaptureForward);

TEST_SETUP(stdRenderCaptureForward)
{
    StdGeneralTest_Startup();
    stdRenderCapture_ResetGlobals();

    // No capture is open yet, so these forwarder calls run the backends unrecorded: the Null
    // std3D_ResetGlobals sets the fog density to 1.0f, the value the GLOBAL records carry.
    std3D_ResetGlobals();
    stdDisplay_ResetGlobals();
}

TEST_TEAR_DOWN(stdRenderCaptureForward)
{
    stdDisplayCapture_Close();
    stdRenderCapture_ResetGlobals();
    std3D_ResetGlobals();
    stdDisplay_ResetGlobals();
    StdGeneralTest_Shutdown();
}

TEST(stdRenderCaptureForward, TestOpenWritesHeaderOnlyAndBackBufferHandle)
{
    remove("stdRenderCaptureForward_Open.rcap");

    TEST_ASSERT_TRUE(stdDisplayCapture_Open("stdRenderCaptureForward_Open.rcap", "null"));
    TEST_ASSERT_TRUE(stdRenderCapture_IsOpen());

    // The back buffer is registered as VBuffer handle 1 by the Open.
    TEST_ASSERT_EQUAL_UINT32(1u, stdRenderCapture_FindHandle(STDRENDERCAPTURE_HANDLE_VBUFFER, &stdDisplay_g_backBuffer));

    stdDisplayCapture_Close();
    TEST_ASSERT_FALSE(stdRenderCapture_IsOpen());

    stdRenderCaptureForward_AssertFileBytes("stdRenderCaptureForward_Open.rcap", stdRenderCaptureForward_aHeaderNull, sizeof(stdRenderCaptureForward_aHeaderNull));

    // The handle table is per-capture: closed, nothing is registered.
    TEST_ASSERT_EQUAL_UINT32(0u, stdRenderCapture_FindHandle(STDRENDERCAPTURE_HANDLE_VBUFFER, &stdDisplay_g_backBuffer));
}

TEST(stdRenderCaptureForward, TestVBufferNewFillFreeBytes)
{
    static const uint8_t aExpected[] = {
        // header
        0x4A, 0x33, 0x44, 0x52, 0x43, 0x41, 0x50, 0x00,
        0x01, 0x00, 0x00, 0x00,
        0x04, 0x00, 0x00, 0x00,
        0x6E, 0x75, 0x6C, 0x6C,
        // VBufferNew (id 124), payload 77: present byte, width/height, 16bpp ColorInfo, flags, handle 2
        0x7C, 0x00, 0x00, 0x00, 0x4D, 0x00, 0x00, 0x00,
        0x01,
        0x02, 0x00, 0x00, 0x00,                                     // width 2
        0x02, 0x00, 0x00, 0x00,                                     // height 2
        0x01, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00, 0x05, 0x00, 0x00, 0x00, 0x06, 0x00, 0x00, 0x00,
        0x05, 0x00, 0x00, 0x00, 0x0B, 0x00, 0x00, 0x00, 0x05, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x03, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,             // alphaPosShift, alphaPosShiftRight
        0x00, 0x00, 0x00, 0x00,                                     // bUseVSurface 0
        0x00, 0x00, 0x00, 0x00,                                     // bUseVideoMemory 0
        0x02, 0x00, 0x00, 0x00,                                     // returned VBuffer handle 2
        // VBufferFill (id 129), payload 29: handle 2, color, present rect (0,0)-(1,1), result 0
        0x81, 0x00, 0x00, 0x00, 0x1D, 0x00, 0x00, 0x00,
        0x02, 0x00, 0x00, 0x00,
        0xDD, 0xCC, 0xBB, 0xAA,
        0x01,
        0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00,
        0x01, 0x00, 0x00, 0x00,
        0x01, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00,
        // VBufferFree (id 125), payload 4: the handle found before the call
        0x7D, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00,
        0x02, 0x00, 0x00, 0x00,
    };
    tRasterInfo raster;
    tVBuffer* pBuffer;

    STD_ZEROMEM(&raster, sizeof(raster));
    raster.width    = 2u;
    raster.height   = 2u;
    // Garbage: the backend recomputes these and the capture must not record them.
    raster.size     = (size_t)0x11111111u;
    raster.rowSize  = (size_t)0x22222222u;
    raster.rowWidth = (size_t)0x33333333u;
    stdRenderCaptureForward_FillColorInfo16(&raster.colorInfo);

    remove("stdRenderCaptureForward_VBuffer.rcap");
    TEST_ASSERT_TRUE(stdDisplayCapture_Open("stdRenderCaptureForward_VBuffer.rcap", "null"));

    pBuffer = stdDisplay_VBufferNew(&raster, /*bUseVSurface=*/0, /*bUseVideoMemory=*/0);
    TEST_ASSERT_NOT_NULL(pBuffer);
    TEST_ASSERT_EQUAL_UINT32(2u, stdRenderCapture_FindHandle(STDRENDERCAPTURE_HANDLE_VBUFFER, pBuffer));

    {
        const StdRect rect = { 0, 0, 1, 1 };
        TEST_ASSERT_EQUAL_INT(0, stdDisplay_VBufferFill(pBuffer, 0xAABBCCDDu, &rect));
    }

    stdDisplay_VBufferFree(pBuffer);
    stdDisplayCapture_Close();

    stdRenderCaptureForward_AssertFileBytes("stdRenderCaptureForward_VBuffer.rcap", aExpected, sizeof(aExpected));
}

TEST(stdRenderCaptureForward, TestFogGlobalAroundStd3DRecords)
{
    static const uint8_t aExpected[] = {
        // header
        0x4A, 0x33, 0x44, 0x52, 0x43, 0x41, 0x50, 0x00,
        0x01, 0x00, 0x00, 0x00,
        0x04, 0x00, 0x00, 0x00,
        0x6E, 0x75, 0x6C, 0x6C,
        // GLOBAL 1 = 1.0f before the first std3D record
        0xF1, 0xFF, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00,
        0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x3F,
        // SetRenderState(0x8000), no new GLOBAL: the density did not change
        0x11, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00,
        0x00, 0x80, 0x00, 0x00,
        // SetRenderState(0), still no new GLOBAL
        0x11, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00,
        // EnableFog(1, 2.0f) records the density from before the call
        0x1D, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00,
        0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x40,
        // GLOBAL 1 = 2.0f before the next std3D record
        0xF1, 0xFF, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00,
        0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x40,
        // SetMipmapFilter(BILINEAR), result 0
        0x1B, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00,
        0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    };

    remove("stdRenderCaptureForward_Fog.rcap");
    TEST_ASSERT_TRUE(stdDisplayCapture_Open("stdRenderCaptureForward_Fog.rcap", "null"));

    std3D_SetRenderState(STD3D_RS_FOG_ENABLED);
    std3D_SetRenderState((Std3DRenderState)0);
    std3D_EnableFog(1, 2.0f);
    TEST_ASSERT_EQUAL_INT(0, std3D_SetMipmapFilter(STD3D_MIPMAPFILTER_BILINEAR));

    stdDisplayCapture_Close();
    stdRenderCaptureForward_AssertFileBytes("stdRenderCaptureForward_Fog.rcap", aExpected, sizeof(aExpected));
}

TEST(stdRenderCaptureForward, TestDrawRenderListTextureHandles)
{
    // DrawRenderList (id 14), payload 118: cache-texture handle 1, rdflags 0x2000, 3 vertices,
    // 3 indices; the second call has an unknown pTex (0xFFFFFFFF) and empty arrays.
    static const uint8_t aExpected[] = {
        // header
        0x4A, 0x33, 0x44, 0x52, 0x43, 0x41, 0x50, 0x00,
        0x01, 0x00, 0x00, 0x00,
        0x04, 0x00, 0x00, 0x00,
        0x6E, 0x75, 0x6C, 0x6C,
        // GLOBAL 1 = 1.0f before the first std3D record
        0xF1, 0xFF, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00,
        0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x3F,
        // UpdateFrameCount (id 25), payload 4: system-texture handle 1
        0x19, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00,
        0x01, 0x00, 0x00, 0x00,
        // DrawRenderList (id 14), payload 118
        0x0E, 0x00, 0x00, 0x00, 0x76, 0x00, 0x00, 0x00,
        0x01, 0x00, 0x00, 0x00,                                     // pTex -> system-texture handle 1
        0x00, 0x20, 0x00, 0x00,                                     // rdflags 0x2000
        0x03, 0x00, 0x00, 0x00,                                     // 3 vertices
        // vertex 0: 1.0 2.0 4.0 0.5 0x11223344 0x55667788 0.25 -0.5
        0x00, 0x00, 0x80, 0x3F, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x80, 0x40, 0x00, 0x00, 0x00, 0x3F,
        0x44, 0x33, 0x22, 0x11, 0x88, 0x77, 0x66, 0x55, 0x00, 0x00, 0x80, 0x3E, 0x00, 0x00, 0x00, 0xBF,
        // vertex 1: all zero except the color
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        // vertex 2: -1.0 1.5 0.0 1.0 0x00FF00FF 0xDEADBEEF 1.0 2.0
        0x00, 0x00, 0x80, 0xBF, 0x00, 0x00, 0xC0, 0x3F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x3F,
        0xFF, 0x00, 0xFF, 0x00, 0xEF, 0xBE, 0xAD, 0xDE, 0x00, 0x00, 0x80, 0x3F, 0x00, 0x00, 0x00, 0x40,
        0x03, 0x00, 0x00, 0x00,                                     // 3 indices
        0x00, 0x00, 0x01, 0x00, 0x02, 0x00,
        // DrawRenderList with an unknown pTex: 0xFFFFFFFF, two empty arrays, payload 16
        0x0E, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00,
        0xFF, 0xFF, 0xFF, 0xFF,
        0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00,
    };
    static StdVertexTL aVerts[3] = {
        {  1.0f,  2.0f, 4.0f, 0.5f, 0x11223344u, 0x55667788u,  0.25f, -0.5f },
        {  0.0f,  0.0f, 0.0f, 0.0f, 0xFFFFFFFFu, 0x00000000u,  0.0f,   0.0f },
        { -1.0f,  1.5f, 0.0f, 1.0f, 0x00FF00FFu, 0xDEADBEEFu,  1.0f,   2.0f },
    };
    static const uint16_t aIndices[3] = { 0u, 1u, 2u };
    static tSystemTexture texture;
    static int aCacheObject;
    static int aOtherObject;

    STD_ZEROMEM(&texture, sizeof(texture));
    texture.pCachedTexture = (tSysTexture*)&aCacheObject;

    remove("stdRenderCaptureForward_Draw.rcap");
    TEST_ASSERT_TRUE(stdDisplayCapture_Open("stdRenderCaptureForward_Draw.rcap", "null"));

    // First sight through UpdateFrameCount assigns the system-texture handle 1 (the VBuffer kind
    // is independent, so the back buffer's handle 1 does not interfere).
    std3D_UpdateFrameCount(&texture);
    std3D_DrawRenderList((tSysTexture*)&aCacheObject, STD3D_RS_ZWRITE_DISABLED, aVerts, 3u, (uint16_t*)aIndices, 3u);
    std3D_DrawRenderList((tSysTexture*)&aOtherObject, (Std3DRenderState)0, NULL, 0u, NULL, 0u);

    stdDisplayCapture_Close();
    stdRenderCaptureForward_AssertFileBytes("stdRenderCaptureForward_Draw.rcap", aExpected, sizeof(aExpected));
}

TEST(stdRenderCaptureForward, TestFrameWindowFiltering)
{
    static const uint8_t aExpected[] = {
        // header
        0x4A, 0x33, 0x44, 0x52, 0x43, 0x41, 0x50, 0x00,
        0x01, 0x00, 0x00, 0x00,
        0x04, 0x00, 0x00, 0x00,
        0x6E, 0x75, 0x6C, 0x6C,
        // FRAMES 0, written by Open because frames were already disabled
        0xF2, 0xFF, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00,
        // SetRenderState is not frame-class: GLOBAL + record even with frames disabled
        0xF1, 0xFF, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00,
        0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x3F,
        0x11, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00,
        0x00, 0x20, 0x00, 0x00,
    };
    uint8_t aFile[512];
    size_t size;

    // StartScene, Update and EndScene are frame-class: with frames disabled they run and record
    // nothing; SetRenderState is always recorded.
    remove("stdRenderCaptureForward_Frames.rcap");
    stdRenderCapture_SetFramesEnabled(false);
    TEST_ASSERT_TRUE(stdDisplayCapture_Open("stdRenderCaptureForward_Frames.rcap", "null"));

    TEST_ASSERT_EQUAL_INT(0, std3D_StartScene());
    TEST_ASSERT_EQUAL_INT(1, stdDisplay_Update()); // Null: no mode set
    std3D_EndScene();
    std3D_SetRenderState(STD3D_RS_ZWRITE_DISABLED);

    stdDisplayCapture_Close();
    size = stdRenderCaptureForward_ReadFile("stdRenderCaptureForward_Frames.rcap", aFile, sizeof(aFile));
    TEST_ASSERT_EQUAL_size_t(sizeof(aExpected), size);
    TEST_ASSERT_EQUAL_MEMORY(aExpected, aFile, sizeof(aExpected));
    remove("stdRenderCaptureForward_Frames.rcap");
}

TEST(stdRenderCaptureForward, TestVBufferUnlockRecordsWithFramesDisabled)
{
    // VBufferUnlock is not frame-class: even with frames disabled it writes its content blob
    // and its record. The handle is assigned on first sight (the back buffer is handle 1).
    static const uint8_t aExpected[] = {
        // header
        0x4A, 0x33, 0x44, 0x52, 0x43, 0x41, 0x50, 0x00,
        0x01, 0x00, 0x00, 0x00,
        0x04, 0x00, 0x00, 0x00,
        0x6E, 0x75, 0x6C, 0x6C,
        // FRAMES 0, written by Open because frames were already disabled
        0xF2, 0xFF, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00,
        // BLOB 1: the 4 content bytes (height 2 * rowSize 2)
        0xF0, 0xFF, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00,
        0x01, 0x00, 0x00, 0x00,
        0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x11, 0x22, 0x33, 0x44,
        // VBufferUnlock (id 128), payload 12: handle 2, blob 1, result 1
        0x80, 0x00, 0x00, 0x00, 0x0C, 0x00, 0x00, 0x00,
        0x02, 0x00, 0x00, 0x00,
        0x01, 0x00, 0x00, 0x00,
        0x01, 0x00, 0x00, 0x00,
    };
    static uint8_t aPixels[4] = { 0x11, 0x22, 0x33, 0x44 };
    tVBuffer buffer;

    STD_ZEROMEM(&buffer, sizeof(buffer));
    buffer.type              = VBUFFER_SOFTWARE;
    buffer.lockRefCount      = 1u;
    buffer.rasterInfo.width  = 2u;
    buffer.rasterInfo.height = 2u;
    buffer.rasterInfo.size   = 4u;
    buffer.rasterInfo.rowSize = 2u;
    buffer.rasterInfo.rowWidth = 2u;
    buffer.pPixels           = aPixels;

    remove("stdRenderCaptureForward_UnlockFrames.rcap");
    stdRenderCapture_SetFramesEnabled(false);
    TEST_ASSERT_TRUE(stdDisplayCapture_Open("stdRenderCaptureForward_UnlockFrames.rcap", "null"));

    TEST_ASSERT_EQUAL_INT(1, stdDisplay_VBufferUnlock(&buffer));

    stdDisplayCapture_Close();
    stdRenderCapture_SetFramesEnabled(true);
    stdRenderCaptureForward_AssertFileBytes(
        "stdRenderCaptureForward_UnlockFrames.rcap",
        aExpected,
        sizeof(aExpected)
    );
}

TEST(stdRenderCaptureForward, TestVBufferUnlockBlob)
{
    static const uint8_t aExpected[] = {
        // header
        0x4A, 0x33, 0x44, 0x52, 0x43, 0x41, 0x50, 0x00,
        0x01, 0x00, 0x00, 0x00,
        0x04, 0x00, 0x00, 0x00,
        0x6E, 0x75, 0x6C, 0x6C,
        // VBufferNew (id 124), payload 77: 2x2 32bpp raster, handle 2
        0x7C, 0x00, 0x00, 0x00, 0x4D, 0x00, 0x00, 0x00,
        0x01,
        0x02, 0x00, 0x00, 0x00,                                     // width 2
        0x02, 0x00, 0x00, 0x00,                                     // height 2
        0x01, 0x00, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00,
        0x08, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,             // alphaPosShift, alphaPosShiftRight
        0x00, 0x00, 0x00, 0x00,                                     // bUseVSurface 0
        0x01, 0x00, 0x00, 0x00,                                     // bUseVideoMemory 1
        0x02, 0x00, 0x00, 0x00,                                     // handle 2
        // VBufferFill (id 129), payload 13: handle 2, 0xDEADBEEF, absent rect, result 0
        0x81, 0x00, 0x00, 0x00, 0x0D, 0x00, 0x00, 0x00,
        0x02, 0x00, 0x00, 0x00,
        0xEF, 0xBE, 0xAD, 0xDE,
        0x00,
        0x00, 0x00, 0x00, 0x00,
        // VBufferLock (id 126), payload 8: handle 2, result 1 (lockRefCount becomes 2)
        0x7E, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00,
        0x02, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00,
        // VBufferUnlock (id 128) with lockRefCount 2 before the call: no blob, payload 12
        0x80, 0x00, 0x00, 0x00, 0x0C, 0x00, 0x00, 0x00,
        0x02, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00,
        0x01, 0x00, 0x00, 0x00,
        // BLOB record 1: the 16 bytes filled above
        0xF0, 0xFF, 0x00, 0x00, 0x1C, 0x00, 0x00, 0x00,
        0x01, 0x00, 0x00, 0x00,
        0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0xEF, 0xBE, 0xAD, 0xDE, 0xEF, 0xBE, 0xAD, 0xDE,
        0xEF, 0xBE, 0xAD, 0xDE, 0xEF, 0xBE, 0xAD, 0xDE,
        // VBufferUnlock (id 128) with lockRefCount 1 before the call: blob 1, payload 12
        0x80, 0x00, 0x00, 0x00, 0x0C, 0x00, 0x00, 0x00,
        0x02, 0x00, 0x00, 0x00,
        0x01, 0x00, 0x00, 0x00,
        0x01, 0x00, 0x00, 0x00,
        // VBufferFree (id 125), payload 4
        0x7D, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00,
        0x02, 0x00, 0x00, 0x00,
    };
    static const uint8_t aColor32[56] = {
        0x01, 0x00, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00,
        0x08, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    };
    tRasterInfo raster;
    tVBuffer* pBuffer;

    STD_ZEROMEM(&raster, sizeof(raster));
    raster.width    = 2u;
    raster.height   = 2u;
    // Garbage: the backend recomputes these and the capture must not record them.
    raster.size     = (size_t)0x44444444u;
    raster.rowSize  = (size_t)0x55555555u;
    raster.rowWidth = (size_t)0x66666666u;
    raster.colorInfo.colorMode          = STDCOLOR_RGB;
    raster.colorInfo.bpp                = 32u;
    raster.colorInfo.redBPP             = 8u;
    raster.colorInfo.greenBPP           = 8u;
    raster.colorInfo.blueBPP            = 8u;
    raster.colorInfo.redPosShift        = 16;
    raster.colorInfo.greenPosShift      = 8;

    // The hand-written ColorInfo bytes must agree with the fields set above.
    TEST_ASSERT_EQUAL_MEMORY(&raster.colorInfo, aColor32, sizeof(aColor32));

    remove("stdRenderCaptureForward_Unlock.rcap");
    TEST_ASSERT_TRUE(stdDisplayCapture_Open("stdRenderCaptureForward_Unlock.rcap", "null"));

    pBuffer = stdDisplay_VBufferNew(&raster, 0, 1);
    TEST_ASSERT_NOT_NULL(pBuffer);
    TEST_ASSERT_EQUAL_INT(0, stdDisplay_VBufferFill(pBuffer, 0xDEADBEEFu, NULL));
    TEST_ASSERT_EQUAL_INT(1, stdDisplay_VBufferLock(pBuffer));
    TEST_ASSERT_EQUAL_INT(1, stdDisplay_VBufferUnlock(pBuffer)); // lockRefCount was 2: no blob
    TEST_ASSERT_EQUAL_INT(1, stdDisplay_VBufferUnlock(pBuffer)); // lockRefCount was 1: blob 1
    stdDisplay_VBufferFree(pBuffer);

    stdDisplayCapture_Close();
    stdRenderCaptureForward_AssertFileBytes("stdRenderCaptureForward_Unlock.rcap", aExpected, sizeof(aExpected));
}

TEST(stdRenderCaptureForward, TestForwardersPassThroughWithoutCapture)
{
    ColorInfo colorInfo;

    TEST_ASSERT_FALSE(stdRenderCapture_IsOpen());

    // Every forwarder still returns exactly what the Null backend returns and writes nothing:
    // the capture stays closed, so a write anywhere would be a hard error in the writer.
    TEST_ASSERT_EQUAL_INT(0, stdDisplay_CanRenderWindowed());
    TEST_ASSERT_EQUAL_size_t(3u, stdDisplay_GetAdapterNum(3u));
    TEST_ASSERT_EQUAL_size_t(0u, std3D_GetNumDevices());
    TEST_ASSERT_FALSE(stdDisplay_IsOpen());

    STD_ZEROMEM(&colorInfo, sizeof(colorInfo));
    TEST_ASSERT_EQUAL_INT(STDCOLOR_FORMAT_RGB, std3D_GetColorFormat(&colorInfo));
    // The Null backend has no current video mode here, so its zeroed ColorInfo shifts every
    // channel down to bit 0 and 0xFFFF comes back as 0xFF; the forwarder must pass that through.
    TEST_ASSERT_EQUAL_UINT32(0xFFu, stdDisplay_EncodeFromRGB565(0xFFFFu));

    std3D_SetRenderState(STD3D_RS_FOG_ENABLED);
    stdDisplay_BackBufferFill(0u, NULL);
    TEST_ASSERT_EQUAL_INT(0, stdDisplay_FlipToGDISurface());

    remove("stdRenderCaptureForward_PassThrough.rcap");
    TEST_ASSERT_FALSE(stdRenderCapture_IsOpen());
}

TEST_GROUP_RUNNER(stdRenderCaptureForward)
{
    RUN_TEST_CASE(stdRenderCaptureForward, TestOpenWritesHeaderOnlyAndBackBufferHandle);
    RUN_TEST_CASE(stdRenderCaptureForward, TestVBufferNewFillFreeBytes);
    RUN_TEST_CASE(stdRenderCaptureForward, TestFogGlobalAroundStd3DRecords);
    RUN_TEST_CASE(stdRenderCaptureForward, TestDrawRenderListTextureHandles);
    RUN_TEST_CASE(stdRenderCaptureForward, TestFrameWindowFiltering);
    RUN_TEST_CASE(stdRenderCaptureForward, TestVBufferUnlockRecordsWithFramesDisabled);
    RUN_TEST_CASE(stdRenderCaptureForward, TestVBufferUnlockBlob);
    RUN_TEST_CASE(stdRenderCaptureForward, TestForwardersPassThroughWithoutCapture);
}

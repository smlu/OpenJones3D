#include <unity_fixture.h>

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <std/General/std.h>
#include <std/General/stdBmp.h>
#include <std/General/stdColor.h>
#include <std/General/stdUtil.h>

#include "stdGeneralTest.h"

static void stdBmpTest_DeleteFile(const char* pPath)
{
    StdGeneralTest_DeleteFile(pPath);
}

static void stdBmpTest_DeleteRejectFiles(void)
{
    for ( size_t i = 0; i < 8u; ++i )
    {
        char aPath[64];
        STD_FORMAT(aPath, "stdBmpTest_reject_%u.bmp", (unsigned)i);
        stdBmpTest_DeleteFile(aPath);
    }
}

static void stdBmpTest_DeleteGeneratedFiles(void)
{
    stdBmpTest_DeleteFile("stdBmpTest_generated.bmp");
}

static void stdBmpTest_SetBgr(uint8_t* pPixel, uint8_t r, uint8_t g, uint8_t b)
{
    pPixel[0] = b;
    pPixel[1] = g;
    pPixel[2] = r;
}

static void stdBmpTest_AssertFilesEqual(const char* pExpectedPath, const char* pActualPath)
{
    tFileHandle expectedFile;
    tFileHandle actualFile;
    size_t expectedSize = stdFileSize(pExpectedPath);
    size_t actualSize   = stdFileSize(pActualPath);

    TEST_ASSERT_EQUAL_size_t(expectedSize, actualSize);

    expectedFile = stdFileOpen(pExpectedPath, "rb");
    actualFile   = stdFileOpen(pActualPath, "rb");
    TEST_ASSERT_NOT_EQUAL(0u, expectedFile);
    TEST_ASSERT_NOT_EQUAL(0u, actualFile);

    for ( size_t i = 0; i < expectedSize; ++i )
    {
        uint8_t expectedByte = 0;
        uint8_t actualByte = 0;

        TEST_ASSERT_EQUAL_size_t(1u, stdFileRead(expectedFile, &expectedByte, 1u));
        TEST_ASSERT_EQUAL_size_t(1u, stdFileRead(actualFile, &actualByte, 1u));
        TEST_ASSERT_EQUAL_UINT8(expectedByte, actualByte);
    }

    TEST_ASSERT_EQUAL_INT(0, stdFileClose(actualFile));
    TEST_ASSERT_EQUAL_INT(0, stdFileClose(expectedFile));
}

static void stdBmpTest_AssertBitmapInfo(HBITMAP hBitmap, LONG width, LONG height, WORD bitsPerPixel)
{
    BITMAP bitmap;

    STD_ZEROMEM(&bitmap, sizeof(bitmap));
    TEST_ASSERT_EQUAL_INT(sizeof(bitmap), GetObject(hBitmap, sizeof(bitmap), &bitmap));
    TEST_ASSERT_EQUAL_INT32(width, bitmap.bmWidth);
    TEST_ASSERT_EQUAL_INT32(height, bitmap.bmHeight);
    TEST_ASSERT_EQUAL_UINT16(bitsPerPixel, bitmap.bmBitsPixel);
}

static void stdBmpTest_AssertBitmapPixelsMatchFile(HBITMAP hBitmap, const char* pPath)
{
    DIBSECTION dib;
    STD_ZEROMEM(&dib, sizeof(dib));
    TEST_ASSERT_EQUAL_INT(sizeof(dib), GetObject(hBitmap, sizeof(dib), &dib));
    TEST_ASSERT_NOT_NULL(dib.dsBm.bmBits);
    TEST_ASSERT_EQUAL_UINT16(1u, dib.dsBm.bmPlanes);
    TEST_ASSERT_EQUAL_UINT16(24u, dib.dsBm.bmBitsPixel);

    tFileHandle fh = stdFileOpen(pPath, "rb");
    TEST_ASSERT_NOT_EQUAL(0u, fh);
    BITMAPFILEHEADER fileHeader;
    BITMAPINFOHEADER infoHeader;
    TEST_ASSERT_EQUAL_size_t(sizeof(fileHeader), stdFileRead(fh, &fileHeader, sizeof(fileHeader)));
    TEST_ASSERT_EQUAL_size_t(sizeof(infoHeader), stdFileRead(fh, &infoHeader, sizeof(infoHeader)));
    TEST_ASSERT_EQUAL_INT32(infoHeader.biWidth, dib.dsBm.bmWidth);
    TEST_ASSERT_EQUAL_INT32(infoHeader.biHeight, dib.dsBm.bmHeight);
    size_t stride = ((size_t)infoHeader.biWidth * 3u + 3u) & ~(size_t)3u;
    TEST_ASSERT_EQUAL_INT((int)stride, dib.dsBm.bmWidthBytes);
    TEST_ASSERT_EQUAL_INT(0, stdFileSeek(fh, (int)fileHeader.bfOffBits, SEEK_SET));

    uint8_t aExpectedRow[4096];
    TEST_ASSERT_TRUE(stride <= sizeof(aExpectedRow));
    for ( LONG row = 0; row < infoHeader.biHeight; ++row )
    {
        TEST_ASSERT_EQUAL_size_t(stride, stdFileRead(fh, aExpectedRow, stride));
        TEST_ASSERT_EQUAL_MEMORY(aExpectedRow, (uint8_t*)dib.dsBm.bmBits + (size_t)row * stride, stride);
    }
    TEST_ASSERT_EQUAL_INT(0, stdFileClose(fh));
}

static void stdBmpTest_AssertLoadBitmapVector(const char* pVectorPath, const char* pTempPath, LONG width, LONG height, WORD bitsPerPixel)
{
    HBITMAP hBitmap;

    TEST_ASSERT_TRUE(StdGeneralTest_CopyTestVectorFile(pVectorPath, pTempPath));
    hBitmap = stdBmp_Load(pTempPath);
    TEST_ASSERT_NOT_NULL(hBitmap);
    stdBmpTest_AssertBitmapInfo(hBitmap, width, height, bitsPerPixel);
    stdBmpTest_AssertBitmapPixelsMatchFile(hBitmap, pTempPath);
    DeleteObject(hBitmap);
}

static void stdBmpTest_AssertRejectsBitmapVector(const char* pVectorPath, const char* pTempPath)
{
    TEST_ASSERT_TRUE(StdGeneralTest_CopyTestVectorFile(pVectorPath, pTempPath));
    TEST_ASSERT_NULL(stdBmp_Load(pTempPath));
}

static tVBuffer stdBmpTest_MakeVBuffer(uint8_t* pPixels, uint32_t width, uint32_t height, const ColorInfo* pColorInfo)
{
    tVBuffer vbuffer;

    STD_ZEROMEM(&vbuffer, sizeof(vbuffer));
    vbuffer.type = VBUFFER_SOFTWARE;
    vbuffer.rasterInfo.width = width;
    vbuffer.rasterInfo.height = height;
    vbuffer.rasterInfo.rowSize = width * (pColorInfo->bpp / 8u);
    vbuffer.rasterInfo.rowWidth = vbuffer.rasterInfo.rowSize;
    vbuffer.rasterInfo.size = vbuffer.rasterInfo.rowSize * height;
    vbuffer.rasterInfo.colorInfo = *pColorInfo;
    vbuffer.pPixels = pPixels;
    return vbuffer;
}

static void stdBmpTest_WriteBmpHeaderEx(
    const char* pPath,
    WORD bitCount,
    DWORD compression,
    LONG width,
    LONG height,
    DWORD pixelBytes,
    DWORD offBits,
    DWORD fileSize,
    DWORD infoHeaderSize
)
{
    BITMAPFILEHEADER fileHeader;
    BITMAPINFOHEADER infoHeader;
    tFileHandle fh;

    STD_ZEROMEM(&fileHeader, sizeof(fileHeader));
    STD_ZEROMEM(&infoHeader, sizeof(infoHeader));

    fileHeader.bfType = 0x4D42;
    fileHeader.bfOffBits = offBits;
    fileHeader.bfSize = fileSize;

    infoHeader.biSize = infoHeaderSize;
    infoHeader.biWidth = width;
    infoHeader.biHeight = height;
    infoHeader.biPlanes = 1;
    infoHeader.biBitCount = bitCount;
    infoHeader.biCompression = compression;
    infoHeader.biSizeImage = pixelBytes;

    stdBmpTest_DeleteFile(pPath);
    fh = stdFileOpen(pPath, "wb");
    TEST_ASSERT_NOT_EQUAL(0u, fh);
    TEST_ASSERT_EQUAL_size_t(sizeof(fileHeader), stdFileWrite(fh, &fileHeader, sizeof(fileHeader)));
    TEST_ASSERT_EQUAL_size_t(sizeof(infoHeader), stdFileWrite(fh, &infoHeader, sizeof(infoHeader)));
    TEST_ASSERT_EQUAL_INT(0, stdFileClose(fh));
}

static void stdBmpTest_WriteBmpHeader(const char* pPath, WORD bitCount, DWORD compression, LONG width, LONG height, DWORD pixelBytes)
{
    const DWORD offBits = (DWORD)(sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER));

    stdBmpTest_WriteBmpHeaderEx(
        pPath,
        bitCount,
        compression,
        width,
        height,
        pixelBytes,
        offBits,
        offBits + pixelBytes,
        (DWORD)sizeof(BITMAPINFOHEADER)
    );
}

TEST_GROUP(stdBmp);

TEST_SETUP(stdBmp)
{
    StdGeneralTest_Startup();
}

TEST_TEAR_DOWN(stdBmp)
{
    stdBmpTest_DeleteFile("stdBmpTest_valid.bmp");
    stdBmpTest_DeleteFile("stdBmpTest_expected.bmp");
    stdBmpTest_DeleteFile("stdBmpTest_invalid.bmp");
    stdBmpTest_DeleteFile("stdBmpTest_unlocked.bmp");
    stdBmpTest_DeleteFile("stdBmpTest_bmpsuite_rgb24.bmp");
    stdBmpTest_DeleteFile("stdBmpTest_bmpsuite_rgb24pal.bmp");
    stdBmpTest_DeleteRejectFiles();
    stdBmpTest_DeleteGeneratedFiles();
    StdGeneralTest_Shutdown();
}

TEST(stdBmp, TestWriteVBufferCreatesBottomUpPadded24BppBmp)
{
    const char* pPath = "stdBmpTest_valid.bmp";
    const char* pExpectedPath = "stdBmpTest_expected.bmp";
    BITMAPFILEHEADER fileHeader;
    BITMAPINFOHEADER infoHeader;
    uint8_t aPixels[12];
    uint8_t aRow[8];
    tVBuffer vbuffer;
    tFileHandle fh;

    stdBmpTest_SetBgr(&aPixels[0], 255, 0, 0);
    stdBmpTest_SetBgr(&aPixels[3], 0, 255, 0);
    stdBmpTest_SetBgr(&aPixels[6], 0, 0, 255);
    stdBmpTest_SetBgr(&aPixels[9], 255, 255, 255);
    vbuffer = stdBmpTest_MakeVBuffer(aPixels, 2u, 2u, &stdColor_cfRGB888);

    TEST_ASSERT_TRUE(StdGeneralTest_CopyTestVectorFile("stdBmp/valid_2x2_24bpp.bmp", pExpectedPath));
    TEST_ASSERT_EQUAL_INT(0, stdBmp_WriteVBuffer(pPath, &vbuffer));
    TEST_ASSERT_EQUAL_size_t(70u, stdFileSize(pPath));
    stdBmpTest_AssertFilesEqual(pExpectedPath, pPath);

    fh = stdFileOpen(pPath, "rb");
    TEST_ASSERT_NOT_EQUAL(0u, fh);
    TEST_ASSERT_EQUAL_size_t(sizeof(fileHeader), stdFileRead(fh, &fileHeader, sizeof(fileHeader)));
    TEST_ASSERT_EQUAL_size_t(sizeof(infoHeader), stdFileRead(fh, &infoHeader, sizeof(infoHeader)));
    TEST_ASSERT_EQUAL_UINT16(0x4D42u, fileHeader.bfType);
    TEST_ASSERT_EQUAL_UINT32(70u, fileHeader.bfSize);
    TEST_ASSERT_EQUAL_UINT32(54u, fileHeader.bfOffBits);
    TEST_ASSERT_EQUAL_INT32(2, infoHeader.biWidth);
    TEST_ASSERT_EQUAL_INT32(2, infoHeader.biHeight);
    TEST_ASSERT_EQUAL_UINT16(24u, infoHeader.biBitCount);
    TEST_ASSERT_EQUAL_UINT32(16u, infoHeader.biSizeImage);

    TEST_ASSERT_EQUAL_size_t(sizeof(aRow), stdFileRead(fh, aRow, sizeof(aRow)));
    TEST_ASSERT_EQUAL_UINT8(255u, aRow[0]);
    TEST_ASSERT_EQUAL_UINT8(0u, aRow[1]);
    TEST_ASSERT_EQUAL_UINT8(0u, aRow[2]);
    TEST_ASSERT_EQUAL_UINT8(255u, aRow[3]);
    TEST_ASSERT_EQUAL_UINT8(255u, aRow[4]);
    TEST_ASSERT_EQUAL_UINT8(255u, aRow[5]);
    TEST_ASSERT_EQUAL_UINT8(0u, aRow[6]);
    TEST_ASSERT_EQUAL_UINT8(0u, aRow[7]);
    TEST_ASSERT_EQUAL_INT(0, stdFileClose(fh));
}

TEST(stdBmp, TestLoadAcceptsWritten24BppBmpAndRejectsNullOrMissing)
{
    const char* pPath = "stdBmpTest_valid.bmp";
    HBITMAP hBitmap;

    TEST_ASSERT_TRUE(StdGeneralTest_CopyTestVectorFile("stdBmp/valid_2x2_24bpp.bmp", pPath));
    hBitmap = stdBmp_Load(pPath);
    TEST_ASSERT_NOT_NULL(hBitmap);
    stdBmpTest_AssertBitmapPixelsMatchFile(hBitmap, pPath);
    DeleteObject(hBitmap);

    TEST_ASSERT_NULL(stdBmp_Load(NULL));
    TEST_ASSERT_NULL(stdBmp_Load("stdBmpTest_missing.bmp"));
}

TEST(stdBmp, TestLoadAcceptsBmpSuiteRgb24References)
{
    stdBmpTest_AssertLoadBitmapVector("stdBmp/bmpsuite_g_rgb24.bmp", "stdBmpTest_bmpsuite_rgb24.bmp", 127, 64, 24u);
    stdBmpTest_AssertLoadBitmapVector("stdBmp/bmpsuite_g_rgb24pal.bmp", "stdBmpTest_bmpsuite_rgb24pal.bmp", 127, 64, 24u);
}

TEST(stdBmp, TestWriteRejectsInvalidInputsFormatsDimensionsAndUnlockedBuffers)
{
    uint8_t aPixels[3];
    tVBuffer vbuffer;
    ColorInfo palInfo = stdColor_cfRGB888;

    stdBmpTest_SetBgr(aPixels, 1, 2, 3);
    vbuffer = stdBmpTest_MakeVBuffer(aPixels, 1u, 1u, &stdColor_cfRGB888);

#ifdef J3D_RUNTIME_GUARDS
    TEST_ASSERT_EQUAL_INT(1, stdBmp_WriteVBuffer(NULL, &vbuffer));
    TEST_ASSERT_EQUAL_INT(1, stdBmp_WriteVBuffer("stdBmpTest_invalid.bmp", NULL));
#endif

    palInfo.colorMode = STDCOLOR_PAL;
    vbuffer = stdBmpTest_MakeVBuffer(aPixels, 1u, 1u, &palInfo);
    TEST_ASSERT_EQUAL_INT(1, stdBmp_WriteVBuffer("stdBmpTest_invalid.bmp", &vbuffer));

    vbuffer = stdBmpTest_MakeVBuffer(aPixels, 0u, 1u, &stdColor_cfRGB888);
    TEST_ASSERT_EQUAL_INT(1, stdBmp_WriteVBuffer("stdBmpTest_invalid.bmp", &vbuffer));

#ifdef J3D_RUNTIME_GUARDS
    vbuffer = stdBmpTest_MakeVBuffer(NULL, 1u, 1u, &stdColor_cfRGB888);
    TEST_ASSERT_EQUAL_INT(1, stdBmp_WriteVBuffer("stdBmpTest_unlocked.bmp", &vbuffer));
#endif
}

TEST(stdBmp, TestWriteRejectsFileAndAllocationFailures)
{
    uint8_t aPixels[3];
    tVBuffer vbuffer;

    stdBmpTest_SetBgr(aPixels, 16, 32, 48);
    vbuffer = stdBmpTest_MakeVBuffer(aPixels, 1u, 1u, &stdColor_cfRGB888);

    StdGeneralTest_FailNextFileOpens(0);
    TEST_ASSERT_EQUAL_INT(1, stdBmp_WriteVBuffer("stdBmpTest_invalid.bmp", &vbuffer));
    StdGeneralTest_ClearFileFailures();

    StdGeneralTest_FailNextFileWrites(0);
    TEST_ASSERT_EQUAL_INT(1, stdBmp_WriteVBuffer("stdBmpTest_invalid.bmp", &vbuffer));
    StdGeneralTest_ClearFileFailures();

    StdGeneralTest_FailNextFileWrites(1);
    TEST_ASSERT_EQUAL_INT(1, stdBmp_WriteVBuffer("stdBmpTest_invalid.bmp", &vbuffer));
    StdGeneralTest_ClearFileFailures();

    StdGeneralTest_FailNextAllocations(0);
    TEST_ASSERT_EQUAL_INT(1, stdBmp_WriteVBuffer("stdBmpTest_invalid.bmp", &vbuffer));
    StdGeneralTest_ClearAllocationFailures();

    StdGeneralTest_FailNextFileWrites(2);
    TEST_ASSERT_EQUAL_INT(1, stdBmp_WriteVBuffer("stdBmpTest_invalid.bmp", &vbuffer));
    StdGeneralTest_ClearFileFailures();
}

TEST(stdBmp, TestGeneratedWriteLoadRoundTripDimensionsAndPadding)
{
    const char* pPath = "stdBmpTest_generated.bmp";
    uint8_t aPixels[7u * 5u * 3u];

    for ( uint32_t height = 1u; height <= 5u; ++height )
    {
        for ( uint32_t width = 1u; width <= 7u; ++width )
        {
            HBITMAP hBitmap;
            tVBuffer vbuffer;

            for ( uint32_t y = 0u; y < height; ++y )
            {
                for ( uint32_t x = 0u; x < width; ++x )
                {
                    size_t pixelOffset = (size_t)(y * width + x) * 3u;

                    stdBmpTest_SetBgr(
                        &aPixels[pixelOffset],
                        (uint8_t)(x * 31u + y),
                        (uint8_t)(y * 47u + x),
                        (uint8_t)(x * 11u + y * 13u)
                    );
                }
            }

            vbuffer = stdBmpTest_MakeVBuffer(aPixels, width, height, &stdColor_cfRGB888);
            stdBmpTest_DeleteFile(pPath);

            TEST_ASSERT_EQUAL_INT(0, stdBmp_WriteVBuffer(pPath, &vbuffer));

            {
                size_t rowBytes     = (size_t)width * 3u;
                size_t stride       = (rowBytes + 3u) & ~3u;
                size_t expectedSize = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + stride * height;

                TEST_ASSERT_EQUAL_size_t(expectedSize, stdFileSize(pPath));
            }

            hBitmap = stdBmp_Load(pPath);
            TEST_ASSERT_NOT_NULL(hBitmap);
            stdBmpTest_AssertBitmapInfo(hBitmap, (LONG)width, (LONG)height, 24u);
            stdBmpTest_AssertBitmapPixelsMatchFile(hBitmap, pPath);
            DeleteObject(hBitmap);
        }
    }
}

TEST(stdBmp, TestLoadRejectsInvalidUnsupportedCompressedAndTruncatedBmpFiles)
{
    const char* pPath = "stdBmpTest_invalid.bmp";

    stdBmpTest_WriteBmpHeader(pPath, 8u, BI_RGB, 1, 1, 4u);
    TEST_ASSERT_NULL(stdBmp_Load(pPath));

    stdBmpTest_WriteBmpHeader(pPath, 16u, BI_RGB, 1, 1, 4u);
    TEST_ASSERT_NULL(stdBmp_Load(pPath));

    stdBmpTest_WriteBmpHeader(pPath, 32u, BI_RGB, 1, 1, 4u);
    TEST_ASSERT_NULL(stdBmp_Load(pPath));

    stdBmpTest_WriteBmpHeader(pPath, 24u, BI_RLE8, 1, 1, 4u);
    TEST_ASSERT_NULL(stdBmp_Load(pPath));

    stdBmpTest_WriteBmpHeader(pPath, 24u, BI_RGB, 1, 1, 4u);
    TEST_ASSERT_NULL(stdBmp_Load(pPath));

    stdBmpTest_AssertRejectsBitmapVector("stdBmp/bmpsuite_b_badheadersize.bmp", "stdBmpTest_reject_0.bmp");
    stdBmpTest_AssertRejectsBitmapVector("stdBmp/bmpsuite_b_badbitcount.bmp", "stdBmpTest_reject_1.bmp");
    stdBmpTest_AssertRejectsBitmapVector("stdBmp/bmpsuite_b_reallybig.bmp", "stdBmpTest_reject_2.bmp");
    stdBmpTest_AssertRejectsBitmapVector("stdBmp/bmpsuite_b_shortfile.bmp", "stdBmpTest_reject_3.bmp");
    stdBmpTest_AssertRejectsBitmapVector("stdBmp/bmpsuite_q_rgb24png.bmp", "stdBmpTest_reject_4.bmp");
}

TEST(stdBmp, TestGeneratedHeaderMutationCorpusRejects)
{
    const char* pPath = "stdBmpTest_invalid.bmp";
    static const LONG aWidths[] = { 0, -1, 1 };
    static const LONG aHeights[] = { 1, 0, -1 };
    static const DWORD aCompressions[] = { BI_RLE8, BI_RLE4, BI_BITFIELDS };

    for ( WORD bitCount = 0u; bitCount <= 32u; ++bitCount )
    {
        if ( bitCount != 24u )
        {
            stdBmpTest_WriteBmpHeader(pPath, bitCount, BI_RGB, 1, 1, 4u);
            TEST_ASSERT_NULL(stdBmp_Load(pPath));
        }
    }

    for ( size_t i = 0; i < STD_ARRAYLEN(aWidths); ++i )
    {
        stdBmpTest_WriteBmpHeader(pPath, 24u, BI_RGB, aWidths[i], aHeights[i], 4u);
        TEST_ASSERT_NULL(stdBmp_Load(pPath));
    }

    for ( size_t i = 0; i < STD_ARRAYLEN(aCompressions); ++i )
    {
        stdBmpTest_WriteBmpHeader(pPath, 24u, aCompressions[i], 1, 1, 4u);
        TEST_ASSERT_NULL(stdBmp_Load(pPath));
    }

    stdBmpTest_WriteBmpHeaderEx(
        pPath,
        24u,
        BI_RGB,
        1,
        1,
        4u,
        UINT32_MAX,
        UINT32_MAX,
        (DWORD)sizeof(BITMAPINFOHEADER)
    );
    TEST_ASSERT_NULL(stdBmp_Load(pPath));

    stdBmpTest_WriteBmpHeaderEx(
        pPath,
        24u,
        BI_RGB,
        1,
        1,
        4u,
        (DWORD)(sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER)),
        (DWORD)(sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + 4u),
        (DWORD)(sizeof(BITMAPINFOHEADER) - 1u)
    );
    TEST_ASSERT_NULL(stdBmp_Load(pPath));
}

TEST(stdBmp, TestLoadRejectsReadAndSeekFailures)
{
    const char* pPath = "stdBmpTest_valid.bmp";

    TEST_ASSERT_TRUE(StdGeneralTest_CopyTestVectorFile("stdBmp/valid_2x2_24bpp.bmp", pPath));

    for ( int successfulReads = 0; successfulReads < 3; ++successfulReads )
    {
        StdGeneralTest_FailNextFileReads(successfulReads);
        TEST_ASSERT_NULL(stdBmp_Load(pPath));
        StdGeneralTest_ClearFileFailures();
    }

    StdGeneralTest_FailNextFileSeeks(0);
    TEST_ASSERT_NULL(stdBmp_Load(pPath));
    StdGeneralTest_ClearFileFailures();

    StdGeneralTest_FailNextFileReads(2);
    TEST_ASSERT_NULL(stdBmp_Load(pPath));
    StdGeneralTest_ClearFileFailures();
}

TEST(stdBmp, TestWriteDimensionLimitsRejectBeforeAccessingPixels)
{
    uint8_t aPixel[3] = { 1u, 2u, 3u };
    tVBuffer vbuffer = stdBmpTest_MakeVBuffer(aPixel, 1u, 1u, &stdColor_cfRGB888);
    // Zero and overflowing dimensions must fail before locking or reading the pixel buffer.
    static const uint32_t aDimensions[][2] = {
        { 0u, 1u }, { 1u, 0u }, { UINT32_MAX, 1u }, { 1u, UINT32_MAX },
        { INT32_MAX, 1u }, { 1u, INT32_MAX }, { 2u, INT32_MAX }
    };
    for ( size_t i = 0u; i < STD_ARRAYLEN(aDimensions); ++i )
    {
        vbuffer.rasterInfo.width  = aDimensions[i][0];
        vbuffer.rasterInfo.height = aDimensions[i][1];
        TEST_ASSERT_EQUAL_INT(1, stdBmp_WriteVBuffer("stdBmpTest_invalid.bmp", &vbuffer));
        TEST_ASSERT_EQUAL_size_t(0u, vbuffer.lockRefCount);
        TEST_ASSERT_EQUAL_MEMORY("\x01\x02\x03", aPixel, sizeof(aPixel));
    }
}

TEST_GROUP_RUNNER(stdBmp)
{
    RUN_TEST_CASE(stdBmp, TestWriteDimensionLimitsRejectBeforeAccessingPixels);
    RUN_TEST_CASE(stdBmp, TestWriteVBufferCreatesBottomUpPadded24BppBmp);
    RUN_TEST_CASE(stdBmp, TestLoadAcceptsWritten24BppBmpAndRejectsNullOrMissing);
    RUN_TEST_CASE(stdBmp, TestLoadAcceptsBmpSuiteRgb24References);
    RUN_TEST_CASE(stdBmp, TestWriteRejectsInvalidInputsFormatsDimensionsAndUnlockedBuffers);
    RUN_TEST_CASE(stdBmp, TestWriteRejectsFileAndAllocationFailures);
    RUN_TEST_CASE(stdBmp, TestGeneratedWriteLoadRoundTripDimensionsAndPadding);
    RUN_TEST_CASE(stdBmp, TestLoadRejectsInvalidUnsupportedCompressedAndTruncatedBmpFiles);
    RUN_TEST_CASE(stdBmp, TestGeneratedHeaderMutationCorpusRejects);
    RUN_TEST_CASE(stdBmp, TestLoadRejectsReadAndSeekFailures);
}

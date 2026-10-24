#include <unity_fixture.h>

#include <stdint.h>
#include <string.h>

#include <j3dcore/Tests/j3dTest.h>
#include <std/General/stdColor.h>
#include <std/General/stdUtil.h>

#include "stdGeneralTest.h"

uint32_t J3DAPI stdColor_ScaleColorComponent(uint32_t cc, int srcBPP, int deltaBPP);

static const ColorInfo stdColorTest_cfRGB332 =
{
    .colorMode          = STDCOLOR_RGB,
    .bpp                = 8,
    .redBPP             = 3,
    .greenBPP           = 3,
    .blueBPP            = 2,
    .redPosShift        = 5,
    .greenPosShift      = 2,
    .bluePosShift       = 0,
    .redPosShiftRight   = 5,
    .greenPosShiftRight = 5,
    .bluePosShiftRight  = 6,
    .alphaBPP           = 0,
    .alphaPosShift      = 0,
    .alphaPosShiftRight = 0
};

TEST_GROUP(stdColor);

TEST_SETUP(stdColor)
{
    StdGeneralTest_Startup();
}

TEST_TEAR_DOWN(stdColor)
{
    StdGeneralTest_Shutdown();
}

static void stdColorTest_AssertRGBWithin(uint8_t r, uint8_t g, uint8_t b, uint8_t er, uint8_t eg, uint8_t eb, uint8_t tolerance)
{
    TEST_ASSERT_UINT8_WITHIN(tolerance, er, r);
    TEST_ASSERT_UINT8_WITHIN(tolerance, eg, g);
    TEST_ASSERT_UINT8_WITHIN(tolerance, eb, b);
}

static void stdColorTest_StorePixel(uint8_t* pPixel, uint32_t bpp, uint32_t encoded)
{
    switch ( bpp )
    {
        case 8:
            pPixel[0] = (uint8_t)encoded;
            break;

        case 16:
            memcpy(pPixel, &encoded, sizeof(uint16_t));
            break;

        case 24:
            pPixel[0] = (uint8_t)encoded;
            pPixel[1] = (uint8_t)(encoded >> 8u);
            pPixel[2] = (uint8_t)(encoded >> 16u);
            break;

        case 32:
            memcpy(pPixel, &encoded, sizeof(encoded));
            break;

        default:
            TEST_FAIL_MESSAGE("Unsupported test pixel depth.");
    }
}

static uint32_t stdColorTest_LoadPixel(const uint8_t* pPixel, uint32_t bpp)
{
    uint32_t encoded = 0;
    switch ( bpp )
    {
        case 8:
            encoded = pPixel[0];
            break;

        case 16:
            memcpy(&encoded, pPixel, sizeof(uint16_t));
            break;

        case 24:
            encoded = pPixel[0] | ((uint32_t)pPixel[1] << 8u) | ((uint32_t)pPixel[2] << 16u);
            break;

        case 32:
            memcpy(&encoded, pPixel, sizeof(encoded));
            break;

        default:
            TEST_FAIL_MESSAGE("Unsupported test pixel depth.");
    }

    return encoded;
}

static uint32_t stdColorTest_PreviousFixedScaleColorComponent(uint32_t cc, int srcBPP, int deltaBPP)
{
    int dsrcBPP = srcBPP + deltaBPP;
    return (cc << -deltaBPP)
        | (dsrcBPP >= 0
            ? cc >> dsrcBPP
            : cc * ((1u << -deltaBPP) - 1u));
}

static uint32_t stdColorTest_PreviousFixedDecodeComponent(uint32_t encoded, uint32_t bpp, int32_t posShift)
{
    uint32_t mask      = (1u << bpp) - 1u;
    uint32_t component = (encoded >> posShift) & mask;
    return stdColorTest_PreviousFixedScaleColorComponent(component, (int)bpp, (int)bpp - 8);
}

TEST(stdColor, TestPackedColorMacros)
{
    tStdColor rgb = STD_RGB(0x12, 0x34, 0x56);
    tStdColor rgba = STD_RGBA(0x12, 0x34, 0x56, 0x78);

    TEST_ASSERT_EQUAL_HEX32(0xFF123456u, rgb);
    TEST_ASSERT_EQUAL_HEX32(0x78123456u, rgba);
    TEST_ASSERT_EQUAL_HEX8(0xFFu, STD_GETALPHA(rgb));
    TEST_ASSERT_EQUAL_HEX8(0x12u, STD_GETRED(rgba));
    TEST_ASSERT_EQUAL_HEX8(0x34u, STD_GETGREEN(rgba));
    TEST_ASSERT_EQUAL_HEX8(0x56u, STD_GETBLUE(rgba));
}

TEST(stdColor, TestRGBtoHSV)
{
    float h;
    float s;
    float v;

    stdColor_RGBtoHSV(255.0f, 0.0f, 0.0f, &h, &s, &v);
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, 0.0f, h);
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, 1.0f, s);
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, 1.0f, v);

    stdColor_RGBtoHSV(0.0f, 255.0f, 0.0f, &h, &s, &v);
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, 120.0f, h);
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, 1.0f, s);
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, 1.0f, v);

    stdColor_RGBtoHSV(0.0f, 0.0f, 255.0f, &h, &s, &v);
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, 240.0f, h);
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, 1.0f, s);
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, 1.0f, v);

    stdColor_RGBtoHSV(128.0f, 128.0f, 128.0f, &h, &s, &v);
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, 0.0f, h);
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, 0.0f, s);
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, 128.0f / 255.0f, v);

    stdColor_RGBtoHSV(0.0f, 0.0f, 0.0f, &h, &s, &v);
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, 0.0f, h);
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, 0.0f, s);
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, 0.0f, v);

    // Magenta exercises the negative intermediate hue wrapped back into [0, 360).
    stdColor_RGBtoHSV(255.0f, 0.0f, 255.0f, &h, &s, &v);
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, 300.0f, h);
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, 1.0f, s);
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, 1.0f, v);
}

TEST(stdColor, TestHSVtoRGB)
{
    const struct
    {
        float hue;
        float r;
        float g;
        float b;
    } cases[] = {
        { 0.0f,   255.0f, 0.0f,   0.0f },
        { 60.0f,  255.0f, 255.0f, 0.0f },
        { 120.0f, 0.0f,   255.0f, 0.0f },
        { 180.0f, 0.0f,   255.0f, 255.0f },
        { 240.0f, 0.0f,   0.0f,   255.0f },
        { 300.0f, 255.0f, 0.0f,   255.0f },
        { 360.0f, 255.0f, 0.0f,   0.0f }
    };

    for ( size_t i = 0; i < STD_ARRAYLEN(cases); ++i )
    {
        float r = -1.0f;
        float g = -1.0f;
        float b = -1.0f;

        stdColor_HSVtoRGB(cases[i].hue, 1.0f, 1.0f, &r, &g, &b);
        TEST_ASSERT_FLOAT_WITHIN(0.0001f, cases[i].r, r);
        TEST_ASSERT_FLOAT_WITHIN(0.0001f, cases[i].g, g);
        TEST_ASSERT_FLOAT_WITHIN(0.0001f, cases[i].b, b);
    }

    {
        float r;
        float g;
        float b;

        stdColor_HSVtoRGB(37.0f, 0.0f, 0.5f, &r, &g, &b);
        TEST_ASSERT_FLOAT_WITHIN(0.0001f, 127.5f, r);
        TEST_ASSERT_FLOAT_WITHIN(0.0001f, 127.5f, g);
        TEST_ASSERT_FLOAT_WITHIN(0.0001f, 127.5f, b);
    }

    {
        float r = 11.0f;
        float g = 22.0f;
        float b = 33.0f;

        // Out-of-range sectors preserve the original early-return behavior.
        stdColor_HSVtoRGB(420.0f, 1.0f, 1.0f, &r, &g, &b);
        TEST_ASSERT_FLOAT_WITHIN(0.0001f, 11.0f, r);
        TEST_ASSERT_FLOAT_WITHIN(0.0001f, 22.0f, g);
        TEST_ASSERT_FLOAT_WITHIN(0.0001f, 33.0f, b);
    }
}

TEST(stdColor, TestScaleColorComponent)
{
    static const uint32_t aTwoBitExpected[]   = { 0u, 85u, 170u, 255u };
    static const uint32_t aThreeBitExpected[] = { 0u, 36u, 73u, 109u, 146u, 182u, 219u, 255u };

    TEST_ASSERT_EQUAL_UINT32(255u, stdColor_ScaleColorComponent(31u, 5, -3));
    TEST_ASSERT_EQUAL_UINT32(130u, stdColor_ScaleColorComponent(32u, 6, -2));
    TEST_ASSERT_EQUAL_UINT32(15u, stdColor_ScaleColorComponent(255u, 8, 4));
    TEST_ASSERT_EQUAL_UINT32(255u, stdColor_ScaleColorComponent(1u, 1, -7));

    for ( size_t value = 0; value < STD_ARRAYLEN(aTwoBitExpected); ++value )
    {
        TEST_ASSERT_EQUAL_UINT32(aTwoBitExpected[value], stdColor_ScaleColorComponent((uint32_t)value, 2, -6));
    }

    for ( size_t value = 0; value < STD_ARRAYLEN(aThreeBitExpected); ++value )
    {
        TEST_ASSERT_EQUAL_UINT32(aThreeBitExpected[value], stdColor_ScaleColorComponent((uint32_t)value, 3, -5));
    }

    for ( int bpp = 1; bpp <= 8; ++bpp )
    {
        uint32_t maxComponent = (1u << bpp) - 1u;
        TEST_ASSERT_EQUAL_UINT32(0u, stdColor_ScaleColorComponent(0u, bpp, bpp - 8));
        TEST_ASSERT_EQUAL_UINT32(255u, stdColor_ScaleColorComponent(maxComponent, bpp, bpp - 8));
    }
}

TEST(stdColor, TestRGB565UpscaleMatchesPreviousFixExhaustive)
{
    static const int aCompatibleBPP[] = { 1, 4, 5, 6, 7, 8 };

    // The prior fix was already correct for these channel widths. Preserve every output.
    for ( size_t bppIdx = 0; bppIdx < STD_ARRAYLEN(aCompatibleBPP); ++bppIdx )
    {
        int srcBPP         = aCompatibleBPP[bppIdx];
        int deltaBPP       = srcBPP - 8;
        uint32_t numValues = 1u << srcBPP;

        for ( uint32_t value = 0; value < numValues; ++value )
        {
            TEST_ASSERT_EQUAL_UINT32(
                stdColorTest_PreviousFixedScaleColorComponent(value, srcBPP, deltaBPP),
                stdColor_ScaleColorComponent(value, srcBPP, deltaBPP)
            );
        }
    }

    // Exercise every RGB565 pixel, including the green channel that exposed the old hue regression.
    for ( uint32_t rgb565 = 0; rgb565 <= UINT16_MAX; ++rgb565 )
    {
        uint32_t r        = stdColorTest_PreviousFixedScaleColorComponent((rgb565 >> 11u) & 0x1Fu, 5, -3);
        uint32_t g        = stdColorTest_PreviousFixedScaleColorComponent((rgb565 >> 5u) & 0x3Fu, 6, -2);
        uint32_t b        = stdColorTest_PreviousFixedScaleColorComponent(rgb565 & 0x1Fu, 5, -3);
        uint32_t expected = (r << 16u) | (g << 8u) | b;
        uint8_t decodedR;
        uint8_t decodedG;
        uint8_t decodedB;

        stdColor_DecodeRGB(rgb565, &stdColor_cfRGB565, &decodedR, &decodedG, &decodedB);
        TEST_ASSERT_EQUAL_UINT8((uint8_t)r, decodedR);
        TEST_ASSERT_EQUAL_UINT8((uint8_t)g, decodedG);
        TEST_ASSERT_EQUAL_UINT8((uint8_t)b, decodedB);
        TEST_ASSERT_EQUAL_HEX32(expected, stdColor_Recode(rgb565, &stdColor_cfRGB565, &stdColor_cfRGB888));
        TEST_ASSERT_EQUAL_HEX32(expected, stdColor_Recode(rgb565, &stdColor_cfRGB565, &stdColor_cfRGB8888));
        TEST_ASSERT_EQUAL_HEX32(expected, (uint32_t)stdColor_ColorConvertOnePixel((ColorInfo*)&stdColor_cfRGB888, (int)rgb565, (ColorInfo*)&stdColor_cfRGB565, 0, NULL));
        TEST_ASSERT_EQUAL_HEX32(expected, (uint32_t)stdColor_ColorConvertOnePixel((ColorInfo*)&stdColor_cfRGB8888, (int)rgb565, (ColorInfo*)&stdColor_cfRGB565, 0, NULL));
    }
}

TEST(stdColor, TestARGB16UpscaleMatchesPreviousFixExhaustive)
{
    static const ColorInfo* apFormats[] = {
        &stdColor_cfARGB4444,
        &stdColor_cfARGB5551
    };

    // ARGB5551 is the project's A1R5G5B5 layout; exercise both 16-bit alpha formats exhaustively.
    for ( size_t formatIdx = 0; formatIdx < STD_ARRAYLEN(apFormats); ++formatIdx )
    {
        const ColorInfo* pFormat = apFormats[formatIdx];

        for ( uint32_t encoded = 0; encoded <= UINT16_MAX; ++encoded )
        {
            uint32_t r            = stdColorTest_PreviousFixedDecodeComponent(encoded, pFormat->redBPP, pFormat->redPosShift);
            uint32_t g            = stdColorTest_PreviousFixedDecodeComponent(encoded, pFormat->greenBPP, pFormat->greenPosShift);
            uint32_t b            = stdColorTest_PreviousFixedDecodeComponent(encoded, pFormat->blueBPP, pFormat->bluePosShift);
            uint32_t a            = stdColorTest_PreviousFixedDecodeComponent(encoded, pFormat->alphaBPP, pFormat->alphaPosShift);
            uint32_t expectedARGB = (a << 24u) | (r << 16u) | (g << 8u) | b;
            uint32_t expectedRGBA = (r << 24u) | (g << 16u) | (b << 8u) | a;
            uint8_t decodedR;
            uint8_t decodedG;
            uint8_t decodedB;
            uint8_t decodedA;

            stdColor_DecodeRGBA(encoded, pFormat, &decodedR, &decodedG, &decodedB, &decodedA);
            TEST_ASSERT_EQUAL_UINT8((uint8_t)r, decodedR);
            TEST_ASSERT_EQUAL_UINT8((uint8_t)g, decodedG);
            TEST_ASSERT_EQUAL_UINT8((uint8_t)b, decodedB);
            TEST_ASSERT_EQUAL_UINT8((uint8_t)a, decodedA);
            TEST_ASSERT_EQUAL_HEX16((uint16_t)encoded, (uint16_t)stdColor_EncodeRGBA(pFormat, decodedR, decodedG, decodedB, decodedA));
            TEST_ASSERT_EQUAL_HEX32(expectedARGB, stdColor_Recode(encoded, pFormat, &stdColor_cfARGB8888));
            TEST_ASSERT_EQUAL_HEX32(expectedRGBA, stdColor_Recode(encoded, pFormat, &stdColor_cfRGBA8888));
            TEST_ASSERT_EQUAL_HEX32(expectedARGB, (uint32_t)stdColor_ColorConvertOnePixel((ColorInfo*)&stdColor_cfARGB8888, (int)encoded, (ColorInfo*)pFormat, 0, NULL));
            TEST_ASSERT_EQUAL_HEX32(expectedRGBA, (uint32_t)stdColor_ColorConvertOnePixel((ColorInfo*)&stdColor_cfRGBA8888, (int)encoded, (ColorInfo*)pFormat, 0, NULL));
            TEST_ASSERT_EQUAL_HEX16((uint16_t)encoded, (uint16_t)stdColor_Recode(encoded, pFormat, pFormat));
        }
    }
}

TEST(stdColor, TestEncodeDecodeRGB)
{
    uint8_t r;
    uint8_t g;
    uint8_t b;
    uint32_t rgb565 = stdColor_EncodeRGB(&stdColor_cfRGB565, 255, 128, 0);

    TEST_ASSERT_EQUAL_HEX32(0x0000FC00u, rgb565);

    stdColor_DecodeRGB(rgb565, &stdColor_cfRGB565, &r, &g, &b);
    TEST_ASSERT_EQUAL_UINT8(255u, r);
    TEST_ASSERT_EQUAL_UINT8(130u, g);
    TEST_ASSERT_EQUAL_UINT8(0u, b);

    TEST_ASSERT_EQUAL_HEX32(0x00123456u, stdColor_EncodeRGB(&stdColor_cfRGB888, 0x12, 0x34, 0x56));
    TEST_ASSERT_EQUAL_HEX32(0x00563412u, stdColor_EncodeRGB(&stdColor_cfBGR888, 0x12, 0x34, 0x56));
}

TEST(stdColor, TestEncodeDecodeRGBA)
{
    uint8_t r;
    uint8_t g;
    uint8_t b;
    uint8_t a;
    uint32_t argb8888 = stdColor_EncodeRGBA(&stdColor_cfARGB8888, 0x12, 0x34, 0x56, 0x78);
    uint32_t argb4444 = stdColor_EncodeRGBA(&stdColor_cfARGB4444, 0xFF, 0x80, 0x00, 0x80);

    TEST_ASSERT_EQUAL_HEX32(0x78123456u, argb8888);
    stdColor_DecodeRGBA(argb8888, &stdColor_cfARGB8888, &r, &g, &b, &a);
    TEST_ASSERT_EQUAL_UINT8(0x12u, r);
    TEST_ASSERT_EQUAL_UINT8(0x34u, g);
    TEST_ASSERT_EQUAL_UINT8(0x56u, b);
    TEST_ASSERT_EQUAL_UINT8(0x78u, a);

    TEST_ASSERT_EQUAL_HEX32(0x8F80u, argb4444);
    stdColor_DecodeRGBA(argb4444, &stdColor_cfARGB4444, &r, &g, &b, &a);
    TEST_ASSERT_EQUAL_UINT8(255u, r);
    TEST_ASSERT_EQUAL_UINT8(136u, g);
    TEST_ASSERT_EQUAL_UINT8(0u, b);
    TEST_ASSERT_EQUAL_UINT8(136u, a);

    stdColor_DecodeRGBA(stdColor_EncodeRGB(&stdColor_cfRGB888, 1, 2, 3), &stdColor_cfRGB888, &r, &g, &b, &a);
    TEST_ASSERT_EQUAL_UINT8(255u, a);
}

TEST(stdColor, TestCalcColorBits)
{
    uint32_t cbpp = 0;
    int32_t posShift = 0;
    int32_t posShiftRight = 0;

    stdColor_CalcColorBits(0x0000F800u, &cbpp, &posShift, &posShiftRight);
    TEST_ASSERT_EQUAL_UINT32(5u, cbpp);
    TEST_ASSERT_EQUAL_INT32(11, posShift);
    TEST_ASSERT_EQUAL_INT32(3, posShiftRight);

    stdColor_CalcColorBits(0x000000FFu, &cbpp, &posShift, &posShiftRight);
    TEST_ASSERT_EQUAL_UINT32(8u, cbpp);
    TEST_ASSERT_EQUAL_INT32(0, posShift);
    TEST_ASSERT_EQUAL_INT32(0, posShiftRight);

    cbpp = 77u;
    stdColor_CalcColorBits(0xFFu, NULL, &posShift, &posShiftRight);
    TEST_ASSERT_EQUAL_UINT32(77u, cbpp);

    cbpp = 77u;
    posShift = 77;
    posShiftRight = 77;
    stdColor_CalcColorBits(0u, &cbpp, &posShift, &posShiftRight);
    TEST_ASSERT_EQUAL_UINT32(0u, cbpp);
    TEST_ASSERT_EQUAL_INT32(0, posShift);
    TEST_ASSERT_EQUAL_INT32(0, posShiftRight);
}

TEST(stdColor, TestColorConvertOnePixelAndRecode)
{
    uint32_t source = 0x78123456u;
    int converted = stdColor_ColorConvertOnePixel((ColorInfo*)&stdColor_cfRGB565, (int)source, (ColorInfo*)&stdColor_cfARGB8888, 0, NULL);
    uint32_t recoded = stdColor_Recode(source, &stdColor_cfARGB8888, &stdColor_cfRGB565);

    TEST_ASSERT_EQUAL_HEX32(recoded, (uint32_t)converted);
    TEST_ASSERT_EQUAL_HEX32(0x000011AAu, (uint32_t)converted);
}

TEST(stdColor, TestColorConvertOneRow16To24And24To16)
{
    uint16_t src565[2] = { 0xF800u, 0x07E0u };
    uint8_t dest888[6] = { 0 };
    uint8_t src888[6] = {
        0x00u, 0x00u, 0xFFu,
        0x00u, 0xFFu, 0x00u
    };
    uint16_t dest565[2] = { 0 };

    stdColor_ColorConvertOneRow(dest888, &stdColor_cfRGB888, (const uint8_t*)src565, &stdColor_cfRGB565, 2, 0, NULL);
    TEST_ASSERT_EQUAL_HEX8(0x00u, dest888[0]);
    TEST_ASSERT_EQUAL_HEX8(0x00u, dest888[1]);
    TEST_ASSERT_EQUAL_HEX8(0xFFu, dest888[2]);
    TEST_ASSERT_EQUAL_HEX8(0x00u, dest888[3]);
    TEST_ASSERT_EQUAL_HEX8(0xFFu, dest888[4]);
    TEST_ASSERT_EQUAL_HEX8(0x00u, dest888[5]);

    stdColor_ColorConvertOneRow((uint8_t*)dest565, &stdColor_cfRGB565, src888, &stdColor_cfRGB888, 2, 0, NULL);
    TEST_ASSERT_EQUAL_HEX16(0xF800u, dest565[0]);
    TEST_ASSERT_EQUAL_HEX16(0x07E0u, dest565[1]);
}

TEST(stdColor, TestColorConvertOneRow32WithAlpha)
{
    uint32_t src[2] = {
        STD_RGBA(0x12, 0x34, 0x56, 0x78),
        STD_RGBA(0xFF, 0x80, 0x00, 0x80)
    };
    uint32_t dest[2] = { 0 };

    stdColor_ColorConvertOneRow((uint8_t*)dest, &stdColor_cfABGR8888, (const uint8_t*)src, &stdColor_cfARGB8888, 2, 0, NULL);

    TEST_ASSERT_EQUAL_HEX32(0x78563412u, dest[0]);
    TEST_ASSERT_EQUAL_HEX32(0x800080FFu, dest[1]);
}

TEST(stdColor, TestColorConvertOneRowScalesOneBitAlpha)
{
    uint16_t src[2] = {
        0xFC00u,
        0x7C00u
    };
    uint32_t dest[2] = { 0 };

    stdColor_ColorConvertOneRow((uint8_t*)dest, &stdColor_cfARGB8888, (const uint8_t*)src, &stdColor_cfARGB5551, 2, 0, NULL);

    TEST_ASSERT_EQUAL_HEX32(0xFFFF0000u, dest[0]);
    TEST_ASSERT_EQUAL_HEX32(0x00FF0000u, dest[1]);
}

#ifdef J3D_COLORKEYSUPPORTED
TEST(stdColor, TestColorConvertOneRowAppliesDX6ColorKey)
{
    const uint16_t aSource[] = {
        0x7C00u,
        0xFC00u
    };
    uint16_t aDestination[STD_ARRAYLEN(aSource)] = { 0 };

    DDCOLORKEY colorKey = { 0 };
    colorKey.dwColorSpaceLowValue = 0x1234u;

    stdColor_ColorConvertOneRow(
        (uint8_t*)aDestination,
        &stdColor_cfRGB565,
        (const uint8_t*)aSource,
        &stdColor_cfARGB5551,
        (int)STD_ARRAYLEN(aSource),
        1,
        &colorKey
    );

    TEST_ASSERT_EQUAL_HEX16(0x1234u, aDestination[0]);
    TEST_ASSERT_EQUAL_HEX16(0xF800u, aDestination[1]);
}
#endif

TEST(stdColor, TestColorConvertOneRowRejectsUnsupportedDepths)
{
    ColorInfo sourceCI = stdColor_cfRGB888;
    ColorInfo destCI   = stdColor_cfRGB8888;
    uint32_t source    = 0x00123456u;
    uint32_t dest      = 0xA5A5A5A5u;

    sourceCI.bpp = 12;
    J3DTest_ResetAssertCapture();
    stdColor_ColorConvertOneRow((uint8_t*)&dest, &destCI, (const uint8_t*)&source, &sourceCI, 1, 0, NULL);
    TEST_ASSERT_EQUAL_INT(1, J3DTest_GetAssertCount());
    TEST_ASSERT_EQUAL_HEX32(0u, dest);

    sourceCI = stdColor_cfRGB888;
    destCI   = stdColor_cfRGB888;

    destCI.bpp = 12;
    dest       = 0xA5A5A5A5u;

    J3DTest_ResetAssertCapture();
    stdColor_ColorConvertOneRow((uint8_t*)&dest, &destCI, (const uint8_t*)&source, &sourceCI, 1, 0, NULL);
    TEST_ASSERT_EQUAL_INT(1, J3DTest_GetAssertCount());
    TEST_ASSERT_EQUAL_HEX32(0xA5A5A5A5u, dest);
}

TEST(stdColor, TestRGBConversionMatrix)
{
    static const ColorInfo* apFormats[] = {
        &stdColorTest_cfRGB332,
        &stdColor_cfRGB555,
        &stdColor_cfRGB565,
        &stdColor_cfRGB888,
        &stdColor_cfBGR888,
        &stdColor_cfRGB8888,
        &stdColor_cfBGR8888
    };

    for ( size_t srcIdx = 0; srcIdx < STD_ARRAYLEN(apFormats); ++srcIdx )
    {
        uint32_t source = stdColor_EncodeRGB(apFormats[srcIdx], 0xA5u, 0x5Au, 0xE3u);
        uint8_t sourceBytes[4] = { 0 };
        uint8_t r;
        uint8_t g;
        uint8_t b;
        stdColorTest_StorePixel(sourceBytes, apFormats[srcIdx]->bpp, source);
        stdColor_DecodeRGB(source, apFormats[srcIdx], &r, &g, &b);

        for ( size_t dstIdx = 0; dstIdx < STD_ARRAYLEN(apFormats); ++dstIdx )
        {
            uint8_t destBytes[4] = { 0 };
            uint32_t expected = stdColor_EncodeRGB(apFormats[dstIdx], r, g, b);

            stdColor_ColorConvertOneRow(destBytes, apFormats[dstIdx], sourceBytes, apFormats[srcIdx], 1, 0, NULL);
            TEST_ASSERT_EQUAL_HEX32(expected, stdColorTest_LoadPixel(destBytes, apFormats[dstIdx]->bpp));
        }
    }
}

TEST(stdColor, TestRGBAConversionMatrix)
{
    static const ColorInfo* apFormats[] = {
        &stdColor_cfARGB5551,
        &stdColor_cfARGB4444,
        &stdColor_cfARGB8888,
        &stdColor_cfABGR8888,
        &stdColor_cfRGBA8888
    };

    for ( size_t srcIdx = 0; srcIdx < STD_ARRAYLEN(apFormats); ++srcIdx )
    {
        uint32_t source = stdColor_EncodeRGBA(apFormats[srcIdx], 0xA5u, 0x5Au, 0xE3u, 0x78u);
        uint8_t sourceBytes[4] = { 0 };
        uint8_t r;
        uint8_t g;
        uint8_t b;
        uint8_t a;
        stdColorTest_StorePixel(sourceBytes, apFormats[srcIdx]->bpp, source);
        stdColor_DecodeRGBA(source, apFormats[srcIdx], &r, &g, &b, &a);

        for ( size_t dstIdx = 0; dstIdx < STD_ARRAYLEN(apFormats); ++dstIdx )
        {
            uint8_t destBytes[4] = { 0 };
            uint32_t expected = stdColor_EncodeRGBA(apFormats[dstIdx], r, g, b, a);

            stdColor_ColorConvertOneRow(destBytes, apFormats[dstIdx], sourceBytes, apFormats[srcIdx], 1, 0, NULL);
            TEST_ASSERT_EQUAL_HEX32(expected, stdColorTest_LoadPixel(destBytes, apFormats[dstIdx]->bpp));
        }
    }
}

TEST(stdColor, TestLowBitEncodeDecodeExhaustive)
{
    static const ColorInfo* apFormats[] = {
        &stdColorTest_cfRGB332,
        &stdColor_cfRGB555,
        &stdColor_cfRGB565,
        &stdColor_cfARGB4444,
        &stdColor_cfARGB5551
    };

    for ( size_t formatIdx = 0; formatIdx < STD_ARRAYLEN(apFormats); ++formatIdx )
    {
        for ( uint32_t value = 0; value <= 255u; ++value )
        {
            uint32_t encoded = stdColor_EncodeRGBA(apFormats[formatIdx], (uint8_t)value, (uint8_t)(255u - value), (uint8_t)(value ^ 0xA5u), (uint8_t)value);
            uint8_t r;
            uint8_t g;
            uint8_t b;
            uint8_t a;
            stdColor_DecodeRGBA(encoded, apFormats[formatIdx], &r, &g, &b, &a);
            TEST_ASSERT_EQUAL_HEX32(encoded, stdColor_EncodeRGBA(apFormats[formatIdx], r, g, b, a));
        }
    }
}

TEST(stdColor, TestZeroWidthConversionLeavesDestinationUntouched)
{
    uint32_t source = STD_RGBA(1u, 2u, 3u, 4u);
    uint32_t destination = 0xA5A5A5A5u;

    stdColor_ColorConvertOneRow((uint8_t*)&destination, &stdColor_cfARGB8888, (const uint8_t*)&source, &stdColor_cfARGB8888, 0, 0, NULL);
    TEST_ASSERT_EQUAL_HEX32(0xA5A5A5A5u, destination);

    stdColor_ColorConvertOneRow((uint8_t*)&destination, &stdColor_cfARGB8888, (const uint8_t*)&source, &stdColor_cfARGB8888, -1, 0, NULL);
    TEST_ASSERT_EQUAL_HEX32(0xA5A5A5A5u, destination);
}

TEST(stdColor, TestRoundTripRGBHSV)
{
    const uint8_t samples[][3] = {
        { 255u, 0u, 0u },
        { 10u, 200u, 30u },
        { 20u, 40u, 220u },
        { 128u, 128u, 128u }
    };

    for ( size_t i = 0; i < STD_ARRAYLEN(samples); ++i )
    {
        float h;
        float s;
        float v;
        float rf;
        float gf;
        float bf;

        stdColor_RGBtoHSV((float)samples[i][0], (float)samples[i][1], (float)samples[i][2], &h, &s, &v);
        stdColor_HSVtoRGB(h, s, v, &rf, &gf, &bf);
        stdColorTest_AssertRGBWithin((uint8_t)rf, (uint8_t)gf, (uint8_t)bf, samples[i][0], samples[i][1], samples[i][2], 1u);
    }
}

TEST_GROUP_RUNNER(stdColor)
{
    RUN_TEST_CASE(stdColor, TestPackedColorMacros);
    RUN_TEST_CASE(stdColor, TestRGBtoHSV);
    RUN_TEST_CASE(stdColor, TestHSVtoRGB);
    RUN_TEST_CASE(stdColor, TestScaleColorComponent);
    RUN_TEST_CASE(stdColor, TestRGB565UpscaleMatchesPreviousFixExhaustive);
    RUN_TEST_CASE(stdColor, TestARGB16UpscaleMatchesPreviousFixExhaustive);
    RUN_TEST_CASE(stdColor, TestEncodeDecodeRGB);
    RUN_TEST_CASE(stdColor, TestEncodeDecodeRGBA);
    RUN_TEST_CASE(stdColor, TestCalcColorBits);
    RUN_TEST_CASE(stdColor, TestColorConvertOnePixelAndRecode);
    RUN_TEST_CASE(stdColor, TestColorConvertOneRow16To24And24To16);
    RUN_TEST_CASE(stdColor, TestColorConvertOneRow32WithAlpha);
    RUN_TEST_CASE(stdColor, TestColorConvertOneRowScalesOneBitAlpha);
#ifdef J3D_COLORKEYSUPPORTED
    RUN_TEST_CASE(stdColor, TestColorConvertOneRowAppliesDX6ColorKey);
#endif
    RUN_TEST_CASE(stdColor, TestColorConvertOneRowRejectsUnsupportedDepths);
    RUN_TEST_CASE(stdColor, TestRGBConversionMatrix);
    RUN_TEST_CASE(stdColor, TestRGBAConversionMatrix);
    RUN_TEST_CASE(stdColor, TestLowBitEncodeDecodeExhaustive);
    RUN_TEST_CASE(stdColor, TestZeroWidthConversionLeavesDestinationUntouched);
    RUN_TEST_CASE(stdColor, TestRoundTripRGBHSV);
}

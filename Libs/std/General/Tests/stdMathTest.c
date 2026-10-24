#include <unity_fixture.h>

#include <float.h>
#include <limits.h>
#include <stdint.h>
#include <string.h>

#include <std/General/std.h>
#include <std/General/stdMath.h>
#include <std/General/stdUtil.h>
#include <j3dcore/Tests/j3dTest.h>

#include "stdGeneralTest.h"

extern float stdMath_aSinTable[4096];
extern float stdMath_aTanTable[4096];

#define STDMATH_TEST_SIN_TABLE_SIZE       STD_ARRAYLEN(stdMath_aSinTable)
#define STDMATH_TEST_SIN_TABLE_LAST_IDX   (STDMATH_TEST_SIN_TABLE_SIZE - 1)
#define STDMATH_TEST_SIN_FULL_CYCLE_SIZE  (4 * STDMATH_TEST_SIN_TABLE_SIZE)
#define STDMATH_TEST_SIN_INDEX_SCALE      ((float)STDMATH_TEST_SIN_TABLE_SIZE / 90.0f)
#define STDMATH_TEST_TAN_TABLE_SIZE       STD_ARRAYLEN(stdMath_aTanTable)
#define STDMATH_TEST_TAN_TABLE_LAST_IDX   (STDMATH_TEST_TAN_TABLE_SIZE - 1)
#define STDMATH_TEST_TAN_FULL_CYCLE_SIZE  (4 * STDMATH_TEST_TAN_TABLE_SIZE)
#define STDMATH_TEST_TAN_INDEX_COUNT      ((float)STDMATH_TEST_TAN_FULL_CYCLE_SIZE)

TEST_GROUP(stdMath);

TEST_SETUP(stdMath)
{
    StdGeneralTest_Startup();
}

TEST_TEAR_DOWN(stdMath)
{
    StdGeneralTest_Shutdown();
}

// A separate traversal checks quadrant boundaries and interpolation independently.
static void stdMathTest_ReferenceSinCos(float angle, float* pSinOut, float* pCosOut)
{
    float normalized;
    float v4;
    float v5;
    float v6;
    float a1;
    int32_t v8;
    float v9;
    float v10;
    float v11;
    float v12;
    float v13;
    int32_t quantized;
    int32_t quantized_plus1;
    float normalized_;
    float v17;
    float v18;
    float v19;
    float v20;
    float v21;
    float v22;
    float v23;
    float v24;

    normalized = stdMath_NormalizeAngle(angle);
    normalized_ = normalized;
    if ( normalized >= 90.0 )
    {
        if ( normalized_ >= 180.0 )
        {
            if ( normalized_ >= 270.0 )
                v8 = 3;
            else
                v8 = 2;
        }
        else
        {
            v8 = 1;
        }
    }
    else
    {
        v8 = 0;
    }

    a1 = normalized_ * STDMATH_TEST_SIN_INDEX_SCALE;
    v6 = a1 - floorf(a1);
    quantized = (int32_t)a1;
    quantized_plus1 = quantized + 1;

    switch ( v8 )
    {
        case 0:
            if ( quantized_plus1 < STDMATH_TEST_SIN_TABLE_SIZE )
                v17 = stdMath_aSinTable[quantized_plus1];
            else
                v17 = stdMath_aSinTable[STDMATH_TEST_SIN_TABLE_LAST_IDX - (quantized - STDMATH_TEST_SIN_TABLE_LAST_IDX)];
            *pSinOut = (v17 - stdMath_aSinTable[quantized]) * v6 + stdMath_aSinTable[quantized];
            if ( quantized_plus1 < STDMATH_TEST_SIN_TABLE_SIZE )
                v18 = stdMath_aSinTable[STDMATH_TEST_SIN_TABLE_LAST_IDX - quantized_plus1];
            else
                v18 = -stdMath_aSinTable[quantized_plus1 - STDMATH_TEST_SIN_TABLE_SIZE];
            *pCosOut = (v18 - stdMath_aSinTable[STDMATH_TEST_SIN_TABLE_LAST_IDX - quantized]) * v6
                + stdMath_aSinTable[STDMATH_TEST_SIN_TABLE_LAST_IDX - quantized];
            break;

        case 1:
            if ( quantized_plus1 < 2 * STDMATH_TEST_SIN_TABLE_SIZE )
                v19 = stdMath_aSinTable[STDMATH_TEST_SIN_TABLE_LAST_IDX - (quantized - STDMATH_TEST_SIN_TABLE_LAST_IDX)];
            else
                v19 = -stdMath_aSinTable[quantized_plus1 - 2 * STDMATH_TEST_SIN_TABLE_SIZE];
            v9 = stdMath_aSinTable[STDMATH_TEST_SIN_TABLE_LAST_IDX - (quantized - STDMATH_TEST_SIN_TABLE_SIZE)];
            *pSinOut = (v19 - v9) * v6 + v9;
            if ( quantized_plus1 < 2 * STDMATH_TEST_SIN_TABLE_SIZE )
                v4 = -stdMath_aSinTable[quantized_plus1 - STDMATH_TEST_SIN_TABLE_SIZE];
            else
                v4 = -stdMath_aSinTable[STDMATH_TEST_SIN_TABLE_LAST_IDX - (quantized - (2 * STDMATH_TEST_SIN_TABLE_SIZE - 1))];
            v20 = v4;
            v10 = -stdMath_aSinTable[quantized - STDMATH_TEST_SIN_TABLE_SIZE];
            *pCosOut = (v20 - v10) * v6 + v10;
            break;

        case 2:
            if ( quantized_plus1 < 3 * STDMATH_TEST_SIN_TABLE_SIZE )
                v5 = -stdMath_aSinTable[quantized_plus1 - 2 * STDMATH_TEST_SIN_TABLE_SIZE];
            else
                v5 = -stdMath_aSinTable[STDMATH_TEST_SIN_TABLE_LAST_IDX - (quantized - (3 * STDMATH_TEST_SIN_TABLE_SIZE - 1))];
            v21 = v5;
            v11 = -stdMath_aSinTable[quantized - 2 * STDMATH_TEST_SIN_TABLE_SIZE];
            *pSinOut = (v21 - v11) * v6 + v11;
            if ( quantized_plus1 < 3 * STDMATH_TEST_SIN_TABLE_SIZE )
                v22 = -stdMath_aSinTable[STDMATH_TEST_SIN_TABLE_LAST_IDX - (quantized - (2 * STDMATH_TEST_SIN_TABLE_SIZE - 1))];
            else
                v22 = stdMath_aSinTable[quantized_plus1 - 3 * STDMATH_TEST_SIN_TABLE_SIZE];
            v12 = -stdMath_aSinTable[STDMATH_TEST_SIN_TABLE_LAST_IDX - (quantized - 2 * STDMATH_TEST_SIN_TABLE_SIZE)];
            *pCosOut = (v22 - v12) * v6 + v12;
            break;

        case 3:
            if ( quantized_plus1 < STDMATH_TEST_SIN_FULL_CYCLE_SIZE )
                v23 = -stdMath_aSinTable[STDMATH_TEST_SIN_TABLE_LAST_IDX - (quantized - (3 * STDMATH_TEST_SIN_TABLE_SIZE - 1))];
            else
                v23 = stdMath_aSinTable[quantized_plus1 - STDMATH_TEST_SIN_FULL_CYCLE_SIZE];
            v13 = -stdMath_aSinTable[STDMATH_TEST_SIN_TABLE_LAST_IDX - (quantized - 3 * STDMATH_TEST_SIN_TABLE_SIZE)];
            *pSinOut = (v23 - v13) * v6 + v13;
            if ( quantized_plus1 < STDMATH_TEST_SIN_FULL_CYCLE_SIZE )
                v24 = stdMath_aSinTable[quantized_plus1 - 3 * STDMATH_TEST_SIN_TABLE_SIZE];
            else
                v24 = stdMath_aSinTable[STDMATH_TEST_SIN_TABLE_LAST_IDX - (quantized - (STDMATH_TEST_SIN_FULL_CYCLE_SIZE - 1))];
            *pCosOut = (v24 - stdMath_aSinTable[quantized - 3 * STDMATH_TEST_SIN_TABLE_SIZE]) * v6
                + stdMath_aSinTable[quantized - 3 * STDMATH_TEST_SIN_TABLE_SIZE];
            break;

        default:
            break;
    }
}

// The third quadrant uses the same forward samples as the first quadrant.
static float stdMathTest_ReferenceTan(float angle)
{
    double v1;
    float v3;
    float a1a;
    int32_t v5;
    float v6;
    float v7;
    int32_t v8;
    float v9;
    int32_t v10;
    float v11;
    float v12;
    float v13;
    float v14;
    float v15;

    v1 = stdMath_NormalizeAngle(angle);
    v15 = (float)v1;
    if ( v1 >= 90.0 )
    {
        if ( v15 >= 180.0 )
        {
            if ( v15 >= 270.0 )
                v5 = 3;
            else
                v5 = 2;
        }
        else
        {
            v5 = 1;
        }
    }
    else
    {
        v5 = 0;
    }

    a1a = (float)(v15 / 360.0 * STDMATH_TEST_TAN_INDEX_COUNT);
    v3 = a1a - floorf(a1a);
    v8 = (int32_t)a1a;
    v10 = v8 + 1;

    switch ( v5 )
    {
        case 0:
            if ( v10 < STDMATH_TEST_TAN_TABLE_SIZE )
                v11 = stdMath_aTanTable[v10];
            else
                v11 = -stdMath_aTanTable[STDMATH_TEST_TAN_TABLE_LAST_IDX - (v8 - STDMATH_TEST_TAN_TABLE_LAST_IDX)];
            v9 = (v11 - stdMath_aTanTable[v8]) * v3 + stdMath_aTanTable[v8];
            break;

        case 1:
            if ( v10 < 2 * STDMATH_TEST_TAN_TABLE_SIZE )
                v12 = -stdMath_aTanTable[STDMATH_TEST_TAN_TABLE_LAST_IDX - (v8 - STDMATH_TEST_TAN_TABLE_LAST_IDX)];
            else
                v12 = stdMath_aTanTable[v10 - 2 * STDMATH_TEST_TAN_TABLE_SIZE];
            v6 = -stdMath_aTanTable[STDMATH_TEST_TAN_TABLE_LAST_IDX - (v8 - STDMATH_TEST_TAN_TABLE_SIZE)];
            v9 = (v12 - v6) * v3 + v6;
            break;

        case 2:
            if ( v10 < 3 * STDMATH_TEST_TAN_TABLE_SIZE )
                v13 = stdMath_aTanTable[v10 - 2 * STDMATH_TEST_TAN_TABLE_SIZE];
            else
                v13 = -stdMath_aTanTable[STDMATH_TEST_TAN_TABLE_LAST_IDX - (v8 - (3 * STDMATH_TEST_TAN_TABLE_SIZE - 1))];
            v9 = (v13 - stdMath_aTanTable[v8 - 2 * STDMATH_TEST_TAN_TABLE_SIZE]) * v3
                + stdMath_aTanTable[v8 - 2 * STDMATH_TEST_TAN_TABLE_SIZE];
            break;

        case 3:
            if ( v10 < STDMATH_TEST_TAN_FULL_CYCLE_SIZE )
                v14 = -stdMath_aTanTable[STDMATH_TEST_TAN_TABLE_LAST_IDX - (v8 - (3 * STDMATH_TEST_TAN_TABLE_SIZE - 1))];
            else
                v14 = stdMath_aTanTable[v10 - STDMATH_TEST_TAN_FULL_CYCLE_SIZE];
            v7 = -stdMath_aTanTable[STDMATH_TEST_TAN_TABLE_LAST_IDX - (v8 - 3 * STDMATH_TEST_TAN_TABLE_SIZE)];
            v9 = (v14 - v7) * v3 + v7;
            break;

        default:
            v9 = 0.0f;
            break;
    }

    return v9;
}

static float stdMathTest_SinAngleForIndex(float index)
{
    return index / STDMATH_TEST_SIN_INDEX_SCALE;
}

static float stdMathTest_TanAngleForIndex(float index)
{
    return index * 360.0f / STDMATH_TEST_TAN_INDEX_COUNT;
}

static uint32_t stdMathTest_FloatBits(float value)
{
    uint32_t bits;

    memcpy(&bits, &value, sizeof(bits));
    return bits;
}

static float stdMathTest_FloatFromBits(uint32_t bits)
{
    float value;

    memcpy(&value, &bits, sizeof(value));
    return value;
}

static void stdMathTest_AssertIsNaN(float value)
{
    TEST_ASSERT_TRUE(isnan(value));
}

static void stdMathTest_AssertIsInf(float value)
{
    TEST_ASSERT_TRUE(isinf(value));
}

static void stdMathTest_AssertFloatBitsEqual(float expected, float actual)
{
    TEST_ASSERT_EQUAL_HEX32(stdMathTest_FloatBits(expected), stdMathTest_FloatBits(actual));
}

static void stdMathTest_AssertLookupTable(const char* pRelativePath, const float* pTable)
{
    uint8_t aData[4096 * sizeof(uint32_t)];
    tFileHandle fh = stdFileOpen(StdGeneralTest_GetTestVectorPath(pRelativePath), "rb");
    TEST_ASSERT_NOT_EQUAL(0u, fh);

    uint8_t extra;
    size_t numRead   = stdFileRead(fh, aData, sizeof(aData));
    size_t extraRead = stdFileRead(fh, &extra, sizeof(extra));
    int closeResult  = stdFileClose(fh);
    TEST_ASSERT_EQUAL_size_t(sizeof(aData), numRead);
    TEST_ASSERT_EQUAL_size_t(0, extraRead);
    TEST_ASSERT_EQUAL_INT(0, closeResult);

    for ( size_t i = 0; i < STD_ARRAYLEN(aData) / sizeof(uint32_t); ++i )
    {
        const uint8_t* pValue = &aData[i * sizeof(uint32_t)];
        uint32_t expected    = (uint32_t)pValue[0] | (uint32_t)pValue[1] << 8
                             | (uint32_t)pValue[2] << 16 | (uint32_t)pValue[3] << 24;
        TEST_ASSERT_EQUAL_HEX32_MESSAGE(expected, stdMathTest_FloatBits(pTable[i]), pRelativePath);
    }
}

TEST(stdMath, TestLookupTables)
{
    // Compare all entries, not rounded decimal approximations or a few landmarks.
    stdMathTest_AssertLookupTable("stdMath/sin_table.bin", stdMath_aSinTable);
    stdMathTest_AssertLookupTable("stdMath/tan_table.bin", stdMath_aTanTable);
}

static void stdMathTest_AssertSinCosMatchesReference(float angle)
{
    float expectedSin;
    float expectedCos;
    float actualSin;
    float actualCos;

    stdMathTest_ReferenceSinCos(angle, &expectedSin, &expectedCos);
    stdMath_SinCos(angle, &actualSin, &actualCos);

    stdMathTest_AssertFloatBitsEqual(expectedSin, actualSin);
    stdMathTest_AssertFloatBitsEqual(expectedCos, actualCos);
}

static void stdMathTest_AssertTanMatchesReference(float angle)
{
    stdMathTest_AssertFloatBitsEqual(stdMathTest_ReferenceTan(angle), stdMath_Tan(angle));
}

static void stdMathTest_AssertDist3D(float expected, float x, float y, float z)
{
    TEST_ASSERT_FLOAT_WITHIN(0.00001f, expected, stdMath_Dist3D1(x, y, z));
}

static void stdMathTest_AssertDoubleWithin(double tolerance, double expected, double actual)
{
    TEST_ASSERT_TRUE(fabs(actual - expected) <= tolerance);
}

TEST(stdMath, TestConstantAndHelperMacros)
{
    TEST_ASSERT_FLOAT_WITHIN(0.0f, (float)M_PI, (float)STDMATH_PI);
    TEST_ASSERT_FLOAT_WITHIN(0.0f, (float)M_PI, STDMATH_PI_F);
    TEST_ASSERT_FLOAT_WITHIN(0.00001f, 1.5707964f, STDMATH_RADIANSF(90.0f));
    TEST_ASSERT_FLOAT_WITHIN(0.00001f, 180.0f, STDMATH_TODEGREESF(STDMATH_PI_F));
    TEST_ASSERT_FLOAT_WITHIN(0.0f, 0.00001f, STDMATH_ZERO_EPSILON);
    TEST_ASSERT_EQUAL_FLOAT(5.0f, STDMATH_CLAMP(7.0f, -2.0f, 5.0f));
    TEST_ASSERT_EQUAL_FLOAT(-2.0f, STDMATH_CLAMP(-7.0f, -2.0f, 5.0f));
    TEST_ASSERT_EQUAL_FLOAT(3.0f, STDMATH_CLAMP(3.0f, -2.0f, 5.0f));
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, 18.8495407f, STDMATH_CIRCLE_CIRCUMF(3.0f));
}

TEST(stdMath, TestDoubleConversionMacros)
{
    const double angles[] = {
        0.0,
        1.0,
        -1.0,
        45.0,
        -45.0,
        90.0,
        180.0,
        360.0,
        720.0,
        123.456789012345,
        -987.654321098765
    };

    stdMathTest_AssertDoubleWithin(1.0e-15, 0.0, STDMATH_RADIANS(0.0));
    stdMathTest_AssertDoubleWithin(1.0e-15, 0.017453292519943295, STDMATH_RADIANS(1.0));
    stdMathTest_AssertDoubleWithin(1.0e-15, 0.78539816339744831, STDMATH_RADIANS(45.0));
    stdMathTest_AssertDoubleWithin(1.0e-15, 1.5707963267948966, STDMATH_RADIANS(90.0));
    stdMathTest_AssertDoubleWithin(1.0e-15, 3.1415926535897931, STDMATH_RADIANS(180.0));
    stdMathTest_AssertDoubleWithin(1.0e-14, 12.566370614359172, STDMATH_RADIANS(720.0));

    stdMathTest_AssertDoubleWithin(1.0e-12, 0.0, STDMATH_TODEGREES(0.0));
    stdMathTest_AssertDoubleWithin(1.0e-12, 45.0, STDMATH_TODEGREES(0.78539816339744831));
    stdMathTest_AssertDoubleWithin(1.0e-12, 90.0, STDMATH_TODEGREES(1.5707963267948966));
    stdMathTest_AssertDoubleWithin(1.0e-12, 180.0, STDMATH_TODEGREES(3.1415926535897931));
    stdMathTest_AssertDoubleWithin(1.0e-12, 360.0, STDMATH_TODEGREES(6.2831853071795862));

    for ( size_t i = 0; i < STD_ARRAYLEN(angles); ++i )
    {
        double radians = STDMATH_RADIANS(angles[i]);
        double degrees = STDMATH_TODEGREES(radians);

        stdMathTest_AssertDoubleWithin(1.0e-12, angles[i], degrees);
    }
}

TEST(stdMath, TestClipNearZero)
{
    TEST_ASSERT_EQUAL_FLOAT(0.0f, stdMath_ClipNearZero(0.0f));
    TEST_ASSERT_EQUAL_FLOAT(0.0f, stdMath_ClipNearZero(STDMATH_ZERO_EPSILON));
    TEST_ASSERT_EQUAL_FLOAT(0.0f, stdMath_ClipNearZero(-STDMATH_ZERO_EPSILON));
    TEST_ASSERT_EQUAL_FLOAT(2.0f * STDMATH_ZERO_EPSILON, stdMath_ClipNearZero(2.0f * STDMATH_ZERO_EPSILON));
    TEST_ASSERT_EQUAL_FLOAT(-2.0f * STDMATH_ZERO_EPSILON, stdMath_ClipNearZero(-2.0f * STDMATH_ZERO_EPSILON));
}

TEST(stdMath, TestExpm1f)
{
    TEST_ASSERT_FLOAT_WITHIN(0.000001f, expf(0.000001f) - 1.0f, stdMath_Expm1f(0.000001f));
    TEST_ASSERT_FLOAT_WITHIN(0.000001f, expf(-0.000001f) - 1.0f, stdMath_Expm1f(-0.000001f));
    TEST_ASSERT_FLOAT_WITHIN(0.000001f, expf(0.25f) - 1.0f, stdMath_Expm1f(0.25f));
    TEST_ASSERT_FLOAT_WITHIN(0.000001f, expf(-0.25f) - 1.0f, stdMath_Expm1f(-0.25f));
}

TEST(stdMath, TestFlexPower)
{
    TEST_ASSERT_FLOAT_WITHIN(0.00001f, 1.0f, stdMath_FlexPower(5.0f, 0));
    TEST_ASSERT_FLOAT_WITHIN(0.00001f, 8.0f, stdMath_FlexPower(2.0f, 3));
    TEST_ASSERT_FLOAT_WITHIN(0.00001f, 0.25f, stdMath_FlexPower(2.0f, -2));
    TEST_ASSERT_FLOAT_WITHIN(0.00001f, -8.0f, stdMath_FlexPower(-2.0f, 3));
    TEST_ASSERT_FLOAT_WITHIN(0.00001f, 0.0625f, stdMath_FlexPower(-2.0f, -4));
}

TEST(stdMath, TestFlexPowerZeroNegativeExponent)
{
    stdMathTest_AssertIsInf(stdMath_FlexPower(0.0f, -1));
}

TEST(stdMath, TestNegativePowers)
{
    // Reciprocating an already-overflowed positive power loses valid subnormals.
    const struct
    {
        float base;
        int exponent;
        uint32_t expected;
    } aCases[] = {
        { 2.0f, -128, 0x00200000u },
        { 2.0f, -127, 0x00400000u },
        { 2.0f, -149, 0x00000001u },
        { 2.0f, -150, 0x00000000u },
        { -2.0f, -149, 0x80000001u },
        { -2.0f, -151, 0x80000000u },
        { 0.5f, -127, 0x7F000000u },
        { -0.5f, -127, 0xFF000000u },
        { 0.5f, -128, 0x7F800000u },
        { 0.0f, -1, 0x7F800000u },
        { -0.0f, -1, 0xFF800000u },
        { -0.0f, -2, 0x7F800000u },
        { -INFINITY, -1, 0x80000000u },
        { INFINITY, -2, 0x00000000u },
        { 1.0f, INT_MIN, 0x3F800000u },
        { -1.0f, INT_MIN, 0x3F800000u },
        { -1.0f, -INT_MAX, 0xBF800000u },
        { 2.0f, INT_MIN, 0x00000000u },
    };
    for ( size_t i = 0; i < STD_ARRAYLEN(aCases); ++i )
    {
        TEST_ASSERT_EQUAL_HEX32(aCases[i].expected, stdMathTest_FloatBits(stdMath_FlexPower(aCases[i].base, aCases[i].exponent)));
    }
    stdMathTest_AssertIsNaN(stdMath_FlexPower(NAN, -3));
}

TEST(stdMath, TestNormalizeAngle)
{
    TEST_ASSERT_FLOAT_WITHIN(0.00001f, 0.0f, stdMath_NormalizeAngle(0.0f));
    TEST_ASSERT_FLOAT_WITHIN(0.00001f, 0.0f, stdMath_NormalizeAngle(360.0f));
    TEST_ASSERT_FLOAT_WITHIN(0.00001f, 90.0f, stdMath_NormalizeAngle(450.0f));
    TEST_ASSERT_FLOAT_WITHIN(0.00001f, 270.0f, stdMath_NormalizeAngle(-90.0f));
    TEST_ASSERT_FLOAT_WITHIN(0.00001f, 0.0f, stdMath_NormalizeAngle(-360.0f));
    TEST_ASSERT_FLOAT_WITHIN(0.00001f, 105.0f, stdMath_NormalizeAngle(12345.0f));
    TEST_ASSERT_FLOAT_WITHIN(0.00001f, 255.0f, stdMath_NormalizeAngle(-12345.0f));
}

TEST(stdMath, TestNormalizeAngleNonFinite)
{
    stdMathTest_AssertIsNaN(stdMath_NormalizeAngle(NAN));
    stdMathTest_AssertIsNaN(stdMath_NormalizeAngle(INFINITY));
    stdMathTest_AssertIsNaN(stdMath_NormalizeAngle(-INFINITY));
}

TEST(stdMath, TestAngleBounds)
{
    // Large finite values must not escape the range used for table indexing.
    const struct
    {
        uint32_t input;
        float expected;
    } aCases[] = {
        { 0x5CB5F66Cu, 320.0f },
        { 0xF8F309E5u, 352.0f },
        { 0x7F7FFFFFu, 0.0f },
        { 0xFF7FFFFFu, 0.0f },
    };

    for ( size_t i = 0; i < STD_ARRAYLEN(aCases); ++i )
    {
        float input      = stdMathTest_FloatFromBits(aCases[i].input);
        float normalized = stdMath_NormalizeAngle(input);
        stdMathTest_AssertFloatBitsEqual(aCases[i].expected, normalized);
        TEST_ASSERT_EQUAL_INT(0, J3DTest_GetAssertCount());

        float expectedSin;
        float expectedCos;
        float actualSin;
        float actualCos;
        stdMath_SinCos(aCases[i].expected, &expectedSin, &expectedCos);
        stdMath_SinCos(input, &actualSin, &actualCos);
        stdMathTest_AssertFloatBitsEqual(expectedSin, actualSin);
        stdMathTest_AssertFloatBitsEqual(expectedCos, actualCos);
        stdMathTest_AssertFloatBitsEqual(stdMath_Tan(aCases[i].expected), stdMath_Tan(input));
    }

    // Probe exponent/sign combinations without ever supplying a non-finite angle.
    for ( uint32_t i = 0; i < 65536u; ++i )
    {
        uint32_t bits = i * 0x9E3779B9u;
        if ( (bits & 0x7F800000u) == 0x7F800000u )
        {
            continue;
        }

        float normalized = stdMath_NormalizeAngle(stdMathTest_FloatFromBits(bits));
        TEST_ASSERT_TRUE(isfinite(normalized));
        TEST_ASSERT_TRUE(normalized >= 0.0f && normalized < 360.0f);
        stdMathTest_AssertFloatBitsEqual(normalized, stdMath_NormalizeAngle(normalized));
    }
    TEST_ASSERT_EQUAL_INT(0, J3DTest_GetAssertCount());
}

TEST(stdMath, TestAngleBoundaries)
{
    const float aCenters[] = { -720.0f, -360.0f, -270.0f, -180.0f, -90.0f, 0.0f, 90.0f, 180.0f, 270.0f, 360.0f, 720.0f };
    for ( size_t i = 0; i < STD_ARRAYLEN(aCenters); ++i )
    {
        const float aInputs[] = { nextafterf(aCenters[i], -INFINITY), aCenters[i], nextafterf(aCenters[i], INFINITY) };
        for ( size_t j = 0; j < STD_ARRAYLEN(aInputs); ++j )
        {
            float normalized = stdMath_NormalizeAngle(aInputs[j]);
            TEST_ASSERT_TRUE(normalized >= 0.0f && normalized < 360.0f);
            stdMathTest_AssertSinCosMatchesReference(aInputs[j]);
        }
    }
    stdMathTest_AssertFloatBitsEqual(-0.0f, stdMath_NormalizeAngle(-0.0f));
    stdMathTest_AssertFloatBitsEqual(0.0f, stdMath_NormalizeAngle(-360.0f));
}

TEST(stdMath, TestNormalizeAngleAcute)
{
    TEST_ASSERT_FLOAT_WITHIN(0.00001f, 0.0f, stdMath_NormalizeAngleAcute(360.0f));
    TEST_ASSERT_FLOAT_WITHIN(0.00001f, 179.0f, stdMath_NormalizeAngleAcute(179.0f));
    TEST_ASSERT_FLOAT_WITHIN(0.00001f, 180.0f, stdMath_NormalizeAngleAcute(180.0f));
    TEST_ASSERT_FLOAT_WITHIN(0.00001f, -179.0f, stdMath_NormalizeAngleAcute(181.0f));
    TEST_ASSERT_FLOAT_WITHIN(0.00001f, -90.0f, stdMath_NormalizeAngleAcute(270.0f));
    TEST_ASSERT_FLOAT_WITHIN(0.00001f, 90.0f, stdMath_NormalizeAngleAcute(-270.0f));
    stdMathTest_AssertIsNaN(stdMath_NormalizeAngleAcute(NAN));
}

TEST(stdMath, TestSinCosTableSweep)
{
    // NOTE: Covers every quantized sin/cos table interval and interpolation branch.
    const float fractions[] = { 0.0f, 0.25f, 0.5f, 0.75f };
    const float wrappedAngles[] = {
        360.0f,
        -0.25f,
        -90.0f,
        -180.0f,
        -270.0f,
        -360.0f,
        720.0f,
        -720.0f,
        12345.67f,
        -12345.67f
    };

    for ( int32_t index = 0; index < STDMATH_TEST_SIN_FULL_CYCLE_SIZE; ++index )
    {
        for ( size_t i = 0; i < STD_ARRAYLEN(fractions); ++i )
        {
            float angle = stdMathTest_SinAngleForIndex((float)index + fractions[i]);

            stdMathTest_AssertSinCosMatchesReference(angle);
        }
    }

    for ( size_t i = 0; i < STD_ARRAYLEN(wrappedAngles); ++i )
    {
        stdMathTest_AssertSinCosMatchesReference(wrappedAngles[i]);
    }
}

TEST(stdMath, TestSinCosHalfDegreeSweep)
{
    // NOTE: Degree-domain sweep catches wrapping regressions missed by raw table indices.
    for ( int32_t halfDegrees = -7200; halfDegrees <= 7200; ++halfDegrees )
    {
        float angle = (float)halfDegrees * 0.5f;

        stdMathTest_AssertSinCosMatchesReference(angle);
    }
}

TEST(stdMath, TestSinCosStdLibApprox)
{
    const float angles[] = {
        -720.0f,
        -450.0f,
        -360.0f,
        -270.0f,
        -180.0f,
        -90.0f,
        -45.0f,
        0.0f,
        30.0f,
        45.0f,
        60.0f,
        90.0f,
        135.0f,
        180.0f,
        225.0f,
        270.0f,
        315.0f,
        360.0f,
        450.0f,
        720.0f
    };

    for ( size_t i = 0; i < STD_ARRAYLEN(angles); ++i )
    {
        float actualSin;
        float actualCos;
        float radians = STDMATH_RADIANSF(angles[i]);

        stdMath_SinCos(angles[i], &actualSin, &actualCos);

        TEST_ASSERT_FLOAT_WITHIN(0.001f, sinf(radians), actualSin);
        TEST_ASSERT_FLOAT_WITHIN(0.001f, cosf(radians), actualCos);
    }
}

TEST(stdMath, TestSinCosNonFinite)
{
    float sinOut = 1.0f;
    float cosOut = 1.0f;

    stdMath_SinCos(NAN, &sinOut, &cosOut);
    stdMathTest_AssertIsNaN(sinOut);
    stdMathTest_AssertIsNaN(cosOut);

    stdMath_SinCos(INFINITY, &sinOut, &cosOut);
    stdMathTest_AssertIsNaN(sinOut);
    stdMathTest_AssertIsNaN(cosOut);

    stdMath_SinCos(-INFINITY, &sinOut, &cosOut);
    stdMathTest_AssertIsNaN(sinOut);
    stdMathTest_AssertIsNaN(cosOut);
}

#ifdef J3D_RUNTIME_GUARDS
TEST(stdMath, TestSinCosNullOutputGuard)
{
    float sinOut = 7.0f;
    float cosOut = 11.0f;

    stdMath_SinCos(45.0f, NULL, &cosOut);
    TEST_ASSERT_EQUAL_FLOAT(11.0f, cosOut);

    stdMath_SinCos(45.0f, &sinOut, NULL);
    TEST_ASSERT_EQUAL_FLOAT(7.0f, sinOut);
}
#endif

TEST(stdMath, TestTanTableSweep)
{
    // Cover every table interval, including forward interpolation in the third quadrant.
    const float fractions[] = { 0.0f, 0.25f, 0.5f, 0.75f };
    const float wrappedAngles[] = {
        360.0f,
        -0.25f,
        -90.0f,
        -180.0f,
        -270.0f,
        -360.0f,
        720.0f,
        -720.0f,
        12345.67f,
        -12345.67f
    };

    for ( int32_t index = 0; index < (int32_t)STDMATH_TEST_TAN_INDEX_COUNT; ++index )
    {
        for ( size_t i = 0; i < STD_ARRAYLEN(fractions); ++i )
        {
            float angle = stdMathTest_TanAngleForIndex((float)index + fractions[i]);

            stdMathTest_AssertTanMatchesReference(angle);
        }
    }

    for ( size_t i = 0; i < STD_ARRAYLEN(wrappedAngles); ++i )
    {
        stdMathTest_AssertTanMatchesReference(wrappedAngles[i]);
    }
}

TEST(stdMath, TestTanHalfDegreeSweep)
{
    // NOTE: Degree-domain sweep mirrors common caller inputs across ten full turns.
    for ( int32_t halfDegrees = -7200; halfDegrees <= 7200; ++halfDegrees )
    {
        float angle = (float)halfDegrees * 0.5f;

        stdMathTest_AssertTanMatchesReference(angle);
    }
}

TEST(stdMath, TestTanStdLibApprox)
{
    const float angles[] = {
        -720.0f,
        -405.0f,
        -360.0f,
        -225.0f,
        -45.0f,
        0.0f,
        15.0f,
        30.0f,
        45.0f,
        60.0f,
        120.0f,
        135.0f,
        180.0f,
        180.5f,
        200.0f,
        225.0f,
        260.0f,
        300.0f,
        315.0f,
        360.0f,
        720.0f
    };

    for ( size_t i = 0; i < STD_ARRAYLEN(angles); ++i )
    {
        float expected = tanf(STDMATH_RADIANSF(angles[i]));
        float actual = stdMath_Tan(angles[i]);

        TEST_ASSERT_FLOAT_WITHIN(0.01f, expected, actual);
    }
}

TEST(stdMath, TestTanQuadrants)
{
    // A reversed base sample returns an asymptote at 180 instead of zero.
    const float aAngles[] = { -180.0f, 0.0f, 180.0f, 360.0f, 540.0f };
    for ( size_t i = 0; i < STD_ARRAYLEN(aAngles); ++i )
    {
        stdMathTest_AssertFloatBitsEqual(0.0f, stdMath_Tan(aAngles[i]));
    }
    stdMathTest_AssertFloatBitsEqual(1.0f, stdMath_Tan(225.0f));

    const float aCenters[] = { 90.0f, 180.0f, 270.0f, 360.0f };
    for ( size_t i = 0; i < STD_ARRAYLEN(aCenters); ++i )
    {
        stdMathTest_AssertTanMatchesReference(nextafterf(aCenters[i], -INFINITY));
        stdMathTest_AssertTanMatchesReference(aCenters[i]);
        stdMathTest_AssertTanMatchesReference(nextafterf(aCenters[i], INFINITY));
    }
}

TEST(stdMath, TestTanNonFinite)
{
    stdMathTest_AssertIsNaN(stdMath_Tan(NAN));
    stdMathTest_AssertIsNaN(stdMath_Tan(INFINITY));
    stdMathTest_AssertIsNaN(stdMath_Tan(-INFINITY));
}

TEST(stdMath, TestArcSinClamp)
{
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 90.0f, stdMath_ArcSin1(1.000001f));
    TEST_ASSERT_FLOAT_WITHIN(0.001f, -90.0f, stdMath_ArcSin1(-1.000001f));
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 90.0f, stdMath_ArcSin2(1.000001f));
    TEST_ASSERT_FLOAT_WITHIN(0.001f, -90.0f, stdMath_ArcSin2(-1.000001f));
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 90.0f, stdMath_ArcSin3(1.000001f));
    TEST_ASSERT_FLOAT_WITHIN(0.001f, -90.0f, stdMath_ArcSin3(-1.000001f));
}

TEST(stdMath, TestArcSinPrecision)
{
    // Pin intermediate rounding and both sides of the range-reduction boundary.
    const struct
    {
        uint32_t input;
        uint32_t expected[3];
    } aCases[] = {
        { 0xBF800000u, { 0xC2B40000u, 0xC2B40000u, 0xC2B40000u } },
        { 0xBF7FFFFFu, { 0xC2B3F5DFu, 0xC2B3F5DFu, 0xC2B3F5DFu } },
        { 0xBF666666u, { 0xC2800818u, 0xC280475Au, 0xC2804F93u } },
        { 0xBF3504F4u, { 0xC234B450u, 0xC23429FBu, 0xC2340BF1u } },
        { 0xBF3504F3u, { 0xC2334BB1u, 0xC233D606u, 0xC233F411u } },
        { 0xBF000000u, { 0xC1F15F6Du, 0xC1F03E5Eu, 0xC1F00BC2u } },
        { 0xBDFCD6DEu, { 0xC0E31669u, 0xC0E2EF21u, 0xC0E2EEBBu } },
        { 0x358637BDu, { 0x387050E1u, 0x387050E1u, 0x387050E1u } },
        { 0x3DFCD6DEu, { 0x40E31669u, 0x40E2EF21u, 0x40E2EEBBu } },
        { 0x3F000000u, { 0x41F15F6Du, 0x41F03E5Eu, 0x41F00BC2u } },
        { 0x3F3504F2u, { 0x42334BB0u, 0x4233D605u, 0x4233F40Fu } },
        { 0x3F3504F3u, { 0x42334BB1u, 0x4233D606u, 0x4233F411u } },
        { 0x3F3504F4u, { 0x4234B450u, 0x423429FBu, 0x42340BF1u } },
        { 0x3F666666u, { 0x42800818u, 0x4280475Au, 0x42804F93u } },
        { 0x3F7FFFFFu, { 0x42B3F5DFu, 0x42B3F5DFu, 0x42B3F5DFu } },
        { 0x3F800000u, { 0x42B40000u, 0x42B40000u, 0x42B40000u } },
    };
    for ( size_t i = 0; i < STD_ARRAYLEN(aCases); ++i )
    {
        float input = stdMathTest_FloatFromBits(aCases[i].input);
        TEST_ASSERT_EQUAL_HEX32(aCases[i].expected[0], stdMathTest_FloatBits(stdMath_ArcSin1(input)));
        TEST_ASSERT_EQUAL_HEX32(aCases[i].expected[1], stdMathTest_FloatBits(stdMath_ArcSin2(input)));
        TEST_ASSERT_EQUAL_HEX32(aCases[i].expected[2], stdMathTest_FloatBits(stdMath_ArcSin3(input)));
    }

    // Zero must remain zero; its sign is not part of the inverse-trig contract.
    const float aZeros[] = { 0.0f, -0.0f };
    for ( size_t i = 0; i < STD_ARRAYLEN(aZeros); ++i )
    {
        TEST_ASSERT_EQUAL_FLOAT(0.0f, stdMath_ArcSin1(aZeros[i]));
        TEST_ASSERT_EQUAL_FLOAT(0.0f, stdMath_ArcSin2(aZeros[i]));
        TEST_ASSERT_EQUAL_FLOAT(0.0f, stdMath_ArcSin3(aZeros[i]));
    }
}

TEST(stdMath, TestArcSinStdLibApprox)
{
    const float inputs[] = {
        -1.0f,
        -0.9f,
        -0.70710677f,
        -0.5f,
        -0.25f,
        0.0f,
        0.25f,
        0.5f,
        0.70710677f,
        0.9f,
        1.0f
    };

    for ( size_t i = 0; i < STD_ARRAYLEN(inputs); ++i )
    {
        float expected = STDMATH_TODEGREESF(asinf(inputs[i]));

        TEST_ASSERT_FLOAT_WITHIN(0.35f, expected, stdMath_ArcSin1(inputs[i]));
        TEST_ASSERT_FLOAT_WITHIN(0.08f, expected, stdMath_ArcSin2(inputs[i]));
        TEST_ASSERT_FLOAT_WITHIN(0.03f, expected, stdMath_ArcSin3(inputs[i]));
    }
}

TEST(stdMath, TestArcSinNaN)
{
    stdMathTest_AssertIsNaN(stdMath_ArcSin1(NAN));
    stdMathTest_AssertIsNaN(stdMath_ArcSin2(NAN));
    stdMathTest_AssertIsNaN(stdMath_ArcSin3(NAN));
}

TEST(stdMath, TestArcTan4)
{
    const struct
    {
        float x;
        float y;
    } samples[] = {
        { 1.0f, 0.0f },
        { 1.0f, 1.0f },
        { 0.0f, 1.0f },
        { -1.0f, 1.0f },
        { -1.0f, 0.0f },
        { -1.0f, -1.0f },
        { 0.0f, -1.0f },
        { 1.0f, -1.0f },
        { 3.0f, 7.0f },
        { -3.0f, 7.0f },
        { -3.0f, -7.0f },
        { 3.0f, -7.0f }
    };

    TEST_ASSERT_EQUAL_FLOAT(0.0f, stdMath_ArcTan4(0.0f, 0.0f));

    for ( size_t i = 0; i < STD_ARRAYLEN(samples); ++i )
    {
        float expected = -STDMATH_TODEGREESF(atan2f(samples[i].y, samples[i].x));
        float actual = stdMath_ArcTan4(samples[i].x, samples[i].y);

        TEST_ASSERT_FLOAT_WITHIN(0.1f, expected, actual);
    }
}

TEST(stdMath, TestArcTanPrecision)
{
    // Axis, quadrant, and unequal-component cases catch changes hidden by tolerances.
    const struct
    {
        uint32_t x;
        uint32_t y;
        uint32_t expected;
    } aCases[] = {
        { 0x3F800000u, 0x00000000u, 0x80000000u },
        { 0x00000000u, 0x3F800000u, 0xC2B40000u },
        { 0xBF800000u, 0x00000000u, 0xC3340000u },
        { 0x00000000u, 0xBF800000u, 0x42B40000u },
        { 0x3F800000u, 0x3F800000u, 0xC2346098u },
        { 0xBF800000u, 0x3F800000u, 0xC306E7DAu },
        { 0xBF800000u, 0xBF800000u, 0x4306E7DAu },
        { 0x3F800000u, 0xBF800000u, 0x42346098u },
        { 0x40400000u, 0x40E00000u, 0xC2859ACCu },
        { 0xC0400000u, 0x40E00000u, 0xC2E26534u },
        { 0xC0400000u, 0xC0E00000u, 0x42E26534u },
        { 0x40400000u, 0xC0E00000u, 0x42859ACCu },
        { 0x3DFCD6DEu, 0x3F43F35Cu, 0xC2A1ACE2u },
        { 0x3F800000u, 0x358637BDu, 0xB8800000u },
        { 0x358637BDu, 0x3F800000u, 0xC2B3FFF8u },
    };
    for ( size_t i = 0; i < STD_ARRAYLEN(aCases); ++i )
    {
        float x = stdMathTest_FloatFromBits(aCases[i].x);
        float y = stdMathTest_FloatFromBits(aCases[i].y);
        TEST_ASSERT_EQUAL_HEX32(aCases[i].expected, stdMathTest_FloatBits(stdMath_ArcTan4(x, y)));
    }
}

TEST(stdMath, TestDist2D)
{
    TEST_ASSERT_FLOAT_WITHIN(0.00001f, 0.0f, stdMath_Dist2D1(0.0f, 0.0f));
    TEST_ASSERT_FLOAT_WITHIN(0.00001f, 5.5f, stdMath_Dist2D1(3.0f, 4.0f));
    TEST_ASSERT_FLOAT_WITHIN(0.00001f, 5.5f, stdMath_Dist2D1(-3.0f, 4.0f));
    TEST_ASSERT_FLOAT_WITHIN(0.00001f, 5.5f, stdMath_Dist2D1(4.0f, -3.0f));
    TEST_ASSERT_FLOAT_WITHIN(0.00001f, 6.0f, stdMath_Dist2D1(-4.0f, -4.0f));
}

TEST(stdMath, TestDist3D)
{
    stdMathTest_AssertDist3D(0.0f, 0.0f, 0.0f, 0.0f);
    stdMathTest_AssertDist3D(7.0f, 1.0f, 5.0f, 3.0f);
    stdMathTest_AssertDist3D(7.0f, 3.0f, 5.0f, 1.0f);
    stdMathTest_AssertDist3D(7.0f, 1.0f, 3.0f, 5.0f);
    stdMathTest_AssertDist3D(7.0f, 5.0f, 1.0f, 3.0f);
    stdMathTest_AssertDist3D(7.0f, 5.0f, 3.0f, 1.0f);
    stdMathTest_AssertDist3D(7.0f, 3.0f, 1.0f, 5.0f);
    stdMathTest_AssertDist3D(7.0f, -1.0f, -5.0f, -3.0f);
}

TEST(stdMath, TestSmoothDamp)
{
    float movedForward;
    float movedBackward;

    TEST_ASSERT_FLOAT_WITHIN(0.00001f, 2.0f, stdMath_SmoothDamp(2.0f, 10.0f, 5.0f, 0.0f));
    TEST_ASSERT_EQUAL_FLOAT(2.0f, stdMath_SmoothDamp(2.0f, 10.0f, 5.0f, -1.0f));
    TEST_ASSERT_EQUAL_FLOAT(2.0f, stdMath_SmoothDamp(2.0f, 10.0f, -1.0f, 0.5f));
    TEST_ASSERT_FLOAT_WITHIN(0.00001f, 2.0f, stdMath_SmoothDamp(2.0f, 10.0f, 0.0f, 0.5f));
    TEST_ASSERT_FLOAT_WITHIN(0.00001f, 10.0f, stdMath_SmoothDamp(2.0f, 10.0f, 100.0f, 1.0f));
    TEST_ASSERT_FLOAT_WITHIN(
        0.00001f,
        10.0f,
        stdMath_SmoothDamp(10.0f + (STDMATH_ZERO_EPSILON * 0.5f), 10.0f, 5.0f, 0.5f)
    );

    movedForward = stdMath_SmoothDamp(0.0f, 10.0f, 2.0f, 0.5f);
    TEST_ASSERT_FLOAT_WITHIN(0.00001f, 6.32120559f, movedForward);

    movedBackward = stdMath_SmoothDamp(10.0f, 0.0f, 2.0f, 0.5f);
    TEST_ASSERT_FLOAT_WITHIN(0.00001f, 3.67879441f, movedBackward);
}

TEST(stdMath, TestDampingRange)
{
    // Opposite finite endpoints can overflow a float subtraction before interpolation.
    const float aEndpoints[][2] = {
        { -FLT_MAX, FLT_MAX },
        { FLT_MAX, -FLT_MAX },
        { FLT_MAX, FLT_MAX },
        { -FLT_MAX, -FLT_MAX },
        { FLT_MAX * 0.75f, FLT_MAX },
        { -FLT_MAX * 0.75f, -FLT_MAX },
    };
    const float aTimesteps[] = { 0.0f, FLT_TRUE_MIN, 0.000001f, 0.25f, 1.0f, 10.0f, 21.0f };
    for ( size_t i = 0; i < STD_ARRAYLEN(aEndpoints); ++i )
    {
        float current = aEndpoints[i][0];
        float target  = aEndpoints[i][1];
        for ( size_t j = 0; j < STD_ARRAYLEN(aTimesteps); ++j )
        {
            double t       = -expm1(-(double)aTimesteps[j]);
            float expected = (float)((double)current + ((double)target - current) * t);
            float actual   = stdMath_SmoothDamp(current, target, 1.0f, aTimesteps[j]);
            TEST_ASSERT_TRUE(isfinite(actual));
            TEST_ASSERT_TRUE(actual >= fminf(current, target) && actual <= fmaxf(current, target));
            TEST_ASSERT_FLOAT_WITHIN(FLT_MAX * 0.000002f, expected, actual);
        }
    }

    float combined = stdMath_SmoothDamp(-FLT_MAX, FLT_MAX, 1.0f, 1.0f);
    float stepped  = -FLT_MAX;
    for ( size_t i = 0; i < 16; ++i )
    {
        stepped = stdMath_SmoothDamp(stepped, FLT_MAX, 1.0f, 0.0625f);
        TEST_ASSERT_TRUE(isfinite(stepped) && stepped >= -FLT_MAX && stepped <= FLT_MAX);
    }
    TEST_ASSERT_FLOAT_WITHIN(FLT_MAX * 0.000005f, combined, stepped);
}

TEST_GROUP_RUNNER(stdMath)
{
    RUN_TEST_CASE(stdMath, TestLookupTables);
    RUN_TEST_CASE(stdMath, TestConstantAndHelperMacros);
    RUN_TEST_CASE(stdMath, TestDoubleConversionMacros);
    RUN_TEST_CASE(stdMath, TestClipNearZero);
    RUN_TEST_CASE(stdMath, TestExpm1f);
    RUN_TEST_CASE(stdMath, TestFlexPower);
    RUN_TEST_CASE(stdMath, TestFlexPowerZeroNegativeExponent);
    RUN_TEST_CASE(stdMath, TestNegativePowers);
    RUN_TEST_CASE(stdMath, TestNormalizeAngle);
    RUN_TEST_CASE(stdMath, TestNormalizeAngleNonFinite);
    RUN_TEST_CASE(stdMath, TestAngleBounds);
    RUN_TEST_CASE(stdMath, TestAngleBoundaries);
    RUN_TEST_CASE(stdMath, TestNormalizeAngleAcute);
    RUN_TEST_CASE(stdMath, TestSinCosTableSweep);
    RUN_TEST_CASE(stdMath, TestSinCosHalfDegreeSweep);
    RUN_TEST_CASE(stdMath, TestSinCosStdLibApprox);
    RUN_TEST_CASE(stdMath, TestSinCosNonFinite);
#ifdef J3D_RUNTIME_GUARDS
    RUN_TEST_CASE(stdMath, TestSinCosNullOutputGuard);
#endif
    RUN_TEST_CASE(stdMath, TestTanTableSweep);
    RUN_TEST_CASE(stdMath, TestTanHalfDegreeSweep);
    RUN_TEST_CASE(stdMath, TestTanStdLibApprox);
    RUN_TEST_CASE(stdMath, TestTanQuadrants);
    RUN_TEST_CASE(stdMath, TestTanNonFinite);
    RUN_TEST_CASE(stdMath, TestArcSinClamp);
    RUN_TEST_CASE(stdMath, TestArcSinPrecision);
    RUN_TEST_CASE(stdMath, TestArcSinStdLibApprox);
    RUN_TEST_CASE(stdMath, TestArcSinNaN);
    RUN_TEST_CASE(stdMath, TestArcTan4);
    RUN_TEST_CASE(stdMath, TestArcTanPrecision);
    RUN_TEST_CASE(stdMath, TestDist2D);
    RUN_TEST_CASE(stdMath, TestDist3D);
    RUN_TEST_CASE(stdMath, TestSmoothDamp);
    RUN_TEST_CASE(stdMath, TestDampingRange);
}

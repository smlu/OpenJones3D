#include "rdMathTest.h"

#include <j3dcore/Tests/j3dTest.h>

#include <float.h>

TEST_GROUP(rdVector);

TEST_SETUP(rdVector)
{
    J3DTest_Startup();
    J3DTest_BindHostServicesGlobal(rdroid_g_pHS_ADDR);
}

TEST_TEAR_DOWN(rdVector)
{
    J3DTest_Shutdown();
}

// Reference evaluation preserves float stores and division versus reciprocal
// multiplication are deliberate; this does not call or claim to execute the EXE.
static float rdVectorTest_ReferenceNormalize(const rdVector4* pSrc, rdVector4* pDest, size_t dimensions, bool bAccumulate)
{
    float aSrc[]  = { pSrc->x, pSrc->y, pSrc->z, pSrc->w };
    float aDest[] = { pSrc->x, pSrc->y, pSrc->z, pSrc->w };

    float lenSqr = aSrc[0] * aSrc[0] + aSrc[1] * aSrc[1];
    for ( size_t i = 2; i < dimensions; ++i )
    {
        lenSqr += aSrc[i] * aSrc[i];
    }

    float length = sqrtf(lenSqr);
    if ( length != 0.0f )
    {
        if ( bAccumulate && dimensions > 2 )
        {
            float reciprocal = (float)(1.0 / length);
            for ( size_t i = 0; i < dimensions; ++i )
            {
                aDest[i] = aSrc[i] * reciprocal;
            }
        }
        else
        {
            for ( size_t i = 0; i < dimensions; ++i )
            {
                aDest[i] = aSrc[i] / length;
            }
        }
    }

    *pDest = (rdVector4){ aDest[0], aDest[1], aDest[2], aDest[3] };
    return length;
}

TEST(rdVector, TestComponentOrder)
{
    // Each small square is just over half an ULP at 1.0. Grouping y/z first
    // loses a different bit, and the difference survives the square root.
    const rdVector3 input = { 1.0f, 0.00025f, 0.00025f };
    TEST_ASSERT_EQUAL_HEX32(0x3f800001u, rdMathTest_FloatBits(rdVector_Len3(&input)));
    TEST_ASSERT_EQUAL_HEX32(0x3f800002u, rdMathTest_FloatBits(rdVector_Dot3(&input, &input)));

    rdVector3 normalized = input;
    float length         = rdVector_Normalize3Acc(&normalized);
    TEST_ASSERT_EQUAL_HEX32(0x3f800001u, rdMathTest_FloatBits(length));
    TEST_ASSERT_EQUAL_HEX32(0x3f7ffffeu, rdMathTest_FloatBits(normalized.x));
}

TEST(rdVector, TestNormalizePrecision)
{
    // Exercise all dimensional and in-place variants, including signed zero,
    // underflow, overflow, and non-finite results preserved by the original.
    const rdVector4 aInputs[] = {
        { 0.0f, 0.0f, 0.0f, 0.0f },
        { -0.0f, 0.0f, -0.0f, -0.0f },
        { 3.0f, 4.0f, 12.0f, -84.0f },
        { -3.0f, 4.0f, -12.0f, 84.0f },
        { 1.0f, 1.0f, 1.0f, 1.0f },
        { 1.0f, 0.00025f, 0.00025f, 0.00025f },
        { 0.125f, -0.375f, 0.625f, -0.875f },
        { 1.0e-20f, -2.0e-20f, 3.0e-20f, -4.0e-20f },
        { FLT_MIN, -FLT_MIN, FLT_MIN, -FLT_MIN },
        { 1.0e19f, -1.0e19f, 1.0e19f, -1.0e19f },
        { FLT_MAX, -FLT_MAX, FLT_MAX, -FLT_MAX },
        { INFINITY, 1.0f, -1.0f, 0.0f },
        { NAN, 1.0f, 2.0f, 3.0f },
    };

    for ( size_t i = 0; i < STD_ARRAYLEN(aInputs); ++i )
    {
        for ( size_t dimensions = 2; dimensions <= 4; ++dimensions )
        {
            for ( int bAccumulate = 0; bAccumulate <= 1; ++bAccumulate )
            {
                rdVector4 expected;
                float expectedLength = rdVectorTest_ReferenceNormalize(&aInputs[i], &expected, dimensions, bAccumulate != 0);
                rdVector4 actual     = aInputs[i];
                float actualLength;
                if ( dimensions == 2 )
                {
                    rdVector2 src  = { actual.x, actual.y };
                    rdVector2 dest = src;
                    actualLength = bAccumulate ? rdVector_Normalize2Acc(&dest) : rdVector_Normalize2(&dest, &src);
                    actual.x = dest.x;
                    actual.y = dest.y;
                }
                else if ( dimensions == 3 )
                {
                    rdVector3 src  = { actual.x, actual.y, actual.z };
                    rdVector3 dest = src;
                    actualLength = bAccumulate ? rdVector_Normalize3Acc(&dest) : rdVector_Normalize3(&dest, &src);
                    actual.x = dest.x;
                    actual.y = dest.y;
                    actual.z = dest.z;
                }
                else
                {
                    rdVector4 src = actual;
                    actualLength = bAccumulate ? rdVector_Normalize4Acc(&actual) : rdVector_Normalize4(&actual, &src);
                }

                rdMathTest_AssertFloat(expectedLength, actualLength);
                rdMathTest_AssertVector4(&expected, &actual);
            }
        }
    }
}

TEST(rdVector, TestReciprocalRounding)
{
    const rdVector4 src = { 7.0f, -11.0f, 19.0f, -23.0f };
    const float aScalars[] = { 3.0f, -3.0f, 0.1f, 1.0f, 0.0f, -0.0f, FLT_MIN, FLT_MAX, INFINITY, -INFINITY, NAN };

    for ( size_t i = 0; i < STD_ARRAYLEN(aScalars); ++i )
    {
        // The float reciprocal (0x3eaaaaab for 1/3) is rounded before
        // multiplication, unlike keeping 1/3 in a double register.
        float reciprocal   = (float)(1.0 / aScalars[i]);
        rdVector4 expected = { src.x * reciprocal, src.y * reciprocal, src.z * reciprocal, src.w * reciprocal };
        rdVector4 actual;
        rdVector_InvScale4(&actual, &src, aScalars[i]);
        rdMathTest_AssertVector4(&expected, &actual);

        actual = src;
        rdVector_InvScale4Acc(&actual, aScalars[i]);
        rdMathTest_AssertVector4(&expected, &actual);

        rdVector3 src3      = { src.x, src.y, src.z };
        rdVector3 expected3 = { expected.x, expected.y, expected.z };
        rdVector3 actual3;
        rdVector_InvScale3(&actual3, &src3, aScalars[i]);
        rdMathTest_AssertVector3(&expected3, &actual3);

        actual3 = src3;
        rdVector_InvScale3Acc(&actual3, aScalars[i]);
        rdMathTest_AssertVector3(&expected3, &actual3);

        // The non-Acc entry points also permit source/destination aliasing.
        actual = src;
        rdVector_InvScale4(&actual, &actual, aScalars[i]);
        rdMathTest_AssertVector4(&expected, &actual);
        actual3 = src3;
        rdVector_InvScale3(&actual3, &actual3, aScalars[i]);
        rdMathTest_AssertVector3(&expected3, &actual3);
    }

    rdVector4 actual;
    rdVector_InvScale4(&actual, &src, 3.0f);
    TEST_ASSERT_EQUAL_HEX32(0x40155556u, rdMathTest_FloatBits(actual.x));
}

TEST(rdVector, TestQuickNormalizePrecision)
{
    // Sort independently of production, covering all six component orderings,
    // ties, signs, and values that expose float length/reciprocal rounding.
    const rdVector3 aInputs[] = {
        { 1.0f, 2.0f, 3.0f }, { 1.0f, 3.0f, 2.0f },
        { 2.0f, 1.0f, 3.0f }, { 2.0f, 3.0f, 1.0f },
        { 3.0f, 1.0f, 2.0f }, { 3.0f, 2.0f, 1.0f },
        { 1.0f, 1.0f, 1.0f }, { 0.0f, 2.0f, 2.0f },
        { -7.0f, 0.1f, -0.3f }, { 0.00025f, -1.0f, 0.00025f },
        { 1.0e-30f, -2.0e-30f, 3.0e-30f },
    };
    for ( size_t i = 0; i < STD_ARRAYLEN(aInputs); ++i )
    {
        float aSorted[] = { fabsf(aInputs[i].x), fabsf(aInputs[i].y), fabsf(aInputs[i].z) };
        for ( size_t j = 0; j < STD_ARRAYLEN(aSorted); ++j )
        {
            for ( size_t k = j + 1; k < STD_ARRAYLEN(aSorted); ++k )
            {
                if ( aSorted[j] < aSorted[k] )
                {
                    float temp = aSorted[j];
                    aSorted[j] = aSorted[k];
                    aSorted[k] = temp;
                }
            }
        }

        float length       = (float)(0.34375 * aSorted[1] + 0.25 * aSorted[2] + aSorted[0]);
        float reciprocal   = (float)(1.0 / length);
        rdVector3 expected = { aInputs[i].x * reciprocal, aInputs[i].y * reciprocal, aInputs[i].z * reciprocal };
        rdVector3 actual   = aInputs[i];
        rdMathTest_AssertFloat(length, rdVector_Normalize3QuickAcc(&actual));
        rdMathTest_AssertVector3(&expected, &actual);
    }

    // Preserve the original zero-vector failure rather than quietly fixing it.
    rdVector3 zero = { 0.0f, 0.0f, 0.0f };
    rdMathTest_AssertFloat(0.0f, rdVector_Normalize3QuickAcc(&zero));
    TEST_ASSERT_TRUE(isnan(zero.x) && isnan(zero.y) && isnan(zero.z));
}

TEST(rdVector, TestNormalizeAssertions)
{
    // Capture the original non-aliasing assertions without terminating the process.
    rdVector2 v2 = { 3.0f, 4.0f };
    rdVector3 v3 = { 3.0f, 4.0f, 0.0f };
    rdVector4 v4 = { 3.0f, 4.0f, 0.0f, 0.0f };
    rdVector_Normalize2(&v2, &v2);
    rdVector_Normalize3(&v3, &v3);
    rdVector_Normalize4(&v4, &v4);
    TEST_ASSERT_EQUAL_INT(3, J3DTest_GetAssertCount());
    J3DTest_AssertLastAssert("dest != src", "rdVector.c");
}

TEST_GROUP_RUNNER(rdVector)
{
    RUN_TEST_CASE(rdVector, TestComponentOrder);
    RUN_TEST_CASE(rdVector, TestNormalizePrecision);
    RUN_TEST_CASE(rdVector, TestReciprocalRounding);
    RUN_TEST_CASE(rdVector, TestQuickNormalizePrecision);
    RUN_TEST_CASE(rdVector, TestNormalizeAssertions);
}

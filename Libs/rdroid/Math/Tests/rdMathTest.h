#ifndef RDROID_TESTS_RDMATHTEST_H
#define RDROID_TESTS_RDMATHTEST_H

#include <unity_fixture.h>

#include <rdroid/Math/rdVector.h>

#include <stdint.h>
#include <string.h>

static inline uint32_t rdMathTest_FloatBits(float value)
{
    uint32_t bits;
    memcpy(&bits, &value, sizeof(bits));
    return bits;
}

static inline void rdMathTest_AssertFloat(float expected, float actual)
{
    // Approximate float assertions would hide the one-ULP differences under test.
    if ( isnan(expected) )
    {
        TEST_ASSERT_TRUE(isnan(actual));
    }
    else
    {
        TEST_ASSERT_EQUAL_UINT32(rdMathTest_FloatBits(expected), rdMathTest_FloatBits(actual));
    }
}

static inline void rdMathTest_AssertVector3(const rdVector3* pExpected, const rdVector3* pActual)
{
    rdMathTest_AssertFloat(pExpected->x, pActual->x);
    rdMathTest_AssertFloat(pExpected->y, pActual->y);
    rdMathTest_AssertFloat(pExpected->z, pActual->z);
}

static inline void rdMathTest_AssertVector4(const rdVector4* pExpected, const rdVector4* pActual)
{
    rdMathTest_AssertFloat(pExpected->x, pActual->x);
    rdMathTest_AssertFloat(pExpected->y, pActual->y);
    rdMathTest_AssertFloat(pExpected->z, pActual->z);
    rdMathTest_AssertFloat(pExpected->w, pActual->w);
}

#endif // RDROID_TESTS_RDMATHTEST_H

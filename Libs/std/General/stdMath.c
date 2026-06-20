#include "std.h"
#include "stdMath.h"
#include "stdMathTables.h"
#include "stdUtil.h"

#include <j3dcore/j3dhook.h>
#include <std/RTI/symbols.h>

#include <limits.h>

// Helper macros for table lookups
#define SIN_TABLE_SIZE STD_ARRAYLEN(stdMath_aSinTable)
static_assert(SIN_TABLE_SIZE == 4096, "SIN_TABLE_SIZE == 4096");

#define SIN_TABLE_LAST_IDX (SIN_TABLE_SIZE - 1)
#define SIN_TABLE_GET(idx) stdMath_aSinTable[((idx) )]
#define SIN_TABLE_GET_REVERSE(idx) stdMath_aSinTable[SIN_TABLE_LAST_IDX - ((idx) % SIN_TABLE_SIZE)]
#define SIN_TABLE_DEGREES_TO_INDEX ((float)SIN_TABLE_SIZE / 90.0f) // STD_ARRAYLEN(stdMath_aSinTable) over 90 degrees.

#define TAN_TABLE_SIZE STD_ARRAYLEN(stdMath_aTanTable)
static_assert(TAN_TABLE_SIZE == 4096, "TAN_TABLE_SIZE == 4096");

#define TAN_TABLE_LAST_IDX (TAN_TABLE_SIZE - 1)
#define TAN_TABLE_GET(idx) stdMath_aTanTable[(idx) % TAN_TABLE_SIZE]
#define TAN_TABLE_GET_REVERSE(idx) stdMath_aTanTable[TAN_TABLE_LAST_IDX - ((idx) % TAN_TABLE_SIZE)]
#define TAN_TABLE_FULL_CYCLE_SIZE (4 * TAN_TABLE_SIZE) // 4 * STD_ARRAYLEN(stdMath_aTanTable) covers 360 degrees.

// The arc-sine functions approximate asin(x) with odd polynomials on [0, sqrt(1/2)].
// For larger inputs they use asin(x) = pi/2 - asin(sqrt(1 - x*x)).
// Taylor terms come from asin(x) = x + x^3/6 + 3*x^5/40 + 5*x^7/112 + ...
// The non-Taylor values are fitted coefficients preserved from the original engine.
#define ARCSIN1_FITTED_CUBIC_COEFF    0.212749    // Original fitted x^3 coefficient for x + c*x^3.
#define ARCSIN2_FITTED_QUINTIC_COEFF  0.105502    // Original fitted x^5 coefficient after x + x^3/6.
#define ARCSIN_TAYLOR_CUBIC_DIVISOR   6.0         // Taylor x^3 coefficient is 1/6.
#define ARCSIN3_TAYLOR_QUINTIC_COEFF  0.075000003 // Taylor x^5 coefficient is 3/40, preserving original rounding.
#define ARCSIN3_FITTED_SEPTIC_COEFF   0.066797003 // Original fitted x^7 coefficient, not the Taylor 5/112 term.
// Arc-tangent reduces to ratio <= 1, then uses atan(x)'s alternating Taylor series.
// The x^9 coefficient is fitted instead of using the raw Taylor 1/9 term.
#define ARCTAN_TAYLOR_CUBIC_DIVISOR   3.0       // atan Taylor x^3 coefficient is -1/3.
#define ARCTAN_TAYLOR_QUINTIC_DIVISOR 5.0       // atan Taylor x^5 coefficient is +1/5.
#define ARCTAN_TAYLOR_SEPTIC_DIVISOR  7.0       // atan Taylor x^7 coefficient is -1/7.
#define ARCTAN_FITTED_NONIC_COEFF     0.063235f // Original fitted x^9 coefficient, not the Taylor +1/9 term.

static float stdMath_ClampArcTrigInput(float num)
{
    // Added: Preserve NaN inputs while clamping small out-of-range values before sqrt.
    if ( isnan(num) )
    {
        return NAN;
    }

    if ( num > 1.0f )
    {
        return 1.0f;
    }

    if ( num < -1.0f )
    {
        return -1.0f;
    }

    return num;
}

void stdMath_InstallHooks(void)
{
    J3D_HOOKFUNC(stdMath_FlexPower);
    J3D_HOOKFUNC(stdMath_NormalizeAngle);
    J3D_HOOKFUNC(stdMath_NormalizeAngleAcute);
    J3D_HOOKFUNC(stdMath_SinCos);
    J3D_HOOKFUNC(stdMath_Tan);
    J3D_HOOKFUNC(stdMath_ArcSin1);
    J3D_HOOKFUNC(stdMath_ArcSin3);
    J3D_HOOKFUNC(stdMath_ArcTan4);
    J3D_HOOKFUNC(stdMath_Dist2D1);
    J3D_HOOKFUNC(stdMath_Dist3D1);
}

void stdMath_ResetGlobals(void)
{}

float J3DAPI stdMath_FlexPower(float base, int exponent)
{
    // Fixed: base^0 is 1; the original loop returned base for exponent 0.
    if ( exponent == 0 )
    {
        return 1.0f;
    }

    // Fixed: Support negative exponents without overflowing INT_MIN.
    bool bNegativeExponent = exponent < 0;
    unsigned int count = bNegativeExponent
        ? (unsigned int)(-(exponent + 1)) + 1u
        : (unsigned int)exponent;

    float ret = 1.0f;
    for ( unsigned int i = 0; i < count; ++i )
    {
        ret = ret * base;
    }

    if ( bNegativeExponent )
    {
        return ret != 0.0f ? 1.0f / ret : INFINITY;
    }

    return ret;
}

float stdMath_NormalizeAngle(float angle)
{
    // Added: Added nan check
    if ( isnan(angle) )
    {
        return angle;
    }

    // Fixed: Reject infinities before floor() normalization.
    if ( isinf(angle) )
    {
        return NAN;
    }

    // Fixed: Use double to fix float precision errors

    double normAngle;
    if ( angle >= 0.0f )
    {
        if ( angle < 360.0f )
        {
            return angle;
        }
        normAngle = (double)angle - floor((double)angle / 360.0) * 360.0;
    }
    else
    {
        if ( -angle >= 360.0f )
        {
            normAngle = 360.0 - (-(double)angle - floor(-(double)angle / 360.0) * 360.0);
        }
        else
        {
            normAngle = 360.0f + angle;
        }
    }

    if ( normAngle == 360.0 )
    {
        normAngle = 0.0;
    }

    STD_ASSERT(normAngle >= 0.0 && normAngle <= 360.0);
    return (float)normAngle;
}

float J3DAPI stdMath_NormalizeAngleAcute(float angle)
{
    float normAngle = stdMath_NormalizeAngle(angle);
    if ( normAngle > 180.0f )
    {
        return -(360.0f - normAngle);
    }
    return normAngle;
}

void stdMath_SinCos(float angle, float* pSinOut, float* pCosOut)
{
    // Added: Guard for optional output pointers.
    STD_GUARD_VOID(pSinOut != NULL && pCosOut != NULL);

    // Added: Preserve NaN inputs while rejecting them before table-index math.
    if ( isnan(angle) )
    {
        *pSinOut = NAN;
        *pCosOut = NAN;
        return;
    }

    // Fixed: Reject infinities before table-index math.
    if ( isinf(angle) )
    {
        *pSinOut = NAN;
        *pCosOut = NAN;
        return;
    }

    float normAngle = stdMath_NormalizeAngle(angle);

    // Split the normalized angle into a quadrant; each quadrant maps to the same sin table.
    int32_t quadrant = 0;
    if ( normAngle >= 270.0f )
    {
        quadrant = 3;
    }
    else if ( normAngle >= 180.0f )
    {
        quadrant = 2;
    }
    else if ( normAngle >= 90.0f )
    {
        quadrant = 1;
    }

    // Convert degrees to a fractional table index, then linearly interpolate adjacent samples.
    float indexFloat  = normAngle * SIN_TABLE_DEGREES_TO_INDEX;
    float fracPart    = indexFloat - floorf(indexFloat);
    int32_t index     = (int32_t)indexFloat;
    int32_t nextIndex = index + 1;

    float sinLookup = 0.0f, sinValue = 0.0f,
        cosLookup   = 0.0f, cosValue = 0.0f;
    switch ( quadrant )
    {
        case 0:
        {
            sinLookup = SIN_TABLE_GET(index);
            if ( nextIndex < SIN_TABLE_SIZE )
            {
                sinValue = SIN_TABLE_GET(nextIndex);
            }
            else
            {
                sinValue = SIN_TABLE_GET_REVERSE(index - SIN_TABLE_LAST_IDX);
            }

            cosLookup = SIN_TABLE_GET_REVERSE(index);
            if ( nextIndex < SIN_TABLE_SIZE )
            {
                cosValue = SIN_TABLE_GET_REVERSE(nextIndex);
            }
            else
            {
                cosValue = -SIN_TABLE_GET(nextIndex - SIN_TABLE_SIZE);
            }
        }
        break;

        case 1:
        {
            sinLookup = SIN_TABLE_GET_REVERSE(index - SIN_TABLE_SIZE);
            if ( nextIndex < 2 * SIN_TABLE_SIZE )
            {
                sinValue = SIN_TABLE_GET_REVERSE(index - SIN_TABLE_LAST_IDX);
            }
            else
            {
                sinValue = -SIN_TABLE_GET(nextIndex - 2 * SIN_TABLE_SIZE);
            }

            cosLookup = -SIN_TABLE_GET(index - SIN_TABLE_SIZE);
            if ( nextIndex < 2 * SIN_TABLE_SIZE )
            {
                cosValue = -SIN_TABLE_GET(nextIndex - SIN_TABLE_SIZE);
            }
            else
            {
                cosValue = -SIN_TABLE_GET_REVERSE((index - (2 * SIN_TABLE_SIZE - 1)));
            }
        }
        break;

        case 2:
        {
            sinLookup = -SIN_TABLE_GET(index - 2 * SIN_TABLE_SIZE);
            if ( nextIndex < 3 * SIN_TABLE_SIZE )
            {
                sinValue = -SIN_TABLE_GET(nextIndex - 2 * SIN_TABLE_SIZE);
            }
            else
            {
                sinValue = -SIN_TABLE_GET_REVERSE((index - (3 * SIN_TABLE_SIZE - 1)));
            }

            cosLookup = -SIN_TABLE_GET_REVERSE((index - 2 * SIN_TABLE_SIZE));
            if ( nextIndex < 3 * SIN_TABLE_SIZE )
            {
                cosValue = -SIN_TABLE_GET_REVERSE((index - (2 * SIN_TABLE_SIZE - 1)));
            }
            else
            {
                cosValue = SIN_TABLE_GET(nextIndex - 3 * SIN_TABLE_SIZE);
            }
        }
        break;

        case 3:
        {
            sinLookup = -SIN_TABLE_GET_REVERSE((index - 3 * SIN_TABLE_SIZE));
            if ( nextIndex < 4 * SIN_TABLE_SIZE )
            {
                sinValue = -SIN_TABLE_GET_REVERSE((index - (3 * SIN_TABLE_SIZE - 1)));
            }
            else
            {
                sinValue = SIN_TABLE_GET(nextIndex - 4 * SIN_TABLE_SIZE);
            }

            cosLookup = SIN_TABLE_GET(index - 3 * SIN_TABLE_SIZE);
            if ( nextIndex < 4 * SIN_TABLE_SIZE )
            {
                cosValue = SIN_TABLE_GET(nextIndex - 3 * SIN_TABLE_SIZE);
            }
            else
            {
                cosValue = SIN_TABLE_GET_REVERSE(index - (4 * SIN_TABLE_SIZE - 1));
            }
        }
        break;
    }

    *pSinOut = (sinValue - sinLookup) * fracPart + sinLookup;
    *pCosOut = (cosValue - cosLookup) * fracPart + cosLookup;
}

float J3DAPI stdMath_Tan(float angle)
{
    // Added: Preserve the original NaN guard before table-index math.
    if ( isnan(angle) )
    {
        return NAN;
    }

    // Fixed: Reject infinities before table-index math.
    if ( isinf(angle) )
    {
        return NAN;
    }

    float normAngle = stdMath_NormalizeAngle(angle);

    // Split the normalized angle into a quadrant; tangent mirrors/sign-flips the same table.
    int32_t quadrant = 0;
    if ( normAngle >= 270.0f )
    {
        quadrant = 3;
    }
    else if ( normAngle >= 180.0f )
    {
        quadrant = 2;
    }
    else if ( normAngle >= 90.0f )
    {
        quadrant = 1;
    }

    // Map the full 360-degree cycle into four table-sized quadrants and interpolate.
    float indexFloat  = normAngle / 360.0f * (float)TAN_TABLE_FULL_CYCLE_SIZE;
    float fracPart    = indexFloat - floorf(indexFloat);
    int32_t index     = (int32_t)indexFloat;
    int32_t nextIndex = index + 1;

    float tanValue = 0.0f, baseLookup = 0.0f;
    switch ( quadrant )
    {
        case 0:
        {
            baseLookup = TAN_TABLE_GET(index);
            if ( nextIndex < TAN_TABLE_SIZE )
            {
                tanValue = TAN_TABLE_GET(nextIndex);
            }
            else
            {
                tanValue = -TAN_TABLE_GET_REVERSE((index - TAN_TABLE_LAST_IDX));
            }
        }
        break;

        case 1:
        {
            baseLookup = -TAN_TABLE_GET_REVERSE((index - TAN_TABLE_SIZE));
            if ( nextIndex < 2 * TAN_TABLE_SIZE )
            {
                tanValue = -TAN_TABLE_GET_REVERSE((index - TAN_TABLE_LAST_IDX));
            }
            else
            {
                tanValue = TAN_TABLE_GET(nextIndex - 2 * TAN_TABLE_SIZE);
            }
        }
        break;

        case 2:
        {
            // TODO: [BUG] This preserved original reverse lookup makes tan(180..270) start near the
            // asymptote instead of 0. Use TAN_TABLE_GET(index - 2 * TAN_TABLE_SIZE) to fix it.
            baseLookup = TAN_TABLE_GET_REVERSE((index - 2 * TAN_TABLE_SIZE));
            if ( nextIndex < 3 * TAN_TABLE_SIZE )
            {
                tanValue = TAN_TABLE_GET(nextIndex - 2 * TAN_TABLE_SIZE);
            }
            else
            {
                tanValue = -TAN_TABLE_GET_REVERSE((index - (3 * TAN_TABLE_SIZE - 1)));
            }
        }
        break;

        case 3:
        {
            baseLookup = -TAN_TABLE_GET_REVERSE((index - 3 * TAN_TABLE_SIZE));
            if ( nextIndex < 4 * TAN_TABLE_SIZE )
            {
                tanValue = -TAN_TABLE_GET_REVERSE((index - (3 * TAN_TABLE_SIZE - 1)));
            }
            else
            {
                tanValue = TAN_TABLE_GET(nextIndex - 4 * TAN_TABLE_SIZE);
            }
        }
        break;
    }

    return (tanValue - baseLookup) * fracPart + baseLookup;
}

float J3DAPI stdMath_ArcSin1(float num)
{
    // Fixed: Clamp input before sqrt(1 - x*x) to avoid NaN from tiny overshoots.
    num = stdMath_ClampArcTrigInput(num);

    double asinval;
    double absNum = fabs(num);
    // Approximate positive asin and restore the original sign at the end.
    if ( absNum <= M_SQRT1_2 )
    {
        asinval = STDMATH_TODEGREES(pow(absNum, 3) * ARCSIN1_FITTED_CUBIC_COEFF + absNum);
    }
    else
    {
        double sqrtComplement = sqrt(1.0 - absNum * absNum);
        asinval = 90.0 - STDMATH_TODEGREES(pow(sqrtComplement, 3) * ARCSIN1_FITTED_CUBIC_COEFF + sqrtComplement);
    }

    return (float)(num < 0.0 ? -asinval : asinval);
}

float J3DAPI stdMath_ArcSin2(float num)
{
    // Fixed: Clamp input before sqrt(1 - x*x) to avoid NaN from tiny overshoots.
    num = stdMath_ClampArcTrigInput(num);

    double asinval;
    double absnum = fabs(num);
    // Same range reduction as ArcSin1, with one extra odd-polynomial term.
    if ( absnum <= M_SQRT1_2 )
    {
        double term1 = pow(absnum, 3) / ARCSIN_TAYLOR_CUBIC_DIVISOR + absnum;
        asinval = STDMATH_TODEGREES(pow(absnum, 5) * ARCSIN2_FITTED_QUINTIC_COEFF + term1);
    }
    else
    {
        double sqrtComplement = sqrt(1.0 - absnum * absnum);
        double term1 = pow(sqrtComplement, 3) / ARCSIN_TAYLOR_CUBIC_DIVISOR + sqrtComplement;
        asinval = 90.0 - STDMATH_TODEGREES(pow(sqrtComplement, 5) * ARCSIN2_FITTED_QUINTIC_COEFF + term1);
    }

    return (float)(num < 0.0 ? -asinval : asinval);
}

float J3DAPI stdMath_ArcSin3(float num)
{
    // Fixed: Clamp input before sqrt(1 - x*x) to avoid NaN from tiny overshoots.
    num = stdMath_ClampArcTrigInput(num);

    double asinval = fabs(num);
    // Highest-order arc-sine approximation used here: x, x^3, x^5, and fitted x^7.
    if ( asinval <= M_SQRT1_2 )
    {
        double term1 = pow(asinval, 3) / ARCSIN_TAYLOR_CUBIC_DIVISOR + asinval;
        double term2 = pow(asinval, 5) * ARCSIN3_TAYLOR_QUINTIC_COEFF + term1;
        asinval = STDMATH_TODEGREES(pow(asinval, 7) * ARCSIN3_FITTED_SEPTIC_COEFF + term2);
    }
    else
    {
        double sqrtVal = sqrt(1.0 - asinval * asinval);
        double term    = pow(sqrtVal, 3) / ARCSIN_TAYLOR_CUBIC_DIVISOR + sqrtVal;

        term    = pow(sqrtVal, 5) * ARCSIN3_TAYLOR_QUINTIC_COEFF + term;
        asinval = 90.0 - STDMATH_TODEGREES(pow(sqrtVal, 7) * ARCSIN3_FITTED_SEPTIC_COEFF + term);
    }

    return (float)(num < 0.0 ? -asinval : asinval);
}

float stdMath_ArcTan4(float x, float y)
{
    if ( y == 0.0f && x == 0.0f )
        return 0.0f;

    double absX = fabs(x);
    double absY = fabs(y);
    double ratio = 0.0;

    if ( absY <= absX )
    {
        ratio = absY / absX;
    }
    else
    {
        ratio = absX / absY;
    }
    ratio = fabs(ratio);

    // Approximate atan(ratio) in radians, then convert to engine degrees.
    double angle = ratio - pow(ratio, 3) / ARCTAN_TAYLOR_CUBIC_DIVISOR;
    angle = pow(ratio, 5) / ARCTAN_TAYLOR_QUINTIC_DIVISOR + angle;
    angle = angle - pow(ratio, 7) / ARCTAN_TAYLOR_SEPTIC_DIVISOR;
    angle = STDMATH_TODEGREES(pow(ratio, 9) * ARCTAN_FITTED_NONIC_COEFF + angle);

    // Reconstruct the quadrant and sign after reducing the original vector to ratio <= 1.
    if ( absX >= absY )
    {
        angle = 90.0 - angle;
    }

    angle = 90.0 - angle;
    if ( x < 0.0f )
    {
        angle = 180.0 - angle;
    }

    if ( y >= 0.0f )
    {
        angle = -angle;
    }

    return (float)angle;
}

float stdMath_Dist2D1(float x, float y)
{
    x = fabsf(x);
    y = fabsf(y);
    float minValue = x >= y ? y : x;
    float maxValue = x <= y ? y : x;
    return minValue / 2.0f + maxValue;
}

float stdMath_Dist3D1(float x, float y, float z)
{
    x = fabsf(x);
    y = fabsf(y);
    z = fabsf(z);
    float minValue = 0.0f, midValue = 0.0f, maxValue = 0.0f;

    if ( x <= y )
    {
        if ( y > z )
        {
            maxValue = y;
            if ( x <= z )
            {
                midValue = z;
                minValue = x;
            }
            else
            {
                midValue = x;
                minValue = z;
            }
        }
        else
        {
            maxValue = z;
            midValue = y;
            minValue = x;
        }
    }
    else if ( x > z )
    {
        maxValue = x;
        if ( y <= z )
        {
            midValue = z;
            minValue = y;
        }
        else
        {
            midValue = y;
            minValue = z;
        }
    }
    else
    {
        maxValue = z;
        midValue = x;
        minValue = y;
    }

    return midValue / 2.0f + maxValue + minValue / 2.0f;
}

float stdMath_SmoothDamp(float current, float target, float rate, float deltaTime)
{
    // Handle edge cases
    if ( deltaTime <= 0.0f || rate <= 0.0f )
    {
        return current;
    }

    float exponent = -rate * deltaTime;

    // if exponent is very negative, we've essentially reached target
    if ( exponent < -20.0f )
    {
        return target;
    }

    // Use expm1 for better numerical accuracy
    // We want: t = 1 - exp(exponent) = -(exp(exponent) - 1) = -expm1(exponent)
    // ref: https://blog.pkh.me/p/41-fixing-the-iterative-damping-interpolation-in-video-games.html
    //      https://www.johndcook.com/blog/cpp_expm1/
    float t = -stdMath_Expm1f(exponent);

    // if very close to target, snap to it
    float diff = target - current;
    if ( stdMath_ClipNearZero(diff) == 0.0f )
    {
        return target;
    }

    return current + diff * t;
}
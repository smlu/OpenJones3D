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

#define TAN_TABLE_SIZE STD_ARRAYLEN(stdMath_aTanTable)
static_assert(TAN_TABLE_SIZE == 4096, "TAN_TABLE_SIZE == 4096");

#define TAN_TABLE_LAST_IDX (TAN_TABLE_SIZE - 1)
#define TAN_TABLE_GET(idx) stdMath_aTanTable[(idx) % TAN_TABLE_SIZE]
#define TAN_TABLE_GET_REVERSE(idx) stdMath_aTanTable[TAN_TABLE_LAST_IDX - ((idx) % TAN_TABLE_SIZE)]

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
        return -(360.0f - normAngle);
    return normAngle;
}

// TODO: Remove
void stdMath_SinCos2(float angle, float* pSinOut, float* pCosOut)
{
    float normalized; // st7
    float v4; // st7
    float v5; // st7
    float v6; // [esp+Ch] [ebp-20h]
    float a1; // [esp+10h] [ebp-1Ch]
    int32_t v8; // [esp+14h] [ebp-18h]
    float v9; // [esp+18h] [ebp-14h]
    float v10; // [esp+18h] [ebp-14h]
    float v11; // [esp+18h] [ebp-14h]
    float v12; // [esp+18h] [ebp-14h]
    float v13; // [esp+18h] [ebp-14h]
    int32_t quantized; // [esp+1Ch] [ebp-10h]
    int32_t quantized_plus1; // [esp+20h] [ebp-Ch]
    float normalized_; // [esp+24h] [ebp-8h]
    float v17; // [esp+28h] [ebp-4h]
    float v18; // [esp+28h] [ebp-4h]
    float v19; // [esp+28h] [ebp-4h]
    float v20; // [esp+28h] [ebp-4h]
    float v21; // [esp+28h] [ebp-4h]
    float v22; // [esp+28h] [ebp-4h]
    float v23; // [esp+28h] [ebp-4h]
    float v24; // [esp+28h] [ebp-4h]

    //_stdMath_SinCos(angle, pSinOut, pCosOut);
    //return;

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
    a1 = normalized_ * 45.511112f;
    v6 = a1 - floorf(a1);
    quantized = (int32_t)a1;

    quantized_plus1 = quantized + 1;
    switch ( v8 )
    {
        case 0:
            if ( quantized_plus1 < 4096 )
                v17 = stdMath_aSinTable[quantized_plus1];
            else
                v17 = stdMath_aSinTable[4095 - (quantized - 4095)];
            *pSinOut = (v17 - stdMath_aSinTable[quantized]) * v6 + stdMath_aSinTable[quantized];
            if ( quantized_plus1 < 4096 )
                v18 = stdMath_aSinTable[4095 - quantized_plus1];
            else
                v18 = -stdMath_aSinTable[quantized_plus1 - 0x1000];
            *pCosOut = (v18 - stdMath_aSinTable[4095 - quantized]) * v6 + stdMath_aSinTable[4095 - quantized];
            break;
        case 1:
            if ( quantized_plus1 < 0x2000 )
                v19 = stdMath_aSinTable[4095 - (quantized - 4095)];
            else
                v19 = -stdMath_aSinTable[quantized_plus1 - 0x2000];
            v9 = stdMath_aSinTable[4095 - (quantized - 4096)];
            *pSinOut = (v19 - v9) * v6 + v9;
            if ( quantized_plus1 < 0x2000 )
                v4 = -stdMath_aSinTable[quantized_plus1 - 0x1000];
            else
                v4 = -stdMath_aSinTable[4095 - (quantized - 0x1FFF)];
            v20 = v4;
            v10 = -stdMath_aSinTable[quantized - 0x1000];
            *pCosOut = (v20 - v10) * v6 + v10;
            break;
        case 2:
            if ( quantized_plus1 < 0x3000 )
                v5 = -stdMath_aSinTable[quantized_plus1 - 0x2000];
            else
                v5 = -stdMath_aSinTable[4095 - (quantized - 12287)];
            v21 = v5;
            v11 = -stdMath_aSinTable[quantized - 0x2000];
            *pSinOut = (v21 - v11) * v6 + v11;
            if ( quantized_plus1 < 0x3000 )
                v22 = -stdMath_aSinTable[4095 - (quantized - 0x1FFF)];
            else
                v22 = stdMath_aSinTable[quantized_plus1 - 0x3000];
            v12 = -stdMath_aSinTable[4095 - (quantized - 0x2000)];
            *pCosOut = (v22 - v12) * v6 + v12;
            break;
        case 3:
            if ( quantized_plus1 < 0x4000 )
                v23 = -stdMath_aSinTable[4095 - (quantized - 0x2FFF)];
            else
                v23 = stdMath_aSinTable[quantized_plus1 - 0x4000];
            v13 = -stdMath_aSinTable[4095 - (quantized - 0x3000)];
            *pSinOut = (v23 - v13) * v6 + v13;
            if ( quantized_plus1 < 0x4000 )
                v24 = stdMath_aSinTable[quantized_plus1 - 0x3000];
            else
                v24 = stdMath_aSinTable[4095 - (quantized - 0x3FFF)];
            *pCosOut = (v24 - stdMath_aSinTable[quantized - 0x3000]) * v6 + stdMath_aSinTable[quantized - 0x3000];
            break;
        default:
            return;
    }
}
// TODO: Remove

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

    int32_t quadrant = 0;
    if ( normAngle >= 270.0f )
        quadrant = 3;
    else if ( normAngle >= 180.0f )
        quadrant = 2;
    else if ( normAngle >= 90.0f )
        quadrant = 1;

    float indexFloat = normAngle * 45.511112f; // SIN_TABLE_SIZE / 90
    float fracPart = indexFloat - floorf(indexFloat);
    int32_t index = (int32_t)indexFloat;
    int32_t nextIndex = index + 1;

    float sinLookup = 0.0f, sinValue = 0.0f,
        cosLookup=  0.0f, cosValue = 0.0f;
    switch ( quadrant )
    {
        case 0:
        {
            sinLookup = SIN_TABLE_GET(index);
            if ( nextIndex < SIN_TABLE_SIZE )
                sinValue = SIN_TABLE_GET(nextIndex);
            else
                sinValue = SIN_TABLE_GET_REVERSE(index - SIN_TABLE_LAST_IDX);

            cosLookup = SIN_TABLE_GET_REVERSE(index);
            if ( nextIndex < SIN_TABLE_SIZE )
                cosValue = SIN_TABLE_GET_REVERSE(nextIndex);
            else
                cosValue = -SIN_TABLE_GET(nextIndex - SIN_TABLE_SIZE);
        }
        break;

        case 1:
        {
            sinLookup = SIN_TABLE_GET_REVERSE(index - SIN_TABLE_SIZE);
            if ( nextIndex < 2 * SIN_TABLE_SIZE )
                sinValue = SIN_TABLE_GET_REVERSE(index - SIN_TABLE_LAST_IDX);
            else
                sinValue = -SIN_TABLE_GET(nextIndex - 2 * SIN_TABLE_SIZE);

            cosLookup = -SIN_TABLE_GET(index - SIN_TABLE_SIZE);
            if ( nextIndex < 2 * SIN_TABLE_SIZE )
                cosValue = -SIN_TABLE_GET(nextIndex - SIN_TABLE_SIZE);
            else
                cosValue = -SIN_TABLE_GET_REVERSE((index - (2 * SIN_TABLE_SIZE - 1)));
        }
        break;

        case 2:
        {
            sinLookup = -SIN_TABLE_GET(index - 2 * SIN_TABLE_SIZE);
            if ( nextIndex < 3 * SIN_TABLE_SIZE )
                sinValue = -SIN_TABLE_GET(nextIndex - 2 * SIN_TABLE_SIZE);
            else
                sinValue = -SIN_TABLE_GET_REVERSE((index - (3 * SIN_TABLE_SIZE - 1)));

            cosLookup = -SIN_TABLE_GET_REVERSE((index - 2 * SIN_TABLE_SIZE));
            if ( nextIndex < 3 * SIN_TABLE_SIZE )
                cosValue = -SIN_TABLE_GET_REVERSE((index - (2 * SIN_TABLE_SIZE - 1)));
            else
                cosValue = SIN_TABLE_GET(nextIndex - 3 * SIN_TABLE_SIZE);
        }
        break;

        case 3:
        {
            sinLookup = -SIN_TABLE_GET_REVERSE((index - 3 * SIN_TABLE_SIZE));
            if ( nextIndex < 4 * SIN_TABLE_SIZE )
                sinValue = -SIN_TABLE_GET_REVERSE((index - (3 * SIN_TABLE_SIZE - 1)));
            else
                sinValue = SIN_TABLE_GET(nextIndex - 4 * SIN_TABLE_SIZE);

            cosLookup = SIN_TABLE_GET(index - 3 * SIN_TABLE_SIZE);
            if ( nextIndex < 4 * SIN_TABLE_SIZE )
                cosValue = SIN_TABLE_GET(nextIndex - 3 * SIN_TABLE_SIZE);
            else
                cosValue = SIN_TABLE_GET_REVERSE(index - (4 * SIN_TABLE_SIZE - 1));
        }
        break;
    }

    *pSinOut = (sinValue - sinLookup) * fracPart + sinLookup;
    *pCosOut = (cosValue - cosLookup) * fracPart + cosLookup;

    // TEST scope remove
#ifdef J3D_DEBUG
    float sn, css;
    stdMath_SinCos2(angle, &sn, &css); // TODO: remove
    if ( sn != *pSinOut || css != *pCosOut )
    {
        STDLOG_ERROR("SinCos result differ from original for angle: %.f. sin=%.f osin=%.f cos=%.f ocos=%.f", angle, *pSinOut, sn, *pCosOut, css);
    }
    STD_ASSERT(sn == *pSinOut && css == *pCosOut);
#endif
// TEST scope remove
}

// TODO: Remove
float stdMath_Tan2(float a1)
{
    double v1; // st7
    float v3; // [esp+Ch] [ebp-20h]
    float a1a; // [esp+10h] [ebp-1Ch]
    int32_t v5; // [esp+14h] [ebp-18h]
    float v6; // [esp+18h] [ebp-14h]
    float v7; // [esp+18h] [ebp-14h]
    int32_t v8; // [esp+1Ch] [ebp-10h]
    float v9; // [esp+20h] [ebp-Ch]
    int32_t v10; // [esp+24h] [ebp-8h]
    float v11; // [esp+28h] [ebp-4h]
    float v12; // [esp+28h] [ebp-4h]
    float v13; // [esp+28h] [ebp-4h]
    float v14; // [esp+28h] [ebp-4h]
    float v15; // [esp+34h] [ebp+8h]

    v1 = stdMath_NormalizeAngle(a1);
    v15 = v1;
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
    a1a = v15 / 360.0 * 16384.0;
    v3 = a1a - floorf(a1a);
    v8 = (__int64)a1a;
    v10 = v8 + 1;
    switch ( v5 )
    {
        case 0:
            if ( v10 < 0x1000 )
                v11 = stdMath_aTanTable[v10];
            else
                v11 = -stdMath_aTanTable[0xFFF - (v8 - 0xFFF)];
            v9 = (v11 - stdMath_aTanTable[v8]) * v3 + stdMath_aTanTable[v8];
            break;
        case 1:
            if ( v10 < 0x2000 )
                v12 = -stdMath_aTanTable[0xFFF - (v8 - 0xFFF)];
            else
                v12 = stdMath_aTanTable[v10 - 0x2000];
            v6 = -stdMath_aTanTable[0xFFF - (v8 - 0x1000)];
            v9 = (v12 - v6) * v3 + v6;
            break;
        case 2:
            if ( v10 < 0x3000 )
                v13 = stdMath_aTanTable[v10 - 0x3000];
            else
                v13 = -stdMath_aTanTable[0xFFF - (v8 - 0x2FFF)];
            v9 = (v13 - stdMath_aTanTable[0xFFF - (v8 - 0x2000)]) * v3 + stdMath_aTanTable[0xFFF - (v8 - 0x2000)];
            break;
        case 3:
            if ( v10 < 0x4000 )
                v14 = -stdMath_aTanTable[0xFFF - (v8 - 0x2FFF)];
            else
                v14 = stdMath_aTanTable[v10 - 0x4000];
            v7 = -stdMath_aTanTable[0xFFF - (v8 - 0x3000)];
            v9 = (v14 - v7) * v3 + v7;
            break;
        default:
            v9 = 0.0; // added
            return v9;
    }
    return v9;
}
// TODO: Remove

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

    int32_t quadrant = 0;
    if ( normAngle >= 270.0f )
        quadrant = 3;
    else if ( normAngle >= 180.0f )
        quadrant = 2;
    else if ( normAngle >= 90.0f )
        quadrant = 1;

    float indexFloat = normAngle / 360.0f * 16384.0f; // 16384.0f - 4 * TAN_TABLE_SIZE
    float fracPart = indexFloat - floorf(indexFloat);
    int32_t index = (int32_t)indexFloat;
    int32_t nextIndex = index + 1;

    float tanValue = 0.0f, baseLookup = 0.0f;
    switch ( quadrant )
    {
        case 0:
        {
            baseLookup = TAN_TABLE_GET(index);
            if ( nextIndex < TAN_TABLE_SIZE )
                tanValue = TAN_TABLE_GET(nextIndex);
            else
                tanValue = -TAN_TABLE_GET_REVERSE((index - TAN_TABLE_LAST_IDX));
        }
        break;

        case 1:
        {
            baseLookup = -TAN_TABLE_GET_REVERSE((index - TAN_TABLE_SIZE));
            if ( nextIndex < 2 * TAN_TABLE_SIZE )
                tanValue = -TAN_TABLE_GET_REVERSE((index - TAN_TABLE_LAST_IDX));
            else
                tanValue = TAN_TABLE_GET(nextIndex - 2 * TAN_TABLE_SIZE);
        }
        break;

        case 2:
        {
            baseLookup = TAN_TABLE_GET_REVERSE((index - 2 * TAN_TABLE_SIZE));
            if ( nextIndex < 3 * TAN_TABLE_SIZE )
                tanValue = TAN_TABLE_GET(nextIndex - 2 * TAN_TABLE_SIZE);
            else
                tanValue = -TAN_TABLE_GET_REVERSE((index - (3 * TAN_TABLE_SIZE - 1)));
        }
        break;

        case 3:
        {
            baseLookup = -TAN_TABLE_GET_REVERSE((index - 3 * TAN_TABLE_SIZE));
            if ( nextIndex < 4 * TAN_TABLE_SIZE )
                tanValue = -TAN_TABLE_GET_REVERSE((index - (3 * TAN_TABLE_SIZE - 1)));
            else
                tanValue = TAN_TABLE_GET(nextIndex - 4 * TAN_TABLE_SIZE);
        }
        break;
    }

    // TODO: Remove
#ifdef J3D_DEBUG
    float tan = ((tanValue - baseLookup) * fracPart + baseLookup);
    float otan = stdMath_Tan2(angle);
    if ( tan != otan )
    {
        STDLOG_ERROR("stdMath_Tan2 result differ from original for angle: %.f. tan=%.f otan=%.f", angle, tan, otan);
    }
    STD_ASSERT(tan == otan);
#endif
// TODO: Remove
    return (tanValue - baseLookup) * fracPart + baseLookup;
}

float J3DAPI stdMath_ArcSin1(float num)
{
    // Fixed: Clamp input before sqrt(1 - x*x) to avoid NaN from tiny overshoots.
    num = stdMath_ClampArcTrigInput(num);

    double asinval;
    double absNum = fabs(num);
    if ( absNum <= M_SQRT1_2 )
    {
        asinval = STDMATH_TODEGREES(pow(absNum, 3) * 0.212749 + absNum);
    }
    else
    {
        double sqrtComplement = sqrt(1.0 - absNum * absNum);
        asinval = 90.0 - STDMATH_TODEGREES(pow(sqrtComplement, 3) * 0.212749 + sqrtComplement);
    }

    return (float)(num < 0.0 ? -asinval : asinval);
}

float J3DAPI stdMath_ArcSin2(float num)
{
    // Fixed: Clamp input before sqrt(1 - x*x) to avoid NaN from tiny overshoots.
    num = stdMath_ClampArcTrigInput(num);

    double asinval;
    double absnum = fabs(num);
    if ( absnum <= M_SQRT1_2 )
    {
        double term1 = pow(absnum, 3) / 6.0 + absnum;
        asinval = STDMATH_TODEGREES(pow(absnum, 5) * 0.105502 + term1);
    }
    else
    {
        double sqrtComplement = sqrt(1.0 - absnum * absnum);
        double term1 = pow(sqrtComplement, 3) / 6.0 + sqrtComplement;
        asinval = 90.0 - STDMATH_TODEGREES(pow(sqrtComplement, 5) * 0.105502 + term1);
    }

    return (float)(num < 0.0 ? -asinval : asinval);
}

float J3DAPI stdMath_ArcSin3(float num)
{
    // Fixed: Clamp input before sqrt(1 - x*x) to avoid NaN from tiny overshoots.
    num = stdMath_ClampArcTrigInput(num);

    double asinval = fabs(num);
    if ( asinval <= M_SQRT1_2 )
    {
        double term1 = pow(asinval, 3) / 6.0 + asinval;
        double term2 = pow(asinval, 5) * 0.075000003 + term1;
        asinval = STDMATH_TODEGREES(pow(asinval, 7) * 0.066797003 + term2);
    }
    else
    {
        double sqrtVal = sqrt(1.0 - asinval * asinval);
        double term = pow(sqrtVal, 3) / 6.0 + sqrtVal;
        term        = pow(sqrtVal, 5) * 0.075000003 + term;
        asinval     = 90.0 - STDMATH_TODEGREES(pow(sqrtVal, 7) * 0.066797003 + term);
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

    double angle = ratio - pow(ratio, 3) / 3.0;
    angle = pow(ratio, 5) / 5.0 + angle;
    angle = angle - pow(ratio, 7) / 7.0;
    angle = STDMATH_TODEGREES(pow(ratio, 9) * 0.063235f + angle);

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
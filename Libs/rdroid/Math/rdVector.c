#include "rdMatrix.h"
#include "rdVector.h"

#include <j3dcore/j3dhook.h>
#include <rdroid/Main/rdroid.h>
#include <rdroid/RTI/symbols.h>

const rdVector2 rdroid_zeroVector2 = { 0.0f,  0.0f };
const rdVector2 rdroid_xVector2    = { 1.0f,  0.0f };
const rdVector2 rdroid_yVector2    = { 0.0f,  1.0f };

const rdVector3 rdroid_g_zeroVector3 = { 0.0f, 0.0f, 0.0f };
const rdVector3 rdroid_g_xVector3    = { 1.0f, 0.0f, 0.0f };
const rdVector3 rdroid_g_yVector3    = { 0.0f, 1.0f, 0.0f };
const rdVector3 rdroid_g_zVector3    = { 0.0f, 0.0f, 1.0f };


void rdVector_InstallHooks(void)
{
    J3D_HOOKFUNC(rdVector_Set3);
    J3D_HOOKFUNC(rdVector_Set4);
    J3D_HOOKFUNC(rdVector_Neg3);
    J3D_HOOKFUNC(rdVector_Add3Acc);
    J3D_HOOKFUNC(rdVector_Dot3);
    J3D_HOOKFUNC(rdVector_Len3);
    J3D_HOOKFUNC(rdVector_Normalize3);
    J3D_HOOKFUNC(rdVector_Normalize2Acc);
    J3D_HOOKFUNC(rdVector_Normalize3Acc);
    J3D_HOOKFUNC(rdVector_Normalize3QuickAcc);
    J3D_HOOKFUNC(rdVector_InvScale3);
    J3D_HOOKFUNC(rdVector_Rotate3);
    J3D_HOOKFUNC(rdVector_Rotate3Acc);
}

void rdVector_ResetGlobals(void)
{}

float J3DAPI rdVector_Normalize2(rdVector2* dest, const rdVector2* src)
{
    RD_ASSERTREL(dest != src);
    float len = sqrtf(src->x * src->x + src->y * src->y);
    if ( len == 0.0 )
    {
        dest->x = src->x;
        dest->y = src->y;
    }
    else
    {
        dest->x = (float)(src->x / len);
        dest->y = (float)(src->y / len);
    }

    return (float)len;
}

float J3DAPI rdVector_Normalize3(rdVector3* dest, const rdVector3* src)
{
    RD_ASSERTREL(dest != src);
    float len = sqrtf(src->x * src->x + src->y * src->y + src->z * src->z);
    if ( len == 0.0 )
    {
        dest->x = src->x;
        dest->y = src->y;
        dest->z = src->z;
    }
    else
    {
        dest->x = (float)(src->x / len);
        dest->y = (float)(src->y / len);
        dest->z = (float)(src->z / len);
    }

    return (float)len;
}

float J3DAPI rdVector_Normalize4(rdVector4* dest, const rdVector4* src)
{
    RD_ASSERTREL(dest != src);
    float len = sqrtf(src->x * src->x + src->y * src->y + src->z * src->z + src->w * src->w);
    if ( len == 0.0 )
    {
        dest->x = src->x;
        dest->y = src->y;
        dest->z = src->z;
        dest->w = src->w;
    }
    else
    {
        dest->x = (float)(src->x / len);
        dest->y = (float)(src->y / len);
        dest->z = (float)(src->z / len);
        dest->w = (float)(src->w / len);
    }

    return (float)len;
}

float J3DAPI rdVector_Normalize2Acc(rdVector2* vec)
{
    float magnitude = sqrtf(vec->x * vec->x + vec->y * vec->y);
    if ( magnitude == 0.0 )
    {
        return 0.0f;
    }

    vec->x = (float)(vec->x / magnitude);
    vec->y = (float)(vec->y / magnitude);
    return (float)magnitude;
}

float J3DAPI rdVector_Normalize3Acc(rdVector3* vec)
{
    float len = sqrtf(vec->x * vec->x + vec->y * vec->y + vec->z * vec->z);
    if ( len == 0.0 )
    {
        return 0.0f;
    }

    float invLen = (float)(1.0 / len);
    vec->x = (float)(vec->x * invLen);
    vec->y = (float)(vec->y * invLen);
    vec->z = (float)(vec->z * invLen);
    return (float)len;
}

float J3DAPI rdVector_Normalize3QuickAcc(rdVector3* src)
{
    float x;
    if ( src->x >= 0.0 )
    {
        x = src->x;
    }
    else
    {
        x = -src->x;
    }

    float max = x;
    float y;
    if ( src->y >= 0.0 )
    {
        y = src->y;
    }
    else
    {
        y = -src->y;
    }

    float med = y;
    float z;
    if ( src->z >= 0.0 )
    {
        z = src->z;
    }
    else
    {
        z = -src->z;
    }

    float min = z;
    if ( z <= y )
    {
        if ( x < y )
        {
            med = x;
            max = y;
            if ( z > x )
            {
                min = x;
                med = z;
            }
        }
    }
    else if ( z <= x )
    {
        min = y;
        med = z;
    }
    else
    {
        min = x;
        max = z;
        if ( y < x )
        {
            min = y;
            med = x;
        }
    }

    RD_ASSERTREL((min <= med) && (med <= max));
    float len    = (float)(0.34375 * med + 0.25 * min + max);
    float invLen = (float)(1.0 / len);
    src->x = (float)(src->x * invLen);
    src->y = (float)(src->y * invLen);
    src->z = (float)(src->z * invLen);
    return (float)len;
}

float J3DAPI rdVector_Normalize4Acc(rdVector4* vec)
{
    float len = sqrtf(vec->x * vec->x + vec->y * vec->y + vec->z * vec->z + vec->w * vec->w);
    if ( len == 0.0 )
    {
        return 0.0f;
    }

    float invLen = (float)(1.0 / len);
    vec->x = (float)(vec->x * invLen);
    vec->y = (float)(vec->y * invLen);
    vec->z = (float)(vec->z * invLen);
    vec->w = (float)(vec->w * invLen);
    return (float)len;
}

void J3DAPI rdVector_Rotate3(rdVector3* vec, const rdVector3* pivot, const rdVector3* pyr)
{
    rdMatrix34 mat;
    rdMatrix_BuildRotate34(&mat, pyr);
    rdMatrix_TransformVector34(vec, pivot, &mat);
}

void J3DAPI rdVector_Rotate3Acc(rdVector3* vec, const rdVector3* pyr)
{
    rdMatrix34 mat;
    rdMatrix_BuildRotate34(&mat, pyr);
    rdMatrix_TransformVector34Acc(vec, &mat);
}

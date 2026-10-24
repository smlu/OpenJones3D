#include "rdMathTest.h"

#include <j3dcore/Tests/j3dTest.h>

#include <rdroid/Math/rdMatrix.h>

TEST_GROUP(rdMatrix);

TEST_SETUP(rdMatrix)
{
    J3DTest_Startup();
    J3DTest_BindHostServicesGlobal(rdroid_g_pHS_ADDR);
}

TEST_TEAR_DOWN(rdMatrix)
{
    J3DTest_Shutdown();
}

static void rdMatrixTest_AssertMatrix(const rdMatrix34* pExpected, const rdMatrix34* pActual)
{
    rdMathTest_AssertVector3(&pExpected->rvec, &pActual->rvec);
    rdMathTest_AssertVector3(&pExpected->lvec, &pActual->lvec);
    rdMathTest_AssertVector3(&pExpected->uvec, &pActual->uvec);
    rdMathTest_AssertVector3(&pExpected->dvec, &pActual->dvec);
}

// Reference evaluation preserves explicit float stores, denominator, and division.
static void rdMatrixTest_ReferenceAxisAngle(rdMatrix34* mat, const rdVector3* vec, float angle)
{
    float angSin, angCos;
    stdMath_SinCos(angle, &angSin, &angCos);
    if ( vec->z >= 1.0f || vec->z <= -1.0f )
    {
        float orientedSin = vec->z >= 1.0f ? angSin : -angSin;
        mat->rvec = (rdVector3){ angCos, orientedSin, 0.0f };
        mat->lvec = (rdVector3){ -orientedSin, angCos, 0.0f };
        mat->uvec = (rdVector3){ 0.0f, 0.0f, 1.0f };
    }
    else
    {
        float v17 = (float)(1.0 - angCos);
        float yx  = vec->y * vec->x;
        float zy  = vec->z * vec->y;
        float zx  = vec->z * vec->x;
        float v7  = angSin * vec->x;
        float v6  = vec->y * angSin;
        float v4  = vec->z * angSin;
        float v11 = vec->x * vec->x;
        float v5  = vec->y * vec->y;
        float v18 = (float)(1.0 - v11 - v5);
        float v12 = (float)(1.0 - v18);
        float v9  = angCos * v11;
        float v3  = angCos * v5;
        float v10 = v17 * zx;
        float v8  = v17 * zy;
        mat->rvec.x = (v9 * v18 + v3) / v12 + v11;
        mat->lvec.y = (v3 * v18 + v9) / v12 + v5;
        mat->uvec.z = v18 + v9 + v3;
        mat->rvec.y = v17 * yx + v4;
        mat->lvec.x = v17 * yx - v4;
        mat->rvec.z = v10 - v6;
        mat->lvec.z = v8 + v7;
        mat->uvec.x = v10 + v6;
        mat->uvec.y = v8 - v7;
    }
    mat->dvec = (rdVector3){ 0.0f, 0.0f, 0.0f };
}

// Keep pre-comparison double
// quotients distinct from float arcsine inputs and the stored float lengths.
static void rdMatrixTest_ReferenceExtractAngles(const rdMatrix34* mat, rdVector3* pyr)
{
    rdVector3 v16 = mat->lvec;
    float v9     = -mat->rvec.x;
    float v10    = -mat->rvec.y;
    float v11    = -mat->rvec.z;
    float v5     = (float)(v16.x * v16.x + v16.y * v16.y + 0.0 * 0.0);
    double v2    = sqrtf(v5);
    float v15    = (float)v2;
    if ( v2 >= 0.001 )
    {
        float v7  = v16.y / v15;
        float v18 = (float)(90.0 - stdMath_ArcSin3(v7));
        pyr->y = v16.x > 0.0f ? -v18 : v18;
    }
    else
    {
        float v6  = -v9;
        float v17 = (float)(90.0 - stdMath_ArcSin3(v6));
        if ( (v10 > 0.0 && v16.z > 0.0) || (v10 < 0.0 && v16.z < 0.0) )
        {
            v17 = -v17;
        }
        pyr->z = v17;
        pyr->y = 0.0f;
    }
    if ( v15 >= 0.001 )
    {
        double v3 = (v16.x * v16.x + v16.y * v16.y + 0.0 * v16.z) / v15;
        pyr->x = v3 < 1.0 ? (float)(90.0 - stdMath_ArcSin3((float)v3)) : 0.0f;
    }
    else
    {
        pyr->x = 90.0f;
    }
    if ( v16.z < 0.0f )
    {
        pyr->x = -pyr->x;
    }

    float v21 = -v16.y;
    float v8  = (float)(v21 * v21 + v16.x * v16.x + 0.0 * 0.0);
    float v14 = sqrtf(v8);
    if ( v15 >= 0.001 )
    {
        double v4 = (v21 * v9 + v16.x * v10 + 0.0 * v11) / v14;
        float v13 = (float)v4;
        pyr->z = v4 < 1.0 ? (v13 > -1.0f ? (float)(90.0 - stdMath_ArcSin3(v13)) : 180.0f) : 0.0f;
        if ( v11 < 0.0f )
        {
            pyr->z = -pyr->z;
        }
    }
}

TEST(rdMatrix, TestAxisAnglePrecision)
{
    // Include principal axes and oblique axes: the latter expose the changed
    // zz rounding and reciprocal-multiply rewrite, not just trivial rotations.
    const rdVector3 aAxes[] = {
        { 1.0f, 0.0f, 0.0f }, { 0.0f, 1.0f, 0.0f },
        { 0.0f, 0.0f, 1.0f }, { 0.0f, 0.0f, -1.0f },
        { 0.3f, 0.4f, 0.8660254f }, { -0.3f, 0.4f, -0.8660254f },
        { 0.70710677f, 0.70710677f, 0.0f },
        { 0.1f, -0.7f, 0.70710677f },
    };
    const float aAngles[] = { 0.0f, 1.0f, -1.0f, 30.0f, 45.0f, 90.0f, -90.0f, 123.5f, 180.0f, 270.0f, 359.5f };
    for ( size_t i = 0; i < STD_ARRAYLEN(aAxes); ++i )
    {
        for ( size_t j = 0; j < STD_ARRAYLEN(aAngles); ++j )
        {
            rdMatrix34 expected;
            rdMatrix34 actual;
            rdMatrixTest_ReferenceAxisAngle(&expected, &aAxes[i], aAngles[j]);
            rdMatrix_BuildFromVectorAngle34(&actual, &aAxes[i], aAngles[j]);
            rdMatrixTest_AssertMatrix(&expected, &actual);
        }
    }
}

TEST(rdMatrix, TestNearAxisGuards)
{
    // These intentional OpenJones3D fixes predate the precision correction.
    // Keep near-pole inputs on the safe planar branch instead of dividing by zero.
    const float aZ[] = { 1.0f, nextafterf(1.0f, 0.0f), -1.0f, nextafterf(-1.0f, 0.0f) };
    float sine, cosine;
    stdMath_SinCos(45.0f, &sine, &cosine);
    for ( size_t i = 0; i < STD_ARRAYLEN(aZ); ++i )
    {
        rdVector3 axis      = { 0.000001f, 0.0f, aZ[i] };
        float orientedSin   = aZ[i] > 0.0f ? sine : -sine;
        rdMatrix34 expected = {
            { cosine, orientedSin, 0.0f }, { -orientedSin, cosine, 0.0f },
            { 0.0f, 0.0f, 1.0f }, { 0.0f, 0.0f, 0.0f }
        };
        rdMatrix34 actual;
        rdMatrix_BuildFromVectorAngle34(&actual, &axis, 45.0f);
        rdMatrixTest_AssertMatrix(&expected, &actual);
    }
}

TEST(rdMatrix, TestExtractAnglesPrecision)
{
    // Rotations exercise pitch/yaw/roll quadrants and signs. BuildRotate34
    // only supplies inputs; the expected extraction comes from the reference evaluation.
    const float aAngles[] = { -179.5f, -90.0f, -45.0f, -0.01f, 0.0f, 0.01f, 45.0f, 90.0f, 179.5f };
    for ( size_t i = 0; i < STD_ARRAYLEN(aAngles); ++i )
    {
        for ( size_t j = 0; j < STD_ARRAYLEN(aAngles); ++j )
        {
            for ( size_t k = 0; k < STD_ARRAYLEN(aAngles); ++k )
            {
                rdVector3 rotation = { aAngles[i], aAngles[j], aAngles[k] };
                rdMatrix34 input;
                rdMatrix_BuildRotate34(&input, &rotation);
                rdVector3 expected;
                rdVector3 actual;
                rdMatrixTest_ReferenceExtractAngles(&input, &expected);
                rdMatrix_ExtractAngles34(&input, &actual);
                rdMathTest_AssertVector3(&expected, &actual);
            }
        }
    }
}

TEST(rdMatrix, TestExtractAngleBoundaries)
{
    // Check both sides of the 0.001 singularity and the +/-1 roll clamps,
    // including the original degenerate zero-matrix fallback.
    const rdVector3 aForward[] = {
        { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 1.0f }, { 0.0f, 0.0f, -1.0f },
        { 0.0009999999f, 0.0f, 1.0f }, { 0.001f, 0.0f, -1.0f },
        { 0.0010000002f, 0.0f, 1.0f }, { 0.3f, 0.4f, 0.8660254f },
        { 1.0f, 0.0f, 0.0f }, { 0.3f, 0.4f, INFINITY },
    };
    const rdVector3 aRight[] = {
        { 1.0f, 0.0f, 0.0f }, { -1.0f, 0.0f, -0.0f },
        { 0.0f, 1.0f, 0.1f }, { 0.0f, -1.0f, -0.1f },
        { 0.0f, 2.0f, 0.0f }, { 0.0f, -2.0f, 0.0f },
    };
    for ( size_t i = 0; i < STD_ARRAYLEN(aForward); ++i )
    {
        for ( size_t j = 0; j < STD_ARRAYLEN(aRight); ++j )
        {
            rdMatrix34 input = { 0 };
            input.lvec = aForward[i];
            input.rvec = aRight[j];
            rdVector3 expected;
            rdVector3 actual;
            rdMatrixTest_ReferenceExtractAngles(&input, &expected);
            rdMatrix_ExtractAngles34(&input, &actual);
            rdMathTest_AssertVector3(&expected, &actual);
        }
    }
}

TEST_GROUP_RUNNER(rdMatrix)
{
    RUN_TEST_CASE(rdMatrix, TestAxisAnglePrecision);
    RUN_TEST_CASE(rdMatrix, TestNearAxisGuards);
    RUN_TEST_CASE(rdMatrix, TestExtractAnglesPrecision);
    RUN_TEST_CASE(rdMatrix, TestExtractAngleBoundaries);
}

#include "rdMathTest.h"

#include <j3dcore/Tests/j3dTest.h>

#include <rdroid/Math/rdMatrix.h>

// Identity matrix redirected in rdMathTestHost.h
const rdMatrix34 rdMathTest_identMatrix34 = {
    { 1.0f, 0.0f, 0.0f }, { 0.0f, 1.0f, 0.0f }, { 0.0f, 0.0f, 1.0f }, { 0.0f, 0.0f, 0.0f }
};

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

// Rotation matrix tests against double precision sin and cos. They also cover stdMath_SinCos through its main users:
// rdMatrix_BuildRotate34 turns things every frame (physics angular velocity, AI turns, rdMatrix_PreRotate34/PostRotate34).

// stdMath_SinCos is within 1e-6 of sin and cos; a matrix element sums products of up to three of them, so 4e-6 bounds it.
// The original stdMath_SinCos mirror error was up to 3.8e-4.
#define RDMATRIX_TEST_ROTATE_TOLERANCE 4e-6f

// rdMatrix_BuildRotate34 formula with double precision sin and cos
static void rdMatrixTest_StdLibRotate34(rdMatrix34* pMat, const rdVector3* pPyr)
{
    const double toRadians = 3.14159265358979323846 / 180.0;
    const double xs = sin(pPyr->x * toRadians), xc = cos(pPyr->x * toRadians);
    const double ys = sin(pPyr->y * toRadians), yc = cos(pPyr->y * toRadians);
    const double zs = sin(pPyr->z * toRadians), zc = cos(pPyr->z * toRadians);

    pMat->rvec = (rdVector3){ (float)(-(ys * zs) * xs + yc * zc), (float)((yc * zs) * xs + ys * zc), (float)(-zs * xc) };
    pMat->lvec = (rdVector3){ (float)(-ys * xc), (float)(yc * xc), (float)xs };
    pMat->uvec = (rdVector3){ (float)((ys * zc) * xs + yc * zs), (float)(-xs * (yc * zc) + ys * zs), (float)(zc * xc) };
    pMat->dvec = (rdVector3){ 0.0f, 0.0f, 0.0f };
}

static void rdMatrixTest_AssertVector3Near(float tolerance, const rdVector3* pExpected, const rdVector3* pActual)
{
    TEST_ASSERT_FLOAT_WITHIN(tolerance, pExpected->x, pActual->x);
    TEST_ASSERT_FLOAT_WITHIN(tolerance, pExpected->y, pActual->y);
    TEST_ASSERT_FLOAT_WITHIN(tolerance, pExpected->z, pActual->z);
}

static void rdMatrixTest_AssertRotationNear(float tolerance, const rdMatrix34* pExpected, const rdMatrix34* pActual)
{
    rdMatrixTest_AssertVector3Near(tolerance, &pExpected->rvec, &pActual->rvec);
    rdMatrixTest_AssertVector3Near(tolerance, &pExpected->lvec, &pActual->lvec);
    rdMatrixTest_AssertVector3Near(tolerance, &pExpected->uvec, &pActual->uvec);
}

TEST(rdMatrix, TestBuildRotate34MatchesStdLib)
{
    // Every pitch, yaw and roll combination in 15 degree steps over a full turn, then small per-frame steps both ways
    for ( int32_t pitch = -12; pitch <= 12; ++pitch )
    {
        for ( int32_t yaw = -12; yaw <= 12; ++yaw )
        {
            for ( int32_t roll = -12; roll <= 12; ++roll )
            {
                rdVector3 pyr = { (float)pitch * 15.0f, (float)yaw * 15.0f, (float)roll * 15.0f };
                rdMatrix34 expected;
                rdMatrix34 actual;
                rdMatrixTest_StdLibRotate34(&expected, &pyr);
                rdMatrix_BuildRotate34(&actual, &pyr);
                rdMatrixTest_AssertRotationNear(RDMATRIX_TEST_ROTATE_TOLERANCE, &expected, &actual);
                rdMathTest_AssertVector3(&rdroid_g_zeroVector3, &actual.dvec);
            }
        }
    }

    const float aSteps[] = { -0.5f, -0.03f, -0.01f, -0.001f, 0.001f, 0.01f, 0.03f, 0.5f };
    for ( size_t i = 0; i < STD_ARRAYLEN(aSteps); ++i )
    {
        const float a = aSteps[i];
        const rdVector3 aPyr[] = { { a, 0.0f, 0.0f }, { 0.0f, a, 0.0f }, { 0.0f, 0.0f, a }, { a, a, a }, { a, -a, 45.0f } };
        for ( size_t j = 0; j < STD_ARRAYLEN(aPyr); ++j )
        {
            rdMatrix34 expected;
            rdMatrix34 actual;
            rdMatrixTest_StdLibRotate34(&expected, &aPyr[j]);
            rdMatrix_BuildRotate34(&actual, &aPyr[j]);
            rdMatrixTest_AssertRotationNear(RDMATRIX_TEST_ROTATE_TOLERANCE, &expected, &actual);
        }
    }
}

TEST(rdMatrix, TestBuild34IsBuildRotate34WithTranslation)
{
    const rdVector3 aPyr[] = { { 0.0f, 0.0f, 0.0f }, { 15.0f, -30.0f, 45.0f }, { -0.03f, 0.03f, -179.5f }, { 90.0f, 270.0f, -90.0f } };
    const rdVector3 pos = { 1.5f, -2.25f, 3.0f };
    for ( size_t i = 0; i < STD_ARRAYLEN(aPyr); ++i )
    {
        rdMatrix34 expected;
        rdMatrix34 actual;
        rdMatrix_BuildRotate34(&expected, &aPyr[i]);
        expected.dvec = pos;
        rdMatrix_Build34(&actual, &aPyr[i], &pos);
        rdMatrixTest_AssertMatrix(&expected, &actual);
    }
}

TEST(rdMatrix, TestSmallRotationStepsAddUp)
{
    // Regression: things turn by a small step every frame. The original stdMath_SinCos lost 0.022 degrees in every
    // negative step, so 9000 steps of -0.01 degrees didn't turn at all and 3000 steps of -0.03 degrees turned 24 instead of 90.
    // The remaining error comes from the table rounding, the same in every step: 0.004 (0.23 degrees) bounds it.
    const float aSteps[] = { 0.01f, 0.03f, 0.5f };
    for ( size_t axis = 0; axis < 3; ++axis )
    {
        for ( int32_t sign = -1; sign <= 1; sign += 2 )
        {
            for ( size_t i = 0; i < STD_ARRAYLEN(aSteps); ++i )
            {
                rdVector3 step   = { 0.0f, 0.0f, 0.0f };
                rdVector3 target = { 0.0f, 0.0f, 0.0f };
                (&step.x)[axis]   = (float)sign * aSteps[i];
                (&target.x)[axis] = (float)sign * 90.0f;

                rdMatrix34 pre;
                rdMatrix34 post;
                rdMatrix_Identity34(&pre);
                rdMatrix_Identity34(&post);
                const int32_t numSteps = (int32_t)(90.0f / aSteps[i] + 0.5f);
                for ( int32_t n = 0; n < numSteps; ++n )
                {
                    rdMatrix_PreRotate34(&pre, &step);
                    rdMatrix_Normalize34(&pre);
                    rdMatrix_PostRotate34(&post, &step);
                    rdMatrix_Normalize34(&post);
                }

                rdMatrix34 expected;
                rdMatrixTest_StdLibRotate34(&expected, &target);
                rdMatrixTest_AssertRotationNear(0.004f, &expected, &pre);
                rdMatrixTest_AssertRotationNear(0.004f, &expected, &post);
            }
        }
    }
}

static float rdMatrixTest_Dot3(const rdVector3* a, const rdVector3* b)
{
    return a->x * b->x + a->y * b->y + a->z * b->z;
}

TEST(rdMatrix, TestBuildFromVectorAngle34RotatesAroundTheAxis)
{
    // Checks that don't depend on the matrix layout: the axis stays fixed, the trace is 1 + 2 cos(angle),
    // the matrix is orthonormal, and the matrix of -angle is the transpose. The formula divides by 1 - z^2,
    // so its float rounding is larger than BuildRotate34's: 2e-5. The original mirror error was up to 3.8e-4.
    const float tolerance = 2e-5f;
    const rdVector3 aAxes[] = {
        { 1.0f, 0.0f, 0.0f }, { 0.0f, 1.0f, 0.0f }, { 0.0f, 0.0f, 1.0f }, { 0.0f, 0.0f, -1.0f },
        { 0.57735026f, 0.57735026f, 0.57735026f }, { 0.3f, -0.4f, 0.8660254f }, { -0.6f, 0.0f, 0.8f },
    };
    const float aAngles[] = { -179.0f, -90.0f, -30.0f, -0.5f, -0.03f, -0.01f, 0.01f, 0.03f, 0.5f, 30.0f, 90.0f, 179.0f };
    for ( size_t i = 0; i < STD_ARRAYLEN(aAxes); ++i )
    {
        const rdVector3* pAxis = &aAxes[i];
        for ( size_t j = 0; j < STD_ARRAYLEN(aAngles); ++j )
        {
            rdMatrix34 m;
            rdMatrix34 inverse;
            rdMatrix_BuildFromVectorAngle34(&m, pAxis, aAngles[j]);
            rdMatrix_BuildFromVectorAngle34(&inverse, pAxis, -aAngles[j]);

            const rdVector3 rotatedAxis = {
                m.rvec.x * pAxis->x + m.lvec.x * pAxis->y + m.uvec.x * pAxis->z,
                m.rvec.y * pAxis->x + m.lvec.y * pAxis->y + m.uvec.y * pAxis->z,
                m.rvec.z * pAxis->x + m.lvec.z * pAxis->y + m.uvec.z * pAxis->z,
            };
            rdMatrixTest_AssertVector3Near(tolerance, pAxis, &rotatedAxis);

            const double radians = (double)aAngles[j] * 3.14159265358979323846 / 180.0;
            TEST_ASSERT_FLOAT_WITHIN(tolerance, (float)(1.0 + 2.0 * cos(radians)), m.rvec.x + m.lvec.y + m.uvec.z);

            TEST_ASSERT_FLOAT_WITHIN(tolerance, 1.0f, rdMatrixTest_Dot3(&m.rvec, &m.rvec));
            TEST_ASSERT_FLOAT_WITHIN(tolerance, 1.0f, rdMatrixTest_Dot3(&m.lvec, &m.lvec));
            TEST_ASSERT_FLOAT_WITHIN(tolerance, 1.0f, rdMatrixTest_Dot3(&m.uvec, &m.uvec));
            TEST_ASSERT_FLOAT_WITHIN(tolerance, 0.0f, rdMatrixTest_Dot3(&m.rvec, &m.lvec));
            TEST_ASSERT_FLOAT_WITHIN(tolerance, 0.0f, rdMatrixTest_Dot3(&m.rvec, &m.uvec));
            TEST_ASSERT_FLOAT_WITHIN(tolerance, 0.0f, rdMatrixTest_Dot3(&m.lvec, &m.uvec));

            // inverse = transpose of m
            const rdVector3 transposedR = { m.rvec.x, m.lvec.x, m.uvec.x };
            const rdVector3 transposedL = { m.rvec.y, m.lvec.y, m.uvec.y };
            const rdVector3 transposedU = { m.rvec.z, m.lvec.z, m.uvec.z };
            rdMatrixTest_AssertVector3Near(tolerance, &transposedR, &inverse.rvec);
            rdMatrixTest_AssertVector3Near(tolerance, &transposedL, &inverse.lvec);
            rdMatrixTest_AssertVector3Near(tolerance, &transposedU, &inverse.uvec);
        }
    }
}

TEST_GROUP_RUNNER(rdMatrix)
{
    RUN_TEST_CASE(rdMatrix, TestAxisAnglePrecision);
    RUN_TEST_CASE(rdMatrix, TestNearAxisGuards);
    RUN_TEST_CASE(rdMatrix, TestExtractAnglesPrecision);
    RUN_TEST_CASE(rdMatrix, TestExtractAngleBoundaries);
    RUN_TEST_CASE(rdMatrix, TestBuildRotate34MatchesStdLib);
    RUN_TEST_CASE(rdMatrix, TestBuild34IsBuildRotate34WithTranslation);
    RUN_TEST_CASE(rdMatrix, TestSmallRotationStepsAddUp);
    RUN_TEST_CASE(rdMatrix, TestBuildFromVectorAngle34RotatesAroundTheAxis);
}

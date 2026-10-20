#include "rdMath.h"
#include <j3dcore/j3dhook.h>

#include <rdroid/Math/rdVector.h>
#include <rdroid/RTI/symbols.h>

#include <std/General/stdMath.h>


void rdMath_InstallHooks(void)
{
    J3D_HOOKFUNC(rdMath_DistancePointToPlane);
    J3D_HOOKFUNC(rdMath_DeltaAngleNormalized);
}

void rdMath_ResetGlobals(void)
{}

float J3DAPI rdMath_DeltaAngleNormalized(const rdVector3* pVectorX, const rdVector3* pVectorY, const rdVector3* pVectorZ)
{
    // Calculate dot product of vectors X and Y
    float dot = rdVector_Dot3(pVectorX, pVectorY);

    // Clamp dot product to [-1, 1] range
    dot = STDMATH_CLAMP(dot, -1.0f, 1.0f);

    // Handle special cases
    if ( dot == 1.0f ) return 0.0f;
    if ( dot == -1.0f ) return 180.0f;

    // Calculate angle using arcsin
    float angle = 90.0f - stdMath_ArcSin1(dot);

    // Calculate the direction of rotation using cross product and dot
    dot = rdVector_CrossDot3(pVectorX, pVectorY, pVectorZ);

    // Return signed angle based on the direction of rotation
    return (dot > 0.0f) ? angle : -angle;
}

void J3DAPI rdMath_CalcSurfaceNormal(rdVector3* pDestNormal, const rdVector3* pVert1, const rdVector3* pVert2, const rdVector3* pVert3)
{
    rdVector3 d31;
    rdVector3 d21;

    d21.x = pVert2->x - pVert1->x;
    d21.y = pVert2->y - pVert1->y;
    d21.z = pVert2->z - pVert1->z;
    rdVector_Normalize3Acc(&d21);

    d31.x = pVert3->x - pVert1->x;
    d31.y = pVert3->y - pVert1->y;
    d31.z = pVert3->z - pVert1->z;
    rdVector_Normalize3Acc(&d31);

    pDestNormal->x = d21.y * d31.z - d21.z * d31.y;
    pDestNormal->y = d21.z * d31.x - d21.x * d31.z;
    pDestNormal->z = d21.x * d31.y - d21.y * d31.x;
    rdVector_Normalize3Acc(pDestNormal);
    rdMath_ClipVector3Acc(pDestNormal, 0.000001f);
}

bool J3DAPI rdMath_PointsCollinear(const rdVector3* p1, const rdVector3* p2, const rdVector3* p3)
{
    rdVector3 d21;
    rdVector_Sub3(&d21, p2, p1);
    rdVector_Normalize3Acc(&d21);

    rdVector3 d31;
    rdVector_Sub3(&d31, p3, p1);
    rdVector_Normalize3Acc(&d31);

    float dot = fabsf(rdVector_Dot3(&d21, &d31));
    return dot >= 0.99900001f && dot <= 1.001f;
}

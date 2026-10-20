#ifndef RDROID_RDMATH_H
#define RDROID_RDMATH_H
#include <j3dcore/j3d.h>
#include <rdroid/types.h>
#include <rdroid/RTI/addresses.h>

#include <std/General/stdMath.h>
#include <std/types.h>

#include <stdbool.h>
#include <math.h>

#include "rdVector.h"

J3D_EXTERN_C_START

void J3DAPI rdMath_CalcSurfaceNormal(rdVector3* pDestNormal, const rdVector3* pVert1, const rdVector3* pVert2, const rdVector3* pVert3); // Added

static inline void J3DAPI rdMath_ClipVector3(rdVector3* out, const rdVector3* vect, float minVal); // Added
static inline void J3DAPI rdMath_ClipVector3Acc(rdVector3* vect, float minVal); // Added (renamed from rdMathClampVector)

static inline void J3DAPI rdMath_ClampVector3(rdVector3* out, const rdVector3* vect, float minVal, float maxVal); // Added
static inline void J3DAPI rdMath_ClampVector3Acc(rdVector3* vect, float minVal, float maxVal); // Added

static inline void J3DAPI rdMath_ClampVector4(rdVector4* out, const rdVector4* vect, float minVal, float maxVal); // Added
static inline void J3DAPI rdMath_ClampVector4Acc(rdVector4* vect, float minVal, float maxVal); // Added

static inline float J3DAPI rdMath_DistancePointToPlane(const rdVector3* pPoint, const rdVector3* pPlaneNormal, const rdVector3* pPointOnPlane);
static inline float J3DAPI rdMath_DeltaAngleNormalizedAbs(const rdVector3* pVectorX, const rdVector3* pVectorY); // Added
float J3DAPI rdMath_DeltaAngleNormalized(const rdVector3* pVectorX, const rdVector3* pVectorY, const rdVector3* pVectorZ);

// Added: not found in original rdMath.c
static inline void J3DAPI rdMath_ProjectPointOntoPlane(rdVector3* dest, const rdVector3* pPoint, const rdVector3* pPlaneNormal, const rdVector3* pPointOnPlane);

// Added: not found in original rdMath.c, but there must be some similar either macro or function that was inlined
static inline void J3DAPI rdMath_ProjectPointOntoPlaneNormalized(rdVector3* dest, const rdVector3* pPoint, const rdVector3* pPlaneNormal, const rdVector3* pPointOnPlane);

bool J3DAPI rdMath_PointsCollinear(const rdVector3* p1, const rdVector3* p2, const rdVector3* p3); // Added

void J3DAPI rdMath_ClipVector3(rdVector3* out, const rdVector3* vect, float minVal)
{
    out->x = (fabsf(vect->x) > minVal) ? vect->x : 0.0f;
    out->y = (fabsf(vect->y) > minVal) ? vect->y : 0.0f;
    out->z = (fabsf(vect->z) > minVal) ? vect->z : 0.0f;
}

void J3DAPI rdMath_ClipVector3Acc(rdVector3* vect, float minVal)
{
    rdMath_ClipVector3(vect, vect, minVal);
}

void J3DAPI rdMath_ClampVector3(rdVector3* out, const rdVector3* vect, float minVal, float maxVal)
{
    out->x = STDMATH_CLAMP(vect->x, minVal, maxVal);
    out->y = STDMATH_CLAMP(vect->y, minVal, maxVal);
    out->z = STDMATH_CLAMP(vect->z, minVal, maxVal);
}

void J3DAPI rdMath_ClampVector3Acc(rdVector3* vect, float minVal, float maxVal)
{
    rdMath_ClampVector3(vect, vect, minVal, maxVal);
}

void J3DAPI rdMath_ClampVector4(rdVector4* out, const rdVector4* vect, float minVal, float maxVal)
{
    out->x = STDMATH_CLAMP(vect->x, minVal, maxVal);
    out->y = STDMATH_CLAMP(vect->y, minVal, maxVal);
    out->z = STDMATH_CLAMP(vect->z, minVal, maxVal);
    out->w = STDMATH_CLAMP(vect->w, minVal, maxVal);
}

void J3DAPI rdMath_ClampVector4Acc(rdVector4* vect, float minVal, float maxVal)
{
    rdMath_ClampVector4(vect, vect, minVal, maxVal);
}

float J3DAPI rdMath_DistancePointToPlane(const rdVector3* pPoint, const rdVector3* pPlaneNormal, const rdVector3* pPointOnPlane)
{
    return (pPoint->x - pPointOnPlane->x) * pPlaneNormal->x
        + (pPoint->y - pPointOnPlane->y) * pPlaneNormal->y
        + (pPoint->z - pPointOnPlane->z) * pPlaneNormal->z;
}

float J3DAPI rdMath_DeltaAngleNormalizedAbs(const rdVector3* pVectorX, const rdVector3* pVectorY)
{
    float dot  = rdVector_Dot3(pVectorX, pVectorY);
    if ( dot == 1.0 )
    {
        return 0.0f;
    }

    return 90.0f - stdMath_ArcSin1(dot);
}

// Added: not found in original rdMath.c
void J3DAPI rdMath_ProjectPointOntoPlane(rdVector3* dest, const rdVector3* pPoint, const rdVector3* pPlaneNormal, const rdVector3* pPointOnPlane)
{
    // Project point onto plane: result = point - normal * ((point - origin) dot normal ) => point - vector projection
    float dist = rdMath_DistancePointToPlane(pPoint, pPlaneNormal, pPointOnPlane);
    rdVector_ScaleAdd3(dest, pPlaneNormal, -dist, pPoint);
}

// Added: not found in original rdMath.c, but there must be some similar either macro or function that was inlined
void J3DAPI rdMath_ProjectPointOntoPlaneNormalized(rdVector3* dest, const rdVector3* pPoint, const rdVector3* pPlaneNormal, const rdVector3* pPointOnPlane)
{
    rdMath_ProjectPointOntoPlane(dest, pPoint, pPlaneNormal, pPointOnPlane);
    rdVector_Normalize3Acc(dest);
}

// Helper hooking functions
void rdMath_InstallHooks(void);
void rdMath_ResetGlobals(void);

J3D_EXTERN_C_END
#endif // RDROID_RDMATH_H

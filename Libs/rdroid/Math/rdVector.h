#ifndef RDROID_RDVECTOR_H
#define RDROID_RDVECTOR_H
#include <j3dcore/j3d.h>
#include <j3dcore/j3dhook.h>

#include <rdroid/types.h>
#include <rdroid/Main/rdroid.h>
#include <rdroid/RTI/addresses.h>

#include <std/General/stdMath.h>
#include <std/General/stdUtil.h>
#include <std/types.h>

#include <math.h>

J3D_EXTERN_C_START

extern const rdVector2 rdroid_zeroVector2;
extern const rdVector2 rdroid_xVector2;
extern const rdVector2 rdroid_yVector2;

extern const rdVector3 rdroid_g_zeroVector3;
extern const rdVector3 rdroid_g_xVector3;
extern const rdVector3 rdroid_g_yVector3;
extern const rdVector3 rdroid_g_zVector3;

static inline bool J3DAPI rdVector_IsZero2(const rdVector2* v); // Added
static inline bool J3DAPI rdVector_IsZero3(const rdVector3* v); // Added
static inline bool J3DAPI rdVector_IsZero4(const rdVector4* v); // Added

static inline bool J3DAPI rdVector_IsNAN2(const rdVector2* v); // Added
static inline bool J3DAPI rdVector_IsNAN3(const rdVector3* v); // Added
static inline bool J3DAPI rdVector_IsNAN4(const rdVector4* v); // Added

static inline bool J3DAPI rdVector_Equal2(const rdVector2* a, const rdVector2* b); // Added
static inline bool J3DAPI rdVector_Equal3(const rdVector3* a, const rdVector3* b); // Added
static inline bool J3DAPI rdVector_Equal4(const rdVector4* a, const rdVector4* b); // Added

static inline void J3DAPI rdVector_Zero2(rdVector2* dest); // Added
static inline void J3DAPI rdVector_Zero3(rdVector3* dest); // Added
static inline void J3DAPI rdVector_Zero4(rdVector4* dest); // Added

static inline void J3DAPI rdVector_Set2(rdVector2* dest, float x, float y); // Added
static inline void J3DAPI rdVector_Set3(rdVector3* dest, float x, float y, float z);
static inline void J3DAPI rdVector_Set4(rdVector4* dest, float x, float y, float z, float w);

static inline void J3DAPI rdVector_Copy2(rdVector2* dest, const rdVector2* src); // Added from debug
static inline void J3DAPI rdVector_Copy2List(rdVector2* dest, const rdVector2* src, size_t num); // Added

static inline void J3DAPI rdVector_Copy3(rdVector3* dest, const rdVector3* src); // Added from debug
static inline void J3DAPI rdVector_Copy3List(rdVector3* dest, const rdVector3* src, size_t num); // Added

static inline void J3DAPI rdVector_Copy4(rdVector4* dest, const rdVector4* src); // Added from debug
static inline void J3DAPI rdVector_Copy4List(rdVector4* dest, const rdVector4* src, size_t num); // Added

static inline void J3DAPI rdVector_Neg2(rdVector2* dest, const rdVector2* src); // Added from debug version
static inline void J3DAPI rdVector_Neg3(rdVector3* dest, const rdVector3* src);
static inline void J3DAPI rdVector_Neg4(rdVector4* dest, const rdVector4* src); // Added from debug version

#define RDVECTOR_NEG3(src) (rdVector3){ -(src).x, -(src).y, -(src).z } // Added

static inline void J3DAPI rdVector_Neg2Acc(rdVector2* a); // Added from debug version
static inline void J3DAPI rdVector_Neg3Acc(rdVector3* a); // Added
static inline void J3DAPI rdVector_Neg4Acc(rdVector4* a); // Added from debug version

static inline void J3DAPI rdVector_Add2(rdVector2* c, const rdVector2* a, const rdVector2* b); // Added from debug version
static inline void J3DAPI rdVector_Add3(rdVector3* c, const rdVector3* a, const rdVector3* b); // Added
static inline void J3DAPI rdVector_Add4(rdVector4* c, const rdVector4* a, const rdVector4* b); // Added

static inline void J3DAPI rdVector_Add2Acc(rdVector2* a, const rdVector2* b); // Added from debug version
static inline void J3DAPI rdVector_Add3Acc(rdVector3* a, const rdVector3* b);
static inline void J3DAPI rdVector_Add4Acc(rdVector4* a, const rdVector4* b); // Added

static inline void J3DAPI rdVector_Sub2(rdVector2* c, const rdVector2* a, const rdVector2* b); // Added from debug version
static inline void J3DAPI rdVector_Sub3(rdVector3* c, const rdVector3* a, const rdVector3* b); // Added
static inline void J3DAPI rdVector_Sub4(rdVector4* c, const rdVector4* a, const rdVector4* b); // Added from debug version

static inline void J3DAPI rdVector_Sub2Acc(rdVector2* a, const rdVector2* b); // Added from debug version
static inline void J3DAPI rdVector_Sub3Acc(rdVector3* a, const rdVector3* b); // Added
static inline void J3DAPI rdVector_Sub4Acc(rdVector4* a, const rdVector4* b);  // Added from debug version

static inline void J3DAPI rdVector_Scale2(rdVector2* dest, const rdVector2* src, float scalar); // Added from debug version
static inline void J3DAPI rdVector_Scale3(rdVector3* dest, const rdVector3* src, float scalar); // Added from debug version
static inline void J3DAPI rdVector_Scale4(rdVector4* dest, const rdVector4* src, float scalar); // Added from debug version

static inline void J3DAPI rdVector_Scale2Acc(rdVector2* v, float scalar); // Added from debug version
static inline void J3DAPI rdVector_Scale3Acc(rdVector3* v, float scalar); // Added from debug version
static inline void J3DAPI rdVector_Scale4Acc(rdVector4* v, float scalar); // Added from debug version

static inline void J3DAPI rdVector_InvScale2(rdVector2* dest, const rdVector2* src, float scalar); // Added from debug version
static inline void J3DAPI rdVector_InvScale3(rdVector3* dest, const rdVector3* src, float scalar);
static inline void J3DAPI rdVector_InvScale4(rdVector4* dest, const rdVector4* src, float scalar); // Added from debug version

static inline void J3DAPI rdVector_InvScale2Acc(rdVector2* v, float scalar); // Added from debug version
static inline void J3DAPI rdVector_InvScale3Acc(rdVector3* v, float scalar); // Added from debug version
static inline void J3DAPI rdVector_InvScale4Acc(rdVector4* v, float scalar); // Added from debug version

static inline void J3DAPI rdVector_ScaleAdd2(rdVector2* dest, const rdVector2* src, float scalar, const rdVector2* add); // Added
static inline void J3DAPI rdVector_ScaleAdd3(rdVector3* dest, const rdVector3* src, float scalar, const rdVector3* add); // Added
static inline void J3DAPI rdVector_ScaleAdd4(rdVector4* dest, const rdVector4* src, float scalar, const rdVector4* add); // Added

// Helper functions for scaling destination vector by scalar and adding another vector; i.e. destvec = destvec * scalar + addvec
// To not be confused with rdVector_MultAccX
static inline void J3DAPI rdVector_ScaleAdd2Acc(rdVector2* dest, float scalar, const rdVector2* add); // Added
static inline void J3DAPI rdVector_ScaleAdd3Acc(rdVector3* dest, float scalar, const rdVector3* add); // Added
static inline void J3DAPI rdVector_ScaleAdd4Acc(rdVector4* dest, float scalar, const rdVector4* add); // Added

// Helper functions for accumulating scaled vector; i.e. destvec += toscalevec * scalar
// To not be confused with rdVector_ScaleAddxAcc
static inline void J3DAPI rdVector_MultAcc2(rdVector2* acc, const rdVector2* toscale, float scalar); // Added
static inline void J3DAPI rdVector_MultAcc3(rdVector3* acc, const rdVector3* toscale, float scalar); // Added
static inline void J3DAPI rdVector_MultAcc4(rdVector4* acc, const rdVector4* toscale, float scalar); // Added

static inline float J3DAPI rdVector_Len2(const rdVector2* v); // Added from debug version
static inline float J3DAPI rdVector_Len3(const rdVector3* vec);
static inline float J3DAPI rdVector_Len4(rdVector4* a); // Added from debug version

static inline float J3DAPI rdVector_Dist2(const rdVector2* a, const rdVector2* b); // Added
static inline float J3DAPI rdVector_Dist3(const rdVector3* a, const rdVector3* b); // Added
static inline float J3DAPI rdVector_Dist4(const rdVector4* a, const rdVector4* b); // Added

static inline float J3DAPI rdVector_Dot2(const rdVector2* a, const rdVector2* b); // Added from debug version
static inline float J3DAPI rdVector_Dot3(const rdVector3* a, const rdVector3* b);
static inline float J3DAPI rdVector_Dot4(const rdVector4* a, const rdVector4* b); // Added from debug version

static inline float J3DAPI rdVector_Cross2(const rdVector2* v1, const rdVector2* v2); // Added
static inline void J3DAPI rdVector_Cross3(rdVector3* dest, const rdVector3* v1, const rdVector3* v2); // Added
static inline void J3DAPI rdVector_Cross3Acc(rdVector3* dest, const rdVector3* v2); // Added

/**
 * @brief Calculates scalar triple product.
     i.e.: (`a` cross `b`) dot `c`
 *
 * @param a - Pointer to the first input vector.
 * @param b - Pointer to the second input vector.
 * @param c - Pointer to the third input vector.
 * @return The dot product of the cross product of `a` and `b` with `c`.
 */
static inline float J3DAPI rdVector_CrossDot3(const rdVector3* a, const rdVector3* b, const rdVector3* c); // Added

float J3DAPI J3DAPI rdVector_Normalize2(rdVector2* dest, const rdVector2* src); // Added
float J3DAPI J3DAPI rdVector_Normalize3(rdVector3* dest, const rdVector3* src);
float J3DAPI J3DAPI rdVector_Normalize4(rdVector4* dest, const rdVector4* src); // Added

float J3DAPI J3DAPI rdVector_Normalize2Acc(rdVector2* vec);
float J3DAPI J3DAPI rdVector_Normalize3Acc(rdVector3* vec);
float J3DAPI J3DAPI rdVector_Normalize3QuickAcc(rdVector3* src);
float J3DAPI J3DAPI rdVector_Normalize4Acc(rdVector4* vec);

/**
 * @brief Linearly interpolates between two 2D vectors.
 * @param dest - Pointer to the destination vector.
 * @param a - Pointer to the first input vector.
 * @param b - Pointer to the second input vector.
 * @param t - The interpolation factor.
 */
static inline void J3DAPI rdVector_Lerp2(rdVector2* dest, const rdVector2* a, const rdVector2* b, float t); // Added

/**
 * @brief Linearly interpolates between two 3D vectors.
 * @param dest - Pointer to the destination vector.
 * @param a - Pointer to the first input vector.
 * @param b - Pointer to the second input vector.
 * @param t - The interpolation factor.
 */
static inline void J3DAPI rdVector_Lerp3(rdVector3* dest, const rdVector3* a, const rdVector3* b, float t); // Added

/**
 * @brief Linearly interpolates between two 4D vectors.
 * @param dest - Pointer to the destination vector.
 * @param a - Pointer to the first input vector.
 * @param b - Pointer to the second input vector.
 * @param t - The interpolation factor.
 */
static inline void J3DAPI rdVector_Lerp4(rdVector4* dest, const rdVector4* a, const rdVector4* b, float t); // Added

static inline void J3DAPI rdVector_Lerp2Acc(rdVector2* a, const rdVector2* b, float t); // Added
static inline void J3DAPI rdVector_Lerp3Acc(rdVector3* a, const rdVector3* b, float t); // Added
static inline void J3DAPI rdVector_Lerp4Acc(rdVector4* a, const rdVector4* b, float t); // Added

/**
 * Smoothly interpolate a `src` vector towards `target` vector using exponential damping..
 * @param dest   - Pointer to the destination vector.
 * @param src    - Pointer to the source vector.
 * @param target - Pointer to the targeting vector.
 * @param rate   - Damping rate/speed (higher = faster convergence).
 * @param t      - The interpolation factor.
 */
static inline void J3DAPI rdVector_SmoothDamp2(rdVector2* dest, const rdVector2* src, const rdVector2* target, float rate, float t); // Added

/**
 * Smoothly interpolate a `src` vector towards `target` vector using exponential damping..
 * @param dest   - Pointer to the destination vector.
 * @param src    - Pointer to the source vector.
 * @param target - Pointer to the targeting vector.
 * @param rate   - Damping rate/speed (higher = faster convergence).
 * @param t      - The interpolation factor.
 */
static inline void J3DAPI rdVector_SmoothDamp3(rdVector3* dest, const rdVector3* src, const rdVector3* target, float rate, float t); // Added

/**
 * Smoothly interpolate a `src` vector towards `target` vector using exponential damping..
 * @param dest   - Pointer to the destination vector.
 * @param src    - Pointer to the source vector.
 * @param target - Pointer to the targeting vector.
 * @param rate   - Damping rate/speed (higher = faster convergence).
 * @param t      - The interpolation factor.
 */
static inline void J3DAPI rdVector_SmoothDamp4(rdVector4* dest, const rdVector4* src, const rdVector4* target, float rate, float t); // Added

static inline void J3DAPI rdVector_SmoothDamp2Acc(rdVector2* dest, const rdVector2* target, float rate, float t); // Added
static inline void J3DAPI rdVector_SmoothDamp3Acc(rdVector3* dest, const rdVector3* target, float rate, float t); // Added
static inline void J3DAPI rdVector_SmoothDamp4Acc(rdVector4* dest, const rdVector4* target, float rate, float t); // Added

void J3DAPI rdVector_Rotate3(rdVector3* vec, const rdVector3* pivot, const rdVector3* pyr);
void J3DAPI rdVector_Rotate3Acc(rdVector3* vec, const rdVector3* pyr);

bool J3DAPI rdVector_IsZero2(const rdVector2* v)
{
    return (v->x == 0.0f && v->y == 0.0f);
}

bool J3DAPI rdVector_IsZero3(const rdVector3* v)
{
    return (v->x == 0.0f && v->y == 0.0f && v->z == 0.0f);
}

bool J3DAPI rdVector_IsZero4(const rdVector4* v)
{
    return (v->x == 0.0f && v->y == 0.0f && v->z == 0.0f && v->w == 0.0f);
}

bool J3DAPI rdVector_IsNAN2(const rdVector2* v)
{
    return isnan(v->x) || isnan(v->y);
}

bool J3DAPI rdVector_IsNAN3(const rdVector3* v)
{
    return isnan(v->x) || isnan(v->y) || isnan(v->z);
}

bool J3DAPI rdVector_IsNAN4(const rdVector4* v)
{
    return isnan(v->x) || isnan(v->y) || isnan(v->z) || isnan(v->w);
}

bool J3DAPI rdVector_Equal2(const rdVector2* a, const rdVector2* b)
{
    return a->x == b->x && a->y == b->y;
}

bool J3DAPI rdVector_Equal3(const rdVector3* a, const rdVector3* b)
{
    return a->x == b->x && a->y == b->y && a->z == b->z;
}

bool J3DAPI rdVector_Equal4(const rdVector4* a, const rdVector4* b)
{
    return a->x == b->x && a->y == b->y && a->z == b->z && a->w == b->w;
}

void J3DAPI rdVector_Zero2(rdVector2* dest)
{
    STD_ZEROMEM(dest, sizeof(rdVector2));
}

void J3DAPI rdVector_Zero3(rdVector3* dest)
{
    STD_ZEROMEM(dest, sizeof(rdVector3));
}

void J3DAPI rdVector_Zero4(rdVector4* dest)
{
    STD_ZEROMEM(dest, sizeof(rdVector4));
}

void J3DAPI rdVector_Set2(rdVector2* dest, float x, float y)
{
    dest->x = x;
    dest->y = y;
}

void J3DAPI rdVector_Set3(rdVector3* dest, float x, float y, float z)
{
    dest->x = x;
    dest->y = y;
    dest->z = z;
}

void J3DAPI rdVector_Set4(rdVector4* dest, float x, float y, float z, float w)
{
    dest->x = x;
    dest->y = y;
    dest->z = z;
    dest->w = w;
}

void J3DAPI rdVector_Copy2(rdVector2* dest, const rdVector2* src)
{
    STD_COPYMEM(dest, src, sizeof(rdVector2));
}

void J3DAPI rdVector_Copy2List(rdVector2* dest, const rdVector2* src, size_t num)
{
    STD_COPYMEM(dest, src, sizeof(rdVector2) * num);
}

void J3DAPI rdVector_Copy3(rdVector3* dest, const rdVector3* src)
{
    STD_COPYMEM(dest, src, sizeof(rdVector3));
}

void J3DAPI rdVector_Copy3List(rdVector3* dest, const rdVector3* src, size_t num)
{
    STD_COPYMEM(dest, src, sizeof(rdVector3) * num);
}

void J3DAPI rdVector_Copy4(rdVector4* dest, const rdVector4* src)
{
    STD_COPYMEM(dest, src, sizeof(rdVector4));
}

void J3DAPI rdVector_Copy4List(rdVector4* dest, const rdVector4* src, size_t num)
{
    STD_COPYMEM(dest, src, sizeof(rdVector4) * num);
}

void J3DAPI rdVector_Neg2(rdVector2* dest, const rdVector2* src)
{
    dest->x = -src->x;
    dest->y = -src->y;
}

void J3DAPI rdVector_Neg3(rdVector3* dest, const rdVector3* src)
{
    dest->x = -src->x;
    dest->y = -src->y;
    dest->z = -src->z;
}

void J3DAPI rdVector_Neg4(rdVector4* dest, const rdVector4* src)
{
    dest->x = -src->x;
    dest->y = -src->y;
    dest->z = -src->z;
    dest->w = -src->w;
}

void J3DAPI rdVector_Neg2Acc(rdVector2* a)
{
    a->x = -a->x;
    a->y = -a->y;
}

void J3DAPI rdVector_Neg3Acc(rdVector3* a)
{
    a->x = -a->x;
    a->y = -a->y;
    a->z = -a->z;
}

void J3DAPI rdVector_Neg4Acc(rdVector4* a)
{
    a->x = -a->x;
    a->y = -a->y;
    a->z = -a->z;
    a->w = -a->w;
}

void J3DAPI rdVector_Add2(rdVector2* c, const rdVector2* a, const rdVector2* b)
{
    c->x = a->x + b->x;
    c->y = a->y + b->y;
}

void J3DAPI rdVector_Add3(rdVector3* c, const rdVector3* a, const rdVector3* b)
{
    c->x = a->x + b->x;
    c->y = a->y + b->y;
    c->z = a->z + b->z;
}

void J3DAPI rdVector_Add4(rdVector4* c, const rdVector4* a, const rdVector4* b)
{
    c->x = a->x + b->x;
    c->y = a->y + b->y;
    c->z = a->z + b->z;
    c->w = a->w + b->w;
}

void J3DAPI rdVector_Add2Acc(rdVector2* a, const rdVector2* b)
{
    a->x = a->x + b->x;
    a->y = a->y + b->y;
}

void J3DAPI rdVector_Add3Acc(rdVector3* a, const rdVector3* b)
{
    a->x += b->x;
    a->y += b->y;
    a->z += b->z;
}

void J3DAPI rdVector_Add4Acc(rdVector4* a, const rdVector4* b)
{
    a->x += b->x;
    a->y += b->y;
    a->z += b->z;
    a->w += b->w;
}

void J3DAPI rdVector_Sub2(rdVector2* c, const rdVector2* a, const rdVector2* b)
{
    c->x = a->x - b->x;
    c->y = a->y - b->y;
}

void J3DAPI rdVector_Sub3(rdVector3* c, const rdVector3* a, const rdVector3* b)
{
    c->x = a->x - b->x;
    c->y = a->y - b->y;
    c->z = a->z - b->z;
}

void J3DAPI rdVector_Sub4(rdVector4* c, const rdVector4* a, const rdVector4* b)
{
    c->x = a->x - b->x;
    c->y = a->y - b->y;
    c->z = a->z - b->z;
    c->w = a->w - b->w;
}

void J3DAPI rdVector_Sub2Acc(rdVector2* a, const rdVector2* b)
{
    a->x -= b->x;
    a->y -= b->y;
}

void J3DAPI rdVector_Sub3Acc(rdVector3* a, const rdVector3* b)
{
    a->x -= b->x;
    a->y -= b->y;
    a->z -= b->z;
}

void J3DAPI rdVector_Sub4Acc(rdVector4* a, const rdVector4* b)
{
    a->x -= b->x;
    a->y -= b->y;
    a->z -= b->z;
    a->w -= b->w;
}

void J3DAPI rdVector_Scale2(rdVector2* dest, const rdVector2* src, float scalar)
{
    dest->x = src->x * scalar;
    dest->y = src->y * scalar;
}

void J3DAPI rdVector_Scale3(rdVector3* dest, const rdVector3* src, float scalar)
{
    dest->x = src->x * scalar;
    dest->y = src->y * scalar;
    dest->z = src->z * scalar;
}

void J3DAPI rdVector_Scale2Acc(rdVector2* v, float scalar)
{
    v->x = v->x * scalar;
    v->y = v->y * scalar;
}

void J3DAPI rdVector_Scale3Acc(rdVector3* v, float scalar)
{
    v->x = v->x * scalar;
    v->y = v->y * scalar;
    v->z = v->z * scalar;
}

void J3DAPI rdVector_Scale4Acc(rdVector4* v, float scalar)
{
    v->x = v->x * scalar;
    v->y = v->y * scalar;
    v->z = v->z * scalar;
    v->w = v->w * scalar;
}

void J3DAPI rdVector_Scale4(rdVector4* dest, const rdVector4* src, float scalar)
{
    dest->x = src->x * scalar;
    dest->y = src->y * scalar;
    dest->z = src->z * scalar;
    dest->w = src->w * scalar;
}

void J3DAPI rdVector_InvScale2(rdVector2* dest, const rdVector2* src, float scalar)
{
    dest->x = src->x / scalar;
    dest->y = src->y / scalar;
}

void J3DAPI rdVector_InvScale3(rdVector3* dest, const rdVector3* src, float scalar)
{
    double invScalar = 1.0 / scalar;
    dest->x = (float)(src->x * invScalar);
    dest->y = (float)(src->y * invScalar);
    dest->z = (float)(src->z * invScalar);
}

void J3DAPI rdVector_InvScale4(rdVector4* dest, const rdVector4* src, float scalar)
{
    double invScalar = 1.0 / scalar;
    dest->x = (float)(src->x * invScalar);
    dest->y = (float)(src->y * invScalar);
    dest->z = (float)(src->z * invScalar);
    dest->w = (float)(src->w * invScalar);
}

void J3DAPI rdVector_InvScale2Acc(rdVector2* v, float scalar)
{
    v->x = v->x / scalar;
    v->y = v->y / scalar;
}

void J3DAPI rdVector_InvScale3Acc(rdVector3* v, float scalar)
{
    double inv = 1.0 / scalar;
    v->x = (float)(v->x * inv);
    v->y = (float)(v->y * inv);
    v->z = (float)(v->z * inv);
}

void J3DAPI rdVector_InvScale4Acc(rdVector4* v, float scalar)
{
    double inv = 1.0 / scalar;
    v->x = (float)(v->x * inv);
    v->y = (float)(v->y * inv);
    v->z = (float)(v->z * inv);
    v->w = (float)(v->w * inv);
}

void J3DAPI rdVector_ScaleAdd2(rdVector2* dest, const rdVector2* src, float scalar, const rdVector2* add)
{
    dest->x = src->x * scalar + add->x;
    dest->y = src->y * scalar + add->y;
}

void J3DAPI rdVector_ScaleAdd3(rdVector3* dest, const rdVector3* src, float scalar, const rdVector3* add)
{
    dest->x = src->x * scalar + add->x;
    dest->y = src->y * scalar + add->y;
    dest->z = src->z * scalar + add->z;
}

void J3DAPI rdVector_ScaleAdd4(rdVector4* dest, const rdVector4* src, float scalar, const rdVector4* add)
{
    dest->x = src->x * scalar + add->x;
    dest->y = src->y * scalar + add->y;
    dest->z = src->z * scalar + add->z;
    dest->w = src->w * scalar + add->w;
}

void J3DAPI rdVector_ScaleAdd2Acc(rdVector2* dest, float scalar, const rdVector2* add)
{
    dest->x = dest->x * scalar + add->x;
    dest->y = dest->y * scalar + add->y;
}

void J3DAPI rdVector_ScaleAdd3Acc(rdVector3* dest, float scalar, const rdVector3* add)
{
    dest->x = dest->x * scalar + add->x;
    dest->y = dest->y * scalar + add->y;
    dest->z = dest->z * scalar + add->z;
}

void J3DAPI rdVector_ScaleAdd4Acc(rdVector4* dest, float scalar, const rdVector4* add)
{
    dest->x = dest->x * scalar + add->x;
    dest->y = dest->y * scalar + add->y;
    dest->z = dest->z * scalar + add->z;
    dest->w = dest->w * scalar + add->w;
}

void J3DAPI rdVector_MultAcc2(rdVector2* acc, const rdVector2* toscale, float scalar)
{
    acc->x += toscale->x * scalar;
    acc->y += toscale->y * scalar;
}

void J3DAPI rdVector_MultAcc3(rdVector3* acc, const rdVector3* toscale, float scalar)
{
    acc->x += toscale->x * scalar;
    acc->y += toscale->y * scalar;
    acc->z += toscale->z * scalar;
}

void J3DAPI rdVector_MultAcc4(rdVector4* acc, const rdVector4* toscale, float scalar)
{
    acc->x += toscale->x * scalar;
    acc->y += toscale->y * scalar;
    acc->z += toscale->z * scalar;
    acc->w += toscale->w * scalar;
}

float J3DAPI rdVector_Len2(const rdVector2* v)
{
    return sqrtf(v->x * v->x + v->y * v->y);
}

float J3DAPI rdVector_Len3(const rdVector3* vec)
{
    return sqrtf(vec->y * vec->y + vec->z * vec->z + vec->x * vec->x);
}

float J3DAPI rdVector_Len4(rdVector4* a)
{
    return sqrtf(a->x * a->x + a->y * a->y + a->z * a->z + a->w * a->w);
}

float J3DAPI rdVector_Dist2(const rdVector2* a, const rdVector2* b)
{
    float dx = a->x - b->x;
    float dy = a->y - b->y;
    return sqrtf(dx * dx + dy * dy);
}

float J3DAPI rdVector_Dist3(const rdVector3* a, const rdVector3* b)
{
    float dx = a->x - b->x;
    float dy = a->y - b->y;
    float dz = a->z - b->z;
    return sqrtf(dx * dx + dy * dy + dz * dz);
}

float J3DAPI rdVector_Dist4(const rdVector4* a, const rdVector4* b)
{
    float dx = a->x - b->x;
    float dy = a->y - b->y;
    float dz = a->z - b->z;
    float dw = a->w - b->w;
    return sqrtf(dx * dx + dy * dy + dz * dz + dw * dw);
}

float J3DAPI rdVector_Dot2(const rdVector2* a, const rdVector2* b)
{
    return a->x * b->x + a->y * b->y;
}

float J3DAPI rdVector_Dot3(const rdVector3* a, const rdVector3* b)
{
    return a->x * b->x + a->y * b->y + a->z * b->z;
}

float J3DAPI rdVector_Dot4(const rdVector4* a, const rdVector4* b)
{
    return a->x * b->x + a->y * b->y + a->z * b->z + a->w * b->w;
}

float J3DAPI rdVector_Cross2(const rdVector2* v1, const rdVector2* v2)
{
    return v1->x * v2->y - v1->y * v2->x;
}

void J3DAPI rdVector_Cross3(rdVector3* dest, const rdVector3* v1, const rdVector3* v2)
{
    RD_ASSERT(((dest != v1) && (dest != v2)));
    dest->x = v1->y * v2->z - v1->z * v2->y;
    dest->y = v1->z * v2->x - v1->x * v2->z;
    dest->z = v1->x * v2->y - v1->y * v2->x;
}

void J3DAPI rdVector_Cross3Acc(rdVector3* dest, const rdVector3* v2)
{
    rdVector3 res;
    res.x = dest->y * v2->z - dest->z * v2->y;
    res.y = dest->z * v2->x - dest->x * v2->z;
    res.z = dest->x * v2->y - dest->y * v2->x;
    *dest = res;
}

float J3DAPI rdVector_CrossDot3(const rdVector3* a, const rdVector3* b, const rdVector3* c)
{
    return (a->y * b->z - b->y * a->z) * c->x
        + (a->z * b->x - b->z * a->x) * c->y
        + (a->x * b->y - b->x * a->y) * c->z;
}

void J3DAPI rdVector_Lerp2(rdVector2* dest, const rdVector2* a, const rdVector2* b, float t)
{
    dest->x = a->x + (b->x - a->x) * t;
    dest->y = a->y + (b->y - a->y) * t;
}

void J3DAPI rdVector_Lerp3(rdVector3* dest, const rdVector3* a, const rdVector3* b, float t)
{
    dest->x = a->x + (b->x - a->x) * t;
    dest->y = a->y + (b->y - a->y) * t;
    dest->z = a->z + (b->z - a->z) * t;
}

void J3DAPI rdVector_Lerp4(rdVector4* dest, const rdVector4* a, const rdVector4* b, float t)
{
    dest->x = a->x + (b->x - a->x) * t;
    dest->y = a->y + (b->y - a->y) * t;
    dest->z = a->z + (b->z - a->z) * t;
    dest->w = a->w + (b->w - a->w) * t;
}

void J3DAPI rdVector_Lerp2Acc(rdVector2* a, const rdVector2* b, float t)
{
    a->x = a->x + (b->x - a->x) * t;
    a->y = a->y + (b->y - a->y) * t;
}

void J3DAPI rdVector_Lerp3Acc(rdVector3* a, const rdVector3* b, float t)
{
    a->x = a->x + (b->x - a->x) * t;
    a->y = a->y + (b->y - a->y) * t;
    a->z = a->z + (b->z - a->z) * t;
}

void J3DAPI rdVector_Lerp4Acc(rdVector4* a, const rdVector4* b, float t)
{
    a->x = a->x + (b->x - a->x) * t;
    a->y = a->y + (b->y - a->y) * t;
    a->z = a->z + (b->z - a->z) * t;
    a->w = a->w + (b->w - a->w) * t;
}

void J3DAPI rdVector_SmoothDamp2(rdVector2* dest, const rdVector2* src, const rdVector2* target, float rate, float t)
{
    dest->x = stdMath_SmoothDamp(src->x, target->x, rate, t);
    dest->y = stdMath_SmoothDamp(src->y, target->y, rate, t);
}

void J3DAPI rdVector_SmoothDamp3(rdVector3* dest, const rdVector3* src, const rdVector3* target, float rate, float t)
{
    dest->x = stdMath_SmoothDamp(src->x, target->x, rate, t);
    dest->y = stdMath_SmoothDamp(src->y, target->y, rate, t);
    dest->z = stdMath_SmoothDamp(src->z, target->z, rate, t);
}

void J3DAPI rdVector_SmoothDamp4(rdVector4* dest, const rdVector4* src, const rdVector4* target, float rate, float t)
{
    dest->x = stdMath_SmoothDamp(src->x, target->x, rate, t);
    dest->y = stdMath_SmoothDamp(src->y, target->y, rate, t);
    dest->z = stdMath_SmoothDamp(src->z, target->z, rate, t);
    dest->w = stdMath_SmoothDamp(src->w, target->w, rate, t);
}

void J3DAPI rdVector_SmoothDamp2Acc(rdVector2* dest, const rdVector2* target, float rate, float t)
{
    dest->x = stdMath_SmoothDamp(dest->x, target->x, rate, t);
    dest->y = stdMath_SmoothDamp(dest->y, target->y, rate, t);
}

void J3DAPI rdVector_SmoothDamp3Acc(rdVector3* dest, const rdVector3* target, float rate, float t)
{
    dest->x = stdMath_SmoothDamp(dest->x, target->x, rate, t);
    dest->y = stdMath_SmoothDamp(dest->y, target->y, rate, t);
    dest->z = stdMath_SmoothDamp(dest->z, target->z, rate, t);
}

void J3DAPI rdVector_SmoothDamp4Acc(rdVector4* dest, const rdVector4* target, float rate, float t)
{
    dest->x = stdMath_SmoothDamp(dest->x, target->x, rate, t);
    dest->y = stdMath_SmoothDamp(dest->y, target->y, rate, t);
    dest->z = stdMath_SmoothDamp(dest->z, target->z, rate, t);
    dest->w = stdMath_SmoothDamp(dest->w, target->w, rate, t);
}

// Helper hooking functions
void rdVector_InstallHooks(void);
void rdVector_ResetGlobals(void);

J3D_EXTERN_C_END
#endif // RDROID_RDVECTOR_H

#ifndef STD_STDMATH_H
#define STD_STDMATH_H
#include <j3dcore/j3d.h>
#include <std/types.h>
#include <std/RTI/addresses.h>

#define _USE_MATH_DEFINES
#include <math.h>

J3D_EXTERN_C_START

#define STDMATH_ZERO_EPSILON 0.00001f

// Clamp value between min and max
#define STDMATH_CLAMP(val, minv, maxv) (((val) < (minv)) ? (minv) : ((val) > (maxv)) ? (maxv) : (val))

// PI
#define STDMATH_PI M_PI
#define STDMATH_PI_F ((float)M_PI)

// Convert radians to degrees
#define STDMATH_TODEGREES(rad)  ((rad) * (180.0 / STDMATH_PI))
#define STDMATH_TODEGREESF(rad) (float)STDMATH_TODEGREES(rad)

// Convert degrees to radians
#define STDMATH_RADIANS(deg)  ((deg) * (STDMATH_PI / 180.0))
#define STDMATH_RADIANSF(deg) (float)STDMATH_RADIANS(deg)

// Quick static test
static_assert(STDMATH_RADIANS(90.0) == 1.57079632679489661923, "");
static_assert(STDMATH_RADIANSF(90.0) == 1.57079632f, "");

// Macro calculates circle circumference of circle with given radius
// TODO: PI constant 3.1415901f was taken from engine, replace with (float)M_PI if possible
#define STDMATH_CIRCLE_CIRCUMF(radius) (3.1415901f * (radius) * 2.0f)

float J3DAPI stdMath_FlexPower(float base, int exponent);
static inline float stdMath_ClipNearZero(float val); // Added

float J3DAPI stdMath_NormalizeAngle(float angle);
float J3DAPI stdMath_NormalizeAngleAcute(float angle);

// Calculate sinus and cousins of value angle
void J3DAPI stdMath_SinCos(float angle, float* pSinOut, float* pCosOut);
float J3DAPI stdMath_Tan(float angle);
float J3DAPI stdMath_ArcSin1(float angle);
float J3DAPI stdMath_ArcSin2(float value); // Added
float J3DAPI stdMath_ArcSin3(float angle);

// Returns value of the arc tangent
float J3DAPI stdMath_ArcTan4(float x, float y);
float J3DAPI stdMath_Dist2D1(float x, float y);
float J3DAPI stdMath_Dist3D1(float x, float y, float z);

/**
 * Smoothly interpolate a value towards a target using exponential damping.
 * This creates smooth, natural-looking motion that slows as it approaches the target.
 *
 * Unlike linear interpolation (lerp), this maintains a constant rate of approach
 * regardless of frame rate, and never overshoots the target.
 *
 * @param current   - Current value (e.g. current position, rotation, or any float).
 * @param target    - Target value to approach.
 * @param rate      - Damping rate/speed (higher = faster convergence).
 *                    Typical values: 5-10 for smooth, 15-25 for snappy.
 * @param deltaTime - Time elapsed since last frame.
 *                    If <= 0, returns current value unchanged.
 *
 * @return New smoothed value, exponentially moved from current toward target.
 *
 * @note This function is frame-rate independent. The same rate value produces
 *       consistent behavior regardless of frame rate.
 * @note Uses expm1 internally for improved numerical accuracy at high frame rates.
 */
float stdMath_SmoothDamp(float current, float target, float rate, float deltaTime); // New

float stdMath_ClipNearZero(float val)
{
    if ( fabsf(val) <= STDMATH_ZERO_EPSILON )
    {
        return 0.0f;
    }
    return val;
}

/**
 * Computes exp(x) - 1 with better numerical accuracy for small x.
 * For |x| < 1e-5, uses Taylor series approximation to avoid catastrophic cancellation.
 */
static inline float stdMath_Expm1f(float x)
{
  // Use standard library expm1f if available (C99+)
#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 199901L
    return expm1f(x);
#else
    // ref: https://www.johndcook.com/blog/cpp_expm1/
    if ( fabsf(x) < 1e-5f )
    {
        // Taylor series: exp(x) - 1 = x + x^2/2 + x^3/6 + ...
        // For small x, first two terms are sufficient
        return x + 0.5f * x * x;
    }
    else
    {
        return expf(x) - 1.0f;
    }
#endif
}

// Helper hooking functions
void stdMath_InstallHooks(void);
void stdMath_ResetGlobals(void);

J3D_EXTERN_C_END
#endif // STD_STDMATH_H

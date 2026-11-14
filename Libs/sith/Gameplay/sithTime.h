#ifndef SITH_SITHTIME_H
#define SITH_SITHTIME_H
#include <j3dcore/j3d.h>
#include <math.h>
#include <rdroid/types.h>
#include <sith/types.h>
#include <sith/Main/sithMain.h>
#include <sith/RTI/addresses.h>
#include <std/types.h>

J3D_EXTERN_C_START

#define sithTime_g_frameTime J3D_DECL_FAR_VAR(sithTime_g_frameTime, uint32_t)
// extern unsigned int sithTime_g_frameTime;

#define sithTime_g_frameTimeFlex J3D_DECL_FAR_VAR(sithTime_g_frameTimeFlex, float)
// extern float sithTime_g_secFrameTime;

#define sithTime_g_fps J3D_DECL_FAR_VAR(sithTime_g_fps, float)
// extern float sithTime_g_fps;

#define sithTime_g_msecGameTime J3D_DECL_FAR_VAR(sithTime_g_msecGameTime, uint32_t)
// extern unsigned int sithTime_g_msecGameTime;

#define sithTime_g_secGameTime J3D_DECL_FAR_VAR(sithTime_g_secGameTime, float)
// extern float sithTime_g_secGameTime;

#define sithTime_g_clockTime J3D_DECL_FAR_VAR(sithTime_g_clockTime, uint32_t)
// extern unsigned int sithTime_g_clockTime;

#define sithTime_g_bPaused J3D_DECL_FAR_VAR(sithTime_g_bPaused, int)
// extern int sithTime_g_bPaused;

#ifdef J3D_QOL_IMPROVEMENTS
/**
 * Reference frame rate of SITHTIME_ISFRAMECYCLE: effects that the original engine ran on every n-th frame
 * run as often as they did at this frame rate, at any FPS. The game was designed for 30 FPS.
 */
#  define SITHTIME_FRAMECYCLE_FPS 30u

/**
 * Returns the number of reference frames (at SITHTIME_FRAMECYCLE_FPS) in a game time.
 * Splits off whole seconds, so it doesn't overflow and only divides by constants, which compile to multiplications.
 * @note new function
 */
static inline uint32_t sithTime_GetReferenceFrame(uint32_t msecTime)
{
    return (msecTime / 1000u) * SITHTIME_FRAMECYCLE_FPS + (msecTime % 1000u) * SITHTIME_FRAMECYCLE_FPS / 1000u;
}

/**
 * Returns the game time in msec at which a reference frame (at SITHTIME_FRAMECYCLE_FPS) starts, rounded up to a whole msec,
 * so sithTime_GetReferenceFrame of it is the same frame again.
 * @note new function
 */
static inline uint32_t sithTime_GetReferenceFrameTime(uint32_t frame)
{
    return (frame / SITHTIME_FRAMECYCLE_FPS) * 1000u
        + ((frame % SITHTIME_FRAMECYCLE_FPS) * 1000u + SITHTIME_FRAMECYCLE_FPS - 1u) / SITHTIME_FRAMECYCLE_FPS;
}

/**
 * Time-based SITH_ISFRAMECYCLE. Returns true in the frame in which the game time passes a multiple of n reference frames
 * (at SITHTIME_FRAMECYCLE_FPS), so an effect that the original ran on every n-th frame runs as often at any FPS.
 * Below the reference rate it runs at most once per frame.
 * @param offset - Phase offset in reference frames, e.g. the thing index, so things don't all run in the same frame
 * @param n      - Cycle length in reference frames, a power of two like in SITH_ISFRAMECYCLE
 * @note new function
 */
static inline bool sithTime_IsFrameCycle(uint32_t offset, uint32_t n)
{
    // A cycle of one runs once per reference frame. A frame as long as a reference frame or longer runs it every frame,
    // like the original: whole msec frames (33, 34) don't always cross a reference frame boundary (every 33.33 msec).
    if ( n == 1u && sithTime_g_frameTime >= 1000u / SITHTIME_FRAMECYCLE_FPS )
    {
        return true;
    }

    const uint32_t msecPrev  = sithTime_g_msecGameTime >= sithTime_g_frameTime ? sithTime_g_msecGameTime - sithTime_g_frameTime : 0u;
    const uint32_t frameNow  = sithTime_GetReferenceFrame(sithTime_g_msecGameTime) + offset;
    const uint32_t framePrev = sithTime_GetReferenceFrame(msecPrev) + offset;

    // A multiple of n was passed if the frame numbers differ above the low bits
    const uint32_t cycleMask = ~(n - 1u);
    return (frameNow & cycleMask) != (framePrev & cycleMask);
}

#  define SITHTIME_ISFRAMECYCLE(offset, n) sithTime_IsFrameCycle((uint32_t)(offset), (n))

// Longest interval sithTime_CalcDeadline puts on the 30 FPS frames. Longer ones gain nothing from it,
// and the frame count of this interval doesn't overflow.
#  define SITHTIME_MAX_FRAMEALIGNED_MSEC 60000u
#else
#  define SITHTIME_ISFRAMECYCLE(offset, n) (SITH_ISFRAMECYCLE(offset, n))
#endif

/**
 * Deadlines: a deadline is a game time in msec, msecInterval from now, that the engine checks once per frame.
 * Create one with sithTime_CalcDeadline (it is due when the game time reaches it: pulses, timers, waits) or
 * sithTime_CalcDeadlineAfter (it is due when the game time is past it: sleeps, events), and test it with the matching
 * sithTime_IsDeadlineReached or sithTime_IsDeadlinePassed.
 *
 * At 30 FPS, the frame rate the game was made for, an interval lasts whole frames. In QOL builds, while frames are shorter
 * than a 30 FPS frame, the deadline is put on the start of the 30 FPS frame the original reached it in, so everything
 * scheduled this way ticks as at 30 FPS at any higher frame rate. At 30 FPS and below it is now + msecInterval.
 * @note new functions
 */
static inline tStdTime sithTime_CalcDeadlineEx(tStdTime msecInterval, bool bPast)
{
#ifdef J3D_QOL_IMPROVEMENTS
    if ( sithTime_g_frameTime < 1000u / SITHTIME_FRAMECYCLE_FPS && msecInterval <= SITHTIME_MAX_FRAMEALIGNED_MSEC )
    {
        // 30 FPS frames the original waits: past the interval the frames past it, otherwise the frames to it (at least one)
        uint32_t numFrames = bPast
            ? msecInterval * SITHTIME_FRAMECYCLE_FPS / 1000u + 1u
            : (msecInterval * SITHTIME_FRAMECYCLE_FPS + 999u) / 1000u;
        numFrames = J3DMAX(numFrames, 1u);

        // The start of that 30 FPS frame. Frames are shorter than a 30 FPS frame here, so the deadline is always handled
        // in the 30 FPS frame it lies in, and counting from the current one keeps the 30 FPS rhythm without any state.
        const tStdTime msecDeadline = sithTime_GetReferenceFrameTime(sithTime_GetReferenceFrame(sithTime_g_msecGameTime) + numFrames);
        return bPast ? msecDeadline - 1u : msecDeadline; // A past deadline is due when the game time is past it
    }
#else
    J3D_UNUSED(bPast);
#endif

    return sithTime_g_msecGameTime + msecInterval;
}

static inline tStdTime sithTime_CalcDeadline(tStdTime msecInterval)
{
    return sithTime_CalcDeadlineEx(msecInterval, /*bPast=*/false);
}

static inline tStdTime sithTime_CalcDeadlineAfter(tStdTime msecInterval)
{
    return sithTime_CalcDeadlineEx(msecInterval, /*bPast=*/true);
}

static inline bool sithTime_IsDeadlineReached(tStdTime msecDeadline)
{
    return sithTime_g_msecGameTime >= msecDeadline;
}

static inline bool sithTime_IsDeadlinePassed(tStdTime msecDeadline)
{
    return sithTime_g_msecGameTime > msecDeadline;
}

// The same for code that keeps its deadline as float msec with a fraction (AI combat move and jump): the original float sum
// below 30 FPS, so the stored value is the original's; a whole msec on the 30 FPS frames above it
static inline float sithTime_CalcDeadlineFlex(float msecInterval)
{
#ifdef J3D_QOL_IMPROVEMENTS
    if ( msecInterval > 0.0f && sithTime_g_frameTime < 1000u / SITHTIME_FRAMECYCLE_FPS )
    {
        const tStdTime msecWhole = (tStdTime)ceilf(msecInterval); // Due at the first whole msec after the interval, like the float sum
        if ( msecWhole <= SITHTIME_MAX_FRAMEALIGNED_MSEC )
        {
            return (float)sithTime_CalcDeadline(msecWhole);
        }
    }
#endif

    return (float)sithTime_g_msecGameTime + msecInterval;
}

static inline bool sithTime_IsDeadlineReachedFlex(float msecDeadline)
{
    return (float)sithTime_g_msecGameTime >= msecDeadline;
}

// The same for code that keeps its deadlines in seconds (sithTime_g_secGameTime is the game time in msec * 0.001f)
static inline float sithTime_CalcDeadlineSec(float secInterval)
{
#ifdef J3D_QOL_IMPROVEMENTS
    if ( secInterval > 0.0f && sithTime_g_frameTime < 1000u / SITHTIME_FRAMECYCLE_FPS )
    {
        const tStdTime msecInterval = (tStdTime)(secInterval * 1000.0f + 0.5f);
        if ( msecInterval <= SITHTIME_MAX_FRAMEALIGNED_MSEC )
        {
            return (float)sithTime_CalcDeadline(msecInterval) * 0.001f; // Like sithTime_g_secGameTime, so the deadline frame compares equal
        }
    }
#endif

    return sithTime_g_secGameTime + secInterval; // The original float sum, so the plain deadline is bit-exact
}

static inline bool sithTime_IsDeadlineReachedSec(float secDeadline)
{
    return sithTime_g_secGameTime >= secDeadline;
}

void sithTime_Advance(void);
void sithTime_Pause(void);
void sithTime_Resume(void);
void J3DAPI sithTime_SetFrameTime(uint32_t frameTime); //Added
void sithTime_Reset(void);
void J3DAPI sithTime_SetGameTime(uint32_t msecTime);
int sithTime_IsPaused(void);

// Helper hooking functions
void sithTime_InstallHooks(void);
void sithTime_ResetGlobals(void);

J3D_EXTERN_C_END
#endif // SITH_SITHTIME_H

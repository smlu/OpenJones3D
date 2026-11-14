#ifndef SITH_SITHTIME_H
#define SITH_SITHTIME_H
#include <j3dcore/j3d.h>
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
#else
#  define SITHTIME_ISFRAMECYCLE(offset, n) (SITH_ISFRAMECYCLE(offset, n))
#endif

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

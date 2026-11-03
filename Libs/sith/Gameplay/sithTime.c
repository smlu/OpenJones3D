#include "sithTime.h"
#include <j3dcore/j3dhook.h>

#include <sith/RTI/symbols.h>

#include <std/General/stdMath.h>
#include <std/General/stdPlatform.h>

// Added: Standalone builds have no retail executable to hold these. *_ResetGlobals sets the starting values.
uint32_t sithTime_g_frameTime;
float sithTime_g_frameTimeFlex;
float sithTime_g_fps;
uint32_t sithTime_g_msecGameTime;
float sithTime_g_secGameTime;
uint32_t sithTime_g_clockTime;
int sithTime_g_bPaused;

#define SITHTIME_MAXFRAMETIME 200 // 200ms

static uint32_t sithTime_msecPauseStartTime;

void sithTime_InstallHooks(void)
{
    J3D_HOOKFUNC(sithTime_Advance);
    J3D_HOOKFUNC(sithTime_Pause);
    J3D_HOOKFUNC(sithTime_Resume);
    J3D_HOOKFUNC(sithTime_Reset);
    J3D_HOOKFUNC(sithTime_SetGameTime);
    J3D_HOOKFUNC(sithTime_IsPaused);
}

void sithTime_ResetGlobals(void)
{
    memset(&sithTime_g_frameTime, 0, sizeof(sithTime_g_frameTime));
    memset(&sithTime_g_frameTimeFlex, 0, sizeof(sithTime_g_frameTimeFlex));
    memset(&sithTime_g_fps, 0, sizeof(sithTime_g_fps));
    memset(&sithTime_g_msecGameTime, 0, sizeof(sithTime_g_msecGameTime));
    memset(&sithTime_g_secGameTime, 0, sizeof(sithTime_g_secGameTime));
    memset(&sithTime_g_clockTime, 0, sizeof(sithTime_g_clockTime));
    memset(&sithTime_g_bPaused, 0, sizeof(sithTime_g_bPaused));
}

void sithTime_Advance(void)
{
    if ( !sithTime_g_clockTime || sithTime_g_bPaused )
    {
        return;
    }

    uint32_t curTime = stdPlatform_GetTimeMsec();
    if ( curTime >= sithTime_g_clockTime )
    {
        sithTime_g_frameTime = curTime - sithTime_g_clockTime;
    }
    else
    {
        SITHLOG_STATUS("Clock looped around.  Handling it.\n");
        sithTime_g_frameTime = curTime - sithTime_g_clockTime - 1;
    }

    sithTime_g_frameTime = STDMATH_CLAMP(sithTime_g_frameTime, 1, SITHTIME_MAXFRAMETIME); // min 1ms
    sithTime_g_clockTime = curTime;

    if ( (sithMain_g_sith_mode.debugModeFlags & SITHDEBUG_SLOWMODE) != 0 )
    {
        sithTime_g_frameTime = (uint32_t)((float)sithTime_g_frameTime * 0.2f); // frameTime / 5.0f
    }

    sithTime_g_frameTimeFlex = (float)sithTime_g_frameTime * 0.001f;
    if ( sithTime_g_frameTimeFlex <= 0.0f )
    {
        sithTime_g_frameTimeFlex = 1.0f / 10000.0f; // 0.000099999997f
    }

    sithTime_g_fps           = 1.0f / sithTime_g_frameTimeFlex;
    sithTime_g_msecGameTime += sithTime_g_frameTime;
    sithTime_g_secGameTime   = (float)sithTime_g_msecGameTime * 0.001f;
}

void sithTime_Pause(void)
{
    if ( !sithTime_g_bPaused )
    {
        sithTime_msecPauseStartTime = stdPlatform_GetTimeMsec();
        sithTime_g_bPaused = 1;
    }
}

void sithTime_Resume(void)
{
    if ( sithTime_g_bPaused )
    {
        sithTime_g_clockTime += stdPlatform_GetTimeMsec() - sithTime_msecPauseStartTime;
        sithTime_g_bPaused = 0;
    }
}

void J3DAPI sithTime_SetFrameTime(uint32_t frameTime)
{
    SITH_ASSERTREL(sithTime_g_clockTime != 0);
    sithTime_g_frameTime = frameTime;
    sithTime_g_clockTime = stdPlatform_GetTimeMsec();
    sithTime_g_frameTime = sithTime_g_frameTime = STDMATH_CLAMP(sithTime_g_frameTime, 10, SITHTIME_MAXFRAMETIME); // min 10ms;

    sithTime_g_frameTimeFlex = sithTime_g_frameTime * 0.001f;
    SITH_ASSERTREL(sithTime_g_frameTimeFlex > 0.0f);

    sithTime_g_fps           = 1.0f / sithTime_g_frameTimeFlex;
    sithTime_g_msecGameTime += sithTime_g_frameTime;
    sithTime_g_secGameTime   = (float)sithTime_g_msecGameTime * 0.001f;
}

void sithTime_Reset(void)
{
    sithTime_g_msecGameTime  = 0;
    sithTime_g_secGameTime   = 0.0f;
    sithTime_g_frameTime     = 0;
    sithTime_g_frameTimeFlex = 0.0f;
    sithTime_g_fps           = 0.0f;
    sithTime_g_clockTime     = stdPlatform_GetTimeMsec();
}

void J3DAPI sithTime_SetGameTime(uint32_t msecTime)
{
    sithTime_g_msecGameTime  = msecTime;
    sithTime_g_secGameTime   = (float)msecTime * 0.001f;
    sithTime_g_frameTime     = 0;
    sithTime_g_frameTimeFlex = 0.0f;
    sithTime_g_fps           = 0.0f;
    sithTime_g_clockTime     = stdPlatform_GetTimeMsec();
}

int J3DAPI sithTime_IsPaused()
{
    return sithTime_g_bPaused;
}

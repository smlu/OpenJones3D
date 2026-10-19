// Timer.c: the timer service (`TIMER=insane`) and its TIME_CRITICAL tick thread (original 0x004EA7D0-0x004EAE10).
#include <smush/insane/Timer/Timer.h>
#include <j3dcore/j3d.h>
#include <j3dcore/j3dhook.h>
#include <smush/RTI/symbols.h>

void Timer_InstallHooks(void)
{
    // Uncomment only lines for functions that have full definition and doesn't call original function (non-thunk functions)

    // J3D_HOOKFUNC(timer_Register);
    // J3D_HOOKFUNC(timer_Unregister);
    // J3D_HOOKFUNC(timer_Startup);
    // J3D_HOOKFUNC(timer_Shutdown);
    // J3D_HOOKFUNC(timer_Start);
    // J3D_HOOKFUNC(timer_Thread);
    // J3D_HOOKFUNC(timer_Stop);
    // J3D_HOOKFUNC(timer_Pause);
    // J3D_HOOKFUNC(timer_Resume);
    // J3D_HOOKFUNC(timer_Alloc);
    // J3D_HOOKFUNC(timer_Free);
    // J3D_HOOKFUNC(timer_SetInterval);
    // J3D_HOOKFUNC(timer_Wait);
    // J3D_HOOKFUNC(timer_GetTicks);
    // J3D_HOOKFUNC(timer_GetTickLen);
    // J3D_HOOKFUNC(timer_Sync);
    // J3D_HOOKFUNC(timer_Tick);
    // J3D_HOOKFUNC(timer_Poll);
    // J3D_HOOKFUNC(timer_RunDeferred);
}

void Timer_ResetGlobals(void)
{}

// Pcplay.c: the animation player (`ANIMPLAY=insane`) (original 0x004E3810-0x004E4230).
#include <smush/insane/Animate/Pcplay.h>
#include <j3dcore/j3d.h>
#include <j3dcore/j3dhook.h>
#include <smush/RTI/symbols.h>

void Pcplay_InstallHooks(void)
{
    // Uncomment only lines for functions that have full definition and doesn't call original function (non-thunk functions)

    // J3D_HOOKFUNC(animplay_Register);
    // J3D_HOOKFUNC(animplay_Unregister);
    // J3D_HOOKFUNC(animplay_Startup);
    // J3D_HOOKFUNC(animplay_Shutdown);
    // J3D_HOOKFUNC(animplay_Play);
    // J3D_HOOKFUNC(animplay_Create);
    // J3D_HOOKFUNC(animplay_GetTime);
    // J3D_HOOKFUNC(animplay_Pace);
    // J3D_HOOKFUNC(animplay_Draw);
    // J3D_HOOKFUNC(animplay_Destroy);
    // J3D_HOOKFUNC(animplay_SetRate);
    // J3D_HOOKFUNC(animplay_Rewind);
    // J3D_HOOKFUNC(animplay_IsDone);
    // J3D_HOOKFUNC(animplay_Step);
    // J3D_HOOKFUNC(animplay_TickCallback);
    // J3D_HOOKFUNC(animplay_PaceCallback);
    // J3D_HOOKFUNC(animplay_DebugName);
    // J3D_HOOKFUNC(animplay_SetDebug);
    // J3D_HOOKFUNC(animplay_GetDebug);
}

void Pcplay_ResetGlobals(void)
{}

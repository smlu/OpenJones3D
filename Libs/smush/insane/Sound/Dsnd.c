// Dsnd.c: the DirectSound driver (`SNDDRIVER=insane_dsnd`) (original 0x004E74B0-0x004E7D60).
#include <smush/insane/Sound/Dsnd.h>
#include <j3dcore/j3d.h>
#include <j3dcore/j3dhook.h>
#include <smush/RTI/symbols.h>

void Dsnd_InstallHooks(void)
{
    // Uncomment only lines for functions that have full definition and doesn't call original function (non-thunk functions)

    // J3D_HOOKFUNC(dsnd_Register);
    // J3D_HOOKFUNC(dsnd_Configure);
    // J3D_HOOKFUNC(dsnd_Unregister);
    // J3D_HOOKFUNC(dsnd_Startup);
    // J3D_HOOKFUNC(dsnd_Shutdown);
    // J3D_HOOKFUNC(dsnd_Start);
    // J3D_HOOKFUNC(dsnd_Stop);
    // J3D_HOOKFUNC(dsnd_SetVolume);
    // J3D_HOOKFUNC(dsnd_GetVolume);
    // J3D_HOOKFUNC(dsnd_GetRate);
    // J3D_HOOKFUNC(dsnd_Callback);
    // J3D_HOOKFUNC(dsnd_Init);
    // J3D_HOOKFUNC(dsnd_OpenSecondary);
    // J3D_HOOKFUNC(dsnd_Release);
    // J3D_HOOKFUNC(dsnd_GetPlayPos);
}

void Dsnd_ResetGlobals(void)
{}

// Readauto.c: the `READANIM=insane_auto` animation-file reader (original 0x004E33F0-0x004E3810).
#include <smush/insane/Image/Readauto.h>
#include <j3dcore/j3d.h>
#include <j3dcore/j3dhook.h>
#include <smush/RTI/symbols.h>

void Readauto_InstallHooks(void)
{
    // Uncomment only lines for functions that have full definition and doesn't call original function (non-thunk functions)

    // J3D_HOOKFUNC(readauto_Register);
    // J3D_HOOKFUNC(readauto_Unregister);
    // J3D_HOOKFUNC(readauto_Startup);
    // J3D_HOOKFUNC(readauto_Shutdown);
    // J3D_HOOKFUNC(readauto_Query);
    // J3D_HOOKFUNC(readauto_Read);
    // J3D_HOOKFUNC(readauto_Open);
    // J3D_HOOKFUNC(readauto_Close);
    // J3D_HOOKFUNC(readauto_Op3);
    // J3D_HOOKFUNC(readauto_Op4);
}

void Readauto_ResetGlobals(void)
{}

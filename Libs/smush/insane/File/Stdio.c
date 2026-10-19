// Stdio.c: the `FILIO=insane_stdio` module (C stdio) (original 0x004EA020-0x004EA0F0).
#include <smush/insane/File/Stdio.h>
#include <j3dcore/j3d.h>
#include <j3dcore/j3dhook.h>
#include <smush/RTI/symbols.h>

void Stdio_InstallHooks(void)
{
    // Uncomment only lines for functions that have full definition and doesn't call original function (non-thunk functions)

    // J3D_HOOKFUNC(stdio_Register);
    // J3D_HOOKFUNC(stdio_Unregister);
    // J3D_HOOKFUNC(stdio_open);
    // J3D_HOOKFUNC(stdio_close);
    // J3D_HOOKFUNC(stdio_flush);
}

void Stdio_ResetGlobals(void)
{}

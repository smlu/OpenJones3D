// Bitmap.c: the bitmap service (`BITMAP=insane`) (original 0x004EA0F0-0x004EA7D0).
#include <smush/insane/Bitmap/Bitmap.h>
#include <j3dcore/j3d.h>
#include <j3dcore/j3dhook.h>
#include <smush/RTI/symbols.h>

void Bitmap_InstallHooks(void)
{
    // Uncomment only lines for functions that have full definition and doesn't call original function (non-thunk functions)

    // J3D_HOOKFUNC(bitmap_Register);
    // J3D_HOOKFUNC(bitmap_Unregister);
    // J3D_HOOKFUNC(bitmap_Startup);
    // J3D_HOOKFUNC(bitmap_Shutdown);
    // J3D_HOOKFUNC(bitmap_Create);
    // J3D_HOOKFUNC(bitmap_CreateEx);
    // J3D_HOOKFUNC(bitmap_Destroy);
}

void Bitmap_ResetGlobals(void)
{}

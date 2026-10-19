// Zfilio.c: the `FILIO=insane_zfilio` module (gz-compressed files through the executable's zlib) (original 0x004E9B30-0x004EA020).
#include <smush/insane/File/Zfilio/Zfilio.h>
#include <j3dcore/j3d.h>
#include <j3dcore/j3dhook.h>
#include <smush/RTI/symbols.h>

void Zfilio_InstallHooks(void)
{
    // Uncomment only lines for functions that have full definition and doesn't call original function (non-thunk functions)

    // J3D_HOOKFUNC(zfilio_Register);
    // J3D_HOOKFUNC(zfilio_Configure);
    // J3D_HOOKFUNC(zfilio_Unregister);
    // J3D_HOOKFUNC(zfilio_Startup);
    // J3D_HOOKFUNC(zfilio_Shutdown);
    // J3D_HOOKFUNC(zfilio_open);
    // J3D_HOOKFUNC(zfilio_close);
    // J3D_HOOKFUNC(zfilio_read);
    // J3D_HOOKFUNC(zfilio_write);
    // J3D_HOOKFUNC(zfilio_tell);
    // J3D_HOOKFUNC(zfilio_seek);
    // J3D_HOOKFUNC(zfilio_flush);
}

void Zfilio_ResetGlobals(void)
{}

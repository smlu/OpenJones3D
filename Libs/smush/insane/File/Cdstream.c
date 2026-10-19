// Cdstream.c: the file ring-buffer streamer (`STREAMER=insane_streamer`) (original 0x004E4D50-0x004E5470).
#include <smush/insane/File/Cdstream.h>
#include <j3dcore/j3d.h>
#include <j3dcore/j3dhook.h>
#include <smush/RTI/symbols.h>

void Cdstream_InstallHooks(void)
{
    // Uncomment only lines for functions that have full definition and doesn't call original function (non-thunk functions)

    // J3D_HOOKFUNC(streamer_Register);
    // J3D_HOOKFUNC(streamer_Unregister);
    // J3D_HOOKFUNC(streamer_Startup);
    // J3D_HOOKFUNC(streamer_Shutdown);
    // J3D_HOOKFUNC(streamer_Create);
    // J3D_HOOKFUNC(streamer_Destroy);
    // J3D_HOOKFUNC(streamer_Reset);
    // J3D_HOOKFUNC(streamer_Fill);
    // J3D_HOOKFUNC(streamer_SetFile);
    // J3D_HOOKFUNC(streamer_Prime);
    // J3D_HOOKFUNC(streamer_IsActive);
    // J3D_HOOKFUNC(streamer_Advance);
    // J3D_HOOKFUNC(streamer_Cancel);
    // J3D_HOOKFUNC(streamer_InRange);
    // J3D_HOOKFUNC(streamer_Percent);
}

void Cdstream_ResetGlobals(void)
{}

// Frame.c: the animation frame service (`ANIMFRAME=insane`): parses and decodes frames through the frame-object decoders (original 0x004E4230-0x004E4D50).
#include <smush/insane/Animate/Frame.h>
#include <j3dcore/j3d.h>
#include <j3dcore/j3dhook.h>
#include <smush/RTI/symbols.h>

void Frame_InstallHooks(void)
{
    // Uncomment only lines for functions that have full definition and doesn't call original function (non-thunk functions)

    // J3D_HOOKFUNC(animframe_Register);
    // J3D_HOOKFUNC(animframe_Unregister);
    // J3D_HOOKFUNC(animframe_Startup);
    // J3D_HOOKFUNC(animframe_Shutdown);
    // J3D_HOOKFUNC(animframe_ResetDecoders);
    // J3D_HOOKFUNC(animframe_Parse);
    // J3D_HOOKFUNC(animframe_Decode);
    // J3D_HOOKFUNC(animframe_Load);
    // J3D_HOOKFUNC(animframe_Fetch);
    // J3D_HOOKFUNC(animframe_Draw);
    // J3D_HOOKFUNC(animframe_DebugName);
    // J3D_HOOKFUNC(animframe_SetDebug);
    // J3D_HOOKFUNC(animframe_GetDebug);
}

void Frame_ResetGlobals(void)
{}

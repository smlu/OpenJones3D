// Framestream.c: the chunked frame stream the animation player drives (original 0x004EBDE0-0x004EC560).
#include <smush/insane/File/Framestream.h>
#include <j3dcore/j3d.h>
#include <j3dcore/j3dhook.h>
#include <smush/RTI/symbols.h>

void Framestream_InstallHooks(void)
{
    // Uncomment only lines for functions that have full definition and doesn't call original function (non-thunk functions)

    // J3D_HOOKFUNC(framestream_Reset);
    // J3D_HOOKFUNC(framestream_Tick);
    // J3D_HOOKFUNC(framestream_Fetch);
    // J3D_HOOKFUNC(framestream_Continue);
    // J3D_HOOKFUNC(framestream_Present);
    // J3D_HOOKFUNC(framestream_Header);
    // J3D_HOOKFUNC(framestream_IsOpen);
    // J3D_HOOKFUNC(framestream_Stall);
    // J3D_HOOKFUNC(framestream_Open);
    // J3D_HOOKFUNC(framestream_Seek);
    // J3D_HOOKFUNC(framestream_Mode);
    // J3D_HOOKFUNC(framestream_Copy);
    // J3D_HOOKFUNC(framestream_CopyB);
}

void Framestream_ResetGlobals(void)
{}

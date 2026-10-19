// Mixer.c: the software mixer (original 0x004EC720-0x004EDEF0).
#include <smush/insane/Sound/Mixer.h>
#include <j3dcore/j3d.h>
#include <j3dcore/j3dhook.h>
#include <smush/RTI/symbols.h>

void Mixer_InstallHooks(void)
{
    // Uncomment only lines for functions that have full definition and doesn't call original function (non-thunk functions)

    // J3D_HOOKFUNC(mixer_Init);
    // J3D_HOOKFUNC(mixer_Done);
    // J3D_HOOKFUNC(mixer_Begin);
    // J3D_HOOKFUNC(mixer_Feed);
    // J3D_HOOKFUNC(mixer_Mix);
    // J3D_HOOKFUNC(mixer_Setup);
    // J3D_HOOKFUNC(mixer_Free);
    // J3D_HOOKFUNC(mixer_Clear);
    // J3D_HOOKFUNC(mixer_Add);
    // J3D_HOOKFUNC(mixer_Output);
    // J3D_HOOKFUNC(mixer_Mono8);
    // J3D_HOOKFUNC(mixer_Stereo8);
    // J3D_HOOKFUNC(mixer_Mono16);
    // J3D_HOOKFUNC(mixer_Stereo16);
    // J3D_HOOKFUNC(mixer_MonoU16);
    // J3D_HOOKFUNC(mixer_StereoU16);
    // J3D_HOOKFUNC(mixer_MonoSwap16);
    // J3D_HOOKFUNC(mixer_StereoSwap16);
    // J3D_HOOKFUNC(mixer_Blit);
}

void Mixer_ResetGlobals(void)
{}

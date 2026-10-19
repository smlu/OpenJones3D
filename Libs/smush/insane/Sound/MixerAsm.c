// MixerAsm.c: C translation of INSANE's MASM source MixerAsm.asm (mixer buffer procedures) (original 0x004EC560-0x004EC71F).
#include <smush/insane/Sound/MixerAsm.h>
#include <j3dcore/j3d.h>
#include <j3dcore/j3dhook.h>
#include <smush/RTI/symbols.h>

void MixerAsm_InstallHooks(void)
{
    // Uncomment only lines for functions that have full definition and doesn't call original function (non-thunk functions)

    // J3D_HOOKFUNC(mixer_MemCopy);
    // J3D_HOOKFUNC(mixer_MemFill);
    // J3D_HOOKFUNC(mixer_Lut16);
    // J3D_HOOKFUNC(mixer_Lut16Add);
    // J3D_HOOKFUNC(mixer_Fill16);
    // J3D_HOOKFUNC(mixer_Lut8);
    // J3D_HOOKFUNC(mixer_Lut8Add);
    // J3D_HOOKFUNC(mixer_FillB);
}

void MixerAsm_ResetGlobals(void)
{}

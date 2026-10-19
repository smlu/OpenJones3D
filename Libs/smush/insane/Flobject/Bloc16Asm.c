// Bloc16Asm.c: C translation of INSANE's MASM source Bloc16Asm.asm (blocky16 codec 2 block decoder) (original 0x004EDEF0-0x004EE02C).
#include <smush/insane/Flobject/Bloc16Asm.h>
#include <j3dcore/j3d.h>
#include <j3dcore/j3dhook.h>
#include <smush/RTI/symbols.h>

void Bloc16Asm_InstallHooks(void)
{
    // Uncomment only lines for functions that have full definition and doesn't call original function (non-thunk functions)

    // J3D_HOOKFUNC(dbloc16_Interpolate);
    // J3D_HOOKFUNC(dbloc16_Codec2);
}

void Bloc16Asm_ResetGlobals(void)
{}

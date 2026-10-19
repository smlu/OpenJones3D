// BlitAsm.c: C translation of INSANE's MASM source BlitAsm.asm (blit_Masm) (original 0x004F2220-0x004F22B7).
#include <smush/insane/Bitmap/BlitAsm.h>
#include <j3dcore/j3d.h>
#include <j3dcore/j3dhook.h>
#include <smush/RTI/symbols.h>

void BlitAsm_InstallHooks(void)
{
    // Uncomment only lines for functions that have full definition and doesn't call original function (non-thunk functions)

    // J3D_HOOKFUNC(blit_Masm);
}

void BlitAsm_ResetGlobals(void)
{}

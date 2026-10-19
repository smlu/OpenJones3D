// MemCopy.c: C translation of INSANE's MASM source MemCopy.asm (blit_MemCopy) (original 0x004F04D0-0x004F04F5).
#include <smush/insane/Bitmap/MemCopy.h>
#include <j3dcore/j3d.h>
#include <j3dcore/j3dhook.h>
#include <smush/RTI/symbols.h>

void MemCopy_InstallHooks(void)
{
    // Uncomment only lines for functions that have full definition and doesn't call original function (non-thunk functions)

    // J3D_HOOKFUNC(blit_MemCopy);
}

void MemCopy_ResetGlobals(void)
{}

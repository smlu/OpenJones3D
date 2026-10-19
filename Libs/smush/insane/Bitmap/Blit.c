// Blit.c: C wrappers around the MASM blitters (original 0x004F0460-0x004F0590).
#include <smush/insane/Bitmap/Blit.h>
#include <j3dcore/j3d.h>
#include <j3dcore/j3dhook.h>
#include <smush/RTI/symbols.h>

void Blit_InstallHooks(void)
{
    // Uncomment only lines for functions that have full definition and doesn't call original function (non-thunk functions)

    // J3D_HOOKFUNC(blit_Fill);
    // J3D_HOOKFUNC(blit_Wrapper);
}

void Blit_ResetGlobals(void)
{}

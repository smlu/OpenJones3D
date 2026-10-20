// MemFill.c: C translation of INSANE's MASM source MemFill.asm (blit_MemCopyB) (original 0x004F04F8-0x004F054F).
#include <smush/insane/Bitmap/MemFill.h>
#include <j3dcore/j3d.h>
#include <j3dcore/j3dhook.h>
#include <smush/RTI/addresses.h>
#include <smush/RTI/symbols.h>

#include <string.h>

void MemFill_InstallHooks(void)
{
    // Uncomment only lines for functions that have full definition and doesn't call original function (non-thunk functions)

    J3D_HOOKFUNC(blit_MemCopyB);
}

void MemFill_ResetGlobals(void)
{}

void J3DAPI blit_MemCopyB(void* pDst, uint8_t value, int size)
{
    // The original replicates value into a dword and stores the size bytes in 20000-byte runs, then the size & 3 odd bytes,
    // then whole dwords. Every byte gets the same value, so the result is a plain fill of size bytes.
    memset(pDst, value, (uint32_t)size);
}

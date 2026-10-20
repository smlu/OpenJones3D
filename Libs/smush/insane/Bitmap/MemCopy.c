// MemCopy.c: C translation of INSANE's MASM source MemCopy.asm (blit_MemCopy) (original 0x004F04D0-0x004F04F5).
#include <smush/insane/Bitmap/MemCopy.h>
#include <j3dcore/j3d.h>
#include <j3dcore/j3dhook.h>
#include <smush/RTI/symbols.h>

#include <string.h>

void MemCopy_InstallHooks(void)
{
    // Uncomment only lines for functions that have full definition and doesn't call original function (non-thunk functions)

    J3D_HOOKFUNC(blit_MemCopy);
}

void MemCopy_ResetGlobals(void)
{}

void J3DAPI blit_MemCopy(void* pDst, const void* pSrc, int size)
{
    // The size & 3 odd bytes first (rep movsb), then whole dwords (rep movsd); both forward and element by element,
    // so overlapping buffers behave like the original's string instructions.
    uint8_t* pOut      = (uint8_t*)pDst;
    const uint8_t* pIn = (const uint8_t*)pSrc;

    for ( uint32_t count = (uint32_t)size & 3; count != 0; --count )
    {
        *pOut++ = *pIn++;
    }

    for ( uint32_t count = (uint32_t)size >> 2; count != 0; --count )
    {
        uint32_t value;
        memcpy(&value, pIn, sizeof(value));
        memcpy(pOut, &value, sizeof(value));
        pIn  += sizeof(value);
        pOut += sizeof(value);
    }
}



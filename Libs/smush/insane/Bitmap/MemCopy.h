#ifndef SMUSH_INSANE_BITMAP_MEMCOPY_H
#define SMUSH_INSANE_BITMAP_MEMCOPY_H
#include <j3dcore/j3d.h>
#include <smush/RTI/addresses.h>

/* MemCopy.asm: blit_MemCopy copies size bytes, the size & 3 odd bytes first, then whole dwords. */

J3D_EXTERN_C_START

void J3DAPI blit_MemCopy(void* pDst, const void* pSrc, int size);

void MemCopy_InstallHooks(void);
void MemCopy_ResetGlobals(void);

J3D_EXTERN_C_END

#endif // SMUSH_INSANE_BITMAP_MEMCOPY_H

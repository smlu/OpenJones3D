#ifndef SMUSH_INSANE_BITMAP_MEMFILL_H
#define SMUSH_INSANE_BITMAP_MEMFILL_H
#include <j3dcore/j3d.h>

/*
 * MemFill.asm: blit_MemCopyB replicates the low byte of value into a dword and stores size bytes, in 20000-byte
 * rep stosd runs, then the size & 3 odd bytes, then whole dwords.
 */

J3D_EXTERN_C_START

void J3DAPI blit_MemCopyB(void* pDst, uint8_t value, int size);

void MemFill_InstallHooks(void);
void MemFill_ResetGlobals(void);

J3D_EXTERN_C_END

#endif // SMUSH_INSANE_BITMAP_MEMFILL_H

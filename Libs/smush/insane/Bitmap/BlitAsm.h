#ifndef SMUSH_INSANE_BITMAP_BLITASM_H
#define SMUSH_INSANE_BITMAP_BLITASM_H
#include <j3dcore/j3d.h>

/*
 * BlitAsm.asm: blit_Masm(pSrc, pDst, srcX, srcY, dstX, dstY, width, height, srcPitch, dstPitch) copies a width x height
 * rectangle row by row (rep movsd, then the width & 3 odd bytes). The pitches multiply as 16-bit values (mul bx / mul dx):
 * preserve that.
 */

J3D_EXTERN_C_START

void J3DAPI blit_Masm(void* pSrc, void* pDst, int srcX, int srcY, int dstX, int dstY, int width, int height, int srcPitch, int dstPitch);

void BlitAsm_InstallHooks(void);
void BlitAsm_ResetGlobals(void);

J3D_EXTERN_C_END

#endif // SMUSH_INSANE_BITMAP_BLITASM_H

#ifndef SMUSH_INSANE_BITMAP_BLIT_H
#define SMUSH_INSANE_BITMAP_BLIT_H
#include <j3dcore/j3d.h>
#include <smush/RTI/addresses.h>
#include <smush/insane/Bitmap/Bitmap.h>

/*
 * Blit.c's C wrappers around the MASM blitter: blit_Fill clears a bitmap to one colour (the frame stream's clear),
 * blit_Wrapper reorders a clipped copy's arguments for BlitAsm.asm (Mixer.c's blit).
 */

J3D_EXTERN_C_START

void J3DAPI blit_Fill(LecBitmapHeader* pBitmap, int color);
void J3DAPI blit_Wrapper(void* pDst, void* pSrc, int a3, int a4, int a5, int a6, int width, int height, int dstPitch, int srcPitch);

void Blit_InstallHooks(void);
void Blit_ResetGlobals(void);

J3D_EXTERN_C_END

#endif // SMUSH_INSANE_BITMAP_BLIT_H

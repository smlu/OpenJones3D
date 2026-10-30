#ifndef SMUSH_INSANE_DECODE_DECODE_H
#define SMUSH_INSANE_DECODE_DECODE_H
#include <j3dcore/j3d.h>
#include <smush/RTI/addresses.h>
#include <smush/insane/Bitmap/Bitmap.h>

/*
 * INSANE's pixel-format conversion and fill (Decode.c), used by the blitters (Blit.c, Mixer.c) and the blocky16 decoder
 * (D_bloc16.c). decode_Begin converts n pixels from 32-bit RGBA to the bitmap's format; decode_Run fills n pixels of the
 * bitmap's format with one converted pixel.
 */

J3D_EXTERN_C_START

void J3DAPI decode_Rle(uint8_t* pDst, uint8_t* pSrc, int n);
void J3DAPI decode_RleInit(uint8_t* pSrc);
void decode_RleEnd(void); // Folded onto 0x004EE700; no address of its own, never hook
uint8_t decode_RleByte(void);
void J3DAPI decode_Run(uint8_t* pDst, LecBitmapHeader* pBitmap, int n, uint8_t* pPixel);
int J3DAPI decode_MakeTable(LecBitmapHeader* pSrcBitmap, LecBitmapHeader* pDstBitmap, void* pTable, void* pPalette);
int J3DAPI decode_Convert(uint8_t* pSrc, LecBitmapHeader* pSrcBitmap, int n, void* pPalette, uint8_t* pDst, LecBitmapHeader* pDstBitmap, void* pTable);
int J3DAPI decode_ToRGBA(uint8_t* pSrc, LecBitmapHeader* pBitmap, int n, void* pPalette, uint8_t* pDst);
int J3DAPI decode_Begin(uint8_t* pDst, LecBitmapHeader* pBitmap, int n, void* pPalette, uint8_t* pSrc);

void Decode_InstallHooks(void);
void Decode_ResetGlobals(void);

J3D_EXTERN_C_END

#endif // SMUSH_INSANE_DECODE_DECODE_H

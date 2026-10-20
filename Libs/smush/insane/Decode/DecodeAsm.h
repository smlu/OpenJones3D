#ifndef SMUSH_INSANE_DECODE_DECODEASM_H
#define SMUSH_INSANE_DECODE_DECODEASM_H
#include <j3dcore/j3d.h>
#include <smush/RTI/addresses.h>
#include <stdint.h>

/*
 * DecodeAsm.asm: INSANE's pixel-format converters. decode_Lut8/decode_Lut16(pDst, pSrc, n, dstSize, pTable) map 8- or
 * 16-bit source pixels through a table to 1-, 2-, 3- or 4-byte destination pixels (dstSize); the other six convert n
 * pixels between 32-bit RGBA and 555, 565 and byte-swapped 565 words, two pixels per iteration after an odd first one.
 */

J3D_EXTERN_C_START

void J3DAPI decode_Lut8(void* pDst, const void* pSrc, int n, int dstSize, const void* pTable);
void J3DAPI decode_Lut16(void* pDst, const void* pSrc, int n, int dstSize, const void* pTable);
void J3DAPI decode_RGBAto555(void* pDst, const void* pSrc, int n);
void J3DAPI decode_555toRGBA(void* pDst, const void* pSrc, int n);
void J3DAPI decode_RGBAto565(void* pDst, const void* pSrc, int n);
void J3DAPI decode_565toRGBA(void* pDst, const void* pSrc, int n);
void J3DAPI decode_RGBAto565B(void* pDst, const void* pSrc, int n);
void J3DAPI decode_565BtoRGBA(void* pDst, const void* pSrc, int n);

void DecodeAsm_InstallHooks(void);
void DecodeAsm_ResetGlobals(void);

J3D_EXTERN_C_END

#endif // SMUSH_INSANE_DECODE_DECODEASM_H

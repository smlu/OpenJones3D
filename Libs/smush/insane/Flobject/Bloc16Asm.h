#ifndef SMUSH_INSANE_FLOBJECT_BLOC16ASM_H
#define SMUSH_INSANE_FLOBJECT_BLOC16ASM_H
#include <j3dcore/j3d.h>
#include <smush/RTI/addresses.h>
#include <stdint.h>
#include <smush/insane/Flobject/D_bloc16.h>

/*
 * Bloc16Asm.asm: blocky16's block decoder. dbloc16_Codec2 decodes codec 2's 8x8 blocks: each opcode byte selects a
 * motion-vector copy from the previous or delta frame, a solid fill, a two-colour glyph or a split into four 4x4 (and
 * then 2x2) blocks. dbloc16_Interpolate (a 2x upscale through a pair-interpolation table) has no caller in the original.
 *
 * The block procedures dbloc16_Block8/Block4/Block2 (original 0x004EE02C, 0x004EE3C4, 0x004EE58C) are internal subroutines
 * with a register interface (esi = opcode stream, edi = destination, ebp = line length) and are not declared here: translate
 * them as static helpers of Bloc16Asm.c. The assembly's own data (dbloc16_prevOffset, deltaOffset, width, motion, colors,
 * glyph4, glyph8, palette, rowSkip, blocksX, countX, blocksY) is private to Bloc16Asm.c as well.
 */

J3D_EXTERN_C_START

void J3DAPI dbloc16_Interpolate(uint8_t* pDst, const uint8_t* pSrc, int stride, int rows, const uint8_t* pTable); // No caller in the original; parameter roles inferred from the assembly
void J3DAPI dbloc16_Codec2(uint16_t* pDst, uint8_t* pSrc, int offset1, int offset2, int width, int16_t* pMotion, uint8_t* pParams, int height, int nextLine, Bl16Small* pSmall, Bl16Big* pBig, uint16_t* pPalette);

void Bloc16Asm_InstallHooks(void);
void Bloc16Asm_ResetGlobals(void);

J3D_EXTERN_C_END

#endif // SMUSH_INSANE_FLOBJECT_BLOC16ASM_H

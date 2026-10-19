#ifndef SMUSH_INSANE_SOUND_MIXERASM_H
#define SMUSH_INSANE_SOUND_MIXERASM_H
#include <j3dcore/j3d.h>
#include <stdint.h>

/*
 * MixerAsm.asm: eight cdecl procedures that copy, fill and table-convert sample buffers. Engine.c calls mixer_MemCopy and
 * mixer_MemFill; nothing in the original calls the other six (they are original code all the same). Arguments: pDst, pSrc or
 * value, n; the table procedures also take pTables and the table index (512-byte word tables, 256-byte byte tables).
 */

J3D_EXTERN_C_START

void J3DAPI mixer_MemCopy(void* pDst, const void* pSrc, int n);
void J3DAPI mixer_MemFill(void* pDst, int value, int n);
void J3DAPI mixer_Lut16(uint16_t* pDst, const uint8_t* pSrc, int n, const uint16_t* pTables, int tableIndex); // Unused by the original
void J3DAPI mixer_Lut16Add(uint16_t* pDst, const uint8_t* pSrc, int n, const uint16_t* pTables, int tableIndex); // Unused by the original
void J3DAPI mixer_Fill16(uint16_t* pDst, int value, int n); // Unused by the original
void J3DAPI mixer_Lut8(uint8_t* pDst, const uint8_t* pSrc, int n, const uint8_t* pTables, int tableIndex); // Unused by the original
void J3DAPI mixer_Lut8Add(uint8_t* pDst, const uint8_t* pSrc, int n, const uint8_t* pTables, int tableIndex); // Unused by the original
void J3DAPI mixer_FillB(void* pDst, int value, int n); // Unused by the original

void MixerAsm_InstallHooks(void);
void MixerAsm_ResetGlobals(void);

J3D_EXTERN_C_END

#endif // SMUSH_INSANE_SOUND_MIXERASM_H

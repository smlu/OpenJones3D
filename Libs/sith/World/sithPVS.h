#ifndef SITH_SITHPVS_H
#define SITH_SITHPVS_H
#include <j3dcore/j3d.h>
#include <rdroid/types.h>
#include <sith/types.h>
#include <sith/Main/sithMain.h>
#include <std/types.h>

J3D_EXTERN_C_START

int J3DAPI sithPVS_WritePVSText(const SithWorld* pWorld); // Added from debug
int J3DAPI sithPVS_ReadPVSText(SithWorld* pWorld, int bSkip);

int J3DAPI sithPVS_WritePVSBinary(tFileHandle fh, const SithWorld* pWorld);
int J3DAPI sithPVS_ReadPVSBinary(tFileHandle fh, SithWorld* pWorld);
void J3DAPI sithPVS_SetTable(uint8_t* paAdjoinTable, const uint8_t* aElements, size_t numAdjoints);

void sithPVS_ResetGlobals(void);

J3D_EXTERN_C_END
#endif // SITH_SITHPVS_H

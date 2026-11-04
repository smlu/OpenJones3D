#ifndef SITH_SITHSECTOR_H
#define SITH_SITHSECTOR_H
#include <j3dcore/j3d.h>
#include <rdroid/types.h>
#include <sith/types.h>
#include <sith/Main/sithMain.h>
#include <std/types.h>

J3D_EXTERN_C_START

int J3DAPI sithSector_WriteSectorsListText(const SithWorld* pWorld); // Added
int J3DAPI sithSector_ReadSectorsListText(SithWorld* pWorld, int bSkip);

int J3DAPI sithSector_WriteSectorsListBinary(tFileHandle fh, const SithWorld* pWorld);
int J3DAPI sithSector_ReadSectorsListBinary(tFileHandle fh, SithWorld* pWorld);

int J3DAPI sithSector_ValidateSectorPointer(const SithWorld* pWorld, const SithSector* pSector);

int J3DAPI sithSector_AllocWorldSectors(SithWorld* pWorld, size_t numSectors);
void J3DAPI sithSector_ResetAllSectors(SithWorld* pWorld);
void J3DAPI sithSector_FreeWorldSectors(SithWorld* pWorld);

SithSector* J3DAPI sithSector_FindSectorAtPos(const SithWorld* pWorld, const rdVector3* pos);

void J3DAPI sithSector_HideSectorAdjoins(SithSector* pSector);
void J3DAPI sithSector_ShowSectorAdjoins(SithSector* pSector);

void J3DAPI sithSector_SetSectorFlags(SithSector* pSector, SithSectorFlag flags);
void J3DAPI sithSector_ClearSectorFlags(SithSector* pSector, SithSectorFlag flags);

size_t J3DAPI sithSector_GetSectorThingCount(const SithSector* pSector);
size_t J3DAPI sithSector_GetSectorPlayerCount(const SithSector* pSector);
size_t J3DAPI sithSector_GetAdjoinCount(const SithSector* pSector); // Added

void J3DAPI sithSector_SyncSector(SithSector* pSector, int flags);
size_t sithSector_SyncSectors(void);

SithSector* J3DAPI sithSector_GetSectorEx(const SithWorld* pWorld, int idx);
int J3DAPI sithSector_GetSectorIndexEx(const SithWorld* pWorld, const SithSector* pSector);
int J3DAPI sithSector_GetSectorIndex(const SithSector* pSector);

// Helper hooking functions
void sithSector_ResetGlobals(void);

J3D_EXTERN_C_END
#endif // SITH_SITHSECTOR_H

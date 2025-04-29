#ifndef SITH_SITHMATERIAL_H
#define SITH_SITHMATERIAL_H
#include <j3dcore/j3d.h>
#include <rdroid/types.h>
#include <sith/types.h>
#include <sith/Main/sithMain.h>
#include <sith/RTI/addresses.h>
#include <std/types.h>

J3D_EXTERN_C_START

int sithMaterial_Startup(void);
void sithMaterial_Shutdown(void);

int J3DAPI sithMaterial_AllocWorldMaterials(SithWorld* pWorld, size_t numMaterials);
void J3DAPI sithMaterial_FreeWorldMaterials(SithWorld* pWorld);

int J3DAPI sithMaterial_WriteMaterialsListText(const SithWorld* pWorld); // Added from debug
int J3DAPI sithMaterial_ReadMaterialsListText(SithWorld* pWorld, int bSkip);

int J3DAPI sithMaterial_WriteMaterialsListBinary(tFileHandle fh, const SithWorld* pWorld);
int J3DAPI sithMaterial_ReadMaterialsListBinary(tFileHandle fh, SithWorld* pWorld);

rdMaterial* J3DAPI sithMaterial_Load(const char* pName);
rdMaterial* J3DAPI sithMaterial_GetMaterialByIndex(int idx);

// Helper hooking functions
void sithMaterial_InstallHooks(void);
void sithMaterial_ResetGlobals(void);

J3D_EXTERN_C_END
#endif // SITH_SITHMATERIAL_H

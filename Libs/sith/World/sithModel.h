#ifndef SITH_SITHMODEL_H
#define SITH_SITHMODEL_H
#include <j3dcore/j3d.h>

#include <rdroid/types.h>

#include <sith/types.h>
#include <sith/Main/sithMain.h>

J3D_EXTERN_C_START

int sithModel_Startup(void);
void sithModel_Shutdown(void);

// Loading functions which reads NDY models section and loads models
int J3DAPI sithModel_WriteStaticModelsListText(const SithWorld* pWorld); // Added
int J3DAPI sithModel_ReadStaticModelsListText(SithWorld* pWorld, int bSkip);

// Loading functions which reads CND models section and loads models
int J3DAPI sithModel_WriteStaticModelsListBinary(tFileHandle fh, const SithWorld* pWorld);
int J3DAPI sithModel_ReadStaticModelsListBinary(tFileHandle fh, SithWorld* pWorld);

rdModel3* J3DAPI sithModel_Load(const char* pName, int bSkipDefault);

int J3DAPI sithModel_AllocWorldModels(SithWorld* pWorld, size_t size);
void J3DAPI sithModel_FreeWorldModels(SithWorld* pWorld);
size_t  J3DAPI sithModel_GetModelMemUsage(const rdModel3* pModel);

rdModel3* J3DAPI sithModel_GetModel(const char* pName);
rdModel3* J3DAPI sithModel_GetModelByIndex(size_t modelIdx);
int J3DAPI sithModel_GetModelIndex(const rdModel3* pModel);
int J3DAPI sithModel_GetMeshIndex(const rdModel3* pModel, const char* pMeshName);

void J3DAPI sithModel_EnableHiPoly(int bEnabled);
bool sithModel_IsHiPolyEnabled(void); // Added

// Helper hooking functions
void sithModel_ResetGlobals(void);

J3D_EXTERN_C_END
#endif // SITH_SITHMODEL_H

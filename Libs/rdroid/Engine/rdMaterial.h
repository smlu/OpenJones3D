#ifndef RDROID_RDMATERIAL_H
#define RDROID_RDMATERIAL_H
#include <j3dcore/j3d.h>
#include <rdroid/types.h>
#include <std/types.h>

J3D_EXTERN_C_START

rdMaterialLoaderFunc J3DAPI rdMaterial_RegisterLoader(rdMaterialLoaderFunc pLoader);
rdMaterialUnloaderFunc J3DAPI rdMaterial_RegisterUnloader(rdMaterialUnloaderFunc pFunc); // Added

rdMaterial* J3DAPI rdMaterial_Load(const char* pFilename);
int J3DAPI rdMaterial_LoadEntry(const char* pFilename, rdMaterial* pMat);
void J3DAPI rdMaterial_Free(rdMaterial* pMaterial);
void J3DAPI rdMaterial_FreeEntry(rdMaterial* pMaterial);

int J3DAPI rdMaterial_Write(const char* pFilename, const rdMaterial* pMaterial, tVBuffer*** paTextures, size_t numMipLevels); // Added

size_t J3DAPI rdMaterial_GetMaterialMemUsage(const rdMaterial* pMaterial);
size_t J3DAPI rdMaterial_GetMipSize(int width, int height, size_t mipLevels);

// Helper hooking functions
void rdMaterial_ResetGlobals(void);

J3D_EXTERN_C_END
#endif // RDROID_RDMATERIAL_H

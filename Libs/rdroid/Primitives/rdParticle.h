#ifndef RDROID_RDPARTICLE_H
#define RDROID_RDPARTICLE_H
#include <j3dcore/j3d.h>
#include <rdroid/types.h>

#define RDPARTICLE_MAXVERTS 256

J3D_EXTERN_C_START
typedef rdParticle* (J3DAPI* rdParticleLoaderFunc)(const char* pFilename);

rdParticleLoaderFunc J3DAPI rdParticle_RegisterLoader(rdParticleLoaderFunc pFunc); // Added

rdParticle* J3DAPI rdParticle_New(size_t num, float size, rdMaterial* pMat, rdLightMode lightMode);
int J3DAPI rdParticle_NewEntry(rdParticle* pParticle, size_t num, float size, rdMaterial* pMaterial, rdLightMode lightMode);
rdParticle* J3DAPI rdParticle_Duplicate(const rdParticle* pOriginal);

void J3DAPI rdParticle_Free(rdParticle* pParticle);
void J3DAPI rdParticle_FreeEntry(rdParticle* pParticle);

rdParticle* J3DAPI rdParticle_Load(char* pFilename); // Added
int J3DAPI rdParticle_LoadEntry(const char* pFilename, rdParticle* pParticle);
int J3DAPI rdParticle_Write(const char* pFilename, rdParticle* pParticle, const char* pCreatedName); // Added

int J3DAPI rdParticle_Draw(const rdThing* pParticle, const rdMatrix34* pOrient);

void rdParticle_ResetGlobals(void);

J3D_EXTERN_C_END
#endif // RDROID_RDPARTICLE_H

#ifndef SITH_SITHRENDERSKY_H
#define SITH_SITHRENDERSKY_H
#include <j3dcore/j3d.h>
#include <rdroid/types.h>
#include <sith/types.h>
#include <sith/Main/sithMain.h>
#include <sith/RTI/addresses.h>
#include <std/types.h>

J3D_EXTERN_C_START

int J3DAPI sithRenderSky_Open(float horizonDistance, float ceilingHeight);
void sithRenderSky_Close(void); // Added

void sithRenderSky_Update(void);

// Function transforms horizon sky surface vertices to NDC screen space
// 
// aVerts should be transformed to clip space.
void J3DAPI sithRenderSky_HorizonFaceToPlane(rdCacheProcEntry* pPoly, const rdFace* pFace, rdVector3* aVerts, size_t numVerts);

// Function transforms ceiling sky vertices to NDC screen space
// 
// aVerts here should be transformed to camera space.
// 
// aTransformedVerts should be projected to clip space. (camera->pfProject)
void J3DAPI sithRenderSky_CeilingFaceToPlane(rdCacheProcEntry* pPoly, const rdFace* pFace, const rdVector3* aVerts, const rdVector3* aTransformedVerts, size_t numVerts);

// Helper hooking functions
void sithRenderSky_InstallHooks(void);
void sithRenderSky_ResetGlobals(void);

J3D_EXTERN_C_END
#endif // SITH_SITHRENDERSKY_H

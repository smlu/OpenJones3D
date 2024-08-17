#ifndef RDROID_RDLIGHT_H
#define RDROID_RDLIGHT_H
#include <j3dcore/j3d.h>
#include <rdroid/types.h>
#include <rdroid/RTI/addresses.h>
#include <std/types.h>

J3D_EXTERN_C_START

int J3DAPI rdLight_NewEntry(rdLight* pLight);
void J3DAPI rdLight_CalcVertexIntensities(const rdLight** apLights, const rdVector3* aLightPos, size_t numLights, const rdVector3* aVertexNormal, const rdVector3* aVertices, const rdVector4* aVertexColors, rdVector4* aLightColors, size_t numVertices);
void J3DAPI rdLight_CalcFaceIntensity(const rdLight** apLights, const rdVector3* apLightPos, size_t numLights, const rdFace* pFace, const rdVector3* pNormal, const rdVector3* apVertices, float attenuation, rdVector4* pOutLightColor);
float J3DAPI rdLight_GetIntensity(const rdVector4* pLight);

// Helper hooking functions
void rdLight_InstallHooks(void);
void rdLight_ResetGlobals(void);

J3D_EXTERN_C_END
#endif // RDROID_RDLIGHT_H

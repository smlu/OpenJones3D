#ifndef SITH_SITHINTERSECT_H
#define SITH_SITHINTERSECT_H
#include <j3dcore/j3d.h>
#include <rdroid/types.h>
#include <sith/types.h>
#include <sith/Main/sithMain.h>
#include <std/types.h>

J3D_EXTERN_C_START

int J3DAPI sithIntersect_IsSphereInSector(const SithWorld* pWorld, const rdVector3* pos, float radius, const SithSector* pSector);
SithCollisionType J3DAPI sithIntersect_CheckFaceVerticesIntersection(const rdVector3* startPos, float radius, const rdFace* pFace, const rdVector3* aVertices, rdVector3* pHitVert);
SithCollisionType J3DAPI sithIntersect_CheckSphereThingIntersection(SithThing* pThing, const rdVector3* startPos, const rdVector3* moveNorm, float moveDist, float radius, SithThing* pCheck, int flags, float* pHitDistance, rdModel3Mesh** ppHitMesh, rdFace** ppHitFace, rdVector3* hitNorm);
SithCollisionType J3DAPI sithIntersect_CheckSphereThingModelIntersection(const rdVector3* startPos, const rdVector3* moveNorm, float moveDist, float radius, const rdThing* prdThing, float* hitDistance, rdFace** ppFace, rdModel3Mesh** ppMesh, rdVector3* hitNorm);
SithCollisionType J3DAPI sithIntersect_CheckSphereMeshIntersection(const rdVector3* startPos, const rdVector3* moveNorm, float moveDist, float radius, const rdModel3Mesh* pMesh, float* hitDistance, rdFace** ppFace, rdVector3* hitNorm);
int J3DAPI sithIntersect_CheckSphereIntersection(const rdVector3* startPos, const rdVector3* moveNorm, float moveDist, float radius, const rdVector3* endPos, float size, float* hitDistance, int bCheckEndPos, int flags);
int J3DAPI sithIntersect_TestSphereFaceHit(const rdVector3* startPos, float radius, const rdFace* pFace, const rdVector3* aVertices, int* pHitMask);
SithCollisionType J3DAPI sithIntersect_CheckSphereFaceHitVerticesIntersection(const rdVector3* startPos, float radius, const rdFace* pFace, const rdVector3* aVertices, int vertHitMask, rdVector3* pHitVert);
int J3DAPI sithIntersect_CheckSphereHit(const rdVector3* startPos, const rdVector3* moveNorm, float moveDistance, float radius, const rdVector3* normal, const rdVector3* point, float* pSphereHitDist, int flags);
SithCollisionType J3DAPI sithIntersect_CheckSphereFaceIntersectionEx(const rdVector3* startPos, const rdVector3* moveNorm, float moveDistance, float radius, const rdFace* pFace, const rdVector3* aVertices, float* pHitDist, rdVector3* hitNorm, int flags);
SithCollisionType J3DAPI sithIntersect_CheckSphereFaceIntersection(const rdVector3* startPos, const rdVector3* moveNorm, float moveDistance, float radius, const rdFace* pFace, const rdVector3* aVertices, float* pHitDist, int flags);
int J3DAPI sithIntersect_CheckThingBoundBoxIntersection(const SithThing* pThing1, const SithThing* pThing2, const rdVector3* moveNorm, float moveDist, float* hitDist); // Added

// Helper hooking functions
void sithIntersect_ResetGlobals(void);

J3D_EXTERN_C_END
#endif // SITH_SITHINTERSECT_H

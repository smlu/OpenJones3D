#ifndef RDROID_RDCLIP_H
#define RDROID_RDCLIP_H
#include <j3dcore/j3d.h>
#include <rdroid/types.h>
#include <rdroid/RTI/addresses.h>
#include <std/types.h>

J3D_EXTERN_C_START

#define MAX_CLIP_VERTICIES 80u

#define rdClip_g_faceStatus J3D_DECL_FAR_VAR(rdClip_g_faceStatus, int)
// extern int rdClip_g_faceStatus;

int J3DAPI rdClip_Line2(const rdCanvas* pCanvas, int* x1, int* y1, int* x2, int* y2);
rdClipOutcode J3DAPI rdClip_CalcOutcode2(const rdCanvas* pCanvas, int x, int y);


int J3DAPI rdClip_Line2Ex(float* x1, float* y1, float* x2, float* y2); // overload of rdClip_Line2
int J3DAPI rdClip_CalcOutcode2Ex(float x, float y, float width, float height); // overload of rdClip_CalcOutcode2

int J3DAPI rdClip_ClipFacePVS(rdClipFrustum* pFrustrum, const rdPrimit3* pSrc, rdPrimit3* pDest);

int J3DAPI rdClip_Point3(const rdClipFrustum* pFrustum, const rdVector3* pPoint); // Added

int J3DAPI rdClip_Face3WPVS(rdClipFrustum* pFrustrum, rdVector3* aVertices, size_t numVertices);
int J3DAPI rdClip_Face3W(const rdClipFrustum* pFrustrum, rdVector3* aVertices, size_t numVertices);
int J3DAPI rdClip_Face3WOrtho(const rdClipFrustum* pFrustrum, rdVector3* aVertices, size_t numVertices);
int J3DAPI rdClip_Face3S(const rdClipFrustum* pFrustrum, rdVector3* aVertices, size_t numVertices);
int J3DAPI rdClip_Face3SOrtho(const rdClipFrustum* pFrustum, rdVector3* aVertices, size_t numVertices);
int J3DAPI rdClip_Face3GS(const rdClipFrustum* pFrustrum, rdVector3* aVertices, rdVector4* aIntensities, size_t numVertices);
int J3DAPI rdClip_Face3GSOrtho(const rdClipFrustum* pFrustum, rdVector3* aVertices, rdVector4* aIntensities, size_t numVertices);
int J3DAPI rdClip_Face3GT(const rdClipFrustum* pFrustrum, rdVector3* aVertices, rdVector2* aTexVertices, size_t numVertices);
int J3DAPI rdClip_Face3GTOrtho(const rdClipFrustum* pFrustum, rdVector3* aVertices, rdVector2* aTexVertices, size_t numVertices);
int J3DAPI rdClip_Face3T(const rdClipFrustum* pFrustrum, rdVector3* aVertices, rdVector2* aTexVertices, rdVector4* aIntensities, size_t numVertices);
int J3DAPI rdClip_Face3TOrtho(const rdClipFrustum* pFrustum, rdVector3* aVertices, rdVector2* aTexVertices, rdVector4* aIntensities, size_t numVertices);

// Checks if sphere is in frustum.
// pCenter should be in camera space.
// Returns RdFrustumCull:
//  0 - inside frustum
//  1 - intersects frustum
//  2 - outside frustum 
RdFrustumCull J3DAPI rdClip_SphereInFrustrum(const rdClipFrustum* pFrustum, const rdVector3* pCenter, float radius);

// Function clips vertices to clipfrustum and output them to pDst
// Function does quick clipiping by getting number of vertices in laying in frustum and than  only near plane clipping is performed
int J3DAPI rdClip_QClipFaceT(rdClipFrustum* pFrustrum, const rdPrimit3* pSrc, rdPrimit3* pDest, const rdVector2* pTVOffset); // Added from debug
void J3DAPI rdClip_QClipFaceW(const rdClipFrustum* pFrustrum, const rdPrimit3* pSrc, rdPrimit3* pDst);

// Function clips and transforms face vertices to screen space then assigns them to out pProcFace 
// aVerts here should be transformed to camera space.
int J3DAPI rdClip_FaceToPlane(const rdClipFrustum* pFrustrum, rdCacheProcEntry* pProcFace, const rdFace* pFace, const rdVector3* aVerts, const rdVector2* aTexVerts, const rdVector4* aLightColors, const rdVector4* aVertColors);

int J3DAPI rdClip_FaceToPlaneEx(const rdClipFrustum* pFrustrum, rdCacheProcEntry* pProcFace, const rdFace* pFace, const rdVector3* aVerts, const rdVector3* aVertNormals, const rdVector2* aTexVerts, const rdVector4* aLightColors, const rdVector4* aVertColors); // new

// Function transforms vertices to screen space and assigns them to out pProcFace 
// aVerts here should be transformed to camera space.
void J3DAPI rdClip_VerticesToPlane(rdCacheProcEntry* pProcFace, const rdVector3* aVerts, const rdVector2* aTexVerts, size_t numVerts);

// Helper hooking functions
void rdClip_InstallHooks(void);
void rdClip_ResetGlobals(void);

J3D_EXTERN_C_END
#endif // RDROID_RDCLIP_H

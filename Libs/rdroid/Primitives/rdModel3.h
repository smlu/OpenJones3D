#ifndef RDROID_RDMODEL3_H
#define RDROID_RDMODEL3_H
#include <j3dcore/j3d.h>
#include <rdroid/Raster/rdCache.h>
#include <rdroid/types.h>
#include <rdroid/RTI/addresses.h>

J3D_EXTERN_C_START

#define RDMODEL3_MAX_VERTICES      RDCACHE_MAXVERTICES / 2                              // Changed: Was fixed 1024, now is bound to RDCACHE_MAXVERTICES
#define RDMODEL3_MAX_TEXVRTICES    RDMODEL3_MAX_VERTICES + (RDMODEL3_MAX_VERTICES / 2)  // Changed: Was fixed to 768, now is calculated based on RDMODEL3_MAX_VERTICES

#define RDMODEL3_MAX_FACES         16384 // Changed: Was 1528
#define RDMODEL3_MAX_FACE_VERTICES RDCACHE_MAXFACEVERTICES 

#define rdModel3_g_numDrawnFaces J3D_DECL_FAR_VAR(rdModel3_g_numDrawnFaces, int)
// extern int rdModel3_g_numDrawnFaces;

#define rdModel3_g_numDrawnAlphaFaces J3D_DECL_FAR_VAR(rdModel3_g_numDrawnAlphaFaces, int)
// extern int rdModel3_g_numDrawnAlphaFaces;

rdModel3LoaderFunc J3DAPI rdModel3_RegisterLoader(rdModel3LoaderFunc pfFunc);
rdModel3UnloaderFunc J3DAPI rdModel3_RegisterUnloader(rdModel3UnloaderFunc pfFunc); // Added

void J3DAPI rdModel3_NewEntry(rdModel3* pModel);
rdModel3* J3DAPI rdModel3_Load(const char* pName); // Added
int J3DAPI rdModel3_LoadEntry(const char* pFilename, rdModel3* pModel3);
int J3DAPI rdModel3_Write(const char* pFilename, const rdModel3* pModel, const char* pCratedName); // Added

void J3DAPI rdModel3_Free(rdModel3* pModel3); // Added
void J3DAPI rdModel3_FreeEntry(rdModel3* pModel3);
void J3DAPI rdModel3_FreeEntryGeometryOnly(rdModel3* pModel);

bool J3DAPI rdModel3_Validate(const rdModel3* pModel3); // Added
void J3DAPI rdModel3_CalcRadii(rdModel3* pModel3); // Added
void J3DAPI rdModel3_BuildExpandedRadius(const rdModel3* pModel, const rdModel3HNode* pNode, const rdMatrix34* mat);
void J3DAPI rdModel3_CalcFaceNormals(rdModel3* pModel3); // Added
void J3DAPI rdModel3_CalcVertexNormals(rdModel3* pModel); // Added

rdModel3HNode* J3DAPI rdModel3_FindNamedNode(const char* pName, rdModel3* pModel3); // Added

int J3DAPI rdModel3_GetMeshMatrix(rdThing* pThing, const rdMatrix34* orient, unsigned int nodeNum, rdMatrix34* meshOrient);
int J3DAPI rdModel3_ReplaceMesh(rdModel3* pModel, unsigned int geosetNum, unsigned int meshNum, const rdModel3Mesh* pSrcMesh);
int J3DAPI rdModel3_SwapMesh(rdModel3* pModel1, unsigned int meshNum1, rdModel3* pModel2, int meshNum2);

void J3DAPI rdModel3_GetThingColor(const rdThing* pThing, rdVector4* pDestColor);
void J3DAPI rdModel3_SetModelColor(rdModel3* pModel, const rdVector4* pColor);
void J3DAPI rdModel3_SetThingColor(rdThing* prdThing, const rdVector4* pColor);
void J3DAPI rdModel3_SetNodeColor(const rdModel3* pModel, rdModel3GeoSet* pGeoSet, const rdModel3HNode* pNode, const rdVector4* pColor);
void J3DAPI rdModel3_SetNodeColorForNonAmputatedJoints(const rdThing* prdThing, rdModel3GeoSet* pGeoSet, const rdModel3HNode* pNode, const rdVector4* pColor);

int J3DAPI rdModel3_Draw(rdThing* pThing, const rdMatrix34* pPlacement);
void J3DAPI rdModel3_DrawHNode(const rdModel3GeoSet* prdGeo, const rdModel3HNode* pNode);
void J3DAPI rdModel3_DrawMesh(const rdModel3Mesh* pMesh, const rdMatrix34* pOrient);
void J3DAPI rdModel3_DrawFace(const rdFace* pFace, const rdVector3* aTransformedVertices, int bIsBackFace, const rdVector4* pMeshColor);
void J3DAPI rdModel3_EnableFogRendering(int bEnabled);
rdModel3DrawFaceFunc J3DAPI rdModel3_RegisterFaceDraw(rdModel3DrawFaceFunc pFunc); // Added

// Helper hooking functions
void rdModel3_InstallHooks(void);
void rdModel3_ResetGlobals(void);

J3D_EXTERN_C_END
#endif // RDROID_RDMODEL3_H

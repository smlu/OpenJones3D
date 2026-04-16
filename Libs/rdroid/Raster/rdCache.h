#ifndef RDROID_RDCACHE_H
#define RDROID_RDCACHE_H
#include <j3dcore/j3d.h>
#include <rdroid/types.h>
#include <rdroid/RTI/addresses.h>
#include <std/types.h>
#include <std/Win95/std3D.h>    

J3D_EXTERN_C_START

// Changed: For compatibility reasons was changed to use macros from std3D.h
#define RDCACHE_MAXFACEVERTICES STD3D_MAXFACEVERTICES // Max vertices per face
                                                      // Note, some clipping functions, such as rdClip_FaceToPlane can handle at max 144 vertices. 
                                                      // So be conservative when raising this number

#define RDCACHE_MAXVERTICES STD3D_MAXVERTICES // Changed: Was 2048, and for compatibility reasons was changed to use macros from std3D.h

    void rdCache_Startup(void);
void rdCache_AdvanceFrame(void);
size_t rdCache_GetFrameNum(void);

rdCacheProcEntry* rdCache_GetProcEntry(void);
rdCacheProcEntry* rdCache_GetAlphaProcEntry(void);

void  rdCache_Flush(void);
void  rdCache_FlushAlpha(void);

void J3DAPI rdCache_AddProcFace(size_t numVerts);
void J3DAPI rdCache_AddAlphaProcFace(size_t numVertices);

void J3DAPI rdCache_SendFaceListToHardware(size_t numPolys, rdCacheProcEntry* aPolys, rdCacheSortFunc pfSort);
void J3DAPI rdCache_SendWireframeFaceListToHardware(size_t numPolys, rdCacheProcEntry* aPolys);

void J3DAPI rdCache_AddToTextureCache(tSystemTexture* pTexture, StdColorFormatType format);

// Helper hooking functions
void rdCache_InstallHooks(void);
void rdCache_ResetGlobals(void);

#ifdef J3D_OPENGL
void rdCache_InitFaceDrawInfo(size_t numFaces);
void rdCache_FreeFaceDrawInfos(void);
size_t rdCache_AddFaceInfoEntry(size_t indexOffset, size_t numVertices);
rdPayload* rdCache_GetOpaqueDrawCall(rdDrawType type);
rdPayload* rdCache_GetTransparentDrawCall(rdDrawType type);
void rdCache_AddOpaqueDrawCall(void);
void rdCache_AddTransparentDrawCall(void);
void J3DAPI rdCache_AddLineDrawCall(LPD3DTLVERTEX aVerts, size_t numVerts);
#endif
J3D_EXTERN_C_END
#endif // RDROID_RDCACHE_H

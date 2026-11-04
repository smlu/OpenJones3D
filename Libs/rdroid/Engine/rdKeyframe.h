#ifndef RDROID_RDKEYFRAME_H
#define RDROID_RDKEYFRAME_H
#include <j3dcore/j3d.h>
#include <rdroid/types.h>
#include <std/types.h>

J3D_EXTERN_C_START

rdKeyframeLoadFunc J3DAPI rdKeyframe_RegisterLoader(rdKeyframeLoadFunc pFunc);
rdKeyframeUnloadFunc J3DAPI rdKeyframe_RegisterUnloader(rdKeyframeUnloadFunc pFunc); // Added

void J3DAPI rdKeyframe_NewEntry(rdKeyframe* pKeyframe);
rdKeyframe* J3DAPI rdKeyframe_Load(const char* pFilename); // Added
int J3DAPI rdKeyframe_LoadEntry(const char* pFilename, rdKeyframe* pKeyframe);

void J3DAPI rdKeyframe_Free(rdKeyframe* pKeyframe); // Added
void J3DAPI rdKeyframe_FreeEntry(rdKeyframe* pKeyfreame);

void rdKeyframe_PrintKeyframe(const rdKeyframe* pKeyframe); // Added

// Helper hooking functions
void rdKeyframe_ResetGlobals(void);

J3D_EXTERN_C_END
#endif // RDROID_RDKEYFRAME_H

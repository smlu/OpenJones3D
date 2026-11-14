#ifndef RDROID_RDCACHETESTHOST_H
#define RDROID_RDCACHETESTHOST_H
#include <rdroid/Main/rdroid.h>
#include <rdroid/Tests/rdroidTestHost.h>

extern rdLightMode rdCacheTest_lighting;
extern rdGeometryMode rdCacheTest_geometry;

// Share the harness host for debug-only stdMath assertions as well.
#undef std_g_pHS
#define std_g_pHS rdroidTest_pHS

#undef rdroid_g_curLightingMode_ADDR
#define rdroid_g_curLightingMode_ADDR ((uintptr_t)&rdCacheTest_lighting)
#undef rdroid_g_curGeometryMode_ADDR
#define rdroid_g_curGeometryMode_ADDR ((uintptr_t)&rdCacheTest_geometry)

// The device-facing std3D and fade calls of rdCache.c go to the test fixtures in rdCacheTest.c. Renamed in the cache
// translation units only, so the other suites of the executable keep the real std functions.
#define stdEffect_GetFadeFactor       rdCacheTest_GetFadeFactor
#define std3D_UpdateFrameCount        rdCacheTest_UpdateFrameCount
#define std3D_AddToTextureCache       rdCacheTest_AddToTextureCache
#define std3D_SetWireframeRenderState rdCacheTest_SetWireframeRenderState
#define std3D_DrawPointList           rdCacheTest_DrawPointList
#define std3D_DrawLineStrip           rdCacheTest_DrawLineStrip
#define std3D_DrawRenderList          rdCacheTest_DrawRenderList
#endif

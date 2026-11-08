#ifndef RDROID_RDCACHETESTHOST_H
#define RDROID_RDCACHETESTHOST_H
#include <rdroid/Main/rdroid.h>

extern tHostServices* rdCacheTest_pHS;
extern rdLightMode rdCacheTest_lighting;
extern rdGeometryMode rdCacheTest_geometry;

// Share the harness host for debug-only stdMath assertions as well.
#undef std_g_pHS
#define std_g_pHS rdCacheTest_pHS

#undef rdroid_g_pHS_ADDR
#define rdroid_g_pHS_ADDR ((uintptr_t)&rdCacheTest_pHS)
#undef rdroid_g_curLightingMode_ADDR
#define rdroid_g_curLightingMode_ADDR ((uintptr_t)&rdCacheTest_lighting)
#undef rdroid_g_curGeometryMode_ADDR
#define rdroid_g_curGeometryMode_ADDR ((uintptr_t)&rdCacheTest_geometry)
#endif

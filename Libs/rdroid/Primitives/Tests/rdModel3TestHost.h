#ifndef RDROID_TESTS_RDMODEL3TESTHOST_H
#define RDROID_TESTS_RDMODEL3TESTHOST_H

#include <rdroid/Main/rdroid.h>
#include <rdroid/Tests/rdroidTestHost.h>
#include <rdroid/Engine/rdCamera.h>

extern rdCamera* rdModel3Test_pCamera;
extern rdMatrix34 rdModel3Test_cameraMatrix;
extern rdRoidFlags rdModel3Test_renderOptions;
extern rdLightMode rdModel3Test_lightingMode;
extern int rdModel3Test_numDrawnFaces;
extern int rdModel3Test_numDrawnAlphaFaces;

// Relocate only the test translation units before inline helpers expand RTI
// addresses. Production renderer/math bindings retain their original contract.
#undef rdCamera_g_pCurCamera_ADDR
#define rdCamera_g_pCurCamera_ADDR ((uintptr_t)&rdModel3Test_pCamera)
#undef rdCamera_g_camMatrix_ADDR
#define rdCamera_g_camMatrix_ADDR ((uintptr_t)&rdModel3Test_cameraMatrix)
#undef rdroid_g_curRenderOptions_ADDR
#define rdroid_g_curRenderOptions_ADDR ((uintptr_t)&rdModel3Test_renderOptions)
#undef rdroid_g_curLightingMode_ADDR
#define rdroid_g_curLightingMode_ADDR ((uintptr_t)&rdModel3Test_lightingMode)
#undef rdModel3_g_numDrawnFaces_ADDR
#define rdModel3_g_numDrawnFaces_ADDR ((uintptr_t)&rdModel3Test_numDrawnFaces)
#undef rdModel3_g_numDrawnAlphaFaces_ADDR
#define rdModel3_g_numDrawnAlphaFaces_ADDR ((uintptr_t)&rdModel3Test_numDrawnAlphaFaces)

// Keep the actual model/face submission code while replacing the device-facing
// queue and clipping endpoints with deferred CPU fixtures.
#define rdCache_GetProcEntry rdModel3Test_GetProcEntry
#define rdCache_GetAlphaProcEntry rdModel3Test_GetAlphaProcEntry
#define rdCache_AddProcFace rdModel3Test_AddProcFace
#define rdCache_AddAlphaProcFace rdModel3Test_AddAlphaProcFace
#define rdClip_FaceToPlane rdModel3Test_ClipFaceToPlane

#endif

#ifndef STD3D_SYSTEM_TEST_SUPPORT_H
#define STD3D_SYSTEM_TEST_SUPPORT_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define STD3D_SYSTEM_TEST_CUBE_VECTOR_FRAME_COUNT 9u
#define STD3D_SYSTEM_TEST_FEATURE_CUBE_FRAME_COUNT 32u
#define STD3D_SYSTEM_TEST_HIGH_QUALITY_VECTOR_WIDTH 640u
#define STD3D_SYSTEM_TEST_HIGH_QUALITY_VECTOR_HEIGHT 480u

typedef enum eStd3DSystemTestCubeVectorMode
{
    STD3D_SYSTEM_TEST_CUBE_VECTOR_VERTICES,
    STD3D_SYSTEM_TEST_CUBE_VECTOR_WIREFRAME,
    STD3D_SYSTEM_TEST_CUBE_VECTOR_SOLID,
    STD3D_SYSTEM_TEST_CUBE_VECTOR_TEXTURED,
    STD3D_SYSTEM_TEST_CUBE_VECTOR_TEXTURED_VERTEX_COLOR,
    STD3D_SYSTEM_TEST_CUBE_VECTOR_SOLID_VERTEX_INTENSITY,
} Std3DSystemTestCubeVectorMode;

bool Std3DSystemTest_CurrentDeviceSupportsFog(void);
bool Std3DSystemTest_CurrentDeviceSupportsMipmaps(void);
const char* Std3DSystemTest_GetCubeVectorModeName(Std3DSystemTestCubeVectorMode mode);
void Std3DSystemTest_RenderPrimitiveHarnessScene(void);
void Std3DSystemTest_RenderVertexGeometryCubeScene(void);
void Std3DSystemTest_RenderWireframeGeometryCubeScene(void);
void Std3DSystemTest_RenderSolidGeometryCubeScene(void);
void Std3DSystemTest_RenderTexturedGeometryCubeScene(void);
void Std3DSystemTest_RenderCubeVectorFrame(Std3DSystemTestCubeVectorMode mode, size_t frameNum);
void Std3DSystemTest_RenderFogGeometryScene(void);
void Std3DSystemTest_RenderSolidTextureQuadScene(uint8_t red, uint8_t green, uint8_t blue);
void Std3DSystemTest_RenderMultiMipmapTextureQuadScene(uint8_t red, uint8_t green, uint8_t blue);
void Std3DSystemTest_RenderMipmapLodQuadScene(void);
void Std3DSystemTest_RenderMipmapLodCubeFrame(size_t frameNum);
void Std3DSystemTest_RenderHighQualityMipmapLodCubeFrame(size_t frameNum);
void Std3DSystemTest_RenderMipmapAutoGenCubeFrame(size_t frameNum);
void Std3DSystemTest_RenderMipmapAutoGenAnisotropicCubeFrame(size_t frameNum);
void Std3DSystemTest_RenderAnisotropicTextureFilterScene(void);
void Std3DSystemTest_RenderAnisotropicTextureFilterCubeScene(void);
void Std3DSystemTest_RenderMipmapAnisotropicCubeFrame(size_t frameNum);
void Std3DSystemTest_RenderMSAATriangleScene(void);
void Std3DSystemTest_RenderMSAACubeVectorFrame(Std3DSystemTestCubeVectorMode mode, size_t frameNum);
void Std3DSystemTest_RenderMSAATexturedCubeFrame(size_t frameNum);
void Std3DSystemTest_RenderMSAAWireframeCubeFrame(size_t frameNum);
void Std3DSystemTest_RenderFormattedTextureQuadScene(int formatType, uint8_t red, uint8_t green, uint8_t blue, uint8_t alpha);
void Std3DSystemTest_RenderTexturedDepthOcclusionScene(void);
void Std3DSystemTest_RenderZBufferOcclusionScene(void);
void Std3DSystemTest_ExerciseTextureCacheResetAndReuse(void);
void Std3DSystemTest_ExerciseTextureCacheEviction(void);
void Std3DSystemTest_ExerciseAlphaAndTransparency(void);
void Std3DSystemTest_ExerciseTextureAddressFilteringAndStateIsolation(void);
void Std3DSystemTest_ExerciseRectangularTextureUpload(void);
void Std3DSystemTest_ExerciseDepthWindingAndProjectionBoundaries(void);
void Std3DSystemTest_ExerciseRepeated3DLifecycle(void);
#if defined(J3D_DIRECTX9)
void Std3DSystemTest_ExerciseDeviceResetRecovery(void);
void Std3DSystemTest_ExerciseDynamicBufferWraparound(void);
#endif
void Std3DSystemTest_AssertValidDimensionsFollowDeviceCaps(void);
void Std3DSystemTest_ExerciseRenderStatePermutations(void);
void Std3DSystemTest_AssertBackBufferMatchesBmpMask(const char* pVectorName, uint8_t tolerance);
void Std3DSystemTest_AssertBackBufferMatchesBmpMaskAtResolution(const char* pVectorName, uint8_t tolerance, uint32_t width, uint32_t height);
void Std3DSystemTest_AssertBackBufferMatchesBmpProbeMask(const char* pVectorName, uint8_t tolerance, uint32_t searchRadius);
void Std3DSystemTest_AssertBackBufferPixelNear(uint32_t x, uint32_t y, uint8_t red, uint8_t green, uint8_t blue, uint8_t tolerance);
void Std3DSystemTest_AssertBackBufferPixelInRange(uint32_t x, uint32_t y, uint8_t minRed, uint8_t maxRed, uint8_t minGreen, uint8_t maxGreen, uint8_t minBlue, uint8_t maxBlue);
void Std3DSystemTest_AssertBackBufferAnyPixelNear(uint32_t left, uint32_t top, uint32_t width, uint32_t height, uint8_t red, uint8_t green, uint8_t blue, uint8_t tolerance);
void Std3DSystemTest_AssertBackBufferAnyPixelInRange(uint32_t left, uint32_t top, uint32_t width, uint32_t height, uint8_t minRed, uint8_t maxRed, uint8_t minGreen, uint8_t maxGreen, uint8_t minBlue, uint8_t maxBlue);
void Std3DSystemTest_AssertBackBufferRectNear(uint32_t left, uint32_t top, uint32_t width, uint32_t height, uint8_t red, uint8_t green, uint8_t blue, uint8_t tolerance);
void Std3DSystemTest_ReadBackBufferPixel(uint32_t x, uint32_t y, uint8_t* pRed, uint8_t* pGreen, uint8_t* pBlue);

#endif // STD3D_SYSTEM_TEST_SUPPORT_H

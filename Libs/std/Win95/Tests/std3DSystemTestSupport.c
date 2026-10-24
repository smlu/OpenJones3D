#include "std3DSystemTestSupport.h"

#include <unity_fixture.h>

#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include <std/General/std.h>
#include <std/General/stdColor.h>
#include <std/General/stdUtil.h>
#include <std/Win95/std3D.h>
#include <std/Win95/stdDisplay.h>

#include "stdWin95SystemTestSupport.h"

#define STD3DSYSTEMTEST_VECTOR_WIDTH  320u
#define STD3DSYSTEMTEST_VECTOR_HEIGHT 240u

// Ignore magenta pixels in the mask for comparison
#define STD3DSYSTEMTEST_MASK_IGNORE_R 255u
#define STD3DSYSTEMTEST_MASK_IGNORE_G 0u
#define STD3DSYSTEMTEST_MASK_IGNORE_B 255u

#define STD3DSYSTEMTEST_CUBE_FRAME_DEGREES 10.0f
#define STD3DSYSTEMTEST_CUBE_PITCH_DEGREES 22.0f
#define STD3DSYSTEMTEST_CUBE_CAMERA_DISTANCE 4.0f
#define STD3DSYSTEMTEST_CUBE_PROJECT_SCALE 170.0f
#define STD3DSYSTEMTEST_CUBE_FACE_VISIBILITY_EPSILON 0.20f
#define STD3DSYSTEMTEST_DEGREES_TO_RADIANS 0.017453292519943295f
#define STD3DSYSTEMTEST_FEATURE_CUBE_PHASE_FRAME_COUNT 8u
#define STD3DSYSTEMTEST_FEATURE_CUBE_NEAR_DISTANCE 4.2f
#define STD3DSYSTEMTEST_FEATURE_CUBE_FAR_DISTANCE 7.0f
#define STD3DSYSTEMTEST_FEATURE_CUBE_PROJECT_SCALE 150.0f

#if defined(J3D_DIRECTX6)
#define STD3DSYSTEMTEST_BACKEND_VECTOR_DIR "dx6"
#define STD3DSYSTEMTEST_MIPMAP_TOP_LEVEL_SIZE 256u
#define STD3DSYSTEMTEST_MIPMAP_LEVEL_COUNT 9u // Complete power-of-two chain includes 1x1.
#elif defined(J3D_DIRECTX9)
#define STD3DSYSTEMTEST_BACKEND_VECTOR_DIR "dx9"
#define STD3DSYSTEMTEST_MIPMAP_TOP_LEVEL_SIZE 512u
#define STD3DSYSTEMTEST_MIPMAP_LEVEL_COUNT 10u // Complete power-of-two chain includes 1x1.
#else
#error Unsupported std3D system-test backend.
#endif

typedef struct sStd3DSystemTestModelVertex
{
    float x;
    float y;
    float z;
} Std3DSystemTestModelVertex;

typedef struct sStd3DSystemTestBmp
{
    uint32_t width;
    uint32_t height;
    uint8_t* pPixels;
} Std3DSystemTestBmp;

typedef struct sStd3DSystemTestScreenVertex
{
    float x;
    float y;
    float z;
    float rhw;
} Std3DSystemTestScreenVertex;

typedef struct sStd3DSystemTestCubeFace
{
    uint8_t aVertices[4];
} Std3DSystemTestCubeFace;

static const Std3DSystemTestScreenVertex std3DSystemTest_aCubeVerts[8] =
{
    { 100.0f,  78.0f, 0.40f, 1.0f },
    { 220.0f,  98.0f, 0.40f, 1.0f },
    { 210.0f, 180.0f, 0.40f, 1.0f },
    {  92.0f, 158.0f, 0.40f, 1.0f },
    {  62.0f,  48.0f, 0.70f, 1.0f },
    { 182.0f,  68.0f, 0.70f, 1.0f },
    { 172.0f, 148.0f, 0.70f, 1.0f },
    {  54.0f, 128.0f, 0.70f, 1.0f },
};

static const Std3DSystemTestModelVertex std3DSystemTest_aCubeModelVerts[8] =
{
    { -1.0f,  1.0f,  1.0f },
    {  1.0f,  1.0f,  1.0f },
    {  1.0f, -1.0f,  1.0f },
    { -1.0f, -1.0f,  1.0f },
    { -1.0f,  1.0f, -1.0f },
    {  1.0f,  1.0f, -1.0f },
    {  1.0f, -1.0f, -1.0f },
    { -1.0f, -1.0f, -1.0f },
};

static const Std3DSystemTestCubeFace std3DSystemTest_aCubeFaces[] =
{
    { { 0u, 1u, 2u, 3u } },
    { { 4u, 0u, 3u, 7u } },
    { { 1u, 5u, 6u, 2u } },
    { { 4u, 5u, 1u, 0u } },
    { { 3u, 2u, 6u, 7u } },
    { { 5u, 4u, 7u, 6u } },
};

static const Std3DSystemTestModelVertex std3DSystemTest_aCubeFaceNormals[] =
{
    {  0.0f,  0.0f,  1.0f },
    { -1.0f,  0.0f,  0.0f },
    {  1.0f,  0.0f,  0.0f },
    {  0.0f,  1.0f,  0.0f },
    {  0.0f, -1.0f,  0.0f },
    {  0.0f,  0.0f, -1.0f },
};

static const uint32_t std3DSystemTest_aCubeVertexColors[] =
{
    0xFFFF0000u,
    0xFF00FF00u,
    0xFF0000FFu,
    0xFFFFFFFFu,
    0xFFFFFF00u,
    0xFF00FFFFu,
    0xFFFF8000u,
    0xFF8000FFu,
};

static const float std3DSystemTest_aCubeTexCoords[4][2] =
{
    { 0.0f, 0.0f },
    { 1.0f, 0.0f },
    { 1.0f, 1.0f },
    { 0.0f, 1.0f },
};

static const char* const std3DSystemTest_aCubeVectorModeNames[] =
{
    "vertices",
    "wireframe",
    "solid",
    "textured",
    "textured_vertex_color",
    "solid_vertex_intensity",
};

static uint16_t Std3DSystemTest_ReadU16LE(FILE* pFile)
{
    uint8_t aBytes[2];

    TEST_ASSERT_EQUAL_size_t(sizeof(aBytes), fread(aBytes, 1u, sizeof(aBytes), pFile));
    return (uint16_t)(aBytes[0] | (aBytes[1] << 8));
}

static uint32_t Std3DSystemTest_ReadU32LE(FILE* pFile)
{
    uint8_t aBytes[4];

    TEST_ASSERT_EQUAL_size_t(sizeof(aBytes), fread(aBytes, 1u, sizeof(aBytes), pFile));
    return (uint32_t)(aBytes[0] | (aBytes[1] << 8) | (aBytes[2] << 16) | (aBytes[3] << 24));
}

static int32_t Std3DSystemTest_ReadS32LE(FILE* pFile)
{
    return (int32_t)Std3DSystemTest_ReadU32LE(pFile);
}

static void Std3DSystemTest_FreeBmp(Std3DSystemTestBmp* pBmp)
{
    if ( pBmp->pPixels )
    {
        free(pBmp->pPixels);
        pBmp->pPixels = NULL;
    }
}

static void Std3DSystemTest_LoadBmp(const char* pVectorName, Std3DSystemTestBmp* pBmp, uint32_t expectedWidth, uint32_t expectedHeight)
{
    char aPath[512];
    FILE* pFile = NULL;

    STD_ZEROMEM(pBmp, sizeof(*pBmp));
    STD_FORMAT(aPath, "%s/std3D/%s/%s", STD_WIN95_TEST_TV_DIR, STD3DSYSTEMTEST_BACKEND_VECTOR_DIR, pVectorName);

    TEST_ASSERT_EQUAL_INT(0, fopen_s(&pFile, aPath, "rb"));
    TEST_ASSERT_NOT_NULL_MESSAGE(pFile, aPath);

    uint16_t type = Std3DSystemTest_ReadU16LE(pFile);
    (void)Std3DSystemTest_ReadU32LE(pFile);
    (void)Std3DSystemTest_ReadU16LE(pFile);
    (void)Std3DSystemTest_ReadU16LE(pFile);
    uint32_t offBits = Std3DSystemTest_ReadU32LE(pFile);

    uint32_t dibHeaderSize = Std3DSystemTest_ReadU32LE(pFile);
    int32_t width          = Std3DSystemTest_ReadS32LE(pFile);
    int32_t height         = Std3DSystemTest_ReadS32LE(pFile);
    uint16_t planes        = Std3DSystemTest_ReadU16LE(pFile);
    uint16_t bitCount      = Std3DSystemTest_ReadU16LE(pFile);
    uint32_t compression   = Std3DSystemTest_ReadU32LE(pFile);

    TEST_ASSERT_EQUAL_HEX16(0x4D42u, type);
    TEST_ASSERT_EQUAL_UINT32(40u, dibHeaderSize);
    if ( expectedWidth )
    {
        TEST_ASSERT_EQUAL_INT32((int32_t)expectedWidth, width);
    }

    if ( expectedHeight )
    {
        TEST_ASSERT_EQUAL_INT32((int32_t)expectedHeight, height);
    }

    TEST_ASSERT_TRUE(width > 0);
    TEST_ASSERT_TRUE(height > 0);
    TEST_ASSERT_EQUAL_UINT16(1u, planes);
    TEST_ASSERT_EQUAL_UINT16(24u, bitCount);
    TEST_ASSERT_EQUAL_UINT32(0u, compression);
    TEST_ASSERT_EQUAL_INT(0, fseek(pFile, (long)offBits, SEEK_SET));

    pBmp->width   = (uint32_t)width;
    pBmp->height  = (uint32_t)height;
    pBmp->pPixels = (uint8_t*)malloc((size_t)pBmp->width * pBmp->height * 3u);
    TEST_ASSERT_NOT_NULL(pBmp->pPixels);

    size_t rowSize = ((size_t)pBmp->width * 3u + 3u) & ~3u;
    uint8_t* pRow = (uint8_t*)malloc(rowSize);
    TEST_ASSERT_NOT_NULL(pRow);

    for ( uint32_t fileRow = 0u; fileRow < pBmp->height; ++fileRow )
    {
        uint32_t y = pBmp->height - 1u - fileRow;

        TEST_ASSERT_EQUAL_size_t(rowSize, fread(pRow, 1u, rowSize, pFile));
        for ( uint32_t x = 0u; x < pBmp->width; ++x )
        {
            const uint8_t* pSrc = &pRow[x * 3u];
            uint8_t* pDst = &pBmp->pPixels[((size_t)y * pBmp->width + x) * 3u];

            pDst[0] = pSrc[2];
            pDst[1] = pSrc[1];
            pDst[2] = pSrc[0];
        }
    }

    free(pRow);
    fclose(pFile);
}

static void Std3DSystemTest_LoadBmpMask(const char* pVectorName, Std3DSystemTestBmp* pBmp, uint32_t width, uint32_t height)
{
    Std3DSystemTest_LoadBmp(pVectorName, pBmp, width, height);
}

static void Std3DSystemTest_UpdateBmpMask(const char* pVectorName)
{
    const char* pUpdateVectors = getenv("JONES3D_SYSTEM_TEST_UPDATE_VECTORS");
    if ( !pUpdateVectors || !pUpdateVectors[0] || pUpdateVectors[0] == '0' )
    {
        return;
    }

    char aPath[512];
    STD_FORMAT(aPath, "%s/std3D/%s/%s", STD_WIN95_TEST_TV_DIR, STD3DSYSTEMTEST_BACKEND_VECTOR_DIR, pVectorName);
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, stdDisplay_SaveScreen(aPath), aPath);
}

static void Std3DSystemTest_InitVertex(D3DTLVERTEX* pVertex, float x, float y, float z, uint32_t color)
{
    STD_ZEROMEM(pVertex, sizeof(*pVertex));
    pVertex->sx       = x;
    pVertex->sy       = y;
    pVertex->sz       = z;
    pVertex->rhw      = 1.0f;
    pVertex->color    = color;
    pVertex->specular = 0xFF000000u;
}

static void Std3DSystemTest_InitTexturedVertex(D3DTLVERTEX* pVertex, float x, float y, float z, float tu, float tv, uint32_t color)
{
    Std3DSystemTest_InitVertex(pVertex, x, y, z, color);
    pVertex->tu = tu;
    pVertex->tv = tv;
}

static void Std3DSystemTest_InitTexturedVertexRhw(D3DTLVERTEX* pVertex, float x, float y, float z, float rhw, float tu, float tv, uint32_t color)
{
    Std3DSystemTest_InitTexturedVertex(pVertex, x, y, z, tu, tv, color);
    pVertex->rhw = rhw;
}

static void Std3DSystemTest_InitCubeVertex(D3DTLVERTEX* pVertex, size_t vertexNum, uint32_t color)
{
    const Std3DSystemTestScreenVertex* pSrc = &std3DSystemTest_aCubeVerts[vertexNum];

    Std3DSystemTest_InitVertex(pVertex, pSrc->x, pSrc->y, pSrc->z, color);
    pVertex->rhw = pSrc->rhw;
}

static void Std3DSystemTest_InitCubeTexturedVertex(D3DTLVERTEX* pVertex, size_t vertexNum, float tu, float tv, uint32_t color)
{
    const Std3DSystemTestScreenVertex* pSrc = &std3DSystemTest_aCubeVerts[vertexNum];

    Std3DSystemTest_InitTexturedVertex(pVertex, pSrc->x, pSrc->y, pSrc->z, tu, tv, color);
    pVertex->rhw = pSrc->rhw;
}

static void Std3DSystemTest_EndScene(void)
{
    std3D_EndScene();
    StdWin95SystemTest_PresentVisualFrame();
}

static float Std3DSystemTest_GetFeatureCubeCameraDistance(size_t frameNum)
{
    size_t phase = frameNum / STD3DSYSTEMTEST_FEATURE_CUBE_PHASE_FRAME_COUNT;
    size_t local = frameNum % STD3DSYSTEMTEST_FEATURE_CUBE_PHASE_FRAME_COUNT;
    float t      = (float)local / (float)(STD3DSYSTEMTEST_FEATURE_CUBE_PHASE_FRAME_COUNT - 1u);

    switch ( phase )
    {
        case 0u:
            return STD3DSYSTEMTEST_FEATURE_CUBE_NEAR_DISTANCE;

        case 1u:
            return STD3DSYSTEMTEST_FEATURE_CUBE_NEAR_DISTANCE
                + (STD3DSYSTEMTEST_FEATURE_CUBE_FAR_DISTANCE - STD3DSYSTEMTEST_FEATURE_CUBE_NEAR_DISTANCE) * t;

        case 2u:
            return STD3DSYSTEMTEST_FEATURE_CUBE_FAR_DISTANCE;

        default:
            return STD3DSYSTEMTEST_FEATURE_CUBE_FAR_DISTANCE
                - (STD3DSYSTEMTEST_FEATURE_CUBE_FAR_DISTANCE - STD3DSYSTEMTEST_FEATURE_CUBE_NEAR_DISTANCE) * t;
    }
}

static void Std3DSystemTest_ProjectCubeFrame(size_t frameNum, float frameDegrees, float cameraDistance, float projectScale, uint32_t vectorWidth, uint32_t vectorHeight, bool bPerspectiveCorrect, Std3DSystemTestScreenVertex aVerts[8])
{
    float yaw          = (float)frameNum * frameDegrees * STD3DSYSTEMTEST_DEGREES_TO_RADIANS;
    float pitch        = STD3DSYSTEMTEST_CUBE_PITCH_DEGREES * STD3DSYSTEMTEST_DEGREES_TO_RADIANS;
    float cosYaw       = cosf(yaw);
    float sinYaw       = sinf(yaw);
    float cosPitch     = cosf(pitch);
    float sinPitch     = sinf(pitch);

    for ( size_t i = 0u; i < STD_ARRAYLEN(std3DSystemTest_aCubeModelVerts); ++i )
    {
        const Std3DSystemTestModelVertex* pSrc = &std3DSystemTest_aCubeModelVerts[i];
        float yawX      = pSrc->x * cosYaw + pSrc->z * sinYaw;
        float yawZ      = -pSrc->x * sinYaw + pSrc->z * cosYaw;
        float pitchY    = pSrc->y * cosPitch - yawZ * sinPitch;
        float pitchZ    = pSrc->y * sinPitch + yawZ * cosPitch;
        float denom     = cameraDistance - pitchZ;

        aVerts[i].x     = (float)vectorWidth * 0.5f + yawX * projectScale / denom;
        aVerts[i].y     = (float)vectorHeight * 0.5f - pitchY * projectScale / denom;
        aVerts[i].z     = 0.55f - pitchZ * 0.18f;
        aVerts[i].rhw   = bPerspectiveCorrect ? 1.0f / denom : 1.0f;
    }
}

static void Std3DSystemTest_ProjectCubeVectorFrame(size_t frameNum, Std3DSystemTestScreenVertex aVerts[8])
{
    Std3DSystemTest_ProjectCubeFrame(
        frameNum,
        STD3DSYSTEMTEST_CUBE_FRAME_DEGREES,
        STD3DSYSTEMTEST_CUBE_CAMERA_DISTANCE,
        STD3DSYSTEMTEST_CUBE_PROJECT_SCALE,
        STD3DSYSTEMTEST_VECTOR_WIDTH,
        STD3DSYSTEMTEST_VECTOR_HEIGHT,
        false,
        aVerts
    );
}

static void Std3DSystemTest_ProjectFeatureCubeFrame(size_t frameNum, Std3DSystemTestScreenVertex aVerts[8])
{
    Std3DSystemTest_ProjectCubeFrame(
        frameNum,
        STD3DSYSTEMTEST_CUBE_FRAME_DEGREES,
        Std3DSystemTest_GetFeatureCubeCameraDistance(frameNum),
        STD3DSYSTEMTEST_FEATURE_CUBE_PROJECT_SCALE,
        STD3DSYSTEMTEST_VECTOR_WIDTH,
        STD3DSYSTEMTEST_VECTOR_HEIGHT,
        false,
        aVerts
    );
}

static void Std3DSystemTest_ProjectMipmapCubeFrame(size_t frameNum, Std3DSystemTestScreenVertex aVerts[8])
{
    Std3DSystemTest_ProjectCubeFrame(
        frameNum,
        STD3DSYSTEMTEST_CUBE_FRAME_DEGREES,
        Std3DSystemTest_GetFeatureCubeCameraDistance(frameNum),
        STD3DSYSTEMTEST_FEATURE_CUBE_PROJECT_SCALE,
        STD3DSYSTEMTEST_VECTOR_WIDTH,
        STD3DSYSTEMTEST_VECTOR_HEIGHT,
        true,
        aVerts
    );
}

static void Std3DSystemTest_ProjectHighQualityFeatureCubeFrame(size_t frameNum, Std3DSystemTestScreenVertex aVerts[8])
{
    Std3DSystemTest_ProjectCubeFrame(
        frameNum,
        STD3DSYSTEMTEST_CUBE_FRAME_DEGREES,
        Std3DSystemTest_GetFeatureCubeCameraDistance(frameNum),
        STD3DSYSTEMTEST_FEATURE_CUBE_PROJECT_SCALE * 2.0f,
        STD3D_SYSTEM_TEST_HIGH_QUALITY_VECTOR_WIDTH,
        STD3D_SYSTEM_TEST_HIGH_QUALITY_VECTOR_HEIGHT,
        true,
        aVerts
    );
}

static void Std3DSystemTest_CalculateCubeFaceVisibility(size_t frameNum, float frameDegrees, bool abVisibleFaces[STD_ARRAYLEN(std3DSystemTest_aCubeFaces)])
{
    float yaw      = (float)frameNum * frameDegrees * STD3DSYSTEMTEST_DEGREES_TO_RADIANS;
    float pitch    = STD3DSYSTEMTEST_CUBE_PITCH_DEGREES * STD3DSYSTEMTEST_DEGREES_TO_RADIANS;
    float cosYaw   = cosf(yaw);
    float sinYaw   = sinf(yaw);
    float cosPitch = cosf(pitch);
    float sinPitch = sinf(pitch);

    for ( size_t faceNum = 0u; faceNum < STD_ARRAYLEN(std3DSystemTest_aCubeFaceNormals); ++faceNum )
    {
        const Std3DSystemTestModelVertex* pNormal = &std3DSystemTest_aCubeFaceNormals[faceNum];
        float yawZ      = -pNormal->x * sinYaw + pNormal->z * cosYaw;
        float pitchZ    = pNormal->y * sinPitch + yawZ * cosPitch;

        abVisibleFaces[faceNum] = pitchZ > STD3DSYSTEMTEST_CUBE_FACE_VISIBILITY_EPSILON;
    }
}

static void Std3DSystemTest_CalculateCubeVectorFaceVisibility(size_t frameNum, bool abVisibleFaces[STD_ARRAYLEN(std3DSystemTest_aCubeFaces)])
{
    Std3DSystemTest_CalculateCubeFaceVisibility(frameNum, STD3DSYSTEMTEST_CUBE_FRAME_DEGREES, abVisibleFaces);
}

static void Std3DSystemTest_InitProjectedCubeVertex(D3DTLVERTEX* pVertex, const Std3DSystemTestScreenVertex aCubeVerts[8], size_t vertexNum, uint32_t color)
{
    const Std3DSystemTestScreenVertex* pSrc = &aCubeVerts[vertexNum];

    Std3DSystemTest_InitVertex(pVertex, pSrc->x, pSrc->y, pSrc->z, color);
    pVertex->rhw = pSrc->rhw;
}

static void Std3DSystemTest_InitProjectedCubeTexturedVertex(D3DTLVERTEX* pVertex, const Std3DSystemTestScreenVertex aCubeVerts[8], size_t vertexNum, float tu, float tv, uint32_t color)
{
    const Std3DSystemTestScreenVertex* pSrc = &aCubeVerts[vertexNum];

    Std3DSystemTest_InitTexturedVertex(pVertex, pSrc->x, pSrc->y, pSrc->z, tu, tv, color);
    pVertex->rhw = pSrc->rhw;
}

static uint32_t Std3DSystemTest_ClampToDeviceLimit(uint32_t value, size_t minValue, size_t maxValue)
{
    if ( value < minValue )
    {
        return (uint32_t)minValue;
    }

    if ( value > maxValue )
    {
        return (uint32_t)maxValue;
    }

    return value;
}

static void Std3DSystemTest_WriteVBufferPixel(tVBuffer* pVBuffer, uint32_t x, uint32_t y, uint32_t pixel)
{
    size_t bytesPerPixel = pVBuffer->rasterInfo.colorInfo.bpp / 8u;
    uint8_t* pDst = &pVBuffer->pPixels[(size_t)y * pVBuffer->rasterInfo.rowSize + (size_t)x * bytesPerPixel];

    TEST_ASSERT_TRUE(bytesPerPixel > 0u && bytesPerPixel <= sizeof(pixel));
    for ( size_t i = 0u; i < bytesPerPixel; ++i )
    {
        pDst[i] = (uint8_t)(pixel >> (i * 8u));
    }
}

static tVBuffer* Std3DSystemTest_CreateSolidTextureVBufferWithFormat(uint32_t width, uint32_t height, StdColorFormatType formatType, uint8_t red, uint8_t green, uint8_t blue, uint8_t alpha)
{
    ColorInfo colorInfo;
    int bColorKeySet = 0;
    LPDDCOLORKEY pColorKey = NULL;
    tRasterInfo rasterInfo;

    STD_ZEROMEM(&colorInfo, sizeof(colorInfo));
    std3D_GetTextureFormat(formatType, &colorInfo, &bColorKeySet, &pColorKey);
    TEST_ASSERT_TRUE(colorInfo.bpp >= 16u);
    if ( formatType == STDCOLOR_FORMAT_RGB )
    {
        TEST_ASSERT_FALSE(bColorKeySet);
        TEST_ASSERT_NULL(pColorKey);
    }

    STD_ZEROMEM(&rasterInfo, sizeof(rasterInfo));
    rasterInfo.width     = width;
    rasterInfo.height    = height;
    rasterInfo.colorInfo = colorInfo;

    tVBuffer* pVBuffer = stdDisplay_VBufferNew(&rasterInfo, 0, 0);
    TEST_ASSERT_NOT_NULL(pVBuffer);
    TEST_ASSERT_TRUE(stdDisplay_VBufferLock(pVBuffer));

    uint32_t pixel = formatType == STDCOLOR_FORMAT_RGB
        ? stdColor_EncodeRGB(&colorInfo, red, green, blue)
        : stdColor_EncodeRGBA(&colorInfo, red, green, blue, alpha);
    for ( uint32_t y = 0u; y < height; ++y )
    {
        for ( uint32_t x = 0u; x < width; ++x )
        {
            Std3DSystemTest_WriteVBufferPixel(pVBuffer, x, y, pixel);
        }
    }

    TEST_ASSERT_EQUAL_INT(1, stdDisplay_VBufferUnlock(pVBuffer));
    return pVBuffer;
}

static tVBuffer* Std3DSystemTest_CreateSolidTextureVBuffer(uint32_t width, uint32_t height, uint8_t red, uint8_t green, uint8_t blue)
{
    return Std3DSystemTest_CreateSolidTextureVBufferWithFormat(width, height, STDCOLOR_FORMAT_RGB, red, green, blue, 255u);
}

static tVBuffer* Std3DSystemTest_CreateCubePatternTextureVBuffer(void)
{
    ColorInfo colorInfo;
    int bColorKeySet = 0;
    LPDDCOLORKEY pColorKey = NULL;
    tRasterInfo rasterInfo;

    STD_ZEROMEM(&colorInfo, sizeof(colorInfo));
    std3D_GetTextureFormat(STDCOLOR_FORMAT_RGB, &colorInfo, &bColorKeySet, &pColorKey);
    TEST_ASSERT_TRUE(colorInfo.bpp >= 16u);

    STD_ZEROMEM(&rasterInfo, sizeof(rasterInfo));
    rasterInfo.width     = 8u;
    rasterInfo.height    = 8u;
    rasterInfo.colorInfo = colorInfo;

    tVBuffer* pVBuffer = stdDisplay_VBufferNew(&rasterInfo, 0, 0);
    TEST_ASSERT_NOT_NULL(pVBuffer);
    TEST_ASSERT_TRUE(stdDisplay_VBufferLock(pVBuffer));

    for ( uint32_t y = 0u; y < rasterInfo.height; ++y )
    {
        for ( uint32_t x = 0u; x < rasterInfo.width; ++x )
        {
            uint8_t red   = x < 4u ? 255u : 0u;
            uint8_t green = x < 4u ? 0u : 255u;
            uint8_t blue  = 0u;

            if ( y >= 4u )
            {
                red   = x < 4u ? 0u : 255u;
                green = x < 4u ? 0u : 255u;
                blue  = x < 4u ? 255u : 0u;
            }

            Std3DSystemTest_WriteVBufferPixel(pVBuffer, x, y, stdColor_EncodeRGB(&colorInfo, red, green, blue));
        }
    }

    TEST_ASSERT_EQUAL_INT(1, stdDisplay_VBufferUnlock(pVBuffer));
    return pVBuffer;
}

static tVBuffer* Std3DSystemTest_CreateTextureVBufferFromPixels(const Std3DSystemTestBmp* pBmp)
{
    ColorInfo colorInfo;
    int bColorKeySet = 0;
    LPDDCOLORKEY pColorKey = NULL;
    tRasterInfo rasterInfo;

    STD_ZEROMEM(&colorInfo, sizeof(colorInfo));
    std3D_GetTextureFormat(STDCOLOR_FORMAT_RGB, &colorInfo, &bColorKeySet, &pColorKey);
    TEST_ASSERT_TRUE(colorInfo.bpp >= 16u);

    STD_ZEROMEM(&rasterInfo, sizeof(rasterInfo));
    rasterInfo.width     = pBmp->width;
    rasterInfo.height    = pBmp->height;
    rasterInfo.colorInfo = colorInfo;

    tVBuffer* pVBuffer = stdDisplay_VBufferNew(&rasterInfo, 0, 0);
    TEST_ASSERT_NOT_NULL(pVBuffer);
    TEST_ASSERT_TRUE(stdDisplay_VBufferLock(pVBuffer));

    for ( uint32_t y = 0u; y < pBmp->height; ++y )
    {
        for ( uint32_t x = 0u; x < pBmp->width; ++x )
        {
            const uint8_t* pSrc = &pBmp->pPixels[((size_t)y * pBmp->width + x) * 3u];
            uint32_t pixel = stdColor_EncodeRGB(&colorInfo, pSrc[0], pSrc[1], pSrc[2]);

            Std3DSystemTest_WriteVBufferPixel(pVBuffer, x, y, pixel);
        }
    }

    TEST_ASSERT_EQUAL_INT(1, stdDisplay_VBufferUnlock(pVBuffer));
    return pVBuffer;
}

static tVBuffer* Std3DSystemTest_CreateTextureVBufferFromBmp(const char* pVectorName)
{
    Std3DSystemTestBmp bmp;
    Std3DSystemTest_LoadBmp(pVectorName, &bmp, 0u, 0u);

    tVBuffer* pVBuffer = Std3DSystemTest_CreateTextureVBufferFromPixels(&bmp);

    Std3DSystemTest_FreeBmp(&bmp);
    return pVBuffer;
}

static void Std3DSystemTest_DownsampleBmp2x(const Std3DSystemTestBmp* pSrc, Std3DSystemTestBmp* pDst)
{
    TEST_ASSERT_NOT_NULL(pSrc);
    TEST_ASSERT_NOT_NULL(pSrc->pPixels);
    TEST_ASSERT_NOT_NULL(pDst);
    TEST_ASSERT_TRUE(pSrc->width > 1u || pSrc->height > 1u);

    pDst->width   = J3DMAX(pSrc->width / 2u, 1u);
    pDst->height  = J3DMAX(pSrc->height / 2u, 1u);
    pDst->pPixels = (uint8_t*)malloc((size_t)pDst->width * pDst->height * 3u);
    TEST_ASSERT_NOT_NULL(pDst->pPixels);

    for ( uint32_t y = 0u; y < pDst->height; ++y )
    {
        for ( uint32_t x = 0u; x < pDst->width; ++x )
        {
            uint32_t aSums[3] = { 0u, 0u, 0u };
            uint32_t numSamples = 0u;

            for ( uint32_t sampleY = 0u; sampleY < 2u; ++sampleY )
            {
                uint32_t srcY = y * 2u + sampleY;
                if ( srcY >= pSrc->height )
                {
                    continue;
                }

                for ( uint32_t sampleX = 0u; sampleX < 2u; ++sampleX )
                {
                    uint32_t srcX = x * 2u + sampleX;
                    if ( srcX >= pSrc->width )
                    {
                        continue;
                    }

                    const uint8_t* pSrcPixel = &pSrc->pPixels[((size_t)srcY * pSrc->width + srcX) * 3u];
                    for ( size_t channel = 0u; channel < STD_ARRAYLEN(aSums); ++channel )
                    {
                        aSums[channel] += pSrcPixel[channel];
                    }

                    ++numSamples;
                }
            }

            TEST_ASSERT_TRUE(numSamples > 0u);
            uint8_t* pDstPixel = &pDst->pPixels[((size_t)y * pDst->width + x) * 3u];
            for ( size_t channel = 0u; channel < STD_ARRAYLEN(aSums); ++channel )
            {
                pDstPixel[channel] = (uint8_t)((aSums[channel] + numSamples / 2u) / numSamples);
            }
        }
    }
}

static size_t Std3DSystemTest_CreateMipmapLodTextureVBuffers(tVBuffer* apMipmaps[STD3DSYSTEMTEST_MIPMAP_LEVEL_COUNT])
{
    Std3DSystemTestBmp mipLevel;
    size_t numMipLevels = 0u;

    Std3DSystemTest_LoadBmp(
        "uv_grid_directx.bmp",
        &mipLevel,
        STD3DSYSTEMTEST_MIPMAP_TOP_LEVEL_SIZE,
        STD3DSYSTEMTEST_MIPMAP_TOP_LEVEL_SIZE
    );
    while ( true )
    {
        TEST_ASSERT_TRUE(numMipLevels < STD3DSYSTEMTEST_MIPMAP_LEVEL_COUNT);
        apMipmaps[numMipLevels++] = Std3DSystemTest_CreateTextureVBufferFromPixels(&mipLevel);

        if ( mipLevel.width == 1u && mipLevel.height == 1u )
        {
            break;
        }

        Std3DSystemTestBmp nextMipLevel;
        Std3DSystemTest_DownsampleBmp2x(&mipLevel, &nextMipLevel);
        Std3DSystemTest_FreeBmp(&mipLevel);
        mipLevel = nextMipLevel;
    }

    Std3DSystemTest_FreeBmp(&mipLevel);
    TEST_ASSERT_EQUAL_size_t(STD3DSYSTEMTEST_MIPMAP_LEVEL_COUNT, numMipLevels);
    return numMipLevels;
}

static void Std3DSystemTest_AssertSystemTextureAllocated(const tSystemTexture* pTexture)
{
#if defined(J3D_DIRECTX6)
    TEST_ASSERT_NOT_NULL(pTexture->pTexture);
#else
    TEST_ASSERT_TRUE(std3D_GetMipMapCount(pTexture) > 0u);
#endif
}

static void Std3DSystemTest_DrawCachedTextureQuad(tSysTexture* pTexture)
{
    D3DTLVERTEX aVerts[4];
    WORD aIndices[6] = { 0u, 1u, 2u, 0u, 2u, 3u };

    TEST_ASSERT_EQUAL_INT(0, stdDisplay_BackBufferFill(0u, NULL));
    std3D_ClearZBuffer();

    Std3DSystemTest_InitTexturedVertex(&aVerts[0], 96.0f, 72.0f, 0.4f, 0.0f, 0.0f, 0xFFFFFFFFu);
    Std3DSystemTest_InitTexturedVertex(&aVerts[1], 224.0f, 72.0f, 0.4f, 1.0f, 0.0f, 0xFFFFFFFFu);
    Std3DSystemTest_InitTexturedVertex(&aVerts[2], 224.0f, 168.0f, 0.4f, 1.0f, 1.0f, 0xFFFFFFFFu);
    Std3DSystemTest_InitTexturedVertex(&aVerts[3], 96.0f, 168.0f, 0.4f, 0.0f, 1.0f, 0xFFFFFFFFu);

    TEST_ASSERT_EQUAL_INT(0, std3D_StartScene());
    std3D_DrawRenderList(
        pTexture,
        STD3D_RS_TEXFILTER_BILINEAR | STD3D_RS_TEX_CPAMP_U | STD3D_RS_TEX_CPAMP_V,
        aVerts,
        STD_ARRAYLEN(aVerts),
        aIndices,
        STD_ARRAYLEN(aIndices)
    );
    Std3DSystemTest_EndScene();
}

static void Std3DSystemTest_DrawCachedTextureQuadVerts(tSysTexture* pTexture, D3DTLVERTEX aVerts[4], Std3DRenderState rdflags)
{
    WORD aIndices[6] = { 0u, 1u, 2u, 0u, 2u, 3u };

    TEST_ASSERT_EQUAL_INT(0, stdDisplay_BackBufferFill(0u, NULL));
    std3D_EnableFog(0, 0.0f);
    std3D_SetRenderState(0u);
    std3D_ClearZBuffer();

    TEST_ASSERT_EQUAL_INT(0, std3D_StartScene());
    std3D_DrawRenderList(pTexture, rdflags, aVerts, 4u, aIndices, STD_ARRAYLEN(aIndices));
    Std3DSystemTest_EndScene();
}

static void Std3DSystemTest_DrawFeatureCubeTexturedFaces(tSysTexture* pTexture, const D3DTLVERTEX aVerts[12], Std3DRenderState rdflags)
{
    WORD aIndices[6] = { 0u, 1u, 2u, 0u, 2u, 3u };

    TEST_ASSERT_EQUAL_INT(0, stdDisplay_BackBufferFill(0u, NULL));
    std3D_EnableFog(0, 0.0f);
    std3D_SetRenderState(0u);
    std3D_ClearZBuffer();

    TEST_ASSERT_EQUAL_INT(0, std3D_StartScene());
    for ( size_t faceNum = 0u; faceNum < 3u; ++faceNum )
    {
        std3D_DrawRenderList(pTexture, rdflags, (LPD3DTLVERTEX)&aVerts[faceNum * 4u], 4u, aIndices, STD_ARRAYLEN(aIndices));
    }
    Std3DSystemTest_EndScene();
}

static tSysTexture* Std3DSystemTest_CreateCachedMipmapLodTexture(tSystemTexture* pTexture, Std3DMipmapFilterType filter)
{
    tVBuffer* apMipmaps[STD3DSYSTEMTEST_MIPMAP_LEVEL_COUNT];

    TEST_ASSERT_EQUAL_INT(0, std3D_SetMipmapFilter(filter));

    STD_ZEROMEM(pTexture, sizeof(*pTexture));
    size_t numMipLevels = Std3DSystemTest_CreateMipmapLodTextureVBuffers(apMipmaps);

    std3D_AllocSystemTexture(pTexture, apMipmaps, numMipLevels, STDCOLOR_FORMAT_RGB);
    for ( size_t i = 0u; i < numMipLevels; ++i )
    {
        stdDisplay_VBufferFree(apMipmaps[i]);
    }

    TEST_ASSERT_TRUE(std3D_GetMipMapCount(pTexture) >= numMipLevels);
    std3D_AddToTextureCache(pTexture, STDCOLOR_FORMAT_RGB);
    if ( !pTexture->pCachedTexture )
    {
        std3D_ClearSystemTexture(pTexture);
        TEST_IGNORE_MESSAGE("Direct3D texture cache upload unavailable on this host.");
    }

    std3D_UpdateFrameCount(pTexture);
    return pTexture->pCachedTexture;
}

#if defined(J3D_DIRECTX9)
static tSysTexture* Std3DSystemTest_CreateCachedMipmapAutoGenTexture(tSystemTexture* pTexture)
{
    tVBuffer* apMipmaps[1];

    TEST_ASSERT_EQUAL_INT(0, std3D_SetMipmapFilter(STD3D_MIPMAPFILTER_TRILINEAR));

    STD_ZEROMEM(pTexture, sizeof(*pTexture));
    apMipmaps[0] = Std3DSystemTest_CreateTextureVBufferFromBmp("uv_grid_directx.bmp");
    std3D_AllocSystemTexture(pTexture, apMipmaps, STD_ARRAYLEN(apMipmaps), STDCOLOR_FORMAT_RGB);
    stdDisplay_VBufferFree(apMipmaps[0]);

    TEST_ASSERT_EQUAL_size_t(1u, std3D_GetMipMapCount(pTexture));

    LPDIRECT3D9 pD3D = stdDisplay_GetDirect3D();
    TEST_ASSERT_NOT_NULL(pD3D);

    D3DDISPLAYMODE displayMode;
    TEST_ASSERT_EQUAL_HEX32(D3D_OK, IDirect3D9_GetAdapterDisplayMode(pD3D, D3DADAPTER_DEFAULT, &displayMode));

    HRESULT autoGenSupport = IDirect3D9_CheckDeviceFormat(
        pD3D,
        D3DADAPTER_DEFAULT,
        D3DDEVTYPE_HAL,
        displayMode.Format,
        D3DUSAGE_AUTOGENMIPMAP,
        D3DRTYPE_TEXTURE,
        pTexture->format
    );
    if ( autoGenSupport != D3D_OK )
    {
        std3D_ClearSystemTexture(pTexture);
        TEST_IGNORE_MESSAGE("Automatic mipmap generation is unavailable for the selected DX9 texture format.");
    }

    std3D_AddToTextureCache(pTexture, STDCOLOR_FORMAT_RGB);
    if ( !pTexture->pCachedTexture )
    {
        std3D_ClearSystemTexture(pTexture);
        TEST_IGNORE_MESSAGE("Direct3D automatic-mipmap texture upload unavailable on this host.");
    }

    D3DSURFACE_DESC desc;
    TEST_ASSERT_EQUAL_HEX32(D3D_OK, IDirect3DTexture9_GetLevelDesc(pTexture->pCachedTexture, 0u, &desc));
    if ( (desc.Usage & D3DUSAGE_AUTOGENMIPMAP) == 0 )
    {
        std3D_ClearSystemTexture(pTexture);
        TEST_IGNORE_MESSAGE("Automatic mipmap generation is unavailable for the selected DX9 texture format.");
    }

    // Direct3D exposes only level zero for an automatically generated mip chain.
    TEST_ASSERT_EQUAL_UINT32(1u, IDirect3DTexture9_GetLevelCount(pTexture->pCachedTexture));

    D3DTEXTUREFILTERTYPE filter = IDirect3DTexture9_GetAutoGenFilterType(pTexture->pCachedTexture);
    TEST_ASSERT_TRUE(filter == D3DTEXF_LINEAR || filter == D3DTEXF_POINT);

    std3D_UpdateFrameCount(pTexture);
    return pTexture->pCachedTexture;
}
#endif

static void Std3DSystemTest_DrawCachedCubeTextureFace(tSysTexture* pTexture)
{
    D3DTLVERTEX aVerts[4];
    WORD aIndices[6] = { 0u, 1u, 2u, 0u, 2u, 3u };

    TEST_ASSERT_EQUAL_INT(0, stdDisplay_BackBufferFill(0u, NULL));
    std3D_EnableFog(0, 0.0f);
    std3D_SetRenderState(0u);
    std3D_ClearZBuffer();

    Std3DSystemTest_InitCubeTexturedVertex(&aVerts[0], 0u, 0.0f, 0.0f, 0xFFFFFFFFu);
    Std3DSystemTest_InitCubeTexturedVertex(&aVerts[1], 1u, 1.0f, 0.0f, 0xFFFFFFFFu);
    Std3DSystemTest_InitCubeTexturedVertex(&aVerts[2], 2u, 1.0f, 1.0f, 0xFFFFFFFFu);
    Std3DSystemTest_InitCubeTexturedVertex(&aVerts[3], 3u, 0.0f, 1.0f, 0xFFFFFFFFu);

    TEST_ASSERT_EQUAL_INT(0, std3D_StartScene());
    std3D_DrawRenderList(
        pTexture,
        STD3D_RS_TEX_CPAMP_U | STD3D_RS_TEX_CPAMP_V,
        aVerts,
        STD_ARRAYLEN(aVerts),
        aIndices,
        STD_ARRAYLEN(aIndices)
    );
    Std3DSystemTest_EndScene();
}

static void Std3DSystemTest_RenderTextureQuadFromMipmaps(tVBuffer** apMipmaps, size_t numMipLevels, StdColorFormatType formatType, size_t minExpectedMipLevels)
{
    tSystemTexture texture;

    STD_ZEROMEM(&texture, sizeof(texture));
    std3D_AllocSystemTexture(&texture, apMipmaps, numMipLevels, formatType);
    for ( size_t i = 0u; i < numMipLevels; ++i )
    {
        stdDisplay_VBufferFree(apMipmaps[i]);
    }

    TEST_ASSERT_TRUE(std3D_GetMipMapCount(&texture) >= minExpectedMipLevels);
    std3D_AddToTextureCache(&texture, formatType);
    if ( !texture.pCachedTexture )
    {
        std3D_ClearSystemTexture(&texture);
        TEST_IGNORE_MESSAGE("Direct3D texture cache upload unavailable on this host.");
    }

    std3D_UpdateFrameCount(&texture);
    Std3DSystemTest_DrawCachedTextureQuad(texture.pCachedTexture);
    std3D_ClearSystemTexture(&texture);
}

static tSysTexture* Std3DSystemTest_CreateCachedCubeVectorTexture(tSystemTexture* pTexture)
{
    tVBuffer* apMipmaps[1];

    STD_ZEROMEM(pTexture, sizeof(*pTexture));
    apMipmaps[0] = Std3DSystemTest_CreateTextureVBufferFromBmp("uv_grid_directx.bmp");
    std3D_AllocSystemTexture(pTexture, apMipmaps, STD_ARRAYLEN(apMipmaps), STDCOLOR_FORMAT_RGB);
    stdDisplay_VBufferFree(apMipmaps[0]);

    Std3DSystemTest_AssertSystemTextureAllocated(pTexture);
    std3D_AddToTextureCache(pTexture, STDCOLOR_FORMAT_RGB);
    if ( !pTexture->pCachedTexture )
    {
        std3D_ClearSystemTexture(pTexture);
        TEST_IGNORE_MESSAGE("Direct3D texture cache upload unavailable on this host.");
    }

    std3D_UpdateFrameCount(pTexture);
    return pTexture->pCachedTexture;
}

static void Std3DSystemTest_PrepareCubeVectorFrame(void)
{
    TEST_ASSERT_EQUAL_INT(0, stdDisplay_BackBufferFill(0u, NULL));
    std3D_EnableFog(0, 0.0f);
    std3D_SetRenderState(0u);
    TEST_ASSERT_EQUAL_INT(0, std3D_SetMipmapFilter(STD3D_MIPMAPFILTER_NONE));
    std3D_ClearZBuffer();
}

static void Std3DSystemTest_DrawCubeVectorPointFrame(const Std3DSystemTestScreenVertex aCubeVerts[8])
{
    D3DTLVERTEX aVerts[8];

    for ( size_t i = 0u; i < STD_ARRAYLEN(aVerts); ++i )
    {
        Std3DSystemTest_InitProjectedCubeVertex(&aVerts[i], aCubeVerts, i, 0xFFFFFFFFu);
    }

    std3D_SetWireframeRenderState();
    std3D_DrawPointList(aVerts, STD_ARRAYLEN(aVerts));
}

static void Std3DSystemTest_DrawCubeVectorWireframe(const Std3DSystemTestScreenVertex aCubeVerts[8])
{
    static const uint8_t aaEdges[][2] =
    {
        { 0u, 1u },
        { 1u, 2u },
        { 2u, 3u },
        { 3u, 0u },
        { 4u, 5u },
        { 5u, 6u },
        { 6u, 7u },
        { 7u, 4u },
        { 0u, 4u },
        { 1u, 5u },
        { 2u, 6u },
        { 3u, 7u },
    };

    std3D_SetWireframeRenderState();
    for ( size_t edgeNum = 0u; edgeNum < STD_ARRAYLEN(aaEdges); ++edgeNum )
    {
        D3DTLVERTEX aEdgeVerts[2];

        Std3DSystemTest_InitProjectedCubeVertex(&aEdgeVerts[0], aCubeVerts, aaEdges[edgeNum][0], 0xFFFFFFFFu);
        Std3DSystemTest_InitProjectedCubeVertex(&aEdgeVerts[1], aCubeVerts, aaEdges[edgeNum][1], 0xFFFFFFFFu);
        std3D_DrawLineStrip(aEdgeVerts, STD_ARRAYLEN(aEdgeVerts));
    }
}

static uint32_t Std3DSystemTest_GetCubeVectorFaceVertexColor(Std3DSystemTestCubeVectorMode mode, size_t faceNum, size_t cornerNum)
{
    size_t vertexNum = std3DSystemTest_aCubeFaces[faceNum].aVertices[cornerNum];

    switch ( mode )
    {
        case STD3D_SYSTEM_TEST_CUBE_VECTOR_SOLID:
            return 0xFFFFFFFFu;

        case STD3D_SYSTEM_TEST_CUBE_VECTOR_TEXTURED:
            return 0xFFFFFFFFu;

        case STD3D_SYSTEM_TEST_CUBE_VECTOR_TEXTURED_VERTEX_COLOR:
            return std3DSystemTest_aCubeVertexColors[vertexNum];

        case STD3D_SYSTEM_TEST_CUBE_VECTOR_SOLID_VERTEX_INTENSITY:
            return std3DSystemTest_aCubeVertexColors[vertexNum];

        default:
            return 0xFFFFFFFFu;
    }
}

static void Std3DSystemTest_DrawCubeVectorFaceFrame(Std3DSystemTestCubeVectorMode mode, const Std3DSystemTestScreenVertex aCubeVerts[8], const bool abVisibleFaces[STD_ARRAYLEN(std3DSystemTest_aCubeFaces)], tSysTexture* pTexture)
{
    WORD aIndices[6] = { 0u, 1u, 2u, 0u, 2u, 3u };
    Std3DRenderState rdflags = pTexture
        ? STD3D_RS_TEXFILTER_BILINEAR | STD3D_RS_TEX_CPAMP_U | STD3D_RS_TEX_CPAMP_V
        : 0u;

    for ( size_t faceNum = 0u; faceNum < STD_ARRAYLEN(std3DSystemTest_aCubeFaces); ++faceNum )
    {
        D3DTLVERTEX aVerts[4];

        if ( !abVisibleFaces[faceNum] )
        {
            continue;
        }

        for ( size_t cornerNum = 0u; cornerNum < STD_ARRAYLEN(std3DSystemTest_aCubeFaces[faceNum].aVertices); ++cornerNum )
        {
            size_t vertexNum = std3DSystemTest_aCubeFaces[faceNum].aVertices[cornerNum];
            uint32_t color   = Std3DSystemTest_GetCubeVectorFaceVertexColor(mode, faceNum, cornerNum);

            if ( pTexture )
            {
                float tu = std3DSystemTest_aCubeTexCoords[cornerNum][0];
                float tv = std3DSystemTest_aCubeTexCoords[cornerNum][1];

                Std3DSystemTest_InitProjectedCubeTexturedVertex(&aVerts[cornerNum], aCubeVerts, vertexNum, tu, tv, color);
            }
            else
            {
                Std3DSystemTest_InitProjectedCubeVertex(&aVerts[cornerNum], aCubeVerts, vertexNum, color);
            }
        }

        std3D_DrawRenderList(pTexture, rdflags, aVerts, STD_ARRAYLEN(aVerts), aIndices, STD_ARRAYLEN(aIndices));
    }
}

static void Std3DSystemTest_DrawFeatureCubeTexturedFrame(const Std3DSystemTestScreenVertex aCubeVerts[8], const bool abVisibleFaces[STD_ARRAYLEN(std3DSystemTest_aCubeFaces)], tSysTexture* pTexture, Std3DRenderState rdflags, float texCoordScale)
{
    WORD aIndices[6] = { 0u, 1u, 2u, 0u, 2u, 3u };

    for ( size_t faceNum = 0u; faceNum < STD_ARRAYLEN(std3DSystemTest_aCubeFaces); ++faceNum )
    {
        D3DTLVERTEX aVerts[4];

        if ( !abVisibleFaces[faceNum] )
        {
            continue;
        }

        for ( size_t cornerNum = 0u; cornerNum < STD_ARRAYLEN(std3DSystemTest_aCubeFaces[faceNum].aVertices); ++cornerNum )
        {
            size_t vertexNum = std3DSystemTest_aCubeFaces[faceNum].aVertices[cornerNum];
            float tu         = std3DSystemTest_aCubeTexCoords[cornerNum][0] * texCoordScale;
            float tv         = std3DSystemTest_aCubeTexCoords[cornerNum][1] * texCoordScale;

            Std3DSystemTest_InitProjectedCubeTexturedVertex(&aVerts[cornerNum], aCubeVerts, vertexNum, tu, tv, 0xFFFFFFFFu);
        }

        std3D_DrawRenderList(pTexture, rdflags, aVerts, STD_ARRAYLEN(aVerts), aIndices, STD_ARRAYLEN(aIndices));
    }
}

bool Std3DSystemTest_CurrentDeviceSupportsFog(void)
{
    const Device3D* pDevice = std3D_GetCurrentDevice();

    TEST_ASSERT_NOT_NULL(pDevice);
#if defined(J3D_DIRECTX9)
    return std3D_IsShaderSystemActive()
        || (pDevice->d3dDesc.RasterCaps & (D3DPRASTERCAPS_FOGTABLE | D3DPRASTERCAPS_FOGVERTEX)) != 0;
#elif defined(J3D_DIRECTX6)
    return (pDevice->d3dDesc.dpcTriCaps.dwRasterCaps & (D3DPRASTERCAPS_FOGTABLE | D3DPRASTERCAPS_FOGVERTEX)) != 0;
#else
    return false;
#endif
}

bool Std3DSystemTest_CurrentDeviceSupportsMipmaps(void)
{
    const Device3D* pDevice = std3D_GetCurrentDevice();

    TEST_ASSERT_NOT_NULL(pDevice);
#if defined(J3D_DIRECTX9)
    return (pDevice->d3dDesc.TextureFilterCaps & D3DPTFILTERCAPS_MIPFPOINT) != 0;
#elif defined(J3D_DIRECTX6)
    return (pDevice->d3dDesc.dpcTriCaps.dwTextureFilterCaps & D3DPTFILTERCAPS_MIPFPOINT) != 0;
#else
    return false;
#endif
}

const char* Std3DSystemTest_GetCubeVectorModeName(Std3DSystemTestCubeVectorMode mode)
{
    TEST_ASSERT_TRUE((size_t)mode < STD_ARRAYLEN(std3DSystemTest_aCubeVectorModeNames));
    return std3DSystemTest_aCubeVectorModeNames[mode];
}

void Std3DSystemTest_RenderCubeVectorFrame(Std3DSystemTestCubeVectorMode mode, size_t frameNum)
{
    Std3DSystemTestScreenVertex aCubeVerts[8];
    bool abVisibleFaces[STD_ARRAYLEN(std3DSystemTest_aCubeFaces)];
    tSystemTexture texture;
    tSysTexture* pTexture = NULL;

    TEST_ASSERT_TRUE(frameNum < STD3D_SYSTEM_TEST_CUBE_VECTOR_FRAME_COUNT);
    TEST_ASSERT_TRUE((size_t)mode < STD_ARRAYLEN(std3DSystemTest_aCubeVectorModeNames));

    STD_ZEROMEM(&texture, sizeof(texture));
    Std3DSystemTest_ProjectCubeVectorFrame(frameNum, aCubeVerts);
    Std3DSystemTest_CalculateCubeVectorFaceVisibility(frameNum, abVisibleFaces);
    if ( mode == STD3D_SYSTEM_TEST_CUBE_VECTOR_TEXTURED
        || mode == STD3D_SYSTEM_TEST_CUBE_VECTOR_TEXTURED_VERTEX_COLOR )
    {
        TEST_ASSERT_EQUAL_INT(0, std3D_SetMipmapFilter(STD3D_MIPMAPFILTER_NONE));
        pTexture = Std3DSystemTest_CreateCachedCubeVectorTexture(&texture);
    }

    Std3DSystemTest_PrepareCubeVectorFrame();

    TEST_ASSERT_EQUAL_INT(0, std3D_StartScene());
    switch ( mode )
    {
        case STD3D_SYSTEM_TEST_CUBE_VECTOR_VERTICES:
            Std3DSystemTest_DrawCubeVectorPointFrame(aCubeVerts);
            break;

        case STD3D_SYSTEM_TEST_CUBE_VECTOR_WIREFRAME:
            Std3DSystemTest_DrawCubeVectorWireframe(aCubeVerts);
            break;

        case STD3D_SYSTEM_TEST_CUBE_VECTOR_SOLID:
        case STD3D_SYSTEM_TEST_CUBE_VECTOR_TEXTURED:
        case STD3D_SYSTEM_TEST_CUBE_VECTOR_TEXTURED_VERTEX_COLOR:
        case STD3D_SYSTEM_TEST_CUBE_VECTOR_SOLID_VERTEX_INTENSITY:
            Std3DSystemTest_DrawCubeVectorFaceFrame(mode, aCubeVerts, abVisibleFaces, pTexture);
            break;

        default:
            TEST_FAIL_MESSAGE("Unknown std3D cube vector mode.");
            break;
    }
    Std3DSystemTest_EndScene();

    if ( pTexture )
    {
        std3D_ClearSystemTexture(&texture);
    }
}

void Std3DSystemTest_RenderPrimitiveHarnessScene(void)
{
    D3DTLVERTEX aTriangleVerts[3];
    D3DTLVERTEX aLineVerts[2];
    D3DTLVERTEX pointVert;
    WORD aIndices[3] = { 0u, 1u, 2u };

    TEST_ASSERT_EQUAL_INT(0, stdDisplay_BackBufferFill(0u, NULL));
    std3D_EnableFog(0, 0.0f);
    std3D_SetRenderState(0u);
    std3D_ClearZBuffer();

    Std3DSystemTest_InitVertex(&aTriangleVerts[0], 80.0f, 80.0f, 0.5f, 0xFF00FF00u);
    Std3DSystemTest_InitVertex(&aTriangleVerts[1], 240.0f, 80.0f, 0.5f, 0xFF00FF00u);
    Std3DSystemTest_InitVertex(&aTriangleVerts[2], 160.0f, 190.0f, 0.5f, 0xFF00FF00u);

    Std3DSystemTest_InitVertex(&aLineVerts[0], 40.0f, 210.0f, 0.4f, 0xFF0000FFu);
    Std3DSystemTest_InitVertex(&aLineVerts[1], 160.0f, 210.0f, 0.4f, 0xFF0000FFu);

    Std3DSystemTest_InitVertex(&pointVert, 40.0f, 40.0f, 0.3f, 0xFFFF0000u);

    TEST_ASSERT_EQUAL_INT(0, std3D_StartScene());
    std3D_DrawRenderList(NULL, 0u, aTriangleVerts, STD_ARRAYLEN(aTriangleVerts), aIndices, STD_ARRAYLEN(aIndices));
    std3D_DrawLineStrip(aLineVerts, STD_ARRAYLEN(aLineVerts));
    std3D_DrawPointList(&pointVert, 1u);
    Std3DSystemTest_EndScene();
}

void Std3DSystemTest_RenderVertexGeometryCubeScene(void)
{
    D3DTLVERTEX aVerts[8];

    TEST_ASSERT_EQUAL_INT(0, stdDisplay_BackBufferFill(0u, NULL));
    std3D_EnableFog(0, 0.0f);
    std3D_SetRenderState(0u);
    std3D_ClearZBuffer();

    for ( size_t i = 0u; i < STD_ARRAYLEN(aVerts); ++i )
    {
        Std3DSystemTest_InitCubeVertex(&aVerts[i], i, 0xFFFFFFFFu);
    }

    TEST_ASSERT_EQUAL_INT(0, std3D_StartScene());
    std3D_SetWireframeRenderState();
    std3D_DrawPointList(aVerts, STD_ARRAYLEN(aVerts));
    Std3DSystemTest_EndScene();
}

void Std3DSystemTest_RenderWireframeGeometryCubeScene(void)
{
    static const size_t aFront[] = { 0u, 1u, 2u, 3u, 0u };
    static const size_t aBack[]  = { 4u, 5u, 6u, 7u, 4u };
    static const size_t aEdge0[] = { 0u, 4u };
    static const size_t aEdge1[] = { 1u, 5u };
    static const size_t aEdge2[] = { 2u, 6u };
    static const size_t aEdge3[] = { 3u, 7u };
    static const size_t* const apLines[] =
    {
        aFront,
        aBack,
        aEdge0,
        aEdge1,
        aEdge2,
        aEdge3,
    };
    static const size_t aLineSizes[] =
    {
        STD_ARRAYLEN(aFront),
        STD_ARRAYLEN(aBack),
        STD_ARRAYLEN(aEdge0),
        STD_ARRAYLEN(aEdge1),
        STD_ARRAYLEN(aEdge2),
        STD_ARRAYLEN(aEdge3),
    };

    TEST_ASSERT_EQUAL_INT(0, stdDisplay_BackBufferFill(0u, NULL));
    std3D_EnableFog(0, 0.0f);
    std3D_SetRenderState(0u);
    std3D_ClearZBuffer();

    TEST_ASSERT_EQUAL_INT(0, std3D_StartScene());
    std3D_SetWireframeRenderState();
    for ( size_t lineNum = 0u; lineNum < STD_ARRAYLEN(apLines); ++lineNum )
    {
        D3DTLVERTEX aVerts[5];

        for ( size_t i = 0u; i < aLineSizes[lineNum]; ++i )
        {
            Std3DSystemTest_InitCubeVertex(&aVerts[i], apLines[lineNum][i], 0xFFFFFFFFu);
        }

        std3D_DrawLineStrip(aVerts, aLineSizes[lineNum]);
    }
    Std3DSystemTest_EndScene();
}

void Std3DSystemTest_RenderSolidGeometryCubeScene(void)
{
    D3DTLVERTEX aVerts[12];
    WORD aIndices[6] = { 0u, 1u, 2u, 0u, 2u, 3u };

    TEST_ASSERT_EQUAL_INT(0, stdDisplay_BackBufferFill(0u, NULL));
    std3D_EnableFog(0, 0.0f);
    std3D_SetRenderState(0u);
    std3D_ClearZBuffer();

    Std3DSystemTest_InitCubeVertex(&aVerts[0], 4u, 0xFFFF0000u);
    Std3DSystemTest_InitCubeVertex(&aVerts[1], 5u, 0xFFFF0000u);
    Std3DSystemTest_InitCubeVertex(&aVerts[2], 1u, 0xFFFF0000u);
    Std3DSystemTest_InitCubeVertex(&aVerts[3], 0u, 0xFFFF0000u);

    Std3DSystemTest_InitCubeVertex(&aVerts[4], 4u, 0xFF0000FFu);
    Std3DSystemTest_InitCubeVertex(&aVerts[5], 0u, 0xFF0000FFu);
    Std3DSystemTest_InitCubeVertex(&aVerts[6], 3u, 0xFF0000FFu);
    Std3DSystemTest_InitCubeVertex(&aVerts[7], 7u, 0xFF0000FFu);

    Std3DSystemTest_InitCubeVertex(&aVerts[8], 0u, 0xFF00FF00u);
    Std3DSystemTest_InitCubeVertex(&aVerts[9], 1u, 0xFF00FF00u);
    Std3DSystemTest_InitCubeVertex(&aVerts[10], 2u, 0xFF00FF00u);
    Std3DSystemTest_InitCubeVertex(&aVerts[11], 3u, 0xFF00FF00u);

    TEST_ASSERT_EQUAL_INT(0, std3D_StartScene());
    std3D_DrawRenderList(NULL, 0u, &aVerts[0], 4u, aIndices, STD_ARRAYLEN(aIndices));
    std3D_DrawRenderList(NULL, 0u, &aVerts[4], 4u, aIndices, STD_ARRAYLEN(aIndices));
    std3D_DrawRenderList(NULL, 0u, &aVerts[8], 4u, aIndices, STD_ARRAYLEN(aIndices));
    Std3DSystemTest_EndScene();
}

void Std3DSystemTest_RenderTexturedGeometryCubeScene(void)
{
    tSystemTexture texture;
    tVBuffer* apMipmaps[1];

    STD_ZEROMEM(&texture, sizeof(texture));
    apMipmaps[0] = Std3DSystemTest_CreateCubePatternTextureVBuffer();
    std3D_AllocSystemTexture(&texture, apMipmaps, STD_ARRAYLEN(apMipmaps), STDCOLOR_FORMAT_RGB);
    stdDisplay_VBufferFree(apMipmaps[0]);

    Std3DSystemTest_AssertSystemTextureAllocated(&texture);
    std3D_AddToTextureCache(&texture, STDCOLOR_FORMAT_RGB);
    if ( !texture.pCachedTexture )
    {
        std3D_ClearSystemTexture(&texture);
        TEST_IGNORE_MESSAGE("Direct3D texture cache upload unavailable on this host.");
    }

    std3D_UpdateFrameCount(&texture);
    Std3DSystemTest_DrawCachedCubeTextureFace(texture.pCachedTexture);
    std3D_ClearSystemTexture(&texture);
}

void Std3DSystemTest_RenderFogGeometryScene(void)
{
    D3DTLVERTEX aNearVerts[4];
    D3DTLVERTEX aFarVerts[4];
    WORD aIndices[6] = { 0u, 1u, 2u, 0u, 2u, 3u };

    TEST_ASSERT_EQUAL_INT(0, stdDisplay_BackBufferFill(0u, NULL));
    TEST_ASSERT_EQUAL_INT(0, std3D_SetProjection(1.04719758f, 0.1f, 1.0f));
    std3D_EnableFog(1, 1.0f);
    std3D_SetFog(1.0f, 0.0f, 0.0f, 1.20f, 10.0f);
    std3D_SetRenderState(0u);
    std3D_ClearZBuffer();

    Std3DSystemTest_InitVertex(&aNearVerts[0], 64.0f, 70.0f, 0.4f, 0xFF00FF00u);
    Std3DSystemTest_InitVertex(&aNearVerts[1], 144.0f, 70.0f, 0.4f, 0xFF00FF00u);
    Std3DSystemTest_InitVertex(&aNearVerts[2], 144.0f, 170.0f, 0.4f, 0xFF00FF00u);
    Std3DSystemTest_InitVertex(&aNearVerts[3], 64.0f, 170.0f, 0.4f, 0xFF00FF00u);

    Std3DSystemTest_InitVertex(&aFarVerts[0], 176.0f, 70.0f, 0.4f, 0xFF00FF00u);
    Std3DSystemTest_InitVertex(&aFarVerts[1], 256.0f, 70.0f, 0.4f, 0xFF00FF00u);
    Std3DSystemTest_InitVertex(&aFarVerts[2], 256.0f, 170.0f, 0.4f, 0xFF00FF00u);
    Std3DSystemTest_InitVertex(&aFarVerts[3], 176.0f, 170.0f, 0.4f, 0xFF00FF00u);

    for ( size_t i = 0u; i < STD_ARRAYLEN(aNearVerts); ++i )
    {
        aNearVerts[i].rhw = 0.95f;
        aFarVerts[i].rhw  = 0.05f;
    }

    TEST_ASSERT_EQUAL_INT(0, std3D_StartScene());
    std3D_DrawRenderList(NULL, STD3D_RS_FOG_ENABLED, aNearVerts, STD_ARRAYLEN(aNearVerts), aIndices, STD_ARRAYLEN(aIndices));
    std3D_DrawRenderList(NULL, STD3D_RS_FOG_ENABLED, aFarVerts, STD_ARRAYLEN(aFarVerts), aIndices, STD_ARRAYLEN(aIndices));
    Std3DSystemTest_EndScene();
}

void Std3DSystemTest_RenderSolidTextureQuadScene(uint8_t red, uint8_t green, uint8_t blue)
{
    tVBuffer* apMipmaps[1];

    apMipmaps[0] = Std3DSystemTest_CreateSolidTextureVBuffer(4u, 4u, red, green, blue);
    Std3DSystemTest_RenderTextureQuadFromMipmaps(apMipmaps, STD_ARRAYLEN(apMipmaps), STDCOLOR_FORMAT_RGB, 1u);
}

void Std3DSystemTest_RenderMultiMipmapTextureQuadScene(uint8_t red, uint8_t green, uint8_t blue)
{
    tVBuffer* apMipmaps[3];

    TEST_ASSERT_EQUAL_INT(0, std3D_SetMipmapFilter(STD3D_MIPMAPFILTER_BILINEAR));
    apMipmaps[0] = Std3DSystemTest_CreateSolidTextureVBuffer(8u, 8u, red, green, blue);
#if defined(J3D_DIRECTX6)
    apMipmaps[1] = Std3DSystemTest_CreateSolidTextureVBuffer(8u, 8u, red, green, blue);
    apMipmaps[2] = Std3DSystemTest_CreateSolidTextureVBuffer(8u, 8u, red, green, blue);
#else
    apMipmaps[1] = Std3DSystemTest_CreateSolidTextureVBuffer(4u, 4u, red, green, blue);
    apMipmaps[2] = Std3DSystemTest_CreateSolidTextureVBuffer(2u, 2u, red, green, blue);
#endif
    Std3DSystemTest_RenderTextureQuadFromMipmaps(apMipmaps, STD_ARRAYLEN(apMipmaps), STDCOLOR_FORMAT_RGB, STD_ARRAYLEN(apMipmaps));
}

void Std3DSystemTest_RenderMipmapLodQuadScene(void)
{
    tSystemTexture texture;
    tVBuffer* apMipmaps[3];
    D3DTLVERTEX aVerts[4];

    TEST_ASSERT_EQUAL_INT(0, std3D_SetMipmapFilter(STD3D_MIPMAPFILTER_BILINEAR));

    STD_ZEROMEM(&texture, sizeof(texture));
    apMipmaps[0] = Std3DSystemTest_CreateSolidTextureVBuffer(64u, 64u, 255u, 0u, 0u);
    apMipmaps[1] = Std3DSystemTest_CreateSolidTextureVBuffer(32u, 32u, 0u, 255u, 0u);
    apMipmaps[2] = Std3DSystemTest_CreateSolidTextureVBuffer(16u, 16u, 0u, 0u, 255u);

    std3D_AllocSystemTexture(&texture, apMipmaps, STD_ARRAYLEN(apMipmaps), STDCOLOR_FORMAT_RGB);
    for ( size_t i = 0u; i < STD_ARRAYLEN(apMipmaps); ++i )
    {
        stdDisplay_VBufferFree(apMipmaps[i]);
    }

    TEST_ASSERT_TRUE(std3D_GetMipMapCount(&texture) >= STD_ARRAYLEN(apMipmaps));
    std3D_AddToTextureCache(&texture, STDCOLOR_FORMAT_RGB);
    if ( !texture.pCachedTexture )
    {
        std3D_ClearSystemTexture(&texture);
        TEST_IGNORE_MESSAGE("Direct3D texture cache upload unavailable on this host.");
    }

    std3D_UpdateFrameCount(&texture);

    Std3DSystemTest_InitTexturedVertex(&aVerts[0], 152.0f, 112.0f, 0.4f, 0.0f, 0.0f, 0xFFFFFFFFu);
    Std3DSystemTest_InitTexturedVertex(&aVerts[1], 168.0f, 112.0f, 0.4f, 1.0f, 0.0f, 0xFFFFFFFFu);
    Std3DSystemTest_InitTexturedVertex(&aVerts[2], 168.0f, 128.0f, 0.4f, 1.0f, 1.0f, 0xFFFFFFFFu);
    Std3DSystemTest_InitTexturedVertex(&aVerts[3], 152.0f, 128.0f, 0.4f, 0.0f, 1.0f, 0xFFFFFFFFu);

    Std3DSystemTest_DrawCachedTextureQuadVerts(
        texture.pCachedTexture,
        aVerts,
        STD3D_RS_TEXFILTER_BILINEAR | STD3D_RS_TEX_CPAMP_U | STD3D_RS_TEX_CPAMP_V
    );
    std3D_ClearSystemTexture(&texture);
}

static void Std3DSystemTest_RenderMipmapCubeFrame(size_t frameNum, tSysTexture* pTexture, Std3DRenderState rdflags, bool bHighQuality)
{
    TEST_ASSERT_TRUE(frameNum < STD3D_SYSTEM_TEST_FEATURE_CUBE_FRAME_COUNT);

    Std3DSystemTestScreenVertex aCubeVerts[8];
    if ( bHighQuality )
    {
        Std3DSystemTest_ProjectHighQualityFeatureCubeFrame(frameNum, aCubeVerts);
    }
    else
    {
        Std3DSystemTest_ProjectMipmapCubeFrame(frameNum, aCubeVerts);
    }

    bool abVisibleFaces[STD_ARRAYLEN(std3DSystemTest_aCubeFaces)];
    Std3DSystemTest_CalculateCubeFaceVisibility(frameNum, STD3DSYSTEMTEST_CUBE_FRAME_DEGREES, abVisibleFaces);

    TEST_ASSERT_EQUAL_INT(0, stdDisplay_BackBufferFill(0u, NULL));
    std3D_EnableFog(0, 0.0f);
    std3D_SetRenderState(0u);
    std3D_ClearZBuffer();

    TEST_ASSERT_EQUAL_INT(0, std3D_StartScene());
    Std3DSystemTest_DrawFeatureCubeTexturedFrame(
        aCubeVerts,
        abVisibleFaces,
        pTexture,
        rdflags,
        1.0f
    );
    Std3DSystemTest_EndScene();
}

void Std3DSystemTest_RenderMipmapLodCubeFrame(size_t frameNum)
{
    tSystemTexture texture;
    tSysTexture* pTexture = Std3DSystemTest_CreateCachedMipmapLodTexture(&texture, STD3D_MIPMAPFILTER_BILINEAR);

    Std3DSystemTest_RenderMipmapCubeFrame(
        frameNum,
        pTexture,
        STD3D_RS_TEXFILTER_BILINEAR | STD3D_RS_TEX_CPAMP_U | STD3D_RS_TEX_CPAMP_V,
        false
    );

    std3D_ClearSystemTexture(&texture);
}

void Std3DSystemTest_RenderHighQualityMipmapLodCubeFrame(size_t frameNum)
{
    tSystemTexture texture;
    tSysTexture* pTexture = Std3DSystemTest_CreateCachedMipmapLodTexture(&texture, STD3D_MIPMAPFILTER_BILINEAR);

    Std3DSystemTest_RenderMipmapCubeFrame(
        frameNum,
        pTexture,
        STD3D_RS_TEXFILTER_BILINEAR | STD3D_RS_TEX_CPAMP_U | STD3D_RS_TEX_CPAMP_V,
        true
    );

    std3D_ClearSystemTexture(&texture);
}

static void Std3DSystemTest_RenderMipmapAutoGenCubeFrameWithState(size_t frameNum, Std3DRenderState rdflags)
{
#if defined(J3D_DIRECTX9)
    tSystemTexture texture;
    tSysTexture* pTexture = Std3DSystemTest_CreateCachedMipmapAutoGenTexture(&texture);

    Std3DSystemTest_RenderMipmapCubeFrame(frameNum, pTexture, rdflags, true);

    std3D_ClearSystemTexture(&texture);
#else
    J3D_UNUSED(rdflags);
    J3D_UNUSED(frameNum);
    TEST_FAIL_MESSAGE("Automatic mipmap generation is only available in the DX9 backend.");
#endif
}

void Std3DSystemTest_RenderMipmapAutoGenCubeFrame(size_t frameNum)
{
    Std3DSystemTest_RenderMipmapAutoGenCubeFrameWithState(
        frameNum,
        STD3D_RS_TEXFILTER_BILINEAR | STD3D_RS_TEX_CPAMP_U | STD3D_RS_TEX_CPAMP_V
    );
}

void Std3DSystemTest_RenderMipmapAutoGenAnisotropicCubeFrame(size_t frameNum)
{
    Std3DSystemTest_RenderMipmapAutoGenCubeFrameWithState(
        frameNum,
        STD3D_RS_TEXFILTER_ANISOTROPIC | STD3D_RS_TEX_CPAMP_U | STD3D_RS_TEX_CPAMP_V
    );
}

void Std3DSystemTest_RenderMipmapAnisotropicCubeFrame(size_t frameNum)
{
    tSystemTexture texture;
    tSysTexture* pTexture = Std3DSystemTest_CreateCachedMipmapLodTexture(&texture, STD3D_MIPMAPFILTER_TRILINEAR);

    Std3DSystemTest_RenderMipmapCubeFrame(
        frameNum,
        pTexture,
        STD3D_RS_TEXFILTER_ANISOTROPIC | STD3D_RS_TEX_CPAMP_U | STD3D_RS_TEX_CPAMP_V,
        true
    );

    std3D_ClearSystemTexture(&texture);
}

void Std3DSystemTest_RenderAnisotropicTextureFilterScene(void)
{
    tSystemTexture texture;
    tVBuffer* apMipmaps[1];
    D3DTLVERTEX aVerts[4];

    STD_ZEROMEM(&texture, sizeof(texture));
    apMipmaps[0] = Std3DSystemTest_CreateTextureVBufferFromBmp("uv_grid_directx.bmp");
    std3D_AllocSystemTexture(&texture, apMipmaps, STD_ARRAYLEN(apMipmaps), STDCOLOR_FORMAT_RGB);
    stdDisplay_VBufferFree(apMipmaps[0]);

    Std3DSystemTest_AssertSystemTextureAllocated(&texture);
    std3D_AddToTextureCache(&texture, STDCOLOR_FORMAT_RGB);
    if ( !texture.pCachedTexture )
    {
        std3D_ClearSystemTexture(&texture);
        TEST_IGNORE_MESSAGE("Direct3D texture cache upload unavailable on this host.");
    }

    std3D_UpdateFrameCount(&texture);

    Std3DSystemTest_InitTexturedVertex(&aVerts[0], 118.0f, 68.0f, 0.4f, 0.0f, 0.0f, 0xFFFFFFFFu);
    Std3DSystemTest_InitTexturedVertex(&aVerts[1], 202.0f, 68.0f, 0.4f, 1.0f, 0.0f, 0xFFFFFFFFu);
    Std3DSystemTest_InitTexturedVertex(&aVerts[2], 278.0f, 184.0f, 0.4f, 1.0f, 1.0f, 0xFFFFFFFFu);
    Std3DSystemTest_InitTexturedVertex(&aVerts[3], 42.0f, 184.0f, 0.4f, 0.0f, 1.0f, 0xFFFFFFFFu);
    aVerts[0].rhw = 0.35f;
    aVerts[1].rhw = 0.35f;
    aVerts[2].rhw = 1.0f;
    aVerts[3].rhw = 1.0f;

    Std3DSystemTest_DrawCachedTextureQuadVerts(
        texture.pCachedTexture,
        aVerts,
        STD3D_RS_TEXFILTER_ANISOTROPIC | STD3D_RS_TEX_CPAMP_U | STD3D_RS_TEX_CPAMP_V
    );
    std3D_ClearSystemTexture(&texture);
}

void Std3DSystemTest_RenderAnisotropicTextureFilterCubeScene(void)
{
    tSystemTexture texture;
    tVBuffer* apMipmaps[1];
    D3DTLVERTEX aVerts[12];

    STD_ZEROMEM(&texture, sizeof(texture));
    apMipmaps[0] = Std3DSystemTest_CreateTextureVBufferFromBmp("uv_grid_directx.bmp");
    std3D_AllocSystemTexture(&texture, apMipmaps, STD_ARRAYLEN(apMipmaps), STDCOLOR_FORMAT_RGB);
    stdDisplay_VBufferFree(apMipmaps[0]);

    Std3DSystemTest_AssertSystemTextureAllocated(&texture);
    std3D_AddToTextureCache(&texture, STDCOLOR_FORMAT_RGB);
    if ( !texture.pCachedTexture )
    {
        std3D_ClearSystemTexture(&texture);
        TEST_IGNORE_MESSAGE("Direct3D texture cache upload unavailable on this host.");
    }

    std3D_UpdateFrameCount(&texture);

    Std3DSystemTest_InitTexturedVertexRhw(&aVerts[0], 118.0f, 110.0f, 0.4f, 0.45f, 0.0f, 0.0f, 0xFFFFFFFFu);
    Std3DSystemTest_InitTexturedVertexRhw(&aVerts[1], 202.0f, 110.0f, 0.4f, 0.45f, 1.0f, 0.0f, 0xFFFFFFFFu);
    Std3DSystemTest_InitTexturedVertexRhw(&aVerts[2], 202.0f, 182.0f, 0.4f, 1.0f, 1.0f, 1.0f, 0xFFFFFFFFu);
    Std3DSystemTest_InitTexturedVertexRhw(&aVerts[3], 118.0f, 182.0f, 0.4f, 1.0f, 0.0f, 1.0f, 0xFFFFFFFFu);

    Std3DSystemTest_InitTexturedVertexRhw(&aVerts[4], 202.0f, 110.0f, 0.4f, 0.45f, 0.0f, 0.0f, 0xFFFFFFFFu);
    Std3DSystemTest_InitTexturedVertexRhw(&aVerts[5], 238.0f, 80.0f, 0.4f, 0.35f, 1.0f, 0.0f, 0xFFFFFFFFu);
    Std3DSystemTest_InitTexturedVertexRhw(&aVerts[6], 238.0f, 152.0f, 0.4f, 0.75f, 1.0f, 1.0f, 0xFFFFFFFFu);
    Std3DSystemTest_InitTexturedVertexRhw(&aVerts[7], 202.0f, 182.0f, 0.4f, 1.0f, 0.0f, 1.0f, 0xFFFFFFFFu);

    Std3DSystemTest_InitTexturedVertexRhw(&aVerts[8], 118.0f, 110.0f, 0.4f, 0.45f, 0.0f, 0.0f, 0xFFFFFFFFu);
    Std3DSystemTest_InitTexturedVertexRhw(&aVerts[9], 202.0f, 110.0f, 0.4f, 0.45f, 1.0f, 0.0f, 0xFFFFFFFFu);
    Std3DSystemTest_InitTexturedVertexRhw(&aVerts[10], 238.0f, 80.0f, 0.4f, 0.35f, 1.0f, 1.0f, 0xFFFFFFFFu);
    Std3DSystemTest_InitTexturedVertexRhw(&aVerts[11], 152.0f, 80.0f, 0.4f, 0.35f, 0.0f, 1.0f, 0xFFFFFFFFu);

    Std3DSystemTest_DrawFeatureCubeTexturedFaces(
        texture.pCachedTexture,
        aVerts,
        STD3D_RS_TEXFILTER_ANISOTROPIC | STD3D_RS_TEX_CPAMP_U | STD3D_RS_TEX_CPAMP_V
    );
    std3D_ClearSystemTexture(&texture);
}

void Std3DSystemTest_RenderMSAATriangleScene(void)
{
    D3DTLVERTEX aVerts[3];
    WORD aIndices[3] = { 0u, 1u, 2u };

    TEST_ASSERT_EQUAL_INT(0, stdDisplay_BackBufferFill(0u, NULL));
    std3D_EnableFog(0, 0.0f);
    std3D_SetRenderState(0u);
    std3D_ClearZBuffer();

    Std3DSystemTest_InitVertex(&aVerts[0], 70.35f, 56.20f, 0.4f, 0xFF00FF00u);
    Std3DSystemTest_InitVertex(&aVerts[1], 250.70f, 80.50f, 0.4f, 0xFF00FF00u);
    Std3DSystemTest_InitVertex(&aVerts[2], 104.20f, 190.30f, 0.4f, 0xFF00FF00u);

    TEST_ASSERT_EQUAL_INT(0, std3D_StartScene());
    std3D_DrawRenderList(NULL, 0u, aVerts, STD_ARRAYLEN(aVerts), aIndices, STD_ARRAYLEN(aIndices));
    Std3DSystemTest_EndScene();
}

void Std3DSystemTest_RenderMSAACubeVectorFrame(Std3DSystemTestCubeVectorMode mode, size_t frameNum)
{
    Std3DSystemTestScreenVertex aCubeVerts[8];
    bool abVisibleFaces[STD_ARRAYLEN(std3DSystemTest_aCubeFaces)];
    tSystemTexture texture;
    tSysTexture* pTexture = NULL;

    TEST_ASSERT_TRUE(frameNum < STD3D_SYSTEM_TEST_FEATURE_CUBE_FRAME_COUNT);
    TEST_ASSERT_TRUE(
        mode == STD3D_SYSTEM_TEST_CUBE_VECTOR_WIREFRAME
        || mode == STD3D_SYSTEM_TEST_CUBE_VECTOR_SOLID
        || mode == STD3D_SYSTEM_TEST_CUBE_VECTOR_TEXTURED
        || mode == STD3D_SYSTEM_TEST_CUBE_VECTOR_TEXTURED_VERTEX_COLOR
        || mode == STD3D_SYSTEM_TEST_CUBE_VECTOR_SOLID_VERTEX_INTENSITY
    );

    STD_ZEROMEM(&texture, sizeof(texture));
    if ( mode == STD3D_SYSTEM_TEST_CUBE_VECTOR_TEXTURED
        || mode == STD3D_SYSTEM_TEST_CUBE_VECTOR_TEXTURED_VERTEX_COLOR )
    {
        TEST_ASSERT_EQUAL_INT(0, std3D_SetMipmapFilter(STD3D_MIPMAPFILTER_NONE));
        pTexture = Std3DSystemTest_CreateCachedCubeVectorTexture(&texture);
    }

    Std3DSystemTest_ProjectFeatureCubeFrame(frameNum, aCubeVerts);
    Std3DSystemTest_CalculateCubeFaceVisibility(frameNum, STD3DSYSTEMTEST_CUBE_FRAME_DEGREES, abVisibleFaces);

    TEST_ASSERT_EQUAL_INT(0, stdDisplay_BackBufferFill(0u, NULL));
    std3D_EnableFog(0, 0.0f);
    std3D_SetRenderState(0u);
    std3D_ClearZBuffer();

    TEST_ASSERT_EQUAL_INT(0, std3D_StartScene());
    if ( mode == STD3D_SYSTEM_TEST_CUBE_VECTOR_WIREFRAME )
    {
        Std3DSystemTest_DrawCubeVectorWireframe(aCubeVerts);
    }
    else
    {
        Std3DSystemTest_DrawCubeVectorFaceFrame(mode, aCubeVerts, abVisibleFaces, pTexture);
    }
    Std3DSystemTest_EndScene();

    if ( pTexture )
    {
        std3D_ClearSystemTexture(&texture);
    }
}

void Std3DSystemTest_RenderMSAATexturedCubeFrame(size_t frameNum)
{
    Std3DSystemTest_RenderMSAACubeVectorFrame(STD3D_SYSTEM_TEST_CUBE_VECTOR_TEXTURED, frameNum);
}

void Std3DSystemTest_RenderMSAAWireframeCubeFrame(size_t frameNum)
{
    Std3DSystemTest_RenderMSAACubeVectorFrame(STD3D_SYSTEM_TEST_CUBE_VECTOR_WIREFRAME, frameNum);
}

void Std3DSystemTest_RenderFormattedTextureQuadScene(int formatType, uint8_t red, uint8_t green, uint8_t blue, uint8_t alpha)
{
    tVBuffer* apMipmaps[1];

    apMipmaps[0] = Std3DSystemTest_CreateSolidTextureVBufferWithFormat(4u, 4u, (StdColorFormatType)formatType, red, green, blue, alpha);
    Std3DSystemTest_RenderTextureQuadFromMipmaps(apMipmaps, STD_ARRAYLEN(apMipmaps), (StdColorFormatType)formatType, 1u);
}

void Std3DSystemTest_ExerciseTextureCacheResetAndReuse(void)
{
    tSystemTexture texture;
    tVBuffer* apMipmaps[1];

    STD_ZEROMEM(&texture, sizeof(texture));
    apMipmaps[0] = Std3DSystemTest_CreateSolidTextureVBuffer(4u, 4u, 255u, 128u, 0u);
    std3D_AllocSystemTexture(&texture, apMipmaps, STD_ARRAYLEN(apMipmaps), STDCOLOR_FORMAT_RGB);
    stdDisplay_VBufferFree(apMipmaps[0]);

    Std3DSystemTest_AssertSystemTextureAllocated(&texture);
    std3D_AddToTextureCache(&texture, STDCOLOR_FORMAT_RGB);
    if ( !texture.pCachedTexture )
    {
        std3D_ClearSystemTexture(&texture);
        TEST_IGNORE_MESSAGE("Direct3D texture cache upload unavailable on this host.");
    }

    std3D_UpdateFrameCount(&texture);
    std3D_ResetTextureCache();
    TEST_ASSERT_NULL(texture.pCachedTexture);

    std3D_AddToTextureCache(&texture, STDCOLOR_FORMAT_RGB);
    TEST_ASSERT_NOT_NULL(texture.pCachedTexture);
    std3D_UpdateFrameCount(&texture);
    Std3DSystemTest_DrawCachedTextureQuad(texture.pCachedTexture);
    std3D_ClearSystemTexture(&texture);
}

void Std3DSystemTest_RenderTexturedDepthOcclusionScene(void)
{
    tSystemTexture texture;
    tVBuffer* apMipmaps[1];
    D3DTLVERTEX aQuadVerts[4];
    D3DTLVERTEX aNearVerts[3];
    WORD aQuadIndices[6] = { 0u, 1u, 2u, 0u, 2u, 3u };
    WORD aNearIndices[3] = { 0u, 1u, 2u };

    STD_ZEROMEM(&texture, sizeof(texture));
    apMipmaps[0] = Std3DSystemTest_CreateSolidTextureVBuffer(4u, 4u, 255u, 255u, 0u);
    std3D_AllocSystemTexture(&texture, apMipmaps, STD_ARRAYLEN(apMipmaps), STDCOLOR_FORMAT_RGB);
    stdDisplay_VBufferFree(apMipmaps[0]);

    Std3DSystemTest_AssertSystemTextureAllocated(&texture);
    std3D_AddToTextureCache(&texture, STDCOLOR_FORMAT_RGB);
    if ( !texture.pCachedTexture )
    {
        std3D_ClearSystemTexture(&texture);
        TEST_IGNORE_MESSAGE("Direct3D texture cache upload unavailable on this host.");
    }

    std3D_UpdateFrameCount(&texture);

    TEST_ASSERT_EQUAL_INT(0, stdDisplay_BackBufferFill(0u, NULL));
    std3D_EnableFog(0, 0.0f);
    std3D_SetRenderState(0u);
    std3D_ClearZBuffer();

    Std3DSystemTest_InitTexturedVertex(&aQuadVerts[0], 96.0f, 72.0f, 0.7f, 0.0f, 0.0f, 0xFFFFFFFFu);
    Std3DSystemTest_InitTexturedVertex(&aQuadVerts[1], 224.0f, 72.0f, 0.7f, 1.0f, 0.0f, 0xFFFFFFFFu);
    Std3DSystemTest_InitTexturedVertex(&aQuadVerts[2], 224.0f, 168.0f, 0.7f, 1.0f, 1.0f, 0xFFFFFFFFu);
    Std3DSystemTest_InitTexturedVertex(&aQuadVerts[3], 96.0f, 168.0f, 0.7f, 0.0f, 1.0f, 0xFFFFFFFFu);

    Std3DSystemTest_InitVertex(&aNearVerts[0], 128.0f, 90.0f, 0.2f, 0xFF00FF00u);
    Std3DSystemTest_InitVertex(&aNearVerts[1], 192.0f, 90.0f, 0.2f, 0xFF00FF00u);
    Std3DSystemTest_InitVertex(&aNearVerts[2], 160.0f, 150.0f, 0.2f, 0xFF00FF00u);

    TEST_ASSERT_EQUAL_INT(0, std3D_StartScene());
    std3D_DrawRenderList(
        texture.pCachedTexture,
        STD3D_RS_TEXFILTER_BILINEAR | STD3D_RS_TEX_CPAMP_U | STD3D_RS_TEX_CPAMP_V,
        aQuadVerts,
        STD_ARRAYLEN(aQuadVerts),
        aQuadIndices,
        STD_ARRAYLEN(aQuadIndices)
    );
    std3D_DrawRenderList(NULL, 0u, aNearVerts, STD_ARRAYLEN(aNearVerts), aNearIndices, STD_ARRAYLEN(aNearIndices));
    Std3DSystemTest_EndScene();

    std3D_ClearSystemTexture(&texture);
}

void Std3DSystemTest_RenderZBufferOcclusionScene(void)
{
    D3DTLVERTEX aNearVerts[3];
    D3DTLVERTEX aFarVerts[3];
    WORD aIndices[3] = { 0u, 1u, 2u };

    TEST_ASSERT_EQUAL_INT(0, stdDisplay_BackBufferFill(0u, NULL));
    std3D_EnableFog(0, 0.0f);
    std3D_SetRenderState(0u);
    std3D_ClearZBuffer();

    Std3DSystemTest_InitVertex(&aNearVerts[0], 80.0f, 70.0f, 0.2f, 0xFF00FF00u);
    Std3DSystemTest_InitVertex(&aNearVerts[1], 240.0f, 70.0f, 0.2f, 0xFF00FF00u);
    Std3DSystemTest_InitVertex(&aNearVerts[2], 160.0f, 190.0f, 0.2f, 0xFF00FF00u);

    Std3DSystemTest_InitVertex(&aFarVerts[0], 80.0f, 70.0f, 0.8f, 0xFFFF0000u);
    Std3DSystemTest_InitVertex(&aFarVerts[1], 240.0f, 70.0f, 0.8f, 0xFFFF0000u);
    Std3DSystemTest_InitVertex(&aFarVerts[2], 160.0f, 190.0f, 0.8f, 0xFFFF0000u);

    TEST_ASSERT_EQUAL_INT(0, std3D_StartScene());
    std3D_DrawRenderList(NULL, 0u, aNearVerts, STD_ARRAYLEN(aNearVerts), aIndices, STD_ARRAYLEN(aIndices));
    std3D_DrawRenderList(NULL, 0u, aFarVerts, STD_ARRAYLEN(aFarVerts), aIndices, STD_ARRAYLEN(aIndices));
    Std3DSystemTest_EndScene();
}

void Std3DSystemTest_AssertValidDimensionsFollowDeviceCaps(void)
{
    static const uint32_t aWidths[]  = { 1u, 3u, 64u, 4096u, 8192u };
    static const uint32_t aHeights[] = { 2u, 5u, 32u, 2048u, 8192u };

    const Device3D* pDevice = std3D_GetCurrentDevice();
    TEST_ASSERT_NOT_NULL(pDevice);

    for ( size_t i = 0u; i < STD_ARRAYLEN(aWidths); ++i )
    {
        uint32_t actualWidth;
        uint32_t actualHeight;
        uint32_t expectedWidth  = Std3DSystemTest_ClampToDeviceLimit(aWidths[i], pDevice->minTexWidth, pDevice->maxTexWidth);
        uint32_t expectedHeight = Std3DSystemTest_ClampToDeviceLimit(aHeights[i], pDevice->minTexHeight, pDevice->maxTexHeight);

        if ( pDevice->bSqareOnlyTexture && expectedWidth != expectedHeight )
        {
            uint32_t squareSize = expectedWidth > expectedHeight ? expectedWidth : expectedHeight;

            expectedWidth  = squareSize;
            expectedHeight = squareSize;
        }

        std3D_GetValidDimensions(aWidths[i], aHeights[i], &actualWidth, &actualHeight);
        TEST_ASSERT_EQUAL_UINT32(expectedWidth, actualWidth);
        TEST_ASSERT_EQUAL_UINT32(expectedHeight, actualHeight);
    }
}

void Std3DSystemTest_ExerciseRenderStatePermutations(void)
{
    std3D_EnableFog(1, 1.0f);
    std3D_SetFog(0.25f, 0.5f, 0.75f, 1.0f, 100.0f);

    std3D_SetRenderState(STD3D_RS_TEXFILTER_BILINEAR | STD3D_RS_TEX_CPAMP_U | STD3D_RS_TEX_CPAMP_V);
    std3D_SetRenderState(STD3D_RS_TEXFILTER_BILINEAR | STD3D_RS_ALPHAREF_SET);
    std3D_SetRenderState(STD3D_RS_ZWRITE_DISABLED | STD3D_RS_FOG_ENABLED);
    std3D_SetRenderState(STD3D_RS_TEXFILTER_ANISOTROPIC | STD3D_RS_TEX_CPAMP_U | STD3D_RS_TEX_CPAMP_V);
    std3D_SetWireframeRenderState();
    std3D_SetRenderState(0u);
    std3D_EnableFog(0, 0.0f);
}

static void Std3DSystemTest_ReadBackBufferPixelFromSurface(const uint8_t* pSurface, uint32_t surfaceHeight, int32_t pitch, uint32_t x, uint32_t y, const ColorInfo* pColorInfo, uint8_t* pRed, uint8_t* pGreen, uint8_t* pBlue)
{
    uint32_t encoded     = 0u;
    size_t bytesPerPixel = pColorInfo->bpp / 8u;
    const uint8_t* pRow = pitch >= 0
        ? pSurface + (size_t)y * (size_t)pitch
        : pSurface + (size_t)(surfaceHeight - 1u - y) * (size_t)-pitch;
    const uint8_t* pPixel = pRow + (size_t)x * bytesPerPixel;

    TEST_ASSERT_TRUE(bytesPerPixel > 0u && bytesPerPixel <= sizeof(encoded));
    STD_COPYMEM(&encoded, pPixel, bytesPerPixel);
    stdColor_DecodeRGB(encoded, pColorInfo, pRed, pGreen, pBlue);
}

static int Std3DSystemTest_AbsDiff(uint8_t a, uint8_t b)
{
    return a > b ? (int)(a - b) : (int)(b - a);
}

static void Std3DSystemTest_AssertRGBNear(uint32_t x, uint32_t y, uint8_t expectedRed, uint8_t expectedGreen, uint8_t expectedBlue, uint8_t actualRed, uint8_t actualGreen, uint8_t actualBlue, uint8_t tolerance)
{
    if ( Std3DSystemTest_AbsDiff(actualRed, expectedRed) > tolerance
        || Std3DSystemTest_AbsDiff(actualGreen, expectedGreen) > tolerance
        || Std3DSystemTest_AbsDiff(actualBlue, expectedBlue) > tolerance )
    {
        char aMessage[256];
        STD_FORMAT(
            aMessage,
            "Back buffer pixel mismatch at (%u,%u), expected RGB(%u,%u,%u), got RGB(%u,%u,%u).",
            x,
            y,
            expectedRed,
            expectedGreen,
            expectedBlue,
            actualRed,
            actualGreen,
            actualBlue
        );
        TEST_FAIL_MESSAGE(aMessage);
    }
}

static bool Std3DSystemTest_IsRGBNear(uint8_t expectedRed, uint8_t expectedGreen, uint8_t expectedBlue, uint8_t actualRed, uint8_t actualGreen, uint8_t actualBlue, uint8_t tolerance)
{
    return Std3DSystemTest_AbsDiff(actualRed, expectedRed) <= tolerance
        && Std3DSystemTest_AbsDiff(actualGreen, expectedGreen) <= tolerance
        && Std3DSystemTest_AbsDiff(actualBlue, expectedBlue) <= tolerance;
}

void Std3DSystemTest_ReadBackBufferPixel(uint32_t x, uint32_t y, uint8_t* pRed, uint8_t* pGreen, uint8_t* pBlue)
{
    StdVideoMode mode;
    void* pSurface;
    uint32_t width;
    uint32_t height;
    int32_t pitch;

    TEST_ASSERT_NOT_NULL(pRed);
    TEST_ASSERT_NOT_NULL(pGreen);
    TEST_ASSERT_NOT_NULL(pBlue);
    TEST_ASSERT_EQUAL_INT(0, stdDisplay_GetCurrentVideoMode(&mode));
    TEST_ASSERT_EQUAL_INT(0, stdDisplay_LockBackBufferReadOnly(&pSurface, &width, &height, &pitch));
    TEST_ASSERT_TRUE(x < width);
    TEST_ASSERT_TRUE(y < height);

    Std3DSystemTest_ReadBackBufferPixelFromSurface((const uint8_t*)pSurface, height, pitch, x, y, &mode.rasterInfo.colorInfo, pRed, pGreen, pBlue);
    stdDisplay_UnlockBackBuffer();
}

void Std3DSystemTest_AssertBackBufferPixelNear(uint32_t x, uint32_t y, uint8_t red, uint8_t green, uint8_t blue, uint8_t tolerance)
{
    uint8_t actualRed;
    uint8_t actualGreen;
    uint8_t actualBlue;

    Std3DSystemTest_ReadBackBufferPixel(x, y, &actualRed, &actualGreen, &actualBlue);

    Std3DSystemTest_AssertRGBNear(x, y, red, green, blue, actualRed, actualGreen, actualBlue, tolerance);
}

void Std3DSystemTest_AssertBackBufferPixelInRange(uint32_t x, uint32_t y, uint8_t minRed, uint8_t maxRed, uint8_t minGreen, uint8_t maxGreen, uint8_t minBlue, uint8_t maxBlue)
{
    StdVideoMode mode;
    void* pSurface;
    uint32_t width;
    uint32_t height;
    int32_t pitch;
    uint8_t actualRed;
    uint8_t actualGreen;
    uint8_t actualBlue;

    TEST_ASSERT_EQUAL_INT(0, stdDisplay_GetCurrentVideoMode(&mode));
    TEST_ASSERT_EQUAL_INT(0, stdDisplay_LockBackBufferReadOnly(&pSurface, &width, &height, &pitch));
    TEST_ASSERT_TRUE(x < width);
    TEST_ASSERT_TRUE(y < height);

    Std3DSystemTest_ReadBackBufferPixelFromSurface((const uint8_t*)pSurface, height, pitch, x, y, &mode.rasterInfo.colorInfo, &actualRed, &actualGreen, &actualBlue);
    stdDisplay_UnlockBackBuffer();

    if ( actualRed < minRed || actualRed > maxRed
        || actualGreen < minGreen || actualGreen > maxGreen
        || actualBlue < minBlue || actualBlue > maxBlue )
    {
        char aMessage[256];
        STD_FORMAT(
            aMessage,
            "Back buffer pixel range mismatch at (%u,%u), expected R[%u,%u] G[%u,%u] B[%u,%u], got RGB(%u,%u,%u).",
            x,
            y,
            minRed,
            maxRed,
            minGreen,
            maxGreen,
            minBlue,
            maxBlue,
            actualRed,
            actualGreen,
            actualBlue
        );
        TEST_FAIL_MESSAGE(aMessage);
    }
}

void Std3DSystemTest_AssertBackBufferAnyPixelNear(uint32_t left, uint32_t top, uint32_t width, uint32_t height, uint8_t red, uint8_t green, uint8_t blue, uint8_t tolerance)
{
    StdVideoMode mode;
    void* pSurface;
    uint32_t surfaceWidth;
    uint32_t surfaceHeight;
    int32_t pitch;
    bool bFound = false;

    TEST_ASSERT_EQUAL_INT(0, stdDisplay_GetCurrentVideoMode(&mode));
    TEST_ASSERT_EQUAL_INT(0, stdDisplay_LockBackBufferReadOnly(&pSurface, &surfaceWidth, &surfaceHeight, &pitch));
    TEST_ASSERT_TRUE(left + width <= surfaceWidth);
    TEST_ASSERT_TRUE(top + height <= surfaceHeight);

    for ( uint32_t y = top; y < top + height && !bFound; ++y )
    {
        for ( uint32_t x = left; x < left + width; ++x )
        {
            uint8_t actualRed;
            uint8_t actualGreen;
            uint8_t actualBlue;

            Std3DSystemTest_ReadBackBufferPixelFromSurface((const uint8_t*)pSurface, surfaceHeight, pitch, x, y, &mode.rasterInfo.colorInfo, &actualRed, &actualGreen, &actualBlue);
            if ( Std3DSystemTest_AbsDiff(actualRed, red) <= tolerance
                && Std3DSystemTest_AbsDiff(actualGreen, green) <= tolerance
                && Std3DSystemTest_AbsDiff(actualBlue, blue) <= tolerance )
            {
                bFound = true;
                break;
            }
        }
    }

    stdDisplay_UnlockBackBuffer();

    if ( !bFound )
    {
        char aMessage[256];
        STD_FORMAT(
            aMessage,
            "Back buffer area (%u,%u,%u,%u) did not contain RGB(%u,%u,%u).",
            left,
            top,
            width,
            height,
            red,
            green,
            blue
        );
        TEST_FAIL_MESSAGE(aMessage);
    }
}

void Std3DSystemTest_AssertBackBufferAnyPixelInRange(uint32_t left, uint32_t top, uint32_t width, uint32_t height, uint8_t minRed, uint8_t maxRed, uint8_t minGreen, uint8_t maxGreen, uint8_t minBlue, uint8_t maxBlue)
{
    StdVideoMode mode;
    void* pSurface;
    uint32_t surfaceWidth;
    uint32_t surfaceHeight;
    int32_t pitch;
    bool bFound = false;

    TEST_ASSERT_EQUAL_INT(0, stdDisplay_GetCurrentVideoMode(&mode));
    TEST_ASSERT_EQUAL_INT(0, stdDisplay_LockBackBufferReadOnly(&pSurface, &surfaceWidth, &surfaceHeight, &pitch));
    TEST_ASSERT_TRUE(left + width <= surfaceWidth);
    TEST_ASSERT_TRUE(top + height <= surfaceHeight);

    for ( uint32_t y = top; y < top + height && !bFound; ++y )
    {
        for ( uint32_t x = left; x < left + width; ++x )
        {
            uint8_t actualRed;
            uint8_t actualGreen;
            uint8_t actualBlue;

            Std3DSystemTest_ReadBackBufferPixelFromSurface((const uint8_t*)pSurface, surfaceHeight, pitch, x, y, &mode.rasterInfo.colorInfo, &actualRed, &actualGreen, &actualBlue);
            if ( actualRed >= minRed && actualRed <= maxRed
                && actualGreen >= minGreen && actualGreen <= maxGreen
                && actualBlue >= minBlue && actualBlue <= maxBlue )
            {
                bFound = true;
                break;
            }
        }
    }

    stdDisplay_UnlockBackBuffer();

    if ( !bFound )
    {
        char aMessage[256];
        STD_FORMAT(
            aMessage,
            "Back buffer area (%u,%u,%u,%u) did not contain any pixel in R[%u,%u] G[%u,%u] B[%u,%u].",
            left,
            top,
            width,
            height,
            minRed,
            maxRed,
            minGreen,
            maxGreen,
            minBlue,
            maxBlue
        );
        TEST_FAIL_MESSAGE(aMessage);
    }
}

void Std3DSystemTest_AssertBackBufferRectNear(uint32_t left, uint32_t top, uint32_t width, uint32_t height, uint8_t red, uint8_t green, uint8_t blue, uint8_t tolerance)
{
    StdVideoMode mode;
    void* pSurface;
    uint32_t surfaceWidth;
    uint32_t surfaceHeight;
    int32_t pitch;

    TEST_ASSERT_EQUAL_INT(0, stdDisplay_GetCurrentVideoMode(&mode));
    TEST_ASSERT_EQUAL_INT(0, stdDisplay_LockBackBufferReadOnly(&pSurface, &surfaceWidth, &surfaceHeight, &pitch));
    TEST_ASSERT_TRUE(left + width <= surfaceWidth);
    TEST_ASSERT_TRUE(top + height <= surfaceHeight);

    for ( uint32_t y = top; y < top + height; ++y )
    {
        for ( uint32_t x = left; x < left + width; ++x )
        {
            uint8_t actualRed;
            uint8_t actualGreen;
            uint8_t actualBlue;

            Std3DSystemTest_ReadBackBufferPixelFromSurface((const uint8_t*)pSurface, surfaceHeight, pitch, x, y, &mode.rasterInfo.colorInfo, &actualRed, &actualGreen, &actualBlue);
            Std3DSystemTest_AssertRGBNear(x, y, red, green, blue, actualRed, actualGreen, actualBlue, tolerance);
        }
    }

    stdDisplay_UnlockBackBuffer();
}

void Std3DSystemTest_AssertBackBufferMatchesBmpMask(const char* pVectorName, uint8_t tolerance)
{
    Std3DSystemTest_AssertBackBufferMatchesBmpMaskAtResolution(
        pVectorName,
        tolerance,
        STD3DSYSTEMTEST_VECTOR_WIDTH,
        STD3DSYSTEMTEST_VECTOR_HEIGHT
    );
}

void Std3DSystemTest_AssertBackBufferMatchesBmpMaskAtResolution(const char* pVectorName, uint8_t tolerance, uint32_t expectedWidth, uint32_t expectedHeight)
{
    Std3DSystemTestBmp bmp;
    StdVideoMode mode;
    void* pSurface;
    uint32_t width;
    uint32_t height;
    int32_t pitch;
    size_t numCompared = 0u;
    size_t numFailed   = 0u;
    uint32_t firstFailX = 0u;
    uint32_t firstFailY = 0u;
    uint8_t firstExpected[3] = { 0u, 0u, 0u };
    uint8_t firstActual[3] = { 0u, 0u, 0u };

    Std3DSystemTest_UpdateBmpMask(pVectorName);
    Std3DSystemTest_LoadBmpMask(pVectorName, &bmp, expectedWidth, expectedHeight);

    TEST_ASSERT_EQUAL_INT(0, stdDisplay_GetCurrentVideoMode(&mode));
    TEST_ASSERT_EQUAL_INT(0, stdDisplay_LockBackBufferReadOnly(&pSurface, &width, &height, &pitch));
    TEST_ASSERT_EQUAL_UINT32(bmp.width, width);
    TEST_ASSERT_EQUAL_UINT32(bmp.height, height);

    for ( uint32_t y = 0u; y < bmp.height; ++y )
    {
        for ( uint32_t x = 0u; x < bmp.width; ++x )
        {
            const uint8_t* pExpected = &bmp.pPixels[((size_t)y * bmp.width + x) * 3u];
            uint8_t actualRed;
            uint8_t actualGreen;
            uint8_t actualBlue;

            if ( pExpected[0] == STD3DSYSTEMTEST_MASK_IGNORE_R
                && pExpected[1] == STD3DSYSTEMTEST_MASK_IGNORE_G
                && pExpected[2] == STD3DSYSTEMTEST_MASK_IGNORE_B )
            {
                continue;
            }

            Std3DSystemTest_ReadBackBufferPixelFromSurface((const uint8_t*)pSurface, height, pitch, x, y, &mode.rasterInfo.colorInfo, &actualRed, &actualGreen, &actualBlue);
            ++numCompared;

            if ( Std3DSystemTest_AbsDiff(actualRed, pExpected[0]) > tolerance
                || Std3DSystemTest_AbsDiff(actualGreen, pExpected[1]) > tolerance
                || Std3DSystemTest_AbsDiff(actualBlue, pExpected[2]) > tolerance )
            {
                if ( !numFailed )
                {
                    firstFailX = x;
                    firstFailY = y;
                    firstExpected[0] = pExpected[0];
                    firstExpected[1] = pExpected[1];
                    firstExpected[2] = pExpected[2];
                    firstActual[0] = actualRed;
                    firstActual[1] = actualGreen;
                    firstActual[2] = actualBlue;
                }

                ++numFailed;
            }
        }
    }

    stdDisplay_UnlockBackBuffer();
    Std3DSystemTest_FreeBmp(&bmp);

    TEST_ASSERT_TRUE_MESSAGE(numCompared > 0u, "std3D BMP mask did not contain any comparable pixels.");
    if ( numFailed )
    {
        char aMessage[256];

        STD_FORMAT(
            aMessage,
            "std3D BMP mask '%s' mismatch: %zu failed pixels. First at (%u,%u), expected RGB(%u,%u,%u), got RGB(%u,%u,%u).",
            pVectorName,
            numFailed,
            firstFailX,
            firstFailY,
            firstExpected[0],
            firstExpected[1],
            firstExpected[2],
            firstActual[0],
            firstActual[1],
            firstActual[2]
        );
        TEST_FAIL_MESSAGE(aMessage);
    }
}

void Std3DSystemTest_AssertBackBufferMatchesBmpProbeMask(const char* pVectorName, uint8_t tolerance, uint32_t searchRadius)
{
    Std3DSystemTestBmp bmp;
    StdVideoMode mode;
    void* pSurface;
    uint32_t width;
    uint32_t height;
    int32_t pitch;
    size_t numCompared = 0u;
    size_t numFailed   = 0u;
    uint32_t firstFailX = 0u;
    uint32_t firstFailY = 0u;
    uint8_t firstExpected[3] = { 0u, 0u, 0u };

    Std3DSystemTest_UpdateBmpMask(pVectorName);
    Std3DSystemTest_LoadBmpMask(pVectorName, &bmp, STD3DSYSTEMTEST_VECTOR_WIDTH, STD3DSYSTEMTEST_VECTOR_HEIGHT);

    TEST_ASSERT_EQUAL_INT(0, stdDisplay_GetCurrentVideoMode(&mode));
    TEST_ASSERT_EQUAL_INT(0, stdDisplay_LockBackBufferReadOnly(&pSurface, &width, &height, &pitch));
    TEST_ASSERT_EQUAL_UINT32(bmp.width, width);
    TEST_ASSERT_EQUAL_UINT32(bmp.height, height);

    for ( uint32_t y = 0u; y < bmp.height; ++y )
    {
        for ( uint32_t x = 0u; x < bmp.width; ++x )
        {
            const uint8_t* pExpected = &bmp.pPixels[((size_t)y * bmp.width + x) * 3u];
            bool bFound = false;

            if ( pExpected[0] == STD3DSYSTEMTEST_MASK_IGNORE_R
                && pExpected[1] == STD3DSYSTEMTEST_MASK_IGNORE_G
                && pExpected[2] == STD3DSYSTEMTEST_MASK_IGNORE_B )
            {
                continue;
            }

            uint32_t minX = x > searchRadius ? x - searchRadius : 0u;
            uint32_t minY = y > searchRadius ? y - searchRadius : 0u;
            uint32_t maxX = x + searchRadius < width ? x + searchRadius : width - 1u;
            uint32_t maxY = y + searchRadius < height ? y + searchRadius : height - 1u;

            ++numCompared;
            for ( uint32_t sampleY = minY; sampleY <= maxY && !bFound; ++sampleY )
            {
                for ( uint32_t sampleX = minX; sampleX <= maxX; ++sampleX )
                {
                    uint8_t actualRed;
                    uint8_t actualGreen;
                    uint8_t actualBlue;

                    Std3DSystemTest_ReadBackBufferPixelFromSurface((const uint8_t*)pSurface, height, pitch, sampleX, sampleY, &mode.rasterInfo.colorInfo, &actualRed, &actualGreen, &actualBlue);
                    if ( Std3DSystemTest_IsRGBNear(pExpected[0], pExpected[1], pExpected[2], actualRed, actualGreen, actualBlue, tolerance) )
                    {
                        bFound = true;
                        break;
                    }
                }
            }

            if ( !bFound )
            {
                if ( !numFailed )
                {
                    firstFailX = x;
                    firstFailY = y;
                    firstExpected[0] = pExpected[0];
                    firstExpected[1] = pExpected[1];
                    firstExpected[2] = pExpected[2];
                }

                ++numFailed;
            }
        }
    }

    stdDisplay_UnlockBackBuffer();
    Std3DSystemTest_FreeBmp(&bmp);

    TEST_ASSERT_TRUE_MESSAGE(numCompared > 0u, "std3D BMP probe mask did not contain any comparable pixels.");
    if ( numFailed )
    {
        char aMessage[256];
        STD_FORMAT(
            aMessage,
            "std3D BMP probe mask '%s' mismatch: %zu failed probes. First at (%u,%u), expected nearby RGB(%u,%u,%u).",
            pVectorName,
            numFailed,
            firstFailX,
            firstFailY,
            firstExpected[0],
            firstExpected[1],
            firstExpected[2]
        );
        TEST_FAIL_MESSAGE(aMessage);
    }
}

static uint32_t Std3DSystemTest_EncodeBackBufferColor(uint8_t red, uint8_t green, uint8_t blue)
{
    StdVideoMode mode;

    STD_ZEROMEM(&mode, sizeof(mode));
    TEST_ASSERT_EQUAL_INT(0, stdDisplay_GetCurrentVideoMode(&mode));
    return stdColor_EncodeRGB(&mode.rasterInfo.colorInfo, red, green, blue);
}

static tSysTexture* Std3DSystemTest_CacheTextureVBuffer(tSystemTexture* pTexture, tVBuffer* pVBuffer, StdColorFormatType formatType)
{
    tVBuffer* apMipmaps[1] = { pVBuffer };

    STD_ZEROMEM(pTexture, sizeof(*pTexture));
    std3D_AllocSystemTexture(pTexture, apMipmaps, STD_ARRAYLEN(apMipmaps), formatType);
    stdDisplay_VBufferFree(pVBuffer);
    Std3DSystemTest_AssertSystemTextureAllocated(pTexture);

    std3D_AddToTextureCache(pTexture, formatType);
    if ( !pTexture->pCachedTexture )
    {
        std3D_ClearSystemTexture(pTexture);
        TEST_IGNORE_MESSAGE("Direct3D texture cache upload unavailable on this host.");
    }

    std3D_UpdateFrameCount(pTexture);
    return pTexture->pCachedTexture;
}

static tVBuffer* Std3DSystemTest_CreateQuadrantTextureVBuffer(uint32_t width, uint32_t height)
{
    ColorInfo colorInfo;
    int bColorKeySet = 0;
    LPDDCOLORKEY pColorKey = NULL;
    tRasterInfo rasterInfo;

    STD_ZEROMEM(&colorInfo, sizeof(colorInfo));
    std3D_GetTextureFormat(STDCOLOR_FORMAT_RGB, &colorInfo, &bColorKeySet, &pColorKey);
    TEST_ASSERT_TRUE(colorInfo.bpp >= 16u);

    STD_ZEROMEM(&rasterInfo, sizeof(rasterInfo));
    rasterInfo.width     = width;
    rasterInfo.height    = height;
    rasterInfo.colorInfo = colorInfo;

    tVBuffer* pVBuffer = stdDisplay_VBufferNew(&rasterInfo, 0, 0);
    TEST_ASSERT_NOT_NULL(pVBuffer);
    TEST_ASSERT_TRUE(stdDisplay_VBufferLock(pVBuffer));

    for ( uint32_t y = 0u; y < height; ++y )
    {
        for ( uint32_t x = 0u; x < width; ++x )
        {
            bool bRight  = x >= width / 2u;
            bool bBottom = y >= height / 2u;
            uint8_t red  = bBottom ? (bRight ? 255u : 0u) : (bRight ? 0u : 255u);
            uint8_t green = bRight ? 255u : 0u;
            uint8_t blue  = bBottom && !bRight ? 255u : 0u;

            Std3DSystemTest_WriteVBufferPixel(pVBuffer, x, y, stdColor_EncodeRGB(&colorInfo, red, green, blue));
        }
    }

    TEST_ASSERT_EQUAL_INT(1, stdDisplay_VBufferUnlock(pVBuffer));
    return pVBuffer;
}

static tVBuffer* Std3DSystemTest_CreateSplitAlphaTextureVBuffer(StdColorFormatType formatType, uint8_t leftAlpha, uint8_t rightAlpha)
{
    ColorInfo colorInfo;
    int bColorKeySet = 0;
    LPDDCOLORKEY pColorKey = NULL;
    tRasterInfo rasterInfo;

    STD_ZEROMEM(&colorInfo, sizeof(colorInfo));
    std3D_GetTextureFormat(formatType, &colorInfo, &bColorKeySet, &pColorKey);
    TEST_ASSERT_TRUE(colorInfo.alphaBPP > 0u);

    STD_ZEROMEM(&rasterInfo, sizeof(rasterInfo));
    rasterInfo.width     = 4u;
    rasterInfo.height    = 2u;
    rasterInfo.colorInfo = colorInfo;

    tVBuffer* pVBuffer = stdDisplay_VBufferNew(&rasterInfo, 0, 0);
    TEST_ASSERT_NOT_NULL(pVBuffer);
    TEST_ASSERT_TRUE(stdDisplay_VBufferLock(pVBuffer));

    for ( uint32_t y = 0u; y < rasterInfo.height; ++y )
    {
        for ( uint32_t x = 0u; x < rasterInfo.width; ++x )
        {
            uint8_t alpha = x < rasterInfo.width / 2u ? leftAlpha : rightAlpha;
            uint32_t pixel = stdColor_EncodeRGBA(&colorInfo, 255u, 0u, 255u, alpha);
            Std3DSystemTest_WriteVBufferPixel(pVBuffer, x, y, pixel);
        }
    }

    TEST_ASSERT_EQUAL_INT(1, stdDisplay_VBufferUnlock(pVBuffer));
    return pVBuffer;
}

static void Std3DSystemTest_RenderTextureQuad(tSysTexture* pTexture, uint32_t backgroundColor, uint32_t vertexColor, float tu, float tv, Std3DRenderState rdflags)
{
    D3DTLVERTEX aVerts[4];
    WORD aIndices[6] = { 0u, 1u, 2u, 0u, 2u, 3u };

    TEST_ASSERT_EQUAL_INT(0, stdDisplay_BackBufferFill(backgroundColor, NULL));
    std3D_EnableFog(0, 0.0f);
    std3D_SetRenderState(0u);
    std3D_ClearZBuffer();

    Std3DSystemTest_InitTexturedVertex(&aVerts[0], 96.0f, 72.0f, 0.4f, tu, tv, vertexColor);
    Std3DSystemTest_InitTexturedVertex(&aVerts[1], 224.0f, 72.0f, 0.4f, tu, tv, vertexColor);
    Std3DSystemTest_InitTexturedVertex(&aVerts[2], 224.0f, 168.0f, 0.4f, tu, tv, vertexColor);
    Std3DSystemTest_InitTexturedVertex(&aVerts[3], 96.0f, 168.0f, 0.4f, tu, tv, vertexColor);

    TEST_ASSERT_EQUAL_INT(0, std3D_StartScene());
    std3D_DrawRenderList(pTexture, rdflags, aVerts, STD_ARRAYLEN(aVerts), aIndices, STD_ARRAYLEN(aIndices));
    Std3DSystemTest_EndScene();
}

static void Std3DSystemTest_RenderTextureQuadWithUvRange(tSysTexture* pTexture, uint32_t backgroundColor, float minU, float minV, float maxU, float maxV, Std3DRenderState rdflags)
{
    D3DTLVERTEX aVerts[4];
    WORD aIndices[6] = { 0u, 1u, 2u, 0u, 2u, 3u };

    TEST_ASSERT_EQUAL_INT(0, stdDisplay_BackBufferFill(backgroundColor, NULL));
    std3D_EnableFog(0, 0.0f);
    std3D_SetRenderState(0u);
    std3D_ClearZBuffer();

    Std3DSystemTest_InitTexturedVertex(&aVerts[0], 80.0f, 60.0f, 0.4f, minU, minV, 0xFFFFFFFFu);
    Std3DSystemTest_InitTexturedVertex(&aVerts[1], 240.0f, 60.0f, 0.4f, maxU, minV, 0xFFFFFFFFu);
    Std3DSystemTest_InitTexturedVertex(&aVerts[2], 240.0f, 180.0f, 0.4f, maxU, maxV, 0xFFFFFFFFu);
    Std3DSystemTest_InitTexturedVertex(&aVerts[3], 80.0f, 180.0f, 0.4f, minU, maxV, 0xFFFFFFFFu);

    TEST_ASSERT_EQUAL_INT(0, std3D_StartScene());
    std3D_DrawRenderList(pTexture, rdflags, aVerts, STD_ARRAYLEN(aVerts), aIndices, STD_ARRAYLEN(aIndices));
    Std3DSystemTest_EndScene();
}

void Std3DSystemTest_ExerciseAlphaAndTransparency(void)
{
    const Device3D* pDevice = std3D_GetCurrentDevice();
    TEST_ASSERT_NOT_NULL(pDevice);
    if ( !pDevice->bAlphaTextureSupported || !pDevice->bAlphaBlendSupported )
    {
        TEST_IGNORE_MESSAGE("Texture alpha blending is unavailable on this Direct3D device.");
    }

    uint32_t blueBackground = Std3DSystemTest_EncodeBackBufferColor(0u, 0u, 255u);
    uint32_t greenBackground = Std3DSystemTest_EncodeBackBufferColor(0u, 255u, 0u);
    Std3DRenderState clampState = STD3D_RS_TEX_CPAMP_U | STD3D_RS_TEX_CPAMP_V;

    tSystemTexture texture;
    tVBuffer* pVBuffer = Std3DSystemTest_CreateSolidTextureVBufferWithFormat(4u, 4u, STDCOLOR_FORMAT_RGBA, 255u, 0u, 0u, 128u);
    tSysTexture* pCachedTexture = Std3DSystemTest_CacheTextureVBuffer(&texture, pVBuffer, STDCOLOR_FORMAT_RGBA);
    Std3DSystemTest_RenderTextureQuad(pCachedTexture, blueBackground, 0xFFFFFFFFu, 0.5f, 0.5f, clampState);
    Std3DSystemTest_AssertBackBufferPixelNear(160u, 120u, 128u, 0u, 127u, 40u);
    std3D_ClearSystemTexture(&texture);

    pVBuffer = Std3DSystemTest_CreateSolidTextureVBuffer(4u, 4u, 255u, 255u, 255u);
    pCachedTexture = Std3DSystemTest_CacheTextureVBuffer(&texture, pVBuffer, STDCOLOR_FORMAT_RGB);
    Std3DSystemTest_RenderTextureQuad(pCachedTexture, greenBackground, 0x80FF0000u, 0.5f, 0.5f, clampState);
    Std3DSystemTest_AssertBackBufferPixelNear(160u, 120u, 128u, 127u, 0u, 40u);
    std3D_ClearSystemTexture(&texture);

    pVBuffer = Std3DSystemTest_CreateSplitAlphaTextureVBuffer(STDCOLOR_FORMAT_RGBA_1BITALPHA, 0u, 255u);
    pCachedTexture = Std3DSystemTest_CacheTextureVBuffer(&texture, pVBuffer, STDCOLOR_FORMAT_RGBA_1BITALPHA);
    Std3DSystemTest_RenderTextureQuadWithUvRange(pCachedTexture, greenBackground, 0.0f, 0.0f, 1.0f, 1.0f, clampState);
    Std3DSystemTest_AssertBackBufferPixelNear(120u, 120u, 0u, 255u, 0u, 32u);
    Std3DSystemTest_AssertBackBufferPixelNear(200u, 120u, 255u, 0u, 255u, 32u);
    std3D_ClearSystemTexture(&texture);

    pVBuffer = Std3DSystemTest_CreateSplitAlphaTextureVBuffer(STDCOLOR_FORMAT_RGBA, 128u, 255u);
    pCachedTexture = Std3DSystemTest_CacheTextureVBuffer(&texture, pVBuffer, STDCOLOR_FORMAT_RGBA);
    Std3DSystemTest_RenderTextureQuadWithUvRange(pCachedTexture, blueBackground, 0.0f, 0.0f, 1.0f, 1.0f, clampState | STD3D_RS_ALPHAREF_SET);
    Std3DSystemTest_AssertBackBufferPixelNear(120u, 120u, 0u, 0u, 255u, 32u);
    Std3DSystemTest_AssertBackBufferPixelNear(200u, 120u, 255u, 0u, 255u, 32u);
    std3D_ClearSystemTexture(&texture);
}

void Std3DSystemTest_ExerciseTextureAddressFilteringAndStateIsolation(void)
{
    tSystemTexture texture;
    tSysTexture* pTexture = Std3DSystemTest_CacheTextureVBuffer(&texture, Std3DSystemTest_CreateQuadrantTextureVBuffer(2u, 2u), STDCOLOR_FORMAT_RGB);
    uint32_t black = Std3DSystemTest_EncodeBackBufferColor(0u, 0u, 0u);

    Std3DSystemTest_RenderTextureQuad(pTexture, black, 0xFFFFFFFFu, 1.25f, 0.25f, 0u);
    Std3DSystemTest_AssertBackBufferPixelNear(160u, 120u, 255u, 0u, 0u, 24u);

    Std3DSystemTest_RenderTextureQuad(pTexture, black, 0xFFFFFFFFu, 1.25f, 0.25f, STD3D_RS_TEX_CPAMP_U);
    Std3DSystemTest_AssertBackBufferPixelNear(160u, 120u, 0u, 255u, 0u, 24u);

    Std3DSystemTest_RenderTextureQuad(pTexture, black, 0xFFFFFFFFu, 0.25f, 1.25f, 0u);
    Std3DSystemTest_AssertBackBufferPixelNear(160u, 120u, 255u, 0u, 0u, 24u);

    Std3DSystemTest_RenderTextureQuad(pTexture, black, 0xFFFFFFFFu, 0.25f, 1.25f, STD3D_RS_TEX_CPAMP_V);
    Std3DSystemTest_AssertBackBufferPixelNear(160u, 120u, 0u, 0u, 255u, 24u);

    Std3DSystemTest_RenderTextureQuad(
        pTexture,
        black,
        0xFFFFFFFFu,
        0.5f,
        0.5f,
        STD3D_RS_TEXFILTER_BILINEAR | STD3D_RS_TEX_CPAMP_U | STD3D_RS_TEX_CPAMP_V
    );
    Std3DSystemTest_AssertBackBufferPixelNear(160u, 120u, 128u, 128u, 64u, 48u);
    std3D_ClearSystemTexture(&texture);

    D3DTLVERTEX aNearVerts[3];
    D3DTLVERTEX aFarVerts[3];
    WORD aIndices[3] = { 0u, 1u, 2u };

    Std3DSystemTest_InitVertex(&aNearVerts[0], 80.0f, 70.0f, 0.2f, 0xFF00FF00u);
    Std3DSystemTest_InitVertex(&aNearVerts[1], 240.0f, 70.0f, 0.2f, 0xFF00FF00u);
    Std3DSystemTest_InitVertex(&aNearVerts[2], 160.0f, 190.0f, 0.2f, 0xFF00FF00u);
    Std3DSystemTest_InitVertex(&aFarVerts[0], 80.0f, 70.0f, 0.8f, 0xFFFF0000u);
    Std3DSystemTest_InitVertex(&aFarVerts[1], 240.0f, 70.0f, 0.8f, 0xFFFF0000u);
    Std3DSystemTest_InitVertex(&aFarVerts[2], 160.0f, 190.0f, 0.8f, 0xFFFF0000u);

    TEST_ASSERT_EQUAL_INT(0, stdDisplay_BackBufferFill(black, NULL));
    std3D_ClearZBuffer();
    TEST_ASSERT_EQUAL_INT(0, std3D_StartScene());
    std3D_DrawRenderList(NULL, STD3D_RS_ZWRITE_DISABLED, aNearVerts, STD_ARRAYLEN(aNearVerts), aIndices, STD_ARRAYLEN(aIndices));
    std3D_DrawRenderList(NULL, 0u, aFarVerts, STD_ARRAYLEN(aFarVerts), aIndices, STD_ARRAYLEN(aIndices));
    Std3DSystemTest_EndScene();
    Std3DSystemTest_AssertBackBufferPixelNear(160u, 120u, 255u, 0u, 0u, 24u);

    TEST_ASSERT_EQUAL_INT(0, stdDisplay_BackBufferFill(black, NULL));
    std3D_ClearZBuffer();
    TEST_ASSERT_EQUAL_INT(0, std3D_StartScene());
    std3D_DrawRenderList(NULL, 0u, aNearVerts, STD_ARRAYLEN(aNearVerts), aIndices, STD_ARRAYLEN(aIndices));
    std3D_DrawRenderList(NULL, 0u, aFarVerts, STD_ARRAYLEN(aFarVerts), aIndices, STD_ARRAYLEN(aIndices));
    Std3DSystemTest_EndScene();
    Std3DSystemTest_AssertBackBufferPixelNear(160u, 120u, 0u, 255u, 0u, 24u);

    Std3DSystemTest_ExerciseRenderStatePermutations();
    Std3DSystemTest_RenderPrimitiveHarnessScene();
    Std3DSystemTest_AssertBackBufferPixelNear(160u, 120u, 0u, 255u, 0u, 24u);
}

void Std3DSystemTest_ExerciseTextureCacheEviction(void)
{
    tSystemTexture aTextures[3];

    Std3DSystemTest_CacheTextureVBuffer(&aTextures[0], Std3DSystemTest_CreateSolidTextureVBuffer(4u, 4u, 255u, 0u, 0u), STDCOLOR_FORMAT_RGB);
    Std3DSystemTest_CacheTextureVBuffer(&aTextures[1], Std3DSystemTest_CreateSolidTextureVBuffer(8u, 8u, 0u, 255u, 0u), STDCOLOR_FORMAT_RGB);

    TEST_ASSERT_EQUAL_INT(0, std3D_StartScene());
    Std3DSystemTest_EndScene();
    std3D_UpdateFrameCount(&aTextures[1]);

    Device3D* pDevice = (Device3D*)std3D_GetCurrentDevice();
    TEST_ASSERT_NOT_NULL(pDevice);
    pDevice->availableMemory = 0u;

    Std3DSystemTest_CacheTextureVBuffer(&aTextures[2], Std3DSystemTest_CreateSolidTextureVBuffer(4u, 4u, 0u, 0u, 255u), STDCOLOR_FORMAT_RGB);
    TEST_ASSERT_NULL(aTextures[0].pCachedTexture);
    TEST_ASSERT_NOT_NULL(aTextures[1].pCachedTexture);
    TEST_ASSERT_NOT_NULL(aTextures[2].pCachedTexture);
    TEST_ASSERT_NULL(aTextures[1].pPrevCachedTexture);
    TEST_ASSERT_EQUAL_PTR(&aTextures[2], aTextures[1].pNextCachedTexture);
    TEST_ASSERT_EQUAL_PTR(&aTextures[1], aTextures[2].pPrevCachedTexture);
    TEST_ASSERT_NULL(aTextures[2].pNextCachedTexture);

    std3D_ClearSystemTexture(&aTextures[0]);
    std3D_ClearSystemTexture(&aTextures[1]);
    std3D_ClearSystemTexture(&aTextures[2]);
    std3D_ResetTextureCache();
    TEST_ASSERT_EQUAL_size_t(pDevice->totalMemory, pDevice->availableMemory);
}

void Std3DSystemTest_ExerciseRectangularTextureUpload(void)
{
    const Device3D* pDevice = std3D_GetCurrentDevice();
    TEST_ASSERT_NOT_NULL(pDevice);

    if ( !pDevice->bSqareOnlyTexture )
    {
        tSystemTexture mipTexture;
        tVBuffer* apMipmaps[4];
        static const uint32_t aWidths[]  = { 8u, 4u, 2u, 1u };
        static const uint32_t aHeights[] = { 4u, 2u, 1u, 1u };

        TEST_ASSERT_EQUAL_INT(0, std3D_SetMipmapFilter(STD3D_MIPMAPFILTER_BILINEAR));
        for ( size_t i = 0u; i < STD_ARRAYLEN(apMipmaps); ++i )
        {
            apMipmaps[i] = Std3DSystemTest_CreateSolidTextureVBuffer(aWidths[i], aHeights[i], (uint8_t)(32u + i * 60u), 128u, 255u);
        }

        STD_ZEROMEM(&mipTexture, sizeof(mipTexture));
        std3D_AllocSystemTexture(&mipTexture, apMipmaps, STD_ARRAYLEN(apMipmaps), STDCOLOR_FORMAT_RGB);
        for ( size_t i = 0u; i < STD_ARRAYLEN(apMipmaps); ++i )
        {
            stdDisplay_VBufferFree(apMipmaps[i]);
        }

        TEST_ASSERT_TRUE(std3D_GetMipMapCount(&mipTexture) >= STD_ARRAYLEN(apMipmaps));
        std3D_AddToTextureCache(&mipTexture, STDCOLOR_FORMAT_RGB);
        TEST_ASSERT_NOT_NULL(mipTexture.pCachedTexture);
        std3D_ClearSystemTexture(&mipTexture);
    }

#if defined(J3D_DIRECTX9)
    bool bPowerOfTwoOnly = (pDevice->d3dDesc.TextureCaps & D3DPTEXTURECAPS_POW2) != 0
        && (pDevice->d3dDesc.TextureCaps & D3DPTEXTURECAPS_NONPOW2CONDITIONAL) == 0;
#else
    bool bPowerOfTwoOnly = (pDevice->d3dDesc.dpcTriCaps.dwTextureCaps & D3DPTEXTURECAPS_POW2) != 0;
#endif
    if ( bPowerOfTwoOnly || pDevice->bSqareOnlyTexture )
    {
        return;
    }

    TEST_ASSERT_EQUAL_INT(0, std3D_SetMipmapFilter(STD3D_MIPMAPFILTER_NONE));

    tSystemTexture texture;
    tSysTexture* pTexture = Std3DSystemTest_CacheTextureVBuffer(&texture, Std3DSystemTest_CreateQuadrantTextureVBuffer(3u, 5u), STDCOLOR_FORMAT_RGB);
    TEST_ASSERT_TRUE(texture.textureSize >= 3u * 5u * 2u);

    Std3DSystemTest_RenderTextureQuadWithUvRange(
        pTexture,
        Std3DSystemTest_EncodeBackBufferColor(0u, 0u, 0u),
        0.0f,
        0.0f,
        1.0f,
        1.0f,
        STD3D_RS_TEX_CPAMP_U | STD3D_RS_TEX_CPAMP_V
    );
    Std3DSystemTest_AssertBackBufferPixelNear(96u, 76u, 255u, 0u, 0u, 40u);
    Std3DSystemTest_AssertBackBufferPixelNear(224u, 76u, 0u, 255u, 0u, 40u);
    Std3DSystemTest_AssertBackBufferPixelNear(96u, 164u, 0u, 0u, 255u, 40u);
    Std3DSystemTest_AssertBackBufferPixelNear(224u, 164u, 255u, 255u, 0u, 40u);
    std3D_ClearSystemTexture(&texture);
}

void Std3DSystemTest_ExerciseDepthWindingAndProjectionBoundaries(void)
{
    const float fov       = 1.57079632679f;
    const float nearPlane = 0.5f;
    const float farPlane  = 100.0f;
    const float q         = farPlane / (farPlane - nearPlane);

    TEST_ASSERT_EQUAL_INT(0, std3D_SetProjection(fov, nearPlane, farPlane));
    D3DMATRIX projection;
    STD_ZEROMEM(&projection, sizeof(projection));
#if defined(J3D_DIRECTX9)
    TEST_ASSERT_EQUAL_HEX32(D3D_OK, IDirect3DDevice9_GetTransform(std3D_GetD3DDevice(), D3DTS_PROJECTION, &projection));
#else
    TEST_ASSERT_EQUAL_HEX32(D3D_OK, IDirect3DDevice3_GetTransform(std3D_GetD3DDevice(), D3DTRANSFORMSTATE_PROJECTION, &projection));
#endif
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, 1.0f, projection._11);
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, 1.0f, projection._22);
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, q, projection._33);
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, 1.0f, projection._34);
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, -(q * nearPlane), projection._43);
    TEST_ASSERT_NOT_EQUAL_INT(0, std3D_SetProjection(fov, 1.0f, 1.0f));

    D3DTLVERTEX aVerts[6];
    WORD aClockwise[3] = { 0u, 1u, 2u };
    WORD aCounterClockwise[3] = { 0u, 2u, 1u };
    uint32_t black = Std3DSystemTest_EncodeBackBufferColor(0u, 0u, 0u);

    Std3DSystemTest_InitVertex(&aVerts[0], 30.0f, 50.0f, 0.0f, 0xFF00FF00u);
    Std3DSystemTest_InitVertex(&aVerts[1], 145.0f, 50.0f, 0.0f, 0xFF00FF00u);
    Std3DSystemTest_InitVertex(&aVerts[2], 88.0f, 190.0f, 0.0f, 0xFF00FF00u);
    Std3DSystemTest_InitVertex(&aVerts[3], 175.0f, 50.0f, 0.99f, 0xFF0000FFu);
    Std3DSystemTest_InitVertex(&aVerts[4], 290.0f, 50.0f, 0.99f, 0xFF0000FFu);
    Std3DSystemTest_InitVertex(&aVerts[5], 232.0f, 190.0f, 0.99f, 0xFF0000FFu);

    TEST_ASSERT_EQUAL_INT(0, stdDisplay_BackBufferFill(black, NULL));
    std3D_SetRenderState(0u);
    std3D_ClearZBuffer();
    TEST_ASSERT_EQUAL_INT(0, std3D_StartScene());
    std3D_DrawRenderList(NULL, 0u, &aVerts[0], 3u, aClockwise, STD_ARRAYLEN(aClockwise));
    std3D_DrawRenderList(NULL, 0u, &aVerts[3], 3u, aCounterClockwise, STD_ARRAYLEN(aCounterClockwise));
    Std3DSystemTest_EndScene();
    Std3DSystemTest_AssertBackBufferPixelNear(88u, 110u, 0u, 255u, 0u, 24u);
    Std3DSystemTest_AssertBackBufferPixelNear(232u, 110u, 0u, 0u, 255u, 24u);

    D3DTLVERTEX aQuadVerts[4];
    WORD aQuadIndices[6] = { 0u, 1u, 2u, 0u, 2u, 3u };
    for ( size_t i = 0u; i < STD_ARRAYLEN(aQuadVerts); ++i )
    {
        static const float aX[] = { 80.0f, 240.0f, 240.0f, 80.0f };
        static const float aY[] = { 60.0f, 60.0f, 180.0f, 180.0f };
        Std3DSystemTest_InitVertex(&aQuadVerts[i], aX[i], aY[i], 0.4f, 0xFFFFFFFFu);
    }

    TEST_ASSERT_EQUAL_INT(0, stdDisplay_BackBufferFill(black, NULL));
    std3D_ClearZBuffer();
    TEST_ASSERT_EQUAL_INT(0, std3D_StartScene());
    std3D_DrawRenderList(NULL, 0u, aQuadVerts, STD_ARRAYLEN(aQuadVerts), aQuadIndices, STD_ARRAYLEN(aQuadIndices));
    Std3DSystemTest_EndScene();
    for ( uint32_t offset = 0u; offset < 20u; ++offset )
    {
        Std3DSystemTest_AssertBackBufferPixelNear(140u + offset, 105u + offset, 255u, 255u, 255u, 24u);
    }

    D3DTLVERTEX aTriangle[3];
    WORD aTriangleIndices[3] = { 0u, 1u, 2u };
    Std3DSystemTest_InitVertex(&aTriangle[0], 80.0f, 70.0f, 0.2f, 0xFF00FF00u);
    Std3DSystemTest_InitVertex(&aTriangle[1], 240.0f, 70.0f, 0.2f, 0xFF00FF00u);
    Std3DSystemTest_InitVertex(&aTriangle[2], 160.0f, 190.0f, 0.2f, 0xFF00FF00u);

    TEST_ASSERT_EQUAL_INT(0, stdDisplay_BackBufferFill(black, NULL));
    std3D_ClearZBuffer();
    TEST_ASSERT_EQUAL_INT(0, std3D_StartScene());
    std3D_DrawRenderList(NULL, 0u, aTriangle, STD_ARRAYLEN(aTriangle), aTriangleIndices, STD_ARRAYLEN(aTriangleIndices));
    Std3DSystemTest_EndScene();

    for ( size_t i = 0u; i < STD_ARRAYLEN(aTriangle); ++i )
    {
        aTriangle[i].sz    = 0.8f;
        aTriangle[i].color = 0xFFFF0000u;
    }

    TEST_ASSERT_EQUAL_INT(0, stdDisplay_BackBufferFill(black, NULL));
    TEST_ASSERT_EQUAL_INT(0, std3D_StartScene());
    std3D_DrawRenderList(NULL, 0u, aTriangle, STD_ARRAYLEN(aTriangle), aTriangleIndices, STD_ARRAYLEN(aTriangleIndices));
    Std3DSystemTest_EndScene();
    Std3DSystemTest_AssertBackBufferPixelNear(160u, 120u, 0u, 0u, 0u, 24u);

    std3D_ClearZBuffer();
    TEST_ASSERT_EQUAL_INT(0, std3D_StartScene());
    std3D_DrawRenderList(NULL, 0u, aTriangle, STD_ARRAYLEN(aTriangle), aTriangleIndices, STD_ARRAYLEN(aTriangleIndices));
    Std3DSystemTest_EndScene();
    Std3DSystemTest_AssertBackBufferPixelNear(160u, 120u, 255u, 0u, 0u, 24u);
}

void Std3DSystemTest_ExerciseRepeated3DLifecycle(void)
{
    for ( size_t iteration = 0u; iteration < 4u; ++iteration )
    {
        TEST_ASSERT_TRUE(std3D_Startup());
        StdWin95SystemTest_SetStd3DStarted(true);
        TEST_ASSERT_TRUE(std3D_GetNumDevices() > 0u);
        TEST_ASSERT_TRUE(std3D_Open(0u));
        Std3DSystemTest_RenderPrimitiveHarnessScene();
        Std3DSystemTest_AssertBackBufferPixelNear(160u, 120u, 0u, 255u, 0u, 24u);
        std3D_Close();
        std3D_Shutdown();
        StdWin95SystemTest_SetStd3DStarted(false);
        TEST_ASSERT_EQUAL_size_t(0u, std3D_GetNumDevices());
        TEST_ASSERT_NULL(std3D_GetD3DDevice());
    }
}

#if defined(J3D_DIRECTX9)
void Std3DSystemTest_ExerciseDeviceResetRecovery(void)
{
    tSystemTexture texture;
    tSysTexture* pTexture = Std3DSystemTest_CacheTextureVBuffer(
        &texture,
        Std3DSystemTest_CreateSolidTextureVBuffer(8u, 8u, 255u, 128u, 0u),
        STDCOLOR_FORMAT_RGB
    );

    Std3DSystemTest_DrawCachedTextureQuad(pTexture);
    Std3DSystemTest_AssertBackBufferPixelNear(160u, 120u, 255u, 128u, 0u, 24u);

    stdDisplay_DisableVSync(true);
    TEST_ASSERT_NULL(texture.pCachedTexture);
    TEST_ASSERT_TRUE(std3D_IsShaderSystemActive());
    std3D_AddToTextureCache(&texture, STDCOLOR_FORMAT_RGB);
    TEST_ASSERT_NOT_NULL(texture.pCachedTexture);
    std3D_UpdateFrameCount(&texture);
    Std3DSystemTest_DrawCachedTextureQuad(texture.pCachedTexture);
    Std3DSystemTest_AssertBackBufferPixelNear(160u, 120u, 255u, 128u, 0u, 24u);

    stdDisplay_DisableVSync(false);
    TEST_ASSERT_NULL(texture.pCachedTexture);
    TEST_ASSERT_TRUE(std3D_IsShaderSystemActive());
    std3D_AddToTextureCache(&texture, STDCOLOR_FORMAT_RGB);
    TEST_ASSERT_NOT_NULL(texture.pCachedTexture);
    std3D_UpdateFrameCount(&texture);
    Std3DSystemTest_DrawCachedTextureQuad(texture.pCachedTexture);
    Std3DSystemTest_AssertBackBufferPixelNear(160u, 120u, 255u, 128u, 0u, 24u);

    std3D_ClearSystemTexture(&texture);
}

void Std3DSystemTest_ExerciseDynamicBufferWraparound(void)
{
    size_t numVerts = J3DMIN(std3D_g_maxVertices, (size_t)STD3D_MAXVERTICES);
    TEST_ASSERT_TRUE(numVerts >= 3u);

    size_t numIndices = numVerts * 3u;
    D3DTLVERTEX* aVerts = (D3DTLVERTEX*)malloc(numVerts * sizeof(*aVerts));
    WORD* aIndices = (WORD*)malloc(numIndices * sizeof(*aIndices));
    TEST_ASSERT_NOT_NULL(aVerts);
    TEST_ASSERT_NOT_NULL(aIndices);

    for ( size_t i = 0u; i < numVerts; ++i )
    {
        Std3DSystemTest_InitVertex(&aVerts[i], 0.0f, 0.0f, 0.4f, 0xFF00FF00u);
    }
    Std3DSystemTest_InitVertex(&aVerts[0], 80.0f, 70.0f, 0.4f, 0xFF00FF00u);
    Std3DSystemTest_InitVertex(&aVerts[1], 240.0f, 70.0f, 0.4f, 0xFF00FF00u);
    Std3DSystemTest_InitVertex(&aVerts[2], 160.0f, 190.0f, 0.4f, 0xFF00FF00u);

    for ( size_t i = 0u; i < numIndices; ++i )
    {
        aIndices[i] = 0u;
    }
    aIndices[0] = 0u;
    aIndices[1] = 1u;
    aIndices[2] = 2u;

    TEST_ASSERT_EQUAL_INT(0, stdDisplay_BackBufferFill(Std3DSystemTest_EncodeBackBufferColor(0u, 0u, 0u), NULL));
    std3D_ClearZBuffer();
    TEST_ASSERT_EQUAL_INT(0, std3D_StartScene());
    std3D_TestSetUseBuffers(true);

    size_t bufferSize = (size_t)STD3D_MAXVERTICES * STD3D_MAXFACEVERTICES;
    size_t numDraws   = bufferSize / numVerts + 2u;
    for ( size_t draw = 0u; draw < numDraws; ++draw )
    {
        std3D_DrawRenderList(NULL, 0u, aVerts, numVerts, aIndices, numIndices);
    }

    std3D_TestSetUseBuffers(false);
    Std3DSystemTest_EndScene();
    Std3DSystemTest_AssertBackBufferPixelNear(160u, 120u, 0u, 255u, 0u, 24u);

    free(aIndices);
    free(aVerts);
}
#endif

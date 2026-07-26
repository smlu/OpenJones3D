#include <unity_fixture.h>

#include <stdint.h>
#include <stdio.h>

#include <std/General/std.h>
#include <std/General/stdFileUtil.h>
#include <std/General/stdMemory.h>
#include <std/General/stdUtil.h>

#include "stdGeneralTest.h"

#define STD3D_TEST_VECTOR_BMP_HEADER_SIZE       54u
#define STD3D_TEST_VECTOR_BMP_INFO_HEADER_SIZE  40u
#define STD3D_TEST_VECTOR_BMP_BITS_PER_PIXEL    24u
#define STD3D_TEST_VECTOR_MAX_SEQUENCE_FRAMES   32u
#define STD3D_TEST_VECTOR_MIN_SCENE_PIXELS      8u
#define STD3D_TEST_VECTOR_FNV_OFFSET_BASIS      14695981039346656037ull
#define STD3D_TEST_VECTOR_FNV_PRIME             1099511628211ull

typedef struct sStd3DTestVectorBmpInfo
{
    uint64_t pixelHash;
    size_t numScenePixels;
    bool bHasDifferentPixels;
} tStd3DTestVectorBmpInfo;

typedef struct sStd3DTestVectorFamily
{
    const char* pBackend;
    const char* pStem;
    size_t numFrames;
    uint32_t width;
    uint32_t height;
} tStd3DTestVectorFamily;

typedef struct sStd3DTestVectorFile
{
    const char* pBackend;
    const char* pName;
    uint32_t width;
    uint32_t height;
} tStd3DTestVectorFile;

static const tStd3DTestVectorFamily std3DTestVector_aFamilies[] = {
    { "dx6", "cube_vertices",                            9u, 320u, 240u },
    { "dx6", "cube_wireframe",                           9u, 320u, 240u },
    { "dx6", "cube_solid",                               9u, 320u, 240u },
    { "dx6", "cube_textured",                            9u, 320u, 240u },
    { "dx6", "cube_textured_vertex_color",               9u, 320u, 240u },
    { "dx6", "cube_solid_vertex_intensity",              9u, 320u, 240u },
    { "dx6", "mipmap_lod_cube",                         32u, 640u, 480u },
    { "dx9", "cube_vertices",                            9u, 320u, 240u },
    { "dx9", "cube_wireframe",                           9u, 320u, 240u },
    { "dx9", "cube_solid",                               9u, 320u, 240u },
    { "dx9", "cube_textured",                            9u, 320u, 240u },
    { "dx9", "cube_textured_vertex_color",               9u, 320u, 240u },
    { "dx9", "cube_solid_vertex_intensity",              9u, 320u, 240u },
    { "dx9", "mipmap_lod_cube",                         32u, 320u, 240u },
    { "dx9", "mipmap_autogen_cube",                     32u, 640u, 480u },
    { "dx9", "mipmap_msaa_anisotropic_cube",            32u, 640u, 480u },
    { "dx9", "mipmap_autogen_msaa_anisotropic_cube",    32u, 640u, 480u },
    { "dx9", "msaa_wireframe_cube",                     32u, 320u, 240u },
    { "dx9", "msaa_solid_cube",                         32u, 320u, 240u },
    { "dx9", "msaa_textured_cube",                      32u, 320u, 240u },
    { "dx9", "msaa_textured_vertex_color_cube",         32u, 320u, 240u },
    { "dx9", "msaa_solid_vertex_intensity_cube",        32u, 320u, 240u },
};

static const tStd3DTestVectorFile std3DTestVector_aFiles[] = {
    { "dx6",    "display_fill_rect_mask.bmp",                  320u, 240u },
    { "dx6",    "mipmap_lod_mask.bmp",                        320u, 240u },
    { "dx6",    "primitive_harness_mask.bmp",                  320u, 240u },
    { "dx6",    "texture_depth_occlusion_mask.bmp",            320u, 240u },
    { "dx6",    "texture_orange_quad_mask.bmp",                320u, 240u },
    { "dx6",    "texture_quad_mask.bmp",                       320u, 240u },
    { "dx6",    "texture_rgba1_quad_mask.bmp",                 320u, 240u },
    { "dx6",    "texture_rgba_quad_mask.bmp",                  320u, 240u },
    { "dx6",    "uv_grid_directx.bmp",                         256u, 256u },
    { "dx6",    "zbuffer_occlusion_mask.bmp",                  320u, 240u },
    { "dx9",    "anisotropic_texture_filter_cube_mask.bmp",    320u, 240u },
    { "dx9",    "anisotropic_texture_filter_mask.bmp",         320u, 240u },
    { "dx9",    "display_fill_rect_mask.bmp",                  320u, 240u },
    { "dx9",    "mipmap_lod_mask.bmp",                        320u, 240u },
    { "dx9",    "msaa_triangle_mask.bmp",                      320u, 240u },
    { "dx9",    "primitive_harness_mask.bmp",                  320u, 240u },
    { "dx9",    "texture_depth_occlusion_mask.bmp",            320u, 240u },
    { "dx9",    "texture_orange_quad_mask.bmp",                320u, 240u },
    { "dx9",    "texture_quad_mask.bmp",                       320u, 240u },
    { "dx9",    "texture_rgba1_quad_mask.bmp",                 320u, 240u },
    { "dx9",    "texture_rgba_quad_mask.bmp",                  320u, 240u },
    { "dx9",    "uv_grid_directx.bmp",                         512u, 512u },
    { "dx9",    "zbuffer_occlusion_mask.bmp",                  320u, 240u },
    { "opengl", "uv_grid_opengl.bmp",                          512u, 512u },
};

static uint16_t std3DTestVector_ReadU16LE(const uint8_t* pData)
{
    return (uint16_t)((uint16_t)pData[0] | ((uint16_t)pData[1] << 8u));
}

static uint32_t std3DTestVector_ReadU32LE(const uint8_t* pData)
{
    return (uint32_t)pData[0]
        | ((uint32_t)pData[1] << 8u)
        | ((uint32_t)pData[2] << 16u)
        | ((uint32_t)pData[3] << 24u);
}

static void std3DTestVector_MakePath(char* pPath, size_t pathSize, const char* pBackend, const char* pName)
{
    stdUtil_Format(pPath, pathSize, "%s/std3D/%s/%s", STD_WIN95_TEST_TV_DIR, pBackend, pName);
}

static tStd3DTestVectorBmpInfo std3DTestVector_AuditBmp(const char* pPath, uint32_t expectedWidth, uint32_t expectedHeight)
{
    uint8_t aHeader[STD3D_TEST_VECTOR_BMP_HEADER_SIZE];
    size_t fileSize = stdFileSize(pPath);
    tFileHandle fh  = stdFileOpen(pPath, "rb");

    TEST_ASSERT_NOT_EQUAL_MESSAGE(0u, fh, pPath);
    TEST_ASSERT_TRUE_MESSAGE(fileSize >= sizeof(aHeader), pPath);
    TEST_ASSERT_EQUAL_size_t_MESSAGE(sizeof(aHeader), stdFileRead(fh, aHeader, sizeof(aHeader)), pPath);
    TEST_ASSERT_EQUAL_UINT8_MESSAGE('B', aHeader[0], pPath);
    TEST_ASSERT_EQUAL_UINT8_MESSAGE('M', aHeader[1], pPath);

    uint32_t declaredFileSize = std3DTestVector_ReadU32LE(&aHeader[2]);
    uint32_t pixelOffset      = std3DTestVector_ReadU32LE(&aHeader[10]);
    uint32_t infoHeaderSize   = std3DTestVector_ReadU32LE(&aHeader[14]);
    uint32_t width            = std3DTestVector_ReadU32LE(&aHeader[18]);
    uint32_t height           = std3DTestVector_ReadU32LE(&aHeader[22]);
    uint16_t planes           = std3DTestVector_ReadU16LE(&aHeader[26]);
    uint16_t bitsPerPixel     = std3DTestVector_ReadU16LE(&aHeader[28]);
    uint32_t compression      = std3DTestVector_ReadU32LE(&aHeader[30]);
    uint32_t imageSize        = std3DTestVector_ReadU32LE(&aHeader[34]);

    TEST_ASSERT_EQUAL_UINT32_MESSAGE((uint32_t)fileSize, declaredFileSize, pPath);
    TEST_ASSERT_EQUAL_UINT32_MESSAGE(STD3D_TEST_VECTOR_BMP_INFO_HEADER_SIZE, infoHeaderSize, pPath);
    TEST_ASSERT_EQUAL_UINT32_MESSAGE(expectedWidth, width, pPath);
    TEST_ASSERT_EQUAL_UINT32_MESSAGE(expectedHeight, height, pPath);
    TEST_ASSERT_EQUAL_UINT16_MESSAGE(1u, planes, pPath);
    TEST_ASSERT_EQUAL_UINT16_MESSAGE(STD3D_TEST_VECTOR_BMP_BITS_PER_PIXEL, bitsPerPixel, pPath);
    TEST_ASSERT_EQUAL_UINT32_MESSAGE(BI_RGB, compression, pPath);
    TEST_ASSERT_EQUAL_UINT32_MESSAGE(sizeof(aHeader), pixelOffset, pPath);

    size_t rowSize      = ((size_t)width * 3u + 3u) & ~(size_t)3u;
    size_t expectedSize = rowSize * (size_t)height;
    TEST_ASSERT_TRUE_MESSAGE((size_t)pixelOffset <= fileSize, pPath);
    TEST_ASSERT_TRUE_MESSAGE(expectedSize <= fileSize - (size_t)pixelOffset, pPath);
    TEST_ASSERT_EQUAL_size_t_MESSAGE(fileSize, (size_t)pixelOffset + expectedSize, pPath);
    TEST_ASSERT_TRUE_MESSAGE(imageSize == 0u || (size_t)imageSize == expectedSize, pPath);
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, stdFileSeek(fh, (int)pixelOffset, SEEK_SET), pPath);

    uint8_t* pRow = (uint8_t*)STDMALLOC(rowSize);
    TEST_ASSERT_NOT_NULL_MESSAGE(pRow, pPath);

    tStd3DTestVectorBmpInfo info = { STD3D_TEST_VECTOR_FNV_OFFSET_BASIS, 0u, false };
    uint32_t firstPixel = 0u;
    bool bHasFirstPixel = false;

    for ( uint32_t y = 0u; y < height; ++y )
    {
        TEST_ASSERT_EQUAL_size_t_MESSAGE(rowSize, stdFileRead(fh, pRow, rowSize), pPath);

        for ( uint32_t x = 0u; x < width; ++x )
        {
            const uint8_t* pPixel = &pRow[(size_t)x * 3u];
            uint32_t pixel = (uint32_t)pPixel[0] | ((uint32_t)pPixel[1] << 8u) | ((uint32_t)pPixel[2] << 16u);

            for ( size_t i = 0u; i < 3u; ++i )
            {
                info.pixelHash ^= pPixel[i];
                info.pixelHash *= STD3D_TEST_VECTOR_FNV_PRIME;
            }

            if ( bHasFirstPixel )
            {
                info.bHasDifferentPixels = info.bHasDifferentPixels || pixel != firstPixel;
            }
            else
            {
                firstPixel     = pixel;
                bHasFirstPixel = true;
            }

            bool bIgnored = pPixel[0] == 255u && pPixel[1] == 0u && pPixel[2] == 255u;
            bool bBlack   = pPixel[0] == 0u && pPixel[1] == 0u && pPixel[2] == 0u;
            if ( !bIgnored && !bBlack )
            {
                ++info.numScenePixels;
            }
        }
    }

    STDFREE(pRow);
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, stdFileClose(fh), pPath);
    TEST_ASSERT_TRUE_MESSAGE(info.bHasDifferentPixels, pPath);
    TEST_ASSERT_TRUE_MESSAGE(info.numScenePixels >= STD3D_TEST_VECTOR_MIN_SCENE_PIXELS, pPath);
    return info;
}

static void std3DTestVector_AuditFamily(const tStd3DTestVectorFamily* pFamily)
{
    uint64_t aHashes[STD3D_TEST_VECTOR_MAX_SEQUENCE_FRAMES];
    TEST_ASSERT_TRUE(pFamily->numFrames <= STD_ARRAYLEN(aHashes));

    for ( size_t i = 0u; i < pFamily->numFrames; ++i )
    {
        char aName[128];
        char aPath[512];
        STD_FORMAT(aName, "%s_frame%02u_mask.bmp", pFamily->pStem, (unsigned)i);
        std3DTestVector_MakePath(aPath, sizeof(aPath), pFamily->pBackend, aName);

        tStd3DTestVectorBmpInfo info = std3DTestVector_AuditBmp(aPath, pFamily->width, pFamily->height);
        aHashes[i] = info.pixelHash;
        if ( i > 0u )
        {
            TEST_ASSERT_TRUE_MESSAGE(aHashes[i - 1u] != aHashes[i], aPath);
        }
    }

    size_t numUnique = 0u;
    for ( size_t i = 0u; i < pFamily->numFrames; ++i )
    {
        bool bSeen = false;
        for ( size_t j = 0u; j < i; ++j )
        {
            bSeen = bSeen || aHashes[j] == aHashes[i];
        }

        if ( !bSeen )
        {
            ++numUnique;
        }
    }

    size_t minUnique = (pFamily->numFrames * 3u + 3u) / 4u;
    TEST_ASSERT_TRUE_MESSAGE(numUnique >= minUnique, pFamily->pStem);
}

static size_t std3DTestVector_CountBmpFiles(const char* pDir)
{
    // Fixture discovery must not depend on stdFileUtil's 128-byte search buffer.
    char aPattern[512];
    TEST_ASSERT_TRUE(stdUtil_Format(aPattern, sizeof(aPattern), "%s/*.bmp", pDir) > 0);
    WIN32_FIND_DATAA data;
    HANDLE hFind = FindFirstFileA(aPattern, &data);
    TEST_ASSERT_NOT_EQUAL(INVALID_HANDLE_VALUE, hFind);
    size_t count = 0u;
    bool bHasDirectory = false;
    do
    {
        bHasDirectory |= (data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0u;
        ++count;
    } while ( FindNextFileA(hFind, &data) );
    DWORD lastError = GetLastError();
    TEST_ASSERT_TRUE(FindClose(hFind));
    TEST_ASSERT_EQUAL_UINT32(ERROR_NO_MORE_FILES, lastError);
    TEST_ASSERT_FALSE(bHasDirectory);
    return count;
}

TEST_GROUP(std3DTestVector);

TEST_SETUP(std3DTestVector)
{
    StdGeneralTest_Startup();
}

TEST_TEAR_DOWN(std3DTestVector)
{
    StdGeneralTest_Shutdown();
}

TEST(std3DTestVector, TestGoldenBmpCorpus)
{
    char aPath[512];
    STD_FORMAT(aPath, "%s/std3D/dx6", STD_WIN95_TEST_TV_DIR);
    TEST_ASSERT_EQUAL_INT(96, std3DTestVector_CountBmpFiles(aPath));
    STD_FORMAT(aPath, "%s/std3D/dx9", STD_WIN95_TEST_TV_DIR);
    TEST_ASSERT_EQUAL_INT(355, std3DTestVector_CountBmpFiles(aPath));
    STD_FORMAT(aPath, "%s/std3D/opengl", STD_WIN95_TEST_TV_DIR);
    TEST_ASSERT_EQUAL_INT(1, std3DTestVector_CountBmpFiles(aPath));

    for ( size_t i = 0u; i < STD_ARRAYLEN(std3DTestVector_aFamilies); ++i )
    {
        std3DTestVector_AuditFamily(&std3DTestVector_aFamilies[i]);
    }

    for ( size_t i = 0u; i < STD_ARRAYLEN(std3DTestVector_aFiles); ++i )
    {
        const tStd3DTestVectorFile* pFile = &std3DTestVector_aFiles[i];
        std3DTestVector_MakePath(aPath, sizeof(aPath), pFile->pBackend, pFile->pName);
        std3DTestVector_AuditBmp(aPath, pFile->width, pFile->height);
    }
}

TEST_GROUP_RUNNER(std3DTestVector)
{
    RUN_TEST_CASE(std3DTestVector, TestGoldenBmpCorpus);
}

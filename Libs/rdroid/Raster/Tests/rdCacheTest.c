#include "rdCacheTestHost.h"

#include <j3dcore/Tests/j3dTest.h>
#include <rdroid/Raster/rdCache.h>
#include <std/General/stdEffect.h>
#include <std/General/stdUtil.h>
#include <unity_fixture.h>

#include <math.h>
#include <stdlib.h>

extern int J3DAPI rdCache_ProcFaceDistanceCompare(const rdCacheProcEntry*, const rdCacheProcEntry*);

tHostServices* rdCacheTest_pHS;
rdLightMode rdCacheTest_lighting;
rdGeometryMode rdCacheTest_geometry;
size_t std3D_g_maxVertices;

#define RDCACHETEST_MAX_FACES 4096

static tStdFadeFactor rdCacheTest_fade;
static rdMaterial rdCacheTest_material;
static tSystemTexture rdCacheTest_texture;
static rdCacheProcEntry* rdCacheTest_pFaces;
static D3DTLVERTEX* rdCacheTest_pVertices;
static rdVector4 rdCacheTest_aColors[RDCACHE_MAXFACEVERTICES];
static uint64_t rdCacheTest_aSeenVertices[RDCACHETEST_MAX_FACES];
static size_t rdCacheTest_aDrawVertices[RDCACHETEST_MAX_FACES];
static size_t rdCacheTest_aFaceOrder[RDCACHETEST_MAX_FACES];
static size_t rdCacheTest_numFaces;
static size_t rdCacheTest_faceVertices;
static size_t rdCacheTest_draws;
static size_t rdCacheTest_triangles;
static size_t rdCacheTest_numSeenFaces;

const tStdFadeFactor* J3DAPI stdEffect_GetFadeFactor(void)
{
    return &rdCacheTest_fade;
}

void J3DAPI std3D_UpdateFrameCount(tSystemTexture* pTexture) { (void)pTexture; }
void J3DAPI std3D_AddToTextureCache(tSystemTexture* pTexture, StdColorFormatType format) { (void)pTexture; (void)format; }
void std3D_SetWireframeRenderState(void) {}
void J3DAPI std3D_DrawPointList(LPD3DTLVERTEX pVertices, size_t count) { (void)pVertices; (void)count; TEST_FAIL_MESSAGE("Unexpected point draw"); }
void J3DAPI std3D_DrawLineStrip(LPD3DTLVERTEX pVertices, size_t count) { (void)pVertices; (void)count; TEST_FAIL_MESSAGE("Unexpected wireframe draw"); }

void J3DAPI std3D_DrawRenderList(tSysTexture* pTexture, Std3DRenderState flags,
    LPD3DTLVERTEX pVertices, size_t numVertices, LPWORD pIndices, size_t numIndices)
{
    (void)pTexture;
    (void)flags;
    TEST_ASSERT_TRUE(rdCacheTest_draws < RDCACHETEST_MAX_FACES);
    TEST_ASSERT_TRUE(numVertices <= UINT16_MAX);
    TEST_ASSERT_TRUE(numVertices <= std3D_g_maxVertices);
    TEST_ASSERT_EQUAL_size_t(0, numIndices % 3);
    rdCacheTest_aDrawVertices[rdCacheTest_draws++] = numVertices;

    for ( size_t i = 0; i < numIndices; i += 3 )
    {
        for ( size_t j = 0; j < 3; ++j ) TEST_ASSERT_TRUE(pIndices[i + j] < numVertices);
        const size_t face = (size_t)pVertices[pIndices[i]].sx;
        TEST_ASSERT_TRUE(face < rdCacheTest_numFaces);
        if ( rdCacheTest_aSeenVertices[face] == 0 )
        {
            rdCacheTest_aFaceOrder[rdCacheTest_numSeenFaces++] = face;
        }
        TEST_ASSERT_NOT_EQUAL(pIndices[i], pIndices[i + 1]);
        TEST_ASSERT_NOT_EQUAL(pIndices[i], pIndices[i + 2]);
        TEST_ASSERT_NOT_EQUAL(pIndices[i + 1], pIndices[i + 2]);
        for ( size_t j = 0; j < 3; ++j )
        {
            TEST_ASSERT_TRUE(pIndices[i + j] < numVertices);
            const D3DTLVERTEX* pVertex = &pVertices[pIndices[i + j]];
            TEST_ASSERT_EQUAL_FLOAT((float)face, pVertex->sx);
            const size_t localVertex = (size_t)pVertex->sy;
            TEST_ASSERT_TRUE(localVertex < rdCacheTest_faceVertices);
            rdCacheTest_aSeenVertices[face] |= UINT64_C(1) << localVertex;
        }
        ++rdCacheTest_triangles;
    }
}

TEST_GROUP(rdCache);

TEST_SETUP(rdCache)
{
    J3DTest_Startup();
    J3DTest_BindHostServicesGlobal(rdroid_g_pHS_ADDR);
    rdCacheTest_lighting = RD_LIGHTING_GOURAUD;
    rdCacheTest_geometry = RD_GEOMETRY_FULL;
    std3D_g_maxVertices = 0x00ffffff;
    rdCacheTest_fade = (tStdFadeFactor){ 0 };
    rdCacheTest_material = (rdMaterial){ 0 };
    rdCacheTest_texture = (tSystemTexture){ 0 };
    rdCacheTest_material.numCels = 1;
    rdCacheTest_material.aTextures = &rdCacheTest_texture;
    rdCacheTest_material.formatType = STDCOLOR_FORMAT_RGB;
    for ( size_t i = 0; i < STD_ARRAYLEN(rdCacheTest_aColors); ++i )
    {
        rdCacheTest_aColors[i] = (rdVector4){ 1.0f, 1.0f, 1.0f, 0.5f };
    }
    memset(rdCacheTest_aSeenVertices, 0, sizeof(rdCacheTest_aSeenVertices));
    memset(rdCacheTest_aDrawVertices, 0, sizeof(rdCacheTest_aDrawVertices));
    rdCacheTest_draws = rdCacheTest_triangles = rdCacheTest_numSeenFaces = 0;
    rdCacheTest_numFaces = rdCacheTest_faceVertices = 0;
    rdCache_AdvanceFrame();
}

TEST_TEAR_DOWN(rdCache)
{
    free(rdCacheTest_pFaces);
    free(rdCacheTest_pVertices);
    rdCacheTest_pFaces = NULL;
    rdCacheTest_pVertices = NULL;
    const int numAsserts = J3DTest_GetAssertCount();
    J3DTest_Shutdown();
    TEST_ASSERT_EQUAL_INT(0, numAsserts);
}

static void rdCacheTest_InitFace(rdCacheProcEntry* pFace, D3DTLVERTEX* pVertices, rdVector4* pColors, size_t face)
{
    *pFace = (rdCacheProcEntry){ 0 };
    pFace->aVertices = pVertices;
    pFace->aVertIntensities = pColors;
    pFace->numVertices = rdCacheTest_faceVertices;
    pFace->lightingMode = RD_LIGHTING_GOURAUD;
    pFace->extraLight.alpha = 1.0f;
    pFace->pMaterial = &rdCacheTest_material;
    pFace->matCelNum = -1;
    pFace->flags = RD_FF_TEX_TRANSLUCENT;
    pFace->distance = 1.0f - (float)face / 8192.0f;
    for ( size_t j = 0; j < rdCacheTest_faceVertices; ++j )
    {
        pVertices[j] = (D3DTLVERTEX){ 0 };
        pVertices[j].sx = (float)face;
        pVertices[j].sy = (float)j;
        pVertices[j].sz = pFace->distance;
        pVertices[j].rhw = 1.0f;
    }
}

static void rdCacheTest_MakeFaces(size_t numFaces, size_t numVertices)
{
    rdCacheTest_numFaces = numFaces;
    rdCacheTest_faceVertices = numVertices;
    rdCacheTest_pFaces = calloc(numFaces, sizeof(*rdCacheTest_pFaces));
    rdCacheTest_pVertices = calloc(numFaces * numVertices, sizeof(*rdCacheTest_pVertices));
    TEST_ASSERT_NOT_NULL(rdCacheTest_pFaces);
    TEST_ASSERT_NOT_NULL(rdCacheTest_pVertices);
    for ( size_t i = 0; i < numFaces; ++i )
    {
        rdCacheTest_InitFace(&rdCacheTest_pFaces[i], &rdCacheTest_pVertices[i * numVertices], rdCacheTest_aColors, i);
    }
}

static void rdCacheTest_AssertCoverage(void)
{
    const uint64_t expected = rdCacheTest_faceVertices == 64 ? UINT64_MAX : (UINT64_C(1) << rdCacheTest_faceVertices) - 1;
    size_t submittedVertices = 0;
    for ( size_t i = 0; i < rdCacheTest_draws; ++i ) submittedVertices += rdCacheTest_aDrawVertices[i];
    TEST_ASSERT_EQUAL_size_t(rdCacheTest_numFaces * rdCacheTest_faceVertices, submittedVertices);
    TEST_ASSERT_EQUAL_size_t(rdCacheTest_numFaces, rdCacheTest_numSeenFaces);
    TEST_ASSERT_EQUAL_size_t(rdCacheTest_numFaces * (rdCacheTest_faceVertices - 2), rdCacheTest_triangles);
    for ( size_t i = 0; i < rdCacheTest_numFaces; ++i )
    {
        TEST_ASSERT_EQUAL_HEX32((uint32_t)expected, (uint32_t)rdCacheTest_aSeenVertices[i]);
        TEST_ASSERT_EQUAL_HEX32((uint32_t)(expected >> 32), (uint32_t)(rdCacheTest_aSeenVertices[i] >> 32));
    }
}

TEST(rdCache, DistanceComparatorHandlesEqualDepthsAndSignedZero)
{
    rdCacheProcEntry a = { 0 }, b = { 0 };
    a.distance = b.distance = 0.25f;
    TEST_ASSERT_EQUAL_INT(0, rdCache_ProcFaceDistanceCompare(&a, &a));
    TEST_ASSERT_EQUAL_INT(0, rdCache_ProcFaceDistanceCompare(&a, &b));
    TEST_ASSERT_EQUAL_INT(0, rdCache_ProcFaceDistanceCompare(&b, &a));
    a.distance = -0.0f;
    b.distance = 0.0f;
    TEST_ASSERT_EQUAL_INT(0, rdCache_ProcFaceDistanceCompare(&a, &b));
}

TEST(rdCache, DistanceComparatorProvidesConsistentOrdering)
{
    const float depths[] = { INFINITY, FLT_MAX, 1.0f, 0.0f, -0.0f, -1.0f, -INFINITY, NAN, -NAN };
    for ( size_t i = 0; i < STD_ARRAYLEN(depths); ++i )
    {
        for ( size_t j = 0; j < STD_ARRAYLEN(depths); ++j )
        {
            rdCacheProcEntry a = { 0 }, b = { 0 };
            a.distance = depths[i];
            b.distance = depths[j];
            const int ab = rdCache_ProcFaceDistanceCompare(&a, &b);
            const int ba = rdCache_ProcFaceDistanceCompare(&b, &a);
            TEST_ASSERT_EQUAL_INT(-ab, ba);
            if ( i < j ) TEST_ASSERT_TRUE(ab <= 0);
            if ( i == j ) TEST_ASSERT_EQUAL_INT(0, ab);
        }
    }
}

TEST(rdCache, MaximumSizeFacesSplitBeforeIndexWrap)
{
    rdCacheTest_MakeFaces(1025, 64);
    rdCache_SendFaceListToHardware(rdCacheTest_numFaces, rdCacheTest_pFaces, NULL);
    TEST_ASSERT_EQUAL_size_t(2, rdCacheTest_draws);
    TEST_ASSERT_EQUAL_size_t(65472, rdCacheTest_aDrawVertices[0]);
    TEST_ASSERT_EQUAL_size_t(128, rdCacheTest_aDrawVertices[1]);
    rdCacheTest_AssertCoverage();
}

TEST(rdCache, ExactVertexLimitFitsAndNextFaceStartsNewBatch)
{
    rdCacheTest_MakeFaces(3856, 17);
    rdCache_SendFaceListToHardware(rdCacheTest_numFaces, rdCacheTest_pFaces, NULL);
    TEST_ASSERT_EQUAL_size_t(2, rdCacheTest_draws);
    TEST_ASSERT_EQUAL_size_t(65535, rdCacheTest_aDrawVertices[0]);
    TEST_ASSERT_EQUAL_size_t(17, rdCacheTest_aDrawVertices[1]);
    rdCacheTest_AssertCoverage();
}

TEST(rdCache, StockSizedFacesRemainInOneBatch)
{
    rdCacheTest_MakeFaces(4096, 12);
    rdCache_SendFaceListToHardware(rdCacheTest_numFaces, rdCacheTest_pFaces, NULL);
    TEST_ASSERT_EQUAL_size_t(1, rdCacheTest_draws);
    rdCacheTest_AssertCoverage();
}

TEST(rdCache, ExistingBackendIndexBudgetStillSplitsBatches)
{
    std3D_g_maxVertices = 372;
    rdCacheTest_MakeFaces(3, 64);
    rdCache_SendFaceListToHardware(rdCacheTest_numFaces, rdCacheTest_pFaces, NULL);
    TEST_ASSERT_EQUAL_size_t(3, rdCacheTest_draws);
    rdCacheTest_AssertCoverage();
}

TEST(rdCache, AlphaQueuePreservesFacesAndOrderAcrossVertexSplit)
{
    rdCacheTest_numFaces = 1025;
    rdCacheTest_faceVertices = 64;
    for ( size_t i = 0; i < rdCacheTest_numFaces; ++i )
    {
        rdCacheProcEntry* pFace = rdCache_GetAlphaProcEntry();
        TEST_ASSERT_NOT_NULL(pFace);
        D3DTLVERTEX* pVertices = pFace->aVertices;
        rdVector4* pColors = pFace->aVertIntensities;
        memcpy(pColors, rdCacheTest_aColors, sizeof(rdCacheTest_aColors));
        rdCacheTest_InitFace(pFace, pVertices, pColors, i);
        rdCache_AddAlphaProcFace(rdCacheTest_faceVertices);
    }
    rdCache_FlushAlpha();
    TEST_ASSERT_EQUAL_size_t(2, rdCacheTest_draws);
    rdCacheTest_AssertCoverage();
    for ( size_t i = 0; i < rdCacheTest_numFaces; ++i )
    {
        TEST_ASSERT_EQUAL_size_t(i, rdCacheTest_aFaceOrder[i]);
    }
    rdCache_FlushAlpha();
    TEST_ASSERT_EQUAL_size_t(2, rdCacheTest_draws);
}

TEST(rdCache, AlphaSortOrdersFiniteDepthsBeforeNaNsWithoutLosingFaces)
{
    rdCacheTest_MakeFaces(5, 3);
    const float depths[] = { NAN, -INFINITY, 0.5f, INFINITY, 1.0f };
    const size_t order[] = { 3, 4, 2, 1, 0 };
    for ( size_t i = 0; i < STD_ARRAYLEN(depths); ++i ) rdCacheTest_pFaces[i].distance = depths[i];
    rdCache_SendFaceListToHardware(rdCacheTest_numFaces, rdCacheTest_pFaces, rdCache_ProcFaceDistanceCompare);
    TEST_ASSERT_EQUAL_size_t(1, rdCacheTest_draws);
    rdCacheTest_AssertCoverage();
    for ( size_t i = 0; i < STD_ARRAYLEN(order); ++i ) TEST_ASSERT_EQUAL_size_t(order[i], rdCacheTest_aFaceOrder[i]);
}

TEST(rdCache, EmptyFaceListDoesNotDraw)
{
    rdCache_SendFaceListToHardware(0, NULL, NULL);
    TEST_ASSERT_EQUAL_size_t(0, rdCacheTest_draws);
}

TEST_GROUP_RUNNER(rdCache)
{
    RUN_TEST_CASE(rdCache, DistanceComparatorHandlesEqualDepthsAndSignedZero);
    RUN_TEST_CASE(rdCache, DistanceComparatorProvidesConsistentOrdering);
    RUN_TEST_CASE(rdCache, MaximumSizeFacesSplitBeforeIndexWrap);
    RUN_TEST_CASE(rdCache, ExactVertexLimitFitsAndNextFaceStartsNewBatch);
    RUN_TEST_CASE(rdCache, StockSizedFacesRemainInOneBatch);
    RUN_TEST_CASE(rdCache, ExistingBackendIndexBudgetStillSplitsBatches);
    RUN_TEST_CASE(rdCache, AlphaQueuePreservesFacesAndOrderAcrossVertexSplit);
    RUN_TEST_CASE(rdCache, AlphaSortOrdersFiniteDepthsBeforeNaNsWithoutLosingFaces);
    RUN_TEST_CASE(rdCache, EmptyFaceListDoesNotDraw);
}

static void rdCacheTest_RunAll(void) { TEST_rdCache_GROUP_RUNNER(); }
int main(int argc, const char* argv[]) { return UnityMain(argc, argv, rdCacheTest_RunAll); }

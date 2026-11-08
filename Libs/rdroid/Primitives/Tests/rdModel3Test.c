#include "rdModel3TestHost.h"

#include <unity_fixture.h>
#include <j3dcore/Tests/j3dTest.h>

#include <rdroid/Math/rdMatrix.h>
#include <rdroid/Math/rdVector.h>
#include <rdroid/Primitives/rdModel3.h>
#include <rdroid/Raster/rdCache.h>
#include <rdroid/Engine/rdClip.h>

#include <math.h>

tHostServices* rdModel3Test_pHS;
rdCamera* rdModel3Test_pCamera;
rdMatrix34 rdModel3Test_cameraMatrix;
rdMatrix34 rdModel3Test_identity = { {1,0,0}, {0,1,0}, {0,0,1}, {0,0,0} };
rdRoidFlags rdModel3Test_renderOptions;
rdLightMode rdModel3Test_lightingMode;
int rdModel3Test_numDrawnFaces;
int rdModel3Test_numDrawnAlphaFaces;

static rdCamera rdModel3Test_camera;
static rdModel3Mesh rdModel3Test_mesh;
static rdFace rdModel3Test_aFaces[6];
static rdVector3 rdModel3Test_aVertices[8];
static int rdModel3Test_aIndices[6][4] = {
    {1,3,7,5}, {0,4,6,2}, {2,6,7,3}, {0,1,5,4}, {4,5,7,6}, {0,2,3,1}
};
static const rdVector3 rdModel3Test_aNormals[6] = {
    {1,0,0}, {-1,0,0}, {0,1,0}, {0,-1,0}, {0,0,1}, {0,0,-1}
};
static rdModel3DrawFaceFunc rdModel3Test_pfPreviousDraw;
static rdCacheProcEntry rdModel3Test_aQueuedFaces[64];
static rdVector4 rdModel3Test_aQueuedLights[64][4];
static D3DTLVERTEX rdModel3Test_aQueuedVertices[64][4];
static size_t rdModel3Test_numQueuedFaces;
static int rdModel3Test_bClipFace;
static rdModel3 rdModel3Test_model;
static rdModel3HNode rdModel3Test_node;
static rdThing rdModel3Test_thing;
static rdMatrix34 rdModel3Test_joint;
static rdCanvas rdModel3Test_canvas;
static tVBuffer rdModel3Test_buffer;
static int rdModel3Test_bNestedDraw;
static int rdModel3Test_bDidNestedDraw;
static size_t rdModel3Test_numNestedFaces;

TEST_GROUP(rdModel3);

rdCacheProcEntry* rdModel3Test_GetProcEntry(void)
{
    TEST_ASSERT_TRUE(rdModel3Test_numQueuedFaces < STD_ARRAYLEN(rdModel3Test_aQueuedFaces));
    rdCacheProcEntry* pEntry = &rdModel3Test_aQueuedFaces[rdModel3Test_numQueuedFaces];
    *pEntry = (rdCacheProcEntry){ 0 };
    pEntry->aVertIntensities = rdModel3Test_aQueuedLights[rdModel3Test_numQueuedFaces];
    pEntry->aVertices = rdModel3Test_aQueuedVertices[rdModel3Test_numQueuedFaces];
    return pEntry;
}

rdCacheProcEntry* rdModel3Test_GetAlphaProcEntry(void)
{
    return rdModel3Test_GetProcEntry();
}

void J3DAPI rdModel3Test_AddProcFace(size_t numVertices)
{
    rdModel3Test_aQueuedFaces[rdModel3Test_numQueuedFaces].numVertices = numVertices;
    ++rdModel3Test_numQueuedFaces;
}

void J3DAPI rdModel3Test_AddAlphaProcFace(size_t numVertices)
{
    rdModel3Test_AddProcFace(numVertices);
}

int J3DAPI rdModel3Test_ClipFaceToPlane(const rdClipFrustum* pFrustum, rdCacheProcEntry* pEntry, const rdFace* pFace,
    const rdVector3* aVertices, const rdVector2* aTexVertices, const rdVector4* aLightColors, const rdVector4* aVertColors)
{
    J3D_UNUSED(pFrustum);
    J3D_UNUSED(aVertices);
    J3D_UNUSED(aTexVertices);
    J3D_UNUSED(aLightColors);
    J3D_UNUSED(aVertColors);
    TEST_ASSERT_TRUE(pFace->numVertices <= 4u);
    if ( !rdModel3Test_bClipFace )
    {
        return 0;
    }
    for ( size_t i = 0; i < pFace->numVertices; ++i )
    {
        pEntry->aVertIntensities[i] = (rdVector4){ 0,0,0,1 };
    }
    return 1;
}

static void rdModel3Test_InitCube(float radius)
{
    float halfExtent = radius / sqrtf(3.0f);
    for ( size_t i = 0; i < STD_ARRAYLEN(rdModel3Test_aVertices); ++i )
    {
        rdModel3Test_aVertices[i] = (rdVector3){
            (i & 1u) ? halfExtent : -halfExtent,
            (i & 2u) ? halfExtent : -halfExtent,
            (i & 4u) ? halfExtent : -halfExtent
        };
    }
    for ( size_t i = 0; i < STD_ARRAYLEN(rdModel3Test_aFaces); ++i )
    {
        rdModel3Test_aFaces[i] = (rdFace){ 0 };
        rdModel3Test_aFaces[i].num = i;
        rdModel3Test_aFaces[i].normal = rdModel3Test_aNormals[i];
        rdModel3Test_aFaces[i].numVertices = 4;
        rdModel3Test_aFaces[i].aVertices = rdModel3Test_aIndices[i];
    }
    rdModel3Test_mesh = (rdModel3Mesh){ 0 };
    rdModel3Test_mesh.geoMode = RD_GEOMETRY_FULL;
    rdModel3Test_mesh.lightMode = RD_LIGHTING_NONE;
    rdModel3Test_mesh.apVertices = rdModel3Test_aVertices;
    rdModel3Test_mesh.numVertices = STD_ARRAYLEN(rdModel3Test_aVertices);
    rdModel3Test_mesh.aFaces = rdModel3Test_aFaces;
    rdModel3Test_mesh.numFaces = STD_ARRAYLEN(rdModel3Test_aFaces);
    rdModel3Test_mesh.meshColor = (rdVector4){ 1,1,1,1 };
    rdModel3Test_mesh.radius = radius;
}

TEST_SETUP(rdModel3)
{
    J3DTest_Startup();
    J3DTest_BindHostServicesGlobal(rdroid_g_pHS_ADDR);
    rdModel3Test_camera = (rdCamera){ 0 };
    rdModel3Test_pCamera = &rdModel3Test_camera;
    rdModel3Test_renderOptions = RDROID_BACKFACE_CULLING_ENABLED;
    rdModel3Test_lightingMode = RD_LIGHTING_NONE;
    rdVector3 cameraPyr = { -17.700001f, 0, 0 };
    rdVector3 cameraPos = { 0, -2, 0.64999998f };
    rdMatrix_Build34(&rdModel3Test_cameraMatrix, &cameraPyr, &cameraPos);
    rdMatrix_InvertOrtho34(&rdModel3Test_camera.viewMatrix, &rdModel3Test_cameraMatrix);
    rdModel3Test_InitCube(0.03f);
    rdModel3Test_numQueuedFaces = 0;
    rdModel3Test_bClipFace = 1;
    rdModel3Test_bNestedDraw = 0;
    rdModel3Test_bDidNestedDraw = 0;
    rdModel3Test_numNestedFaces = 0;
    rdModel3_EnableFogRendering(0);
    rdModel3Test_pfPreviousDraw = rdModel3_RegisterFaceDraw(rdModel3_DrawFace);
}

TEST_TEAR_DOWN(rdModel3)
{
    rdModel3_RegisterFaceDraw(rdModel3Test_pfPreviousDraw);
    rdModel3_EnableFogRendering(0);
    TEST_ASSERT_EQUAL_INT(0, J3DTest_GetAssertCount());
    rdModel3Test_pCamera = NULL;
    J3DTest_Shutdown();
}

static void rdModel3Test_InitQueuedDraw(void)
{
    rdModel3Test_model = (rdModel3){ 0 };
    rdModel3Test_node = (rdModel3HNode){ 0 };
    rdModel3Test_thing = (rdThing){ 0 };
    rdModel3Test_canvas = (rdCanvas){ 0 };
    rdModel3Test_buffer = (tVBuffer){ 0 };
    rdModel3Test_joint = rdModel3Test_identity;
    rdModel3Test_node.meshIdx = 0;
    rdModel3Test_model.numGeos = 1;
    rdModel3Test_model.aGeos[0].numMeshes = 1;
    rdModel3Test_model.aGeos[0].aMeshes = &rdModel3Test_mesh;
    rdModel3Test_model.aHierarchyNodes = &rdModel3Test_node;
    rdModel3Test_model.numHNodes = 1;
    rdModel3Test_thing.type = RD_THING_MODEL3;
    rdModel3Test_thing.data.pModel3 = &rdModel3Test_model;
    rdModel3Test_thing.geosetNum = 0;
    rdModel3Test_thing.paJointMatrices = &rdModel3Test_joint;
    rdModel3Test_thing.rdFrameNum = rdCache_GetFrameNum();
    rdModel3Test_buffer.rasterInfo.colorInfo.bpp = 32;
    rdModel3Test_canvas.pVBuffer = &rdModel3Test_buffer;
    rdModel3Test_camera.pCanvas = &rdModel3Test_canvas;
    rdModel3Test_mesh.meshColor.alpha = 0.4f;
    rdModel3_RegisterFaceDraw(rdModel3_DrawFace);
}

static void rdModel3Test_AssertQueuedFlags(size_t first, size_t end, rdFaceFlags flags)
{
    TEST_ASSERT_TRUE(end > first);
    for ( size_t i = first; i < end; ++i )
    {
        TEST_ASSERT_EQUAL_HEX32(flags, rdModel3Test_aQueuedFaces[i].flags);
        TEST_ASSERT_EQUAL_UINT(4u, rdModel3Test_aQueuedFaces[i].numVertices);
        TEST_ASSERT_EQUAL_FLOAT(0.4f, rdModel3Test_aQueuedFaces[i].aVertIntensities[0].alpha);
    }
}

TEST(rdModel3, TestDrawFlagsPersistInDeferredFacesWithoutChangingAssets)
{
    rdModel3Test_InitQueuedDraw();
    for ( size_t i = 0; i < STD_ARRAYLEN(rdModel3Test_aFaces); ++i )
    {
        rdModel3Test_aFaces[i].flags = RD_FF_TEX_CLAMP_X;
    }
    rdFace aOriginalFaces[6];
    memcpy(aOriginalFaces, rdModel3Test_aFaces, sizeof(aOriginalFaces));
    rdModel3Mesh originalMesh = rdModel3Test_mesh;
    TEST_ASSERT_EQUAL_INT(1, rdModel3_DrawWithFlags(&rdModel3Test_thing, &rdModel3Test_joint, RD_FF_ZWRITE_DISABLED));
    size_t overriddenFaces = rdModel3Test_numQueuedFaces;
    rdModel3Test_AssertQueuedFlags(0, overriddenFaces, RD_FF_TEX_CLAMP_X | RD_FF_TEX_TRANSLUCENT | RD_FF_ZWRITE_DISABLED);

    // No flush occurs between draws. Earlier queue entries must retain their
    // flags after the scoped override has already been restored.
    TEST_ASSERT_EQUAL_INT(1, rdModel3_Draw(&rdModel3Test_thing, &rdModel3Test_joint));
    rdModel3Test_AssertQueuedFlags(overriddenFaces, rdModel3Test_numQueuedFaces, RD_FF_TEX_CLAMP_X | RD_FF_TEX_TRANSLUCENT);
    rdModel3Test_AssertQueuedFlags(0, overriddenFaces, RD_FF_TEX_CLAMP_X | RD_FF_TEX_TRANSLUCENT | RD_FF_ZWRITE_DISABLED);
    TEST_ASSERT_EQUAL_MEMORY(aOriginalFaces, rdModel3Test_aFaces, sizeof(aOriginalFaces));
    TEST_ASSERT_EQUAL_MEMORY(&originalMesh, &rdModel3Test_mesh, sizeof(originalMesh));
}

TEST(rdModel3, TestRejectedDrawAndClippedFacesDoNotLeakFlags)
{
    rdModel3Test_InitQueuedDraw();
    rdModel3Test_thing.frustumCull = RDFRUSTUMCULL_OUTSIDE;
    TEST_ASSERT_EQUAL_INT(0, rdModel3_DrawWithFlags(&rdModel3Test_thing, &rdModel3Test_joint, RD_FF_ZWRITE_DISABLED));
    TEST_ASSERT_EQUAL_UINT(0u, rdModel3Test_numQueuedFaces);
    rdModel3Test_thing.frustumCull = RDFRUSTUMCULL_INSIDE;
    rdModel3Test_bClipFace = 0;
    TEST_ASSERT_EQUAL_INT(1, rdModel3_DrawWithFlags(&rdModel3Test_thing, &rdModel3Test_joint, RD_FF_ZWRITE_DISABLED));
    TEST_ASSERT_EQUAL_UINT(0u, rdModel3Test_numQueuedFaces);
    rdModel3Test_bClipFace = 1;
    TEST_ASSERT_EQUAL_INT(1, rdModel3_Draw(&rdModel3Test_thing, &rdModel3Test_joint));
    rdModel3Test_AssertQueuedFlags(0, rdModel3Test_numQueuedFaces, RD_FF_TEX_TRANSLUCENT);
}

TEST(rdModel3, TestDrawFlagsCoexistWithFogAndAuthoredFlags)
{
    rdModel3Test_InitQueuedDraw();
    for ( size_t i = 0; i < STD_ARRAYLEN(rdModel3Test_aFaces); ++i )
    {
        rdModel3Test_aFaces[i].flags = RD_FF_TEX_CLAMP_X | RD_FF_ZWRITE_DISABLED;
    }
    rdModel3_EnableFogRendering(1);
    TEST_ASSERT_EQUAL_INT(1, rdModel3_DrawWithFlags(&rdModel3Test_thing, &rdModel3Test_joint, RD_FF_TEX_CLAMP_Y));
    size_t overriddenFaces = rdModel3Test_numQueuedFaces;
    rdModel3Test_AssertQueuedFlags(0, overriddenFaces,
        RD_FF_TEX_CLAMP_X | RD_FF_TEX_CLAMP_Y | RD_FF_ZWRITE_DISABLED | RD_FF_TEX_TRANSLUCENT | RD_FF_FOG_ENABLED);
    rdModel3_EnableFogRendering(0);
    TEST_ASSERT_EQUAL_INT(1, rdModel3_Draw(&rdModel3Test_thing, &rdModel3Test_joint));
    rdModel3Test_AssertQueuedFlags(overriddenFaces, rdModel3Test_numQueuedFaces,
        RD_FF_TEX_CLAMP_X | RD_FF_ZWRITE_DISABLED | RD_FF_TEX_TRANSLUCENT);
}

static void J3DAPI rdModel3Test_NestedDrawFace(const rdFace* pFace, const rdVector3* aView, int bBackFace, const rdVector4* pColor)
{
    // The callback still receives the authored face, not an overridden copy.
    TEST_ASSERT_EQUAL_PTR(&rdModel3Test_aFaces[pFace->num], pFace);
    if ( !rdModel3Test_bNestedDraw && !rdModel3Test_bDidNestedDraw )
    {
        rdModel3Test_bDidNestedDraw = 1;
        rdModel3Test_bNestedDraw = 1;
        TEST_ASSERT_EQUAL_INT(1, rdModel3_Draw(&rdModel3Test_thing, &rdModel3Test_joint));
        rdModel3Test_numNestedFaces = rdModel3Test_numQueuedFaces;
        rdThing rejected = rdModel3Test_thing;
        rejected.frustumCull = RDFRUSTUMCULL_OUTSIDE;
        TEST_ASSERT_EQUAL_INT(0, rdModel3_DrawWithFlags(&rejected, &rdModel3Test_joint, RD_FF_TEX_CLAMP_Y));
        rdModel3Test_bNestedDraw = 0;
    }
    rdModel3_DrawFace(pFace, aView, bBackFace, pColor);
}

TEST(rdModel3, TestNestedOrdinaryDrawClearsAndRestoresOuterFlags)
{
    rdModel3Test_InitQueuedDraw();
    rdModel3_RegisterFaceDraw(rdModel3Test_NestedDrawFace);
    TEST_ASSERT_EQUAL_INT(1, rdModel3_DrawWithFlags(&rdModel3Test_thing, &rdModel3Test_joint, RD_FF_ZWRITE_DISABLED));
    TEST_ASSERT_TRUE(rdModel3Test_bDidNestedDraw);
    rdModel3Test_AssertQueuedFlags(0, rdModel3Test_numNestedFaces, RD_FF_TEX_TRANSLUCENT);
    rdModel3Test_AssertQueuedFlags(rdModel3Test_numNestedFaces, rdModel3Test_numQueuedFaces,
        RD_FF_TEX_TRANSLUCENT | RD_FF_ZWRITE_DISABLED);
}

TEST_GROUP_RUNNER(rdModel3)
{
    RUN_TEST_CASE(rdModel3, TestDrawFlagsPersistInDeferredFacesWithoutChangingAssets);
    RUN_TEST_CASE(rdModel3, TestRejectedDrawAndClippedFacesDoNotLeakFlags);
    RUN_TEST_CASE(rdModel3, TestDrawFlagsCoexistWithFogAndAuthoredFlags);
    RUN_TEST_CASE(rdModel3, TestNestedOrdinaryDrawClearsAndRestoresOuterFlags);
}

static void rdModel3Test_RunAll(void)
{
    RUN_TEST_GROUP(rdModel3);
}

int main(int argc, const char* argv[])
{
    return UnityMain(argc, argv, rdModel3Test_RunAll);
}

#include <unity_fixture.h>

#include <stdio.h>

#include <std/General/std.h>
#include <std/General/stdColor.h>
#include <std/General/stdUtil.h>
#include <std/Win95/std3D.h>
#include <std/Win95/stdDisplay.h>

#include "std3DSystemTestSupport.h"
#include "stdWin95SystemTestSupport.h"

TEST_GROUP(std3DX6System);

TEST_SETUP(std3DX6System)
{
    StdWin95SystemTest_Startup();
}

TEST_TEAR_DOWN(std3DX6System)
{
    StdWin95SystemTest_Shutdown();
}

static void std3DX6SystemTest_RequireOpen3DDeviceAtResolution(uint32_t width, uint32_t height)
{
    StdWin95SystemTest_RequireHiddenWindow();
    StdWin95SystemTest_RequireWindowedDisplayModeAtResolution(width, height);

    if ( !std3D_Startup() )
    {
        TEST_IGNORE_MESSAGE("Direct3D device enumeration unavailable on this host.");
    }

    StdWin95SystemTest_SetStd3DStarted(true);
    if ( !std3D_Open(0u) )
    {
        TEST_IGNORE_MESSAGE("Direct3D device open unavailable on this host.");
    }
}

static void std3DX6SystemTest_RequireOpen3DDevice(void)
{
    std3DX6SystemTest_RequireOpen3DDeviceAtResolution(320u, 240u);
}

static bool std3DX6SystemTest_ValidateDisplayEnvironment(const StdDisplayEnvironment* pEnv)
{
    if ( !pEnv || !pEnv->numInfos || !pEnv->aDisplayInfos )
    {
        return false;
    }

    for ( size_t i = 0u; i < pEnv->numInfos; ++i )
    {
        const StdDisplayInfo* pInfo = &pEnv->aDisplayInfos[i];
        if ( !pInfo->numModes || !pInfo->aModes )
        {
            return false;
        }

        for ( size_t modeNum = 0u; modeNum < pInfo->numModes; ++modeNum )
        {
            const StdVideoMode* pMode = &pInfo->aModes[modeNum];
            if ( !pMode->rasterInfo.width || !pMode->rasterInfo.height || !pMode->rasterInfo.colorInfo.bpp )
            {
                return false;
            }
        }

        if ( pInfo->numDevices && !pInfo->aDevices )
        {
            return false;
        }

        for ( size_t deviceNum = 0u; deviceNum < pInfo->numDevices; ++deviceNum )
        {
            const Device3D* pDevice = &pInfo->aDevices[deviceNum];
            if ( pDevice->minTexWidth > pDevice->maxTexWidth
                || pDevice->minTexHeight > pDevice->maxTexHeight
                || !pDevice->maxVertexCount )
            {
                return false;
            }
        }
    }

    return true;
}

static void std3DX6SystemTest_AssertCubeVectorFrames(void)
{
    static const Std3DSystemTestCubeVectorMode aModes[] =
    {
        STD3D_SYSTEM_TEST_CUBE_VECTOR_VERTICES,
        STD3D_SYSTEM_TEST_CUBE_VECTOR_WIREFRAME,
        STD3D_SYSTEM_TEST_CUBE_VECTOR_SOLID,
        STD3D_SYSTEM_TEST_CUBE_VECTOR_TEXTURED,
        STD3D_SYSTEM_TEST_CUBE_VECTOR_TEXTURED_VERTEX_COLOR,
        STD3D_SYSTEM_TEST_CUBE_VECTOR_SOLID_VERTEX_INTENSITY,
    };

    for ( size_t modeNum = 0u; modeNum < STD_ARRAYLEN(aModes); ++modeNum )
    {
        for ( size_t frameNum = 0u; frameNum < STD3D_SYSTEM_TEST_CUBE_VECTOR_FRAME_COUNT; ++frameNum )
        {
            char aVectorName[128];

            snprintf(
                aVectorName,
                sizeof(aVectorName),
                "cube_%s_frame%02u_mask.bmp",
                Std3DSystemTest_GetCubeVectorModeName(aModes[modeNum]),
                (unsigned int)frameNum
            );
            Std3DSystemTest_RenderCubeVectorFrame(aModes[modeNum], frameNum);
            if ( aModes[modeNum] == STD3D_SYSTEM_TEST_CUBE_VECTOR_VERTICES
                || aModes[modeNum] == STD3D_SYSTEM_TEST_CUBE_VECTOR_WIREFRAME )
            {
                Std3DSystemTest_AssertBackBufferMatchesBmpProbeMask(aVectorName, 64u, 3u);
            }
            else
            {
                Std3DSystemTest_AssertBackBufferMatchesBmpMask(aVectorName, 96u);
            }
        }
    }
}

static void std3DX6SystemTest_AssertMipmapLodCubeFrames(void)
{
    for ( size_t frameNum = 0u; frameNum < STD3D_SYSTEM_TEST_FEATURE_CUBE_FRAME_COUNT; ++frameNum )
    {
        char aVectorName[128];

        snprintf(aVectorName, sizeof(aVectorName), "mipmap_lod_cube_frame%02u_mask.bmp", (unsigned int)frameNum);
        Std3DSystemTest_RenderHighQualityMipmapLodCubeFrame(frameNum);
        Std3DSystemTest_AssertBackBufferMatchesBmpMaskAtResolution(
            aVectorName,
            128u,
            STD3D_SYSTEM_TEST_HIGH_QUALITY_VECTOR_WIDTH,
            STD3D_SYSTEM_TEST_HIGH_QUALITY_VECTOR_HEIGHT
        );
    }
}

static tVBuffer* std3DX6SystemTest_CreateSolidVBuffer(uint32_t width, uint32_t height, const ColorInfo* pColorInfo, uint8_t red, uint8_t green, uint8_t blue)
{
    tRasterInfo rasterInfo;
    STD_ZEROMEM(&rasterInfo, sizeof(rasterInfo));
    rasterInfo.width     = width;
    rasterInfo.height    = height;
    rasterInfo.colorInfo = *pColorInfo;

    tVBuffer* pVBuffer = stdDisplay_VBufferNew(&rasterInfo, 0, 0);
    TEST_ASSERT_NOT_NULL(pVBuffer);

    uint32_t color = stdColor_EncodeRGB(pColorInfo, red, green, blue);
    TEST_ASSERT_TRUE(stdDisplay_VBufferFill(pVBuffer, color, NULL));
    return pVBuffer;
}

static void std3DX6SystemTest_AssertSurfacePixelNear(LPDIRECTDRAWSURFACE4 pSurface, const ColorInfo* pColorInfo, uint8_t expectedRed, uint8_t expectedGreen, uint8_t expectedBlue)
{
    DDSURFACEDESC2 desc;
    STD_ZEROMEM(&desc, sizeof(desc));
    desc.dwSize = sizeof(desc);
    TEST_ASSERT_EQUAL_HEX32(DD_OK, IDirectDrawSurface4_Lock(pSurface, NULL, &desc, DDLOCK_WAIT, NULL));

    size_t bytesPerPixel = pColorInfo->bpp / 8u;
    TEST_ASSERT_TRUE(bytesPerPixel > 0u && bytesPerPixel <= sizeof(uint32_t));

    uint32_t pixel = 0u;
    STD_COPYMEM(&pixel, desc.lpSurface, bytesPerPixel);
    TEST_ASSERT_EQUAL_HEX32(DD_OK, IDirectDrawSurface4_Unlock(pSurface, NULL));

    uint8_t red;
    uint8_t green;
    uint8_t blue;
    stdColor_DecodeRGB(pixel, pColorInfo, &red, &green, &blue);
    TEST_ASSERT_UINT8_WITHIN(8u, expectedRed, red);
    TEST_ASSERT_UINT8_WITHIN(8u, expectedGreen, green);
    TEST_ASSERT_UINT8_WITHIN(8u, expectedBlue, blue);
}

static void std3DX6SystemTest_ExerciseResizedMipChain(void)
{
    static const uint32_t aWidths[]  = { 8u, 4u, 2u, 1u };
    static const uint32_t aHeights[] = { 4u, 2u, 1u, 1u };
    static const uint8_t aColors[][3] =
    {
        { 255u, 0u,   0u   },
        { 0u,   255u, 0u   },
        { 0u,   0u,   255u },
        { 255u, 255u, 255u },
    };
    tVBuffer* apMipmaps[STD_ARRAYLEN(aWidths)];
    ColorInfo colorInfo;
    int bColorKeySet = 0;
    LPDDCOLORKEY pColorKey = NULL;

    STD_ZEROMEM(&colorInfo, sizeof(colorInfo));
    std3D_GetTextureFormat(STDCOLOR_FORMAT_RGB, &colorInfo, &bColorKeySet, &pColorKey);
    for ( size_t i = 0u; i < STD_ARRAYLEN(apMipmaps); ++i )
    {
        apMipmaps[i] = std3DX6SystemTest_CreateSolidVBuffer(
            aWidths[i],
            aHeights[i],
            &colorInfo,
            aColors[i][0],
            aColors[i][1],
            aColors[i][2]
        );
    }

    TEST_ASSERT_EQUAL_INT(0, std3D_SetMipmapFilter(STD3D_MIPMAPFILTER_BILINEAR));

    Device3D* pDevice = (Device3D*)std3D_GetCurrentDevice();
    TEST_ASSERT_NOT_NULL(pDevice);
    bool bSquareOnly = pDevice->bSqareOnlyTexture;
    pDevice->bSqareOnlyTexture = true;

    tSystemTexture texture;
    STD_ZEROMEM(&texture, sizeof(texture));
    std3D_AllocSystemTexture(&texture, apMipmaps, STD_ARRAYLEN(apMipmaps), STDCOLOR_FORMAT_RGB);
    pDevice->bSqareOnlyTexture = bSquareOnly;

    for ( size_t i = 0u; i < STD_ARRAYLEN(apMipmaps); ++i )
    {
        stdDisplay_VBufferFree(apMipmaps[i]);
    }

    TEST_ASSERT_NOT_NULL(texture.pTexture);
    TEST_ASSERT_EQUAL_size_t(STD_ARRAYLEN(apMipmaps), std3D_GetMipMapCount(&texture));

    LPDIRECTDRAWSURFACE4 pRoot = NULL;
    TEST_ASSERT_EQUAL_HEX32(
        DD_OK,
        IDirect3DTexture2_QueryInterface(texture.pTexture, &IID_IDirectDrawSurface4, &pRoot)
    );
    TEST_ASSERT_NOT_NULL(pRoot);

    LPDIRECTDRAWSURFACE4 pLevel = pRoot;
    for ( size_t i = 0u; i < STD_ARRAYLEN(apMipmaps); ++i )
    {
        DDSURFACEDESC2 desc;
        STD_ZEROMEM(&desc, sizeof(desc));
        desc.dwSize = sizeof(desc);
        TEST_ASSERT_EQUAL_HEX32(DD_OK, IDirectDrawSurface4_GetSurfaceDesc(pLevel, &desc));

        uint32_t expectedSize = 8u >> i;
        TEST_ASSERT_EQUAL_UINT32(expectedSize, desc.dwWidth);
        TEST_ASSERT_EQUAL_UINT32(expectedSize, desc.dwHeight);
        std3DX6SystemTest_AssertSurfacePixelNear(
            pLevel,
            &colorInfo,
            aColors[i][0],
            aColors[i][1],
            aColors[i][2]
        );

        if ( i + 1u < STD_ARRAYLEN(apMipmaps) )
        {
            DDSCAPS2 caps;
            STD_ZEROMEM(&caps, sizeof(caps));
            caps.dwCaps = DDSCAPS_MIPMAP | DDSCAPS_TEXTURE;

            LPDIRECTDRAWSURFACE4 pNextLevel = NULL;
            TEST_ASSERT_EQUAL_HEX32(
                DD_OK,
                IDirectDrawSurface4_GetAttachedSurface(pLevel, &caps, &pNextLevel)
            );
            if ( pLevel != pRoot )
            {
                IDirectDrawSurface4_Release(pLevel);
            }
            pLevel = pNextLevel;
        }
    }

    if ( pLevel != pRoot )
    {
        IDirectDrawSurface4_Release(pLevel);
    }
    IDirectDrawSurface4_Release(pRoot);
    std3D_ClearSystemTexture(&texture);
}

TEST(std3DX6System, TestStartupEnumeratesDevicesAfterWindowedDisplayMode)
{
    StdWin95SystemTest_RequireHiddenWindow();
    StdWin95SystemTest_RequireWindowedDisplayMode();

    if ( !std3D_Startup() )
    {
        TEST_IGNORE_MESSAGE("Direct3D device enumeration unavailable on this host.");
    }

    StdWin95SystemTest_SetStd3DStarted(true);
    TEST_ASSERT_TRUE(std3D_GetNumDevices() > 0u);

    const Device3D* aDevices = std3D_GetAllDevices();
    TEST_ASSERT_NOT_NULL(aDevices);

    for ( size_t i = 0u; i < std3D_GetNumDevices(); ++i )
    {
        TEST_ASSERT_TRUE(aDevices[i].minTexWidth <= aDevices[i].maxTexWidth);
        TEST_ASSERT_TRUE(aDevices[i].minTexHeight <= aDevices[i].maxTexHeight);
        TEST_ASSERT_TRUE(aDevices[i].maxVertexCount > 0u);
        TEST_ASSERT_TRUE(aDevices[i].totalMemory >= aDevices[i].availableMemory);
    }
}

TEST(std3DX6System, TestBuildDisplayEnvironmentEnumeratesDevicesAndModes)
{
    StdWin95SystemTest_RequireHiddenWindow();
    StdWin95SystemTest_RequireDisplayConfig();

    StdDisplayEnvironment* pEnv = std3D_BuildDisplayEnvironment();
    if ( !pEnv )
    {
        TEST_IGNORE_MESSAGE("Display environment unavailable on this host.");
    }

    bool bValid = std3DX6SystemTest_ValidateDisplayEnvironment(pEnv);
    std3D_FreeDisplayEnvironment(pEnv);

    TEST_ASSERT_TRUE(bValid);
    TEST_ASSERT_FALSE(stdDisplay_IsOpen());
}

TEST(std3DX6System, TestOpenSceneStateAndPrimitiveDraws)
{
    ColorInfo colorInfo;
    int bColorKeySet = 0;
    LPDDCOLORKEY pColorKey = NULL;

    std3DX6SystemTest_RequireOpen3DDevice();

    TEST_ASSERT_NOT_NULL(std3D_GetCurrentDevice());
    TEST_ASSERT_NOT_NULL(std3D_GetD3DDevice());
    TEST_ASSERT_TRUE(std3D_GetNumTextureFormats() > 0u);
    TEST_ASSERT_TRUE(std3D_GetNumTextureFormats() <= 8u);

    const Device3D* pDevice = std3D_GetCurrentDevice();
    bool bExpectedAnisotropic = (pDevice->d3dDesc.dpcTriCaps.dwRasterCaps & D3DPRASTERCAPS_ANISOTROPY) != 0
        && (pDevice->d3dDesc.dpcTriCaps.dwTextureFilterCaps & D3DPTFILTERCAPS_MINFANISOTROPIC) != 0
        && pDevice->d3dDesc.dwMaxAnisotropy > 1u;
    TEST_ASSERT_EQUAL_INT(bExpectedAnisotropic, pDevice->bAnisotropicFilteringSupported);
    TEST_ASSERT_EQUAL_INT(bExpectedAnisotropic, std3D_IsAnisotropicFilteringSupported());

    TEST_ASSERT_EQUAL_INT(0, std3D_SetProjection(1.04719758f, 0.1f, 1000.0f));
    TEST_ASSERT_EQUAL_INT(0, std3D_SetMipmapFilter(STD3D_MIPMAPFILTER_BILINEAR));

    std3D_GetTextureFormat(STDCOLOR_FORMAT_RGB, &colorInfo, &bColorKeySet, &pColorKey);
    TEST_ASSERT_TRUE(colorInfo.bpp > 0u);
    TEST_ASSERT_FALSE(bColorKeySet);
    TEST_ASSERT_NULL(pColorKey);

    std3D_EnableFog(1, 1.0f);
    std3D_SetFog(0.1f, 0.2f, 0.3f, 1.0f, 100.0f);
    std3D_ClearZBuffer();

    Std3DSystemTest_RenderPrimitiveHarnessScene();
    Std3DSystemTest_AssertBackBufferMatchesBmpMask("primitive_harness_mask.bmp", 12u);

    std3D_Close();
}

TEST(std3DX6System, TestTextureUploadZBufferAndRenderStates)
{
    std3DX6SystemTest_RequireOpen3DDevice();

    Std3DSystemTest_AssertValidDimensionsFollowDeviceCaps();
    Std3DSystemTest_ExerciseRenderStatePermutations();

    Std3DSystemTest_RenderSolidTextureQuadScene(255u, 255u, 0u);
    Std3DSystemTest_AssertBackBufferPixelNear(160u, 120u, 255u, 255u, 0u, 18u);
    Std3DSystemTest_AssertBackBufferMatchesBmpMask("texture_quad_mask.bmp", 18u);

    Std3DSystemTest_RenderMultiMipmapTextureQuadScene(255u, 128u, 0u);
    Std3DSystemTest_AssertBackBufferPixelNear(160u, 120u, 255u, 128u, 0u, 18u);
    Std3DSystemTest_AssertBackBufferMatchesBmpMask("texture_orange_quad_mask.bmp", 18u);

    Std3DSystemTest_RenderFormattedTextureQuadScene(STDCOLOR_FORMAT_RGBA, 128u, 64u, 255u, 255u);
    Std3DSystemTest_AssertBackBufferPixelNear(160u, 120u, 128u, 64u, 255u, 24u);
    Std3DSystemTest_AssertBackBufferMatchesBmpMask("texture_rgba_quad_mask.bmp", 24u);

    Std3DSystemTest_RenderFormattedTextureQuadScene(STDCOLOR_FORMAT_RGBA_1BITALPHA, 0u, 255u, 255u, 255u);
    Std3DSystemTest_AssertBackBufferPixelNear(160u, 120u, 0u, 255u, 255u, 24u);
    Std3DSystemTest_AssertBackBufferMatchesBmpMask("texture_rgba1_quad_mask.bmp", 24u);

    Std3DSystemTest_ExerciseTextureCacheResetAndReuse();
    Std3DSystemTest_AssertBackBufferPixelNear(160u, 120u, 255u, 128u, 0u, 18u);
    Std3DSystemTest_AssertBackBufferMatchesBmpMask("texture_orange_quad_mask.bmp", 18u);

    Std3DSystemTest_RenderTexturedDepthOcclusionScene();
    Std3DSystemTest_AssertBackBufferPixelNear(160u, 120u, 0u, 255u, 0u, 18u);
    Std3DSystemTest_AssertBackBufferPixelNear(112u, 112u, 255u, 255u, 0u, 18u);
    Std3DSystemTest_AssertBackBufferMatchesBmpMask("texture_depth_occlusion_mask.bmp", 24u);

    Std3DSystemTest_RenderZBufferOcclusionScene();
    Std3DSystemTest_AssertBackBufferPixelNear(160u, 120u, 0u, 255u, 0u, 18u);
    Std3DSystemTest_AssertBackBufferMatchesBmpMask("zbuffer_occlusion_mask.bmp", 18u);

    std3D_Close();
}

TEST(std3DX6System, TestAlphaBlendAlphaTestAndOneBitTransparency)
{
    std3DX6SystemTest_RequireOpen3DDevice();

    Std3DSystemTest_ExerciseAlphaAndTransparency();

    std3D_Close();
}

TEST(std3DX6System, TestTextureAddressFilteringZWriteAndStateIsolation)
{
    std3DX6SystemTest_RequireOpen3DDevice();

    Std3DSystemTest_ExerciseTextureAddressFilteringAndStateIsolation();

    std3D_Close();
}

TEST(std3DX6System, TestTextureCacheEvictsOldestEligibleEntry)
{
    std3DX6SystemTest_RequireOpen3DDevice();

    Std3DSystemTest_ExerciseTextureCacheEviction();

    std3D_Close();
}

TEST(std3DX6System, TestRectangularMipChainAndOddPitchTextureUpload)
{
    std3DX6SystemTest_RequireOpen3DDevice();

    Std3DSystemTest_ExerciseRectangularTextureUpload();

    std3D_Close();
}

TEST(std3DX6System, TestResizedMipChainPreservesEachSourceLevel)
{
    std3DX6SystemTest_RequireOpen3DDevice();

    std3DX6SystemTest_ExerciseResizedMipChain();

    std3D_Close();
}

TEST(std3DX6System, TestDepthWindingSharedEdgesAndProjectionBoundaries)
{
    std3DX6SystemTest_RequireOpen3DDevice();

    Std3DSystemTest_ExerciseDepthWindingAndProjectionBoundaries();

    std3D_Close();
}

TEST(std3DX6System, TestRepeated3DLifecycle)
{
    StdWin95SystemTest_RequireHiddenWindow();
    StdWin95SystemTest_RequireWindowedDisplayMode();

    Std3DSystemTest_ExerciseRepeated3DLifecycle();
}

TEST(std3DX6System, TestMipmapLodVectorUsesMipChain)
{
    std3DX6SystemTest_RequireOpen3DDevice();

    if ( !Std3DSystemTest_CurrentDeviceSupportsMipmaps() )
    {
        std3D_Close();
        TEST_IGNORE_MESSAGE("Mipmap filtering is unavailable on this host.");
    }

    Std3DSystemTest_RenderMipmapLodQuadScene();
    Std3DSystemTest_AssertBackBufferPixelNear(160u, 120u, 0u, 0u, 255u, 48u);
    Std3DSystemTest_AssertBackBufferMatchesBmpMask("mipmap_lod_mask.bmp", 48u);

    std3D_Close();
}

TEST(std3DX6System, TestMipmapLodCubeVectorUsesMipChain)
{
    std3DX6SystemTest_RequireOpen3DDeviceAtResolution(
        STD3D_SYSTEM_TEST_HIGH_QUALITY_VECTOR_WIDTH,
        STD3D_SYSTEM_TEST_HIGH_QUALITY_VECTOR_HEIGHT
    );

    if ( !Std3DSystemTest_CurrentDeviceSupportsMipmaps() )
    {
        std3D_Close();
        TEST_IGNORE_MESSAGE("Mipmap filtering is unavailable on this host.");
    }

    std3DX6SystemTest_AssertMipmapLodCubeFrames();

    std3D_Close();
}

TEST(std3DX6System, TestGeometryModesRenderCubeAndVertexColors)
{
    std3DX6SystemTest_RequireOpen3DDevice();

    Std3DSystemTest_RenderVertexGeometryCubeScene();
    Std3DSystemTest_AssertBackBufferAnyPixelNear(98u, 76u, 5u, 5u, 255u, 255u, 255u, 24u);
    Std3DSystemTest_AssertBackBufferAnyPixelNear(208u, 178u, 5u, 5u, 255u, 255u, 255u, 24u);
    Std3DSystemTest_AssertBackBufferPixelNear(20u, 20u, 0u, 0u, 0u, 8u);

    Std3DSystemTest_RenderWireframeGeometryCubeScene();
    Std3DSystemTest_AssertBackBufferAnyPixelNear(158u, 86u, 7u, 7u, 255u, 255u, 255u, 24u);
    Std3DSystemTest_AssertBackBufferAnyPixelNear(56u, 86u, 7u, 7u, 255u, 255u, 255u, 24u);
    Std3DSystemTest_AssertBackBufferAnyPixelNear(80u, 62u, 7u, 7u, 255u, 255u, 255u, 24u);
    Std3DSystemTest_AssertBackBufferPixelNear(20u, 20u, 0u, 0u, 0u, 8u);

    Std3DSystemTest_RenderSolidGeometryCubeScene();
    Std3DSystemTest_AssertBackBufferPixelNear(145u, 74u, 255u, 0u, 0u, 24u);
    Std3DSystemTest_AssertBackBufferPixelNear(78u, 104u, 0u, 0u, 255u, 24u);
    Std3DSystemTest_AssertBackBufferPixelNear(156u, 130u, 0u, 255u, 0u, 24u);

    Std3DSystemTest_RenderTexturedGeometryCubeScene();
    Std3DSystemTest_AssertBackBufferPixelNear(125u, 105u, 255u, 0u, 0u, 48u);
    Std3DSystemTest_AssertBackBufferPixelNear(190u, 116u, 0u, 255u, 0u, 48u);
    Std3DSystemTest_AssertBackBufferPixelNear(120u, 145u, 0u, 0u, 255u, 48u);
    Std3DSystemTest_AssertBackBufferPixelNear(185u, 158u, 255u, 255u, 0u, 48u);

    std3D_Close();
}

TEST(std3DX6System, TestCubeVectorFramesMatchMasks)
{
    std3DX6SystemTest_RequireOpen3DDevice();

    std3DX6SystemTest_AssertCubeVectorFrames();

    std3D_Close();
}

TEST(std3DX6System, TestFogRenderingTintsFarGeometryWhenSupported)
{
    std3DX6SystemTest_RequireOpen3DDevice();

    if ( !Std3DSystemTest_CurrentDeviceSupportsFog() )
    {
        std3D_Close();
        TEST_IGNORE_MESSAGE("Direct3D fog rendering is unavailable on this host.");
    }

    Std3DSystemTest_RenderFogGeometryScene();
    Std3DSystemTest_AssertBackBufferPixelInRange(104u, 120u, 0u, 96u, 160u, 255u, 0u, 96u);
    Std3DSystemTest_AssertBackBufferPixelInRange(216u, 120u, 160u, 255u, 0u, 128u, 0u, 96u);

    std3D_Close();
}

TEST_GROUP_RUNNER(std3DX6System)
{
    RUN_TEST_CASE(std3DX6System, TestStartupEnumeratesDevicesAfterWindowedDisplayMode);
    RUN_TEST_CASE(std3DX6System, TestBuildDisplayEnvironmentEnumeratesDevicesAndModes);
    RUN_TEST_CASE(std3DX6System, TestOpenSceneStateAndPrimitiveDraws);
    RUN_TEST_CASE(std3DX6System, TestTextureUploadZBufferAndRenderStates);
    RUN_TEST_CASE(std3DX6System, TestAlphaBlendAlphaTestAndOneBitTransparency);
    RUN_TEST_CASE(std3DX6System, TestTextureAddressFilteringZWriteAndStateIsolation);
    RUN_TEST_CASE(std3DX6System, TestTextureCacheEvictsOldestEligibleEntry);
    RUN_TEST_CASE(std3DX6System, TestRectangularMipChainAndOddPitchTextureUpload);
    RUN_TEST_CASE(std3DX6System, TestResizedMipChainPreservesEachSourceLevel);
    RUN_TEST_CASE(std3DX6System, TestDepthWindingSharedEdgesAndProjectionBoundaries);
    RUN_TEST_CASE(std3DX6System, TestRepeated3DLifecycle);
    RUN_TEST_CASE(std3DX6System, TestMipmapLodVectorUsesMipChain);
    RUN_TEST_CASE(std3DX6System, TestMipmapLodCubeVectorUsesMipChain);
    RUN_TEST_CASE(std3DX6System, TestGeometryModesRenderCubeAndVertexColors);
    RUN_TEST_CASE(std3DX6System, TestCubeVectorFramesMatchMasks);
    RUN_TEST_CASE(std3DX6System, TestFogRenderingTintsFarGeometryWhenSupported);
}

#include <unity_fixture.h>

#include <stdio.h>

#include <std/General/std.h>
#include <std/General/stdColor.h>
#include <std/General/stdConfig.h>
#include <std/General/stdUtil.h>
#include <std/Win95/std3D.h>
#include <std/Win95/stdDisplay.h>

#include "std3DSystemTestSupport.h"
#include "stdWin95SystemTestSupport.h"

#define STD3DX9SYSTEMTEST_MAX_MSAA_SAMPLES 16

TEST_GROUP(std3DX9System);

static const D3DFORMAT std3DX9SystemTest_aDepthFormats[] =
{
    D3DFMT_D24S8,
    D3DFMT_D24X4S4,
    D3DFMT_D24X8,
    D3DFMT_D32,
    D3DFMT_D15S1,
    D3DFMT_D16
};

TEST_SETUP(std3DX9System)
{
    StdWin95SystemTest_Startup();
}

TEST_TEAR_DOWN(std3DX9System)
{
    StdWin95SystemTest_Shutdown();
}

static void std3DX9SystemTest_RequireOpen3DDevice(void)
{
    StdWin95SystemTest_RequireHiddenWindow();
    StdWin95SystemTest_RequireWindowedDisplayMode();

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

static void std3DX9SystemTest_RequireOpen3DDeviceWithAnisotropicConfig(bool bEnabled)
{
    StdWin95SystemTest_RequireHiddenWindow();
    StdWin95SystemTest_RequireDisplayConfig();

    TEST_ASSERT_TRUE(stdConfig_SetBool(STD3D_CFG_ANISOTROPICFILTER, bEnabled));
    StdWin95SystemTest_RequireWindowedDisplayMode();

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

static void std3DX9SystemTest_RequireOpen3DDeviceWithMipmapAutoGen(void)
{
    StdWin95SystemTest_RequireHiddenWindow();
    StdWin95SystemTest_RequireDisplayConfig();

    TEST_ASSERT_TRUE(stdConfig_SetBool(STD3D_CFG_MIPMAPAUTOGEN, true));
    TEST_ASSERT_TRUE(stdConfig_SetBool(STD3D_CFG_ANISOTROPICFILTER, false));
    StdWin95SystemTest_RequireWindowedDisplayModeAtResolution(
        STD3D_SYSTEM_TEST_HIGH_QUALITY_VECTOR_WIDTH,
        STD3D_SYSTEM_TEST_HIGH_QUALITY_VECTOR_HEIGHT
    );

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

static bool std3DX9SystemTest_FindWindowedDepthFormat(UINT* pAdapter, D3DDISPLAYMODE* pDesktopMode, D3DFORMAT* pDepthFormat)
{
    LPDIRECT3D9 pD3D = stdDisplay_GetDirect3D();
    StdDisplayDevice displayDevice;
    if ( !pD3D || stdDisplay_GetCurrentDevice(&displayDevice) )
    {
        return false;
    }

    UINT adapter = displayDevice.caps.AdapterOrdinal;
    if ( FAILED(IDirect3D9_GetAdapterDisplayMode(pD3D, adapter, pDesktopMode)) )
    {
        return false;
    }

    for ( size_t i = 0u; i < STD_ARRAYLEN(std3DX9SystemTest_aDepthFormats); ++i )
    {
        D3DFORMAT depthFormat = std3DX9SystemTest_aDepthFormats[i];
        if ( SUCCEEDED(IDirect3D9_CheckDeviceFormat(
                pD3D,
                adapter,
                D3DDEVTYPE_HAL,
                pDesktopMode->Format,
                D3DUSAGE_DEPTHSTENCIL,
                D3DRTYPE_SURFACE,
                depthFormat
            ))
            && SUCCEEDED(IDirect3D9_CheckDepthStencilMatch(
                pD3D,
                adapter,
                D3DDEVTYPE_HAL,
                pDesktopMode->Format,
                pDesktopMode->Format,
                depthFormat
            )) )
        {
            *pAdapter     = adapter;
            *pDepthFormat = depthFormat;
            return true;
        }
    }

    return false;
}

static bool std3DX9SystemTest_IsWindowedMSAASupported(D3DMULTISAMPLE_TYPE sampleType)
{
    LPDIRECT3D9 pD3D = stdDisplay_GetDirect3D();
    if ( !pD3D )
    {
        return false;
    }

    UINT adapter;
    D3DDISPLAYMODE desktopMode;
    D3DFORMAT depthFormat;
    if ( !std3DX9SystemTest_FindWindowedDepthFormat(&adapter, &desktopMode, &depthFormat) )
    {
        return false;
    }

    DWORD colorQualityLevels = 0u;
    HRESULT colorResult = IDirect3D9_CheckDeviceMultiSampleType(
        pD3D,
        adapter,
        D3DDEVTYPE_HAL,
        desktopMode.Format,
        TRUE,
        sampleType,
        &colorQualityLevels
    );

    DWORD depthQualityLevels = 0u;
    HRESULT depthResult = IDirect3D9_CheckDeviceMultiSampleType(
        pD3D,
        adapter,
        D3DDEVTYPE_HAL,
        depthFormat,
        TRUE,
        sampleType,
        &depthQualityLevels
    );

    return SUCCEEDED(colorResult)
        && SUCCEEDED(depthResult)
        && colorQualityLevels > 0u
        && depthQualityLevels > 0u;
}

static D3DMULTISAMPLE_TYPE std3DX9SystemTest_GetHighestWindowedMSAA(void)
{
    static const D3DMULTISAMPLE_TYPE aSampleTypes[] =
    {
        D3DMULTISAMPLE_16_SAMPLES,
        D3DMULTISAMPLE_8_SAMPLES,
        D3DMULTISAMPLE_4_SAMPLES,
        D3DMULTISAMPLE_2_SAMPLES,
    };

    for ( size_t i = 0u; i < STD_ARRAYLEN(aSampleTypes); ++i )
    {
        if ( std3DX9SystemTest_IsWindowedMSAASupported(aSampleTypes[i]) )
        {
            return aSampleTypes[i];
        }
    }

    return D3DMULTISAMPLE_NONE;
}

static void std3DX9SystemTest_RequireOpen3DDeviceWithMSAA(uint32_t width, uint32_t height, bool bAnisotropic, bool bMipmapAutoGen)
{
    StdWin95SystemTest_RequireHiddenWindow();
    StdWin95SystemTest_RequireDisplayConfig();

    TEST_ASSERT_TRUE(stdConfig_SetBool(STD3D_CFG_MSAAENABLED, true));
    TEST_ASSERT_TRUE(stdConfig_SetInt(STD3D_CFG_MSAASAMPLES, STD3DX9SYSTEMTEST_MAX_MSAA_SAMPLES));
    TEST_ASSERT_TRUE(stdConfig_SetBool(STD3D_CFG_ANISOTROPICFILTER, bAnisotropic));
    TEST_ASSERT_TRUE(stdConfig_SetBool(STD3D_CFG_MIPMAPAUTOGEN, bMipmapAutoGen));
    StdWin95SystemTest_RequireDisplayStartup();

    D3DMULTISAMPLE_TYPE expectedSampleType = std3DX9SystemTest_GetHighestWindowedMSAA();
    if ( expectedSampleType == D3DMULTISAMPLE_NONE )
    {
        TEST_IGNORE_MESSAGE("Windowed MSAA is unavailable on this host.");
    }

    if ( stdDisplay_GetNumDevices() == 0u )
    {
        TEST_IGNORE_MESSAGE("No DirectX display devices were enumerated on this host.");
    }

    if ( !stdDisplay_Open(0u) )
    {
        TEST_IGNORE_MESSAGE("DirectX display device 0 could not be opened on this host.");
    }

    StdWin95SystemTest_SetDisplayOpen(true);
    stdDisplay_SetDefaultResolution(width, height);
    if ( stdDisplay_SetMode(0u, 0, 1u) )
    {
        TEST_IGNORE_MESSAGE("Windowed DirectX MSAA display mode unavailable on this host.");
    }

    if ( !std3D_Startup() )
    {
        TEST_IGNORE_MESSAGE("Direct3D device enumeration unavailable on this host.");
    }

    StdWin95SystemTest_SetStd3DStarted(true);
    if ( !std3D_Open(0u) )
    {
        TEST_IGNORE_MESSAGE("Direct3D device open unavailable on this host.");
    }

    D3DMULTISAMPLE_TYPE sampleType = stdDisplay_g_backBuffer.surface.desc.MultiSampleType;
    TEST_ASSERT_EQUAL_INT((int)expectedSampleType, (int)sampleType);
    TEST_ASSERT_EQUAL_UINT32(width, stdDisplay_g_backBuffer.surface.desc.Width);
    TEST_ASSERT_EQUAL_UINT32(height, stdDisplay_g_backBuffer.surface.desc.Height);
    TEST_ASSERT_EQUAL_INT((int)sampleType, stdConfig_GetInt(STD3D_CFG_MSAASAMPLES, 0));

    printf(" using %dx MSAA at %ux%u.\n", (int)sampleType, width, height);
}

static bool std3DX9SystemTest_ValidateDisplayEnvironment(const StdDisplayEnvironment* pEnv)
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

static void std3DX9SystemTest_AssertCubeVectorFrames(void)
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

            STD_FORMAT(
                aVectorName,
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

static void std3DX9SystemTest_AssertMipmapLodCubeFrames(void)
{
    for ( size_t frameNum = 0u; frameNum < STD3D_SYSTEM_TEST_FEATURE_CUBE_FRAME_COUNT; ++frameNum )
    {
        char aVectorName[128];
        STD_FORMAT(aVectorName, "mipmap_lod_cube_frame%02u_mask.bmp", (unsigned int)frameNum);
        Std3DSystemTest_RenderMipmapLodCubeFrame(frameNum);
        Std3DSystemTest_AssertBackBufferMatchesBmpMask(aVectorName, 128u);
    }
}

static void std3DX9SystemTest_AssertMipmapAutoGenCubeFrames(void)
{
    for ( size_t frameNum = 0u; frameNum < STD3D_SYSTEM_TEST_FEATURE_CUBE_FRAME_COUNT; ++frameNum )
    {
        char aVectorName[128];

        STD_FORMAT(aVectorName, "mipmap_autogen_cube_frame%02u_mask.bmp", (unsigned int)frameNum);
        Std3DSystemTest_RenderMipmapAutoGenCubeFrame(frameNum);
        Std3DSystemTest_AssertBackBufferMatchesBmpMaskAtResolution(
            aVectorName,
            128u,
            STD3D_SYSTEM_TEST_HIGH_QUALITY_VECTOR_WIDTH,
            STD3D_SYSTEM_TEST_HIGH_QUALITY_VECTOR_HEIGHT
        );
    }
}

static void std3DX9SystemTest_AssertMSAACubeVectorFrames(void)
{
    static const Std3DSystemTestCubeVectorMode aModes[] =
    {
        STD3D_SYSTEM_TEST_CUBE_VECTOR_WIREFRAME,
        STD3D_SYSTEM_TEST_CUBE_VECTOR_SOLID,
        STD3D_SYSTEM_TEST_CUBE_VECTOR_TEXTURED,
        STD3D_SYSTEM_TEST_CUBE_VECTOR_TEXTURED_VERTEX_COLOR,
        STD3D_SYSTEM_TEST_CUBE_VECTOR_SOLID_VERTEX_INTENSITY,
    };

    for ( size_t modeNum = 0u; modeNum < STD_ARRAYLEN(aModes); ++modeNum )
    {
        for ( size_t frameNum = 0u; frameNum < STD3D_SYSTEM_TEST_FEATURE_CUBE_FRAME_COUNT; ++frameNum )
        {
            char aVectorName[128];

            STD_FORMAT(
                aVectorName,
                "msaa_%s_cube_frame%02u_mask.bmp",
                Std3DSystemTest_GetCubeVectorModeName(aModes[modeNum]),
                (unsigned int)frameNum
            );
            Std3DSystemTest_RenderMSAACubeVectorFrame(aModes[modeNum], frameNum);
            if ( aModes[modeNum] == STD3D_SYSTEM_TEST_CUBE_VECTOR_WIREFRAME )
            {
                Std3DSystemTest_AssertBackBufferMatchesBmpProbeMask(aVectorName, 96u, 4u);
            }
            else
            {
                Std3DSystemTest_AssertBackBufferMatchesBmpMask(aVectorName, 128u);
            }
        }
    }
}

static void std3DX9SystemTest_AssertMipmapMSAAAnisotropicCubeFrames(void)
{
    for ( size_t frameNum = 0u; frameNum < STD3D_SYSTEM_TEST_FEATURE_CUBE_FRAME_COUNT; ++frameNum )
    {
        char aVectorName[128];

        STD_FORMAT(aVectorName, "mipmap_msaa_anisotropic_cube_frame%02u_mask.bmp", (unsigned int)frameNum);
        Std3DSystemTest_RenderMipmapAnisotropicCubeFrame(frameNum);
        Std3DSystemTest_AssertBackBufferMatchesBmpMaskAtResolution(
            aVectorName,
            128u,
            STD3D_SYSTEM_TEST_HIGH_QUALITY_VECTOR_WIDTH,
            STD3D_SYSTEM_TEST_HIGH_QUALITY_VECTOR_HEIGHT
        );
    }
}

static void std3DX9SystemTest_AssertMipmapAutoGenMSAAAnisotropicCubeFrames(void)
{
    for ( size_t frameNum = 0u; frameNum < STD3D_SYSTEM_TEST_FEATURE_CUBE_FRAME_COUNT; ++frameNum )
    {
        char aVectorName[128];

        STD_FORMAT(aVectorName, "mipmap_autogen_msaa_anisotropic_cube_frame%02u_mask.bmp", (unsigned int)frameNum);
        Std3DSystemTest_RenderMipmapAutoGenAnisotropicCubeFrame(frameNum);
        Std3DSystemTest_AssertBackBufferMatchesBmpMaskAtResolution(
            aVectorName,
            128u,
            STD3D_SYSTEM_TEST_HIGH_QUALITY_VECTOR_WIDTH,
            STD3D_SYSTEM_TEST_HIGH_QUALITY_VECTOR_HEIGHT
        );
    }
}

static void std3DX9SystemTest_AssertAnisotropicMipmapSamplerState(const Device3D* pDevice)
{
    DWORD minFilter = 0u;
    TEST_ASSERT_EQUAL_HEX32(D3D_OK, IDirect3DDevice9_GetSamplerState(std3D_GetD3DDevice(), 0, D3DSAMP_MINFILTER, &minFilter));
    TEST_ASSERT_EQUAL_UINT32(D3DTEXF_ANISOTROPIC, minFilter);

    DWORD mipFilter         = 0u;
    DWORD expectedMipFilter = (pDevice->d3dDesc.TextureFilterCaps & D3DPTFILTERCAPS_MIPFLINEAR) != 0
        ? D3DTEXF_LINEAR
        : D3DTEXF_POINT;
    TEST_ASSERT_EQUAL_HEX32(D3D_OK, IDirect3DDevice9_GetSamplerState(std3D_GetD3DDevice(), 0, D3DSAMP_MIPFILTER, &mipFilter));
    TEST_ASSERT_EQUAL_UINT32(expectedMipFilter, mipFilter);

    DWORD maxAnisotropy = 0u;
    TEST_ASSERT_EQUAL_HEX32(D3D_OK, IDirect3DDevice9_GetSamplerState(std3D_GetD3DDevice(), 0, D3DSAMP_MAXANISOTROPY, &maxAnisotropy));
    TEST_ASSERT_EQUAL_UINT32(pDevice->d3dDesc.MaxAnisotropy, maxAnisotropy);
}

TEST(std3DX9System, TestStartupEnumeratesDevicesAfterWindowedDisplayMode)
{
    StdWin95SystemTest_RequireHiddenWindow();
    StdWin95SystemTest_RequireWindowedDisplayMode();

    if ( !std3D_Startup() )
    {
        TEST_IGNORE_MESSAGE("Direct3D device enumeration unavailable on this host.");
    }

    StdWin95SystemTest_SetStd3DStarted(true);
    TEST_ASSERT_EQUAL_size_t(1u, std3D_GetNumDevices());

    const Device3D* aDevices = std3D_GetAllDevices();
    TEST_ASSERT_NOT_NULL(aDevices);

    StdDisplayDevice displayDevice;
    TEST_ASSERT_EQUAL_INT(0, stdDisplay_GetCurrentDevice(&displayDevice));
    TEST_ASSERT_EQUAL_UINT32(displayDevice.caps.AdapterOrdinal, aDevices[0].d3dDesc.AdapterOrdinal);
    TEST_ASSERT_TRUE(aDevices[0].hasZBuffer);

    for ( size_t i = 0u; i < std3D_GetNumDevices(); ++i )
    {
        TEST_ASSERT_TRUE(aDevices[i].minTexWidth <= aDevices[i].maxTexWidth);
        TEST_ASSERT_TRUE(aDevices[i].minTexHeight <= aDevices[i].maxTexHeight);
        TEST_ASSERT_TRUE(aDevices[i].maxVertexCount > 0u);
        TEST_ASSERT_TRUE(aDevices[i].totalMemory >= aDevices[i].availableMemory);
    }
}

TEST(std3DX9System, TestBuildDisplayEnvironmentEnumeratesDevicesAndModes)
{
    StdWin95SystemTest_RequireHiddenWindow();
    StdWin95SystemTest_RequireDisplayConfig();

    StdDisplayEnvironment* pEnv = std3D_BuildDisplayEnvironment();
    if ( !pEnv )
    {
        TEST_IGNORE_MESSAGE("Display environment unavailable on this host.");
    }

    bool bValid = std3DX9SystemTest_ValidateDisplayEnvironment(pEnv);
    std3D_FreeDisplayEnvironment(pEnv);

    TEST_ASSERT_TRUE(bValid);
    TEST_ASSERT_FALSE(stdDisplay_IsOpen());
}

TEST(std3DX9System, TestOpenSceneStateAndPrimitiveDraws)
{
    ColorInfo colorInfo;
    int bColorKeySet = 0;
    LPDDCOLORKEY pColorKey = NULL;

    std3DX9SystemTest_RequireOpen3DDevice();

    TEST_ASSERT_NOT_NULL(std3D_GetCurrentDevice());
    TEST_ASSERT_NOT_NULL(std3D_GetD3DDevice());
    TEST_ASSERT_TRUE(std3D_GetNumTextureFormats() > 0u);

    LPDIRECT3DSURFACE9 pDepthSurface = NULL;
    TEST_ASSERT_EQUAL_HEX32(D3D_OK, IDirect3DDevice9_GetDepthStencilSurface(std3D_GetD3DDevice(), &pDepthSurface));
    TEST_ASSERT_NOT_NULL(pDepthSurface);

    D3DSURFACE_DESC depthDesc;
    STD_ZEROMEM(&depthDesc, sizeof(depthDesc));
    TEST_ASSERT_EQUAL_HEX32(D3D_OK, IDirect3DSurface9_GetDesc(pDepthSurface, &depthDesc));
    IDirect3DSurface9_Release(pDepthSurface);

    bool bKnownDepthFormat = false;
    for ( size_t i = 0u; i < STD_ARRAYLEN(std3DX9SystemTest_aDepthFormats); ++i )
    {
        bKnownDepthFormat = bKnownDepthFormat || depthDesc.Format == std3DX9SystemTest_aDepthFormats[i];
    }
    TEST_ASSERT_TRUE(bKnownDepthFormat);

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

    std3D_TestSetUseBuffers(true);
    Std3DSystemTest_RenderPrimitiveHarnessScene();
    std3D_TestSetUseBuffers(false);
    Std3DSystemTest_AssertBackBufferAnyPixelNear(40u, 40u, 2u, 2u, 255u, 0u, 0u, 24u);
    Std3DSystemTest_AssertBackBufferAnyPixelNear(80u, 210u, 2u, 2u, 0u, 0u, 255u, 24u);

    std3D_Close();
}

TEST(std3DX9System, TestAnisotropicConfigEnabledSetsSupportedSamplerState)
{
    std3DX9SystemTest_RequireOpen3DDeviceWithAnisotropicConfig(true);

    const Device3D* pDevice = std3D_GetCurrentDevice();
    TEST_ASSERT_NOT_NULL(pDevice);
    if ( !pDevice->bAnisotropicFilteringSupported
        || (pDevice->d3dDesc.TextureFilterCaps & D3DPTFILTERCAPS_MINFANISOTROPIC) == 0 )
    {
        std3D_Close();
        TEST_IGNORE_MESSAGE("Anisotropic texture filtering is unavailable on this host.");
    }

    DWORD maxAnisotropy = 0u;
    TEST_ASSERT_EQUAL_HEX32(D3D_OK, IDirect3DDevice9_GetSamplerState(std3D_GetD3DDevice(), 0, D3DSAMP_MAXANISOTROPY, &maxAnisotropy));
    TEST_ASSERT_EQUAL_UINT32(pDevice->d3dDesc.MaxAnisotropy, maxAnisotropy);

    std3D_SetRenderState(STD3D_RS_TEXFILTER_BILINEAR);
    std3D_SetRenderState(STD3D_RS_TEXFILTER_ANISOTROPIC);

    DWORD minFilter = 0u;
    TEST_ASSERT_EQUAL_HEX32(D3D_OK, IDirect3DDevice9_GetSamplerState(std3D_GetD3DDevice(), 0, D3DSAMP_MINFILTER, &minFilter));
    TEST_ASSERT_EQUAL_UINT32(D3DTEXF_ANISOTROPIC, minFilter);

    std3D_Close();
}

TEST(std3DX9System, TestAnisotropicConfigDisabledKeepsNonAnisotropicSamplerState)
{
    std3DX9SystemTest_RequireOpen3DDeviceWithAnisotropicConfig(false);

    std3D_SetRenderState(STD3D_RS_TEXFILTER_ANISOTROPIC);

    DWORD minFilter = 0u;
    TEST_ASSERT_EQUAL_HEX32(D3D_OK, IDirect3DDevice9_GetSamplerState(std3D_GetD3DDevice(), 0, D3DSAMP_MINFILTER, &minFilter));
    TEST_ASSERT_NOT_EQUAL_UINT32(D3DTEXF_ANISOTROPIC, minFilter);

    std3D_Close();
}

TEST(std3DX9System, TestEachEnumerated3DDeviceOpensWithDefaultConfig)
{
    StdWin95SystemTest_RequireHiddenWindow();
    StdWin95SystemTest_RequireDisplayConfig();

    TEST_ASSERT_TRUE(stdConfig_SetBool(STD3D_CFG_ANISOTROPICFILTER, false));
    StdWin95SystemTest_RequireWindowedDisplayMode();

    if ( !std3D_Startup() )
    {
        TEST_IGNORE_MESSAGE("Direct3D device enumeration unavailable on this host.");
    }

    StdWin95SystemTest_SetStd3DStarted(true);

    size_t numOpenedDevices = 0u;
    for ( size_t deviceNum = 0u; deviceNum < std3D_GetNumDevices(); ++deviceNum )
    {
        if ( !std3D_Open(deviceNum) )
        {
            continue;
        }

        const Device3D* pDevice = std3D_GetCurrentDevice();
        TEST_ASSERT_NOT_NULL(pDevice);
        TEST_ASSERT_TRUE(pDevice->minTexWidth <= pDevice->maxTexWidth);
        TEST_ASSERT_TRUE(pDevice->minTexHeight <= pDevice->maxTexHeight);
        TEST_ASSERT_EQUAL_INT(0, std3D_SetProjection(1.04719758f, 0.1f, 1000.0f));

        bool bExpectedAnisotropic = (pDevice->d3dDesc.RasterCaps & D3DPRASTERCAPS_ANISOTROPY) != 0
            && (pDevice->d3dDesc.TextureFilterCaps & D3DPTFILTERCAPS_MINFANISOTROPIC) != 0
            && pDevice->d3dDesc.MaxAnisotropy > 1;
        TEST_ASSERT_EQUAL_INT(bExpectedAnisotropic, pDevice->bAnisotropicFilteringSupported);
        TEST_ASSERT_EQUAL_INT(pDevice->bAnisotropicFilteringSupported, std3D_IsAnisotropicFilteringSupported());
        TEST_ASSERT_EQUAL_INT(pDevice->bMipmapAutoGenSupported, std3D_IsMipmapAutoGenSupported());
        TEST_ASSERT_EQUAL_INT(pDevice->bMSAASupported, std3D_IsMSAASupported());

        std3D_Close();
        ++numOpenedDevices;
    }

    TEST_ASSERT_TRUE(numOpenedDevices > 0u);
}

TEST(std3DX9System, TestTextureUploadZBufferAndRenderStates)
{
    std3DX9SystemTest_RequireOpen3DDevice();

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

TEST(std3DX9System, TestAlphaBlendAlphaTestAndOneBitTransparency)
{
    std3DX9SystemTest_RequireOpen3DDevice();

    Std3DSystemTest_ExerciseAlphaAndTransparency();

    std3D_Close();
}

TEST(std3DX9System, TestTextureAddressFilteringZWriteAndStateIsolation)
{
    std3DX9SystemTest_RequireOpen3DDevice();

    Std3DSystemTest_ExerciseTextureAddressFilteringAndStateIsolation();

    std3D_Close();
}

TEST(std3DX9System, TestTextureCacheEvictsOldestEligibleEntry)
{
    std3DX9SystemTest_RequireOpen3DDevice();

    Std3DSystemTest_ExerciseTextureCacheEviction();

    std3D_Close();
}

TEST(std3DX9System, TestRectangularMipChainAndOddPitchTextureUpload)
{
    std3DX9SystemTest_RequireOpen3DDevice();

    Std3DSystemTest_ExerciseRectangularTextureUpload();

    std3D_Close();
}

TEST(std3DX9System, TestDepthWindingSharedEdgesAndProjectionBoundaries)
{
    std3DX9SystemTest_RequireOpen3DDevice();

    Std3DSystemTest_ExerciseDepthWindingAndProjectionBoundaries();

    std3D_Close();
}

TEST(std3DX9System, TestDeviceResetRecreates3DResources)
{
    std3DX9SystemTest_RequireOpen3DDevice();

    Std3DSystemTest_ExerciseDeviceResetRecovery();

    std3D_Close();
}

TEST(std3DX9System, TestDynamicVertexAndIndexBuffersWrapWithoutStaleData)
{
    std3DX9SystemTest_RequireOpen3DDevice();

    Std3DSystemTest_ExerciseDynamicBufferWraparound();

    std3D_Close();
}

TEST(std3DX9System, TestRepeated3DLifecycle)
{
    StdWin95SystemTest_RequireHiddenWindow();
    StdWin95SystemTest_RequireWindowedDisplayMode();

    Std3DSystemTest_ExerciseRepeated3DLifecycle();
}

TEST(std3DX9System, TestMipmapLodVectorUsesMipChain)
{
    std3DX9SystemTest_RequireOpen3DDevice();

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

TEST(std3DX9System, TestMipmapLodCubeVectorUsesMipChain)
{
    std3DX9SystemTest_RequireOpen3DDevice();

    if ( !Std3DSystemTest_CurrentDeviceSupportsMipmaps() )
    {
        std3D_Close();
        TEST_IGNORE_MESSAGE("Mipmap filtering is unavailable on this host.");
    }

    std3DX9SystemTest_AssertMipmapLodCubeFrames();

    std3D_Close();
}

TEST(std3DX9System, TestMipmapAutoGenCubeVectorsWhenSupported)
{
    std3DX9SystemTest_RequireOpen3DDeviceWithMipmapAutoGen();

    const Device3D* pDevice = std3D_GetCurrentDevice();
    TEST_ASSERT_NOT_NULL(pDevice);
    if ( !pDevice->bMipmapAutoGenSupported )
    {
        std3D_Close();
        TEST_IGNORE_MESSAGE("Automatic mipmap generation is unavailable on this host.");
    }

    TEST_ASSERT_TRUE(stdConfig_GetBool(STD3D_CFG_MIPMAPAUTOGEN, false));
    std3DX9SystemTest_AssertMipmapAutoGenCubeFrames();

    DWORD mipFilter         = 0u;
    DWORD expectedMipFilter = (pDevice->d3dDesc.TextureFilterCaps & D3DPTFILTERCAPS_MIPFLINEAR) != 0
        ? D3DTEXF_LINEAR
        : D3DTEXF_POINT;
    TEST_ASSERT_EQUAL_HEX32(D3D_OK, IDirect3DDevice9_GetSamplerState(std3D_GetD3DDevice(), 0, D3DSAMP_MIPFILTER, &mipFilter));
    TEST_ASSERT_EQUAL_UINT32(expectedMipFilter, mipFilter);

    std3D_Close();
}

TEST(std3DX9System, TestAnisotropicTextureFilterVectorWhenSupported)
{
    std3DX9SystemTest_RequireOpen3DDeviceWithAnisotropicConfig(true);

    const Device3D* pDevice = std3D_GetCurrentDevice();
    TEST_ASSERT_NOT_NULL(pDevice);
    if ( !pDevice->bAnisotropicFilteringSupported
        || (pDevice->d3dDesc.TextureFilterCaps & D3DPTFILTERCAPS_MINFANISOTROPIC) == 0 )
    {
        std3D_Close();
        TEST_IGNORE_MESSAGE("Anisotropic texture filtering is unavailable on this host.");
    }

    Std3DSystemTest_RenderAnisotropicTextureFilterScene();
    Std3DSystemTest_AssertBackBufferMatchesBmpMask("anisotropic_texture_filter_mask.bmp", 128u);

    DWORD minFilter = 0u;
    TEST_ASSERT_EQUAL_HEX32(D3D_OK, IDirect3DDevice9_GetSamplerState(std3D_GetD3DDevice(), 0, D3DSAMP_MINFILTER, &minFilter));
    TEST_ASSERT_EQUAL_UINT32(D3DTEXF_ANISOTROPIC, minFilter);

    std3D_Close();
}

TEST(std3DX9System, TestAnisotropicTextureFilterCubeVectorWhenSupported)
{
    std3DX9SystemTest_RequireOpen3DDeviceWithAnisotropicConfig(true);

    const Device3D* pDevice = std3D_GetCurrentDevice();
    TEST_ASSERT_NOT_NULL(pDevice);
    if ( !pDevice->bAnisotropicFilteringSupported
        || (pDevice->d3dDesc.TextureFilterCaps & D3DPTFILTERCAPS_MINFANISOTROPIC) == 0 )
    {
        std3D_Close();
        TEST_IGNORE_MESSAGE("Anisotropic texture filtering is unavailable on this host.");
    }

    Std3DSystemTest_RenderAnisotropicTextureFilterCubeScene();
    Std3DSystemTest_AssertBackBufferMatchesBmpMask("anisotropic_texture_filter_cube_mask.bmp", 128u);

    DWORD minFilter = 0u;
    TEST_ASSERT_EQUAL_HEX32(D3D_OK, IDirect3DDevice9_GetSamplerState(std3D_GetD3DDevice(), 0, D3DSAMP_MINFILTER, &minFilter));
    TEST_ASSERT_EQUAL_UINT32(D3DTEXF_ANISOTROPIC, minFilter);

    std3D_Close();
}

TEST(std3DX9System, TestMSAATriangleVectorWhenSupported)
{
    std3DX9SystemTest_RequireOpen3DDeviceWithMSAA(320u, 240u, false, false);

    Std3DSystemTest_RenderMSAATriangleScene();
    Std3DSystemTest_AssertBackBufferMatchesBmpMask("msaa_triangle_mask.bmp", 32u);
    Std3DSystemTest_AssertBackBufferAnyPixelInRange(64u, 50u, 196u, 150u, 0u, 96u, 24u, 232u, 0u, 96u);

    std3D_Close();
}

TEST(std3DX9System, TestMSAACubeVectorsWhenSupported)
{
    std3DX9SystemTest_RequireOpen3DDeviceWithMSAA(320u, 240u, false, false);

    std3DX9SystemTest_AssertMSAACubeVectorFrames();

    std3D_Close();
}

TEST(std3DX9System, TestMipmapMSAAAnisotropicCubeVectorsWhenSupported)
{
    std3DX9SystemTest_RequireOpen3DDeviceWithMSAA(
        STD3D_SYSTEM_TEST_HIGH_QUALITY_VECTOR_WIDTH,
        STD3D_SYSTEM_TEST_HIGH_QUALITY_VECTOR_HEIGHT,
        true,
        false
    );

    const Device3D* pDevice = std3D_GetCurrentDevice();
    TEST_ASSERT_NOT_NULL(pDevice);
    if ( !Std3DSystemTest_CurrentDeviceSupportsMipmaps() )
    {
        std3D_Close();
        TEST_IGNORE_MESSAGE("Mipmap filtering is unavailable on this host.");
    }

    if ( !pDevice->bAnisotropicFilteringSupported
        || (pDevice->d3dDesc.TextureFilterCaps & D3DPTFILTERCAPS_MINFANISOTROPIC) == 0 )
    {
        std3D_Close();
        TEST_IGNORE_MESSAGE("Anisotropic texture filtering is unavailable on this host.");
    }

    std3DX9SystemTest_AssertMipmapMSAAAnisotropicCubeFrames();

    std3DX9SystemTest_AssertAnisotropicMipmapSamplerState(pDevice);

    std3D_Close();
}

TEST(std3DX9System, TestMipmapAutoGenMSAAAnisotropicCubeVectorsWhenSupported)
{
    std3DX9SystemTest_RequireOpen3DDeviceWithMSAA(
        STD3D_SYSTEM_TEST_HIGH_QUALITY_VECTOR_WIDTH,
        STD3D_SYSTEM_TEST_HIGH_QUALITY_VECTOR_HEIGHT,
        true,
        true
    );

    const Device3D* pDevice = std3D_GetCurrentDevice();
    TEST_ASSERT_NOT_NULL(pDevice);
    if ( !Std3DSystemTest_CurrentDeviceSupportsMipmaps() || !pDevice->bMipmapAutoGenSupported )
    {
        std3D_Close();
        TEST_IGNORE_MESSAGE("Automatic mipmap generation is unavailable on this host.");
    }

    if ( !pDevice->bAnisotropicFilteringSupported
        || (pDevice->d3dDesc.TextureFilterCaps & D3DPTFILTERCAPS_MINFANISOTROPIC) == 0 )
    {
        std3D_Close();
        TEST_IGNORE_MESSAGE("Anisotropic texture filtering is unavailable on this host.");
    }

    TEST_ASSERT_TRUE(stdConfig_GetBool(STD3D_CFG_MIPMAPAUTOGEN, false));
    std3DX9SystemTest_AssertMipmapAutoGenMSAAAnisotropicCubeFrames();
    std3DX9SystemTest_AssertAnisotropicMipmapSamplerState(pDevice);

    std3D_Close();
}

TEST(std3DX9System, TestGeometryModesRenderCubeAndVertexColors)
{
    std3DX9SystemTest_RequireOpen3DDevice();

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

TEST(std3DX9System, TestCubeVectorFramesMatchMasks)
{
    std3DX9SystemTest_RequireOpen3DDevice();

    std3DX9SystemTest_AssertCubeVectorFrames();

    std3D_Close();
}

TEST(std3DX9System, TestFogRenderingTintsFarGeometryWhenSupported)
{
    std3DX9SystemTest_RequireOpen3DDevice();

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

TEST_GROUP_RUNNER(std3DX9System)
{
    RUN_TEST_CASE(std3DX9System, TestStartupEnumeratesDevicesAfterWindowedDisplayMode);
    RUN_TEST_CASE(std3DX9System, TestBuildDisplayEnvironmentEnumeratesDevicesAndModes);
    RUN_TEST_CASE(std3DX9System, TestOpenSceneStateAndPrimitiveDraws);
    RUN_TEST_CASE(std3DX9System, TestAnisotropicConfigEnabledSetsSupportedSamplerState);
    RUN_TEST_CASE(std3DX9System, TestAnisotropicConfigDisabledKeepsNonAnisotropicSamplerState);
    RUN_TEST_CASE(std3DX9System, TestEachEnumerated3DDeviceOpensWithDefaultConfig);
    RUN_TEST_CASE(std3DX9System, TestTextureUploadZBufferAndRenderStates);
    RUN_TEST_CASE(std3DX9System, TestAlphaBlendAlphaTestAndOneBitTransparency);
    RUN_TEST_CASE(std3DX9System, TestTextureAddressFilteringZWriteAndStateIsolation);
    RUN_TEST_CASE(std3DX9System, TestTextureCacheEvictsOldestEligibleEntry);
    RUN_TEST_CASE(std3DX9System, TestRectangularMipChainAndOddPitchTextureUpload);
    RUN_TEST_CASE(std3DX9System, TestDepthWindingSharedEdgesAndProjectionBoundaries);
    RUN_TEST_CASE(std3DX9System, TestDeviceResetRecreates3DResources);
    RUN_TEST_CASE(std3DX9System, TestDynamicVertexAndIndexBuffersWrapWithoutStaleData);
    RUN_TEST_CASE(std3DX9System, TestRepeated3DLifecycle);
    RUN_TEST_CASE(std3DX9System, TestMipmapLodVectorUsesMipChain);
    RUN_TEST_CASE(std3DX9System, TestMipmapLodCubeVectorUsesMipChain);
    RUN_TEST_CASE(std3DX9System, TestMipmapAutoGenCubeVectorsWhenSupported);
    RUN_TEST_CASE(std3DX9System, TestAnisotropicTextureFilterVectorWhenSupported);
    RUN_TEST_CASE(std3DX9System, TestAnisotropicTextureFilterCubeVectorWhenSupported);
    RUN_TEST_CASE(std3DX9System, TestMSAATriangleVectorWhenSupported);
    RUN_TEST_CASE(std3DX9System, TestMSAACubeVectorsWhenSupported);
    RUN_TEST_CASE(std3DX9System, TestMipmapMSAAAnisotropicCubeVectorsWhenSupported);
    RUN_TEST_CASE(std3DX9System, TestMipmapAutoGenMSAAAnisotropicCubeVectorsWhenSupported);
    RUN_TEST_CASE(std3DX9System, TestGeometryModesRenderCubeAndVertexColors);
    RUN_TEST_CASE(std3DX9System, TestCubeVectorFramesMatchMasks);
    RUN_TEST_CASE(std3DX9System, TestFogRenderingTintsFarGeometryWhenSupported);
}

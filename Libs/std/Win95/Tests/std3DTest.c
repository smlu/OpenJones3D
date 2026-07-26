#include <unity_fixture.h>

#include <string.h>

#include <std/General/std.h>
#include <std/General/stdColor.h>
#include <std/General/stdMemory.h>
#include <std/General/stdUtil.h>
#include <std/Win95/std3D.h>
#include <std/Win95/stdDisplay.h>

#include "stdGeneralTest.h"

TEST_GROUP(std3D);

TEST_SETUP(std3D)
{
    StdGeneralTest_Startup();
    stdDisplay_Shutdown();
}

TEST_TEAR_DOWN(std3D)
{
    stdDisplay_Shutdown();
    StdGeneralTest_Shutdown();
}

TEST(std3D, TestPublicLimitsAndConfigKeys)
{
    TEST_ASSERT_EQUAL_UINT32(64u, STD3D_MAXFACEVERTICES);
    TEST_ASSERT_EQUAL_UINT32(32768u, STD3D_MAXVERTICES);
    TEST_ASSERT_TRUE(STD3D_MAXVERTICES > STD3D_MAXFACEVERTICES);

    TEST_ASSERT_EQUAL_STRING("graphics.mipmapAutoGen", STD3D_CFG_MIPMAPAUTOGEN);
    TEST_ASSERT_EQUAL_STRING("graphics.anisotropicFilter", STD3D_CFG_ANISOTROPICFILTER);
    TEST_ASSERT_EQUAL_STRING("graphics.msaa.enabled", STD3D_CFG_MSAAENABLED);
    TEST_ASSERT_EQUAL_STRING("graphics.msaa.samples", STD3D_CFG_MSAASAMPLES);

    TEST_ASSERT_NOT_EQUAL(0, strcmp(STD3D_CFG_MIPMAPAUTOGEN, STD3D_CFG_ANISOTROPICFILTER));
    TEST_ASSERT_NOT_EQUAL(0, strcmp(STD3D_CFG_MIPMAPAUTOGEN, STD3D_CFG_MSAAENABLED));
    TEST_ASSERT_NOT_EQUAL(0, strcmp(STD3D_CFG_MSAAENABLED, STD3D_CFG_MSAASAMPLES));
}

TEST(std3D, TestPublicEnumAndRenderStateValues)
{
    TEST_ASSERT_EQUAL_INT(0, STD3D_MIPMAPFILTER_NONE);
    TEST_ASSERT_EQUAL_INT(1, STD3D_MIPMAPFILTER_BILINEAR);
    TEST_ASSERT_EQUAL_INT(2, STD3D_MIPMAPFILTER_TRILINEAR);

    TEST_ASSERT_EQUAL_UINT32(0x00000001u, STD3D_RS_UNKNOWN_1);
    TEST_ASSERT_EQUAL_UINT32(0x00000002u, STD3D_RS_UNKNOWN_2);
    TEST_ASSERT_EQUAL_UINT32(0x00000004u, STD3D_RS_ALPHAREF_SET);
    TEST_ASSERT_EQUAL_UINT32(0x00000008u, STD3D_RS_UNKNOWN_8);
    TEST_ASSERT_EQUAL_UINT32(0x00000010u, STD3D_RS_SUBPIXEL_CORRECTION);
    TEST_ASSERT_EQUAL_UINT32(0x00000080u, STD3D_RS_TEXFILTER_BILINEAR);
    TEST_ASSERT_EQUAL_UINT32(0x00000100u, STD3D_RS_UNKNOWN_100);
    TEST_ASSERT_EQUAL_UINT32(0x00000200u, STD3D_RS_UNKNOWN_200);
    TEST_ASSERT_EQUAL_UINT32(0x00000400u, STD3D_RS_UNKNOWN_400);
    TEST_ASSERT_EQUAL_UINT32(0x00000800u, STD3D_RS_TEX_CPAMP_U);
    TEST_ASSERT_EQUAL_UINT32(0x00001000u, STD3D_RS_TEX_CPAMP_V);
    TEST_ASSERT_EQUAL_UINT32(0x00002000u, STD3D_RS_ZWRITE_DISABLED);
    TEST_ASSERT_EQUAL_UINT32(0x00008000u, STD3D_RS_FOG_ENABLED);
    TEST_ASSERT_EQUAL_UINT32(0x00010000u, STD3D_RS_TEXFILTER_ANISOTROPIC);
}

TEST(std3D, TestDevice3DPublicFieldsAcceptFeatureLimits)
{
    Device3D device = { 0 };

    device.bHAL                           = 1;
    device.bTexturePerspectiveSupported   = 1;
    device.hasZBuffer                     = 1;
    device.bAlphaBlendSupported           = 1;
    device.minTexWidth                    = 1u;
    device.minTexHeight                   = 1u;
    device.maxTexWidth                    = 4096u;
    device.maxTexHeight                   = 4096u;
    device.maxVertexCount                 = STD3D_MAXVERTICES;
    device.bAnisotropicFilteringSupported = true;
    device.bMipmapAutoGenSupported        = true;
    device.bMSAASupported                 = true;

    TEST_ASSERT_EQUAL_INT(1, device.bHAL);
    TEST_ASSERT_EQUAL_INT(1, device.bTexturePerspectiveSupported);
    TEST_ASSERT_EQUAL_INT(1, device.hasZBuffer);
    TEST_ASSERT_EQUAL_INT(1, device.bAlphaBlendSupported);
    TEST_ASSERT_EQUAL_size_t(1u, device.minTexWidth);
    TEST_ASSERT_EQUAL_size_t(1u, device.minTexHeight);
    TEST_ASSERT_EQUAL_size_t(4096u, device.maxTexWidth);
    TEST_ASSERT_EQUAL_size_t(4096u, device.maxTexHeight);
    TEST_ASSERT_EQUAL_size_t(STD3D_MAXVERTICES, device.maxVertexCount);
    TEST_ASSERT_TRUE(device.bAnisotropicFilteringSupported);
    TEST_ASSERT_TRUE(device.bMipmapAutoGenSupported);
    TEST_ASSERT_TRUE(device.bMSAASupported);
}

TEST(std3D, TestNoDeviceRuntimeStateAndStartupFailure)
{
    const Device3D* pDevices = std3D_GetAllDevices();

    TEST_ASSERT_NULL(std3D_GetCurrentDevice());
    TEST_ASSERT_NOT_NULL(pDevices);
    TEST_ASSERT_EQUAL_size_t(0u, std3D_GetNumDevices());
    TEST_ASSERT_EQUAL_size_t(0u, std3D_GetNumTextureFormats());
    TEST_ASSERT_NULL(std3D_GetD3DDevice());
    TEST_ASSERT_EQUAL_INT(0, std3D_Startup());
    TEST_ASSERT_EQUAL_size_t(0u, std3D_GetNumDevices());
    TEST_ASSERT_NULL(std3D_GetCurrentDevice());
    TEST_ASSERT_EQUAL_size_t(0u, std3D_FindClosestFormat(&stdColor_cfRGB888));
    TEST_ASSERT_NOT_EQUAL(0, std3D_SetProjection(1.0f, 1.0f, 1.0f));
    TEST_ASSERT_NOT_EQUAL(0, std3D_SetProjection(0.0f, 0.1f, 1.0f));

    std3D_SetFindAllDevices(1);
    std3D_SetFindAllDevices(0);

#ifdef J3D_DIRECTX9
    TEST_ASSERT_FALSE(std3D_IsAnisotropicFilteringSupported());
    TEST_ASSERT_FALSE(std3D_IsMipmapAutoGenSupported());
    TEST_ASSERT_FALSE(std3D_IsMSAASupported());
#else
    TEST_ASSERT_FALSE(std3D_IsShaderSystemActive());
    TEST_ASSERT_FALSE(std3D_IsAnisotropicFilteringSupported());
    TEST_ASSERT_FALSE(std3D_IsMipmapAutoGenSupported());
    TEST_ASSERT_FALSE(std3D_IsMSAASupported());
#endif
}

TEST(std3D, TestColorFormatClassificationAndStatusStrings)
{
    ColorInfo colorInfo = stdColor_cfRGB888;

    TEST_ASSERT_EQUAL_INT(STDCOLOR_FORMAT_RGB, std3D_GetColorFormat(&colorInfo));

    colorInfo = stdColor_cfARGB5551;
    TEST_ASSERT_EQUAL_INT(STDCOLOR_FORMAT_RGBA_1BITALPHA, std3D_GetColorFormat(&colorInfo));

    colorInfo = stdColor_cfARGB8888;
    TEST_ASSERT_EQUAL_INT(STDCOLOR_FORMAT_RGBA, std3D_GetColorFormat(&colorInfo));

    TEST_ASSERT_EQUAL_STRING("D3D_OK", std3D_D3DGetStatus(D3D_OK));
    TEST_ASSERT_EQUAL_STRING("Unknown Error", std3D_D3DGetStatus((HRESULT)0x12345678));
}

TEST(std3D, TestEmptyTextureState)
{
    tSystemTexture texture;

    STD_ZEROMEM(&texture, sizeof(texture));
    TEST_ASSERT_EQUAL_size_t(0u, std3D_GetMipMapCount(NULL));
#ifdef J3D_DIRECTX9
    texture.numMipLevels = 3u;
    TEST_ASSERT_EQUAL_size_t(3u, std3D_GetMipMapCount(&texture));
    texture.numMipLevels = 0u;
    std3D_ClearSystemTexture(&texture);
    TEST_ASSERT_EQUAL_size_t(0u, texture.numMipLevels);
#else
    TEST_ASSERT_EQUAL_size_t(0u, std3D_GetMipMapCount(&texture));
    std3D_ClearSystemTexture(&texture);
    TEST_ASSERT_NULL(texture.pTexture);
#endif
}

#ifdef J3D_DIRECTX9
TEST(std3D, TestClearSystemTextureReleasesDX9MipBuffers)
{
    tRasterInfo rasterInfo = { 0 };
    tSystemTexture texture;
    tVBuffer** apMipmaps;

    rasterInfo.width     = 2u;
    rasterInfo.height    = 2u;
    rasterInfo.colorInfo = stdColor_cfRGB8888;

    apMipmaps = (tVBuffer**)STDMALLOC(sizeof(*apMipmaps) * 2u);
    TEST_ASSERT_NOT_NULL(apMipmaps);

    apMipmaps[0] = stdDisplay_VBufferNew(&rasterInfo, 0, 0);
    apMipmaps[1] = stdDisplay_VBufferNew(&rasterInfo, 0, 0);
    TEST_ASSERT_NOT_NULL(apMipmaps[0]);
    TEST_ASSERT_NOT_NULL(apMipmaps[1]);

    STD_ZEROMEM(&texture, sizeof(texture));
    texture.apMipmaps     = apMipmaps;
    texture.numMipLevels  = 2u;
    texture.textureSize   = apMipmaps[0]->rasterInfo.size + apMipmaps[1]->rasterInfo.size;
    texture.frameNum      = 77u;

    std3D_ClearSystemTexture(&texture);

    TEST_ASSERT_NULL(texture.apMipmaps);
    TEST_ASSERT_EQUAL_size_t(0u, texture.numMipLevels);
    TEST_ASSERT_EQUAL_size_t(0u, texture.textureSize);
    TEST_ASSERT_EQUAL_size_t(0u, texture.frameNum);
}
#endif

TEST_GROUP_RUNNER(std3D)
{
    RUN_TEST_CASE(std3D, TestPublicLimitsAndConfigKeys);
    RUN_TEST_CASE(std3D, TestPublicEnumAndRenderStateValues);
    RUN_TEST_CASE(std3D, TestDevice3DPublicFieldsAcceptFeatureLimits);
    RUN_TEST_CASE(std3D, TestNoDeviceRuntimeStateAndStartupFailure);
    RUN_TEST_CASE(std3D, TestColorFormatClassificationAndStatusStrings);
    RUN_TEST_CASE(std3D, TestEmptyTextureState);
#ifdef J3D_DIRECTX9
    RUN_TEST_CASE(std3D, TestClearSystemTextureReleasesDX9MipBuffers);
#endif
}

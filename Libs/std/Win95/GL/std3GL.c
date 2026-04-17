#include <std/Win95/std3D.h>
#include <std/Win95/stdDisplay.h>
#include <std/Win95/stdShader.h>

#include <j3dcore/j3dhook.h>
#include <std/General/std.h>
#include <std/General/stdColor.h>
#include <std/General/stdConfig.h>
#include <std/General/stdMath.h>
#include <std/General/stdMemory.h>
#include <std/General/stdUtil.h>

#define STD3D_DEFAULT_MAX_VERTICES 512
#define STD3D_MAX_VERTICES_PER_DRAW 65536
#define STD3D_MAX_INDICES_PER_DRAW 131072
#define STD3D_MAX_LEGACY_BATCHES 10000

static bool bStartup            = false;
static bool std3D_bOpen         = false;

bool std3D_g_bUseLegacyRendering;


static GLRECT std3D_activeRect = { 0 };
static_assert(sizeof(std3D_activeRect) == 4 * sizeof(float), "sizeof(std3D_activeRect) == 4 * sizeof(float)");
// Must be 4 floats to be used in shader

static size_t std3D_frameCount            = 1;
static Std3DRenderState std3D_renderState = 0;
static tSysTexture* std3D_pD3DTex         = NULL;

static bool std3D_bAnisotropicFilter; // Added
static bool std3D_bAutoGenMipmap;     // Added

static Std3DMipmapFilterType std3D_mipmapFilter = -1;
static GLfloat std3D_maxAnisoLevel              = 0.0f;

static bool std3D_bRenderFog          = true;
static float std3D_fogDepthFactor     = 0.0f;
static float std3D_fogStartDepth      = 0.0f;
static float std3D_fogEndDepth        = 0.0f;
static StdShaderVector std3D_fogColor = { 0 };

static bool std3D_bFindAllD3Devices = false;
static Device3D* std3D_pCurDevice   = NULL;

static size_t std3D_numDevices    = 0;
static Device3D std3D_aDevices[4] = { 0 };

static size_t std3D_numCachedTextures       = 0;
static tSystemTexture* std3D_pFirstTexCache = NULL;
static tSystemTexture* std3D_pLastTexCache  = NULL;
static tSysTexture* std3D_pWhiteTexture     = NULL;

static const tSysPixelFormat std3D_RGBATextureFormat =
{
    .glInternalFormat = GL_RGBA8,
    .glFormat = GL_BGRA,
    .glType = GL_UNSIGNED_INT_8_8_8_8_REV
};

static tSysPixelFormat std3D_RGBTextureFormat =
{
    .glInternalFormat = GL_RGB8,
    .glFormat = GL_BGR,
    .glType = GL_UNSIGNED_BYTE
};

static bool std3D_bHasRGBTextureFormat = true;

typedef enum eDrawMode
{
    DM_NONE      = 0,
    DM_VERTEX    = 1,
    DM_WIREFRAME = 2,
    DM_SOLID     = 3,
    DM_FULL      = 4
} DrawMode;

static LegacyBatch std3D_cachedLegacyBatches[STD3D_MAX_LEGACY_BATCHES];
static size_t std3D_numCachedLegacyBatches = 0;

static D3DTLVERTEX std3D_pScreenSpaceVertexBuffer[STD3D_MAX_VERTICES_PER_DRAW * sizeof(D3DTLVERTEX)];
static WORD std3D_pScreenSpaceElementBuffer[STD3D_MAX_INDICES_PER_DRAW * sizeof(GLushort)];
static size_t std3D_numScreenSpaceVertices = 0;
static size_t std3D_numScreenSpaceIndices  = 0;

// VBO & IBO
static GLuint std3D_screenSpaceVao          = 0;
static GLuint std3D_screenSpaceVbo          = 0;
static GLuint std3D_screenSpaceEbo          = 0;
static bool std3D_bScreenSpaceBuffersMapped = false;


static GLuint std3D_staticVao      = 0;
static GLuint std3D_staticVbo      = 0;
static GLuint std3D_staticEbo      = 0;
static GLuint std3D_instanceBuffer = 0;
static size_t std3D_instanceOffset = 0;

static size_t std3D_numDrawCalls   = 0;

static GLuint std3D_activeSampler = 0;

static DrawMode std3D_currentDrawMode = DM_FULL;

// Shaders
static GLShaderProgram* std3D_pDefaultShader    = NULL;
static GLShaderProgram* std3D_pModelShader      = NULL;
static GLShaderProgram* std3D_pCeilingSkyShader = NULL;
static GLShaderProgram* std3D_pHorizonSkyShader = NULL;
static GLShaderProgram* std3D_pSpriteShader     = NULL;
static GLShaderProgram* std3D_pParticleShader   = NULL;
static GLShaderProgram* std3D_pPolyLineShader   = NULL;
static GLShaderProgram* std3D_pLegacyShader     = NULL;
static GLShaderProgram* std3D_pActiveShader     = NULL;

static int std3D_InitRenderState(void);
static int std3D_BuildDeviceList(void);

static int std3D_CreateViewport(void);
static void J3DAPI std3D_AddTextureToCacheList(tSystemTexture* pTexture);
static void J3DAPI std3D_RemoveTextureFromCacheList(tSystemTexture* pCacheTexture);
static int J3DAPI std3D_PurgeTextureCache(size_t size);

bool std3D_InitVertexBuffers(GLuint* vbo, GLuint* ebo, GLuint* vao);
void std3D_ReleaseScreenSpaceBuffers(void);

bool std3D_InitShaderSystem(void);
void std3D_ShutdownShaderSystem(void);

static void std3D_MapScreenSpaceBuffers(void);
static void std3D_UnmapScreenSpaceBuffers(void);

void std3D_InstallHooks(void)
{
    J3D_HOOKFUNC(std3D_Startup);
    J3D_HOOKFUNC(std3D_Shutdown);
    J3D_HOOKFUNC(std3D_GetNumDevices);
    J3D_HOOKFUNC(std3D_GetAllDevices);
    J3D_HOOKFUNC(std3D_Open);
    J3D_HOOKFUNC(std3D_Close);
    J3D_HOOKFUNC(std3D_GetTextureFormat);
    J3D_HOOKFUNC(std3D_GetColorFormat);
    J3D_HOOKFUNC(std3D_GetNumTextureFormats);
    J3D_HOOKFUNC(std3D_StartScene);
    J3D_HOOKFUNC(std3D_EndScene);
    J3D_HOOKFUNC(std3D_DrawRenderList);
    J3D_HOOKFUNC(std3D_SetWireframeRenderState);
    J3D_HOOKFUNC(std3D_DrawLineStrip);
    J3D_HOOKFUNC(std3D_DrawPointList);
    J3D_HOOKFUNC(std3D_SetRenderState);
    J3D_HOOKFUNC(std3D_AllocSystemTexture);
    J3D_HOOKFUNC(std3D_GetValidDimensions);
    J3D_HOOKFUNC(std3D_ClearSystemTexture);
    J3D_HOOKFUNC(std3D_AddToTextureCache);
    J3D_HOOKFUNC(std3D_GetMipMapCount);
    J3D_HOOKFUNC(std3D_ResetTextureCache);
    J3D_HOOKFUNC(std3D_UpdateFrameCount);
    //J3D_HOOKFUNC(std3D_FindClosestFormat);
    J3D_HOOKFUNC(std3D_InitRenderState);
    J3D_HOOKFUNC(std3D_SetMipmapFilter);
    J3D_HOOKFUNC(std3D_SetProjection);
    J3D_HOOKFUNC(std3D_EnableFog);
    J3D_HOOKFUNC(std3D_SetFog);
    J3D_HOOKFUNC(std3D_ClearZBuffer);
    J3D_HOOKFUNC(std3D_CreateViewport);
    J3D_HOOKFUNC(std3D_AddTextureToCacheList);
    J3D_HOOKFUNC(std3D_RemoveTextureFromCacheList);
    J3D_HOOKFUNC(std3D_PurgeTextureCache);
    //J3D_HOOKFUNC(std3D_D3DGetStatus);
    J3D_HOOKFUNC(std3D_BuildDisplayEnvironment);
    J3D_HOOKFUNC(std3D_FreeDisplayEnvironment);
    J3D_HOOKFUNC(std3D_SetFindAllDevices);
}

void std3D_ResetGlobals(void)
{
    float std3D_g_fogDensity_tmp = 1.0f;
    memcpy(&std3D_g_fogDensity, &std3D_g_fogDensity_tmp, sizeof(std3D_g_fogDensity));
    memset(&std3D_g_maxVertices, 0, sizeof(std3D_g_maxVertices));
}

int std3D_Startup(void)
{
    STD_ASSERTREL(bStartup == false);
    memset(std3D_aDevices, 0, sizeof(std3D_aDevices));

    if ( !std3D_BuildDeviceList() )
    {
        STDLOG_ERROR("Error building device list.\n");
        return 0;
    }

    if ( std3D_numDevices == 0 )
    {
        return 0;
    }

    bStartup = true;
    return 1;
}

void std3D_Shutdown(void)
{
    if ( !bStartup )
    {
        STDLOG_WARNING("Warning: System already shutdown!\n");
        return;
    }

    if ( std3D_bOpen )
    {
        std3D_Close();
    }

    memset(std3D_aDevices, 0, sizeof(std3D_aDevices));

    std3D_numDevices = 0;
    bStartup         = false;
}

const Device3D* std3D_GetCurrentDevice(void) { return std3D_pCurDevice; }

size_t std3D_GetNumDevices(void) { return std3D_numDevices; }

const Device3D* std3D_GetAllDevices(void) { return std3D_aDevices; }

static bool std3D_InitSystem(void)
{
    if ( stdDisplay_CreateZBuffer(NULL, false) )
    {
        STDLOG_ERROR("Error creating Z buffer.\n");
        return false;
    }

    std3D_g_bUseLegacyRendering = stdConfig_GetBool(STD3D_CFG_LEGACYRENDERING, false);

    // create 1x1 white texture for solid mode
    std3D_pWhiteTexture          = STDMALLOC(sizeof(tSysTexture));
    std3D_pWhiteTexture->pShader = NULL;
    glGenTextures(1, &std3D_pWhiteTexture->id);
    glBindTexture(GL_TEXTURE_2D, std3D_pWhiteTexture->id);

    unsigned char whitePixel[] = { 255, 255, 255, 255 };

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, whitePixel);
    glBindTexture(GL_TEXTURE_2D, 0);

    // Get autogen support
    std3D_bAutoGenMipmap = stdConfig_GetBool(STD3D_CFG_MIPMAPAUTOGEN, true);
    if ( std3D_bAutoGenMipmap && !std3D_pCurDevice->bMipmapAutoGenSupported )
    {
        STDLOG_WARNING("Warning: Automatic mipmap generation disabled, no device support!\n");
        std3D_bAutoGenMipmap = false;
    }

    std3D_bAnisotropicFilter = std3D_bAutoGenMipmap && stdConfig_GetBool(STD3D_CFG_ANISOTROPICFILTER, true);
    if ( std3D_bAnisotropicFilter && !std3D_pCurDevice->bAnisotropicFilteringSupported )
    {
        STDLOG_WARNING("Warning: Anisotropic filtering disabled, no device support!\n");
        std3D_bAutoGenMipmap     = false;
        std3D_bAnisotropicFilter = false;
    }
    else
    {
        glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &std3D_maxAnisoLevel);
    }

    std3D_CreateViewport();

    std3D_g_maxVertices = std3D_pCurDevice->maxVertexCount;
    if ( std3D_g_maxVertices == 0 )
    {
        std3D_g_maxVertices = STD3D_DEFAULT_MAX_VERTICES;
    }
    STDLOG_STATUS("Max vertices: %d.\n", std3D_g_maxVertices);

    std3D_frameCount        = 1;
    std3D_numCachedTextures = 0;
    std3D_pFirstTexCache    = NULL;
    std3D_pLastTexCache     = NULL;

    glGenSamplers(1, &std3D_activeSampler);

    std3D_InitVertexBuffers(&std3D_screenSpaceVbo, &std3D_screenSpaceEbo, &std3D_screenSpaceVao);
    if ( !std3D_InitShaderSystem() )
    {
        STDLOG_ERROR("Error initializing pShader system.\n");
        return false;
    }

    std3D_mipmapFilter = -1; // Must be reset
    std3D_InitRenderState();

    if ( stdDisplay_GetTextureMemory(&std3D_pCurDevice->totalMemory, &std3D_pCurDevice->availableMemory) )
    {
        // Since we failed to get texture memory info indicate that opening failed
        return false;
    }

    return true;
}

static void std3D_ReleaseSystemResources(void)
{
    glDeleteTextures(1, &std3D_pWhiteTexture->id);
    STDFREE(std3D_pWhiteTexture);
    std3D_pWhiteTexture = NULL;

    std3D_ResetTextureCache();
    std3D_ShutdownShaderSystem();
    std3D_ReleaseScreenSpaceBuffers();
}

static void std3D_OnDisplayDeviceReset(tSysDevice3D* pDevice)
{
    J3D_UNUSED(pDevice);
    // Release any cached texture before device is changed
    STDLOG_DEBUG("Received display device to be reset signal. Releasing system resources...\n");
    std3D_ReleaseSystemResources();
}

static void std3D_OnDisplayDevicePostReset(tSysDevice3D* pDevice)
{
    J3D_UNUSED(pDevice);
    STDLOG_DEBUG("Received display device reset signal. Re-initializing the system...\n");

    std3D_InitSystem();
}

static void std3D_OnDisplayDeviceRelease(tSysDevice3D* pDevice)
{
    J3D_UNUSED(pDevice);
    STDLOG_DEBUG("Received signal that display device is about to be released. Releasing system resources...\n");
    std3D_ReleaseSystemResources();
}

int J3DAPI std3D_Open(size_t deviceNum)
{
    STD_ASSERTREL(bStartup == true);
    if ( std3D_bOpen )
    {
        STDLOG_ERROR("Warning: System already open!\n");
        return 0;
    }

    // Added
    if ( !stdDisplay_IsOpen() )
    {
        STDLOG_ERROR("std3D_Open: Display system is not opened!\n");
        return 0;
    }

    if ( deviceNum >= std3D_numDevices )
    {
        return 0;
    }

    std3D_pCurDevice = &std3D_aDevices[deviceNum];

    // Register device changed callback
    stdDisplay_RegisterDevicePreResetCallback(std3D_OnDisplayDeviceReset);
    stdDisplay_RegisterDevicePostResetCallback(std3D_OnDisplayDevicePostReset);
    stdDisplay_RegisterDeviceReleaseCallback(std3D_OnDisplayDeviceRelease);

    // Initialize system
    if ( !std3D_InitSystem() )
    {
        STDLOG_ERROR("Failed to initialize system, closing...\n");
        std3D_Close();
        return 0;
    }

    // Print device information
    size_t memFree  = 0;
    size_t memTotal = 0;
    stdDisplay_GetTotalMemory(&memTotal, &memFree);

    STDLOG_STATUS("Texture Ram  Total: %u bytes  Free: %u bytes.\n", std3D_pCurDevice->totalMemory, std3D_pCurDevice->availableMemory);
    STDLOG_STATUS("Video Ram Total: %u bytes  Free: %u bytes.\n", memTotal, memFree);

    std3D_bOpen = true;
    return 1;
}

void std3D_Close(void)
{
    if ( !std3D_bOpen )
    {
        STDLOG_WARNING("Warning: System already closed!\n");
        return;
    }

    stdDisplay_RegisterDevicePreResetCallback(NULL);
    stdDisplay_RegisterDevicePostResetCallback(NULL);
    stdDisplay_RegisterDeviceReleaseCallback(NULL);

    std3D_ReleaseSystemResources();

    std3D_mipmapFilter         = -1;
    std3D_pCurDevice           = NULL;
    std3D_bHasRGBTextureFormat = false;
    std3D_bOpen                = false;
}

void J3DAPI std3D_GetTextureFormat(StdColorFormatType type, ColorInfo* pDest, int* pbColorKeySet, LPDDCOLORKEY* ppColorKey)
{
    if ( type == STDCOLOR_FORMAT_RGBA_1BITALPHA )
    {
        *pbColorKeySet = 0;
        *ppColorKey    = 0;
        *pDest         = stdColor_cfARGB5551;
    }
    else if ( type == STDCOLOR_FORMAT_RGBA )
    {
        *pbColorKeySet = 0;
        *ppColorKey    = 0;
        *pDest         = stdColor_cfARGB8888;
    }
    else
    {
        *pbColorKeySet = 0;
        *pDest         = stdColor_cfRGB888;
    }
}

StdColorFormatType J3DAPI std3D_GetColorFormat(const ColorInfo* pCi)
{
    if ( pCi->alphaBPP == 0 )
    {
        return STDCOLOR_FORMAT_RGB;
    }

    if ( pCi->alphaBPP == 1 )
    {
        return STDCOLOR_FORMAT_RGBA_1BITALPHA;
    }

    return STDCOLOR_FORMAT_RGBA;
}

size_t std3D_GetNumTextureFormats(void)
{
    return 2; // always two, since we only use RGB and RGBA as texture format
}

int std3D_StartScene(void)
{
    glEnable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);
    //stdShader_UpdateGlobalUniforms();
    glBindSampler(TU_3D_DRAW, std3D_activeSampler);
    ++std3D_frameCount;
    const float vp[4] = {
        (float)std3D_activeRect.x1, // x
        (float)std3D_activeRect.y1, // y
        (float)std3D_activeRect.x2, // width
        (float)std3D_activeRect.y2  // height
    };

    return !stdShader_SetViewport(vp);
}

void std3D_EndScene(void)
{
    std3D_DrawLegacyBatches();
    std3D_SetRenderState(0);
    std3D_renderState  = 0;
    std3D_numDrawCalls = 0;
    std3D_pD3DTex      = NULL;
    glBindSampler(TU_3D_DRAW, 0);
    stdShader_DisableFog();
}

void std3D_SetWireframeRenderState(void)
{
} //not needed anymore, as this is done when wireframe draw call is cached

void J3DAPI std3D_DrawLineStrip(LPD3DTLVERTEX aVerts, size_t numVerts)
{
    STDLOG_ERROR("Usage of std3D_DrawLineStrip is not supported in OpenGL. Use rdCache_AddLineDrawCall instead.\n");
}

void J3DAPI std3D_DrawPointList(LPD3DTLVERTEX aVerts, size_t numVerts)
{
    STDLOG_ERROR("Usage of std3D_DrawPointList is not supported in OpenGL.\n");
}

void J3DAPI std3D_SetRenderState(Std3DRenderState rdflags)
{
    if ( std3D_renderState == rdflags )
        return;

    if ( (std3D_renderState & STD3D_RS_ZWRITE_DISABLED) != (rdflags & STD3D_RS_ZWRITE_DISABLED) )
    {
        if ( rdflags & STD3D_RS_ZWRITE_DISABLED )
        {
            glDepthMask(GL_FALSE);
        }
        else
        {
            glDepthMask(GL_TRUE);
        }
    }

    if ( (std3D_renderState & STD3D_BLEND_ENABLED) != (rdflags & STD3D_BLEND_ENABLED) )
    {
        if ( rdflags & STD3D_BLEND_ENABLED )
        {
            glEnable(GL_BLEND);
        }
        else
        {
            glDisable(GL_BLEND);
        }
    }

    if ( (std3D_renderState & STD3D_CULL_DISABLED) != (rdflags & STD3D_CULL_DISABLED) )
    {
        if ( rdflags & STD3D_CULL_DISABLED )
        {
            glDisable(GL_CULL_FACE);
        }
        else
        {
            glEnable(GL_CULL_FACE);
        }
    }

    if ( (std3D_renderState & STD3D_RS_TEX_CPAMP_U) != (rdflags & STD3D_RS_TEX_CPAMP_U) )
    {
        glSamplerParameteri(std3D_activeSampler, GL_TEXTURE_WRAP_S, (rdflags & STD3D_RS_TEX_CPAMP_U) ? GL_CLAMP_TO_EDGE : GL_REPEAT);
    }

    if ( (std3D_renderState & STD3D_RS_TEX_CPAMP_V) != (rdflags & STD3D_RS_TEX_CPAMP_V) )
    {
        glSamplerParameteri(std3D_activeSampler, GL_TEXTURE_WRAP_T, (rdflags & STD3D_RS_TEX_CPAMP_V) ? GL_CLAMP_TO_EDGE : GL_REPEAT);
    }

    if ( (std3D_renderState & STD3D_RS_FOG_ENABLED) != (rdflags & STD3D_RS_FOG_ENABLED) )
    {
        if ( (rdflags & STD3D_RS_FOG_ENABLED) && std3D_bRenderFog )
        {
            stdShader_SetFog(std3D_bRenderFog, std3D_fogStartDepth, std3D_fogEndDepth, std3D_fogDepthFactor, std3D_fogColor);
        }
        else
        {
            stdShader_DisableFog();
        }
    }


    if ( (std3D_renderState & STD3D_RS_TEXFILTER_ANISOTROPIC) != (rdflags & STD3D_RS_TEXFILTER_ANISOTROPIC) )
    {
        if ( (rdflags & STD3D_RS_TEXFILTER_ANISOTROPIC) != 0 && std3D_bAnisotropicFilter )
        {
            glSamplerParameteri(std3D_activeSampler, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
            glSamplerParameteri(std3D_activeSampler, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glSamplerParameterf(std3D_activeSampler, GL_TEXTURE_MAX_ANISOTROPY_EXT, std3D_maxAnisoLevel);
        }
        else
        {
            glSamplerParameteri(std3D_activeSampler, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glSamplerParameteri(std3D_activeSampler, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glSamplerParameterf(std3D_activeSampler, GL_TEXTURE_MAX_ANISOTROPY_EXT, 1.0f);
        }
    }
    if ( (std3D_renderState & STD3D_RS_TEXFILTER_BILINEAR) != (rdflags & STD3D_RS_TEXFILTER_BILINEAR) )
    {
        if ( rdflags & STD3D_RS_TEXFILTER_BILINEAR )
        {
            glSamplerParameteri(std3D_activeSampler, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glSamplerParameteri(std3D_activeSampler, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        }
        else
        {
            glSamplerParameteri(std3D_activeSampler, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glSamplerParameteri(std3D_activeSampler, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        }
    }
    std3D_renderState = rdflags;
}

void J3DAPI std3D_AllocSystemTexture(tSystemTexture* pTexture, tVBuffer** apVBuffers, size_t numMipLevels, StdColorFormatType formatType)
{
    memset(pTexture, 0, sizeof(tSystemTexture));

    // Get mipmap buffer at LOD 0
    tVBuffer* pVBuffer = *apVBuffers;
    uint32_t texHeight = 0, texWidth = 0;
    std3D_GetValidDimensions(pVBuffer->rasterInfo.width, pVBuffer->rasterInfo.height, &texWidth, &texHeight);
    while ( numMipLevels > 1 && (pVBuffer->rasterInfo.width > texWidth || pVBuffer->rasterInfo.height > texHeight) )
    {
        --numMipLevels;
        pVBuffer = *++apVBuffers;
        std3D_GetValidDimensions(pVBuffer->rasterInfo.width, pVBuffer->rasterInfo.height, &texWidth, &texHeight);
    }

    size_t texSize = (pVBuffer->rasterInfo.colorInfo.bpp * texHeight * texWidth) / 8;
    if ( std3D_mipmapFilter == STD3D_MIPMAPFILTER_NONE )
    {
        numMipLevels = 1;
    }

    tSysPixelFormat glFormat;
    if ( formatType == STDCOLOR_FORMAT_RGBA || formatType == STDCOLOR_FORMAT_RGBA_1BITALPHA )
    {
        glFormat = std3D_RGBATextureFormat;
    }
    else
    {
        glFormat = std3D_RGBTextureFormat;
    }

    // Allocate array for VBuffer pointers - NO DirectX objects created
    pTexture->apMipmaps = (tVBuffer**)STDMALLOC(numMipLevels * sizeof(tVBuffer*));
    if ( !pTexture->apMipmaps )
    {
        STDLOG_ERROR("Failed to allocate memory for VBuffer array.\n");
        goto error;
    }

    pTexture->numMipLevels = numMipLevels;
    pTexture->format       = glFormat;
    pTexture->textureSize  = texSize;

    // Create owned VBuffer copies for each mip level
    for ( size_t mmNum = 0; mmNum < numMipLevels; ++mmNum )
    {
        // Create new VBuffer copy using stdDisplay_VBufferNew
        pTexture->apMipmaps[mmNum] = stdDisplay_VBufferNew(&apVBuffers[mmNum]->rasterInfo, /*bUseVSurface*/ 0, /*bUseVideoMemory*/0);
        if ( !pTexture->apMipmaps[mmNum] )
        {
            STDLOG_ERROR("Failed to create VBuffer for mip level %d.\n", mmNum);
            goto error;
        }

        // Copy pixel data from source VBuffer to our owned copy
        tColorMode colorMode = apVBuffers[mmNum]->rasterInfo.colorInfo.colorMode;
        if ( colorMode )
        {
            if ( colorMode > STDCOLOR_PAL && colorMode <= STDCOLOR_RGBA )
            {
                stdDisplay_VBufferLock(apVBuffers[mmNum]);
                stdDisplay_VBufferLock(pTexture->apMipmaps[mmNum]);

                size_t pixelDataSize = apVBuffers[mmNum]->rasterInfo.height * apVBuffers[mmNum]->rasterInfo.rowSize;
                memcpy(pTexture->apMipmaps[mmNum]->pPixels, apVBuffers[mmNum]->pPixels, pixelDataSize);

                stdDisplay_VBufferUnlock(pTexture->apMipmaps[mmNum]);
                stdDisplay_VBufferUnlock(apVBuffers[mmNum]);
            }
        }
        else
        {
            STDLOG_ERROR("Can't use paletized textures.\n");
            goto error;
        }
    }

    return;

error:
    if ( pTexture->apMipmaps )
    {
        stdMemory_Free(pTexture->apMipmaps);
    }
    memset(pTexture, 0, sizeof(tSystemTexture));

    STDLOG_ERROR("Done error exit from std3D_AllocSystemTexture.\n");
}

void J3DAPI std3D_GetValidDimensions(uint32_t width, uint32_t height, uint32_t* pOutWidth, uint32_t* pOutHeight)
{
    uint32_t texWidth  = STDMATH_CLAMP(width, std3D_pCurDevice->minTexWidth, std3D_pCurDevice->maxTexWidth);
    uint32_t texHeight = STDMATH_CLAMP(height, std3D_pCurDevice->minTexHeight, std3D_pCurDevice->maxTexHeight);

    if ( !std3D_pCurDevice->bSqareOnlyTexture || texWidth == texHeight )
    {
        *pOutWidth  = texWidth;
        *pOutHeight = texHeight;
    }
    else
    {
        if ( texWidth > texHeight )
        {
            texHeight = texWidth;
        }

        *pOutWidth  = texHeight;
        *pOutHeight = texHeight;
    }
}

void J3DAPI std3D_ClearSystemTexture(tSystemTexture* pTex)
{
    while ( pTex->numMipLevels > 0 )
    {
        stdDisplay_VBufferFree(pTex->apMipmaps[--pTex->numMipLevels]);
    }

    if ( pTex->apMipmaps )
    {
        stdMemory_Free(pTex->apMipmaps);
        pTex->apMipmaps = NULL;
    }

    if ( pTex->pCachedTexture )
    {
        std3D_RemoveTextureFromCacheList(pTex);
        glDeleteTextures(1, &pTex->pCachedTexture->id);
        STDFREE(pTex->pCachedTexture);
        pTex->pCachedTexture = NULL;
    }

    memset(pTex, 0, sizeof(tSystemTexture));
}

void J3DAPI std3D_AddToTextureCache(tSystemTexture* pCacheTexture, StdColorFormatType format)
{
    J3D_UNUSED(format);
    STD_ASSERTREL(pCacheTexture);

    if ( pCacheTexture->numMipLevels == 0 || !pCacheTexture->apMipmaps )
    {
        STDLOG_ERROR("No source texture.\n");
        return;
    }

    if ( pCacheTexture->textureSize > std3D_pCurDevice->availableMemory )
    {
        std3D_PurgeTextureCache(pCacheTexture->textureSize);
    }

    GLuint tex = 0;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    GLenum formatGL        = pCacheTexture->format.glFormat;
    GLenum typeGL          = pCacheTexture->format.glType;
    GLint internalFormatGL = pCacheTexture->format.glInternalFormat;

    tVBuffer* mainTex = pCacheTexture->apMipmaps[0];
    GLsizei width     = (GLsizei)mainTex->rasterInfo.width;
    GLsizei height    = (GLsizei)mainTex->rasterInfo.height;
    glTexImage2D(GL_TEXTURE_2D, 0, internalFormatGL, width, height, 0, formatGL, typeGL, mainTex->pPixels);
    if ( std3D_bAutoGenMipmap )
    {
        glGenerateMipmap(GL_TEXTURE_2D);
    }
    else
    {
        for ( size_t i = 1; i < pCacheTexture->numMipLevels; i++ )
        {
            tVBuffer* mipMap  = pCacheTexture->apMipmaps[i];
            GLsizei mipWidth  = (GLsizei)mipMap->rasterInfo.width;
            GLsizei mipHeight = (GLsizei)mipMap->rasterInfo.height;
            glTexImage2D(GL_TEXTURE_2D, (GLint)i, internalFormatGL, mipWidth, mipHeight, 0, formatGL, typeGL, mipMap->pPixels);
        }
    }

    tSysTexture* texture = STDMALLOC(sizeof(tSysTexture));
    texture->id          = tex;

    pCacheTexture->pCachedTexture = texture;
    pCacheTexture->frameNum       = std3D_frameCount;
    std3D_AddTextureToCacheList(pCacheTexture);
}

size_t J3DAPI std3D_GetMipMapCount(const tSystemTexture* pTexture)
{
    if ( !pTexture )
    {
        return 0;
    }

    return pTexture->numMipLevels;
}

void std3D_ResetTextureCache(void)
{
    if ( std3D_bScreenSpaceBuffersMapped )
    {
        //clear cached batches since saved textures are not valid anymore
        std3D_DrawLegacyBatches();
    }


    STDLOG_DEBUG("Clearing texture cache....\n");
    glBindFramebuffer(GL_FRAMEBUFFER, 0); //draw in empty framebuffer for one time to avoid texture flickering
    stdShader_SetActiveTextureUnit(TU_3D_DRAW);
    glBindTexture(GL_TEXTURE_2D, 0);
    std3D_pD3DTex = NULL;

    tSystemTexture* pCurTex = std3D_pFirstTexCache;
    while ( pCurTex )
    {
        if ( pCurTex->pCachedTexture )
        {
            GLuint tex = pCurTex->pCachedTexture->id;
            glDeleteTextures(1, &tex);
            pCurTex->pCachedTexture->pShader = NULL;
            STDFREE(pCurTex->pCachedTexture);
            pCurTex->pCachedTexture = NULL;
        }

        tSystemTexture* pNextTex    = pCurTex->pNextCachedTexture;
        pCurTex->frameNum           = 0;
        pCurTex->pNextCachedTexture = NULL;
        pCurTex->pPrevCachedTexture = NULL;

        pCurTex = pNextTex;
    }

    std3D_pFirstTexCache    = NULL;
    std3D_pLastTexCache     = NULL;
    std3D_numCachedTextures = 0;

    if ( std3D_pCurDevice )
    {
        std3D_pCurDevice->availableMemory = std3D_pCurDevice->totalMemory;
    }

    std3D_frameCount = 1;
    std3D_pD3DTex    = NULL;
}

void J3DAPI std3D_UpdateFrameCount(tSystemTexture* pTexture)
{
    std3D_RemoveTextureFromCacheList(pTexture);
    std3D_AddTextureToCacheList(pTexture);
    pTexture->frameNum = std3D_frameCount; // Fixed: Moved frameNum update to the
    // end of the function.
    //        Originally it was updated at the beginning of the function,
    //        and the frameNum was immediately invalidated by call to
    //        std3D_RemoveTextureFromCacheList.
}

int std3D_InitRenderState(void)
{
    std3D_renderState = 0;

    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_TRUE);
    glDepthFunc(GL_LEQUAL);
    glDisable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glFrontFace(GL_CCW);
    glEnable(GL_CULL_FACE);

    std3D_renderState |= STD3D_RS_UNKNOWN_1 | STD3D_RS_UNKNOWN_2 | STD3D_RS_TEXFILTER_BILINEAR;
    std3D_SetMipmapFilter(STD3D_MIPMAPFILTER_TRILINEAR);

    return true;
}

int J3DAPI std3D_SetMipmapFilter(Std3DMipmapFilterType filter)
{
    std3D_mipmapFilter = filter;
    return 0;
}

int J3DAPI std3D_SetProjection(float fov, float nearPlane, float farPlane) // This is currently not needed since projection is not done in shader
{
    stdShader_UpdateGlobalUniforms();
    return 1;
}

void J3DAPI std3D_EnableFog(int bEnabled, float density)
{
    std3D_bRenderFog   = bEnabled;
    std3D_g_fogDensity = density;
}

void J3DAPI std3D_SetFog(float red, float green, float blue, float startDepth, float endDepth)
{
    // Store fog parameters for shader use
    float fogFactor = std3D_g_bUseLegacyRendering ? 1.0f : 0.03459f;
    std3D_EnableFog(std3D_bRenderFog, std3D_g_fogDensity);
    std3D_fogStartDepth  = startDepth * fogFactor;
    std3D_fogEndDepth    = (2.0f - std3D_g_fogDensity) * endDepth * fogFactor;
    std3D_fogDepthFactor = 1.0f / (std3D_fogEndDepth - std3D_fogStartDepth);

    std3D_fogColor[0] = red;
    std3D_fogColor[1] = green;
    std3D_fogColor[2] = blue;
    std3D_fogColor[3] = 1.0f;
}

void std3D_ClearZBuffer(void)
{
    glClearStencil(0);
    glDepthMask(GL_TRUE);
    glClear(GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
}

static int std3D_BuildDeviceList(void)
{
    std3D_numDevices = 0;

    // Get display devices from stdDisplay module
    size_t numDisplayDevices = stdDisplay_GetNumDevices();
    if ( numDisplayDevices == 0 )
    {
        return 0;
    }

    for ( size_t i = 0; i < numDisplayDevices && std3D_numDevices < STD_ARRAYLEN(std3D_aDevices); ++i )
    {
        StdDisplayDevice displayDevice = { 0 };
        if ( stdDisplay_GetDevice(i, &displayDevice) )
        {
            continue;
        }

        // Skip non-HAL devices if not finding all devices
        if ( !std3D_bFindAllD3Devices && !displayDevice.bHAL )
        {
            continue;
        }

        Device3D* pD3DDriver = &std3D_aDevices[std3D_numDevices];
        ZeroMemory(pD3DDriver, sizeof(Device3D));

        STD_STRCPY(pD3DDriver->deviceDescription, displayDevice.aDeviceName);
        STD_STRCPY(pD3DDriver->deviceName, displayDevice.aDriverName);

        pD3DDriver->bHAL                         = displayDevice.bHAL;
        pD3DDriver->bTexturePerspectiveSupported = TRUE;  // Always supported in GL
        pD3DDriver->hasZBuffer                   = TRUE;  // Always supported in GL
        pD3DDriver->bSqareOnlyTexture            = TRUE;  // Always supported in GL
        pD3DDriver->bAlphaTextureSupported       = TRUE;  // Always supported in GL
        pD3DDriver->bColorkeyTextureSupported    = TRUE;  // // Always supported in GL
        pD3DDriver->bStippledShadeSupported      = FALSE; // ?
        pD3DDriver->minTexWidth                  = 1;
        pD3DDriver->minTexHeight                 = 1;

        GLint maxTextureSize = 0;
        glGetIntegerv(GL_MAX_TEXTURE_SIZE, &maxTextureSize);

        pD3DDriver->maxTexWidth                    = maxTextureSize;
        pD3DDriver->maxTexHeight                   = maxTextureSize;
        pD3DDriver->bAnisotropicFilteringSupported = GLAD_GL_EXT_texture_filter_anisotropic;
        pD3DDriver->bMipmapAutoGenSupported        = TRUE;

        pD3DDriver->bAlphaBlendSupported = TRUE;

        pD3DDriver->maxVertexCount = STD3D_MAX_VERTICES_PER_DRAW;
        if ( pD3DDriver->maxVertexCount == 0 )
        {
            pD3DDriver->maxVertexCount = 65535; // Reasonable default
        }

        pD3DDriver->totalMemory     = displayDevice.totalVideoMemory;
        pD3DDriver->availableMemory = displayDevice.freeVideoMemory;

        // TODO: proly no point to make log here since same info can be logged in stdDisplay
        STDLOG_STATUS("Found |%s|%s|%s|%s| D3D Device\n",
                      pD3DDriver->hasZBuffer ? "Z" : "Non-Z",
                      pD3DDriver->bAlphaTextureSupported ? "Alpha" : "No Alpha",
                      pD3DDriver->bStippledShadeSupported ? "Stippled" : "Blend",
                      pD3DDriver->bColorkeyTextureSupported ? "Colorkey"
                      : "No Colorkey");

        STDLOG_STATUS("Description: %s [%s]\n", pD3DDriver->deviceName, pD3DDriver->deviceDescription);

        ++std3D_numDevices;
    }

    return std3D_numDevices > 0;
}

int std3D_CreateViewport(void)
{
    GLint width  = (GLint)stdDisplay_g_backBuffer.rasterInfo.width;
    GLint height = (GLint)stdDisplay_g_backBuffer.rasterInfo.height;

    glViewport(0, 0, width, height);

    std3D_activeRect.x1 = 0;
    std3D_activeRect.y1 = 0;
    std3D_activeRect.x2 = width;
    std3D_activeRect.y2 = height;

    return 1;
}

void J3DAPI std3D_AddTextureToCacheList(tSystemTexture* pTexture)
{
    if ( std3D_pFirstTexCache )
    {
        std3D_pLastTexCache->pNextCachedTexture = pTexture;
        pTexture->pPrevCachedTexture            = std3D_pLastTexCache;
        pTexture->pNextCachedTexture            = NULL;
        std3D_pLastTexCache                     = pTexture;
    }
    else
    {
        std3D_pLastTexCache          = pTexture;
        std3D_pFirstTexCache         = pTexture;
        pTexture->pPrevCachedTexture = NULL;
        pTexture->pNextCachedTexture = NULL;
    }

    ++std3D_numCachedTextures;
    std3D_pCurDevice->availableMemory -= pTexture->textureSize;
}

void J3DAPI std3D_RemoveTextureFromCacheList(tSystemTexture* pCacheTexture)
{
    if ( pCacheTexture == std3D_pFirstTexCache )
    {
        std3D_pFirstTexCache = pCacheTexture->pNextCachedTexture;
        if ( std3D_pFirstTexCache )
        {
            std3D_pFirstTexCache->pPrevCachedTexture = NULL;
            if ( !std3D_pFirstTexCache->pNextCachedTexture )
            {
                std3D_pLastTexCache = std3D_pFirstTexCache;
            }
        }
        else
        {
            std3D_pLastTexCache = NULL;
        }
    }
    else if ( pCacheTexture == std3D_pLastTexCache )
    {
        std3D_pLastTexCache                                   = pCacheTexture->pPrevCachedTexture;
        pCacheTexture->pPrevCachedTexture->pNextCachedTexture = NULL;
    }
    else
    {
        pCacheTexture->pPrevCachedTexture->pNextCachedTexture = pCacheTexture->pNextCachedTexture;
        pCacheTexture->pNextCachedTexture->pPrevCachedTexture = pCacheTexture->pPrevCachedTexture;
    }

    pCacheTexture->pNextCachedTexture = NULL;
    pCacheTexture->pPrevCachedTexture = NULL;
    pCacheTexture->frameNum           = 0;

    --std3D_numCachedTextures;
    std3D_pCurDevice->availableMemory += pCacheTexture->textureSize;
}

int J3DAPI std3D_PurgeTextureCache(size_t size)
{
    size_t purgedBytes = 0;
    for ( tSystemTexture* pCacheTexture = std3D_pFirstTexCache;
          pCacheTexture && pCacheTexture->frameNum != std3D_frameCount;
          pCacheTexture = pCacheTexture->pNextCachedTexture )
    {
        if ( pCacheTexture->textureSize == size )
        {
            glDeleteTextures(1, &pCacheTexture->pCachedTexture->id);
            STDFREE(pCacheTexture->pCachedTexture);
            pCacheTexture->pCachedTexture = NULL;
            std3D_RemoveTextureFromCacheList(pCacheTexture);
            return 1;
        }
    }

    tSystemTexture* pNextCachedTexture = NULL;
    for ( tSystemTexture* pCacheTexture = std3D_pFirstTexCache; pCacheTexture && purgedBytes < size; pCacheTexture = pNextCachedTexture )
    {
        pNextCachedTexture = pCacheTexture->pNextCachedTexture;
        if ( pCacheTexture->frameNum != std3D_frameCount )
        {
            if ( pCacheTexture->pCachedTexture )
            {
                glDeleteTextures(1, &pCacheTexture->pCachedTexture->id);
                STDFREE(pCacheTexture->pCachedTexture);
            }
            pCacheTexture->pCachedTexture = NULL;
            purgedBytes += pCacheTexture->textureSize;
            std3D_RemoveTextureFromCacheList(pCacheTexture);
        }
    }

    return purgedBytes != 0;
}

StdDisplayEnvironment* J3DAPI std3D_BuildDisplayEnvironment()
{
    StdDisplayEnvironment* pDeviceList = (StdDisplayEnvironment*)STDMALLOC(sizeof(StdDisplayEnvironment));
    memset(pDeviceList, 0, sizeof(StdDisplayEnvironment));

    if ( !stdDisplay_Startup() )
    {
        STDLOG_ERROR("Error starting stdDisplay system.\n");
        std3D_FreeDisplayEnvironment(pDeviceList);
        return NULL;
    }

    pDeviceList->numInfos      = stdDisplay_GetNumDevices();
    pDeviceList->aDisplayInfos = NULL;

    if ( pDeviceList->numInfos )
    {
        pDeviceList->aDisplayInfos = (StdDisplayInfo*)STDMALLOC(sizeof(StdDisplayInfo) * pDeviceList->numInfos);

        StdDisplayInfo* pCurInfo = pDeviceList->aDisplayInfos;
        for ( size_t deviceNum = 0; deviceNum < pDeviceList->numInfos; ++deviceNum )
        {
            memset(pCurInfo, 0, sizeof(StdDisplayInfo));

            if ( stdDisplay_GetDevice(deviceNum, &pCurInfo->displayDevice) )
            {
                STDLOG_ERROR("Error getting stdDisplay device.\n");
                std3D_FreeDisplayEnvironment(pDeviceList);
                return NULL;
            }

            if ( !stdDisplay_Open(deviceNum) )
            {
                STDLOG_ERROR("Error opening stdDisplay device.\n");
                std3D_FreeDisplayEnvironment(pDeviceList);
                return NULL;
            }

            pCurInfo->numModes = stdDisplay_GetNumVideoModes();
            pCurInfo->aModes   = NULL;

            if ( pCurInfo->numModes )
            {
                pCurInfo->aModes = (StdVideoMode*)STDMALLOC(sizeof(StdVideoMode) *
                    pCurInfo->numModes);
                StdVideoMode* pCurMode = pCurInfo->aModes;
                for ( size_t modeNum = 0; modeNum < pCurInfo->numModes; ++modeNum )
                {
                    if ( !stdDisplay_GetVideoMode(modeNum, pCurMode) )
                    {
                        ++pCurMode;
                    }
                }

                pCurInfo->numDevices = 0;
                pCurInfo->aDevices   = NULL;
                if ( pCurInfo->displayDevice.bHAL )
                {
                    if ( !std3D_Startup() )
                    {
                        STDLOG_ERROR("Error starting std3D system.\n");
                        std3D_FreeDisplayEnvironment(pDeviceList);
                        return NULL;
                    }

                    pCurInfo->numDevices = std3D_GetNumDevices();
                    if ( pCurInfo->numDevices )
                    {
                        size_t listSize    = sizeof(Device3D) * pCurInfo->numDevices;
                        pCurInfo->aDevices = (Device3D*)STDMALLOC(listSize);
                        memcpy(pCurInfo->aDevices, std3D_GetAllDevices(), listSize);
                    }

                    std3D_Shutdown();
                }
            }

            stdDisplay_Close();
            ++pCurInfo;
        }
    }

    stdDisplay_Shutdown();
    return pDeviceList;
}

void J3DAPI std3D_FreeDisplayEnvironment(StdDisplayEnvironment* pEnv)
{
    if ( pEnv->aDisplayInfos )
    {
        StdDisplayInfo* pCurDevice = pEnv->aDisplayInfos;
        for ( size_t i = 0; i < pEnv->numInfos; ++i )
        {
            if ( pCurDevice->aDevices )
            {
                stdMemory_Free(pCurDevice->aDevices);
                pCurDevice->aDevices = NULL;
            }

            if ( pCurDevice->aModes )
            {
                stdMemory_Free(pCurDevice->aModes);
                pCurDevice->aModes = NULL;
            }

            ++pCurDevice;
        }

        if ( pEnv->aDisplayInfos )
        {
            stdMemory_Free(pEnv->aDisplayInfos);
            pEnv->aDisplayInfos = NULL;
        }
    }

    stdMemory_Free(pEnv);
}

void J3DAPI std3D_SetFindAllDevices(int bFindAll)
{
    std3D_bFindAllD3Devices = bFindAll;
}

bool std3D_InitVertexBuffers(GLuint* vbo, GLuint* ebo, GLuint* vao)
{
    glGenVertexArrays(1, vao);
    glBindVertexArray(*vao);

    // create vbo
    glGenBuffers(1, vbo);
    glBindBuffer(GL_ARRAY_BUFFER, *vbo);
    glBufferData(GL_ARRAY_BUFFER, STD3D_MAX_VERTICES_PER_DRAW * sizeof(D3DTLVERTEX), NULL, GL_STREAM_DRAW);

    // create ibo
    glGenBuffers(1, ebo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, *ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, STD3D_MAX_INDICES_PER_DRAW * sizeof(GLushort), NULL, GL_STREAM_DRAW);

    const GLsizei stride = sizeof(D3DTLVERTEX);

    // set attributes
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, stride, (void*)offsetof(D3DTLVERTEX, sx));
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 4, GL_UNSIGNED_BYTE, GL_TRUE, stride, (void*)offsetof(D3DTLVERTEX, color));
    glEnableVertexAttribArray(1);

    glVertexAttribPointer(2, 4, GL_UNSIGNED_BYTE, GL_TRUE, stride, (void*)offsetof(D3DTLVERTEX, specular));
    glEnableVertexAttribArray(2);

    glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, stride, (void*)offsetof(D3DTLVERTEX, tu));
    glEnableVertexAttribArray(3);

    glBindVertexArray(0);

    return true;
}

static void std3D_MapScreenSpaceBuffers(void)
{
    glBindVertexArray(std3D_screenSpaceVao);
    glBindBuffer(GL_ARRAY_BUFFER, std3D_screenSpaceVbo);
    //std3D_pScreenSpaceVertexBuffer = glMapBufferRange(GL_ARRAY_BUFFER, 0, STD3D_MAX_VERTICES_PER_DRAW * sizeof(D3DTLVERTEX), GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, std3D_screenSpaceEbo);
    //std3D_pScreenSpaceElementBuffer = glMapBufferRange(GL_ELEMENT_ARRAY_BUFFER, 0, STD3D_MAX_INDICES_PER_DRAW * sizeof(GLushort), GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT);

    std3D_bScreenSpaceBuffersMapped = true;
}

static void std3D_UnmapScreenSpaceBuffers(void)
{
    glBindVertexArray(std3D_screenSpaceVao);

    // Upload vertex data
    glBindBuffer(GL_ARRAY_BUFFER, std3D_screenSpaceVbo);
    glUnmapBuffer(GL_ARRAY_BUFFER);

    // Upload index data;
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, std3D_screenSpaceEbo);
    glUnmapBuffer(GL_ELEMENT_ARRAY_BUFFER);

    std3D_bScreenSpaceBuffersMapped = false;

    std3D_numScreenSpaceVertices = 0;
    std3D_numScreenSpaceIndices  = 0;
}

size_t std3D_AddScreenSpaceVertices(LPD3DTLVERTEX aVertices, size_t numVertices, LPWORD aIndices, size_t numIndices, GLenum type)
{
    // if ( !std3D_bScreenSpaceBuffersMapped )
    // {
    //     std3D_MapScreenSpaceBuffers();
    // }

    if ( std3D_numScreenSpaceVertices + numVertices > STD3D_MAX_VERTICES_PER_DRAW || std3D_numScreenSpaceIndices + numIndices > STD3D_MAX_INDICES_PER_DRAW )
    {
        std3D_DrawLegacyBatches();
    }

    size_t baseVertex  = std3D_numScreenSpaceVertices;
    size_t indexOffset = std3D_numScreenSpaceIndices;

    LPD3DTLVERTEX pVertices = &std3D_pScreenSpaceVertexBuffer[baseVertex];
    memcpy(pVertices, aVertices, sizeof(D3DTLVERTEX) * numVertices);

    if ( type == GL_TRIANGLES )
    {
        for ( size_t i = 0; i < numIndices; ++i )
        {
            std3D_pScreenSpaceElementBuffer[indexOffset + i] = aIndices[i] + baseVertex;
        }
    }
    else if ( type == GL_LINES ) //for line strip, just create a pair of indices for each line
    {
        for ( size_t i = 0; i < numVertices - 1; ++i )
        {
            std3D_pScreenSpaceElementBuffer[indexOffset + i * 2]     = baseVertex + i;
            std3D_pScreenSpaceElementBuffer[indexOffset + i * 2 + 1] = baseVertex + i + 1;
        }
    }


    std3D_numScreenSpaceVertices += numVertices;
    std3D_numScreenSpaceIndices += numIndices;

    return indexOffset;
}

void std3D_ReleaseScreenSpaceBuffers(void)
{
    if ( std3D_screenSpaceVbo )
    {
        glDeleteBuffers(1, &std3D_screenSpaceVbo);
        std3D_screenSpaceVbo = 0;
    }

    if ( std3D_screenSpaceEbo )
    {
        glDeleteBuffers(1, &std3D_screenSpaceEbo);
        std3D_screenSpaceEbo = 0;
    }
}

void std3D_ReleaseStaticBuffers(void)
{
    if ( std3D_staticVao > 0 )
    {
        glDeleteVertexArrays(1, &std3D_staticVao);
        glDeleteBuffers(1, &std3D_staticVbo);
        glDeleteBuffers(1, &std3D_staticEbo);
    }
}

bool std3D_InitShaderSystem(void)
{
    if ( !stdShader_Open() )
    {
        return false;
    }

    // Create default shader
    std3D_pDefaultShader = stdShader_CompileAndCreate("std_default", "default.vert", "default.frag");

    if ( !std3D_pDefaultShader )
    {
        STDLOG_ERROR("Failed to create default shader\n");
        return false;
    }

    // Creade model shader
    std3D_pModelShader = stdShader_CompileAndCreate("std_model", "model.vert", "model.frag");

    if ( !std3D_pModelShader )
    {
        STDLOG_ERROR("Failed to create model shader\n");
        return false;
    }

    // Create ceiling sky shader
    std3D_pCeilingSkyShader = stdShader_CompileAndCreate("std_ceilingSky", "ceilingSky.vert", "ceilingSky.frag");

    if ( !std3D_pCeilingSkyShader )
    {
        STDLOG_ERROR("Failed to create ceiling sky shader\n");
        return false;
    }

    // Create horizon sky shader
    std3D_pHorizonSkyShader = stdShader_CompileAndCreate("std_horizonSky", "horizonSky.vert", "horizonSky.frag");

    if ( !std3D_pHorizonSkyShader )
    {
        STDLOG_ERROR("Failed to create horizon sky shader\n");
        return false;
    }

    // Create sprite shader
    std3D_pSpriteShader = stdShader_CompileAndCreate("std_sprite", "sprite.vert", "default.frag");

    if ( !std3D_pSpriteShader )
    {
        STDLOG_ERROR("Failed to create sprite shader\n");
        return false;
    }

    // Create particle shader
    std3D_pParticleShader = stdShader_CompileAndCreate("std_particle", "particle.vert", "default.frag");

    if ( !std3D_pParticleShader )
    {
        STDLOG_ERROR("Failed to create particle shader\n");
        return false;
    }

    // Create polyline shader
    std3D_pPolyLineShader = stdShader_CompileAndCreate("std_polyline", "polyline.vert", "default.frag");

    if ( !std3D_pPolyLineShader )
    {
        STDLOG_ERROR("Failed to create polyline shader\n");
        return false;
    }

    // Create legacy shader
    std3D_pLegacyShader = stdShader_CompileAndCreate("std_legacy", "legacy.vert", "legacy.frag");

    if ( !std3D_pPolyLineShader )
    {
        STDLOG_ERROR("Failed to create legacy shader\n");
        return false;
    }

    return true;
}

void std3D_ShutdownShaderSystem(void)
{
    std3D_pDefaultShader = NULL;
    stdShader_Close();
}

bool std3D_IsAnisotropicFilteringSupported(void) { return true; }

bool std3D_IsMipmapAutoGenSupported(void) { return true; }

bool std3D_IsMSAASupported(void) { return true; }

void std3D_UpdateCeilingSky(float height, float offsetX, float offsetY)
{
    if ( !std3D_pCeilingSkyShader )
    {
        STDLOG_ERROR("Ceiling sky shader was not initialized yet");
    }

    stdShader_SetActiveShader(std3D_pCeilingSkyShader);
    glUniform1f(glGetUniformLocation(std3D_pCeilingSkyShader->handle, "uCeilingZ"), height);
    glUniform2f(glGetUniformLocation(std3D_pCeilingSkyShader->handle, "ceilingSkyOffset"), offsetX, offsetY);
}

void std3D_UpdateHorizonSky(float camPitch, float camYaw, float scale, float horizonOffsetX, float horizonOffsetY)
{
    if ( !std3D_pHorizonSkyShader )
    {
        STDLOG_ERROR("Horizon sky shader was not initialized yet");
    }
    stdShader_SetActiveShader(std3D_pHorizonSkyShader);
    glUniform1f(glGetUniformLocation(std3D_pHorizonSkyShader->handle, "horizonScale"), scale);
    glUniform3f(glGetUniformLocation(std3D_pHorizonSkyShader->handle, "camPYR"), camPitch, camYaw, 0);
    glUniform2f(glGetUniformLocation(std3D_pHorizonSkyShader->handle, "horizonSkyOffset"), horizonOffsetX, horizonOffsetY);
}


void std3D_InitGeometryVBO(LPD3DTLVERTEX vertices, size_t numVertices, GLuint* indices, size_t numIndices)
{
    if ( std3D_staticVao > 0 )
    {
        glDeleteVertexArrays(1, &std3D_staticVao);
        glDeleteBuffers(1, &std3D_staticVbo);
        glDeleteBuffers(1, &std3D_staticEbo);
    }
    glGenVertexArrays(1, &std3D_staticVao);
    glBindVertexArray(std3D_staticVao);

    // create vbo
    glGenBuffers(1, &std3D_staticVbo);
    glBindBuffer(GL_ARRAY_BUFFER, std3D_staticVbo);
    glBufferData(GL_ARRAY_BUFFER, numVertices * sizeof(D3DTLVERTEX), vertices, GL_STATIC_DRAW);

    // create ebo
    glGenBuffers(1, &std3D_staticEbo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, std3D_staticEbo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, numIndices * sizeof(GLuint), indices, GL_STATIC_DRAW);

    const GLsizei stride = sizeof(D3DTLVERTEX);

    // set attributes
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, stride, (void*)offsetof(D3DTLVERTEX, sx));
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 4, GL_UNSIGNED_BYTE, GL_TRUE, stride, (void*)offsetof(D3DTLVERTEX, color));
    glEnableVertexAttribArray(1);

    glVertexAttribPointer(2, 4, GL_UNSIGNED_BYTE, GL_TRUE, stride, (void*)offsetof(D3DTLVERTEX, specular));
    glEnableVertexAttribArray(2);

    glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, stride, (void*)offsetof(D3DTLVERTEX, tu));
    glEnableVertexAttribArray(3);

    glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, stride, (void*)offsetof(D3DTLVERTEX, nx));
    glEnableVertexAttribArray(4);

    glEnableVertexAttribArray(5);
    glEnableVertexAttribArray(6);
    glEnableVertexAttribArray(7);
    glEnableVertexAttribArray(8);
    glEnableVertexAttribArray(9);
    glEnableVertexAttribArray(10);
    glEnableVertexAttribArray(11);

    glVertexAttribDivisor(5, 1);
    glVertexAttribDivisor(6, 1);
    glVertexAttribDivisor(7, 1);
    glVertexAttribDivisor(8, 1);
    glVertexAttribDivisor(9, 1);
    glVertexAttribDivisor(10, 1);
    glVertexAttribDivisor(11, 1);

    glBindVertexArray(0);
}

void std3D_InitInstanceVBO(const size_t maxInstances)
{
    if ( std3D_instanceBuffer > 0 )
    {
        glDeleteBuffers(1, &std3D_instanceBuffer);
    }

    glGenBuffers(1, &std3D_instanceBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, std3D_instanceBuffer);
    glBufferData(GL_ARRAY_BUFFER, maxInstances * sizeof(InstanceData), NULL, GL_DYNAMIC_DRAW);
}

void std3D_UpdateInstanceVBO(const InstanceData* pData, size_t numInstances)
{
    if ( numInstances == 0 )
        return;

    glBindVertexArray(std3D_staticVao);
    glBindBuffer(GL_ARRAY_BUFFER, std3D_instanceBuffer);
    glBufferSubData(GL_ARRAY_BUFFER, 0, numInstances * sizeof(InstanceData), pData);
    std3D_instanceOffset = 0;
}

static void std3D_UpdateInstancePointers()
{
    size_t base    = std3D_instanceOffset * sizeof(InstanceData);
    GLsizei stride = sizeof(InstanceData);

    glBindBuffer(GL_ARRAY_BUFFER, std3D_instanceBuffer);

    // mat4 (locations 5–8)
    glVertexAttribPointer(5, 4, GL_FLOAT, GL_FALSE, stride, (void*)(base + 0));
    glVertexAttribPointer(6, 4, GL_FLOAT, GL_FALSE, stride, (void*)(base + 16));
    glVertexAttribPointer(7, 4, GL_FLOAT, GL_FALSE, stride, (void*)(base + 32));
    glVertexAttribPointer(8, 4, GL_FLOAT, GL_FALSE, stride, (void*)(base + 48));

    // extraLight
    glVertexAttribPointer(
        9, 4, GL_UNSIGNED_BYTE, GL_TRUE,
        stride, (void*)(base + offsetof(InstanceData, extraLight))
    );

    // secLightPos
    glVertexAttribPointer(
        10, 4, GL_FLOAT, GL_FALSE,
        stride, (void*)(base + offsetof(InstanceData, secLightPos))
    );

    // secLightColor
    glVertexAttribPointer(
        11, 4, GL_FLOAT, GL_FALSE,
        stride, (void*)(base + offsetof(InstanceData, secLightColor))
    );
}


void std3D_DrawGeometryBatch(GeometryBatch* pBatch)
{
    glBindVertexArray(std3D_staticVao);

    stdShader_SetActiveTextureUnit(TU_3D_DRAW);

    if ( pBatch->pShader != NULL )
    {
        std3D_pActiveShader = pBatch->pShader;
    }
    else
    {
        std3D_pActiveShader = std3D_pDefaultShader;
    }

    stdShader_SetActiveShader(std3D_pActiveShader);

    const GLuint texID = std3D_currentDrawMode == DM_FULL && pBatch->pTex ? pBatch->pTex->id : std3D_pWhiteTexture->id;

    stdShader_SetTexture(std3D_pActiveShader, texID);
    std3D_SetRenderState(pBatch->rdFlags);

    glUniform3f(std3D_pActiveShader->extraLightLoc, pBatch->extraLight[0], pBatch->extraLight[1], pBatch->extraLight[2]);
    glUniform1i(std3D_pActiveShader->lightModeLoc, pBatch->lightMode);
    glUniform1f(std3D_pActiveShader->alphaLoc, pBatch->extraLight[3]);
    glUniform1i(std3D_pActiveShader->renderLightsLoc, pBatch->lightMode == 3);
    glUniform1i(std3D_pActiveShader->alphaCutLoc, (pBatch->rdFlags & STD3D_RS_ALPHAREF_SET) != 0);

    STD_ASSERTREL(glIsVertexArray(std3D_staticVao));


    glMultiDrawElements(GL_TRIANGLES, pBatch->indexCounts, GL_UNSIGNED_INT, (const void* const*)pBatch->indexOffsets, pBatch->drawCount);
    std3D_numDrawCalls++;
}

void std3D_DrawModelBatch(ModelBatch* pBatch)
{
    glBindVertexArray(std3D_staticVao);

    std3D_UpdateInstancePointers();

    stdShader_SetActiveTextureUnit(TU_3D_DRAW);

    if ( pBatch->pShader != NULL )
    {
        std3D_pActiveShader = pBatch->pShader;
    }
    else
    {
        std3D_pActiveShader = std3D_pModelShader;
    }

    stdShader_SetActiveShader(std3D_pActiveShader);

    GLuint texID = std3D_currentDrawMode == DM_FULL && pBatch->pTex ? pBatch->pTex->id : std3D_pWhiteTexture->id;
    stdShader_SetTexture(std3D_pActiveShader, texID);
    std3D_SetRenderState(pBatch->rdFlags);

    glUniform1i(std3D_pActiveShader->lightModeLoc, pBatch->lightMode);
    glUniform1i(std3D_pActiveShader->renderLightsLoc, pBatch->lightMode == 3);
    glUniform1i(std3D_pActiveShader->alphaCutLoc, (pBatch->rdFlags & STD3D_RS_ALPHAREF_SET) != 0);

    glDrawElementsInstanced(GL_TRIANGLES, (GLsizei)pBatch->indexCount, GL_UNSIGNED_INT, (void*)(pBatch->indexOffset * sizeof(GLuint)), pBatch->numberOfInstances);
    std3D_instanceOffset += pBatch->numberOfInstances;
    std3D_numDrawCalls++;
}

void std3D_DrawQuadBatch(QuadBatch* pBatch)
{
    glBindVertexArray(std3D_staticVao);

    std3D_UpdateInstancePointers();

    GLuint texID = std3D_currentDrawMode == DM_FULL && pBatch->pTex ? pBatch->pTex->id : std3D_pWhiteTexture->id;
    stdShader_SetActiveTextureUnit(TU_3D_DRAW);

    if ( pBatch->pShader != NULL )
    {
        std3D_pActiveShader = pBatch->pShader;
    }
    else
    {
        std3D_pActiveShader = std3D_pSpriteShader;
    }

    stdShader_SetActiveShader(std3D_pActiveShader);
    stdShader_SetTexture(std3D_pActiveShader, texID);
    std3D_SetRenderState(pBatch->rdFlags);
    glUniform1i(std3D_pActiveShader->spriteTypeLoc, pBatch->spriteType);

    glDrawArraysInstanced(GL_TRIANGLE_FAN, 0, 4, pBatch->numberOfInstances);
    std3D_instanceOffset += pBatch->numberOfInstances;

    std3D_numDrawCalls++;
}

void std3D_CacheLegacyBatch(const LegacyBatch batch)
{
    std3D_cachedLegacyBatches[std3D_numCachedLegacyBatches++] = batch;
}

void std3D_DrawLegacyBatches(void)
{
    // if ( !std3D_bScreenSpaceBuffersMapped )
    // {
    //     return;
    // }
    // std3D_UnmapScreenSpaceBuffers();

    glBindVertexArray(std3D_screenSpaceVao);

    glBindBuffer(GL_ARRAY_BUFFER, std3D_screenSpaceVbo);
    glBufferSubData(GL_ARRAY_BUFFER, 0, std3D_numScreenSpaceVertices * sizeof(D3DTLVERTEX), std3D_pScreenSpaceVertexBuffer);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, std3D_screenSpaceEbo);
    glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, std3D_numScreenSpaceIndices * sizeof(GLushort), std3D_pScreenSpaceElementBuffer);

    std3D_numScreenSpaceVertices = 0;
    std3D_numScreenSpaceIndices  = 0;

    stdShader_SetActiveTextureUnit(TU_3D_DRAW);
    std3D_pActiveShader = std3D_pLegacyShader;

    stdShader_SetActiveShader(std3D_pActiveShader);

    for ( size_t i = 0; i < std3D_numCachedLegacyBatches; i++ )
    {
        LegacyBatch* pBatch = &std3D_cachedLegacyBatches[i];
        GLuint texID        = std3D_currentDrawMode == DM_FULL && pBatch->pTex ? pBatch->pTex->id : std3D_pWhiteTexture->id;

        stdShader_SetTexture(std3D_pActiveShader, texID);
        std3D_SetRenderState(pBatch->rdFlags);
        glUniform1i(std3D_pActiveShader->alphaCutLoc, (pBatch->rdFlags & STD3D_RS_ALPHAREF_SET) != 0);
        glMultiDrawElements(pBatch->type, pBatch->indexCounts, GL_UNSIGNED_SHORT, (const void* const*)pBatch->indexOffsets, pBatch->drawCount);
        std3D_numDrawCalls++;
    }

    std3D_numCachedLegacyBatches = 0;
}

void J3DAPI std3D_DrawRenderList(tSysTexture* pTex, Std3DRenderState rdflags, LPD3DTLVERTEX aVerts, size_t numVerts, LPWORD aIndices, size_t numIndices)
{
    STDLOG_ERROR("Usage of std3D_DrawRenderList is not supported in OpenGL! Use rdCache_AddLegacyDrawCall instead.\n");
}

void std3D_SetDrawMode(const int mode)
{
    stdShader_SetActiveShader(std3D_pLegacyShader);
    switch ( mode )
    {
        case DM_VERTEX:
            glUniform3f(std3D_pLegacyShader->extraLightLoc, 1.0f, 1.0f, 1.0f);
            glPolygonMode(GL_FRONT_AND_BACK, GL_POINT);
            break;
        case DM_WIREFRAME:
            glUniform3f(std3D_pLegacyShader->extraLightLoc, 1.0f, 1.0f, 1.0f);
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
            break;
        case DM_SOLID:
        case DM_FULL:
            glUniform3f(std3D_pLegacyShader->extraLightLoc, 0.0f, 0.0f, 0.0f);
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        default:
            break;
    }
    stdShader_SetActiveShader(std3D_pActiveShader);
    std3D_currentDrawMode = mode;
}



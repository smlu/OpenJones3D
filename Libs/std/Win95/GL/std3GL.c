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

#include <math.h>

#define STD3D_DEFAULT_MAX_VERTICES 512

static bool bStartup    = false;
static bool std3D_bOpen = false;

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

const size_t std3D_maxVerticesPerDrawCall = 65536;  // max vertices which can be drawn in one draw call
const size_t std3D_maxIndicesPerDrawCall  = 131072; // max indices which can be drawn in one draw call
const size_t std3D_maxDrawCallGroupSize   = 16384;  // max amount of draw calls which can be summarized in a group

// structure for caching a draw call
typedef struct sGLDrawCall
{
    size_t firstIndex;
    GLsizei indexCount;
    tSysTexture* tex;
    Std3DRenderState rdflags;
    GLenum type; // GL_TRIANGLES / GL_LINES / GL_POINTS
} GLDrawCall;

typedef struct sFrameBatch
{
    D3DTLVERTEX* verts;
    size_t vertCapacity;
    size_t vertCount;

    GLushort* indices;
    size_t indexCapacity;
    size_t indexCount;

    GLDrawCall* draws;
    size_t drawCapacity;
    size_t drawCount;
} FrameBatch;

static FrameBatch std3D_frameBatch = { 0 };

// Global state

// VBO & IBO
static GLuint std3D_pVertexArrayObject  = 0;
static GLuint std3D_pVertexBufferOpaque = 0;

static GLuint std3D_pIndexBuffer = 0;
static size_t std3D_numDrawCalls = 0;

static GLuint std3D_activeSampler = 0;

// Shader system state
static GLShaderProgram* std3D_defaultShader    = NULL;
static GLShaderProgram* std3D_defaultShaderWf  = NULL;
static GLShaderProgram* std3D_ceilingSkyShader = NULL;
static GLShaderProgram* std3D_horizonSkyShader = NULL;
static GLShaderProgram* std3D_activeShader     = NULL;

static int std3D_InitRenderState(void);
static int std3D_BuildDeviceList(void);

static int std3D_CreateViewport(void);
static void J3DAPI std3D_AddTextureToCacheList(tSystemTexture* pTexture);
static void J3DAPI
std3D_RemoveTextureFromCacheList(tSystemTexture* pCacheTexture);
static int J3DAPI std3D_PurgeTextureCache(size_t size);

bool std3D_InitVertexBuffers(GLuint* vbo, GLuint* vao, GLuint* vio);
void std3D_ReleaseVertexBuffers(void);

bool std3D_InitShaderSystem(void);
void std3D_ShutdownShaderSystem(void);

static void std3D_DrawFrameBatch(void);                                       //new
static bool std3D_EnsureDrawCapacity(size_t extraVerts, size_t extraIndices); //new
static void std3D_MapVertexBuffers(void);                                     //new

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
    memcpy(&std3D_g_fogDensity, &std3D_g_fogDensity_tmp,
           sizeof(std3D_g_fogDensity));
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

    //stdShader_Shutdown();

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

    // create 1x1 white texture for solid mode
    std3D_pWhiteTexture = STDMALLOC(sizeof(tSysTexture));
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

    std3D_InitVertexBuffers(&std3D_pVertexBufferOpaque, &std3D_pIndexBuffer, &std3D_pVertexArrayObject);


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
    std3D_ReleaseVertexBuffers();
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
        *pDest         = stdColor_cfARGB8888;
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
    std3D_MapVertexBuffers();
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
    if ( std3D_frameBatch.drawCount > 0 )
    {
        std3D_DrawFrameBatch();
    }
    std3D_renderState  = 0;
    std3D_numDrawCalls = 0;
    std3D_pD3DTex      = NULL;
    glSamplerParameteri(std3D_activeSampler, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glSamplerParameteri(std3D_activeSampler, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glBindSampler(TU_3D_DRAW, 0);
    stdShader_DisableFog();
}

int std3D_CacheDrawCall(GLenum type, tSysTexture* pTex, Std3DRenderState rdflags, LPD3DTLVERTEX aVerts, size_t numVerts, LPWORD aIndices, size_t numIndices)
{
    if ( type == GL_LINES )
    {
        numIndices = (numVerts - 1) * 2;
    }
    else if ( type == GL_POINTS )
    {
        numIndices = numVerts;
    }

    if ( !std3D_EnsureDrawCapacity(numVerts, numIndices) )
    {
        std3D_DrawFrameBatch();
        std3D_MapVertexBuffers();
    }

    size_t baseVertex = std3D_frameBatch.vertCount;
    size_t firstIndex = std3D_frameBatch.indexCount;

    // paste new vertices
    memcpy(&std3D_frameBatch.verts[baseVertex], aVerts, numVerts * sizeof(D3DTLVERTEX));
    std3D_frameBatch.vertCount += numVerts;

    // paste new indices
    if ( type == GL_TRIANGLES )
    {
        for ( size_t i = 0; i < numIndices; i++ )
        {
            std3D_frameBatch.indices[firstIndex + i] = aIndices[i] + baseVertex;
        }
    }
    else if ( type == GL_LINES ) //for line strip, just create a pair of indices for each line
    {
        for ( size_t i = 0; i < numVerts - 1; i++ )
        {
            std3D_frameBatch.indices[firstIndex + i * 2]     = baseVertex + i;
            std3D_frameBatch.indices[firstIndex + i * 2 + 1] = baseVertex + i + 1;
        }
    }
    else if ( type == GL_POINTS ) // for points, just give each vertex an index.
    {
        for ( size_t i = 0; i < numVerts - 1; i++ )
        {
            std3D_frameBatch.indices[firstIndex + i] = baseVertex + i;
        }
    }
    else
    {
        STDLOG_ERROR("Unknown draw type %d.\n", type);
        return 0;
    }

    std3D_frameBatch.indexCount += numIndices;

    // cache draw call
    GLDrawCall* dc = &std3D_frameBatch.draws[std3D_frameBatch.drawCount++];
    dc->firstIndex = firstIndex;
    dc->indexCount = (GLsizei)numIndices;
    dc->tex        = pTex ? pTex : std3D_pWhiteTexture;
    dc->rdflags    = rdflags;
    dc->type       = type;

    return 1;
}

static void std3D_DrawFrameBatch(void)
{
    if ( !std3D_frameBatch.vertCount || !std3D_frameBatch.indexCount )
    {
        return;
    }
    stdShader_SetActiveTextureUnit(TU_3D_DRAW);

    glBindVertexArray(std3D_pVertexArrayObject);

    // Upload vertex data
    glBindBuffer(GL_ARRAY_BUFFER, std3D_pVertexBufferOpaque);
    glUnmapBuffer(GL_ARRAY_BUFFER);

    // Upload index data;
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, std3D_pIndexBuffer);
    glUnmapBuffer(GL_ELEMENT_ARRAY_BUFFER);


    // fire draw calls
    for ( size_t i = 0; i < std3D_frameBatch.drawCount; i++ )
    {
        GLDrawCall* dc = &std3D_frameBatch.draws[i];

        GLsizei indexCount = dc->indexCount;
        size_t j           = i + 1;

        //batch draw calls with same textures and same rdFlags together.
        while ( j < std3D_frameBatch.drawCount && dc->tex->id == std3D_frameBatch.draws[j].tex->id && dc->rdflags == std3D_frameBatch.draws[j].rdflags )
        {
            indexCount += std3D_frameBatch.draws[j].indexCount;
            i = j++;
        }

        if ( dc->type == GL_LINES || dc->type == GL_POINTS )
        {
            std3D_activeShader = std3D_defaultShaderWf;
        }
        else if ( dc->tex->pShader )
        {
            std3D_activeShader = dc->tex->pShader;
        }
        else
        {
            std3D_activeShader = std3D_defaultShader;
        }

        stdShader_SetActiveShader(std3D_activeShader);


        if ( dc->tex != std3D_pD3DTex && dc->type == GL_TRIANGLES )
        {
            stdShader_SetTexture(std3D_activeShader, dc->tex->id);
            std3D_pD3DTex = dc->tex;
        }

        std3D_SetRenderState(dc->rdflags);

        const void* indexPtr = (const void*)(dc->firstIndex * sizeof(GLushort));
        glDrawElements(dc->type, indexCount, GL_UNSIGNED_SHORT, indexPtr);
    }

    //clear
    std3D_frameBatch.vertCount  = 0;
    std3D_frameBatch.indexCount = 0;
    std3D_frameBatch.drawCount  = 0;

    glBindVertexArray(0);
}

static bool std3D_EnsureDrawCapacity(size_t extraVerts, size_t extraIndices)
{
    return std3D_frameBatch.vertCount + extraVerts < std3D_maxVerticesPerDrawCall &&
        std3D_frameBatch.indexCount + extraIndices < std3D_maxIndicesPerDrawCall &&
        std3D_frameBatch.drawCount + 1 < std3D_maxDrawCallGroupSize;
}

void J3DAPI std3D_DrawRenderList(tSysTexture* pTex, Std3DRenderState rdflags, LPD3DTLVERTEX aVerts, size_t numVerts, LPWORD aIndices, size_t numIndices)
{
    // STDLOG_DEBUG("Draw %d vertices\n", numVerts);
    if ( numVerts > (unsigned int)std3D_g_maxVertices )
    {
        STDLOG_ERROR("Error %d > %d maxVertices.\n", numVerts, std3D_g_maxVertices);
        return;
    }

    std3D_CacheDrawCall(GL_TRIANGLES, pTex, rdflags, aVerts, numVerts, aIndices, numIndices);
}

void std3D_SetWireframeRenderState(void)
{
} //not needed anymore, as this is done when wireframe draw call is cached

void J3DAPI std3D_DrawLineStrip(LPD3DTLVERTEX aVerts, size_t numVerts)
{
    if ( numVerts > std3D_g_maxVertices )
    {
        STDLOG_ERROR("Error %d > %d maxVertices.\n", numVerts,
                     std3D_g_maxVertices);
        return;
    }

    Std3DRenderState rdstate = std3D_renderState & ~(STD3D_RS_FOG_ENABLED | STD3D_RS_UNKNOWN_400 | STD3D_RS_UNKNOWN_200);

    std3D_CacheDrawCall(GL_LINES, NULL, rdstate, aVerts, numVerts, NULL, 0);
}

void J3DAPI std3D_DrawPointList(LPD3DTLVERTEX aVerts, size_t numVerts)
{
    if ( numVerts > std3D_g_maxVertices )
    {
        STDLOG_ERROR("Error %d > %d maxVertices.\n", numVerts,
                     std3D_g_maxVertices);
        return;
    }

    std3D_CacheDrawCall(GL_POINTS, NULL, 0, aVerts, numVerts, NULL, 0);
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
    if ( formatType == STDCOLOR_FORMAT_RGBA )
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
    return;
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
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glFrontFace(GL_CW);

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
    // J3D_UNUSED(fov);
    // if ( fabsf(farPlane - nearPlane) < 1e-4f )
    //     return 0;
    //
    //
    // // float f = 1.0f / tanf(fov * 0.5f * 0.01745329252);
    // //
    // // float proj[16] = {
    // //     f,    0,    0,                              0,
    // //     0,    f,    0,                              0,
    // //     0,    0,   (farPlane + nearPlane) / (nearPlane - farPlane),  -1,
    // //     0,    0,   (2.0f * farPlane * nearPlane) / (nearPlane - farPlane),  0
    // // };
    // //
    // // glMatrixMode(GL_PROJECTION);
    // // glLoadMatrixf(proj);
    // // glMatrixMode(GL_MODELVIEW);
    // return 1;
}

void J3DAPI std3D_EnableFog(int bEnabled, float density)
{
    std3D_bRenderFog   = bEnabled;
    std3D_g_fogDensity = density;
}

void J3DAPI std3D_SetFog(float red, float green, float blue, float startDepth, float endDepth)
{
    // Store fog parameters for shader use
    std3D_EnableFog(std3D_bRenderFog, std3D_g_fogDensity);
    std3D_fogStartDepth  = startDepth;
    std3D_fogEndDepth    = (2.0f - std3D_g_fogDensity) * endDepth;
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

        pD3DDriver->maxVertexCount = std3D_maxVerticesPerDrawCall;
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

bool std3D_InitVertexBuffers(GLuint* vbo, GLuint* ibo, GLuint* vao)
{
    memset(&std3D_frameBatch, 0, sizeof(std3D_frameBatch));
    std3D_frameBatch.draws = STDMALLOC(std3D_maxDrawCallGroupSize * sizeof(GLDrawCall));

    glGenVertexArrays(1, vao);
    glBindVertexArray(*vao);

    // create vbo
    glGenBuffers(1, vbo);
    glBindBuffer(GL_ARRAY_BUFFER, *vbo);
    glBufferData(GL_ARRAY_BUFFER, std3D_maxVerticesPerDrawCall * sizeof(D3DTLVERTEX), NULL, GL_DYNAMIC_DRAW);

    // create ibo
    glGenBuffers(1, ibo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, *ibo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, std3D_maxIndicesPerDrawCall * sizeof(GL_SHORT), NULL, GL_DYNAMIC_DRAW);

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

static void std3D_MapVertexBuffers(void)
{
    glBindBuffer(GL_ARRAY_BUFFER, std3D_pVertexBufferOpaque);
    std3D_frameBatch.verts = glMapBufferRange(GL_ARRAY_BUFFER, 0, std3D_maxVerticesPerDrawCall * sizeof(D3DTLVERTEX), GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, std3D_pIndexBuffer);
    std3D_frameBatch.indices = glMapBufferRange(GL_ELEMENT_ARRAY_BUFFER, 0, std3D_maxIndicesPerDrawCall * sizeof(GL_SHORT), GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT);
}

static void std3D_UnmapVertexBuffers(void)
{
    // Upload vertex data
    glBindBuffer(GL_ARRAY_BUFFER, std3D_pVertexBufferOpaque);
    glUnmapBuffer(GL_ARRAY_BUFFER);

    // Upload index data;
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, std3D_pIndexBuffer);
    glUnmapBuffer(GL_ELEMENT_ARRAY_BUFFER);
}

void std3D_ReleaseVertexBuffers(void)
{
    STDFREE(std3D_frameBatch.draws);
    if ( std3D_pVertexBufferOpaque )
    {
        glDeleteBuffers(1, &std3D_pVertexBufferOpaque);
        std3D_pVertexBufferOpaque = 0;
    }

    if ( std3D_pIndexBuffer )
    {
        glDeleteBuffers(1, &std3D_pIndexBuffer);
        std3D_pIndexBuffer = 0;
    }
}

bool std3D_InitShaderSystem(void)
{
    if ( !stdShader_Open() )
    {
        return false;
    }

    // Create default shader
    std3D_defaultShader = stdShader_CompileAndCreate("std_default", "default.vert", "default.frag");

    if ( !std3D_defaultShader )
    {
        STDLOG_ERROR("Failed to create default shader\n");
        return false;
    }

    // Create ceiling sky shader
    std3D_ceilingSkyShader = stdShader_CompileAndCreate("std_ceilingSky", "ceilingSky.vert", "ceilingSky.frag");

    if ( !std3D_ceilingSkyShader )
    {
        STDLOG_ERROR("Failed to create ceiling sky shader\n");
        return false;
    }

    // Create horizon sky shader
    std3D_horizonSkyShader = stdShader_CompileAndCreate("std_horizonSky", "horizonSky.vert", "horizonSky.frag");

    if ( !std3D_horizonSkyShader )
    {
        STDLOG_ERROR("Failed to create horizon sky shader\n");
        return false;
    }

    //create wireframe shader
    std3D_defaultShaderWf = stdShader_CompileAndCreate("std_default_wf", "default.vert", "default_wf.frag");
    if ( !std3D_defaultShaderWf )
    {
        STDLOG_ERROR("Failed to create default wf shader\n");
        return false;
    }

    stdShader_SetActiveShader(std3D_defaultShaderWf);
    const GLint wfColorLoc = glGetUniformLocation(std3D_defaultShaderWf->handle, "vWireFrameColor");
    if ( wfColorLoc == -1 )
    {
        STDLOG_ERROR("Failed to get wireframe color location in shader \n");
        return false;
    }
    glUniform4f(wfColorLoc, 1.0f, 1.0f, 1.0f, 1.0f); //set wireframe color to white.

    return true;
}

void std3D_ShutdownShaderSystem(void)
{
    std3D_defaultShader   = NULL;
    std3D_defaultShaderWf = NULL;
    stdShader_Close();
}

bool J3DAPI std3D_IsShaderSystemActive(void) { return true; }

bool std3D_IsAnisotropicFilteringSupported(void) { return true; }

bool std3D_IsMipmapAutoGenSupported(void) { return true; }

bool std3D_IsMSAASupported(void) { return true; }

void std3D_SetCeilingSkyHeight(float height)
{
    if ( !std3D_ceilingSkyShader )
    {
        STDLOG_ERROR("Ceiling sky shader was not initialized yet");
    }

    stdShader_SetActiveShader(std3D_ceilingSkyShader);
    glUniform1f(glGetUniformLocation(std3D_ceilingSkyShader->handle, "uCeilingZ"), height);
}

void std3D_UpdateHorizonSky(float camPitch, float camYaw, float scale)
{
    if ( !std3D_horizonSkyShader )
    {
        STDLOG_ERROR("Horizon sky shader was not initialized yet");
    }
    stdShader_SetActiveShader(std3D_horizonSkyShader);
    glUniform1f(glGetUniformLocation(std3D_horizonSkyShader->handle, "horizonScale"), scale);
    glUniform3f(glGetUniformLocation(std3D_horizonSkyShader->handle, "camPYR"), camPitch, camYaw, 0);
}

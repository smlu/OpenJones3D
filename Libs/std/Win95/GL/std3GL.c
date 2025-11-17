#include <std/Win95/GL/stdPixelFormatGL.h>
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
#include <std/RTI/symbols.h>

#include <math.h>
#include "std/Win95/stdWin95.h"

#define STD3D_DEFAULT_MAX_VERTICES 512

static bool bStartup    = false;
static bool std3D_bOpen = false;

static D3DRECT std3D_activeRect = { 0 };
static_assert(sizeof(std3D_activeRect) == 4 * sizeof(float),
              "sizeof(std3D_activeRect) == 4 * sizeof(float)");
// Must be 4 floats to be used in shader

static size_t std3D_frameCount            = 1;
static float std3D_zDepth                 = 0.0f;
static Std3DRenderState std3D_renderState = 0;
static tSysTexture* std3D_pD3DTex         = NULL;

static bool std3D_bAnisotropicFilter; // Added
static bool std3D_bAutoGenMipmap; // Added
static Std3DMipmapFilterType std3D_mipmapFilter = -1;

static bool std3D_bRenderFog          = true;
static bool std3D_bFogTable           = false;
static float std3D_fogDepthFactor     = 0.0f;
static float std3D_fogStartDepth      = 0.0f;
static float std3D_fogEndDepth        = 0.0f;
static StdShaderVector std3D_fogColor = { 0 };

static bool std3D_bFindAllD3Devices = false;
static size_t std3D_curDevice       = 0;
static Device3D* std3D_pCurDevice   = NULL;

static size_t std3D_numDevices    = 0;
static Device3D std3D_aDevices[4] = { 0 };

static size_t std3D_numCachedTextures       = 0;
static tSystemTexture* std3D_pFirstTexCache = NULL;
static tSystemTexture* std3D_pLastTexCache  = NULL;

static size_t std3D_RGBATextureFormat;
static size_t std3D_RGBTextureFormat;
static size_t std3D_RGBAKeyTextureFormat;

static bool std3D_bHasRGBTextureFormat           = false;
static size_t std3D_numTextureFormats            = 0;
static StdTextureFormat std3D_aTextureFormats[8] = { 0 };

static GLenum std3D_currentMipmapFiler = 0;

static const DXStatus std3D_aD3DStatusTbl[30] = {
    { D3D_OK, "D3D_OK" },
    { D3DERR_WRONGTEXTUREFORMAT, "D3DERR_WRONGTEXTUREFORMAT" },
    { D3DERR_UNSUPPORTEDCOLOROPERATION, "D3DERR_UNSUPPORTEDCOLOROPERATION" },
    { D3DERR_UNSUPPORTEDCOLORARG, "D3DERR_UNSUPPORTEDCOLORARG" },
    { D3DERR_UNSUPPORTEDALPHAOPERATION, "D3DERR_UNSUPPORTEDALPHAOPERATION" },
    { D3DERR_UNSUPPORTEDALPHAARG, "D3DERR_UNSUPPORTEDALPHAARG" },
    { D3DERR_TOOMANYOPERATIONS, "D3DERR_TOOMANYOPERATIONS" },
    { D3DERR_CONFLICTINGTEXTUREFILTER, "D3DERR_CONFLICTINGTEXTUREFILTER" },
    { D3DERR_UNSUPPORTEDFACTORVALUE, "D3DERR_UNSUPPORTEDFACTORVALUE" },
    { D3DERR_CONFLICTINGRENDERSTATE, "D3DERR_CONFLICTINGRENDERSTATE" },
    { D3DERR_UNSUPPORTEDTEXTUREFILTER, "D3DERR_UNSUPPORTEDTEXTUREFILTER" },
    { D3DERR_CONFLICTINGTEXTUREPALETTE, "D3DERR_CONFLICTINGTEXTUREPALETTE" },
    { D3DERR_DRIVERINTERNALERROR, "D3DERR_DRIVERINTERNALERROR" },
    { D3DERR_NOTFOUND, "D3DERR_NOTFOUND" },
    { D3DERR_MOREDATA, "D3DERR_MOREDATA" },
    { D3DERR_DEVICELOST, "D3DERR_DEVICELOST" },
    { D3DERR_DEVICENOTRESET, "D3DERR_DEVICENOTRESET" },
    { D3DERR_NOTAVAILABLE, "D3DERR_NOTAVAILABLE" },
    { D3DERR_OUTOFVIDEOMEMORY, "D3DERR_OUTOFVIDEOMEMORY" },
    { D3DERR_INVALIDDEVICE, "D3DERR_INVALIDDEVICE" },
    { D3DERR_INVALIDCALL, "D3DERR_INVALIDCALL" },
    { D3DERR_DRIVERINVALIDCALL, "D3DERR_DRIVERINVALIDCALL" },
    { D3DERR_WASSTILLDRAWING, "D3DERR_WASSTILLDRAWING" },
    { E_FAIL, "E_FAIL" },
    { E_INVALIDARG, "E_INVALIDARG" },
    { E_OUTOFMEMORY, "E_OUTOFMEMORY" },
    { E_NOTIMPL, "E_NOTIMPL" },
    { S_FALSE, "S_FALSE" },
    { E_NOINTERFACE, "E_NOINTERFACE" },
    { E_POINTER, "E_POINTER" }
};

const size_t std3D_maxVerticesPerDrawCall = 65536; // max vertices which can be drawn in one draw call
const size_t std3D_maxIndicesPerDrawCall  = 131072; // max indices which can be drawn in one draw call
const size_t std3D_maxDrawCallGroupSize   = 16384; // max amount of draw calls which can be summarized in a group

// structure for caching a draw call
typedef struct sGLDrawCall
{
    size_t firstIndex;
    GLsizei indexCount;
    tSysTexture* tex;
    Std3DRenderState rdflags;
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
#define STD3D_VERTBUFFERSIZE STD3D_MAXVERTICES *STD3D_MAXFACEVERTICES

static bool std3D_bUseBuffers = true; // Slow for small geometry, so disabled by
// default. Consider hybrid approach later
static GLuint std3D_pVertexArrayObject  = 0;
static const size_t std3D_vbSize        = STD3D_VERTBUFFERSIZE;
static size_t std3D_vbOffset            = 0;
static GLuint std3D_pVertexBufferOpaque = 0;

static const size_t std3D_ibSize            = STD3D_VERTBUFFERSIZE * 3;
static size_t std3D_ibOffset                = 0;
static GLuint std3D_pIndexBuffer            = 0;
static size_t std3D_numOpaqueDrawCalls      = 0;
static size_t std3D_numTransparentDrawCalls = 0;

// Shader system state
static bool std3D_bShadersActive              = true;
static GLShaderProgram* std3D_defaultShader   = NULL;
static GLShaderProgram* std3D_defaultShaderWf = NULL;

static int std3D_InitRenderState(void);
static int std3D_BuildDeviceList(void);
static void std3D_InitTextureFormats(void);

static int std3D_CreateViewport(void);
static bool J3DAPI std3D_GetZBufferFormat(GLenum* pPixelFormat);
static void J3DAPI std3D_AddTextureToCacheList(tSystemTexture* pTexture);
static void J3DAPI
std3D_RemoveTextureFromCacheList(tSystemTexture* pCacheTexture);
static int J3DAPI std3D_PurgeTextureCache(size_t size);

bool std3D_InitVertexBuffers(GLuint* vbo, GLuint* vao, GLuint* vio);
void std3D_ReleaseVertexBuffers(void);

bool std3D_InitShaderSystem(void);
void std3D_ShutdownShaderSystem(void);


static int std3D_DrawIndexedPrimitiveUP(GLenum primType, const D3DTLVERTEX* aVerts, size_t numVerts,
                                        LPWORD aIndices, size_t numIndices);

static void std3D_DrawFrameBatch(void);
static bool std3D_EnsureDrawCapacity(size_t extraVerts, size_t extraIndices);

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
    J3D_HOOKFUNC(std3D_FindClosestFormat);
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
    J3D_HOOKFUNC(std3D_D3DGetStatus);
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
    memset(std3D_aTextureFormats, 0, sizeof(std3D_aTextureFormats));
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

    memset(std3D_aTextureFormats, 0, sizeof(std3D_aTextureFormats));
    memset(std3D_aDevices, 0, sizeof(std3D_aDevices));

    std3D_numDevices = 0;
    bStartup         = false;
}

const Device3D* std3D_GetCurrentDevice(void) { return std3D_pCurDevice; }

size_t std3D_GetNumDevices(void) { return std3D_numDevices; }

const Device3D* std3D_GetAllDevices(void) { return std3D_aDevices; }

static bool std3D_InitSystem(void)
{
    GLenum pixelFormat = { 0 };
    if ( std3D_GetZBufferFormat(&pixelFormat) )
    {
        if ( stdDisplay_CreateZBuffer(NULL, std3D_pCurDevice->bHAL == 0) )
        {
            STDLOG_ERROR("Error creating Z buffer.\n");
            return false;
        }
    }
    else
    {
        STDLOG_WARNING("Warning: No stencil Z buffer format found, using default "
            "without stencil.\n");
    }

    // Get autogen support
    std3D_bAutoGenMipmap = stdConfig_GetBool(STD3D_CFG_MIPMAPAUTOGEN, true);
    if ( std3D_bAutoGenMipmap && !std3D_pCurDevice->bMipmapAutoGenSupported )
    {
        STDLOG_WARNING(
            "Warning: Automatic mipmap generation disabled, no device support!\n");
        std3D_bAutoGenMipmap = false;
    }

    std3D_bAnisotropicFilter =
        stdConfig_GetBool(STD3D_CFG_ANISOTROPICFILTER, true);
    if ( std3D_bAnisotropicFilter &&
        !std3D_pCurDevice->bAnisotropicFilteringSupported )
    {
        STDLOG_WARNING(
            "Warning: Anisotropic filtering disabled, no device support!\n");
        std3D_bAutoGenMipmap = false;
    }

    // Initialize texture formats
    std3D_InitTextureFormats();

    if ( !std3D_numTextureFormats || !std3D_bHasRGBTextureFormat )
    {
        STDLOG_ERROR("Error no texture formats found.\n");
        return false;
    }

    if ( !std3D_CreateViewport() )
    {
        STDLOG_ERROR("Error creating viewport.\n");
        return false;
    }

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

    // Get color formats for RGB, RGBA and RGBA key formats
    std3D_RGBTextureFormat     = std3D_FindClosestFormat(&stdColor_cfRGB888);
    std3D_RGBAKeyTextureFormat = std3D_FindClosestFormat(&stdColor_cfRGBA8888);
    std3D_RGBATextureFormat    = std3D_FindClosestFormat(&stdColor_cfRGBA8888);

    if ( std3D_bUseBuffers && !std3D_InitVertexBuffers(&std3D_pVertexBufferOpaque, &std3D_pIndexBuffer,
                                                       &std3D_pVertexArrayObject) )
    {
        return false;
    }

    if ( !std3D_InitShaderSystem() )
    {
        STDLOG_ERROR("Error initializing pShader system.\n");
        return false;
    }

    std3D_mipmapFilter = -1; // Must be reset
    if ( !std3D_InitRenderState() )
    {
        STDLOG_ERROR("Error initializing render state.\n");
        return false;
    }

    if ( stdDisplay_GetTextureMemory(&std3D_pCurDevice->totalMemory,
                                     &std3D_pCurDevice->availableMemory) )
    {
        // Since we failed to get texture memory info indicate that opening failed
        return false;
    }

    return true;
}

static void std3D_ReleaseSystemResources(void)
{
    std3D_ResetTextureCache();
    std3D_ShutdownShaderSystem();
    std3D_ReleaseVertexBuffers();
}

static void std3D_OnDisplayDeviceReset(tSysDevice3D* pDevice)
{
    J3D_UNUSED(pDevice);
    // Release any cached texture before device is changed
    STDLOG_DEBUG("Received display device to be reset signal. Releasing system "
        "resources...\n");
    std3D_ReleaseSystemResources();
}

static void std3D_OnDisplayDevicePostReset(tSysDevice3D* pDevice)
{
    STDLOG_DEBUG(
        "Received display device reset signal. Re-initializing the system...\n");

    std3D_InitSystem();
}

static void std3D_OnDisplayDeviceRelease(tSysDevice3D* pDevice)
{
    J3D_UNUSED(pDevice);
    STDLOG_DEBUG("Received signal that display device is about to be released. "
        "Releasing system resources...\n");
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

    std3D_curDevice  = deviceNum;
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

    STDLOG_STATUS("Texture Ram  Total: %u bytes  Free: %u bytes.\n",
                  std3D_pCurDevice->totalMemory,
                  std3D_pCurDevice->availableMemory);
    STDLOG_STATUS("Video Ram Total: %u bytes  Free: %u bytes.\n", memTotal,
                  memFree);

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
    std3D_numTextureFormats    = 0;
    std3D_curDevice            = 0;
    std3D_pCurDevice           = NULL;
    std3D_bHasRGBTextureFormat = false;
    std3D_bOpen                = false;
}

void J3DAPI std3D_GetTextureFormat(StdColorFormatType type, ColorInfo* pDest, int* pbColorKeySet,
                                   LPDDCOLORKEY* ppColorKey)
{
    if ( type == STDCOLOR_FORMAT_RGBA_1BITALPHA )
    {
        *pbColorKeySet = std3D_aTextureFormats[std3D_RGBAKeyTextureFormat].bColorKey;
        *ppColorKey    = std3D_aTextureFormats[std3D_RGBAKeyTextureFormat].pColorKey;
        *pDest         = std3D_aTextureFormats[std3D_RGBAKeyTextureFormat].ci;
    }
    else if ( type == STDCOLOR_FORMAT_RGBA )
    {
        *pbColorKeySet = std3D_aTextureFormats[std3D_RGBATextureFormat].bColorKey;
        *ppColorKey    = std3D_aTextureFormats[std3D_RGBATextureFormat].pColorKey;
        *pDest         = std3D_aTextureFormats[std3D_RGBATextureFormat].ci;
    }
    else
    {
        *pbColorKeySet = 0;
        *pDest         = std3D_aTextureFormats[std3D_RGBTextureFormat].ci;
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
    return std3D_numTextureFormats;
}

int std3D_StartScene(void)
{
    ++std3D_frameCount;

    if ( std3D_bShadersActive )
    {
        const float vp[4] = {
            (float)std3D_activeRect.x1, // x
            (float)std3D_activeRect.y1, // y
            (float)std3D_activeRect.x2, // width
            (float)std3D_activeRect.y2 // height
        };
        if ( !stdShader_SetViewport(vp) )
        {
            // error
        }
        stdShader_SetActiveShader(std3D_defaultShader);
    }

    // glEnable(GL_SCISSOR_TEST);
    // glScissor(
    //     (GLint)std3D_activeRect.x1,
    //     (GLint)std3D_activeRect.y1,
    //     (GLint)std3D_activeRect.x2,
    //     (GLint)std3D_activeRect.y2
    // );

    GLenum err = glGetError();
    if ( err != GL_NO_ERROR )
    {
        STDLOG_ERROR("OpenGL error 0x%x in std3D_StartScene.\n", err);
        return 1;
    }

    return 0;
}

void std3D_EndScene(void)
{
    if ( std3D_bUseBuffers && std3D_frameBatch.drawCount > 0 )
    {
        std3D_DrawFrameBatch();
    }
    std3D_numOpaqueDrawCalls = 0;
    std3D_pD3DTex            = NULL;
}

int std3D_CacheDrawCall(tSysTexture* pTex, Std3DRenderState rdflags, LPD3DTLVERTEX aVerts, size_t numVerts, LPWORD aIndices, size_t numIndices)
{
    if ( !std3D_EnsureDrawCapacity(numVerts, numIndices) )
    {
        std3D_DrawFrameBatch();
    }

    size_t baseVertex = std3D_frameBatch.vertCount;
    size_t firstIndex = std3D_frameBatch.indexCount;

    // paste new vertices
    memcpy(&std3D_frameBatch.verts[baseVertex], aVerts, numVerts * sizeof(D3DTLVERTEX));
    std3D_frameBatch.vertCount += numVerts;

    // paste new indices
    memcpy(&std3D_frameBatch.indices[firstIndex], aIndices, numIndices * sizeof(WORD));
    for ( size_t i = 0; i < numIndices; i++ )
    {
        std3D_frameBatch.indices[firstIndex + i] = aIndices[i] + baseVertex;
    }
    std3D_frameBatch.indexCount += numIndices;

    // cache draw call
    GLDrawCall* dc = &std3D_frameBatch.draws[std3D_frameBatch.drawCount++];
    dc->firstIndex = firstIndex;
    dc->indexCount = (GLsizei)numIndices;
    dc->tex        = pTex;
    dc->rdflags    = rdflags;

    return 1;
}

static void std3D_DrawFrameBatch(void)
{
    if ( !std3D_frameBatch.vertCount || !std3D_frameBatch.indexCount )
    {
        return;
    }

    glBindVertexArray(std3D_pVertexArrayObject);

    // Upload vertex data
    glBindBuffer(GL_ARRAY_BUFFER, std3D_pVertexBufferOpaque);
    glBufferData(GL_ARRAY_BUFFER, std3D_frameBatch.vertCount * sizeof(D3DTLVERTEX), std3D_frameBatch.verts, GL_STREAM_DRAW);

    // Upload index data;
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, std3D_pIndexBuffer);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, std3D_frameBatch.indexCount * sizeof(GLushort), std3D_frameBatch.indices, GL_STREAM_DRAW);

    // fire draw calls
    for ( size_t i = 0; i < std3D_frameBatch.drawCount; i++ )
    {
        GLDrawCall* dc = &std3D_frameBatch.draws[i];

        GLsizei indexCount = dc->indexCount;
        size_t j           = i + 1;

        //batch draw calls with same textue together. Note: this assumes they have the same rendering flags
        while ( j < std3D_frameBatch.drawCount && dc->tex->id == std3D_frameBatch.draws[j].tex->id )
        {
            indexCount += std3D_frameBatch.draws[j].indexCount;
            i = j++;
        }

        if ( dc->tex != std3D_pD3DTex )
        {
            stdShader_SetTexture(std3D_defaultShader, dc->tex->id);
            std3D_pD3DTex = dc->tex;
        }
        std3D_SetRenderState(dc->rdflags);

        const void* indexPtr = (const void*)(dc->firstIndex * sizeof(GLushort));
        glDrawElements(GL_TRIANGLES, indexCount, GL_UNSIGNED_SHORT, indexPtr);
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

    if ( std3D_bUseBuffers )
    {
        if ( !std3D_CacheDrawCall(pTex, rdflags, aVerts, numVerts, aIndices, numIndices) )
        {
            // draw failed
        }
        return;
    }


    // Set texture
    if ( pTex != std3D_pD3DTex )
    {
        stdShader_SetTexture(std3D_defaultShader, pTex->id);
        std3D_pD3DTex = pTex;
    }

    std3D_SetRenderState(rdflags);


    // Fog processing
    // if ( std3D_bFogTable )
    // {
    //     for ( size_t i = 0; i < numVerts; ++i )
    //     {
    //         D3DTLVERTEX* pCurVert = &aVerts[i];
    //         pCurVert->specular = 0xFF000000;
    //         if ( pCurVert->rhw > 0.0 )
    //         {
    //             pCurVert->specular = 0;
    //             float depth = (std3D_fogEndDepth - pCurVert->rhw *
    //             std3D_zDepth) * std3D_fogDepthFactor; if ( depth < 1.0 )
    //             {
    //                 pCurVert->specular = 0xFF000000;
    //                 if ( depth >= 0.0 )
    //                 {
    //                     pCurVert->specular = (int)((1.0 - depth) * 255.0) <<
    //                     24;
    //                 }
    //             }
    //         }
    //     }
    // }

    std3D_DrawIndexedPrimitiveUP(GL_TRIANGLES, aVerts, numVerts, aIndices, numIndices);

    // else
    // {
    //     HRESULT d3dres = IDirect3DDevice9_DrawIndexedPrimitiveUP(
    //         std3D_pD3Device,
    //         D3DPT_TRIANGLELIST,
    //         0,
    //         numVerts,
    //         numIndices / 3,
    //         aIndices,
    //         D3DFMT_INDEX16,
    //         aVerts,
    //         sizeof(D3DTLVERTEX)
    //     );
    //
    //     if ( d3dres != D3D_OK )
    //     {
    //         STDLOG_ERROR("Error %s DrawIndexedPrimitiveUP.\n",
    //         std3D_D3DGetStatus(d3dres));
    //     }
    // }
}

static int std3D_DrawIndexedPrimitiveUP(GLenum primType, const D3DTLVERTEX* aVerts, size_t numVerts,
                                        LPWORD aIndices, size_t numIndices)
{
    if ( !aVerts || !aIndices || numIndices == 0 )
    {
        STDLOG_ERROR("DrawUP: invalid vertex/index pointers or counts\n");
        return 0;
    }

    // binding empty buffers is required to make the rendering work
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    const GLsizei stride = sizeof(D3DTLVERTEX);

    glEnableVertexAttribArray(0); // position
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE,
                          stride,
                          &aVerts[0].sx);

    glEnableVertexAttribArray(1); // rhw
    glVertexAttribPointer(1, 1, GL_FLOAT, GL_FALSE,
                          stride,
                          &aVerts[0].rhw);

    glEnableVertexAttribArray(2); // diffuse color
    glVertexAttribPointer(2, 4, GL_UNSIGNED_BYTE, GL_TRUE,
                          stride,
                          &aVerts[0].color);

    glEnableVertexAttribArray(3); // specular
    glVertexAttribPointer(3, 4, GL_UNSIGNED_BYTE, GL_TRUE,
                          stride,
                          &aVerts[0].specular);

    glEnableVertexAttribArray(4); // texcoords
    glVertexAttribPointer(4, 2, GL_FLOAT, GL_FALSE,
                          stride,
                          &aVerts[0].tu);

    GLsizei indexCount = (GLsizei)numIndices;
    glDrawElements(primType, indexCount, GL_UNSIGNED_SHORT, aIndices);

    return 1;
}

void std3D_SetWireframeRenderState(void)
{
    // Std3DRenderState rdstate = std3D_renderState & ~(STD3D_RS_FOG_ENABLED |
    // STD3D_RS_UNKNOWN_400 | STD3D_RS_UNKNOWN_200);
    // std3D_SetRenderState(rdstate);
    //
    // HRESULT d3dres = IDirect3DDevice9_SetTexture(std3D_pD3Device, 0, NULL);
    // if ( d3dres != D3D_OK )
    // {
    //     STDLOG_ERROR("Error %s SetTexture.\n", std3D_D3DGetStatus(d3dres));
    //     return;
    // }
    //
    // std3D_pD3DTex = NULL;
}

void J3DAPI std3D_DrawLineStrip(LPD3DTLVERTEX aVerts, size_t numVerts)
{
    // if ( numVerts > std3D_g_maxVertices )
    // {
    //     STDLOG_ERROR("Error %d > %d maxVertices.\n", numVerts,
    //     std3D_g_maxVertices); return;
    // }
    //
    // if ( std3D_bShadersActive )
    // {
    //     std3D_UpdateShaderState(std3D_defaultShaderWf);
    // }
    //
    // if ( std3D_bUseBuffers )
    // {
    //     if ( !std3D_DrawPrimitive(D3DPT_LINESTRIP, aVerts, numVerts) )
    //     {
    //          // draw failed
    //     }
    // }
    // else
    // {
    //     HRESULT d3dres = IDirect3DDevice9_DrawPrimitiveUP(
    //         std3D_pD3Device,
    //         D3DPT_LINESTRIP,
    //         numVerts - 1,
    //         aVerts,
    //         sizeof(D3DTLVERTEX)
    //     );
    //
    //     if ( d3dres != D3D_OK )
    //     {
    //         STDLOG_ERROR("Error %s DrawPrimitiveUP.\n",
    //         std3D_D3DGetStatus(d3dres));
    //     }
    // }
}

void J3DAPI std3D_DrawPointList(LPD3DTLVERTEX aVerts, size_t numVerts)
{
    // if ( numVerts > std3D_g_maxVertices )
    // {
    //     STDLOG_ERROR("Error %d > %d maxVertices.\n", numVerts,
    //     std3D_g_maxVertices); return;
    // }
    //
    // if ( std3D_bShadersActive )
    // {
    //     std3D_UpdateShaderState(std3D_defaultShaderWf);
    // }
    //
    // if ( std3D_bUseBuffers )
    // {
    //     if ( !std3D_DrawPrimitive(D3DPT_LINESTRIP, aVerts, numVerts) )
    //     {
    //          // draw failed
    //     }
    // }
    // else
    // {
    //     HRESULT d3dres = IDirect3DDevice9_DrawPrimitiveUP(
    //         std3D_pD3Device,
    //         D3DPT_POINTLIST,
    //         numVerts,
    //         aVerts,
    //         sizeof(D3DTLVERTEX)
    //     );
    //
    //     if ( d3dres != D3D_OK )
    //     {
    //         STDLOG_ERROR("Error %s DrawPrimitiveUP.\n",
    //         std3D_D3DGetStatus(d3dres));
    //     }
    // }
}

void J3DAPI std3D_SetRenderState(Std3DRenderState rdflags)
{
    if ( std3D_renderState == rdflags )
        return;

    glEnable(GL_DEPTH_TEST);
    // --- ZWRITE ---
    if ( (std3D_renderState & STD3D_RS_ZWRITE_DISABLED) !=
        (rdflags & STD3D_RS_ZWRITE_DISABLED) )
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

    //--- Texture Address Mode U/V ---
    if ( (std3D_renderState & STD3D_RS_TEX_CPAMP_U) !=
        (rdflags & STD3D_RS_TEX_CPAMP_U) )
    {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S,
                        (rdflags & STD3D_RS_TEX_CPAMP_U)
                            ? GL_CLAMP_TO_EDGE
                            : GL_REPEAT);
    }

    if ( (std3D_renderState & STD3D_RS_TEX_CPAMP_V) !=
        (rdflags & STD3D_RS_TEX_CPAMP_V) )
    {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T,
                        (rdflags & STD3D_RS_TEX_CPAMP_V)
                            ? GL_CLAMP_TO_EDGE
                            : GL_REPEAT);
    }


    // --- Texture Filter ---
    // TODO: add anistropic filtering later
    if ( (std3D_renderState & STD3D_RS_TEXFILTER_ANISOTROPIC) !=
        (rdflags & STD3D_RS_TEXFILTER_ANISOTROPIC) )
    {
        // if ((rdflags & STD3D_RS_TEXFILTER_ANISOTROPIC) &&
        // std3D_bAnisotropicFilter) {
        //     GLfloat maxAniso = 0.0f;
        //     glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY, &maxAniso);
        //     glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY, maxAniso);
        //     glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
        //     GL_LINEAR_MIPMAP_LINEAR); glTexParameteri(GL_TEXTURE_2D,
        //     GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        // } else {
        //     // zurück zu bilinear
        //     glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY, 1.0f);
        //     glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
        //     GL_LINEAR_MIPMAP_LINEAR); glTexParameteri(GL_TEXTURE_2D,
        //     GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        // }
    }
    // else if ( (std3D_renderState & STD3D_RS_TEXFILTER_BILINEAR) !=
    //     (rdflags & STD3D_RS_TEXFILTER_BILINEAR) )
    // {
    //     if ( rdflags & STD3D_RS_TEXFILTER_BILINEAR )
    //     {
    //         glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    //         glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    //     }
    //     else
    //     {
    //         glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    //         glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    //     }
    // }

    // // --- Alpha Reference / Alpha Test ---
    // if ( (std3D_renderState & STD3D_RS_ALPHAREF_SET) !=
    //     (rdflags & STD3D_RS_ALPHAREF_SET) )
    // {
    //     if ( rdflags & STD3D_RS_ALPHAREF_SET )
    //     {
    //         glEnable(GL_ALPHA_TEST);
    //         glAlphaFunc(GL_GREATER, 160.0f / 255.0f);
    //     }
    //     else
    //     {
    //         glDisable(GL_ALPHA_TEST);
    //     }
    // }

    // Update cached flags
    std3D_renderState = rdflags;

    GLenum err = glGetError();
    if ( err != GL_NO_ERROR )
        STDLOG_ERROR("OpenGL error 0x%x in std3D_SetRenderState.\n", err);
}

void J3DAPI std3D_AllocSystemTexture(tSystemTexture* pTexture, tVBuffer** apVBuffers, size_t numMipLevels,
                                     StdColorFormatType formatType)
{
    memset(pTexture, 0, sizeof(tSystemTexture));

    if ( !std3D_numTextureFormats )
    {
        return;
    }

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

    size_t texSize =
        (pVBuffer->rasterInfo.colorInfo.bpp * texHeight * texWidth) / 8;
    // if ( std3D_mipmapFilter == STD3D_MIPMAPFILTER_NONE )
    // {
    //     numMipLevels = 1;
    // }

    tSysPixelFormat glFormat;
    if ( formatType == STDCOLOR_FORMAT_RGBA_1BITALPHA )
    {
        glFormat = std3D_aTextureFormats[std3D_RGBAKeyTextureFormat].ddPixelFmt;
    }
    else if ( formatType == STDCOLOR_FORMAT_RGBA )
    {
        glFormat = std3D_aTextureFormats[std3D_RGBATextureFormat].ddPixelFmt;
    }
    else
    {
        glFormat = std3D_aTextureFormats[std3D_RGBTextureFormat].ddPixelFmt;
    }

    // Allocate array for VBuffer pointers - NO DirectX objects created
    pTexture->apMipmaps =
        (tVBuffer**)STDMALLOC(numMipLevels * sizeof(tVBuffer*));
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
        pTexture->apMipmaps[mmNum] = stdDisplay_VBufferNew(
            &apVBuffers[mmNum]->rasterInfo, /*bUseVSurface*/ 0,
            /*bUseVideoMemory*/
            0);
        // Important: must not use video surface (D3D) as texture can live longer
        // than display device
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

                size_t pixelDataSize = apVBuffers[mmNum]->rasterInfo.height *
                    apVBuffers[mmNum]->rasterInfo.rowSize;
                memcpy(pTexture->apMipmaps[mmNum]->pPixels, apVBuffers[mmNum]->pPixels,
                       pixelDataSize);

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

void J3DAPI std3D_GetValidDimensions(uint32_t width, uint32_t height,
                                     uint32_t* pOutWidth,
                                     uint32_t* pOutHeight)
{
    uint32_t texWidth = STDMATH_CLAMP(width, std3D_pCurDevice->minTexWidth,
                                      std3D_pCurDevice->maxTexWidth);
    uint32_t texHeight = STDMATH_CLAMP(height, std3D_pCurDevice->minTexHeight,
                                       std3D_pCurDevice->maxTexHeight);

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
        // IDirect3DTexture9_Release(pTex->pCachedTexture);
    }

    memset(pTex, 0, sizeof(tSystemTexture));
}

void J3DAPI std3D_AddToTextureCache(tSystemTexture* pCacheTexture,
                                    StdColorFormatType format)
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

    bool autoMipmap         = true;
    const size_t numMipmaps = autoMipmap ? 1 : pCacheTexture->numMipLevels;

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
                    autoMipmap ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    GLenum formatGL        = pCacheTexture->format.glFormat;
    GLenum typeGL          = pCacheTexture->format.glType;
    GLint internalFormatGL = pCacheTexture->format.glInternalFormat;

    tVBuffer* mip  = pCacheTexture->apMipmaps[0];
    GLsizei width  = (GLsizei)mip->rasterInfo.width;
    GLsizei height = (GLsizei)mip->rasterInfo.height;
    glTexImage2D(GL_TEXTURE_2D, 0, internalFormatGL,
                 width, height, 0, formatGL, typeGL, mip->pPixels);

    glGenerateMipmap(GL_TEXTURE_2D);

    // for ( size_t mm = 0; mm <= numMipmaps; ++mm )
    // {
    //     tVBuffer* mip = pCacheTexture->apMipmaps[mm];
    //     if ( !mip || !mip->pPixels )
    //     {
    //         STDLOG_ERROR("Missing mipmap %zu.\n", mm);
    //         continue;
    //     }
    //     GLsizei width  = (GLsizei)mip->rasterInfo.width;
    //     GLsizei height = (GLsizei)mip->rasterInfo.height;
    //     glPixelStorei(GL_UNPACK_ROW_LENGTH, width);
    //
    //
    //     // const GLint expected = (GLint)(width * mip->rasterInfo.colorInfo.bpp);
    //     // const GLint pitch    = (GLint)mip->rasterInfo.rowSize;
    //     //
    //     // if ( pitch != expected )
    //     // {
    //     //     glPixelStorei(GL_UNPACK_ROW_LENGTH, mip->rasterInfo.rowSize);
    //     // }
    //     // else
    //     // {
    //     //     glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
    //     // }
    //     // glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_A, GL_ONE);
    //
    //     glTexImage2D(GL_TEXTURE_2D, (GLint)mm, internalFormatGL,
    //                  width, height, 0, formatGL, typeGL, mip->pPixels);
    // }

    // if ( autoMipmap )
    // {
    //     glGenerateMipmap(GL_TEXTURE_2D);
    // }

    GLenum err = glGetError();
    if ( err != GL_NO_ERROR )
    {
        STDLOG_ERROR("OpenGL error 0x%x while uploading texture.\n", err);
        glDeleteTextures(1, &tex);
        return;
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


    glBindTexture(GL_TEXTURE_2D, 0);

    tSystemTexture* pCurTex = std3D_pFirstTexCache;
    while ( pCurTex )
    {
        if ( pCurTex->pCachedTexture )
        {
            GLuint tex = (GLuint)(uintptr_t)pCurTex->pCachedTexture;
            glDeleteTextures(1, &tex);
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

    GLenum err = glGetError();
    if ( err != GL_NO_ERROR )
    {
        STDLOG_ERROR("OpenGL error 0x%x while resetting texture cache.\n", err);
    }
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

size_t J3DAPI std3D_FindClosestFormat(const ColorInfo* pMatch)
{
    if ( !std3D_numTextureFormats )
    {
        return 0;
    }

    size_t bestMatchLevel = 0;
    size_t closestMatch   = 0;
    for ( size_t i = 0; i < std3D_numTextureFormats; ++i )
    {
        StdTextureFormat* pFormat = &std3D_aTextureFormats[i];
        size_t matchLevel         = 0;

        if ( pFormat->ci.colorMode == pMatch->colorMode )
        {
            matchLevel = 1;
            if ( pFormat->ci.bpp == pMatch->bpp )
            {
                matchLevel = 2;
                if ( pMatch->colorMode == STDCOLOR_RGB )
                {
                    if ( pFormat->ci.redBPP == pMatch->redBPP &&
                        pFormat->ci.greenBPP == pMatch->greenBPP &&
                        pFormat->ci.blueBPP == pMatch->blueBPP )
                    {
                        return i;
                    }
                }
                else
                {
                    if ( pMatch->colorMode != STDCOLOR_RGBA )
                    {
                        // Index color mode
                        STDLOG_STATUS("Found a perfect mode matchLevel #%d!\n", i);
                        return i;
                    }

                    if ( pFormat->ci.colorMode == STDCOLOR_RGBA )
                    {
                        matchLevel = 3;
                    }

                    if ( pFormat->ci.redBPP == pMatch->redBPP &&
                        pFormat->ci.greenBPP == pMatch->greenBPP &&
                        pFormat->ci.blueBPP == pMatch->blueBPP &&
                        pFormat->ci.alphaBPP == pMatch->alphaBPP )
                    {
                        return i;
                    }
                }
            }
        }

        if ( matchLevel > bestMatchLevel )
        {
            closestMatch   = i;
            bestMatchLevel = matchLevel;
        }
    }

    STDLOG_STATUS("Settling for a closest matchLevel #%d..\n", closestMatch);
    return closestMatch;
}

int std3D_InitRenderState(void)
{
    std3D_renderState = 0;

    glEnable(GL_DEPTH_TEST);
    //glDepthRange(0.0, 1.0);
    glDepthMask(GL_TRUE);
    glDepthFunc(GL_LEQUAL);
    std3D_renderState |= STD3D_RS_UNKNOWN_1;

    std3D_SetMipmapFilter(STD3D_MIPMAPFILTER_TRILINEAR);

    glBindTexture(GL_TEXTURE_2D, 0); // Default-Basis

    // if (std3D_bAnisotropicFilter)
    // {
    //     GLfloat maxAniso = 0.0f;
    //     glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY, &maxAniso);
    //     glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY, maxAniso);
    //     std3D_renderState |= STD3D_RS_TEXFILTER_ANISOTROPIC;
    // }
    // else
    // {
    //     glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
    //     GL_LINEAR_MIPMAP_LINEAR); glTexParameteri(GL_TEXTURE_2D,
    //     GL_TEXTURE_MAG_FILTER, GL_LINEAR); std3D_renderState |=
    //     STD3D_RS_TEXFILTER_BILINEAR;
    // }

    // glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
    // GL_LINEAR_MIPMAP_LINEAR); glTexParameteri(GL_TEXTURE_2D,
    // GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    std3D_renderState |= STD3D_RS_TEXFILTER_BILINEAR;

    // // --- 3️⃣ Texture Wrapping ---
    // glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    // glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    // --- 4️⃣ Alpha Blending ---
    glEnable(GL_BLEND);
    glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ZERO);
    //glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    // Alpha-Test (falls Fixed-Function-Kompatibilität)
    // glEnable(GL_ALPHA_TEST);
    // glAlphaFunc(GL_GREATER, 0.0f);

    // glShadeModel(GL_SMOOTH); // D3DSHADE_GOURAUD
    // glDisable(GL_LIGHTING);
    // glDisable(GL_COLOR_MATERIAL);
    // glDisable(GL_SEPARATE_SPECULAR_COLOR);

    std3D_bRenderFog = false;
    glDisable(GL_FOG);

    //glPolygonMode(GL_BACK, GL_FILL);

    //glEnable(GL_DITHER);

    std3D_renderState |= STD3D_RS_UNKNOWN_2;

    // --- 9️⃣ Culling ---
    //glDisable(GL_CULL_FACE); // D3DCULL_NONE
    glFrontFace(GL_CW);

    GLenum err = glGetError();
    if ( err != GL_NO_ERROR )
    {
        STDLOG_ERROR("OpenGL error 0x%x during render state init.\n", err);
        return false;
    }

    return true;
}

int J3DAPI std3D_SetMipmapFilter(Std3DMipmapFilterType filter)
{
    if ( filter == std3D_mipmapFilter )
        return 0;

    GLenum minFilter = GL_LINEAR; // default: keine Mipmaps
    switch ( filter )
    {
        case STD3D_MIPMAPFILTER_BILINEAR:
            minFilter = GL_LINEAR_MIPMAP_NEAREST;
            break;
        case STD3D_MIPMAPFILTER_TRILINEAR:
            minFilter = GL_LINEAR_MIPMAP_LINEAR;
            break;
        case STD3D_MIPMAPFILTER_NONE:
            minFilter = GL_LINEAR;
            break;
        default:
            minFilter = GL_LINEAR;
            break;
    }

    std3D_currentMipmapFiler = minFilter;

    std3D_mipmapFilter = filter;
    return 0;
}

int J3DAPI std3D_SetProjection(float fov, float nearPlane, float farPlane)
{
    if ( fabsf(farPlane - nearPlane) < 1e-4f )
        return 0;

    // is this actually needed? Since we don't need extra projection matrix in
    // shader. float f = 1.0f / tanf(fov * 0.5f * 0.01745329252);
    //
    // float proj[16] = {
    //     f,    0,    0,                              0,
    //     0,    f,    0,                              0,
    //     0,    0,   (farPlane + nearPlane) / (nearPlane - farPlane),  -1,
    //     0,    0,   (2.0f * farPlane * nearPlane) / (nearPlane - farPlane),  0
    // };
    //
    // glMatrixMode(GL_PROJECTION);
    // glLoadMatrixf(proj);
    // glMatrixMode(GL_MODELVIEW);
    return 1;
}

void J3DAPI std3D_EnableFog(int bEnabled, float density)
{
    // std3D_bRenderFog = bEnabled;
    // if ( !std3D_pCurDevice || !std3D_pD3Device )
    // {
    //     std3D_bFogTable  = false;
    //     return;
    // }
    //
    // std3D_g_fogDensity = density;
    //
    // if ( std3D_bShadersActive )
    // {
    //     return;
    // }
    //
    //  // Get device capabilities
    // D3DCAPS9 caps;
    // if ( FAILED(IDirect3DDevice9_GetDeviceCaps(std3D_pD3Device, &caps)) )
    // {
    //     std3D_bRenderFog = false;
    //     return;
    // }
    //
    // // Check fog support
    // bool bTableFogSupported = (caps.RasterCaps & D3DPRASTERCAPS_FOGTABLE) != 0;
    // bool bVertexFogSupported = (caps.RasterCaps & D3DPRASTERCAPS_FOGVERTEX) !=
    // 0;
    //
    // if ( !bTableFogSupported && !bVertexFogSupported && !std3D_bShadersActive )
    // {
    //     std3D_bRenderFog = false;
    // }
    //
    // std3D_bFogTable = bTableFogSupported && std3D_bRenderFog &&
    // !std3D_bShadersActive;
}

void J3DAPI std3D_SetFog(float red, float green, float blue, float startDepth,
                         float endDepth)
{
    // // Update table stare
    // std3D_EnableFog(std3D_bRenderFog, std3D_g_fogDensity);
    //
    // if ( std3D_bShadersActive )
    // {
    //     // Store fog parameters for shader use
    //     std3D_fogStartDepth  = startDepth;
    //     std3D_fogEndDepth    = (2.0f - std3D_g_fogDensity) * endDepth;
    //     std3D_fogDepthFactor = 1.0f / (std3D_fogEndDepth -
    //     std3D_fogStartDepth);
    //
    //     std3D_fogColor[0] = red;
    //     std3D_fogColor[1] = green;
    //     std3D_fogColor[2] = blue;
    //     std3D_fogColor[3] = 1.0f;
    // }
    // else
    // {
    //     if ( IDirect3DDevice9_SetRenderState(std3D_pD3Device, D3DRS_FOGCOLOR,
    //     D3DRGB(red, green, blue)) == D3D_OK )
    //     {
    //         if ( std3D_g_fogDensity == 0.0f )
    //         {
    //             IDirect3DDevice9_SetRenderState(std3D_pD3Device,
    //             D3DRS_FOGTABLEMODE, D3DFOG_NONE);
    //             IDirect3DDevice9_SetRenderState(std3D_pD3Device,
    //             D3DRS_FOGVERTEXMODE, D3DFOG_NONE);
    //         }
    //         else
    //         {
    //             endDepth = (2.0f - std3D_g_fogDensity) * endDepth;
    //
    //             bool bSuccess = false;
    //             if ( std3D_bFogTable )
    //             {
    //                 // Use table fog for pre-transformed vertices
    //                 if (
    //                 SUCCEEDED(IDirect3DDevice9_SetRenderState(std3D_pD3Device,
    //                 D3DRS_FOGTABLEMODE, D3DFOG_LINEAR)) )
    //                 {
    //                     bSuccess =
    //                     IDirect3DDevice9_SetRenderState(std3D_pD3Device,
    //                     D3DRS_FOGSTART, *(DWORD*)(&startDepth)) == D3D_OK
    //                         && IDirect3DDevice9_SetRenderState(std3D_pD3Device,
    //                         D3DRS_FOGEND, *(DWORD*)(&endDepth)) == D3D_OK;
    //                 }
    //             }
    //             else
    //             {
    //                 // Use vertex fog (for non-pre-transformed vertices)
    //                 if (
    //                 SUCCEEDED(IDirect3DDevice9_SetRenderState(std3D_pD3Device,
    //                 D3DRS_FOGVERTEXMODE, D3DFOG_LINEAR)) )
    //                 {
    //                     bSuccess =
    //                     IDirect3DDevice9_SetRenderState(std3D_pD3Device,
    //                     D3DRS_FOGSTART, *(DWORD*)(&startDepth)) == D3D_OK
    //                         && IDirect3DDevice9_SetRenderState(std3D_pD3Device,
    //                         D3DRS_FOGEND, *(DWORD*)(&endDepth)) == D3D_OK;
    //                 }
    //             }
    //
    //             if ( bSuccess )
    //             {
    //                 std3D_fogStartDepth  = startDepth;
    //                 std3D_fogEndDepth    = endDepth;
    //                 std3D_fogDepthFactor = 1.0f / (endDepth - startDepth);
    //             }
    //         }
    //     }
    // }
}

void std3D_ClearZBuffer(void)
{
    // Z- und Stencil-Werte, wie in Direct3D
    //glClearDepth(1.0f);
    glClearStencil(0);

    std3D_renderState &= ~STD3D_RS_ZWRITE_DISABLED;
    glDepthMask(GL_TRUE);

    // Jetzt nur Depth- und Stencil-Buffer löschen
    glClear(GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);

    GLenum err = glGetError();
    if ( err != GL_NO_ERROR )
    {
        STDLOG_ERROR("OpenGL error 0x%x when clearing Z.\n", err);
    }
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

    for ( size_t i = 0;
          i < numDisplayDevices && std3D_numDevices < STD_ARRAYLEN(std3D_aDevices);
          ++i )
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
        pD3DDriver->d3dDesc                      = displayDevice.caps;
        pD3DDriver->bTexturePerspectiveSupported = TRUE; // Always supported in DX9
        pD3DDriver->hasZBuffer                   = TRUE; // Always supported in DX9
        pD3DDriver->bSqareOnlyTexture            =
            (displayDevice.caps.TextureCaps & D3DPTEXTURECAPS_SQUAREONLY) != 0;
        pD3DDriver->bAlphaTextureSupported =
            (displayDevice.caps.TextureCaps & D3DPTEXTURECAPS_ALPHA) != 0;
        pD3DDriver->bColorkeyTextureSupported      = TRUE; // Always supported in DX9
        pD3DDriver->bStippledShadeSupported        = FALSE; // Not commonly used in DX9
        pD3DDriver->minTexWidth                    = 1;
        pD3DDriver->minTexHeight                   = 1;
        pD3DDriver->maxTexWidth                    = 4096;
        pD3DDriver->maxTexHeight                   = 4096;
        pD3DDriver->bAnisotropicFilteringSupported =
            (displayDevice.caps.RasterCaps & D3DPRASTERCAPS_ANISOTROPY) != 0;
        pD3DDriver->bMipmapAutoGenSupported =
            (displayDevice.caps.Caps2 & D3DCAPS2_CANAUTOGENMIPMAP) != 0;

        pD3DDriver->bAlphaBlendSupported =
            (displayDevice.caps.SrcBlendCaps & D3DPBLENDCAPS_SRCALPHA) != 0 &&
            (displayDevice.caps.DestBlendCaps & D3DPBLENDCAPS_INVSRCALPHA) != 0;

        pD3DDriver->maxVertexCount = displayDevice.caps.MaxVertexIndex;
        if ( pD3DDriver->maxVertexCount == 0 )
        {
            pD3DDriver->maxVertexCount = 65535; // Reasonable default
        }

        pD3DDriver->totalMemory     = displayDevice.totalVideoMemory;
        pD3DDriver->availableMemory = displayDevice.freeVideoMemory;

        // TODO: proly no point to make log here since same info can be logged in
        // stdDisplay
        STDLOG_STATUS("Found |%s|%s|%s|%s| D3D Device\n",
                      pD3DDriver->hasZBuffer ? "Z" : "Non-Z",
                      pD3DDriver->bAlphaTextureSupported ? "Alpha" : "No Alpha",
                      pD3DDriver->bStippledShadeSupported ? "Stippled" : "Blend",
                      pD3DDriver->bColorkeyTextureSupported ? "Colorkey"
                      : "No Colorkey");

        STDLOG_STATUS("Description: %s [%s]\n", pD3DDriver->deviceName,
                      pD3DDriver->deviceDescription);

        ++std3D_numDevices;
    }

    return std3D_numDevices > 0;
}

static void std3D_InitTextureFormats(void)
{
    std3D_numTextureFormats    = 0;
    std3D_bHasRGBTextureFormat = true;

    // available color formats
    const ColorInfo formats[] = {
        stdColor_cfRGB888, // 24-bit RGB
        stdColor_cfRGB8888, // 32-bit RGB
        stdColor_cfARGB8888, // 32-bit ARGB
        stdColor_cfRGB565, // 16-bit RGB
        stdColor_cfARGB5551, // 16-bit ARGB
        stdColor_cfARGB4444, // 16-bit ARGB
    };

    // corresponding gl formats
    const tSysPixelFormat glPixelFormats[] = {
        stdPixelFormatGL_RGB888, stdPixelFormatGL_RGB8888,
        stdPixelFormatGL_ARGB8888, stdPixelFormatGL_RGB565,
        stdPixelFormalGL_ARGB5551, stdPixelFormalGL_ARGB4444
    };

    // assume all texture formats are compatible in OpenGL
    for ( size_t i = 0;
          i < STD_ARRAYLEN(formats) &&
          std3D_numTextureFormats < STD_ARRAYLEN(std3D_aTextureFormats);
          ++i )
    {
        StdTextureFormat* pTexFormat =
            &std3D_aTextureFormats[std3D_numTextureFormats];
        memset(pTexFormat, 0, sizeof(StdTextureFormat));

        pTexFormat->ddPixelFmt = glPixelFormats[i];
        pTexFormat->ci         = formats[i];

        ++std3D_numTextureFormats;
    }
}

int std3D_CreateViewport(void)
{
    GLenum err = glGetError();
    if ( err != GL_NO_ERROR )
    {
        STDLOG_ERROR("OpenGL error 0x%x when creating viewport.\n", err);
        return 0;
    }

    // Viewport-Größe aus Backbuffer übernehmen
    GLint width  = (GLint)stdDisplay_g_backBuffer.rasterInfo.width;
    GLint height = (GLint)stdDisplay_g_backBuffer.rasterInfo.height;

    // Viewport setzen
    glViewport(0, 0, width, height);

    // Aktive Region merken (wie in Original)
    std3D_activeRect.x1 = 0;
    std3D_activeRect.y1 = 0;
    std3D_activeRect.x2 = width;
    std3D_activeRect.y2 = height;

    // Farbe, Depth und Stencil löschen
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f); // Schwarz
    glClearDepth(1.0f); // Depth auf 1.0 (wie D3D)
    glClearStencil(0); // Stencil auf 0
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

    return 1;
}

static bool J3DAPI std3D_GetZBufferFormat(GLenum* pPixelFormat)
{
    if ( !pPixelFormat )
    {
        return false;
    }

    // Check supported Z-buffer formats
    GLenum aFormats[] = {
        GL_DEPTH24_STENCIL8, // 24-bit depth, 8-bit stencil
    };

    *pPixelFormat = aFormats[0];

    return true;
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
        pCacheTexture->pPrevCachedTexture->pNextCachedTexture =
            pCacheTexture->pNextCachedTexture;
        pCacheTexture->pNextCachedTexture->pPrevCachedTexture =
            pCacheTexture->pPrevCachedTexture;
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
            // IDirect3DTexture9_Release(pCacheTexture->pCachedTexture);
            pCacheTexture->pCachedTexture = NULL;
            std3D_RemoveTextureFromCacheList(pCacheTexture);
            return 1;
        }
    }

    tSystemTexture* pNextCachedTexture = NULL;
    for ( tSystemTexture* pCacheTexture = std3D_pFirstTexCache;
          pCacheTexture && purgedBytes < size;
          pCacheTexture = pNextCachedTexture )
    {
        pNextCachedTexture = pCacheTexture->pNextCachedTexture;
        if ( pCacheTexture->frameNum != std3D_frameCount )
        {
            if ( pCacheTexture->pCachedTexture )
            {
                // IDirect3DTexture9_Release(pCacheTexture->pCachedTexture);
            }
            pCacheTexture->pCachedTexture = NULL;
            purgedBytes += pCacheTexture->textureSize;
            std3D_RemoveTextureFromCacheList(pCacheTexture);
        }
    }

    return purgedBytes != 0;
}

const char* J3DAPI std3D_D3DGetStatus(HRESULT res)
{
    for ( size_t i = 0; i < STD_ARRAYLEN(std3D_aD3DStatusTbl); ++i )
    {
        if ( std3D_aD3DStatusTbl[i].code == res )
        {
            return std3D_aD3DStatusTbl[i].text;
        }
    }

    return "Unknown Error";
}

StdDisplayEnvironment* J3DAPI std3D_BuildDisplayEnvironment()
{
    StdDisplayEnvironment* pDeviceList =
        (StdDisplayEnvironment*)STDMALLOC(sizeof(StdDisplayEnvironment));
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
        pDeviceList->aDisplayInfos = (StdDisplayInfo*)STDMALLOC(
            sizeof(StdDisplayInfo) * pDeviceList->numInfos);

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
    std3D_frameBatch.verts   = STDMALLOC(std3D_maxVerticesPerDrawCall * sizeof(D3DTLVERTEX));
    std3D_frameBatch.indices = STDMALLOC(std3D_maxIndicesPerDrawCall * sizeof(GL_SHORT));
    std3D_frameBatch.draws   = STDMALLOC(std3D_maxDrawCallGroupSize * sizeof(GLDrawCall));

    glGenVertexArrays(1, vao);
    glBindVertexArray(*vao);

    glGenBuffers(1, vbo);
    glBindBuffer(GL_ARRAY_BUFFER, *vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(D3DTLVERTEX), NULL,
                 GL_STREAM_DRAW);

    glGenBuffers(1, ibo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, *ibo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLushort), NULL,
                 GL_STREAM_DRAW);

    const GLsizei stride = sizeof(D3DTLVERTEX);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride,
                          (void*)offsetof(D3DTLVERTEX, sx));
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 1, GL_FLOAT, GL_FALSE, stride,
                          (void*)offsetof(D3DTLVERTEX, rhw));
    glEnableVertexAttribArray(1);

    glVertexAttribPointer(2, 4, GL_UNSIGNED_BYTE, GL_TRUE, stride,
                          (void*)offsetof(D3DTLVERTEX, color));
    glEnableVertexAttribArray(2);

    glVertexAttribPointer(3, 4, GL_UNSIGNED_BYTE, GL_TRUE, stride,
                          (void*)offsetof(D3DTLVERTEX, specular));
    glEnableVertexAttribArray(3);

    glVertexAttribPointer(4, 2, GL_FLOAT, GL_FALSE, stride,
                          (void*)offsetof(D3DTLVERTEX, tu));
    glEnableVertexAttribArray(4);

    glBindVertexArray(0);
    GLenum err = glGetError();
    if ( err != GL_NO_ERROR )
    {
        STDLOG_ERROR("Error 0x%x during VBO/attribute init\n", err);
        return false;
    }

    return true;
}

void std3D_ReleaseVertexBuffers(void)
{
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
    std3D_bShadersActive = false;

    if ( !stdShader_Open() )
    {
        return false;
    }

    // Create default shader
    std3D_defaultShader = stdShader_CompileAndCreate("std_default", "default.vert", "default.frag");
    // std3D_defaultShader =
    //     stdShader_CompileAndCreate("std_default", "default.vert", "default.frag");
    if ( !std3D_defaultShader )
    {
        STDLOG_ERROR("Failed to create default shader\n");
        return false;
    }

    // std3D_defaultShaderWf = stdShader_CompileAndCreate("std_defaultWf",
    // "C:\\Users\\morit\\Documents\\GitHub\\OpenJones3D\\Libs\\std\\Win95\\GL\\Shaders\\default.vert",
    //     "C:\\Users\\morit\\Documents\\GitHub\\OpenJones3D\\Libs\\std\\Win95\\GL\\Shaders\\default.frag");
    // if ( !std3D_defaultShader )
    // {
    //     STDLOG_ERROR("Failed to create default wf shader\n");
    //     return false;
    // }

    // if ( !stdShader_RegisterShaderParam(std3D_defaultShaderWf,
    // "g_wireframeColor", STDSHADER_TYPE_PIXEL, STDSHADER_PARAM_VECTOR4,
    // /*registerIndex=*/0) )
    // {
    //     return false;
    // }
    //
    // StdShaderParamValue val;
    // val.type = STDSHADER_PARAM_VECTOR4;
    // val.value.vector[0] = 1.0f; // Red
    // val.value.vector[1] = 1.0f; // Green
    // val.value.vector[2] = 1.0f; // Blue
    // val.value.vector[3] = 1.0f; // Alpha
    // if ( !stdShader_SetShaderParam(std3D_defaultShaderWf, "g_wireframeColor",
    // STDSHADER_TYPE_PIXEL, &val) )
    // {
    //     return false;
    // }

    std3D_bShadersActive = true;
    return true;
}

void std3D_ShutdownShaderSystem(void)
{
    std3D_defaultShader   = NULL;
    std3D_defaultShaderWf = NULL;
    // Important, reset active shader to STDSHADER_INVALIDHANDLE to avoid dangling
    // handle on next system init
    stdShader_Close();
}

bool J3DAPI std3D_IsShaderSystemActive(void) { return std3D_bShadersActive; }

bool std3D_IsAnisotropicFilteringSupported(void) { return true; }

bool std3D_IsMipmapAutoGenSupported(void) { return true; }

bool std3D_IsMSAASupported(void) { return true; }

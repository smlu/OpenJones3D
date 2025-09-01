#include <std/Win95/std3D.h>
#include <std/Win95/stdDisplay.h>
#include <std/Win95/stdShader.h>

#include <j3dcore/j3dhook.h>
#include <std/General/std.h>
#include <std/General/stdColor.h>
#include <std/General/stdMath.h>
#include <std/General/stdMemory.h>
#include <std/General/stdUtil.h>
#include <std/RTI/symbols.h>

#include <math.h>

#include "Shaders/std_default_vs.h"
#include "Shaders/std_default_ps.h"
#include "Shaders/std_default_wf_ps.h"
#include "Shaders/std_composite_ps.h"
#include "Shaders/std_wave_vs.h"
#include "Shaders/std_wave_ps.h"

#define STD3D_DEFAULT_MAX_VERTICES 512

static bool bStartup    = false;
static bool std3D_bOpen = false;

static LPDIRECT3D9 std3D_pDirect3D       = NULL;
static LPDIRECT3DDEVICE9 std3D_pD3Device = NULL;
static D3DRECT std3D_activeRect          = { 0 };
static_assert(sizeof(std3D_activeRect) == 4 * sizeof(float), "sizeof(std3D_activeRect) == 4 * sizeof(float)"); // Must be 4 floats to be used in shader

static size_t std3D_frameCount                  = 1;
static float std3D_zDepth                       = 0.0f;
static Std3DRenderState std3D_renderState       = 0;
static LPDIRECT3DTEXTURE9 std3D_pD3DTex         = NULL;
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

static size_t std3D_numDevices      = 0;
static Device3D std3D_aDevices[4]   = { 0 };

static size_t std3D_numCachedTextures       = 0;
static tSystemTexture* std3D_pFirstTexCache = NULL;
static tSystemTexture* std3D_pLastTexCache  = NULL;

static size_t std3D_RGBATextureFormat;
static size_t std3D_RGBTextureFormat;
static size_t std3D_RGBAKeyTextureFormat;

static bool std3D_bHasRGBTextureFormat           = false;
static size_t std3D_numTextureFormats            = 0;
static StdTextureFormat std3D_aTextureFormats[8] = { 0 };

static const DXStatus std3D_aD3DStatusTbl[30] =
{
    { D3D_OK,                                "D3D_OK" },
    { D3DERR_WRONGTEXTUREFORMAT,             "D3DERR_WRONGTEXTUREFORMAT" },
    { D3DERR_UNSUPPORTEDCOLOROPERATION,      "D3DERR_UNSUPPORTEDCOLOROPERATION" },
    { D3DERR_UNSUPPORTEDCOLORARG,            "D3DERR_UNSUPPORTEDCOLORARG" },
    { D3DERR_UNSUPPORTEDALPHAOPERATION,      "D3DERR_UNSUPPORTEDALPHAOPERATION" },
    { D3DERR_UNSUPPORTEDALPHAARG,            "D3DERR_UNSUPPORTEDALPHAARG" },
    { D3DERR_TOOMANYOPERATIONS,              "D3DERR_TOOMANYOPERATIONS" },
    { D3DERR_CONFLICTINGTEXTUREFILTER,       "D3DERR_CONFLICTINGTEXTUREFILTER" },
    { D3DERR_UNSUPPORTEDFACTORVALUE,         "D3DERR_UNSUPPORTEDFACTORVALUE" },
    { D3DERR_CONFLICTINGRENDERSTATE,         "D3DERR_CONFLICTINGRENDERSTATE" },
    { D3DERR_UNSUPPORTEDTEXTUREFILTER,       "D3DERR_UNSUPPORTEDTEXTUREFILTER" },
    { D3DERR_CONFLICTINGTEXTUREPALETTE,      "D3DERR_CONFLICTINGTEXTUREPALETTE" },
    { D3DERR_DRIVERINTERNALERROR,            "D3DERR_DRIVERINTERNALERROR" },
    { D3DERR_NOTFOUND,                       "D3DERR_NOTFOUND" },
    { D3DERR_MOREDATA,                       "D3DERR_MOREDATA" },
    { D3DERR_DEVICELOST,                     "D3DERR_DEVICELOST" },
    { D3DERR_DEVICENOTRESET,                 "D3DERR_DEVICENOTRESET" },
    { D3DERR_NOTAVAILABLE,                   "D3DERR_NOTAVAILABLE" },
    { D3DERR_OUTOFVIDEOMEMORY,               "D3DERR_OUTOFVIDEOMEMORY" },
    { D3DERR_INVALIDDEVICE,                  "D3DERR_INVALIDDEVICE" },
    { D3DERR_INVALIDCALL,                    "D3DERR_INVALIDCALL" },
    { D3DERR_DRIVERINVALIDCALL,              "D3DERR_DRIVERINVALIDCALL" },
    { D3DERR_WASSTILLDRAWING,                "D3DERR_WASSTILLDRAWING" },
    { E_FAIL,                                "E_FAIL" },
    { E_INVALIDARG,                          "E_INVALIDARG" },
    { E_OUTOFMEMORY,                         "E_OUTOFMEMORY" },
    { E_NOTIMPL,                             "E_NOTIMPL" },
    { S_FALSE,                               "S_FALSE" },
    { E_NOINTERFACE,                         "E_NOINTERFACE" },
    { E_POINTER,                             "E_POINTER" }
};

// Global state

// VBO & IBO
#define STD3D_VERTBUFFERSIZE STD3D_MAXVERTICES * STD3D_MAXFACEVERTICES

static bool std3D_bUseBuffers    = false; // Slow for small geometry, so disabled by default. Consider hybrid approach later
static const size_t std3D_vbSize = STD3D_VERTBUFFERSIZE;
static size_t std3D_vbOffset     = 0;
static IDirect3DVertexBuffer9* std3D_pVertexBuffer = NULL;

static const size_t std3D_ibSize = STD3D_VERTBUFFERSIZE * 3;
static size_t std3D_ibOffset     = 0;
static IDirect3DIndexBuffer9* std3D_pIndexBuffer = NULL;

// Shader system state
static bool std3D_bShadersActive = true;
static StdShaderHandle std3D_defaultShader = STDSHADER_INVALIDHANDLE;
static StdShaderHandle std3D_defaultShaderWf = STDSHADER_INVALIDHANDLE;
static StdShaderHandle std3D_activeShader = STDSHADER_INVALIDHANDLE;


// Multi-pass rendering state
static bool std3D_bMultiPassActive = false;
static LPDIRECT3DTEXTURE9 std3D_pRenderTargets[STDSHADER_RT_COUNT] = { NULL };
static LPDIRECT3DSURFACE9 std3D_pRTSurfaces[STDSHADER_RT_COUNT] = { NULL };
static LPDIRECT3DSURFACE9 std3D_pOriginalRenderTarget = NULL;
static LPDIRECT3DSURFACE9 std3D_pOriginalDepthStencil = NULL;

// Current rendering pass
static int std3D_currentPass = 0;
static StdShaderHandle std3D_compositeShader = STDSHADER_INVALIDHANDLE;
static StdShaderHandle std3D_lightingShader = STDSHADER_INVALIDHANDLE;
static StdShaderHandle std3D_waveShader = STDSHADER_INVALIDHANDLE;

// Global shader parameters
//static StdShaderGlobalParams std3D_globalParams = { 0 };

// Custom shader for current draw call
static StdShaderHandle std3D_customShader = STDSHADER_INVALIDHANDLE;


static int std3D_InitRenderState(void);
static int std3D_BuildDeviceList(void);
static void std3D_InitTextureFormats(void);

static int std3D_CreateViewport(void);
static bool J3DAPI std3D_GetZBufferFormat(tSysPixelFormat* pPixelFormat);
static void J3DAPI std3D_AddTextureToCacheList(tSystemTexture* pTexture);
static void J3DAPI std3D_RemoveTextureFromCacheList(tSystemTexture* pCacheTexture);
static int J3DAPI std3D_PurgeTextureCache(size_t size);

int std3D_InitVertexBuffers(void);
void std3D_ReleaseVertexBuffers(void);

int std3D_InitShaderSystem(void);
void std3D_ShutdownShaderSystem(void);

bool std3D_InitMultiPassRenderTargets(void);
void std3D_ReleaseMultiPassRenderTargets(void);

static void std3D_UpdateShaderState(StdShaderHandle activeShader);

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
    memcpy(&std3D_g_fogDensity, &std3D_g_fogDensity_tmp, sizeof(std3D_g_fogDensity));
    memset(&std3D_g_maxVertices, 0, sizeof(std3D_g_maxVertices));
}

int std3D_Startup(void)
{
    STD_ASSERTREL(bStartup == 0);
    memset(std3D_aTextureFormats, 0, sizeof(std3D_aTextureFormats));
    memset(std3D_aDevices, 0, sizeof(std3D_aDevices));

    std3D_pDirect3D = stdDisplay_GetDirect3D();
    if ( !std3D_pDirect3D )
    {
        STDLOG_ERROR("Direct3D9 not created yet!\n");
        return 0;
    }

    if ( !stdShader_Startup() )
    {
        STDLOG_ERROR("Error initializing shader system.\n");
        return 0;
    }

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
    if ( std3D_bOpen )
    {
        std3D_Close();
    }

    stdShader_Shutdown();

    memset(std3D_aTextureFormats, 0, sizeof(std3D_aTextureFormats));
    memset(std3D_aDevices, 0, sizeof(std3D_aDevices));

    std3D_pD3Device     = NULL;
    std3D_pDirect3D     = NULL;
    std3D_numDevices    = 0;
    bStartup            = false;
}

const Device3D* std3D_GetCurrentDevice(void)
{
    return std3D_pCurDevice;
}

size_t std3D_GetNumDevices(void)
{
    return std3D_numDevices;
}

const Device3D* std3D_GetAllDevices(void)
{
    return std3D_aDevices;
}

static bool std3D_InitSystem(void)
{
    tSysPixelFormat pixelFormat = { 0 };
    if ( std3D_GetZBufferFormat(&pixelFormat) )
    {
        if ( stdDisplay_CreateZBuffer(&pixelFormat, std3D_pCurDevice->bHAL == 0) )
        {
            STDLOG_ERROR("Error creating Z buffer.\n");
            return false;
        }
    }
    else
    {
        STDLOG_WARNING("Warning: No stencil Z buffer format found, using default without stencil.\n");
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
    std3D_RGBTextureFormat     = std3D_FindClosestFormat(&stdColor_cfBGR8888);
    std3D_RGBAKeyTextureFormat = std3D_FindClosestFormat(&stdColor_cfRGBA8888);
    std3D_RGBATextureFormat    = std3D_FindClosestFormat(&stdColor_cfRGBA8888);

    if ( !std3D_InitVertexBuffers() )
    {
        return false;
    }

    if ( !std3D_InitShaderSystem() )
    {
        STDLOG_ERROR("Error initializing pShader system.\n");
        return false;
    }

    if ( !std3D_InitRenderState() )
    {
        STDLOG_ERROR("Error initializing render state.\n");
        return false;
    }

    if ( stdDisplay_GetTextureMemory(&std3D_pCurDevice->totalMemory, &std3D_pCurDevice->availableMemory) )
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
};

static void std3D_OnDisplayDeviceReset(tSysDevice3D* pDevice)
{
    J3D_UNUSED(pDevice);
    // Release any cached texture before device is changed
    STDLOG_DEBUG("Received display device to be reset signal. Releasing system resources...\n");
    std3D_ReleaseSystemResources();
}

static void std3D_OnDisplayDevicePostReset(tSysDevice3D* pDevice)
{
    STDLOG_DEBUG("Received display device reset signal. Re-initializing the system...\n");
    std3D_pD3Device = pDevice;
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
    STD_ASSERTREL(bStartup == 1);
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

    if ( deviceNum >= std3D_numDevices ) {
        return 0;
    }

    std3D_curDevice  = deviceNum;
    std3D_pCurDevice = &std3D_aDevices[deviceNum];

    // Get D3D device from stdDisplay (should be already created)
    // Note, should be called before std3D_InitSystem()
    std3D_pD3Device = stdDisplay_GetSystemDevice();
    if ( !std3D_pD3Device )
    {
        STDLOG_ERROR("Error getting Direct3D device from stdDisplay.\n");
        return 0;
    }

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
    std3D_ReleaseSystemResources();

    std3D_numTextureFormats    = 0;
    std3D_curDevice            = 0;
    std3D_pCurDevice           = NULL;
    std3D_bHasRGBTextureFormat = false;
    std3D_bOpen                = false;
}

void J3DAPI std3D_GetTextureFormat(StdColorFormatType type, ColorInfo* pDest, int* pbColorKeySet, LPDDCOLORKEY* ppColorKey)
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

//int std3D_StartScene(void)
//{
//    ++std3D_frameCount;
//
//    HRESULT d3dres = IDirect3DDevice9_BeginScene(std3D_pD3Device);
//    if ( d3dres != D3D_OK ) {
//        STDLOG_ERROR("Error %s beginning scene.\n", std3D_D3DGetStatus(d3dres));
//    }
//
//    std3D_pD3DTex = NULL;
//
//    if ( std3D_bShadersActive )
//    {
//        StdShaderViewport vp =
//        {
//            (float)std3D_activeRect.x1, // x
//            (float)std3D_activeRect.y1, // y
//            (float)std3D_activeRect.x2, // width
//            (float)std3D_activeRect.y2  // height
//        };
//        if ( !stdShader_SetViewport(vp) )
//        {
//            // error
//        }
//    }
//
//    return d3dres;
//}


// Enhanced StartScene for multi-pass
int std3D_StartScene(void)
{
    ++std3D_frameCount;

    HRESULT d3dres = IDirect3DDevice9_BeginScene(std3D_pD3Device);
    if ( d3dres != D3D_OK )
    {
        STDLOG_ERROR("Error %s beginning scene.\n", std3D_D3DGetStatus(d3dres));
        std3D_pD3DTex = NULL;
        return d3dres;
    }

    std3D_pD3DTex = NULL;

    // Initialize multi-pass if shaders are active
    if ( std3D_bShadersActive && std3D_pRenderTargets[0] )
    {
        std3D_bMultiPassActive = true;
        std3D_currentPass = 0;

        // Store original render target
        IDirect3DDevice9_GetRenderTarget(std3D_pD3Device, 0, &std3D_pOriginalRenderTarget);
        IDirect3DDevice9_GetDepthStencilSurface(std3D_pD3Device, &std3D_pOriginalDepthStencil);

        // Start with albedo pass
        IDirect3DDevice9_SetRenderTarget(std3D_pD3Device, 0, std3D_pRTSurfaces[STDSHADER_RT_ALBEDO]);
        IDirect3DDevice9_Clear(std3D_pD3Device, 0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, 0x00000000, 1.0f, 0);
    }

    if ( std3D_bShadersActive )
    {
        StdShaderViewport vp =
        {
            (float)std3D_activeRect.x1, // x
            (float)std3D_activeRect.y1, // y
            (float)std3D_activeRect.x2, // width
            (float)std3D_activeRect.y2  // height
        };
        stdShader_SetViewport(vp);

        // Apply global parameters
        //stdShader_SetGlobalParams(&std3D_globalParams);
    }

    return d3dres;
}

static void std3D_DoMultiPassComposite(void)
{
    if ( !std3D_bMultiPassActive || !std3D_compositeShader ) return;

    // Set final render target
    IDirect3DDevice9_SetRenderTarget(std3D_pD3Device, 0, std3D_pOriginalRenderTarget);

    // Clear final target
    IDirect3DDevice9_Clear(std3D_pD3Device, 0, NULL, D3DCLEAR_TARGET, 0x00000000, 1.0f, 0);

    // Set composite shader
    std3D_UpdateShaderState(std3D_compositeShader);

    // Bind render target textures
    for ( int i = 0; i < STDSHADER_RT_COUNT - 1; i++ )
    {
        IDirect3DDevice9_SetTexture(std3D_pD3Device, i, (IDirect3DBaseTexture9*)std3D_pRenderTargets[i]);
    }

    // Draw fullscreen quad for compositing
    D3DTLVERTEX quad[4] =
    {
        { 0.0f, 0.0f, 0.0f, 1.0f, 0xFFFFFFFF, 0xFF000000, 0.0f, 0.0f },
        { (float)stdDisplay_g_backBuffer.rasterInfo.width, 0.0f, 0.0f, 1.0f, 0xFFFFFFFF, 0xFF000000, 1.0f, 0.0f },
        { (float)stdDisplay_g_backBuffer.rasterInfo.width, (float)stdDisplay_g_backBuffer.rasterInfo.height, 0.0f, 1.0f, 0xFFFFFFFF, 0xFF000000, 1.0f, 1.0f },
        { 0.0f, (float)stdDisplay_g_backBuffer.rasterInfo.height, 0.0f, 1.0f, 0xFFFFFFFF, 0xFF000000, 0.0f, 1.0f }
    };

    WORD indices[6] = { 0, 1, 2, 0, 2, 3 };

    HRESULT hr = IDirect3DDevice9_DrawIndexedPrimitiveUP(
        std3D_pD3Device,
        D3DPT_TRIANGLELIST,
        0, 4, 2,
        indices, D3DFMT_INDEX16,
        quad, sizeof(D3DTLVERTEX)
    );

    if ( FAILED(hr) )
    {
        STDLOG_ERROR("Failed composite draw: %s\n", std3D_D3DGetStatus(hr));
    }

    // Clean up texture bindings
    for ( int i = 0; i < STDSHADER_RT_COUNT - 1; i++ )
    {
        IDirect3DDevice9_SetTexture(std3D_pD3Device, i, NULL);
    }
}


//void std3D_EndScene(void)
//{
//    HRESULT d3dres = IDirect3DDevice9_EndScene(std3D_pD3Device);
//    if ( d3dres != D3D_OK ) {
//        STDLOG_ERROR("Error %s ending scene.\n", std3D_D3DGetStatus(d3dres));
//    }
//    std3D_pD3DTex = NULL;
//}

void std3D_EndScene(void)
{
    if ( std3D_bMultiPassActive )
    {
        // Do lighting pass if we have a lighting shader
        if ( std3D_lightingShader != STDSHADER_INVALIDHANDLE )
        {
            // Set lighting render target
            IDirect3DDevice9_SetRenderTarget(std3D_pD3Device, 0, std3D_pRTSurfaces[STDSHADER_RT_LIGHTING]);
            IDirect3DDevice9_Clear(std3D_pD3Device, 0, NULL, D3DCLEAR_TARGET, 0x00000000, 1.0f, 0);

            // Bind albedo texture
            IDirect3DDevice9_SetTexture(std3D_pD3Device, 0, (IDirect3DBaseTexture9*)std3D_pRenderTargets[STDSHADER_RT_ALBEDO]);

            std3D_UpdateShaderState(std3D_lightingShader);

            // Draw fullscreen quad for lighting
            D3DTLVERTEX quad[4] =
            {
                { 0.0f, 0.0f, 0.0f, 1.0f, 0xFFFFFFFF, 0xFF000000, 0.0f, 0.0f },
                { (float)stdDisplay_g_backBuffer.rasterInfo.width, 0.0f, 0.0f, 1.0f, 0xFFFFFFFF, 0xFF000000, 1.0f, 0.0f },
                { (float)stdDisplay_g_backBuffer.rasterInfo.width, (float)stdDisplay_g_backBuffer.rasterInfo.height, 0.0f, 1.0f, 0xFFFFFFFF, 0xFF000000, 1.0f, 1.0f },
                { 0.0f, (float)stdDisplay_g_backBuffer.rasterInfo.height, 0.0f, 1.0f, 0xFFFFFFFF, 0xFF000000, 0.0f, 1.0f }
            };

            WORD indices[6] = { 0, 1, 2, 0, 2, 3 };

            IDirect3DDevice9_DrawIndexedPrimitiveUP(
                std3D_pD3Device,
                D3DPT_TRIANGLELIST,
                0, 4, 2,
                indices, D3DFMT_INDEX16,
                quad, sizeof(D3DTLVERTEX)
            );
        }

        //if ( std3D_waveShader != STDSHADER_INVALIDHANDLE )
        //{
        //    // Set lighting render target
        //    IDirect3DDevice9_SetRenderTarget(std3D_pD3Device, 0, std3D_pRTSurfaces[STDSHADER_RT_EFFECTS]);
        //    IDirect3DDevice9_Clear(std3D_pD3Device, 0, NULL, D3DCLEAR_TARGET, 0x00000000, 1.0f, 0);

        //    // Bind albedo texture
        //    IDirect3DDevice9_SetTexture(std3D_pD3Device, 0, (IDirect3DBaseTexture9*)std3D_pRenderTargets[STDSHADER_RT_ALBEDO]);

        //    std3D_UpdateShaderState(std3D_waveShader);

        //    // Draw fullscreen quad for lighting
        //    D3DTLVERTEX quad[4] =
        //    {
        //        { 0.0f, 0.0f, 0.0f, 1.0f, 0xFFFFFFFF, 0xFF000000, 0.0f, 0.0f },
        //        { (float)stdDisplay_g_backBuffer.rasterInfo.width, 0.0f, 0.0f, 1.0f, 0xFFFFFFFF, 0xFF000000, 1.0f, 0.0f },
        //        { (float)stdDisplay_g_backBuffer.rasterInfo.width, (float)stdDisplay_g_backBuffer.rasterInfo.height, 0.0f, 1.0f, 0xFFFFFFFF, 0xFF000000, 1.0f, 1.0f },
        //        { 0.0f, (float)stdDisplay_g_backBuffer.rasterInfo.height, 0.0f, 1.0f, 0xFFFFFFFF, 0xFF000000, 0.0f, 1.0f }
        //    };

        //    WORD indices[6] = { 0, 1, 2, 0, 2, 3 };

        //    IDirect3DDevice9_DrawIndexedPrimitiveUP(
        //        std3D_pD3Device,
        //        D3DPT_TRIANGLELIST,
        //        0, 4, 2,
        //        indices, D3DFMT_INDEX16,
        //        quad, sizeof(D3DTLVERTEX)
        //    );
        //}

        // Do final composite
        std3D_DoMultiPassComposite();

        // Clean up multi-pass state
        std3D_bMultiPassActive = false;

        if ( std3D_pOriginalRenderTarget )
        {
            IDirect3DSurface9_Release(std3D_pOriginalRenderTarget);
            std3D_pOriginalRenderTarget = NULL;
        }

        if ( std3D_pOriginalDepthStencil )
        {
            IDirect3DSurface9_Release(std3D_pOriginalDepthStencil);
            std3D_pOriginalDepthStencil = NULL;
        }
    }

    HRESULT d3dres = IDirect3DDevice9_EndScene(std3D_pD3Device);
    if ( d3dres != D3D_OK ) {
        STDLOG_ERROR("Error %s ending scene.\n", std3D_D3DGetStatus(d3dres));
    }
    std3D_pD3DTex = NULL;
}

static int std3D_CopyVertexDataToBuffer(const LPD3DTLVERTEX aVerts, size_t numVerts, LPWORD aIndices, size_t numIndices)
{
    // Check for buffer wraparound
    DWORD vbLockFlags = D3DLOCK_NOOVERWRITE;
    DWORD ibLockFlags = D3DLOCK_NOOVERWRITE;

    // Only discard if we actually need to wrap
    bool vbNeedsWrap = (std3D_vbOffset + numVerts > std3D_vbSize);
    bool ibNeedsWrap = (std3D_ibOffset + numIndices > std3D_ibSize);

    if ( vbNeedsWrap )
    {
        vbLockFlags = D3DLOCK_DISCARD;
        std3D_vbOffset = 0;
    }

    if ( ibNeedsWrap )
    {
        ibLockFlags = D3DLOCK_DISCARD;
        std3D_ibOffset = 0;
    }


    // Copy vertices to vertex buffer at current offset
    size_t vbOffsetBytes   = std3D_vbOffset * sizeof(D3DTLVERTEX);
    size_t vbCopySizeBytes = numVerts * sizeof(D3DTLVERTEX);
    void* pVertData        = NULL;
    HRESULT hr = IDirect3DVertexBuffer9_Lock(std3D_pVertexBuffer, (UINT)vbOffsetBytes, (UINT)vbCopySizeBytes, &pVertData, vbLockFlags);
    if ( hr != D3D_OK )
    {
        STDLOG_ERROR("Error %s while locking vertex buffer.\n", std3D_D3DGetStatus(hr));
        return 0;
    }

    memcpy(pVertData, aVerts, vbCopySizeBytes);
    IDirect3DVertexBuffer9_Unlock(std3D_pVertexBuffer);

    // Copy indices to buffer at current offset
    // Lock index buffer (if needed)
    if ( aIndices && numIndices > 0 )
    {
        size_t ibOffsetBytes   = std3D_ibOffset * sizeof(WORD);
        size_t ibCopySizeBytes = numIndices * sizeof(WORD);
        void* pIndexData       = NULL;
        hr = IDirect3DIndexBuffer9_Lock(std3D_pIndexBuffer, ibOffsetBytes, ibCopySizeBytes, &pIndexData, ibLockFlags);
        if ( hr != D3D_OK )
        {
            STDLOG_ERROR("Error %s while locking index buffer.\n", std3D_D3DGetStatus(hr));
            return 0;
        }

        memcpy(pIndexData, aIndices, ibCopySizeBytes);
        IDirect3DIndexBuffer9_Unlock(std3D_pIndexBuffer);
    }

    return 1;
}

int std3D_DrawIndexedPrimitive(D3DPRIMITIVETYPE type, LPD3DTLVERTEX aVerts, size_t numVerts, LPWORD aIndices, size_t numIndices)
{
     // Copy vertices to buffers
    if ( !std3D_CopyVertexDataToBuffer(aVerts, numVerts, aIndices, numIndices) )
    {
        return 0;
    }

    // Draw
    HRESULT hr = IDirect3DDevice9_DrawIndexedPrimitive(
        std3D_pD3Device,
        type,
        (INT)std3D_vbOffset,
        0,
        (UINT)numVerts,
        (UINT)std3D_ibOffset,
        (UINT)(numIndices / 3)
    );

    if ( FAILED(hr) )
    {
        STDLOG_ERROR("DrawIndexedPrimitive failed: %s\n", std3D_D3DGetStatus(hr));
        return 0;
    }

    // Drawing succeeded, advance offsets
    std3D_vbOffset += numVerts;
    std3D_ibOffset += numIndices;

    return 1;
}

int std3D_DrawPrimitive(D3DPRIMITIVETYPE type, LPD3DTLVERTEX aVerts, size_t numVerts)
{
     // Copy vertices to buffers
    if ( !std3D_CopyVertexDataToBuffer(aVerts, numVerts, NULL, 0) )
    {
        return 0;
    }

    // Draw
    HRESULT hr = IDirect3DDevice9_DrawPrimitive(
        std3D_pD3Device,
        type,
        (UINT)std3D_vbOffset,
        numVerts - 1
    );

    if ( FAILED(hr) )
    {
        STDLOG_ERROR("DrawPrimitive failed: %s\n", std3D_D3DGetStatus(hr));
        return 0;
    }

    // Drawing succeeded, advance offsets
    std3D_vbOffset += numVerts;
    return 1;
}

void std3D_UpdateShaderState(StdShaderHandle activeShader)
{
    if ( activeShader == STDSHADER_INVALIDHANDLE ) return;

    // Set shader
    if ( activeShader != std3D_activeShader || stdShader_IsShaderDirty(activeShader) )
    {
        if ( !stdShader_SetActiveShader(activeShader) )
        {
            // Failed to set active shader
            return;
        }

        // Apply shader parameters
        stdShader_ApplyShaderParams(activeShader);
        std3D_activeShader = activeShader;
    }
}

void J3DAPI std3D_DrawRenderList(tSysTexture* pTex, Std3DRenderState rdflags, LPD3DTLVERTEX aVerts, size_t numVerts, LPWORD aIndices, size_t numIndices)
{
    std3D_DrawRenderListEx(pTex, rdflags, aVerts, numVerts, aIndices, numIndices, STDSHADER_INVALIDHANDLE);
    //if ( numVerts > (unsigned int)std3D_g_maxVertices )
    //{
    //    STDLOG_ERROR("Error %d > %d maxVertices.\n", numVerts, std3D_g_maxVertices);
    //    return;
    //}

    //if ( std3D_bShadersActive )
    //{
    //    std3D_UpdateShaderState(std3D_defaultShader);
    //}

    //std3D_SetRenderState(rdflags);

    //// Set texture
    //if ( pTex != std3D_pD3DTex )
    //{
    //    HRESULT d3dres = IDirect3DDevice9_SetTexture(std3D_pD3Device, 0, (IDirect3DBaseTexture9*)pTex);
    //    if ( d3dres != D3D_OK )
    //    {
    //        STDLOG_ERROR("Error %s SetTexture.\n", std3D_D3DGetStatus(d3dres));
    //    }
    //    else
    //    {
    //        std3D_pD3DTex = pTex;
    //    }
    //}

    //// Fog processing
    //if ( std3D_bFogTable )
    //{
    //    for ( size_t i = 0; i < numVerts; ++i )
    //    {
    //        D3DTLVERTEX* pCurVert = &aVerts[i];
    //        pCurVert->specular = 0xFF000000;
    //        if ( pCurVert->rhw > 0.0 )
    //        {
    //            pCurVert->specular = 0;
    //            float depth = (std3D_fogEndDepth - pCurVert->rhw * std3D_zDepth) * std3D_fogDepthFactor;
    //            if ( depth < 1.0 )
    //            {
    //                pCurVert->specular = 0xFF000000;
    //                if ( depth >= 0.0 )
    //                {
    //                    pCurVert->specular = (int)((1.0 - depth) * 255.0) << 24;
    //                }
    //            }
    //        }
    //    }
    //}

    //if ( std3D_bUseBuffers )
    //{
    //    if ( !std3D_DrawIndexedPrimitive(D3DPT_TRIANGLELIST, aVerts, numVerts, aIndices, numIndices) )
    //    {
    //         // draw failed
    //    }
    //}
    //else
    //{
    //    HRESULT d3dres = IDirect3DDevice9_DrawIndexedPrimitiveUP(
    //        std3D_pD3Device,
    //        D3DPT_TRIANGLELIST,
    //        0,
    //        numVerts,
    //        numIndices / 3,
    //        aIndices,
    //        D3DFMT_INDEX16,
    //        aVerts,
    //        sizeof(D3DTLVERTEX)
    //    );

    //    if ( d3dres != D3D_OK )
    //    {
    //        STDLOG_ERROR("Error %s DrawIndexedPrimitiveUP.\n", std3D_D3DGetStatus(d3dres));
    //    }
    //}
}

void std3D_SetWireframeRenderState(void)
{
    Std3DRenderState rdstate = std3D_renderState & ~(STD3D_RS_FOG_ENABLED | STD3D_RS_UNKNOWN_400 | STD3D_RS_UNKNOWN_200);
    std3D_SetRenderState(rdstate);

    HRESULT d3dres = IDirect3DDevice9_SetTexture(std3D_pD3Device, 0, NULL);
    if ( d3dres != D3D_OK )
    {
        STDLOG_ERROR("Error %s SetTexture.\n", std3D_D3DGetStatus(d3dres));
        return;
    }

    std3D_pD3DTex = NULL;
}

void J3DAPI std3D_DrawLineStrip(LPD3DTLVERTEX aVerts, size_t numVerts)
{
    if ( numVerts > std3D_g_maxVertices )
    {
        STDLOG_ERROR("Error %d > %d maxVertices.\n", numVerts, std3D_g_maxVertices);
        return;
    }

    if ( std3D_bShadersActive )
    {
        std3D_UpdateShaderState(std3D_defaultShaderWf);
    }

    if ( std3D_bUseBuffers )
    {
        if ( !std3D_DrawPrimitive(D3DPT_LINESTRIP, aVerts, numVerts) )
        {
             // draw failed
        }
    }
    else
    {
        HRESULT d3dres = IDirect3DDevice9_DrawPrimitiveUP(
            std3D_pD3Device,
            D3DPT_LINESTRIP,
            numVerts - 1,
            aVerts,
            sizeof(D3DTLVERTEX)
        );

        if ( d3dres != D3D_OK )
        {
            STDLOG_ERROR("Error %s DrawPrimitiveUP.\n", std3D_D3DGetStatus(d3dres));
        }
    }
}

void J3DAPI std3D_DrawPointList(LPD3DTLVERTEX aVerts, size_t numVerts)
{
    if ( numVerts > std3D_g_maxVertices )
    {
        STDLOG_ERROR("Error %d > %d maxVertices.\n", numVerts, std3D_g_maxVertices);
        return;
    }

    if ( std3D_bShadersActive )
    {
        std3D_UpdateShaderState(std3D_defaultShaderWf);
    }

    if ( std3D_bUseBuffers )
    {
        if ( !std3D_DrawPrimitive(D3DPT_LINESTRIP, aVerts, numVerts) )
        {
             // draw failed
        }
    }
    else
    {
        HRESULT d3dres = IDirect3DDevice9_DrawPrimitiveUP(
            std3D_pD3Device,
            D3DPT_POINTLIST,
            numVerts,
            aVerts,
            sizeof(D3DTLVERTEX)
        );

        if ( d3dres != D3D_OK ) {
            STDLOG_ERROR("Error %s DrawPrimitiveUP.\n", std3D_D3DGetStatus(d3dres));
        }
    }
}

void J3DAPI std3D_SetRenderState(Std3DRenderState rdflags)
{
    if ( std3D_renderState != rdflags )
    {
        if ( (std3D_renderState & STD3D_RS_ZWRITE_DISABLED) != (rdflags & STD3D_RS_ZWRITE_DISABLED) )
        {
            if ( (rdflags & STD3D_RS_ZWRITE_DISABLED) != 0 ) {
                IDirect3DDevice9_SetRenderState(std3D_pD3Device, D3DRS_ZWRITEENABLE, FALSE);
            }
            else {
                IDirect3DDevice9_SetRenderState(std3D_pD3Device, D3DRS_ZWRITEENABLE, TRUE);
            }
        }

        if ( (std3D_renderState & STD3D_RS_TEX_CPAMP_U) != (rdflags & STD3D_RS_TEX_CPAMP_U) )
        {
            if ( (rdflags & STD3D_RS_TEX_CPAMP_U) != 0 ) {
                IDirect3DDevice9_SetSamplerState(std3D_pD3Device, 0, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP);
            }
            else {
                IDirect3DDevice9_SetSamplerState(std3D_pD3Device, 0, D3DSAMP_ADDRESSU, D3DTADDRESS_WRAP);
            }
        }

        if ( (std3D_renderState & STD3D_RS_TEX_CPAMP_V) != (rdflags & STD3D_RS_TEX_CPAMP_V) )
        {
            if ( (rdflags & STD3D_RS_TEX_CPAMP_V) != 0 ) {
                IDirect3DDevice9_SetSamplerState(std3D_pD3Device, 0, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP);
            }
            else {
                IDirect3DDevice9_SetSamplerState(std3D_pD3Device, 0, D3DSAMP_ADDRESSV, D3DTADDRESS_WRAP);
            }
        }

        if ( (std3D_renderState & STD3D_RS_FOG_ENABLED) != (rdflags & STD3D_RS_FOG_ENABLED) )
        {
            if ( (rdflags & STD3D_RS_FOG_ENABLED) != 0 && std3D_bRenderFog )
            {
                if ( std3D_bShadersActive )
                {
                    stdShader_SetFog(std3D_bRenderFog, std3D_fogStartDepth, std3D_fogEndDepth, std3D_fogDepthFactor, std3D_fogColor);
                }
                else
                {
                    IDirect3DDevice9_SetRenderState(std3D_pD3Device, D3DRS_FOGENABLE, TRUE);

                }
            }
            else
            {
                if ( std3D_bShadersActive )
                {
                    stdShader_DisableFog();
                }
                else
                {
                    IDirect3DDevice9_SetRenderState(std3D_pD3Device, D3DRS_FOGENABLE, FALSE);
                }
            }
        }

        if ( (std3D_renderState & STD3D_RS_TEXFILTER_ANISOTROPIC) != (rdflags & STD3D_RS_TEXFILTER_ANISOTROPIC) )
        {
            if ( (rdflags & STD3D_RS_TEXFILTER_ANISOTROPIC) != 0 )
            {
                IDirect3DDevice9_SetSamplerState(std3D_pD3Device, 0, D3DSAMP_MAGFILTER, D3DTEXF_ANISOTROPIC);
                IDirect3DDevice9_SetSamplerState(std3D_pD3Device, 0, D3DSAMP_MINFILTER, D3DTEXF_ANISOTROPIC);
            }
            else if ( (std3D_pCurDevice->d3dDesc.TextureFilterCaps & D3DPTFILTERCAPS_MAGFLINEAR) != 0 )
            {
                IDirect3DDevice9_SetSamplerState(std3D_pD3Device, 0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
                IDirect3DDevice9_SetSamplerState(std3D_pD3Device, 0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
            }
            else if ( (std3D_pCurDevice->d3dDesc.TextureFilterCaps & D3DPTFILTERCAPS_MAGFPOINT) != 0 )
            {
                IDirect3DDevice9_SetSamplerState(std3D_pD3Device, 0, D3DSAMP_MAGFILTER, D3DTEXF_POINT);
                IDirect3DDevice9_SetSamplerState(std3D_pD3Device, 0, D3DSAMP_MINFILTER, D3DTEXF_POINT);
            }
        }
        else if ( (std3D_renderState & STD3D_RS_TEXFILTER_BILINEAR) != (rdflags & STD3D_RS_TEXFILTER_BILINEAR) )
        {
            if ( (rdflags & STD3D_RS_TEXFILTER_BILINEAR) != 0 )
            {
                IDirect3DDevice9_SetSamplerState(std3D_pD3Device, 0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
                IDirect3DDevice9_SetSamplerState(std3D_pD3Device, 0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
            }
            else if ( (std3D_pCurDevice->d3dDesc.TextureFilterCaps & D3DPTFILTERCAPS_MAGFPOINT) != 0 )
            {
                IDirect3DDevice9_SetSamplerState(std3D_pD3Device, 0, D3DSAMP_MAGFILTER, D3DTEXF_POINT);
                IDirect3DDevice9_SetSamplerState(std3D_pD3Device, 0, D3DSAMP_MINFILTER, D3DTEXF_POINT);
            }
        }

        if ( (std3D_renderState & STD3D_RS_ALPHAREF_SET) != (rdflags & STD3D_RS_ALPHAREF_SET) )
        {
            if ( (rdflags & STD3D_RS_ALPHAREF_SET) != 0 )
            {
                IDirect3DDevice9_SetRenderState(std3D_pD3Device, D3DRS_ALPHAREF, 0xA0);
                std3D_renderState = rdflags;
                return;
            }

            IDirect3DDevice9_SetRenderState(std3D_pD3Device, D3DRS_ALPHAREF, 0);
        }

        std3D_renderState = rdflags;
    }
}

void J3DAPI std3D_AllocSystemTexture(tSystemTexture* pTexture, tVBuffer** apVBuffers, size_t numMipLevels, StdColorFormatType formatType)
{
    memset(pTexture, 0, sizeof(tSystemTexture));

    if ( !std3D_numTextureFormats ) {
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

    size_t texSize = (pVBuffer->rasterInfo.colorInfo.bpp * texHeight * texWidth) / 8;
    if ( std3D_mipmapFilter == STD3D_MIPMAPFILTER_NONE ) {
        numMipLevels = 1;
    }

    D3DFORMAT d3dFormat = D3DFMT_UNKNOWN;
    if ( formatType == STDCOLOR_FORMAT_RGBA_1BITALPHA ) {
        d3dFormat = std3D_aTextureFormats[std3D_RGBAKeyTextureFormat].ddPixelFmt;
    }
    else if ( formatType == STDCOLOR_FORMAT_RGBA ) {
        d3dFormat = std3D_aTextureFormats[std3D_RGBATextureFormat].ddPixelFmt;
    }
    else {
        d3dFormat = std3D_aTextureFormats[std3D_RGBTextureFormat].ddPixelFmt;
    }

    // Allocate array for VBuffer pointers - NO DirectX objects created
    pTexture->apMipmaps = (tVBuffer**)STDMALLOC(numMipLevels * sizeof(tVBuffer*));
    if ( !pTexture->apMipmaps )
    {
        STDLOG_ERROR("Failed to allocate memory for VBuffer array.\n");
        goto error;
    }

    pTexture->numMipLevels = numMipLevels;
    pTexture->format       = d3dFormat;
    pTexture->textureSize  = texSize;

    // Create owned VBuffer copies for each mip level
    for ( size_t mmNum = 0; mmNum < numMipLevels; ++mmNum )
    {
        // Create new VBuffer copy using stdDisplay_VBufferNew
        pTexture->apMipmaps[mmNum] = stdDisplay_VBufferNew(&apVBuffers[mmNum]->rasterInfo, /*bUseVSurface*/0, /*bUseVideoMemory*/0); // Important: must not use video surface (D3D) as texture can live longer than display device
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

    if ( pTex->pCachedTexture )
    {
        std3D_RemoveTextureFromCacheList(pTex);
        IDirect3DTexture9_Release(pTex->pCachedTexture);
    }

    memset(pTex, 0, sizeof(tSystemTexture));
}

void J3DAPI std3D_AddToTextureCache(tSystemTexture* pCacheTexture, StdColorFormatType format)
{
    J3D_UNUSED(format);

    STD_ASSERTREL(pCacheTexture);

    LPDIRECT3DTEXTURE9 pD3DTex = NULL;

    if ( pCacheTexture->numMipLevels == 0 || !pCacheTexture->apMipmaps )
    {
        STDLOG_ERROR("No Source texture.\n");
        goto error;
    }

    if ( pCacheTexture->textureSize > std3D_pCurDevice->availableMemory ) {
        std3D_PurgeTextureCache(pCacheTexture->textureSize);
    }

    HRESULT d3dres = IDirect3DDevice9_CreateTexture(
        std3D_pD3Device,
        pCacheTexture->apMipmaps[0]->rasterInfo.width,
        pCacheTexture->apMipmaps[0]->rasterInfo.height,
        pCacheTexture->numMipLevels,
        0, // Usage
        pCacheTexture->format,
        D3DPOOL_MANAGED, // IMPORTANT: Must be managed video/system memory to copy pixel data to
        &pD3DTex,
        NULL
    );

    while ( d3dres == D3DERR_OUTOFVIDEOMEMORY )
    {
        if ( !std3D_PurgeTextureCache(pCacheTexture->textureSize) )
        {
            STDLOG_ERROR("Error: Unable to purge texture cache for %x bytes!!!.\n", pCacheTexture->textureSize);
            pD3DTex = NULL;
            goto error;
        }

        d3dres = IDirect3DDevice9_CreateTexture(
            std3D_pD3Device,
            pCacheTexture->apMipmaps[0]->rasterInfo.width,
            pCacheTexture->apMipmaps[0]->rasterInfo.height,
            pCacheTexture->numMipLevels,
            0, // Usage
            pCacheTexture->format,
            D3DPOOL_MANAGED, // IMPORTANT: Must be managed video/system memory to copy pixel data to
            &pD3DTex,
            NULL
        );
    }

    if ( d3dres != D3D_OK )
    {
        STDLOG_ERROR("Error %s creating Direct3D texture for cache.\n", std3D_D3DGetStatus(d3dres));
        goto error;
    }

    // Copy from device-independent texture to video memory using direct pixel access
    for ( uint32_t mmNum = 0; mmNum < pCacheTexture->numMipLevels; ++mmNum )
    {
        if ( !stdDisplay_VBufferLock(pCacheTexture->apMipmaps[mmNum]) )
        {
            STDLOG_ERROR("Failed to lock source texture level %d.\n", mmNum);
            goto error;
        }

        // Lock destination texture (video memory, D3DPOOL_MANAGED)
        D3DLOCKED_RECT destRect = { 0 };
        d3dres = IDirect3DTexture9_LockRect(pD3DTex, mmNum, &destRect, NULL, 0);

        while ( d3dres == D3DERR_OUTOFVIDEOMEMORY )
        {
            // Unlock source before retrying
            stdDisplay_VBufferUnlock(pCacheTexture->apMipmaps[mmNum]);

            if ( !std3D_PurgeTextureCache(pCacheTexture->textureSize) )
            {
                STDLOG_ERROR("Error: Unable to purge texture cache for %x bytes!!!.\n", pCacheTexture->textureSize);
                goto error;
            }

            // Re-lock source
            if ( !stdDisplay_VBufferLock(pCacheTexture->apMipmaps[mmNum]) )
            {
                STDLOG_ERROR("Failed re-locking source texture level %d.\n", mmNum);
                goto error;
            }

            // Retry locking destination
            d3dres = IDirect3DTexture9_LockRect(pD3DTex, mmNum, &destRect, NULL, 0);
        }

        if ( d3dres != D3D_OK )
        {
            stdDisplay_VBufferUnlock(pCacheTexture->apMipmaps[mmNum]);
            STDLOG_ERROR("Error %s locking destination texture level %d.\n", std3D_D3DGetStatus(d3dres), mmNum);
            goto error;
        }

        // Get mip level dimensions
        D3DSURFACE_DESC desc = { 0 };
        IDirect3DTexture9_GetLevelDesc(pD3DTex, mmNum, &desc);

        // Copy pixel data row by row
        for ( uint32_t row = 0; row < desc.Height; ++row )
        {
            void* pSrcPixels  = pCacheTexture->apMipmaps[mmNum]->pPixels + row * pCacheTexture->apMipmaps[mmNum]->rasterInfo.rowSize;
            void* pDestPixels = (uint8_t*)destRect.pBits + row * destRect.Pitch;
            size_t rowBytes   = J3DMIN(pCacheTexture->apMipmaps[mmNum]->rasterInfo.rowSize, destRect.Pitch);
            memcpy(pDestPixels, pSrcPixels, rowBytes);
        }

        // Unlock both textures
        stdDisplay_VBufferUnlock(pCacheTexture->apMipmaps[mmNum]);
        d3dres = IDirect3DTexture9_UnlockRect(pD3DTex, mmNum);
        if ( d3dres != D3D_OK )
        {
            STDLOG_ERROR("Error %s unlocking destination texture level %d.\n", std3D_D3DGetStatus(d3dres), mmNum);
            goto error;
        }
    }

    // Success
    pCacheTexture->pCachedTexture = pD3DTex;
    pCacheTexture->frameNum       = std3D_frameCount;
    std3D_AddTextureToCacheList(pCacheTexture);
    return;

error:
    if ( pD3DTex ) {
        IDirect3DTexture9_Release(pD3DTex);
    }

    pCacheTexture->pCachedTexture = NULL;
    pCacheTexture->frameNum       = 0;
    STDLOG_ERROR("Done error exit from std3D_AddToTextureCache.\n");
}

size_t J3DAPI std3D_GetMipMapCount(const tSystemTexture* pTexture)
{
    if ( !pTexture ) {
        return 0;
    }

    return pTexture->numMipLevels;
}

void std3D_ResetTextureCache(void)
{
    STDLOG_DEBUG("Clearing texture cache....\n");
    if ( std3D_pD3Device )
    {
        HRESULT d3dres = IDirect3DDevice9_SetTexture(std3D_pD3Device, 0, NULL);
        if ( d3dres != D3D_OK ) {
            STDLOG_ERROR("Error %s SetTexture.\n", std3D_D3DGetStatus(d3dres));
        }
    }

    tSystemTexture* pCurTex = std3D_pFirstTexCache;
    while ( pCurTex )
    {
        if ( pCurTex->pCachedTexture )
        {
            IDirect3DTexture9_Release(pCurTex->pCachedTexture);
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

    if ( std3D_pCurDevice ) {
        std3D_pCurDevice->availableMemory = std3D_pCurDevice->totalMemory;
    }

    std3D_frameCount = 1;
    std3D_pD3DTex    = NULL;
}

void J3DAPI std3D_UpdateFrameCount(tSystemTexture* pTexture)
{
    std3D_RemoveTextureFromCacheList(pTexture);
    std3D_AddTextureToCacheList(pTexture);
    pTexture->frameNum = std3D_frameCount; // Fixed: Moved frameNum update to the end of the function.
                                           //        Originally it was updated at the beginning of the function, 
                                           //        and the frameNum was immediately invalidated by call to std3D_RemoveTextureFromCacheList.
}

size_t J3DAPI std3D_FindClosestFormat(const ColorInfo* pMatch)
{
    if ( !std3D_numTextureFormats ) {
        return 0;
    }

    size_t bestMatchLevel = 0;
    size_t closestMatch = 0;
    for ( size_t i = 0; i < std3D_numTextureFormats; ++i )
    {
        StdTextureFormat* pFormat = &std3D_aTextureFormats[i];
        size_t matchLevel = 0;

        if ( pFormat->ci.colorMode == pMatch->colorMode )
        {
            matchLevel = 1;
            if ( pFormat->ci.bpp == pMatch->bpp )
            {
                matchLevel = 2;
                if ( pMatch->colorMode == STDCOLOR_RGB )
                {
                    if ( pFormat->ci.redBPP == pMatch->redBPP && pFormat->ci.greenBPP == pMatch->greenBPP && pFormat->ci.blueBPP == pMatch->blueBPP )
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

                    if ( pFormat->ci.redBPP == pMatch->redBPP
                        && pFormat->ci.greenBPP == pMatch->greenBPP
                        && pFormat->ci.blueBPP == pMatch->blueBPP
                        && pFormat->ci.alphaBPP == pMatch->alphaBPP )
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

   // Set vertex format for pre-transformed vertices
    if ( !std3D_bShadersActive )
    {
        if ( IDirect3DDevice9_SetFVF(std3D_pD3Device, D3DFVF_XYZRHW | D3DFVF_DIFFUSE | D3DFVF_SPECULAR | D3DFVF_TEX1) != D3D_OK ) {
            return 0;
        }
    }

    // Make sure clipping is enabled
    if ( IDirect3DDevice9_SetRenderState(std3D_pD3Device, D3DRS_CLIPPING, TRUE) != D3D_OK )
    {
        return 0;
    }

    if ( IDirect3DDevice9_SetRenderState(std3D_pD3Device, D3DRS_ZENABLE, TRUE) != D3D_OK )
    {
        return 0;
    }

    if ( IDirect3DDevice9_SetRenderState(std3D_pD3Device, D3DRS_ZWRITEENABLE, TRUE) != D3D_OK )
    {
        return 0;
    }

    if ( IDirect3DDevice9_SetRenderState(std3D_pD3Device, D3DRS_ZFUNC, D3DCMP_LESSEQUAL) != D3D_OK )
    {
        return 0;
    }

    std3D_renderState |= STD3D_RS_UNKNOWN_1;

    // Set mipmap filter
    if ( std3D_SetMipmapFilter(STD3D_MIPMAPFILTER_TRILINEAR) )
    {
        return 0;
    }

    // Set texture filtering
    if ( (std3D_pCurDevice->d3dDesc.TextureFilterCaps & D3DPTFILTERCAPS_MAGFANISOTROPIC) != 0 )
    {
        if ( IDirect3DDevice9_SetSamplerState(std3D_pD3Device, 0, D3DSAMP_MAGFILTER, D3DTEXF_ANISOTROPIC) != D3D_OK )
        {
            return 0;
        }

        if ( IDirect3DDevice9_SetSamplerState(std3D_pD3Device, 0, D3DSAMP_MINFILTER, D3DTEXF_ANISOTROPIC) != D3D_OK )
        {
            return 0;
        }

        std3D_renderState |= STD3D_RS_TEXFILTER_ANISOTROPIC;
    }
    else if ( (std3D_pCurDevice->d3dDesc.TextureFilterCaps & D3DPTFILTERCAPS_MAGFLINEAR) != 0 )
    {
        if ( IDirect3DDevice9_SetSamplerState(std3D_pD3Device, 0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR) != D3D_OK )
        {
            return 0;
        }

        if ( IDirect3DDevice9_SetSamplerState(std3D_pD3Device, 0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR) != D3D_OK )
        {
            return 0;
        }

        std3D_renderState |= STD3D_RS_TEXFILTER_BILINEAR;
    }

    else if ( (std3D_pCurDevice->d3dDesc.TextureFilterCaps & D3DPTFILTERCAPS_MAGFPOINT) != 0 )
    {
        if ( IDirect3DDevice9_SetSamplerState(std3D_pD3Device, 0, D3DSAMP_MAGFILTER, D3DTEXF_POINT) != D3D_OK )
        {
            return 0;
        }

        if ( IDirect3DDevice9_SetSamplerState(std3D_pD3Device, 0, D3DSAMP_MINFILTER, D3DTEXF_POINT) != D3D_OK )
        {
            return 0;
        }
    }

    // Just backport for DirectX 6 state
    std3D_renderState |= STD3D_RS_SUBPIXEL_CORRECTION;

    // Set texture wrapping
    if ( IDirect3DDevice9_SetSamplerState(std3D_pD3Device, 0, D3DSAMP_ADDRESSU, D3DTADDRESS_WRAP) != D3D_OK )
    {
        return 0;
    }

    if ( IDirect3DDevice9_SetSamplerState(std3D_pD3Device, 0, D3DSAMP_ADDRESSV, D3DTADDRESS_WRAP) != D3D_OK )
    {
        return 0;
    }

    // Set alpha blend state
    // TODO: Check for alpha support is backported from vanilla DirectX 6 version.
    //       Could probably be omitted.
    if ( (std3D_pCurDevice->d3dDesc.TextureOpCaps & D3DTEXOPCAPS_MODULATE) != 0
        && (std3D_pCurDevice->d3dDesc.SrcBlendCaps & D3DPBLENDCAPS_SRCALPHA) != 0
        && (std3D_pCurDevice->d3dDesc.DestBlendCaps & D3DPBLENDCAPS_INVSRCALPHA) != 0 )
    {
        if ( IDirect3DDevice9_SetRenderState(std3D_pD3Device, D3DRS_ALPHABLENDENABLE, TRUE) != D3D_OK )
        {
            return 0;
        }

        if ( IDirect3DDevice9_SetTextureStageState(std3D_pD3Device, 0, D3DTSS_COLOROP, D3DTOP_MODULATE) != D3D_OK )
        {
            return 0;
        }

        if ( IDirect3DDevice9_SetTextureStageState(std3D_pD3Device, 0, D3DTSS_COLORARG1, D3DTA_TEXTURE) != D3D_OK )
        {
            return 0;
        }

        if ( IDirect3DDevice9_SetTextureStageState(std3D_pD3Device, 0, D3DTSS_COLORARG2, D3DTA_CURRENT) != D3D_OK )
        {
            return 0;
        }

        if ( IDirect3DDevice9_SetTextureStageState(std3D_pD3Device, 0, D3DTSS_ALPHAOP, D3DTOP_MODULATE) != D3D_OK )
        {
            return 0;
        }

        if ( IDirect3DDevice9_SetTextureStageState(std3D_pD3Device, 0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE) != D3D_OK )
        {
            return 0;
        }

        if ( IDirect3DDevice9_SetTextureStageState(std3D_pD3Device, 0, D3DTSS_ALPHAARG2, D3DTA_CURRENT) != D3D_OK )
        {
            return 0;
        }

        if ( IDirect3DDevice9_SetRenderState(std3D_pD3Device, D3DRS_SRCBLEND, D3DBLEND_SRCALPHA) != D3D_OK )
        {
            return 0;
        }

        if ( IDirect3DDevice9_SetRenderState(std3D_pD3Device, D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA) != D3D_OK )
        {
            return 0;
        }

        if ( IDirect3DDevice9_SetRenderState(std3D_pD3Device, D3DRS_ALPHATESTENABLE, TRUE) != D3D_OK )
        {
            return 0;
        }

        if ( IDirect3DDevice9_SetRenderState(std3D_pD3Device, D3DRS_ALPHAFUNC, D3DCMP_GREATER) != D3D_OK )
        {
            return 0;
        }

        if ( IDirect3DDevice9_SetRenderState(std3D_pD3Device, D3DRS_ALPHAREF, 0) != D3D_OK )
        {
            return 0;
        }

        if ( (std3D_pCurDevice->d3dDesc.AlphaCmpCaps & D3DPCMPCAPS_GREATER) != 0 )
        {
            if ( IDirect3DDevice9_SetRenderState(std3D_pD3Device, D3DRS_ALPHAFUNC, D3DCMP_GREATER) != D3D_OK )
            {
                return 0;
            }

            if ( IDirect3DDevice9_SetRenderState(std3D_pD3Device, D3DRS_ALPHAREF, 0) != D3D_OK )
            {
                return 0;
            }
        }
    }
    else if ( IDirect3DDevice9_SetRenderState(std3D_pD3Device, D3DRS_ALPHABLENDENABLE, FALSE) != D3D_OK ) // No alpha blending supported
    {
        return 0;
    }

    // Set shade mode
    if ( IDirect3DDevice9_SetRenderState(std3D_pD3Device, D3DRS_SHADEMODE, D3DSHADE_GOURAUD) != D3D_OK )
    {
        return 0;
    }

    // Disable specular highlights
    if ( IDirect3DDevice9_SetRenderState(std3D_pD3Device, D3DRS_SPECULARENABLE, FALSE) != D3D_OK )
    {
        return 0;
    }

    // Disable lightening
    if ( IDirect3DDevice9_SetRenderState(std3D_pD3Device, D3DRS_LIGHTING, FALSE) != D3D_OK )
    {
        return 0;
    }

    if ( !(std3D_pCurDevice->d3dDesc.RasterCaps & D3DPRASTERCAPS_FOGTABLE) && !(std3D_pCurDevice->d3dDesc.RasterCaps & D3DPRASTERCAPS_FOGVERTEX) )
    {
        std3D_bRenderFog = false;
    }

    if ( IDirect3DDevice9_SetRenderState(std3D_pD3Device, D3DRS_FOGENABLE, FALSE) != D3D_OK )
    {
        return 0;
    }

    if ( std3D_bShadersActive )
    {
        stdShader_DisableFog();
    }

    if ( IDirect3DDevice9_SetRenderState(std3D_pD3Device, D3DRS_FILLMODE, D3DFILL_SOLID) != D3D_OK )
    {
        return 0;
    }

    if ( IDirect3DDevice9_SetRenderState(std3D_pD3Device, D3DRS_DITHERENABLE, TRUE) != D3D_OK )
    {
        return 0;
    }

    std3D_renderState |= STD3D_RS_UNKNOWN_2;

    return IDirect3DDevice9_SetRenderState(std3D_pD3Device, D3DRS_CULLMODE, D3DCULL_NONE) == D3D_OK;
}

int J3DAPI std3D_SetMipmapFilter(Std3DMipmapFilterType filter)
{
    HRESULT d3dres = D3D_OK;
    if ( filter == STD3D_MIPMAPFILTER_TRILINEAR
        && (std3D_pCurDevice->d3dDesc.TextureFilterCaps & D3DPTFILTERCAPS_MIPFLINEAR) == 0 )
    {
        filter = STD3D_MIPMAPFILTER_BILINEAR;
    }

    if ( filter == STD3D_MIPMAPFILTER_BILINEAR
        && (std3D_pCurDevice->d3dDesc.TextureFilterCaps & D3DPTFILTERCAPS_MIPFPOINT) == 0 )
    {
        filter = STD3D_MIPMAPFILTER_NONE;
    }

    if ( filter != std3D_mipmapFilter )
    {
        switch ( filter )
        {
            case STD3D_MIPMAPFILTER_BILINEAR:
                d3dres = IDirect3DDevice9_SetSamplerState(std3D_pD3Device, 0, D3DSAMP_MIPFILTER, D3DTEXF_POINT);
                break;
            case STD3D_MIPMAPFILTER_TRILINEAR:
                d3dres = IDirect3DDevice9_SetSamplerState(std3D_pD3Device, 0, D3DSAMP_MIPFILTER, D3DTEXF_LINEAR);
                break;
            default:
                d3dres = IDirect3DDevice9_SetSamplerState(std3D_pD3Device, 0, D3DSAMP_MIPFILTER, D3DTEXF_NONE);
                break;
        }
    }

    std3D_mipmapFilter = filter;
    return d3dres != D3D_OK;
}

int J3DAPI std3D_SetProjection(float fov, float nearPlane, float farPlane)
{
    float distance = farPlane - nearPlane;
    if ( fabs(distance) < 0.009999999776482582 )
    {
        return E_INVALIDARG;
    }

    float hfov = fov / 2.0f;
    float sinhfov = sinf(hfov);
    if ( fabs(sinhfov) < 0.009999999776482582f ) {
        return E_INVALIDARG;
    }

    float Q = farPlane / distance;
    float s = cosf(hfov) / sinhfov;

    D3DMATRIX proj = { 0 };
    proj._11 = s;
    proj._22 = s;
    proj._33 = Q;
    proj._34 = 1.0f;
    proj._43 = -(Q * nearPlane);

    std3D_zDepth = distance * 10.0f;
    return IDirect3DDevice9_SetTransform(std3D_pD3Device, D3DTS_PROJECTION, &proj);
}

void J3DAPI std3D_EnableFog(int bEnabled, float density)
{
    std3D_bRenderFog = bEnabled;
    if ( !std3D_pCurDevice || !std3D_pD3Device )
    {
        std3D_bFogTable  = false;
        return;
    }

    std3D_g_fogDensity = density;

    if ( std3D_bShadersActive )
    {
        return;
    }

     // Get device capabilities
    D3DCAPS9 caps;
    if ( FAILED(IDirect3DDevice9_GetDeviceCaps(std3D_pD3Device, &caps)) )
    {
        std3D_bRenderFog = false;
        return;
    }

    // Check fog support
    bool bTableFogSupported = (caps.RasterCaps & D3DPRASTERCAPS_FOGTABLE) != 0;
    bool bVertexFogSupported = (caps.RasterCaps & D3DPRASTERCAPS_FOGVERTEX) != 0;

    if ( !bTableFogSupported && !bVertexFogSupported && !std3D_bShadersActive )
    {
        std3D_bRenderFog = false;
    }

    std3D_bFogTable = bTableFogSupported && std3D_bRenderFog && !std3D_bShadersActive;
}

void J3DAPI std3D_SetFog(float red, float green, float blue, float startDepth, float endDepth)
{
    // Update table stare
    std3D_EnableFog(std3D_bRenderFog, std3D_g_fogDensity);

    if ( std3D_bShadersActive )
    {
        // Store fog parameters for shader use
        std3D_fogStartDepth  = startDepth;
        std3D_fogEndDepth    = (2.0f - std3D_g_fogDensity) * endDepth;
        std3D_fogDepthFactor = 1.0f / (std3D_fogEndDepth - std3D_fogStartDepth);

        std3D_fogColor[0] = red;
        std3D_fogColor[1] = green;
        std3D_fogColor[2] = blue;
        std3D_fogColor[3] = 1.0f;
    }
    else
    {
        if ( IDirect3DDevice9_SetRenderState(std3D_pD3Device, D3DRS_FOGCOLOR, D3DRGB(red, green, blue)) == D3D_OK )
        {
            if ( std3D_g_fogDensity == 0.0f )
            {
                IDirect3DDevice9_SetRenderState(std3D_pD3Device, D3DRS_FOGTABLEMODE, D3DFOG_NONE);
                IDirect3DDevice9_SetRenderState(std3D_pD3Device, D3DRS_FOGVERTEXMODE, D3DFOG_NONE);
            }
            else
            {
                endDepth = (2.0f - std3D_g_fogDensity) * endDepth;

                bool bSuccess = false;
                if ( std3D_bFogTable )
                {
                    // Use table fog for pre-transformed vertices
                    if ( SUCCEEDED(IDirect3DDevice9_SetRenderState(std3D_pD3Device, D3DRS_FOGTABLEMODE, D3DFOG_LINEAR)) )
                    {
                        bSuccess = IDirect3DDevice9_SetRenderState(std3D_pD3Device, D3DRS_FOGSTART, *(DWORD*)(&startDepth)) == D3D_OK
                            && IDirect3DDevice9_SetRenderState(std3D_pD3Device, D3DRS_FOGEND, *(DWORD*)(&endDepth)) == D3D_OK;
                    }
                }
                else
                {
                    // Use vertex fog (for non-pre-transformed vertices)
                    if ( SUCCEEDED(IDirect3DDevice9_SetRenderState(std3D_pD3Device, D3DRS_FOGVERTEXMODE, D3DFOG_LINEAR)) )
                    {
                        bSuccess = IDirect3DDevice9_SetRenderState(std3D_pD3Device, D3DRS_FOGSTART, *(DWORD*)(&startDepth)) == D3D_OK
                            && IDirect3DDevice9_SetRenderState(std3D_pD3Device, D3DRS_FOGEND, *(DWORD*)(&endDepth)) == D3D_OK;
                    }
                }

                if ( bSuccess )
                {
                    std3D_fogStartDepth  = startDepth;
                    std3D_fogEndDepth    = endDepth;
                    std3D_fogDepthFactor = 1.0f / (endDepth - startDepth);
                }
            }
        }
    }
}

void std3D_ClearZBuffer(void)
{
    if ( std3D_pD3Device )
    {
        HRESULT d3dres = IDirect3DDevice9_Clear(
            std3D_pD3Device,
            1,
            &std3D_activeRect,
            D3DCLEAR_ZBUFFER | D3DCLEAR_STENCIL | D3DCLEAR_TARGET, // TODO: Add D3DCLEAR_TARGET when enabled
            0,    // Color (black)
            1.0f, // Z value
            0     // Stencil value
        );

        if ( d3dres != D3D_OK )
        {
            STDLOG_ERROR("Error %s when clearing Z.\n", std3D_D3DGetStatus(d3dres));
        }
    }
}

int std3D_CreateViewport(void)
{
    // DirectX 9 doesn't use separate viewport objects like DX6
    // The viewport is set directly on the device

    D3DVIEWPORT9 viewport;
    viewport.X      = 0;
    viewport.Y      = 0;
    viewport.Width  = stdDisplay_g_backBuffer.rasterInfo.width;
    viewport.Height = stdDisplay_g_backBuffer.rasterInfo.height;
    viewport.MinZ   = 0.0f;
    viewport.MaxZ   = 1.0f;

    HRESULT d3dres = IDirect3DDevice9_SetViewport(std3D_pD3Device, &viewport);
    if ( d3dres != D3D_OK )
    {
        STDLOG_ERROR("Error %s when setting D3D9 viewport.\n", std3D_D3DGetStatus(d3dres));
        return 0;
    }

    // Set the active rectangle (for compatibility with existing code)
    std3D_activeRect.x1 = 0;
    std3D_activeRect.y1 = 0;
    std3D_activeRect.x2 = viewport.Width;
    std3D_activeRect.y2 = viewport.Height;

    // Clear the back buffer, Z-buffer, and stencil buffer
    // DirectX 9 uses Clear() on the device instead of viewport Clear2()
    d3dres = IDirect3DDevice9_Clear(
        std3D_pD3Device,
        0,                      // Count (0 = clear entire viewport)
        NULL,                   // pRects (NULL = clear entire viewport)
        D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER | D3DCLEAR_STENCIL,
        0x00000000,            // Color (black)
        1.0f,                  // Z value
        0                      // Stencil value
    );

    if ( d3dres != D3D_OK )
    {
        STDLOG_ERROR("Error %s when clearing viewport.\n", std3D_D3DGetStatus(d3dres));
        return 0;
    }

    return 1;
}

static bool J3DAPI std3D_GetZBufferFormat(tSysPixelFormat* pPixelFormat)
{
    if ( !std3D_pD3Device || !pPixelFormat )
    {
        return false;
    }

    // Check supported Z-buffer formats
    D3DFORMAT aFormats[] = {
        D3DFMT_D24S8,     // 24-bit depth, 8-bit stencil
    };

    for ( size_t i = 0; i < STD_ARRAYLEN(aFormats); i++ )
    {
        HRESULT hr = IDirect3D9_CheckDeviceFormat(std3D_pDirect3D, D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, stdDisplay_g_backBuffer.surface.desc.Format, D3DUSAGE_DEPTHSTENCIL, D3DRTYPE_SURFACE, aFormats[i]);
        if ( SUCCEEDED(hr) )
        {
            *pPixelFormat = aFormats[i];
            return true;
        }
    }

    return false; // No suitable Z-buffer format found
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
        pD3DDriver->d3dDesc                      = displayDevice.caps;
        pD3DDriver->bTexturePerspectiveSupported = TRUE; // Always supported in DX9
        pD3DDriver->hasZBuffer                   = TRUE; // Always supported in DX9
        pD3DDriver->bSqareOnlyTexture            = (displayDevice.caps.TextureCaps & D3DPTEXTURECAPS_SQUAREONLY) != 0;
        pD3DDriver->bAlphaTextureSupported       = (displayDevice.caps.TextureCaps & D3DPTEXTURECAPS_ALPHA) != 0;
        pD3DDriver->bColorkeyTextureSupported    = TRUE; // Always supported in DX9
        pD3DDriver->bStippledShadeSupported      = FALSE; // Not commonly used in DX9
        pD3DDriver->minTexWidth                  = 1;
        pD3DDriver->minTexHeight                 = 1;
        pD3DDriver->maxTexWidth                  = displayDevice.caps.MaxTextureWidth;
        pD3DDriver->maxTexHeight                 = displayDevice.caps.MaxTextureHeight;
        pD3DDriver->bAlphaBlendSupported         = (displayDevice.caps.SrcBlendCaps & D3DPBLENDCAPS_SRCALPHA) != 0
            && (displayDevice.caps.DestBlendCaps & D3DPBLENDCAPS_INVSRCALPHA) != 0;

        pD3DDriver->maxVertexCount = displayDevice.caps.MaxVertexIndex;
        if ( pD3DDriver->maxVertexCount == 0 ) {
            pD3DDriver->maxVertexCount = 65535; // Reasonable default
        }

        pD3DDriver->totalMemory     = displayDevice.totalVideoMemory;
        pD3DDriver->availableMemory = displayDevice.freeVideoMemory;

        // TODO: proly no point to make log here since same info can be logged in stdDisplay
        STDLOG_STATUS("Found |%s|%s|%s|%s| D3D Device\n",
            pD3DDriver->hasZBuffer ? "Z" : "Non-Z",
            pD3DDriver->bAlphaTextureSupported ? "Alpha" : "No Alpha",
            pD3DDriver->bStippledShadeSupported ? "Stippled" : "Blend",
            pD3DDriver->bColorkeyTextureSupported ? "Colorkey" : "No Colorkey"
        );

        STDLOG_STATUS("Description: %s [%s]\n", pD3DDriver->deviceName, pD3DDriver->deviceDescription);

        ++std3D_numDevices;
    }

    return std3D_numDevices > 0;
}

static void std3D_InitTextureFormats(void)
{
    std3D_numTextureFormats = 0;
    std3D_bHasRGBTextureFormat = false;

    // Common DX9 texture formats
    const D3DFORMAT formats[] = {
        D3DFMT_R8G8B8,      // 24-bit RGB
        D3DFMT_X8R8G8B8,    // 32-bit RGB
        D3DFMT_A8R8G8B8,    // 32-bit ARGB
        D3DFMT_R5G6B5,      // 16-bit RGB
        D3DFMT_A1R5G5B5,    // 16-bit ARGB
        D3DFMT_A4R4G4B4,    // 16-bit ARGB
       // D3DFMT_X1R5G5B5,    // 16-bit RGB
        D3DFMT_A8L8,        // 8-bit Alpha
        D3DFMT_L8,          // 8-bit Luminance
        D3DFMT_L16          // 16-bit Luminance
    };

    for ( size_t i = 0; i < STD_ARRAYLEN(formats) && std3D_numTextureFormats < STD_ARRAYLEN(std3D_aTextureFormats); ++i )
    {
        // Check if format is supported
        if ( IDirect3D9_CheckDeviceFormat(std3D_pDirect3D, std3D_curDevice, D3DDEVTYPE_HAL, stdDisplay_g_backBuffer.surface.desc.Format, 0, D3DRTYPE_TEXTURE, formats[i]) == D3D_OK )
        {
            StdTextureFormat* pTexFormat = &std3D_aTextureFormats[std3D_numTextureFormats];
            memset(pTexFormat, 0, sizeof(StdTextureFormat));

            pTexFormat->ddPixelFmt = formats[i];

            switch ( formats[i] )
            {
                case D3DFMT_R8G8B8:
                    pTexFormat->ci = stdColor_cfRGB888;
                    std3D_bHasRGBTextureFormat = true;
                    break;
                case D3DFMT_X8R8G8B8:
                    pTexFormat->ci = stdColor_cfRGB8888;
                    std3D_bHasRGBTextureFormat = true;
                    break;

                case D3DFMT_A8R8G8B8:
                    pTexFormat->ci = stdColor_cfARGB8888;
                    std3D_bHasRGBTextureFormat = true;
                    break;

                case D3DFMT_R5G6B5:
                    pTexFormat->ci = stdColor_cfRGB565;
                    std3D_bHasRGBTextureFormat = true;
                    break;

                case D3DFMT_A1R5G5B5:
                    pTexFormat->ci = stdColor_cfARGB5551;
                    std3D_bHasRGBTextureFormat = true;
                    break;

                case D3DFMT_A4R4G4B4:
                    pTexFormat->ci = stdColor_cfARGB4444;
                    std3D_bHasRGBTextureFormat = true;
                    break;
                case D3DFMT_L8:
                    STDLOG_STATUS("Found 8-bit Luminance tex format.\n");
                    continue;
                case D3DFMT_L16:
                    STDLOG_STATUS("Found 16-bit Luminance tex format.\n");
                    continue;
                case D3DFMT_A8L8:
                    STDLOG_STATUS("Found Luminance Alpha tex format (8:8).\n");
                    continue; // Skip unsupported formats
                default:
                    STDLOG_STATUS("Found Unknown tex format.\n");
                    continue; // Skip unsupported formats
            }

            STDLOG_STATUS("Found texture format: %d (%d:%d:%d:%d)\n", formats[i], pTexFormat->ci.redBPP, pTexFormat->ci.greenBPP, pTexFormat->ci.blueBPP, pTexFormat->ci.alphaBPP);
            ++std3D_numTextureFormats;
        }
    }
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
            if ( !std3D_pFirstTexCache->pNextCachedTexture ) {
                std3D_pLastTexCache = std3D_pFirstTexCache;
            }
        }
        else {
            std3D_pLastTexCache = NULL;
        }
    }
    else if ( pCacheTexture == std3D_pLastTexCache )
    {
        std3D_pLastTexCache = pCacheTexture->pPrevCachedTexture;
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
    for ( tSystemTexture* pCacheTexture = std3D_pFirstTexCache; pCacheTexture && pCacheTexture->frameNum != std3D_frameCount; pCacheTexture = pCacheTexture->pNextCachedTexture )
    {
        if ( pCacheTexture->textureSize == size )
        {
            IDirect3DTexture9_Release(pCacheTexture->pCachedTexture);
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
            if ( pCacheTexture->pCachedTexture ) {
                IDirect3DTexture9_Release(pCacheTexture->pCachedTexture);
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
                pCurInfo->aModes = (StdVideoMode*)STDMALLOC(sizeof(StdVideoMode) * pCurInfo->numModes);
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
                        size_t listSize = sizeof(Device3D) * pCurInfo->numDevices;
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

int std3D_InitVertexBuffers(void)
{
    std3D_vbOffset = 0;
    std3D_ibOffset = 0;

    // Create vertex buffer
    HRESULT hr = IDirect3DDevice9_CreateVertexBuffer(
        std3D_pD3Device,
        std3D_vbSize * sizeof(D3DTLVERTEX),
        D3DUSAGE_DYNAMIC | D3DUSAGE_WRITEONLY,
        0, // FVF not used with vertex declarations
        D3DPOOL_DEFAULT,
        &std3D_pVertexBuffer,
        NULL
    );
    if ( FAILED(hr) )
    {
        STDLOG_ERROR("Failed to create vertex buffer: %s\n", std3D_D3DGetStatus(hr));
        return 0;
    }

    // Create index buffer
    hr = IDirect3DDevice9_CreateIndexBuffer(
        std3D_pD3Device,
        std3D_ibSize * sizeof(WORD),
        D3DUSAGE_DYNAMIC | D3DUSAGE_WRITEONLY,
        D3DFMT_INDEX16,
        D3DPOOL_DEFAULT,
        &std3D_pIndexBuffer,
        NULL
    );
    if ( FAILED(hr) )
    {
        STDLOG_ERROR("Failed to create index buffer: %s\n", std3D_D3DGetStatus(hr));
        return 0;
    }

    // Set vertex/index buffers
    hr = IDirect3DDevice9_SetStreamSource(std3D_pD3Device, 0, std3D_pVertexBuffer, 0, sizeof(D3DTLVERTEX));
    if ( FAILED(hr) )
    {
        STDLOG_ERROR("Failed to set vertex buffer: %s\n", std3D_D3DGetStatus(hr));
        return 0;
    }

    hr = IDirect3DDevice9_SetIndices(std3D_pD3Device, std3D_pIndexBuffer);
    if ( FAILED(hr) )
    {
        STDLOG_ERROR("Failed to set index buffer: %s\n", std3D_D3DGetStatus(hr));
        return 0;
    }

    return 1;
}

void std3D_ReleaseVertexBuffers(void)
{
    if ( std3D_pVertexBuffer )
    {
        IDirect3DVertexBuffer9_Release(std3D_pVertexBuffer);
        std3D_pVertexBuffer = NULL;
    }

    if ( std3D_pIndexBuffer )
    {
        IDirect3DIndexBuffer9_Release(std3D_pIndexBuffer);
        std3D_pIndexBuffer = NULL;
    }
}

int std3D_InitShaderSystem(void)
{
    std3D_bShadersActive = false;
    if ( std3D_pCurDevice->d3dDesc.PixelShaderVersion < D3DPS_VERSION(3, 0) ||
        std3D_pCurDevice->d3dDesc.VertexShaderVersion < D3DVS_VERSION(3, 0) )
    {
        STDLOG_STATUS("Shader system disabled, due to not meeting minimum shader version requirements.\n"
            "  Pixel Shader Version: %d.%d, Vertex Shader Version: %d.%d\n",
            D3DSHADER_VERSION_MAJOR(std3D_pCurDevice->d3dDesc.PixelShaderVersion),
            D3DSHADER_VERSION_MINOR(std3D_pCurDevice->d3dDesc.PixelShaderVersion),
            D3DSHADER_VERSION_MAJOR(std3D_pCurDevice->d3dDesc.VertexShaderVersion),
            D3DSHADER_VERSION_MINOR(std3D_pCurDevice->d3dDesc.VertexShaderVersion)
        );
        return 1;
    }

    if ( !stdShader_Open() )
    {
        return 0;
    }

    // Create default shader
    std3D_defaultShader = stdShader_Create("std_default", std_default_vs, std_default_ps);
    if ( !std3D_defaultShader )
    {
        STDLOG_ERROR("Failed to create default shader\n");
        return 0;
    }

    std3D_defaultShaderWf = stdShader_Create("std_default", std_default_vs, std_default_wf_ps);
    if ( !std3D_defaultShader )
    {
        STDLOG_ERROR("Failed to create default wf shader\n");
        return 0;
    }

    if ( !stdShader_RegisterShaderParam(std3D_defaultShaderWf, "g_wireframeColor", STDSHADER_TYPE_PIXEL, STDSHADER_PARAM_VECTOR4, /*registerIndex=*/0) )
    {
        return 0;
    }

    StdShaderParamValue val;
    val.type = STDSHADER_PARAM_VECTOR4;
    val.value.vector[0] = 1.0f; // Red
    val.value.vector[1] = 1.0f; // Green
    val.value.vector[2] = 1.0f; // Blue
    val.value.vector[3] = 1.0f; // Alpha
    if ( !stdShader_SetShaderParam(std3D_defaultShaderWf, "g_wireframeColor", STDSHADER_TYPE_PIXEL, &val) )
    {
        return 0;
    }

    // Create composite shader
    std3D_compositeShader = stdShader_Create("std_composite", std_default_vs, std_composite_ps);
    if ( !std3D_compositeShader )
    {
        STDLOG_ERROR("Failed to create composite shader\n");
        return 0;
    }

    // Create wave shader
    std3D_waveShader = stdShader_Create("std_wave", std_wave_vs, std_wave_ps);
    if ( !std3D_waveShader )
    {
        STDLOG_ERROR("Failed to create wave shader\n");
        return 0;
    }

    // Initialize multi-pass render targets
    if ( !std3D_InitMultiPassRenderTargets() )
    {
        STDLOG_ERROR("Failed to initialize multi-pass render targets\n");
        return 1; // Continue without multi-pass
    }

    std3D_bShadersActive = true;
    return 1;
}

void std3D_ShutdownShaderSystem(void)
{
    // Release multi-pass render targets
    std3D_ReleaseMultiPassRenderTargets();

    if ( std3D_compositeShader )
    {
        stdShader_Free(std3D_compositeShader);
        std3D_compositeShader = STDSHADER_INVALIDHANDLE;
    }

    if ( std3D_lightingShader )
    {
        stdShader_Free(std3D_lightingShader);
        std3D_lightingShader = STDSHADER_INVALIDHANDLE;
    }

    if ( std3D_defaultShader )
    {
        stdShader_Free(std3D_defaultShader);
        std3D_defaultShader = STDSHADER_INVALIDHANDLE;
    }

    std3D_activeShader = STDSHADER_INVALIDHANDLE; // Important, reset active shader to STDSHADER_INVALIDHANDLE to avoid dangling handle on next system init
    stdShader_Close();
}

bool std3D_InitMultiPassRenderTargets(void)
{
    if ( !std3D_pD3Device )
    {
        return false;
    }

    UINT width = stdDisplay_g_backBuffer.rasterInfo.width;
    UINT height = stdDisplay_g_backBuffer.rasterInfo.height;

    // Create render targets for each pass
    for ( int i = 0; i < STDSHADER_RT_COUNT; i++ )
    {
        HRESULT hr = IDirect3DDevice9_CreateTexture(
            std3D_pD3Device,
            width, height,
            1, // mip levels
            D3DUSAGE_RENDERTARGET,
            stdDisplay_g_backBuffer.surface.desc.Format, // Use same format as back buffer
            D3DPOOL_DEFAULT,
            &std3D_pRenderTargets[i],
            NULL
        );

        if ( FAILED(hr) )
        {
            STDLOG_ERROR("Failed to create render target %d: %s\n", i, std3D_D3DGetStatus(hr));
            return false;
        }

        // Get render target surface
        hr = IDirect3DTexture9_GetSurfaceLevel(std3D_pRenderTargets[i], 0, &std3D_pRTSurfaces[i]);
        if ( FAILED(hr) )
        {
            STDLOG_ERROR("Failed to get render target surface %d: %s\n", i, std3D_D3DGetStatus(hr));
            return false;
        }
    }

    return true;
}

void std3D_ReleaseMultiPassRenderTargets(void)
{
    for ( int i = 0; i < STDSHADER_RT_COUNT; i++ )
    {
        if ( std3D_pRTSurfaces[i] )
        {
            IDirect3DSurface9_Release(std3D_pRTSurfaces[i]);
            std3D_pRTSurfaces[i] = NULL;
        }

        if ( std3D_pRenderTargets[i] )
        {
            IDirect3DTexture9_Release(std3D_pRenderTargets[i]);
            std3D_pRenderTargets[i] = NULL;
        }
    }
}


tSysDevice3D* std3D_GetD3DDevice(void)
{
    return std3D_pD3Device;
}

bool J3DAPI std3D_IsShaderSystemActive(void)
{
    return std3D_bShadersActive;
}

void J3DAPI std3D_DrawRenderListEx(tSysTexture* pTex, Std3DRenderState rdflags, LPD3DTLVERTEX aVerts, size_t numVerts, LPWORD aIndices, size_t numIndices, StdShaderHandle customShader)
{
    if ( numVerts > (unsigned int)std3D_g_maxVertices )
    {
        STDLOG_ERROR("Error %d > %d maxVertices.\n", numVerts, std3D_g_maxVertices);
        return;
    }

    StdShaderHandle activeShader = STDSHADER_INVALIDHANDLE;

    if ( std3D_bShadersActive )
    {
        // Use custom shader if provided, otherwise use default
        if ( customShader != STDSHADER_INVALIDHANDLE )
        {
            activeShader = customShader;
        }
        else
        {
            activeShader = std3D_defaultShader;
        }

        std3D_UpdateShaderState(activeShader);
    }

    // Rest of original DrawRenderList implementation...
    std3D_SetRenderState(rdflags);

    if ( pTex != std3D_pD3DTex )
    {
        HRESULT d3dres = IDirect3DDevice9_SetTexture(std3D_pD3Device, 0, (IDirect3DBaseTexture9*)pTex);
        if ( d3dres != D3D_OK )
        {
            STDLOG_ERROR("Error %s SetTexture.\n", std3D_D3DGetStatus(d3dres));
        }
        else
        {
            std3D_pD3DTex = pTex;
        }
    }

    // Fog processing (unchanged)
    if ( std3D_bFogTable )
    {
        for ( size_t i = 0; i < numVerts; ++i )
        {
            D3DTLVERTEX* pCurVert = &aVerts[i];
            pCurVert->specular = 0xFF000000;
            if ( pCurVert->rhw > 0.0 )
            {
                pCurVert->specular = 0;
                float depth = (std3D_fogEndDepth - pCurVert->rhw * std3D_zDepth) * std3D_fogDepthFactor;
                if ( depth < 1.0 )
                {
                    pCurVert->specular = 0xFF000000;
                    if ( depth >= 0.0 )
                    {
                        pCurVert->specular = (int)((1.0 - depth) * 255.0) << 24;
                    }
                }
            }
        }
    }

    // Draw geometry
    if ( std3D_bUseBuffers )
    {
        if ( !std3D_DrawIndexedPrimitive(D3DPT_TRIANGLELIST, aVerts, numVerts, aIndices, numIndices) )
        {
             // draw failed
        }
    }
    else
    {
        HRESULT d3dres = IDirect3DDevice9_DrawIndexedPrimitiveUP(
            std3D_pD3Device,
            D3DPT_TRIANGLELIST,
            0,
            numVerts,
            numIndices / 3,
            aIndices,
            D3DFMT_INDEX16,
            aVerts,
            sizeof(D3DTLVERTEX)
        );

        if ( d3dres != D3D_OK )
        {
            STDLOG_ERROR("Error %s DrawIndexedPrimitiveUP.\n", std3D_D3DGetStatus(d3dres));
        }
    }
}
#include <std/Win95/std3D.h>
#include <std/Win95/stdDisplay.h>
#include <std/Win95/stdWin95.h>
#include <std/Win95/stdShader.h>

#include <j3dcore/j3dhook.h>
#include <std/General/std.h>
#include <std/General/stdBmp.h>
#include <std/General/stdConfig.h>
#include <std/General/stdColor.h>
#include <std/General/stdMemory.h>
#include <std/General/stdUtil.h>

#include "wkernel/wkernel.h"


#define STDDISPLAY_MINFRAMERATE 30
#define STDDISPLAY_MAXFRAMERATE 256

typedef struct
{
    HDC hdc;
    HBITMAP hbm;
    void* bits;
    int w, h, pitch;
    int lockRef;
} GdiBuffer;

static GdiBuffer g_gdi      = { 0 };
static GdiBuffer g_frontGDI = { 0 };

// Public globals
tVBuffer stdDisplay_g_backBuffer = { 0 };

// Private globals
static bool stdDisplay_bStartup    = false;
static bool stdDisplay_bOpen       = false;
static bool stdDisplay_bModeSet    = false;
static bool stdDisplay_bFullscreen = false;
static bool stdDisplay_bNoSync     = false;
static bool stdDisplay_bDeviceLost = false;

static float stdDisplay_windowViewport[4] = { 0 };

// Back buffer local vars
static int stdDisplay_backbufWidth   = 0;
static int stdDisplay_backbufHeight  = 0;
static size_t stdDisplay_backLockRef = 0;
static HDC stdDisplay_hdcBack        = NULL;

// Front buffer local vars
static size_t stdDisplay_frontLockRef = 0;
static HDC stdDisplay_hdcFront        = NULL;

static StdVideoMode* stdDisplay_pCurVideoMode   = NULL;
static StdVideoMode stdDisplay_primaryVideoMode = { 0 };

static size_t stdDisplay_numVideoModes          = 0;
static StdVideoMode stdDisplay_aVideoModes[512] = { 0 };

static SDL_DisplayID stdDisplay_curDevice;
static SDL_DisplayID* stdDisplay_availableDisplays = NULL;
static StdDisplayDevice* stdDisplay_pCurDevice     = NULL;

static size_t stdDisplay_numDevices                    = 0;
static StdDisplayDevice stdDisplay_aDisplayDevices[16] = { 0 };

static HFONT stdDisplay_hFont;

static int stdDisplay_dword_5D73D8;
static int stdDisplay_dword_5D73DC;

// Callbacks
static tDisplayDevicePreResetCallback stdDisplay_pfDevicePreResetCallback   = NULL;
static tDisplayDevicePostResetCallback stdDisplay_pfDevicePostResetCallback = NULL;
static tDisplayDeviceReleaseCallback stdDisplay_pfDeviceReleaseCallback     = NULL;

// MSAA vars
static bool stdDisplay_bMSAAEnabled   = false;
static int stdDisplay_msaaSampleCount = 0;

static GLShaderProgram* stdDisplay_fboShader = NULL;
static GLuint stdDisplay_fullscreenVao       = 0;
// Helper functions
static int J3DAPI stdDisplay_VideoModeCompare(const StdVideoMode* pMode1, const StdVideoMode* pMode2);

static int stdDisplay_EnumerateDevices(void);
static int J3DAPI stdDisplay_EnumerateVideoModes(SDL_DisplayID adapter);
static bool stdDisplay_GetVideoColorFormat(SDL_PixelFormat format, ColorInfo* pFormat);

static inline void J3DAPI stdDisplay_SetAspectRatio(StdVideoMode* pMode);
static inline int J3DAPI stdDisplay_SetWindowMode(HWND hWnd, StdVideoMode* pDisplayMode);
static inline int J3DAPI stdDisplay_SetFullscreenMode(HWND hwnd, const StdVideoMode* pDisplayMode, size_t numBackBuffers);
static int J3DAPI stdDisplay_InitBuffers(PDIRECT3DDEVICE9 pDevice, const StdVideoMode* pDisplayMode, bool bWindowMode, size_t numBuffers);

static inline void stdDisplay_ReleaseBuffers(void);
static int stdDisplay_CheckDeviceState(void);
static void stdDisplay_ReleaseDevice(void);
static int stdDisplay_ResetDevice(void);
static int stdDisplay_InitFBOShader(void);


static int stdDisplay_InitGDIBackbuffer(GdiBuffer* buffer, int w, int h)
{
    if ( g_gdi.hdc ) return 1;

    BITMAPINFO bi              = { 0 };
    bi.bmiHeader.biSize        = sizeof(BITMAPINFOHEADER);
    bi.bmiHeader.biWidth       = w;
    bi.bmiHeader.biHeight      = -h;
    bi.bmiHeader.biPlanes      = 1;
    bi.bmiHeader.biBitCount    = 32;
    bi.bmiHeader.biCompression = BI_RGB;

    HDC screen  = GetDC(NULL);
    buffer->hdc = CreateCompatibleDC(screen);
    ReleaseDC(NULL, screen);

    g_gdi.hbm = CreateDIBSection(g_gdi.hdc, &bi, DIB_RGB_COLORS, &g_gdi.bits, NULL, 0);
    if ( !g_gdi.hbm || !g_gdi.bits ) return 0;

    SelectObject(g_gdi.hdc, g_gdi.hbm);
    g_gdi.w       = w;
    g_gdi.h       = h;
    g_gdi.pitch   = w * 4;
    g_gdi.lockRef = 0;
    return 1;
}

void stdDisplay_InstallHooks(void)
{
    J3D_HOOKFUNC(stdDisplay_Startup);
    J3D_HOOKFUNC(stdDisplay_Shutdown);
    J3D_HOOKFUNC(stdDisplay_Open);
    J3D_HOOKFUNC(stdDisplay_Close);
    J3D_HOOKFUNC(stdDisplay_SetMode);
    J3D_HOOKFUNC(stdDisplay_ClearMode);
    J3D_HOOKFUNC(stdDisplay_GetNumDevices);
    J3D_HOOKFUNC(stdDisplay_GetDevice);
    J3D_HOOKFUNC(stdDisplay_GetCurrentDevice);
    J3D_HOOKFUNC(stdDisplay_Refresh);
    J3D_HOOKFUNC(stdDisplay_VBufferNew);
    J3D_HOOKFUNC(stdDisplay_VBufferFree);
    J3D_HOOKFUNC(stdDisplay_VBufferLock);
    J3D_HOOKFUNC(stdDisplay_VBufferUnlock);
    //J3D_HOOKFUNC(stdDisplay_VBufferFill);
    J3D_HOOKFUNC(stdDisplay_VBufferConvertColorFormat);
    J3D_HOOKFUNC(stdDisplay_VideoModeCompare);
    J3D_HOOKFUNC(stdDisplay_GetTextureMemory);
    J3D_HOOKFUNC(stdDisplay_GetTotalMemory);
    J3D_HOOKFUNC(stdDisplay_CreateZBuffer);
    J3D_HOOKFUNC(stdDisplay_SetAspectRatio);
    J3D_HOOKFUNC(stdDisplay_SetWindowMode);
    J3D_HOOKFUNC(stdDisplay_SetFullscreenMode);
    J3D_HOOKFUNC(stdDisplay_ReleaseBuffers);
    //J3D_HOOKFUNC(stdDisplay_LockSurface);
    //J3D_HOOKFUNC(stdDisplay_UnlockSurface);
    J3D_HOOKFUNC(stdDisplay_Update);
    //J3D_HOOKFUNC(stdDisplay_ColorFillSurface);
    J3D_HOOKFUNC(stdDisplay_BackBufferFill);
    J3D_HOOKFUNC(stdDisplay_SaveScreen);
    J3D_HOOKFUNC(stdDisplay_SetDefaultResolution);
    J3D_HOOKFUNC(stdDisplay_GetBackBufferSize);
    J3D_HOOKFUNC(stdDisplay_GetNumVideoModes);
    J3D_HOOKFUNC(stdDisplay_GetVideoMode);
    J3D_HOOKFUNC(stdDisplay_GetCurrentVideoMode);
    J3D_HOOKFUNC(stdDisplay_GetFrontBufferDC);
    J3D_HOOKFUNC(stdDisplay_ReleaseFrontBufferDC);
    J3D_HOOKFUNC(stdDisplay_GetBackBufferDC);
    J3D_HOOKFUNC(stdDisplay_ReleaseBackBufferDC);
    J3D_HOOKFUNC(stdDisplay_FlipToGDISurface);
    J3D_HOOKFUNC(stdDisplay_CanRenderWindowed);
    J3D_HOOKFUNC(stdDisplay_SetBufferClipper);
    J3D_HOOKFUNC(stdDisplay_RemoveBufferClipper);
    J3D_HOOKFUNC(stdDisplay_IsFullscreen);
    J3D_HOOKFUNC(stdDisplay_LockBackBuffer);
    J3D_HOOKFUNC(stdDisplay_UnlockBackBuffer);
    J3D_HOOKFUNC(stdDisplay_EncodeFromRGB565);
}

void stdDisplay_ResetGlobals(void)
{
    memset(&stdDisplay_g_backBuffer, 0, sizeof(stdDisplay_g_backBuffer));
}

static void stdDisplay_InitMSAASettings(void)
{
    // Read MSAA settings from registry/config
    stdDisplay_bMSAAEnabled    = stdConfig_GetBool(STD3D_CFG_MSAAENABLED, true);
    stdDisplay_msaaSampleCount = stdConfig_GetInt(STD3D_CFG_MSAASAMPLES, 16);

    // Get max supported sample count from OpenGL
    GLint maxSamples = 0;
    glGetIntegerv(GL_MAX_SAMPLES, &maxSamples);

    if ( maxSamples <= 0 )
    {
        stdDisplay_bMSAAEnabled = false;
    }
    else if ( stdDisplay_msaaSampleCount > maxSamples )
    {
        stdDisplay_msaaSampleCount = maxSamples;
    }

    STDLOG_DEBUG("MSAA Settings: Enabled=%d, Samples=%d\n", stdDisplay_bMSAAEnabled, stdDisplay_msaaSampleCount);
}

int stdDisplay_Startup(void)
{
    STDLOG_STATUS("Starting display system using OpenGL GAPI ...\n");
    if ( stdDisplay_bStartup )
    {
        return 1;
    }

    memset(&stdDisplay_g_backBuffer, 0, sizeof(stdDisplay_g_backBuffer));

    stdDisplay_bStartup   = true;
    stdDisplay_numDevices = 0;

    if ( !SDL_GL_GetCurrentContext() )
    {
        STDLOG_ERROR("No OpenGL context was created yet.\n");
        return 0;
    }

    if ( !stdShader_Startup() || !stdDisplay_InitFBOShader() )
    {
        STDLOG_ERROR("Error initializing shader system.\n");
        return 0;
    }

    // Initialize MSAA settings from config
    stdDisplay_InitMSAASettings();

    // Enumerate devices and display modes
    if ( !stdDisplay_EnumerateDevices() )
    {
        return 0;
    }

    // Set default resolution
    stdDisplay_primaryVideoMode.rasterInfo.width  = 640;
    stdDisplay_primaryVideoMode.rasterInfo.height = 480;

    STDLOG_STATUS("Found %d Display Devices.\n", stdDisplay_numDevices);
    return 1;
}

static int stdDisplay_InitFBOShader(void)
{
    if ( !stdShader_Open() )
    {
        STDLOG_ERROR("Error opening shader system.\n");
        return 0;
    }

    stdDisplay_fboShader = stdShader_CompileAndCreate("std_fbo", "fbo.vert", "fbo.frag");
    if ( !stdDisplay_fboShader )
    {
        STDLOG_ERROR("Failed to creat fbo shader!\n");
    }

    glGenVertexArrays(1, &stdDisplay_fullscreenVao);
    return 1;
}

void stdDisplay_Shutdown(void)
{
    if ( stdDisplay_bOpen )
    {
        stdDisplay_Close();
    }

    stdShader_Shutdown();

    memset(stdDisplay_aDisplayDevices, 0, sizeof(stdDisplay_aDisplayDevices));
    memset(&stdDisplay_g_backBuffer, 0, sizeof(stdDisplay_g_backBuffer));

    stdDisplay_pCurVideoMode = NULL;
    stdDisplay_numDevices    = 0;
    stdDisplay_numVideoModes = 0;
    stdDisplay_bStartup      = false;
}

int J3DAPI stdDisplay_Open(size_t deviceNum)
{
    STD_ASSERTREL(stdDisplay_bStartup == true);
    if ( stdDisplay_bOpen )
    {
        STDLOG_ERROR("Warning: System already open!\n");
        stdDisplay_Close();
    }

    if ( deviceNum >= stdDisplay_numDevices )
    {
        STDLOG_ERROR("Error: Invalid device num %d (total: %d)!\n", deviceNum, stdDisplay_numDevices);
        return 0;
    }
    SDL_SetWindowAlwaysOnTop(SDL_GL_GetCurrentWindow(), false);

    stdDisplay_curDevice  = stdDisplay_availableDisplays[deviceNum];
    stdDisplay_pCurDevice = &stdDisplay_aDisplayDevices[deviceNum];

    // Enumerate display modes for this adapter
    stdDisplay_numVideoModes = 0;
    stdDisplay_EnumerateVideoModes(stdDisplay_curDevice);
    qsort(
        stdDisplay_aVideoModes,
        stdDisplay_numVideoModes,
        sizeof(StdVideoMode),
        (int(__cdecl*)(const void*, const void*))stdDisplay_VideoModeCompare
    );

    stdDisplay_bOpen = true;
    return 1;
}

bool stdDisplay_IsOpen(void)
{
    return stdDisplay_bOpen;
}

void stdDisplay_Close(void)
{
    if ( !stdDisplay_bStartup )
    {
        STDLOG_ERROR("stdDisplay_Close when not started.\n");
        return;
    }

    if ( !stdDisplay_bOpen )
    {
        STDLOG_ERROR("Warning: System already closed!\n");
        return;
    }

    if ( stdDisplay_bModeSet )
    {
        stdDisplay_ClearMode();
    }
    stdDisplay_numVideoModes = 0;

    stdDisplay_pfDevicePreResetCallback  = NULL;
    stdDisplay_pfDevicePostResetCallback = NULL;
    stdDisplay_pfDeviceReleaseCallback   = NULL;

    stdDisplay_curDevice  = 0;
    stdDisplay_pCurDevice = NULL;
    stdDisplay_bOpen      = false;
}

int J3DAPI stdDisplay_SetMode(size_t modeNum, int bFullscreen, size_t numBackBuffers)
{
    if ( bFullscreen && modeNum >= stdDisplay_numVideoModes )
    {
        return 1;
    }

    if ( stdDisplay_bModeSet )
    {
        stdDisplay_ClearMode();
    }

    if ( bFullscreen )
    {
        stdDisplay_pCurVideoMode = &stdDisplay_aVideoModes[modeNum];
        HWND hwnd                = stdWin95_GetWindow();
        if ( !stdDisplay_SetFullscreenMode(hwnd, &stdDisplay_aVideoModes[modeNum], numBackBuffers) )
        {
            return 1;
        }
    }
    else
    {
        stdDisplay_pCurVideoMode = &stdDisplay_aVideoModes[modeNum];
        HWND hwnd                = stdWin95_GetWindow();
        if ( !stdDisplay_SetWindowMode(hwnd, &stdDisplay_aVideoModes[modeNum]) )
        {
            return 1;
        }
    }

    int fheight      = -(stdDisplay_pCurVideoMode->rasterInfo.width < 640);
    fheight          = fheight & 0xF4;
    stdDisplay_hFont = CreateFont(fheight + 24, 0, 0, 0, FW_NORMAL, 0, 0, 0, ANSI_CHARSET, OUT_DEFAULT_PRECIS,
                                  CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, VARIABLE_PITCH, "Arial");

    stdDisplay_dword_5D73D8  = 0;
    stdDisplay_dword_5D73DC  = 0;
    stdDisplay_backbufWidth  = stdDisplay_g_backBuffer.rasterInfo.width;
    stdDisplay_backbufHeight = stdDisplay_g_backBuffer.rasterInfo.height;
    stdDisplay_bModeSet      = true;
    stdDisplay_bFullscreen   = bFullscreen;

    glClear(GL_COLOR_BUFFER_BIT);

    stdDisplay_Update();

    if ( bFullscreen )
    {
        glClear(GL_COLOR_BUFFER_BIT);
    }

    return 0;
}

void stdDisplay_ClearMode(void)
{
    if ( stdDisplay_bModeSet )
    {
        stdDisplay_ReleaseDevice();
    }

    if ( stdDisplay_hFont )
    {
        DeleteObject(stdDisplay_hFont);
        stdDisplay_hFont = NULL;
    }

    stdDisplay_bModeSet = false;
}

size_t stdDisplay_GetNumDevices(void)
{
    return stdDisplay_numDevices;
}

int J3DAPI stdDisplay_GetDevice(size_t deviceNum, StdDisplayDevice* pDest)
{
    if ( deviceNum >= stdDisplay_numDevices )
    {
        return 1;
    }

    *pDest = stdDisplay_aDisplayDevices[deviceNum];
    return 0;
}

int J3DAPI stdDisplay_GetCurrentDevice(StdDisplayDevice* pDevice)
{
    if ( stdDisplay_numDevices == 0 )
    {
        return 1;
    }

    *pDevice = stdDisplay_aDisplayDevices[stdDisplay_curDevice];
    return 0;
}

const StdDisplayDevice* stdDisplay_GetAllDevices(void)
{
    return stdDisplay_aDisplayDevices;
}

void J3DAPI stdDisplay_Refresh(int bReload)
{
    if ( stdDisplay_bOpen && stdDisplay_bModeSet && bReload )
    {
        if ( stdDisplay_CheckDeviceState() != 0 )
        {
            stdDisplay_ResetDevice();
        }
    }
}

void stdDisplay_RegisterDevicePreResetCallback(tDisplayDevicePreResetCallback pCallback)
{
    stdDisplay_pfDevicePreResetCallback = pCallback;
}

void stdDisplay_RegisterDevicePostResetCallback(tDisplayDevicePostResetCallback pCallback)
{
    stdDisplay_pfDevicePostResetCallback = pCallback;
}

void stdDisplay_RegisterDeviceReleaseCallback(tDisplayDeviceReleaseCallback pCallback)
{
    stdDisplay_pfDeviceReleaseCallback = pCallback;
}

// Device state checking and reset functions
static int J3DAPI stdDisplay_CheckDeviceState()
{
    return 0;
}

static void stdDisplay_ReleaseDevice(void)
{
    // Release all default pool resources
    stdDisplay_ReleaseBuffers();
}

static int stdDisplay_ResetDevice(void)
{
    if ( stdDisplay_pfDevicePreResetCallback )
    {
        stdDisplay_pfDevicePreResetCallback(NULL);
    }

    // Release all default pool resources before reset
    stdDisplay_ReleaseBuffers();

    // Recreate buffers
    if ( !stdDisplay_InitBuffers(NULL, stdDisplay_pCurVideoMode, 0, 0) )
    {
        STDLOG_ERROR("Error initializing buffers after device reset.\n");
        return 0;
    }

    stdDisplay_bDeviceLost = false;

    if ( stdDisplay_pfDevicePostResetCallback )
    {
        stdDisplay_pfDevicePostResetCallback(NULL);
    }

    return 1;
}

tVBuffer* J3DAPI stdDisplay_VBufferNew(const tRasterInfo* pRasterInfo, int bUseVSurface, int bUseVideoMemory) //check
{
    STD_ASSERTREL((pRasterInfo->colorInfo.bpp % 8) == 0);

    tVBuffer* vbuffer = STDMALLOC(sizeof(tVBuffer));
    if ( !vbuffer )
    {
        STDLOG_ERROR("Error allocating vbuffer.\n");
        return NULL;
    }

    vbuffer->pPixels      = NULL;
    vbuffer->lockRefCount = 0;
    vbuffer->rasterInfo   = *pRasterInfo;
    vbuffer->unknown1     = 0;

    uint32_t bpp                 = (uint32_t)vbuffer->rasterInfo.colorInfo.bpp / 8;
    vbuffer->rasterInfo.rowSize  = vbuffer->rasterInfo.width * bpp;
    vbuffer->rasterInfo.rowWidth = vbuffer->rasterInfo.width;
    vbuffer->rasterInfo.size     = vbuffer->rasterInfo.rowSize * vbuffer->rasterInfo.height;

    vbuffer->type         = VBUFFER_SOFTWARE;
    vbuffer->bVideoMemory = 0;
    vbuffer->pPixels      = (uint8_t*)STDMALLOC(vbuffer->rasterInfo.size);
    if ( !vbuffer->pPixels )
    {
        stdMemory_Free(vbuffer);
        return NULL;
    }

    vbuffer->lockRefCount = 1; // Set it to 1 as pPixels is set, which is normally done in VBufferLock function.
    return vbuffer;
}

void J3DAPI stdDisplay_VBufferFree(tVBuffer* pVBuffer)
{
    STD_ASSERTREL(pVBuffer != NULL);

    if ( pVBuffer->pPixels )
    {
        stdMemory_Free(pVBuffer->pPixels);
        pVBuffer->pPixels = NULL;
    }

    stdMemory_Free(pVBuffer);
}

int J3DAPI stdDisplay_VBufferLock(tVBuffer* pVBuffer)
{
    STD_ASSERTREL(pVBuffer != NULL);

    if ( pVBuffer->type == VBUFFER_SOFTWARE )
    {
        ++pVBuffer->lockRefCount;
    }
    else if ( pVBuffer->type == VBUFFER_HARDWARE )
    {
        // TODO: should lock front buffer same way to copy to copy front buff to lockable surface incase of MSAA
        if ( pVBuffer == &stdDisplay_g_backBuffer )
        {
            uint32_t width, height;
            int pitch;
            stdDisplay_LockBackBuffer((void**)&pVBuffer->pPixels, &width, &height, &pitch);
        }

        if ( !pVBuffer->pPixels )
        {
            return 0;
        }

        ++pVBuffer->lockRefCount;
    }

    return 1;
}

int J3DAPI stdDisplay_VBufferUnlock(tVBuffer* pVBuffer)
{
    STD_ASSERTREL(pVBuffer != NULL);

    if ( pVBuffer->type == VBUFFER_SOFTWARE )
    {
        if ( pVBuffer->lockRefCount )
        {
            --pVBuffer->lockRefCount;
            //TODO: Hmm shouldn't it be clamped to 1 as software buffer has always locked surface, i.g.: pPixels member set through out the life of VBuffer
        }
        return 1; // 1 marks still rock
    }

    if ( pVBuffer->type != VBUFFER_HARDWARE )
    {
        return 1;
    }

    if ( pVBuffer->lockRefCount == 0 )
    {
        return 0;
    }

    if ( pVBuffer == &stdDisplay_g_backBuffer )
    {
        stdDisplay_UnlockBackBuffer();
    }

    --pVBuffer->lockRefCount;

    return 0; // 1 locked - 0 unlocked
}

tVBuffer* J3DAPI stdDisplay_VBufferConvertColorFormat(const ColorInfo* pDesiredColorFormat, tVBuffer* pSrc, int bColorKey, LPDDCOLORKEY pColorKey)
{
    STD_ASSERTREL(pSrc != NULL);

    if ( memcmp(pDesiredColorFormat, &pSrc->rasterInfo.colorInfo, sizeof(ColorInfo)) == 0 )
    {
        return pSrc;
    }

    if ( pSrc->rasterInfo.colorInfo.colorMode == STDCOLOR_PAL )
    {
        if ( pDesiredColorFormat->colorMode == STDCOLOR_PAL )
        {
            return pSrc;
        }
        STD_ASSERTREL(pSrc->rasterInfo.colorInfo.colorMode != STDCOLOR_PAL);
    }

    const ColorInfo* pSrcColorFormat = &pSrc->rasterInfo.colorInfo;
    STD_ASSERTREL(pDesiredColorFormat->colorMode != STDCOLOR_PAL);
    STD_ASSERTREL(pSrcColorFormat->redBPP != 0);
    STD_ASSERTREL(pSrcColorFormat->greenBPP != 0);
    STD_ASSERTREL(pSrcColorFormat->blueBPP != 0);
    STD_ASSERTREL(pDesiredColorFormat->redBPP != 0);
    STD_ASSERTREL(pDesiredColorFormat->greenBPP != 0);
    STD_ASSERTREL(pDesiredColorFormat->blueBPP != 0);
    STD_ASSERTREL(
        pSrcColorFormat->redPosShift != 0 || pSrcColorFormat->greenPosShift != 0 || pSrcColorFormat->bluePosShift != 0);
    STD_ASSERTREL(
        pDesiredColorFormat->redPosShift != 0 || pDesiredColorFormat->greenPosShift != 0 || pDesiredColorFormat->
        bluePosShift != 0);

    tVBuffer* pDest;
    if ( pSrc->rasterInfo.colorInfo.bpp == pDesiredColorFormat->bpp )
    {
        pDest = pSrc;
    }
    else
    {
        tRasterInfo rasterInfo;
        memcpy(&rasterInfo, &pSrc->rasterInfo, sizeof(rasterInfo));
        memcpy(&rasterInfo.colorInfo, pDesiredColorFormat, sizeof(rasterInfo.colorInfo));

        pDest = stdDisplay_VBufferNew(&rasterInfo, /*bUseVSurface=*/0, /*bUseVideoMemory=*/0);
        if ( !pDest )
        {
            STDLOG_ERROR("Unable to allocate memory for new tVBuffer");
            return NULL;
        }
    }

    // Convert pixel data
    STD_ASSERTREL(pSrc->pPixels != NULL);
    STD_ASSERTREL(pDest->pPixels != NULL);

    stdDisplay_VBufferLock(pSrc);
    stdDisplay_VBufferLock(pDest);

    const uint8_t* pSrcRow = NULL;
    uint8_t* pDestRow      = NULL;
    for ( size_t row = 0; row < pDest->rasterInfo.height; ++row )
    {
        pSrcRow  = &pSrc->pPixels[pSrc->rasterInfo.rowSize * row];
        pDestRow = &pDest->pPixels[pDest->rasterInfo.rowSize * row];

        stdColor_ColorConvertOneRow(
            pDestRow,
            pDesiredColorFormat,
            pSrcRow,
            &pSrc->rasterInfo.colorInfo,
            pDest->rasterInfo.width,
            bColorKey,
            pColorKey
        );
    }

    STD_ASSERTREL(pDestRow <= pDest->pPixels + pDest->rasterInfo.size);
    STD_ASSERTREL(pSrcRow <= pSrc->pPixels + pSrc->rasterInfo.size);
    stdDisplay_VBufferUnlock(pSrc);
    stdDisplay_VBufferUnlock(pDest);

    // Copy color format
    memcpy(&pDest->rasterInfo.colorInfo, pDesiredColorFormat, sizeof(pDest->rasterInfo.colorInfo));

    if ( pDest != pSrc )
    {
        stdDisplay_VBufferFree(pSrc);
    }

    return pDest;
}

int J3DAPI stdDisplay_VideoModeCompare(const StdVideoMode* pMode1, const StdVideoMode* pMode2)
{
    unsigned int bpp1 = pMode1->rasterInfo.colorInfo.bpp;
    unsigned int bpp2 = pMode2->rasterInfo.colorInfo.bpp;
    if ( bpp1 != bpp2 )
    {
        return bpp1 - bpp2;
    }

    unsigned int width1 = pMode1->rasterInfo.width;
    unsigned int width2 = pMode2->rasterInfo.width;
    if ( width1 != width2 )
    {
        return width1 - width2;
    }

    unsigned int height1 = pMode1->rasterInfo.height;
    unsigned int height2 = pMode2->rasterInfo.height;
    return height1 - height2;
}

int J3DAPI stdDisplay_GetTextureMemory(size_t* pTotal, size_t* pFree)
{
    GLint maxTex = 0;
    glGetIntegerv(GL_MAX_TEXTURE_SIZE, &maxTex);
    if ( maxTex <= 0 ) maxTex = 4096; // defensiver Fallback

    *pTotal = (size_t)maxTex * (size_t)maxTex * 4; // RGBA8-Annahme
    *pFree  = *pTotal;                             // unbekannt
    return 0;
}

int J3DAPI stdDisplay_GetTotalMemory(size_t* pTotal, size_t* pFree)
{
    // TODO: Enhance / Fix
    *pTotal = stdDisplay_numVideoModes * 1024 * 1024; // Approximation
    *pFree  = *pTotal / 2;                            // Rough estimate
    return 0;
}

int J3DAPI stdDisplay_CreateZBuffer(const tSysPixelFormat* pPixelFormat, int bSystemMemory)
{
    J3D_UNUSED(bSystemMemory);
    J3D_UNUSED(pPixelFormat);

    if ( !stdDisplay_bOpen )
    {
        STDLOG_ERROR("Error creating zBuffer, system is closed!\n");
        return 1;
    }

    //add depth buffer as texture attachment to backbuffer fbo
    stdShader_SetActiveTextureUnit(TU_DEPTH);
    tVSurface* surface = &stdDisplay_g_backBuffer.surface;
    glBindFramebuffer(GL_FRAMEBUFFER, surface->fbo);
    glGenTextures(1, &surface->depthTex);
    glBindTexture(GL_TEXTURE_2D, surface->depthTex);

    uint32_t width  = stdDisplay_g_backBuffer.rasterInfo.width;
    uint32_t height = stdDisplay_g_backBuffer.rasterInfo.height;
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH24_STENCIL8, width, height, 0, GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_NONE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_TEXTURE_2D, surface->depthTex, 0);

    if ( stdDisplay_bMSAAEnabled )
    {
        glBindFramebuffer(GL_FRAMEBUFFER, surface->msaaFbo);
        glGenRenderbuffers(1, &surface->msaaDepthTex);
        glBindRenderbuffer(GL_RENDERBUFFER, surface->msaaDepthTex);
        glRenderbufferStorageMultisample(GL_RENDERBUFFER, stdDisplay_msaaSampleCount, GL_DEPTH24_STENCIL8, width, height);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, surface->msaaDepthTex);
    }


    stdShader_SetActiveTextureUnit(TU_DEFAULT);


    return 0;
}

static int J3DAPI stdDisplay_EnumerateDevices(void) //check
{
    if ( stdDisplay_availableDisplays )
    {
        SDL_free(stdDisplay_availableDisplays);
    }

    SDL_DisplayID primaryDisplay = SDL_GetPrimaryDisplay();
    int adapterCount;
    stdDisplay_availableDisplays = SDL_GetDisplays(&adapterCount);
    stdDisplay_numDevices        = 0;

    for ( int i = 0; i < adapterCount - 1; i++ )
    {
        if ( stdDisplay_availableDisplays[i] == primaryDisplay )
        {
            SDL_DisplayID tmp                              = stdDisplay_availableDisplays[adapterCount - 1];
            stdDisplay_availableDisplays[adapterCount - 1] = stdDisplay_availableDisplays[i];
            stdDisplay_availableDisplays[i]                = tmp;
            break;
        }
    }

    if ( !stdDisplay_availableDisplays || adapterCount <= 0 )
    {
        STDLOG_ERROR("SDL_GetDisplays failed or found no displays: %s\n", SDL_GetError());
        stdDisplay_numDevices = 0;
        return 0;
    }

    const char* renderer = (const char*)glGetString(GL_RENDERER);

    GLint maxTex = 0;
    glGetIntegerv(GL_MAX_TEXTURE_SIZE, &maxTex);
    GLint maxSamples = 0;
    glGetIntegerv(0x8D57 /*GL_MAX_SAMPLES*/, &maxSamples);

#ifndef GL_GPU_MEMORY_INFO_TOTAL_AVAILABLE_MEMORY_NVX
#define GL_GPU_MEMORY_INFO_TOTAL_AVAILABLE_MEMORY_NVX    0x9048
#define GL_GPU_MEMORY_INFO_CURRENT_AVAILABLE_VIDMEM_NVX  0x9049
#endif
#ifndef GL_TEXTURE_FREE_MEMORY_ATI
#define GL_TEXTURE_FREE_MEMORY_ATI                       0x87FC
#endif
    GLint total_kb = 0, free_kb = 0;
    glGetIntegerv(GL_GPU_MEMORY_INFO_TOTAL_AVAILABLE_MEMORY_NVX, &total_kb); // NVIDIA
    glGetIntegerv(GL_GPU_MEMORY_INFO_CURRENT_AVAILABLE_VIDMEM_NVX, &free_kb);
    if ( !total_kb )
    {
        GLint ati[4] = { 0, 0, 0, 0 };
        glGetIntegerv(GL_TEXTURE_FREE_MEMORY_ATI, ati); // AMD/ATI
        // ati[0] = total-ish, ati[1] = largest free block, ati[2]/[3] = aux/tex free
        total_kb = ati[0];
        free_kb  = ati[0];
    }

    for ( int i = 0; i < adapterCount && i < STD_ARRAYLEN(stdDisplay_aDisplayDevices); i++ )
    {
        StdDisplayDevice* pDevice = &stdDisplay_aDisplayDevices[stdDisplay_numDevices];

        // Fill device information
        const char* pDisplayName = SDL_GetDisplayName(stdDisplay_availableDisplays[i]);;
        // Left strip name to the last '\' (.e.g. "\\.\DISPLAY1" -> "\DISPLAY1")
        STD_STRCPY(pDevice->aDriverName, pDisplayName);                          //aDriver should be actually aDisplayDevice
        STD_STRCPY(pDevice->aDeviceName, renderer ? renderer : "OpenGL Device"); // aDeviceName should be a3DDevice


        // Try to get monitor friendly name
        DISPLAY_DEVICE displayDevice;
        displayDevice.cb = sizeof(displayDevice);
        if ( EnumDisplayDevices(pDisplayName, 0, &displayDevice, 0) )
        {
            STD_STRCPY(pDevice->aDriverName, displayDevice.DeviceString);
        }

        // Get device capabilities
        ZeroMemory(&pDevice->caps, sizeof(pDevice->caps));
        pDevice->bHAL                      = TRUE;
        pDevice->bWindowRenderNotSupported = FALSE; // OpenGL/SDL always support windowed rendering
        //pDevice->guid                      = identifier.DeviceIdentifier;
        if ( total_kb > 0 )
        {
            pDevice->totalVideoMemory = total_kb * 1024;
            pDevice->freeVideoMemory  = free_kb * 1024;
        }
        else
        {
            pDevice->totalVideoMemory = maxTex * maxTex * 4; // guessing
            pDevice->freeVideoMemory  = pDevice->totalVideoMemory / 2;
        }

        STDLOG_STATUS("Found %s OpenGL Device: %s [%s]\n", pDevice->bHAL ? "HAL" : "REF", pDevice->aDeviceName,
                      pDevice->aDriverName);
        STDLOG_STATUS("Memory: 0x%x out of 0x%x free\n", pDevice->freeVideoMemory, pDevice->totalVideoMemory);
        ++stdDisplay_numDevices;
    }

    return stdDisplay_numDevices > 0;
}

static int J3DAPI stdDisplay_EnumerateVideoModes(SDL_DisplayID adapter)
{
    //Check that the current desktop mode is supported is skipped in GL. Is that needed?

    int modeCount                      = 0;
    SDL_DisplayMode** modes            = SDL_GetFullscreenDisplayModes(adapter, &modeCount);
    const SDL_DisplayMode* desktopMode = SDL_GetDesktopDisplayMode(adapter);


    if ( !modes || modeCount <= 0 )
    {
        STDLOG_ERROR("  SDL_GetFullscreenDisplayModes failed: %s", SDL_GetError());
    }


    for ( int i = 0; i < modeCount && stdDisplay_numVideoModes < STD_ARRAYLEN(stdDisplay_aVideoModes); i++ )
    {
        SDL_DisplayMode* mode = modes[i];
        if ( mode->refresh_rate != desktopMode->refresh_rate )
            if ( mode->refresh_rate != desktopMode->refresh_rate )
            {
                continue; //due to borderless window mode, refresh rate will be always equal to desktop refresh rat.
            }


        // Filter out modes below 24-bit color and 30 Hz
        const int bpp = SDL_BITSPERPIXEL(mode->format);
        if ( bpp < 24 || mode->refresh_rate < STDDISPLAY_MINFRAMERATE || mode->refresh_rate > STDDISPLAY_MAXFRAMERATE )
        {
            continue;
        }

        StdVideoMode* pVideoMode      = &stdDisplay_aVideoModes[stdDisplay_numVideoModes];
        pVideoMode->refreshRate       = (uint32_t)(mode->refresh_rate + 0.5f);;
        pVideoMode->rasterInfo.width  = mode->w;
        pVideoMode->rasterInfo.height = mode->h;
        pVideoMode->format            = mode->format;
        pVideoMode->pDisplayMode      = *mode;

        // Set color bit information based on format
        if ( !stdDisplay_GetVideoColorFormat(mode->format, &pVideoMode->rasterInfo.colorInfo) )
        {
            STDLOG_ERROR("Couldn't get color info for format %d, adapter: %d videomode: %d ", mode->format, adapter, i);
            continue;
        }

        // Calculate row information
        unsigned int bytesPerPixel      = bpp / 8;
        pVideoMode->rasterInfo.rowSize  = pVideoMode->rasterInfo.width * bytesPerPixel;
        pVideoMode->rasterInfo.rowWidth = pVideoMode->rasterInfo.width;
        pVideoMode->rasterInfo.size     = pVideoMode->rasterInfo.rowSize * pVideoMode->rasterInfo.height;
        stdDisplay_SetAspectRatio(pVideoMode);

        // Check memory requirements (copied from DX6)
        size_t requiredVRam = 3 * pVideoMode->rasterInfo.size;
        STDLOG_STATUS("Video Mode: %ux%u %u bit (%u Hz), Required: %u bytes.\n", pVideoMode->rasterInfo.width,
                      pVideoMode->rasterInfo.height, bpp, pVideoMode->refreshRate, requiredVRam);

        ++stdDisplay_numVideoModes;
    }

    if ( modeCount > STD_ARRAYLEN(stdDisplay_aVideoModes) - stdDisplay_numVideoModes )
    {
        STDLOG_WARNING("Too many video modes for adapter %d, only %zu modes supported.\n", adapter,
                       STD_ARRAYLEN(stdDisplay_aVideoModes) - stdDisplay_numVideoModes);
    }

    SDL_free(modes);


    return stdDisplay_numVideoModes;
}

// Get color info for format of video mode and video surface
bool stdDisplay_GetVideoColorFormat(SDL_PixelFormat format, ColorInfo* pFormat)
{
    switch ( format )
    {
        case SDL_PIXELFORMAT_RGB24:
            *pFormat = stdColor_cfRGB888;
            return true;
        case SDL_PIXELFORMAT_XRGB8888:
        case SDL_PIXELFORMAT_ARGB8888:
            *pFormat = stdColor_cfRGB8888;
            // Note, no alpha for video mode color format
            return true;

        case SDL_PIXELFORMAT_RGB565:
            *pFormat = stdColor_cfRGB565;
            return true;

        case SDL_PIXELFORMAT_XRGB1555:
            *pFormat = stdColor_cfRGB555;
            return true;

        default:
            return false;
    }
}

void J3DAPI stdDisplay_SetAspectRatio(StdVideoMode* pMode)
{
    if ( pMode->rasterInfo.width == 320 && pMode->rasterInfo.height == 200 )
    {
        pMode->aspectRatio = 0.75f;
    }
    else if ( pMode->rasterInfo.width == 320 && pMode->rasterInfo.height == 400 )
    {
        pMode->aspectRatio = 0.75f;
    }
    else if ( pMode->rasterInfo.width == 640 && pMode->rasterInfo.height == 400 )
    {
        pMode->aspectRatio = 0.75f;
    }
    else
    {
        pMode->aspectRatio = 1.0f;
    }
}

LPDIRECT3D9 stdDisplay_GetDirect3D(void)
{
    return NULL;
}

tSysDisplayDevice* stdDisplay_GetSystemDevice(void)
{
    return NULL;
}

static int J3DAPI stdDisplay_SetWindowMode(HWND hWnd, StdVideoMode* pDisplayMode)
{
    J3D_UNUSED(hWnd);

    //in window mode, just make window as big as framebuffer
    SDL_Window* window           = SDL_GL_GetCurrentWindow();
    stdDisplay_windowViewport[0] = 0;
    stdDisplay_windowViewport[1] = 0;
    stdDisplay_windowViewport[2] = pDisplayMode->rasterInfo.width;
    stdDisplay_windowViewport[3] = pDisplayMode->rasterInfo.height;
    stdDisplay_curDevice         = SDL_GetDisplayForWindow(window);
    SDL_Rect displayRect;
    SDL_GetDisplayBounds(stdDisplay_curDevice, &displayRect);
    int Top, Left, Bottom, Right;
    SDL_GetWindowBordersSize(window, &Top, &Left, &Bottom, &Right);
    SDL_SetWindowPosition(window, displayRect.x + Left, displayRect.y + Top);

    wkernel_SetWindowSize(pDisplayMode->rasterInfo.width, pDisplayMode->rasterInfo.height);

    return stdDisplay_InitBuffers(NULL, pDisplayMode, /*bWindowMode=*/true, 1);
}

int J3DAPI stdDisplay_SetFullscreenMode(HWND hwnd, const StdVideoMode* pDisplayMode, size_t numBackBuffers)
{
    J3D_UNUSED(hwnd);
    J3D_UNUSED(numBackBuffers);


    // Use borderless fullscreen so dialogboxes are visible.

    const SDL_DisplayMode* desktopMode = SDL_GetDesktopDisplayMode(stdDisplay_curDevice);
    int winW                           = desktopMode->w;
    int winH                           = desktopMode->h;

    // calculate window aspect ratio and target aspect ratio of the frame buffer
    float targetAspect = (float)pDisplayMode->rasterInfo.width / (float)pDisplayMode->rasterInfo.height;
    float windowAspect = (float)winW / (float)winH;

    int vpW, vpH, vpX, vpY;

    //correct aspect ratio and stretch framebuffer size to window size
    if ( windowAspect > targetAspect )
    {
        vpH = winH;
        vpW = (int)(winH * targetAspect);
        vpX = (winW - vpW) / 2;
        vpY = 0;
    }
    else
    {
        vpW = winW;
        vpH = (int)(winW / targetAspect);
        vpX = 0;
        vpY = (winH - vpH) / 2;
    }

    stdDisplay_windowViewport[0] = vpX;
    stdDisplay_windowViewport[1] = vpY;
    stdDisplay_windowViewport[2] = vpW;
    stdDisplay_windowViewport[3] = vpH;


    wkernel_SetWindowSize(winW, winH);


    //SDL_SetWindowFullscreenMode(SDL_GL_GetCurrentWindow(), &pDisplayMode->pDisplayMode);
    //SDL_SetWindowFullscreen(SDL_GL_GetCurrentWindow(), true);
    //SDL_SetWindowAlwaysOnTop(SDL_GL_GetCurrentWindow(), false);
    //SDL_RaiseWindow(SDL_GL_GetCurrentWindow());

    return stdDisplay_InitBuffers(NULL, pDisplayMode, /*bWindowMode=*/true, 1);
}

int J3DAPI stdDisplay_InitBuffers(PDIRECT3DDEVICE9 pDevice, const StdVideoMode* pDisplayMode, bool bWindowMode, size_t numBuffers)
{
    tVSurface* surface = &stdDisplay_g_backBuffer.surface;
    if ( surface->fbo > 0 )
    {
        STDLOG_WARNING("Backbuffer with a FBO is already initialized, skipping re-initialisation!\n");
        return 1;
    }
    stdDisplay_backLockRef = 0;


    stdDisplay_g_backBuffer.lockRefCount = 0;
    stdDisplay_g_backBuffer.bVideoMemory = 1;
    stdDisplay_g_backBuffer.type         = VBUFFER_HARDWARE;
    stdDisplay_g_backBuffer.pPixels      = NULL;

    uint32_t width  = pDisplayMode->rasterInfo.width;
    uint32_t height = pDisplayMode->rasterInfo.height;

    // Update raster info with actual back buffer dimensions
    stdDisplay_g_backBuffer.rasterInfo        = pDisplayMode->rasterInfo;
    stdDisplay_g_backBuffer.rasterInfo.width  = width;
    stdDisplay_g_backBuffer.rasterInfo.height = height;

    unsigned int bpp                            = pDisplayMode->rasterInfo.colorInfo.bpp / 8;
    stdDisplay_g_backBuffer.rasterInfo.rowSize  = pDisplayMode->rasterInfo.width * bpp;
    stdDisplay_g_backBuffer.rasterInfo.rowWidth = width;
    stdDisplay_g_backBuffer.rasterInfo.size     = stdDisplay_g_backBuffer.rasterInfo.rowSize;

    stdShader_SetActiveTextureUnit(TU_SCENE);
    // generate fbo
    glGenFramebuffers(1, &surface->fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, surface->fbo);

    glGenTextures(1, &surface->colorTex);
    glBindTexture(GL_TEXTURE_2D, surface->colorTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, surface->colorTex, 0);

    if ( stdDisplay_bMSAAEnabled )
    {
        glGenFramebuffers(1, &surface->msaaFbo);
        glBindFramebuffer(GL_FRAMEBUFFER, surface->msaaFbo);

        glGenRenderbuffers(1, &surface->msaaColorTex);
        glBindRenderbuffer(GL_RENDERBUFFER, surface->msaaColorTex);
        glRenderbufferStorageMultisample(GL_RENDERBUFFER, stdDisplay_msaaSampleCount, GL_RGBA8, width, height);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, surface->msaaColorTex);
    }


    stdShader_SetActiveShader(stdDisplay_fboShader);
    GLint loc = glGetUniformLocation(stdDisplay_fboShader->handle, "sSceneTexture");
    glUniform1i(loc, TU_SCENE);

    stdShader_SetActiveTextureUnit(TU_DEFAULT);


    if ( glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE )
    {
        STDLOG_ERROR("FBO setup for backbuffer failed!\n");
        return 0;
    }

    return 1;
}


void stdDisplay_ReleaseBuffers(void) // checked
{
    tVSurface* surface = &stdDisplay_g_backBuffer.surface;
    if ( !surface )
        return;

    stdShader_SetActiveTextureUnit(0);
    glDeleteFramebuffers(1, &surface->fbo);
    glDeleteTextures(1, &surface->colorTex);
    glDeleteTextures(1, &surface->depthTex);
    stdShader_SetActiveTextureUnit(1);
    surface->fbo      = 0;
    surface->colorTex = 0;
    surface->depthTex = 0;

    if ( stdDisplay_bMSAAEnabled )
    {
        glDeleteFramebuffers(1, &surface->msaaFbo);
        glDeleteRenderbuffers(1, &surface->msaaColorTex);
        glDeleteRenderbuffers(1, &surface->msaaDepthTex);
    }

    if ( stdDisplay_g_backBuffer.pPixels )
    {
        stdMemory_Free(stdDisplay_g_backBuffer.pPixels);
        stdDisplay_g_backBuffer.pPixels = NULL;
    }
}

void stdDisplay_DisableVSync(bool bDisable)
{
    if ( stdDisplay_bNoSync != bDisable )
    {
        // Update presentation parameters
        if ( bDisable )
        {
            SDL_GL_SetSwapInterval(0);
        }
        else
        {
            SDL_GL_SetSwapInterval(1);
        }

        stdDisplay_bNoSync = bDisable;
    }
}

int stdDisplay_Update(void)
{
    SDL_Window* pWindow = SDL_GL_GetCurrentWindow();
    if ( !pWindow )
    {
        return 1;
    }
    glBindVertexArray(stdDisplay_fullscreenVao);
    glDisable(GL_DEPTH_TEST);

    tVSurface* backBufferSurface = &stdDisplay_g_backBuffer.surface;
    uint32_t width               = stdDisplay_g_backBuffer.rasterInfo.width;
    uint32_t height              = stdDisplay_g_backBuffer.rasterInfo.height;

    if ( stdDisplay_bMSAAEnabled && !backBufferSurface->skipMSAA )
    {
        glBindFramebuffer(GL_READ_FRAMEBUFFER, backBufferSurface->msaaFbo);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, backBufferSurface->fbo);
        glBlitFramebuffer(0, 0, width, height, 0, 0, width, height, GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT, GL_NEAREST);
    }

    backBufferSurface->skipMSAA = false;

    const float* vp = stdDisplay_windowViewport;
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(vp[0], vp[1], vp[2], vp[3]);
    stdShader_SetActiveShader(stdDisplay_fboShader);
    glClear(GL_COLOR_BUFFER_BIT);
    glDisable(GL_BLEND);

    glDrawArrays(GL_TRIANGLES, 0, 3);


    SDL_GL_SwapWindow(pWindow);

    if ( stdDisplay_bMSAAEnabled )
    {
        glBindFramebuffer(GL_FRAMEBUFFER, stdDisplay_g_backBuffer.surface.msaaFbo);
    }
    else
    {
        glBindFramebuffer(GL_FRAMEBUFFER, stdDisplay_g_backBuffer.surface.fbo);
    }
    //glClear(GL_COLOR_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glViewport(0, 0, stdDisplay_g_backBuffer.rasterInfo.width, stdDisplay_g_backBuffer.rasterInfo.height);

    return 0;
}

int J3DAPI stdDisplay_BackBufferFill(uint32_t color, const StdRect* pRect)
{
    J3D_UNUSED(color);
    J3D_UNUSED(pRect);
    glClear(GL_COLOR_BUFFER_BIT);
    return 0;
}

int J3DAPI stdDisplay_SaveScreen(const char* pFilename)
{
    if ( stdDisplay_g_backBuffer.surface.fbo == 0 )
    {
        return 1;
    }

    return stdBmp_WriteVBuffer(pFilename, &stdDisplay_g_backBuffer);
}

void J3DAPI stdDisplay_SetDefaultResolution(uint32_t width, uint32_t height)
{
    stdDisplay_primaryVideoMode.rasterInfo.width  = width;
    stdDisplay_primaryVideoMode.rasterInfo.height = height;
}

void J3DAPI stdDisplay_GetBackBufferSize(uint32_t* pWidth, uint32_t* pHeight)
{
    *pWidth  = stdDisplay_g_backBuffer.rasterInfo.width;
    *pHeight = stdDisplay_g_backBuffer.rasterInfo.height;
}

size_t stdDisplay_GetNumVideoModes(void)
{
    return stdDisplay_numVideoModes;
}

int J3DAPI stdDisplay_GetVideoMode(size_t modeNum, StdVideoMode* pDestMode)
{
    if ( modeNum >= stdDisplay_numVideoModes )
    {
        return 1;
    }

    memcpy(pDestMode, &stdDisplay_aVideoModes[modeNum], sizeof(StdVideoMode));
    return 0;
}

int J3DAPI stdDisplay_GetCurrentVideoMode(StdVideoMode* pDisplayMode)
{
    if ( !stdDisplay_pCurVideoMode )
    {
        return 1;
    }

    memcpy(pDisplayMode, stdDisplay_pCurVideoMode, sizeof(StdVideoMode));
    return 0;
}

HDC stdDisplay_GetFrontBufferDC(void)
{
    if ( !stdDisplay_bOpen || !stdDisplay_bModeSet )
        return NULL;

    if ( stdDisplay_frontLockRef > 0 )
    {
        stdDisplay_frontLockRef++;
        return g_frontGDI.hdc;
    }

    int w = (int)stdDisplay_g_backBuffer.rasterInfo.width;
    int h = (int)stdDisplay_g_backBuffer.rasterInfo.height;

    if ( !g_frontGDI.hdc || g_frontGDI.w != w || g_frontGDI.h != h )
    {
        if ( !stdDisplay_InitGDIBackbuffer(&g_frontGDI, w, h) )
        {
            STDLOG_ERROR("Failed to create GDI frontbuffer.\n");
            return NULL;
        }
    }

    stdDisplay_frontLockRef = 1;
    stdDisplay_hdcFront     = g_frontGDI.hdc;
    return g_frontGDI.hdc;
}

void J3DAPI stdDisplay_ReleaseFrontBufferDC(HDC hdc)
{
    if ( !g_frontGDI.hdc || hdc != g_frontGDI.hdc )
        return;

    if ( --g_frontGDI.lockRef > 0 )
        return;

    stdDisplay_hdcFront = NULL;
}

HDC stdDisplay_GetBackBufferDC(void)
{
    if ( !stdDisplay_bOpen || !stdDisplay_bModeSet ) return NULL;

    if ( stdDisplay_backLockRef > 0 )
    {
        stdDisplay_backLockRef++;
        return g_gdi.hdc;
    }

    const int w = (int)stdDisplay_g_backBuffer.rasterInfo.width;
    const int h = (int)stdDisplay_g_backBuffer.rasterInfo.height;

    if ( !g_gdi.hdc || g_gdi.w != w || g_gdi.h != h )
    {
        if ( !stdDisplay_InitGDIBackbuffer(&g_gdi, w, h) )
        {
            STDLOG_ERROR("Failed to create GDI backbuffer.\n");
            return NULL;
        }
    }

    stdDisplay_backLockRef = 1;
    stdDisplay_hdcBack     = g_gdi.hdc;
    return g_gdi.hdc;
}

void J3DAPI stdDisplay_ReleaseBackBufferDC(HDC hdc)
{
    if ( !g_gdi.hdc || hdc != g_gdi.hdc )
        return;

    if ( --g_gdi.lockRef > 0 )
        return;

    if ( stdDisplay_g_backBuffer.pPixels )
    {
        size_t copyPitch = stdDisplay_g_backBuffer.rasterInfo.rowSize;
        uint8_t* dst     = stdDisplay_g_backBuffer.pPixels;
        uint8_t* src     = g_gdi.bits;

        for ( int y = 0; y < g_gdi.h; ++y )
        {
            memcpy(dst + y * copyPitch, src + y * g_gdi.pitch, copyPitch);
        }
    }

    stdDisplay_hdcBack = NULL;
}

int stdDisplay_FlipToGDISurface(void)
{
    // DirectX 9 doesn't have a direct equivalent to FlipToGDISurface
    // TODO: Do something about this and can be removed after GDI stuff is out
    return S_OK;
}

int J3DAPI stdDisplay_CanRenderWindowed(void)
{
    return 1; //always true in OpenGL/SDL
}

int J3DAPI stdDisplay_SetBufferClipper(int bFrontBuffer)
{
    J3D_UNUSED(bFrontBuffer);

    // DirectX 9 handles clipping automatically in windowed mode
    // No explicit clipper management needed
    return 1; // Success - clipping is handled automatically
}

HRESULT J3DAPI stdDisplay_RemoveBufferClipper(int bFrontBuffer)
{
    J3D_UNUSED(bFrontBuffer);

    // DirectX 9 handles clipping automatically
    // No explicit clipper removal needed
    return S_OK;
}

int J3DAPI stdDisplay_IsFullscreen(void)
{
    return stdDisplay_bFullscreen;
}

static void stdDisplay_FlipBackBufferPixels(void)
{
    uint8_t* pPixelData    = stdDisplay_g_backBuffer.pPixels;
    int rowSize            = stdDisplay_g_backBuffer.rasterInfo.rowSize;
    unsigned char* tempRow = STDMALLOC(rowSize);
    int height             = (int)stdDisplay_g_backBuffer.rasterInfo.height;
    for ( int i = 0; i < height / 2; i++ )
    {
        int topRow    = i;
        int bottomRow = height - 1 - i;

        memcpy(tempRow, &pPixelData[topRow * rowSize], rowSize);
        memcpy(&pPixelData[topRow * rowSize], &pPixelData[bottomRow * rowSize], rowSize);
        memcpy(&pPixelData[bottomRow * rowSize], tempRow, rowSize);
    }
    STDFREE(tempRow);
}

int J3DAPI stdDisplay_LockBackBuffer(void** pSurface, uint32_t* pWidth, uint32_t* pHeight, int32_t* pPitch)
{
    if ( !stdDisplay_bOpen || !stdDisplay_bModeSet )
    {
        return 1;
    }

    glBindFramebuffer(GL_FRAMEBUFFER, stdDisplay_g_backBuffer.surface.fbo);
    glReadBuffer(GL_COLOR_ATTACHMENT0);

    *pWidth  = stdDisplay_g_backBuffer.rasterInfo.width;
    *pHeight = stdDisplay_g_backBuffer.rasterInfo.height;
    *pPitch  = stdDisplay_g_backBuffer.rasterInfo.rowSize;

    if ( stdDisplay_backLockRef++ > 0 )
    {
        *pSurface = stdDisplay_g_backBuffer.pPixels;
        return 0;
    }

    //free already available pixels
    if ( stdDisplay_g_backBuffer.pPixels )
    {
        STDFREE(stdDisplay_g_backBuffer.pPixels);
    }
    size_t numPixels  = *pWidth * *pHeight;
    size_t bufferSize = numPixels * 4; //RGBA8

    stdDisplay_g_backBuffer.pPixels = STDMALLOC(bufferSize);
    if ( !stdDisplay_g_backBuffer.pPixels )
    {
        STDLOG_ERROR("Couldn't allocate back buffer pixels.\n");
        stdDisplay_g_backBuffer.pPixels = NULL;
        return 1;
    }

    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glReadPixels(0, 0, *pWidth, *pHeight, GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV, stdDisplay_g_backBuffer.pPixels);

    stdDisplay_FlipBackBufferPixels();

    *pSurface = stdDisplay_g_backBuffer.pPixels;
    return 0;
}

void stdDisplay_UnlockBackBuffer(void)
{
    //upload written back buffer pixels to back buffer fbo texture
    tVSurface* surface = &stdDisplay_g_backBuffer.surface;
    if ( stdDisplay_bMSAAEnabled ) //drawing directly on multisampled framebuffer is not possible
    {
        glBindFramebuffer(GL_READ_FRAMEBUFFER, surface->fbo);
        surface->skipMSAA = true;
    }
    stdDisplay_FlipBackBufferPixels();
    stdShader_SetActiveTextureUnit(TU_SCENE);
    glBindTexture(GL_TEXTURE_2D, surface->colorTex);
    uint32_t height = stdDisplay_g_backBuffer.rasterInfo.height;
    uint32_t width  = stdDisplay_g_backBuffer.rasterInfo.width;
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height, GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV, stdDisplay_g_backBuffer.pPixels);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_A, GL_ONE);
    stdShader_SetActiveTextureUnit(TU_DEFAULT);
    stdDisplay_backLockRef--;
}

uint32_t J3DAPI stdDisplay_EncodeFromRGB565(uint16_t pixel)
{
    ColorInfo colorInfo;
    memcpy(&colorInfo, &stdDisplay_pCurVideoMode->rasterInfo.colorInfo, sizeof(colorInfo));

    uint8_t red = 8 * (pixel >> 11);
    if ( (red & 8) != 0 )
    {
        red |= 7;
    }

    uint8_t green = 4 * (pixel >> 5);
    if ( (green & 4) != 0 )
    {
        green |= 3;
    }

    uint8_t blue = 8 * pixel;
    if ( (blue & 8) != 0 )
    {
        blue = blue | 7;
    }

    return (red >> (colorInfo.redPosShiftRight & 0xFF) << (colorInfo.redPosShift & 0xFF))
        | (green >> (colorInfo.greenPosShiftRight & 0xFF) << (colorInfo.greenPosShift & 0xFF))
        | (blue >> (colorInfo.bluePosShiftRight & 0xFF) << (colorInfo.bluePosShift & 0xFF));
}

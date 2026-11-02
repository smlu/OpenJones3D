// Added: Null version of std/Win95/DX9/stdDisplayDX9.c (DirectDraw/Direct3D present).
// Buffers are real: a mode set on the one reported device allocates 640x480 32-bit pixel
// buffers for the front and back buffer, and tVBuffer creation mallocs the pixel buffer
// described by the rasterInfo (STDMALLOC), which Lock hands out and VBufferFree releases.
// There is no present: Update flips the buffers without touching a screen, SaveScreen
// reports the DX9 no-device answer, and the DC/clipper services hold no GDI resources.
#include <j3dcore/j3d.h>

#include <std/Win95/stdDisplay.h>
#include <std/Win95/stdWin95.h>

#include <std/General/std.h>
#include <std/General/stdColor.h>
#include <std/General/stdMemory.h>
#include <std/General/stdUtil.h>
#include <std/Null/stdPlatformNull.h>

#define STDDISPLAY_DEFAULT_WIDTH     640u
#define STDDISPLAY_DEFAULT_HEIGHT    480u
#define STDDISPLAY_MAXTEXWIDTH       4096u
#define STDDISPLAY_TOTALVIDEOMEMORY  (STDDISPLAY_MAXTEXWIDTH * STDDISPLAY_MAXTEXWIDTH * 4u) // DX9 caps-derived memory
#define STDDISPLAY_FREEVIDEOMEMORY   (STDDISPLAY_TOTALVIDEOMEMORY / 2u)
#define STDDISPLAY_DEVICE_NAME       "Jones3D Null Display"
#define STDDISPLAY_DRIVER_NAME       "stdDisplayNull"

tVBuffer stdDisplay_g_frontBuffer;
tVBuffer stdDisplay_g_backBuffer;

static bool stdDisplay_bStartup             = false;
static bool stdDisplay_bOpen                = false;
static bool stdDisplay_bModeSet             = false;
static bool stdDisplay_bFullscreen          = false;
static bool stdDisplay_bNoSync              = false;
static bool stdDisplay_bBackBufferWriteLock = false;

static size_t stdDisplay_numDevices    = 0;
static size_t stdDisplay_curDevice     = 0;
static size_t stdDisplay_numVideoModes = 0;

static StdDisplayDevice stdDisplay_aDisplayDevices[1] = { 0 };
static StdVideoMode stdDisplay_aVideoModes[1]         = { 0 };
static StdVideoMode stdDisplay_primaryVideoMode       = { 0 };
static StdVideoMode stdDisplay_fullscreenVideoMode    = { 0 };
static StdVideoMode* stdDisplay_pCurVideoMode         = NULL;

static uint32_t stdDisplay_backbufWidth  = 0;
static uint32_t stdDisplay_backbufHeight = 0;

static int stdDisplay_frontLockRef = 0;
static int stdDisplay_backLockRef  = 0;
static int stdDisplay_backDCRef    = 0;

// Added: Same pixel fillers as the DX9 SetPixels32 helper.
static void stdDisplay_SetPixels32(uint32_t* pPixels32, uint32_t pixel, size_t size)
{
    for ( size_t i = 0; i < size; ++i )
    {
        pPixels32[i] = pixel;
    }
}

// Added: The Null stand-in for stdDisplay_ColorFillSurface: fills the region of a RAM
// buffer with the tightly-packed row stride the DX9 lockable back buffer carries.
static void J3DAPI stdDisplay_ColorFillBytePixels(uint8_t* pPixels, const tRasterInfo* pRasterInfo, uint32_t fillColor, const StdRect* pRect)
{
    StdRect rect = { 0, 0, (int)pRasterInfo->width, (int)pRasterInfo->height };
    if ( pRect )
    {
        rect = *pRect;
    }

    if ( rect.left < 0 )
    {
        rect.left = 0;
    }
    if ( rect.top < 0 )
    {
        rect.top = 0;
    }
    if ( rect.right > (int)pRasterInfo->width )
    {
        rect.right = (int)pRasterInfo->width;
    }
    if ( rect.bottom > (int)pRasterInfo->height )
    {
        rect.bottom = (int)pRasterInfo->height;
    }

    for ( int y = rect.top; y < rect.bottom; ++y )
    {
        uint32_t* pRow = (uint32_t*)&pPixels[pRasterInfo->rowSize * y];
        stdDisplay_SetPixels32(&pRow[rect.left], fillColor, (size_t)(rect.right - rect.left));
    }
}

static int J3DAPI stdDisplay_VBufferFillPixels(tVBuffer* pVBuffer, uint32_t fillColor, const StdRect* pRect)
{
    STD_ASSERTREL(pVBuffer != NULL);
    if ( !pVBuffer->pPixels )
    {
        return 1;
    }

    stdDisplay_ColorFillBytePixels(pVBuffer->pPixels, &pVBuffer->rasterInfo, fillColor, pRect);
    return 0;
}

static void* J3DAPI stdDisplay_AllocBufferPixels(const tRasterInfo* pRasterInfo)
{
    size_t bytesPerPixel = pRasterInfo->colorInfo.bpp / 8u;
    size_t rowSize = (size_t)pRasterInfo->width * bytesPerPixel;
    size_t size = rowSize * pRasterInfo->height;
    if ( !size )
    {
        return NULL;
    }

    return STDMALLOC(size);
}

// Added: The presentation-buffer setup of DX9's InitBuffers: the front and back buffer are
// software RAM buffers sized from the current video mode.
static void stdDisplay_InitBuffers(void)
{
    tVBuffer* aBuffers[2] = { &stdDisplay_g_frontBuffer, &stdDisplay_g_backBuffer };
    for ( size_t i = 0; i < STD_ARRAYLEN(aBuffers); ++i )
    {
        tVBuffer* pBuffer = aBuffers[i];
        STD_ZEROMEM(pBuffer, sizeof(*pBuffer));
        pBuffer->type         = VBUFFER_SOFTWARE;
        pBuffer->bVideoMemory = 0;
        pBuffer->rasterInfo   = stdDisplay_pCurVideoMode->rasterInfo;
        pBuffer->pPixels      = (uint8_t*)stdDisplay_AllocBufferPixels(&pBuffer->rasterInfo);
        pBuffer->lockRefCount = pBuffer->pPixels ? 1 : 0;
    }

    stdDisplay_backbufWidth  = stdDisplay_g_backBuffer.rasterInfo.width;
    stdDisplay_backbufHeight = stdDisplay_g_backBuffer.rasterInfo.height;
}

static void stdDisplay_ReleaseBuffers(void)
{
    if ( stdDisplay_g_backBuffer.pPixels )
    {
        STDFREE(stdDisplay_g_backBuffer.pPixels);
    }
    if ( stdDisplay_g_frontBuffer.pPixels )
    {
        STDFREE(stdDisplay_g_frontBuffer.pPixels);
    }
    STD_ZEROMEM(&stdDisplay_g_backBuffer, sizeof(stdDisplay_g_backBuffer));
    STD_ZEROMEM(&stdDisplay_g_frontBuffer, sizeof(stdDisplay_g_frontBuffer));

    stdDisplay_frontLockRef = 0;
    stdDisplay_backLockRef  = 0;
    stdDisplay_backDCRef    = 0;
    stdDisplay_bBackBufferWriteLock = false;
    stdDisplay_backbufWidth  = 0;
    stdDisplay_backbufHeight = 0;
}

int stdDisplay_Startup(void)
{
    STDLOG_STATUS("Starting display system using the Null backend ...\n");
    if ( stdDisplay_bStartup )
    {
        return 1;
    }

    STD_ZEROMEM(&stdDisplay_g_frontBuffer, sizeof(stdDisplay_g_frontBuffer));
    STD_ZEROMEM(&stdDisplay_g_backBuffer, sizeof(stdDisplay_g_backBuffer));
    stdDisplay_bBackBufferWriteLock = false;
    stdDisplay_numDevices           = 0;

    // Added: One display device, filling every field of the DX9 EnumerateDevices that
    // engine code reads (names, HAL flag, memory and caps blob).
    StdDisplayDevice* pDevice = &stdDisplay_aDisplayDevices[0];
    stdUtil_StringCopy(pDevice->aDeviceName, STD_ARRAYLEN(pDevice->aDeviceName), STDDISPLAY_DEVICE_NAME);
    stdUtil_StringCopy(pDevice->aDriverName, STD_ARRAYLEN(pDevice->aDriverName), STDDISPLAY_DRIVER_NAME);
    pDevice->bHAL                      = 1;
    pDevice->bWindowRenderNotSupported = 0;
    pDevice->totalVideoMemory          = STDDISPLAY_TOTALVIDEOMEMORY;
    pDevice->freeVideoMemory           = STDDISPLAY_FREEVIDEOMEMORY;
    stdDisplay_numDevices              = 1;

    stdDisplay_bNoSync = false;

    // Same default 640x480 resolution of the DX9 Startup.
    stdDisplay_primaryVideoMode.rasterInfo.width  = STDDISPLAY_DEFAULT_WIDTH;
    stdDisplay_primaryVideoMode.rasterInfo.height = STDDISPLAY_DEFAULT_HEIGHT;
    // Added: The presentation buffers exist from Startup on, as 640x480 32-bit buffers.
    stdDisplay_pCurVideoMode = &stdDisplay_primaryVideoMode;
    {
        size_t bytesPerPixel = stdColor_cfRGB8888.bpp / 8u;
        stdDisplay_primaryVideoMode.rasterInfo.rowWidth = STDDISPLAY_DEFAULT_WIDTH;
        stdDisplay_primaryVideoMode.rasterInfo.rowSize = STDDISPLAY_DEFAULT_WIDTH * bytesPerPixel;
        stdDisplay_primaryVideoMode.rasterInfo.size = stdDisplay_primaryVideoMode.rasterInfo.rowSize * STDDISPLAY_DEFAULT_HEIGHT;
        stdDisplay_primaryVideoMode.rasterInfo.colorInfo = stdColor_cfRGB8888;
        float width  = (float)STDDISPLAY_DEFAULT_WIDTH;
        float height = (float)STDDISPLAY_DEFAULT_HEIGHT;
        stdDisplay_primaryVideoMode.aspectRatio = width / height;
    }
    stdDisplay_InitBuffers();
    stdDisplay_pCurVideoMode = NULL;

    stdDisplay_bStartup = true;
    STDLOG_STATUS("Found %zu Display Devices.\n", stdDisplay_numDevices);
    return 1;
}

void stdDisplay_Shutdown(void)
{
    if ( stdDisplay_bOpen )
    {
        stdDisplay_Close();
    }

    memset(stdDisplay_aDisplayDevices, 0, sizeof(stdDisplay_aDisplayDevices));
    stdDisplay_ReleaseBuffers();

    stdDisplay_pCurVideoMode = NULL;
    stdDisplay_numDevices    = 0;
    stdDisplay_numVideoModes = 0;
    stdDisplay_bStartup      = false;
}

int J3DAPI stdDisplay_Open(size_t deviceNum)
{
    STD_ASSERTREL(stdDisplay_bStartup);
    if ( stdDisplay_bOpen )
    {
        STDLOG_ERROR("Warning: System already open!\n");
        stdDisplay_Close();
    }

    if ( deviceNum >= stdDisplay_numDevices )
    {
        STDLOG_ERROR("Error: Invalid device num %zu (total: %zu)!\n", deviceNum, stdDisplay_numDevices);
        return 0;
    }

    stdDisplay_curDevice = deviceNum;

    // Added: The one advertised mode of the Null device, filled like EnumerateVideoModes
    // (one 640x480 32-bit mode with all DX9 fields the engine reads).
    StdVideoMode* pMode = &stdDisplay_aVideoModes[0];
    STD_ZEROMEM(pMode, sizeof(*pMode));
    pMode->refreshRate       = 60;
    pMode->rasterInfo.width  = STDDISPLAY_DEFAULT_WIDTH;
    pMode->rasterInfo.height = STDDISPLAY_DEFAULT_HEIGHT;
    {
        size_t bytesPerPixel = stdColor_cfRGB8888.bpp / 8u;
        pMode->rasterInfo.rowWidth = pMode->rasterInfo.width;
        pMode->rasterInfo.rowSize = pMode->rasterInfo.width * bytesPerPixel;
        pMode->rasterInfo.size = pMode->rasterInfo.rowSize * pMode->rasterInfo.height;
        pMode->rasterInfo.colorInfo = stdColor_cfRGB8888;
        float width  = (float)pMode->rasterInfo.width;
        float height = (float)pMode->rasterInfo.height;
        pMode->aspectRatio = width / height;
    }
    stdDisplay_numVideoModes = 1;

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

    stdDisplay_curDevice = 0;
    stdDisplay_bOpen     = false;
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

// Added: Same one-to-one adapter mapping answer as the DX9 backend.
size_t stdDisplay_GetAdapterNum(size_t deviceNum)
{
    return deviceNum;
}

tSysDisplayDevice* stdDisplay_GetSystemDevice(void)
{
    // Added: The DX9 version returns the built D3D device; there is none on this platform.
    return NULL;
}

int J3DAPI stdDisplay_CreateZBuffer(const tSysPixelFormat* pPixelFormat, int bSystemMemory)
{
    J3D_UNUSED(pPixelFormat);
    J3D_UNUSED(bSystemMemory);
    // Added: DX9 CreateZBuffer returns 0 on success; no z buffer is needed for Null draws.
    return 0;
}

void stdDisplay_DisableVSync(bool bDisable)
{
    stdDisplay_bNoSync = bDisable;
}

int J3DAPI stdDisplay_SetMode(size_t modeNum, int bFullscreen, size_t numBackBuffers)
{
    J3D_UNUSED(numBackBuffers); // Added: The Null platform has no swap chain to size.

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
        // Fixed: Preserve enumerated modes when the presentation layer adjusts them, like
        // the DX9 SetMode fix.
        stdDisplay_fullscreenVideoMode = stdDisplay_aVideoModes[modeNum];
        stdDisplay_pCurVideoMode       = &stdDisplay_fullscreenVideoMode;
    }
    else
    {
        stdDisplay_pCurVideoMode = &stdDisplay_primaryVideoMode;
        // Added: The primary mode stays packed like a freshly enumerated mode.
        size_t bytesPerPixel = stdDisplay_pCurVideoMode->rasterInfo.colorInfo.bpp ? stdDisplay_pCurVideoMode->rasterInfo.colorInfo.bpp / 8u : 4u;
        stdDisplay_pCurVideoMode->rasterInfo.rowWidth = stdDisplay_pCurVideoMode->rasterInfo.width;
        stdDisplay_pCurVideoMode->rasterInfo.rowSize = stdDisplay_pCurVideoMode->rasterInfo.width * bytesPerPixel;
        stdDisplay_pCurVideoMode->rasterInfo.size = stdDisplay_pCurVideoMode->rasterInfo.rowSize * stdDisplay_pCurVideoMode->rasterInfo.height;
    }

    stdDisplay_InitBuffers();
    stdDisplay_bModeSet    = true;
    stdDisplay_bFullscreen = bFullscreen != 0;

    stdDisplay_VBufferFillPixels(&stdDisplay_g_backBuffer, 0, NULL);
    stdDisplay_Update();

    if ( bFullscreen )
    {
        stdDisplay_VBufferFillPixels(&stdDisplay_g_backBuffer, 0, NULL);
    }

    return 0;
}

void stdDisplay_ClearMode(void)
{
    if ( stdDisplay_bModeSet )
    {
        stdDisplay_ReleaseBuffers();
    }

    stdDisplay_pCurVideoMode        = NULL;
    stdDisplay_bModeSet             = false;
    stdDisplay_bFullscreen          = false;
    stdDisplay_bBackBufferWriteLock = false;
}

void stdDisplay_RegisterDevicePreResetCallback(tDisplayDevicePreResetCallback pCallback)
{
    J3D_UNUSED(pCallback);
}

void stdDisplay_RegisterDevicePostResetCallback(tDisplayDevicePostResetCallback pCallback)
{
    J3D_UNUSED(pCallback);
}

void stdDisplay_RegisterDeviceReleaseCallback(tDisplayDeviceReleaseCallback pCallback)
{
    J3D_UNUSED(pCallback);
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

size_t stdDisplay_GetNumVideoModes(void)
{
    return stdDisplay_numVideoModes;
}

void J3DAPI stdDisplay_Refresh(int bReload)
{
    J3D_UNUSED(bReload);
}

tVBuffer* J3DAPI stdDisplay_VBufferNew(const tRasterInfo* pRasterInfo, int bUseVSurface, int bUseVideoMemory)
{
    J3D_UNUSED(bUseVSurface);
    J3D_UNUSED(bUseVideoMemory);

    // Added: the assert is compiled out in release builds, so log the offending format first.
    if ( (pRasterInfo->colorInfo.bpp % 8) != 0 )
    {
        STDLOG_ERROR("stdDisplay_VBufferNew: Invalid bpp %u for %u x %u buffer.\n",
            pRasterInfo->colorInfo.bpp,
            pRasterInfo->width,
            pRasterInfo->height
        );
    }

    STD_ASSERTREL((pRasterInfo->colorInfo.bpp % 8) == 0);

    // Added: The bUseVSurface branch creates a hardware surface on Windows; the Null
    // platform has none, so every buffer takes the DX9 software path (app memory).
    tVBuffer* vbuffer = (tVBuffer*)STDMALLOC(sizeof(tVBuffer));
    if ( !vbuffer )
    {
        STDLOG_ERROR("Error allocating vbuffer.\n");
        return NULL;
    }

    STD_ZEROMEM(vbuffer, sizeof(*vbuffer));
    vbuffer->rasterInfo = *pRasterInfo;

    uint32_t bpp = (uint32_t)vbuffer->rasterInfo.colorInfo.bpp / 8;
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

    if ( pVBuffer->type == VBUFFER_SOFTWARE )
    {
        if ( pVBuffer->pPixels )
        {
            stdMemory_Free(pVBuffer->pPixels);
            pVBuffer->pPixels = NULL;
        }
    }

    stdMemory_Free(pVBuffer);
}

int J3DAPI stdDisplay_VBufferLock(tVBuffer* pVBuffer)
{
    STD_ASSERTREL(pVBuffer != NULL);

    if ( pVBuffer->type == VBUFFER_SOFTWARE )
    {
        // Added: Same invariant as the DX9 software branch.
        STD_ASSERT(pVBuffer->pPixels);
        ++pVBuffer->lockRefCount;
        return 1;
    }

    return 0;
}

int stdDisplay_VBufferLockReadOnly(tVBuffer* pVBuffer)
{
    return stdDisplay_VBufferLock(pVBuffer);
}

int J3DAPI stdDisplay_VBufferUnlock(tVBuffer* pVBuffer)
{
    STD_ASSERTREL(pVBuffer != NULL);

    if ( pVBuffer->type == VBUFFER_SOFTWARE )
    {
        if ( pVBuffer->lockRefCount )
        {
            --pVBuffer->lockRefCount;
        }
        return 1;
    }

    if ( pVBuffer->type != VBUFFER_HARDWARE )
    {
        return 1;
    }

    return 0;
}

int J3DAPI stdDisplay_VBufferFill(tVBuffer* pVBuffer, uint32_t dwFillColor, const StdRect* pRect)
{
    return stdDisplay_VBufferFillPixels(pVBuffer, dwFillColor, pRect);
}

tVBuffer* J3DAPI stdDisplay_VBufferConvertColorFormat(const ColorInfo* pDesiredColorFormat, tVBuffer* pSrc, int bColorKey, LPDDCOLORKEY pColorKey)
{
    STD_ASSERTREL(pSrc != NULL);

    if ( memcmp(pDesiredColorFormat, &pSrc->rasterInfo.colorInfo, sizeof(ColorInfo)) == 0 )
    {
        return pSrc;
    }

    // Added: Same palette guard as the DX9 ConvertColorFormat.
    if ( pSrc->rasterInfo.colorInfo.colorMode == STDCOLOR_PAL )
    {
        STD_ASSERTREL(pDesiredColorFormat->colorMode != STDCOLOR_PAL);
        return pSrc;
    }

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

    // Convert pixel data, exactly like the DX9 ConvertColorFormat loop.
    STD_ASSERTREL(pSrc->pPixels != NULL);
    STD_ASSERTREL(pDest->pPixels != NULL);

    stdDisplay_VBufferLock(pSrc);
    stdDisplay_VBufferLock(pDest);

    const uint8_t* pSrcRow = NULL;
    uint8_t* pDestRow = NULL;
    for ( size_t row = 0; row < pDest->rasterInfo.height; ++row )
    {
        pSrcRow = &pSrc->pPixels[pSrc->rasterInfo.rowSize * row];
        pDestRow = &pDest->pPixels[pDest->rasterInfo.rowSize * row];

        stdColor_ColorConvertOneRow(
            pDestRow,
            pDesiredColorFormat,
            pSrcRow,
            &pSrc->rasterInfo.colorInfo,
            (int)pDest->rasterInfo.width,
            bColorKey,
            pColorKey
        );
    }

    stdDisplay_VBufferUnlock(pSrc);
    stdDisplay_VBufferUnlock(pDest);

    memcpy(&pDest->rasterInfo.colorInfo, pDesiredColorFormat, sizeof(pDest->rasterInfo.colorInfo));

    if ( pDest != pSrc )
    {
        stdDisplay_VBufferFree(pSrc);
    }

    return pDest;
}

int J3DAPI stdDisplay_GetTextureMemory(size_t* pTotal, size_t* pFree)
{
    if ( pTotal )
    {
        *pTotal = STDDISPLAY_TOTALVIDEOMEMORY;
    }
    if ( pFree )
    {
        *pFree = STDDISPLAY_FREEVIDEOMEMORY;
    }
    return 0;
}

int J3DAPI stdDisplay_GetTotalMemory(size_t* pTotal, size_t* pFree)
{
    if ( pTotal )
    {
        *pTotal = STDDISPLAY_TOTALVIDEOMEMORY;
    }
    if ( pFree )
    {
        *pFree = STDDISPLAY_FREEVIDEOMEMORY;
    }
    return 0;
}

void J3DAPI stdDisplay_GetBackBufferSize(uint32_t* pWidth, uint32_t* pHeight)
{
    *pWidth  = stdDisplay_g_backBuffer.rasterInfo.width;
    *pHeight = stdDisplay_g_backBuffer.rasterInfo.height;
}

int J3DAPI stdDisplay_BackBufferFill(uint32_t color, const StdRect* lpRect)
{
    if ( !stdDisplay_g_backBuffer.pPixels )
    {
        return 1;
    }

    stdDisplay_ColorFillBytePixels(stdDisplay_g_backBuffer.pPixels, &stdDisplay_g_backBuffer.rasterInfo, color, lpRect);
    return 0;
}

int J3DAPI stdDisplay_SaveScreen(const char* pFilename)
{
    J3D_UNUSED(pFilename);
    // Added: Same answer as the DX9 SaveScreen without device/presentation surface (1);
    // the stdBmp screenshot writer is a Windows backend and is not linked here.
    return 1;
}

void J3DAPI stdDisplay_SetDefaultResolution(uint32_t width, uint32_t height)
{
    stdDisplay_primaryVideoMode.rasterInfo.width  = width;
    stdDisplay_primaryVideoMode.rasterInfo.height = height;
}

tSysDrawContext stdDisplay_GetFrontBufferDC(void)
{
    if ( !stdDisplay_bOpen || !stdDisplay_bModeSet )
    {
        return NULL;
    }

    ++stdDisplay_frontLockRef;
    // Added: No GDI DC exists on this platform; the ref counting stays consistent for
    // ReleaseFrontBufferDC and every consumer gets a NULL context.
    return NULL;
}

void J3DAPI stdDisplay_ReleaseFrontBufferDC(tSysDrawContext hdc)
{
    J3D_UNUSED(hdc);
    if ( stdDisplay_frontLockRef )
    {
        --stdDisplay_frontLockRef;
    }
}

tSysDrawContext stdDisplay_GetBackBufferDC(void)
{
    if ( !stdDisplay_bOpen || !stdDisplay_bModeSet )
    {
        return NULL;
    }

    ++stdDisplay_backDCRef;
    return NULL;
}

void J3DAPI stdDisplay_ReleaseBackBufferDC(tSysDrawContext hdc)
{
    J3D_UNUSED(hdc);
    if ( stdDisplay_backDCRef )
    {
        --stdDisplay_backDCRef;
    }
}

int stdDisplay_FlipToGDISurface(void)
{
    return 0;
}

int stdDisplay_CanRenderWindowed(void)
{
    return 0;
}

int stdDisplay_IsFullscreen(void)
{
    return stdDisplay_bFullscreen;
}

// Added: Same 0-on-success clipper answer as the DX9 calls.
int J3DAPI stdDisplay_SetBufferClipper(int bFrontBuffer)
{
    J3D_UNUSED(bFrontBuffer);
    return 0;
}

int J3DAPI stdDisplay_RemoveBufferClipper(int bFrontBuffer)
{
    J3D_UNUSED(bFrontBuffer);
    return 0;
}

// Added: Same lock state walk as the DX9 LockBackBufferInternal write path: the back
// buffer pixels live in RAM with the tightly-packed pitch.
int J3DAPI stdDisplay_LockBackBuffer(void** pSurface, uint32_t* pWidth, uint32_t* pHeight, int32_t* pPitch)
{
    if ( !stdDisplay_bOpen || !stdDisplay_bModeSet || !stdDisplay_g_backBuffer.pPixels )
    {
        return 1;
    }

    ++stdDisplay_backLockRef;
    stdDisplay_bBackBufferWriteLock = true;
    *pWidth   = stdDisplay_g_backBuffer.rasterInfo.width;
    *pHeight  = stdDisplay_g_backBuffer.rasterInfo.height;
    *pPitch   = (int32_t)stdDisplay_g_backBuffer.rasterInfo.rowSize;
    *pSurface = stdDisplay_g_backBuffer.pPixels;
    return 0;
}

int stdDisplay_LockBackBufferReadOnly(void** pSurface, uint32_t* pWidth, uint32_t* pHeight, int32_t* pPitch)
{
    if ( !stdDisplay_bOpen || !stdDisplay_bModeSet || !stdDisplay_g_backBuffer.pPixels )
    {
        return 1;
    }

    ++stdDisplay_backLockRef;
    *pWidth   = stdDisplay_g_backBuffer.rasterInfo.width;
    *pHeight  = stdDisplay_g_backBuffer.rasterInfo.height;
    *pPitch   = (int32_t)stdDisplay_g_backBuffer.rasterInfo.rowSize;
    *pSurface = stdDisplay_g_backBuffer.pPixels;
    return 0;
}

void stdDisplay_UnlockBackBuffer(void)
{
    if ( stdDisplay_bModeSet && stdDisplay_backLockRef == 1 )
    {
        stdDisplay_bBackBufferWriteLock = false;
    }

    if ( stdDisplay_backLockRef )
    {
        --stdDisplay_backLockRef;
    }
}

// Added: Exact copy of the DX9 EncodeFromRGB565 channel expansion for the current mode
// (the color key pattern never bleeds between channels).
uint32_t J3DAPI stdDisplay_EncodeFromRGB565(uint16_t pixel)
{
    ColorInfo colorInfo;
    if ( stdDisplay_pCurVideoMode )
    {
        colorInfo = stdDisplay_pCurVideoMode->rasterInfo.colorInfo;
    }
    else
    {
        STD_ZEROMEM(&colorInfo, sizeof(colorInfo));
    }

    uint8_t red = (uint8_t)(((pixel >> 11) & 0x1F) << 3);
    red |= (uint8_t)(red >> 5);

    uint8_t green = (uint8_t)(((pixel >> 5) & 0x3F) << 2);
    green |= (uint8_t)(green >> 6);

    uint8_t blue = (uint8_t)((pixel & 0x1F) << 3);
    blue |= (uint8_t)(blue >> 5);

    return (red >> (colorInfo.redPosShiftRight & 0xFF) << (colorInfo.redPosShift & 0xFF))
        | (green >> (colorInfo.greenPosShiftRight & 0xFF) << (colorInfo.greenPosShift & 0xFF))
        | (blue >> (colorInfo.bluePosShiftRight & 0xFF) << (colorInfo.bluePosShift & 0xFF));
}

int stdDisplay_Update(void)
{
    // Added: a present takes 1 ms of virtual time, like waiting for vsync, so loops that wait on the clock around it finish. No-op on the real clock.
    stdPlatformNull_AdvanceClock(1);

    // Added: No presentation surface: the swap is a no-op and 0 stays the success answer.
    if ( !stdDisplay_bModeSet )
    {
        return 1;
    }

    return 0;
}

void stdDisplay_InstallHooks(void)
{
    // Added: No RTI hooks in the standalone Null build (J3D_STANDALONE): nothing to hook.
}

void stdDisplay_ResetGlobals(void)
{
    STD_ZEROMEM(&stdDisplay_g_frontBuffer, sizeof(stdDisplay_g_frontBuffer));
    STD_ZEROMEM(&stdDisplay_g_backBuffer, sizeof(stdDisplay_g_backBuffer));
    stdDisplay_bBackBufferWriteLock = false;
}

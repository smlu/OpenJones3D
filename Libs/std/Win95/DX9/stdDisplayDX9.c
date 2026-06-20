#include <std/Win95/std3D.h>
#include <std/Win95/stdDisplay.h>
#include <std/Win95/stdWin95.h>

#include <j3dcore/j3dhook.h>
#include <std/General/std.h>
#include <std/General/stdBmp.h>
#include <std/General/stdConfig.h>
#include <std/General/stdColor.h>
#include <std/General/stdMemory.h>
#include <std/General/stdUtil.h>
#include <std/RTI/symbols.h>

#define STDDISPLAY_MINFRAMERATE 30
#define STDDISPLAY_MAXFRAMERATE 256

// Public globals
tVBuffer stdDisplay_g_frontBuffer = { 0 };
tVBuffer stdDisplay_g_backBuffer  = { 0 };

// Private globals
static bool stdDisplay_bStartup    = false;
static bool stdDisplay_bOpen       = false;
static bool stdDisplay_bModeSet    = false;
static bool stdDisplay_bFullscreen = false;
static bool stdDisplay_bNoSync     = false;
static bool stdDisplay_bDeviceLost = false;
static bool stdDisplay_bDeviceResourcesInvalid = false;
static bool stdDisplay_bPresentationBufferDirty = false;

static LPDIRECT3D9 stdDisplay_pD3D9;

// Current D3D device created in SetMode
static D3DPRESENT_PARAMETERS stdDisplay_presentParams;
static D3DCAPS9 stdDisplay_deviceCaps;
static LPDIRECT3DDEVICE9 stdDisplay_pD3DDevice;

// Back buffer local vars
static int stdDisplay_backbufWidth                 = 0;
static int stdDisplay_backbufHeight                = 0;
static size_t stdDisplay_backLockRef               = 0;
static size_t stdDisplay_backDCRef                 = 0;
static bool stdDisplay_bBackBufferWriteLock        = false;
static HDC stdDisplay_hdcBack                      = NULL;
static D3DLOCKED_RECT stdDisplay_backLockedRect    = { 0 };
static LPDIRECT3DSURFACE9 stdDisplay_pBackLockSurf = NULL; // temp lockable backbuffer surface when MSAA is enabled

// Front buffer local vars
static size_t stdDisplay_frontLockRef               = 0;
static HDC stdDisplay_hdcFront                      = NULL;
static LPDIRECT3DSURFACE9 stdDisplay_pFrontLockSurf = NULL; // temp lockable frontbuffer surface when MSAA is enabled

// Z buffer local vars
static tVSurface stdDisplay_zBuffer;

static const D3DFORMAT stdDisplay_aSupportedFormats[] = { D3DFMT_R5G6B5, D3DFMT_X8R8G8B8 };
static const D3DFORMAT stdDisplay_aDepthFormats[] = { D3DFMT_D24S8, D3DFMT_D24X4S4, D3DFMT_D24X8, D3DFMT_D32, D3DFMT_D15S1, D3DFMT_D16 };
static StdVideoMode* stdDisplay_pCurVideoMode   = NULL;
static StdVideoMode stdDisplay_primaryVideoMode = { 0 };

static size_t stdDisplay_numVideoModes          = 0;
static StdVideoMode stdDisplay_aVideoModes[512] = { 0 };

static size_t stdDisplay_curDevice;
static UINT stdDisplay_curAdapter = D3DADAPTER_DEFAULT;
static StdDisplayDevice* stdDisplay_pCurDevice = NULL;

static size_t stdDisplay_numDevices = 0;
static StdDisplayDevice stdDisplay_aDisplayDevices[16] = { 0 };
static UINT stdDisplay_aAdapterNums[STD_ARRAYLEN(stdDisplay_aDisplayDevices)] = { 0 };

typedef struct sStdDisplayTrackedVBuffer
{
    tVBuffer* pVBuffer;
    struct sStdDisplayTrackedVBuffer* pNext;
} StdDisplayTrackedVBuffer;

static StdDisplayTrackedVBuffer* stdDisplay_pTrackedVBuffers = NULL;

static HFONT stdDisplay_hFont;

static int stdDisplay_dword_5D73D8;
static int stdDisplay_dword_5D73DC;

// Callbacks
static tDisplayDevicePreResetCallback stdDisplay_pfDevicePreResetCallback   = NULL;
static tDisplayDevicePostResetCallback stdDisplay_pfDevicePostResetCallback = NULL;
static tDisplayDeviceReleaseCallback stdDisplay_pfDeviceReleaseCallback     = NULL;

// MSAA vars
static bool stdDisplay_bMSAAEnabled       = false;
static DWORD stdDisplay_msaaSampleQuality = 0;
static int stdDisplay_msaaSampleCount     = 0;
static D3DMULTISAMPLE_TYPE stdDisplay_msaaSampleType = D3DMULTISAMPLE_NONE;

// DirectX 9 status table - simplified version of common errors
static const DXStatus stdDisplay_aD3DStatusTbl[] =
{
    { D3D_OK,                                      "D3D_OK"                                    },
    { D3DERR_DEVICELOST,                           "D3DERR_DEVICELOST"                         },
    { D3DERR_DEVICENOTRESET,                       "D3DERR_DEVICENOTRESET"                     },
    { D3DERR_NOTAVAILABLE,                         "D3DERR_NOTAVAILABLE"                       },
    { D3DERR_OUTOFVIDEOMEMORY,                     "D3DERR_OUTOFVIDEOMEMORY"                   },
    { D3DERR_INVALIDDEVICE,                        "D3DERR_INVALIDDEVICE"                      },
    { D3DERR_INVALIDCALL,                          "D3DERR_INVALIDCALL"                        },
    { D3DERR_DRIVERINVALIDCALL,                    "D3DERR_DRIVERINVALIDCALL"                  },
    { D3DERR_WASSTILLDRAWING,                      "D3DERR_WASSTILLDRAWING"                    },
    { E_OUTOFMEMORY,                               "E_OUTOFMEMORY"                             },
    { E_INVALIDARG,                                "E_INVALIDARG"                              },
    { E_FAIL,                                      "E_FAIL"                                    }
};

// Helper functions
static int J3DAPI stdDisplay_VideoModeCompare(const StdVideoMode* pMode1, const StdVideoMode* pMode2);
static const char* J3DAPI stdDisplay_D3DGetStatus(HRESULT status);

static int J3DAPI stdDisplay_InitDirect3D9(HWND hwnd);
static int stdDisplay_EnumerateDevices(void);
static int J3DAPI stdDisplay_EnumerateVideoModes(UINT adapter);
static D3DFORMAT J3DAPI stdDisplay_GetD3DFormat(int bpp);
static int J3DAPI stdDisplay_BppFromD3DFormat(D3DFORMAT format);
static bool stdDisplay_GetVideoColorFormat(D3DFORMAT format, ColorInfo* pFormat);
static bool stdDisplay_FindDepthFormat(UINT adapter, D3DFORMAT adapterFormat, D3DFORMAT renderTargetFormat, D3DFORMAT* pDepthFormat);
static bool stdDisplay_CreateHardwareVBufferSurface(tVBuffer* pVBuffer);
static bool stdDisplay_TrackHardwareVBuffer(tVBuffer* pVBuffer);
static void stdDisplay_UntrackHardwareVBuffer(tVBuffer* pVBuffer);
static bool stdDisplay_CanResetDevice(void);
static void stdDisplay_ReleaseTrackedVBufferSurfaces(bool bReleaseAll, bool bReleaseOnly);
static bool stdDisplay_RestoreTrackedVBufferSurfaces(void);

static inline void J3DAPI stdDisplay_SetAspectRatio(StdVideoMode* pMode);
static inline int J3DAPI stdDisplay_SetWindowMode(HWND hWnd, StdVideoMode* pDisplayMode);
static inline int J3DAPI stdDisplay_SetFullscreenMode(HWND hwnd, const StdVideoMode* pDisplayMode, size_t numBackBuffers);
static int J3DAPI stdDisplay_InitBuffers(PDIRECT3DDEVICE9 pDevice, StdVideoMode* pDisplayMode, bool bWindowMode, size_t numBuffers);

static void stdDisplay_ReleaseBuffers(void);
static void stdDisplay_ReleaseBuffersInternal(bool bReleaseOnly);
static inline uint8_t* J3DAPI stdDisplay_LockSurface(tVSurface* pVSurf);
static inline int J3DAPI stdDisplay_UnlockSurface(tVSurface* pSurf);
static int J3DAPI stdDisplay_ColorFillSurface(tVSurface* pSurf, uint32_t dwFillColor, const StdRect* lpRect);
static int stdDisplay_CheckDeviceState(void);
static void stdDisplay_ReleaseDevice(void);
static int stdDisplay_ResetDevice(void);
static int stdDisplay_ResolveBackBuffer(void);
static int stdDisplay_LockBackBufferInternal(void** pSurface, uint32_t* pWidth, uint32_t* pHeight, int32_t* pPitch, bool bWrite);
static DWORD stdDisplay_GetDeviceCreateFlags(void);

// Color format conversion helpers
static void J3DAPI stdDisplay_SetPixels16(uint16_t* pPixels16, uint16_t pixel, size_t size)
{
    if ( (size & 1) != 0 )
    {
        for ( size_t i = 0; i < size; ++i )
        {
            pPixels16[i] = pixel;
        }
    }
    else
    {
        uint32_t dword_pixel = ((uint32_t)pixel << 16) | pixel;
        uint32_t* pPixels32 = (uint32_t*)pPixels16;
        for ( size_t i = 0; i < size / 2; ++i )
        {
            pPixels32[i] = dword_pixel;
        }
    }
}

static void J3DAPI stdDisplay_SetPixels32(uint32_t* pPixels32, uint32_t pixel, size_t size)
{
    for ( size_t i = 0; i < size; ++i )
    {
        pPixels32[i] = pixel;
    }
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
    J3D_HOOKFUNC(stdDisplay_VBufferFill);
    J3D_HOOKFUNC(stdDisplay_VBufferConvertColorFormat);
    J3D_HOOKFUNC(stdDisplay_VideoModeCompare);
    J3D_HOOKFUNC(stdDisplay_GetTextureMemory);
    J3D_HOOKFUNC(stdDisplay_GetTotalMemory);
    J3D_HOOKFUNC(stdDisplay_CreateZBuffer);
    J3D_HOOKFUNC(stdDisplay_SetAspectRatio);
    J3D_HOOKFUNC(stdDisplay_SetWindowMode);
    J3D_HOOKFUNC(stdDisplay_SetFullscreenMode);
    J3D_HOOKFUNC(stdDisplay_ReleaseBuffers);
    J3D_HOOKFUNC(stdDisplay_LockSurface);
    J3D_HOOKFUNC(stdDisplay_UnlockSurface);
    J3D_HOOKFUNC(stdDisplay_Update);
    J3D_HOOKFUNC(stdDisplay_ColorFillSurface);
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
    memset(&stdDisplay_g_frontBuffer, 0, sizeof(stdDisplay_g_frontBuffer));
    memset(&stdDisplay_g_backBuffer, 0, sizeof(stdDisplay_g_backBuffer));
    stdDisplay_bDeviceLost              = false;
    stdDisplay_bDeviceResourcesInvalid  = false;
    stdDisplay_bBackBufferWriteLock     = false;
    stdDisplay_bPresentationBufferDirty = false;
}


static bool stdDisplay_FindDepthFormat(UINT adapter, D3DFORMAT adapterFormat, D3DFORMAT renderTargetFormat, D3DFORMAT* pDepthFormat)
{
    if ( !stdDisplay_pD3D9 || !pDepthFormat )
    {
        return false;
    }

    for ( size_t i = 0u; i < STD_ARRAYLEN(stdDisplay_aDepthFormats); ++i )
    {
        D3DFORMAT depthFormat = stdDisplay_aDepthFormats[i];
        HRESULT hr = IDirect3D9_CheckDeviceFormat(
            stdDisplay_pD3D9,
            adapter,
            D3DDEVTYPE_HAL,
            adapterFormat,
            D3DUSAGE_DEPTHSTENCIL,
            D3DRTYPE_SURFACE,
            depthFormat
        );
        if ( FAILED(hr) )
        {
            continue;
        }

        hr = IDirect3D9_CheckDepthStencilMatch(
            stdDisplay_pD3D9,
            adapter,
            D3DDEVTYPE_HAL,
            adapterFormat,
            renderTargetFormat,
            depthFormat
        );
        if ( SUCCEEDED(hr) )
        {
            *pDepthFormat = depthFormat;
            return true;
        }
    }

    return false;
}

static bool J3DAPI stdDisplay_CheckMSAASupport(UINT adapter, D3DFORMAT adapterFormat, D3DFORMAT colorFormat, D3DFORMAT depthFormat, BOOL windowed, D3DMULTISAMPLE_TYPE sampleType, DWORD* pQualityLevels)
{
    if ( !stdDisplay_pD3D9 )
    {
        return false;
    }

    if ( FAILED(IDirect3D9_CheckDepthStencilMatch(
        stdDisplay_pD3D9,
        adapter,
        D3DDEVTYPE_HAL,
        adapterFormat,
        colorFormat,
        depthFormat
    )) )
    {
        return false;
    }

    DWORD colorQualityLevels = 0;
    HRESULT hr = IDirect3D9_CheckDeviceMultiSampleType(
        stdDisplay_pD3D9,
        adapter,
        D3DDEVTYPE_HAL,
        colorFormat,
        windowed,
        sampleType,
        &colorQualityLevels
    );
    if ( FAILED(hr) )
    {
        return false;
    }

    DWORD depthQualityLevels = 0;
    hr = IDirect3D9_CheckDeviceMultiSampleType(
        stdDisplay_pD3D9,
        adapter,
        D3DDEVTYPE_HAL,
        depthFormat,
        windowed,
        sampleType,
        &depthQualityLevels
    );
    if ( FAILED(hr) )
    {
        return false;
    }

    DWORD qualityLevels = colorQualityLevels < depthQualityLevels ? colorQualityLevels : depthQualityLevels;
    if ( pQualityLevels )
    {
        *pQualityLevels = qualityLevels;
    }

    return qualityLevels > 0;
}

static void stdDisplay_InitMSAASettings(void)
{
    // Read MSAA settings from registry/config
    stdDisplay_bMSAAEnabled    = stdConfig_GetBool(STD3D_CFG_MSAAENABLED, true);
    stdDisplay_msaaSampleCount = stdConfig_GetInt(STD3D_CFG_MSAASAMPLES, 16);

    // Clamp sample count to valid values
    if ( stdDisplay_msaaSampleCount < 2 )
    {
        stdDisplay_msaaSampleCount = 2;
    }
    else if ( stdDisplay_msaaSampleCount > 16 )
    {
        stdDisplay_msaaSampleCount = 16;
    }

    // Convert sample count to D3D multisample type
    switch ( stdDisplay_msaaSampleCount )
    {
        case 2:  stdDisplay_msaaSampleType = D3DMULTISAMPLE_2_SAMPLES; break;
        case 4:  stdDisplay_msaaSampleType = D3DMULTISAMPLE_4_SAMPLES; break;
        case 8:  stdDisplay_msaaSampleType = D3DMULTISAMPLE_8_SAMPLES; break;
        case 16: stdDisplay_msaaSampleType = D3DMULTISAMPLE_16_SAMPLES; break;
        default:
            stdDisplay_msaaSampleType  = D3DMULTISAMPLE_4_SAMPLES;
            stdDisplay_msaaSampleCount = 4;
            break;
    }

    if ( !stdDisplay_bMSAAEnabled )
    {
        stdDisplay_msaaSampleType    = D3DMULTISAMPLE_NONE;
        stdDisplay_msaaSampleQuality = 0;
    }

    STDLOG_DEBUG("MSAA Settings: Enabled=%d, Samples=%d\n", stdDisplay_bMSAAEnabled, stdDisplay_msaaSampleCount);
}

static void stdDisplay_ValidateMSAASettings(UINT adapter, D3DFORMAT adapterFormat, D3DFORMAT colorFormat, D3DFORMAT depthFormat, BOOL windowed)
{
    if ( !stdDisplay_bMSAAEnabled )
    {
        stdDisplay_msaaSampleType    = D3DMULTISAMPLE_NONE;
        stdDisplay_msaaSampleQuality = 0;
        return;
    }

    DWORD qualityLevels = 0;

    // Check if requested MSAA level is supported
    if ( stdDisplay_CheckMSAASupport(adapter, adapterFormat, colorFormat, depthFormat, windowed, stdDisplay_msaaSampleType, &qualityLevels) )
    {
        stdDisplay_msaaSampleQuality = qualityLevels > 0 ? qualityLevels - 1 : 0;
        STDLOG_DEBUG("MSAA %dx supported with %d quality levels\n", stdDisplay_msaaSampleCount, qualityLevels);
    }
    else
    {
        // Fall back to lower MSAA levels
        D3DMULTISAMPLE_TYPE fallbackTypes[] = {
            D3DMULTISAMPLE_8_SAMPLES,
            D3DMULTISAMPLE_4_SAMPLES,
            D3DMULTISAMPLE_2_SAMPLES
        };
        int fallbackCounts[] = { 8, 4, 2 };

        bool found = false;
        for ( int i = 0; i < 3; i++ )
        {
            // A fallback must use fewer samples than the unsupported requested mode.
            if ( fallbackCounts[i] >= stdDisplay_msaaSampleCount )
            {
                continue;
            }

            if ( stdDisplay_CheckMSAASupport(adapter, adapterFormat, colorFormat, depthFormat, windowed, fallbackTypes[i], &qualityLevels) )
            {
                stdDisplay_msaaSampleType    = fallbackTypes[i];
                stdDisplay_msaaSampleCount   = fallbackCounts[i];
                stdDisplay_msaaSampleQuality = qualityLevels > 0 ? qualityLevels - 1 : 0;
                found = true;
                STDLOG_STATUS("MSAA fallback: Using %dx with %d quality levels\n", stdDisplay_msaaSampleCount, qualityLevels);
                stdConfig_SetInt(STD3D_CFG_MSAASAMPLES, stdDisplay_msaaSampleCount);
                break;
            }
        }

        if ( !found )
        {
            STDLOG_WARNING("MSAA not supported, disabling\n");
            stdDisplay_bMSAAEnabled      = false;
            stdDisplay_msaaSampleType    = D3DMULTISAMPLE_NONE;
            stdDisplay_msaaSampleQuality = 0;
        }
    }
}

int stdDisplay_Startup(void)
{
    STDLOG_STATUS("Starting display system using DirectX 9 GAPI ...\n");
    if ( stdDisplay_bStartup )
    {
        return 1;
    }

    memset(&stdDisplay_g_frontBuffer, 0, sizeof(stdDisplay_g_frontBuffer));
    memset(&stdDisplay_g_backBuffer, 0, sizeof(stdDisplay_g_backBuffer));
    memset(&stdDisplay_zBuffer, 0, sizeof(stdDisplay_zBuffer));
    memset(stdDisplay_aAdapterNums, 0, sizeof(stdDisplay_aAdapterNums));

    stdDisplay_bDeviceLost              = false;
    stdDisplay_bDeviceResourcesInvalid  = false;
    stdDisplay_bBackBufferWriteLock     = false;
    stdDisplay_bPresentationBufferDirty = false;
    stdDisplay_numDevices               = 0;

    // Create Direct3D9 object
    stdDisplay_pD3D9 = Direct3DCreate9(D3D_SDK_VERSION);
    if ( !stdDisplay_pD3D9 )
    {
        STDLOG_ERROR("Failed to create Direct3D9 object.\n");
        return 0;
    }

    // Initialize MSAA settings from config
    stdDisplay_InitMSAASettings();

    // Enumerate devices and display modes
    if ( !stdDisplay_EnumerateDevices() )
    {
        IDirect3D9_Release(stdDisplay_pD3D9);
        stdDisplay_pD3D9 = NULL;
        return 0;
    }

    // Set default resolution
    stdDisplay_primaryVideoMode.rasterInfo.width  = 640;
    stdDisplay_primaryVideoMode.rasterInfo.height = 480;

    stdDisplay_bStartup = true;
    STDLOG_STATUS("Found %d Display Devices.\n", stdDisplay_numDevices);
    return 1;
}

void stdDisplay_Shutdown(void)
{
    if ( stdDisplay_bOpen )
    {
        stdDisplay_Close();
    }

    if ( stdDisplay_pD3D9 )
    {
        IDirect3D9_Release(stdDisplay_pD3D9);
        stdDisplay_pD3D9 = NULL;
    }

    memset(stdDisplay_aDisplayDevices, 0, sizeof(stdDisplay_aDisplayDevices));
    memset(stdDisplay_aAdapterNums, 0, sizeof(stdDisplay_aAdapterNums));
    memset(&stdDisplay_g_frontBuffer, 0, sizeof(stdDisplay_g_frontBuffer));
    memset(&stdDisplay_g_backBuffer, 0, sizeof(stdDisplay_g_backBuffer));
    memset(&stdDisplay_zBuffer, 0, sizeof(stdDisplay_zBuffer));

    stdDisplay_pCurVideoMode           = NULL;
    stdDisplay_numDevices              = 0;
    stdDisplay_numVideoModes           = 0;
    stdDisplay_bDeviceLost              = false;
    stdDisplay_bDeviceResourcesInvalid  = false;
    stdDisplay_bBackBufferWriteLock     = false;
    stdDisplay_bPresentationBufferDirty = false;
    stdDisplay_bStartup                 = false;
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

    stdDisplay_curDevice  = deviceNum;
    stdDisplay_curAdapter = stdDisplay_aAdapterNums[deviceNum];
    stdDisplay_pCurDevice = &stdDisplay_aDisplayDevices[deviceNum];

    if ( !stdDisplay_InitDirect3D9(stdWin95_GetWindow()) )
    {
        return 0;
    }

    // Enumerate display modes for this adapter
    stdDisplay_numVideoModes = 0;
    stdDisplay_EnumerateVideoModes(stdDisplay_curAdapter);
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
    stdDisplay_curAdapter = D3DADAPTER_DEFAULT;
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
        HWND hwnd = stdWin95_GetWindow();
        if ( !stdDisplay_SetFullscreenMode(hwnd, &stdDisplay_aVideoModes[modeNum], numBackBuffers) )
        {
            stdDisplay_pCurVideoMode = NULL;
            return 1;
        }
    }
    else
    {
        stdDisplay_pCurVideoMode = &stdDisplay_primaryVideoMode;
        HWND hwnd = stdWin95_GetWindow();
        if ( !stdDisplay_SetWindowMode(hwnd, &stdDisplay_primaryVideoMode) )
        {
            stdDisplay_pCurVideoMode = NULL;
            return 1;
        }
    }

    int fheight = -(stdDisplay_pCurVideoMode->rasterInfo.width < 640);
    fheight = fheight & 0xF4;
    stdDisplay_hFont = CreateFont(fheight + 24, 0, 0, 0, FW_NORMAL, 0, 0, 0, ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, VARIABLE_PITCH, "Arial");

    stdDisplay_dword_5D73D8 = 0;
    stdDisplay_dword_5D73DC = 0;
    stdDisplay_backbufWidth  = stdDisplay_g_backBuffer.rasterInfo.width;
    stdDisplay_backbufHeight = stdDisplay_g_backBuffer.rasterInfo.height;
    stdDisplay_bModeSet      = true;
    stdDisplay_bFullscreen   = bFullscreen;

    stdDisplay_VBufferFill(&stdDisplay_g_backBuffer, 0, NULL);
    stdDisplay_Update();

    if ( bFullscreen )
    {
        stdDisplay_VBufferFill(&stdDisplay_g_backBuffer, 0, NULL);
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

    stdDisplay_pCurVideoMode            = NULL;
    stdDisplay_bModeSet                 = false;
    stdDisplay_bFullscreen              = false;
    stdDisplay_bDeviceLost              = false;
    stdDisplay_bDeviceResourcesInvalid  = false;
    stdDisplay_bBackBufferWriteLock     = false;
    stdDisplay_bPresentationBufferDirty = false;
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
        int deviceState = stdDisplay_CheckDeviceState();
        if ( deviceState == 2 )
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
    if ( !stdDisplay_pD3DDevice )
    {
        return -1;
    }

    HRESULT hr = IDirect3DDevice9_TestCooperativeLevel(stdDisplay_pD3DDevice);
    switch ( hr )
    {
        case D3D_OK:
            stdDisplay_bDeviceLost = false;
            return stdDisplay_bDeviceResourcesInvalid ? 2 : 0;
        case D3DERR_DEVICELOST:
            stdDisplay_bDeviceLost = true;
            return 1;
        case D3DERR_DEVICENOTRESET:
            stdDisplay_bDeviceLost = true;
            return 2;
        default:
            return -1;
    }
}

static void stdDisplay_ReleaseDevice(void)
{
    bool bReleaseOnly = false;
    if ( stdDisplay_pD3DDevice )
    {
        bReleaseOnly = FAILED(IDirect3DDevice9_TestCooperativeLevel(stdDisplay_pD3DDevice));
        if ( stdDisplay_pfDeviceReleaseCallback )
        {
            stdDisplay_pfDeviceReleaseCallback(stdDisplay_pD3DDevice);
        }
    }

    // Every surface created from the device must release its device reference before shutdown.
    stdDisplay_ReleaseTrackedVBufferSurfaces(true, bReleaseOnly);
    stdDisplay_ReleaseBuffersInternal(bReleaseOnly);

    if ( stdDisplay_pD3DDevice )
    {
        ULONG refCount = IDirect3DDevice9_Release(stdDisplay_pD3DDevice);
        if ( refCount > 0 )
        {
            STDLOG_WARNING("Warning: D3D9 device released with %lu references remaining.\n", refCount);
        }

        stdDisplay_pD3DDevice = NULL;
    }

    stdDisplay_bDeviceResourcesInvalid = false;
}

static int stdDisplay_ResetDevice(void)
{
    if ( !stdDisplay_pD3DDevice )
    {
        return 0;
    }

    if ( !stdDisplay_CanResetDevice() )
    {
        return 0;
    }

    bool bReleaseOnly = FAILED(IDirect3DDevice9_TestCooperativeLevel(stdDisplay_pD3DDevice));

    if ( stdDisplay_pfDevicePreResetCallback )
    {
        stdDisplay_pfDevicePreResetCallback(stdDisplay_pD3DDevice);
    }

    stdDisplay_bDeviceResourcesInvalid = true;

    // Release all default-pool resources before reset.
    stdDisplay_ReleaseTrackedVBufferSurfaces(false, bReleaseOnly);
    stdDisplay_ReleaseBuffersInternal(bReleaseOnly);

    D3DPRESENT_PARAMETERS resetParams = stdDisplay_presentParams;
    HRESULT hr = IDirect3DDevice9_Reset(stdDisplay_pD3DDevice, &resetParams);
    if ( FAILED(hr) )
    {
        stdDisplay_bDeviceLost = true;
        STDLOG_ERROR("Error %s when resetting D3D device.\n", stdDisplay_D3DGetStatus(hr));
        return 0;
    }

    // Recreate buffers
    if ( !stdDisplay_InitBuffers(stdDisplay_pD3DDevice, stdDisplay_pCurVideoMode, stdDisplay_presentParams.Windowed, stdDisplay_presentParams.BackBufferCount) )
    {
        STDLOG_ERROR("Error initializing buffers after device reset.\n");
        return 0;
    }

    if ( !stdDisplay_RestoreTrackedVBufferSurfaces() )
    {
        STDLOG_ERROR("Error restoring hardware VBuffers after device reset.\n");
        return 0;
    }

    if ( stdDisplay_pfDevicePostResetCallback
        && !stdDisplay_pfDevicePostResetCallback(stdDisplay_pD3DDevice) )
    {
        STDLOG_ERROR("Error reinitializing 3D resources after device reset.\n");
        return 0;
    }

    stdDisplay_bDeviceLost             = false;
    stdDisplay_bDeviceResourcesInvalid = false;
    return 1;
}

static bool stdDisplay_CreateHardwareVBufferSurface(tVBuffer* pVBuffer)
{
    if ( !stdDisplay_pD3DDevice || !pVBuffer )
    {
        return false;
    }

    D3DFORMAT format = stdDisplay_GetD3DFormat(pVBuffer->rasterInfo.colorInfo.bpp);
    D3DPOOL pool = pVBuffer->bVideoMemory ? D3DPOOL_DEFAULT : D3DPOOL_SYSTEMMEM;
    HRESULT hr = IDirect3DDevice9_CreateOffscreenPlainSurface(
        stdDisplay_pD3DDevice,
        pVBuffer->rasterInfo.width,
        pVBuffer->rasterInfo.height,
        format,
        pool,
        &pVBuffer->surface.pSysSurface,
        NULL
    );
    if ( FAILED(hr) )
    {
        STDLOG_ERROR("Error %s when creating D3D9 VBuffer surface.\n", stdDisplay_D3DGetStatus(hr));
        return false;
    }

    hr = IDirect3DSurface9_GetDesc(pVBuffer->surface.pSysSurface, &pVBuffer->surface.desc);
    if ( FAILED(hr) )
    {
        STDLOG_ERROR("Error %s when getting VBuffer surface description.\n", stdDisplay_D3DGetStatus(hr));
        IDirect3DSurface9_Release(pVBuffer->surface.pSysSurface);
        pVBuffer->surface.pSysSurface = NULL;
        return false;
    }

    D3DLOCKED_RECT lockedRect = { 0 };
    hr = IDirect3DSurface9_LockRect(pVBuffer->surface.pSysSurface, &lockedRect, NULL, D3DLOCK_READONLY);
    if ( FAILED(hr) )
    {
        STDLOG_ERROR("Error %s when probing VBuffer surface pitch.\n", stdDisplay_D3DGetStatus(hr));
        IDirect3DSurface9_Release(pVBuffer->surface.pSysSurface);
        pVBuffer->surface.pSysSurface = NULL;
        return false;
    }

    size_t rowSize       = lockedRect.Pitch < 0 ? (size_t)-lockedRect.Pitch : (size_t)lockedRect.Pitch;
    size_t bytesPerPixel = pVBuffer->rasterInfo.colorInfo.bpp / 8u;

    pVBuffer->rasterInfo.rowSize  = rowSize;
    pVBuffer->rasterInfo.rowWidth = rowSize / bytesPerPixel;
    pVBuffer->rasterInfo.size     = rowSize * pVBuffer->rasterInfo.height;

    hr = IDirect3DSurface9_UnlockRect(pVBuffer->surface.pSysSurface);
    if ( FAILED(hr) )
    {
        STDLOG_ERROR("Error %s when unlocking the VBuffer pitch probe.\n", stdDisplay_D3DGetStatus(hr));
        IDirect3DSurface9_Release(pVBuffer->surface.pSysSurface);
        pVBuffer->surface.pSysSurface = NULL;
        return false;
    }

    pVBuffer->pPixels      = NULL;
    pVBuffer->lockRefCount = 0u;
    return true;
}

static bool stdDisplay_TrackHardwareVBuffer(tVBuffer* pVBuffer)
{
    StdDisplayTrackedVBuffer* pEntry = (StdDisplayTrackedVBuffer*)STDMALLOC(sizeof(*pEntry));
    if ( !pEntry )
    {
        return false;
    }

    pEntry->pVBuffer = pVBuffer;
    pEntry->pNext    = stdDisplay_pTrackedVBuffers;

    stdDisplay_pTrackedVBuffers = pEntry;
    return true;
}

static void stdDisplay_UntrackHardwareVBuffer(tVBuffer* pVBuffer)
{
    StdDisplayTrackedVBuffer** ppEntry = &stdDisplay_pTrackedVBuffers;
    while ( *ppEntry )
    {
        if ( (*ppEntry)->pVBuffer == pVBuffer )
        {
            StdDisplayTrackedVBuffer* pEntry = *ppEntry;
            *ppEntry = pEntry->pNext;
            STDFREE(pEntry);
            return;
        }

        ppEntry = &(*ppEntry)->pNext;
    }
}

static bool stdDisplay_CanResetDevice(void)
{
    if ( stdDisplay_backLockRef || stdDisplay_frontLockRef || stdDisplay_backDCRef )
    {
        STDLOG_ERROR("Cannot reset the D3D9 device while a display buffer lock or DC is held.\n");
        return false;
    }

    for ( StdDisplayTrackedVBuffer* pEntry = stdDisplay_pTrackedVBuffers; pEntry; pEntry = pEntry->pNext )
    {
        tVBuffer* pVBuffer = pEntry->pVBuffer;
        if ( pVBuffer->bVideoMemory && pVBuffer->surface.pSysSurface && pVBuffer->lockRefCount )
        {
            STDLOG_ERROR("Cannot reset the D3D9 device while a video-memory VBuffer is locked.\n");
            return false;
        }
    }

    return true;
}

static void stdDisplay_ReleaseTrackedVBufferSurfaces(bool bReleaseAll, bool bReleaseOnly)
{
    for ( StdDisplayTrackedVBuffer* pEntry = stdDisplay_pTrackedVBuffers; pEntry; pEntry = pEntry->pNext )
    {
        tVBuffer* pVBuffer = pEntry->pVBuffer;
        if ( !pVBuffer->surface.pSysSurface || (!bReleaseAll && !pVBuffer->bVideoMemory) )
        {
            continue;
        }

        if ( pVBuffer->lockRefCount )
        {
            if ( !bReleaseOnly )
            {
                IDirect3DSurface9_UnlockRect(pVBuffer->surface.pSysSurface);
            }
            pVBuffer->lockRefCount = 0u;
            pVBuffer->pPixels      = NULL;
        }

        IDirect3DSurface9_Release(pVBuffer->surface.pSysSurface);
        STD_ZEROMEM(&pVBuffer->surface, sizeof(pVBuffer->surface));
    }
}

static bool stdDisplay_RestoreTrackedVBufferSurfaces(void)
{
    for ( StdDisplayTrackedVBuffer* pEntry = stdDisplay_pTrackedVBuffers; pEntry; pEntry = pEntry->pNext )
    {
        if ( !pEntry->pVBuffer->surface.pSysSurface
            && !stdDisplay_CreateHardwareVBufferSurface(pEntry->pVBuffer) )
        {
            stdDisplay_ReleaseTrackedVBufferSurfaces(true, false);
            return false;
        }
    }

    return true;
}

tVBuffer* J3DAPI stdDisplay_VBufferNew(const tRasterInfo* pRasterInfo, int bUseVSurface, int bUseVideoMemory)
{
    STD_ASSERTREL((pRasterInfo->colorInfo.bpp % 8) == 0);

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

    if ( bUseVSurface && stdDisplay_bOpen && stdDisplay_pD3DDevice )
    {
        vbuffer->bVideoMemory = bUseVideoMemory ? 1 : 0;
        vbuffer->type         = VBUFFER_HARDWARE;

        if ( !stdDisplay_CreateHardwareVBufferSurface(vbuffer)
            || !stdDisplay_TrackHardwareVBuffer(vbuffer) )
        {
            if ( vbuffer->surface.pSysSurface )
            {
                IDirect3DSurface9_Release(vbuffer->surface.pSysSurface);
            }
            stdMemory_Free(vbuffer);
            return NULL;
        }

        return vbuffer;
    }
    else
    {
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
    else if ( pVBuffer->type == VBUFFER_HARDWARE )
    {
        stdDisplay_UntrackHardwareVBuffer(pVBuffer);
        if ( pVBuffer->surface.pSysSurface )
        {
            IDirect3DSurface9_Release(pVBuffer->surface.pSysSurface);
            pVBuffer->surface.pSysSurface = NULL;
        }
    }

    stdMemory_Free(pVBuffer);
}

static int stdDisplay_VBufferLockInternal(tVBuffer* pVBuffer, bool bReadOnly)
{
    STD_ASSERTREL(pVBuffer != NULL);

    if ( pVBuffer->type == VBUFFER_SOFTWARE )
    {
        // Added: Software vbuffers own RAM for their full lifetime; assert the invariant before updating the lock ref count.
        STD_ASSERT(pVBuffer->pPixels);
        ++pVBuffer->lockRefCount;
    }
    else if ( pVBuffer->type == VBUFFER_HARDWARE )
    {
        // Reuse an existing hardware lock instead of trying to lock the Direct3D surface a second time.
        if ( pVBuffer->lockRefCount )
        {
            if ( pVBuffer == &stdDisplay_g_backBuffer && !bReadOnly && !stdDisplay_bBackBufferWriteLock )
            {
                STDLOG_ERROR("Cannot upgrade an active read-only back-buffer lock to a write lock.\n");
                return 0;
            }
            ++pVBuffer->lockRefCount;
            return 1;
        }

        // TODO: should lock front buffer same way to copy to copy front buff to lockable surface incase of MSAA
        if ( pVBuffer == &stdDisplay_g_backBuffer )
        {
            uint32_t width, height;
            int pitch;
            int lockResult = bReadOnly
                ? stdDisplay_LockBackBufferReadOnly(&pVBuffer->pPixels, &width, &height, &pitch)
                : stdDisplay_LockBackBuffer(&pVBuffer->pPixels, &width, &height, &pitch);
            if ( lockResult )
            {
                return 0;
            }

            size_t rowSize = pitch < 0 ? (size_t)-pitch : (size_t)pitch;
            size_t bytesPerPixel = pVBuffer->rasterInfo.colorInfo.bpp / 8u;
            pVBuffer->rasterInfo.width    = width;
            pVBuffer->rasterInfo.height   = height;
            pVBuffer->rasterInfo.rowSize  = rowSize;
            pVBuffer->rasterInfo.rowWidth = rowSize / bytesPerPixel;
            pVBuffer->rasterInfo.size     = rowSize * height;
        }
        else
        {
            pVBuffer->pPixels = stdDisplay_LockSurface(&pVBuffer->surface);
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
            --pVBuffer->lockRefCount; //TODO: Hmm shouldn't it be clamped to 1 as software buffer has always locked surface, i.g.: pPixels member set through out the life of VBuffer
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

    // Keep the underlying surface locked until the final nested VBuffer unlock.
    if ( pVBuffer->lockRefCount > 1 )
    {
        --pVBuffer->lockRefCount;
        return 1;
    }

    int bFailed = 0;
    if ( pVBuffer == &stdDisplay_g_backBuffer )
    {
        stdDisplay_UnlockBackBuffer();
    }
    else
    {
        bFailed = stdDisplay_UnlockSurface(&pVBuffer->surface);
    }

    if ( !bFailed )
    {
        --pVBuffer->lockRefCount;
        pVBuffer->pPixels = NULL;
    }

    return bFailed; // 1 locked - 0 unlocked
}

int J3DAPI stdDisplay_VBufferFill(tVBuffer* pVBuffer, uint32_t color, const StdRect* pRect)
{
    STD_ASSERTREL(pVBuffer != NULL);

    if ( pVBuffer->type )
    {
        if ( pVBuffer->type != VBUFFER_HARDWARE )
        {
            return 1;
        }

        if ( pVBuffer->bVideoMemory )
        {
            return stdDisplay_ColorFillSurface(&pVBuffer->surface, color, pRect) == 0;
        }

        // ColorFill only accepts default-pool offscreen surfaces, so fill system-memory surfaces through their lock.
        if ( !stdDisplay_VBufferLock(pVBuffer) )
        {
            return 0;
        }

        pVBuffer->type = VBUFFER_SOFTWARE;
        int bFilled = stdDisplay_VBufferFill(pVBuffer, color, pRect);
        pVBuffer->type = VBUFFER_HARDWARE;
        return stdDisplay_VBufferUnlock(pVBuffer) == 0 && bFilled;
    }

    // Software fill for system memory buffers
    switch ( pVBuffer->rasterInfo.colorInfo.bpp )
    {
        case 8:
            if ( pRect )
            {
                uint8_t* pPixels8 = &pVBuffer->pPixels[pVBuffer->rasterInfo.rowSize * pRect->top + pRect->left];
                for ( int32_t height = 0; height < pRect->bottom; ++height )
                {
                    memset(pPixels8, (uint8_t)color, pRect->right);
                    pPixels8 += pVBuffer->rasterInfo.rowSize;
                }
            }
            else
            {
                memset(pVBuffer->pPixels, (uint8_t)color, pVBuffer->rasterInfo.size);
            }
            break;

        case 16:
            if ( pRect )
            {
                uint16_t* pPixels16 = (uint16_t*)&pVBuffer->pPixels[pVBuffer->rasterInfo.rowSize * pRect->top + 2 * pRect->left];
                for ( int32_t height = 0; height < pRect->bottom; ++height )
                {
                    stdDisplay_SetPixels16(pPixels16, (uint16_t)color, pRect->right);
                    pPixels16 = (uint16_t*)((char*)pPixels16 + pVBuffer->rasterInfo.rowSize);
                }
            }
            else
            {
                stdDisplay_SetPixels16((uint16_t*)pVBuffer->pPixels, (uint16_t)color, pVBuffer->rasterInfo.size / 2);
            }
            break;

        case 24:
            STDLOG_FATAL("24-bit fill not implemented");
            break;

        case 32:
            if ( pRect )
            {
                uint32_t* pPixels32 = (uint32_t*)&pVBuffer->pPixels[pVBuffer->rasterInfo.rowSize * pRect->top + 4 * pRect->left];
                for ( int32_t height = 0; height < pRect->bottom; ++height )
                {
                    stdDisplay_SetPixels32(pPixels32, color, pRect->right);
                    pPixels32 = (uint32_t*)((char*)pPixels32 + pVBuffer->rasterInfo.rowSize);
                }
            }
            else
            {
                stdDisplay_SetPixels32((uint32_t*)pVBuffer->pPixels, color, pVBuffer->rasterInfo.size / 4);
            }
            break;
    }

    return 1;
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
    STD_ASSERTREL(pSrcColorFormat->redPosShift != 0 || pSrcColorFormat->greenPosShift != 0 || pSrcColorFormat->bluePosShift != 0);
    STD_ASSERTREL(pDesiredColorFormat->redPosShift != 0 || pDesiredColorFormat->greenPosShift != 0 || pDesiredColorFormat->bluePosShift != 0);

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
    if ( !stdDisplay_pD3DDevice )
    {
        return 1;
    }

    // TODO: Enhance
    UINT availableTextureMem = IDirect3DDevice9_GetAvailableTextureMem(stdDisplay_pD3DDevice);
    *pFree  = availableTextureMem;
    *pTotal = stdDisplay_deviceCaps.MaxTextureWidth * stdDisplay_deviceCaps.MaxTextureHeight * 4; // Approximation
    return 0;
}

int J3DAPI stdDisplay_GetTotalMemory(size_t* pTotal, size_t* pFree)
{
    if ( !stdDisplay_pD3D9 )
    {
        return 1;
    }

    // TODO: Enhance / Fix
    UINT adapter = stdDisplay_numDevices > 0 ? stdDisplay_curAdapter : D3DADAPTER_DEFAULT;
    *pTotal = IDirect3D9_GetAdapterModeCount(stdDisplay_pD3D9, adapter, D3DFMT_X8R8G8B8) * 1024 * 1024; // Approximation
    *pFree = *pTotal / 2; // Rough estimate
    return 0;
}

const char* J3DAPI stdDisplay_D3DGetStatus(HRESULT status)
{
    for ( size_t i = 0; i < STD_ARRAYLEN(stdDisplay_aD3DStatusTbl); ++i )
    {
        if ( stdDisplay_aD3DStatusTbl[i].code == status )
        {
            return stdDisplay_aD3DStatusTbl[i].text;
        }
    }
    return "Unknown D3D Error";
}

int J3DAPI stdDisplay_CreateZBuffer(const tSysPixelFormat* pPixelFormat, int bSystemMemory)
{
    J3D_UNUSED(bSystemMemory);

    if ( !stdDisplay_bOpen || !stdDisplay_pD3DDevice )
    {
        STDLOG_ERROR("Error creating zBuffer, system is closed!\n");
        return 1;
    }

    // Release an existing Z-buffer before replacing it during a repeated std3D open cycle.
    if ( stdDisplay_zBuffer.pSysSurface )
    {
        IDirect3DDevice9_SetDepthStencilSurface(stdDisplay_pD3DDevice, NULL);
        IDirect3DSurface9_Release(stdDisplay_zBuffer.pSysSurface);
        STD_ZEROMEM(&stdDisplay_zBuffer, sizeof(stdDisplay_zBuffer));
    }

    // Convert DirectDraw pixel format to D3D9 format
    D3DFORMAT depthFormat = *pPixelFormat;
    if ( stdDisplay_bMSAAEnabled )
    {
        D3DDISPLAYMODE adapterMode;
        HRESULT modeResult = IDirect3D9_GetAdapterDisplayMode(stdDisplay_pD3D9, stdDisplay_curAdapter, &adapterMode);
        if ( FAILED(modeResult) )
        {
            STDLOG_ERROR("Error %s when querying the adapter format for the Z-buffer.\n", stdDisplay_D3DGetStatus(modeResult));
            return 1;
        }

        DWORD qualityLevels = 0;
        if ( !stdDisplay_CheckMSAASupport(
            stdDisplay_curAdapter,
            adapterMode.Format,
            stdDisplay_g_backBuffer.surface.desc.Format,
            depthFormat,
            stdDisplay_presentParams.Windowed,
            stdDisplay_msaaSampleType,
            &qualityLevels
        ) || stdDisplay_msaaSampleQuality >= qualityLevels )
        {
            STDLOG_ERROR("Depth format %d does not support the active MSAA type and quality.\n", depthFormat);
            return 1;
        }
    }

    HRESULT hr = IDirect3DDevice9_CreateDepthStencilSurface(
        stdDisplay_pD3DDevice,
        stdDisplay_g_backBuffer.rasterInfo.width,
        stdDisplay_g_backBuffer.rasterInfo.height,
        depthFormat,
        stdDisplay_msaaSampleType,        // Use MSAA settings
        stdDisplay_msaaSampleQuality,     // Use MSAA quality
        TRUE,
        &stdDisplay_zBuffer.pSysSurface,
        NULL
    );

    if ( FAILED(hr) )
    {
        STDLOG_ERROR("Error %s when creating depth/stencil surface.\n", stdDisplay_D3DGetStatus(hr));
        return 1;
    }

    hr = IDirect3DDevice9_SetDepthStencilSurface(stdDisplay_pD3DDevice, stdDisplay_zBuffer.pSysSurface);
    if ( FAILED(hr) )
    {
        STDLOG_ERROR("Error %s when setting depth/stencil surface.\n", stdDisplay_D3DGetStatus(hr));

        IDirect3DSurface9_Release(stdDisplay_zBuffer.pSysSurface);
        stdDisplay_zBuffer.pSysSurface = NULL;
        return 1;
    }

    hr = IDirect3DSurface9_GetDesc(stdDisplay_zBuffer.pSysSurface, &stdDisplay_zBuffer.desc);
    if ( FAILED(hr) )
    {
        STDLOG_ERROR("Error %s when getting zbuffer surface description.\n", stdDisplay_D3DGetStatus(hr));
        // Unbind and release the Z-buffer when querying its description fails.
        IDirect3DDevice9_SetDepthStencilSurface(stdDisplay_pD3DDevice, NULL);
        IDirect3DSurface9_Release(stdDisplay_zBuffer.pSysSurface);
        STD_ZEROMEM(&stdDisplay_zBuffer, sizeof(stdDisplay_zBuffer));
        return 1;
    }

    return 0;
}

static int J3DAPI stdDisplay_InitDirect3D9(HWND hwnd)
{
    J3D_UNUSED(hwnd);

    if ( !stdDisplay_pD3D9 )
    {
        return 0;
    }

    // Get device capabilities
    UINT adapter = stdDisplay_numDevices > 0 ? stdDisplay_curAdapter : D3DADAPTER_DEFAULT;
    HRESULT hr = IDirect3D9_GetDeviceCaps(stdDisplay_pD3D9, adapter, D3DDEVTYPE_HAL, &stdDisplay_deviceCaps); // TODO: device should already have caps set
    if ( FAILED(hr) )
    {
        STDLOG_ERROR("Error %s when getting device capabilities.\n", stdDisplay_D3DGetStatus(hr));
        return 0;
    }

    return 1;
}

static int J3DAPI stdDisplay_EnumerateDevices(void)
{
    if ( !stdDisplay_pD3D9 )
    {
        return 0;
    }

    UINT adapterCount = IDirect3D9_GetAdapterCount(stdDisplay_pD3D9);
    stdDisplay_numDevices = 0;

    for ( UINT i = 0; i < adapterCount && i < STD_ARRAYLEN(stdDisplay_aDisplayDevices); i++ )
    {
        D3DADAPTER_IDENTIFIER9 identifier;
        HRESULT hr = IDirect3D9_GetAdapterIdentifier(stdDisplay_pD3D9, i, 0, &identifier);
        if ( FAILED(hr) )
        {
            continue;
        }

        StdDisplayDevice* pDevice = &stdDisplay_aDisplayDevices[stdDisplay_numDevices];

        // Fill device information
        char* pDisplayName = strrchr(identifier.DeviceName, '\\'); // Left strip name to the last '\' (.e.g. "\\.\DISPLAY1" -> "\DISPLAY1")
        STD_STRCPY(pDevice->aDriverName, pDisplayName ? pDisplayName + 1 : identifier.DeviceName); //aDriver should be actually aDisplayDevice
        STD_STRCPY(pDevice->aDeviceName, identifier.Description);  // aDeviceName should be a3DDevice

        // Try to get monitor friendly name
        DISPLAY_DEVICE displayDevice;
        displayDevice.cb = sizeof(displayDevice);
        if ( EnumDisplayDevices(identifier.DeviceName, 0, &displayDevice, 0) )
        {
            STD_STRCPY(pDevice->aDriverName, displayDevice.DeviceString);
        }

        // Get device capabilities
        ZeroMemory(&pDevice->caps, sizeof(pDevice->caps));
        hr = IDirect3D9_GetDeviceCaps(stdDisplay_pD3D9, i, D3DDEVTYPE_HAL, &pDevice->caps);
        if ( FAILED(hr) )
        {
            continue;
        }

        D3DDISPLAYMODE desktopMode;
        hr = IDirect3D9_GetAdapterDisplayMode(stdDisplay_pD3D9, i, &desktopMode);
        if ( FAILED(hr)
            || FAILED(IDirect3D9_CheckDeviceType(
                stdDisplay_pD3D9,
                i,
                D3DDEVTYPE_HAL,
                desktopMode.Format,
                desktopMode.Format,
                TRUE
            )) )
        {
            continue;
        }

        pDevice->bHAL                      = TRUE;
        pDevice->bWindowRenderNotSupported = FALSE;
        pDevice->guid                      = identifier.DeviceIdentifier;
        pDevice->totalVideoMemory          = pDevice->caps.MaxTextureWidth * pDevice->caps.MaxTextureHeight * 4;
        pDevice->freeVideoMemory           = pDevice->totalVideoMemory / 2;
        stdDisplay_aAdapterNums[stdDisplay_numDevices] = i;

        STDLOG_STATUS("Found %s D3D9 Device: %s [%s]\n", pDevice->bHAL ? "HAL" : "REF", pDevice->aDeviceName, pDevice->aDriverName);
        STDLOG_STATUS("Memory: 0x%x out of 0x%x free\n", pDevice->freeVideoMemory, pDevice->totalVideoMemory);
        ++stdDisplay_numDevices;
    }

    return stdDisplay_numDevices > 0;
}

static int J3DAPI stdDisplay_EnumerateVideoModes(UINT adapter)
{
    if ( !stdDisplay_pD3D9 )
    {
        return 0;
    }

    D3DDISPLAYMODE curDesktopMode = { 0 };
    HRESULT hr = IDirect3D9_GetAdapterDisplayMode(stdDisplay_pD3D9, adapter, &curDesktopMode);
    if ( FAILED(hr) )
    {
        STDLOG_ERROR("Error: Could not get adapter display mode for adapter %d.\n", adapter);
        return 0;
    }

    ColorInfo desktopColorInfo;
    if ( !stdDisplay_GetVideoColorFormat(curDesktopMode.Format, &desktopColorInfo) )
    {
        STDLOG_ERROR("stdDisplay_EnumerateVideoModes: Current desktop mode format %d is not supported by stdDisplay.\n", curDesktopMode.Format);
        return 0;
    }
    STDLOG_DEBUG("stdDisplay_EnumerateVideoModes: Using current desktop mode format: %d\n", curDesktopMode.Format);
    stdDisplay_primaryVideoMode.rasterInfo.colorInfo = desktopColorInfo;

    bool bModeLimitReached = false;
    for ( size_t formatNum = 0u; formatNum < STD_ARRAYLEN(stdDisplay_aSupportedFormats); ++formatNum )
    {
        D3DFORMAT format = stdDisplay_aSupportedFormats[formatNum];
        UINT modeCount = IDirect3D9_GetAdapterModeCount(stdDisplay_pD3D9, adapter, format);
        for ( UINT i = 0u; i < modeCount; ++i )
        {
            if ( stdDisplay_numVideoModes >= STD_ARRAYLEN(stdDisplay_aVideoModes) )
            {
                bModeLimitReached = true;
                break;
            }

            D3DDISPLAYMODE mode;
            hr = IDirect3D9_EnumAdapterModes(stdDisplay_pD3D9, adapter, format, i, &mode);
            if ( FAILED(hr) )
            {
                continue;
            }

            int bpp = stdDisplay_BppFromD3DFormat(mode.Format);
            if ( bpp < 16 || mode.RefreshRate < STDDISPLAY_MINFRAMERATE || mode.RefreshRate > STDDISPLAY_MAXFRAMERATE )
            {
                continue;
            }

            D3DFORMAT backBufferFormat = stdDisplay_GetD3DFormat(bpp);
            if ( FAILED(IDirect3D9_CheckDeviceType(
                stdDisplay_pD3D9,
                adapter,
                D3DDEVTYPE_HAL,
                mode.Format,
                backBufferFormat,
                FALSE
            )) )
            {
                continue;
            }

            StdVideoMode* pVideoMode = &stdDisplay_aVideoModes[stdDisplay_numVideoModes];
            STD_ZEROMEM(pVideoMode, sizeof(*pVideoMode));
            pVideoMode->refreshRate       = mode.RefreshRate;
            pVideoMode->rasterInfo.width  = mode.Width;
            pVideoMode->rasterInfo.height = mode.Height;

            if ( !stdDisplay_GetVideoColorFormat(backBufferFormat, &pVideoMode->rasterInfo.colorInfo) )
            {
                continue;
            }

            size_t bytesPerPixel = (size_t)bpp / 8u;
            pVideoMode->rasterInfo.rowSize  = pVideoMode->rasterInfo.width * bytesPerPixel;
            pVideoMode->rasterInfo.rowWidth = pVideoMode->rasterInfo.width;
            pVideoMode->rasterInfo.size     = pVideoMode->rasterInfo.rowSize * pVideoMode->rasterInfo.height;
            stdDisplay_SetAspectRatio(pVideoMode);

            size_t requiredVRam = 3u * pVideoMode->rasterInfo.size;
            STDLOG_STATUS("Video Mode: %ux%u %u bit (%u Hz), Required: %zu bytes.\n",
                pVideoMode->rasterInfo.width,
                pVideoMode->rasterInfo.height,
                bpp,
                pVideoMode->refreshRate,
                requiredVRam
            );

            ++stdDisplay_numVideoModes;
        }
    }

    if ( bModeLimitReached )
    {
        STDLOG_WARNING("Too many video modes for adapter %d, only %zu modes are retained.\n", adapter, STD_ARRAYLEN(stdDisplay_aVideoModes));
    }

    return stdDisplay_numVideoModes;
}

D3DFORMAT J3DAPI stdDisplay_GetD3DFormat(int bpp)
{
    switch ( bpp )
    {
        case 16: return D3DFMT_R5G6B5;
        case 24: return D3DFMT_R8G8B8;
        case 32: return D3DFMT_X8R8G8B8;
        default: return D3DFMT_X8R8G8B8;
    }
}

int J3DAPI stdDisplay_BppFromD3DFormat(D3DFORMAT format)
{
    switch ( format )
    {
        case D3DFMT_R5G6B5:
        case D3DFMT_X1R5G5B5:
        case D3DFMT_A1R5G5B5:
            return 16;
        case D3DFMT_R8G8B8:
            return 24;
        case D3DFMT_X8R8G8B8:
        case D3DFMT_A8R8G8B8:
            return 32;
        default:
            return 32;
    }
}

// Get color info for format of video mode and video surface
bool stdDisplay_GetVideoColorFormat(D3DFORMAT format, ColorInfo* pFormat)
{
    switch ( format )
    {
        case D3DFMT_R8G8B8:
            *pFormat = stdColor_cfRGB888;
            return true;
        case D3DFMT_X8R8G8B8:
        case D3DFMT_A8R8G8B8:
            *pFormat = stdColor_cfRGB8888;
            // Note, no alpha for video mode color format
            return true;

        case D3DFMT_R5G6B5:
            *pFormat = stdColor_cfRGB565;
            return true;

        case D3DFMT_X1R5G5B5:
            *pFormat = stdColor_cfRGB555;
            return true;
    }
    return false;
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
    return stdDisplay_pD3D9;
}

size_t stdDisplay_GetAdapterNum(size_t deviceNum)
{
    return deviceNum < stdDisplay_numDevices ? stdDisplay_aAdapterNums[deviceNum] : D3DADAPTER_DEFAULT;
}

tSysDisplayDevice* stdDisplay_GetSystemDevice(void)
{
    return stdDisplay_pD3DDevice;
}

static int J3DAPI stdDisplay_SetWindowMode(HWND hWnd, StdVideoMode* pDisplayMode)
{
    if ( !SetWindowPos(hWnd, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE) )
    {
        return 0;
    }

    // Get desktop format for windowed mode
    D3DDISPLAYMODE desktopMode;
    HRESULT hr = IDirect3D9_GetAdapterDisplayMode(stdDisplay_pD3D9, stdDisplay_curAdapter, &desktopMode);
    if ( FAILED(hr) )
    {
        STDLOG_ERROR("Error %d getting desktop display mode\n", stdDisplay_D3DGetStatus(hr));
        return 0;
    }

    D3DFORMAT depthFormat = D3DFMT_UNKNOWN;
    if ( !stdDisplay_FindDepthFormat(stdDisplay_curAdapter, desktopMode.Format, desktopMode.Format, &depthFormat) )
    {
        STDLOG_WARNING("No compatible D3D9 depth format was found for windowed mode.\n");
    }

    // Validate MSAA settings for the exact windowed render-target and depth pair.
    stdDisplay_ValidateMSAASettings(stdDisplay_curAdapter, desktopMode.Format, desktopMode.Format, depthFormat, TRUE);

    // Setup present parameters for windowed mode
    ZeroMemory(&stdDisplay_presentParams, sizeof(stdDisplay_presentParams));
    stdDisplay_presentParams.BackBufferWidth            = pDisplayMode->rasterInfo.width;
    stdDisplay_presentParams.BackBufferHeight           = pDisplayMode->rasterInfo.height;
    stdDisplay_presentParams.BackBufferFormat           = D3DFMT_UNKNOWN; // Use desktop format
    stdDisplay_presentParams.BackBufferCount            = 1;
    stdDisplay_presentParams.MultiSampleType            = D3DMULTISAMPLE_NONE;
    stdDisplay_presentParams.MultiSampleQuality         = 0;
    stdDisplay_presentParams.SwapEffect                 = D3DSWAPEFFECT_DISCARD;
    stdDisplay_presentParams.hDeviceWindow              = hWnd;
    stdDisplay_presentParams.Windowed                   = TRUE;
    stdDisplay_presentParams.EnableAutoDepthStencil     = FALSE; // TODO: In the future this could be enabled and z buffer creation function removed
    stdDisplay_presentParams.AutoDepthStencilFormat     = depthFormat;
    stdDisplay_presentParams.Flags                      = D3DPRESENTFLAG_LOCKABLE_BACKBUFFER;
    stdDisplay_presentParams.FullScreen_RefreshRateInHz = 0; // Must be 0 for window mode
    stdDisplay_presentParams.PresentationInterval       = stdDisplay_bNoSync ? D3DPRESENT_INTERVAL_IMMEDIATE : D3DPRESENT_INTERVAL_DEFAULT;

    // Reset or Create D3D9 device
    if ( stdDisplay_pD3DDevice )
    {
        if ( !stdDisplay_ResetDevice() )
        {
            STDLOG_ERROR("Error resetting D3D9 device when setting window mode.\n");
            return 0;
        }
        return 1;
    }
    else
    {
        // Create new device
        D3DPRESENT_PARAMETERS createParams = stdDisplay_presentParams;
        hr = IDirect3D9_CreateDevice(
            stdDisplay_pD3D9,
            stdDisplay_curAdapter,
            D3DDEVTYPE_HAL,
            hWnd,
            stdDisplay_GetDeviceCreateFlags(),
            &createParams,
            &stdDisplay_pD3DDevice
        );

        if ( FAILED(hr) )
        {
            STDLOG_ERROR("Error %s when creating D3D9 device for windowed mode.\n", stdDisplay_D3DGetStatus(hr));
            return 0;
        }
    }

    if ( !stdDisplay_InitBuffers(stdDisplay_pD3DDevice, pDisplayMode, /*bWindowMode=*/true, stdDisplay_presentParams.BackBufferCount)
        || !stdDisplay_RestoreTrackedVBufferSurfaces() )
    {
        stdDisplay_ReleaseDevice();
        return 0;
    }

    return 1;
}

static DWORD stdDisplay_GetDeviceCreateFlags(void)
{
    return (stdDisplay_deviceCaps.DevCaps & D3DDEVCAPS_HWTRANSFORMANDLIGHT) != 0
        ? D3DCREATE_HARDWARE_VERTEXPROCESSING
        : D3DCREATE_SOFTWARE_VERTEXPROCESSING;
}

int J3DAPI stdDisplay_VBufferLock(tVBuffer* pVBuffer)
{
    return stdDisplay_VBufferLockInternal(pVBuffer, false);
}

int stdDisplay_VBufferLockReadOnly(tVBuffer* pVBuffer)
{
    return stdDisplay_VBufferLockInternal(pVBuffer, true);
}

int J3DAPI stdDisplay_SetFullscreenMode(HWND hwnd, const StdVideoMode* pDisplayMode, size_t numBackBuffers)
{
    // NOTE: the return value indicating success or failure changed from original code.
    // Now function will return 0 on error and 1 on success.

    // Validate MSAA settings for the exact fullscreen render-target and depth pair.
    D3DFORMAT format = stdDisplay_GetD3DFormat(pDisplayMode->rasterInfo.colorInfo.bpp);
    D3DFORMAT depthFormat = D3DFMT_UNKNOWN;
    if ( !stdDisplay_FindDepthFormat(stdDisplay_curAdapter, format, format, &depthFormat) )
    {
        STDLOG_WARNING("No compatible D3D9 depth format was found for fullscreen mode.\n");
    }
    stdDisplay_ValidateMSAASettings(stdDisplay_curAdapter, format, format, depthFormat, FALSE);

    // Setup present parameters for fullscreen mode
    ZeroMemory(&stdDisplay_presentParams, sizeof(stdDisplay_presentParams));
    stdDisplay_presentParams.BackBufferWidth            = pDisplayMode->rasterInfo.width;
    stdDisplay_presentParams.BackBufferHeight           = pDisplayMode->rasterInfo.height;
    stdDisplay_presentParams.BackBufferFormat           = stdDisplay_GetD3DFormat(pDisplayMode->rasterInfo.colorInfo.bpp);
    stdDisplay_presentParams.BackBufferCount            = numBackBuffers;
    stdDisplay_presentParams.MultiSampleType            = D3DMULTISAMPLE_NONE;
    stdDisplay_presentParams.MultiSampleQuality         = 0;
    stdDisplay_presentParams.SwapEffect                 = D3DSWAPEFFECT_DISCARD;
    stdDisplay_presentParams.hDeviceWindow              = hwnd;
    stdDisplay_presentParams.Windowed                   = FALSE;
    stdDisplay_presentParams.EnableAutoDepthStencil     = FALSE; // TODO: In the future this could be enabled and z buffer creation function removed
    stdDisplay_presentParams.AutoDepthStencilFormat     = depthFormat;
    stdDisplay_presentParams.Flags                      = D3DPRESENTFLAG_LOCKABLE_BACKBUFFER;
    stdDisplay_presentParams.FullScreen_RefreshRateInHz = pDisplayMode->refreshRate;
    stdDisplay_presentParams.PresentationInterval       = stdDisplay_bNoSync ? D3DPRESENT_INTERVAL_IMMEDIATE : D3DPRESENT_INTERVAL_DEFAULT;

    STDLOG_STATUS("Set video mode %d %d %d with MSAA %dx.\n",
        pDisplayMode->rasterInfo.width,
        pDisplayMode->rasterInfo.height,
        pDisplayMode->rasterInfo.colorInfo.bpp,
        stdDisplay_bMSAAEnabled ? stdDisplay_msaaSampleCount : 1
    );

    // Reset or Create D3D9 device
    if ( stdDisplay_pD3DDevice )
    {
        if ( !stdDisplay_ResetDevice() )
        {
            STDLOG_ERROR("Error resetting D3D9 device when setting fullscreen mode.\n");
            return 0;
        }

        return 1;
    }
    else
    {
        // Create D3D9 device
        D3DPRESENT_PARAMETERS createParams = stdDisplay_presentParams;
        HRESULT hr = IDirect3D9_CreateDevice(
            stdDisplay_pD3D9,
            stdDisplay_curAdapter,
            D3DDEVTYPE_HAL,
            hwnd,
            stdDisplay_GetDeviceCreateFlags(),
            &createParams,
            &stdDisplay_pD3DDevice
        );

        if ( hr == D3DERR_DEVICELOST )
        {
            STDLOG_WARNING("Warning: D3D9 device lost when creating for fullscreen mode, retrying in hybrid mode...\n");

            // Switch to windowed mode for recovery
            D3DPRESENT_PARAMETERS recoveryParams = stdDisplay_presentParams;
            recoveryParams.Windowed                   = TRUE;
            recoveryParams.FullScreen_RefreshRateInHz = 0;
            hr = IDirect3D9_CreateDevice(
                stdDisplay_pD3D9,
                stdDisplay_curAdapter,
                D3DDEVTYPE_HAL,
                hwnd,
                stdDisplay_GetDeviceCreateFlags(),
                &recoveryParams,
                &stdDisplay_pD3DDevice
            );

            if ( FAILED(hr) )
            {
                STDLOG_ERROR("Error %s when creating D3D9 device for hybrid mode.\n", stdDisplay_D3DGetStatus(hr));
                return 0;
            }

            D3DPRESENT_PARAMETERS fullscreenParams = stdDisplay_presentParams;
            hr = IDirect3DDevice9_Reset(stdDisplay_pD3DDevice, &fullscreenParams);
        }

        if ( FAILED(hr) )
        {
            STDLOG_ERROR("Error %s when creating D3D9 device for fullscreen mode.\n", stdDisplay_D3DGetStatus(hr));
            stdDisplay_ReleaseDevice();
            return 0;
        }
    }

    if ( !stdDisplay_InitBuffers(stdDisplay_pD3DDevice, (StdVideoMode*)pDisplayMode, /*bWindowMode=*/false, stdDisplay_presentParams.BackBufferCount)
        || !stdDisplay_RestoreTrackedVBufferSurfaces() )
    {
        stdDisplay_ReleaseDevice();
        return 0;
    }

    return 1;
}

int J3DAPI stdDisplay_InitBuffers(PDIRECT3DDEVICE9 pDevice, StdVideoMode* pDisplayMode, bool bWindowMode, size_t numBuffers)
{
    J3D_UNUSED(numBuffers);

    if ( stdDisplay_g_frontBuffer.surface.pSysSurface || stdDisplay_g_backBuffer.surface.pSysSurface )
    {
        stdDisplay_ReleaseBuffers();
    }

    stdDisplay_frontLockRef               = 0;
    stdDisplay_backLockRef                = 0;
    stdDisplay_backDCRef                  = 0;
    stdDisplay_bBackBufferWriteLock       = false;
    stdDisplay_hdcFront                   = NULL;
    stdDisplay_hdcBack                    = NULL;
    stdDisplay_bPresentationBufferDirty   = false;
    STD_ZEROMEM(&stdDisplay_backLockedRect, sizeof(stdDisplay_backLockedRect));

    // Direct3D 9 does not expose a writable front buffer. Keep the engine's front-buffer
    // wrapper on swap-chain buffer zero, which is the surface that will be presented.
    HRESULT hr = IDirect3DDevice9_GetBackBuffer(
        pDevice,
        0,
        0,
        D3DBACKBUFFER_TYPE_MONO,
        &stdDisplay_g_frontBuffer.surface.pSysSurface
    );
    if ( FAILED(hr) )
    {
        STDLOG_ERROR("Error %s when getting the presentation buffer.\n", stdDisplay_D3DGetStatus(hr));
        goto error;
    }

    hr = IDirect3DSurface9_GetDesc(
        stdDisplay_g_frontBuffer.surface.pSysSurface,
        &stdDisplay_g_frontBuffer.surface.desc
    );
    if ( FAILED(hr) )
    {
        STDLOG_ERROR("Error %s when getting the presentation buffer description.\n", stdDisplay_D3DGetStatus(hr));
        goto error;
    }

    pDisplayMode->rasterInfo.width  = stdDisplay_g_frontBuffer.surface.desc.Width;
    pDisplayMode->rasterInfo.height = stdDisplay_g_frontBuffer.surface.desc.Height;
    if ( !stdDisplay_GetVideoColorFormat(
        stdDisplay_g_frontBuffer.surface.desc.Format,
        &pDisplayMode->rasterInfo.colorInfo
    ) )
    {
        STDLOG_ERROR("Couldn't get presentation-buffer color info for format %d.\n", stdDisplay_g_frontBuffer.surface.desc.Format);
        goto error;
    }

    size_t bytesPerPixel = pDisplayMode->rasterInfo.colorInfo.bpp / 8u;
    pDisplayMode->rasterInfo.rowSize  = pDisplayMode->rasterInfo.width * bytesPerPixel;
    pDisplayMode->rasterInfo.rowWidth = pDisplayMode->rasterInfo.width;
    pDisplayMode->rasterInfo.size     = pDisplayMode->rasterInfo.rowSize * pDisplayMode->rasterInfo.height;
    if ( bWindowMode )
    {
        stdDisplay_SetAspectRatio(pDisplayMode);
    }

    stdDisplay_g_frontBuffer.type             = VBUFFER_HARDWARE;
    stdDisplay_g_frontBuffer.lockRefCount     = 0;
    stdDisplay_g_frontBuffer.bVideoMemory     = 1;
    stdDisplay_g_frontBuffer.pPixels          = NULL;
    stdDisplay_g_frontBuffer.rasterInfo       = pDisplayMode->rasterInfo;

    // Render into a separate multisampled target. The swap-chain itself remains
    // non-multisampled and lockable so CPU and GDI writes have a valid surface.
    if ( stdDisplay_bMSAAEnabled )
    {
        hr = IDirect3DDevice9_CreateRenderTarget(
            pDevice,
            stdDisplay_g_frontBuffer.surface.desc.Width,
            stdDisplay_g_frontBuffer.surface.desc.Height,
            stdDisplay_g_frontBuffer.surface.desc.Format,
            stdDisplay_msaaSampleType,
            stdDisplay_msaaSampleQuality,
            FALSE,
            &stdDisplay_g_backBuffer.surface.pSysSurface,
            NULL
        );
        if ( FAILED(hr) )
        {
            STDLOG_ERROR("Error %s when creating the multisampled render target.\n", stdDisplay_D3DGetStatus(hr));
            goto error;
        }

        hr = IDirect3DDevice9_GetBackBuffer(pDevice, 0, 0, D3DBACKBUFFER_TYPE_MONO, &stdDisplay_pFrontLockSurf);
        if ( FAILED(hr) )
        {
            STDLOG_ERROR("Error %s when acquiring the front presentation surface.\n", stdDisplay_D3DGetStatus(hr));
            goto error;
        }

        hr = IDirect3DDevice9_GetBackBuffer(pDevice, 0, 0, D3DBACKBUFFER_TYPE_MONO, &stdDisplay_pBackLockSurf);
        if ( FAILED(hr) )
        {
            STDLOG_ERROR("Error %s when acquiring the back presentation surface.\n", stdDisplay_D3DGetStatus(hr));
            goto error;
        }
    }
    else
    {
        hr = IDirect3DDevice9_GetBackBuffer(
            pDevice,
            0,
            0,
            D3DBACKBUFFER_TYPE_MONO,
            &stdDisplay_g_backBuffer.surface.pSysSurface
        );
        if ( FAILED(hr) )
        {
            STDLOG_ERROR("Error %s when getting the render back buffer.\n", stdDisplay_D3DGetStatus(hr));
            goto error;
        }
    }

    hr = IDirect3DSurface9_GetDesc(
        stdDisplay_g_backBuffer.surface.pSysSurface,
        &stdDisplay_g_backBuffer.surface.desc
    );
    if ( FAILED(hr) )
    {
        STDLOG_ERROR("Error %s when getting the render-target description.\n", stdDisplay_D3DGetStatus(hr));
        goto error;
    }

    stdDisplay_g_backBuffer.type         = VBUFFER_HARDWARE;
    stdDisplay_g_backBuffer.lockRefCount = 0;
    stdDisplay_g_backBuffer.bVideoMemory = 1;
    stdDisplay_g_backBuffer.pPixels      = NULL;
    stdDisplay_g_backBuffer.rasterInfo   = pDisplayMode->rasterInfo;
    stdDisplay_g_backBuffer.rasterInfo.width  = stdDisplay_g_backBuffer.surface.desc.Width;
    stdDisplay_g_backBuffer.rasterInfo.height = stdDisplay_g_backBuffer.surface.desc.Height;
    if ( !stdDisplay_GetVideoColorFormat(
        stdDisplay_g_backBuffer.surface.desc.Format,
        &stdDisplay_g_backBuffer.rasterInfo.colorInfo
    ) )
    {
        STDLOG_ERROR("Couldn't get render-target color info for format %d.\n", stdDisplay_g_backBuffer.surface.desc.Format);
        goto error;
    }

    bytesPerPixel = stdDisplay_g_backBuffer.rasterInfo.colorInfo.bpp / 8u;
    stdDisplay_g_backBuffer.rasterInfo.rowSize  = stdDisplay_g_backBuffer.rasterInfo.width * bytesPerPixel;
    stdDisplay_g_backBuffer.rasterInfo.rowWidth = stdDisplay_g_backBuffer.rasterInfo.width;
    stdDisplay_g_backBuffer.rasterInfo.size     = stdDisplay_g_backBuffer.rasterInfo.rowSize
        * stdDisplay_g_backBuffer.rasterInfo.height;

    hr = IDirect3DDevice9_SetRenderTarget(pDevice, 0, stdDisplay_g_backBuffer.surface.pSysSurface);
    if ( FAILED(hr) )
    {
        STDLOG_ERROR("Error %s when selecting the engine render target.\n", stdDisplay_D3DGetStatus(hr));
        goto error;
    }

    return 1;

error:
    stdDisplay_ReleaseBuffers();
    return 0;
}

static void stdDisplay_ReleaseBuffers(void)
{
    stdDisplay_ReleaseBuffersInternal(false);
}

static void stdDisplay_ReleaseBuffersInternal(bool bReleaseOnly)
{
    if ( stdDisplay_hdcBack )
    {
        LPDIRECT3DSURFACE9 pSurface = stdDisplay_bMSAAEnabled
            ? stdDisplay_pBackLockSurf
            : stdDisplay_g_backBuffer.surface.pSysSurface;
        if ( pSurface && !bReleaseOnly )
        {
            IDirect3DSurface9_ReleaseDC(pSurface, stdDisplay_hdcBack);
        }
        stdDisplay_hdcBack = NULL;
    }

    if ( stdDisplay_hdcFront )
    {
        LPDIRECT3DSURFACE9 pSurface = stdDisplay_bMSAAEnabled
            ? stdDisplay_pFrontLockSurf
            : stdDisplay_g_frontBuffer.surface.pSysSurface;
        if ( pSurface && !bReleaseOnly )
        {
            IDirect3DSurface9_ReleaseDC(pSurface, stdDisplay_hdcFront);
        }
        stdDisplay_hdcFront = NULL;
    }

    if ( stdDisplay_backLockedRect.pBits )
    {
        LPDIRECT3DSURFACE9 pSurface = stdDisplay_bMSAAEnabled
            ? stdDisplay_pBackLockSurf
            : stdDisplay_g_backBuffer.surface.pSysSurface;
        if ( pSurface && !bReleaseOnly )
        {
            IDirect3DSurface9_UnlockRect(pSurface);
        }
        STD_ZEROMEM(&stdDisplay_backLockedRect, sizeof(stdDisplay_backLockedRect));
    }

    if ( stdDisplay_zBuffer.pSysSurface )
    {
        if ( !bReleaseOnly )
        {
            IDirect3DDevice9_SetDepthStencilSurface(stdDisplay_pD3DDevice, NULL);
        }
        IDirect3DSurface9_Release(stdDisplay_zBuffer.pSysSurface);
        stdDisplay_zBuffer.pSysSurface = NULL;
    }

    // Remove the device's reference to a custom MSAA render target before releasing it.
    if ( !bReleaseOnly && stdDisplay_pD3DDevice && stdDisplay_g_frontBuffer.surface.pSysSurface )
    {
        IDirect3DDevice9_SetRenderTarget(stdDisplay_pD3DDevice, 0, stdDisplay_g_frontBuffer.surface.pSysSurface);
    }

    if ( stdDisplay_g_backBuffer.surface.pSysSurface )
    {
        IDirect3DSurface9_Release(stdDisplay_g_backBuffer.surface.pSysSurface);
        stdDisplay_g_backBuffer.surface.pSysSurface = NULL;
    }

    if ( stdDisplay_pBackLockSurf )
    {
        IDirect3DSurface9_Release(stdDisplay_pBackLockSurf);
        stdDisplay_pBackLockSurf = NULL;
        stdDisplay_backLockRef   = 0;
    }

    if ( stdDisplay_g_frontBuffer.surface.pSysSurface )
    {
        IDirect3DSurface9_Release(stdDisplay_g_frontBuffer.surface.pSysSurface);
        stdDisplay_g_frontBuffer.surface.pSysSurface = NULL;
    }

    if ( stdDisplay_pFrontLockSurf )
    {
        IDirect3DSurface9_Release(stdDisplay_pFrontLockSurf);
        stdDisplay_pFrontLockSurf = NULL;
        stdDisplay_frontLockRef   = 0;
    }

    memset(&stdDisplay_zBuffer, 0, sizeof(stdDisplay_zBuffer));
    memset(&stdDisplay_g_backBuffer, 0, sizeof(stdDisplay_g_backBuffer));
    memset(&stdDisplay_g_frontBuffer, 0, sizeof(stdDisplay_g_frontBuffer));
    stdDisplay_backLockRef                = 0;
    stdDisplay_backDCRef                  = 0;
    stdDisplay_frontLockRef               = 0;
    stdDisplay_bBackBufferWriteLock       = false;
    stdDisplay_bPresentationBufferDirty   = false;
}

uint8_t* J3DAPI stdDisplay_LockSurface(tVSurface* pVSurf)
{
    if ( stdDisplay_bMSAAEnabled
        && pVSurf == &stdDisplay_g_frontBuffer.surface
        && !stdDisplay_bPresentationBufferDirty
        && !stdDisplay_ResolveBackBuffer() )
    {
        return NULL;
    }

    D3DLOCKED_RECT lockedRect;
    HRESULT hr = IDirect3DSurface9_LockRect(pVSurf->pSysSurface, &lockedRect, NULL, 0);

    if ( SUCCEEDED(hr) )
    {
        /*pVSurf->desc.Pitch = lockedRect.Pitch;
        pVSurf->desc.pBits = lockedRect.pBits;*/
        return (uint8_t*)lockedRect.pBits;
    }

    if ( hr == D3DERR_DEVICELOST || hr == D3DERR_DEVICENOTRESET )
    {
        stdDisplay_bDeviceLost = true;
        return NULL;
    }

    STDLOG_ERROR("Error %s when locking the D3D surface.\n", stdDisplay_D3DGetStatus(hr));
    return 0;
}

int J3DAPI stdDisplay_UnlockSurface(tVSurface* pSurf)
{
    HRESULT hr = IDirect3DSurface9_UnlockRect(pSurf->pSysSurface);
    if ( SUCCEEDED(hr) )
    {
        if ( stdDisplay_bMSAAEnabled && pSurf == &stdDisplay_g_frontBuffer.surface )
        {
            stdDisplay_bPresentationBufferDirty = true;
        }
        return 0;
    }

    if ( hr == D3DERR_DEVICELOST || hr == D3DERR_DEVICENOTRESET )
    {
        stdDisplay_bDeviceLost = true;
        return 1;
    }

    STDLOG_ERROR("Error %s when unlocking the display surface.\n", stdDisplay_D3DGetStatus(hr));
    return 1;
}

void stdDisplay_DisableVSync(bool bDisable)
{
    if ( stdDisplay_bNoSync != bDisable )
    {
        UINT oldPresentationInterval = stdDisplay_presentParams.PresentationInterval;
        stdDisplay_presentParams.PresentationInterval = bDisable
            ? D3DPRESENT_INTERVAL_IMMEDIATE
            : D3DPRESENT_INTERVAL_DEFAULT;

        if ( !stdDisplay_ResetDevice() )
        {
            stdDisplay_presentParams.PresentationInterval = oldPresentationInterval;
            STDLOG_ERROR("stdDisplay_DisableVSync: Error resetting D3D device.\n");
            return;
        }

        stdDisplay_bNoSync = bDisable;
    }
}

static int stdDisplay_ResolveBackBuffer(void)
{
    if ( !stdDisplay_bMSAAEnabled || stdDisplay_bPresentationBufferDirty )
    {
        return 1;
    }

    if ( !stdDisplay_pD3DDevice
        || !stdDisplay_g_backBuffer.surface.pSysSurface
        || !stdDisplay_g_frontBuffer.surface.pSysSurface )
    {
        return 0;
    }

    HRESULT hr = IDirect3DDevice9_StretchRect(
        stdDisplay_pD3DDevice,
        stdDisplay_g_backBuffer.surface.pSysSurface,
        NULL,
        stdDisplay_g_frontBuffer.surface.pSysSurface,
        NULL,
        D3DTEXF_NONE
    );
    if ( SUCCEEDED(hr) )
    {
        return 1;
    }

    if ( hr == D3DERR_DEVICELOST || hr == D3DERR_DEVICENOTRESET )
    {
        stdDisplay_bDeviceLost = true;
        return 0;
    }

    STDLOG_ERROR("Error %s when resolving the multisampled render target.\n", stdDisplay_D3DGetStatus(hr));
    return 0;
}

int stdDisplay_Update(void)
{
    if ( !stdDisplay_pD3DDevice )
    {
        return 1;
    }

    // Check device state first
    HRESULT hr = IDirect3DDevice9_TestCooperativeLevel(stdDisplay_pD3DDevice);
    if ( hr == D3DERR_DEVICELOST )
    {
        stdDisplay_bDeviceLost = true;
        STDLOG_WARNING("Warning: D3D device lost, skipping frame.\n");
        return 0; // Device lost, can't present
    }
    else if ( hr == D3DERR_DEVICENOTRESET || stdDisplay_bDeviceResourcesInvalid )
    {
        // Device needs to be reset
        if ( !stdDisplay_ResetDevice() )
        {
            STDLOG_ERROR("stdDisplay_Update: Failed to reset device!\n");
            return 0;
        }
    }
    else if ( FAILED(hr) )
    {
        STDLOG_ERROR("Error %s when checking D3D device state.\n", stdDisplay_D3DGetStatus(hr));
        return 1;
    }

    bool bPresentingCPUBuffer = stdDisplay_bMSAAEnabled && stdDisplay_bPresentationBufferDirty;
    if ( stdDisplay_bMSAAEnabled && !bPresentingCPUBuffer && !stdDisplay_ResolveBackBuffer() )
    {
        return stdDisplay_bDeviceLost ? 0 : 1;
    }

    hr = IDirect3DDevice9_Present(stdDisplay_pD3DDevice, NULL, NULL, NULL, NULL);

    if ( FAILED(hr) )
    {
        if ( hr == D3DERR_DEVICELOST )
        {
            stdDisplay_bDeviceLost = true;
            return 0; // This is expected, just skip this frame
        }

        STDLOG_ERROR("Error %s when presenting the frame.\n", stdDisplay_D3DGetStatus(hr));
        return 1;
    }

    if ( bPresentingCPUBuffer )
    {
        stdDisplay_bPresentationBufferDirty = false;
    }

    return 0;
}

int J3DAPI stdDisplay_ColorFillSurface(tVSurface* pSurf, uint32_t dwFillColor, const StdRect* pRect)
{
    if ( !stdDisplay_pD3DDevice || !pSurf->pSysSurface )
    {
        return 1;
    }

    RECT rect;
    if ( pRect )
    {
        if ( pRect->right == 0 )
        {
            return 1;
        }
        if ( pRect->bottom == 0 )
        {
            return 1;
        }

        rect.left   = pRect->left;
        rect.top    = pRect->top;
        rect.right  = pRect->right + rect.left;
        rect.bottom = pRect->bottom + pRect->top;
    }
    else
    {
        rect.left   = 0;
        rect.top    = 0;
        rect.right  = pSurf->desc.Width;
        rect.bottom = pSurf->desc.Height;
    }

    HRESULT hr = IDirect3DDevice9_ColorFill(stdDisplay_pD3DDevice, pSurf->pSysSurface, &rect, dwFillColor);

    if ( SUCCEEDED(hr) )
    {
        if ( stdDisplay_bMSAAEnabled && pSurf == &stdDisplay_g_frontBuffer.surface )
        {
            stdDisplay_bPresentationBufferDirty = true;
        }
        return 0;
    }

    if ( hr == D3DERR_DEVICELOST || hr == D3DERR_DEVICENOTRESET )
    {
        stdDisplay_bDeviceLost = true;
        return 1;
    }

    STDLOG_ERROR("Error %s when color filling the surface.\n", stdDisplay_D3DGetStatus(hr));
    return 1;
}

int J3DAPI stdDisplay_BackBufferFill(uint32_t color, const StdRect* pRect)
{
    return stdDisplay_ColorFillSurface(&stdDisplay_g_backBuffer.surface, color, pRect);
}

int J3DAPI stdDisplay_SaveScreen(const char* pFilename)
{
    if ( !stdDisplay_pD3DDevice || !stdDisplay_g_backBuffer.surface.pSysSurface )
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
    {
        return NULL;
    }

    if ( stdDisplay_frontLockRef > 0 )
    {
        stdDisplay_frontLockRef++;
        return stdDisplay_hdcFront;
    }

    // Use separate lockable surface for MSAA, otherwise use backbuffer directly
    LPDIRECT3DSURFACE9 pSurf = stdDisplay_bMSAAEnabled ? stdDisplay_pFrontLockSurf : stdDisplay_g_frontBuffer.surface.pSysSurface;
    if ( !pSurf )
    {
        STDLOG_ERROR("No surface available for locking.\n");
        return NULL;
    }

    if ( stdDisplay_bMSAAEnabled
        && !stdDisplay_bPresentationBufferDirty
        && !stdDisplay_ResolveBackBuffer() )
    {
        return NULL;
    }

    HRESULT hr = IDirect3DSurface9_GetDC(pSurf, &stdDisplay_hdcFront);
    if ( FAILED(hr) )
    {
        STDLOG_ERROR("Error %s when getting DC of front buffer.\n", stdDisplay_D3DGetStatus(hr));
        return NULL;
    }

    stdDisplay_frontLockRef++;
    return stdDisplay_hdcFront;
}

void J3DAPI stdDisplay_ReleaseFrontBufferDC(HDC hdc)
{
    if ( stdDisplay_bOpen && stdDisplay_bModeSet && stdDisplay_frontLockRef == 1 )
    {
        LPDIRECT3DSURFACE9 pSurf = stdDisplay_bMSAAEnabled ? stdDisplay_pFrontLockSurf : stdDisplay_g_frontBuffer.surface.pSysSurface;
        HRESULT hr = IDirect3DSurface9_ReleaseDC(pSurf, hdc);
        if ( FAILED(hr) )
        {
            STDLOG_ERROR("Error %s when releasing DC of front buffer.\n", stdDisplay_D3DGetStatus(hr));
            return;
        }

        stdDisplay_hdcFront = NULL;

        if ( stdDisplay_bMSAAEnabled )
        {
            stdDisplay_bPresentationBufferDirty = true;
        }
    }

    if ( stdDisplay_frontLockRef )
    {
        --stdDisplay_frontLockRef;
    }
}

HDC stdDisplay_GetBackBufferDC(void)
{
    if ( !stdDisplay_bOpen || !stdDisplay_bModeSet )
    {
        return NULL;
    }

    if ( stdDisplay_backDCRef > 0 )
    {
        ++stdDisplay_backDCRef;
        return stdDisplay_hdcBack;
    }

    // Use separate lockable surface for MSAA, otherwise use backbuffer directly
    LPDIRECT3DSURFACE9 pSysSurf = stdDisplay_bMSAAEnabled ? stdDisplay_pBackLockSurf : stdDisplay_g_backBuffer.surface.pSysSurface;
    if ( !pSysSurf )
    {
        STDLOG_ERROR("No surface available for locking.\n");
        return NULL;
    }

    if ( stdDisplay_bMSAAEnabled
        && !stdDisplay_bPresentationBufferDirty
        && !stdDisplay_ResolveBackBuffer() )
    {
        return NULL;
    }

    HRESULT hr = IDirect3DSurface9_GetDC(pSysSurf, &stdDisplay_hdcBack);
    if ( FAILED(hr) )
    {
        STDLOG_ERROR("Error %s when getting DC of back buffer.\n", stdDisplay_D3DGetStatus(hr));
        return NULL;
    }

    ++stdDisplay_backDCRef;
    return stdDisplay_hdcBack;
}

void J3DAPI stdDisplay_ReleaseBackBufferDC(HDC hdc)
{
    if ( stdDisplay_bOpen && stdDisplay_bModeSet && stdDisplay_backDCRef == 1 )
    {
        LPDIRECT3DSURFACE9 pSysSurf = stdDisplay_bMSAAEnabled ? stdDisplay_pBackLockSurf : stdDisplay_g_backBuffer.surface.pSysSurface;
        HRESULT hr = IDirect3DSurface9_ReleaseDC(pSysSurf, hdc);
        if ( FAILED(hr) )
        {
            STDLOG_ERROR("Error %s when releasing DC of back buffer.\n", stdDisplay_D3DGetStatus(hr));
            return;
        }

        stdDisplay_hdcBack = NULL;

        if ( stdDisplay_bMSAAEnabled )
        {
            stdDisplay_bPresentationBufferDirty = true;
        }
    }

    if ( stdDisplay_backDCRef )
    {
        --stdDisplay_backDCRef;
    }
}

int stdDisplay_FlipToGDISurface(void)
{
    // DirectX 9 doesn't have a direct equivalent to FlipToGDISurface
    // TODO: Do something about this and can be removed after GDI stuff is out
    return S_OK;
}

int J3DAPI stdDisplay_CanRenderWindowed(void)
{
    if ( !stdDisplay_pD3D9 )
    {
        return -1;
    }

    UINT adapter = stdDisplay_numDevices > 0 ? stdDisplay_curAdapter : D3DADAPTER_DEFAULT;

    // Check if device supports windowed mode
    D3DDISPLAYMODE displayMode;
    HRESULT hr = IDirect3D9_GetAdapterDisplayMode(stdDisplay_pD3D9, adapter, &displayMode);
    if ( FAILED(hr) )
    {
        return -1;
    }

    // Check if we can create a device in windowed mode
    hr = IDirect3D9_CheckDeviceType(stdDisplay_pD3D9, adapter, D3DDEVTYPE_HAL, displayMode.Format, displayMode.Format, TRUE);
    return SUCCEEDED(hr) ? 1 : 0;
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

int J3DAPI stdDisplay_LockBackBuffer(void** pSurface, uint32_t* pWidth, uint32_t* pHeight, int32_t* pPitch)
{
    return stdDisplay_LockBackBufferInternal(pSurface, pWidth, pHeight, pPitch, true);
}

int stdDisplay_LockBackBufferReadOnly(void** pSurface, uint32_t* pWidth, uint32_t* pHeight, int32_t* pPitch)
{
    return stdDisplay_LockBackBufferInternal(pSurface, pWidth, pHeight, pPitch, false);
}

static int stdDisplay_LockBackBufferInternal(void** pSurface, uint32_t* pWidth, uint32_t* pHeight, int32_t* pPitch, bool bWrite)
{
    if ( !stdDisplay_bOpen || !stdDisplay_bModeSet )
    {
        return 1;
    }

    if ( stdDisplay_backLockRef > 0 )
    {
        if ( bWrite && !stdDisplay_bBackBufferWriteLock )
        {
            STDLOG_ERROR("Cannot upgrade an active read-only back-buffer lock to a write lock.\n");
            return 1;
        }
        stdDisplay_backLockRef++;
        *pWidth   = stdDisplay_g_backBuffer.surface.desc.Width;
        *pHeight  = stdDisplay_g_backBuffer.surface.desc.Height;
        *pPitch   = stdDisplay_backLockedRect.Pitch;
        *pSurface = stdDisplay_backLockedRect.pBits;
        return 0;
    }

    // Use separate lockable surface for MSAA, otherwise use backbuffer directly
    LPDIRECT3DSURFACE9 pSysSurf = stdDisplay_bMSAAEnabled ? stdDisplay_pBackLockSurf : stdDisplay_g_backBuffer.surface.pSysSurface;
    if ( !pSysSurf )
    {
        STDLOG_ERROR("No surface available for locking.\n");
        return 1;
    }

    if ( stdDisplay_bMSAAEnabled
        && !stdDisplay_bPresentationBufferDirty
        && !stdDisplay_ResolveBackBuffer() )
    {
        return 1;
    }

    DWORD flags = bWrite ? 0 : D3DLOCK_READONLY;
    HRESULT hr = IDirect3DSurface9_LockRect(pSysSurf, &stdDisplay_backLockedRect, NULL, flags);
    if ( SUCCEEDED(hr) )
    {
        stdDisplay_backLockRef++;
        stdDisplay_bBackBufferWriteLock = bWrite;
        *pWidth   = stdDisplay_g_backBuffer.surface.desc.Width;
        *pHeight  = stdDisplay_g_backBuffer.surface.desc.Height;
        *pPitch   = stdDisplay_backLockedRect.Pitch;
        *pSurface = stdDisplay_backLockedRect.pBits;
        return 0;
    }

    if ( hr == D3DERR_DEVICELOST || hr == D3DERR_DEVICENOTRESET )
    {
        stdDisplay_bDeviceLost = true;
        return 1;
    }

    STDLOG_ERROR("Error %s when locking back buffer.\n", stdDisplay_D3DGetStatus(hr));
    return 1;
}

void stdDisplay_UnlockBackBuffer(void)
{
    if ( stdDisplay_bOpen && stdDisplay_bModeSet && stdDisplay_backLockRef == 1 )
    {
        LPDIRECT3DSURFACE9 pSurface = stdDisplay_bMSAAEnabled ? stdDisplay_pBackLockSurf : stdDisplay_g_backBuffer.surface.pSysSurface;
        if ( pSurface )
        {
            HRESULT hr = IDirect3DSurface9_UnlockRect(pSurface);
            if ( FAILED(hr) )
            {
                if ( hr == D3DERR_DEVICELOST || hr == D3DERR_DEVICENOTRESET )
                {
                    stdDisplay_bDeviceLost = true;
                }
                STDLOG_ERROR("Error %s when unlocking back buffer.\n", stdDisplay_D3DGetStatus(hr));
                return;
            }

            if ( stdDisplay_bMSAAEnabled && stdDisplay_bBackBufferWriteLock )
            {
                stdDisplay_bPresentationBufferDirty = true;
            }

            stdDisplay_bBackBufferWriteLock = false;
            STD_ZEROMEM(&stdDisplay_backLockedRect, sizeof(stdDisplay_backLockedRect));
        }
    }

    if ( stdDisplay_backLockRef )
    {
        --stdDisplay_backLockRef;
    }
}

uint32_t J3DAPI stdDisplay_EncodeFromRGB565(uint16_t pixel)
{
    ColorInfo colorInfo;
    memcpy(&colorInfo, &stdDisplay_pCurVideoMode->rasterInfo.colorInfo, sizeof(colorInfo));

    // Mask each RGB565 component before expanding it to 8-bit so neighboring channels cannot bleed through.
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

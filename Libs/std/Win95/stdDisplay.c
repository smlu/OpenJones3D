#include "stdDisplay.h"
#include "stdWin95.h"

#include <j3dcore/j3dhook.h>
#include <std/General/std.h>
#include <std/General/stdBmp.h>
#include <std/General/stdColor.h>
#include <std/General/stdMemory.h>
#include <std/General/stdUtil.h>
#include <std/RTI/symbols.h>

static bool stdDisplay_bStartup    = false;
static bool stdDisplay_bOpen       = false;
static bool stdDisplay_bModeSet    = false;
static bool stdDisplay_bFullscreen = false;
static bool stdDisplay_bNoSync     = false;

static LPDIRECTDRAW4 stdDisplay_lpDD;

static int stdDisplay_coopLevelFlags = DDSCL_NORMAL;
static int stdDisplay_backbufWidth   = 0;
static int stdDisplay_backbufHeight  = 0;
static tVSurface stdDisplay_zBuffer;

static StdVideoMode* stdDisplay_pCurVideoMode   = NULL;
static StdVideoMode stdDisplay_primaryVideoMode = { 0 };

static size_t stdDisplay_numVideoModes          = 0;
static StdVideoMode stdDisplay_aVideoModes[64]  = { 0 };

static size_t stdDisplay_curDevice;
static StdDisplayDevice* stdDisplay_pCurDevice = NULL;

static size_t stdDisplay_numDevices = 0;
static StdDisplayDevice stdDisplay_aDisplayDevices[16] = { 0 };

static HFONT stdDisplay_hFont;

static int stdDisplay_dword_5D73D8;
static int stdDisplay_dword_5D73DC;

static const DXStatus stdDisplay_aDDStatusTbl[111] =
{
    { DD_OK,                                      "DD_OK"                                    },
    { DDERR_ALREADYINITIALIZED,                   "DDERR_ALREADYINITIALIZED"                 },
    { DDERR_CANNOTATTACHSURFACE,                  "DDERR_CANNOTATTACHSURFACE"                },
    { DDERR_CANNOTDETACHSURFACE,                  "DDERR_CANNOTDETACHSURFACE"                },
    { DDERR_CURRENTLYNOTAVAIL,                    "DDERR_CURRENTLYNOTAVAIL"                  },
    { DDERR_EXCEPTION,                            "DDERR_EXCEPTION"                          },
    { DDERR_GENERIC,                              "DDERR_GENERIC"                            },
    { DDERR_HEIGHTALIGN,                          "DDERR_HEIGHTALIGN"                        },
    { DDERR_INCOMPATIBLEPRIMARY,                  "DDERR_INCOMPATIBLEPRIMARY"                },
    { DDERR_INVALIDCAPS,                          "DDERR_INVALIDCAPS"                        },
    { DDERR_INVALIDCLIPLIST,                      "DDERR_INVALIDCLIPLIST"                    },
    { DDERR_INVALIDMODE,                          "DDERR_INVALIDMODE"                        },
    { DDERR_INVALIDOBJECT,                        "DDERR_INVALIDOBJECT"                      },
    { DDERR_INVALIDPARAMS,                        "DDERR_INVALIDPARAMS"                      },
    { DDERR_INVALIDPIXELFORMAT,                   "DDERR_INVALIDPIXELFORMAT"                 },
    { DDERR_INVALIDRECT,                          "DDERR_INVALIDRECT"                        },
    { DDERR_LOCKEDSURFACES,                       "DDERR_LOCKEDSURFACES"                     },
    { DDERR_NO3D,                                 "DDERR_NO3D"                               },
    { DDERR_NOALPHAHW,                            "DDERR_NOALPHAHW"                          },
    { DDERR_NOCLIPLIST,                           "DDERR_NOCLIPLIST"                         },
    { DDERR_NOCOLORCONVHW,                        "DDERR_NOCOLORCONVHW"                      },
    { DDERR_NOCOOPERATIVELEVELSET,                "DDERR_NOCOOPERATIVELEVELSET"              },
    { DDERR_NOCOLORKEY,                           "DDERR_NOCOLORKEY"                         },
    { DDERR_NOCOLORKEYHW,                         "DDERR_NOCOLORKEYHW"                       },
    { DDERR_NODIRECTDRAWSUPPORT,                  "DDERR_NODIRECTDRAWSUPPORT"                },
    { DDERR_NOEXCLUSIVEMODE,                      "DDERR_NOEXCLUSIVEMODE"                    },
    { DDERR_NOFLIPHW,                             "DDERR_NOFLIPHW"                           },
    { DDERR_NOGDI,                                "DDERR_NOGDI"                              },
    { DDERR_NOMIRRORHW,                           "DDERR_NOMIRRORHW"                         },
    { DDERR_NOTFOUND,                             "DDERR_NOTFOUND"                           },
    { DDERR_NOOVERLAYHW,                          "DDERR_NOOVERLAYHW"                        },
    { DDERR_OVERLAPPINGRECTS,                     "DDERR_OVERLAPPINGRECTS"                   },
    { DDERR_NORASTEROPHW,                         "DDERR_NORASTEROPHW"                       },
    { DDERR_NOROTATIONHW,                         "DDERR_NOROTATIONHW"                       },
    { DDERR_NOSTRETCHHW,                          "DDERR_NOSTRETCHHW"                        },
    { DDERR_NOT4BITCOLOR,                         "DDERR_NOT4BITCOLOR"                       },
    { DDERR_NOT4BITCOLORINDEX,                    "DDERR_NOT4BITCOLORINDEX"                  },
    { DDERR_NOT8BITCOLOR,                         "DDERR_NOT8BITCOLOR"                       },
    { DDERR_NOTEXTUREHW,                          "DDERR_NOTEXTUREHW"                        },
    { DDERR_NOVSYNCHW,                            "DDERR_NOVSYNCHW"                          },
    { DDERR_NOZBUFFERHW,                          "DDERR_NOZBUFFERHW"                        },
    { DDERR_NOZOVERLAYHW,                         "DDERR_NOZOVERLAYHW"                       },
    { DDERR_OUTOFCAPS,                            "DDERR_OUTOFCAPS"                          },
    { DDERR_OUTOFMEMORY,                          "DDERR_OUTOFMEMORY"                        },
    { DDERR_OUTOFVIDEOMEMORY,                     "DDERR_OUTOFVIDEOMEMORY"                   },
    { DDERR_OVERLAYCANTCLIP,                      "DDERR_OVERLAYCANTCLIP"                    },
    { DDERR_OVERLAYCOLORKEYONLYONEACTIVE,         "DDERR_OVERLAYCOLORKEYONLYONEACTIVE"       },
    { DDERR_PALETTEBUSY,                          "DDERR_PALETTEBUSY"                        },
    { DDERR_COLORKEYNOTSET,                       "DDERR_COLORKEYNOTSET"                     },
    { DDERR_SURFACEALREADYATTACHED,               "DDERR_SURFACEALREADYATTACHED"             },
    { DDERR_SURFACEALREADYDEPENDENT,              "DDERR_SURFACEALREADYDEPENDENT"            },
    { DDERR_SURFACEBUSY,                          "DDERR_SURFACEBUSY"                        },
    { DDERR_CANTLOCKSURFACE,                      "DDERR_CANTLOCKSURFACE"                    },
    { DDERR_SURFACEISOBSCURED,                    "DDERR_SURFACEISOBSCURED"                  },
    { DDERR_SURFACELOST,                          "DDERR_SURFACELOST"                        },
    { DDERR_SURFACENOTATTACHED,                   "DDERR_SURFACENOTATTACHED"                 },
    { DDERR_TOOBIGHEIGHT,                         "DDERR_TOOBIGHEIGHT"                       },
    { DDERR_TOOBIGSIZE,                           "DDERR_TOOBIGSIZE"                         },
    { DDERR_TOOBIGWIDTH,                          "DDERR_TOOBIGWIDTH"                        },
    { DDERR_UNSUPPORTED,                          "DDERR_UNSUPPORTED"                        },
    { DDERR_UNSUPPORTEDFORMAT,                    "DDERR_UNSUPPORTEDFORMAT"                  },
    { DDERR_UNSUPPORTEDMASK,                      "DDERR_UNSUPPORTEDMASK"                    },
    { DDERR_INVALIDSTREAM,                        "DDERR_INVALIDSTREAM"                      },
    { DDERR_VERTICALBLANKINPROGRESS,              "DDERR_VERTICALBLANKINPROGRESS"            },
    { DDERR_WASSTILLDRAWING,                      "DDERR_WASSTILLDRAWING"                    },
    { DDERR_XALIGN,                               "DDERR_XALIGN"                             },
    { DDERR_INVALIDDIRECTDRAWGUID,                "DDERR_INVALIDDIRECTDRAWGUID"              },
    { DDERR_DIRECTDRAWALREADYCREATED,             "DDERR_DIRECTDRAWALREADYCREATED"           },
    { DDERR_NODIRECTDRAWHW,                       "DDERR_NODIRECTDRAWHW"                     },
    { DDERR_PRIMARYSURFACEALREADYEXISTS,          "DDERR_PRIMARYSURFACEALREADYEXISTS"        },
    { DDERR_NOEMULATION,                          "DDERR_NOEMULATION"                        },
    { DDERR_REGIONTOOSMALL,                       "DDERR_REGIONTOOSMALL"                     },
    { DDERR_CLIPPERISUSINGHWND,                   "DDERR_CLIPPERISUSINGHWND"                 },
    { DDERR_NOCLIPPERATTACHED,                    "DDERR_NOCLIPPERATTACHED"                  },
    { DDERR_NOHWND,                               "DDERR_NOHWND"                             },
    { DDERR_HWNDSUBCLASSED,                       "DDERR_HWNDSUBCLASSED"                     },
    { DDERR_HWNDALREADYSET,                       "DDERR_HWNDALREADYSET"                     },
    { DDERR_NOPALETTEATTACHED,                    "DDERR_NOPALETTEATTACHED"                  },
    { DDERR_NOPALETTEHW,                          "DDERR_NOPALETTEHW"                        },
    { DDERR_BLTFASTCANTCLIP,                      "DDERR_BLTFASTCANTCLIP"                    },
    { DDERR_NOBLTHW,                              "DDERR_NOBLTHW"                            },
    { DDERR_NODDROPSHW,                           "DDERR_NODDROPSHW"                         },
    { DDERR_OVERLAYNOTVISIBLE,                    "DDERR_OVERLAYNOTVISIBLE"                  },
    { DDERR_NOOVERLAYDEST,                        "DDERR_NOOVERLAYDEST"                      },
    { DDERR_INVALIDPOSITION,                      "DDERR_INVALIDPOSITION"                    },
    { DDERR_NOTAOVERLAYSURFACE,                   "DDERR_NOTAOVERLAYSURFACE"                 },
    { DDERR_EXCLUSIVEMODEALREADYSET,              "DDERR_EXCLUSIVEMODEALREADYSET"            },
    { DDERR_NOTFLIPPABLE,                         "DDERR_NOTFLIPPABLE"                       },
    { DDERR_CANTDUPLICATE,                        "DDERR_CANTDUPLICATE"                      },
    { DDERR_NOTLOCKED,                            "DDERR_NOTLOCKED"                          },
    { DDERR_CANTCREATEDC,                         "DDERR_CANTCREATEDC"                       },
    { DDERR_NODC,                                 "DDERR_NODC"                               },
    { DDERR_WRONGMODE,                            "DDERR_WRONGMODE"                          },
    { DDERR_IMPLICITLYCREATED,                    "DDERR_IMPLICITLYCREATED"                  },
    { DDERR_NOTPALETTIZED,                        "DDERR_NOTPALETTIZED"                      },
    { DDERR_UNSUPPORTEDMODE,                      "DDERR_UNSUPPORTEDMODE"                    },
    { DDERR_NOMIPMAPHW,                           "DDERR_NOMIPMAPHW"                         },
    { DDERR_INVALIDSURFACETYPE,                   "DDERR_INVALIDSURFACETYPE"                 },
    { DDERR_NOOPTIMIZEHW,                         "DDERR_NOOPTIMIZEHW"                       },
    { DDERR_NOTLOADED,                            "DDERR_NOTLOADED"                          },
    { DDERR_NOFOCUSWINDOW,                        "DDERR_NOFOCUSWINDOW"                      },
    { DDERR_DCALREADYCREATED,                     "DDERR_DCALREADYCREATED"                   },
    { DDERR_NONONLOCALVIDMEM,                     "DDERR_NONONLOCALVIDMEM"                   },
    { DDERR_CANTPAGELOCK,                         "DDERR_CANTPAGELOCK"                       },
    { DDERR_CANTPAGEUNLOCK,                       "DDERR_CANTPAGEUNLOCK"                     },
    { DDERR_NOTPAGELOCKED,                        "DDERR_NOTPAGELOCKED"                      },
    { DDERR_MOREDATA,                             "DDERR_MOREDATA"                           },
    { DDERR_EXPIRED,                              "DDERR_EXPIRED"                            },
    { DDERR_VIDEONOTACTIVE,                       "DDERR_VIDEONOTACTIVE"                     },
    { DDERR_DEVICEDOESNTOWNSURFACE,               "DDERR_DEVICEDOESNTOWNSURFACE"             },
    { DDERR_NOTINITIALIZED,                       "DDERR_NOTINITIALIZED"                     }
};

static int J3DAPI stdDisplay_VideoModeCompare(const StdVideoMode* pMode1, const StdVideoMode* pMode2);
static const char* J3DAPI stdDisplay_DDGetStatus(HRESULT status);

static int J3DAPI stdDisplay_InitDirectDraw(HWND hwnd);
static void J3DAPI stdDisplay_ReleaseDirectDraw();
static BOOL PASCAL stdDisplay_DDEnumCallback(GUID* lpGUID, LPSTR szDriverName, LPSTR szDriverDescription, LPVOID lpContext);
static HRESULT PASCAL stdDisplay_EnumVideoModesCallback(LPDDSURFACEDESC2 lpDDSurfaceDesc, LPVOID lpContext);

static inline void J3DAPI stdDisplay_SetAspectRatio(StdVideoMode* pMode);
static inline int J3DAPI stdDisplay_SetWindowMode(HWND hWnd, StdVideoMode* pDisplayMode);
static inline int J3DAPI stdDisplay_SetFullscreenMode(HWND hwnd, const StdVideoMode* pDisplayMode, size_t numBackBuffers); // numBackBuffers - Specifies the number of back buffers associated with the surface

static inline void J3DAPI stdDisplay_ReleaseBuffers();
static inline uint8_t* J3DAPI stdDisplay_LockSurface(tVSurface* pVSurf);
static inline int J3DAPI stdDisplay_UnlockSurface(tVSurface* pSurf);
static int J3DAPI stdDisplay_ColorFillSurface(tVSurface* pSurf, uint32_t dwFillColor, const StdRect* lpRect);

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
    J3D_HOOKFUNC(stdDisplay_DDGetStatus);
    J3D_HOOKFUNC(stdDisplay_CreateZBuffer);
    J3D_HOOKFUNC(stdDisplay_InitDirectDraw);
    J3D_HOOKFUNC(stdDisplay_ReleaseDirectDraw);
    J3D_HOOKFUNC(stdDisplay_DDEnumCallback);
    J3D_HOOKFUNC(stdDisplay_EnumVideoModesCallback);
    J3D_HOOKFUNC(stdDisplay_SetAspectRatio);
    J3D_HOOKFUNC(stdDisplay_GetDirectDraw);
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
    /* const DXStatus stdDisplay_aDDStatusTbl_tmp[111] = {
     memcpy((DXStatus*)&stdDisplay_aDDStatusTbl, &stdDisplay_aDDStatusTbl_tmp, sizeof(stdDisplay_aDDStatusTbl));

     int stdDisplay_coopLevelFlags_tmp = 8;
     memcpy(&stdDisplay_coopLevelFlags, &stdDisplay_coopLevelFlags_tmp, sizeof(stdDisplay_coopLevelFlags));
     memset(&stdDisplay_hFont, 0, sizeof(stdDisplay_hFont));
     memset(&stdDisplay_curDevice, 0, sizeof(stdDisplay_curDevice));
     memset(&stdDisplay_primaryVideoMode, 0, sizeof(stdDisplay_primaryVideoMode));
     memset(&stdDisplay_dword_5D73D8, 0, sizeof(stdDisplay_dword_5D73D8));
     memset(&stdDisplay_dword_5D73DC, 0, sizeof(stdDisplay_dword_5D73DC));
     memset(&stdDisplay_backbufWidth, 0, sizeof(stdDisplay_backbufWidth));
     memset(&stdDisplay_backbufHeight, 0, sizeof(stdDisplay_backbufHeight));
     memset(&stdDisplay_zBuffer, 0, sizeof(stdDisplay_zBuffer));
     memset(&stdDisplay_aVideoModes, 0, sizeof(stdDisplay_aVideoModes));
     memset(&stdDisplay_aDisplayDevices, 0, sizeof(stdDisplay_aDisplayDevices));
     memset(&stdDisplay_bStartup, 0, sizeof(stdDisplay_bStartup));
     memset(&stdDisplay_bOpen, 0, sizeof(stdDisplay_bOpen));
     memset(&stdDisplay_bModeSet, 0, sizeof(stdDisplay_bModeSet));
     memset(&stdDisplay_numDevices, 0, sizeof(stdDisplay_numDevices));
     memset(&stdDisplay_pCurDevice, 0, sizeof(stdDisplay_pCurDevice));
     memset(&stdDisplay_numVideoModes, 0, sizeof(stdDisplay_numVideoModes));
     memset(&stdDisplay_pCurVideoMode, 0, sizeof(stdDisplay_pCurVideoMode));
     memset(&stdDisplay_lpDD, 0, sizeof(stdDisplay_lpDD));
     memset(&stdDisplay_bFullscreen, 0, sizeof(stdDisplay_bFullscreen));*/

    memset(&stdDisplay_g_frontBuffer, 0, sizeof(stdDisplay_g_frontBuffer));
    memset(&stdDisplay_g_backBuffer, 0, sizeof(stdDisplay_g_backBuffer));
}

// Added
static void J3DAPI stdDisplay_SetPixels16(uint16_t* pPixels16, uint16_t pixel, size_t size)
{
    int v3;
    uint16_t* pCurPixel;
    unsigned int count;
    int v6;

    pCurPixel = pPixels16;
    count = size;
    if ( (size & 1) != 0 )
    {
        while ( count )
        {
            *pCurPixel++ = pixel;
            --count;
        }
    }
    else
    {
        // The size is multiple of 2 so we can copy 2 pixels (32 bit) at a time
        v3 = (uint16_t)(pixel & 0xFFFFU);
        v6 = v3 << 16;
        v6 |= (uint16_t)(pixel & 0xFFFFU);
        memset(pPixels16, v6, (size / 2) * sizeof(int)); // Note, divide by 2 because 2 copy 2 uint16_t at a time in form of 32 bit number 
    }
}

// Added
static void J3DAPI stdDisplay_SetPixels32(uint32_t* pPixels32, uint32_t pixel, size_t size)
{
    memset(pPixels32, pixel, size * sizeof(pixel));
}

int stdDisplay_Startup(void)
{
    if ( stdDisplay_bStartup ) {
        return 1;
    }

    memset(&stdDisplay_g_frontBuffer, 0u, sizeof(stdDisplay_g_frontBuffer));
    memset(&stdDisplay_g_backBuffer, 0u, sizeof(stdDisplay_g_backBuffer));
    memset(&stdDisplay_zBuffer, 0u, sizeof(stdDisplay_zBuffer));

    stdDisplay_bStartup   = true;
    stdDisplay_numDevices = 0;

    HRESULT ddres = DirectDrawEnumerate(stdDisplay_DDEnumCallback, NULL);
    if ( ddres != DD_OK )
    {
        STDLOG_ERROR("Error %s when enumerating DDraw devices.\n", stdDisplay_DDGetStatus(ddres));
        return 0;
    }

    // TODO: Change default res
    stdDisplay_primaryVideoMode.rasterInfo.width  = 640;
    stdDisplay_primaryVideoMode.rasterInfo.height = 480;

    STDLOG_STATUS("Found %d Display Devices.\n", stdDisplay_numDevices);
    return 1;
}

void stdDisplay_Shutdown(void)
{
    if ( stdDisplay_bOpen ) {
        stdDisplay_Close();
    }

    memset(stdDisplay_aDisplayDevices, 0, sizeof(stdDisplay_aDisplayDevices));
    memset(&stdDisplay_g_frontBuffer, 0, sizeof(stdDisplay_g_frontBuffer));
    memset(&stdDisplay_g_backBuffer, 0, sizeof(stdDisplay_g_backBuffer));
    memset(&stdDisplay_zBuffer, 0, sizeof(stdDisplay_zBuffer));

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

    if ( deviceNum > stdDisplay_numDevices ) {
        return 0;
    }

    stdDisplay_curDevice  = deviceNum;
    stdDisplay_pCurDevice = &stdDisplay_aDisplayDevices[deviceNum];
    if ( !stdDisplay_InitDirectDraw(stdWin95_GetWindow()) ) {
        return 0;
    }

    qsort(
        stdDisplay_aVideoModes,
        stdDisplay_numVideoModes,
        sizeof(StdVideoMode),
        (int(__cdecl*)(const void*, const void*))stdDisplay_VideoModeCompare);

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

    if ( stdDisplay_bModeSet ) {
        stdDisplay_ClearMode();
    }

    stdDisplay_ReleaseDirectDraw();

    memset(&stdDisplay_g_frontBuffer, 0, sizeof(stdDisplay_g_frontBuffer));
    memset(&stdDisplay_g_backBuffer, 0, sizeof(stdDisplay_g_backBuffer));
    memset(&stdDisplay_zBuffer, 0, sizeof(stdDisplay_zBuffer));

    stdDisplay_curDevice  = 0;
    stdDisplay_pCurDevice = NULL;
    stdDisplay_bOpen      = false;
}

int J3DAPI stdDisplay_SetMode(size_t modeNum, int bFullscreen, size_t numBackBuffers)
{
    if ( bFullscreen && modeNum >= stdDisplay_numVideoModes ) {
        return 1;
    }

    if ( stdDisplay_bModeSet ) {
        stdDisplay_ClearMode();
    }

    if ( bFullscreen )
    {
        stdDisplay_pCurVideoMode = &stdDisplay_aVideoModes[modeNum];
        HWND hwnd = stdWin95_GetWindow();
        if ( stdDisplay_SetFullscreenMode(hwnd, &stdDisplay_aVideoModes[modeNum], numBackBuffers) ) {
            return 1;
        }
    }
    else
    {
        stdDisplay_pCurVideoMode = &stdDisplay_primaryVideoMode;
        HWND hwnd = stdWin95_GetWindow();
        if ( !stdDisplay_SetWindowMode(hwnd, &stdDisplay_primaryVideoMode) )
        {
            return 1;
        }
    }

    int fheight = -(stdDisplay_pCurVideoMode->rasterInfo.width < 640);
    fheight = fheight & 0xF4;
    stdDisplay_hFont = CreateFontA(fheight + 24, 0, 0, 0, FW_NORMAL, 0, 0, 0, ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, VARIABLE_PITCH, "Arial");

    stdDisplay_dword_5D73D8  = 0;
    stdDisplay_dword_5D73DC  = 0;
    stdDisplay_backbufWidth  = stdDisplay_g_backBuffer.rasterInfo.width;
    stdDisplay_backbufHeight = stdDisplay_g_backBuffer.rasterInfo.height;
    stdDisplay_bModeSet      = true;
    stdDisplay_bFullscreen   = bFullscreen;

    stdDisplay_VBufferFill(&stdDisplay_g_backBuffer, 0, NULL); // 0 - BLACK 
    stdDisplay_Update();

    if ( bFullscreen )
    {
        // TODO: Why filling back buffer again? Maybe front buffer should be filled
        stdDisplay_VBufferFill(&stdDisplay_g_backBuffer, 0, NULL); // 0 - BLACK 
    }

    return 0;
}

void stdDisplay_ClearMode(void)
{
    if ( stdDisplay_bModeSet ) {
        stdDisplay_ReleaseBuffers();
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

    memcpy(pDest, &stdDisplay_aDisplayDevices[deviceNum], sizeof(StdDisplayDevice));
    return 0;
}

int J3DAPI stdDisplay_GetCurrentDevice(StdDisplayDevice* pDevice)
{
    if ( stdDisplay_numDevices == 0 ) {
        return 1;
    }

    memcpy(pDevice, &stdDisplay_aDisplayDevices[stdDisplay_curDevice], sizeof(StdDisplayDevice));
    return 0;
}

void J3DAPI stdDisplay_Refresh(int bReload)
{
    if ( stdDisplay_bOpen && stdDisplay_bModeSet && bReload )
    {
        HRESULT ddres = IDirectDraw4_SetCooperativeLevel(stdDisplay_lpDD, stdWin95_GetWindow(), stdDisplay_coopLevelFlags);
        if ( ddres != DD_OK )
        {
            STDLOG_ERROR("Error %s when setting cooperative level.\n", stdDisplay_DDGetStatus(ddres));
            return;
        }

        if ( stdDisplay_bFullscreen )
        {
            if ( stdDisplay_lpDD )
            {
                ddres = IDirectDraw4_SetDisplayMode(
                    stdDisplay_lpDD,
                    stdDisplay_pCurVideoMode->rasterInfo.width,
                    stdDisplay_pCurVideoMode->rasterInfo.height,
                    stdDisplay_pCurVideoMode->rasterInfo.colorInfo.bpp,
                    0,
                    0
                );
                if ( ddres != DD_OK )
                {
                    STDLOG_ERROR("Error %s on set display Mode front buffer.\n", stdDisplay_DDGetStatus(ddres));
                    return;
                }
            }

            if ( stdDisplay_g_frontBuffer.surface.pDDSurf )
            {
                ddres = IDirectDrawSurface4_Restore(stdDisplay_g_frontBuffer.surface.pDDSurf);
                if ( ddres != DD_OK )
                {
                    STDLOG_ERROR("Error %s on restore front buffer.\n", stdDisplay_DDGetStatus(ddres));
                    return;
                }
            }
        }
        else if ( stdDisplay_g_backBuffer.surface.pDDSurf )
        {
            ddres = IDirectDrawSurface4_Restore(stdDisplay_g_backBuffer.surface.pDDSurf);
            if ( ddres != DD_OK )
            {
                STDLOG_ERROR("Error %s on restore front buffer.\n", stdDisplay_DDGetStatus(ddres));
                return;
            }
        }

        if ( stdDisplay_zBuffer.pDDSurf )
        {
            ddres = IDirectDrawSurface4_Restore(stdDisplay_zBuffer.pDDSurf);
            if ( ddres != DD_OK )
            {
                STDLOG_ERROR("Error %s on restore zBuffer.\n", stdDisplay_DDGetStatus(ddres));
            }
        }
    }
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

    vbuffer->pPixels          = NULL;
    vbuffer->lockSurfRefCount = 0;
    memcpy(&vbuffer->rasterInfo, pRasterInfo, sizeof(vbuffer->rasterInfo));

    vbuffer->unknown1 = 0;

    unsigned int bbp             = (unsigned int)vbuffer->rasterInfo.colorInfo.bpp / 8;
    vbuffer->rasterInfo.rowSize  = vbuffer->rasterInfo.width * bbp;
    vbuffer->rasterInfo.rowWidth = vbuffer->rasterInfo.width * bbp / bbp;
    vbuffer->rasterInfo.size     = vbuffer->rasterInfo.height * vbuffer->rasterInfo.width * bbp;

    if ( bUseVSurface && stdDisplay_bOpen )
    {
        vbuffer->bVideoMemory = 0;
        vbuffer->lockRefCount = 1;

        memset(&vbuffer->surface.ddSurfDesc, 0, sizeof(vbuffer->surface.ddSurfDesc));
        vbuffer->surface.ddSurfDesc.dwSize = sizeof(DDSURFACEDESC2);
        vbuffer->surface.ddSurfDesc.dwFlags = DDSD_WIDTH | DDSD_HEIGHT | DDSD_CAPS;
        vbuffer->surface.ddSurfDesc.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN;

        if ( !bUseVideoMemory ) {
            vbuffer->surface.ddSurfDesc.ddsCaps.dwCaps = DDSCAPS_SYSTEMMEMORY | DDSCAPS_OFFSCREENPLAIN;
        }

        vbuffer->surface.ddSurfDesc.dwWidth  = vbuffer->rasterInfo.width;
        vbuffer->surface.ddSurfDesc.dwHeight = vbuffer->rasterInfo.height;
        HRESULT ddres = IDirectDraw4_CreateSurface(stdDisplay_lpDD, &vbuffer->surface.ddSurfDesc, &vbuffer->surface.pDDSurf, NULL);
        if ( ddres != DD_OK )
        {
            STDLOG_ERROR("Error %s when creating a DirectDraw vbuffer surface.\n", stdDisplay_DDGetStatus(ddres));
            return NULL;
        }

        ddres = IDirectDrawSurface4_GetSurfaceDesc(vbuffer->surface.pDDSurf, &vbuffer->surface.ddSurfDesc);
        if ( ddres != DD_OK )
        {
            STDLOG_ERROR("Error %s when getting desc of surface.\n", stdDisplay_DDGetStatus(ddres));
            return NULL;
        }

        if ( (vbuffer->surface.ddSurfDesc.ddsCaps.dwCaps & DDSCAPS_VIDEOMEMORY) != 0 ) {
            vbuffer->bVideoMemory = 1;
        }

        vbuffer->rasterInfo.rowSize  = vbuffer->surface.ddSurfDesc.dwLinearSize;
        vbuffer->rasterInfo.rowWidth = vbuffer->surface.ddSurfDesc.dwLinearSize / bbp;
        return vbuffer;
    }
    else
    {
        vbuffer->lockRefCount = 0;
        vbuffer->bVideoMemory = 0;
        vbuffer->pPixels = (uint8_t*)STDMALLOC(vbuffer->rasterInfo.size);
        if ( !vbuffer->pPixels ) {
            return NULL;
        }

        vbuffer->lockSurfRefCount = 1;
        return vbuffer;
    }
}

void J3DAPI stdDisplay_VBufferFree(tVBuffer* pVBuffer)
{
    STD_ASSERTREL(pVBuffer != ((void*)0));
    if ( pVBuffer->lockRefCount )
    {
        if ( pVBuffer->lockRefCount == 1 )
        {
            if ( pVBuffer->surface.pDDSurf )
            {
                IDirectDrawSurface4_Release(pVBuffer->surface.pDDSurf);
                pVBuffer->surface.pDDSurf = NULL;
            }
        }
    }
    else if ( pVBuffer->pPixels )
    {
        stdMemory_Free(pVBuffer->pPixels);
        pVBuffer->pPixels = 0;
    }

    stdMemory_Free(pVBuffer);
}

int J3DAPI stdDisplay_VBufferLock(tVBuffer* pVBuffer)
{
    STD_ASSERTREL(pVBuffer != ((void*)0));

    if ( pVBuffer->lockRefCount )
    {
        if ( pVBuffer->lockRefCount == 1 )
        {
            if ( (pVBuffer->surface.ddSurfDesc.ddsCaps.dwCaps & DDSCAPS_FRONTBUFFER) != 0
               && (pVBuffer->surface.ddSurfDesc.ddsCaps.dwCaps & DDSCAPS_MODEX) != 0 ) {
                return 0;
            }

            pVBuffer->pPixels = stdDisplay_LockSurface(&pVBuffer->surface);
            if ( !pVBuffer->pPixels ) {
                return 0;
            }

            ++pVBuffer->lockSurfRefCount;
        }
    }
    else
    {
        ++pVBuffer->lockSurfRefCount;
    }

    return 1;
}

int J3DAPI stdDisplay_VBufferUnlock(tVBuffer* pVBuffer)
{
    int result;

    STD_ASSERTREL(pVBuffer != ((void*)0));
    if ( pVBuffer->lockRefCount == 0 )
    {
        if ( pVBuffer->lockSurfRefCount ) {
            --pVBuffer->lockSurfRefCount;
        }

        return 1;
    }

    if ( pVBuffer->lockRefCount != 1 ) {
        return 1;
    }

    if ( pVBuffer->lockSurfRefCount == 0 ) {
        return 0;
    }

    result = stdDisplay_UnlockSurface(&pVBuffer->surface);
    if ( !result ) {
        --pVBuffer->lockSurfRefCount;
    }

    return result;
}

int J3DAPI stdDisplay_VBufferFill(tVBuffer* pVBuffer, uint32_t color, const StdRect* pRect)
{
    STD_ASSERTREL(pVBuffer != ((void*)0));

    if ( pVBuffer->lockRefCount )
    {
        if ( pVBuffer->lockRefCount != 1 )
        {
            // Skip filling because more than 1 refs is using this buffer
            return 1;
        }

        DWORD dwCaps = pVBuffer->surface.ddSurfDesc.ddsCaps.dwCaps;
        return ((dwCaps & DDSCAPS_FRONTBUFFER) == 0 || (dwCaps & DDSCAPS_MODEX) == 0)
            && stdDisplay_ColorFillSurface(&pVBuffer->surface, color, pRect) == 0;
    }

    switch ( pVBuffer->rasterInfo.colorInfo.bpp )
    {
        case 8:
            if ( pRect )
            {
                uint8_t* pPixels8 = &pVBuffer->pPixels[pVBuffer->rasterInfo.rowWidth * pRect->top + pRect->left];
                for ( int32_t height = 0; height < pRect->bottom; ++height )
                {
                    memset(pPixels8, (uint8_t)color, pRect->right);
                    pPixels8 += pVBuffer->rasterInfo.rowSize;
                }
            }
            else {
                memset(pVBuffer->pPixels, (uint8_t)color, pVBuffer->rasterInfo.size);
            }

            break;

        case 16:
            if ( pRect )
            {
                uint16_t* pPixels16 = (uint16_t*)&pVBuffer->pPixels[2 * pVBuffer->rasterInfo.rowWidth * pRect->top + 2 * pRect->left];
                for ( int32_t height = 0; height < pRect->bottom; ++height )
                {
                    stdDisplay_SetPixels16(pPixels16, color, pRect->right);
                    pPixels16 = (uint16_t*)((char*)pPixels16 + pVBuffer->rasterInfo.rowSize);
                }
            }
            else {
                stdDisplay_SetPixels16((uint16_t*)pVBuffer->pPixels, color, (size_t)(pVBuffer->rasterInfo.size / 2));
            }

            break;

        case 24:
            STDLOG_FATAL("0"); // TODO: implement
            break;

        case 32:
            if ( pRect )
            {
                uint32_t* pPixels32 = (uint32_t*)&pVBuffer->pPixels[4 * pVBuffer->rasterInfo.rowWidth * pRect->top + 4 * pRect->left];
                for ( int32_t height = 0; height < pRect->bottom; ++height )
                {
                    stdDisplay_SetPixels32(pPixels32, color, pRect->right);
                    pPixels32 = (uint32_t*)((char*)pPixels32 + pVBuffer->rasterInfo.rowSize);
                }
            }
            else {
                stdDisplay_SetPixels32((uint32_t*)pVBuffer->pPixels, color, (size_t)(pVBuffer->rasterInfo.size / 4));
            }

            break;
    }

    return 1;
}

tVBuffer* J3DAPI stdDisplay_VBufferConvertColorFormat(const ColorInfo* pDesiredColorFormat, tVBuffer* pSrc, int bColorKey, LPDDCOLORKEY pColorKey)
{
    STD_ASSERTREL(pSrc != ((void*)0));
    if ( !memcmp(pDesiredColorFormat, &pSrc->rasterInfo.colorInfo, sizeof(ColorInfo)) ) {
        return pSrc;
    }

    if ( pSrc->rasterInfo.colorInfo.colorMode == STDCOLOR_PAL )
    {
        if ( pDesiredColorFormat->colorMode == STDCOLOR_PAL ) {
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
        pDest = pSrc;
    }
    else
    {
        tRasterInfo rasterInfo;
        memcpy(&rasterInfo, &pSrc->rasterInfo, sizeof(rasterInfo));
        memcpy(&rasterInfo.colorInfo, pDesiredColorFormat, sizeof(rasterInfo.colorInfo));

        pDest = stdDisplay_VBufferNew(&rasterInfo, 0, 0);
        if ( !pDest )
        {
            STDLOG_ERROR("Unable to allocate memory for new tVBuffer", 0, 0, 0, 0);
            return 0;
        }
    }

    STD_ASSERTREL(pSrc->pPixels != ((void*)0));
    STD_ASSERTREL(pDest->pPixels != ((void*)0));

    stdDisplay_VBufferLock(pSrc);
    stdDisplay_VBufferLock(pDest);

    const uint8_t* pSrcRow = NULL;
    uint8_t* pDestRow      = NULL;
    for ( size_t row = 0; row < (signed int)pDest->rasterInfo.height; ++row )
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
            pColorKey);
    }

    STD_ASSERTREL(pDestRow <= pDest->pPixels + pDest->rasterInfo.size);
    STD_ASSERTREL(pSrcRow <= pSrc->pPixels + pSrc->rasterInfo.size);
    stdDisplay_VBufferUnlock(pSrc);
    stdDisplay_VBufferUnlock(pDest);
    memcpy(&pDest->rasterInfo.colorInfo, pDesiredColorFormat, sizeof(pDest->rasterInfo.colorInfo));

    if ( pDest != pSrc ) {
        stdDisplay_VBufferFree(pSrc);
    }

    return pDest;
}

int J3DAPI stdDisplay_VideoModeCompare(const StdVideoMode* pMode1, const StdVideoMode* pMode2)
{
    unsigned int bpp1;
    unsigned int bpp2;

    bpp1 = pMode1->rasterInfo.colorInfo.bpp;
    bpp2 = pMode2->rasterInfo.colorInfo.bpp;
    if ( bpp1 != bpp2 )
    {
        return bpp1 - bpp2;
    }

    bpp1 = pMode1->rasterInfo.width;
    bpp2 = pMode2->rasterInfo.width;
    if ( bpp1 != bpp2 )
    {
        return bpp1 - bpp2;
    }

    bpp1 = pMode1->rasterInfo.height;
    bpp2 = pMode2->rasterInfo.height;
    return bpp1 - bpp2;
}

int J3DAPI stdDisplay_GetTextureMemory(size_t* pTotal, size_t* pFree)
{
    DDSCAPS2 ddsc = { 0 };
    ddsc.dwCaps = DDSCAPS_TEXTURE;
    HRESULT ddres = IDirectDraw4_GetAvailableVidMem(stdDisplay_lpDD, &ddsc, (LPDWORD)pTotal, (LPDWORD)pFree);
    if ( ddres == DD_OK ) {
        return 0;
    }
    STDLOG_ERROR("Error %s in stdDisplay_GetTextureMemory.\n", stdDisplay_DDGetStatus(ddres));
    return 1;
}

int J3DAPI stdDisplay_GetTotalMemory(size_t* pTotal, size_t* pFree)
{
    DDSCAPS2 ddscaps = { 0 };
    ddscaps.dwCaps = DDSCAPS_VIDEOMEMORY;
    HRESULT ddres = IDirectDraw4_GetAvailableVidMem(stdDisplay_lpDD, &ddscaps, (LPDWORD)pTotal, (LPDWORD)pFree);
    if ( ddres == DD_OK ) {
        return 0;
    }
    STDLOG_ERROR("Error %s in stdDisplay_GetTotalMemory.\n", stdDisplay_DDGetStatus(ddres));
    return 1;
}

const char* J3DAPI stdDisplay_DDGetStatus(HRESULT status)
{
    for ( size_t i = 0; i < STD_ARRAYLEN(stdDisplay_aDDStatusTbl); ++i )
    {
        if ( stdDisplay_aDDStatusTbl[i].code == status ) {
            return stdDisplay_aDDStatusTbl[i].text;
        }
    }

    return "Unknown Error";
}

int J3DAPI stdDisplay_CreateZBuffer(LPDDPIXELFORMAT pPixelFormat, int bSystemMemory)
{
    // Added
    if ( !stdDisplay_bOpen )
    {
        STDLOG_ERROR("Error creating zBuffer, system is closed!\n");
        return 1;
    }

    memset(&stdDisplay_zBuffer.ddSurfDesc, 0, sizeof(stdDisplay_zBuffer.ddSurfDesc));
    stdDisplay_zBuffer.ddSurfDesc.dwHeight = stdDisplay_g_backBuffer.rasterInfo.height;
    stdDisplay_zBuffer.ddSurfDesc.dwWidth  = stdDisplay_g_backBuffer.rasterInfo.width;
    stdDisplay_zBuffer.ddSurfDesc.dwSize   = sizeof(DDSURFACEDESC2);
    stdDisplay_zBuffer.ddSurfDesc.dwFlags  = DDSD_PIXELFORMAT | DDSD_WIDTH | DDSD_HEIGHT | DDSD_CAPS;// DDSD_PIXELFORMAT  | DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH
    stdDisplay_zBuffer.ddSurfDesc.ddsCaps.dwCaps = bSystemMemory != 0
        ? DDSCAPS_ZBUFFER | DDSCAPS_SYSTEMMEMORY
        : DDSCAPS_ZBUFFER | DDSCAPS_VIDEOMEMORY;

    memcpy(&stdDisplay_zBuffer.ddSurfDesc.ddpfPixelFormat, pPixelFormat, sizeof(stdDisplay_zBuffer.ddSurfDesc.ddpfPixelFormat));
    HRESULT ddres = IDirectDraw4_CreateSurface(stdDisplay_lpDD, &stdDisplay_zBuffer.ddSurfDesc, &stdDisplay_zBuffer.pDDSurf, NULL);
    if ( ddres != DD_OK )
    {
        STDLOG_ERROR("Error %s when creating zBuffer DDraw surface.\n", stdDisplay_DDGetStatus(ddres));
        return 1;
    }

    ddres = IDirectDrawSurface4_AddAttachedSurface(stdDisplay_g_backBuffer.surface.pDDSurf, stdDisplay_zBuffer.pDDSurf);
    if ( ddres != DD_OK )
    {
        STDLOG_ERROR("Error %s when attaching zbuffer.\n", stdDisplay_DDGetStatus(ddres));
        return 1;
    }

    ddres = IDirectDrawSurface4_GetSurfaceDesc(stdDisplay_zBuffer.pDDSurf, &stdDisplay_zBuffer.ddSurfDesc);
    if ( ddres != DD_OK )
    {
        STDLOG_ERROR("Error %s when getting zbuffer surface description.\n", stdDisplay_DDGetStatus(ddres));
        return 1;
    }

    return 0;
}

int J3DAPI stdDisplay_InitDirectDraw(HWND hwnd)
{
    J3D_UNUSED(hwnd);
    HRESULT ddres;
    LPDIRECTDRAW lpDD = NULL;
    if ( stdDisplay_pCurDevice->bGuidNotSet ) {
        ddres = DirectDrawCreate(NULL, &lpDD, NULL);
    }
    else {
        ddres = DirectDrawCreate(&stdDisplay_pCurDevice->guid, &lpDD, NULL);
    }

    if ( ddres != DD_OK )
    {
        STDLOG_ERROR("Error %s when creating DirectDraw object.\n", stdDisplay_DDGetStatus(ddres));
        return 0;
    }

    ddres = IDirectDraw_QueryInterface(lpDD, &IID_IDirectDraw4, &stdDisplay_lpDD);
    if ( ddres != DD_OK )
    {
        STDLOG_ERROR("Error getting DirectDraw4 %s.\n", stdDisplay_DDGetStatus(ddres));
        return 0;
    }

    ddres = IDirectDraw_Release(lpDD);
    if ( ddres != DD_OK )
    {
        STDLOG_ERROR("Error Release %s.\n", stdDisplay_DDGetStatus(ddres));
        return 0;
    }
    lpDD = NULL;

    stdDisplay_numVideoModes = 0;
    ddres = IDirectDraw4_EnumDisplayModes(stdDisplay_lpDD, 0, NULL, NULL, stdDisplay_EnumVideoModesCallback);
    if ( ddres != DD_OK )
    {
        STDLOG_ERROR("Error %s when enumerating video modes.\n", stdDisplay_DDGetStatus(ddres));
        return 0;
    }

    return 1;
}

void J3DAPI stdDisplay_ReleaseDirectDraw()
{
    if ( stdDisplay_lpDD )
    {
        HRESULT ddres = IDirectDraw4_SetCooperativeLevel(stdDisplay_lpDD, 0, DDSCL_NORMAL);
        if ( ddres != DD_OK ) {
            STDLOG_ERROR("Error %s when setting coop level.\n", stdDisplay_DDGetStatus(ddres));
        }

        ddres = IDirectDraw4_RestoreDisplayMode(stdDisplay_lpDD);
        if ( ddres != DD_OK ) {
            STDLOG_ERROR("Error %s when restoring the video mode.\n", stdDisplay_DDGetStatus(ddres));
        }

        IDirectDraw4_Release(stdDisplay_lpDD);
        stdDisplay_lpDD = NULL;
    }

    stdDisplay_coopLevelFlags = DDSCL_NORMAL;
    stdDisplay_numVideoModes = 0;
}

BOOL PASCAL stdDisplay_DDEnumCallback(GUID* lpGUID, LPSTR szDriverName, LPSTR szDriverDescription, LPVOID lpContext)
{
    J3D_UNUSED(lpContext);

    if ( (unsigned int)stdDisplay_numDevices >= STD_ARRAYLEN(stdDisplay_aDisplayDevices) ) {
        return 0;
    }

    StdDisplayDevice* pDevice = &stdDisplay_aDisplayDevices[stdDisplay_numDevices];
    if ( lpGUID )
    {
        pDevice->guid = *lpGUID;
        pDevice->bGuidNotSet = 0;
    }
    else
    {
        memset(&pDevice->guid, 0, sizeof(pDevice->guid));
        pDevice->bGuidNotSet = 1;
    }

    STD_STRCPY(pDevice->aDriverName, szDriverName);
    STD_STRCPY(pDevice->aDeviceName, szDriverDescription);

    LPDIRECTDRAW lpDD = NULL;
    HRESULT ddres = DirectDrawCreate(lpGUID, &lpDD, 0);
    if ( ddres != DD_OK )
    {
        STDLOG_ERROR("Error %s when creating scratch DirectDraw object.\n", stdDisplay_DDGetStatus(ddres));
        return 0;
    }

    memset(&pDevice->ddcaps, 0, sizeof(pDevice->ddcaps));
    pDevice->ddcaps.dwSize = sizeof(DDCAPS_DX6);
    ddres = IDirectDraw_GetCaps(lpDD, (LPDDCAPS)&pDevice->ddcaps, 0);
    if ( ddres != DD_OK )
    {
        STDLOG_ERROR("Error %s when reading DDraw CAPS.\n", stdDisplay_DDGetStatus(ddres));
        IDirectDraw_Release(lpDD);
        return 0;
    }

    pDevice->bWindowRenderNotSupported = (pDevice->ddcaps.dwCaps2 & DDCAPS2_CANRENDERWINDOWED) == 0;
    pDevice->bHAL                      = (pDevice->ddcaps.dwCaps & DDCAPS_3D) != 0;
    pDevice->totalVideoMemory          = pDevice->ddcaps.dwVidMemTotal;
    pDevice->freeVideoMemory           = pDevice->ddcaps.dwVidMemFree;

    STDLOG_STATUS("Found %s DD Device: %s [%s]\n", pDevice->bHAL ? "3D" : "Non-3D", pDevice->aDeviceName, pDevice->aDriverName);
    STDLOG_STATUS("Memory: 0x%x out of 0x%x free\n", pDevice->freeVideoMemory, pDevice->totalVideoMemory);

    IDirectDraw_Release(lpDD);
    ++stdDisplay_numDevices;
    return 1;
}

HRESULT PASCAL stdDisplay_EnumVideoModesCallback(LPDDSURFACEDESC2 lpDDSurfaceDesc, LPVOID lpContext)
{
    J3D_UNUSED(lpContext);

    if ( stdDisplay_numVideoModes >= STD_ARRAYLEN(stdDisplay_aVideoModes) ) {
        return 0;
    }

    // Added: Cap the resolution to HD since std3D device doesn't support higher resolutions
    // TODO: Remove this constrain when porting to newer GAPI
    if ( lpDDSurfaceDesc->dwWidth >= 2000 || lpDDSurfaceDesc->dwHeight >= 2000 ) {
        return 1;
    }

    StdVideoMode* pMode       = &stdDisplay_aVideoModes[stdDisplay_numVideoModes];
    pMode->rasterInfo.width   = lpDDSurfaceDesc->dwWidth;
    pMode->rasterInfo.height  = lpDDSurfaceDesc->dwHeight;
    pMode->rasterInfo.rowSize = lpDDSurfaceDesc->lPitch;
    stdDisplay_SetAspectRatio(pMode);

    uint32_t pfflags = lpDDSurfaceDesc->ddpfPixelFormat.dwFlags;
    if ( (pfflags & DDPF_PALETTEINDEXED8) != 0 )
    {
        pMode->rasterInfo.colorInfo.colorMode = STDCOLOR_PAL;
        pMode->rasterInfo.colorInfo.bpp       = 8;
        pMode->rasterInfo.colorInfo.redBPP    = 0;
        pMode->rasterInfo.colorInfo.greenBPP  = 0;
        pMode->rasterInfo.colorInfo.blueBPP   = 0;
    }
    else if ( (pfflags & DDPF_RGB) != 0 )
    {
        pMode->rasterInfo.colorInfo.colorMode = STDCOLOR_RGB;
        pMode->rasterInfo.colorInfo.bpp       = lpDDSurfaceDesc->ddpfPixelFormat.dwRGBBitCount;

        stdColor_CalcColorBits(lpDDSurfaceDesc->ddpfPixelFormat.dwRBitMask, &pMode->rasterInfo.colorInfo.redBPP, &pMode->rasterInfo.colorInfo.redPosShift, &pMode->rasterInfo.colorInfo.redPosShiftRight);
        stdColor_CalcColorBits(lpDDSurfaceDesc->ddpfPixelFormat.dwGBitMask, &pMode->rasterInfo.colorInfo.greenBPP, &pMode->rasterInfo.colorInfo.greenPosShift, &pMode->rasterInfo.colorInfo.greenPosShiftRight);
        stdColor_CalcColorBits(lpDDSurfaceDesc->ddpfPixelFormat.dwBBitMask, &pMode->rasterInfo.colorInfo.blueBPP, &pMode->rasterInfo.colorInfo.bluePosShift, &pMode->rasterInfo.colorInfo.bluePosShiftRight);

        // Added
        pMode->rasterInfo.colorInfo.alphaBPP           = 0;
        pMode->rasterInfo.colorInfo.alphaPosShift      = 0;
        pMode->rasterInfo.colorInfo.alphaPosShiftRight = 0;
    }

    switch ( pMode->rasterInfo.colorInfo.bpp )
    {
        case 8u:
            pMode->rasterInfo.rowWidth = pMode->rasterInfo.rowSize;
            break;
        case 16u:
            pMode->rasterInfo.rowWidth = pMode->rasterInfo.rowSize / 2u;
            break;
        case 24u:
            pMode->rasterInfo.rowWidth = pMode->rasterInfo.rowSize / 3u;
            break;
        case 32u:
            pMode->rasterInfo.rowWidth = pMode->rasterInfo.rowSize / 4u;
            break;
        default:
            // TODO: maybe make log here?
            break;
    }

    pMode->rasterInfo.size = pMode->rasterInfo.width * pMode->rasterInfo.height * (pMode->rasterInfo.colorInfo.bpp / 8);

    size_t freeVRam = 0, totalVRam = 0, freeTexMem = 0, totalTexMem = 0;
    stdDisplay_GetTotalMemory(&totalVRam, &freeVRam);
    stdDisplay_GetTextureMemory(&totalTexMem, &freeTexMem);

    // Fixed: Fixed bytes display format from d to u
    size_t requiredVRam = 3u * pMode->rasterInfo.size;
    STDLOG_STATUS("Video Ram Total: %u bytes  Free: %u bytes Required: %u bytes.\n", totalVRam, freeVRam, requiredVRam);
    STDLOG_STATUS("Texture Ram Total: %u bytes  Free: %u bytes.\n", totalTexMem, freeTexMem);

    if ( requiredVRam <= totalVRam )
    {
        ++stdDisplay_numVideoModes;
        return 1;
    }

    if ( pMode->rasterInfo.width <= 640 && pMode->rasterInfo.height <= 480 && pMode->rasterInfo.colorInfo.bpp == 16 )
    {
        if ( totalTexMem < 0x200000 ) {
            STDLOG_STATUS("Not enough video memory, allowing <= 640x480x16 anyway...\n");
        }
        else {
            STDLOG_STATUS("Allowing <= 640x480x16 with 2MB texture mem.\n");
        }

        ++stdDisplay_numVideoModes;
        return 1;
    }

    STDLOG_ERROR("Not enough video RAM for mode %d by %d by %d!\n", pMode->rasterInfo.width, pMode->rasterInfo.height, pMode->rasterInfo.colorInfo.bpp);
    return 1;
}

void J3DAPI stdDisplay_SetAspectRatio(StdVideoMode* pMode)
{
    if ( pMode->rasterInfo.width == 320 && pMode->rasterInfo.height == 200 )
    {
        pMode->aspectRatio = 0.75f; // 1/1.333f aka 1/ 4:3
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

LPDIRECTDRAW4 stdDisplay_GetDirectDraw(void)
{
    return stdDisplay_lpDD;
}

int J3DAPI stdDisplay_SetWindowMode(HWND hWnd, StdVideoMode* pDisplayMode)
{
    unsigned int width  = pDisplayMode->rasterInfo.width;
    unsigned int height = pDisplayMode->rasterInfo.height;
    if ( !SetWindowPos(hWnd, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE) )
    {
        return 0;
    }

    stdDisplay_coopLevelFlags = DDSCL_NORMAL;
    HRESULT ddres = IDirectDraw4_SetCooperativeLevel(stdDisplay_lpDD, hWnd, DDSCL_NORMAL);
    if ( ddres != DD_OK )
    {
        STDLOG_ERROR("Error %s when setting cooperative level.\n", stdDisplay_DDGetStatus(ddres));
        return ddres;
    }

    // Set front buffer
    // We also set pDisplayMode in this scope
    {
        stdDisplay_g_frontBuffer.lockRefCount     = 1;
        stdDisplay_g_frontBuffer.lockSurfRefCount = 0;
        stdDisplay_g_frontBuffer.bVideoMemory     = 0;
        stdDisplay_g_frontBuffer.pPixels          = NULL;

        memset(&stdDisplay_g_frontBuffer.surface.ddSurfDesc, 0, sizeof(stdDisplay_g_frontBuffer.surface.ddSurfDesc));
        stdDisplay_g_frontBuffer.surface.ddSurfDesc.dwSize         = sizeof(DDSURFACEDESC2);
        stdDisplay_g_frontBuffer.surface.ddSurfDesc.dwFlags        = DDSD_CAPS;
        stdDisplay_g_frontBuffer.surface.ddSurfDesc.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE;

        ddres = IDirectDraw4_CreateSurface(stdDisplay_lpDD, &stdDisplay_g_frontBuffer.surface.ddSurfDesc, &stdDisplay_g_frontBuffer.surface.pDDSurf, NULL);
        if ( ddres != DD_OK )
        {
            STDLOG_ERROR("Error %s when creating the DirectDraw primary surface.\n", stdDisplay_DDGetStatus(ddres));
            return 0;
        }

        LPDIRECTDRAWCLIPPER pDDClipper;
        ddres = IDirectDraw4_CreateClipper(stdDisplay_lpDD, 0, &pDDClipper, NULL);
        if ( ddres != DD_OK )
        {
            STDLOG_ERROR("Error %s when creating DirectDraw clipper.\n", stdDisplay_DDGetStatus(ddres));
            return 0;
        }

        ddres = IDirectDrawClipper_SetHWnd(pDDClipper, 0, hWnd);
        if ( ddres != DD_OK )
        {
            IDirectDrawClipper_Release(pDDClipper);
            STDLOG_ERROR("Error %s when setting clipper hwnd.\n", stdDisplay_DDGetStatus(ddres));
            return 0;
        }

        ddres = IDirectDrawSurface4_SetClipper(stdDisplay_g_frontBuffer.surface.pDDSurf, pDDClipper);
        if ( ddres != DD_OK )
        {
            IDirectDrawClipper_Release(pDDClipper);
            STDLOG_ERROR("Error %s when attaching clipper to primary surface.\n", stdDisplay_DDGetStatus(ddres));
            return 0;
        }

        IDirectDrawClipper_Release(pDDClipper);

        ddres = IDirectDrawSurface4_GetSurfaceDesc(stdDisplay_g_frontBuffer.surface.pDDSurf, &stdDisplay_g_frontBuffer.surface.ddSurfDesc);
        if ( ddres != DD_OK )
        {
            STDLOG_ERROR("Error %s when getting desc of DirectDraw primary surface.\n", stdDisplay_DDGetStatus(ddres));
            return 0;
        }

        pDisplayMode->rasterInfo.width   = stdDisplay_g_frontBuffer.surface.ddSurfDesc.dwWidth;
        pDisplayMode->rasterInfo.height  = stdDisplay_g_frontBuffer.surface.ddSurfDesc.dwHeight;
        pDisplayMode->rasterInfo.rowSize = stdDisplay_g_frontBuffer.surface.ddSurfDesc.lPitch;
        stdDisplay_SetAspectRatio(pDisplayMode);

        if ( (stdDisplay_g_frontBuffer.surface.ddSurfDesc.ddpfPixelFormat.dwFlags & (DDPF_PALETTEINDEXED2 | DDPF_PALETTEINDEXED1 | DDPF_PALETTEINDEXED8 | DDPF_PALETTEINDEXED4)) != 0 )
        {
            // We don't support palette
            return 0;
        }

        // Set front buffer raster color info
        if ( (stdDisplay_g_frontBuffer.surface.ddSurfDesc.ddpfPixelFormat.dwFlags & DDPF_RGB) != 0 ) // IF RGB
        {
            ColorInfo* pCI = &pDisplayMode->rasterInfo.colorInfo;
            memset(pCI, 0, sizeof(*pCI)); // Added: init to zero
            pCI->colorMode  = STDCOLOR_RGB;
            pCI->bpp       = stdDisplay_g_frontBuffer.surface.ddSurfDesc.ddpfPixelFormat.dwRGBBitCount;


            DDPIXELFORMAT* pFormat = &stdDisplay_g_frontBuffer.surface.ddSurfDesc.ddpfPixelFormat;
            stdColor_CalcColorBits(pFormat->dwRBitMask, &pCI->redBPP, &pCI->redPosShift, &pCI->redPosShiftRight);
            stdColor_CalcColorBits(pFormat->dwGBitMask, &pCI->greenBPP, &pCI->greenPosShift, &pCI->greenPosShiftRight);
            stdColor_CalcColorBits(pFormat->dwBBitMask, &pCI->blueBPP, &pCI->bluePosShift, &pCI->bluePosShiftRight);
        }

        // Set raster sizes
        unsigned int bpp = pDisplayMode->rasterInfo.colorInfo.bpp;
        switch ( bpp )
        {
            case 8u:
                pDisplayMode->rasterInfo.rowWidth = pDisplayMode->rasterInfo.rowSize;
                break;

            case 16u:
                pDisplayMode->rasterInfo.rowWidth = pDisplayMode->rasterInfo.rowSize / 2u;
                break;

            case 24u:
                pDisplayMode->rasterInfo.rowWidth = pDisplayMode->rasterInfo.rowSize / 3u;
                break;

            case 32u:
                pDisplayMode->rasterInfo.rowWidth = pDisplayMode->rasterInfo.rowSize / 4u;
                break;

            default:
                break;
        }
        pDisplayMode->rasterInfo.size =  pDisplayMode->rasterInfo.height * pDisplayMode->rasterInfo.width * (bpp / 8);

        // Update raster info of front buffer
        memcpy(&stdDisplay_g_frontBuffer.rasterInfo, &pDisplayMode->rasterInfo, sizeof(stdDisplay_g_frontBuffer.rasterInfo));
        stdDisplay_g_frontBuffer.rasterInfo.rowSize  = stdDisplay_g_frontBuffer.surface.ddSurfDesc.lPitch;
        stdDisplay_g_frontBuffer.rasterInfo.rowWidth = stdDisplay_g_frontBuffer.surface.ddSurfDesc.lPitch / (bpp / 8);

        if ( (stdDisplay_g_frontBuffer.surface.ddSurfDesc.ddsCaps.dwCaps & DDSCAPS_VIDEOMEMORY) != 0 ) {
            stdDisplay_g_frontBuffer.bVideoMemory = 1;
        }
    }

    // Set back buffer
    {
        memset(&stdDisplay_g_backBuffer.surface.ddSurfDesc, 0, sizeof(stdDisplay_g_backBuffer.surface.ddSurfDesc));
        stdDisplay_g_backBuffer.surface.ddSurfDesc.dwSize         = sizeof(DDSURFACEDESC2);
        stdDisplay_g_backBuffer.surface.ddSurfDesc.dwFlags        = DDSD_WIDTH | DDSD_HEIGHT | DDSD_CAPS;
        stdDisplay_g_backBuffer.surface.ddSurfDesc.dwWidth        = width;
        stdDisplay_g_backBuffer.surface.ddSurfDesc.dwHeight       = height;
        stdDisplay_g_backBuffer.surface.ddSurfDesc.ddsCaps.dwCaps = DDSCAPS_3DDEVICE | DDSCAPS_OFFSCREENPLAIN;

        ddres = IDirectDraw4_CreateSurface(stdDisplay_lpDD, &stdDisplay_g_backBuffer.surface.ddSurfDesc, &stdDisplay_g_backBuffer.surface.pDDSurf, NULL);
        if ( ddres != DD_OK )
        {
            STDLOG_ERROR("Error %s creating off-screen back buffer.\n", stdDisplay_DDGetStatus(ddres));
            return 0;
        }

        stdDisplay_g_backBuffer.lockRefCount = 1;
        stdDisplay_g_backBuffer.lockSurfRefCount = 0;
        stdDisplay_g_backBuffer.bVideoMemory = 1;

        ddres = IDirectDrawSurface4_GetSurfaceDesc(stdDisplay_g_backBuffer.surface.pDDSurf, &stdDisplay_g_backBuffer.surface.ddSurfDesc);
        if ( ddres != DD_OK )
        {
            STDLOG_ERROR("Error %s when getting desc of back buffer rendering surface.\n", stdDisplay_DDGetStatus(ddres));
            return 0;
        }

        stdDisplay_g_backBuffer.rasterInfo.width   = stdDisplay_g_backBuffer.surface.ddSurfDesc.dwWidth;
        stdDisplay_g_backBuffer.rasterInfo.height  = stdDisplay_g_backBuffer.surface.ddSurfDesc.dwHeight;
        stdDisplay_g_backBuffer.rasterInfo.rowSize = stdDisplay_g_backBuffer.surface.ddSurfDesc.lPitch;

        if ( (stdDisplay_g_backBuffer.surface.ddSurfDesc.ddpfPixelFormat.dwFlags & (DDPF_PALETTEINDEXED2 | DDPF_PALETTEINDEXED1 | DDPF_PALETTEINDEXED8 | DDPF_PALETTEINDEXED4)) != 0 )
        {
            // We don't support palette
            return 0;
        }

        if ( (stdDisplay_g_backBuffer.surface.ddSurfDesc.ddpfPixelFormat.dwFlags & DDPF_RGB) != 0 )
        {
            ColorInfo* pCI = &stdDisplay_g_backBuffer.rasterInfo.colorInfo;
            memset(pCI, 0, sizeof(*pCI)); // Added: init to zero
            pCI->colorMode = STDCOLOR_RGB;
            pCI->bpp       = stdDisplay_g_backBuffer.surface.ddSurfDesc.ddpfPixelFormat.dwRGBBitCount;

            DDPIXELFORMAT* pFormat = &stdDisplay_g_backBuffer.surface.ddSurfDesc.ddpfPixelFormat;
            stdColor_CalcColorBits(pFormat->dwRBitMask, &pCI->redBPP, &pCI->redPosShift, &pCI->redPosShiftRight);
            stdColor_CalcColorBits(pFormat->dwGBitMask, &pCI->greenBPP, &pCI->greenPosShift, &pCI->greenPosShiftRight);
            stdColor_CalcColorBits(pFormat->dwBBitMask, &pCI->blueBPP, &pCI->bluePosShift, &pCI->bluePosShiftRight);
        }

        stdDisplay_coopLevelFlags = DDSCL_NORMAL;
        stdDisplay_g_backBuffer.rasterInfo.rowSize  = stdDisplay_g_backBuffer.surface.ddSurfDesc.lPitch; // TODO: fyi the row size was already set few lines above
        stdDisplay_g_backBuffer.rasterInfo.rowWidth = stdDisplay_g_backBuffer.rasterInfo.rowSize / (stdDisplay_g_backBuffer.rasterInfo.colorInfo.bpp / 8);
    }

    STDLOG_STATUS("Primary buffer in %s memory.\n", stdDisplay_g_frontBuffer.bVideoMemory ? "VIDEO" : "SYSTEM");
    STDLOG_STATUS("Back buffer in %s memory.\n", stdDisplay_g_backBuffer.bVideoMemory ? "VIDEO" : "SYSTEM");
    return 1;
}

int J3DAPI stdDisplay_SetFullscreenMode(HWND hwnd, const StdVideoMode* pDisplayMode, size_t numBackBuffers)
{
    stdDisplay_coopLevelFlags = DDSCL_EXCLUSIVE | DDSCL_FULLSCREEN;
    HRESULT ddres = IDirectDraw4_SetCooperativeLevel(stdDisplay_lpDD, hwnd, stdDisplay_coopLevelFlags);
    if ( ddres != DD_OK )
    {
        STDLOG_ERROR("Error %s when setting cooperative level.\n", stdDisplay_DDGetStatus(ddres));
        return ddres;
    }

    STDLOG_STATUS("Set video mode %d %d %d.\n", pDisplayMode->rasterInfo.width, pDisplayMode->rasterInfo.height, pDisplayMode->rasterInfo.colorInfo.bpp);

    ddres = IDirectDraw4_SetDisplayMode(stdDisplay_lpDD, pDisplayMode->rasterInfo.width, pDisplayMode->rasterInfo.height, pDisplayMode->rasterInfo.colorInfo.bpp, 0, 0);
    if ( ddres != DD_OK )
    {
        STDLOG_ERROR("Error %s when setting video mode.\n", stdDisplay_DDGetStatus(ddres));
        return ddres;
    }

    unsigned int bbpp = (unsigned int)pDisplayMode->rasterInfo.colorInfo.bpp / 8;

    // Setup front buffer
    {
        stdDisplay_g_frontBuffer.lockSurfRefCount = 0;
        stdDisplay_g_frontBuffer.bVideoMemory     = 0;
        stdDisplay_g_frontBuffer.lockRefCount     = 1;
        stdDisplay_g_frontBuffer.pPixels          = NULL;
        memcpy(&stdDisplay_g_frontBuffer.rasterInfo, &pDisplayMode->rasterInfo, sizeof(stdDisplay_g_frontBuffer.rasterInfo));

        memset(&stdDisplay_g_frontBuffer.surface.ddSurfDesc, 0, sizeof(stdDisplay_g_frontBuffer.surface.ddSurfDesc));
        stdDisplay_g_frontBuffer.surface.ddSurfDesc.dwSize            = sizeof(DDSURFACEDESC2);
        stdDisplay_g_frontBuffer.surface.ddSurfDesc.dwWidth           = pDisplayMode->rasterInfo.width;
        stdDisplay_g_frontBuffer.surface.ddSurfDesc.dwHeight          = pDisplayMode->rasterInfo.height;
        stdDisplay_g_frontBuffer.surface.ddSurfDesc.dwBackBufferCount = numBackBuffers;
        stdDisplay_g_frontBuffer.surface.ddSurfDesc.dwFlags           = DDSD_BACKBUFFERCOUNT | DDSD_CAPS;
        stdDisplay_g_frontBuffer.surface.ddSurfDesc.ddsCaps.dwCaps    = DDSCAPS_3DDEVICE | DDSCAPS_PRIMARYSURFACE | DDSCAPS_FLIP | DDSCAPS_COMPLEX;

        ddres = IDirectDraw4_CreateSurface(stdDisplay_lpDD, &stdDisplay_g_frontBuffer.surface.ddSurfDesc, &stdDisplay_g_frontBuffer.surface.pDDSurf, NULL);
        if ( ddres != DD_OK )
        {
            STDLOG_ERROR("Error %s when creating the DirectDraw primary surface.\n", stdDisplay_DDGetStatus(ddres));
            return ddres;
        }

        ddres = IDirectDrawSurface4_GetSurfaceDesc(stdDisplay_g_frontBuffer.surface.pDDSurf, &stdDisplay_g_frontBuffer.surface.ddSurfDesc);
        if ( ddres != DD_OK )
        {
            STDLOG_ERROR("Error %s when getting desc of DirectDraw primary surface.\n", stdDisplay_DDGetStatus(ddres));
            return ddres;
        }

        stdDisplay_g_frontBuffer.rasterInfo.rowSize  = stdDisplay_g_frontBuffer.surface.ddSurfDesc.lPitch;
        stdDisplay_g_frontBuffer.rasterInfo.rowWidth = stdDisplay_g_frontBuffer.surface.ddSurfDesc.lPitch / bbpp;

        if ( (stdDisplay_g_frontBuffer.surface.ddSurfDesc.ddsCaps.dwCaps & DDSCAPS_VIDEOMEMORY) != 0 ) {
            stdDisplay_g_frontBuffer.bVideoMemory = 1;
        }
    }

    // Setup back buffer
    {
        stdDisplay_g_backBuffer.lockSurfRefCount = 0;
        stdDisplay_g_backBuffer.bVideoMemory     = 0;
        stdDisplay_g_backBuffer.lockRefCount     = 1;
        stdDisplay_g_backBuffer.pPixels          = 0;
        memcpy(&stdDisplay_g_backBuffer.rasterInfo, &pDisplayMode->rasterInfo, sizeof(stdDisplay_g_backBuffer.rasterInfo));

        memset(&stdDisplay_g_backBuffer.surface.ddSurfDesc, 0, sizeof(stdDisplay_g_backBuffer.surface.ddSurfDesc));
        stdDisplay_g_backBuffer.surface.ddSurfDesc.dwSize            = sizeof(DDSURFACEDESC2);
        stdDisplay_g_backBuffer.surface.ddSurfDesc.dwWidth           = pDisplayMode->rasterInfo.width;
        stdDisplay_g_backBuffer.surface.ddSurfDesc.dwHeight          = pDisplayMode->rasterInfo.height;
        stdDisplay_g_backBuffer.surface.ddSurfDesc.dwFlags           = DDSD_BACKBUFFERCOUNT | DDSD_CAPS;
        stdDisplay_g_backBuffer.surface.ddSurfDesc.ddsCaps.dwCaps    = DDSCAPS_3DDEVICE | DDSCAPS_BACKBUFFER;
        stdDisplay_g_backBuffer.surface.ddSurfDesc.dwBackBufferCount = numBackBuffers;

        ddres = IDirectDrawSurface4_GetAttachedSurface(
            stdDisplay_g_frontBuffer.surface.pDDSurf,
            &stdDisplay_g_backBuffer.surface.ddSurfDesc.ddsCaps,
            &stdDisplay_g_backBuffer.surface.pDDSurf
        );
        if ( ddres != DD_OK )
        {
            STDLOG_ERROR("Error %s when creating the DirectDraw backbuffer surface.\n", stdDisplay_DDGetStatus(ddres));
            return ddres;
        }

        ddres = IDirectDrawSurface4_GetSurfaceDesc(stdDisplay_g_backBuffer.surface.pDDSurf, &stdDisplay_g_backBuffer.surface.ddSurfDesc);
        if ( ddres != DD_OK )
        {
            STDLOG_ERROR("Error %s when getting desc of DirectDraw back surface.\n", stdDisplay_DDGetStatus(ddres));
            return ddres;
        }

        if ( (stdDisplay_g_backBuffer.surface.ddSurfDesc.ddsCaps.dwCaps & DDSCAPS_VIDEOMEMORY) != 0 ) {
            stdDisplay_g_backBuffer.bVideoMemory = 1;
        }

        stdDisplay_g_backBuffer.rasterInfo.rowSize  = stdDisplay_g_backBuffer.surface.ddSurfDesc.lPitch;
        stdDisplay_g_backBuffer.rasterInfo.rowWidth = stdDisplay_g_backBuffer.surface.ddSurfDesc.lPitch / bbpp;
    }

    STDLOG_STATUS("Primary buffer in %s memory.\n", stdDisplay_g_frontBuffer.bVideoMemory ? "VIDEO" : "SYSTEM");
    STDLOG_STATUS("Back buffer in %s memory.\n", stdDisplay_g_backBuffer.bVideoMemory ? "VIDEO" : "SYSTEM");
    return 0;
}

void J3DAPI stdDisplay_ReleaseBuffers()
{
    if ( stdDisplay_zBuffer.pDDSurf )
    {
        IDirectDrawSurface4_DeleteAttachedSurface(stdDisplay_g_backBuffer.surface.pDDSurf, 0, stdDisplay_zBuffer.pDDSurf);
        IDirectDrawSurface4_Release(stdDisplay_zBuffer.pDDSurf);
    }

    if ( stdDisplay_g_frontBuffer.surface.pDDSurf ) {
        IDirectDrawSurface4_Release(stdDisplay_g_frontBuffer.surface.pDDSurf);
    }

    memset(&stdDisplay_zBuffer, 0, sizeof(stdDisplay_zBuffer));
    memset(&stdDisplay_g_backBuffer, 0, sizeof(stdDisplay_g_backBuffer));
    memset(&stdDisplay_g_frontBuffer, 0, sizeof(stdDisplay_g_frontBuffer));
}

uint8_t* J3DAPI stdDisplay_LockSurface(tVSurface* pVSurf)
{
    while ( 1 )
    {
        HRESULT ddres = IDirectDrawSurface4_Lock(pVSurf->pDDSurf, NULL, &pVSurf->ddSurfDesc, DDLOCK_WAIT, NULL);
        if ( ddres == DD_OK ) {
            break;
        }

        if ( ddres == DDERR_SURFACELOST )
        {
            if ( (pVSurf->ddSurfDesc.ddsCaps.dwCaps & DDSCAPS_BACKBUFFER) != 0 ) {
                ddres = IDirectDrawSurface4_Restore(stdDisplay_g_frontBuffer.surface.pDDSurf);
            }
            else {
                ddres = IDirectDrawSurface4_Restore(pVSurf->pDDSurf);
            }
        }

        if ( ddres != DD_OK )
        {
            STDLOG_ERROR("Error %s when locking the DD buffer.\n", stdDisplay_DDGetStatus(ddres));
            return 0;
        }
    }

    return (uint8_t*)pVSurf->ddSurfDesc.lpSurface;
}

int J3DAPI stdDisplay_UnlockSurface(tVSurface* pSurf)
{
    while ( 1 )
    {
        HRESULT ddres = IDirectDrawSurface4_Unlock(pSurf->pDDSurf, 0);
        if ( ddres == DD_OK ) {
            break;
        }

        if ( ddres == DDERR_SURFACELOST )
        {
            if ( (pSurf->ddSurfDesc.ddsCaps.dwCaps & DDSCAPS_BACKBUFFER) != 0 ) {
                ddres = IDirectDrawSurface4_Restore(stdDisplay_g_frontBuffer.surface.pDDSurf);
            }
            else {
                ddres = IDirectDrawSurface4_Restore(pSurf->pDDSurf);
            }
        }

        if ( ddres != DD_OK )
        {
            STDLOG_ERROR("Error %s when unlocking the display buffer surface.\n", stdDisplay_DDGetStatus(ddres));
            return 1;
        }
    }

    return 0;
}

void stdDisplay_DisableVSync(bool bDisable)
{
    stdDisplay_bNoSync = bDisable;
}

int stdDisplay_Update(void)
{
    // TODO: Find a way to disable VSync. This will be usefully especially for when game is loading as it might speed up level loading.
    // TODO: Add frame-cap in the loop
    HRESULT ddres;
    while ( 1 )
    {
        if ( stdDisplay_bFullscreen )
        {
            // Added: VSync disable mode
            // TODO: Add display update flags and control flip/blt option via flags instead of blting by default when nosync is enabled
            if ( stdDisplay_bNoSync ) {
                // Although the device driver might support DDFLIP_NOVSYNC option it is still too slow to run at max fps possible.
                // To circumvent this and run at max fps wee use blt function instead.
                // Note, useful for speeding up game load times
                ddres = IDirectDrawSurface4_Blt(stdDisplay_g_frontBuffer.surface.pDDSurf, NULL, stdDisplay_g_backBuffer.surface.pDDSurf, NULL, DDBLT_WAIT, NULL);
            }
            else {
                ddres = IDirectDrawSurface4_Flip(stdDisplay_g_frontBuffer.surface.pDDSurf, NULL, 0);
            }

            //if ( !stdDisplay_bNoSync || (stdDisplay_pCurDevice->ddcaps.dwCaps2 & DDCAPS2_FLIPNOVSYNC) != 0 ) {
            //    int test = stdDisplay_bNoSync ? DDFLIP_NOVSYNC : 0;
            //    ddres = IDirectDrawSurface4_Flip(stdDisplay_g_frontBuffer.surface.pDDSurf, NULL, stdDisplay_bNoSync ? DDFLIP_NOVSYNC : 0);
            //}
            //else
            //{
            //    // The device driver doesn't support DDFLIP_NOVSYNC option so this is a way around
            //    ddres = IDirectDrawSurface4_Blt(stdDisplay_g_frontBuffer.surface.pDDSurf, NULL, stdDisplay_g_backBuffer.surface.pDDSurf, NULL, DDBLT_WAIT, NULL);
            //}
        }
        else
        {
            // Blit to window screen
            POINT screenPos = { 0 };
            ClientToScreen(stdWin95_GetWindow(), &screenPos);

            RECT dstRect = {
                .left   = screenPos.x,
                .right  = screenPos.x + stdDisplay_g_backBuffer.rasterInfo.width,
                .top    = screenPos.y,
                .bottom = screenPos.y + stdDisplay_g_backBuffer.rasterInfo.height
            };

            RECT srcRect = {
                .left   = 0,
                .right  = stdDisplay_g_backBuffer.rasterInfo.width,
                .top    = 0,
                .bottom = stdDisplay_g_backBuffer.rasterInfo.height
            };

            // Added: Simulate VSync to framecap to match the refresh rate of the display which caps framerate to screen refresh rate.
            if ( !stdDisplay_bNoSync ) {
                IDirectDraw4_WaitForVerticalBlank(stdDisplay_lpDD, DDWAITVB_BLOCKBEGIN, NULL);
            }

            ddres = IDirectDrawSurface4_Blt(stdDisplay_g_frontBuffer.surface.pDDSurf, &dstRect, stdDisplay_g_backBuffer.surface.pDDSurf, &srcRect, DDBLT_WAIT, NULL);
        }

        if ( ddres == DD_OK ) {
            break;
        }

        if ( ddres == DDERR_SURFACELOST )
        {
            ddres = IDirectDrawSurface4_Restore(stdDisplay_g_frontBuffer.surface.pDDSurf);
            if ( ddres != DD_OK )
            {
                STDLOG_ERROR("Error %s when flipping the page.\n", stdDisplay_DDGetStatus(ddres));
                return 1;
            }

            if ( !stdDisplay_bFullscreen )
            {
                ddres = IDirectDrawSurface4_Restore(stdDisplay_g_backBuffer.surface.pDDSurf);
                if ( ddres != DD_OK )
                {
                    STDLOG_ERROR("Error %s when flipping the page.\n", stdDisplay_DDGetStatus(ddres));
                    return 1;
                }
            }
        }

        if ( (ddres != DDERR_WASSTILLDRAWING) && (ddres != DD_OK) )
        {
            STDLOG_ERROR("Error %s when flipping the page.\n", stdDisplay_DDGetStatus(ddres));
            return 1;
        }
    }

    return 0;
}

int J3DAPI stdDisplay_ColorFillSurface(tVSurface* pSurf, uint32_t dwFillColor, const StdRect* pRect)
{
    RECT rect;
    if ( pRect )
    {
        if ( pRect->right == 0 ) {
            return 1;
        }
        if ( pRect->bottom == 0 ) {
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
        rect.right  = pSurf->ddSurfDesc.dwWidth;
        rect.bottom = pSurf->ddSurfDesc.dwHeight;
    }

    DDBLTFX ddbltfx;
    memset(&ddbltfx, 0, sizeof(ddbltfx));
    ddbltfx.dwSize      = sizeof(DDBLTFX);
    ddbltfx.dwFillColor = dwFillColor;

    while ( 1 )
    {
        HRESULT ddres = IDirectDrawSurface4_Blt(pSurf->pDDSurf, &rect, NULL, NULL, DDBLT_WAIT | DDBLT_COLORFILL, &ddbltfx);
        if ( ddres == DD_OK ) {
            break;
        }

        if ( ddres == DDERR_SURFACELOST )
        {
            if ( (pSurf->ddSurfDesc.ddsCaps.dwCaps & DDSCAPS_BACKBUFFER) != 0 ) {
                ddres = IDirectDrawSurface4_Restore(stdDisplay_g_frontBuffer.surface.pDDSurf);
            }
            else {
                ddres = IDirectDrawSurface4_Restore(pSurf->pDDSurf);
            }
        }

        if ( ddres != DD_OK )
        {
            STDLOG_ERROR("Error %s when color filling the surface.\n", stdDisplay_DDGetStatus(ddres));
            return 1;
        }
    }

    return 0;
}

int J3DAPI stdDisplay_BackBufferFill(uint32_t color, const StdRect* pRect)
{
    return stdDisplay_ColorFillSurface(&stdDisplay_g_backBuffer.surface, color, pRect);
}

int J3DAPI stdDisplay_SaveScreen(const char* pFilename)
{
    return stdBmp_WriteVBuffer(pFilename, &stdDisplay_g_backBuffer);
}

void J3DAPI stdDisplay_SetDefaultResolution(uint32_t width, uint32_t height)
{
    stdDisplay_primaryVideoMode.rasterInfo.width  = width;
    stdDisplay_primaryVideoMode.rasterInfo.height = height;
}

void J3DAPI stdDisplay_GetBackBufferSize(uint32_t* pWidth, uint32_t* pHeight)
{
    *pWidth = stdDisplay_g_backBuffer.rasterInfo.width;
    *pHeight = stdDisplay_g_backBuffer.rasterInfo.height;
}

size_t stdDisplay_GetNumVideoModes(void)
{
    return stdDisplay_numVideoModes;
}

int J3DAPI stdDisplay_GetVideoMode(size_t modeNum, StdVideoMode* pDestMode)
{
    if ( modeNum >= stdDisplay_numVideoModes ) {
        return 1;
    }

    memcpy(pDestMode, &stdDisplay_aVideoModes[modeNum], sizeof(StdVideoMode));
    return 0;
}

int J3DAPI stdDisplay_GetCurrentVideoMode(StdVideoMode* pDisplayMode)
{
    if ( !stdDisplay_pCurVideoMode ) {
        return 1;
    }

    memcpy(pDisplayMode, stdDisplay_pCurVideoMode, sizeof(StdVideoMode));
    return 0;
}

HDC stdDisplay_GetFrontBufferDC(void)
{
    if ( !stdDisplay_bOpen || !stdDisplay_bModeSet ) {
        return 0;
    }

    HDC hdc = 0;
    HRESULT ddres = IDirectDrawSurface4_GetDC(stdDisplay_g_frontBuffer.surface.pDDSurf, &hdc);
    if ( ddres == DD_OK ) {
        return hdc;
    }

    STDLOG_ERROR("Error %s when getting DC of back buffer.\n", stdDisplay_DDGetStatus(ddres));
    return 0;
}

void J3DAPI stdDisplay_ReleaseFrontBufferDC(HDC hdc)
{
    if ( stdDisplay_bOpen && stdDisplay_bModeSet )
    {
        HRESULT ddres = IDirectDrawSurface4_ReleaseDC(stdDisplay_g_frontBuffer.surface.pDDSurf, hdc);
        if ( ddres != DD_OK ) {
            STDLOG_ERROR("Error %s when releasing DC of front buffer.\n", stdDisplay_DDGetStatus(ddres));
        }
    }
}

HDC stdDisplay_GetBackBufferDC(void)
{
    if ( !stdDisplay_bOpen || !stdDisplay_bModeSet )
    {
        return 0;
    }

    HDC hdc = NULL;
    HRESULT ddres = IDirectDrawSurface4_GetDC(stdDisplay_g_backBuffer.surface.pDDSurf, &hdc);
    if ( ddres != DD_OK ) {
        return hdc;
    }

    STDLOG_ERROR("Error %s when getting DC of back buffer.\n", stdDisplay_DDGetStatus(ddres));
    return 0;
}

void J3DAPI stdDisplay_ReleaseBackBufferDC(HDC hdc)
{
    if ( stdDisplay_bOpen && stdDisplay_bModeSet )
    {
        HRESULT ddres = IDirectDrawSurface4_ReleaseDC(stdDisplay_g_backBuffer.surface.pDDSurf, hdc);
        if ( ddres != DD_OK ) {
            STDLOG_ERROR("Error %s when releasing DC of back buffer.\n", stdDisplay_DDGetStatus(ddres));
        }
    }
}

int stdDisplay_FlipToGDISurface(void)
{
    HRESULT dderr = IDirectDraw4_FlipToGDISurface(stdDisplay_lpDD);
    if ( dderr != DD_OK ) {
        STDLOG_ERROR("Error %s when flipping to GDI surface.\n", stdDisplay_DDGetStatus(dderr));
    }
    return dderr;
}

int J3DAPI stdDisplay_CanRenderWindowed()
{
    if ( !stdDisplay_lpDD ) {
        return -1;
    }

    DDCAPS ddcaps;
    memset(&ddcaps, 0, sizeof(ddcaps));
    ddcaps.dwSize = sizeof(DDCAPS);
    if ( IDirectDraw4_GetCaps(stdDisplay_lpDD, &ddcaps, 0) != DD_OK ) {
        return -1;
    }

    return ddcaps.dwCaps2 & DDCAPS2_CANRENDERWINDOWED;
}


int J3DAPI stdDisplay_SetBufferClipper(int bFrontBuffer)
{
    IDirectDrawClipper* pClipper;

    if ( !stdDisplay_bFullscreen ) {
        return 0;
    }

    if ( bFrontBuffer )
    {
        IDirectDrawSurface4_GetClipper(stdDisplay_g_frontBuffer.surface.pDDSurf, &pClipper);
        if ( pClipper ) {
            return 1; // Clipper already set for buffer
        }
    }
    else
    {
        IDirectDrawSurface4_GetClipper(stdDisplay_g_backBuffer.surface.pDDSurf, &pClipper);
        if ( pClipper ) {
            return 1; // Clipper already set for buffer
        }
    }

    HRESULT ddres = IDirectDraw4_CreateClipper(stdDisplay_lpDD, 0, &pClipper, 0);
    if ( ddres != DD_OK )
    {
        STDLOG_ERROR("Error %s when creating DirectDraw clipper.\n", stdDisplay_DDGetStatus(ddres));
        return 0;
    }

    ddres =  IDirectDrawClipper_SetHWnd(pClipper, 0, stdWin95_GetWindow());
    if ( ddres != DD_OK )
    {
        IDirectDrawClipper_Release(pClipper);
        STDLOG_ERROR("Error %s when setting clipper hwnd.\n", stdDisplay_DDGetStatus(ddres));
        return 0;
    }

    LPDIRECTDRAWSURFACE4 pDDSurf = stdDisplay_g_frontBuffer.surface.pDDSurf;
    if ( !bFrontBuffer ) {
        pDDSurf = stdDisplay_g_backBuffer.surface.pDDSurf;
    }

    ddres = IDirectDrawSurface4_SetClipper(pDDSurf, pClipper);
    if ( ddres != DD_OK )
    {
        STDLOG_ERROR("Error %s when attaching clipper to primary surface.\n", stdDisplay_DDGetStatus(ddres));
        return 0;
    }

    IDirectDrawClipper_Release(pClipper);
    return 0;
}

HRESULT J3DAPI stdDisplay_RemoveBufferClipper(int bFrontBuffer)
{
    if ( !stdDisplay_bFullscreen ) {
        return 0;
    }

    if ( bFrontBuffer ) {
        return IDirectDrawSurface4_SetClipper(stdDisplay_g_frontBuffer.surface.pDDSurf, NULL);
    }

    return IDirectDrawSurface4_SetClipper(stdDisplay_g_backBuffer.surface.pDDSurf, NULL);
}

int J3DAPI stdDisplay_IsFullscreen()
{
    return stdDisplay_bFullscreen;
}

int J3DAPI stdDisplay_LockBackBuffer(void** pSurface, uint32_t* pWidth, uint32_t* pHeight, int32_t* pPitch)
{
    if ( !stdDisplay_bOpen || !stdDisplay_bModeSet ) {
        return 1;
    }

    DDSURFACEDESC2 dddesc = { 0 }; // Added: Init to zero
    dddesc.dwSize = sizeof(DDSURFACEDESC2);
    HRESULT ddres = IDirectDrawSurface4_Lock(
        stdDisplay_g_backBuffer.surface.pDDSurf,
        NULL,
        &dddesc,
        DDLOCK_NOSYSLOCK | DDLOCK_WRITEONLY | DDLOCK_WAIT,
        NULL
    );

    if ( ddres == DD_OK )
    {
        *pWidth   = dddesc.dwWidth;
        *pHeight  = dddesc.dwHeight;
        *pPitch   = dddesc.lPitch;
        *pSurface = dddesc.lpSurface;
        return 0;
    }

    // Added: Quick fix, trying to restore and re-lock the surface
    if ( ddres == DDERR_SURFACELOST )
    {
        stdDisplay_Refresh(1);
        ddres = IDirectDrawSurface4_Lock(
           stdDisplay_g_backBuffer.surface.pDDSurf,
           NULL,
           &dddesc,
           DDLOCK_NOSYSLOCK | DDLOCK_WRITEONLY | DDLOCK_WAIT,
           NULL
        );

    }

    if ( ddres == DD_OK )
    {
        *pWidth   = dddesc.dwWidth;
        *pHeight  = dddesc.dwHeight;
        *pPitch   = dddesc.lPitch;
        *pSurface = dddesc.lpSurface;
        return 0;
    }

    STDLOG_ERROR("Error %s when locking back buffer.\n", stdDisplay_DDGetStatus(ddres));
    return 1;
}

void stdDisplay_UnlockBackBuffer(void)
{
    if ( stdDisplay_bOpen && stdDisplay_bModeSet )
    {
        HRESULT ddres = IDirectDrawSurface4_Unlock(stdDisplay_g_backBuffer.surface.pDDSurf, 0);
        if ( ddres != DD_OK ) {
            STDLOG_ERROR("Error %s when unlocking back buffer.\n", stdDisplay_DDGetStatus(ddres));
        }
    }
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
    if ( (blue & 8) != 0 ) {
        blue = blue | 7;
    }

    return (red >> (colorInfo.redPosShiftRight & 0xFF) << (colorInfo.redPosShift & 0xFF))
        | (green >> (colorInfo.greenPosShiftRight & 0xFF) << (colorInfo.greenPosShift & 0xFF))
        | (blue >> (colorInfo.bluePosShiftRight & 0xFF) << (colorInfo.bluePosShift & 0xFF));
}

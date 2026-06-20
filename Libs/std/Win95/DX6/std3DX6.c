#include <std/Win95/std3D.h>
#include <std/Win95/stdDisplay.h>

#include <j3dcore/j3dhook.h>
#include <std/General/std.h>
#include <std/General/stdColor.h>
#include <std/General/stdMath.h>
#include <std/General/stdMemory.h>
#include <std/General/stdUtil.h>
#include <std/RTI/symbols.h>

#include <math.h>

#define STD3D_DEFAULT_MAX_VERTICES 512

static bool bStartup    = false;
static bool std3D_bOpen = false;

static LPDIRECTDRAW4 std3D_lpDD             = NULL;
static LPDIRECT3D3 std3D_pDirect3D          = NULL;
static LPDIRECT3DDEVICE3 std3D_pD3Device    = NULL;
static LPDIRECTDRAWPALETTE std3D_pDDPalette = NULL;

static size_t std3D_frameCount                  = 1;
static float std3D_zDepth                       = 0.0f;
static Std3DRenderState std3D_renderState       = 0;
static LPDIRECT3DTEXTURE2 std3D_pD3DTex         = NULL;
static Std3DMipmapFilterType std3D_mipmapFilter = -1;

static bool std3D_bRenderFog      = true;
static bool std3D_bFogTable       = false;
static float std3D_fogDepthFactor = 0.0f;
static float std3D_fogStartDepth  = 0.0f;
static float std3D_fogEndDepth    = 0.0f;

static bool std3D_bFindAllD3Devices = false;
static size_t std3D_curDevice       = 0;
static Device3D* std3D_pCurDevice   = NULL;

static size_t std3D_numDevices      = 0;
static Device3D std3D_aDevices[4]   = { 0 };

static size_t std3D_numCachedTextures       = 0;
static tSystemTexture* std3D_pFirstTexCache = NULL;
static tSystemTexture* std3D_pLastTexCache  = NULL;

static D3DRECT std3D_activeRect                = { 0 };
static LPDIRECT3DVIEWPORT3 std3D_lpD3DViewPort = NULL;

static size_t std3D_RGBATextureFormat;
static size_t std3D_RGBTextureFormat;
static size_t std3D_RGBAKeyTextureFormat;

static bool std3D_bHasRGBTextureFormat           = false;
static size_t std3D_numTextureFormats            = 0;
static StdTextureFormat std3D_aTextureFormats[8] = { 0 };

static const ColorInfo std3D_cfRGB565   = { STDCOLOR_RGB,  16, 5, 6, 5, 11,  5, 0, 3, 2, 3, 0, 0, 0 };
static const ColorInfo std3D_cfRGB5551  = { STDCOLOR_RGBA, 16, 5, 5, 5, 11,  6, 1, 3, 3, 3, 1, 0, 7 };
static const ColorInfo std3D_cfRGB4444  = { STDCOLOR_RGBA, 16, 4, 4, 4, 12,  8, 4, 4, 4, 4, 4, 0, 4 };

static const DXStatus std3D_aD3DStatusTbl[186] =
{
  { D3D_OK,                                "D3D_OK" },
  { D3DERR_BADMAJORVERSION,                 "D3DERR_BADMAJORVERSION" },
  { D3DERR_BADMINORVERSION,                 "D3DERR_BADMINORVERSION" },
  { D3DERR_INVALID_DEVICE,                  "D3DERR_INVALID_DEVICE" },
  { D3DERR_INITFAILED,                      "D3DERR_INITFAILED" },
  { D3DERR_DEVICEAGGREGATED,                "D3DERR_DEVICEAGGREGATED" },
  { D3DERR_EXECUTE_CREATE_FAILED,           "D3DERR_EXECUTE_CREATE_FAILED" },
  { D3DERR_EXECUTE_DESTROY_FAILED,          "D3DERR_EXECUTE_DESTROY_FAILED" },
  { D3DERR_EXECUTE_LOCK_FAILED,             "D3DERR_EXECUTE_LOCK_FAILED" },
  { D3DERR_EXECUTE_UNLOCK_FAILED,           "D3DERR_EXECUTE_UNLOCK_FAILED" },
  { D3DERR_EXECUTE_LOCKED,                  "D3DERR_EXECUTE_LOCKED" },
  { D3DERR_EXECUTE_NOT_LOCKED,              "D3DERR_EXECUTE_NOT_LOCKED" },
  { D3DERR_EXECUTE_FAILED,                  "D3DERR_EXECUTE_FAILED" },
  { D3DERR_EXECUTE_CLIPPED_FAILED,          "D3DERR_EXECUTE_CLIPPED_FAILED" },
  { D3DERR_TEXTURE_NO_SUPPORT,              "D3DERR_TEXTURE_NO_SUPPORT" },
  { D3DERR_TEXTURE_CREATE_FAILED,           "D3DERR_TEXTURE_CREATE_FAILED" },
  { D3DERR_TEXTURE_DESTROY_FAILED,          "D3DERR_TEXTURE_DESTROY_FAILED" },
  { D3DERR_TEXTURE_LOCK_FAILED,             "D3DERR_TEXTURE_LOCK_FAILED" },
  { D3DERR_TEXTURE_UNLOCK_FAILED,           "D3DERR_TEXTURE_UNLOCK_FAILED" },
  { D3DERR_TEXTURE_LOAD_FAILED,             "D3DERR_TEXTURE_LOAD_FAILED" },
  { D3DERR_TEXTURE_SWAP_FAILED,             "D3DERR_TEXTURE_SWAP_FAILED" },
  { D3DERR_TEXTURE_LOCKED,                  "D3DERR_TEXTURE_LOCKED" },
  { D3DERR_TEXTURE_NOT_LOCKED,              "D3DERR_TEXTURE_NOT_LOCKED" },
  { D3DERR_TEXTURE_GETSURF_FAILED,          "D3DERR_TEXTURE_GETSURF_FAILED" },
  { D3DERR_MATRIX_CREATE_FAILED,            "D3DERR_MATRIX_CREATE_FAILED" },
  { D3DERR_MATRIX_DESTROY_FAILED,           "D3DERR_MATRIX_DESTROY_FAILED" },
  { D3DERR_MATRIX_SETDATA_FAILED,           "D3DERR_MATRIX_SETDATA_FAILED" },
  { D3DERR_MATRIX_GETDATA_FAILED,           "D3DERR_MATRIX_GETDATA_FAILED" },
  { D3DERR_SETVIEWPORTDATA_FAILED,          "D3DERR_SETVIEWPORTDATA_FAILED" },
  { D3DERR_INVALIDCURRENTVIEWPORT,          "D3DERR_INVALIDCURRENTVIEWPORT" },
  { D3DERR_INVALIDPRIMITIVETYPE,            "D3DERR_INVALIDPRIMITIVETYPE" },
  { D3DERR_INVALIDVERTEXTYPE,               "D3DERR_INVALIDVERTEXTYPE" },
  { D3DERR_TEXTURE_BADSIZE,                 "D3DERR_TEXTURE_BADSIZE" },
  { D3DERR_INVALIDRAMPTEXTURE,              "D3DERR_INVALIDRAMPTEXTURE" },
  { D3DERR_MATERIAL_CREATE_FAILED,          "D3DERR_MATERIAL_CREATE_FAILED" },
  { D3DERR_MATERIAL_DESTROY_FAILED,         "D3DERR_MATERIAL_DESTROY_FAILED" },
  { D3DERR_MATERIAL_SETDATA_FAILED,         "D3DERR_MATERIAL_SETDATA_FAILED" },
  { D3DERR_MATERIAL_GETDATA_FAILED,         "D3DERR_MATERIAL_GETDATA_FAILED" },
  { D3DERR_INVALIDPALETTE,                  "D3DERR_INVALIDPALETTE" },
  { D3DERR_ZBUFF_NEEDS_SYSTEMMEMORY,        "D3DERR_ZBUFF_NEEDS_SYSTEMMEMORY" },
  { D3DERR_ZBUFF_NEEDS_VIDEOMEMORY,         "D3DERR_ZBUFF_NEEDS_VIDEOMEMORY" },
  { D3DERR_SURFACENOTINVIDMEM,              "D3DERR_SURFACENOTINVIDMEM" },
  { D3DERR_LIGHT_SET_FAILED,                "D3DERR_LIGHT_SET_FAILED" },
  { D3DERR_LIGHTHASVIEWPORT,                "D3DERR_LIGHTHASVIEWPORT" },
  { D3DERR_LIGHTNOTINTHISVIEWPORT,          "D3DERR_LIGHTNOTINTHISVIEWPORT" },
  { D3DERR_SCENE_IN_SCENE,                  "D3DERR_SCENE_IN_SCENE" },
  { D3DERR_SCENE_NOT_IN_SCENE,              "D3DERR_SCENE_NOT_IN_SCENE" },
  { D3DERR_SCENE_BEGIN_FAILED,              "D3DERR_SCENE_BEGIN_FAILED" },
  { D3DERR_SCENE_END_FAILED,                "D3DERR_SCENE_END_FAILED" },
  { D3DERR_INBEGIN,                         "D3DERR_INBEGIN" },
  { D3DERR_NOTINBEGIN,                      "D3DERR_NOTINBEGIN" },
  { D3DERR_NOVIEWPORTS,                     "D3DERR_NOVIEWPORTS" },
  { D3DERR_VIEWPORTDATANOTSET,              "D3DERR_VIEWPORTDATANOTSET" },
  { D3DERR_VIEWPORTHASNODEVICE,             "D3DERR_VIEWPORTHASNODEVICE" },
  { D3DERR_NOCURRENTVIEWPORT,               "D3DERR_NOCURRENTVIEWPORT" },
  { D3DERR_INVALIDVERTEXFORMAT,             "D3DERR_INVALIDVERTEXFORMAT" },
  { D3DERR_COLORKEYATTACHED,                "D3DERR_COLORKEYATTACHED" },
  { D3DERR_VERTEXBUFFEROPTIMIZED,           "D3DERR_VERTEXBUFFEROPTIMIZED" },
  { D3DERR_VBUF_CREATE_FAILED,              "D3DERR_VBUF_CREATE_FAILED" },
  { D3DERR_VERTEXBUFFERLOCKED,              "D3DERR_VERTEXBUFFERLOCKED" },
  { D3DERR_ZBUFFER_NOTPRESENT,              "D3DERR_ZBUFFER_NOTPRESENT" },
  { D3DERR_STENCILBUFFER_NOTPRESENT,        "D3DERR_STENCILBUFFER_NOTPRESENT" },
  { D3DERR_WRONGTEXTUREFORMAT,              "D3DERR_WRONGTEXTUREFORMAT" },
  { D3DERR_UNSUPPORTEDCOLOROPERATION,       "D3DERR_UNSUPPORTEDCOLOROPERATION" },
  { D3DERR_UNSUPPORTEDCOLORARG,             "D3DERR_UNSUPPORTEDCOLORARG" },
  { D3DERR_UNSUPPORTEDALPHAOPERATION,       "D3DERR_UNSUPPORTEDALPHAOPERATION" },
  { D3DERR_UNSUPPORTEDALPHAARG,             "D3DERR_UNSUPPORTEDALPHAARG" },
  { D3DERR_TOOMANYOPERATIONS,               "D3DERR_TOOMANYOPERATIONS" },
  { D3DERR_CONFLICTINGTEXTUREFILTER,        "D3DERR_CONFLICTINGTEXTUREFILTER" },
  { D3DERR_UNSUPPORTEDFACTORVALUE,          "D3DERR_UNSUPPORTEDFACTORVALUE" },
  { D3DERR_CONFLICTINGRENDERSTATE,          "D3DERR_CONFLICTINGRENDERSTATE" },
  { D3DERR_UNSUPPORTEDTEXTUREFILTER,        "D3DERR_UNSUPPORTEDTEXTUREFILTER" },
  { D3DERR_TOOMANYPRIMITIVES,               "D3DERR_TOOMANYPRIMITIVES" },
  { D3DERR_INVALIDMATRIX,                   "D3DERR_INVALIDMATRIX" },
  { D3DERR_TOOMANYVERTICES,                 "D3DERR_TOOMANYVERTICES" },
  { D3DERR_CONFLICTINGTEXTUREPALETTE,       "D3DERR_CONFLICTINGTEXTUREPALETTE" },
  { DDERR_ALREADYINITIALIZED,               "DDERR_ALREADYINITIALIZED" },
  { DDERR_CANNOTATTACHSURFACE,              "DDERR_CANNOTATTACHSURFACE" },
  { DDERR_CANNOTDETACHSURFACE,              "DDERR_CANNOTDETACHSURFACE" },
  { DDERR_CURRENTLYNOTAVAIL,                "DDERR_CURRENTLYNOTAVAIL" },
  { DDERR_EXCEPTION,                        "DDERR_EXCEPTION" },
  { DDERR_GENERIC,                          "DDERR_GENERIC" },
  { DDERR_HEIGHTALIGN,                      "DDERR_HEIGHTALIGN" },
  { DDERR_INCOMPATIBLEPRIMARY,              "DDERR_INCOMPATIBLEPRIMARY" },
  { DDERR_INVALIDCAPS,                      "DDERR_INVALIDCAPS" },
  { DDERR_INVALIDCLIPLIST,                  "DDERR_INVALIDCLIPLIST" },
  { DDERR_INVALIDMODE,                      "DDERR_INVALIDMODE" },
  { DDERR_INVALIDOBJECT,                    "DDERR_INVALIDOBJECT" },
  { DDERR_INVALIDPARAMS,                    "DDERR_INVALIDPARAMS" },
  { DDERR_INVALIDPIXELFORMAT,               "DDERR_INVALIDPIXELFORMAT" },
  { DDERR_INVALIDRECT,                      "DDERR_INVALIDRECT" },
  { DDERR_LOCKEDSURFACES,                   "DDERR_LOCKEDSURFACES" },
  { DDERR_NO3D,                             "DDERR_NO3D" },
  { DDERR_NOALPHAHW,                        "DDERR_NOALPHAHW" },
  { DDERR_NOCLIPLIST,                       "DDERR_NOCLIPLIST" },
  { DDERR_NOCOLORCONVHW,                    "DDERR_NOCOLORCONVHW" },
  { DDERR_NOCOOPERATIVELEVELSET,            "DDERR_NOCOOPERATIVELEVELSET" },
  { DDERR_NOCOLORKEY,                       "DDERR_NOCOLORKEY" },
  { DDERR_NOCOLORKEYHW,                     "DDERR_NOCOLORKEYHW" },
  { DDERR_NODIRECTDRAWSUPPORT,              "DDERR_NODIRECTDRAWSUPPORT" },
  { DDERR_NOEXCLUSIVEMODE,                  "DDERR_NOEXCLUSIVEMODE" },
  { DDERR_NOFLIPHW,                         "DDERR_NOFLIPHW" },
  { DDERR_NOGDI,                            "DDERR_NOGDI" },
  { DDERR_NOMIRRORHW,                       "DDERR_NOMIRRORHW" },
  { DDERR_NOTFOUND,                         "DDERR_NOTFOUND" },
  { DDERR_NOOVERLAYHW,                      "DDERR_NOOVERLAYHW" },
  { DDERR_OVERLAPPINGRECTS,                 "DDERR_OVERLAPPINGRECTS" },
  { DDERR_NORASTEROPHW,                     "DDERR_NORASTEROPHW" },
  { DDERR_NOROTATIONHW,                     "DDERR_NOROTATIONHW" },
  { DDERR_NOSTRETCHHW,                      "DDERR_NOSTRETCHHW" },
  { DDERR_NOT4BITCOLOR,                     "DDERR_NOT4BITCOLOR" },
  { DDERR_NOT4BITCOLORINDEX,                "DDERR_NOT4BITCOLORINDEX" },
  { DDERR_NOT8BITCOLOR,                     "DDERR_NOT8BITCOLOR" },
  { DDERR_NOTEXTUREHW,                      "DDERR_NOTEXTUREHW" },
  { DDERR_NOVSYNCHW,                        "DDERR_NOVSYNCHW" },
  { DDERR_NOZBUFFERHW,                      "DDERR_NOZBUFFERHW" },
  { DDERR_NOZOVERLAYHW,                     "DDERR_NOZOVERLAYHW" },
  { DDERR_OUTOFCAPS,                        "DDERR_OUTOFCAPS" },
  { DDERR_OUTOFMEMORY,                      "DDERR_OUTOFMEMORY" },
  { DDERR_OUTOFVIDEOMEMORY,                 "DDERR_OUTOFVIDEOMEMORY" },
  { DDERR_OVERLAYCANTCLIP,                  "DDERR_OVERLAYCANTCLIP" },
  { DDERR_OVERLAYCOLORKEYONLYONEACTIVE,     "DDERR_OVERLAYCOLORKEYONLYONEACTIVE" },
  { DDERR_PALETTEBUSY,                      "DDERR_PALETTEBUSY" },
  { DDERR_COLORKEYNOTSET,                   "DDERR_COLORKEYNOTSET" },
  { DDERR_SURFACEALREADYATTACHED,           "DDERR_SURFACEALREADYATTACHED" },
  { DDERR_SURFACEALREADYDEPENDENT,          "DDERR_SURFACEALREADYDEPENDENT" },
  { DDERR_SURFACEBUSY,                      "DDERR_SURFACEBUSY" },
  { DDERR_CANTLOCKSURFACE,                  "DDERR_CANTLOCKSURFACE" },
  { DDERR_SURFACEISOBSCURED,                "DDERR_SURFACEISOBSCURED" },
  { DDERR_SURFACELOST,                      "DDERR_SURFACELOST" },
  { DDERR_SURFACENOTATTACHED,               "DDERR_SURFACENOTATTACHED" },
  { DDERR_TOOBIGHEIGHT,                     "DDERR_TOOBIGHEIGHT" },
  { DDERR_TOOBIGSIZE,                       "DDERR_TOOBIGSIZE" },
  { DDERR_TOOBIGWIDTH,                      "DDERR_TOOBIGWIDTH" },
  { DDERR_UNSUPPORTED,                      "DDERR_UNSUPPORTED" },
  { DDERR_UNSUPPORTEDFORMAT,                "DDERR_UNSUPPORTEDFORMAT" },
  { DDERR_UNSUPPORTEDMASK,                  "DDERR_UNSUPPORTEDMASK" },
  { DDERR_INVALIDSTREAM,                    "DDERR_INVALIDSTREAM" },
  { DDERR_VERTICALBLANKINPROGRESS,          "DDERR_VERTICALBLANKINPROGRESS" },
  { DDERR_WASSTILLDRAWING,                  "DDERR_WASSTILLDRAWING" },
  { DDERR_XALIGN,                           "DDERR_XALIGN" },
  { DDERR_INVALIDDIRECTDRAWGUID,            "DDERR_INVALIDDIRECTDRAWGUID" },
  { DDERR_DIRECTDRAWALREADYCREATED,         "DDERR_DIRECTDRAWALREADYCREATED" },
  { DDERR_NODIRECTDRAWHW,                   "DDERR_NODIRECTDRAWHW" },
  { DDERR_PRIMARYSURFACEALREADYEXISTS,      "DDERR_PRIMARYSURFACEALREADYEXISTS" },
  { DDERR_NOEMULATION,                      "DDERR_NOEMULATION" },
  { DDERR_REGIONTOOSMALL,                   "DDERR_REGIONTOOSMALL" },
  { DDERR_CLIPPERISUSINGHWND,               "DDERR_CLIPPERISUSINGHWND" },
  { DDERR_NOCLIPPERATTACHED,                "DDERR_NOCLIPPERATTACHED" },
  { DDERR_NOHWND,                           "DDERR_NOHWND" },
  { DDERR_HWNDSUBCLASSED,                   "DDERR_HWNDSUBCLASSED" },
  { DDERR_HWNDALREADYSET,                   "DDERR_HWNDALREADYSET" },
  { DDERR_NOPALETTEATTACHED,                "DDERR_NOPALETTEATTACHED" },
  { DDERR_NOPALETTEHW,                      "DDERR_NOPALETTEHW" },
  { DDERR_BLTFASTCANTCLIP,                  "DDERR_BLTFASTCANTCLIP" },
  { DDERR_NOBLTHW,                          "DDERR_NOBLTHW" },
  { DDERR_NODDROPSHW,                       "DDERR_NODDROPSHW" },
  { DDERR_OVERLAYNOTVISIBLE,                "DDERR_OVERLAYNOTVISIBLE" },
  { DDERR_NOOVERLAYDEST,                    "DDERR_NOOVERLAYDEST" },
  { DDERR_INVALIDPOSITION,                  "DDERR_INVALIDPOSITION" },
  { DDERR_NOTAOVERLAYSURFACE,               "DDERR_NOTAOVERLAYSURFACE" },
  { DDERR_EXCLUSIVEMODEALREADYSET,          "DDERR_EXCLUSIVEMODEALREADYSET" },
  { DDERR_NOTFLIPPABLE,                     "DDERR_NOTFLIPPABLE" },
  { DDERR_CANTDUPLICATE,                    "DDERR_CANTDUPLICATE" },
  { DDERR_NOTLOCKED,                        "DDERR_NOTLOCKED" },
  { DDERR_CANTCREATEDC,                     "DDERR_CANTCREATEDC" },
  { DDERR_NODC,                             "DDERR_NODC" },
  { DDERR_WRONGMODE,                        "DDERR_WRONGMODE" },
  { DDERR_IMPLICITLYCREATED,                "DDERR_IMPLICITLYCREATED" },
  { DDERR_NOTPALETTIZED,                    "DDERR_NOTPALETTIZED" },
  { DDERR_UNSUPPORTEDMODE,                  "DDERR_UNSUPPORTEDMODE" },
  { DDERR_NOMIPMAPHW,                       "DDERR_NOMIPMAPHW" },
  { DDERR_INVALIDSURFACETYPE,               "DDERR_INVALIDSURFACETYPE" },
  { DDERR_NOOPTIMIZEHW,                     "DDERR_NOOPTIMIZEHW" },
  { DDERR_NOTLOADED,                        "DDERR_NOTLOADED" },
  { DDERR_NOFOCUSWINDOW,                    "DDERR_NOFOCUSWINDOW" },
  { DDERR_DCALREADYCREATED,                 "DDERR_DCALREADYCREATED" },
  { DDERR_NONONLOCALVIDMEM,                 "DDERR_NONONLOCALVIDMEM" },
  { DDERR_CANTPAGELOCK,                     "DDERR_CANTPAGELOCK" },
  { DDERR_CANTPAGEUNLOCK,                   "DDERR_CANTPAGEUNLOCK" },
  { DDERR_NOTPAGELOCKED,                    "DDERR_NOTPAGELOCKED" },
  { DDERR_MOREDATA,                         "DDERR_MOREDATA" },
  { DDERR_EXPIRED,                          "DDERR_EXPIRED" },
  { DDERR_VIDEONOTACTIVE,                   "DDERR_VIDEONOTACTIVE" },
  { DDERR_DEVICEDOESNTOWNSURFACE,           "DDERR_DEVICEDOESNTOWNSURFACE" },
  { DDERR_NOTINITIALIZED,                   "DDERR_NOTINITIALIZED" }
};

static int std3D_InitRenderState(void);
static HRESULT CALLBACK std3D_EnumZBufferFormatsCallback(LPDDPIXELFORMAT lpDDPixFmt, DDPIXELFORMAT* lpContext);
static HRESULT CALLBACK std3D_D3DEnumDevicesCallback(GUID* lpGuid, LPSTR lpDeviceDescription, LPSTR lpDeviceName, LPD3DDEVICEDESC lpD3DHalDDesc, LPD3DDEVICEDESC lpD3DHelDDesc, LPVOID lpContext);
static HRESULT CALLBACK std3D_EnumTextureFormatsCallback(LPDDPIXELFORMAT lpDDPixFmt, LPVOID lpContext);

static int std3D_CreateViewport(void);
static void J3DAPI std3D_GetZBufferFormat(DDPIXELFORMAT* pPixelFormat);
static void J3DAPI std3D_AddTextureToCacheList(tSystemTexture* pTexture);
static void J3DAPI std3D_RemoveTextureFromCacheList(tSystemTexture* pCacheTexture);
static int J3DAPI std3D_PurgeTextureCache(size_t size);

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
    J3D_HOOKFUNC(std3D_GetZBufferFormat);
    J3D_HOOKFUNC(std3D_EnumZBufferFormatsCallback);
    J3D_HOOKFUNC(std3D_D3DEnumDevicesCallback);
    J3D_HOOKFUNC(std3D_EnumTextureFormatsCallback);
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
    STD_ZEROMEM(&std3D_g_maxVertices, sizeof(std3D_g_maxVertices));
}

int std3D_Startup(void)
{
    STD_ASSERTREL(bStartup == 0);
    STD_ZEROMEM(std3D_aTextureFormats, sizeof(std3D_aTextureFormats));
    STD_ZEROMEM(std3D_aDevices, sizeof(std3D_aDevices));

    std3D_lpDD = stdDisplay_GetSystemDevice();
    if ( !std3D_lpDD )
    {
        STDLOG_ERROR("DDraw device not created yet!\n");
        return 0;
    }

    HRESULT ddres = IDirectDraw4_QueryInterface(std3D_lpDD, &IID_IDirect3D3, &std3D_pDirect3D);
    if ( ddres != DD_OK )
    {
        STDLOG_ERROR("Error %s creating Direct3D interface object.\n", std3D_D3DGetStatus(ddres));
        return 0;
    }

    std3D_numDevices = 0;
    ddres = IDirect3D3_EnumDevices(std3D_pDirect3D, std3D_D3DEnumDevicesCallback, 0);
    if ( ddres != DD_OK )
    {
        STDLOG_ERROR("Error %s when enumerating D3D devices.\n", std3D_D3DGetStatus(ddres));
        // Fixed: Release the Direct3D interface when device enumeration fails during startup.
        IDirect3D3_Release(std3D_pDirect3D);
        std3D_pDirect3D = NULL;
        return 0;
    }

    if ( std3D_numDevices == 0 )
    {
        // Fixed: Release the Direct3D interface when startup finds no usable devices.
        IDirect3D3_Release(std3D_pDirect3D);
        std3D_pDirect3D = NULL;
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

    if ( std3D_pDirect3D )
    {
        IDirect3D3_Release(std3D_pDirect3D);
    }
    std3D_pD3Device = NULL;

    STD_ZEROMEM(std3D_aTextureFormats, sizeof(std3D_aTextureFormats));
    STD_ZEROMEM(std3D_aDevices, sizeof(std3D_aDevices));

    std3D_pDirect3D  = NULL;
    std3D_numDevices = 0;
    bStartup         = false;
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

    if ( deviceNum >= std3D_numDevices )
    {
        return 0;
    }

    std3D_curDevice  = deviceNum;
    std3D_pCurDevice = &std3D_aDevices[deviceNum];
    // Fixed: Route failures after device selection through cleanup so a partial open can be retried safely.
    if ( !std3D_pCurDevice->hasZBuffer )
    {
        goto error;
    }

    DDPIXELFORMAT pixelFormat = { 0 };
    std3D_GetZBufferFormat(&pixelFormat);
    if ( stdDisplay_CreateZBuffer(&pixelFormat, std3D_pCurDevice->bHAL == 0) )
    {
        STDLOG_ERROR("Error creating Z buffer.\n");
        goto error;
    }

    // Init palette
    PALETTEENTRY aPalette[256];
    for ( size_t i = 0; i < STD_ARRAYLEN(aPalette); ++i )
    {
        aPalette[i].peFlags = D3DPAL_RESERVED;
        aPalette[i].peBlue  = 254;
        aPalette[i].peGreen = 254;
        aPalette[i].peRed   = 254;
    }

    HRESULT dres = IDirectDraw4_CreatePalette(
        std3D_lpDD,
        DDPCAPS_ALLOW256 | DDPCAPS_INITIALIZE | DDPCAPS_8BIT,
        (LPPALETTEENTRY)aPalette,
        &std3D_pDDPalette,
        NULL
    );

    if ( dres != DD_OK )
    {
        STDLOG_ERROR("Error %s creating palette.\n", std3D_D3DGetStatus(dres));
        goto error;
    }

    dres = IDirect3D3_CreateDevice(
        std3D_pDirect3D,
        &std3D_pCurDevice->duid,
        stdDisplay_g_backBuffer.surface.pSysSurface,
        &std3D_pD3Device,
        NULL
    );

    if ( dres != D3D_OK )
    {
        STDLOG_ERROR("Error %s creating Direct3D device.\n", std3D_D3DGetStatus(dres));
        goto error;
    }

    std3D_numTextureFormats    = 0;
    std3D_bHasRGBTextureFormat = false;
    dres = IDirect3DDevice3_EnumTextureFormats(std3D_pD3Device, std3D_EnumTextureFormatsCallback, 0);
    if ( dres != D3D_OK )
    {
        STDLOG_ERROR("Error %s when enumerating D3D device texture formats.\n", std3D_D3DGetStatus(dres));
        goto error;
    }

    if ( !std3D_numTextureFormats || !std3D_bHasRGBTextureFormat )
    {
        STDLOG_ERROR("Error: no 16 texture formats found.\n");
        goto error;
    }

    if ( !std3D_CreateViewport() )
    {
        STDLOG_ERROR("Error creating viewport.\n");
        goto error;
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
#ifdef J3D_QOL_IMPROVEMENTS
    // Altered: Use 32 bit formats; was using 16 bit
    std3D_RGBTextureFormat     = std3D_FindClosestFormat(&stdColor_cfBGR8888);
    std3D_RGBAKeyTextureFormat = std3D_FindClosestFormat(&stdColor_cfRGBA8888);
    std3D_RGBATextureFormat    = std3D_FindClosestFormat(&stdColor_cfRGBA8888);
#else
    std3D_RGBTextureFormat     = std3D_FindClosestFormat(&std3D_cfRGB565);
    std3D_RGBAKeyTextureFormat = std3D_FindClosestFormat(&std3D_cfRGB5551);
    std3D_RGBATextureFormat    = std3D_FindClosestFormat(&std3D_cfRGB4444);
#endif

    if ( std3D_aTextureFormats[std3D_RGBAKeyTextureFormat].ci.alphaBPP == 0
        && std3D_pCurDevice->bColorkeyTextureSupported )
    {
        std3D_aTextureFormats[std3D_RGBAKeyTextureFormat].bColorKey = 1;
        std3D_aTextureFormats[std3D_RGBAKeyTextureFormat].pColorKey = NULL;
    }

    if ( std3D_aTextureFormats[std3D_RGBATextureFormat].ci.alphaBPP == 0
        && std3D_pCurDevice->bColorkeyTextureSupported )
    {
        std3D_aTextureFormats[std3D_RGBATextureFormat].bColorKey = 1;
        std3D_aTextureFormats[std3D_RGBATextureFormat].pColorKey = NULL;
    }

    std3D_mipmapFilter = -1; // Fixed: Reset mipmap filter to be able to set filter to newly created device in std3D_SetMipmapFilter
    if ( !std3D_InitRenderState() )
    {
        STDLOG_ERROR("Error initializing render state.\n");
        goto error;
    }

    if ( stdDisplay_GetTextureMemory(&std3D_pCurDevice->totalMemory, &std3D_pCurDevice->availableMemory) )
    {
        // Since we failed to get texture memory info indicate that opening failed
        goto error;
    }

    size_t memFree  = 0;
    size_t memTotal = 0;
    stdDisplay_GetTotalMemory(&memTotal, &memFree);

    STDLOG_STATUS("Texture Ram  Total: %u bytes  Free: %u bytes.\n", std3D_pCurDevice->totalMemory, std3D_pCurDevice->availableMemory);
    STDLOG_STATUS("Video Ram Total: %u bytes  Free: %u bytes.\n", memTotal, memFree);

    std3D_bOpen = true;
    return 1;

error:
    std3D_Close();
    return 0;
}

void std3D_Close(void)
{
    std3D_ResetTextureCache();
    if ( std3D_pDDPalette )
    {
        IDirectDrawPalette_Release(std3D_pDDPalette);
        std3D_pDDPalette = NULL;
    }

    if ( std3D_lpD3DViewPort )
    {
        IDirect3DViewport3_Release(std3D_lpD3DViewPort);
        std3D_lpD3DViewPort = NULL;
    }

    if ( std3D_pD3Device )
    {
        IDirect3DDevice3_Release(std3D_pD3Device);
        std3D_pD3Device = NULL;
    }

    std3D_mipmapFilter         = -1; // Added: invalidate mipmap filter
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

int std3D_StartScene(void)
{
    ++std3D_frameCount;

    HRESULT d3dres = IDirect3DDevice3_BeginScene(std3D_pD3Device);
    if ( d3dres != D3D_OK )
    {
        STDLOG_ERROR("Error %s beginning scene.\n", std3D_D3DGetStatus(d3dres));
    }

    std3D_pD3DTex = NULL;
    return d3dres;
}

void std3D_EndScene(void)
{
    HRESULT d3dres = IDirect3DDevice3_EndScene(std3D_pD3Device);
    if ( d3dres != D3D_OK )
    {
        STDLOG_ERROR("Error %s ending scene.\n", std3D_D3DGetStatus(d3dres));
    }
    std3D_pD3DTex = NULL;
}

void J3DAPI std3D_DrawRenderList(tSysTexture* pTex, Std3DRenderState rdflags, LPD3DTLVERTEX aVerts, size_t numVerts, LPWORD aIndices, size_t numIndices)
{
    HRESULT d3dres;

    if ( numVerts > (unsigned int)std3D_g_maxVertices )
    {
        STDLOG_ERROR("Error %d > %d maxVertices.\n", numVerts, std3D_g_maxVertices);
        return;
    }

    std3D_SetRenderState(rdflags);

    // Fixed: EndScene clears the cached texture pointer without unbinding the device texture, so untextured draws must bind NULL explicitly.
    if ( pTex != std3D_pD3DTex || !pTex )
    {
        d3dres = IDirect3DDevice3_SetTexture(std3D_pD3Device, 0, pTex);
        if ( d3dres != D3D_OK )
        {
            STDLOG_ERROR("Error %s SetRenderState.\n", std3D_D3DGetStatus(d3dres));
        }
        else
        {
            std3D_pD3DTex = pTex;
        }
    }

    if ( std3D_bFogTable )
    {
        D3DTLVERTEX* pCurVert = aVerts;
        for ( size_t i = 0; i < numVerts; ++i )
        {
            if ( pCurVert->rhw > 0.0 )
            {
                float depth = (std3D_fogEndDepth - pCurVert->rhw * std3D_zDepth) * std3D_fogDepthFactor;
                if ( depth < 1.0 )
                {
                    if ( depth >= 0.0 )
                    {
                        pCurVert->specular = (int)((1.0 - depth) * 255.0) << 24;
                    }
                    else
                    {
                        pCurVert->specular = 0xFF000000;
                    }
                }
                else
                {
                    pCurVert->specular = 0;
                }
            }
            else
            {
                pCurVert->specular = 0xFF000000;
            }

            ++pCurVert;
        }
    }

    d3dres = IDirect3DDevice3_DrawIndexedPrimitive(
        std3D_pD3Device,
        D3DPT_TRIANGLELIST,
        D3DFVF_TLVERTEX,
        aVerts,
        numVerts,
        aIndices,
        numIndices,
        D3DDP_DONOTLIGHT | D3DDP_DONOTUPDATEEXTENTS
    );

    if ( d3dres != D3D_OK )
    {
        STDLOG_ERROR("Error %s DrawPrim.\n", std3D_D3DGetStatus(d3dres));
    }
}

void std3D_SetWireframeRenderState(void)
{
    Std3DRenderState rdstate = std3D_renderState & ~(STD3D_RS_FOG_ENABLED | STD3D_RS_UNKNOWN_400 | STD3D_RS_UNKNOWN_200);
    std3D_SetRenderState(rdstate);

    HRESULT d3dres = IDirect3DDevice3_SetTexture(std3D_pD3Device, 0, NULL);
    if ( d3dres != D3D_OK )
    {
        STDLOG_ERROR("Error %s SetRenderState.\n", std3D_D3DGetStatus(d3dres));
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

    HRESULT d3dres = IDirect3DDevice3_DrawPrimitive(
        std3D_pD3Device,
        D3DPT_LINESTRIP,
        D3DFVF_TLVERTEX,
        aVerts,
        numVerts,
        D3DDP_DONOTLIGHT | D3DDP_DONOTUPDATEEXTENTS
    );

    if ( d3dres != D3D_OK )
    {
        STDLOG_ERROR("Error %s DrawPrim.\n", std3D_D3DGetStatus(d3dres));
    }
}

void J3DAPI std3D_DrawPointList(LPD3DTLVERTEX aVerts, size_t numVerts)
{
    if ( numVerts > std3D_g_maxVertices )
    {
        STDLOG_ERROR("Error %d > %d maxVertices.\n", numVerts, std3D_g_maxVertices);
        return;
    }

    HRESULT ddres = IDirect3DDevice3_DrawPrimitive(
        std3D_pD3Device,
        D3DPT_POINTLIST,
        D3DFVF_TLVERTEX,
        aVerts,
        numVerts,
        D3DDP_DONOTLIGHT | D3DDP_DONOTUPDATEEXTENTS
    );

    if ( ddres != D3D_OK )
    {
        STDLOG_ERROR("Error %s DrawPrim.\n", std3D_D3DGetStatus(ddres));
    }
}

void J3DAPI std3D_SetRenderState(Std3DRenderState rdflags)
{
    if ( std3D_renderState != rdflags )
    {
        if ( (std3D_renderState & STD3D_RS_ZWRITE_DISABLED) != (rdflags & STD3D_RS_ZWRITE_DISABLED) )
        {
            if ( (rdflags & STD3D_RS_ZWRITE_DISABLED) != 0 )
            {
                IDirect3DDevice3_SetRenderState(std3D_pD3Device, D3DRENDERSTATE_ZWRITEENABLE, FALSE);
            }
            else
            {
                IDirect3DDevice3_SetRenderState(std3D_pD3Device, D3DRENDERSTATE_ZWRITEENABLE, TRUE);
            }
        }

        if ( (std3D_renderState & STD3D_RS_TEX_CPAMP_U) != (rdflags & STD3D_RS_TEX_CPAMP_U) )
        {
            if ( (rdflags & STD3D_RS_TEX_CPAMP_U) != 0 )
            {
                IDirect3DDevice3_SetTextureStageState(std3D_pD3Device, 0, D3DTSS_ADDRESSU, D3DTADDRESS_CLAMP);
            }
            else
            {
                IDirect3DDevice3_SetTextureStageState(std3D_pD3Device, 0, D3DTSS_ADDRESSU, D3DTADDRESS_WRAP);
            }
        }

        if ( (std3D_renderState & STD3D_RS_TEX_CPAMP_V) != (rdflags & STD3D_RS_TEX_CPAMP_V) )
        {
            if ( (rdflags & STD3D_RS_TEX_CPAMP_V) != 0 )
            {
                IDirect3DDevice3_SetTextureStageState(std3D_pD3Device, 0, D3DTSS_ADDRESSV, D3DTADDRESS_CLAMP);
            }
            else
            {
                IDirect3DDevice3_SetTextureStageState(std3D_pD3Device, 0, D3DTSS_ADDRESSV, D3DTADDRESS_WRAP);
            }
        }

        if ( (std3D_renderState & STD3D_RS_FOG_ENABLED) != (rdflags & STD3D_RS_FOG_ENABLED) )
        {
            if ( (rdflags & STD3D_RS_FOG_ENABLED) != 0 && std3D_bRenderFog )
            {
                IDirect3DDevice3_SetRenderState(std3D_pD3Device, D3DRENDERSTATE_FOGENABLE, TRUE);
            }
            else
            {
                IDirect3DDevice3_SetRenderState(std3D_pD3Device, D3DRENDERSTATE_FOGENABLE, FALSE);
            }
        }

        // Update texture filtering
    #ifdef J3D_QOL_IMPROVEMENTS 
        // Added: Added handling anisotropic filtering
        if ( (std3D_renderState & STD3D_RS_TEXFILTER_ANISOTROPIC) != (rdflags & STD3D_RS_TEXFILTER_ANISOTROPIC) )
        {
            if ( (rdflags & STD3D_RS_TEXFILTER_ANISOTROPIC) != 0 && std3D_pCurDevice->bAnisotropicFilteringSupported )
            {
                if ( (std3D_pCurDevice->d3dDesc.dpcTriCaps.dwTextureFilterCaps & D3DPTFILTERCAPS_MAGFANISOTROPIC) != 0 )
                {
                    IDirect3DDevice3_SetTextureStageState(std3D_pD3Device, 0, D3DTSS_MAGFILTER, D3DTFG_ANISOTROPIC);
                }
                // Fixed: Linear magnification support, not point support, gates the linear fallback.
                else if ( (std3D_pCurDevice->d3dDesc.dpcTriCaps.dwTextureFilterCaps & D3DPTFILTERCAPS_MAGFLINEAR) != 0 )
                {
                    IDirect3DDevice3_SetTextureStageState(std3D_pD3Device, 0, D3DTSS_MAGFILTER, D3DTFG_LINEAR);
                }
                else if ( (std3D_pCurDevice->d3dDesc.dpcTriCaps.dwTextureFilterCaps & D3DPTFILTERCAPS_MAGFPOINT) != 0 )
                {
                    IDirect3DDevice3_SetTextureStageState(std3D_pD3Device, 0, D3DTSS_MAGFILTER, D3DTFG_POINT);
                }

                IDirect3DDevice3_SetTextureStageState(std3D_pD3Device, 0, D3DTSS_MINFILTER, D3DTFN_ANISOTROPIC);
            }
            else
            {
                DWORD filterCaps  = std3D_pCurDevice->d3dDesc.dpcTriCaps.dwTextureFilterCaps;
                bool bUseLinear   = (rdflags & STD3D_RS_TEXFILTER_BILINEAR) != 0
                    || !std3D_pCurDevice->bAnisotropicFilteringSupported;

                // Fixed: Select magnification and minification fallbacks from their respective capability bits.
                if ( bUseLinear && (filterCaps & D3DPTFILTERCAPS_MAGFLINEAR) != 0 )
                {
                    IDirect3DDevice3_SetTextureStageState(std3D_pD3Device, 0, D3DTSS_MAGFILTER, D3DTFG_LINEAR);
                }
                else if ( (filterCaps & D3DPTFILTERCAPS_MAGFPOINT) != 0 )
                {
                    IDirect3DDevice3_SetTextureStageState(std3D_pD3Device, 0, D3DTSS_MAGFILTER, D3DTFG_POINT);
                }

                if ( bUseLinear && (filterCaps & D3DPTFILTERCAPS_MINFLINEAR) != 0 )
                {
                    IDirect3DDevice3_SetTextureStageState(std3D_pD3Device, 0, D3DTSS_MINFILTER, D3DTFN_LINEAR);
                }
                else if ( (filterCaps & D3DPTFILTERCAPS_MINFPOINT) != 0 )
                {
                    IDirect3DDevice3_SetTextureStageState(std3D_pD3Device, 0, D3DTSS_MINFILTER, D3DTFN_POINT);
                }
            }
        }
        else
        #endif // J3D_QOL_IMPROVEMENTS

            if ( (std3D_renderState & STD3D_RS_TEXFILTER_BILINEAR) != (rdflags & STD3D_RS_TEXFILTER_BILINEAR) )
            {
                DWORD filterCaps = std3D_pCurDevice->d3dDesc.dpcTriCaps.dwTextureFilterCaps;
                bool bUseLinear  = (rdflags & STD3D_RS_TEXFILTER_BILINEAR) != 0;

                // Fixed: Select magnification and minification filters from their respective capability bits.
                if ( bUseLinear && (filterCaps & D3DPTFILTERCAPS_MAGFLINEAR) != 0 )
                {
                    IDirect3DDevice3_SetTextureStageState(std3D_pD3Device, 0, D3DTSS_MAGFILTER, D3DTFG_LINEAR);
                }
                else if ( (filterCaps & D3DPTFILTERCAPS_MAGFPOINT) != 0 )
                {
                    IDirect3DDevice3_SetTextureStageState(std3D_pD3Device, 0, D3DTSS_MAGFILTER, D3DTFG_POINT);
                }

                if ( bUseLinear && (filterCaps & D3DPTFILTERCAPS_MINFLINEAR) != 0 )
                {
                    IDirect3DDevice3_SetTextureStageState(std3D_pD3Device, 0, D3DTSS_MINFILTER, D3DTFN_LINEAR);
                }
                else if ( (filterCaps & D3DPTFILTERCAPS_MINFPOINT) != 0 )
                {
                    IDirect3DDevice3_SetTextureStageState(std3D_pD3Device, 0, D3DTSS_MINFILTER, D3DTFN_POINT);
                }
            }

        if ( (std3D_renderState & STD3D_RS_ALPHAREF_SET) != (rdflags & STD3D_RS_ALPHAREF_SET) )
        {
            if ( (rdflags & STD3D_RS_ALPHAREF_SET) != 0 )
            {
                IDirect3DDevice3_SetRenderState(std3D_pD3Device, D3DRENDERSTATE_ALPHAREF, 160);
                std3D_renderState = rdflags;
                return;
            }

            IDirect3DDevice3_SetRenderState(std3D_pD3Device, D3DRENDERSTATE_ALPHAREF, 0);
        }

        std3D_renderState = rdflags;
    }
}

void J3DAPI std3D_AllocSystemTexture(tSystemTexture* pTexture, tVBuffer** apVBuffers, size_t numMipLevels, StdColorFormatType formatType)
{
    LPDIRECTDRAWSURFACE4 pSrcRoot  = NULL;
    LPDIRECTDRAWSURFACE4 pSrcLevel = NULL;
    LPDIRECTDRAWSURFACE4 pDstRoot  = NULL;
    LPDIRECTDRAWSURFACE4 pDstLevel = NULL;
    LPDIRECT3DTEXTURE2 pTex        = NULL;
    LPDIRECT3DTEXTURE2 pTmpTex     = NULL;
    tVBuffer* pLockedVBuffer       = NULL;
    bool bSurfaceLocked            = false;
    bool bVBufferLocked            = false;

    STD_ZEROMEM(pTexture, sizeof(tSystemTexture));

    if ( !std3D_numTextureFormats || !apVBuffers || !numMipLevels )
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

    size_t texSize = (pVBuffer->rasterInfo.colorInfo.bpp * texHeight * texWidth) / 8;
    if ( std3D_mipmapFilter == STD3D_MIPMAPFILTER_NONE )
    {
        numMipLevels = 1;
    }

    DDPIXELFORMAT ddpixfmt = { 0 };
    if ( formatType == STDCOLOR_FORMAT_RGBA_1BITALPHA )
    {
        ddpixfmt = std3D_aTextureFormats[std3D_RGBAKeyTextureFormat].ddPixelFmt;
    }
    else if ( formatType == STDCOLOR_FORMAT_RGBA )
    {
        ddpixfmt = std3D_aTextureFormats[std3D_RGBATextureFormat].ddPixelFmt;
    }
    else
    {
        ddpixfmt = std3D_aTextureFormats[std3D_RGBTextureFormat].ddPixelFmt;
    }

    DDSURFACEDESC2 ddsdSrc  = { 0 };
    ddsdSrc.dwSize          = sizeof(DDSURFACEDESC2);
    ddsdSrc.dwFlags         = DDSD_PIXELFORMAT | DDSD_WIDTH | DDSD_HEIGHT | DDSD_CAPS;
    ddsdSrc.ddsCaps.dwCaps  = DDSCAPS_TEXTURE | DDSCAPS_SYSTEMMEMORY;
    ddsdSrc.dwWidth         = pVBuffer->rasterInfo.width;
    ddsdSrc.dwHeight        = pVBuffer->rasterInfo.height;
    ddsdSrc.dwMipMapCount   = 0;
    ddsdSrc.ddpfPixelFormat = ddpixfmt;

    if ( std3D_mipmapFilter )
    {
        ddsdSrc.dwFlags        |= DDSD_MIPMAPCOUNT;
        ddsdSrc.ddsCaps.dwCaps |= DDSCAPS_MIPMAP | DDSCAPS_COMPLEX;
        ddsdSrc.dwMipMapCount  = numMipLevels;
    }

    DDSURFACEDESC2 ddsdSrcTmp = ddsdSrc;
    HRESULT ddres = IDirectDraw4_CreateSurface(std3D_lpDD, &ddsdSrc, &pSrcRoot, NULL);
    if ( ddres != DD_OK )
    {
        STDLOG_ERROR("Error %s when creating the DirectDraw source surface.\n", std3D_D3DGetStatus(ddres));
        goto error;
    }

    ddres = IDirectDrawSurface4_QueryInterface(pSrcRoot, &IID_IDirect3DTexture2, &pTex);
    if ( ddres != DD_OK )
    {
        STDLOG_ERROR("Error %s creating Direct3D source texture.\n", std3D_D3DGetStatus(ddres));
        goto error;
    }

    pSrcLevel = pSrcRoot;
    for ( size_t mmNum = 0; mmNum < numMipLevels; ++mmNum )
    {
        DDSURFACEDESC2 ddsdMipMap;
        STD_ZEROMEM(&ddsdMipMap, sizeof(ddsdMipMap));
        ddsdMipMap.dwSize = sizeof(DDSURFACEDESC2);
        ddres = IDirectDrawSurface4_Lock(pSrcLevel, NULL, &ddsdMipMap, DDLOCK_WAIT, NULL);
        if ( ddres != DD_OK )
        {
            STDLOG_ERROR("Error %s when locking the DDSurface source buffer.\n", std3D_D3DGetStatus(ddres));
            goto error;
        }
        bSurfaceLocked = true;

        // Fixed: Copy only bytes available in both rows; a DirectDraw pitch can be wider than the source VBuffer row.
        tColorMode colorMode = apVBuffers[mmNum]->rasterInfo.colorInfo.colorMode;
        if ( colorMode > STDCOLOR_PAL && colorMode <= STDCOLOR_RGBA )
        {
            pLockedVBuffer = apVBuffers[mmNum];
            if ( !stdDisplay_VBufferLock(pLockedVBuffer) )
            {
                STDLOG_ERROR("Error locking source VBuffer mip level %zu.\n", mmNum);
                goto error;
            }
            bVBufferLocked = true;

            size_t destPitch = ddsdMipMap.lPitch < 0
                ? (size_t)-ddsdMipMap.lPitch
                : (size_t)ddsdMipMap.lPitch;
            size_t copySize = pLockedVBuffer->rasterInfo.rowSize < destPitch
                ? pLockedVBuffer->rasterInfo.rowSize
                : destPitch;

            for ( size_t row = 0; row < ddsdMipMap.dwHeight; ++row )
            {
                // Fixed: lPitch is the signed byte distance to the next row, including for bottom-up surfaces.
                uint8_t* pDestPixels = (uint8_t*)ddsdMipMap.lpSurface + (ptrdiff_t)row * ddsdMipMap.lPitch;
                memset(pDestPixels, 0, destPitch);

                if ( row < pLockedVBuffer->rasterInfo.height )
                {
                    const uint8_t* pSrcPixels = &pLockedVBuffer->pPixels[pLockedVBuffer->rasterInfo.rowSize * row];
                    memcpy(pDestPixels, pSrcPixels, copySize);
                }
            }

            stdDisplay_VBufferUnlock(pLockedVBuffer);
            pLockedVBuffer = NULL;
            bVBufferLocked = false;
        }
        else
        {
            STDLOG_ERROR("Can't use paletized textures.\n");
        }

        ddres = IDirectDrawSurface4_Unlock(pSrcLevel, NULL);
        if ( ddres != DD_OK )
        {
            STDLOG_ERROR("Error %s when unlocking the DDSurface source buffer.\n", std3D_D3DGetStatus(ddres));
            goto error;
        }
        bSurfaceLocked = false;

        if ( mmNum < numMipLevels - 1 )
        {
            DDSCAPS2 ddscaps = { 0 };
            ddscaps.dwCaps = DDSCAPS_MIPMAP | DDSCAPS_TEXTURE;

            LPDIRECTDRAWSURFACE4 pNextLevel = NULL;
            ddres = IDirectDrawSurface4_GetAttachedSurface(pSrcLevel, &ddscaps, &pNextLevel);
            if ( ddres != DD_OK )
            {
                STDLOG_ERROR("Error %s when getting next DDSurface source buffer.\n", std3D_D3DGetStatus(ddres));
                goto error;
            }

            // Fixed: GetAttachedSurface adds a reference; release each traversed non-root level.
            if ( pSrcLevel != pSrcRoot )
            {
                IDirectDrawSurface4_Release(pSrcLevel);
            }
            pSrcLevel = pNextLevel;
        }
    }

    if ( pSrcLevel != pSrcRoot )
    {
        IDirectDrawSurface4_Release(pSrcLevel);
    }
    pSrcLevel = NULL;

    if ( texWidth != pVBuffer->rasterInfo.width || texHeight != pVBuffer->rasterInfo.height )
    {
        ddsdSrcTmp.dwWidth  = texWidth;
        ddsdSrcTmp.dwHeight = texHeight;
        ddres = IDirectDraw4_CreateSurface(std3D_lpDD, &ddsdSrcTmp, &pDstRoot, NULL);
        if ( ddres != DD_OK )
        {
            STDLOG_ERROR("Error %s when creating the resized DirectDraw texture surface.\n", std3D_D3DGetStatus(ddres));
            goto error;
        }

        ddres = IDirectDrawSurface4_QueryInterface(pDstRoot, &IID_IDirect3DTexture2, &pTmpTex);
        if ( ddres != DD_OK )
        {
            STDLOG_ERROR("Error %s creating resized Direct3D texture.\n", std3D_D3DGetStatus(ddres));
            goto error;
        }

        // Fixed: Traverse source and destination mip chains together so each destination level receives its matching source level.
        pSrcLevel = pSrcRoot;
        pDstLevel = pDstRoot;
        for ( size_t mmNum = 0; mmNum < numMipLevels; ++mmNum )
        {
            ddres = IDirectDrawSurface4_Blt(pDstLevel, NULL, pSrcLevel, NULL, DDBLT_WAIT, NULL);
            if ( ddres != DD_OK )
            {
                STDLOG_ERROR("Error %s when resizing DirectDraw texture mip level %zu.\n", std3D_D3DGetStatus(ddres), mmNum);
                goto error;
            }

            if ( mmNum < numMipLevels - 1 )
            {
                DDSCAPS2 ddscaps = { 0 };
                ddscaps.dwCaps = DDSCAPS_MIPMAP | DDSCAPS_TEXTURE;

                LPDIRECTDRAWSURFACE4 pNextSrcLevel = NULL;
                ddres = IDirectDrawSurface4_GetAttachedSurface(pSrcLevel, &ddscaps, &pNextSrcLevel);
                if ( ddres != DD_OK )
                {
                    STDLOG_ERROR("Error %s when getting the next source mip level.\n", std3D_D3DGetStatus(ddres));
                    goto error;
                }

                LPDIRECTDRAWSURFACE4 pNextDstLevel = NULL;
                ddres = IDirectDrawSurface4_GetAttachedSurface(pDstLevel, &ddscaps, &pNextDstLevel);
                if ( ddres != DD_OK )
                {
                    IDirectDrawSurface4_Release(pNextSrcLevel);
                    STDLOG_ERROR("Error %s when getting the next destination mip level.\n", std3D_D3DGetStatus(ddres));
                    goto error;
                }

                if ( pSrcLevel != pSrcRoot )
                {
                    IDirectDrawSurface4_Release(pSrcLevel);
                }
                if ( pDstLevel != pDstRoot )
                {
                    IDirectDrawSurface4_Release(pDstLevel);
                }

                pSrcLevel = pNextSrcLevel;
                pDstLevel = pNextDstLevel;
            }
        }

        if ( pSrcLevel != pSrcRoot )
        {
            IDirectDrawSurface4_Release(pSrcLevel);
        }
        pSrcLevel = NULL;

        if ( pDstLevel != pDstRoot )
        {
            IDirectDrawSurface4_Release(pDstLevel);
        }
        pDstLevel = NULL;

        IDirect3DTexture2_Release(pTex);
        pTex = NULL;
        ddsdSrc = ddsdSrcTmp;
        pTex    = pTmpTex;
        pTmpTex = NULL;
    }

    if ( pSrcRoot )
    {
        IDirectDrawSurface4_Release(pSrcRoot);
        pSrcRoot = NULL;
    }

    if ( pDstRoot )
    {
        IDirectDrawSurface4_Release(pDstRoot);
        pDstRoot = NULL;
    }

    pTexture->desc           = ddsdSrc;
    pTexture->pTexture       = pTex;
    pTexture->textureSize    = texSize;
    return;

error:
    if ( bVBufferLocked && pLockedVBuffer )
    {
        stdDisplay_VBufferUnlock(pLockedVBuffer);
    }

    if ( bSurfaceLocked && pSrcLevel )
    {
        IDirectDrawSurface4_Unlock(pSrcLevel, NULL);
    }

    if ( pSrcLevel && pSrcLevel != pSrcRoot )
    {
        IDirectDrawSurface4_Release(pSrcLevel);
    }

    if ( pDstLevel && pDstLevel != pDstRoot )
    {
        IDirectDrawSurface4_Release(pDstLevel);
    }

    if ( pTex )
    {
        IDirect3DTexture2_Release(pTex);
    }

    if ( pTmpTex )
    {
        IDirect3DTexture2_Release(pTmpTex);
    }

    if ( pSrcRoot )
    {
        IDirectDrawSurface4_Release(pSrcRoot);
    }

    if ( pDstRoot )
    {
        IDirectDrawSurface4_Release(pDstRoot);
    }

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
    if ( pTex->pTexture )
    {
        IDirect3DTexture2_Release(pTex->pTexture);
    }

    if ( pTex->pCachedTexture )
    {
        std3D_RemoveTextureFromCacheList(pTex);
        IDirect3DTexture2_Release(pTex->pCachedTexture);
    }

    STD_ZEROMEM(pTex, sizeof(tSystemTexture));
}

void J3DAPI std3D_AddToTextureCache(tSystemTexture* pCacheTexture, StdColorFormatType format)
{
    STD_ASSERTREL(pCacheTexture);

    LPDIRECTDRAWSURFACE4 pDestSurf = NULL;
    LPDIRECT3DTEXTURE2 pD3DTex     = NULL;

    if ( !pCacheTexture->pTexture )
    {
        STDLOG_ERROR("No Source texture.\n");
        goto error;
    }


    if ( pCacheTexture->textureSize > std3D_pCurDevice->availableMemory )
    {
        std3D_PurgeTextureCache(pCacheTexture->textureSize);
    }

    DDSURFACEDESC2 ddsdDest = pCacheTexture->desc;
    ddsdDest.ddsCaps.dwCaps &= ~DDSCAPS_SYSTEMMEMORY;
    ddsdDest.ddsCaps.dwCaps |=  DDSCAPS_ALLOCONLOAD | DDSCAPS_VIDEOMEMORY; //DDSCAPS_ALLOCONLOAD requires the D3D texture to call load before it can be used

    HRESULT ddres = IDirectDraw4_CreateSurface(std3D_lpDD, &ddsdDest, &pDestSurf, NULL);
    while ( ddres == DDERR_OUTOFVIDEOMEMORY )
    {
        if ( !std3D_PurgeTextureCache(pCacheTexture->textureSize) )
        {
            STDLOG_ERROR("Error: Unable to purge texture cache for %x bytes!!!.\n", pCacheTexture->textureSize);
            pDestSurf = NULL;
            goto error;
        }
        ddres = IDirectDraw4_CreateSurface(std3D_lpDD, &ddsdDest, &pDestSurf, NULL);
    }

    if ( ddres != DD_OK )
    {
        STDLOG_ERROR("Error %s creating DirectDraw surface for texture.\n", std3D_D3DGetStatus(ddres));
        goto error;
    }

    if ( std3D_aTextureFormats[format].bColorKey )
    {
        ddres = IDirectDrawSurface4_SetColorKey(pDestSurf, DDCKEY_SRCBLT, std3D_aTextureFormats[format].pColorKey);
        if ( ddres != DD_OK )
        {
            STDLOG_ERROR("Error %s setting ColorKey dest texture.\n", std3D_D3DGetStatus(ddres));
        }
    }

    ddres = IDirectDrawSurface4_QueryInterface(pDestSurf, &IID_IDirect3DTexture2, &pD3DTex);
    if ( ddres != DD_OK )
    {
        STDLOG_ERROR("Error %s creating Direct3D dest texture.\n", std3D_D3DGetStatus(ddres));
        pD3DTex = NULL;
        goto error;
    }

    ddres = IDirect3DTexture2_Load(pD3DTex, pCacheTexture->pTexture);
    while ( ddres == DDERR_OUTOFVIDEOMEMORY || ddres == D3DERR_TEXTURE_LOAD_FAILED ) // Added check for D3DERR_TEXTURE_LOAD_FAILED error (might be wrong).
    {                                                                                // Note, Check for DDERR_OUTOFVIDEOMEMORY error is most likely wrong here because it's part of DDraw and DDrawSurface system. 
        if ( !std3D_PurgeTextureCache(pCacheTexture->textureSize) )
        {
            STDLOG_ERROR("Error: Unable to purge texture cache for %x bytes!!!.\n", pCacheTexture->textureSize);
            goto error;
        }
        ddres = IDirect3DTexture2_Load(pD3DTex, pCacheTexture->pTexture);
    }

    if ( ddres == D3D_OK )
    {
        // Success
        IDirectDrawSurface4_Release(pDestSurf);
        pCacheTexture->pCachedTexture = pD3DTex;
        pCacheTexture->frameNum      = std3D_frameCount;
        std3D_AddTextureToCacheList(pCacheTexture);
        return;
    }

    STDLOG_ERROR("Error %s Loading texture.\n", std3D_D3DGetStatus(ddres));

error:
    if ( pDestSurf )
    {
        IDirectDrawSurface4_Release(pDestSurf);
    }

    if ( pD3DTex )
    {
        IDirect3DTexture2_Release(pD3DTex);
    }

    pCacheTexture->pCachedTexture = NULL;
    pCacheTexture->frameNum = 0;
    STDLOG_ERROR("Done error exit from std3D_AddToTextureCache.\n");
}

size_t J3DAPI std3D_GetMipMapCount(const tSystemTexture* pTexture)
{
    if ( !pTexture )
    {
        return 0;
    }

    LPDIRECTDRAWSURFACE4 pSysSurface = NULL;
    if ( !pTexture->pTexture || IDirect3DTexture2_QueryInterface(pTexture->pTexture, &IID_IDirectDrawSurface4, &pSysSurface) != S_OK )
    {
        return 0;
    }

    DDSURFACEDESC2 desc = { 0 };
    desc.dwSize         = sizeof(DDSURFACEDESC2);
    desc.dwFlags        = DDSD_MIPMAPCOUNT;

    HRESULT ddres = IDirectDrawSurface4_GetSurfaceDesc(pSysSurface, &desc);
    IDirectDrawSurface4_Release(pSysSurface);
    return ddres == DD_OK ? desc.dwMipMapCount : 0;
}

void J3DAPI std3D_ResetTextureCache()
{
    tSystemTexture* pCurTex = std3D_pFirstTexCache;
    while ( pCurTex )
    {
        if ( pCurTex->pCachedTexture )
        {
            IDirect3DTexture2_Release(pCurTex->pCachedTexture);
            pCurTex->pCachedTexture = NULL;
        }

        tSystemTexture* pNextTex = pCurTex->pNextCachedTexture;
        pCurTex->frameNum = 0;
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

    if ( std3D_pD3Device )
    {
        HRESULT derr = IDirect3DDevice3_SetTexture(std3D_pD3Device, 0, NULL);
        if ( derr != D3D_OK )
        {
            STDLOG_ERROR("Error %s SetRenderState.\n", std3D_D3DGetStatus(derr)); // TODO: error text says SetRenderState, why is that??? Shall SetRenderState be called instead?
        }
    }

    // Fixed: Mirror the explicit device unbind in the cached texture pointer.
    std3D_pD3DTex = NULL;
    std3D_frameCount = 1;
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
    if ( !std3D_numTextureFormats )
    {
        return 0;
    }

    size_t bestMatchLevel    = 0;
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

    if ( IDirect3DDevice3_SetRenderState(std3D_pD3Device, D3DRENDERSTATE_ZENABLE, TRUE) != D3D_OK )
    {
        return 0;
    }

    if ( IDirect3DDevice3_SetRenderState(std3D_pD3Device, D3DRENDERSTATE_ZWRITEENABLE, TRUE) != D3D_OK )
    {
        return 0;
    }

    if ( IDirect3DDevice3_SetRenderState(std3D_pD3Device, D3DRENDERSTATE_ZFUNC, D3DCMP_LESSEQUAL) != D3D_OK )
    {
        return 0;
    }

    if ( IDirect3DDevice3_SetRenderState(std3D_pD3Device, D3DRENDERSTATE_TEXTUREPERSPECTIVE, TRUE) != D3D_OK )
    {
        return 0;
    }

    std3D_renderState |= STD3D_RS_UNKNOWN_1;

#ifdef J3D_QOL_IMPROVEMENTS
     // Altered: Changed from STD3D_MIPMAPFILTER_BILINEAR to STD3D_MIPMAPFILTER_TRILINEAR
    if ( std3D_SetMipmapFilter(STD3D_MIPMAPFILTER_TRILINEAR) )
    {
        return 0;
    }
#else
    if ( std3D_SetMipmapFilter(STD3D_MIPMAPFILTER_BILINEAR) )
    {
        return 0;
    }
#endif

     // Set texture filtering

#ifdef J3D_QOL_IMPROVEMENTS
// Added: Set anisotropy to max
    if ( std3D_pCurDevice->bAnisotropicFilteringSupported )
    {
        if ( IDirect3DDevice3_SetRenderState(std3D_pD3Device, D3DRENDERSTATE_ANISOTROPY, std3D_pCurDevice->d3dDesc.dwMaxAnisotropy) != D3D_OK )
        {
            return 0;
        }
    }
#endif

#ifdef J3D_QOL_IMPROVEMENTS
    // Added: Add anisotropic tex filtering
    if ( std3D_pCurDevice->bAnisotropicFilteringSupported )
    {
        DWORD filterCaps = std3D_pCurDevice->d3dDesc.dpcTriCaps.dwTextureFilterCaps;
        if ( (std3D_pCurDevice->d3dDesc.dpcTriCaps.dwTextureFilterCaps & D3DPTFILTERCAPS_MAGFANISOTROPIC) != 0 )
        {
            if ( IDirect3DDevice3_SetTextureStageState(std3D_pD3Device, 0, D3DTSS_MAGFILTER, D3DTFG_ANISOTROPIC) != D3D_OK )
            {
                return 0;
            }
        }
        // Fixed: Fall back only to a magnification filter reported by the Direct3D device.
        else if ( (filterCaps & D3DPTFILTERCAPS_MAGFLINEAR) != 0 )
        {
            if ( IDirect3DDevice3_SetTextureStageState(std3D_pD3Device, 0, D3DTSS_MAGFILTER, D3DTFG_LINEAR) != D3D_OK )
            {
                return 0;
            }
        }
        else if ( (filterCaps & D3DPTFILTERCAPS_MAGFPOINT) != 0 )
        {
            if ( IDirect3DDevice3_SetTextureStageState(std3D_pD3Device, 0, D3DTSS_MAGFILTER, D3DTFG_POINT) != D3D_OK )
            {
                return 0;
            }
        }

        if ( IDirect3DDevice3_SetTextureStageState(std3D_pD3Device, 0, D3DTSS_MINFILTER, D3DTFN_ANISOTROPIC) != D3D_OK )
        {
            return 0;
        }

        std3D_renderState |= STD3D_RS_TEXFILTER_ANISOTROPIC;
    }
    else
    #endif // J3D_QOL_IMPROVEMENTS
    {
        DWORD filterCaps = std3D_pCurDevice->d3dDesc.dpcTriCaps.dwTextureFilterCaps;
        bool bLinearFiltering = false;

        // Fixed: Initialize magnification and minification filters from their independent capability bits.
        if ( (filterCaps & D3DPTFILTERCAPS_MAGFLINEAR) != 0 )
        {
            if ( IDirect3DDevice3_SetTextureStageState(std3D_pD3Device, 0, D3DTSS_MAGFILTER, D3DTFG_LINEAR) != D3D_OK )
            {
                return 0;
            }
            bLinearFiltering = true;
        }
        else if ( (filterCaps & D3DPTFILTERCAPS_MAGFPOINT) != 0 )
        {
            if ( IDirect3DDevice3_SetTextureStageState(std3D_pD3Device, 0, D3DTSS_MAGFILTER, D3DTFG_POINT) != D3D_OK )
            {
                return 0;
            }
        }

        if ( (filterCaps & D3DPTFILTERCAPS_MINFLINEAR) != 0 )
        {
            if ( IDirect3DDevice3_SetTextureStageState(std3D_pD3Device, 0, D3DTSS_MINFILTER, D3DTFN_LINEAR) != D3D_OK )
            {
                return 0;
            }
            bLinearFiltering = true;
        }
        else if ( (filterCaps & D3DPTFILTERCAPS_MINFPOINT) != 0 )
        {
            if ( IDirect3DDevice3_SetTextureStageState(std3D_pD3Device, 0, D3DTSS_MINFILTER, D3DTFN_POINT) != D3D_OK )
            {
                return 0;
            }
        }

        if ( bLinearFiltering )
        {
            std3D_renderState |= STD3D_RS_TEXFILTER_BILINEAR;
        }
    }

    if ( IDirect3DDevice3_SetRenderState(std3D_pD3Device, D3DRENDERSTATE_SUBPIXEL, TRUE) != D3D_OK )
    {
        return 0;
    }

    std3D_renderState |= STD3D_RS_SUBPIXEL_CORRECTION;

    if ( IDirect3DDevice3_SetTextureStageState(std3D_pD3Device, 0, D3DTSS_ADDRESSU, D3DTADDRESS_WRAP) != D3D_OK )
    {
        return 0;
    }

    if ( IDirect3DDevice3_SetTextureStageState(std3D_pD3Device, 0, D3DTSS_ADDRESSV, D3DTADDRESS_WRAP) != D3D_OK )
    {
        return 0;
    }

    if ( (std3D_pCurDevice->d3dDesc.dwTextureOpCaps & D3DTEXOPCAPS_MODULATE) != 0
        && (std3D_pCurDevice->d3dDesc.dpcTriCaps.dwSrcBlendCaps & D3DPBLENDCAPS_SRCALPHA) != 0
        && (std3D_pCurDevice->d3dDesc.dpcTriCaps.dwDestBlendCaps & D3DPBLENDCAPS_INVSRCALPHA) != 0 )
    {
        if ( IDirect3DDevice3_SetRenderState(std3D_pD3Device, D3DRENDERSTATE_ALPHABLENDENABLE, TRUE) != D3D_OK )
        {
            return 0;
        }

        if ( IDirect3DDevice3_SetTextureStageState(std3D_pD3Device, 0, D3DTSS_COLOROP, D3DTOP_MODULATE) != D3D_OK )
        {
            return 0;
        }

        if ( IDirect3DDevice3_SetTextureStageState(std3D_pD3Device, 0, D3DTSS_COLORARG1, D3DTA_TEXTURE) != D3D_OK )
        {
            return 0;
        }

        if ( IDirect3DDevice3_SetTextureStageState(std3D_pD3Device, 0, D3DTSS_COLORARG2, D3DTA_CURRENT) != D3D_OK )
        {
            return 0;
        }

        if ( IDirect3DDevice3_SetTextureStageState(std3D_pD3Device, 0, D3DTSS_ALPHAOP, D3DTOP_MODULATE) != D3D_OK )
        {
            return 0;
        }

        if ( IDirect3DDevice3_SetTextureStageState(std3D_pD3Device, 0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE) != D3D_OK )
        {
            return 0;
        }

        if ( IDirect3DDevice3_SetTextureStageState(std3D_pD3Device, 0, D3DTSS_ALPHAARG2, D3DTA_CURRENT) != D3D_OK )
        {
            return 0;
        }

        if ( IDirect3DDevice3_SetRenderState(std3D_pD3Device, D3DRENDERSTATE_SRCBLEND, D3DBLEND_SRCALPHA) != D3D_OK )
        {
            return 0;
        }

        if ( IDirect3DDevice3_SetRenderState(std3D_pD3Device, D3DRENDERSTATE_DESTBLEND, D3DBLEND_INVSRCALPHA) != D3D_OK )
        {
            return 0;
        }

        if ( IDirect3DDevice3_SetRenderState(std3D_pD3Device, D3DRENDERSTATE_ALPHATESTENABLE, TRUE) != D3D_OK )
        {
            return 0;
        }

        if ( (std3D_pCurDevice->d3dDesc.dpcTriCaps.dwAlphaCmpCaps & D3DPCMPCAPS_GREATER) != 0 )
        {
            if ( IDirect3DDevice3_SetRenderState(std3D_pD3Device, D3DRENDERSTATE_ALPHAFUNC, D3DCMP_GREATER) != D3D_OK )
            {
                return 0;
            }

            if ( IDirect3DDevice3_SetRenderState(std3D_pD3Device, D3DRENDERSTATE_ALPHAREF, 0) != D3D_OK )
            {
                return 0;
            }
        }
    }
    else if ( IDirect3DDevice3_SetRenderState(std3D_pD3Device, D3DRENDERSTATE_ALPHABLENDENABLE, FALSE) != D3D_OK )
    {
        return 0;
    }

    if ( (std3D_aTextureFormats[std3D_RGBAKeyTextureFormat].bColorKey
        || std3D_aTextureFormats[std3D_RGBATextureFormat].bColorKey)
        && IDirect3DDevice3_SetRenderState(std3D_pD3Device, D3DRENDERSTATE_COLORKEYENABLE, TRUE) != D3D_OK )
    {
        return 0;
    }

    if ( std3D_pCurDevice->bStippledShadeSupported )
    {
        if ( IDirect3DDevice3_SetRenderState(std3D_pD3Device, D3DRENDERSTATE_STIPPLEDALPHA, TRUE) != D3D_OK )
        {
            return 0;
        }
    }

    else if ( IDirect3DDevice3_SetRenderState(std3D_pD3Device, D3DRENDERSTATE_STIPPLEDALPHA, FALSE) != D3D_OK )
    {
        return 0;
    }

    if ( IDirect3DDevice3_SetRenderState(std3D_pD3Device, D3DRENDERSTATE_SHADEMODE, D3DSHADE_GOURAUD) != D3D_OK )
    {
        return 0;
    }

    if ( IDirect3DDevice3_SetRenderState(std3D_pD3Device, D3DRENDERSTATE_MONOENABLE, FALSE) != D3D_OK )
    {
        return 0;
    }

    if ( IDirect3DDevice3_SetRenderState(std3D_pD3Device, D3DRENDERSTATE_SPECULARENABLE, FALSE) != D3D_OK )
    {
        return 0;
    }

    if ( (std3D_pCurDevice->d3dDesc.dpcTriCaps.dwRasterCaps & D3DPRASTERCAPS_FOGTABLE) == 0
        && (std3D_pCurDevice->d3dDesc.dpcTriCaps.dwRasterCaps & D3DPRASTERCAPS_FOGVERTEX) == 0 )
    {
        std3D_bRenderFog = 0;
    }

    if ( IDirect3DDevice3_SetRenderState(std3D_pD3Device, D3DRENDERSTATE_FOGENABLE, FALSE) != D3D_OK )
    {
        return 0;
    }

    if ( IDirect3DDevice3_SetRenderState(std3D_pD3Device, D3DRENDERSTATE_FILLMODE, D3DFILL_SOLID) != D3D_OK )
    {
        return 0;
    }

    if ( IDirect3DDevice3_SetRenderState(std3D_pD3Device, D3DRENDERSTATE_DITHERENABLE, TRUE) != D3D_OK )
    {
        return 0;
    }

    std3D_renderState |= STD3D_RS_UNKNOWN_2;

    return IDirect3DDevice3_SetRenderState(std3D_pD3Device, D3DRENDERSTATE_ANTIALIAS, 0) == D3D_OK
        && IDirect3DDevice3_SetRenderState(std3D_pD3Device, D3DRENDERSTATE_CULLMODE, D3DCULL_NONE) == D3D_OK;
}

int J3DAPI std3D_SetMipmapFilter(Std3DMipmapFilterType filter)
{
    int d3dres = 0;
    if ( filter == STD3D_MIPMAPFILTER_TRILINEAR
        && (std3D_pCurDevice->d3dDesc.dpcTriCaps.dwTextureFilterCaps & D3DPTFILTERCAPS_MIPFLINEAR) == 0
        && (std3D_pCurDevice->d3dDesc.dpcTriCaps.dwTextureFilterCaps & D3DPTFILTERCAPS_LINEARMIPLINEAR) == 0 )
    {
        filter = STD3D_MIPMAPFILTER_BILINEAR;
    }

    if ( filter == STD3D_MIPMAPFILTER_BILINEAR
        && (std3D_pCurDevice->d3dDesc.dpcTriCaps.dwTextureFilterCaps & D3DPTFILTERCAPS_MIPFPOINT) == 0
        && (std3D_pCurDevice->d3dDesc.dpcTriCaps.dwTextureFilterCaps & D3DPTFILTERCAPS_MIPLINEAR) == 0 )
    {
        filter = STD3D_MIPMAPFILTER_NONE;
    }

    if ( filter != std3D_mipmapFilter )
    {
        switch ( filter )
        {
            case STD3D_MIPMAPFILTER_BILINEAR:
                d3dres = IDirect3DDevice3_SetTextureStageState(std3D_pD3Device, 0, D3DTSS_MIPFILTER, D3DTFP_POINT);
                break;
            case STD3D_MIPMAPFILTER_TRILINEAR:
                d3dres = IDirect3DDevice3_SetTextureStageState(std3D_pD3Device, 0, D3DTSS_MIPFILTER, D3DTFP_LINEAR);
                break;
            default:
                d3dres = IDirect3DDevice3_SetTextureStageState(std3D_pD3Device, 0, D3DTSS_MIPFILTER, D3DTFP_NONE);
                break;
        }
    }

    std3D_mipmapFilter = filter;
    return d3dres;
}

int J3DAPI std3D_SetProjection(float fov, float nearPlane, float farPlane)
{
    // TODO: refactor this function to use aspect ratio

    float distance = farPlane - nearPlane;
    if ( fabs(distance) < 0.009999999776482582 )
    {
        return DDERR_INVALIDPARAMS;
    }

    float hfov = fov / 2.0f;
    float sinhfov = sinf(hfov);
    if ( fabs(sinhfov) < 0.009999999776482582f )
    {
        return DDERR_INVALIDPARAMS;
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
    return IDirect3DDevice3_SetTransform(std3D_pD3Device, D3DTRANSFORMSTATE_PROJECTION, &proj);
}

void J3DAPI std3D_EnableFog(int bEnabled, float density)
{
    std3D_bRenderFog = bEnabled;
    if ( !std3D_pCurDevice )
    {
        std3D_bFogTable = false;
        return;
    }

    std3D_g_fogDensity = density;

    DWORD dwRasterCaps = std3D_pCurDevice->d3dDesc.dpcTriCaps.dwRasterCaps;
    if ( (dwRasterCaps & D3DPRASTERCAPS_FOGTABLE) == 0 && (dwRasterCaps & D3DPRASTERCAPS_FOGVERTEX) == 0 )
    {
        std3D_bRenderFog = 0;
    }

    std3D_bFogTable = (dwRasterCaps & D3DPRASTERCAPS_FOGTABLE) == 0 && std3D_bRenderFog;
}

void J3DAPI std3D_SetFog(float red, float green, float blue, float startDepth, float endDepth)
{
    std3D_EnableFog(std3D_bRenderFog, std3D_g_fogDensity);
    if ( IDirect3DDevice3_SetRenderState(std3D_pD3Device, D3DRENDERSTATE_FOGCOLOR, D3DRGB(red, green, blue)) == D3D_OK )
    {
        if ( std3D_g_fogDensity == 0.0f )
        {
            IDirect3DDevice3_SetRenderState(std3D_pD3Device, D3DRENDERSTATE_FOGTABLEMODE, D3DFOG_NONE);
        }
        else if ( IDirect3DDevice3_SetRenderState(std3D_pD3Device, D3DRENDERSTATE_FOGTABLEMODE, D3DFOG_LINEAR) == DD_OK )
        {
            endDepth = (2.0f - std3D_g_fogDensity) * endDepth;
            if ( IDirect3DDevice3_SetRenderState(std3D_pD3Device, D3DRENDERSTATE_FOGTABLESTART, *(DWORD*)(&startDepth)) == D3D_OK
                && IDirect3DDevice3_SetRenderState(std3D_pD3Device, D3DRENDERSTATE_FOGTABLEEND, *(DWORD*)(&endDepth)) == D3D_OK )
            {
                std3D_fogStartDepth  = startDepth;
                std3D_fogEndDepth    = endDepth;
                std3D_fogDepthFactor = 1.0f / (endDepth - startDepth);
            }
        }
    }
}

void std3D_ClearZBuffer(void)
{
    if ( std3D_lpD3DViewPort )
    {
        HRESULT d3dres = IDirect3DViewport3_Clear2(
            std3D_lpD3DViewPort,
            1,                   // dwCount
            &std3D_activeRect,
            D3DCLEAR_ZBUFFER,
            0,                   // color = black
            1.0f,                // dvZ = 1.0f
            0
        );
        if ( d3dres != D3D_OK )
        {
            STDLOG_ERROR("Error %s when clearing Z.\n", std3D_D3DGetStatus(d3dres));
        }
    }
}

int std3D_CreateViewport(void)
{
    HRESULT d3dres = IDirect3D3_CreateViewport(std3D_pDirect3D, &std3D_lpD3DViewPort, NULL);
    if ( d3dres != D3D_OK )
    {
        STDLOG_ERROR("Error %s when creating D3D viewport.\n", std3D_D3DGetStatus(d3dres));
        return 0;
    }

    d3dres = IDirect3DDevice3_AddViewport(std3D_pD3Device, std3D_lpD3DViewPort);
    if ( d3dres != D3D_OK )
    {
        STDLOG_ERROR("Error %s when adding the D3D viewport to the device.\n", std3D_D3DGetStatus(d3dres));
        return 0;
    }

    D3DVIEWPORT2 d3dviewport = { 0 }; // Altered: Zero init
    d3dviewport.dwSize       = sizeof(D3DVIEWPORT2);
    d3dviewport.dwWidth      = stdDisplay_g_backBuffer.rasterInfo.width;
    d3dviewport.dwHeight     = stdDisplay_g_backBuffer.rasterInfo.height;
    d3dviewport.dvClipWidth  = (float)stdDisplay_g_backBuffer.rasterInfo.width;
    d3dviewport.dvClipHeight = (float)stdDisplay_g_backBuffer.rasterInfo.height;
    d3dviewport.dwX          = 0;
    d3dviewport.dwY          = 0;
    d3dviewport.dvClipX      = 0.0f;
    d3dviewport.dvClipY      = 0.0f;
    d3dviewport.dvMinZ       = 0.0f;
    d3dviewport.dvMaxZ       = 1.0f;

    d3dres = IDirect3DViewport3_SetViewport2(std3D_lpD3DViewPort, &d3dviewport);
    if ( d3dres != D3D_OK )
    {
        STDLOG_ERROR("Error %s when creating D3D viewport.\n", std3D_D3DGetStatus(d3dres));
        return 0;
    }

    d3dres = IDirect3DDevice3_SetCurrentViewport(std3D_pD3Device, std3D_lpD3DViewPort);
    if ( d3dres != D3D_OK )
    {
        STDLOG_ERROR("Error %s when setting D3D viewport.\n", std3D_D3DGetStatus(d3dres));
        return 0;
    }

    std3D_activeRect.x1 = 0;
    std3D_activeRect.y1 = 0;
    std3D_activeRect.x2 = (int32_t)d3dviewport.dvClipWidth;
    std3D_activeRect.y2 = (int32_t)d3dviewport.dvClipHeight;

    d3dres = IDirect3DViewport3_Clear2(
        std3D_lpD3DViewPort,
        1,
        &std3D_activeRect,
        D3DCLEAR_ZBUFFER,
        0,                 // color = black
        1.0f,              // dvZ = 1.0f
        0
    );
    if ( d3dres != D3D_OK )
    {
        STDLOG_ERROR("Error %s when Clearing the Z.\n", std3D_D3DGetStatus(d3dres));
    }


    // TODO: why is stencil buffer being cleared here?
    d3dres = IDirect3DViewport3_Clear2(
        std3D_lpD3DViewPort,
        1,
        &std3D_activeRect,
        D3DCLEAR_STENCIL,
        0,                // color = black
        1.0f,             // dvZ = 1.0f
        0
    );

    // Added
#ifdef J3D_QOL_IMPROVEMENTS
    if ( d3dres != D3D_OK )
    {
        STDLOG_ERROR("Error %s when Clearing the stencil buffer.\n", std3D_D3DGetStatus(d3dres));
    }
#endif

    return 1;
}

void J3DAPI std3D_GetZBufferFormat(DDPIXELFORMAT* pPixelFormat)
{
    if ( pPixelFormat && std3D_pDirect3D )
    {
        if ( std3D_pCurDevice )
        {
            STD_ZEROMEM(pPixelFormat, sizeof(DDPIXELFORMAT));

            pPixelFormat->dwZBufferBitDepth = 0xFFFFu;
            IDirect3D3_EnumZBufferFormats(std3D_pDirect3D, &std3D_pCurDevice->duid, std3D_EnumZBufferFormatsCallback, pPixelFormat);
        }
    }
}

HRESULT CALLBACK std3D_EnumZBufferFormatsCallback(LPDDPIXELFORMAT lpDDPixFmt, DDPIXELFORMAT* lpContext)
{
    if ( !lpDDPixFmt || !lpContext )
    {
        return 0;
    }

    // Fixed: A usable Z format may also expose stencil or other compatible pixel-format flags.
    if ( (lpDDPixFmt->dwFlags & DDPF_ZBUFFER) != 0
        && lpDDPixFmt->dwZBufferBitDepth >= 16 // TODO: try make it 32 bit
        && lpContext->dwZBufferBitDepth > lpDDPixFmt->dwZBufferBitDepth )
    {
        memcpy(lpContext, lpDDPixFmt, sizeof(DDPIXELFORMAT));
    }

    return 1;
}

HRESULT CALLBACK std3D_D3DEnumDevicesCallback(GUID* lpGuid, LPSTR lpDeviceDescription, LPSTR lpDeviceName, LPD3DDEVICEDESC lpD3DHalDDesc, LPD3DDEVICEDESC lpD3DHelDDesc, LPVOID lpContext)
{
    J3D_UNUSED(lpContext);
    if ( !lpGuid || !lpD3DHalDDesc || !lpD3DHelDDesc )
    {
        return 0;
    }

    if ( !std3D_bFindAllD3Devices )
    {
        // Skip devices we don't want i.e.: software emulated devices

        if ( memcmp(lpGuid, &IID_IDirect3DNullDevice, sizeof(*lpGuid)) == 0 )
        {
            return 1;
        }

        if ( memcmp(lpGuid, &IID_IDirect3DRGBDevice, sizeof(*lpGuid)) == 0 )
        {
            return 1;
        }

        if ( memcmp(lpGuid, &IID_IDirect3DRampDevice, sizeof(*lpGuid)) == 0 )
        {
            return 1;
        }

        if ( memcmp(lpGuid, &IID_IDirect3DMMXDevice, sizeof(*lpGuid)) == 0 )
        {
            return 1;
        }

        if ( memcmp(lpGuid, &IID_IDirect3DRefDevice, sizeof(*lpGuid)) == 0 )
        {
            return 1;
        }
    }

    if ( std3D_numDevices >= STD_ARRAYLEN(std3D_aDevices) )
    {
        return 0;
    }

    Device3D* pD3DDriver = &std3D_aDevices[std3D_numDevices];
    pD3DDriver->duid = *lpGuid;

    STD_STRCPY(pD3DDriver->deviceDescription, lpDeviceDescription);
    STD_STRCPY(pD3DDriver->deviceName, lpDeviceName);

    pD3DDriver->bHAL = lpD3DHalDDesc->dwFlags != 0;
    if ( pD3DDriver->bHAL )
    {
        memcpy(&pD3DDriver->d3dDesc, lpD3DHalDDesc, sizeof(pD3DDriver->d3dDesc));
    }
    else
    {
        memcpy(&pD3DDriver->d3dDesc, lpD3DHelDDesc, sizeof(pD3DDriver->d3dDesc));
    }

    StdDisplayDevice device = { 0 }; // Added: Init to zero
    if ( stdDisplay_GetCurrentDevice(&device) )
    {
        return 0;
    }

    if ( !device.bGuidNotSet && !pD3DDriver->bHAL )
    {
        return 1;
    }

    pD3DDriver->bTexturePerspectiveSupported   = (pD3DDriver->d3dDesc.dpcTriCaps.dwTextureCaps & D3DPTEXTURECAPS_PERSPECTIVE) != 0;
    pD3DDriver->hasZBuffer                     = pD3DDriver->d3dDesc.dwDeviceZBufferBitDepth != 0;
    pD3DDriver->bSqareOnlyTexture              = (pD3DDriver->d3dDesc.dpcTriCaps.dwTextureCaps & D3DPTEXTURECAPS_SQUAREONLY) != 0;
    pD3DDriver->bAlphaTextureSupported         = (pD3DDriver->d3dDesc.dpcTriCaps.dwTextureCaps & D3DPTEXTURECAPS_ALPHA) != 0;
    pD3DDriver->bColorkeyTextureSupported      = (pD3DDriver->d3dDesc.dpcTriCaps.dwTextureCaps & D3DPTEXTURECAPS_TRANSPARENCY) != 0;
    // Added: Detect the caps and usable level required by the optional anisotropic filtering feature.
    pD3DDriver->bAnisotropicFilteringSupported = (pD3DDriver->d3dDesc.dpcTriCaps.dwRasterCaps & D3DPRASTERCAPS_ANISOTROPY) != 0
        && (pD3DDriver->d3dDesc.dpcTriCaps.dwTextureFilterCaps & D3DPTFILTERCAPS_MINFANISOTROPIC) != 0
        && pD3DDriver->d3dDesc.dwMaxAnisotropy > 1;
    pD3DDriver->bMipmapAutoGenSupported        = false; // Added

    pD3DDriver->bStippledShadeSupported =
        (pD3DDriver->d3dDesc.dpcTriCaps.dwShadeCaps & D3DPSHADECAPS_ALPHAFLATBLEND) == 0 &&
        (pD3DDriver->d3dDesc.dpcTriCaps.dwShadeCaps & D3DPSHADECAPS_ALPHAFLATSTIPPLED) != 0;

    pD3DDriver->bAlphaBlendSupported =
        (pD3DDriver->d3dDesc.dpcTriCaps.dwTextureBlendCaps & D3DPTBLENDCAPS_MODULATEALPHA) != 0 &&
        (pD3DDriver->d3dDesc.dpcTriCaps.dwShadeCaps & D3DPSHADECAPS_ALPHAGOURAUDBLEND) != 0 ||
        pD3DDriver->bStippledShadeSupported;

    if ( pD3DDriver->d3dDesc.dwMinTextureWidth )
    {
        pD3DDriver->minTexWidth  = pD3DDriver->d3dDesc.dwMinTextureWidth;
        pD3DDriver->minTexHeight = pD3DDriver->d3dDesc.dwMinTextureHeight;
        pD3DDriver->maxTexWidth  = pD3DDriver->d3dDesc.dwMaxTextureWidth;
        pD3DDriver->maxTexHeight = pD3DDriver->d3dDesc.dwMaxTextureHeight;
    }
    else
    {
        pD3DDriver->minTexWidth  = 16;
        pD3DDriver->minTexHeight = 16;
        pD3DDriver->maxTexWidth  = 256;
        pD3DDriver->maxTexHeight = 256;
    }

    pD3DDriver->maxVertexCount = pD3DDriver->d3dDesc.dwMaxVertexCount;

    STDLOG_STATUS("Found |%s|%s|%s|%s| D3D Device\n",
        pD3DDriver->hasZBuffer ? "Z" : "Non-Z",
        pD3DDriver->bAlphaTextureSupported ? "Alpha" : "No Alpha",
        pD3DDriver->bStippledShadeSupported ? "Stippled" : "Blend",
        pD3DDriver->bColorkeyTextureSupported ? "Colorkey" : "No Colorkey"
    );

    STDLOG_STATUS("Description: %s [%s]\n", pD3DDriver->deviceName, pD3DDriver->deviceDescription);

    ++std3D_numDevices;
    return 1;
}

HRESULT CALLBACK std3D_EnumTextureFormatsCallback(LPDDPIXELFORMAT lpDDPixFmt, LPVOID lpContext)
{
    J3D_UNUSED(lpContext);
    // Fixed: Compare the format count with the number of entries, not the array's byte size.
    if ( std3D_numTextureFormats >= STD_ARRAYLEN(std3D_aTextureFormats) )
    {
        return 0;
    }

    StdTextureFormat* pTexFormat = &std3D_aTextureFormats[std3D_numTextureFormats];
    memcpy(&pTexFormat->ddPixelFmt, lpDDPixFmt, sizeof(pTexFormat->ddPixelFmt));

    if ( (lpDDPixFmt->dwFlags & DDPF_PALETTEINDEXED8) != 0 )
    {
        STDLOG_STATUS("Found 8 bpp palettized tex format.\n");
    }
    else if ( (lpDDPixFmt->dwFlags & DDPF_PALETTEINDEXED4) != 0 )
    {
        STDLOG_STATUS("Found 4 bpp palettized tex format.\n");
    }
    else if ( (lpDDPixFmt->dwFlags & DDPF_PALETTEINDEXED2) != 0 )
    {
        STDLOG_STATUS("Found 2 bpp palettized tex format.\n");
    }
    else if ( (lpDDPixFmt->dwFlags & DDPF_PALETTEINDEXED1) != 0 )
    {
        STDLOG_STATUS("Found 1 bpp palettized tex format.\n");
    }
    else if ( (lpDDPixFmt->dwFlags & DDPF_LUMINANCE) != 0 )
    {
        STDLOG_STATUS("Found Luminance Alpha tex format (%d:%d).\n", lpDDPixFmt->dwRGBBitCount, lpDDPixFmt->dwRGBBitCount);
    }
    else if ( (lpDDPixFmt->dwFlags & DDPF_ALPHA) != 0 )
    {
        STDLOG_STATUS("Found Alpha tex format. (%d)\n", lpDDPixFmt->dwRGBBitCount);
    }
    else if ( (lpDDPixFmt->dwFlags & DDPF_FOURCC) != 0 )
    {
        char aFourCC[8] = { 0 }; // Added: Zero init.
        memcpy(aFourCC, &lpDDPixFmt->dwFourCC, 4u);
        aFourCC[4] = 0;
        STDLOG_STATUS("Found FourCC tex format (%s)\n", aFourCC);
    }
    else if ( (lpDDPixFmt->dwFlags & DDPF_BUMPDUDV) != 0 )
    {
        STDLOG_STATUS("Found Luminance BumpMap UV tex format (%d:%d)\n", lpDDPixFmt->dwRGBBitCount, lpDDPixFmt->dwRGBBitCount);
    }
    else if ( (lpDDPixFmt->dwFlags & DDPF_ALPHAPIXELS) != 0 )
    {
        std3D_bHasRGBTextureFormat = true;
        pTexFormat->ci.colorMode = STDCOLOR_RGBA;
        pTexFormat->ci.bpp = lpDDPixFmt->dwRGBBitCount;

        stdColor_CalcColorBits(lpDDPixFmt->dwRBitMask, &pTexFormat->ci.redBPP, &pTexFormat->ci.redPosShift, &pTexFormat->ci.redPosShiftRight);
        stdColor_CalcColorBits(lpDDPixFmt->dwGBitMask, &pTexFormat->ci.greenBPP, &pTexFormat->ci.greenPosShift, &pTexFormat->ci.greenPosShiftRight);
        stdColor_CalcColorBits(lpDDPixFmt->dwBBitMask, &pTexFormat->ci.blueBPP, &pTexFormat->ci.bluePosShift, &pTexFormat->ci.bluePosShiftRight);
        stdColor_CalcColorBits(lpDDPixFmt->dwRGBAlphaBitMask, &pTexFormat->ci.alphaBPP, &pTexFormat->ci.alphaPosShift, &pTexFormat->ci.alphaPosShiftRight);

        STDLOG_STATUS("Found RGBA tex format (%d:%d:%d:%d).\n", pTexFormat->ci.redBPP, pTexFormat->ci.greenBPP, pTexFormat->ci.blueBPP, pTexFormat->ci.alphaBPP);
        ++std3D_numTextureFormats;
    }
    else if ( (lpDDPixFmt->dwFlags & DDPF_RGB) != 0 )
    {
        std3D_bHasRGBTextureFormat = true;
        pTexFormat->ci.colorMode = STDCOLOR_RGB;
        pTexFormat->ci.bpp = lpDDPixFmt->dwRGBBitCount;

        stdColor_CalcColorBits(lpDDPixFmt->dwRBitMask, &pTexFormat->ci.redBPP, &pTexFormat->ci.redPosShift, &pTexFormat->ci.redPosShiftRight);
        stdColor_CalcColorBits(lpDDPixFmt->dwGBitMask, &pTexFormat->ci.greenBPP, &pTexFormat->ci.greenPosShift, &pTexFormat->ci.greenPosShiftRight);
        stdColor_CalcColorBits(lpDDPixFmt->dwBBitMask, &pTexFormat->ci.blueBPP, &pTexFormat->ci.bluePosShift, &pTexFormat->ci.bluePosShiftRight);

        pTexFormat->ci.alphaBPP           = 0;
        pTexFormat->ci.alphaPosShift      = 0;
        pTexFormat->ci.alphaPosShiftRight = 0;

        STDLOG_STATUS("Found RGB tex format (%d:%d:%d).\n", pTexFormat->ci.redBPP, pTexFormat->ci.greenBPP, pTexFormat->ci.blueBPP);
        ++std3D_numTextureFormats;
    }
    else
    {
        STDLOG_STATUS("Found Unknown tex format.\n");
    }

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
            IDirect3DTexture2_Release(pCacheTexture->pCachedTexture);
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
            // Fixed: Added check for null pointer
            if ( pCacheTexture->pCachedTexture )
            {
                IDirect3DTexture2_Release(pCacheTexture->pCachedTexture);
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
    STD_ZEROMEM(pDeviceList, sizeof(StdDisplayEnvironment));

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
            STD_ZEROMEM(pCurInfo, sizeof(StdDisplayInfo));

            if ( stdDisplay_GetDevice(deviceNum, &pCurInfo->displayDevice) )
            {
                STDLOG_ERROR("Error getting stdDisplay device.\n", 0);
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

tSysDevice3D* std3D_GetD3DDevice(void)
{
    return std3D_pD3Device;
}

bool std3D_IsShaderSystemActive(void)
{
    return false;
}

bool std3D_IsAnisotropicFilteringSupported(void)
{
    // Added: Report anisotropic filtering support discovered from the active Direct3D 6 device.
    return std3D_pCurDevice && std3D_pCurDevice->bAnisotropicFilteringSupported;
}

bool std3D_IsMipmapAutoGenSupported(void)
{
    return false;
}

bool std3D_IsMSAASupported(void)
{
    return false;
}

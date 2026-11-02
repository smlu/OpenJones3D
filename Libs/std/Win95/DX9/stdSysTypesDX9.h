// Added: DirectX 9 mapping of the renderer types in std/types.h. Include this first, before
// any other header, in every backend file that touches these types.
#ifndef STD_STDSYSTYPESDX9_H
#define STD_STDSYSTYPESDX9_H

#include <std/Win95/stdSysTagsWin32.h>

// Added: Map the opaque renderer struct tags to their real DirectX 9 types, so backend
// code needs no casts.
#define sSysDisplayDevice IDirect3DDevice9
#define sSysDevice3D IDirect3DDevice9
#define sSysSurface IDirect3DSurface9
#define sSysTexture IDirect3DTexture9

// Added: std/types.h maps these types to the real DirectX 9 types below instead of opaque blobs.
#define STD_SYS_BACKEND_TYPES

#undef DIRECT3D_VERSION
#ifdef J3D_DEBUG
#define D3D_DEBUG_INFO // TODO: comment out when done
#endif
#include <d3d9.h>

// Added: The real DirectX 9 renderer types.
typedef D3DCAPS9 tSysDisplayDeviceCaps;
typedef D3DCAPS9 tSysDevice3DDesc;
typedef D3DFORMAT tSysPixelFormat;
typedef D3DSURFACE_DESC tSysSurfaceDesc;
typedef GUID tSysGuid;

// Added: The sizes the DirectX 9 backend provides; assert each mapped type against its number.
#include <std/Win95/DX9/stdBackendTypes.h>
#include <assert.h>
static_assert(sizeof(tSysDisplayDeviceCaps) == STD_SYS_DISPLAYCAPS_SIZE, "tSysDisplayDeviceCaps == D3DCAPS9");
static_assert(sizeof(tSysDevice3DDesc) == STD_SYS_DEVICE3DDESC_SIZE, "tSysDevice3DDesc == D3DCAPS9");
static_assert(sizeof(tSysPixelFormat) == STD_SYS_PIXELFORMAT_SIZE, "tSysPixelFormat == D3DFORMAT");
static_assert(sizeof(tSysSurfaceDesc) == STD_SYS_SURFACEDESC_SIZE, "tSysSurfaceDesc == D3DSURFACE_DESC");
static_assert(sizeof(tSysGuid) == STD_SYS_GUID_SIZE, "tSysGuid == GUID");

#endif // STD_STDSYSTYPESDX9_H

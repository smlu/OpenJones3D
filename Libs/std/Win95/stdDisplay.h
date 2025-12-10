#ifndef STD_STDDISPLAY_H
#define STD_STDDISPLAY_H
#include <j3dcore/j3d.h>
#include <std/types.h>
#include <std/RTI/addresses.h>

J3D_EXTERN_C_START

typedef void (*tDisplayDevicePreResetCallback)(tSysDevice3D*);
typedef void (*tDisplayDevicePostResetCallback)(tSysDevice3D*);
typedef void (*tDisplayDeviceReleaseCallback)(tSysDevice3D*);

extern tVBuffer stdDisplay_g_frontBuffer;
extern tVBuffer stdDisplay_g_backBuffer;

int stdDisplay_Startup(void);
void stdDisplay_Shutdown(void);

int J3DAPI stdDisplay_Open(size_t deviceNum);
bool stdDisplay_IsOpen(void); // Added
void stdDisplay_Close(void);

#ifdef J3D_DIRECTX9
LPDIRECT3D9 stdDisplay_GetDirect3D(void);
#endif

#ifndef J3D_OPENGL
tSysDisplayDevice* stdDisplay_GetSystemDevice(void); // Can be retrieved after display system is opened
#endif

int J3DAPI stdDisplay_CreateZBuffer(const tSysPixelFormat* pPixelFormat, int bSystemMemory);

void stdDisplay_DisableVSync(bool bDisable); // Added

int stdDisplay_Update(void); // Flips back & front buffers

void J3DAPI stdDisplay_Refresh(int bReload); // If called with bReload = true it will re-set the current video mode

void stdDisplay_RegisterDevicePreResetCallback(tDisplayDevicePreResetCallback pCallback);   // Added, Register callback which is called when display device is about to reset
void stdDisplay_RegisterDevicePostResetCallback(tDisplayDevicePostResetCallback pCallback); // Added, Register callback which is called after display device has been reset
void stdDisplay_RegisterDeviceReleaseCallback(tDisplayDeviceReleaseCallback pCallback);     // Added, Register callback which is called when display device is about to be released

size_t stdDisplay_GetNumDevices(void);
int J3DAPI stdDisplay_GetDevice(size_t deviceNum, StdDisplayDevice* pDest); // Copies display  device at deviceNum to pDest
int J3DAPI stdDisplay_GetCurrentDevice(StdDisplayDevice* pDevice);          // Copies current display device to pDest
const StdDisplayDevice* stdDisplay_GetAllDevices(void);                     // Added

int J3DAPI stdDisplay_SetMode(size_t modeNum, int bFullscreen, size_t numBackBuffers); // Sets current video mode
void stdDisplay_ClearMode(void);                                                       // Clears current video mode

size_t stdDisplay_GetNumVideoModes(void);
int J3DAPI stdDisplay_GetVideoMode(size_t modeNum, StdVideoMode* pDestMode); // Copies video mode at modeNum to pDestMode
int J3DAPI stdDisplay_GetCurrentVideoMode(StdVideoMode* pDisplayMode);       // Copies current video mode to pDestMode

tVBuffer* J3DAPI stdDisplay_VBufferNew(const tRasterInfo* pRasterInfo, int bUseVSurface, int bUseVideoMemory); //bUseVSurface - creates surface in system surface, otherwise in app memory is allocated
void J3DAPI stdDisplay_VBufferFree(tVBuffer* pVBuffer);
int J3DAPI stdDisplay_VBufferLock(tVBuffer* pVBuffer);
int J3DAPI stdDisplay_VBufferUnlock(tVBuffer* pVBuffer);
int J3DAPI stdDisplay_VBufferFill(tVBuffer* pVBuffer, uint32_t dwFillColor, const StdRect* pRect);
tVBuffer* J3DAPI stdDisplay_VBufferConvertColorFormat(const ColorInfo* pDesiredColorFormat, tVBuffer* pSrc, int bColorKey, LPDDCOLORKEY pColorKey);

int J3DAPI stdDisplay_GetTextureMemory(size_t* pTotal, size_t* pFree);
int J3DAPI stdDisplay_GetTotalMemory(size_t* pTotal, size_t* pFree);

void J3DAPI stdDisplay_GetBackBufferSize(uint32_t* pWidth, uint32_t* pHeight);
int J3DAPI stdDisplay_BackBufferFill(uint32_t color, const StdRect* lpRect);
int J3DAPI stdDisplay_SaveScreen(const char* pFilename);

void J3DAPI stdDisplay_SetDefaultResolution(uint32_t width, uint32_t height);

HDC stdDisplay_GetFrontBufferDC(void);
void J3DAPI stdDisplay_ReleaseFrontBufferDC(HDC hdc);

HDC stdDisplay_GetBackBufferDC(void);
void J3DAPI stdDisplay_ReleaseBackBufferDC(HDC hdc);

int stdDisplay_FlipToGDISurface(void);

int J3DAPI stdDisplay_SetBufferClipper(int bFrontBuffer);
HRESULT J3DAPI stdDisplay_RemoveBufferClipper(int bFrontBuffer);

int stdDisplay_CanRenderWindowed(void);
int stdDisplay_IsFullscreen(void);

int J3DAPI stdDisplay_LockBackBuffer(void** ppSurface, uint32_t* pWidth, uint32_t* pHeight, int32_t* pPitch);
void stdDisplay_UnlockBackBuffer(void);

uint32_t J3DAPI stdDisplay_EncodeFromRGB565(uint16_t pixel);

// Helper hooking functions
void stdDisplay_InstallHooks(void);
void stdDisplay_ResetGlobals(void);

J3D_EXTERN_C_END
#endif // STD_STDDISPLAY_H

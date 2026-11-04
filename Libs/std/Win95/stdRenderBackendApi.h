// Backend-side prototypes of the pass-through layer (M6.0). A backend sees these next to the
// macro-renamed public declarations of std3D.h/stdDisplay.h, so the compiler checks that the
// two agree on every one of the 84 renderer functions.
#ifndef STD_STDRENDERBACKENDAPI_H
#define STD_STDRENDERBACKENDAPI_H

#include <std/Win95/std3D.h>
#include <std/Win95/stdDisplay.h>

J3D_EXTERN_C_START

int std3DBackend_Startup(void);
void std3DBackend_Shutdown(void);
const Device3D* std3DBackend_GetCurrentDevice(void);
size_t std3DBackend_GetNumDevices(void);
const Device3D* std3DBackend_GetAllDevices(void);
void J3DAPI std3DBackend_SetFindAllDevices(int bFindAll);
int J3DAPI std3DBackend_Open(size_t deviceNum);
void std3DBackend_Close(void);
void J3DAPI std3DBackend_GetTextureFormat(StdColorFormatType type, ColorInfo* pDest, int* pbColorKeySet, LPDDCOLORKEY* ppColorKey);
StdColorFormatType J3DAPI std3DBackend_GetColorFormat(const ColorInfo* pCi);
size_t std3DBackend_GetNumTextureFormats(void);
int std3DBackend_StartScene(void);
void std3DBackend_EndScene(void);
void J3DAPI std3DBackend_DrawRenderList(tSysTexture* pTex, Std3DRenderState rdflags, StdVertexTL* aVerts, size_t numVerts, uint16_t* aIndices, size_t numIndices);
void J3DAPI std3DBackend_DrawLineStrip(StdVertexTL* aVerts, size_t numVerts);
void J3DAPI std3DBackend_DrawPointList(StdVertexTL* aVerts, size_t numVerts);
void J3DAPI std3DBackend_SetRenderState(Std3DRenderState rdflags);
void std3DBackend_SetWireframeRenderState(void);
void J3DAPI std3DBackend_AllocSystemTexture(tSystemTexture* pTexture, tVBuffer** apVBuffers, size_t numMipLevels, StdColorFormatType formatType);
void J3DAPI std3DBackend_GetValidDimensions(uint32_t width, uint32_t height, uint32_t* pOutWidth, uint32_t* pOutHeight);
void J3DAPI std3DBackend_ClearSystemTexture(tSystemTexture* pTex);
void J3DAPI std3DBackend_AddToTextureCache(tSystemTexture* pCacheTexture, StdColorFormatType format);
void std3DBackend_ResetTextureCache(void);
size_t J3DAPI std3DBackend_GetMipMapCount(const tSystemTexture* pTexture);
void J3DAPI std3DBackend_UpdateFrameCount(tSystemTexture* pTexture);
size_t J3DAPI std3DBackend_FindClosestFormat(const ColorInfo* pMatch);
int J3DAPI std3DBackend_SetMipmapFilter(Std3DMipmapFilterType mode);
int J3DAPI std3DBackend_SetProjection(float fov, float nearPlane, float farPlane);
void J3DAPI std3DBackend_EnableFog(int bEnabled, float density);
void J3DAPI std3DBackend_SetFog(float red, float green, float blue, float startDepth, float endDepth);
void std3DBackend_ClearZBuffer(void);
StdDisplayEnvironment* J3DAPI std3DBackend_BuildDisplayEnvironment();
void J3DAPI std3DBackend_FreeDisplayEnvironment(StdDisplayEnvironment* pEnv);
void std3DBackend_ResetGlobals(void);
int stdDisplayBackend_Startup(void);
void stdDisplayBackend_Shutdown(void);
int J3DAPI stdDisplayBackend_Open(size_t deviceNum);
bool stdDisplayBackend_IsOpen(void);
void stdDisplayBackend_Close(void);
size_t stdDisplayBackend_GetAdapterNum(size_t deviceNum);
tSysDisplayDevice* stdDisplayBackend_GetSystemDevice(void);
int J3DAPI stdDisplayBackend_CreateZBuffer(const tSysPixelFormat* pPixelFormat, int bSystemMemory);
void stdDisplayBackend_DisableVSync(bool bDisable);
int stdDisplayBackend_Update(void);
void J3DAPI stdDisplayBackend_Refresh(int bReload);
void stdDisplayBackend_RegisterDevicePreResetCallback(tDisplayDevicePreResetCallback pCallback);
void stdDisplayBackend_RegisterDevicePostResetCallback(tDisplayDevicePostResetCallback pCallback);
void stdDisplayBackend_RegisterDeviceReleaseCallback(tDisplayDeviceReleaseCallback pCallback);
size_t stdDisplayBackend_GetNumDevices(void);
int J3DAPI stdDisplayBackend_GetDevice(size_t deviceNum, StdDisplayDevice* pDest);
int J3DAPI stdDisplayBackend_GetCurrentDevice(StdDisplayDevice* pDevice);
const StdDisplayDevice* stdDisplayBackend_GetAllDevices(void);
int J3DAPI stdDisplayBackend_SetMode(size_t modeNum, int bFullscreen, size_t numBackBuffers);
void stdDisplayBackend_ClearMode(void);
size_t stdDisplayBackend_GetNumVideoModes(void);
int J3DAPI stdDisplayBackend_GetVideoMode(size_t modeNum, StdVideoMode* pDestMode);
int J3DAPI stdDisplayBackend_GetCurrentVideoMode(StdVideoMode* pDisplayMode);
tVBuffer* J3DAPI stdDisplayBackend_VBufferNew(const tRasterInfo* pRasterInfo, int bUseVSurface, int bUseVideoMemory);
void J3DAPI stdDisplayBackend_VBufferFree(tVBuffer* pVBuffer);
int J3DAPI stdDisplayBackend_VBufferLock(tVBuffer* pVBuffer);
int stdDisplayBackend_VBufferLockReadOnly(tVBuffer* pVBuffer);
int J3DAPI stdDisplayBackend_VBufferUnlock(tVBuffer* pVBuffer);
int J3DAPI stdDisplayBackend_VBufferFill(tVBuffer* pVBuffer, uint32_t dwFillColor, const StdRect* pRect);
tVBuffer* J3DAPI stdDisplayBackend_VBufferConvertColorFormat(const ColorInfo* pDesiredColorFormat, tVBuffer* pSrc, int bColorKey, LPDDCOLORKEY pColorKey);
int J3DAPI stdDisplayBackend_GetTextureMemory(size_t* pTotal, size_t* pFree);
int J3DAPI stdDisplayBackend_GetTotalMemory(size_t* pTotal, size_t* pFree);
void J3DAPI stdDisplayBackend_GetBackBufferSize(uint32_t* pWidth, uint32_t* pHeight);
int J3DAPI stdDisplayBackend_BackBufferFill(uint32_t color, const StdRect* lpRect);
int J3DAPI stdDisplayBackend_SaveScreen(const char* pFilename);
void J3DAPI stdDisplayBackend_SetDefaultResolution(uint32_t width, uint32_t height);
tSysDrawContext stdDisplayBackend_GetFrontBufferDC(void);
void J3DAPI stdDisplayBackend_ReleaseFrontBufferDC(tSysDrawContext hdc);
tSysDrawContext stdDisplayBackend_GetBackBufferDC(void);
void J3DAPI stdDisplayBackend_ReleaseBackBufferDC(tSysDrawContext hdc);
int stdDisplayBackend_FlipToGDISurface(void);
int J3DAPI stdDisplayBackend_SetBufferClipper(int bFrontBuffer);
int J3DAPI stdDisplayBackend_RemoveBufferClipper(int bFrontBuffer);
int stdDisplayBackend_CanRenderWindowed(void);
int stdDisplayBackend_IsFullscreen(void);
int J3DAPI stdDisplayBackend_LockBackBuffer(void** ppSurface, uint32_t* pWidth, uint32_t* pHeight, int32_t* pPitch);
int stdDisplayBackend_LockBackBufferReadOnly(void** ppSurface, uint32_t* pWidth, uint32_t* pHeight, int32_t* pPitch);
void stdDisplayBackend_UnlockBackBuffer(void);
uint32_t J3DAPI stdDisplayBackend_EncodeFromRGB565(uint16_t pixel);
void stdDisplayBackend_ResetGlobals(void);

J3D_EXTERN_C_END
#endif // STD_STDRENDERBACKENDAPI_H

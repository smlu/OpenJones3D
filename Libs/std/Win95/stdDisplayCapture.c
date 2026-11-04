// Forwarding layer of the render capture (M6.0): one public stdDisplay_ name per display
// function, forwarding to the backend. Recording is added later.

#include <std/Win95/stdDisplay.h>
#include <std/Win95/stdRenderBackendApi.h>

int stdDisplay_Startup(void)
{
    return stdDisplayBackend_Startup();
}

void stdDisplay_Shutdown(void)
{
    stdDisplayBackend_Shutdown();
}

int J3DAPI stdDisplay_Open(size_t deviceNum)
{
    return stdDisplayBackend_Open(deviceNum);
}

bool stdDisplay_IsOpen(void)
{
    return stdDisplayBackend_IsOpen();
}

void stdDisplay_Close(void)
{
    stdDisplayBackend_Close();
}

size_t stdDisplay_GetAdapterNum(size_t deviceNum)
{
    return stdDisplayBackend_GetAdapterNum(deviceNum);
}

tSysDisplayDevice* stdDisplay_GetSystemDevice(void)
{
    return stdDisplayBackend_GetSystemDevice();
}

int J3DAPI stdDisplay_CreateZBuffer(const tSysPixelFormat* pPixelFormat, int bSystemMemory)
{
    return stdDisplayBackend_CreateZBuffer(pPixelFormat, bSystemMemory);
}

void stdDisplay_DisableVSync(bool bDisable)
{
    stdDisplayBackend_DisableVSync(bDisable);
}

int stdDisplay_Update(void)
{
    return stdDisplayBackend_Update();
}

void J3DAPI stdDisplay_Refresh(int bReload)
{
    stdDisplayBackend_Refresh(bReload);
}

void stdDisplay_RegisterDevicePreResetCallback(tDisplayDevicePreResetCallback pCallback)
{
    stdDisplayBackend_RegisterDevicePreResetCallback(pCallback);
}

void stdDisplay_RegisterDevicePostResetCallback(tDisplayDevicePostResetCallback pCallback)
{
    stdDisplayBackend_RegisterDevicePostResetCallback(pCallback);
}

void stdDisplay_RegisterDeviceReleaseCallback(tDisplayDeviceReleaseCallback pCallback)
{
    stdDisplayBackend_RegisterDeviceReleaseCallback(pCallback);
}

size_t stdDisplay_GetNumDevices(void)
{
    return stdDisplayBackend_GetNumDevices();
}

int J3DAPI stdDisplay_GetDevice(size_t deviceNum, StdDisplayDevice* pDest)
{
    return stdDisplayBackend_GetDevice(deviceNum, pDest);
}

int J3DAPI stdDisplay_GetCurrentDevice(StdDisplayDevice* pDevice)
{
    return stdDisplayBackend_GetCurrentDevice(pDevice);
}

const StdDisplayDevice* stdDisplay_GetAllDevices(void)
{
    return stdDisplayBackend_GetAllDevices();
}

int J3DAPI stdDisplay_SetMode(size_t modeNum, int bFullscreen, size_t numBackBuffers)
{
    return stdDisplayBackend_SetMode(modeNum, bFullscreen, numBackBuffers);
}

void stdDisplay_ClearMode(void)
{
    stdDisplayBackend_ClearMode();
}

size_t stdDisplay_GetNumVideoModes(void)
{
    return stdDisplayBackend_GetNumVideoModes();
}

int J3DAPI stdDisplay_GetVideoMode(size_t modeNum, StdVideoMode* pDestMode)
{
    return stdDisplayBackend_GetVideoMode(modeNum, pDestMode);
}

int J3DAPI stdDisplay_GetCurrentVideoMode(StdVideoMode* pDisplayMode)
{
    return stdDisplayBackend_GetCurrentVideoMode(pDisplayMode);
}

tVBuffer* J3DAPI stdDisplay_VBufferNew(const tRasterInfo* pRasterInfo, int bUseVSurface, int bUseVideoMemory)
{
    return stdDisplayBackend_VBufferNew(pRasterInfo, bUseVSurface, bUseVideoMemory);
}

void J3DAPI stdDisplay_VBufferFree(tVBuffer* pVBuffer)
{
    stdDisplayBackend_VBufferFree(pVBuffer);
}

int J3DAPI stdDisplay_VBufferLock(tVBuffer* pVBuffer)
{
    return stdDisplayBackend_VBufferLock(pVBuffer);
}

int stdDisplay_VBufferLockReadOnly(tVBuffer* pVBuffer)
{
    return stdDisplayBackend_VBufferLockReadOnly(pVBuffer);
}

int J3DAPI stdDisplay_VBufferUnlock(tVBuffer* pVBuffer)
{
    return stdDisplayBackend_VBufferUnlock(pVBuffer);
}

int J3DAPI stdDisplay_VBufferFill(tVBuffer* pVBuffer, uint32_t dwFillColor, const StdRect* pRect)
{
    return stdDisplayBackend_VBufferFill(pVBuffer, dwFillColor, pRect);
}

tVBuffer* J3DAPI stdDisplay_VBufferConvertColorFormat(const ColorInfo* pDesiredColorFormat, tVBuffer* pSrc, int bColorKey, LPDDCOLORKEY pColorKey)
{
    return stdDisplayBackend_VBufferConvertColorFormat(pDesiredColorFormat, pSrc, bColorKey, pColorKey);
}

int J3DAPI stdDisplay_GetTextureMemory(size_t* pTotal, size_t* pFree)
{
    return stdDisplayBackend_GetTextureMemory(pTotal, pFree);
}

int J3DAPI stdDisplay_GetTotalMemory(size_t* pTotal, size_t* pFree)
{
    return stdDisplayBackend_GetTotalMemory(pTotal, pFree);
}

void J3DAPI stdDisplay_GetBackBufferSize(uint32_t* pWidth, uint32_t* pHeight)
{
    stdDisplayBackend_GetBackBufferSize(pWidth, pHeight);
}

int J3DAPI stdDisplay_BackBufferFill(uint32_t color, const StdRect* lpRect)
{
    return stdDisplayBackend_BackBufferFill(color, lpRect);
}

int J3DAPI stdDisplay_SaveScreen(const char* pFilename)
{
    return stdDisplayBackend_SaveScreen(pFilename);
}

void J3DAPI stdDisplay_SetDefaultResolution(uint32_t width, uint32_t height)
{
    stdDisplayBackend_SetDefaultResolution(width, height);
}

tSysDrawContext stdDisplay_GetFrontBufferDC(void)
{
    return stdDisplayBackend_GetFrontBufferDC();
}

void J3DAPI stdDisplay_ReleaseFrontBufferDC(tSysDrawContext hdc)
{
    stdDisplayBackend_ReleaseFrontBufferDC(hdc);
}

tSysDrawContext stdDisplay_GetBackBufferDC(void)
{
    return stdDisplayBackend_GetBackBufferDC();
}

void J3DAPI stdDisplay_ReleaseBackBufferDC(tSysDrawContext hdc)
{
    stdDisplayBackend_ReleaseBackBufferDC(hdc);
}

int stdDisplay_FlipToGDISurface(void)
{
    return stdDisplayBackend_FlipToGDISurface();
}

int J3DAPI stdDisplay_SetBufferClipper(int bFrontBuffer)
{
    return stdDisplayBackend_SetBufferClipper(bFrontBuffer);
}

int J3DAPI stdDisplay_RemoveBufferClipper(int bFrontBuffer)
{
    return stdDisplayBackend_RemoveBufferClipper(bFrontBuffer);
}

int stdDisplay_CanRenderWindowed(void)
{
    return stdDisplayBackend_CanRenderWindowed();
}

int stdDisplay_IsFullscreen(void)
{
    return stdDisplayBackend_IsFullscreen();
}

int J3DAPI stdDisplay_LockBackBuffer(void** ppSurface, uint32_t* pWidth, uint32_t* pHeight, int32_t* pPitch)
{
    return stdDisplayBackend_LockBackBuffer(ppSurface, pWidth, pHeight, pPitch);
}

int stdDisplay_LockBackBufferReadOnly(void** ppSurface, uint32_t* pWidth, uint32_t* pHeight, int32_t* pPitch)
{
    return stdDisplayBackend_LockBackBufferReadOnly(ppSurface, pWidth, pHeight, pPitch);
}

void stdDisplay_UnlockBackBuffer(void)
{
    stdDisplayBackend_UnlockBackBuffer();
}

uint32_t J3DAPI stdDisplay_EncodeFromRGB565(uint16_t pixel)
{
    return stdDisplayBackend_EncodeFromRGB565(pixel);
}

void stdDisplay_ResetGlobals(void)
{
    stdDisplayBackend_ResetGlobals();
}

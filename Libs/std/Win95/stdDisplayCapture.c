// Forwarding layer of the render capture (M6.0): one public stdDisplay_ name per display
// function, forwarding to the backend and, while a capture is open, recording every call.
// The capture control stdDisplayCapture_Open/Close and the Put helpers shared with
// std3DCapture.c (declared in stdRenderCaptureForward.h) also live here, so all per-capture
// state of the layer sits in one file.

#include <string.h>

#include <std/General/std.h>
#include <std/General/stdMemory.h>
#include <std/General/stdRenderCapture.h>
#include <std/General/stdRenderCaptureRecord.h>

#include <std/Win95/stdDisplay.h>
#include <std/Win95/stdDisplayCapture.h>
#include <std/Win95/stdRenderBackendApi.h>
#include <std/Win95/stdRenderCaptureForward.h>
#include <std/Win95/stdRenderCaptureIds.h>

// Per-capture state of the forwarding layer, reset by stdDisplayCapture_Open and Close.
static bool stdDisplayCapture_bWarnedUnknownVBuffer;
static bool stdDisplayCapture_bWarnedBlobSize;
static bool stdDisplayCapture_bWarnedBackBufferPitch;
static bool stdDisplayCapture_bWarnedCountOverflow;
static bool stdDisplayCapture_bWarnedUnknownCacheTexture;

// The remembered back-buffer lock: stdDisplay_LockBackBuffer stores what the lock handed out
// even when nothing records, so stdDisplay_UnlockBackBuffer can capture the pixels the lock
// covered. Only one lock is remembered; the last successful Lock wins.
static void* stdDisplayCapture_pBackBufferPixels;
static uint32_t stdDisplayCapture_backBufferLockHeight;
static int32_t stdDisplayCapture_backBufferLockPitch;

static void stdDisplayCapture_ResetState(void)
{
    stdDisplayCapture_bWarnedUnknownVBuffer    = false;
    stdDisplayCapture_bWarnedBlobSize          = false;
    stdDisplayCapture_bWarnedBackBufferPitch   = false;
    stdDisplayCapture_bWarnedCountOverflow     = false;
    stdDisplayCapture_bWarnedUnknownCacheTexture = false;

    stdDisplayCapture_pBackBufferPixels      = NULL;
    stdDisplayCapture_backBufferLockHeight   = 0u;
    stdDisplayCapture_backBufferLockPitch    = 0;
}

bool stdDisplayCapture_Open(const char* pPath, const char* pColumn)
{
    if ( !stdRenderCapture_Open(pPath, pColumn) )
    {
        return false;
    }

    if ( stdRenderCapture_AddHandle(STDRENDERCAPTURE_HANDLE_VBUFFER, &stdDisplay_g_backBuffer) != 1u )
    {
        STDLOG_ERROR("Render capture: the back buffer did not become VBuffer handle 1.\n");
        stdDisplayCapture_Close();
        return false;
    }

    stdDisplayCapture_ResetState();
    return true;
}

void stdDisplayCapture_Close(void)
{
    stdRenderCapture_Close();
    stdDisplayCapture_ResetState();
}

static bool stdDisplayCapture_bFrameRecording(void)
{
    return stdRenderCapture_IsOpen() && stdRenderCapture_GetFramesEnabled();
}

// The 14 ColorInfo fields without the present byte, in declaration order.
static void stdDisplayCapture_PutColorFields(const ColorInfo* pColorInfo)
{
    stdRenderCapture_PutI32((int32_t)pColorInfo->colorMode);
    stdRenderCapture_PutU32(pColorInfo->bpp);
    stdRenderCapture_PutU32(pColorInfo->redBPP);
    stdRenderCapture_PutU32(pColorInfo->greenBPP);
    stdRenderCapture_PutU32(pColorInfo->blueBPP);
    stdRenderCapture_PutI32(pColorInfo->redPosShift);
    stdRenderCapture_PutI32(pColorInfo->greenPosShift);
    stdRenderCapture_PutI32(pColorInfo->bluePosShift);
    stdRenderCapture_PutI32(pColorInfo->redPosShiftRight);
    stdRenderCapture_PutI32(pColorInfo->greenPosShiftRight);
    stdRenderCapture_PutI32(pColorInfo->bluePosShiftRight);
    stdRenderCapture_PutU32(pColorInfo->alphaBPP);
    stdRenderCapture_PutI32(pColorInfo->alphaPosShift);
    stdRenderCapture_PutI32(pColorInfo->alphaPosShiftRight);
}

void stdDisplayCapture_PutColorInfo(const ColorInfo* pColorInfo)
{
    if ( !pColorInfo )
    {
        stdRenderCapture_PutU8(0u);
        return;
    }

    stdRenderCapture_PutU8(1u);
    stdDisplayCapture_PutColorFields(pColorInfo);
}

static void stdDisplayCapture_PutRasterFields(const tRasterInfo* pRasterInfo)
{
    stdRenderCapture_PutU32(pRasterInfo->width);
    stdRenderCapture_PutU32(pRasterInfo->height);
    stdRenderCapture_PutU64((uint64_t)pRasterInfo->size);
    stdRenderCapture_PutU64((uint64_t)pRasterInfo->rowSize);
    stdRenderCapture_PutU64((uint64_t)pRasterInfo->rowWidth);
    stdDisplayCapture_PutColorFields(&pRasterInfo->colorInfo);
}

// The input raster of VBufferNew only carries what the caller decides. The game leaves size,
// rowSize and rowWidth uninitialized because every backend recomputes them, so recording them
// would put garbage (and run-to-run differences) into the capture.
static void stdDisplayCapture_PutRasterInfo(const tRasterInfo* pRasterInfo)
{
    if ( !pRasterInfo )
    {
        stdRenderCapture_PutU8(0u);
        return;
    }

    stdRenderCapture_PutU8(1u);
    stdRenderCapture_PutU32(pRasterInfo->width);
    stdRenderCapture_PutU32(pRasterInfo->height);
    stdDisplayCapture_PutColorFields(&pRasterInfo->colorInfo);
}

static void stdDisplayCapture_PutRect(const StdRect* pRect)
{
    if ( !pRect )
    {
        stdRenderCapture_PutU8(0u);
        return;
    }

    stdRenderCapture_PutU8(1u);
    stdRenderCapture_PutI32(pRect->left);
    stdRenderCapture_PutI32(pRect->top);
    stdRenderCapture_PutI32(pRect->right);
    stdRenderCapture_PutI32(pRect->bottom);
}

static void stdDisplayCapture_PutColorKey(const DDCOLORKEY* pColorKey)
{
    if ( !pColorKey )
    {
        stdRenderCapture_PutU8(0u);
        return;
    }

    stdRenderCapture_PutU8(1u);
    stdRenderCapture_PutU32(pColorKey->dwColorSpaceLowValue);
    stdRenderCapture_PutU32(pColorKey->dwColorSpaceHighValue);
}

uint32_t stdDisplayCapture_VBufferHandle(const tVBuffer* pVBuffer)
{
    uint32_t handle;

    if ( !pVBuffer )
    {
        return 0u;
    }

    handle = stdRenderCapture_FindHandle(STDRENDERCAPTURE_HANDLE_VBUFFER, pVBuffer);
    if ( handle )
    {
        return handle;
    }

    if ( !stdDisplayCapture_bWarnedUnknownVBuffer )
    {
        stdDisplayCapture_bWarnedUnknownVBuffer = true;
        STDLOG_WARNING("Render capture: unknown VBuffer got a new handle.\n");
    }

    return stdRenderCapture_AddHandle(STDRENDERCAPTURE_HANDLE_VBUFFER, pVBuffer);
}

// A VBuffer's content is rasterInfo.height * rasterInfo.rowSize bytes at pPixels. Without pixels
// a hardware buffer is read through a read-only lock of the backend (the backend entry, so the
// lock itself is not recorded); every failure and a non-hardware buffer without pixels gives
// blob 0.
uint32_t stdDisplayCapture_WriteVBufferBlob(tVBuffer* pVBuffer)
{
    uint64_t bytes;
    uint32_t blobId;

    if ( !pVBuffer )
    {
        return 0u;
    }

    if ( pVBuffer->rasterInfo.rowSize != 0u
        && (uint64_t)pVBuffer->rasterInfo.height > (uint64_t)SIZE_MAX / pVBuffer->rasterInfo.rowSize )
    {
        if ( !stdDisplayCapture_bWarnedBlobSize )
        {
            stdDisplayCapture_bWarnedBlobSize = true;
            STDLOG_WARNING("Render capture: a VBuffer content size overflowed; recorded blob 0.\n");
        }

        return 0u;
    }

    bytes = (uint64_t)pVBuffer->rasterInfo.height * (uint64_t)pVBuffer->rasterInfo.rowSize;

    if ( pVBuffer->pPixels )
    {
        return stdRenderCapture_WriteBlob(pVBuffer->pPixels, (size_t)bytes);
    }

    if ( pVBuffer->type != VBUFFER_HARDWARE )
    {
        return 0u;
    }

    if ( !stdDisplayBackend_VBufferLockReadOnly(pVBuffer) )
    {
        return 0u;
    }

    if ( !pVBuffer->pPixels )
    {
        stdDisplayBackend_VBufferUnlock(pVBuffer);
        return 0u;
    }

    blobId = stdRenderCapture_WriteBlob(pVBuffer->pPixels, (size_t)bytes);
    stdDisplayBackend_VBufferUnlock(pVBuffer);
    return blobId;
}

// A vertex or index count above UINT32_MAX cannot be recorded: the error is logged once per
// capture and the count is recorded as 0. A NULL array records count 0 (its content cannot be
// read), which keeps the payload matching its count.
static void stdDisplayCapture_PutCount(size_t count, const void* pData)
{
    if ( count > (size_t)UINT32_MAX )
    {
        if ( !stdDisplayCapture_bWarnedCountOverflow )
        {
            stdDisplayCapture_bWarnedCountOverflow = true;
            STDLOG_ERROR("Render capture: a vertex or index count exceeded the recordable range.\n");
        }

        stdRenderCapture_PutU32(0u);
        return;
    }

    stdRenderCapture_PutU32((uint32_t)(!pData ? 0u : count));
}

void stdDisplayCapture_PutVertices(const StdVertexTL* aVerts, size_t numVerts)
{
    size_t i;

    stdDisplayCapture_PutCount(numVerts, aVerts);
    if ( !aVerts )
    {
        return;
    }

    for ( i = 0; i < numVerts; ++i )
    {
        stdRenderCapture_PutF32(aVerts[i].sx);
        stdRenderCapture_PutF32(aVerts[i].sy);
        stdRenderCapture_PutF32(aVerts[i].sz);
        stdRenderCapture_PutF32(aVerts[i].rhw);
        stdRenderCapture_PutU32(aVerts[i].color);
        stdRenderCapture_PutU32(aVerts[i].specular);
        stdRenderCapture_PutF32(aVerts[i].tu);
        stdRenderCapture_PutF32(aVerts[i].tv);
    }
}

void stdDisplayCapture_PutIndices(const uint16_t* aIndices, size_t numIndices)
{
    size_t i;

    stdDisplayCapture_PutCount(numIndices, aIndices);
    if ( !aIndices )
    {
        return;
    }

    for ( i = 0; i < numIndices; ++i )
    {
        stdRenderCapture_PutU16(aIndices[i]);
    }
}

static bool stdDisplayCapture_MatchCachedTexture(const void* p, const void* pContext)
{
    return ((const tSystemTexture*)p)->pCachedTexture == (const tSysTexture*)pContext;
}

// The DrawRenderList pTex is the backend's cached texture: the handle of the live system texture
// whose pCachedTexture is it. An unknown pointer is recorded as 0xFFFFFFFF, warned once per
// capture.
uint32_t stdDisplayCapture_CacheTextureHandle(const tSysTexture* pTex)
{
    uint32_t handle;

    if ( !pTex )
    {
        return 0u;
    }

    handle = stdRenderCapture_FindHandleWhere(
        STDRENDERCAPTURE_HANDLE_SYSTEXTURE,
        stdDisplayCapture_MatchCachedTexture,
        pTex
    );
    if ( handle )
    {
        return handle;
    }

    if ( !stdDisplayCapture_bWarnedUnknownCacheTexture )
    {
        stdDisplayCapture_bWarnedUnknownCacheTexture = true;
        STDLOG_WARNING("Render capture: unknown cached texture pointer recorded as 0xFFFFFFFF.\n");
    }

    return 0xFFFFFFFFu;
}

int stdDisplay_Startup(void)
{
    int result;

    if ( !stdRenderCapture_IsOpen() )
    {
        return stdDisplayBackend_Startup();
    }

    result = stdDisplayBackend_Startup();
    stdRenderCapture_BeginRecord(STDRENDERCAPTURE_ID_STDDISPLAY_STARTUP);
    stdRenderCapture_PutI32(result);
    stdRenderCapture_EndRecord();
    return result;
}

void stdDisplay_Shutdown(void)
{
    if ( !stdRenderCapture_IsOpen() )
    {
        stdDisplayBackend_Shutdown();
        return;
    }

    stdDisplayBackend_Shutdown();
    stdRenderCapture_BeginRecord(STDRENDERCAPTURE_ID_STDDISPLAY_SHUTDOWN);
    stdRenderCapture_EndRecord();
}

int J3DAPI stdDisplay_Open(size_t deviceNum)
{
    int result;

    if ( !stdRenderCapture_IsOpen() )
    {
        return stdDisplayBackend_Open(deviceNum);
    }

    result = stdDisplayBackend_Open(deviceNum);
    stdRenderCapture_BeginRecord(STDRENDERCAPTURE_ID_STDDISPLAY_OPEN);
    stdRenderCapture_PutU64((uint64_t)deviceNum);
    stdRenderCapture_PutI32(result);
    stdRenderCapture_EndRecord();
    return result;
}

bool stdDisplay_IsOpen(void)
{
    bool result;

    if ( !stdRenderCapture_IsOpen() )
    {
        return stdDisplayBackend_IsOpen();
    }

    result = stdDisplayBackend_IsOpen();
    stdRenderCapture_BeginRecord(STDRENDERCAPTURE_ID_STDDISPLAY_ISOPEN);
    stdRenderCapture_PutBool(result);
    stdRenderCapture_EndRecord();
    return result;
}

void stdDisplay_Close(void)
{
    if ( !stdRenderCapture_IsOpen() )
    {
        stdDisplayBackend_Close();
        return;
    }

    stdDisplayBackend_Close();
    stdRenderCapture_BeginRecord(STDRENDERCAPTURE_ID_STDDISPLAY_CLOSE);
    stdRenderCapture_EndRecord();
}

size_t stdDisplay_GetAdapterNum(size_t deviceNum)
{
    size_t result;

    if ( !stdRenderCapture_IsOpen() )
    {
        return stdDisplayBackend_GetAdapterNum(deviceNum);
    }

    result = stdDisplayBackend_GetAdapterNum(deviceNum);
    stdRenderCapture_BeginRecord(STDRENDERCAPTURE_ID_STDDISPLAY_GETADAPTERNUM);
    stdRenderCapture_PutU64((uint64_t)deviceNum);
    stdRenderCapture_PutU64((uint64_t)result);
    stdRenderCapture_EndRecord();
    return result;
}

tSysDisplayDevice* stdDisplay_GetSystemDevice(void)
{
    tSysDisplayDevice* pResult;

    if ( !stdRenderCapture_IsOpen() )
    {
        return stdDisplayBackend_GetSystemDevice();
    }

    pResult = stdDisplayBackend_GetSystemDevice();
    stdRenderCapture_BeginRecord(STDRENDERCAPTURE_ID_STDDISPLAY_GETSYSTEMDEVICE);
    stdRenderCapture_PutU8(pResult != NULL ? 1u : 0u);
    stdRenderCapture_EndRecord();
    return pResult;
}

int J3DAPI stdDisplay_CreateZBuffer(const tSysPixelFormat* pPixelFormat, int bSystemMemory)
{
    int result;

    if ( !stdRenderCapture_IsOpen() )
    {
        return stdDisplayBackend_CreateZBuffer(pPixelFormat, bSystemMemory);
    }

    result = stdDisplayBackend_CreateZBuffer(pPixelFormat, bSystemMemory);
    stdRenderCapture_BeginRecord(STDRENDERCAPTURE_ID_STDDISPLAY_CREATEZBUFFER);
    stdRenderCapture_PutU8(pPixelFormat != NULL ? 1u : 0u);
    stdRenderCapture_PutI32(bSystemMemory);
    stdRenderCapture_PutI32(result);
    stdRenderCapture_EndRecord();
    return result;
}

void stdDisplay_DisableVSync(bool bDisable)
{
    if ( !stdRenderCapture_IsOpen() )
    {
        stdDisplayBackend_DisableVSync(bDisable);
        return;
    }

    stdDisplayBackend_DisableVSync(bDisable);
    stdRenderCapture_BeginRecord(STDRENDERCAPTURE_ID_STDDISPLAY_DISABLEVSYNC);
    stdRenderCapture_PutBool(bDisable);
    stdRenderCapture_EndRecord();
}

int stdDisplay_Update(void)
{
    int result;

    if ( !stdDisplayCapture_bFrameRecording() )
    {
        return stdDisplayBackend_Update();
    }

    result = stdDisplayBackend_Update();
    stdRenderCapture_BeginRecord(STDRENDERCAPTURE_ID_STDDISPLAY_UPDATE);
    stdRenderCapture_PutI32(result);
    stdRenderCapture_EndRecord();
    return result;
}

void J3DAPI stdDisplay_Refresh(int bReload)
{
    if ( !stdRenderCapture_IsOpen() )
    {
        stdDisplayBackend_Refresh(bReload);
        return;
    }

    stdDisplayBackend_Refresh(bReload);
    stdRenderCapture_BeginRecord(STDRENDERCAPTURE_ID_STDDISPLAY_REFRESH);
    stdRenderCapture_PutI32(bReload);
    stdRenderCapture_EndRecord();
}

void stdDisplay_RegisterDevicePreResetCallback(tDisplayDevicePreResetCallback pCallback)
{
    if ( !stdRenderCapture_IsOpen() )
    {
        stdDisplayBackend_RegisterDevicePreResetCallback(pCallback);
        return;
    }

    stdDisplayBackend_RegisterDevicePreResetCallback(pCallback);
    stdRenderCapture_BeginRecord(STDRENDERCAPTURE_ID_STDDISPLAY_REGISTERDEVICEPRERESETCALLBACK);
    stdRenderCapture_PutU8(pCallback != NULL ? 1u : 0u);
    stdRenderCapture_EndRecord();
}

void stdDisplay_RegisterDevicePostResetCallback(tDisplayDevicePostResetCallback pCallback)
{
    if ( !stdRenderCapture_IsOpen() )
    {
        stdDisplayBackend_RegisterDevicePostResetCallback(pCallback);
        return;
    }

    stdDisplayBackend_RegisterDevicePostResetCallback(pCallback);
    stdRenderCapture_BeginRecord(STDRENDERCAPTURE_ID_STDDISPLAY_REGISTERDEVICEPOSTRESETCALLBACK);
    stdRenderCapture_PutU8(pCallback != NULL ? 1u : 0u);
    stdRenderCapture_EndRecord();
}

void stdDisplay_RegisterDeviceReleaseCallback(tDisplayDeviceReleaseCallback pCallback)
{
    if ( !stdRenderCapture_IsOpen() )
    {
        stdDisplayBackend_RegisterDeviceReleaseCallback(pCallback);
        return;
    }

    stdDisplayBackend_RegisterDeviceReleaseCallback(pCallback);
    stdRenderCapture_BeginRecord(STDRENDERCAPTURE_ID_STDDISPLAY_REGISTERDEVICERELEASECALLBACK);
    stdRenderCapture_PutU8(pCallback != NULL ? 1u : 0u);
    stdRenderCapture_EndRecord();
}

size_t stdDisplay_GetNumDevices(void)
{
    size_t result;

    if ( !stdRenderCapture_IsOpen() )
    {
        return stdDisplayBackend_GetNumDevices();
    }

    result = stdDisplayBackend_GetNumDevices();
    stdRenderCapture_BeginRecord(STDRENDERCAPTURE_ID_STDDISPLAY_GETNUMDEVICES);
    stdRenderCapture_PutU64((uint64_t)result);
    stdRenderCapture_EndRecord();
    return result;
}

int J3DAPI stdDisplay_GetDevice(size_t deviceNum, StdDisplayDevice* pDest)
{
    int result;

    if ( !stdRenderCapture_IsOpen() )
    {
        return stdDisplayBackend_GetDevice(deviceNum, pDest);
    }

    result = stdDisplayBackend_GetDevice(deviceNum, pDest);
    stdRenderCapture_BeginRecord(STDRENDERCAPTURE_ID_STDDISPLAY_GETDEVICE);
    stdRenderCapture_PutU64((uint64_t)deviceNum);
    stdRenderCapture_PutU8(pDest != NULL ? 1u : 0u);
    stdRenderCapture_PutI32(result);
    stdRenderCapture_EndRecord();
    return result;
}

int J3DAPI stdDisplay_GetCurrentDevice(StdDisplayDevice* pDevice)
{
    int result;

    if ( !stdRenderCapture_IsOpen() )
    {
        return stdDisplayBackend_GetCurrentDevice(pDevice);
    }

    result = stdDisplayBackend_GetCurrentDevice(pDevice);
    stdRenderCapture_BeginRecord(STDRENDERCAPTURE_ID_STDDISPLAY_GETCURRENTDEVICE);
    stdRenderCapture_PutU8(pDevice != NULL ? 1u : 0u);
    stdRenderCapture_PutI32(result);
    stdRenderCapture_EndRecord();
    return result;
}

const StdDisplayDevice* stdDisplay_GetAllDevices(void)
{
    const StdDisplayDevice* pResult;

    if ( !stdRenderCapture_IsOpen() )
    {
        return stdDisplayBackend_GetAllDevices();
    }

    pResult = stdDisplayBackend_GetAllDevices();
    stdRenderCapture_BeginRecord(STDRENDERCAPTURE_ID_STDDISPLAY_GETALLDEVICES);
    stdRenderCapture_PutU8(pResult != NULL ? 1u : 0u);
    stdRenderCapture_EndRecord();
    return pResult;
}

int J3DAPI stdDisplay_SetMode(size_t modeNum, int bFullscreen, size_t numBackBuffers)
{
    int result;

    if ( !stdRenderCapture_IsOpen() )
    {
        return stdDisplayBackend_SetMode(modeNum, bFullscreen, numBackBuffers);
    }

    result = stdDisplayBackend_SetMode(modeNum, bFullscreen, numBackBuffers);
    stdRenderCapture_BeginRecord(STDRENDERCAPTURE_ID_STDDISPLAY_SETMODE);
    stdRenderCapture_PutU64((uint64_t)modeNum);
    stdRenderCapture_PutI32(bFullscreen);
    stdRenderCapture_PutU64((uint64_t)numBackBuffers);
    stdRenderCapture_PutI32(result);
    stdRenderCapture_EndRecord();
    return result;
}

void stdDisplay_ClearMode(void)
{
    if ( !stdRenderCapture_IsOpen() )
    {
        stdDisplayBackend_ClearMode();
        return;
    }

    stdDisplayBackend_ClearMode();
    stdRenderCapture_BeginRecord(STDRENDERCAPTURE_ID_STDDISPLAY_CLEARMODE);
    stdRenderCapture_EndRecord();
}

size_t stdDisplay_GetNumVideoModes(void)
{
    size_t result;

    if ( !stdRenderCapture_IsOpen() )
    {
        return stdDisplayBackend_GetNumVideoModes();
    }

    result = stdDisplayBackend_GetNumVideoModes();
    stdRenderCapture_BeginRecord(STDRENDERCAPTURE_ID_STDDISPLAY_GETNUMVIDEOMODES);
    stdRenderCapture_PutU64((uint64_t)result);
    stdRenderCapture_EndRecord();
    return result;
}

// The recorded video mode: present byte, aspectRatio, the tRasterInfo encoding, refreshRate.
static void stdDisplayCapture_PutVideoModeOut(const StdVideoMode* pMode)
{
    if ( !pMode )
    {
        stdRenderCapture_PutU8(0u);
        return;
    }

    stdRenderCapture_PutU8(1u);
    stdRenderCapture_PutF32(pMode->aspectRatio);
    stdDisplayCapture_PutRasterFields(&pMode->rasterInfo);
    stdRenderCapture_PutU32(pMode->refreshRate);
}

int J3DAPI stdDisplay_GetVideoMode(size_t modeNum, StdVideoMode* pDestMode)
{
    int result;

    if ( !stdRenderCapture_IsOpen() )
    {
        return stdDisplayBackend_GetVideoMode(modeNum, pDestMode);
    }

    result = stdDisplayBackend_GetVideoMode(modeNum, pDestMode);
    stdRenderCapture_BeginRecord(STDRENDERCAPTURE_ID_STDDISPLAY_GETVIDEOMODE);
    stdRenderCapture_PutU64((uint64_t)modeNum);
    stdRenderCapture_PutI32(result);
    stdDisplayCapture_PutVideoModeOut(pDestMode);
    stdRenderCapture_EndRecord();
    return result;
}

int J3DAPI stdDisplay_GetCurrentVideoMode(StdVideoMode* pDisplayMode)
{
    int result;

    if ( !stdRenderCapture_IsOpen() )
    {
        return stdDisplayBackend_GetCurrentVideoMode(pDisplayMode);
    }

    result = stdDisplayBackend_GetCurrentVideoMode(pDisplayMode);
    stdRenderCapture_BeginRecord(STDRENDERCAPTURE_ID_STDDISPLAY_GETCURRENTVIDEOMODE);
    stdRenderCapture_PutI32(result);
    stdDisplayCapture_PutVideoModeOut(pDisplayMode);
    stdRenderCapture_EndRecord();
    return result;
}

tVBuffer* J3DAPI stdDisplay_VBufferNew(const tRasterInfo* pRasterInfo, int bUseVSurface, int bUseVideoMemory)
{
    tVBuffer* pResult;
    uint32_t handle;

    if ( !stdRenderCapture_IsOpen() )
    {
        return stdDisplayBackend_VBufferNew(pRasterInfo, bUseVSurface, bUseVideoMemory);
    }

    pResult = stdDisplayBackend_VBufferNew(pRasterInfo, bUseVSurface, bUseVideoMemory);
    handle = stdRenderCapture_AddHandle(STDRENDERCAPTURE_HANDLE_VBUFFER, pResult);

    stdRenderCapture_BeginRecord(STDRENDERCAPTURE_ID_STDDISPLAY_VBUFFERNEW);
    stdDisplayCapture_PutRasterInfo(pRasterInfo);
    stdRenderCapture_PutI32(bUseVSurface);
    stdRenderCapture_PutI32(bUseVideoMemory);
    stdRenderCapture_PutU32(handle);
    stdRenderCapture_EndRecord();
    return pResult;
}

void J3DAPI stdDisplay_VBufferFree(tVBuffer* pVBuffer)
{
    uint32_t handle;

    if ( !stdRenderCapture_IsOpen() )
    {
        stdDisplayBackend_VBufferFree(pVBuffer);
        return;
    }

    handle = stdDisplayCapture_VBufferHandle(pVBuffer);
    stdDisplayBackend_VBufferFree(pVBuffer);
    stdRenderCapture_BeginRecord(STDRENDERCAPTURE_ID_STDDISPLAY_VBUFFERFREE);
    stdRenderCapture_PutU32(handle);
    stdRenderCapture_EndRecord();
    stdRenderCapture_ReleaseHandle(STDRENDERCAPTURE_HANDLE_VBUFFER, pVBuffer);
}

int J3DAPI stdDisplay_VBufferLock(tVBuffer* pVBuffer)
{
    int result;
    uint32_t handle;

    if ( !stdRenderCapture_IsOpen() )
    {
        return stdDisplayBackend_VBufferLock(pVBuffer);
    }

    handle = stdDisplayCapture_VBufferHandle(pVBuffer);
    result = stdDisplayBackend_VBufferLock(pVBuffer);
    stdRenderCapture_BeginRecord(STDRENDERCAPTURE_ID_STDDISPLAY_VBUFFERLOCK);
    stdRenderCapture_PutU32(handle);
    stdRenderCapture_PutI32(result);
    stdRenderCapture_EndRecord();
    return result;
}

int stdDisplay_VBufferLockReadOnly(tVBuffer* pVBuffer)
{
    int result;
    uint32_t handle;

    if ( !stdRenderCapture_IsOpen() )
    {
        return stdDisplayBackend_VBufferLockReadOnly(pVBuffer);
    }

    handle = stdDisplayCapture_VBufferHandle(pVBuffer);
    result = stdDisplayBackend_VBufferLockReadOnly(pVBuffer);
    stdRenderCapture_BeginRecord(STDRENDERCAPTURE_ID_STDDISPLAY_VBUFFERLOCKREADONLY);
    stdRenderCapture_PutU32(handle);
    stdRenderCapture_PutI32(result);
    stdRenderCapture_EndRecord();
    return result;
}

int J3DAPI stdDisplay_VBufferUnlock(tVBuffer* pVBuffer)
{
    int result;
    uint32_t handle;
    uint32_t blobId = 0u;

    if ( !stdRenderCapture_IsOpen() )
    {
        return stdDisplayBackend_VBufferUnlock(pVBuffer);
    }

    handle = stdDisplayCapture_VBufferHandle(pVBuffer);

    // The buffer is captured when this unlock takes it from the locked state (lockRefCount 1)
    // down to unlocked: its content is read before the backend call clears the lock.
    if ( pVBuffer && pVBuffer->lockRefCount == 1u )
    {
        blobId = stdDisplayCapture_WriteVBufferBlob(pVBuffer);
    }

    result = stdDisplayBackend_VBufferUnlock(pVBuffer);
    stdRenderCapture_BeginRecord(STDRENDERCAPTURE_ID_STDDISPLAY_VBUFFERUNLOCK);
    stdRenderCapture_PutU32(handle);
    stdRenderCapture_PutU32(blobId);
    stdRenderCapture_PutI32(result);
    stdRenderCapture_EndRecord();
    return result;
}

int J3DAPI stdDisplay_VBufferFill(tVBuffer* pVBuffer, uint32_t dwFillColor, const StdRect* pRect)
{
    int result;
    uint32_t handle;

    if ( !stdRenderCapture_IsOpen() )
    {
        return stdDisplayBackend_VBufferFill(pVBuffer, dwFillColor, pRect);
    }

    handle = stdDisplayCapture_VBufferHandle(pVBuffer);
    result = stdDisplayBackend_VBufferFill(pVBuffer, dwFillColor, pRect);
    stdRenderCapture_BeginRecord(STDRENDERCAPTURE_ID_STDDISPLAY_VBUFFERFILL);
    stdRenderCapture_PutU32(handle);
    stdRenderCapture_PutU32(dwFillColor);
    stdDisplayCapture_PutRect(pRect);
    stdRenderCapture_PutI32(result);
    stdRenderCapture_EndRecord();
    return result;
}

tVBuffer* J3DAPI stdDisplay_VBufferConvertColorFormat(const ColorInfo* pDesiredColorFormat, tVBuffer* pSrc, int bColorKey, LPDDCOLORKEY pColorKey)
{
    tVBuffer* pResult;
    uint32_t srcHandle = 0u;
    uint32_t srcBlobId = 0u;
    uint32_t resultHandle;

    if ( !stdRenderCapture_IsOpen() )
    {
        return stdDisplayBackend_VBufferConvertColorFormat(pDesiredColorFormat, pSrc, bColorKey, pColorKey);
    }

    srcHandle = stdDisplayCapture_VBufferHandle(pSrc);
    srcBlobId = stdDisplayCapture_WriteVBufferBlob(pSrc);
    pResult = stdDisplayBackend_VBufferConvertColorFormat(pDesiredColorFormat, pSrc, bColorKey, pColorKey);
    resultHandle = stdRenderCapture_AddHandle(STDRENDERCAPTURE_HANDLE_VBUFFER, pResult);

    stdRenderCapture_BeginRecord(STDRENDERCAPTURE_ID_STDDISPLAY_VBUFFERCONVERTCOLORFORMAT);
    stdDisplayCapture_PutColorInfo(pDesiredColorFormat);
    stdRenderCapture_PutU32(srcHandle);
    stdRenderCapture_PutU32(srcBlobId);
    stdRenderCapture_PutI32(bColorKey);
    stdDisplayCapture_PutColorKey(pColorKey);
    stdRenderCapture_PutU32(resultHandle);
    stdRenderCapture_EndRecord();
    return pResult;
}

int J3DAPI stdDisplay_GetTextureMemory(size_t* pTotal, size_t* pFree)
{
    int result;

    if ( !stdRenderCapture_IsOpen() )
    {
        return stdDisplayBackend_GetTextureMemory(pTotal, pFree);
    }

    result = stdDisplayBackend_GetTextureMemory(pTotal, pFree);
    stdRenderCapture_BeginRecord(STDRENDERCAPTURE_ID_STDDISPLAY_GETTEXTUREMEMORY);
    stdRenderCapture_PutI32(result);
    stdRenderCapture_EndRecord();
    return result;
}

int J3DAPI stdDisplay_GetTotalMemory(size_t* pTotal, size_t* pFree)
{
    int result;

    if ( !stdRenderCapture_IsOpen() )
    {
        return stdDisplayBackend_GetTotalMemory(pTotal, pFree);
    }

    result = stdDisplayBackend_GetTotalMemory(pTotal, pFree);
    stdRenderCapture_BeginRecord(STDRENDERCAPTURE_ID_STDDISPLAY_GETTOTALMEMORY);
    stdRenderCapture_PutI32(result);
    stdRenderCapture_EndRecord();
    return result;
}

void J3DAPI stdDisplay_GetBackBufferSize(uint32_t* pWidth, uint32_t* pHeight)
{
    if ( !stdDisplayCapture_bFrameRecording() )
    {
        stdDisplayBackend_GetBackBufferSize(pWidth, pHeight);
        return;
    }

    stdDisplayBackend_GetBackBufferSize(pWidth, pHeight);
    stdRenderCapture_BeginRecord(STDRENDERCAPTURE_ID_STDDISPLAY_GETBACKBUFFERSIZE);
    stdRenderCapture_PutU32(pWidth ? *pWidth : 0u);
    stdRenderCapture_PutU32(pHeight ? *pHeight : 0u);
    stdRenderCapture_EndRecord();
}

int J3DAPI stdDisplay_BackBufferFill(uint32_t color, const StdRect* lpRect)
{
    int result;

    if ( !stdDisplayCapture_bFrameRecording() )
    {
        return stdDisplayBackend_BackBufferFill(color, lpRect);
    }

    result = stdDisplayBackend_BackBufferFill(color, lpRect);
    stdRenderCapture_BeginRecord(STDRENDERCAPTURE_ID_STDDISPLAY_BACKBUFFERFILL);
    stdRenderCapture_PutU32(color);
    stdDisplayCapture_PutRect(lpRect);
    stdRenderCapture_PutI32(result);
    stdRenderCapture_EndRecord();
    return result;
}

int J3DAPI stdDisplay_SaveScreen(const char* pFilename)
{
    int result;

    if ( !stdRenderCapture_IsOpen() )
    {
        return stdDisplayBackend_SaveScreen(pFilename);
    }

    result = stdDisplayBackend_SaveScreen(pFilename);
    stdRenderCapture_BeginRecord(STDRENDERCAPTURE_ID_STDDISPLAY_SAVESCREEN);
    stdRenderCapture_PutString(pFilename);
    stdRenderCapture_PutI32(result);
    stdRenderCapture_EndRecord();
    return result;
}

void J3DAPI stdDisplay_SetDefaultResolution(uint32_t width, uint32_t height)
{
    if ( !stdRenderCapture_IsOpen() )
    {
        stdDisplayBackend_SetDefaultResolution(width, height);
        return;
    }

    stdDisplayBackend_SetDefaultResolution(width, height);
    stdRenderCapture_BeginRecord(STDRENDERCAPTURE_ID_STDDISPLAY_SETDEFAULTRESOLUTION);
    stdRenderCapture_PutU32(width);
    stdRenderCapture_PutU32(height);
    stdRenderCapture_EndRecord();
}

tSysDrawContext stdDisplay_GetFrontBufferDC(void)
{
    tSysDrawContext hdc;

    if ( !stdRenderCapture_IsOpen() )
    {
        return stdDisplayBackend_GetFrontBufferDC();
    }

    hdc = stdDisplayBackend_GetFrontBufferDC();
    stdRenderCapture_BeginRecord(STDRENDERCAPTURE_ID_STDDISPLAY_GETFRONTBUFFERDC);
    stdRenderCapture_PutU8(hdc != NULL ? 1u : 0u);
    stdRenderCapture_EndRecord();
    return hdc;
}

void J3DAPI stdDisplay_ReleaseFrontBufferDC(tSysDrawContext hdc)
{
    if ( !stdRenderCapture_IsOpen() )
    {
        stdDisplayBackend_ReleaseFrontBufferDC(hdc);
        return;
    }

    stdDisplayBackend_ReleaseFrontBufferDC(hdc);
    stdRenderCapture_BeginRecord(STDRENDERCAPTURE_ID_STDDISPLAY_RELEASEFRONTBUFFERDC);
    stdRenderCapture_PutU8(hdc != NULL ? 1u : 0u);
    stdRenderCapture_EndRecord();
}

tSysDrawContext stdDisplay_GetBackBufferDC(void)
{
    tSysDrawContext hdc;

    if ( !stdRenderCapture_IsOpen() )
    {
        return stdDisplayBackend_GetBackBufferDC();
    }

    hdc = stdDisplayBackend_GetBackBufferDC();
    stdRenderCapture_BeginRecord(STDRENDERCAPTURE_ID_STDDISPLAY_GETBACKBUFFERDC);
    stdRenderCapture_PutU8(hdc != NULL ? 1u : 0u);
    stdRenderCapture_EndRecord();
    return hdc;
}

void J3DAPI stdDisplay_ReleaseBackBufferDC(tSysDrawContext hdc)
{
    if ( !stdRenderCapture_IsOpen() )
    {
        stdDisplayBackend_ReleaseBackBufferDC(hdc);
        return;
    }

    stdDisplayBackend_ReleaseBackBufferDC(hdc);
    stdRenderCapture_BeginRecord(STDRENDERCAPTURE_ID_STDDISPLAY_RELEASEBACKBUFFERDC);
    stdRenderCapture_PutU8(hdc != NULL ? 1u : 0u);
    stdRenderCapture_EndRecord();
}

int stdDisplay_FlipToGDISurface(void)
{
    int result;

    if ( !stdRenderCapture_IsOpen() )
    {
        return stdDisplayBackend_FlipToGDISurface();
    }

    result = stdDisplayBackend_FlipToGDISurface();
    stdRenderCapture_BeginRecord(STDRENDERCAPTURE_ID_STDDISPLAY_FLIPTOGDISURFACE);
    stdRenderCapture_PutI32(result);
    stdRenderCapture_EndRecord();
    return result;
}

int J3DAPI stdDisplay_SetBufferClipper(int bFrontBuffer)
{
    int result;

    if ( !stdRenderCapture_IsOpen() )
    {
        return stdDisplayBackend_SetBufferClipper(bFrontBuffer);
    }

    result = stdDisplayBackend_SetBufferClipper(bFrontBuffer);
    stdRenderCapture_BeginRecord(STDRENDERCAPTURE_ID_STDDISPLAY_SETBUFFERCLIPPER);
    stdRenderCapture_PutI32(bFrontBuffer);
    stdRenderCapture_PutI32(result);
    stdRenderCapture_EndRecord();
    return result;
}

int J3DAPI stdDisplay_RemoveBufferClipper(int bFrontBuffer)
{
    int result;

    if ( !stdRenderCapture_IsOpen() )
    {
        return stdDisplayBackend_RemoveBufferClipper(bFrontBuffer);
    }

    result = stdDisplayBackend_RemoveBufferClipper(bFrontBuffer);
    stdRenderCapture_BeginRecord(STDRENDERCAPTURE_ID_STDDISPLAY_REMOVEBUFFERCLIPPER);
    stdRenderCapture_PutI32(bFrontBuffer);
    stdRenderCapture_PutI32(result);
    stdRenderCapture_EndRecord();
    return result;
}

int stdDisplay_CanRenderWindowed(void)
{
    int result;

    if ( !stdRenderCapture_IsOpen() )
    {
        return stdDisplayBackend_CanRenderWindowed();
    }

    result = stdDisplayBackend_CanRenderWindowed();
    stdRenderCapture_BeginRecord(STDRENDERCAPTURE_ID_STDDISPLAY_CANRENDERWINDOWED);
    stdRenderCapture_PutI32(result);
    stdRenderCapture_EndRecord();
    return result;
}

int stdDisplay_IsFullscreen(void)
{
    int result;

    if ( !stdRenderCapture_IsOpen() )
    {
        return stdDisplayBackend_IsFullscreen();
    }

    result = stdDisplayBackend_IsFullscreen();
    stdRenderCapture_BeginRecord(STDRENDERCAPTURE_ID_STDDISPLAY_ISFULLSCREEN);
    stdRenderCapture_PutI32(result);
    stdRenderCapture_EndRecord();
    return result;
}

// The remembered back-buffer lock of the forwarding layer: the pixels of every successful
// LockBackBuffer / LockBackBufferReadOnly (both backends return 0 on success), so the matching
// Unlock can capture what was drawn into them.
static void stdDisplayCapture_RememberBackBufferLock(int result, void** ppSurface, uint32_t* pHeight, int32_t* pPitch)
{
    if ( result != 0 || !ppSurface || !*ppSurface )
    {
        return;
    }

    stdDisplayCapture_pBackBufferPixels    = *ppSurface;
    stdDisplayCapture_backBufferLockHeight = pHeight ? *pHeight : 0u;
    stdDisplayCapture_backBufferLockPitch  = pPitch ? *pPitch : 0;
}

static void stdDisplayCapture_RecordBackBufferLock(
    uint16_t id,
    int result,
    uint32_t* pWidth,
    uint32_t* pHeight,
    int32_t* pPitch
)
{
    stdRenderCapture_BeginRecord(id);
    stdRenderCapture_PutI32(result);
    stdRenderCapture_PutU32(pWidth ? *pWidth : 0u);
    stdRenderCapture_PutU32(pHeight ? *pHeight : 0u);
    stdRenderCapture_PutI32(pPitch ? *pPitch : 0);
    stdRenderCapture_EndRecord();
}

int J3DAPI stdDisplay_LockBackBuffer(void** ppSurface, uint32_t* pWidth, uint32_t* pHeight, int32_t* pPitch)
{
    int result = stdDisplayBackend_LockBackBuffer(ppSurface, pWidth, pHeight, pPitch);

    stdDisplayCapture_RememberBackBufferLock(result, ppSurface, pHeight, pPitch);
    if ( !stdDisplayCapture_bFrameRecording() )
    {
        return result;
    }

    stdDisplayCapture_RecordBackBufferLock(STDRENDERCAPTURE_ID_STDDISPLAY_LOCKBACKBUFFER, result, pWidth, pHeight, pPitch);
    return result;
}

int stdDisplay_LockBackBufferReadOnly(void** ppSurface, uint32_t* pWidth, uint32_t* pHeight, int32_t* pPitch)
{
    int result = stdDisplayBackend_LockBackBufferReadOnly(ppSurface, pWidth, pHeight, pPitch);

    stdDisplayCapture_RememberBackBufferLock(result, ppSurface, pHeight, pPitch);
    if ( !stdDisplayCapture_bFrameRecording() )
    {
        return result;
    }

    stdDisplayCapture_RecordBackBufferLock(STDRENDERCAPTURE_ID_STDDISPLAY_LOCKBACKBUFFERREADONLY, result, pWidth, pHeight, pPitch);
    return result;
}

void stdDisplay_UnlockBackBuffer(void)
{
    uint32_t blobId = 0u;

    if ( stdDisplayCapture_pBackBufferPixels && stdDisplayCapture_bFrameRecording() )
    {
        uint64_t bytes;

        if ( stdDisplayCapture_backBufferLockPitch > 0 )
        {
            bytes = (uint64_t)stdDisplayCapture_backBufferLockHeight * (uint64_t)stdDisplayCapture_backBufferLockPitch;
            if ( bytes <= (uint64_t)SIZE_MAX )
            {
                blobId = stdRenderCapture_WriteBlob(stdDisplayCapture_pBackBufferPixels, (size_t)bytes);
            }
            else if ( !stdDisplayCapture_bWarnedBlobSize )
            {
                stdDisplayCapture_bWarnedBlobSize = true;
                STDLOG_WARNING("Render capture: a back-buffer content size overflowed; recorded blob 0.\n");
            }
        }
        else if ( !stdDisplayCapture_bWarnedBackBufferPitch )
        {
            stdDisplayCapture_bWarnedBackBufferPitch = true;
            STDLOG_WARNING("Render capture: a remembered back-buffer lock has no positive pitch; recorded blob 0.\n");
        }
    }

    // The lock is forgotten whether or not this call was recorded.
    stdDisplayCapture_pBackBufferPixels    = NULL;
    stdDisplayCapture_backBufferLockHeight = 0u;
    stdDisplayCapture_backBufferLockPitch  = 0;

    if ( !stdDisplayCapture_bFrameRecording() )
    {
        stdDisplayBackend_UnlockBackBuffer();
        return;
    }

    stdDisplayBackend_UnlockBackBuffer();
    stdRenderCapture_BeginRecord(STDRENDERCAPTURE_ID_STDDISPLAY_UNLOCKBACKBUFFER);
    stdRenderCapture_PutU32(blobId);
    stdRenderCapture_EndRecord();
}

uint32_t J3DAPI stdDisplay_EncodeFromRGB565(uint16_t pixel)
{
    uint32_t result;

    if ( !stdRenderCapture_IsOpen() )
    {
        return stdDisplayBackend_EncodeFromRGB565(pixel);
    }

    result = stdDisplayBackend_EncodeFromRGB565(pixel);
    stdRenderCapture_BeginRecord(STDRENDERCAPTURE_ID_STDDISPLAY_ENCODEFROMRGB565);
    stdRenderCapture_PutU16(pixel);
    stdRenderCapture_PutU32(result);
    stdRenderCapture_EndRecord();
    return result;
}

void stdDisplay_ResetGlobals(void)
{
    if ( !stdRenderCapture_IsOpen() )
    {
        stdDisplayBackend_ResetGlobals();
        return;
    }

    stdDisplayBackend_ResetGlobals();
    stdRenderCapture_BeginRecord(STDRENDERCAPTURE_ID_STDDISPLAY_RESETGLOBALS);
    stdRenderCapture_EndRecord();
}

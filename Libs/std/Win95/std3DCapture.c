// Forwarding layer of the render capture (M6.0): one public std3D_ name per renderer function,
// forwarding to the backend and, while a capture is open, recording every call. Every recorded
// std3D call first makes the current fog density available as GLOBAL 1 (the writer dedups it).

#include <string.h>

#include <std/General/std.h>
#include <std/General/stdMemory.h>
#include <std/General/stdRenderCapture.h>
#include <std/General/stdRenderCaptureRecord.h>

#include <std/Win95/std3D.h>
#include <std/Win95/stdRenderBackendApi.h>
#include <std/Win95/stdRenderCaptureForward.h>
#include <std/Win95/stdRenderCaptureIds.h>

static bool std3DCapture_bFrameRecording(void)
{
    return stdRenderCapture_IsOpen() && stdRenderCapture_GetFramesEnabled();
}

// GLOBAL 1: the bits of the fog density the backend sees before this call. The writer emits it
// only when it changed since the last GLOBAL 1 of the capture.
static void std3DCapture_WriteFogGlobal(void)
{
    uint32_t bits;

    memcpy(&bits, &std3D_g_fogDensity, sizeof(bits));
    stdRenderCapture_WriteGlobal(1u, bits);
}

// First sight of a system texture is normal (std3D_AllocSystemTexture), so it is silent.
static uint32_t std3DCapture_SysTextureHandle(const tSystemTexture* pTexture)
{
    uint32_t handle;

    if ( !pTexture )
    {
        return 0u;
    }

    handle = stdRenderCapture_FindHandle(STDRENDERCAPTURE_HANDLE_SYSTEXTURE, pTexture);
    if ( handle )
    {
        return handle;
    }

    return stdRenderCapture_AddHandle(STDRENDERCAPTURE_HANDLE_SYSTEXTURE, pTexture);
}

int std3D_Startup(void)
{
    int result;

    if ( !stdRenderCapture_IsOpen() )
    {
        return std3DBackend_Startup();
    }

    std3DCapture_WriteFogGlobal();
    result = std3DBackend_Startup();
    stdRenderCapture_BeginRecord(STDRENDERCAPTURE_ID_STD3D_STARTUP);
    stdRenderCapture_PutI32(result);
    stdRenderCapture_EndRecord();
    return result;
}

void std3D_Shutdown(void)
{
    if ( !stdRenderCapture_IsOpen() )
    {
        std3DBackend_Shutdown();
        return;
    }

    std3DCapture_WriteFogGlobal();
    std3DBackend_Shutdown();
    stdRenderCapture_BeginRecord(STDRENDERCAPTURE_ID_STD3D_SHUTDOWN);
    stdRenderCapture_EndRecord();
}

const Device3D* std3D_GetCurrentDevice(void)
{
    const Device3D* pResult;

    if ( !stdRenderCapture_IsOpen() )
    {
        return std3DBackend_GetCurrentDevice();
    }

    std3DCapture_WriteFogGlobal();
    pResult = std3DBackend_GetCurrentDevice();
    stdRenderCapture_BeginRecord(STDRENDERCAPTURE_ID_STD3D_GETCURRENTDEVICE);
    stdRenderCapture_PutU8(pResult != NULL ? 1u : 0u);
    stdRenderCapture_EndRecord();
    return pResult;
}

size_t std3D_GetNumDevices(void)
{
    size_t result;

    if ( !stdRenderCapture_IsOpen() )
    {
        return std3DBackend_GetNumDevices();
    }

    std3DCapture_WriteFogGlobal();
    result = std3DBackend_GetNumDevices();
    stdRenderCapture_BeginRecord(STDRENDERCAPTURE_ID_STD3D_GETNUMDEVICES);
    stdRenderCapture_PutU64((uint64_t)result);
    stdRenderCapture_EndRecord();
    return result;
}

const Device3D* std3D_GetAllDevices(void)
{
    const Device3D* pResult;

    if ( !stdRenderCapture_IsOpen() )
    {
        return std3DBackend_GetAllDevices();
    }

    std3DCapture_WriteFogGlobal();
    pResult = std3DBackend_GetAllDevices();
    stdRenderCapture_BeginRecord(STDRENDERCAPTURE_ID_STD3D_GETALLDEVICES);
    stdRenderCapture_PutU8(pResult != NULL ? 1u : 0u);
    stdRenderCapture_EndRecord();
    return pResult;
}

void J3DAPI std3D_SetFindAllDevices(int bFindAll)
{
    if ( !stdRenderCapture_IsOpen() )
    {
        std3DBackend_SetFindAllDevices(bFindAll);
        return;
    }

    std3DCapture_WriteFogGlobal();
    std3DBackend_SetFindAllDevices(bFindAll);
    stdRenderCapture_BeginRecord(STDRENDERCAPTURE_ID_STD3D_SETFINDALLDEVICES);
    stdRenderCapture_PutI32(bFindAll);
    stdRenderCapture_EndRecord();
}

int J3DAPI std3D_Open(size_t deviceNum)
{
    int result;

    if ( !stdRenderCapture_IsOpen() )
    {
        return std3DBackend_Open(deviceNum);
    }

    std3DCapture_WriteFogGlobal();
    result = std3DBackend_Open(deviceNum);
    stdRenderCapture_BeginRecord(STDRENDERCAPTURE_ID_STD3D_OPEN);
    stdRenderCapture_PutU64((uint64_t)deviceNum);
    stdRenderCapture_PutI32(result);
    stdRenderCapture_EndRecord();
    return result;
}

void std3D_Close(void)
{
    if ( !stdRenderCapture_IsOpen() )
    {
        std3DBackend_Close();
        return;
    }

    std3DCapture_WriteFogGlobal();
    std3DBackend_Close();
    stdRenderCapture_BeginRecord(STDRENDERCAPTURE_ID_STD3D_CLOSE);
    stdRenderCapture_EndRecord();
}

void J3DAPI std3D_GetTextureFormat(StdColorFormatType type, ColorInfo* pDest, int* pbColorKeySet, LPDDCOLORKEY* ppColorKey)
{
    if ( !stdRenderCapture_IsOpen() )
    {
        std3DBackend_GetTextureFormat(type, pDest, pbColorKeySet, ppColorKey);
        return;
    }

    std3DCapture_WriteFogGlobal();
    std3DBackend_GetTextureFormat(type, pDest, pbColorKeySet, ppColorKey);
    stdRenderCapture_BeginRecord(STDRENDERCAPTURE_ID_STD3D_GETTEXTUREFORMAT);
    stdRenderCapture_PutI32((int32_t)type);
    stdRenderCapture_EndRecord();
}

StdColorFormatType J3DAPI std3D_GetColorFormat(const ColorInfo* pCi)
{
    StdColorFormatType result;

    if ( !stdRenderCapture_IsOpen() )
    {
        return std3DBackend_GetColorFormat(pCi);
    }

    std3DCapture_WriteFogGlobal();
    result = std3DBackend_GetColorFormat(pCi);
    stdRenderCapture_BeginRecord(STDRENDERCAPTURE_ID_STD3D_GETCOLORFORMAT);
    stdDisplayCapture_PutColorInfo(pCi);
    stdRenderCapture_PutI32((int32_t)result);
    stdRenderCapture_EndRecord();
    return result;
}

size_t std3D_GetNumTextureFormats(void)
{
    size_t result;

    if ( !stdRenderCapture_IsOpen() )
    {
        return std3DBackend_GetNumTextureFormats();
    }

    std3DCapture_WriteFogGlobal();
    result = std3DBackend_GetNumTextureFormats();
    stdRenderCapture_BeginRecord(STDRENDERCAPTURE_ID_STD3D_GETNUMTEXTUREFORMATS);
    stdRenderCapture_PutU64((uint64_t)result);
    stdRenderCapture_EndRecord();
    return result;
}

int std3D_StartScene(void)
{
    int result;

    if ( !std3DCapture_bFrameRecording() )
    {
        return std3DBackend_StartScene();
    }

    std3DCapture_WriteFogGlobal();
    result = std3DBackend_StartScene();
    stdRenderCapture_BeginRecord(STDRENDERCAPTURE_ID_STD3D_STARTSCENE);
    stdRenderCapture_PutI32(result);
    stdRenderCapture_EndRecord();
    return result;
}

void std3D_EndScene(void)
{
    if ( !std3DCapture_bFrameRecording() )
    {
        std3DBackend_EndScene();
        return;
    }

    std3DCapture_WriteFogGlobal();
    std3DBackend_EndScene();
    stdRenderCapture_BeginRecord(STDRENDERCAPTURE_ID_STD3D_ENDSCENE);
    stdRenderCapture_EndRecord();
}

void J3DAPI std3D_DrawRenderList(tSysTexture* pTex, Std3DRenderState rdflags, StdVertexTL* aVerts, size_t numVerts, uint16_t* aIndices, size_t numIndices)
{
    uint32_t texHandle;

    if ( !std3DCapture_bFrameRecording() )
    {
        std3DBackend_DrawRenderList(pTex, rdflags, aVerts, numVerts, aIndices, numIndices);
        return;
    }

    std3DCapture_WriteFogGlobal();
    texHandle = stdDisplayCapture_CacheTextureHandle(pTex);
    std3DBackend_DrawRenderList(pTex, rdflags, aVerts, numVerts, aIndices, numIndices);
    stdRenderCapture_BeginRecord(STDRENDERCAPTURE_ID_STD3D_DRAWRENDERLIST);
    stdRenderCapture_PutU32(texHandle);
    stdRenderCapture_PutI32((int32_t)rdflags);
    stdDisplayCapture_PutVertices(aVerts, numVerts);
    stdDisplayCapture_PutIndices(aIndices, numIndices);
    stdRenderCapture_EndRecord();
}

void J3DAPI std3D_DrawLineStrip(StdVertexTL* aVerts, size_t numVerts)
{
    if ( !std3DCapture_bFrameRecording() )
    {
        std3DBackend_DrawLineStrip(aVerts, numVerts);
        return;
    }

    std3DCapture_WriteFogGlobal();
    std3DBackend_DrawLineStrip(aVerts, numVerts);
    stdRenderCapture_BeginRecord(STDRENDERCAPTURE_ID_STD3D_DRAWLINESTRIP);
    stdDisplayCapture_PutVertices(aVerts, numVerts);
    stdRenderCapture_EndRecord();
}

void J3DAPI std3D_DrawPointList(StdVertexTL* aVerts, size_t numVerts)
{
    if ( !std3DCapture_bFrameRecording() )
    {
        std3DBackend_DrawPointList(aVerts, numVerts);
        return;
    }

    std3DCapture_WriteFogGlobal();
    std3DBackend_DrawPointList(aVerts, numVerts);
    stdRenderCapture_BeginRecord(STDRENDERCAPTURE_ID_STD3D_DRAWPOINTLIST);
    stdDisplayCapture_PutVertices(aVerts, numVerts);
    stdRenderCapture_EndRecord();
}

void J3DAPI std3D_SetRenderState(Std3DRenderState rdflags)
{
    if ( !stdRenderCapture_IsOpen() )
    {
        std3DBackend_SetRenderState(rdflags);
        return;
    }

    std3DCapture_WriteFogGlobal();
    std3DBackend_SetRenderState(rdflags);
    stdRenderCapture_BeginRecord(STDRENDERCAPTURE_ID_STD3D_SETRENDERSTATE);
    stdRenderCapture_PutI32((int32_t)rdflags);
    stdRenderCapture_EndRecord();
}

void std3D_SetWireframeRenderState(void)
{
    if ( !stdRenderCapture_IsOpen() )
    {
        std3DBackend_SetWireframeRenderState();
        return;
    }

    std3DCapture_WriteFogGlobal();
    std3DBackend_SetWireframeRenderState();
    stdRenderCapture_BeginRecord(STDRENDERCAPTURE_ID_STD3D_SETWIREFRAMERENDERSTATE);
    stdRenderCapture_EndRecord();
}

void J3DAPI std3D_AllocSystemTexture(tSystemTexture* pTexture, tVBuffer** apVBuffers, size_t numMipLevels, StdColorFormatType formatType)
{
    typedef struct sStd3DCaptureMip
    {
        uint32_t handle;
        uint32_t blobId;
    } Std3DCaptureMip;

    Std3DCaptureMip* pMips;
    uint32_t textureHandle;
    size_t num;
    size_t i;

    if ( !stdRenderCapture_IsOpen() )
    {
        std3DBackend_AllocSystemTexture(pTexture, apVBuffers, numMipLevels, formatType);
        return;
    }

    std3DCapture_WriteFogGlobal();

    // The mip content and handles are read before the backend call, while the caller still owns
    // the arrays; a NULL apVBuffers records count 0.
    textureHandle = std3DCapture_SysTextureHandle(pTexture);
    if ( !apVBuffers || numMipLevels > (size_t)UINT32_MAX )
    {
        num = 0u;
    }
    else
    {
        num = numMipLevels;
    }

    pMips = NULL;
    if ( num > 0u )
    {
        pMips = (Std3DCaptureMip*)STDMALLOC(num * sizeof(*pMips));
        if ( !pMips )
        {
            STDLOG_ERROR("Render capture: could not allocate the mipmap blob list.\n");
            num = 0u;
        }
    }

    for ( i = 0; i < num; ++i )
    {
        pMips[i].handle = stdDisplayCapture_VBufferHandle(apVBuffers[i]);
        pMips[i].blobId = stdDisplayCapture_WriteVBufferBlob(apVBuffers[i]);
    }

    std3DBackend_AllocSystemTexture(pTexture, apVBuffers, numMipLevels, formatType);

    stdRenderCapture_BeginRecord(STDRENDERCAPTURE_ID_STD3D_ALLOCSYSTEMTEXTURE);
    stdRenderCapture_PutU32(textureHandle);
    stdRenderCapture_PutU32((uint32_t)num);
    for ( i = 0; i < num; ++i )
    {
        stdRenderCapture_PutU32(pMips[i].handle);
        stdRenderCapture_PutU32(pMips[i].blobId);
    }

    stdRenderCapture_PutI32((int32_t)formatType);
    stdRenderCapture_EndRecord();

    if ( pMips )
    {
        STDFREE(pMips);
    }
}

void J3DAPI std3D_GetValidDimensions(uint32_t width, uint32_t height, uint32_t* pOutWidth, uint32_t* pOutHeight)
{
    if ( !stdRenderCapture_IsOpen() )
    {
        std3DBackend_GetValidDimensions(width, height, pOutWidth, pOutHeight);
        return;
    }

    std3DCapture_WriteFogGlobal();
    std3DBackend_GetValidDimensions(width, height, pOutWidth, pOutHeight);
    stdRenderCapture_BeginRecord(STDRENDERCAPTURE_ID_STD3D_GETVALIDDIMENSIONS);
    stdRenderCapture_PutU32(width);
    stdRenderCapture_PutU32(height);
    stdRenderCapture_PutU32(pOutWidth ? *pOutWidth : 0u);
    stdRenderCapture_PutU32(pOutHeight ? *pOutHeight : 0u);
    stdRenderCapture_EndRecord();
}

void J3DAPI std3D_ClearSystemTexture(tSystemTexture* pTex)
{
    uint32_t handle;

    if ( !stdRenderCapture_IsOpen() )
    {
        std3DBackend_ClearSystemTexture(pTex);
        return;
    }

    std3DCapture_WriteFogGlobal();
    handle = std3DCapture_SysTextureHandle(pTex);
    std3DBackend_ClearSystemTexture(pTex);
    stdRenderCapture_BeginRecord(STDRENDERCAPTURE_ID_STD3D_CLEARSYSTEMTEXTURE);
    stdRenderCapture_PutU32(handle);
    stdRenderCapture_EndRecord();
    stdRenderCapture_ReleaseHandle(STDRENDERCAPTURE_HANDLE_SYSTEXTURE, pTex);
}

void J3DAPI std3D_AddToTextureCache(tSystemTexture* pCacheTexture, StdColorFormatType format)
{
    uint32_t handle;

    if ( !stdRenderCapture_IsOpen() )
    {
        std3DBackend_AddToTextureCache(pCacheTexture, format);
        return;
    }

    std3DCapture_WriteFogGlobal();
    handle = std3DCapture_SysTextureHandle(pCacheTexture);
    std3DBackend_AddToTextureCache(pCacheTexture, format);
    stdRenderCapture_BeginRecord(STDRENDERCAPTURE_ID_STD3D_ADDTOTEXTURECACHE);
    stdRenderCapture_PutU32(handle);
    stdRenderCapture_PutI32((int32_t)format);
    stdRenderCapture_EndRecord();
}

void std3D_ResetTextureCache(void)
{
    if ( !stdRenderCapture_IsOpen() )
    {
        std3DBackend_ResetTextureCache();
        return;
    }

    std3DCapture_WriteFogGlobal();
    std3DBackend_ResetTextureCache();
    stdRenderCapture_BeginRecord(STDRENDERCAPTURE_ID_STD3D_RESETTEXTURECACHE);
    stdRenderCapture_EndRecord();
}

size_t J3DAPI std3D_GetMipMapCount(const tSystemTexture* pTexture)
{
    size_t result;
    uint32_t handle;

    if ( !stdRenderCapture_IsOpen() )
    {
        return std3DBackend_GetMipMapCount(pTexture);
    }

    std3DCapture_WriteFogGlobal();
    handle = std3DCapture_SysTextureHandle(pTexture);
    result = std3DBackend_GetMipMapCount(pTexture);
    stdRenderCapture_BeginRecord(STDRENDERCAPTURE_ID_STD3D_GETMIPMAPCOUNT);
    stdRenderCapture_PutU32(handle);
    stdRenderCapture_PutU64((uint64_t)result);
    stdRenderCapture_EndRecord();
    return result;
}

void J3DAPI std3D_UpdateFrameCount(tSystemTexture* pTexture)
{
    uint32_t handle;

    if ( !std3DCapture_bFrameRecording() )
    {
        std3DBackend_UpdateFrameCount(pTexture);
        return;
    }

    std3DCapture_WriteFogGlobal();
    handle = std3DCapture_SysTextureHandle(pTexture);
    std3DBackend_UpdateFrameCount(pTexture);
    stdRenderCapture_BeginRecord(STDRENDERCAPTURE_ID_STD3D_UPDATEFRAMECOUNT);
    stdRenderCapture_PutU32(handle);
    stdRenderCapture_EndRecord();
}

size_t J3DAPI std3D_FindClosestFormat(const ColorInfo* pMatch)
{
    size_t result;

    if ( !stdRenderCapture_IsOpen() )
    {
        return std3DBackend_FindClosestFormat(pMatch);
    }

    std3DCapture_WriteFogGlobal();
    result = std3DBackend_FindClosestFormat(pMatch);
    stdRenderCapture_BeginRecord(STDRENDERCAPTURE_ID_STD3D_FINDCLOSESTFORMAT);
    stdDisplayCapture_PutColorInfo(pMatch);
    stdRenderCapture_PutU64((uint64_t)result);
    stdRenderCapture_EndRecord();
    return result;
}

int J3DAPI std3D_SetMipmapFilter(Std3DMipmapFilterType mode)
{
    int result;

    if ( !stdRenderCapture_IsOpen() )
    {
        return std3DBackend_SetMipmapFilter(mode);
    }

    std3DCapture_WriteFogGlobal();
    result = std3DBackend_SetMipmapFilter(mode);
    stdRenderCapture_BeginRecord(STDRENDERCAPTURE_ID_STD3D_SETMIPMAPFILTER);
    stdRenderCapture_PutI32((int32_t)mode);
    stdRenderCapture_PutI32(result);
    stdRenderCapture_EndRecord();
    return result;
}

int J3DAPI std3D_SetProjection(float fov, float nearPlane, float farPlane)
{
    int result;

    if ( !stdRenderCapture_IsOpen() )
    {
        return std3DBackend_SetProjection(fov, nearPlane, farPlane);
    }

    std3DCapture_WriteFogGlobal();
    result = std3DBackend_SetProjection(fov, nearPlane, farPlane);
    stdRenderCapture_BeginRecord(STDRENDERCAPTURE_ID_STD3D_SETPROJECTION);
    stdRenderCapture_PutF32(fov);
    stdRenderCapture_PutF32(nearPlane);
    stdRenderCapture_PutF32(farPlane);
    stdRenderCapture_PutI32(result);
    stdRenderCapture_EndRecord();
    return result;
}

void J3DAPI std3D_EnableFog(int bEnabled, float density)
{
    if ( !stdRenderCapture_IsOpen() )
    {
        std3DBackend_EnableFog(bEnabled, density);
        return;
    }

    std3DCapture_WriteFogGlobal();
    std3DBackend_EnableFog(bEnabled, density);
    stdRenderCapture_BeginRecord(STDRENDERCAPTURE_ID_STD3D_ENABLEFOG);
    stdRenderCapture_PutI32(bEnabled);
    stdRenderCapture_PutF32(density);
    stdRenderCapture_EndRecord();
}

void J3DAPI std3D_SetFog(float red, float green, float blue, float startDepth, float endDepth)
{
    if ( !stdRenderCapture_IsOpen() )
    {
        std3DBackend_SetFog(red, green, blue, startDepth, endDepth);
        return;
    }

    std3DCapture_WriteFogGlobal();
    std3DBackend_SetFog(red, green, blue, startDepth, endDepth);
    stdRenderCapture_BeginRecord(STDRENDERCAPTURE_ID_STD3D_SETFOG);
    stdRenderCapture_PutF32(red);
    stdRenderCapture_PutF32(green);
    stdRenderCapture_PutF32(blue);
    stdRenderCapture_PutF32(startDepth);
    stdRenderCapture_PutF32(endDepth);
    stdRenderCapture_EndRecord();
}

void std3D_ClearZBuffer(void)
{
    if ( !std3DCapture_bFrameRecording() )
    {
        std3DBackend_ClearZBuffer();
        return;
    }

    std3DCapture_WriteFogGlobal();
    std3DBackend_ClearZBuffer();
    stdRenderCapture_BeginRecord(STDRENDERCAPTURE_ID_STD3D_CLEARZBUFFER);
    stdRenderCapture_EndRecord();
}

StdDisplayEnvironment* J3DAPI std3D_BuildDisplayEnvironment()
{
    StdDisplayEnvironment* pResult;

    if ( !stdRenderCapture_IsOpen() )
    {
        return std3DBackend_BuildDisplayEnvironment();
    }

    std3DCapture_WriteFogGlobal();
    pResult = std3DBackend_BuildDisplayEnvironment();
    stdRenderCapture_BeginRecord(STDRENDERCAPTURE_ID_STD3D_BUILDDISPLAYENVIRONMENT);
    stdRenderCapture_PutU8(pResult != NULL ? 1u : 0u);
    stdRenderCapture_EndRecord();
    return pResult;
}

void J3DAPI std3D_FreeDisplayEnvironment(StdDisplayEnvironment* pEnv)
{
    if ( !stdRenderCapture_IsOpen() )
    {
        std3DBackend_FreeDisplayEnvironment(pEnv);
        return;
    }

    std3DCapture_WriteFogGlobal();
    std3DBackend_FreeDisplayEnvironment(pEnv);
    stdRenderCapture_BeginRecord(STDRENDERCAPTURE_ID_STD3D_FREEDISPLAYENVIRONMENT);
    stdRenderCapture_PutU8(pEnv != NULL ? 1u : 0u);
    stdRenderCapture_EndRecord();
}

void std3D_ResetGlobals(void)
{
    if ( !stdRenderCapture_IsOpen() )
    {
        std3DBackend_ResetGlobals();
        return;
    }

    std3DCapture_WriteFogGlobal();
    std3DBackend_ResetGlobals();
    stdRenderCapture_BeginRecord(STDRENDERCAPTURE_ID_STD3D_RESETGLOBALS);
    stdRenderCapture_EndRecord();
}

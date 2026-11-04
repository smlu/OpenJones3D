// Forwarding layer of the render capture (M6.0): one public std3D_ name per renderer
// function, forwarding to the backend. Recording is added later.

#include <std/Win95/std3D.h>
#include <std/Win95/stdRenderBackendApi.h>

int std3D_Startup(void)
{
    return std3DBackend_Startup();
}

void std3D_Shutdown(void)
{
    std3DBackend_Shutdown();
}

const Device3D* std3D_GetCurrentDevice(void)
{
    return std3DBackend_GetCurrentDevice();
}

size_t std3D_GetNumDevices(void)
{
    return std3DBackend_GetNumDevices();
}

const Device3D* std3D_GetAllDevices(void)
{
    return std3DBackend_GetAllDevices();
}

void J3DAPI std3D_SetFindAllDevices(int bFindAll)
{
    std3DBackend_SetFindAllDevices(bFindAll);
}

int J3DAPI std3D_Open(size_t deviceNum)
{
    return std3DBackend_Open(deviceNum);
}

void std3D_Close(void)
{
    std3DBackend_Close();
}

void J3DAPI std3D_GetTextureFormat(StdColorFormatType type, ColorInfo* pDest, int* pbColorKeySet, LPDDCOLORKEY* ppColorKey)
{
    std3DBackend_GetTextureFormat(type, pDest, pbColorKeySet, ppColorKey);
}

StdColorFormatType J3DAPI std3D_GetColorFormat(const ColorInfo* pCi)
{
    return std3DBackend_GetColorFormat(pCi);
}

size_t std3D_GetNumTextureFormats(void)
{
    return std3DBackend_GetNumTextureFormats();
}

int std3D_StartScene(void)
{
    return std3DBackend_StartScene();
}

void std3D_EndScene(void)
{
    std3DBackend_EndScene();
}

void J3DAPI std3D_DrawRenderList(tSysTexture* pTex, Std3DRenderState rdflags, StdVertexTL* aVerts, size_t numVerts, uint16_t* aIndices, size_t numIndices)
{
    std3DBackend_DrawRenderList(pTex, rdflags, aVerts, numVerts, aIndices, numIndices);
}

void J3DAPI std3D_DrawLineStrip(StdVertexTL* aVerts, size_t numVerts)
{
    std3DBackend_DrawLineStrip(aVerts, numVerts);
}

void J3DAPI std3D_DrawPointList(StdVertexTL* aVerts, size_t numVerts)
{
    std3DBackend_DrawPointList(aVerts, numVerts);
}

void J3DAPI std3D_SetRenderState(Std3DRenderState rdflags)
{
    std3DBackend_SetRenderState(rdflags);
}

void std3D_SetWireframeRenderState(void)
{
    std3DBackend_SetWireframeRenderState();
}

void J3DAPI std3D_AllocSystemTexture(tSystemTexture* pTexture, tVBuffer** apVBuffers, size_t numMipLevels, StdColorFormatType formatType)
{
    std3DBackend_AllocSystemTexture(pTexture, apVBuffers, numMipLevels, formatType);
}

void J3DAPI std3D_GetValidDimensions(uint32_t width, uint32_t height, uint32_t* pOutWidth, uint32_t* pOutHeight)
{
    std3DBackend_GetValidDimensions(width, height, pOutWidth, pOutHeight);
}

void J3DAPI std3D_ClearSystemTexture(tSystemTexture* pTex)
{
    std3DBackend_ClearSystemTexture(pTex);
}

void J3DAPI std3D_AddToTextureCache(tSystemTexture* pCacheTexture, StdColorFormatType format)
{
    std3DBackend_AddToTextureCache(pCacheTexture, format);
}

void std3D_ResetTextureCache(void)
{
    std3DBackend_ResetTextureCache();
}

size_t J3DAPI std3D_GetMipMapCount(const tSystemTexture* pTexture)
{
    return std3DBackend_GetMipMapCount(pTexture);
}

void J3DAPI std3D_UpdateFrameCount(tSystemTexture* pTexture)
{
    std3DBackend_UpdateFrameCount(pTexture);
}

size_t J3DAPI std3D_FindClosestFormat(const ColorInfo* pMatch)
{
    return std3DBackend_FindClosestFormat(pMatch);
}

int J3DAPI std3D_SetMipmapFilter(Std3DMipmapFilterType mode)
{
    return std3DBackend_SetMipmapFilter(mode);
}

int J3DAPI std3D_SetProjection(float fov, float nearPlane, float farPlane)
{
    return std3DBackend_SetProjection(fov, nearPlane, farPlane);
}

void J3DAPI std3D_EnableFog(int bEnabled, float density)
{
    std3DBackend_EnableFog(bEnabled, density);
}

void J3DAPI std3D_SetFog(float red, float green, float blue, float startDepth, float endDepth)
{
    std3DBackend_SetFog(red, green, blue, startDepth, endDepth);
}

void std3D_ClearZBuffer(void)
{
    std3DBackend_ClearZBuffer();
}

StdDisplayEnvironment* J3DAPI std3D_BuildDisplayEnvironment()
{
    return std3DBackend_BuildDisplayEnvironment();
}

void J3DAPI std3D_FreeDisplayEnvironment(StdDisplayEnvironment* pEnv)
{
    std3DBackend_FreeDisplayEnvironment(pEnv);
}

void std3D_ResetGlobals(void)
{
    std3DBackend_ResetGlobals();
}

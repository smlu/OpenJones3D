#ifndef STD_STD3D_H
#define STD_STD3D_H
#include <j3dcore/j3d.h>
#include <std/types.h>


// Added following macros from rdCache to use for VBO & IBO sizes
#define STD3D_MAXFACEVERTICES 64
#define STD3D_MAXVERTICES     32768

#define STD3D_CFG_MIPMAPAUTOGEN     "graphics.mipmapAutoGen"
#define STD3D_CFG_ANISOTROPICFILTER "graphics.anisotropicFilter"

#define STD3D_CFG_MSAAENABLED "graphics.msaa.enabled"
#define STD3D_CFG_MSAASAMPLES "graphics.msaa.samples"

J3D_EXTERN_C_START

#define std3D_g_fogDensity J3D_DECL_FAR_VAR(std3D_g_fogDensity, float)
// extern float std3D_g_fogDensity ;

#define std3D_g_maxVertices J3D_DECL_FAR_VAR(std3D_g_maxVertices, size_t)
// extern size_t std3D_g_maxVertices;

int std3D_Startup(void);
void std3D_Shutdown(void);

const Device3D* std3D_GetCurrentDevice(void); // Added

size_t std3D_GetNumDevices(void);
const Device3D* std3D_GetAllDevices(void);
void J3DAPI std3D_SetFindAllDevices(int bFindAll);

int J3DAPI std3D_Open(size_t deviceNum);
void std3D_Close(void);

void J3DAPI std3D_GetTextureFormat(StdColorFormatType type, ColorInfo* pDest, int* pbColorKeySet, LPDDCOLORKEY* ppColorKey);
StdColorFormatType J3DAPI std3D_GetColorFormat(const ColorInfo* pCi);
size_t std3D_GetNumTextureFormats(void);

int std3D_StartScene(void);
void std3D_EndScene(void);


void J3DAPI std3D_SetRenderState(Std3DRenderState rdflags);
void std3D_SetWireframeRenderState(void);

void J3DAPI std3D_AllocSystemTexture(tSystemTexture* pTexture, tVBuffer** apVBuffers, size_t numMipLevels, StdColorFormatType formatType);
void J3DAPI std3D_GetValidDimensions(uint32_t width, uint32_t height, uint32_t* pOutWidth, uint32_t* pOutHeight);
void J3DAPI std3D_ClearSystemTexture(tSystemTexture* pTex);

void J3DAPI std3D_AddToTextureCache(tSystemTexture* pCacheTexture, StdColorFormatType format);
void std3D_ResetTextureCache(void);

size_t J3DAPI std3D_GetMipMapCount(const tSystemTexture* pTexture);
void J3DAPI std3D_UpdateFrameCount(tSystemTexture* pTexture);
size_t J3DAPI std3D_FindClosestFormat(const ColorInfo* pMatch);

// On success 0 is returned
int J3DAPI std3D_SetMipmapFilter(Std3DMipmapFilterType mode);
int J3DAPI std3D_SetProjection(float fov, float nearPlane, float farPlane);
void J3DAPI std3D_EnableFog(int bEnabled, float density);
void J3DAPI std3D_SetFog(float red, float green, float blue, float startDepth, float endDepth);
void std3D_ClearZBuffer(void);

StdDisplayEnvironment* J3DAPI std3D_BuildDisplayEnvironment();
void J3DAPI std3D_FreeDisplayEnvironment(StdDisplayEnvironment* pEnv);

#if defined(J3D_DIRECTX9) || defined(J3D_DIRECTX6) || defined(J3D_OPENGL)
const char* J3DAPI std3D_D3DGetStatus(HRESULT res);
tSysDevice3D* std3D_GetD3DDevice(void); // Added
#endif // J3D_DIRECTX9 || J3D_DIRECTX6

// New funcs
bool std3D_IsShaderSystemActive(void);
bool std3D_IsAnisotropicFilteringSupported(void); // Checks if underlying GAPI supports anisotropic texute filtering. Note, device might still not support it.
bool std3D_IsMipmapAutoGenSupported(void);        // Checks if underlying GAPI supports automatic generation of Mipmaps. Note, device might still not support it.
bool std3D_IsMSAASupported(void);                 // Checks if underlying GAPI supports Multisample anti-aliasing (MSAA). Note, device might still not support it.

// Helper hooking functions
void std3D_InstallHooks(void);
void std3D_ResetGlobals(void);
#ifdef J3D_OPENGL
void std3D_SetCeilingSkyHeight(float height);
void std3D_UpdateHorizonSky(float camPitch, float camYaw, float scale);
void std3D_SetDrawState(std3DDrawState drawState);
std3DDrawState std3D_GetCurrentDrawState(void);
std3DVertexSpace std3D_GetCurrentVertexSpace(void);
void J3DAPI std3D_DrawRenderList(tSysTexture* pTex, Std3DRenderState rdflags, LPD3DTLVERTEX aVerts, size_t numVerts, LPWORD aIndices, size_t numIndices, std3DVertexSpace vs, bool bUseShaderLighting);
void J3DAPI std3D_DrawLineStrip(LPD3DTLVERTEX aVerts, size_t numVerts, std3DVertexSpace vs);
void J3DAPI std3D_DrawPointList(LPD3DTLVERTEX aVerts, size_t numVerts, std3DVertexSpace vs);
#else
void J3DAPI std3D_DrawRenderList(tSysTexture* pTex, Std3DRenderState rdflags, LPD3DTLVERTEX aVerts, size_t numVerts, LPWORD aIndices, size_t numIndices);
void J3DAPI std3D_DrawLineStrip(LPD3DTLVERTEX aVerts, size_t numVerts);
void J3DAPI std3D_DrawPointList(LPD3DTLVERTEX aVerts, size_t numVerts);

#endif

J3D_EXTERN_C_END
#endif // STD_STD3D_H

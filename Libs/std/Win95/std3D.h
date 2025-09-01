#ifndef STD_STD3D_H
#define STD_STD3D_H
#include <j3dcore/j3d.h>
#include <std/types.h>
#include <std/RTI/addresses.h>

#include "stdShader.h"

#include <math.h>

// Added following macros from rdCache to use for VBO & IBO sizes
#define STD3D_MAXFACEVERTICES 64 
#define STD3D_MAXVERTICES     32768

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
void J3DAPI std3D_Close();

void J3DAPI std3D_GetTextureFormat(StdColorFormatType type, ColorInfo* pDest, int* pbColorKeySet, LPDDCOLORKEY* ppColorKey);
StdColorFormatType J3DAPI std3D_GetColorFormat(const ColorInfo* pCi);
size_t std3D_GetNumTextureFormats(void);

int std3D_StartScene(void);
void std3D_EndScene(void);

void J3DAPI std3D_DrawRenderListEx(tSysTexture* pTex, Std3DRenderState rdflags, LPD3DTLVERTEX aVerts, size_t numVerts, LPWORD aIndices, size_t numIndices, StdShaderHandle customShader); // New extended function with custom shader support
void J3DAPI std3D_DrawRenderList(tSysTexture* pTex, Std3DRenderState rdflags, LPD3DTLVERTEX aVerts, size_t numVerts, LPWORD aIndices, size_t numIndices);
void J3DAPI std3D_DrawLineStrip(LPD3DTLVERTEX aVerts, size_t numVerts);
void J3DAPI std3D_DrawPointList(LPD3DTLVERTEX aVerts, size_t numVerts);
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

#if defined(J3D_DIRECTX9) || defined(J3D_DIRECTX6)
const char* J3DAPI std3D_D3DGetStatus(HRESULT res);
tSysDevice3D* std3D_GetD3DDevice(void); // Added
#endif // J3D_DIRECTX9 || J3D_DIRECTX6

// New funcs
bool J3DAPI std3D_IsShaderSystemActive(void);

// Helper hooking functions
void std3D_InstallHooks(void);
void std3D_ResetGlobals(void);



typedef struct {
    float _11, _12, _13, _14;
    float _21, _22, _23, _24;
    float _31, _32, _33, _34;
    float _41, _42, _43, _44;
} Matrix4x4;

#include <std/General/stdMath.h>

inline void MatrixPerspectiveFovLH(Matrix4x4* pOut, float fovY, float aspect, float zn, float zf)
{
    float yScale = 1.0f / tanf(STDMATH_RADIANSF(fovY / 2.0f));
    float xScale = yScale / aspect;

    memset(pOut, 0, sizeof(Matrix4x4)); // Zero out the matrix

    pOut->_11 = xScale;
    pOut->_22 = yScale;
    pOut->_33 = zf / (zf - zn);
    pOut->_34 = 1.0f;
    pOut->_43 = -zn * zf / (zf - zn);

    //pOut->_11 = xScale;
    //pOut->_22 = yScale;
    //pOut->_33 = zf / (zf - zn);
    //pOut->_34 = -zn * zf / (zf - zn);  // Fixed: moved this
    //pOut->_43 = 1.0f;
}

inline void MatrixMultiply4x4(Matrix4x4* pOut, const Matrix4x4* pM1, const Matrix4x4* pM2)
{
    if ( !pOut || !pM1 || !pM2 ) {
        // Handle error: Null pointers provided
        // In a robust library, you might assert or return an error code.
        return;
    }

    // A temporary matrix to store the result before copying to pOut,
    // in case pOut is the same as pM1 or pM2 (in-place operation).
    Matrix4x4 tempResult;

    // Calculate each element of the resulting matrix
    // Row 1
    tempResult._11 = pM1->_11 * pM2->_11 + pM1->_12 * pM2->_21 + pM1->_13 * pM2->_31 + pM1->_14 * pM2->_41;
    tempResult._12 = pM1->_11 * pM2->_12 + pM1->_12 * pM2->_22 + pM1->_13 * pM2->_32 + pM1->_14 * pM2->_42;
    tempResult._13 = pM1->_11 * pM2->_13 + pM1->_12 * pM2->_23 + pM1->_13 * pM2->_33 + pM1->_14 * pM2->_43;
    tempResult._14 = pM1->_11 * pM2->_14 + pM1->_12 * pM2->_24 + pM1->_13 * pM2->_34 + pM1->_14 * pM2->_44;

    // Row 2
    tempResult._21 = pM1->_21 * pM2->_11 + pM1->_22 * pM2->_21 + pM1->_23 * pM2->_31 + pM1->_24 * pM2->_41;
    tempResult._22 = pM1->_21 * pM2->_12 + pM1->_22 * pM2->_22 + pM1->_23 * pM2->_32 + pM1->_24 * pM2->_42;
    tempResult._23 = pM1->_21 * pM2->_13 + pM1->_22 * pM2->_23 + pM1->_23 * pM2->_33 + pM1->_24 * pM2->_43;
    tempResult._24 = pM1->_21 * pM2->_14 + pM1->_22 * pM2->_24 + pM1->_23 * pM2->_34 + pM1->_24 * pM2->_44;

    // Row 3
    tempResult._31 = pM1->_31 * pM2->_11 + pM1->_32 * pM2->_21 + pM1->_33 * pM2->_31 + pM1->_34 * pM2->_41;
    tempResult._32 = pM1->_31 * pM2->_12 + pM1->_32 * pM2->_22 + pM1->_33 * pM2->_32 + pM1->_34 * pM2->_42;
    tempResult._33 = pM1->_31 * pM2->_13 + pM1->_32 * pM2->_23 + pM1->_33 * pM2->_33 + pM1->_34 * pM2->_43;
    tempResult._34 = pM1->_31 * pM2->_14 + pM1->_32 * pM2->_24 + pM1->_33 * pM2->_34 + pM1->_34 * pM2->_44;

    // Row 4
    tempResult._41 = pM1->_41 * pM2->_11 + pM1->_42 * pM2->_21 + pM1->_43 * pM2->_31 + pM1->_44 * pM2->_41;
    tempResult._42 = pM1->_41 * pM2->_12 + pM1->_42 * pM2->_22 + pM1->_43 * pM2->_32 + pM1->_44 * pM2->_42;
    tempResult._43 = pM1->_41 * pM2->_13 + pM1->_42 * pM2->_23 + pM1->_43 * pM2->_33 + pM1->_44 * pM2->_43;
    tempResult._44 = pM1->_41 * pM2->_14 + pM1->_42 * pM2->_24 + pM1->_43 * pM2->_34 + pM1->_44 * pM2->_44;

    // Copy the result to the output matrix
    // If you are in C++, `*pOut = tempResult;` is cleaner and safer.
    memcpy(pOut, &tempResult, sizeof(Matrix4x4));
}

static int MatrixInverse4x4(Matrix4x4* pOut, const Matrix4x4* pM) {
    if ( !pOut || !pM ) {
        return 0;
    }

    float src[16] = {
        pM->_11, pM->_12, pM->_13, pM->_14,
        pM->_21, pM->_22, pM->_23, pM->_24,
        pM->_31, pM->_32, pM->_33, pM->_34,
        pM->_41, pM->_42, pM->_43, pM->_44
    };
    float dst[16];

    // Calculate all cofactors
    dst[0] = src[5] * src[10] * src[15] - src[5] * src[11] * src[14] - src[9] * src[6] * src[15] + src[9] * src[7] * src[14] + src[13] * src[6] * src[11] - src[13] * src[7] * src[10];
    dst[4] = -src[4] * src[10] * src[15] + src[4] * src[11] * src[14] + src[8] * src[6] * src[15] - src[8] * src[7] * src[14] - src[12] * src[6] * src[11] + src[12] * src[7] * src[10];
    dst[8] = src[4] * src[9] * src[15] - src[4] * src[11] * src[13] - src[8] * src[5] * src[15] + src[8] * src[7] * src[13] + src[12] * src[5] * src[11] - src[12] * src[7] * src[9];
    dst[12] = -src[4] * src[9] * src[14] + src[4] * src[10] * src[13] + src[8] * src[5] * src[14] - src[8] * src[6] * src[13] - src[12] * src[5] * src[10] + src[12] * src[6] * src[9];
    dst[1] = -src[1] * src[10] * src[15] + src[1] * src[11] * src[14] + src[9] * src[2] * src[15] - src[9] * src[3] * src[14] - src[13] * src[2] * src[11] + src[13] * src[3] * src[10];
    dst[5] = src[0] * src[10] * src[15] - src[0] * src[11] * src[14] - src[8] * src[2] * src[15] + src[8] * src[3] * src[14] + src[12] * src[2] * src[11] - src[12] * src[3] * src[10];
    dst[9] = -src[0] * src[9] * src[15] + src[0] * src[11] * src[13] + src[8] * src[1] * src[15] - src[8] * src[3] * src[13] - src[12] * src[1] * src[11] + src[12] * src[3] * src[9];
    dst[13] = src[0] * src[9] * src[14] - src[0] * src[10] * src[13] - src[8] * src[1] * src[14] + src[8] * src[2] * src[13] + src[12] * src[1] * src[10] - src[12] * src[2] * src[9];
    dst[2] = src[1] * src[6] * src[15] - src[1] * src[7] * src[14] - src[5] * src[2] * src[15] + src[5] * src[3] * src[14] + src[13] * src[2] * src[7] - src[13] * src[3] * src[6];
    dst[6] = -src[0] * src[6] * src[15] + src[0] * src[7] * src[14] + src[4] * src[2] * src[15] - src[4] * src[3] * src[14] - src[12] * src[2] * src[7] + src[12] * src[3] * src[6];
    dst[10] = src[0] * src[5] * src[15] - src[0] * src[7] * src[13] - src[4] * src[1] * src[15] + src[4] * src[3] * src[13] + src[12] * src[1] * src[7] - src[12] * src[3] * src[5];
    dst[14] = -src[0] * src[5] * src[14] + src[0] * src[6] * src[13] + src[4] * src[1] * src[14] - src[4] * src[2] * src[13] - src[12] * src[1] * src[6] + src[12] * src[2] * src[5];
    dst[3] = -src[1] * src[6] * src[11] + src[1] * src[7] * src[10] + src[5] * src[2] * src[11] - src[5] * src[3] * src[10] - src[9] * src[2] * src[7] + src[9] * src[3] * src[6];
    dst[7] = src[0] * src[6] * src[11] - src[0] * src[7] * src[10] - src[4] * src[2] * src[11] + src[4] * src[3] * src[10] + src[8] * src[2] * src[7] - src[8] * src[3] * src[6];
    dst[11] = -src[0] * src[5] * src[11] + src[0] * src[7] * src[9] + src[4] * src[1] * src[11] - src[4] * src[3] * src[9] - src[8] * src[1] * src[7] + src[8] * src[3] * src[5];
    dst[15] = src[0] * src[5] * src[10] - src[0] * src[6] * src[9] - src[4] * src[1] * src[10] + src[4] * src[2] * src[9] + src[8] * src[1] * src[6] - src[8] * src[2] * src[5];

    // Calculate determinant
    float det = src[0] * dst[0] + src[1] * dst[4] + src[2] * dst[8] + src[3] * dst[12];

    const float EPSILON = 1e-10f;
    if ( fabsf(det) < EPSILON ) {
        return 0; // Singular matrix
    }

    float invDet = 1.0f / det;

    // Apply inverse determinant to all elements
    for ( int i = 0; i < 16; i++ ) {
        ((float*)pOut)[i] = dst[i] * invDet;
    }

    return 1;
}

J3D_EXTERN_C_END
#endif // STD_STD3D_H

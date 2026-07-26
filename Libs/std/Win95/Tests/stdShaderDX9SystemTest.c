#include <unity_fixture.h>

#include <stdio.h>
#include <string.h>

#include <std/General/std.h>
#include <std/General/stdUtil.h>
#include <std/Win95/stdDisplay.h>
#include <std/Win95/DX9/stdShaderDX9.h>
#include <std/Win95/std3D.h>
#include <std/Win95/stdShader.h>

#include <d3dcompiler.h>

#include "std3DSystemTestSupport.h"
#include "stdWin95SystemTestSupport.h"

TEST_GROUP(stdShaderDX9System);

extern LPDIRECT3DDEVICE9 stdShader_pDevice;

static const char stdShaderDX9SystemTest_aVertexShader[] =
"struct VS_IN {"
"    float4 pos : POSITION;"
"    float4 color0 : COLOR0;"
"    float4 color1 : COLOR1;"
"    float2 tex : TEXCOORD0;"
"};"
"struct VS_OUT {"
"    float4 pos : POSITION;"
"    float4 color0 : COLOR0;"
"    float4 color1 : COLOR1;"
"    float2 tex : TEXCOORD0;"
"    float depth : TEXCOORD1;"
"};"
"VS_OUT main(VS_IN input) {"
"    VS_OUT output;"
"    output.pos = input.pos;"
"    output.color0 = input.color0;"
"    output.color1 = input.color1;"
"    output.tex = input.tex;"
"    output.depth = input.pos.z;"
"    return output;"
"}";

static const char stdShaderDX9SystemTest_aPixelShader[] =
"struct PS_IN {"
"    float4 color0 : COLOR0;"
"    float2 tex : TEXCOORD0;"
"    float depth : TEXCOORD1;"
"};"
"float4 main(PS_IN input) : COLOR0 {"
"    return input.color0;"
"}";

static const char stdShaderDX9SystemTest_aConstantPixelShader[] =
"struct PS_IN {"
"    float4 color0 : COLOR0;"
"    float2 tex : TEXCOORD0;"
"    float depth : TEXCOORD1;"
"};"
"float4 uColor : register(c8);"
"float4 main(PS_IN input) : COLOR0 {"
"    return uColor;"
"}";

static const char stdShaderDX9SystemTest_aTexturePixelShader[] =
"struct PS_IN {"
"    float4 color0 : COLOR0;"
"    float2 tex : TEXCOORD0;"
"    float depth : TEXCOORD1;"
"};"
"sampler2D uTexture : register(s0);"
"float4 main(PS_IN input) : COLOR0 {"
"    return tex2D(uTexture, input.tex);"
"}";

TEST_SETUP(stdShaderDX9System)
{
    StdWin95SystemTest_Startup();
}

TEST_TEAR_DOWN(stdShaderDX9System)
{
    StdWin95SystemTest_Shutdown();
}

static void stdShaderDX9SystemTest_RequireOpenShaderSystem(void)
{
    StdWin95SystemTest_RequireHiddenWindow();
    StdWin95SystemTest_RequireWindowedDisplayMode();

    if ( !std3D_Startup() )
    {
        TEST_IGNORE_MESSAGE("Direct3D device enumeration unavailable on this host.");
    }

    StdWin95SystemTest_SetStd3DStarted(true);
    if ( !std3D_Open(0u) )
    {
        TEST_IGNORE_MESSAGE("Direct3D device open unavailable on this host.");
    }

    if ( !std3D_IsShaderSystemActive() )
    {
        TEST_IGNORE_MESSAGE("DX9 shader system inactive on this host.");
    }
}

static void stdShaderDX9SystemTest_SetFloatParam(StdShaderHandle shader, const char* pName, StdShaderType type, float value)
{
    StdShaderParamValue paramValue;

    STD_ZEROMEM(&paramValue, sizeof(paramValue));
    paramValue.type             = STDSHADER_PARAM_FLOAT;
    paramValue.value.floatValue = value;
    TEST_ASSERT_TRUE(stdShader_SetShaderParam(shader, pName, type, &paramValue));
}

static void stdShaderDX9SystemTest_SetVectorParam(StdShaderHandle shader, const char* pName, StdShaderType type, StdShaderParamType paramType, const float* aValues)
{
    char aMessage[128];
    StdShaderParamValue paramValue;

    STD_ZEROMEM(&paramValue, sizeof(paramValue));
    paramValue.type = paramType;
    for ( size_t i = 0u; i < STD_ARRAYLEN(paramValue.value.vector); ++i )
    {
        paramValue.value.vector[i] = aValues[i];
    }

    STD_FORMAT(aMessage, "stdShader_SetShaderParam failed for vector param '%s' type %d.", pName, type);
    TEST_ASSERT_TRUE_MESSAGE(stdShader_SetShaderParam(shader, pName, type, &paramValue), aMessage);
}

static void stdShaderDX9SystemTest_SetMatrixParam(StdShaderHandle shader, const char* pName, StdShaderType type, const StdShaderMatrix matrix)
{
    StdShaderParamValue paramValue;

    STD_ZEROMEM(&paramValue, sizeof(paramValue));
    paramValue.type = STDSHADER_PARAM_MATRIX;
    STD_COPYMEM(paramValue.value.matrix, matrix, sizeof(paramValue.value.matrix));
    TEST_ASSERT_TRUE(stdShader_SetShaderParam(shader, pName, type, &paramValue));
}

static void stdShaderDX9SystemTest_SetTextureParam(StdShaderHandle shader, const char* pName, StdShaderType type, tSysTexture* pTexture)
{
    StdShaderParamValue paramValue;

    STD_ZEROMEM(&paramValue, sizeof(paramValue));
    paramValue.type           = STDSHADER_PARAM_TEXTURE;
    paramValue.value.pTexture = pTexture;
    TEST_ASSERT_TRUE(stdShader_SetShaderParam(shader, pName, type, &paramValue));
}

static void stdShaderDX9SystemTest_AssertConstant4(const float* aExpected, const float* aActual)
{
    for ( size_t i = 0u; i < 4u; ++i )
    {
        TEST_ASSERT_FLOAT_WITHIN(0.0001f, aExpected[i], aActual[i]);
    }
}

static void stdShaderDX9SystemTest_InitClipVertex(D3DTLVERTEX* pVertex, float x, float y, float tu, float tv)
{
    STD_ZEROMEM(pVertex, sizeof(*pVertex));
    pVertex->sx       = x;
    pVertex->sy       = y;
    pVertex->sz       = 0.5f;
    pVertex->rhw      = 1.0f;
    pVertex->color    = 0xFFFFFFFFu;
    pVertex->specular = 0xFF000000u;
    pVertex->tu       = tu;
    pVertex->tv       = tv;
}

static void stdShaderDX9SystemTest_DrawClipTriangle(void)
{
    D3DTLVERTEX aVerts[3];

    stdShaderDX9SystemTest_InitClipVertex(&aVerts[0], -0.7f, -0.7f, 0.0f, 1.0f);
    stdShaderDX9SystemTest_InitClipVertex(&aVerts[1], 0.0f, 0.7f, 0.5f, 0.0f);
    stdShaderDX9SystemTest_InitClipVertex(&aVerts[2], 0.7f, -0.7f, 1.0f, 1.0f);

    TEST_ASSERT_EQUAL_INT(D3D_OK, IDirect3DDevice9_SetRenderState(stdShader_pDevice, D3DRS_CULLMODE, D3DCULL_NONE));
    TEST_ASSERT_EQUAL_INT(0, std3D_StartScene());
    HRESULT hr = IDirect3DDevice9_DrawPrimitiveUP(
        stdShader_pDevice,
        D3DPT_TRIANGLELIST,
        1u,
        aVerts,
        sizeof(D3DTLVERTEX)
    );
    TEST_ASSERT_EQUAL_INT(D3D_OK, hr);
    std3D_EndScene();
}

static IDirect3DTexture9* stdShaderDX9SystemTest_CreateSolidTexture(uint32_t argbColor)
{
    IDirect3DTexture9* pTexture = NULL;
    D3DLOCKED_RECT lockedRect;

    HRESULT hr = IDirect3DDevice9_CreateTexture(
        stdShader_pDevice,
        2u,
        2u,
        1u,
        0u,
        D3DFMT_A8R8G8B8,
        D3DPOOL_MANAGED,
        &pTexture,
        NULL
    );
    TEST_ASSERT_EQUAL_INT(D3D_OK, hr);
    TEST_ASSERT_NOT_NULL(pTexture);

    hr = IDirect3DTexture9_LockRect(pTexture, 0u, &lockedRect, NULL, 0u);
    TEST_ASSERT_EQUAL_INT(D3D_OK, hr);

    for ( uint32_t y = 0u; y < 2u; ++y )
    {
        uint32_t* pRow = (uint32_t*)((uint8_t*)lockedRect.pBits + (size_t)y * (size_t)lockedRect.Pitch);

        pRow[0] = argbColor;
        pRow[1] = argbColor;
    }

    hr = IDirect3DTexture9_UnlockRect(pTexture, 0u);
    TEST_ASSERT_EQUAL_INT(D3D_OK, hr);
    return pTexture;
}

static void stdShaderDX9SystemTest_CompileBytecode(const char* pSource, const char* pProfile, ID3DBlob** ppBlob)
{
    ID3DBlob* pErrorBlob = NULL;
    HRESULT hr = D3DCompile(
        pSource,
        strlen(pSource),
        NULL,
        NULL,
        NULL,
        "main",
        pProfile,
        D3DCOMPILE_OPTIMIZATION_LEVEL3,
        0u,
        ppBlob,
        &pErrorBlob
    );

    if ( pErrorBlob )
    {
        pErrorBlob->lpVtbl->Release(pErrorBlob);
    }

    TEST_ASSERT_EQUAL_INT(D3D_OK, hr);
    TEST_ASSERT_NOT_NULL(*ppBlob);
}

TEST(stdShaderDX9System, TestCompileCreateParamsAndGlobalConstants)
{
    StdShaderHandle shader;
    StdShaderParamValue value;
    StdShaderViewport viewport = { 0.0f, 0.0f, 320.0f, 240.0f };
    StdShaderVector fogColor = { 0.25f, 0.5f, 0.75f, 1.0f };

    stdShaderDX9SystemTest_RequireOpenShaderSystem();

    TEST_ASSERT_EQUAL_size_t(STDSHADER_INVALIDHANDLE, stdShader_CompileAndCreate("bad_shader", "not hlsl", "not hlsl"));

    shader = stdShader_CompileAndCreate(
        "stdShaderDX9SystemTestShader",
        stdShaderDX9SystemTest_aVertexShader,
        stdShaderDX9SystemTest_aPixelShader
    );
    TEST_ASSERT_NOT_EQUAL_size_t(STDSHADER_INVALIDHANDLE, shader);
    TEST_ASSERT_EQUAL_size_t(shader, stdShader_GetShader("stdShaderDX9SystemTestShader"));

    TEST_ASSERT_TRUE(stdShader_SetViewport(viewport));
    TEST_ASSERT_TRUE(stdShader_SetFog(true, 1.0f, 100.0f, 0.25f, fogColor));
    TEST_ASSERT_TRUE(stdShader_DisableFog());
    TEST_ASSERT_TRUE(stdShader_SetActiveShader(shader));

    TEST_ASSERT_FALSE(stdShader_IsShaderDirty(shader));
    TEST_ASSERT_TRUE(stdShader_RegisterShaderParam(shader, "uVertexVector", STDSHADER_TYPE_VERTEX, STDSHADER_PARAM_VECTOR4, 0u));
    TEST_ASSERT_FALSE(stdShader_RegisterShaderParam(shader, "uVertexVector", STDSHADER_TYPE_VERTEX, STDSHADER_PARAM_VECTOR4, 1u));
    TEST_ASSERT_FALSE(stdShader_RegisterShaderParam(shader, "uVertexAlias", STDSHADER_TYPE_VERTEX, STDSHADER_PARAM_VECTOR4, 0u));
    TEST_ASSERT_TRUE(stdShader_RegisterShaderParam(shader, "uPixelFloat", STDSHADER_TYPE_PIXEL, STDSHADER_PARAM_FLOAT, 0u));
    TEST_ASSERT_TRUE(stdShader_RegisterShaderParam(shader, "uPixelRegisterLimit", STDSHADER_TYPE_PIXEL, STDSHADER_PARAM_VECTOR4, STDSHADERDX9_MAX_PS_PARAMS - 1u));
    TEST_ASSERT_FALSE(stdShader_RegisterShaderParam(shader, "uPixelRegisterOverflow", STDSHADER_TYPE_PIXEL, STDSHADER_PARAM_VECTOR4, STDSHADERDX9_MAX_PS_PARAMS));

    value.type = STDSHADER_PARAM_FLOAT;
    value.value.floatValue = 99.0f;
    TEST_ASSERT_FALSE(stdShader_SetShaderParam(shader, "uVertexVector", STDSHADER_TYPE_VERTEX, &value));
    TEST_ASSERT_FALSE(stdShader_IsShaderDirty(shader));

    value.type = STDSHADER_PARAM_VECTOR4;
    value.value.vector[0] = 1.0f;
    value.value.vector[1] = 2.0f;
    value.value.vector[2] = 3.0f;
    value.value.vector[3] = 4.0f;
    TEST_ASSERT_TRUE(stdShader_SetShaderParam(shader, "uVertexVector", STDSHADER_TYPE_VERTEX, &value));
    TEST_ASSERT_TRUE(stdShader_IsShaderDirty(shader));
    TEST_ASSERT_FALSE(stdShader_SetShaderParam(shader, "missing", STDSHADER_TYPE_VERTEX, &value));

    value.type = STDSHADER_PARAM_FLOAT;
    value.value.floatValue = 0.75f;
    TEST_ASSERT_TRUE(stdShader_SetShaderParam(shader, "uPixelFloat", STDSHADER_TYPE_PIXEL, &value));
    TEST_ASSERT_TRUE(stdShader_ApplyShaderParams(shader));
    TEST_ASSERT_FALSE(stdShader_IsShaderDirty(shader));

    stdShader_Free(shader);
    TEST_ASSERT_EQUAL_size_t(STDSHADER_INVALIDHANDLE, stdShader_GetShader("stdShaderDX9SystemTestShader"));
}

TEST(stdShaderDX9System, TestRejectsInvalidHandlesTypesAndRegisterRanges)
{
    StdShaderParamValue value;
    char aLongName[65];

    stdShaderDX9SystemTest_RequireOpenShaderSystem();

    StdShaderHandle shader = stdShader_CompileAndCreate(
        "stdShaderDX9SystemValidation",
        stdShaderDX9SystemTest_aVertexShader,
        stdShaderDX9SystemTest_aPixelShader
    );
    TEST_ASSERT_NOT_EQUAL_size_t(STDSHADER_INVALIDHANDLE, shader);

    memset(aLongName, 'x', sizeof(aLongName) - 1u);
    aLongName[STD_ARRAYLEN(aLongName) - 1u] = '\0';

    TEST_ASSERT_EQUAL_size_t(STDSHADER_INVALIDHANDLE, stdShader_GetShader(NULL));
    TEST_ASSERT_FALSE(stdShader_SetViewport(NULL));
    TEST_ASSERT_FALSE(stdShader_SetFog(false, 0.0f, 1.0f, 1.0f, NULL));
    TEST_ASSERT_FALSE(stdShader_SetActiveShader(STDSHADER_INVALIDHANDLE));
    TEST_ASSERT_FALSE(stdShader_ApplyShaderParams(STDSHADER_INVALIDHANDLE));
    TEST_ASSERT_FALSE(stdShader_IsShaderDirty(STDSHADER_INVALIDHANDLE));

    TEST_ASSERT_FALSE(stdShader_RegisterShaderParam(STDSHADER_INVALIDHANDLE, "invalidHandle", STDSHADER_TYPE_PIXEL, STDSHADER_PARAM_FLOAT, 0u));
    TEST_ASSERT_FALSE(stdShader_RegisterShaderParam(shader, NULL, STDSHADER_TYPE_PIXEL, STDSHADER_PARAM_FLOAT, 0u));
    TEST_ASSERT_FALSE(stdShader_RegisterShaderParam(shader, "", STDSHADER_TYPE_PIXEL, STDSHADER_PARAM_FLOAT, 0u));
    TEST_ASSERT_FALSE(stdShader_RegisterShaderParam(shader, aLongName, STDSHADER_TYPE_PIXEL, STDSHADER_PARAM_FLOAT, 0u));
    TEST_ASSERT_FALSE(stdShader_RegisterShaderParam(shader, "invalidShaderType", (StdShaderType)0, STDSHADER_PARAM_FLOAT, 0u));
    TEST_ASSERT_FALSE(stdShader_RegisterShaderParam(shader, "invalidParamType", STDSHADER_TYPE_PIXEL, (StdShaderParamType)0, 0u));

    TEST_ASSERT_TRUE(stdShader_RegisterShaderParam(shader, "pixelMatrix", STDSHADER_TYPE_PIXEL, STDSHADER_PARAM_MATRIX, 8u));
    TEST_ASSERT_FALSE(stdShader_RegisterShaderParam(shader, "matrixOverlap", STDSHADER_TYPE_PIXEL, STDSHADER_PARAM_FLOAT, 10u));
    TEST_ASSERT_TRUE(stdShader_RegisterShaderParam(shader, "afterMatrix", STDSHADER_TYPE_PIXEL, STDSHADER_PARAM_FLOAT, 12u));
    TEST_ASSERT_FALSE(stdShader_RegisterShaderParam(shader, "matrixOverflow", STDSHADER_TYPE_PIXEL, STDSHADER_PARAM_MATRIX, STDSHADERDX9_MAX_PS_PARAMS - 3u));

    TEST_ASSERT_TRUE(stdShader_RegisterShaderParam(shader, "pixelSamplerLimit", STDSHADER_TYPE_PIXEL, STDSHADER_PARAM_TEXTURE, STDSHADERDX9_MAX_PS_SAMPLERS - 1u));
    TEST_ASSERT_FALSE(stdShader_RegisterShaderParam(shader, "pixelSamplerOverflow", STDSHADER_TYPE_PIXEL, STDSHADER_PARAM_TEXTURE, STDSHADERDX9_MAX_PS_SAMPLERS));
    TEST_ASSERT_TRUE(stdShader_RegisterShaderParam(shader, "vertexSamplerLimit", STDSHADER_TYPE_VERTEX, STDSHADER_PARAM_TEXTURE, STDSHADERDX9_MAX_VS_SAMPLERS - 1u));
    TEST_ASSERT_FALSE(stdShader_RegisterShaderParam(shader, "vertexSamplerOverflow", STDSHADER_TYPE_VERTEX, STDSHADER_PARAM_TEXTURE, STDSHADERDX9_MAX_VS_SAMPLERS));
    TEST_ASSERT_TRUE(stdShader_RegisterShaderParam(shader, "constantAtSamplerIndex", STDSHADER_TYPE_PIXEL, STDSHADER_PARAM_FLOAT, STDSHADERDX9_MAX_PS_SAMPLERS - 1u));

    STD_ZEROMEM(&value, sizeof(value));
    value.type = STDSHADER_PARAM_FLOAT;
    TEST_ASSERT_FALSE(stdShader_SetShaderParam(STDSHADER_INVALIDHANDLE, "afterMatrix", STDSHADER_TYPE_PIXEL, &value));
    TEST_ASSERT_FALSE(stdShader_SetShaderParam(shader, NULL, STDSHADER_TYPE_PIXEL, &value));
    TEST_ASSERT_FALSE(stdShader_SetShaderParam(shader, "afterMatrix", (StdShaderType)0, &value));
    TEST_ASSERT_FALSE(stdShader_SetShaderParam(shader, "afterMatrix", STDSHADER_TYPE_PIXEL, NULL));

    value.type = (StdShaderParamType)0;
    TEST_ASSERT_FALSE(stdShader_SetShaderParam(shader, "afterMatrix", STDSHADER_TYPE_PIXEL, &value));

    stdShader_Free(shader);
    stdShader_Free(shader);

    StdShaderHandle first = stdShader_CompileAndCreate(
        "stdShaderDX9SystemDoubleFreeA",
        stdShaderDX9SystemTest_aVertexShader,
        stdShaderDX9SystemTest_aPixelShader
    );
    StdShaderHandle second = stdShader_CompileAndCreate(
        "stdShaderDX9SystemDoubleFreeB",
        stdShaderDX9SystemTest_aVertexShader,
        stdShaderDX9SystemTest_aPixelShader
    );
    TEST_ASSERT_NOT_EQUAL_size_t(STDSHADER_INVALIDHANDLE, first);
    TEST_ASSERT_NOT_EQUAL_size_t(STDSHADER_INVALIDHANDLE, second);
    TEST_ASSERT_NOT_EQUAL_size_t(first, second);

    stdShader_Free(second);
    stdShader_Free(first);
}

TEST(stdShaderDX9System, TestVertexTextureUsesVertexSamplerNamespace)
{
    stdShaderDX9SystemTest_RequireOpenShaderSystem();

    StdShaderHandle shader = stdShader_CompileAndCreate(
        "stdShaderDX9SystemVertexSampler",
        stdShaderDX9SystemTest_aVertexShader,
        stdShaderDX9SystemTest_aPixelShader
    );
    TEST_ASSERT_NOT_EQUAL_size_t(STDSHADER_INVALIDHANDLE, shader);

    IDirect3DTexture9* pTexture = stdShaderDX9SystemTest_CreateSolidTexture(0xFFFFFFFFu);
    DWORD vertexSampler = D3DVERTEXTEXTURESAMPLER0 + STDSHADERDX9_MAX_VS_SAMPLERS - 1u;
    HRESULT hr = IDirect3DDevice9_SetTexture(stdShader_pDevice, vertexSampler, (IDirect3DBaseTexture9*)pTexture);
    if ( FAILED(hr) )
    {
        stdShader_Free(shader);
        IDirect3DTexture9_Release(pTexture);
        TEST_IGNORE_MESSAGE("This adapter cannot bind the test texture to a vertex sampler.");
    }

    TEST_ASSERT_EQUAL_INT(D3D_OK, IDirect3DDevice9_SetTexture(stdShader_pDevice, STDSHADERDX9_MAX_VS_SAMPLERS - 1u, (IDirect3DBaseTexture9*)pTexture));
    TEST_ASSERT_TRUE(stdShader_RegisterShaderParam(shader, "vertexTexture", STDSHADER_TYPE_VERTEX, STDSHADER_PARAM_TEXTURE, STDSHADERDX9_MAX_VS_SAMPLERS - 1u));
    stdShaderDX9SystemTest_SetTextureParam(shader, "vertexTexture", STDSHADER_TYPE_VERTEX, NULL);
    TEST_ASSERT_TRUE(stdShader_ApplyShaderParams(shader));

    IDirect3DBaseTexture9* pBoundTexture = NULL;
    TEST_ASSERT_EQUAL_INT(D3D_OK, IDirect3DDevice9_GetTexture(stdShader_pDevice, vertexSampler, &pBoundTexture));
    TEST_ASSERT_NULL(pBoundTexture);

    TEST_ASSERT_EQUAL_INT(D3D_OK, IDirect3DDevice9_GetTexture(stdShader_pDevice, STDSHADERDX9_MAX_VS_SAMPLERS - 1u, &pBoundTexture));
    TEST_ASSERT_EQUAL_PTR(pTexture, pBoundTexture);
    IDirect3DBaseTexture9_Release(pBoundTexture);

    TEST_ASSERT_EQUAL_INT(D3D_OK, IDirect3DDevice9_SetTexture(stdShader_pDevice, STDSHADERDX9_MAX_VS_SAMPLERS - 1u, NULL));
    stdShader_Free(shader);
    IDirect3DTexture9_Release(pTexture);
}

TEST(stdShaderDX9System, TestApplyWritesAllParamTypesToDX9Registers)
{
    static const float aVertexVector2[4] = { 2.0f, 3.0f, 0.0f, 0.0f };
    static const float aVertexVector3[4] = { 4.0f, 5.0f, 6.0f, 0.0f };
    static const float aVertexVector4[4] = { 7.0f, 8.0f, 9.0f, 10.0f };
    static const float aPixelVector4[4]  = { 11.0f, 12.0f, 13.0f, 14.0f };
    static const StdShaderMatrix matrix =
    {
        { 1.0f, 2.0f, 3.0f, 4.0f },
        { 5.0f, 6.0f, 7.0f, 8.0f },
        { 9.0f, 10.0f, 11.0f, 12.0f },
        { 13.0f, 14.0f, 15.0f, 16.0f }
    };

    StdShaderHandle shader;

    stdShaderDX9SystemTest_RequireOpenShaderSystem();

    shader = stdShader_CompileAndCreate(
        "stdShaderDX9SystemTestAllParams",
        stdShaderDX9SystemTest_aVertexShader,
        stdShaderDX9SystemTest_aPixelShader
    );
    TEST_ASSERT_NOT_EQUAL_size_t(STDSHADER_INVALIDHANDLE, shader);

    TEST_ASSERT_TRUE(stdShader_RegisterShaderParam(shader, "uVertexFloat", STDSHADER_TYPE_VERTEX, STDSHADER_PARAM_FLOAT, 0u));
    TEST_ASSERT_TRUE(stdShader_RegisterShaderParam(shader, "uVertexVector2", STDSHADER_TYPE_VERTEX, STDSHADER_PARAM_VECTOR2, 1u));
    TEST_ASSERT_TRUE(stdShader_RegisterShaderParam(shader, "uVertexVector3", STDSHADER_TYPE_VERTEX, STDSHADER_PARAM_VECTOR3, 2u));
    TEST_ASSERT_TRUE(stdShader_RegisterShaderParam(shader, "uVertexVector4", STDSHADER_TYPE_VERTEX, STDSHADER_PARAM_VECTOR4, 3u));
    TEST_ASSERT_TRUE(stdShader_RegisterShaderParam(shader, "uVertexMatrix", STDSHADER_TYPE_VERTEX, STDSHADER_PARAM_MATRIX, 4u));
    TEST_ASSERT_TRUE(stdShader_RegisterShaderParam(shader, "uPixelVector4", STDSHADER_TYPE_PIXEL, STDSHADER_PARAM_VECTOR4, 0u));
    TEST_ASSERT_TRUE(stdShader_RegisterShaderParam(shader, "uPixelTexture", STDSHADER_TYPE_PIXEL, STDSHADER_PARAM_TEXTURE, 7u));

    stdShaderDX9SystemTest_SetFloatParam(shader, "uVertexFloat", STDSHADER_TYPE_VERTEX, 1.5f);
    stdShaderDX9SystemTest_SetVectorParam(shader, "uVertexVector2", STDSHADER_TYPE_VERTEX, STDSHADER_PARAM_VECTOR2, aVertexVector2);
    stdShaderDX9SystemTest_SetVectorParam(shader, "uVertexVector3", STDSHADER_TYPE_VERTEX, STDSHADER_PARAM_VECTOR3, aVertexVector3);
    stdShaderDX9SystemTest_SetVectorParam(shader, "uVertexVector4", STDSHADER_TYPE_VERTEX, STDSHADER_PARAM_VECTOR4, aVertexVector4);
    stdShaderDX9SystemTest_SetMatrixParam(shader, "uVertexMatrix", STDSHADER_TYPE_VERTEX, matrix);
    stdShaderDX9SystemTest_SetVectorParam(shader, "uPixelVector4", STDSHADER_TYPE_PIXEL, STDSHADER_PARAM_VECTOR4, aPixelVector4);
    stdShaderDX9SystemTest_SetTextureParam(shader, "uPixelTexture", STDSHADER_TYPE_PIXEL, NULL);

    TEST_ASSERT_TRUE(stdShader_SetActiveShader(shader));
    TEST_ASSERT_TRUE(stdShader_IsShaderDirty(shader));
    TEST_ASSERT_TRUE(stdShader_ApplyShaderParams(shader));
    TEST_ASSERT_FALSE(stdShader_IsShaderDirty(shader));

    float aActual[4];
    float aExpectedFloat[4] = { 1.5f, 0.0f, 0.0f, 0.0f };
    TEST_ASSERT_EQUAL_INT(D3D_OK, IDirect3DDevice9_GetVertexShaderConstantF(stdShader_pDevice, STDSHADERDX9_VS_CONSTANTS_START_REGISTER + 0u, aActual, 1u));
    stdShaderDX9SystemTest_AssertConstant4(aExpectedFloat, aActual);

    TEST_ASSERT_EQUAL_INT(D3D_OK, IDirect3DDevice9_GetVertexShaderConstantF(stdShader_pDevice, STDSHADERDX9_VS_CONSTANTS_START_REGISTER + 1u, aActual, 1u));
    stdShaderDX9SystemTest_AssertConstant4(aVertexVector2, aActual);

    TEST_ASSERT_EQUAL_INT(D3D_OK, IDirect3DDevice9_GetVertexShaderConstantF(stdShader_pDevice, STDSHADERDX9_VS_CONSTANTS_START_REGISTER + 2u, aActual, 1u));
    stdShaderDX9SystemTest_AssertConstant4(aVertexVector3, aActual);

    TEST_ASSERT_EQUAL_INT(D3D_OK, IDirect3DDevice9_GetVertexShaderConstantF(stdShader_pDevice, STDSHADERDX9_VS_CONSTANTS_START_REGISTER + 3u, aActual, 1u));
    stdShaderDX9SystemTest_AssertConstant4(aVertexVector4, aActual);

    for ( size_t row = 0u; row < 4u; ++row )
    {
        TEST_ASSERT_EQUAL_INT(D3D_OK, IDirect3DDevice9_GetVertexShaderConstantF(stdShader_pDevice, STDSHADERDX9_VS_CONSTANTS_START_REGISTER + 4u + (UINT)row, aActual, 1u));
        stdShaderDX9SystemTest_AssertConstant4(matrix[row], aActual);
    }

    TEST_ASSERT_EQUAL_INT(D3D_OK, IDirect3DDevice9_GetPixelShaderConstantF(stdShader_pDevice, STDSHADERDX9_PS_CONSTANTS_START_REGISTER + 0u, aActual, 1u));
    stdShaderDX9SystemTest_AssertConstant4(aPixelVector4, aActual);

    IDirect3DBaseTexture9* pTexture = NULL;
    TEST_ASSERT_EQUAL_INT(D3D_OK, IDirect3DDevice9_GetTexture(stdShader_pDevice, 7u, &pTexture));
    bool bTextureIsNull = pTexture == NULL;
    if ( pTexture )
    {
        IDirect3DBaseTexture9_Release(pTexture);
    }

    TEST_ASSERT_TRUE(bTextureIsNull);

    stdShader_Free(shader);
}

TEST(stdShaderDX9System, TestRenderOutputFromConstantAndTextureParams)
{
    stdShaderDX9SystemTest_RequireOpenShaderSystem();

    StdShaderHandle constantShader = stdShader_CompileAndCreate(
        "stdShaderDX9SystemRenderConstant",
        stdShaderDX9SystemTest_aVertexShader,
        stdShaderDX9SystemTest_aConstantPixelShader
    );
    TEST_ASSERT_NOT_EQUAL_size_t(STDSHADER_INVALIDHANDLE, constantShader);

    static const float aColor[4] = { 0.0f, 1.0f, 1.0f, 1.0f };
    TEST_ASSERT_TRUE(stdShader_RegisterShaderParam(constantShader, "uColor", STDSHADER_TYPE_PIXEL, STDSHADER_PARAM_VECTOR4, 0u));
    stdShaderDX9SystemTest_SetVectorParam(constantShader, "uColor", STDSHADER_TYPE_PIXEL, STDSHADER_PARAM_VECTOR4, aColor);

    TEST_ASSERT_EQUAL_INT(0, stdDisplay_BackBufferFill(0u, NULL));
    std3D_ClearZBuffer();
    TEST_ASSERT_TRUE(stdShader_SetActiveShader(constantShader));
    TEST_ASSERT_TRUE(stdShader_ApplyShaderParams(constantShader));

    stdShaderDX9SystemTest_DrawClipTriangle();
    Std3DSystemTest_AssertBackBufferPixelNear(160u, 120u, 0u, 255u, 255u, 12u);

    StdShaderHandle textureShader = stdShader_CompileAndCreate(
        "stdShaderDX9SystemRenderTexture",
        stdShaderDX9SystemTest_aVertexShader,
        stdShaderDX9SystemTest_aTexturePixelShader
    );
    TEST_ASSERT_NOT_EQUAL_size_t(STDSHADER_INVALIDHANDLE, textureShader);

    IDirect3DTexture9* pTexture = stdShaderDX9SystemTest_CreateSolidTexture(0xFFFF8000u);
    TEST_ASSERT_TRUE(stdShader_RegisterShaderParam(textureShader, "uTexture", STDSHADER_TYPE_PIXEL, STDSHADER_PARAM_TEXTURE, 0u));
    stdShaderDX9SystemTest_SetTextureParam(textureShader, "uTexture", STDSHADER_TYPE_PIXEL, (tSysTexture*)pTexture);

    TEST_ASSERT_EQUAL_INT(D3D_OK, IDirect3DDevice9_SetSamplerState(stdShader_pDevice, 0u, D3DSAMP_MAGFILTER, D3DTEXF_POINT));
    TEST_ASSERT_EQUAL_INT(D3D_OK, IDirect3DDevice9_SetSamplerState(stdShader_pDevice, 0u, D3DSAMP_MINFILTER, D3DTEXF_POINT));
    TEST_ASSERT_EQUAL_INT(0, stdDisplay_BackBufferFill(0u, NULL));

    std3D_ClearZBuffer();
    TEST_ASSERT_TRUE(stdShader_SetActiveShader(textureShader));
    TEST_ASSERT_TRUE(stdShader_ApplyShaderParams(textureShader));
    stdShaderDX9SystemTest_DrawClipTriangle();
    Std3DSystemTest_AssertBackBufferPixelNear(160u, 120u, 255u, 128u, 0u, 12u);

    IDirect3DTexture9_Release(pTexture);
    stdShader_Free(textureShader);
    stdShader_Free(constantShader);
}

TEST(stdShaderDX9System, TestFreeActiveShaderThenApplyAnotherShader)
{
    static const float aFirstColor[4]  = { 1.0f, 0.0f, 1.0f, 1.0f };
    static const float aSecondColor[4] = { 0.0f, 1.0f, 0.5f, 1.0f };

    StdShaderHandle firstShader;
    StdShaderHandle secondShader;

    stdShaderDX9SystemTest_RequireOpenShaderSystem();

    firstShader = stdShader_CompileAndCreate(
        "stdShaderDX9SystemFreeActiveA",
        stdShaderDX9SystemTest_aVertexShader,
        stdShaderDX9SystemTest_aConstantPixelShader
    );
    TEST_ASSERT_NOT_EQUAL_size_t(STDSHADER_INVALIDHANDLE, firstShader);

    secondShader = stdShader_CompileAndCreate(
        "stdShaderDX9SystemFreeActiveB",
        stdShaderDX9SystemTest_aVertexShader,
        stdShaderDX9SystemTest_aConstantPixelShader
    );
    TEST_ASSERT_NOT_EQUAL_size_t(STDSHADER_INVALIDHANDLE, secondShader);

    TEST_ASSERT_TRUE(stdShader_RegisterShaderParam(firstShader, "uColor", STDSHADER_TYPE_PIXEL, STDSHADER_PARAM_VECTOR4, 0u));
    TEST_ASSERT_TRUE(stdShader_RegisterShaderParam(secondShader, "uColor", STDSHADER_TYPE_PIXEL, STDSHADER_PARAM_VECTOR4, 0u));
    stdShaderDX9SystemTest_SetVectorParam(firstShader, "uColor", STDSHADER_TYPE_PIXEL, STDSHADER_PARAM_VECTOR4, aFirstColor);
    stdShaderDX9SystemTest_SetVectorParam(secondShader, "uColor", STDSHADER_TYPE_PIXEL, STDSHADER_PARAM_VECTOR4, aSecondColor);

    TEST_ASSERT_EQUAL_INT(0, stdDisplay_BackBufferFill(0u, NULL));
    std3D_ClearZBuffer();
    TEST_ASSERT_TRUE(stdShader_SetActiveShader(firstShader));
    TEST_ASSERT_TRUE(stdShader_ApplyShaderParams(firstShader));
    stdShaderDX9SystemTest_DrawClipTriangle();
    Std3DSystemTest_AssertBackBufferPixelNear(160u, 120u, 255u, 0u, 255u, 12u);

    stdShader_Free(firstShader);

    TEST_ASSERT_EQUAL_INT(0, stdDisplay_BackBufferFill(0u, NULL));
    std3D_ClearZBuffer();
    TEST_ASSERT_TRUE(stdShader_SetActiveShader(secondShader));
    TEST_ASSERT_TRUE(stdShader_ApplyShaderParams(secondShader));
    stdShaderDX9SystemTest_DrawClipTriangle();
    Std3DSystemTest_AssertBackBufferPixelNear(160u, 120u, 0u, 255u, 128u, 12u);

    stdShader_Free(secondShader);
}

TEST(stdShaderDX9System, TestCreateFromBytecodeHandleExhaustionAndReuse)
{
    ID3DBlob* pVSBlob = NULL;
    ID3DBlob* pPSBlob = NULL;
    StdShaderHandle aShaders[STDSHADERDX9_MAX_SHADERS];
    size_t numCreated = 0u;

    stdShaderDX9SystemTest_RequireOpenShaderSystem();
    STD_ZEROMEM(aShaders, sizeof(aShaders));

    stdShaderDX9SystemTest_CompileBytecode(stdShaderDX9SystemTest_aVertexShader, "vs_3_0", &pVSBlob);
    stdShaderDX9SystemTest_CompileBytecode(stdShaderDX9SystemTest_aPixelShader, "ps_3_0", &pPSBlob);

    StdShaderHandle duplicateSource = stdShader_Create(
        "stdShaderDX9DuplicateName",
        (const uint8_t*)pVSBlob->lpVtbl->GetBufferPointer(pVSBlob),
        (const uint8_t*)pPSBlob->lpVtbl->GetBufferPointer(pPSBlob)
    );
    TEST_ASSERT_NOT_EQUAL_size_t(STDSHADER_INVALIDHANDLE, duplicateSource);
    TEST_ASSERT_EQUAL_size_t(STDSHADER_INVALIDHANDLE, stdShader_Create(
        "stdShaderDX9DuplicateName",
        (const uint8_t*)pVSBlob->lpVtbl->GetBufferPointer(pVSBlob),
        (const uint8_t*)pPSBlob->lpVtbl->GetBufferPointer(pPSBlob)
    ));
    TEST_ASSERT_EQUAL_size_t(duplicateSource, stdShader_GetShader("stdShaderDX9DuplicateName"));
    stdShader_Free(duplicateSource);

    for ( size_t i = 0u; i < STD_ARRAYLEN(aShaders); ++i )
    {
        char aName[64];

        STD_FORMAT(aName, "stdShaderDX9Pool%02zu", i);
        aShaders[i] = stdShader_Create(
            aName,
            (const uint8_t*)pVSBlob->lpVtbl->GetBufferPointer(pVSBlob),
            (const uint8_t*)pPSBlob->lpVtbl->GetBufferPointer(pPSBlob)
        );

        if ( aShaders[i] == STDSHADER_INVALIDHANDLE )
        {
            break;
        }

        ++numCreated;
    }

    TEST_ASSERT_TRUE(numCreated > 0u);
    TEST_ASSERT_EQUAL_size_t(STDSHADER_INVALIDHANDLE, stdShader_Create(
        "stdShaderDX9PoolOverflow",
        (const uint8_t*)pVSBlob->lpVtbl->GetBufferPointer(pVSBlob),
        (const uint8_t*)pPSBlob->lpVtbl->GetBufferPointer(pPSBlob)
    ));

    for ( size_t i = 0u; i < numCreated; ++i )
    {
        stdShader_Free(aShaders[i]);
    }

    StdShaderHandle first = stdShader_Create(
        "stdShaderDX9ReuseA",
        (const uint8_t*)pVSBlob->lpVtbl->GetBufferPointer(pVSBlob),
        (const uint8_t*)pPSBlob->lpVtbl->GetBufferPointer(pPSBlob)
    );

    TEST_ASSERT_NOT_EQUAL_size_t(STDSHADER_INVALIDHANDLE, first);
    stdShader_Free(first);
    TEST_ASSERT_EQUAL_size_t(STDSHADER_INVALIDHANDLE, stdShader_GetShader("stdShaderDX9ReuseA"));

    StdShaderHandle second = stdShader_Create(
        "stdShaderDX9ReuseB",
        (const uint8_t*)pVSBlob->lpVtbl->GetBufferPointer(pVSBlob),
        (const uint8_t*)pPSBlob->lpVtbl->GetBufferPointer(pPSBlob)
    );
    TEST_ASSERT_EQUAL_size_t(first, second);
    stdShader_Free(second);

    pVSBlob->lpVtbl->Release(pVSBlob);
    pPSBlob->lpVtbl->Release(pPSBlob);
}

TEST_GROUP_RUNNER(stdShaderDX9System)
{
    RUN_TEST_CASE(stdShaderDX9System, TestCompileCreateParamsAndGlobalConstants);
    RUN_TEST_CASE(stdShaderDX9System, TestRejectsInvalidHandlesTypesAndRegisterRanges);
    RUN_TEST_CASE(stdShaderDX9System, TestVertexTextureUsesVertexSamplerNamespace);
    RUN_TEST_CASE(stdShaderDX9System, TestApplyWritesAllParamTypesToDX9Registers);
    RUN_TEST_CASE(stdShaderDX9System, TestRenderOutputFromConstantAndTextureParams);
    RUN_TEST_CASE(stdShaderDX9System, TestFreeActiveShaderThenApplyAnotherShader);
    RUN_TEST_CASE(stdShaderDX9System, TestCreateFromBytecodeHandleExhaustionAndReuse);
}

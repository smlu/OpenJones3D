#include "stdShaderDX9.h"

#include <std/General/std.h>
#include <std/General/stdHashtbl.h>
#include <std/General/stdMemory.h>
#include <std/General/stdUtil.h>
#include <std/Win95/std3D.h>

#include <d3d9.h>
#include <D3Dcommon.h>
#include <d3dcompiler.h>
#pragma comment(lib,"d3dcompiler.lib")

#include <stdint.h>

#define STDSHADER_HANDLE_TO_INDEX(handle) ((handle) - 1)
#define STDSHADER_INDEX_TO_HANDLE(index) ((index) + 1)
#define STDSHADER_ISVALIDHANDLE(sh) \
    ((sh) > STDSHADER_INVALIDHANDLE && (sh) < STDSHADER_INDEX_TO_HANDLE(STDSHADERDX9_MAX_SHADERS))

#define STDSHADER_GETREGISTER(type, idx) \
    ((type) == STDSHADER_TYPE_VERTEX ? (STDSHADERDX9_VS_CONSTANTS_START_REGISTER + (idx)) : \
     (type) == STDSHADER_TYPE_PIXEL ? (STDSHADERDX9_PS_CONSTANTS_START_REGISTER + (idx)) : -1) //  TODO: should  indicate error

#define STDSHADER_SETSHADERCONSTANTF(device, type, reg, data, count) \
    ((type) == STDSHADER_TYPE_VERTEX ? IDirect3DDevice9_SetVertexShaderConstantF((device), STDSHADER_GETREGISTER(type, reg), (data), (count)) : \
     (type) == STDSHADER_TYPE_PIXEL ? IDirect3DDevice9_SetPixelShaderConstantF((device), STDSHADER_GETREGISTER(type, reg), (data), (count)) : \
     E_INVALIDARG)

static bool stdShader_bStartup = false;
static bool stdShader_bOpen    = false;

static tHashTable* stdShader_pTable = NULL;
static StdShaderDX9 stdShader_aShaders[STDSHADERDX9_MAX_SHADERS] = { 0 };

static size_t stdShader_numFreeHandles = 0;
static StdShaderHandle stdShader_endHandle = STDSHADER_INVALIDHANDLE;
static StdShaderHandle stdShader_aFreeHandles[STDSHADERDX9_MAX_SHADERS] = { STDSHADER_INVALIDHANDLE };

static size_t stdShader_maxVsParams = 0;

LPDIRECT3DDEVICE9 stdShader_pDevice;

// Vertex declaration for shader pipeline
static D3DVERTEXELEMENT9 std3D_vertexElements[] =
{
    {0, 0,  D3DDECLTYPE_FLOAT4, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0}, // IMPORTANT, position must be set to D3DDECLUSAGE_POSITION so vertex shader is process (D3DDECLUSAGE_POSITIONT will not use vertex shader)
    {0, 16, D3DDECLTYPE_D3DCOLOR, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_COLOR, 0},
    {0, 20, D3DDECLTYPE_D3DCOLOR, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_COLOR, 1},
    {0, 24, D3DDECLTYPE_FLOAT2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 0},
    D3DDECL_END()
};

static bool stdShader_IsShaderTypeValid(StdShaderType type)
{
    return type == STDSHADER_TYPE_VERTEX || type == STDSHADER_TYPE_PIXEL;
}

static bool stdShader_IsParamTypeValid(StdShaderParamType type)
{
    return type >= STDSHADER_PARAM_FLOAT && type <= STDSHADER_PARAM_TEXTURE;
}

static size_t stdShader_GetMaxParams(StdShaderType type)
{
    if ( type == STDSHADER_TYPE_VERTEX )
    {
        return stdShader_maxVsParams;
    }

    if ( type == STDSHADER_TYPE_PIXEL )
    {
        return STDSHADERDX9_MAX_PS_PARAMS;
    }

    return 0u;
}

static size_t stdShader_GetMaxSamplers(StdShaderType type)
{
    if ( type == STDSHADER_TYPE_VERTEX )
    {
        return STDSHADERDX9_MAX_VS_SAMPLERS;
    }

    if ( type == STDSHADER_TYPE_PIXEL )
    {
        return STDSHADERDX9_MAX_PS_SAMPLERS;
    }

    return 0u;
}

static size_t stdShader_GetRegisterCount(StdShaderParamType type)
{
    return type == STDSHADER_PARAM_MATRIX ? 4u : 1u;
}

static bool stdShader_DoRegisterRangesOverlap(size_t firstIndex, size_t firstCount, size_t secondIndex, size_t secondCount)
{
    return firstIndex < secondIndex + secondCount && secondIndex < firstIndex + firstCount;
}

static StdShaderDX9* stdShader_GetShaderPtr(StdShaderHandle sh)
{
    if ( !STDSHADER_ISVALIDHANDLE(sh) )
    {
        return NULL;
    }

    return &stdShader_aShaders[STDSHADER_HANDLE_TO_INDEX(sh)];
}

static bool stdShader_IsShaderInUse(const StdShaderDX9* pShader)
{
    return pShader && (pShader->base.aName[0] || pShader->pVertexShader || pShader->pPixelShader || pShader->pVertexDecl);
}

static bool stdShader_IsShaderReady(const StdShaderDX9* pShader)
{
    return pShader && pShader->base.aName[0] && pShader->pVertexShader && pShader->pPixelShader && pShader->pVertexDecl;
}

static DWORD stdShader_GetTextureSampler(StdShaderType type, size_t registerIndex)
{
    if ( type == STDSHADER_TYPE_VERTEX )
    {
        return D3DVERTEXTEXTURESAMPLER0 + (DWORD)registerIndex;
    }

    return (DWORD)registerIndex;
}

static char* stdShader_DuplicateParamName(const char* pName)
{
    size_t nameSize = strlen(pName) + 1u;
    char* pNameCopy = (char*)STDMALLOC(nameSize);
    if ( !pNameCopy )
    {
        return NULL;
    }

    stdUtil_StringCopy(pNameCopy, nameSize, pName);
    return pNameCopy;
}

static void stdShader_FreeParamTableNames(tHashTable* pParamTable)
{
    if ( !pParamTable || !pParamTable->paNodes )
    {
        return;
    }

    for ( size_t i = 0u; i < pParamTable->numNodes; ++i )
    {
        for ( tLinkListNode* pNode = &pParamTable->paNodes[i]; pNode && pNode->name; pNode = pNode->next )
        {
            STDFREE((void*)pNode->name);
            pNode->name = NULL;
        }
    }
}

void stdShader_ResetShader(StdShaderDX9* pShader)
{
    if ( !pShader )
    {
        STDLOG_ERROR("stdShader_ResetShader called with NULL shader.\n");
        return;
    }

    // Remove shader parameters
    for ( size_t i = 0; i < STDSHADER_MAX_TYPES; i++ )
    {
        StdShaderTypeParams* pTypeParams = &pShader->base.aTypeParams[i];
        if ( pTypeParams->pParamTable )
        {
            // Shader param hash keys are owned copies because the params array can move when it grows.
            stdShader_FreeParamTableNames(pTypeParams->pParamTable);
            stdHashtbl_Free(pTypeParams->pParamTable);
            pTypeParams->pParamTable = NULL;
        }
        if ( pTypeParams->aParams )
        {
            stdMemory_Free(pTypeParams->aParams);
            pTypeParams->aParams   = NULL;
            pTypeParams->numParams = 0;
        }
    }

    // Release Direct3D resources
    if ( pShader->pVertexShader )
    {
        IDirect3DVertexShader9_Release(pShader->pVertexShader);
        pShader->pVertexShader = NULL;
    }

    if ( pShader->pPixelShader )
    {
        IDirect3DPixelShader9_Release(pShader->pPixelShader);
        pShader->pPixelShader = NULL;
    }

    if ( pShader->pVertexDecl )
    {
        IDirect3DVertexDeclaration9_Release(pShader->pVertexDecl);
        pShader->pVertexDecl = NULL;
    }

    // Remove from global shader list & zerout shader
    if ( stdShader_pTable && pShader->base.aName[0] )
    {
        stdHashtbl_Remove(stdShader_pTable, pShader->base.aName);
    }

    memset(pShader, 0, sizeof(StdShaderDX9));
}

void stdShader_ResetAllShaders(void)
{
    for ( size_t i = STD_ARRAYLEN(stdShader_aFreeHandles); i > STDSHADER_INVALIDHANDLE; --i )
    {
        StdShaderDX9* pShader = stdShader_GetShaderPtr(i);

        stdShader_ResetShader(pShader);
        stdShader_aFreeHandles[STDSHADER_HANDLE_TO_INDEX(i)] = STD_ARRAYLEN(stdShader_aFreeHandles) - (i - 1);
    }

    memset(stdShader_aShaders, 0, sizeof(stdShader_aShaders));
    stdShader_numFreeHandles = STD_ARRAYLEN(stdShader_aShaders);
    stdShader_endHandle      = STDSHADER_INVALIDHANDLE;
}

bool J3DAPI stdShader_Startup(void)
{
    if ( stdShader_bStartup )
    {
        STDLOG_WARNING("Shader system already started.\n");
        return true;
    }

    stdShader_pTable = stdHashtbl_New(STDSHADERDX9_MAX_SHADERS);
    if ( !stdShader_pTable )
    {
        STDLOG_ERROR("Failed to allocate memory for shader table.\n");
        return false;
    }

    stdShader_ResetAllShaders();
    stdShader_bStartup = true;
    return true;
}

void stdShader_Shutdown(void)
{
    if ( !stdShader_bStartup )
    {
        STDLOG_WARNING("Shader system not started.\n");
        return;
    }

    if ( stdShader_bOpen )
    {
        stdShader_Close();
    }
    else
    {
        stdShader_ResetAllShaders();
    }

    stdHashtbl_Free(stdShader_pTable);
    stdShader_pTable = NULL;

    stdShader_pDevice     = NULL;
    stdShader_maxVsParams = 0u;
    stdShader_bStartup    = false;
}

bool J3DAPI stdShader_Open(void)
{
    if ( !stdShader_bStartup )
    {
        STDLOG_ERROR("Shader system not started.\n");
        return false;
    }

    if ( stdShader_bOpen )
    {
        STDLOG_WARNING("Shader system already open.\n");
        return true;
    }

    stdShader_pDevice = std3D_GetD3DDevice();
    if ( !stdShader_pDevice )
    {
        STDLOG_ERROR("Failed to get D3D device.\n");
        return false;
    }

    const Device3D* pDevice = std3D_GetCurrentDevice();
    if ( !pDevice )
    {
        STDLOG_ERROR("Failed to get current 3D device info.\n");
        stdShader_pDevice = NULL;
        return false;
    }

    if ( pDevice->d3dDesc.MaxVertexShaderConst <= STDSHADERDX9_VS_CONSTANTS_START_REGISTER )
    {
        STDLOG_ERROR("D3D device exposes too few vertex shader constant registers.\n");
        stdShader_pDevice = NULL;
        return false;
    }

    stdShader_maxVsParams = pDevice->d3dDesc.MaxVertexShaderConst - STDSHADERDX9_VS_CONSTANTS_START_REGISTER; // Typically 256

    stdShader_bOpen = true;
    return true;
}

void stdShader_Close(void)
{
    if ( !stdShader_bOpen )
    {
        STDLOG_WARNING("Shader system not open!\n");
        return;
    }

    stdShader_ResetAllShaders();
    stdShader_pDevice     = NULL;
    stdShader_maxVsParams = 0u;
    stdShader_bOpen       = false;
}

bool J3DAPI stdShader_SetViewport(const StdShaderViewport vp)
{
    if ( !stdShader_bOpen || !stdShader_pDevice || !vp )
    {
        STDLOG_ERROR("Shader system is not open or viewport is invalid.\n");
        return false;
    }

    HRESULT hr = IDirect3DDevice9_SetVertexShaderConstantF(stdShader_pDevice, /*StartRegister=*/STDSHADERDX9_VS_VIEWPORT_REGISTER, vp, 1);
    if ( FAILED(hr) )
    {
        STDLOG_ERROR("Error %s setting shader global viewport!\n", std3D_D3DGetStatus(hr));
        return false;
    }
    return true;
}

bool J3DAPI stdShader_SetFog(bool enable, float start, float end, float depthDactor, const StdShaderVector color)
{
    if ( !stdShader_bOpen || !stdShader_pDevice || !color )
    {
        STDLOG_ERROR("Shader system is not open or fog color is invalid.\n");
        return false;
    }

    float fogParams[4] = { start, end, depthDactor, (float)enable ? 1.0f : 0.0f, };
    HRESULT hr = IDirect3DDevice9_SetPixelShaderConstantF(stdShader_pDevice, /*StartRegister=*/STDSHADERDX9_PS_FOGPARAM_REGISTER, fogParams, 1);
    if ( FAILED(hr) )
    {
        STDLOG_ERROR("Error %s setting shader global fog parameters!\n", std3D_D3DGetStatus(hr));
        return false;
    }

    hr = IDirect3DDevice9_SetPixelShaderConstantF(stdShader_pDevice, /*StartRegister=*/STDSHADERDX9_PS_FOGCOLOR_REGISTER, color, 1);
    if ( FAILED(hr) )
    {
        STDLOG_ERROR("Error %s setting shader global fog color!\n", std3D_D3DGetStatus(hr));
        return false;
    }

    return true;
}

bool stdShader_DisableFog(void)
{
    if ( !stdShader_bOpen || !stdShader_pDevice )
    {
        STDLOG_ERROR("Shader system not open.\n");
        return false;
    }

    float fogParams[4] = { 0 }; // Disable fog
    HRESULT hr = IDirect3DDevice9_SetPixelShaderConstantF(stdShader_pDevice, /*StartRegister=*/STDSHADERDX9_PS_FOGPARAM_REGISTER, fogParams, 1);
    if ( FAILED(hr) )
    {
        STDLOG_ERROR("Error %s disabling shader global fog!\n", std3D_D3DGetStatus(hr));
        return false;
    }

    return true;
}

StdShaderHandle stdShader_GetShader(const char* pName)
{
    if ( !stdShader_bOpen )
    {
        STDLOG_ERROR("Shader system not open.\n");
        return STDSHADER_INVALIDHANDLE;
    }

    if ( !pName || !pName[0] )
    {
        STDLOG_ERROR("Invalid shader name.\n");
        return STDSHADER_INVALIDHANDLE;
    }

    StdShaderHandle sh = (StdShaderHandle)(size_t)stdHashtbl_Find(stdShader_pTable, pName);
    if ( !sh )
    {
        STDLOG_ERROR("Shader '%s' not found.\n", pName);
        return STDSHADER_INVALIDHANDLE;
    }
    return sh;
}

bool J3DAPI stdShader_SetActiveShader(StdShaderHandle sh)
{
    if ( !stdShader_bOpen )
    {
        STDLOG_ERROR("Shader system not open.\n");
        return false;
    }

    StdShaderDX9* pShader = stdShader_GetShaderPtr(sh);
    if ( !stdShader_IsShaderReady(pShader) )
    {
        STDLOG_ERROR("Invalid or uninitialized shader handle %zu.\n", sh);
        return false;
    }

    HRESULT hr = IDirect3DDevice9_SetVertexShader(stdShader_pDevice, pShader->pVertexShader);
    if ( FAILED(hr) )
    {
        STDLOG_ERROR("Failed to set active vertex shader '%s'. Error: %s\n", pShader->base.aName, std3D_D3DGetStatus(hr));
        return false;
    }

    hr = IDirect3DDevice9_SetPixelShader(stdShader_pDevice, pShader->pPixelShader);
    if ( FAILED(hr) )
    {
        STDLOG_ERROR("Failed to set active pixel shader '%s'. Error: %s\n", pShader->base.aName, std3D_D3DGetStatus(hr));
        return false;
    }

    hr = IDirect3DDevice9_SetVertexDeclaration(stdShader_pDevice, pShader->pVertexDecl);
    if ( FAILED(hr) )
    {
        STDLOG_ERROR("Failed to set vertex declaration for shader '%s'. Error: %s\n", pShader->base.aName, std3D_D3DGetStatus(hr));
        return false;
    }

    return true;
}

HRESULT stdShader_CompileShader(const char* source, const char* entryPoint, const char* profile, ID3DBlob** ppBlob)
{
    if ( !source || !entryPoint || !profile || !ppBlob )
    {
        return E_INVALIDARG;
    }

    *ppBlob = NULL;

    ID3DBlob* pErrorBlob = NULL;
    HRESULT hr = D3DCompile(
        source, strlen(source), NULL, NULL, NULL,
        entryPoint, profile,
        D3DCOMPILE_OPTIMIZATION_LEVEL3, 0,
        ppBlob, &pErrorBlob
    );

    if ( pErrorBlob )
    {
        const char* pMessage = (const char*)pErrorBlob->lpVtbl->GetBufferPointer(pErrorBlob);

        if ( FAILED(hr) )
        {
            STDLOG_ERROR("Shader compilation error: %s\n", pMessage);
        }
        else
        {
            STDLOG_WARNING("Shader compilation warning: %s\n", pMessage);
        }

        pErrorBlob->lpVtbl->Release(pErrorBlob);
    }

    return hr;
}

StdShaderHandle stdShader_CompileAndCreate(const char* pName, const char* pVertexShaderCode, const char* pPixelShaderCode)
{
    if ( !stdShader_bOpen )
    {
        STDLOG_ERROR("Shader system not open.\n");
        return STDSHADER_INVALIDHANDLE;
    }

    if ( !pName || !pName[0] || strlen(pName) >= STD_ARRAYLEN(stdShader_aShaders[0].base.aName)
        || !pVertexShaderCode || !pPixelShaderCode )
    {
        STDLOG_ERROR("Invalid shader parameters.\n");
        return STDSHADER_INVALIDHANDLE;
    }

    if ( stdShader_numFreeHandles == 0 )
    {
        STDLOG_ERROR("No free shaders available.\n");
        return STDSHADER_INVALIDHANDLE;
    }

    StdShaderHandle sh = STDSHADER_INVALIDHANDLE;
    ID3DBlob* pVSBlob  = NULL;
    ID3DBlob* pPSBlob  = NULL;

    // Compile vertex shader
    HRESULT hr = stdShader_CompileShader(pVertexShaderCode, "main", "vs_3_0", &pVSBlob);
    if ( FAILED(hr) ) goto cleanup;

    // Compile pixel shader
    hr = stdShader_CompileShader(pPixelShaderCode, "main", "ps_3_0", &pPSBlob);
    if ( FAILED(hr) ) goto cleanup;

    // Create shader
    sh = stdShader_Create(pName, pVSBlob->lpVtbl->GetBufferPointer(pVSBlob), pPSBlob->lpVtbl->GetBufferPointer(pPSBlob));

cleanup:
    if ( pVSBlob ) pVSBlob->lpVtbl->Release(pVSBlob);
    if ( pPSBlob ) pPSBlob->lpVtbl->Release(pPSBlob);
    return sh;
}

StdShaderHandle stdShader_Create(const char* pName, const uint8_t* pCompiledVertexShader, const uint8_t* pCompiledPixelShader)
{
    if ( !stdShader_bOpen )
    {
        STDLOG_ERROR("Shader system not open.\n");
        return STDSHADER_INVALIDHANDLE;
    }

    if ( !pName || !pCompiledVertexShader || !pCompiledPixelShader )
    {
        STDLOG_ERROR("Invalid shader parameters.\n");
        return STDSHADER_INVALIDHANDLE;
    }

    if ( !pName[0] || strlen(pName) >= STD_ARRAYLEN(stdShader_aShaders[0].base.aName) )
    {
        STDLOG_ERROR("Invalid or overlong shader name.\n");
        return STDSHADER_INVALIDHANDLE;
    }

    // Reject duplicate shader names before allocating D3D resources.
    if ( stdHashtbl_Find(stdShader_pTable, pName) )
    {
        STDLOG_ERROR("Shader '%s' already exists.\n", pName);
        return STDSHADER_INVALIDHANDLE;
    }

    if ( stdShader_numFreeHandles == 0 )
    {
        STDLOG_ERROR("No free shaders available.\n");
        return STDSHADER_INVALIDHANDLE;
    }

    StdShaderHandle sh = stdShader_aFreeHandles[--stdShader_numFreeHandles];
    if ( sh > stdShader_endHandle )
    {
        stdShader_endHandle = sh;
    }

    StdShaderDX9* pShader = stdShader_GetShaderPtr(sh);
    STD_STRCPY(pShader->base.aName, pName);

    HRESULT hr = IDirect3DDevice9_CreateVertexShader(stdShader_pDevice, (const DWORD*)pCompiledVertexShader, &pShader->pVertexShader);
    if ( hr != D3D_OK )
    {
        STDLOG_ERROR("Failed to create vertex shader '%s'. Error: %s\n", pName, std3D_D3DGetStatus(hr));
        goto error;
    }

    hr = IDirect3DDevice9_CreatePixelShader(stdShader_pDevice, (const DWORD*)pCompiledPixelShader, &pShader->pPixelShader);
    if ( hr != D3D_OK )
    {
        STDLOG_ERROR("Failed to create pixel shader '%s'. Error: %s\n", pName, std3D_D3DGetStatus(hr));
        goto error;
    }

    hr = IDirect3DDevice9_CreateVertexDeclaration(stdShader_pDevice, std3D_vertexElements, &pShader->pVertexDecl);
    if ( hr != D3D_OK )
    {
        STDLOG_ERROR("Failed to create vertex declaration for shader '%s'. Error: %s\n", pName, std3D_D3DGetStatus(hr));
        goto error;
    }

    // Success
    // Roll back shader creation if the global name table insert fails.
    if ( !stdHashtbl_Add(stdShader_pTable, pShader->base.aName, (void*)sh) )
    {
        STDLOG_ERROR("Failed to add shader '%s' to the shader table.\n", pName);
        goto error;
    }

    return sh;

error:
    stdShader_Free(sh); // Should reset shader and free handle
    return STDSHADER_INVALIDHANDLE;
}

void stdShader_Free(StdShaderHandle sh)
{
    if ( !stdShader_bOpen )
    {
        STDLOG_ERROR("Shader system not open.\n");
        return;
    }

    StdShaderDX9* pShader = stdShader_GetShaderPtr(sh);
    if ( !stdShader_IsShaderInUse(pShader) )
    {
        STDLOG_ERROR("Invalid or unused shader handle %zu.\n", sh);
        return;
    }

    if ( stdShader_numFreeHandles >= STD_ARRAYLEN(stdShader_aFreeHandles) )
    {
        STDLOG_ERROR("Shader free handle table is full.\n");
        return;
    }

    stdShader_ResetShader(pShader);
    stdShader_aFreeHandles[stdShader_numFreeHandles++] = sh;

    // Update last used shader number
    if ( sh == stdShader_endHandle )
    {
        StdShaderHandle newEndHandle = STDSHADER_INVALIDHANDLE;

        for ( StdShaderHandle i = sh - 1; i > STDSHADER_INVALIDHANDLE; --i )
        {
            if ( stdShader_IsShaderInUse(stdShader_GetShaderPtr(i)) )
            {
                newEndHandle = i;
                break;
            }
        }

        stdShader_endHandle = newEndHandle;
    }
}

bool J3DAPI stdShader_RegisterShaderParam(StdShaderHandle sh, const char* pName, StdShaderType type, StdShaderParamType valueType, size_t registerIndex)
{
    if ( !stdShader_bOpen )
    {
        STDLOG_ERROR("Shader system not open.\n");
        return false;
    }

    if ( !pName || !pName[0] || strlen(pName) >= STD_ARRAYLEN(stdShader_aShaders[0].base.aTypeParams[0].aParams[0].aName) )
    {
        STDLOG_ERROR("Invalid or overlong shader parameter name.\n");
        return false;
    }

    if ( !stdShader_IsShaderTypeValid(type) || !stdShader_IsParamTypeValid(valueType) )
    {
        STDLOG_ERROR("Invalid shader or parameter type.\n");
        return false;
    }

    StdShaderDX9* pShader = stdShader_GetShaderPtr(sh);
    if ( !stdShader_IsShaderReady(pShader) )
    {
        STDLOG_ERROR("Invalid or uninitialized shader handle %zu.\n", sh);
        return false;
    }

    StdShaderTypeParams* pTypeParams = &pShader->base.aTypeParams[type];

    const size_t maxParams   = stdShader_GetMaxParams(type);
    const size_t maxSamplers = stdShader_GetMaxSamplers(type);
    const size_t maxEntries  = maxParams + maxSamplers;
    if ( pTypeParams->numParams >= maxEntries )
    {
        STDLOG_ERROR("Failed to register shader parameter '%s' in shader '%s'. Maximum parameter count reached (%zu).\n",
            pName, pShader->base.aName, maxEntries);
        return false;
    }

    const bool bTextureParam   = valueType == STDSHADER_PARAM_TEXTURE;
    const size_t registerCount = stdShader_GetRegisterCount(valueType);
    const size_t registerLimit = bTextureParam ? maxSamplers : maxParams;
    if ( registerCount > registerLimit || registerIndex > registerLimit - registerCount )
    {
        STDLOG_ERROR("Invalid register range %zu..%zu for shader parameter '%s' in shader '%s'. Register limit is %zu.\n",
            registerIndex, registerIndex + registerCount - 1u, pName, pShader->base.aName, registerLimit);
        return false;
    }

    // Check if parameter already exists
    if ( pTypeParams->pParamTable )
    {
        if ( stdHashtbl_Find(pTypeParams->pParamTable, pName) )
        {
            STDLOG_WARNING("Shader parameter '%s' already exists for type %d in shader '%s'.\n", pName, type, pShader->base.aName);
            return false;
        }
    }
    else // If no parameter table exists, create one
    {
        pTypeParams->pParamTable = stdHashtbl_New(maxEntries);
        if ( !pTypeParams->pParamTable )
        {
            STDLOG_ERROR("Failed to allocate memory for shader parameter table in shader '%s'.\n", pShader->base.aName);
            return false;
        }
    }

    // Constant and sampler registers use separate namespaces. Matrices reserve four consecutive constant registers.
    for ( size_t i = 0; i < pTypeParams->numParams; ++i )
    {
        const StdShaderParam* pRegisteredParam = &pTypeParams->aParams[i];
        const bool bRegisteredTexture           = pRegisteredParam->value.type == STDSHADER_PARAM_TEXTURE;
        const size_t registeredCount            = stdShader_GetRegisterCount(pRegisteredParam->value.type);

        if ( bTextureParam == bRegisteredTexture
            && stdShader_DoRegisterRangesOverlap(registerIndex, registerCount, pRegisteredParam->registerIndex, registeredCount) )
        {
            STDLOG_ERROR("Register range for parameter '%s' overlaps parameter '%s' of type %d in shader '%s'.\n",
                pName, pRegisteredParam->aName, type, pShader->base.aName);
            return false;
        }
    }

    // Keep existing shader params intact if growing the array fails.
    StdShaderParam* aParams = (StdShaderParam*)STDREALLOC(pTypeParams->aParams, (pTypeParams->numParams + 1) * sizeof(StdShaderParam));
    if ( !aParams )
    {
        STDLOG_ERROR("Failed to allocate memory for shader parameters in shader '%s'.\n", pShader->base.aName);
        return false;
    }

    pTypeParams->aParams = aParams;

    StdShaderParam* pParam = &pTypeParams->aParams[pTypeParams->numParams];
    memset(pParam, 0, sizeof(StdShaderParam)); // Initialize new parameter

    STD_STRCPY(pParam->aName, pName);
    pParam->registerIndex = registerIndex;
    pParam->value.type    = valueType;

    // Use a stable copied hash key instead of pParam->aName, which can move when aParams is reallocated.
    char* pParamName = stdShader_DuplicateParamName(pName);
    if ( !pParamName )
    {
        STDLOG_ERROR("Failed to allocate memory for shader parameter name '%s' in shader '%s'.\n", pName, pShader->base.aName);
        memset(pParam, 0, sizeof(StdShaderParam));
        return false;
    }

    // Store 1-based parameter indices instead of pointers into the reallocating params array.
    if ( !stdHashtbl_Add(pTypeParams->pParamTable, pParamName, (void*)(uintptr_t)(pTypeParams->numParams + 1u)) )
    {
        STDLOG_ERROR("Failed to add shader parameter '%s' to lookup table in shader '%s'.\n", pName, pShader->base.aName);
        STDFREE(pParamName);
        memset(pParam, 0, sizeof(StdShaderParam));
        return false;
    }

    pTypeParams->numParams++;

    return true;
}

bool J3DAPI stdShader_SetShaderParam(StdShaderHandle sh, const char* pName, StdShaderType type, const StdShaderParamValue* pValue)
{
    if ( !stdShader_bOpen )
    {
        STDLOG_ERROR("Shader system not open.\n");
        return false;
    }

    if ( !pName || !pName[0] || !pValue || !stdShader_IsShaderTypeValid(type) || !stdShader_IsParamTypeValid(pValue->type) )
    {
        STDLOG_ERROR("Invalid shader parameter arguments.\n");
        return false;
    }

    StdShaderDX9* pShader = stdShader_GetShaderPtr(sh);
    if ( !stdShader_IsShaderReady(pShader) )
    {
        STDLOG_ERROR("Invalid or uninitialized shader handle %zu.\n", sh);
        return false;
    }

    StdShaderTypeParams* pTypeParams = &pShader->base.aTypeParams[type];
    if ( !pTypeParams->pParamTable || !pTypeParams->aParams )
    {
        STDLOG_ERROR("Shader parameter '%s' for shader type %d not found in shader '%s'.\n", pName, type, pShader->base.aName);
        return false;
    }

    // Setting an existing shader parameter must not fail when the registered parameter table is full.
    // Shader param lookups are stored as 1-based indices so they remain valid after params array reallocations.
    size_t paramIndex = (size_t)(uintptr_t)stdHashtbl_Find(pTypeParams->pParamTable, pName);
    if ( !paramIndex || paramIndex > pTypeParams->numParams )
    {
        STDLOG_ERROR("Shader parameter '%s' for shader type %d not found in shader '%s'.\n", pName, type, pShader->base.aName);
        return false;
    }

    StdShaderParam* pParam = &pTypeParams->aParams[paramIndex - 1u];

    if ( pValue->type != pParam->value.type )
    {
        STDLOG_ERROR("Type mismatch for shader parameter '%s' for shader type %d in shader '%s'. Expected type %d, got type %d.\n",
            pName, type, pShader->base.aName, pParam->value.type, pValue->type);
        return false;
    }

    pParam->value        = *pValue;
    pShader->base.bDirty = true;

    return true;
}

bool J3DAPI stdShader_ApplyShaderParams(StdShaderHandle sh)
{
    if ( !stdShader_bOpen )
    {
        STDLOG_ERROR("Shader system not open.\n");
        return false;
    }

    StdShaderDX9* pShader = stdShader_GetShaderPtr(sh);
    if ( !stdShader_IsShaderReady(pShader) )
    {
        STDLOG_ERROR("Invalid or uninitialized shader handle %zu.\n", sh);
        return false;
    }

    for ( StdShaderType type = STDSHADER_TYPE_VERTEX; type < STDSHADER_MAX_TYPES; ++type )
    {
        StdShaderTypeParams* pTypeParams = &pShader->base.aTypeParams[type];
        if ( pTypeParams->numParams > 0 )
        {
            for ( size_t i = 0; i < pTypeParams->numParams; i++ )
            {
                StdShaderParam* pParam = &pTypeParams->aParams[i];
                HRESULT hr = D3D_OK;
                switch ( pParam->value.type )
                {
                    case STDSHADER_PARAM_FLOAT:
                        hr = STDSHADER_SETSHADERCONSTANTF(stdShader_pDevice, type, pParam->registerIndex, &pParam->value.value.floatValue, 1);
                        break;
                    case STDSHADER_PARAM_VECTOR2:
                    case STDSHADER_PARAM_VECTOR3:
                    case STDSHADER_PARAM_VECTOR4:
                        hr = STDSHADER_SETSHADERCONSTANTF(stdShader_pDevice, type, pParam->registerIndex, pParam->value.value.vector, 1);
                        break;
                    case STDSHADER_PARAM_MATRIX:
                        hr = STDSHADER_SETSHADERCONSTANTF(stdShader_pDevice, type, pParam->registerIndex, (const float*)pParam->value.value.matrix, 4);
                        break;
                    case STDSHADER_PARAM_TEXTURE:
                    {
                        DWORD sampler = stdShader_GetTextureSampler(type, pParam->registerIndex);

                        hr = IDirect3DDevice9_SetTexture(stdShader_pDevice, sampler, (IDirect3DBaseTexture9*)pParam->value.value.pTexture);
                        break;
                    }
                    default:
                        hr = E_INVALIDARG;
                        break;
                }

                if ( FAILED(hr) )
                {
                    STDLOG_ERROR("Failed to set shader parameter '%s' for shader type %d in shader '%s'. Error: %s\n",
                        pParam->aName, type, pShader->base.aName, std3D_D3DGetStatus(hr));
                    return false;
                }
            }
        }
    }

    pShader->base.bDirty = false;
    return true;
}

bool J3DAPI stdShader_IsShaderDirty(StdShaderHandle sh)
{
    StdShaderDX9* pShader = stdShader_GetShaderPtr(sh);
    if ( !stdShader_bOpen || !stdShader_IsShaderReady(pShader) )
    {
        STDLOG_ERROR("Invalid or uninitialized shader handle %zu.\n", sh);
        return false;
    }

    return pShader->base.bDirty;
}

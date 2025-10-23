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

inline size_t stdShader_GetMaxParams(StdShaderType type)
{
    return (type == STDSHADER_TYPE_VERTEX) ? stdShader_maxVsParams : STDSHADERDX9_MAX_PS_PARAMS;
}

inline StdShaderDX9* stdShader_GetShaderPtr(StdShaderHandle sh)
{
    STD_ASSERT(STDSHADER_ISVALIDHANDLE(sh)); //Only in debug
    return &stdShader_aShaders[STDSHADER_HANDLE_TO_INDEX(sh)];
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
    stdHashtbl_Remove(stdShader_pTable, pShader->base.aName);
    memset(pShader, 0, sizeof(StdShaderDX9));
}

void stdShader_ResetAllShaders(void)
{
    for ( size_t i = STD_ARRAYLEN(stdShader_aFreeHandles); i > STDSHADER_INVALIDHANDLE; --i )
    {
        stdShader_ResetShader(stdShader_GetShaderPtr(i));
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

    stdShader_ResetAllShaders();
    stdHashtbl_Free(stdShader_pTable);
    stdShader_pTable = NULL;

    stdShader_bStartup = false;
}

bool J3DAPI stdShader_Open()
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
    stdShader_bOpen = false;
}

bool J3DAPI stdShader_SetViewport(const StdShaderViewport vp)
{
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
    STD_ASSERT(pName); //Only in debug
    if ( !stdShader_bOpen )
    {
        STDLOG_ERROR("Shader system not open.\n");
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
    STD_ASSERT(pShader->pVertexShader); //Only in debug, check if shader is initialized

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
    ID3DBlob* pErrorBlob = NULL;
    HRESULT hr = D3DCompile(
        source, strlen(source), NULL, NULL, NULL,
        entryPoint, profile,
        D3DCOMPILE_OPTIMIZATION_LEVEL3, 0,
        ppBlob, &pErrorBlob
    );

    if ( FAILED(hr) && pErrorBlob )
    {
        STDLOG_ERROR("Shader compilation error: %s\n", (char*)pErrorBlob->lpVtbl->GetBufferPointer(pErrorBlob));
        pErrorBlob->lpVtbl->Release(pErrorBlob);
    }

    return hr;
}

StdShaderHandle stdShader_CompileAndCreate(const char* pName, const char* pVertexShaderCode, const char* pPixelShaderCode)
{
    if ( !pName || !pVertexShaderCode || !pPixelShaderCode )
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
    stdHashtbl_Add(stdShader_pTable, pShader->base.aName, (void*)sh);
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

    stdShader_ResetShader(stdShader_GetShaderPtr(sh));
    stdShader_aFreeHandles[stdShader_numFreeHandles++] = sh;

    // Update last used shader number
    if ( sh == stdShader_endHandle )
    {
        for ( StdShaderHandle i = sh - 1; i > STDSHADER_INVALIDHANDLE; --i )
        {
            if ( stdShader_GetShaderPtr(i)->pVertexShader )
            {
                stdShader_endHandle = i;
                break;
            }
        }
    }
}

bool J3DAPI stdShader_RegisterShaderParam(StdShaderHandle sh, const char* pName, StdShaderType type, StdShaderParamType valueType, size_t registerIndex)
{
    // TODO: Missing check for max texture sampler parameters

    STD_ASSERT(pName);

    if ( !stdShader_bOpen )
    {
        STDLOG_ERROR("Shader system not open.\n");
        return false;
    }

    StdShaderDX9* pShader = stdShader_GetShaderPtr(sh);
    STD_ASSERT(pShader->pVertexShader); //Only in debug, check if shader is initialized

    StdShaderTypeParams* pTypeParams = &pShader->base.aTypeParams[type];

    const size_t maxParams = stdShader_GetMaxParams(type);
    if ( pTypeParams->numParams >= maxParams )
    {
        STDLOG_ERROR("Failed to register shader parameter '%s' in shader '%s'. Maximum parameter count reached (%d).\n",
            pName, pShader->base.aName, maxParams);
        return false;
    }

    if ( registerIndex >= maxParams )
    {
        STDLOG_ERROR("Invalid register index %zu for shader parameter '%s' in shader '%s'. Maximum index is %d.\n",
            registerIndex, pName, pShader->base.aName, maxParams - 1);
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
        pTypeParams->pParamTable = stdHashtbl_New(maxParams);
        if ( !pTypeParams->pParamTable )
        {
            STDLOG_ERROR("Failed to allocate memory for shader parameter table in shader '%s'.\n", pShader->base.aName);
            return false;
        }
    }

    // Check if index has already been used
    for ( size_t i = 0; i < pTypeParams->numParams; i++ )
    {
        if ( pTypeParams->aParams[i].registerIndex == registerIndex )
        {
            STDLOG_ERROR("Register index %zu already used for parameter '%s' of type %d in shader '%s'.\n",
                registerIndex, pTypeParams->aParams[i].aName, type, pShader->base.aName);
            return false;
        }
    }

    pTypeParams->aParams = (StdShaderParam*)STDREALLOC(pTypeParams->aParams, (pTypeParams->numParams + 1) * sizeof(StdShaderParam));
    if ( !pTypeParams->aParams )
    {
        STDLOG_ERROR("Failed to allocate memory for shader parameters in shader '%s'.\n", pShader->base.aName);
        return false;
    }

    StdShaderParam* pParam = &pTypeParams->aParams[pTypeParams->numParams];
    memset(pParam, 0, sizeof(StdShaderParam)); // Initialize new parameter

    STD_STRCPY(pParam->aName, pName);
    pParam->registerIndex = registerIndex;
    pParam->value.type    = valueType;

    stdHashtbl_Add(pTypeParams->pParamTable, pName, (void*)pParam);
    pTypeParams->numParams++;

    return true;
}

bool J3DAPI stdShader_SetShaderParam(StdShaderHandle sh, const char* pName, StdShaderType type, const StdShaderParamValue* pValue)
{
    STD_ASSERT(pName);

    StdShaderDX9* pShader = stdShader_GetShaderPtr(sh);
    STD_ASSERT(pShader->pVertexShader); //Only in debug, check if shader is initialized

    StdShaderTypeParams* pTypeParams = &pShader->base.aTypeParams[type];

    const size_t maxParams =stdShader_GetMaxParams(type);
    if ( pTypeParams->numParams >= maxParams )
    {
        STDLOG_ERROR("Shader '%s' has reached maximum parameter count (%d).\n", pShader->base.aName, maxParams);
        return false;
    }

    // Find existing parameter or create new one
    StdShaderParam* pParam = (StdShaderParam*)stdHashtbl_Find(pTypeParams->pParamTable, pName);
    if ( !pParam )
    {
        STDLOG_ERROR("Shader parameter '%s' for shader type %d not found in shader '%s'.\n", pName, type, pShader->base.aName);
        return false;
    }

    if ( pValue->type != pParam->value.type )
    {
        // Type mismatch, log error
        STDLOG_ERROR("Type mismatch for shader parameter '%s' for shader type %d in shader '%s'. Expected type %d, got type %d.\n",
            pName, type, pShader->base.aName, pParam->value.type, pValue->type);
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
    STD_ASSERT(pShader->pVertexShader); //Only in debug, check if shader is initialized

    for ( size_t type = 0; type < STDSHADER_MAX_TYPES; type++ )
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
                        STDSHADER_SETSHADERCONSTANTF(stdShader_pDevice, type, pParam->registerIndex, (const float*)pParam->value.value.matrix, 4);
                        break;
                    case STDSHADER_PARAM_TEXTURE:
                        hr = IDirect3DDevice9_SetTexture(stdShader_pDevice, pParam->registerIndex, (IDirect3DBaseTexture9*)pParam->value.value.pTexture);
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
    STD_ASSERT(pShader->pVertexShader); //Only in debug, check if shader is initialized
    return pShader->base.bDirty;
}

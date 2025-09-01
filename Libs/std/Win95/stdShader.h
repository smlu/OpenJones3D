#ifndef STD_SHADER_H
#define STD_SHADER_H
// This is new module for shader management

#include <j3dcore/j3d.h>

#include <std/types.h>
#include <std/General/stdHashtbl.h>

J3D_EXTERN_C_START

typedef float StdShaderVector[4];
typedef StdShaderVector StdShaderViewport;

typedef float StdShaderMatrix[4][4];

typedef size_t StdShaderHandle;

#define STDSHADER_INVALIDHANDLE ((StdShaderHandle)0)

// Shader types
typedef enum eStdShaderType
{
    STDSHADER_TYPE_VERTEX = 1,
    STDSHADER_TYPE_PIXEL  = 2,
    STDSHADER_MAX_TYPES
} StdShaderType;

// Shader parameter types
typedef enum eStdShaderParamType
{
    STDSHADER_PARAM_FLOAT   = 1,
    STDSHADER_PARAM_VECTOR2 = 2,
    STDSHADER_PARAM_VECTOR3 = 3,
    STDSHADER_PARAM_VECTOR4 = 4,
    STDSHADER_PARAM_MATRIX  = 5,
    STDSHADER_PARAM_TEXTURE = 6
} StdShaderParamType;

typedef struct sStdShaderParamValue
{
    StdShaderParamType type;
    union
    {
        float floatValue;
        StdShaderVector vector;
        StdShaderMatrix matrix;
        tSysTexture* pTexture;
    } value;
} StdShaderParamValue;

// Shader parameter structure
typedef struct sStdShaderParam
{
    char aName[64];
    size_t registerIndex;
    StdShaderParamValue value;
} StdShaderParam;

typedef struct sStdShaderTypeParams
{
    StdShaderParam* aParams;
    size_t numParams;
    tHashTable* pParamTable; // Hash table for fast parameter lookup
} StdShaderTypeParams;

// Shader handle
typedef struct sStdShader
{
    char aName[64];
    StdShaderTypeParams aTypeParams[STDSHADER_MAX_TYPES]; // Array of parameters for vertex and pixel shaders
    bool bDirty;
} StdShader;

// Light structure for shader system (simplified version of rdLight)

typedef enum eStdShaderLightType
{
    STDSHADER_LIGHT_DIRECTIONAL = 0,
    STDSHADER_LIGHT_POINT       = 1,
    STDSHADER_LIGHT_SPOT        = 2,
    STDSHADER_LIGHTTYPE_COUNT
} StdShaderLightType;

typedef struct sStdShaderLight
{
    StdShaderLightType type;    // Light type as float for shader compatibility
    bool bEnabled;
    float minRadius;            // Max light range
    float maxRadius;            // Max light intensity before falloff
    StdShaderVector direction;  // Light direction (normalized)
    StdShaderVector color;      // RGBA color, note missing ambient, specular components
    StdShaderVector position;   // World position for point lights
} StdShaderLight;

#define STDSHADER_MAX_LIGHTS 1

// Multi-pass render target types
typedef enum eStdShaderRenderTarget
{
    STDSHADER_RT_ALBEDO   = 0,  // Base color/texture
    STDSHADER_RT_LIGHTING = 1,  // Lighting pass
    STDSHADER_RT_EFFECTS  = 2,  // Special effects (waves, etc.)
    STDSHADER_RT_FINAL    = 3,  // Final composite
    STDSHADER_RT_COUNT
} StdShaderRenderTarget;


bool J3DAPI stdShader_Startup(void);
void stdShader_Shutdown(void);

bool J3DAPI stdShader_Open(void);
void stdShader_Close(void);

// Global constants
bool J3DAPI stdShader_SetWorldMatrix(const StdShaderMatrix mat);
bool J3DAPI stdShader_SetViewport(const StdShaderViewport vp);
bool J3DAPI stdShader_SetViewMatrix(const StdShaderMatrix mat);
bool J3DAPI stdShader_SetViewPosition(const StdShaderVector pos);
bool J3DAPI stdShader_SetViewProjectMatrix(const StdShaderMatrix mat);
bool J3DAPI stdShader_SetInvViewProjectMatrix(const StdShaderMatrix mat);
bool J3DAPI stdShader_AddLight(const StdShaderLight* pLight);

bool J3DAPI stdShader_SetFog(bool enable, float start, float end, float depthDactor, const StdShaderVector color);
bool stdShader_DisableFog(void);

// Shader op
StdShaderHandle stdShader_GetShader(const char* pName); // Get shader handle by name
bool J3DAPI stdShader_SetActiveShader(StdShaderHandle sh); // Apply shader to device

StdShaderHandle J3DAPI stdShader_CompileAndCreate(const char* pName, const char* pVertexShaderCode, const char* pPixelShaderCode);     // Compile shader from source code and create new shader
StdShaderHandle J3DAPI stdShader_Create(const char* pName, const uint8_t* pCompiledVertexShader, const uint8_t* pCompiledPixelShader); // Create shader from compiled shader code
void J3DAPI stdShader_Free(StdShaderHandle sh);

bool J3DAPI stdShader_RegisterShaderParam(StdShaderHandle sh, const char* pName, StdShaderType type, StdShaderParamType valueType, size_t registerIndex); // Register shader parameter
bool J3DAPI stdShader_SetShaderParam(StdShaderHandle sh, const char* pName, StdShaderType type, const StdShaderParamValue* pValue); // Set registered shader parameter value
bool J3DAPI stdShader_ApplyShaderParams(StdShaderHandle sh); // Apply all registered shader parameters to device
bool J3DAPI stdShader_IsShaderDirty(StdShaderHandle sh); // Check if shader has dirty parameters that need to be applied

J3D_EXTERN_C_END
#endif // STD_STDSHADER_H
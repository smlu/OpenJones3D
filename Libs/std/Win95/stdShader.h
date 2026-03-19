#ifndef STD_SHADER_H
#define STD_SHADER_H
// This is new module for shader management

#include <j3dcore/j3d.h>

#include <std/types.h>
#include <std/General/stdHashtbl.h>

#include "rdroid/types.h"

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


bool J3DAPI stdShader_Startup(void);
void stdShader_Shutdown(void);

bool J3DAPI stdShader_Open(void);
void stdShader_Close(void);

// Global constants
bool J3DAPI stdShader_SetViewport(const StdShaderViewport vp);
bool J3DAPI stdShader_SetFog(bool enable, float start, float end, float depthFactor, const StdShaderVector color);
bool stdShader_DisableFog(void);

// Shader op
#if defined(J3D_OPENGL)
bool J3DAPI stdShader_SetActiveShader(GLShaderProgram* pSp); // Apply shader to device
GLShaderProgram* J3DAPI stdShader_CompileAndCreate(const char* pName, const char* pVertexShaderCode, const char* pPixelShaderCode);
void J3DAPI stdShader_Free(GLShaderProgram* sh);
void stdShader_SetTexture(GLShaderProgram* sh, GLuint tex);
void stdShader_SetActiveTextureUnit(GLTextureUnit unit);
void stdShader_UpdateGlobalUniforms(void);
void stdShader_SetShaderLights(void);
GLShaderProgram* stdShader_GetShader(const char* pName); // Get shader handle by name
void stdShader_ConvertToMat4(const rdMatrix34* pMat, float out[16]);
void stdShader_UpdateFadeFactor(void);
#else
bool J3DAPI stdShader_SetActiveShader(StdShaderHandle sh);                                                                         // Apply shader to device
StdShaderHandle J3DAPI stdShader_CompileAndCreate(const char* pName, const char* pVertexShaderCode, const char* pPixelShaderCode); // Compile shader from source code and create new shader
void J3DAPI stdShader_Free(StdShaderHandle sh);
StdShaderHandle stdShader_GetShader(const char* pName); // Get shader handle by name
#endif
StdShaderHandle J3DAPI stdShader_Create(const char* pName, const uint8_t* pCompiledVertexShader, const uint8_t* pCompiledPixelShader); // Create shader from compiled shader code

bool J3DAPI stdShader_RegisterShaderParam(StdShaderHandle sh, const char* pName, StdShaderType type, StdShaderParamType valueType, size_t registerIndex); // Register shader parameter
bool J3DAPI stdShader_SetShaderParam(StdShaderHandle sh, const char* pName, StdShaderType type, const StdShaderParamValue* pValue); // Set registered shader parameter value
bool J3DAPI stdShader_ApplyShaderParams(StdShaderHandle sh); // Apply all registered shader parameters to device
bool J3DAPI stdShader_IsShaderDirty(StdShaderHandle sh); // Check if shader has dirty parameters that need to be applied

J3D_EXTERN_C_END
#endif // STD_STDSHADER_H

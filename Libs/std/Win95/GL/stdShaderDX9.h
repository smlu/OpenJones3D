#ifndef STD_SHADER_DX9_H
#define STD_SHADER_DX9_H
// This is new module for shader management

#include <j3dcore/j3d.h>
#include <std/Win95/stdShader.h>
#include <std/types.h>

#define STDSHADERDX9_MAX_SHADERS           64

// For following register offsets see common.hlsli
#define STDSHADERDX9_VS_VIEWPORT_REGISTER         0
#define STDSHADERDX9_VS_CONSTANTS_START_REGISTER 24 // all custom shader constants starts from this 

// For following register offsets see common.hlsli
#define STDSHADERDX9_PS_FOGPARAM_REGISTER         0
#define STDSHADERDX9_PS_FOGCOLOR_REGISTER         1

#define STDSHADERDX9_PS_CONSTANTS_START_REGISTER 8
#define STDSHADERDX9_MAX_PS_PARAMS               224 - STDSHADERDX9_PS_CONSTANTS_START_REGISTER // 224 - default max constant registers in D3D9


J3D_EXTERN_C_START
typedef struct sStdShaderDX9
{
    StdShader base; // Base shader structure; do not change order of this structure as it is used in stdShader.h

    IDirect3DVertexShader9* pVertexShader;
    IDirect3DPixelShader9* pPixelShader;
    IDirect3DVertexDeclaration9* pVertexDecl;
} StdShaderDX9;


J3D_EXTERN_C_END
#endif // STD_STDSHADERDX9_H
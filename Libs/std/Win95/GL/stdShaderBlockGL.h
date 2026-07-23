#ifndef STD_SHADER_BLOCK_GL_H
#define STD_SHADER_BLOCK_GL_H

#include <j3dcore/j3d.h>
#include <std/types.h>


J3D_EXTERN_C_START

#define STDSHADERBLOCK_COUNT 5
#define MAX_SHADER_LIGHTS 64

typedef enum eStdShaderBlockId
{
    STDSHADERBLOCK_CAMERA     = 0,
    STDSHADERBLOCK_VIEWPORT   = 1,
    STDSHADERBLOCK_LIGHTS     = 2,
    STDSHADERBLOCK_FADEFACTOR = 3,
    STDSHADERBLOCK_FOGDATA    = 4
} StdShaderBlockId;

typedef struct sStdShaderBlockDesc
{
    StdShaderBlockId id;
    const char* glslName;
    GLuint bindingPoint;
    size_t size;
    const void* initialData;
} StdShaderBlockDesc;

typedef struct sStdShaderBlockCamera
{
    float view[16];
    float inverseView[16];
    float projection[16];
    float inverseProjection[16];
    float viewProjection[16];
    float nearPlane;
    float farPlane;
    float focalLength;
    float time;
} StdShaderBlockCamera;

static const StdShaderBlockCamera stdShaderBlock_defaultCameraBlock = {
    .view = {
        1, 0, 0, 0,
        0, 1, 0, 0,
        0, 0, 1, 0,
        0, 0, 0, 1
    },
    .inverseView = {
        1, 0, 0, 0,
        0, 1, 0, 0,
        0, 0, 1, 0,
        0, 0, 0, 1
    },
    .projection = {
        1, 0, 0, 0,
        0, 1, 0, 0,
        0, 0, 1, 0,
        0, 0, 0, 1
    },
    .inverseProjection = {
        1, 0, 0, 0,
        0, 1, 0, 0,
        0, 0, 1, 0,
        0, 0, 0, 1
    },
    .viewProjection = {
        1, 0, 0, 0,
        0, 1, 0, 0,
        0, 0, 1, 0,
        0, 0, 0, 1
    },
    .nearPlane = 0.0f,
    .farPlane = 0.0f,
    .focalLength = 1.0f,
    .time = 0.0f
};

typedef struct sStdShaderBlockPointLight
{
    float position[4]; // xyz + pad
    float color[4];    // rgba
    float minRadius;
    float maxRadius;
    float pad[2]; // std140 padding
} StdShaderBlockPointLight;

typedef struct sStdShaderBlockLights
{
    StdShaderBlockPointLight lights[MAX_SHADER_LIGHTS];
    int lightCount;
    int pad[3]; // std140 padding auf 16 Byte
} StdShaderBlockLights;

typedef struct sStdShaderBlockFadeFactor
{
    float fadeFactor;
    float _pad[3];
} StdShaderBlockFadeFactor;

typedef struct sStdShaderBlockFogData
{
    float bEnabled;
    float start;
    float end;
    float depth;
    float color[3];
    float _pad;
} StdShaderBlockFogData;

static const StdShaderBlockDesc stdShaderBlock_g_aBlockDesc[STDSHADERBLOCK_COUNT] = {
    [STDSHADERBLOCK_CAMERA] = {
        .id = STDSHADERBLOCK_CAMERA,
        .glslName = "CameraData",
        .bindingPoint = 0,
        .size = sizeof(StdShaderBlockCamera),
        .initialData = &stdShaderBlock_defaultCameraBlock
    },
    [STDSHADERBLOCK_VIEWPORT] = {
        .id = STDSHADERBLOCK_VIEWPORT,
        .glslName = "ViewportData",
        .bindingPoint = 1,
        .size = 4 * sizeof(float),
        .initialData = NULL
    },
    [STDSHADERBLOCK_LIGHTS] = {
        .id = STDSHADERBLOCK_LIGHTS,
        .glslName = "PointLightsData",
        .bindingPoint = 2,
        .size = sizeof(StdShaderBlockLights),
        .initialData = NULL
    },
    [STDSHADERBLOCK_FADEFACTOR] = {
        .id = STDSHADERBLOCK_FADEFACTOR,
        .glslName = "FadeFactorData",
        .bindingPoint = 3,
        .size = sizeof(StdShaderBlockFadeFactor),
        .initialData = NULL
    },
    [STDSHADERBLOCK_FOGDATA] = {
        .id = STDSHADERBLOCK_FOGDATA,
        .glslName = "FogData",
        .bindingPoint = 4,
        .size = sizeof(StdShaderBlockFogData),
        .initialData = NULL
    }
};

J3D_EXTERN_C_END
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <rdroid/types.h>
#include <rdroid/Engine/rdCamera.h>
#include <sith/Gameplay/sithTime.h>


#include <std/General/std.h>
#include <std/General/stdHashtbl.h>
#include <std/General/stdMemory.h>
#include <std/Win95/stdShader.h>
#include <std/Win95/GL/Shaders/stdGLSLShaders.h>

#include "std/Win95/std3D.h"
#include "std/Win95/stdDisplay.h"

#define MAX_SHADER_PROGRAMS 64

static bool stdShader_bStartup = false;
static bool stdShader_bOpen    = false;

static tHashTable* stdShader_pTable = NULL;

static size_t stdShader_maxVsParams = 0;


static GLShaderProgram stdShader_ShaderPrograms[MAX_SHADER_PROGRAMS];
static size_t stdShader_shaderCount = 0;

static GLTextureUnit stdShader_activeTextureUnit = 0;

static GLShaderProgram* stdShader_activeShader = NULL;

typedef struct sCameraDataGPU
{
    float view[16];
    float inverseView[16];
    float projection[16];
    float inverseProjection[16];
    float viewProjection[16];
    float farPlane;
    float nearPlane;
    float focalLength;
    float time; // vec4
} CameraDataGPU;

static CameraDataGPU cameraData = { 0 };
static GLuint cameraDataUBO     = 0;
static GLuint viewPortUBO       = 0;

static void stdShader_MulMat4(const float a[16], const float b[16], float out[16])
{
    float r[16];

    for ( int col = 0; col < 4; ++col )
    {
        for ( int row = 0; row < 4; ++row )
        {
            r[col * 4 + row] =
                a[0 * 4 + row] * b[col * 4 + 0] +
                a[1 * 4 + row] * b[col * 4 + 1] +
                a[2 * 4 + row] * b[col * 4 + 2] +
                a[3 * 4 + row] * b[col * 4 + 3];
        }
    }

    memcpy(out, r, sizeof(float) * 16);
}

static void stdShader_ConvertToMat4(const rdMatrix34* pMat, float out[16])
{
    // In JonesEngine z is up and +y id forward
    // So for OpenGL Y <-> Z and Z <-> -Y
    out[0] = pMat->rvec.x;
    out[1] = pMat->rvec.z;
    out[2] = -pMat->rvec.y;
    out[3] = 0.0f;

    // up vector
    out[4] = pMat->uvec.x;
    out[5] = pMat->uvec.z;
    out[6] = -pMat->uvec.y;
    out[7] = 0.0f;

    // forward vector
    out[8]  = -pMat->lvec.x;
    out[9]  = -pMat->lvec.z;
    out[10] = pMat->lvec.y;
    out[11] = 0.0f;

    // position
    out[12] = pMat->dvec.x;
    out[13] = pMat->dvec.z;
    out[14] = -pMat->dvec.y;
    out[15] = 1.0f;
}

void stdShader_ResetShader(GLShaderProgram* shaderProgram)
{
    if ( shaderProgram->handle > 0 )
    {
        if ( stdShader_activeShader == shaderProgram )
        {
            glUseProgram(0);
            stdShader_activeShader = NULL;
        }
        glDeleteProgram(shaderProgram->handle);
        shaderProgram->handle = 0;
        stdShader_shaderCount--;
    }
}

void stdShader_ResetAllShaders(void)
{
    for ( size_t i = 0; i < MAX_SHADER_PROGRAMS; i++ )
    {
        stdShader_ResetShader(&stdShader_ShaderPrograms[i]);
    }
}

static void stdShader_InitUniformBuffers(void)
{
    const float unitMatrix[16] = {
        1.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f
    };
    memcpy(&cameraData.view, unitMatrix, sizeof(unitMatrix));
    memcpy(&cameraData.inverseView, unitMatrix, sizeof(unitMatrix));
    memcpy(&cameraData.projection, unitMatrix, sizeof(unitMatrix));
    memcpy(&cameraData.inverseProjection, unitMatrix, sizeof(unitMatrix));
    memcpy(&cameraData.viewProjection, unitMatrix, sizeof(unitMatrix));
    memcpy(&cameraData.view, unitMatrix, sizeof(unitMatrix));
    cameraData.nearPlane   = 0;
    cameraData.farPlane    = 0;
    cameraData.focalLength = 5.0f;
    cameraData.time        = 0;
    glGenBuffers(1, &cameraDataUBO);
    glBindBuffer(GL_UNIFORM_BUFFER, cameraDataUBO);
    glBufferData(GL_UNIFORM_BUFFER, sizeof(CameraDataGPU), &cameraData, GL_DYNAMIC_DRAW);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);
    glBindBufferBase(GL_UNIFORM_BUFFER, 0, cameraDataUBO);

    glGenBuffers(1, &viewPortUBO);
    glBindBuffer(GL_UNIFORM_BUFFER, viewPortUBO);
    glBufferData(GL_UNIFORM_BUFFER, sizeof(StdShaderViewport), NULL, GL_DYNAMIC_DRAW);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);
    glBindBufferBase(GL_UNIFORM_BUFFER, 1, viewPortUBO);
}

bool J3DAPI stdShader_Startup(void)
{
    if ( stdShader_bStartup )
    {
        STDLOG_WARNING("Shader system already started.\n");
        return true;
    }

    stdShader_pTable = stdHashtbl_New(64);
    if ( !stdShader_pTable )
    {
        STDLOG_ERROR("Failed to allocate memory for shader table.\n");
        return false;
    }

    memset(&cameraData, 0, sizeof(CameraDataGPU));
    stdShader_ResetAllShaders();
    stdShader_bStartup = true;
    stdShader_InitUniformBuffers();
    return true;
}

void stdShader_Shutdown(void)
{
    if ( !stdShader_bStartup )
    {
        STDLOG_WARNING("Shader system not started.\n");
        return;
    }

    memset(&cameraData, 0, sizeof(CameraDataGPU));
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

    stdShader_maxVsParams = 256; // Typically 256

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
    glBindBuffer(GL_UNIFORM_BUFFER, viewPortUBO);
    glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(StdShaderViewport), vp);
    // for ( size_t i = 0; i < MAX_SHADER_PROGRAMS; i++ )
    // {
    //     GLShaderProgram* shaderProgram = &stdShader_ShaderPrograms[i];
    //     if ( shaderProgram->handle > 0 )
    //     {
    //         glUseProgram(shaderProgram->handle);
    //         int loc = glGetUniformLocation(shaderProgram->handle, "viewPort");
    //
    //         if ( loc == -1 )
    //         {
    //             continue;
    //         }
    //         glUniform4f(loc, vp[0], vp[1], vp[2], vp[3]);
    //     }
    // }
    return true;
}

bool J3DAPI stdShader_SetFog(bool enable, float start, float end, float depthDactor, const StdShaderVector color)
{
    glUniform4f(glGetUniformLocation(stdShader_activeShader->handle, "vFogParams"), start, end, depthDactor, enable ? 1.0f : 0.0f);
    glUniform3f(glGetUniformLocation(stdShader_activeShader->handle, "vFogColor"), color[0], color[1], color[2]);

    return true;
}

bool stdShader_DisableFog(void)
{
    float fogParams[4] = { 0 }; // Disable fog
    glUniform4fv(glGetUniformLocation(stdShader_activeShader->handle, "vFogParams"), 1, fogParams);

    return true;
}

GLShaderProgram* stdShader_GetGLShader(const char* pName)
{
    STD_ASSERT(pName); //Only in debug
    if ( !stdShader_bOpen )
    {
        STDLOG_ERROR("Shader system not open.\n");
        return NULL;
    }

    GLShaderProgram* pProgram = stdHashtbl_Find(stdShader_pTable, pName);
    if ( !pProgram )
    {
        STDLOG_ERROR("Shader '%s' not found.\n", pName);
        return NULL;
    }
    return pProgram;
}

bool J3DAPI stdShader_SetActiveShader(GLShaderProgram* pSp)
{
    if ( !stdShader_bOpen )
    {
        STDLOG_ERROR("Shader system not open.\n");
        return false;
    }
    if ( pSp == NULL )
    {
        stdShader_activeShader = NULL;
        glUseProgram(0);
        return true;
    }
    if ( stdShader_activeShader == pSp )
    {
        return true;
    }

    stdShader_activeShader = pSp;
    glUseProgram(pSp->handle);

    return true;
}

static char* stdShader_readGLSLFile(const char* path)
{
    FILE* f = fopen(path, "rb");
    if ( !f )
    {
        STDLOG_ERROR("read_text_file: can't open %s\n", path);
        return NULL;
    }
    fseek(f, 0, SEEK_END);
    long len = ftell(f);
    rewind(f);

    char* buf = STDMALLOC(len + 1);
    if ( !buf )
    {
        fclose(f);
        return NULL;
    }

    size_t rd = fread(buf, 1, len, f);
    fclose(f);

    buf[rd] = '\0'; // null-terminate
    return buf;
}

static GLuint stdShader_compileShader(GLenum type, const char* source, size_t shaderSize, const char* debugName)
{
    GLuint sh = glCreateShader(type);
    glShaderSource(sh, 1, &source, (const GLint*)&shaderSize);
    glCompileShader(sh);

    GLint ok = GL_FALSE;
    glGetShaderiv(sh, GL_COMPILE_STATUS, &ok);
    if ( !ok )
    {
        GLint logLen = 0;
        glGetShaderiv(sh, GL_INFO_LOG_LENGTH, &logLen);
        char* log = STDMALLOC(logLen > 1 ? logLen : 1);
        if ( logLen > 1 )
        {
            glGetShaderInfoLog(sh, logLen, NULL, log);
        }
        else
        {
            log[0] = '\0';
        }

        STDLOG_ERROR("[GLSL] Compile error in %s:\n%s\n", debugName ? debugName : "(shader)", log);
        STDFREE(log);
        glDeleteShader(sh);
        return 0;
    }

    return sh;
}

static GLuint stdShader_LinkShaderProgram(GLuint vs, GLuint fs)
{
    GLuint prog = glCreateProgram();
    glAttachShader(prog, vs);
    glAttachShader(prog, fs);
    glLinkProgram(prog);

    GLint ok = GL_FALSE;
    glGetProgramiv(prog, GL_LINK_STATUS, &ok);
    if ( !ok )
    {
        GLint logLen = 0;
        glGetProgramiv(prog, GL_INFO_LOG_LENGTH, &logLen);
        char* log = STDMALLOC(logLen > 1 ? logLen : 1);
        if ( logLen > 1 )
        {
            glGetProgramInfoLog(prog, logLen, NULL, log);
        }
        else
        {
            log[0] = '\0';
        }

        STDLOG_ERROR("[GLSL] Link error:\n%s\n", log);
        STDFREE(log);
        glDeleteProgram(prog);
        return 0;
    }

    // Shader können nach erfolgreichem Link gelöscht werden
    glDetachShader(prog, vs);
    glDetachShader(prog, fs);
    glDeleteShader(vs);
    glDeleteShader(fs);

    return prog;
}

GLShaderProgram* stdShader_CompileAndCreate(const char* pName, const char* pVertexShaderCode, const char* pPixelShaderCode)
{
    if ( !pName || !pVertexShaderCode || !pPixelShaderCode )
    {
        STDLOG_ERROR("Invalid shader parameters.\n");
        return NULL;
    }

    if ( stdShader_shaderCount >= MAX_SHADER_PROGRAMS )
    {
        STDLOG_ERROR("No free shaders available.\n");
        return NULL;
    }

    GLShaderProgram* pProgram = NULL;
    for ( size_t i = 0; i < MAX_SHADER_PROGRAMS; i++ )
    {
        if ( stdShader_ShaderPrograms[i].handle == 0 )
        {
            pProgram = &stdShader_ShaderPrograms[i];
            break;
        }
    }

    if ( !pProgram )
    {
        STDLOG_ERROR("There should be a free shader available, but couldn't find one.\n");
        return NULL;
    }

    size_t vsSize;
    const char* vsrc = stdGLSLShaders_GetShader(pVertexShaderCode, &vsSize);
    size_t fsSize;
    const char* fsrc = stdGLSLShaders_GetShader(pPixelShaderCode, &fsSize);

    // char* vsrc = stdShader_readGLSLFile(pVertexShaderCode);
    // char* fsrc = stdShader_readGLSLFile(pPixelShaderCode);
    if ( !vsrc || !fsrc )
    {
        if ( vsrc )
        {
            STDFREE(vsrc);
        }

        if ( fsrc )
        {
            STDFREE(fsrc);
        }

        return NULL;
    }

    GLuint vs = stdShader_compileShader(GL_VERTEX_SHADER, vsrc, vsSize, pPixelShaderCode);
    GLuint fs = stdShader_compileShader(GL_FRAGMENT_SHADER, fsrc, fsSize, pPixelShaderCode);

    // STDFREE(vsrc);
    // STDFREE(fsrc);

    if ( !vs || !fs )
    {
        if ( vs )
        {
            glDeleteShader(vs);
        }

        if ( fs )
        {
            glDeleteShader(fs);
        }

        return NULL;
    }

    // Create shader
    pProgram->handle = stdShader_LinkShaderProgram(vs, fs);
    pProgram->name   = pName;
    stdHashtbl_Add(stdShader_pTable, pName, pProgram);
    stdShader_shaderCount++;

    GLuint blockIndex = glGetUniformBlockIndex(pProgram->handle, "CameraData");
    if ( blockIndex != GL_INVALID_INDEX )
    {
        glUniformBlockBinding(pProgram->handle, blockIndex, 0);
    }

    blockIndex = glGetUniformBlockIndex(pProgram->handle, "ViewportData");
    if ( blockIndex != GL_INVALID_INDEX )
    {
        glUniformBlockBinding(pProgram->handle, blockIndex, 1);
    }
    return pProgram;
}

void J3DAPI stdShader_Free(GLShaderProgram* sh)
{
    stdShader_ResetShader(sh);
}

void stdShader_SetTexture(GLShaderProgram* sh, GLuint tex)
{
    if ( stdShader_activeTextureUnit != TU_3D_DRAW ) //other units are reserved for framebuffer textures and must not be changed
    {
        STDLOG_ERROR("Assigning texture is not allowed on this unit");
        return;
    }
    GLShaderProgram* currentProgram = stdShader_activeShader;
    if ( currentProgram != sh )
    {
        stdShader_SetActiveShader(sh);
    }
    glBindTexture(GL_TEXTURE_2D, tex);
    glUniform1i(glGetUniformLocation(stdShader_activeShader->handle, "sTexture"), stdShader_activeTextureUnit);
    stdShader_SetActiveShader(currentProgram);
}

void stdShader_SetActiveTextureUnit(const GLTextureUnit unit)
{
    glActiveTexture(GL_TEXTURE0 + unit);
    stdShader_activeTextureUnit = unit;
}

static void stdShader_SetProjection(float out[16])
{
    rdClipFrustum* pFrustum = rdCamera_g_pCurCamera->pFrustum;
    if ( !pFrustum )
        return;

    float n = pFrustum->nearPlane;
    float f = pFrustum->farPlane;

    if ( fabsf(f - n) < 1e-4f )
        return;

    float hwidth  = (rdCamera_g_pCurCamera->pCanvas->rect.right - rdCamera_g_pCurCamera->pCanvas->rect.left) / 2.0f;
    float hheight = (rdCamera_g_pCurCamera->pCanvas->rect.bottom - rdCamera_g_pCurCamera->pCanvas->rect.top) / 2.0f;

    float fx = rdCamera_g_pCurCamera->focalLength / hwidth;
    float fy = rdCamera_g_pCurCamera->focalLength / hheight;

    float proj[16] = {
        fx, 0, 0, 0,
        0, fy, 0, 0,
        0, 0, (f + n) / (n - f), -1,
        0, 0, (2.0f * f * n) / (n - f), 0
    };

    memcpy(out, proj, sizeof(proj));
}

static void stdShader_SetInverseProjection(float out[16])
{
    rdClipFrustum* pFrustum = rdCamera_g_pCurCamera->pFrustum;
    if ( !pFrustum )
        return;

    float n = pFrustum->nearPlane;
    float f = pFrustum->farPlane;

    if ( fabsf(f - n) < 1e-4f )
        return;

    float hwidth  = (rdCamera_g_pCurCamera->pCanvas->rect.right - rdCamera_g_pCurCamera->pCanvas->rect.left) / 2.0f;
    float hheight = (rdCamera_g_pCurCamera->pCanvas->rect.bottom - rdCamera_g_pCurCamera->pCanvas->rect.top) / 2.0f;

    float fx = rdCamera_g_pCurCamera->focalLength / hwidth;
    float fy = rdCamera_g_pCurCamera->focalLength / hheight;

    float invProj[16] = {
        1.0f / fx, 0, 0, 0,
        0, 1.0f / fy, 0, 0,
        0, 0, 0, (n - f) / (2.0f * f * n),
        0, 0, -1, (f + n) / (2.0f * f * n)
    };

    memcpy(out, invProj, sizeof(invProj));
}


void stdShader_UpdateGlobalUniforms(void)
{
    rdCamera* cam = rdCamera_g_pCurCamera;

    if ( cam == NULL )
    {
        return;
    }
    rdMatrix34* view = &cam->viewMatrix;
    stdShader_ConvertToMat4(view, cameraData.view);
    stdShader_ConvertToMat4(&rdCamera_g_camMatrix, cameraData.inverseView);
    stdShader_SetProjection(cameraData.projection);
    stdShader_SetInverseProjection(cameraData.inverseProjection);
    cameraData.nearPlane   = cam->pFrustum->nearPlane;
    cameraData.farPlane    = cam->pFrustum->farPlane;
    cameraData.focalLength = cam->focalLength * cam->aspectRatio;
    cameraData.time += sithTime_g_frameTimeFlex;

    glBindBuffer(GL_UNIFORM_BUFFER, cameraDataUBO);
    glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(CameraDataGPU), &cameraData);
}

GLShaderProgram* stdShader_GetShader(const char* pName)
{
    STD_ASSERT(pName); //Only in debug
    if ( !stdShader_bOpen )
    {
        STDLOG_ERROR("Shader system not open.\n");
        return NULL;
    }

    GLShaderProgram* pShader = stdHashtbl_Find(stdShader_pTable, pName);
    if ( !pShader )
    {
        STDLOG_ERROR("Shader '%s' not found.\n", pName);
        return NULL;
    }
    return pShader;
}

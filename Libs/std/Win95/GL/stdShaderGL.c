#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#include <std/General/std.h>
#include <std/General/stdHashtbl.h>
#include <std/General/stdMemory.h>
#include <std/Win95/stdShader.h>
#include <std/Win95/GL/Shaders/stdGLSLShaders.h>

#include <std/General/stdEffect.h>
#include <std/Win95/GL/stdShaderBlockGL.h>

#define MAX_SHADER_PROGRAMS 64

static bool stdShader_bStartup = false;
static bool stdShader_bOpen    = false;

static tHashTable* stdShader_pTable = NULL;

static size_t stdShader_maxVsParams = 0;

static GLShaderProgram stdShader_ShaderPrograms[MAX_SHADER_PROGRAMS];
static size_t stdShader_shaderCount = 0;

static GLTextureUnit stdShader_activeTextureUnit = 0;

static GLShaderProgram* stdShader_activeShader = NULL;

typedef struct sStdShaderUbo
{
    GLuint ubo;
    const StdShaderBlockDesc* pDesc;
} StdShaderUbo;

static StdShaderUbo stdShader_aUbo[STDSHADERBLOCK_COUNT];

static StdShaderBlockFadeFactor stdShader_fadeFactor = { 0 };
static StdShaderBlockFogData stdShader_fogData       = { 0 };

static void GetUniformLocations(void);

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
    for ( size_t i = 0; i < STDSHADERBLOCK_COUNT; i++ )
    {
        const StdShaderBlockDesc* desc = &stdShaderBlock_g_aBlockDesc[i];
        StdShaderUbo* pUbo             = &stdShader_aUbo[i];
        pUbo->pDesc                    = desc;

        glGenBuffers(1, &pUbo->ubo);
        glBindBuffer(GL_UNIFORM_BUFFER, pUbo->ubo);
        glBufferData(GL_UNIFORM_BUFFER, (GLsizeiptr)desc->size, desc->initialData, GL_DYNAMIC_DRAW);
        glBindBufferBase(GL_UNIFORM_BUFFER, desc->bindingPoint, pUbo->ubo);
    }

    stdShader_fadeFactor.fadeFactor = 1.0f;
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
    stdShader_UpdateUniformBufferObject(STDSHADERBLOCK_VIEWPORT, vp);
    return true;
}

bool J3DAPI stdShader_SetFog(bool enable, float start, float end, float depthFactor, const StdShaderVector color)
{
    StdShaderBlockFogData* data = &stdShader_fogData;
    data->bEnabled              = enable ? 1.0f : 0.0f;
    data->start                 = start;
    data->end                   = end;
    data->depth                 = depthFactor;
    memcpy(&data->color, color, sizeof(StdShaderVector));

    stdShader_UpdateUniformBufferObject(STDSHADERBLOCK_FOGDATA, &stdShader_fogData);

    return true;
}

bool stdShader_DisableFog(void)
{
    stdShader_SetFog(false, 0.0f, 0.0f, 0.0f, stdShader_fogData.color);
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

    GLShaderProgram* pOld = stdShader_activeShader;
    stdShader_SetActiveShader(pProgram);
    GetUniformLocations();
    stdShader_SetActiveShader(pOld);

    GLuint blockIndex = GL_INVALID_INDEX;
    for ( size_t i = 0; i < STDSHADERBLOCK_COUNT; i++ )
    {
        blockIndex = glGetUniformBlockIndex(pProgram->handle, stdShader_aUbo[i].pDesc->glslName);
        if ( blockIndex != GL_INVALID_INDEX )
        {
            glUniformBlockBinding(pProgram->handle, blockIndex, stdShader_aUbo[i].pDesc->bindingPoint);
        }
    }
    return pProgram;
}

static void GetUniformLocations(void)
{
    GLShaderProgram* pProgram    = stdShader_activeShader;
    GLuint handle                = pProgram->handle;
    pProgram->mainTexLoc         = glGetUniformLocation(handle, "sTexture");
    pProgram->extraLightLoc      = glGetUniformLocation(handle, "cExtraLight");
    pProgram->vertexSpaceLoc     = glGetUniformLocation(handle, "iVertexSpace");
    pProgram->lightModeLoc       = glGetUniformLocation(handle, "iLightMode");
    pProgram->alphaLoc           = glGetUniformLocation(handle, "fAlpha");
    pProgram->modelMatrixLoc     = glGetUniformLocation(handle, "mModelMatrix");
    pProgram->alphaCutLoc        = glGetUniformLocation(handle, "bAlphaCut");
    pProgram->alphaToCoverageLoc = glGetUniformLocation(handle, "bAlphaToCoverage");
    pProgram->renderLightsLoc    = glGetUniformLocation(handle, "bRenderLights");
    //Sprite stuff
    pProgram->spritePosLoc    = glGetUniformLocation(handle, "spritePos");
    pProgram->spriteSizeLoc   = glGetUniformLocation(handle, "spriteSize");
    pProgram->spriteRollLoc   = glGetUniformLocation(handle, "rollAngle");
    pProgram->spriteTypeLoc   = glGetUniformLocation(handle, "spriteType");
    pProgram->spriteYVecLoc   = glGetUniformLocation(handle, "yVec");
    pProgram->spriteOffsetLoc = glGetUniformLocation(handle, "spriteOffset");
    //particle stuff
    pProgram->particlePosLoc      = glGetUniformLocation(handle, "particlePos");
    pProgram->particleHalfSizeLoc = glGetUniformLocation(handle, "particleHalfSize");
    //polyline stuff
    pProgram->polylinePosLoc = glGetUniformLocation(handle, "polyViewPos");
    pProgram->polylineUVLoc  = glGetUniformLocation(handle, "polyTexCoord");
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
    glUniform1i(stdShader_activeShader->mainTexLoc, stdShader_activeTextureUnit);
    stdShader_SetActiveShader(currentProgram);
}

void stdShader_SetActiveTextureUnit(const GLTextureUnit unit)
{
    glActiveTexture(GL_TEXTURE0 + unit);
    stdShader_activeTextureUnit = unit;
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

void stdShader_UpdateFadeFactor(void)
{
    const tStdFadeFactor* fadeFactor = stdEffect_GetFadeFactor();
    stdShader_fadeFactor.fadeFactor  = fadeFactor->bEnabled ? fadeFactor->factor : 1.0f;
    stdShader_UpdateUniformBufferObject(STDSHADERBLOCK_FADEFACTOR, &stdShader_fadeFactor);
}

void stdShader_UpdateUniformBufferObject(const StdShaderBlockId id, const void* data)
{
    glBindBuffer(GL_UNIFORM_BUFFER, stdShader_aUbo[id].ubo);
    glBufferSubData(GL_UNIFORM_BUFFER, 0, (GLsizeiptr)stdShader_aUbo[id].pDesc->size, data);
}

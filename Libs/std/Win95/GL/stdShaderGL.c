#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#include <std/General/std.h>
#include <std/General/stdHashtbl.h>
#include <std/General/stdMemory.h>
#include <std/Win95/stdShader.h>

#define MAX_SHADER_PROGRAMS 64
#define STDSHADER_HANDLE_TO_INDEX(handle) ((handle) - 1)
#define STDSHADER_INDEX_TO_HANDLE(index) ((index) + 1)

static bool stdShader_bStartup = false;
static bool stdShader_bOpen    = false;

static tHashTable* stdShader_pTable = NULL;

static size_t stdShader_maxVsParams = 0;


static GLShaderProgram stdShader_ShaderPrograms[MAX_SHADER_PROGRAMS];
static size_t stdShader_shaderCount = 0;

void stdShader_ResetShader(GLShaderProgram* shaderProgram)
{
    if ( shaderProgram->handle > 0 )
    {
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
    for ( size_t i = 0; i < MAX_SHADER_PROGRAMS; i++ )
    {
        GLShaderProgram* shaderProgram = &stdShader_ShaderPrograms[i];
        if ( shaderProgram->handle > 0 )
        {
            glUseProgram(shaderProgram->handle);
            int loc = glGetUniformLocation(shaderProgram->handle, "viewPort");
            glUniform4f(loc, vp[0], vp[1], vp[2], vp[3]);
            GLenum err = glGetError();
            if ( err != GL_NO_ERROR )
            {
                STDLOG_ERROR("OpenGL error 0x%x in stdShader_SetViewport.\n", err);
                return false;
            }
        }
    }
    return true;
}

bool J3DAPI stdShader_SetFog(bool enable, float start, float end, float depthDactor, const StdShaderVector color)
{
    // TODO: implement fog rendering later
    // float fogParams[4] = { start, end, depthDactor, (float)enable ? 1.0f : 0.0f, };
    // HRESULT hr = IDirect3DDevice9_SetPixelShaderConstantF(stdShader_pDevice, /*StartRegister=*/STDSHADERDX9_PS_FOGPARAM_REGISTER, fogParams, 1);
    // if ( FAILED(hr) )
    // {
    //     STDLOG_ERROR("Error %s setting shader global fog parameters!\n", std3D_D3DGetStatus(hr));
    //     return false;
    // }
    //
    // hr = IDirect3DDevice9_SetPixelShaderConstantF(stdShader_pDevice, /*StartRegister=*/STDSHADERDX9_PS_FOGCOLOR_REGISTER, color, 1);
    // if ( FAILED(hr) )
    // {
    //     STDLOG_ERROR("Error %s setting shader global fog color!\n", std3D_D3DGetStatus(hr));
    //     return false;
    // }

    return true;
}

bool stdShader_DisableFog(void)
{
    // float fogParams[4] = { 0 }; // Disable fog
    // HRESULT hr = IDirect3DDevice9_SetPixelShaderConstantF(stdShader_pDevice, /*StartRegister=*/STDSHADERDX9_PS_FOGPARAM_REGISTER, fogParams, 1);
    // if ( FAILED(hr) )
    // {
    //     STDLOG_ERROR("Error %s disabling shader global fog!\n", std3D_D3DGetStatus(hr));
    //     return false;
    // }

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

static GLuint stdShader_compileShader(GLenum type, const char* source, const char* debugName)
{
    GLuint sh = glCreateShader(type);
    glShaderSource(sh, 1, &source, NULL);
    glCompileShader(sh);

    GLint ok = GL_FALSE;
    glGetShaderiv(sh, GL_COMPILE_STATUS, &ok);
    if ( !ok )
    {
        GLint logLen = 0;
        glGetShaderiv(sh, GL_INFO_LOG_LENGTH, &logLen);
        char* log = STDMALLOC(logLen > 1 ? logLen : 1);
        if ( logLen > 1 )
            glGetShaderInfoLog(sh, logLen, NULL, log);
        else log[0] = '\0';
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

    // Optional: Attribut-Standorte vor dem Linken binden
    // glBindAttribLocation(prog, 0, "aPos");
    // glBindAttribLocation(prog, 1, "aColor");
    // glBindAttribLocation(prog, 2, "aTexCoord");

    glLinkProgram(prog);

    GLint ok = GL_FALSE;
    glGetProgramiv(prog, GL_LINK_STATUS, &ok);
    if ( !ok )
    {
        GLint logLen = 0;
        glGetProgramiv(prog, GL_INFO_LOG_LENGTH, &logLen);
        char* log = STDMALLOC(logLen > 1 ? logLen : 1);
        if ( logLen > 1 )
            glGetProgramInfoLog(prog, logLen, NULL, log);
        else log[0] = '\0';
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

GLShaderProgram* stdShader_CompileAndCreate(const char* pName, const char* pVertexShaderCode,
                                            const char* pPixelShaderCode)
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


    char* vsrc = stdShader_readGLSLFile(pVertexShaderCode);
    char* fsrc = stdShader_readGLSLFile(pPixelShaderCode);
    if ( !vsrc || !fsrc )
    {
        if ( vsrc )
            STDFREE(vsrc);
        if ( fsrc )
            STDFREE(fsrc);
        return NULL;
    }

    GLuint vs = stdShader_compileShader(GL_VERTEX_SHADER, vsrc, pPixelShaderCode);
    GLuint fs = stdShader_compileShader(GL_FRAGMENT_SHADER, fsrc, pPixelShaderCode);

    STDFREE(vsrc);
    STDFREE(fsrc);

    if ( !vs || !fs )
    {
        if ( vs )
            glDeleteShader(vs);
        if ( fs )
            glDeleteShader(fs);
        return NULL;
    }


    // Create shader
    pProgram->handle = stdShader_LinkShaderProgram(vs, fs);
    pProgram->name   = pName;
    stdHashtbl_Add(stdShader_pTable, pName, pProgram);
    stdShader_shaderCount++;

    return pProgram;
}

void J3DAPI stdShader_Free(GLShaderProgram* sh)
{
    stdShader_ResetShader(sh);
}

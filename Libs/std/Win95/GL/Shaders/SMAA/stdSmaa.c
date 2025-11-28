#include <std/Win95/GL/Shaders/SMAA/stdSmaa.h>
#include <std/Win95/stdShader.h>

#include <std/General/std.h>

#include "AreaTex.h"
#include "SearchTex.h"

static GLShaderProgram* stdSmaa_pEdgeDetectionShader     = NULL;
static GLShaderProgram* stdSmaa_pBlendingWeightShader    = NULL;
static GLShaderProgram* stdSmaa_pNeighborhoodBlendShader = NULL;

static GLuint stdSmaa_edgesFBO  = 0;
static GLuint stdSmaa_weightFBO = 0;
static GLuint stdSmaa_blendFBO  = 0;

static GLuint stdSmaa_edgeTex   = 0;
static GLuint stdSmaa_weightTex = 0;
static GLuint stdSmaa_blendTex  = 0;
static GLuint stdSmaa_areaTex   = 0;
static GLuint stdSmaa_searchTex = 0;


int stdSmaa_InitShaders(void)
{
    stdSmaa_pEdgeDetectionShader = stdShader_CompileAndCreate("smaa_edge", "SMAA\\smaa-edges.vert",
                                                              "SMAA\\smaa-edges.frag");

    stdSmaa_pBlendingWeightShader = stdShader_CompileAndCreate("smaa_weight", "SMAA\\smaa-weights.vert",
                                                               "SMAA\\smaa-weights.frag");


    stdSmaa_pNeighborhoodBlendShader = stdShader_CompileAndCreate("smaa_blend", "SMAA\\smaa-blend.vert",
                                                                  "SMAA\\smaa-blend.frag");

    if ( !stdSmaa_pEdgeDetectionShader || !stdSmaa_pBlendingWeightShader || !stdSmaa_pNeighborhoodBlendShader )
    {
        STDLOG_ERROR("Failed to create smaa shaders!\n");
        return 0;
    }

    return 1;
}

int stdSmaa_InitFBOs(uint32_t width, uint32_t height)
{
    // edge detection
    stdShader_SetActiveTextureUnit(TU_SMAA_EDGE);
    glGenFramebuffers(1, &stdSmaa_edgesFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, stdSmaa_edgesFBO);
    glGenTextures(1, &stdSmaa_edgeTex);
    glBindTexture(GL_TEXTURE_2D, stdSmaa_edgeTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RG8, width, height, 0, GL_RG, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, stdSmaa_edgeTex, 0);
    stdShader_SetActiveShader(stdSmaa_pEdgeDetectionShader);
    GLint loc = glGetUniformLocation(stdSmaa_pEdgeDetectionShader->handle, "uTexelSize");
    glUniform2f(loc, 1.0f / (float)width, 1.0f / (float)height);
    loc = glGetUniformLocation(stdSmaa_pEdgeDetectionShader->handle, "uColorTexture");
    glUniform1i(loc, TU_SCENE);
    loc = glGetUniformLocation(stdSmaa_pEdgeDetectionShader->handle, "uDepthTexture");
    glUniform1i(loc, TU_DEPTH);

    //weight
    stdShader_SetActiveTextureUnit(TU_SMAA_WEIGHT);
    glGenFramebuffers(1, &stdSmaa_weightFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, stdSmaa_weightFBO);
    glGenTextures(1, &stdSmaa_weightTex);
    glBindTexture(GL_TEXTURE_2D, stdSmaa_weightTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, stdSmaa_weightTex, 0);

    stdShader_SetActiveTextureUnit(TU_SMAA_AREA);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glGenTextures(1, &stdSmaa_areaTex);
    glBindTexture(GL_TEXTURE_2D, stdSmaa_areaTex);
    unsigned char areaBytes[AREATEX_SIZE];
    for ( unsigned int y = 0; y < AREATEX_HEIGHT; y++ )
    {
        unsigned int srcY = AREATEX_HEIGHT - 1 - y;
        memcpy(&areaBytes[y * AREATEX_PITCH], searchTexBytes + srcY * AREATEX_PITCH, AREATEX_PITCH);
    }
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RG8, AREATEX_WIDTH, AREATEX_HEIGHT, 0, GL_RG, GL_UNSIGNED_BYTE, areaBytes);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    stdShader_SetActiveTextureUnit(TU_SMAA_SEARCH);
    glGenTextures(1, &stdSmaa_searchTex);
    glBindTexture(GL_TEXTURE_2D, stdSmaa_searchTex);
    unsigned char searchBytes[SEARCHTEX_SIZE];
    for ( unsigned int y = 0; y < SEARCHTEX_HEIGHT; y++ )
    {
        unsigned int srcY = SEARCHTEX_HEIGHT - 1 - y;
        memcpy(&searchBytes[y * SEARCHTEX_PITCH], searchTexBytes + srcY * SEARCHTEX_PITCH, SEARCHTEX_PITCH);
    }
    glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, SEARCHTEX_WIDTH, SEARCHTEX_HEIGHT, 0, GL_RED, GL_UNSIGNED_BYTE, searchBytes);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    stdShader_SetActiveShader(stdSmaa_pBlendingWeightShader);

    loc = glGetUniformLocation(stdSmaa_pBlendingWeightShader->handle, "uTexelSize");
    glUniform2f(loc, 1.0f / (float)width, 1.0f / (float)height);

    loc = glGetUniformLocation(stdSmaa_pBlendingWeightShader->handle, "uViewportSize");
    glUniform2f(loc, (float)width, (float)height);


    loc = glGetUniformLocation(stdSmaa_pBlendingWeightShader->handle, "uEdgesTexture");
    glUniform1i(loc, TU_SMAA_EDGE);

    loc = glGetUniformLocation(stdSmaa_pBlendingWeightShader->handle, "uAreaTexture");
    glUniform1i(loc, TU_SMAA_AREA);

    loc = glGetUniformLocation(stdSmaa_pBlendingWeightShader->handle, "uSearchTexture");
    glUniform1i(loc, TU_SMAA_SEARCH);

    // final blending
    stdShader_SetActiveTextureUnit(TU_SMAA_BLEND);

    glGenFramebuffers(1, &stdSmaa_blendFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, stdSmaa_blendFBO);
    glGenTextures(1, &stdSmaa_blendTex);
    glBindTexture(GL_TEXTURE_2D, stdSmaa_blendTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, stdSmaa_blendTex, 0);
    stdShader_SetActiveShader(stdSmaa_pNeighborhoodBlendShader);

    loc = glGetUniformLocation(stdSmaa_pNeighborhoodBlendShader->handle, "uTexelSize");
    glUniform2f(loc, 1.0f / (float)width, 1.0f / (float)height);


    loc = glGetUniformLocation(stdSmaa_pNeighborhoodBlendShader->handle, "uColorTexture");
    glUniform1i(loc, TU_SCENE);

    loc = glGetUniformLocation(stdSmaa_pNeighborhoodBlendShader->handle, "uBlendTexture");
    glUniform1i(loc, TU_SMAA_BLEND);

    stdShader_SetActiveTextureUnit(TU_DEFAULT);
    stdShader_SetActiveShader(NULL);


    return 1;
}

void stdSmaa_Reset(void)
{
    glDeleteBuffers(1, &stdSmaa_edgesFBO);
    glDeleteTextures(1, &stdSmaa_edgeTex);
    stdSmaa_edgesFBO = 0;
    stdSmaa_edgeTex  = 0;

    glDeleteBuffers(1, &stdSmaa_weightFBO);
    glDeleteTextures(1, &stdSmaa_weightTex);
    glDeleteTextures(1, &stdSmaa_areaTex);
    glDeleteTextures(1, &stdSmaa_searchTex);
    stdSmaa_weightFBO = 0;
    stdSmaa_weightTex = 0;
    stdSmaa_areaTex   = 0;
    stdSmaa_searchTex = 0;

    glDeleteBuffers(1, &stdSmaa_blendFBO);
    glDeleteTextures(1, &stdSmaa_blendTex);
    stdSmaa_blendFBO = 0;
    stdSmaa_blendTex = 0;
}

void stdSmaa_ApplySmaa(void)
{
    glBindFramebuffer(GL_FRAMEBUFFER, stdSmaa_edgesFBO);
    stdShader_SetActiveShader(stdSmaa_pEdgeDetectionShader);
    glClear(GL_COLOR_BUFFER_BIT);
    glDrawArrays(GL_TRIANGLES, 0, 3);

    glBindFramebuffer(GL_FRAMEBUFFER, stdSmaa_weightFBO);
    stdShader_SetActiveShader(stdSmaa_pBlendingWeightShader);
    glClear(GL_COLOR_BUFFER_BIT);
    glDrawArrays(GL_TRIANGLES, 0, 3);

    glBindFramebuffer(GL_FRAMEBUFFER, stdSmaa_blendFBO);
    stdShader_SetActiveShader(stdSmaa_pNeighborhoodBlendShader);
    glClear(GL_COLOR_BUFFER_BIT);
    glDrawArrays(GL_TRIANGLES, 0, 3);
}

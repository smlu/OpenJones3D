#include <j3dcore/j3dhook.h>
#include "rdShader.h"

#include <rdroid/Engine/rdCamera.h>
#include <rdroid/types.h>
#include <std/Win95/GL/stdShaderBlockGL.h>

#include "sith/Gameplay/sithTime.h"
#include "std/Win95/stdShader.h"

static StdShaderBlockCamera rdShader_cameraData = { };
static StdShaderBlockLights rdShader_lightsData = { };

static void rdShader_SetProjection(float out[16])
{
    rdClipFrustum* pFrustum = rdCamera_g_pCurCamera->pFrustum;
    if ( !pFrustum )
        return;

    float n = 1.0f / rdCamera_g_pCurCamera->invNearClipPlane;
    float f = 1.0f / rdCamera_g_pCurCamera->invFarClipPlane;

    if ( fabsf(f - n) < 1e-4f )
        return;

    float hwidth  = (float)(rdCamera_g_pCurCamera->pCanvas->rect.right - rdCamera_g_pCurCamera->pCanvas->rect.left) / 2.0f;
    float hheight = (float)(rdCamera_g_pCurCamera->pCanvas->rect.bottom - rdCamera_g_pCurCamera->pCanvas->rect.top) / 2.0f;

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

static void rdShader_SetInverseProjection(float out[16])
{
    rdClipFrustum* pFrustum = rdCamera_g_pCurCamera->pFrustum;
    if ( !pFrustum )
        return;

    float n = pFrustum->nearPlane;
    float f = pFrustum->farPlane;

    if ( fabsf(f - n) < 1e-4f )
        return;

    float hwidth  = (float)(rdCamera_g_pCurCamera->pCanvas->rect.right - rdCamera_g_pCurCamera->pCanvas->rect.left) / 2.0f;
    float hheight = (float)(rdCamera_g_pCurCamera->pCanvas->rect.bottom - rdCamera_g_pCurCamera->pCanvas->rect.top) / 2.0f;

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

static void rdShader_MulMat4(const float a[16], const float b[16], float out[16])
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

void rdShader_ConvertToMat4(const rdMatrix34* pMat, float out[16])
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

void rdShader_UpdateCameraData(void)
{
    rdCamera* pCam = rdCamera_g_pCurCamera;

    if ( pCam == NULL || pCam->pCanvas == NULL || pCam->pFrustum == NULL )
    {
        return;
    }

    rdMatrix34* view = &pCam->viewMatrix;
    rdShader_ConvertToMat4(view, rdShader_cameraData.view);
    rdShader_ConvertToMat4(&rdCamera_g_camMatrix, rdShader_cameraData.inverseView);
    rdShader_SetProjection(rdShader_cameraData.projection);
    rdShader_SetInverseProjection(rdShader_cameraData.inverseProjection);
    rdShader_MulMat4(rdShader_cameraData.projection, rdShader_cameraData.view, rdShader_cameraData.viewProjection);
    rdShader_cameraData.nearPlane   = pCam->pFrustum->nearPlane;
    rdShader_cameraData.farPlane    = pCam->pFrustum->farPlane;
    rdShader_cameraData.focalLength = pCam->focalLength * pCam->aspectRatio;
    rdShader_cameraData.time        += sithTime_g_frameTimeFlex;
    stdShader_UpdateUniformBufferObject(STDSHADERBLOCK_CAMERA, &rdShader_cameraData);
}

void rdShader_SetShaderLights(void)
{
    rdCamera* pCam = rdCamera_g_pCurCamera;

    if ( pCam == NULL )
    {
        return;
    }
    int lightCount = pCam->numLights;
    for ( int i = 0; i < lightCount; i++ )
    {
        rdLight* pLight    = pCam->aLights[i];
        rdVector3 lightPos = pCam->aLightPositions[pLight->num];

        StdShaderBlockPointLight* pShaderLight = &rdShader_lightsData.lights[i];
        memcpy(&pShaderLight->color, &pLight->color, sizeof(rdVector4));
        pShaderLight->position[0] = lightPos.x;
        pShaderLight->position[1] = lightPos.z;
        pShaderLight->position[2] = -lightPos.y;
        pShaderLight->maxRadius   = pLight->maxRadius;
        pShaderLight->minRadius   = pLight->minRadius;
    }
    rdShader_lightsData.lightCount = lightCount;

    stdShader_UpdateUniformBufferObject(STDSHADERBLOCK_LIGHTS, &rdShader_lightsData);
}

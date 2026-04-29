#include "rdCamera.h"
#include <j3dcore/j3dhook.h>

#include <rdroid/Engine/rdLight.h>
#include <rdroid/Main/rdroid.h>
#include <rdroid/Math/rdMatrix.h>
#include <rdroid/Math/rdVector.h>
#include <rdroid/RTI/symbols.h>

#include <std/General/stdMath.h>
#include <std/General/stdMemory.h>
#include <std/General/stdUtil.h>
#include <std/General/stdMemory.h>

#include <std/Win95/stdDisplay.h>

#include "rdroid/Raster/rdCache.h"
#include "std/Win95/stdShader.h"

bool rdCamera_dword_5E10E8 = false;

void J3DAPI rdCamera_BuildFOV(rdCamera* pCamera);
void J3DAPI rdCamera_BuildClipFrustrum(const rdCamera* pCamera, rdClipFrustum* pFrustum, float width, float height);


void rdCamera_InstallHooks(void)
{
    J3D_HOOKFUNC(rdCamera_New);
    J3D_HOOKFUNC(rdCamera_NewEntry);
    J3D_HOOKFUNC(rdCamera_Free);
    J3D_HOOKFUNC(rdCamera_FreeEntry);
    J3D_HOOKFUNC(rdCamera_SetCanvas);
    J3D_HOOKFUNC(rdCamera_SetCurrent);
    J3D_HOOKFUNC(rdCamera_SetFOV);
    J3D_HOOKFUNC(rdCamera_SetProjectType);
    J3D_HOOKFUNC(rdCamera_SetAspectRatio);
    J3D_HOOKFUNC(rdCamera_BuildFOV);
    J3D_HOOKFUNC(rdCamera_BuildClipFrustrum);
    J3D_HOOKFUNC(rdCamera_SetFrustrum);
    J3D_HOOKFUNC(rdCamera_Update);
    J3D_HOOKFUNC(rdCamera_OrthoProject);
    J3D_HOOKFUNC(rdCamera_OrthoProjectLst);
    J3D_HOOKFUNC(rdCamera_OrthoProjectSquare);
    J3D_HOOKFUNC(rdCamera_OrthoProjectSquareLst);
    J3D_HOOKFUNC(rdCamera_PerspProject);
    J3D_HOOKFUNC(rdCamera_PerspProjectLst);
    J3D_HOOKFUNC(rdCamera_PerspProjectSquare);
    J3D_HOOKFUNC(rdCamera_PerspProjectSquareLst);
    J3D_HOOKFUNC(rdCamera_SetAmbientLight);
    J3D_HOOKFUNC(rdCamera_SetAttenuation);
    J3D_HOOKFUNC(rdCamera_AddLight);
    J3D_HOOKFUNC(rdCamera_ClearLights);
    J3D_HOOKFUNC(rdCamera_sub_4C60B0);
}

void rdCamera_ResetGlobals(void)
{
    memset(&rdCamera_g_camPYR, 0, sizeof(rdCamera_g_camPYR));
    memset(&rdCamera_g_pCurCamera, 0, sizeof(rdCamera_g_pCurCamera));
    memset(&rdCamera_g_camMatrix, 0, sizeof(rdCamera_g_camMatrix));
}

inline float rdCamera_ClampFOV(float fov)
{
    return STDMATH_CLAMP(fov, RDCAMERA_FOVMIN, RDCAMERA_FOVMAX);
}

rdCamera* J3DAPI rdCamera_New(float fov, int bFarClip, float nearPlane, float farPlane, float aspectRatio)
{
    rdCamera* pCamera = (rdCamera*)STDMALLOC(sizeof(rdCamera));
    if ( !pCamera )
    {
        RDLOG_ERROR("Error allocating memory for camera.\n");
        return NULL;
    }

    if ( !rdCamera_NewEntry(pCamera, fov, bFarClip, nearPlane, farPlane, aspectRatio) )
    {
        return NULL;
    }

    return pCamera;
}

int J3DAPI rdCamera_NewEntry(rdCamera* pCamera, float fov, int bClipFar, float nearPlane, float farPlane, float aspectRatio)
{
    pCamera->pFrustum = (rdClipFrustum*)STDMALLOC(sizeof(rdClipFrustum));
    if ( !pCamera->pFrustum )
    {
        RDLOG_ERROR("Error allocating memory for camera clip frustrum.\n");
        return 0;
    }

    pCamera->fov     = rdCamera_ClampFOV(fov);
    pCamera->pCanvas = NULL;

    pCamera->pFrustum->bClipFar  = bClipFar;
    pCamera->pFrustum->nearPlane = nearPlane;
    pCamera->pFrustum->farPlane  = farPlane;

    pCamera->aspectRatio      = aspectRatio;
    pCamera->invNearClipPlane = 0.0f;
    pCamera->invFarClipPlane  = 0.0f;
    pCamera->orthoScale       = 1.0f;

    pCamera->numLights      = 0;
    pCamera->attenuationMin = 0.2f;
    pCamera->attenuationMax = 0.1f;

    memset(&pCamera->pFrustum->leftPlaneNormal, 0, sizeof(pCamera->pFrustum->leftPlaneNormal));
    memset(&pCamera->pFrustum->rightPlaneNormal, 0, sizeof(pCamera->pFrustum->rightPlaneNormal));
    memset(&pCamera->pFrustum->topPlaneNormal, 0, sizeof(pCamera->pFrustum->topPlaneNormal));
    memset(&pCamera->pFrustum->bottomPlaneNormal, 0, sizeof(pCamera->pFrustum->bottomPlaneNormal));

    rdCamera_SetProjectType(pCamera, RDCAMERA_PROJECT_PERSPECTIVE);
    return 1;
}

void J3DAPI rdCamera_Free(rdCamera* pCamera)
{
    if ( !pCamera )
    {
        RDLOG_ERROR("Warning: attempt to free NULL camera ptr.\n");
        return;
    }

    if ( pCamera == rdCamera_g_pCurCamera )
    {
        rdCamera_g_pCurCamera = NULL;
    }

    rdCamera_FreeEntry(pCamera);
    stdMemory_Free(pCamera);
}

void J3DAPI rdCamera_FreeEntry(rdCamera* pCamera)
{
    if ( pCamera->pFrustum )
    {
        stdMemory_Free(pCamera->pFrustum);
    }
}

void J3DAPI rdCamera_SetCanvas(rdCamera* pCamera, rdCanvas* pCanvas)
{
    RD_ASSERTREL(pCamera != NULL);
    RD_ASSERTREL(pCanvas != NULL);

    pCamera->pCanvas = pCanvas;
    rdCamera_BuildFOV(pCamera);
}

void J3DAPI rdCamera_SetCurrent(rdCamera* pCamera)
{
    if ( pCamera )
    {
#ifdef J3D_OPENGL
        if ( rdCamera_g_pCurCamera != pCamera )
        {
            rdCache_Flush();
            rdCache_FlushAlpha();
            rdCamera_g_pCurCamera = pCamera;
            stdShader_UpdateGlobalUniforms();
        }
#else
        if ( rdCamera_g_pCurCamera != pCamera )
        {
            rdCamera_g_pCurCamera = pCamera;
        }
#endif
    }
}

void J3DAPI rdCamera_SetFOV(rdCamera* pCamera, float fov)
{
    pCamera->fov = rdCamera_ClampFOV(fov);
    rdCamera_BuildFOV(pCamera);
}

void J3DAPI rdCamera_SetProjectType(rdCamera* pCamera, rdCameraProjType type)
{
    pCamera->projectType = type;
    if ( type == RDCAMERA_PROJECT_ORTHO )
    {
        if ( pCamera->aspectRatio == 1.0f )
        {
            pCamera->pfProject = rdCamera_OrthoProjectSquare;
            pCamera->pfProjectList = rdCamera_OrthoProjectSquareLst;
        }
        else
        {
            pCamera->pfProject = rdCamera_OrthoProject;
            pCamera->pfProjectList = rdCamera_OrthoProjectLst;
        }
    }
    else if ( type == RDCAMERA_PROJECT_PERSPECTIVE )
    {
        if ( pCamera->aspectRatio == 1.0f )
        {
            pCamera->pfProject = rdCamera_PerspProjectSquare;
            pCamera->pfProjectList = rdCamera_PerspProjectSquareLst;
        }
        else
        {
            pCamera->pfProject = rdCamera_PerspProject;
            pCamera->pfProjectList = rdCamera_PerspProjectLst;
        }
    }

    if ( pCamera->pCanvas )
    {
        rdCamera_BuildFOV(pCamera);
    }
}

int J3DAPI rdCamera_SetOrthoScale(rdCamera* pCamera, float scale)
{
    pCamera->orthoScale = scale;
    rdCamera_BuildFOV(pCamera);
    return 1;
}

void J3DAPI rdCamera_SetAspectRatio(rdCamera* pCamera, float ratio)
{
    pCamera->aspectRatio = ratio;
    if ( pCamera->projectType == RDCAMERA_PROJECT_ORTHO )
    {
        if ( pCamera->aspectRatio == 1.0f )
        {
            pCamera->pfProject = rdCamera_OrthoProjectSquare;
            pCamera->pfProjectList = rdCamera_OrthoProjectSquareLst;
        }
        else
        {
            pCamera->pfProject = rdCamera_OrthoProject;
            pCamera->pfProjectList = rdCamera_OrthoProjectLst;
        }
    }

    else if ( pCamera->projectType == RDCAMERA_PROJECT_PERSPECTIVE )
    {
        if ( pCamera->aspectRatio == 1.0f )
        {
            pCamera->pfProject = rdCamera_PerspProjectSquare;
            pCamera->pfProjectList = rdCamera_PerspProjectSquareLst;
        }
        else
        {
            pCamera->pfProject = rdCamera_PerspProject;
            pCamera->pfProjectList = rdCamera_PerspProjectLst;
        }
    }

    rdCamera_BuildFOV(pCamera);
}

void J3DAPI rdCamera_BuildFOV(rdCamera* pCamera)
{
    if ( pCamera->pCanvas )
    {
        if ( pCamera->projectType == RDCAMERA_PROJECT_ORTHO )
        {
            float hwidth = (pCamera->pCanvas->rect.right - pCamera->pCanvas->rect.left) / 2.0f;
            float hheight = (pCamera->pCanvas->rect.bottom - pCamera->pCanvas->rect.top) / 2.0f;

            pCamera->pFrustum->orthoLeftPlane   = -(hwidth / pCamera->orthoScale) / pCamera->aspectRatio;
            pCamera->pFrustum->orthoTopPlane    = hheight / pCamera->orthoScale / pCamera->aspectRatio;
            pCamera->pFrustum->orthoRightPlane  = hwidth / pCamera->orthoScale / pCamera->aspectRatio;
            pCamera->pFrustum->orthoBottomPlane = -(hheight / pCamera->orthoScale) / pCamera->aspectRatio;

            pCamera->focalLength = 0.0f;

            pCamera->pFrustum->topPlane    = 0.0f;
            pCamera->pFrustum->bottomPlane = 0.0f;
            pCamera->pFrustum->leftPlane   = 0.0f;
            pCamera->pFrustum->rightPlane  = 0.0f;
        }
        else if ( pCamera->projectType == RDCAMERA_PROJECT_PERSPECTIVE )
        {
            float hwidth  = (pCamera->pCanvas->rect.right - pCamera->pCanvas->rect.left) / 2.0f;
            float hheight = (pCamera->pCanvas->rect.bottom - pCamera->pCanvas->rect.top) / 2.0f;

            pCamera->focalLength = hwidth / stdMath_Tan(pCamera->fov / 2.0f); // distance from cam to near plane

            // Added: Adjust focal length for canvas aspect ratio
            float fscale         = RD_REF_APECTRATIO / (hwidth / hheight);
            pCamera->focalLength = pCamera->focalLength * fscale;

            if ( rdCamera_dword_5E10E8 )
            {
                pCamera->invNearClipPlane = 1.0f / pCamera->pFrustum->nearPlane;
                pCamera->invFarClipPlane  = 1.0f / (1.0f / pCamera->pFrustum->farPlane - pCamera->invNearClipPlane) * 0.1f;
            }
            else
            {
                pCamera->invNearClipPlane = 1.0f / pCamera->pFrustum->nearPlane;
                pCamera->invFarClipPlane  = 1.0f / (1.0f / pCamera->pFrustum->farPlane - pCamera->invNearClipPlane) * 0.89999998f;// Used for converting vertex to screen cords; 0.89999f - 1/1.111f
                pCamera->invNearClipPlane = 0.1f / (0.89999998f * pCamera->invFarClipPlane) + pCamera->invNearClipPlane; // Used for converting vertex to screen cords
            }

            // Setup frustum planes
            pCamera->pFrustum->topPlane    =  (hheight / pCamera->focalLength) / pCamera->aspectRatio;
            pCamera->pFrustum->bottomPlane = -(hheight / pCamera->focalLength) / pCamera->aspectRatio;
            pCamera->pFrustum->leftPlane   = -(hwidth / pCamera->focalLength) / pCamera->aspectRatio;
            pCamera->pFrustum->rightPlane  =  (hwidth / pCamera->focalLength) / pCamera->aspectRatio;

            float height = hheight * 2.0f;
            float width  = hwidth * 2.0f;
            rdCamera_BuildClipFrustrum(pCamera, pCamera->pFrustum, width, height);
        }
    }
}

void J3DAPI rdCamera_BuildClipFrustrum(const rdCamera* pCamera, rdClipFrustum* pFrustum, float width, float height)
{
    float hwidth  = width * 0.5f;
    float hheight = height * 0.5f;

    pFrustum->leftPlaneNormal.x = -pCamera->focalLength;
    pFrustum->leftPlaneNormal.y = -hwidth;
    pFrustum->leftPlaneNormal.z = 0.0f;
    rdVector_Normalize3Acc(&pFrustum->leftPlaneNormal);

    rdVector_Copy3(&pFrustum->rightPlaneNormal, &pFrustum->leftPlaneNormal);
    pFrustum->rightPlaneNormal.x = -pFrustum->rightPlaneNormal.x;

    pFrustum->topPlaneNormal.x = 0.0f;
    pFrustum->topPlaneNormal.y = -hheight;
    pFrustum->topPlaneNormal.z = pCamera->focalLength;
    rdVector_Normalize3Acc(&pFrustum->topPlaneNormal);

    rdVector_Copy3(&pFrustum->bottomPlaneNormal, &pFrustum->topPlaneNormal);
    pFrustum->bottomPlaneNormal.z = -pFrustum->bottomPlaneNormal.z;
}

int J3DAPI rdCamera_SetFrustrum(rdCamera* pCamera, rdClipFrustum* pFrustrum, int left, int top, int right, int bottom)
{
    const rdCanvas* pCanvas = pCamera->pCanvas;
    if ( !pCanvas )
    {
        return 0;
    }

    float lsize = pCanvas->center.x - (left - 0.5f);
    float rsize = right - 0.5f - pCanvas->center.x;
    float tsize = pCanvas->center.y - (top - 0.5f);
    float bsize = bottom - 0.5f - pCanvas->center.y;

    pFrustrum->topPlane    =  tsize / pCamera->focalLength / pCamera->aspectRatio;
    pFrustrum->leftPlane   = -lsize / pCamera->focalLength / pCamera->aspectRatio;
    pFrustrum->bottomPlane = -bsize / pCamera->focalLength / pCamera->aspectRatio;
    pFrustrum->rightPlane  =  rsize / pCamera->focalLength / pCamera->aspectRatio;

    pFrustrum->bClipFar    = pCamera->pFrustum->bClipFar;
    pFrustrum->nearPlane   = pCamera->pFrustum->nearPlane;
    pFrustrum->farPlane    = pCamera->pFrustum->farPlane;

    if ( rdCamera_dword_5E10E8 )
    {
        pCamera->invNearClipPlane = 1.0f / pCamera->pFrustum->nearPlane;
        pCamera->invFarClipPlane = 1.0f / (1.0f / pCamera->pFrustum->farPlane - pCamera->invNearClipPlane) * 0.1f;
    }
    else
    {
        pCamera->invNearClipPlane = 1.0f / pCamera->pFrustum->nearPlane;
        pCamera->invFarClipPlane = 1.0f / (1.0f / pCamera->pFrustum->farPlane - pCamera->invNearClipPlane) * 0.89999998f;
        pCamera->invNearClipPlane = 0.1f / (0.89999998f * pCamera->invFarClipPlane) + pCamera->invNearClipPlane;
    }

    float height = (float)(pCanvas->rect.bottom - pCanvas->rect.top);
    float width = (float)(pCanvas->rect.right - pCanvas->rect.left);
    rdCamera_BuildClipFrustrum(pCamera, pFrustrum, width, height);
    return 1;
}

void J3DAPI rdCamera_Update(const rdMatrix34* orient)
{
    rdMatrix_InvertOrtho34(&rdCamera_g_pCurCamera->viewMatrix, orient);  // Transform to view matrix (i.e.: inverse of orient)
    memcpy(&rdCamera_g_camMatrix, orient, sizeof(rdCamera_g_camMatrix)); // Copy camera model matrix
    rdMatrix_ExtractAngles34(&rdCamera_g_camMatrix, &rdCamera_g_camPYR); // Set camera pitch, yaw, roll from world matrix
}

void J3DAPI rdCamera_OrthoProject(rdVector3* pDestVertex, const rdVector3* pSrcVertex)
{
    // Function does orthogonal projection from view space (camera space ) to screen space.

    const rdCanvas* pCanvas = rdCamera_g_pCurCamera->pCanvas;
    pDestVertex->x = rdCamera_g_pCurCamera->orthoScale * rdCamera_g_pCurCamera->aspectRatio * pSrcVertex->x + pCanvas->center.x; // Fixed: Multiplied focalLength by aspectRatio
    pDestVertex->y = -(pSrcVertex->z * rdCamera_g_pCurCamera->orthoScale) * rdCamera_g_pCurCamera->aspectRatio + pCanvas->center.y;
    pDestVertex->z =  1.0f / pSrcVertex->y; // Fixed: Fixed inverting y coord to fit within the range of 0.9999. In original implementation y-coord was assigned without being inverted, i.e. 1/y.
}

void J3DAPI rdCamera_OrthoProjectLst(rdVector3* pDestVerts, const rdVector3* pSrcVerts, size_t numVerts)
{
    // Function does orthogonal projection from view space (camera space ) to screen space.
    while ( numVerts-- )
    {
        rdCamera_OrthoProject(pDestVerts, pSrcVerts);
        ++pSrcVerts;
        ++pDestVerts;
    }
}

void J3DAPI rdCamera_OrthoProjectSquare(rdVector3* pDestVertex, const rdVector3* pSrcVertex)
{
    // Function does orthogonal projection from view space (camera space) to screen space.

    const rdCanvas* pCanvas = rdCamera_g_pCurCamera->pCanvas;
    pDestVertex->x = rdCamera_g_pCurCamera->orthoScale * pSrcVertex->x + pCanvas->center.x;
    pDestVertex->y = pCanvas->center.y - pSrcVertex->z * rdCamera_g_pCurCamera->orthoScale;
    pDestVertex->z = 1.0f / pSrcVertex->y; // Fixed: Fixed inverting y coord to fit within the range of 0.9999. In original implementation y-coord was assigned without being inverted, i.e. 1/y.
}

void J3DAPI rdCamera_OrthoProjectSquareLst(rdVector3* pDestVerts, const rdVector3* pSrcVerts, size_t numVerts)
{
    // Function does ortho projection from view space (camera space ) to screen space.

    while ( numVerts-- )
    {
        rdCamera_OrthoProjectSquare(pDestVerts, pSrcVerts);
        ++pSrcVerts;
        ++pDestVerts;
    }
}

void J3DAPI rdCamera_PerspProject(rdVector3* pDestVertex, const rdVector3* pSrcVertex)
{
    // Function does perspective projection from view space (camera space ) to screen space.
    // 2D clip space coord system:
    //   ^
    // y | /z
    //   |/
    //   ----->
    //     x

    const rdCanvas* pCanvas = rdCamera_g_pCurCamera->pCanvas;
    float tz    = rdCamera_g_pCurCamera->focalLength / pSrcVertex->y;
    float scale = rdCamera_g_pCurCamera->aspectRatio * tz;
    pDestVertex->x = pSrcVertex->x * scale + pCanvas->center.x; // Fixed: Multiplied focalLength by aspectRatio
    pDestVertex->y = pCanvas->center.y - pSrcVertex->z * scale;
    pDestVertex->z = 1.0f / pSrcVertex->y; // Fixed: Fixed inverting y coord to fit within the range of 0.9999. In original implementation y-coord was assigned without being inverted, i.e. 1/y. This fixes Z coord for sky rendering
}

void J3DAPI rdCamera_PerspProjectLst(rdVector3* pDestVerts, const rdVector3* pSrcVerts, size_t numVerts)
{
    // Function does perspective projection from view space (camera space ) to intermediate projection space. (2D projection in intermediate space between camera space and clip space) 
    while ( numVerts-- )
    {
        rdCamera_PerspProject(pDestVerts, pSrcVerts);
        ++pSrcVerts;
        ++pDestVerts;
    }
}

void J3DAPI rdCamera_PerspProjectSquare(rdVector3* pDestVertex, const rdVector3* pSrcVertex)
{
    // Function does perspective projection from view space (camera space ) to screen space.
    // 2D clip space coord system:
    //   ^
    // y | /z
    //   |/
    //   ----->
    //     x

    const rdCanvas* pCanvas = rdCamera_g_pCurCamera->pCanvas;
    float tz    = 1.0f / pSrcVertex->y;
    float scale = rdCamera_g_pCurCamera->focalLength * tz;
    pDestVertex->x = pSrcVertex->x * scale + pCanvas->center.x;
    pDestVertex->y = pCanvas->center.y - pSrcVertex->z * scale;
    pDestVertex->z = tz;
}

void J3DAPI rdCamera_PerspProjectSquareLst(rdVector3* pDestVerts, const rdVector3* pSrcVerts, size_t numVerts)
{
    // Function does perspective projection from view space (camera space ) to screen space.
    while ( numVerts-- )
    {
        rdCamera_PerspProjectSquare(pDestVerts, pSrcVerts);
        ++pSrcVerts;
        ++pDestVerts;
    }
}

void J3DAPI rdCamera_SetAmbientLight(rdCamera* pCamera, const rdVector4* pLight)
{
    rdVector_Copy4(&pCamera->ambientLight, pLight);
}

void J3DAPI rdCamera_SetAttenuation(rdCamera* pCamera, float min, float max)
{
    pCamera->attenuationMin = min;
    pCamera->attenuationMax = max;

    for ( size_t i = 0; i < pCamera->numLights; ++i )
    {
        rdLight* pLight = pCamera->aLights[i];
        if ( pLight->minRadius == 0.0f )
        {
            pLight->minRadius = pCamera->attenuationMin * 40.0f;
        }

        if ( pLight->maxRadius == 0.0f )
        {
            pLight->maxRadius = pCamera->attenuationMax * 50.0f;
        }
    }
}

int J3DAPI rdCamera_AddLight(rdCamera* pCamera, rdLight* pLight, const rdVector3* pPos)
{
    if ( pCamera->numLights >= STD_ARRAYLEN(pCamera->aLights) ) // Fixed: Fixed bound check to smaller than aLights array size. Was smaller or equal.
    {
        return 0;
    }

    pLight->num = pCamera->numLights;
    pCamera->aLights[pCamera->numLights] = pLight;
    rdVector_Copy3(&pCamera->aLightPositions[pCamera->numLights], pPos);

    if ( pLight->minRadius == 0.0f )
    {
        pLight->minRadius = rdLight_GetIntensity(&pLight->color) / pCamera->attenuationMin;
    }

    if ( pLight->maxRadius == 0.0f )
    {
        pLight->maxRadius = rdLight_GetIntensity(&pLight->color) / pCamera->attenuationMax;
    }

    ++pCamera->numLights;
    return 1;
}

int J3DAPI rdCamera_ClearLights(rdCamera* pCamera)
{
    pCamera->numLights = 0;
    return 1;
}

void J3DAPI rdCamera_sub_4C60B0(bool bEnable)
{
    rdCamera_dword_5E10E8 = bEnable;
}

bool rdCamera_sub_506861(void)
{
    return rdCamera_dword_5E10E8;
}

#include "sithRenderSky.h"
#include <j3dcore/j3dhook.h>

#include <sith/Engine/sithCamera.h>
#include <sith/Engine/sithIntersect.h>
#include <sith/RTI/symbols.h>
#include <sith/World/sithWorld.h>

#include <rdroid/Engine/rdCamera.h>
#include <rdroid/Math/rdMatrix.h>
#include <rdroid/Math/rdVector.h>

#include <std/General/stdMath.h>
#include <std/Win95/std3D.h>

// Ceiling sky globals
static float ceilingSkyHeight;
static rdVector3 ceilingSkyMinZ;
static rdVector3 ceilingSkyMaxZ;
static const rdVector3 ceilingSkyNormal = { 0.0f, 0.0f, -1.0f };

// Horizon sky globals
static float horizonSkyDistance;
static float horizonPixelsPerRev;
static float horizonScale;

static float lookPitch;
static float lookYaw;

static float lookRollCos;
static float lookRollSin;

static const float horizonPlaneVertices[4][4] =
{
    { -1.0f, -1.0f, 1.0f },
    { 1.0f, -1.0f, 1.0f },
    { 1.0f, 1.0f, 1.0f },
    { -1.0f, 1.0f, 1.0f }
};

void sithRenderSky_InstallHooks(void)
{
    J3D_HOOKFUNC(sithRenderSky_Open);
    J3D_HOOKFUNC(sithRenderSky_Update);
    J3D_HOOKFUNC(sithRenderSky_HorizonFaceToPlane);
    J3D_HOOKFUNC(sithRenderSky_CeilingFaceToPlane);
}

void sithRenderSky_ResetGlobals(void)
{
}

int J3DAPI sithRenderSky_Open(float horizonDistance, float ceilingHeight)
{
    horizonPixelsPerRev = 0.0083333338f; // (768.0f / 360.0f) / 256.0f
    horizonSkyDistance  = horizonDistance / 256.0f;

    ceilingSkyHeight = ceilingHeight;

    ceilingSkyMaxZ.x = 0.0f;
    ceilingSkyMaxZ.y = 0.0f;
    ceilingSkyMaxZ.z = ceilingHeight;

    ceilingSkyMinZ.x = 0.0f;
    ceilingSkyMinZ.y = 0.0f;
    ceilingSkyMinZ.z = -ceilingHeight;

#ifdef J3D_OPENGL
    std3D_SetCeilingSkyHeight(ceilingHeight);
#endif

    return 0;
}

void sithRenderSky_Close(void)
{
}

void sithRenderSky_Update(void)
{
    horizonScale = horizonSkyDistance / (rdCamera_g_pCurCamera->focalLength * rdCamera_g_pCurCamera->aspectRatio);
    // Added: Multiply focalLength by aspectRatio, fixes sky rendering when aspect ratio is not 1:1 (underwater)
    stdMath_SinCos(sithCamera_g_pCurCamera->lookPYR.z, &lookRollSin, &lookRollCos);

    lookYaw   = -(sithCamera_g_pCurCamera->lookPYR.yaw * horizonPixelsPerRev);
    lookPitch = -(sithCamera_g_pCurCamera->lookPYR.pitch * horizonPixelsPerRev);

    std3D_UpdateHorizonSky(lookPitch, lookYaw, horizonScale);
}

void J3DAPI sithRenderSky_HorizonFaceToPlane(rdCacheProcEntry* pPoly, const rdFace* pFace, rdVector3* aVerts, size_t numVerts)
{
    pPoly->lightingMode = RD_LIGHTING_NONE;
    for ( size_t i = 0; i < numVerts; ++i )
    {
        LPD3DTLVERTEX pOutVert = &pPoly->aVertices[i];
        pOutVert->sx           = aVerts[i].x;
        pOutVert->sy           = aVerts[i].y;
        pOutVert->sz           = aVerts[i].z;

        pOutVert->rhw = horizonSkyDistance;
        pOutVert->sz  = 0.99996948f; // almost at cam far plane dist

        float xFactor = (pOutVert->sx - rdCamera_g_pCurCamera->pCanvas->center.x) * horizonScale;
        float yFactor = (pOutVert->sy - rdCamera_g_pCurCamera->pCanvas->center.y) * horizonScale;

        pOutVert->tu = xFactor * lookRollCos - yFactor * lookRollSin + lookYaw;
        pOutVert->tv = xFactor * lookRollSin + yFactor * lookRollCos + lookPitch;
        pOutVert->tu += sithWorld_g_pCurrentWorld->horizonSkyOffset.x + pFace->texVertOffset.x;
        pOutVert->tv += sithWorld_g_pCurrentWorld->horizonSkyOffset.y + pFace->texVertOffset.y;
    }
}

void J3DAPI sithRenderSky_CeilingFaceToPlane(rdCacheProcEntry* pPoly, const rdFace* pFace, const rdVector3* aVerts, const rdVector3* aTransformedVerts, size_t numVerts)
{
    if ( pFace->pMaterial )
    {
        float invMatWidth   = 1.0f / (float)pFace->pMaterial->width;
        float invMatHeight  = 1.0f / (float)pFace->pMaterial->height;
        pPoly->lightingMode = RD_LIGHTING_NONE;

        for ( size_t i = 0; i < numVerts; ++i )
        {
            rdVector3 skyVert = aVerts[i]; //sithWorld_g_pCurrentWorld->aVertices[pFace->aVertices[i]];
            //rdMatrix_TransformPoint34(&skyVert, &aVerts[i], &rdCamera_g_camMatrix);

            rdVector_Sub3Acc(&skyVert, &sithCamera_g_pCurCamera->lookPos);
            rdVector_Normalize3Acc(&skyVert);

            float skyDist;
            if ( !sithIntersect_CheckSphereHit(&sithCamera_g_pCurCamera->lookPos, &skyVert, 1000.0f, 0.0f, &ceilingSkyNormal, &ceilingSkyMaxZ, &skyDist, 0) )
            {
                skyDist = 1000.0f;
            }

            rdVector_Scale3Acc(&skyVert, skyDist);
            rdVector_Add3Acc(&skyVert, &sithCamera_g_pCurCamera->lookPos);

            float tu = skyVert.x * 16.0f * invMatWidth;
            float tv = skyVert.y * 16.0f * invMatHeight;
            tu += sithWorld_g_pCurrentWorld->ceilingSkyOffset.x;
            tv += sithWorld_g_pCurrentWorld->ceilingSkyOffset.y;
            tu += pFace->texVertOffset.x;
            tv += pFace->texVertOffset.y;

            LPD3DTLVERTEX pOutVert = &pPoly->aVertices[i];
            pOutVert->tu           = tu;
            pOutVert->tv           = tv;
            pOutVert->sx           = aTransformedVerts[i].x;
            pOutVert->sy           = aTransformedVerts[i].y;
            pOutVert->sz           = 0.99996948f;
            pOutVert->rhw          = aTransformedVerts[i].z / 32.0f;
        }
    }
}

#ifdef J3D_OPENGL
void J3DAPI sithRenderSky_SetCeilingSkyVertices(rdCacheProcEntry* pPoly, rdFace* pFace, const rdVector3* aVerts, size_t numVerts)
{
    pPoly->lightingMode = RD_LIGHTING_NONE;
    pPoly->flags |= RD_FF_CEILING_SKY;

    for ( size_t i = 0; i < numVerts; ++i )
    {
        LPD3DTLVERTEX pOutVert = &pPoly->aVertices[i];
        pOutVert->tu           = sithWorld_g_pCurrentWorld->ceilingSkyOffset.x + pFace->texVertOffset.x;
        pOutVert->tv           = sithWorld_g_pCurrentWorld->ceilingSkyOffset.y + pFace->texVertOffset.y;

        rdVector3 vertex = aVerts[i];

        pOutVert->sx  = vertex.x;
        pOutVert->sy  = vertex.z;
        pOutVert->sz  = -vertex.y;
        pOutVert->rhw = 1.0f;
    }
}

void J3DAPI sithRenderSky_SetHorizonSkyVertices(rdCacheProcEntry* pPoly)
{
    pPoly->lightingMode = RD_LIGHTING_NONE;
    pPoly->flags |= RD_FF_HORIZON_SKY;
    const LPD3DTLVERTEX pOutVert = pPoly->aVertices;

    for ( size_t i = 0; i < 4; ++i )
    {
        const float* position = horizonPlaneVertices[i];
        pOutVert[i].sx        = position[0];
        pOutVert[i].sy        = position[1];
        pOutVert[i].sz        = position[2];
        pOutVert[i].rhw       = 1.0f;

        pOutVert[i].tu = sithWorld_g_pCurrentWorld->horizonSkyOffset.x;
        pOutVert[i].tv = sithWorld_g_pCurrentWorld->horizonSkyOffset.y;
    }
}
#endif

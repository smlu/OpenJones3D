#include "rdClip.h"
#include <j3dcore/j3dhook.h>

#include <rdroid/Engine/rdCamera.h>
#include <rdroid/Engine/rdQClip.h>
#include <rdroid/Math/rdMath.h>
#include <rdroid/Math/rdVector.h>
#include <rdroid/Main/rdroid.h>
#include <rdroid/RTI/symbols.h>

#include <std/General/stdUtil.h>
#include <std/Win95/stdDisplay.h>

#include<math.h>

#include "std/Win95/std3D.h"

static rdVector2 rdClip_aWorkTVerts[MAX_CLIP_VERTICIES] = { 0 };
static rdVector2* rdClip_pSourceTVert;
static rdVector2* rdClip_pDestTVert;

static rdVector3 rdClip_aWorkVerts[MAX_CLIP_VERTICIES]  = { 0 };
static rdVector3* rdClip_pSourceVert;
static rdVector3* rdClip_pDestVert;

static rdVector4 rdClip_aWorkVertIntensities[MAX_CLIP_VERTICIES] = { 0 };
static rdVector4* rdClip_pSourceVertIntensity;
static rdVector4* rdClip_pDestVertIntensity;

static rdVector3 rdClip_aWorkFaceVerts[144]           = { 0 };
static rdVector2 rdClip_aWorkFaceTVerts[144]          = { 0 };
static rdVector4 rdClip_aWorkFaceVertLight[144]       = { 0 };
static rdVector4 rdClip_aWorkFaceVertIntensities[144] = { 0 };

void rdClip_InstallHooks(void)
{
    J3D_HOOKFUNC(rdClip_Line2);
    J3D_HOOKFUNC(rdClip_CalcOutcode2);
    J3D_HOOKFUNC(rdClip_Line2Ex);
    J3D_HOOKFUNC(rdClip_CalcOutcode2Ex);
    J3D_HOOKFUNC(rdClip_ClipFacePVS);
    J3D_HOOKFUNC(rdClip_Face3WPVS);
    J3D_HOOKFUNC(rdClip_Face3W);
    J3D_HOOKFUNC(rdClip_Face3WOrtho);
    J3D_HOOKFUNC(rdClip_Face3S);
    J3D_HOOKFUNC(rdClip_Face3SOrtho);
    J3D_HOOKFUNC(rdClip_Face3GS);
    J3D_HOOKFUNC(rdClip_Face3GSOrtho);
    J3D_HOOKFUNC(rdClip_Face3GT);
    J3D_HOOKFUNC(rdClip_Face3GTOrtho);
    J3D_HOOKFUNC(rdClip_Face3T);
    J3D_HOOKFUNC(rdClip_Face3TOrtho);
    J3D_HOOKFUNC(rdClip_SphereInFrustrum);
    J3D_HOOKFUNC(rdClip_QClipFaceW);
    J3D_HOOKFUNC(rdClip_FaceToPlane);
    J3D_HOOKFUNC(rdClip_VerticesToPlane);
}

void rdClip_ResetGlobals(void)
{
    memset(&rdClip_g_faceStatus, 0, sizeof(rdClip_g_faceStatus));
}

int J3DAPI rdClip_Line2(const rdCanvas* pCanvas, int* x1, int* y1, int* x2, int* y2)
{
    rdClipOutcode ccode1 = rdClip_CalcOutcode2(pCanvas, *x1, *y1);
    rdClipOutcode ccode2 = rdClip_CalcOutcode2(pCanvas, *x2, *y2);
    if ( !ccode1 && !ccode2 )
    {
        return 1;
    }

    if ( (ccode2 & ccode1) != 0 )
    {
        return 0;
    }

    float fx1 = (float)*x1;
    float fy1 = (float)*y1;

    float fx2 = (float)*x2;
    float fy2 = (float)*y2;

    bool bXEqual = fx1 == fx2;
    bool bYEqual = fy1 == fy2;
    do
    {
        rdClipOutcode ccode = ccode1 ? ccode1 : ccode2;

        float x, y;
        if ( (ccode & RDCLIP_OUTCODE_TOP) != 0 )
        {
            if ( bXEqual )
            {
                x = fx1;
            }
            else
            {
                x = (fx2 - fx1) / (fy2 - fy1) * ((float)pCanvas->rect.top - fy1) + fx1;
            }

            y = (float)pCanvas->rect.top;
        }
        else if ( (ccode & RDCLIP_OUTCODE_BOTTOM) != 0 )
        {
            if ( bXEqual )
            {
                x = fx1;
            }
            else
            {
                x = (fx2 - fx1) / (fy2 - fy1) * ((float)pCanvas->rect.bottom - fy1) + fx1;
            }

            y = (float)pCanvas->rect.bottom;
        }
        else if ( (ccode & RDCLIP_OUTCODE_RIGHT) != 0 )
        {
            if ( bYEqual )
            {
                y = fy1;
            }
            else
            {
                y = (fy2 - fy1) / (fx2 - fx1) * ((float)pCanvas->rect.right - fx1) + fy1;
            }

            x = (float)pCanvas->rect.right;
        }
        else
        {
            if ( bYEqual )
            {
                y = fy1;
            }
            else
            {
                y = (fy2 - fy1) / (fx2 - fx1) * ((float)pCanvas->rect.left - fx1) + fy1;
            }

            x = (float)pCanvas->rect.left;
        }

        if ( ccode == ccode1 )
        {
            fx1 = x;
            fy1 = y;
            ccode1 = rdClip_CalcOutcode2(pCanvas, (int32_t)x, (int32_t)y);
        }
        else
        {
            fx2 = x;
            fy2 = y;
            ccode2 = rdClip_CalcOutcode2(pCanvas, (int32_t)x, (int32_t)y);
        }

        if ( !ccode1 && !ccode2 )
        {
            *x1 = (int32_t)fx1;
            *y1 = (int32_t)fy1;
            *x2 = (int32_t)fx2;
            *y2 = (int32_t)fy2;
            return 1;
        }
    } while ( (ccode2 & ccode1) == 0 );

    return 0;
}

rdClipOutcode J3DAPI rdClip_CalcOutcode2(const rdCanvas* pCanvas, int x, int y)
{
    rdClipOutcode ccode = 0;
    if ( x >= pCanvas->rect.left )
    {
        if ( x > pCanvas->rect.right )
        {
            ccode = RDCLIP_OUTCODE_RIGHT;
        }
    }
    else
    {
        ccode = RDCLIP_OUTCODE_LEFT;
    }

    if ( y >= pCanvas->rect.top )
    {
        if ( y > pCanvas->rect.bottom )
        {
            ccode |= RDCLIP_OUTCODE_BOTTOM;                 // 0x100 - RDCLIP_OUTCODE_BOTTOM
        }
    }
    else
    {
        ccode |= RDCLIP_OUTCODE_TOP;                  // 0x1000 - RDCLIP_OUTCODE_TOP
    }

    return ccode;
}

int J3DAPI rdClip_Line2Ex(float* pX1, float* pY1, float* pX2, float* pY2)
{
    uint32_t bwidth, bheight;
    stdDisplay_GetBackBufferSize(&bwidth, &bheight);
    float width = (float)bwidth;
    float height = (float)bheight;

    rdClipOutcode ccode1 = rdClip_CalcOutcode2Ex(*pX1, *pY1, width, height);
    rdClipOutcode ccode2 = rdClip_CalcOutcode2Ex(*pX2, *pY2, width, height);

    if ( !ccode1 && !ccode2 )
    {
        return 1;
    }

    if ( (ccode2 & ccode1) != 0 )
    {
        return 0;
    }

    float x1 = *pX1;
    float y1 = *pY1;

    float x2 = *pX2;
    float y2 = *pY2;

    bool bXEqual = *pX1 == *pX2;
    bool bYEqual = y1 == y2;

    do
    {
        rdClipOutcode ccode = ccode1 ? ccode1 : ccode2;

        float x, y;
        if ( (ccode & RDCLIP_OUTCODE_TOP) != 0 )
        {
            if ( bXEqual )
            {
                x = x1;
            }
            else
            {
                x = (x2 - x1) / (y2 - y1) * (0.0f - y1) + x1;
            }

            y = 0.0f;
        }
        else if ( (ccode & RDCLIP_OUTCODE_BOTTOM) != 0 )
        {
            if ( bXEqual )
            {
                x = x1;
            }
            else
            {
                x = (x2 - x1) / (y2 - y1) * (height - y1) + x1;
            }

            y = height;
        }
        else if ( (ccode & RDCLIP_OUTCODE_RIGHT) != 0 )
        {
            if ( bYEqual )
            {
                y = y1;
            }
            else
            {
                y = (y2 - y1) / (x2 - x1) * (width - x1) + y1;
            }

            x = width;
        }
        else
        {
            if ( bYEqual )
            {
                y = y1;
            }
            else
            {
                y = (y2 - y1) / (x2 - x1) * (0.0f - x1) + y1;
            }

            x = 0.0f;
        }

        if ( ccode == ccode1 )
        {
            x1 = x;
            y1 = y;
            ccode1 = rdClip_CalcOutcode2Ex(x, y, width, height);
        }
        else
        {
            x2 = x;
            y2 = y;
            ccode2 = rdClip_CalcOutcode2Ex(x, y, width, height);
        }

        if ( !ccode1 && !ccode2 )
        {
            *pX1 = x1;
            *pY1 = y1;
            *pX2 = x2;
            *pY2 = y2;
            return 1;
        }
    } while ( (ccode2 & ccode1) == 0 );

    return 0;
}

int J3DAPI rdClip_CalcOutcode2Ex(float x, float y, float width, float height)
{
    rdClipOutcode ccode = 0;
    if ( x >= 0.0f )
    {
        if ( x > width )
        {
            ccode = RDCLIP_OUTCODE_RIGHT;
        }
    }
    else
    {
        ccode = RDCLIP_OUTCODE_LEFT;
    }

    if ( y >= 0.0f )
    {
        if ( y > height )
        {
            ccode |= RDCLIP_OUTCODE_BOTTOM;
        }
    }
    else
    {
        ccode |= RDCLIP_OUTCODE_TOP;
    }

    return ccode;
}

int J3DAPI rdClip_Point3(const rdClipFrustum* pFrustum, const rdVector3* pPoint)
{
    if ( pPoint->y < pFrustum->nearPlane )
    {
        return 0;
    }

    if ( pFrustum->bClipFar && pPoint->y > pFrustum->farPlane )
    {
        return 0;
    }

    float orthoLeftPlane = pFrustum->orthoLeftPlane;
    if ( rdCamera_g_pCurCamera->projectType == RDCAMERA_PROJECT_PERSPECTIVE )
    {
        orthoLeftPlane = pPoint->y * pFrustum->leftPlane;
    }

    if ( pPoint->x < orthoLeftPlane )
    {
        return 0;
    }

    float orthoRightPlane = pFrustum->orthoRightPlane;
    if ( rdCamera_g_pCurCamera->projectType == RDCAMERA_PROJECT_PERSPECTIVE )
    {
        orthoRightPlane = pPoint->y * pFrustum->rightPlane;
    }

    if ( pPoint->x > orthoRightPlane )
    {
        return 0;
    }

    float orthoTopPlane = pFrustum->orthoTopPlane;
    if ( rdCamera_g_pCurCamera->projectType == RDCAMERA_PROJECT_PERSPECTIVE )
    {
        orthoTopPlane = pPoint->y * pFrustum->topPlane;
    }

    if ( pPoint->z > orthoTopPlane )
    {
        return 0;
    }

    float orthoBottomPlane = pFrustum->orthoBottomPlane;
    if ( rdCamera_g_pCurCamera->projectType == RDCAMERA_PROJECT_PERSPECTIVE )
    {
        orthoBottomPlane = pPoint->y * pFrustum->bottomPlane;
    }

    return pPoint->z >= orthoBottomPlane;
}

int J3DAPI rdClip_ClipFacePVS(rdClipFrustum* pFrustrum, const rdPrimit3* pSrc, rdPrimit3* pDest)
{
    for ( size_t i = 0; i < pSrc->numVertices; ++i )
    {
        rdVector_Copy3(&pDest->aVertices[i], &pSrc->aVertices[pSrc->aVertIdxs[i]]);
    }

    pDest->numVertices = rdClip_Face3WPVS(pFrustrum, pDest->aVertices, pSrc->numVertices);
    return pDest->numVertices;
}

int J3DAPI rdClip_Face3WPVS(rdClipFrustum* pFrustrum, rdVector3* aVertices, size_t numVertices)
{
    // The difference between this and rdClip_Face3W is that this function
    // does not clip the face against the frustrum far plane.

    rdClip_g_faceStatus = 0;
    rdClip_pSourceVert  = aVertices;
    rdClip_pDestVert    = rdClip_aWorkVerts;

    size_t clipindex     = 0;
    rdVector3* pPrevVert = &aVertices[numVertices - 1];
    rdVector3* pCurVert  = aVertices;

    //Start with left clipping
    for ( size_t i = 0; i < numVertices; ++i )
    {
        float prevPlaneProjY = pPrevVert->y * pFrustrum->leftPlane;
        float curPlaneProjY  = pCurVert->y * pFrustrum->leftPlane;
        if ( pPrevVert->x >= (double)prevPlaneProjY || pCurVert->x >= (double)curPlaneProjY )
        {
            if ( pPrevVert->x != prevPlaneProjY
                && pCurVert->x != curPlaneProjY
                && (pPrevVert->x < (double)prevPlaneProjY || pCurVert->x < (double)curPlaneProjY) )
            {
                float planeProj = pFrustrum->leftPlane * (pCurVert->y - pPrevVert->y) - (pCurVert->x - pPrevVert->x);
                float clippedY  = pCurVert->y * pPrevVert->x - pPrevVert->y * pCurVert->x;
                if ( planeProj != 0.0f ) {
                    clippedY = clippedY / planeProj;
                }

                float clippedX = pFrustrum->leftPlane * clippedY;

                float dY = fabsf(pCurVert->y - pPrevVert->y);
                float dX = fabsf(pCurVert->x - pPrevVert->x);
                float lerpFactor = 0.0f;
                if ( dY <= (double)dX )
                {
                    lerpFactor = (clippedX - pPrevVert->x) / (pCurVert->x - pPrevVert->x);
                }
                else
                {
                    lerpFactor = (clippedY - pPrevVert->y) / (pCurVert->y - pPrevVert->y);
                }

                rdClip_pDestVert[clipindex].x = clippedX;
                rdClip_pDestVert[clipindex].y = clippedY;
                rdClip_pDestVert[clipindex].z = (pCurVert->z - pPrevVert->z) * lerpFactor + pPrevVert->z;

                clipindex++;
                rdClip_g_faceStatus |= 0x10;
            }

            if ( pCurVert->x >= (double)curPlaneProjY )
            {
                rdClip_pDestVert[clipindex] = *pCurVert;
                clipindex++;
            }
        }

        RD_ASSERTREL(clipindex < STD_ARRAYLEN(rdClip_aWorkVerts));
        pPrevVert = pCurVert++;
    }

    if ( clipindex < 3 )
    {
        return clipindex;
    }

    // Right clipping
    rdVector3* paCurSrc = rdClip_pDestVert;
    rdClip_pDestVert    = rdClip_pSourceVert;
    rdClip_pSourceVert  = paCurSrc;

    size_t numVerts = clipindex;
    clipindex       = 0;

    pPrevVert       = &paCurSrc[numVerts - 1];
    pCurVert        = paCurSrc;

    for ( size_t i = 0; i < numVerts; ++i )
    {
        float prevPlaneProjY = pPrevVert->y * pFrustrum->rightPlane;
        float curPlaneProjY  = pCurVert->y * pFrustrum->rightPlane;
        if ( pPrevVert->x <= (double)prevPlaneProjY || pCurVert->x <= (double)curPlaneProjY )
        {
            if ( pPrevVert->x != prevPlaneProjY
                && pCurVert->x != curPlaneProjY
                && (pPrevVert->x > (double)prevPlaneProjY || pCurVert->x > (double)curPlaneProjY) )
            {
                float planeProj = pFrustrum->rightPlane * (pCurVert->y - pPrevVert->y) - (pCurVert->x - pPrevVert->x);
                float clippedY  = pCurVert->y * pPrevVert->x - pPrevVert->y * pCurVert->x;
                if ( planeProj != 0.0f ) {
                    clippedY = clippedY / planeProj;
                }

                float clippedX = pFrustrum->rightPlane * clippedY;

                float dY = fabsf(pCurVert->y - pPrevVert->y);
                float dX = fabsf(pCurVert->x - pPrevVert->x);
                float lerpFactor = 0.0;
                if ( dY <= (double)dX )
                {
                    lerpFactor = (clippedX - pPrevVert->x) / (pCurVert->x - pPrevVert->x);
                }
                else
                {
                    lerpFactor = (clippedY - pPrevVert->y) / (pCurVert->y - pPrevVert->y);
                }

                rdClip_pDestVert[clipindex].x = clippedX;
                rdClip_pDestVert[clipindex].y = clippedY;
                rdClip_pDestVert[clipindex].z = (pCurVert->z - pPrevVert->z) * lerpFactor + pPrevVert->z;

                clipindex++;
                rdClip_g_faceStatus |= 0x20;
            }

            if ( pCurVert->x <= (double)curPlaneProjY )
            {
                rdClip_pDestVert[clipindex] = *pCurVert;
                clipindex++;
            }
        }

        RD_ASSERTREL(clipindex < STD_ARRAYLEN(rdClip_aWorkVerts));
        pPrevVert = pCurVert++;
    }

    if ( clipindex < 3 )
    {
        return clipindex;
    }

    //Top Clipping
    paCurSrc           = rdClip_pDestVert;
    rdClip_pDestVert   = rdClip_pSourceVert;
    rdClip_pSourceVert = paCurSrc;

    numVerts  = clipindex;
    clipindex = 0;

    pPrevVert = &paCurSrc[numVerts - 1];
    pCurVert  = paCurSrc;

    for ( size_t i = 0; i < numVerts; ++i )
    {
        float prevPlaneProjY = pPrevVert->y * pFrustrum->topPlane;
        float curPlaneProjY  = pCurVert->y * pFrustrum->topPlane;
        if ( pPrevVert->z <= (double)prevPlaneProjY || pCurVert->z <= (double)curPlaneProjY )
        {
            if ( pPrevVert->z != prevPlaneProjY
                && pCurVert->z != curPlaneProjY
                && (pPrevVert->z > (double)prevPlaneProjY || pCurVert->z > (double)curPlaneProjY) )
            {
                float planeProj = pFrustrum->topPlane * (pCurVert->y - pPrevVert->y) - (pCurVert->z - pPrevVert->z);
                float clippedY  = pPrevVert->z * pCurVert->y - pCurVert->z * pPrevVert->y;
                if ( planeProj != 0.0f ) {
                    clippedY = clippedY / planeProj;
                }

                float clippedZ = pFrustrum->topPlane * clippedY;

                float dY = fabsf(pCurVert->y - pPrevVert->y);
                float dZ = fabsf(pCurVert->z - pPrevVert->z);
                float lerpFactor = 0.0f;
                if ( dY <= (double)dZ )
                {
                    lerpFactor = (clippedZ - pPrevVert->z) / (pCurVert->z - pPrevVert->z);
                }
                else
                {
                    lerpFactor = (clippedY - pPrevVert->y) / (pCurVert->y - pPrevVert->y);
                }

                rdClip_pDestVert[clipindex].x = (pCurVert->x - pPrevVert->x) * lerpFactor + pPrevVert->x;
                rdClip_pDestVert[clipindex].y = clippedY;
                rdClip_pDestVert[clipindex].z = clippedZ;

                clipindex++;
                rdClip_g_faceStatus |= 0x04;
            }

            if ( pCurVert->z <= (double)curPlaneProjY )
            {
                rdClip_pDestVert[clipindex] = *pCurVert;
                clipindex++;
            }
        }

        RD_ASSERTREL(clipindex < STD_ARRAYLEN(rdClip_aWorkVerts));
        pPrevVert = pCurVert++;
    }

    if ( clipindex < 3 )
    {
        return clipindex;
    }

    // Bottom clipping
    paCurSrc           = rdClip_pDestVert;
    rdClip_pDestVert   = rdClip_pSourceVert;
    rdClip_pSourceVert = paCurSrc;

    numVerts  = clipindex;
    clipindex = 0;

    pPrevVert = &paCurSrc[numVerts - 1];
    pCurVert  = paCurSrc;

    for ( size_t i = 0; i < numVerts; ++i )
    {
        float prevPlaneProjY = pPrevVert->y * pFrustrum->bottomPlane;
        float curPlaneProjY  = pCurVert->y * pFrustrum->bottomPlane;
        if ( pPrevVert->z >= (double)prevPlaneProjY || pCurVert->z >= (double)curPlaneProjY )
        {
            if ( pPrevVert->z != prevPlaneProjY
                && pCurVert->z != curPlaneProjY
                && (pPrevVert->z < (double)prevPlaneProjY || pCurVert->z < (double)curPlaneProjY) )
            {
                float planeProj = pFrustrum->bottomPlane * (pCurVert->y - pPrevVert->y) - (pCurVert->z - pPrevVert->z);
                float clippedY  = pPrevVert->z * pCurVert->y - pCurVert->z * pPrevVert->y;;
                if ( planeProj != 0.0f ) {
                    clippedY = clippedY / planeProj;
                }

                float clippedZ = pFrustrum->bottomPlane * clippedY;

                float dY = fabsf(pCurVert->y - pPrevVert->y);
                float dZ = fabsf(pCurVert->z - pPrevVert->z);
                float lerpFactor = 0.0f;
                if ( dY <= (double)dZ )
                {
                    lerpFactor = (clippedZ - pPrevVert->z) / (pCurVert->z - pPrevVert->z);
                }
                else
                {
                    lerpFactor = (clippedY - pPrevVert->y) / (pCurVert->y - pPrevVert->y);
                }

                rdClip_pDestVert[clipindex].x = (pCurVert->x - pPrevVert->x) * lerpFactor + pPrevVert->x;
                rdClip_pDestVert[clipindex].y = clippedY;
                rdClip_pDestVert[clipindex].z = clippedZ;

                clipindex++;
                rdClip_g_faceStatus |= 0x08;
            }

            if ( pCurVert->z >= (double)curPlaneProjY )
            {
                rdClip_pDestVert[clipindex] = *pCurVert;
                clipindex++;
            }
        }

        RD_ASSERTREL(clipindex < STD_ARRAYLEN(rdClip_aWorkVerts));
        pPrevVert = pCurVert++;
    }

    if ( clipindex < 3 )
    {
        return clipindex;
    }

    // Now do near clipping
    paCurSrc = rdClip_pDestVert;
    rdClip_pDestVert   = rdClip_pSourceVert;
    rdClip_pSourceVert = paCurSrc;

    numVerts  = clipindex;
    clipindex = 0;

    pPrevVert = &paCurSrc[numVerts - 1];
    pCurVert  = paCurSrc;

    for ( size_t i = 0; i < numVerts; ++i )
    {
        if ( pPrevVert->y >= (double)pFrustrum->nearPlane || pCurVert->y >= (double)pFrustrum->nearPlane )
        {
            if ( pPrevVert->y != pFrustrum->nearPlane
                && pCurVert->y != pFrustrum->nearPlane
                && (pPrevVert->y < (double)pFrustrum->nearPlane || pCurVert->y < (double)pFrustrum->nearPlane) )
            {
                float planeProj = (pFrustrum->nearPlane - pPrevVert->y) / (pCurVert->y - pPrevVert->y);
                rdClip_pDestVert[clipindex].y = pFrustrum->nearPlane;
                rdClip_pDestVert[clipindex].z = (pCurVert->z - pPrevVert->z) * planeProj + pPrevVert->z;
                rdClip_pDestVert[clipindex].x = (pCurVert->x - pPrevVert->x) * planeProj + pPrevVert->x;

                clipindex++;
                rdClip_g_faceStatus |= 0x01;
            }

            if ( pCurVert->y >= (double)pFrustrum->nearPlane )
            {
                rdClip_pDestVert[clipindex] = *pCurVert;
                clipindex++;
            }
        }

        RD_ASSERTREL(clipindex < STD_ARRAYLEN(rdClip_aWorkVerts));
        pPrevVert = pCurVert++;
    }

    if ( clipindex < 3 )
    {
        rdClip_g_faceStatus |= 0x41;
        return clipindex;
    }

    if ( rdClip_pDestVert != aVertices )
    {
        rdVector_Copy3List(aVertices, rdClip_pDestVert, clipindex);
    }
    return clipindex;
}

int J3DAPI rdClip_Face3W(const rdClipFrustum* pFrustrum, rdVector3* aVertices, size_t numVertices)
{
   // Do left plane clipping
    rdClip_g_faceStatus = 0;
    rdClip_pSourceVert  = aVertices;
    rdClip_pDestVert    = rdClip_aWorkVerts;

    size_t clipindex     = 0;
    rdVector3* pPrevVert = &aVertices[numVertices - 1];
    rdVector3* pCurVert  = aVertices;

    for ( size_t i = 0; i < numVertices; ++i )
    {
        float prevPlaneProjY = pPrevVert->y * pFrustrum->leftPlane;
        float curPlaneProyY  = pCurVert->y * pFrustrum->leftPlane;
        if ( pPrevVert->x >= (double)prevPlaneProjY || pCurVert->x >= (double)curPlaneProyY )
        {
            if ( pPrevVert->x != prevPlaneProjY
                && pCurVert->x != curPlaneProyY
                && (pPrevVert->x < (double)prevPlaneProjY || pCurVert->x < (double)curPlaneProyY) )
            {
                float planeProj = pFrustrum->leftPlane * (pCurVert->y - pPrevVert->y) - (pCurVert->x - pPrevVert->x);
                float clippedY  = pCurVert->y * pPrevVert->x - pPrevVert->y * pCurVert->x;
                if ( planeProj != 0.0f )
                {
                    clippedY = clippedY / planeProj;
                }

                float clippedX = pFrustrum->leftPlane * clippedY;

                float dY = fabsf(pCurVert->y - pPrevVert->y);
                float dX = fabsf(pCurVert->x - pPrevVert->x);

                float lerpFactor = 0.0f;
                if ( dY <= (double)dX )
                {
                    lerpFactor = (clippedX - pPrevVert->x) / (pCurVert->x - pPrevVert->x);
                }
                else
                {
                    lerpFactor = (clippedY - pPrevVert->y) / (pCurVert->y - pPrevVert->y);
                }

                rdClip_pDestVert[clipindex].x = clippedX;
                rdClip_pDestVert[clipindex].y = clippedY;
                rdClip_pDestVert[clipindex].z = (pCurVert->z - pPrevVert->z) * lerpFactor + pPrevVert->z;

                clipindex++;
                rdClip_g_faceStatus |= 0x10;
            }

            if ( pCurVert->x >= (double)curPlaneProyY )
            {
                rdClip_pDestVert[clipindex] = *pCurVert;
                clipindex++;
            }
        }

        RD_ASSERTREL(clipindex < STD_ARRAYLEN(rdClip_aWorkVerts));
        pPrevVert = pCurVert++;
    }

    if ( clipindex < 3 )
    {
        return clipindex;
    }

    //Do right plane clipping
    rdVector3* paVerts = rdClip_pDestVert;
    rdClip_pDestVert   = rdClip_pSourceVert;
    rdClip_pSourceVert = paVerts;

    size_t numVerts = clipindex;
    clipindex       = 0;

    pPrevVert       = &paVerts[numVerts - 1];
    pCurVert        = paVerts;

    for ( size_t i = 0; i < numVerts; ++i )
    {
        float prevPlaneProjY = pPrevVert->y * pFrustrum->rightPlane;
        float curPlaneProyY  = pCurVert->y * pFrustrum->rightPlane;
        if ( pPrevVert->x <= (double)prevPlaneProjY || pCurVert->x <= (double)curPlaneProyY )
        {
            if ( pPrevVert->x != prevPlaneProjY
                && pCurVert->x != curPlaneProyY
                && (pPrevVert->x > (double)prevPlaneProjY || pCurVert->x > (double)curPlaneProyY) )
            {
                float planeProj = pFrustrum->rightPlane * (pCurVert->y - pPrevVert->y) - (pCurVert->x - pPrevVert->x);
                float clippedY = pCurVert->y * pPrevVert->x - pPrevVert->y * pCurVert->x;
                if ( planeProj != 0.0f )
                {
                    clippedY = clippedY / planeProj;
                }

                float clippedX = pFrustrum->rightPlane * clippedY;

                float dY = fabsf(pCurVert->y - pPrevVert->y);
                float dX = fabsf(pCurVert->x - pPrevVert->x);

                float lerpFactor = 0.0f;
                if ( dY <= (double)dX )
                {
                    lerpFactor = (clippedX - pPrevVert->x) / (pCurVert->x - pPrevVert->x);
                }
                else
                {
                    lerpFactor = (clippedY - pPrevVert->y) / (pCurVert->y - pPrevVert->y);
                }

                rdClip_pDestVert[clipindex].x = clippedX;
                rdClip_pDestVert[clipindex].y = clippedY;
                rdClip_pDestVert[clipindex].z = (pCurVert->z - pPrevVert->z) * lerpFactor + pPrevVert->z;

                clipindex++;
                rdClip_g_faceStatus |= 0x20;
            }

            if ( pCurVert->x <= (double)curPlaneProyY )
            {
                rdClip_pDestVert[clipindex] = *pCurVert;
                clipindex++;
            }
        }

        RD_ASSERTREL(clipindex < STD_ARRAYLEN(rdClip_aWorkVerts));
        pPrevVert = pCurVert++;
    }

    if ( clipindex < 3 )
    {
        return clipindex;
    }

    //Do top plane clipping
    paVerts = rdClip_pDestVert;
    rdClip_pDestVert   = rdClip_pSourceVert;
    rdClip_pSourceVert = paVerts;

    numVerts  = clipindex;
    clipindex = 0;

    pPrevVert = &paVerts[numVerts - 1];
    pCurVert  = paVerts;

    for ( size_t i = 0; i < numVerts; ++i )
    {
        float prevPlaneProjY = pPrevVert->y * pFrustrum->topPlane;
        float curPlaneProyY  = pCurVert->y * pFrustrum->topPlane;

        if ( pPrevVert->z <= (double)prevPlaneProjY || pCurVert->z <= (double)curPlaneProyY )
        {
            if ( pPrevVert->z != prevPlaneProjY
                && pCurVert->z != curPlaneProyY
                && (pPrevVert->z > (double)prevPlaneProjY || pCurVert->z > (double)curPlaneProyY) )
            {
                float  planeProj = pFrustrum->topPlane * (pCurVert->y - pPrevVert->y) - (pCurVert->z - pPrevVert->z);
                float clippedY   = pPrevVert->z * pCurVert->y - pCurVert->z * pPrevVert->y;
                if ( planeProj != 0.0f )
                {
                    clippedY = clippedY / planeProj;
                }

                float clippedZ = pFrustrum->topPlane * clippedY;

                float dY = fabsf(pCurVert->y - pPrevVert->y);
                float dZ = fabsf(pCurVert->z - pPrevVert->z);

                float lerpFactor = 0.0f;
                if ( dY <= (double)dZ )
                {
                    lerpFactor = (clippedZ - pPrevVert->z) / (pCurVert->z - pPrevVert->z);
                }
                else
                {
                    lerpFactor = (clippedY - pPrevVert->y) / (pCurVert->y - pPrevVert->y);
                }

                rdClip_pDestVert[clipindex].x = (pCurVert->x - pPrevVert->x) * lerpFactor + pPrevVert->x;
                rdClip_pDestVert[clipindex].y = clippedY;
                rdClip_pDestVert[clipindex].z = clippedZ;

                clipindex++;
                rdClip_g_faceStatus |= 0x04;
            }

            if ( pCurVert->z <= (double)curPlaneProyY )
            {
                rdClip_pDestVert[clipindex] = *pCurVert;
                clipindex++;
            }
        }

        RD_ASSERTREL(clipindex < STD_ARRAYLEN(rdClip_aWorkVerts));
        pPrevVert = pCurVert++;
    }

    if ( clipindex < 3 )
    {
        return clipindex;
    }

   // Do bottom plane clipping
    paVerts = rdClip_pDestVert;
    rdClip_pDestVert   = rdClip_pSourceVert;
    rdClip_pSourceVert = paVerts;

    numVerts  = clipindex;
    clipindex = 0;

    pPrevVert = &paVerts[numVerts - 1];
    pCurVert  = paVerts;

    for ( size_t i = 0; i < numVerts; ++i )
    {
        float prevPlaneProjY = pPrevVert->y * pFrustrum->bottomPlane;
        float curPlaneProyY  = pCurVert->y * pFrustrum->bottomPlane;

        if ( pPrevVert->z >= (double)prevPlaneProjY || pCurVert->z >= (double)curPlaneProyY )
        {
            if ( pPrevVert->z != prevPlaneProjY
                && pCurVert->z != curPlaneProyY
                && (pPrevVert->z < (double)prevPlaneProjY || pCurVert->z < (double)curPlaneProyY) )
            {
                float planeProj = pFrustrum->bottomPlane * (pCurVert->y - pPrevVert->y) - (pCurVert->z - pPrevVert->z);
                float clippedY  = pPrevVert->z * pCurVert->y - pCurVert->z * pPrevVert->y;
                if ( planeProj != 0.0f )
                {
                    clippedY = clippedY / planeProj;
                }

                float clippedZ = pFrustrum->bottomPlane * clippedY;

                float dY = fabsf(pCurVert->y - pPrevVert->y);
                float dZ = fabsf(pCurVert->z - pPrevVert->z);

                float lerpFactor = 0.0f;
                if ( dY <= (double)dZ )
                {
                    lerpFactor = (clippedZ - pPrevVert->z) / (pCurVert->z - pPrevVert->z);
                }
                else
                {
                    lerpFactor = (clippedY - pPrevVert->y) / (pCurVert->y - pPrevVert->y);
                }

                rdClip_pDestVert[clipindex].x = (pCurVert->x - pPrevVert->x) * lerpFactor + pPrevVert->x;
                rdClip_pDestVert[clipindex].y = clippedY;
                rdClip_pDestVert[clipindex].z = clippedZ;

                clipindex++;
                rdClip_g_faceStatus |= 0x08;
            }

            if ( pCurVert->z >= (double)curPlaneProyY )
            {
                rdClip_pDestVert[clipindex] = *pCurVert;
                clipindex++;
            }
        }

        RD_ASSERTREL(clipindex < STD_ARRAYLEN(rdClip_aWorkVerts));
        pPrevVert = pCurVert++;
    }

    if ( clipindex < 3 )
    {
        return clipindex;
    }

    // Do near / far plane clipping
    paVerts = rdClip_pDestVert;
    rdClip_pDestVert   = rdClip_pSourceVert;
    rdClip_pSourceVert = paVerts;

    numVerts  = clipindex;
    clipindex = 0;

    pPrevVert = &paVerts[numVerts - 1];
    pCurVert  = paVerts;

    for ( size_t i = 0; i < numVerts; ++i )
    {
        if ( pPrevVert->y >= (double)pFrustrum->nearPlane || pCurVert->y >= (double)pFrustrum->nearPlane )
        {
            if ( pPrevVert->y != pFrustrum->nearPlane
                && pCurVert->y != pFrustrum->nearPlane
                && (pPrevVert->y < (double)pFrustrum->nearPlane || pCurVert->y < (double)pFrustrum->nearPlane) )
            {
                float lerpFactor = (pFrustrum->nearPlane - pPrevVert->y) / (pCurVert->y - pPrevVert->y);
                rdClip_pDestVert[clipindex].y = pFrustrum->nearPlane;
                rdClip_pDestVert[clipindex].z = (pCurVert->z - pPrevVert->z) * lerpFactor + pPrevVert->z;
                rdClip_pDestVert[clipindex].x = (pCurVert->x - pPrevVert->x) * lerpFactor + pPrevVert->x;

                clipindex++;
                rdClip_g_faceStatus |= 0x01;
            }

            if ( pCurVert->y >= (double)pFrustrum->nearPlane )
            {
                rdClip_pDestVert[clipindex] = *pCurVert;
                clipindex++;
            }
        }

        RD_ASSERTREL(clipindex < STD_ARRAYLEN(rdClip_aWorkVerts));
        pPrevVert = pCurVert++;
    }

    if ( clipindex < 3 )
    {
        rdClip_g_faceStatus |= 0x40;
        return clipindex;
    }

    if ( !pFrustrum->bClipFar )
    {
        if ( rdClip_pDestVert != aVertices )
        {
            rdVector_Copy3List(aVertices, rdClip_pDestVert, clipindex);
        }

        return clipindex;
    }

    paVerts = rdClip_pDestVert;
    rdClip_pDestVert   = rdClip_pSourceVert;
    rdClip_pSourceVert = paVerts;

    numVerts  = clipindex;
    clipindex = 0;

    pPrevVert = &paVerts[numVerts - 1];
    pCurVert  = paVerts;

    for ( size_t i = 0; i < numVerts; ++i )
    {
        if ( pPrevVert->y <= (double)pFrustrum->farPlane || pCurVert->y <= (double)pFrustrum->farPlane )
        {
            if ( pPrevVert->y != pFrustrum->farPlane
                && pCurVert->y != pFrustrum->farPlane
                && (pPrevVert->y > (double)pFrustrum->farPlane || pCurVert->y > (double)pFrustrum->farPlane) )
            {
                float lerpFactor = (pFrustrum->farPlane - pPrevVert->y) / (pCurVert->y - pPrevVert->y);
                rdClip_pDestVert[clipindex].y = pFrustrum->farPlane;
                rdClip_pDestVert[clipindex].z = (pCurVert->z - pPrevVert->z) * lerpFactor + pPrevVert->z;
                rdClip_pDestVert[clipindex].x = (pCurVert->x - pPrevVert->x) * lerpFactor + pPrevVert->x;

                clipindex++;
                rdClip_g_faceStatus |= 0x02;
            }

            if ( pCurVert->y <= (double)pFrustrum->farPlane )
            {
                rdClip_pDestVert[clipindex] = *pCurVert;
                clipindex++;
            }
        }

        RD_ASSERTREL(clipindex < STD_ARRAYLEN(rdClip_aWorkVerts));
        pPrevVert = pCurVert++;
    }

    if ( clipindex < 3 )
    {
        return clipindex;
    }

    if ( rdClip_pDestVert != aVertices )
    {
        rdVector_Copy3List(aVertices, rdClip_pDestVert, clipindex);
    }

    return clipindex;
}

int J3DAPI rdClip_Face3WOrtho(const rdClipFrustum* pFrustrum, rdVector3* aVertices, size_t numVertices)
{
    rdClip_g_faceStatus = 0;
    rdClip_pSourceVert  = aVertices;
    rdClip_pDestVert    = rdClip_aWorkVerts;

    size_t clipindex     = 0;
    rdVector3* pPrevVert = &aVertices[numVertices - 1];
    rdVector3* pCurVert  = aVertices;

    for ( size_t i = 0; i < numVertices; ++i )
    {
        if ( pPrevVert->x >= pFrustrum->orthoLeftPlane || pCurVert->x >= pFrustrum->orthoLeftPlane )
        {
            if ( pPrevVert->x != pFrustrum->orthoLeftPlane
                && pCurVert->x != pFrustrum->orthoLeftPlane
                && (pPrevVert->x < pFrustrum->orthoLeftPlane || pCurVert->x < pFrustrum->orthoLeftPlane) )
            {
                float lerpFactor = (pFrustrum->orthoLeftPlane - pPrevVert->x) / (pCurVert->x - pPrevVert->x);
                rdClip_pDestVert[clipindex].x   = pFrustrum->orthoLeftPlane;
                rdClip_pDestVert[clipindex].y   = (pCurVert->y - pPrevVert->y) * lerpFactor + pPrevVert->y;
                rdClip_pDestVert[clipindex++].z = (pCurVert->z - pPrevVert->z) * lerpFactor + pPrevVert->z;

                rdClip_g_faceStatus |= 0x10;
            }

            if ( pCurVert->x >= pFrustrum->orthoLeftPlane )
            {
                rdVector_Copy3(&rdClip_pDestVert[clipindex++], pCurVert);
            }
        }

        RD_ASSERTREL(clipindex < STD_ARRAYLEN(rdClip_aWorkVerts));
        pPrevVert = pCurVert++;
    }

    if ( clipindex < 3 )
    {
        return clipindex;
    }

    rdVector3* paVerts = rdClip_pDestVert;
    rdClip_pDestVert   = rdClip_pSourceVert;
    rdClip_pSourceVert = paVerts;

    size_t numVerts = clipindex;
    clipindex       = 0;
    pPrevVert       = &paVerts[numVerts - 1];
    pCurVert        = paVerts;

    for ( size_t i = 0; i < numVerts; ++i )
    {
        if ( pPrevVert->x <= pFrustrum->orthoRightPlane || pCurVert->x <= pFrustrum->orthoRightPlane )
        {
            if ( pPrevVert->x != pFrustrum->orthoRightPlane
                && pCurVert->x != pFrustrum->orthoRightPlane
                && (pPrevVert->x > pFrustrum->orthoRightPlane || pCurVert->x > pFrustrum->orthoRightPlane) )
            {
                float lerpFactor = (pFrustrum->orthoRightPlane - pPrevVert->x) / (pCurVert->x - pPrevVert->x);
                rdClip_pDestVert[clipindex].x   =  pFrustrum->orthoRightPlane;
                rdClip_pDestVert[clipindex].y   = (pCurVert->y - pPrevVert->y) * lerpFactor + pPrevVert->y;
                rdClip_pDestVert[clipindex++].z = (pCurVert->z - pPrevVert->z) * lerpFactor + pPrevVert->z;

                rdClip_g_faceStatus |= 0x20;
            }

            if ( pCurVert->x <= pFrustrum->orthoRightPlane )
            {
                rdVector_Copy3(&rdClip_pDestVert[clipindex++], pCurVert);
            }
        }

        RD_ASSERTREL(clipindex < STD_ARRAYLEN(rdClip_aWorkVerts));
        pPrevVert = pCurVert++;
    }

    if ( clipindex < 3 )
    {
        return clipindex;
    }

    paVerts = rdClip_pDestVert;
    rdClip_pDestVert   = rdClip_pSourceVert;
    rdClip_pSourceVert = paVerts;

    numVerts  = clipindex;
    clipindex = 0;
    pPrevVert = &paVerts[numVerts - 1];
    pCurVert  = paVerts;

    for ( size_t i = 0; i < numVerts; ++i )
    {
        if ( pPrevVert->z <= pFrustrum->orthoTopPlane || pCurVert->z <= pFrustrum->orthoTopPlane )
        {
            if ( pPrevVert->z != pFrustrum->orthoTopPlane && pCurVert->z != pFrustrum->orthoTopPlane && (pPrevVert->z > pFrustrum->orthoTopPlane || pCurVert->z > pFrustrum->orthoTopPlane) )
            {
                float lerpFactor = (pFrustrum->orthoTopPlane - pPrevVert->z) / (pCurVert->z - pPrevVert->z);
                rdClip_pDestVert[clipindex].x   = (pCurVert->x - pPrevVert->x) * lerpFactor + pPrevVert->x;
                rdClip_pDestVert[clipindex].y   = (pCurVert->y - pPrevVert->y) * lerpFactor + pPrevVert->y;
                rdClip_pDestVert[clipindex++].z = pFrustrum->orthoTopPlane;

                rdClip_g_faceStatus |= 0x04;
            }

            if ( pCurVert->z <= pFrustrum->orthoTopPlane )
            {
                rdVector_Copy3(&rdClip_pDestVert[clipindex++], pCurVert);
            }
        }

        RD_ASSERTREL(clipindex < STD_ARRAYLEN(rdClip_aWorkVerts));
        pPrevVert = pCurVert++;
    }

    if ( clipindex < 3 )
    {
        return clipindex;
    }

    paVerts            = rdClip_pDestVert;
    rdClip_pDestVert   = rdClip_pSourceVert;
    rdClip_pSourceVert = paVerts;

    numVerts  = clipindex;
    clipindex = 0;
    pPrevVert = &paVerts[numVerts - 1];
    pCurVert  = paVerts;

    for ( size_t i = 0; i < numVerts; ++i )
    {
        if ( pPrevVert->z >= pFrustrum->orthoBottomPlane || pCurVert->z >= pFrustrum->orthoBottomPlane )
        {
            if ( pPrevVert->z != pFrustrum->orthoBottomPlane
                && pCurVert->z != pFrustrum->orthoBottomPlane
                && (pPrevVert->z < pFrustrum->orthoBottomPlane || pCurVert->z < pFrustrum->orthoBottomPlane) )
            {
                float lerpFactor = (pFrustrum->orthoBottomPlane - pPrevVert->z) / (pCurVert->z - pPrevVert->z);
                rdClip_pDestVert[clipindex].x   = (pCurVert->x - pPrevVert->x) * lerpFactor + pPrevVert->x;
                rdClip_pDestVert[clipindex].y   = (pCurVert->y - pPrevVert->y) * lerpFactor + pPrevVert->y;
                rdClip_pDestVert[clipindex++].z = pFrustrum->orthoBottomPlane;

                rdClip_g_faceStatus |= 0x08;
            }

            if ( pCurVert->z >= pFrustrum->orthoBottomPlane )
            {
                rdVector_Copy3(&rdClip_pDestVert[clipindex++], pCurVert);
            }
        }

        RD_ASSERTREL(clipindex < STD_ARRAYLEN(rdClip_aWorkVerts));
        pPrevVert = pCurVert++;
    }

    if ( clipindex < 3 )
    {
        return clipindex;
    }

    paVerts            = rdClip_pDestVert;
    rdClip_pDestVert   = rdClip_pSourceVert;
    rdClip_pSourceVert = paVerts;

    numVerts  = clipindex;
    clipindex = 0;
    pPrevVert = &paVerts[numVerts - 1];
    pCurVert  = paVerts;

    for ( size_t i = 0; i < numVerts; ++i )
    {
        if ( pPrevVert->y >= pFrustrum->nearPlane || pCurVert->y >= pFrustrum->nearPlane )
        {
            if ( pPrevVert->y != pFrustrum->nearPlane
                && pCurVert->y != pFrustrum->nearPlane
                && (pPrevVert->y < pFrustrum->nearPlane || pCurVert->y < pFrustrum->nearPlane) )
            {
                float lerpFactor = (pFrustrum->nearPlane - pPrevVert->y) / (pCurVert->y - pPrevVert->y);
                rdClip_pDestVert[clipindex].y = pFrustrum->nearPlane;
                rdClip_pDestVert[clipindex].z = (pCurVert->z - pPrevVert->z) * lerpFactor + pPrevVert->z;
                rdClip_pDestVert[clipindex++].x = (pCurVert->x - pPrevVert->x) * lerpFactor + pPrevVert->x;

                rdClip_g_faceStatus |= 0x01;
            }

            if ( pCurVert->y >= pFrustrum->nearPlane )
            {
                rdVector_Copy3(&rdClip_pDestVert[clipindex++], pCurVert);
            }
        }

        RD_ASSERTREL(clipindex < STD_ARRAYLEN(rdClip_aWorkVerts));
        pPrevVert = pCurVert++;
    }

    if ( clipindex < 3 )
    {
        rdClip_g_faceStatus |= 0x40;
        return clipindex;
    }

    if ( !pFrustrum->bClipFar )
    {
        if ( rdClip_pDestVert != aVertices )
        {
            rdVector_Copy3List(aVertices, rdClip_pDestVert, clipindex);
        }

        return clipindex;
    }

    paVerts = rdClip_pDestVert;
    rdClip_pDestVert   = rdClip_pSourceVert;
    rdClip_pSourceVert = paVerts;

    numVerts  = clipindex;
    clipindex = 0;
    pPrevVert = &paVerts[numVerts - 1];
    pCurVert  = paVerts;

    for ( size_t i = 0; i < numVerts; ++i )
    {
        if ( pPrevVert->y <= pFrustrum->farPlane || pCurVert->y <= pFrustrum->farPlane )
        {
            if ( pPrevVert->y != pFrustrum->farPlane
                && pCurVert->y != pFrustrum->farPlane
                && (pPrevVert->y > pFrustrum->farPlane || pCurVert->y > pFrustrum->farPlane) )
            {
                float lerpFactor = (pFrustrum->farPlane - pPrevVert->y) / (pCurVert->y - pPrevVert->y);
                rdClip_pDestVert[clipindex].y   = pFrustrum->farPlane;
                rdClip_pDestVert[clipindex].z   = (pCurVert->z - pPrevVert->z) * lerpFactor + pPrevVert->z;
                rdClip_pDestVert[clipindex++].x = (pCurVert->x - pPrevVert->x) * lerpFactor + pPrevVert->x;

                rdClip_g_faceStatus |= 0x02;
            }

            if ( pCurVert->y <= pFrustrum->farPlane )
            {
                rdVector_Copy3(&rdClip_pDestVert[clipindex++], pCurVert);
            }
        }

        RD_ASSERTREL(clipindex < STD_ARRAYLEN(rdClip_aWorkVerts));
        pPrevVert = pCurVert++;
    }

    if ( clipindex < 3 )
    {
        return clipindex;
    }

    if ( rdClip_pDestVert != aVertices )
    {
        rdVector_Copy3List(aVertices, rdClip_pDestVert, clipindex);
    }

    return clipindex;
}

int J3DAPI rdClip_Face3S(const rdClipFrustum* pFrustrum, rdVector3* aVertices, size_t numVertices)
{
    //Note, the code was simplified to omit boilerplate code
    return rdClip_Face3W(pFrustrum, aVertices, numVertices);
}

int J3DAPI rdClip_Face3SOrtho(const rdClipFrustum* pFrustum, rdVector3* aVertices, size_t numVertices)
{
    //Note, the code was simplified to omit boilerplate code
    return rdClip_Face3WOrtho(pFrustum, aVertices, numVertices);
}

int J3DAPI rdClip_Face3GS(const rdClipFrustum* pFrustrum, rdVector3* aVertices, rdVector4* aIntensities, size_t numVertices)
{
    rdClip_g_faceStatus         = 0;
    rdClip_pSourceVert          = aVertices;
    rdClip_pSourceVertIntensity = aIntensities;
    rdClip_pDestVert            = rdClip_aWorkVerts;
    rdClip_pDestVertIntensity   = rdClip_aWorkVertIntensities;

    size_t clipindex       = 0;
    rdVector3* pPrevVert   = &aVertices[numVertices - 1];
    rdVector4* pPrevIntens = &aIntensities[numVertices - 1];
    rdVector3* pCurVert    = aVertices;
    rdVector4* pCurIntens  = aIntensities;

    for ( size_t i = 0; i < numVertices; ++i )
    {
        float prevPlaneProjY = pPrevVert->y * pFrustrum->leftPlane;
        float curPlaneProjY = pCurVert->y * pFrustrum->leftPlane;
        if ( pPrevVert->x >= prevPlaneProjY || pCurVert->x >= curPlaneProjY )
        {
            if ( pPrevVert->x != prevPlaneProjY
                && pCurVert->x != curPlaneProjY
                && (pPrevVert->x < prevPlaneProjY || pCurVert->x < curPlaneProjY) )
            {
                float planeProj = pFrustrum->leftPlane * (pCurVert->y - pPrevVert->y) - (pCurVert->x - pPrevVert->x);
                float clippedY  = pCurVert->y * pPrevVert->x - pPrevVert->y * pCurVert->x;
                if ( planeProj != 0.0f ) {
                    clippedY = clippedY / planeProj;
                }

                float clippedX = pFrustrum->leftPlane * clippedY;

                float dY = fabsf(pCurVert->y - pPrevVert->y);
                float dX = fabsf(pCurVert->x - pPrevVert->x);

                float lerpFactor = 0.0f;
                if ( dY <= dX )
                {
                    lerpFactor = (clippedX - pPrevVert->x) / (pCurVert->x - pPrevVert->x);
                }
                else
                {
                    lerpFactor = (clippedY - pPrevVert->y) / (pCurVert->y - pPrevVert->y);
                }

                rdClip_pDestVert[clipindex].x = clippedX;
                rdClip_pDestVert[clipindex].y = clippedY;
                rdClip_pDestVert[clipindex].z = (pCurVert->z - pPrevVert->z) * lerpFactor + pPrevVert->z;

                rdClip_pDestVertIntensity[clipindex].x   = (pCurIntens->x - pPrevIntens->x) * lerpFactor + pPrevIntens->x;
                rdClip_pDestVertIntensity[clipindex].y   = (pCurIntens->y - pPrevIntens->y) * lerpFactor + pPrevIntens->y;
                rdClip_pDestVertIntensity[clipindex++].z = (pCurIntens->z - pPrevIntens->z) * lerpFactor + pPrevIntens->z;

                rdClip_g_faceStatus |= 0x10;
            }

            if ( pCurVert->x >= curPlaneProjY )
            {
                rdVector_Copy3(&rdClip_pDestVert[clipindex], pCurVert);
                rdVector_Copy4(&rdClip_pDestVertIntensity[clipindex++], pCurIntens);
            }
        }

        RD_ASSERTREL(clipindex < STD_ARRAYLEN(rdClip_aWorkVerts));
        pPrevVert   = pCurVert++;
        pPrevIntens = pCurIntens++;
    }

    if ( clipindex < 3 )
    {
        return clipindex;
    }

    rdVector3* paVerts = rdClip_pDestVert;
    rdClip_pDestVert   = rdClip_pSourceVert;
    rdClip_pSourceVert = paVerts;

    rdVector4* paIntens         = rdClip_pDestVertIntensity;
    rdClip_pDestVertIntensity   = rdClip_pSourceVertIntensity;
    rdClip_pSourceVertIntensity = paIntens;

    size_t numVerts = clipindex;
    clipindex       = 0;
    pPrevVert       = &paVerts[numVerts - 1];
    pPrevIntens     = &paIntens[numVerts - 1];
    pCurVert        = paVerts;
    pCurIntens      = paIntens;

    for ( size_t i = 0; i < numVerts; ++i )
    {
        float prevPlaneProjY = pPrevVert->y * pFrustrum->rightPlane;
        float curPlaneProjY = pCurVert->y * pFrustrum->rightPlane;
        if ( pPrevVert->x <= prevPlaneProjY || pCurVert->x <= curPlaneProjY )
        {
            if ( pPrevVert->x != prevPlaneProjY
                && pCurVert->x != curPlaneProjY
                && (pPrevVert->x > prevPlaneProjY || pCurVert->x > curPlaneProjY) )
            {
                float planeProj = pFrustrum->rightPlane * (pCurVert->y - pPrevVert->y) - (pCurVert->x - pPrevVert->x);
                float clippedY  = pCurVert->y * pPrevVert->x - pPrevVert->y * pCurVert->x;
                if ( planeProj != 0.0f )
                {
                    clippedY = clippedY / planeProj;
                }

                float clippedX = pFrustrum->rightPlane * clippedY;

                float dY = fabsf(pCurVert->y - pPrevVert->y);
                float dX = fabsf(pCurVert->x - pPrevVert->x);

                float lerpFactor = 0.0f;
                if ( dY <= dX )
                {
                    lerpFactor = (clippedX - pPrevVert->x) / (pCurVert->x - pPrevVert->x);
                }
                else
                {
                    lerpFactor = (clippedY - pPrevVert->y) / (pCurVert->y - pPrevVert->y);
                }

                rdClip_pDestVert[clipindex].x = clippedX;
                rdClip_pDestVert[clipindex].y = clippedY;
                rdClip_pDestVert[clipindex].z = (pCurVert->z - pPrevVert->z) * lerpFactor + pPrevVert->z;

                rdClip_pDestVertIntensity[clipindex].x   = (pCurIntens->x - pPrevIntens->x) * lerpFactor + pPrevIntens->x;
                rdClip_pDestVertIntensity[clipindex].y   = (pCurIntens->y - pPrevIntens->y) * lerpFactor + pPrevIntens->y;
                rdClip_pDestVertIntensity[clipindex++].z = (pCurIntens->z - pPrevIntens->z) * lerpFactor + pPrevIntens->z;

                rdClip_g_faceStatus |= 0x20;
            }

            if ( pCurVert->x <= curPlaneProjY )
            {
                rdVector_Copy3(&rdClip_pDestVert[clipindex], pCurVert);
                rdVector_Copy4(&rdClip_pDestVertIntensity[clipindex++], pCurIntens);
            }
        }

        RD_ASSERTREL(clipindex < STD_ARRAYLEN(rdClip_aWorkVerts));
        pPrevVert   = pCurVert++;
        pPrevIntens = pCurIntens++;
    }

    if ( clipindex < 3 )
    {
        return clipindex;
    }

    paVerts            = rdClip_pDestVert;
    rdClip_pDestVert   = rdClip_pSourceVert;
    rdClip_pSourceVert = paVerts;

    paIntens                    = rdClip_pDestVertIntensity;
    rdClip_pDestVertIntensity   = rdClip_pSourceVertIntensity;
    rdClip_pSourceVertIntensity = paIntens;

    numVerts    = clipindex;
    clipindex   = 0;
    pPrevVert   = &paVerts[numVerts - 1];
    pPrevIntens = &paIntens[numVerts - 1];
    pCurVert    = paVerts;
    pCurIntens  = paIntens;

    for ( size_t i = 0; i < numVerts; ++i )
    {
        float prevPlaneProjY = pPrevVert->y * pFrustrum->topPlane;
        float curPlaneProjY = pCurVert->y * pFrustrum->topPlane;
        if ( pPrevVert->z <= prevPlaneProjY || pCurVert->z <= curPlaneProjY )
        {
            if ( pPrevVert->z != prevPlaneProjY
                && pCurVert->z != curPlaneProjY
                && (pPrevVert->z > prevPlaneProjY || pCurVert->z > curPlaneProjY) )
            {
                float planeProj = pFrustrum->topPlane * (pCurVert->y - pPrevVert->y) - (pCurVert->z - pPrevVert->z);
                float clippedY  = pPrevVert->z * pCurVert->y - pCurVert->z * pPrevVert->y;;
                if ( planeProj != 0.0f ) {
                    clippedY = clippedY / planeProj;
                }

                float clippedZ = pFrustrum->topPlane * clippedY;

                float dY = fabsf(pCurVert->y - pPrevVert->y);
                float dZ = fabsf(pCurVert->z - pPrevVert->z);

                float lerpFactor = 0.0f;
                if ( dY <= dZ )
                {
                    lerpFactor = (clippedZ - pPrevVert->z) / (pCurVert->z - pPrevVert->z);
                }
                else
                {
                    lerpFactor = (clippedY - pPrevVert->y) / (pCurVert->y - pPrevVert->y);
                }

                rdClip_pDestVert[clipindex].x = (pCurVert->x - pPrevVert->x) * lerpFactor + pPrevVert->x;
                rdClip_pDestVert[clipindex].y = clippedY;
                rdClip_pDestVert[clipindex].z = clippedZ;

                rdClip_pDestVertIntensity[clipindex].x   = (pCurIntens->x - pPrevIntens->x) * lerpFactor + pPrevIntens->x;
                rdClip_pDestVertIntensity[clipindex].y   = (pCurIntens->y - pPrevIntens->y) * lerpFactor + pPrevIntens->y;
                rdClip_pDestVertIntensity[clipindex++].z = (pCurIntens->z - pPrevIntens->z) * lerpFactor + pPrevIntens->z;

                rdClip_g_faceStatus |= 0x04;
            }

            if ( pCurVert->z <= curPlaneProjY )
            {
                rdVector_Copy3(&rdClip_pDestVert[clipindex], pCurVert);
                rdVector_Copy4(&rdClip_pDestVertIntensity[clipindex++], pCurIntens);
            }
        }

        RD_ASSERTREL(clipindex < STD_ARRAYLEN(rdClip_aWorkVerts));
        pPrevVert   = pCurVert++;
        pPrevIntens = pCurIntens++;
    }

    if ( clipindex < 3 )
    {
        return clipindex;
    }

    paVerts            = rdClip_pDestVert;
    rdClip_pDestVert   = rdClip_pSourceVert;
    rdClip_pSourceVert = paVerts;

    paIntens                    = rdClip_pDestVertIntensity;
    rdClip_pDestVertIntensity   = rdClip_pSourceVertIntensity;
    rdClip_pSourceVertIntensity = paIntens;

    numVerts    = clipindex;
    clipindex   = 0;
    pPrevVert   = &paVerts[numVerts - 1];
    pPrevIntens = &paIntens[numVerts - 1];
    pCurVert    = paVerts;
    pCurIntens  = paIntens;

    for ( size_t i = 0; i < numVerts; ++i )
    {
        float prevPlaneProjY = pPrevVert->y * pFrustrum->bottomPlane;
        float curPlaneProjY = pCurVert->y * pFrustrum->bottomPlane;
        if ( pPrevVert->z >= prevPlaneProjY || pCurVert->z >= curPlaneProjY )
        {
            if ( pPrevVert->z != prevPlaneProjY
                && pCurVert->z != curPlaneProjY
                && (pPrevVert->z < prevPlaneProjY || pCurVert->z < curPlaneProjY) )
            {
                float planeProj = pFrustrum->bottomPlane * (pCurVert->y - pPrevVert->y) - (pCurVert->z - pPrevVert->z);
                float clippedY  = pPrevVert->z * pCurVert->y - pCurVert->z * pPrevVert->y;
                if ( planeProj != 0.0f ) {
                    clippedY = clippedY / planeProj;
                }

                float clippedZ = pFrustrum->bottomPlane * clippedY;

                float dY = fabsf(pCurVert->y - pPrevVert->y);
                float dZ = fabsf(pCurVert->z - pPrevVert->z);

                float lerpFactor = 0.0f;
                if ( dY <= dZ )
                {
                    lerpFactor = (clippedZ - pPrevVert->z) / (pCurVert->z - pPrevVert->z);
                }
                else
                {
                    lerpFactor = (clippedY - pPrevVert->y) / (pCurVert->y - pPrevVert->y);
                }

                rdClip_pDestVert[clipindex].x = (pCurVert->x - pPrevVert->x) * lerpFactor + pPrevVert->x;
                rdClip_pDestVert[clipindex].y = clippedY;
                rdClip_pDestVert[clipindex].z = clippedZ;

                rdClip_pDestVertIntensity[clipindex].x   = (pCurIntens->x - pPrevIntens->x) * lerpFactor + pPrevIntens->x;
                rdClip_pDestVertIntensity[clipindex].y   = (pCurIntens->y - pPrevIntens->y) * lerpFactor + pPrevIntens->y;
                rdClip_pDestVertIntensity[clipindex++].z = (pCurIntens->z - pPrevIntens->z) * lerpFactor + pPrevIntens->z;

                rdClip_g_faceStatus |= 0x08;
            }

            if ( pCurVert->z >= curPlaneProjY )
            {
                rdVector_Copy3(&rdClip_pDestVert[clipindex], pCurVert);
                rdVector_Copy4(&rdClip_pDestVertIntensity[clipindex++], pCurIntens);
            }
        }

        RD_ASSERTREL(clipindex < STD_ARRAYLEN(rdClip_aWorkVerts));
        pPrevVert   = pCurVert++;
        pPrevIntens = pCurIntens++;
    }

    if ( clipindex < 3 )
    {
        return clipindex;
    }

    paVerts            = rdClip_pDestVert;
    rdClip_pDestVert   = rdClip_pSourceVert;
    rdClip_pSourceVert = paVerts;

    paIntens                    = rdClip_pDestVertIntensity;
    rdClip_pDestVertIntensity   = rdClip_pSourceVertIntensity;
    rdClip_pSourceVertIntensity = paIntens;

    numVerts    = clipindex;
    clipindex   = 0;
    pPrevVert   = &paVerts[numVerts - 1];
    pPrevIntens = &paIntens[numVerts - 1];
    pCurVert    = paVerts;
    pCurIntens  = paIntens;

    for ( size_t i = 0; i < numVerts; ++i )
    {
        if ( pPrevVert->y >= pFrustrum->nearPlane || pCurVert->y >= pFrustrum->nearPlane )
        {
            if ( pPrevVert->y != pFrustrum->nearPlane
                && pCurVert->y != pFrustrum->nearPlane
                && (pPrevVert->y < pFrustrum->nearPlane || pCurVert->y < pFrustrum->nearPlane) )
            {
                float lerpFactor = (pFrustrum->nearPlane - pPrevVert->y) / (pCurVert->y - pPrevVert->y);
                rdClip_pDestVert[clipindex].y = pFrustrum->nearPlane;
                rdClip_pDestVert[clipindex].z = (pCurVert->z - pPrevVert->z) * lerpFactor + pPrevVert->z;
                rdClip_pDestVert[clipindex].x = (pCurVert->x - pPrevVert->x) * lerpFactor + pPrevVert->x;

                rdClip_pDestVertIntensity[clipindex].x   = (pCurIntens->x - pPrevIntens->x) * lerpFactor + pPrevIntens->x;
                rdClip_pDestVertIntensity[clipindex].y   = (pCurIntens->y - pPrevIntens->y) * lerpFactor + pPrevIntens->y;
                rdClip_pDestVertIntensity[clipindex++].z = (pCurIntens->z - pPrevIntens->z) * lerpFactor + pPrevIntens->z;

                rdClip_g_faceStatus |= 0x01;
            }

            if ( pCurVert->y >= pFrustrum->nearPlane )
            {
                rdVector_Copy3(&rdClip_pDestVert[clipindex], pCurVert);
                rdVector_Copy4(&rdClip_pDestVertIntensity[clipindex++], pCurIntens);
            }
        }

        RD_ASSERTREL(clipindex < STD_ARRAYLEN(rdClip_aWorkVerts));
        pPrevVert = pCurVert++;
        pPrevIntens = pCurIntens++;
    }

    if ( clipindex < 3 )
    {
        rdClip_g_faceStatus |= 0x40;
        return clipindex;
    }

    if ( !pFrustrum->bClipFar )
    {
        if ( rdClip_pDestVert == aVertices )
        {
            return clipindex;
        }

        rdVector_Copy3List(aVertices, rdClip_pDestVert, clipindex);
        rdVector_Copy4List(aIntensities, rdClip_pDestVertIntensity, clipindex);
        return clipindex;
    }

    paVerts            = rdClip_pDestVert;
    rdClip_pDestVert   = rdClip_pSourceVert;
    rdClip_pSourceVert = paVerts;

    paIntens                    = rdClip_pDestVertIntensity;
    rdClip_pDestVertIntensity   = rdClip_pSourceVertIntensity;
    rdClip_pSourceVertIntensity = paIntens;

    numVerts    = clipindex;
    clipindex   = 0;
    pPrevVert   = &paVerts[numVerts - 1];
    pPrevIntens = &paIntens[numVerts - 1];
    pCurVert    = paVerts;
    pCurIntens  = paIntens;

    for ( size_t i = 0; i < numVerts; ++i )
    {
        if ( pPrevVert->y <= pFrustrum->farPlane || pCurVert->y <= pFrustrum->farPlane )
        {
            if ( pPrevVert->y != pFrustrum->farPlane
                && pCurVert->y != pFrustrum->farPlane
                && (pPrevVert->y > pFrustrum->farPlane || pCurVert->y > pFrustrum->farPlane) )
            {
                float lerpFactor = (pFrustrum->farPlane - pPrevVert->y) / (pCurVert->y - pPrevVert->y);
                rdClip_pDestVert[clipindex].y = pFrustrum->farPlane;
                rdClip_pDestVert[clipindex].z = (pCurVert->z - pPrevVert->z) * lerpFactor + pPrevVert->z;
                rdClip_pDestVert[clipindex].x = (pCurVert->x - pPrevVert->x) * lerpFactor + pPrevVert->x;

                rdClip_pDestVertIntensity[clipindex].x   = (pCurIntens->x - pPrevIntens->x) * lerpFactor + pPrevIntens->x;
                rdClip_pDestVertIntensity[clipindex].y   = (pCurIntens->y - pPrevIntens->y) * lerpFactor + pPrevIntens->y;
                rdClip_pDestVertIntensity[clipindex++].z = (pCurIntens->z - pPrevIntens->z) * lerpFactor + pPrevIntens->z;

                rdClip_g_faceStatus |= 0x02;
            }

            if ( pCurVert->y <= pFrustrum->farPlane )
            {
                rdVector_Copy3(&rdClip_pDestVert[clipindex], pCurVert);
                rdVector_Copy4(&rdClip_pDestVertIntensity[clipindex++], pCurIntens);
            }
        }

        RD_ASSERTREL(clipindex < STD_ARRAYLEN(rdClip_aWorkVerts));
        pPrevVert = pCurVert++;
        pPrevIntens = pCurIntens++;
    }

    if ( clipindex < 3 )
    {
        return clipindex;
    }

    if ( rdClip_pDestVert == aVertices )
    {
        return clipindex;
    }

    rdVector_Copy3List(aVertices, rdClip_pDestVert, clipindex);
    rdVector_Copy4List(aIntensities, rdClip_pDestVertIntensity, clipindex);
    return clipindex;
}

int J3DAPI rdClip_Face3GSOrtho(const rdClipFrustum* pFrustum, rdVector3* aVertices, rdVector4* aIntensities, size_t numVertices)
{
    rdClip_g_faceStatus = 0;

    rdClip_pSourceVert = aVertices;
    rdClip_pDestVert  = rdClip_aWorkVerts;

    rdClip_pSourceVertIntensity = aIntensities;
    rdClip_pDestVertIntensity  = rdClip_aWorkVertIntensities;

    size_t clipindex       = 0;
    rdVector3* pPrevVert   = &aVertices[numVertices - 1];
    rdVector4* pPrevIntens = &aIntensities[numVertices - 1];
    rdVector3* pCurVert    = aVertices;
    rdVector4* pCurIntens  = aIntensities;

    for ( size_t i = 0; i < numVertices; ++i )
    {
        if ( pPrevVert->x >= pFrustum->orthoLeftPlane || pCurVert->x >= pFrustum->orthoLeftPlane )
        {
            if ( pPrevVert->x != pFrustum->orthoLeftPlane
                && pCurVert->x != pFrustum->orthoLeftPlane
                && (pPrevVert->x < pFrustum->orthoLeftPlane || pCurVert->x < pFrustum->orthoLeftPlane) )
            {
                float lerpFactor = (pFrustum->orthoLeftPlane - pPrevVert->x) / (pCurVert->x - pPrevVert->x);
                rdClip_pDestVert[clipindex].x = pFrustum->orthoLeftPlane;
                rdClip_pDestVert[clipindex].y = (pCurVert->y - pPrevVert->y) * lerpFactor + pPrevVert->y;
                rdClip_pDestVert[clipindex].z = (pCurVert->z - pPrevVert->z) * lerpFactor + pPrevVert->z;

                rdClip_pDestVertIntensity[clipindex].x   = (pCurIntens->x - pPrevIntens->x) * lerpFactor + pPrevIntens->x;
                rdClip_pDestVertIntensity[clipindex].y   = (pCurIntens->y - pPrevIntens->y) * lerpFactor + pPrevIntens->y;
                rdClip_pDestVertIntensity[clipindex++].z = (pCurIntens->z - pPrevIntens->z) * lerpFactor + pPrevIntens->z;

                rdClip_g_faceStatus |= 0x10;
            }

            if ( pCurVert->x >= pFrustum->orthoLeftPlane )
            {
                rdVector_Copy3(&rdClip_pDestVert[clipindex], pCurVert);
                rdVector_Copy4(&rdClip_pDestVertIntensity[clipindex++], pCurIntens);
            }
        }

        RD_ASSERTREL(clipindex < STD_ARRAYLEN(rdClip_aWorkVerts));
        pPrevVert = pCurVert++;
        pPrevIntens = pCurIntens++;
    }

    if ( clipindex < 3 )
    {
        return clipindex;
    }

    rdVector3* paVerts = rdClip_pDestVert;
    rdClip_pDestVert = rdClip_pSourceVert;
    rdClip_pSourceVert = paVerts;

    rdVector4* paIntens = rdClip_pDestVertIntensity;
    rdClip_pDestVertIntensity = rdClip_pSourceVertIntensity;
    rdClip_pSourceVertIntensity = paIntens;

    size_t numVerts = clipindex;
    clipindex       = 0;
    pPrevVert       = &paVerts[numVerts - 1];
    pPrevIntens     = &paIntens[numVerts - 1];
    pCurVert        = paVerts;
    pCurIntens      = paIntens;

    for ( size_t i = 0; i < numVerts; ++i )
    {
        if ( pPrevVert->x <= pFrustum->orthoRightPlane || pCurVert->x <= pFrustum->orthoRightPlane )
        {
            if ( pPrevVert->x != pFrustum->orthoRightPlane
                && pCurVert->x != pFrustum->orthoRightPlane
                && (pPrevVert->x > pFrustum->orthoRightPlane || pCurVert->x > pFrustum->orthoRightPlane) )
            {
                float lerpFactor = (pFrustum->orthoRightPlane - pPrevVert->x) / (pCurVert->x - pPrevVert->x);
                rdClip_pDestVert[clipindex].x = pFrustum->orthoRightPlane;
                rdClip_pDestVert[clipindex].y = (pCurVert->y - pPrevVert->y) * lerpFactor + pPrevVert->y;
                rdClip_pDestVert[clipindex].z = (pCurVert->z - pPrevVert->z) * lerpFactor + pPrevVert->z;

                rdClip_pDestVertIntensity[clipindex].x   = (pCurIntens->x - pPrevIntens->x) * lerpFactor + pPrevIntens->x;
                rdClip_pDestVertIntensity[clipindex].y   = (pCurIntens->y - pPrevIntens->y) * lerpFactor + pPrevIntens->y;
                rdClip_pDestVertIntensity[clipindex++].z = (pCurIntens->z - pPrevIntens->z) * lerpFactor + pPrevIntens->z;

                rdClip_g_faceStatus |= 0x20;
            }

            if ( pCurVert->x <= pFrustum->orthoRightPlane )
            {
                rdVector_Copy3(&rdClip_pDestVert[clipindex], pCurVert);
                rdVector_Copy4(&rdClip_pDestVertIntensity[clipindex++], pCurIntens);
            }
        }

        RD_ASSERTREL(clipindex < STD_ARRAYLEN(rdClip_aWorkVerts));
        pPrevVert = pCurVert++;
        pPrevIntens = pCurIntens++;
    }

    if ( clipindex < 3 )
    {
        return clipindex;
    }

    paVerts = rdClip_pDestVert;
    rdClip_pDestVert   = rdClip_pSourceVert;
    rdClip_pSourceVert = paVerts;

    paIntens = rdClip_pDestVertIntensity;
    rdClip_pDestVertIntensity   = rdClip_pSourceVertIntensity;
    rdClip_pSourceVertIntensity = paIntens;

    numVerts    = clipindex;
    clipindex   = 0;
    pPrevVert   = &paVerts[numVerts - 1];
    pPrevIntens = &paIntens[numVerts - 1];
    pCurVert    = paVerts;
    pCurIntens  = paIntens;

    for ( size_t i = 0; i < numVerts; ++i )
    {
        if ( pPrevVert->z <= pFrustum->orthoTopPlane || pCurVert->z <= pFrustum->orthoTopPlane )
        {
            if ( pPrevVert->z != pFrustum->orthoTopPlane && pCurVert->z != pFrustum->orthoTopPlane && (pPrevVert->z > pFrustum->orthoTopPlane || pCurVert->z > pFrustum->orthoTopPlane) )
            {
                float lerpFactor = (pFrustum->orthoTopPlane - pPrevVert->z) / (pCurVert->z - pPrevVert->z);
                rdClip_pDestVert[clipindex].x = (pCurVert->x - pPrevVert->x) * lerpFactor + pPrevVert->x;
                rdClip_pDestVert[clipindex].y = (pCurVert->y - pPrevVert->y) * lerpFactor + pPrevVert->y;
                rdClip_pDestVert[clipindex].z = pFrustum->orthoTopPlane;

                rdClip_pDestVertIntensity[clipindex].x   = (pCurIntens->x - pPrevIntens->x) * lerpFactor + pPrevIntens->x;
                rdClip_pDestVertIntensity[clipindex].y   = (pCurIntens->y - pPrevIntens->y) * lerpFactor + pPrevIntens->y;
                rdClip_pDestVertIntensity[clipindex++].z = (pCurIntens->z - pPrevIntens->z) * lerpFactor + pPrevIntens->z;

                rdClip_g_faceStatus |= 0x04;
            }

            if ( pCurVert->z <= pFrustum->orthoTopPlane )
            {
                rdVector_Copy3(&rdClip_pDestVert[clipindex], pCurVert);
                rdVector_Copy4(&rdClip_pDestVertIntensity[clipindex++], pCurIntens);
            }
        }

        RD_ASSERTREL(clipindex < STD_ARRAYLEN(rdClip_aWorkVerts));
        pPrevVert = pCurVert++;
        pPrevIntens = pCurIntens++;
    }

    if ( clipindex < 3 )
    {
        return clipindex;
    }

    paVerts = rdClip_pDestVert;
    rdClip_pDestVert   = rdClip_pSourceVert;
    rdClip_pSourceVert = paVerts;

    paIntens = rdClip_pDestVertIntensity;
    rdClip_pDestVertIntensity   = rdClip_pSourceVertIntensity;
    rdClip_pSourceVertIntensity = paIntens;

    numVerts    = clipindex;
    clipindex   = 0;
    pPrevVert   = &paVerts[numVerts - 1];
    pPrevIntens = &paIntens[numVerts - 1];
    pCurVert    = paVerts;
    pCurIntens  = paIntens;

    for ( size_t i = 0; i < numVerts; ++i )
    {
        if ( pPrevVert->z >= pFrustum->orthoBottomPlane || pCurVert->z >= pFrustum->orthoBottomPlane )
        {
            if ( pPrevVert->z != pFrustum->orthoBottomPlane
                && pCurVert->z != pFrustum->orthoBottomPlane
                && (pPrevVert->z < pFrustum->orthoBottomPlane || pCurVert->z < pFrustum->orthoBottomPlane) )
            {
                float lerpFactor = (pFrustum->orthoBottomPlane - pPrevVert->z) / (pCurVert->z - pPrevVert->z);
                rdClip_pDestVert[clipindex].x = (pCurVert->x - pPrevVert->x) * lerpFactor + pPrevVert->x;
                rdClip_pDestVert[clipindex].y = (pCurVert->y - pPrevVert->y) * lerpFactor + pPrevVert->y;
                rdClip_pDestVert[clipindex].z = pFrustum->orthoBottomPlane;

                rdClip_pDestVertIntensity[clipindex].x   = (pCurIntens->x - pPrevIntens->x) * lerpFactor + pPrevIntens->x;
                rdClip_pDestVertIntensity[clipindex].y   = (pCurIntens->y - pPrevIntens->y) * lerpFactor + pPrevIntens->y;
                rdClip_pDestVertIntensity[clipindex++].z = (pCurIntens->z - pPrevIntens->z) * lerpFactor + pPrevIntens->z;

                rdClip_g_faceStatus |= 0x08;
            }

            if ( pCurVert->z >= pFrustum->orthoBottomPlane )
            {
                memcpy(&rdClip_pDestVert[clipindex], pCurVert, sizeof(rdVector3));
                memcpy(&rdClip_pDestVertIntensity[clipindex++], pCurIntens, sizeof(rdVector4));
            }
        }

        RD_ASSERTREL(clipindex < STD_ARRAYLEN(rdClip_aWorkVerts));
        pPrevVert = pCurVert++;
        pPrevIntens = pCurIntens++;
    }

    if ( clipindex < 3 )
    {
        return clipindex;
    }

    paVerts = rdClip_pDestVert;
    rdClip_pDestVert = rdClip_pSourceVert;
    rdClip_pSourceVert = paVerts;

    paIntens = rdClip_pDestVertIntensity;
    rdClip_pDestVertIntensity = rdClip_pSourceVertIntensity;
    rdClip_pSourceVertIntensity = paIntens;

    numVerts    = clipindex;
    clipindex   = 0;
    pPrevVert   = &paVerts[numVerts - 1];
    pPrevIntens = &paIntens[numVerts - 1];
    pCurVert    = paVerts;
    pCurIntens  = paIntens;

    for ( size_t i = 0; i < numVerts; ++i )
    {
        if ( pPrevVert->y >= pFrustum->nearPlane || pCurVert->y >= pFrustum->nearPlane )
        {
            if ( pPrevVert->y != pFrustum->nearPlane
                && pCurVert->y != pFrustum->nearPlane
                && (pPrevVert->y < pFrustum->nearPlane || pCurVert->y < pFrustum->nearPlane) )
            {
                float lerpFactor = (pFrustum->nearPlane - pPrevVert->y) / (pCurVert->y - pPrevVert->y);
                rdClip_pDestVert[clipindex].y = pFrustum->nearPlane;
                rdClip_pDestVert[clipindex].z = (pCurVert->z - pPrevVert->z) * lerpFactor + pPrevVert->z;
                rdClip_pDestVert[clipindex].x = (pCurVert->x - pPrevVert->x) * lerpFactor + pPrevVert->x;

                rdClip_pDestVertIntensity[clipindex].x   = (pCurIntens->x - pPrevIntens->x) * lerpFactor + pPrevIntens->x;
                rdClip_pDestVertIntensity[clipindex].y   = (pCurIntens->y - pPrevIntens->y) * lerpFactor + pPrevIntens->y;
                rdClip_pDestVertIntensity[clipindex++].z = (pCurIntens->z - pPrevIntens->z) * lerpFactor + pPrevIntens->z;

                rdClip_g_faceStatus |= 0x01;
            }

            if ( pCurVert->y >= pFrustum->nearPlane )
            {
                memcpy(&rdClip_pDestVert[clipindex], pCurVert, sizeof(rdVector3));
                memcpy(&rdClip_pDestVertIntensity[clipindex++], pCurIntens, sizeof(rdVector4));
            }
        }

        RD_ASSERTREL(clipindex < STD_ARRAYLEN(rdClip_aWorkVerts));
        pPrevVert = pCurVert++;
        pPrevIntens = pPrevIntens++;
    }

    if ( clipindex < 3 )
    {
        rdClip_g_faceStatus |= 0x40;
        return clipindex;
    }

    if ( !pFrustum->bClipFar )
    {
        if ( rdClip_pDestVert == aVertices )
        {
            return clipindex;
        }

        rdVector_Copy3List(aVertices, rdClip_pDestVert, clipindex);
        rdVector_Copy4List(aIntensities, rdClip_pDestVertIntensity, clipindex);
        return clipindex;
    }

    paVerts = rdClip_pDestVert;
    rdClip_pDestVert = rdClip_pSourceVert;
    rdClip_pSourceVert = paVerts;

    paIntens = rdClip_pDestVertIntensity;
    rdClip_pDestVertIntensity = rdClip_pSourceVertIntensity;
    rdClip_pSourceVertIntensity = paIntens;

    numVerts    = clipindex;
    clipindex   = 0;
    pPrevVert   = &paVerts[numVerts - 1];
    pPrevIntens = &paIntens[numVerts - 1];
    pCurVert    = paVerts;
    pCurIntens  = paIntens;

    for ( size_t i = 0; i < numVerts; ++i )
    {
        if ( pPrevVert->y <= pFrustum->farPlane || pCurVert->y <= pFrustum->farPlane )
        {
            if ( pPrevVert->y != pFrustum->farPlane
                && pCurVert->y != pFrustum->farPlane
                && (pPrevVert->y > pFrustum->farPlane || pCurVert->y > pFrustum->farPlane) )
            {
                float lerpFactor = (pFrustum->farPlane - pPrevVert->y) / (pCurVert->y - pPrevVert->y);
                rdClip_pDestVert[clipindex].y = pFrustum->farPlane;
                rdClip_pDestVert[clipindex].z = (pCurVert->z - pPrevVert->z) * lerpFactor + pPrevVert->z;
                rdClip_pDestVert[clipindex].x = (pCurVert->x - pPrevVert->x) * lerpFactor + pPrevVert->x;

                rdClip_pDestVertIntensity[clipindex].x   = (pCurIntens->x - pPrevIntens->x) * lerpFactor + pPrevIntens->x;
                rdClip_pDestVertIntensity[clipindex].y   = (pCurIntens->y - pPrevIntens->y) * lerpFactor + pPrevIntens->y;
                rdClip_pDestVertIntensity[clipindex++].z = (pCurIntens->z - pPrevIntens->z) * lerpFactor + pPrevIntens->z;

                rdClip_g_faceStatus |= 0x02;
            }

            if ( pCurVert->y <= pFrustum->farPlane )
            {
                memcpy(&rdClip_pDestVert[clipindex], pCurVert, sizeof(rdVector3));
                memcpy(&rdClip_pDestVertIntensity[clipindex++], pCurIntens, sizeof(rdVector4));
            }
        }

        RD_ASSERTREL(clipindex < STD_ARRAYLEN(rdClip_aWorkVerts));
        pPrevVert   = pCurVert++;
        pPrevIntens = pCurIntens++;
    }

    if ( clipindex < 3 )
    {
        return clipindex;
    }


    if ( rdClip_pDestVert == aVertices )
    {
        return clipindex;
    }

    rdVector_Copy3List(aVertices, rdClip_pDestVert, clipindex);
    rdVector_Copy4List(aIntensities, rdClip_pDestVertIntensity, clipindex);
    return clipindex;
}

int J3DAPI rdClip_Face3GT(const rdClipFrustum* pFrustrum, rdVector3* aVertices, rdVector2* aTexVertices, size_t numVertices)
{
    rdClip_g_faceStatus = 0;

    rdClip_pSourceVert  = aVertices;
    rdClip_pDestVert    = rdClip_aWorkVerts;

    rdClip_pSourceTVert = aTexVertices;
    rdClip_pDestTVert   = rdClip_aWorkTVerts;

    size_t clipindex       = 0;
    rdVector3* pPrevVert   = &aVertices[numVertices - 1];
    rdVector2* pPrevTVerts = &aTexVertices[numVertices - 1];
    rdVector3* pCurVert    = aVertices;
    rdVector2* pCurTVert   = aTexVertices;

    for ( size_t i = 0; i < numVertices; ++i )
    {
        float prevPlaneProjY = pPrevVert->y * pFrustrum->leftPlane;
        float curPlaneProjY = pCurVert->y * pFrustrum->leftPlane;
        if ( pPrevVert->x >= prevPlaneProjY || pCurVert->x >= curPlaneProjY )
        {
            if ( pPrevVert->x != prevPlaneProjY
                && pCurVert->x != curPlaneProjY
                && (pPrevVert->x < prevPlaneProjY || pCurVert->x < curPlaneProjY) )
            {
                float planeProj = pFrustrum->leftPlane * (pCurVert->y - pPrevVert->y) - (pCurVert->x - pPrevVert->x);
                float clippedY  = pCurVert->y * pPrevVert->x - pPrevVert->y * pCurVert->x;
                if ( planeProj != 0.0f ) {
                    clippedY = clippedY / planeProj;
                }

                float clippedX = pFrustrum->leftPlane * clippedY;

                float dY = fabsf(pCurVert->y - pPrevVert->y);
                float dX = fabsf(pCurVert->x - pPrevVert->x);

                float lerpFactor = 0.0f;
                if ( dY <= dX )
                {
                    lerpFactor = (clippedX - pPrevVert->x) / (pCurVert->x - pPrevVert->x);
                }
                else
                {
                    lerpFactor = (clippedY - pPrevVert->y) / (pCurVert->y - pPrevVert->y);
                }

                rdClip_pDestVert[clipindex].x = clippedX;
                rdClip_pDestVert[clipindex].y = clippedY;
                rdClip_pDestVert[clipindex].z = (pCurVert->z - pPrevVert->z) * lerpFactor + pPrevVert->z;

                rdClip_pDestTVert[clipindex].x   = (pCurTVert->x - pPrevTVerts->x) * lerpFactor + pPrevTVerts->x;
                rdClip_pDestTVert[clipindex++].y = (pCurTVert->y - pPrevTVerts->y) * lerpFactor + pPrevTVerts->y;

                rdClip_g_faceStatus |= 0x10;
            }

            if ( pCurVert->x >= curPlaneProjY )
            {
                memcpy(&rdClip_pDestVert[clipindex], pCurVert, sizeof(rdVector3));
                memcpy(&rdClip_pDestTVert[clipindex++], pCurTVert, sizeof(*rdClip_pDestTVert));
            }
        }

        RD_ASSERTREL(clipindex < STD_ARRAYLEN(rdClip_aWorkVerts));
        pPrevVert   = pCurVert++;
        pPrevTVerts = pCurTVert++;
    }

    if ( clipindex < 3 )
    {
        return clipindex;
    }

    rdVector3* paVerts = rdClip_pDestVert;
    rdClip_pDestVert   = rdClip_pSourceVert;
    rdClip_pSourceVert = paVerts;

    rdVector2* paTVerts= rdClip_pDestTVert;
    rdClip_pDestTVert   = rdClip_pSourceTVert;
    rdClip_pSourceTVert = paTVerts;

    size_t numVerts = clipindex;
    clipindex       = 0;
    pPrevVert       = &paVerts[numVerts - 1];
    pPrevTVerts     = &paTVerts[numVerts - 1];
    pCurVert        = paVerts;
    pCurTVert       = paTVerts;

    for ( size_t i = 0; i < numVerts; ++i )
    {
        float prevPlaneProjY = pPrevVert->y * pFrustrum->rightPlane;
        float curPlaneProjY = pCurVert->y * pFrustrum->rightPlane;
        if ( pPrevVert->x <= prevPlaneProjY || pCurVert->x <= curPlaneProjY )
        {
            if ( pPrevVert->x != prevPlaneProjY
                && pCurVert->x != curPlaneProjY
                && (pPrevVert->x > prevPlaneProjY || pCurVert->x > curPlaneProjY) )
            {
                float planeProj = pFrustrum->rightPlane * (pCurVert->y - pPrevVert->y) - (pCurVert->x - pPrevVert->x);
                float clippedY  = pCurVert->y * pPrevVert->x - pPrevVert->y * pCurVert->x;
                if ( planeProj != 0.0f )
                {
                    clippedY = clippedY / planeProj;
                }

                float clippedX = pFrustrum->rightPlane * clippedY;

                float dY = fabsf(pCurVert->y - pPrevVert->y);
                float dX = fabsf(pCurVert->x - pPrevVert->x);

                float lerpFactor = 0.0f;
                if ( dY <= dX )
                {
                    lerpFactor = (clippedX - pPrevVert->x) / (pCurVert->x - pPrevVert->x);
                }
                else
                {
                    lerpFactor = (clippedY - pPrevVert->y) / (pCurVert->y - pPrevVert->y);
                }

                rdClip_pDestVert[clipindex].x = clippedX;
                rdClip_pDestVert[clipindex].y = clippedY;
                rdClip_pDestVert[clipindex].z = (pCurVert->z - pPrevVert->z) * lerpFactor + pPrevVert->z;

                rdClip_pDestTVert[clipindex].x   = (pCurTVert->x - pPrevTVerts->x) * lerpFactor + pPrevTVerts->x;
                rdClip_pDestTVert[clipindex++].y = (pCurTVert->y - pPrevTVerts->y) * lerpFactor + pPrevTVerts->y;

                rdClip_g_faceStatus |= 0x20;
            }

            if ( pCurVert->x <= curPlaneProjY )
            {
                memcpy(&rdClip_pDestVert[clipindex], pCurVert, sizeof(rdVector3));
                memcpy(&rdClip_pDestTVert[clipindex++], pCurTVert, sizeof(*rdClip_pDestTVert));
            }
        }

        RD_ASSERTREL(clipindex < STD_ARRAYLEN(rdClip_aWorkVerts));
        pPrevVert   = pCurVert++;
        pPrevTVerts = pCurTVert++;
    }

    if ( clipindex < 3 )
    {
        return clipindex;
    }

    paVerts            = rdClip_pDestVert;
    rdClip_pDestVert   = rdClip_pSourceVert;
    rdClip_pSourceVert = paVerts;

    paTVerts            = rdClip_pDestTVert;
    rdClip_pDestTVert   = rdClip_pSourceTVert;
    rdClip_pSourceTVert = paTVerts;

    numVerts    = clipindex;
    clipindex   = 0;
    pPrevVert   = &paVerts[numVerts - 1];
    pPrevTVerts = &paTVerts[numVerts - 1];
    pCurVert    = paVerts;
    pCurTVert   = paTVerts;

    for ( size_t i = 0; i < numVerts; ++i )
    {
        float prevPlaneProjY = pPrevVert->y * pFrustrum->topPlane;
        float curPlaneProjY = pCurVert->y * pFrustrum->topPlane;
        if ( pPrevVert->z <= prevPlaneProjY || pCurVert->z <= curPlaneProjY )
        {
            if ( pPrevVert->z != prevPlaneProjY
                && pCurVert->z != curPlaneProjY
                && (pPrevVert->z > prevPlaneProjY || pCurVert->z > curPlaneProjY) )
            {
                float planeProj = pFrustrum->topPlane * (pCurVert->y - pPrevVert->y) - (pCurVert->z - pPrevVert->z);
                float clippedY  = pPrevVert->z * pCurVert->y - pCurVert->z * pPrevVert->y;;
                if ( planeProj != 0.0f ) {
                    clippedY = clippedY / planeProj;
                }

                float clippedZ = pFrustrum->topPlane * clippedY;

                float dY = fabsf(pCurVert->y - pPrevVert->y);
                float dZ = fabsf(pCurVert->z - pPrevVert->z);

                float lerpFactor = 0.0f;
                if ( dY <= dZ )
                {
                    lerpFactor = (clippedZ - pPrevVert->z) / (pCurVert->z - pPrevVert->z);
                }
                else
                {
                    lerpFactor = (clippedY - pPrevVert->y) / (pCurVert->y - pPrevVert->y);
                }

                rdClip_pDestVert[clipindex].x = (pCurVert->x - pPrevVert->x) * lerpFactor + pPrevVert->x;
                rdClip_pDestVert[clipindex].y = clippedY;
                rdClip_pDestVert[clipindex].z = clippedZ;

                rdClip_pDestTVert[clipindex].x   = (pCurTVert->x - pPrevTVerts->x) * lerpFactor + pPrevTVerts->x;
                rdClip_pDestTVert[clipindex++].y = (pCurTVert->y - pPrevTVerts->y) * lerpFactor + pPrevTVerts->y;

                rdClip_g_faceStatus |= 0x04;
            }

            if ( pCurVert->z <= curPlaneProjY )
            {
                memcpy(&rdClip_pDestVert[clipindex], pCurVert, sizeof(rdVector3));
                memcpy(&rdClip_pDestTVert[clipindex++], pCurTVert, sizeof(*rdClip_pDestTVert));
            }
        }

        RD_ASSERTREL(clipindex < STD_ARRAYLEN(rdClip_aWorkVerts));
        pPrevVert   = pCurVert++;
        pPrevTVerts = pCurTVert++;
    }

    if ( clipindex < 3 )
    {
        return clipindex;
    }

    paVerts            = rdClip_pDestVert;
    rdClip_pDestVert   = rdClip_pSourceVert;
    rdClip_pSourceVert = paVerts;

    paTVerts            = rdClip_pDestTVert;
    rdClip_pDestTVert   = rdClip_pSourceTVert;
    rdClip_pSourceTVert = paTVerts;

    numVerts    = clipindex;
    clipindex   = 0;
    pPrevVert   = &paVerts[numVerts - 1];
    pPrevTVerts = &paTVerts[numVerts - 1];
    pCurVert    = paVerts;
    pCurTVert   = paTVerts;

    for ( size_t i = 0; i < numVerts; ++i )
    {
        float prevPlaneProjY = pPrevVert->y * pFrustrum->bottomPlane;
        float curPlaneProjY = pCurVert->y * pFrustrum->bottomPlane;
        if ( pPrevVert->z >= prevPlaneProjY || pCurVert->z >= curPlaneProjY )
        {
            if ( pPrevVert->z != prevPlaneProjY
                && pCurVert->z != curPlaneProjY
                && (pPrevVert->z < prevPlaneProjY || pCurVert->z < curPlaneProjY) )
            {
                float planeProj = pFrustrum->bottomPlane * (pCurVert->y - pPrevVert->y) - (pCurVert->z - pPrevVert->z);
                float clippedY  = pPrevVert->z * pCurVert->y - pCurVert->z * pPrevVert->y;
                if ( planeProj != 0.0f ) {
                    clippedY = clippedY / planeProj;
                }

                float clippedZ = pFrustrum->bottomPlane * clippedY;

                float dY = fabsf(pCurVert->y - pPrevVert->y);
                float dZ = fabsf(pCurVert->z - pPrevVert->z);

                float lerpFactor = 0.0f;
                if ( dY <= dZ )
                {
                    lerpFactor = (clippedZ - pPrevVert->z) / (pCurVert->z - pPrevVert->z);
                }
                else
                {
                    lerpFactor = (clippedY - pPrevVert->y) / (pCurVert->y - pPrevVert->y);
                }

                rdClip_pDestVert[clipindex].x = (pCurVert->x - pPrevVert->x) * lerpFactor + pPrevVert->x;
                rdClip_pDestVert[clipindex].y = clippedY;
                rdClip_pDestVert[clipindex].z = clippedZ;

                rdClip_pDestTVert[clipindex].x = (pCurTVert->x - pPrevTVerts->x) * lerpFactor + pPrevTVerts->x;
                rdClip_pDestTVert[clipindex].y = (pCurTVert->y - pPrevTVerts->y) * lerpFactor + pPrevTVerts->y;

                clipindex++;
                rdClip_g_faceStatus |= 0x08;
            }

            if ( pCurVert->z >= curPlaneProjY )
            {
                memcpy(&rdClip_pDestVert[clipindex], pCurVert, sizeof(rdVector3));
                memcpy(&rdClip_pDestTVert[clipindex++], pCurTVert, sizeof(*rdClip_pDestTVert));
            }
        }

        RD_ASSERTREL(clipindex < STD_ARRAYLEN(rdClip_aWorkVerts));
        pPrevVert   = pCurVert++;
        pPrevTVerts = pCurTVert++;
    }

    if ( clipindex < 3 )
    {
        return clipindex;
    }

    paVerts            = rdClip_pDestVert;
    rdClip_pDestVert   = rdClip_pSourceVert;
    rdClip_pSourceVert = paVerts;

    paTVerts            = rdClip_pDestTVert;
    rdClip_pDestTVert   = rdClip_pSourceTVert;
    rdClip_pSourceTVert = paTVerts;

    numVerts    = clipindex;
    clipindex   = 0;
    pPrevVert   = &paVerts[numVerts - 1];
    pPrevTVerts = &paTVerts[numVerts - 1];
    pCurVert    = paVerts;
    pCurTVert   = paTVerts;

    for ( size_t i = 0; i < numVerts; ++i )
    {
        if ( pPrevVert->y >= pFrustrum->nearPlane || pCurVert->y >= pFrustrum->nearPlane )
        {
            if ( pPrevVert->y != pFrustrum->nearPlane
                && pCurVert->y != pFrustrum->nearPlane
                && (pPrevVert->y < pFrustrum->nearPlane || pCurVert->y < pFrustrum->nearPlane) )
            {
                float lerpFactor = (pFrustrum->nearPlane - pPrevVert->y) / (pCurVert->y - pPrevVert->y);
                rdClip_pDestVert[clipindex].y = pFrustrum->nearPlane;
                rdClip_pDestVert[clipindex].z = (pCurVert->z - pPrevVert->z) * lerpFactor + pPrevVert->z;
                rdClip_pDestVert[clipindex].x = (pCurVert->x - pPrevVert->x) * lerpFactor + pPrevVert->x;

                rdClip_pDestTVert[clipindex].x   = (pCurTVert->x - pPrevTVerts->x) * lerpFactor + pPrevTVerts->x;
                rdClip_pDestTVert[clipindex++].y = (pCurTVert->y - pPrevTVerts->y) * lerpFactor + pPrevTVerts->y;

                rdClip_g_faceStatus |= 0x01;
            }

            if ( pCurVert->y >= pFrustrum->nearPlane )
            {
                memcpy(&rdClip_pDestVert[clipindex], pCurVert, sizeof(rdVector3));
                memcpy(&rdClip_pDestTVert[clipindex++], pCurTVert, sizeof(*rdClip_pDestTVert));
            }
        }

        RD_ASSERTREL(clipindex < STD_ARRAYLEN(rdClip_aWorkVerts));
        pPrevVert = pCurVert++;
        pPrevTVerts = pCurTVert++;
    }

    if ( clipindex < 3 )
    {
        rdClip_g_faceStatus |= 0x40;
        return clipindex;
    }

    if ( !pFrustrum->bClipFar )
    {
        if ( rdClip_pDestVert == aVertices )
        {
            return clipindex;
        }

        rdVector_Copy3List(aVertices, rdClip_pDestVert, clipindex);
        rdVector_Copy2List(aTexVertices, rdClip_pDestTVert, clipindex);
        return clipindex;
    }

    paVerts            = rdClip_pDestVert;
    rdClip_pDestVert   = rdClip_pSourceVert;
    rdClip_pSourceVert = paVerts;

    paTVerts            = rdClip_pDestTVert;
    rdClip_pDestTVert   = rdClip_pSourceTVert;
    rdClip_pSourceTVert = paTVerts;

    numVerts    = clipindex;
    clipindex   = 0;
    pPrevVert   = &paVerts[numVerts - 1];
    pPrevTVerts = &paTVerts[numVerts - 1];
    pCurVert    = paVerts;
    pCurTVert   = paTVerts;

    for ( size_t i = 0; i < numVerts; ++i )
    {
        if ( pPrevVert->y <= pFrustrum->farPlane || pCurVert->y <= pFrustrum->farPlane )
        {
            if ( pPrevVert->y != pFrustrum->farPlane
                && pCurVert->y != pFrustrum->farPlane
                && (pPrevVert->y > pFrustrum->farPlane || pCurVert->y > pFrustrum->farPlane) )
            {
                float lerpFactor = (pFrustrum->farPlane - pPrevVert->y) / (pCurVert->y - pPrevVert->y);
                rdClip_pDestVert[clipindex].y = pFrustrum->farPlane;
                rdClip_pDestVert[clipindex].z = (pCurVert->z - pPrevVert->z) * lerpFactor + pPrevVert->z;
                rdClip_pDestVert[clipindex].x = (pCurVert->x - pPrevVert->x) * lerpFactor + pPrevVert->x;

                rdClip_pDestTVert[clipindex].x   = (pCurTVert->x - pPrevTVerts->x) * lerpFactor + pPrevTVerts->x;
                rdClip_pDestTVert[clipindex++].y = (pCurTVert->y - pPrevTVerts->y) * lerpFactor + pPrevTVerts->y;

                rdClip_g_faceStatus |= 0x02;
            }

            if ( pCurVert->y <= pFrustrum->farPlane )
            {
                memcpy(&rdClip_pDestVert[clipindex], pCurVert, sizeof(rdVector3));
                memcpy(&rdClip_pDestTVert[clipindex++], pCurTVert, sizeof(*rdClip_pDestTVert));
            }
        }

        RD_ASSERTREL(clipindex < STD_ARRAYLEN(rdClip_aWorkVerts));
        pPrevVert = pCurVert++;
        pPrevTVerts = pCurTVert++;
    }

    if ( clipindex < 3 )
    {
        return clipindex;
    }

    if ( rdClip_pDestVert == aVertices )
    {
        return clipindex;
    }

    rdVector_Copy3List(aVertices, rdClip_pDestVert, clipindex);
    rdVector_Copy2List(aTexVertices, rdClip_pDestTVert, clipindex);
    return clipindex;
}

int J3DAPI rdClip_Face3GTOrtho(const rdClipFrustum* pFrustum, rdVector3* aVertices, rdVector2* aTexVertices, size_t numVertices)
{

    rdClip_g_faceStatus = 0;

    rdClip_pSourceVert = aVertices;
    rdClip_pDestVert   = rdClip_aWorkVerts;

    rdClip_pSourceTVert = aTexVertices;
    rdClip_pDestTVert   = rdClip_aWorkTVerts;

    size_t clipindex       = 0;
    rdVector3* pPrevVert   = &aVertices[numVertices - 1];
    rdVector2* pPrevTVert = &aTexVertices[numVertices - 1];
    rdVector3* pCurVert    = aVertices;
    rdVector2* pCurTVert  = aTexVertices;

    for ( size_t i = 0; i < numVertices; ++i )
    {
        if ( pPrevVert->x >= pFrustum->orthoLeftPlane || pCurVert->x >= pFrustum->orthoLeftPlane )
        {
            if ( pPrevVert->x != pFrustum->orthoLeftPlane
                && pCurVert->x != pFrustum->orthoLeftPlane
                && (pPrevVert->x < pFrustum->orthoLeftPlane || pCurVert->x < pFrustum->orthoLeftPlane) )
            {
                float lerpFactor = (pFrustum->orthoLeftPlane - pPrevVert->x) / (pCurVert->x - pPrevVert->x);
                rdClip_pDestVert[clipindex].x = pFrustum->orthoLeftPlane;
                rdClip_pDestVert[clipindex].y = (pCurVert->y - pPrevVert->y) * lerpFactor + pPrevVert->y;
                rdClip_pDestVert[clipindex].z = (pCurVert->z - pPrevVert->z) * lerpFactor + pPrevVert->z;

                rdClip_pDestTVert[clipindex].x   = (pCurTVert->x - pPrevTVert->x) * lerpFactor + pPrevTVert->x;
                rdClip_pDestTVert[clipindex++].y = (pCurTVert->y - pPrevTVert->y) * lerpFactor + pPrevTVert->y;

                rdClip_g_faceStatus |= 0x10;
            }

            if ( pCurVert->x >= pFrustum->orthoLeftPlane )
            {
                memcpy(&rdClip_pDestVert[clipindex], pCurVert, sizeof(rdVector3));
                memcpy(&rdClip_pDestTVert[clipindex++], pCurTVert, sizeof(*rdClip_pDestTVert));
            }
        }

        RD_ASSERTREL(clipindex < STD_ARRAYLEN(rdClip_aWorkVerts));
        pPrevVert = pCurVert++;
        pPrevTVert = pCurTVert++;
    }

    if ( clipindex < 3 )
    {
        return clipindex;
    }

    rdVector3* paVerts = rdClip_pDestVert;
    rdClip_pDestVert = rdClip_pSourceVert;
    rdClip_pSourceVert = paVerts;

    rdVector2* paTVerts = rdClip_pDestTVert;
    rdClip_pDestTVert   = rdClip_pSourceTVert;
    rdClip_pSourceTVert = paTVerts;

    size_t numVerts = clipindex;
    clipindex       = 0;
    pPrevVert       = &paVerts[numVerts - 1];
    pPrevTVert      = &paTVerts[numVerts - 1];
    pCurVert        = paVerts;
    pCurTVert       = paTVerts;

    for ( size_t i = 0; i < numVerts; ++i )
    {
        if ( pPrevVert->x <= pFrustum->orthoRightPlane || pCurVert->x <= pFrustum->orthoRightPlane )
        {
            if ( pPrevVert->x != pFrustum->orthoRightPlane
                && pCurVert->x != pFrustum->orthoRightPlane
                && (pPrevVert->x > pFrustum->orthoRightPlane || pCurVert->x > pFrustum->orthoRightPlane) )
            {
                float lerpFactor = (pFrustum->orthoRightPlane - pPrevVert->x) / (pCurVert->x - pPrevVert->x);
                rdClip_pDestVert[clipindex].x = pFrustum->orthoRightPlane;
                rdClip_pDestVert[clipindex].y = (pCurVert->y - pPrevVert->y) * lerpFactor + pPrevVert->y;
                rdClip_pDestVert[clipindex].z = (pCurVert->z - pPrevVert->z) * lerpFactor + pPrevVert->z;

                rdClip_pDestTVert[clipindex].x   = (pCurTVert->x - pPrevTVert->x) * lerpFactor + pPrevTVert->x;
                rdClip_pDestTVert[clipindex++].y = (pCurTVert->y - pPrevTVert->y) * lerpFactor + pPrevTVert->y;

                rdClip_g_faceStatus |= 0x20;
            }

            if ( pCurVert->x <= pFrustum->orthoRightPlane )
            {
                memcpy(&rdClip_pDestVert[clipindex], pCurVert, sizeof(rdVector3));
                memcpy(&rdClip_pDestTVert[clipindex++], pCurTVert, sizeof(*rdClip_pDestTVert));
            }
        }

        RD_ASSERTREL(clipindex < STD_ARRAYLEN(rdClip_aWorkVerts));
        pPrevVert = pCurVert++;
        pPrevTVert = pCurTVert++;
    }

    if ( clipindex < 3 )
    {
        return clipindex;
    }

    paVerts = rdClip_pDestVert;
    rdClip_pDestVert  = rdClip_pSourceVert;
    rdClip_pSourceVert = paVerts;

    paTVerts = rdClip_pDestTVert;
    rdClip_pDestTVert  = rdClip_pSourceTVert;
    rdClip_pSourceTVert = paTVerts;

    numVerts    = clipindex;
    clipindex   = 0;
    pPrevVert   = &paVerts[numVerts - 1];
    pPrevTVert = &paTVerts[numVerts - 1];
    pCurVert    = paVerts;
    pCurTVert  = paTVerts;

    for ( size_t i = 0; i < numVerts; ++i )
    {
        if ( pPrevVert->z <= pFrustum->orthoTopPlane || pCurVert->z <= pFrustum->orthoTopPlane )
        {
            if ( pPrevVert->z != pFrustum->orthoTopPlane && pCurVert->z != pFrustum->orthoTopPlane && (pPrevVert->z > pFrustum->orthoTopPlane || pCurVert->z > pFrustum->orthoTopPlane) )
            {
                float lerpFactor = (pFrustum->orthoTopPlane - pPrevVert->z) / (pCurVert->z - pPrevVert->z);
                rdClip_pDestVert[clipindex].x = (pCurVert->x - pPrevVert->x) * lerpFactor + pPrevVert->x;
                rdClip_pDestVert[clipindex].y = (pCurVert->y - pPrevVert->y) * lerpFactor + pPrevVert->y;
                rdClip_pDestVert[clipindex].z = pFrustum->orthoTopPlane;

                rdClip_pDestTVert[clipindex].x   = (pCurTVert->x - pPrevTVert->x) * lerpFactor + pPrevTVert->x;
                rdClip_pDestTVert[clipindex++].y = (pCurTVert->y - pPrevTVert->y) * lerpFactor + pPrevTVert->y;

                rdClip_g_faceStatus |= 0x04;
            }

            if ( pCurVert->z <= pFrustum->orthoTopPlane )
            {
                memcpy(&rdClip_pDestVert[clipindex], pCurVert, sizeof(rdVector3));
                memcpy(&rdClip_pDestTVert[clipindex++], pCurTVert, sizeof(*rdClip_pDestTVert));
            }
        }

        RD_ASSERTREL(clipindex < STD_ARRAYLEN(rdClip_aWorkVerts));
        pPrevVert = pCurVert++;
        pPrevTVert = pCurTVert++;
    }

    if ( clipindex < 3 )
    {
        return clipindex;
    }

    paVerts = rdClip_pDestVert;
    rdClip_pDestVert   = rdClip_pSourceVert;
    rdClip_pSourceVert = paVerts;

    paTVerts = rdClip_pDestTVert;
    rdClip_pDestTVert   = rdClip_pSourceTVert;
    rdClip_pSourceTVert = paTVerts;

    numVerts    = clipindex;
    clipindex   = 0;
    pPrevVert   = &paVerts[numVerts - 1];
    pPrevTVert  = &paTVerts[numVerts - 1];
    pCurVert    = paVerts;
    pCurTVert   = paTVerts;

    for ( size_t i = 0; i < numVerts; ++i )
    {
        if ( pPrevVert->z >= pFrustum->orthoBottomPlane || pCurVert->z >= pFrustum->orthoBottomPlane )
        {
            if ( pPrevVert->z != pFrustum->orthoBottomPlane
                && pCurVert->z != pFrustum->orthoBottomPlane
                && (pPrevVert->z < pFrustum->orthoBottomPlane || pCurVert->z < pFrustum->orthoBottomPlane) )
            {
                float lerpFactor = (pFrustum->orthoBottomPlane - pPrevVert->z) / (pCurVert->z - pPrevVert->z);
                rdClip_pDestVert[clipindex].x = (pCurVert->x - pPrevVert->x) * lerpFactor + pPrevVert->x;
                rdClip_pDestVert[clipindex].y = (pCurVert->y - pPrevVert->y) * lerpFactor + pPrevVert->y;
                rdClip_pDestVert[clipindex].z = pFrustum->orthoBottomPlane;

                rdClip_pDestTVert[clipindex].x   = (pCurTVert->x - pPrevTVert->x) * lerpFactor + pPrevTVert->x;
                rdClip_pDestTVert[clipindex++].y = (pCurTVert->y - pPrevTVert->y) * lerpFactor + pPrevTVert->y;

                rdClip_g_faceStatus |= 0x08;
            }

            if ( pCurVert->z >= pFrustum->orthoBottomPlane )
            {
                memcpy(&rdClip_pDestVert[clipindex], pCurVert, sizeof(rdVector3));
                memcpy(&rdClip_pDestTVert[clipindex++], pCurTVert, sizeof(*rdClip_pDestTVert));
            }
        }

        RD_ASSERTREL(clipindex < STD_ARRAYLEN(rdClip_aWorkVerts));
        pPrevVert  = pCurVert++;
        pPrevTVert = pCurTVert++;
    }

    if ( clipindex < 3 )
    {
        return clipindex;
    }

    paVerts = rdClip_pDestVert;
    rdClip_pDestVert   = rdClip_pSourceVert;
    rdClip_pSourceVert = paVerts;

    paTVerts = rdClip_pDestTVert;
    rdClip_pDestTVert   = rdClip_pSourceTVert;
    rdClip_pSourceTVert = paTVerts;

    numVerts  = clipindex;
    clipindex = 0;
    pPrevVert = &paVerts[numVerts - 1];
    pCurTVert = &paTVerts[numVerts - 1];
    pCurVert  = paVerts;
    pCurTVert = paTVerts;

    for ( size_t i = 0; i < numVerts; ++i )
    {
        if ( pPrevVert->y >= pFrustum->nearPlane || pCurVert->y >= pFrustum->nearPlane )
        {
            if ( pPrevVert->y != pFrustum->nearPlane
                && pCurVert->y != pFrustum->nearPlane
                && (pPrevVert->y < pFrustum->nearPlane || pCurVert->y < pFrustum->nearPlane) )
            {
                float lerpFactor = (pFrustum->nearPlane - pPrevVert->y) / (pCurVert->y - pPrevVert->y);
                rdClip_pDestVert[clipindex].y = pFrustum->nearPlane;
                rdClip_pDestVert[clipindex].z = (pCurVert->z - pPrevVert->z) * lerpFactor + pPrevVert->z;
                rdClip_pDestVert[clipindex].x = (pCurVert->x - pPrevVert->x) * lerpFactor + pPrevVert->x;

                rdClip_pDestTVert[clipindex].x   = (pCurTVert->x - pPrevTVert->x) * lerpFactor + pPrevTVert->x;
                rdClip_pDestTVert[clipindex++].y = (pCurTVert->y - pPrevTVert->y) * lerpFactor + pPrevTVert->y;

                rdClip_g_faceStatus |= 0x01;
            }

            if ( pCurVert->y >= pFrustum->nearPlane )
            {
                memcpy(&rdClip_pDestVert[clipindex], pCurVert, sizeof(rdVector3));
                memcpy(&rdClip_pDestTVert[clipindex++], pCurTVert, sizeof(*rdClip_pDestTVert));
            }
        }

        RD_ASSERTREL(clipindex < STD_ARRAYLEN(rdClip_aWorkVerts));
        pPrevVert = pCurVert++;
        pPrevTVert = pCurTVert++;
    }

    if ( clipindex < 3 )
    {
        rdClip_g_faceStatus |= 0x40;
        return clipindex;
    }

    if ( !pFrustum->bClipFar )
    {
        if ( rdClip_pDestVert == aVertices )
        {
            return clipindex;
        }

        rdVector_Copy3List(aVertices, rdClip_pDestVert, clipindex);
        rdVector_Copy2List(aTexVertices, rdClip_pDestTVert, clipindex);
        return clipindex;
    }

    paVerts = rdClip_pDestVert;
    rdClip_pDestVert   = rdClip_pSourceVert;
    rdClip_pSourceVert = paVerts;

    paTVerts = rdClip_pDestTVert;
    rdClip_pDestTVert   = rdClip_pSourceTVert;
    rdClip_pSourceTVert = paTVerts;

    numVerts   = clipindex;
    clipindex  = 0;
    pPrevVert  = &paVerts[numVerts - 1];
    pPrevTVert = &paTVerts[numVerts - 1];
    pCurVert   = paVerts;
    pCurTVert  = paTVerts;

    for ( size_t i = 0; i < numVerts; ++i )
    {
        if ( pPrevVert->y <= pFrustum->farPlane || pCurVert->y <= pFrustum->farPlane )
        {
            if ( pPrevVert->y != pFrustum->farPlane
                && pCurVert->y != pFrustum->farPlane
                && (pPrevVert->y > pFrustum->farPlane || pCurVert->y > pFrustum->farPlane) )
            {
                float lerpFactor = (pFrustum->farPlane - pPrevVert->y) / (pCurVert->y - pPrevVert->y);
                rdClip_pDestVert[clipindex].y = pFrustum->farPlane;
                rdClip_pDestVert[clipindex].z = (pCurVert->z - pPrevVert->z) * lerpFactor + pPrevVert->z;
                rdClip_pDestVert[clipindex].x = (pCurVert->x - pPrevVert->x) * lerpFactor + pPrevVert->x;

                rdClip_pDestTVert[clipindex].x   = (pCurTVert->x - pPrevTVert->x) * lerpFactor + pPrevTVert->x;
                rdClip_pDestTVert[clipindex++].y = (pCurTVert->y - pPrevTVert->y) * lerpFactor + pPrevTVert->y;

                rdClip_g_faceStatus |= 0x02;
            }

            if ( pCurVert->y <= pFrustum->farPlane )
            {
                memcpy(&rdClip_pDestVert[clipindex], pCurVert, sizeof(rdVector3));
                memcpy(&rdClip_pDestTVert[clipindex++], pCurTVert, sizeof(*rdClip_pDestTVert));
            }
        }

        RD_ASSERTREL(clipindex < STD_ARRAYLEN(rdClip_aWorkVerts));
        pPrevVert  = pCurVert++;
        pPrevTVert = pCurTVert++;
    }

    if ( clipindex < 3 )
    {
        return clipindex;
    }

    if ( rdClip_pDestVert == aVertices )
    {
        return clipindex;
    }

    rdVector_Copy3List(aVertices, rdClip_pDestVert, clipindex);
    rdVector_Copy2List(aTexVertices, rdClip_pDestTVert, clipindex);
    return clipindex;
}

int J3DAPI rdClip_Face3T(const rdClipFrustum* pFrustrum, rdVector3* aVertices, rdVector2* aTexVertices, rdVector4* aIntensities, size_t numVertices)
{
    // TODO: Clipping of vert intensities is buggy, needs to be fixed
    // TODO: There is discrepancy in the clipping algorithm between the original code and this one.

    // Added: Ensure that the input arrays are not the same as the work arrays
    RD_ASSERT(aVertices != rdClip_aWorkVerts);
    RD_ASSERT(aTexVertices != rdClip_aWorkTVerts);
    RD_ASSERT(aIntensities != rdClip_aWorkVertIntensities);

    rdClip_g_faceStatus = 0;

    rdClip_pSourceVert  = aVertices;
    rdClip_pDestVert    = rdClip_aWorkVerts;

    rdClip_pSourceTVert = aTexVertices;
    rdClip_pDestTVert   = rdClip_aWorkTVerts;

    rdClip_pSourceVertIntensity = aIntensities;
    rdClip_pDestVertIntensity   = rdClip_aWorkVertIntensities;

    size_t clipindex       = 0;

    rdVector3* pPrevVert   = &aVertices[numVertices - 1];
    rdVector2* pPrevTVerts = &aTexVertices[numVertices - 1];

    rdVector3* pCurVert    = aVertices;
    rdVector2* pCurTVert   = aTexVertices;

    rdVector4* pPrevIntens = &aIntensities[numVertices - 1];
    rdVector4* pCurIntens  = aIntensities;

    // LEFT PLANE CLIPPING
    for ( size_t i = 0; i < numVertices; ++i )
    {
        float prevPlaneProjY = pPrevVert->y * pFrustrum->leftPlane;
        float curPlaneProjY  = pCurVert->y * pFrustrum->leftPlane;
        if ( pPrevVert->x >= prevPlaneProjY || pCurVert->x >= curPlaneProjY )
        {
            if ( pPrevVert->x != prevPlaneProjY
                && pCurVert->x != curPlaneProjY
                && (pPrevVert->x < (double)prevPlaneProjY || pCurVert->x < (double)curPlaneProjY) )
            {
                float planeProj = pFrustrum->leftPlane * (pCurVert->y - pPrevVert->y) - (pCurVert->x - pPrevVert->x);
                float clippedY  = pCurVert->y * pPrevVert->x - pPrevVert->y * pCurVert->x;
                if ( planeProj != 0.0f ) {
                    clippedY = clippedY / planeProj;
                }

                float clippedX = pFrustrum->leftPlane * clippedY;

                float dY = fabsf(pCurVert->y - pPrevVert->y);
                float dX = fabsf(pCurVert->x - pPrevVert->x);

                float lerpFactor = 0.0f;
                if ( dY <= (double)dX )
                {
                    lerpFactor = (clippedX - pPrevVert->x) / (pCurVert->x - pPrevVert->x);
                }
                else
                {
                    lerpFactor = (clippedY - pPrevVert->y) / (pCurVert->y - pPrevVert->y);
                }

                rdClip_pDestVert[clipindex].x = clippedX;
                rdClip_pDestVert[clipindex].y = clippedY;
                rdClip_pDestVert[clipindex].z = (pCurVert->z - pPrevVert->z) * lerpFactor + pPrevVert->z;

                rdClip_pDestTVert[clipindex].x = (pCurTVert->x - pPrevTVerts->x) * lerpFactor + pPrevTVerts->x;
                rdClip_pDestTVert[clipindex].y = (pCurTVert->y - pPrevTVerts->y) * lerpFactor + pPrevTVerts->y;

                rdClip_pDestVertIntensity[clipindex].red   = (pCurIntens->red - pPrevIntens->red) * lerpFactor + pPrevIntens->red;
                rdClip_pDestVertIntensity[clipindex].green = (pCurIntens->green - pPrevIntens->green) * lerpFactor + pPrevIntens->green;
                rdClip_pDestVertIntensity[clipindex].blue  = (pCurIntens->blue - pPrevIntens->blue) * lerpFactor + pPrevIntens->blue;
                rdClip_pDestVertIntensity[clipindex].alpha = (pCurIntens->alpha - pPrevIntens->alpha) * lerpFactor + pPrevIntens->alpha;

                clipindex++;
                rdClip_g_faceStatus |= 0x10;
            }

            if ( pCurVert->x >= (double)curPlaneProjY )
            {
                rdClip_pDestVert[clipindex]          = *pCurVert;
                rdClip_pDestTVert[clipindex]         = *pCurTVert;
                rdClip_pDestVertIntensity[clipindex] = *pCurIntens;
                clipindex++;
            }
        }

        RD_ASSERTREL(clipindex < STD_ARRAYLEN(rdClip_aWorkVerts));
        pPrevVert   = pCurVert++;
        pPrevTVerts = pCurTVert++;
        pPrevIntens = pCurIntens++;
    }

    if ( clipindex < 3 )
    {
        return clipindex;
    }

    rdVector3* paVerts = rdClip_pDestVert;
    rdClip_pDestVert   = rdClip_pSourceVert;
    rdClip_pSourceVert = paVerts;

    rdVector2* paTVerts = rdClip_pDestTVert;
    rdClip_pDestTVert   = rdClip_pSourceTVert;
    rdClip_pSourceTVert = paTVerts;

    rdVector4* paIntens         = rdClip_pDestVertIntensity;
    rdClip_pDestVertIntensity   = rdClip_pSourceVertIntensity;
    rdClip_pSourceVertIntensity = paIntens;

    size_t numVerts = clipindex;
    clipindex       = 0;

    pPrevVert       = &paVerts[numVerts - 1];
    pPrevTVerts     = &paTVerts[numVerts - 1];
    pPrevIntens     = &paIntens[numVerts - 1];

    pCurVert        = paVerts;
    pCurTVert       = paTVerts;
    pCurIntens      = paIntens;

    // RIGHT PLANE CLIPPING
    for ( size_t i = 0; i < numVerts; ++i )
    {
        float prevPlaneProjY = pPrevVert->y * pFrustrum->rightPlane;
        float curPlaneProjY = pCurVert->y * pFrustrum->rightPlane;
        if ( pPrevVert->x <= (double)prevPlaneProjY || pCurVert->x <= (double)curPlaneProjY )
        {
            if ( pPrevVert->x != prevPlaneProjY
                && pCurVert->x != curPlaneProjY
                && (pPrevVert->x > (double)prevPlaneProjY || pCurVert->x > (double)curPlaneProjY) )
            {
                float planeProj = pFrustrum->rightPlane * (pCurVert->y - pPrevVert->y) - (pCurVert->x - pPrevVert->x);
                float clippedY  = pCurVert->y * pPrevVert->x - pPrevVert->y * pCurVert->x;
                if ( planeProj != 0.0f )
                {
                    clippedY = clippedY / planeProj;
                }

                float clippedX = pFrustrum->rightPlane * clippedY;

                float dY = fabsf(pCurVert->y - pPrevVert->y);
                float dX = fabsf(pCurVert->x - pPrevVert->x);

                float lerpFactor = 0.0f;
                if ( dY <= (double)dX )
                {
                    lerpFactor = (clippedX - pPrevVert->x) / (pCurVert->x - pPrevVert->x);
                }
                else
                {
                    lerpFactor = (clippedY - pPrevVert->y) / (pCurVert->y - pPrevVert->y);
                }

                rdClip_pDestVert[clipindex].x = clippedX;
                rdClip_pDestVert[clipindex].y = clippedY;
                rdClip_pDestVert[clipindex].z = (pCurVert->z - pPrevVert->z) * lerpFactor + pPrevVert->z;

                rdClip_pDestTVert[clipindex].x = (pCurTVert->x - pPrevTVerts->x) * lerpFactor + pPrevTVerts->x;
                rdClip_pDestTVert[clipindex].y = (pCurTVert->y - pPrevTVerts->y) * lerpFactor + pPrevTVerts->y;

                rdClip_pDestVertIntensity[clipindex].red   = (pCurIntens->red - pPrevIntens->red) * lerpFactor + pPrevIntens->red;
                rdClip_pDestVertIntensity[clipindex].green = (pCurIntens->green - pPrevIntens->green) * lerpFactor + pPrevIntens->green;
                rdClip_pDestVertIntensity[clipindex].blue  = (pCurIntens->blue - pPrevIntens->blue) * lerpFactor + pPrevIntens->blue;
                rdClip_pDestVertIntensity[clipindex].alpha = (pCurIntens->alpha - pPrevIntens->alpha) * lerpFactor + pPrevIntens->alpha;

                clipindex++;
                rdClip_g_faceStatus |= 0x20;
            }

            if ( pCurVert->x <= (double)curPlaneProjY )
            {
                rdClip_pDestVert[clipindex]          = *pCurVert;
                rdClip_pDestTVert[clipindex]         = *pCurTVert;
                rdClip_pDestVertIntensity[clipindex] = *pCurIntens;
                clipindex++;
            }
        }

        RD_ASSERTREL(clipindex < STD_ARRAYLEN(rdClip_aWorkVerts));
        pPrevVert   = pCurVert++;
        pPrevTVerts = pCurTVert++;
        pPrevIntens = pCurIntens++;
    }

    if ( clipindex < 3 )
    {
        return clipindex;
    }

    paVerts            = rdClip_pDestVert;
    rdClip_pDestVert   = rdClip_pSourceVert;
    rdClip_pSourceVert = paVerts;

    paTVerts            = rdClip_pDestTVert;
    rdClip_pDestTVert   = rdClip_pSourceTVert;
    rdClip_pSourceTVert = paTVerts;

    paIntens                    = rdClip_pDestVertIntensity;
    rdClip_pDestVertIntensity   = rdClip_pSourceVertIntensity;
    rdClip_pSourceVertIntensity = paIntens;

    numVerts    = clipindex;
    clipindex   = 0;

    pPrevVert   = &paVerts[numVerts - 1];
    pPrevTVerts = &paTVerts[numVerts - 1];
    pPrevIntens = &paIntens[numVerts - 1];

    pCurVert    = paVerts;
    pCurTVert   = paTVerts;
    pCurIntens  = paIntens;

    // TOP PLANE CLIPPING
    for ( size_t i = 0; i < numVerts; ++i )
    {
        float prevPlaneProjY = pPrevVert->y * pFrustrum->topPlane;
        float curPlaneProjY = pCurVert->y * pFrustrum->topPlane;
        if ( pPrevVert->z <= (double)prevPlaneProjY || pCurVert->z <= (double)curPlaneProjY )
        {
            if ( pPrevVert->z != prevPlaneProjY
                && pCurVert->z != curPlaneProjY
                && (pPrevVert->z > (double)prevPlaneProjY || pCurVert->z > (double)curPlaneProjY) )
            {
                float planeProj = pFrustrum->topPlane * (pCurVert->y - pPrevVert->y) - (pCurVert->z - pPrevVert->z);
                float clippedY  = pPrevVert->z * pCurVert->y - pCurVert->z * pPrevVert->y;;
                if ( planeProj != 0.0f ) {
                    clippedY = clippedY / planeProj;
                }

                float clippedZ = pFrustrum->topPlane * clippedY;

                float dY = fabsf(pCurVert->y - pPrevVert->y);
                float dZ = fabsf(pCurVert->z - pPrevVert->z);

                float lerpFactor = 0.0f;
                if ( dY <= (double)dZ )
                {
                    lerpFactor = (clippedZ - pPrevVert->z) / (pCurVert->z - pPrevVert->z);
                }
                else
                {
                    lerpFactor = (clippedY - pPrevVert->y) / (pCurVert->y - pPrevVert->y);
                }

                rdClip_pDestVert[clipindex].x = (pCurVert->x - pPrevVert->x) * lerpFactor + pPrevVert->x;
                rdClip_pDestVert[clipindex].y = clippedY;
                rdClip_pDestVert[clipindex].z = clippedZ;

                rdClip_pDestTVert[clipindex].x = (pCurTVert->x - pPrevTVerts->x) * lerpFactor + pPrevTVerts->x;
                rdClip_pDestTVert[clipindex].y = (pCurTVert->y - pPrevTVerts->y) * lerpFactor + pPrevTVerts->y;

                rdClip_pDestVertIntensity[clipindex].red   = (pCurIntens->red - pPrevIntens->red) * lerpFactor + pPrevIntens->red;
                rdClip_pDestVertIntensity[clipindex].green = (pCurIntens->green - pPrevIntens->green) * lerpFactor + pPrevIntens->green;
                rdClip_pDestVertIntensity[clipindex].blue  = (pCurIntens->blue - pPrevIntens->blue) * lerpFactor + pPrevIntens->blue;
                rdClip_pDestVertIntensity[clipindex].alpha = (pCurIntens->alpha - pPrevIntens->alpha) * lerpFactor + pPrevIntens->alpha;

                clipindex++;
                rdClip_g_faceStatus |= 0x04;
            }

            if ( pCurVert->z <= (double)curPlaneProjY )
            {
                rdClip_pDestVert[clipindex]          = *pCurVert;
                rdClip_pDestTVert[clipindex]         = *pCurTVert;
                rdClip_pDestVertIntensity[clipindex] = *pCurIntens;
                clipindex++;
            }
        }

        RD_ASSERTREL(clipindex < STD_ARRAYLEN(rdClip_aWorkVerts));
        pPrevVert   = pCurVert++;
        pPrevTVerts = pCurTVert++;
        pPrevIntens = pCurIntens++;
    }

    if ( clipindex < 3 )
    {
        return clipindex;
    }

    paVerts            = rdClip_pDestVert;
    rdClip_pDestVert   = rdClip_pSourceVert;
    rdClip_pSourceVert = paVerts;

    paTVerts            = rdClip_pDestTVert;
    rdClip_pDestTVert   = rdClip_pSourceTVert;
    rdClip_pSourceTVert = paTVerts;

    paIntens                    = rdClip_pDestVertIntensity;
    rdClip_pDestVertIntensity   = rdClip_pSourceVertIntensity;
    rdClip_pSourceVertIntensity = paIntens;

    numVerts    = clipindex;
    clipindex   = 0;

    pPrevVert   = &paVerts[numVerts - 1];
    pPrevTVerts = &paTVerts[numVerts - 1];
    pPrevIntens = &paIntens[numVerts - 1];

    pCurVert    = paVerts;
    pCurTVert   = paTVerts;
    pCurIntens  = paIntens;

    // BOTTOM PLANE CLIPPING
    for ( size_t i = 0; i < numVerts; ++i )
    {
        float prevPlaneProjY = pPrevVert->y * pFrustrum->bottomPlane;
        float curPlaneProjY = pCurVert->y * pFrustrum->bottomPlane;
        if ( pPrevVert->z >= (double)prevPlaneProjY || pCurVert->z >= (double)curPlaneProjY )
        {
            if ( pPrevVert->z != prevPlaneProjY
                && pCurVert->z != curPlaneProjY
                && (pPrevVert->z < prevPlaneProjY || pCurVert->z < curPlaneProjY) )
            {
                float planeProj = pFrustrum->bottomPlane * (pCurVert->y - pPrevVert->y) - (pCurVert->z - pPrevVert->z);
                float clippedY  = pPrevVert->z * pCurVert->y - pCurVert->z * pPrevVert->y;
                if ( planeProj != 0.0f ) {
                    clippedY = clippedY / planeProj;
                }

                float clippedZ = pFrustrum->bottomPlane * clippedY;

                float dY = fabsf(pCurVert->y - pPrevVert->y);
                float dZ = fabsf(pCurVert->z - pPrevVert->z);

                float lerpFactor = 0.0f;
                if ( dY <= (double)dZ )
                {
                    lerpFactor = (clippedZ - pPrevVert->z) / (pCurVert->z - pPrevVert->z);
                }
                else
                {
                    lerpFactor = (clippedY - pPrevVert->y) / (pCurVert->y - pPrevVert->y);
                }

                rdClip_pDestVert[clipindex].x = (pCurVert->x - pPrevVert->x) * lerpFactor + pPrevVert->x;
                rdClip_pDestVert[clipindex].y = clippedY;
                rdClip_pDestVert[clipindex].z = clippedZ;

                rdClip_pDestTVert[clipindex].x = (pCurTVert->x - pPrevTVerts->x) * lerpFactor + pPrevTVerts->x;
                rdClip_pDestTVert[clipindex].y = (pCurTVert->y - pPrevTVerts->y) * lerpFactor + pPrevTVerts->y;

                rdClip_pDestVertIntensity[clipindex].red   = (pCurIntens->red - pPrevIntens->red) * lerpFactor + pPrevIntens->red;
                rdClip_pDestVertIntensity[clipindex].green = (pCurIntens->green - pPrevIntens->green) * lerpFactor + pPrevIntens->green;
                rdClip_pDestVertIntensity[clipindex].blue  = (pCurIntens->blue - pPrevIntens->blue) * lerpFactor + pPrevIntens->blue;
                rdClip_pDestVertIntensity[clipindex].alpha = (pCurIntens->alpha - pPrevIntens->alpha) * lerpFactor + pPrevIntens->alpha;

                clipindex++;
                rdClip_g_faceStatus |= 0x08;
            }

            if ( pCurVert->z >= (double)curPlaneProjY )
            {
                rdClip_pDestVert[clipindex]          = *pCurVert;
                rdClip_pDestTVert[clipindex]         = *pCurTVert;
                rdClip_pDestVertIntensity[clipindex] = *pCurIntens;
                clipindex++;
            }
        }

        RD_ASSERTREL(clipindex < STD_ARRAYLEN(rdClip_aWorkVerts));
        pPrevVert   = pCurVert++;
        pPrevTVerts = pCurTVert++;
        pPrevIntens = pCurIntens++;
    }

    if ( clipindex < 3 )
    {
        return clipindex;
    }

    paVerts            = rdClip_pDestVert;
    rdClip_pDestVert   = rdClip_pSourceVert;
    rdClip_pSourceVert = paVerts;

    paTVerts            = rdClip_pDestTVert;
    rdClip_pDestTVert   = rdClip_pSourceTVert;
    rdClip_pSourceTVert = paTVerts;

    paIntens                    = rdClip_pDestVertIntensity;
    rdClip_pDestVertIntensity   = rdClip_pSourceVertIntensity;
    rdClip_pSourceVertIntensity = paIntens;

    numVerts    = clipindex;
    clipindex   = 0;

    pPrevVert   = &paVerts[numVerts - 1];
    pPrevTVerts = &paTVerts[numVerts - 1];
    pPrevIntens = &paIntens[numVerts - 1];

    pCurVert    = paVerts;
    pCurTVert   = paTVerts;
    pCurIntens  = paIntens;

    // NEAR PLANE CLIPPING
    for ( size_t i = 0; i < numVerts; ++i )
    {
        if ( pPrevVert->y >= (double)pFrustrum->nearPlane || pCurVert->y >= (double)pFrustrum->nearPlane )
        {
            if ( pPrevVert->y != pFrustrum->nearPlane
                && pCurVert->y != pFrustrum->nearPlane
                && (pPrevVert->y < (double)pFrustrum->nearPlane || pCurVert->y < (double)pFrustrum->nearPlane) )
            {
                float lerpFactor = (pFrustrum->nearPlane - pPrevVert->y) / (pCurVert->y - pPrevVert->y);
                rdClip_pDestVert[clipindex].y = pFrustrum->nearPlane;
                rdClip_pDestVert[clipindex].z = (pCurVert->z - pPrevVert->z) * lerpFactor + pPrevVert->z;
                rdClip_pDestVert[clipindex].x = (pCurVert->x - pPrevVert->x) * lerpFactor + pPrevVert->x;

                rdClip_pDestTVert[clipindex].x = (pCurTVert->x - pPrevTVerts->x) * lerpFactor + pPrevTVerts->x;
                rdClip_pDestTVert[clipindex].y = (pCurTVert->y - pPrevTVerts->y) * lerpFactor + pPrevTVerts->y;

                rdClip_pDestVertIntensity[clipindex].red   = (pCurIntens->red - pPrevIntens->red) * lerpFactor + pPrevIntens->red;
                rdClip_pDestVertIntensity[clipindex].green = (pCurIntens->green - pPrevIntens->green) * lerpFactor + pPrevIntens->green;
                rdClip_pDestVertIntensity[clipindex].blue  = (pCurIntens->blue - pPrevIntens->blue) * lerpFactor + pPrevIntens->blue;
                rdClip_pDestVertIntensity[clipindex].alpha = (pCurIntens->alpha - pPrevIntens->alpha) * lerpFactor + pPrevIntens->alpha;

                clipindex++;
                rdClip_g_faceStatus |= 0x01;
            }

            if ( pCurVert->y >= (double)pFrustrum->nearPlane )
            {
                rdClip_pDestVert[clipindex]          = *pCurVert;
                rdClip_pDestTVert[clipindex]         = *pCurTVert;
                rdClip_pDestVertIntensity[clipindex] = *pCurIntens;
                clipindex++;
            }
        }

        RD_ASSERTREL(clipindex < STD_ARRAYLEN(rdClip_aWorkVerts));
        pPrevVert   = pCurVert++;
        pPrevTVerts = pCurTVert++;
        pPrevIntens = pCurIntens++;
    }

    if ( clipindex < 3 )
    {
        rdClip_g_faceStatus |= 0x40;
        return clipindex;
    }

    if ( !pFrustrum->bClipFar )
    {
        if ( rdClip_pDestVert == aVertices )
        {
            return clipindex;
        }

        rdVector_Copy3List(aVertices, rdClip_pDestVert, clipindex);
        rdVector_Copy2List(aTexVertices, rdClip_pDestTVert, clipindex);
        rdVector_Copy4List(aIntensities, rdClip_pDestVertIntensity, clipindex);

        return clipindex;
    }

    // FAR PLANE CLIPPING
    paVerts            = rdClip_pDestVert;
    rdClip_pDestVert   = rdClip_pSourceVert;
    rdClip_pSourceVert = paVerts;

    paTVerts            = rdClip_pDestTVert;
    rdClip_pDestTVert   = rdClip_pSourceTVert;
    rdClip_pSourceTVert = paTVerts;

    paIntens                    = rdClip_pDestVertIntensity;
    rdClip_pDestVertIntensity   = rdClip_pSourceVertIntensity;
    rdClip_pSourceVertIntensity = paIntens;

    numVerts    = clipindex;
    clipindex   = 0;

    pPrevVert   = &paVerts[numVerts - 1];
    pPrevTVerts = &paTVerts[numVerts - 1];
    pPrevIntens = &paIntens[numVerts - 1];

    pCurVert    = paVerts;
    pCurTVert   = paTVerts;
    pCurIntens  = paIntens;

    for ( size_t i = 0; i < numVerts; ++i )
    {
        if ( pPrevVert->y <= (double)pFrustrum->farPlane || pCurVert->y <= (double)pFrustrum->farPlane )
        {
            if ( pPrevVert->y != pFrustrum->farPlane
                && pCurVert->y != pFrustrum->farPlane
                && (pPrevVert->y > (double)pFrustrum->farPlane || pCurVert->y > (double)pFrustrum->farPlane) )
            {
                float lerpFactor = (pFrustrum->farPlane - pPrevVert->y) / (pCurVert->y - pPrevVert->y);
                rdClip_pDestVert[clipindex].y = pFrustrum->farPlane;
                rdClip_pDestVert[clipindex].z = (pCurVert->z - pPrevVert->z) * lerpFactor + pPrevVert->z;
                rdClip_pDestVert[clipindex].x = (pCurVert->x - pPrevVert->x) * lerpFactor + pPrevVert->x;

                rdClip_pDestTVert[clipindex].x = (pCurTVert->x - pPrevTVerts->x) * lerpFactor + pPrevTVerts->x;
                rdClip_pDestTVert[clipindex].y = (pCurTVert->y - pPrevTVerts->y) * lerpFactor + pPrevTVerts->y;

                rdClip_pDestVertIntensity[clipindex].red   = (pCurIntens->red - pPrevIntens->red) * lerpFactor + pPrevIntens->red;
                rdClip_pDestVertIntensity[clipindex].green = (pCurIntens->green - pPrevIntens->green) * lerpFactor + pPrevIntens->green;
                rdClip_pDestVertIntensity[clipindex].blue  = (pCurIntens->blue - pPrevIntens->blue) * lerpFactor + pPrevIntens->blue;
                rdClip_pDestVertIntensity[clipindex].alpha = (pCurIntens->alpha - pPrevIntens->alpha) * lerpFactor + pPrevIntens->alpha;

                clipindex++;
                rdClip_g_faceStatus |= 0x02;
            }

            if ( pCurVert->y <= (double)pFrustrum->farPlane )
            {
                rdClip_pDestVert[clipindex]          = *pCurVert;
                rdClip_pDestTVert[clipindex]         = *pCurTVert;
                rdClip_pDestVertIntensity[clipindex] = *pCurIntens;
                clipindex++;
            }
        }

        RD_ASSERTREL(clipindex < STD_ARRAYLEN(rdClip_aWorkVerts));
        pPrevVert   = pCurVert++;
        pPrevTVerts = pCurTVert++;
        pPrevIntens = pCurIntens++;
    }

    if ( clipindex < 3 )
    {
        return clipindex;
    }

    if ( rdClip_pDestVert == aVertices )
    {
        return clipindex;
    }

    rdVector_Copy3List(aVertices, rdClip_pDestVert, clipindex);
    rdVector_Copy2List(aTexVertices, rdClip_pDestTVert, clipindex);
    rdVector_Copy4List(aIntensities, rdClip_pDestVertIntensity, clipindex);

    return clipindex;
}

int J3DAPI rdClip_Face3TOrtho(const rdClipFrustum* pFrustum, rdVector3* aVertices, rdVector2* aTexVertices, rdVector4* aIntensities, size_t numVertices)
{
    rdClip_g_faceStatus = 0;

    rdClip_pSourceVert = aVertices;
    rdClip_pDestVert   = rdClip_aWorkVerts;

    rdClip_pSourceTVert = aTexVertices;
    rdClip_pDestTVert   = rdClip_aWorkTVerts;

    rdClip_pSourceVertIntensity = aIntensities;
    rdClip_pDestVertIntensity   = rdClip_aWorkVertIntensities;

    size_t clipindex       = 0;

    rdVector3* pPrevVert   = &aVertices[numVertices - 1];
    rdVector2* pPrevTVert  = &aTexVertices[numVertices - 1];
    rdVector4* pPrevIntens = &aIntensities[numVertices - 1];

    rdVector3* pCurVert    = aVertices;
    rdVector2* pCurTVert   = aTexVertices;
    rdVector4* pCurIntens  = aIntensities;

    for ( size_t i = 0; i < numVertices; ++i )
    {
        if ( pPrevVert->x >= pFrustum->orthoLeftPlane || pCurVert->x >= pFrustum->orthoLeftPlane )
        {
            if ( pPrevVert->x != pFrustum->orthoLeftPlane
                && pCurVert->x != pFrustum->orthoLeftPlane
                && (pPrevVert->x < pFrustum->orthoLeftPlane || pCurVert->x < pFrustum->orthoLeftPlane) )
            {
                float lerpFactor = (pFrustum->orthoLeftPlane - pPrevVert->x) / (pCurVert->x - pPrevVert->x);
                rdClip_pDestVert[clipindex].x = pFrustum->orthoLeftPlane;
                rdClip_pDestVert[clipindex].y = (pCurVert->y - pPrevVert->y) * lerpFactor + pPrevVert->y;
                rdClip_pDestVert[clipindex].z = (pCurVert->z - pPrevVert->z) * lerpFactor + pPrevVert->z;

                rdClip_pDestTVert[clipindex].x = (pCurTVert->x - pPrevTVert->x) * lerpFactor + pPrevTVert->x;
                rdClip_pDestTVert[clipindex].y = (pCurTVert->y - pPrevTVert->y) * lerpFactor + pPrevTVert->y;

                rdClip_pDestVertIntensity[clipindex].x   = (pCurIntens->x - pPrevIntens->x) * lerpFactor + pPrevIntens->x;
                rdClip_pDestVertIntensity[clipindex].y   = (pCurIntens->y - pPrevIntens->y) * lerpFactor + pPrevIntens->y;
                rdClip_pDestVertIntensity[clipindex++].z = (pCurIntens->z - pPrevIntens->z) * lerpFactor + pPrevIntens->z;

                rdClip_g_faceStatus |= 0x10;
            }

            if ( pCurVert->x >= pFrustum->orthoLeftPlane )
            {
                memcpy(&rdClip_pDestVert[clipindex], pCurVert, sizeof(rdVector3));
                memcpy(&rdClip_pDestTVert[clipindex], pCurTVert, sizeof(*rdClip_pDestTVert));
                memcpy(&rdClip_pDestVertIntensity[clipindex++], pCurIntens, sizeof(*rdClip_pDestVertIntensity));
            }
        }

        RD_ASSERTREL(clipindex < STD_ARRAYLEN(rdClip_aWorkVerts));
        pPrevVert   = pCurVert++;
        pPrevTVert  = pCurTVert++;
        pPrevIntens = pCurIntens++;
    }

    if ( clipindex < 3 )
    {
        return clipindex;
    }

    rdVector3* paVerts = rdClip_pDestVert;
    rdClip_pDestVert = rdClip_pSourceVert;
    rdClip_pSourceVert = paVerts;

    rdVector2* paTVerts = rdClip_pDestTVert;
    rdClip_pDestTVert   = rdClip_pSourceTVert;
    rdClip_pSourceTVert = paTVerts;

    rdVector4* paIntens         = rdClip_pDestVertIntensity;
    rdClip_pDestVertIntensity   = rdClip_pSourceVertIntensity;
    rdClip_pSourceVertIntensity = paIntens;

    size_t numVerts = clipindex;
    clipindex       = 0;
    pPrevVert       = &paVerts[numVerts - 1];
    pPrevTVert      = &paTVerts[numVerts - 1];
    pPrevIntens     = &paIntens[numVerts - 1];
    pCurVert        = paVerts;
    pCurTVert       = paTVerts;
    pCurIntens      = paIntens;

    for ( size_t i = 0; i < numVerts; ++i )
    {
        if ( pPrevVert->x <= pFrustum->orthoRightPlane || pCurVert->x <= pFrustum->orthoRightPlane )
        {
            if ( pPrevVert->x != pFrustum->orthoRightPlane
                && pCurVert->x != pFrustum->orthoRightPlane
                && (pPrevVert->x > pFrustum->orthoRightPlane || pCurVert->x > pFrustum->orthoRightPlane) )
            {
                float lerpFactor = (pFrustum->orthoRightPlane - pPrevVert->x) / (pCurVert->x - pPrevVert->x);
                rdClip_pDestVert[clipindex].x = pFrustum->orthoRightPlane;
                rdClip_pDestVert[clipindex].y = (pCurVert->y - pPrevVert->y) * lerpFactor + pPrevVert->y;
                rdClip_pDestVert[clipindex].z = (pCurVert->z - pPrevVert->z) * lerpFactor + pPrevVert->z;

                rdClip_pDestTVert[clipindex].x = (pCurTVert->x - pPrevTVert->x) * lerpFactor + pPrevTVert->x;
                rdClip_pDestTVert[clipindex].y = (pCurTVert->y - pPrevTVert->y) * lerpFactor + pPrevTVert->y;

                rdClip_pDestVertIntensity[clipindex].x   = (pCurIntens->x - pPrevIntens->x) * lerpFactor + pPrevIntens->x;
                rdClip_pDestVertIntensity[clipindex].y   = (pCurIntens->y - pPrevIntens->y) * lerpFactor + pPrevIntens->y;
                rdClip_pDestVertIntensity[clipindex++].z = (pCurIntens->z - pPrevIntens->z) * lerpFactor + pPrevIntens->z;

                rdClip_g_faceStatus |= 0x20;
            }

            if ( pCurVert->x <= pFrustum->orthoRightPlane )
            {
                memcpy(&rdClip_pDestVert[clipindex], pCurVert, sizeof(rdVector3));
                memcpy(&rdClip_pDestTVert[clipindex], pCurTVert, sizeof(*rdClip_pDestTVert));
                memcpy(&rdClip_pDestVertIntensity[clipindex++], pCurIntens, sizeof(*rdClip_pDestVertIntensity));
            }
        }

        RD_ASSERTREL(clipindex < STD_ARRAYLEN(rdClip_aWorkVerts));
        pPrevVert   = pCurVert++;
        pPrevTVert  = pCurTVert++;
        pPrevIntens = pCurIntens++;
    }

    if ( clipindex < 3 )
    {
        return clipindex;
    }

    paVerts = rdClip_pDestVert;
    rdClip_pDestVert   = rdClip_pSourceVert;
    rdClip_pSourceVert = paVerts;

    paTVerts = rdClip_pDestTVert;
    rdClip_pDestTVert   = rdClip_pSourceTVert;
    rdClip_pSourceTVert = paTVerts;

    paIntens                    = rdClip_pDestVertIntensity;
    rdClip_pDestVertIntensity   = rdClip_pSourceVertIntensity;
    rdClip_pSourceVertIntensity = paIntens;

    numVerts        = clipindex;
    clipindex       = 0;

    pPrevVert       = &paVerts[numVerts - 1];
    pPrevTVert      = &paTVerts[numVerts - 1];
    pPrevIntens     = &paIntens[numVerts - 1];

    pCurVert        = paVerts;
    pCurTVert       = paTVerts;
    pCurIntens      = paIntens;

    for ( size_t i = 0; i < numVerts; ++i )
    {
        if ( pPrevVert->z <= pFrustum->orthoTopPlane || pCurVert->z <= pFrustum->orthoTopPlane )
        {
            if ( pPrevVert->z != pFrustum->orthoTopPlane && pCurVert->z != pFrustum->orthoTopPlane && (pPrevVert->z > pFrustum->orthoTopPlane || pCurVert->z > pFrustum->orthoTopPlane) )
            {
                float lerpFactor = (pFrustum->orthoTopPlane - pPrevVert->z) / (pCurVert->z - pPrevVert->z);
                rdClip_pDestVert[clipindex].x = (pCurVert->x - pPrevVert->x) * lerpFactor + pPrevVert->x;
                rdClip_pDestVert[clipindex].y = (pCurVert->y - pPrevVert->y) * lerpFactor + pPrevVert->y;
                rdClip_pDestVert[clipindex].z = pFrustum->orthoTopPlane;

                rdClip_pDestTVert[clipindex].x = (pCurTVert->x - pPrevTVert->x) * lerpFactor + pPrevTVert->x;
                rdClip_pDestTVert[clipindex].y = (pCurTVert->y - pPrevTVert->y) * lerpFactor + pPrevTVert->y;

                rdClip_pDestVertIntensity[clipindex].x   = (pCurIntens->x - pPrevIntens->x) * lerpFactor + pPrevIntens->x;
                rdClip_pDestVertIntensity[clipindex].y   = (pCurIntens->y - pPrevIntens->y) * lerpFactor + pPrevIntens->y;
                rdClip_pDestVertIntensity[clipindex++].z = (pCurIntens->z - pPrevIntens->z) * lerpFactor + pPrevIntens->z;

                rdClip_g_faceStatus |= 0x04;
            }

            if ( pCurVert->z <= pFrustum->orthoTopPlane )
            {
                memcpy(&rdClip_pDestVert[clipindex], pCurVert, sizeof(rdVector3));
                memcpy(&rdClip_pDestTVert[clipindex], pCurTVert, sizeof(*rdClip_pDestTVert));
                memcpy(&rdClip_pDestVertIntensity[clipindex++], pCurIntens, sizeof(*rdClip_pDestVertIntensity));
            }
        }

        RD_ASSERTREL(clipindex < STD_ARRAYLEN(rdClip_aWorkVerts));
        pPrevVert   = pCurVert++;
        pPrevTVert  = pCurTVert++;
        pPrevIntens = pCurIntens++;
    }

    if ( clipindex < 3 )
    {
        return clipindex;
    }

    paVerts = rdClip_pDestVert;
    rdClip_pDestVert   = rdClip_pSourceVert;
    rdClip_pSourceVert = paVerts;

    paTVerts = rdClip_pDestTVert;
    rdClip_pDestTVert   = rdClip_pSourceTVert;
    rdClip_pSourceTVert = paTVerts;

    paIntens                    = rdClip_pDestVertIntensity;
    rdClip_pDestVertIntensity   = rdClip_pSourceVertIntensity;
    rdClip_pSourceVertIntensity = paIntens;

    numVerts  = clipindex;
    clipindex = 0;

    pPrevVert   = &paVerts[numVerts - 1];
    pPrevTVert  = &paTVerts[numVerts - 1];
    pPrevIntens = &paIntens[numVerts - 1];

    pCurVert   = paVerts;
    pCurTVert  = paTVerts;
    pCurIntens = paIntens;

    for ( size_t i = 0; i < numVerts; ++i )
    {
        if ( pPrevVert->z >= pFrustum->orthoBottomPlane || pCurVert->z >= pFrustum->orthoBottomPlane )
        {
            if ( pPrevVert->z != pFrustum->orthoBottomPlane
                && pCurVert->z != pFrustum->orthoBottomPlane
                && (pPrevVert->z < pFrustum->orthoBottomPlane || pCurVert->z < pFrustum->orthoBottomPlane) )
            {
                float lerpFactor = (pFrustum->orthoBottomPlane - pPrevVert->z) / (pCurVert->z - pPrevVert->z);
                rdClip_pDestVert[clipindex].x = (pCurVert->x - pPrevVert->x) * lerpFactor + pPrevVert->x;
                rdClip_pDestVert[clipindex].y = (pCurVert->y - pPrevVert->y) * lerpFactor + pPrevVert->y;
                rdClip_pDestVert[clipindex].z = pFrustum->orthoBottomPlane;

                rdClip_pDestTVert[clipindex].x = (pCurTVert->x - pPrevTVert->x) * lerpFactor + pPrevTVert->x;
                rdClip_pDestTVert[clipindex].y = (pCurTVert->y - pPrevTVert->y) * lerpFactor + pPrevTVert->y;

                rdClip_pDestVertIntensity[clipindex].x   = (pCurIntens->x - pPrevIntens->x) * lerpFactor + pPrevIntens->x;
                rdClip_pDestVertIntensity[clipindex].y   = (pCurIntens->y - pPrevIntens->y) * lerpFactor + pPrevIntens->y;
                rdClip_pDestVertIntensity[clipindex++].z = (pCurIntens->z - pPrevIntens->z) * lerpFactor + pPrevIntens->z;

                rdClip_g_faceStatus |= 0x08;
            }

            if ( pCurVert->z >= pFrustum->orthoBottomPlane )
            {
                memcpy(&rdClip_pDestVert[clipindex], pCurVert, sizeof(rdVector3));
                memcpy(&rdClip_pDestTVert[clipindex], pCurTVert, sizeof(*rdClip_pDestTVert));
                memcpy(&rdClip_pDestVertIntensity[clipindex++], pCurIntens, sizeof(*rdClip_pDestVertIntensity));
            }
        }

        RD_ASSERTREL(clipindex < STD_ARRAYLEN(rdClip_aWorkVerts));
        pPrevVert   = pCurVert++;
        pPrevTVert  = pCurTVert++;
        pPrevIntens = pCurIntens++;
    }

    if ( clipindex < 3 )
    {
        return clipindex;
    }

    paVerts = rdClip_pDestVert;
    rdClip_pDestVert   = rdClip_pSourceVert;
    rdClip_pSourceVert = paVerts;

    paTVerts = rdClip_pDestTVert;
    rdClip_pDestTVert   = rdClip_pSourceTVert;
    rdClip_pSourceTVert = paTVerts;

    paIntens                    = rdClip_pDestVertIntensity;
    rdClip_pDestVertIntensity   = rdClip_pSourceVertIntensity;
    rdClip_pSourceVertIntensity = paIntens;

    numVerts  = clipindex;
    clipindex = 0;

    pPrevVert   = &paVerts[numVerts - 1];
    pPrevTVert  = &paTVerts[numVerts - 1];
    pPrevIntens = &paIntens[numVerts - 1];

    pCurVert   = paVerts;
    pCurTVert  = paTVerts;
    pCurIntens = paIntens;

    for ( size_t i = 0; i < numVerts; ++i )
    {
        if ( pPrevVert->y >= pFrustum->nearPlane || pCurVert->y >= pFrustum->nearPlane )
        {
            if ( pPrevVert->y != pFrustum->nearPlane
                && pCurVert->y != pFrustum->nearPlane
                && (pPrevVert->y < pFrustum->nearPlane || pCurVert->y < pFrustum->nearPlane) )
            {
                float lerpFactor = (pFrustum->nearPlane - pPrevVert->y) / (pCurVert->y - pPrevVert->y);
                rdClip_pDestVert[clipindex].y = pFrustum->nearPlane;
                rdClip_pDestVert[clipindex].z = (pCurVert->z - pPrevVert->z) * lerpFactor + pPrevVert->z;
                rdClip_pDestVert[clipindex].x = (pCurVert->x - pPrevVert->x) * lerpFactor + pPrevVert->x;

                rdClip_pDestTVert[clipindex].x = (pCurTVert->x - pPrevTVert->x) * lerpFactor + pPrevTVert->x;
                rdClip_pDestTVert[clipindex].y = (pCurTVert->y - pPrevTVert->y) * lerpFactor + pPrevTVert->y;

                rdClip_pDestVertIntensity[clipindex].x   = (pCurIntens->x - pPrevIntens->x) * lerpFactor + pPrevIntens->x;
                rdClip_pDestVertIntensity[clipindex].y   = (pCurIntens->y - pPrevIntens->y) * lerpFactor + pPrevIntens->y;
                rdClip_pDestVertIntensity[clipindex++].z = (pCurIntens->z - pPrevIntens->z) * lerpFactor + pPrevIntens->z;

                rdClip_g_faceStatus |= 0x01;
            }

            if ( pCurVert->y >= pFrustum->nearPlane )
            {
                memcpy(&rdClip_pDestVert[clipindex], pCurVert, sizeof(rdVector3));
                memcpy(&rdClip_pDestTVert[clipindex], pCurTVert, sizeof(*rdClip_pDestTVert));
                memcpy(&rdClip_pDestVertIntensity[clipindex++], pCurIntens, sizeof(*rdClip_pDestVertIntensity));

            }
        }

        RD_ASSERTREL(clipindex < STD_ARRAYLEN(rdClip_aWorkVerts));
        pPrevVert   = pCurVert++;
        pPrevTVert  = pCurTVert++;
        pPrevIntens = pCurIntens++;
    }

    if ( clipindex < 3 )
    {
        rdClip_g_faceStatus |= 0x40;
        return clipindex;
    }

    if ( !pFrustum->bClipFar )
    {
        if ( rdClip_pDestVert == aVertices )
        {
            return clipindex;
        }

        rdVector_Copy3List(aVertices, rdClip_pDestVert, clipindex);
        rdVector_Copy2List(aTexVertices, rdClip_pDestTVert, clipindex);
        rdVector_Copy4List(aIntensities, rdClip_pDestVertIntensity, clipindex);
        return clipindex;
    }

    paVerts = rdClip_pDestVert;
    rdClip_pDestVert   = rdClip_pSourceVert;
    rdClip_pSourceVert = paVerts;

    paTVerts = rdClip_pDestTVert;
    rdClip_pDestTVert   = rdClip_pSourceTVert;
    rdClip_pSourceTVert = paTVerts;

    paIntens                    = rdClip_pDestVertIntensity;
    rdClip_pDestVertIntensity   = rdClip_pSourceVertIntensity;
    rdClip_pSourceVertIntensity = paIntens;

    numVerts  = clipindex;
    clipindex = 0;

    pPrevVert   = &paVerts[numVerts - 1];
    pPrevTVert  = &paTVerts[numVerts - 1];
    pPrevIntens = &paIntens[numVerts - 1];

    pCurVert   = paVerts;
    pCurTVert  = paTVerts;
    pCurIntens = paIntens;

    for ( size_t i = 0; i < numVerts; ++i )
    {
        if ( pPrevVert->y <= pFrustum->farPlane || pCurVert->y <= pFrustum->farPlane )
        {
            if ( pPrevVert->y != pFrustum->farPlane
                && pCurVert->y != pFrustum->farPlane
                && (pPrevVert->y > pFrustum->farPlane || pCurVert->y > pFrustum->farPlane) )
            {
                float lerpFactor = (pFrustum->farPlane - pPrevVert->y) / (pCurVert->y - pPrevVert->y);
                rdClip_pDestVert[clipindex].y = pFrustum->farPlane;
                rdClip_pDestVert[clipindex].z = (pCurVert->z - pPrevVert->z) * lerpFactor + pPrevVert->z;
                rdClip_pDestVert[clipindex].x = (pCurVert->x - pPrevVert->x) * lerpFactor + pPrevVert->x;

                rdClip_pDestTVert[clipindex].x = (pCurTVert->x - pPrevTVert->x) * lerpFactor + pPrevTVert->x;
                rdClip_pDestTVert[clipindex].y = (pCurTVert->y - pPrevTVert->y) * lerpFactor + pPrevTVert->y;

                rdClip_pDestVertIntensity[clipindex].x   = (pCurIntens->x - pPrevIntens->x) * lerpFactor + pPrevIntens->x;
                rdClip_pDestVertIntensity[clipindex].y   = (pCurIntens->y - pPrevIntens->y) * lerpFactor + pPrevIntens->y;
                rdClip_pDestVertIntensity[clipindex++].z = (pCurIntens->z - pPrevIntens->z) * lerpFactor + pPrevIntens->z;

                rdClip_g_faceStatus |= 0x02;
            }

            if ( pCurVert->y <= pFrustum->farPlane )
            {
                memcpy(&rdClip_pDestVert[clipindex], pCurVert, sizeof(rdVector3));
                memcpy(&rdClip_pDestTVert[clipindex++], pCurTVert, sizeof(*rdClip_pDestTVert));
                memcpy(&rdClip_pDestVertIntensity[clipindex++], pCurIntens, sizeof(*rdClip_pDestVertIntensity));
            }
        }

        RD_ASSERTREL(clipindex < STD_ARRAYLEN(rdClip_aWorkVerts));
        pPrevVert   = pCurVert++;
        pPrevTVert  = pCurTVert++;
        pPrevIntens = pCurIntens++;
    }

    if ( clipindex < 3 )
    {
        return clipindex;
    }

    if ( rdClip_pDestVert == aVertices )
    {
        return clipindex;
    }

    rdVector_Copy3List(aVertices, rdClip_pDestVert, clipindex);
    rdVector_Copy2List(aTexVertices, rdClip_pDestTVert, clipindex);
    rdVector_Copy4List(aIntensities, rdClip_pDestVertIntensity, clipindex);
    return clipindex;
}

RdFrustumCull J3DAPI rdClip_SphereInFrustrum(const rdClipFrustum* pFrustum, const rdVector3* pCenter, float radius)
{
    int bInFrustum = 1;

    float fwdDist  = pCenter->y + radius;
    float backDist = pCenter->y - radius;

    if ( fwdDist < pFrustum->nearPlane )
    {
        return RDFRUSTUMCULL_OUTSIDE;
    }

    if ( backDist < pFrustum->nearPlane )
    {
        bInFrustum = 0;
    }

    if ( pFrustum->bClipFar )
    {
        if ( backDist > pFrustum->farPlane )
        {
            return RDFRUSTUMCULL_OUTSIDE;
        }

        if ( fwdDist > pFrustum->farPlane )
        {
            bInFrustum = 0;
        }
    }

    float planeDist = pFrustum->topPlaneNormal.y * pCenter->y + pFrustum->topPlaneNormal.z * pCenter->z;
    if ( planeDist > radius )
    {
        return RDFRUSTUMCULL_OUTSIDE;
    }

    if ( -radius < planeDist )
    {
        bInFrustum = 0;
    }

    planeDist = pFrustum->bottomPlaneNormal.y * pCenter->y + pFrustum->bottomPlaneNormal.z * pCenter->z;
    if ( planeDist > radius )
    {
        return RDFRUSTUMCULL_OUTSIDE;
    }

    if ( -radius < planeDist )
    {
        bInFrustum = 0;
    }

    planeDist = pFrustum->leftPlaneNormal.y * pCenter->y + pFrustum->leftPlaneNormal.x * pCenter->x;
    if ( planeDist > radius )
    {
        return RDFRUSTUMCULL_OUTSIDE;
    }

    if ( -radius < planeDist )
    {
        bInFrustum = 0;
    }

    planeDist = pFrustum->rightPlaneNormal.y * pCenter->y + pFrustum->rightPlaneNormal.x * pCenter->x;
    if ( planeDist > radius )
    {
        return RDFRUSTUMCULL_OUTSIDE;
    }

    if ( -radius < planeDist )
    {
        bInFrustum = 0;
    }

    return bInFrustum == 0 ? RDFRUSTUMCULL_INTERSECT : RDFRUSTUMCULL_INSIDE;
}

int J3DAPI rdClip_QClipFaceT(rdClipFrustum* pFrustrum, const rdPrimit3* pSrc, rdPrimit3* pDest, const rdVector2* pTVOffset)
{
    if ( pSrc->aVertIntensities )
    {
        for ( size_t i = 0; i < pSrc->numVertices; ++i )
        {
            pDest->aVertices[i] = pSrc->aVertices[pSrc->aVertIdxs[i]];

            pDest->aTexVertices[i], pSrc->aTexVertices[pSrc->aTexVertIdxs[i]];
            rdVector_Add2Acc(&pDest->aTexVertices[i], pTVOffset);

            rdVector_Add4(&pDest->aVertLights[i], &pSrc->aVertLights[pSrc->aVertIdxs[i]], &pSrc->aVertIntensities[i]);
            rdMath_ClampVector4Acc(&pDest->aVertLights[i], 0.0f, 1.0f);
        }
    }
    else if ( pSrc->aVertLights ) // Fixed: Check for not null
    {
        for ( size_t i = 0; i < pSrc->numVertices; ++i )
        {
            pDest->aVertices[i] = pSrc->aVertices[pSrc->aVertIdxs[i]];

            pDest->aTexVertices[i] = pSrc->aTexVertices[pSrc->aTexVertIdxs[i]];
            rdVector_Add2Acc(&pDest->aTexVertices[i], pTVOffset);

            pDest->aVertLights[i] = pSrc->aVertLights[pSrc->aVertIdxs[i]];
            rdMath_ClampVector4Acc(&pDest->aVertLights[i], 0.0f, 1.0f);
        }
    }
    else // Added
    {
        for ( size_t i = 0; i < pSrc->numVertices; ++i )
        {
            pDest->aVertices[i] = pSrc->aVertices[pSrc->aVertIdxs[i]];

            pDest->aTexVertices[i] = pSrc->aTexVertices[pSrc->aTexVertIdxs[i]];
            rdVector_Add2Acc(&pDest->aTexVertices[i], pTVOffset);
        }
    }

    if ( rdCamera_g_pCurCamera->projectType == RDCAMERA_PROJECT_PERSPECTIVE )
    {
        pDest->numVertices = rdQClip_VerticesInFrustrum(pFrustrum, pDest->aVertices, pSrc->numVertices);
        if ( pDest->numVertices )
        {
            pDest->numVertices = rdQClip_Face3T(pFrustrum, pDest->aVertices, pDest->aTexVertices, pDest->aVertLights, pSrc->numVertices);
        }
    }
    else
    {
        pDest->numVertices = rdClip_Face3TOrtho(pFrustrum, pDest->aVertices, pDest->aTexVertices, pDest->aVertLights, pSrc->numVertices);
    }

    return pDest->numVertices;
}

void J3DAPI rdClip_QClipFaceW(const rdClipFrustum* pFrustrum, const rdPrimit3* pSrc, rdPrimit3* pDst)
{

    for ( size_t i = 0; i < pSrc->numVertices; ++i )
    {
        pDst->aVertices[i] = pSrc->aVertices[pSrc->aVertIdxs[i]];
    }

    pDst->numVertices = rdQClip_VerticesInFrustrum(pFrustrum, pDst->aVertices, pSrc->numVertices);
    if ( pDst->numVertices )
    {
        pDst->numVertices = rdQClip_Face3W(pFrustrum, pDst->aVertices, pSrc->numVertices);
    }
}

int J3DAPI rdClip_FaceToPlane(const rdClipFrustum* pFrustrum, rdCacheProcEntry* pProcFace, const rdFace* pFace, const rdVector3* aVerts, const rdVector2* aTexVerts, const rdVector4* aLightColors, const rdVector4* aVertColors)
{
#ifdef J3D_OPENGL
    if ( std3D_GetCurrentDrawState() != STD3D_DS_HUD )
    {
        rdClip_AddFaceVertices(pProcFace, pFace, aVerts, aTexVerts, aLightColors, aVertColors);
        return 1;
    }
#endif

    // Clip and transform face vertices to screen space then assigns pProcFace

    // Added bounds check
    RD_ASSERTREL(pFace->numVertices < STD_ARRAYLEN(rdClip_aWorkFaceVerts));

    for ( size_t i = 0; i < pFace->numVertices; ++i )
    {
        rdClip_aWorkFaceVerts[i] = aVerts[pFace->aVertices[i]];
    }

    if ( !rdQClip_VerticesInFrustrum(pFrustrum, rdClip_aWorkFaceVerts, pFace->numVertices) )
    {
        return 0;
    }

    float ccenterX = rdCamera_g_pCurCamera->pCanvas->center.x;
    float ccenterY = rdCamera_g_pCurCamera->pCanvas->center.y;

    float invNearPlane = rdCamera_g_pCurCamera->invNearClipPlane;
    float invFarPlane  = rdCamera_g_pCurCamera->invFarClipPlane;
    float focalLength  = rdCamera_g_pCurCamera->focalLength * rdCamera_g_pCurCamera->aspectRatio; // Fixed: Multiplied focalLength by aspectRatio to account camera aspect ratio

    if ( aLightColors )
    {
        if ( aVertColors )
        {
            for ( size_t i = 0; i < pFace->numVertices; ++i )
            {
                float invY = 1.0f / rdClip_aWorkFaceVerts[i].y;
                float  scale = focalLength * invY;

                LPD3DTLVERTEX pOutVert = &pProcFace->aVertices[i];
                pOutVert->sx = rdClip_aWorkFaceVerts[i].x * scale + ccenterX;
                pOutVert->sy = ccenterY - rdClip_aWorkFaceVerts[i].z * scale;
                pOutVert->sz = (invY - invNearPlane) * invFarPlane;

                pOutVert->rhw = invY / 32;

                pOutVert->tu = aTexVerts[pFace->aTexVertices[i]].x + pFace->texVertOffset.x;
                pOutVert->tv = aTexVerts[pFace->aTexVertices[i]].y + pFace->texVertOffset.y;

                int vertIdx = pFace->aVertices[i];
                rdVector_Add4(&pProcFace->aVertIntensities[i], &aLightColors[vertIdx], &aVertColors[i]);
                rdMath_ClampVector4Acc(&pProcFace->aVertIntensities[i], 0.0f, 1.0f);
            }
        }
        else
        {
            for ( size_t i = 0; i < pFace->numVertices; ++i )
            {
                float invY = 1.0f / rdClip_aWorkFaceVerts[i].y;
                float  scale = focalLength * invY;

                LPD3DTLVERTEX pOutVert = &pProcFace->aVertices[i];
                pOutVert->sx = rdClip_aWorkFaceVerts[i].x * scale + ccenterX;
                pOutVert->sy = ccenterY - rdClip_aWorkFaceVerts[i].z * scale;
                pOutVert->sz = (invY - invNearPlane) * invFarPlane;

                pOutVert->rhw = invY / 32;

                pOutVert->tu = aTexVerts[pFace->aTexVertices[i]].x + pFace->texVertOffset.x;
                pOutVert->tv = aTexVerts[pFace->aTexVertices[i]].y + pFace->texVertOffset.y;

                pProcFace->aVertIntensities[i] = aLightColors[pFace->aVertices[i]];
            }
        }
    }
    else
    {
        for ( size_t i = 0; i < pFace->numVertices; ++i )
        {
            float invY  = 1.0f / rdClip_aWorkFaceVerts[i].y;
            float  scale = focalLength * invY;

            LPD3DTLVERTEX pOutVert = &pProcFace->aVertices[i];
            pOutVert->sx = rdClip_aWorkFaceVerts[i].x * scale + ccenterX;
            pOutVert->sy = ccenterY - rdClip_aWorkFaceVerts[i].z * scale;
            pOutVert->sz = (invY - invNearPlane) * invFarPlane;

            pOutVert->rhw = invY / 32;

            pOutVert->tu = aTexVerts[pFace->aTexVertices[i]].x + pFace->texVertOffset.x;
            pOutVert->tv = aTexVerts[pFace->aTexVertices[i]].y + pFace->texVertOffset.y;
        }
    }

    return 1;
}

void J3DAPI rdClip_VerticesToPlane(rdCacheProcEntry* pProcFace, const rdVector3* aVerts, const rdVector2* aTexVerts, size_t numVerts)
{
#ifdef J3D_OPENGL
    if ( std3D_GetCurrentDrawState() != STD3D_DS_HUD )
    {
        rdClip_AddVertices(pProcFace, aVerts, aTexVerts, numVerts);
        return;
    }
#endif

    // Transform vertices to screen space then assigns pProcFace

    float ccenterX = rdCamera_g_pCurCamera->pCanvas->center.x;
    float ccenterY = rdCamera_g_pCurCamera->pCanvas->center.y;

    float invNearClipPlane = rdCamera_g_pCurCamera->invNearClipPlane;
    float invFarClipPlane  = rdCamera_g_pCurCamera->invFarClipPlane;
    float focalLength      = rdCamera_g_pCurCamera->focalLength * rdCamera_g_pCurCamera->aspectRatio; // Fixed: Multiplied focalLength by aspectRatio to account camera aspect ratio

    for ( size_t i = 0; i < numVerts; ++i )
    {
        float invY = 1.0f / aVerts[i].y;
        float scale = focalLength * invY;

        LPD3DTLVERTEX pOutVert = &pProcFace->aVertices[i];
        pOutVert->sx = aVerts[i].x * scale + ccenterX;
        pOutVert->sy = ccenterY - aVerts[i].z * scale;
        pOutVert->sz = (invY - invNearClipPlane) * invFarClipPlane;

        pOutVert->rhw = invY / 32;

        pOutVert->tu = aTexVerts[i].x;
        pOutVert->tv = aTexVerts[i].y;
    }
}

#ifdef J3D_OPENGL
void J3DAPI rdClip_AddFaceVertices(rdCacheProcEntry* pProcFace, const rdFace* pFace, const rdVector3* aVerts, const rdVector2* aTexVerts, const rdVector4* aLightColors, const rdVector4* aVertColors)
{
    for ( size_t i = 0; i < pFace->numVertices; ++i )
    {
        rdClip_aWorkFaceVerts[i] = aVerts[pFace->aVertices[i]];
    }

    if ( aLightColors )
    {
        if ( aVertColors )
        {
            for ( size_t i = 0; i < pFace->numVertices; ++i )
            {
                LPD3DTLVERTEX pOutVert = &pProcFace->aVertices[i];
                pOutVert->sx           = rdClip_aWorkFaceVerts[i].x;
                pOutVert->sy           = rdClip_aWorkFaceVerts[i].z;
                pOutVert->sz           = -rdClip_aWorkFaceVerts[i].y;

                pOutVert->rhw = 1.0f;

                pOutVert->tu = aTexVerts[pFace->aTexVertices[i]].x + pFace->texVertOffset.x;
                pOutVert->tv = aTexVerts[pFace->aTexVertices[i]].y + pFace->texVertOffset.y;

                int vertIdx = pFace->aVertices[i];
                rdVector_Add4(&pProcFace->aVertIntensities[i], &aLightColors[vertIdx], &aVertColors[i]);
                rdMath_ClampVector4Acc(&pProcFace->aVertIntensities[i], 0.0f, 1.0f);
            }
        }
        else
        {
            for ( size_t i = 0; i < pFace->numVertices; ++i )
            {
                LPD3DTLVERTEX pOutVert = &pProcFace->aVertices[i];
                pOutVert->sx           = rdClip_aWorkFaceVerts[i].x;
                pOutVert->sy           = rdClip_aWorkFaceVerts[i].z;
                pOutVert->sz           = -rdClip_aWorkFaceVerts[i].y;

                pOutVert->rhw = 1.0f;

                pOutVert->tu = aTexVerts[pFace->aTexVertices[i]].x + pFace->texVertOffset.x;
                pOutVert->tv = aTexVerts[pFace->aTexVertices[i]].y + pFace->texVertOffset.y;

                pOutVert->tu = aTexVerts[pFace->aTexVertices[i]].x + pFace->texVertOffset.x;
                pOutVert->tv = aTexVerts[pFace->aTexVertices[i]].y + pFace->texVertOffset.y;

                pProcFace->aVertIntensities[i] = aLightColors[pFace->aVertices[i]];
            }
        }
    }
    else
    {
        for ( size_t i = 0; i < pFace->numVertices; ++i )
        {
            LPD3DTLVERTEX pOutVert = &pProcFace->aVertices[i];
            pOutVert->sx           = rdClip_aWorkFaceVerts[i].x;
            pOutVert->sy           = rdClip_aWorkFaceVerts[i].z;
            pOutVert->sz           = -rdClip_aWorkFaceVerts[i].y;

            pOutVert->rhw = 1.0f;

            pOutVert->tu = aTexVerts[pFace->aTexVertices[i]].x + pFace->texVertOffset.x;
            pOutVert->tv = aTexVerts[pFace->aTexVertices[i]].y + pFace->texVertOffset.y;

            pOutVert->tu = aTexVerts[pFace->aTexVertices[i]].x + pFace->texVertOffset.x;
            pOutVert->tv = aTexVerts[pFace->aTexVertices[i]].y + pFace->texVertOffset.y;
        }
    }
}

void J3DAPI rdClip_AddVertices(rdCacheProcEntry* pProcFace, const rdVector3* aVerts, const rdVector2* aTexVerts, size_t numVerts)
{
    // Transform vertices to screen space then assigns pProcFace


    for ( size_t i = 0; i < numVerts; ++i )
    {
        LPD3DTLVERTEX pOutVert = &pProcFace->aVertices[i];
        pOutVert->sx           = aVerts[i].x;
        pOutVert->sy           = aVerts[i].z;
        pOutVert->sz           = -aVerts[i].y;

        pOutVert->rhw = 1.0f;

        pOutVert->tu = aTexVerts[i].x;
        pOutVert->tv = aTexVerts[i].y;
    }
}
#endif

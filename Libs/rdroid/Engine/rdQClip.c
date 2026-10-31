#include "rdQClip.h"
#include <j3dcore/j3dhook.h>

#include <rdroid/Engine/rdClip.h>
#include <rdroid/Main/rdroid.h>
#include <rdroid/Math/rdVector.h>
#include <rdroid/RTI/symbols.h>

#include <std/General/stdUtil.h>

static rdVector3* rdQClip_pDestVert   = NULL;
static rdVector3* rdQClip_pSourceVert = NULL;
static rdVector3 rdQClip_aWorkVerts[RDQCLIP_MAXWORKVERTS];

static rdVector2* rdQClip_pSourceTVert = NULL;
static rdVector2* rdQClip_pDestTVert   = NULL;
static rdVector2 rdQClip_aWorkTVerts[RDQCLIP_MAXWORKVERTS];

static rdVector4* rdQClip_pSourceVertIntensity = NULL;
static rdVector4* rdQClip_pDestVertIntensity   = NULL;
static rdVector4 rdQClip_aWorkVertIntensities[RDQCLIP_MAXWORKVERTS];

void rdQClip_InstallHooks(void)
{

    J3D_HOOKFUNC(rdQClip_VerticesInFrustrum);
    J3D_HOOKFUNC(rdQClip_Face3W);
}

void rdQClip_ResetGlobals(void)
{}

size_t J3DAPI rdQClip_VerticesInFrustrum(const rdClipFrustum* pFrustrum, const rdVector3* aVertices, size_t numVertices)
{
    const rdVector3* pCurVert;

    if ( aVertices->y >= (double)pFrustrum->nearPlane )
    {
        if ( pFrustrum->bClipFar && aVertices->y > (double)pFrustrum->farPlane )
        {
            for ( pCurVert = &aVertices[numVertices - 1]; pCurVert > aVertices && pCurVert->y > (double)pFrustrum->farPlane; --pCurVert )
            {
                ;
            }

            if ( pCurVert == aVertices )
            {
                return 0;
            }
        }
    }
    else
    {
        for ( pCurVert = &aVertices[numVertices - 1]; pCurVert > aVertices && pCurVert->y < (double)pFrustrum->nearPlane; --pCurVert )
        {
            ;
        }

        if ( pCurVert == aVertices )
        {
            return 0;
        }
    }

    if ( aVertices->y * pFrustrum->leftPlane <= aVertices->x )
    {
        if ( aVertices->y * pFrustrum->rightPlane < aVertices->x )
        {
            for ( pCurVert = &aVertices[numVertices - 1]; pCurVert > aVertices && pCurVert->y * pFrustrum->rightPlane < pCurVert->x; --pCurVert )
            {
                ;
            }

            if ( pCurVert == aVertices )
            {
                return 0;
            }
        }
    }
    else
    {
        for ( pCurVert = &aVertices[numVertices - 1]; pCurVert > aVertices && pCurVert->y * pFrustrum->leftPlane > pCurVert->x; --pCurVert )
        {
            ;
        }

        if ( pCurVert == aVertices )
        {
            return 0;
        }
    }

    if ( aVertices->y * pFrustrum->topPlane >= aVertices->z )
    {
        if ( aVertices->y * pFrustrum->bottomPlane > aVertices->z )
        {
            for ( pCurVert = &aVertices[numVertices - 1]; pCurVert > aVertices && pCurVert->y * pFrustrum->bottomPlane > pCurVert->z; --pCurVert )
            {
                ;
            }

            if ( pCurVert == aVertices )
            {
                return 0;
            }
        }
    }
    else
    {
        for ( pCurVert = &aVertices[numVertices - 1]; pCurVert > aVertices && pCurVert->y * pFrustrum->topPlane < pCurVert->z; --pCurVert )
        {
            ;
        }

        if ( pCurVert == aVertices )
        {
            return 0;
        }
    }

    return numVertices;
}

int J3DAPI rdQClip_Face3T(const rdClipFrustum* pFrustum, rdVector3* aVerts, rdVector2* aTexVerts, rdVector4* aIntensities, size_t numVertices)
{
    RD_ASSERT(aVerts != rdQClip_aWorkVerts);
    RD_ASSERT(aTexVerts != rdQClip_aWorkTVerts);
    RD_ASSERT(aIntensities != rdQClip_aWorkVertIntensities);

    rdClip_g_faceStatus = 0;

    rdQClip_pSourceVert = aVerts;
    rdQClip_pDestVert   = rdQClip_aWorkVerts;

    rdQClip_pSourceTVert = aTexVerts;
    rdQClip_pDestTVert   = rdQClip_aWorkTVerts;

    rdQClip_pSourceVertIntensity = aIntensities;
    rdQClip_pDestVertIntensity   = rdQClip_aWorkVertIntensities;

    size_t clipindex = 0;

    rdVector3* pPrevVert    = &aVerts[numVertices - 1];
    rdVector2* pPrevTVert   = &aTexVerts[numVertices - 1];
    rdVector4* pPrevVertInt = &aIntensities[numVertices - 1];

    rdVector3* pCurVert    = aVerts;
    rdVector2* pCurTVert   = aTexVerts;
    rdVector4* pCurVertInt = aIntensities;

    for ( size_t i = 0; i < numVertices; ++i )
    {
        if ( pPrevVert->y >= (double)pFrustum->nearPlane || pCurVert->y >= (double)pFrustum->nearPlane )
        {
            if ( pPrevVert->y != pFrustum->nearPlane
                && pCurVert->y != pFrustum->nearPlane
                && (pPrevVert->y < (double)pFrustum->nearPlane || pCurVert->y < (double)pFrustum->nearPlane) )
            {
                float clipFactor = (pFrustum->nearPlane - pPrevVert->y) / (pCurVert->y - pPrevVert->y);
                rdQClip_pDestVert[clipindex].y = pFrustum->nearPlane;
                rdQClip_pDestVert[clipindex].z = (pCurVert->z - pPrevVert->z) * clipFactor + pPrevVert->z;
                rdQClip_pDestVert[clipindex].x = (pCurVert->x - pPrevVert->x) * clipFactor + pPrevVert->x;

                rdQClip_pDestTVert[clipindex].x = (pCurTVert->x - pPrevTVert->x) * clipFactor + pPrevTVert->x;
                rdQClip_pDestTVert[clipindex].y = (pCurTVert->y - pPrevTVert->y) * clipFactor + pPrevTVert->y;

                rdQClip_pDestVertIntensity[clipindex].x = (pCurVertInt->x - pPrevVertInt->x) * clipFactor + pPrevVertInt->x;
                rdQClip_pDestVertIntensity[clipindex].y = (pCurVertInt->y - pPrevVertInt->y) * clipFactor + pPrevVertInt->y;
                rdQClip_pDestVertIntensity[clipindex].z = (pCurVertInt->z - pPrevVertInt->z) * clipFactor + pPrevVertInt->z;
                rdQClip_pDestVertIntensity[clipindex].w = (pCurVertInt->w - pPrevVertInt->w) * clipFactor + pPrevVertInt->w;

                clipindex++;
                rdClip_g_faceStatus |= 1;
            }

            if ( pCurVert->y >= (double)pFrustum->nearPlane )
            {
                rdQClip_pDestVert[clipindex]          = *pCurVert;
                rdQClip_pDestTVert[clipindex]         = *pCurTVert;
                rdQClip_pDestVertIntensity[clipindex] = *pCurVertInt;
                clipindex++;
            }
        }

        RD_ASSERTREL(clipindex < 80);

        pPrevVert    = pCurVert++;
        pPrevTVert   = pCurTVert++;
        pPrevVertInt = pCurVertInt++;
    }

    if ( clipindex < 3 )
    {
        rdClip_g_faceStatus |= 0x40u;
        return clipindex;
    }

    if ( rdQClip_pDestVert == aVerts )
    {
        return clipindex;
    }

    rdVector_Copy3List(aVerts, rdQClip_pDestVert, clipindex);
    rdVector_Copy2List(aTexVerts, rdQClip_pDestTVert, clipindex);
    rdVector_Copy4List(aIntensities, rdQClip_pDestVertIntensity, clipindex);

    return clipindex;
}

int J3DAPI rdQClip_Face3W(const rdClipFrustum* pFrustrum, rdVector3* aVertices, size_t numVertices)
{
    rdClip_g_faceStatus = 0;
    rdQClip_pSourceVert = aVertices;
    rdQClip_pDestVert   = rdQClip_aWorkVerts;

    size_t clipindex     = 0;
    rdVector3* pPrevVert = &aVertices[numVertices - 1];
    rdVector3* pCurVert  = aVertices;

    for ( size_t i = 0; i < numVertices; ++i )
    {
        if ( pPrevVert->y >= (double)pFrustrum->nearPlane || pCurVert->y >= (double)pFrustrum->nearPlane )
        {
            if ( pPrevVert->y != pFrustrum->nearPlane
                && pCurVert->y != pFrustrum->nearPlane
                && (pPrevVert->y < (double)pFrustrum->nearPlane || pCurVert->y < (double)pFrustrum->nearPlane) )
            {
                float clipFactor = (pFrustrum->nearPlane - pPrevVert->y) / (pCurVert->y - pPrevVert->y);
                rdQClip_pDestVert[clipindex].y = pFrustrum->nearPlane;
                rdQClip_pDestVert[clipindex].z = (pCurVert->z - pPrevVert->z) * clipFactor + pPrevVert->z;
                rdQClip_pDestVert[clipindex].x = (pCurVert->x - pPrevVert->x) * clipFactor + pPrevVert->x;

                clipindex++;
                rdClip_g_faceStatus |= 0x01;
            }

            if ( pCurVert->y >= (double)pFrustrum->nearPlane )
            {
                rdQClip_pDestVert[clipindex] = *pCurVert;
                clipindex++;
            }
        }

        RD_ASSERTREL(clipindex < STD_ARRAYLEN(rdQClip_aWorkVerts));
        pPrevVert = pCurVert++;
    }

    if ( clipindex < 3 )
    {
        rdClip_g_faceStatus |= 0x40u;
        return clipindex;
    }

    if ( rdQClip_pDestVert != aVertices )
    {
        rdVector_Copy3List(aVertices, rdQClip_pDestVert, clipindex);
    }

    return clipindex;
}

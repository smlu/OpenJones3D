#include "rdCache.h"
#include <j3dcore/j3dhook.h>

#include <rdroid/Main/rdroid.h>
#include <rdroid/RTI/symbols.h>

#include <std/General/stdEffect.h>
#include <std/General/stdMath.h>
#include <std/General/stdUtil.h>
#include <std/Win95/std3D.h>

#include <stdlib.h>

#define RDCACHE_VERTBUFFERSIZE RDCACHE_MAXVERTICES * RDCACHE_MAXFACEVERTICES

static size_t rdCache_numProcFaces              = 0;
static rdCacheProcEntry rdCache_aProcFaces[128] = { 0 };

static size_t rdCache_numUsedVertices = 0;
static D3DTLVERTEX rdCache_aVertices[RDCACHE_VERTBUFFERSIZE]      = { 0 };
static rdVector4 rdCache_aVertIntensities[RDCACHE_VERTBUFFERSIZE] = { 0 }; // Fixed: Changed to match size of rdCache_aVertices, was 8192


static size_t rdCache_numAlphaProcFaces               = 0;
static rdCacheProcEntry rdCache_aAlphaProcFaces[2048] = { 0 };

static size_t rdCache_numUsedAlphaVertices = 0;
static D3DTLVERTEX rdCache_aAlphaVertices[RDCACHE_VERTBUFFERSIZE]      = { 0 };
static rdVector4 rdCache_aAlphaVertIntensities[RDCACHE_VERTBUFFERSIZE] = { 0 };

static size_t rdCache_totalVerts             = 0;
static LPD3DTLVERTEX rdCache_pCurInVert      = NULL;
static LPD3DTLVERTEX rdCache_pCurCacheVertex = NULL;

static D3DTLVERTEX rdCache_aHWVertices[RDCACHE_VERTBUFFERSIZE]     = { 0 };
static uint16_t rdCache_aVertIndices[RDCACHE_VERTBUFFERSIZE * 3]   = { 0 };

static size_t rdCache_drawnFaces = 0;
static size_t rdCache_frameNum   = 0;

int J3DAPI rdCache_ProcFaceDistanceCompare(const rdCacheProcEntry* pEntry1, const rdCacheProcEntry* pEntry2);
int J3DAPI rdCache_ProcFaceCompare(const rdCacheProcEntry* pEntry1, const rdCacheProcEntry* pEntry2);

void rdCache_InstallHooks(void)
{
    J3D_HOOKFUNC(rdCache_Startup);
    J3D_HOOKFUNC(rdCache_AdvanceFrame);
    J3D_HOOKFUNC(rdCache_GetFrameNum);
    J3D_HOOKFUNC(rdCache_GetProcEntry);
    J3D_HOOKFUNC(rdCache_GetAlphaProcEntry);
    J3D_HOOKFUNC(rdCache_Flush);
    J3D_HOOKFUNC(rdCache_FlushAlpha);
    J3D_HOOKFUNC(rdCache_AddProcFace);
    J3D_HOOKFUNC(rdCache_AddAlphaProcFace);
    J3D_HOOKFUNC(rdCache_SendFaceListToHardware);
    J3D_HOOKFUNC(rdCache_SendWireframeFaceListToHardware);
    J3D_HOOKFUNC(rdCache_AddToTextureCache);
    J3D_HOOKFUNC(rdCache_ProcFaceDistanceCompare);
    J3D_HOOKFUNC(rdCache_ProcFaceCompare);
}

void rdCache_ResetGlobals(void)
{

}

void J3DAPI rdCache_Startup()
{
    memset(rdCache_aHWVertices, 0, sizeof(rdCache_aHWVertices)); // Fixed: num bytes to the bytes size of rdCache_aHWVertices. Was STD_ARRAYLEN(rdCache_aHWVertices)
    rdCache_frameNum = 0;
}

void rdCache_AdvanceFrame(void)
{
    rdCache_drawnFaces = 0; // Could be part rdCache_ClearFrameCounters();
    ++rdCache_frameNum;

    // Below code could be part of function rdCache_Reset
    rdCache_numProcFaces         = 0;
    rdCache_numUsedVertices      = 0;
    rdCache_numAlphaProcFaces    = 0;
    rdCache_numUsedAlphaVertices = 0;
}

D3DTLVERTEX* rdCache_GetVertexPointer(int index) {
    if ( index >= 0 && index < RDCACHE_VERTBUFFERSIZE ) {
        return &rdCache_aHWVertices[index];
    }
    return NULL;
}

int J3DAPI rdCache_GetFrameNum()
{
    return rdCache_frameNum;
}

rdCacheProcEntry* rdCache_GetProcEntry(void)
{
    size_t procNum;
    rdCacheProcEntry* pProcEntry;

    procNum = rdCache_numProcFaces;
    if ( rdCache_numProcFaces >= STD_ARRAYLEN(rdCache_aProcFaces) )
    {
        rdCache_Flush();
        procNum = rdCache_numProcFaces;
    }

     // Added: Check if the vertex buffer is large enough to hold all face vertices.
    //        Note, taken from grimengine / OpenJKDF2
    if ( RDCACHE_VERTBUFFERSIZE - rdCache_numUsedVertices < RDCACHE_MAXFACEVERTICES ) {
        return NULL;
    }

    // TODO: add bound check for cached verts
    pProcEntry                   = &rdCache_aProcFaces[procNum];
    pProcEntry->aVertices        = &rdCache_aVertices[rdCache_numUsedVertices];
    pProcEntry->aVertIntensities = &rdCache_aVertIntensities[rdCache_numUsedVertices];
    return pProcEntry;
}

rdCacheProcEntry* rdCache_GetAlphaProcEntry(void)
{
    size_t entryNum;
    rdCacheProcEntry* pProcEntry;

    entryNum = rdCache_numAlphaProcFaces;
    if ( rdCache_numAlphaProcFaces >= STD_ARRAYLEN(rdCache_aAlphaProcFaces) )
    {
        rdCache_Flush();
        rdCache_FlushAlpha();
        entryNum = rdCache_numAlphaProcFaces;
    }

    // Added: Check if the vertex buffer is large enough to hold all face vertices.
    //        Note, taken from grimengine / OpenJKDF2
    if ( RDCACHE_VERTBUFFERSIZE - rdCache_numUsedAlphaVertices < RDCACHE_MAXFACEVERTICES ) {
        return NULL;
    }

    // TODO: add bound check for verts
    pProcEntry                   = &rdCache_aAlphaProcFaces[entryNum];
    pProcEntry->aVertices        = &rdCache_aAlphaVertices[rdCache_numUsedAlphaVertices];
    pProcEntry->aVertIntensities = &rdCache_aAlphaVertIntensities[rdCache_numUsedAlphaVertices];
    return pProcEntry;
}

void rdCache_Flush(void)
{
    if ( rdCache_numProcFaces )
    {
        switch ( rdroid_g_curGeometryMode )
        {
            case RD_GEOMETRY_NONE:
                break;

            case RD_GEOMETRY_VERTEX:
            case RD_GEOMETRY_WIREFRAME:
                rdCache_SendWireframeFaceListToHardware(rdCache_numProcFaces, rdCache_aProcFaces);
                break;

            default:
                rdCache_SendFaceListToHardware(rdCache_numProcFaces, rdCache_aProcFaces, rdCache_ProcFaceCompare);
                break;
        }

        rdCache_drawnFaces      += rdCache_numProcFaces;
        rdCache_numProcFaces    = 0;
        rdCache_numUsedVertices = 0;
    }
}

void rdCache_FlushAlpha(void)
{
    if ( rdCache_numAlphaProcFaces )
    {
        switch ( rdroid_g_curGeometryMode )
        {
            case RD_GEOMETRY_NONE:
                break;

            case RD_GEOMETRY_VERTEX:
            case RD_GEOMETRY_WIREFRAME:
                rdCache_SendWireframeFaceListToHardware(rdCache_numAlphaProcFaces, rdCache_aAlphaProcFaces);
                break;

            default:
                rdCache_SendFaceListToHardware(rdCache_numAlphaProcFaces, rdCache_aAlphaProcFaces, rdCache_ProcFaceDistanceCompare);
                break;
        }

        rdCache_drawnFaces          += rdCache_numAlphaProcFaces;
        rdCache_numAlphaProcFaces    = 0;
        rdCache_numUsedAlphaVertices = 0;
    }
}

void J3DAPI rdCache_AddProcFace(size_t numVerts)
{
    rdCache_aProcFaces[rdCache_numProcFaces].numVertices = numVerts;
    rdCache_numUsedVertices += numVerts;
    ++rdCache_numProcFaces;
}

void J3DAPI rdCache_AddAlphaProcFace(size_t numVertices)
{
    rdCacheProcEntry* pEntry = &rdCache_aAlphaProcFaces[rdCache_numAlphaProcFaces];
    pEntry->numVertices = numVertices;

    float sz = 3.4028235e38f;
    for ( size_t i = 0; i < numVertices; ++i )
    {
        if ( pEntry->aVertices[i].sz < (double)sz ) {
            sz = pEntry->aVertices[i].sz;
        }
    }

    pEntry->distance = sz;
    rdCache_numUsedAlphaVertices += numVertices;
    ++rdCache_numAlphaProcFaces;
}

void J3DAPI rdCache_SendFaceListToHardware(size_t numPolys, rdCacheProcEntry* pCurPoly, rdCacheSortFunc pfSort)
{
    size_t polyNum   = 0;
    int curMatCelNum = -1;
    const tStdFadeFactor* pFadeFactor = stdEffect_GetFadeFactor();

    if ( pfSort == rdCache_ProcFaceDistanceCompare )
    {
        qsort(pCurPoly, numPolys, sizeof(rdCacheProcEntry), pfSort);
    }

LABEL_4:
    if ( polyNum < numPolys )
    {
        rdFaceFlags fflags = pCurPoly->flags;
        Std3DRenderState rdflags = STD3D_RS_SUBPIXEL_CORRECTION | STD3D_RS_UNKNOWN_2 | STD3D_RS_UNKNOWN_1;

        if ( (fflags & RD_FF_TEX_CLAMP_X) != 0 )
        {
            rdflags = STD3D_RS_TEX_CPAMP_U | STD3D_RS_SUBPIXEL_CORRECTION | STD3D_RS_UNKNOWN_2 | STD3D_RS_UNKNOWN_1;
        }

        if ( (fflags & RD_FF_TEX_CLAMP_Y) != 0 )
        {
            rdflags |= STD3D_RS_TEX_CPAMP_V;  // 0x1000 - STD3D_RS_TEX_CPAMP_V
        }

        if ( (fflags & RD_FF_TEX_FILTER_NEAREST) == 0 )
        {
            rdflags |= STD3D_RS_TEXFILTER_BILINEAR;
        }

        if ( (fflags & RD_FF_ZWRITE_DISABLED) != 0 )
        {
            rdflags |=  STD3D_RS_ZWRITE_DISABLED;  // 0x2000 - STD3D_RS_ZWRITE_DISABLED
        }

        if ( (fflags & RD_FF_FOG_ENABLED) != 0 )
        {
            rdflags |= STD3D_RS_FOG_ENABLED;  // 0x8000 - STD3D_RS_FOG_ENABLED
        }

        rdMaterial* pCurMat = NULL;
        if ( rdroid_g_curGeometryMode != RD_GEOMETRY_SOLID )
        {
            pCurMat = pCurPoly->pMaterial;
        }

        LPDIRECT3DTEXTURE2 pD3DCachedTex = NULL;
        if ( pCurMat )
        {
            // format = pCurMat->formatType;
            if ( pCurMat->formatType == STDCOLOR_FORMAT_RGBA_1BIT_ALPHA )
            {
                rdflags |= STD3D_RS_ALPHAREF_SET;
            }

            tSystemTexture* pTex = NULL;
            curMatCelNum = pCurPoly->matCelNum;
            if ( curMatCelNum == -1 )
            {
                if ( pCurPoly->pMaterial->curCelNum < 0 )
                {
                    curMatCelNum = 0;
                }
                else if ( pCurPoly->pMaterial->curCelNum > pCurPoly->pMaterial->numCels - 1 )
                {
                    curMatCelNum = pCurPoly->pMaterial->numCels - 1;
                }
                else
                {
                    curMatCelNum = pCurPoly->pMaterial->curCelNum;
                }

                pTex = &pCurPoly->pMaterial->aTextures[curMatCelNum];
                rdCache_AddToTextureCache(pTex, pCurMat->formatType);
            }
            else
            {
                if ( curMatCelNum < 0 )
                {
                    curMatCelNum = 0;
                }

                else if ( curMatCelNum > pCurPoly->pMaterial->numCels - 1 )
                {
                    curMatCelNum = pCurPoly->pMaterial->numCels - 1;
                }
                else
                {
                    curMatCelNum = pCurPoly->matCelNum;
                }

                pTex = &pCurPoly->pMaterial->aTextures[curMatCelNum];
                rdCache_AddToTextureCache(pTex, pCurMat->formatType);
            }

            pD3DCachedTex = pTex->pD3DCachedTex;
        }

        size_t totalIndices = 0;
        rdCache_totalVerts  = 0;
        while ( 1 )
        {
            uint16_t curVertIdx = rdCache_totalVerts;
            rdLightMode lightingMode = rdroid_g_curLightingMode;
            if ( pCurPoly->lightingMode < rdroid_g_curLightingMode )
            {
                lightingMode = pCurPoly->lightingMode;
            }

            rdCache_pCurInVert = pCurPoly->aVertices;
            rdCache_pCurCacheVertex = &rdCache_aHWVertices[rdCache_totalVerts];
            for ( size_t i = 0; i < pCurPoly->numVertices; ++i )
            {
                memcpy(rdCache_pCurCacheVertex, rdCache_pCurInVert++, sizeof(D3DTLVERTEX));

                float blue  = 1.0f;
                float green = 1.0f;
                float red   = 1.0f;
                float alpha = pCurPoly->aVertIntensities->alpha;
                if ( lightingMode > RD_LIGHTING_NONE && lightingMode <= RD_LIGHTING_GOURAUD )
                {
                    red   = pCurPoly->extraLight.red;
                    green = pCurPoly->extraLight.green;
                    blue  = pCurPoly->extraLight.blue;
                    alpha = pCurPoly->aVertIntensities->alpha;
                }

                if ( lightingMode == RD_LIGHTING_GOURAUD )
                {
                    red   += pCurPoly->aVertIntensities[i].red;
                    green += pCurPoly->aVertIntensities[i].green;
                    blue  += pCurPoly->aVertIntensities[i].blue;
                    if ( pCurPoly->extraLight.alpha >= 1.0f ) {
                        alpha = pCurPoly->aVertIntensities[i].alpha;
                    }
                    else {
                        alpha = pCurPoly->aVertIntensities[i].alpha + pCurPoly->extraLight.alpha;
                    }
                }

                if ( pFadeFactor->bEnabled )
                {
                    red   = pFadeFactor->factor * red;
                    green = pFadeFactor->factor * green;
                    blue  = pFadeFactor->factor * blue;
                }

                red   = STDMATH_CLAMP(red, 0.0f, 1.0f);
                green = STDMATH_CLAMP(green, 0.0f, 1.0f);
                blue  = STDMATH_CLAMP(blue, 0.0f, 1.0f);
                alpha = STDMATH_CLAMP(alpha, 0.0f, 1.0f);

                if ( (fflags & RD_FF_TEX_TRANSLUCENT) != 0 ) {
                    rdCache_pCurCacheVertex->color = D3DRGBA(red, green, blue, alpha); // (int32_t)(blue * 255.0f) | ((unsigned int)(int32_t)(green * 255.0f) << 8) | ((unsigned int)(int32_t)(red * 255.0f) << 16) | ((unsigned int)(int32_t)(alpha * 255.0f) << 24);
                }
                else {
                    rdCache_pCurCacheVertex->color = D3DRGB(red, green, blue); //(int32_t)(blue * 255.0f) | ((unsigned int)(int32_t)(green * 255.0f) << 8) | ((unsigned int)(int32_t)(red * 255.0f) << 16) | 0xFF000000;
                }

                ++rdCache_pCurCacheVertex;
                ++rdCache_totalVerts;
            }

            // Triangulation
            if ( pCurPoly->numVertices <= 3u )
            {
                rdCache_aVertIndices[totalIndices++] = curVertIdx;
                rdCache_aVertIndices[totalIndices++] = curVertIdx + 1;
                rdCache_aVertIndices[totalIndices++] = curVertIdx + 2;
            }
            else
            {
                uint16_t triCount     = pCurPoly->numVertices - 2;
                uint16_t triPoint1Num = 0;
                uint16_t triPoint2Num = 1;
                uint16_t triPoint3Num = pCurPoly->numVertices - 1;

                for ( size_t triNum = 0; triNum < triCount; ++triNum )
                {
                    rdCache_aVertIndices[totalIndices++] = triPoint1Num + curVertIdx;
                    rdCache_aVertIndices[totalIndices++] = triPoint2Num + curVertIdx;
                    rdCache_aVertIndices[totalIndices++] = triPoint3Num + curVertIdx;
                    if ( (triNum & 1) != 0 ) // if odd
                    {
                        triPoint1Num = triPoint3Num;
                        triPoint3Num--;// = triPoint3Num - 1;
                    }
                    else
                    {
                        triPoint1Num = triPoint2Num++;
                    }
                }
            }

            ++pCurPoly;
            ++polyNum;

            if ( 3 * (pCurPoly->numVertices - 2) + totalIndices >= std3D_g_maxVertices // i.e. totalIndices + num required triangle indices for next poly >= std3D_g_maxVertices
                || polyNum >= numPolys
                || pCurMat != pCurPoly->pMaterial
                || curMatCelNum != pCurPoly->matCelNum
                || fflags != pCurPoly->flags )
            {
                RD_ASSERTREL(rdCache_totalVerts < RDCACHE_VERTBUFFERSIZE);
                std3D_DrawRenderList(pD3DCachedTex, rdflags, rdCache_aHWVertices, rdCache_totalVerts, rdCache_aVertIndices, totalIndices);
                goto LABEL_4;
            }
        }
    }
}

void J3DAPI rdCache_SendWireframeFaceListToHardware(size_t numPolys, rdCacheProcEntry* pCurPoly)
{
    std3D_SetWireframeRenderState();

    for ( size_t i = 0; i < numPolys; ++i )
    {
        for ( size_t j = 0; j < pCurPoly->numVertices; ++j )
        {
            pCurPoly->aVertices[j].color = RGBA_MAKE(255, 255, 255, 255);// aka white color
        }

        if ( rdroid_g_curGeometryMode == RD_GEOMETRY_VERTEX )
        {
            std3D_DrawPointList(pCurPoly->aVertices, pCurPoly->numVertices); // Fixed: Use correct vertex buffer for drawing. Was using rdCache_aHWVertices.
        }
        else if ( rdroid_g_curGeometryMode == RD_GEOMETRY_WIREFRAME )
        {
            std3D_DrawLineStrip(pCurPoly->aVertices, pCurPoly->numVertices);

            D3DTLVERTEX aVerts[2]; // Draw end vert twice to complete the line
            memcpy(aVerts, &pCurPoly->aVertices[pCurPoly->numVertices - 1], sizeof(D3DTLVERTEX));
            memcpy(&aVerts[1], pCurPoly->aVertices, sizeof(D3DTLVERTEX));
            std3D_DrawLineStrip(aVerts, 2u);
        }

        ++pCurPoly;
    }
}

void J3DAPI rdCache_AddToTextureCache(tSystemTexture* pTexture, StdColorFormatType format)
{
    if ( pTexture->pD3DCachedTex ) {
        std3D_UpdateFrameCount(pTexture);
    }
    else {
        std3D_AddToTextureCache(pTexture, format);
    }
}

int J3DAPI rdCache_ProcFaceDistanceCompare(const rdCacheProcEntry* pEntry1, const rdCacheProcEntry* pEntry2)
{
    if ( pEntry2->distance <= (double)pEntry1->distance )
    {
        return -1;
    }
    return 1;
}

int J3DAPI rdCache_ProcFaceCompare(const rdCacheProcEntry* pEntry1, const rdCacheProcEntry* pEntry2)
{
    rdMaterial* pMat1 = pEntry1->pMaterial;
    rdMaterial* pMat2 = pEntry2->pMaterial;
    if ( pMat1 == pMat2 ) {
        return pEntry2->matCelNum - pEntry1->matCelNum;
    }

    return pMat2 - pMat1;
}
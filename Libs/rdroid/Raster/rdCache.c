#include "rdCache.h"
#include <j3dcore/j3dhook.h>

#include <rdroid/Main/rdroid.h>

#include <std/General/stdEffect.h>
#include <std/General/stdMath.h>
#include <std/General/stdUtil.h>
#include <std/Win95/std3D.h>

#include <stdlib.h>

#include "rdroid/Engine/rdCamera.h"
#include "rdroid/Math/rdMatrix.h"
#include "sith/Engine/sithCamera.h"
#include "std/General/stdMemory.h"
#include "std/Win95/stdShader.h"

#define RDCACHE_VERTBUFFERSIZE RDCACHE_MAXVERTICES * RDCACHE_MAXFACEVERTICES

static size_t rdCache_numProcFaces              = 0;
static rdCacheProcEntry rdCache_aProcFaces[128] = { 0 };

static size_t rdCache_numUsedVertices                             = 0;
static D3DTLVERTEX rdCache_aVertices[RDCACHE_VERTBUFFERSIZE]      = { 0 };
static rdVector4 rdCache_aVertIntensities[RDCACHE_VERTBUFFERSIZE] = { 0 }; // Fixed: Changed to match size of rdCache_aVertices, was 8192


static size_t rdCache_numAlphaProcFaces               = 0;
static rdCacheProcEntry rdCache_aAlphaProcFaces[4096] = { 0 };

static size_t rdCache_numUsedAlphaVertices                             = 0;
static D3DTLVERTEX rdCache_aAlphaVertices[RDCACHE_VERTBUFFERSIZE]      = { 0 };
static rdVector4 rdCache_aAlphaVertIntensities[RDCACHE_VERTBUFFERSIZE] = { 0 };

static size_t rdCache_totalVerts             = 0;
static LPD3DTLVERTEX rdCache_pCurInVert      = NULL;
static LPD3DTLVERTEX rdCache_pCurCacheVertex = NULL;

static D3DTLVERTEX rdCache_aHWVertices[RDCACHE_VERTBUFFERSIZE]   = { 0 };
static uint16_t rdCache_aVertIndices[RDCACHE_VERTBUFFERSIZE * 3] = { 0 };

static size_t rdCache_drawnFaces = 0;
static size_t rdCache_frameNum   = 0;

int J3DAPI rdCache_ProcFaceDistanceCompare(const rdCacheProcEntry* pEntry1, const rdCacheProcEntry* pEntry2);
int J3DAPI rdCache_ProcFaceCompare(const rdCacheProcEntry* pEntry1, const rdCacheProcEntry* pEntry2);

#ifdef J3D_OPENGL
void J3DAPI rdCache_AddLegacyDrawCall(tSysTexture* pTex, Std3DRenderState rdflags, LPD3DTLVERTEX aVerts, size_t numVerts, LPWORD aIndices, size_t numIndices, bool bAlpha);
#endif

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

void rdCache_Startup(void)
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

size_t rdCache_GetFrameNum(void)
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
    if ( RDCACHE_VERTBUFFERSIZE - rdCache_numUsedVertices < RDCACHE_MAXFACEVERTICES )
    {
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
    if ( RDCACHE_VERTBUFFERSIZE - rdCache_numUsedAlphaVertices < RDCACHE_MAXFACEVERTICES )
    {
        return NULL;
    }

    // TODO: add bound check for verts
    pProcEntry                   = &rdCache_aAlphaProcFaces[entryNum];
    pProcEntry->aVertices        = &rdCache_aAlphaVertices[rdCache_numUsedAlphaVertices];
    pProcEntry->aVertIntensities = &rdCache_aAlphaVertIntensities[rdCache_numUsedAlphaVertices];
    return pProcEntry;
}

#ifndef J3D_OPENGL
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

        rdCache_drawnFaces += rdCache_numProcFaces;
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

        rdCache_drawnFaces += rdCache_numAlphaProcFaces;
        rdCache_numAlphaProcFaces    = 0;
        rdCache_numUsedAlphaVertices = 0;
    }
}
#else
void rdCache_Flush(void)
{
    if ( rdCache_numProcFaces )
    {
        switch ( rdroid_g_curGeometryMode )
        {
            case RD_GEOMETRY_NONE:
                break;

            default:
                rdCache_SendFaceListToHardware(rdCache_numProcFaces, rdCache_aProcFaces, rdCache_ProcFaceCompare);
                break;
        }

        rdCache_drawnFaces += rdCache_numProcFaces;
        rdCache_numProcFaces    = 0;
        rdCache_numUsedVertices = 0;
    }
    rdCache_FlushOpaqueDrawCalls();
}

void rdCache_FlushAlpha(void)
{
    if ( rdCache_numAlphaProcFaces )
    {
        switch ( rdroid_g_curGeometryMode )
        {
            case RD_GEOMETRY_NONE:
                break;

            default:
                rdCache_SendFaceListToHardware(rdCache_numAlphaProcFaces, rdCache_aAlphaProcFaces, rdCache_ProcFaceDistanceCompare);
                break;
        }

        rdCache_drawnFaces += rdCache_numAlphaProcFaces;
        rdCache_numAlphaProcFaces    = 0;
        rdCache_numUsedAlphaVertices = 0;
    }
    rdCache_FlushTransparentDrawCalls();
}
#endif


#ifdef J3D_OPENGL

void J3DAPI rdCache_AddProcFace(size_t numVerts)
{
    rdCacheProcEntry* pEntry = &rdCache_aProcFaces[rdCache_numProcFaces];

    pEntry->numVertices = numVerts;
    pEntry->flags &= ~RD_FF_BLEND_ENABLED;

    rdCache_numUsedVertices += numVerts;
    ++rdCache_numProcFaces;
}

void J3DAPI rdCache_AddAlphaProcFace(size_t numVertices)
{
    rdCacheProcEntry* pEntry = &rdCache_aAlphaProcFaces[rdCache_numAlphaProcFaces];
    pEntry->numVertices      = numVertices;
    //pEntry->distance         = rdCache_CalculatePolyDistance(pEntry);
    pEntry->flags |= RD_FF_BLEND_ENABLED;
    rdCache_numUsedAlphaVertices += numVertices;
    ++rdCache_numAlphaProcFaces;
}

#else


void J3DAPI rdCache_AddProcFace(size_t numVerts)
{
    rdCache_aProcFaces[rdCache_numProcFaces].numVertices = numVerts;
    rdCache_numUsedVertices += numVerts;
    ++rdCache_numProcFaces;
}

void J3DAPI rdCache_AddAlphaProcFace(size_t numVertices)
{
    rdCacheProcEntry* pEntry = &rdCache_aAlphaProcFaces[rdCache_numAlphaProcFaces];
    pEntry->numVertices      = numVertices;

    float sz = 3.4028235e38f;
    for ( size_t i = 0; i < numVertices; ++i )
    {
        if ( pEntry->aVertices[i].sz < (double)sz )
        {
            sz = pEntry->aVertices[i].sz;
        }
    }

    pEntry->distance = sz;
    rdCache_numUsedAlphaVertices += numVertices;
    ++rdCache_numAlphaProcFaces;
}


#endif


void J3DAPI rdCache_SendFaceListToHardware(size_t numPolys, rdCacheProcEntry* pCurPoly, rdCacheSortFunc pfSort)
{
    size_t polyNum                    = 0;
    int curMatCelNum                  = -1;
    const tStdFadeFactor* pFadeFactor = stdEffect_GetFadeFactor();

#ifndef J3D_OPENGL
    // sort polys by texture, gives a small fps boost for OpenGL since draw calls with same textures can be batched
    qsort(pCurPoly, numPolys, sizeof(rdCacheProcEntry), rdCache_ProcFaceCompare);
#endif

    if ( pfSort == rdCache_ProcFaceDistanceCompare )
    {
        qsort(pCurPoly, numPolys, sizeof(rdCacheProcEntry), pfSort);
    }

LABEL_4:
    if ( polyNum < numPolys )
    {
        rdFaceFlags fflags       = pCurPoly->flags;
        Std3DRenderState rdflags = STD3D_RS_SUBPIXEL_CORRECTION | STD3D_RS_UNKNOWN_2 | STD3D_RS_UNKNOWN_1;

        if ( (fflags & RD_FF_TEX_CLAMP_X) != 0 )
        {
            rdflags |= STD3D_RS_TEX_CPAMP_U;
        }

        if ( (fflags & RD_FF_TEX_CLAMP_Y) != 0 )
        {
            rdflags |= STD3D_RS_TEX_CPAMP_V;
        }

        if ( (fflags & RD_FF_TEX_FILTER_NEAREST) == 0 )
        {
            rdflags |= J3D_QOL_VALUE(STD3D_RS_TEXFILTER_ANISOTROPIC, STD3D_RS_TEXFILTER_BILINEAR); // Altered: Use STD3D_RS_TEXFILTER_ANISOTROPIC.
        }

        if ( (fflags & RD_FF_ZWRITE_DISABLED) != 0 )
        {
            rdflags |= STD3D_RS_ZWRITE_DISABLED;
        }

        if ( (fflags & RD_FF_FOG_ENABLED) != 0 )
        {
            rdflags |= STD3D_RS_FOG_ENABLED;
        }

        if ( (fflags & RD_FF_BLEND_ENABLED) != 0 )
        {
            rdflags |= STD3D_BLEND_ENABLED;
        }

        rdMaterial* pCurMat = NULL;
        if ( rdroid_g_curGeometryMode != RD_GEOMETRY_SOLID )
        {
            pCurMat = pCurPoly->pMaterial;
        }

        tSysTexture* pCachedTexture = NULL;
        if ( pCurMat )
        {
            // format = pCurMat->formatType;
            if ( pCurMat->formatType == STDCOLOR_FORMAT_RGBA_1BITALPHA )
            {
                rdflags |= STD3D_RS_ALPHAREF_SET;
            }

            tSystemTexture* pTex = NULL;
            curMatCelNum         = pCurPoly->matCelNum;
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

            pCachedTexture = pTex->pCachedTexture;
        }

        size_t totalIndices = 0;
        rdCache_totalVerts  = 0;
        while ( 1 )
        {
            uint16_t curVertIdx      = rdCache_totalVerts;
            rdLightMode lightingMode = rdroid_g_curLightingMode;
            if ( pCurPoly->lightingMode < rdroid_g_curLightingMode )
            {
                lightingMode = pCurPoly->lightingMode;
            }

            rdCache_pCurInVert      = pCurPoly->aVertices;
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
                    red += pCurPoly->aVertIntensities[i].red;
                    green += pCurPoly->aVertIntensities[i].green;
                    blue += pCurPoly->aVertIntensities[i].blue;
                    if ( pCurPoly->extraLight.alpha >= 1.0f )
                    {
                        alpha = pCurPoly->aVertIntensities[i].alpha;
                    }
                    else
                    {
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

                if ( (fflags & RD_FF_TEX_TRANSLUCENT) != 0 )
                {
                    rdCache_pCurCacheVertex->color = D3DRGBA(red, green, blue, alpha);
                    // (int32_t)(blue * 255.0f) | ((unsigned int)(int32_t)(green * 255.0f) << 8) | ((unsigned int)(int32_t)(red * 255.0f) << 16) | ((unsigned int)(int32_t)(alpha * 255.0f) << 24);
                }
                else
                {
                    rdCache_pCurCacheVertex->color = D3DRGB(red, green, blue);
                    //(int32_t)(blue * 255.0f) | ((unsigned int)(int32_t)(green * 255.0f) << 8) | ((unsigned int)(int32_t)(red * 255.0f) << 16) | 0xFF000000;
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
                        triPoint3Num--; // = triPoint3Num - 1;
                    }
                    else
                    {
                        triPoint1Num = triPoint2Num++;
                    }
                }
            }

#ifdef J3D_OPENGL

            ++pCurPoly;
            ++polyNum;

            if ( 3 * (pCurPoly->numVertices - 2) + totalIndices >= std3D_g_maxVertices // i.e. totalIndices + num required triangle indices for next poly >= std3D_g_maxVertices
                || polyNum >= numPolys
                || pCurMat != pCurPoly->pMaterial
                || curMatCelNum != pCurPoly->matCelNum
                || fflags != pCurPoly->flags )
            {
                RD_ASSERTREL(rdCache_totalVerts < RDCACHE_VERTBUFFERSIZE);
                //std3D_DrawRenderList(pCachedTexture, rdflags, rdCache_aHWVertices, rdCache_totalVerts, rdCache_aVertIndices, totalIndices, vertexSpace, useShaderLighting);
                rdCache_AddLegacyDrawCall(pCachedTexture, rdflags, rdCache_aHWVertices, rdCache_totalVerts, rdCache_aVertIndices, totalIndices, pfSort == rdCache_ProcFaceDistanceCompare);
                goto LABEL_4;
            }
#else
            ++pCurPoly;
            ++polyNum;

            if ( 3 * (pCurPoly->numVertices - 2) + totalIndices >= std3D_g_maxVertices // i.e. totalIndices + num required triangle indices for next poly >= std3D_g_maxVertices
                || polyNum >= numPolys
                || pCurMat != pCurPoly->pMaterial
                || curMatCelNum != pCurPoly->matCelNum
                || fflags != pCurPoly->flags )
            {
                RD_ASSERTREL(rdCache_totalVerts < RDCACHE_VERTBUFFERSIZE);
                std3D_DrawRenderList(pCachedTexture, rdflags, rdCache_aHWVertices, rdCache_totalVerts, rdCache_aVertIndices, totalIndices);
                goto LABEL_4;
            }
#endif
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
            pCurPoly->aVertices[j].color = RGBA_MAKE(255, 255, 255, 255); // aka white color
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
    if ( pTexture->pCachedTexture )
    {
        std3D_UpdateFrameCount(pTexture);
    }
    else
    {
        std3D_AddToTextureCache(pTexture, format);
    }
}

int J3DAPI rdCache_ProcFaceDistanceCompare(const rdCacheProcEntry* pEntry1, const rdCacheProcEntry* pEntry2)
{
    if ( (double)pEntry2->distance <= (double)pEntry1->distance )
    {
        return -1;
    }
    return 1;
}

int J3DAPI rdCache_ProcFaceCompare(const rdCacheProcEntry* pEntry1, const rdCacheProcEntry* pEntry2)
{
    rdMaterial* pMat1 = pEntry1->pMaterial;
    rdMaterial* pMat2 = pEntry2->pMaterial;
    if ( pMat1 == pMat2 )
    {
        return pEntry2->matCelNum - pEntry1->matCelNum;
    }

    return pMat2 - pMat1;
}

#ifdef J3D_OPENGL

#define RD_CACHE_MAX_OPAQUE_DRAW_CALLS 10000
#define RD_CACHE_MAX_TRANSPARENT_DRAW_CALLS 5000

#define RD_CACHE_MAX_INSTANCES 3000

typedef struct sFaceDrawInfo
{
    size_t indexOffset;
    size_t numVertices;
} FaceDrawInfo;

typedef struct sDrawCallSortBucket
{
    uint64_t key;
    size_t index;
} DrawCallSortBucket;

static FaceDrawInfo* rdCache_FaceDrawInfos = NULL;
static size_t rdCache_NumFaces             = 0;

static rdPayload rdCache_OpaqueDrawCalls[RD_CACHE_MAX_OPAQUE_DRAW_CALLS];
static DrawCallSortBucket rdCache_opaqueSortBuckets[RD_CACHE_MAX_OPAQUE_DRAW_CALLS];
static size_t rdCache_NumOpaqueDrawCalls = 0;

static rdPayload rdCache_transparentDrawCalls[RD_CACHE_MAX_TRANSPARENT_DRAW_CALLS];
static DrawCallSortBucket rdCache_transparentSortBuckets[RD_CACHE_MAX_TRANSPARENT_DRAW_CALLS];
static size_t rdCache_NumTransparentDrawCalls = 0;

static InstanceData rdCache_instanceData[RD_CACHE_MAX_INSTANCES];
static size_t rdCache_numInstances = 0;

static rdDrawType rdCache_currentDrawType;

static GeometryBatch rdCache_geometryBatch = { 0 };
static ModelBatch rdCache_modelBatch       = { 0 };
static QuadBatch rdCache_quadBatch         = { 0 };
static LegacyBatch rdCache_legacyBatch     = { 0 };

static void rdCache_SendDrawCallsToHardware(rdPayload* drawCalls, DrawCallSortBucket* pSortBuckets, size_t numDrawCalls);
static size_t rdCache_BatchGeometryDrawCalls(size_t start, rdPayload* drawCalls, DrawCallSortBucket* pSortBuckets, size_t numDrawCalls);
static size_t rdCache_BatchModelDrawCalls(size_t start, rdPayload* drawCalls, DrawCallSortBucket* pSortBuckets, size_t numDrawCalls);
static size_t rdCache_BatchQuadDrawCalls(size_t start, rdPayload* drawCalls, DrawCallSortBucket* pSortBuckets, size_t numDrawCalls);
static size_t rdCache_BatchLegacyDrawCalls(size_t start, rdPayload* drawCalls, DrawCallSortBucket* pSortBuckets, size_t numDrawCalls);

int rdCache_DrawCallOpaqueCompare(const DrawCallSortBucket* a, DrawCallSortBucket* b)
{
    return (a->key > b->key) - (a->key < b->key);
}

static int rdCache_DrawCallDistanceCompare(const DrawCallSortBucket* pEntry1, const DrawCallSortBucket* pEntry2)
{
    rdPayload* a = &rdCache_transparentDrawCalls[pEntry1->index];
    rdPayload* b = &rdCache_transparentDrawCalls[pEntry2->index];
    if ( a->distance > b->distance ) return -1;
    if ( a->distance < b->distance ) return 1;
    return 0;
}

static uint8_t rdQuantizeFloat8(float v)
{
    return (uint8_t)(DWORD)(v * 255.0f) & 0xFF;
}

static uint16_t rdQuantizeFloat16(float v)
{
    if ( v <= 0.0f ) return 0;
    if ( v >= 10.0f ) return 0xFFFF; // 65535
    return (uint16_t)(v / 10.0f * 65535.0f + 0.5f);
}

static uint8_t Std3D_ExtractBatchState(Std3DRenderState rs)
{
    uint8_t state = 0;

    if ( rs & STD3D_RS_TEXFILTER_BILINEAR )
        state |= 1 << 0;

    if ( rs & STD3D_RS_TEXFILTER_ANISOTROPIC )
        state |= 1 << 1;

    if ( rs & STD3D_RS_TEX_CPAMP_U )
        state |= 1 << 2;

    if ( rs & STD3D_RS_TEX_CPAMP_V )
        state |= 1 << 3;

    if ( rs & STD3D_RS_ZWRITE_DISABLED )
        state |= 1 << 4;

    if ( rs & STD3D_RS_FOG_ENABLED )
        state |= 1 << 5;

    if ( rs & STD3D_CULL_DISABLED )
        state |= 1 << 6;

    return state;
}

static uint64_t rdCache_GenerateOpaqueGeoSortKey(rdPayload* draw)
{
    uint64_t key = 0;

    key |= ((uint64_t)(draw->type & 0x7)) << 61;

    uint8_t shaderId = (uint8_t)((draw->pShader ? draw->pShader->handle : 0) & 0xFF);
    key |= ((uint64_t)shaderId) << 53;
    uint16_t texId = (uint16_t)((draw->pTex ? draw->pTex->id : 0) & 0xFFFF);
    key |= ((uint64_t)texId) << 37;

    uint32_t rgb24 =
        D3DRGB(
            draw->extraLight.red,
            draw->extraLight.green,
            draw->extraLight.blue
        ) & 0x00FFFFFF;

    key |= ((uint64_t)rgb24) << 13;


    uint8_t batchFlags = Std3D_ExtractBatchState(draw->rdFlags);
    key |= ((uint64_t)batchFlags) << 5;


    return key;
}

static uint64_t rdCache_GenerateLegacySortKey(rdPayload* draw)
{
    rdLegacyPayload* pPayload = &draw->legacyPayload;

    uint64_t key = 0;

    key |= ((uint64_t)(draw->type & 0x7)) << 61;

    uint16_t texId = (uint16_t)((draw->pTex ? draw->pTex->id : 0) & 0xFFFF);
    key |= ((uint64_t)texId) << 45;

    uint32_t type = pPayload->type;
    key |= ((uint64_t)type) << 13;

    uint8_t batchFlags = Std3D_ExtractBatchState(draw->rdFlags);
    key |= ((uint64_t)batchFlags) << 5;

    return key;
}


static uint64_t rdCache_GenerateOpaqueModelSortKey(rdPayload* draw)
{
    rdModelFacePayload* pModelData = &draw->modelFacePayload;
    uint64_t key                   = 0;

    key |= ((uint64_t)draw->type & 0xFFu) << 56;

    key |= ((uint64_t)pModelData & 0x00FFFFFFFFFFFFFFull);

    return key;
}

static uint64_t rdCache_GenerateSpriteSortKey(rdPayload* draw)
{
    uint64_t key             = 0;
    rdSpritePayload* payload = &draw->spritePayload;


    key |= ((uint64_t)payload->spriteType & 0xFFu) << 56;


    uint32_t texId = draw->pTex ? draw->pTex->id : 0;
    key |= (uint64_t)texId;

    return key;
}

static uint64_t rdCache_GenerateParticleSortKey(rdPayload* draw)
{
    uint64_t key               = 0;
    rdParticlePayload* payload = &draw->particlePayload;


    key |= ((uint64_t)draw->type & 0xFF) << 56;


    uint16_t texId = (uint16_t)((draw->pTex ? draw->pTex->id : 0) & 0xFFFF);
    key |= ((uint64_t)texId) << 40;


    float halfSize     = payload->particleHalfSize;   // Muss als float im DrawHeader existieren
    uint16_t qHalfSize = rdQuantizeFloat16(halfSize); // Funktion quantisiert Float auf 16 Bit
    key |= ((uint64_t)qHalfSize) << 24;


    uint8_t r = rdQuantizeFloat8(draw->extraLight.red);
    uint8_t g = rdQuantizeFloat8(draw->extraLight.green);
    uint8_t b = rdQuantizeFloat8(draw->extraLight.blue);
    key |= ((uint64_t)r << 16);
    key |= ((uint64_t)g << 8);
    key |= (uint64_t)b;

    return key;
}

static uint64_t rdCache_GeneratePolyLineSortKey(rdPayload* draw)
{
    uint64_t key = 0;

    key |= ((uint64_t)(draw->type & 0x7)) << 61;

    uint16_t texId = (uint16_t)((draw->pTex ? draw->pTex->id : 0) & 0xFFFF);
    key |= ((uint64_t)texId) << 45;

    return key;
}


void rdCache_InitFaceDrawInfo(const size_t numFaces)
{
    rdCache_FaceDrawInfos = STDMALLOC(numFaces * sizeof(FaceDrawInfo));
    std3D_InitInstanceVBO(RD_CACHE_MAX_INSTANCES);
}

void rdCache_FreeFaceDrawInfos(void)
{
    if ( !rdCache_FaceDrawInfos )
        return;

    STDFREE(rdCache_FaceDrawInfos);
    rdCache_FaceDrawInfos           = NULL;
    rdCache_NumFaces                = 0;
    rdCache_NumOpaqueDrawCalls      = 0;
    rdCache_NumTransparentDrawCalls = 0;
}

size_t rdCache_AddFaceInfoEntry(const size_t indexOffset, const size_t numVertices)
{
    size_t faceNum         = rdCache_NumFaces++;
    FaceDrawInfo* drawInfo = &rdCache_FaceDrawInfos[faceNum];
    drawInfo->indexOffset  = indexOffset;
    drawInfo->numVertices  = numVertices;

    return faceNum;
}

static rdPayload* rdCache_GetDrawCall(rdDrawType type, rdPayload* payloads, size_t numDrawCalls)
{
    rdCache_currentDrawType = type;

    rdPayload* pPayload = &payloads[numDrawCalls];

    pPayload->pShader   = NULL;
    pPayload->pMaterial = NULL;
    return pPayload;
}

rdPayload* rdCache_GetOpaqueDrawCall(rdDrawType type)
{
    if ( rdCache_NumOpaqueDrawCalls >= RD_CACHE_MAX_OPAQUE_DRAW_CALLS )
    {
        rdCache_FlushOpaqueDrawCalls();
    }
    return rdCache_GetDrawCall(type, rdCache_OpaqueDrawCalls, rdCache_NumOpaqueDrawCalls);
}

rdPayload* rdCache_GetTransparentDrawCall(rdDrawType type)
{
    if ( rdCache_NumTransparentDrawCalls >= RD_CACHE_MAX_TRANSPARENT_DRAW_CALLS )
    {
        rdCache_FlushTransparentDrawCalls();
    }
    return rdCache_GetDrawCall(type, rdCache_transparentDrawCalls, rdCache_NumTransparentDrawCalls);
}


static Std3DRenderState rdCache_GetRenderStateOfFace(const rdPayload* pPayload)
{
    rdFaceFlags fFlags       = pPayload->flags;
    Std3DRenderState rdFlags = STD3D_RS_SUBPIXEL_CORRECTION | STD3D_RS_UNKNOWN_2 | STD3D_RS_UNKNOWN_1;

    if ( (fFlags & RD_FF_TEX_CLAMP_X) != 0 )
    {
        rdFlags |= STD3D_RS_TEX_CPAMP_U;
    }

    if ( (fFlags & RD_FF_TEX_CLAMP_Y) != 0 )
    {
        rdFlags |= STD3D_RS_TEX_CPAMP_V;
    }

    if ( (fFlags & RD_FF_TEX_FILTER_NEAREST) == 0 )
    {
        rdFlags |= J3D_QOL_VALUE(STD3D_RS_TEXFILTER_ANISOTROPIC, STD3D_RS_TEXFILTER_BILINEAR); // Altered: Use STD3D_RS_TEXFILTER_ANISOTROPIC.
    }

    if ( (fFlags & RD_FF_ZWRITE_DISABLED) != 0 )
    {
        rdFlags |= STD3D_RS_ZWRITE_DISABLED;
    }

    if ( (fFlags & RD_FF_FOG_ENABLED) != 0 )
    {
        rdFlags |= STD3D_RS_FOG_ENABLED;
    }

    // if ( (fFlags & RD_FF_BLEND_ENABLED) != 0 )
    // {
    //     rdFlags |= STD3D_BLEND_ENABLED;
    // }

    if ( (fFlags & RD_FF_DOUBLE_SIDED) != 0 || (rdroid_g_curRenderOptions & RDROID_BACKFACE_CULLING_ENABLED) == 0 )
    {
        rdFlags |= STD3D_CULL_DISABLED;
    }

    if ( pPayload->pMaterial && pPayload->pMaterial->formatType == STDCOLOR_FORMAT_RGBA_1BITALPHA )
    {
        rdFlags |= STD3D_RS_ALPHAREF_SET;
        rdFlags &= ~STD3D_RS_ZWRITE_DISABLED;
    }

    return rdFlags;
}

static tSysTexture* rdCache_GetFaceTexture(const rdPayload* pPayload)
{
    rdMaterial* pCurMat = NULL;
    if ( rdroid_g_curGeometryMode != RD_GEOMETRY_SOLID )
    {
        pCurMat = pPayload->pMaterial;
    }

    int curMatCelNum            = -1;
    tSysTexture* pCachedTexture = NULL;
    if ( pCurMat )
    {
        tSystemTexture* pTex = NULL;
        curMatCelNum         = pPayload->matCelNum;
        if ( curMatCelNum == -1 )
        {
            if ( pCurMat->curCelNum < 0 )
            {
                curMatCelNum = 0;
            }
            if ( pCurMat->curCelNum > pCurMat->numCels - 1 )
            {
                curMatCelNum = pCurMat->numCels - 1;
            }
            else
            {
                curMatCelNum = pCurMat->curCelNum;
            }

            pTex = &pCurMat->aTextures[curMatCelNum];
            rdCache_AddToTextureCache(pTex, pCurMat->formatType);
        }
        else
        {
            if ( curMatCelNum < 0 )
            {
                curMatCelNum = 0;
            }

            else if ( curMatCelNum > pCurMat->numCels - 1 )
            {
                curMatCelNum = pCurMat->numCels - 1;
            }
            else
            {
                curMatCelNum = pPayload->matCelNum;
            }

            pTex = &pCurMat->aTextures[curMatCelNum];
            rdCache_AddToTextureCache(pTex, pCurMat->formatType);
        }

        pCachedTexture = pTex->pCachedTexture;
    }

    return pCachedTexture;
}

static void rdCache_AddDrawCall(rdPayload* header, DrawCallSortBucket* pSortBucket)
{
    if ( rdCache_currentDrawType == RD_DRAW_LEGACY )
    {
        header->type     = RD_DRAW_LEGACY;
        pSortBucket->key = rdCache_GenerateLegacySortKey(header);
        return;
    }

    header->rdFlags = rdCache_GetRenderStateOfFace(header);
    header->pTex    = rdCache_GetFaceTexture(header);
    if ( rdroid_g_curGeometryMode == RD_GEOMETRY_VERTEX || rdroid_g_curGeometryMode == RD_GEOMETRY_WIREFRAME )
    {
        header->lightingMode = RD_LIGHTING_NONE;
        header->rdFlags &= ~STD3D_RS_FOG_ENABLED;
    }

    switch ( rdCache_currentDrawType )
    {
        case RD_DRAW_GEOMETRY:
            header->type = RD_DRAW_GEOMETRY;
            pSortBucket->key = rdCache_GenerateOpaqueGeoSortKey(header);
            break;
        case RD_DRAW_MODEL:
            header->type = RD_DRAW_MODEL;
            pSortBucket->key = rdCache_GenerateOpaqueModelSortKey(header);
            break;
        case RD_DRAW_SPRITE:
            header->type = RD_DRAW_SPRITE;
            pSortBucket->key = rdCache_GenerateSpriteSortKey(header);
            break;
        case RD_DRAW_PARTICLE:
            header->type = RD_DRAW_PARTICLE;
            pSortBucket->key = rdCache_GenerateParticleSortKey(header);
            break;
        case RD_DRAW_POLYLINE:
        case RD_DRAW_SHADOW:
            header->type = RD_DRAW_POLYLINE;
            pSortBucket->key = rdCache_GeneratePolyLineSortKey(header);
            break;
        default:
            break;
    }
}

void rdCache_AddOpaqueDrawCall(void)
{
    rdPayload* header               = &rdCache_OpaqueDrawCalls[rdCache_NumOpaqueDrawCalls];
    DrawCallSortBucket* pSortBucket = &rdCache_opaqueSortBuckets[rdCache_NumOpaqueDrawCalls];
    rdCache_AddDrawCall(header, pSortBucket);
    pSortBucket->index = rdCache_NumOpaqueDrawCalls++;
}

void rdCache_AddTransparentDrawCall(void)
{
    rdPayload* header               = &rdCache_transparentDrawCalls[rdCache_NumTransparentDrawCalls];
    DrawCallSortBucket* pSortBucket = &rdCache_transparentSortBuckets[rdCache_NumTransparentDrawCalls];
    rdCache_AddDrawCall(header, pSortBucket);
    header->rdFlags |= STD3D_BLEND_ENABLED;
    pSortBucket->index = rdCache_NumTransparentDrawCalls++;
}

static void rdCache_SetInstanceData(rdPayload* drawCalls, DrawCallSortBucket* sortBuckets, size_t numDrawCalls)
{
    for ( size_t i = 0; i < numDrawCalls; i++ )
    {
        DrawCallSortBucket* pSortBucket = &sortBuckets[i];
        rdPayload* header               = &drawCalls[pSortBucket->index];
        if ( header->type != RD_DRAW_MODEL && header->type != RD_DRAW_SPRITE && header->type != RD_DRAW_PARTICLE && header->type != RD_DRAW_POLYLINE )
            continue;

        InstanceData* pData = &rdCache_instanceData[rdCache_numInstances++];

        switch ( header->type )
        {
            case RD_DRAW_MODEL:
                rdModelFacePayload* pModelData = &header->modelFacePayload;
                stdShader_ConvertToMat4(pModelData->modelMatrix, pData->modelMatrix);
                pData->secLightPos[0] = pModelData->lightPosition.x;
                pData->secLightPos[1] = pModelData->lightPosition.z;
                pData->secLightPos[2] = -pModelData->lightPosition.y;
                pData->secLightPos[3] = pModelData->sectorLight.maxRadius;

                pData->secLightColor[0] = pModelData->sectorLight.color.red;
                pData->secLightColor[1] = pModelData->sectorLight.color.green;
                pData->secLightColor[2] = pModelData->sectorLight.color.blue;
                pData->secLightColor[3] = pModelData->sectorLight.minRadius;
                break;
            case RD_DRAW_SPRITE:
                rdSpritePayload* pSpriteData = &header->spritePayload;
                //sprite pos
                stdShader_ConvertToMat4(&pSpriteData->modelMatrix, pData->modelMatrix);
                //sprite offset
                pData->spriteOffset[0] = pSpriteData->spriteOffset.x;
                pData->spriteOffset[1] = pSpriteData->spriteOffset.z;
                pData->spriteOffset[2] = -pSpriteData->spriteOffset.y;
                //halfSize
                pData->spriteHalfSize[0] = pSpriteData->spriteSize.x;
                pData->spriteHalfSize[1] = pSpriteData->spriteSize.y;
                break;
            case RD_DRAW_PARTICLE:
                rdParticlePayload* pParticleData = &header->particlePayload;
                pData->particlePos[0] = pParticleData->particlePos.x;
                pData->particlePos[1] = pParticleData->particlePos.z;
                pData->particlePos[2] = -pParticleData->particlePos.y;
                pData->particlePos[3] = pParticleData->particleHalfSize;

                break;
            case RD_DRAW_POLYLINE:
            case RD_DRAW_SHADOW:
                rdPolyLinePayload* polyLineData = &header->polyLinePayload;
                for ( size_t j = 0; j < 4; ++j )
                {
                    pData->modelMatrix[j * 4 + 0] = polyLineData->vertices[j].x;
                    pData->modelMatrix[j * 4 + 1] = polyLineData->vertices[j].z;
                    pData->modelMatrix[j * 4 + 2] = -polyLineData->vertices[j].y;

                    if ( j < 2 )
                    {
                        pData->polyLineUV01[j * 2 + 0] = polyLineData->texCoords[j].x;
                        pData->polyLineUV01[j * 2 + 1] = polyLineData->texCoords[j].y;
                    }
                    else
                    {
                        pData->polyLineUV23[(j - 2) * 2 + 0] = polyLineData->texCoords[j].x;
                        pData->polyLineUV23[(j - 2) * 2 + 1] = polyLineData->texCoords[j].y;
                    }
                }
                break;
            default:
                continue;
        }
        float red         = header->extraLight.red;
        float green       = header->extraLight.green;
        float blue        = header->extraLight.blue;
        float alpha       = header->extraLight.alpha;
        pData->extraLight = D3DRGBA(red, green, blue, alpha);
    }
}

void rdCache_FlushOpaqueDrawCalls(void)
{
    if ( !rdCache_NumOpaqueDrawCalls )
    {
        return;
    }

    std3D_SetDrawMode(rdroid_g_curGeometryMode);

    //qsort(rdCache_OpaqueDrawCalls, rdCache_NumOpaqueDrawCalls, sizeof(rdPayload), rdCache_DrawCallOpaqueCompare);
    qsort(rdCache_opaqueSortBuckets, rdCache_NumOpaqueDrawCalls, sizeof(DrawCallSortBucket), rdCache_DrawCallOpaqueCompare);

    rdCache_SetInstanceData(rdCache_OpaqueDrawCalls, rdCache_opaqueSortBuckets, rdCache_NumOpaqueDrawCalls);

    std3D_UpdateInstanceVBO(rdCache_instanceData, rdCache_numInstances);

    rdCache_SendDrawCallsToHardware(rdCache_OpaqueDrawCalls, rdCache_opaqueSortBuckets, rdCache_NumOpaqueDrawCalls);

    rdCache_NumOpaqueDrawCalls = 0;
    rdCache_numInstances       = 0;
}

void rdCache_FlushTransparentDrawCalls(void)
{
    if ( !rdCache_NumTransparentDrawCalls )
    {
        return;
    }

    std3D_SetDrawMode(rdroid_g_curGeometryMode);

    //qsort(rdCache_transparentDrawCalls, rdCache_NumTransparentDrawCalls, sizeof(rdPayload), rdCache_DrawCallDistanceCompare);
    qsort(rdCache_transparentSortBuckets, rdCache_NumTransparentDrawCalls, sizeof(DrawCallSortBucket), rdCache_DrawCallDistanceCompare);

    rdCache_SetInstanceData(rdCache_transparentDrawCalls, rdCache_transparentSortBuckets, rdCache_NumTransparentDrawCalls);

    std3D_UpdateInstanceVBO(rdCache_instanceData, rdCache_numInstances);

    rdCache_SendDrawCallsToHardware(rdCache_transparentDrawCalls, rdCache_transparentSortBuckets, rdCache_NumTransparentDrawCalls);

    rdCache_NumTransparentDrawCalls = 0;
    rdCache_numInstances            = 0;
}

static void rdCache_SendDrawCallsToHardware(rdPayload* drawCalls, DrawCallSortBucket* pSortBuckets, size_t numDrawCalls)
{
    for ( size_t i = 0; i < numDrawCalls; i++ )
    {
        DrawCallSortBucket* pSortBucket = &pSortBuckets[i];
        rdPayload* header               = &drawCalls[pSortBucket->index];
        switch ( header->type )
        {
            case RD_DRAW_GEOMETRY:
                i += rdCache_BatchGeometryDrawCalls(i, drawCalls, pSortBuckets, numDrawCalls) - 1;
                std3D_DrawGeometryBatch(&rdCache_geometryBatch);
                break;
            case RD_DRAW_MODEL:
                i += rdCache_BatchModelDrawCalls(i, drawCalls, pSortBuckets, numDrawCalls) - 1;
                std3D_DrawModelBatch(&rdCache_modelBatch);
                break;
            case RD_DRAW_SPRITE:
                i += rdCache_BatchQuadDrawCalls(i, drawCalls, pSortBuckets, numDrawCalls) - 1;
                const rdSpritePayload* pPayload = &header->spritePayload;
                rdCache_quadBatch.spriteType    = pPayload->spriteType;
                rdCache_quadBatch.pShader       = stdShader_GetShader("std_sprite");
                std3D_DrawQuadBatch(&rdCache_quadBatch);
                break;
            case RD_DRAW_PARTICLE:
                i += rdCache_BatchQuadDrawCalls(i, drawCalls, pSortBuckets, numDrawCalls) - 1;
                rdCache_quadBatch.pShader = stdShader_GetShader("std_particle");
                std3D_DrawQuadBatch(&rdCache_quadBatch);
                break;
            case RD_DRAW_POLYLINE:
            case RD_DRAW_SHADOW:
                i += rdCache_BatchQuadDrawCalls(i, drawCalls, pSortBuckets, numDrawCalls) - 1;
                rdCache_quadBatch.pShader = stdShader_GetShader("std_polyline");
                std3D_DrawQuadBatch(&rdCache_quadBatch);
                break;
            case RD_DRAW_LEGACY:
                i += rdCache_BatchLegacyDrawCalls(i, drawCalls, pSortBuckets, numDrawCalls) - 1;
                std3D_CacheLegacyBatch(rdCache_legacyBatch);
        }
    }
}

static size_t rdCache_BatchGeometryDrawCalls(size_t start, rdPayload* drawCalls, DrawCallSortBucket* pSortBuckets, size_t numDrawCalls)
{
    size_t drawCount = 0;
    size_t i         = start;

    DrawCallSortBucket* pSortBucket = &pSortBuckets[i];

    rdPayload* header         = &drawCalls[pSortBucket->index];
    rdGeoFacePayload* payload = &header->geoFacePayload;
    uint64_t sortKey          = pSortBucket->key;

    FaceDrawInfo* drawInfo = &rdCache_FaceDrawInfos[payload->faceNum];

    // Batch state
    rdCache_geometryBatch.pTex          = header->pTex;
    rdCache_geometryBatch.pShader       = header->pShader;
    rdCache_geometryBatch.rdFlags       = header->rdFlags;
    rdCache_geometryBatch.extraLight[0] = header->extraLight.red;
    rdCache_geometryBatch.extraLight[1] = header->extraLight.green;
    rdCache_geometryBatch.extraLight[2] = header->extraLight.blue;
    rdCache_geometryBatch.extraLight[3] = header->extraLight.alpha;
    rdCache_geometryBatch.lightMode     = header->lightingMode;

    // First draw
    rdCache_geometryBatch.indexCounts[0]  = drawInfo->numVertices;
    rdCache_geometryBatch.indexOffsets[0] = drawInfo->indexOffset * sizeof(GLuint);
    drawCount++;
    i++;

    // Batch following
    while ( i < numDrawCalls && drawCount < MAX_BATCHES )
    {
        pSortBucket = &pSortBuckets[i];
        header      = &drawCalls[pSortBucket->index];
        if ( pSortBucket->key != sortKey ||
            header->lightingMode != rdCache_geometryBatch.lightMode || header->rdFlags != rdCache_geometryBatch.rdFlags )
        {
            break;
        }

        payload  = &header->geoFacePayload;
        drawInfo = &rdCache_FaceDrawInfos[payload->faceNum];

        rdCache_geometryBatch.indexCounts[drawCount]  = drawInfo->numVertices;
        rdCache_geometryBatch.indexOffsets[drawCount] = drawInfo->indexOffset * sizeof(GLuint);

        drawCount++;
        i++;
    }

    rdCache_geometryBatch.drawCount = (GLsizei)drawCount;
    return drawCount;
}

static size_t rdCache_BatchModelDrawCalls(size_t start, rdPayload* drawCalls, DrawCallSortBucket* pSortBuckets, size_t numDrawCalls)
{
    size_t drawCount = 0;
    size_t i         = start;

    DrawCallSortBucket* pSortBucket = &pSortBuckets[i];

    rdPayload* header           = &drawCalls[pSortBucket->index];
    rdModelFacePayload* payload = &header->modelFacePayload;
    uint64_t sortKey            = pSortBucket->key;

    // Batch state
    rdCache_modelBatch.pTex        = header->pTex;
    rdCache_modelBatch.pShader     = header->pShader;
    rdCache_modelBatch.rdFlags     = header->rdFlags;
    rdCache_modelBatch.lightMode   = header->lightingMode; // First draw
    rdCache_modelBatch.indexCount  = rdCache_FaceDrawInfos[payload->faceNum].numVertices;
    rdCache_modelBatch.indexOffset = rdCache_FaceDrawInfos[payload->faceNum].indexOffset;
    drawCount++;
    i++;

    // Batch following
    while ( i < numDrawCalls )
    {
        pSortBucket = &pSortBuckets[i];
        header      = &drawCalls[pSortBucket->index];

        if ( pSortBucket->key != sortKey ||
            header->lightingMode != rdCache_modelBatch.lightMode )
        {
            break;
        }

        drawCount++;
        i++;
    }

    rdCache_modelBatch.numberOfInstances = (GLsizei)drawCount;
    return drawCount;
}

static size_t rdCache_BatchQuadDrawCalls(size_t start, rdPayload* drawCalls, DrawCallSortBucket* pSortBuckets, size_t numDrawCalls)
{
    size_t drawCount = 0;
    size_t i         = start;

    DrawCallSortBucket* pSortBucket = &pSortBuckets[i];
    rdPayload* header               = &drawCalls[pSortBucket->index];
    uint64_t sortKey                = pSortBucket->key;

    QuadBatch* batch = &rdCache_quadBatch;

    // Batch state
    batch->pTex      = header->pTex;
    batch->rdFlags   = header->rdFlags;
    batch->lightMode = header->lightingMode;


    drawCount++;
    i++;

    // Batch following
    while ( i < numDrawCalls )
    {
        pSortBucket = &pSortBuckets[i];
        header      = &drawCalls[pSortBucket->index];

        if ( pSortBucket->key != sortKey ||
            header->lightingMode != batch->lightMode )
        {
            break;
        }
        drawCount++;
        i++;
    }

    batch->numberOfInstances = (GLsizei)drawCount;
    return drawCount;
}

static size_t rdCache_BatchLegacyDrawCalls(size_t start, rdPayload* drawCalls, DrawCallSortBucket* pSortBuckets, size_t numDrawCalls)
{
    size_t drawCount = 0;
    size_t i         = start;

    DrawCallSortBucket* pSortBucket = &pSortBuckets[i];

    rdPayload* header        = &drawCalls[pSortBucket->index];
    rdLegacyPayload* payload = &header->legacyPayload;
    uint64_t sortKey         = pSortBucket->key;

    // Batch state
    rdCache_legacyBatch.pTex    = header->pTex;
    rdCache_legacyBatch.rdFlags = header->rdFlags;
    rdCache_legacyBatch.type    = payload->type;

    // First draw
    rdCache_legacyBatch.indexCounts[0]  = payload->numIndices;
    rdCache_legacyBatch.indexOffsets[0] = payload->indexOffset * sizeof(GLushort);
    drawCount++;
    i++;

    // Batch following
    while ( i < numDrawCalls && drawCount < MAX_BATCHES )
    {
        pSortBucket = &pSortBuckets[i];
        if ( pSortBucket->key != sortKey )
        {
            break;
        }

        header  = &drawCalls[pSortBucket->index];
        payload = &header->legacyPayload;

        rdCache_legacyBatch.indexCounts[drawCount]  = payload->numIndices;
        rdCache_legacyBatch.indexOffsets[drawCount] = payload->indexOffset * sizeof(GLushort);

        drawCount++;
        i++;
    }

    rdCache_legacyBatch.drawCount = (GLsizei)drawCount;
    return drawCount;
}

void J3DAPI rdCache_AddLegacyDrawCall(tSysTexture* pTex, Std3DRenderState rdflags, LPD3DTLVERTEX aVerts, size_t numVerts, LPWORD aIndices, size_t numIndices, bool bAlpha)
{
    rdPayload* pDrawCall;
    if ( bAlpha )
    {
        pDrawCall      = rdCache_GetTransparentDrawCall(RD_DRAW_LEGACY);
        float distance = FLT_MAX; // 3.4028235e38f;
        for ( size_t i = 0; i < numVerts; ++i )
        {
            if ( aVerts[i].sz < distance )
            {
                distance = aVerts[i].sz;
            }
        }
        pDrawCall->distance = distance;
    }
    else
    {
        pDrawCall = rdCache_GetOpaqueDrawCall(RD_DRAW_LEGACY);
    }
    pDrawCall->pTex    = pTex;
    pDrawCall->rdFlags = rdflags | STD3D_CULL_DISABLED;

    rdLegacyPayload* pPayload = &pDrawCall->legacyPayload;
    pPayload->type            = GL_TRIANGLES;
    pPayload->numIndices      = numIndices;
    pPayload->indexOffset     = std3D_AddScreenSpaceVertices(aVerts, numVerts, aIndices, numIndices, GL_TRIANGLES);

    if ( bAlpha )
    {
        rdCache_AddTransparentDrawCall();
    }
    else
    {
        rdCache_AddOpaqueDrawCall();
    }
}

void J3DAPI rdCache_AddLineDrawCall(LPD3DTLVERTEX aVerts, size_t numVerts)
{
    rdPayload* pDrawCall = rdCache_GetOpaqueDrawCall(RD_DRAW_LEGACY);
    pDrawCall->pTex      = NULL;
    pDrawCall->rdFlags   = ~(STD3D_RS_FOG_ENABLED | STD3D_RS_UNKNOWN_400 | STD3D_RS_UNKNOWN_200) | STD3D_CULL_DISABLED;

    rdLegacyPayload* pPayload = &pDrawCall->legacyPayload;
    size_t numIndices         = (numVerts - 1) * 2;
    pPayload->numIndices      = numIndices;
    pPayload->indexOffset     = std3D_AddScreenSpaceVertices(aVerts, numVerts, NULL, numIndices, GL_LINES);
    pPayload->type            = GL_LINES;
    rdCache_AddOpaqueDrawCall();
}

#endif

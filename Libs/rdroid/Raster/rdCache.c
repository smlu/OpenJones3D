#include "rdCache.h"
#include <j3dcore/j3dhook.h>

#include <rdroid/Main/rdroid.h>

#include <std/General/stdEffect.h>
#include <std/General/stdMath.h>
#include <std/General/stdUtil.h>
#include <std/Win95/std3D.h>

#include <stdlib.h>

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
#endif

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

    float sz = FLT_MAX; // 3.4028235e38f;
    for ( size_t i = 0; i < numVertices; ++i )
    {
        if ( pEntry->aVertices[i].sz < sz )
        {
            sz = pEntry->aVertices[i].sz;
        }
    }

    pEntry->distance = sz;

    rdCache_numUsedAlphaVertices += numVertices;
    ++rdCache_numAlphaProcFaces;
}

void J3DAPI rdCache_SendFaceListToHardware(size_t numPolys, rdCacheProcEntry* pCurPoly, rdCacheSortFunc pfSort)
{
    size_t polyNum                    = 0;
    int curMatCelNum                  = -1;
    const tStdFadeFactor* pFadeFactor = stdEffect_GetFadeFactor();

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

            ++pCurPoly;
            ++polyNum;

            if ( 3 * (pCurPoly->numVertices - 2) + totalIndices >= std3D_g_maxVertices // i.e. totalIndices + num required triangle indices for next poly >= std3D_g_maxVertices
                || polyNum >= numPolys
                || pCurMat != pCurPoly->pMaterial
                || curMatCelNum != pCurPoly->matCelNum
                || fflags != pCurPoly->flags )
            {
                RD_ASSERTREL(rdCache_totalVerts < RDCACHE_VERTBUFFERSIZE);
#ifdef J3D_OPENGL
                rdCache_AddLegacyDrawCall(pCachedTexture, rdflags, rdCache_aHWVertices, rdCache_totalVerts, rdCache_aVertIndices, totalIndices, pfSort == rdCache_ProcFaceDistanceCompare);
#else
                std3D_DrawRenderList(pCachedTexture, rdflags, rdCache_aHWVertices, rdCache_totalVerts, rdCache_aVertIndices, totalIndices);
#endif
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

#define RD_CACHE_MAX_INSTANCES 5000

typedef struct srdFaceDrawInfo
{
    size_t indexOffset;
    size_t numElements;
} rdFaceDrawInfo;

typedef struct srdDrawCallSortBucket
{
    uint64_t key;
    size_t index;
} rdDrawCallSortBucket;

static rdFaceDrawInfo* rdCache_aFaceDrawInfos = NULL;
static size_t rdCache_numFaces                = 0;

static rdPayload rdCache_aOpaqueDrawCalls[RD_CACHE_MAX_OPAQUE_DRAW_CALLS];
static rdDrawCallSortBucket rdCache_aOpaqueSortBuckets[RD_CACHE_MAX_OPAQUE_DRAW_CALLS];
static size_t rdCache_numOpaqueDrawCalls = 0;

static rdPayload rdCache_aTransparentDrawCalls[RD_CACHE_MAX_TRANSPARENT_DRAW_CALLS];
static rdDrawCallSortBucket rdCache_aTransparentSortBuckets[RD_CACHE_MAX_TRANSPARENT_DRAW_CALLS];
static size_t rdCache_numTransparentDrawCalls = 0;

static InstanceData rdCache_aInstanceData[RD_CACHE_MAX_INSTANCES];
static size_t rdCache_numInstances = 0;

static rdDrawType rdCache_currentDrawType;

static GeometryBatch rdCache_geometryBatch = { 0 };
static ModelBatch rdCache_modelBatch       = { 0 };
static QuadBatch rdCache_quadBatch         = { 0 };
static LegacyBatch rdCache_legacyBatch     = { 0 };

static void rdCache_DrawOpaqueDrawCalls(void);
static void rdCache_DrawTransparentDrawCalls(void);
static void rdCache_SendDrawCallsToHardware(rdPayload* aDrawCalls, rdDrawCallSortBucket* aSortBuckets, size_t numDrawCalls);
static size_t rdCache_BatchGeometryDrawCalls(size_t start, rdPayload* aDrawCalls, rdDrawCallSortBucket* aSortBuckets, size_t numDrawCalls);
static size_t rdCache_BatchModelDrawCalls(size_t start, rdPayload* aDrawCalls, rdDrawCallSortBucket* aSortBuckets, size_t numDrawCalls);
static size_t rdCache_BatchQuadDrawCalls(size_t start, rdPayload* aDrawCalls, rdDrawCallSortBucket* aSortBuckets, size_t numDrawCalls);
static size_t rdCache_BatchLegacyDrawCalls(size_t start, rdPayload* aDrawCalls, rdDrawCallSortBucket* aSortBuckets, size_t numDrawCalls);

int rdCache_DrawCallOpaqueCompare(const rdDrawCallSortBucket* pEntry1, rdDrawCallSortBucket* pEntry2)
{
    return (pEntry1->key > pEntry2->key) - (pEntry1->key < pEntry2->key);
}

static int rdCache_DrawCallDistanceCompare(const rdDrawCallSortBucket* pEntry1, const rdDrawCallSortBucket* pEntry2)
{
    rdPayload* a = &rdCache_aTransparentDrawCalls[pEntry1->index];
    rdPayload* b = &rdCache_aTransparentDrawCalls[pEntry2->index];
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

    if ( rs & STD3D_RS_ALPHAREF_SET )
        state |= 1 << 7;

    return state;
}

static uint64_t rdCache_GenerateGeoBatchKey(rdPayload* pDrawCall)
{
    uint64_t key = 0;

    // type is 0-7, so it only needs 3 bits
    key |= ((uint64_t)(pDrawCall->type & 0x7)) << 61;

    // 8 bits for shader should be enough, that still makes 256 possible shaders
    uint8_t shaderId = (uint8_t)((pDrawCall->pShader ? pDrawCall->pShader->handle : 0) & 0xFF);
    key |= ((uint64_t)shaderId) << 53;

    // 16 bits for texture => max 65535 different textures possible
    uint16_t texId = (uint16_t)((pDrawCall->pTex ? pDrawCall->pTex->id : 0) & 0xFFFF);
    key |= ((uint64_t)texId) << 37;

    // 24 bits for extra light colors, alpha shouldn't be needed for opaque batches
    uint32_t rgb24 = D3DRGB(pDrawCall->extraLight.red, pDrawCall->extraLight.green, pDrawCall->extraLight.blue) & 0x00FFFFFF;

    key |= ((uint64_t)rgb24) << 13;

    // 8 bits for batching relevant render flags
    uint8_t batchFlags = Std3D_ExtractBatchState(pDrawCall->rdFlags);
    key |= ((uint64_t)batchFlags) << 5;


    return key;
}

static uint64_t rdCache_GenerateLegacyBatchKey(rdPayload* pDrawCall)
{
    rdLegacyPayload* pPayload = &pDrawCall->legacyPayload;

    uint64_t key = 0;

    key |= ((uint64_t)(pDrawCall->type & 0x7)) << 61;

    uint16_t texId = (uint16_t)((pDrawCall->pTex ? pDrawCall->pTex->id : 0) & 0xFFFF);
    key |= ((uint64_t)texId) << 45;

    uint32_t type = pPayload->type;
    key |= ((uint64_t)type) << 13;

    uint8_t batchFlags = Std3D_ExtractBatchState(pDrawCall->rdFlags);
    key |= ((uint64_t)batchFlags) << 5;

    return key;
}


static uint64_t rdCache_GenerateOpaqueModelBatchKey(rdPayload* pDrawCall)
{
    rdModelFacePayload* pModelData = &pDrawCall->modelFacePayload;
    uint64_t key                   = 0;

    key |= ((uint64_t)pDrawCall->type & 0x7) << 61;

    // Faces with same face num can be rendered via instancing
    key |= ((uint64_t)(pModelData->faceNum & 0xFFFFFFFF)) << 29;

    return key;
}

static uint64_t rdCache_GenerateSpriteBatchKey(rdPayload* pDrawCall)
{
    uint64_t key              = 0;
    rdSpritePayload* pPayload = &pDrawCall->spritePayload;

    key |= ((uint64_t)pDrawCall->type & 0x7) << 61;

    key |= ((uint64_t)pPayload->spriteType & 0xFFu) << 53;


    uint16_t texId = (uint16_t)((pDrawCall->pTex ? pDrawCall->pTex->id : 0) & 0xFFFF);
    key |= ((uint64_t)texId) << 37;

    return key;
}

static uint64_t rdCache_GenerateParticleBatchKey(rdPayload* pDrawCall)
{
    uint64_t key                = 0;
    rdParticlePayload* pPayload = &pDrawCall->particlePayload;


    key |= ((uint64_t)pDrawCall->type & 0x7) << 61;


    uint16_t texId = (uint16_t)((pDrawCall->pTex ? pDrawCall->pTex->id : 0) & 0xFFFF);
    key |= ((uint64_t)texId) << 45;


    float halfSize     = pPayload->particleHalfSize;
    uint16_t qHalfSize = rdQuantizeFloat16(halfSize);
    key |= ((uint64_t)qHalfSize) << 31;


    uint8_t r = rdQuantizeFloat8(pDrawCall->extraLight.red);
    uint8_t g = rdQuantizeFloat8(pDrawCall->extraLight.green);
    uint8_t b = rdQuantizeFloat8(pDrawCall->extraLight.blue);
    key |= ((uint64_t)r << 23);
    key |= ((uint64_t)g << 15);
    key |= ((uint64_t)b << 7);

    return key;
}

static uint64_t rdCache_GeneratePolyLineBatchKey(rdPayload* pDrawCall)
{
    uint64_t key = 0;

    key |= ((uint64_t)(pDrawCall->type & 0x7)) << 61;

    uint16_t texId = (uint16_t)((pDrawCall->pTex ? pDrawCall->pTex->id : 0) & 0xFFFF);
    key |= ((uint64_t)texId) << 45;

    return key;
}


void rdCache_InitFaceDrawInfo(const size_t numFaces)
{
    rdCache_aFaceDrawInfos = STDMALLOC(numFaces * sizeof(rdFaceDrawInfo));
    std3D_InitInstanceVBO(RD_CACHE_MAX_INSTANCES);
}

void rdCache_FreeFaceDrawInfos(void)
{
    if ( !rdCache_aFaceDrawInfos )
        return;

    STDFREE(rdCache_aFaceDrawInfos);
    rdCache_aFaceDrawInfos          = NULL;
    rdCache_numFaces                = 0;
    rdCache_numOpaqueDrawCalls      = 0;
    rdCache_numTransparentDrawCalls = 0;
}

size_t rdCache_AddFaceInfoEntry(const size_t indexOffset, const size_t numVertices)
{
    size_t faceNum           = rdCache_numFaces++;
    rdFaceDrawInfo* drawInfo = &rdCache_aFaceDrawInfos[faceNum];
    drawInfo->indexOffset    = indexOffset;
    drawInfo->numElements    = numVertices;

    return faceNum;
}

static rdPayload* rdCache_GetDrawCall(rdDrawType type, rdPayload* aDrawCalls, size_t numDrawCalls)
{
    rdCache_currentDrawType = type;

    rdPayload* pDrawCall = &aDrawCalls[numDrawCalls];

    pDrawCall->pShader   = NULL;
    pDrawCall->pMaterial = NULL;
    return pDrawCall;
}

rdPayload* rdCache_GetOpaqueDrawCall(rdDrawType type)
{
    if ( rdCache_numInstances >= RD_CACHE_MAX_INSTANCES )
    {
        RDLOG_WARNING("rdCache_aInstanceData is too small to render all instances at once for this frame."
            " Consider increasing 'RD_CACHE_MAX_INSTANCES' for improving instancing.\n");
        rdCache_DrawOpaqueDrawCalls();
        rdCache_DrawTransparentDrawCalls();
    }
    else if ( rdCache_numOpaqueDrawCalls >= RD_CACHE_MAX_OPAQUE_DRAW_CALLS )
    {
        RDLOG_WARNING("'rdCache_aOpaqueDrawCalls' is too small to store all transparent draw calls for this frame."
            " Consider increasing 'RD_CACHE_MAX_OPAQUE_DRAW_CALLS' for better batching possibilies!");
        rdCache_DrawOpaqueDrawCalls();
    }
    return rdCache_GetDrawCall(type, rdCache_aOpaqueDrawCalls, rdCache_numOpaqueDrawCalls);
}

rdPayload* rdCache_GetTransparentDrawCall(rdDrawType type)
{
    if ( rdCache_numInstances >= RD_CACHE_MAX_INSTANCES )
    {
        RDLOG_WARNING("rdCache_aInstanceData is too small to render all instances at once for this frame."
            " Consider increasing 'RD_CACHE_MAX_INSTANCES' for improving instancing.\n");
        rdCache_DrawOpaqueDrawCalls();
        rdCache_DrawTransparentDrawCalls();
    }
    else if ( rdCache_numTransparentDrawCalls >= RD_CACHE_MAX_TRANSPARENT_DRAW_CALLS )
    {
        RDLOG_WARNING("'rdCache_aTransparentDrawCalls' is too small to store all transparent draw calls for this frame. The transparent faces may will be rendered in the wrong order. "
            "Consider increasing 'RD_CACHE_MAX_TRANSPARENT_DRAW_CALLS'!.\n");
        rdCache_DrawTransparentDrawCalls();
    }
    return rdCache_GetDrawCall(type, rdCache_aTransparentDrawCalls, rdCache_numTransparentDrawCalls);
}


static Std3DRenderState rdCache_GetRenderStateOfDrawCall(const rdPayload* pDrawCall)
{
    rdFaceFlags fFlags       = pDrawCall->flags;
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

    if ( (fFlags & RD_FF_DOUBLE_SIDED) != 0 || (rdroid_g_curRenderOptions & RDROID_BACKFACE_CULLING_ENABLED) == 0 )
    {
        rdFlags |= STD3D_CULL_DISABLED;
    }

    if ( pDrawCall->pMaterial && pDrawCall->pMaterial->formatType == STDCOLOR_FORMAT_RGBA_1BITALPHA )
    {
        rdFlags |= STD3D_RS_ALPHAREF_SET;
        rdFlags &= ~STD3D_RS_ZWRITE_DISABLED;
    }

    return rdFlags;
}

static tSysTexture* rdCache_GetDrawCallTexture(const rdPayload* pDrawCall)
{
    rdMaterial* pCurMat = NULL;
    if ( rdroid_g_curGeometryMode != RD_GEOMETRY_SOLID )
    {
        pCurMat = pDrawCall->pMaterial;
    }

    int curMatCelNum            = -1;
    tSysTexture* pCachedTexture = NULL;
    if ( pCurMat )
    {
        tSystemTexture* pTex = NULL;
        curMatCelNum         = pDrawCall->matCelNum;
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
                curMatCelNum = pDrawCall->matCelNum;
            }

            pTex = &pCurMat->aTextures[curMatCelNum];
            rdCache_AddToTextureCache(pTex, pCurMat->formatType);
        }

        pCachedTexture = pTex->pCachedTexture;
    }

    return pCachedTexture;
}

static void rdCache_AddDrawCall(rdPayload* pDrawCall, rdDrawCallSortBucket* pSortBucket)
{
    pDrawCall->type = rdCache_currentDrawType;
    if ( rdCache_currentDrawType == RD_DRAW_LEGACY )
    {
        pSortBucket->key = rdCache_GenerateLegacyBatchKey(pDrawCall);
        return;
    }

    pDrawCall->rdFlags = rdCache_GetRenderStateOfDrawCall(pDrawCall);
    pDrawCall->pTex    = rdCache_GetDrawCallTexture(pDrawCall);

    rdVector4* extraLight = &pDrawCall->extraLight;
    extraLight->red       = STDMATH_CLAMP(extraLight->red, 0.0f, 1.0f);
    extraLight->green     = STDMATH_CLAMP(extraLight->green, 0.0f, 1.0f);
    extraLight->blue      = STDMATH_CLAMP(extraLight->blue, 0.0f, 1.0f);
    extraLight->alpha     = STDMATH_CLAMP(extraLight->alpha, 0.0f, 1.0f);

    if ( rdroid_g_curGeometryMode == RD_GEOMETRY_VERTEX || rdroid_g_curGeometryMode == RD_GEOMETRY_WIREFRAME )
    {
        pDrawCall->lightingMode = RD_LIGHTING_NONE;
        pDrawCall->rdFlags &= ~STD3D_RS_FOG_ENABLED;
    }

    switch ( rdCache_currentDrawType )
    {
        case RD_DRAW_GEOMETRY:
            pSortBucket->key = rdCache_GenerateGeoBatchKey(pDrawCall);
            return;
        case RD_DRAW_MODEL:
            pSortBucket->key = rdCache_GenerateOpaqueModelBatchKey(pDrawCall);
            break;
        case RD_DRAW_SPRITE:
            pSortBucket->key = rdCache_GenerateSpriteBatchKey(pDrawCall);
            break;
        case RD_DRAW_PARTICLE:
            pSortBucket->key = rdCache_GenerateParticleBatchKey(pDrawCall);
            break;
        case RD_DRAW_POLYLINE:
        case RD_DRAW_SHADOW:
            pSortBucket->key = rdCache_GeneratePolyLineBatchKey(pDrawCall);
            break;
        default:
            break;
    }
    rdCache_numInstances++;
}

void rdCache_AddOpaqueDrawCall(void)
{
    rdPayload* header                 = &rdCache_aOpaqueDrawCalls[rdCache_numOpaqueDrawCalls];
    rdDrawCallSortBucket* pSortBucket = &rdCache_aOpaqueSortBuckets[rdCache_numOpaqueDrawCalls];
    rdCache_AddDrawCall(header, pSortBucket);
    pSortBucket->index = rdCache_numOpaqueDrawCalls++;
}

void rdCache_AddTransparentDrawCall(void)
{
    rdPayload* header                 = &rdCache_aTransparentDrawCalls[rdCache_numTransparentDrawCalls];
    rdDrawCallSortBucket* pSortBucket = &rdCache_aTransparentSortBuckets[rdCache_numTransparentDrawCalls];
    rdCache_AddDrawCall(header, pSortBucket);
    header->rdFlags |= STD3D_BLEND_ENABLED;
    pSortBucket->index = rdCache_numTransparentDrawCalls++;
}

static size_t rdCache_SetInstanceData(rdPayload* aDrawCalls, rdDrawCallSortBucket* aSortBuckets, size_t numDrawCalls)
{
    size_t numInstances = 0;

    for ( int i = 0; i < numDrawCalls; i++ )
    {
        rdDrawCallSortBucket* pSortBucket = &aSortBuckets[i];
        rdPayload* header                 = &aDrawCalls[pSortBucket->index];
        if ( header->type == RD_DRAW_GEOMETRY || header->type == RD_DRAW_LEGACY )
            continue;

        InstanceData* pData = &rdCache_aInstanceData[numInstances++];

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

                stdShader_ConvertToMat4(&pSpriteData->modelMatrix, pData->modelMatrix);

                pData->spriteOffset[0] = pSpriteData->spriteOffset.x;
                pData->spriteOffset[1] = pSpriteData->spriteOffset.z;
                pData->spriteOffset[2] = -pSpriteData->spriteOffset.y;

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
    rdCache_numInstances -= numInstances;
    return numInstances;
}

static void rdCache_DrawOpaqueDrawCalls(void)
{
    if ( !rdCache_numOpaqueDrawCalls )
    {
        return;
    }

    std3D_SetDrawMode(rdroid_g_curGeometryMode);

    qsort(rdCache_aOpaqueSortBuckets, rdCache_numOpaqueDrawCalls, sizeof(rdDrawCallSortBucket), rdCache_DrawCallOpaqueCompare);

    size_t numInstances = rdCache_SetInstanceData(rdCache_aOpaqueDrawCalls, rdCache_aOpaqueSortBuckets, rdCache_numOpaqueDrawCalls);

    std3D_UpdateInstanceVBO(rdCache_aInstanceData, numInstances);

    rdCache_SendDrawCallsToHardware(rdCache_aOpaqueDrawCalls, rdCache_aOpaqueSortBuckets, rdCache_numOpaqueDrawCalls);

    rdCache_numOpaqueDrawCalls = 0;
    //rdCache_numInstances       = 0;
}

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

    rdCache_DrawOpaqueDrawCalls();
}

static void rdCache_DrawTransparentDrawCalls(void)
{
    if ( !rdCache_numTransparentDrawCalls )
    {
        return;
    }

    std3D_SetDrawMode(rdroid_g_curGeometryMode);

    qsort(rdCache_aTransparentSortBuckets, rdCache_numTransparentDrawCalls, sizeof(rdDrawCallSortBucket), rdCache_DrawCallDistanceCompare);

    size_t numInstances = rdCache_SetInstanceData(rdCache_aTransparentDrawCalls, rdCache_aTransparentSortBuckets, rdCache_numTransparentDrawCalls);

    std3D_UpdateInstanceVBO(rdCache_aInstanceData, numInstances);

    rdCache_SendDrawCallsToHardware(rdCache_aTransparentDrawCalls, rdCache_aTransparentSortBuckets, rdCache_numTransparentDrawCalls);

    rdCache_numTransparentDrawCalls = 0;
    //rdCache_numInstances            = 0;
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

    rdCache_DrawTransparentDrawCalls();
}

static void rdCache_SendDrawCallsToHardware(rdPayload* aDrawCalls, rdDrawCallSortBucket* aSortBuckets, size_t numDrawCalls)
{
    for ( size_t i = 0; i < numDrawCalls; i++ )
    {
        rdDrawCallSortBucket* pSortBucket = &aSortBuckets[i];
        rdPayload* pDrawCall              = &aDrawCalls[pSortBucket->index];
        size_t drawCount                  = 0;
        switch ( pDrawCall->type )
        {
            case RD_DRAW_GEOMETRY:
                drawCount = rdCache_BatchGeometryDrawCalls(i, aDrawCalls, aSortBuckets, numDrawCalls);
                std3D_DrawGeometryBatch(&rdCache_geometryBatch);
                break;
            case RD_DRAW_MODEL:
                drawCount = rdCache_BatchModelDrawCalls(i, aDrawCalls, aSortBuckets, numDrawCalls);
                std3D_DrawModelBatch(&rdCache_modelBatch);
                break;
            case RD_DRAW_SPRITE:
                drawCount = rdCache_BatchQuadDrawCalls(i, aDrawCalls, aSortBuckets, numDrawCalls);
                const rdSpritePayload* pPayload = &pDrawCall->spritePayload;
                rdCache_quadBatch.spriteType    = pPayload->spriteType;
                rdCache_quadBatch.pShader       = stdShader_GetShader("std_sprite");
                std3D_DrawQuadBatch(&rdCache_quadBatch);
                break;
            case RD_DRAW_PARTICLE:
                drawCount = rdCache_BatchQuadDrawCalls(i, aDrawCalls, aSortBuckets, numDrawCalls);
                rdCache_quadBatch.pShader = stdShader_GetShader("std_particle");
                std3D_DrawQuadBatch(&rdCache_quadBatch);
                break;
            case RD_DRAW_POLYLINE:
            case RD_DRAW_SHADOW:
                drawCount = rdCache_BatchQuadDrawCalls(i, aDrawCalls, aSortBuckets, numDrawCalls);
                rdCache_quadBatch.pShader = stdShader_GetShader("std_polyline");
                std3D_DrawQuadBatch(&rdCache_quadBatch);
                break;
            case RD_DRAW_LEGACY:
                drawCount = rdCache_BatchLegacyDrawCalls(i, aDrawCalls, aSortBuckets, numDrawCalls);
                std3D_CacheLegacyBatch(rdCache_legacyBatch);
                break;
        }

        i += drawCount - 1;
    }
}

static size_t rdCache_BatchGeometryDrawCalls(size_t start, rdPayload* aDrawCalls, rdDrawCallSortBucket* aSortBuckets, size_t numDrawCalls)
{
    size_t drawCount = 0;
    size_t i         = start;

    rdDrawCallSortBucket* pSortBucket = &aSortBuckets[i];

    rdPayload* pDrawCall      = &aDrawCalls[pSortBucket->index];
    rdGeoFacePayload* payload = &pDrawCall->geoFacePayload;
    uint64_t sortKey          = pSortBucket->key;

    rdFaceDrawInfo* drawInfo = &rdCache_aFaceDrawInfos[payload->faceNum];

    // Batch state
    rdCache_geometryBatch.pTex          = pDrawCall->pTex;
    rdCache_geometryBatch.pShader       = pDrawCall->pShader;
    rdCache_geometryBatch.rdFlags       = pDrawCall->rdFlags;
    rdCache_geometryBatch.extraLight[0] = pDrawCall->extraLight.red;
    rdCache_geometryBatch.extraLight[1] = pDrawCall->extraLight.green;
    rdCache_geometryBatch.extraLight[2] = pDrawCall->extraLight.blue;
    rdCache_geometryBatch.extraLight[3] = pDrawCall->extraLight.alpha;
    rdCache_geometryBatch.lightMode     = pDrawCall->lightingMode;

    // First draw
    rdCache_geometryBatch.indexCounts[0]  = drawInfo->numElements;
    rdCache_geometryBatch.indexOffsets[0] = drawInfo->indexOffset * sizeof(GLuint);
    drawCount++;
    i++;

    // Batch following
    while ( i < numDrawCalls && drawCount < MAX_BATCHES )
    {
        pSortBucket = &aSortBuckets[i];
        pDrawCall   = &aDrawCalls[pSortBucket->index];
        if ( pSortBucket->key != sortKey ||
            pDrawCall->lightingMode != rdCache_geometryBatch.lightMode || pDrawCall->rdFlags != rdCache_geometryBatch.rdFlags )
        {
            break;
        }

        payload  = &pDrawCall->geoFacePayload;
        drawInfo = &rdCache_aFaceDrawInfos[payload->faceNum];

        rdCache_geometryBatch.indexCounts[drawCount]  = drawInfo->numElements;
        rdCache_geometryBatch.indexOffsets[drawCount] = drawInfo->indexOffset * sizeof(GLuint);

        drawCount++;
        i++;
    }

    rdCache_geometryBatch.drawCount = (GLsizei)drawCount;
    return drawCount;
}

static size_t rdCache_BatchModelDrawCalls(size_t start, rdPayload* aDrawCalls, rdDrawCallSortBucket* aSortBuckets, size_t numDrawCalls)
{
    size_t drawCount = 0;
    size_t i         = start;

    rdDrawCallSortBucket* pSortBucket = &aSortBuckets[i];

    rdPayload* header             = &aDrawCalls[pSortBucket->index];
    rdModelFacePayload* pDrawCall = &header->modelFacePayload;
    uint64_t sortKey              = pSortBucket->key;

    // Batch state
    rdCache_modelBatch.pTex        = header->pTex;
    rdCache_modelBatch.pShader     = header->pShader;
    rdCache_modelBatch.rdFlags     = header->rdFlags;
    rdCache_modelBatch.lightMode   = header->lightingMode; // First draw
    rdCache_modelBatch.indexCount  = rdCache_aFaceDrawInfos[pDrawCall->faceNum].numElements;
    rdCache_modelBatch.indexOffset = rdCache_aFaceDrawInfos[pDrawCall->faceNum].indexOffset;
    drawCount++;
    i++;

    // Batch following
    while ( i < numDrawCalls )
    {
        pSortBucket = &aSortBuckets[i];
        header      = &aDrawCalls[pSortBucket->index];

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

static size_t rdCache_BatchQuadDrawCalls(size_t start, rdPayload* aDrawCalls, rdDrawCallSortBucket* aSortBuckets, size_t numDrawCalls)
{
    size_t drawCount = 0;
    size_t i         = start;

    rdDrawCallSortBucket* pSortBucket = &aSortBuckets[i];
    rdPayload* pDrawCall              = &aDrawCalls[pSortBucket->index];
    uint64_t sortKey                  = pSortBucket->key;

    QuadBatch* pBatch = &rdCache_quadBatch;

    // Batch state
    pBatch->pTex      = pDrawCall->pTex;
    pBatch->rdFlags   = pDrawCall->rdFlags;
    pBatch->lightMode = pDrawCall->lightingMode;


    drawCount++;
    i++;

    // Batch following
    while ( i < numDrawCalls )
    {
        pSortBucket = &aSortBuckets[i];
        pDrawCall   = &aDrawCalls[pSortBucket->index];

        if ( pSortBucket->key != sortKey ||
            pDrawCall->lightingMode != pBatch->lightMode )
        {
            break;
        }
        drawCount++;
        i++;
    }

    pBatch->numberOfInstances = (GLsizei)drawCount;
    return drawCount;
}

static size_t rdCache_BatchLegacyDrawCalls(size_t start, rdPayload* aDrawCalls, rdDrawCallSortBucket* aSortBuckets, size_t numDrawCalls)
{
    size_t drawCount = 0;
    size_t i         = start;

    rdDrawCallSortBucket* pSortBucket = &aSortBuckets[i];

    rdPayload* pDrawCall     = &aDrawCalls[pSortBucket->index];
    rdLegacyPayload* payload = &pDrawCall->legacyPayload;
    uint64_t sortKey         = pSortBucket->key;

    // Batch state
    rdCache_legacyBatch.pTex    = pDrawCall->pTex;
    rdCache_legacyBatch.rdFlags = pDrawCall->rdFlags;
    rdCache_legacyBatch.type    = payload->type;

    // First draw
    rdCache_legacyBatch.indexCounts[0]  = payload->numIndices;
    rdCache_legacyBatch.indexOffsets[0] = payload->indexOffset * sizeof(GLushort);
    drawCount++;
    i++;

    // Batch following
    while ( i < numDrawCalls && drawCount < MAX_BATCHES )
    {
        pSortBucket = &aSortBuckets[i];
        if ( pSortBucket->key != sortKey )
        {
            break;
        }

        pDrawCall = &aDrawCalls[pSortBucket->index];
        payload   = &pDrawCall->legacyPayload;

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
        pDrawCall           = rdCache_GetTransparentDrawCall(RD_DRAW_LEGACY);
        pDrawCall->distance = RD_CACHE_MAX_TRANSPARENT_DRAW_CALLS - rdCache_numTransparentDrawCalls;
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

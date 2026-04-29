#include "sithRender.h"
#include <j3dcore/j3dhook.h>

#include <rdroid/Engine/rdCamera.h>
#include <rdroid/Engine/rdClip.h>
#include <rdroid/Engine/rdKeyframe.h>
#include <rdroid/Engine/rdLight.h>
#include <rdroid/Engine/rdMaterial.h>
#include <rdroid/Engine/rdPuppet.h>
#include <rdroid/Math/rdMath.h>
#include <rdroid/Math/rdMatrix.h>
#include <rdroid/Math/rdVector.h>
#include <rdroid/Primitives/rdModel3.h>
#include <rdroid/Primitives/rdPrimit3.h>
#include <rdroid/Raster/rdCache.h>

#include <sith/AI/sithAIUtil.h>
#include <sith/Cog/sithCog.h>
#include <sith/Devices/sithConsole.h>
#include <sith/Engine/sithCamera.h>
#include <sith/Engine/sithPuppet.h>
#include <sith/Engine/sithRenderSky.h>
#include <sith/Engine/sithShadow.h>
#include <sith/Gameplay/sithTime.h>
#include <sith/RTI/symbols.h>
#include <sith/World/sithMaterial.h>
#include <sith/World/sithModel.h>
#include <sith/World/sithPVS.h>
#include <sith/World/sithSprite.h>
#include <sith/World/sithThing.h>
#include <sith/World/sithVoice.h>
#include <sith/World/sithWorld.h>

#include  <std/General/stdEffect.h>
#include  <std/General/stdFileUtil.h>
#include  <std/General/stdFnames.h>
#include  <std/General/stdMath.h>
#include  <std/General/stdMemory.h>
#include  <std/General/stdUtil.h>
#include  <std/Win95/std3D.h>
#include  <std/Win95/stdDisplay.h>

#include <math.h>
#include <stdint.h>

#include "std/Win95/stdShader.h"

#ifdef J3D_OPENGL
#include <sith/Engine/sithRenderVertexWelder.h>
#endif

typedef struct sSithRenderSectorQueueEntry
{
    SithSector* pSector;
    float distance;
} SithRenderSectorQueueEntry;

// Queue for BFS traversal
typedef struct sSithRenderSectorQueue
{
    size_t head;
    size_t tail;
    SithRenderSectorQueueEntry aEntries[SITHRENDER_MAX_VISIBLE_THING_SECTORS * 4];
} SithRenderSectorQueue;

#define SITHRENDER_MAXTHINGLIGHTS   RDCAMERA_MAX_LIGHTS / 2  // 64; note this var must not exceed RDCAMERA_MAX_LIGHTS - 1 (1 for sector light)
#define SITHRENDER_MAXSECTORLIGHTS  (RDCAMERA_MAX_LIGHTS - SITHRENDER_MAXTHINGLIGHTS)

// There are 2 types of thing light the dynamic light that affect emitting thing and surrounding area,
// and there is flat light that lits only the emitting thing.
#define SITHRENDER_FLATLIGHRANGE          0.01f
#define SITHRENDER_ISDYNAMICLIGHT(range) (range > SITHRENDER_FLATLIGHRANGE)
#define SITHRENDER_ISFLATLIGHT(range )   (range <= SITHRENDER_FLATLIGHRANGE)

#define SITHRENDER_MAXSCREENSHOTS 100u

// Render state vars
static int sithRender_renderflags;
rdLightMode sithRender_lightMode;

// Sector occlusion related vars
static size_t sithRender_numSectorFrustums;
static rdClipFrustum sithRender_aSectorFrustrums[SITHRENDER_MAX_VISIBLE_SECTORS];

static SithSector* sithRender_aVisibleSectors[SITHRENDER_MAX_VISIBLE_SECTORS];

// PVS vars
static bool sithRender_bPVSCull = true;
static size_t sithRender_curPVSIndex;
static size_t sithRender_lastPVSIndex;
static uint8_t* sithRender_aAdjoinTable;
static SithSurfaceAdjoin* sithRender_aVisibleAdjoins[SITHRENDER_MAX_VISIBLE_SECTORS];

// Visible thing sector(s) related vars
static SithRenderThingTraversal sithRender_culledSectorTraversalMode = SITHRENDER_CULLEDSECTOR_TRAVERSALMODE_DEFAULT; // Added

static size_t sithRender_totalVisibleThingSectors;
static size_t sithRender_numVisibleThingSectors;
static SithSector* sithRender_aVisibleThingSectors[SITHRENDER_MAX_VISIBLE_THING_SECTORS];

static float sithRender_maxThingCollectDistance = SITHRENDER_MAXTHINGCOLLECTDISTANCE_DEFAULT;
static float sithRender_maxLightCollectDistance = SITHRENDER_MAXLIGHTCOLLECTDISTANCE_DEFAULT;

// Queue for BFS traversal
static SithRenderSectorQueue sithRender_visitedSectorQueue = { 0 };

// Rendering vars
static bool sithRender_bResetCameraAspect;
static size_t sithRender_curCamSectorIdx; // Not used for anything particular

static size_t sithRender_numRenderedSectors;

static size_t sithRender_numSectorPointLights;
static rdLight sithRender_aSectorPointLights[SITHRENDER_MAXSECTORLIGHTS];

static size_t sithRender_numAlphaAdjoins;
static SithSurface* sithRender_aAlphaAdjoins[SITHRENDER_MAX_VISIBLE_SECTORS / 2]; // Altered: Was fixed 64; I guess half of 128 visible sectors

// Transformers & Clipping vars
static rdPrimit3 sithRender_clipFaceView;
static rdPrimit3 sithRender_faceView;
static rdVector3 sithRender_aClipVertices[MAX_CLIP_VERTICIES]               = { 0 }; // Added: Init to 0
static rdVector3 sithRender_aTransformedClipVertices[MAX_CLIP_VERTICIES]    = { 0 }; // Added: Init to 0
static rdVector3 sithRender_aSurfaceTransformedVertices[MAX_CLIP_VERTICIES] = { 0 }; // Added: Init to 0

// Thing render vars
static int (J3DAPI* sithRender_pExtraThingRenderFunc)(SithThing* pThing);

static size_t sithRender_numSpritesToDraw;

static size_t sithRender_numThingLights;
static rdLight sithRender_aThingLights[SITHRENDER_MAXTHINGLIGHTS];

static void sithRender_Draw(void);

static void J3DAPI sithRender_BuildVisibleSectorList(SithSector* pSector, rdClipFrustum* pFrustrum);
static void J3DAPI sithRender_BuildVisibleSurface(SithSurface* pSurface);
static void J3DAPI sithRender_BuildClipFrustrum(rdClipFrustum* pFrustrum, size_t numVertices, float orthLeft, float orthTop, float orthRight, float orthBottom);
static void J3DAPI sithRender_BuildVisibleSectorListPVS(SithSector* pSector, rdClipFrustum* pClipFrustum);
static void J3DAPI sithRender_BuildVisibleSectorPVS(SithSector* pSector);
static void J3DAPI sithRender_BuildVisibleSector(SithSector* pSector, const rdClipFrustum* pFrustrum);

static void sithRender_RenderSectors(void);

static void sithRender_BuildVisibleThingSectorList(void);
static void sithRender_BuildVisibleThingSectorListDFS(void); // New legacy sithRender_BuildVisibleThingSectorList put into this func
static void J3DAPI sithRender_CollectVisibleThingSector(SithSector* pSector, float curDistance, float startDistance);
static void sithRender_BuildVisibleThingSectorListBFS(void); // New

static inline void sithRender_CollectThingLights(const SithThing* pThing); // New

static void sithRender_BuildDynamicLights(void);
static void sithRender_RenderThings(void);
static int J3DAPI sithRender_RenderThing(SithThing* pThing);
static void sithRender_RenderAlphaAdjoins(void);

void sithRender_InstallHooks(void)
{
    J3D_HOOKFUNC(sithRender_Startup);
    J3D_HOOKFUNC(sithRender_Open);
    J3D_HOOKFUNC(sithRender_Close);
    J3D_HOOKFUNC(sithRender_Shutdown);
    J3D_HOOKFUNC(sithRender_SetRenderFlags);
    J3D_HOOKFUNC(sithRender_GetRenderFlags);
    J3D_HOOKFUNC(sithRender_SetLightingMode);
    J3D_HOOKFUNC(sithRender_Draw);
    J3D_HOOKFUNC(sithRender_BuildVisibleSectorList);
    J3D_HOOKFUNC(sithRender_BuildVisibleSurface);
    J3D_HOOKFUNC(sithRender_BuildClipFrustrum);
    J3D_HOOKFUNC(sithRender_BuildVisibleSectorListPVS);
    J3D_HOOKFUNC(sithRender_BuildVisibleSectorPVS);
    J3D_HOOKFUNC(sithRender_BuildVisibleSector);
    J3D_HOOKFUNC(sithRender_RenderSectors);
    J3D_HOOKFUNC(sithRender_BuildVisibleThingSectorList);
    J3D_HOOKFUNC(sithRender_CollectVisibleThingSector);
    J3D_HOOKFUNC(sithRender_BuildDynamicLights);
    J3D_HOOKFUNC(sithRender_RenderThings);
    J3D_HOOKFUNC(sithRender_RenderThing);
    J3D_HOOKFUNC(sithRender_RenderAlphaAdjoins);
    J3D_HOOKFUNC(sithRender_MakeScreenShot);
}

void sithRender_ResetGlobals(void)
{
    float jonesConfig_g_fogDensity_tmp = 100.0f;
    memcpy(&sithRender_g_fogDensity, &jonesConfig_g_fogDensity_tmp, sizeof(sithRender_g_fogDensity));

    STD_ZEROMEM(&sithRender_g_numDrawnThings, sizeof(sithRender_g_numDrawnThings));
    STD_ZEROMEM(&sithRender_g_numArchPolys, sizeof(sithRender_g_numArchPolys));
    STD_ZEROMEM(&sithRender_g_numAlphaArchPolys, sizeof(sithRender_g_numAlphaArchPolys));
    STD_ZEROMEM(&sithRender_g_numThingPolys, sizeof(sithRender_g_numThingPolys));
    STD_ZEROMEM(&sithRender_g_numAlphaThingPoly, sizeof(sithRender_g_numAlphaThingPoly));
    STD_ZEROMEM(&sithRender_g_numVisibleAdjoins, sizeof(sithRender_g_numVisibleAdjoins));
    STD_ZEROMEM(&sithRender_g_numVisibleSectors, sizeof(sithRender_g_numVisibleSectors));
}

int sithRender_Startup(void)
{
    rdMaterial_RegisterLoader(sithMaterial_Load);
    rdModel3_RegisterLoader(sithModel_Load);
    rdKeyframe_RegisterLoader(sithPuppet_LoadKeyframe);

    sithRender_renderflags           = 0;
    sithRender_pExtraThingRenderFunc = NULL;
    sithRender_aAdjoinTable          = NULL;
    return 1;
}

#ifdef J3D_OPENGL

static void sithRender_CollectFaces(void)
{
    SithWorld* pCurWorld = sithWorld_g_pCurrentWorld;

    size_t numFaces    = pCurWorld->numSurfaces;
    size_t numVertices = 0;
    size_t numIndices  = 0;

    for ( size_t i = 0; i < pCurWorld->numSurfaces; i++ )
    {
        const SithSurface* pSurface = &pCurWorld->aSurfaces[i];

        const size_t numVerticesOfFace = pSurface->face.numVertices;
        numVertices += numVerticesOfFace;

        // Triangulation
        if ( numVerticesOfFace <= 3u )
        {
            numIndices += 3;
        }
        else
        {
            uint16_t triCount = numVerticesOfFace - 2;
            for ( size_t triNum = 0; triNum < triCount; ++triNum )
            {
                numIndices += 3;
            }
        }
    }

    size_t numModels = pCurWorld->numModels + sithWorld_g_pStaticWorld->numModels;

    rdModel3** aModels = STDMALLOC(numModels * sizeof(rdModel3*));

    size_t idx = 0;

    for ( size_t i     = 0; i < pCurWorld->numModels; i++ )
        aModels[idx++] = &pCurWorld->aModels[i];

    for ( size_t i     = 0; i < sithWorld_g_pStaticWorld->numModels; i++ )
        aModels[idx++] = &sithWorld_g_pStaticWorld->aModels[i];


    for ( size_t i = 0; i < numModels; i++ )
    {
        const rdModel3* pModel = aModels[i];

        const rdModel3GeoSet* pGeoSet = &pModel->aGeos[0];

        for ( size_t k = 0; k < pGeoSet->numMeshes; k++ )
        {
            rdModel3Mesh* pMesh = &pGeoSet->aMeshes[k];
            //numFaces += pMesh->numFaces;

            for ( size_t f = 0; f < pMesh->numFaces; f++ )
            {
                const size_t numVerticesOfFace = pMesh->aFaces[f].numVertices;
                numVertices += numVerticesOfFace;

                // Triangulation
                if ( numVerticesOfFace <= 3u )
                {
                    numIndices += 3;
                }
                else
                {
                    uint16_t triCount = numVerticesOfFace - 2;
                    for ( size_t triNum = 0; triNum < triCount; ++triNum )
                    {
                        numIndices += 3;
                    }
                }
            }

            for ( size_t f = 0; f < pMesh->numFaces; f++ )
            {
                numFaces++;
                rdFace* pFace         = &pMesh->aFaces[f];
                pFace->num            = 1;
                rdMaterial* pMaterial = pFace->pMaterial;
                rdFaceFlags flags     = pFace->flags;
                // if ( (flags & RD_FF_TEX_TRANSLUCENT) != 0 || pMaterial->formatType == STDCOLOR_FORMAT_RGBA )
                // {
                //     continue;
                // }
                rdLightMode lm        = pFace->lightingMode;
                rdGeometryMode gm     = pFace->geometryMode;
                size_t f2             = f;
                while ( ++f2 < pMesh->numFaces )
                {
                    rdFace* pFace2         = &pMesh->aFaces[f2];
                    rdMaterial* pMaterial2 = pFace2->pMaterial;
                    rdFaceFlags flags2     = pFace2->flags;
                    rdLightMode lm2        = pFace2->lightingMode;
                    rdGeometryMode gm2     = pFace2->geometryMode;

                    if ( pMaterial2 != pMaterial ||
                        flags2 != flags ||
                        lm2 != lm ||
                        gm2 != gm )
                    {
                        break;
                    }
                    pFace2->num = -1;
                }
                f = f2 - 1;
            }
        }
    }

    rdCache_InitFaceDrawInfo(numFaces);

    rdVector3* normals = STDMALLOC(sithWorld_g_pCurrentWorld->numVertices * sizeof(rdVector3));
    memset(normals, 0, sithWorld_g_pCurrentWorld->numVertices * sizeof(rdVector3));

    for ( size_t s = 0; s < sithWorld_g_pCurrentWorld->numSurfaces; s++ )
    {
        rdFace* pFace = &sithWorld_g_pCurrentWorld->aSurfaces[s].face;
        for ( size_t pv = 0; pv < pFace->numVertices; pv++ )
        {
            size_t vertIndex = pFace->aVertices[pv];
            rdVector_Add3Acc(&normals[vertIndex], &pFace->normal);
        }
    }


    for ( size_t v = 0; v < sithWorld_g_pCurrentWorld->numVertices; v++ )
    {
        rdVector_Normalize3Acc(&normals[v]);
    }


    LPD3DTLVERTEX vertices = STDMALLOC(numVertices * sizeof(D3DTLVERTEX));
    GLuint* indices        = STDMALLOC(numIndices * sizeof(GLuint));

    size_t currentVert      = 0;
    GLuint currentIndex     = 0;
    GLuint currentVertIndex = 0;

    size_t faceVertexCount = 0;
    size_t faceIndexOffset = 0;

    for ( size_t i = 0; i < pCurWorld->numSurfaces; i++ )
    {
        SithSurface* pSurface          = &pCurWorld->aSurfaces[i];
        rdFace* pFace                  = &pSurface->face;
        const size_t numVerticesOfFace = pFace->numVertices;
        rdVector3 faceNormal           = { 0, 1, 0 };
        if ( numVerticesOfFace >= 3 )
        {
            rdVector3* v0 = &sithWorld_g_pCurrentWorld->aVertices[pFace->aVertices[0]];
            rdVector3* v1 = &sithWorld_g_pCurrentWorld->aVertices[pFace->aVertices[1]];
            rdVector3* v2 = &sithWorld_g_pCurrentWorld->aVertices[pFace->aVertices[2]];

            rdVector3 e0 = { 0 };
            rdVector3 e1 = { 0 };
            rdVector_Sub3(&e0, v1, v0);
            rdVector_Sub3(&e1, v2, v0);
            rdVector_Cross3(&faceNormal, &e0, &e1);
            rdVector_Normalize3Acc(&faceNormal);
        }
        for ( size_t k = 0; k < numVerticesOfFace; k++, currentVert++ )
        {
            STD_ASSERTREL(currentVert < numVertices);
            rdVector3* pVertex      = &sithWorld_g_pCurrentWorld->aVertices[pFace->aVertices[k]];
            LPD3DTLVERTEX pGLVertex = &vertices[currentVert];
            pGLVertex->sx           = pVertex->x;
            pGLVertex->sy           = pVertex->z;
            pGLVertex->sz           = -pVertex->y;
            pGLVertex->rhw          = 1.0f;
            pGLVertex->specular     = 0;

            rdVector2* pTexCoords = &sithWorld_g_pCurrentWorld->aTexVerticies[pFace->aTexVertices[k]];
            pGLVertex->tu         = pTexCoords->x;
            pGLVertex->tv         = pTexCoords->y;

            //rdVector3* pNormal = &normals[pFace->aVertices[k]];
            rdVector3* pNormal = &pFace->normal;
            pGLVertex->nx      = 0;
            pGLVertex->ny      = 0;
            pGLVertex->nz      = 0;

            rdVector4* intensity = &pSurface->aIntensities[k];
            float red            = intensity->red;
            float green          = intensity->green;
            float blue           = intensity->blue;
            pGLVertex->color     = D3DRGB(red, green, blue);
        }

        faceIndexOffset = currentIndex;

        // Triangulation
        if ( numVerticesOfFace <= 3u )
        {
            indices[currentIndex++] = currentVertIndex;
            indices[currentIndex++] = currentVertIndex + 1;
            STD_ASSERTREL(currentIndex < numIndices);
            indices[currentIndex++] = currentVertIndex + 2;
            faceVertexCount         = 3;
        }
        else
        {
            uint16_t triCount     = numVerticesOfFace - 2;
            uint16_t triPoint1Num = 0;
            uint16_t triPoint2Num = 1;
            uint16_t triPoint3Num = numVerticesOfFace - 1;
            faceVertexCount       = triCount * 3;

            for ( size_t triNum = 0; triNum < triCount; ++triNum )
            {
                indices[currentIndex++] = triPoint1Num + currentVertIndex;
                indices[currentIndex++] = triPoint2Num + currentVertIndex;
                STD_ASSERTREL(currentIndex < numIndices);
                indices[currentIndex++] = triPoint3Num + currentVertIndex;

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
        pFace->num = rdCache_AddFaceInfoEntry(faceIndexOffset, faceVertexCount);
        currentVertIndex += numVerticesOfFace;
    }

    STDFREE(normals);

    for ( size_t i = 0; i < numModels; i++ )
    {
        const rdModel3* pModel        = aModels[i];
        const rdModel3GeoSet* pGeoSet = &pModel->aGeos[0];
        for ( size_t k = 0; k < pGeoSet->numMeshes; k++ )
        {
            rdModel3Mesh* pMesh = &pGeoSet->aMeshes[k];
            rdFace* pRenderFace = &pMesh->aFaces[0];
            faceVertexCount     = 0;
            faceIndexOffset     = currentIndex;

            for ( size_t f = 0; f < pMesh->numFaces; f++ )
            {
                rdFace* pFace                  = &pMesh->aFaces[f];
                const size_t numVerticesOfFace = pFace->numVertices;

                for ( size_t l = 0; l < numVerticesOfFace; l++ )
                {
                    STD_ASSERTREL(currentVert < numVertices);
                    rdVector3* pVertex      = &pMesh->apVertices[pFace->aVertices[l]];
                    LPD3DTLVERTEX pGLVertex = &vertices[currentVert++];
                    pGLVertex->sx           = pVertex->x;
                    pGLVertex->sy           = pVertex->z;
                    pGLVertex->sz           = -pVertex->y;
                    pGLVertex->rhw          = 1.0f;
                    pGLVertex->specular     = 0;

                    rdVector2* pTexCoords = &pMesh->apTexVertices[pFace->aTexVertices[l]];
                    pGLVertex->tu         = pTexCoords->x;
                    pGLVertex->tv         = pTexCoords->y;

                    rdVector3* pNormal = &pMesh->aVertNormals[pFace->aVertices[l]];
                    pGLVertex->nx      = pNormal->x;
                    pGLVertex->ny      = pNormal->z;
                    pGLVertex->nz      = -pNormal->y;

                    pGLVertex->color = D3DRGB(0.0f, 0.0f, 0.0f);
                }

                if ( f > 0 && pFace->num != -1 )
                {
                    pRenderFace->num = rdCache_AddFaceInfoEntry(faceIndexOffset, faceVertexCount);
                    faceIndexOffset  = currentIndex;
                    faceVertexCount  = 0;
                    pRenderFace      = pFace;
                }
                //faceIndexOffset = currentIndex;

                // Triangulation
                if ( numVerticesOfFace <= 3u )
                {
                    indices[currentIndex++] = currentVertIndex;
                    indices[currentIndex++] = currentVertIndex + 1;
                    STD_ASSERTREL(currentIndex < numIndices);

                    indices[currentIndex++] = currentVertIndex + 2;
                    faceVertexCount += 3;
                }
                else
                {
                    uint16_t triCount     = numVerticesOfFace - 2;
                    uint16_t triPoint1Num = 0;
                    uint16_t triPoint2Num = 1;
                    uint16_t triPoint3Num = numVerticesOfFace - 1;
                    faceVertexCount += triCount * 3;

                    for ( size_t triNum = 0; triNum < triCount; ++triNum )
                    {
                        indices[currentIndex++] = triPoint1Num + currentVertIndex;
                        indices[currentIndex++] = triPoint2Num + currentVertIndex;
                        STD_ASSERTREL(currentIndex < numIndices);

                        indices[currentIndex++] = triPoint3Num + currentVertIndex;

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
                //pFace->num = rdCache_AddFaceInfoEntry(faceIndexOffset, faceVertexCount);
                currentVertIndex += numVerticesOfFace;
            }
            if ( faceVertexCount > 0 )
            {
                pRenderFace->num = rdCache_AddFaceInfoEntry(faceIndexOffset, faceVertexCount);
            }
        }
    }

    STD_ASSERTREL(currentVert == numVertices);
    STD_ASSERTREL(currentIndex == numIndices);

    D3DTLVERTEX* weldedVertices = NULL;
    size_t numWeldedVertices    = 0;
    if ( sithRenderVertexWelder_Weld(vertices, numVertices, indices, numIndices, &weldedVertices, &numWeldedVertices) )
    {
        std3D_InitGeometryVBO(weldedVertices, numWeldedVertices, indices, numIndices);
        STDFREE(weldedVertices);
    }
    else
    {
        std3D_InitGeometryVBO(vertices, numVertices, indices, numIndices);
    }

    STDFREE(aModels);
    STDFREE(vertices);
    STDFREE(indices);
}
#endif

int sithRender_Open(void)
{
#ifdef J3D_OPENGL
    sithRender_CollectFaces();
#endif
    sithRender_lightMode = RD_LIGHTING_GOURAUD;

    for ( size_t i = 0; i < STD_ARRAYLEN(sithRender_aThingLights); ++i )
    {
        rdLight_NewEntry(&sithRender_aThingLights[i]);
    }

    // TODO: Verify if sithRender_aSectorPointLights array also has to be initialized

    if ( sithRenderSky_Open(sithWorld_g_pCurrentWorld->horizonDistance, sithWorld_g_pCurrentWorld->ceilingSkyHeight) )
    {
        RDLOG_ERROR("Unable to Open sithRenderSky.\n"); // TODO: Why using RDLOG
        return 1;
    }

    sithRender_aAdjoinTable = (uint8_t*)STDMALLOC(sithWorld_g_pCurrentWorld->numAdjoins + 1);
    if ( !sithRender_aAdjoinTable )
    {
        RDLOG_ERROR("Unable to Alloc Adjoin Table.\n"); // TODO: Why using RDLOG
        return 1;
    }

    sithRender_bResetCameraAspect = false;
    return 0;
}

void sithRender_Close(void)
{
    if ( sithRender_aAdjoinTable )
    {
        stdMemory_Free(sithRender_aAdjoinTable);
    }

    sithRender_aAdjoinTable = NULL;
    sithRenderSky_Close();
#ifdef J3D_OPENGL
    std3D_ReleaseStaticBuffers();
    rdCache_FreeFaceDrawInfos();
#endif
}

void sithRender_Shutdown(void)
{
    sithRender_Close();
    sithRender_pExtraThingRenderFunc = NULL;
    sithRender_aAdjoinTable          = NULL;
}

void J3DAPI sithRender_SetRenderFlags(int flags)
{
    sithRender_renderflags = flags;
}

int sithRender_GetRenderFlags(void)
{
    return sithRender_renderflags;
}

void J3DAPI sithRender_SetLightingMode(rdLightMode mode)
{
    sithRender_lightMode = mode;
}

rdLightMode sithRender_GetLightingMode(void)
{
    return sithRender_lightMode;
}

void J3DAPI sithRender_SetCulledSectorTraversalMode(SithRenderThingTraversal mode)
{
    sithRender_culledSectorTraversalMode = mode;
}

SithRenderThingTraversal sithRender_GetCulledSectorTraversalMode(void)
{
    return sithRender_culledSectorTraversalMode;
}

float sithRender_GetMaxThingCollectDistance(void)
{
    return sithRender_maxThingCollectDistance;
}

void J3DAPI sithRender_SetMaxThingCollectDistance(float distance)
{
    sithRender_maxThingCollectDistance = distance;
}

float sithRender_GetMaxLightCollectDistance(void)
{
    return sithRender_maxLightCollectDistance;
}

void J3DAPI sithRender_SetMaxLightCollectDistance(float distance)
{
    sithRender_maxLightCollectDistance = distance;
}

void sithRender_RenderScene(void)
{
    SithWorld* pWorld = sithWorld_g_pCurrentWorld;
    sithRenderSky_Update();

    if ( (pWorld->state & SITH_WORLD_STATE_UPDATE_FOG) != 0 )
    {
        std3D_g_fogDensity = sithRender_g_fogDensity / 100.0f;
        std3D_SetFog(pWorld->fog.color.red, pWorld->fog.color.green, pWorld->fog.color.blue, pWorld->fog.startDepth, pWorld->fog.endDepth);
        pWorld->state &= ~SITH_WORLD_STATE_UPDATE_FOG;
    }

    if ( sithTime_IsPaused() )
    {
        stdEffect_SetFadeFactor(1, 0.5f);
    }

    sithRender_Draw();
    stdEffect_SetFadeFactor(0, 1.0f);
    sithVoice_Draw();
    sithConsole_Flush();
}

void J3DAPI sithRender_EnablePVSCull(bool bEnable)
{
    sithRender_bPVSCull = bEnable;
}

void sithRender_TogglePVSCull(void)
{
    sithRender_bPVSCull = !sithRender_bPVSCull;
}

void sithRender_Draw(void)
{
    rdSetLightingMode(sithRender_lightMode);

    rdRoidFlags rdflags = rdGetRenterOptions();
    rdflags |= RDROID_USE_AMBIENT_CAMERA_LIGHT;
    rdSetRenderOptions(rdflags);

    // Set camera
    if ( (sithCamera_g_pCurCamera->pSector->flags & SITH_SECTOR_UNDERWATER) != 0 )
    {
        // Little underwater effect
        float sin, cos;
        float angle = sithTime_g_secGameTime * 70.0f;
        stdMath_SinCos(angle, &sin, &cos);

        float fov = sin * 1.0f + sithCamera_g_pCurCamera->fov;
        rdCamera_SetFOV(&sithCamera_g_pCurCamera->rdCamera, fov);

        angle = sithTime_g_secGameTime * 100.0f;
        stdMath_SinCos(angle, &sin, &cos);

        float aspect = sin / 30.0f + sithCamera_g_pCurCamera->aspectRatio;
        rdCamera_SetAspectRatio(&sithCamera_g_pCurCamera->rdCamera, aspect);

        sithRender_bResetCameraAspect = true;
    }
    else if ( sithRender_bResetCameraAspect )
    {
        rdCamera_SetFOV(&sithCamera_g_pCurCamera->rdCamera, sithCamera_g_pCurCamera->fov);
        rdCamera_SetAspectRatio(&sithCamera_g_pCurCamera->rdCamera, sithCamera_g_pCurCamera->aspectRatio);
        sithRender_bResetCameraAspect = false;
    }

    sithRender_g_numVisibleSectors      = 0;
    sithRender_numVisibleThingSectors   = 0;
    sithRender_numThingLights           = 0;
    sithRender_numSectorFrustums        = 0;
    sithRender_numAlphaAdjoins          = 0;
    sithRender_numSpritesToDraw         = 0;
    sithRender_totalVisibleThingSectors = 0;
    sithRender_numRenderedSectors       = 0;
    sithRender_g_numDrawnThings         = 0;
    sithRender_g_numVisibleAdjoins      = 0;

    rdCamera_ClearLights(rdCamera_g_pCurCamera);

    sithRender_curCamSectorIdx = sithCamera_g_pCurCamera->pSector - sithWorld_g_pCurrentWorld->aSectors; // TODO: ?? redundant

    // Collect sector and things to draw
    if ( sithRender_bPVSCull && sithWorld_g_pCurrentWorld->aPVS )
    {
        sithRender_BuildVisibleSectorListPVS(sithCamera_g_pCurCamera->pSector, rdCamera_g_pCurCamera->pFrustum);
    }
    else
    {
        sithRender_BuildVisibleSectorList(sithCamera_g_pCurCamera->pSector, rdCamera_g_pCurCamera->pFrustum);
    }

    sithRender_BuildVisibleThingSectorList();
    if ( sithRender_totalVisibleThingSectors > STD_ARRAYLEN(sithRender_aVisibleThingSectors) ) // Note, this was originally prolly put in place to inform level designers of too many visible thing sectors
    {
        RDLOG_ERROR("Too many sectors with things in view %d of %d\n", STD_ARRAYLEN(sithRender_aVisibleThingSectors), sithRender_totalVisibleThingSectors); // TODO: Why using RDLOG
    }

    if ( (sithRender_renderflags & RDROID_USE_AMBIENT_CAMERA_LIGHT) != 0 )
    {
        sithRender_BuildDynamicLights();
    }

    // Set projection
    std3D_SetProjection(sithCamera_g_pCurCamera->rdCamera.fov, sithCamera_g_pCurCamera->rdCamera.pFrustum->nearPlane, sithCamera_g_pCurCamera->rdCamera.pFrustum->farPlane);

    // Now draw everything
    sithRender_RenderSectors();

    if ( sithRender_numVisibleThingSectors > 0 )
    {
        sithRender_RenderThings();
    }

    if ( sithRender_numAlphaAdjoins > 0 )
    {
        sithRender_RenderAlphaAdjoins();
    }

    sithAIUtil_RenderAIWaypoints();
}

void J3DAPI sithRender_BuildVisibleSectorList(SithSector* pSector, rdClipFrustum* pFrustrum)
{
    if ( pSector->renderTick == sithMain_g_curRenderTick )
    {
        pSector->pClipFrustum = rdCamera_g_pCurCamera->pFrustum;
    }
    else
    {
        sithRender_BuildVisibleSector(pSector, pFrustrum);
    }

    int bBuildingSector           = pSector->bBuildingSector;
    pSector->bBuildingSector      = 1;
    sithRender_faceView.aVertices = sithWorld_g_pCurrentWorld->aTransformedVertices;

    for ( SithSurfaceAdjoin* pAdjoin = pSector->pFirstAdjoin; pAdjoin; pAdjoin = pAdjoin->pNextAdjoin )
    {
        if ( !pAdjoin->pAdjoinSector->bBuildingSector )
        {
            ++sithRender_g_numVisibleAdjoins;

            SithSurface* pSurface = pAdjoin->pAdjoinSurface;

            rdVector3 lookDir;
            rdVector_Sub3(&lookDir, &sithCamera_g_pCurCamera->lookPos, &sithWorld_g_pCurrentWorld->aVertices[*pSurface->face.aVertices]);
            float dot = rdVector_Dot3(&pSurface->face.normal, &lookDir);
            if ( dot > 0.0f || (dot == 0.0f && pSector == sithCamera_g_pCurCamera->pSector) )
            {
                sithRender_BuildVisibleSurface(pSurface);

                sithRender_clipFaceView.aVertices = sithRender_aClipVertices;
                sithRender_faceView.numVertices   = pSurface->face.numVertices;
                sithRender_faceView.aVertIdxs     = pSurface->face.aVertices;
                rdPrimit3_ClipFace(pFrustrum, RD_GEOMETRY_WIREFRAME, RD_LIGHTING_LIT, &sithRender_faceView, &sithRender_clipFaceView, &pSurface->face.texVertOffset);

                if ( (sithRender_clipFaceView.numVertices >= 3 || (rdClip_g_faceStatus & 0x40) != 0)
                    && ((rdClip_g_faceStatus & 0x41) != 0
                        || (pAdjoin->flags & SITH_ADJOIN_VISIBLE) != 0
                        && (!pSurface->face.pMaterial || pSurface->face.geometryMode == RD_GEOMETRY_NONE
                            || (pSurface->face.flags & RD_FF_TEX_TRANSLUCENT) != 0
                            || pSurface->face.pMaterial->formatType != STDCOLOR_FORMAT_RGB)) )
                {
                    rdCamera_g_pCurCamera->pfProjectList(sithRender_aTransformedClipVertices, sithRender_aClipVertices, sithRender_clipFaceView.numVertices);
                    if ( (rdClip_g_faceStatus & 0x41) != 0 )
                    {
                        sithRender_BuildVisibleSectorList(pAdjoin->pAdjoinSector, pFrustrum);
                    }
                    else
                    {
                        rdClipFrustum frustrum;
                        sithRender_BuildClipFrustrum(&frustrum, sithRender_clipFaceView.numVertices, FLT_MAX, FLT_MAX, -FLT_MAX, -FLT_MAX); // OG FLT_MAX was defined as 3.4028235e38f;
                        sithRender_BuildVisibleSectorList(pAdjoin->pAdjoinSector, &frustrum);
                    }
                }
            }
        }
    }

    pSector->bBuildingSector = bBuildingSector;
}

void J3DAPI sithRender_BuildVisibleSurface(SithSurface* pSurface)
{
    if ( pSurface->renderTick != sithMain_g_curRenderTick )
    {
        for ( size_t i = 0; i < pSurface->face.numVertices; ++i )
        {
            int vertIdx = pSurface->face.aVertices[i];
            if ( sithWorld_g_pCurrentWorld->aVertexRenderTickIds[vertIdx] != sithMain_g_curRenderTick )
            {
                // Transform vertices to view space
                rdMatrix_TransformPoint34(
                    &sithWorld_g_pCurrentWorld->aTransformedVertices[vertIdx],
                    &sithWorld_g_pCurrentWorld->aVertices[vertIdx],
                    &rdCamera_g_pCurCamera->viewMatrix
                );
                sithWorld_g_pCurrentWorld->aVertexRenderTickIds[vertIdx] = sithMain_g_curRenderTick;
            }
        }

        pSurface->renderTick = sithMain_g_curRenderTick;
    }
}

void J3DAPI sithRender_BuildClipFrustrum(rdClipFrustum* pFrustrum, size_t numVertices, float orthLeft, float orthTop, float orthRight, float orthBottom)
{
    for ( size_t i = 0; i < numVertices; ++i )
    {
        float x = sithRender_aTransformedClipVertices[i].x;
        float y = sithRender_aTransformedClipVertices[i].y;
        if ( orthLeft >= x )
        {
            orthLeft = x;
        }

        if ( orthRight <= x )
        {
            orthRight = x;
        }

        if ( orthTop >= y )
        {
            orthTop = y;
        }

        if ( orthBottom <= y )
        {
            orthBottom = y;
        }
    }

    pFrustrum->orthoLeftPlane   = orthLeft;
    pFrustrum->orthoTopPlane    = orthTop;
    pFrustrum->orthoRightPlane  = orthRight;
    pFrustrum->orthoBottomPlane = orthBottom;

    int bottom = (int)ceilf(orthBottom);
    int right  = (int)ceilf(orthRight);
    int top    = (int)(ceilf(orthTop) + 0.5f);
    int left   = (int)(ceilf(orthLeft) + 0.5f);
    rdCamera_SetFrustrum(rdCamera_g_pCurCamera, pFrustrum, left, top, right, bottom);
}

void J3DAPI sithRender_BuildVisibleSectorListPVS(SithSector* pSector, rdClipFrustum* pClipFrustum)
{
    sithPVS_SetTable(sithRender_aAdjoinTable, &sithWorld_g_pCurrentWorld->aPVS[pSector->pvsIdx], sithWorld_g_pCurrentWorld->numAdjoins);
    STD_ZEROMEM(sithRender_aVisibleAdjoins, sizeof(sithRender_aVisibleAdjoins));

    sithRender_curPVSIndex     = 1;
    sithRender_faceView.aVertices = sithWorld_g_pCurrentWorld->aTransformedVertices;

    uint32_t width, height;
    stdDisplay_GetBackBufferSize(&width, &height);
    pClipFrustum->orthoLeftPlane   = 0.0f;
    pClipFrustum->orthoTopPlane    = 0.0f;
    pClipFrustum->orthoRightPlane  = (float)width;
    pClipFrustum->orthoBottomPlane = (float)height;

    pSector->pClipFrustum = pClipFrustum;
    sithRender_BuildVisibleSectorPVS(pSector);

    while ( sithRender_curPVSIndex <= sithRender_lastPVSIndex )
    {
        SithSurfaceAdjoin* pAdjoin = sithRender_aVisibleAdjoins[sithRender_curPVSIndex];
        sithRender_aVisibleAdjoins[sithRender_curPVSIndex] = NULL;

        while ( pAdjoin )
        {
            ++sithRender_g_numVisibleAdjoins;

            rdClipFrustum* pFrustum = pAdjoin->pAdjoinSurface->pSector->pClipFrustum;
            if ( (sithRender_aAdjoinTable[pAdjoin - sithWorld_g_pCurrentWorld->aAdjoins] & 0x80) != 0 )
            {
                pFrustum = rdCamera_g_pCurCamera->pFrustum;
                pFrustum->orthoLeftPlane   = 0.0f;
                pFrustum->orthoTopPlane    = 0.0f;
                pFrustum->orthoRightPlane  = (float)width;
                pFrustum->orthoBottomPlane = (float)height;
            }

            SithSurface* pSurface = pAdjoin->pAdjoinSurface;

            rdVector3 lookDir;
            rdVector_Sub3(&lookDir, &sithCamera_g_pCurCamera->lookPos, &sithWorld_g_pCurrentWorld->aVertices[*pSurface->face.aVertices]);

            if ( rdVector_Dot3(&pSurface->face.normal, &lookDir) > 0.0f ) // If surface facing camera
            {
                sithRender_BuildVisibleSurface(pSurface);

                sithRender_clipFaceView.aVertices = sithRender_aClipVertices;
                sithRender_faceView.numVertices   = pSurface->face.numVertices;
                sithRender_faceView.aVertIdxs     = pSurface->face.aVertices;

                rdClip_ClipFacePVS(pFrustum, &sithRender_faceView, &sithRender_clipFaceView);
                //numVertices = sithRender_clipFaceView.numVertices;

                if ( (sithRender_clipFaceView.numVertices || (rdClip_g_faceStatus & 0x01) != 0)
                    && (!pSurface->face.pMaterial || pSurface->face.pMaterial->formatType != STDCOLOR_FORMAT_RGB
                        || (pSurface->face.flags & RD_FF_TEX_TRANSLUCENT) != 0
                        || pSurface->face.geometryMode == RD_GEOMETRY_NONE)
                    && (pAdjoin->flags & SITH_ADJOIN_VISIBLE) != 0 )
                {
                    // Project vertices to camera space
                    rdCamera_g_pCurCamera->pfProjectList(sithRender_aTransformedClipVertices, sithRender_aClipVertices, sithRender_clipFaceView.numVertices);

                    SithSector* pAdjoinSector = pAdjoin->pAdjoinSector;
                    if ( pAdjoinSector->renderTick == sithMain_g_curRenderTick ) // If sector was already build at current frame then  build only it's clip frustum
                    {
                        if ( (rdClip_g_faceStatus & 0x01) != 0 )
                        {
                            sithRender_BuildClipFrustrum(
                                pAdjoinSector->pClipFrustum,
                                sithRender_clipFaceView.numVertices,
                                pFrustum->orthoLeftPlane,
                                pFrustum->orthoTopPlane,
                                pFrustum->orthoRightPlane,
                                pFrustum->orthoBottomPlane
                            );
                        }
                        else
                        {
                            sithRender_BuildClipFrustrum(
                                pAdjoinSector->pClipFrustum,
                                sithRender_clipFaceView.numVertices,
                                pAdjoinSector->pClipFrustum->orthoLeftPlane,
                                pAdjoinSector->pClipFrustum->orthoTopPlane,
                                pAdjoinSector->pClipFrustum->orthoRightPlane,
                                pAdjoinSector->pClipFrustum->orthoBottomPlane
                            );
                        }
                    }
                    else
                    {
                        rdClipFrustum frustum;
                        if ( (rdClip_g_faceStatus & 0x01) != 0 )
                        {
                            pAdjoinSector->pClipFrustum = pFrustum;
                        }
                        else
                        {
                            sithRender_BuildClipFrustrum(&frustum, sithRender_clipFaceView.numVertices, FLT_MAX, FLT_MAX, -FLT_MAX, -FLT_MAX); // OG FLT_MAX was defined as 3.4028235e38f;
                            pAdjoinSector->pClipFrustum = &frustum;
                        }

                        sithRender_BuildVisibleSectorPVS(pAdjoinSector);
                    }
                }
            }

            pAdjoin = pAdjoin->pNextVisibleAdjoin;
        }

        if ( !sithRender_aVisibleAdjoins[sithRender_curPVSIndex] )
        {
            ++sithRender_curPVSIndex;
        }
    }
}

void J3DAPI sithRender_BuildVisibleSectorPVS(SithSector* pSector)
{
    // Update PVS stuff
    for ( SithSurfaceAdjoin* pAdjoin = pSector->pFirstAdjoin; pAdjoin; pAdjoin = pAdjoin->pNextAdjoin )
    {
        size_t adjIdx = pAdjoin - sithWorld_g_pCurrentWorld->aAdjoins;
        if ( (pAdjoin->flags & SITH_ADJOIN_VISIBLE) != 0 && sithRender_aAdjoinTable[adjIdx] )
        {
            size_t idx = sithRender_aAdjoinTable[adjIdx] & ~0xFFFFFF80;
            if ( idx < sithRender_curPVSIndex )
            {
                idx = sithRender_curPVSIndex;
            }

            pAdjoin->pNextVisibleAdjoin = sithRender_aVisibleAdjoins[idx];
            sithRender_aVisibleAdjoins[idx] = pAdjoin;
            if ( sithRender_lastPVSIndex <= idx )
            {
                sithRender_lastPVSIndex = idx;
            }
        }
    }

    // Now build sector
    sithRender_BuildVisibleSector(pSector, pSector->pClipFrustum);
}

void J3DAPI sithRender_BuildVisibleSector(SithSector* pSector, const rdClipFrustum* pFrustrum)
{
    pSector->renderTick = sithMain_g_curRenderTick;
    if ( sithRender_g_numVisibleSectors >= STD_ARRAYLEN(sithRender_aVisibleSectors) )
    {
        RDLOG_WARNING("Max visible sectors exceeded!\n");
        return;
    }

    sithRender_aVisibleSectors[sithRender_g_numVisibleSectors++] = pSector;

    if ( (pSector->flags & SITH_SECTOR_SEEN) == 0 )
    {
        pSector->flags |= SITH_SECTOR_SEEN;
        if ( (pSector->flags & SITH_SECTOR_COGLINKED) != 0 )
        {
            // Notify sector's cog
            sithCog_SectorSendMessage(pSector, NULL, SITHCOG_MSG_SIGHTED);
        }
    }

    // Assign sectors frustum
    sithRender_aSectorFrustrums[sithRender_numSectorFrustums] = *pFrustrum;
    pSector->pClipFrustum = &sithRender_aSectorFrustrums[sithRender_numSectorFrustums++];

    // Collect thing lights (ambient spot light & actor head light)
    for ( SithThing* pThing = pSector->pFirstThingInSector; pThing && sithRender_numThingLights < STD_ARRAYLEN(sithRender_aThingLights); pThing = pThing->pNextThingInSector )
    {
        if ( (pThing->flags & SITH_TF_EMITLIGHT) != 0 && (pThing->flags & (SITH_TF_DISABLED | SITH_TF_INVISIBLE | SITH_TF_DESTROYED)) == 0 )
        {
            // Collect Thing's light if light range is > 0.01f
            if ( !rdVector_IsZero3((rdVector3*)&pThing->light.color)
                && SITHRENDER_ISDYNAMICLIGHT(pThing->light.color.alpha) )
            {
                sithRender_aThingLights[sithRender_numThingLights].color     = pThing->light.color;
                sithRender_aThingLights[sithRender_numThingLights].minRadius = pThing->light.minRadius;
                sithRender_aThingLights[sithRender_numThingLights].maxRadius = pThing->light.maxRadius;

                rdCamera_AddLight(rdCamera_g_pCurCamera, &sithRender_aThingLights[sithRender_numThingLights], &pThing->pos);
                ++sithRender_numThingLights;
            }

            // Collect actor's head light
            if ( (pThing->type == SITH_THING_ACTOR || pThing->type == SITH_THING_PLAYER)
                && sithRender_numThingLights < STD_ARRAYLEN(sithRender_aThingLights)
                && (pThing->thingInfo.actorInfo.flags & SITH_AF_HEADLIGHT) != 0
                && !rdVector_IsZero3((rdVector3*)&pThing->thingInfo.actorInfo.headLightIntensity) )
            {
                sithRender_aThingLights[sithRender_numThingLights].color     = pThing->thingInfo.actorInfo.headLightIntensity;
                sithRender_aThingLights[sithRender_numThingLights].minRadius = pThing->thingInfo.actorInfo.headLightIntensity.alpha;
                sithRender_aThingLights[sithRender_numThingLights].maxRadius = pThing->thingInfo.actorInfo.headLightIntensity.alpha;

                rdVector3 lightPos;
                rdMatrix_TransformVector34(&lightPos, &pThing->thingInfo.actorInfo.lightOffset, &pThing->orient); // Fixed: Replaced clall to rdMatrix_TransformPoint34 with rdMatrix_TransformVector34
                rdVector_Add3Acc(&lightPos, &pThing->pos);                                                        //        Fixes light flickering as pThing->orient.dvec might not be zero

                rdCamera_AddLight(rdCamera_g_pCurCamera, &sithRender_aThingLights[sithRender_numThingLights], &lightPos);
                ++sithRender_numThingLights;
            }
        }
    }

    sithRender_aVisibleThingSectors[sithRender_numVisibleThingSectors++] = pSector;
}

#ifdef J3D_OPENGL

static void sithRender_RenderSectorsStatic(void)
{
    sithRender_g_numArchPolys = 0;

    rdFaceFlags extraFaceFlags = 0;
    if ( sithWorld_g_pCurrentWorld->fog.bEnabled )
    {
        extraFaceFlags = RD_FF_FOG_ENABLED;
    }

    for ( size_t secNum = 0; secNum < sithRender_g_numVisibleSectors; ++secNum )
    {
        SithSector* pSector = sithRender_aVisibleSectors[secNum];
        for ( size_t surfNum = 0; surfNum < pSector->numSurfaces; ++surfNum )
        {
            SithSurface* pSurf = &pSector->pFirstSurface[surfNum];
            if ( pSurf->face.geometryMode == RD_GEOMETRY_NONE )
            {
                continue;
            }

            rdVector3 camDir;
            rdVector_Sub3(&camDir, &sithCamera_g_pCurCamera->lookPos, &sithWorld_g_pCurrentWorld->aVertices[*pSurf->face.aVertices]);
            if ( rdVector_Dot3(&pSurf->face.normal, &camDir) <= 0.0f ) // If surface is not facing camera
            {
                continue;
            }

            if ( pSurf->pAdjoin && (pSurf->face.flags & RD_FF_TEX_TRANSLUCENT) != 0 && pSurf->face.pMaterial->formatType == STDCOLOR_FORMAT_RGBA )
            {
                if ( sithRender_numAlphaAdjoins < STD_ARRAYLEN(sithRender_aAlphaAdjoins) )
                {
                    sithRender_aAlphaAdjoins[sithRender_numAlphaAdjoins++] = pSurf;
                }
                // Maybe add log when alpha adjoin couldn't be processed
            }
            else
            {
                rdPayload* pPayload         = rdCache_GetOpaqueDrawCall(RD_DRAW_GEOMETRY);
                rdGeoFacePayload* pDrawCall = &pPayload->geoFacePayload;
                rdVector4 extraLight        = pSurf->face.extraLight;
                rdVector_Add4Acc(&extraLight, &pSurf->pSector->extraLight);
                extraLight.alpha       = 1.0f;
                pPayload->lightingMode = pSurf->face.lightingMode;
                pDrawCall->faceNum     = pSurf->face.num;
                pPayload->flags        = pSurf->face.flags;
                pPayload->pMaterial    = pSurf->face.pMaterial;
                pPayload->matCelNum    = pSurf->face.matCelNum;


                if ( (pSurf->flags & (SITH_SURFACE_CEILINGSKY | SITH_SURFACE_HORIZONSKY)) != 0 )
                {
                    rdVector_Set4(&extraLight, 1.0f, 1.0f, 1.0f, 1.0f);

                    if ( (pSurf->flags & SITH_SURFACE_CEILINGSKY) != 0 )
                    {
                        pPayload->pShader = stdShader_GetShader("std_ceilingSky");
                    }
                    else
                    {
                        pPayload->pShader = stdShader_GetShader("std_horizonSky");
                    }
                }

                if ( pPayload->lightingMode >= sithRender_lightMode )
                {
                    pPayload->lightingMode = sithRender_lightMode;
                }
                pPayload->extraLight = extraLight;
                pPayload->flags |= extraFaceFlags;
                rdCache_AddOpaqueDrawCall();
                ++sithRender_g_numArchPolys;
            }
        }

        ++sithRender_numRenderedSectors;
    }
    rdCache_Flush();
}
#endif


void sithRender_RenderSectors(void)
{
#ifdef J3D_OPENGL
    if ( !std3D_g_bUseLegacyRendering )
    {
        sithRender_RenderSectorsStatic();
        return;
    }
#endif

    sithRender_g_numArchPolys = 0;

    rdFaceFlags extraFaceFlags = 0;
    if ( sithWorld_g_pCurrentWorld->fog.bEnabled )
    {
        extraFaceFlags = RD_FF_FOG_ENABLED;
    }

    sithRender_faceView.aVertices    = sithWorld_g_pCurrentWorld->aTransformedVertices;
    sithRender_faceView.aTexVertices = sithWorld_g_pCurrentWorld->aTexVerticies;
    sithRender_faceView.aVertLights  = sithWorld_g_pCurrentWorld->aVertDynamicLights;

    for ( size_t secNum = 0; secNum < sithRender_g_numVisibleSectors; ++secNum )
    {
        SithSector* pSector = sithRender_aVisibleSectors[secNum];
        for ( size_t surfNum = 0; surfNum < pSector->numSurfaces; ++surfNum )
        {
            SithSurface* pSurf = &pSector->pFirstSurface[surfNum];
            if ( pSurf->face.geometryMode == RD_GEOMETRY_NONE )
            {
                continue;
            }

            rdVector3 camDir;
            rdVector_Sub3(&camDir, &sithCamera_g_pCurCamera->lookPos, &sithWorld_g_pCurrentWorld->aVertices[*pSurf->face.aVertices]);
            if ( rdVector_Dot3(&pSurf->face.normal, &camDir) <= 0.0f ) // If surface is not facing camera
            {
                continue;
            }

            if ( pSurf->pAdjoin && (pSurf->face.flags & RD_FF_TEX_TRANSLUCENT) != 0 )
            {
                if ( sithRender_numAlphaAdjoins < STD_ARRAYLEN(sithRender_aAlphaAdjoins) )
                {
                    sithRender_aAlphaAdjoins[sithRender_numAlphaAdjoins++] = pSurf;
                }
                // Maybe add log when alpha adjoin couldn't be processed
            }
            else
            {
                // Transform vertices to view space (camera space)
                if ( pSurf->renderTick != sithMain_g_curRenderTick )
                {
                    for ( size_t i = 0; i < pSurf->face.numVertices; ++i )
                    {
                        int vertIdx = pSurf->face.aVertices[i];
                        if ( sithWorld_g_pCurrentWorld->aVertexRenderTickIds[vertIdx] != sithMain_g_curRenderTick )
                        {
                            rdMatrix_TransformPoint34(
                                &sithWorld_g_pCurrentWorld->aTransformedVertices[vertIdx],
                                &sithWorld_g_pCurrentWorld->aVertices[vertIdx],
                                &rdCamera_g_pCurCamera->viewMatrix
                            );
                            sithWorld_g_pCurrentWorld->aVertexRenderTickIds[vertIdx] = sithMain_g_curRenderTick;
                        }
                    }

                    pSurf->renderTick = sithMain_g_curRenderTick;
                }

                rdCacheProcEntry* pPoly = rdCache_GetProcEntry();
                if ( !pPoly )
                {
                    // TODO: Add maybe log
                    continue;
                }

                // Render sky surface
                if ( (pSurf->flags & (SITH_SURFACE_CEILINGSKY | SITH_SURFACE_HORIZONSKY)) != 0 )
                {
                    // Clip vertices and transform to camera space
                    sithRender_clipFaceView.aVertices = sithRender_aClipVertices;
                    sithRender_faceView.numVertices   = pSurf->face.numVertices;
                    sithRender_faceView.aVertIdxs     = pSurf->face.aVertices;

                    rdClip_QClipFaceW(pSector->pClipFrustum, &sithRender_faceView, &sithRender_clipFaceView);
                    if ( sithRender_clipFaceView.numVertices < 3 )
                    {
                        continue;
                    }
                    // There are enough clipped vertices to render n-gon,
                    // first let's project them to NDC space
                    rdCamera_g_pCurCamera->pfProjectList(sithRender_aSurfaceTransformedVertices, sithRender_aClipVertices, sithRender_clipFaceView.numVertices);

                    pPoly->flags     = pSurf->face.flags;
                    pPoly->pMaterial = pSurf->face.pMaterial;

                    // Now make sky poly from transformed vertices
                    if ( (pSurf->flags & SITH_SURFACE_HORIZONSKY) != 0 )
                    {
                        sithRenderSky_HorizonFaceToPlane(pPoly, &pSurf->face, sithRender_aSurfaceTransformedVertices, sithRender_clipFaceView.numVertices);
                    }
                    else if ( (pSurf->flags & SITH_SURFACE_CEILINGSKY) != 0 )
                    {
                        sithRenderSky_CeilingFaceToPlane(pPoly, &pSurf->face, sithRender_aClipVertices, sithRender_aSurfaceTransformedVertices, sithRender_clipFaceView.numVertices);
                    }

                    pPoly->matCelNum = pSurf->face.matCelNum;
                    rdCache_AddProcFace(sithRender_clipFaceView.numVertices);

                    ++sithRender_g_numArchPolys;
                }
                else // Not a sky surface
                {
                    // Clip vertices and transform to NDC screen space
                    if ( !rdClip_FaceToPlane(pSector->pClipFrustum, pPoly, &pSurf->face, sithWorld_g_pCurrentWorld->aTransformedVertices, sithWorld_g_pCurrentWorld->aTexVerticies,
                                             sithWorld_g_pCurrentWorld->aVertDynamicLights, pSurf->aIntensities) )
                    {
                        // Face is fully outside frustrum
                        continue;
                    }

                    pPoly->lightingMode = pSurf->face.lightingMode;
                    if ( pPoly->lightingMode >= sithRender_lightMode )
                    {
                        pPoly->lightingMode = sithRender_lightMode;
                    }

                    // TODO: Clamp maybe vector to 0.0f - 1.0f?
                    rdVector_Add4(&pPoly->extraLight, &pSurf->face.extraLight, &pSector->extraLight);

                    // TODO: Also camera ambient light is not set (rdCamera_SetAmbientLight), but the alpha adjoin surfaces has ambient light set to sed.extraLight + sec.ambientLight

                    pPoly->flags = pSurf->face.flags;
                    if ( (pSurf->flags & SITH_SURFACE_CEILINGSKY) != 0 ) // TODO: ???
                    {
                        sithRenderSky_CeilingFaceToPlane(pPoly, &pSurf->face, sithRender_aClipVertices, sithRender_aSurfaceTransformedVertices, pSurf->face.numVertices);
                    }
                    else
                    {
                        pPoly->flags |= extraFaceFlags;
                    }

                    pPoly->pMaterial = pSurf->face.pMaterial;
                    pPoly->matCelNum = pSurf->face.matCelNum;
                    rdCache_AddProcFace(pSurf->face.numVertices);

                    ++sithRender_g_numArchPolys;
                }
            }
        }

        ++sithRender_numRenderedSectors;
    }

    rdCache_Flush();
}

void sithRender_BuildVisibleThingSectorList(void)
{
    // Altered: Added new BFS traversal option
    switch ( sithRender_culledSectorTraversalMode )
    {
        case SITHRENDER_THING_TRAVERSAL_BFS:
            sithRender_BuildVisibleThingSectorListBFS();
            break;

        case SITHRENDER_THING_TRAVERSAL_LDFS:
        default: // Legacy custom DFS alog
            sithRender_BuildVisibleThingSectorListDFS();
            break;
    };
}

void sithRender_BuildVisibleThingSectorListDFS(void)
{
    // Function builds a list of sectors with visible things (i.e. thing that should be rendered) and their lights.
    //
    // Algorithm: vanilla DFS-like recursive sector traversal
    //
    // Iterates through previously built visible sector list and recursively traverses adjoins of each visible sector.
    // Sectors are marked as processed on first encounter using a frame tick,
    // which prevents revisiting them through alternative paths.
    //
    // Traversal order depends on adjoin order and accumulated path distance.
    // Sector traversal is distance-limited and may miss sectors that are
    // reachable via shorter or different paths later in the traversal.
    //
    // This behavior is kept for legacy compatibility.

    for ( size_t i = 0; i < sithRender_g_numVisibleSectors; ++i )
    {
        SithSector* pSector = sithRender_aVisibleSectors[i];
        for ( SithSurfaceAdjoin* pAdjoin = pSector->pFirstAdjoin; pAdjoin; pAdjoin = pAdjoin->pNextAdjoin )
        {
            // Legacy mode prevent crwaling through already processed sector
            if ( pAdjoin->pAdjoinSector->renderTick != sithMain_g_curRenderTick && (pAdjoin->flags & SITH_ADJOIN_VISIBLE) != 0 )
            {
                pAdjoin->pAdjoinSector->pClipFrustum = pSector->pClipFrustum;
                float distance = pAdjoin->distance + pAdjoin->pMirrorAdjoin->distance;
                sithRender_CollectVisibleThingSector(pAdjoin->pAdjoinSector, 0.0f, distance);
            }
        }
    }
}

void J3DAPI sithRender_CollectVisibleThingSector(SithSector* pSector, float curDistance, float startDistance)
{
    // Recursive legacy DFS-like visible thing sector collection function

    if ( pSector->renderTick != sithMain_g_curRenderTick )
    {
        pSector->renderTick = sithMain_g_curRenderTick;

        // Added: Added check for new light collection distance
        if ( curDistance < sithRender_maxLightCollectDistance )
        {
            for ( SithThing* pThing = pSector->pFirstThingInSector; pThing; pThing = pThing->pNextThingInSector )
            {
                // Altered: Replaced inplace code with the new function
                sithRender_CollectThingLights(pThing);
            }
        }

        if ( curDistance < sithRender_maxThingCollectDistance ) // Altered: Replaced with new max thing collection distance
        {
            ++sithRender_totalVisibleThingSectors;
            if ( sithRender_numVisibleThingSectors < SITHRENDER_MAX_VISIBLE_THING_SECTORS )
            {
                sithRender_aVisibleThingSectors[sithRender_numVisibleThingSectors++] = pSector;
            }
        }

        for ( SithSurfaceAdjoin* pAdjoin = pSector->pFirstAdjoin; pAdjoin; pAdjoin = pAdjoin->pNextAdjoin )
        {
            if ( (pAdjoin->flags & SITH_ADJOIN_VISIBLE) != 0 && pAdjoin->pAdjoinSector->renderTick != sithMain_g_curRenderTick )
            {
                float distance = startDistance + curDistance + pAdjoin->distance + pAdjoin->pMirrorAdjoin->distance;
                if ( distance < J3DMAX(sithRender_maxThingCollectDistance, sithRender_maxLightCollectDistance) ) // Altered: Replaced with new max collection distance
                {
                    pAdjoin->pAdjoinSector->pClipFrustum = pSector->pClipFrustum;
                    sithRender_CollectVisibleThingSector(pAdjoin->pAdjoinSector, distance, 0.0f);
                }
            }
        }
    }
}

void sithRender_BuildVisibleThingSectorListBFS(void)
{
    // Function builds a list of sectors with visible things (i.e. thing that should be rendered) and their lights.
    //
    // Algorithm: Breadth-first sector traversal (BFS)
    //
    // Iterates through previously built visible sector list and traverses all adjoins of each sector level-by-level
    // using a queue. Multiple paths may reach the same sector, but traversal
    // is bounded by configured collection distances.
    //
    // Ensures all reachable sectors within range are processed regardless of
    // traversal order, preventing missed thing or dynamic light collection
    // caused by path-order dependency like in case of sithRender_BuildVisibleThingSectorListDFS function.

    sithRender_visitedSectorQueue.head = 0;
    sithRender_visitedSectorQueue.tail = 0;

    float maxDistance = J3DMAX(sithRender_maxThingCollectDistance, sithRender_maxLightCollectDistance);

    // Initialize queue with already collected visible sectors
    for ( size_t i = 0; i < sithRender_g_numVisibleSectors; ++i )
    {
        SithSector* pSector = sithRender_aVisibleSectors[i];
        for ( SithSurfaceAdjoin* pAdjoin = pSector->pFirstAdjoin; pAdjoin; pAdjoin = pAdjoin->pNextAdjoin )
        {
            if ( (pAdjoin->flags & SITH_ADJOIN_VISIBLE) != 0 )
            {
                float distance = pAdjoin->distance + pAdjoin->pMirrorAdjoin->distance;
                if ( distance < maxDistance && sithRender_visitedSectorQueue.tail < STD_ARRAYLEN(sithRender_visitedSectorQueue.aEntries) )
                {
                    SithSector* pNextSector   = pAdjoin->pAdjoinSector;
                    pNextSector->pClipFrustum = pSector->pClipFrustum;
                    sithRender_visitedSectorQueue.aEntries[sithRender_visitedSectorQueue.tail++] = (SithRenderSectorQueueEntry){
                          .pSector  = pNextSector,
                          .distance = distance
                    };
                }
            }
        }
    }

    // Process queue
    while ( sithRender_visitedSectorQueue.head < sithRender_visitedSectorQueue.tail )
    {
        SithRenderSectorQueueEntry* pEntry = &sithRender_visitedSectorQueue.aEntries[sithRender_visitedSectorQueue.head++];
        SithSector* pCurSector = pEntry->pSector;
        float curDistance      = pEntry->distance;

        // Collect things and lights only once per sector
        if ( pCurSector->renderTick != sithMain_g_curRenderTick )
        {
            pCurSector->renderTick = sithMain_g_curRenderTick;

            // Collect lights
            if ( curDistance < sithRender_maxLightCollectDistance )
            {
                for ( SithThing* pThing = pCurSector->pFirstThingInSector; pThing; pThing = pThing->pNextThingInSector )
                {
                    sithRender_CollectThingLights(pThing);
                }
            }

            // Collect things
            if ( curDistance < sithRender_maxThingCollectDistance )
            {
                ++sithRender_totalVisibleThingSectors;
                if ( sithRender_numVisibleThingSectors < SITHRENDER_MAX_VISIBLE_THING_SECTORS )
                {
                    sithRender_aVisibleThingSectors[sithRender_numVisibleThingSectors++] = pCurSector;
                }
            }

            // Add adjoining sectors to queue
            for ( SithSurfaceAdjoin* pAdjoin = pCurSector->pFirstAdjoin; pAdjoin; pAdjoin = pAdjoin->pNextAdjoin )
            {
                // Only add if adjoin is visible and it's sector is not already processed
                SithSector* pNextSector = pAdjoin->pAdjoinSector;
                if ( (pAdjoin->flags & SITH_ADJOIN_VISIBLE) != 0 && pNextSector->renderTick != sithMain_g_curRenderTick )
                {
                    float distance = curDistance + pAdjoin->distance + pAdjoin->pMirrorAdjoin->distance;
                    if ( distance < maxDistance && sithRender_visitedSectorQueue.tail < STD_ARRAYLEN(sithRender_visitedSectorQueue.aEntries) )
                    {
                        pNextSector->pClipFrustum = pCurSector->pClipFrustum;
                        sithRender_visitedSectorQueue.aEntries[sithRender_visitedSectorQueue.tail++] = (SithRenderSectorQueueEntry){
                           .pSector  = pNextSector,
                           .distance = distance
                        };
                    }
                }
            }
        }
    }
}

void sithRender_CollectThingLights(const SithThing* pThing)
{
    // Collect thing spot light & actor head light

    if ( sithRender_numThingLights < STD_ARRAYLEN(sithRender_aThingLights)
        && (pThing->flags & SITH_TF_EMITLIGHT) != 0
        && (pThing->flags & (SITH_TF_DISABLED | SITH_TF_INVISIBLE | SITH_TF_DESTROYED)) == 0 ) // Fixed: Added check for SITH_TF_INVISIBLE flag,
                                                                                               //        OG this flag was checked only in sithRender_BuildVisibleSector
                                                                                               //        but not in case of sithRender_BuildVisibleThingSector
    {
        // Collect thing light if range is > 0.01f
        if ( !rdVector_IsZero3((const rdVector3*)&pThing->light.color)
            && SITHRENDER_ISDYNAMICLIGHT(pThing->light.color.alpha) )
        {
            sithRender_aThingLights[sithRender_numThingLights].color     = pThing->light.color;
            sithRender_aThingLights[sithRender_numThingLights].minRadius = pThing->light.minRadius;
            sithRender_aThingLights[sithRender_numThingLights].maxRadius = pThing->light.maxRadius;

            rdCamera_AddLight(rdCamera_g_pCurCamera, &sithRender_aThingLights[sithRender_numThingLights], &pThing->pos);
            ++sithRender_numThingLights;
        }

        // Collect actor head light
        if ( (pThing->type == SITH_THING_ACTOR || pThing->type == SITH_THING_PLAYER)
            && sithRender_numThingLights < STD_ARRAYLEN(sithRender_aThingLights)
            && (pThing->thingInfo.actorInfo.flags & SITH_AF_HEADLIGHT) != 0
            && !rdVector_IsZero3((const rdVector3*)&pThing->thingInfo.actorInfo.headLightIntensity) )
        {
            sithRender_aThingLights[sithRender_numThingLights].color     = pThing->thingInfo.actorInfo.headLightIntensity;
            sithRender_aThingLights[sithRender_numThingLights].minRadius = pThing->thingInfo.actorInfo.headLightIntensity.alpha; // Fixed: Replaced assigning pThing->light.minRadius with headLightIntensity.alpha to match code in sithRender_BuildVisibleSector
            sithRender_aThingLights[sithRender_numThingLights].maxRadius = pThing->thingInfo.actorInfo.headLightIntensity.alpha; // Fixed: Replaced assigning pThing->light.maxRadius with headLightIntensity.alpha to match code in sithRender_BuildVisibleSector

            rdVector3 lightPos;
            rdMatrix_TransformVector34(&lightPos, &pThing->thingInfo.actorInfo.lightOffset, &pThing->orient); // Fixed: Replaced clall to rdMatrix_TransformPoint34 with rdMatrix_TransformVector34
            rdVector_Add3Acc(&lightPos, &pThing->pos);                                                        //        Fixes light flickering as pThing->orient.dvec might not be zero

            rdCamera_AddLight(rdCamera_g_pCurCamera, &sithRender_aThingLights[sithRender_numThingLights], &lightPos);
            ++sithRender_numThingLights;
        }
    }
}

void sithRender_BuildDynamicLights(void)
{
#ifdef J3D_OPENGL
    if ( !std3D_g_bUseLegacyRendering )
    {
        stdShader_SetShaderLights();
        return;
    }
#endif
    // Function calculates vertex intensities from collected dynamic lights for all visible sectors

    rdVector3* aVertices             = sithWorld_g_pCurrentWorld->aVertices;
    rdVector4* aVertDynamicLights    = sithWorld_g_pCurrentWorld->aVertDynamicLights;
    uint32_t* aVertLightsRenderTicks = sithWorld_g_pCurrentWorld->aVertDynamicLightsRenderTicks;
    float attenuationMax             = rdCamera_g_pCurCamera->attenuationMax;

    rdLight* aLights[RDCAMERA_MAX_LIGHTS];

    for ( size_t secNum = 0; secNum < sithRender_g_numVisibleSectors; ++secNum )
    {
        SithSector* pSector = sithRender_aVisibleSectors[secNum];
        size_t numLights    = 0;

        for ( size_t lightNum = 0; lightNum < rdCamera_g_pCurCamera->numLights; ++lightNum )
        {
            rdVector3 lightDir;
            rdVector_Sub3(&lightDir, &rdCamera_g_pCurCamera->aLightPositions[lightNum], &pSector->center);

            rdLight* pLight  = rdCamera_g_pCurCamera->aLights[lightNum];
            if ( (pSector->radius + pLight->minRadius) > rdVector_Len3(&lightDir) )
            {
                aLights[numLights++] = pLight;
            }
        }

        for ( size_t i = 0; i < pSector->numVertices; ++i )
        {
            int vertIdx = pSector->aVertIdxs[i];
            if ( aVertLightsRenderTicks[vertIdx] != sithMain_g_curRenderTick )
            {
                // Set default light color to black
                rdVector_Set4(&aVertDynamicLights[vertIdx], 0.0f, 0.0f, 0.0f, 0.0f);

                for ( size_t lightNum = 0; lightNum < numLights; ++lightNum )
                {
                    rdLight* pLight = aLights[lightNum];

                    rdVector3 lightDir;
                    rdVector_Sub3(&lightDir, &rdCamera_g_pCurCamera->aLightPositions[pLight->num], &aVertices[vertIdx]);

                    float dist = rdVector_Len3(&lightDir);
                    if ( dist < (double)pLight->maxRadius )
                    {
                        float att = dist * attenuationMax;
                        aVertDynamicLights[vertIdx].red   += pLight->color.red - att;
                        aVertDynamicLights[vertIdx].green += pLight->color.green - att;
                        aVertDynamicLights[vertIdx].blue  += pLight->color.blue - att;
                    }

                    rdMath_ClampVector3Acc((rdVector3*)&aVertDynamicLights[vertIdx], 0.0f, 1.0f);
                    // TODO: alpha value is not set
                }

                aVertLightsRenderTicks[vertIdx] = sithMain_g_curRenderTick;
            }
        }
    }
}

void sithRender_RenderThings(void)
{
    bool bLightSet = false;
    sithRender_g_numThingPolys     = 0;
    sithRender_g_numAlphaThingPoly = 0;

    rdModel3_EnableFogRendering(sithWorld_g_pCurrentWorld->fog.bEnabled);

    for ( size_t i = 0; i < sithRender_numVisibleThingSectors; ++i )
    {
        SithSector* pSector = sithRender_aVisibleThingSectors[i];

        rdVector4 sectorAmbientLight;
        rdVector_Add4(&sectorAmbientLight, &pSector->ambientLight, &pSector->extraLight);
        rdMath_ClampVector4Acc(&sectorAmbientLight, 0.0f, 1.0f);
        sectorAmbientLight.alpha = 0.0f;

        for ( SithThing* pCurThing = pSector->pFirstThingInSector; pCurThing; pCurThing = pCurThing->pNextThingInSector )
        {
            if ( (pCurThing->flags & (SITH_TF_DISABLED | SITH_TF_INVISIBLE | SITH_TF_DESTROYED)) == 0
                && ((sithCamera_g_pCurCamera->type & (SITHCAMERA_ORBITAL | SITHCAMERA_UNKNOWN_40 | SITHCAMERA_IDLE | SITHCAMERA_UNKNOWN_10 | SITHCAMERA_CINEMATIC | SITHCAMERA_EXTERNAL)) != 0
                    || pCurThing != sithCamera_g_pCurCamera->pPrimaryFocusThing) )
            {
                // Transform thing pos to view (camera) space
                rdMatrix_TransformPoint34(&pCurThing->cameraSpacePos, &pCurThing->pos, &rdCamera_g_pCurCamera->viewMatrix);

                rdThing* prdThing = &pCurThing->renderData;
                float radius = 0.0f; // Added: Init to 0;

                switch ( pCurThing->renderData.type )
                {
                    case RD_THING_MODEL3:
                    {
                        if ( pCurThing->collide.movesize >= pCurThing->collide.size )
                        {
                            radius = pCurThing->collide.movesize;
                        }
                        else
                        {
                            radius = pCurThing->collide.size;
                        }

                        if ( prdThing->data.pModel3->size >= radius )
                        {
                            radius = prdThing->data.pModel3->size;
                        }

                        // Set sector point light
                        //
                        // TODO [bug]: sithRender_numSectorPointLights is never incremented, but changing this breaks thin illumination as the light will affect also things from other sectors
                        //             Maybe just a temp var for light can be used instead and we can then increase limitation of max thing light?
                        sithRender_aSectorPointLights[sithRender_numSectorPointLights].color     = pSector->light.color;
                        sithRender_aSectorPointLights[sithRender_numSectorPointLights].minRadius = pSector->light.minRadius;
                        sithRender_aSectorPointLights[sithRender_numSectorPointLights].maxRadius = pSector->light.maxRadius;

                        rdCamera_AddLight(rdCamera_g_pCurCamera, &sithRender_aSectorPointLights[sithRender_numSectorPointLights], &pSector->light.pos);
                        bLightSet = true;
                    }break;

                    case RD_THING_SPRITE3:
                        radius = prdThing->data.pSprite3->radius;
                        ++sithRender_numSpritesToDraw;
                        break;

                    case RD_THING_PARTICLE:
                        radius = prdThing->data.pParticle->radius;
                        break;

                    case RD_THING_POLYLINE:
                        radius = prdThing->data.pPolyline->length;
                        break;

                    default:
                        break;
                }

                bool bRender = false;
                if ( prdThing->type == RD_THING_POLYLINE )
                {
                    bRender = true;
                }
                else
                {
                    // Render only if thing is inside frustum or intersects with frustum
                    prdThing->frustumCull = rdClip_SphereInFrustrum(pSector->pClipFrustum, &pCurThing->cameraSpacePos, radius);
                    if ( prdThing->frustumCull != RDFRUSTUMCULL_OUTSIDE )
                    {
                        bRender = true;
                    }
                }

                if ( bRender )
                {
                    // Collect thing flat light
                    if ( (pCurThing->flags & SITH_TF_EMITLIGHT) != 0
                        && !rdVector_IsZero3((rdVector3*)&pCurThing->light.color)
                        && SITHRENDER_ISFLATLIGHT(pCurThing->light.color.alpha) )// if light range is <= 0.01f
                    {
                        rdVector4 ambientLight;
                        rdVector_Add4(&ambientLight, &sectorAmbientLight, &pCurThing->light.color);
                        rdMath_ClampVector4Acc(&ambientLight, 0.0f, 1.0f);
                        ambientLight.alpha = sectorAmbientLight.alpha;

                        rdCamera_SetAmbientLight(rdCamera_g_pCurCamera, &ambientLight);
                    }
                    else
                    {
                        rdCamera_SetAmbientLight(rdCamera_g_pCurCamera, &sectorAmbientLight);
                    }

                    // Now draw thing
                    if ( sithRender_RenderThing(pCurThing) )
                    {
                        ++sithRender_g_numDrawnThings;
                    }
                }

                if ( bLightSet == 1 )
                {
                    --rdCamera_g_pCurCamera->numLights;
                    bLightSet = false;
                }
            }
        }
    }

    rdModel3_EnableFogRendering(0);
    rdCache_Flush();
}

int J3DAPI sithRender_RenderThing(SithThing* pThing)
{
    RD_ASSERTREL(pThing != NULL); // TODO: Why using RD_ASSERTREL

    if ( (pThing->flags & SITH_TF_SEEN) == 0 )
    {
        if ( (pThing->flags & SITH_TF_COGLINKED) != 0 )
        {
            sithCog_ThingSendMessage(pThing, 0, SITHCOG_MSG_SIGHTED);
        }

        if ( pThing->controlType == SITH_CT_AI && pThing->controlInfo.aiControl.pLocal )
        {
            pThing->controlInfo.aiControl.pLocal->mode &= ~SITHAI_MODE_SLEEPING;
        }

        pThing->flags |= SITH_TF_SEEN;
    }

    // Set thing cur render frame num
    pThing->renderFrame = sithMain_g_frameNumber;

    // Now draw thing
    pThing->orient.dvec = pThing->pos;
    int drawResult = sithThing_Draw(pThing);
    if ( (pThing->flags & SITH_TF_SHADOW) != 0 ) // draw shadow
    {
        sithShadow_RenderThingShadow(pThing);
    }

    sithRender_g_numThingPolys     += rdModel3_g_numDrawnFaces;
    sithRender_g_numAlphaThingPoly += rdModel3_g_numDrawnAlphaFaces;

    // Clear cur dvec aka position
    pThing->orient.dvec = rdroid_g_zeroVector3;

    // Extra draw func
    if ( sithRender_pExtraThingRenderFunc && (pThing->flags & SITH_TF_UNKNOWN_200000) != 0 )
    {
        sithRender_pExtraThingRenderFunc(pThing);
    }

    if ( pThing->type != SITH_THING_EXPLOSION || (pThing->thingInfo.explosionInfo.flags & SITH_EF_BLINDPLAYER) == 0 )
    {
        return drawResult;
    }

    // Note: Looks like some part of the code is missing.
    // In JKDF2 (OpenJKDF2), here is section of code that adds light flashing effect to player view
    stdMath_Dist3D1(pThing->cameraSpacePos.x, pThing->cameraSpacePos.y, pThing->cameraSpacePos.z);

    pThing->thingInfo.explosionInfo.flags &= ~SITH_EF_BLINDPLAYER;// 0x100 - SITH_EF_BLINDPLAYER
    return drawResult;
}

#ifdef J3D_OPENGL
void sithRender_RenderAlphaAdjoinsStatic(void)
{
    sithRender_g_numAlphaArchPolys = 0;

    rdFaceFlags extraFaceFlags = 0;
    if ( sithWorld_g_pCurrentWorld->fog.bEnabled )
    {
        extraFaceFlags = RD_FF_FOG_ENABLED;
    }

    bool bFlush = false;
    for ( size_t surfNum = 0; surfNum < sithRender_numAlphaAdjoins; ++surfNum )
    {
        SithSurface* pSurf  = sithRender_aAlphaAdjoins[surfNum];
        rdPayload* pPayload = rdCache_GetTransparentDrawCall(RD_DRAW_GEOMETRY);

        rdGeoFacePayload* pDrawCall = &pPayload->geoFacePayload;


        rdVector4 extraLight = pSurf->face.extraLight;
        rdVector_Add4Acc(&extraLight, &pSurf->pSector->extraLight);
        extraLight.alpha       = pSurf->aIntensities[0].alpha;
        pPayload->lightingMode = pSurf->face.lightingMode >= sithRender_lightMode ? sithRender_lightMode : pSurf->face.lightingMode;
        pDrawCall->faceNum     = pSurf->face.num;
        pPayload->flags        = pSurf->face.flags;
        pPayload->flags |= extraFaceFlags;
        pPayload->extraLight = extraLight;
        pPayload->pMaterial  = pSurf->face.pMaterial;
        pPayload->matCelNum  = pSurf->face.matCelNum;

        float distance = FLT_MAX; // 3.4028235e38f;

        // get min z in camera space
        for ( size_t i = 0; i < pSurf->face.numVertices; ++i )
        {
            rdVector3 min = sithWorld_g_pCurrentWorld->aVertices[pSurf->face.aVertices[i]];

            rdMatrix_TransformPoint34Acc(&min, &rdCamera_g_pCurCamera->viewMatrix);
            if ( min.y < distance )
            {
                distance = min.y;
            }
        }
        pPayload->distance = distance;
        rdCache_AddTransparentDrawCall();

        ++sithRender_g_numAlphaArchPolys;
        bFlush = true;
    }


    if ( bFlush )
    {
        rdCache_FlushAlpha();
    }
}
#endif

void sithRender_RenderAlphaAdjoins(void)
{
#ifdef J3D_OPENGL
    if ( !std3D_g_bUseLegacyRendering )
    {
        sithRender_RenderAlphaAdjoinsStatic();
        return;
    }
#endif

    sithRender_g_numAlphaArchPolys = 0;

    rdFaceFlags extraFaceFlags = 0;
    if ( sithWorld_g_pCurrentWorld->fog.bEnabled )
    {
        extraFaceFlags = RD_FF_FOG_ENABLED;
    }

    bool bFlush = false;
    for ( size_t surfNum = 0; surfNum < sithRender_numAlphaAdjoins; ++surfNum )
    {
        SithSurface* pSurf  = sithRender_aAlphaAdjoins[surfNum];
        SithSector* pSector = pSurf->pSector;

        rdVector4 ambientLight = pSector->ambientLight; // TODO: Why the ambient light is applied? For normal surfs that is not the case
        rdVector_Add4Acc(&ambientLight, &pSector->extraLight);
        ambientLight.alpha = 0.0f;
        // TOCO: clamp vector to 0.0 - 1.0f?

        rdCamera_SetAmbientLight(rdCamera_g_pCurCamera, &ambientLight);

        // Transform vertices to view (camera) space
        if ( pSurf->renderTick != sithMain_g_curRenderTick )
        {
            for ( size_t vertNum = 0; vertNum < pSurf->face.numVertices; ++vertNum )
            {
                int vertIdx = pSurf->face.aVertices[vertNum];
                if ( sithWorld_g_pCurrentWorld->aVertexRenderTickIds[vertIdx] != sithMain_g_curRenderTick )
                {
                    rdMatrix_TransformPoint34(
                        &sithWorld_g_pCurrentWorld->aTransformedVertices[vertIdx],
                        &sithWorld_g_pCurrentWorld->aVertices[vertIdx],
                        &rdCamera_g_pCurCamera->viewMatrix
                    );
                    sithWorld_g_pCurrentWorld->aVertexRenderTickIds[vertIdx] = sithMain_g_curRenderTick;
                }
            }

            pSurf->renderTick = sithMain_g_curRenderTick;
        }

        rdCacheProcEntry* pPoly = rdCache_GetAlphaProcEntry();
        if ( !pPoly )
        {
            // TODO: Maybe add log
            continue;
        }

        pPoly->lightingMode = pSurf->face.lightingMode >= sithRender_lightMode ? sithRender_lightMode : pSurf->face.lightingMode;

        if ( !rdClip_FaceToPlane(pSector->pClipFrustum, pPoly, &pSurf->face, sithWorld_g_pCurrentWorld->aTransformedVertices, sithWorld_g_pCurrentWorld->aTexVerticies, sithWorld_g_pCurrentWorld->aVertDynamicLights, pSurf->aIntensities) )
        {
            // Face is fully outside frustrum
            continue;
        }

        rdVector_Add4(&pPoly->extraLight, &pSurf->face.extraLight, &pSector->extraLight);
        // TODO: Maybe clamp [0.0,1.0]

        pPoly->matCelNum = pSurf->face.matCelNum;
        pPoly->flags     = extraFaceFlags | pSurf->face.flags;
        pPoly->pMaterial = pSurf->face.pMaterial;

        rdCache_AddAlphaProcFace(pSurf->face.numVertices);

        ++sithRender_g_numAlphaArchPolys;
        bFlush = true;
    }

    if ( bFlush )
    {
        rdCache_FlushAlpha();
    }
}

void J3DAPI sithRender_SetExtraThingRenderFunc(int (J3DAPI* pfFunc)(SithThing* pThing))
{
    // Note, in OpenJkdf this looks like to be a special weapon render function
    sithRender_pExtraThingRenderFunc = pfFunc;
}

int sithRender_MakeScreenShot(void)
{
    const char* pNameTemplate = sithGetScreenShotFileTemplate();

    char aFilename[64];
    size_t ssnum = 0;
    STD_FORMAT(aFilename, pNameTemplate, ssnum);
    const char* pDir = sithGetScreenShotsDir();

    char aFilePath[128];
    stdFnames_MakePath(aFilePath, sizeof(aFilePath), pDir, aFilename);

    while ( stdFileUtil_FileExists(aFilePath) && ssnum < SITHRENDER_MAXSCREENSHOTS )
    {
        pNameTemplate = sithGetScreenShotFileTemplate();
        STD_FORMAT(aFilename, pNameTemplate, ++ssnum);

        pDir = sithGetScreenShotsDir();
        stdFnames_MakePath(aFilePath, sizeof(aFilePath), pDir, aFilename);
    }

    if ( ssnum <= SITHRENDER_MAXSCREENSHOTS )
    {
        return stdDisplay_SaveScreen(aFilePath);
    }

    ssnum = 0;
    pNameTemplate = sithGetScreenShotFileTemplate();
    STD_FORMAT(aFilename, pNameTemplate, ssnum);

    pDir = sithGetScreenShotsDir();
    stdFnames_MakePath(aFilePath, sizeof(aFilePath), pDir, aFilename);
    return stdDisplay_SaveScreen(aFilePath);
}
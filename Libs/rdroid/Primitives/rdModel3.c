#include "rdModel3.h"
#include <j3dcore/j3dhook.h>

#include <rdroid/Main/rdroid.h>
#include <rdroid/Math/rdMath.h>
#include <rdroid/Math/rdMatrix.h>
#include <rdroid/Math/rdVector.h>
#include <rdroid/Engine/rdCamera.h>
#include <rdroid/Engine/rdClip.h>
#include <rdroid/Engine/rdMaterial.h>
#include <rdroid/Engine/rdLight.h>
#include <rdroid/Engine/rdPuppet.h>
#include <rdroid/Primitives/rdPrimit3.h>
#include <rdroid/Raster/rdCache.h>
#include <rdroid/Raster/rdFace.h>
#include <rdroid/RTI/symbols.h>

#include <std/General/stdConffile.h>
#include <std/General/stdMemory.h>
#include <std/General/stdUtil.h>

#define RDMODEL3_3DO_MAJVER      2
#define RDMODEL3_3DO_MINVER_I    1
#define RDMODEL3_3DO_MINVER_RGB  2
#define RDMODEL3_3DO_MINVER_RGBA 3

static rdModel3LoaderFunc pModel3Loader     = NULL;
static rdModel3UnloaderFunc pModel3Unloader = NULL;

// Vars used when loading 3DO model
static bool bCalcGeoRadius   = false;
static float maxGeoRadius = 0.0;

// Vars used for drawing model 
static rdModel3DrawFaceFunc pfDrawFace = &rdModel3_DrawFace;

static const rdThing* pCurThing    = NULL;
static rdLightMode curLightingMode = RD_LIGHTING_NONE;

static size_t numGeoLights = 0;
static rdLight* apGeoLights[RDCAMERA_MAX_LIGHTS] = { 0 };

static bool bDrawMeshBounds                   = false;
static const rdModel3Mesh* pCurMesh           = NULL;
static rdLightMode lightingMode               = RD_LIGHTING_NONE;
static rdVector3 localCamera                  = { 0 };
static rdVector3 aView[RDMODEL3_MAX_VERTICES] = { 0 };

static size_t numMeshLights = 0;
static rdLight* apMeshLights[RDCAMERA_MAX_LIGHTS] = { 0 };
rdVector3 aLocalLightPos[RDCAMERA_MAX_LIGHTS]     = { 0 };

static rdFaceFlags extraFaceFlags = 0;

void J3DAPI rdModel3_LoadPostProcess(rdModel3* pModel3); // Added

void rdModel3_InstallHooks(void)
{
    J3D_HOOKFUNC(rdModel3_RegisterLoader);
    J3D_HOOKFUNC(rdModel3_NewEntry);
    J3D_HOOKFUNC(rdModel3_LoadEntry);
    J3D_HOOKFUNC(rdModel3_FreeEntry);
    J3D_HOOKFUNC(rdModel3_FreeEntryGeometryOnly);
    J3D_HOOKFUNC(rdModel3_BuildExpandedRadius);
    J3D_HOOKFUNC(rdModel3_GetMeshMatrix);
    J3D_HOOKFUNC(rdModel3_ReplaceMesh);
    J3D_HOOKFUNC(rdModel3_SwapMesh);
    J3D_HOOKFUNC(rdModel3_GetThingColor);
    J3D_HOOKFUNC(rdModel3_SetModelColor);
    J3D_HOOKFUNC(rdModel3_SetThingColor);
    J3D_HOOKFUNC(rdModel3_SetNodeColor);
    J3D_HOOKFUNC(rdModel3_SetNodeColorForNonAmputatedJoints);
    J3D_HOOKFUNC(rdModel3_Draw);
    J3D_HOOKFUNC(rdModel3_DrawHNode);
    J3D_HOOKFUNC(rdModel3_DrawMesh);
    J3D_HOOKFUNC(rdModel3_DrawFace);
    J3D_HOOKFUNC(rdModel3_EnableFogRendering);
}

void rdModel3_ResetGlobals(void)
{
    memset(&rdModel3_g_numDrawnFaces, 0, sizeof(rdModel3_g_numDrawnFaces));
    memset(&rdModel3_g_numDrawnAlphaFaces, 0, sizeof(rdModel3_g_numDrawnAlphaFaces));
}

rdModel3LoaderFunc J3DAPI rdModel3_RegisterLoader(rdModel3LoaderFunc pfFunc)
{
    rdModel3LoaderFunc pPrevFunc;

    pPrevFunc = pModel3Loader;
    pModel3Loader = pfFunc;
    return pPrevFunc;
}

rdModel3UnloaderFunc J3DAPI rdModel3_RegisterUnloader(rdModel3UnloaderFunc pfFunc)
{
    rdModel3UnloaderFunc pPrevFunc;

    pPrevFunc = pModel3Unloader;
    pModel3Unloader = pfFunc;
    return pPrevFunc;
}

void J3DAPI rdModel3_NewEntry(rdModel3* pModel3)
{
    memset(pModel3, 0, sizeof(rdModel3));
    STD_STRCPY(pModel3->aName, "UNKNOWN");
    pModel3->curGeoNum = 0;
}

rdModel3* J3DAPI rdModel3_Load(const char* pName)
{
    if ( pModel3Loader )
    {
        return pModel3Loader(pName, /*bSkipLoadingDefaultModel=*/0);
    }

    rdModel3* pModel3 = (rdModel3*)STDMALLOC(sizeof(rdModel3));
    if ( !pModel3 )
    {
        RDLOG_ERROR("Error: Bad memory allocation for model '%s'.\n", pName);
        return NULL;
    }

    if ( !rdModel3_LoadEntry(pName, pModel3) )
    {
        rdModel3_Free(pModel3);
        return NULL;
    }

    return pModel3;
}

int J3DAPI rdModel3_LoadEntry(const char* pFilename, rdModel3* pModel3)
{
    rdModel3_NewEntry(pModel3);

    STD_STRCPY(pModel3->aName, stdFileFromPath(pFilename));

    if ( !stdConffile_Open(pFilename) )
    {
        RDLOG_ERROR("Error: Invalid load filename '%s'.\n", pFilename);
        return 0;
    }

    if ( stdConffile_ScanLine(" section: %s", std_g_genBuffer, (rsize_t)sizeof(std_g_genBuffer)) != 1 )
    {
        goto syntax_error;
    }

    int vmin = -1, vmaj = -1;
    int nRead = 0;
    if ( nRead = stdConffile_ScanLine(" 3do %d.%d", &vmaj, &vmin), nRead != 2 )
    {
        if ( nRead < 0 )
        {
            goto syntax_error;
        }
        RDLOG_ERROR("Warning: Invalid 3DO version %d.%d for file '%s'.\n", vmaj, vmin, pFilename);
        // TODO: Should return error?
    }

    // TODO: Add check for vmaj

    bool bRGB = false;
    if ( vmin == RDMODEL3_3DO_MINVER_RGB || vmin == RDMODEL3_3DO_MINVER_RGBA )
    {
        bRGB = true;
    }
    else if ( vmin != RDMODEL3_3DO_MINVER_I )
    {
        RDLOG_ERROR("Warning: Invalid 3DO version %d.%d for file '%s'.\n", vmaj, vmin, pFilename);
    }

    if ( stdConffile_ScanLine(" section: %s", std_g_genBuffer, (rsize_t)sizeof(std_g_genBuffer)) != 1 )
    {
        goto syntax_error;
    }

    size_t numMats = 0;
    if ( nRead = stdConffile_ScanLine(" materials %d", &numMats), nRead != 1 )
    {
        if ( nRead < 0 )
        {
            goto eof_error;
        }
        goto syntax_error;
    }

    if ( numMats )
    {
        pModel3->apMaterials = (rdMaterial**)STDMALLOC(sizeof(rdMaterial**) * numMats);
        if ( !pModel3->apMaterials )
        {
            goto alloc_error;
        }

        for ( size_t i = 0; i < numMats; ++i )
        {
            size_t entryNum;
            if ( nRead = stdConffile_ScanLine(" %d: %s", &entryNum, std_g_genBuffer, (rsize_t)sizeof(std_g_genBuffer)), nRead != 2 )
            {
                if ( nRead < 0 )
                {
                    goto eof_error;
                }
                goto syntax_error;
            }

            pModel3->apMaterials[i] = rdMaterial_Load(std_g_genBuffer);
            if ( !pModel3->apMaterials[i] )
            {
                goto syntax_error;
            }
        }
    }
    pModel3->numMaterials = numMats; // TODO [BUG]: Note, assigning numMats here makes sure that materials are not getting freed in case of mat load error.
                                     //             But this might also be cause of bug because from here on in any case of an error the materials are getting freed. 
                                     //             Since materials are cached in sithMaterial module all freed materials will be invalidated in cache.
                                     //             And any part of the code that is referencing cached materials will access freed pointer! 

    if ( stdConffile_ScanLine(" section: %s", std_g_genBuffer, (rsize_t)sizeof(std_g_genBuffer)) != 1 )
    {
        goto syntax_error;
    }

    if ( stdConffile_ScanLine(" radius %f", &pModel3->radius) != 1 )
    {
        goto syntax_error;
    }

    if ( stdConffile_ScanLine(" insert offset %f %f %f", &pModel3->insertOffset.x, &pModel3->insertOffset.y, &pModel3->insertOffset.z) != 3 )
    {
        goto syntax_error;
    }

    if ( stdConffile_ScanLine(" geosets %d", &pModel3->numGeos) != 1 )
    {
        goto syntax_error;
    }

    for ( size_t geoNum = 0; geoNum < pModel3->numGeos; geoNum++ )
    {
        rdModel3GeoSet* pGeo = &pModel3->aGeos[geoNum];
        int entryNum = 0;
        if ( stdConffile_ScanLine(" geoset %d", &entryNum) != 1 )
        {
            goto syntax_error;
        }

        if ( nRead = stdConffile_ScanLine(" meshes %d", &pGeo->numMeshes), nRead != 1 )
        {
            if ( nRead < 0 )
            {
                goto eof_error;
            }
            goto syntax_error;
        }

        // TODO: maybe check numMeshes range, if 0 should continue 

        pGeo->aMeshes = (rdModel3Mesh*)STDMALLOC(sizeof(rdModel3Mesh) * pGeo->numMeshes);
        if ( !pGeo->aMeshes )
        {
            goto alloc_error;
        }

        for ( size_t meshNum = 0; meshNum < pGeo->numMeshes; meshNum++ )
        {
            rdModel3Mesh* pMesh = &pGeo->aMeshes[meshNum];
            pMesh->num = meshNum;

            rdVector_Set4(&pMesh->meshColor, 1.0f, 1.0f, 1.0f, 1.0f); // white

            if ( nRead = stdConffile_ScanLine(" mesh %d", std_g_genBuffer, (rsize_t)sizeof(std_g_genBuffer)), nRead != 1 )
            {
                if ( nRead < 0 )
                {
                    goto eof_error;
                }
                goto syntax_error;
            }

            if ( nRead = stdConffile_ScanLine(" name %s", pMesh->name, (rsize_t)STD_ARRAYLEN(pMesh->name)), nRead != 1 )
            {
                if ( nRead < 0 )
                {
                    goto eof_error;
                }
                goto syntax_error;
            }
            //STD_STRCPY(pMesh->name, std_g_genBuffer);

            if ( stdConffile_ScanLine(" radius %f", &pMesh->radius) != 1 )
            {
                goto syntax_error;
            }

            if ( nRead = stdConffile_ScanLine(" geometrymode %d", &pMesh->geoMode), nRead != 1 )
            {
                if ( nRead < 0 )
                {
                    goto eof_error;
                }
                goto syntax_error;
            }

            if ( nRead = stdConffile_ScanLine(" lightingmode %d", &pMesh->lightMode), nRead != 1 )
            {
                if ( nRead < 0 )
                {
                    goto eof_error;
                }
                goto syntax_error;
            }

            int texMode;
            if ( nRead = stdConffile_ScanLine(" texturemode %d", &texMode), nRead != 1 ) // texture mode not used
            {
                if ( nRead < 0 )
                {
                    goto eof_error;
                }
                goto syntax_error;
            }

            // Read mesh vertices

            size_t numVerts = 0;
            if ( nRead = stdConffile_ScanLine(" vertices %d", &numVerts), nRead != 1 )
            {
                if ( nRead < 0 )
                {
                    goto eof_error;
                }

                goto syntax_error;
            }

            if ( numVerts > RDMODEL3_MAX_VERTICES )
            {
                goto range_error;
            }

            pMesh->apVertices        = NULL;
            pMesh->aVertColors       = NULL;
            pMesh->aLightIntensities = NULL;

            pMesh->someFaceFlags = 0;
            if ( pMesh->lightMode != RD_LIGHTING_GOURAUD )
            {
                pMesh->someFaceFlags = 1;
            }

            if ( numVerts )
            {
                pMesh->apVertices = (rdVector3*)STDMALLOC(sizeof(rdVector3) * numVerts);
                if ( !pMesh->apVertices )
                {
                    goto alloc_error;
                }

                pMesh->aVertColors = (rdVector4*)STDMALLOC(sizeof(rdVector4) * numVerts);
                if ( !pMesh->aVertColors )
                {
                    goto alloc_error;
                }

                pMesh->aLightIntensities = (rdVector4*)STDMALLOC(sizeof(rdVector4) * numVerts);
                if ( !pMesh->aLightIntensities )
                {
                    goto alloc_error;
                }

                memset(pMesh->aLightIntensities, 0, sizeof(rdVector4) * numVerts);
            }

            for ( size_t vertNum = 0; vertNum < numVerts; ++vertNum )
            {
                if ( !stdConffile_ReadLine() )
                {
                    goto eof_error;
                }

                float x, y, z;
                if ( bRGB )
                {
                    float red, green, blue, alpha = 1.0f;
                    if ( sscanf_s(stdConffile_g_aLine, " %d: %f %f %f %f %f %f %f", &entryNum, &x, &y, &z, &red, &green, &blue, &alpha) != 8
                        && sscanf_s(stdConffile_g_aLine, " %d: %f %f %f %f %f %f", &entryNum, &x, &y, &z, &red, &green, &blue) != 7 )
                    {
                        goto syntax_error;
                    }

                    rdVector_Set4(&pMesh->aVertColors[vertNum], red, green, blue, alpha);
                }
                else
                {
                    float i;
                    if ( sscanf_s(stdConffile_g_aLine, " %d: %f %f %f %f", &entryNum, &x, &y, &z, &i) != 5 )
                    {
                        goto syntax_error;
                    }
                    rdVector_Set4(&pMesh->aVertColors[vertNum], i, i, i, 1.0f);
                }

                pMesh->apVertices[vertNum].x = x;
                pMesh->apVertices[vertNum].y = y;
                pMesh->apVertices[vertNum].z = z;
            }

            pMesh->numVertices = numVerts;

            // Read UV vertices

            size_t numTexVerts = 0;
            if ( nRead = stdConffile_ScanLine(" texture vertices %d", &numTexVerts), nRead != 1 )
            {
                if ( nRead < 0 )
                {
                    goto eof_error;
                }
                goto syntax_error;
            }

            if ( numTexVerts > RDMODEL3_MAX_TEXVRTICES )
            {
                goto range_error;
            }

            pMesh->apTexVertices = NULL;
            if ( numTexVerts )
            {
                pMesh->apTexVertices = (rdVector2*)STDMALLOC(sizeof(rdVector2) * numTexVerts);
                if ( !pMesh->apTexVertices )
                {
                    goto alloc_error;
                }

                for ( size_t vertNum = 0; vertNum < numTexVerts; ++vertNum )
                {
                    if ( nRead = stdConffile_ScanLine(" %d: %f %f", &entryNum, &pMesh->apTexVertices[vertNum].x, &pMesh->apTexVertices[vertNum].y), nRead != 3 )
                    {
                        if ( nRead < 0 )
                        {
                            goto eof_error;
                        }
                        goto syntax_error;
                    }
                }
            }

            pMesh->numTexVertices = numTexVerts;

            // Read vert normals

            // Skip section line "VERTEX NORMALS"
            if ( !stdConffile_ReadLine() )
            {
                goto eof_error;
            }

            pMesh->aVertNormals = NULL;
            if ( numVerts )
            {
                pMesh->aVertNormals = (rdVector3*)STDMALLOC(sizeof(rdVector3) * numVerts);
                if ( !pMesh->aVertNormals )
                {
                    goto alloc_error;
                }
            }

            for ( size_t vertNum = 0; vertNum < numVerts; vertNum++ )
            {
                rdVector3* pNormal = &pMesh->aVertNormals[vertNum];
                if ( nRead = stdConffile_ScanLine(" %d: %f %f %f", &entryNum, &pNormal->x, &pNormal->y, &pNormal->z), nRead != 4 )
                {
                    if ( nRead < 0 )
                    {
                        goto eof_error;
                    }
                    goto syntax_error;
                }
            }

            // Read mesh faces

            size_t numFaces = 0;
            if ( nRead = stdConffile_ScanLine(" faces %d", &numFaces), nRead != 1 )
            {
                if ( nRead < 0 )
                {
                    goto eof_error;
                }
                goto syntax_error;
            }

            if ( numFaces > RDMODEL3_MAX_FACES )
            {
                goto range_error;
            }

            pMesh->numFaces = numFaces;
            pMesh->aFaces   = NULL;

            if ( numFaces )
            {
                pMesh->aFaces = (rdFace*)STDMALLOC(sizeof(rdFace) * numFaces);
                if ( !pMesh->aFaces )
                {
                    goto alloc_error;
                }
            }

            for ( size_t faceNum = 0; faceNum < numFaces; ++faceNum )
            {
                rdFace* pFace = &pMesh->aFaces[faceNum];
                rdFace_NewEntry(pFace);

                // Read face line 
                if ( !stdConffile_ReadLine() )
                {
                    goto eof_error;
                }
                pFace->num = faceNum;

                // Skip face num
                char* pntok = NULL;
                strtok_r(stdConffile_g_aLine, " \t", &pntok);

                // Parse mat idx
                int matIdx = atoi(strtok_r(NULL, " \t", &pntok));
                pFace->pMaterial = (matIdx == -1) ? NULL : pModel3->apMaterials[matIdx]; // TODO: Add bounds check

                // Parse faceflags
                if ( sscanf_s(strtok_r(NULL, " \t", &pntok), "%x", &pFace->flags) != 1 )
                {
                    goto syntax_error;
                }

                // Parse geo mode
                if ( sscanf_s(strtok_r(NULL, " \t", &pntok), "%d", &pFace->geometryMode) != 1 )
                {
                    goto syntax_error;
                }

                // Parse light mode
                if ( sscanf_s(strtok_r(NULL, " \t", &pntok), "%d", &pFace->lightingMode) != 1 )
                {
                    goto syntax_error;
                }

                if ( pFace->lightingMode != RD_LIGHTING_GOURAUD )
                {
                    pMesh->someFaceFlags = 1;
                }

                if ( sscanf_s(strtok_r(NULL, " \t", &pntok), "%d", &texMode) != 1 )
                { // textMode not used
                    goto syntax_error;
                }

                // Parse extra light
                if ( bRGB )
                {
                    float red, green, blue, alpha = 1.0f;
                    const char* aRGB = strtok_r(NULL, " \t", &pntok);
                    if ( sscanf_s(aRGB, "(%f/%f/%f/%f)", &red, &green, &blue, &alpha) != 4
                        && sscanf_s(aRGB, "(%f/%f/%f)", &red, &green, &blue) != 3 )
                    {
                        goto syntax_error;
                    }

                    rdVector_Set4(&pFace->extraLight, red, green, blue, alpha);
                }
                else
                {
                    float li;
                    if ( sscanf_s(strtok_r(NULL, " \t", &pntok), "%f", &li) != 1 )
                    {
                        goto syntax_error;
                    }
                    rdVector_Set4(&pFace->extraLight, li, li, li, 1.0f);
                }

                size_t faceVerts = atoi(strtok_r(NULL, " \t", &pntok));
                if ( !faceVerts )
                {
                    goto syntax_error;
                }

                if ( faceVerts > RDMODEL3_MAX_FACE_VERTICES )
                {
                    goto range_error;
                }

                pFace->numVertices = faceVerts;
                pFace->aVertices = (int*)STDMALLOC(sizeof(int*) * faceVerts);
                if ( !pFace->aVertices )
                {
                    goto alloc_error;
                }

                if ( pFace->pMaterial )
                {
                    pFace->aTexVertices = (int*)STDMALLOC(sizeof(int*) * faceVerts);
                    if ( !pFace->aTexVertices )
                    {
                        goto alloc_error;
                    }

                    for ( size_t vertNum = 0; vertNum < faceVerts; ++vertNum )
                    {
                        pFace->aVertices[vertNum] = atoi(strtok_r(NULL, " \t,", &pntok)); // Parse vert index
                        if ( pFace->aVertices[vertNum] > pMesh->numVertices - 1 )
                        {
                            pFace->aVertices[vertNum] = 0;
                            RDLOG_ERROR("Out of range vertex on mesh %s of model %s!\n", pMesh->name, pModel3->aName);
                        }

                        pFace->aTexVertices[vertNum] = atoi(strtok_r(NULL, " \t,", &pntok)); // Parse UV index;
                        if ( pFace->aTexVertices[vertNum] > pMesh->numTexVertices - 1 )
                        {
                            pFace->aTexVertices[vertNum] = 0;
                            RDLOG_ERROR("Out of range tex vertex on mesh %s of model %s!\n", pMesh->name, pModel3->aName);
                        }
                    }
                }
                else
                {
                    for ( size_t vertNum = 0; vertNum < faceVerts; ++vertNum )
                    {
                        pFace->aVertices[vertNum] = atoi(strtok_r(NULL, " \t,", &pntok)); // Parse vert index

                        // Added
                        if ( pFace->aVertices[vertNum] > pMesh->numVertices - 1 )
                        {
                            pFace->aVertices[vertNum] = 0;
                            RDLOG_ERROR("Out of range vertex on mesh %s of model %s!\n", pMesh->name, pModel3->aName);
                        }

                        strtok_r(NULL, " \t,", &pntok);// Skip texVertIdx
                    }
                }

                pMesh->someFaceFlags |= pFace->flags & RD_FF_TEX_TRANSLUCENT;
                if ( pFace->pMaterial )
                {
                    pMesh->someFaceFlags |= pFace->pMaterial->formatType == STDCOLOR_FORMAT_RGBA;
                }
            } // Read faces

            // Read Face normals

            // Skip section line "FACE NORMALS"
            if ( !stdConffile_ReadLine() )
            {
                goto eof_error;
            }

            for ( size_t faceNum = 0; faceNum < pMesh->numFaces; ++faceNum )
            {
                rdFace* pFace = &pMesh->aFaces[faceNum];
                if ( nRead = stdConffile_ScanLine(" %d: %f %f %f", &entryNum, &pFace->normal.x, &pFace->normal.y, &pFace->normal.z), nRead != 4 )
                {
                    if ( nRead < 0 )
                    {
                        goto eof_error;
                    }
                    goto syntax_error;
                }
            }
        }
    }


    // Section HIERARCHYDEF
    if ( stdConffile_ScanLine(" section: %s", std_g_genBuffer, (rsize_t)sizeof(std_g_genBuffer)) != 1 )
    {
        goto syntax_error;
    }

    size_t numNodes = 0;
    if ( nRead = stdConffile_ScanLine(" hierarchy nodes %d", &numNodes), nRead != 1 )
    {
        if ( nRead < 0 )
        {
            goto eof_error;
        }
        goto syntax_error;
    }

    pModel3->aHierarchyNodes = (rdModel3HNode*)STDMALLOC(sizeof(rdModel3HNode) * numNodes);
    if ( !pModel3->aHierarchyNodes )
    {
        goto alloc_error;
    }
    pModel3->numHNodes = numNodes;

    for ( size_t nodeNum = 0; nodeNum < numNodes; ++nodeNum )
    {
        rdModel3HNode* pNode = &pModel3->aHierarchyNodes[nodeNum];
        pNode->num = nodeNum;

        if ( !stdConffile_ReadLine() )
        {
            goto eof_error;
        }
        size_t entryNum;
        int flags, parentIdx, childIdx, siblingIdx;
        if ( sscanf_s(
            stdConffile_g_aLine,
            " %d: %x %x %d %d %d %d %d %f %f %f %f %f %f %f %f %f %s",
            &entryNum,
            &flags,
            &pNode->type,
            &pNode->meshIdx,
            &parentIdx,
            &childIdx,
            &siblingIdx,
            &pNode->numChildren,
            &pNode->pos.x,
            &pNode->pos.y,
            &pNode->pos.z,
            &pNode->pyr.pitch,
            &pNode->pyr.yaw,
            &pNode->pyr.roll,
            &pNode->pivot.x,
            &pNode->pivot.y,
            &pNode->pivot.z,
            pNode->aName, (rsize_t)STD_ARRAYLEN(pNode->aName)) != 18 )
        {
            goto syntax_error;
        }

        // TODO: Add bounds check
        pNode->pParent  = (parentIdx == -1) ? NULL : &pModel3->aHierarchyNodes[parentIdx];
        pNode->pChild   = (childIdx == -1) ? NULL : &pModel3->aHierarchyNodes[childIdx];
        pNode->pSibling = (siblingIdx == -1) ? NULL : &pModel3->aHierarchyNodes[siblingIdx];
    }

    stdConffile_Close();

    bCalcGeoRadius = true;
    maxGeoRadius = 0.0f;
    rdModel3_BuildExpandedRadius(pModel3, pModel3->aHierarchyNodes, &rdroid_g_identMatrix34);
    pModel3->size = maxGeoRadius;
    bCalcGeoRadius = false;

    return 1; // Success

    // Failure
eof_error:
    RDLOG_ERROR("Error: Unexpected EOF in '%s'\n", pFilename);
    goto error;

syntax_error:
    RDLOG_ERROR("Error: Invalid syntax '%s' in '%s'\n", stdConffile_g_aLine, pFilename);
    goto error;

alloc_error:
    RDLOG_ERROR("Error: Bad memory allocation for '%s' in '%s'\n", stdConffile_g_aLine, pFilename);
    goto error;

range_error:
    RDLOG_ERROR("Error: Value out of range for '%s' in '%s'\n", stdConffile_g_aLine, pFilename);
    goto error;

error:
    rdModel3_FreeEntry(pModel3);
    stdConffile_Close();
    return 0;
}

void J3DAPI rdModel3_LoadPostProcess(rdModel3* pModel3)
{
    rdModel3_CalcRadii(pModel3);
    rdModel3_CalcFaceNormals(pModel3);
    rdModel3_CalcVertexNormals(pModel3);
    RD_ASSERTREL(rdModel3_Validate(pModel3));
}

int J3DAPI rdModel3_Write(const char* pFilename, const rdModel3* pModel, const char* pCratedName)
{
    tFileHandle fh = rdroid_g_pHS->pFileOpen(pFilename, "wt+");
    if ( !fh )
    {
        return 0;
    }

    rdroid_g_pHS->pFilePrintf(fh, "# MODEL '%s' created from '%s'\n\n", pModel, pCratedName);
    rdroid_g_pHS->pFilePrintf(fh, "###############\n");
    rdroid_g_pHS->pFilePrintf(fh, "SECTION: HEADER\n\n");
    rdroid_g_pHS->pFilePrintf(fh, "3DO %d.%d\n\n", RDMODEL3_3DO_MAJVER, RDMODEL3_3DO_MINVER_RGBA);
    rdroid_g_pHS->pFilePrintf(fh, "###############\n");
    rdroid_g_pHS->pFilePrintf(fh, "SECTION: MODELRESOURCE\n\n");
    rdroid_g_pHS->pFilePrintf(fh, "# Materials list\n");
    rdroid_g_pHS->pFilePrintf(fh, "MATERIALS %d\n\n", pModel->numMaterials);

    for ( size_t i = 0; i < pModel->numMaterials; ++i )
    {
        rdroid_g_pHS->pFilePrintf(fh, "%10d:%15s\n", i, pModel->apMaterials[i]);
    }

    rdroid_g_pHS->pFilePrintf(fh, "\n\n");
    rdroid_g_pHS->pFilePrintf(fh, "###############\n");
    rdroid_g_pHS->pFilePrintf(fh, "SECTION: GEOMETRYDEF\n\n");
    rdroid_g_pHS->pFilePrintf(fh, "# Object radius\n");
    rdroid_g_pHS->pFilePrintf(fh, "RADIUS %10.6f\n\n", pModel->radius);
    rdroid_g_pHS->pFilePrintf(fh, "# Insertion offset\n");
    rdroid_g_pHS->pFilePrintf(fh, "INSERT OFFSET %10.6f %10.6f %10.6f\n\n", pModel->insertOffset.x, pModel->insertOffset.y, pModel->insertOffset.z);
    rdroid_g_pHS->pFilePrintf(fh, "# Number of Geometry Sets\n");
    rdroid_g_pHS->pFilePrintf(fh, "GEOSETS %d\n\n", pModel->numGeos);

    for ( size_t i = 0; i < pModel->numGeos; ++i )
    {
        const rdModel3GeoSet* pGeo = &pModel->aGeos[i];
        rdroid_g_pHS->pFilePrintf(fh, "# Geometry Set definition\n");
        rdroid_g_pHS->pFilePrintf(fh, "GEOSET %d\n\n", i);
        rdroid_g_pHS->pFilePrintf(fh, "# Number of Meshes\n");
        rdroid_g_pHS->pFilePrintf(fh, "MESHES %d\n\n\n", pGeo->numMeshes);

        for ( size_t meshNum = 0; meshNum < pGeo->numMeshes; ++meshNum )
        {
            // Write mesh properties

            const rdModel3Mesh* pMesh = &pModel->aGeos[i].aMeshes[meshNum];
            rdroid_g_pHS->pFilePrintf(fh, "# Mesh definition\n");
            rdroid_g_pHS->pFilePrintf(fh, "MESH %d\n\n", meshNum);
            rdroid_g_pHS->pFilePrintf(fh, "NAME %s\n\n", pMesh);
            rdroid_g_pHS->pFilePrintf(fh, "RADIUS %10.6f\n\n", pMesh->radius);
            rdroid_g_pHS->pFilePrintf(fh, "GEOMETRYMODE\t%d\n", pMesh->geoMode);
            rdroid_g_pHS->pFilePrintf(fh, "LIGHTINGMODE\t%d\n", pMesh->lightMode);
            rdroid_g_pHS->pFilePrintf(fh, "TEXTUREMODE\t%d\n", 3);
            rdroid_g_pHS->pFilePrintf(fh, "\n\n");

            // Write mesh vertices

            rdroid_g_pHS->pFilePrintf(fh, "VERTICES %d\n\n", pMesh->numVertices);
            rdroid_g_pHS->pFilePrintf(fh, "# num:     x:         y:         z:         i: \n");
            for ( size_t vertNum = 0; vertNum < pMesh->numVertices; ++vertNum )
            {
                rdroid_g_pHS->pFilePrintf(
                    fh,
                    "  %3d: %10.6f %10.6f %10.6f %8.6f %8.6f %8.6f %8.6f\n",
                    vertNum,
                    pMesh->apVertices[vertNum].x,
                    pMesh->apVertices[vertNum].y,
                    pMesh->apVertices[vertNum].z,
                    pMesh->aVertColors[vertNum].red,
                    pMesh->aVertColors[vertNum].green,
                    pMesh->aVertColors[vertNum].blue,
                    pMesh->aVertColors[vertNum].alpha
                );
            }

            // Write mesh UV vertices
            rdroid_g_pHS->pFilePrintf(fh, "\n\n");
            rdroid_g_pHS->pFilePrintf(fh, "TEXTURE VERTICES %d\n\n", pMesh->numTexVertices);
            for ( size_t texVertNum = 0; texVertNum < pMesh->numTexVertices; ++texVertNum )
            {
                rdroid_g_pHS->pFilePrintf(fh, "  %3d: %10.6f %10.6f\n", texVertNum, pMesh->apTexVertices[texVertNum].x, pMesh->apTexVertices[texVertNum].y);
            }

            rdroid_g_pHS->pFilePrintf(fh, "\n\n");

            // Write vert normals
            rdroid_g_pHS->pFilePrintf(fh, "VERTEX NORMALS\n\n");
            rdroid_g_pHS->pFilePrintf(fh, "# num:     x:         y:         z:\n");
            for ( size_t vertNum = 0; vertNum < pMesh->numVertices; ++vertNum )
            {
                rdroid_g_pHS->pFilePrintf(
                    fh,
                    "  %3d: %10.6f %10.6f %10.6f\n",
                    vertNum,
                    pMesh->aVertNormals[vertNum].x,
                    pMesh->aVertNormals[vertNum].y,
                    pMesh->aVertNormals[vertNum].z
                );
            }

            rdroid_g_pHS->pFilePrintf(fh, "\n\n");

            // Write mesh faces 
            rdroid_g_pHS->pFilePrintf(fh, "FACES %d\n\n", pMesh->numFaces);

            rdroid_g_pHS->pFilePrintf(fh, "#  num:  material:   type:  geo:  light:   tex:  R:  G:  B:  A:  verts:\n");
            for ( size_t faceNum = 0; faceNum < pMesh->numFaces; ++faceNum )
            {
                rdFace* pFace = &pMesh->aFaces[faceNum];

                int matNum = -1;
                if ( pFace->pMaterial )
                {
                    for ( size_t matIdx = 0; matIdx < pModel->numMaterials; ++matIdx )
                    {
                        if ( pFace->pMaterial == pModel->apMaterials[matIdx] )
                        {
                            matNum = matIdx;
                        }
                    }

                    RD_ASSERTREL(matNum != -1);
                }

                rdroid_g_pHS->pFilePrintf(
                    fh,
                    "   %3d: %9d  0x%04x  %4d %7d %6d (%f/%f/%f/%f) %7d  ",
                    faceNum,
                    matNum,
                    pFace->flags,
                    pFace->geometryMode,
                    pFace->lightingMode,
                    3, // tex mode
                    pFace->extraLight.red,
                    pFace->extraLight.green,
                    pFace->extraLight.blue,
                    pFace->extraLight.alpha,
                    pFace->numVertices
                );

                if ( pFace->aTexVertices )
                {
                    for ( size_t vertNum = 0; vertNum < pFace->numVertices; ++vertNum )
                    {
                        rdroid_g_pHS->pFilePrintf(fh, "%3d,%3d ", pFace->aVertices[vertNum], pFace->aTexVertices[vertNum]);
                    }
                }
                else
                {
                    for ( size_t vertNum = 0; vertNum < pFace->numVertices; ++vertNum )
                    {
                        rdroid_g_pHS->pFilePrintf(fh, "%3d, 0 ", pFace->aVertices[vertNum]);
                    }
                }

                rdroid_g_pHS->pFilePrintf(fh, "\n");
            }

            rdroid_g_pHS->pFilePrintf(fh, "\n\n");

            // Write face normals
            rdroid_g_pHS->pFilePrintf(fh, "FACE NORMALS\n\n");

            rdroid_g_pHS->pFilePrintf(fh, "# num:     x:         y:         z:\n");
            for ( size_t faceNum = 0; faceNum < pMesh->numFaces; ++faceNum )
            {
                rdFace* pFace = &pMesh->aFaces[faceNum];
                rdroid_g_pHS->pFilePrintf(fh, "  %3d: %10.6f %10.6f %10.6f\n", faceNum, pFace->normal.x, pFace->normal.y, pFace->normal.z);
            }

            rdroid_g_pHS->pFilePrintf(fh, "\n\n");
        }
    }

    // Write hnodes

    rdroid_g_pHS->pFilePrintf(fh, "###############\n");
    rdroid_g_pHS->pFilePrintf(fh, "SECTION: HIERARCHYDEF\n\n");
    rdroid_g_pHS->pFilePrintf(fh, "# Hierarchy node list\n");
    rdroid_g_pHS->pFilePrintf(fh, "HIERARCHY NODES %d\n\n", pModel->numHNodes);
    rdroid_g_pHS->pFilePrintf(
        fh,
        "#  num:   flags:   type:    mesh:  parent:  child:  sibling:  numChildren:        x:         y:         z:     pitch:       yaw:      roll:    pivotx:  "
        "  pivoty:    pivotz:  hnodename:\n");

    for ( size_t nodeNum = 0; nodeNum < pModel->numHNodes; ++nodeNum )
    {
        rdModel3HNode* pNode = &pModel->aHierarchyNodes[nodeNum];
        int parentNum  = pNode->pParent ? pNode->pParent->num : -1;
        int childNum   = pNode->pChild ? pNode->pChild->num : -1;
        int siblingNum = pNode->pSibling ? pNode->pSibling->num : -1;

        rdroid_g_pHS->pFilePrintf(
            fh,
            "   %3d:  0x%04x 0x%05X %8d %8d %7d %9d %13d %10.6f %10.6f %10.6f %10.6f %10.6f %10.6f %10.6f %10.6f %10.6f  %s\n",
            nodeNum,
            0, // flags
            pNode->type,
            pNode->meshIdx,
            parentNum,
            childNum,
            siblingNum,
            pNode->numChildren,
            pNode->pos.x,
            pNode->pos.y,
            pNode->pos.z,
            pNode->pyr.pitch,
            pNode->pyr.yaw,
            pNode->pyr.roll,
            pNode->pivot.x,
            pNode->pivot.y,
            pNode->pivot.z,
            pNode->aName
        );
    }

    rdroid_g_pHS->pFileClose(fh);
    return 1;
}

void J3DAPI rdModel3_Free(rdModel3* pModel3)
{
    if ( !pModel3 )
    {
        RDLOG_ERROR("Warning: Attempt to free NULL model ptr.\n");
        return;
    }

    if ( pModel3Unloader )
    {
        pModel3Unloader(pModel3);
    }
    else
    {
        rdModel3_FreeEntry(pModel3);
        stdMemory_Free(pModel3);
    }
}

void J3DAPI rdModel3_FreeEntry(rdModel3* pModel3)
{
    if ( !pModel3 )
    {
        RDLOG_ERROR("Warning: Attempt to free NULL model ptr.\n");
        return;
    }

    for ( size_t i = 0; i < pModel3->numGeos; ++i )
    {
        for ( size_t j = 0; j < pModel3->aGeos[i].numMeshes; ++j )
        {
            rdModel3Mesh* pMesh = &pModel3->aGeos[i].aMeshes[j];
            if ( pMesh->apVertices )
            {
                stdMemory_Free(pMesh->apVertices);
            }

            if ( pMesh->apTexVertices )
            {
                stdMemory_Free(pMesh->apTexVertices);
            }

            if ( pMesh->aFaces )
            {
                for ( size_t k = 0; k < pMesh->numFaces; ++k )
                {
                    rdFace_FreeEntry(&pMesh->aFaces[k]);
                }

                stdMemory_Free(pMesh->aFaces);
            }

            if ( pMesh->aVertColors )
            {
                stdMemory_Free(pMesh->aVertColors);
            }

            if ( pMesh->aLightIntensities )
            {
                stdMemory_Free(pMesh->aLightIntensities);
            }

            if ( pMesh->aVertNormals )
            {
                stdMemory_Free(pMesh->aVertNormals);
            }
        }

        if ( pModel3->aGeos[i].aMeshes )
        {
            stdMemory_Free(pModel3->aGeos[i].aMeshes);
        }
    }

    if ( pModel3->aHierarchyNodes )
    {
        stdMemory_Free(pModel3->aHierarchyNodes);
    }

    for ( size_t i = 0; i < pModel3->numMaterials; ++i )
    {
        // TODO: add check for null material
        rdMaterial_Free(pModel3->apMaterials[i]);
    }

    if ( pModel3->apMaterials )
    {
        stdMemory_Free(pModel3->apMaterials);
    }
}

void J3DAPI rdModel3_FreeEntryGeometryOnly(rdModel3* pModel3)
{
    if ( !pModel3 )
    {
        RDLOG_ERROR("Warning: Attempt to free NULL model ptr.\n");
        return;
    }

    for ( size_t i = 0; i < pModel3->numGeos; ++i )
    {
        for ( size_t j = 0; j < pModel3->aGeos[i].numMeshes; ++j )
        {
            rdModel3Mesh* pMesh = &pModel3->aGeos[i].aMeshes[j];
            if ( pMesh->apVertices )
            {
                stdMemory_Free(pMesh->apVertices);
            }

            if ( pMesh->apTexVertices )
            {
                stdMemory_Free(pMesh->apTexVertices);
            }

            if ( pMesh->aFaces )
            {
                for ( size_t k = 0; k < pMesh->numFaces; ++k )
                {
                    rdFace_FreeEntry(&pMesh->aFaces[k]);
                }

                stdMemory_Free(pMesh->aFaces);
            }

            if ( pMesh->aVertColors )
            {
                stdMemory_Free(pMesh->aVertColors);
            }

            if ( pMesh->aLightIntensities )
            {
                stdMemory_Free(pMesh->aLightIntensities);
            }

            if ( pMesh->aVertNormals )
            {
                stdMemory_Free(pMesh->aVertNormals);
            }
        }

        if ( pModel3->aGeos[i].aMeshes )
        {
            stdMemory_Free(pModel3->aGeos[i].aMeshes);
        }
    }

    if ( pModel3->aHierarchyNodes )
    {
        stdMemory_Free(pModel3->aHierarchyNodes);
    }

    if ( pModel3->apMaterials )
    {
        stdMemory_Free(pModel3->apMaterials);
    }
}

bool J3DAPI rdModel3_Validate(const rdModel3* pModel3)
{
    // Added
    if ( !pModel3 || pModel3->numGeos == 0 || pModel3->numGeos >= STD_ARRAYLEN(pModel3->aGeos) )
    {
        return false;
    }

    for ( size_t i = 0; i < pModel3->numGeos; ++i )
    {
        for ( size_t j = 0; j < pModel3->aGeos[i].numMeshes; ++j )
        {
            rdModel3Mesh* pMesh = &pModel3->aGeos[i].aMeshes[j];
            for ( size_t k = 0; k < pMesh->numFaces; ++k )
            {
                rdFace* pFace = &pMesh->aFaces[k];
                if ( pFace->pMaterial )
                {
                    if ( pFace->geometryMode < RD_GEOMETRY_FULL )
                    {
                        return false;
                    }
                }
                else if ( pFace->geometryMode > RD_GEOMETRY_NONE )
                {
                    return false;
                }
            }
        }
    }

    return true;
}

void J3DAPI rdModel3_CalcRadii(rdModel3* pModel3)
{
    // Added
    if ( !pModel3 )
    {
        return;
    }

    for ( size_t i = 0; i < pModel3->aGeos[0].numMeshes; ++i )
    {
        float radius = 0.0f;
        rdModel3Mesh* pMesh = &pModel3->aGeos[0].aMeshes[i];
        for ( size_t j = 0; j < pMesh->numVertices; ++j )
        {
            float len = rdVector_Len3(&pMesh->apVertices[j]);
            if ( len > radius )
            {
                radius = len;
            }
        }

        pMesh->radius = radius;
    }

    bCalcGeoRadius = true; // Added
    maxGeoRadius = 0.0f;
    rdModel3_BuildExpandedRadius(pModel3, pModel3->aHierarchyNodes, &rdroid_g_identMatrix34);
    pModel3->radius = maxGeoRadius;
    bCalcGeoRadius = false;
}

void J3DAPI rdModel3_BuildExpandedRadius(const rdModel3* pModel, const rdModel3HNode* pNode, const rdMatrix34* orient)
{
    rdMatrix34 nmat, norient;
    rdMatrix_Build34(&nmat, &pNode->pyr, &pNode->pos);
    rdMatrix_BuildTranslate34(&norient, &pNode->pivot);
    rdMatrix_PostMultiply34(&norient, &nmat);

    if ( pNode->pParent )
    {
        rdVector3 pivot;
        rdVector_Neg3(&pivot, &pNode->pParent->pivot);
        rdMatrix_PostTranslate34(&norient, &pivot);
    }

    rdMatrix34 tmat;
    rdMatrix_Multiply34(&tmat, orient, &norient);

    if ( pNode->meshIdx != -1 )
    {
        const rdModel3Mesh* pMesh = &pModel->aGeos[0].aMeshes[pNode->meshIdx];

        for ( size_t i = 0; i < pMesh->numVertices; ++i )
        {
            rdVector3 tvert;
            rdMatrix_TransformPoint34(&tvert, &pMesh->apVertices[i], &tmat);

            float radius = tvert.z;
            if ( bCalcGeoRadius )
            {
                radius = rdVector_Len3(&tvert);
            }

            if ( radius > (double)maxGeoRadius )
            {
                maxGeoRadius = radius;
            }
        }
    }

    if ( pNode->numChildren )
    {
        const rdModel3HNode* pChild = pNode->pChild;
        for ( size_t i = 0; i < pNode->numChildren; ++i )
        {
            rdModel3_BuildExpandedRadius(pModel, pChild, &tmat);
            pChild = pChild->pSibling;
        }
    }
}

void J3DAPI rdModel3_CalcFaceNormals(rdModel3* pModel3)
{
    for ( size_t i = 0; i < pModel3->numGeos; ++i )
    {
        for ( size_t j = 0; j < pModel3->aGeos[i].numMeshes; ++j )
        {
            rdModel3Mesh* pMesh = &pModel3->aGeos[i].aMeshes[j];
            for ( size_t k = 0; k < pMesh->numFaces; ++k )
            {
                rdFace* pFace = &pMesh->aFaces[k];
                RD_ASSERTREL(pFace->numVertices > 2);

                // Find collinear vertices
                size_t vertNum = 0, prevVertNum = 0, nextVertNum = 0;
                for ( ; vertNum < pFace->numVertices; ++vertNum )
                {
                    prevVertNum = pFace->numVertices - 1;
                    if ( vertNum > 0 )
                    {
                        prevVertNum = vertNum - 1;
                    }

                    nextVertNum = (vertNum + 1) % pFace->numVertices;
                    if ( !rdMath_PointsCollinear(
                        &pMesh->apVertices[pFace->aVertices[vertNum]],
                        &pMesh->apVertices[pFace->aVertices[nextVertNum]],
                        &pMesh->apVertices[pFace->aVertices[prevVertNum]]) )
                    {
                        break;
                    }
                }

                if ( vertNum >= pFace->numVertices )
                {
                    RDLOG_ERROR("Warning: Invalid Face encountered while calculating normals in model: '%s'\n", pModel3->aName);
                }
                else
                {
                    rdMath_CalcSurfaceNormal(
                        &pFace->normal,
                        &pMesh->apVertices[pFace->aVertices[vertNum]],
                        &pMesh->apVertices[pFace->aVertices[nextVertNum]],
                        &pMesh->apVertices[pFace->aVertices[prevVertNum]]);
                }
            }
        }
    }
}

void J3DAPI rdModel3_CalcVertexNormals(rdModel3* pModel)
{
    for ( size_t i = 0; i < pModel->numGeos; ++i )
    {
        for ( size_t j = 0; j < pModel->aGeos[i].numMeshes; ++j )
        {
            rdModel3Mesh* pMesh = &pModel->aGeos[i].aMeshes[j];
            for ( size_t k = 0; k < pMesh->numVertices; ++k )
            {
                rdVector3 normal = { 0 };
                float numVerts   = 0.0f;
                for ( size_t m = 0; m < pMesh->numFaces; ++m )
                {
                    rdFace* pFace = &pMesh->aFaces[m];
                    for ( size_t n = 0; n < pFace->numVertices; ++n )
                    {
                        if ( pFace->aVertices[n] == k )
                        {
                            rdVector_Add3Acc(&normal, &pFace->normal);
                            numVerts = numVerts + 1.0f;
                            break;
                        }
                    }
                }

                rdVector3* pVertNormal = &pMesh->aVertNormals[k];
                if ( numVerts == 0.0f )
                {
                    rdVector_Set3(pVertNormal, 1.0f, 0.0f, 0.0f);
                    RDLOG_ERROR("Warning: Unused vertex found while calculating vert normals.\n");
                }
                else
                {
                    if ( numVerts == 1.0f )
                    {
                        rdFace* pFace = pMesh->aFaces;

                        size_t vertNum     = pFace->numVertices - 1;
                        size_t nextVertNum = pFace->numVertices != 1 ? 1 : 0;//1 % pFace->numVertices;

                        rdVector3 dvert;
                        rdVector_Sub3(&dvert, &pMesh->apVertices[0], &pMesh->apVertices[pFace->aVertices[vertNum]]);

                        rdVector3 dvertNext;
                        rdVector_Sub3(&dvertNext, &pMesh->apVertices[0], &pMesh->apVertices[pFace->aVertices[nextVertNum]]);

                        rdVector_Add3(pVertNormal, &dvert, &dvertNext);
                    }
                    else
                    {
                        pVertNormal->x = normal.x / numVerts;
                        pVertNormal->y = normal.y / numVerts;
                        pVertNormal->z = normal.z / numVerts;
                    }
                    rdVector_Normalize3Acc(pVertNormal);
                }
            }
        }
    }
}

rdModel3HNode* J3DAPI rdModel3_FindNamedNode(const char* pName, rdModel3* pModel3)
{
    for ( size_t i = 0; i < pModel3->numHNodes; ++i )
    {
        rdModel3HNode* pNode = &pModel3->aHierarchyNodes[i];
        if ( streqi(pNode->aName, pName) )
        {
            return pNode;
        }
    }

    return NULL;
}

int J3DAPI rdModel3_GetMeshMatrix(rdThing* pThing, const rdMatrix34* orient, unsigned int nodeNum, rdMatrix34* meshOrient)
{
    RD_ASSERTREL(pThing && meshOrient);
    RD_ASSERTREL(orient);
    RD_ASSERTREL(pThing->type == RD_THING_MODEL3);
    RD_ASSERTREL(pThing->data.pModel3);

    if ( nodeNum >= pThing->data.pModel3->numHNodes )
    {
        return 0;
    }

    if ( pThing->rdFrameNum != rdCache_GetFrameNum() )
    {
        rdPuppet_BuildJointMatrices(pThing, orient);
    }

    rdMatrix_Copy34(meshOrient, &pThing->paJointMatrices[nodeNum]);
    return 1;
}

int J3DAPI rdModel3_ReplaceMesh(rdModel3* pModel, unsigned int geosetNum, unsigned int meshNum, const rdModel3Mesh* pSrcMesh)
{
    if ( geosetNum >= pModel->numGeos )
    {
        return 1;
    }

    if ( meshNum >= pModel->aGeos[geosetNum].numMeshes )
    {
        return 1;
    }

    memcpy(&pModel->aGeos[geosetNum].aMeshes[meshNum], pSrcMesh, sizeof(pModel->aGeos[geosetNum].aMeshes[meshNum]));
    return 0;
}

int J3DAPI rdModel3_SwapMesh(rdModel3* pModel1, unsigned int meshNum1, rdModel3* pModel2, int meshNum2)
{
    if ( !pModel1 || !pModel2 )
    {
        return 1;
    }

    rdModel3Mesh* pMesh2 = &pModel2->aGeos[0].aMeshes[meshNum2];

    rdModel3Mesh mesh1;
    memcpy(&mesh1, &pModel1->aGeos[0].aMeshes[meshNum1], sizeof(mesh1));

    if ( rdModel3_ReplaceMesh(pModel1, 0, meshNum1, pMesh2) || rdModel3_ReplaceMesh(pModel2, 0, meshNum2, &mesh1) )
    {
        RDLOG_ERROR("Error: SwapMesh for models '%s and %s'.\n", pModel1->aName, pModel2->aName);
        return 1;

    }

    return 0;
}

void J3DAPI rdModel3_GetThingColor(const rdThing* pThing, rdVector4* pDestColor)
{
    const rdModel3* pModel3 = pThing->data.pModel3;
    const rdModel3HNode* pNode = pModel3->aHierarchyNodes;

    size_t geoNum = pThing->geosetNum;
    if ( pThing->geosetNum == -1 )
    {
        geoNum = pModel3->curGeoNum;
    }

    if ( pNode->meshIdx == -1 )
    {
        if ( pNode->numChildren )
        {
            const rdModel3HNode* pChild = pNode->pChild;
            if ( pChild->meshIdx == -1 )
            {
                pDestColor->red   = 0.0f;
                pDestColor->green = 0.0f;
                pDestColor->blue  = 0.0f;
                pDestColor->alpha = 1.0f;
            }
            else
            {
                rdVector_Copy4(pDestColor, &pModel3->aGeos[geoNum].aMeshes[pChild->meshIdx].meshColor);
            }
        }
    }
    else
    {
        rdVector_Copy4(pDestColor, &pModel3->aGeos[geoNum].aMeshes[pNode->meshIdx].meshColor);
    }
}

void J3DAPI rdModel3_SetModelColor(rdModel3* pModel, const rdVector4* pColor)
{
    rdModel3_SetNodeColor(pModel, pModel->aGeos, pModel->aHierarchyNodes, pColor);
}

void J3DAPI rdModel3_SetThingColor(rdThing* prdThing, const rdVector4* pColor)
{
    if ( prdThing->type == RD_THING_MODEL3 )
    {
        rdModel3* pModel3 = prdThing->data.pModel3;
        size_t geoNum = prdThing->geosetNum;
        if ( prdThing->geosetNum == -1 )
        {
            geoNum = pModel3->curGeoNum;
        }

        rdModel3_SetNodeColorForNonAmputatedJoints(prdThing, &pModel3->aGeos[geoNum], pModel3->aHierarchyNodes, pColor);
    }
}

void J3DAPI rdModel3_SetNodeColor(const rdModel3* pModel, rdModel3GeoSet* pGeoSet, const rdModel3HNode* pNode, const rdVector4* pColor)
{
    if ( pNode->meshIdx != -1 )
    {
        rdVector_Copy4(&pGeoSet->aMeshes[pNode->meshIdx].meshColor, pColor);
    }

    if ( pNode->numChildren )
    {
        rdModel3HNode* pChild = pNode->pChild;
        for ( size_t i = 0; i < pNode->numChildren; ++i )
        {
            rdModel3_SetNodeColor(pModel, pGeoSet, pChild, pColor);
            pChild = pChild->pSibling;
        }
    }
}

void J3DAPI rdModel3_SetNodeColorForNonAmputatedJoints(const rdThing* prdThing, rdModel3GeoSet* pGeoSet, const rdModel3HNode* pNode, const rdVector4* pColor)
{
    if ( pNode->meshIdx != -1 )
    {
        rdVector_Copy4(&pGeoSet->aMeshes[pNode->meshIdx].meshColor, pColor);
    }

    if ( pNode->numChildren )
    {
        rdModel3HNode* pChild = pNode->pChild;
        for ( size_t i = 0; i < pNode->numChildren; ++i )
        {
            if ( !prdThing->paJointAmputationFlags[pChild->num] )
            {
                rdModel3_SetNodeColorForNonAmputatedJoints(prdThing, pGeoSet, pChild, pColor);
            }

            pChild = pChild->pSibling;
        }
    }
}

int J3DAPI rdModel3_Draw(rdThing* pThing, const rdMatrix34* pPlacement)
{
    RD_ASSERTREL(rdCamera_g_pCurCamera != NULL);
    RD_ASSERTREL(pThing != NULL);
    RD_ASSERTREL(pThing->data.pModel3 != NULL);

    rdModel3_g_numDrawnAlphaFaces = 0;
    rdModel3_g_numDrawnFaces      = 0;

    pCurThing = pThing;
    rdModel3* pModel3  = pThing->data.pModel3;

    if ( pThing->frustumCull == RDFRUSTUMCULL_OUTSIDE )
    {
        return 0;
    }

    size_t geoNum = pThing->geosetNum;
    if ( pThing->geosetNum == -1 ) // TODO: add lower bound check for greater or equal to 0
    {
        geoNum = pModel3->curGeoNum;
    }
    // TODO: Add upper bound check for geoNum

    if ( pThing->rdFrameNum != rdCache_GetFrameNum() || rdCamera_g_pCurCamera->pCanvas->pVBuffer->rasterInfo.colorInfo.bpp == 8 )
    {
        rdPuppet_BuildJointMatrices(pThing, pPlacement);
    }

    curLightingMode = rdroid_g_curLightingMode;
    if ( rdroid_g_curLightingMode > RD_LIGHTING_LIT )
    {
        numGeoLights = 0;
        for ( size_t i = 0; i < rdCamera_g_pCurCamera->numLights; ++i )
        {
            apGeoLights[numGeoLights++] = rdCamera_g_pCurCamera->aLights[i];
        }
    }

    rdModel3_DrawHNode(&pModel3->aGeos[geoNum], pModel3->aHierarchyNodes);
    return 1;
}

void J3DAPI rdModel3_DrawHNode(const rdModel3GeoSet* prdGeo, const rdModel3HNode* pNode)
{
    if ( pNode->meshIdx != -1 )
    {
        rdModel3Mesh* pMesh = &prdGeo->aMeshes[pNode->meshIdx];
        // Note the rdModel3K module is buggy and will not be used
        // 
        //bool bOpaque = pMesh->meshColor.alpha >= 1.0f;
        //if ( (bOpaque & (pMesh->someFaceFlags == 0) & (uint8_t)rdModel3K_sub_4E2F20()) != 0 )
        //{
        //    rdModel3K_DrawMesh(pMesh, &rdModel3_pCurThing->paJointMatrices[pNode->num]);
        //}
        //else
        {
            rdModel3_DrawMesh(pMesh, &pCurThing->paJointMatrices[pNode->num]);
        }
    }

    if ( pNode->numChildren )
    {
        rdModel3HNode* pChild = pNode->pChild;
        for ( size_t i = 0; i < pNode->numChildren; ++i )
        {
            if ( !pCurThing->paJointAmputationFlags[pChild->num] )
            {
                rdModel3_DrawHNode(prdGeo, pChild);
            }

            pChild = pChild->pSibling;
        }
    }
}

#ifdef J3D_OPENGL
static void rdModel3_DrawMeshStatic(const rdModel3Mesh* pMesh, const rdMatrix34* orient)
{
    lightingMode = pCurMesh->lightMode;

    for ( size_t i = 0; i < pMesh->numFaces; ++i )
    {
        rdFace* pFace     = &pMesh->aFaces[i];
        bool bTranslucent = (pFace->flags & RD_FF_TEX_TRANSLUCENT) != 0;
        if ( pFace->pMaterial )
        {
            bTranslucent = (bTranslucent || pFace->pMaterial->formatType == STDCOLOR_FORMAT_RGBA) && pFace->pMaterial->formatType != STDCOLOR_FORMAT_RGBA_1BITALPHA;
        }

        if ( pMesh->meshColor.alpha != 1.0f )
        {
            bTranslucent = true;
        }

        if ( bTranslucent )
        {
            ++rdModel3_g_numDrawnAlphaFaces;
        }
        else
        {
            ++rdModel3_g_numDrawnFaces;
        }

        if ( pFace->num == -1 )
        {
            continue;
        }

        rdPayload* pPayload;
        if ( bTranslucent )
        {
            pPayload = rdCache_GetTransparentDrawCall(RD_DRAW_MODEL);
        }
        else
        {
            pPayload = rdCache_GetOpaqueDrawCall(RD_DRAW_MODEL);
        }
        rdModelFacePayload* pDrawCall = pPayload->payload;

        pPayload->lightingMode = pFace->lightingMode;
        if ( lightingMode < pFace->lightingMode )
        {
            pPayload->lightingMode = lightingMode;
        }
        rdVector4 extraLight = pFace->extraLight;
        extraLight.alpha     = 1.0f;
        if ( (rdroid_g_curRenderOptions & RDROID_USE_AMBIENT_CAMERA_LIGHT) != 0 )
        {
            rdVector_Add4Acc(&extraLight, &rdCamera_g_pCurCamera->ambientLight);
        }

        pDrawCall->modelMatrix = orient;

        if ( rdCamera_g_pCurCamera->numLights > 0 )
        {
            pDrawCall->sectorLight   = *rdCamera_g_pCurCamera->aLights[rdCamera_g_pCurCamera->numLights - 1];
            pDrawCall->lightPosition = rdCamera_g_pCurCamera->aLightPositions[rdCamera_g_pCurCamera->numLights - 1];
        }
        else
        {
            pDrawCall->sectorLight.bEnabled = false;
        }

        pPayload->flags     = pFace->flags | extraFaceFlags;
        pPayload->pMaterial = pFace->pMaterial;
        pPayload->matCelNum = pFace->matCelNum;
        pDrawCall->faceNum  = pFace->num;


        if ( bTranslucent )
        {
            extraLight.alpha = pMesh->meshColor.alpha;

            // rdVector3 viewPos = orient->dvec; // world position (pivot)
            // rdMatrix_TransformPoint34Acc(&viewPos, &rdCamera_g_pCurCamera->viewMatrix);

            rdMatrix34 tmat;
            rdMatrix_Multiply34(&tmat, &rdCamera_g_pCurCamera->viewMatrix, orient);
            float distance = FLT_MAX; // 3.4028235e38f;

            size_t f = i;

            while ( f < pMesh->numFaces )
            {
                const rdFace* nextFace = &pMesh->aFaces[f];
                if ( f > i && nextFace->num != -1 )
                {
                    break;
                }
                // get min z in camera space
                for ( size_t j = 0; j < nextFace->numVertices; ++j )
                {
                    rdVector3 min = pMesh->apVertices[nextFace->aVertices[j]];
                    rdMatrix_TransformPoint34Acc(&min, &tmat);
                    if ( min.y < distance )
                    {
                        distance = min.y;
                    }
                }
                f++;
            }
            pPayload->distance   = distance;
            pPayload->extraLight = extraLight;
            rdCache_AddTransparentDrawCall();
        }
        else
        {
            pPayload->extraLight = extraLight;
            rdCache_AddOpaqueDrawCall();
        }
    }
}
#endif

void J3DAPI rdModel3_DrawMesh(const rdModel3Mesh* pMesh, const rdMatrix34* orient)
{
    RD_ASSERTREL(rdCamera_g_pCurCamera != NULL);
    RD_ASSERTREL(pMesh != NULL);

    pCurMesh = pMesh;

#ifdef J3D_OPENGL
    if ( !std3D_g_bUseLegacyRendering )
    {
        rdModel3_DrawMeshStatic(pMesh, orient);
        return;
    }
#endif

    if ( pMesh->geoMode )
    {
        rdVector3 tpos;
        rdMatrix_TransformPoint34(&tpos, &orient->dvec, &rdCamera_g_pCurCamera->viewMatrix); // Transform mesh position to camera view space
        // GrimEngine does this:
        //    if ( pThing->frustrumCullStatus ) // status here can be either 0 (in frustum) or 1 (intersect) and not 2 because such thing would be skipped for drawing in rdModel3_Draw
        //    {
        //       if ( (rdroid_curCullFlags & 1) != 0 )
        //       {
        //          meshFrustrumCull = rdClip_SphereInFrustrum(
        //                                        rdCamera_pCurCamera->pClipFrustum,
        //                                        &tpos.x,
        //                                        pCurMesh->radius);
        //          if ( meshFrustrumCull == 2 ) { // not in frustum
        //             return;
        //          }
        //       }
        //       else {
        //          meshFrustrumCull = 1;
        //       }
        //    }
        //    else {
        //       meshFrustrumCull = 0;
        //    }

        // Rotate vertices for orient model matrix (world space) and transform them to view (camera) space
        rdMatrix34 tmat;
        rdMatrix_Multiply34(&tmat, &rdCamera_g_pCurCamera->viewMatrix, orient); // Combine model-view matrices
        rdMatrix_TransformPointList34(&tmat, pCurMesh->apVertices, aView, pCurMesh->numVertices); // Transform vertices to view space (i.e. rotate to orinet and convert to view space)

        // Calculate model matrix
        rdMatrix34 InvModelMatrix;
        rdMatrix_InvertOrtho34(&InvModelMatrix, orient);

        lightingMode = pCurMesh->lightMode;
        if ( curLightingMode < lightingMode )
        {
            lightingMode = curLightingMode;
        }

        if ( lightingMode == RD_LIGHTING_DIFFUSE )
        {
            numMeshLights = 0;
            for ( size_t i = 0; i < numGeoLights; ++i )
            {
                rdLight* pLight = apGeoLights[i];
                // TODO: grimengine checks if pLight->bEnabled

                rdVector3 dir;
                rdVector_Sub3(&dir, &rdCamera_g_pCurCamera->aLightPositions[pLight->num], &orient->dvec);
                // TODO: grimengine check if (pCurMesh->radius + pLight->minRadius <= rdVector_Len3(&dir)) continue;

                // Transform light to model local position
                rdMatrix_TransformPoint34(&aLocalLightPos[numMeshLights], &rdCamera_g_pCurCamera->aLightPositions[pLight->num], &InvModelMatrix);
                apMeshLights[numMeshLights] = pLight;
                ++numMeshLights;
            }
        }
        else if ( lightingMode == RD_LIGHTING_GOURAUD )
        {
            numMeshLights = 0;
            for ( size_t i = 0; i < numGeoLights; ++i )
            {
                rdLight* pLight = apGeoLights[i];
                // TODO: grimengine checks if pLight->bEnabled

                rdVector3 dir;
                rdVector_Sub3(&dir, &rdCamera_g_pCurCamera->aLightPositions[pLight->num], &orient->dvec);
                // TODO: grimengine check if (pCurMesh->radius + pLight->minRadius <= rdVector_Len3(&dir)) continue;

                // Transform light to mesh local space
                rdMatrix_TransformPoint34(&aLocalLightPos[numMeshLights], &rdCamera_g_pCurCamera->aLightPositions[pLight->num], &InvModelMatrix);
                apMeshLights[numMeshLights] = pLight;
                ++numMeshLights;
            }

            rdLight_CalcVertexIntensities(
                rdCamera_g_pCurCamera->aLights, // TODO: use apMeshLights
                aLocalLightPos,
                rdCamera_g_pCurCamera->numLights, // TODO: use numMeshLights
                pCurMesh->aVertNormals,
                pCurMesh->apVertices,
                pCurMesh->aVertColors,
                pCurMesh->aLightIntensities,
                pCurMesh->numVertices
            );

            if ( bDrawMeshBounds )
            {
                // Draw mesh bounds
                rdPrimit3_DrawClippedCircle(&orient->dvec, pCurMesh->radius, 60.0f, 0xFFFF3F2F, 0xFFFFFFFF);
            }
        }

        // Transform camera world position to mesh local position
        rdMatrix_TransformPoint34(&localCamera, &rdCamera_g_camMatrix.dvec, &InvModelMatrix);

        for ( size_t i = 0; i < pMesh->numFaces; ++i )
        {
            const rdFace* pFace = &pMesh->aFaces[i];

            // Draw mesh face if it's 1st vertex is facing camera, or backface culling is disabled
            rdVector3 dir;
            rdVector_Sub3(&dir, &localCamera, &pCurMesh->apVertices[*pFace->aVertices]);
            float dot = rdVector_Dot3(&pFace->normal, &dir);
            if ( dot > 0.0f || (pFace->flags & RD_FF_DOUBLE_SIDED) != 0 || (rdroid_g_curRenderOptions & RDROID_BACKFACE_CULLING_ENABLED) == 0 )
            {
                int bBackFace = dot <= 0.0f;
                pfDrawFace(pFace, aView, bBackFace, &pMesh->meshColor);
            }
        }
    }
}

void J3DAPI rdModel3_DrawFace(const rdFace* pFace, const rdVector3* aTransformedVertices, int bIsBackFace, const rdVector4* pMeshColor)
{
    bool bTranslucent = (pFace->flags & RD_FF_TEX_TRANSLUCENT) != 0;
    if ( pFace->pMaterial )
    {
        bTranslucent = bTranslucent || (pFace->pMaterial->formatType == STDCOLOR_FORMAT_RGBA);
    }

    if ( pMeshColor->alpha != 1.0f )
    {
        bTranslucent = true;
    }

    rdCacheProcEntry* pPoly;
    if ( bTranslucent )
    {
        pPoly = rdCache_GetAlphaProcEntry();
    }
    else
    {
        pPoly = rdCache_GetProcEntry();
    }

    pPoly->lightingMode  = pFace->lightingMode;
    if ( lightingMode < pFace->lightingMode )
    {
        pPoly->lightingMode = lightingMode;
    }

    // Project vertices to view space and assign to poly
    // Fyi, grimengine uses either rdPrim3_ClipFace or rdPrim3_NoClipFace because it does manual clipping of polys that are not in clip frustum 
    // We expect HW / GPU API will do the clipping for us
    if ( !rdClip_FaceToPlane(rdCamera_g_pCurCamera->pFrustum, pPoly, pFace, aTransformedVertices, pCurMesh->apTexVertices, pCurMesh->aLightIntensities, NULL) )
    {
        // Poly is fully outside of the frustum
        return;
    }

    rdVector_Copy4(&pPoly->extraLight, &pFace->extraLight);

    if ( pPoly->lightingMode == RD_LIGHTING_DIFFUSE )
    {
        // Note: grim engine also uses pre-calculated light directions (aLocalLightDir)

        rdVector3 faceNormal = pFace->normal;
        if ( bIsBackFace )
        {
            rdVector_Neg3Acc(&faceNormal);
        }

        rdVector4 faceIntensity = { 0 };
        rdLight_CalcFaceIntensity(
            rdCamera_g_pCurCamera->aLights, // TODO: use apMeshLights
            aLocalLightPos,
            rdCamera_g_pCurCamera->numLights, // TODO: use numMeshLights
            pFace,
            &faceNormal,
            pCurMesh->apVertices,
            rdCamera_g_pCurCamera->attenuationMin,
            &faceIntensity
        );

        rdVector_Add4Acc(&pPoly->extraLight, &faceIntensity);
    }

    if ( (rdroid_g_curRenderOptions & RDROID_USE_AMBIENT_CAMERA_LIGHT) != 0 )
    {
        rdVector_Add4Acc(&pPoly->extraLight, &rdCamera_g_pCurCamera->ambientLight);
    }

    if ( pMeshColor->alpha != 1.0f )
    {
        pPoly->extraLight.alpha = 1.0f;
        for ( size_t i = 0; i < pFace->numVertices; ++i )
        {
            pPoly->aVertIntensities[i].alpha = pMeshColor->alpha;
        }
    }

    pPoly->flags     = pFace->flags | extraFaceFlags;
    pPoly->matCelNum = pFace->matCelNum;
    pPoly->pMaterial = pFace->pMaterial;

    if ( pMeshColor->alpha != 1.0f )
    {
        pPoly->flags |= RD_FF_TEX_TRANSLUCENT;
    }

    if ( bTranslucent )
    {
        ++rdModel3_g_numDrawnAlphaFaces;
        rdCache_AddAlphaProcFace(pFace->numVertices);
    }
    else
    {
        ++rdModel3_g_numDrawnFaces;
        rdCache_AddProcFace(pFace->numVertices);
    }
}

void J3DAPI rdModel3_EnableFogRendering(int bEnabled)
{
    extraFaceFlags = 0;
    if ( bEnabled )
    {
        extraFaceFlags = RD_FF_FOG_ENABLED;
    }
}

rdModel3DrawFaceFunc J3DAPI rdModel3_RegisterFaceDraw(rdModel3DrawFaceFunc pFunc)
{
    rdModel3DrawFaceFunc pPreFunc = pfDrawFace;
    pfDrawFace = pFunc;
    return pPreFunc;
}

void J3DAPI rdModel3_BendJointAngle(rdThing* pThing, size_t jointNum, size_t axis, float angle, float rate, float secDeltaTime)
{
    RD_ASSERT(axis < 3);
    RD_ASSERT(pThing->type == RD_THING_MODEL3);
    RD_ASSERT(jointNum < pThing->data.pModel3->numHNodes);

    float* axisAngle = &((float*)&pThing->apTweakedAngles[jointNum])[axis];

    if ( secDeltaTime <= 0.0f )
    {

        *axisAngle = angle;
        return;
    }

    *axisAngle = stdMath_SmoothDamp(*axisAngle, angle, rate, secDeltaTime);
}

void J3DAPI rdModel3_BendJointPitch(rdThing* pThing, size_t jointNum, float angle, float rate, float secDeltaTime)
{
    rdModel3_BendJointAngle(pThing, jointNum, 0, angle, rate, secDeltaTime);
}

void J3DAPI rdModel3_BendJointYaw(rdThing* pThing, size_t jointNum, float angle, float rate, float secDeltaTime)
{
    rdModel3_BendJointAngle(pThing, jointNum, 1, angle, rate, secDeltaTime);
}

void J3DAPI rdModel3_BendJointRoll(rdThing* pThing, size_t jointNum, float angle, float rate, float secDeltaTime)
{
    rdModel3_BendJointAngle(pThing, jointNum, 2, angle, rate, secDeltaTime);
}

void J3DAPI rdModel3_BendJoint(rdThing* pThing, size_t jointNum, const rdVector3* targetPYR, float rate, float secDeltaTime)
{
    RD_ASSERT(pThing->type == RD_THING_MODEL3);
    RD_ASSERT(jointNum < pThing->data.pModel3->numHNodes);

    rdVector3* curPYR = &pThing->apTweakedAngles[jointNum];

    if ( secDeltaTime <= 0.0f )
    {
        *curPYR = *targetPYR;
        return;
    }

    rdVector_SmoothDamp3Acc(curPYR, targetPYR, rate, secDeltaTime);
}
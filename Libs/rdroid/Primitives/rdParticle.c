#include "rdParticle.h"
#include <j3dcore/j3dhook.h>

#include <rdroid/Engine/rdCamera.h>
#include <rdroid/Engine/rdClip.h>
#include <rdroid/Engine/rdQClip.h>
#include <rdroid/Engine/rdMaterial.h>
#include <rdroid/Main/rdroid.h>
#include <rdroid/Math/rdMath.h>
#include <rdroid/Math/rdMatrix.h>
#include <rdroid/Math/rdVector.h>
#include <rdroid/Raster/rdCache.h>
#include <rdroid/RTI/symbols.h>

#include <sith/World/sithWorld.h>

#include <std/General/stdConffile.h>
#include <std/General/stdMemory.h>
#include <std/General/stdUtil.h>

#define RDPARTICLE_MAJVER   1
#define RDPARTICLE_MINVER   0

static rdVector2 rdParticle_aFaceTexVerts[4] = { { 0.0f, 0.0f }, { 1.0f, 0.0f }, { 1.0f, 1.0f }, { 0.0f, 1.0f } };
static rdVector3 rdParticle_aFaceVerts[4];
static rdVector3 rdParticle_aTransformedVerts[RDPARTICLE_MAXVERTS];
static rdParticleLoaderFunc rdParticle_pfLoader = NULL;

void rdParticle_InstallHooks(void)
{
    J3D_HOOKFUNC(rdParticle_New);
    J3D_HOOKFUNC(rdParticle_NewEntry);
    J3D_HOOKFUNC(rdParticle_Duplicate);
    J3D_HOOKFUNC(rdParticle_Free);
    J3D_HOOKFUNC(rdParticle_FreeEntry);
    J3D_HOOKFUNC(rdParticle_LoadEntry);
    J3D_HOOKFUNC(rdParticle_Draw);
}

void rdParticle_ResetGlobals(void)
{}

rdParticleLoaderFunc J3DAPI rdParticle_RegisterLoader(rdParticleLoaderFunc pFunc)
{
    RD_ASSERTREL(pFunc != NULL);

    rdParticleLoaderFunc pPrevFunc = rdParticle_pfLoader;
    rdParticle_pfLoader = pFunc;
    return pPrevFunc;
}

rdParticle* J3DAPI rdParticle_New(size_t num, float size, rdMaterial* pMat, rdLightMode lightMode)
{
    rdParticle* pParticle = (rdParticle*)STDMALLOC(sizeof(rdParticle));
    if ( !pParticle )
    {
        RDLOG_ERROR("Error allocating memory for particle.\n");
        return NULL;
    }

    memset(pParticle, 0, sizeof(rdParticle));

    if ( rdParticle_NewEntry(pParticle, num, size, pMat, lightMode) )
    {
        stdMemory_Free(pParticle);
        return NULL;
    }

    return pParticle;
}

int J3DAPI rdParticle_NewEntry(rdParticle* pParticle, size_t num, float size, rdMaterial* pMaterial, rdLightMode lightMode)
{
    RD_ASSERTREL(num <= RDPARTICLE_MAXVERTS);

    pParticle->numVertices   = num;
    pParticle->pMaterial     = pMaterial;
    pParticle->lightningMode = lightMode;
    pParticle->size          = size;
    pParticle->sizeHalf      = size / 2.0f;
    pParticle->radius        = 0.0f;

    pParticle->aVerticies      = (rdVector3*)STDMALLOC(sizeof(rdVector3) * pParticle->numVertices);
    pParticle->aVertMatCelNums = (int*)STDMALLOC(sizeof(*pParticle->aVertMatCelNums) * pParticle->numVertices);
    pParticle->aExtraLights    = (rdVector4*)STDMALLOC(sizeof(rdVector4) * pParticle->numVertices);

    if ( !pParticle->aVerticies || !pParticle->aVertMatCelNums || !pParticle->aExtraLights )
    {
        rdParticle_FreeEntry(pParticle);
        RDLOG_ERROR("Error: Bad memory allocation for particle.\n");
        return 1;
    }

    memset(pParticle->aVerticies, 0, sizeof(rdVector3) * pParticle->numVertices);
    memset(pParticle->aVertMatCelNums, -1, sizeof(*pParticle->aVertMatCelNums) * pParticle->numVertices);
    //memset(pParticle->aExtraLights, 255, sizeof(rdVector4) * pParticle->numVertices); // TODO: verify if we have a bug here and the set value should be 1.0f instead
    for ( int i = 0; i < pParticle->numVertices; i++ )
    {
        pParticle->aExtraLights[i].x = 1.0f;
        pParticle->aExtraLights[i].y = 1.0f;
        pParticle->aExtraLights[i].z = 1.0f;
        pParticle->aExtraLights[i].w = 1.0f;
    }
    return 0;
}

rdParticle* J3DAPI rdParticle_Duplicate(const rdParticle* pOriginal)
{
    RD_ASSERTREL(pOriginal);

    // Altered: Use rdParticle_New instead of boilerplate code from rdParticle_NewEntry
    rdParticle* pParticle = rdParticle_New(pOriginal->numVertices, pOriginal->size, pOriginal->pMaterial, pOriginal->lightningMode);
    if ( !pParticle )
    {
        return NULL;
    }

    memcpy(pParticle->aVerticies, pOriginal->aVerticies, sizeof(rdVector3) * pOriginal->numVertices);
    memcpy(pParticle->aVertMatCelNums, pOriginal->aVertMatCelNums, sizeof(*pParticle->aVertMatCelNums) * pOriginal->numVertices);
    memcpy(pParticle->aExtraLights, pOriginal->aExtraLights, sizeof(rdVector4) * pOriginal->numVertices);
    return pParticle;
}

void J3DAPI rdParticle_Free(rdParticle* pParticle)
{
    if ( !pParticle )
    {
        RDLOG_ERROR("Warning: Attempt to free NULL particle ptr.\n");
        return;
    }

    rdParticle_FreeEntry(pParticle);
    stdMemory_Free(pParticle);
}

void J3DAPI rdParticle_FreeEntry(rdParticle* pParticle)
{
    if ( pParticle->aVerticies )
    {
        stdMemory_Free(pParticle->aVerticies);
        pParticle->aVerticies = NULL;
    }

    if ( pParticle->aVertMatCelNums )
    {
        stdMemory_Free(pParticle->aVertMatCelNums);
        pParticle->aVertMatCelNums = NULL;
    }

    if ( pParticle->aExtraLights )
    {
        stdMemory_Free(pParticle->aExtraLights);
        pParticle->aExtraLights = NULL;
    }
}

rdParticle* J3DAPI rdParticle_Load(char* pFilename)
{
    if ( rdParticle_pfLoader )
    {
        return rdParticle_pfLoader(pFilename);
    }

    rdParticle* pParticle = (rdParticle*)STDMALLOC(sizeof(rdParticle));
    if ( !pParticle )
    {
        RDLOG_ERROR("Error allocating memory for particle.\n");
        return NULL;
    }
    else if ( !rdParticle_LoadEntry(pFilename, pParticle) )
    {
        rdParticle_Free(pParticle);
        return NULL;
    }

    return pParticle;
}

int J3DAPI rdParticle_LoadEntry(const char* pFilename, rdParticle* pParticle)
{
    const char* pName = stdFileFromPath(pFilename);
    STD_STRCPY(pParticle->aName, pName);

    if ( !stdConffile_Open(pFilename) )
    {
        RDLOG_ERROR("Error: Invalid load filename '%s'.\n", pFilename);
        return 0;
    }

    if ( stdConffile_ScanLine(" section: %s", std_g_genBuffer, (rsize_t)sizeof(std_g_genBuffer)) != 1 )
    {
        goto syntax_error;
    }

    // TODO: Maybe return error on version issue
    int vermaj, vermin;
    if ( stdConffile_ScanLine(" par %d.%d", &vermaj, &vermin) != 2 )
    {
        RDLOG_ERROR("Warning: Invalid PAR version %d.%d for file '%s'.\n", vermaj, vermin, pFilename);
    }

    if ( vermaj != RDPARTICLE_MAJVER || vermin != RDPARTICLE_MINVER )
    {
        RDLOG_ERROR("Warning: Invalid PAR version %d.%d for file '%s'.\n", vermaj, vermin, pFilename);
    }

    if ( stdConffile_ScanLine(" size %f", &pParticle->size) != 1 )
    {
        goto syntax_error;
    }
    pParticle->sizeHalf = pParticle->size / 2.0f;

    if ( !stdConffile_ReadLine() )
    {
        goto syntax_error;
    }

    if ( stdConffile_ScanLine(" material %s", std_g_genBuffer, (rsize_t)sizeof(std_g_genBuffer)) != 1 )
    {
        goto syntax_error;
    }

    pParticle->pMaterial = rdMaterial_Load(std_g_genBuffer);
    if ( !pParticle->pMaterial )
    {
        goto syntax_error;
    }

    if ( stdConffile_ScanLine(" lightingmode %d", &pParticle->lightningMode) != 1 )
    {
        goto syntax_error;
    }

    if ( stdConffile_ScanLine(" section: %s", std_g_genBuffer, (rsize_t)sizeof(std_g_genBuffer)) != 1 )
    {
        goto syntax_error;
    }

    if ( stdConffile_ScanLine(" radius %f", &pParticle->radius) != 1 )
    {
        goto syntax_error;
    }

    if ( stdConffile_ScanLine(" insert offset %f %f %f", &pParticle->insertOffset.x, &pParticle->insertOffset.y, &pParticle->insertOffset.z) != 3 )
    {
        goto syntax_error;
    }

    int nRead = 0;
    if ( nRead = stdConffile_ScanLine(" vertices %d", &pParticle->numVertices), nRead != 1 )
    {
        if ( nRead < 0 )
        {
            goto eof_error;
        }
        goto syntax_error;
    }

    if ( pParticle->numVertices > RDPARTICLE_MAXVERTS )
    {
        RDLOG_ERROR("Error: Value out of range for '%s' in '%s'\n", stdConffile_g_aLine, pFilename);
        stdConffile_Close();
        return 0;
    }

    pParticle->aVerticies      = (rdVector3*)STDMALLOC(sizeof(rdVector3) * pParticle->numVertices);
    pParticle->aVertMatCelNums = (int*)STDMALLOC(sizeof(*pParticle->aVertMatCelNums) * pParticle->numVertices);
    pParticle->aExtraLights    = (rdVector4*)STDMALLOC(sizeof(rdVector4) * pParticle->numVertices);

    if ( !pParticle->aVerticies || !pParticle->aVertMatCelNums || !pParticle->aExtraLights )
    {
        RDLOG_ERROR("Error: Bad memory allocation for '%s' in '%s'\n", stdConffile_g_aLine, pFilename);
        stdConffile_Close();
        return 0;
    }

    rdVector3* pCurVert = pParticle->aVerticies;
    int* pCurCelNums = pParticle->aVertMatCelNums;
    for ( size_t i = 0; i < pParticle->numVertices; ++i )
    {
        int num;
        float x, y, z;
        if ( nRead = stdConffile_ScanLine(" %d: %f %f %f %d", &num, &x, &y, &z, pCurCelNums), nRead != 5 || *pCurCelNums >= pParticle->pMaterial->numCels )
        {
            if ( nRead < 0 )
            {
                goto eof_error;
            }
            goto syntax_error;
        }

        pCurVert->x = x;
        pCurVert->y = y;
        pCurVert->z = z;
        ++pCurVert;
        ++pCurCelNums;
    }

    // Success
    stdConffile_Close();
    return 1;

eof_error:
    RDLOG_ERROR("Error: Unexpected EOF in '%s'\n", pFilename);
    stdConffile_Close();
    return 0;

syntax_error:
    RDLOG_ERROR("Error: Invalid syntax '%s' in '%s'\n", stdConffile_g_aLine, pFilename);
    stdConffile_Close();
    return 0;
}

int J3DAPI rdParticle_Write(const char* pFilename, rdParticle* pParticle, const char* pCreatedName)
{
    tFileHandle fh = rdroid_g_pHS->pFileOpen(pFilename, "wt+");
    if ( !fh )
    {
        RDLOG_ERROR("Error: Could not open file for writing: '%s'\n", pFilename);
        return 0;
    }

    rdroid_g_pHS->pFilePrintf(fh, "# PAR '%s' created from '%s'\n\n", pParticle, pCreatedName);
    rdroid_g_pHS->pFilePrintf(fh, "###############\n");
    rdroid_g_pHS->pFilePrintf(fh, "SECTION: HEADER\n\n");
    rdroid_g_pHS->pFilePrintf(fh, "PAR %d.%d\n\n", RDPARTICLE_MAJVER, RDPARTICLE_MINVER);
    rdroid_g_pHS->pFilePrintf(fh, "SIZE %.6f\n\n", pParticle->size);
    rdroid_g_pHS->pFilePrintf(fh, "MATERIAL %s\n\n", pParticle->pMaterial);
    rdroid_g_pHS->pFilePrintf(fh, "LIGHTINGMODE %d\n\n", pParticle->lightningMode);
    rdroid_g_pHS->pFilePrintf(fh, "###############\n");
    rdroid_g_pHS->pFilePrintf(fh, "SECTION: GEOMETRYDEF\n\n");
    rdroid_g_pHS->pFilePrintf(fh, "# Object radius\n");
    rdroid_g_pHS->pFilePrintf(fh, "RADIUS %10.6f\n\n", pParticle->radius);
    rdroid_g_pHS->pFilePrintf(fh, "# Insertion offset\n");
    rdroid_g_pHS->pFilePrintf(fh, "INSERT OFFSET %10.6f %10.6f %10.6f\n\n", pParticle->insertOffset.x, pParticle->insertOffset.y, pParticle->insertOffset.z);
    rdroid_g_pHS->pFilePrintf(fh, "VERTICES %d\n\n", pParticle->numVertices);
    rdroid_g_pHS->pFilePrintf(fh, "# num:     x:         y:         z:       cel:\n");

    for ( size_t i = 0; i < pParticle->numVertices; ++i )
    {
        rdroid_g_pHS->pFilePrintf(
            fh,
            "  %3d: %10.6f %10.6f %10.6f %d\n",
            i,
            pParticle->aVerticies[i].x,
            pParticle->aVerticies[i].y,
            pParticle->aVerticies[i].z,
            pParticle->aVertMatCelNums[i]
        );
    }

    rdroid_g_pHS->pFilePrintf(fh, "\n\n");
    rdroid_g_pHS->pFileClose(fh);
    return 1;
}

int J3DAPI rdParticle_Draw(const rdThing* pParticle, const rdMatrix34* pOrient)
{
    if ( pParticle->frustumCull == RDFRUSTUMCULL_OUTSIDE )
    {
        return 0;
    }

    rdParticle* prdParticle = pParticle->data.pParticle;

    bool bAlpha = false;
    if ( prdParticle->pMaterial )
    {
        bAlpha = prdParticle->pMaterial->formatType == STDCOLOR_FORMAT_RGBA;
    }

    rdVector4 ambientLight = { 0 };
    if ( (rdroid_g_curRenderOptions & RDROID_USE_AMBIENT_CAMERA_LIGHT) != 0 )
    {
        rdVector_Copy4(&ambientLight, &rdCamera_g_pCurCamera->ambientLight);
    }
    ambientLight.alpha = 1.0f;

    rdMatrix34 tmat;
    rdMatrix_Multiply34(&tmat, &rdCamera_g_pCurCamera->viewMatrix, pOrient); // Combine model and view matrices
    rdMatrix_TransformPointList34(&tmat, prdParticle->aVerticies, rdParticle_aTransformedVerts, prdParticle->numVertices); // Transform verts to view space

    for ( size_t i = 0; i < prdParticle->numVertices; ++i )
    {
        rdParticle_aFaceVerts[0].x = rdParticle_aTransformedVerts[i].x - prdParticle->sizeHalf;
        rdParticle_aFaceVerts[0].y = rdParticle_aTransformedVerts[i].y;
        rdParticle_aFaceVerts[0].z = rdParticle_aTransformedVerts[i].z - prdParticle->sizeHalf;

        rdParticle_aFaceVerts[1].x = rdParticle_aTransformedVerts[i].x + prdParticle->sizeHalf;
        rdParticle_aFaceVerts[1].y = rdParticle_aTransformedVerts[i].y;
        rdParticle_aFaceVerts[1].z = rdParticle_aTransformedVerts[i].z - prdParticle->sizeHalf;

        rdParticle_aFaceVerts[2].x = rdParticle_aTransformedVerts[i].x + prdParticle->sizeHalf;
        rdParticle_aFaceVerts[2].y = rdParticle_aTransformedVerts[i].y;
        rdParticle_aFaceVerts[2].z = rdParticle_aTransformedVerts[i].z + prdParticle->sizeHalf;

        rdParticle_aFaceVerts[3].x = rdParticle_aTransformedVerts[i].x - prdParticle->sizeHalf;
        rdParticle_aFaceVerts[3].y = rdParticle_aTransformedVerts[i].y;
        rdParticle_aFaceVerts[3].z = rdParticle_aTransformedVerts[i].z + prdParticle->sizeHalf;

        // Check all verts are in frustum
        size_t numVertsInFrustum = rdQClip_VerticesInFrustrum(rdCamera_g_pCurCamera->pFrustum, rdParticle_aFaceVerts, STD_ARRAYLEN(rdParticle_aFaceVerts));
        if ( numVertsInFrustum != STD_ARRAYLEN(rdParticle_aFaceVerts) )
        {
            continue;
        }

        rdCacheProcEntry* pPoly = NULL;
        bAlpha = bAlpha && rdParticle_aFaceVerts[0].y < 1.5f;
        if ( bAlpha )
        {
            pPoly = rdCache_GetAlphaProcEntry();
        }
        else
        {
            pPoly = rdCache_GetProcEntry();
        }

        if ( !pPoly )
        {
            return 0;
        }

        rdClip_VerticesToPlane(pPoly, rdParticle_aFaceVerts, rdParticle_aFaceTexVerts, STD_ARRAYLEN(rdParticle_aFaceVerts)); // Project verts to screen space

        pPoly->aVertIntensities[0] = ambientLight;
        pPoly->aVertIntensities[1] = ambientLight;
        pPoly->aVertIntensities[2] = ambientLight;
        pPoly->aVertIntensities[3] = ambientLight;

        pPoly->extraLight.red   = 0.0f;
        pPoly->extraLight.green = 0.0f;
        pPoly->extraLight.blue  = 0.0f;
        pPoly->extraLight.alpha = 1.0f;

        pPoly->lightingMode = prdParticle->lightningMode;
        pPoly->pMaterial    = prdParticle->pMaterial;
        pPoly->matCelNum    = prdParticle->aVertMatCelNums[i];

        pPoly->extraLight = prdParticle->aExtraLights[i];

        pPoly->flags = RD_FF_FOG_ENABLED | RD_FF_TEX_CLAMP_Y | RD_FF_TEX_CLAMP_X | RD_FF_TEX_TRANSLUCENT; // TODO: The RD_FF_TEX_TRANSLUCENT is set also for non-alpha poly, verify if this is a bug

        // Fixed: Disable fog rendering for poly when fog is globally disabled
        //        OG: Poly fog rendering was enabled by default which lead to undesired render effect when fog is disabled in level (i.e.: fog color is applied)
        if ( !sithWorld_g_pCurrentWorld->fog.bEnabled ) // TODO: add special function that will enable/disable fog rendering
        {
            pPoly->flags &= ~RD_FF_FOG_ENABLED;
        }

        if ( bAlpha )
        {
            pPoly->flags |= RD_FF_ZWRITE_DISABLED;
            rdCache_AddAlphaProcFace(STD_ARRAYLEN(rdParticle_aFaceVerts));
        }
        else
        {
            rdCache_AddProcFace(STD_ARRAYLEN(rdParticle_aFaceVerts));
        }
    }

    return 1;
}

#include "sithSurface.h"
#include "sithWorld.h"

#include <j3dcore/j3dhook.h>

#include <sith/Cog/sithCog.h>
#include <sith/Devices/sithComm.h>
#include <sith/Devices/sithSoundMixer.h>
#include <sith/Dss/sithDSS.h>
#include <sith/Engine/sithIntersect.h>
#include <sith/RTI/symbols.h>

#include <rdroid/Math/rdVector.h>
#include <rdroid/Raster/rdCache.h>
#include <rdroid/Raster/rdFace.h>

#include <sound/Sound.h>
#include <sound/types.h>

#include <std/General/stdConffile.h>
#include <std/General/stdMemory.h>
#include <std/General/stdUtil.h>
#include <std/types.h>
#include <std/Win95/stdComm.h>

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

static SithSurface* sithSurface_apUnsyncedSurfaces[32] = { 0 }; // Added: Init to 0
static size_t sithSurface_numUnsyncedSurfaces = 0; // Added: Init to 0

void sithSurface_InstallHooks(void)
{
    J3D_HOOKFUNC(sithSurface_HideSectorAdjoin);
    J3D_HOOKFUNC(sithSurface_ShowSectorAdjoin);
    J3D_HOOKFUNC(sithSurface_AllocWorldSurfaces);
    J3D_HOOKFUNC(sithSurface_FreeWorldSurfaces);
    J3D_HOOKFUNC(sithSurface_WriteAdjoinsListBinary);
    J3D_HOOKFUNC(sithSurface_ReadAdjoinsListBinary);
    J3D_HOOKFUNC(sithSurface_AllocWorldAdjoins);
    J3D_HOOKFUNC(sithSurface_WriteSurfacesListBinary);
    J3D_HOOKFUNC(sithSurface_ReadSurfacesListBinary);
    J3D_HOOKFUNC(sithSurface_ReadSurfacesListText);
    J3D_HOOKFUNC(sithSurface_ValidateSurfacePointer);
    J3D_HOOKFUNC(sithSurface_ValidateWorldSurfaces);
    J3D_HOOKFUNC(sithSurface_HandleThingImpact);
    J3D_HOOKFUNC(sithSurface_PlaySurfaceHitSound);
    J3D_HOOKFUNC(sithSurface_GetCenterPoint);
    J3D_HOOKFUNC(sithSurface_SyncSurface);
    J3D_HOOKFUNC(sithSurface_SyncSurfaces);
    J3D_HOOKFUNC(sithSurface_GetSurfaceEx);
    J3D_HOOKFUNC(sithSurface_GetSurfaceIndex);
    J3D_HOOKFUNC(sithSurface_GetSurfaceIndexEx);
}

void sithSurface_ResetGlobals(void)
{}

void J3DAPI sithSurface_HideSectorAdjoin(SithSurfaceAdjoin* pAdjoin)
{
    SITH_ASSERTREL(pAdjoin);
    if ( (pAdjoin->flags & SITH_ADJOIN_VISIBLE) != 0 )
    {
        pAdjoin->flags |= SITH_ADJOIN_SECTORSET;
        pAdjoin->flags &= ~SITH_ADJOIN_VISIBLE;
    }
}

void J3DAPI sithSurface_ShowSectorAdjoin(SithSurfaceAdjoin* pAdjoin)
{
    SITH_ASSERTREL(pAdjoin);
    if ( (pAdjoin->flags & SITH_ADJOIN_SECTORSET) != 0 )
    {
        pAdjoin->flags |= SITH_ADJOIN_VISIBLE;
        pAdjoin->flags &= ~SITH_ADJOIN_SECTORSET;
    }
}

int J3DAPI sithSurface_AllocWorldSurfaces(SithWorld* pWorld, size_t numSurfaces)
{
    SITH_ASSERTREL(pWorld != NULL);
    SITH_ASSERTREL(pWorld->aSurfaces == NULL);

    const size_t sizeSurfaces = sizeof(SithSurface) * numSurfaces;
    pWorld->aSurfaces = (SithSurface*)STDMALLOC(sizeSurfaces);
    if ( !pWorld->aSurfaces )
    {
        SITHLOG_ERROR("Failed to allocate mem for %d surfaces.\n", numSurfaces);
        return 1;
    }

    memset(pWorld->aSurfaces, 0, sizeSurfaces);
    pWorld->numSurfaces = numSurfaces;

    SithSurface* pCurf = pWorld->aSurfaces;
    for ( size_t i = 0; i < numSurfaces; ++i )
    {
        rdFace_NewEntry(&pCurf->face);
        ++pCurf;
    }

    return 0;
}

void J3DAPI sithSurface_FreeWorldSurfaces(SithWorld* pWorld)
{
    SITH_ASSERTREL(pWorld != NULL);
    SITH_ASSERTREL((pWorld->aSurfaces != NULL));

    for ( size_t i = 0; i < pWorld->numSurfaces; ++i )
    {
        SithSurface* pSurf = &pWorld->aSurfaces[i];

        pSurf->face.numVertices = 0;
        if ( pSurf->face.aVertices )
        {
            stdMemory_Free(pSurf->face.aVertices);
        }

        if ( pSurf->face.aTexVertices )
        {
            stdMemory_Free(pSurf->face.aTexVertices);
        }

        if ( pSurf->aIntensities )
        {
            stdMemory_Free(pSurf->aIntensities);
        }

        pSurf->flags = 0;
    }

    stdMemory_Free(pWorld->aSurfaces);

    pWorld->aSurfaces   = NULL;
    pWorld->numSurfaces = 0;
    sithSurface_numUnsyncedSurfaces = 0;

    if ( pWorld->aAdjoins )
    {
        stdMemory_Free(pWorld->aAdjoins);
    }

    pWorld->aAdjoins   = NULL;
    pWorld->numAdjoins = 0;
}

int J3DAPI sithSurface_WriteAdjoinsListBinary(tFileHandle fh, const SithWorld* pWorld)
{
    size_t sizeAdjoins = sizeof(CndSurfaceAdjoin) * pWorld->numAdjoins;
    CndSurfaceAdjoin* aCndAdjoins = (CndSurfaceAdjoin*)STDMALLOC(sizeAdjoins);
    if ( !aCndAdjoins )
    {
        return 1;
    }

    for ( size_t i = 0; i < pWorld->numAdjoins; ++i )
    {
        SithSurfaceAdjoin* pAdjoin   = &pWorld->aAdjoins[i];
        CndSurfaceAdjoin* pCndAdjoin = &aCndAdjoins[i];

        pCndAdjoin->flags     = pAdjoin->flags;
        pCndAdjoin->mirrorIdx = pAdjoin->pMirrorAdjoin ? (pAdjoin->pMirrorAdjoin - pWorld->aAdjoins) : -1;
        pCndAdjoin->distance  = pAdjoin->distance;
    }

    size_t nWritten = sith_g_pHS->pFileWrite(fh, aCndAdjoins, sizeAdjoins);
    stdMemory_Free(aCndAdjoins);
    return nWritten != sizeAdjoins;
}

int J3DAPI sithSurface_ReadAdjoinsListBinary(tFileHandle fh, SithWorld* pWorld)
{
    size_t sizeAdjoins = sizeof(CndSurfaceAdjoin) * pWorld->numAdjoins;
    CndSurfaceAdjoin* aCndAdjoins = (CndSurfaceAdjoin*)STDMALLOC(sizeAdjoins);
    if ( !aCndAdjoins || sith_g_pHS->pFileRead(fh, aCndAdjoins, sizeAdjoins) != sizeAdjoins || sithSurface_AllocWorldAdjoins(pWorld, pWorld->numAdjoins) )
    {
        if ( aCndAdjoins )
        {
            stdMemory_Free(aCndAdjoins);
        }

        return 1;
    }

    for ( size_t i = 0; i < pWorld->numAdjoins; ++i )
    {
        SithSurfaceAdjoin* pAdjoin   = &pWorld->aAdjoins[i];
        CndSurfaceAdjoin* pCndAdjoin = &aCndAdjoins[i];

        pAdjoin->flags         = pCndAdjoin->flags;
        pAdjoin->pMirrorAdjoin = NULL;
        if ( pCndAdjoin->mirrorIdx != -1 )
        {
            pAdjoin->pMirrorAdjoin = &pWorld->aAdjoins[pCndAdjoin->mirrorIdx];
        }

        pAdjoin->distance = pCndAdjoin->distance;
    }

    stdMemory_Free(aCndAdjoins);
    return 0;
}

int J3DAPI sithSurface_AllocWorldAdjoins(SithWorld* pWorld, size_t numAdjoins)
{
    SITH_ASSERTREL(pWorld);
    SITH_ASSERTREL(!pWorld->aAdjoins);

    if ( !numAdjoins )
    {
        pWorld->aAdjoins = NULL;
        return 0;
    }

    const size_t sizeAdjoins = sizeof(SithSurfaceAdjoin) * numAdjoins;
    pWorld->aAdjoins = (SithSurfaceAdjoin*)STDMALLOC(sizeAdjoins);
    if ( !pWorld->aAdjoins )
    {
        SITHLOG_ERROR("Failed to allocate mem for %d adjoins.\n", numAdjoins);
        return 1;
    }

    memset(pWorld->aAdjoins, 0, sizeAdjoins);
    pWorld->numAdjoins = numAdjoins;
    return 0;
}

int J3DAPI sithSurface_WriteAdjoinsListText(const SithWorld* pWorld)
{
    if ( stdConffile_WriteLine("#------ Adjoins Subsection -----\n")
        || stdConffile_Printf("World adjoins %d\n", pWorld->numAdjoins)
        || stdConffile_WriteLine("#num:\tflags:\tmirror:\tdist:\n") )
    {
        return 1;
    }

    for ( size_t i = 0; i < pWorld->numAdjoins; ++i )
    {
        SithSurfaceAdjoin* pAdjoin = &pWorld->aAdjoins[i];
        int mirror = -1;
        if ( pAdjoin->pMirrorAdjoin )
        {
            mirror = pAdjoin->pMirrorAdjoin - pWorld->aAdjoins;
        }

        if ( stdConffile_Printf("%d:\t0x%x\t%d\t%.2f\n", i, pAdjoin->flags, mirror, pAdjoin->distance) )
        {
            return 1;
        }
    }

    return stdConffile_WriteLine("\n") != 0;
}

int J3DAPI sithSurface_WriteSurfacesListBinary(tFileHandle fh, const SithWorld* pWorld)
{
    size_t sizeSurfaces = sizeof(CndSurfaceInfo) * pWorld->numSurfaces;
    CndSurfaceInfo* aCndSurfaces = (CndSurfaceInfo*)STDMALLOC(sizeSurfaces);
    if ( !aCndSurfaces )
    {
        return 1;
    }

    uint32_t totalVerts = 0;
    for ( size_t i = 0; i < pWorld->numSurfaces; ++i )
    {
        SithSurface* pSurf       = &pWorld->aSurfaces[i];
        CndSurfaceInfo* pCndSurf = &aCndSurfaces[i];

        int matIdx = -1;
        if ( pSurf->face.pMaterial )
        {
            matIdx = pSurf->face.pMaterial->num;
        }

        int numMaterials;
        if ( matIdx != -1 && SITHWORLD_IS_STATICINDEX(matIdx) ) // Added: Check for matIdx != -1
        {
            numMaterials = SITHWORLD_STATICINDEX(sithWorld_g_pStaticWorld->numMaterials);
        }
        else
        {
            numMaterials = pWorld->numMaterials;
        }

        if ( matIdx != -1 && matIdx > numMaterials ) // Added: Check for matIdx != -1
        {
            matIdx = -1;
            SITHLOG_ERROR("Surface Material %s not in world.\n", pSurf->face.pMaterial->aName);
        }

        if ( matIdx == -1 && (pSurf->face.geometryMode > RD_GEOMETRY_SOLID) )
        {
            pSurf->face.geometryMode = RD_GEOMETRY_SOLID;
        }

        pCndSurf->matNum    = matIdx;
        pCndSurf->surfflags = pSurf->flags;
        pCndSurf->faceflags = pSurf->face.flags;
        pCndSurf->geoMode   = pSurf->face.geometryMode;
        pCndSurf->lightMode = pSurf->face.lightingMode;

        int adjIdx = -1;
        if ( pSurf->pAdjoin )
        {
            adjIdx = pSurf->pAdjoin - pWorld->aAdjoins;
        }

        pCndSurf->adjoinNum = adjIdx;

        rdVector_Copy4(&pCndSurf->color, &pSurf->face.extraLight);
        rdVector_Copy3(&pCndSurf->normal, &pSurf->face.normal);
        pCndSurf->numVerts = pSurf->face.numVertices;

        totalVerts += pSurf->face.numVertices;
    }

    size_t nWritten = sith_g_pHS->pFileWrite(fh, aCndSurfaces, sizeSurfaces);
    stdMemory_Free(aCndSurfaces); // Free
    if ( nWritten != sizeSurfaces )
    {
        return 1;
    }

    nWritten = sith_g_pHS->pFileWrite(fh, &totalVerts, sizeof(totalVerts)); static_assert(sizeof(totalVerts) == 4, "sizeof(totalVerts) == 4");
    if ( nWritten != 4 )
    {
        return 1;
    }

    size_t sizeVerts = sizeof(CndSurfaceVertInfo) * totalVerts;
    CndSurfaceVertInfo* aCndSurfVerts = (CndSurfaceVertInfo*)STDMALLOC(sizeVerts);
    if ( !aCndSurfVerts )
    {
        return 1;
    }

    CndSurfaceVertInfo* pCndSurfVert = aCndSurfVerts;
    for ( size_t i = 0; i < pWorld->numSurfaces; ++i )
    {
        SithSurface* pSurf = &pWorld->aSurfaces[i];
        for ( size_t j = 0; j < pSurf->face.numVertices; ++j )
        {
            pCndSurfVert->vertIdx = pSurf->face.aVertices[j];
            pCndSurfVert->texIdx = 0;
            if ( pSurf->face.aTexVertices )
            {
                pCndSurfVert->texIdx = pSurf->face.aTexVertices[j];
            }

            rdVector_Copy4(&pCndSurfVert->color, &pSurf->aIntensities[j]);
            ++pCndSurfVert;
        }
    }

    nWritten = sith_g_pHS->pFileWrite(fh, aCndSurfVerts, sizeVerts);
    stdMemory_Free(aCndSurfVerts);
    return nWritten != sizeVerts;
}

int J3DAPI sithSurface_ReadSurfacesListBinary(tFileHandle fh, SithWorld* pWorld)
{
    CndSurfaceVertInfo* aCndSurfVerts = NULL; // Added: Init to null, fixes potential freeing unallocated mem when error occurs 

    size_t sizeSurfaces = sizeof(CndSurfaceInfo) * pWorld->numSurfaces;
    CndSurfaceInfo* aCndSurfaces = (CndSurfaceInfo*)STDMALLOC(sizeSurfaces);
    if ( !aCndSurfaces )
    {
        goto error;
    }

    size_t nRead = sith_g_pHS->pFileRead(fh, aCndSurfaces, sizeSurfaces);
    if ( nRead != sizeSurfaces || sithSurface_AllocWorldSurfaces(pWorld, pWorld->numSurfaces) )
    {
        goto error;
    }

    for ( size_t i = 0; i < pWorld->numSurfaces; ++i )
    {
        SithSurface* pSurf       = &pWorld->aSurfaces[i];
        CndSurfaceInfo* pCndSurf = &aCndSurfaces[i];

        if ( pCndSurf->matNum == -1 )
        {
            pSurf->face.pMaterial = NULL;
        }
        else if ( SITHWORLD_IS_STATICINDEX(pCndSurf->matNum) )
        {
            pCndSurf->matNum = SITHWORLD_FROM_STATICINDEX(pCndSurf->matNum);
            pSurf->face.pMaterial = &sithWorld_g_pStaticWorld->aMaterials[pCndSurf->matNum];
        }
        else
        {
            pSurf->face.pMaterial = pWorld->apMatArray[pCndSurf->matNum];
        }

        pSurf->flags             = pCndSurf->surfflags;
        pSurf->face.flags        = pCndSurf->faceflags;
        pSurf->face.geometryMode = pCndSurf->geoMode;
        pSurf->face.lightingMode = pCndSurf->lightMode;

        pSurf->pAdjoin = NULL;
        if ( pCndSurf->adjoinNum != -1 )
        {
            pSurf->pAdjoin = &pWorld->aAdjoins[pCndSurf->adjoinNum];
            pSurf->pAdjoin->pAdjoinSurface = pSurf;
        }

        rdVector_Copy4(&pSurf->face.extraLight, &pCndSurf->color);
        rdVector_Copy3(&pSurf->face.normal, &pCndSurf->normal);
        pSurf->face.numVertices = pCndSurf->numVerts;
    }

    if ( aCndSurfaces )
    {
        stdMemory_Free(aCndSurfaces);
        aCndSurfaces = NULL; // Added
    }

    // Free apMatArray
    // TODO: why is array is freed?
    if ( pWorld->apMatArray )
    {
        stdMemory_Free(pWorld->apMatArray);
    }
    pWorld->apMatArray = NULL;

    size_t totalVerts;
    nRead = sith_g_pHS->pFileRead(fh, &totalVerts, sizeof(int)); static_assert(sizeof(int) == 4, "sizeof(int) == 4");
    if ( nRead != 4 )
    {
        return 1;
    }

    size_t sizeVerts = sizeof(CndSurfaceVertInfo) * totalVerts;
    aCndSurfVerts = (CndSurfaceVertInfo*)STDMALLOC(sizeVerts);
    if ( !aCndSurfVerts )
    {
        return 1;
    }

    nRead = sith_g_pHS->pFileRead(fh, aCndSurfVerts, sizeVerts);
    if ( nRead != sizeVerts )
    {
        goto error;
    }

    CndSurfaceVertInfo* pCndSurfVert = aCndSurfVerts;
    for ( size_t i = 0; i < pWorld->numSurfaces; ++i )
    {
        SithSurface* pSurf = &pWorld->aSurfaces[i];

        int* aVerts = (int*)STDMALLOC(4 * pSurf->face.numVertices);
        pSurf->face.aVertices = aVerts;
        if ( !pSurf->face.aVertices )
        {
            goto error;
        }

        int* aTexVerts = (int*)STDMALLOC(4 * pSurf->face.numVertices);
        pSurf->face.aTexVertices = aTexVerts;
        if ( !pSurf->face.aTexVertices )
        {
            goto error;
        }

        rdVector4* aVertColors = (rdVector4*)STDMALLOC(sizeof(rdVector4) * pSurf->face.numVertices);
        pSurf->aIntensities = aVertColors;
        if ( !pSurf->aIntensities )
        {
            goto error;
        }

        for ( size_t j = 0; j < pSurf->face.numVertices; ++j )
        {
            pSurf->face.aVertices[j]    = pCndSurfVert->vertIdx;
            pSurf->face.aTexVertices[j] = pCndSurfVert->texIdx;
            rdVector_Copy4(&pSurf->aIntensities[j], &pCndSurfVert->color);
            ++pCndSurfVert;
        }
    }

    // Success
    stdMemory_Free(aCndSurfVerts);
    return 0;

error:
    if ( aCndSurfaces )
    {
        stdMemory_Free(aCndSurfaces);
    }

    if ( aCndSurfVerts )
    {
        stdMemory_Free(aCndSurfVerts);
    }

    return 1;
}

int J3DAPI sithSurface_WriteSurfacesListText(const SithWorld* pWorld)
{
    if ( stdConffile_WriteLine("#----- Surfaces Subsection -----\n")
        || stdConffile_Printf("World surfaces %d\n", pWorld->numSurfaces)
        || stdConffile_WriteLine("#num:\tmat:\tsurfflags:\tfaceflags:\tgeo:\tlight:\ttex:\tadjoin:\tExLR:\tExLG:\tExLB:\tnverts:\tvertices:\t\t\tiR:\tiG\tiB\n") != 0 )
    {
        return 1;
    }

    for ( size_t i = 0; i < pWorld->numSurfaces; ++i )
    {
        SithSurface* pSurf = &pWorld->aSurfaces[i];

        int matIdx = -1;
        if ( pSurf->face.pMaterial )
        {
            matIdx = pSurf->face.pMaterial->num;
        }

        int numMaterials = pWorld->numMaterials;
        if ( matIdx != -1 && SITHWORLD_IS_STATICINDEX(matIdx) ) // Added: Check for matIdx != -1
        {
            numMaterials = SITHWORLD_STATICINDEX(sithWorld_g_pStaticWorld->numMaterials);
        }

        if ( matIdx != -1 && matIdx > numMaterials ) // Added: Check for matIdx != -1
        {
            SITHLOG_ERROR("Surface Material %s not in world.\n", pSurf->face.pMaterial->aName);

            pSurf->face.pMaterial = NULL; // TODO: why set to NULL here?
            matIdx = -1;
        }

        if ( matIdx == -1 && pSurf->face.geometryMode > RD_GEOMETRY_SOLID )
        {
            pSurf->face.geometryMode  = RD_GEOMETRY_SOLID;
        }

        int  adjIdx = -1;
        if ( pSurf->pAdjoin )
        {
            adjIdx = pSurf->pAdjoin - pWorld->aAdjoins;
        }

        if ( stdConffile_Printf(
            "%d:\t%d\t0x%x\t\t0x%x\t\t%d\t%d\t%d\t%d\t%.2f\t%.2f\t%.2f\t%.2f\t\t%d\t",
            i,
            matIdx,
            pSurf->flags,
            pSurf->face.flags,
            pSurf->face.geometryMode,
            pSurf->face.lightingMode,
            3, // texmode
            adjIdx,
            pSurf->face.extraLight.red,
            pSurf->face.extraLight.green,
            pSurf->face.extraLight.blue,
            pSurf->face.extraLight.alpha,
            pSurf->face.numVertices) != 0 )
        {
            return 1;
        }

        for ( size_t j = 0; j < pSurf->face.numVertices; ++j )
        {
            if ( pSurf->face.geometryMode < RD_GEOMETRY_FULL )
            {
                if ( stdConffile_Printf("%d,-1\t", pSurf->face.aVertices[j]) != 0 )
                {
                    return 1;
                }
            }
            else
            {
                SITH_ASSERTREL(pSurf->face.pMaterial != NULL);
                SITH_ASSERTREL(pSurf->face.aTexVertices != NULL);
                if ( stdConffile_Printf("%d,%d\t", pSurf->face.aVertices[j], pSurf->face.aTexVertices[j]) != 0 )
                {
                    return 1;
                }
            }
        }

        for ( size_t k = 0; k < pSurf->face.numVertices; ++k )
        {
            if ( stdConffile_Printf("%f\t%f\t%f\t", pSurf->aIntensities[k].red, pSurf->aIntensities[k].green, pSurf->aIntensities[k].blue) != 0 )
            {
                return 1;
            }
        }

        if ( stdConffile_WriteLine("\n") != 0 )
        {
            return 1;
        }
    }

    if ( stdConffile_WriteLine("\n#--- Surface normals ---\n") != 0 )
    {
        return 1;
    }

    // Write face normals
    for ( size_t i = 0; i < pWorld->numSurfaces; i++ )
    {
        rdFace* pFace = &pWorld->aSurfaces[i].face;
        if ( stdConffile_Printf("%d:\t%f\t%f\t%f\n", i, pFace->normal.x, pFace->normal.y, pFace->normal.z) != 0 )
        {
            return 1;
        }
    }

    return 0;
}

int J3DAPI sithSurface_ReadSurfacesListText(SithWorld* pWorld, int bSkip)
{
    J3D_UNUSED(bSkip);

    SITH_ASSERTREL(pWorld != NULL);
    SITH_ASSERTREL(pWorld->apMatArray != NULL);

    // Parse adjoins

    int nRead = 0;
    size_t numAdjoins;
    if ( nRead = stdConffile_ScanLine(" world adjoins %d", &numAdjoins), nRead != 1 )
    {
        if ( nRead < 0 )
        {
            goto eof_error;
        }
        goto syntax_error;
    }

    if ( sithSurface_AllocWorldAdjoins(pWorld, numAdjoins) )
    {
        goto alloc_error;
    }

    for ( size_t i = 0; i < numAdjoins; i++ )
    {
        if ( !stdConffile_ReadArgs() )
        {
            goto eof_error;
        }

        int curArg = 1;
        SithSurfaceAdjoin* pAdjoin = &pWorld->aAdjoins[i];
        if ( sscanf_s(stdConffile_g_entry.aArgs[curArg++].argValue, "%x", &pAdjoin->flags) != 1 )
        {
            goto syntax_error;
        }

        size_t mirrorIdx = atoi(stdConffile_g_entry.aArgs[curArg++].argValue);
        if ( mirrorIdx >= pWorld->numAdjoins )
        {
            SITHLOG_ERROR("Adjoin mirror index out of range!\n");
            goto range_error;
        }

        pAdjoin->pMirrorAdjoin = &pWorld->aAdjoins[mirrorIdx];
        pAdjoin->distance      = strtof(stdConffile_g_entry.aArgs[curArg++].argValue, NULL);
    }

    // Parse surface

    size_t numSurfaces;
    if ( nRead = stdConffile_ScanLine(" world surfaces %d", &numSurfaces), nRead != 1 )
    {
        if ( nRead < 0 )
        {
            goto eof_error;
        }
        goto syntax_error;
    }

    if ( sithSurface_AllocWorldSurfaces(pWorld, numSurfaces) )
    {
        goto alloc_error;
    }

    for ( size_t i = 0; i < numSurfaces; i++ )
    {
        SithSurface* pSurf = &pWorld->aSurfaces[i];

        if ( !stdConffile_ReadArgs() )
        {
            goto eof_error;
        }

        rdFace* pFace = &pSurf->face;
        pFace->pMaterial = NULL;

        int curArg = 1;
        int matIdx = atoi(stdConffile_g_entry.aArgs[curArg++].argValue);
        if ( matIdx >= 0 )
        {
            if ( SITHWORLD_IS_STATICINDEX(matIdx) )
            {
                matIdx = SITHWORLD_FROM_STATICINDEX(matIdx);
                if ( matIdx >= sithWorld_g_pStaticWorld->numMaterials )
                {
                    SITHLOG_ERROR("Surface material index out of range!\n");
                    goto range_error;
                }

                pSurf->face.pMaterial = &sithWorld_g_pStaticWorld->aMaterials[matIdx]; // Why not grabbed from apMatArray??
            }
            else
            {
                if ( matIdx >= pWorld->sizeMaterials )
                {
                    SITHLOG_ERROR("Surface material index out of range!\n");
                    goto range_error;
                }

                pFace->pMaterial = pWorld->apMatArray[matIdx];
            }
        }

        // surfflags
        if ( sscanf_s(stdConffile_g_entry.aArgs[curArg++].argValue, "%x", &pSurf->flags) != 1 )
        {
            goto syntax_error;
        }

        // faceflags
        if ( sscanf_s(stdConffile_g_entry.aArgs[curArg++].argValue, "%x", &pFace->flags) != 1 )
        {
            goto syntax_error;
        }

        // geomode
        pFace->geometryMode = atoi(stdConffile_g_entry.aArgs[curArg++].argValue);
        if ( !pFace->pMaterial && pFace->geometryMode > RD_GEOMETRY_SOLID )
        {
            pFace->geometryMode = RD_GEOMETRY_SOLID;
        }

        // lightmode
        pFace->lightingMode = atoi(stdConffile_g_entry.aArgs[curArg++].argValue);;
        if ( (pSurf->flags & (SITH_SURFACE_CEILINGSKY | SITH_SURFACE_HORIZONSKY)) != 0 )
        {
            pFace->lightingMode = RD_LIGHTING_NONE;
        }

        // Skip tex mode
        // Removed: atoi(stdConffile_g_entry.aArgs[curArg++].argValue);
        curArg++;

        // Adjoin
        pSurf->pAdjoin = NULL;
        int adjIdx = atoi(stdConffile_g_entry.aArgs[curArg++].argValue);
        // Fixed: .ndy adjoin indices are file data, so reject invalid indices before indexing aAdjoins.
        if ( adjIdx < -1 || (adjIdx >= 0 && (size_t)adjIdx >= pWorld->numAdjoins) )
        {
            SITHLOG_ERROR("Surface adjoin index out of range!\n");
            goto range_error;
        }

        if ( adjIdx != -1 )
        {
            pSurf->pAdjoin = &pWorld->aAdjoins[adjIdx];
            pSurf->pAdjoin->pAdjoinSurface = pSurf;

            if ( (pSurf->pAdjoin->flags & SITH_ADJOIN_VISIBLE) == 0 || pFace->pMaterial
                && pFace->geometryMode
                && (pFace->flags & RD_FF_TEX_TRANSLUCENT) == 0
                && pFace->pMaterial->formatType == STDCOLOR_FORMAT_RGB )
            {
                pSurf->pAdjoin->flags |= SITH_ADJOIN_UNKNOWN_80; // Probably solid
            }
        }

        // Extra light
        pFace->extraLight.red   = strtof(stdConffile_g_entry.aArgs[curArg++].argValue, NULL);
        pFace->extraLight.green = strtof(stdConffile_g_entry.aArgs[curArg++].argValue, NULL);
        pFace->extraLight.blue  = strtof(stdConffile_g_entry.aArgs[curArg++].argValue, NULL);
        pFace->extraLight.alpha = strtof(stdConffile_g_entry.aArgs[curArg++].argValue, NULL);

        // num verts
        size_t numVerts = atoi(stdConffile_g_entry.aArgs[curArg++].argValue);
        if ( numVerts < 3 || numVerts > stdConffile_g_entry.numArgs - curArg )
        {
            goto syntax_error;
        }

        if ( numVerts > RDCACHE_MAXFACEVERTICES ) // Changed: Set to RDCACHE_MAXFACEVERTICES, was fixed 64
        {
            goto range_error;
        }

        // Alloc vert related buffers
        int* aVerts = (int*)STDMALLOC(sizeof(int) * numVerts);
        pFace->aVertices = aVerts;
        if ( !pFace->aVertices )
        {
            goto alloc_error;
        }

        pSurf->aIntensities = (rdVector4*)STDMALLOC(sizeof(rdVector4) * numVerts);
        if ( !pSurf->aIntensities )
        {
            goto alloc_error;
        }

        pFace->aTexVertices = (int*)STDMALLOC(sizeof(int) * numVerts);;
        if ( !pFace->aTexVertices )
        {
            goto alloc_error;
        }

        // parse vert/tex vert idxs
        for ( size_t j = 0; j < numVerts; ++j )
        {
            int vertIdx = atoi(stdConffile_g_entry.aArgs[curArg++].argValue);
            if ( vertIdx < 0 || (size_t)vertIdx >= pWorld->numVertices )
            {
                SITHLOG_ERROR("Surface vertex index out of range!\n");
                goto range_error;
            }

            pFace->aVertices[j] = vertIdx;

            if ( pFace->pMaterial )
            {
                int texVertIdx = atoi(stdConffile_g_entry.aArgs[curArg++].argValue);
                // Fixed: Validate serialized texture vertex indices before storing them on the face.
                if ( texVertIdx < 0 || (size_t)texVertIdx >= pWorld->numTexVertices )
                {
                    SITHLOG_ERROR("Surface texture vertex index out of range!\n");
                    goto range_error;
                }

                pFace->aTexVertices[j] = texVertIdx;
            }
            else
            {
                ++curArg; // Skip UV vert arg
                pFace->aTexVertices[j] = 0;
            }
        }

        // vert colors
        for ( size_t j = 0; j < numVerts; ++j )
        {

            pSurf->aIntensities[j].red   = strtof(stdConffile_g_entry.aArgs[curArg++].argValue, NULL);
            pSurf->aIntensities[j].green = strtof(stdConffile_g_entry.aArgs[curArg++].argValue, NULL);
            pSurf->aIntensities[j].blue  = strtof(stdConffile_g_entry.aArgs[curArg++].argValue, NULL);
            pSurf->aIntensities[j].alpha = pFace->extraLight.alpha;
        }

        pFace->num = i;
        pFace->numVertices = numVerts;
    }

    // Read face normals
    for ( size_t i = 0; i < numSurfaces; ++i )
    {
        int normNum;
        float nx, ny, nz;
        if ( nRead = stdConffile_ScanLine("%d: %f %f %f", &normNum, &nx, &ny, &nz), nRead != 4 )
        {
            if ( nRead < 0 )
            {
                goto eof_error;
            }
            goto syntax_error;
        }

        rdFace* pFace = &pWorld->aSurfaces[i].face;
        pFace->normal.x = nx;
        pFace->normal.y = ny;
        pFace->normal.z = nz;
    }

    pWorld->numSurfaces = numSurfaces;

    // TODO: Why apMatArray is being freed here?
    if ( pWorld->apMatArray )
    {
        stdMemory_Free(pWorld->apMatArray);
    }

    // Success
    pWorld->apMatArray = NULL;
    return 0;

alloc_error:
    SITHLOG_ERROR("Error: Bad memory allocation for '%s' in '%s'\n", stdConffile_g_aLine, stdConffile_GetFilename());
    return 1;

eof_error:
    SITHLOG_ERROR("Error: Unexpected EOF in '%s'\n", stdConffile_GetFilename());
    return 1;

range_error:
    SITHLOG_ERROR("Error: Value out of range for '%s' in '%s'\n", stdConffile_g_aLine, stdConffile_GetFilename());
    return 1;

syntax_error:
    SITHLOG_ERROR("Error: Invalid syntax '%s' in '%s'\n", stdConffile_g_aLine, stdConffile_GetFilename());
    return 1;
}

int J3DAPI sithSurface_ValidateSurfacePointer(const SithSurface* pSurf)
{
    return pSurf && pSurf->pSector != NULL;
}

int J3DAPI sithSurface_ValidateWorldSurfaces(const SithWorld* pWorld)
{
    SITH_ASSERTREL(pWorld != NULL);

    for ( size_t i = 0; i < pWorld->numSurfaces; ++i )
    {
        if ( !sithSurface_ValidateSurfacePointer(&pWorld->aSurfaces[i]) )
        {
            return 0;
        }
    }

    return 1;
}

void J3DAPI sithSurface_HandleThingImpact(SithSurface* pSurf, SithThing* pThing, float damage, int damageType)
{
    // Fixed: Fixed check for not null pThing
    if ( pThing && (!stdComm_IsGameActive() || (pThing->flags & SITH_TF_REMOTE) == 0) )
    {
        if ( pThing->type == SITH_THING_WEAPON && (pThing->thingInfo.weaponInfo.flags & SITH_WF_IMPACTSOUND) != 0 )
        {
            sithSurface_PlaySurfaceHitSound(pSurf, pThing, damageType);
        }

        if ( (pSurf->flags & SITH_SURFACE_COGLINKED) != 0 )
        {
            sithCog_SurfaceSendMessageEx(pSurf, pThing, SITHCOG_MSG_DAMAGED, (int32_t)damage, damageType, 0, 0);
        }
    }
}

void J3DAPI sithSurface_PlaySurfaceHitSound(const SithSurface* pSurf, SithThing* pThing, int damageType)
{
    if ( (damageType & SITH_DAMAGE_MACHETE) != 0 )
    {
        tSoundHandle hSnd;
        if ( (pSurf->flags & SITH_SURFACE_METAL) != 0 )
        {
            hSnd = Sound_GetSoundHandle(SITHWORLD_STATICINDEX(82)); // gen_machete_hit_metal.wav
            sithSoundMixer_PlaySoundPos(hSnd, &pThing->pos, pThing->pInSector, 1.0f, 0.5f, 2.5f, SOUNDPLAY_ABSOLUTE_POS);
        }
        else if ( (pSurf->flags & (SITH_SURFACE_SNOW | SITH_SURFACE_EARTH | SITH_SURFACE_EARTHECHO)) != 0 )
        {
            hSnd = Sound_GetSoundHandle(SITHWORLD_STATICINDEX(84)); // gen_machete_hit_vine.wav
            sithSoundMixer_PlaySoundPos(hSnd, &pThing->pos, pThing->pInSector, 1.0f, 0.5f, 2.5f, SOUNDPLAY_ABSOLUTE_POS);
        }
        else if ( (pSurf->flags & SITH_SURFACE_WEB) != 0 )
        {
            hSnd = Sound_GetSoundHandle(SITHWORLD_STATICINDEX(85)); // gen_machete_hit_web.wav
            sithSoundMixer_PlaySoundPos(hSnd, &pThing->pos, pThing->pInSector, 1.0f, 0.5f, 2.5f, SOUNDPLAY_ABSOLUTE_POS);
        }
        else
        {
            if ( (pSurf->flags & (SITH_SURFACE_WOODECHO | SITH_SURFACE_WOOD)) != 0 )
            {
                hSnd = Sound_GetSoundHandle(SITHWORLD_STATICINDEX(83)); // 0x8053
            }
            else
            {
                hSnd = Sound_GetSoundHandle(SITHWORLD_STATICINDEX(81)); // 0x8051
            }

            sithSoundMixer_PlaySoundPos(hSnd, &pThing->pos, pThing->pInSector, 1.0f, 0.5f, 2.5f, SOUNDPLAY_ABSOLUTE_POS);
        }
    }
#ifdef J3D_QOL_IMPROVEMENTS
    else if ( pThing->type == SITH_THING_WEAPON // Just making sure we have weapon
        && (damageType & SITH_DAMAGE_IMPACT) != 0
        && (pThing->thingInfo.weaponInfo.flags & SITH_WF_SURFACERICOCHET) == 0 ) // Make sure ricochet flag is off as it should already handle sfx
    {
        if ( (pSurf->flags & SITH_SURFACE_METAL) != 0 )
        {
            uint32_t sndIdx = rand() % 2 + 141; // gen_ricochet_a.wav or gen_ricochet_b.wav
            tSoundHandle hSnd = Sound_GetSoundHandle(SITHWORLD_STATICINDEX(sndIdx));
            sithSoundMixer_PlaySoundPos(hSnd, &pThing->pos, pThing->pInSector, 1.0f, 0.15f, 5.8f, SOUNDPLAY_ABSOLUTE_POS);
        }
        else if ( (pSurf->flags & SITH_SURFACE_SNOW) != 0 )
        {
            tSoundHandle  hSnd = Sound_GetSoundHandle(SITHWORLD_STATICINDEX(110)); // fol_in_jumpsnow.wav
            sithSoundMixer_PlaySoundPos(hSnd, &pThing->pos, pThing->pInSector, 1.0f, 0.15f, 5.8f, SOUNDPLAY_ABSOLUTE_POS);
        }
        else if ( (pSurf->flags & (SITH_SURFACE_EARTH | SITH_SURFACE_EARTHECHO)) != 0 )
        {
            tSoundHandle  hSnd = Sound_GetSoundHandle(SITHWORLD_STATICINDEX(21)); // fol_in_rrunearth.wav
            sithSoundMixer_PlaySoundPos(hSnd, &pThing->pos, pThing->pInSector, 1.0f, 0.15f, 5.8f, SOUNDPLAY_ABSOLUTE_POS);
        }
        else if ( (pSurf->flags & (SITH_SURFACE_WATER | SITH_SURFACE_SHALLOWWATER)) != 0 )
        {
            tSoundHandle hSnd = Sound_GetSoundHandle(SITHWORLD_STATICINDEX(26)); // fol_in_lwalkwater.wav
            sithSoundMixer_PlaySoundPos(hSnd, &pThing->pos, pThing->pInSector, 1.0f, 0.15f, 5.8f, SOUNDPLAY_ABSOLUTE_POS);
        }
        else if ( (pSurf->flags & SITH_SURFACE_WEB) != 0 )
        {
            tSoundHandle hSnd = Sound_GetSoundHandle(SITHWORLD_STATICINDEX(85)); // gen_machete_hit_web.wav
            sithSoundMixer_PlaySoundPos(hSnd, &pThing->pos, pThing->pInSector, 1.0f, 0.15f, 5.8f, SOUNDPLAY_ABSOLUTE_POS);
        }
        else if ( (pSurf->flags & (SITH_SURFACE_WOODECHO | SITH_SURFACE_WOOD)) != 0 )
        {
            tSoundHandle hSnd = Sound_GetSoundHandle(SITHWORLD_STATICINDEX(83)); // gen_machete_hit_wood.wav
            sithSoundMixer_PlaySoundPos(hSnd, &pThing->pos, pThing->pInSector, 1.0f, 0.15f, 5.8f, SOUNDPLAY_ABSOLUTE_POS);
        }
        else if ( (pSurf->flags & (SITH_SURFACE_LAVA)) != 0 )
        {
            // No soundFX atm
        }
        else
        {
            uint32_t sndIdx = rand() % 2 + 141; // gen_ricochet_a.wav or gen_ricochet_b.wav
            tSoundHandle hSnd = Sound_GetSoundHandle(SITHWORLD_STATICINDEX(sndIdx));
            sithSoundMixer_PlaySoundPos(hSnd, &pThing->pos, pThing->pInSector, 1.0f, 0.15f, 5.8f, SOUNDPLAY_ABSOLUTE_POS);
        }
    }
#endif
}

int J3DAPI sithSurface_GetCenterPoint(const SithSurface* pSurface, rdVector3* centerpoint)
{
    rdVector3 vertSum = { 0 };
    for ( size_t i = 0; i < pSurface->face.numVertices; ++i )
    {
        rdVector_Add3Acc(&vertSum, &sithWorld_g_pCurrentWorld->aVertices[pSurface->face.aVertices[i]]);
    }

    // Divide by num verts
    float scale = (float)pSurface->face.numVertices;
    rdVector_InvScale3(centerpoint, &vertSum, scale);

    if ( !sithIntersect_IsSphereInSector(sithWorld_g_pCurrentWorld, centerpoint, 0.0f, pSurface->pSector) )
    {
        rdVector3 offset;
        offset.x = pSurface->face.normal.x * 0.00019999999f; // / 5000
        offset.y = pSurface->face.normal.y * 0.00019999999f;
        offset.z = pSurface->face.normal.z * 0.00019999999f;
        rdVector_Add3Acc(centerpoint, &offset);
    }

    int bValid = sithIntersect_IsSphereInSector(sithWorld_g_pCurrentWorld, centerpoint, 0.0f, pSurface->pSector);
    if ( !bValid )
    {
        SITHLOG_ERROR("Cannot place surface centerpoint in parent sector.\n");
    }

    return bValid;
}

void J3DAPI sithSurface_SyncSurface(SithSurface* pSurface)
{
    SITH_ASSERTREL(pSurface);

    pSurface->flags |= SITH_SURFACE_SYNC;
    if ( sithSurface_numUnsyncedSurfaces < STD_ARRAYLEN(sithSurface_apUnsyncedSurfaces) )
    {
        sithSurface_apUnsyncedSurfaces[sithSurface_numUnsyncedSurfaces++] = pSurface;
    }
}

void sithSurface_SyncSurfaces(void)
{
    if ( sithMessage_g_outputstream && sithSurface_numUnsyncedSurfaces )
    {
        for ( size_t i = 0; i < sithSurface_numUnsyncedSurfaces; ++i )
        {
            sithDSS_SurfaceStatus(sithSurface_apUnsyncedSurfaces[i], SITHMESSAGE_SENDTOJOINEDPLAYERS, SITHMESSAGE_STREAM_ALL);
        }

        sithSurface_numUnsyncedSurfaces = 0;
    }
}

SithSurface* J3DAPI sithSurface_GetSurfaceEx(const SithWorld* pWorld, int surfIdx)
{
    if ( pWorld && surfIdx >= 0 && surfIdx < pWorld->numSurfaces )
    {
        return &pWorld->aSurfaces[surfIdx];
    }
    return NULL;
}

int J3DAPI sithSurface_GetSurfaceIndex(const SithSurface* pSurf)
{
    if ( pSurf >= sithWorld_g_pCurrentWorld->aSurfaces && pSurf <= &sithWorld_g_pCurrentWorld->aSurfaces[sithWorld_g_pCurrentWorld->numSurfaces] )
    {
        return pSurf - sithWorld_g_pCurrentWorld->aSurfaces;
    }
    return -1;
}

int J3DAPI sithSurface_GetSurfaceIndexEx(const SithWorld* pWorld, const SithSurface* pSurface)
{
    if ( pSurface >= pWorld->aSurfaces && pSurface <= &pWorld->aSurfaces[pWorld->numSurfaces] )
    {
        return pSurface - pWorld->aSurfaces;
    }
    return -1;
}

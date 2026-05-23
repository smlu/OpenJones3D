#include "sithSector.h"
#include <j3dcore/j3dhook.h>
#include <sith/RTI/symbols.h>

#include <rdroid/Math/rdVector.h>

#include <sith/Devices/sithComm.h>
#include <sith/Devices/sithSound.h>
#include <sith/Dss/sithDSS.h>
#include <sith/Engine/sithIntersect.h>
#include <sith/World/sithSurface.h>
#include <sith/World/sithWorld.h>
#include <sith/World/sithThing.h>

#include <sound/Sound.h>

#include <std/General/stdConffile.h>
#include <std/General/stdMemory.h>
#include <std/General/stdUtil.h>

static SithSector* sithSector_aModifiedSectors[16] = { 0 }; // Added Init to 0
static int sithSector_aSyncFlags[16];
static size_t sithSector_numModifiedSectors = 0; // Added Init to 0

void sithSector_InstallHooks(void)
{
    J3D_HOOKFUNC(sithSector_ReadSectorsListText);
    J3D_HOOKFUNC(sithSector_WriteSectorsListBinary);
    J3D_HOOKFUNC(sithSector_ReadSectorsListBinary);
    J3D_HOOKFUNC(sithSector_ValidateSectorPointer);
    J3D_HOOKFUNC(sithSector_AllocWorldSectors);
    J3D_HOOKFUNC(sithSector_ResetAllSectors);
    J3D_HOOKFUNC(sithSector_FreeWorldSectors);
    J3D_HOOKFUNC(sithSector_FindSectorAtPos);
    J3D_HOOKFUNC(sithSector_HideSectorAdjoins);
    J3D_HOOKFUNC(sithSector_ShowSectorAdjoins);
    J3D_HOOKFUNC(sithSector_SetSectorFlags);
    J3D_HOOKFUNC(sithSector_ClearSectorFlags);
    J3D_HOOKFUNC(sithSector_GetSectorThingCount);
    J3D_HOOKFUNC(sithSector_GetSectorPlayerCount);
    J3D_HOOKFUNC(sithSector_SyncSector);
    J3D_HOOKFUNC(sithSector_SyncSectors);
    J3D_HOOKFUNC(sithSector_GetSectorEx);
    J3D_HOOKFUNC(sithSector_GetSectorIndexEx);
    J3D_HOOKFUNC(sithSector_GetSectorIndex);
}

void sithSector_ResetGlobals(void)
{}

int J3DAPI sithSector_WriteSectorsListText(const SithWorld* pWorld)
{
    if ( stdConffile_WriteLine("###### Sector information ######\n")
        || stdConffile_WriteLine("Section: SECTORS\n\n")
        || stdConffile_Printf("World sectors %d\n\n", pWorld->numSectors) )
    {
        return 1;
    }

    for ( size_t i = 0; i < pWorld->numSectors; ++i )
    {
        SithSector* pSec = &pWorld->aSectors[i];
        if ( stdConffile_Printf(
            "\nSECTOR\t%d\nFLAGS\t0x%x\nAMBIENT LIGHT\t%.2f %.2f %.2f\nEXTRA LIGHT\t%.2f %.2f %.2f\n",
            i,
            pSec->flags,
            pSec->ambientLight.red,
            pSec->ambientLight.green,
            pSec->ambientLight.blue,
            pSec->extraLight.red,
            pSec->extraLight.green,
            pSec->extraLight.blue)
            || stdConffile_Printf("TINT\t%.2f\t%.2f\t%.2f\n", pSec->tint.red, pSec->tint.green, pSec->tint.blue)
            || stdConffile_Printf("AVERAGE LIGHT INTENSITY\t%.4f\t%.4f\t%.4f\n", pSec->light.color.red, pSec->light.color.green, pSec->light.color.blue)
            || stdConffile_Printf("AVERAGE LIGHT POSITION\t%f\t%f\t%f\n", pSec->light.pos.x, pSec->light.pos.y, pSec->light.pos.z)
            || stdConffile_Printf("AVERAGE LIGHT FALLOFF\t%f\t%f\n", pSec->light.minRadius, pSec->light.maxRadius)
            || stdConffile_Printf(
                "BOUNDBOX\t%f %f %f %f %f %f\n", pSec->boundBox.v0.x, pSec->boundBox.v0.y, pSec->boundBox.v0.z, pSec->boundBox.v1.x, pSec->boundBox.v1.y, pSec->boundBox.v1.z)
            || (pSec->flags & SITH_SECTOR_HASCOLLIDEBOX) != 0
            && stdConffile_Printf("COLLIDEBOX\t%f %f %f %f %f %f\n", pSec->collideBox.v0.x, pSec->collideBox.v0.y, pSec->collideBox.v0.z, pSec->collideBox.v1.x, pSec->collideBox.v1.y, pSec->collideBox.v1.z) )
        {
            return 1;
        }

        if ( pSec->hAmbientSound )
        {
            const char* pSoundFilename = Sound_GetSoundFilename(pSec->hAmbientSound);
            if ( stdConffile_Printf("SOUND\t%s %f\n", pSoundFilename, pSec->ambientSoundVolume) )
            {
                return 1;
            }
        }

        if ( (pSec->thrust.x != 0.0f || pSec->thrust.y != 0.0f || pSec->thrust.z != 0.0f)
            && stdConffile_Printf("THRUST\t%f %f %f\n", pSec->thrust.x, pSec->thrust.y, pSec->thrust.z) )
        {
            return 1;
        }

        if ( stdConffile_Printf("CENTER\t%f %f %f\n", pSec->center.x, pSec->center.y, pSec->center.z)
            || stdConffile_Printf("RADIUS\t%f\n", pSec->radius)
            || stdConffile_Printf("VERTICES\t%d\n", pSec->numVertices) )
        {
            return 1;
        }

        for ( size_t j = 0; j < pSec->numVertices; ++j )
        {
            if ( stdConffile_Printf("%d:\t%d\n", j, pSec->aVertIdxs[j]) )
            {
                return 1;
            }
        }

        int surfIdx = sithSurface_GetSurfaceIndexEx(pWorld, pSec->pFirstSurface);
        if ( stdConffile_Printf("SURFACES\t%d\t%d\n\n", surfIdx, pSec->numSurfaces) )
        {
            return 1;
        }
    }

    return 0;
}

int J3DAPI sithSector_ReadSectorsListText(SithWorld* pWorld, int bSkip)
{
    if ( bSkip )
    {
        return 1;
    }

    int nRead;
    size_t numSectors;
    if ( nRead = stdConffile_ScanLine(" world sectors %d", &numSectors), nRead != 1 )
    {
        if ( nRead < 0 ) {
            goto eof_error;
        }
        goto syntax_error;
    }

    if ( sithSector_AllocWorldSectors(pWorld, numSectors) )
    {
        goto alloc_error;
    }

    for ( size_t i = 0; i < numSectors; i++ )
    {
        SithSector* pSec = &pWorld->aSectors[i];

        int secNum;
        if ( nRead = stdConffile_ScanLine(" sector %d", &secNum), nRead != 1 )
        {
            if ( nRead < 0 ) {
                goto eof_error;
            }
            goto syntax_error;
        }

        // flags
        if ( nRead = stdConffile_ScanLine(" flags %x", &pSec->flags), nRead != 1 )
        {
            if ( nRead < 0 ) {
                goto eof_error;
            }
            goto syntax_error;
        }

        // ambient light
        float red, green, blue;
        if ( nRead = stdConffile_ScanLine(" ambient light %f %f %f", &red, &green, &blue), nRead != 3 )
        {
            if ( nRead < 0 ) {
                goto eof_error;
            }
            goto syntax_error;
        }

        pSec->ambientLight.red   = red;
        pSec->ambientLight.green = green;
        pSec->ambientLight.blue  = blue;
        pSec->ambientLight.alpha = 0.0f;

        // Extra light
        if ( nRead = stdConffile_ScanLine(" extra light %f %f %f", &red, &green, &blue), nRead != 3 )
        {
            if ( nRead < 0 ) {
                goto eof_error;
            }
            goto syntax_error;
        }

        pSec->extraLight.red   = red;
        pSec->extraLight.green = green;
        pSec->extraLight.blue  = blue;
        pSec->extraLight.alpha = 0.0f;

        // Optional tint
        if ( !stdConffile_ReadLine()
            || sscanf_s(stdConffile_g_aLine, " tint %f %f %f", &pSec->tint.red, &pSec->tint.green, &pSec->tint.blue) == 3 && !stdConffile_ReadLine() )
        {
            goto eof_error;
        }

        if ( sscanf_s(stdConffile_g_aLine, " average light intensity %f %f %f", &pSec->light.color.red, &pSec->light.color.green, &pSec->light.color.blue) == 3
            && !stdConffile_ReadLine() )
        {
            goto eof_error;
        }

        // Optional average light position
        if ( sscanf_s(stdConffile_g_aLine, " average light position %f %f %f", &pSec->light.pos.x, &pSec->light.pos.y, &pSec->light.pos.z) == 3
            && !stdConffile_ReadLine() )
        {
            goto eof_error;
        }

        // Optional average light falloff
        if ( sscanf_s(stdConffile_g_aLine, " average light falloff %f %f", &pSec->light.minRadius, &pSec->light.maxRadius) == 2 )
        {
            if ( !stdConffile_ReadLine() )
            {
                goto eof_error;
            }
        }

        // Bound box
        if ( sscanf_s(stdConffile_g_aLine, " boundbox %f %f %f %f %f %f ", &pSec->boundBox.v0.x, &pSec->boundBox.v0.y, &pSec->boundBox.v0.z, &pSec->boundBox.v1.x, &pSec->boundBox.v1.y, &pSec->boundBox.v1.z) != 6 )
        {
            goto syntax_error;
        }

        if ( !stdConffile_ReadLine() )
        {
            goto eof_error;
        }

        // Optional collide box
        if ( sscanf_s(stdConffile_g_aLine, " collidebox %f %f %f %f %f %f ", &pSec->collideBox.v0.x, &pSec->collideBox.v0.y, &pSec->collideBox.v0.z, &pSec->collideBox.v1.x, &pSec->collideBox.v1.y, &pSec->collideBox.v1.z) == 6 )
        {
            pSec->flags |= SITH_SECTOR_HASCOLLIDEBOX;
            if ( !stdConffile_ReadLine() )
            {
                goto eof_error;
            }
        }

        // Optional sound
        char aSound[64];
        if ( sscanf_s(stdConffile_g_aLine, "sound %s %f", aSound, (rsize_t)STD_ARRAYLEN(aSound), &pSec->ambientSoundVolume) == 2 )
        {
            pSec->hAmbientSound = sithSound_Load(pWorld, aSound);
            if ( !stdConffile_ReadLine() )
            {
                goto eof_error;
            }
        }

        // Optional thrust
        if ( sscanf_s(stdConffile_g_aLine, "thrust %f %f %f", &pSec->thrust.x, &pSec->thrust.y, &pSec->thrust.z) == 3 && !stdConffile_ReadLine() )
        {
            goto eof_error;
        }

        // Center
        if ( sscanf_s(stdConffile_g_aLine, " center %f %f %f", &pSec->center.x, &pSec->center.y, &pSec->center.z) != 3 )
        {
            goto syntax_error;
        }

        if ( nRead = stdConffile_ScanLine(" radius %f", &pSec->radius), nRead != 1 )
        {
            if ( nRead < 0 ) {
                goto eof_error;
            }
            goto syntax_error;
        }

        // Read sector vertices
        size_t numVerts;
        if ( nRead = stdConffile_ScanLine(" vertices %d", &numVerts), nRead != 1 )
        {
            if ( nRead < 0 ) {
                goto eof_error;
            }
            goto syntax_error;
        }

        int32_t* aVerts = (int32_t*)STDMALLOC(sizeof(int32_t) * numVerts);
        pSec->aVertIdxs = aVerts;
        if ( !pSec->aVertIdxs )
        {
            goto alloc_error;
        }

        for ( size_t j = 0; j < numVerts; ++j )
        {
            int vertNum;
            int entryNum;
            if ( nRead = stdConffile_ScanLine(" %d: %d", &entryNum, &vertNum), nRead != 2 )
            {
                if ( nRead < 0 ) {
                    goto eof_error;
                }
                goto syntax_error;
            }

            // Fixed: Reject invalid .ndy sector vertex indices before later world-vertex lookups.
            if ( vertNum < 0 || (size_t)vertNum >= pWorld->numVertices )
            {
                goto range_error;
            }

            pSec->aVertIdxs[j] = vertNum;
        }

        pSec->numVertices = numVerts;

        // Read sector surfaces info
        size_t numSurfs; int surfIdx;
        if ( nRead = stdConffile_ScanLine(" surfaces %d %d", &surfIdx, &numSurfs), nRead != 2 )
        {
            if ( nRead < 0 ) {
                goto eof_error;
            }
            goto syntax_error;
        }

        pSec->numSurfaces = numSurfs;

        // Fixed: Validate the surface range before assigning pSector through the returned pointer.
        if ( numSurfs && (surfIdx < 0 || (size_t)surfIdx >= pWorld->numSurfaces || numSurfs > pWorld->numSurfaces - (size_t)surfIdx) )
        {
            goto range_error;
        }

        SithSurface* pSurf = numSurfs ? sithSurface_GetSurfaceEx(pWorld, surfIdx) : NULL;
        pSec->pFirstSurface = pSurf;

        for ( size_t j = 0; j < numSurfs; ++j )
        {
            pSurf->pSector = pSec;
            ++pSurf;
        }
    }

    // Success
    return 0;

eof_error:
    SITHLOG_ERROR("Error: Unexpected EOF in '%s'\n", stdConffile_GetFilename());
    return 1;

syntax_error:
    SITHLOG_ERROR("Error: Invalid syntax '%s' in '%s'\n", stdConffile_g_aLine, stdConffile_GetFilename());
    return 1;

alloc_error:
    SITHLOG_ERROR("Error: Bad memory allocation for '%s' in '%s'\n", stdConffile_g_aLine, stdConffile_GetFilename());
    return 1;

range_error:
    SITHLOG_ERROR("Error: Value out of range for '%s' in '%s'\n", stdConffile_g_aLine, stdConffile_GetFilename());
    return 1;
}

int J3DAPI sithSector_WriteSectorsListBinary(tFileHandle fh, const SithWorld* pWorld)
{
    size_t sizes = sizeof(CndSectorInfo) * pWorld->numSectors;
    CndSectorInfo* aSectorInfos = (CndSectorInfo*)STDMALLOC(sizes);
    if ( !aSectorInfos )
    {
        return 1;
    }

    // Collect sector infos
    size_t numVertices = 0;
    for ( size_t i = 0; i < pWorld->numSectors; ++i )
    {
        const SithSector* pSec = &pWorld->aSectors[i];
        CndSectorInfo* pSecInfo = &aSectorInfos[i];

        pSecInfo->flags = pSec->flags;
        rdVector_Copy4(&pSecInfo->ambientLight, &pSec->ambientLight);
        rdVector_Copy4(&pSecInfo->extraLight, &pSec->extraLight);
        rdVector_Copy3(&pSecInfo->tint, &pSec->tint);

        rdVector_Copy3(&pSecInfo->light.pos, &pSec->light.pos);
        rdVector_Copy4(&pSecInfo->light.color, &pSec->light.color);
        pSecInfo->light.minRadius = pSec->light.minRadius;
        pSecInfo->light.maxRadius = pSec->light.maxRadius;

        memcpy(&pSecInfo->collideBox, &pSec->collideBox, sizeof(pSecInfo->collideBox));
        memcpy(&pSecInfo->boundBox, &pSec->boundBox, sizeof(pSecInfo->boundBox));

        const char* pSecSound;
        if ( pSec->hAmbientSound && (pSecSound = Sound_GetSoundFilename(pSec->hAmbientSound)) != 0 )
        {
            STD_STRCPY(pSecInfo->aAmbientSound, pSecSound); // Fixed: Ensured null termination by using STD_STRCPY
        }
        else
        {
            memset(pSecInfo->aAmbientSound, 0, sizeof(pSecInfo->aAmbientSound));
        }

        pSecInfo->ambientSoundVolume = pSec->ambientSoundVolume;

        rdVector_Copy3(&pSecInfo->center, &pSec->center);

        pSecInfo->radius = pSec->radius;
        pSecInfo->pvsIdx = pSec->pvsIdx;

        pSecInfo->firstSurfaceIdx = sithSurface_GetSurfaceIndexEx(pWorld, pSec->pFirstSurface);
        pSecInfo->numSurfaces     = pSec->numSurfaces;

        rdVector_Copy3(&pSecInfo->thrust, &pSec->thrust);

        pSecInfo->numVertices = pSec->numVertices;
        numVertices += pSec->numVertices;
    }

    // Now write infos to file
    size_t nWritten = sith_g_pHS->pFileWrite(fh, aSectorInfos, sizes);
    stdMemory_Free(aSectorInfos);

    if ( nWritten != sizes )
    {
        return 1;
    }

    // TODO: Check that numVertices > max uint32_t
    nWritten = sith_g_pHS->pFileWrite(fh, &numVertices, sizeof(uint32_t));
    if ( nWritten != sizeof(uint32_t) )
    {
        return 1;
    }

    // Now write verts indices of all sectors
    sizes = sizeof(int32_t) * numVertices;
    int32_t* aVerts = (int32_t*)STDMALLOC(sizes);
    if ( !aVerts )
    {
        return 1;
    }

    int32_t* pVertNum = aVerts;
    for ( size_t i = 0; i < pWorld->numSectors; ++i )
    {
        const SithSector* pSec = &pWorld->aSectors[i];
        memcpy(pVertNum, pSec->aVertIdxs, sizeof(int32_t) * pSec->numVertices);
        pVertNum += pSec->numVertices;
    }

    nWritten = sith_g_pHS->pFileWrite(fh, aVerts, sizes);
    stdMemory_Free(aVerts);
    return nWritten != sizes;
}

int J3DAPI sithSector_ReadSectorsListBinary(tFileHandle fh, SithWorld* pWorld)
{
    int32_t* aVerts = NULL;

    size_t sizes = sizeof(CndSectorInfo) * pWorld->numSectors;
    CndSectorInfo* aSecInfos = (CndSectorInfo*)STDMALLOC(sizes);
    if ( !aSecInfos )
    {
        goto error;
    }

    size_t nRead = sith_g_pHS->pFileRead(fh, aSecInfos, sizes);
    if ( nRead != sizes || sithSector_AllocWorldSectors(pWorld, pWorld->numSectors) )
    {
        goto error;
    }

    for ( size_t i = 0; i < pWorld->numSectors; ++i )
    {
        SithSector* pSec = &pWorld->aSectors[i];
        const CndSectorInfo* pSecInfo = &aSecInfos[i];

        pSec->flags = pSecInfo->flags;

        rdVector_Copy4(&pSec->ambientLight, &pSecInfo->ambientLight);
        rdVector_Copy4(&pSec->extraLight, &pSecInfo->extraLight);
        rdVector_Copy3(&pSec->tint, &pSecInfo->tint);

        rdVector_Copy3(&pSec->light.pos, &pSecInfo->light.pos);
        rdVector_Copy4(&pSec->light.color, &pSecInfo->light.color);
        pSec->light.minRadius = pSecInfo->light.minRadius;
        pSec->light.maxRadius = pSecInfo->light.maxRadius;

        memcpy(&pSec->collideBox, &pSecInfo->collideBox, sizeof(pSec->collideBox));
        memcpy(&pSec->boundBox, &pSecInfo->boundBox, sizeof(pSec->boundBox));

        if ( pSecInfo->aAmbientSound[0] )
        {
            pSec->hAmbientSound = sithSound_Load(pWorld, pSecInfo->aAmbientSound);
        }
        else
        {
            pSec->hAmbientSound = 0;
        }

        pSec->ambientSoundVolume = pSecInfo->ambientSoundVolume;

        rdVector_Copy3(&pSec->center, &pSecInfo->center);

        pSec->radius = pSecInfo->radius;
        pSec->pvsIdx = pSecInfo->pvsIdx;

        pSec->pFirstSurface = sithSurface_GetSurfaceEx(pWorld, pSecInfo->firstSurfaceIdx);
        pSec->numSurfaces = pSecInfo->numSurfaces;

        rdVector_Copy3(&pSec->thrust, &pSecInfo->thrust);

        pSec->numVertices = pSecInfo->numVertices;

        SithSurface* pSurf = pSec->pFirstSurface;
        for ( size_t j = 0; j < pSec->numSurfaces; ++j )
        {
            pSurf->pSector = pSec;
            ++pSurf;
        }
    }

    if ( aSecInfos )
    {
        stdMemory_Free(aSecInfos);
        aSecInfos = NULL;
    }

    uint32_t numVerts;
    nRead = sith_g_pHS->pFileRead(fh, &numVerts, sizeof(numVerts));
    if ( nRead != sizeof(numVerts) )
    {
        return 1;
    }

    // Read sec vertices
    sizes = sizeof(int32_t) * numVerts;
    aVerts = (int32_t*)STDMALLOC(sizes);
    if ( !aVerts )
    {
        return 1;
    }

    nRead = sith_g_pHS->pFileRead(fh, aVerts, sizes);
    if ( nRead != sizes )
    {
        goto error;
    }

    int32_t* pCurVertNum = aVerts;
    for ( size_t i = 0; i < pWorld->numSectors; ++i )
    {
        SithSector* pSec = &pWorld->aSectors[i];
        pSec->aVertIdxs = (int*)STDMALLOC(sizeof(int) * pSec->numVertices);
        if ( !pSec->aVertIdxs )
        {
            goto error;
        }

        memcpy(pSec->aVertIdxs, pCurVertNum, sizeof(int32_t) * pSec->numVertices);
        pCurVertNum += pSec->numVertices;
    }

    stdMemory_Free(aVerts);
    return 0;

error:
    if ( pWorld->aSectors )
    {
        for ( size_t i = 0; i < pWorld->numSectors; ++i )
        {
            SithSector* pSec = &pWorld->aSectors[i];
            if ( pSec->aVertIdxs )
            {
                stdMemory_Free(pSec->aVertIdxs);
            }
        }

        stdMemory_Free(pWorld->aSectors);
        pWorld->aSectors = NULL;
    }

    if ( aSecInfos )
    {
        stdMemory_Free(aSecInfos);
    }

    if ( aVerts )
    {
        stdMemory_Free(aVerts);
    }

    return 1;
}

int J3DAPI sithSector_ValidateSectorPointer(const SithWorld* pWorld, const SithSector* pSector)
{
    return pWorld && pSector && pSector >= pWorld->aSectors && pSector < &pWorld->aSectors[pWorld->numSectors];
}

int J3DAPI sithSector_AllocWorldSectors(SithWorld* pWorld, size_t numSectors)
{
    SITH_ASSERTREL(pWorld != NULL);
    SITH_ASSERTREL(pWorld->aSectors == NULL);

    pWorld->aSectors = (SithSector*)STDMALLOC(sizeof(SithSector) * numSectors);
    if ( !pWorld->aSectors )
    {
        SITHLOG_ERROR("Failed to allocate mem for %d sectors.\n", numSectors);
        return 1;
    }

    memset(pWorld->aSectors, 0, sizeof(SithSector) * numSectors);
    pWorld->numSectors = numSectors;
    sithSector_ResetAllSectors(pWorld);
    return 0;
}

void J3DAPI sithSector_ResetAllSectors(SithWorld* pWorld)
{
    for ( size_t i = 0; i < pWorld->numSectors; ++i )
    {
        SithSector* pSec = &pWorld->aSectors[i];
        pSec->numVertices         = 0;
        pSec->aVertIdxs           = NULL;
        pSec->numSurfaces         = 0;
        pSec->pFirstSurface       = NULL;
        pSec->pFirstThingInSector = NULL;
    }
}

void J3DAPI sithSector_FreeWorldSectors(SithWorld* pWorld)
{
    SITH_ASSERTREL(pWorld->aSectors);

    for ( size_t i = 0; i < pWorld->numSectors; i++ )
    {
        SithSector* pSec = &pWorld->aSectors[i];
        if ( pSec->aVertIdxs )
        {
            stdMemory_Free(pSec->aVertIdxs);
            pSec->aVertIdxs = NULL; // Added: Set to 0
        }
    }

    stdMemory_Free(pWorld->aSectors);
    pWorld->aSectors   = NULL;
    pWorld->numSectors = 0;
}

SithSector* J3DAPI sithSector_FindSectorAtPos(const SithWorld* pWorld, const rdVector3* pos)
{
    SITH_ASSERTREL(pos != NULL);
    SITH_ASSERTREL(pWorld != NULL);

    for ( size_t i = 0; i < pWorld->numSectors; ++i )
    {
        SithSector* pSec = &pWorld->aSectors[i];
        if ( pSec->boundBox.v0.x <= (double)pos->x
            && pSec->boundBox.v1.x >= (double)pos->x
            && pSec->boundBox.v0.y <= (double)pos->y
            && pSec->boundBox.v1.y >= (double)pos->y )
        {
            bool bInBounds = pSec->boundBox.v0.z <= (double)pos->z && pSec->boundBox.v1.z >= (double)pos->z;
            if ( bInBounds && sithIntersect_IsSphereInSector(pWorld, pos, /*radius=*/0.0f, pSec) )
            {
                return pSec;
            }
        }
    }

    return NULL;
}

void J3DAPI sithSector_HideSectorAdjoins(SithSector* pSector)
{
    SITH_ASSERTREL(pSector);
    for ( SithSurfaceAdjoin* i = pSector->pFirstAdjoin; i; i = i->pNextAdjoin )
    {
        sithSurface_HideSectorAdjoin(i);
    }

    pSector->flags |= SITH_SECTOR_ADJOINSOFF;
}

void J3DAPI sithSector_ShowSectorAdjoins(SithSector* pSector)
{
    for ( SithSurfaceAdjoin* i = pSector->pFirstAdjoin; i; i = i->pNextAdjoin )
    {
        sithSurface_ShowSectorAdjoin(i);
    }

    pSector->flags &= ~SITH_SECTOR_ADJOINSOFF;
}

void J3DAPI sithSector_SetSectorFlags(SithSector* pSector, SithSectorFlag flags)
{
    SITH_ASSERTREL(pSector != NULL);
    SITH_ASSERTREL(sithSector_ValidateSectorPointer(sithWorld_g_pCurrentWorld, pSector));

    SithSectorFlag prevflags = pSector->flags;
    pSector->flags |= flags;

    for ( SithThing* pCurrentCheckThing = pSector->pFirstThingInSector; pCurrentCheckThing; pCurrentCheckThing = pCurrentCheckThing->pNextThingInSector )
    {
        SITH_ASSERTREL(sithThing_ValidateThingPointer(sithWorld_g_pCurrentWorld, pCurrentCheckThing));
        if ( (prevflags & SITH_SECTOR_UNDERWATER) == 0 && (pSector->flags & SITH_SECTOR_UNDERWATER) != 0 )
        {
            switch ( pCurrentCheckThing->type )
            {
                case SITH_THING_ACTOR:
                case SITH_THING_PLAYER:
                    sithThing_ExitSector(pCurrentCheckThing);
                    sithThing_EnterSector(pCurrentCheckThing, pSector, 0, 0);
                    break;

                case SITH_THING_PARTICLE:
                case SITH_THING_SPRITE:
                    sithThing_DestroyThing(pCurrentCheckThing);
                    break;

                default:
                    continue;
            }
        }
    }
}

void J3DAPI sithSector_ClearSectorFlags(SithSector* pSector, SithSectorFlag flags)
{
    SITH_ASSERTREL(pSector != NULL);
    SITH_ASSERTREL(sithSector_ValidateSectorPointer(sithWorld_g_pCurrentWorld, pSector));

    SithSectorFlag prevflags = pSector->flags;
    pSector->flags &= ~flags;

    for ( SithThing* pCurrentCheckThing = pSector->pFirstThingInSector; pCurrentCheckThing; pCurrentCheckThing = pCurrentCheckThing->pNextThingInSector )
    {
        SITH_ASSERTREL(sithThing_ValidateThingPointer(sithWorld_g_pCurrentWorld, pCurrentCheckThing));
        if ( (prevflags & SITH_SECTOR_UNDERWATER) != 0 && (pSector->flags & SITH_SECTOR_UNDERWATER) == 0 )
        {
            switch ( pCurrentCheckThing->type )
            {
                case SITH_THING_ACTOR:
                case SITH_THING_PLAYER:
                    sithThing_ExitSector(pCurrentCheckThing);
                    sithThing_EnterSector(pCurrentCheckThing, pSector, 0, 0);
                    break;

                case SITH_THING_PARTICLE:
                case SITH_THING_SPRITE:
                    sithThing_DestroyThing(pCurrentCheckThing);
                    break;

                default:
                    continue;
            }
        }
    }
}

size_t J3DAPI sithSector_GetSectorThingCount(const SithSector* pSector)
{
    SITH_ASSERTREL(pSector != NULL);
    SITH_ASSERTREL(sithSector_ValidateSectorPointer(sithWorld_g_pCurrentWorld, pSector));

    size_t count = 0;
    for ( SithThing* pCurrentCheckThing = pSector->pFirstThingInSector; pCurrentCheckThing; pCurrentCheckThing = pCurrentCheckThing->pNextThingInSector )
    {
        SITH_ASSERTREL(sithThing_ValidateThingPointer(sithWorld_g_pCurrentWorld, pCurrentCheckThing));
        ++count;
    }

    return count;
}

size_t J3DAPI sithSector_GetSectorPlayerCount(const SithSector* pSector)
{
    SITH_ASSERTREL(pSector != NULL);
    SITH_ASSERTREL(sithSector_ValidateSectorPointer(sithWorld_g_pCurrentWorld, pSector));

    size_t count = 0;
    for ( SithThing* pCurrentCheckThing = pSector->pFirstThingInSector; pCurrentCheckThing; pCurrentCheckThing = pCurrentCheckThing->pNextThingInSector )
    {
        SITH_ASSERTREL(sithThing_ValidateThingPointer(sithWorld_g_pCurrentWorld, pCurrentCheckThing));
        if ( pCurrentCheckThing->type == SITH_THING_PLAYER )
        {
            ++count;
        }
    }

    return count;
}

size_t J3DAPI sithSector_GetAdjoinCount(const SithSector* pSector)
{
    SITH_ASSERTREL(pSector != NULL);
    SITH_ASSERTREL(sithSector_ValidateSectorPointer(sithWorld_g_pCurrentWorld, pSector));

    size_t count = 0;
    for ( SithSurfaceAdjoin* i = pSector->pFirstAdjoin; i; i = i->pNextAdjoin )
    {
        ++count;
    }

    return count;
}

void J3DAPI sithSector_SyncSector(SithSector* pSector, int flags)
{
    SITH_ASSERTREL(pSector);

    pSector->flags |= SITH_SECTOR_SYNC; // TODO: Shall flag setting be moved inside the scope?
    if ( sithMessage_g_outputstream )
    {
        // TODO: Wouldn't it make more sense for this check to move after for loop
        if ( sithSector_numModifiedSectors >= STD_ARRAYLEN(sithSector_aModifiedSectors) )
        {
            SITHLOG_ERROR("Too many modified sectors.\n");
            return;
        }

        for ( size_t i = 0; i < sithSector_numModifiedSectors; ++i )
        {
            if ( sithSector_aModifiedSectors[i] == pSector )
            {
                sithSector_aSyncFlags[i] |= flags;
                return;
            }
        }

        sithSector_aModifiedSectors[sithSector_numModifiedSectors] = pSector;
        sithSector_aSyncFlags[sithSector_numModifiedSectors++] = flags;
    }
}

size_t sithSector_SyncSectors(void)
{
    size_t  numSynced = 0;
    for ( size_t i = 0; i < sithSector_numModifiedSectors; ++i )
    {
        if ( (sithSector_aSyncFlags[i] & 1) != 0 )
        {
            sithDSS_SectorStatus(sithSector_aModifiedSectors[i], SITHMESSAGE_SENDTOJOINEDPLAYERS, SITHMESSAGE_STREAM_ALL);
        }
        else
        {
            sithDSS_SectorFlags(sithSector_aModifiedSectors[i], SITHMESSAGE_SENDTOJOINEDPLAYERS, SITHMESSAGE_STREAM_ALL);
        }

        numSynced = i + 1;
    }

    sithSector_numModifiedSectors = 0;
    return numSynced;
}

SithSector* J3DAPI sithSector_GetSectorEx(const SithWorld* pWorld, int idx)
{
    if ( pWorld && idx >= 0 && idx < pWorld->numSectors )
    {
        return &pWorld->aSectors[idx];
    }

    return NULL;
}

int J3DAPI sithSector_GetSectorIndexEx(const SithWorld* pWorld, const SithSector* pSector)
{
    if ( pSector < pWorld->aSectors || pSector > &pWorld->aSectors[pWorld->numSectors] )
    {
        return -1;
    }

    return pSector - pWorld->aSectors;
}

int J3DAPI sithSector_GetSectorIndex(const SithSector* pSector)
{
    if ( pSector < sithWorld_g_pCurrentWorld->aSectors || pSector > &sithWorld_g_pCurrentWorld->aSectors[sithWorld_g_pCurrentWorld->numSectors] )
    {
        return -1;
    }

    return pSector - sithWorld_g_pCurrentWorld->aSectors;
}

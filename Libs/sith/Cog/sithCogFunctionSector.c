#include "sithCogFunctionSector.h"
#include <j3dcore/j3dhook.h>
#include <sith/RTI/symbols.h>

#include <rdroid/Math/rdMath.h>
#include <rdroid/Math/rdVector.h>

#include <sith/Cog/sithCog.h>
#include <sith/Cog/sithCogExec.h>
#include <sith/Engine/sithAnimate.h>
#include <sith/World/sithSector.h>
#include <sith/World/sithSurface.h>
#include <sith/World/sithWorld.h>

void J3DAPI sithCogFunctionSector_GetSectorTint(SithCog* pCog)
{
    SithSector* pSector = sithCogExec_PopSector(pCog);
    if ( !pSector )
    {
        SITHLOG_ERROR("Cog %s: Illegal call to GetSectorTint().\n", pCog->aName);
        sithCogExec_PushVector(pCog, &rdroid_g_zeroVector3);
    }
    else
    {
        sithCogExec_PushVector(pCog, &pSector->tint);
    }
}

void J3DAPI sithCogFunctionSector_SetSectorTint(SithCog* pCog)
{
    rdVector3 color;
    sithCogExec_PopVector(pCog, &color);

    SithSector* pSector = sithCogExec_PopSector(pCog);
    if ( !pSector )
    {
        SITHLOG_ERROR("Cog %s: Bad sector in SetSectorTint().\n", pCog->aName);
        return;
    }

    rdMath_ClampVector3(&pSector->tint, &color, 0.0f, 1.0f);
    if ( (pCog->flags & SITHCOG_NOSYNC) == 0 )
    {
        if ( pCog->execMsgType != SITHCOG_MSG_STARTUP && pCog->execMsgType != SITHCOG_MSG_SHUTDOWN )
        {
            sithSector_SyncSector(pSector, 1);
        }
    }
}

void J3DAPI sithCogFunctionSector_GetSectorLight(SithCog* pCog)
{
    SithSector* pSector = sithCogExec_PopSector(pCog);
    if ( !pSector )
    {
        SITHLOG_ERROR("Cog %s: Illegal call to GetSectorLight().\n", pCog->aName);
        sithCogExec_PushFlex(pCog, 0.0f);
        return;
    }

    rdVector3 color;
    rdVector_Copy3(&color, (rdVector3*)&pSector->extraLight);
    sithCogExec_PushVector(pCog, &color);
}

void J3DAPI sithCogFunctionSector_SetSectorLight(SithCog* pCog)
{
    rdVector3 color;
    float timeDelta     = sithCogExec_PopFlex(pCog);
    int bVec            = sithCogExec_PopVector(pCog, &color);
    SithSector* pSector = sithCogExec_PopSector(pCog);
    if ( !pSector || !bVec )
    {
        SITHLOG_ERROR("Cog %s: Illegal call to SectorLight().\n", pCog->aName);
        return;
    }

    if ( timeDelta == 0.0f )
    {
        pSector->extraLight.red   = color.red;
        pSector->extraLight.green = color.green;
        pSector->extraLight.blue  = color.blue;
        sithSector_SyncSector(pSector, 1);
    }
    else
    {
        sithAnimate_StartSectorLightAnim(pSector, &color, timeDelta, (SithAnimateFlags)0);
    }
}

void J3DAPI sithCogFunctionSector_SetSectorAdjoins(SithCog* pCog)
{
    int bOn = sithCogExec_PopInt(pCog);
    SithSector* pSector = sithCogExec_PopSector(pCog);
    if ( !pSector )
    {
        SITHLOG_ERROR("Cog %s: Illegal params in SetSectorAdjoins().\n", pCog->aName);
        return;
    }

    if ( bOn && (pSector->flags & SITH_SECTOR_ADJOINSOFF) != 0 )
    {
        sithSector_ShowSectorAdjoins(pSector);
        sithSector_SyncSector(pSector, 0);
        return;
    }

    if ( !bOn && (pSector->flags & SITH_SECTOR_ADJOINSOFF) == 0 )
    {
        sithSector_HideSectorAdjoins(pSector);
        sithSector_SyncSector(pSector, 0);
        return;
    }
}

void J3DAPI sithCogFunctionSector_SetSectorSurfflags(SithCog* pCog)
{
    int surfflags = sithCogExec_PopInt(pCog);
    SithSector* pSector = sithCogExec_PopSector(pCog);
    if ( !pSector || !surfflags )
    {
        SITHLOG_ERROR("Cog %s: Cannot set sector surfaces flags.\n", pCog->aName);
        return;
    }

    for ( size_t i = 0; i < pSector->numSurfaces; ++i )
    {
        SithSurface* pSurf = &pSector->pFirstSurface[i];
        pSurf->flags |= surfflags;
        sithSurface_SyncSurface(pSurf);
    }
}

void J3DAPI sithCogFunctionSector_ClearSectorSurfflags(SithCog* pCog)
{
    int surfflags       = sithCogExec_PopInt(pCog);
    SithSector* pSector = sithCogExec_PopSector(pCog);
    if ( !pSector || !surfflags )
    {
        SITHLOG_ERROR("Cog %s: Cannot set sector surfaces flags.\n", pCog->aName);
        return;
    }

    for ( size_t i = 0; i < (signed int)pSector->numSurfaces; ++i )
    {
        SithSurface* pSurf = &pSector->pFirstSurface[i];
        pSurf->flags &= ~surfflags;
        sithSurface_SyncSurface(pSurf);
    }
}

void J3DAPI sithCogFunctionSector_GetSectorColormap(SithCog* pCog)
{
    sithCogExec_PopSector(pCog);

    SITHLOG_ERROR("Cog %s: Sector colormaps NOT SUPPORTED.\n", pCog->aName);
    sithCogExec_PushInt(pCog, -1);
}

void J3DAPI sithCogFunctionSector_SetSectorColormap(SithCog* pCog)
{
    sithCogExec_PopInt(pCog);
    SITHLOG_ERROR("Cog %s: SetSectorColormap NOT SUPPORTED.\n", pCog->aName);
}

void J3DAPI sithCogFunctionSector_GetSectorThrust(SithCog* pCog)
{
    SithSector* pSector = sithCogExec_PopSector(pCog);
    if ( !pSector )
    {
        SITHLOG_ERROR("Cog %s: Illegal call to GetSectorThrust().\n", pCog->aName);
        sithCogExec_PushVector(pCog, &rdroid_g_zeroVector3);
    }
    else
    {
        sithCogExec_PushVector(pCog, &pSector->thrust);
    }
}

void J3DAPI sithCogFunctionSector_SetSectorThrust(SithCog* pCog)
{
    rdVector3 dir;
    float speed         = sithCogExec_PopFlex(pCog);
    int bVec            = sithCogExec_PopVector(pCog, &dir);
    SithSector* pSector = sithCogExec_PopSector(pCog);

    if ( pSector && bVec )
    {
        if ( rdVector_IsZero3(&dir) )
        {
            pSector->flags &= ~SITH_SECTOR_USETHRUST;
            pSector->thrust = rdroid_g_zeroVector3;
        }
        else
        {
            pSector->flags |= SITH_SECTOR_USETHRUST;
            rdVector_Scale3(&pSector->thrust, &dir, speed);
        }

        sithSector_SyncSector(pSector, 1);
    }
}

void J3DAPI sithCogFunctionSector_SetSectorFlags(SithCog* pCog)
{
    SithSectorFlag flags = sithCogExec_PopInt(pCog);
    SithSector* pSector  = sithCogExec_PopSector(pCog);
    if ( !pSector || !flags )
    {
        SITHLOG_ERROR("Cog %s: Bad Args in SetSectorFlags().\n", pCog->aName);
        return;
    }

    sithSector_SetSectorFlags(pSector, flags);
    sithSector_SyncSector(pSector, 0);
}

void J3DAPI sithCogFunctionSector_ClearSectorFlags(SithCog* pCog)
{
    SithSectorFlag flags = sithCogExec_PopInt(pCog);
    SithSector* pSector  = sithCogExec_PopSector(pCog);
    if ( !pSector || !flags )
    {
        SITHLOG_ERROR("Cog %s: Bad Args in ClearSectorFlags().\n", pCog->aName);
        return;
    }

    sithSector_ClearSectorFlags(pSector, flags);
    sithSector_SyncSector(pSector, 0);
}

void J3DAPI sithCogFunctionSector_GetSectorFlags(SithCog* pCog)
{
    SithSector* pSector = sithCogExec_PopSector(pCog);
    if ( pSector )
    {
        sithCogExec_PushInt(pCog, pSector->flags);
    }
    else
    {
        sithCogExec_PushInt(pCog, -1);
    }
}

void J3DAPI sithCogFunctionSector_GetSectorThingCount(SithCog* pCog)
{
    SithSector* pSector = sithCogExec_PopSector(pCog);
    if ( pSector )
    {
        size_t numThings = sithSector_GetSectorThingCount(pSector);
        sithCogExec_PushInt(pCog, numThings);
    }
    else
    {
        sithCogExec_PushInt(pCog, -1);
    }
}

void J3DAPI sithCogFunctionSector_GetSectorPlayerCount(SithCog* pCog)
{
    SithSector* pSector = sithCogExec_PopSector(pCog);
    if ( pSector )
    {
        size_t count = sithSector_GetSectorPlayerCount(pSector);
        sithCogExec_PushInt(pCog, count);
    }
    else
    {
        sithCogExec_PushInt(pCog, -1);
    }
}

void J3DAPI sithCogFunctionSector_GetSectorCount(SithCog* pCog)
{
    sithCogExec_PushInt(pCog, sithWorld_g_pCurrentWorld->numSectors);
}

void J3DAPI sithCogFunctionSector_GetSectorCenter(SithCog* pCog)
{
    SithSector* pSector = sithCogExec_PopSector(pCog);
    if ( !pSector )
    {
        SITHLOG_ERROR("Cog %s: Bad sector passed to GetSectorCenter.\n", pCog->aName);
        sithCogExec_PushVector(pCog, &rdroid_g_zeroVector3);
    }
    else
    {
        sithCogExec_PushVector(pCog, &pSector->center);
    }
}

void J3DAPI sithCogFunctionSector_GetNumSectorVertices(SithCog* pCog)
{
    SithSector* pSector = sithCogExec_PopSector(pCog);
    if ( !pSector )
    {
        SITHLOG_ERROR("Cog %s: Bad sector passed to GetNumSectorVertices.\n", pCog->aName);

        sithCogExec_PushInt(pCog, -1);
    }
    else
    {
        sithCogExec_PushInt(pCog, pSector->numVertices);
    }
}

void J3DAPI sithCogFunctionSector_GetNumSectorSurfaces(SithCog* pCog)
{
    SithSector* pSector = sithCogExec_PopSector(pCog);
    if ( !pSector )
    {
        SITHLOG_ERROR("Cog %s: Bad sector passed to GetNumSectorSurfaces.\n", pCog->aName);

        sithCogExec_PushInt(pCog, -1);
    }
    else
    {
        sithCogExec_PushInt(pCog, pSector->numSurfaces);
    }
}

void J3DAPI sithCogFunctionSector_GetSectorVertexPos(SithCog* pCog)
{
    size_t vertNum      = sithCogExec_PopInt(pCog);
    SithSector* pSector = sithCogExec_PopSector(pCog);

    if ( !pSector )
    {
        SITHLOG_ERROR("Cog %s: Bad sector passed to GetSectorVertexPos.\n", pCog->aName);
        sithCogExec_PushVector(pCog, &rdroid_g_zeroVector3);
        return;
    }

    if ( vertNum >= pSector->numVertices )
    {
        SITHLOG_ERROR("Cog %s: Bad index passed to GetSectorVertexPos.\n", pCog->aName);
        sithCogExec_PushVector(pCog, &rdroid_g_zeroVector3);
    }
    else
    {
        sithCogExec_PushVector(pCog, &sithWorld_g_pCurrentWorld->aVertices[pSector->aVertIdxs[vertNum]]);
    }
}

void J3DAPI sithCogFunctionSector_GetSectorSurfaceRef(SithCog* pCog)
{
    size_t surfNum      = sithCogExec_PopInt(pCog);
    SithSector* pSector = sithCogExec_PopSector(pCog);
    if ( !pSector )
    {
        SITHLOG_ERROR("Cog %s: Bad sector passed to GetSectorSurfaceRef.\n", pCog->aName);
        sithCogExec_PushInt(pCog, -1);
        return;
    }

    if ( surfNum >= pSector->numSurfaces )
    {
        SITHLOG_ERROR("Cog %s: Bad index passed to GetSectorSurfaceRef.\n", pCog->aName);
        sithCogExec_PushInt(pCog, -1);
    }
    else
    {
        int surfIdx = sithSurface_GetSurfaceIndex(&pSector->pFirstSurface[surfNum]);
        sithCogExec_PushInt(pCog, surfIdx);
    }
}

void J3DAPI sithCogFunctionSector_SyncSector(SithCog* pCog)
{
    SithSector* pSector = sithCogExec_PopSector(pCog);
    if ( !pSector )
    {
        SITHLOG_ERROR("Cog %s: Bad sector in SyncSector().\n", pCog->aName);
        return;
    }

    sithSector_SyncSector(pSector, 1);
}

void J3DAPI sithCogFunctionSector_FindSectorAtPos(SithCog* pCog)
{
    rdVector3 pos;
    sithCogExec_PopVector(pCog, &pos);
    SithSector* pFoundSector = sithSector_FindSectorAtPos(sithWorld_g_pCurrentWorld, &pos);

    if ( pFoundSector )
    {
        int secNum = sithSector_GetSectorIndex(pFoundSector);
        sithCogExec_PushInt(pCog, secNum);
    }
    else
    {
        sithCogExec_PushInt(pCog, -1);
    }
}

int J3DAPI sithCogFunctionSector_RegisterFunctions(SithCogSymbolTable* pTable)
{
    return sithCog_RegisterFunction(pTable, sithCogFunctionSector_GetSectorTint, "getsectortint")
        || sithCog_RegisterFunction(pTable, sithCogFunctionSector_SetSectorTint, "setsectortint")
        || sithCog_RegisterFunction(pTable, sithCogFunctionSector_SetSectorAdjoins, "setsectoradjoins")
        || sithCog_RegisterFunction(pTable, sithCogFunctionSector_SetSectorAdjoins, "sectoradjoins")
        || sithCog_RegisterFunction(pTable, sithCogFunctionSector_GetSectorLight, "getsectorlight")
        || sithCog_RegisterFunction(pTable, sithCogFunctionSector_SetSectorLight, "setsectorlight")
        || sithCog_RegisterFunction(pTable, sithCogFunctionSector_SetSectorLight, "sectorlight")
        || sithCog_RegisterFunction(pTable, sithCogFunctionSector_GetSectorColormap, "getcolormap")
        || sithCog_RegisterFunction(pTable, sithCogFunctionSector_GetSectorColormap, "getsectorcolormap")
        || sithCog_RegisterFunction(pTable, sithCogFunctionSector_SetSectorColormap, "setcolormap")
        || sithCog_RegisterFunction(pTable, sithCogFunctionSector_SetSectorColormap, "setsectorcolormap")
        || sithCog_RegisterFunction(pTable, sithCogFunctionSector_GetSectorThrust, "getsectorthrust")
        || sithCog_RegisterFunction(pTable, sithCogFunctionSector_SetSectorThrust, "setsectorthrust")
        || sithCog_RegisterFunction(pTable, sithCogFunctionSector_SetSectorThrust, "sectorthrust")
        || sithCog_RegisterFunction(pTable, sithCogFunctionSector_GetSectorFlags, "getsectorflags")
        || sithCog_RegisterFunction(pTable, sithCogFunctionSector_SetSectorFlags, "setsectorflags")
        || sithCog_RegisterFunction(pTable, sithCogFunctionSector_ClearSectorFlags, "clearsectorflags")
        || sithCog_RegisterFunction(pTable, sithCogFunctionSector_GetSectorThingCount, "getsectorthingcount")
        || sithCog_RegisterFunction(pTable, sithCogFunctionSector_GetSectorThingCount, "sectorthingcount")
        || sithCog_RegisterFunction(pTable, sithCogFunctionSector_GetSectorPlayerCount, "getsectorplayercount")
        || sithCog_RegisterFunction(pTable, sithCogFunctionSector_GetSectorPlayerCount, "sectorplayercount")
        || sithCog_RegisterFunction(pTable, sithCogFunctionSector_GetSectorCount, "getsectorcount")
        || sithCog_RegisterFunction(pTable, sithCogFunctionSector_GetSectorCenter, "getsectorcenter")
        || sithCog_RegisterFunction(pTable, sithCogFunctionSector_GetNumSectorVertices, "getnumsectorvertices")
        || sithCog_RegisterFunction(pTable, sithCogFunctionSector_GetSectorVertexPos, "getsectorvertexpos")
        || sithCog_RegisterFunction(pTable, sithCogFunctionSector_GetNumSectorSurfaces, "getnumsectorsurfaces")
        || sithCog_RegisterFunction(pTable, sithCogFunctionSector_GetSectorSurfaceRef, "getsectorsurfaceref")
        || sithCog_RegisterFunction(pTable, sithCogFunctionSector_SyncSector, "syncsector")
        || sithCog_RegisterFunction(pTable, sithCogFunctionSector_FindSectorAtPos, "findsectoratpos")
        || sithCog_RegisterFunction(pTable, sithCogFunctionSector_SetSectorSurfflags, "setsectorsurfflags")
        || sithCog_RegisterFunction(pTable, sithCogFunctionSector_ClearSectorSurfflags, "clearsectorsurfflags");
}


void sithCogFunctionSector_InstallHooks(void)
{
    J3D_HOOKFUNC(sithCogFunctionSector_RegisterFunctions);
    J3D_HOOKFUNC(sithCogFunctionSector_GetSectorTint);
    J3D_HOOKFUNC(sithCogFunctionSector_SetSectorTint);
    J3D_HOOKFUNC(sithCogFunctionSector_GetSectorLight);
    J3D_HOOKFUNC(sithCogFunctionSector_SetSectorLight);
    J3D_HOOKFUNC(sithCogFunctionSector_SetSectorAdjoins);
    J3D_HOOKFUNC(sithCogFunctionSector_SetSectorSurfflags);
    J3D_HOOKFUNC(sithCogFunctionSector_ClearSectorSurfflags);
    J3D_HOOKFUNC(sithCogFunctionSector_GetSectorColormap);
    J3D_HOOKFUNC(sithCogFunctionSector_SetSectorColormap);
    J3D_HOOKFUNC(sithCogFunctionSector_GetSectorThrust);
    J3D_HOOKFUNC(sithCogFunctionSector_SetSectorThrust);
    J3D_HOOKFUNC(sithCogFunctionSector_SetSectorFlags);
    J3D_HOOKFUNC(sithCogFunctionSector_ClearSectorFlags);
    J3D_HOOKFUNC(sithCogFunctionSector_GetSectorFlags);
    J3D_HOOKFUNC(sithCogFunctionSector_GetSectorThingCount);
    J3D_HOOKFUNC(sithCogFunctionSector_GetSectorPlayerCount);
    J3D_HOOKFUNC(sithCogFunctionSector_GetSectorCount);
    J3D_HOOKFUNC(sithCogFunctionSector_GetSectorCenter);
    J3D_HOOKFUNC(sithCogFunctionSector_GetNumSectorVertices);
    J3D_HOOKFUNC(sithCogFunctionSector_GetNumSectorSurfaces);
    J3D_HOOKFUNC(sithCogFunctionSector_GetSectorVertexPos);
    J3D_HOOKFUNC(sithCogFunctionSector_GetSectorSurfaceRef);
    J3D_HOOKFUNC(sithCogFunctionSector_SyncSector);
    J3D_HOOKFUNC(sithCogFunctionSector_FindSectorAtPos);
}

void sithCogFunctionSector_ResetGlobals(void)
{}
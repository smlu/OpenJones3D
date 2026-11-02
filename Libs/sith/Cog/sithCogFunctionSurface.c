#include "sithCogFunctionSurface.h"
#include <j3dcore/j3dhook.h>
#include <sith/RTI/symbols.h>

#include <rdroid/Math/rdVector.h>

#include <sith/Cog/sithCog.h>
#include <sith/Cog/sithCogExec.h>
#include <sith/Engine/sithAnimate.h>
#include <sith/World/sithSector.h>
#include <sith/World/sithSurface.h>
#include <sith/World/sithWorld.h>

#include <std/General/stdMath.h>

void J3DAPI sithCogFunctionSurface_GetAdjoinAlpha(SithCog* pCog)
{
    SithSurface* pSurf;
    float alpha;

    pSurf = sithCogExec_PopSurface(pCog);
    if ( !pSurf )
    {
        SITHLOG_ERROR("Cog %s: Bad surface passed to GetAdjoinAlpha.\n", pCog->aName);
        sithCogExec_PushInt(pCog, -1);
        return;
    }

    alpha = pSurf->aIntensities->alpha * 255.0f; // TODO: Is this right? Shouldn't be extralight.alpha?
    sithCogExec_PushFlex(pCog, alpha);
}

void J3DAPI sithCogFunctionSurface_SetAdjoinAlpha(SithCog* pCog)
{

    float alpha = sithCogExec_PopFlex(pCog);
    SithSurface* pSurface = sithCogExec_PopSurface(pCog);

    if ( !pSurface )
    {
        SITHLOG_ERROR("Cog %s: Bad surface passed to SetAdjoinAlpha.\n", pCog->aName);
        return;
    }

    alpha = STDMATH_CLAMP(alpha / 255.f, 0.0f, 1.0f);
    if ( alpha >= 1.0f )
    {
        pSurface->face.flags  &= ~RD_FF_TEX_TRANSLUCENT;
    }
    else
    {
        pSurface->face.flags |= RD_FF_TEX_TRANSLUCENT;
    }

    // Update alpha of all vertices
    for ( size_t i = 0; i < pSurface->face.numVertices; ++i )
    {
        pSurface->aIntensities[i].alpha = alpha;
    }


    pSurface->flags |= SITH_SURFACE_SYNC;
}

void J3DAPI sithCogFunctionSurface_GetSurfaceAdjoin(SithCog* pCog)
{
    SithSurface* pSurf = sithCogExec_PopSurface(pCog);
    if ( !pSurf || !pSurf->pAdjoin || !pSurf->pAdjoin->pMirrorAdjoin )
    {
        SITHLOG_ERROR("Cog %s: Bad surface passed to GetSurfaceAdjoin.\n", pCog->aName);
        sithCogExec_PushInt(pCog, -1);
        return;
    }

    SithSurface* pAdjSurf = pSurf->pAdjoin->pMirrorAdjoin->pAdjoinSurface;
    if ( !pAdjSurf )
    {
        SITHLOG_ERROR("Cog %s: No adjoining surface in GetSurfaceAdjoin.\n", pCog->aName);
        sithCogExec_PushInt(pCog, -1);
    }
    else
    {
        int surfIdx = sithSurface_GetSurfaceIndex(pAdjSurf);
        sithCogExec_PushInt(pCog, surfIdx);
    }
}

void J3DAPI sithCogFunctionSurface_GetSurfaceSector(SithCog* pCog)
{
    SithSurface* pSurf = sithCogExec_PopSurface(pCog);
    if ( !pSurf )
    {
        SITHLOG_ERROR("Cog %s: Bad surface passed to GetSurfaceSector.\n", pCog->aName);
        sithCogExec_PushInt(pCog, -1);
        return;
    }

    const SithSector* pSector = pSurf->pSector;
    if ( pSector )
    {
        int secIdx = sithSector_GetSectorIndex(pSector);
        sithCogExec_PushInt(pCog, secIdx);
    }
    else
    {
        sithCogExec_PushInt(pCog, -1);
    }
}

void J3DAPI sithCogFunctionSurface_GetNumSurfaceVertices(SithCog* pCog)
{
    SithSurface* pSurf = sithCogExec_PopSurface(pCog);
    if ( !pSurf )
    {
        SITHLOG_ERROR("Cog %s: Bad surface passed to GetNumSurfaceVertices.\n", pCog->aName);
        sithCogExec_PushInt(pCog, -1);
    }
    else
    {
        sithCogExec_PushInt(pCog, pSurf->face.numVertices);
    }
}

void J3DAPI sithCogFunctionSurface_GetSurfaceVertexPos(SithCog* pCog)
{
    size_t vertNum = sithCogExec_PopInt(pCog);
    SithSurface* pSurf = sithCogExec_PopSurface(pCog);

    if ( !pSurf )
    {
        SITHLOG_ERROR("Cog %s: Bad surface passed to GetSurfaceVertexPos.\n", pCog->aName);
        sithCogExec_PushVector(pCog, &rdroid_g_zeroVector3);
        return;
    }

    if ( vertNum >= pSurf->face.numVertices )
    {
        SITHLOG_ERROR("Cog %s: Bad index passed to GetSurfaceVertexPos.\n", pCog->aName);
        sithCogExec_PushVector(pCog, &rdroid_g_zeroVector3);
        return;
    }

    sithCogExec_PushVector(pCog, &sithWorld_g_pCurrentWorld->aVertices[pSurf->face.aVertices[vertNum]]);
}


void J3DAPI sithCogFunctionSurface_SetHorizonSkyOffset(SithCog* pCog)
{
    rdVector3 offset;
    sithCogExec_PopVector(pCog, &offset);
    sithWorld_g_pCurrentWorld->horizonSkyOffset.x = offset.x;
    sithWorld_g_pCurrentWorld->horizonSkyOffset.y = offset.y;
}

void J3DAPI sithCogFunctionSurface_GetHorizonSkyOffset(SithCog* pCog)
{
    rdVector3 offset;
    offset.x = sithWorld_g_pCurrentWorld->horizonSkyOffset.x;
    offset.y = sithWorld_g_pCurrentWorld->horizonSkyOffset.y;
    offset.z = 0.0f;
    sithCogExec_PushVector(pCog, &offset);
}

void J3DAPI sithCogFunctionSurface_SetCeilingSkyOffset(SithCog* pCog)
{
    rdVector3 offset;
    sithCogExec_PopVector(pCog, &offset);
    sithWorld_g_pCurrentWorld->ceilingSkyOffset.x = offset.x;
    sithWorld_g_pCurrentWorld->ceilingSkyOffset.y = offset.y;
}

void J3DAPI sithCogFunctionSurface_GetCeilingSkyOffset(SithCog* pCog)
{
    rdVector3 offset;
    offset.x = sithWorld_g_pCurrentWorld->ceilingSkyOffset.x;
    offset.y = sithWorld_g_pCurrentWorld->ceilingSkyOffset.y;
    offset.z = 0.0f;
    sithCogExec_PushVector(pCog, &offset);
}

void J3DAPI sithCogFunctionSurface_SlideHorizonSky(SithCog* pCog)
{
    rdVector2 dir;
    dir.y = sithCogExec_PopFlex(pCog);
    dir.x = sithCogExec_PopFlex(pCog);

    SithAnimationSlot* pAnim = sithAnimate_StartSideSkyAnim(SITH_SURFACE_HORIZONSKY, &dir);
    if ( pAnim )
    {
        sithCogExec_PushInt(pCog, pAnim->animID);
    }
    else
    {
        sithCogExec_PushInt(pCog, -1);
    }
}

void J3DAPI sithCogFunctionSurface_SlideCeilingSky(SithCog* pCog)
{
    rdVector2 dir;
    dir.y = sithCogExec_PopFlex(pCog);
    dir.x = sithCogExec_PopFlex(pCog);

    SithAnimationSlot* pAnim = sithAnimate_StartSideSkyAnim(SITH_SURFACE_CEILINGSKY, &dir);
    if ( pAnim )
    {
        sithCogExec_PushInt(pCog, pAnim->animID);
    }
    else
    {
        sithCogExec_PushInt(pCog, -1);
    }
}

void J3DAPI sithCogFunctionSurface_SlideWall(SithCog* pCog)
{
    rdVector3 dir;
    float speed = sithCogExec_PopFlex(pCog) / 10.0f;
    int bVec    = sithCogExec_PopVector(pCog, &dir);
    SithSurface* pSurf = sithCogExec_PopSurface(pCog);

    if ( !pSurf || !pSurf->face.pMaterial )
    {
        SITHLOG_ERROR("Cog %s: Illegal surface index in SlideWall call.\n", pCog->aName);
        return;
    }
    else if ( !bVec )
    {
        SITHLOG_ERROR("Cog %s: Illegal vector in SlideWall call.\n", pCog->aName);
        return;
    }

    rdVector_Scale3Acc(&dir, speed);
    SithAnimationSlot* pAnim = sithAnimate_StartScrollSurfaceAnim(pSurf, &dir);
    if ( !pAnim )
    {
        sithCogExec_PushInt(pCog, -1);
        return;
    }

    sithSurface_SyncSurface(pSurf);
    sithCogExec_PushInt(pCog, pAnim->animID);
}

void J3DAPI sithCogFunctionSurface_GetWallCel(SithCog* pCog)
{
    SITH_ASSERTREL(pCog);

    SithSurface* pSurf = sithCogExec_PopSurface(pCog);
    if ( !pSurf || !pSurf->face.pMaterial )
    {
        SITHLOG_ERROR("Cog %s: Illegal surface index in GetWallCel call.\n", pCog->aName);
        sithCogExec_PushInt(pCog, -1);
    }
    else
    {
        sithCogExec_PushInt(pCog, pSurf->face.matCelNum);
    }
}

void J3DAPI sithCogFunctionSurface_SetWallCel(SithCog* pCog)
{
    SITH_ASSERTREL(pCog);
    int celNum         = sithCogExec_PopInt(pCog);
    SithSurface* pSurf = sithCogExec_PopSurface(pCog);
    if ( !pSurf || pSurf->face.pMaterial == NULL )
    {
        SITHLOG_ERROR("Cog %s: Illegal surface index in SetWallCel call.\n", pCog->aName);
        return;
    }

    // cell num can be -1 or < numCels
    if ( celNum < -1 || celNum >= pSurf->face.pMaterial->numCels )
    {
        SITHLOG_ERROR("Cog %s: Illegal cel number %d in SetWallCel call.\n", pCog->aName, celNum);
        sithCogExec_PushInt(pCog, -1);
        return;
    }

    int curCelNum = pSurf->face.matCelNum;
    pSurf->face.matCelNum = celNum;

    sithSurface_SyncSurface(pSurf);
    sithCogExec_PushInt(pCog, curCelNum);
}

void J3DAPI sithCogFunctionSurface_GetSurfaceMaterial(SithCog* pCog)
{
    SithSurface* pSurf = sithCogExec_PopSurface(pCog);
    if ( pSurf && pSurf->face.pMaterial != NULL )
    {
        sithCogExec_PushInt(pCog, pSurf->face.pMaterial - sithWorld_g_pCurrentWorld->aMaterials);
    }
    else
    {
        sithCogExec_PushInt(pCog, -1);
    }
}

void J3DAPI sithCogFunctionSurface_SetSurfaceMaterial(SithCog* pCog)
{
    rdMaterial* pMat   = sithCogExec_PopMaterial(pCog);
    SithSurface* pSurf = sithCogExec_PopSurface(pCog);
    if ( !pSurf )
    {
        sithCogExec_PushInt(pCog, -1);
        return;
    }

    rdMaterial* pCurrentMat = pSurf->face.pMaterial;
    pSurf->face.pMaterial = pMat;
    if ( pCurrentMat )
    {
        sithCogExec_PushInt(pCog, pCurrentMat - sithWorld_g_pCurrentWorld->aMaterials);
    }
    else
    {
        sithCogExec_PushInt(pCog, -1);
    }

    sithSurface_SyncSurface(pSurf);
}

void J3DAPI sithCogFunctionSurface_SetSurfaceFlags(SithCog* pCog)
{
    int surfflags      = sithCogExec_PopInt(pCog);
    SithSurface* pSurf = sithCogExec_PopSurface(pCog);
    if ( !pSurf || !surfflags )
    {
        SITHLOG_ERROR("Cog %s: Cannot set surface flags.\n", pCog->aName);
        return;
    }

    pSurf->flags |= surfflags;
    sithSurface_SyncSurface(pSurf);
}

void J3DAPI sithCogFunctionSurface_ClearSurfaceFlags(SithCog* pCog)
{
    int surfflags      = sithCogExec_PopInt(pCog);
    SithSurface* pSurf = sithCogExec_PopSurface(pCog);
    if ( !pSurf || !surfflags )
    {
        SITHLOG_ERROR("Cog %s: Cannot clear surface flags.\n", pCog->aName);
        return;
    }

    pSurf->flags &= ~surfflags;
    sithSurface_SyncSurface(pSurf);
}

void J3DAPI sithCogFunctionSurface_GetSurfaceFlags(SithCog* pCog)
{
    SithSurface* pSurf = sithCogExec_PopSurface(pCog);
    if ( pSurf )
    {
        sithCogExec_PushInt(pCog, pSurf->flags);
    }
    else
    {
        sithCogExec_PushInt(pCog, -1);
    }
}

void J3DAPI sithCogFunctionSurface_SetAdjoinFlags(SithCog* pCog)
{
    int adjflags       = sithCogExec_PopInt(pCog);
    SithSurface* pSurf = sithCogExec_PopSurface(pCog);
    if ( !pSurf || pSurf->pAdjoin == NULL || adjflags == 0 )
    {
        SITHLOG_ERROR("Cog %s: Cannot set adjoin flags.\n", pCog->aName);
        return;
    }

    pSurf->pAdjoin->flags |= adjflags;
    sithSurface_SyncSurface(pSurf);
}

void J3DAPI sithCogFunctionSurface_ClearAdjoinFlags(SithCog* pCog)
{
    int adjflags        = sithCogExec_PopInt(pCog);
    SithSurface* pSurf = sithCogExec_PopSurface(pCog);
    if ( !pSurf || pSurf->pAdjoin == NULL || adjflags == 0 )
    {
        SITHLOG_ERROR("Cog %s: Cannot clear adjoin flags.\n", pCog->aName);
        return;
    }

    pSurf->pAdjoin->flags &= ~adjflags;
    sithSurface_SyncSurface(pSurf);
}

void J3DAPI sithCogFunctionSurface_GetAdjoinFlags(SithCog* pCog)
{
    SithSurface* pSurf = sithCogExec_PopSurface(pCog);
    if ( !pSurf )
    {
        sithCogExec_PushInt(pCog, -1);
    }
    else if ( pSurf->pAdjoin )
    {
        sithCogExec_PushInt(pCog, pSurf->pAdjoin->flags);
    }
}

void J3DAPI sithCogFunctionSurface_SetFaceType(SithCog* pCog)
{
    rdFaceFlags faceflags = sithCogExec_PopInt(pCog);
    SithSurface* pSurf    = sithCogExec_PopSurface(pCog);
    if ( !pSurf )
    {
        SITHLOG_ERROR("Cog %s: Cannot set face type.\n", pCog->aName);
        return;
    }

    pSurf->face.flags |= faceflags;
    sithSurface_SyncSurface(pSurf);
}

void J3DAPI sithCogFunctionSurface_ClearFaceType(SithCog* pCog)
{
    rdFaceFlags faceflags = sithCogExec_PopInt(pCog);
    SithSurface* pSurf    = sithCogExec_PopSurface(pCog);
    if ( !pSurf )
    {
        SITHLOG_ERROR("Cog %s: Cannot clear face type.\n", pCog->aName);
        return;
    }

    pSurf->face.flags &= ~faceflags;
    sithSurface_SyncSurface(pSurf);
}

void J3DAPI sithCogFunctionSurface_GetFaceType(SithCog* pCog)
{
    SithSurface* pSurf = sithCogExec_PopSurface(pCog);
    if ( pSurf )
    {
        sithCogExec_PushInt(pCog, pSurf->face.flags);
    }
    else
    {
        sithCogExec_PushInt(pCog, -1);
    }
}

void J3DAPI sithCogFunctionSurface_SetFaceGeoMode(SithCog* pCog)
{
    int geoMode        = sithCogExec_PopInt(pCog);
    SithSurface* pSurf = sithCogExec_PopSurface(pCog);
    if ( !pSurf )
    {
        SITHLOG_ERROR("Cog %s: Cannot set face geometryMode.\n", pCog->aName);
        return;
    }

    if ( pSurf->face.pMaterial )
    {
        pSurf->face.geometryMode = geoMode;
        sithSurface_SyncSurface(pSurf);
    }
    else
    {
        pSurf->face.geometryMode = RD_GEOMETRY_NONE;
        if ( (pCog->flags & SITHCOG_NOSYNC) == 0 && pCog->execMsgType != SITHCOG_MSG_STARTUP && pCog->execMsgType != SITHCOG_MSG_SHUTDOWN )
        {
            sithSurface_SyncSurface(pSurf);
        }
    }
}

void J3DAPI sithCogFunctionSurface_GetFaceGeoMode(SithCog* pCog)
{
    SithSurface* pSurf = sithCogExec_PopSurface(pCog);
    if ( pSurf )
    {
        sithCogExec_PushInt(pCog, pSurf->face.geometryMode);
    }
    else
    {
        sithCogExec_PushInt(pCog, -1);
    }
}

void J3DAPI sithCogFunctionSurface_SetFaceLightMode(SithCog* pCog)
{
    int lightmode      = sithCogExec_PopInt(pCog);
    SithSurface* pSurf = sithCogExec_PopSurface(pCog);
    if ( !pSurf )
    {
        SITHLOG_ERROR("Cog %s: Cannot set face lightingMode.\n", pCog->aName);
        return;
    }

    pSurf->face.lightingMode = lightmode;
    sithSurface_SyncSurface(pSurf);
}

void J3DAPI sithCogFunctionSurface_GetFaceLightMode(SithCog* pCog)
{
    SithSurface* pSurf = sithCogExec_PopSurface(pCog);
    if ( pSurf )
    {
        sithCogExec_PushInt(pCog, pSurf->face.lightingMode);
    }
    else
    {
        sithCogExec_PushInt(pCog, -1);
    }
}

void J3DAPI sithCogFunctionSurface_SetSurfaceLight(SithCog* pCog)
{
    rdVector3 light;
    float time = sithCogExec_PopFlex(pCog);
    int bVec = sithCogExec_PopVector(pCog, &light);
    SithSurface* pSurf = sithCogExec_PopSurface(pCog);
    if ( !pSurf || !bVec )
    {
        SITHLOG_ERROR("Cog %s: Illegal params in SetSurfaceLight().\n", pCog->aName);
        sithCogExec_PushInt(pCog, -1);
        return;
    }

    if ( time == 0.0f )
    {

        rdVector_Copy3((rdVector3*)&pSurf->face.extraLight, &light);
        sithSurface_SyncSurface(pSurf);
    }
    else
    {
        SithAnimationSlot* pAnim = sithAnimate_StartSurfaceLightAnim(pSurf, &light, time);
        if ( pAnim )
        {
            sithCogExec_PushInt(pCog, pAnim->animID);
        }
        else
        {
            sithCogExec_PushInt(pCog, -1);
        }
    }
}

void J3DAPI sithCogFunctionSurface_GetSurfaceLight(SithCog* pCog)
{
    SithSurface* pSurf = sithCogExec_PopSurface(pCog);
    if ( !pSurf )
    {
        SITHLOG_ERROR("Cog %s: Illegal params in GetSurfaceLight().\n", pCog->aName);
        sithCogExec_PushVector(pCog, &rdroid_g_zeroVector3);
    }
    else
    {
        sithCogExec_PushVector(pCog, (const rdVector3*)&pSurf->face.extraLight);
    }
}

void J3DAPI sithCogFunctionSurface_GetSurfaceCenter(SithCog* pCog)
{
    SithSurface* pSurf = sithCogExec_PopSurface(pCog);
    if ( !pSurf )
    {
        sithCogExec_PushVector(pCog, &rdroid_g_zeroVector3);
        return;
    }

    rdVector3 point;
    sithSurface_GetCenterPoint(pSurf, &point);
    sithCogExec_PushVector(pCog, &point);
}

void J3DAPI sithCogFunctionSurface_SurfaceLightAnim(SithCog* pCog)
{
    float speed = sithCogExec_PopFlex(pCog);

    // TODO: [BUG] I think this is wrong, the order of the colors is wrong
    rdVector3 endColor;
    endColor.red   = sithCogExec_PopFlex(pCog);
    endColor.green = sithCogExec_PopFlex(pCog);
    endColor.blue  = sithCogExec_PopFlex(pCog);

    // TODO: [BUG] I think this is wrong, the order of the colors is wrong
    rdVector4 startColor;
    startColor.red   = sithCogExec_PopFlex(pCog);
    startColor.green = sithCogExec_PopFlex(pCog);
    startColor.blue  = sithCogExec_PopFlex(pCog);

    SithSurface* pSurf = sithCogExec_PopSurface(pCog);
    if ( !pSurf )
    {
        SITHLOG_ERROR("Cog %s: Bad parameters in SurfaceLightAnim().\n", pCog->aName);
        sithCogExec_PushInt(pCog, -1);
        return;
    }

    pSurf->face.extraLight.red   = startColor.red;
    pSurf->face.extraLight.green = startColor.green;
    pSurf->face.extraLight.blue  = startColor.blue;
    //pSurf->face.extraLight.alpha = startColor.alpha; // Removed: Original code has a bug here as alpha is not initialized

    SithAnimationSlot* pAnim = sithAnimate_StartSurfaceLightAnim(pSurf, &endColor, speed * 0.5f);
    if ( pAnim )
    {
        sithCogExec_PushInt(pCog, pAnim->animID);
    }
    else
    {
        sithCogExec_PushInt(pCog, -1);
    }
}

void J3DAPI sithCogFunctionSurface_GetSurfaceCount(SithCog* pCog)
{
    if ( sithWorld_g_pCurrentWorld )
    {
        sithCogExec_PushInt(pCog, sithWorld_g_pCurrentWorld->numSurfaces);
    }
    else
    {
        sithCogExec_PushInt(pCog, -1);
    }
}

void J3DAPI sithCogFunctionSurface_GetSurfaceNormal(SithCog* pCog)
{
    SithSurface* pSurf = sithCogExec_PopSurface(pCog);
    if ( !pSurf )
    {
        SITHLOG_ERROR("Cog %s: Illegal call to GetSurfaceNormal().\n", pCog->aName);
        sithCogExec_PushVector(pCog, &rdroid_g_zeroVector3);
    }
    else
    {
        sithCogExec_PushVector(pCog, &pSurf->face.normal);
    }
}

void J3DAPI sithCogFunctionSurface_SyncSurface(SithCog* pCog)
{
    SithSurface* pSurf = sithCogExec_PopSurface(pCog);
    if ( !pSurf )
    {
        SITHLOG_ERROR("Cog %s: Bad surface in SyncSurface().\n", pCog->aName);
    }
    else
    {
        sithSurface_SyncSurface(pSurf);
    }
}

void J3DAPI sithCogFunctionSurface_RegisterFunctions(SithCogSymbolTable* pTbl)
{
    sithCog_RegisterFunction(pTbl, sithCogFunctionSurface_GetSurfaceAdjoin, "getsurfaceadjoin");
    sithCog_RegisterFunction(pTbl, sithCogFunctionSurface_GetSurfaceSector, "getsurfacesector");
    sithCog_RegisterFunction(pTbl, sithCogFunctionSurface_GetNumSurfaceVertices, "getnumsurfacevertices");
    sithCog_RegisterFunction(pTbl, sithCogFunctionSurface_GetSurfaceVertexPos, "getsurfacevertexpos");
    sithCog_RegisterFunction(pTbl, sithCogFunctionSurface_SetHorizonSkyOffset, "sethorizonskyoffset");
    sithCog_RegisterFunction(pTbl, sithCogFunctionSurface_GetHorizonSkyOffset, "gethorizonskyoffset");
    sithCog_RegisterFunction(pTbl, sithCogFunctionSurface_SetCeilingSkyOffset, "setceilingskyoffset");
    sithCog_RegisterFunction(pTbl, sithCogFunctionSurface_GetCeilingSkyOffset, "getceilingskyoffset");
    sithCog_RegisterFunction(pTbl, sithCogFunctionSurface_SlideHorizonSky, "slidehorizonsky");
    sithCog_RegisterFunction(pTbl, sithCogFunctionSurface_SlideCeilingSky, "slideceilingsky");
    sithCog_RegisterFunction(pTbl, sithCogFunctionSurface_GetSurfaceCount, "getsurfacecount");
    sithCog_RegisterFunction(pTbl, sithCogFunctionSurface_SlideWall, "slidewall");
    sithCog_RegisterFunction(pTbl, sithCogFunctionSurface_SlideWall, "slidesurface");
    sithCog_RegisterFunction(pTbl, sithCogFunctionSurface_GetWallCel, "getwallcel");
    sithCog_RegisterFunction(pTbl, sithCogFunctionSurface_SetWallCel, "setwallcel");
    sithCog_RegisterFunction(pTbl, sithCogFunctionSurface_GetWallCel, "getsurfacecel");
    sithCog_RegisterFunction(pTbl, sithCogFunctionSurface_SetWallCel, "setsurfacecel");
    sithCog_RegisterFunction(pTbl, sithCogFunctionSurface_GetSurfaceMaterial, "getsurfacemat");
    sithCog_RegisterFunction(pTbl, sithCogFunctionSurface_SetSurfaceMaterial, "setsurfacemat");
    sithCog_RegisterFunction(pTbl, sithCogFunctionSurface_GetSurfaceFlags, "getsurfaceflags");
    sithCog_RegisterFunction(pTbl, sithCogFunctionSurface_SetSurfaceFlags, "setsurfaceflags");
    sithCog_RegisterFunction(pTbl, sithCogFunctionSurface_ClearSurfaceFlags, "clearsurfaceflags");
    sithCog_RegisterFunction(pTbl, sithCogFunctionSurface_GetAdjoinFlags, "getadjoinflags");
    sithCog_RegisterFunction(pTbl, sithCogFunctionSurface_SetAdjoinFlags, "setadjoinflags");
    sithCog_RegisterFunction(pTbl, sithCogFunctionSurface_ClearAdjoinFlags, "clearadjoinflags");
    sithCog_RegisterFunction(pTbl, sithCogFunctionSurface_SetFaceType, "setfacetype");
    sithCog_RegisterFunction(pTbl, sithCogFunctionSurface_ClearFaceType, "clearfacetype");
    sithCog_RegisterFunction(pTbl, sithCogFunctionSurface_GetFaceType, "getfacetype");
    sithCog_RegisterFunction(pTbl, sithCogFunctionSurface_SetFaceGeoMode, "setfacegeomode");
    sithCog_RegisterFunction(pTbl, sithCogFunctionSurface_GetFaceGeoMode, "getfacegeomode");
    sithCog_RegisterFunction(pTbl, sithCogFunctionSurface_SetFaceLightMode, "setfacelightmode");
    sithCog_RegisterFunction(pTbl, sithCogFunctionSurface_GetFaceLightMode, "getfacelightmode");
    sithCog_RegisterFunction(pTbl, sithCogFunctionSurface_GetSurfaceLight, "getsurfacelight");
    sithCog_RegisterFunction(pTbl, sithCogFunctionSurface_SetSurfaceLight, "setsurfacelight");
    sithCog_RegisterFunction(pTbl, sithCogFunctionSurface_GetSurfaceCenter, "getsurfacecenter");
    sithCog_RegisterFunction(pTbl, sithCogFunctionSurface_SurfaceLightAnim, "surfacelightanim");
    sithCog_RegisterFunction(pTbl, sithCogFunctionSurface_GetSurfaceNormal, "getsurfacenormal");
    sithCog_RegisterFunction(pTbl, sithCogFunctionSurface_SyncSurface, "syncsurface");
    sithCog_RegisterFunction(pTbl, sithCogFunctionSurface_GetAdjoinAlpha, "getadjoinalpha");
    sithCog_RegisterFunction(pTbl, sithCogFunctionSurface_SetAdjoinAlpha, "setadjoinalpha");
}

void sithCogFunctionSurface_InstallHooks(void)
{
    J3D_HOOKFUNC(sithCogFunctionSurface_RegisterFunctions);
    J3D_HOOKFUNC(sithCogFunctionSurface_GetAdjoinAlpha);
    J3D_HOOKFUNC(sithCogFunctionSurface_SetAdjoinAlpha);
    J3D_HOOKFUNC(sithCogFunctionSurface_GetSurfaceAdjoin);
    J3D_HOOKFUNC(sithCogFunctionSurface_GetSurfaceSector);
    J3D_HOOKFUNC(sithCogFunctionSurface_GetNumSurfaceVertices);
    J3D_HOOKFUNC(sithCogFunctionSurface_GetSurfaceVertexPos);
    J3D_HOOKFUNC(sithCogFunctionSurface_SetHorizonSkyOffset);
    J3D_HOOKFUNC(sithCogFunctionSurface_GetHorizonSkyOffset);
    J3D_HOOKFUNC(sithCogFunctionSurface_SetCeilingSkyOffset);
    J3D_HOOKFUNC(sithCogFunctionSurface_GetCeilingSkyOffset);
    J3D_HOOKFUNC(sithCogFunctionSurface_SlideHorizonSky);
    J3D_HOOKFUNC(sithCogFunctionSurface_SlideCeilingSky);
    J3D_HOOKFUNC(sithCogFunctionSurface_SurfaceLightAnim);
    J3D_HOOKFUNC(sithCogFunctionSurface_SlideWall);
    J3D_HOOKFUNC(sithCogFunctionSurface_GetWallCel);
    J3D_HOOKFUNC(sithCogFunctionSurface_SetWallCel);
    J3D_HOOKFUNC(sithCogFunctionSurface_GetSurfaceMaterial);
    J3D_HOOKFUNC(sithCogFunctionSurface_SetSurfaceMaterial);
    J3D_HOOKFUNC(sithCogFunctionSurface_SetSurfaceFlags);
    J3D_HOOKFUNC(sithCogFunctionSurface_ClearSurfaceFlags);
    J3D_HOOKFUNC(sithCogFunctionSurface_GetSurfaceFlags);
    J3D_HOOKFUNC(sithCogFunctionSurface_SetAdjoinFlags);
    J3D_HOOKFUNC(sithCogFunctionSurface_ClearAdjoinFlags);
    J3D_HOOKFUNC(sithCogFunctionSurface_GetAdjoinFlags);
    J3D_HOOKFUNC(sithCogFunctionSurface_SetFaceType);
    J3D_HOOKFUNC(sithCogFunctionSurface_ClearFaceType);
    J3D_HOOKFUNC(sithCogFunctionSurface_GetFaceType);
    J3D_HOOKFUNC(sithCogFunctionSurface_SetFaceGeoMode);
    J3D_HOOKFUNC(sithCogFunctionSurface_GetFaceGeoMode);
    J3D_HOOKFUNC(sithCogFunctionSurface_SetFaceLightMode);
    J3D_HOOKFUNC(sithCogFunctionSurface_GetFaceLightMode);
    J3D_HOOKFUNC(sithCogFunctionSurface_SetSurfaceLight);
    J3D_HOOKFUNC(sithCogFunctionSurface_GetSurfaceLight);
    J3D_HOOKFUNC(sithCogFunctionSurface_GetSurfaceCenter);
    J3D_HOOKFUNC(sithCogFunctionSurface_GetSurfaceCount);
    J3D_HOOKFUNC(sithCogFunctionSurface_GetSurfaceNormal);
    J3D_HOOKFUNC(sithCogFunctionSurface_SyncSurface);
}

void sithCogFunctionSurface_ResetGlobals(void)
{

}

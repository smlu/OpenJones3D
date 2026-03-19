#include "sithShadow.h"
#include <j3dcore/j3dhook.h>

#include <sith/Engine/sithCollision.h>
#include <sith/Engine/sithPhysics.h>
#include <sith/RTI/symbols.h>
#include <sith/World/sithMaterial.h>
#include <sith/World/sithThing.h>
#include <sith/World/sithWorld.h>

#include <rdroid/Engine/rdCamera.h>
#include <rdroid/Engine/rdClip.h>
#include <rdroid/Math/rdMatrix.h>
#include <rdroid/Math/rdVector.h>
#include <rdroid/Math/rdMath.h>
#include <rdroid/Primitives/rdModel3.h>
#include <rdroid/Raster/rdCache.h>

#include <std/General/stdUtil.h>

static rdVector3 sithShadow_aVertices[4];
static rdVector3 sithShadow_aView[STD_ARRAYLEN(sithShadow_aVertices)];
static const rdVector2 sithShadow_aShadowUVs[STD_ARRAYLEN(sithShadow_aVertices)] = { { 0.0f, 1.0f }, { 0.0f, 0.0f }, { 1.0f, 0.0f }, { 1.0f, 1.0f } };

void sithShadow_InstallHooks(void)
{
    J3D_HOOKFUNC(sithShadow_RenderThingShadow);
    J3D_HOOKFUNC(sithShadow_SearchForAttachDistance);
    J3D_HOOKFUNC(sithShadow_DistanceThingToSurface);
    J3D_HOOKFUNC(sithShadow_DrawShadow);
    J3D_HOOKFUNC(sithShadow_DrawWalkShadow);
}

void sithShadow_ResetGlobals(void)
{}

void J3DAPI sithShadow_RenderThingShadow(SithThing* pThing)
{
    if ( pThing->renderData.data.pModel3 && (pThing->pInSector->flags & SITH_SECTOR_UNDERWATER) == 0 && (pThing->pInSector->flags & SITH_SECTOR_AETHERIUM) == 0 )
    {
        rdVector3 pos;
        rdVector_Copy3(&pos, &pThing->pos);

        float radius;
        float scale = 1.0f; // Added init to 1.0f
        float distance;

        if ( pThing->moveStatus == SITHPLAYERMOVE_CRAWLIDLE )
        {
            if ( sithShadow_SearchForAttachDistance(pThing, &distance, &radius) )
            {
                return;
            }
            scale = 1.0f;
        }
        else if ( pThing->moveStatus == SITHPLAYERMOVE_WHIPSWINGING )
        {
            if ( sithShadow_SearchForAttachDistance(pThing, &distance, &radius) )
            {
                return;
            }

            scale = 1.0f - (distance - pThing->renderData.data.pModel3->insertOffset.z) / radius;
        }
        else if ( pThing->attach.flags == SITH_ATTACH_SURFACE )
        {
            distance = sithShadow_DistanceThingToSurface(pThing, &pThing->attach.pFace->normal, pThing->attach.pFace);
            scale = 1.0f;
        }
        else
        {
            if ( sithShadow_SearchForAttachDistance(pThing, &distance, &radius) )
            {
                return;
            }

            scale = 1.0f - (distance - pThing->renderData.data.pModel3->insertOffset.z) / radius;
        }

        pos.z = pos.z - (distance - 0.0020000001f);

        rdMatrix34 orient;
        rdMatrix_Copy34(&orient, &pThing->orient);
        rdVector_Copy3(&orient.dvec, &pos);
        rdVector_Sub3Acc(&orient.dvec, &pThing->renderData.data.pModel3->aHierarchyNodes->pos);

        if ( strneq(pThing->aName, "mine", 4u) )
        {
            float size = pThing->renderData.data.pModel3->size;
            sithShadow_DrawShadow(&orient, size, scale, 1);
            return;
        }
        else if ( strneq(pThing->aName, "jeep", 4u) )
        {
            float size = pThing->renderData.data.pModel3->size * 1.8f;
            sithShadow_DrawShadow(&orient, size, scale, 1);
        }
        else // human
        {
            rdThing* prdThing = &pThing->renderData;
            const float size  = prdThing->data.pModel3->size;

            int lshoeIdx = sithThing_GetThingMeshIndex(pThing, "inlshoe");
            int rshoeIdx = sithThing_GetThingMeshIndex(pThing, "inrshoe");
            if ( lshoeIdx == -1 || rshoeIdx == -1 )
            {
                sithShadow_DrawShadow(&orient, size, scale, 0);
                return;
            }

            // Get left foot position
            rdMatrix34 meshOrient;
            rdModel3_GetMeshMatrix(prdThing, &rdroid_g_identMatrix34, lshoeIdx, &meshOrient);

            rdVector3 lshoePos = meshOrient.dvec;
            lshoePos.z = pos.z;

            // Get right foot position
            rdModel3_GetMeshMatrix(prdThing, &rdroid_g_identMatrix34, rshoeIdx, &meshOrient);
            rdVector3 rshoePos = meshOrient.dvec;
            rshoePos.z = pos.z;

            if ( pThing->moveStatus == SITHPLAYERMOVE_CRAWLIDLE )
            {
                sithShadow_DrawShadow(&orient, size, scale, 0);
            }
            else
            {
                sithShadow_DrawWalkShadow(size, scale, &lshoePos, &rshoePos, &orient.lvec, &orient.rvec);
            }
        }
    }
}

int J3DAPI sithShadow_SearchForAttachDistance(SithThing* pThing, float* distance, float* radius)
{
    SITH_ASSERTREL(pThing);
    if ( pThing->moveType != SITH_MT_PHYSICS || !pThing->renderData.data.pModel3 || !pThing->pInSector )
    {
        return 1;
    }

    rdVector3 moveNorm;
    rdVector_Neg3(&moveNorm, &rdroid_g_zVector3);

    float moveDist = sithPhysics_GetThingHeight(pThing);
    if ( moveDist == 0.0f )
    {
        moveDist = pThing->collide.movesize + 0.005f;
    }

    moveDist = moveDist * 4.0f;
    *radius = moveDist;
    if ( moveDist <= 0.0f )
    {
        return 1;
    }

    int someFlag = 0; // The purpose is unknown

    sithCollision_SearchForCollisions(pThing->pInSector, NULL, &pThing->pos, &moveNorm, moveDist, 0.0f, 0x2812);
    while ( 1 )
    {
        SithCollision* pCollision = sithCollision_PopStack();
        if ( !pCollision )
        {
            break;
        }

        if ( (pCollision->type & SITHCOLLISION_WORLD) != 0 )
        {
            *distance = sithShadow_DistanceThingToSurface(pThing, &pCollision->pSurfaceCollided->face.normal, &pCollision->pSurfaceCollided->face);
            sithCollision_DecreaseStackLevel();
            return 0;
        }

        if ( (pCollision->type & SITHCOLLISION_THING) != 0 )
        {
            SithThing* pHitThing = pCollision->pThingCollided;
            SITH_ASSERTREL(pHitThing);
            if ( pHitThing != pThing )
            {
                if ( !pCollision->pFaceCollided || !pCollision->pMeshCollided )
                {
                    SITHLOG_ERROR("Hit standon object but not enough collide info to attach.\n");
                    sithCollision_DecreaseStackLevel();
                    return 1;
                }

                rdVector3 hitDir;
                rdMatrix_TransformVector34(&hitDir, &pCollision->pFaceCollided->normal, &pCollision->pThingCollided->orient);

            #ifdef J3D_DEBUG
                if ( (someFlag & 0x10) == 0 || rdVector_Dot3(&hitDir, &rdroid_g_zVector3) >= 0.60000002f )// This check is removed in release
                #else
                J3D_UNUSED(someFlag);
            #endif

                {
                    rdVector3 hitfaceVert;
                    rdVector_Copy3(&hitfaceVert, &pCollision->pMeshCollided->apVertices[*pCollision->pFaceCollided->aVertices]);

                    rdMatrix_TransformVector34(&hitDir, &pCollision->pFaceCollided->normal, &pHitThing->orient);

                    rdVector3 hitPos;
                    rdMatrix_TransformVector34(&hitPos, &hitfaceVert, &pHitThing->orient);
                    rdVector_Add3Acc(&hitPos, &pHitThing->pos);

                    *distance = rdMath_DistancePointToPlane(&pThing->pos, &hitDir, &hitPos);

                    sithCollision_DecreaseStackLevel();
                    return 0;
                }
            }
        }
    }

    sithCollision_DecreaseStackLevel();
    return 1;
}

float J3DAPI sithShadow_DistanceThingToSurface(const SithThing* pThing, const rdVector3* normal, const rdFace* pFace)
{
    return rdMath_DistancePointToPlane(&pThing->pos, normal, &sithWorld_g_pCurrentWorld->aVertices[*pFace->aVertices]);
}

#ifdef J3D_OPENGL
static void J3DAPI sithShadow_DrawShadowStatic(const rdMatrix34* orient, float size, float scale, int bCar)
{
    if ( scale <= 0.0f )
    {
        return;
    }

    rdMaterial* pMat = NULL; // Added: Init to null
    if ( bCar )
    {
        pMat = sithMaterial_GetMaterialByIndex(SITHWORLD_STATICINDEX(2)); // jep_4shadow.mat - 0x8000 - SITH_STATICRESOURCE_INDEXMASK
    }
    else
    {
        pMat = sithMaterial_GetMaterialByIndex(SITHWORLD_STATICINDEX(0)); // gen_4indy_shadow.mat - 0x8000 - SITH_STATICRESOURCE_INDEXMAS
    }

    if ( !pMat )
    {
        return;
    }

    float shsize = size * scale / 2.0f;

    sithShadow_aVertices[0].x = -shsize;
    sithShadow_aVertices[0].y = -shsize;
    sithShadow_aVertices[0].z = 0.0f;

    sithShadow_aVertices[1].x = shsize;
    sithShadow_aVertices[1].y = -shsize;
    sithShadow_aVertices[1].z = 0.0f;

    sithShadow_aVertices[2].x = sithShadow_aVertices[1].x;
    sithShadow_aVertices[2].y = sithShadow_aVertices[1].x;
    sithShadow_aVertices[2].z = 0.0f;

    sithShadow_aVertices[3].x = -shsize;
    sithShadow_aVertices[3].y = sithShadow_aVertices[1].x;
    sithShadow_aVertices[3].z = 0.0f;

    // Rotate vertices to orient model matrix (world space) and transform to view (camera) space
    rdMatrix34 tmat;
    rdMatrix_Multiply34(&tmat, &rdCamera_g_pCurCamera->viewMatrix, orient);
    rdMatrix_TransformPointList34(&tmat, sithShadow_aVertices, sithShadow_aView, STD_ARRAYLEN(sithShadow_aView));

    rdPayload* pPayload        = rdCache_GetTransparentDrawCall(RD_DRAW_SHADOW);
    rdShadowPayload* pDrawCall = pPayload->payload;

    float distance = FLT_MAX;
    for ( size_t i = 0; i < 4; i++ )
    {
        pDrawCall->vertices[i] = sithShadow_aView[i];

        pDrawCall->texCoords[i] = sithShadow_aShadowUVs[i];

        if ( sithShadow_aView[i].y < distance )
        {
            distance = sithShadow_aView[i].y;
        }
    }
    pPayload->distance = distance;


    pPayload->lightingMode = RD_LIGHTING_GOURAUD;
    pPayload->flags        = RD_FF_ZWRITE_DISABLED | RD_FF_TEX_CLAMP_Y | RD_FF_TEX_CLAMP_X | RD_FF_TEX_TRANSLUCENT;
    pPayload->pMaterial    = pMat;
    pPayload->matCelNum    = -1;


    float alpha          = scale * 0.80000001f + 0.2f;
    rdVector4 extraLight = { 1.0f, 1.0f, 1.0f, alpha };
    pPayload->extraLight = extraLight;


    rdCache_AddTransparentDrawCall();
}

#endif


void J3DAPI sithShadow_DrawShadow(const rdMatrix34* orient, float size, float scale, int bCar)
{
#ifdef J3D_OPENGL
    sithShadow_DrawShadowStatic(orient, size, scale, bCar);
    return;
#endif
    if ( scale <= 0.0f ) {
        return;
    }

    rdMaterial* pMat = NULL; // Added: Init to null
    if ( bCar ) {
        pMat = sithMaterial_GetMaterialByIndex(SITHWORLD_STATICINDEX(2));// jep_4shadow.mat - 0x8000 - SITH_STATICRESOURCE_INDEXMASK
    }
    else {
        pMat = sithMaterial_GetMaterialByIndex(SITHWORLD_STATICINDEX(0));// gen_4indy_shadow.mat - 0x8000 - SITH_STATICRESOURCE_INDEXMAS
    }

    if ( !pMat ) {
        return;
    }

    rdCacheProcEntry* pPoly = rdCache_GetAlphaProcEntry();
    if ( !pPoly ) {
        return;
    }

    float shsize = size * scale / 2.0f;

    sithShadow_aVertices[0].x = -shsize;
    sithShadow_aVertices[0].y = -shsize;
    sithShadow_aVertices[0].z = 0.0f;

    sithShadow_aVertices[1].x = shsize;
    sithShadow_aVertices[1].y = -shsize;
    sithShadow_aVertices[1].z = 0.0f;

    sithShadow_aVertices[2].x = sithShadow_aVertices[1].x;
    sithShadow_aVertices[2].y = sithShadow_aVertices[1].x;
    sithShadow_aVertices[2].z = 0.0f;

    sithShadow_aVertices[3].x = -shsize;
    sithShadow_aVertices[3].y = sithShadow_aVertices[1].x;
    sithShadow_aVertices[3].z = 0.0f;

    // Rotate vertices to orient model matrix (world space) and transform to view (camera) space
    rdMatrix34 tmat;
    rdMatrix_Multiply34(&tmat, &rdCamera_g_pCurCamera->viewMatrix, orient);
    rdMatrix_TransformPointList34(&tmat, sithShadow_aVertices, sithShadow_aView, STD_ARRAYLEN(sithShadow_aView));

    // Project vertices to screen space and assign to poly
    rdClip_VerticesToPlane(pPoly, sithShadow_aView, sithShadow_aShadowUVs, STD_ARRAYLEN(sithShadow_aShadowUVs));

    pPoly->lightingMode = RD_LIGHTING_GOURAUD;
    pPoly->flags        = RD_FF_ZWRITE_DISABLED | RD_FF_TEX_CLAMP_Y | RD_FF_TEX_CLAMP_X | RD_FF_TEX_TRANSLUCENT;
    pPoly->pMaterial    = pMat;
    pPoly->matCelNum    = -1;

    float alpha = scale * 0.80000001f + 0.2f;
    rdVector_Set4(&pPoly->aVertIntensities[0], 1.0f, 1.0f, 1.0f, alpha);
    rdVector_Set4(&pPoly->aVertIntensities[1], 1.0f, 1.0f, 1.0f, alpha);
    rdVector_Set4(&pPoly->aVertIntensities[2], 1.0f, 1.0f, 1.0f, alpha);
    rdVector_Set4(&pPoly->aVertIntensities[3], 1.0f, 1.0f, 1.0f, alpha);

    rdVector_Set4(&pPoly->extraLight, 1.0f, 1.0f, 1.0f, 1.0f);

    rdCache_AddAlphaProcFace(4u);
}

#ifdef J3D_OPENGL
static void J3DAPI sithShadow_DrawWalkShadowStatic(float size, float scale, const rdVector3* leg, const rdVector3* rleg, const rdVector3* lvec, const rdVector3* rvec)
{
    if ( scale <= 0.0f )
    {
        return;
    }

    rdMaterial* pMat = sithMaterial_GetMaterialByIndex(SITHWORLD_STATICINDEX(1)); // gen_4indy_shadow_stretch.mat
    if ( !pMat )
    {
        return;
    }

    rdVector3 ldir;
    rdVector_Normalize3(&ldir, lvec);

    rdVector3 rdir;
    rdVector_Normalize3(&rdir, rvec);

    float sscale = size * scale / 8.0f;
    float escale = 0.0074999998f * scale; // / 133.3333f

    float sx = rdVector_Dot3(&ldir, &rdroid_g_xVector3) * sscale;
    float sy = rdVector_Dot3(&ldir, &rdroid_g_yVector3) * sscale;

    float ex = rdVector_Dot3(&rdir, &rdroid_g_xVector3) * escale;
    float ey = rdVector_Dot3(&rdir, &rdroid_g_yVector3) * escale;

    sithShadow_aVertices[0].x = leg->x - sx - ex;
    sithShadow_aVertices[0].y = leg->y - sy - ey;
    sithShadow_aVertices[0].z = leg->z;

    sithShadow_aVertices[1].x = leg->x + sx - ex;
    sithShadow_aVertices[1].y = leg->y + sy - ey;
    sithShadow_aVertices[1].z = leg->z;

    sithShadow_aVertices[2].x = rleg->x + sx + ex;
    sithShadow_aVertices[2].y = rleg->y + sy + ey;
    sithShadow_aVertices[2].z = rleg->z;

    sithShadow_aVertices[3].x = rleg->x - sx + ex;
    sithShadow_aVertices[3].y = rleg->y - sy + ey;
    sithShadow_aVertices[3].z = rleg->z;

    // Transform vertices to view space
    rdMatrix_TransformPointList34(&rdCamera_g_pCurCamera->viewMatrix, sithShadow_aVertices, sithShadow_aView, STD_ARRAYLEN(sithShadow_aView));

    rdPayload* pPayload        = rdCache_GetTransparentDrawCall(RD_DRAW_SHADOW);
    rdShadowPayload* pDrawCall = pPayload->payload;

    float distance = FLT_MAX;
    for ( size_t i = 0; i < 4; i++ )
    {
        pDrawCall->vertices[i] = sithShadow_aView[i];

        pDrawCall->texCoords[i] = sithShadow_aShadowUVs[i];

        if ( sithShadow_aView[i].y < distance )
        {
            distance = sithShadow_aView[i].y;
        }
    }
    pPayload->distance     = distance;
    pPayload->lightingMode = RD_LIGHTING_GOURAUD;
    pPayload->flags        = RD_FF_ZWRITE_DISABLED | RD_FF_TEX_CLAMP_Y | RD_FF_TEX_CLAMP_X | RD_FF_TEX_TRANSLUCENT | RD_FF_DOUBLE_SIDED;
    pPayload->pMaterial    = pMat;
    pPayload->matCelNum    = -1;

    float alpha          = scale * 0.8f + 0.2f;
    rdVector4 extraLight = { 1.0f, 1.0f, 1.0f, alpha };
    pPayload->extraLight = extraLight;


    rdCache_AddTransparentDrawCall();
}

#endif


void J3DAPI sithShadow_DrawWalkShadow(float size, float scale, const rdVector3* leg, const rdVector3* rleg, const rdVector3* lvec, const rdVector3* rvec)
{
#ifdef J3D_OPENGL
    sithShadow_DrawWalkShadowStatic(size, scale, leg, rleg, lvec, rvec);
    return;
#endif

    if ( scale <= 0.0f ) {
        return;
    }

    rdMaterial* pMat = sithMaterial_GetMaterialByIndex(SITHWORLD_STATICINDEX(1));// gen_4indy_shadow_stretch.mat
    if ( !pMat ) {
        return;
    }

    rdCacheProcEntry* pPoly = rdCache_GetAlphaProcEntry();
    if ( !pPoly ) { // Added: Null check
        return;
    }

    rdVector3 ldir;
    rdVector_Normalize3(&ldir, lvec);

    rdVector3 rdir;
    rdVector_Normalize3(&rdir, rvec);

    float sscale = size * scale / 8.0f;
    float escale = 0.0074999998f * scale; // / 133.3333f

    float sx = rdVector_Dot3(&ldir, &rdroid_g_xVector3) * sscale;
    float sy = rdVector_Dot3(&ldir, &rdroid_g_yVector3) * sscale;

    float ex = rdVector_Dot3(&rdir, &rdroid_g_xVector3) * escale;
    float ey = rdVector_Dot3(&rdir, &rdroid_g_yVector3) * escale;

    sithShadow_aVertices[0].x = leg->x - sx - ex;
    sithShadow_aVertices[0].y = leg->y - sy - ey;
    sithShadow_aVertices[0].z = leg->z;

    sithShadow_aVertices[1].x = leg->x + sx - ex;
    sithShadow_aVertices[1].y = leg->y + sy - ey;
    sithShadow_aVertices[1].z = leg->z;

    sithShadow_aVertices[2].x = rleg->x + sx + ex;
    sithShadow_aVertices[2].y = rleg->y + sy + ey;
    sithShadow_aVertices[2].z = rleg->z;

    sithShadow_aVertices[3].x = rleg->x - sx + ex;
    sithShadow_aVertices[3].y = rleg->y - sy + ey;
    sithShadow_aVertices[3].z = rleg->z;

    // Transform vertices to view space
    rdMatrix_TransformPointList34(&rdCamera_g_pCurCamera->viewMatrix, sithShadow_aVertices, sithShadow_aView, STD_ARRAYLEN(sithShadow_aView));

    // Project vertices to screen space and assign to poly
    rdClip_VerticesToPlane(pPoly, sithShadow_aView, sithShadow_aShadowUVs, STD_ARRAYLEN(sithShadow_aShadowUVs));

    pPoly->lightingMode = RD_LIGHTING_GOURAUD;
    pPoly->flags        = RD_FF_ZWRITE_DISABLED | RD_FF_TEX_CLAMP_Y | RD_FF_TEX_CLAMP_X | RD_FF_TEX_TRANSLUCENT;
    pPoly->pMaterial    = pMat;
    pPoly->matCelNum    = -1;

    float alpha = scale * 0.8f + 0.2f;
    rdVector_Set4(&pPoly->aVertIntensities[0], 1.0f, 1.0f, 1.0f, alpha);
    rdVector_Set4(&pPoly->aVertIntensities[1], 1.0f, 1.0f, 1.0f, alpha);
    rdVector_Set4(&pPoly->aVertIntensities[2], 1.0f, 1.0f, 1.0f, alpha);
    rdVector_Set4(&pPoly->aVertIntensities[3], 1.0f, 1.0f, 1.0f, alpha);

    rdVector_Set4(&pPoly->extraLight, 1.0f, 1.0f, 1.0f, 1.0f);

    rdCache_AddAlphaProcFace(STD_ARRAYLEN(sithShadow_aVertices));
}

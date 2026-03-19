#include "rdSprite.h"
#include <j3dcore/j3dhook.h>

#include <rdroid/Engine/rdCamera.h>
#include <rdroid/Engine/rdClip.h>
#include <rdroid/Engine/rdMaterial.h>
#include <rdroid/Main/rdroid.h>
#include <rdroid/Math/rdMath.h>
#include <rdroid/Math/rdMatrix.h>
#include <rdroid/Math/rdVector.h>
#include <rdroid/Raster/rdCache.h>
#include <rdroid/RTI/symbols.h>

#include <sith/World/sithWorld.h>

#include <std/General/stdMemory.h>
#include <std/General/stdUtil.h>

rdMatrix34 rdSprite_aView;

void rdSprite_InstallHooks(void)
{
    J3D_HOOKFUNC(rdSprite_NewEntry);
    J3D_HOOKFUNC(rdSprite_FreeEntry);
    J3D_HOOKFUNC(rdSprite_Draw);
}

void rdSprite_ResetGlobals(void)
{}

rdSprite3* J3DAPI rdSprite_New(int type, const char* pName, const char* pMatNam, float width, float height, rdGeometryMode geomode, rdLightMode lightmode, const rdVector4* pExtraLight, const rdVector3* pOffset)
{
    rdSprite3* pSprite = (rdSprite3*)STDMALLOC(sizeof(rdSprite3));
    if ( !pSprite )
    {
        RDLOG_ERROR("Error allocating memory for sprite.\n");
        return NULL;
    }

    rdSprite_NewEntry(pSprite, pName, type, pMatNam, width, height, geomode, lightmode, pExtraLight, pOffset);
    return pSprite;
}

int J3DAPI rdSprite_NewEntry(rdSprite3* pSprite, const char* pName, int type, const char* pMatName, float width, float height, rdGeometryMode geomode, rdLightMode lightmode, const rdVector4* pExtraLight, const rdVector3* pOffset)
{
    if ( pName )
    {
        STD_STRCPY(pSprite->aName, pName);
    }

    pSprite->type              = type;
    pSprite->width             = width;
    pSprite->height            = height;
    pSprite->face.flags        = RD_FF_FOG_ENABLED | RD_FF_DOUBLE_SIDED;
    pSprite->face.geometryMode = geomode;
    pSprite->face.lightingMode = lightmode;
    pSprite->offset            = *pOffset;
    pSprite->face.extraLight   = *pExtraLight;

    pSprite->face.pMaterial = rdMaterial_Load(pMatName);
    if ( !pSprite->face.pMaterial )
    {
        return 0;
    }

    pSprite->face.numVertices = 4;
    pSprite->face.aVertices = (int*)STDMALLOC(sizeof(*pSprite->face.aVertices) * pSprite->face.numVertices);
    if ( !pSprite->face.aVertices )
    {
        goto alloc_error;
    }

    if ( pSprite->face.geometryMode <= RD_GEOMETRY_SOLID )
    {
        for ( size_t i = 0; i < pSprite->face.numVertices; ++i )
        {
            pSprite->face.aVertices[i] = i;
        }
    }
    else
    {

        pSprite->face.aTexVertices = (int*)STDMALLOC(sizeof(*pSprite->face.aTexVertices) * pSprite->face.numVertices);
        if ( !pSprite->face.aTexVertices )
        {
            goto alloc_error;
        }

        for ( size_t i = 0; i < pSprite->face.numVertices; ++i )
        {
            pSprite->face.aVertices[i]    = i;
            pSprite->face.aTexVertices[i] = i;
        }

        pSprite->aTexVerts = (rdVector2*)STDMALLOC(sizeof(*pSprite->aTexVerts) * pSprite->face.numVertices);
        if ( !pSprite->aTexVerts )
        {
            goto alloc_error;
        }

        pSprite->aTexVerts->x = 0.0f;
        pSprite->aTexVerts->y = 0.0f;

        pSprite->aTexVerts[1].x = 1.0f;
        pSprite->aTexVerts[1].y = 0.0f;

        pSprite->aTexVerts[2].x = 1.0f;
        pSprite->aTexVerts[2].y = 1.0f;

        pSprite->aTexVerts[3].x = 0.0f;
        pSprite->aTexVerts[3].y = 1.0f;
    }

    // Success
    pSprite->widthHalf  = pSprite->width / 2.0f;
    pSprite->heightHalf = pSprite->height / 2.0f;
    pSprite->radius     = sqrtf(pSprite->widthHalf * pSprite->widthHalf + pSprite->heightHalf * pSprite->heightHalf);
    return 1;

alloc_error:
    RDLOG_ERROR("Error: Bad memory allocation for sprite '%'.\n", pName);
    return 0;
}

void J3DAPI rdSprite_Free(rdSprite3* pSprite)
{
    if ( !pSprite )
    {
        RDLOG_ERROR("Warning: Attempt to free NULL sprite ptr.\n");
        return;
    }

    rdSprite_FreeEntry(pSprite);
    stdMemory_Free(pSprite);
}

void J3DAPI rdSprite_FreeEntry(rdSprite3* pSprite)
{
    if ( pSprite->aTexVerts )
    {
        stdMemory_Free(pSprite->aTexVerts);
        pSprite->aTexVerts = NULL;
    }

    if ( pSprite->face.aVertices )
    {
        stdMemory_Free(pSprite->face.aVertices);
        pSprite->face.aVertices = NULL;
    }

    if ( pSprite->face.aTexVertices )
    {
        stdMemory_Free(pSprite->face.aTexVertices);
        pSprite->face.aTexVertices = NULL;
    }
}
#ifdef J3D_OPENGL
static int J3DAPI rdSprite_DrawStatic(rdThing* prdThing, const rdMatrix34* orient)
{
    rdSprite3* pSprite3 = prdThing->data.pSprite3;

    RdFrustumCull frustumCull = prdThing->frustumCull;
    if ( frustumCull == RDFRUSTUMCULL_OUTSIDE )
    {
        return 0;
    }
    rdPayload* pPayload        = rdCache_GetTransparentDrawCall(RD_DRAW_SPRITE);
    rdSpritePayload* pDrawCall = pPayload->payload;
    pDrawCall->spriteType      = pSprite3->type;

    rdVector3 tpos         = { 0 };
    pDrawCall->modelMatrix = rdroid_g_identMatrix34;
    if ( pSprite3->type == 0 )
    {
        // Transform position to view space
        rdMatrix_TransformPoint34(&tpos, &orient->dvec, &rdCamera_g_pCurCamera->viewMatrix);
        pDrawCall->spriteOffset = pSprite3->offset;

        if ( pSprite3->rollAngle != 0.0f )
        {
            rdMatrix_BuildFromVectorAngle34(&pDrawCall->modelMatrix, &rdroid_g_yVector3, pSprite3->rollAngle);
        }
    }
    else if ( pSprite3->type == 2 )
    {
        tpos                    = orient->dvec;
        pDrawCall->spriteOffset = orient->lvec;
        if ( pSprite3->rollAngle != 0.0f )
        {
            rdMatrix_BuildFromVectorAngle34(&pDrawCall->modelMatrix, &orient->lvec, pSprite3->rollAngle);
        }
    }
    pDrawCall->modelMatrix.dvec = tpos;
    pSprite3->rollAngle         = 0.0f;


    pDrawCall->spriteSize.x = pSprite3->widthHalf;
    pDrawCall->spriteSize.y = pSprite3->heightHalf;


    rdVector4 extraLight = pSprite3->face.extraLight;

    if ( (rdroid_g_curRenderOptions & RDROID_USE_AMBIENT_CAMERA_LIGHT) != 0 )
    {
        rdVector_Add4Acc(&extraLight, &rdCamera_g_pCurCamera->ambientLight);
        rdMath_ClampVector4Acc(&extraLight, 0.0f, 1.0f); // Added: Clamp to [0,1.0]
    }
    pPayload->pMaterial  = pSprite3->face.pMaterial;
    pPayload->matCelNum  = prdThing->matCelNum;
    pPayload->extraLight = extraLight;

    pPayload->flags = RD_FF_FOG_ENABLED | RD_FF_ZWRITE_DISABLED | RD_FF_TEX_CLAMP_Y | RD_FF_TEX_CLAMP_X | RD_FF_TEX_TRANSLUCENT;

    // Fixed: Disable fog rendering for poly when fog is globally disabled
    //        OG: Poly fog rendering was enabled by default which lead to undesired render effect when fog is disabled in level (i.e.: fog color is applied)
    if ( !sithWorld_g_pCurrentWorld->fog.bEnabled ) // TODO: add special function that will enable/disable fog rendering
    {
        pPayload->flags &= ~RD_FF_FOG_ENABLED;
    }
    rdVector3 camPos;
    rdMatrix_TransformPoint34(&camPos, &orient->dvec, &rdCamera_g_pCurCamera->viewMatrix);
    pPayload->distance     = camPos.y;
    pPayload->lightingMode = J3DMIN(J3DMIN(pSprite3->face.lightingMode, RD_LIGHTING_DIFFUSE), rdroid_g_curLightingMode);
    rdCache_AddTransparentDrawCall();

    return 1;
}
#endif

int J3DAPI rdSprite_Draw(rdThing* prdThing, const rdMatrix34* orient)
{
#ifdef J3D_OPENGL
    return rdSprite_DrawStatic(prdThing, orient);
#endif

    rdSprite3* pSprite3 = prdThing->data.pSprite3;

    rdVector3 tpos = { 0 };
    rdVector3 yVec = { 0 };

    if ( pSprite3->type == 0 )
    {
        // Transform position to view space
        rdMatrix_TransformPoint34(&tpos, &orient->dvec, &rdCamera_g_pCurCamera->viewMatrix);
    }
    else if ( pSprite3->type == 2 )
    {
        tpos = orient->dvec;
        yVec = orient->lvec;
    }

    RdFrustumCull frustumCull = prdThing->frustumCull;
    if ( frustumCull == RDFRUSTUMCULL_OUTSIDE )
    {
        return 0;
    }

    rdCacheProcEntry* pPoly = rdCache_GetAlphaProcEntry();
    if ( !pPoly )
    {
        return 0;
    }

    if ( pSprite3->type == 0 )
    {
        rdSprite_aView.rvec.x = pSprite3->offset.x - pSprite3->widthHalf;
        rdSprite_aView.rvec.y = pSprite3->offset.y;
        rdSprite_aView.rvec.z = pSprite3->heightHalf + pSprite3->offset.z;

        rdSprite_aView.lvec.x = pSprite3->widthHalf + pSprite3->offset.x;
        rdSprite_aView.lvec.y = pSprite3->offset.y;
        rdSprite_aView.lvec.z = pSprite3->heightHalf + pSprite3->offset.z;

        rdSprite_aView.uvec.x = pSprite3->widthHalf + pSprite3->offset.x;
        rdSprite_aView.uvec.y = pSprite3->offset.y;
        rdSprite_aView.uvec.z = pSprite3->offset.z - pSprite3->heightHalf;

        rdSprite_aView.dvec.x = pSprite3->offset.x - pSprite3->widthHalf;
        rdSprite_aView.dvec.y = pSprite3->offset.y;
        rdSprite_aView.dvec.z = pSprite3->offset.z - pSprite3->heightHalf;

        if ( pSprite3->rollAngle != 0.0f )
        {
            rdVector3 vecPYR = { 0 };
            vecPYR.z = pSprite3->rollAngle;
            rdMatrix_PostRotate34(&rdSprite_aView, &vecPYR);
            pSprite3->rollAngle = 0.0f;
        }

        rdVector_Add3Acc(&rdSprite_aView.rvec, &tpos);
        rdVector_Add3Acc(&rdSprite_aView.lvec, &tpos);
        rdVector_Add3Acc(&rdSprite_aView.uvec, &tpos);
        rdVector_Add3Acc(&rdSprite_aView.dvec, &tpos);
    }
    else if ( pSprite3->type == 2 )
    {
        // TODO: Due to fixed normalization some sprites might have too large sizes (width/height) here. 
        //       One such case was raft splash created by sithFX_CreateRaftSplatterFX(, 1) which is fixed now.
        //       Do more testing!

        rdVector3 xVec;
        if ( fabsf(yVec.z) < 0.99989998f ) // Fixed: OG -> if ( fabsf((double)(lvec.z >= 0.99989998f)) == 0.0f )
        {
            rdVector_Cross3(&xVec, &rdroid_g_zVector3, &yVec);
            rdVector_Normalize3Acc(&xVec); // Fixed: OG forgot to do normalization. Note this fixes the proper size of sprites, e.g. row ripple sprites
        }
        else
        {
            rdVector_Cross3(&xVec, &rdroid_g_xVector3, &yVec);
            rdVector_Normalize3Acc(&xVec); // Fixed: OG forgot to do normalization
        }

        rdVector3 zVec;
        rdVector_Cross3(&zVec, &xVec, &yVec);
        rdVector_Normalize3Acc(&zVec); // Fixed: OG forgot to do normalization

        rdVector3 right;
        rdVector_Scale3(&right, &xVec, pSprite3->widthHalf);

        rdVector3 up;
        rdVector_Scale3(&up, &zVec, pSprite3->heightHalf);

        rdVector3 left;
        rdVector_Neg3(&left, &right);

        rdSprite_aView.rvec.x = left.x - up.x;
        rdSprite_aView.rvec.y = left.y - up.y;
        rdSprite_aView.rvec.z = left.z - up.z;

        rdSprite_aView.lvec.x = right.x - up.x;
        rdSprite_aView.lvec.y = right.y - up.y;
        rdSprite_aView.lvec.z = right.z - up.z;

        rdSprite_aView.uvec.x = right.x + up.x;
        rdSprite_aView.uvec.y = right.y + up.y;
        rdSprite_aView.uvec.z = right.z + up.z;

        rdSprite_aView.dvec.x = left.x + up.x;
        rdSprite_aView.dvec.y = left.y + up.y;
        rdSprite_aView.dvec.z = left.z + up.z;

        if ( pSprite3->rollAngle != 0.0f )
        {
            rdMatrix34 tmat;
            rdMatrix_BuildFromVectorAngle34(&tmat, &yVec, pSprite3->rollAngle);
            rdMatrix_PostMultiply34(&rdSprite_aView, &tmat);
            pSprite3->rollAngle = 0.0f;
        }

        rdVector_Add3Acc(&rdSprite_aView.rvec, &tpos);
        rdVector_Add3Acc(&rdSprite_aView.lvec, &tpos);
        rdVector_Add3Acc(&rdSprite_aView.uvec, &tpos);
        rdVector_Add3Acc(&rdSprite_aView.dvec, &tpos);
    }

    if ( pSprite3->type == 2 )
    {
        for ( size_t i = 0; i < 4; ++i )
        {
            // Transform matrix to view space
            rdVector3 tvec;
            rdMatrix_TransformPoint34(&tvec, (const rdVector3*)&rdSprite_aView.rvec + i, &rdCamera_g_pCurCamera->viewMatrix);
            rdVector_Copy3(&rdSprite_aView.rvec + i, &tvec);
        }
    }

#ifdef J3D_OPENGL
    // For now transform sprite vertices back to world space. Later when each thing has its own vao, it's better to do everything in shader.
    if ( std3D_GetCurrentDrawState() == STD3D_DS_THINGS )
    {
        for ( size_t i = 0; i < 4; ++i )
        {
            rdMatrix_TransformPoint34Acc(&rdSprite_aView.rvec + i, &rdCamera_g_camMatrix);
        }
    }
#endif

    // Project vertices to screen space and assign them to pPoly
    if ( !rdClip_FaceToPlane(rdCamera_g_pCurCamera->pFrustum, pPoly, &pSprite3->face, &rdSprite_aView.rvec, pSprite3->aTexVerts, NULL, NULL) )
    {
        // Poly is fully outside the camera frustum
        return 0;
    }

    pPoly->aVertIntensities->alpha = pSprite3->face.extraLight.alpha;
    pPoly->extraLight = pSprite3->face.extraLight;

    if ( (rdroid_g_curRenderOptions & RDROID_USE_AMBIENT_CAMERA_LIGHT) != 0 )
    {
        rdVector_Add4Acc(&pPoly->extraLight, &rdCamera_g_pCurCamera->ambientLight);
        rdMath_ClampVector4Acc(&pPoly->extraLight, 0.0f, 1.0f); // Added: Clamp to [0,1.0]
    }

    pPoly->matCelNum = prdThing->matCelNum;
    pPoly->flags     = pSprite3->face.flags;
    pPoly->flags    |=  RD_FF_FOG_ENABLED | RD_FF_ZWRITE_DISABLED | RD_FF_TEX_CLAMP_Y | RD_FF_TEX_CLAMP_X | RD_FF_TEX_TRANSLUCENT;

    // Fixed: Disable fog rendering for poly when fog is globally disabled
    //        OG: Poly fog rendering was enabled by default which lead to undesired render effect when fog is disabled in level (i.e.: fog color is applied)
    if ( !sithWorld_g_pCurrentWorld->fog.bEnabled ) // TODO: add special function that will enable/disable fog rendering
    {
        pPoly->flags &= ~RD_FF_FOG_ENABLED;
    }

    pPoly->pMaterial    = pSprite3->face.pMaterial;
    pPoly->lightingMode = J3DMIN(J3DMIN(pSprite3->face.lightingMode, RD_LIGHTING_DIFFUSE), rdroid_g_curLightingMode);
    rdCache_AddAlphaProcFace(pSprite3->face.numVertices);
    return 1;
}

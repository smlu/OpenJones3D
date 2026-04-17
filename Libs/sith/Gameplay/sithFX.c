#include "sithFX.h"
#include <j3dcore/j3dhook.h>

#include <sith/Engine/sithAnimate.h>
#include <sith/Engine/sithCollision.h>
#include <sith/Engine/sithPhysics.h>
#include <sith/Engine/sithParticle.h>
#include <sith/Gameplay/sithTime.h>
#include <sith/Main/sithMain.h>
#include <sith/RTI/symbols.h>
#include <sith/World/sithMaterial.h>
#include <sith/World/sithSprite.h>
#include <sith/World/sithTemplate.h>
#include <sith/World/sithThing.h>
#include <sith/World/sithWorld.h>

#include <rdroid/Engine/rdThing.h>
#include <rdroid/Math/rdMath.h>
#include <rdroid/Math/rdMatrix.h>
#include <rdroid/Math/rdVector.h>
#include <rdroid/Primitives/rdParticle.h>
#include <rdroid/Primitives/rdPolyline.h>

#include <std/General/stdConffile.h>
#include <std/General/stdMemory.h>
#include <std/General/stdUtil.h>

const char* sithFX_aChalkMarkNames[35] =
{
    "+1947_mark",
    "+alien_mark",
    "+ankh_mark",
    "+ark_mark",
    "+cccr_mark",
    "+check_mark",
    "+cir_mark",
    "+cross_mark",
    "+eye_mark",
    "+five_mark",
    "+grail_mark",
    "+guy_mark",
    "+happy_mark",
    "+hawk_mark",
    "+here_mark",
    "+indy_mark",
    "+indy2_mark",
    "+la_mark",
    "+love_mark",
    "+portrait_mark",
    "+pound_mark",
    "+pyramid_mark",
    "+question_mark",
    "+r2d2_mark",
    "+skull_mark",
    "+snake_mark",
    "+square_mark",
    "+star_mark",
    "+stones_mark",
    "+sunrise_mark",
    "+tentacle_mark",
    "+tictactoe_mark",
    "+triangle_mark",
    "+usa_mark",
    "+x_mark"
};

rdVector3 sithFX_curPaddleRipplePos     = { 0 }; // Added: Init to 0
float sithFX_secLastCreatedPaddleRipple = 0.0f;  // Added: Init to 0

rdVector4 sithFX_rippleExtraLight       = { 0.35f, 0.35f, 0.35f, 1.0f };
float sithFX_surfaceOffsetZ             = 0.001f; // Altered: Was 0.002f

bool sithFX_bWakeRippleLoaded           = false;
rdSprite3* sithFX_pWakeRippleSprite     = NULL;
float sithFX_raftRippleAlpha            = J3D_QOL_VALUE(0.35f, 1.0f);

rdVector3 sithFx_rightTireStartPosOffset = { 0 }; // Added: Init to 0
rdVector3 sithFx_leftTireStartPosOffset  = { 0 }; // Added: Init to 0

size_t sithFX_fariyDustSizeFactor         = 0; // Added: Init to 0
size_t sithFX_newFairyDustDeluxSizeFactor = 0; // Added: Init to 0
size_t sithFX_fairyDustDeluxSizeFactor    = 0; // Added: Init to 0
unsigned int sithFX_msecLastCreatedFairyDustDeluxTime = 0; // Added: Init to 0

static SithThing* sithFX_CreateThingFacingUp(const SithThing* pRippleTpl, const rdVector3* pos, SithSector* pSector, const rdVector3* upDir)
{
    SithThing* pRipple = sithThing_CreateThingAtPos(pRippleTpl, pos, &pRippleTpl->orient, pSector, NULL);
    if ( !pRipple )
    {
        return NULL;
    }

    // Orient the ripple to face upwards
    pRipple->orient.lvec = *upDir; // Note: OG in most case used zVector3
    return pRipple;
}

static void sithFX_TransformPointToAttachSurface(rdVector3* point, const SithThing* pThing)
{
    rdVector_Add3Acc(point, &pThing->pos);
    point->z -= rdMath_DistancePointToPlane(&pThing->pos, &pThing->attach.pFace->normal, &pThing->attach.attachedFaceFirstVert) - sithFX_surfaceOffsetZ; // Fixed: Move sprite slightly above water surface; OG was below surface i.e.: +0.002f
}

static void sithFX_RotateSpriteToMovement(SithThing* pSprite, const rdVector3* velocity)
{
    rdVector3 moveNorm;
    rdVector_Normalize3(&moveNorm, velocity);
    float rollAngle = rdMath_DeltaAngleNormalized(&rdroid_g_xVector3, &moveNorm, &rdroid_g_zVector3);
    pSprite->thingInfo.spriteInfo.rollAngle = rollAngle;
}

void J3DAPI sithFX_CreatePaddleWaterSplash(SithThing* pThing, const rdVector3* pos);

bool J3DAPI sithFX_GetWaterPosition(SithThing* pThing, rdVector3* waterPos, SithSector* waterSec, rdVector3* waterNormal)
{
    SITH_ASSERT(waterPos && waterSec && waterNormal);

    rdVector3 rippleStartPos = *waterPos;
    rippleStartPos.z = pThing->pos.z;

    SithSector* pStartSec = sithCollision_FindSectorInRadius(pThing->pInSector, &pThing->pos, &rippleStartPos, 0.0f);
    if ( pStartSec )
    {
        rdVector3 endPos = rippleStartPos;
        rdVector_MultAcc3(&endPos, &RDVECTOR_NEG3(rdroid_g_zVector3), sithPhysics_GetThingHeight(pThing));

        SithSurface* pWaterSurf = sithCollision_FindWaterSurface(pStartSec, &rippleStartPos, &endPos, sithFX_surfaceOffsetZ);
        if ( pWaterSurf )
        {
            waterSec     = pWaterSurf->pSector;
            *waterNormal = pWaterSurf->face.normal;

            // Assign found position
            // Note: No need to shift Z for sithFX_surfaceOffsetZ here.
            // The water surface is found via collision raycast using a search radius;
            // the returned position is not adjusted by that radius (foundPos + radius
            // equals the actual water surface height).
            *waterPos = endPos;
            return true;
        }
    }

    return false;
}

void sithFX_InstallHooks(void)
{
    J3D_HOOKFUNC(sithFX_Reset);
    J3D_HOOKFUNC(sithFX_ClearChalkMarks);
    J3D_HOOKFUNC(sithFX_DestroyFairyDustDeluxe);
    J3D_HOOKFUNC(sithFX_CreateFairyDust);
    J3D_HOOKFUNC(sithFX_SetFairyDustSize);
    J3D_HOOKFUNC(sithFX_UpdateFairyDustUserBlock);
    J3D_HOOKFUNC(sithFX_CreateFairyDustDelux);
    J3D_HOOKFUNC(sithFX_CreateFairyDustDeluxDusts);
    J3D_HOOKFUNC(sithFX_CreateBubble);
    J3D_HOOKFUNC(sithFX_CreateWaterRipple);
    J3D_HOOKFUNC(sithFX_CreateRaftSplatterFX);
    J3D_HOOKFUNC(sithFX_CreateRaftWakeFX);
    J3D_HOOKFUNC(sithFX_CreatePaddleWaterSplash);
    J3D_HOOKFUNC(sithFX_CreateRaftPaddleWaterFX);
    J3D_HOOKFUNC(sithFX_CreateRaftInflateWaterFX);
    J3D_HOOKFUNC(sithFX_CreateMineCarSparks);
    J3D_HOOKFUNC(sithFX_CreateChalkMark);
    J3D_HOOKFUNC(sithFX_CreateThingOnSurface);
    J3D_HOOKFUNC(sithFX_UpdatePolyline);
    J3D_HOOKFUNC(sithFX_ResetPolylineTexVertOffset);
    J3D_HOOKFUNC(sithFX_CreateLaserThing);
    J3D_HOOKFUNC(sithFX_CreateLightningThing);
    J3D_HOOKFUNC(sithFX_CreatePolylineThing);
}

void sithFX_ResetGlobals(void)
{
    int sithFX_g_lastChalkMarkNum_tmp = -1;
    STD_COPYMEM(&sithFX_g_lastChalkMarkNum, &sithFX_g_lastChalkMarkNum_tmp, sizeof(sithFX_g_lastChalkMarkNum));

    size_t sithFX_g_numChalkMarks_tmp = 0;
    STD_COPYMEM(&sithFX_g_numChalkMarks, &sithFX_g_numChalkMarks_tmp, sizeof(sithFX_g_numChalkMarks));

    STD_ZEROMEM(&sithFX_g_aChalkMarks, sizeof(sithFX_g_aChalkMarks));
}

void sithFX_Reset(void)
{
    sithFX_g_numChalkMarks    = 0;
    sithFX_g_lastChalkMarkNum = -1;
    sithFX_ClearChalkMarks();
}

void sithFX_ClearChalkMarks(void)
{
    for ( size_t i = 0; i < STD_ARRAYLEN(sithFX_g_aChalkMarks); ++i )
    {
        sithFX_g_aChalkMarks[i] = NULL;
    }
}

void J3DAPI sithFX_DestroyFairyDustDeluxe(SithThing* pThing)
{
    if ( pThing && pThing->userblock.pFairydust )
    {
        SithFairyDustUserBlock* pFairydust = pThing->userblock.pFairydust;
        for ( size_t i = 0; i < STD_ARRAYLEN(pFairydust->aDusts); ++i )
        {
            if ( pFairydust->aDusts[i] && pFairydust->aDusts[i]->type != SITH_THING_FREE && (pFairydust->aDusts[i]->flags & SITH_TF_DESTROYED) == 0 )
            {
                sithParticle_DestroyParticle(pFairydust->aDusts[i]);
            }

            pFairydust->aDusts[i] = NULL;
        }

        STDFREE(pThing->userblock.pFairydust);
        pThing->userblock.pFairydust = NULL;
    }
}

void J3DAPI sithFX_CreateFairyDust(const rdVector3* pos, SithSector* pSector)
{
    SithThing* pTemplate = sithTemplate_GetTemplate("+imp2_trail");
    if ( !pTemplate )
    {
        return;
    }

    SithThing* pDust = sithThing_CreateThingAtPos(pTemplate, pos, &pTemplate->orient, pSector, NULL); // Fixed: Fixed initing. pDust
    if ( pDust )
    {
        sithFX_fariyDustSizeFactor = sithFX_SetFairyDustSize(pDust->renderData.data.pParticle, sithFX_fariyDustSizeFactor);
    }
}

size_t J3DAPI sithFX_SetFairyDustSize(rdParticle* pParticle, size_t sizeFactor)
{
    if ( sizeFactor > 0 )
    {
        --sizeFactor;
    }
    else
    {
        sizeFactor = 2;
    }

    pParticle->size     = (float)sizeFactor * 0.0025f + 0.005f;
    pParticle->sizeHalf = pParticle->size / 2.0f;
    return sizeFactor;
}

void J3DAPI sithFX_UpdateFairyDustUserBlock(SithThing* pThing)
{
    if ( !pThing )
    {
        return;
    }

    if ( !pThing->pParent )
    {
        return;
    }

    SithFairyDustUserBlock* pFairydust = pThing->pParent->userblock.pFairydust;
    if ( !pFairydust )
    {
        return;
    }

    for ( size_t i = 0; i < STD_ARRAYLEN(pFairydust->aDusts); ++i )
    {
        if ( pFairydust->aDusts[i] && ((pFairydust->aDusts[i]->flags & SITH_TF_DESTROYED) != 0 || pFairydust->aDusts[i]->type == SITH_THING_FREE) )
        {
            pFairydust->aDusts[i] = NULL;
        }
    }

    if ( streq(pThing->thingInfo.particleInfo.pMaterial->aName, "aet_4sprite_glow_fairy_dust.mat") )
    {
        pThing->pos.x = pThing->pParent->pos.x;
        pThing->pos.y = pThing->pParent->pos.y;
    }
}

void J3DAPI sithFX_CreateFairyDustDelux(SithThing* pThing, const rdVector3* pPos)
{
    if ( !pThing )
    {
        return;
    }

    SithFairyDustUserBlock* pFairydust = pThing->userblock.pFairydust;
    if ( pFairydust ) // If exists, update
    {
        if ( pFairydust->bCreateNormalFairyDust )
        {
            if ( pPos )
            {
                sithFX_CreateFairyDust(pPos, pThing->pInSector);
            }
        }

        else if ( sithTime_g_msecGameTime - sithFX_msecLastCreatedFairyDustDeluxTime <= 900 )
        {
            sithFX_CreateFairyDustDeluxDusts(pThing, pFairydust);
        }
        else
        {
            pFairydust->bCreateNormalFairyDust = 1;
        }
    }
    else
    {
        pFairydust = (SithFairyDustUserBlock*)STDMALLOC(sizeof(SithFairyDustUserBlock));
        if ( pFairydust )
        {
            STD_ZEROMEM(pFairydust, sizeof(SithFairyDustUserBlock));
            if ( sithFX_CreateFairyDustDeluxDusts(pThing, pFairydust) )
            {
                pFairydust->bCreateNormalFairyDust = 0;
                pThing->userblock.pFairydust       = pFairydust;
                sithFX_msecLastCreatedFairyDustDeluxTime = sithTime_g_msecGameTime;
            }
            else
            {
                STDFREE(pFairydust);
            }
        }
    }
}

int J3DAPI sithFX_CreateFairyDustDeluxDusts(SithThing* pThing, SithFairyDustUserBlock* pFairydust)
{
    SithThing* pTemplate = sithTemplate_GetTemplate("+imp2_dust");
    if ( !pTemplate )
    {
        return 0;
    }

    rdVector3 pos = pThing->pos;
    pos.z = pos.z - sithPhysics_GetThingHeight(pThing);
    pos.z = (float)pFairydust->numUsedDusts * 0.045000002f + 0.022500001f + pos.z;

    for ( size_t i = 0; i < STD_ARRAYLEN(pFairydust->aDusts); ++i )
    {
        if ( pFairydust->aDusts[i] )
        {
            if ( pFairydust->aDusts[i]->type )
            {
                sithFX_fairyDustDeluxSizeFactor = sithFX_SetFairyDustSize(pFairydust->aDusts[i]->renderData.data.pParticle, sithFX_fairyDustDeluxSizeFactor);
            }
        }
    }

    if ( pFairydust->aDusts[pFairydust->numUsedDusts] && pFairydust->aDusts[pFairydust->numUsedDusts]->type != SITH_THING_FREE )
    {
        return 1;
    }

    SithThing* pDust = sithThing_CreateThingAtPos(pTemplate, &pos, &pTemplate->orient, pThing->pInSector, NULL);
    if ( !pDust )
    {
        return 0;
    }

    pDust->pParent = pThing;
    sithFX_newFairyDustDeluxSizeFactor = sithFX_SetFairyDustSize(pDust->renderData.data.pParticle, sithFX_newFairyDustDeluxSizeFactor);

    pFairydust->aDusts[pFairydust->numUsedDusts++] = pDust;
    if ( pFairydust->numUsedDusts == STD_ARRAYLEN(pFairydust->aDusts) )
    {
        pFairydust->numUsedDusts = 0;
    }

    return 1;
}

void J3DAPI sithFX_CreateBubble(SithThing* pThing)
{
    if ( pThing->type == SITH_THING_PLAYER && (!pThing->pInSector || (pThing->pInSector->flags & SITH_SECTOR_UNDERWATER) != 0) )
    {
        SithThing* pTemplate = sithTemplate_GetTemplate("+bubbles");
        if ( !pTemplate )
        {
            return;
        }

        SithThing* pBubble = sithThing_CreateThingAtPos(pTemplate, &pThing->pos, &pTemplate->orient, pThing->pInSector, NULL);
        if ( !pBubble )
        {
            SITHLOG_ERROR("Couldn't make a bubble thing!\n");
            return;
        }

        pBubble->flags |= SITH_TF_AIRDESTROYED;

        rdVector3 offset;
        rdVector_Scale3(&offset, &pThing->orient.lvec, 0.07f);

        rdVector_Add3Acc(&pBubble->pos, &offset);
        pBubble->pos.z = pBubble->pos.z + pThing->collide.movesize;

        float size = pThing->collide.movesize / 3.0f;
        rdVector_Scale3(&offset, &pThing->orient.rvec, size);

        if ( SITH_RANDF() > 0.5f )
        {
            rdVector_Sub3Acc(&pBubble->pos, &offset);
        }
        else
        {
            rdVector_Add3Acc(&pBubble->pos, &offset);
        }

        rdVector3 start;
        start.x = 0.001f;
        start.y = 0.001f;
        start.z = 1.0f;

        rdVector3 end;
        end.x = 0.02f;
        end.y = 0.02f;
        end.z = 0.0f;
        float time = (SITH_RANDF() - 0.5f) * 2.0f + 4.0f;
        sithAnimate_StartAnimateSpriteSize(pBubble, &start, &end, time);

        rdVector3 vel;
        vel.x = SITH_RANDF() - 0.5f;
        vel.y = SITH_RANDF() - 0.5f;
        vel.z = SITH_RANDF() + 0.55000001f;
        rdVector_Scale3Acc(&vel, 0.02f);

        pBubble->moveInfo.physics.velocity = vel;
    }
}

void J3DAPI sithFX_CreateWaterRipple(SithThing* pThing)
{
    if ( ((pThing->pInSector->flags & SITH_SECTOR_UNDERWATER) == 0 || (pThing->moveInfo.physics.flags & SITH_PF_ONWATERSURFACE) != 0)
        && (pThing->pInSector->flags & SITH_SECTOR_AETHERIUM) == 0 )
    {
        SithThing* pTemplate = sithTemplate_GetTemplate("+ripples");
        if ( !pTemplate )
        {
            return;
        }

        SithSector* pRippleSec = pThing->pInSector;
        rdVector3 rippleUVec = rdroid_g_zVector3;

        rdVector3 ripplePos, start, end;
        if ( (pThing->moveInfo.physics.flags & SITH_PF_ONWATERSURFACE) != 0
            || (pThing->moveInfo.physics.flags & SITH_PF_RAFT) != 0 )
        {
            float size = pThing->collide.movesize;
            start.x = size;
            start.y = size;
            start.z = 0.8f; // Altered: Changed ripple alpha to 0.8f from 1.0f

            size = pThing->collide.movesize * 4.0f;
            end.x = size;
            end.y = size;
            end.z = 0.0f;

            // Fixed: Added search for water surface position.
            //        This fixes potential wrong ripple position when e.g.: climbing out of water where ripple 
            //        could be created at position some distance above the water surface.
            ripplePos = pThing->pos;
            ripplePos.z += pThing->attach.distToWaterSurface + sithFX_surfaceOffsetZ; // Altered: Replaced 0.001 constant with sithFX_surfaceOffsetZ
            SithSurface* pWaterSurf = sithCollision_FindWaterSurface(pThing->pInSector, &pThing->pos, &ripplePos, pThing->collide.movesize);
            if ( pWaterSurf )
            {
                pRippleSec = pWaterSurf->pSector;
                rippleUVec = pWaterSurf->face.normal;
                ripplePos.z += sithFX_surfaceOffsetZ;
            }
        }
        else
        {
            float size = pThing->collide.movesize / 4.0f;
            start.x = size;
            start.y = size;
            start.z = 0.5f; // Altered: Changed ripple alpha to 0.5f from 1.0f

            size = pThing->collide.movesize * 2.0f;
            end.x = size;
            end.y = size;
            end.z = 0.0f;

            // Fixed: Added search for water surface position.
            //        Since OG did fixed position at thing.z - movesize*2 depending on 
            //        the water height the ripple could be crated under the water or above the water.
            //        The position is now placed directly on the water surface by searching for the exact position of water surface below thing.
            ripplePos = pThing->pos;
            ripplePos.z -= sithPhysics_GetThingHeight(pThing);
            SithSurface* pWaterSurf = sithCollision_FindWaterSurface(pThing->pInSector, &pThing->pos, &ripplePos, pThing->collide.movesize);
            if ( pWaterSurf )
            {
                pRippleSec = pWaterSurf->pSector;
                rippleUVec = pWaterSurf->face.normal;
                ripplePos.z += sithFX_surfaceOffsetZ;
            }
            else
            {
                // Fallback to OG code
                ripplePos.z = pThing->pos.z - pThing->collide.movesize * 2.0f;
            }
        }

        // Altered: Replaced OG code with sithFX_CreateThingFacingUp
        SithThing* pSprite = sithFX_CreateThingFacingUp(pTemplate, &ripplePos, pRippleSec, &rippleUVec); // Note: don't change zVector to thing uvec as thing might not look up
        if ( !pSprite )
        {
            SITHLOG_ERROR("Can't make a ripple, no thing space!\n");
            return;
        }

        float time = SITH_RANDF() + 1.0f;
        sithAnimate_StartAnimateSpriteSize(pSprite, &start, &end, time);
    }
}

void J3DAPI sithFX_CreateRaftSplatterFX(SithThing* pThing, int bCreateSplash)
{
    rdVector3 ripplePos = { 0 }; // Added: Init to 0

    SithThing* pSplashTpl = sithTemplate_GetTemplate("+ripples");
    if ( pSplashTpl && pThing->attach.flags )
    {
        float size = pThing->collide.movesize;
        rdVector3 start = { .x=size, .y=size, .z=0.25f }; // Altered: Changed alpha from 1.0f to 0.25f to make ripple better blend with water

        size = pThing->collide.movesize * 4.0f;
        rdVector3 end = { .x=size, .y=size, .z=0.0f };

        // Randomize ripple position a bit
        ripplePos.x = (SITH_RANDF() - 0.5f) * 0.03f;
        ripplePos.y = (SITH_RANDF() - 0.5f) * 0.03f;

        // Transform to water surface
        // Altered: Replaced OG code with sithFX_TransformPointToAttachSurface
        sithFX_TransformPointToAttachSurface(&ripplePos, pThing);

        // Fixed: Use surface normal instead of thing uvec.
        //        Resolves visual glitches where the raft could tilt opposite
        //        to the water surface, causing ripples to be oriented partly in the air.
        SithSector* pRippleSec = pThing->pInSector;
        rdVector3 rippleUpDir  = pThing->attach.pFace->normal;

        // Fixed: Search for the exact water surface at the ripple spawn position and
        //        assign the resolved position, water sector, and surface normal.
        //        This prevents visual issues where ripples could be misplaced or
        //        incorrectly oriented (e.g. embedded in the water surface or spawned
        //        in the air) due to steep, uneven, or overlapping water surfaces.
        sithFX_GetWaterPosition(pThing, &ripplePos, pRippleSec, &rippleUpDir);

        // Altered: Replaced OG code with sithFX_CreateThingFacingUp
        SithThing* pRipple = sithFX_CreateThingFacingUp(pSplashTpl, &ripplePos, pRippleSec, &rippleUpDir);
        if ( !pRipple )
        {
            SITHLOG_ERROR("Can't make a ripple, no thing space!\n");
            return;
        }

        // Added: Roll the ripple according to raft movement direction
        sithFX_RotateSpriteToMovement(pRipple, &pThing->moveInfo.physics.velocity);

        float time = SITH_RANDF() + 1.5f;
        sithAnimate_StartAnimateSpriteSize(pRipple, &start, &end, time);
    }

    if ( bCreateSplash )
    {
        pSplashTpl = sithTemplate_GetTemplate("+raft_splash");
        SithThing* pMistTpl = sithTemplate_GetTemplate("spritlemist");
        if ( !pSplashTpl || !pMistTpl )
        {
            // TODO: maybe log warning
            return;
        }

        // Sprite sizes
        rdVector3 start;
        start.x = 0.35f; // Altered: due to normalization fix in rdSprite_Draw size had to be decreased x10. OG 3.5f
        start.y = 0.15f; // Altered: due to normalization fix in rdSprite_Draw size had to be decreased x10. OG 1.5f
        start.z = sithFX_raftRippleAlpha; // Altered: Use sithFX_raftRippleAlpha instead of hardcoded value 1.0f

        rdVector3 end;
        end.x = 0.35f; // Altered: due to normalization fix in rdSprite_Draw size had to be decreased x10. OG 3.5f
        end.y = 0.15f; // Altered: due to normalization fix in rdSprite_Draw size had to be decreased x10. OG 1.5f
        end.z = 1.0f;

        // Create splash thing
        rdVector3 pos;
        pos.x = 0.12f; // right
        pos.y = 0.090000004f;
        pos.z = 0.0099999998f;
        rdMatrix_TransformVector34Acc(&pos, &pThing->orient);
        rdVector_Add3Acc(&pos, &ripplePos);

        rdMatrix34 orient = pThing->orient;

        rdVector3 pyr;
        pyr.x = 90.0f;
        pyr.y = 90.0f;
        pyr.z = 0.0f;
        rdMatrix_PreRotate34(&orient, &pyr);
        orient.uvec = pThing->attach.pFace->normal; // Rotate up

        // Create splash
        SithThing* pSplash = sithThing_CreateThingAtPos(pSplashTpl, &pos, &orient, pThing->pInSector, NULL);
        if ( pSplash )
        {
            sithAnimate_StartAnimateSpriteSize(pSplash, &start, &end, 0.5f);
        }

        // Create mist thing
        pos.z += 0.050000001f;
        sithThing_CreateThingAtPos(pMistTpl, &pos, &pMistTpl->orient, pThing->pInSector, NULL);

        // Create another splash thing rotated for 90 degrees
        pos.x = -0.12f; // left
        pos.y = 0.090000004f;
        pos.z = 0.0099999998f;
        rdMatrix_TransformVector34Acc(&pos, &pThing->orient);
        rdVector_Add3Acc(&pos, &ripplePos);

        orient = pThing->orient;

        pyr.x = 90.0f;
        pyr.y = -90.0f;
        pyr.z = 0.0f;
        rdMatrix_PreRotate34(&orient, &pyr);
        orient.uvec = pThing->attach.pFace->normal;

        pSplash = sithThing_CreateThingAtPos(pSplashTpl, &pos, &orient, pThing->pInSector, NULL);
        if ( pSplash )
        {
            sithAnimate_StartAnimateSpriteSize(pSplash, &start, &end, 0.5f);
        }

        // Create another mist thing
        pos.z += 0.050000001f;
        sithThing_CreateThingAtPos(pMistTpl, &pos, &pMistTpl->orient, pThing->pInSector, NULL);

        // Start mat animation
        rdMaterial* pMat = sithMaterial_Load("riv_a4sprite_rocksplash.mat");
        if ( pMat )
        {
            sithAnimate_StartMaterialAnim(pMat, 16.0f, (SithAnimateFlags)0);
        }
    }
}

void J3DAPI sithFX_CreateRaftWakeFX(SithThing* pThing)
{
    // Added: Added check for water surface 
    if ( (pThing->attach.flags & SITH_ATTACH_SURFACE) == 0
        || (pThing->attach.attachedToStructure.pSurfaceAttached->flags & (SITH_SURFACE_WATER | SITH_SURFACE_SHALLOWWATER)) == 0 )
    {
        return;
    }

#ifdef J3D_QOL_IMPROVEMENTS
    // Added: Use ripple effect instead of wake effect for better visuals

    SithThing* pRippleTpl = sithTemplate_GetTemplate("+ripples");
    if ( pRippleTpl && pThing->attach.flags )
    {
        // Randomize ripple position a bit
        rdVector3 ripplePos = { 0 };
        ripplePos.x = (SITH_RANDF() - 0.5f) * 0.03f;
        ripplePos.y = (SITH_RANDF() - 0.5f) * 0.03f;

        // Move position slightly forward based on movement direction
        rdVector3 moveNorm;
        rdVector_Normalize3(&moveNorm, &pThing->moveInfo.physics.velocity);
        rdVector_MultAcc3(&ripplePos, &moveNorm, 0.0423f);

        // Transform to water surface
        sithFX_TransformPointToAttachSurface(&ripplePos, pThing);

        SithSector* pRippleSec = pThing->pInSector;
        rdVector3 rippleUpDir  = pThing->attach.pFace->normal;

        // Fixed: Search for the exact water surface at the ripple spawn position and
        //        assign the resolved position, water sector, and surface normal.
        //        This prevents visual issues where ripples could be misplaced or
        //        incorrectly oriented (e.g. embedded in the water surface or spawned
        //        in the air) due to steep, uneven, or overlapping water surfaces.
        sithFX_GetWaterPosition(pThing, &ripplePos, pRippleSec, &rippleUpDir);

        SithThing* pRipple = sithFX_CreateThingFacingUp(pRippleTpl, &ripplePos, pRippleSec, &rippleUpDir);
        if ( !pRipple )
        {
            SITHLOG_ERROR("Can't make a ripple, no thing space!\n");
            return;
        }

        // Rotate to match movement direction
        sithFX_RotateSpriteToMovement(pRipple, &pThing->moveInfo.physics.velocity);


        // Try replace sprite with wake ripple sprite
        if ( !sithFX_bWakeRippleLoaded )
        {
            sithFX_pWakeRippleSprite = sithSprite_Load(sithWorld_g_pStaticWorld, "wake_ripple.spr");
            sithFX_bWakeRippleLoaded = true;
        }

        float size = pThing->collide.movesize;
        if ( sithFX_pWakeRippleSprite )
        {
            // Adjust size based on the raft orient and movement direction
            // i.e.: side raft movement should make larger wakes
            float moveDot = rdVector_Dot3(&pThing->orient.lvec, &moveNorm);
            size = J3DMIN(size * 2.0f / fabsf(moveDot), size * 2.9f);

            pRipple->renderData.data.pSprite3 = sithFX_pWakeRippleSprite;
        }

        rdVector3 start = { .x=size, .y=size, .z=sithFX_raftRippleAlpha };

        size *= 4.0f;
        rdVector3 end = { .x=size, .y=size, .z=0.0f };

        float time = SITH_RANDF() + 1.5f;
        sithAnimate_StartAnimateSpriteSize(pRipple, &start, &end, time);
    }
#else
    SithThing* pWakeTpl = sithTemplate_GetTemplate("+raft_wake");
    if ( pWakeTpl )
    {
        rdVector3 start;
        start.x = 0.175f;
        start.y = 0.1f;
        start.z = 0.25f;

        rdVector3 end;
        end.x = 0.27500001f;
        end.y = 0.40000001f;
        end.z = 0.0f;

        // Transform to water surface
        // Altered: Replaced OG code with sithFX_TransformPointToAttachSurface
        rdVector3 pos = { 0 };
        sithFX_TransformPointToAttachSurface(&pos, pThing);

        SithThing* pWake = sithFX_CreateThingFacingUp(pWakeTpl, &pos, pThing->pInSector, &pThing->orient.uvec);
        if ( !pWake )
        {
            SITHLOG_ERROR("Can't make a wake, no thing space!\n");
            return;
        }

        // Added: Set light mode to gouraud so it looks better in dark areas, and added extra light
       /* pWake->renderData.data.pSprite3->face.lightingMode = RD_LIGHTING_GOURAUD;
        pWake->renderData.data.pSprite3->face.extraLight   = sithFX_rippleExtraLight;*/

        // Rotate to match movement direction and flip 180 degrees
        sithFX_RotateSpriteToMovement(pWake, &pThing->moveInfo.physics.velocity);
        pWake->thingInfo.spriteInfo.rollAngle += 180.0f;

        sithAnimate_StartAnimateSpriteSize(pWake, &start, &end, 0.5f);
    }
#endif
}

void J3DAPI sithFX_CreatePaddleWaterSplash(SithThing* pThing, const rdVector3* pos)
{
    SithThing* pTemplate = sithTemplate_GetTemplate("paddlesplash");
    if ( pTemplate )
    {
        SithThing* pSplash = sithThing_CreateThingAtPos(pTemplate, pos, &pTemplate->orient, pThing->pInSector, NULL);
        if ( !pSplash )
        {
            SITHLOG_ERROR("Can't make a paddle splash, no thing space!\n");
            return;
        }
        pSplash->moveInfo.physics.velocity.z = 0.07f;
    }
}

void J3DAPI sithFX_CreateRaftPaddleWaterFX(SithThing* pThing, float secTime)
{
    float secDeltaTime = sithTime_g_secGameTime - secTime;
    float secDelatLastCreated = sithTime_g_secGameTime - sithFX_secLastCreatedPaddleRipple;

    rdVector3 ripplePos = { 0 };
    switch ( pThing->moveStatus )
    {
        case SITHPLAYERMOVE_RAFT_PADDLE_FORWARD_LEFT:
            ripplePos.x = -0.075000003f - 0.0f * secDeltaTime;
            ripplePos.y = -0.31999999f * secDeltaTime + 0.090000004f;
            ripplePos.z = -sithPhysics_GetThingHeight(pThing);
            break;

        case SITHPLAYERMOVE_RAFT_PADDLE_FORWARD_RIGHT:
            ripplePos.x = 0.0f * secDeltaTime + 0.075000003f;
            ripplePos.y = -0.31999999f * secDeltaTime + 0.090000004f;
            ripplePos.z = -sithPhysics_GetThingHeight(pThing);
            break;

        case SITHPLAYERMOVE_RAFT_TURN_LEFT:
        case SITHPLAYERMOVE_RAFT_PADDLE_BACK_LEFT:
            ripplePos.x = -0.07f - 0.0f * secDeltaTime;
            ripplePos.y = 0.12f * secDeltaTime + -0.039999999f;
            ripplePos.z = -sithPhysics_GetThingHeight(pThing);
            break;

        case SITHPLAYERMOVE_RAFT_TURN_RIGHT:
        case SITHPLAYERMOVE_RAFT_PADDLE_BACK_RIGHT:
            ripplePos.x = 0.0f * secDeltaTime + 0.07f;
            ripplePos.y = 0.12f * secDeltaTime + -0.039999999f;
            ripplePos.z = -sithPhysics_GetThingHeight(pThing);
            break;

        default:
            return;
    }

    rdMatrix_TransformPoint34Acc(&ripplePos, &pThing->orient);
    sithFX_TransformPointToAttachSurface(&ripplePos, pThing);

    // Fixed: Clamp ripple Z position to the water surface.
    //        Prevents ripples from appearing in the air when the raft
    //        is partially lifted by collisions with solid geometry.
    //
    // Note, used as fallback if bellow water surface search function fails
    if ( (pThing->attach.flags & SITH_ATTACH_SURFACE) != 0 )
    {
        ripplePos.z = pThing->attach.attachedFaceFirstVert.z;
    }

    ripplePos.z += sithFX_surfaceOffsetZ; // Altered: Replaced constant 0.001 with sithFX_surfaceOffsetZ


    SithSector* pRippleSec = pThing->pInSector;
    rdVector3 rippleUpDir  = pThing->orient.uvec;

    // Fixed: Use surface normal instead of thing uvec.
    //        Resolves visual glitches where the raft could tilt opposite
    //        to the water surface, causing ripples to be oriented partly in the air.
    //
    // Note, used as fallback if bellow water surface search function fails
    if ( pThing->attach.flags )
    {
        rippleUpDir = pThing->attach.pFace->normal;
    }

    // Fixed: Search for the exact water surface at the ripple spawn position and
    //        assign the resolved position, water sector, and surface normal.
    //        This prevents visual issues where ripples could be misplaced or
    //        incorrectly oriented (e.g. embedded in the water surface or spawned
    //        in the air) due to steep, uneven, or overlapping water surfaces.
    sithFX_GetWaterPosition(pThing, &ripplePos, pRippleSec, &rippleUpDir);

    bool bCreateSplashFx = false;
    if ( secDeltaTime == 0.0f || secDelatLastCreated >= 0.2f )
    {
        bCreateSplashFx = true;
    }
    else
    {
        if ( rdVector_Dist3(&ripplePos, &sithFX_curPaddleRipplePos) > 0.0049999999f )
        {
            bCreateSplashFx = true;
        }
    }

    if ( !bCreateSplashFx )
    {
        goto skip;
    }

#ifndef J3D_QOL_IMPROVEMENTS
    if ( (SithPhysicsWaterSurfaceType)pThing->userval != SITHPHYSICS_WATERSURFACE_ADJOIN ) // If not on underwater adjoin surface, (set by sithPhysics_CheckWaterSurfaceAtPos)
    {
        goto skip;
    }
#endif // J3D_QOL_IMPROVEMENTS

    SithThing* pRippleTbl = sithTemplate_GetTemplate("+ripples");
    if ( !pRippleTbl )
    {
        goto skip;
    }

    // Altered: Replaced OG code with sithFX_CreateThingFacingUp
    SithThing* pRipple = sithFX_CreateThingFacingUp(pRippleTbl, &ripplePos, pRippleSec, &rippleUpDir);
    if ( !pRipple )
    {
        SITHLOG_ERROR("Can't make a ripple, no thing space!\n", 0);
        return;
    }

    //
    // Start sprite animation
    //

    rdVector3 start;
    start.x = 0.0049999999f;
    start.y = 0.0049999999f;
    start.z = 0.8f; // Altered: Changed to 0.8 from 1.0

    rdVector3 end;
    end.x = 0.029999999f;
    end.y = 0.029999999f;
    end.z = 0.0f;

    float time = SITH_RANDF() + 1.0f;
    sithAnimate_StartAnimateSpriteSize(pRipple, &start, &end, time);

    sithFX_curPaddleRipplePos         = ripplePos;
    sithFX_secLastCreatedPaddleRipple = sithTime_g_secGameTime;

skip:
    if ( bCreateSplashFx )
    {
        ripplePos.z += 0.0080000004f;
        sithFX_CreatePaddleWaterSplash(pThing, &ripplePos);
    }
}

void J3DAPI sithFX_CreateRaftInflateWaterFX(SithThing* pThing, float size)
{
    SithThing* pRippleTpl = sithTemplate_GetTemplate("+ripples");
    if ( pRippleTpl && pThing->attach.flags )
    {

        rdVector3 start;
        start.x = size;
        start.y = size;
        start.z = sithFX_raftRippleAlpha; // Altered: Use sithFX_raftRippleAlpha instead of hardcoded value 1.0f

        float endSize = size * 2.0f;
        rdVector3 end;
        end.x = endSize;
        end.y = endSize;
        end.z = 0.0f;

        // Randomize ripple position a bit
        rdVector3 pos = { 0 };
        pos.x = (SITH_RANDF() - 0.5f) * (size * 0.5f);
        pos.y = (SITH_RANDF() - 0.5f) * (size * 0.5f);

        // Transform to water surface
        // Altered: Replaced OG code with sithFX_TransformPointToAttachSurface
        sithFX_TransformPointToAttachSurface(&pos, pThing);

        // Altered: Replaced OG code with sithFX_CreateThingFacingUp
        SithThing* pRipple = sithFX_CreateThingFacingUp(pRippleTpl, &pos, pThing->pInSector, &rdroid_g_zVector3); // Note: don't replace zVector3 with thing uvec as thing is not raft and might not be oriented up
        if ( !pRipple )
        {
            SITHLOG_ERROR("Can't make a ripple, no thing space!\n");
            return;
        }

        sithAnimate_StartAnimateSpriteSize(pRipple, &start, &end, 2.5f);
    }
}

void J3DAPI sithFx_CreateTireFX(SithThing* pThing, const rdVector3* pPosRight, rdMaterial* pMatRight, const rdVector3* pPosLeft, rdMaterial* pMatLeft)
{
    // Normalize velocity and get speed
    rdVector3 dir;
    float speed = rdVector_Normalize3(&dir, &pThing->moveInfo.physics.velocity);

    // Calculate tire skid probability based on lateral velocity and angular velocity
    float dot = rdVector_Dot3(&pThing->orient.rvec, &dir);
    float lateralSpeed = speed * dot * 1.0f;
    float yawAngle = pThing->moveInfo.physics.angularVelocity.y * 0.0099999998f + lateralSpeed;

    float skidProbability = 0.001f;
    skidProbability += fabsf(dot) * 0.1f;
    skidProbability += speed * 0.0049999999f;

    // Check if right tire should emit particles
    bool bRight = false;
    float rightTireDist = rdVector_Dist3(pPosRight, &sithFx_rightTireStartPosOffset);
    if ( rightTireDist >= 0.0049999999f && SITH_RANDF() < skidProbability )
    {
        bRight = true;
    }

    // Check if left tire should emit particles
    bool bLeft = false;
    float leftTireDist = rdVector_Dist3(pPosLeft, &sithFx_leftTireStartPosOffset);
    if ( leftTireDist >= 0.0049999999f && SITH_RANDF() < skidProbability )
    {
        bLeft = true;
    }

    SithThing* pTemplate = sithTemplate_GetTemplate("paddlesplash");
    if ( !pTemplate || (!bLeft && !bRight) )
    {
        return;
    }

    // Create right tire particle effect
    SithThing* pRightParticles = NULL;
    if ( pMatRight )
    {
        rdVector3 pos = *pPosRight;
        rdVector_MultAcc3(&pos, &pThing->orient.lvec, -0.025f);
        rdVector_MultAcc3(&pos, &pThing->orient.rvec, 0.0070000002f);
        rdVector_MultAcc3(&pos, &pThing->orient.uvec, 0.035f);

        pRightParticles = sithThing_CreateThingAtPos(pTemplate, &pos, &pTemplate->orient, pThing->pInSector, NULL);
        if ( !pRightParticles )
        {
            SITHLOG_ERROR("Can't make a tirefx, no thing space!\n");
            return;
        }

        pRightParticles->renderData.data.pParticle->pMaterial = pMatRight;
    }

    // Create left tire particle effect
    SithThing* pLeftParticles = NULL;
    if ( pMatLeft )
    {
        rdVector3 pos = *pPosLeft;
        rdVector_MultAcc3(&pos, &pThing->orient.lvec, -0.025f);
        rdVector_MultAcc3(&pos, &pThing->orient.rvec, -0.0070000002f);
        rdVector_MultAcc3(&pos, &pThing->orient.uvec, 0.035f);

        pLeftParticles = sithThing_CreateThingAtPos(pTemplate, &pos, &pTemplate->orient, pThing->pInSector, NULL);
        if ( !pLeftParticles )
        {
            SITHLOG_ERROR("Can't make a tirefx, no thing space!\n");
            return;
        }

        pLeftParticles->renderData.data.pParticle->pMaterial = pMatLeft;
    }

    // Set velocity for right tire particles
    if ( pRightParticles )
    {
        rdVector_Scale3(&pRightParticles->moveInfo.physics.velocity, &pThing->orient.rvec, yawAngle);
        pRightParticles->moveInfo.physics.velocity.z += speed * 0.40000001f;
        pRightParticles->msecLifeLeft = 200;
    }

    // Set velocity for left tire particles
    if ( pLeftParticles )
    {
        rdVector_Scale3(&pLeftParticles->moveInfo.physics.velocity, &pThing->orient.rvec, yawAngle);
        pLeftParticles->moveInfo.physics.velocity.z += speed * 0.40000001f;
        pLeftParticles->msecLifeLeft = 200;
    }
}

SithThing* J3DAPI sithFX_CreateMineCarSparks(SithThing* pThing, const rdVector3* pSparkPos)
{
    SithThing* pSparks = sithTemplate_GetTemplate("minecarsparks");
    if ( !pSparks )
    {
        return pSparks;
    }

    rdVector3 pos;
    rdMatrix_TransformPoint34(&pos, pSparkPos, &pThing->orient);
    rdVector_Add3Acc(&pos, &pThing->pos);

    pSparks = sithThing_CreateThingAtPos(pSparks, &pos, &pSparks->orient, pThing->pInSector, NULL);
    if ( !pSparks )
    {
        SITHLOG_ERROR("Can't make a spark, no thing space!\n");
        return NULL;
    }

    pSparks->moveInfo.physics.velocity.z = 0.5f;
    return pSparks;
}

void J3DAPI sithFX_CreateChalkMark(const rdVector3* pPos, const SithSurface* pAttSurf, SithSector* pSector)
{
    if ( sithFX_g_lastChalkMarkNum + 1 >= STD_ARRAYLEN(sithFX_g_aChalkMarks) )
    {
        sithFX_g_lastChalkMarkNum = -1;
    }

    size_t markNum = sithFX_g_lastChalkMarkNum + 1;
    size_t tplNum  = 0;
    bool bSearch = false; // Added init to false
    do
    {
        tplNum = (size_t)(SITH_RAND() * (float)STD_ARRAYLEN(sithFX_aChalkMarkNames));
        bSearch = false;
        for ( size_t i = 0; i < sithFX_g_numChalkMarks && !bSearch && sithFX_g_aChalkMarks[i]; i++ )
        {
            if ( streq(sithFX_g_aChalkMarks[i]->aName, sithFX_aChalkMarkNames[tplNum]) )
            {
                bSearch = true;
            }
        }
    } while ( bSearch );

    SithThing* pTemplate = sithTemplate_GetTemplate(sithFX_aChalkMarkNames[tplNum]);
    if ( pTemplate )
    {
        if ( sithFX_g_aChalkMarks[markNum] )
        {
            sithThing_RemoveThing(sithWorld_g_pCurrentWorld, sithFX_g_aChalkMarks[markNum]);
            sithFX_g_aChalkMarks[markNum] = NULL;
        }

        sithFX_g_aChalkMarks[markNum] = sithFX_CreateThingOnSurface(pTemplate, pPos, pAttSurf, pSector);
        if ( sithFX_g_aChalkMarks[markNum] )
        {
            ++sithFX_g_lastChalkMarkNum;
            if ( ++sithFX_g_numChalkMarks > STD_ARRAYLEN(sithFX_g_aChalkMarks) )
            {
                sithFX_g_numChalkMarks = STD_ARRAYLEN(sithFX_g_aChalkMarks);
            }
        }
    }
}

SithThing* J3DAPI sithFX_CreateThingOnSurface(const SithThing* pTemplate, const rdVector3* pPos, const SithSurface* pAttSurf, SithSector* pSector)
{
    SITH_ASSERTREL(pTemplate && pAttSurf && pSector);

    rdVector3 offset;
    rdVector_Scale3(&offset, &pAttSurf->face.normal, 0.001f);

    rdVector3 pos;
    rdVector_Add3(&pos, pPos, &offset);

    SithThing* pThing = sithThing_CreateThingAtPos(pTemplate, &pos, &pTemplate->orient, pSector, NULL);
    if ( !pThing )
    {
        return NULL;
    }

    //rdVector_Copy3(&pThing->pos, &pos); // ?? should be already set by sithThing_CreateThingAtPos
    pThing->orient.lvec = pAttSurf->face.normal;

    // Calculate rvec = lvec X zVector3
    rdVector_Cross3(&pThing->orient.rvec, &pThing->orient.lvec, &rdroid_g_zVector3);
    rdVector_Normalize3Acc(&pThing->orient.rvec);

    // Calculate uvec = rvec X lvec
    rdVector_Cross3(&pThing->orient.uvec, &pThing->orient.rvec, &pThing->orient.lvec);

    return pThing;
}

void J3DAPI sithFX_UpdatePolyline(SithThing* pThing)
{
    if ( pThing->renderData.type == RD_THING_POLYLINE && !streq(pThing->aName, "+laserbeam") ) // TODO: maybe should also check for polyline mat gen_a4sfx_rbbeam_b.mat, as robot creates laser via CreatPolylineThing and not via CreatLaser function
    {
        if ( streq(pThing->aName, "+lightning") )
        {
            rdPolyline* pPolyline = pThing->renderData.data.pPolyline;
            pPolyline->face.texVertOffset.y = (SITH_RANDF() - 0.69999999f) * 0.079999998f + pPolyline->face.texVertOffset.y;
            size_t numCels = pThing->renderData.data.pPolyline->face.pMaterial->numCels;
            if ( numCels > 1 && SITH_ISFRAMECYCLE(pThing->idx, 4) ) // On every 4th frame random select new cel
            {
                pThing->renderData.data.pPolyline->face.matCelNum = (int32_t)(SITH_RAND() * (double)numCels);
            }
        }
        else if ( streq(pThing->aName, "+plcogend") )
        {
            // Fixed: Added parent null check.
            //        There might be issue in savegame system. Spoted this issue 
            //        while loading nub game at robot scene, and indy was hit by robo laser.
            if ( !pThing->pParent ) // unlikely
            {
                SITHLOG_ERROR("sithFX_UpdatePolyline: Strange the parent thing is gone, destroying polyline ...\n");
                sithThing_DestroyThing(pThing);
                return;
            }

            rdVector3 look;
            rdVector_Sub3(&look, &pThing->pParent->pos, &pThing->pos);
            float lineLen = rdVector_Normalize3Acc(&look);

            rdMatrix_BuildFromLook34(&pThing->orient, &look);
            pThing->renderData.data.pPolyline->length = lineLen;
        }
        else if ( streq(pThing->aName, "+plcog") )
        {
            rdVector3 look;
            rdVector_Sub3(&look, &pThing->forceMoveStartPos, &pThing->pos);
            float lineLen = rdVector_Normalize3Acc(&look);

            rdMatrix_BuildFromLook34(&pThing->orient, &look);
            pThing->renderData.data.pPolyline->length = lineLen;
        }
    }
}

void J3DAPI sithFX_ResetPolylineTexVertOffset(SithThing* pThing)
{
    if ( pThing->renderData.type == RD_THING_POLYLINE )
    {
        pThing->renderData.data.pPolyline->face.texVertOffset.x = 0.0f;
        pThing->renderData.data.pPolyline->face.texVertOffset.y = 0.0f;
    }
}

SithThing* J3DAPI sithFX_CreateLaserThing(const SithThing* pSourceThing, const rdVector3* vecOffset, const rdVector3* vecEnd, float baseRadius, float tipRadius, float duration)
{
    SithThing* pThing = sithThing_Create(SITH_THING_POLYLINE);
    if ( !pThing )
    {
        SITHLOG_ERROR("Could not create sithThing for Laser\n");
        return NULL;
    }

    SithThing* pTemplate = sithTemplate_GetTemplate("+laserbeam");
    if ( !pTemplate )
    {
        // Maybe add log
        return NULL;
    }

    sithThing_SetThingBasedOn(pThing, pTemplate);
    pThing->userval = 0.0f;
    pThing->msecLifeLeft = (int32_t)(duration * 1000.0f);

    rdVector3 startPos;
    rdVector_Add3(&startPos, &pSourceThing->pos, vecOffset);

    rdVector3 look;
    rdVector_Sub3(&look, vecEnd, &startPos);
    float length = rdVector_Normalize3Acc(&look);
    rdMatrix_BuildFromLook34(&pThing->orient, &look);

    rdVector4 color = { .red=1.0f, .green=0.40000001f, .blue=0.2f, .alpha=0.89999998f };
    rdPolyline* pPolyline = rdPolyline_New("+laserbeam", "gen_a4sfx_rbbeam_b.mat", "gen_a4sfx_rbbeam_b.mat", length, baseRadius, tipRadius, RD_GEOMETRY_FULL, RD_LIGHTING_GOURAUD, &color);
    if ( !pPolyline )
    {
        sithThing_RemoveThing(sithWorld_g_pCurrentWorld, pThing);
        SITHLOG_ERROR("Could not create polyline for laser.\n");
        return NULL;
    }

    rdThing_NewEntry(&pThing->renderData, NULL);
    rdThing_SetPolyline(&pThing->renderData, pPolyline);

    // Maybe should call sithFX_ResetPolylineTexVertOffset
    pThing->renderData.data.pPolyline->face.texVertOffset.x = 0.0f;
    pThing->renderData.data.pPolyline->face.texVertOffset.y = 0.0f;

    pThing->orient.dvec = rdroid_g_zeroVector3; // Reset position vector
    pThing->pos         = startPos;
    sithThing_EnterSector(pThing, pSourceThing->pInSector, 1, 1);
    return pThing;
}

SithThing* J3DAPI sithFX_CreateLightningThing(const SithThing* pSourceThing, const rdVector3* offset, const rdVector3* endPos, float baseRadius, float tipRadius, float duration)
{
    SithThing* pThing = sithThing_Create(SITH_THING_POLYLINE);
    if ( !pThing )
    {
        SITHLOG_ERROR("Could not create sithThing for Lightning\n");
        return NULL;
    }

    SithThing* pTemplate = sithTemplate_GetTemplate("+lightning");
    if ( !pTemplate )
    {
        return NULL;
    }

    sithThing_SetThingBasedOn(pThing, pTemplate);

    pThing->flags |= SITH_TF_EMITLIGHT;
    pThing->light.color.red   = 0.0f;
    pThing->light.color.green = 0.0f;
    pThing->light.color.blue  = 0.89999998f;
    pThing->light.color.alpha = 1.0f;

    pThing->msecLifeLeft = (int32_t)(duration * 1000.0f);

    rdVector3 startPos;
    rdVector_Add3(&startPos, &pSourceThing->pos, offset);

    rdVector3 look;
    rdVector_Sub3(&look, endPos, &startPos);
    float length = rdVector_Normalize3Acc(&look);

    pThing->light.minRadius = length * 2.0f;
    pThing->light.maxRadius = pThing->light.minRadius;

    rdMatrix_BuildFromLook34(&pThing->orient, &look);

    rdVector4 color = { .red= 1.0f, .green=0.40000001f, .blue=0.2f, .alpha=0.89999998f };

    rdPolyline* pPolyline = rdPolyline_New("+lightning", "gen_a4sfx_lightning.mat", "gen_a4sfx_lightning.mat", length, baseRadius, tipRadius, RD_GEOMETRY_FULL, RD_LIGHTING_GOURAUD, &color);
    if ( !pPolyline )
    {
        sithThing_RemoveThing(sithWorld_g_pCurrentWorld, pThing);
        SITHLOG_ERROR("Could not create polyline for lightning.\n");
        return NULL;
    }

    rdThing_NewEntry(&pThing->renderData, NULL);
    rdThing_SetPolyline(&pThing->renderData, pPolyline);

    rdVector_Zero3(&pThing->orient.dvec); // Remove position from orientation
    pThing->pos = startPos;

    sithThing_EnterSector(pThing, pSourceThing->pInSector, 1, 1);
    return pThing;
}

SithThing* J3DAPI sithFX_CreatePolylineThing(const SithThing* pSourceThing, SithThing* pEndThing, const rdVector3* pEndPos, const rdMaterial* pMaterial, float baseRadius, float tipRadius, float duration)
{
    SithThing* pThing = sithThing_Create(SITH_THING_POLYLINE);
    if ( !pThing )
    {
        SITHLOG_ERROR("Could not create sithThing for CreatePolylineThing\n");
        return NULL;
    }

    if ( !pMaterial )
    {
        SITHLOG_ERROR("NULL Material pointer passed to CreatePolylineThing\n");
        sithThing_RemoveThing(sithWorld_g_pCurrentWorld, pThing);
        return NULL;
    }

    rdVector3 endPos;
    if ( pEndThing )
    {
        SithThing* pTemplate = sithTemplate_GetTemplate("+plcogend");
        if ( !pTemplate )
        {
            return NULL;
        }

        sithThing_SetThingBasedOn(pThing, pTemplate);
        endPos = pEndThing->pos;
        pThing->pParent = pEndThing;
    }
    else
    {
        SithThing* pTemplate = sithTemplate_GetTemplate("+plcog");
        if ( !pTemplate )
        {
            return NULL;
        }

        sithThing_SetThingBasedOn(pThing, pTemplate);
        endPos = *pEndPos;
        pThing->forceMoveStartPos = endPos;
    }

    if ( duration > 0.0f )
    {
        pThing->msecLifeLeft = (int32_t)(duration * 1000.0f);
    }

    rdVector3 startPos = pSourceThing->pos;

    rdVector3 look;
    rdVector_Sub3(&look, &endPos, &startPos);
    float length = rdVector_Normalize3Acc(&look);
    rdMatrix_BuildFromLook34(&pThing->orient, &look);

    rdVector4 color = { .red=0.1f, .green=0.1f, .blue=0.1f, .alpha=1.0f };

    // Added: Added hardcoded check for rope, metal wire (vol elevator) mats to tile polyline UVs instead of stretching UVs.
    //        Corrects visual rendering of polyline
    rdPolylineFlags flags = { 0 };
    if ( streqi(pMaterial->aName, "fhead_rope_sde.mat")
        || streqi(pMaterial->aName, "riv_floor_metal.mat")
        || streqi(pMaterial->aName, "obj_vol_wire_tram.mat") )
    {
        flags = RDPOLYLINE_UVTILE;
        if ( streqi(pMaterial->aName, "obj_vol_wire_tram.mat") ) // Needs to be rotated as texture is oriented up instead of right
        {
            flags |= RDPOLYLINE_UVROTATE;
        }
    }

    // Altered Replaced rdPolyline_New with rdPolyline_NewEx to add flags
    rdPolyline* pPolyline = rdPolyline_NewEx(pThing->aName, pMaterial->aName, length, baseRadius, tipRadius, RD_GEOMETRY_FULL, RD_LIGHTING_GOURAUD, &color, flags);
    if ( !pPolyline )
    {
        sithThing_RemoveThing(sithWorld_g_pCurrentWorld, pThing);
        SITHLOG_ERROR("Could not create polyline for CreatePolylineThing.\n");
        return NULL;
    }

    rdThing_NewEntry(&pThing->renderData, NULL);
    rdThing_SetPolyline(&pThing->renderData, pPolyline);

    rdVector_Zero3(&pThing->orient.dvec); // Remove position from orientation
    pThing->pos = startPos;

    sithThing_EnterSector(pThing, pSourceThing->pInSector, 1, 1);

    pThing->alpha = 1.0f;
    return pThing;
}

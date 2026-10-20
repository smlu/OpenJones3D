#include "sithExplosion.h"
#include <j3dcore/j3dhook.h>

#include <rdroid/Math/rdMatrix.h>
#include <rdroid/Math/rdVector.h>

#include <sith/AI/sithAIAwareness.h>
#include <sith/Engine/sithAnimate.h>
#include <sith/Engine/sithCollision.h>
#include <sith/Engine/sithPhysics.h>
#include <sith/Gameplay/sithTime.h>
#include <sith/Main/sithMain.h>

#include <sith/World/sithSurface.h>
#include <sith/World/sithTemplate.h>
#include <sith/World/sithThing.h>

#include <std/General/stdConffile.h>
#include <std/General/stdMath.h>
#include <std/General/stdUtil.h>

static bool sithExplosion_bExpandFadeSet = false;

void sithExplosion_InstallHooks(void)
{
    J3D_HOOKFUNC(sithExplosion_Initialize);
    J3D_HOOKFUNC(sithExplosion_CreateSpriteThing);
    J3D_HOOKFUNC(sithExplosion_UpdateExpandFade);
    J3D_HOOKFUNC(sithExplosion_Lerp);
    J3D_HOOKFUNC(sithExplosion_Update);
    J3D_HOOKFUNC(sithExplosion_MakeBlast);
    J3D_HOOKFUNC(sithExplosion_ParseArg);
}

void sithExplosion_ResetGlobals(void)
{}

void J3DAPI sithExplosion_Initialize(SithThing* pThing)
{
    SITH_ASSERTREL(pThing);
    SITH_ASSERTREL(pThing->type == SITH_THING_EXPLOSION);

    pThing->thingInfo.explosionInfo.msecLifeLeft    = pThing->msecLifeLeft;
    pThing->thingInfo.explosionInfo.msecBabyTime   += sithTime_g_msecGameTime;
    pThing->thingInfo.explosionInfo.msecExpandTime += sithTime_g_msecGameTime;
    pThing->thingInfo.explosionInfo.msecFadeTime   += sithTime_g_msecGameTime;
    pThing->thingInfo.explosionInfo.pHitSurfaceMat  = NULL;
}


void J3DAPI sithExplosion_CreateSpriteThing(SithThing* pThing)
{
    SithExplosionInfo* pExplode = &pThing->thingInfo.explosionInfo;
    if ( strlen(pExplode->aSpriteTemplateName) )
    {
        const SithThing* pTemplate = sithTemplate_GetTemplate(pExplode->aSpriteTemplateName);
        if ( pTemplate )
        {
            SithThing* pThingSprite = sithThing_CreateThingAtPos(pTemplate, &pThing->pos, &pThing->orient, pThing->pInSector, NULL);
            if ( !pThingSprite )
            {
                SITHLOG_ERROR("could not create thingsprite for explosion");
                pExplode->pSprite = NULL;
            }
            else
            {
                pExplode->pSprite = pThingSprite;
                if ( (pExplode->flags & SITH_EF_EXPAND) != 0 || (pExplode->flags & SITH_EF_FADE) != 0 )
                {
                    sithExplosion_UpdateExpandFade(pExplode);
                }
                else
                {
                    float time = (float)pThingSprite->msecLifeLeft / 1000.0f;
                    sithAnimate_StartAnimateSpriteSize(pExplode->pSprite, &pExplode->spriteStart, &pExplode->spriteEnd, time);
                }
            }
        }
    }

    if ( (pExplode->flags & SITH_EF_ANIMATESPRITE) != 0 )
    {
        if ( pThing->renderData.type == RD_THING_SPRITE3 )
        {
            size_t numCels;
            rdMaterial* pMaterial = pThing->renderData.data.pSprite3->face.pMaterial;
            if ( pMaterial && (numCels = pMaterial->numCels, numCels > 1) )
            {
                float fps = (float)numCels / (float)pThing->msecLifeLeft * 1000.0f;
                sithAnimate_StartSpriteAnim(pThing, fps, SITHANIMATE_PAGEFLIP);
            }
            else
            {
                pExplode->flags &= ~SITH_EF_ANIMATESPRITE;
            }
        }

        else if ( pExplode->pSprite )
        {
            size_t numCels;
            rdMaterial* pMaterial = pExplode->pSprite->renderData.data.pSprite3->face.pMaterial;
            if ( pMaterial && (numCels = pMaterial->numCels, numCels > 1) && (float)pExplode->pSprite->msecLifeLeft != 0.0f )
            {
                float fps = (float)numCels / (float)pExplode->pSprite->msecLifeLeft * 1000.0f;
                sithAnimate_StartSpriteAnim(pExplode->pSprite, fps, SITHANIMATE_PAGEFLIP);
            }
            else
            {
                pExplode->flags &= ~SITH_EF_ANIMATESPRITE;
            }
        }
    }

    if ( pExplode && pExplode->pSprite && pExplode->pSprite->renderData.data.pSprite3 )
    {
        if ( (pExplode->flags & SITH_EF_RANDOMROLL) != 0 )
        {
            pExplode->pSprite->thingInfo.spriteInfo.rollAngle = SITH_RANDF() * 360.0f;
        }
        else
        {
            pExplode->pSprite->thingInfo.spriteInfo.rollAngle = 0.0f;
        }
    }
}

void J3DAPI sithExplosion_UpdateExpandFade(SithExplosionInfo* pExplode)
{
    if ( pExplode && pExplode->pSprite )
    {
        rdVector3 start = pExplode->spriteStart;
        rdVector3 end   = pExplode->spriteEnd;

        float msecLifeLeft = (float)pExplode->pSprite->msecLifeLeft;
        float secAnimTime  = msecLifeLeft / 1000.0f;

        if ( (double)sithTime_g_msecGameTime + msecLifeLeft == (double)pExplode->msecFadeTime )
        {
            pExplode->flags &= ~SITH_EF_FADE;
        }

        if ( (double)sithTime_g_msecGameTime + msecLifeLeft == (double)pExplode->msecExpandTime )
        {
            pExplode->flags &= ~SITH_EF_EXPAND;// ~0x800 - SITH_EF_EXPAND
        }

        if ( sithExplosion_bExpandFadeSet )
        {
            if ( pExplode->msecExpandTime == pExplode->msecFadeTime )
            {
                if ( sithTime_g_msecGameTime >= pExplode->msecExpandTime && (pExplode->flags & SITH_EF_EXPAND) != 0 && (pExplode->flags & SITH_EF_FADE) != 0 )
                {
                    sithAnimate_StartAnimateSpriteSize(pExplode->pSprite, &start, &end, secAnimTime);
                    pExplode->flags &= ~(SITH_EF_EXPAND | SITH_EF_FADE);
                }
            }
            else if ( sithTime_g_msecGameTime >= pExplode->msecExpandTime && (pExplode->flags & SITH_EF_EXPAND) != 0 )
            {
                if ( pExplode->msecFadeTime <= pExplode->msecExpandTime )
                {
                    float maxTime = (float)sithTime_g_msecGameTime + msecLifeLeft - (float)pExplode->msecFadeTime;
                    float deltaTime = (float)(sithTime_g_msecGameTime - pExplode->msecFadeTime);
                    start.z = sithExplosion_Lerp(pExplode->spriteStart.z, pExplode->spriteEnd.z, maxTime, deltaTime);
                }
                else
                {
                    float deltaTime = (float)(pExplode->msecFadeTime - sithTime_g_msecGameTime);
                    end.x = sithExplosion_Lerp(pExplode->spriteStart.x, pExplode->spriteEnd.x, msecLifeLeft, deltaTime);
                    end.y = sithExplosion_Lerp(pExplode->spriteStart.y, pExplode->spriteEnd.y, msecLifeLeft, deltaTime);
                    end.z = pExplode->spriteStart.z;

                    secAnimTime = (float)(pExplode->msecFadeTime - sithTime_g_msecGameTime) / 1000.0f;
                }

                sithAnimate_StartAnimateSpriteSize(pExplode->pSprite, &start, &end, secAnimTime);

                pExplode->msecExpandTime = sithTime_g_msecGameTime;
                pExplode->flags &= ~SITH_EF_EXPAND;
            }
            else if ( sithTime_g_msecGameTime >= pExplode->msecFadeTime && (pExplode->flags & SITH_EF_FADE) != 0 )
            {
                if ( pExplode->msecExpandTime <= pExplode->msecFadeTime )
                {
                    float maxTime = (float)sithTime_g_msecGameTime + msecLifeLeft - (float)pExplode->msecExpandTime;
                    float deltaTime = (float)(sithTime_g_msecGameTime - pExplode->msecExpandTime);
                    start.x = sithExplosion_Lerp(pExplode->spriteStart.x, pExplode->spriteEnd.x, maxTime, deltaTime);
                    start.y = sithExplosion_Lerp(pExplode->spriteStart.y, pExplode->spriteEnd.y, maxTime, deltaTime);
                }
                else
                {
                    float deltaTime = (float)(pExplode->msecExpandTime - sithTime_g_msecGameTime);
                    end.x = pExplode->spriteStart.x;
                    end.y = pExplode->spriteStart.y;
                    end.z = sithExplosion_Lerp(pExplode->spriteStart.z, pExplode->spriteEnd.z, msecLifeLeft, deltaTime);

                    secAnimTime = (float)(pExplode->msecExpandTime - sithTime_g_msecGameTime) / 1000.0f;
                }

                sithAnimate_StartAnimateSpriteSize(pExplode->pSprite, &start, &end, secAnimTime);

                pExplode->msecFadeTime = sithTime_g_msecGameTime;
                pExplode->flags &= ~SITH_EF_FADE;
            }
        }
        else
        {
            if ( pExplode->msecFadeTime == sithTime_g_msecGameTime )
            {
                float deltaTime = (float)(pExplode->msecExpandTime - sithTime_g_msecGameTime);
                end.x = pExplode->spriteStart.x;
                end.y = pExplode->spriteStart.y;
                end.z = sithExplosion_Lerp(pExplode->spriteStart.z, pExplode->spriteEnd.z, msecLifeLeft, deltaTime);

                secAnimTime = (float)(pExplode->msecExpandTime - sithTime_g_msecGameTime) / 1000.0f;
                sithAnimate_StartAnimateSpriteSize(pExplode->pSprite, &start, &end, secAnimTime);

                pExplode->flags &= ~SITH_EF_FADE;
            }
            else if ( pExplode->msecExpandTime == sithTime_g_msecGameTime )
            {
                float deltaTime = (float)(pExplode->msecFadeTime - sithTime_g_msecGameTime);
                end.x = sithExplosion_Lerp(pExplode->spriteStart.x, pExplode->spriteEnd.x, msecLifeLeft, deltaTime);
                end.y = sithExplosion_Lerp(pExplode->spriteStart.y, pExplode->spriteEnd.y, msecLifeLeft, deltaTime);
                end.z = pExplode->spriteStart.z;

                secAnimTime = (float)(pExplode->msecFadeTime - sithTime_g_msecGameTime) / 1000.0f;
                sithAnimate_StartAnimateSpriteSize(pExplode->pSprite, &start, &end, secAnimTime);

                pExplode->flags &= ~SITH_EF_EXPAND;// ~0x800 - SITH_EF_EXPAND
            }
            else
            {
                if ( pExplode->msecExpandTime <= pExplode->msecFadeTime )
                {
                    secAnimTime = (float)(pExplode->msecExpandTime - sithTime_g_msecGameTime) / 1000.0f;
                }
                else
                {
                    secAnimTime = (float)(pExplode->msecFadeTime - sithTime_g_msecGameTime) / 1000.0f;
                }

                sithAnimate_StartAnimateSpriteSize(pExplode->pSprite, &start, &start, secAnimTime);
            }

            sithExplosion_bExpandFadeSet = true;
        }

        if ( (pExplode->flags & SITH_EF_EXPAND) == 0 && (pExplode->flags & SITH_EF_FADE) == 0 )
        {
            sithExplosion_bExpandFadeSet = false;
        }
    }
}

float J3DAPI sithExplosion_Lerp(float min, float max, float maxTime, float deltaTime)
{
    if ( maxTime <= 0.0f )
    {
        return max;
    }

    return (max - min) * deltaTime / maxTime + min;
}

void J3DAPI sithExplosion_Update(SithThing* pThing, float sedDeltaTime)
{
    J3D_UNUSED(sedDeltaTime);

    SithExplosionInfo* pExplode = &pThing->thingInfo.explosionInfo;
    SITH_ASSERTREL(pThing && pExplode);

    if ( (pExplode->flags & SITH_EF_BLASTPHASE) != 0 && pThing->msecLifeLeft <= pExplode->msecBlastTime )
    {
        sithExplosion_MakeBlast(pThing);
        pExplode->flags &= ~SITH_EF_BLASTPHASE;
    }

    if ( (pExplode->flags & SITH_EF_CHILDEXPLOSION) != 0
        && pExplode->msecBabyTime <= sithTime_g_msecGameTime
        && pThing->pCreateThingTemplate )
    {
        SithThing* pChildExplosion = sithThing_CreateThingAtPos(pThing->pCreateThingTemplate, &pThing->pos, &pThing->orient, pThing->pInSector, pThing->pParent);
        if ( pChildExplosion && (pThing->flags & SITH_TF_REMOTE) != 0 )
        {
            pChildExplosion->flags |= SITH_TF_REMOTE; // Note: sithThing_CreateThingAtPos should already set SITH_TF_REMOTE
        }

        pExplode->flags &= ~SITH_EF_CHILDEXPLOSION;
    }

    if ( (pExplode->flags & SITH_EF_EXPAND) != 0 || (pExplode->flags & SITH_EF_FADE) != 0 )
    {
        sithExplosion_UpdateExpandFade(pExplode);
    }

    if ( (pExplode->flags & SITH_EF_VARIABLELIGHT) != 0 && (pThing->flags & SITH_TF_EMITLIGHT) != 0 )
    {
        double msecBlastTime;
        double msecLifeLeft;
        if ( pThing->msecLifeLeft <= pExplode->msecBlastTime )
        {
            msecLifeLeft  = (double)pThing->msecLifeLeft;
            msecBlastTime = (double)pExplode->msecBlastTime;
        }
        else
        {
            msecLifeLeft  = (double)(pExplode->msecLifeLeft - pThing->msecLifeLeft);
            msecBlastTime = (double)(pExplode->msecLifeLeft - pExplode->msecBlastTime);
        }

        float delta = (float)(msecLifeLeft / msecBlastTime);
        pThing->light.color.red   = pExplode->maxLight * delta + pThing->light.emitColor.red;
        pThing->light.color.green = pExplode->maxLight * delta + pThing->light.emitColor.green;
        pThing->light.color.blue  = pExplode->maxLight * delta + pThing->light.emitColor.blue;
    }
}

void J3DAPI sithExplosion_MakeBlast(SithThing* pThing)
{
    SITH_ASSERTREL(pThing && (pThing->type == SITH_THING_EXPLOSION));

    SithExplosionInfo* pExplode = &pThing->thingInfo.explosionInfo;
    const float exprange  = pExplode->range;
    const float expforce  = pExplode->force;
    float expdamage       = pExplode->damage;

    if ( exprange > 0.0f && (expdamage > 0.0f || expforce > 0.0f) )
    {
        sithAIAwareness_CreateTransmittingEvent(pThing->pInSector, &pThing->pos, 1, 3.0f, pThing);
        sithCollision_SearchForCollisions(pThing->pInSector, NULL, &pThing->pos, &rdroid_g_zeroVector3, 0.0f, exprange, 0x482);

        SithCollision* pCollision;
        while ( (pCollision = sithCollision_PopStack()) != NULL )
        {
            float scale = 1.0f - powf(pCollision->distance / exprange, 2);
            scale = STDMATH_CLAMP(scale, 0.25f, 1.0f);

            if ( (pCollision->type & SITHCOLLISION_WORLD) != 0 )
            {
                float damage = scale * expdamage;
                sithSurface_HandleThingImpact(pCollision->pSurfaceCollided, pThing, damage, pExplode->damageType);
            }
            else
            {
                SITH_ASSERTREL(((pCollision->type) & (SITHCOLLISION_THING)));

                SithThing* pVictim = pCollision->pThingCollided;
                if ( ((pExplode->flags & SITH_EF_NOSHOOTERDAMAGE) == 0 || pVictim != pThing->pParent) && sithCollision_CheckThingLOS(pThing, pVictim, 1) )
                {
                    if ( expforce != 0.0f && pVictim->moveType == SITH_MT_PHYSICS && (pVictim->moveInfo.physics.flags & SITH_PF_USEBLASTFORCE) != 0 )
                    {
                        rdVector3 vecForce;
                        rdVector_Scale3(&vecForce, &pCollision->hitNorm, -(expforce * scale));
                        sithPhysics_ApplyForce(pVictim, &vecForce);
                    }

                    expdamage = sithThing_ScaleCombatDamage(pThing, pVictim);
                    if ( expdamage != 0.0f )
                    {
                        float damage = scale * expdamage;
                        sithThing_DamageThing(pVictim, pThing, damage, pExplode->damageType);
                    }
                }
            }
        }

        sithCollision_DecreaseStackLevel();
    }

    if ( (pExplode->flags & SITH_EF_UPDATEDEBRISMATERIAL) == 0 || pExplode->pHitSurfaceMat )
    {
        for ( size_t i = 0; i < STD_ARRAYLEN(pExplode->apDebries); ++i )
        {
            if ( pExplode->apDebries[i] )
            {
                rdVector3 pyr;
                pyr.pitch = SITH_RAND() * 360.0f;
                pyr.yaw   = SITH_RAND() * 360.0f;
                pyr.roll  = SITH_RAND() * 360.0f;

                rdMatrix34 orient;
                rdMatrix_BuildRotate34(&orient, &pyr);

                SithThing* pDebris = sithThing_CreateThingAtPos(pExplode->apDebries[i], &pThing->pos, &orient, pThing->pInSector, NULL);
                if ( pDebris )
                {
                    // Extend life left up to 1.5sec
                    pDebris->msecLifeLeft += (uint32_t)(SITH_RAND() * 1500.0f);

                    if ( (pExplode->flags & SITH_EF_UPDATEDEBRISMATERIAL) != 0 && pDebris->renderData.data.pModel3 )
                    {
                        rdModel3* pModel = pDebris->renderData.data.pModel3;
                        for ( size_t j = 0; j < pModel->aGeos[0].numMeshes; ++j )
                        {
                            for ( size_t k = 0; k < pModel->aGeos[0].aMeshes[j].numFaces; ++k )
                            {
                                if ( pModel->aGeos[0].aMeshes[j].aFaces[k].pMaterial )
                                {
                                    pModel->aGeos[0].aMeshes[j].aFaces[k].pMaterial = pExplode->pHitSurfaceMat;
                                }
                            }
                        }
                    }
                    else if ( (pExplode->flags & SITH_EF_ANIMATEDEBRISMATERIAL) != 0 )
                    {
                        if ( pDebris )
                        {

                            if ( pDebris->renderData.data.pModel3 )
                            {
                                rdMaterial* pMat = pDebris->renderData.data.pModel3->apMaterials[i]; // TODO: Why mat array index is same as debris index? Bug?
                                if ( pMat )
                                {
                                    sithAnimate_StartMaterialAnim(pMat, 12.0f, (SithAnimateFlags)0);
                                }
                            }
                        }

                        pExplode->flags &= ~SITH_EF_ANIMATEDEBRISMATERIAL;
                    }
                }
            }
        }
    }
}

int J3DAPI sithExplosion_ParseArg(const StdConffileArg* pArg, SithThing* pThing, int adjNum)
{
    SITH_ASSERTREL(pThing && pArg && (adjNum < SITHTHING_NUMADJECTIVES));

    SithExplosionInfo* pExplode = &pThing->thingInfo.explosionInfo;
    switch ( adjNum )
    {
        case SITHTHING_ARG_TYPEFLAGS:
        {
            if ( sscanf_s(pArg->argValue, "%x", &pExplode->flags) != 1 )
            {
                return 0;
            }
            return 1;
        }
        case SITHTHING_ARG_DAMAGE:
        {
            pExplode->damage = atof(pArg->argValue);
            pExplode->flags |= SITH_EF_BLASTDAMAGE | SITH_EF_BLASTPHASE;
            return 1;
        }
        case SITHTHING_ARG_DAMAGECLASS:
        {
            if ( sscanf_s(pArg->argValue, "%x", &pExplode->damageType) != 1 )
            {
                return 0;
            }
            return 1;
        }
        case SITHTHING_ARG_BLASTTIME:
        {
            float secBlastTime = atof(pArg->argValue);
            pExplode->msecBlastTime = (uint32_t)(1000.0f * secBlastTime);
            pExplode->flags |= SITH_EF_BLASTPHASE;

            return 1;
        }
        case SITHTHING_ARG_BABYTIME:
        {
            float secBabyTime = atof(pArg->argValue);
            pExplode->msecBabyTime = (uint32_t)(1000.0f * secBabyTime);
            pExplode->flags |= SITH_EF_CHILDEXPLOSION;
            return 1;
        }
        case SITHTHING_ARG_FORCE:
        {
            pExplode->force = atof(pArg->argValue);
            pExplode->flags |= SITH_EF_BLASTPHASE;
            return 1;
        }
        case SITHTHING_ARG_MAXLIGHT:
        {
            pExplode->maxLight = atof(pArg->argValue);
            pExplode->flags |= SITH_EF_VARIABLELIGHT;
            return 1;
        }
        case SITHTHING_ARG_RANGE:
        {
            pExplode->range = atof(pArg->argValue);
            pExplode->flags |= SITH_EF_BLASTPHASE;
            return 1;
        }
        case SITHTHING_ARG_FLASHRGB:
        {
            // Note, flashColor is not used by the engine
            return sscanf_s(pArg->argValue, "(%d/%d/%d)", &pExplode->flashColor.red, &pExplode->flashColor.green, &pExplode->flashColor.blue) == 3;
        }
        case SITHTHING_ARG_EXPANDTIME:
        {
            float secExpandTime = atof(pArg->argValue);
            pExplode->msecExpandTime = (int32_t)(1000.0f * secExpandTime);
            pExplode->flags |= SITH_EF_EXPAND;
            return 1;
        }
        case SITHTHING_ARG_FADETIME:
        {
            float secFadeTime = atof(pArg->argValue);
            pExplode->msecFadeTime = (uint32_t)(1000.0f * secFadeTime);
            pExplode->flags |= SITH_EF_FADE;
            return 1;
        }
        case SITHTHING_ARG_DEBRIS:
        {
            for ( size_t i = 0; i < STD_ARRAYLEN(pExplode->apDebries); i++ )
            {
                if ( !pExplode->apDebries[i] )
                {
                    pExplode->apDebries[i] = sithTemplate_GetTemplate(pArg->argValue);
                    break;
                }
            }

            return 1;
        }
        case SITHTHING_ARG_SPRITETHING:
        {
            char aTplName[256]; // TODO: shrink to 64
            if ( sscanf_s(pArg->argValue, "%s", aTplName, (rsize_t)STD_ARRAYLEN(aTplName)) == 1 )
            {
                STD_STRCPY(pExplode->aSpriteTemplateName, aTplName);
                return 1;
            }
            else
            {
                memset(pExplode->aSpriteTemplateName, 0, sizeof(pExplode->aSpriteTemplateName));
                return 0;
            }
        }
        case SITHTHING_ARG_SPRITESTART:
        {
            return sscanf_s(pArg->argValue, "(%f/%f/%f)", &pExplode->spriteStart.x, &pExplode->spriteStart.y, &pExplode->spriteStart.z) == 3;
        }
        case SITHTHING_ARG_SPRITEEND:
        {
            return sscanf_s(pArg->argValue, "(%f/%f/%f)", &pExplode->spriteEnd.x, &pExplode->spriteEnd.y, &pExplode->spriteEnd.z) == 3;
        }
        default:
            return 0;
    }
}

#include "sithActor.h"
#include <j3dcore/j3dhook.h>

#include <sith/AI/sithAI.h>
#include <sith/AI/sithAIAwareness.h>
#include <sith/AI/sithAIMove.h>
#include <sith/Cog/sithCog.h>
#include <sith/Devices/sithComm.h>
#include <sith/Devices/sithSoundMixer.h>
#include <sith/Devices/sithConsole.h>
#include <sith/Dss/sithDSS.h>
#include <sith/Dss/sithDSSThing.h>
#include <sith/Engine/sithCollision.h>
#include <sith/Engine/sithPhysics.h>
#include <sith/Engine/sithPuppet.h>
#include <sith/Gameplay/sithInventory.h>
#include <sith/Gameplay/sithPlayer.h>
#include <sith/Gameplay/sithVehicleControls.h>
#include <sith/Main/sithMain.h>
#include <sith/RTI/symbols.h>
#include <sith/World/sithSoundClass.h>
#include <sith/World/sithTemplate.h>
#include <sith/World/sithThing.h>
#include <sith/World/sithVoice.h>
#include <sith/World/sithWorld.h>

#include <rdroid/Math/rdMatrix.h>
#include <rdroid/Math/rdVector.h>
#include <rdroid/Primitives/rdModel3.h>

#include <sound/Sound.h>

#include <std/Win95/stdComm.h>

#include <stdlib.h>

static float sithActor_headRotDampRate = 10.0f; // Added

void J3DAPI sithActor_PlayDamageSoundFx(SithThing* pThing, SithDamageType damageType);

void sithActor_InstallHooks(void)
{
    J3D_HOOKFUNC(sithActor_SetDifficulty);
    J3D_HOOKFUNC(sithActor_Update);
    J3D_HOOKFUNC(sithActor_DamageActor);
    J3D_HOOKFUNC(sithActor_DamageRaftActor);
    J3D_HOOKFUNC(sithActor_PlayDamageSoundFx);
    J3D_HOOKFUNC(sithActor_KillActor);
    J3D_HOOKFUNC(sithActor_SurfaceCollisionHandler);
    J3D_HOOKFUNC(sithActor_ActorCollisionHandler);
    J3D_HOOKFUNC(sithActor_SetHeadPYR);
    J3D_HOOKFUNC(sithActor_DestroyActor);
    J3D_HOOKFUNC(sithActor_DestroyCorpse);
    J3D_HOOKFUNC(sithActor_ParseArg);
}

void sithActor_ResetGlobals(void)
{
}

void J3DAPI sithActor_SetDifficulty(SithThing* pActor)
{
    if ( !pActor || pActor->type != SITH_THING_ACTOR )
    {
        SITHLOG_ERROR("Invalid ACTOR thing passed to sithActor_SetDifficulty().\n");
        return;
    }

    if ( pActor->controlType == SITH_CT_AI && pActor->controlInfo.aiControl.pClass )
    {
        float health;
        float maxHealth;
        if ( pActor->pTemplate )
        {
            float healthRatio = pActor->thingInfo.actorInfo.health / pActor->thingInfo.actorInfo.maxHealth;
            maxHealth         = pActor->pTemplate->thingInfo.actorInfo.maxHealth;
            maxHealth         = sithGetHitAccuarancyScalar() * maxHealth;
            health            = healthRatio * maxHealth;
        }
        else
        {
            maxHealth = pActor->thingInfo.actorInfo.maxHealth;
            maxHealth = sithGetHitAccuarancyScalar() * maxHealth;

            health = pActor->thingInfo.actorInfo.health;
            health = sithGetHitAccuarancyScalar() * health;
        }

        if ( maxHealth < 1.0f )
        {
            maxHealth = 1.0f;
        }

        else if ( maxHealth > 1000.0f )
        {
            maxHealth = 1000.0f;
        }

        pActor->thingInfo.actorInfo.maxHealth = maxHealth;
        if ( health < 1.0f )
        {
            health = 1.0f;
        }

        else if ( health > 1000.0f )
        {
            health = 1000.0f;
        }

        pActor->thingInfo.actorInfo.health = health;
    }
}

void J3DAPI sithActor_Update(SithThing* pThing, unsigned int msecDeltaTime)
{
    if ( (pThing->thingInfo.actorInfo.flags & SITH_AF_BREATHEUNDERWATER) == 0 && (pThing->flags & (SITH_TF_DYING | SITH_TF_DESTROYED)) == 0 )
    {
        if ( (pThing->moveInfo.physics.flags & SITH_PF_ONWATERSURFACE) == 0 && (pThing->pInSector->flags & SITH_SECTOR_UNDERWATER) != 0 )
        {
#ifdef J3D_DEBUG
            // Note, found in debug version of Indy3D
            if ( (pThing->moveInfo.physics.flags & SITH_PF_RAFT) != 0 )
            {
                sithConsole_PrintString("Uh Oh. Raft underwater.");
            }
#endif

            pThing->thingInfo.actorInfo.endurance.msecUnderwater += msecDeltaTime;
            if ( pThing->thingInfo.actorInfo.endurance.msecUnderwater >= SITHACTOR_MAX_UNDERWATER_MSEC )
            {
                float damage = (float)msecDeltaTime / 5.0f;
                damage       = J3DMAX(damage, 1.0f); // not ideal solution. Applies too much damage on high framerates
                sithThing_DamageThing(pThing, pThing, damage, SITH_DAMAGE_DROWN);
                pThing->thingInfo.actorInfo.endurance.msecUnderwater = SITHACTOR_MAX_UNDERWATER_MSEC;
            }
        }
        else if ( pThing->thingInfo.actorInfo.endurance.msecUnderwater && (pThing->moveInfo.physics.flags & SITH_PF_RAFT) == 0 )
        {
            // actor is swimming afloat (on water surface) in water

            if ( pThing->thingInfo.actorInfo.endurance.msecUnderwater > (SITHACTOR_MAX_UNDERWATER_MSEC - 10000) )
            {
                if ( !SITH_ISFRAMECYCLE(pThing->idx, 16) ) // Not on every 16th frame 
                {
                    if ( pThing->thingInfo.actorInfo.endurance.msecUnderwater > (SITHACTOR_MAX_UNDERWATER_MSEC / 2) // TODO: ???, verify this if statement
                        && SITH_ISFRAMECYCLE(pThing->idx, 8) )                                                      // On every 8th frame
                    {
                        sithSoundClass_PlayModeRandom(pThing, SITHSOUNDCLASS_BREATH);
                    }
                }
                else
                {
                    sithSoundClass_PlayModeRandom(pThing, SITHSOUNDCLASS_GASP);
                }
            }

            pThing->thingInfo.actorInfo.endurance.msecUnderwater -= 8 * msecDeltaTime;
            if ( pThing->thingInfo.actorInfo.endurance.msecUnderwater > SITHACTOR_MAX_UNDERWATER_MSEC ) // TODO: ???
            {
                pThing->thingInfo.actorInfo.endurance.msecUnderwater = 0;
            }
        }
        else if ( pThing->thingInfo.actorInfo.endurance.raftLeakDamage
            && (pThing->moveInfo.physics.flags & SITH_PF_RAFT) != 0
            && pThing->moveStatus != SITHPLAYERMOVE_RAFT_UNBOARDING_LEFT
            && pThing->moveStatus != SITHPLAYERMOVE_RAFT_UNBOARDING_RIGHT
            && pThing->moveStatus != SITHPLAYERMOVE_RAFT_UNBOARD_START
            && pThing->moveStatus != SITHPLAYERMOVE_RAFT_BOARDING )
        {
            // Update Raft damage

            float damage = (float)sithGetGameDifficulty() * (0.60000002f - 1.2f) / 5.0f + 1.2f;
            damage       = (float)msecDeltaTime / damage;
            sithThing_DamageThing(pThing, pThing, damage, SITH_DAMAGE_RAFT_LEAK);
        }

        if ( pThing->thingInfo.actorInfo.pThingMeshAttached )
        {
            SithThing* pThingMeshAttached = pThing->thingInfo.actorInfo.pThingMeshAttached;
            int nodeNum                   = pThing->thingInfo.actorInfo.attachMeshNum; // TODO: [BUG] the meshNum and the actual model nodeNum can have different number

            int bSkipBuildingJoints                            = pThingMeshAttached->renderData.bSkipBuildingJoints;
            pThingMeshAttached->renderData.bSkipBuildingJoints = 1;

            rdMatrix34 meshOrient;
            rdModel3_GetMeshMatrix(&pThingMeshAttached->renderData, &pThingMeshAttached->orient, nodeNum, &meshOrient);
            pThingMeshAttached->renderData.bSkipBuildingJoints = bSkipBuildingJoints;

            pThing->pos    = meshOrient.dvec;
            pThing->orient = meshOrient;
            sithThing_SetSector(pThing, pThingMeshAttached->pInSector, 1);
        }

        sithVoice_UpdateLipSync(pThing);
    }
}

float J3DAPI sithActor_DamageActor(SithThing* pActor, SithThing* pThing, float damage, SithDamageType damageType)
{
    SITH_ASSERTREL(pActor && (damage > 0.0f));
    SITH_ASSERTREL(pActor->type == SITH_THING_ACTOR || pActor->type == SITH_THING_PLAYER);

    if ( pActor->type == SITH_THING_PLAYER
        && (pActor->moveStatus == SITHPLAYERMOVE_RAFT_BOARDING
            || pActor->moveStatus == SITHPLAYERMOVE_RAFT_UNBOARDING_LEFT
            || pActor->moveStatus == SITHPLAYERMOVE_RAFT_UNBOARDING_RIGHT
            || pActor->moveStatus == SITHPLAYERMOVE_RAFT_UNBOARD_START
            || pActor->moveStatus == SITHPLAYERMOVE_MINECAR_BOARDING
            || pActor->moveStatus == SITHPLAYERMOVE_MINECAR_UNBOARDING_LEFT
            || pActor->moveStatus == SITHPLAYERMOVE_MINECAR_UNBOARDING_RIGHT
            || pActor->moveStatus == SITHPLAYERMOVE_JEEP_BOARDING
            || pActor->moveStatus == SITHPLAYERMOVE_JEEP_UNBOARDING) )
    {
        return 0.0f;
    }

    if ( stdComm_IsGameActive() && (pActor->flags & SITH_TF_REMOTE) != 0 )
    {
        if ( pActor->pPuppetClass && pActor != pThing && damage * 0.050000001f > SITH_RAND() )
        {
            sithPuppet_PlayMode(pActor, SITHPUPPETSUBMODE_HIT, NULL);
        }

        return damage;
    }

    if ( (pActor->thingInfo.actorInfo.flags & SITH_AF_INVULNERABLE) != 0 && damageType != SITH_DAMAGE_CRUSH )
    {
        if ( !pThing->pParent )
        {
            return 0.0f;
        }

        if ( pThing->pParent->type == SITH_THING_PLAYER )
        {
            sithActor_PlayDamageSoundFx(pActor, damageType);
        }

        return 0.0f;
    }

    if ( pActor->thingInfo.actorInfo.health <= 0.0f )
    {
        return damage;
    }

    if ( pActor->type == SITH_THING_PLAYER )
    {
        damage = sithInventory_BroadcastMessage(pActor, SITHCOG_SYM_REF_THING, pThing->idx, SITHCOG_MSG_DAMAGED, SITHINVENTORY_TYPE_DAMAGEABLE, damage, damageType, 0, 0);
        if ( damage == 0.0f )
        {
            return 0.0f;
        }
    }

    pActor->thingInfo.actorInfo.health = pActor->thingInfo.actorInfo.health - damage;
    sithActor_PlayDamageSoundFx(pActor, damageType);
    if ( pThing )
    {
        if ( pThing != pActor && pActor->controlType == SITH_CT_AI )
        {
            sithAI_EmitEvent(pActor->controlInfo.aiControl.pLocal, SITHAI_EVENT_HIT_SECTOR, (void*)pThing);
        }

        pThing = sithThing_GetThingParent(pThing);
    }

    if ( pActor->thingInfo.actorInfo.health >= 1.0f )
    {
        if ( pActor->pPuppetClass && pActor != pThing && damage * 0.050000001f > SITH_RAND() )
        {
            sithPuppet_PlayMode(pActor, SITHPUPPETSUBMODE_HIT, NULL);
        }

        return damage;
    }

    // Actor died

    if ( sithMessage_g_outputstream )
    {
        sithDSSThing_Death(pActor, pThing, 0, SITHMESSAGE_SENDTOJOINEDPLAYERS, SITHMESSAGE_STREAM_ALL);
    }

    sithActor_KillActor(pActor, pThing, damageType);
    return damage - pActor->thingInfo.actorInfo.health;
}

float J3DAPI sithActor_DamageRaftActor(SithThing* pActor, SithThing* pPerpetrator, float damage, SithDamageType damageType)
{
    SITH_ASSERTREL(pActor && (damage > 0.0f));
    SITH_ASSERTREL(pActor->type == SITH_THING_ACTOR || pActor->type == SITH_THING_PLAYER);

    if ( stdComm_IsGameActive() && (pActor->flags & SITH_TF_REMOTE) != 0 )
    {
        if ( pActor->pPuppetClass && pActor != pPerpetrator && damage * 0.050000001f > SITH_RAND() )
        {
            sithPuppet_PlayMode(pActor, SITHPUPPETSUBMODE_HIT, NULL);
        }

        return damage;
    }

    if ( (pActor->thingInfo.actorInfo.flags & SITH_AF_INVULNERABLE) != 0 && damageType != SITH_DAMAGE_CRUSH )
    {
        return 0.0f;
    }

    if ( pActor->thingInfo.actorInfo.endurance.raftLeakDamage >= SITHACTOR_MAX_RAFT_DAMAGE )
    {
        return damage;
    }

    if ( pActor->type == SITH_THING_PLAYER )
    {
        damage = sithInventory_BroadcastMessage(pActor, SITHCOG_SYM_REF_THING, pPerpetrator->idx, SITHCOG_MSG_DAMAGED, SITHINVENTORY_TYPE_DAMAGEABLE, damage, damageType, 0, 0);
        if ( damage == 0.0f )
        {
            return 0.0f;
        }
    }

    pActor->thingInfo.actorInfo.endurance.raftLeakDamage += (uint32_t)damage;
    if ( (damageType & SITH_DAMAGE_RAFT_LEAK) != 0 )
    {
        sithSoundClass_PlayModeFirst(pActor, SITHSOUNDCLASS_JUMPWOOD);
    }
    else
    {
        sithSoundClass_PlayModeFirst(pActor, SITHSOUNDCLASS_HITDAMAGED);
    }

    if ( pActor->thingInfo.actorInfo.endurance.raftLeakDamage < SITHACTOR_MAX_RAFT_DAMAGE )
    {
        if ( pActor->pPuppetClass && pActor != pPerpetrator && damage * 0.050000001f > SITH_RAND() )
        {
            sithPuppet_PlayMode(pActor, SITHPUPPETSUBMODE_HIT, NULL);
        }

        return damage;
    }

    // Raft actor died

    if ( sithMessage_g_outputstream )
    {
        sithDSSThing_Death(pActor, pPerpetrator, 0, SITHMESSAGE_SENDTOJOINEDPLAYERS, SITHMESSAGE_STREAM_ALL);
    }

    sithActor_KillActor(pActor, pPerpetrator, damageType);
    return damage;
}

void J3DAPI sithActor_PlayDamageSoundFx(SithThing* pThing, SithDamageType damageType)
{
    if ( damageType <= SITH_DAMAGE_FISTS )
    {
        if ( damageType == SITH_DAMAGE_FISTS )
        {
            sithSoundClass_PlayModeRandom(pThing, SITHSOUNDCLASS_HURTFISTS);
            return;
        }

        if ( damageType != SITH_DAMAGE_ENERGY )
        {
            if ( damageType == SITH_DAMAGE_FIRE )
            {
                sithSoundClass_PlayModeRandom(pThing, SITHSOUNDCLASS_HURTFIRE);
                return;
            }

            goto LABEL_43;
        }

        goto hurt_energy;
    }

    if ( damageType > SITH_DAMAGE_DROWN )
    {
        if ( damageType > SITH_DAMAGE_IMP1 )
        {
            if ( damageType > (SITH_DAMAGE_2000 | SITH_DAMAGE_IMP1) )
            {
                if ( damageType > SITH_DAMAGE_IMP5 )
                {
                    if ( damageType > SITH_DAMAGE_LASER )
                    {
                        if ( damageType == SITH_DAMAGE_BONK )
                        {
                            goto hurt_impact;
                        }

                        if ( damageType == SITH_DAMAGE_IMP_BLAST )
                        {
                            return;
                        }

                    LABEL_43:
                        if ( pThing->type == SITH_THING_PLAYER && ((pThing->flags & SITH_TF_SUBMERGED) != 0 || SITH_RAND() > 0.30000001f) )
                        {
                            return;
                        }

                    hurt_impact:
                        sithSoundClass_PlayModeRandom(pThing, SITHSOUNDCLASS_HURTIMPACT);
                        return;
                    }

                    if ( damageType != SITH_DAMAGE_LASER && damageType != SITH_DAMAGE_LIGHTNING )
                    {
                        goto LABEL_43;
                    }

                hurt_energy:
                    sithSoundClass_PlayModeRandom(pThing, SITHSOUNDCLASS_HURTENERGY);
                    return;
                }

                if ( damageType != SITH_DAMAGE_IMP5 && damageType != SITH_DAMAGE_IMP4 )
                {
                    goto LABEL_43;
                }
            }
            else if ( damageType != (SITH_DAMAGE_2000 | SITH_DAMAGE_IMP1) && damageType != SITH_DAMAGE_2000 )
            {
                goto LABEL_43;
            }
        }
        else if ( damageType != SITH_DAMAGE_IMP1 && damageType != SITH_DAMAGE_POISON )
        {
            goto LABEL_43;
        }
    }
    else if ( damageType == SITH_DAMAGE_DROWN )
    {
        if ( SITH_ISFRAMECYCLE(pThing->idx, 16) ) // On every 16th frame make drawning sound fx
        {
            sithSoundClass_PlayModeRandom(pThing, SITHSOUNDCLASS_DROWNING);
        }
    }
    else
    {
        if ( damageType != SITH_DAMAGE_MACHETE )
        {
            goto LABEL_43;
        }

        // Damage fx for machete 
        if ( !sithSoundClass_PlayModeRandom(pThing, SITHSOUNDCLASS_HURTMACHETE) )
        {
            tSoundHandle hSnd;
            if ( (pThing->thingInfo.actorInfo.flags & SITH_AF_DROID) != 0 )
            {
                hSnd = Sound_GetSoundHandle(SITHWORLD_STATICINDEX(81)); // gen_machete_hit_stone.wav
            }
            else
            {
                hSnd = Sound_GetSoundHandle(SITHWORLD_STATICINDEX(80)); // gen_machete_hit_flesh.wav
            }

            sithSoundMixer_PlaySoundPos(hSnd, &pThing->pos, pThing->pInSector, 1.0f, 0.5f, 2.5f, SOUNDPLAY_ABSOLUTE_POS);
        }
    }
}

void J3DAPI sithActor_KillActor(SithThing* pThing, SithThing* pSrcThing, SithDamageType damageType)
{
    if ( (pThing->flags & (SITH_TF_DYING | SITH_TF_DESTROYED)) != 0 )
    {
        return;
    }

    float curHealth                    = pThing->thingInfo.actorInfo.health;
    pThing->thingInfo.actorInfo.health = 0.0f;

    sithCog_ThingSendMessageEx(pThing, pSrcThing, SITHCOG_MSG_KILLED, 0, damageType, 0, 0);

    if ( (pThing->flags & SITH_TF_DESTROYED) != 0 )
    {
        return;
    }

    sithSoundMixer_StopSoundThing(pThing, 0);

    if ( (pThing->moveInfo.physics.flags & SITH_PF_JEEP) == 0 && pThing->moveStatus != SITHPLAYERMOVE_RUNOVER )
    {
        if ( damageType == SITH_DAMAGE_DROWN )
        {
            sithSoundClass_PlayModeRandom(pThing, SITHSOUNDCLASS_DROWNED);
        }

        else if ( damageType == SITH_DAMAGE_CRUSH )
        {
            sithSoundClass_PlayModeRandom(pThing, SITHSOUNDCLASS_SPLATTERED);
        }

        else if ( (pThing->flags & SITH_TF_SUBMERGED) != 0 )
        {
            sithSoundClass_PlayModeRandom(pThing, SITHSOUNDCLASS_DEATHUNDER);
        }

        else if ( curHealth >= -10.0f )
        {
            sithSoundClass_PlayModeRandom(pThing, SITHSOUNDCLASS_DEATH1);
        }
        else
        {
            sithSoundClass_PlayModeRandom(pThing, SITHSOUNDCLASS_DEATH2);
        }
    }

    sithActor_SetHeadPYR(pThing, &rdroid_g_zeroVector3);
    sithAIAwareness_CreateTransmittingEvent(pThing->pInSector, &pThing->pos, 0, 2.0f, pSrcThing);

    if ( pThing->type == SITH_THING_PLAYER )
    {
        sithPlayer_PlayerKilledAction(pThing, pSrcThing);
    }

    pThing->moveInfo.physics.flags &= ~SITH_PF_CROUCHING;

    //flags = pThing->flags;
    //flags = pThing->flags | 0x200;// 0x200 - dying
    //pThing->flags = flags;
    pThing->flags |= SITH_TF_DYING;

    //v4 = pThing->flags;
    //v4 = pThing->flags & ~0x4000;// 0x4000 - HasShadow
    //pThing->flags = v4;
    pThing->flags &= ~SITH_TF_SHADOW;

    if ( pThing->type == SITH_THING_PLAYER )
    {
        switch ( pThing->moveStatus )
        {
            case SITHPLAYERMOVE_CRAWLIDLE:
            case SITHPLAYERMOVE_UNKNOWN_4:
            case SITHPLAYERMOVE_UNKNOWN_5:
                sithPuppet_SetMoveMode(pThing, SITHPUPPET_MOVEMODE_NORMAL);
                sithPuppet_RemoveAllTracks(pThing);
                pThing->collide.movesize = 0.039999999f;
                break;

            case SITHPLAYERMOVE_HANGING:
            case SITHPLAYERMOVE_CLIMBIDLE:
            case SITHPLAYERMOVE_CLIMBING_UP:
            case SITHPLAYERMOVE_CLIMBING_DOWN:
            case SITHPLAYERMOVE_CLIMBING_LEFT:
            case SITHPLAYERMOVE_CLIMBING_RIGHT:
                sithThing_DetachThing(pThing);
                pThing->moveInfo.physics.flags |= SITH_PF_FLOORSTICK;
                pThing->collide.movesize = 0.039999999f;
                break;

            default:
                break;
        }
    }

    rdPuppetTrackCallback pfCallback = NULL;
    if ( pThing->type == SITH_THING_ACTOR && pThing->controlType == SITH_CT_AI )
    {
        pfCallback = sithAIMove_PuppetCallback;
        if ( pThing->moveStatus >= SITHPLAYERMOVE_ROLLING_LEFT && pThing->moveStatus <= SITHPLAYERMOVE_ROLLING_RIGHT )
        {
            sithPuppet_SetArmedMode(pThing, 1u);
        }
    }

    if ( pThing->pPuppetClass && pThing->moveStatus != SITHPLAYERMOVE_RUNOVER )
    {
        sithPuppet_RemoveAllTracks(pThing);
        if ( (pThing->moveInfo.physics.flags & SITH_PF_RAFT) != 0 )
        {
            if ( pThing->thingInfo.actorInfo.endurance.raftLeakDamage < SITHACTOR_MAX_RAFT_DAMAGE )
            {
                sithPuppet_PlayMode(pThing, SITHPUPPETSUBMODE_DEATH, NULL);
            }
            else
            {
                sithPuppet_PlayMode(pThing, SITHPUPPETSUBMODE_DEATH2, sithVehicleControls_PuppetCallback);
                pThing->thingInfo.actorInfo.bControlsDisabled = 1;
            }
        }
        else if ( (pThing->moveInfo.physics.flags & SITH_PF_JEEP) != 0 )
        {
            sithPuppet_PlayMode(pThing, SITHPUPPETSUBMODE_DEATH, sithVehicleControls_PuppetCallback);
            pThing->thingInfo.actorInfo.bControlsDisabled = 1;
        }
        else
        {
            if ( damageType == SITH_DAMAGE_COLD_WATER )
            {
                pThing->pPuppetState->majorMode = SITH_PUPPET_GETMOVEMAJORMODE(pThing, SITHPUPPET_MOVEMODE_SWIM); // TODO: why not use sithPuppet_SetMoveMode?
                curHealth                       = -31.0f;
            }

            if ( curHealth < -30.0f && pThing->pPuppetClass->aModes[pThing->pPuppetState->majorMode][SITHPUPPETSUBMODE_DEATH2].pKeyframe )
            {
                sithPuppet_PlayMode(pThing, SITHPUPPETSUBMODE_DEATH2, pfCallback); // water drown
            }
            else
            {
                sithPuppet_PlayMode(pThing, SITHPUPPETSUBMODE_DEATH, pfCallback); // underwater drown
            }
        }
    }

    if ( pThing->type != SITH_THING_PLAYER )
    {
        if ( pThing->controlType == SITH_CT_AI )
        {
            sithAI_Stop(pThing);
        }

        if ( (pThing->thingInfo.actorInfo.flags & SITH_AF_EXPLODE_WHEN_KILLED) != 0 && pThing->thingInfo.actorInfo.pExplodeTemplate )
        {
            sithThing_CreateThingAtPos(pThing->thingInfo.actorInfo.pExplodeTemplate, &pThing->pos, &pThing->orient, pThing->pInSector, NULL);
            sithThing_DestroyThing(pThing);
        }
        else
        {
            if ( (pThing->thingInfo.actorInfo.flags & SITH_AF_BREATHEUNDERWATER) != 0 )
            {
                pThing->moveInfo.physics.buoyancy = 0.30000001f;
            }

            if ( (pThing->moveInfo.physics.flags & SITH_PF_FLY) != 0 )
            {
                sithActor_DestroyActor(pThing);
            }
            else
            {
                pThing->msecLifeLeft = 3000;
            }
        }
    }
}

int J3DAPI sithActor_SurfaceCollisionHandler(SithThing* pThing, SithSurface* pSurface, SithCollision* pHitStack)
{
    if ( pThing->controlType != SITH_CT_AI )
    {
        return 0;
    }

    if ( (pThing->moveInfo.physics.flags & SITH_PF_MINECAR) != 0 && pThing->collide.movesize > 0.30000001f )
    {
        return 0;
    }

    int bCollided = sithCollision_HandleThingHitSurface(pThing, pSurface, pHitStack);
    if ( !bCollided )
    {
        return bCollided;
    }

    if ( (pSurface->flags & SITH_SURFACE_ISFLOOR) != 0 )
    {
        sithAI_EmitEvent(pThing->controlInfo.aiControl.pLocal, SITHAI_EVENT_HIT_FLOOR, pSurface);
    }
    else
    {
        sithAI_EmitEvent(pThing->controlInfo.aiControl.pLocal, SITHAI_EVENT_HIT_WALL, pSurface);
    }

    return bCollided;
}

int J3DAPI sithActor_ActorCollisionHandler(SithThing* pSrcThing, SithThing* pThing, SithCollision* pCollision, int a4)
{
    int bCollision = sithCollision_ThingCollisionHandler(pSrcThing, pThing, pCollision, a4);
    if ( !bCollision )
    {
        return 0;
    }

    if ( pSrcThing->controlType == SITH_CT_AI && pSrcThing->controlInfo.aiControl.pLocal )
    {
        sithAI_EmitEvent(pSrcThing->controlInfo.aiControl.pLocal, SITHAI_EVENT_TOUCHED, pThing);
    }

    if ( pThing->controlType == SITH_CT_AI && pThing->controlInfo.aiControl.pLocal )
    {
        sithAI_EmitEvent(pThing->controlInfo.aiControl.pLocal, SITHAI_EVENT_TOUCHED, pSrcThing);
    }

    return bCollision;
}

void J3DAPI sithActor_SetHeadPYR(SithThing* pThing, const rdVector3* headAngles)
{
    sithActor_RotateHead(pThing, headAngles, /*secDeltaTime=*/ 0.0f);
}

void J3DAPI sithActor_RotateHead(SithThing* pThing, const rdVector3* headAngles, float secDeltaTime)
{
    SITH_ASSERTREL(pThing && headAngles && ((pThing->type == SITH_THING_ACTOR) || (pThing->type == SITH_THING_PLAYER) || (pThing->type == SITH_THING_CORPSE)));

#ifndef J3D_QOL_IMPROVEMENTS
    // Restore original code that won't rotate interpolate joint
    secDeltaTime = 0.0f;
#endif

    pThing->thingInfo.actorInfo.flags &= ~SITH_AF_VIEWCENTRED;

    if ( secDeltaTime <= 0.0f )
    {
        pThing->thingInfo.actorInfo.headPYR = *headAngles;
    }
    else
    {
        rdVector_SmoothDamp3Acc(&pThing->thingInfo.actorInfo.headPYR, headAngles, sithActor_headRotDampRate, secDeltaTime);
    }

    if ( pThing->pPuppetClass && pThing->renderData.type == RD_THING_MODEL3 && pThing->renderData.apTweakedAngles )
    {
        int lastNodeNum = pThing->renderData.data.pModel3->numHNodes - 1;

        int neckNum = pThing->pPuppetClass->aJoints[SITHPUPPET_JOINTINDEX_NECK];
        if ( neckNum >= 0 && neckNum <= lastNodeNum )
        {
            // Altered: Added smooth interpolation
            /*pThing->renderData.apTweakedAngles[neckNum].pitch = headAngles->pitch * 0.5f;
            pThing->renderData.apTweakedAngles[neckNum].yaw = headAngles->yaw;*/
            rdModel3_BendJointPitch(&pThing->renderData, neckNum, headAngles->pitch * 0.5f, sithActor_headRotDampRate, secDeltaTime);
            rdModel3_BendJointYaw(&pThing->renderData, neckNum, headAngles->yaw, sithActor_headRotDampRate, secDeltaTime);
        }

        int hipNum = pThing->pPuppetClass->aJoints[SITHPUPPET_JOINTINDEX_HIP];
        if ( hipNum >= 0 && hipNum <= lastNodeNum )
        {
            // Altered: Added smooth interpolation
            // pThing->renderData.apTweakedAngles[hipNum].pitch = headAngles->pitch * 0.5f;
            rdModel3_BendJointPitch(&pThing->renderData, hipNum, headAngles->pitch * 0.5f, sithActor_headRotDampRate, secDeltaTime);
        }

        int aim1Num = pThing->pPuppetClass->aJoints[SITHPUPPET_JOINTINDEX_AIM1];
        if ( aim1Num >= 0 && aim1Num <= lastNodeNum )
        {
            // Altered: Added smooth interpolation
            //pThing->renderData.apTweakedAngles[aim1Num].pitch = headAngles->pitch * 0.30000001f;
            rdModel3_BendJointPitch(&pThing->renderData, aim1Num, headAngles->pitch * 0.30000001f, sithActor_headRotDampRate, secDeltaTime);
        }

        int aim2Num = pThing->pPuppetClass->aJoints[SITHPUPPET_JOINTINDEX_AIM2];
        if ( aim2Num >= 0 && aim2Num <= lastNodeNum )
        {
            // Altered: Added smooth interpolation
            //pThing->renderData.apTweakedAngles[aim2Num].pitch = headAngles->pitch * 0.30000001f;
            rdModel3_BendJointPitch(&pThing->renderData, aim2Num, headAngles->pitch * 0.30000001f, sithActor_headRotDampRate, secDeltaTime);
        }
    }
}

void J3DAPI sithActor_UpdateAimJoints(SithThing* pThing)
{
    SITH_ASSERTREL(pThing && ((pThing->type == SITH_THING_ACTOR) || (pThing->type == SITH_THING_PLAYER) || (pThing->type == SITH_THING_CORPSE)));
    if ( pThing->pPuppetClass )
    {
        int aimPitchNum = pThing->pPuppetClass->aJoints[SITHPUPPET_JOINTINDEX_AIMPITCH];
        if ( aimPitchNum >= 0 )
        {
            pThing->renderData.apTweakedAngles[aimPitchNum].pitch = pThing->thingInfo.actorInfo.headPYR.pitch;
        }

        int aimYawNum = pThing->pPuppetClass->aJoints[SITHPUPPET_JOINTINDEX_AIMYAW];
        if ( aimYawNum >= 0 )
        {
            pThing->renderData.apTweakedAngles[aimYawNum].yaw = pThing->thingInfo.actorInfo.headPYR.yaw;
        }
    }
}

void J3DAPI sithActor_DestroyActor(SithThing* pActor)
{
    /*flags = pActor->flags;
    flags = pActor->flags | SITH_TF_DYING;
    pActor->flags = flags;*/

    pActor->flags |= SITH_TF_DYING;

    sithThing_DetachAttachedThings(pActor);

    pActor->type         = SITH_THING_CORPSE;
    pActor->msecLifeLeft = 30000; // corpse will be visible for 30 sec

    pActor->moveInfo.physics.flags |= SITH_PF_FLOORSTICK | SITH_PF_ALIGNSURFACE | SITH_PF_USEGRAVITY;
    pActor->moveInfo.physics.flags &= ~(SITH_PF_FLY | SITH_PF_ALIGNUP | SITH_PF_ALIGNED | SITH_PF_WALLSTICK);
    sithPhysics_FindFloor(pActor, 0);
}

void J3DAPI sithActor_DestroyCorpse(SithThing* pThing)
{
    // Thing should be of a type SITH_THING_CORPSE, set in sithActor_DestroyActor
    // Fyi, here we can make the actor corpse to not disappear
    if ( pThing->renderFrame + 1 == sithMain_g_frameNumber )
    {
        // In camera view, extend life for 3 sec
        pThing->msecLifeLeft = 3000;
    }
    else
    {
        sithThing_DestroyThing(pThing);
    }
}

int J3DAPI sithActor_ParseArg(const StdConffileArg* pArg, SithThing* pThing, int adjNum)
{
    SithActorInfo* pActorInfo = &pThing->thingInfo.actorInfo;
    switch ( adjNum )
    {
        case SITHTHING_ARG_TYPEFLAGS:
        {
            SithActorFlag actorflags;
            if ( sscanf_s(pArg->argValue, "%x", &actorflags) != 1 )
            {
                goto error;
            }

            pActorInfo->flags = actorflags;
            return 1;
        }
        case SITHTHING_ARG_HEALTH:
        {
            float health = strtof(pArg->argValue, NULL); // Changed: Use strtof instead atof
            if ( health < 0.0f || (errno == ERANGE) )    // Added: Conversion range error check
            {
                goto error;
            }

            pActorInfo->health = health;

            if ( pActorInfo->health >= pActorInfo->maxHealth )
            {
                pActorInfo->maxHealth = health;
            }
            return 1;
        }
        case SITHTHING_ARG_MAXTHRUST:
        {
            float maxthrust = strtof(pArg->argValue, NULL); // Changed: Use strtof instead atof
            if ( maxthrust < 0.0f || (errno == ERANGE) )    // Added: Conversion range error check
            {
                goto error;
            }

            pActorInfo->maxThrust = maxthrust;
            return 1;
        }
        case SITHTHING_ARG_MAXROTTHRUST:
        {
            float maxrotthrust = strtof(pArg->argValue, NULL); // Changed: Use strtof instead atof
            if ( maxrotthrust < 0.0f || (errno == ERANGE) )    // Added: Conversion range error check
            {
                goto error;
            }

            pActorInfo->maxRotVelocity = maxrotthrust;
            return 1;
        }
        case SITHTHING_ARG_MAXHEADVEL:
        {
            float maxheadvel = strtof(pArg->argValue, NULL); // Changed: Use strtof instead atof
            if ( maxheadvel < 0.0f || (errno == ERANGE) )    // Added: Conversion range error check
            {
                goto error;
            }

            pActorInfo->maxHeadVelocity = maxheadvel;
            return 1;
        }
        case SITHTHING_ARG_MAXHEADYAW:
        {
            float maxheadyaw = strtof(pArg->argValue, NULL); // Changed: Use strtof instead atof
            if ( maxheadyaw < 0.0f || (errno == ERANGE) )    // Added: Conversion range error check
            {
                goto error;
            }

            pActorInfo->maxHeadYaw = maxheadyaw;
            return 1;
        }
        case SITHTHING_ARG_JUMPSPEED:
        {
            float jumpspeed = strtof(pArg->argValue, NULL); // Changed: Use strtof instead atof
            if ( jumpspeed < 0.0f || (errno == ERANGE) )    // Added: Conversion range error check
            {
                goto error;
            }

            pActorInfo->jumpSpeed = jumpspeed;
            return 1;
        }
        case SITHTHING_ARG_WEAPON:
        {
            pActorInfo->pWeaponTemplate = sithTemplate_GetTemplate(pArg->argValue);
            return 1;
        }
        case SITHTHING_ARG_EXPLODE:
        {
            pActorInfo->pExplodeTemplate = sithTemplate_GetTemplate(pArg->argValue);
            return 1;
        }
        case SITHTHING_ARG_MAXHEALTH:
        {
            float maxhealth = strtof(pArg->argValue, NULL); // Changed: Use strtof instead atof
            if ( maxhealth < 0.0f || (errno == ERANGE) )    // Added: Conversion range error check
            {
                goto error;
            }

            pActorInfo->maxHealth = maxhealth;
            return 1;
        }
        case SITHTHING_ARG_EYEOFFSET:
        {
            if ( sscanf_s(pArg->argValue, "(%f/%f/%f)", &pActorInfo->eyeOffset.x, &pActorInfo->eyeOffset.y, &pActorInfo->eyeOffset.z) != 3 )
            {
                goto error;
            }
            return 1;
        }
        case SITHTHING_ARG_MINHEADPITCH:
        {
            float minpitch;
            if ( sscanf_s(pArg->argValue, "%f", &minpitch) != 1 )
            {
                goto error;
            }

            pActorInfo->minHeadPitch = minpitch;
            return 1;
        }
        case SITHTHING_ARG_MAXHEADPITCH:
        {
            float maxpitch;
            if ( sscanf_s(pArg->argValue, "%f", &maxpitch) != 1 )
            {
                goto error;
            }

            pActorInfo->maxHeadPitch = maxpitch;
            return 1;
        }
        case SITHTHING_ARG_FIREOFFSET:
        {
            if ( sscanf_s(pArg->argValue, "(%f/%f/%f)", &pActorInfo->fireOffset.x, &pActorInfo->fireOffset.y, &pActorInfo->fireOffset.z) != 3 )
            {
                goto error;
            }
            return 1;
        }
        case SITHTHING_ARG_LIGHTOFFSET:
        {
            if ( sscanf_s(pArg->argValue, "(%f/%f/%f)", &pActorInfo->lightOffset.x, &pActorInfo->lightOffset.y, &pActorInfo->lightOffset.z) != 3 )
            {
                goto error;
            }

            pThing->flags |= SITH_TF_EMITLIGHT;
            return 1;
        }
        case SITHTHING_ARG_LIGHTINTENSITY:
        {
            rdVector4 headLight = { 0 }; // Added: Init to 0
            if ( sscanf_s(pArg->argValue, "(%f/%f/%f)", &headLight.red, &headLight.green, &headLight.blue) != 3 )
            {
                goto error;
            }

            // TODO: Add additional format option with light range (see thing light param)
            pActorInfo->headLightIntensity = headLight;
            pThing->flags |= SITH_TF_EMITLIGHT;
            return 1;
        }
        case SITHTHING_ARG_VOICECOLOR:
        {
            if ( sscanf_s(
                pArg->argValue,
                "(%f/%f/%f/%f/%f/%f/%f/%f/%f/%f/%f/%f/%f/%f/%f/%f)",
                &pActorInfo->voiceInfo.voiceColor.top.red,
                &pActorInfo->voiceInfo.voiceColor.top.green,
                &pActorInfo->voiceInfo.voiceColor.top.blue,
                &pActorInfo->voiceInfo.voiceColor.top.alpha,
                &pActorInfo->voiceInfo.voiceColor.middle.red,
                &pActorInfo->voiceInfo.voiceColor.middle.green,
                &pActorInfo->voiceInfo.voiceColor.middle.blue,
                &pActorInfo->voiceInfo.voiceColor.middle.alpha,
                &pActorInfo->voiceInfo.voiceColor.bottomLeft.red,
                &pActorInfo->voiceInfo.voiceColor.bottomLeft.green,
                &pActorInfo->voiceInfo.voiceColor.bottomLeft.blue,
                &pActorInfo->voiceInfo.voiceColor.bottomLeft.alpha,
                &pActorInfo->voiceInfo.voiceColor.bottomRight.red,
                &pActorInfo->voiceInfo.voiceColor.bottomRight.green,
                &pActorInfo->voiceInfo.voiceColor.bottomRight.blue,
                &pActorInfo->voiceInfo.voiceColor.bottomRight.alpha) != 16 )
            {
                goto error;
            }
            return 1;
        }
        default:
            return 0;
    }

error:
    SITHLOG_STATUS("Invalid argument '%s=%s' line %d.\n", pArg->argName, pArg->argValue, stdConffile_GetLineNumber());
    return 0;
}

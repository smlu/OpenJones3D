#include "sithAIInstinct.h"
#include <j3dcore/j3dhook.h>
#include <sith/RTI/symbols.h>

#include <rdroid/Math/rdMath.h>
#include <rdroid/Math/rdMatrix.h>
#include <rdroid/Math/rdVector.h>

#include <sith/AI/sithAI.h>
#include <sith/AI/sithAIAwareness.h>
#include <sith/AI/sithAIMove.h>
#include <sith/AI/sithAIUtil.h>
#include <sith/Cog/sithCog.h>
#include <sith/Devices/sithSound.h>
#include <sith/Devices/sithSoundMixer.h>
#include <sith/Engine/sithCollision.h>
#include <sith/Engine/sithPhysics.h>
#include <sith/Engine/sithPuppet.h>
#include <sith/Gameplay/sithPlayerActions.h>
#include <sith/Gameplay/sithPlayer.h>
#include <sith/Gameplay/sithPlayerControls.h>
#include <sith/Gameplay/sithTime.h>
#include <sith/Main/sithMain.h>
#include <sith/World/sithActor.h>
#include <sith/World/sithSector.h>
#include <sith/World/sithSoundClass.h>
#include <sith/World/sithThing.h>
#include <sith/World/sithWeapon.h>
#include <sith/World/sithWorld.h>

#include <std/General/std.h>
#include <std/General/stdMath.h>

#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#define sithAIInstinct_flt_539AB8 J3D_DECL_FAR_VAR(sithAIInstinct_flt_539AB8, float)

void sithAIInstinct_InstallHooks(void)
{
    J3D_HOOKFUNC(sithAIInstinct_InitInstincts);
    J3D_HOOKFUNC(sithAIInstinct_Listen);
    J3D_HOOKFUNC(sithAIInstinct_LookForTarget);
    J3D_HOOKFUNC(sithAIInstinct_SenseDanger);
    J3D_HOOKFUNC(sithAIInstinct_FearGunshot);
    J3D_HOOKFUNC(sithAIInstinct_Hop);
    J3D_HOOKFUNC(sithAIInstinct_HoverDrift);
    J3D_HOOKFUNC(sithAIInstinct_Talk);
    J3D_HOOKFUNC(sithAIInstinct_RandomTurn);
    J3D_HOOKFUNC(sithAIInstinct_OpenDoors);
    J3D_HOOKFUNC(sithAIInstinct_Jump);
    J3D_HOOKFUNC(sithAIInstinct_LobFire);
    J3D_HOOKFUNC(sithAIInstinct_PrimaryFire);
    J3D_HOOKFUNC(sithAIInstinct_AlternateFire);
    J3D_HOOKFUNC(sithAIInstinct_CircleStrafe);
    J3D_HOOKFUNC(sithAIInstinct_Dodge);
    J3D_HOOKFUNC(sithAIInstinct_HitAndRun);
    J3D_HOOKFUNC(sithAIInstinct_Retreat);
    J3D_HOOKFUNC(sithAIInstinct_RandomMove);
    J3D_HOOKFUNC(sithAIInstinct_HumanCombatMove);
    J3D_HOOKFUNC(sithAIInstinct_Roam);
    J3D_HOOKFUNC(sithAIInstinct_ReturnHome);
    J3D_HOOKFUNC(sithAIInstinct_Flee);
    J3D_HOOKFUNC(sithAIInstinct_BasicFallow);
    J3D_HOOKFUNC(sithAIInstinct_WallCrawl);
    J3D_HOOKFUNC(sithAIInstinct_sub_494360);
    J3D_HOOKFUNC(sithAIInstinct_SnakeMungeTestCheck);
    J3D_HOOKFUNC(sithAIInstinct_SnakeFollow);
    J3D_HOOKFUNC(sithAIInstinct_sub_494FF0);
}

void sithAIInstinct_ResetGlobals(void)
{
    float sithAIInstinct_flt_539AB8_tmp = 1.0f;
    memcpy(&sithAIInstinct_flt_539AB8, &sithAIInstinct_flt_539AB8_tmp, sizeof(sithAIInstinct_flt_539AB8));
    
}

void J3DAPI sithAIInstinct_InitInstincts()
{
    sithAI_RegisterInstinct("listen", (SithAIInstinctFunc)sithAIInstinct_Listen, 0, 0, 7);
    sithAI_RegisterInstinct("lookfortarget", (SithAIInstinctFunc)sithAIInstinct_LookForTarget, 0x204, 0x800, 0x100);
    sithAI_RegisterInstinct("sensedanger", (SithAIInstinctFunc)sithAIInstinct_SenseDanger, 0x804, 0, 7);
    sithAI_RegisterInstinct("primaryfire", (SithAIInstinctFunc)sithAIInstinct_PrimaryFire, 2, 0x800, 0);
    sithAI_RegisterInstinct("alternatefire", (SithAIInstinctFunc)sithAIInstinct_AlternateFire, 2, 0x800, 0);
    sithAI_RegisterInstinct("lobfire", (SithAIInstinctFunc)sithAIInstinct_LobFire, 2, 0x800, 0);
    sithAI_RegisterInstinct("opendoors", (SithAIInstinctFunc)sithAIInstinct_OpenDoors, 2, 0, 0);
    sithAI_RegisterInstinct("jump", (SithAIInstinctFunc)sithAIInstinct_Jump, 0, 0, 0x604);
    sithAI_RegisterInstinct("randomturn", (SithAIInstinctFunc)sithAIInstinct_RandomTurn, 4, 0, 0);
    sithAI_RegisterInstinct("roam", (SithAIInstinctFunc)sithAIInstinct_Roam, 4, 0xA12, 0xF04);
    sithAI_RegisterInstinct("flee", (SithAIInstinctFunc)sithAIInstinct_Flee, 0x800, 0, 0xF04);
    sithAI_RegisterInstinct("retreat", (SithAIInstinctFunc)sithAIInstinct_Retreat, 2, 0x1000800, 1);
    sithAI_RegisterInstinct("circlestrafe", (SithAIInstinctFunc)sithAIInstinct_CircleStrafe, 2, 0x1000800, 0x88E04);
    sithAI_RegisterInstinct("returnhome", (SithAIInstinctFunc)sithAIInstinct_ReturnHome, 0, 0, 0x900);
    sithAI_RegisterInstinct("talk", (SithAIInstinctFunc)sithAIInstinct_Talk, -1, 0, 0x400104);
    sithAI_RegisterInstinct("hitandrun", (SithAIInstinctFunc)sithAIInstinct_HitAndRun, 0xC00, 4, 0x4000);
    sithAI_RegisterInstinct("dodge", (SithAIInstinctFunc)sithAIInstinct_Dodge, 0, 0x100000, 0x81803);
    sithAI_RegisterInstinct("hop", (SithAIInstinctFunc)sithAIInstinct_Hop, 1, 0, 0);
    sithAI_RegisterInstinct("hoverdrift", (SithAIInstinctFunc)sithAIInstinct_HoverDrift, 0x6FE, 0xF001, 0);
    sithAI_RegisterInstinct("basicfollow", (SithAIInstinctFunc)sithAIInstinct_BasicFallow, 0x202, 0x1004814, 0x8CE04);
    sithAI_RegisterInstinct("wallcrawl", (SithAIInstinctFunc)sithAIInstinct_WallCrawl, 0x202, 0x3004014, 0x10A604);
    sithAI_RegisterInstinct("snakefollow", (SithAIInstinctFunc)sithAIInstinct_SnakeFollow, 6, 0, 0x8E04);
    sithAI_RegisterInstinct("randommove", (SithAIInstinctFunc)sithAIInstinct_RandomMove, 2, 0x104805, 0x4800);
    sithAI_RegisterInstinct("humancombatmove", (SithAIInstinctFunc)sithAIInstinct_HumanCombatMove, 2, 0x1104805, 0x80901);
    sithAI_RegisterInstinct("feargunshot", (SithAIInstinctFunc)sithAIInstinct_FearGunshot, 6, 0x800, 2);
}

int J3DAPI sithAIInstinct_Listen(SithAIControlBlock* pLocal, SithAIInstinct* pInstinct, SithAIInstinctState* pState, SithAIEventType event, SithThing* pThing)
{
    SITH_ASSERTREL(pLocal && pLocal->pClass && pLocal->pOwner);
    if ( (pLocal->mode & SITHAI_MODE_SEARCHING) == 0 )
    {
        return 0;
    }

    if ( pThing && (pThing->flags & SITH_TF_DISABLED) != 0 )
    {
        return 0;
    }

    SithThing* pListener = pLocal->pOwner;
    SithAIAwarenessSector* pSectorInfo = &sithAIAwareness_g_aSectors[sithSector_GetSectorIndex(pListener->pInSector)];
    if ( !pSectorInfo )
    {
        SITHLOG_ERROR("Listen: Bad sector data passed!\n");
        return 0;
    }

    switch ( event )
    {
        case SITHAI_EVENT_HIT_SECTOR:
        case SITHAI_EVENT_TOUCHED:
            if ( pState->aParams[0] == 0.0f )
            {
                if ( event == SITHAI_EVENT_HIT_SECTOR )
                {
                    sithAIUtil_AIPlaySoundMode(pLocal, SITHSOUNDCLASS_SURPRISE);
                }
                else
                {
                    sithAIUtil_AIPlaySoundMode(pLocal, SITHSOUNDCLASS_CURIOUS);
                }
                pState->aParams[0] = 0.1f;
            }

            if ( pThing )
            {
                SithThing* pShooter = sithThing_GetThingParent(pThing);
                if ( event == SITHAI_EVENT_HIT_SECTOR )
                {
                    if ( SITH_RANDF() > pInstinct->fltArg[1] )
                    {
                        sithAIMove_AISetLookThing(pLocal, pShooter);
                        return 0;
                    }

                    if ( (pLocal->mode & (SITHAI_MODE_HUNTING | SITHAI_MODE_INSTINCTUSEWPNTS)) != 0 )
                    {
                        sithAIUtil_AISetMode(pLocal, SITHAI_MODE_HUNTING);
                        pLocal->pTargetThing = sithPlayer_g_pLocalPlayerThing;
                        if ( sithAIUtil_AIMoveToNextWpnt(pLocal, 1.7f, 0.0f, 0) )
                        {
                            return 1;
                        }
                    }
                    else
                    {
                        sithAIMove_AISetLookThing(pLocal, pShooter);
                        sithAIMove_AISetMovePos(pLocal, &pShooter->pos, 1.7f);
                        return 1;
                    }
                }
                else if ( event == SITHAI_EVENT_TOUCHED )
                {
                    if ( pShooter->type == SITH_THING_PLAYER )
                    {
                        sithAIMove_StopAIMovement(pLocal);
                        sithAIMove_AISetLookPosEyeLevel(pLocal, &pShooter->pos);
                        return 0;
                    }

                    if ( SITH_RANDF() > pInstinct->fltArg[1] )
                    {
                        sithAIMove_AISetLookThing(pLocal, pShooter);
                        return 0;
                    }

                    if ( (pLocal->mode & (SITHAI_MODE_HUNTING | SITHAI_MODE_INSTINCTUSEWPNTS)) != 0 )
                    {
                        if ( sithAIUtil_AIMoveToNextWpnt(pLocal, pLocal->moveSpeed, 0.0f, 0) )
                        {
                            return 0;
                        }
                    }
                    else
                    {
                        rdVector3 pos;

                        rdVector_Sub3(&pos, &pLocal->pOwner->pos, &pShooter->pos);
                        rdVector_Normalize3Acc(&pos);
                        if ( rdVector_Dot3(&pos, &pLocal->moveDirection) < 0.0f )
                        {
                            rdVector_Scale3Acc(&pos, pInstinct->fltArg[2]);
                            rdVector_Add3Acc(&pos, &pLocal->pOwner->pos);
                            sithAIMove_StopAIMovement(pLocal);
                            sithAIMove_AISetLookPosEyeLevel(pLocal, &pos);
                            sithAIMove_AISetMoveTargetPos(pLocal, &pos, 0.75f);
                            return 0;
                        }
                    }
                }
            }
            return 0;

        case SITHAI_EVENT_SOUND:
            if ( (pListener->thingInfo.actorInfo.flags & SITH_AF_DEAF) != 0 )
            {
                return 0;
            }
            break;

        default:
            return 0;
    }

    if ( pSectorInfo->aLevelAtTransmittingPos[0] != 0.0f && SITH_RANDF() < pInstinct->fltArg[0] )
    {
        int bMove = 0;
        int bLook = 0;
        int bStop = 0;
        int bHunt = 0;
        SithThing* pSource = pSectorInfo->aTransmittingThing[0];
        if ( pSource && (pSource->thingInfo.actorInfo.flags & SITH_AF_INVISIBLE) != 0 )
        {
            return 0;
        }

        if ( !sithAIAwareness_IsInTransmittingRange(pSectorInfo, 0, pListener) )
        {
            return 0;
        }

        float distance;
        rdVector3 toDest;
        int sightVal = sithAIUtil_GetDistanceToTargetPos(pListener, &pListener->pos, &pSectorInfo->aStartPos[0], -1.0f, pLocal->pClass->sightDistance, 0.0f, &toDest, &distance);
        if ( distance <= pLocal->pClass->heardDistance )
        {
            int thingVis;
            if ( pSource && pSource->type )
            {
                rdVector3 thingVec;
                float hitDist;
                thingVis = sithAIUtil_GetDistanceToTargetPos(pListener, &pListener->pos, &pSource->pos, -1.0f, pLocal->pClass->sightDistance, 0.0f, &thingVec, &hitDist);
            }
            else
            {
                thingVis = 3;
            }

            float loudness = 0.0f;
            if ( pSource && pSource->type && (pLocal->mode & (SITHAI_MODE_HUNTING | SITHAI_MODE_INSTINCTUSEWPNTS)) != 0 )
            {
                bHunt = 1;

                // TODO: [BUG] The original never sets loudness on this path; it reads the stack slot of the pInstinct argument as a float
                memcpy(&loudness, &pInstinct, sizeof(loudness));
            }
            else if ( !thingVis )
            {
                rdVector_Copy3(&pLocal->alertPos, &pSource->pos);
                loudness = pSectorInfo->aLevelAtTransmittingPos[0];
                bMove = 1;
                bLook = 1;
            }
            else if ( !sightVal )
            {
                rdVector_Copy3(&pLocal->alertPos, &pSectorInfo->aStartPos[0]);
                loudness = pSectorInfo->aLevelAtTransmittingPos[0];
                bMove = 1;
                bLook = 1;
            }
            else if ( pSource && (pLocal->mode & SITHAI_MODE_HUNTING) == 0 )
            {
                loudness = pSectorInfo->aLevelAtTransmittingPos[0];
                bStop = 1;
                bLook = 1;
            }

            if ( bStop || bMove || bLook || bHunt )
            {
                if ( pSource && pSource->type )
                {
                    int bSrcUnderwater = (pSource->pInSector->flags & SITH_SECTOR_UNDERWATER) != 0;
                    int underwater     = (pLocal->pOwner->pInSector->flags & SITH_SECTOR_UNDERWATER) != 0;
                    if ( (bSrcUnderwater && !underwater) || (!bSrcUnderwater && underwater) )
                    {
                        return 0;
                    }
                }

                if ( loudness > pState->aParams[0] )
                {
                    pState->aParams[0] = loudness;
                }

                sithAIUtil_AIPlaySoundMode(pLocal, SITHSOUNDCLASS_CURIOUS);
                if ( bStop )
                {
                    sithAIMove_StopAIMovement(pLocal);
                }

                if ( bMove && SITH_RANDF() <= pInstinct->fltArg[1] )
                {
                    sithAIMove_AISetMovePos(pLocal, &pLocal->alertPos, 0.75f);
                }

                if ( bLook )
                {
                    sithAIMove_AISetLookPosEyeLevel(pLocal, &pLocal->alertPos);
                }

                if ( bHunt && SITH_RANDF() <= pInstinct->fltArg[1] )
                {
                    sithAIUtil_AISetMode(pLocal, SITHAI_MODE_HUNTING);
                    pLocal->pTargetThing = sithPlayer_g_pLocalPlayerThing;
                    sithAIUtil_AIMoveToNextWpnt(pLocal, 1.7f, 0.0f, 0);
                }
            }
        }
    }
    return 0;
}

int J3DAPI sithAIInstinct_LookForTarget(SithAIControlBlock* pLocal, SithAIInstinct* pInstinct, SithAIInstinctState* pState, SithAIEventType event, SithAIMode newMode)
{
    if ( event == SITHAI_EVENT_MODECHANGED && newMode )
    {
        if ( (pLocal->mode & SITHAI_MODE_HUNTING) != 0 && (newMode & SITHAI_MODE_HUNTING) == 0 )
        {
            pState->aParams[0] = sithTime_g_secGameTime;
        }
    }

    if ( event )
    {
        return 0;
    }

    if ( (sithMain_g_sith_mode.debugModeFlags & SITHDEBUG_AINOTARGET) != 0 )
    {
        return 0;
    }

    if ( sithPlayerControls_g_bCutsceneMode == 1 )
    {
        return 0;
    }

    if ( (pLocal->mode & SITHAI_MODE_HUNTING) != 0 )
    {
        float curhuntTime = pInstinct->fltArg[3];
        if ( !pLocal->pTargetThing )
        {
            SITHLOG_DEBUG("LookForTarget: AI '%s' lost track of its target.  Ending HUNT mode.\n", pLocal->pOwner->aName);
            sithAIUtil_AIClearMode(pLocal, SITHAI_MODE_HUNTING);
        }
        else
        {
            if ( curhuntTime == 0.0f )
            {
                curhuntTime = 20.0f;
            }

            if ( sithTime_g_secGameTime > pState->aParams[0] + curhuntTime )
            {
                sithAIUtil_AIClearMode(pLocal, SITHAI_MODE_HUNTING);
            }
            else if ( (pLocal->pOwner->thingInfo.actorInfo.flags & SITH_AF_HUMAN) == 0 && (pLocal->pTargetThing->moveInfo.physics.flags & (SITH_PF_MINECAR | SITH_PF_RAFT | SITH_PF_JEEP | SITH_PF_UNKNOWN_8000000)) != 0 )
            {
                sithAIUtil_AIClearMode(pLocal, SITHAI_MODE_HUNTING);
            }
        }
    }

    if ( (pLocal->mode & SITHAI_MODE_ACTIVE) != 0 )
    {
        if ( !pInstinct->intArg[1] )
        {
            return 0;
        }

        if ( pLocal->msecAttackStart + pInstinct->intArg[1] < sithTime_g_msecGameTime )
        {
            sithAIUtil_AISetMode(pLocal, SITHAI_MODE_SEARCHING);
            if ( (pLocal->mode & SITHAI_MODE_INSTINCTUSEWPNTS) != 0 )
            {
                if ( (pLocal->pOwner->thingInfo.actorInfo.flags & SITH_AF_HUMAN) == 0 && (pLocal->pTargetThing->moveInfo.physics.flags & (SITH_PF_MINECAR | SITH_PF_RAFT | SITH_PF_JEEP | SITH_PF_UNKNOWN_8000000)) != 0 )
                {
                    return 0;
                }

                sithAIUtil_AISetMode(pLocal, SITHAI_MODE_HUNTING);
                sithAIUtil_AIMoveToNextWpnt(pLocal, 1.7f, 0.0f, 0);
            }
            return 1;
        }
        return 0;
    }

    if ( (pLocal->mode & SITHAI_MODE_SEARCHING) == 0 )
    {
        return 0;
    }

    pState->msecNextUpdate = sithTime_g_msecGameTime + pInstinct->intArg[0];
    if ( (pLocal->pOwner->thingInfo.actorInfo.flags & SITH_AF_BLIND) != 0 )
    {
        return 0;
    }

    pLocal->pTargetThing = sithPlayer_g_pLocalPlayerThing;
    SITH_ASSERTREL(pLocal->pTargetThing);
    if ( !sithAIUtil_CanAttackTarget(pLocal->pTargetThing) )
    {
        pLocal->pTargetThing = NULL;
        return 0;
    }

    if ( (pLocal->pOwner->thingInfo.actorInfo.flags & SITH_AF_HUMAN) == 0
        && (pLocal->pOwner->moveInfo.physics.flags & (SITH_PF_MINECAR | SITH_PF_RAFT | SITH_PF_JEEP | SITH_PF_UNKNOWN_8000000)) == 0
        && (pLocal->pTargetThing->moveInfo.physics.flags & (SITH_PF_MINECAR | SITH_PF_RAFT | SITH_PF_JEEP | SITH_PF_UNKNOWN_8000000)) != 0 )
    {
        pLocal->pTargetThing = NULL;
        return 0;
    }

    sithAIUtil_sub_49B2E0(pLocal, 0);
    if ( !pLocal->targetSightState )
    {
        if ( pLocal->goalThing == pLocal->pTargetThing )
        {
            pLocal->unknown141 = sithMain_g_frameNumber;
            pLocal->targetSightState2 = pLocal->targetSightState;
            rdVector_Copy3(&pLocal->vecUnknown0, &pLocal->toTarget);
            pLocal->targetDistance = pLocal->distance;
            rdVector_Copy3(&pLocal->vecUnknown5, &pLocal->pTargetThing->pos);
            rdVector_Copy3(&pLocal->vecUnknown6, &pLocal->pTargetThing->pos);
            pLocal->unknown150 = sithTime_g_msecGameTime;
        }

        if ( pInstinct->fltArg[2] != 0.0f )
        {
            sithAIMove_AISetLookThing(pLocal, pLocal->pTargetThing);
            return 0;
        }

        sithAIUtil_AISetMode(pLocal, SITHAI_MODE_ATTACKING);
        if ( (pLocal->submode & SITHAI_SUBMODE_CONTINUOUSWPNTMOTION) == 0 )
        {
            sithAIUtil_AIClearMode(pLocal, SITHAI_MODE_TRAVERSEWPNTS);
        }

        sithAIUtil_AIPlaySoundMode(pLocal, SITHSOUNDCLASS_ALERT);
        sithAIUtil_AIPlaySoundMode(pLocal, SITHSOUNDCLASS_ACTIVATE);
        sithAIAwareness_CreateTransmittingEvent(pLocal->pTargetThing->pInSector, &pLocal->pOwner->pos, 0, 1.5f, pLocal->pTargetThing);
        pLocal->goalThing = pLocal->pTargetThing;
        return 1;
    }

    if ( pInstinct->fltArg[0] == 0.0f )
    {
        pInstinct->fltArg[0] = 500.0f;
    }
    return 0;
}

int J3DAPI sithAIInstinct_SenseDanger(SithAIControlBlock* pLocal, SithAIInstinct* pInstance, SithAIInstinctState* pState, SithAIEventType event, SithThing* pThing)
{
    SithThing* pOwner = pLocal->pOwner;
    SithAIAwarenessSector* pAISector = &sithAIAwareness_g_aSectors[sithSector_GetSectorIndex(pOwner->pInSector)];
    if ( !event && pInstance->intArg[0] )
    {
        pState->msecNextUpdate = sithTime_g_msecGameTime + pInstance->intArg[0];
    }

    if ( (pLocal->mode & SITHAI_MODE_FLEEING) != 0 )
    {
        return 0;
    }

    if ( (pLocal->mode & SITHAI_MODE_SEARCHING) == 0 )
    {
        return 0;
    }

    switch ( event )
    {
        case SITHAI_EVENT_HIT_SECTOR:
            if ( SITH_RANDF() > pInstance->fltArg[3] )
            {
                return 0;
            }

            sithAIUtil_AIPlaySoundMode(pLocal, SITHSOUNDCLASS_SURPRISE);
            if ( pThing )
            {
                pLocal->pFleeFromThing = sithThing_GetThingParent(pThing);
            }

            sithAIUtil_AISetMode(pLocal, SITHAI_MODE_FLEEING);
            return 1;

        case SITHAI_EVENT_SOUND:
            if ( pAISector->aLevelAtTransmittingPos[2] <= pInstance->fltArg[1] )
            {
                return 0;
            }

            SithThing* pDanger = pAISector->aTransmittingThing[2];
            if ( !pDanger )
            {
                return 0;
            }

            if ( pDanger == pLocal->pOwner )
            {
                return 0;
            }

            float distance;
            rdVector3 toTarget;
            int curSightState = sithAIUtil_GetDistanceToTarget(pOwner, &pOwner->pos, pDanger, -1.0f, pLocal->pClass->heardDistance, 0.0f, &toTarget, &distance);
            if ( curSightState )
            {
                return 0;
            }

            pLocal->pFleeFromThing = pDanger;
            if ( (pLocal->mode & SITHAI_MODE_FLEEING) == 0 )
            {
                sithAIUtil_AIPlaySoundMode(pLocal, SITHSOUNDCLASS_FEAR);
                sithAIAwareness_CreateTransmittingEvent(pLocal->pOwner->pInSector, &pLocal->pOwner->pos, 2, 4.0f, pLocal->pOwner);
            }

            sithAIUtil_AISetMode(pLocal, SITHAI_MODE_FLEEING);
            return 1;

        case (SithAIEventType)0:
            if ( SITH_RANDF() > pInstance->fltArg[2] )
            {
                return 0;
            }

            pLocal->pTargetThing = sithPlayer_g_pLocalPlayerThing;
            if ( !sithAIUtil_CanAttackTarget(pLocal->pTargetThing) )
            {
                pLocal->pTargetThing = NULL;
                return 0;
            }

            sithAIUtil_sub_49B2E0(pLocal, 0);
            if ( !pLocal->targetSightState )
            {
                sithAIUtil_AISetMode(pLocal, SITHAI_MODE_FLEEING);
                sithAIUtil_AIPlaySoundMode(pLocal, SITHSOUNDCLASS_FEAR);
                sithAIAwareness_CreateTransmittingEvent(pLocal->pOwner->pInSector, &pLocal->pOwner->pos, 1, 3.0f, pLocal->pOwner);
                pLocal->pFleeFromThing = pLocal->pTargetThing;
            }
            break;
    }

    return 0;
}

signed int J3DAPI sithAIInstinct_FearGunshot(SithAIControlBlock* pLocal, SithAIInstinct* pInstinct, SithAIInstinctState* pState, SithAIEventType event, void* pObject)
{
    J3D_UNUSED(pState);
    J3D_UNUSED(pObject);

    SithThing* owner = pLocal->pOwner;
    SithAIAwarenessSector* pAISector = &sithAIAwareness_g_aSectors[sithSector_GetSectorIndex(owner->pInSector)];
    if ( (pLocal->mode & SITHAI_MODE_FLEEING) != 0 )
    {
        return 0;
    }

    switch ( event )
    {
        case SITHAI_EVENT_SOUND:
            if ( pAISector->aLevelAtTransmittingPos[3] > 0.0f )
            {
                SithThing* pShooter = pAISector->aTransmittingThing[3];
                if ( pShooter && pShooter != pLocal->pOwner )
                {
                    rdVector3 curdir;
                    rdVector_Sub3(&curdir, &pLocal->pOwner->pos, &pShooter->pos);
                    float dist = rdVector_Normalize3Acc(&curdir);
                    float dot  = rdVector_Dot3(&pShooter->orient.lvec, &curdir);
                    if ( dist < pInstinct->fltArg[0] || (dist < pInstinct->fltArg[1] && dot > pInstinct->fltArg[3] && SITH_RANDF() < pInstinct->fltArg[2]) )
                    {
                        sithAIUtil_AISetMode(pLocal, SITHAI_MODE_FLEEING);
                        pLocal->pFleeFromThing = pShooter;
                        sithAIUtil_AIPlaySoundMode(pLocal, SITHSOUNDCLASS_FEAR);
                        return 1;
                    }
                }
            }
            break;
    }

    return 0;
}

int J3DAPI sithAIInstinct_Hop(SithAIControlBlock* pLocal, SithAIInstinct* pInstinct, SithAIInstinctState* pState, SithAIEventType event, void* pObject)
{
    J3D_UNUSED(event);
    J3D_UNUSED(pObject);

    float hopDelay = (float)pInstinct->intArg[0];
    pState->msecNextUpdate = sithTime_g_msecGameTime + (uint32_t)(hopDelay * SITH_RANDF());
    if ( !pLocal->pOwner->attach.flags )
    {
        return 0;
    }

    if ( (pLocal->mode & SITHAI_MODE_MOVING) == 0 )
    {
        return 0;
    }

    sithThing_DetachThing(pLocal->pOwner);
    float hopForce = pInstinct->fltArg[1];
    rdVector3 force;
    rdVector_Set3(&force, 0.0f, 0.0f, hopForce * SITH_RANDF());
    sithPhysics_ApplyForce(pLocal->pOwner, &force);
    sithAIUtil_AIPlaySoundMode(pLocal, SITHSOUNDCLASS_JUMP);
    return 0;
}

int J3DAPI sithAIInstinct_HoverDrift(SithAIControlBlock* pLocal, SithAIInstinct* pInstinct, SithAIInstinctState* pState, SithAIEventType event, void* pObject)
{
    J3D_UNUSED(event);
    J3D_UNUSED(pObject);

    SITH_ASSERTREL(pLocal && pLocal->pOwner);
    if ( (pLocal->pOwner->moveInfo.physics.flags & SITH_PF_FLY) == 0 )
    {
        return 0;
    }

    float curSpread = pInstinct->fltArg[0];
    curSpread = (SITH_RANDF() - 0.5f) * curSpread + curSpread;
    pState->msecNextUpdate = sithTime_g_msecGameTime + (uint32_t)curSpread;
    if ( (pLocal->mode & SITHAI_MODE_MOVING) != 0 || (pLocal->pOwner->thingInfo.actorInfo.flags & SITH_AF_IMMOBILE) != 0 )
    {
        return 0;
    }

    rdVector3 direction;
    direction.x = (SITH_RANDF() - 0.5f) * 2.0f;
    direction.y = (SITH_RANDF() - 0.5f) * 2.0f;
    direction.z = (SITH_RANDF() - 0.5f) * 2.0f;

    // Note, the original draws a separate random scale for each axis
    rdVector3 curForce;
    curForce.x = SITH_RANDF() * pInstinct->fltArg[1] * direction.x;
    curForce.y = SITH_RANDF() * pInstinct->fltArg[1] * direction.y;
    curForce.z = SITH_RANDF() * pInstinct->fltArg[1] * direction.z;
    sithPhysics_ApplyForce(pLocal->pOwner, &curForce);
    return 0;
}

int J3DAPI sithAIInstinct_Talk(SithAIControlBlock* pLocal, SithAIInstinct* pInstinct, SithAIInstinctState* pState, SithAIEventType event, void* pObject)
{
    float talkChance = pInstinct->fltArg[1];
    float delay = pInstinct->fltArg[0];
    float nextTalkTime = -1.0f;
    SithThing* pOwner = pLocal->pOwner;
    int bPlayerWater = 0;
    int bOwnerWater = 0;
    int bSuccess = 0;
    int hChannel = 0;
    if ( pInstinct->fltArg[2] != 0.0f )
    {
        nextTalkTime = pState->aParams[0] + pInstinct->fltArg[2];
    }

    int arg;
    switch ( event )
    {
        case (SithAIEventType)0:
            if ( (pLocal->mode & SITHAI_MODE_UNKNOWN_100000) != 0 )
            {
                delay = delay / 2.0f;
                talkChance = (float)(talkChance * 2.0);
            }

            delay = delay * 1000.0f;
            pState->msecNextUpdate = sithTime_g_msecGameTime + (uint32_t)delay;
            if ( nextTalkTime > sithTime_g_secGameTime )
            {
                return 0;
            }

            if ( sithPlayer_g_pLocalPlayerThing )
            {
                bPlayerWater = (sithPlayer_g_pLocalPlayerThing->pInSector->flags & SITH_SECTOR_UNDERWATER) != 0;
            }

            bOwnerWater = (pLocal->pOwner->pInSector->flags & SITH_SECTOR_UNDERWATER) != 0;
            if ( (bPlayerWater && !bOwnerWater) || (!bPlayerWater && bOwnerWater) )
            {
                return 0;
            }

            if ( SITH_RANDF() < talkChance )
            {
                float hpFrac = pOwner->thingInfo.actorInfo.health / pOwner->thingInfo.actorInfo.maxHealth;
                if ( (pLocal->mode & SITHAI_MODE_ACTIVE) == 0 )
                {
                    hChannel = sithSoundClass_PlayModeRandom(pOwner, SITHSOUNDCLASS_IDLE);
                }
                else if ( (pLocal->mode & SITHAI_MODE_FLEEING) != 0 )
                {
                    if ( hpFrac > 0.5f )
                    {
                        hChannel = sithSoundClass_PlayModeRandom(pOwner, SITHSOUNDCLASS_GLOAT);
                    }
                    else
                    {
                        hChannel = sithSoundClass_PlayModeRandom(pOwner, SITHSOUNDCLASS_FEAR);
                    }
                }
                else if ( (pLocal->mode & SITHAI_MODE_UNKNOWN_100000) != 0 )
                {
                    if ( SITH_RANDF() < 0.6f )
                    {
                        hChannel = sithSoundClass_PlayModeFirst(pLocal->pOwner, SITHSOUNDCLASS_RESERVED8);
                    }
                    else if ( hpFrac > 0.25f )
                    {
                        hChannel = sithSoundClass_PlayModeRandom(pLocal->pOwner, SITHSOUNDCLASS_GLOAT);
                    }
                    else
                    {
                        hChannel = sithSoundClass_PlayModeRandom(pLocal->pOwner, SITHSOUNDCLASS_FEAR);
                    }
                }
                else if ( (pLocal->mode & SITHAI_MODE_TARGETVISIBLE) == 0 )
                {
                    hChannel = sithSoundClass_PlayModeRandom(pOwner, SITHSOUNDCLASS_SEARCH);
                }
                else if ( hpFrac < 0.25f )
                {
                    hChannel = sithSoundClass_PlayModeRandom(pOwner, SITHSOUNDCLASS_FEAR);
                }
                else
                {
                    hChannel = sithSoundClass_PlayModeRandom(pOwner, SITHSOUNDCLASS_BOAST);
                }
            }
            break;

        case SITHAI_EVENT_MODECHANGED:
            arg = (int)pObject;
            if ( (pLocal->mode & SITHAI_MODE_UNKNOWN_100000) != 0 && (arg & SITHAI_MODE_UNKNOWN_100000) == 0 )
            {
                sithSoundMixer_StopAllSoundsThing(pLocal->pOwner);
                hChannel = sithSoundClass_PlayModeFirst(pLocal->pOwner, SITHSOUNDCLASS_RESERVED8);
            }
            break;

        case SITHAI_EVENT_TOUCHED:
            if ( (pLocal->mode & SITHAI_MODE_SEARCHING) == 0 )
            {
                return 0;
            }

            if ( nextTalkTime > sithTime_g_secGameTime )
            {
                return 0;
            }

            SithThing* pCheckThing = (SithThing*)pObject;
            if ( !pCheckThing )
            {
                return 0;
            }

            if ( (pCheckThing->type == SITH_THING_PLAYER || pCheckThing->type == SITH_THING_ACTOR) && SITH_RANDF() < talkChance )
            {
                hChannel = sithSoundClass_PlayModeRandom(pLocal->pOwner, SITHSOUNDCLASS_ALERT);
            }
            break;

        case SITHAI_EVENT_TALK:
            if ( nextTalkTime > sithTime_g_secGameTime )
            {
                return 0;
            }

            arg = (int)pObject;
            hChannel = sithSoundClass_PlayModeRandom(pLocal->pOwner, (SithSoundClassMode)arg);
            bSuccess = 1;
            break;

        default:
            return 0;
    }

    if ( hChannel > 0 )
    {
        pState->aParams[0] = sithTime_g_secGameTime;
    }

    return bSuccess;
}

int J3DAPI sithAIInstinct_RandomTurn(SithAIControlBlock* pLocal, SithAIInstinct* pInstinct, SithAIInstinctState* pState, SithAIEventType event, void* pObject)
{
    J3D_UNUSED(event);
    J3D_UNUSED(pObject);

    if ( !pInstinct->intArg[0] )
    {
        pState->msecNextUpdate = sithTime_g_msecGameTime + 5000;
    }
    else
    {
        pState->msecNextUpdate = sithTime_g_msecGameTime + pInstinct->intArg[0];
    }

    if ( (pLocal->mode & SITHAI_MODE_SEARCHING) == 0 )
    {
        return 0;
    }

    rdVector3 randomPyr;
    memset(&randomPyr, 0, sizeof(randomPyr));
    randomPyr.yaw = SITH_RANDF() * 360.0f;
    rdVector3 dir;
    rdVector_Copy3(&dir, &rdroid_g_yVector3);
    rdVector_Rotate3Acc(&dir, &randomPyr);

    rdVector3 lookPos;
    rdVector_ScaleAdd3(&lookPos, &dir, pInstinct->fltArg[1], &pLocal->pOwner->pos);
    rdVector3 toTarget;
    float distToLook;
    int sightState = sithAIUtil_GetDistanceToTargetPos(pLocal->pOwner, &pLocal->pOwner->pos, &lookPos, -1.0f, pInstinct->fltArg[1], 0.0f, &toTarget, &distToLook);
    if ( !sightState )
    {
        sithAIMove_AISetLookPosEyeLevel(pLocal, &lookPos);

        // TODO: [BUG] The original has no return statement here and returns whatever eax holds after AISetLookPos:
        //       pLocal->pClass (submode & 1 early return), pLocal->mode | 0x80, or sithAI_EmitEvent's result when mode & 0x10000 (WANTALLEVENTS).
        //       All are nonzero (the EmitEvent case is approximated), so report the event as handled.
        return 1;
    }

    // TODO: [BUG] The original has no return statement here; the result of GetDistanceToTargetPos is left in eax
    return sightState;
}

int J3DAPI sithAIInstinct_OpenDoors(SithAIControlBlock* pLocal, SithAIInstinct* pInstinct, SithAIInstinctState* pState, SithAIEventType event, void* pObject)
{
    J3D_UNUSED(pInstinct);
    J3D_UNUSED(event);
    J3D_UNUSED(pObject);

    if ( (pLocal->mode & SITHAI_MODE_MOVING) == 0 )
    {
        return 0;
    }

    sithPlayerActions_Activate(pLocal->pOwner);
    pState->msecNextUpdate = sithTime_g_msecGameTime + 1000;
    return 0;
}

int J3DAPI sithAIInstinct_Jump(SithAIControlBlock* pLocal, SithAIInstinct* pInstinct, SithAIInstinctState* pState, SithAIEventType event, void* pObject)
{
    J3D_UNUSED(pObject);

    SITH_ASSERTREL(pLocal && pLocal->pOwner);
    SithThing* pThing = pLocal->pOwner;
    SithSector* pSect = pThing->pInSector;
    if ( !pThing->attach.flags )
    {
        return 0;
    }

    if ( (pLocal->mode & SITHAI_MODE_MOVING) == 0 )
    {
        return 0;
    }

    if ( pThing->thingInfo.actorInfo.bForceMovePlay )
    {
        return 0;
    }

    float curdot = rdVector_Dot3(&pThing->moveInfo.physics.velocity, &pLocal->vecUnknown0);
    if ( curdot > 0.02f )
    {
        return 0;
    }

    if ( (double)sithTime_g_msecGameTime < pState->aParams[0] )
    {
        return 0;
    }

    pState->aParams[0] = (float)((double)sithTime_g_msecGameTime + pInstinct->fltArg[0]);
    rdVector3 upPos;
    rdVector3 curjumpPos;
    switch ( event )
    {
        case SITHAI_EVENT_TOUCHED:
        case SITHAI_EVENT_HIT_WALL:
            rdVector_ScaleAdd3(&upPos, &rdroid_g_zVector3, pInstinct->fltArg[1], &pThing->pos);
            pSect = sithCollision_FindSectorInRadius(pSect, &pThing->pos, &upPos, 0.0f);
            if ( !pSect )
            {
                return 0;
            }

            float upZ = upPos.z;
            rdVector_ScaleAdd3(&curjumpPos, &pLocal->moveDirection, 0.1f, &upPos);
            curjumpPos.z = upZ;
            pSect = sithCollision_FindSectorInRadius(pSect, &upPos, &curjumpPos, 0.0f);
            if ( !pSect )
            {
                return 0;
            }

            int bNoCollision;
            if ( sithAIUtil_CheckFloorAtPos(pLocal, &curjumpPos, pSect, &bNoCollision) == 1 && bNoCollision )
            {
                sithAIMove_AIJump(pLocal, &curjumpPos, 1.0f);
            }
            break;

        case SITHAI_EVENT_HIT_CLIFF:
            rdVector_ScaleAdd3(&curjumpPos, &pLocal->moveDirection, pInstinct->fltArg[2], &pThing->pos);
            if ( sithAIUtil_CheckPosition(pLocal, &curjumpPos, NULL) )
            {
                rdVector_MultAcc3(&pThing->moveInfo.physics.velocity, &pLocal->moveDirection, 0.1f);
                sithAIMove_AIJump(pLocal, &pLocal->movePos, 1.0f);
            }
            break;

        default:
            return 0;
    }
    return 1;
}

int J3DAPI sithAIInstinct_LobFire(SithAIControlBlock* pLocal, SithAIInstinct* pInstinct, SithAIInstinctState* pState, SithAIEventType event, void* pObject)
{
    J3D_UNUSED(event);
    J3D_UNUSED(pObject);

    SithAIUtilFireFlags fireFlags = SITHAIUTIL_FIRE_LOB;
    SITH_ASSERTREL(pLocal);
    SithThing* pActor = pLocal->pOwner;
    SithThing* pTarget = pLocal->pTargetThing;
    SITH_ASSERTREL(pActor);
    if ( pActor->thingInfo.actorInfo.bForceMovePlay )
    {
        return 0;
    }

    if ( !sithAIUtil_CanAttackTarget(pTarget) )
    {
        if ( pTarget && (pTarget->flags & (SITH_TF_DYING | SITH_TF_DESTROYED)) != 0 && (pLocal->mode & SITHAI_MODE_TARGETVISIBLE) != 0 )
        {
            sithAIUtil_AIPlaySoundMode(pLocal, SITHSOUNDCLASS_VICTORY);
            sithPuppet_PlayMode(pLocal->pOwner, SITHPUPPETSUBMODE_VICTORY, NULL);
        }

        sithAIUtil_AISetMode(pLocal, SITHAI_MODE_SEARCHING);
        pLocal->pTargetThing = NULL;
        return 1;
    }

    if ( sithPuppet_IsAnyModeOnTrack(pLocal->pOwner, SITHPUPPETSUBMODE_FIRE, SITHPUPPETSUBMODE_FIRE4) )
    {
        pState->msecNextUpdate = sithTime_g_msecGameTime + 150;
        return 0;
    }

    int weaponNum = pInstinct->intArg[6];
    if ( SITH_RANDF() < pInstinct->fltArg[5] )
    {
        ++weaponNum;
    }

    sithAIUtil_SetWeaponFireFlags(weaponNum, &fireFlags);
    if ( sithAIUtil_AIFire(pLocal, pInstinct->fltArg[2], pInstinct->fltArg[3], pInstinct->fltArg[1], pInstinct->fltArg[4], weaponNum, fireFlags, 1) )
    {
        pState->msecNextUpdate = sithTime_g_msecGameTime + pInstinct->intArg[0];
        pLocal->msecFireWaitTime = sithTime_g_msecGameTime + pInstinct->intArg[0];
    }
    else
    {
        sithAIMove_AISetLookThing(pLocal, pTarget);
        pState->msecNextUpdate = sithTime_g_msecGameTime + 500;
    }
    return 0;
}

int J3DAPI sithAIInstinct_PrimaryFire(SithAIControlBlock* pLocal, SithAIInstinct* pInstinct, SithAIInstinctState* pState, SithAIEventType event, void* pObject)
{
    J3D_UNUSED(event);
    J3D_UNUSED(pObject);

    SithAIUtilFireFlags flags = SITHAIUTIL_FIRE_PRIMARY;
    SITH_ASSERTREL(pLocal);
    SithThing* pActor = pLocal->pOwner;
    SITH_ASSERTREL(pActor);

    if ( (pLocal->mode & SITHAI_MODE_FLEEING) != 0 )
    {
        return 0;
    }

    if ( pActor->thingInfo.actorInfo.bForceMovePlay )
    {
        return 0;
    }

    SithThing* pTarget = pLocal->pTargetThing;
    if ( !sithAIUtil_CanAttackTarget(pTarget) )
    {
        if ( pTarget && (pTarget->flags & (SITH_TF_DYING | SITH_TF_DESTROYED)) != 0 && (pLocal->mode & SITHAI_MODE_TARGETVISIBLE) != 0 )
        {
            sithAIUtil_AIPlaySoundMode(pLocal, SITHSOUNDCLASS_VICTORY);
            sithPuppet_PlayMode(pLocal->pOwner, SITHPUPPETSUBMODE_VICTORY, NULL);
        }

        sithAIUtil_AISetMode(pLocal, SITHAI_MODE_SEARCHING);
        pLocal->pTargetThing = NULL;
        return 1;
    }

    if ( sithPuppet_IsAnyModeOnTrack(pLocal->pOwner, SITHPUPPETSUBMODE_FIRE, SITHPUPPETSUBMODE_FIRE4) )
    {
        pState->msecNextUpdate = sithTime_g_msecGameTime + 250;
        return 0;
    }

    if ( pInstinct->fltArg[17] != 0.0f && rdVector_Dot3(&pLocal->toTarget, &pActor->orient.lvec) > pInstinct->fltArg[17] )
    {
        if ( pInstinct->fltArg[16] == 0.0f )
        {
            sithAIMove_AISetLookThing(pLocal, pLocal->pTargetThing);
        }
        return 0;
    }

    if ( pInstinct->fltArg[6] != 0.0f && SITH_RANDF() <= pInstinct->fltArg[6] )
    {
        flags |= SITHAIUTIL_FIRE_LEAD;
    }

    int weaponNum = pInstinct->intArg[11];
    if ( (pLocal->mode & SITHAI_MODE_MOVING) != 0 )
    {
        switch ( pInstinct->intArg[14] )
        {
            case 1:
                weaponNum = -1;
                break;

            case 2:
                sithAIUtil_sub_49B2E0(pLocal, 0);
                return 0;
        }
    }
    else if ( pInstinct->fltArg[10] != 0.0f )
    {
        float curFireDist = pInstinct->fltArg[15];
        if ( curFireDist != 0.0f )
        {
            sithAIUtil_sub_49B2E0(pLocal, 0);
            if ( curFireDist < pLocal->distance )
            {
                weaponNum = pInstinct->intArg[11] + 1;
            }
        }
        else
        {
            weaponNum = pInstinct->intArg[11] + 1;
        }
    }

    if ( weaponNum == pInstinct->intArg[11] && pInstinct->fltArg[7] != 0.0f && SITH_RANDF() <= pInstinct->fltArg[7] )
    {
        weaponNum = pInstinct->intArg[11] + 1;
    }

    sithAIUtil_SetWeaponFireFlags(weaponNum, &flags);

    int curBurstNum = pInstinct->intArg[8];
    if ( !curBurstNum )
    {
        curBurstNum = 1;
    }

    if ( (pLocal->mode & SITHAI_MODE_UNKNOWN_100000) != 0 )
    {
        curBurstNum *= rand() % (6 - abs(curBurstNum)) + abs(curBurstNum);
    }
    else if ( curBurstNum < 0 )
    {
        curBurstNum = rand() % abs(curBurstNum) + 1;
    }

    float closestDist = pInstinct->fltArg[4];
    float farDist = pInstinct->fltArg[2];
    if ( sithWeapon_HasWeaponSelected(pActor) )
    {
        farDist = sithWeapon_GetWeaponMaxAimDistance(pActor->thingInfo.actorInfo.weaponInfo.curWeaponID);
    }

    if ( (pLocal->mode & SITHAI_MODE_UNKNOWN_100000) == 0 && !sithAIUtil_sub_49F1F0(pLocal, &pLocal->pOwner->pos) )
    {
        // No clear line of fire, skip the shot
    }
    else if ( sithAIUtil_AIFire(pLocal, closestDist, farDist, pInstinct->fltArg[1], pInstinct->fltArg[3], weaponNum, flags, curBurstNum) )
    {
        if ( pInstinct->fltArg[16] == 0.0f && !sithWeapon_IsAiming(pLocal->pOwner) )
        {
            sithAIMove_AISetLookThing(pLocal, pTarget);
        }

        float fireDelay = (SITH_RANDF() * 0.4f - 0.2f + 1.0f) * pInstinct->fltArg[0];
        pState->msecNextUpdate = sithTime_g_msecGameTime + (uint32_t)fireDelay;
        return 0;
    }

    if ( pInstinct->fltArg[16] == 0.0f && (pLocal->mode & SITHAI_MODE_MOVING) == 0 )
    {
        SithWeaponId curWeaponID = pLocal->pOwner->thingInfo.actorInfo.weaponInfo.curWeaponID;
        switch ( pLocal->targetSightState )
        {
            case 2:
                sithAIMove_AISetLookThing(pLocal, pLocal->pTargetThing);
                break;

            case 0:
                if ( pLocal->pTargetThing->moveInfo.physics.velocity.x != 0.0f
                  || pLocal->pTargetThing->moveInfo.physics.velocity.y != 0.0f
                  || pLocal->pTargetThing->moveInfo.physics.velocity.z != 0.0f )
                {
                    rdVector3 lookPosition;
                    rdVector_ScaleAdd3(&lookPosition, &pLocal->pTargetThing->moveInfo.physics.velocity, 0.5f, &pLocal->pTargetThing->pos);
                    sithAIMove_AISetLookPosEyeLevel(pLocal, &lookPosition);
                }
                else if ( (unsigned int)curWeaponID >= SITHWEAPON_COMFISTS && (unsigned int)curWeaponID <= SITHWEAPON_COMSHOTGUN )
                {
                    sithAIMove_AISetLookThing(pLocal, pLocal->pTargetThing);
                }
                break;
        }
    }

    pState->msecNextUpdate = sithTime_g_msecGameTime + 750;
    return 0;
}

int J3DAPI sithAIInstinct_AlternateFire(SithAIControlBlock* pLocal, SithAIInstinct* pInstinct, SithAIInstinctState* pState, SithAIEventType event, void* pObject)
{
    J3D_UNUSED(event);
    J3D_UNUSED(pObject);

    SithAIUtilFireFlags flags = SITHAIUTIL_FIRE_ALT;
    SITH_ASSERTREL(pLocal);

    SithThing* pTarget = pLocal->pTargetThing;
    if ( !sithAIUtil_CanAttackTarget(pTarget) )
    {
        return 1;
    }

    if ( (pLocal->mode & SITHAI_MODE_ATTACKING) == 0 )
    {
        pState->msecNextUpdate = sithTime_g_msecGameTime + pInstinct->intArg[1];
        return 0;
    }

    if ( sithPuppet_IsAnyModeOnTrack(pLocal->pOwner, SITHPUPPETSUBMODE_FIRE, SITHPUPPETSUBMODE_FIRE4) )
    {
        pState->msecNextUpdate = sithTime_g_msecGameTime + 150;
        return 0;
    }

    int weaponNum = pInstinct->intArg[0];
    if ( weaponNum < 0 )
    {
        weaponNum = rand() % (abs(weaponNum) + 1);
    }

    sithAIUtil_SetWeaponFireFlags(weaponNum, &flags);

    if ( pInstinct->fltArg[7] != 0.0f && SITH_RANDF() <= pInstinct->fltArg[7] )
    {
        flags |= SITHAIUTIL_FIRE_LEAD;
    }

    int burstCount = pInstinct->intArg[8];
    if ( !burstCount )
    {
        burstCount = 1;
    }

    if ( (pLocal->mode & SITHAI_MODE_UNKNOWN_100000) != 0 )
    {
        burstCount *= rand() % 8 + 3;
    }
    else if ( burstCount < 0 )
    {
        burstCount = rand() % abs(burstCount) + 1;
    }

    if ( pInstinct->intArg[9] )
    {
        flags |= SITHAIUTIL_FIRE_STOP_ON_FIRE;
    }

    if ( sithAIUtil_AIFire(pLocal, pInstinct->fltArg[4], pInstinct->fltArg[5], pInstinct->fltArg[3], pInstinct->fltArg[6], weaponNum, flags, burstCount) )
    {
        float currentfireDelay = (SITH_RANDF() * 0.4f - 0.2f + 1.0f) * pInstinct->fltArg[2];
        pState->msecNextUpdate = sithTime_g_msecGameTime + (uint32_t)currentfireDelay;
    }
    else
    {
        if ( pLocal->targetSightState == 2 )
        {
            sithAIMove_AISetLookThing(pLocal, pTarget);
        }
        else if ( pLocal->targetSightState == 0
               && pTarget->moveType == SITH_MT_PHYSICS
               && (pTarget->moveInfo.physics.velocity.x != 0.0f || pTarget->moveInfo.physics.velocity.y != 0.0f || pTarget->moveInfo.physics.velocity.z != 0.0f) )
        {
            rdVector3 lookPos;
            rdVector_ScaleAdd3(&lookPos, &pTarget->moveInfo.physics.velocity, 0.5f, &pTarget->pos);
            sithAIMove_AISetLookPosEyeLevel(pLocal, &lookPos);
        }

        pState->msecNextUpdate = sithTime_g_msecGameTime + pInstinct->intArg[1];
    }

    return 0;
}

int J3DAPI sithAIInstinct_CircleStrafe(SithAIControlBlock* pLocal, SithAIInstinct* pInstinct, SithAIInstinctState* pState, SithAIEventType event, SithThing* pThing)
{
    float rate = 1.25f;
    int result = 0;
    SITH_ASSERTREL(pLocal);

    pState->msecNextUpdate = sithTime_g_msecGameTime + pInstinct->intArg[0];
    if ( (pLocal->mode & (SITHAI_MODE_UNKNOWN_1000000 | SITHAI_MODE_FLEEING | SITHAI_MODE_SEARCHING)) != 0 )
    {
        pLocal->mode &= ~SITHAI_MODE_CIRCLESTRAFING;
        return 0;
    }

    if ( (pLocal->mode & SITHAI_MODE_ATTACKING) == 0 )
    {
        pLocal->mode &= ~SITHAI_MODE_CIRCLESTRAFING;
        return 0;
    }

    if ( !sithAIUtil_CanAttackTarget(pLocal->goalThing) )
    {
        sithAIUtil_AISetMode(pLocal, SITHAI_MODE_SEARCHING);
        pLocal->goalThing = NULL;
        return 0;
    }

    if ( pInstinct->fltArg[7] != 0.0f && sithPuppet_IsAnyModeOnTrack(pLocal->pOwner, SITHPUPPETSUBMODE_FIRE, SITHPUPPETSUBMODE_FIRE4) )
    {
        pState->msecNextUpdate = sithTime_g_msecGameTime + 500;
        return 0;
    }

    if ( pInstinct->fltArg[8] != 0.0f )
    {
        rate = pInstinct->fltArg[8];
    }

    switch ( event )
    {
        case SITHAI_EVENT_GOAL_REACHED:
            result = 1;
            break;

        case (SithAIEventType)0:
            break;

        case SITHAI_EVENT_TOUCHED:
            if ( (pLocal->mode & SITHAI_MODE_CIRCLESTRAFING) != 0 )
            {
                SithThing* pTouched = pThing;
                if ( pTouched && pTouched->type != SITH_THING_PLAYER )
                {
                    goto stop;
                }
            }

            return 0;

        case SITHAI_EVENT_HIT_WALL:
        case SITHAI_EVENT_HIT_CLIFF:
        case SITHAI_EVENT_HIT_THING:
        case SITHAI_EVENT_GOAL_UNREACHABLE:
            if ( (pLocal->mode & SITHAI_MODE_CIRCLESTRAFING) != 0 )
            {
                goto stop;
            }

            return 0;

        default:
            return 0;
    }

    if ( (pLocal->mode & SITHAI_MODE_CIRCLESTRAFING) != 0 && !pInstinct->intArg[6] )
    {
        sithAIMove_AISetLookThing(pLocal, pLocal->goalThing);
    }

    int bRandom = pInstinct->intArg[4];
    sithAIUtil_sub_49B640(pLocal);
    if ( pInstinct->fltArg[2] < pLocal->targetDistance || pLocal->targetSightState2 )
    {
        goto stop;
    }

    rdVector3 strafeDir;
    rdVector_Scale3(&strafeDir, &pLocal->vecUnknown0, -pLocal->targetDistance);
    if ( !bRandom && pLocal->goalThing->orient.lvec.x * strafeDir.x + pLocal->goalThing->orient.lvec.y * strafeDir.y + pLocal->goalThing->orient.lvec.z * strafeDir.z < 0.0f )
    {
        goto stop;
    }

    if ( pState->aParams[1]++ > 4.0f )
    {
        goto stop;
    }

    if ( pState->aParams[0] == 0.0f || bRandom )
    {
        if ( SITH_RANDF() < 0.5f )
        {
            pState->aParams[0] = -1.0f;
        }
        else
        {
            pState->aParams[0] = 1.0f;
        }
    }

    float side = pState->aParams[0];
    rdVector3 pyr;
    memset(&pyr, 0, sizeof(pyr));
    if ( !bRandom )
    {
        pyr.y = pInstinct->fltArg[1] * side;
    }
    else
    {
        pyr.y = (SITH_RANDF() + 0.5f) * side * pInstinct->fltArg[1];
    }

    {
        rdVector3 toGoal;

        rdVector_Sub3(&toGoal, &pLocal->goalThing->pos, &pLocal->pOwner->pos);
        rdVector_Normalize3Acc(&toGoal);
        rdVector_Rotate3(&strafeDir, &toGoal, &pyr);
    }

    rdVector3 nextPos;
    rdVector_ScaleAdd3(&nextPos, &strafeDir, pLocal->pOwner->collide.movesize * 3.0f, &pLocal->pOwner->pos);

    if ( pInstinct->fltArg[5] != 0.0f )
    {
        rdVector3 pushDir;
        float pushDist;

        rdVector_Sub3(&pushDir, &pLocal->pOwner->pos, &nextPos);
        pushDist = rdVector_Normalize3Acc(&pushDir);
        if ( pushDist < pInstinct->fltArg[5] )
        {
            pushDist = pInstinct->fltArg[5];
            rdVector_ScaleAdd3(&nextPos, &pushDir, pushDist, &nextPos);
        }
    }

    float range;
    rdVector3 dirToTarget;
    if ( !sithAIUtil_GetDistanceToTargetPos(pLocal->goalThing, &pLocal->goalThing->pos, &nextPos, -1.0f, pLocal->pClass->sightDistance, 0.0f, &dirToTarget, &range) )
    {
        pLocal->mode |= SITHAI_MODE_CIRCLESTRAFING;
        sithAIMove_AISetMovePos(pLocal, &nextPos, rate);
        if ( !pInstinct->intArg[6] )
        {
            sithAIMove_AISetLookThing(pLocal, pLocal->goalThing);
        }
        else
        {
            sithAIMove_AISetLookPosEyeLevel(pLocal, &nextPos);
        }

        pState->msecNextUpdate = sithTime_g_msecGameTime + pInstinct->intArg[3];
    }
    else
    {
        pState->aParams[0] = -side;
        pState->aParams[1] = 0.0f;
        pLocal->mode &= ~SITHAI_MODE_CIRCLESTRAFING;
    }

    return result;

stop:
    sithAIMove_AISetLookThing(pLocal, pLocal->goalThing);
    pLocal->mode &= ~SITHAI_MODE_CIRCLESTRAFING;
    pState->aParams[0] = 0.0f;
    pState->aParams[1] = 0.0f;
    return result;
}

int J3DAPI sithAIInstinct_Dodge(SithAIControlBlock* pLocal, SithAIInstinct* pInstinct, SithAIInstinctState* pState, SithAIEventType event, SithThing* pThing)
{
    float moveSpeed = 1.5f;
    float nextCheck = -1.0f;
    float yaw = 0.0f;
    int bEvaded = 0;

    SITH_ASSERTREL(pLocal && pLocal->pOwner);

    if ( !event )
    {
        return 0;
    }

    if ( (event & (SITHAI_EVENT_GOAL_REACHED | SITHAI_EVENT_GOAL_UNREACHABLE)) != 0 && (pLocal->mode & SITHAI_MODE_UNKNOWN_10) != 0 )
    {
        goto goal_reached;
    }

    if ( (pLocal->mode & (SITHAI_MODE_UNKNOWN_10 | SITHAI_MODE_FLEEING | SITHAI_MODE_UNKNOWN_100000 | SITHAI_MODE_UNKNOWN_1000000)) != 0 )
    {
        return 0;
    }

    float minDodgeDist = pInstinct->fltArg[0] + 0.03f;
    float maxStep = pInstinct->fltArg[1] + 0.03f;
    float moveDist = SITH_RANDF() * (maxStep - minDodgeDist) + minDodgeDist;
    float minTargetLen = pInstinct->fltArg[4];
    float minDistVal = pInstinct->fltArg[10];

    if ( pInstinct->fltArg[2] != 0.0f )
    {
        moveSpeed = pInstinct->fltArg[2];
    }

    if ( pInstinct->fltArg[3] != 0.0f )
    {
        nextCheck = pState->aParams[0] + pInstinct->fltArg[3];
    }

    rdVector3 toDest;
    int moveFlags;
    float targetGap;
    SithThing* pFirer;
    SithThing* pSource;
    if ( event == SITHAI_EVENT_TARGETED )
    {
        if ( (pLocal->mode & SITHAI_MODE_SEARCHING) != 0 )
        {
            return 0;
        }

        pSource = pThing;
        if ( !pSource )
        {
            return 0;
        }

        if ( (pSource->thingInfo.weaponInfo.damageType & 0x810) != 0 )
        {
            return 0;
        }

        if ( (pSource->thingInfo.weaponInfo.flags & SITH_WF_AITARGETNODODGE) == 0 )
        {
            if ( nextCheck > sithTime_g_secGameTime )
            {
                return 0;
            }

            if ( SITH_RANDF() >= pInstinct->fltArg[5] )
            {
                return 0;
            }
        }

        bEvaded = 1;
        pFirer = sithThing_GetThingParent(pSource);
        if ( sithAIUtil_GetDistanceToTarget(pLocal->pOwner, &pLocal->pOwner->pos, pFirer, pLocal->pClass->fov, pLocal->pClass->sightDistance, 0.0f, &toDest, &targetGap) )
        {
            return 0;
        }

        if ( targetGap < minTargetLen )
        {
            return 0;
        }

        if ( pInstinct->fltArg[8] != 0.0f && SITH_RANDF() < pInstinct->fltArg[8] && sithAIMove_AISpecialMove(pLocal, SITHACTORSPECIALMOVE_DIR_RANDOM | SITHACTORSPECIALMOVE_ROLL) )
        {
            goto dodged;
        }

        moveFlags = 0x8001;
    }
    else if ( event == SITHAI_EVENT_HIT_SECTOR )
    {
        if ( nextCheck > sithTime_g_secGameTime )
        {
            return 0;
        }

        if ( SITH_RANDF() >= pInstinct->fltArg[6] )
        {
            return 0;
        }

        pSource = pThing;
        if ( !pSource )
        {
            return 0;
        }

        if ( pSource->type != SITH_THING_WEAPON || pSource->moveType != SITH_MT_PHYSICS )
        {
            return 0;
        }

        pFirer = sithThing_GetThingParent(pSource);
        if ( pFirer == pLocal->pOwner )
        {
            return 0;
        }

        if ( pFirer->controlType == SITH_CT_AI && pFirer->controlInfo.aiControl.pLocal )
        {
            rdVector_Sub3(&toDest, &pFirer->controlInfo.aiControl.pLocal->pTargetThing->pos, &pFirer->pos);
            rdVector_Normalize3Acc(&toDest);
            pLocal->pFleeFromThing = pFirer;
            moveFlags = 0x9001;
            moveDist = 1.0f;
            goto make_path;
        }

        if ( sithAIUtil_GetDistanceToTarget(pLocal->pOwner, &pLocal->pOwner->pos, pFirer, pLocal->pClass->fov, pLocal->pClass->sightDistance, 0.0f, &toDest, &targetGap) )
        {
            return 0;
        }

        if ( targetGap < minTargetLen )
        {
            return 0;
        }

        yaw = 60.0f;
        moveFlags = 0x8002;
    }
    else if ( event == SITHAI_EVENT_SOUND )
    {
        SithAIAwarenessSector* pCurSector = &sithAIAwareness_g_aSectors[sithSector_GetSectorIndex(pLocal->pOwner->pInSector)];
        if ( pCurSector->aLevelAtTransmittingPos[2] == 0.0f )
        {
            return 0;
        }

        pSource = pCurSector->aTransmittingThing[2];
        if ( !pSource )
        {
            return 0;
        }

        if ( pSource->moveType != SITH_MT_PHYSICS )
        {
            return 0;
        }

        if ( pSource->type == SITH_THING_WEAPON && (pSource->thingInfo.weaponInfo.damageType & 0x10) != 0 )
        {
            return 0;
        }

        pFirer = sithThing_GetThingParent(pSource);
        if ( pFirer == pLocal->pOwner )
        {
            return 0;
        }

        if ( (pFirer->moveInfo.physics.flags & (SITH_PF_MINECAR | SITH_PF_RAFT | SITH_PF_JEEP | SITH_PF_UNKNOWN_8000000)) == 0 )
        {
            if ( sithAIUtil_GetDistanceToTarget(pLocal->pOwner, &pLocal->pOwner->pos, pFirer, pLocal->pClass->fov, 1.0f, 0.0f, &toDest, &targetGap) )
            {
                return 0;
            }
        }
        else
        {
            float tmpVal = sqrtf(rdVector_Dot3(&pFirer->moveInfo.physics.velocity, &pFirer->moveInfo.physics.velocity)) * 2.0f;
            tmpVal = J3DMIN(pLocal->pClass->sightDistance, tmpVal);
            tmpVal = J3DMAX(tmpVal, 1.0f);
            if ( sithAIUtil_GetDistanceToTarget(pLocal->pOwner, &pLocal->pOwner->pos, pFirer, -1.0f, tmpVal, 0.0f, &toDest, &targetGap) )
            {
                return 0;
            }
        }

        bEvaded = 1;
        if ( (pFirer->moveInfo.physics.flags & (SITH_PF_MINECAR | SITH_PF_RAFT | SITH_PF_JEEP | SITH_PF_UNKNOWN_8000000)) != 0 )
        {
            float chance;
            rdVector3 heading;

            sithAIUtil_GetXYHeadingVector(pSource, &heading);
            if ( rdVector_Dot3(&toDest, &heading) > -0.5f )
            {
                return 0;
            }

            moveDist = 0.5f;
            moveFlags = 0x8001;
            chance = pInstinct->fltArg[9];
            if ( chance == 0.0f )
            {
                chance = 0.98f;
            }

            if ( sqrtf(rdVector_Dot3(&pFirer->moveInfo.physics.velocity, &pFirer->moveInfo.physics.velocity)) > 1.4f )
            {
                chance -= 0.2f;
            }

            if ( SITH_RANDF() < chance )
            {
                moveFlags |= 0x1000;
            }

            pLocal->pFleeFromThing = pFirer;
            goto make_path;
        }

        if ( nextCheck > sithTime_g_secGameTime )
        {
            return 0;
        }

        if ( SITH_RANDF() >= pInstinct->fltArg[7] )
        {
            return 0;
        }

        sithAIUtil_AIPlaySoundMode(pLocal, SITHSOUNDCLASS_FEAR);
        pLocal->pFleeFromThing = pFirer;
        yaw = 45.0f;
        moveFlags = 0xC002;
    }
    else
    {
        return 0;
    }

make_path:
    pLocal->pFleeFromThing = NULL;
    rdVector3 targetPos;
    if ( sithAIUtil_MakePathPos(pLocal, &pLocal->pOwner->pos, minDistVal, moveDist, yaw, moveFlags, &toDest, &targetPos) )
    {
        sithAIMove_AISetLookPosEyeLevel(pLocal, &targetPos);
        sithAIMove_AISetMoveTargetPos(pLocal, &targetPos, moveSpeed);
    }
    else
    {
        return 0;
    }

dodged:
    pLocal->mode |= SITHAI_MODE_UNKNOWN_10;
    sithAIUtil_AIClearMode(pLocal, SITHAI_MODE_TRAVERSEWPNTS);
    pState->aParams[0] = sithTime_g_secGameTime;
    return bEvaded;

goal_reached:
    pLocal->mode &= ~SITHAI_MODE_UNKNOWN_10;
    if ( pLocal->pTargetThing )
    {
        sithAIMove_AISetLookThing(pLocal, pLocal->pTargetThing);
    }

    return 0;
}

int J3DAPI sithAIInstinct_HitAndRun(SithAIControlBlock* pLocal, SithAIInstinct* pInstinct, SithAIInstinctState* pState, SithAIEventType event, void* pObject)
{
    J3D_UNUSED(pObject);

    switch ( event )
    {
        case (SithAIEventType)0:
            if ( pState->aParams[1] == 1.0f )
            {
                if ( sithPuppet_IsAnyModeOnTrack(pLocal->pOwner, SITHPUPPETSUBMODE_FIRE, SITHPUPPETSUBMODE_FIRE4) )
                {
                    pLocal->mode &= ~SITHAI_MODE_ATTACKING;
                    pState->msecNextUpdate = sithTime_g_msecGameTime + 100;
                    return 0;
                }

                sithAIUtil_AISetMode(pLocal, SITHAI_MODE_FLEEING);
                pLocal->pFleeFromThing = pLocal->pTargetThing;
                goto flee;
            }
            break;

        case SITHAI_EVENT_FIRE:
            pState->aParams[3] = 1.0f;
            if ( SITH_RANDF() < pInstinct->fltArg[2] )
            {
                pState->aParams[1] = 1.0f;
                pState->msecNextUpdate = sithTime_g_msecGameTime + 500;
            }
            return 0;

        default:
            return 0;
    }

    if ( (pLocal->mode & SITHAI_MODE_FLEEING) != 0 )
    {
        if ( pState->aParams[2] == 1.0f )
        {
            sithAIUtil_AISetMode(pLocal, SITHAI_MODE_ATTACKING);
            pState->aParams[2] = 0.0f;
        }

        return 0;
    }

    if ( (pLocal->mode & SITHAI_MODE_TARGETVISIBLE) != 0 && pState->aParams[3] != 0.0f && sithAIUtil_CanAttackTarget(pLocal->pTargetThing) )
    {
        float time = pState->aParams[0];
        time += 1000.0f;
        if ( time > pInstinct->fltArg[0] )
        {
            sithAIUtil_AISetMode(pLocal, SITHAI_MODE_FLEEING);
            pLocal->pFleeFromThing = pLocal->pTargetThing;
            goto flee;
        }

        pState->aParams[0] = time;
        pState->msecNextUpdate = sithTime_g_msecGameTime + 1000;
    }
    else
    {
        pState->aParams[0] = 0.0f;
        pState->msecNextUpdate = sithTime_g_msecGameTime + 1000;
    }

    return 0;

flee:
    pState->aParams[2] = 1.0f;
    pState->aParams[0] = 0.0f;
    pState->aParams[1] = 0.0f;
    pState->aParams[3] = 0.0f;
    pState->msecNextUpdate = sithTime_g_msecGameTime + (int)pInstinct->fltArg[1];
    return 0;
}

int J3DAPI sithAIInstinct_Retreat(SithAIControlBlock* pLocal, SithAIInstinct* pInstinct, SithAIInstinctState* pState, SithAIEventType event, void* pObject)
{
    J3D_UNUSED(pObject);

    if ( (pLocal->mode & SITHAI_MODE_ATTACKING) == 0 )
    {
        return 0;
    }

    if ( !sithAIUtil_CanAttackTarget(pLocal->pTargetThing) )
    {
        pState->msecNextUpdate = sithTime_g_msecGameTime + pInstinct->intArg[0];
        return 0;
    }

    if ( (pLocal->mode & SITHAI_MODE_UNKNOWN_100000) != 0 )
    {
        pState->flags |= SITHAI_INSTINCT_DISABLED;
        return 0;
    }

    float healthRatio = pLocal->pOwner->thingInfo.actorInfo.health / pLocal->pOwner->thingInfo.actorInfo.maxHealth;
    if ( pInstinct->fltArg[3] != 0.0f && pState->aParams[0] > pInstinct->fltArg[3] )
    {
        return 0;
    }

    if ( healthRatio < pInstinct->fltArg[1] )
    {
        switch ( event )
        {
            case (SithAIEventType)0:
            case SITHAI_EVENT_HIT_SECTOR:
                if ( SITH_RANDF() < pInstinct->fltArg[2] )
                {
                    pState->aParams[0] = pState->aParams[0] + 1.0f;
                    sithAIUtil_AIPlaySoundMode(pLocal, SITHSOUNDCLASS_FLEE);
                    sithAIUtil_AISetMode(pLocal, SITHAI_MODE_FLEEING);
                    pLocal->pFleeFromThing = pLocal->pTargetThing;
                }
                else if ( SITH_RANDF() < 0.1f )
                {
                    sithAIUtil_AIPlaySoundMode(pLocal, SITHSOUNDCLASS_FEAR);
                }
                break;
        }
    }

    pState->msecNextUpdate = sithTime_g_msecGameTime + pInstinct->intArg[0];
    return 0;
}

int J3DAPI sithAIInstinct_RandomMove(SithAIControlBlock* pLocal, SithAIInstinct* pInstinct, SithAIInstinctState* pState, SithAIEventType event, void* pObject)
{
    J3D_UNUSED(pObject);

    if ( (pLocal->mode & SITHAI_MODE_ATTACKING) == 0 )
    {
        return 0;
    }

    if ( !sithAIUtil_CanAttackTarget(pLocal->pTargetThing) )
    {
        return 0;
    }

    if ( (pLocal->mode & (SITHAI_MODE_SEARCHING | SITHAI_MODE_FLEEING | SITHAI_MODE_CIRCLESTRAFING)) != 0 )
    {
        return 0;
    }

    if ( (pLocal->mode & SITHAI_MODE_UNKNOWN_100000) != 0 )
    {
        return 0;
    }

    if ( pInstinct->intArg[6] && sithPuppet_IsAnyModeOnTrack(pLocal->pOwner, SITHPUPPETSUBMODE_FIRE, SITHPUPPETSUBMODE_FIRE4) )
    {
        return 0;
    }

    sithAIUtil_sub_49B2E0(pLocal, 0);

    rdVector3 direction;
    int delay;
    switch ( event )
    {
        case SITHAI_EVENT_FIRE:
            if ( !pInstinct->intArg[7] )
            {
                return 0;
            }
            break;

        case (SithAIEventType)0:
            delay = pInstinct->intArg[0];
            delay += (int)((float)(uint32_t)delay * (SITH_RANDF() - 0.5f));
            pState->msecNextUpdate = sithTime_g_msecGameTime + delay;
            if ( pLocal->targetSightState )
            {
                return 0;
            }
            break;

        case SITHAI_EVENT_GOAL_REACHED:
            if ( !pLocal->targetSightState && pLocal->pTargetThing )
            {
                sithAIMove_AISetLookThing(pLocal, pLocal->pTargetThing);
            }
            else if ( pState->aParams[3] != 0.0f )
            {
                rdVector_Set3(&direction, pState->aParams[0], pState->aParams[1], pState->aParams[2]);
                sithAIMove_AISetLookPosEyeLevel(pLocal, &direction);
            }

            pState->aParams[3] = 0.0f;
            return 0;

        default:
            return 0;
    }

    float minDist = pInstinct->fltArg[1] + 0.03f;
    float maxLen = pInstinct->fltArg[2] + 0.03f;
    float moveDist = SITH_RANDF() * (maxLen - minDist) + minDist;
    float lowSpeed = pInstinct->fltArg[3];
    float maxSpeed = pInstinct->fltArg[4];
    float moveSpeed = SITH_RANDF() * (maxSpeed - lowSpeed) + lowSpeed;

    rdVector_Neg3(&direction, &pLocal->toTarget);

    int pathOpts = 0x808002;
    if ( pInstinct->intArg[9] )
    {
        pathOpts |= 4;
    }

    float angleDeg = pInstinct->fltArg[5];
    if ( angleDeg < 0.0f )
    {
        pathOpts |= 0x4000;
    }

    rdVector3 goalPos;
    if ( sithAIUtil_MakePathPos(pLocal, &pLocal->pOwner->pos, minDist, moveDist, angleDeg, pathOpts, &direction, &goalPos) )
    {
        if ( pLocal->pTargetThing )
        {
            pState->aParams[0] = pLocal->pTargetThing->pos.x;
            pState->aParams[1] = pLocal->pTargetThing->pos.y;
            pState->aParams[2] = pLocal->pTargetThing->pos.z;
            pState->aParams[3] = 1.0f;
        }
        else
        {
            pState->aParams[3] = 0.0f;
        }

        if ( pInstinct->intArg[8] || moveDist >= (maxLen - minDist) / 2.0f + minDist )
        {
            sithAIMove_AISetLookPosEyeLevel(pLocal, &goalPos);
        }

        sithAIMove_AISetMoveTargetPos(pLocal, &goalPos, moveSpeed);
    }

    return 0;
}

int J3DAPI sithAIInstinct_HumanCombatMove(SithAIControlBlock* pLocal, SithAIInstinct* pInstinct, SithAIInstinctState* pState, SithAIEventType event, void* pObject)
{
    SithThing* pOwner = pLocal->pOwner;
    int bAimAtDest = 1;
    int bCheckPath = 0;

    int bTargetVisible;
    float enemyRange;
    rdVector3 dirNorm;
    rdVector3 targetPos;
    float minDist;
    float maxRange;
    float minSpeed;
    float fastSpeed;
    unsigned int triedMoves;
    unsigned int curMoveType;
    float curMoveSpeed;
    float angle;
    int opts;

    SITH_ASSERTREL(pLocal && pLocal->pOwner);

    if ( !sithAIUtil_CanAttackTarget(pLocal->pTargetThing) )
    {
        return 0;
    }

    if ( (pLocal->mode & SITHAI_MODE_ATTACKING) == 0 )
    {
        goto clear;
    }

    if ( (pLocal->mode & (SITHAI_MODE_SEARCHING | SITHAI_MODE_FLEEING | SITHAI_MODE_CIRCLESTRAFING | SITHAI_MODE_UNKNOWN_100000)) != 0 )
    {
        goto clear;
    }

    sithAIUtil_sub_49B2E0(pLocal, 0);
    float healthPct = pOwner->thingInfo.actorInfo.health / pOwner->thingInfo.actorInfo.maxHealth;

    switch ( event )
    {
        case SITHAI_EVENT_MODECHANGED:
            if ( (pLocal->mode & SITHAI_MODE_CHASE_GOAL) != 0 && ((SithAIMode)pObject & SITHAI_MODE_CHASE_GOAL) == 0 )
            {
                if ( (pLocal->mode & SITHAI_MODE_UNKNOWN_1000000) != 0 )
                {
                    return 0;
                }

                sithAIMove_StopAIMovement(pLocal);
                bTargetVisible = sithAIUtil_GetDistanceToTargetPos(pLocal->pOwner, &pLocal->pOwner->pos, &pLocal->vecUnknown5, -1.0f, pLocal->pClass->sightDistance, 0.0f, &dirNorm, &enemyRange);
                if ( healthPct <= pInstinct->fltArg[6] && SITH_RANDF() < pInstinct->fltArg[9] && bTargetVisible )
                {
                    sithAIMove_AISetLookPosEyeLevel(pLocal, &pLocal->vecUnknown);
                    sithAIMove_AISetMovePos(pLocal, &pLocal->vecUnknown, pInstinct->fltArg[5]);
                    pLocal->mode |= SITHAI_MODE_UNKNOWN_1000000;
                    pLocal->mode &= ~SITHAI_MODE_CHASE_GOAL;
                    pState->aParams[3] = 3.0f;
                    return 1;
                }

                if ( bTargetVisible )
                {
                    sithAIMove_AISetLookPosEyeLevel(pLocal, &pLocal->vecUnknown);
                    sithAIMove_AISetMovePos(pLocal, &pLocal->vecUnknown, pInstinct->fltArg[5]);
                    return 1;
                }
            }
            break;

        case SITHAI_EVENT_HIT_SECTOR:
            if ( healthPct <= pInstinct->fltArg[6] )
            {
                if ( SITH_RANDF() < pInstinct->fltArg[7] )
                {
                    sithAIUtil_AIPlaySoundMode(pLocal, SITHSOUNDCLASS_FLEE);
                    sithAIUtil_AISetMode(pLocal, SITHAI_MODE_FLEEING);
                    pLocal->pFleeFromThing = pLocal->pTargetThing;
                    return 0;
                }

                if ( SITH_RANDF() < pInstinct->fltArg[8] && sithAI_HasInstinct(pLocal, "circlestrafe") )
                {
                    sithAI_EnableInstinct(pLocal, "circlestrafe", 1);
                    pLocal->submode |= 0x400000;
                }

                if ( SITH_RANDF() < pInstinct->fltArg[10] )
                {
                    sithAIMove_StopAIMovement(pLocal);
                    pLocal->mode |= SITHAI_MODE_UNKNOWN_100000;
                    sithAI_ForceInstinctUpdate(pLocal, "primaryfire", 0);
                    sithAI_ForceInstinctUpdate(pLocal, "lobfire", 0);
                    sithAI_ForceInstinctUpdate(pLocal, "alternatefire", 0);
                }
            }
            break;

        case (SithAIEventType)0:
            if ( pLocal->targetSightState != 0 )
            {
                return 0;
            }

            if ( sithAIUtil_sub_49F1F0(pLocal, &pLocal->pOwner->pos) )
            {
                bCheckPath = 1;
                if ( (pLocal->pTargetThing->moveInfo.physics.flags & (SITH_PF_MINECAR | SITH_PF_RAFT | SITH_PF_JEEP | SITH_PF_UNKNOWN_8000000)) == 0 && sithAIUtil_IsThingMoving(pLocal->pTargetThing) )
                {
                    pState->aParams[1] += 0.2f;
                }

                if ( pState->aParams[0] > (float)sithTime_g_msecGameTime )
                {
                    return 0;
                }

                float arc = pInstinct->fltArg[0];
                arc = (SITH_RANDF() - 0.5f) * arc + arc;
                pState->aParams[0] = sithTime_g_msecGameTime + arc;
                if ( SITH_RANDF() > pState->aParams[1] )
                {
                    pState->aParams[1] += 0.05f;
                    return 0;
                }
            }

            minDist = pInstinct->fltArg[2] + 0.03f;
            maxRange = pInstinct->fltArg[3] + 0.03f;
            enemyRange = SITH_RANDF() * (maxRange - minDist) + minDist;
            minSpeed = pInstinct->fltArg[4];
            fastSpeed = pInstinct->fltArg[5];
            triedMoves = (unsigned int)pState->aParams[2];
            if ( triedMoves )
            {
                if ( (triedMoves & 1) == 0 )
                {
                    goto move_toward;
                }

                if ( (triedMoves & 2) == 0 )
                {
                    goto move_back;
                }

                if ( (triedMoves & 4) == 0 )
                {
                    goto move_side;
                }

                pState->aParams[2] = 0.0f;
                return 0;
            }

            float rndVal = SITH_RANDF();
            if ( rndVal >= 0.0f && rndVal <= 0.4f )
            {
                goto move_toward;
            }

            if ( rndVal >= 0.5f && rndVal <= 0.8f )
            {
                goto move_back;
            }

            goto move_side;

        move_toward:
            curMoveType = 1;
            curMoveSpeed = fastSpeed;
            angle = 0.0f;
            opts = 0x8001;
            rdVector_Copy3(&dirNorm, &pLocal->toTarget);
            goto make_path;

        move_back:
            curMoveType = 2;
            if ( !pInstinct->intArg[1] && pLocal->distance > pOwner->collide.movesize * 4.0f )
            {
                return 1;
            }

            bAimAtDest = 0;
            curMoveSpeed = minSpeed;
            angle = 30.0f;
            opts = 0xC002;
            rdVector_Neg3(&dirNorm, &pLocal->toTarget);
            goto make_path;

        move_side:
            curMoveType = 4;
            if ( !pInstinct->intArg[1] )
            {
                return 1;
            }

            curMoveSpeed = SITH_RANDF() * (fastSpeed - minSpeed) + minSpeed;
            angle = 30.0f;
            opts = 0xC002;
            rdVector_Copy3(&dirNorm, &pLocal->toTarget);

        make_path:
            if ( sithAIUtil_MakePathPos(pLocal, &pLocal->pOwner->pos, minDist, enemyRange, angle, opts, &dirNorm, &targetPos) )
            {
                if ( !bCheckPath || sithAIUtil_sub_49F1F0(pLocal, &targetPos) )
                {
                    pState->aParams[1] = 0.0f;
                    pState->aParams[2] = 0.0f;
                    if ( bAimAtDest )
                    {
                        sithAIMove_AISetLookPosEyeLevel(pLocal, &targetPos);
                    }
                    else
                    {
                        rdVector_Sub3(&dirNorm, &pOwner->pos, &targetPos);
                        rdVector_Add3(&dirNorm, &pOwner->pos, &dirNorm);
                        sithAIMove_AISetLookPosEyeLevel(pLocal, &dirNorm);
                    }

                    sithAIMove_AISetMoveTargetPos(pLocal, &targetPos, curMoveSpeed);
                    return 0;
                }
            }

            triedMoves |= curMoveType;
            pState->aParams[2] = (float)triedMoves;
            break;

        case SITHAI_EVENT_GOAL_REACHED:
            if ( (pLocal->mode & SITHAI_MODE_UNKNOWN_1000000) != 0 )
            {
                if ( pState->aParams[3] == 3.0f )
                {
                    if ( pLocal->targetSightState != 0 )
                    {
                        pLocal->mode &= ~SITHAI_MODE_UNKNOWN_1000000;
                        return 0;
                    }

                    sithAIMove_AISetLookPosEyeLevel(pLocal, &pLocal->vecUnknown122);
                    sithAIUtil_AIPauseMove(pLocal, rand() % 1001 + 2000);
                    pState->aParams[3] = 2.0f;
                }
                else if ( pState->aParams[3] == 2.0f )
                {
                    if ( pLocal->targetSightState == 0 )
                    {
                        pLocal->mode &= ~SITHAI_MODE_UNKNOWN_1000000;
                        return 0;
                    }

                    sithAIUtil_AIPauseMove(pLocal, rand() % 2001 + 1000);
                    pState->aParams[3] = 3.0f;
                }

                rdVector_Sub3(&targetPos, &pLocal->vecUnknown122, &pLocal->pOwner->pos);
                rdVector_Normalize3Acc(&targetPos);
                rdVector_Sub3(&dirNorm, &pLocal->vecUnknown, &pLocal->pOwner->pos);
                rdVector_Normalize3Acc(&dirNorm);

                pOwner->thingInfo.actorInfo.stateChange.type = SITHACTORSTATECHANGE_ANIMMOVE;
                angle = rdMath_DeltaAngleNormalized(&dirNorm, &targetPos, &pOwner->orient.uvec);
                if ( angle < 0.0f )
                {
                    pOwner->thingInfo.actorInfo.stateChange.params.moveFlags = SITHACTORSPECIALMOVE_DIR_LEFT | SITHACTORSPECIALMOVE_ROLL;
                }
                else
                {
                    pOwner->thingInfo.actorInfo.stateChange.params.moveFlags = SITHACTORSPECIALMOVE_DIR_RIGHT | SITHACTORSPECIALMOVE_ROLL;
                }
                return 1;
            }

            if ( (pLocal->mode & SITHAI_MODE_CHASE_GOAL) != 0 )
            {
                bTargetVisible = sithAIUtil_GetDistanceToTargetPos(pLocal->pOwner, &pLocal->pOwner->pos, &pLocal->vecUnknown5, -1.0f, pLocal->pClass->sightDistance, 0.0f, &dirNorm, &enemyRange);
                if ( bTargetVisible )
                {
                    sithAIMove_AISetMovePos(pLocal, &pLocal->vecUnknown5, 1.7f);
                    sithAIMove_AISetLookPosEyeLevel(pLocal, &pLocal->vecUnknown5);
                    return 1;
                }
            }
            else if ( pLocal->pTargetThing )
            {
                sithAIMove_AISetLookThing(pLocal, pLocal->pTargetThing);
            }

            goto clear;

        case SITHAI_EVENT_GOAL_UNREACHABLE:
            goto clear;
    }

    return 0;

clear:
    pState->aParams[1] = 0.0f;
    pState->aParams[2] = 0.0f;
    return 0;
}

int J3DAPI sithAIInstinct_Roam(SithAIControlBlock* pLocal, SithAIInstinct* pInstinct, SithAIInstinctState* pState, SithAIEventType event, SithThing* pThing)
{
    float moveSpeed = 0.75f;
    int bEnabled = 0;

    if ( (pLocal->mode & (SITHAI_MODE_ATTACKING | SITHAI_MODE_UNKNOWN_10 | SITHAI_MODE_ACTIVE | SITHAI_MODE_FLEEING)) != 0 )
    {
        return 0;
    }

    if ( event == SITHAI_EVENT_MODECHANGED && pThing )
    {
        SithAIMode mode = (SithAIMode)pThing;
        if ( (pLocal->mode & SITHAI_MODE_SEARCHING) != 0 && (mode & SITHAI_MODE_SEARCHING) == 0 )
        {
            pState->msecNextUpdate = sithTime_g_msecGameTime + pInstinct->intArg[0];
            return 0;
        }
    }

    if ( (pLocal->mode & SITHAI_MODE_TRAVERSEWPNTS) != 0 )
    {
        return 0;
    }

    float closestDist = pLocal->pOwner->collide.movesize + 0.03f;
    float homeRadius = pInstinct->fltArg[1];
    float roamDist = SITH_RANDF() * (fabsf(homeRadius) - closestDist) + closestDist;

    if ( pInstinct->fltArg[3] != 0.0f )
    {
        moveSpeed = pInstinct->fltArg[3];
    }

    rdVector3 startPos;
    if ( homeRadius > 0.0f )
    {
        rdVector_Copy3(&startPos, &pLocal->homePos);
    }
    else
    {
        rdVector_Copy3(&startPos, &pLocal->pOwner->pos);
    }

    if ( pInstinct->fltArg[2] != 0.0f )
    {
        bEnabled = 1;
    }

    rdVector3 movePos;
    int pathOpts;
    switch ( event )
    {
        case SITHAI_EVENT_TOUCHED:
        case SITHAI_EVENT_HIT_WALL:
        case SITHAI_EVENT_HIT_CLIFF:
            pathOpts = 0x20C002;
            rdVector3 direction;
            sithAIUtil_GetXYHeadingVector(pLocal->pOwner, &direction);
            rdVector_Neg3Acc(&direction);
            if ( sithAIUtil_MakePathPos(pLocal, &pLocal->pOwner->pos, closestDist, roamDist, 45.0f, pathOpts, &direction, &movePos) )
            {
                sithAIMove_StopAIMovement(pLocal);
                if ( sithAIMove_AISetMovePos(pLocal, &movePos, moveSpeed) )
                {
                    sithAIMove_AISetLookPosEyeLevel(pLocal, &movePos);
                    return 1;
                }
            }
            break;

        case SITHAI_EVENT_GOAL_REACHED:
            if ( (pLocal->submode & 0x20200) == 0 )
            {
                return 0;
            }
            // fallthrough

        case (SithAIEventType)0:
            pState->msecNextUpdate = sithTime_g_msecGameTime + pInstinct->intArg[0];
            if ( (pLocal->mode & SITHAI_MODE_INSTINCTUSEWPNTS) != 0 && sithAIUtil_AIMoveToNextWpnt(pLocal, moveSpeed, 0.0f, 1) )
            {
                return 0;
            }

            if ( homeRadius > 0.0f
                && sqrtf((pLocal->pOwner->pos.x - pLocal->homePos.x) * (pLocal->pOwner->pos.x - pLocal->homePos.x) + (pLocal->pOwner->pos.y - pLocal->homePos.y) * (pLocal->pOwner->pos.y - pLocal->homePos.y) + (pLocal->pOwner->pos.z - pLocal->homePos.z) * (pLocal->pOwner->pos.z - pLocal->homePos.z)) > homeRadius )
            {
                sithAIMove_AISetLookPosEyeLevel(pLocal, &pLocal->homePos);
                sithAIMove_AISetMovePos(pLocal, &pLocal->homePos, moveSpeed);
                return 0;
            }
            break;

        default:
            return 0;
    }

    if ( sithAIUtil_sub_49F010(pLocal, &startPos, closestDist, roamDist, bEnabled, 2, &movePos) && sithAIMove_AISetMovePos(pLocal, &movePos, moveSpeed) )
    {
        sithAIMove_AISetLookPosEyeLevel(pLocal, &movePos);
        return 1;
    }

    if ( sithAIMove_AISetMovePos(pLocal, &pLocal->homePos, moveSpeed) )
    {
        sithAIMove_AISetLookPosEyeLevel(pLocal, &pLocal->homePos);
        return 1;
    }

    return 0;
}

int J3DAPI sithAIInstinct_ReturnHome(SithAIControlBlock* pLocal, SithAIInstinct* pInstinct, SithAIInstinctState* pState, SithAIEventType event, void* pObject)
{
    J3D_UNUSED(pInstinct);
    J3D_UNUSED(pState);

    SithAIMode prevMode;
    switch ( event )
    {
        case SITHAI_EVENT_MODECHANGED:
            prevMode = (SithAIMode)pObject;
            if ( (pLocal->mode & SITHAI_MODE_SEARCHING) != 0 && (prevMode & SITHAI_MODE_SEARCHING) == 0 )
            {
                sithAIMove_AISetMovePos(pLocal, &pLocal->homePos, 1.2f);
                sithAIMove_AISetLookPosEyeLevel(pLocal, &pLocal->homePos);
            }
            break;

        case SITHAI_EVENT_GOAL_REACHED:
            if ( (pLocal->mode & SITHAI_MODE_SEARCHING) != 0 )
            {
                rdVector3 lookPos;
                rdVector_ScaleAdd3(&lookPos, &pLocal->homeOrient, 1.0f, &pLocal->pOwner->pos);
                sithAIMove_AISetLookPosEyeLevel(pLocal, &lookPos);
            }
            break;
    }

    return 0;
}

int J3DAPI sithAIInstinct_Flee(SithAIControlBlock* pLocal, SithAIInstinct* pInstinct, SithAIInstinctState* pState, SithAIEventType event, SithThing* pThing)
{
    float panicTime = pInstinct->fltArg[2];
    float fleeRange = pInstinct->fltArg[0];
    float minStep = pLocal->pOwner->collide.movesize + 0.03f;
    float fleeingSpeed = 1.5f;
    int bPanic = 0;

    if ( (pLocal->mode & SITHAI_MODE_FLEEING) == 0 )
    {
        if ( pState->aParams[0] > 0.0f )
        {
            goto stop;
        }

        return 0;
    }

    pLocal->goalThing    = NULL;
    pLocal->pTargetThing = NULL;
    if ( pState->aParams[0] == 0.0f )
    {
        pState->aParams[0] = sithTime_g_secGameTime;
        bPanic = 1;
    }

    if ( panicTime == 0.0f )
    {
        panicTime = 10.0f;
        if ( (pLocal->mode & SITHAI_MODE_FLEEINGTOWAYPOINT) != 0 )
        {
            panicTime += 15.1f;
        }
    }

    if ( !pLocal->pFleeFromThing )
    {
        goto stop;
    }

    if ( sithTime_g_secGameTime > pState->aParams[0] + panicTime )
    {
        goto stop;
    }

    if ( pInstinct->fltArg[4] != 0.0f )
    {
        fleeingSpeed = pInstinct->fltArg[4];
    }

    if ( pInstinct->intArg[1] )
    {
        pState->msecNextUpdate = sithTime_g_msecGameTime + pInstinct->intArg[1];
    }
    else
    {
        pState->msecNextUpdate = sithTime_g_msecGameTime + 5000;
    }

    rdVector3 direction;
    float threatRange;
    if ( !sithAIUtil_GetDistanceToTarget(pLocal->pOwner, &pLocal->pOwner->pos, pLocal->pFleeFromThing, -1.0f, fleeRange, 0.0f, &direction, &threatRange) || bPanic )
    {
        SithThing* pTouched;
        switch ( event )
        {
            case SITHAI_EVENT_MODECHANGED:
                if ( pThing )
                {
                    SithAIMode mode = (SithAIMode)pThing;
                    if ( (pLocal->mode & SITHAI_MODE_FLEEING) != 0 && (mode & SITHAI_MODE_FLEEING) == 0 )
                    {
                        rdVector_Neg3Acc(&direction);
                        goto flee;
                    }
                }

                return 0;

            case (SithAIEventType)0:
            case SITHAI_EVENT_GOAL_REACHED:
                rdVector_Neg3Acc(&direction);
                goto flee;

            case SITHAI_EVENT_TOUCHED:
                pTouched = pThing;
                if ( pTouched && pTouched->type == SITH_THING_PLAYER )
                {
                    return 1;
                }
                break;
        }

        sithAIUtil_GetXYHeadingVector(pLocal->pOwner, &direction);
        pLocal->mode &= ~SITHAI_MODE_TRAVERSEWPNTS;
        goto flee;
    }
    else if ( (pLocal->submode & 0x8000) != 0 )
    {
        return 0;
    }

stop:
    if ( pLocal->pFleeFromThing )
    {
        sithAIMove_AISetLookPosEyeLevel(pLocal, &pLocal->pFleeFromThing->pos);
    }

    sithAIMove_StopAIMovement(pLocal);
    sithAIUtil_AISetMode(pLocal, SITHAI_MODE_SEARCHING);
    pState->aParams[0] = 0.0f;
    pLocal->pFleeFromThing  = NULL;
    return 0;

flee:
    if ( (pLocal->mode & SITHAI_MODE_INSTINCTUSEWPNTS) != 0 )
    {
        if ( (pLocal->mode & SITHAI_MODE_TRAVERSEWPNTS) != 0 )
        {
            return 1;
        }

        if ( sithAIUtil_AIMoveToNextWpnt(pLocal, fleeingSpeed, 0.0f, 0) )
        {
            return 1;
        }
    }

    sithAIMove_StopAIMovement(pLocal);
    int pathOpts = 0x21002;
    if ( pInstinct->fltArg[5] > 0.0f )
    {
        minStep = pInstinct->fltArg[5];
    }

    float angle = pInstinct->fltArg[3];
    if ( angle != 0.0f )
    {
        pathOpts |= 0x8000;
        if ( angle < 0.0f )
        {
            pathOpts |= 0x4000;
        }
    }

    rdVector3 endPos;
    if ( sithAIUtil_MakePathPos(pLocal, &pLocal->pOwner->pos, minStep, fleeRange, angle, pathOpts, &direction, &endPos) && sithAIMove_AISetMovePos(pLocal, &endPos, fleeingSpeed) )
    {
        sithAIMove_AISetLookPosEyeLevel(pLocal, &endPos);
        return 1;
    }

    pathOpts = 0x29001;
    if ( sithAIUtil_MakePathPos(pLocal, &pLocal->pOwner->pos, minStep, fleeRange, angle, pathOpts, &direction, &endPos) && sithAIMove_AISetMovePos(pLocal, &endPos, fleeingSpeed) )
    {
        sithAIMove_AISetLookPosEyeLevel(pLocal, &endPos);
        return 1;
    }

    goto stop;
}

int J3DAPI sithAIInstinct_BasicFallow(SithAIControlBlock* pLocal, SithAIInstinct* pInstinct, SithAIInstinctState* pState, SithAIEventType event, void* pObject)
{
    int pathCheck = 0;
    float moveSpeed = 1.0f;
    float length = 0.0f;

    SITH_ASSERTREL(pLocal && pLocal->pClass && pLocal->pOwner);
    SithThing* pFoe = pLocal->pOwner;

    if ( (pLocal->mode & (SITHAI_MODE_UNKNOWN_1000000 | SITHAI_MODE_FLEEING | SITHAI_MODE_UNKNOWN_10)) != 0 )
    {
        return 0;
    }

    if ( (pLocal->mode & SITHAI_MODE_CIRCLESTRAFING) != 0 )
    {
        if ( (pLocal->mode & SITHAI_MODE_MOVING) != 0 )
        {
            return 0;
        }

        pLocal->mode &= ~SITHAI_MODE_CIRCLESTRAFING;
    }

    if ( (pLocal->mode & SITHAI_MODE_TRAVERSEWPNTS) != 0 )
    {
        return 0;
    }

    float gap;
    rdVector3 movePos;
    rdVector3 heading = { 0 };
    if ( (event & (SITHAI_EVENT_HIT_THING | SITHAI_EVENT_HIT_WALL | SITHAI_EVENT_TOUCHED)) != 0 )
    {
        if ( (pLocal->mode & SITHAI_MODE_SEARCHING) != 0 )
        {
            return 0;
        }

        if ( (pLocal->mode & (SITHAI_MODE_ACTIVE | SITHAI_MODE_MOVING)) == 0 )
        {
            return 0;
        }

        if ( (pLocal->submode & SITHAI_SUBMODE_UNKNOWN_4) != 0 )
        {
            return 1;
        }

        sithAIUtil_GetXYHeadingVector(pFoe, &heading);
        if ( pState->aParams[0] > 3.0f )
        {
            pState->aParams[0] = 0.0f;
            pState->aParams[1] = 0.0f;
            rdVector_Copy3(&pLocal->vecUnknown3, &pLocal->movePos);
            rdVector_Neg3Acc(&heading);
            gap = pFoe->collide.movesize * 4.0f;
            rdVector_ScaleAdd3(&movePos, &heading, gap, &pFoe->pos);
            if ( sithAIUtil_sub_49EE50(pLocal, &movePos, 0.0f, 60.0f, 30.0f, 0x100) )
            {
                pLocal->submode |= SITHAI_SUBMODE_UNKNOWN_8;
                return 1;
            }
            return 0;
        }
    }

    if ( (event & (SITHAI_EVENT_HIT_THING | SITHAI_EVENT_HIT_CLIFF | SITHAI_EVENT_HIT_WALL | SITHAI_EVENT_TOUCHED)) != 0 )
    {
        pState->aParams[1] = 1.0f;
    }

    if ( pInstinct->intArg[5] && sithPuppet_IsAnyModeOnTrack(pFoe, SITHPUPPETSUBMODE_FIRE, SITHPUPPETSUBMODE_FIRE4) )
    {
        pState->msecNextUpdate = sithTime_g_msecGameTime + 500;
        return 0;
    }

    if ( !pLocal->goalThing )
    {
        return 0;
    }

    SithThing* pGoal = pLocal->goalThing;
    sithAIUtil_sub_49B640(pLocal);
    SithSurface* pSurface;
    rdVector3 toDir;
    switch ( event )
    {
        case SITHAI_EVENT_HIT_CLIFF:
            return 0;

        case SITHAI_EVENT_TOUCHED:
        case SITHAI_EVENT_HIT_THING:
        {
            SithThing* pHitThing;

            pHitThing = (SithThing*)pObject;
            if ( pHitThing )
            {
                if ( pHitThing->type == SITH_THING_PLAYER )
                {
                    return 0;
                }

                rdVector_Sub3(&toDir, &pFoe->pos, &pHitThing->pos);
                toDir.z = 0.0f;
                rdVector_Normalize3Acc(&toDir);
                if ( rdVector_Dot3(&heading, &toDir) > 0.0f )
                {
                    return 1;
                }
            }

            pState->aParams[0] += 1.0f;
            pLocal->submode |= SITHAI_SUBMODE_UNKNOWN_4;
            if ( sithAIUtil_sub_49EE50(pLocal, &pLocal->movePos, 45.0f, 135.0f, 45.0f, 0x100) )
            {
                return 1;
            }
            return 0;
        }

        case SITHAI_EVENT_HIT_WALL:
            pSurface = (SithSurface*)pObject;
            if ( !pSurface )
            {
                return 0;
            }

            rdVector_Copy3(&toDir, &pSurface->face.normal);
            toDir.z = 0.0f;
            toDir.x = stdMath_ClipNearZero(toDir.x);
            toDir.y = stdMath_ClipNearZero(toDir.y);
            toDir.z = stdMath_ClipNearZero(toDir.z);
            rdVector_Normalize3Acc(&toDir);

            if ( !pLocal->targetSightState2 )
            {
                float goalDot;
                float wallDot;

                rdVector3 targetVec;
                targetVec.x = pLocal->vecUnknown0.x;
                targetVec.y = pLocal->vecUnknown0.y;
                targetVec.z = 0.0f;
                targetVec.x = stdMath_ClipNearZero(targetVec.x);
                targetVec.y = stdMath_ClipNearZero(targetVec.y);
                targetVec.z = stdMath_ClipNearZero(targetVec.z);
                rdVector_Normalize3Acc(&targetVec);

                goalDot = heading.x * targetVec.x + heading.y * targetVec.y;
                wallDot = heading.x * toDir.x + heading.y * toDir.y;
                if ( goalDot > 0.701f && wallDot > -0.966f )
                {
                    return 1;
                }
            }

            if ( pLocal->pClass->rank < 0.5f && SITH_RANDF() > pLocal->pClass->rank )
            {
                return 0;
            }

            pState->aParams[0] += 1.0f;
            pLocal->submode |= SITHAI_SUBMODE_UNKNOWN_4;
            float pathDistance;
            if ( !pLocal->targetSightState2 )
            {
                pathDistance = pFoe->collide.movesize;
                gap = pFoe->collide.movesize * 2.0f;
                pathCheck = sithAIUtil_CheckPathToPos(pLocal, pFoe->pInSector, &pFoe->pos, 0x2120, pathDistance, gap, 0.0f, &toDir, &movePos);
                if ( (pathCheck & 2) == 0 )
                {
                    sithAIMove_AISetLookPosEyeLevel(pLocal, &movePos);
                    sithAIMove_AISetMoveTargetPos(pLocal, &movePos, 1.0f);
                    return 1;
                }
            }

            pathDistance = J3DMAX(pFoe->collide.movesize, 0.089999996f);
            gap = SITH_RANDF() * 0.1f + pathDistance;
            int pathMask = 0x8102;
            if ( sithAIUtil_MakePathPos(pLocal, &pFoe->pos, pathDistance, gap, 45.0f, pathMask, &toDir, &movePos) )
            {
                sithAIMove_StopAIMovement(pLocal);
                sithAIMove_AISetLookPosEyeLevel(pLocal, &movePos);
                sithAIMove_AISetMoveTargetPos(pLocal, &movePos, moveSpeed);
                return 1;
            }
            return 0;

        case (SithAIEventType)0:
            if ( pInstinct->intArg[0] )
            {
                pState->msecNextUpdate = sithTime_g_msecGameTime + pInstinct->intArg[0];
            }
            break;

        case SITHAI_EVENT_GOAL_UNREACHABLE:
            return 0;

        case SITHAI_EVENT_GOAL_REACHED:
            pState->aParams[0] = 0.0f;
            if ( pState->aParams[1] == 1.0f )
            {
                pState->aParams[1] = 0.0f;
                sithAIMove_AISetLookPosEyeLevel(pLocal, &pGoal->pos);
            }

            if ( (pLocal->mode & SITHAI_MODE_SEARCHING) != 0 )
            {
                return 0;
            }

            if ( (pLocal->submode & (SITHAI_SUBMODE_SEMICONTINUOUSMOTION | SITHAI_SUBMODE_CONTINUOUSMOTION)) == 0 )
            {
                return 0;
            }
            break;

        default:
            return 0;
    }

    pLocal->submode &= ~SITHAI_SUBMODE_UNKNOWN_2000000;
    if ( (pFoe->thingInfo.actorInfo.flags & SITH_AF_HUMAN) == 0 && (pGoal->moveInfo.physics.flags & (SITH_PF_MINECAR | SITH_PF_RAFT | SITH_PF_JEEP | SITH_PF_UNKNOWN_8000000)) != 0 )
    {
        return 0;
    }

    float minDist;
    float idealDistance;
    float reach;
    if ( (pLocal->mode & SITHAI_MODE_UNKNOWN_100000) != 0 )
    {
        moveSpeed = 2.0f;
        minDist   = pInstinct->fltArg[1];
        reach   = J3DMIN(pFoe->collide.movesize * 3.0f, minDist);
        idealDistance = 0.0f;
    }
    else
    {
        minDist   = pInstinct->fltArg[1];
        reach   = pInstinct->fltArg[2];
        idealDistance = pInstinct->fltArg[3];
        if ( sithWeapon_HasWeaponSelected(pFoe) )
        {
            reach   = sithWeapon_GetWeaponMaxAimDistance(pFoe->thingInfo.actorInfo.weaponInfo.curWeaponID);
            reach  -= 0.1f;
            idealDistance = 0.0f;
        }

        if ( (pLocal->submode & SITHAI_SUBMODE_UNKNOWN_400000) != 0 )
        {
            minDist   = (float)(minDist * 0.5);
            reach   = (float)(reach * 0.5);
            idealDistance = (float)(idealDistance * 0.5);
        }
    }

    if ( (pLocal->submode & SITHAI_SUBMODE_USEMATCHVELOCITY) != 0
        && (pLocal->goalThing->moveInfo.physics.velocity.x != 0.0f || pLocal->goalThing->moveInfo.physics.velocity.y != 0.0f || pLocal->goalThing->moveInfo.physics.velocity.z != 0.0f) )
    {
        if ( pLocal->targetDistance >= 0.0f && pLocal->targetDistance <= pFoe->collide.movesize * 2.0f )
        {
            pLocal->mode |= SITHAI_MODE_UNKNOWN_8000;
        }
        else
        {
            pLocal->mode &= ~SITHAI_MODE_UNKNOWN_8000;
        }
    }
    else
    {
        pLocal->mode &= ~SITHAI_MODE_UNKNOWN_8000;
    }

    if ( !pLocal->targetSightState2 || pLocal->targetSightState2 == 2 )
    {
        pLocal->mode &= ~SITHAI_MODE_CHASE_GOAL;
        if ( (pLocal->submode & SITHAI_SUBMODE_CONTINUOUSWPNTMOTION) == 0 )
        {
            sithAIUtil_AIClearMode(pLocal, SITHAI_MODE_TRAVERSEWPNTS);
        }

        sithAIMove_AISetLookPosEyeLevel(pLocal, &pGoal->pos);
        if ( pLocal->targetDistance > reach )
        {
            length = pLocal->targetDistance - reach;
        }
        else if ( pLocal->targetDistance < minDist )
        {
            if ( idealDistance != 0.0f )
            {
                length = pLocal->targetDistance - idealDistance;
            }
            else
            {
                length = pLocal->targetDistance - minDist;
            }
        }

        if ( length != 0.0f )
        {
            if ( length < 0.0f )
            {
                pLocal->submode |= SITHAI_SUBMODE_UNKNOWN_2000000;
            }

            rdVector_ScaleAdd3(&movePos, &pLocal->vecUnknown0, length, &pFoe->pos);
            if ( (pFoe->moveInfo.physics.flags & SITH_PF_FLY) != 0 )
            {
                movePos.z = pGoal->pos.z + pFoe->collide.movesize;
            }
            else if ( (pFoe->flags & SITH_TF_SUBMERGED) != 0 )
            {
                movePos.z = pGoal->pos.z;
            }
        }
        else
        {
            pLocal->submode |= SITHAI_SUBMODE_UNKNOWN_2000000;
            if ( (pFoe->moveInfo.physics.flags & SITH_PF_FLY) == 0 )
            {
                return 0;
            }

            length = pFoe->collide.movesize / 2.0f + pGoal->pos.z;
            if ( fabsf(pFoe->pos.z - length) <= 0.05f )
            {
                return 0;
            }

            movePos.x = pFoe->pos.x;
            movePos.y = pFoe->pos.y;
            movePos.z = length;
        }

        int losCheck = sithAIUtil_GetDistanceToTargetPos(pGoal, &pGoal->pos, &movePos, -1.0f, pLocal->pClass->sightDistance, 0.0f, &toDir, &gap);
        if ( pInstinct->fltArg[4] != 0.0f || !losCheck )
        {
            sithAIMove_AISetMovePos(pLocal, &movePos, moveSpeed);
            return 1;
        }

        if ( (pLocal->mode & SITHAI_MODE_WANTALLEVENTS) != 0 )
        {
            sithAI_EmitEvent(pLocal, SITHAI_EVENT_GOAL_UNREACHABLE, NULL);
        }
    }
    else if ( (pLocal->mode & SITHAI_MODE_CHASE_GOAL) == 0 )
    {
        if ( (pGoal->type == SITH_THING_PLAYER || pGoal->type == SITH_THING_ACTOR) && (pGoal->thingInfo.actorInfo.flags & SITH_AF_INVISIBLE) != 0 )
        {
            return 0;
        }

        if ( (pFoe->thingInfo.actorInfo.flags & SITH_AF_BLIND) != 0 )
        {
            return 0;
        }

        pLocal->mode |= SITHAI_MODE_CHASE_GOAL;
        return 1;
    }

    return 0;
}

int J3DAPI sithAIInstinct_WallCrawl(SithAIControlBlock* pLocal, SithAIInstinct* pInstinct, SithAIInstinctState* pState, SithAIEventType event, SithSurface* pSurface)
{
    int curresult = 0;
    SITH_ASSERTREL(pLocal && pLocal->pClass && pLocal->pOwner);
    SithThing* pThing = pLocal->pOwner;

    if ( (event & (SITHAI_EVENT_HIT_WALL | SITHAI_EVENT_HIT_FLOOR)) != 0 && (pLocal->mode & (SITHAI_MODE_ACTIVE | SITHAI_MODE_MOVING)) == 0 )
    {
        return 0;
    }

    float size = pThing->collide.movesize + 0.03f;
    rdVector3 movePos;
    if ( (pLocal->mode & (SITHAI_MODE_ACTIVE | SITHAI_MODE_ATTACKING)) != 0 )
    {
        sithAIUtil_sub_49B640(pLocal);
        sithAIUtil_AIGetMovePos(pLocal, 0x100, &movePos);
    }
    else
    {
        sithAIUtil_AIGetMovePos(pLocal, 0x400, &movePos);
    }

    float heightDiff = movePos.z - pThing->pos.z;
    SithSurface* pSurf;
    switch ( event )
    {
        case SITHAI_EVENT_HIT_CLIFF:
            if ( (pLocal->mode & SITHAI_MODE_WALLCRAWLING) != 0 )
            {
                sithAIMove_Unreachable(pLocal);
                return 1;
            }

            return 0;

        case SITHAI_EVENT_TOUCHED:
        case SITHAI_EVENT_HIT_THING:
            if ( (pLocal->mode & SITHAI_MODE_WALLCRAWLING) != 0 )
            {
                return 1;
            }
            break;

        case SITHAI_EVENT_HIT_FLOOR:
        case SITHAI_EVENT_LAND_FLOOR:
            pSurf = pSurface;
            if ( !pSurf )
            {
                break;
            }

            if ( (pLocal->mode & SITHAI_MODE_UNKNOWN_400000) != 0 )
            {
                pLocal->mode &= ~SITHAI_MODE_UNKNOWN_400000;
                return 1;
            }

            if ( (pLocal->mode & SITHAI_MODE_WALLCRAWLING) != 0 )
            {
                sithAIUtil_AIClearMode(pLocal, SITHAI_MODE_WALLCRAWLING);
                goto crawl;
            }
            break;

        case SITHAI_EVENT_HIT_WALL:
            if ( (pLocal->submode & (SITHAI_SUBMODE_WALLCRAWLLOCKED | SITHAI_SUBMODE_UNKNOWN_4)) != 0 )
            {
                return 1;
            }

            pSurf = pSurface;
            if ( !pSurf )
            {
                break;
            }

            if ( (pLocal->mode & SITHAI_MODE_WALLCRAWLING) != 0 )
            {
                goto crawl;
            }

            if ( heightDiff > size )
            {
                sithAIUtil_AISetMode(pLocal, SITHAI_MODE_WALLCRAWLING);
                goto crawl;
            }
            break;

        case (SithAIEventType)0:
            if ( (pLocal->mode & SITHAI_MODE_WALLCRAWLING) != 0 )
            {
                if ( (pThing->attach.flags & SITH_ATTACH_SURFACE) != 0 && (pThing->attach.attachedToStructure.pSurfaceAttached->flags & SITH_SURFACE_ISFLOOR) != 0 )
                {
                    sithAIUtil_AIClearMode(pLocal, SITHAI_MODE_WALLCRAWLING);
                }

                pState->msecNextUpdate = sithTime_g_msecGameTime + 500;
                return 1;
            }
            break;

        default:
            return 0;
    }

    if ( (pLocal->mode & (SITHAI_MODE_UNKNOWN_1000000 | SITHAI_MODE_CIRCLESTRAFING | SITHAI_MODE_FLEEING | SITHAI_MODE_UNKNOWN_10 | SITHAI_MODE_SEARCHING)) != 0 )
    {
        return 0;
    }

    curresult = sithAIInstinct_BasicFallow(pLocal, pInstinct, pState, event, pSurface);
    return curresult;

crawl:
    if ( (pLocal->submode & SITHAI_SUBMODE_UNKNOWN_4) == 0 && pSurf )
    {
        float dot = rdVector_Dot3(&pThing->orient.lvec, &pSurf->face.normal);
        if ( dot >= 0.0f )
        {
            return 1;
        }

        pLocal->submode |= SITHAI_SUBMODE_UNKNOWN_4;
        rdVector_Copy3(&pLocal->vecUnknown3, &pSurf->face.normal);
        if ( sithAIMove_AISpecialMove(pLocal, SITHACTORSPECIALMOVE_MOUNTWALL) )
        {
            return 1;
        }

        sithThing_AttachThingToSurface(pThing, pSurf, 1);
    }

    return 1;
}

int J3DAPI sithAIInstinct_SnakeMungeTestCheck(SithAIControlBlock* pLocal, rdVector3* pDirection, float distance, rdVector3* pOutPoint)
{
    SithThing* pThing = pLocal->pOwner;
    if ( !sithThing_CreateQuetzUserBlock(pThing) )
    {
        return 0;
    }

    SithQuetzUserBlock* pQuetz = pThing->userblock.pQuetz;

    float fudge = 0.0f;
    if ( pQuetz->bPosFudged )
    {
        fudge = pQuetz->unknown393;
    }

    rdVector_ScaleAdd3(pOutPoint, pDirection, distance, &pThing->pos);

    rdVector3 beginPos;
    rdVector_Copy3(&beginPos, &pThing->pos);
    beginPos.z = 0.3f - sithPhysics_GetThingHeight(pThing) + 0.002f - fudge + beginPos.z;

    rdVector3 endPos;
    rdVector_Copy3(&endPos, &beginPos);

    SithSector* pCurSector = sithCollision_FindSectorInRadius(pThing->pInSector, &pThing->pos, &beginPos, 0.0f);
    if ( !pCurSector )
    {
        SITHLOG_ERROR("_SnakeMungeTestCheck - couldn't find sector of start pointt");
        return 0;
    }

    rdVector3 delta;
    rdVector_Sub3(&delta, &endPos, &beginPos);
    delta.x = stdMath_ClipNearZero(delta.x);
    delta.y = stdMath_ClipNearZero(delta.y);
    delta.z = stdMath_ClipNearZero(delta.z);
    if ( delta.x != 0.0f || delta.y != 0.0f || delta.z != 0.0f )
    {
        SITHLOG_ERROR("_SnakeMungeTestCheck - no LOS to start point");
        return 0;
    }

    // TODO: [BUG] The original ORs into the bit pattern of the distance argument (the path flags were never initialized)
    int mask;
    memcpy(&mask, &distance, sizeof(mask));
    mask |= 0x62010;

    rdVector3 curHitPos;
    int result = sithAIUtil_CheckPathToPos(pLocal, pCurSector, &beginPos, mask, 0.0f, distance + 0.05f, 0.3f, pDirection, &curHitPos);
    switch ( result )
    {
        case 0:
            return 0;

        case 1:
            return 1;

        default:
        {
            float len = sqrtf((beginPos.x - curHitPos.x) * (beginPos.x - curHitPos.x) + (beginPos.y - curHitPos.y) * (beginPos.y - curHitPos.y));
            if ( len - 0.05f >= 0.2f )
            {
                rdVector_ScaleAdd3(pOutPoint, pDirection, len - 0.05f, &pThing->pos);
                return 1;
            }

            return 0;
        }
    }
}

void J3DAPI sithAIInstinct_sub_494360(SithAIControlBlock* pLocal, rdVector3* moveToPosLeft, rdVector3* moveToPosRight)
{
    SithThing* pThing = pLocal->pOwner;

    rdVector3 diff;
    rdVector3 probe;
    rdVector_Sub3(&diff, moveToPosLeft, &pThing->pos);
    float range = rdVector_Normalize3(&probe, &diff);
    if ( sithAIInstinct_SnakeMungeTestCheck(pLocal, &probe, range, moveToPosLeft) )
    {
        return;
    }

    rdVector_Sub3(&diff, moveToPosRight, &pThing->pos);
    float altReach = rdVector_Normalize3(&probe, &diff);
    if ( sithAIInstinct_SnakeMungeTestCheck(pLocal, &probe, altReach, moveToPosLeft) )
    {
        return;
    }

    int side = SITH_RANDF() - 0.5f < 0.0f ? -1 : 1;
    rdVector_Copy3(&probe, &pThing->orient.lvec);

    int bFoundIt = 0;
    float angle  = 0.0f;
    int retries  = 24;
    do
    {
        if ( sithAIInstinct_SnakeMungeTestCheck(pLocal, &probe, 0.3f, moveToPosLeft) )
        {
            bFoundIt = 1;
        }

        if ( angle == 0.0f )
        {
            angle = side * 15.0f;
        }
        else if ( (angle < 0.0f ? -1 : 1) == side )
        {
            angle = -angle;
        }
        else
        {
            angle = -angle + side * 15.0f;
        }

        rdVector3 angles = { 0 };
        angles.y = angle;
        rdVector_Rotate3(&probe, &pThing->orient.lvec, &angles);
    } while ( !bFoundIt && retries-- );

    // Note, the original also formats "Retires %d" into an unused local buffer when a position was found
    if ( !bFoundIt )
    {
        rdVector_Copy3(moveToPosLeft, &pThing->pos);
    }
}

signed int J3DAPI sithAIInstinct_SnakeFollow(SithAIControlBlock* pLocal, SithAIInstinct* pInstinct, SithAIInstinctState* pState, SithAIEventType event, SithThing* pThing)
{
    J3D_UNUSED(pInstinct);

    SITH_ASSERTREL(pLocal && pLocal->pClass && pLocal->pOwner);
    int bAdvance = 1;
    SithThing* pOwner = pLocal->pOwner;

    switch ( event )
    {
        case SITHAI_EVENT_HIT_WALL:
        case SITHAI_EVENT_HIT_CLIFF:
        case SITHAI_EVENT_HIT_THING:
            if ( (pLocal->mode & SITHAI_MODE_MOVING) == 0 || (pLocal->mode & SITHAI_MODE_ACTIVE) == 0 )
            {
                return 1;
            }
            break;

        case SITHAI_EVENT_TOUCHED:
            if ( pThing->type == SITH_THING_PLAYER )
            {
                bAdvance = 0;
            }
            break;

        case (SithAIEventType)0:
            if ( (pLocal->mode & SITHAI_MODE_MOVING) != 0 )
            {
                bAdvance = 0;
            }
            break;

        case SITHAI_EVENT_GOAL_REACHED:
            break;

        default:
            return 0;
    }

    if ( !pLocal->goalThing )
    {
        pLocal->submode &= ~0x200;
        return 0;
    }

    pState->msecNextUpdate = sithTime_g_msecGameTime + 500;
    sithAIUtil_sub_49B640(pLocal);
    sithAIUtil_sub_49B2E0(pLocal, 0);

    rdVector3 dirVec;
    float distance;
    if ( !pLocal->targetSightState2 || pLocal->targetSightState2 == 2 )
    {
        pState->aParams[0] = 0.0f;
        rdVector_Copy3(&dirVec, &pLocal->vecUnknown0);
        distance = pLocal->targetDistance;
        pLocal->mode |= SITHAI_MODE_LOSTSIGHTOFGOAL;
    }
    else
    {
        pState->aParams[0] = 1.0f;
        rdVector_Sub3(&dirVec, &pLocal->vecUnknown5, &pLocal->pOwner->pos);
        distance = rdVector_Normalize3Acc(&dirVec);
        pLocal->mode &= ~SITHAI_MODE_LOSTSIGHTOFGOAL;
    }

    if ( bAdvance )
    {
        float dotRight;
        float dotFwd;
        float yaw;
        float turn1;
        float turnAngle2;
        float farDist;
        float nearRange;

        dotFwd = rdVector_Dot3(&pLocal->pOwner->orient.lvec, &dirVec);
        dotRight = rdVector_Dot3(&pLocal->pOwner->orient.rvec, &dirVec);
        yaw = fabsf(rdMath_DeltaAngleNormalized(&pLocal->pOwner->orient.lvec, &dirVec, &rdroid_g_zVector3));

        if ( (pLocal->mode & SITHAI_MODE_ATTACKING) != 0 )
        {
            farDist = 0.8f;
            nearRange = 0.3f;
        }
        else
        {
            farDist = 0.65f;
            nearRange = sithAIMove_g_flt_585468;
        }

        float travelDist;
        float lookLen;
        if ( dotFwd <= -0.98f )
        {
            turn1 = (SITH_RANDF() - 0.5f < 0.0f ? -1 : 1) * 45.0f;
            turnAngle2 = -turn1;
            travelDist = 0.25f;
            lookLen = 0.25f;
        }
        else if ( dotFwd < 0.5f )
        {
            turn1 = 45.0f;
            turnAngle2 = 60.0f;
            travelDist = 0.25f;
            lookLen = 0.25f;
        }
        else if ( distance > farDist )
        {
            if ( sithAIInstinct_flt_539AB8 < 0.5f )
            {
                turn1 = yaw;
                turnAngle2 = 0.0f;
                travelDist = 0.2f;
                lookLen = 0.2f;
            }
            else
            {
                turn1 = yaw + 30.0f;
                turn1 = STDMATH_CLAMP(turn1, 0.0f, 60.0f);
                turnAngle2 = -10.0f + yaw;
                travelDist = 0.5f;
                lookLen = 0.5f;
            }
        }
        else if ( distance > nearRange )
        {
            turn1 = yaw;
            turnAngle2 = yaw;
            if ( (pLocal->mode & SITHAI_MODE_ATTACKING) != 0 )
            {
                travelDist = distance - 0.1f;
                travelDist = STDMATH_CLAMP(travelDist, 0.1f, distance);
                lookLen = distance - 0.45f;
                lookLen = STDMATH_CLAMP(lookLen, 0.1f, distance);
            }
            else
            {
                travelDist = distance;
                lookLen = distance - 0.05f;
                lookLen = STDMATH_CLAMP(lookLen, 0.1f, distance);
            }
        }
        else
        {
            turn1 = -yaw;
            turnAngle2 = yaw * 2.0f;
            travelDist = distance + 0.1f;
            lookLen = distance + 0.1f;
        }

        sithAIInstinct_flt_539AB8 = dotFwd;
        rdVector3 pyr2;
        rdVector3 pyr1;
        if ( dotRight > 0.0f )
        {
            rdVector_Set3(&pyr1, 0.0f, -turn1, 0.0f);
            rdVector_Set3(&pyr2, 0.0f, -turnAngle2, 0.0f);
        }
        else
        {
            rdVector_Set3(&pyr1, 0.0f, turn1, 0.0f);
            rdVector_Set3(&pyr2, 0.0f, turnAngle2, 0.0f);
        }

        rdVector_Rotate3(&dirVec, &pLocal->pOwner->orient.lvec, &pyr1);
        rdVector3 moveGoal;
        rdVector_ScaleAdd3(&moveGoal, &dirVec, travelDist, &pLocal->pOwner->pos);

        rdVector_Rotate3(&dirVec, &pLocal->pOwner->orient.lvec, &pyr2);
        rdVector3 altPos;
        rdVector_ScaleAdd3(&altPos, &dirVec, lookLen, &pLocal->pOwner->pos);

        moveGoal.z = pLocal->pOwner->pos.z;
        altPos.z = pLocal->pOwner->pos.z;
        sithAIInstinct_sub_494360(pLocal, &moveGoal, &altPos);
        sithAIMove_AISetMovePos(pLocal, &moveGoal, 2.0f);

        rdVector_Copy3(&moveGoal, &pLocal->movePos);
        moveGoal.z += pLocal->pOwner->thingInfo.actorInfo.eyeOffset.z;
        sithAIMove_AISetLookPos(pLocal, &moveGoal);

        rdVector_Sub3(&dirVec, &pLocal->movePos, &pOwner->pos);
        dotRight = rdVector_Dot3(&pLocal->pOwner->orient.rvec, &dirVec);
        sithAIInstinct_sub_494FF0(pOwner, dotRight);

        if ( pLocal->pOwner->userblock.pQuetz )
        {
            pLocal->pOwner->userblock.pQuetz->unknown391 = 1;
        }
    }

    pLocal->mode &= ~SITHAI_MODE_UNKNOWN_80;
    pLocal->submode |= 0x400;
    pLocal->submode |= 0x200;
    pLocal->submode |= 0x100;

    switch ( event )
    {
        case SITHAI_EVENT_TOUCHED:
        case SITHAI_EVENT_HIT_WALL:
        case SITHAI_EVENT_HIT_CLIFF:
        case SITHAI_EVENT_HIT_THING:
            return 1;

        default:
            return 0;
    }
}

void J3DAPI sithAIInstinct_sub_494FF0(const SithThing* pThing, float a2)
{
    tSoundHandle hSnd = 0;
    if ( fabsf(a2) < 0.1f )
    {
        a2 = 0.0f;
    }
    else if ( a2 > 0.0f )
    {
        a2 = 1.0f;
    }
    else
    {
        a2 = -1.0f;
    }

    if ( sithAIMove_g_flt_585464 != a2 )
    {
        if ( a2 < 0.0f )
        {
            hSnd = sithSound_Load(sithWorld_g_pCurrentWorld, "olv_boss_slither1.wav");
        }
        else
        {
            hSnd = sithSound_Load(sithWorld_g_pCurrentWorld, "olv_boss_slither2.wav");
        }

        if ( hSnd )
        {
            sithSoundMixer_PlaySoundThing(hSnd, pThing, 1.0f, 0.01f, 7.0f, SOUNDPLAY_THING_POS);
        }
        sithAIMove_g_flt_585464 = a2;
    }
}

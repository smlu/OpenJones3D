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

//
// Indices of the instinct arguments (SithAIInstinct fltArg/intArg, both hold the same class file argument) and
// of the instinct state parameters (SithAIInstinctState aParams) per instinct
//
// Listen
#define SITHAI_LISTEN_ARG_HEARCHANCE        0
#define SITHAI_LISTEN_ARG_INVESTIGATECHANCE 1
#define SITHAI_LISTEN_ARG_BACKOFFDIST       2
#define SITHAI_LISTEN_PARAM_LOUDNESS        0

// LookForTarget
#define SITHAI_LOOKFORTARGET_ARG_INTERVAL    0
#define SITHAI_LOOKFORTARGET_ARG_ATTACKTIME  1
#define SITHAI_LOOKFORTARGET_ARG_LOOKONLY    2
#define SITHAI_LOOKFORTARGET_ARG_HUNTTIME    3
#define SITHAI_LOOKFORTARGET_PARAM_HUNTSTART 0

// SenseDanger
#define SITHAI_SENSEDANGER_ARG_INTERVAL    0
#define SITHAI_SENSEDANGER_ARG_MINLOUDNESS 1
#define SITHAI_SENSEDANGER_ARG_SIGHTCHANCE 2
#define SITHAI_SENSEDANGER_ARG_HITCHANCE   3

// FearGunshot
#define SITHAI_FEARGUNSHOT_ARG_NEARDIST   0
#define SITHAI_FEARGUNSHOT_ARG_FARDIST    1
#define SITHAI_FEARGUNSHOT_ARG_FLEECHANCE 2
#define SITHAI_FEARGUNSHOT_ARG_MINDOT     3

// Hop
#define SITHAI_HOP_ARG_DELAY 0
#define SITHAI_HOP_ARG_FORCE 1

// HoverDrift
#define SITHAI_HOVERDRIFT_ARG_INTERVAL 0
#define SITHAI_HOVERDRIFT_ARG_FORCE    1

// Talk
#define SITHAI_TALK_ARG_DELAY          0
#define SITHAI_TALK_ARG_CHANCE         1
#define SITHAI_TALK_ARG_COOLDOWN       2
#define SITHAI_TALK_PARAM_LASTTALKTIME 0

// RandomTurn
#define SITHAI_RANDOMTURN_ARG_INTERVAL 0
#define SITHAI_RANDOMTURN_ARG_LOOKDIST 1

// Jump
#define SITHAI_JUMP_ARG_DELAY          0
#define SITHAI_JUMP_ARG_HEIGHT         1
#define SITHAI_JUMP_ARG_CLIFFDIST      2
#define SITHAI_JUMP_PARAM_NEXTJUMPTIME 0

// LobFire
#define SITHAI_LOBFIRE_ARG_FIREDELAY    0
#define SITHAI_LOBFIRE_ARG_FIREDOT      1
#define SITHAI_LOBFIRE_ARG_MINDIST      2
#define SITHAI_LOBFIRE_ARG_MAXDIST      3
#define SITHAI_LOBFIRE_ARG_AIMERROR     4
#define SITHAI_LOBFIRE_ARG_ALTWPNCHANCE 5
#define SITHAI_LOBFIRE_ARG_WEAPON       6

// PrimaryFire
#define SITHAI_PRIMARYFIRE_ARG_FIREDELAY       0
#define SITHAI_PRIMARYFIRE_ARG_FIREDOT         1
#define SITHAI_PRIMARYFIRE_ARG_MAXDIST         2
#define SITHAI_PRIMARYFIRE_ARG_AIMERROR        3
#define SITHAI_PRIMARYFIRE_ARG_MINDIST         4
#define SITHAI_PRIMARYFIRE_ARG_LEADCHANCE      6
#define SITHAI_PRIMARYFIRE_ARG_ALTWPNCHANCE    7
#define SITHAI_PRIMARYFIRE_ARG_BURST           8
#define SITHAI_PRIMARYFIRE_ARG_STILLALTWPN     10
#define SITHAI_PRIMARYFIRE_ARG_WEAPON          11
#define SITHAI_PRIMARYFIRE_ARG_MOVEFIREMODE    14
#define SITHAI_PRIMARYFIRE_ARG_ALTWPNDIST      15
#define SITHAI_PRIMARYFIRE_ARG_NOLOOK          16
#define SITHAI_PRIMARYFIRE_ARG_HOLDDOT         17
#define SITHAI_PRIMARYFIRE_MOVEFIRE_DEFAULTWPN 1 // Value of MOVEFIREMODE: fire weapon -1 while moving
#define SITHAI_PRIMARYFIRE_MOVEFIRE_NONE       2 // Value of MOVEFIREMODE: do not fire while moving

// AlternateFire
#define SITHAI_ALTERNATEFIRE_ARG_WEAPON     0
#define SITHAI_ALTERNATEFIRE_ARG_RETRYDELAY 1
#define SITHAI_ALTERNATEFIRE_ARG_FIREDELAY  2
#define SITHAI_ALTERNATEFIRE_ARG_FIREDOT    3
#define SITHAI_ALTERNATEFIRE_ARG_MINDIST    4
#define SITHAI_ALTERNATEFIRE_ARG_MAXDIST    5
#define SITHAI_ALTERNATEFIRE_ARG_AIMERROR   6
#define SITHAI_ALTERNATEFIRE_ARG_LEADCHANCE 7
#define SITHAI_ALTERNATEFIRE_ARG_BURST      8
#define SITHAI_ALTERNATEFIRE_ARG_STOPONFIRE 9

// CircleStrafe
#define SITHAI_CIRCLESTRAFE_ARG_INTERVAL     0
#define SITHAI_CIRCLESTRAFE_ARG_ANGLE        1
#define SITHAI_CIRCLESTRAFE_ARG_MAXDIST      2
#define SITHAI_CIRCLESTRAFE_ARG_MOVEINTERVAL 3
#define SITHAI_CIRCLESTRAFE_ARG_RANDOM       4
#define SITHAI_CIRCLESTRAFE_ARG_MINSTEP      5
#define SITHAI_CIRCLESTRAFE_ARG_LOOKATDEST   6
#define SITHAI_CIRCLESTRAFE_ARG_WAITFIRE     7
#define SITHAI_CIRCLESTRAFE_ARG_SPEED        8
#define SITHAI_CIRCLESTRAFE_PARAM_SIDE       0
#define SITHAI_CIRCLESTRAFE_PARAM_NUMSTEPS   1

// Dodge
#define SITHAI_DODGE_ARG_MINDIST         0
#define SITHAI_DODGE_ARG_MAXDIST         1
#define SITHAI_DODGE_ARG_SPEED           2
#define SITHAI_DODGE_ARG_COOLDOWN        3
#define SITHAI_DODGE_ARG_MINTARGETDIST   4
#define SITHAI_DODGE_ARG_TARGETEDCHANCE  5
#define SITHAI_DODGE_ARG_HITCHANCE       6
#define SITHAI_DODGE_ARG_SOUNDCHANCE     7
#define SITHAI_DODGE_ARG_ROLLCHANCE      8
#define SITHAI_DODGE_ARG_VEHICLECHANCE   9
#define SITHAI_DODGE_ARG_PATHMINDIST     10
#define SITHAI_DODGE_PARAM_LASTDODGETIME 0

// HitAndRun
#define SITHAI_HITANDRUN_ARG_ATTACKTIME      0
#define SITHAI_HITANDRUN_ARG_FLEETIME        1
#define SITHAI_HITANDRUN_ARG_FIRECHANCE      2
#define SITHAI_HITANDRUN_PARAM_ATTACKELAPSED 0
#define SITHAI_HITANDRUN_PARAM_FLEEPENDING   1
#define SITHAI_HITANDRUN_PARAM_FLEEING       2
#define SITHAI_HITANDRUN_PARAM_HASFIRED      3

// Retreat
#define SITHAI_RETREAT_ARG_INTERVAL      0
#define SITHAI_RETREAT_ARG_HEALTH        1
#define SITHAI_RETREAT_ARG_CHANCE        2
#define SITHAI_RETREAT_ARG_MAXRETREATS   3
#define SITHAI_RETREAT_PARAM_NUMRETREATS 0

// RandomMove
#define SITHAI_RANDOMMOVE_ARG_INTERVAL    0
#define SITHAI_RANDOMMOVE_ARG_MINDIST     1
#define SITHAI_RANDOMMOVE_ARG_MAXDIST     2
#define SITHAI_RANDOMMOVE_ARG_MINSPEED    3
#define SITHAI_RANDOMMOVE_ARG_MAXSPEED    4
#define SITHAI_RANDOMMOVE_ARG_ANGLE       5
#define SITHAI_RANDOMMOVE_ARG_WAITFIRE    6
#define SITHAI_RANDOMMOVE_ARG_MOVEONFIRE  7
#define SITHAI_RANDOMMOVE_ARG_ALWAYSLOOK  8
#define SITHAI_RANDOMMOVE_ARG_ALLOW3D     9
#define SITHAI_RANDOMMOVE_PARAM_TARGETX   0
#define SITHAI_RANDOMMOVE_PARAM_TARGETY   1
#define SITHAI_RANDOMMOVE_PARAM_TARGETZ   2
#define SITHAI_RANDOMMOVE_PARAM_HASTARGET 3

// HumanCombatMove
#define SITHAI_HUMANCOMBATMOVE_ARG_MOVEDELAY       0
#define SITHAI_HUMANCOMBATMOVE_ARG_ALLOWSTRAFE     1
#define SITHAI_HUMANCOMBATMOVE_ARG_MINDIST         2
#define SITHAI_HUMANCOMBATMOVE_ARG_MAXDIST         3
#define SITHAI_HUMANCOMBATMOVE_ARG_MINSPEED        4
#define SITHAI_HUMANCOMBATMOVE_ARG_FASTSPEED       5
#define SITHAI_HUMANCOMBATMOVE_ARG_FLEEHEALTH      6
#define SITHAI_HUMANCOMBATMOVE_ARG_FLEECHANCE      7
#define SITHAI_HUMANCOMBATMOVE_ARG_STRAFECHANCE    8
#define SITHAI_HUMANCOMBATMOVE_ARG_CHASECHANCE     9
#define SITHAI_HUMANCOMBATMOVE_ARG_STANDFIRECHANCE 10
#define SITHAI_HUMANCOMBATMOVE_PARAM_NEXTMOVETIME  0
#define SITHAI_HUMANCOMBATMOVE_PARAM_MOVECHANCE    1
#define SITHAI_HUMANCOMBATMOVE_PARAM_TRIEDMOVES    2
#define SITHAI_HUMANCOMBATMOVE_PARAM_CHASESTATE    3

// Roam
#define SITHAI_ROAM_ARG_INTERVAL 0
#define SITHAI_ROAM_ARG_RADIUS   1
#define SITHAI_ROAM_ARG_ALLOW3D  2
#define SITHAI_ROAM_ARG_SPEED    3

// Flee
#define SITHAI_FLEE_ARG_RANGE       0
#define SITHAI_FLEE_ARG_INTERVAL    1
#define SITHAI_FLEE_ARG_PANICTIME   2
#define SITHAI_FLEE_ARG_ANGLE       3
#define SITHAI_FLEE_ARG_SPEED       4
#define SITHAI_FLEE_ARG_MINSTEP     5
#define SITHAI_FLEE_PARAM_STARTTIME 0

// BasicFallow
#define SITHAI_BASICFOLLOW_ARG_INTERVAL     0
#define SITHAI_BASICFOLLOW_ARG_MINDIST      1
#define SITHAI_BASICFOLLOW_ARG_MAXDIST      2
#define SITHAI_BASICFOLLOW_ARG_IDEALDIST    3
#define SITHAI_BASICFOLLOW_ARG_IGNORELOS    4
#define SITHAI_BASICFOLLOW_ARG_WAITFIRE     5
#define SITHAI_BASICFOLLOW_PARAM_NUMBLOCKED 0
#define SITHAI_BASICFOLLOW_PARAM_HITFLAG    1

// SnakeFollow
#define SITHAI_SNAKEFOLLOW_PARAM_USELASTPOS 0

//
// Snake follow vars
//
static float sithAIInstinct_snakeLastDotFwd      = 1.0f; // Dot of the facing and goal direction of the last snake update
static float sithAIInstinct_snakeSlitherDir      = 0.0f; // Last slither direction (-1, 0 or 1) a slither sound was played for
static float sithAIInstinct_snakeFollowNearRange = 0.0f; // Range below which a non attacking snake backs off, never written by the original

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
    J3D_HOOKFUNC(sithAIInstinct_SnakeFindMovePos);
    J3D_HOOKFUNC(sithAIInstinct_SnakeMungeTestCheck);
    J3D_HOOKFUNC(sithAIInstinct_SnakeFollow);
    J3D_HOOKFUNC(sithAIInstinct_SnakePlaySlitherSound);
}

void sithAIInstinct_ResetGlobals(void)
{
    sithAIInstinct_snakeLastDotFwd      = 1.0f;
    sithAIInstinct_snakeSlitherDir      = 0.0f;
    sithAIInstinct_snakeFollowNearRange = 0.0f;
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
            if ( pState->aParams[SITHAI_LISTEN_PARAM_LOUDNESS] == 0.0f )
            {
                if ( event == SITHAI_EVENT_HIT_SECTOR )
                {
                    sithAIUtil_AIPlaySoundMode(pLocal, SITHSOUNDCLASS_SURPRISE);
                }
                else
                {
                    sithAIUtil_AIPlaySoundMode(pLocal, SITHSOUNDCLASS_CURIOUS);
                }
                pState->aParams[SITHAI_LISTEN_PARAM_LOUDNESS] = 0.1f;
            }

            if ( pThing )
            {
                SithThing* pShooter = sithThing_GetThingParent(pThing);
                if ( event == SITHAI_EVENT_HIT_SECTOR )
                {
                    if ( SITH_RANDF() > pInstinct->fltArg[SITHAI_LISTEN_ARG_INVESTIGATECHANCE] )
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

                    if ( SITH_RANDF() > pInstinct->fltArg[SITHAI_LISTEN_ARG_INVESTIGATECHANCE] )
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
                            rdVector_Scale3Acc(&pos, pInstinct->fltArg[SITHAI_LISTEN_ARG_BACKOFFDIST]);
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

    if ( pSectorInfo->aLevelAtTransmittingPos[0] != 0.0f && SITH_RANDF() < pInstinct->fltArg[SITHAI_LISTEN_ARG_HEARCHANCE] )
    {
        bool bMove = false;
        bool bLook = false;
        bool bStop = false;
        bool bHunt = false;
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
        SithAISightState sightVal = sithAIUtil_GetDistanceToTargetPos(pListener, &pListener->pos, &pSectorInfo->aStartPos[0], -1.0f, pLocal->pClass->sightDistance, 0.0f, &toDest, &distance);
        if ( distance <= pLocal->pClass->heardDistance )
        {
            SithAISightState thingVis;
            if ( pSource && pSource->type )
            {
                rdVector3 thingVec;
                float hitDist;
                thingVis = sithAIUtil_GetDistanceToTargetPos(pListener, &pListener->pos, &pSource->pos, -1.0f, pLocal->pClass->sightDistance, 0.0f, &thingVec, &hitDist);
            }
            else
            {
                thingVis = SITHAI_SIGHT_HIDDEN;
            }

            float loudness = 0.0f;
            if ( pSource && pSource->type && (pLocal->mode & (SITHAI_MODE_HUNTING | SITHAI_MODE_INSTINCTUSEWPNTS)) != 0 )
            {
                bHunt = true;

                // TODO: [BUG] The original never sets loudness on this path; it reads the stack slot of the pInstinct argument as a float
                memcpy(&loudness, &pInstinct, sizeof(loudness));
            }
            else if ( thingVis == SITHAI_SIGHT_VISIBLE )
            {
                pLocal->alertPos = pSource->pos;
                loudness = pSectorInfo->aLevelAtTransmittingPos[0];
                bMove = true;
                bLook = true;
            }
            else if ( sightVal == SITHAI_SIGHT_VISIBLE )
            {
                pLocal->alertPos = pSectorInfo->aStartPos[0];
                loudness = pSectorInfo->aLevelAtTransmittingPos[0];
                bMove = true;
                bLook = true;
            }
            else if ( pSource && (pLocal->mode & SITHAI_MODE_HUNTING) == 0 )
            {
                loudness = pSectorInfo->aLevelAtTransmittingPos[0];
                bStop = true;
                bLook = true;
            }

            if ( bStop || bMove || bLook || bHunt )
            {
                if ( pSource && pSource->type )
                {
                    bool bSrcUnderwater = (pSource->pInSector->flags & SITH_SECTOR_UNDERWATER) != 0;
                    bool bUnderwater    = (pLocal->pOwner->pInSector->flags & SITH_SECTOR_UNDERWATER) != 0;
                    if ( (bSrcUnderwater && !bUnderwater) || (!bSrcUnderwater && bUnderwater) )
                    {
                        return 0;
                    }
                }

                if ( loudness > pState->aParams[SITHAI_LISTEN_PARAM_LOUDNESS] )
                {
                    pState->aParams[SITHAI_LISTEN_PARAM_LOUDNESS] = loudness;
                }

                sithAIUtil_AIPlaySoundMode(pLocal, SITHSOUNDCLASS_CURIOUS);
                if ( bStop )
                {
                    sithAIMove_StopAIMovement(pLocal);
                }

                if ( bMove && SITH_RANDF() <= pInstinct->fltArg[SITHAI_LISTEN_ARG_INVESTIGATECHANCE] )
                {
                    sithAIMove_AISetMovePos(pLocal, &pLocal->alertPos, 0.75f);
                }

                if ( bLook )
                {
                    sithAIMove_AISetLookPosEyeLevel(pLocal, &pLocal->alertPos);
                }

                if ( bHunt && SITH_RANDF() <= pInstinct->fltArg[SITHAI_LISTEN_ARG_INVESTIGATECHANCE] )
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
            pState->aParams[SITHAI_LOOKFORTARGET_PARAM_HUNTSTART] = sithTime_g_secGameTime;
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
        float curhuntTime = pInstinct->fltArg[SITHAI_LOOKFORTARGET_ARG_HUNTTIME];
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

            if ( sithTime_g_secGameTime > pState->aParams[SITHAI_LOOKFORTARGET_PARAM_HUNTSTART] + curhuntTime )
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
        if ( !pInstinct->intArg[SITHAI_LOOKFORTARGET_ARG_ATTACKTIME] )
        {
            return 0;
        }

        if ( pLocal->msecAttackStart + pInstinct->intArg[SITHAI_LOOKFORTARGET_ARG_ATTACKTIME] < sithTime_g_msecGameTime )
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

    pState->msecNextUpdate = sithTime_g_msecGameTime + pInstinct->intArg[SITHAI_LOOKFORTARGET_ARG_INTERVAL];
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

    sithAIUtil_UpdateTargetSight(pLocal, 0);
    if ( pLocal->targetSightState == SITHAI_SIGHT_VISIBLE )
    {
        if ( pLocal->goalThing == pLocal->pTargetThing )
        {
            pLocal->unknown141        = sithMain_g_frameNumber;
            pLocal->targetSightState2 = pLocal->targetSightState;
            pLocal->vecUnknown0       = pLocal->toTarget;
            pLocal->targetDistance    = pLocal->distance;
            pLocal->vecUnknown5       = pLocal->pTargetThing->pos;
            pLocal->vecUnknown6       = pLocal->pTargetThing->pos;
            pLocal->unknown150        = sithTime_g_msecGameTime;
        }

        if ( pInstinct->fltArg[SITHAI_LOOKFORTARGET_ARG_LOOKONLY] != 0.0f )
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

    if ( pInstinct->fltArg[SITHAI_LOOKFORTARGET_ARG_INTERVAL] == 0.0f )
    {
        pInstinct->fltArg[SITHAI_LOOKFORTARGET_ARG_INTERVAL] = 500.0f;
    }
    return 0;
}

int J3DAPI sithAIInstinct_SenseDanger(SithAIControlBlock* pLocal, SithAIInstinct* pInstance, SithAIInstinctState* pState, SithAIEventType event, SithThing* pThing)
{
    SithThing* pOwner = pLocal->pOwner;
    SithAIAwarenessSector* pAISector = &sithAIAwareness_g_aSectors[sithSector_GetSectorIndex(pOwner->pInSector)];
    if ( !event && pInstance->intArg[SITHAI_SENSEDANGER_ARG_INTERVAL] )
    {
        pState->msecNextUpdate = sithTime_g_msecGameTime + pInstance->intArg[SITHAI_SENSEDANGER_ARG_INTERVAL];
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
            if ( SITH_RANDF() > pInstance->fltArg[SITHAI_SENSEDANGER_ARG_HITCHANCE] )
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
            if ( pAISector->aLevelAtTransmittingPos[2] <= pInstance->fltArg[SITHAI_SENSEDANGER_ARG_MINLOUDNESS] )
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
            SithAISightState curSightState = sithAIUtil_GetDistanceToTarget(pOwner, &pOwner->pos, pDanger, -1.0f, pLocal->pClass->heardDistance, 0.0f, &toTarget, &distance);
            if ( curSightState != SITHAI_SIGHT_VISIBLE )
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
            if ( SITH_RANDF() > pInstance->fltArg[SITHAI_SENSEDANGER_ARG_SIGHTCHANCE] )
            {
                return 0;
            }

            pLocal->pTargetThing = sithPlayer_g_pLocalPlayerThing;
            if ( !sithAIUtil_CanAttackTarget(pLocal->pTargetThing) )
            {
                pLocal->pTargetThing = NULL;
                return 0;
            }

            sithAIUtil_UpdateTargetSight(pLocal, 0);
            if ( pLocal->targetSightState == SITHAI_SIGHT_VISIBLE )
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
                    if ( dist < pInstinct->fltArg[SITHAI_FEARGUNSHOT_ARG_NEARDIST] || (dist < pInstinct->fltArg[SITHAI_FEARGUNSHOT_ARG_FARDIST] && dot > pInstinct->fltArg[SITHAI_FEARGUNSHOT_ARG_MINDOT] && SITH_RANDF() < pInstinct->fltArg[SITHAI_FEARGUNSHOT_ARG_FLEECHANCE]) )
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

    float hopDelay = (float)pInstinct->intArg[SITHAI_HOP_ARG_DELAY];
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

    float hopForce = pInstinct->fltArg[SITHAI_HOP_ARG_FORCE];
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

    float curSpread = pInstinct->fltArg[SITHAI_HOVERDRIFT_ARG_INTERVAL];
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
    curForce.x = SITH_RANDF() * pInstinct->fltArg[SITHAI_HOVERDRIFT_ARG_FORCE] * direction.x;
    curForce.y = SITH_RANDF() * pInstinct->fltArg[SITHAI_HOVERDRIFT_ARG_FORCE] * direction.y;
    curForce.z = SITH_RANDF() * pInstinct->fltArg[SITHAI_HOVERDRIFT_ARG_FORCE] * direction.z;
    sithPhysics_ApplyForce(pLocal->pOwner, &curForce);
    return 0;
}

int J3DAPI sithAIInstinct_Talk(SithAIControlBlock* pLocal, SithAIInstinct* pInstinct, SithAIInstinctState* pState, SithAIEventType event, void* pObject)
{
    float talkChance = pInstinct->fltArg[SITHAI_TALK_ARG_CHANCE];
    float delay = pInstinct->fltArg[SITHAI_TALK_ARG_DELAY];
    float nextTalkTime = -1.0f;
    SithThing* pOwner = pLocal->pOwner;
    bool bSuccess     = false;
    int hChannel      = 0;

    if ( pInstinct->fltArg[SITHAI_TALK_ARG_COOLDOWN] != 0.0f )
    {
        nextTalkTime = pState->aParams[SITHAI_TALK_PARAM_LASTTALKTIME] + pInstinct->fltArg[SITHAI_TALK_ARG_COOLDOWN];
    }

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

            bool bPlayerWater = false;
            if ( sithPlayer_g_pLocalPlayerThing )
            {
                bPlayerWater = (sithPlayer_g_pLocalPlayerThing->pInSector->flags & SITH_SECTOR_UNDERWATER) != 0;
            }

            bool bOwnerWater = (pLocal->pOwner->pInSector->flags & SITH_SECTOR_UNDERWATER) != 0;
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
        {
            SithAIMode prevMode = (SithAIMode)(int)pObject;
            if ( (pLocal->mode & SITHAI_MODE_UNKNOWN_100000) != 0 && (prevMode & SITHAI_MODE_UNKNOWN_100000) == 0 )
            {
                sithSoundMixer_StopAllSoundsThing(pLocal->pOwner);
                hChannel = sithSoundClass_PlayModeFirst(pLocal->pOwner, SITHSOUNDCLASS_RESERVED8);
            }
            break;
        }

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

            hChannel = sithSoundClass_PlayModeRandom(pLocal->pOwner, (SithSoundClassMode)(int)pObject);
            bSuccess = true;
            break;

        default:
            return 0;
    }

    if ( hChannel > 0 )
    {
        pState->aParams[SITHAI_TALK_PARAM_LASTTALKTIME] = sithTime_g_secGameTime;
    }

    return bSuccess;
}

int J3DAPI sithAIInstinct_RandomTurn(SithAIControlBlock* pLocal, SithAIInstinct* pInstinct, SithAIInstinctState* pState, SithAIEventType event, void* pObject)
{
    J3D_UNUSED(event);
    J3D_UNUSED(pObject);

    if ( !pInstinct->intArg[SITHAI_RANDOMTURN_ARG_INTERVAL] )
    {
        pState->msecNextUpdate = sithTime_g_msecGameTime + 5000;
    }
    else
    {
        pState->msecNextUpdate = sithTime_g_msecGameTime + pInstinct->intArg[SITHAI_RANDOMTURN_ARG_INTERVAL];
    }

    if ( (pLocal->mode & SITHAI_MODE_SEARCHING) == 0 )
    {
        return 0;
    }

    rdVector3 randomPyr;
    memset(&randomPyr, 0, sizeof(randomPyr));
    randomPyr.yaw = SITH_RANDF() * 360.0f;
    rdVector3 dir = rdroid_g_yVector3;
    rdVector_Rotate3Acc(&dir, &randomPyr);

    rdVector3 lookPos;
    rdVector_ScaleAdd3(&lookPos, &dir, pInstinct->fltArg[SITHAI_RANDOMTURN_ARG_LOOKDIST], &pLocal->pOwner->pos);

    rdVector3 toTarget;
    float distToLook;
    SithAISightState sightState = sithAIUtil_GetDistanceToTargetPos(pLocal->pOwner, &pLocal->pOwner->pos, &lookPos, -1.0f, pInstinct->fltArg[SITHAI_RANDOMTURN_ARG_LOOKDIST], 0.0f, &toTarget, &distToLook);
    if ( sightState == SITHAI_SIGHT_VISIBLE )
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

    if ( (double)sithTime_g_msecGameTime < pState->aParams[SITHAI_JUMP_PARAM_NEXTJUMPTIME] )
    {
        return 0;
    }

    pState->aParams[SITHAI_JUMP_PARAM_NEXTJUMPTIME] = (float)((double)sithTime_g_msecGameTime + pInstinct->fltArg[SITHAI_JUMP_ARG_DELAY]);
    rdVector3 upPos;
    rdVector3 curjumpPos;
    switch ( event )
    {
        case SITHAI_EVENT_TOUCHED:
        case SITHAI_EVENT_HIT_WALL:
            rdVector_ScaleAdd3(&upPos, &rdroid_g_zVector3, pInstinct->fltArg[SITHAI_JUMP_ARG_HEIGHT], &pThing->pos);
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
            if ( sithAIUtil_CheckFloorAtPos(pLocal, &curjumpPos, pSect, &bNoCollision) == SITHAIUTIL_FLOOR_OK && bNoCollision )
            {
                sithAIMove_AIJump(pLocal, &curjumpPos, 1.0f);
            }
            break;

        case SITHAI_EVENT_HIT_CLIFF:
            rdVector_ScaleAdd3(&curjumpPos, &pLocal->moveDirection, pInstinct->fltArg[SITHAI_JUMP_ARG_CLIFFDIST], &pThing->pos);
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

    SITH_ASSERTREL(pLocal);

    SithThing* pActor  = pLocal->pOwner;
    SithThing* pTarget = pLocal->pTargetThing;
    SITH_ASSERTREL(pActor);

    SithAIUtilFireFlags fireFlags = SITHAIUTIL_FIRE_LOB;
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

    int weaponNum = pInstinct->intArg[SITHAI_LOBFIRE_ARG_WEAPON];
    if ( SITH_RANDF() < pInstinct->fltArg[SITHAI_LOBFIRE_ARG_ALTWPNCHANCE] )
    {
        ++weaponNum;
    }

    sithAIUtil_SetWeaponFireFlags(weaponNum, &fireFlags);
    if ( sithAIUtil_AIFire(pLocal, pInstinct->fltArg[SITHAI_LOBFIRE_ARG_MINDIST], pInstinct->fltArg[SITHAI_LOBFIRE_ARG_MAXDIST], pInstinct->fltArg[SITHAI_LOBFIRE_ARG_FIREDOT], pInstinct->fltArg[SITHAI_LOBFIRE_ARG_AIMERROR], weaponNum, fireFlags, 1) )
    {
        pState->msecNextUpdate = sithTime_g_msecGameTime + pInstinct->intArg[SITHAI_LOBFIRE_ARG_FIREDELAY];
        pLocal->msecFireWaitTime = sithTime_g_msecGameTime + pInstinct->intArg[SITHAI_LOBFIRE_ARG_FIREDELAY];
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

    if ( pInstinct->fltArg[SITHAI_PRIMARYFIRE_ARG_HOLDDOT] != 0.0f && rdVector_Dot3(&pLocal->toTarget, &pActor->orient.lvec) > pInstinct->fltArg[SITHAI_PRIMARYFIRE_ARG_HOLDDOT] )
    {
        if ( pInstinct->fltArg[SITHAI_PRIMARYFIRE_ARG_NOLOOK] == 0.0f )
        {
            sithAIMove_AISetLookThing(pLocal, pLocal->pTargetThing);
        }
        return 0;
    }

    if ( pInstinct->fltArg[SITHAI_PRIMARYFIRE_ARG_LEADCHANCE] != 0.0f && SITH_RANDF() <= pInstinct->fltArg[SITHAI_PRIMARYFIRE_ARG_LEADCHANCE] )
    {
        flags |= SITHAIUTIL_FIRE_LEAD;
    }

    int weaponNum = pInstinct->intArg[SITHAI_PRIMARYFIRE_ARG_WEAPON];
    if ( (pLocal->mode & SITHAI_MODE_MOVING) != 0 )
    {
        switch ( pInstinct->intArg[SITHAI_PRIMARYFIRE_ARG_MOVEFIREMODE] )
        {
            case SITHAI_PRIMARYFIRE_MOVEFIRE_DEFAULTWPN:
                weaponNum = -1;
                break;

            case SITHAI_PRIMARYFIRE_MOVEFIRE_NONE:
                sithAIUtil_UpdateTargetSight(pLocal, 0);
                return 0;
        }
    }
    else if ( pInstinct->fltArg[SITHAI_PRIMARYFIRE_ARG_STILLALTWPN] != 0.0f )
    {
        float curFireDist = pInstinct->fltArg[SITHAI_PRIMARYFIRE_ARG_ALTWPNDIST];
        if ( curFireDist != 0.0f )
        {
            sithAIUtil_UpdateTargetSight(pLocal, 0);
            if ( curFireDist < pLocal->distance )
            {
                weaponNum = pInstinct->intArg[SITHAI_PRIMARYFIRE_ARG_WEAPON] + 1;
            }
        }
        else
        {
            weaponNum = pInstinct->intArg[SITHAI_PRIMARYFIRE_ARG_WEAPON] + 1;
        }
    }

    if ( weaponNum == pInstinct->intArg[SITHAI_PRIMARYFIRE_ARG_WEAPON] && pInstinct->fltArg[SITHAI_PRIMARYFIRE_ARG_ALTWPNCHANCE] != 0.0f && SITH_RANDF() <= pInstinct->fltArg[SITHAI_PRIMARYFIRE_ARG_ALTWPNCHANCE] )
    {
        weaponNum = pInstinct->intArg[SITHAI_PRIMARYFIRE_ARG_WEAPON] + 1;
    }

    sithAIUtil_SetWeaponFireFlags(weaponNum, &flags);

    int curBurstNum = pInstinct->intArg[SITHAI_PRIMARYFIRE_ARG_BURST];
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

    float closestDist = pInstinct->fltArg[SITHAI_PRIMARYFIRE_ARG_MINDIST];
    float farDist = pInstinct->fltArg[SITHAI_PRIMARYFIRE_ARG_MAXDIST];
    if ( sithWeapon_HasWeaponSelected(pActor) )
    {
        farDist = sithWeapon_GetWeaponMaxAimDistance(pActor->thingInfo.actorInfo.weaponInfo.curWeaponID);
    }

    if ( (pLocal->mode & SITHAI_MODE_UNKNOWN_100000) == 0 && !sithAIUtil_IsLineOfFireClear(pLocal, &pLocal->pOwner->pos) )
    {
        // No clear line of fire, skip the shot
    }
    else if ( sithAIUtil_AIFire(pLocal, closestDist, farDist, pInstinct->fltArg[SITHAI_PRIMARYFIRE_ARG_FIREDOT], pInstinct->fltArg[SITHAI_PRIMARYFIRE_ARG_AIMERROR], weaponNum, flags, curBurstNum) )
    {
        if ( pInstinct->fltArg[SITHAI_PRIMARYFIRE_ARG_NOLOOK] == 0.0f && !sithWeapon_IsAiming(pLocal->pOwner) )
        {
            sithAIMove_AISetLookThing(pLocal, pTarget);
        }

        float fireDelay = (SITH_RANDF() * 0.4f - 0.2f + 1.0f) * pInstinct->fltArg[SITHAI_PRIMARYFIRE_ARG_FIREDELAY];
        pState->msecNextUpdate = sithTime_g_msecGameTime + (uint32_t)fireDelay;
        return 0;
    }

    if ( pInstinct->fltArg[SITHAI_PRIMARYFIRE_ARG_NOLOOK] == 0.0f && (pLocal->mode & SITHAI_MODE_MOVING) == 0 )
    {
        SithWeaponId curWeaponID = pLocal->pOwner->thingInfo.actorInfo.weaponInfo.curWeaponID;
        switch ( pLocal->targetSightState )
        {
            case SITHAI_SIGHT_OUTOFFOV:
                sithAIMove_AISetLookThing(pLocal, pLocal->pTargetThing);
                break;

            case SITHAI_SIGHT_VISIBLE:
                if ( !rdVector_IsZero3(&pLocal->pTargetThing->moveInfo.physics.velocity) )
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
        pState->msecNextUpdate = sithTime_g_msecGameTime + pInstinct->intArg[SITHAI_ALTERNATEFIRE_ARG_RETRYDELAY];
        return 0;
    }

    if ( sithPuppet_IsAnyModeOnTrack(pLocal->pOwner, SITHPUPPETSUBMODE_FIRE, SITHPUPPETSUBMODE_FIRE4) )
    {
        pState->msecNextUpdate = sithTime_g_msecGameTime + 150;
        return 0;
    }

    int weaponNum = pInstinct->intArg[SITHAI_ALTERNATEFIRE_ARG_WEAPON];
    if ( weaponNum < 0 )
    {
        weaponNum = rand() % (abs(weaponNum) + 1);
    }

    sithAIUtil_SetWeaponFireFlags(weaponNum, &flags);

    if ( pInstinct->fltArg[SITHAI_ALTERNATEFIRE_ARG_LEADCHANCE] != 0.0f && SITH_RANDF() <= pInstinct->fltArg[SITHAI_ALTERNATEFIRE_ARG_LEADCHANCE] )
    {
        flags |= SITHAIUTIL_FIRE_LEAD;
    }

    int burstCount = pInstinct->intArg[SITHAI_ALTERNATEFIRE_ARG_BURST];
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

    if ( pInstinct->intArg[SITHAI_ALTERNATEFIRE_ARG_STOPONFIRE] )
    {
        flags |= SITHAIUTIL_FIRE_STOP_ON_FIRE;
    }

    if ( sithAIUtil_AIFire(pLocal, pInstinct->fltArg[SITHAI_ALTERNATEFIRE_ARG_MINDIST], pInstinct->fltArg[SITHAI_ALTERNATEFIRE_ARG_MAXDIST], pInstinct->fltArg[SITHAI_ALTERNATEFIRE_ARG_FIREDOT], pInstinct->fltArg[SITHAI_ALTERNATEFIRE_ARG_AIMERROR], weaponNum, flags, burstCount) )
    {
        float currentfireDelay = (SITH_RANDF() * 0.4f - 0.2f + 1.0f) * pInstinct->fltArg[SITHAI_ALTERNATEFIRE_ARG_FIREDELAY];
        pState->msecNextUpdate = sithTime_g_msecGameTime + (uint32_t)currentfireDelay;
    }
    else
    {
        if ( pLocal->targetSightState == SITHAI_SIGHT_OUTOFFOV )
        {
            sithAIMove_AISetLookThing(pLocal, pTarget);
        }
        else if ( pLocal->targetSightState == SITHAI_SIGHT_VISIBLE
               && pTarget->moveType == SITH_MT_PHYSICS
               && !rdVector_IsZero3(&pTarget->moveInfo.physics.velocity) )
        {
            rdVector3 lookPos;
            rdVector_ScaleAdd3(&lookPos, &pTarget->moveInfo.physics.velocity, 0.5f, &pTarget->pos);
            sithAIMove_AISetLookPosEyeLevel(pLocal, &lookPos);
        }

        pState->msecNextUpdate = sithTime_g_msecGameTime + pInstinct->intArg[SITHAI_ALTERNATEFIRE_ARG_RETRYDELAY];
    }

    return 0;
}

int J3DAPI sithAIInstinct_CircleStrafe(SithAIControlBlock* pLocal, SithAIInstinct* pInstinct, SithAIInstinctState* pState, SithAIEventType event, SithThing* pThing)
{
    float rate = 1.25f;
    int result = 0;
    SITH_ASSERTREL(pLocal);

    pState->msecNextUpdate = sithTime_g_msecGameTime + pInstinct->intArg[SITHAI_CIRCLESTRAFE_ARG_INTERVAL];
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

    if ( pInstinct->fltArg[SITHAI_CIRCLESTRAFE_ARG_WAITFIRE] != 0.0f && sithPuppet_IsAnyModeOnTrack(pLocal->pOwner, SITHPUPPETSUBMODE_FIRE, SITHPUPPETSUBMODE_FIRE4) )
    {
        pState->msecNextUpdate = sithTime_g_msecGameTime + 500;
        return 0;
    }

    if ( pInstinct->fltArg[SITHAI_CIRCLESTRAFE_ARG_SPEED] != 0.0f )
    {
        rate = pInstinct->fltArg[SITHAI_CIRCLESTRAFE_ARG_SPEED];
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
                if ( pThing && pThing->type != SITH_THING_PLAYER )
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

    if ( (pLocal->mode & SITHAI_MODE_CIRCLESTRAFING) != 0 && !pInstinct->intArg[SITHAI_CIRCLESTRAFE_ARG_LOOKATDEST] )
    {
        sithAIMove_AISetLookThing(pLocal, pLocal->goalThing);
    }

    int bRandom = pInstinct->intArg[SITHAI_CIRCLESTRAFE_ARG_RANDOM];
    sithAIUtil_UpdateGoalSight(pLocal);
    if ( pInstinct->fltArg[SITHAI_CIRCLESTRAFE_ARG_MAXDIST] < pLocal->targetDistance || pLocal->targetSightState2 != SITHAI_SIGHT_VISIBLE )
    {
        goto stop;
    }

    rdVector3 strafeDir;
    rdVector_Scale3(&strafeDir, &pLocal->vecUnknown0, -pLocal->targetDistance);
    if ( !bRandom && rdVector_Dot3(&pLocal->goalThing->orient.lvec, &strafeDir) < 0.0f )
    {
        goto stop;
    }

    if ( pState->aParams[SITHAI_CIRCLESTRAFE_PARAM_NUMSTEPS]++ > 4.0f )
    {
        goto stop;
    }

    if ( pState->aParams[SITHAI_CIRCLESTRAFE_PARAM_SIDE] == 0.0f || bRandom )
    {
        if ( SITH_RANDF() < 0.5f )
        {
            pState->aParams[SITHAI_CIRCLESTRAFE_PARAM_SIDE] = -1.0f;
        }
        else
        {
            pState->aParams[SITHAI_CIRCLESTRAFE_PARAM_SIDE] = 1.0f;
        }
    }

    float side = pState->aParams[SITHAI_CIRCLESTRAFE_PARAM_SIDE];
    rdVector3 pyr;
    memset(&pyr, 0, sizeof(pyr));
    if ( !bRandom )
    {
        pyr.y = pInstinct->fltArg[SITHAI_CIRCLESTRAFE_ARG_ANGLE] * side;
    }
    else
    {
        pyr.y = (SITH_RANDF() + 0.5f) * side * pInstinct->fltArg[SITHAI_CIRCLESTRAFE_ARG_ANGLE];
    }

    {
        rdVector3 toGoal;

        rdVector_Sub3(&toGoal, &pLocal->goalThing->pos, &pLocal->pOwner->pos);
        rdVector_Normalize3Acc(&toGoal);
        rdVector_Rotate3(&strafeDir, &toGoal, &pyr);
    }

    rdVector3 nextPos;
    rdVector_ScaleAdd3(&nextPos, &strafeDir, pLocal->pOwner->collide.movesize * 3.0f, &pLocal->pOwner->pos);

    if ( pInstinct->fltArg[SITHAI_CIRCLESTRAFE_ARG_MINSTEP] != 0.0f )
    {
        rdVector3 pushDir;
        float pushDist;

        rdVector_Sub3(&pushDir, &pLocal->pOwner->pos, &nextPos);
        pushDist = rdVector_Normalize3Acc(&pushDir);
        if ( pushDist < pInstinct->fltArg[SITHAI_CIRCLESTRAFE_ARG_MINSTEP] )
        {
            pushDist = pInstinct->fltArg[SITHAI_CIRCLESTRAFE_ARG_MINSTEP];
            rdVector_ScaleAdd3(&nextPos, &pushDir, pushDist, &nextPos);
        }
    }

    float range;
    rdVector3 dirToTarget;
    if ( sithAIUtil_GetDistanceToTargetPos(pLocal->goalThing, &pLocal->goalThing->pos, &nextPos, -1.0f, pLocal->pClass->sightDistance, 0.0f, &dirToTarget, &range) == SITHAI_SIGHT_VISIBLE )
    {
        pLocal->mode |= SITHAI_MODE_CIRCLESTRAFING;
        sithAIMove_AISetMovePos(pLocal, &nextPos, rate);
        if ( !pInstinct->intArg[SITHAI_CIRCLESTRAFE_ARG_LOOKATDEST] )
        {
            sithAIMove_AISetLookThing(pLocal, pLocal->goalThing);
        }
        else
        {
            sithAIMove_AISetLookPosEyeLevel(pLocal, &nextPos);
        }

        pState->msecNextUpdate = sithTime_g_msecGameTime + pInstinct->intArg[SITHAI_CIRCLESTRAFE_ARG_MOVEINTERVAL];
    }
    else
    {
        pState->aParams[SITHAI_CIRCLESTRAFE_PARAM_SIDE] = -side;
        pState->aParams[SITHAI_CIRCLESTRAFE_PARAM_NUMSTEPS] = 0.0f;
        pLocal->mode &= ~SITHAI_MODE_CIRCLESTRAFING;
    }

    return result;

stop:
    sithAIMove_AISetLookThing(pLocal, pLocal->goalThing);
    pLocal->mode &= ~SITHAI_MODE_CIRCLESTRAFING;
    pState->aParams[SITHAI_CIRCLESTRAFE_PARAM_SIDE] = 0.0f;
    pState->aParams[SITHAI_CIRCLESTRAFE_PARAM_NUMSTEPS] = 0.0f;
    return result;
}

int J3DAPI sithAIInstinct_Dodge(SithAIControlBlock* pLocal, SithAIInstinct* pInstinct, SithAIInstinctState* pState, SithAIEventType event, SithThing* pThing)
{
    float moveSpeed = 1.5f;
    float nextCheck = -1.0f;
    float yaw = 0.0f;
    bool bEvaded = false;

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

    float minDodgeDist = pInstinct->fltArg[SITHAI_DODGE_ARG_MINDIST] + 0.03f;
    float maxStep      = pInstinct->fltArg[SITHAI_DODGE_ARG_MAXDIST] + 0.03f;
    float moveDist     = SITH_RANDF() * (maxStep - minDodgeDist) + minDodgeDist;
    float minTargetLen = pInstinct->fltArg[SITHAI_DODGE_ARG_MINTARGETDIST];
    float minDistVal   = pInstinct->fltArg[SITHAI_DODGE_ARG_PATHMINDIST];

    if ( pInstinct->fltArg[SITHAI_DODGE_ARG_SPEED] != 0.0f )
    {
        moveSpeed = pInstinct->fltArg[SITHAI_DODGE_ARG_SPEED];
    }

    if ( pInstinct->fltArg[SITHAI_DODGE_ARG_COOLDOWN] != 0.0f )
    {
        nextCheck = pState->aParams[SITHAI_DODGE_PARAM_LASTDODGETIME] + pInstinct->fltArg[SITHAI_DODGE_ARG_COOLDOWN];
    }

    rdVector3 toDest;
    SithAIUtilMkPointFlags moveFlags;
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

        if ( (pSource->thingInfo.weaponInfo.damageType & (SITH_DAMAGE_WHIP | SITH_DAMAGE_ELECTROWHIP)) != 0 )
        {
            return 0;
        }

        if ( (pSource->thingInfo.weaponInfo.flags & SITH_WF_AITARGETNODODGE) == 0 )
        {
            if ( nextCheck > sithTime_g_secGameTime )
            {
                return 0;
            }

            if ( SITH_RANDF() >= pInstinct->fltArg[SITHAI_DODGE_ARG_TARGETEDCHANCE] )
            {
                return 0;
            }
        }

        bEvaded = true;
        pFirer = sithThing_GetThingParent(pSource);
        if ( sithAIUtil_GetDistanceToTarget(pLocal->pOwner, &pLocal->pOwner->pos, pFirer, pLocal->pClass->fov, pLocal->pClass->sightDistance, 0.0f, &toDest, &targetGap) != SITHAI_SIGHT_VISIBLE )
        {
            return 0;
        }

        if ( targetGap < minTargetLen )
        {
            return 0;
        }

        if ( pInstinct->fltArg[SITHAI_DODGE_ARG_ROLLCHANCE] != 0.0f && SITH_RANDF() < pInstinct->fltArg[SITHAI_DODGE_ARG_ROLLCHANCE] && sithAIMove_AISpecialMove(pLocal, SITHACTORSPECIALMOVE_DIR_RANDOM | SITHACTORSPECIALMOVE_ROLL) )
        {
            goto dodged;
        }

        moveFlags = SITHAIUTIL_MKPOINT_RANDSIDE | SITHAIUTIL_MKPOINT_SIDEWAYS;
    }
    else if ( event == SITHAI_EVENT_HIT_SECTOR )
    {
        if ( nextCheck > sithTime_g_secGameTime )
        {
            return 0;
        }

        if ( SITH_RANDF() >= pInstinct->fltArg[SITHAI_DODGE_ARG_HITCHANCE] )
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
            moveFlags = SITHAIUTIL_MKPOINT_RANDSIDE | SITHAIUTIL_MKPOINT_AWAYFROMFLEE | SITHAIUTIL_MKPOINT_SIDEWAYS;
            moveDist = 1.0f;
            goto make_path;
        }

        if ( sithAIUtil_GetDistanceToTarget(pLocal->pOwner, &pLocal->pOwner->pos, pFirer, pLocal->pClass->fov, pLocal->pClass->sightDistance, 0.0f, &toDest, &targetGap) != SITHAI_SIGHT_VISIBLE )
        {
            return 0;
        }

        if ( targetGap < minTargetLen )
        {
            return 0;
        }

        yaw = 60.0f;
        moveFlags = SITHAIUTIL_MKPOINT_RANDSIDE | SITHAIUTIL_MKPOINT_YAW;
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

        if ( pSource->type == SITH_THING_WEAPON && (pSource->thingInfo.weaponInfo.damageType & SITH_DAMAGE_WHIP) != 0 )
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
            if ( sithAIUtil_GetDistanceToTarget(pLocal->pOwner, &pLocal->pOwner->pos, pFirer, pLocal->pClass->fov, 1.0f, 0.0f, &toDest, &targetGap) != SITHAI_SIGHT_VISIBLE )
            {
                return 0;
            }
        }
        else
        {
            float tmpVal = rdVector_Len3(&pFirer->moveInfo.physics.velocity) * 2.0f;
            tmpVal = J3DMIN(pLocal->pClass->sightDistance, tmpVal);
            tmpVal = J3DMAX(tmpVal, 1.0f);
            if ( sithAIUtil_GetDistanceToTarget(pLocal->pOwner, &pLocal->pOwner->pos, pFirer, -1.0f, tmpVal, 0.0f, &toDest, &targetGap) != SITHAI_SIGHT_VISIBLE )
            {
                return 0;
            }
        }

        bEvaded = true;
        if ( (pFirer->moveInfo.physics.flags & (SITH_PF_MINECAR | SITH_PF_RAFT | SITH_PF_JEEP | SITH_PF_UNKNOWN_8000000)) != 0 )
        {
            rdVector3 heading;

            sithAIUtil_GetXYHeadingVector(pSource, &heading);
            if ( rdVector_Dot3(&toDest, &heading) > -0.5f )
            {
                return 0;
            }

            moveDist  = 0.5f;
            moveFlags = SITHAIUTIL_MKPOINT_RANDSIDE | SITHAIUTIL_MKPOINT_SIDEWAYS;

            float chance = pInstinct->fltArg[SITHAI_DODGE_ARG_VEHICLECHANCE];
            if ( chance == 0.0f )
            {
                chance = 0.98f;
            }

            if ( rdVector_Len3(&pFirer->moveInfo.physics.velocity) > 1.4f )
            {
                chance -= 0.2f;
            }

            if ( SITH_RANDF() < chance )
            {
                moveFlags |= SITHAIUTIL_MKPOINT_AWAYFROMFLEE;
            }

            pLocal->pFleeFromThing = pFirer;
            goto make_path;
        }

        if ( nextCheck > sithTime_g_secGameTime )
        {
            return 0;
        }

        if ( SITH_RANDF() >= pInstinct->fltArg[SITHAI_DODGE_ARG_SOUNDCHANCE] )
        {
            return 0;
        }

        sithAIUtil_AIPlaySoundMode(pLocal, SITHSOUNDCLASS_FEAR);
        pLocal->pFleeFromThing = pFirer;
        yaw = 45.0f;
        moveFlags = SITHAIUTIL_MKPOINT_RANDSIDE | SITHAIUTIL_MKPOINT_RANDANGLE | SITHAIUTIL_MKPOINT_YAW;
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
    pState->aParams[SITHAI_DODGE_PARAM_LASTDODGETIME] = sithTime_g_secGameTime;
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
            if ( pState->aParams[SITHAI_HITANDRUN_PARAM_FLEEPENDING] == 1.0f )
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
            pState->aParams[SITHAI_HITANDRUN_PARAM_HASFIRED] = 1.0f;
            if ( SITH_RANDF() < pInstinct->fltArg[SITHAI_HITANDRUN_ARG_FIRECHANCE] )
            {
                pState->aParams[SITHAI_HITANDRUN_PARAM_FLEEPENDING] = 1.0f;
                pState->msecNextUpdate = sithTime_g_msecGameTime + 500;
            }
            return 0;

        default:
            return 0;
    }

    if ( (pLocal->mode & SITHAI_MODE_FLEEING) != 0 )
    {
        if ( pState->aParams[SITHAI_HITANDRUN_PARAM_FLEEING] == 1.0f )
        {
            sithAIUtil_AISetMode(pLocal, SITHAI_MODE_ATTACKING);
            pState->aParams[SITHAI_HITANDRUN_PARAM_FLEEING] = 0.0f;
        }

        return 0;
    }

    if ( (pLocal->mode & SITHAI_MODE_TARGETVISIBLE) != 0 && pState->aParams[SITHAI_HITANDRUN_PARAM_HASFIRED] != 0.0f && sithAIUtil_CanAttackTarget(pLocal->pTargetThing) )
    {
        float time = pState->aParams[SITHAI_HITANDRUN_PARAM_ATTACKELAPSED];
        time += 1000.0f;
        if ( time > pInstinct->fltArg[SITHAI_HITANDRUN_ARG_ATTACKTIME] )
        {
            sithAIUtil_AISetMode(pLocal, SITHAI_MODE_FLEEING);
            pLocal->pFleeFromThing = pLocal->pTargetThing;
            goto flee;
        }

        pState->aParams[SITHAI_HITANDRUN_PARAM_ATTACKELAPSED] = time;
        pState->msecNextUpdate = sithTime_g_msecGameTime + 1000;
    }
    else
    {
        pState->aParams[SITHAI_HITANDRUN_PARAM_ATTACKELAPSED] = 0.0f;
        pState->msecNextUpdate = sithTime_g_msecGameTime + 1000;
    }

    return 0;

flee:
    pState->aParams[SITHAI_HITANDRUN_PARAM_FLEEING] = 1.0f;
    pState->aParams[SITHAI_HITANDRUN_PARAM_ATTACKELAPSED] = 0.0f;
    pState->aParams[SITHAI_HITANDRUN_PARAM_FLEEPENDING] = 0.0f;
    pState->aParams[SITHAI_HITANDRUN_PARAM_HASFIRED] = 0.0f;
    pState->msecNextUpdate = sithTime_g_msecGameTime + (int)pInstinct->fltArg[SITHAI_HITANDRUN_ARG_FLEETIME];
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
        pState->msecNextUpdate = sithTime_g_msecGameTime + pInstinct->intArg[SITHAI_RETREAT_ARG_INTERVAL];
        return 0;
    }

    if ( (pLocal->mode & SITHAI_MODE_UNKNOWN_100000) != 0 )
    {
        pState->flags |= SITHAI_INSTINCT_DISABLED;
        return 0;
    }

    float healthRatio = pLocal->pOwner->thingInfo.actorInfo.health / pLocal->pOwner->thingInfo.actorInfo.maxHealth;
    if ( pInstinct->fltArg[SITHAI_RETREAT_ARG_MAXRETREATS] != 0.0f && pState->aParams[SITHAI_RETREAT_PARAM_NUMRETREATS] > pInstinct->fltArg[SITHAI_RETREAT_ARG_MAXRETREATS] )
    {
        return 0;
    }

    if ( healthRatio < pInstinct->fltArg[SITHAI_RETREAT_ARG_HEALTH] )
    {
        switch ( event )
        {
            case (SithAIEventType)0:
            case SITHAI_EVENT_HIT_SECTOR:
                if ( SITH_RANDF() < pInstinct->fltArg[SITHAI_RETREAT_ARG_CHANCE] )
                {
                    pState->aParams[SITHAI_RETREAT_PARAM_NUMRETREATS] = pState->aParams[SITHAI_RETREAT_PARAM_NUMRETREATS] + 1.0f;
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

    pState->msecNextUpdate = sithTime_g_msecGameTime + pInstinct->intArg[SITHAI_RETREAT_ARG_INTERVAL];
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

    if ( pInstinct->intArg[SITHAI_RANDOMMOVE_ARG_WAITFIRE] && sithPuppet_IsAnyModeOnTrack(pLocal->pOwner, SITHPUPPETSUBMODE_FIRE, SITHPUPPETSUBMODE_FIRE4) )
    {
        return 0;
    }

    sithAIUtil_UpdateTargetSight(pLocal, 0);

    rdVector3 direction;
    int delay;
    switch ( event )
    {
        case SITHAI_EVENT_FIRE:
            if ( !pInstinct->intArg[SITHAI_RANDOMMOVE_ARG_MOVEONFIRE] )
            {
                return 0;
            }
            break;

        case (SithAIEventType)0:
            delay = pInstinct->intArg[SITHAI_RANDOMMOVE_ARG_INTERVAL];
            delay += (int)((float)(uint32_t)delay * (SITH_RANDF() - 0.5f));
            pState->msecNextUpdate = sithTime_g_msecGameTime + delay;
            if ( pLocal->targetSightState != SITHAI_SIGHT_VISIBLE )
            {
                return 0;
            }
            break;

        case SITHAI_EVENT_GOAL_REACHED:
            if ( pLocal->targetSightState == SITHAI_SIGHT_VISIBLE && pLocal->pTargetThing )
            {
                sithAIMove_AISetLookThing(pLocal, pLocal->pTargetThing);
            }
            else if ( pState->aParams[SITHAI_RANDOMMOVE_PARAM_HASTARGET] != 0.0f )
            {
                rdVector_Set3(&direction, pState->aParams[SITHAI_RANDOMMOVE_PARAM_TARGETX], pState->aParams[SITHAI_RANDOMMOVE_PARAM_TARGETY], pState->aParams[SITHAI_RANDOMMOVE_PARAM_TARGETZ]);
                sithAIMove_AISetLookPosEyeLevel(pLocal, &direction);
            }

            pState->aParams[SITHAI_RANDOMMOVE_PARAM_HASTARGET] = 0.0f;
            return 0;

        default:
            return 0;
    }

    float minDist  = pInstinct->fltArg[SITHAI_RANDOMMOVE_ARG_MINDIST] + 0.03f;
    float maxLen   = pInstinct->fltArg[SITHAI_RANDOMMOVE_ARG_MAXDIST] + 0.03f;
    float moveDist = SITH_RANDF() * (maxLen - minDist) + minDist;

    float lowSpeed  = pInstinct->fltArg[SITHAI_RANDOMMOVE_ARG_MINSPEED];
    float maxSpeed  = pInstinct->fltArg[SITHAI_RANDOMMOVE_ARG_MAXSPEED];
    float moveSpeed = SITH_RANDF() * (maxSpeed - lowSpeed) + lowSpeed;

    rdVector_Neg3(&direction, &pLocal->toTarget);

    int pathOpts = (int)SITHAIUTIL_PATHFLAG_TARGETVISIBLE | SITHAIUTIL_MKPOINT_RANDSIDE | SITHAIUTIL_MKPOINT_YAW;
    if ( pInstinct->intArg[SITHAI_RANDOMMOVE_ARG_ALLOW3D] )
    {
        pathOpts |= SITHAIUTIL_MKPOINT_PITCH;
    }

    float angleDeg = pInstinct->fltArg[SITHAI_RANDOMMOVE_ARG_ANGLE];
    if ( angleDeg < 0.0f )
    {
        pathOpts |= SITHAIUTIL_MKPOINT_RANDANGLE;
    }

    rdVector3 goalPos;
    if ( sithAIUtil_MakePathPos(pLocal, &pLocal->pOwner->pos, minDist, moveDist, angleDeg, pathOpts, &direction, &goalPos) )
    {
        if ( pLocal->pTargetThing )
        {
            pState->aParams[SITHAI_RANDOMMOVE_PARAM_TARGETX] = pLocal->pTargetThing->pos.x;
            pState->aParams[SITHAI_RANDOMMOVE_PARAM_TARGETY] = pLocal->pTargetThing->pos.y;
            pState->aParams[SITHAI_RANDOMMOVE_PARAM_TARGETZ] = pLocal->pTargetThing->pos.z;
            pState->aParams[SITHAI_RANDOMMOVE_PARAM_HASTARGET] = 1.0f;
        }
        else
        {
            pState->aParams[SITHAI_RANDOMMOVE_PARAM_HASTARGET] = 0.0f;
        }

        if ( pInstinct->intArg[SITHAI_RANDOMMOVE_ARG_ALWAYSLOOK] || moveDist >= (maxLen - minDist) / 2.0f + minDist )
        {
            sithAIMove_AISetLookPosEyeLevel(pLocal, &goalPos);
        }

        sithAIMove_AISetMoveTargetPos(pLocal, &goalPos, moveSpeed);
    }

    return 0;
}

int J3DAPI sithAIInstinct_HumanCombatMove(SithAIControlBlock* pLocal, SithAIInstinct* pInstinct, SithAIInstinctState* pState, SithAIEventType event, void* pObject)
{
    SITH_ASSERTREL(pLocal && pLocal->pOwner);
    SithThing* pOwner = pLocal->pOwner;

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

    sithAIUtil_UpdateTargetSight(pLocal, 0);
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

                float enemyRange;
                rdVector3 dirNorm;
                SithAISightState sightState = sithAIUtil_GetDistanceToTargetPos(pLocal->pOwner, &pLocal->pOwner->pos, &pLocal->vecUnknown5, -1.0f, pLocal->pClass->sightDistance, 0.0f, &dirNorm, &enemyRange);
                if ( healthPct <= pInstinct->fltArg[SITHAI_HUMANCOMBATMOVE_ARG_FLEEHEALTH]
                  && SITH_RANDF() < pInstinct->fltArg[SITHAI_HUMANCOMBATMOVE_ARG_CHASECHANCE]
                  && sightState != SITHAI_SIGHT_VISIBLE )
                {
                    sithAIMove_AISetLookPosEyeLevel(pLocal, &pLocal->vecUnknown);
                    sithAIMove_AISetMovePos(pLocal, &pLocal->vecUnknown, pInstinct->fltArg[SITHAI_HUMANCOMBATMOVE_ARG_FASTSPEED]);

                    pLocal->mode |= SITHAI_MODE_UNKNOWN_1000000;
                    pLocal->mode &= ~SITHAI_MODE_CHASE_GOAL;
                    pState->aParams[SITHAI_HUMANCOMBATMOVE_PARAM_CHASESTATE] = 3.0f;
                    return 1;
                }

                if ( sightState != SITHAI_SIGHT_VISIBLE )
                {
                    sithAIMove_AISetLookPosEyeLevel(pLocal, &pLocal->vecUnknown);
                    sithAIMove_AISetMovePos(pLocal, &pLocal->vecUnknown, pInstinct->fltArg[SITHAI_HUMANCOMBATMOVE_ARG_FASTSPEED]);
                    return 1;
                }
            }
            break;

        case SITHAI_EVENT_HIT_SECTOR:
            if ( healthPct <= pInstinct->fltArg[SITHAI_HUMANCOMBATMOVE_ARG_FLEEHEALTH] )
            {
                if ( SITH_RANDF() < pInstinct->fltArg[SITHAI_HUMANCOMBATMOVE_ARG_FLEECHANCE] )
                {
                    sithAIUtil_AIPlaySoundMode(pLocal, SITHSOUNDCLASS_FLEE);
                    sithAIUtil_AISetMode(pLocal, SITHAI_MODE_FLEEING);
                    pLocal->pFleeFromThing = pLocal->pTargetThing;
                    return 0;
                }

                if ( SITH_RANDF() < pInstinct->fltArg[SITHAI_HUMANCOMBATMOVE_ARG_STRAFECHANCE] && sithAI_HasInstinct(pLocal, "circlestrafe") )
                {
                    sithAI_EnableInstinct(pLocal, "circlestrafe", 1);
                    pLocal->submode |= SITHAI_SUBMODE_UNKNOWN_400000;
                }

                if ( SITH_RANDF() < pInstinct->fltArg[SITHAI_HUMANCOMBATMOVE_ARG_STANDFIRECHANCE] )
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
        {
            if ( pLocal->targetSightState != SITHAI_SIGHT_VISIBLE )
            {
                return 0;
            }

            bool bCheckPath = false;
            if ( sithAIUtil_IsLineOfFireClear(pLocal, &pLocal->pOwner->pos) )
            {
                bCheckPath = true;
                if ( (pLocal->pTargetThing->moveInfo.physics.flags & (SITH_PF_MINECAR | SITH_PF_RAFT | SITH_PF_JEEP | SITH_PF_UNKNOWN_8000000)) == 0 && sithAIUtil_IsThingMoving(pLocal->pTargetThing) )
                {
                    pState->aParams[SITHAI_HUMANCOMBATMOVE_PARAM_MOVECHANCE] += 0.2f;
                }

                if ( pState->aParams[SITHAI_HUMANCOMBATMOVE_PARAM_NEXTMOVETIME] > (float)sithTime_g_msecGameTime )
                {
                    return 0;
                }

                float arc = pInstinct->fltArg[SITHAI_HUMANCOMBATMOVE_ARG_MOVEDELAY];
                arc       = (SITH_RANDF() - 0.5f) * arc + arc;

                pState->aParams[SITHAI_HUMANCOMBATMOVE_PARAM_NEXTMOVETIME] = sithTime_g_msecGameTime + arc;
                if ( SITH_RANDF() > pState->aParams[SITHAI_HUMANCOMBATMOVE_PARAM_MOVECHANCE] )
                {
                    pState->aParams[SITHAI_HUMANCOMBATMOVE_PARAM_MOVECHANCE] += 0.05f;
                    return 0;
                }
            }

            float minDist  = pInstinct->fltArg[SITHAI_HUMANCOMBATMOVE_ARG_MINDIST] + 0.03f;
            float maxRange = pInstinct->fltArg[SITHAI_HUMANCOMBATMOVE_ARG_MAXDIST] + 0.03f;
            float moveDist = SITH_RANDF() * (maxRange - minDist) + minDist;

            float minSpeed  = pInstinct->fltArg[SITHAI_HUMANCOMBATMOVE_ARG_MINSPEED];
            float fastSpeed = pInstinct->fltArg[SITHAI_HUMANCOMBATMOVE_ARG_FASTSPEED];

            unsigned int triedMoves = (unsigned int)pState->aParams[SITHAI_HUMANCOMBATMOVE_PARAM_TRIEDMOVES];

            bool bAimAtDest = true;
            unsigned int curMoveType;
            float curMoveSpeed;
            float angle;
            SithAIUtilMkPointFlags pathFlags;
            rdVector3 dirNorm;
            rdVector3 targetPos;
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

                pState->aParams[SITHAI_HUMANCOMBATMOVE_PARAM_TRIEDMOVES] = 0.0f;
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
            curMoveType  = 1;
            curMoveSpeed = fastSpeed;
            angle        = 0.0f;
            pathFlags    = SITHAIUTIL_MKPOINT_RANDSIDE | SITHAIUTIL_MKPOINT_SIDEWAYS;
            dirNorm      = pLocal->toTarget;
            goto make_path;

        move_back:
            curMoveType = 2;
            if ( !pInstinct->intArg[SITHAI_HUMANCOMBATMOVE_ARG_ALLOWSTRAFE] && pLocal->distance > pOwner->collide.movesize * 4.0f )
            {
                return 1;
            }

            bAimAtDest   = false;
            curMoveSpeed = minSpeed;
            angle        = 30.0f;
            pathFlags    = SITHAIUTIL_MKPOINT_RANDSIDE | SITHAIUTIL_MKPOINT_RANDANGLE | SITHAIUTIL_MKPOINT_YAW;
            rdVector_Neg3(&dirNorm, &pLocal->toTarget);
            goto make_path;

        move_side:
            curMoveType = 4;
            if ( !pInstinct->intArg[SITHAI_HUMANCOMBATMOVE_ARG_ALLOWSTRAFE] )
            {
                return 1;
            }

            curMoveSpeed = SITH_RANDF() * (fastSpeed - minSpeed) + minSpeed;
            angle        = 30.0f;
            pathFlags    = SITHAIUTIL_MKPOINT_RANDSIDE | SITHAIUTIL_MKPOINT_RANDANGLE | SITHAIUTIL_MKPOINT_YAW;
            dirNorm      = pLocal->toTarget;

        make_path:
            if ( sithAIUtil_MakePathPos(pLocal, &pLocal->pOwner->pos, minDist, moveDist, angle, pathFlags, &dirNorm, &targetPos) )
            {
                if ( !bCheckPath || sithAIUtil_IsLineOfFireClear(pLocal, &targetPos) )
                {
                    pState->aParams[SITHAI_HUMANCOMBATMOVE_PARAM_MOVECHANCE] = 0.0f;
                    pState->aParams[SITHAI_HUMANCOMBATMOVE_PARAM_TRIEDMOVES] = 0.0f;
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
            pState->aParams[SITHAI_HUMANCOMBATMOVE_PARAM_TRIEDMOVES] = (float)triedMoves;
            break;
        }

        case SITHAI_EVENT_GOAL_REACHED:
            if ( (pLocal->mode & SITHAI_MODE_UNKNOWN_1000000) != 0 )
            {
                if ( pState->aParams[SITHAI_HUMANCOMBATMOVE_PARAM_CHASESTATE] == 3.0f )
                {
                    if ( pLocal->targetSightState != SITHAI_SIGHT_VISIBLE )
                    {
                        pLocal->mode &= ~SITHAI_MODE_UNKNOWN_1000000;
                        return 0;
                    }

                    sithAIMove_AISetLookPosEyeLevel(pLocal, &pLocal->vecUnknown122);
                    sithAIUtil_AIPauseMove(pLocal, rand() % 1001 + 2000);
                    pState->aParams[SITHAI_HUMANCOMBATMOVE_PARAM_CHASESTATE] = 2.0f;
                }
                else if ( pState->aParams[SITHAI_HUMANCOMBATMOVE_PARAM_CHASESTATE] == 2.0f )
                {
                    if ( pLocal->targetSightState == SITHAI_SIGHT_VISIBLE )
                    {
                        pLocal->mode &= ~SITHAI_MODE_UNKNOWN_1000000;
                        return 0;
                    }

                    sithAIUtil_AIPauseMove(pLocal, rand() % 2001 + 1000);
                    pState->aParams[SITHAI_HUMANCOMBATMOVE_PARAM_CHASESTATE] = 3.0f;
                }

                rdVector3 dirToLook;
                rdVector_Sub3(&dirToLook, &pLocal->vecUnknown122, &pLocal->pOwner->pos);
                rdVector_Normalize3Acc(&dirToLook);

                rdVector3 dirToMove;
                rdVector_Sub3(&dirToMove, &pLocal->vecUnknown, &pLocal->pOwner->pos);
                rdVector_Normalize3Acc(&dirToMove);

                pOwner->thingInfo.actorInfo.stateChange.type = SITHACTORSTATECHANGE_ANIMMOVE;

                float angle = rdMath_DeltaAngleNormalized(&dirToMove, &dirToLook, &pOwner->orient.uvec);
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
                float enemyRange;
                rdVector3 dirNorm;
                SithAISightState sightState = sithAIUtil_GetDistanceToTargetPos(pLocal->pOwner, &pLocal->pOwner->pos, &pLocal->vecUnknown5, -1.0f, pLocal->pClass->sightDistance, 0.0f, &dirNorm, &enemyRange);
                if ( sightState != SITHAI_SIGHT_VISIBLE )
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
    pState->aParams[SITHAI_HUMANCOMBATMOVE_PARAM_MOVECHANCE] = 0.0f;
    pState->aParams[SITHAI_HUMANCOMBATMOVE_PARAM_TRIEDMOVES] = 0.0f;
    return 0;
}

int J3DAPI sithAIInstinct_Roam(SithAIControlBlock* pLocal, SithAIInstinct* pInstinct, SithAIInstinctState* pState, SithAIEventType event, SithThing* pThing)
{
    float moveSpeed = 0.75f;

    if ( (pLocal->mode & (SITHAI_MODE_ATTACKING | SITHAI_MODE_UNKNOWN_10 | SITHAI_MODE_ACTIVE | SITHAI_MODE_FLEEING)) != 0 )
    {
        return 0;
    }

    if ( event == SITHAI_EVENT_MODECHANGED && pThing )
    {
        SithAIMode prevMode = (SithAIMode)pThing;
        if ( (pLocal->mode & SITHAI_MODE_SEARCHING) != 0 && (prevMode & SITHAI_MODE_SEARCHING) == 0 )
        {
            pState->msecNextUpdate = sithTime_g_msecGameTime + pInstinct->intArg[SITHAI_ROAM_ARG_INTERVAL];
            return 0;
        }
    }

    if ( (pLocal->mode & SITHAI_MODE_TRAVERSEWPNTS) != 0 )
    {
        return 0;
    }

    float closestDist = pLocal->pOwner->collide.movesize + 0.03f;
    float homeRadius  = pInstinct->fltArg[SITHAI_ROAM_ARG_RADIUS];
    float roamDist    = SITH_RANDF() * (fabsf(homeRadius) - closestDist) + closestDist;

    if ( pInstinct->fltArg[SITHAI_ROAM_ARG_SPEED] != 0.0f )
    {
        moveSpeed = pInstinct->fltArg[SITHAI_ROAM_ARG_SPEED];
    }

    rdVector3 startPos = homeRadius > 0.0f ? pLocal->homePos : pLocal->pOwner->pos;
    int bAllow3D = pInstinct->fltArg[SITHAI_ROAM_ARG_ALLOW3D] != 0.0f;

    rdVector3 movePos;
    switch ( event )
    {
        case SITHAI_EVENT_TOUCHED:
        case SITHAI_EVENT_HIT_WALL:
        case SITHAI_EVENT_HIT_CLIFF:
        {
            rdVector3 direction;
            sithAIUtil_GetXYHeadingVector(pLocal->pOwner, &direction);
            rdVector_Neg3Acc(&direction);

            if ( sithAIUtil_MakePathPos(pLocal, &pLocal->pOwner->pos, closestDist, roamDist, 45.0f, SITHAIUTIL_MKPOINT_REFHOME | SITHAIUTIL_MKPOINT_RANDSIDE | SITHAIUTIL_MKPOINT_RANDANGLE | SITHAIUTIL_MKPOINT_YAW, &direction, &movePos) )
            {
                sithAIMove_StopAIMovement(pLocal);
                if ( sithAIMove_AISetMovePos(pLocal, &movePos, moveSpeed) )
                {
                    sithAIMove_AISetLookPosEyeLevel(pLocal, &movePos);
                    return 1;
                }
            }
            break;
        }

        case SITHAI_EVENT_GOAL_REACHED:
            if ( (pLocal->submode & (SITHAI_SUBMODE_SEMICONTINUOUSMOTION | SITHAI_SUBMODE_CONTINUOUSMOTION)) == 0 )
            {
                return 0;
            }
            // fallthrough

        case (SithAIEventType)0:
            pState->msecNextUpdate = sithTime_g_msecGameTime + pInstinct->intArg[SITHAI_ROAM_ARG_INTERVAL];
            if ( (pLocal->mode & SITHAI_MODE_INSTINCTUSEWPNTS) != 0 && sithAIUtil_AIMoveToNextWpnt(pLocal, moveSpeed, 0.0f, 1) )
            {
                return 0;
            }

            if ( homeRadius > 0.0f && rdVector_Dist3(&pLocal->pOwner->pos, &pLocal->homePos) > homeRadius )
            {
                sithAIMove_AISetLookPosEyeLevel(pLocal, &pLocal->homePos);
                sithAIMove_AISetMovePos(pLocal, &pLocal->homePos, moveSpeed);
                return 0;
            }
            break;

        default:
            return 0;
    }

    if ( sithAIUtil_MakeRoamPos(pLocal, &startPos, closestDist, roamDist, bAllow3D, 2, &movePos) && sithAIMove_AISetMovePos(pLocal, &movePos, moveSpeed) )
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

    switch ( event )
    {
        case SITHAI_EVENT_MODECHANGED:
        {
            SithAIMode prevMode = (SithAIMode)pObject;
            if ( (pLocal->mode & SITHAI_MODE_SEARCHING) != 0 && (prevMode & SITHAI_MODE_SEARCHING) == 0 )
            {
                sithAIMove_AISetMovePos(pLocal, &pLocal->homePos, 1.2f);
                sithAIMove_AISetLookPosEyeLevel(pLocal, &pLocal->homePos);
            }
            break;
        }

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
    float panicTime    = pInstinct->fltArg[SITHAI_FLEE_ARG_PANICTIME];
    float fleeRange    = pInstinct->fltArg[SITHAI_FLEE_ARG_RANGE];
    float minStep      = pLocal->pOwner->collide.movesize + 0.03f;
    float fleeingSpeed = 1.5f;
    bool bPanic        = false;

    if ( (pLocal->mode & SITHAI_MODE_FLEEING) == 0 )
    {
        if ( pState->aParams[SITHAI_FLEE_PARAM_STARTTIME] > 0.0f )
        {
            goto stop;
        }

        return 0;
    }

    pLocal->goalThing    = NULL;
    pLocal->pTargetThing = NULL;
    if ( pState->aParams[SITHAI_FLEE_PARAM_STARTTIME] == 0.0f )
    {
        pState->aParams[SITHAI_FLEE_PARAM_STARTTIME] = sithTime_g_secGameTime;
        bPanic = true;
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

    if ( sithTime_g_secGameTime > pState->aParams[SITHAI_FLEE_PARAM_STARTTIME] + panicTime )
    {
        goto stop;
    }

    if ( pInstinct->fltArg[SITHAI_FLEE_ARG_SPEED] != 0.0f )
    {
        fleeingSpeed = pInstinct->fltArg[SITHAI_FLEE_ARG_SPEED];
    }

    if ( pInstinct->intArg[SITHAI_FLEE_ARG_INTERVAL] )
    {
        pState->msecNextUpdate = sithTime_g_msecGameTime + pInstinct->intArg[SITHAI_FLEE_ARG_INTERVAL];
    }
    else
    {
        pState->msecNextUpdate = sithTime_g_msecGameTime + 5000;
    }

    rdVector3 direction;
    float threatRange;
    if ( sithAIUtil_GetDistanceToTarget(pLocal->pOwner, &pLocal->pOwner->pos, pLocal->pFleeFromThing, -1.0f, fleeRange, 0.0f, &direction, &threatRange) == SITHAI_SIGHT_VISIBLE || bPanic )
    {
        switch ( event )
        {
            case SITHAI_EVENT_MODECHANGED:
                if ( pThing )
                {
                    SithAIMode prevMode = (SithAIMode)pThing;
                    if ( (pLocal->mode & SITHAI_MODE_FLEEING) != 0 && (prevMode & SITHAI_MODE_FLEEING) == 0 )
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
                if ( pThing && pThing->type == SITH_THING_PLAYER )
                {
                    return 1;
                }
                break;
        }

        sithAIUtil_GetXYHeadingVector(pLocal->pOwner, &direction);
        pLocal->mode &= ~SITHAI_MODE_TRAVERSEWPNTS;
        goto flee;
    }
    else if ( (pLocal->submode & SITHAI_SUBMODE_CONTINUOUSWPNTMOTION) != 0 )
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

    pState->aParams[SITHAI_FLEE_PARAM_STARTTIME] = 0.0f;
    pLocal->pFleeFromThing                       = NULL;
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
    int pathOpts = (int)SITHAIUTIL_PATHFLAG_IGNOREPLAYER | SITHAIUTIL_MKPOINT_AWAYFROMFLEE | SITHAIUTIL_MKPOINT_YAW;
    if ( pInstinct->fltArg[SITHAI_FLEE_ARG_MINSTEP] > 0.0f )
    {
        minStep = pInstinct->fltArg[SITHAI_FLEE_ARG_MINSTEP];
    }

    float angle = pInstinct->fltArg[SITHAI_FLEE_ARG_ANGLE];
    if ( angle != 0.0f )
    {
        pathOpts |= SITHAIUTIL_MKPOINT_RANDSIDE;
        if ( angle < 0.0f )
        {
            pathOpts |= SITHAIUTIL_MKPOINT_RANDANGLE;
        }
    }

    rdVector3 endPos;
    if ( sithAIUtil_MakePathPos(pLocal, &pLocal->pOwner->pos, minStep, fleeRange, angle, pathOpts, &direction, &endPos) && sithAIMove_AISetMovePos(pLocal, &endPos, fleeingSpeed) )
    {
        sithAIMove_AISetLookPosEyeLevel(pLocal, &endPos);
        return 1;
    }

    pathOpts = (int)SITHAIUTIL_PATHFLAG_IGNOREPLAYER | SITHAIUTIL_MKPOINT_RANDSIDE | SITHAIUTIL_MKPOINT_AWAYFROMFLEE | SITHAIUTIL_MKPOINT_SIDEWAYS;
    if ( sithAIUtil_MakePathPos(pLocal, &pLocal->pOwner->pos, minStep, fleeRange, angle, pathOpts, &direction, &endPos) && sithAIMove_AISetMovePos(pLocal, &endPos, fleeingSpeed) )
    {
        sithAIMove_AISetLookPosEyeLevel(pLocal, &endPos);
        return 1;
    }

    goto stop;
}

int J3DAPI sithAIInstinct_BasicFallow(SithAIControlBlock* pLocal, SithAIInstinct* pInstinct, SithAIInstinctState* pState, SithAIEventType event, void* pObject)
{
    float moveSpeed = 1.0f;
    float length    = 0.0f;

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
        if ( pState->aParams[SITHAI_BASICFOLLOW_PARAM_NUMBLOCKED] > 3.0f )
        {
            pState->aParams[SITHAI_BASICFOLLOW_PARAM_NUMBLOCKED] = 0.0f;
            pState->aParams[SITHAI_BASICFOLLOW_PARAM_HITFLAG]    = 0.0f;

            pLocal->vecUnknown3 = pLocal->movePos;
            rdVector_Neg3Acc(&heading);

            gap = pFoe->collide.movesize * 4.0f;
            rdVector_ScaleAdd3(&movePos, &heading, gap, &pFoe->pos);
            if ( sithAIUtil_SweepMoveToPathPos(pLocal, &movePos, 0.0f, 60.0f, 30.0f, SITHAIUTIL_MKPOINT_TOWARDGOAL) )
            {
                pLocal->submode |= SITHAI_SUBMODE_UNKNOWN_8;
                return 1;
            }

            return 0;
        }
    }

    if ( (event & (SITHAI_EVENT_HIT_THING | SITHAI_EVENT_HIT_CLIFF | SITHAI_EVENT_HIT_WALL | SITHAI_EVENT_TOUCHED)) != 0 )
    {
        pState->aParams[SITHAI_BASICFOLLOW_PARAM_HITFLAG] = 1.0f;
    }

    if ( pInstinct->intArg[SITHAI_BASICFOLLOW_ARG_WAITFIRE] && sithPuppet_IsAnyModeOnTrack(pFoe, SITHPUPPETSUBMODE_FIRE, SITHPUPPETSUBMODE_FIRE4) )
    {
        pState->msecNextUpdate = sithTime_g_msecGameTime + 500;
        return 0;
    }

    if ( !pLocal->goalThing )
    {
        return 0;
    }

    SithThing* pGoal = pLocal->goalThing;
    sithAIUtil_UpdateGoalSight(pLocal);

    rdVector3 toDir;
    switch ( event )
    {
        case SITHAI_EVENT_HIT_CLIFF:
            return 0;

        case SITHAI_EVENT_TOUCHED:
        case SITHAI_EVENT_HIT_THING:
        {
            SithThing* pHitThing = (SithThing*)pObject;
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

            pState->aParams[SITHAI_BASICFOLLOW_PARAM_NUMBLOCKED] += 1.0f;
            pLocal->submode |= SITHAI_SUBMODE_UNKNOWN_4;
            if ( sithAIUtil_SweepMoveToPathPos(pLocal, &pLocal->movePos, 45.0f, 135.0f, 45.0f, SITHAIUTIL_MKPOINT_TOWARDGOAL) )
            {
                return 1;
            }
            return 0;
        }

        case SITHAI_EVENT_HIT_WALL:
        {
            SithSurface* pSurface = (SithSurface*)pObject;
            if ( !pSurface )
            {
                return 0;
            }

            toDir   = pSurface->face.normal;
            toDir.z = 0.0f;
            toDir.x = stdMath_ClipNearZero(toDir.x);
            toDir.y = stdMath_ClipNearZero(toDir.y);
            toDir.z = stdMath_ClipNearZero(toDir.z);
            rdVector_Normalize3Acc(&toDir);

            if ( pLocal->targetSightState2 == SITHAI_SIGHT_VISIBLE )
            {
                rdVector3 targetVec = {
                    .x = stdMath_ClipNearZero(pLocal->vecUnknown0.x),
                    .y = stdMath_ClipNearZero(pLocal->vecUnknown0.y),
                    .z = 0.0f
                };
                rdVector_Normalize3Acc(&targetVec);

                float goalDot = heading.x * targetVec.x + heading.y * targetVec.y;
                float wallDot = heading.x * toDir.x + heading.y * toDir.y;
                if ( goalDot > 0.701f && wallDot > -0.966f )
                {
                    return 1;
                }
            }

            if ( pLocal->pClass->rank < 0.5f && SITH_RANDF() > pLocal->pClass->rank )
            {
                return 0;
            }

            pState->aParams[SITHAI_BASICFOLLOW_PARAM_NUMBLOCKED] += 1.0f;
            pLocal->submode |= SITHAI_SUBMODE_UNKNOWN_4;

            float pathDistance;
            if ( pLocal->targetSightState2 == SITHAI_SIGHT_VISIBLE )
            {
                pathDistance = pFoe->collide.movesize;
                gap          = pFoe->collide.movesize * 2.0f;

                SithAIUtilPathResult pathCheck = sithAIUtil_CheckPathToPos(pLocal, pFoe->pInSector, &pFoe->pos, SITHAIUTIL_PATHFLAG_GOALVISIBLE | SITHAIUTIL_PATHFLAG_SIDES | SITHAIUTIL_PATHFLAG_PREFERGOAL, pathDistance, gap, 0.0f, &toDir, &movePos);
                if ( (pathCheck & SITHAIUTIL_PATH_WORLD) == 0 )
                {
                    sithAIMove_AISetLookPosEyeLevel(pLocal, &movePos);
                    sithAIMove_AISetMoveTargetPos(pLocal, &movePos, 1.0f);
                    return 1;
                }
            }

            pathDistance = J3DMAX(pFoe->collide.movesize, 0.089999996f);
            gap          = SITH_RANDF() * 0.1f + pathDistance;
            if ( sithAIUtil_MakePathPos(pLocal, &pFoe->pos, pathDistance, gap, 45.0f, SITHAIUTIL_MKPOINT_RANDSIDE | SITHAIUTIL_MKPOINT_TOWARDGOAL | SITHAIUTIL_MKPOINT_YAW, &toDir, &movePos) )
            {
                sithAIMove_StopAIMovement(pLocal);
                sithAIMove_AISetLookPosEyeLevel(pLocal, &movePos);
                sithAIMove_AISetMoveTargetPos(pLocal, &movePos, moveSpeed);
                return 1;
            }

            return 0;
        }

        case (SithAIEventType)0:
            if ( pInstinct->intArg[SITHAI_BASICFOLLOW_ARG_INTERVAL] )
            {
                pState->msecNextUpdate = sithTime_g_msecGameTime + pInstinct->intArg[SITHAI_BASICFOLLOW_ARG_INTERVAL];
            }
            break;

        case SITHAI_EVENT_GOAL_UNREACHABLE:
            return 0;

        case SITHAI_EVENT_GOAL_REACHED:
            pState->aParams[SITHAI_BASICFOLLOW_PARAM_NUMBLOCKED] = 0.0f;
            if ( pState->aParams[SITHAI_BASICFOLLOW_PARAM_HITFLAG] == 1.0f )
            {
                pState->aParams[SITHAI_BASICFOLLOW_PARAM_HITFLAG] = 0.0f;
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
        moveSpeed     = 2.0f;
        minDist       = pInstinct->fltArg[SITHAI_BASICFOLLOW_ARG_MINDIST];
        reach         = J3DMIN(pFoe->collide.movesize * 3.0f, minDist);
        idealDistance = 0.0f;
    }
    else
    {
        minDist       = pInstinct->fltArg[SITHAI_BASICFOLLOW_ARG_MINDIST];
        reach         = pInstinct->fltArg[SITHAI_BASICFOLLOW_ARG_MAXDIST];
        idealDistance = pInstinct->fltArg[SITHAI_BASICFOLLOW_ARG_IDEALDIST];
        if ( sithWeapon_HasWeaponSelected(pFoe) )
        {
            reach         = sithWeapon_GetWeaponMaxAimDistance(pFoe->thingInfo.actorInfo.weaponInfo.curWeaponID);
            reach        -= 0.1f;
            idealDistance = 0.0f;
        }

        if ( (pLocal->submode & SITHAI_SUBMODE_UNKNOWN_400000) != 0 )
        {
            minDist       *= 0.5f;
            reach         *= 0.5f;
            idealDistance *= 0.5f;
        }
    }

    if ( (pLocal->submode & SITHAI_SUBMODE_USEMATCHVELOCITY) != 0 && !rdVector_IsZero3(&pLocal->goalThing->moveInfo.physics.velocity) )
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

    if ( pLocal->targetSightState2 == SITHAI_SIGHT_VISIBLE || pLocal->targetSightState2 == SITHAI_SIGHT_OUTOFFOV )
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

        SithAISightState losCheck = sithAIUtil_GetDistanceToTargetPos(pGoal, &pGoal->pos, &movePos, -1.0f, pLocal->pClass->sightDistance, 0.0f, &toDir, &gap);
        if ( pInstinct->fltArg[SITHAI_BASICFOLLOW_ARG_IGNORELOS] != 0.0f || losCheck == SITHAI_SIGHT_VISIBLE )
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
        sithAIUtil_UpdateGoalSight(pLocal);
        sithAIUtil_AIGetMovePos(pLocal, SITHAIUTIL_MKPOINT_TOWARDGOAL, &movePos);
    }
    else
    {
        sithAIUtil_AIGetMovePos(pLocal, SITHAIUTIL_MKPOINT_TOWARDMOVEPOS, &movePos);
    }

    float heightDiff = movePos.z - pThing->pos.z;
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
            if ( !pSurface )
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

            if ( !pSurface )
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

    return sithAIInstinct_BasicFallow(pLocal, pInstinct, pState, event, pSurface);

crawl:
    if ( (pLocal->submode & SITHAI_SUBMODE_UNKNOWN_4) == 0 && pSurface )
    {
        float dot = rdVector_Dot3(&pThing->orient.lvec, &pSurface->face.normal);
        if ( dot >= 0.0f )
        {
            return 1;
        }

        pLocal->submode |= SITHAI_SUBMODE_UNKNOWN_4;
        pLocal->vecUnknown3 = pSurface->face.normal;
        if ( sithAIMove_AISpecialMove(pLocal, SITHACTORSPECIALMOVE_MOUNTWALL) )
        {
            return 1;
        }

        sithThing_AttachThingToSurface(pThing, pSurface, 1);
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
        fudge = pQuetz->posFudge;
    }

    rdVector_ScaleAdd3(pOutPoint, pDirection, distance, &pThing->pos);

    rdVector3 beginPos = pThing->pos;
    beginPos.z = 0.3f - sithPhysics_GetThingHeight(pThing) + 0.002f - fudge + beginPos.z;

    rdVector3 endPos = beginPos;

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
    if ( !rdVector_IsZero3(&delta) )
    {
        SITHLOG_ERROR("_SnakeMungeTestCheck - no LOS to start point");
        return 0;
    }

    // TODO: [BUG] The original never initializes the path flags it ORs the flags into (the debug build reads an uninitialized local).
    //       The retail build leaves the bit pattern of the distance argument in it, which is emulated here.
    SithAIUtilPathFlags pathFlags;
    memcpy(&pathFlags, &distance, sizeof(pathFlags));
    pathFlags |= SITHAIUTIL_PATHFLAG_IGNOREPLAYER | SITHAIUTIL_PATHFLAG_IGNORETHINGS | SITHAIUTIL_PATHFLAG_GOALVISIBLE | SITHAIUTIL_PATHFLAG_STRAIGHT;

    rdVector3 curHitPos;
    SithAIUtilPathResult result = sithAIUtil_CheckPathToPos(pLocal, pCurSector, &beginPos, pathFlags, 0.0f, distance + 0.05f, 0.3f, pDirection, &curHitPos);
    switch ( result )
    {
        case SITHAIUTIL_PATH_BLOCKED:
            return 0;

        case SITHAIUTIL_PATH_CLEAR:
            return 1;

        default:
        {
            float len = rdVector_Dist2((const rdVector2*)&beginPos, (const rdVector2*)&curHitPos);
            if ( len - 0.05f >= 0.2f )
            {
                rdVector_ScaleAdd3(pOutPoint, pDirection, len - 0.05f, &pThing->pos);
                return 1;
            }

            return 0;
        }
    }
}

void J3DAPI sithAIInstinct_SnakeFindMovePos(SithAIControlBlock* pLocal, rdVector3* moveToPosLeft, rdVector3* moveToPosRight)
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

    probe = pThing->orient.lvec;

    int side      = SITH_RANDF() - 0.5f < 0.0f ? -1 : 1;
    bool bFoundIt = false;
    float angle   = 0.0f;
    int retries   = 24;
    do
    {
        if ( sithAIInstinct_SnakeMungeTestCheck(pLocal, &probe, 0.3f, moveToPosLeft) )
        {
            bFoundIt = true;
        }

        if ( angle == 0.0f )
        {
            angle = side * 15.0f;
        }
        else if ( angle * side > 0.0f )
        {
            angle = -angle;
        }
        else
        {
            angle = side * 15.0f - angle;
        }

        rdVector3 angles = { .y = angle };
        rdVector_Rotate3(&probe, &pThing->orient.lvec, &angles);
    } while ( !bFoundIt && retries-- );

    // Note, the original also formats "Retires %d" into an unused local buffer when a position was found
    if ( !bFoundIt )
    {
        *moveToPosLeft = pThing->pos;
    }
}

signed int J3DAPI sithAIInstinct_SnakeFollow(SithAIControlBlock* pLocal, SithAIInstinct* pInstinct, SithAIInstinctState* pState, SithAIEventType event, SithThing* pThing)
{
    J3D_UNUSED(pInstinct);

    SITH_ASSERTREL(pLocal && pLocal->pClass && pLocal->pOwner);
    bool bAdvance = true;
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
                bAdvance = false;
            }
            break;

        case (SithAIEventType)0:
            if ( (pLocal->mode & SITHAI_MODE_MOVING) != 0 )
            {
                bAdvance = false;
            }
            break;

        case SITHAI_EVENT_GOAL_REACHED:
            break;

        default:
            return 0;
    }

    if ( !pLocal->goalThing )
    {
        pLocal->submode &= ~SITHAI_SUBMODE_CONTINUOUSMOTION;
        return 0;
    }

    pState->msecNextUpdate = sithTime_g_msecGameTime + 500;
    sithAIUtil_UpdateGoalSight(pLocal);
    sithAIUtil_UpdateTargetSight(pLocal, 0);

    rdVector3 dirVec;
    float distance;
    if ( pLocal->targetSightState2 == SITHAI_SIGHT_VISIBLE || pLocal->targetSightState2 == SITHAI_SIGHT_OUTOFFOV )
    {
        pState->aParams[SITHAI_SNAKEFOLLOW_PARAM_USELASTPOS] = 0.0f;
        dirVec = pLocal->vecUnknown0;
        distance = pLocal->targetDistance;
        pLocal->mode |= SITHAI_MODE_LOSTSIGHTOFGOAL;
    }
    else
    {
        pState->aParams[SITHAI_SNAKEFOLLOW_PARAM_USELASTPOS] = 1.0f;
        rdVector_Sub3(&dirVec, &pLocal->vecUnknown5, &pLocal->pOwner->pos);
        distance = rdVector_Normalize3Acc(&dirVec);
        pLocal->mode &= ~SITHAI_MODE_LOSTSIGHTOFGOAL;
    }

    if ( bAdvance )
    {
        float dotFwd   = rdVector_Dot3(&pLocal->pOwner->orient.lvec, &dirVec);
        float dotRight = rdVector_Dot3(&pLocal->pOwner->orient.rvec, &dirVec);
        float yaw      = fabsf(rdMath_DeltaAngleNormalized(&pLocal->pOwner->orient.lvec, &dirVec, &rdroid_g_zVector3));

        float farDist;
        float nearRange;
        if ( (pLocal->mode & SITHAI_MODE_ATTACKING) != 0 )
        {
            farDist   = 0.8f;
            nearRange = 0.3f;
        }
        else
        {
            farDist   = 0.65f;
            nearRange = sithAIInstinct_snakeFollowNearRange;
        }

        float turn1;
        float turnAngle2;
        float travelDist;
        float lookLen;
        if ( dotFwd <= -0.98f )
        {
            turn1      = (SITH_RANDF() - 0.5f < 0.0f ? -1 : 1) * 45.0f;
            turnAngle2 = -turn1;
            travelDist = 0.25f;
            lookLen    = 0.25f;
        }
        else if ( dotFwd < 0.5f )
        {
            turn1      = 45.0f;
            turnAngle2 = 60.0f;
            travelDist = 0.25f;
            lookLen    = 0.25f;
        }
        else if ( distance > farDist )
        {
            if ( sithAIInstinct_snakeLastDotFwd < 0.5f )
            {
                turn1      = yaw;
                turnAngle2 = 0.0f;
                travelDist = 0.2f;
                lookLen    = 0.2f;
            }
            else
            {
                turn1      = STDMATH_CLAMP(yaw + 30.0f, 0.0f, 60.0f);
                turnAngle2 = yaw - 10.0f;
                travelDist = 0.5f;
                lookLen    = 0.5f;
            }
        }
        else if ( distance > nearRange )
        {
            turn1      = yaw;
            turnAngle2 = yaw;
            if ( (pLocal->mode & SITHAI_MODE_ATTACKING) != 0 )
            {
                travelDist = STDMATH_CLAMP(distance - 0.1f, 0.1f, distance);
                lookLen    = STDMATH_CLAMP(distance - 0.45f, 0.1f, distance);
            }
            else
            {
                travelDist = distance;
                lookLen    = STDMATH_CLAMP(distance - 0.05f, 0.1f, distance);
            }
        }
        else
        {
            turn1      = -yaw;
            turnAngle2 = yaw * 2.0f;
            travelDist = distance + 0.1f;
            lookLen    = distance + 0.1f;
        }

        sithAIInstinct_snakeLastDotFwd = dotFwd;

        rdVector3 pyr1;
        rdVector3 pyr2;
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
        altPos.z   = pLocal->pOwner->pos.z;
        sithAIInstinct_SnakeFindMovePos(pLocal, &moveGoal, &altPos);
        sithAIMove_AISetMovePos(pLocal, &moveGoal, 2.0f);

        moveGoal = pLocal->movePos;
        moveGoal.z += pLocal->pOwner->thingInfo.actorInfo.eyeOffset.z;
        sithAIMove_AISetLookPos(pLocal, &moveGoal);

        rdVector_Sub3(&dirVec, &pLocal->movePos, &pOwner->pos);
        dotRight = rdVector_Dot3(&pLocal->pOwner->orient.rvec, &dirVec);
        sithAIInstinct_SnakePlaySlitherSound(pOwner, dotRight);

        if ( pLocal->pOwner->userblock.pQuetz )
        {
            pLocal->pOwner->userblock.pQuetz->bResetSegment = 1;
        }
    }

    pLocal->mode &= ~SITHAI_MODE_UNKNOWN_80;
    pLocal->submode |= SITHAI_SUBMODE_UNKNOWN_400 | SITHAI_SUBMODE_CONTINUOUSMOTION | SITHAI_SUBMODE_UNKNOWN_100;

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

void J3DAPI sithAIInstinct_SnakePlaySlitherSound(const SithThing* pThing, float slitherDir)
{
    if ( fabsf(slitherDir) < 0.1f )
    {
        slitherDir = 0.0f;
    }
    else if ( slitherDir > 0.0f )
    {
        slitherDir = 1.0f;
    }
    else
    {
        slitherDir = -1.0f;
    }

    if ( sithAIInstinct_snakeSlitherDir != slitherDir )
    {
        tSoundHandle hSnd;
        if ( slitherDir < 0.0f )
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

        sithAIInstinct_snakeSlitherDir = slitherDir;
    }
}

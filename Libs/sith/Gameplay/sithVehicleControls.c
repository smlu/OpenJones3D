#include "sithVehicleControls.h"
#include <j3dcore/j3dhook.h>

#include <rdroid/Math/rdMath.h>
#include <rdroid/Math/rdVector.h>

#include <sith/Cog/sithCog.h>

#include <sith/Devices/sithControl.h>

#include <sith/Engine/sithAnimate.h>
#include <sith/Engine/sithCollision.h>
#include <sith/Engine/sithCamera.h>
#include <sith/Engine/sithPhysics.h>
#include <sith/Engine/sithPuppet.h>

#include <sith/Gameplay/sithFX.h>
#include <sith/Gameplay/sithInventory.h>
#include <sith/Gameplay/sithPlayer.h>
#include <sith/Gameplay/sithPlayerActions.h>
#include <sith/Gameplay/sithPlayerControls.h>
#include <sith/Gameplay/sithTime.h>

#include <sith/World/sithSoundClass.h>
#include <sith/World/sithWorld.h>
#include <sith/RTI/symbols.h>

#include <std/General/stdMath.h>
#include <std/General/stdUtil.h>

#define _USE_MATH_DEFINES // For MSVC
#include <math.h>

typedef struct sSithWallCollisionInfo
{
    SithSurface* pSurf;
    float collisionDot;
    rdVector3 dir;
    float distance;
} SithWallCollisionInfo;

//
// MineCar vars
//
static SithMineCarControlsState sithVehicleControls_curMineCarState =
{
    .secElapsedStoppingTime   = 0.0f,
    .moveState                = SITHMINECARCONTROLS_MOVE_IDLE,
    .secDuckTime              = 0.0f,
    .curDuckPuppetTrackNum    = -1,
    .bCanDuck                 = 0,
    .leanState                = SITHMINECARCONTROLS_LEAN_NONE,
    .curPuppetTrack           = -1,
    .hBrakeSnd                = 0,
    .secElapsedBrakingTime    = 0.0f,
    .secUnboardingElapsedTime = 0.0f,
    .unboardState             = SITHMINECARCONTROLS_UNBOARD_NONE
};

static float sithVehicleControls_minecarTiltAngularVelThreshold   = 25.0f;
static float sithVehicleControls_minecarUntiltAngularVelThreshold = 20.0f;
static float sithVehicleControls_minecarTiltVelThreshold          = 0.69999999f;
static float sithVehicleControls_minecarUntiltVelThreshold        = 0.60000002f;
static float sithVehicleControls_sndVolumeBrake                   = 0.60000002f;

//
// Jeep vars
//
static float sithVehicleControls_jeepSurfDrag       = 2.0f;
static float sithVehicleControls_jeepStaticDrag     = 0.02f;
static float sithVehicleControls_jeepStopSurfDrag   = 2.0f;
static float sithVehicleControls_jeepStopStaticDrag = 0.050000001f;

static float sithVehicleControls_jeepMaxVelocity         = 1.0f;
static float sithVehicleControls_jeepBoostMaxVelocity    = 1.98f;
static float sithVehicleControls_jeepMaxBackwardVelocity = 0.34999999f;

static float sithVehicleControls_jeepForwardThrustDecay  = -2.0f;
static float sithVehicleControls_jeepBackwardThrustDecay = 2.0f;

static float sithVehicleControls_jeepNormalAccelRate = 0.34999999f;
static float sithVehicleControls_jeepBoostAccelRate  = 0.5f;

static float sithVehicleControls_jeepMaxRotationVelocity  = 150.0f;
static float sithVehicleControls_jeepRotationDecayRate    = 600.0f;
static float sithVehicleControls_jeepTurnRate             = 3.0f;
static float sithVehicleControls_jeepRotationVelThreshold = 0.40000001f;

// Jeep dimensions
static float sithVehicleControls_jeepHeight         = 0.18000001f;
static float sithVehicleControls_jeepMoveSize       = 0.16500001f;
static float sithVehicleControls_jeepRevSoundVolume = 0.5f;
static float sithVehicleControls_jeepWheelRadius    = 0.037500001f;
static float sithVehicleControls_jeepMinSpeed       = 0.2f;
static float sithVehicleControls_jeepMinAngularVel  = 3.0f;

// State variables
static float sithVehicleControls_secJeepStoppedTime;
static int sithVehicleControls_bJeepStopping;
static int sithVehicleControls_bJeepAccMoveSndFxPlayed;
static int sithVehicleControls_bJeepStartMoveSndFxPlayed;

//
// Raft vars
//
static SithRaftControlsState sithVehicleControls_curRaftState =
{
    .nextMoveStatus  = SITHPLAYERMOVE_RAFT_IDLE,
    .nextPuppetMode  = SITHPUPPETSUBMODE_STAND,
    .unboardPos      = { { 0.0f }, { 0.0f }, { 0.0f } },
    .unboardNorm     = { { 0.0f }, { 0.0f }, { 0.0f } },
    .moveSize        = 0.0f,
    .wakeTimer       = 0.0f,
    .bRowing         = 0,
    .secRowStartTime = 0.0f,
    .secUnboardTime  = 0.0f
};

static float sithVehicleControls_raftBoardingMomentum = 0.039999999f;
static float sithVehicleControls_raftBoardingRotation = 67.0f;

static float sithVehicleControls_raftUnbaordPosSearchRadius = 0.050000001f;
static float sithVehicleControls_raftUnboardPosOffset       = 0.050000001f;
static float sithVehicleControls_raftUnboardThresholdDist   = 0.039999999f;
static float sithVehicleControls_raftUnboardSearchRadius    = 0.2f;
static float sithVehicleControls_secRaftMaxUnboardingTime   = 5.0f; // 5 seconds

static float sithVehicleControls_raftDockingThrust        = 0.15000001f;
static float sithVehicleControls_raftDockingAngularVel    = 120.0f;
static float sithVehicleControls_raftMinDockingAngularVel = 5.0f;

static float sithVehicleControls_raftWakeInterval  = J3D_QOL_VALUE(0.01f, 0.050000001f);
static float sithVehicleControls_raftWakeThreshold = J3D_QOL_VALUE(0.1f, 0.050000001f);
static float sithVehicleControls_raftWakeMinSpeed  = J3D_QOL_VALUE(0.008f, 0.0f); // Added

static float sithVehicleControls_raftHeight         = 0.029999999f;
static float sithVehicleControls_raftSurfDrag       = 0.40000001f;
static float sithVehicleControls_raftMaxThrust      = 0.2f;
static float sithVehicleControls_raftBaseYawThrust  = 60.0f;
static float sithVehicleControls_raftTurnFactor     = 1.5f;
static float sithVehicleControls_raftMaxRotVelocity = 135.0f;
static float sithVehicleControls_raftStaticDrag     = 0.0f; // Fixed: Init to 0. OG had this as uninitialized global
static float sithVehicleControls_raftRotSpeed       = 0.0f; // Fixed: Init to 0. OG had this as uninitialized global

static int sithVehicleControls_raftRowSide = -1;


void J3DAPI sithVehicleControls_PlayRaftPuppetMode(SithThing* pThing, SithPuppetSubMode submode);
int J3DAPI sithVehicleControls_FindRaftUnboardPosition(SithThing* pThing, float moveDist, rdVector3* pOutPos, rdVector3* pOutPYR);
void J3DAPI sithVehicleControls_FindWallSurface(SithThing* pThing, SithWallCollisionInfo* pColInfo, float moveDist);
void J3DAPI sithVehicleControls_FadeJeepMoveSounds(SithThing* pThing);
bool J3DAPI sithVehicleControls_CanUnboardInDirection(SithThing* pThing, const rdVector3* pDir);
void J3DAPI sithVehicleControls_ExitRaft(SithThing* pRaftPlayer);
void J3DAPI sithVehicleControls_UpdateRaftPaddleSoundFx(SithThing* pThing);

void sithVehicleControls_InstallHooks(void)
{
    J3D_HOOKFUNC(sithVehicleControls_GetMineCarState);
    J3D_HOOKFUNC(sithVehicleControls_SetMineCarState);
    J3D_HOOKFUNC(sithVehicleControls_GetRaftState);
    J3D_HOOKFUNC(sithVehicleControls_SetRaftState);
    J3D_HOOKFUNC(sithVehicleControls_PuppetCallback);
    J3D_HOOKFUNC(sithVehicleControls_ProcessMineCarMove);
    J3D_HOOKFUNC(sithVehicleControls_ProcessJeepMove);
    J3D_HOOKFUNC(sithVehicleControls_PlayRaftPuppetMode);
    J3D_HOOKFUNC(sithVehicleControls_ProcessRaftMove);
    J3D_HOOKFUNC(sithVehicleControls_FindRaftUnboardPosition);
    J3D_HOOKFUNC(sithVehicleControls_FindWallSurface);
    J3D_HOOKFUNC(sithVehicleControls_FadeJeepMoveSounds);
    J3D_HOOKFUNC(sithVehicleControls_CanUnboardInDirection);
    J3D_HOOKFUNC(sithVehicleControls_ExitRaft);
    J3D_HOOKFUNC(sithVehicleControls_UpdateRaftPaddleSoundFx);
    J3D_HOOKFUNC(sithVehicleControls_StartBoardCutscene);
    J3D_HOOKFUNC(sithVehicleControls_EndBoardCutscene);
}

void sithVehicleControls_ResetGlobals(void)
{}

const SithMineCarControlsState* sithVehicleControls_GetMineCarState(void)
{
    return &sithVehicleControls_curMineCarState;
}

void J3DAPI sithVehicleControls_SetMineCarState(const SithMineCarControlsState* pState)
{
    J3D_UNUSED(pState);
    sithVehicleControls_curMineCarState = *pState;
}

const SithRaftControlsState* sithVehicleControls_GetRaftState(void)
{
    return &sithVehicleControls_curRaftState;
}

void J3DAPI sithVehicleControls_SetRaftState(const SithRaftControlsState* pState)
{
    J3D_UNUSED(pState);
    sithVehicleControls_curRaftState = *pState;
}

void J3DAPI sithVehicleControls_PuppetCallback(SithThing* pThing, int track, rdKeyMarkerType markerType)
{
    SithPhysicsInfo* pPhysics = &pThing->moveInfo.physics;
    if ( pThing->thingInfo.actorInfo.bControlsDisabled )
    {
        switch ( markerType )
        {
            case RDKEYMARKER_REACHRIGHTARMREST: // Start putout/picking-up folded raft 
                sithFX_CreateRaftInflateWaterFX(pThing, 0.02f);
                sithPuppet_DefaultCallback(pThing, track, RDKEYMARKER_REACHRIGHTARMREST);
                return;

            case RDKEYMARKER_PICKUP: // Raft start/end folding
                sithFX_CreateRaftInflateWaterFX(pThing, 0.039999999f);
                sithPuppet_DefaultCallback(pThing, track, RDKEYMARKER_PICKUP);
                return;

            case RDKEYMARKER_DROP: // Raft mid-folding
                sithFX_CreateRaftInflateWaterFX(pThing, 0.059999999f);
                sithPuppet_DefaultCallback(pThing, track, RDKEYMARKER_DROP);
                return;

            case RDKEYMARKER_MOVE: // Raft start unfolding/folding
                sithFX_CreateRaftInflateWaterFX(pThing, 0.079999998f);
                sithPuppet_DefaultCallback(pThing, track, RDKEYMARKER_MOVE);
                return;

            case RDKEYMARKER_INVENTORYPULL: // Raft end unfolding or deflated
                sithFX_CreateRaftInflateWaterFX(pThing, 0.1f);
                sithPuppet_DefaultCallback(pThing, track, RDKEYMARKER_INVENTORYPULL);
                return;

            case RDKEYMARKER_INVENTORYPUT: // Start inflating/deflating raft
                sithFX_CreateRaftInflateWaterFX(pThing, 0.12f);
                sithPuppet_DefaultCallback(pThing, track, RDKEYMARKER_INVENTORYPUT);
                return;

            case RDKEYMARKER_ATTACKFINISH:
                sithFX_CreateRaftInflateWaterFX(pThing, 0.14f);
                sithPuppet_DefaultCallback(pThing, track, RDKEYMARKER_ATTACKFINISH);
                return;

            case RDKEYMARKER_ROW:
                if ( (pThing->flags & SITH_TF_DYING) != 0 )
                {
                #ifndef J3D_QOL_IMPROVEMENTS // Raft splatter fx was fixed, enabling old logic only for backward compatibility
                    if ( (SithPhysicsWaterSurfaceType)pThing->userval == SITHPHYSICS_WATERSURFACE_ADJOIN )  // If raft is on underwater sector (set by sithPhysics_CheckWaterSurfaceAtPos).
                    #endif
                    {

                        sithFX_CreateRaftSplatterFX(pThing, /*bCreateSplash*/0);
                    }
                }
                else
                {
                    sithVehicleControls_curRaftState.bRowing         = 1;
                    sithVehicleControls_curRaftState.secRowStartTime = sithTime_g_secGameTime;
                    sithFX_CreateRaftPaddleWaterFX(pThing, sithTime_g_secGameTime);
                    sithVehicleControls_UpdateRaftPaddleSoundFx(pThing);
                }

                sithPuppet_DefaultCallback(pThing, track, markerType);
                return;

            case RDKEYMARKER_ROWFINISH:
                sithVehicleControls_curRaftState.bRowing = 0;
                sithPuppet_DefaultCallback(pThing, track, RDKEYMARKER_ROWFINISH);
                return;

            case RDKEYMARKER_CLIMB:
                if ( pThing->moveStatus == SITHPLAYERMOVE_RAFT_PADDLE_FORWARD_LEFT
                    || pThing->moveStatus == SITHPLAYERMOVE_RAFT_PADDLE_FORWARD_RIGHT
                    || pThing->moveStatus == SITHPLAYERMOVE_RAFT_PADDLE_BACK_LEFT
                    || pThing->moveStatus == SITHPLAYERMOVE_RAFT_PADDLE_BACK_RIGHT
                    || pThing->moveStatus == SITHPLAYERMOVE_RAFT_TURN_LEFT
                    || pThing->moveStatus == SITHPLAYERMOVE_RAFT_TURN_RIGHT )
                {
                    if ( pThing->moveStatus != sithVehicleControls_curRaftState.nextMoveStatus )
                    {
                        SithThingMoveStatus prevMoveStatus = pThing->moveStatus;    // redundant?
                        pThing->moveStatus = sithVehicleControls_curRaftState.nextMoveStatus;
                        sithVehicleControls_PlayRaftPuppetMode(pThing, sithVehicleControls_curRaftState.nextPuppetMode);
                        sithPuppet_StopKey(pThing->renderData.pPuppet, track, 0.0f);
                    }
                }
                else if ( pThing->moveStatus != SITHPLAYERMOVE_RAFT_PADDLELEFT_STARTPADDLE_RIGHT
                    && pThing->moveStatus != SITHPLAYERMOVE_RAFT_PADDLERIGHT_STARTPADDLE_LEFT
                    && pThing->moveStatus != SITHPLAYERMOVE_RAFT_ENDPADDLE_LEFT
                    && pThing->moveStatus != SITHPLAYERMOVE_RAFT_ENDPADDLE_RIGHT
                    && pThing->moveStatus != SITHPLAYERMOVE_RAFT_STARTPADDLE_RIGHT
                    && pThing->moveStatus != SITHPLAYERMOVE_RAFT_STARTPADDLE_LEFT )
                {
                    if ( pThing->moveStatus == SITHPLAYERMOVE_RAFT_DOCKING )
                    {
                        sithVehicleControls_PlayRaftPuppetMode(pThing, sithVehicleControls_curRaftState.nextPuppetMode);
                    }

                    sithPuppet_StopKey(pThing->renderData.pPuppet, track, 0.0f);
                }
                else
                {
                    SithThingMoveStatus prevMoveStatus = pThing->moveStatus;    // redundant?
                    pThing->moveStatus = sithVehicleControls_curRaftState.nextMoveStatus;
                    sithVehicleControls_PlayRaftPuppetMode(pThing, sithVehicleControls_curRaftState.nextPuppetMode);
                    sithPuppet_StopKey(pThing->renderData.pPuppet, track, 0.0f);
                }

                sithPuppet_DefaultCallback(pThing, track, markerType);
                return;
        }

        if ( pThing->moveStatus == SITHPLAYERMOVE_RAFT_DOCKING && markerType == 0 ) // 0 - finished playing
        {
            if ( sithVehicleControls_curRaftState.nextMoveStatus == SITHPLAYERMOVE_RAFT_UNBOARD_START )
            {
                pThing->moveStatus = SITHPLAYERMOVE_RAFT_UNBOARD_START;
            }
            else
            {
                sithVehicleControls_curRaftState.nextMoveStatus = SITHPLAYERMOVE_RAFT_UNBOARD_START;
            }

            sithPuppet_DefaultCallback(pThing, track, markerType);
            return;
        }

        if ( pThing->moveStatus == SITHPLAYERMOVE_RAFT_PADDLELEFT_STARTPADDLE_RIGHT
            || pThing->moveStatus == SITHPLAYERMOVE_RAFT_PADDLERIGHT_STARTPADDLE_LEFT
            || pThing->moveStatus == SITHPLAYERMOVE_RAFT_ENDPADDLE_LEFT
            || pThing->moveStatus == SITHPLAYERMOVE_RAFT_ENDPADDLE_RIGHT
            || pThing->moveStatus == SITHPLAYERMOVE_RAFT_STARTPADDLE_RIGHT
            || pThing->moveStatus == SITHPLAYERMOVE_RAFT_STARTPADDLE_LEFT )
        {
            if ( !markerType && pThing->moveStatus != sithVehicleControls_curRaftState.nextMoveStatus )
            {
                SithThingMoveStatus prevMoveStatus = pThing->moveStatus;    // redundant?
                pThing->moveStatus = sithVehicleControls_curRaftState.nextMoveStatus;
                sithVehicleControls_PlayRaftPuppetMode(pThing, sithVehicleControls_curRaftState.nextPuppetMode);
            }

            sithPuppet_DefaultCallback(pThing, track, markerType);
            return;
        }

        if ( pThing->moveStatus != SITHPLAYERMOVE_LAND )
        {
            switch ( pThing->moveStatus )
            {
                case SITHPLAYERMOVE_RAFT_BOARDING:
                    if ( markerType )
                    {
                        if ( markerType == RDKEYMARKER_DUCK ) // Indy in raft
                        {
                            rdVector_Scale3(&pPhysics->velocity, &pThing->orient.rvec, -sithVehicleControls_raftBoardingMomentum);
                            pPhysics->angularVelocity.yaw = sithVehicleControls_raftBoardingRotation;

                        #ifndef J3D_QOL_IMPROVEMENTS // Raft splatter fx was fixed, enabling old logic only for backward compatibility
                            if ( (SithPhysicsWaterSurfaceType)pThing->userval == SITHPHYSICS_WATERSURFACE_ADJOIN )  // If raft is on underwater sector, (set by sithPhysics_CheckWaterSurfaceAtPos)
                            #endif
                            {
                                sithFX_CreateRaftSplatterFX(pThing, /*bCreateSplash*/0);
                            }
                        }
                    }
                    else
                    {
                        pThing->thingInfo.actorInfo.bControlsDisabled = 0;
                        sithCog_ThingSendMessageEx(pThing, NULL, SITHCOG_MSG_CALLBACK, 0, RDKEYMARKER_PLACERIGHTARMREST, 0, 0);
                        sithCog_ThingSendMessageEx(pThing, NULL, SITHCOG_MSG_CALLBACK, 0, RDKEYMARKER_ATTACKFINISH, 0, 0);

                        pThing->moveStatus = SITHPLAYERMOVE_RAFT_IDLE;
                        sithVehicleControls_EndBoardCutscene(pThing);
                    }
                    break;

                case SITHPLAYERMOVE_RAFT_UNBOARDING_LEFT:
                case SITHPLAYERMOVE_RAFT_UNBOARDING_RIGHT:
                    if ( !markerType ) // finished playing unboarding anim?
                    {
                        pThing->thingInfo.actorInfo.bControlsDisabled = 0;
                        sithVehicleControls_ExitRaft(pThing);
                    }
                    break;

                case SITHPLAYERMOVE_MINECAR_BOARDING:
                    sithSoundClass_PlayModeFirst(pThing, SITHSOUNDCLASS_STARTMOVE);
                    if ( !markerType ) // Finished playing boarding anim?
                    {
                        sithVehicleControls_EndBoardCutscene(pThing);
                        pThing->thingInfo.actorInfo.bControlsDisabled = 0;
                        pThing->moveStatus = SITHPLAYERMOVE_MINECAR_IDLE;

                        // Removed: Moved the light turn on logic to sithPhysics
                        // pThing->flags |= SITH_TF_EMITLIGHT; // Added: Turn on rear red light
                        // pThing->thingInfo.actorInfo.flags |= SITH_AF_HEADLIGHT;
                        // pThing->thingInfo.actorInfo.headLightIntensity.alpha = 2.0f; // light range
                    }
                    break;

                case SITHPLAYERMOVE_JEEP_BOARDING:
                    sithSoundClass_PlayModeFirst(pThing, SITHSOUNDCLASS_STARTMOVE);
                    if ( !markerType ) // Finished playing boarding anim?
                    {
                        sithVehicleControls_EndBoardCutscene(pThing);
                        pThing->thingInfo.actorInfo.bControlsDisabled = 0;
                        pThing->moveStatus = SITHPLAYERMOVE_JEEP_IDLE;
                    }
                    break;

                case SITHPLAYERMOVE_JEEP_UNBOARDING:
                    if ( !markerType ) // Finished playing unboarding anim?
                    {
                        sithPlayerControls_ExitVehicle(pThing);
                    }
                    break;

                default:
                    if ( (pPhysics->flags & SITH_PF_JEEP) != 0 && markerType == RDKEYMARKER_DIED )
                    {
                        sithSoundClass_PlayModeFirst(pThing, SITHSOUNDCLASS_DEATH1);
                    }

                    switch ( pThing->moveStatus )
                    {
                        case SITHPLAYERMOVE_MINECAR_UNBOARDING_LEFT:
                        case SITHPLAYERMOVE_MINECAR_UNBOARDING_RIGHT:
                            if ( !markerType ) // Finished playing unboarding anim?
                            {
                                sithPlayerControls_ExitVehicle(pThing);
                            }
                            break;

                        case SITHPLAYERMOVE_MINECAR_DUCKING:
                            if ( markerType == RDKEYMARKER_DUCK )
                            {
                                pThing->thingInfo.actorInfo.bControlsDisabled = 0;
                                pThing->moveStatus = SITHPLAYERMOVE_MINECAR_DUCKED;
                                sithVehicleControls_curMineCarState.secDuckTime = sithTime_g_secGameTime;
                            }
                            break;

                        case SITHPLAYERMOVE_MINECAR_DUCKED:
                            if ( !markerType ) // Finished playing duck anim?
                            {
                                pThing->moveStatus = SITHPLAYERMOVE_MINECAR_GETTING_UP;
                            }
                            break;

                        case SITHPLAYERMOVE_MINECAR_GETTING_UP:
                            if ( !markerType ) // Finished playing getting up anim?
                            {
                                pThing->thingInfo.actorInfo.bControlsDisabled = 0;
                                pThing->moveStatus = SITHPLAYERMOVE_MINECAR_IDLE;
                                pPhysics->flags   &= ~SITH_PF_CROUCHING;
                            }
                            break;

                        default:
                            return;
                    }

                    break;
            }

            sithPuppet_DefaultCallback(pThing, track, markerType);
            return;
        }

        if ( markerType )
        {
            sithPuppet_DefaultCallback(pThing, track, markerType);
            return;
        }

        pThing->thingInfo.actorInfo.bControlsDisabled = 0;
        sithPuppet_FreeTrackByIndex(pThing, track);

        if ( (pPhysics->flags & SITH_PF_RAFT) != 0 )
        {
            pThing->moveStatus = SITHPLAYERMOVE_RAFT_IDLE;
            sithVehicleControls_PlayRaftPuppetMode(pThing, SITHPUPPETSUBMODE_STAND);
        }
        else
        {
            pThing->moveStatus = SITHPLAYERMOVE_STILL;
        }
    }
}

void J3DAPI sithVehicleControls_ProcessMineCarMove(SithThing* pThing, float secDeltaTime)
{
    if ( !sithPhysics_CreateMineCarUserBlock(pThing) )
    {
        return;
    }

    if ( pThing->moveStatus == SITHPLAYERMOVE_MINECAR_BOARDING
        || pThing->moveStatus == SITHPLAYERMOVE_MINECAR_UNBOARDING_RIGHT
        || pThing->moveStatus == SITHPLAYERMOVE_MINECAR_UNBOARDING_LEFT )
    {
        sithVehicleControls_curMineCarState.moveState    = SITHMINECARCONTROLS_MOVE_IDLE;
        sithVehicleControls_curMineCarState.leanState    = SITHMINECARCONTROLS_LEAN_NONE;
        sithVehicleControls_curMineCarState.unboardState = SITHMINECARCONTROLS_UNBOARD_NONE;
        return;
    }

    SithPhysicsInfo* pPhysics = &pThing->moveInfo.physics;
    if ( (pPhysics->velocity.x != 0.0f || pPhysics->velocity.y != 0.0f || pPhysics->velocity.z != 0.0f)
        && sithVehicleControls_curMineCarState.moveState == SITHMINECARCONTROLS_MOVE_IDLE )
    {
        sithVehicleControls_curMineCarState.moveState = SITHMINECARCONTROLS_MOVE_CRUISING;
    }

    if ( !sithControl_GetKey(SITHCONTROL_CRAWLTOGGLE, 0) && pThing->moveStatus == SITHPLAYERMOVE_MINECAR_IDLE )
    {
        sithVehicleControls_curMineCarState.bCanDuck = 1;
    }

    // Zero-out right and up thrust
    pPhysics->thrust.x = 0.0f;
    pPhysics->thrust.z = 0.0f;

    SithMineCarState* pCarState = &pThing->userblock.pMinecar->state;

    // Grab minecar constants from thing
    // Note that there aren't any special struct made for minecar and existing struct fields are reused
    const float forwardThrustFactor         = pPhysics->maxVelocity;
    const float forwardAccelThresholdFactor = pThing->thingInfo.actorInfo.jumpSpeed;
    const float backwardThrustFactor        = pPhysics->orientSpeed;

    const float accelThrust = pThing->thingInfo.actorInfo.maxThrust;
    const float decelThrust = pThing->thingInfo.actorInfo.maxRotVelocity;// used for when thrust is changing direction, e.g.: forward to backward and vie versa

    const float maxDuckTime = pPhysics->airDrag;

    const float maxForwardThrust  = pPhysics->surfDrag * forwardThrustFactor;
    const float maxBackwardThrust = -(pPhysics->surfDrag * backwardThrustFactor);

    const float forwardThrustThreshold  = (pPhysics->surfDrag + 1.0f) * forwardAccelThresholdFactor;
    const float backwardThrustThreshold = -((pPhysics->surfDrag + 1.0f) * backwardThrustFactor);

    rdMath_ClipVector3Acc(&pPhysics->velocity, STDMATH_ZERO_EPSILON);
    if ( rdVector_IsZero3(&pPhysics->velocity)
        && sithVehicleControls_curMineCarState.moveState != SITHMINECARCONTROLS_MOVE_STOPPING )
    {
        sithVehicleControls_curMineCarState.moveState = SITHMINECARCONTROLS_MOVE_IDLE;
    }

    //
    // Update indy lean state
    //
    if ( sithVehicleControls_curMineCarState.leanState == SITHMINECARCONTROLS_LEAN_NONE
        && pThing->moveStatus != SITHPLAYERMOVE_MINECAR_GETTING_UP
        && pThing->moveStatus != SITHPLAYERMOVE_MINECAR_DUCKING
        && pThing->moveStatus != SITHPLAYERMOVE_MINECAR_DUCKED )
    {
        if ( rdVector_Len3(&pPhysics->velocity) > (double)sithVehicleControls_minecarTiltVelThreshold )
        {
            if ( pPhysics->angularVelocity.yaw > (double)sithVehicleControls_minecarTiltAngularVelThreshold )
            {
                sithVehicleControls_curMineCarState.curPuppetTrack = sithPuppet_PlayMode(pThing, SITHPUPPETSUBMODE_STAND2WALK, NULL);// im_leanR.key
                sithVehicleControls_curMineCarState.leanState = SITHMINECARCONTROLS_LEAN_RIGHT;
            }
            else if ( -sithVehicleControls_minecarTiltAngularVelThreshold > pPhysics->angularVelocity.yaw )
            {
                sithVehicleControls_curMineCarState.curPuppetTrack = sithPuppet_PlayMode(pThing, SITHPUPPETSUBMODE_WALK2STAND, NULL);// im_leanL.key
                sithVehicleControls_curMineCarState.leanState = SITHMINECARCONTROLS_LEAN_LEFT;
            }
        }
    }
    else if ( sithVehicleControls_curMineCarState.leanState == SITHMINECARCONTROLS_LEAN_RIGHT )// lean right
    {
        if ( pPhysics->angularVelocity.yaw < (double)sithVehicleControls_minecarUntiltAngularVelThreshold
            || rdVector_Len3(&pPhysics->velocity) < (double)sithVehicleControls_minecarUntiltVelThreshold
            || pThing->moveStatus == SITHPLAYERMOVE_MINECAR_DUCKING )
        {
            sithPuppet_StopKey(pThing->renderData.pPuppet, sithVehicleControls_curMineCarState.curPuppetTrack, 0.15000001f);
            sithVehicleControls_curMineCarState.leanState = SITHMINECARCONTROLS_LEAN_NONE;
        }
    }
    else if ( sithVehicleControls_curMineCarState.leanState == SITHMINECARCONTROLS_LEAN_LEFT )// lean left
    {
        if ( -sithVehicleControls_minecarUntiltAngularVelThreshold < pPhysics->angularVelocity.y
            || rdVector_Len3(&pPhysics->velocity) < (double)sithVehicleControls_minecarUntiltVelThreshold
            || pThing->moveStatus == SITHPLAYERMOVE_MINECAR_DUCKING )
        {
            sithPuppet_StopKey(pThing->renderData.pPuppet, sithVehicleControls_curMineCarState.curPuppetTrack, 0.15000001f);
            sithVehicleControls_curMineCarState.leanState = SITHMINECARCONTROLS_LEAN_NONE;
        }
    }

    //
    // Update car stopping time, and stop it if threshold reached
    //
    if ( sithVehicleControls_curMineCarState.moveState == SITHMINECARCONTROLS_MOVE_STOPPING )
    {
        sithVehicleControls_curMineCarState.secElapsedStoppingTime += secDeltaTime;
        if ( sithVehicleControls_curMineCarState.secElapsedStoppingTime > 0.40000001f )
        {
            sithVehicleControls_curMineCarState.moveState = SITHMINECARCONTROLS_MOVE_IDLE;
        }

        return;
    }

    //
    // Update player ducked state
    //
    if ( pThing->moveStatus == SITHPLAYERMOVE_MINECAR_DUCKED && sithTime_g_secGameTime - sithVehicleControls_curMineCarState.secDuckTime > maxDuckTime )
    {
        sithPuppet_PlayMode(pThing, SITHPUPPETSUBMODE_SLIDEDOWNBACK, sithVehicleControls_PuppetCallback);
        if ( sithVehicleControls_curMineCarState.curDuckPuppetTrackNum != -1 )
        {
            sithPuppet_StopKey(pThing->renderData.pPuppet, sithVehicleControls_curMineCarState.curDuckPuppetTrackNum, 0.1f);
        }

        pThing->thingInfo.actorInfo.bControlsDisabled = 1;
        return;
    }

    if ( sithVehicleControls_curMineCarState.unboardState == SITHMINECARCONTROLS_UNBOARD_NONE
        || sithVehicleControls_curMineCarState.unboardState == SITHMINECARCONTROLS_UNBOARD_BLOCKED ) // Altered: Add check for SITHMINECARCONTROLS_UNBOARD_BLOCKED
    {
        if ( !sithControl_GetKey(SITHCONTROL_ACT2, NULL) )
        {
            sithVehicleControls_curMineCarState.unboardState = SITHMINECARCONTROLS_UNBOARD_READY;
        }

        goto LABEL_106;
    }

    if ( sithVehicleControls_curMineCarState.unboardState == SITHMINECARCONTROLS_UNBOARD_READY )
    {
        if ( sithControl_GetKey(SITHCONTROL_ACT2, NULL)
            && sithVehicleControls_curMineCarState.moveState <= (unsigned int)SITHMINECARCONTROLS_MOVE_STOPPING
            && pThing->moveStatus != SITHPLAYERMOVE_MINECAR_DUCKING
            && pThing->moveStatus != SITHPLAYERMOVE_MINECAR_GETTING_UP
            && pThing->moveStatus != SITHPLAYERMOVE_MINECAR_DUCKED )
        {
            sithVehicleControls_curMineCarState.unboardState = SITHMINECARCONTROLS_UNBOARD_ACTIVATED;
            sithVehicleControls_curMineCarState.secUnboardingElapsedTime = 0.0f;
            sithPlayerControls_bActionKeyActive = true;
        }

        goto LABEL_106;
    }

    if ( sithVehicleControls_curMineCarState.unboardState < SITHMINECARCONTROLS_UNBOARD_ACTIVATED
        || (sithVehicleControls_curMineCarState.secUnboardingElapsedTime += secDeltaTime,
            sithVehicleControls_curMineCarState.secUnboardingElapsedTime < 0.2f)
        && !sithControl_GetKey(SITHCONTROL_TURNLEFT, NULL)
        && !sithControl_GetKey(SITHCONTROL_TURNRIGHT, NULL)
        && sithControl_GetKey(SITHCONTROL_ACT2, NULL) )
    {
    LABEL_106:
        if ( sithVehicleControls_curMineCarState.unboardState >= SITHMINECARCONTROLS_UNBOARD_ACTIVATED )
        {
            return;
        }

        bool bUpdateThrust = false;
        bool bAccelerate   = false;
        float thrustDelta  = 0.0f;

        //
        // Check if duck  key pressed
        //
        if ( sithControl_GetKey(SITHCONTROL_CRAWLTOGGLE, NULL) && sithVehicleControls_curMineCarState.bCanDuck )
        {
            if ( pThing->moveStatus == SITHPLAYERMOVE_MINECAR_IDLE )
            {
                sithVehicleControls_curMineCarState.curDuckPuppetTrackNum = sithPuppet_PlayMode(
                    pThing,
                    SITHPUPPETSUBMODE_SLIDEDOWNFWD,
                    sithVehicleControls_PuppetCallback
                );

                pThing->moveStatus = SITHPLAYERMOVE_MINECAR_DUCKING;
                pThing->moveInfo.physics.flags |= SITH_PF_CROUCHING;
                pThing->thingInfo.actorInfo.bControlsDisabled = 1;
                sithVehicleControls_curMineCarState.bCanDuck = 0;
            }
        }
        //
        // Check if forward acceleration key pressed
        //
        else if ( sithControl_GetKey(SITHCONTROL_FORWARD, NULL) )
        {
            if ( pPhysics->thrust.y < (double)maxForwardThrust
                && pThing->moveStatus != SITHPLAYERMOVE_MINECAR_DUCKING
                && pThing->moveStatus != SITHPLAYERMOVE_MINECAR_DUCKED
                && pThing->moveStatus != SITHPLAYERMOVE_MINECAR_GETTING_UP
                && sithVehicleControls_curMineCarState.leanState == SITHMINECARCONTROLS_LEAN_NONE )
            {
                sithPuppet_PlayMode(pThing, SITHPUPPETSUBMODE_ACTIVATEHIGH, NULL); // im_pushlever.key
            }

            bUpdateThrust = true;
            if ( pPhysics->thrust.y >= 0.0f )
            {
                bAccelerate = true;
                thrustDelta = accelThrust;
            }
            else
            {
                bAccelerate = false; // break
                thrustDelta = decelThrust;
            }
        }
        //
        // Check if backward acceleration key pressed
        //
        else if ( sithControl_GetKey(SITHCONTROL_BACK, NULL) )
        {
            if ( pPhysics->thrust.y > (double)maxBackwardThrust
                && pThing->moveStatus != SITHPLAYERMOVE_MINECAR_DUCKING
                && pThing->moveStatus != SITHPLAYERMOVE_MINECAR_DUCKED
                && pThing->moveStatus != SITHPLAYERMOVE_MINECAR_GETTING_UP
                && sithVehicleControls_curMineCarState.leanState == SITHMINECARCONTROLS_LEAN_NONE )
            {
                sithPuppet_PlayMode(pThing, SITHPUPPETSUBMODE_ACTIVATE, NULL); // im_pulllever.key
            }

            bUpdateThrust = true;
            if ( pPhysics->thrust.y <= 0.0f )
            {
                bAccelerate = true;
                thrustDelta = -accelThrust;
            }
            else
            {
                bAccelerate = false;
                thrustDelta = -decelThrust;
            }
        }
        //
        // Check if car is currently accelerating 
        //
        else if ( sithVehicleControls_curMineCarState.moveState == SITHMINECARCONTROLS_MOVE_ACCELERATING )
        {
            bUpdateThrust = true;
            bAccelerate   = true;
            if ( pPhysics->thrust.y < 0.0f )
            {
                thrustDelta = -accelThrust;
            }
            else
            {
                thrustDelta = accelThrust;
            }
        }
        //
        // Check if car is currently decelerating
        //
        else if ( sithVehicleControls_curMineCarState.moveState == SITHMINECARCONTROLS_MOVE_DECELERATING )
        {
            bUpdateThrust = true;
            bAccelerate   = false;
            if ( pPhysics->thrust.y >= 0.0f )
            {
                thrustDelta = -(decelThrust * 0.5f);
            }
            else
            {
                thrustDelta = decelThrust * 0.5f;
            }
        }

        if ( bUpdateThrust )
        {
            thrustDelta *= secDeltaTime;
            if ( bAccelerate )
            {
                sithVehicleControls_curMineCarState.secElapsedBrakingTime = 0.0f;

                float thrustY = pPhysics->thrust.y;
                if ( thrustY < (double)forwardThrustThreshold && thrustY >(double)backwardThrustThreshold )
                {
                    sithVehicleControls_curMineCarState.moveState = SITHMINECARCONTROLS_MOVE_ACCELERATING;
                }

                if ( thrustY == 0.0f )
                {
                    pPhysics->thrust.y = thrustDelta; // Note: The original keeps the old thrust (zero) in the local copy used for the check below
                }
                else
                {
                    thrustY += thrustDelta;
                    if ( thrustY > maxForwardThrust )
                    {
                        thrustY = maxForwardThrust;
                    }
                    else if ( thrustY < maxBackwardThrust )
                    {
                        thrustY = maxBackwardThrust;
                    }

                    pPhysics->thrust.y = thrustY;
                }

                if ( sithVehicleControls_curMineCarState.moveState == SITHMINECARCONTROLS_MOVE_ACCELERATING
                    && (thrustY >= (double)forwardThrustThreshold || thrustY <= (double)backwardThrustThreshold) )
                {
                    sithVehicleControls_curMineCarState.moveState = SITHMINECARCONTROLS_MOVE_CRUISING;
                }
            }
            else
            {
                // Apply brake soundfx and mark making brake sparks
                sithVehicleControls_curMineCarState.secElapsedBrakingTime += secDeltaTime;
                if ( sithVehicleControls_curMineCarState.secElapsedBrakingTime > 0.2f && !sithVehicleControls_curMineCarState.hBrakeSnd )
                {
                    sithVehicleControls_curMineCarState.hBrakeSnd = sithSoundClass_PlayModeFirst(pThing, SITHSOUNDCLASS_RRUNMETAL);// *screech_brake.wav
                    pCarState->bBraking = 1;
                }

                if ( fabsf(pPhysics->thrust.y) >= fabsf(thrustDelta) )
                {
                    pPhysics->thrust.y += thrustDelta;
                    if ( pPhysics->thrust.y < (double)forwardThrustThreshold && pPhysics->thrust.y >(double)backwardThrustThreshold )
                    {
                        sithVehicleControls_curMineCarState.moveState = SITHMINECARCONTROLS_MOVE_DECELERATING;
                    }
                }
                else
                {
                    // Stop the mine car
                    rdVector_Zero3(&pPhysics->thrust);
                    rdVector_Zero3(&pPhysics->velocity);

                    sithVehicleControls_curMineCarState.moveState              = SITHMINECARCONTROLS_MOVE_STOPPING;
                    sithVehicleControls_curMineCarState.secElapsedStoppingTime = 0.0f;
                    sithVehicleControls_curMineCarState.secElapsedBrakingTime  = 0.0f;
                }
            }
        }
        else
        {
            sithVehicleControls_curMineCarState.secElapsedBrakingTime = 0.0f;
        }

        if ( sithVehicleControls_curMineCarState.secElapsedBrakingTime != 0.0f )
        {
            return;
        }

        if ( !sithVehicleControls_curMineCarState.hBrakeSnd )
        {
            return;
        }

        sithSoundClass_FadeModeVolume(pThing, SITHSOUNDCLASS_RRUNMETAL, 0.0f, sithVehicleControls_sndVolumeBrake); // screech_brake.wav
        sithVehicleControls_curMineCarState.hBrakeSnd = 0;

        if ( !pCarState->bBraking )
        {
            return;
        }

        pCarState->bBraking           = 0;
        pCarState->bUpdateSparksRight = 0;
        pCarState->bUpdateSparksLeft  = 0;
        return;
    }

    // Player activated minecar unboarding


    rdVector3 rdir = pThing->orient.rvec;
    bool bCanUnboardRight = sithVehicleControls_CanUnboardInDirection(pThing, &rdir);

    rdVector_Neg3Acc(&rdir);
    bool bCanUnboardLeft = sithVehicleControls_CanUnboardInDirection(pThing, &rdir);

    bool bUnboard = false;
    bool bUnboardLeft = false;


    // If player pressed left turn key, check if can unboard left
    if ( sithControl_GetKey(SITHCONTROL_TURNLEFT, NULL) )
    {
        if ( bCanUnboardLeft )
        {
            bUnboard     = true;
            bUnboardLeft = true;
        }
        else
        {
            sithVehicleControls_curMineCarState.unboardState = SITHMINECARCONTROLS_UNBOARD_BLOCKED;// I guess blocked movement
            sithSoundClass_PlayPlayerVoiceModeRandom(pThing, SITHSOUNDCLASS_LRUNSNOW);
        }
    }
    // If turn right key pressed unboard either left or right
    else if ( sithControl_GetKey(SITHCONTROL_TURNRIGHT, NULL) )
    {
        // turn right key was pressed here
        if ( bCanUnboardRight )
        {
            bUnboard     = true;
            bUnboardLeft = false;
        }
        else
        {
            sithVehicleControls_curMineCarState.unboardState = SITHMINECARCONTROLS_UNBOARD_BLOCKED;// I guess blocked movement
            sithSoundClass_PlayPlayerVoiceModeRandom(pThing, SITHSOUNDCLASS_LRUNSNOW);
        }
    }
    else // No turn key pressed
    {
        if ( sithVehicleControls_curMineCarState.unboardState == SITHMINECARCONTROLS_UNBOARD_ACTIVATED )
        {
            if ( bCanUnboardLeft )
            {
                bUnboard     = true;
                bUnboardLeft = true;
            }

            else if ( bCanUnboardRight )
            {
                bUnboard     = true;
                bUnboardLeft = false;
            }
            else
            {
                sithVehicleControls_curMineCarState.unboardState = SITHMINECARCONTROLS_UNBOARD_BLOCKED; // Fixed: Replaced SITHMINECARCONTROLS_UNBOARD_NONE with SITHMINECARCONTROLS_UNBOARD_BLOCKED 
                sithSoundClass_PlayPlayerVoiceModeRandom(pThing, SITHSOUNDCLASS_LRUNSNOW); // nope...
            }
        }
    }

    if ( bUnboard )
    {
        //
        // Start unboarding process
        //
        if ( bUnboardLeft )
        {
            pThing->moveStatus = SITHPLAYERMOVE_MINECAR_UNBOARDING_LEFT;
            sithPuppet_PlayMode(pThing, SITHPUPPETSUBMODE_STRAFELEFT, sithVehicleControls_PuppetCallback);
        }
        else
        {
            pThing->moveStatus = SITHPLAYERMOVE_MINECAR_UNBOARDING_RIGHT;
            sithPuppet_PlayMode(pThing, SITHPUPPETSUBMODE_STRAFERIGHT, sithVehicleControls_PuppetCallback);
        }

        // Removed: Moved to sithPhysics module
        //sithSoundClass_PlayModeFirst(pThing, SITHSOUNDCLASS_STOPMOVE);// engine stop; sol_minecar_motor_stop.wav

        pThing->thingInfo.actorInfo.bControlsDisabled    = 1;
        sithVehicleControls_curMineCarState.unboardState = SITHMINECARCONTROLS_UNBOARD_NONE;
        sithVehicleControls_StartBoardCutscene(pThing);
    }
    else if ( !sithControl_GetKey(SITHCONTROL_ACT2, NULL) ) // If unboard key not pressed
    {
        sithVehicleControls_curMineCarState.unboardState = SITHMINECARCONTROLS_UNBOARD_READY;
    }
}

void J3DAPI sithVehicleControls_ProcessJeepMove(SithThing* pThing, float secDeltaTime)
{
    float mouseSensitivity       = 1.8f;
    SithPhysicsInfo* pPhysics    = &pThing->moveInfo.physics;
    const  SithActorInfo* pActor = &pThing->thingInfo.actorInfo;

    if ( pThing->moveStatus != SITHPLAYERMOVE_JEEP_BOARDING
        && pThing->moveStatus != SITHPLAYERMOVE_JEEP_UNBOARDING
        && pThing->moveStatus != SITHPLAYERMOVE_UNKNOWN_83
        && pThing->moveStatus != SITHPLAYERMOVE_UNKNOWN_84
        && pThing->moveStatus != SITHPLAYERMOVE_JEEP_IMPACT )
    {
        pPhysics->height              = sithVehicleControls_jeepHeight;
        pThing->collide.movesize      = sithVehicleControls_jeepMoveSize;
        pPhysics->surfDrag            = sithVehicleControls_jeepSurfDrag;
        pPhysics->staticDrag          = sithVehicleControls_jeepStaticDrag;
        pPhysics->maxVelocity         = sithVehicleControls_jeepMaxVelocity;
        pPhysics->maxRotationVelocity = sithVehicleControls_jeepMaxRotationVelocity;

        pPhysics->flags |= SITH_PF_USEANGULARTHRUST;

        if ( !sithVehicleControls_bJeepStopping )
        {
            sithVehicleControls_secJeepStoppedTime = 0.0f;
        }
        else
        {
            // Set stopping drag values
            pPhysics->surfDrag   = sithVehicleControls_jeepStopSurfDrag;
            pPhysics->staticDrag = sithVehicleControls_jeepStopStaticDrag;

            // use rdVector_IsZero3
            if ( rdVector_IsZero3(&pPhysics->velocity) )
            {
                sithVehicleControls_secJeepStoppedTime += secDeltaTime;
                if ( sithVehicleControls_secJeepStoppedTime > 0.1f )
                {
                    sithVehicleControls_bJeepStopping = 0;
                }
            }
            else
            {
                sithVehicleControls_secJeepStoppedTime = 0.0f;
            }
        }

        //
        // Rotate jeep wheels according to speed and orientation
        //

        if ( !rdVector_IsZero3(&pPhysics->velocity) && !sithVehicleControls_bJeepStopping ) // TODO: Remove braking check
        {
            // TODO: sithVehicleControls_jeepWheelRadius constants should be part of jeep user block
            float wheelRotDelta = sithPhysics_CalcWheelRotationAngle(pThing, sithVehicleControls_jeepWheelRadius, secDeltaTime);

            int jointIdx = sithThing_GetThingJointIndex(pThing, "brwheel");
            if ( jointIdx == -1 )
            {
                SITHLOG_WARNING("Jeep wheel node missing.\n");
            }
            else
            {
                pThing->renderData.apTweakedAngles[jointIdx].pitch -= wheelRotDelta;
            }

            jointIdx = sithThing_GetThingJointIndex(pThing, "blwheel");
            if ( jointIdx == -1 )
            {
                SITHLOG_WARNING("Jeep wheel node missing.\n");
            }

            else
            {
                pThing->renderData.apTweakedAngles[jointIdx].pitch -= wheelRotDelta;
            }

            jointIdx = sithThing_GetThingJointIndex(pThing, "frwheel");
            if ( jointIdx == -1 )
            {
                SITHLOG_WARNING("Jeep wheel node missing.\n");
            }
            else
            {
                pThing->renderData.apTweakedAngles[jointIdx].pitch -= wheelRotDelta;
            }

            jointIdx = sithThing_GetThingJointIndex(pThing, "flwheel");
            if ( jointIdx == -1 )
            {
                SITHLOG_WARNING("Jeep wheel node missing.\n");
            }
            else
            {
                pThing->renderData.apTweakedAngles[jointIdx].pitch -= wheelRotDelta;
            }
        }

        //
        // Set jeep movement parameters
        //

        float thrustAccelRate    = sithVehicleControls_jeepNormalAccelRate;
        float maxForwardVelocity = pPhysics->maxVelocity;

        // Check if player is holding boost key
        if ( sithControl_GetKey(SITHCONTROL_ACT1, NULL) )
        {
            thrustAccelRate = sithVehicleControls_jeepBoostAccelRate;
            maxForwardVelocity = sithVehicleControls_jeepBoostMaxVelocity;
            if ( !sithVehicleControls_bJeepAccMoveSndFxPlayed )
            {
                sithSoundClass_PlayModeFirstEx(pThing, SITHSOUNDCLASS_RWALKSNOW, sithVehicleControls_jeepRevSoundVolume); // jep_start_rev.wav
                sithVehicleControls_bJeepAccMoveSndFxPlayed = 1;
            }
        }
        else
        {
            sithVehicleControls_bJeepAccMoveSndFxPlayed = 0;
        }


        float maxThrustForward    = pPhysics->surfDrag * maxForwardVelocity;
        float maxThrustBackward   = -(pPhysics->surfDrag * sithVehicleControls_jeepMaxBackwardVelocity);
        float thrustDecayForward  = sithVehicleControls_jeepForwardThrustDecay;
        float thrustDecayBackward = sithVehicleControls_jeepBackwardThrustDecay;

        float rotSpeedScale       = STDMATH_CLAMP(rdVector_Len3(&pPhysics->velocity) / sithVehicleControls_jeepRotationVelThreshold, 0.0f, 1.0f);
        float maxRotationVelocity = pPhysics->maxRotationVelocity * rotSpeedScale;
        float maxRotationThrust   = pPhysics->surfDrag * maxRotationVelocity;

        //
        // Check if player init unboard
        //
        if ( sithControl_GetKey(SITHCONTROL_ACT2, NULL) && !sithPlayerControls_bActionKeyActive )
        {
            // Stop jeep if velocity is very low
            if ( rdVector_Len3(&pPhysics->velocity) <= (double)sithVehicleControls_jeepMinSpeed )
            {
                rdVector_Zero3(&pPhysics->velocity);
            }

            if ( fabs(pPhysics->angularVelocity.pitch) <= (double)sithVehicleControls_jeepMinAngularVel )
            {
                pPhysics->angularVelocity.pitch = 0.0f;
            }

            if ( fabs(pPhysics->angularVelocity.yaw) <= (double)sithVehicleControls_jeepMinAngularVel )
            {
                pPhysics->angularVelocity.yaw = 0.0f;
            }

            if ( fabs(pPhysics->angularVelocity.roll) <= (double)sithVehicleControls_jeepMinAngularVel )
            {
                pPhysics->angularVelocity.roll = 0.0f;
            }

            sithPlayerControls_bActionKeyActive = true;

            // rdVector_IsZero3
            const rdVector3 unboardDir = RDVECTOR_NEG3(pThing->orient.rvec);
            if ( rdVector_IsZero3(&pPhysics->velocity)
                && rdVector_IsZero3(&pPhysics->angularVelocity)
                && sithVehicleControls_CanUnboardInDirection(pThing, &unboardDir) )
            {
                // Start unboarding process
                sithVehicleControls_FadeJeepMoveSounds(pThing); // Fade jeep moving soundfx
                sithSoundClass_PlayModeFirst(pThing, SITHSOUNDCLASS_STOPMOVE); // Play jeep stop soundfx

                pThing->thingInfo.actorInfo.bControlsDisabled = 1;
                sithPuppet_PlayMode(pThing, SITHPUPPETSUBMODE_PULLITEM, sithVehicleControls_PuppetCallback);// ij_getup.key

                pThing->moveStatus = SITHPLAYERMOVE_JEEP_UNBOARDING;
                sithVehicleControls_StartBoardCutscene(pThing);
                return;
            }

            // Can't unboard, play voice line
            sithSoundClass_PlayPlayerVoiceModeRandom(pThing, SITHSOUNDCLASS_LRUNSNOW); // it's not safe to get out here
        }

        //
        // Update forward/backward move state based on pressed keys
        //

        float moveDirDot = 0.0f;
        if ( !rdVector_IsZero3(&pPhysics->velocity) )
        {
            moveDirDot = rdVector_Dot3(&pThing->orient.lvec, &pPhysics->velocity);
        }

        if ( sithControl_GetKey(SITHCONTROL_FORWARD, NULL) )
        {
            if ( moveDirDot >= 0.0f && !sithVehicleControls_bJeepStopping ) // moving forward and not stopping?
            {
                if ( !sithVehicleControls_bJeepStartMoveSndFxPlayed )
                {
                    sithSoundClass_PlayModeFirstEx(pThing, SITHSOUNDCLASS_RWALKSNOW, sithVehicleControls_jeepRevSoundVolume);
                }

                sithVehicleControls_bJeepStartMoveSndFxPlayed = 1;

                float thrustDelta  = maxThrustForward * secDeltaTime * thrustAccelRate;
                pPhysics->thrust.y = pPhysics->thrust.y + thrustDelta;
                if ( pPhysics->thrust.y > (double)maxThrustForward )
                {
                    pPhysics->thrust.y = maxThrustForward;
                }
            }
            else
            {
                // Moving backward, stopping
                sithVehicleControls_bJeepStopping = 1;
                pPhysics->thrust.y = 0.0f;
            }
        }
        else if ( sithControl_GetKey(SITHCONTROL_BACK, NULL) )
        {
            if ( moveDirDot <= 0.0f && !sithVehicleControls_bJeepStopping ) // moving backward and not stopping
            {
                float thrustDelta  = maxThrustBackward * secDeltaTime * thrustAccelRate;
                pPhysics->thrust.y = pPhysics->thrust.y + thrustDelta;
                if ( pPhysics->thrust.y < (double)maxThrustBackward )
                {
                    pPhysics->thrust.y = maxThrustBackward;
                }
            }
            else
            {
                // moving forward, stopping
                sithVehicleControls_bJeepStopping = 1;
                pPhysics->thrust.y = 0.0f;
            }
        }
        else
        {
            // No key pressed, apply graduate thrust decay
            if ( moveDirDot < 0.0f ) // backward
            {
                float thrustDelta  = thrustDecayBackward * 0.5f * secDeltaTime;
                if ( -thrustDelta > pPhysics->thrust.y )
                {
                    pPhysics->thrust.y +=  thrustDelta;
                }
                else
                {
                    pPhysics->thrust.y = 0.0f;
                }
            }
            else if ( moveDirDot > 0.0f ) // forward
            {
                float thrustDelta = thrustDecayForward * 0.5f * secDeltaTime;;
                if ( -thrustDelta < pPhysics->thrust.y )
                {
                    pPhysics->thrust.y += thrustDelta;
                }
                else
                {
                    pPhysics->thrust.y = 0.0f;
                }
            }

            sithVehicleControls_bJeepStopping             = 0;
            sithVehicleControls_bJeepStartMoveSndFxPlayed = 0;
        }

        //
        // Update left/right turn state based on pressed keys
        //

        float rotThrustDecay = pPhysics->rotThrust.yaw;
        if ( pThing->moveStatus != SITHPLAYERMOVE_UNKNOWN_82 // falling?
            && (pPhysics->velocity.x != 0.0f || pPhysics->velocity.y != 0.0f || pPhysics->velocity.z != 0.0f) )
        {
            if ( sithControl_GetKey(SITHCONTROL_TURNRIGHT, NULL) )
            {
                if ( rdVector_Dot3(&pThing->orient.lvec, &pPhysics->velocity) < 0.0f )
                {
                    rotThrustDecay = maxRotationThrust * secDeltaTime * sithVehicleControls_jeepTurnRate;
                    pPhysics->rotThrust.yaw = pPhysics->rotThrust.y + rotThrustDecay;
                }
                else
                {
                    if ( rotThrustDecay > 0.0f ) // TODO: ??
                    {
                        rotThrustDecay = 0.0f;
                    }

                    rotThrustDecay = maxRotationThrust * secDeltaTime * sithVehicleControls_jeepTurnRate;
                    pPhysics->rotThrust.yaw = pPhysics->rotThrust.yaw - rotThrustDecay;
                }
            }
            else if ( sithControl_GetKey(SITHCONTROL_TURNLEFT, NULL) )
            {
                // rdVector_Dot3
                if ( rdVector_Dot3(&pThing->orient.lvec, &pPhysics->velocity) < 0.0f )
                {
                    rotThrustDecay = maxRotationThrust * secDeltaTime * sithVehicleControls_jeepTurnRate;
                    pPhysics->rotThrust.yaw = pPhysics->rotThrust.y - rotThrustDecay;
                }
                else
                {
                    if ( rotThrustDecay < 0.0f ) // TODO: ??
                    {
                        rotThrustDecay = 0.0f;
                    }

                    rotThrustDecay = maxRotationThrust * secDeltaTime * sithVehicleControls_jeepTurnRate;
                    pPhysics->rotThrust.yaw = pPhysics->rotThrust.y + rotThrustDecay;
                }
            }
            else // No turn thrust, decay angular velocity
            {
                rotThrustDecay          = sithVehicleControls_jeepRotationDecayRate * secDeltaTime;
                pPhysics->rotThrust.yaw = 0.0f;
                if ( fabs(rotThrustDecay) < (double)fabs(pPhysics->angularVelocity.yaw) )
                {
                    int angVelDir = -1;
                    if ( pPhysics->angularVelocity.yaw >= 0.0f )
                    {
                        angVelDir = 1;
                    }

                    pPhysics->angularVelocity.yaw -= (float)angVelDir * rotThrustDecay;
                }
                else
                {
                    pPhysics->angularVelocity.yaw = 0.0f;
                }
            }
        }

        pPhysics->rotThrust.yaw = STDMATH_CLAMP(pPhysics->rotThrust.yaw, -maxRotationThrust, maxRotationThrust);

        //
        // Mouse turn
        //
        if ( (sithMain_g_sith_mode.debugModeFlags & SITHDEBUG_INEDITOR) != 0
            && !rdVector_IsZero3(&pPhysics->velocity) )
        {
            pPhysics->angularVelocity.yaw += sithPlayerControls_CalculateAngularVelocity(
                &pThing->thingInfo.actorInfo,
                sithControl_GetAxis(SITHCONTROL_MOUSETURN),
                sithControl_GetKeyAsAxis(SITHCONTROL_MOUSETURN),
                mouseSensitivity
            );
        }
    }
}

void J3DAPI sithVehicleControls_PlayRaftPuppetMode(SithThing* pThing, SithPuppetSubMode submode)
{
    sithPuppet_PlayMode(pThing, submode, sithVehicleControls_PuppetCallback);
    pThing->thingInfo.actorInfo.bControlsDisabled = submode != SITHPUPPETSUBMODE_STAND;
    sithVehicleControls_curRaftState.bRowing = 0;
}

void J3DAPI sithVehicleControls_ProcessRaftMove(SithThing* pThing, float secDeltaTime)
{
    SithPhysicsInfo* pPhysics = &pThing->moveInfo.physics;
    SithActorInfo* pActor     = &pThing->thingInfo.actorInfo;

    pPhysics->height     = sithVehicleControls_raftHeight;
    pPhysics->surfDrag   = sithVehicleControls_raftSurfDrag;
    pPhysics->staticDrag = sithVehicleControls_raftStaticDrag;

    pActor->maxThrust             = sithVehicleControls_raftMaxThrust;
    pPhysics->orientSpeed         = sithVehicleControls_raftRotSpeed;
    pPhysics->maxRotationVelocity = sithVehicleControls_raftMaxRotVelocity;

    const float fwdThrust  = pActor->maxThrust;
    const float bkwdThrust = -(fwdThrust * 0.5f);

    const float fwdRotThrust  = sithVehicleControls_raftBaseYawThrust;
    const float bkwdRotThrust = sithVehicleControls_raftBaseYawThrust;

    //
    // Create wake effect
    // TODO: Move to physics module
    if ( sithVehicleControls_curRaftState.wakeTimer <= 0.0f
    #ifndef J3D_QOL_IMPROVEMENTS
        && (rdVector_Len3(&pPhysics->velocity) >= (double)sithVehicleControls_raftWakeThreshold)
        && (SithPhysicsWaterSurfaceType)pThing->userval == SITHPHYSICS_WATERSURFACE_ADJOIN ) // If raft is on underwater sector (set by sithPhysics_CheckWaterSurfaceAtPos).
    {
        sithFX_CreateRaftWakeFX(pThing);
        sithVehicleControls_curRaftState.wakeTimer = sithVehicleControls_raftWakeInterval;
    }
#else
        )
    {
        if ( rdVector_Len3(&pPhysics->velocity) >= sithVehicleControls_raftWakeThreshold )
        {
            sithFX_CreateRaftWakeFX(pThing);
        }
        else
        {
            sithFX_CreateRaftSplatterFX(pThing, /*bCreateSplash=*/0);
        }

        sithVehicleControls_curRaftState.wakeTimer = sithVehicleControls_raftWakeInterval;
    }
#endif
    else if ( sithVehicleControls_curRaftState.wakeTimer > 0.0f )
    {
        float speed  = J3DMAX(rdVector_Len3(&pPhysics->velocity), sithVehicleControls_raftWakeMinSpeed);
        sithVehicleControls_curRaftState.wakeTimer = sithVehicleControls_curRaftState.wakeTimer - speed * secDeltaTime; // Subtract frame time multiplied by raft speed. This makes it dependent on raft speed
    }
    else if ( sithVehicleControls_curRaftState.wakeTimer < 0.0f )
    {
        sithVehicleControls_curRaftState.wakeTimer = 0.0f;
    }

    // Create row water fx
    if ( sithVehicleControls_curRaftState.bRowing )
    {
        sithFX_CreateRaftPaddleWaterFX(pThing, sithVehicleControls_curRaftState.secRowStartTime);
    }

    //
    // Check if player has init raft unboarding
    //
    switch ( pThing->moveStatus )
    {
        case SITHPLAYERMOVE_FALLING:
        case SITHPLAYERMOVE_LAND:
        case SITHPLAYERMOVE_RAFT_DOCKING:
        case SITHPLAYERMOVE_RAFT_BOARDING:
        case SITHPLAYERMOVE_RAFT_UNBOARDING_LEFT:
        case SITHPLAYERMOVE_RAFT_UNBOARDING_RIGHT:
        case SITHPLAYERMOVE_RAFT_UNBOARD_START:
            break;

        default:
        {
            if ( sithControl_GetKey(SITHCONTROL_ACT2, NULL) && !sithPlayerControls_bActionKeyActive )
            {
                sithPlayerControls_bActionKeyActive = true;

                int posSearchResult = sithVehicleControls_FindRaftUnboardPosition(
                    pThing,
                    sithVehicleControls_raftUnboardSearchRadius,
                    &sithVehicleControls_curRaftState.unboardPos,
                    &sithVehicleControls_curRaftState.unboardNorm);
                if ( posSearchResult == 8 ) // not found
                {
                    sithSoundClass_PlayPlayerVoiceModeRandom(pThing, SITHSOUNDCLASS_LRUNSNOW); // Not safe to get out here ...
                }
                else
                {
                    sithVehicleControls_curRaftState.moveSize       = pThing->collide.movesize;
                    sithVehicleControls_curRaftState.secUnboardTime = 0.0f;
                    pThing->collide.movesize = sithVehicleControls_curRaftState.moveSize * 0.5f;

                    sithPhysics_ResetThingMovement(pThing);
                    sithVehicleControls_StartBoardCutscene(pThing);

                    switch ( pThing->moveStatus )
                    {
                        case SITHPLAYERMOVE_RAFT_IDLE:
                            pThing->moveStatus = SITHPLAYERMOVE_RAFT_UNBOARD_START;
                            break;

                        case SITHPLAYERMOVE_RAFT_PADDLE_FORWARD_LEFT:
                        case SITHPLAYERMOVE_RAFT_TURN_LEFT:
                        case SITHPLAYERMOVE_RAFT_PADDLE_BACK_LEFT:
                        case SITHPLAYERMOVE_RAFT_STARTPADDLE_LEFT:
                        case SITHPLAYERMOVE_RAFT_PADDLERIGHT_STARTPADDLE_LEFT:
                            sithVehicleControls_curRaftState.nextMoveStatus = SITHPLAYERMOVE_RAFT_ENDPADDLE_LEFT;
                            pThing->moveStatus = SITHPLAYERMOVE_RAFT_DOCKING;
                            break;

                        case SITHPLAYERMOVE_RAFT_PADDLE_FORWARD_RIGHT:
                        case SITHPLAYERMOVE_RAFT_TURN_RIGHT:
                        case SITHPLAYERMOVE_RAFT_PADDLE_BACK_RIGHT:
                        case SITHPLAYERMOVE_RAFT_STARTPADDLE_RIGHT:
                        case SITHPLAYERMOVE_RAFT_PADDLELEFT_STARTPADDLE_RIGHT:
                            sithVehicleControls_curRaftState.nextMoveStatus = SITHPLAYERMOVE_RAFT_ENDPADDLE_RIGHT;
                            pThing->moveStatus = SITHPLAYERMOVE_RAFT_DOCKING;
                            break;

                        default:
                            pThing->moveStatus = SITHPLAYERMOVE_RAFT_DOCKING;
                            sithVehicleControls_curRaftState.nextMoveStatus = SITHPLAYERMOVE_RAFT_UNBOARD_START;
                            break;
                    }
                }
            }

        } break;
    }

    //
    // Update current & next move status based on current move status and key press state
    //
    switch ( pThing->moveStatus )
    {
        case SITHPLAYERMOVE_FALLING:
            sithCog_ThingSendMessageEx(pThing, NULL, SITHCOG_MSG_CALLBACK, 0, RDKEYMARKER_PLACERIGHTARMREST, 0, 0);
            pActor->bControlsDisabled = 0;
            sithVehicleControls_curRaftState.nextMoveStatus = pThing->moveStatus;
            break;

        case SITHPLAYERMOVE_LAND:
            sithVehicleControls_curRaftState.nextMoveStatus = pThing->moveStatus;
            break;

        case SITHPLAYERMOVE_RAFT_IDLE:
            if ( sithControl_GetKey(SITHCONTROL_FORWARD, NULL)
                && !sithControl_GetKey(SITHCONTROL_TURNLEFT, NULL)
                && !sithControl_GetKey(SITHCONTROL_TURNRIGHT, NULL)
                || sithControl_GetKey(SITHCONTROL_BACK, NULL)
                && !sithControl_GetKey(SITHCONTROL_TURNLEFT, NULL)
                && !sithControl_GetKey(SITHCONTROL_TURNRIGHT, NULL) )
            {
                pThing->moveStatus = SITHPLAYERMOVE_RAFT_STARTPADDLE_RIGHT;
                sithVehicleControls_PlayRaftPuppetMode(pThing, SITHPUPPETSUBMODE_CLIMBWALLRIGHT); // ir_stand_bd_padr.key 
            }
            else if ( sithControl_GetKey(SITHCONTROL_TURNLEFT, NULL) && !sithControl_GetKey(SITHCONTROL_FORWARD, NULL) && !sithControl_GetKey(SITHCONTROL_BACK, NULL) )
            {
                pThing->moveStatus = SITHPLAYERMOVE_RAFT_STARTPADDLE_LEFT;
                sithVehicleControls_PlayRaftPuppetMode(pThing, SITHPUPPETSUBMODE_CLIMBWALLLEFT); //ir_stand_bd_padl.key 
            }
            else if ( sithControl_GetKey(SITHCONTROL_TURNRIGHT, NULL)
                && !sithControl_GetKey(SITHCONTROL_FORWARD, NULL)
                && !sithControl_GetKey(SITHCONTROL_BACK, NULL)
                || sithControl_GetKey(SITHCONTROL_FORWARD, NULL) && sithControl_GetKey(SITHCONTROL_TURNLEFT, NULL) )
            {
                pThing->moveStatus = SITHPLAYERMOVE_RAFT_STARTPADDLE_RIGHT;
                sithVehicleControls_PlayRaftPuppetMode(pThing, SITHPUPPETSUBMODE_CLIMBWALLRIGHT); // ir_stand_bd_padr.key 
            }
            else if ( sithControl_GetKey(SITHCONTROL_FORWARD, NULL) && sithControl_GetKey(SITHCONTROL_TURNRIGHT, NULL)
                || sithControl_GetKey(SITHCONTROL_BACK, NULL) && sithControl_GetKey(SITHCONTROL_TURNLEFT, NULL) )
            {
                pThing->moveStatus = SITHPLAYERMOVE_RAFT_STARTPADDLE_LEFT;
                sithVehicleControls_PlayRaftPuppetMode(pThing, SITHPUPPETSUBMODE_CLIMBWALLLEFT); //ir_stand_bd_padl.key 
            }
            else if ( sithControl_GetKey(SITHCONTROL_BACK, NULL) && sithControl_GetKey(SITHCONTROL_TURNRIGHT, NULL) )
            {
                pThing->moveStatus = SITHPLAYERMOVE_RAFT_STARTPADDLE_RIGHT;
                sithVehicleControls_PlayRaftPuppetMode(pThing, SITHPUPPETSUBMODE_CLIMBWALLRIGHT); // ir_stand_bd_padr.key
            }

            break;

        case SITHPLAYERMOVE_RAFT_PADDLE_FORWARD_LEFT:
            if ( sithControl_GetKey(SITHCONTROL_FORWARD, NULL)
                && !sithControl_GetKey(SITHCONTROL_TURNLEFT, NULL)
                && !sithControl_GetKey(SITHCONTROL_TURNRIGHT, NULL) )
            {
                sithVehicleControls_curRaftState.nextMoveStatus = SITHPLAYERMOVE_RAFT_PADDLELEFT_STARTPADDLE_RIGHT;
            }
            else if ( sithControl_GetKey(SITHCONTROL_BACK, NULL)
                && !sithControl_GetKey(SITHCONTROL_TURNLEFT, NULL)
                && !sithControl_GetKey(SITHCONTROL_TURNRIGHT, NULL) )
            {
                sithVehicleControls_curRaftState.nextMoveStatus = SITHPLAYERMOVE_RAFT_PADDLE_BACK_LEFT;
            }
            else if ( sithControl_GetKey(SITHCONTROL_TURNLEFT, NULL)
                && !sithControl_GetKey(SITHCONTROL_FORWARD, NULL)
                && !sithControl_GetKey(SITHCONTROL_BACK, NULL) )
            {
                sithVehicleControls_curRaftState.nextMoveStatus = SITHPLAYERMOVE_RAFT_TURN_LEFT;
            }
            else if ( sithControl_GetKey(SITHCONTROL_TURNRIGHT, NULL)
                && !sithControl_GetKey(SITHCONTROL_FORWARD, NULL)
                && !sithControl_GetKey(SITHCONTROL_BACK, NULL) )
            {
                sithVehicleControls_curRaftState.nextMoveStatus = SITHPLAYERMOVE_RAFT_PADDLELEFT_STARTPADDLE_RIGHT;
            }
            else if ( sithControl_GetKey(SITHCONTROL_FORWARD, NULL) && sithControl_GetKey(SITHCONTROL_TURNLEFT, NULL) )
            {
                sithVehicleControls_curRaftState.nextMoveStatus = SITHPLAYERMOVE_RAFT_PADDLELEFT_STARTPADDLE_RIGHT;
            }
            else if ( sithControl_GetKey(SITHCONTROL_FORWARD, NULL) && sithControl_GetKey(SITHCONTROL_TURNRIGHT, NULL) )
            {
                sithVehicleControls_curRaftState.nextMoveStatus = SITHPLAYERMOVE_RAFT_PADDLE_FORWARD_LEFT;
            }
            else if ( sithControl_GetKey(SITHCONTROL_BACK, NULL) && sithControl_GetKey(SITHCONTROL_TURNLEFT, NULL) )
            {
                sithVehicleControls_curRaftState.nextMoveStatus = SITHPLAYERMOVE_RAFT_PADDLE_BACK_LEFT;
            }
            else if ( sithControl_GetKey(SITHCONTROL_BACK, NULL) && sithControl_GetKey(SITHCONTROL_TURNRIGHT, NULL) )
            {
                sithVehicleControls_curRaftState.nextMoveStatus = SITHPLAYERMOVE_RAFT_PADDLELEFT_STARTPADDLE_RIGHT;
            }
            else
            {
                sithVehicleControls_curRaftState.nextMoveStatus = SITHPLAYERMOVE_RAFT_ENDPADDLE_LEFT;
            }

            break;

        case SITHPLAYERMOVE_RAFT_PADDLE_FORWARD_RIGHT:
            if ( sithControl_GetKey(SITHCONTROL_FORWARD, NULL)
                && !sithControl_GetKey(SITHCONTROL_TURNLEFT, NULL)
                && !sithControl_GetKey(SITHCONTROL_TURNRIGHT, NULL) )
            {
                sithVehicleControls_curRaftState.nextMoveStatus = SITHPLAYERMOVE_RAFT_PADDLERIGHT_STARTPADDLE_LEFT;
            }
            else if ( sithControl_GetKey(SITHCONTROL_BACK, NULL)
                && !sithControl_GetKey(SITHCONTROL_TURNLEFT, NULL)
                && !sithControl_GetKey(SITHCONTROL_TURNRIGHT, NULL) )
            {
                sithVehicleControls_curRaftState.nextMoveStatus = SITHPLAYERMOVE_RAFT_PADDLE_BACK_RIGHT;
            }
            else if ( sithControl_GetKey(SITHCONTROL_TURNLEFT, NULL)
                && !sithControl_GetKey(SITHCONTROL_FORWARD, NULL)
                && !sithControl_GetKey(SITHCONTROL_BACK, NULL) )
            {
                sithVehicleControls_curRaftState.nextMoveStatus = SITHPLAYERMOVE_RAFT_PADDLERIGHT_STARTPADDLE_LEFT;
            }
            else if ( sithControl_GetKey(SITHCONTROL_TURNRIGHT, NULL)
                && !sithControl_GetKey(SITHCONTROL_FORWARD, NULL)
                && !sithControl_GetKey(SITHCONTROL_BACK, NULL) )
            {
                sithVehicleControls_curRaftState.nextMoveStatus = SITHPLAYERMOVE_RAFT_TURN_RIGHT;
            }
            else if ( sithControl_GetKey(SITHCONTROL_FORWARD, NULL) && sithControl_GetKey(SITHCONTROL_TURNLEFT, NULL) )
            {
                sithVehicleControls_curRaftState.nextMoveStatus = SITHPLAYERMOVE_RAFT_PADDLE_FORWARD_RIGHT;
            }
            else if ( sithControl_GetKey(SITHCONTROL_FORWARD, NULL) && sithControl_GetKey(SITHCONTROL_TURNRIGHT, NULL) )
            {
                sithVehicleControls_curRaftState.nextMoveStatus = SITHPLAYERMOVE_RAFT_PADDLERIGHT_STARTPADDLE_LEFT;
            }
            else if ( sithControl_GetKey(SITHCONTROL_BACK, NULL) && sithControl_GetKey(SITHCONTROL_TURNLEFT, NULL) )
            {
                sithVehicleControls_curRaftState.nextMoveStatus = SITHPLAYERMOVE_RAFT_PADDLERIGHT_STARTPADDLE_LEFT;
            }
            else if ( sithControl_GetKey(SITHCONTROL_BACK, NULL) && sithControl_GetKey(SITHCONTROL_TURNRIGHT, NULL) )
            {
                sithVehicleControls_curRaftState.nextMoveStatus = SITHPLAYERMOVE_RAFT_PADDLE_BACK_RIGHT;
            }
            else
            {
                sithVehicleControls_curRaftState.nextMoveStatus = SITHPLAYERMOVE_RAFT_ENDPADDLE_RIGHT;
            }

            break;

        case SITHPLAYERMOVE_RAFT_TURN_LEFT:
            if ( sithControl_GetKey(SITHCONTROL_FORWARD, NULL)
                && !sithControl_GetKey(SITHCONTROL_TURNLEFT, NULL)
                && !sithControl_GetKey(SITHCONTROL_TURNRIGHT, NULL) )
            {
                sithVehicleControls_curRaftState.nextMoveStatus = SITHPLAYERMOVE_RAFT_PADDLE_FORWARD_LEFT;
            }
            else if ( sithControl_GetKey(SITHCONTROL_BACK, NULL)
                && !sithControl_GetKey(SITHCONTROL_TURNLEFT, NULL)
                && !sithControl_GetKey(SITHCONTROL_TURNRIGHT, NULL) )
            {
                sithVehicleControls_curRaftState.nextMoveStatus = SITHPLAYERMOVE_RAFT_PADDLE_BACK_LEFT;
            }
            else if ( sithControl_GetKey(SITHCONTROL_TURNLEFT, NULL)
                && !sithControl_GetKey(SITHCONTROL_FORWARD, NULL)
                && !sithControl_GetKey(SITHCONTROL_BACK, NULL) )
            {
                sithVehicleControls_curRaftState.nextMoveStatus = SITHPLAYERMOVE_RAFT_TURN_LEFT;
            }
            else if ( sithControl_GetKey(SITHCONTROL_TURNRIGHT, NULL)
                && !sithControl_GetKey(SITHCONTROL_FORWARD, NULL)
                && !sithControl_GetKey(SITHCONTROL_BACK, NULL) )
            {
                sithVehicleControls_curRaftState.nextMoveStatus = SITHPLAYERMOVE_RAFT_PADDLELEFT_STARTPADDLE_RIGHT;
            }
            else if ( sithControl_GetKey(SITHCONTROL_FORWARD, NULL) && sithControl_GetKey(SITHCONTROL_TURNLEFT, NULL) )
            {
                sithVehicleControls_curRaftState.nextMoveStatus = SITHPLAYERMOVE_RAFT_PADDLELEFT_STARTPADDLE_RIGHT;
            }
            else if ( sithControl_GetKey(SITHCONTROL_FORWARD, NULL) && sithControl_GetKey(SITHCONTROL_TURNRIGHT, NULL) )
            {
                sithVehicleControls_curRaftState.nextMoveStatus = SITHPLAYERMOVE_RAFT_PADDLE_FORWARD_LEFT;
            }
            else if ( sithControl_GetKey(SITHCONTROL_BACK, NULL) && sithControl_GetKey(SITHCONTROL_TURNLEFT, NULL) )
            {
                sithVehicleControls_curRaftState.nextMoveStatus = SITHPLAYERMOVE_RAFT_PADDLE_BACK_LEFT;
            }
            else if ( sithControl_GetKey(SITHCONTROL_BACK, NULL) && sithControl_GetKey(SITHCONTROL_TURNRIGHT, NULL) )
            {
                sithVehicleControls_curRaftState.nextMoveStatus = SITHPLAYERMOVE_RAFT_PADDLELEFT_STARTPADDLE_RIGHT;
            }
            else
            {
                sithVehicleControls_curRaftState.nextMoveStatus = SITHPLAYERMOVE_RAFT_ENDPADDLE_LEFT;
            }

            break;

        case SITHPLAYERMOVE_RAFT_TURN_RIGHT:
            if ( sithControl_GetKey(SITHCONTROL_FORWARD, NULL)
                && !sithControl_GetKey(SITHCONTROL_TURNLEFT, NULL)
                && !sithControl_GetKey(SITHCONTROL_TURNRIGHT, NULL) )
            {
                sithVehicleControls_curRaftState.nextMoveStatus = SITHPLAYERMOVE_RAFT_PADDLE_FORWARD_RIGHT;
            }
            else if ( sithControl_GetKey(SITHCONTROL_BACK, NULL)
                && !sithControl_GetKey(SITHCONTROL_TURNLEFT, NULL)
                && !sithControl_GetKey(SITHCONTROL_TURNRIGHT, NULL) )
            {
                sithVehicleControls_curRaftState.nextMoveStatus = SITHPLAYERMOVE_RAFT_PADDLE_BACK_RIGHT;
            }
            else if ( sithControl_GetKey(SITHCONTROL_TURNLEFT, NULL)
                && !sithControl_GetKey(SITHCONTROL_FORWARD, NULL)
                && !sithControl_GetKey(SITHCONTROL_BACK, NULL) )
            {
                sithVehicleControls_curRaftState.nextMoveStatus = SITHPLAYERMOVE_RAFT_PADDLERIGHT_STARTPADDLE_LEFT;
            }
            else if ( sithControl_GetKey(SITHCONTROL_TURNRIGHT, NULL)
                && !sithControl_GetKey(SITHCONTROL_FORWARD, NULL)
                && !sithControl_GetKey(SITHCONTROL_BACK, NULL) )
            {
                sithVehicleControls_curRaftState.nextMoveStatus = SITHPLAYERMOVE_RAFT_TURN_RIGHT;
            }
            else if ( sithControl_GetKey(SITHCONTROL_FORWARD, NULL) && sithControl_GetKey(SITHCONTROL_TURNLEFT, NULL) )
            {
                sithVehicleControls_curRaftState.nextMoveStatus = SITHPLAYERMOVE_RAFT_PADDLE_FORWARD_RIGHT;
            }
            else if ( sithControl_GetKey(SITHCONTROL_FORWARD, NULL) && sithControl_GetKey(SITHCONTROL_TURNRIGHT, NULL) )
            {
                sithVehicleControls_curRaftState.nextMoveStatus = SITHPLAYERMOVE_RAFT_PADDLERIGHT_STARTPADDLE_LEFT;
            }
            else if ( sithControl_GetKey(SITHCONTROL_BACK, NULL) && sithControl_GetKey(SITHCONTROL_TURNLEFT, NULL) )
            {
                sithVehicleControls_curRaftState.nextMoveStatus = SITHPLAYERMOVE_RAFT_PADDLERIGHT_STARTPADDLE_LEFT;
            }
            else if ( sithControl_GetKey(SITHCONTROL_BACK, NULL) && sithControl_GetKey(SITHCONTROL_TURNRIGHT, NULL) )
            {
                sithVehicleControls_curRaftState.nextMoveStatus = SITHPLAYERMOVE_RAFT_PADDLE_BACK_RIGHT;
            }
            else
            {
                sithVehicleControls_curRaftState.nextMoveStatus = SITHPLAYERMOVE_RAFT_ENDPADDLE_RIGHT;
            }

            break;

        case SITHPLAYERMOVE_RAFT_PADDLE_BACK_LEFT:
            if ( sithControl_GetKey(SITHCONTROL_FORWARD, NULL)
                && !sithControl_GetKey(SITHCONTROL_TURNLEFT, NULL)
                && !sithControl_GetKey(SITHCONTROL_TURNRIGHT, NULL) )
            {
                sithVehicleControls_curRaftState.nextMoveStatus = SITHPLAYERMOVE_RAFT_PADDLE_FORWARD_LEFT;
            }
            else if ( sithControl_GetKey(SITHCONTROL_BACK, NULL)
                && !sithControl_GetKey(SITHCONTROL_TURNLEFT, NULL)
                && !sithControl_GetKey(SITHCONTROL_TURNRIGHT, NULL) )
            {
                sithVehicleControls_curRaftState.nextMoveStatus = SITHPLAYERMOVE_RAFT_PADDLELEFT_STARTPADDLE_RIGHT;
            }
            else if ( sithControl_GetKey(SITHCONTROL_TURNLEFT, NULL)
                && !sithControl_GetKey(SITHCONTROL_FORWARD, NULL)
                && !sithControl_GetKey(SITHCONTROL_BACK, NULL) )
            {
                sithVehicleControls_curRaftState.nextMoveStatus = SITHPLAYERMOVE_RAFT_TURN_LEFT;
            }
            else if ( sithControl_GetKey(SITHCONTROL_TURNRIGHT, NULL)
                && !sithControl_GetKey(SITHCONTROL_FORWARD, NULL)
                && !sithControl_GetKey(SITHCONTROL_BACK, NULL) )
            {
                sithVehicleControls_curRaftState.nextMoveStatus = SITHPLAYERMOVE_RAFT_PADDLELEFT_STARTPADDLE_RIGHT;
            }
            else if ( sithControl_GetKey(SITHCONTROL_FORWARD, NULL) && sithControl_GetKey(SITHCONTROL_TURNLEFT, NULL) )
            {
                sithVehicleControls_curRaftState.nextMoveStatus = SITHPLAYERMOVE_RAFT_PADDLELEFT_STARTPADDLE_RIGHT;
            }
            else if ( sithControl_GetKey(SITHCONTROL_FORWARD, NULL) && sithControl_GetKey(SITHCONTROL_TURNRIGHT, NULL) )
            {
                sithVehicleControls_curRaftState.nextMoveStatus = SITHPLAYERMOVE_RAFT_PADDLE_FORWARD_LEFT;
            }
            else if ( sithControl_GetKey(SITHCONTROL_BACK, NULL) && sithControl_GetKey(SITHCONTROL_TURNLEFT, NULL) )
            {
                sithVehicleControls_curRaftState.nextMoveStatus = SITHPLAYERMOVE_RAFT_PADDLE_BACK_LEFT;
            }
            else if ( sithControl_GetKey(SITHCONTROL_BACK, NULL) && sithControl_GetKey(SITHCONTROL_TURNRIGHT, NULL) )
            {
                sithVehicleControls_curRaftState.nextMoveStatus = SITHPLAYERMOVE_RAFT_PADDLELEFT_STARTPADDLE_RIGHT;
            }
            else
            {
                sithVehicleControls_curRaftState.nextMoveStatus = SITHPLAYERMOVE_RAFT_ENDPADDLE_LEFT;
            }

            break;

        case SITHPLAYERMOVE_RAFT_PADDLE_BACK_RIGHT:
            if ( sithControl_GetKey(SITHCONTROL_FORWARD, NULL)
                && !sithControl_GetKey(SITHCONTROL_TURNLEFT, NULL)
                && !sithControl_GetKey(SITHCONTROL_TURNRIGHT, NULL) )
            {
                sithVehicleControls_curRaftState.nextMoveStatus = SITHPLAYERMOVE_RAFT_PADDLE_FORWARD_RIGHT;
            }
            else if ( sithControl_GetKey(SITHCONTROL_BACK, NULL)
                && !sithControl_GetKey(SITHCONTROL_TURNLEFT, NULL)
                && !sithControl_GetKey(SITHCONTROL_TURNRIGHT, NULL) )
            {
                sithVehicleControls_curRaftState.nextMoveStatus = SITHPLAYERMOVE_RAFT_PADDLERIGHT_STARTPADDLE_LEFT;
            }
            else if ( sithControl_GetKey(SITHCONTROL_TURNLEFT, NULL)
                && !sithControl_GetKey(SITHCONTROL_FORWARD, NULL)
                && !sithControl_GetKey(SITHCONTROL_BACK, NULL) )
            {
                sithVehicleControls_curRaftState.nextMoveStatus = SITHPLAYERMOVE_RAFT_PADDLERIGHT_STARTPADDLE_LEFT;
            }
            else if ( sithControl_GetKey(SITHCONTROL_TURNRIGHT, NULL)
                && !sithControl_GetKey(SITHCONTROL_FORWARD, NULL)
                && !sithControl_GetKey(SITHCONTROL_BACK, NULL) )
            {
                sithVehicleControls_curRaftState.nextMoveStatus = SITHPLAYERMOVE_RAFT_TURN_RIGHT;
            }
            else if ( sithControl_GetKey(SITHCONTROL_FORWARD, NULL) && sithControl_GetKey(SITHCONTROL_TURNLEFT, NULL) )
            {
                sithVehicleControls_curRaftState.nextMoveStatus = SITHPLAYERMOVE_RAFT_PADDLE_FORWARD_RIGHT;
            }
            else if ( sithControl_GetKey(SITHCONTROL_FORWARD, NULL) && sithControl_GetKey(SITHCONTROL_TURNRIGHT, NULL) )
            {
                sithVehicleControls_curRaftState.nextMoveStatus = SITHPLAYERMOVE_RAFT_PADDLERIGHT_STARTPADDLE_LEFT;
            }
            else if ( sithControl_GetKey(SITHCONTROL_BACK, NULL) && sithControl_GetKey(SITHCONTROL_TURNLEFT, NULL) )
            {
                sithVehicleControls_curRaftState.nextMoveStatus = SITHPLAYERMOVE_RAFT_PADDLERIGHT_STARTPADDLE_LEFT;
            }
            else if ( sithControl_GetKey(SITHCONTROL_BACK, NULL) && sithControl_GetKey(SITHCONTROL_TURNRIGHT, NULL) )
            {
                sithVehicleControls_curRaftState.nextMoveStatus = SITHPLAYERMOVE_RAFT_PADDLE_BACK_RIGHT;
            }
            else
            {
                sithVehicleControls_curRaftState.nextMoveStatus = SITHPLAYERMOVE_RAFT_ENDPADDLE_RIGHT;
            }

            break;

        case SITHPLAYERMOVE_RAFT_STARTPADDLE_LEFT:
        case SITHPLAYERMOVE_RAFT_PADDLERIGHT_STARTPADDLE_LEFT:
            if ( sithControl_GetKey(SITHCONTROL_FORWARD, NULL)
                && !sithControl_GetKey(SITHCONTROL_TURNLEFT, NULL)
                && !sithControl_GetKey(SITHCONTROL_TURNRIGHT, NULL) )
            {
                sithVehicleControls_curRaftState.nextMoveStatus = SITHPLAYERMOVE_RAFT_PADDLE_FORWARD_LEFT;
            }
            else if ( sithControl_GetKey(SITHCONTROL_BACK, NULL)
                && !sithControl_GetKey(SITHCONTROL_TURNLEFT, NULL)
                && !sithControl_GetKey(SITHCONTROL_TURNRIGHT, NULL) )
            {
                sithVehicleControls_curRaftState.nextMoveStatus = SITHPLAYERMOVE_RAFT_PADDLE_BACK_LEFT;
            }
            else if ( sithControl_GetKey(SITHCONTROL_TURNLEFT, NULL)
                && !sithControl_GetKey(SITHCONTROL_FORWARD, NULL)
                && !sithControl_GetKey(SITHCONTROL_BACK, NULL) )
            {
                sithVehicleControls_curRaftState.nextMoveStatus = SITHPLAYERMOVE_RAFT_TURN_LEFT;
            }
            else if ( sithControl_GetKey(SITHCONTROL_TURNRIGHT, NULL)
                && !sithControl_GetKey(SITHCONTROL_FORWARD, NULL)
                && !sithControl_GetKey(SITHCONTROL_BACK, NULL) )
            {
                sithVehicleControls_curRaftState.nextMoveStatus = SITHPLAYERMOVE_RAFT_PADDLELEFT_STARTPADDLE_RIGHT;
            }
            else if ( sithControl_GetKey(SITHCONTROL_FORWARD, NULL) && sithControl_GetKey(SITHCONTROL_TURNLEFT, NULL) )
            {
                sithVehicleControls_curRaftState.nextMoveStatus = SITHPLAYERMOVE_RAFT_PADDLELEFT_STARTPADDLE_RIGHT;
            }
            else if ( sithControl_GetKey(SITHCONTROL_FORWARD, NULL) && sithControl_GetKey(SITHCONTROL_TURNRIGHT, NULL) )
            {
                sithVehicleControls_curRaftState.nextMoveStatus = SITHPLAYERMOVE_RAFT_PADDLE_FORWARD_LEFT;
            }
            else if ( sithControl_GetKey(SITHCONTROL_BACK, NULL) && sithControl_GetKey(SITHCONTROL_TURNLEFT, NULL) )
            {
                sithVehicleControls_curRaftState.nextMoveStatus = SITHPLAYERMOVE_RAFT_PADDLE_BACK_LEFT;
            }
            else if ( sithControl_GetKey(SITHCONTROL_BACK, NULL) && sithControl_GetKey(SITHCONTROL_TURNRIGHT, NULL) )
            {
                sithVehicleControls_curRaftState.nextMoveStatus = SITHPLAYERMOVE_RAFT_PADDLELEFT_STARTPADDLE_RIGHT;
            }
            else
            {
                sithVehicleControls_curRaftState.nextMoveStatus = SITHPLAYERMOVE_RAFT_ENDPADDLE_LEFT;
            }

            break;

        case SITHPLAYERMOVE_RAFT_ENDPADDLE_LEFT:
            sithVehicleControls_curRaftState.nextMoveStatus = SITHPLAYERMOVE_RAFT_IDLE;
            break;

        case SITHPLAYERMOVE_RAFT_ENDPADDLE_RIGHT:
            sithVehicleControls_curRaftState.nextMoveStatus = SITHPLAYERMOVE_RAFT_IDLE;
            break;

        case SITHPLAYERMOVE_RAFT_STARTPADDLE_RIGHT:
        case SITHPLAYERMOVE_RAFT_PADDLELEFT_STARTPADDLE_RIGHT:
            if ( sithControl_GetKey(SITHCONTROL_FORWARD, NULL)
                && !sithControl_GetKey(SITHCONTROL_TURNLEFT, NULL)
                && !sithControl_GetKey(SITHCONTROL_TURNRIGHT, NULL) )
            {
                sithVehicleControls_curRaftState.nextMoveStatus = SITHPLAYERMOVE_RAFT_PADDLE_FORWARD_RIGHT;
            }
            else if ( sithControl_GetKey(SITHCONTROL_BACK, NULL)
                && !sithControl_GetKey(SITHCONTROL_TURNLEFT, NULL)
                && !sithControl_GetKey(SITHCONTROL_TURNRIGHT, NULL) )
            {
                sithVehicleControls_curRaftState.nextMoveStatus = SITHPLAYERMOVE_RAFT_PADDLE_BACK_RIGHT;
            }
            else if ( sithControl_GetKey(SITHCONTROL_TURNLEFT, NULL)
                && !sithControl_GetKey(SITHCONTROL_FORWARD, NULL)
                && !sithControl_GetKey(SITHCONTROL_BACK, NULL) )
            {
                sithVehicleControls_curRaftState.nextMoveStatus = SITHPLAYERMOVE_RAFT_PADDLERIGHT_STARTPADDLE_LEFT;
            }
            else if ( sithControl_GetKey(SITHCONTROL_TURNRIGHT, NULL)
                && !sithControl_GetKey(SITHCONTROL_FORWARD, NULL)
                && !sithControl_GetKey(SITHCONTROL_BACK, NULL) )
            {
                sithVehicleControls_curRaftState.nextMoveStatus = SITHPLAYERMOVE_RAFT_TURN_RIGHT;
            }
            else if ( sithControl_GetKey(SITHCONTROL_FORWARD, NULL) && sithControl_GetKey(SITHCONTROL_TURNLEFT, NULL) )
            {
                sithVehicleControls_curRaftState.nextMoveStatus = SITHPLAYERMOVE_RAFT_PADDLE_FORWARD_RIGHT;
            }
            else if ( sithControl_GetKey(SITHCONTROL_FORWARD, NULL) && sithControl_GetKey(SITHCONTROL_TURNRIGHT, NULL) )
            {
                sithVehicleControls_curRaftState.nextMoveStatus = SITHPLAYERMOVE_RAFT_PADDLERIGHT_STARTPADDLE_LEFT;
            }
            else if ( sithControl_GetKey(SITHCONTROL_BACK, NULL) && sithControl_GetKey(SITHCONTROL_TURNLEFT, NULL) )
            {
                sithVehicleControls_curRaftState.nextMoveStatus = SITHPLAYERMOVE_RAFT_PADDLERIGHT_STARTPADDLE_LEFT;
            }
            else if ( sithControl_GetKey(SITHCONTROL_BACK, NULL) && sithControl_GetKey(SITHCONTROL_TURNRIGHT, NULL) )
            {
                sithVehicleControls_curRaftState.nextMoveStatus = SITHPLAYERMOVE_RAFT_PADDLE_BACK_RIGHT;
            }
            else
            {
                sithVehicleControls_curRaftState.nextMoveStatus = SITHPLAYERMOVE_RAFT_ENDPADDLE_RIGHT;
            }

            break;

        case SITHPLAYERMOVE_RAFT_DOCKING:
        case SITHPLAYERMOVE_RAFT_UNBOARD_START:
            // Make sure docking didn't take too long, else stop docking
            sithVehicleControls_curRaftState.secUnboardTime += secDeltaTime;
            if ( sithVehicleControls_curRaftState.secUnboardTime >= sithVehicleControls_secRaftMaxUnboardingTime )
            {
                // Unboard timeout - stop unboarding
                pThing->moveStatus       = SITHPLAYERMOVE_RAFT_IDLE;
                pThing->collide.movesize = sithVehicleControls_curRaftState.moveSize;
                sithVehicleControls_EndBoardCutscene(pThing);
            }
            else if ( pThing->moveStatus == SITHPLAYERMOVE_RAFT_UNBOARD_START )
            {
                if ( rdVector_Dist3(&sithVehicleControls_curRaftState.unboardPos, &pThing->pos) <= sithVehicleControls_raftUnboardThresholdDist )
                {
                    // If aligned with docking surface then jump out of raft
                    float unboardDot = rdVector_Dot3(&pThing->orient.rvec, &sithVehicleControls_curRaftState.unboardNorm); // Altered: Moved inside if statement
                    if ( fabsf(unboardDot) > 0.98000002f )
                    {
                        pThing->collide.movesize = sithVehicleControls_curRaftState.moveSize;
                        sithPhysics_ResetThingMovement(pThing);

                        if ( unboardDot >= 0.0f )
                        {
                            sithVehicleControls_PlayRaftPuppetMode(pThing, SITHPUPPETSUBMODE_HOPLEFT); // ir_climb_outL.key
                            pThing->moveStatus  = SITHPLAYERMOVE_RAFT_UNBOARDING_LEFT;
                            pThing->orient.rvec = sithVehicleControls_curRaftState.unboardNorm;
                        }
                        else
                        {
                            sithVehicleControls_PlayRaftPuppetMode(pThing, SITHPUPPETSUBMODE_HOPRIGHT); // ir_climb_out.key
                            pThing->moveStatus  = SITHPLAYERMOVE_RAFT_UNBOARDING_RIGHT;
                            pThing->orient.rvec = RDVECTOR_NEG3(sithVehicleControls_curRaftState.unboardNorm);
                        }

                        rdVector_Cross3(&pThing->orient.lvec, &pThing->orient.uvec, &pThing->orient.rvec);
                        rdVector_Normalize3Acc(&pThing->orient.lvec);

                        sithCog_ThingSendMessageEx(pThing, NULL, SITHCOG_MSG_CALLBACK, 0, RDKEYMARKER_PLACERIGHTARM, 0, 0);
                        sithCog_ThingSendMessageEx(pThing, NULL, SITHCOG_MSG_CALLBACK, 0, RDKEYMARKER_ATTACKFINISH, 0, 0);
                    }
                }
            }

            break;

        case SITHPLAYERMOVE_RAFT_BOARDING:
            sithVehicleControls_curRaftState.nextMoveStatus = SITHPLAYERMOVE_RAFT_IDLE;
            break;

        case SITHPLAYERMOVE_RAFT_UNBOARDING_LEFT:
        case SITHPLAYERMOVE_RAFT_UNBOARDING_RIGHT:
            break;

        default:
            pActor->bControlsDisabled = 0;
            sithVehicleControls_curRaftState.nextMoveStatus = pThing->moveStatus;
            break;
    }

    //
    // Update yaw rotation thrust and forward/backward thrust based on current move status
    //  or rotates/moves raft to dock position
    //
    switch ( pThing->moveStatus )
    {
        case SITHPLAYERMOVE_RAFT_PADDLE_FORWARD_LEFT:
            if ( sithVehicleControls_curRaftState.bRowing )
            {
                if ( sithControl_GetKey(SITHCONTROL_TURNLEFT, NULL) )
                {
                    pPhysics->rotThrust.yaw = 0.0f;
                }
                else if ( sithControl_GetKey(SITHCONTROL_TURNRIGHT, NULL) )
                {
                    pPhysics->rotThrust.yaw = -fwdRotThrust;
                }
                else
                {
                    pPhysics->rotThrust.yaw  = -(fwdRotThrust * 0.5f);
                }

                pPhysics->thrust.y = fwdThrust;
            }

            break;

        case SITHPLAYERMOVE_RAFT_PADDLE_FORWARD_RIGHT:
            if ( sithVehicleControls_curRaftState.bRowing )
            {
                if ( sithControl_GetKey(SITHCONTROL_TURNRIGHT, NULL) )
                {
                    pPhysics->rotThrust.yaw = 0.0f;
                }

                else if ( sithControl_GetKey(SITHCONTROL_TURNLEFT, NULL) )
                {
                    pPhysics->rotThrust.yaw = fwdRotThrust;
                }
                else
                {
                    pPhysics->rotThrust.yaw = fwdRotThrust * 0.5f;
                }

                pPhysics->thrust.y = fwdThrust;
            }

            break;

        case SITHPLAYERMOVE_RAFT_TURN_LEFT:
            if ( sithVehicleControls_curRaftState.bRowing )
            {
                if ( sithControl_GetKey(SITHCONTROL_TURNLEFT, NULL) )
                {
                    pPhysics->rotThrust.yaw = fwdRotThrust * sithVehicleControls_raftTurnFactor;
                }
                else
                {
                    pPhysics->rotThrust.yaw = 0.0f;
                }

                pPhysics->thrust.y = 0.0f;
            }

            break;

        case SITHPLAYERMOVE_RAFT_TURN_RIGHT:
            if ( sithVehicleControls_curRaftState.bRowing )
            {
                if ( sithControl_GetKey(SITHCONTROL_TURNRIGHT, NULL) )
                {
                    pPhysics->rotThrust.yaw = -(fwdRotThrust * sithVehicleControls_raftTurnFactor);
                }
                else
                {
                    pPhysics->rotThrust.y = 0.0f;
                }

                pPhysics->thrust.y = 0.0f;
            }

            break;

        case SITHPLAYERMOVE_RAFT_PADDLE_BACK_LEFT:
            if ( sithVehicleControls_curRaftState.bRowing )
            {
                if ( sithControl_GetKey(SITHCONTROL_TURNRIGHT, NULL) )
                {
                    pPhysics->rotThrust.yaw = 0.0f;
                }

                else if ( sithControl_GetKey(SITHCONTROL_TURNLEFT, NULL) )
                {
                    pPhysics->rotThrust.yaw = bkwdRotThrust;
                }
                else
                {
                    pPhysics->rotThrust.yaw = bkwdRotThrust * 0.5f;
                }

                pPhysics->thrust.y = bkwdThrust;
            }

            break;

        case SITHPLAYERMOVE_RAFT_PADDLE_BACK_RIGHT:
            if ( sithVehicleControls_curRaftState.bRowing )
            {
                if ( sithControl_GetKey(SITHCONTROL_TURNLEFT, NULL) )
                {
                    pPhysics->rotThrust.yaw = 0.0f;
                }
                else if ( sithControl_GetKey(SITHCONTROL_TURNRIGHT, NULL) )
                {
                    pPhysics->rotThrust.yaw = -bkwdRotThrust;
                }
                else
                {
                    pPhysics->rotThrust.yaw = -(bkwdRotThrust * 0.5f);
                }

                pPhysics->thrust.y = bkwdThrust;
            }

            break;

        case SITHPLAYERMOVE_RAFT_STARTPADDLE_LEFT:
            pPhysics->rotThrust.yaw = 0.0f;
            pPhysics->thrust.y      = 0.0f;
            break;

        case SITHPLAYERMOVE_RAFT_ENDPADDLE_LEFT:
            pPhysics->rotThrust.yaw = 0.0f;
            pPhysics->thrust.y      = 0.0f;
            break;

        case SITHPLAYERMOVE_RAFT_ENDPADDLE_RIGHT:
            pPhysics->rotThrust.yaw = 0.0f;
            pPhysics->thrust.y      = 0.0f;
            break;

        case SITHPLAYERMOVE_RAFT_STARTPADDLE_RIGHT:
            pPhysics->rotThrust.yaw = 0.0f;
            pPhysics->thrust.y      = 0.0f;
            break;

        case SITHPLAYERMOVE_RAFT_PADDLERIGHT_STARTPADDLE_LEFT:
            pPhysics->rotThrust.yaw = 0.0f;
            pPhysics->thrust.y      = 0.0f;
            break;

        case SITHPLAYERMOVE_RAFT_PADDLELEFT_STARTPADDLE_RIGHT:
            pPhysics->rotThrust.yaw = 0.0f;
            pPhysics->thrust.y      = 0.0f;
            break;

        case SITHPLAYERMOVE_RAFT_DOCKING:
        case SITHPLAYERMOVE_RAFT_UNBOARD_START:
            // Code handles rotating and moving raft to docking position for player to unbaoard
            if ( rdVector_Dist3(&sithVehicleControls_curRaftState.unboardPos, &pThing->pos) <= sithVehicleControls_raftUnboardThresholdDist )
            {
                // At dock position stop movement
                rdVector_Zero3(&pPhysics->thrust);
                rdVector_Zero3(&pPhysics->velocity);
            }
            else
            {
                rdVector3 dirToDock;
                rdVector_Sub3(&dirToDock, &sithVehicleControls_curRaftState.unboardPos, &pThing->pos);
                rdVector_Normalize3Acc(&dirToDock);

                // rdVector_Dot3
                pPhysics->thrust.y = rdVector_Dot3(&pThing->orient.lvec, &dirToDock) * sithVehicleControls_raftDockingThrust;
                pPhysics->thrust.x = rdVector_Dot3(&pThing->orient.rvec, &dirToDock) * sithVehicleControls_raftDockingThrust;
            }

            pPhysics->angularVelocity.yaw = rdVector_Dot3(&pThing->orient.lvec, &sithVehicleControls_curRaftState.unboardNorm) * sithVehicleControls_raftDockingAngularVel;
            if ( rdVector_Dot3(&pThing->orient.rvec, &sithVehicleControls_curRaftState.unboardNorm) < 0.0f )
            {
                pPhysics->angularVelocity.yaw = -pPhysics->angularVelocity.yaw;
            }

            if ( pPhysics->angularVelocity.yaw < (double)sithVehicleControls_raftMinDockingAngularVel
                && -sithVehicleControls_raftMinDockingAngularVel < pPhysics->angularVelocity.yaw )
            {
                pPhysics->angularVelocity.yaw = ((pPhysics->angularVelocity.yaw >= 0.0f) ? 1 : -1) * sithVehicleControls_raftMinDockingAngularVel;
            }

            break;

        case SITHPLAYERMOVE_RAFT_UNBOARDING_LEFT:
        case SITHPLAYERMOVE_RAFT_UNBOARDING_RIGHT:
            break;

        default:
            rdVector_Zero3(&pPhysics->rotThrust);
            rdVector_Zero3(&pPhysics->thrust);
            break;
    }

    //
    // Process next puppet mode based on next move status
    //
    switch ( sithVehicleControls_curRaftState.nextMoveStatus )
    {
        case SITHPLAYERMOVE_RAFT_IDLE:
            sithVehicleControls_curRaftState.nextPuppetMode = SITHPUPPETSUBMODE_STAND;
            break;

        case SITHPLAYERMOVE_RAFT_PADDLE_FORWARD_LEFT:
            sithVehicleControls_curRaftState.nextPuppetMode = SITHPUPPETSUBMODE_STRAFELEFT;
            break;

        case SITHPLAYERMOVE_RAFT_PADDLE_FORWARD_RIGHT:
            sithVehicleControls_curRaftState.nextPuppetMode = SITHPUPPETSUBMODE_STRAFERIGHT;
            break;

        case SITHPLAYERMOVE_RAFT_TURN_LEFT:
            sithVehicleControls_curRaftState.nextPuppetMode = SITHPUPPETSUBMODE_TURNLEFT;
            break;

        case SITHPLAYERMOVE_RAFT_TURN_RIGHT:
            sithVehicleControls_curRaftState.nextPuppetMode = SITHPUPPETSUBMODE_TURNRIGHT;
            break;

        case SITHPLAYERMOVE_RAFT_PADDLE_BACK_LEFT:
            sithVehicleControls_curRaftState.nextPuppetMode = SITHPUPPETSUBMODE_HANGSHIMLEFT;
            break;

        case SITHPLAYERMOVE_RAFT_PADDLE_BACK_RIGHT:
            sithVehicleControls_curRaftState.nextPuppetMode = SITHPUPPETSUBMODE_HANGSHIMRIGHT;
            break;

        case SITHPLAYERMOVE_RAFT_STARTPADDLE_LEFT:
            sithVehicleControls_curRaftState.nextPuppetMode = SITHPUPPETSUBMODE_CLIMBWALLLEFT;
            break;

        case SITHPLAYERMOVE_RAFT_ENDPADDLE_LEFT:
            sithVehicleControls_curRaftState.nextPuppetMode = SITHPUPPETSUBMODE_WHIPCLIMBLEFT;
            break;

        case SITHPLAYERMOVE_RAFT_ENDPADDLE_RIGHT:
            sithVehicleControls_curRaftState.nextPuppetMode = SITHPUPPETSUBMODE_WHIPCLIMBRIGHT;
            break;

        case SITHPLAYERMOVE_RAFT_STARTPADDLE_RIGHT:
            sithVehicleControls_curRaftState.nextPuppetMode = SITHPUPPETSUBMODE_CLIMBWALLRIGHT;
            break;

        case SITHPLAYERMOVE_RAFT_PADDLERIGHT_STARTPADDLE_LEFT:
            sithVehicleControls_curRaftState.nextPuppetMode = SITHPUPPETSUBMODE_WHIPCLIMBDOWN;
            break;

        case SITHPLAYERMOVE_RAFT_PADDLELEFT_STARTPADDLE_RIGHT:
            sithVehicleControls_curRaftState.nextPuppetMode = SITHPUPPETSUBMODE_WHIPCLIMBUP;
            break;

        case SITHPLAYERMOVE_RAFT_DOCKING:
        case SITHPLAYERMOVE_RAFT_UNBOARD_START:
            sithVehicleControls_curRaftState.nextPuppetMode = SITHPUPPETSUBMODE_STAND;
            break;

        case SITHPLAYERMOVE_RAFT_UNBOARDING_LEFT:
            sithVehicleControls_curRaftState.nextPuppetMode = SITHPUPPETSUBMODE_HOPLEFT;
            break;

        case SITHPLAYERMOVE_RAFT_UNBOARDING_RIGHT:
            sithVehicleControls_curRaftState.nextPuppetMode = SITHPUPPETSUBMODE_HOPRIGHT;
            break;

        default:
            sithVehicleControls_curRaftState.nextPuppetMode = SITHPUPPETSUBMODE_STAND;
            break;
    }
}

int J3DAPI sithVehicleControls_FindRaftUnboardPosition(SithThing* pThing, float searchRadius, rdVector3* pUnboardPos, rdVector3* pUnboardNorm)
{
    int bRight = 8; // 8 - not found

    // Make sure we're not upside down??
    if ( pThing->orient.uvec.z < 0.99860001f )   // ??
    {
        return 8;
    }

    //                                  ^ ^ ^
    // Find ledge wall in 7 directions: |,\,/,<-,->,/,\
    //                                             v   v
    SithWallCollisionInfo aColInfos[7] = { 0 }; // Added: init to 0

    //
    // Forward directions
    //

    // Forward and no Z
    aColInfos[2].dir   = pThing->orient.lvec;
    aColInfos[2].dir.z = 0.0f;
    rdVector_Normalize3Acc(&aColInfos[2].dir);

    sithVehicleControls_FindWallSurface(pThing, &aColInfos[2], searchRadius);

    // Forward Left and no Z
    aColInfos[3].dir = RDVECTOR_NEG3(pThing->orient.rvec);
    rdVector_Add3Acc(&aColInfos[3].dir, &pThing->orient.lvec);
    aColInfos[3].dir.z = 0.0f;
    rdVector_Normalize3Acc(&aColInfos[3].dir);

    sithVehicleControls_FindWallSurface(pThing, &aColInfos[3], searchRadius);

    // Forward Right and no Z
    aColInfos[4].dir =  pThing->orient.rvec;
    rdVector_Add3Acc(&aColInfos[4].dir, &pThing->orient.lvec);
    aColInfos[4].dir.z = 0.0f;
    rdVector_Normalize3Acc(&aColInfos[4].dir);

    sithVehicleControls_FindWallSurface(pThing, &aColInfos[4], searchRadius);

    // Left with no Z
    aColInfos[0].dir   = RDVECTOR_NEG3(pThing->orient.rvec);
    aColInfos[0].dir.z = 0.0f;
    rdVector_Normalize3Acc(&aColInfos[0].dir);

    sithVehicleControls_FindWallSurface(pThing, aColInfos, searchRadius);

    // Right with no Z
    aColInfos[1].dir   =  pThing->orient.rvec;
    aColInfos[1].dir.z = 0.0f;
    rdVector_Normalize3Acc(&aColInfos[1].dir);

    sithVehicleControls_FindWallSurface(pThing, &aColInfos[1], searchRadius);

    //
    // Backward directions
    // Note that straight backward is not searched
    //

    // TODO: Maybe add also check for straight backward

    // Backward Left and no Z
    aColInfos[5].dir = RDVECTOR_NEG3(pThing->orient.rvec);
    rdVector_MultAcc3(&aColInfos[5].dir, &pThing->orient.lvec, -1.0f);
    aColInfos[5].dir.z = 0.0f;
    rdVector_Normalize3Acc(&aColInfos[5].dir);

    sithVehicleControls_FindWallSurface(pThing, &aColInfos[5], searchRadius);

    // Backward Right and no Z
    aColInfos[6].dir = pThing->orient.rvec;
    rdVector_MultAcc3(&aColInfos[6].dir, &pThing->orient.lvec, -1.0f);
    aColInfos[6].dir.z = 0.0f;
    rdVector_Normalize3Acc(&aColInfos[6].dir);

    sithVehicleControls_FindWallSurface(pThing, &aColInfos[6], searchRadius);


    //
    // Now search for exit surface for each found wall surface
    //
    int infoIdx = -1;
    bool bFound = false;
    while ( !bFound )
    {
        // Find first satisfying collision
        infoIdx = -1;
        for ( int i = 0; i < STD_ARRAYLEN(aColInfos); ++i )
        {
            if ( aColInfos[i].pSurf )
            {
                if ( aColInfos[i].collisionDot < 1.0f )
                {
                    infoIdx = i;
                }
            }
        }

        if ( infoIdx == -1 )
        {
            break;
        }

        //
        // We got valid col index, now search for exit surface...
        //

        //
        // 1.) Find if there is any blocking surface 80 cm  up from player
        // Find sector at player position shifted for 80 cm up
        //

        float upDist = sithVehicleControls_raftUnbaordPosSearchRadius - sithPhysics_GetThingHeight(pThing) + 0.059999999f; // height for raft player should be 0.03f; the height should be 0.08f (80cm)
        rdVector3 topStartPos = pThing->pos;
        topStartPos.z = topStartPos.z + upDist;

        SithSector* pSec = sithCollision_FindSectorInRadius(pThing->pInSector, &pThing->pos, &topStartPos, 0.001f);
        if ( pSec )
        {
            //
            // Find blocking surface from up position in direction of found wall surface
            //
            rdVector3 moveNorm = RDVECTOR_NEG3(aColInfos[infoIdx].pSurf->face.normal);
            sithCollision_SearchForCollisions(pSec, pThing, &topStartPos, &moveNorm, searchRadius, sithVehicleControls_raftUnbaordPosSearchRadius, 0x02);

            SithCollision* pCollision = sithCollision_PopStack();
            sithCollision_DecreaseStackLevel();

            //
            // 2.) If no blocking surface is found, search for raft exit floor surface 
            //     in down direction at position topPosition + moveDist
            //

            if ( !pCollision )
            {
                rdVector3 downDir = RDVECTOR_NEG3(rdroid_g_zVector3);

                // Find sector at position topPosition + moveDist + 0.05f in direction of ledge wall surface
                rdVector3 searchStartPos;
                rdVector_ScaleAdd3(&searchStartPos, &moveNorm, searchRadius + 0.050000001f, &topStartPos);


                pSec = sithCollision_FindSectorInRadius(pSec, &topStartPos, &searchStartPos, 0.001f);
                if ( pSec )
                {
                    // Find exit surface in down dir from exitStartPos
                    float exitSearchDist = sithVehicleControls_raftUnbaordPosSearchRadius + 0.02f;
                    sithCollision_SearchForCollisions(pSec, pThing, &searchStartPos, &downDir, exitSearchDist, 0.001f, 0x02);
                    while ( (pCollision = sithCollision_PopStack()) != NULL )
                    {
                        // Check if valid exit floor surface
                        if ( (pCollision->type & SITHCOLLISION_WORLD) != 0
                            && (pCollision->type & SITHCOLLISION_FACE) != 0
                            && pCollision->pSurfaceCollided
                            && (pCollision->pSurfaceCollided->flags & SITH_SURFACE_ISFLOOR) != 0
                            && pCollision->pSurfaceCollided->face.normal.z >= 0.99980003f ) // up surfce
                        {
                            bFound = true;
                            break;
                        }
                    }

                    sithCollision_DecreaseStackLevel();
                }
            }
        }

        if ( !bFound )
        {
            aColInfos[infoIdx].pSurf = NULL;    // Null surface so the entry won't be selected in the for loop at the start of while scope
        }
    }

    if ( !bFound )
    {
        return bRight;
    }

    // rdVector_Dot3
    bRight = rdVector_Dot3(&aColInfos[infoIdx].pSurf->face.normal, &pThing->orient.rvec) <= 0.0f;

    *pUnboardNorm = aColInfos[infoIdx].pSurf->face.normal;
    rdVector_Zero3(pUnboardPos);

    // Average the vertices of the exit surface to find the center
    size_t i;
    for ( i = 0; i < aColInfos[infoIdx].pSurf->face.numVertices; ++i )
    {
        pUnboardPos->x = pUnboardPos->x + sithWorld_g_pCurrentWorld->aVertices[aColInfos[infoIdx].pSurf->face.aVertices[i]].x;
        pUnboardPos->y = pUnboardPos->y + sithWorld_g_pCurrentWorld->aVertices[aColInfos[infoIdx].pSurf->face.aVertices[i]].y;
        pUnboardPos->z = pUnboardPos->z + sithWorld_g_pCurrentWorld->aVertices[aColInfos[infoIdx].pSurf->face.aVertices[i]].z;
    }

    // Divide by number of vertices to get average and move outwards a bit in direction of surface normal
    rdVector_InvScale3Acc(pUnboardPos, (float)i); // divide by i
    rdVector_MultAcc3(pUnboardPos, pUnboardNorm, sithVehicleControls_raftUnboardPosOffset);

    return bRight;
}

void J3DAPI sithVehicleControls_FindWallSurface(SithThing* pThing, SithWallCollisionInfo* pColInfo, float moveDist)
{
    pColInfo->pSurf = NULL;
    sithCollision_SearchForCollisions(pThing->pInSector, pThing, &pThing->pos, &pColInfo->dir, moveDist, 0.001f, 2);
    SithCollision* pCollision = sithCollision_PopStack();

    if ( pCollision
        && (pCollision->type & SITHCOLLISION_WORLD) != 0
        && (pCollision->type & SITHCOLLISION_FACE) != 0
        && (pCollision->pSurfaceCollided->flags & SITH_SURFACE_ISFLOOR) == 0
        && pCollision->pSurfaceCollided->face.normal.z <= 0.02f )// almost vertical i.e. wall
    {
        float surfaceDot = rdVector_Dot3(&pCollision->pSurfaceCollided->face.normal, &pColInfo->dir);
        if ( surfaceDot < -0.90600002f ) // head-on collision
        {
            pColInfo->pSurf = pCollision->pSurfaceCollided;
            pColInfo->collisionDot = surfaceDot;
            pColInfo->distance = pCollision->distance;
        }
    }

    sithCollision_DecreaseStackLevel();
}

void J3DAPI sithVehicleControls_FadeJeepMoveSounds(SithThing* pThing)
{
    sithSoundClass_FadeModeVolume(pThing, SITHSOUNDCLASS_LWALKMETAL, 0.0f, 0.1f);
    sithSoundClass_FadeModeVolume(pThing, SITHSOUNDCLASS_RWALKMETAL, 0.0f, 0.1f);
}

bool J3DAPI sithVehicleControls_CanUnboardInDirection(SithThing* pThing, const rdVector3* pDir)
{
    //
    // 1.) Check if we can move in direction for 1.5m from current pos
    //
    sithCollision_SearchForCollisions(pThing->pInSector, pThing, &pThing->pos, pDir, 0.15000001f, 0.07f, 0x200);

    bool bCanMove = true;
    SithCollision* pCollision;
    while ( (pCollision = sithCollision_PopStack()) != NULL )
    {
        if ( (pCollision->type & SITHCOLLISION_THING) != 0 )
        {
            if ( pThing != pCollision->pThingCollided )
            {
                bCanMove = false;
            }
        }
        else if ( ((pCollision->type & SITHCOLLISION_ADJOINTOUCH) == 0
            || (pCollision->pSurfaceCollided->pAdjoin->flags & SITH_ADJOIN_NOPLAYERMOVE) != 0
            || (pCollision->pSurfaceCollided->pAdjoin->flags & SITH_ADJOIN_MOVE) == 0)
            && (pCollision->type & SITHCOLLISION_ADJOINCROSS) == 0 )
        {
            bCanMove = false;
        }
    }

    sithCollision_DecreaseStackLevel();

    if ( !bCanMove )
    {
        return false;
    }

    //
    // 2.) Ok, there was no obstruction. Now check if there is now obstruction in down direction at pos + dir * 1.5m
    //     and if the surface is flat enough to land on
    //

    rdVector3 downDir = RDVECTOR_NEG3(rdroid_g_zVector3);

    rdVector3 startPos;
    rdVector_ScaleAdd3(&startPos, pDir, 0.15000001f, &pThing->pos);

    SithSector* pSector = sithCollision_FindSectorInRadius(pThing->pInSector, &pThing->pos, &startPos, 0.0f);
    if ( !pSector )
    {
        return false;
    }

    bCanMove = false;

    float moveDist = sithPhysics_GetThingHeight(pThing) + 0.0049999999f;
    sithCollision_SearchForCollisions(pSector, NULL, &startPos, &downDir, moveDist, 0.001f, 0x200);
    while ( (pCollision = sithCollision_PopStack()) != NULL )
    {
        if ( (pCollision->type & SITHCOLLISION_THING) != 0 )
        {
            if ( pThing != pCollision->pThingCollided )
            {
                // There is thing object in the way
                bCanMove = false;
                break;
            }
        }
        else if ( (pCollision->type & SITHCOLLISION_WORLD) != 0 && (pCollision->type & SITHCOLLISION_FACE) != 0 )
        {
            // TODO: should this check be made also for non-jeep vehicles?
            if ( (pThing->moveInfo.physics.flags & SITH_PF_JEEP) != 0
                && pCollision->pSurfaceCollided->pAdjoin
                && (pCollision->pSurfaceCollided->pAdjoin->flags & SITH_ADJOIN_MOVE) == 0
                && (pCollision->pSurfaceCollided->pAdjoin->flags & SITH_ADJOIN_VISIBLE) != 0 )
            {
                bCanMove = false;
                break;
            }

            // Check if surface is flat enough to land on
            if ( pCollision->pSurfaceCollided->face.normal.z < 0.88999999f )
            {
                bCanMove = false;
                break;
            }

            bCanMove = true;
        }
    }

    sithCollision_DecreaseStackLevel();
    return bCanMove;
}

void J3DAPI sithVehicleControls_ExitRaft(SithThing* pRaftPlayer)
{
    sithVehicleControls_EndBoardCutscene(pRaftPlayer);

    // Enable inventory and disable raft repair kit
    sithInventory_SetSwimmingInventory(pRaftPlayer, /*bItemsAvailable=*/1);
    sithInventory_SetInventoryDisabled(pRaftPlayer, 56u, /*bDisabled*/1); // 56 - raft repair kit

    rdMatrix34 newOrient;
    rdVector3 newPos;
    if ( pRaftPlayer->moveStatus == SITHPLAYERMOVE_RAFT_UNBOARDING_LEFT )
    {
        newOrient.uvec = rdroid_g_zVector3;

        rdVector_Cross3(&newOrient.lvec, &pRaftPlayer->orient.lvec, &newOrient.uvec);
        rdVector_Normalize3Acc(&newOrient.lvec);

        rdVector_Cross3(&newOrient.rvec, &newOrient.lvec, &newOrient.uvec);
        rdVector_Normalize3Acc(&newOrient.rvec);

        rdVector_ScaleAdd3(&newPos, &pRaftPlayer->orient.rvec, -0.1f, &pRaftPlayer->pos);
    }
    else
    {
        newOrient.uvec = rdroid_g_zVector3;

        rdVector_Cross3(&newOrient.lvec, &newOrient.uvec, &pRaftPlayer->orient.lvec);
        rdVector_Normalize3Acc(&newOrient.lvec);

        rdVector_Cross3(&newOrient.rvec, &newOrient.lvec, &newOrient.uvec);
        rdVector_Normalize3Acc(&newOrient.rvec);

        rdVector_ScaleAdd3(&newPos, &pRaftPlayer->orient.rvec, 0.1f, &pRaftPlayer->pos);
    }

    // Adjust newPos Z
    SithThing* pPlayerThing = sithPlayer_g_aPlayers[SITHPLAYER_DEFAULTLOCALPLAYERNUM].pThing;

    float playerHeight = sithPhysics_GetThingHeight(pPlayerThing);
    float raftHeight   = sithPhysics_GetThingHeight(pRaftPlayer);
    newPos.z += playerHeight - raftHeight + 0.050000001f;

    // Find sector at newPos
    SithSector* pNewSector = sithCollision_FindSectorInRadius(pRaftPlayer->pInSector, &pRaftPlayer->pos, &newPos, 0.0f);
    if ( !pNewSector )
    {
        SITHLOG_ERROR("This is really bad, trying to exit vehicle to non-existent sector..\n");
    }

    // Send message to raft sector
    bool bSameSector = pNewSector != pRaftPlayer->pInSector;
    size_t raftPlayerNum = sithPlayer_g_playerNum;
    if ( bSameSector )
    {
        sithCog_SectorSendMessage(pRaftPlayer->pInSector, pRaftPlayer, SITHCOG_MSG_EXITED);
    }

    // Hide raft player
    sithPlayer_HidePlayer(raftPlayerNum);

    *pPlayerThing->thingInfo.actorInfo.pPlayer        = *pRaftPlayer->thingInfo.actorInfo.pPlayer;
    pPlayerThing->thingInfo.actorInfo.pPlayer->pThing = pPlayerThing;
    pPlayerThing->thingInfo.actorInfo.health          = pRaftPlayer->thingInfo.actorInfo.health;

    pPlayerThing->thingInfo.actorInfo.flags  &= ~SITH_AF_INVULNERABLE;
    if ( (pRaftPlayer->thingInfo.actorInfo.flags & SITH_AF_INVULNERABLE) != 0 )
    {
        pPlayerThing->thingInfo.actorInfo.flags |= SITH_AF_INVULNERABLE;
    }

    pPlayerThing->thingInfo.actorInfo.flags &= ~SITH_AF_POISONED;
    if ( (pRaftPlayer->thingInfo.actorInfo.flags & SITH_AF_POISONED) != 0 )
    {
        pPlayerThing->thingInfo.actorInfo.flags |= SITH_AF_POISONED;
    }

    // Set new position, sector, and make player 0 visible
    sithThing_ExitSector(pPlayerThing);
    sithPlayer_SetLocalPlayer(SITHPLAYER_DEFAULTLOCALPLAYERNUM);
    sithPlayer_ShowPlayer(SITHPLAYER_DEFAULTLOCALPLAYERNUM, 0);

    sithThing_SetPositionAndOrient(pPlayerThing, &newPos, &newOrient);
    sithThing_EnterSector(pPlayerThing, pNewSector, /*bNoWaterSplash=*/1, /*bNoNotify=*/!bSameSector);

    pPlayerThing->alpha      = 1.0f;
    pPlayerThing->moveStatus = SITHPLAYERMOVE_STILL;

    // Set new move mode, stick player to floor and change camera to 3rd person
    sithPuppet_SetMoveMode(pPlayerThing, SITHPUPPET_MOVEMODE_NORMAL);
    sithPhysics_FindFloor(pPlayerThing, /*bNoThingStateUpdate=*/1);
    sithCamera_SetCameraFocus(&sithCamera_g_aCameras[SITHCAMERA_EXTCAMERANUM], pPlayerThing, NULL);

    sithCog_ThingSendMessageEx(pRaftPlayer, NULL, SITHCOG_MSG_CALLBACK, 0, RDKEYMARKER_ACTIVATE, 0, 0);
    sithSoundClass_StopMode(pRaftPlayer, SITHSOUNDCLASS_JUMPWOOD);
}

void J3DAPI sithVehicleControls_UpdateRaftPaddleSoundFx(SithThing* pThing)
{
    rdVector3 orientDir;
    if ( pThing->moveStatus == SITHPLAYERMOVE_RAFT_PADDLE_FORWARD_LEFT || pThing->moveStatus == SITHPLAYERMOVE_RAFT_PADDLE_FORWARD_RIGHT )
    {
        orientDir = pThing->orient.lvec;
    }
    else
    {
        orientDir = RDVECTOR_NEG3(pThing->orient.lvec);
    }

    rdVector3 moveDir;
    float speed = rdVector_Normalize3(&moveDir, &pThing->moveInfo.physics.velocity);

    float threshold = 0.175f;
    if ( rdVector_Dot3(&pThing->orient.lvec, &moveDir) < 0.0f )
    {
        threshold = 0.1f;
    }

    bool bSplashRow = true;
    if ( speed > threshold )
    {
        if ( rdVector_Dot3(&moveDir, &orientDir) > threshold ) // why dot  and same as speed, should it be maybe > 0.0f?
        {
            bSplashRow = 0;
        }
    }

    if ( sithVehicleControls_raftRowSide > 0 )
    {
        if ( bSplashRow )
        {
            sithSoundClass_PlayModeFirst(pThing, SITHSOUNDCLASS_LWALKWATER);
        }
        else
        {
            sithSoundClass_PlayModeFirst(pThing, SITHSOUNDCLASS_LWALKPUDDLE);
        }
    }

    else if ( bSplashRow )
    {
        sithSoundClass_PlayModeFirst(pThing, SITHSOUNDCLASS_RWALKWATER);
    }
    else
    {
        sithSoundClass_PlayModeFirst(pThing, SITHSOUNDCLASS_RWALKPUDDLE);
    }

    sithVehicleControls_raftRowSide = -sithVehicleControls_raftRowSide;
}

void J3DAPI sithVehicleControls_StartBoardCutscene(SithThing* pThing)
{
    sithCog_ThingSendMessage(pThing, pThing, SITHCOG_MSG_USER0);
    sithPlayerControls_g_bCutsceneMode = 0;
}

void J3DAPI sithVehicleControls_EndBoardCutscene(SithThing* pThing)
{
    sithCog_ThingSendMessage(pThing, pThing, SITHCOG_MSG_USER1);
}

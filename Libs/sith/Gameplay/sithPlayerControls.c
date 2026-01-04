#include "sithPlayerControls.h"
#include <j3dcore/j3dhook.h>

#include <rdroid/Engine/rdPuppet.h>
#include <rdroid/Math/rdMath.h>
#include <rdroid/Math/rdMatrix.h>
#include <rdroid/Math/rdVector.h>
#include <rdroid/Primitives/rdModel3.h>

#include <sith/Cog/sithCog.h>
#include <sith/Devices/sithControl.h>
#include <sith/Devices/sithSoundMixer.h>
#include <sith/Engine/sithCamera.h>
#include <sith/Engine/sithCollision.h>
#include <sith/Engine/sithPuppet.h>
#include <sith/Engine/sithPhysics.h>
#include <sith/Gameplay/sithFX.h>
#include <sith/Gameplay/sithInventory.h>
#include <sith/Gameplay/sithOverlayMap.h>
#include <sith/Gameplay/sithPlayer.h>
#include <sith/Gameplay/sithPlayerActions.h>
#include <sith/Gameplay/sithTime.h>
#include <sith/Gameplay/sithVehicleControls.h>
#include <sith/Gameplay/sithWhip.h>
#include <sith/RTI/symbols.h>
#include <sith/World/sithActor.h>
#include <sith/World/sithModel.h>
#include <sith/World/sithSoundClass.h>
#include <sith/World/sithThing.h>
#include <sith/World/sithWeapon.h>
#include <sith/World/sithWorld.h>

#include <sound/Sound.h>

#include <std/General/stdMath.h>
#include <std/General/stdUtil.h>

#define SITHPLAYERCONTROLS_THINGNAME_MINECAR       "minecar"
#define SITHPLAYERCONTROLS_THINGNAME_MINECARPLAYER "mineplayer"

#define SITHPLAYERCONTROLS_THINGNAME_JEEP       "jeep"
#define SITHPLAYERCONTROLS_THINGNAME_JEEPPLAYER "jeepplayer"

#define SITHPLAYERCONTROLS_THINGNAME_JEEP2       "jeep_pyr"
#define SITHPLAYERCONTROLS_THINGNAME_JEEPPLAYER2 "jeepplayerpyr"

// General move vars
static tStdTime sithPlayerControls_msecUnknownTimer = 0; // Fixed: Init to 0
static float sithPlayerControls_secCommentWaitTimer = 0.0f; // Fixed: Init to 0.0f

static bool sithPlayerControls_bJumpKeyActive = false; // Fixed: Init to false
static int sithPlayerControls_curJumpDirection;

bool sithPlayerControls_bActionKeyActive           = false; // Fixed: Init to false
static bool sithPlayerControls_bLookKeyActive      = false; // Fixed: Init to false
static bool sithPlayerControls_bHealthKeyActive    = false; // Fixed: Init to false
static bool sithPlayerControls_bTurnRightKeyActive = false; // Fixed: Init to false
static bool sithPlayerControls_bTurnLeftKeyActive  = false; // Fixed: Init to false

static SithThingMoveStatus sithPlayerControls_curMoveStatus;
static SithThing* sithPlayerControls_pCurActivatedItemThing = NULL; // Fixed: Init to NULL
static SithThing* sithPlayerControls_pMovableThing          = NULL; // Fixed: Init to NULL

static float sithPlayerControls_moveFactorNormal  = 0.89999998f;
static float sithPlayerControls_moveFactorSlow    = 0.5f;

// Climb controls vars
static int sithPlayerControls_climbPupTrackNum;

// Swim controls vars
static float sithPlayerControls_secSwimIdleTime   = 0.0f; // Fixed: Init to 0.0f
static float sithPlayerControls_secSwimBoostTimer = 0.0f; // Added
static float sithPlayerControls_swimPitchTurnRate = 60.0f;
static float sithPlayerControls_swimMaxPitchAngle = 75.0f;
static float sithPlayerControls_swimYawTurnRate   = 75.0f;

// Boarding/Unboarding vars
static float sithPlayerControls_minecarUnboardDist        = 0.079999998f;
static float sithPlayerControls_vehicleUnboardDist        = 0.12f;
static SithThing* sithPlayerControls_pBoardedVehicleThing = NULL; // Fixed: Init to NULL

// Auto-aim joint rotation offsets
static float sithPlayerControls_aimRotSmoothRate = 10.0f; // Added
static float sithPlayerControls_pistolYawOffset  = 3.0f;
static float sithPlayerControls_rifleYawOffset   = 1.0f;

// Auto-Aiming constants
static SithThing* sithPlayerControls_pTargetThing = NULL; // Fixed: Init to NULL

static float sithPlayerControls_closeRangeAimDistance = 0.30000001f;

static float sithPlayerControls_aimMaxHorizontalAngleAcquire  = 20.5f; // Max horizontal angle for aim acquisition
static float sithPlayerControls_aimMaxHorizontalAngleTrack    = 22.5f; // Max horizontal angle for locked aim tracking
static float sithPlayerControls_aimMaxVerticalAngleAcquire    = 87.0f; // Max vertical angle for aim acquisition
static float sithPlayerControls_aimMaxVerticalAngleTrack      = 89.0f; // Max vertical angle for locked aim tracking
static float sithPlayerControls_aimMaxHorizontalAngleNear     = 50.0f; // Max horizontal angle at close range aiming

static float sithPlayerControls_mirrorAimMaxHorizontalAngleAcquire = 3.0f;
static float sithPlayerControls_mirrorAimMaxHorizontalAngleTrack   = 5.0f;
static float sithPlayerControls_mirrorAimMaxVerticalAngleAcquire   = 14.0f;
static float sithPlayerControls_mirrorAimMaxVerticalAngleTrack     = 16.0f;
static float sithPlayerControls_mirrorAimMaxHorizontalAngleNear    = 5.0f;

// Devmode vars
static rdVector3 sithPlayerControls_curOrbCamDir  = { 0.0f , -1.0f , 0.0f };
static float sithPlayerControls_curOrbCamDist     = 0.2f;
static float sithPlayerControls_maxOrbCamDist     = 10.0f; // Altered: Changed to 10.0f from 3.0f

static int J3DAPI sithPlayerControls_ProcessPlayerDebugControls(SithThing* pThing, float secDeltaTime);
static void J3DAPI sithPlayerControls_ProcessLookControls(SithThing* pThing, float secDeltaTime);
static void J3DAPI sithPlayerControls_ProcessGeneralMove(SithThing* pThing, float secDeltaTime);
static void J3DAPI sithPlayerControls_ProcessClimbMove(SithThing* pThing, float secDeltaTime);
static void J3DAPI sithPlayerControls_ProcessJewelFlyMove(SithThing* pThing, float secDeltaTime);
static void J3DAPI sithPlayerControls_ProcessFlyMove(SithThing* pThing, float secDeltaTime);
static void J3DAPI sithPlayerControls_ProcessHUDControls(SithThing* pThing, float secDeltaTime);
static int J3DAPI sithPlayerControls_ProcessEditorDebugControls(SithThing* pThing, float secDeltaTime);
static int J3DAPI sithPlayerControls_ProcessDeadPlayer(SithThing* pThing, float secDeltaTime);
static void J3DAPI sithPlayerControls_ProcessFallingMove(SithThing* pThing, float secDeltaTime);
static void J3DAPI sithPlayerControls_ProcessSwimMove(SithThing* pThing, float secDeltaTime);
static void J3DAPI sithPlayerControls_ProcessHangMove(SithThing* pThing, float secDeltaTime);
static void J3DAPI sithPlayerControls_ProcessWeaponAim(SithThing* pThing, float secDeltaTime);

static int J3DAPI sithPlayerControls_CheckAimRange(SithThing* pThing, const rdVector3* pStartPos, const rdVector3* pTargetPos, float cosFarAimCone, float cosCloseAimCone, float cosVerticalLimit, float maxAimDist);
static int J3DAPI sithPlayerControls_GetPushPullMoveNorm(rdVector3* moveNorm, const rdVector3* pLVect, float angle);

// Helper function signatures for ProcessGeneralMove (new functions)
static void J3DAPI sithPlayerControls_ProcessStillMove(SithThing* pThing, float secDeltaTime, float moveFactor, bool bRun);
static void J3DAPI sithPlayerControls_ProcessWalkMove(SithThing* pThing, float secDeltaTime, float moveFactor, bool bRun);
static void J3DAPI sithPlayerControls_ProcessRunMove(SithThing* pThing, float secDeltaTime, float moveFactor, bool bRun);
static void J3DAPI sithPlayerControls_ProcessCrawlMove(SithThing* pThing, float secDeltaTime, float moveFactor);
static void J3DAPI sithPlayerControls_ProcessPushPullMove(SithThing* pThing, float secDeltaTime);
static void J3DAPI sithPlayerControls_ProcessSlideDownMove(SithThing* pThing, float secDeltaTime);

/**
 * Check for ledge when in water
 * @param pThing - thing to search for ledge from
 * @return
 *  0 - no ledge found
 *  1 - solid water ledge found, the thing will be positioned to ledge
 *  2 - adjoin water ledge found
 */
static int J3DAPI sithPlayerControls_CheckWaterLedge(SithThing* pThing);
static int J3DAPI sithPlayerControls_FindLedgeInDirection(SithThing* pThing, const rdVector3* moveNorm, SithSurface** ppHitSurf, SithThing** ppHitThing, rdModel3** ppHitModel, rdFace** ppHitFace, rdModel3Mesh** ppHitMesh, int someType);
static SithSurface* J3DAPI sithPlayerControls_FindClimbSurface(SithThing* pThing, const rdVector3* moveNorm, int climbDir, int* pbHitNonClimbSurface);
static bool J3DAPI sithPlayerControls_CanStrafeMove(SithThing* pThing, int bMoveRight);

void sithPlayerControls_InstallHooks(void)
{
    J3D_HOOKFUNC(sithPlayerControls_PuppetCallback);
    J3D_HOOKFUNC(sithPlayerControls_Process);
    J3D_HOOKFUNC(sithPlayerControls_ProcessPlayerDebugControls);
    J3D_HOOKFUNC(sithPlayerControls_ProcessLookControls);
    J3D_HOOKFUNC(sithPlayerControls_ProcessGeneralMove);
    J3D_HOOKFUNC(sithPlayerControls_ProcessClimbMove);
    J3D_HOOKFUNC(sithPlayerControls_ProcessJewelFlyMove);
    J3D_HOOKFUNC(sithPlayerControls_ProcessFlyMove);
    J3D_HOOKFUNC(sithPlayerControls_ProcessHUDControls);
    J3D_HOOKFUNC(sithPlayerControls_ProcessEditorDebugControls);
    J3D_HOOKFUNC(sithPlayerControls_ProcessDeadPlayer);
    J3D_HOOKFUNC(sithPlayerControls_ProcessFallingMove);
    J3D_HOOKFUNC(sithPlayerControls_ProcessSwimMove);
    J3D_HOOKFUNC(sithPlayerControls_ProcessHangMove);
    J3D_HOOKFUNC(sithPlayerControls_ProcessWeaponAim);
    J3D_HOOKFUNC(sithPlayerControls_CheckAimRange);
    J3D_HOOKFUNC(sithPlayerControls_GetPushPullMoveNorm);
    J3D_HOOKFUNC(sithPlayerControls_CheckWaterLedge);
    J3D_HOOKFUNC(sithPlayerControls_FindLedgeInDirection);
    J3D_HOOKFUNC(sithPlayerControls_FindClimbSurface);
    J3D_HOOKFUNC(sithPlayerControls_BoardVehicle);
    J3D_HOOKFUNC(sithPlayerControls_ExitVehicle);
    J3D_HOOKFUNC(sithPlayerControls_RotateAimJoints);
    J3D_HOOKFUNC(sithPlayerControls_ResetAimJoints);
    J3D_HOOKFUNC(sithPlayerControls_GetTargetThing);
    J3D_HOOKFUNC(sithPlayerControls_GetVehicleBoardedThing);
    J3D_HOOKFUNC(sithPlayerControls_SetVehicleBoardedThing);
    J3D_HOOKFUNC(sithPlayerControls_CanStrafeMove);
}

void sithPlayerControls_ResetGlobals(void)
{
    sithPlayerControls_g_bCutsceneMode = 0;
}

void sithPlayerControls_Reset(void)
{
    sithPlayerControls_g_bCutsceneMode = 0;

    sithPlayerControls_msecUnknownTimer       = 0;
    sithPlayerControls_secCommentWaitTimer    = 0.0f;

    sithPlayerControls_bJumpKeyActive         = false;
    sithPlayerControls_curJumpDirection       = 0;
    sithPlayerControls_bActionKeyActive       = false;
    sithPlayerControls_bLookKeyActive         = false;
    sithPlayerControls_bHealthKeyActive       = false;
    sithPlayerControls_bTurnRightKeyActive    = false;
    sithPlayerControls_bTurnLeftKeyActive     = false;

    sithPlayerControls_curMoveStatus          = 0;
    sithPlayerControls_pCurActivatedItemThing = NULL;
    sithPlayerControls_pMovableThing          = NULL;

    sithPlayerControls_climbPupTrackNum       = -1;

    sithPlayerControls_pBoardedVehicleThing   = NULL;
    sithPlayerControls_pTargetThing           = NULL;

    sithPlayerControls_curOrbCamDir           = (rdVector3){ 0.0f, -1.0f, 0.0f };
    sithPlayerControls_curOrbCamDist          = 0.2f;
}

void sithPlayerControls_StopMovementEx(SithThing* pThing, SithPuppetSubMode mode, rdPuppetTrackCallback pfCallback) // New
{
    // Stops thing movement, clears mode and plays stand mode

    pThing->moveStatus = SITHPLAYERMOVE_STILL;
    sithPhysics_ResetThingMovement(pThing);

    sithPuppet_ClearMode(pThing, mode);
    sithPuppet_PlayMode(pThing, SITHPUPPETSUBMODE_STAND, pfCallback);
}

void sithPlayerControls_StopMovement(SithThing* pThing) // New
{
    sithPlayerControls_StopMovementEx(pThing, pThing->pPuppetState->submode, NULL);
}

void J3DAPI sithPlayerControls_PuppetCallback(SithThing* pThing, int track, rdKeyMarkerType markerType)
{
    if ( !pThing->thingInfo.actorInfo.bControlsDisabled )
    {
        return;
    }

    if ( pThing->moveStatus == SITHPLAYERMOVE_PUSHPULL_READY && markerType == RDKEYMARKER_CRAWL )
    {
        pThing->thingInfo.actorInfo.bControlsDisabled = 0;
        return;
    }

    if ( pThing->moveStatus == SITHPLAYERMOVE_WALK2STAND && !markerType )
    {
        pThing->moveStatus = SITHPLAYERMOVE_STILL;
        pThing->thingInfo.actorInfo.bControlsDisabled = 0;
        sithPuppet_PlayMode(pThing, SITHPUPPETSUBMODE_STAND, NULL);
        return;
    }

    if ( pThing->moveStatus == SITHPLAYERMOVE_STAND2WALK && !markerType )
    {
        pThing->moveStatus = SITHPLAYERMOVE_WALKING;
        pThing->thingInfo.actorInfo.bControlsDisabled = 0;
        return;
    }

    if ( pThing->moveStatus == SITHPLAYERMOVE_STAND2RUN && !markerType )
    {
        pThing->moveStatus = SITHPLAYERMOVE_RUNNING;
        pThing->thingInfo.actorInfo.bControlsDisabled = 0;
        return;
    }

    switch ( pThing->moveStatus )
    {
        case SITHPLAYERMOVE_JEEP_IDLE:
        case SITHPLAYERMOVE_MINECAR_IDLE:
            sithPlayerControls_ExitVehicle(pThing);
            return;

        case SITHPLAYERMOVE_LAND:
            if ( !markerType )
            {
                pThing->thingInfo.actorInfo.bControlsDisabled = 0;
                sithPuppet_FreeTrackByIndex(pThing, track);
                pThing->moveStatus = SITHPLAYERMOVE_STILL;
                return;
            }
            sithPuppet_DefaultCallback(pThing, track, markerType);
            return;

        case SITHPLAYERMOVE_ACTIVATING:
            switch ( markerType )
            {
                case RDKEYMARKER_ACTIVATERIGHTARM:
                    if ( sithPlayerControls_pCurActivatedItemThing )
                    {
                        sithCog_ThingSendMessage(sithPlayerControls_pCurActivatedItemThing, pThing, SITHCOG_MSG_TOUCHED);
                    }
                    else
                    {
                        sithFX_CreateChalkMark(&sithPlayerActions_g_chalkWritePos, sithPlayerActions_g_pChalkWriteSurf, sithPlayerActions_g_pChalkWriteSector);
                        sithSoundClass_PlayModeFirst(pThing, SITHSOUNDCLASS_CALM);
                    }
                    return;

                case RDKEYMARKER_ACTIVATERIGHTARMREST:
                    sithCamera_RestoreExtCamera();
                    return;

                case 0:
                    pThing->thingInfo.actorInfo.bControlsDisabled = 0;
                    sithPuppet_RemoveTrackByIndex(pThing, track);
                    pThing->moveStatus = sithPlayerControls_curMoveStatus;

                    if ( !sithPlayerControls_pCurActivatedItemThing )
                    {
                        sithCog_ThingSendMessage(pThing, pThing, SITHCOG_MSG_USER7);
                    }

                    sithPlayerControls_pCurActivatedItemThing = NULL;
                    pThing->thingInfo.actorInfo.flags &= ~SITH_AF_CONTROLSDISABLED;
                    return;
            }
            sithPuppet_DefaultCallback(pThing, track, markerType);
            return;

        case SITHPLAYERMOVE_MOUNTING_WALL:
            pThing->thingInfo.actorInfo.bControlsDisabled = 0;
            pThing->moveStatus = SITHPLAYERMOVE_CLIMBIDLE;
            sithPuppet_PlayMode(pThing, SITHPUPPETSUBMODE_CLIMBWALLIDLE, NULL);
            sithPuppet_FreeTrackByIndex(pThing, track);
            return;

        case SITHPLAYERMOVE_CLIMBIDLE:
            pThing->thingInfo.actorInfo.bControlsDisabled = 0;
            return;

        case SITHPLAYERMOVE_PULLINGUP_1M:
            if ( markerType )
            {
                sithPuppet_DefaultCallback(pThing, track, markerType);
                return;
            }

            pThing->moveStatus = SITHPLAYERMOVE_STILL;
            if ( pThing->pPuppetState->moveMode )
            {
                sithPuppet_SetMoveMode(pThing, SITHPUPPET_MOVEMODE_NORMAL);
            }
            break;

        case SITHPLAYERMOVE_JUMP_READY:
            if ( !markerType )
            {
                pThing->thingInfo.actorInfo.bControlsDisabled = 0;
                sithPuppet_DefaultCallback(pThing, track, (rdKeyMarkerType)0);

                if ( sithPlayerControls_curJumpDirection == 99 )
                {
                    sithPlayerActions_ClimbOn2m(pThing);
                }
                else if ( sithPlayerControls_curJumpDirection )
                {
                    sithPlayerActions_Jump(pThing, 0.76999998f, sithPlayerControls_curJumpDirection);
                }
                else
                {
                    sithPlayerActions_Jump(pThing, 0.60000002f, sithPlayerControls_curJumpDirection);
                }

                sithPlayerControls_curJumpDirection = 0;
                return;
            }
            sithPuppet_DefaultCallback(pThing, track, markerType);
            return;

        case SITHPLAYERMOVE_SLIDEDOWNFORWARD:
        case SITHPLAYERMOVE_SLIDEDOWNBACK:
            if ( markerType )
            {
                sithPuppet_DefaultCallback(pThing, track, markerType);
                return;
            }

            pThing->thingInfo.actorInfo.bControlsDisabled = 0;

            if ( pThing->pInSector
                && ((pThing->pInSector->flags & SITH_SECTOR_UNDERWATER) != 0
                    || (pThing->pInSector->flags & SITH_SECTOR_AETHERIUM) != 0) )
            {
                pThing->moveStatus = SITHPLAYERMOVE_SWIMIDLE;
                if ( pThing->pPuppetState->moveMode != SITHPUPPET_MOVEMODE_SWIM )
                {
                    sithPuppet_SetMoveMode(pThing, SITHPUPPET_MOVEMODE_SWIM);
                }
            }
            else
            {
                pThing->moveStatus = SITHPLAYERMOVE_STILL;
                if ( pThing->pPuppetState->moveMode )
                {
                    sithPuppet_SetMoveMode(pThing, SITHPUPPET_MOVEMODE_NORMAL);
                }
            }
            break;

        case SITHPLAYERMOVE_WHIPSWINGING:
            switch ( markerType )
            {
                case RDKEYMARKER_ATTACK:
                {
                    tSoundHandle hSnd = Sound_GetSoundHandle(SITHWORLD_STATICINDEX(3u)); // 0x8003 - gen_whip_fire.wav
                    if ( hSnd )
                    {
                        sithSoundMixer_PlaySound(hSnd, 1.0f, 0.0f, (SoundPlayFlag)0);
                    }
                    break;
                }

                case RDKEYMARKER_SWING:
                {
                    tSoundHandle hSnd = Sound_GetSoundHandle(SITHWORLD_STATICINDEX(100u)); // 0x8064 - gen_whipswing.wav
                    if ( hSnd )
                    {
                        sithSoundMixer_PlaySound(hSnd, 1.0f, 0.0f, (SoundPlayFlag)0);
                    }
                } break;

                case RDKEYMARKER_ACTIVATERIGHTARMREST:
                {
                    tSoundHandle hSnd = Sound_GetSoundHandle(SITHWORLD_STATICINDEX(24u)); // 0x8018 - fol_in_lrunhard.wav
                    sithCamera_RestoreExtCamera();

                #ifdef J3D_QOL_IMPROVEMENTS
                    // Added
                    // Player has landed after whip swing, check if height distance to solid floor is more than 0.2m.
                    // And in this case stop animation and make player fall
                    rdVector3 downDir = RDVECTOR_NEG3(rdroid_g_zVector3);
                    if ( sithCollision_CheckFloorDistance(pThing, &downDir) > (0.09f + 0.02f) ) // if more than 20cm from ground. Note 0.09 (90cm)is height from bottom to center point of standing indy
                    {
                        // Hight to floor is more than 0.2m, stop animation and make indy fall.
                        sithPuppet_FinishForceMove(pThing, /*bStopTracks=*/1);

                        // Give a gentle push in forward direction, so forward fall animation will play
                        rdVector_Scale3(&pThing->moveInfo.physics.velocity, &pThing->orient.lvec, 0.25f);

                        pThing->moveStatus = SITHPLAYERMOVE_FALLING;
                        sithPuppet_PlayMode(pThing, SITHPUPPETSUBMODE_FALL, NULL);

                        hSnd = 0; // don't play land sound
                    }
                #endif
                    // Altered: Moved following if scope here form start of the case sope
                    if ( hSnd )
                    {
                        sithSoundMixer_PlaySound(hSnd, 1.0f, 0.0f, (SoundPlayFlag)0);
                    }
                }  break;

                case 0:
                    pThing->thingInfo.actorInfo.bControlsDisabled = 0;
                    sithWhip_FinishWhipSwing(pThing);
                    break;
            }
            break;

        case SITHPLAYERMOVE_WHIPCLIMB_START:
            sithWhip_SetActorWhipClimbIdle(pThing);
            break;

        default:
            if ( pThing->moveStatus == SITHPLAYERMOVE_HANGING
                || pThing->moveStatus == SITHPLAYERMOVE_STILL )
            {
                pThing->thingInfo.actorInfo.bControlsDisabled = 0;
            }
            break;
    }
}

int J3DAPI sithPlayerControls_Process(SithThing* pPlayerThing, float secDeltaTime)
{
    SithPhysicsInfo* pPhysics = &pPlayerThing->moveInfo.physics;
    SithActorInfo* pActor     = &pPlayerThing->thingInfo.actorInfo;

    if ( sithPlayerControls_g_bCutsceneMode )
    {
        return 0;
    }

    if ( sithPlayerControls_msecUnknownTimer > 0 )
    {
        sithPlayerControls_msecUnknownTimer -= (tStdTime)(secDeltaTime * 1000.0f);
        if ( sithPlayerControls_msecUnknownTimer < 0 )
        {
            sithPlayerControls_msecUnknownTimer = 0;
        }
    }

    if ( pPlayerThing->moveType != SITH_MT_PHYSICS )
    {
        return 0;
    }

    if ( (sithMain_g_sith_mode.debugModeFlags & SITHDEBUG_INEDITOR) != 0
        && sithPlayerControls_ProcessEditorDebugControls(pPlayerThing, secDeltaTime) )
    {
        return 1;
    }

    if ( (pPlayerThing->flags & SITH_TF_DYING) != 0 )
    {
        sithPlayerControls_ProcessDeadPlayer(pPlayerThing, secDeltaTime);
        return 0;
    }

    if ( sithPlayerControls_secCommentWaitTimer != 0.0f )
    {
        sithPlayerControls_secCommentWaitTimer = J3DMAX(sithPlayerControls_secCommentWaitTimer - secDeltaTime, 0.0f);
    }

    if ( sithTime_g_bPaused == 1 )
    {
        return 1;
    }

    if ( pPlayerThing->moveStatus == SITHPLAYERMOVE_SWIMIDLE )
    {
        sithPlayerControls_secSwimIdleTime += secDeltaTime;
    }
    else
    {
        sithPlayerControls_secSwimIdleTime = 0.0f;
    }

    // Added: Decrement swim boost timer
    if ( sithPlayerControls_secSwimBoostTimer > 0.0f )
    {
        sithPlayerControls_secSwimBoostTimer = J3DMAX(sithPlayerControls_secSwimBoostTimer - secDeltaTime, 0.0f);
    }

    if ( sithPuppet_g_bPlayerLeapForward && pPlayerThing->moveStatus != SITHPLAYERMOVE_RUNNING )
    {
        sithPuppet_g_bPlayerLeapForward = 0;
    }

    if ( sithPlayerControls_ProcessPlayerDebugControls(pPlayerThing, secDeltaTime) )
    {
        return 1;
    }

    // Reset control activation flags when keys released
    if ( sithPlayerControls_bActionKeyActive && !sithControl_GetKey(SITHCONTROL_ACT2, NULL) )
    {
        sithPlayerControls_bActionKeyActive = false;
    }

    if ( sithPlayerControls_bTurnLeftKeyActive && !sithControl_GetKey(SITHCONTROL_TURNLEFT, NULL) )
    {
        sithPlayerControls_bTurnLeftKeyActive = false;
    }

    if ( sithPlayerControls_bTurnRightKeyActive && !sithControl_GetKey(SITHCONTROL_TURNRIGHT, NULL) )
    {
        sithPlayerControls_bTurnRightKeyActive = false;
    }

    if ( sithPlayerControls_bJumpKeyActive && !sithControl_GetKey(SITHCONTROL_JUMP, NULL) )
    {
        sithPlayerControls_bJumpKeyActive = false;
    }

    if ( sithPlayerControls_bHealthKeyActive && !sithControl_GetKey(SITHCONTROL_HEALTH, NULL) )
    {
        sithPlayerControls_bHealthKeyActive = false;
    }

    if ( (sithMain_g_sith_mode.debugModeFlags & SITHDEBUG_INEDITOR) == 0 )
    {
        // Check if Look key is pressed & process movement
        if ( sithControl_GetKey(SITHCONTROL_LOOK, NULL) )
        {
            if ( !sithPlayerControls_g_bCutsceneMode
                && !sithPlayerControls_bLookKeyActive
                && rdVector_IsZero3(&pPlayerThing->moveInfo.physics.velocity)
                && rdVector_IsZero3(&pPlayerThing->moveInfo.physics.angularVelocity) )
            {
                int curWeapon = sithInventory_GetCurrentWeapon(pPlayerThing);
                if ( curWeapon != SITHWEAPON_WHIP || !sithWeapon_IsAiming(pPlayerThing) )
                {
                    pActor->flags |= SITH_AF_VIEWCENTRING;
                    rdVector_Zero3(&pActor->headPYR);
                    sithActor_SetHeadPYR(pPlayerThing, &pActor->headPYR);

                    sithPlayerControls_bLookKeyActive    = true;
                    sithCamera_g_bExtCameraLookMode = 1;

                    if ( pPlayerThing->moveStatus == SITHPLAYERMOVE_CRAWLIDLE )
                    {
                        rdVector3 camLookOffset = { 0.0f, 0.0f, 0.025f };
                        sithCamera_SetExtCameraOffset(&camLookOffset);

                        camLookOffset = (rdVector3){ 0.0f, 0.025f, 0.025f };
                        sithCamera_SetExtCameraLookOffset(&camLookOffset);
                    }
                    else
                    {
                        rdVector3 camLookOffset = { 0.0f, 0.0f, 0.059999999f };
                        sithCamera_SetExtCameraOffset(&camLookOffset);

                        camLookOffset = (rdVector3){ 0.0f, 0.039999999f, 0.059999999f };
                        sithCamera_SetExtCameraLookOffset(&camLookOffset);
                    }
                }
            }
        }
        else if ( sithPlayerControls_bLookKeyActive == 1 )
        {
            pActor->flags |= SITH_AF_VIEWCENTRING;
            rdVector_Zero3(&pActor->headPYR);
            sithActor_SetHeadPYR(pPlayerThing, &pActor->headPYR);

            sithPlayerControls_bLookKeyActive = false;
            sithCamera_RestoreExtCamera();
            sithCamera_g_bExtCameraLookMode = 0;
        }
    }

    //
    // Update weapon aim and look
    //
    sithPlayerControls_ProcessWeaponAim(pPlayerThing, secDeltaTime);

    //
    // Return early if controls are disabled
    //
    if ( pPlayerThing->type == SITH_THING_PLAYER
        && (pPlayerThing->thingInfo.actorInfo.flags & SITH_AF_CONTROLSDISABLED) != 0 )
    {
        return 0;
    }

    //
    // Process Look controls if Look key is active
    //
    if ( sithPlayerControls_bLookKeyActive == true
        && (sithMain_g_sith_mode.debugModeFlags & SITHDEBUG_INEDITOR) == 0 )
    {
        sithPlayerControls_ProcessLookControls(pPlayerThing, secDeltaTime);
        return 0;
    }

    // 
    // Process movement based on physics flags and move status
    //

    if ( (pPhysics->flags & SITH_PF_FLY) != 0
        && pPlayerThing->moveStatus != SITHPLAYERMOVE_JEWELFLYING )
    {
        sithPlayerControls_ProcessFlyMove(pPlayerThing, secDeltaTime);
        sithPlayerControls_ProcessHUDControls(pPlayerThing, secDeltaTime);
        return 0;
    }

    if ( (pPhysics->flags & SITH_PF_JEEP) != 0 )
    {
        sithVehicleControls_ProcessJeepMove(pPlayerThing, secDeltaTime);
        sithPlayerControls_ProcessHUDControls(pPlayerThing, secDeltaTime);
        return 0;
    }

    if ( (pPhysics->flags & SITH_PF_RAFT) != 0 )
    {
        sithVehicleControls_ProcessRaftMove(pPlayerThing, secDeltaTime);
        sithPlayerControls_ProcessHUDControls(pPlayerThing, secDeltaTime);
        return 0;
    }

    if ( (pPhysics->flags & SITH_PF_MINECAR) != 0 )
    {
        sithVehicleControls_ProcessMineCarMove(pPlayerThing, secDeltaTime);
        sithPlayerControls_ProcessHUDControls(pPlayerThing, secDeltaTime);
        return 0;
    }

    if ( (pPhysics->flags & (SITH_PF_JEEP | SITH_PF_RAFT | SITH_PF_MINECAR)) == 0
        && !pPlayerThing->thingInfo.actorInfo.bForceMovePlay )
    {
        switch ( pPlayerThing->moveStatus )
        {
            case SITHPLAYERMOVE_CRAWLIDLE:
            case SITHPLAYERMOVE_UNKNOWN_4:
            case SITHPLAYERMOVE_UNKNOWN_5:
            case SITHPLAYERMOVE_CLIMBIDLE:
            case SITHPLAYERMOVE_SWIMIDLE:
            case SITHPLAYERMOVE_MOUNTING_WALL:
            case SITHPLAYERMOVE_CLIMBING_UP:
            case SITHPLAYERMOVE_CLIMBING_DOWN:
            case SITHPLAYERMOVE_CLIMBING_LEFT:
            case SITHPLAYERMOVE_CLIMBING_RIGHT:
            case SITHPLAYERMOVE_STAND_TO_CRAWL:
            case SITHPLAYERMOVE_CRAWL_TO_STAND:
                break;

            default:
                sithPhysics_SetThingLook(pPlayerThing, &rdroid_g_zVector3, 0.0f);
                break;
        }
    }

    switch ( pPlayerThing->moveStatus )
    {
        case SITHPLAYERMOVE_STILL:
        case SITHPLAYERMOVE_WALKING:
        case SITHPLAYERMOVE_RUNNING:
        case SITHPLAYERMOVE_CRAWLIDLE:
        case SITHPLAYERMOVE_JUMP_READY:
        case SITHPLAYERMOVE_PUSHPULL_READY:
        case SITHPLAYERMOVE_SLIDEDOWNFORWARD:
        case SITHPLAYERMOVE_SLIDEDOWNBACK:
            if ( (pPlayerThing->pInSector->flags & (SITH_SECTOR_UNDERWATER | SITH_SECTOR_AETHERIUM)) != 0 )
            {
                pPlayerThing->moveStatus = SITHPLAYERMOVE_SWIMIDLE;
                sithPlayerControls_secSwimIdleTime = 0.0f;
                sithPlayerControls_ProcessSwimMove(pPlayerThing, secDeltaTime);
            }
            else
            {
                sithPlayerControls_ProcessGeneralMove(pPlayerThing, secDeltaTime);
            }
            break;

        case SITHPLAYERMOVE_HANGING:
            sithPlayerControls_ProcessHangMove(pPlayerThing, secDeltaTime);
            break;

        case SITHPLAYERMOVE_CLIMBIDLE:
        case SITHPLAYERMOVE_CLIMBING_UP:    // Added: Was added so the climbing speed can be updated
        case SITHPLAYERMOVE_CLIMBING_DOWN:  // Added: Was added so the climbing speed can be updated
        case SITHPLAYERMOVE_CLIMBING_LEFT:  // Added: Was added so the climbing speed can be updated
        case SITHPLAYERMOVE_CLIMBING_RIGHT: // Added: Was added so the climbing speed can be updated
            sithPlayerControls_ProcessClimbMove(pPlayerThing, secDeltaTime);
            break;

        case SITHPLAYERMOVE_WHIPCLIMBIDLE:
        case SITHPLAYERMOVE_WHIPCLIMB_START:
        case SITHPLAYERMOVE_WHIP_UNK2:
            sithWhip_ProcessWhipClimbMove(pPlayerThing, secDeltaTime);
            break;

        case SITHPLAYERMOVE_SWIMIDLE:
            sithPlayerControls_ProcessSwimMove(pPlayerThing, secDeltaTime);
            break;

        case SITHPLAYERMOVE_JUMPFWD:
        case SITHPLAYERMOVE_JUMPUP:
        case SITHPLAYERMOVE_FALLING:
        case SITHPLAYERMOVE_LEAPFWD:
            if ( (pPlayerThing->pInSector->flags & (SITH_SECTOR_UNDERWATER | SITH_SECTOR_AETHERIUM)) != 0 )
            {
                pPlayerThing->moveStatus = SITHPLAYERMOVE_SWIMIDLE;
                sithPlayerControls_secSwimIdleTime = 0.0f;
                sithPlayerControls_ProcessSwimMove(pPlayerThing, secDeltaTime);
            }
            else
            {
                sithPlayerControls_ProcessFallingMove(pPlayerThing, secDeltaTime);
            }
            break;

        case SITHPLAYERMOVE_MINECAR_IDLE:
            sithVehicleControls_ProcessMineCarMove(pPlayerThing, secDeltaTime);
            break;

        case SITHPLAYERMOVE_JEEP_IDLE:
            sithVehicleControls_ProcessJeepMove(pPlayerThing, secDeltaTime);
            break;

        case SITHPLAYERMOVE_RAFT_IDLE:
            sithVehicleControls_ProcessRaftMove(pPlayerThing, secDeltaTime);
            break;

        case SITHPLAYERMOVE_JEWELFLYING:
        case SITHPLAYERMOVE_JEWELFLYING_UNKN1:
        case SITHPLAYERMOVE_JEWELFLYING_UNKN2:
            sithPlayerControls_ProcessJewelFlyMove(pPlayerThing, secDeltaTime);
            break;

        default:
            break;
    }

    sithPlayerControls_ProcessHUDControls(pPlayerThing, secDeltaTime);
    return 0;
}

int J3DAPI sithPlayerControls_ProcessPlayerDebugControls(SithThing* pThing, float secDeltaTime)
{
    J3D_UNUSED(secDeltaTime);

    if ( !sithControl_GetKey(SITHCONTROL_DEBUG, NULL) )
    {
        return 0;
    }

    int bPressed;
    sithControl_GetKey(SITHCONTROL_ACT2, &bPressed);
    if ( bPressed )
    {
        pThing->moveStatus                            = SITHPLAYERMOVE_STILL;
        pThing->collide.movesize                      = 0.039999999f;
        pThing->thingInfo.actorInfo.bControlsDisabled = 0;
        pThing->thingInfo.actorInfo.bForceMovePlay    = 0;

        sithPuppet_RemoveAllTracks(pThing);

        pThing->renderData.bSkipBuildingJoints = 0;
        pThing->orient.dvec                    = pThing->pos;

        rdPuppet_BuildJointMatrices(&pThing->renderData, &pThing->orient);
        rdVector_Zero3(&pThing->orient.dvec);
        return 1;
    }

    if ( pThing->moveType == SITH_MT_PHYSICS )
    {
        sithPhysics_ResetThingMovement(pThing);
    }

    for ( SithControlFunction i = SITHCONTROL_WSELECT0; i <= SITHCONTROL_WSELECT9; ++i )
    {
        sithControl_GetKey(i, &bPressed);
        if ( bPressed )
        {
            sithPlayerActions_MoveToPlayerPosition(pThing, i - SITHCONTROL_WSELECT0);
            return 1;
        }
    }

    return 1;
}

void J3DAPI sithPlayerControls_ProcessLookControls(SithThing* pThing, float secDeltaTime)
{
    SithPhysicsInfo* pPhysics = &pThing->moveInfo.physics;
    SithActorInfo* pActor     = &pThing->thingInfo.actorInfo;

    bool bInputReceived = false;
    float yawDelta      = 0.0f;
    float pitchDelta    = 0.0f;

    if ( sithControl_GetKey(SITHCONTROL_TURNRIGHT, NULL) )
    {
        if ( !pThing->thingInfo.actorInfo.bForceMovePlay )
        {
            yawDelta = -0.001f;
            bInputReceived = true;
        }
    }
    else if ( sithControl_GetKey(SITHCONTROL_TURNLEFT, NULL)
        && !pThing->thingInfo.actorInfo.bForceMovePlay )
    {
        yawDelta = 0.001f;
        bInputReceived = true;
    }

    if ( yawDelta != 0.0f )
    {
        yawDelta *= 90.0f * secDeltaTime;
    }

    if ( sithControl_GetKey(SITHCONTROL_FORWARD, NULL) )
    {
        pitchDelta = 0.001f;
    }
    else if ( sithControl_GetKey(SITHCONTROL_BACK, NULL) )
    {
        pitchDelta = -0.001f;
    }

    if ( pitchDelta != 0.0f )
    {
        bInputReceived = true;
        pitchDelta *= 90.0f * secDeltaTime;
    }

    if ( bInputReceived )
    {
        rdVector3 newOffset = sithCamera_g_pCurCamera->offset;
        newOffset.z = STDMATH_CLAMP(newOffset.z + pitchDelta, -0.07f, 0.15000001f);

        if ( yawDelta != 0.0f )
        {
            newOffset.x = STDMATH_CLAMP(newOffset.x + yawDelta, -0.07f, 0.15000001f);
        }

        sithCamera_SetExtCameraOffset(&newOffset);
    }
    else if ( (sithMain_g_sith_mode.debugModeFlags & SITHDEBUG_INEDITOR) != 0 )
    {
        pPhysics->angularVelocity.yaw += sithPlayerControls_CalculateAngularVelocity(
            pActor,
            sithControl_GetAxis(SITHCONTROL_MOUSETURN),
            sithControl_GetKeyAsAxis(SITHCONTROL_MOUSETURN),
            1.0f
        );
    }
}

void J3DAPI sithPlayerControls_ProcessGeneralMove(SithThing* pThing, float secDeltaTime)
{
    sithPlayerActions_g_pCurLedgeThingModelFace = NULL;
    sithPlayerActions_g_pCurLedgeThingModel     = NULL;
    sithPlayerActions_g_pCurLedgeSurface        = NULL;

    // Handle jump direction input when in jump ready state
    if ( pThing->thingInfo.actorInfo.bControlsDisabled )
    {
        if ( pThing->moveStatus == SITHPLAYERMOVE_JUMP_READY )
        {
            if ( sithControl_GetKey(SITHCONTROL_FORWARD, NULL) )
            {
                sithPlayerControls_curJumpDirection = 1;
            }
            else if ( sithControl_GetKey(SITHCONTROL_BACK, NULL) )
            {
                sithPlayerControls_curJumpDirection = -1;
            }
            else
            {
                sithPlayerControls_curJumpDirection = 0;
            }
            return;
        }

        if ( pThing->moveStatus != SITHPLAYERMOVE_PUSHPULL_READY )
        {
            return;
        }
    }

    //
    // Calculate movement speed modifier
    //
    float moveFactor = 1.0f;
    bool bRun        = false;

    if ( (sithControl_g_controlOptions & 2) != 0
        || sithControl_GetKey(SITHCONTROL_RUNFWD, NULL)
        || sithControl_GetKey(SITHCONTROL_ACT1, NULL) )
    {
        moveFactor = 2.5f;
        bRun       = true;
    }

    // Cancel run if always-run is on and ACT1 is pressed (toggle behavior)
    if ( (sithControl_g_controlOptions & 2) != 0 && sithControl_GetKey(SITHCONTROL_ACT1, NULL) )
    {
        moveFactor = 1.0f;
        bRun       = false;
    }

    // Slow down on water/web surfaces
    if ( (pThing->attach.flags & SITH_ATTACH_SURFACE) != 0
        && (pThing->attach.attachedToStructure.pSurfaceAttached->flags & (SITH_SURFACE_WEB | SITH_SURFACE_SHALLOWWATER | SITH_SURFACE_WATER)) != 0 )
    {
        moveFactor *= 0.80000001f;
    }

    switch ( pThing->moveStatus )
    {
        case SITHPLAYERMOVE_STILL:
            sithPlayerControls_ProcessStillMove(pThing, secDeltaTime, moveFactor, bRun);
            return;

        case SITHPLAYERMOVE_WALKING:
            sithPlayerControls_ProcessWalkMove(pThing, secDeltaTime, moveFactor, bRun);
            return;

        case SITHPLAYERMOVE_RUNNING:
            sithPlayerControls_ProcessRunMove(pThing, secDeltaTime, moveFactor, bRun);
            return;

        case SITHPLAYERMOVE_CRAWLIDLE:
            sithPlayerControls_ProcessCrawlMove(pThing, secDeltaTime, moveFactor);
            return;

        case SITHPLAYERMOVE_PUSHPULL_READY:
            sithPlayerControls_ProcessPushPullMove(pThing, secDeltaTime);
            return;

        case SITHPLAYERMOVE_SLIDEDOWNFORWARD:
            sithPlayerControls_ProcessSlideDownMove(pThing, secDeltaTime);
            return;

        default:
            return;
    }
}

// New function
static void J3DAPI sithPlayerControls_UpdateClimbingSpeed(SithThing* pThing, bool bFastClimb)
{
#ifdef J3D_QOL_IMPROVEMENTS
    switch ( pThing->moveStatus )
    {
        case SITHPLAYERMOVE_MOUNTING_WALL:
            sithPuppet_SetModeSpeed(pThing, SITHPUPPETSUBMODE_MOUNTWALL, bFastClimb ? 1.6f : 1.0f);
            break;
        case SITHPLAYERMOVE_PULLINGUP:
            sithPuppet_SetModeSpeed(pThing, SITHPUPPETSUBMODE_CLIMBPULLINGUP, bFastClimb ? 1.4f : 1.0f);
            break;
        case SITHPLAYERMOVE_CLIMBING_UP:
            sithPuppet_SetModeSpeed(pThing, SITHPUPPETSUBMODE_CLIMBWALLUP, bFastClimb ? 1.6f : 1.0f);
            break;

        case SITHPLAYERMOVE_CLIMBING_DOWN:
            sithPuppet_SetModeSpeed(pThing, SITHPUPPETSUBMODE_CLIMBWALLDOWN, bFastClimb ? 1.6f : 1.0f);
            break;

        case SITHPLAYERMOVE_CLIMBING_RIGHT:
            sithPuppet_SetModeSpeed(pThing, SITHPUPPETSUBMODE_CLIMBWALLRIGHT, bFastClimb ? 1.4f : 1.0f);
            break;

        case SITHPLAYERMOVE_CLIMBING_LEFT:
            sithPuppet_SetModeSpeed(pThing, SITHPUPPETSUBMODE_CLIMBWALLLEFT, bFastClimb ? 1.4f : 1.0f);
            break;
    };
#endif
}

// New func
static bool J3DAPI sithPlayerControls_StopClimbing(SithThing* pThing)
{
    switch ( pThing->moveStatus )
    {
        case SITHPLAYERMOVE_CLIMBING_UP:
            sithPuppet_StopMode(pThing, SITHPUPPETSUBMODE_CLIMBWALLUP, 0.0f);
            return true;

        case SITHPLAYERMOVE_CLIMBING_DOWN:
            sithPuppet_StopMode(pThing, SITHPUPPETSUBMODE_CLIMBWALLDOWN, 0.0f);
            return true;

        case SITHPLAYERMOVE_CLIMBING_RIGHT:
            sithPuppet_StopMode(pThing, SITHPUPPETSUBMODE_CLIMBWALLRIGHT, 0.0f);
            return true;

        case SITHPLAYERMOVE_CLIMBING_LEFT:
            sithPuppet_StopMode(pThing, SITHPUPPETSUBMODE_CLIMBWALLLEFT, 0.0f);
            return true;
    };

    return false;
}

void J3DAPI sithPlayerControls_ProcessClimbMove(SithThing* pThing, float secDeltaTime)
{
    J3D_UNUSED(secDeltaTime);

    bool bFastClimb= false;
    if ( (sithControl_g_controlOptions & 2) != 0
        || sithControl_GetKey(SITHCONTROL_RUNFWD, NULL)
        || sithControl_GetKey(SITHCONTROL_ACT1, NULL) )
    {
        bFastClimb = true;
    }

    // Cancel run if always-run is on and ACT1 is pressed (toggle behavior)
    if ( (sithControl_g_controlOptions & 2) != 0 && sithControl_GetKey(SITHCONTROL_ACT1, NULL) )
    {
        bFastClimb = false;
    }

    if ( pThing->thingInfo.actorInfo.bControlsDisabled
        || pThing->thingInfo.actorInfo.bForceMovePlay == 1
        || (pThing->type != SITH_THING_ACTOR && pThing->type != SITH_THING_PLAYER) )
    {
    #ifdef J3D_QOL_IMPROVEMENTS
        // Added: Update climbing animation playback speed
        sithPlayerControls_UpdateClimbingSpeed(pThing, bFastClimb);

        // Added: Jump off from wall climbing if jump key was pressed
        if ( sithControl_GetKey(SITHCONTROL_JUMP, NULL)
            && !sithPlayerControls_bJumpKeyActive )

        {
            if ( sithPlayerControls_StopClimbing(pThing) )
            {
                sithPlayerActions_UnmountWall(pThing, sithPlayerControls_climbPupTrackNum);
                sithPlayerControls_climbPupTrackNum = -1;
            }
            sithPlayerControls_bJumpKeyActive   = true;
        }
    #endif
        return;
    }

    // Nullify velocity
    rdVector_Zero3(&pThing->moveInfo.physics.velocity);


    // Note, sithPlayerControls_climbPupTrackNum is never set to puppet track num.
    // Must be a leftover from an earlier implementation.

    // Handle Pullup key (Act2)
    if ( sithControl_GetKey(SITHCONTROL_ACT2, NULL) )
    {
        if ( !pThing->thingInfo.actorInfo.bForceMovePlay )
        {
            rdVector3 upDir = rdroid_g_zVector3;
            rdMatrix_TransformVector34Acc(&upDir, &pThing->orient);
            rdVector_Normalize3Acc(&upDir);

            int bHitNoneClimbSurf;
            SithSurface* pSurf = sithPlayerControls_FindClimbSurface(pThing, &upDir, /*climbDir=*/1, &bHitNoneClimbSurf);
            if ( !pSurf && !bHitNoneClimbSurf && sithPlayerActions_CanPullUp(pThing) == 1 )
            {
                sithPuppet_StopKey(pThing->renderData.pPuppet, sithPlayerControls_climbPupTrackNum, 0.0f);
                sithPlayerControls_climbPupTrackNum = -1;

                sithPlayerActions_ClimbPullUp(pThing);
                sithPlayerControls_UpdateClimbingSpeed(pThing, bFastClimb); // Added: Speedup play if runmode is enabled
            }
        }
    }
    // Handle Jump-off key
    else if ( sithControl_GetKey(SITHCONTROL_JUMP, NULL)
        && !sithPlayerControls_bJumpKeyActive
        && !pThing->thingInfo.actorInfo.bForceMovePlay )
    {
        sithPlayerActions_UnmountWall(pThing, sithPlayerControls_climbPupTrackNum);
        sithPlayerControls_bJumpKeyActive   = true;
        sithPlayerControls_climbPupTrackNum = -1;
    }
    // Handle Climb Up key
    else if ( sithControl_GetKey(SITHCONTROL_FORWARD, NULL) )
    {
        if ( !pThing->thingInfo.actorInfo.bForceMovePlay )
        {
            rdVector3 upDir = rdroid_g_zVector3;
            rdMatrix_TransformVector34Acc(&upDir, &pThing->orient);
            rdVector_Normalize3Acc(&upDir);

            int bHitNoneClimbSurf;
            SithSurface* pSurf = sithPlayerControls_FindClimbSurface(pThing, &upDir, /*climbDir=*/1, &bHitNoneClimbSurf);
            if ( !pSurf || bHitNoneClimbSurf )
            {
                if ( !bHitNoneClimbSurf && sithPlayerActions_CanPullUp(pThing) == 1 ) // Fixed: Added pull up check
                {
                    sithPuppet_StopKey(pThing->renderData.pPuppet, sithPlayerControls_climbPupTrackNum, 0.0f);
                    sithPlayerControls_climbPupTrackNum = -1;

                    sithPlayerActions_ClimbPullUp(pThing);
                    sithPlayerControls_UpdateClimbingSpeed(pThing, bFastClimb); // Added: Speedup play if runmode is enabled
                }

                return;
            }

            sithPlayerActions_ClimbMove(pThing, pSurf, /*direction*/1);
        }

        // Added: Speedup play if runmode is enabled
        sithPlayerControls_UpdateClimbingSpeed(pThing, bFastClimb);
    }
    // Handle Climb Down key
    else if ( sithControl_GetKey(SITHCONTROL_BACK, NULL) )
    {
        if ( !pThing->thingInfo.actorInfo.bForceMovePlay )
        {
            rdVector3 climbDir = RDVECTOR_NEG3(rdroid_g_zVector3);
            rdMatrix_TransformVector34Acc(&climbDir, &pThing->orient);
            rdVector_Normalize3Acc(&climbDir);

            int bHitNoneClimbSurf;
            SithSurface* pSurf = sithPlayerControls_FindClimbSurface(pThing, &climbDir, /*climbDir=*/2, &bHitNoneClimbSurf);

            if ( pSurf )
            {
                sithPlayerActions_ClimbMove(pThing, pSurf, /*direction=*/2);
            }
        }

        // Added: Speedup play if runmode is enabled
        sithPlayerControls_UpdateClimbingSpeed(pThing, bFastClimb);
    }
    // Handle Climb Right key
    else if ( sithControl_GetKey(SITHCONTROL_TURNRIGHT, NULL) )
    {
        if ( !pThing->thingInfo.actorInfo.bForceMovePlay )
        {
            rdVector3 rightDir = rdroid_g_xVector3;
            rdMatrix_TransformVector34Acc(&rightDir, &pThing->orient);
            rdVector_Normalize3Acc(&rightDir);

            int bHitNoneClimbSurf;
            SithSurface* pSurf = sithPlayerControls_FindClimbSurface(pThing, &rightDir, /*climbDir=*/3, &bHitNoneClimbSurf);

            if ( pSurf )
            {
                sithPlayerActions_ClimbMove(pThing, pSurf, /*direction=*/3);
            }
        }

        // Added: Speedup play if runmode is enabled
        sithPlayerControls_UpdateClimbingSpeed(pThing, bFastClimb);
    }
    // Handle Climb Left key
    else if ( sithControl_GetKey(SITHCONTROL_TURNLEFT, NULL) )
    {
        if ( !pThing->thingInfo.actorInfo.bForceMovePlay )
        {
            rdVector3 leftDir = RDVECTOR_NEG3(rdroid_g_xVector3);
            rdMatrix_TransformVector34Acc(&leftDir, &pThing->orient);
            rdVector_Normalize3Acc(&leftDir);

            int bHitNoneClimbSurf;
            SithSurface* pSurf = sithPlayerControls_FindClimbSurface(pThing, &leftDir, /*climbDir=*/4, &bHitNoneClimbSurf);

            if ( pSurf )
            {
                sithPlayerActions_ClimbMove(pThing, pSurf, /*direction=*/4);
            }
        }

        // Added: Speedup play if runmode is enabled
        sithPlayerControls_UpdateClimbingSpeed(pThing, bFastClimb);
    }
}

void J3DAPI sithPlayerControls_ProcessJewelFlyMove(SithThing* pThing, float secDeltaTime)
{
    J3D_UNUSED(secDeltaTime);
    SithPhysicsInfo* pPhysics = &pThing->moveInfo.physics;
    SithActorInfo* pActor     = &pThing->thingInfo.actorInfo;

    if ( pThing->thingInfo.actorInfo.bControlsDisabled == 1 )
    {
        return;
    }

    pThing->collide.type = SITH_COLLIDE_SPHERE;

    if ( (pPhysics->flags & SITH_PF_FLY) == 0 || pThing->attach.flags )
    {
        if ( pThing->attach.flags )
        {
            pThing->moveStatus = SITHPLAYERMOVE_STILL;
        }
        else
        {
            pThing->moveStatus = SITHPLAYERMOVE_FALLING;
        }

        if ( sithPlayerActions_g_jewelFlyingPuppetTrackNum >= 0 )
        {
            sithPuppet_StopKey(pThing->renderData.pPuppet, sithPlayerActions_g_jewelFlyingPuppetTrackNum, 0.0f);
        }

        sithPlayerActions_g_jewelFlyingPuppetTrackNum = -1;
        sithPlayerActions_DisableJewelFlying();
        return;
    }

    rdVector_Zero2((rdVector2*)&pPhysics->thrust);

    // Check if there are any obstacles in up direction
    bool bCanMoveUp = true;
    sithCollision_SearchForCollisions(pThing->pInSector, pThing, &pThing->pos, &rdroid_g_zVector3, 0.12f, 0.050000001f, 0xA00);

    SithCollision* pCollision;
    while ( (pCollision = sithCollision_PopStack()) != NULL )
    {
        if ( (pCollision->type & (SITHCOLLISION_WORLD | SITHCOLLISION_THING)) != 0
            || ((pCollision->type & SITHCOLLISION_ADJOINTOUCH) != 0
                && (pCollision->pSurfaceCollided->pAdjoin->flags & SITH_ADJOIN_NOPLAYERMOVE) != 0) )
        {
            bCanMoveUp = false;
            break;
        }
    }
    sithCollision_DecreaseStackLevel();

    // Check if there are any obstacles in down direction
    bool bCanMoveDown = true;
    rdVector3 downDir = RDVECTOR_NEG3(rdroid_g_zVector3);
    sithCollision_SearchForCollisions(pThing->pInSector, pThing, &pThing->pos, &downDir, 0.1f, 0.050000001f, 0x200);
    while ( (pCollision = sithCollision_PopStack()) != NULL )
    {
        if ( (pCollision->type & (SITHCOLLISION_WORLD | SITHCOLLISION_THING)) != 0
            || ((pCollision->type & SITHCOLLISION_ADJOINTOUCH) != 0
                && (pCollision->pSurfaceCollided->pAdjoin->flags & SITH_ADJOIN_NOPLAYERMOVE) != 0) )
        {
            bCanMoveDown = false;
            break;
        }
    }
    sithCollision_DecreaseStackLevel();

    // Process Up/Down movement keys
    const float thrustIncrement = 0.00019999999f;
    int bPressed;
    if ( sithControl_GetKey(SITHCONTROL_FORWARD, &bPressed) )
    {
        if ( bCanMoveUp )
        {
            pPhysics->thrust.z = STDMATH_CLAMP(pPhysics->thrust.z + thrustIncrement * sithTime_g_fps, -1.0f, 1.0f);
        }
        else
        {
            pPhysics->velocity.z = 0.0f;
            sithPhysics_ResetThingMovement(pThing);
        }
    }
    else if ( sithControl_GetKey(SITHCONTROL_BACK, &bPressed) )
    {
        if ( bCanMoveDown )
        {
            pPhysics->thrust.z = STDMATH_CLAMP(pPhysics->thrust.z - thrustIncrement * sithTime_g_fps, -1.0f, 1.0f);
        }
        else
        {
            pPhysics->velocity.z = 0.0f;
            sithPhysics_ResetThingMovement(pThing);
        }
    }
    else
    {
        pPhysics->thrust.z = 0.0f;
        if ( !bCanMoveUp )
        {
            pPhysics->velocity.z = 0.0f;
            sithPhysics_ResetThingMovement(pThing);
        }
    }

    // Process turn keys
    if ( sithControl_GetKey(SITHCONTROL_TURNRIGHT, &bPressed) )
    {
        pPhysics->angularVelocity.yaw = sithPlayerControls_CalculateAngularVelocity(pActor, -1.0f, -1.0f, 1.0f);

    }
    else if ( sithControl_GetKey(SITHCONTROL_TURNLEFT, &bPressed) )
    {
        pPhysics->angularVelocity.yaw = sithPlayerControls_CalculateAngularVelocity(pActor, 1.0f, 1.0f, 1.0f);
    }
    else
    {
        pPhysics->angularVelocity.yaw = 0.0f;
    }

    // Check if jump off jewel fly key is pressed
    if ( sithControl_GetKey(SITHCONTROL_JUMP, &bPressed) )
    {
        rdVector3 fwdVel = { 0.0f, 0.5f, 0.0f };
        rdMatrix_TransformVector34Acc(&fwdVel, &pThing->orient);
        pPhysics->velocity = fwdVel;

        pThing->moveStatus = SITHPLAYERMOVE_FALLING;
        sithPuppet_PlayMode(pThing, SITHPUPPETSUBMODE_FALL, NULL);

        if ( sithPlayerActions_g_jewelFlyingPuppetTrackNum >= 0 )
        {
            sithPuppet_StopKey(pThing->renderData.pPuppet, sithPlayerActions_g_jewelFlyingPuppetTrackNum, 0.0f);
        }

        sithPlayerActions_g_jewelFlyingPuppetTrackNum = -1;
        pPhysics->flags &= ~SITH_PF_FLY;
        pPhysics->flags |= SITH_PF_USEGRAVITY;

        sithPlayer_g_impFireType = SITHPLAYER_IMPFIRE_OFF;
        sithWeapon_DeselectWeapon(pThing);
        sithPlayerActions_DisableJewelFlying();
        sithInventory_SetSwimmingInventory(pThing, 1);
    }
}

void J3DAPI sithPlayerControls_ProcessFlyMove(SithThing* pThing, float secDeltaTime)
{
    J3D_UNUSED(secDeltaTime);
    SithPhysicsInfo* pPhysics = &pThing->moveInfo.physics;
    SithActorInfo* pActor     = &pThing->thingInfo.actorInfo;

    sithPlayerControls_curJumpDirection = 0;
    SITH_ASSERTREL(sithThing_ValidateThingPointer(sithWorld_g_pCurrentWorld, pThing));

    // TODO: What is the point of underwater code if code is never executed when flying flag is on?
    //       Also this code is only executed when fly flag is on.
    //       Might be just leftover of an early implementation what was used for handling swimming and flying controls?
    //       Or the code just didn't work as intended?

    bool bUnderwater = false;
    bool bFlying     = false;

    if ( pThing->pInSector && (pThing->pInSector->flags & SITH_SECTOR_UNDERWATER) != 0 )
    {
        bUnderwater = true;
    }

    if ( (pPhysics->flags & SITH_PF_FLY) != 0 )
    {
        bFlying     = true;
        bUnderwater = false;
    }

    if ( bUnderwater )
    {
        // Check for action key pressed and handle water ledge climb out
        if ( sithControl_GetKey(SITHCONTROL_ACT2, NULL) )
        {
            if ( (pThing->moveInfo.physics.flags & SITH_PF_ONWATERSURFACE) != 0 )
            {
                int ledgeType = sithPlayerControls_CheckWaterLedge(pThing);
                if ( ledgeType == 1 )
                {
                    sithPhysics_ResetThingMovement(pThing);

                    pThing->moveStatus        = SITHPLAYERMOVE_PULLINGUP;
                    pThing->collide.movesize /= 4.0f;
                    pThing->forceMoveStartPos = pThing->pos;
                    sithPuppet_PlayForceMoveMode(pThing, SITHPUPPETSUBMODE_MOUNTFROMWATER, NULL);
                    return;
                }

                if ( ledgeType == 2 )
                {
                    sithPlayerActions_ClimbOn1m(pThing);
                    return;
                }
            }
            else
            {
                pPhysics->thrust.y = sithPlayerControls_CalculateThrust(pActor, 1.0f, sithPlayerControls_moveFactorNormal);
            }
        }
        else
        {
            pPhysics->thrust.y = 0.0f;
        }

        if ( (pThing->moveInfo.physics.flags & SITH_PF_ONWATERSURFACE) == 0 )
        {
            pPhysics->thrust.z = 0.0f;
        }

        // Note following code will make Indy to turn and roll in all directions

        if ( sithControl_GetKey(SITHCONTROL_FORWARD, NULL) )
        {
            pPhysics->angularVelocity.pitch = -1.0f * sithTime_g_fps;
        }
        else if ( sithControl_GetKey(SITHCONTROL_BACK, NULL) )
        {
            pPhysics->angularVelocity.pitch = 1.0f * sithTime_g_fps;
        }
        else
        {
            pPhysics->angularVelocity.pitch = 0.0f;
        }

        if ( sithControl_GetKey(SITHCONTROL_TURNRIGHT, NULL) )
        {
            pPhysics->angularVelocity.yaw = sithPlayerControls_CalculateAngularVelocity(pActor, -1.0f, -1.0f, 1.0f);
        }
        else if ( sithControl_GetKey(SITHCONTROL_TURNLEFT, NULL) )
        {
            pPhysics->angularVelocity.yaw = sithPlayerControls_CalculateAngularVelocity(pActor, 1.0f, 1.0f, 1.0f);
        }
        else
        {
            pPhysics->angularVelocity.yaw = 0.0f;
        }

        if ( !rdVector_IsZero3(&pPhysics->thrust)
            && !rdVector_IsZero3(&pActor->headPYR)
            && bFlying
            && (pPhysics->flags & SITH_PF_ONWATERSURFACE) == 0 )
        {
            rdMatrix34 mat;
            rdMatrix_BuildRotate34(&mat, &pActor->headPYR);
            rdMatrix_TransformVector34Acc(&pPhysics->thrust, &mat);
        }
    }
    else
    {
        // Check again for flying/underwater (redundant but kept for original logic)
        if ( (pPhysics->flags & SITH_PF_FLY) != 0
            || (pThing->pInSector && (pThing->pInSector->flags & SITH_SECTOR_UNDERWATER) != 0) )
        {
            bFlying = true;
        }

        // If always run or run key pressed
        float speedMultiplier = 1.0f;
        if ( (sithControl_g_controlOptions & 2) != 0 || sithControl_GetKey(SITHCONTROL_ACT1, NULL) )
        {
            speedMultiplier = 2.5f;
        }

        if ( pThing->type == SITH_THING_ACTOR || pThing->type == SITH_THING_PLAYER )
        {
            if ( sithControl_GetKey(SITHCONTROL_FORWARD, NULL) )
            {
                pPhysics->thrust.y = sithPlayerControls_CalculateThrust(pActor, 1.0f, sithPlayerControls_moveFactorNormal);
            }
            else if ( sithControl_GetKey(SITHCONTROL_BACK, NULL) )
            {
                pPhysics->thrust.y = sithPlayerControls_CalculateThrust(pActor, -1.0f, sithPlayerControls_moveFactorSlow);
            }
            else
            {
                pPhysics->thrust.y = 0.0f;
            }

            if ( sithControl_GetKey(SITHCONTROL_TURNRIGHT, NULL) )
            {
                pPhysics->angularVelocity.yaw = sithPlayerControls_CalculateAngularVelocity(pActor, -1.0f, -1.0f, speedMultiplier);
            }
            else if ( sithControl_GetKey(SITHCONTROL_TURNLEFT, NULL) )
            {
                pPhysics->angularVelocity.yaw = sithPlayerControls_CalculateAngularVelocity(pActor, 1.0f, 1.0f, speedMultiplier);
            }
            else
            {
                pPhysics->angularVelocity.yaw = 0.0f;
            }

            pPhysics->thrust.z = 0.0f;

            if ( !rdVector_IsZero3(&pPhysics->thrust)
                && !rdVector_IsZero3(&pActor->headPYR)
                && bFlying
                && (pPhysics->flags & SITH_PF_ONWATERSURFACE) == 0 )
            {
                rdMatrix34 headOrient;
                rdMatrix_BuildRotate34(&headOrient, &pActor->headPYR);
                rdMatrix_TransformVector34Acc(&pPhysics->thrust, &headOrient);
            }

            if ( (sithMain_g_sith_mode.debugModeFlags & SITHDEBUG_INEDITOR) != 0
                && pPhysics->angularVelocity.yaw == 0.0f )
            {
                pPhysics->angularVelocity.yaw = sithPlayerControls_CalculateAngularVelocity(
                    pActor,
                    sithControl_GetAxis(SITHCONTROL_MOUSETURN),
                    sithControl_GetKeyAsAxis(SITHCONTROL_MOUSETURN),
                    1.0f
                );
            }

            if ( bFlying || bUnderwater )
            {
                int pressState;
                if ( sithControl_GetKey(SITHCONTROL_JUMP, &pressState) )
                {
                    if ( pThing->attach.flags )
                    {
                        sithThing_DetachThing(pThing);
                        pPhysics->thrust.z += 2.0f;
                    }
                    else
                    {
                        pPhysics->thrust.z += pActor->maxThrust * sithPlayerControls_moveFactorSlow;
                    }
                }

                if ( sithControl_GetKey(SITHCONTROL_CRAWLTOGGLE, &pressState) )
                {
                    pPhysics->thrust.z -= pActor->maxThrust * sithPlayerControls_moveFactorSlow;
                }
            }
        }
    }
}

void J3DAPI sithPlayerControls_ProcessHUDControls(SithThing* pThing, float secDeltaTime)
{
    J3D_UNUSED(secDeltaTime);

    //
    // Handle map controls
    //
    int bPressed;
    sithControl_GetKey(SITHCONTROL_MAP, &bPressed);
    if ( (bPressed & 1) != 0 )
    {
        sithOverlayMap_ToggleMap();
    }

    if ( sithControl_GetKey(SITHCONTROL_INCREASE, &bPressed) )
    {
        sithOverlayMap_ZoomIn();
    }

    if ( sithControl_GetKey(SITHCONTROL_DECREASE, &bPressed) )
    {
        sithOverlayMap_ZoomOut();
    }

    //
    // Handle health pack use control
    // 
    if ( !sithPlayerControls_bHealthKeyActive
        && sithControl_GetKey(SITHCONTROL_HEALTH, &bPressed) )
    {
        int senderIdx = 0;

        // Herbs small
        if ( sithInventory_GetInventory(pThing, 39) != 0.0f
            && sithInventory_IsInventoryAvailable(pThing, 39u) )
        {
            senderIdx = 39;
        }
        // Health pack small
        else if ( sithInventory_GetInventory(pThing, 49) != 0.0f
            && sithInventory_IsInventoryAvailable(pThing, 49u) )
        {
            senderIdx = 49;
        }
        // Herbs big
        else if ( sithInventory_GetInventory(pThing, 38) != 0.0f
            && sithInventory_IsInventoryAvailable(pThing, 38u) )
        {
            senderIdx = 38;
        }
        // Health pack big
        else if ( sithInventory_GetInventory(pThing, 48) != 0.0f
            && sithInventory_IsInventoryAvailable(pThing, 48u) )
        {
            senderIdx = 48;
        }

        if ( senderIdx )
        {
            SithInventoryType* pType = sithInventory_GetType(senderIdx);
            if ( pType->pCog )
            {
                // Send message to health pack cog to use it
                sithCog_SendMessage(pType->pCog, SITHCOG_MSG_USER0, SITHCOG_SYM_REF_INT, senderIdx, SITHCOG_SYM_REF_THING, pThing->idx, 0);
            }
        }

        sithPlayerControls_bHealthKeyActive = true;
    }
}
int J3DAPI sithPlayerControls_ProcessEditorDebugControls(SithThing* pThing, float secDeltaTime)
{
    if ( (sithMain_g_sith_mode.debugModeFlags & SITHDEBUG_INEDITOR) != 0
        && !sithControl_GetKey(SITHCONTROL_DEBUG, NULL) )
    {
        return 0;
    }

    // Reset movement
    if ( pThing->moveType == SITH_MT_PHYSICS )
    {
        sithPhysics_ResetThingMovement(pThing);
    }

    // Move player to selected player position - 0-9 key
    int bPressed;
    for ( SithControlFunction i = SITHCONTROL_WSELECT0; i <= SITHCONTROL_WSELECT9; ++i )
    {
        sithControl_GetKey(i, &bPressed);
        if ( bPressed )
        {
            sithPlayerActions_MoveToPlayerPosition(pThing, i - SITHCONTROL_WSELECT0);
            break;
        }
    }

    // Simulate damage - Jump key
    sithControl_GetKey(SITHCONTROL_JUMP, &bPressed);
    if ( bPressed )
    {
        sithActor_DamageActor(pThing, pThing, 200.0f, SITH_DAMAGE_IMPACT);
        return 1;
    }

    // Set external camera - crawl key
    sithControl_GetKey(SITHCONTROL_CRAWLTOGGLE, &bPressed);
    if ( bPressed && sithCamera_g_pCurCamera != &sithCamera_g_aCameras[SITHCAMERA_EXTCAMERANUM] )
    {
        sithCamera_SetCurrentCamera(&sithCamera_g_aCameras[SITHCAMERA_EXTCAMERANUM]);
        return 1;
    }

    // Reset thing puppet state - activate key
    sithControl_GetKey(SITHCONTROL_ACT2, &bPressed);
    if ( bPressed )
    {
        pThing->moveStatus                            = SITHPLAYERMOVE_STILL;
        pThing->collide.movesize                      = 0.039999999f;
        pThing->thingInfo.actorInfo.bControlsDisabled = 0;
        pThing->thingInfo.actorInfo.bForceMovePlay    = 0;

        pThing->moveInfo.physics.flags &= ~SITH_PF_ALIGNSURFACE;
        pThing->moveInfo.physics.flags |= SITH_PF_ALIGNUP;
        pThing->moveInfo.physics.height = 0.090000004f;

        sithPuppet_SetMoveMode(pThing, SITHPUPPET_MOVEMODE_NORMAL);
        return 1;
    }

    // Simulate whip swing - step right key
    sithControl_GetKey(SITHCONTROL_STPRIGHT, &bPressed);
    if ( bPressed && pThing->moveStatus != SITHPLAYERMOVE_WHIPSWINGING )
    {
        pThing->moveStatus        = SITHPLAYERMOVE_WHIPSWINGING;
        pThing->forceMoveStartPos = pThing->pos;
        sithThing_DetachThing(pThing);
        sithPuppet_PlayForceMoveMode(pThing, SITHPUPPETSUBMODE_WHIPSWING, NULL);
        // TODO: return?
    }

    // Cycle camera - activate key
    // Altered: Changed from SITHCONTROL_ACTIVATE to  SITHCONTROL_CAMERAMODE
    sithControl_GetKey(SITHCONTROL_CAMERAMODE, &bPressed);
    if ( bPressed )
    {
        sithCamera_CycleCamera();
    }

    // If cur orbital camera, move it
    if ( sithCamera_g_pCurCamera->type == SITHCAMERA_ORBITAL )
    {
        if ( sithCamera_g_pCurCamera->pPrimaryFocusThing != sithPlayer_g_pLocalPlayerThing )
        {
            sithCamera_SetCameraFocus(sithCamera_g_pCurCamera, sithPlayer_g_pLocalPlayerThing, NULL);
        }

        rdVector3 pyr = { 0 };
        if ( sithControl_GetKey(SITHCONTROL_TURNLEFT, &bPressed) )
        {
            pyr.yaw = -1.0f; // Altered: OG +1
        }
        else if ( sithControl_GetKey(SITHCONTROL_TURNRIGHT, &bPressed) )
        {
            pyr.yaw = 1.0f; // Altered: OG -1
        }

        pyr.pitch = sithControl_GetKeyAsAxis(SITHCONTROL_PITCH) * 0.45f; // Altered: multiply by 0.45

        float angleDelta = secDeltaTime * 25.0f; // Altered: OG 90.0f
        if ( sithControl_GetKey(SITHCONTROL_ACT1, &bPressed) ) // Added: speedup
        {
            angleDelta *= 4.5f;
        }

        rdVector_Scale3Acc(&pyr, angleDelta);
        if ( !rdVector_IsZero3(&pyr) )
        {
            rdMatrix34 mat;
            rdMatrix_BuildRotate34(&mat, &pyr);
            rdMatrix_TransformVector34Acc(&sithPlayerControls_curOrbCamDir, &mat);
            rdVector_Normalize3Acc(&sithPlayerControls_curOrbCamDir);
        }

        float distDelta = secDeltaTime * 0.1f;
        float camDistChange = 0.0f;

        if ( sithControl_GetKey(SITHCONTROL_FORWARD, &bPressed) )
        {
            camDistChange = -distDelta;
        }
        else if ( sithControl_GetKey(SITHCONTROL_BACK, &bPressed) )
        {
            camDistChange = distDelta;
        }

        // Added: speedup
        if ( sithControl_GetKey(SITHCONTROL_ACT1, &bPressed) )
        {
            camDistChange *= 4.5f;
        }

        if ( camDistChange != 0.0f )
        {
            sithPlayerControls_curOrbCamDist += camDistChange;
            sithPlayerControls_curOrbCamDist = STDMATH_CLAMP(sithPlayerControls_curOrbCamDist, pThing->collide.size, sithPlayerControls_maxOrbCamDist);
            SITHLOG_STATUS("New camera distance = %f.\n", sithPlayerControls_curOrbCamDist);
        }

        // Calc new orientation for orbital camera
        sithCamera_g_orbCamOrient.lvec = RDVECTOR_NEG3(sithPlayerControls_curOrbCamDir);
        rdVector_Cross3(&sithCamera_g_orbCamOrient.rvec, &sithCamera_g_orbCamOrient.lvec, &rdroid_g_zVector3);
        rdVector_Cross3(&sithCamera_g_orbCamOrient.uvec, &sithCamera_g_orbCamOrient.rvec, &sithCamera_g_orbCamOrient.lvec);
        rdMatrix_Normalize34(&sithCamera_g_orbCamOrient);

        // Set new position for orbital camera
        rdVector_Scale3(&sithCamera_g_orbCamOrient.dvec, &sithPlayerControls_curOrbCamDir, sithPlayerControls_curOrbCamDist);
    }

    // Toggle map feature on/off - map key
    sithControl_GetKey(SITHCONTROL_MAP, &bPressed);
    if ( bPressed )
    {
        sithMain_g_sith_mode.mapModeFlags ^= SITHMAPMODE_SHOWALLTHINGS | SITHMAPMODE_SHOWALLSECTORS;
    }

    return 1;
}

int J3DAPI sithPlayerControls_ProcessDeadPlayer(SithThing* pThing, float secDeltaTime)
{
    J3D_UNUSED(pThing);
    J3D_UNUSED(secDeltaTime);
    return 1;
}

void J3DAPI sithPlayerControls_ProcessFallingMove(SithThing* pThing, float secDeltaTime)
{
    J3D_UNUSED(secDeltaTime);
    SithPhysicsInfo* pPhysics = &pThing->moveInfo.physics;
    SithActorInfo* pActor     = &pThing->thingInfo.actorInfo;

    if ( pThing->attach.flags && pThing->moveStatus == SITHPLAYERMOVE_FALLING )
    {
        pThing->moveStatus = SITHPLAYERMOVE_STILL;
        sithPuppet_RemoveAllTracks(pThing);
        return;
    }

    //
    // If falling not too fast, check for a ledge to grab
    //
    if ( rdVector_Len3(&pPhysics->velocity) < 1.1f )
    {
        SithSurface* pLedgeSurf  = NULL;
        rdFace* pLedgeFace       = NULL;
        rdModel3Mesh* pLedgeMesh = NULL;
        SithThing* pLedgeThing   = NULL;

        // Look for ledge in front of player
        float ledgeDist = sithPlayerActions_FindLedge(pThing, &pThing->orient.lvec, &pLedgeSurf, &pLedgeFace, &pLedgeMesh, &pLedgeThing);
        if ( ledgeDist >= 0.0f )
        {
            if ( sithControl_GetKey(SITHCONTROL_JUMP, NULL) )
            {
                sithPlayerControls_bJumpKeyActive = true; // TODO: What's the purpose of this line here?
            }

            if ( pThing->thingInfo.actorInfo.bForceMovePlay == 1 )
            {
                sithPuppet_FinishForceMove(pThing, /*bStopTracks=*/1);
            }

            // Grab the ledge
            sithPlayerActions_GrabLedge(pThing, ledgeDist, pLedgeSurf, pLedgeFace, pLedgeMesh, pLedgeThing);

            sithPlayerControls_bJumpKeyActive = true;
            sithPhysics_ResetThingMovement(pThing);

            if ( pThing == sithPlayer_g_pLocalPlayerThing
                && !sithPhysics_IsVehicleThing(pThing) )
            {
                rdKeyframe* pTrack = sithPuppet_GetKeyframe("in_jump_up_grab.key");
                if ( pTrack )
                {
                    if ( sithPuppet_PlayKey(pThing->renderData.pPuppet, pTrack, 3, 5, RDKEYFRAME_FADEOUT_NOLOOP | RDKEYFRAME_RESTART_ACTIVE, NULL) >= 0 )
                    {
                        sithSoundClass_PlayModeFirst(pThing, SITHSOUNDCLASS_HITHARD);
                    }
                }
            }
            return;
        }
    }

    //
    // Process falling movement controls for flying player
    //
    if ( (pPhysics->flags & SITH_PF_FLY) != 0 )
    {
        float moveDir = 0.0f;

        if ( sithControl_GetKey(SITHCONTROL_FORWARD, NULL) )
        {
            moveDir = 1.0f;
            pPhysics->thrust.y = sithPlayerControls_CalculateThrust(pActor, 1.0f, sithPlayerControls_moveFactorNormal);
        }
        else if ( sithControl_GetKey(SITHCONTROL_BACK, NULL) )
        {
            moveDir = -1.0f;
            pPhysics->thrust.y = sithPlayerControls_CalculateThrust(pActor, -1.0f, sithPlayerControls_moveFactorSlow);
        }
        else
        {
            pPhysics->thrust.y = 0.0f;
        }

        if ( sithControl_GetKey(SITHCONTROL_TURNRIGHT, NULL) )
        {
            pPhysics->angularVelocity.yaw = sithPlayerControls_CalculateAngularVelocity(pActor, -1.0f, -1.0f, 1.0f);
            pPhysics->angularVelocity.yaw /= 2.0f;
        }
        else if ( sithControl_GetKey(SITHCONTROL_TURNLEFT, NULL) )
        {
            pPhysics->angularVelocity.yaw = sithPlayerControls_CalculateAngularVelocity(pActor, 1.0f, 1.0f, 1.0f);
            pPhysics->angularVelocity.yaw /= 2.0f;
        }
        else
        {
            pPhysics->angularVelocity.yaw = 0.0f;
        }

        if ( (sithMain_g_sith_mode.debugModeFlags & SITHDEBUG_INEDITOR) != 0 )
        {
            pPhysics->angularVelocity.yaw += sithPlayerControls_CalculateAngularVelocity(
                pActor,
                sithControl_GetAxis(SITHCONTROL_MOUSETURN),
                sithControl_GetKeyAsAxis(SITHCONTROL_MOUSETURN),
                1.0f
            );
        }
        else if ( sithControl_GetKey(SITHCONTROL_JUMP, NULL) )
        {
            pPhysics->thrust.z += pActor->maxThrust * sithPlayerControls_moveFactorSlow;
        }
        else
        {
            pPhysics->thrust.z = 0.0f;
        }

        if ( !rdVector_IsZero3(&pPhysics->thrust) && !rdVector_IsZero3(&pActor->headPYR) )
        {
            rdMatrix34 mat;
            rdMatrix_BuildRotate34(&mat, &pActor->headPYR);
            rdMatrix_TransformVector34Acc(&pPhysics->thrust, &mat);
        }
    }
    else
    {
        // Hmm does this make sense for falling player?
        // Sure wind could nudge
        rdVector_Zero3(&pPhysics->thrust);
        rdVector_Zero3(&pPhysics->angularVelocity);
    }
}

void J3DAPI sithPlayerControls_ProcessSwimMove(SithThing* pThing, float secDeltaTime)
{
    J3D_UNUSED(secDeltaTime);
    SithPhysicsInfo* pPhysics = &pThing->moveInfo.physics;
    SithActorInfo* pActor     = &pThing->thingInfo.actorInfo;

    sithPlayerControls_curJumpDirection = 0;

    if ( sithPlayerControls_secSwimIdleTime < 1.0f )
    {
        pPhysics->thrust.x = 0.0f;
        rdVector_Zero3(&pPhysics->angularVelocity);
        sithPhysics_SetThingLook(pThing, &rdroid_g_zVector3, 0.0f);
        return;
    }

    if ( pThing->pPuppetState->moveMode != SITHPUPPET_MOVEMODE_SWIM
        && (pThing->pInSector->flags & (SITH_SECTOR_AETHERIUM | SITH_SECTOR_UNDERWATER)) != 0 )
    {
        sithPuppet_RemoveAllTracks(pThing);
        sithPuppet_SetMoveMode(pThing, SITHPUPPET_MOVEMODE_SWIM);
    }

    const float pitchRate = sithPlayerControls_swimPitchTurnRate;
    const float yawRate   = sithPlayerControls_swimYawTurnRate;

    // Added: Check for swim boost key pressed
    int bBoostJustPrezzed;
    bool bBoootPressed = sithControl_GetKey(SITHCONTROL_RUNFWD, &bBoostJustPrezzed)
        || sithControl_GetKey(SITHCONTROL_ACT1, &bBoostJustPrezzed);

    // Make new boost move QOL
    bBoostJustPrezzed = J3D_QOL_VALUE(bBoostJustPrezzed, 0);
    bBoootPressed     = J3D_QOL_VALUE(bBoootPressed, false);

    // TODO: Check for thing in sector!

    if ( (pThing->flags & SITH_TF_SUBMERGED) == 0
        && (pThing->pInSector->flags & SITH_SECTOR_AETHERIUM) == 0
        && (pThing->moveInfo.physics.flags & SITH_PF_ONWATERSURFACE) == 0 )
    {
        pThing->moveStatus = SITHPLAYERMOVE_STILL;
        sithPuppet_SetMoveMode(pThing, SITHPUPPET_MOVEMODE_NORMAL);

        sithPhysics_FindFloor(pThing, /*bNoSurfaceImpactUpdate=*/0);
        sithPuppet_PlayMode(pThing, SITHPUPPETSUBMODE_STAND, NULL);
        return;
    }

    if ( pThing->pInSector && (pThing->pInSector->flags & SITH_SECTOR_AETHERIUM) != 0 && pThing->attach.flags )
    {
        sithThing_DetachThing(pThing);
    }

    //
    // Activation key pressed
    //
    if ( sithControl_GetKey(SITHCONTROL_ACT2, NULL)
        && (pThing->moveInfo.physics.flags & SITH_PF_ONWATERSURFACE) == 0
        && !sithWeapon_IsMountingWeapon(pThing) )
    {
        int bFoundFloorItem;
        SithThing* pItemThing = sithCollision_FindItemThing(pThing, &bFoundFloorItem);
        if ( pItemThing )
        {
            pThing->thingInfo.actorInfo.flags |= SITH_AF_CONTROLSDISABLED;
            sithPlayerControls_pCurActivatedItemThing = pItemThing;
            sithCog_ThingSendMessage(pItemThing, pThing, SITHCOG_MSG_ACTIVATE);
            sithPhysics_ResetThingMovement(pThing);

            sithPlayerControls_curMoveStatus = pThing->moveStatus;
            pThing->moveStatus = SITHPLAYERMOVE_ACTIVATING;

            pThing->thingInfo.actorInfo.bControlsDisabled = 1;
            sithPuppet_PlayMode(pThing, SITHPUPPETSUBMODE_PICKUP, sithPlayerControls_PuppetCallback);
            return;
        }

        if ( !pThing->thingInfo.actorInfo.bForceMovePlay
            && sithPlayerActions_Activate(pThing) == 1 )
        {
            return;
        }
    }

    //
    // Jump Key pressed - aka swim forward/grab ledge
    //
    if ( sithControl_GetKey(SITHCONTROL_JUMP, NULL) )
    {
        // If on water surface, check for grab ledge
        if ( !sithInventory_GetCurrentWeapon(pThing)
            && !sithWeapon_IsMountingWeapon(pThing)
            && (pThing->moveInfo.physics.flags & SITH_PF_ONWATERSURFACE) != 0
            && !sithPlayerControls_bJumpKeyActive )
        {
            int ledgeType = sithPlayerControls_CheckWaterLedge(pThing);
            if ( ledgeType == 1 ) // player already positioned to ledge grab position
            {
                sithFX_CreateWaterRipple(pThing);
                sithPhysics_ResetThingMovement(pThing);

                pThing->moveStatus = SITHPLAYERMOVE_PULLINGUP;
                pThing->collide.movesize /= 4.0f;
                pThing->forceMoveStartPos = pThing->pos;

                sithPuppet_PlayForceMoveMode(pThing, SITHPUPPETSUBMODE_MOUNTFROMWATER, NULL);
                sithSoundClass_PlayModeFirst(pThing, SITHSOUNDCLASS_EXITWATER);
                sithFX_CreateWaterRipple(pThing);
                return;
            }

            if ( ledgeType == 2 ) // ledge is adjoint
            {
                sithFX_CreateWaterRipple(pThing);
                sithPhysics_ResetThingMovement(pThing);

                pThing->moveStatus        = SITHPLAYERMOVE_PULLINGUP;
                pThing->collide.movesize /= 4.0f;

                pThing->pos.z            -= 0.0049999999f; // adjust z pos
                pThing->forceMoveStartPos = pThing->pos;

                sithPuppet_PlayForceMoveMode(pThing, SITHPUPPETSUBMODE_MOUNT1MSTEP, NULL);
                sithSoundClass_PlayModeFirst(pThing, SITHSOUNDCLASS_EXITWATER);
                sithFX_CreateWaterRipple(pThing);
                return;
            }
        }
        // if not on water surface push thing forward, i.e. swim
        else if ( (pThing->moveInfo.physics.flags & SITH_PF_ONWATERSURFACE) == 0 )
        {
            float directionFactor = 2.0f; // Default

            // Added: Swim boost logic
            //        When swim boost key is pressed swim faster for 3 sec at expense of endurance penalty
            if ( sithPlayerControls_secSwimBoostTimer == 0.0f && bBoostJustPrezzed )
            {
                sithPlayerControls_secSwimBoostTimer = 3.0f;

            }
            else if ( sithPlayerControls_secSwimBoostTimer > 0.0f && bBoootPressed )
            {
                float enduranceRate = 2.5f;

                // If swimming in direction of water current add extra endurance penalty
                if ( (pThing->pInSector->flags & SITH_SECTOR_USETHRUST) != 0 )
                {
                    rdVector3 swimDir      = pThing->orient.lvec;
                    rdVector3 waterVel     = pThing->pInSector->thrust;
                    float currentSpeed     = rdVector_Len3(&waterVel);

                    if ( currentSpeed > 0.0f )
                    {
                        rdVector3 thrustDir;
                        rdVector_Normalize3(&thrustDir, &waterVel);

                        // Projection of water velocity onto swim direction
                        float projected = rdVector_Dot3(&thrustDir, &swimDir) * currentSpeed;

                        // Opposing current component (positive when slowing movement)
                        float oppose = -projected;
                        if ( oppose < 0.0f )
                        {
                            oppose = 0.0f; // if current helps forward, no penalty
                        }

                        // Apply endurance penalty from opposing current
                        enduranceRate *= (1.0f + oppose * 2.5f);

                        // Adjust directionFactor
                        // Penalize when resisting current, boost a bit when aiding
                        directionFactor *= (1.8f - oppose * 1.5f);
                    }
                    else
                    {
                        // no current
                        directionFactor *= 1.8f;
                    }
                }
                else
                {
                    // no current
                    directionFactor *= 1.8f;
                }

                // Apply underwater endurance penalty
                pThing->thingInfo.actorInfo.endurance.msecUnderwater += (tStdTime)(enduranceRate * secDeltaTime * 1000.0f);

                // Puff some bubblez
                if ( SITH_RAND() < 0.30000001f )
                {
                    sithFX_CreateBubble(pThing);
                }
            }

            // Calculate new swim thrust
            pPhysics->thrust.y = sithPlayerControls_CalculateThrust(pActor, directionFactor, sithPlayerControls_moveFactorNormal);

            if ( (pThing->pInSector->flags & SITH_SECTOR_UNDERWATER) != 0 )
            {
                sithPhysics_FindWaterSurface(pThing);
            }

            sithPlayerControls_bJumpKeyActive = true;
        }
    }
    else
    {
        pPhysics->thrust.y = 0.0f;
    }

    //
    // If on water surface, check for dive key press
    //
    if ( (pThing->moveInfo.physics.flags & SITH_PF_ONWATERSURFACE) != 0 )
    {
        rdVector3 newLook = rdroid_g_zVector3;
        if ( (pThing->pInSector->flags & SITH_SECTOR_AETHERIUM) == 0 )
        {
            sithPhysics_SetThingLook(pThing, &newLook, 0.0f);
        }

        //
        // Check if dive key was pressed
        //
        int keyPressedCount;
        if ( sithControl_GetKey(SITHCONTROL_CRAWLTOGGLE, &keyPressedCount) )
        {
            sithPhysics_ResetThingMovement(pThing);
            sithPuppet_StopMode(pThing, SITHPUPPETSUBMODE_FIRE2, 0.0f);
            sithFX_CreateWaterRipple(pThing);

            pThing->forceMoveStartPos = pThing->pos;
            sithPuppet_PlayForceMoveMode(pThing, SITHPUPPETSUBMODE_DIVEFROMSURFACE, NULL);

            // Added: Speed up dive if boost key pressed
            if ( bBoootPressed )
            {
                sithPuppet_SetModeSpeed(pThing, SITHPUPPETSUBMODE_DIVEFROMSURFACE, 1.3f);
            }

            if ( sithPlayerControls_secCommentWaitTimer == 0.0f )
            {
                sithSoundClass_PlayModeFirst(pThing, SITHSOUNDCLASS_EXITWATERSLOW);
                sithPlayerControls_secCommentWaitTimer = 3.0f;
            }

            sithFX_CreateWaterRipple(pThing);
            return;
        }

        pPhysics->thrust.z = 0.0f;
    }
    else
    {
        pPhysics->thrust.z = 0.0f;
    }


    //
    // Process forward/backward pitch control
    //

    bool bBoostTurn = bBoootPressed && stdMath_ClipNearZero(pPhysics->thrust.y) == 0;

    if ( sithControl_GetKey(SITHCONTROL_FORWARD, NULL) )
    {

        if ( (pThing->moveInfo.physics.flags & SITH_PF_ONWATERSURFACE) != 0 )
        {
            // On water surface move forward
            pPhysics->thrust.y = sithPlayerControls_CalculateThrust(pActor, 1.0f, sithPlayerControls_moveFactorNormal);
        }
        else
        {
            // Underwater pitch up
            rdVector3 pyr;
            rdMatrix_ExtractAngles34(&pThing->orient, &pyr);

            float angleDelta = -pitchRate * sithTime_g_frameTimeFlex * (bBoostTurn ? 1.5f : 1.0f); // Altered: Added boost turn
            if ( fabsf(angleDelta + pyr.pitch) <= sithPlayerControls_swimMaxPitchAngle )
            {
                rdVector3 newLook = { angleDelta, 0.0f, 0.0f };
                rdMatrix_PreRotate34(&pThing->orient, &newLook);
            }
            return;
        }
    }
    else if ( sithControl_GetKey(SITHCONTROL_BACK, NULL) )
    {
        if ( (pThing->moveInfo.physics.flags & SITH_PF_ONWATERSURFACE) != 0 )
        {
            //  On water surface move backward
            pPhysics->thrust.y = sithPlayerControls_CalculateThrust(pActor, -1.0f, sithPlayerControls_moveFactorNormal);
        }
        else
        {
            // Underwater pitch down
            rdVector3 pyr;
            rdMatrix_ExtractAngles34(&pThing->orient, &pyr);

            float angleDelta = pitchRate * sithTime_g_frameTimeFlex * (bBoostTurn ? 1.5f : 1.0f); // Altered: Added boost turn
            if ( fabsf(angleDelta + pyr.pitch) <= sithPlayerControls_swimMaxPitchAngle )
            {
                rdVector3 newLook = { angleDelta, 0.0f, 0.0f };
                rdMatrix_PreRotate34(&pThing->orient, &newLook);
            }
            return;
        }
    }
    else
    {
        pPhysics->angularVelocity.pitch = 0.0f;
    }

    //
    // Process turn keys
    //
    if ( sithControl_GetKey(SITHCONTROL_TURNRIGHT, NULL) )
    {

        if ( (pThing->moveInfo.physics.flags & SITH_PF_ONWATERSURFACE) != 0 )
        {
            pPhysics->angularVelocity.yaw = sithPlayerControls_CalculateAngularVelocity(pActor, -1.0f, -1.0f, 1.0f);
        }
        else
        {
            // Altered: Added boost turn
            rdVector3 newLook = { 0.0f, -yawRate * sithTime_g_frameTimeFlex * (bBoostTurn ? 1.5f : 1.0f), 0.0f };
            rdMatrix_PostRotate34(&pThing->orient, &newLook);
            return;
        }
    }
    else if ( sithControl_GetKey(SITHCONTROL_TURNLEFT, NULL) )
    {
        if ( (pThing->moveInfo.physics.flags & SITH_PF_ONWATERSURFACE) != 0 )
        {
            pPhysics->angularVelocity.yaw = sithPlayerControls_CalculateAngularVelocity(pActor, 1.0f, 1.0f, 1.0f);
        }
        else
        {
            // Altered: Added boost turn
            rdVector3 newLook = { 0.0f, yawRate * sithTime_g_frameTimeFlex * (bBoostTurn ? 1.5f : 1.0f), 0.0f };
            rdMatrix_PostRotate34(&pThing->orient, &newLook);
            return;
        }
    }
    else
    {
        pPhysics->angularVelocity.yaw = 0.0f;
    }

    //
    // Transform thrust based on head rotation
    //
    if ( !rdVector_IsZero3(&pPhysics->thrust)
        && !rdVector_IsZero3(&pActor->headPYR)
        && (pPhysics->flags & SITH_PF_ONWATERSURFACE) == 0 )
    {
        rdMatrix34 mat;
        rdMatrix_BuildRotate34(&mat, &pActor->headPYR);
        rdMatrix_TransformVector34Acc(&pPhysics->thrust, &mat);
    }
}

void J3DAPI sithPlayerControls_ProcessHangMove(SithThing* pThing, float secDeltaTime)
{
    J3D_UNUSED(secDeltaTime);
    if ( pThing->thingInfo.actorInfo.bForceMovePlay == 1
        || pThing->type != SITH_THING_PLAYER )
    {
        return;
    }

    // Pull up move
    if ( sithControl_GetKey(SITHCONTROL_FORWARD, NULL) && !sithPlayerControls_bJumpKeyActive )
    {
        if ( sithPlayerActions_CanPullUp(pThing) )
        {
            // Remove collision temporarily
            pThing->collide.movesize  = 0.001f;
            pThing->collide.type      = SITH_COLLIDE_NONE;

            pThing->forceMoveStartPos = pThing->pos;
            pThing->moveStatus = SITHPLAYERMOVE_PULLINGUP;

            pThing->moveInfo.physics.flags &= ~SITH_PF_ALIGNED;
            pThing->moveInfo.physics.flags |= SITH_PF_FLOORSTICK;

            sithThing_DetachThing(pThing);
            sithPuppet_RemoveAllTracks(pThing);

            sithPuppet_PlayForceMoveMode(pThing, SITHPUPPETSUBMODE_MOUNTLEDGE, NULL);
            sithSoundClass_PlayModeFirst(pThing, SITHSOUNDCLASS_CLIMBONTO);
        }
    }
    //Jump off move
    else if ( (sithControl_GetKey(SITHCONTROL_BACK, NULL) || sithControl_GetKey(SITHCONTROL_JUMP, NULL))
        && !sithPlayerControls_bJumpKeyActive )
    {
        sithPlayerActions_UnmountWall(pThing, -1);
        sithPlayerControls_bJumpKeyActive = true;
    }

    // Right move
    else if ( sithControl_GetKey(SITHCONTROL_TURNRIGHT, NULL) )
    {
        rdVector3 moveNorm = rdroid_g_xVector3;
        rdMatrix_TransformVector34Acc(&moveNorm, &pThing->orient);
        rdVector_Normalize3Acc(&moveNorm);

        SithSurface* pLedgeSurf  = NULL;
        rdModel3* pLedgeModel    = NULL;
        rdFace* pLedgeFace       = NULL;
        rdModel3Mesh* pLedgeMesh = NULL;
        SithThing* pLedgeThing   = NULL;

        if ( sithPlayerControls_FindLedgeInDirection(pThing, &moveNorm, &pLedgeSurf, &pLedgeThing, &pLedgeModel, &pLedgeFace, &pLedgeMesh, 3) )
        {
            sithPuppet_RemoveAllTracks(pThing);

            pThing->forceMoveStartPos = pThing->pos;
            pThing->collide.movesize  = 0.0f;
            sithPuppet_PlayForceMoveMode(pThing, SITHPUPPETSUBMODE_HANGSHIMRIGHT, NULL);
            sithSoundClass_PlayModeFirst(pThing, SITHSOUNDCLASS_CLIMBHANDLEFT);

            if ( pLedgeSurf )
            {
                sithThing_AttachThingToClimbSurface(pThing, pLedgeSurf);
                sithPlayerActions_g_pCurLedgeSurface = pLedgeSurf;
            }
            else
            {
                sithThing_AttachThingToThingFace(pThing, pLedgeThing, pLedgeFace, pLedgeMesh->apVertices, /*bNoImpactUpdate=*/1);
                sithPlayerActions_g_pCurLedgeThingModelFace = pLedgeFace;
                sithPlayerActions_g_pCurLedgeThingModel     = pLedgeModel;
            }
        }
    }
    // Left move
    else if ( sithControl_GetKey(SITHCONTROL_TURNLEFT, NULL) )
    {
        rdVector3 moveNorm = RDVECTOR_NEG3(rdroid_g_xVector3);
        rdMatrix_TransformVector34Acc(&moveNorm, &pThing->orient);
        rdVector_Normalize3Acc(&moveNorm);

        SithSurface* pLedgeSurf  = NULL;
        rdModel3* pLedgeModel    = NULL;
        rdFace* pLedgeFace       = NULL;
        rdModel3Mesh* pLedgeMesh = NULL;
        SithThing* pLedgeThing   = NULL;

        if ( sithPlayerControls_FindLedgeInDirection(pThing, &moveNorm, &pLedgeSurf, &pLedgeThing, &pLedgeModel, &pLedgeFace, &pLedgeMesh, 4) )
        {
            sithPuppet_RemoveAllTracks(pThing);

            pThing->forceMoveStartPos = pThing->pos;
            pThing->collide.movesize = 0.0f;
            sithPuppet_PlayForceMoveMode(pThing, SITHPUPPETSUBMODE_HANGSHIMLEFT, NULL);
            sithSoundClass_PlayModeFirst(pThing, SITHSOUNDCLASS_CLIMBHANDLEFT);

            if ( pLedgeSurf )
            {
                sithThing_AttachThingToClimbSurface(pThing, pLedgeSurf);
                sithPlayerActions_g_pCurLedgeSurface = pLedgeSurf;
            }
            else
            {
                sithThing_AttachThingToThingFace(pThing, pLedgeThing, pLedgeFace, pLedgeMesh->apVertices, 1);
                sithPlayerActions_g_pCurLedgeThingModelFace = pLedgeFace;
                sithPlayerActions_g_pCurLedgeThingModel     = pLedgeModel;
            }
        }
    }
    else
    {
        // Stop movement in right/left direction
        pThing->moveInfo.physics.thrust.x = 0.0f;
    }
}

void J3DAPI sithPlayerControls_ProcessWeaponAim(SithThing* pThing, float secDeltaTime)
{
    SithActorInfo* pActor = &pThing->thingInfo.actorInfo;

    SITH_ASSERTREL(pThing);

    if ( (pThing->type != SITH_THING_ACTOR && pThing->type != SITH_THING_PLAYER) || secDeltaTime == 0.0f )
    {
        return;
    }

    int weaponId = sithInventory_GetCurrentWeapon(pThing);
    //TODO: Add weaponId validity check?

    // Calculate aim cone angles based on weapon
    float sin, cosAimHorAcquire, cosAimHorTrack, cosAimVertAcquire, cosAimVertTrack, cosAimHorNear;
    if ( weaponId == SITHWEAPON_MIRROR )
    {
        stdMath_SinCos(sithPlayerControls_mirrorAimMaxHorizontalAngleAcquire, &sin, &cosAimHorAcquire);
        stdMath_SinCos(sithPlayerControls_mirrorAimMaxHorizontalAngleTrack, &sin, &cosAimHorTrack);
        stdMath_SinCos(sithPlayerControls_mirrorAimMaxVerticalAngleAcquire, &sin, &cosAimVertAcquire);
        stdMath_SinCos(sithPlayerControls_mirrorAimMaxVerticalAngleTrack, &sin, &cosAimVertTrack);
        stdMath_SinCos(sithPlayerControls_mirrorAimMaxHorizontalAngleNear, &sin, &cosAimHorNear);
    }
    else
    {
        stdMath_SinCos(sithPlayerControls_aimMaxHorizontalAngleAcquire, &sin, &cosAimHorAcquire);
        stdMath_SinCos(sithPlayerControls_aimMaxHorizontalAngleTrack, &sin, &cosAimHorTrack);
        stdMath_SinCos(sithPlayerControls_aimMaxVerticalAngleAcquire, &sin, &cosAimVertAcquire);
        stdMath_SinCos(sithPlayerControls_aimMaxVerticalAngleTrack, &sin, &cosAimVertTrack);
        stdMath_SinCos(sithPlayerControls_aimMaxHorizontalAngleNear, &sin, &cosAimHorNear);
    }

    SithInventoryType* pWeaponType = NULL;
    SithThing* pTargetThing        = NULL;
    if ( pThing->type == SITH_THING_ACTOR && pThing->controlInfo.aiControl.pLocal )
    {
        pWeaponType  = sithInventory_GetType(weaponId);
        pTargetThing = pThing->controlInfo.aiControl.pLocal->pTargetThing;
    }
    else if ( pThing->type == SITH_THING_PLAYER )
    {
        if ( sithWeapon_GetMountWait() <= sithTime_g_secGameTime )
        {
            switch ( pThing->moveStatus )
            {
                case SITHPLAYERMOVE_JUMPROLLBACK:
                case SITHPLAYERMOVE_JUMPROLLFWD:
                case SITHPLAYERMOVE_JUMPLEFT:
                case SITHPLAYERMOVE_JUMPRIGHT:
                case SITHPLAYERMOVE_FALLING: // Fixed: Added falling case
                    sithPlayerControls_pTargetThing = NULL;
                    sithPlayerControls_RotateAimJointsEx(pThing, 0.0f, 0.0f, secDeltaTime); // Altered: Added smooth interpolation
                    sithWeapon_SendMessageAim(pThing, /*bAim=*/0);
                    return;

                default:
                    pWeaponType  = sithInventory_GetInventoryType(pThing, weaponId);
                    pTargetThing = sithPlayerControls_pTargetThing;
                    break;
            }
        }
        else
        {
            sithPlayerControls_pTargetThing = NULL;
            sithPlayerControls_RotateAimJointsEx(pThing, 0.0f, 0.0f, secDeltaTime); // Altered: Added smooth interpolation
            sithWeapon_SendMessageAim(pThing, /*bAim=*/0);
            return;
        }
    }

    //
    // Set aim start pos and find sector at that position
    // TODO: Move this code after no-aim scope
    //
    rdVector3 aimStartPos = pThing->pos;
    aimStartPos.z += 0.050000001f; // 0.5m up

    SithSector* pStartSector = sithCollision_FindSectorInRadius(pThing->pInSector, &pThing->pos, &aimStartPos, 0.0f);
    if ( !pStartSector )
    {
        pTargetThing = NULL;
        if ( pThing->type == SITH_THING_PLAYER )
        {
            sithPlayerControls_pTargetThing = NULL;
        }

        sithPlayerControls_RotateAimJointsEx(pThing, 0.0f, 0.0f, secDeltaTime); // Altered: Added smooth interpolation
        sithWeapon_SendMessageAim(pThing, /*bAim=*/0);
        return;
    }

    //
    // No auto-aim?
    //
    if ( !pWeaponType || (pWeaponType->flags & SITHINVENTORY_TYPE_AUTOAIM) == 0 )
    {
        if ( (sithMain_g_sith_mode.debugModeFlags & SITHDEBUG_INEDITOR) == 0
            && sithCamera_g_pCurCamera->type != SITHCAMERA_UNKNOWN_100 )
        {
            // Altered: Added smooth interpolation
            sithPlayerControls_RotateAimJointsEx(pThing, 0.0f, 0.0f, secDeltaTime);
        }

        pTargetThing = NULL;
        if ( pThing->type == SITH_THING_PLAYER )
        {
            sithPlayerControls_pTargetThing = NULL;
        }

        if ( (sithMain_g_sith_mode.debugModeFlags & SITHDEBUG_INEDITOR) != 0
            && (pActor->flags & SITH_AF_CANROTATEHEAD) == 0 )
        {
            if ( sithControl_GetKey(SITHCONTROL_CENTER, NULL) )
            {
                sithPhysics_SetThingLook(pThing, &rdroid_g_zVector3, secDeltaTime);
            }
            return;
        }

        rdVector3 headPyr = pActor->headPYR;
        bool bPitchAxis = false;

        if ( (sithMain_g_sith_mode.debugModeFlags & SITHDEBUG_INEDITOR) != 0 )
        {
            float pitchAxis = sithControl_GetAxis(SITHCONTROL_PITCH);
            if ( pitchAxis != 0.0f )
            {
                bPitchAxis = true;
                headPyr.pitch += pitchAxis;
            }

            pitchAxis = sithControl_GetKeyAsAxis(SITHCONTROL_PITCH);
            if ( pitchAxis != 0.0f )
            {
                bPitchAxis = true;
                headPyr.pitch += pitchAxis * 90.0f * secDeltaTime;
            }

            if ( bPitchAxis )
            {
                headPyr.pitch = STDMATH_CLAMP(headPyr.pitch, pActor->minHeadPitch, pActor->maxHeadPitch);
                sithActor_SetHeadPYR(pThing, &headPyr);
                pActor->flags &= ~SITH_AF_VIEWCENTRING;
            }
            else if ( sithControl_GetKey(SITHCONTROL_CENTER, NULL) || (pActor->flags & SITH_AF_VIEWCENTRING) != 0 )
            {
                pActor->flags |= SITH_AF_VIEWCENTRING;

                float targetPitch = -pActor->headPYR.pitch;
                float dpitch = 180.0f * secDeltaTime;

                if ( targetPitch < -dpitch )
                {
                    dpitch = -dpitch;
                }
                else if ( targetPitch <= dpitch )
                {
                    dpitch = targetPitch;
                }

                dpitch = stdMath_ClipNearZero(dpitch);

                if ( dpitch == 0.0f )
                {
                    pActor->flags &= ~SITH_AF_VIEWCENTRING;
                    pActor->flags |= SITH_AF_VIEWCENTRED;
                }
                else
                {
                    pActor->headPYR.pitch += dpitch;
                    sithActor_SetHeadPYR(pThing, &pActor->headPYR);
                }
            }
        }
        return;
    }

    //
    // Proceeded with auto aim
    //

    float weaponMaxAimDist = sithWeapon_GetWeaponMaxAimDistance((SithWeaponId)weaponId);
    float maxDist = weaponMaxAimDist + 0.1f;

    //
    // Validate current target
    //
    if ( pTargetThing )
    {
        bool bTargetVisible = false;

        if ( pTargetThing->type != SITH_THING_FREE
            && (pTargetThing->flags & (SITH_TF_DYING | SITH_TF_DESTROYED)) == 0
            && (pTargetThing->thingInfo.actorInfo.flags & SITH_AF_NOTARGET) == 0 )
        {
            float cosAimHor  = cosAimHorTrack;
            float cosAimVert = cosAimVertTrack;

            if ( pThing->controlInfo.aiControl.pLocal
                && !sithWeapon_IsMountingWeapon(pThing)
                && !sithPuppet_IsModeOnTrack(pThing, SITHPUPPETSUBMODE_AIMWEAPON) )
            {
                maxDist    = weaponMaxAimDist;
                cosAimHor  = cosAimHorAcquire;
                cosAimVert = cosAimVertAcquire;
            }

            if ( sithPlayerControls_CheckAimRange(pThing, &aimStartPos, &pTargetThing->pos, cosAimHor, cosAimHorNear, cosAimVert, maxDist) )
            {
                if ( pThing->type == SITH_THING_PLAYER )
                {
                    bTargetVisible = sithCollision_CheckLOS(pStartSector, &aimStartPos, &pTargetThing->pos, 0.0f);
                }
                else if ( pThing->controlInfo.aiControl.pLocal )
                {
                    bTargetVisible = pThing->controlInfo.aiControl.pLocal->mode & SITHAI_MODE_TARGETVISIBLE;
                }
            }
        }

        if ( !bTargetVisible )
        {
            pTargetThing = NULL;
            if ( pThing->type == SITH_THING_PLAYER )
            {
                sithPlayerControls_pTargetThing = NULL;
            }
        }
    }

    //
    // Search for new player's target
    //
    if ( !pTargetThing && pThing->type == SITH_THING_PLAYER )
    {
        // Search for target every even frame
        if ( !SITH_ISFRAMECYCLE(pThing->idx, 2) )
        {
            return;
        }

        sithCollision_SearchForCollisions(pThing->pInSector, NULL, &pThing->pos, &pThing->orient.lvec, weaponMaxAimDist, 1.0f, 0x284);

        SithCollision* pCollision;
        while ( (pCollision = sithCollision_PopStack()) != NULL )
        {
            if ( (pCollision->type & SITHCOLLISION_THING) != 0
                && pCollision->pThingCollided->type == SITH_THING_ACTOR
                && (pCollision->pThingCollided->flags & (SITH_TF_WHIPSWING | SITH_TF_WHIPCLIMB)) == 0
                && sithInventory_GetCurrentWeapon(pThing) != SITHWEAPON_WHIP
                && pCollision->pThingCollided->type != SITH_THING_FREE // ???
                && (pCollision->pThingCollided->flags & (SITH_TF_DYING | SITH_TF_DESTROYED)) == 0
                && (pCollision->pThingCollided->thingInfo.actorInfo.flags & SITH_AF_NOTARGET) == 0
                && sithPlayerControls_CheckAimRange(pThing, &aimStartPos, &pCollision->pThingCollided->pos, cosAimHorAcquire, cosAimHorNear, cosAimVertAcquire, weaponMaxAimDist)
                && sithCollision_CheckLOS(pStartSector, &aimStartPos, &pCollision->pThingCollided->pos, 0.0f)
                && (weaponId != SITHWEAPON_MIRROR || !stdUtil_StrCmp(pCollision->pThingCollided->aName, "marduk")) )
            {
                pTargetThing = pCollision->pThingCollided;
                if ( pThing->type == SITH_THING_PLAYER )
                {
                    sithPlayerControls_pTargetThing = pTargetThing;
                }
                break;
            }
        }
        sithCollision_DecreaseStackLevel();
    }

    if ( pTargetThing )
    {
        rdVector3 lookPos = pTargetThing->pos;

        float eyeOffsetZ = pTargetThing->thingInfo.actorInfo.eyeOffset.z;
        if ( (pTargetThing->thingInfo.actorInfo.flags & SITH_AF_HUMAN) != 0 )
        {
            eyeOffsetZ *= 0.25f; // don't aim at head?
        }

        lookPos.z += eyeOffsetZ;

        rdMatrix34 lookMat;
        rdMatrix_LookAt(&lookMat, &aimStartPos, &lookPos, 0.0f);

        rdVector3 orientPYR, lookPYR;
        rdMatrix_ExtractAngles34(&pThing->orient, &orientPYR);
        rdMatrix_ExtractAngles34(&lookMat, &lookPYR);

        lookPYR.pitch = stdMath_NormalizeAngleAcute(orientPYR.pitch - lookPYR.pitch);
        lookPYR.yaw   = stdMath_NormalizeAngleAcute(orientPYR.yaw - lookPYR.yaw);

        lookPYR.pitch = STDMATH_CLAMP(lookPYR.pitch, -80.0f, 80.0f);
        lookPYR.yaw   = STDMATH_CLAMP(lookPYR.yaw, -45.0f, 45.0f);

        // Altered: Added smooth interpolation
        sithPlayerControls_RotateAimJointsEx(pThing, -lookPYR.pitch, -lookPYR.yaw, secDeltaTime);
        sithWeapon_SendMessageAim(pThing, 1);
        return;
    }

    //
    // No target thing was found, reset aim joints and send no-aim message
    //
    if ( pThing->type == SITH_THING_PLAYER )
    {
        sithPlayerControls_pTargetThing = NULL;
    }

    // Altered: Added smooth interpolation
    sithPlayerControls_RotateAimJointsEx(pThing, 0.0f, 0.0f, secDeltaTime);
    sithWeapon_SendMessageAim(pThing, /*bAim=*/0);
}

int J3DAPI sithPlayerControls_CheckAimRange(SithThing* pThing, const rdVector3* pStartPos, const rdVector3* pTargetPos, float minCosHorizontal, float minCosHorizontalNear, float minCosVertical, float maxAimDist)
{
    rdVector3 aimDir;
    rdVector_Sub3(&aimDir, pTargetPos, pStartPos);

    float dist = rdVector_Normalize3Acc(&aimDir);
    if ( dist >= maxAimDist )
    {
        return 0;
    }

    float cosThreshold = (dist <= sithPlayerControls_closeRangeAimDistance)
        ? (minCosHorizontal - minCosHorizontalNear) / sithPlayerControls_closeRangeAimDistance * dist + minCosHorizontalNear
        : minCosHorizontal;

    // Vector rejection - project aimDir onto plane perpendicular to uvec
    rdVector3 aimDirXY;
    rdMath_ProjectPointOntoPlaneNormalized(&aimDirXY, &aimDir, &pThing->orient.uvec, &rdroid_g_zeroVector3);

    if ( rdVector_Dot3(&pThing->orient.lvec, &aimDirXY) < cosThreshold )
    {
        return 0;
    }

    return rdVector_Dot3(&aimDir, &aimDirXY) >= minCosVertical;
}

int J3DAPI sithPlayerControls_GetPushPullMoveNorm(rdVector3* moveNorm, const rdVector3* pLVect, float angle)
{
    rdVector3 flatLVec = { pLVect->x, pLVect->y, 0.0f };

    float angleSin, angleCos;
    stdMath_SinCos(angle, &angleSin, &angleCos);

    if ( pLVect->x == 0.0f && pLVect->y == 0.0f )
    {
        return 0;
    }

    rdVector3 dir =  rdroid_g_xVector3;
    if ( fabsf(pLVect->y) >= fabsf(pLVect->x) )
    {
        // Prefer Y direction
        dir =  rdroid_g_yVector3;
    }

    float dotResult = rdVector_Dot3(pLVect, &dir);
    if ( dotResult > 0.0f )
    {
        if ( dotResult <= angleCos )
        {
            return 0;
        }
        *moveNorm = dir;
    }
    else if ( dotResult >= 0.0f || dotResult > -angleCos )
    {
        return 0;
    }
    else
    {
        // Negative direction
        *moveNorm = RDVECTOR_NEG3(dir);
    }

    return 1;
}

int J3DAPI sithPlayerControls_CheckWaterLedge(SithThing* pThing)
{
    if ( (pThing->moveInfo.physics.flags & SITH_PF_ONWATERSURFACE) == 0 )
    {
        return 0;
    }

    rdVector3 startPos = pThing->pos;
    startPos.z += 0.02f;

    rdVector3 moveNorm = pThing->orient.lvec;
    if ( moveNorm.z != 0.0f ) // If tilted, return
    {
        return 0;
    }

    float radius   = 0.050999999f;
    float moveDist = 0.0099999998f; // 0.1m

    SithSector* pStartSector = sithCollision_FindSectorInRadius(pThing->pInSector, &pThing->pos, &startPos, 0.0f);
    if ( !pStartSector )
    {
        return 0;
    }

    int ledgeType = 0;
    sithCollision_SearchForCollisions(pStartSector, pThing, &startPos, &moveNorm, moveDist, radius, 0x801);

    rdVector3 negMoveNorm = RDVECTOR_NEG3(moveNorm);
    SithCollision* pCollision;
    while ( (pCollision = sithCollision_PopStack()) != NULL )
    {
        if ( (pCollision->type & SITHCOLLISION_WORLD) != 0
            && pCollision->pSurfaceCollided
            && (pCollision->pSurfaceCollided->flags & SITH_SURFACE_WATERLEDGE) != 0 )
        {
            float ledgeDot = rdVector_Dot3(&pCollision->pSurfaceCollided->face.normal, &negMoveNorm);
            if ( ledgeDot > 0.87f )
            {
                rdFace* pLedgeFace = &pCollision->pSurfaceCollided->face;
                float topZ = sithWorld_g_pCurrentWorld->aVertices[*pLedgeFace->aVertices].z;

                for ( size_t i = 0; i < pLedgeFace->numVertices; ++i )
                {
                    float z = sithWorld_g_pCurrentWorld->aVertices[pLedgeFace->aVertices[i]].z;
                    if ( z > topZ )
                    {
                        topZ = z;
                    }
                }

                // Get ledge grab pos & sec
                rdVector3 grabPos = pThing->pos;
                grabPos.z = topZ - 0.094999999f;

                SithSector* pGrabSec = sithCollision_FindSectorInRadius(pThing->pInSector, &pThing->pos, &grabPos, 0.0f);
                if ( !pGrabSec )
                {
                    return 0;
                }

                // Move thing to ledge grab pos
                pThing->pos = grabPos;
                if ( pGrabSec != pThing->pInSector )
                {
                    sithThing_ExitSector(pThing);
                    sithThing_EnterSector(pThing, pGrabSec, /*bNoWaterSplash=*/1, /*bNoNotify=*/0);
                }

                ledgeType = 1;
                break;
            }
        }
    }

    sithCollision_DecreaseStackLevel();

    if ( ledgeType != 0 )
    {
        return ledgeType;
    }

    //
    // Second search with larger distance
    // Search for adjoin water surface
    //
    startPos = pThing->pos;
    startPos.z += 0.02f;

    pStartSector = sithCollision_FindSectorInRadius(pThing->pInSector, &pThing->pos, &startPos, 0.0f);
    if ( !pStartSector )
    {
        return 0;
    }

    radius   = 0.050000001f;
    moveDist = 0.1f; // 1m
    sithCollision_SearchForCollisions(pStartSector, pThing, &startPos, &moveNorm, moveDist, radius, 0x801);

    while ( (pCollision = sithCollision_PopStack()) != NULL )
    {
        if ( (pCollision->type & SITHCOLLISION_ADJOINTOUCH) != 0
            && pCollision->pSurfaceCollided
            && (pCollision->pSurfaceCollided->flags & SITH_SURFACE_WATERLEDGE) != 0 )
        {
            float ledgeDot = rdVector_Dot3(&pCollision->pSurfaceCollided->face.normal, &negMoveNorm);
            if ( ledgeDot > 0.87f )
            {
                ledgeType = 2;
                break;
            }
        }
    }

    sithCollision_DecreaseStackLevel();
    return ledgeType;
}

int J3DAPI sithPlayerControls_FindLedgeInDirection(SithThing* pThing, const rdVector3* moveNorm, SithSurface** ppLedgeSurf, SithThing** ppLedgeThing, rdModel3** ppLedgeModel, rdFace** ppLedgeFace, rdModel3Mesh** ppLedgeMesh, int direction)
{
    SithSurface* pLedgeSurf = NULL;

    float moveDist = (direction == 1 || direction == 2) ? 0.2f : 0.1f; // up/down : left/right

    rdVector3 offset;
    rdVector_Scale3(&offset, moveNorm, moveDist);

    rdVector3 startPos = {
        pThing->pos.x + offset.x,
        pThing->pos.y + offset.y,
        pThing->pos.z + offset.z + 0.12f
    };

    SithSector* pStartSector = sithCollision_FindSectorInRadius(pThing->pInSector, &pThing->pos, &startPos, 0.0f);
    if ( !pStartSector )
    {
        return 0;
    }

    int bFound = 0;
    sithCollision_SearchForCollisions(pStartSector, pThing, &startPos, &pThing->orient.lvec, 0.1f, 0.0049999999f, 0xA00);

    SithCollision* pCollision;
    while ( (pCollision = sithCollision_PopStack()) != NULL )
    {
        if ( (pCollision->type & SITHCOLLISION_WORLD) != 0
            && pCollision->pSurfaceCollided
            && (pCollision->pSurfaceCollided->flags & SITH_SURFACE_LEDGE) != 0 )
        {
            // TODO: It's expected for thing to be attached, maybe there should be another check for this
            float dot = rdVector_Dot3(
                &pThing->attach.attachedToStructure.pSurfaceAttached->face.normal,
                &pCollision->pSurfaceCollided->face.normal
            );
            if ( dot >= 0.89999998f )
            {
                pLedgeSurf = pCollision->pSurfaceCollided;
                bFound     = 1;
                break;
            }
        }
        else if ( (pCollision->type & SITHCOLLISION_THING) != 0
            && (pThing->attach.flags & SITH_ATTACH_THINGFACE) != 0
            && pCollision->pFaceCollided
            && pCollision->pThingCollided == pThing->attach.attachedToStructure.pThingAttached
            && (pCollision->pFaceCollided->flags & RD_FF_3DO_LEDGE) != 0 )
        {
            pLedgeSurf = NULL;
            bFound     = 1;

            *ppLedgeModel = pCollision->pThingCollided->renderData.data.pModel3;
            *ppLedgeFace  = pCollision->pFaceCollided;
            *ppLedgeMesh  = pCollision->pMeshCollided;
            *ppLedgeThing = pCollision->pThingCollided;
            break;
        }
    }

    sithCollision_DecreaseStackLevel();

    // No ledge found and not down dir, skip checking for blocking collisions
    if ( !bFound && direction != 2 )
    {
        *ppLedgeSurf = pLedgeSurf;
        return bFound;
    }

    // Find any blocking surface/floor or thing(s)
    int bBlocked = 0;
    float radius = pThing->collide.movesize / 2.0f;
    sithCollision_SearchForCollisions(pThing->pInSector, pThing, &pThing->pos, moveNorm, moveDist, radius, 0xA00);

    while ( (pCollision = sithCollision_PopStack()) != NULL )
    {
        if ( (pCollision->type & SITHCOLLISION_WORLD) != 0 )
        {
            // Unmount wall if moving down and colliding with floor
            if ( direction == 2 && pCollision->pSurfaceCollided
                && (pCollision->pSurfaceCollided->flags & SITH_SURFACE_ISFLOOR) != 0 )
            {
                sithPlayerActions_UnmountWall(pThing, -1);
                sithCollision_DecreaseStackLevel();
                return 0;
            }

            if ( pCollision->pSurfaceCollided != pLedgeSurf )
            {
                bBlocked = 1;
            }
        }
        else if ( (pCollision->type & SITHCOLLISION_THING) != 0 && pCollision->pThingCollided )
        {
            if ( (pThing->attach.flags & SITH_ATTACH_THINGFACE) == 0
                || pCollision->pThingCollided != pThing->attach.attachedToStructure.pThingAttached )
            {
                bBlocked = 1;
                break;
            }
        }
    }

    sithCollision_DecreaseStackLevel();

    if ( bBlocked )
    {
        *ppLedgeSurf  = NULL;
        *ppLedgeFace  = NULL;
        *ppLedgeModel = NULL;
        *ppLedgeMesh  = NULL;
        return 0;
    }

    *ppLedgeSurf = pLedgeSurf;
    return bFound;
}

SithSurface* J3DAPI sithPlayerControls_FindClimbSurface(SithThing* pThing, const rdVector3* moveNorm, int climbDir, int* pbHitNoneClimbSurf)
{
    SithSurface* pClimbSurf = NULL;
    *pbHitNoneClimbSurf = 0;

    float len = (climbDir == 1 || climbDir == 2) ? 0.2f : 0.1f; // up/down : left/right

    rdVector3 offset;
    rdVector_Scale3(&offset, moveNorm, len);

    rdVector3 startPos;
    rdVector_Add3(&startPos, &pThing->pos, &offset);

    SithSector* pStartSector = sithCollision_FindSectorInRadius(pThing->pInSector, &pThing->pos, &startPos, 0.0f);
    if ( !pStartSector )
    {
        return NULL;
    }

    float moveDist = pThing->collide.movesize + 0.0099999998f;
    sithCollision_SearchForCollisions(pStartSector, pThing, &startPos, &pThing->orient.lvec, moveDist, pThing->collide.movesize, 0x01);

    SithCollision* pCollision;
    while ( (pCollision = sithCollision_PopStack()) != NULL )
    {
        if ( (pCollision->type & SITHCOLLISION_WORLD) != 0 )
        {
            if ( pCollision->pSurfaceCollided
                // Fixed: Added check for found surf != attachedSurf 
                //        This resolves not do left or right climb move animation as it will stick indy right back onto current surface.
                //        Note, up/dowm climb doesn't suffer from this issue because sithPlayerActions_CenterOnClimbSurface centers thing on climb surf only in XY dir. 
                && ((climbDir == 1 || climbDir == 2) || pCollision->pSurfaceCollided != pThing->attach.attachedToStructure.pSurfaceAttached)
                && (pCollision->pSurfaceCollided->flags & SITH_SURFACE_CLIMBABLE) != 0 )
            {
                rdVector3 faceNormal = pCollision->pSurfaceCollided->face.normal;
                float surfDot = rdVector_Dot3(&pThing->attach.attachedToStructure.pSurfaceAttached->face.normal, &faceNormal);

                pClimbSurf = pCollision->pSurfaceCollided;
                *pbHitNoneClimbSurf = 0;
                break;
            }

            *pbHitNoneClimbSurf = 1;
        }
        else if ( (pCollision->type & SITHCOLLISION_ADJOINTOUCH) != 0 )
        {
            if ( pCollision->pSurfaceCollided
                && (pCollision->pSurfaceCollided->flags & SITH_SURFACE_CLIMBABLE) != 0 )
            {
                rdVector3 faceNormal = pCollision->pSurfaceCollided->face.normal;
                float surfDot = rdVector_Dot3(&pThing->attach.attachedToStructure.pSurfaceAttached->face.normal, &faceNormal);

                pClimbSurf = pCollision->pSurfaceCollided;
                *pbHitNoneClimbSurf = 0;
                break;
            }

            if ( !pCollision->pSurfaceCollided
                || !pCollision->pSurfaceCollided->pAdjoin )
            {
                *pbHitNoneClimbSurf = 0; // TODO: Should this be 1?
            }
            else if ( (pCollision->pSurfaceCollided->pAdjoin->flags & SITH_ADJOIN_NOPLAYERMOVE) != 0 )
            {
                *pbHitNoneClimbSurf = 1;
            }
        }
    }

    sithCollision_DecreaseStackLevel();

    if ( !pClimbSurf && climbDir != 2 ) // dir != down
    {
        return pClimbSurf;
    }

    // Check for blocking collisions
    bool bBlocked = false;
    sithCollision_SearchForCollisions(pThing->pInSector, pThing, &pThing->pos, moveNorm, len, 0.02f, 0xA00);
    while ( (pCollision = sithCollision_PopStack()) != NULL )
    {
        if ( (pCollision->type & SITHCOLLISION_WORLD) != 0 )
        {
            // Unmount wall if moving down and colliding with floor
            if ( climbDir == 2 && pCollision->pSurfaceCollided
                && (pCollision->pSurfaceCollided->flags & SITH_SURFACE_ISFLOOR) != 0 )
            {
                sithPlayerControls_bJumpKeyActive = true;
                sithPlayerActions_UnmountWall(pThing, -1);
                sithCollision_DecreaseStackLevel();
                *pbHitNoneClimbSurf = 0;
                return NULL;
            }

            if ( pCollision->pSurfaceCollided != pClimbSurf )
            {
                bBlocked = true;
                *pbHitNoneClimbSurf = 1;
                break;
            }
        }
        else if ( (pCollision->type & SITHCOLLISION_ADJOINCROSS) != 0
            || (pCollision->type & SITHCOLLISION_ADJOINTOUCH) != 0 )
        {
            if ( (pCollision->pSurfaceCollided->flags & (SITH_SURFACE_LAVA | SITH_SURFACE_WATER)) != 0 )
            {
                *pbHitNoneClimbSurf = 1;
                bBlocked = true;
                break;
            }
        }
        else if ( (pCollision->type & SITHCOLLISION_THING) != 0 )
        {
            bBlocked = true;
            *pbHitNoneClimbSurf = 1;
            break;
        }
    }

    sithCollision_DecreaseStackLevel();
    return bBlocked ? NULL : pClimbSurf;
}

int J3DAPI sithPlayerControls_BoardVehicle(SithThing* pThing, int bNoBoardAnim)
{
    if ( sithInventory_GetCurrentWeapon(pThing) != SITHWEAPON_NO_WEAPON || sithWeapon_IsMountingWeapon(pThing) )
    {
        return 0;
    }

    //
    // Find vehicle thing in current player direction
    //
    rdVector3 moveNorm = pThing->orient.lvec;
    sithCollision_SearchForCollisions(pThing->pInSector, pThing, &pThing->pos, &moveNorm, pThing->collide.movesize, pThing->collide.movesize, 0xA00);

    SithThing* pVehicleThing = NULL;
    int foundVehicleType     = 0;
    SithCollision* pCollision;
    while ( (pCollision = sithCollision_PopStack()) != NULL )
    {
        if ( (pCollision->type & SITHCOLLISION_THING) != 0 &&
            pCollision->pThingCollided != pThing )
        {
            pVehicleThing = pCollision->pThingCollided;

            if ( streq(pCollision->pThingCollided->aName, SITHPLAYERCONTROLS_THINGNAME_JEEP)
                || streq(pCollision->pThingCollided->aName, SITHPLAYERCONTROLS_THINGNAME_JEEP2) )
            {
                foundVehicleType = 1;
                break;
            }

            if ( streq(pCollision->pThingCollided->aName, SITHPLAYERCONTROLS_THINGNAME_MINECAR) )
            {
                foundVehicleType = 3;
                break;
            }
        }
    }

    sithCollision_DecreaseStackLevel();
    if ( foundVehicleType == 0 )
    {
        return 0;
    }

    //
    // Find vehicle player thing
    //
    // Note the code was a little bit modified so the find loop is defined only in one place.
    // OG had same loop with different thing name check in both of the if scopes

    const char* vehiclePlayerName    = (foundVehicleType == 1) ? SITHPLAYERCONTROLS_THINGNAME_JEEPPLAYER : SITHPLAYERCONTROLS_THINGNAME_MINECARPLAYER;
    const char* vehiclePlayerNameAlt = (foundVehicleType == 1) ? SITHPLAYERCONTROLS_THINGNAME_JEEPPLAYER2 : NULL;

    SithThing* pVehiclePlayerThing = NULL;
    size_t vehiclePlayerNum        = 0;
    bool bHaveVehiclePlayer        = false;
    for ( size_t i = 0; i < sithPlayer_g_numPlayers; ++i )
    {
        if ( streq(sithPlayer_g_aPlayers[i].pThing->aName, vehiclePlayerName)
            || (vehiclePlayerNameAlt && streq(sithPlayer_g_aPlayers[i].pThing->aName, vehiclePlayerNameAlt)) )
        {
            pVehiclePlayerThing = sithPlayer_g_aPlayers[i].pThing;
            vehiclePlayerNum    = i;
            bHaveVehiclePlayer  = true;
            break;
        }
    }

    // Altered: Moved check here.
    //          OG this check was within minecar or jeep if scope,
    //          right after the code that checks player to vehicle enter position dot 
    if ( !bHaveVehiclePlayer )
    {
        return 0;
    }

    // Handle boarding jeep
    if ( foundVehicleType == 1 )
    {
        // Check if player is on the door side of jeep
        float playerToJeepDot = rdVector_Dot3(&pThing->orient.lvec, &pVehicleThing->orient.rvec);
        if ( playerToJeepDot < 0.70700002f )
        {
            return 0;
        }

        // Check player position relative to back of jeep (1m back from center)
        rdVector3 jeepEnterPos;
        rdVector_ScaleAdd3(&jeepEnterPos, &pVehicleThing->orient.lvec, -0.1f, &pVehicleThing->pos);

        rdVector3 playerToEnterPos;
        rdVector_Sub3(&playerToEnterPos, &pThing->pos, &jeepEnterPos);

        float jeepEnterDot = rdVector_Dot3(&playerToEnterPos, &pVehicleThing->orient.lvec);
        if ( jeepEnterDot < 0.0f )
        {
            return 0;
        }

        // Check player position relative to front of jeep (0.7m forward from center)
        rdVector_ScaleAdd3(&jeepEnterPos, &pVehicleThing->orient.lvec, 0.07f, &pVehicleThing->pos);
        rdVector_Sub3(&playerToEnterPos, &pThing->pos, &jeepEnterPos);

        jeepEnterDot = rdVector_Dot3(&playerToEnterPos, &pVehicleThing->orient.lvec);
        if ( jeepEnterDot > 0.0f )
        {
            return 0;
        }

        // 
        // All checks passed, now board jeep
        //
        sithPlayerControls_pBoardedVehicleThing = pVehicleThing;
        sithCamera_RestoreExtCamera();

        pThing->alpha = 1.0f;
        bool bDiffSec = pThing->pInSector != pVehicleThing->pInSector;
        if ( bDiffSec )
        {
            sithCog_SectorSendMessage(pThing->pInSector, pThing, SITHCOG_MSG_EXITED);
        }

        // Switch to indy from guybrush
        if ( sithPlayer_g_bGuybrush )
        {
            sithPlayer_ToggleGuybrush();
        }

        // Hide current player and jeep thing
        sithPlayer_HidePlayer(sithPlayer_g_playerNum);
        pVehicleThing->flags |= SITH_TF_DISABLED;

        // Setup jeepplayer thing
        sithThing_ExitSector(pVehiclePlayerThing);

        // Copy player info from walkplayer to jeepplayer
        *pVehiclePlayerThing->thingInfo.actorInfo.pPlayer        = *pThing->thingInfo.actorInfo.pPlayer;
        pVehiclePlayerThing->thingInfo.actorInfo.health          = pThing->thingInfo.actorInfo.health;
        pVehiclePlayerThing->thingInfo.actorInfo.pPlayer->pThing = pVehiclePlayerThing;

        // Set jeepplayer as current local player & show it
        sithPlayer_SetLocalPlayer(vehiclePlayerNum);
        sithPlayer_ShowPlayer(vehiclePlayerNum, 0);

        // Set jeepplayer thing pos & orient and enter sector
        sithThing_SetPositionAndOrient(pVehiclePlayerThing, &pVehicleThing->pos, &pVehicleThing->orient);
        sithThing_EnterSector(pVehiclePlayerThing, pVehicleThing->pInSector, /*bNoWaterSplash=*/1, /*bNoNotify=*/!bDiffSec);

        // Transfer attached things from jeep thing to jeepplayer thing
        while ( pVehicleThing->pAttachedThing )
        {
            SithThing* pAttachedThing = pVehicleThing->pAttachedThing;
            SithAttachFlag flags      = pAttachedThing->attach.flags;
            sithThing_DetachThing(pAttachedThing);
            sithThing_AttachThingToThing(pAttachedThing, pVehiclePlayerThing);
            pAttachedThing->attach.flags |= flags & SITH_ATTACH_NOMOVE; //TODO: Check if this is correct
        }

        // Set jeepplayer thing normal move mode & search for floor
        sithPuppet_SetMoveMode(pVehiclePlayerThing, SITHPUPPET_MOVEMODE_NORMAL);
        sithPhysics_FindFloor(pVehiclePlayerThing, /*bNoSurfaceImpactUpdate=*/1);

        // Set external camera to focus on jeepplayer thing
        sithCamera_SetCameraFocus(&sithCamera_g_aCameras[SITHCAMERA_EXTCAMERANUM], pVehiclePlayerThing, NULL);
        // TODO: make also other camera focus change to vehicle player thing?

        // Play board animation on jeepplayer thing
        pVehiclePlayerThing->moveStatus = SITHPLAYERMOVE_JEEP_BOARDING;
        pVehiclePlayerThing->alpha      = 1.0f;

        sithPuppet_PlayMode(pVehiclePlayerThing, SITHPUPPETSUBMODE_PUSHITEM, sithVehicleControls_PuppetCallback);
        pVehiclePlayerThing->thingInfo.actorInfo.bControlsDisabled = 1;

        // Disable swim inv & send boarding message to jeepplayer thing and jeep thing
        sithInventory_SetSwimmingInventory(pVehiclePlayerThing, /*bItemsAvailable=*/0);
        sithCog_ThingSendMessage(pVehiclePlayerThing, pThing, SITHCOG_MSG_BOARDED);
        sithCog_ThingSendMessage(pVehicleThing, pVehiclePlayerThing, SITHCOG_MSG_BOARDED);

        // Attach gas can to jeepplayer thing if present
        if ( (int32_t)pVehicleThing->userval != -1 )
        {
            char aGasCanModel[84];
            sprintf_s(aGasCanModel, STD_ARRAYLEN(aGasCanModel), "gascan_%s", pVehicleThing->renderData.data.pModel3->aName);
            rdModel3* pModel = sithModel_GetModel(aGasCanModel);

            int meshIndex      = sithModel_GetMeshIndex(pModel, "jeep_body");
            int thingMeshIndex = sithThing_GetThingMeshIndex(pVehiclePlayerThing, "jeep_body");

            int swapEntry = sithThing_AddSwapEntry(pVehiclePlayerThing, thingMeshIndex, pModel, meshIndex);
            pVehiclePlayerThing->userval = (float)swapEntry;
        }

        // Start boarding cutscene
        sithVehicleControls_StartBoardCutscene(pVehiclePlayerThing);

        // Copy actor flags to jeepplayer thing
        pVehiclePlayerThing->thingInfo.actorInfo.flags &= ~SITH_AF_INVULNERABLE;
        if ( (pThing->thingInfo.actorInfo.flags & SITH_AF_INVULNERABLE) != 0 )
        {
            pVehiclePlayerThing->thingInfo.actorInfo.flags |= SITH_AF_INVULNERABLE;
        }

        pVehiclePlayerThing->thingInfo.actorInfo.flags &= ~SITH_AF_POISONED;
        if ( (pThing->thingInfo.actorInfo.flags & SITH_AF_POISONED) != 0 )
        {
            pVehiclePlayerThing->thingInfo.actorInfo.flags |= SITH_AF_POISONED;
        }

        return 1;
    }
    // Miencar boarding
    else if ( foundVehicleType == 3 )
    {
        // Check if player is on the boarding side of minecar
        float playerToMinecarDot = rdVector_Dot3(&pThing->orient.lvec, &pVehicleThing->orient.rvec);
        if ( fabsf(playerToMinecarDot) < 0.70700002f ) // Note, has to be absolute since player can enter car from left & right side
        {
            return 0;
        }

        // Check dot player to entrance pos at 0.7m behind minecar center pos
        rdVector3 minecarEnterPos;
        rdVector_ScaleAdd3(&minecarEnterPos, &pVehicleThing->orient.lvec, -0.07f, &pVehicleThing->pos);

        rdVector3 playerToEnterPos;
        rdVector_Sub3(&playerToEnterPos, &pThing->pos, &minecarEnterPos);

        float minecarEnterDot = rdVector_Dot3(&playerToEnterPos, &pVehicleThing->orient.lvec);
        if ( minecarEnterDot < 0.0f )
        {
            return 0;
        }

        // Check dot player to entrance pos at 0.7m forward of minecar center pos
        rdVector_ScaleAdd3(&minecarEnterPos, &pVehicleThing->orient.lvec, 0.07f, &pVehicleThing->pos);
        rdVector_Sub3(&playerToEnterPos, &pThing->pos, &minecarEnterPos);

        minecarEnterDot = rdVector_Dot3(&playerToEnterPos, &pVehicleThing->orient.lvec);
        if ( minecarEnterDot > 0.0f )
        {
            return 0;
        }

        //
        // Board minecar
        //

        // Save boarded vehicle thing so it will be moved to position where indy exited car
        sithPlayerControls_pBoardedVehicleThing = pVehicleThing;
        sithCamera_RestoreExtCamera();

        pThing->alpha = 1.0f;
        bool bDiffSec = pThing->pInSector != pVehicleThing->pInSector;
        if ( bDiffSec )
        {
            sithCog_SectorSendMessage(pThing->pInSector, pThing, SITHCOG_MSG_EXITED);
        }

        // Switch to indy from guybrush
        if ( sithPlayer_g_bGuybrush )
        {
            sithPlayer_ToggleGuybrush();
        }

        // Hide current player and minecar thing
        sithPlayer_HidePlayer(sithPlayer_g_playerNum);
        pVehicleThing->flags |= SITH_TF_DISABLED;

        // Setup mineplayer thing
        sithThing_ExitSector(pVehiclePlayerThing);

        // Copy player info from walkplayer to mineplayer
        *pVehiclePlayerThing->thingInfo.actorInfo.pPlayer        = *pThing->thingInfo.actorInfo.pPlayer;
        pVehiclePlayerThing->thingInfo.actorInfo.pPlayer->pThing = pVehiclePlayerThing;
        pVehiclePlayerThing->thingInfo.actorInfo.health          = pThing->thingInfo.actorInfo.health;

        // Set mineplayer as local player & show it
        sithPlayer_SetLocalPlayer(vehiclePlayerNum);
        sithPlayer_ShowPlayer(vehiclePlayerNum, 0);

        // Set mineplayer thing pos & orient and enter sector
        sithThing_SetPositionAndOrient(pVehiclePlayerThing, &pVehicleThing->pos, &pVehicleThing->orient);
        sithThing_EnterSector(pVehiclePlayerThing, pVehicleThing->pInSector, /*bNoWaterSplash=*/1, /*bNoNotify=*/ !bDiffSec);

        // Set mineplayer thing normal move mode & search for floor
        sithPuppet_SetMoveMode(pVehiclePlayerThing, SITHPUPPET_MOVEMODE_NORMAL);
        sithPhysics_FindFloor(pVehiclePlayerThing, /*bNoSurfaceImpactUpdate=*/1);

        // Set external camera to focus on mineplayer thing
        sithCamera_SetCameraFocus(&sithCamera_g_aCameras[SITHCAMERA_EXTCAMERANUM], pVehiclePlayerThing, NULL);
        // TODO: change also focus of other cameras to vehicle player thing?

        // Play board animation or skip
        pVehiclePlayerThing->moveStatus = SITHPLAYERMOVE_MINECAR_BOARDING;
        pVehiclePlayerThing->alpha      = 1.0f;

        if ( bNoBoardAnim )
        {
            pVehiclePlayerThing->moveStatus = SITHPLAYERMOVE_MINECAR_IDLE;

            // Removed: Moved light turn on logic to sithPhysics
            // Turn on minecar lights
            //pVehiclePlayerThing->thingInfo.actorInfo.flags |= SITH_AF_HEADLIGHT;
            //pVehiclePlayerThing->thingInfo.actorInfo.headLightIntensity.alpha = 2.0f; // light range
        }
        else // Play boarding animation
        {
            // Note, the headlight is set through the puppet callback when boarding animation ends
            if ( playerToMinecarDot <= 0.0f ) // boarding from right side
            {
                sithPuppet_PlayMode(pVehiclePlayerThing, SITHPUPPETSUBMODE_HOPRIGHT, sithVehicleControls_PuppetCallback);
            }
            else
            {
                sithPuppet_PlayMode(pVehiclePlayerThing, SITHPUPPETSUBMODE_HOPLEFT, sithVehicleControls_PuppetCallback);
            }

            pVehiclePlayerThing->thingInfo.actorInfo.bControlsDisabled = 1;
            sithVehicleControls_StartBoardCutscene(pVehiclePlayerThing);
        }

        // Disable swim inv & send boarding message to mineplayer thing and minecar thing
        sithInventory_SetSwimmingInventory(pVehiclePlayerThing, /*bItemsAvailable=*/0);
        sithCog_ThingSendMessage(pVehiclePlayerThing, pThing, SITHCOG_MSG_BOARDED);
        sithCog_ThingSendMessage(pVehicleThing, pVehiclePlayerThing, SITHCOG_MSG_BOARDED);

        // Copy actor flags to mineplayer thing
        pVehiclePlayerThing->thingInfo.actorInfo.flags &= ~SITH_AF_INVULNERABLE;
        if ( (pThing->thingInfo.actorInfo.flags & SITH_AF_INVULNERABLE) != 0 )
        {
            pVehiclePlayerThing->thingInfo.actorInfo.flags |= SITH_AF_INVULNERABLE;
        }

        pVehiclePlayerThing->thingInfo.actorInfo.flags &= ~SITH_AF_POISONED;
        if ( (pThing->thingInfo.actorInfo.flags & SITH_AF_POISONED) != 0 )
        {
            pVehiclePlayerThing->thingInfo.actorInfo.flags |= SITH_AF_POISONED;
        }

        return 1;
    }

    return 0;
}

void J3DAPI sithPlayerControls_ExitVehicle(SithThing* pVehiclePlayerThing)
{
    sithVehicleControls_EndBoardCutscene(pVehiclePlayerThing);
    sithInventory_SetSwimmingInventory(pVehiclePlayerThing, /*bItemsAvailable=*/1);

    rdVector3 unboardPos = pVehiclePlayerThing->pos;
    rdMatrix34 walkPlayerOrient;
    float vehiclePosOffsetZ;

    if ( pVehiclePlayerThing->moveStatus == SITHPLAYERMOVE_MINECAR_UNBOARDING_LEFT )
    {
        // Exit left side
        rdVector3 leftOffset;
        rdVector_Scale3(&leftOffset, &pVehiclePlayerThing->orient.rvec, -sithPlayerControls_minecarUnboardDist);
        rdVector_Add3Acc(&unboardPos, &leftOffset);

        walkPlayerOrient.uvec = pVehiclePlayerThing->orient.uvec;
        walkPlayerOrient.lvec = pVehiclePlayerThing->orient.rvec;
        walkPlayerOrient.rvec = RDVECTOR_NEG3(pVehiclePlayerThing->orient.lvec);

        rdModel3* pModel3 = sithPlayerControls_pBoardedVehicleThing->renderData.data.pModel3;
        vehiclePosOffsetZ = pModel3->insertOffset.z - sithPhysics_GetThingHeight(pVehiclePlayerThing);

        // TODO: Why is emitting light?
        // Fixed: Cleared light emit flag as the light was glitchy and could flicker on/off
        sithPlayerControls_pBoardedVehicleThing->flags &= ~SITH_TF_EMITLIGHT;
    }
    else if ( pVehiclePlayerThing->moveStatus == SITHPLAYERMOVE_MINECAR_UNBOARDING_RIGHT )
    {
        // Exit right side
        rdVector3 rightOffset;
        rdVector_Scale3(&rightOffset, &pVehiclePlayerThing->orient.rvec, sithPlayerControls_minecarUnboardDist);
        rdVector_Add3Acc(&unboardPos, &rightOffset);

        walkPlayerOrient.uvec = pVehiclePlayerThing->orient.uvec;
        walkPlayerOrient.lvec = RDVECTOR_NEG3(pVehiclePlayerThing->orient.rvec);
        walkPlayerOrient.rvec = pVehiclePlayerThing->orient.lvec;

        rdModel3* pModel3 = sithPlayerControls_pBoardedVehicleThing->renderData.data.pModel3;
        vehiclePosOffsetZ = pModel3->insertOffset.z - sithPhysics_GetThingHeight(pVehiclePlayerThing);

        // TODO: Why is emitting light?
        // Fixed: Cleared light emit flag as the light was glitchy and could flicker on/off
        sithPlayerControls_pBoardedVehicleThing->flags &= ~SITH_TF_EMITLIGHT;
    }
    else // jeep
    {
        // Exit left side
        rdVector3 leftOffset;
        rdVector_Scale3(&leftOffset, &pVehiclePlayerThing->orient.rvec, -sithPlayerControls_vehicleUnboardDist);
        rdVector_Add3Acc(&unboardPos, &leftOffset);

        walkPlayerOrient.uvec = rdroid_g_zVector3;

        // Cross product to get forward vector on horizontal plane
        rdVector_Cross3(&walkPlayerOrient.lvec, &walkPlayerOrient.uvec, &pVehiclePlayerThing->orient.lvec);
        rdVector_Normalize3Acc(&walkPlayerOrient.lvec);

        rdVector_Cross3(&walkPlayerOrient.rvec, &walkPlayerOrient.lvec, &walkPlayerOrient.uvec);
        rdVector_Normalize3Acc(&walkPlayerOrient.rvec);

        vehiclePosOffsetZ = 0.0f;

        // Transfer attached things from vehicle player back to vehicle
        while ( pVehiclePlayerThing->pAttachedThing )
        {
            SithThing* pAttachedThing = pVehiclePlayerThing->pAttachedThing;
            SithAttachFlag flags     = pAttachedThing->attach.flags;

            sithThing_DetachThing(pAttachedThing);
            sithThing_AttachThingToThing(pAttachedThing, sithPlayerControls_pBoardedVehicleThing);
            pAttachedThing->attach.flags |= flags & SITH_ATTACH_NOMOVE; // keep only no move flag
        }
    }

    // Get walkplayer thing
    SithThing* pWalkPlayerThing = sithPlayer_g_aPlayers[SITHPLAYER_DEFAULTLOCALPLAYERNUM].pThing;
    pWalkPlayerThing->alpha     = 1.0f;

    // Find exit sector for walk thing
    SithSector* pVehicleExitSec = sithCollision_FindSectorInRadius(pVehiclePlayerThing->pInSector, &pVehiclePlayerThing->pos, &unboardPos, 0.0f);
    if ( !pVehicleExitSec )
    {
        SITHLOG_ERROR("This is really bad, trying to exit vehicle to non-existent sector..\n");
    }

    // Exit vehicle sector and 
    bool bDiffSec           = pVehicleExitSec != pVehiclePlayerThing->pInSector;
    size_t vehiclePlayerNum = sithPlayer_g_playerNum;
    SithSector* pVehicleSec = pVehiclePlayerThing->pInSector;

    if ( bDiffSec )
    {
        sithCog_SectorSendMessage(pVehiclePlayerThing->pInSector, pVehiclePlayerThing, SITHCOG_MSG_EXITED);
    }

    // Hide vehicle player
    sithPlayer_HidePlayer(vehiclePlayerNum);

    // Copy vehicle player state to walkplayer
    *pWalkPlayerThing->thingInfo.actorInfo.pPlayer        = *pVehiclePlayerThing->thingInfo.actorInfo.pPlayer;
    pWalkPlayerThing->thingInfo.actorInfo.pPlayer->pThing = pWalkPlayerThing;
    pWalkPlayerThing->thingInfo.actorInfo.health          = pVehiclePlayerThing->thingInfo.actorInfo.health;

    // Copy actor flags
    pWalkPlayerThing->thingInfo.actorInfo.flags &= ~SITH_AF_INVULNERABLE;
    if ( (pVehiclePlayerThing->thingInfo.actorInfo.flags & SITH_AF_INVULNERABLE) != 0 )
    {
        pWalkPlayerThing->thingInfo.actorInfo.flags |= SITH_AF_INVULNERABLE;
    }

    pWalkPlayerThing->thingInfo.actorInfo.flags &= ~SITH_AF_POISONED;
    if ( (pVehiclePlayerThing->thingInfo.actorInfo.flags & SITH_AF_POISONED) != 0 )
    {
        pWalkPlayerThing->thingInfo.actorInfo.flags |= SITH_AF_POISONED;
    }

    // Set walkplayer position and make it visible
    sithThing_ExitSector(pWalkPlayerThing);
    sithPlayer_SetLocalPlayer(SITHPLAYER_DEFAULTLOCALPLAYERNUM);
    sithPlayer_ShowPlayer(SITHPLAYER_DEFAULTLOCALPLAYERNUM, 0);
    sithThing_SetPositionAndOrient(pWalkPlayerThing, &unboardPos, &walkPlayerOrient);

    // Send unboard message to vehicle thing
    sithCog_ThingSendMessage(sithPlayerControls_pBoardedVehicleThing, pVehiclePlayerThing, SITHCOG_MSG_UNBOARDED);

    sithThing_EnterSector(pWalkPlayerThing, pVehicleExitSec, /*bNoWaterSplash=*/1, /*bNoNotify=*/!bDiffSec);

    // Set walkplayer thing normal move mode
    pWalkPlayerThing->moveStatus = SITHPLAYERMOVE_STILL;
    sithPuppet_SetMoveMode(pWalkPlayerThing, SITHPUPPET_MOVEMODE_NORMAL);

    // Set external camera to focus on walkplayer thing
    sithCamera_SetCameraFocus(&sithCamera_g_aCameras[SITHCAMERA_EXTCAMERANUM], pWalkPlayerThing, NULL);
    // TODO: Set also focus of other cameras to walk player thing?

    sithPhysics_FindFloor(pWalkPlayerThing, /*bNoSurfaceImpactUpdate=*/1);
    pWalkPlayerThing->alpha = 1.0f;

    // Move vehicle thing to vehicleplayer thing pos and orientation
    sithThing_ExitSector(sithPlayerControls_pBoardedVehicleThing);

    rdVector3 vehiclePos = pVehiclePlayerThing->pos;
    vehiclePos.z += vehiclePosOffsetZ;

    sithThing_SetPositionAndOrient(sithPlayerControls_pBoardedVehicleThing, &vehiclePos, &pVehiclePlayerThing->orient);
    sithThing_EnterSector(sithPlayerControls_pBoardedVehicleThing, pVehicleSec, /*bNoWaterSplash=*/1, /*bNoNotify=*/1);
    sithPlayerControls_pBoardedVehicleThing->flags &= ~SITH_TF_DISABLED; // Show vehicle thing
    sithPlayerControls_pBoardedVehicleThing = NULL;
}

static inline void J3DAPI sithPlayerControls_BendAimJoint(SithThing* pThing, size_t jointNum, const rdVector3* pyr, float secDeltaTime)
{
    rdModel3_BendJoint(&pThing->renderData, jointNum, pyr, sithPlayerControls_aimRotSmoothRate, secDeltaTime);
}

static inline void J3DAPI sithPlayerControls_BendAimJointAngle(SithThing* pThing, size_t jointNum, size_t axis, float angle, float secDeltaTime)
{
    rdModel3_BendJointAngle(&pThing->renderData, jointNum, axis, angle, sithPlayerControls_aimRotSmoothRate, secDeltaTime);
}

static inline void J3DAPI sithPlayerControls_BendAimJointPitch(SithThing* pThing, size_t jointNum, float angle, float secDeltaTime)
{
    sithPlayerControls_BendAimJointAngle(pThing, jointNum, 0, angle, secDeltaTime);
}

static inline void J3DAPI sithPlayerControls_BendAimJointYaw(SithThing* pThing, size_t jointNum, float angle, float secDeltaTime)
{
    sithPlayerControls_BendAimJointAngle(pThing, jointNum, 1, angle, secDeltaTime);
}

static inline void J3DAPI sithPlayerControls_BendAimJointRoll(SithThing* pThing, size_t jointNum, float angle, float secDeltaTime)
{
    sithPlayerControls_BendAimJointAngle(pThing, jointNum, 2, angle, secDeltaTime);
}

void J3DAPI sithPlayerControls_RotateAimJoints(SithThing* pThing, float pitch, float yaw)
{
    sithPlayerControls_RotateAimJointsEx(pThing, pitch, yaw, /*secDeltaTime=*/0.0f);
}

void J3DAPI sithPlayerControls_RotateAimJointsEx(SithThing* pThing, float pitch, float yaw, float secDeltaTime)
{
#ifndef J3D_QOL_IMPROVEMENTS
    // Restore original code that won't rotate interpolate joint
    secDeltaTime = 0.0f;
#endif

    int curWeapon = sithInventory_GetCurrentWeapon(pThing);

    if ( pitch == 0.0f && yaw == 0.0f )
    {
        // Altered: Add smooth interpolation
        sithPlayerControls_ResetAimJointsEx(pThing, secDeltaTime);
        return;
    }

    switch ( curWeapon )
    {
        case SITHWEAPON_PISTOL:
        case SITHWEAPON_TOKAREV:
        case SITHWEAPON_MAUSER:
        case SITHWEAPON_COMTOKAREV:
        case SITHWEAPON_COMMAUSER:
        {
            yaw += sithPlayerControls_pistolYawOffset;

            int jointIdx = sithThing_GetThingJointIndex(pThing, "inrarm");
            if ( jointIdx >= 0 )
            {
                pitch = STDMATH_CLAMP(pitch, -80.0f, 80.0f);
                /*pThing->renderData.apTweakedAngles[jointIdx].yaw  = pitch;
                pThing->renderData.apTweakedAngles[jointIdx].roll = pitch * -0.5f;*/

                // Altered: Rotate joints with smooth interpolation
                sithPlayerControls_BendAimJointYaw(pThing, jointIdx, pitch, secDeltaTime);
                sithPlayerControls_BendAimJointRoll(pThing, jointIdx, pitch * -0.5f, secDeltaTime);
            }

            jointIdx = sithThing_GetThingJointIndex(pThing, "intorso");
            if ( jointIdx >= 0 )
            {
                yaw = STDMATH_CLAMP(yaw, -45.0f, 45.0f);
              /*pThing->renderData.apTweakedAngles[jointIdx].yaw   = yaw;
                pThing->renderData.apTweakedAngles[jointIdx].pitch = 0.0;*/

                // Altered: Rotate joints with smooth interpolation
                sithPlayerControls_BendAimJointYaw(pThing, jointIdx, yaw, secDeltaTime);
                sithPlayerControls_BendAimJointPitch(pThing, jointIdx, 0.0f, secDeltaTime);
            }
        } break;

        case SITHWEAPON_SUBMACHINE:
        case SITHWEAPON_SHOTGUN:
        case SITHWEAPON_BAZOOKA:
        case SITHWEAPON_COMSUBMACHINE:
        case SITHWEAPON_COMSHOTGUN:
            yaw += sithPlayerControls_rifleYawOffset;
            // Fall through
        case SITHWEAPON_SIMONOV:
        case SITHWEAPON_COMSIMONOV:
        {
            int jointIdx = sithThing_GetThingJointIndex(pThing, "intorso");
            if ( jointIdx >= 0 )
            {
                pitch = STDMATH_CLAMP(pitch, -45.0f, 45.0f);
                yaw   = STDMATH_CLAMP(yaw, -45.0f, 45.0f);

                /*pThing->renderData.apTweakedAngles[jointIdx].pitch = pitch;
                pThing->renderData.apTweakedAngles[jointIdx].yaw   = yaw;
                pThing->renderData.apTweakedAngles[jointIdx].roll  = 0.0f;*/

                // Altered: Rotate joints with smooth interpolation
                sithPlayerControls_BendAimJointPitch(pThing, jointIdx, pitch, secDeltaTime);
                sithPlayerControls_BendAimJointYaw(pThing, jointIdx, yaw, secDeltaTime);
                sithPlayerControls_BendAimJointRoll(pThing, jointIdx, 0.0f, secDeltaTime);
            }
        } break;

        case SITHWEAPON_MIRROR:
        {
            int jointIdx = sithThing_GetThingJointIndex(pThing, "intorso");
            if ( jointIdx >= 0 )
            {
                pitch = STDMATH_CLAMP(pitch, -45.0f, 45.0f);
                yaw   = STDMATH_CLAMP(yaw, -45.0f, 45.0f);

                /*pThing->renderData.apTweakedAngles[jointIdx].pitch = pitch;
                pThing->renderData.apTweakedAngles[jointIdx].yaw   = yaw;
                pThing->renderData.apTweakedAngles[jointIdx].roll  = 0.0f;*/

                 // Altered: Rotate joints with smooth interpolation
                sithPlayerControls_BendAimJointPitch(pThing, jointIdx, pitch, secDeltaTime);
                sithPlayerControls_BendAimJointYaw(pThing, jointIdx, yaw, secDeltaTime);
                sithPlayerControls_BendAimJointRoll(pThing, jointIdx, 0.0f, secDeltaTime);
            }
        } break;

        default:
            // Altered: Add smooth interpolation
            sithPlayerControls_ResetAimJointsEx(pThing, secDeltaTime);
            break;
    }

    if ( pitch != 0.0f || yaw != 0.0f )
    {
        // Altered: Add smooth interpolation
        rdVector3 newPYR = { pitch, 0.0f, 0.0f };
        //sithActor_SetHeadPYR(pThing, &newPYR);
        sithActor_RotateHead(pThing, &newPYR, secDeltaTime);
    }
}

void J3DAPI sithPlayerControls_ResetAimJoints(SithThing* pThing)
{
    sithPlayerControls_ResetAimJointsEx(pThing, /*secDeltaTime=*/0.0f);
}

void J3DAPI sithPlayerControls_ResetAimJointsEx(SithThing* pThing, float secDeltaTime)
{
#ifndef J3D_QOL_IMPROVEMENTS
    // Restore original code that won't rotate interpolate joint
    secDeltaTime = 0.0f;
#endif

    // Altered: Added smooth interpolation
    //pThing->thingInfo.actorInfo.headPYR = zeroPYR;
    if ( secDeltaTime > 0.0f )
    {
        rdVector_SmoothDamp3Acc(&pThing->thingInfo.actorInfo.headPYR, &rdroid_g_zeroVector3, sithPlayerControls_aimRotSmoothRate, secDeltaTime);
    }
    else
    {
        pThing->thingInfo.actorInfo.headPYR = rdroid_g_zeroVector3;
    }

    int jointIdx = sithThing_GetThingJointIndex(pThing, "inrarm");
    if ( jointIdx >= 0 ) // Fixed: Added check for joint index validity
    {
        // Altered: Added smooth interpolation
        //pThing->renderData.apTweakedAngles[jointIdx] = zeroPYR;
        sithPlayerControls_BendAimJoint(pThing, jointIdx, &rdroid_g_zeroVector3, secDeltaTime);

    }

    jointIdx = sithThing_GetThingJointIndex(pThing, "intorso");
    if ( jointIdx >= 0 ) // Fixed: Added check for joint index validity
    {
        // Altered: Added smooth interpolation
        //pThing->renderData.apTweakedAngles[jointIdx] = zeroPYR;
        sithPlayerControls_BendAimJoint(pThing, jointIdx, &rdroid_g_zeroVector3, secDeltaTime);
    }

    sithActor_RotateHead(pThing, &rdroid_g_zeroVector3, secDeltaTime);
}

SithThing* sithPlayerControls_GetTargetThing(void)
{
    return sithPlayerControls_pTargetThing;
}

SithThing* sithPlayerControls_GetVehicleBoardedThing(void)
{
    return sithPlayerControls_pBoardedVehicleThing;
}

void J3DAPI sithPlayerControls_SetVehicleBoardedThing(SithThing* pThing)
{
    sithPlayerControls_pBoardedVehicleThing = pThing;
}

bool J3DAPI sithPlayerControls_CanStrafeMove(SithThing* pThing, int bMoveRight)
{
    //
    // Search for any blocking collision left or right in distance 1m from thing's current pos
    //

    // Determine strafe direction
    rdVector3 moveNorm = bMoveRight
        ? pThing->orient.rvec
        : RDVECTOR_NEG3(pThing->orient.rvec);

    bool bCanMove = true;
    sithCollision_SearchForCollisions(pThing->pInSector, pThing, &pThing->pos, &moveNorm, 0.1f, pThing->collide.movesize, 0xA00); // Altered: Change radius to movesize from 0.089000002f

    SithCollision* pCollision;
    while ( (pCollision = sithCollision_PopStack()) != NULL )
    {
        if ( (pCollision->type & SITHCOLLISION_WORLD) != 0 )
        {
            bCanMove = false;
            break;
        }

        if ( (pCollision->type & SITHCOLLISION_ADJOINCROSS) != 0 )
        {
            if ( (pCollision->pSurfaceCollided->pAdjoin->flags & SITH_ADJOIN_NOPLAYERMOVE) != 0 ||
                (pCollision->pSurfaceCollided->pAdjoin->flags & SITH_ADJOIN_MOVE) == 0 )
            {
                bCanMove = false;
                break;
            }
        }
        else if ( (pCollision->type & SITHCOLLISION_THING) != 0 )
        {
            bCanMove = false;
            break;
        }
    }

    sithCollision_DecreaseStackLevel();

    if ( !bCanMove )
    {
        return 0;
    }

    //
    // Find floor at strafe end position
    //

    rdVector3 strafeEndPos;
    rdVector_ScaleAdd3(&strafeEndPos, &moveNorm, 0.1f, &pThing->pos); // Fixed: Replaced thing.orient.lvect with strafe moveNorm

    SithSector* pStartSector = sithCollision_FindSectorInRadius(pThing->pInSector, &pThing->pos, &strafeEndPos, 0.0f);
    if ( !pStartSector )
    {
        return 0;
    }

    // Search for collision in down direction at distance 1.1m from previous end pos
    moveNorm = RDVECTOR_NEG3(rdroid_g_zVector3); // Down direction
    bCanMove = false;

    sithCollision_SearchForCollisions(pStartSector, NULL, &strafeEndPos, &moveNorm, 0.11f, 0.0099999998f, 0xA00);

    while ( (pCollision = sithCollision_PopStack()) != NULL )
    {
        if ( (pCollision->type & SITHCOLLISION_WORLD) != 0 )
        {
            rdFace* pFace = &pCollision->pSurfaceCollided->face;

            // Calculate distance from start pos to floor plane
            float dist = rdMath_DistancePointToPlane(&strafeEndPos, &pFace->normal, &sithWorld_g_pCurrentWorld->aVertices[*pFace->aVertices]);
            if ( dist < 0.088f || dist > 0.092f )
            {
                bCanMove = false;
                break;
            }

            // Check found surf is solid floor
            float floorDot = rdVector_Dot3(&pFace->normal, &rdroid_g_zVector3);
            if ( floorDot < 0.97899997f )
            {
                bCanMove = false;
                break;
            }

            bCanMove = true;
        }
        else if ( (pCollision->type & SITHCOLLISION_THING) != 0 && pCollision->pFaceCollided )
        {
            rdFace* pFaceCollided = pCollision->pFaceCollided;

            // Transform face normal to world space
            rdVector3 worldNormal;
            rdMatrix_TransformVector34(&worldNormal, &pFaceCollided->normal, &pCollision->pThingCollided->orient);

            // Check found surf is flat floor
            float faceDot = rdVector_Dot3(&worldNormal, &rdroid_g_zVector3);
            if ( faceDot <= 0.97899997f )
            {
                bCanMove = false;
                break;
            }

            // Transform face vertex to world space
            rdModel3Mesh* pMeshCollided = pCollision->pMeshCollided;
            rdVector3* apVertices = pMeshCollided->apVertices;

            rdVector3 worldVert;
            rdMatrix_TransformPoint34(&worldVert, &apVertices[*pFaceCollided->aVertices], &pCollision->pThingCollided->orient);
            rdVector_Add3Acc(&worldVert, &pCollision->pThingCollided->pos);

            // Calculate distance from start pos to face plane
            float dist = rdMath_DistancePointToPlane(&strafeEndPos, &worldNormal, &worldVert);
            if ( dist < 0.088f || dist > 0.092f )
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

bool sithPlayerControls_CanClimbOn1m(SithThing* pThing)
{
#ifdef J3D_QOL_IMPROVEMENTS
    // Added: Alow climbing on 1m when having weapon mounted
    bool bCanClimb = sithInventory_GetCurrentWeapon(pThing) <= SITHWEAPON_IMP5;
#else
    bool bCanClimb = !sithPlayerActions_HasActiveWeapon(pThing);
#endif
    // Altered: OG did search for 2m climb surface/object first
    return bCanClimb && sithPlayerActions_CanClimbOn1m(pThing) == 1;
}

bool sithPlayerControls_CanClimbOn2m(SithThing* pThing)
{
#ifdef J3D_QOL_IMPROVEMENTS
    // Added: Alow climbing on 1m when having weapon mounted
    int curWeapon = sithInventory_GetCurrentWeapon(pThing);

    bool bCanClimb = (curWeapon < SITHWEAPON_BAZOOKA)
        || (curWeapon >= SITHWEAPON_GRENADE && curWeapon <= SITHWEAPON_IMP5 && curWeapon != SITHWEAPON_ZIPPO);
#else
    bool bCanClimb = !sithPlayerActions_HasActiveWeapon(pThing);
#endif
    // Altered: OG did search for 2m climb surface/object first
    return bCanClimb && sithPlayerActions_CanClimbOn2m(pThing) == 1;
}

bool J3DAPI sithPlayerControls_TryMountClimbWall(SithThing* pThing, bool bQuickMount)
{
    // TODO: this could be new sithPlayerActions function FindAndMountClimbWall
    if ( sithInventory_GetCurrentWeapon(pThing) == SITHWEAPON_NO_WEAPON && !sithWeapon_IsMountingWeapon(pThing) ) //TODO: why not use sithPlayerActions_HasActiveWeapon
    {
        float moveDist = pThing->collide.movesize * 2.0f;
        sithCollision_SearchForCollisions(pThing->pInSector, pThing, &pThing->pos, &pThing->orient.lvec, moveDist, pThing->collide.movesize, 0xA00);

        bool bWallMounted = false;
        SithCollision* pCollision;
        while ( (pCollision = sithCollision_PopStack()) != NULL )
        {
            if ( ((pCollision->type & SITHCOLLISION_WORLD) != 0
                || (pCollision->type & SITHCOLLISION_ADJOINCROSS) != 0)
                && pCollision->pSurfaceCollided
                && (pCollision->pSurfaceCollided->flags & SITH_SURFACE_CLIMBABLE) != 0 )
            {
                rdVector3 surfNormal = RDVECTOR_NEG3(pCollision->pSurfaceCollided->face.normal);
                surfNormal.z = 0.0f;
                rdVector_Normalize3Acc(&surfNormal);

                rdVector3 dir = pThing->orient.lvec;
                dir.z = 0.0f;
                rdVector_Normalize3Acc(&dir);

                if ( rdVector_Dot3(&dir, &surfNormal) > 0.80000001f )
                {
                    pThing->moveStatus = SITHPLAYERMOVE_MOUNTING_WALL;
                    pThing->thingInfo.actorInfo.bControlsDisabled = 1;

                    sithPhysics_ResetThingMovement(pThing);
                    sithPlayerActions_CenterOnClimbSurface(pThing, pCollision->pSurfaceCollided);
                    sithThing_AttachThingToClimbSurface(pThing, pCollision->pSurfaceCollided);

                    sithPuppet_RemoveAllTracks(pThing);
                    sithPuppet_PlayMode(pThing, SITHPUPPETSUBMODE_MOUNTWALL, sithPlayerControls_PuppetCallback);
                    sithSoundClass_PlayModeFirst(pThing, SITHSOUNDCLASS_CLIMBONTO);

                    // Added: Speedup play if runmode is enabled
                    sithPlayerControls_UpdateClimbingSpeed(pThing, bQuickMount);

                    pThing->moveInfo.physics.flags &= ~SITH_PF_FLOORSTICK;
                    bWallMounted = true;

                    sithPlayerControls_bJumpKeyActive = true;
                    sithInventory_SetSwimmingInventory(pThing, /*bItemsAvailable=*/0);
                    break;
                }
            }
        }

        sithCollision_DecreaseStackLevel();
        if ( bWallMounted )
        {
            return true;
        }
    }

    return false;
}

void J3DAPI sithPlayerControls_ProcessStillMove(SithThing* pThing, float secDeltaTime, float moveFactor, bool bRun)
{
    J3D_UNUSED(secDeltaTime);

    bool bMoving = false;
    SithPhysicsInfo* pPhysics = &pThing->moveInfo.physics;
    SithActorInfo* pActor     = &pThing->thingInfo.actorInfo;

    if ( (pThing->thingInfo.actorInfo.flags & SITH_AF_IMMOBILE) != 0 )
    {
        moveFactor = 0.1f;
    }
    else
    {
        // Reset physics height if not at normal standing height
        if ( pThing->moveInfo.physics.height != 0.090000004f )
        {
            pThing->moveInfo.physics.flags &= ~SITH_PF_ALIGNSURFACE;
            pThing->moveInfo.physics.flags |= SITH_PF_ALIGNUP;
            pThing->moveInfo.physics.height = 0.090000004f;

            sithPhysics_FindFloor(pThing, /*bNoSurfaceImpactUpdate=*/1);
            sithPuppet_SetMoveMode(pThing, SITHPUPPET_MOVEMODE_NORMAL);
        }

        // Debug climb jump
        if ( sithControl_GetKey(SITHCONTROL_CAMERAZOOMOUT, NULL) )
        {
            sithPlayerActions_Jump(pThing, 1.0f, 99);
            return;
        }

        //
        // Handle climb-on, mount climb wall, and jumping
        //
        if ( sithControl_GetKey(SITHCONTROL_JUMP, NULL) && !sithPlayerControls_bJumpKeyActive )
        {
            // Try 2m climb
            // Altered: Replaced sithPlayerActions_CanClimbOn2m(pThing) == 1 &&!sithPlayerActions_HasActiveWeapon check with sithPlayerControls_CanDoClimbOn2m
            if ( sithPlayerControls_CanClimbOn2m(pThing) )
            {
                sithPlayerActions_ClimbOn2m(pThing);
                return;
            }

            // Try 1m climb
            // Altered: Replaced sithPlayerActions_CanClimbOn1m(pThing) == 1 && !sithPlayerActions_HasActiveWeapon check with sithPlayerControls_CanDoClimbOn1m
            if ( sithPlayerControls_CanClimbOn1m(pThing) )
            {
                sithPlayerActions_ClimbOn1m(pThing);
                return;
            }

            // Try mounting wall for climbing
            // Altered: Replaced OG inlined coded with call to the new sithPlayerControls_TryMountClimbWall function
            if ( sithPlayerControls_TryMountClimbWall(pThing, /*bQuickMount=*/bRun) )
            {
                return;
            }

            //
            // No climbing surfaces found, handle normal jump
            //
            if ( !pThing->thingInfo.actorInfo.bForceMovePlay )
            {
                if ( pThing->attach.flags
                    && (pThing->attach.flags & SITH_ATTACH_SURFACE) != 0 )
                {
                    float floorDot = rdVector_Dot3(&pThing->attach.pFace->normal, &rdroid_g_zVector3);
                    if ( floorDot > 0.69999999f && floorDot < 0.75f ) // TODO: shouldn't this be a slope check, i.e. sithPhysics_CheckSlopeAngle, tho the cos angles are different
                    {
                        pThing->moveStatus = SITHPLAYERMOVE_STILL;
                        pThing->thingInfo.actorInfo.bForceMovePlay = 0;
                    }
                    else
                    {
                        sithPlayerControls_curJumpDirection = 0;
                        sithPlayerActions_JumpStart(pThing);
                    }
                }
                else
                {
                    sithPlayerControls_curJumpDirection = 0;
                    sithPlayerActions_JumpStart(pThing);
                }
                return;
            }
        } // End jump handling

        //
        // Handle crawling
        //
        if ( sithControl_GetKey(SITHCONTROL_CRAWLTOGGLE, NULL)
            && !pThing->thingInfo.actorInfo.bForceMovePlay
            && !sithPlayerActions_HasActiveWeapon(pThing) )
        {
            bool bGoIntoCrawl = false;
            if ( pThing->attach.flags )
            {
                if ( (pThing->attach.flags & (SITH_ATTACH_THING | SITH_ATTACH_THINGFACE)) != 0 )
                {
                    // Check if attached thing is stationary
                    if ( pThing->attach.attachedToStructure.pThingAttached )
                    {
                        SithThing* pThingAttached = pThing->attach.attachedToStructure.pThingAttached;
                        SithThingMoveType moveType = pThingAttached->moveType;

                        switch ( moveType )
                        {
                            case SITH_MT_NONE:
                                bGoIntoCrawl = true;
                                break;

                            case SITH_MT_PHYSICS:
                                bGoIntoCrawl = rdVector_IsZero3(&pThingAttached->moveInfo.physics.velocity);
                                break;

                            case SITH_MT_PATH:
                                bGoIntoCrawl = (pThingAttached->moveInfo.pathMovement.mode & SITH_PATHMOVE_MOVE) == 0;
                                break;
                        }

                        if ( bGoIntoCrawl && pThing->attach.pFace )
                        {
                            // Check attached face is flat floor
                            rdVector3 worldNormal;
                            rdMatrix_TransformVector34(&worldNormal, &pThing->attach.pFace->normal, &pThingAttached->orient);
                            float dotAbs = fabsf(rdVector_Dot3(&worldNormal, &rdroid_g_zVector3));
                            bGoIntoCrawl = dotAbs >= 0.98000002f;
                        }
                    }
                }
                else if ( (pThing->attach.flags & SITH_ATTACH_SURFACE) != 0
                    && pThing->attach.attachedToStructure.pSurfaceAttached )
                {
                    // Check not on water and surface is flat floor
                    if ( (pThing->attach.attachedToStructure.pSurfaceAttached->flags & (SITH_SURFACE_SHALLOWWATER | SITH_SURFACE_WATER)) != 0 )
                    {
                        bGoIntoCrawl = false;
                    }
                    else
                    {
                        // Check surface is flat floor
                        float dotAbs = fabsf(rdVector_Dot3(&pThing->attach.pFace->normal, &rdroid_g_zVector3));
                        bGoIntoCrawl = dotAbs >= 0.98000002f;
                    }
                }
            }

            // Toggle crawl state
            if ( bGoIntoCrawl )
            {
                sithPlayerActions_Stand2Crawl(pThing);
                return;
            }

            // Couldn't go into crawl, Indy says "can't go into crawl"
            if ( sithPlayerControls_secCommentWaitTimer == 0.0f )
            {
                sithSoundClass_PlayVoiceModeRandom(pThing, SITHSOUNDCLASS_SPLATTERED);
                sithPlayerControls_secCommentWaitTimer = 3.0f;
            }
        }

        //
        // Handle move forward from still
        //
        if ( sithControl_GetKey(SITHCONTROL_FORWARD, NULL) )
        {
            if ( !pThing->thingInfo.actorInfo.bForceMovePlay )
            {
                // Handle jump forward roll
                if ( sithControl_GetKey(SITHCONTROL_ACT3, NULL) )
                {
                    if ( pThing->attach.flags )
                    {
                        pThing->moveStatus = SITHPLAYERMOVE_JUMPROLLFWD;
                        sithPuppet_PlayMode(pThing, SITHPUPPETSUBMODE_JUMPROLLFWD, NULL);
                        return;
                    }
                }
                else if ( !pThing->thingInfo.actorInfo.bControlsDisabled )
                {
                    rdVector3 moveToPos = pThing->pos;
                    rdVector_MultAcc3(&moveToPos, &pThing->orient.lvec, pThing->collide.movesize);

                    int bSurfaceChange;
                    if ( !bRun && !sithPlayerActions_CheckFloorAtPos(pThing, &moveToPos, &bSurfaceChange) )
                    {
                        bMoving = false;
                        return;
                    }

                    // Okay to move forward
                    // Play stand to walk animation and change puppet move mode to walk/run
                    rdKeyframe* pKframe = sithPuppet_GetKeyframe("in_stand_bd_walk.key");
                    if ( pKframe )
                    {
                        // Disable controls during transition animation and
                        // set move status to stand2walk/run
                        pThing->thingInfo.actorInfo.bControlsDisabled = 1;
                        pThing->moveStatus = bRun ? SITHPLAYERMOVE_STAND2RUN : SITHPLAYERMOVE_STAND2WALK;

                        sithPuppet_PlayKey(pThing->renderData.pPuppet, pKframe, 1, 2, RDKEYFRAME_FADEOUT_NOLOOP | RDKEYFRAME_NOLOOP, sithPlayerControls_PuppetCallback);
                        sithPuppet_ClearMode(pThing, SITHPUPPETSUBMODE_STAND);

                        // Set appropriate move mode
                        if ( bRun )
                        {
                            sithPuppet_PlayMode(pThing, SITHPUPPETSUBMODE_RUN, NULL);
                        }
                        else
                        {
                            sithPuppet_PlayMode(pThing, SITHPUPPETSUBMODE_WALK, NULL);
                        }
                        return;
                    }
                }
            }
        }
        //
        // Handle move backward from still
        //
        else if ( sithControl_GetKey(SITHCONTROL_BACK, NULL) )
        {
            // Check for climb down wall/ledge/object
            int climbDownType = sithPlayerActions_CheckClimbDownWall(pThing);
            switch ( climbDownType )
            {
                case 1:
                    sithPlayerActions_ClimbDownToClimb(pThing, /*bAngled=*/0);
                    return;

                case 2:
                    sithPlayerActions_ClimbDownToHang(pThing);
                    return;

                case 3:
                    sithPlayerActions_ClimbDownToClimb(pThing, /*bAngled=*/1);
                    return;

                default:
                    // Handle jump back roll
                    if ( sithControl_GetKey(SITHCONTROL_ACT3, NULL) ) // Jump back roll
                    {
                        if ( pThing->attach.flags )
                        {
                            pThing->moveStatus = SITHPLAYERMOVE_JUMPROLLBACK;
                            sithPuppet_PlayMode(pThing, SITHPUPPETSUBMODE_JUMPROLLBACK, NULL);
                            return;
                        }
                    }
                    else
                    {
                        // Start moving backward
                        pThing->moveStatus = SITHPLAYERMOVE_WALKING;
                        pPhysics->thrust.y = sithPlayerControls_CalculateThrust(pActor, -1.0f, sithPlayerControls_moveFactorSlow);
                        bMoving = true;
                    }
                    break;
            }
        }
        else
        {
            // No forward/backward input, zero forward thrust
            pPhysics->thrust.y = 0.0f;
        }

        //
        // Handle step left/right
        //
        if ( sithControl_GetKey(SITHCONTROL_STPLEFT, NULL) )
        {
            if ( pThing->thingInfo.actorInfo.bForceMovePlay || !sithPlayerControls_CanStrafeMove(pThing, /*bMoveRight=*/0) )
            {
                return;
            }

            sithPlayerActions_StrafeLeft(pThing);
            sithThing_SyncThing(pThing, SITHTHING_SYNC_MOVEPOS);
            return;
        }

        if ( sithControl_GetKey(SITHCONTROL_STPRIGHT, NULL) )
        {
            if ( pThing->thingInfo.actorInfo.bForceMovePlay || !sithPlayerControls_CanStrafeMove(pThing, /*bMoveRight=*/1) )
            {
                return;
            }

            sithPlayerActions_StrafeRight(pThing);
            sithThing_SyncThing(pThing, SITHTHING_SYNC_MOVEPOS);
            return;
        }

        pPhysics->thrust.x = 0.0f;
        pPhysics->angularVelocity.yaw = 0.0f;
    } // (pThing->thingInfo.actorInfo.flags & SITH_AF_IMMOBILE) == 0

    //
    // Handle turn right
    //
    if ( sithControl_GetKey(SITHCONTROL_TURNRIGHT, NULL) )
    {
        if ( !pThing->thingInfo.actorInfo.bForceMovePlay )
        {
            // Handle jump right roll
            if ( sithControl_GetKey(SITHCONTROL_ACT3, NULL)
                && (pThing->thingInfo.actorInfo.flags & SITH_AF_IMMOBILE) == 0 )
            {
                if ( pThing->attach.flags )
                {
                    pThing->moveStatus = SITHPLAYERMOVE_JUMPRIGHT;
                    sithPuppet_PlayMode(pThing, SITHPUPPETSUBMODE_HOPRIGHT, NULL);
                    return;
                }
            }
            else
            {
                pPhysics->angularVelocity.yaw = sithPlayerControls_CalculateAngularVelocity(pActor, -1.0f, -1.0f, moveFactor);

                // Turn faster if run key held
                if ( sithControl_GetKey(SITHCONTROL_ACT1, NULL) )
                {
                    pPhysics->angularVelocity.yaw *= 2.5f;
                }
                else
                {
                    pPhysics->angularVelocity.yaw /= 1.4f;
                }

                bMoving = true;
            }
        }
    }
    //
    // Handle turn left
    //
    else if ( sithControl_GetKey(SITHCONTROL_TURNLEFT, NULL) )
    {
        if ( !pThing->thingInfo.actorInfo.bForceMovePlay )
        {
            // Handle jump left roll
            if ( sithControl_GetKey(SITHCONTROL_ACT3, NULL)
                && (pThing->thingInfo.actorInfo.flags & SITH_AF_IMMOBILE) == 0 )
            {
                if ( pThing->attach.flags )
                {
                    pThing->moveStatus = SITHPLAYERMOVE_JUMPLEFT;
                    sithPuppet_PlayMode(pThing, SITHPUPPETSUBMODE_HOPLEFT, NULL);
                    return;
                }
            }
            else
            {
                pPhysics->angularVelocity.yaw = sithPlayerControls_CalculateAngularVelocity(pActor, 1.0f, 1.0f, moveFactor);

                // Turn faster if run key held
                if ( sithControl_GetKey(SITHCONTROL_ACT1, NULL) )
                {
                    pPhysics->angularVelocity.yaw *= 2.5f;
                }
                else
                {
                    pPhysics->angularVelocity.yaw /= 1.4f;
                }

                bMoving = true;
            }
        }
    }
    else
    {
        // No turn input, zero angular velocity
        pPhysics->angularVelocity.yaw = 0.0f;
    }

    // Skip further processing if immobile
    if ( (pThing->thingInfo.actorInfo.flags & SITH_AF_IMMOBILE) != 0 )
    {
        return;
    }

    //
    // Handle mouse turn
    //
    if ( (sithMain_g_sith_mode.debugModeFlags & SITHDEBUG_INEDITOR) != 0 )
    {
        pPhysics->angularVelocity.yaw += sithPlayerControls_CalculateAngularVelocity(
            pActor,
            sithControl_GetAxis(SITHCONTROL_MOUSETURN),
            sithControl_GetKeyAsAxis(SITHCONTROL_MOUSETURN),
            moveFactor
        );
    }

    //
    // Handle activation key
    //
    if ( !sithControl_GetKey(SITHCONTROL_ACT2, NULL) || sithPlayerControls_bActionKeyActive )
    {
        //  No activation key pressed or already activated this frame
        return;
    }

    // TODO: Should also set sithPlayerControls_bActionKeyActive = true; here?

    // Activation key pressed
    // Try first to pickup/activate item
    if ( !pThing->thingInfo.actorInfo.bForceMovePlay
        && !pThing->thingInfo.actorInfo.bControlsDisabled
        && !sithWeapon_IsMountingWeapon(pThing) )
    {
        // Try find item to activate/pickup
        int bFoundGroundItem;
        SithThing* pItemThing = sithCollision_FindItemThing(pThing, &bFoundGroundItem);
        if ( pItemThing )
        {
            pThing->thingInfo.actorInfo.flags |= SITH_AF_CONTROLSDISABLED;
            sithPlayerControls_pCurActivatedItemThing = pItemThing;

            sithCog_ThingSendMessage(pItemThing, pThing, SITHCOG_MSG_ACTIVATE);
            sithPhysics_ResetThingMovement(pThing);

            sithPlayerControls_curMoveStatus              = pThing->moveStatus;
            pThing->moveStatus                            = SITHPLAYERMOVE_ACTIVATING;
            pThing->thingInfo.actorInfo.bControlsDisabled = 1;

            rdKeyframeFlags kfflags = RDKEYFRAME_NOLOOP;
            rdKeyframe* pKframe     = NULL; // Altered: Init to NULL
            if ( bFoundGroundItem )
            {
                // in_pickup_low.key
                pKframe = sithPuppet_GetKeyframeByIndex(SITHWORLD_STATICINDEX(138)); static_assert(SITHWORLD_STATICINDEX(138) == 0x808A, "");
            }
            else if ( sithInventory_GetCurrentWeapon(pThing) == SITHWEAPON_ZIPPO )
            {
                 // in_pickup_dark.key
                pKframe = sithPuppet_GetKeyframeByIndex(SITHWORLD_STATICINDEX(145)); static_assert(SITHWORLD_STATICINDEX(145) == 0x8091, "");
                kfflags = RDKEYFRAME_FADEOUT_NOLOOP;
            }
            else
            {
                // in_pickup_med.key
                pKframe = sithPuppet_GetKeyframeByIndex(SITHWORLD_STATICINDEX(139)); static_assert(SITHWORLD_STATICINDEX(139) == 0x808B, "");
            }

            if ( pKframe )
            {
                sithPuppet_PlayKey(pThing->renderData.pPuppet, pKframe, /*lowPriority=*/5, /*highPriority=*/8, kfflags, sithPlayerControls_PuppetCallback);
            }
            return;
        }

        //
        // Try activate nearby thing
        //
        if ( sithPlayerActions_Activate(pThing) == 1 )
        {
            return;
        }

        //
        // Try find push/pull object
        //
        pItemThing = NULL;
        rdVector3 pushpullMoveNorm;
        bool bGotPushPullDir = sithPlayerControls_GetPushPullMoveNorm(&pushpullMoveNorm, &pThing->orient.lvec, 15.0f);
        if ( bGotPushPullDir
            && pThing->attach.flags
            && !sithInventory_GetCurrentWeapon(pThing)
            && !sithWeapon_IsMountingWeapon(pThing) ) // TODO: why not use !sithPlayerActions_HasActiveWeapon
        {
            sithCollision_SearchForCollisions(pThing->pInSector, pThing, &pThing->pos, &pushpullMoveNorm, pThing->collide.movesize, pThing->collide.movesize, 0xA00);

            SithCollision* pCollision;
            while ( (pCollision = sithCollision_PopStack()) != NULL )
            {
                if ( (pCollision->type & SITHCOLLISION_THING) != 0
                    && pCollision->pThingCollided != pThing
                    && (pCollision->pThingCollided->flags & SITH_TF_MOVABLE) != 0
                    && pCollision->distance < 0.0089999996f )
                {
                    pItemThing = pCollision->pThingCollided;
                    break;
                }
            }
            sithCollision_DecreaseStackLevel();

            if ( pItemThing )
            {
                // Check player facing movable object
                rdVector3 dirXY = pThing->orient.lvec;
                dirXY.z = 0.0f;
                rdVector_Normalize3Acc(&dirXY);

                rdVector3 playerToItem;
                rdVector_Sub3(&playerToItem, &pThing->pos, &pItemThing->pos);
                playerToItem.z = 0.0f;
                rdVector_Normalize3Acc(&playerToItem);

                if ( fabsf(rdVector_Dot3(&dirXY, &playerToItem)) > 0.94f )
                {
                    sithPlayerControls_pMovableThing = pItemThing;
                    sithPhysics_ResetThingMovement(pThing);

                    pThing->orient.lvec = pushpullMoveNorm;
                    rdVector_Cross3(&pThing->orient.rvec, &pThing->orient.lvec, &rdroid_g_zVector3);
                    rdVector_Normalize3Acc(&pThing->orient.rvec);
                    rdVector_Cross3(&pThing->orient.uvec, &pThing->orient.rvec, &pThing->orient.lvec);

                    pThing->thingInfo.actorInfo.bControlsDisabled = 1;
                    sithPuppet_PlayMode(pThing, SITHPUPPETSUBMODE_PUSHPULLREADY, sithPlayerControls_PuppetCallback);

                    pThing->moveStatus = SITHPLAYERMOVE_PUSHPULL_READY;
                    sithPlayerControls_bActionKeyActive = true;
                    return;
                }

                pItemThing = NULL;
            }
        }

        //
        // Try board vehicle
        //
        if ( !pItemThing
            && !sithPlayerControls_bActionKeyActive
            && sithPlayerControls_BoardVehicle(pThing, /*bNoBoardAnim=*/0) )
        {
            sithPlayerControls_bActionKeyActive = true;
            return;
        }
    } // End activation key pressed handling

    if ( !bMoving && !pThing->thingInfo.actorInfo.bControlsDisabled )
    {
        pThing->moveStatus = SITHPLAYERMOVE_STILL;
    }
}

void J3DAPI sithPlayerControls_ProcessWalkMove(SithThing* pThing, float secDeltaTime, float moveFactor, bool bRun)
{
    J3D_UNUSED(secDeltaTime);

    bool bMoving = false;
    SithPhysicsInfo* pPhysics = &pThing->moveInfo.physics;
    SithActorInfo* pActor     = &pThing->thingInfo.actorInfo;

    // Reset physics height if not standing height
    if ( pThing->moveInfo.physics.height != 0.090000004f )
    {
        pThing->moveInfo.physics.flags &= ~SITH_PF_ALIGNSURFACE;
        pThing->moveInfo.physics.flags |= SITH_PF_ALIGNUP;
        pThing->moveInfo.physics.height = 0.090000004f;
        sithPhysics_FindFloor(pThing, /*bNoSurfaceImpactUpdate=*/1);
        sithPuppet_SetMoveMode(pThing, SITHPUPPET_MOVEMODE_NORMAL);
    }

    //
    // Handle walk jump
    //
    if ( sithControl_GetKey(SITHCONTROL_JUMP, NULL)
        && !sithPlayerControls_bJumpKeyActive
        && !pThing->thingInfo.actorInfo.bForceMovePlay )
    {
        // Try 2m climb
        // Altered: Replaced sithPlayerActions_CanClimbOn2m(pThing) == 1 &&!sithPlayerActions_HasActiveWeapon check with sithPlayerControls_CanDoClimbOn2m
        if ( sithPlayerControls_CanClimbOn2m(pThing) )
        {
            // Fixed: Replaced removing all tracks which might also remove armed mode
            //        with new StopMovement function
            // sithPuppet_RemoveAllTracks(pThing);
            // sithPhysics_ResetThingMovement(pThing);
            sithPlayerControls_StopMovement(pThing);

            sithPlayerActions_ClimbOn2m(pThing);
            return;
        }

        // Try 1m climb
        // Altered: Replaced sithPlayerActions_CanClimbOn1m(pThing) == 1 &&!sithPlayerActions_HasActiveWeapon check with sithPlayerControls_CanDoClimbOn1m
        if ( sithPlayerControls_CanClimbOn1m(pThing) )
        {
            // Fixed: Replaced removing all tracks which might also remove armed mode
            //        with new StopMovement function
            // sithPuppet_RemoveAllTracks(pThing);
            // sithPhysics_ResetThingMovement(pThing);
            sithPlayerControls_StopMovement(pThing);

            sithPlayerActions_ClimbOn1m(pThing);
            return;
        }

    #ifdef J3D_QOL_IMPROVEMENTS
        // Added: Try mounting wall for climbing
        //        OG no option to walk and mount wall
        if ( sithPlayerControls_TryMountClimbWall(pThing, /*bQuickMount*/true) )
        {
            return;
        }
    #endif

         // Note: in still jump case, check for no force move play is performed first
        if ( pThing->attach.flags && (pThing->attach.flags & SITH_ATTACH_SURFACE) != 0 )
        {
            float floorDot = rdVector_Dot3(&pThing->attach.pFace->normal, &rdroid_g_zVector3);
            if ( floorDot > 0.69999999f && floorDot < 0.75f ) // TODO: shouldn't this be a slope check, i.e. sithPhysics_CheckSlopeAngle, tho the cos angles are different
            {
                pThing->moveStatus = SITHPLAYERMOVE_STILL;
                pThing->thingInfo.actorInfo.bForceMovePlay = 0;
            }
            else
            {
                // Fixed: Clear walk move before executing jump move.
                //        This prevents playing walk animation while in air.
                sithPuppet_ClearMode(pThing, SITHPUPPETSUBMODE_WALK);
                sithPlayerControls_curJumpDirection = 0;
                sithPlayerActions_JumpStart(pThing);
            }
        }
        else
        {
            // Fixed: Clear walk move before executing jump move.
            //        This prevents playing walk animation while in air.
            sithPuppet_ClearMode(pThing, SITHPUPPETSUBMODE_WALK);
            sithPlayerControls_curJumpDirection = 0;
            sithPlayerActions_JumpStart(pThing);
        }

        return;
    } // Junp handling end

    //
    // Handle walk forward
    //
    if ( sithControl_GetKey(SITHCONTROL_FORWARD, NULL) )
    {
        if ( !pThing->thingInfo.actorInfo.bForceMovePlay )
        {
            // Handle jump roll
            if ( sithControl_GetKey(SITHCONTROL_ACT3, NULL) && pThing->attach.flags )
            {
                sithPuppet_ClearMode(pThing, SITHPUPPETSUBMODE_WALK);
                pThing->moveStatus = SITHPLAYERMOVE_JUMPROLLFWD;
                sithPhysics_ResetThingMovement(pThing);
                sithPuppet_PlayMode(pThing, SITHPUPPETSUBMODE_JUMPROLLFWD, NULL);
                return;
            }

            // Move forward
            pPhysics->thrust.y = sithPlayerControls_CalculateThrust(pActor, 1.0f, sithPlayerControls_moveFactorNormal);

            if ( bRun )
            {
                // Switch to running mode if run key held
                pThing->moveStatus = SITHPLAYERMOVE_RUNNING;
                if ( sithPuppet_GetModeTrack(pThing, SITHPUPPETSUBMODE_WALK) )
                {
                    sithPuppet_ClearMode(pThing, SITHPUPPETSUBMODE_WALK);
                }
                sithPuppet_PlayMode(pThing, SITHPUPPETSUBMODE_RUN, NULL);
            }
            else
            {
                pThing->moveStatus = SITHPLAYERMOVE_WALKING;
            }

            // Adjust speed by move factor
            if ( moveFactor != 1.0f )
            {
                pPhysics->thrust.y *= moveFactor;
            }

            bMoving = true;
        }
    }
    //
    // Handle walk back
    //
    else if ( sithControl_GetKey(SITHCONTROL_BACK, NULL) )
    {
        if ( !pThing->thingInfo.actorInfo.bForceMovePlay )
        {
            // Handle jump back roll
            if ( sithControl_GetKey(SITHCONTROL_ACT3, NULL) && pThing->attach.flags )
            {
                sithPuppet_ClearMode(pThing, SITHPUPPETSUBMODE_WALKBACK);
                pThing->moveStatus = SITHPLAYERMOVE_JUMPROLLBACK;
                sithPuppet_PlayMode(pThing, SITHPUPPETSUBMODE_JUMPROLLBACK, NULL);
                return;
            }

            pPhysics->thrust.y = sithPlayerControls_CalculateThrust(pActor, -1.0f, sithPlayerControls_moveFactorSlow);
            bMoving = true;
        }
    }
    else
    {
        // No forward/back input, zero forward thrust
        pPhysics->thrust.y = 0.0f;
    }

    //
    // Handle turn right key
    //
    if ( sithControl_GetKey(SITHCONTROL_TURNRIGHT, NULL) )
    {
        if ( !pThing->thingInfo.actorInfo.bForceMovePlay )
        {
            pPhysics->angularVelocity.yaw = sithPlayerControls_CalculateAngularVelocity(pActor, -1.0f, -1.0f, moveFactor);
            bMoving = true;
        }
    }
     //
    // Handle turn left key
    //
    else if ( sithControl_GetKey(SITHCONTROL_TURNLEFT, NULL) )
    {
        if ( !pThing->thingInfo.actorInfo.bForceMovePlay )
        {
            pPhysics->angularVelocity.yaw = sithPlayerControls_CalculateAngularVelocity(pActor, 1.0f, 1.0f, moveFactor);
            bMoving = true;
        }
    }
    else
    {
        // No turn input, zero angular velocity
        pPhysics->angularVelocity.yaw = 0.0f;
    }

    //
    // Handle mouse turn
    //
    if ( (sithMain_g_sith_mode.debugModeFlags & SITHDEBUG_INEDITOR) != 0 )
    {
        pPhysics->angularVelocity.yaw += sithPlayerControls_CalculateAngularVelocity(
            pActor,
            sithControl_GetAxis(SITHCONTROL_MOUSETURN),
            sithControl_GetKeyAsAxis(SITHCONTROL_MOUSETURN),
            moveFactor
        );
    }

    //
    // Transition to standing if not moving anymore
    //
    if ( !pThing->thingInfo.actorInfo.bForceMovePlay
        && !bMoving
        && !pThing->thingInfo.actorInfo.bControlsDisabled )
    {
        rdKeyframe* pKfTrack = sithPuppet_GetKeyframe("in_walk_bd_stand.key");
        if ( pKfTrack )
        {
            pThing->moveStatus = SITHPLAYERMOVE_WALK2STAND;
            pThing->thingInfo.actorInfo.bControlsDisabled = 1;

            if ( sithPuppet_GetModeTrack(pThing, SITHPUPPETSUBMODE_WALK) )
            {
                sithPuppet_ClearMode(pThing, SITHPUPPETSUBMODE_WALK);
            }
            else if ( sithPuppet_GetModeTrack(pThing, SITHPUPPETSUBMODE_WALKBACK) )
            {
                sithPuppet_ClearMode(pThing, SITHPUPPETSUBMODE_WALKBACK);
            }

            sithPuppet_PlayKey(pThing->renderData.pPuppet, pKfTrack, /*lowPriority=*/1, /*heighPriority=*/2,
                RDKEYFRAME_FADEOUT_NOLOOP | RDKEYFRAME_DISABLE_FADEIN | RDKEYFRAME_NOLOOP, sithPlayerControls_PuppetCallback);

            sithPuppet_PlayMode(pThing, SITHPUPPETSUBMODE_STAND, NULL);
            sithPhysics_ResetThingMovement(pThing);
        }
    }
}

void J3DAPI sithPlayerControls_ProcessRunMove(SithThing* pThing, float secDeltaTime, float moveFactor, bool bRun)
{
    J3D_UNUSED(secDeltaTime);

    if ( pThing->thingInfo.actorInfo.bForceMovePlay )
    {
        return;
    }

    bool bMoving = false;
    SithPhysicsInfo* pPhysics = &pThing->moveInfo.physics;
    SithActorInfo* pActor     = &pThing->thingInfo.actorInfo;

    //
    // Handle run jump
    //
    if ( sithControl_GetKey(SITHCONTROL_JUMP, NULL)
        && !sithPlayerControls_bJumpKeyActive
        && !pThing->thingInfo.actorInfo.bForceMovePlay )
    {
        // Try 2m climb
        // Altered: Replaced sithPlayerActions_CanClimbOn2m(pThing) == 1 &&!sithPlayerActions_HasActiveWeapon check with sithPlayerControls_CanDoClimbOn2m
        if ( sithPlayerControls_CanClimbOn2m(pThing) )
        {
            // Fixed: Replaced removing all tracks which might also remove armed mode
            //        with new StopMovement function
            // sithPuppet_RemoveAllTracks(pThing);
            // sithPhysics_ResetThingMovement(pThing);
            sithPlayerControls_StopMovement(pThing);

            sithPlayerActions_ClimbOn2m(pThing);
            return;
        }

        // Try 1m climb
        // Altered: Replaced sithPlayerActions_CanClimbOn1m(pThing) == 1 &&!sithPlayerActions_HasActiveWeapon check with sithPlayerControls_CanDoClimbOn1m
        if ( sithPlayerControls_CanClimbOn1m(pThing) )
        {
            // Fixed: Replaced removing all tracks which might also remove armed mode
            //        with new StopMovement function
            // sithPuppet_RemoveAllTracks(pThing);
            // sithPhysics_ResetThingMovement(pThing);
            sithPlayerControls_StopMovement(pThing);

            // Do climb move
            sithPlayerActions_ClimbOn1m(pThing);
            return;
        }

    #ifdef J3D_QOL_IMPROVEMENTS
        // Added: Try mounting wall for climbing
        //        OG no option to walk and mount wall
        if ( sithPlayerControls_TryMountClimbWall(pThing, /*bQuickMount*/true) )
        {
            return;
        }
    #endif

        // No climb, perform jump by moving to jump state
        sithPuppet_g_bPlayerLeapForward = 1;
        return;
    } // Jump handling end

    //
    // Handle run forward
    //
    if ( sithControl_GetKey(SITHCONTROL_FORWARD, NULL) )
    {
        // Handle jump roll
        if ( sithControl_GetKey(SITHCONTROL_ACT3, NULL) && pThing->attach.flags )
        {
            sithPuppet_ClearMode(pThing, SITHPUPPETSUBMODE_RUN);
            sithPhysics_ResetThingMovement(pThing);

            pThing->moveStatus = SITHPLAYERMOVE_JUMPROLLFWD;
            sithPuppet_PlayMode(pThing, SITHPUPPETSUBMODE_JUMPROLLFWD, NULL);
            return;
        }

        // Run forward
        pPhysics->thrust.y = sithPlayerControls_CalculateThrust(pActor, 1.0f, sithPlayerControls_moveFactorNormal);

        if ( bRun )
        {
            pThing->moveStatus = SITHPLAYERMOVE_RUNNING;
        }
        else
        {
            // Switch to walking mode if run key not held or toggled from always run to walk
            pThing->moveStatus = SITHPLAYERMOVE_WALKING;
            if ( sithPuppet_GetModeTrack(pThing, SITHPUPPETSUBMODE_RUN) )
            {
                sithPuppet_ClearMode(pThing, SITHPUPPETSUBMODE_RUN);
            }
            sithPuppet_PlayMode(pThing, SITHPUPPETSUBMODE_WALK, NULL);
        }

        // Adjust speed by move factor
        if ( moveFactor != 1.0f )
        {
            pPhysics->thrust.y *= moveFactor;
        }

        bMoving = true;
    }
    //
    // Handle run backward (becomes walk back)
    //
    else if ( sithControl_GetKey(SITHCONTROL_BACK, NULL) )
    {
        pThing->moveStatus = SITHPLAYERMOVE_WALKING;
        pPhysics->thrust.y = sithPlayerControls_CalculateThrust(pActor, -1.0f, sithPlayerControls_moveFactorSlow);
        bMoving = true;
    }
    else
    {
        // No forward/back input, zero forward thrust
        pPhysics->thrust.y = 0.0f;
    }

    //
    // Handle turn turn right key
    //
    if ( sithControl_GetKey(SITHCONTROL_TURNRIGHT, NULL) )
    {
        pPhysics->angularVelocity.yaw = sithPlayerControls_CalculateAngularVelocity(pActor, -1.0f, -1.0f, moveFactor);
        bMoving = true;
    }
    else if ( sithControl_GetKey(SITHCONTROL_TURNLEFT, NULL) )
    {
        pPhysics->angularVelocity.yaw = sithPlayerControls_CalculateAngularVelocity(pActor, 1.0f, 1.0f, moveFactor);
        bMoving = true;
    }
    else
    {
        // No turn input, zero angular velocity
        pPhysics->angularVelocity.yaw = 0.0f;
    }

    //
    // Handle mouse turn
    //

    if ( (sithMain_g_sith_mode.debugModeFlags & SITHDEBUG_INEDITOR) != 0 )
    {
        pPhysics->angularVelocity.yaw += sithPlayerControls_CalculateAngularVelocity(
            pActor,
            sithControl_GetAxis(SITHCONTROL_MOUSETURN),
            sithControl_GetKeyAsAxis(SITHCONTROL_MOUSETURN),
            moveFactor
        );
    }

    //
    // Transition to standing if not running anymore
    //
    if ( !bMoving && !pThing->thingInfo.actorInfo.bControlsDisabled )
    {
        rdKeyframe* pKfTrack = sithPuppet_GetKeyframe("in_walk_bd_stand.key");
        if ( pKfTrack )
        {
            pThing->moveStatus = SITHPLAYERMOVE_WALK2STAND;
            pThing->thingInfo.actorInfo.bControlsDisabled = 1;
            sithPuppet_ClearMode(pThing, SITHPUPPETSUBMODE_RUN);

            sithPuppet_PlayKey(pThing->renderData.pPuppet, pKfTrack, /*lowPriority=*/1, /*heighPriority*/2,
                RDKEYFRAME_FADEOUT_NOLOOP | RDKEYFRAME_DISABLE_FADEIN | RDKEYFRAME_NOLOOP, sithPlayerControls_PuppetCallback);

            sithPuppet_PlayMode(pThing, SITHPUPPETSUBMODE_STAND, NULL);
            sithPhysics_ResetThingMovement(pThing);
        }
    }
}

void J3DAPI sithPlayerControls_ProcessCrawlMove(SithThing* pThing, float secDeltaTime, float moveFactor)
{
    J3D_UNUSED(moveFactor);

    SithPhysicsInfo* pPhysics = &pThing->moveInfo.physics;
    SithActorInfo* pActor     = &pThing->thingInfo.actorInfo;

    //
    // Exit crawl if in water
    //
    if ( pThing->attach.flags
        && (pThing->attach.flags & SITH_ATTACH_SURFACE) != 0
        && (pThing->attach.attachedToStructure.pSurfaceAttached->flags & (SITH_SURFACE_SHALLOWWATER | SITH_SURFACE_WATER)) != 0 )
    {
        // Stand up from crawl
        sithPlayerActions_Crawl2Stand(pThing);
        return;
    }

    //
    // Handle crawl to stand toggle
    //
    if ( sithControl_GetKey(SITHCONTROL_CRAWLTOGGLE, NULL) && !pThing->thingInfo.actorInfo.bForceMovePlay )
    {
        // Stand up from crawl
        sithPlayerActions_Crawl2Stand(pThing);
        return;
    }

    //
    // Handle crawl forward
    //
    if ( sithControl_GetKey(SITHCONTROL_FORWARD, NULL) )
    {
        if ( !pThing->thingInfo.actorInfo.bForceMovePlay )
        {
            pPhysics->thrust.y = sithPlayerControls_CalculateAcceleratedThrust(pActor, 1.0f, sithPlayerControls_moveFactorSlow, secDeltaTime);
        }
    }
    //
    // Handle crawl back
    //
    else if ( sithControl_GetKey(SITHCONTROL_BACK, NULL) )
    {
        if ( !pThing->thingInfo.actorInfo.bForceMovePlay )
        {
            pPhysics->thrust.y = sithPlayerControls_CalculateAcceleratedThrust(pActor, -1.0f, sithPlayerControls_moveFactorSlow, secDeltaTime);
        }
    }
    else
    {
        // No movement input, zero forward thrust
        pPhysics->thrust.y = 0.0f;
    }

    //
    // Handle crawl turn right key
    //
    if ( sithControl_GetKey(SITHCONTROL_TURNRIGHT, NULL) )
    {
        pPhysics->angularVelocity.yaw = sithPlayerControls_CalculateAngularVelocity(pActor, -0.5f, -0.5f, moveFactor);
    }
    //
    // Handle crawl turn left key
    //
    else if ( sithControl_GetKey(SITHCONTROL_TURNLEFT, NULL) )
    {
        pPhysics->angularVelocity.yaw = sithPlayerControls_CalculateAngularVelocity(pActor, 0.5f, 0.5f, moveFactor);
    }
    else
    {
        // No turn input, zero angular velocity
        pPhysics->angularVelocity.yaw = 0.0f;
    }

    //
    // Handle activation in crawl mode
    //
    if ( sithControl_GetKey(SITHCONTROL_ACT2, NULL) && !pThing->thingInfo.actorInfo.bForceMovePlay )
    {
        // Try activate object first (different order than standing)
        if ( sithPlayerActions_Activate(pThing) == 1 )
        {
            return;
        }

        // Try pickup item
        int bFoundFloorItem;
        SithThing* pItemThing = sithCollision_FindItemThing(pThing, &bFoundFloorItem);
        if ( pItemThing )
        {
            pThing->thingInfo.actorInfo.flags |= SITH_AF_CONTROLSDISABLED;
            sithPlayerControls_pCurActivatedItemThing = pItemThing;

            sithCog_ThingSendMessage(pItemThing, pThing, SITHCOG_MSG_ACTIVATE);
            sithPhysics_ResetThingMovement(pThing);

            sithPlayerControls_curMoveStatus = pThing->moveStatus;
            pThing->moveStatus = SITHPLAYERMOVE_ACTIVATING;
            pThing->thingInfo.actorInfo.bControlsDisabled = 1;

            sithPuppet_PlayMode(pThing, SITHPUPPETSUBMODE_ACTIVATE, sithPlayerControls_PuppetCallback);
            return;
        }
    }

    //
    // Handle mouse turn
    //
    if ( (sithMain_g_sith_mode.debugModeFlags & SITHDEBUG_INEDITOR) != 0 )
    {
        pPhysics->angularVelocity.yaw += sithPlayerControls_CalculateAngularVelocity(
            pActor,
            sithControl_GetAxis(SITHCONTROL_MOUSETURN),
            sithControl_GetKeyAsAxis(SITHCONTROL_MOUSETURN),
            moveFactor
        );
    }
}

void J3DAPI sithPlayerControls_ProcessPushPullMove(SithThing* pThing, float secDeltaTime)
{
    J3D_UNUSED(secDeltaTime);

    // Indy is in push/pull ready state
    // Handle push or pull controls and move the object accordingly

    if ( pThing->thingInfo.actorInfo.bForceMovePlay )
    {
        return;
    }

    // Check that activation key is still being held, i.e., must be in push/pull grab state
    if ( !sithControl_GetKey(SITHCONTROL_ACT2, NULL) )
    {
        sithPuppet_ClearMode(pThing, SITHPUPPETSUBMODE_PUSHPULLREADY);
        pThing->moveStatus = SITHPLAYERMOVE_STILL;
        sithPlayerControls_pMovableThing = NULL;
        pThing->thingInfo.actorInfo.bControlsDisabled = 0;
        return;
    }

    // No movement key pressed
    if ( !sithControl_GetKey(SITHCONTROL_FORWARD, NULL)
        && !sithControl_GetKey(SITHCONTROL_BACK, NULL) )
    {
        return;
    }

    if ( !sithPlayerControls_pMovableThing )
    {
        return;
    }

    rdVector3 pushPullMoveNorm;
    if ( !sithPlayerControls_GetPushPullMoveNorm(&pushPullMoveNorm, &pThing->orient.lvec, 15.0f) )
    {
        return;
    }

    bool bCanMoveObject = true;
    float searchRadius = (sithPlayerControls_pMovableThing->collide.movesize < 0.1f)
        ? 0.050000001f
        : 0.094999999f;

    //
    // Handle pull object move
    //
    if ( sithControl_GetKey(SITHCONTROL_BACK, NULL) )
    {
        // Check height difference for pull - can't pull objects that are higher
        if ( searchRadius > 0.05f && pThing->pos.z > sithPlayerControls_pMovableThing->pos.z )
        {
            float heightDiff = pThing->pos.z - sithPlayerControls_pMovableThing->pos.z;
            if ( heightDiff > 0.02f ) // 20cm height difference
            {
                // If movable object is not pushgear, can't pull it
                if ( !strneq(sithPlayerControls_pMovableThing->aName, "pushgear", 8u) )
                {
                    bCanMoveObject = false;
                    if ( sithPlayerControls_secCommentWaitTimer == 0.0f )
                    {
                        sithSoundClass_PlayVoiceModeRandom(pThing, SITHSOUNDCLASS_BOAST);
                        sithPlayerControls_secCommentWaitTimer = 3.0f;
                    }

                    sithPuppet_ClearMode(pThing, SITHPUPPETSUBMODE_PUSHPULLREADY);
                    pThing->moveStatus = SITHPLAYERMOVE_STILL;
                    sithPlayerControls_pMovableThing = NULL;
                    pThing->thingInfo.actorInfo.bControlsDisabled = 0;
                    return;
                }
            }
        }

        //
        // Check if movable thing can move in pull direction
        //
        rdVector3 pullDir = RDVECTOR_NEG3(pushPullMoveNorm);
        float pullMoveDist = pThing->collide.movesize + 0.19f;
        sithCollision_SearchForCollisions(
            sithPlayerControls_pMovableThing->pInSector,
            sithPlayerControls_pMovableThing,
            &sithPlayerControls_pMovableThing->pos,
            &pullDir,
            pullMoveDist,
            searchRadius,
            0xA00
        );

        SithCollision* pCollision;
        while ( (pCollision = sithCollision_PopStack()) != NULL )
        {
            if ( (pCollision->type & SITHCOLLISION_THING) != 0
                && pCollision->pThingCollided != pThing )
            {
                bCanMoveObject = false;
                break; // Fixed: originally missing break here
            }

            if ( (pCollision->type & SITHCOLLISION_WORLD) != 0 )
            {
                bCanMoveObject = false;
                break; // Fixed: originally missing break here
            }
        }
        sithCollision_DecreaseStackLevel();

        if ( !bCanMoveObject )
        {
            if ( sithPlayerControls_secCommentWaitTimer == 0.0f )
            {
                sithSoundClass_PlayVoiceModeRandom(pThing, SITHSOUNDCLASS_BOAST); // Indy say I can't pull this object
                sithPlayerControls_secCommentWaitTimer = 3.0f;
            }

            sithPuppet_ClearMode(pThing, SITHPUPPETSUBMODE_PUSHPULLREADY);
            pThing->moveStatus = SITHPLAYERMOVE_STILL;
            sithPlayerControls_pMovableThing = NULL;
            pThing->thingInfo.actorInfo.bControlsDisabled = 0;
            return;
        }

        //
        // Check if player can move in pull direction
        //
        sithCollision_SearchForCollisions(
            pThing->pInSector,
            pThing,
            &pThing->pos,
            &pullDir,
            pullMoveDist,
            pThing->collide.movesize,
            0xA00
        );

        while ( (pCollision = sithCollision_PopStack()) != NULL )
        {
            if ( (pCollision->type & SITHCOLLISION_THING) != 0
                && pCollision->pThingCollided != pThing )
            {
                bCanMoveObject = false;
                break;
            }

            if ( (pCollision->type & SITHCOLLISION_WORLD) != 0 )
            {
                bCanMoveObject = false;
                break;
            }
        }
        sithCollision_DecreaseStackLevel();

        if ( !bCanMoveObject )
        {
            if ( sithPlayerControls_secCommentWaitTimer == 0.0f )
            {
                sithSoundClass_PlayVoiceModeRandom(pThing, SITHSOUNDCLASS_BOAST); // Indy say I can't pull this object
                sithPlayerControls_secCommentWaitTimer = 3.0f;
            }

            sithPuppet_ClearMode(pThing, SITHPUPPETSUBMODE_PUSHPULLREADY);
            pThing->moveStatus = SITHPLAYERMOVE_STILL;
            sithPlayerControls_pMovableThing = NULL;
            pThing->thingInfo.actorInfo.bControlsDisabled = 0;
            return;
        }

        //
        // Check floor at movable thing's pull end position
        //
        rdVector3 negMoveNorm = RDVECTOR_NEG3(pushPullMoveNorm); // TODO: Reuse pullDir?
        rdVector3 pullEndPos;
        rdVector_ScaleAdd3(&pullEndPos, &negMoveNorm, 0.2f, &sithPlayerControls_pMovableThing->pos);

        rdVector3 downDir = RDVECTOR_NEG3(rdroid_g_zVector3);
        SithSector* pPullEndPosSector = sithCollision_FindSectorInRadius(sithPlayerControls_pMovableThing->pInSector, &sithPlayerControls_pMovableThing->pos, &pullEndPos, 0.0f);
        if ( !pPullEndPosSector )
        {
            if ( sithPlayerControls_secCommentWaitTimer == 0.0f )
            {
                sithSoundClass_PlayVoiceModeRandom(pThing, SITHSOUNDCLASS_BOAST); // Indy say I can't pull this object
                sithPlayerControls_secCommentWaitTimer = 3.0f;
            }

            sithPuppet_ClearMode(pThing, SITHPUPPETSUBMODE_PUSHPULLREADY);
            pThing->moveStatus = SITHPLAYERMOVE_STILL;
            sithPlayerControls_pMovableThing = NULL;
            pThing->thingInfo.actorInfo.bControlsDisabled = 0;
            return;
        }

        bCanMoveObject = false;
        sithCollision_SearchForCollisions(pPullEndPosSector, NULL, &pullEndPos, &downDir, 0.17f, 0.039999999f, 0xA00); // 0.039999999f - player col size

        while ( (pCollision = sithCollision_PopStack()) != NULL )
        {
            if ( (pCollision->type & SITHCOLLISION_THING) != 0
                && pCollision->pThingCollided != pThing )
            {
                bCanMoveObject = true;
                break;
            }

            if ( (pCollision->type & SITHCOLLISION_WORLD) != 0 && pCollision->pSurfaceCollided )
            {
                float floorDot = rdVector_Dot3(&pCollision->pSurfaceCollided->face.normal, &rdroid_g_zVector3);
                if ( floorDot > 0.99900001f )
                {
                    bCanMoveObject = true;
                    break;
                }
            }
        }
        sithCollision_DecreaseStackLevel();

        if ( !bCanMoveObject )
        {
            if ( sithPlayerControls_secCommentWaitTimer == 0.0f )
            {
                sithSoundClass_PlayVoiceModeRandom(pThing, SITHSOUNDCLASS_BOAST); // Indy say I can't pull this object
                sithPlayerControls_secCommentWaitTimer = 3.0f;
            }

            sithPuppet_ClearMode(pThing, SITHPUPPETSUBMODE_PUSHPULLREADY);
            pThing->moveStatus = SITHPLAYERMOVE_STILL;
            sithPlayerControls_pMovableThing = NULL;
            pThing->thingInfo.actorInfo.bControlsDisabled = 0;
            return;
        }

        //
        // Check floor at player's pull end position
        //
        rdVector_ScaleAdd3(&pullEndPos, &negMoveNorm, 0.2f, &pThing->pos);
        pPullEndPosSector = sithCollision_FindSectorInRadius(pThing->pInSector, &pThing->pos, &pullEndPos, 0.0f);
        if ( !pPullEndPosSector )
        {
            if ( sithPlayerControls_secCommentWaitTimer == 0.0f )
            {
                sithSoundClass_PlayVoiceModeRandom(pThing, SITHSOUNDCLASS_BOAST); // Indy say I can't pull this object
                sithPlayerControls_secCommentWaitTimer = 3.0f;
            }

            sithPuppet_ClearMode(pThing, SITHPUPPETSUBMODE_PUSHPULLREADY);
            pThing->moveStatus = SITHPLAYERMOVE_STILL;
            sithPlayerControls_pMovableThing = NULL;
            pThing->thingInfo.actorInfo.bControlsDisabled = 0;
            return;
        }

        bCanMoveObject = false;
        sithCollision_SearchForCollisions(pPullEndPosSector, NULL, &pullEndPos, &downDir, 0.063000001f, 0.039999999f, 0xA00); // 0.039999999f - player col size
        while ( (pCollision = sithCollision_PopStack()) != NULL )
        {
            if ( (pCollision->type & SITHCOLLISION_THING) != 0
                && pCollision->pThingCollided != pThing )
            {
                // Check collision distance is valid (20-70cm)
                bCanMoveObject = pCollision->distance >= 0.02f && pCollision->distance <= 0.07f;
                break;
            }

            if ( (pCollision->type & SITHCOLLISION_WORLD) != 0 && pCollision->pSurfaceCollided )
            {
                // Check collision distance is valid (20-70cm) and surface is flat floor
                if ( pCollision->distance < 0.02f || pCollision->distance > 0.07f )
                {
                    bCanMoveObject = false;
                    break;
                }

                // Check if surface is straight floor
                float floorDot = rdVector_Dot3(&pCollision->pSurfaceCollided->face.normal, &rdroid_g_zVector3);
                if ( floorDot > 0.99900001f )
                {
                    // Check surface of movable thing and collided surfaces are at same height
                    if ( (sithPlayerControls_pMovableThing->attach.flags & SITH_ATTACH_SURFACE) != 0 )
                    {
                        rdFace* pSurfFace    = &pCollision->pSurfaceCollided->face;
                        rdFace* pMovableFace = sithPlayerControls_pMovableThing->attach.pFace;
                        float surfZ          = sithWorld_g_pCurrentWorld->aVertices[*pSurfFace->aVertices].z;
                        float movableZ       = sithWorld_g_pCurrentWorld->aVertices[*pMovableFace->aVertices].z;
                        bCanMoveObject       = fabsf(surfZ - movableZ) < 0.0049999999f;
                    }
                    else
                    {
                        bCanMoveObject = true;
                    }
                    break;
                }
            }
        }
        sithCollision_DecreaseStackLevel();

        if ( !bCanMoveObject )
        {
            if ( sithPlayerControls_secCommentWaitTimer == 0.0f )
            {
                sithSoundClass_PlayVoiceModeRandom(pThing, SITHSOUNDCLASS_BOAST); // Indy say I can't pull this object
                sithPlayerControls_secCommentWaitTimer = 3.0f;
            }

            sithPuppet_ClearMode(pThing, SITHPUPPETSUBMODE_PUSHPULLREADY);
            pThing->moveStatus = SITHPLAYERMOVE_STILL;
            sithPlayerControls_pMovableThing = NULL;
            pThing->thingInfo.actorInfo.bControlsDisabled = 0;
            return;
        }

        //
        // Final check for pushgear alignment
        //
        if ( strneq(sithPlayerControls_pMovableThing->aName, "pushgear", 8u) )
        {
            // Check the pull direction aligns with the pushgear direction
            // This makes sure gear can only be pulled in its current direction and not in other directions
            float alignDot = fabsf(rdVector_Dot3(&sithPlayerControls_pMovableThing->orient.lvec, &negMoveNorm));
            if ( alignDot < 0.98000002f )
            {
                if ( sithPlayerControls_secCommentWaitTimer == 0.0f )
                {
                    sithSoundClass_PlayVoiceModeRandom(pThing, SITHSOUNDCLASS_BOAST);
                    sithPlayerControls_secCommentWaitTimer = 3.0f;
                }

                // TODO: note that the push pull state is not cleared
                sithPlayerControls_pMovableThing = NULL;
                return;
            }
        }

        //
        // Finally pull the movable object
        //
        sithPuppet_ClearMode(pThing, SITHPUPPETSUBMODE_PUSHPULLREADY);
        pThing->thingInfo.actorInfo.bControlsDisabled = 0;
        sithPlayerActions_PullItem(pThing, sithPlayerControls_pMovableThing, &pushPullMoveNorm); // Note, don't negate pushPullMoveNorm as sithPlayerActions_PullItem will negate direction
    }
    //
    // Handle push object move
    //
    else
    {
        //
        // Check if movable thing can be pushed to new position
        //
        sithCollision_SearchForCollisions(
            sithPlayerControls_pMovableThing->pInSector,
            sithPlayerControls_pMovableThing,
            &sithPlayerControls_pMovableThing->pos,
            &pushPullMoveNorm,
            0.19f, // move distance
            searchRadius,
            0xA00
        );

        SithCollision* pCollision;
        while ( (pCollision = sithCollision_PopStack()) != NULL )
        {
            if ( (pCollision->type & SITHCOLLISION_THING) != 0
                && pCollision->pThingCollided != pThing )
            {
                bCanMoveObject = false;
                break; // Fixed: originally missing break here
            }

            if ( (pCollision->type & SITHCOLLISION_WORLD) != 0 )
            {
                bCanMoveObject = false;
                break; // Fixed: originally missing break here
            }
        }
        sithCollision_DecreaseStackLevel();

        if ( !bCanMoveObject )
        {
            if ( sithPlayerControls_secCommentWaitTimer == 0.0f )
            {
                sithSoundClass_PlayVoiceModeRandom(pThing, SITHSOUNDCLASS_ALERT); // indy says : can't push this object
                sithPlayerControls_secCommentWaitTimer = 3.0f;
            }

            sithPuppet_ClearMode(pThing, SITHPUPPETSUBMODE_PUSHPULLREADY);
            pThing->moveStatus = SITHPLAYERMOVE_STILL;
            sithPlayerControls_pMovableThing = NULL;
            pThing->thingInfo.actorInfo.bControlsDisabled = 0;
            return;
        }

        //
        // Check floor at push end position
        //
        rdVector3 pushEndPos;
        rdVector_ScaleAdd3(&pushEndPos, &pushPullMoveNorm, 0.2f, &sithPlayerControls_pMovableThing->pos);

        rdVector3 downDir = RDVECTOR_NEG3(rdroid_g_zVector3);
        SithSector* pPushEndPosSector = sithCollision_FindSectorInRadius(
            sithPlayerControls_pMovableThing->pInSector,
            &sithPlayerControls_pMovableThing->pos,
            &pushEndPos,
            0.0f
        );

        if ( !pPushEndPosSector )
        {
            return;
        }

        bCanMoveObject = false;
        sithCollision_SearchForCollisions(pPushEndPosSector, NULL, &pushEndPos, &downDir, 0.16f, 0.0099999998f, 0xA00);
        while ( (pCollision = sithCollision_PopStack()) != NULL )
        {
            if ( (pCollision->type & SITHCOLLISION_THING) != 0
                && pCollision->pThingCollided != sithPlayerControls_pMovableThing )
            {
                bCanMoveObject = true;
                break;
            }

            if ( (pCollision->type & SITHCOLLISION_WORLD) != 0 && pCollision->pSurfaceCollided )
            {
                float floorDot = rdVector_Dot3(&pCollision->pSurfaceCollided->face.normal, &rdroid_g_zVector3);
                if ( floorDot > 0.99900001f )
                {
                    // Check player floor surface and push target floor heights match
                    if ( (pThing->attach.flags & SITH_ATTACH_SURFACE) != 0 )
                    {
                        rdFace* pSurfFace  = &pCollision->pSurfaceCollided->face;
                        rdFace* pThingFace = pThing->attach.pFace;
                        float surfZ        = sithWorld_g_pCurrentWorld->aVertices[*pSurfFace->aVertices].z;
                        float thingZ       = sithWorld_g_pCurrentWorld->aVertices[*pThingFace->aVertices].z;
                        bCanMoveObject     = fabsf(surfZ - thingZ) < 0.0049999999f;
                    }
                    else
                    {
                        bCanMoveObject = true;
                    }
                    break;
                }
            }
        }
        sithCollision_DecreaseStackLevel();

        if ( !bCanMoveObject )
        {
            if ( sithPlayerControls_secCommentWaitTimer == 0.0f )
            {
                sithSoundClass_PlayVoiceModeRandom(pThing, SITHSOUNDCLASS_ALERT); // indy says : can't push this object
                sithPlayerControls_secCommentWaitTimer = 3.0f;
            }

            sithPuppet_ClearMode(pThing, SITHPUPPETSUBMODE_PUSHPULLREADY);
            pThing->moveStatus = SITHPLAYERMOVE_STILL;
            sithPlayerControls_pMovableThing = NULL;
            pThing->thingInfo.actorInfo.bControlsDisabled = 0;
            return;
        }

        //
        // Final check for pushgear alignment
        //
        if ( strneq(sithPlayerControls_pMovableThing->aName, "pushgear", 8u) )
        {
            // Check the push direction aligns with the pushgear direction
            // This makes sure gear can only be pushed in its current direction and not in other directions
            float alignDot = fabsf(rdVector_Dot3(&sithPlayerControls_pMovableThing->orient.lvec, &pushPullMoveNorm));
            if ( alignDot < 0.98000002f )
            {
                if ( sithPlayerControls_secCommentWaitTimer == 0.0f )
                {
                    sithSoundClass_PlayVoiceModeRandom(pThing, SITHSOUNDCLASS_ALERT); // indy says : can't push this object
                    sithPlayerControls_secCommentWaitTimer = 3.0f;
                }

                // TODO: Note that the push/pull state is not reset here
                sithPlayerControls_pMovableThing = NULL;
                return;
            }
        }

        //
        // Finally push the movable object 2m forward
        //
        sithPuppet_ClearMode(pThing, SITHPUPPETSUBMODE_PUSHPULLREADY);
        pThing->thingInfo.actorInfo.bControlsDisabled = 0;
        sithPlayerActions_PushItem(pThing, sithPlayerControls_pMovableThing, &pushPullMoveNorm);
    }
}

void J3DAPI sithPlayerControls_ProcessSlideDownMove(SithThing* pThing, float secDeltaTime)
{
    J3D_UNUSED(secDeltaTime);

    // Handle slide jump
    if ( !sithControl_GetKey(SITHCONTROL_JUMP, NULL) || sithPlayerControls_bJumpKeyActive )
    {
        return;
    }

    SithPuppetTrack* pModeTrack = sithPuppet_GetModeTrack(pThing, SITHPUPPETSUBMODE_SLIDEDOWNFWD);
    if ( pModeTrack )
    {
        sithPuppet_StopKey(pThing->renderData.pPuppet, pModeTrack->trackNum, 0.0f);
        sithPuppet_RemoveTrack(pThing, pModeTrack);
    }

    sithPhysics_ResetThingMovement(pThing);
    pThing->moveStatus = SITHPLAYERMOVE_LEAPFWD;
    sithPuppet_PlayMode(pThing, SITHPUPPETSUBMODE_LEAPLEFT, NULL);
    sithPlayerControls_bJumpKeyActive = true;
}

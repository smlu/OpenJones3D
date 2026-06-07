#include "sithWhip.h"
#include <j3dcore/j3dhook.h>

#include <rdroid/Math/rdMatrix.h>
#include <rdroid/Math/rdVector.h>

#include <sith/Cog/sithCog.h>
#include <sith/Devices/sithComm.h>
#include <sith/Devices/sithControl.h>
#include <sith/Devices/sithSoundMixer.h>
#include <sith/Dss/sithDSS.h>
#include <sith/Engine/sithCamera.h>
#include <sith/Engine/sithCollision.h>
#include <sith/Engine/sithPhysics.h>
#include <sith/Engine/sithPuppet.h>
#include <sith/Gameplay/sithInventory.h>
#include <sith/Gameplay/sithPlayerControls.h>
#include <sith/Gameplay/sithTime.h>
#include <sith/RTI/symbols.h>
#include <sith/World/sithModel.h>
#include <sith/World/sithSoundClass.h>
#include <sith/World/sithThing.h>
#include <sith/World/sithTemplate.h>
#include <sith/World/sithWeapon.h>
#include <sith/World/sithWorld.h>

#include <sound/Sound.h>

// Camera offsets for whip aim at target
static const rdVector3 sithWhip_vecCameraOffsetWhipClimbAim     = { 0.0f , -0.1f , 0.02f };
static const rdVector3 sithWhip_vecCameraLookOffsetWhipClimbAim = { 0.0f , 0.04f , 0.15000001f };

static const rdVector3 sithWhip_vecCameraOffsetWhipSwingAim     = { 0.0f , -0.2f , 0.035f };
static const rdVector3 sithWhip_vecCameraLookOffsetWhipSwingAim = { 0.0f , 0.02f , 0.02f };

static SithThing* sithWhip_pWhipThing      = NULL; // Whip actor thing, created for whip swing or whip climb move. Added: Init. to nil
static SithThing* sithWhip_pWhipSwingThing = NULL; // A thing to which whip mounts to do whip swing move. Added: Init. to nil
static SithThing* sithWhip_pWhipClimbThing = NULL; // A thing to which whip mounts to do whip climbing. Added: Init. to nil

static bool sithWhip_bClimbDismountFinish = false; // Added: Init. to false
static bool sithWhip_bFoundWhipClimbThing = false; // Unused. Added: Init. to false

void J3DAPI sithWhip_WhipClimbDismount(SithThing* pThing);
void J3DAPI sithWhip_CreatePlayerWhip(SithThing* pThing);
void sithWhip_RemoveWhip(void);

int J3DAPI sithWhip_SearchWhipSwingThing(SithThing* pThing);
int J3DAPI sithWhip_SearchWhipClimbThing(SithThing* pThing);

void sithWhip_InstallHooks(void)
{
    J3D_HOOKFUNC(sithWhip_UpdateWhipAim);
    J3D_HOOKFUNC(sithWhip_StartWhipSwing);
    J3D_HOOKFUNC(sithWhip_FinishWhipSwing);
    J3D_HOOKFUNC(sithWhip_StartWhipClimb);
    J3D_HOOKFUNC(sithWhip_SetActorWhipClimbIdle);
    J3D_HOOKFUNC(sithWhip_FinishWhipClimbDismount);
    J3D_HOOKFUNC(sithWhip_WhipClimbDismount);
    J3D_HOOKFUNC(sithWhip_ProcessWhipClimbMove);
    J3D_HOOKFUNC(sithWhip_WhipFirePuppetCallback);
    J3D_HOOKFUNC(sithWhip_WhipClimbPuppetCallback);
    J3D_HOOKFUNC(sithWhip_ClimbDismountPuppetCallback);
    J3D_HOOKFUNC(sithWhip_GetWhipSwingThing);
    J3D_HOOKFUNC(sithWhip_GetWhipClimbThing);
    J3D_HOOKFUNC(sithWhip_Reset);
    J3D_HOOKFUNC(sithWhip_Save);
    J3D_HOOKFUNC(sithWhip_Restore);
    J3D_HOOKFUNC(sithWhip_SearchWhipSwingThing);
    J3D_HOOKFUNC(sithWhip_SearchWhipClimbThing);
    J3D_HOOKFUNC(sithWhip_CreatePlayerWhip);
    J3D_HOOKFUNC(sithWhip_RemoveWhip);
}

void sithWhip_ResetGlobals(void)
{}

void J3DAPI sithWhip_UpdateWhipAim(SithThing* pThing)
{
    SITH_ASSERTREL((pThing != NULL) && (pThing->type == SITH_THING_PLAYER));
    if ( sithWhip_SearchWhipSwingThing(pThing) )
    {
        sithCamera_SetExtCameraOffset(&sithWhip_vecCameraOffsetWhipSwingAim);
        sithCamera_SetExtCameraLookOffset(&sithWhip_vecCameraLookOffsetWhipSwingAim);
        sithWeapon_SendMessageAim(pThing, /*bAim=*/1);
    }

    else if ( sithWhip_SearchWhipClimbThing(pThing) )
    {
        sithCamera_SetExtCameraOffset(&sithWhip_vecCameraOffsetWhipClimbAim);
        sithCamera_SetExtCameraLookOffset(&sithWhip_vecCameraLookOffsetWhipClimbAim);
        sithWeapon_SendMessageAim(pThing, /*bAim=*/1);
    }
    else
    {
        sithWhip_Reset();
        sithWeapon_SendMessageAim(pThing, /*bAim=*/0);
    }
}

int J3DAPI sithWhip_StartWhipSwing(SithThing* pThing)
{
    SITH_ASSERTREL((pThing != NULL) && (pThing->type == SITH_THING_PLAYER));

    float dot = rdVector_Dot3(&pThing->orient.rvec, &sithWhip_pWhipSwingThing->orient.lvec);
    if ( dot != 0.0f )
    {
        rdVector3 pyr;
        if ( dot >= 0.0f )
        {
            rdVector_Set3(&pyr, 0.0f, 90.0f, 0.0f);
        }
        else
        {
            rdVector_Set3(&pyr, 0.0f, -90.0f, 0.0f);
        }

        rdVector3 newLVec = sithWhip_pWhipSwingThing->orient.lvec;
        rdVector_Rotate3Acc(&newLVec, &pyr);

        // Set new LVec
        pThing->orient.lvec = newLVec;

        // Calculate RVec
        rdVector_Cross3(&pThing->orient.rvec, &pThing->orient.lvec, &rdroid_g_zVector3);
        rdVector_Normalize3Acc(&pThing->orient.rvec);

        // Calculate UVec
        rdVector_Cross3(&pThing->orient.uvec, &pThing->orient.rvec, &pThing->orient.lvec);
    }

    sithWhip_CreatePlayerWhip(pThing);
    if ( !sithWhip_pWhipThing )
    {
        return 0;
    }

    sithWhip_pWhipSwingThing = NULL; // Note, no need to free as it's only reference pointer to world thing

    pThing->moveStatus        = SITHPLAYERMOVE_WHIPSWINGING;
    pThing->forceMoveStartPos = pThing->pos;
    pThing->thingInfo.actorInfo.bControlsDisabled = 1;

    SithCog* pCog = sithCog_GetCogByIndex(SITHWORLD_STATICINDEX(8)); // weap_whip.cog
    if ( pCog )
    {
        // Send whip thing start notification via User0 message
        sithWhip_pWhipThing->pCaptureCog = pCog;
        sithCog_SendMessage(pCog, SITHCOG_MSG_USER0, SITHCOG_SYM_REF_THING, sithWhip_pWhipThing->idx, SITHCOG_SYM_REF_THING, pThing->idx, 0);
    }

    sithPuppet_PlayForceMoveMode(pThing, SITHPUPPETSUBMODE_WHIPSWING, sithPlayerControls_PuppetCallback);
    sithPuppet_PlayMode(sithWhip_pWhipThing, SITHPUPPETSUBMODE_WHIPSWING, NULL);
    return 1;
}

void J3DAPI sithWhip_FinishWhipSwing(SithThing* pThing)
{
    if ( pThing->thingInfo.actorInfo.bForceMovePlay == 1 )
    {
        sithPuppet_FinishForceMove(pThing, /*bStopTracks=*/0);
    }

    SithCog* pCog = sithCog_GetCogByIndex(SITHWORLD_STATICINDEX(8)); // weap_whip.cog
    if ( pCog )
    {
        sithCog_SendMessage(pCog, SITHCOG_MSG_DEACTIVATED, SITHCOG_SYM_REF_THING, -99, SITHCOG_SYM_REF_THING, pThing->idx, 0);

        // Added: Added check for whip thing and in case not destried notify it's cog
        if ( sithWhip_pWhipThing && sithWhip_pWhipThing->type != SITH_THING_FREE )
        {
            // Notify script to detach & destroy whip and restore indy weapon model
            sithCog_ThingSendMessageEx(sithWhip_pWhipThing, NULL, SITHCOG_MSG_CALLBACK, 0, RDKEYMARKER_TURNOFF, 0, 0);
        }
        sithWhip_pWhipThing = NULL; // Note, thing whip actor should be destroyed by weap_whip.cog script when calling DetachThingMesh
    }
}

int J3DAPI sithWhip_StartWhipClimb(SithThing* pThing, SithThing* pWhippedThing)
{
    SITH_ASSERTREL((pThing != NULL) && (pThing->type == SITH_THING_PLAYER));

    sithCamera_RestoreExtCamera();
    sithInventory_SetSwimmingInventory(pThing, /*bItemsAvailable=*/0);
    sithWeapon_SetLastWeapon(SITHWEAPON_PISTOL);

    pThing->moveStatus = SITHPLAYERMOVE_WHIPCLIMB_START;
    pThing->thingInfo.actorInfo.bControlsDisabled = 1;

    sithThing_DetachThing(pThing);
    pThing->moveInfo.physics.flags &= ~SITH_PF_USEGRAVITY;

    sithPuppet_StopMode(pThing, SITHPUPPETSUBMODE_DRAWWEAPON, 0.0f);
    sithPuppet_StopMode(pThing, SITHPUPPETSUBMODE_AIMWEAPON, 0.0f);

    pThing->pos.x = pWhippedThing->pos.x;
    pThing->pos.y = pWhippedThing->pos.y;

    pThing->forceMoveStartPos = pThing->pos;
    pThing->collide.movesize  = 0.055f;

    sithWhip_CreatePlayerWhip(pThing);
    if ( !sithWhip_pWhipThing )
    {
        return 0;
    }

    // Why the 2 calls?
    sithPuppet_PlayForceMoveMode(pThing, SITHPUPPETSUBMODE_WHIPCLIMBMOUNT, sithWhip_WhipClimbPuppetCallback);
    sithPuppet_PlayForceMoveMode(pThing, SITHPUPPETSUBMODE_WHIPCLIMBMOUNT, NULL);

    sithPuppet_PlayMode(sithWhip_pWhipThing, SITHPUPPETSUBMODE_WHIPCLIMBMOUNT, sithWhip_WhipFirePuppetCallback);
    sithSoundClass_PlayModeFirst(pThing, SITHSOUNDCLASS_CLIMBONTO);
    return 1;
}

void J3DAPI sithWhip_SetActorWhipClimbIdle(SithThing* pThing)
{
    sithWhip_pWhipClimbThing = NULL;
    sithPhysics_ResetThingMovement(pThing);

    pThing->moveStatus = SITHPLAYERMOVE_WHIPCLIMBIDLE;
    pThing->thingInfo.actorInfo.bControlsDisabled = 0;

    pThing->attach.flags = SITH_ATTACH_THINGCLIMBWHIP;
    pThing->attach.posOffset.z = 0.0f;

    sithPuppet_PlayMode(pThing, SITHPUPPETSUBMODE_WHIPCLIMBIDLE, NULL);
    sithThing_RemoveSwapEntry(pThing, pThing->thingInfo.actorInfo.weaponInfo.swapRefNum);
}

void J3DAPI sithWhip_FinishWhipClimbDismount(SithThing* pThing)
{
    pThing->thingInfo.actorInfo.bControlsDisabled = 0;
    if ( sithWhip_bClimbDismountFinish )
    {
        sithWhip_bClimbDismountFinish = false;
        sithWhip_WhipClimbDismount(pThing);
    }
}

void J3DAPI sithWhip_WhipClimbDismount(SithThing* pThing)
{
    pThing->collide.movesize = 0.04f;

    if ( sithWhip_pWhipThing )
    {
        sithThing_RemoveThing(sithWorld_g_pCurrentWorld, sithWhip_pWhipThing);
        sithWhip_pWhipThing = NULL;
    }

    sithPuppet_StopMode(pThing, SITHPUPPETSUBMODE_WHIPCLIMBIDLE, 0.0f);

    sithInventory_SetCurrentWeapon(pThing, SITHWEAPON_WHIP);
    pThing->thingInfo.actorInfo.weaponInfo.curWeaponID = SITHWEAPON_WHIP;
    sithWeapon_SetWeaponModel(pThing, SITHWEAPON_WHIP);

    sithPuppet_PlayMode(pThing, SITHPUPPETSUBMODE_DRAWWEAPON, NULL);

    pThing->moveStatus   = SITHPLAYERMOVE_FALLING;
    pThing->attach.flags = 0;
    pThing->moveInfo.physics.flags |= SITH_PF_USEGRAVITY;
    sithInventory_SetSwimmingInventory(pThing, 1);
}

void J3DAPI sithWhip_ProcessWhipClimbMove(SithThing* pThing, float secDeltaTime)
{
    J3D_UNUSED(secDeltaTime);
    SITH_ASSERTREL((pThing != NULL) && (pThing->type == SITH_THING_PLAYER));
    if ( !pThing->thingInfo.actorInfo.bControlsDisabled && !pThing->thingInfo.actorInfo.bForceMovePlay )
    {
        const int bJumpOff = sithControl_GetKey(SITHCONTROL_JUMP, NULL);
        const int bUp = sithControl_GetKey(SITHCONTROL_FORWARD, NULL);
        const int bDown = sithControl_GetKey(SITHCONTROL_BACK, NULL);

        if ( bJumpOff )
        {
            if ( pThing->attach.posOffset.z <= 3.0f )
            {
                sithWhip_WhipClimbDismount(pThing);
                sithSoundClass_PlayModeFirst(pThing, SITHSOUNDCLASS_CLIMBDOWNONTO);
            }
            else
            {
                pThing->forceMoveStartPos = pThing->pos;
                sithPuppet_PlayForceMoveMode(pThing, SITHPUPPETSUBMODE_WHIPCLIMBDISMOUNT, sithWhip_ClimbDismountPuppetCallback);
                sithSoundClass_PlayModeFirst(pThing, SITHSOUNDCLASS_CLIMBDOWNONTO);

                if ( sithWhip_pWhipThing )
                {
                    sithPuppet_PlayMode(sithWhip_pWhipThing, SITHPUPPETSUBMODE_WHIPCLIMBDISMOUNT, NULL);
                }

                pThing->thingInfo.actorInfo.bForceMovePlay = 1;
            }
        }

        if ( bUp && !bJumpOff )
        {
            // Move up
            if ( pThing->attach.posOffset.z <= 3.0f )
            {
                pThing->forceMoveStartPos = pThing->pos;

                sithPuppet_PlayForceMoveMode(pThing, SITHPUPPETSUBMODE_WHIPCLIMBUP, NULL);
                sithSoundClass_PlayModeFirst(pThing, SITHSOUNDCLASS_CLIMBHANDLEFT);

                pThing->thingInfo.actorInfo.bForceMovePlay = 1;
                pThing->attach.posOffset.z = pThing->attach.posOffset.z + 1.0f;
            }
        }
        else if ( bDown && !bJumpOff ) // Fixed: Added check for !bJumpOff. This should fix bug where controls could become unresponsive.
        {
            // Move down
            if ( pThing->attach.posOffset.z > 0.0f )
            {
                pThing->forceMoveStartPos = pThing->pos;

                sithPuppet_PlayForceMoveMode(pThing, SITHPUPPETSUBMODE_WHIPCLIMBDOWN, NULL);
                sithSoundClass_PlayModeFirst(pThing, SITHSOUNDCLASS_CLIMBHANDRIGHT);

                pThing->thingInfo.actorInfo.bForceMovePlay = 1;
                pThing->attach.posOffset.z = pThing->attach.posOffset.z - 1.0f;
            }
            else
            {
                sithWhip_WhipClimbDismount(pThing);
            }
        }
        else if ( sithControl_GetKey(SITHCONTROL_TURNLEFT, NULL) )
        {
            sithThing_SyncThing(pThing, SITHTHING_SYNC_MOVEPOS);
            pThing->moveInfo.physics.angularVelocity.yaw = sithPlayerControls_CalculateAngularVelocity(&pThing->thingInfo.actorInfo, 1.0f, 1.0f, 1.0f);
        }

        else if ( sithControl_GetKey(SITHCONTROL_TURNRIGHT, NULL) )
        {
            sithThing_SyncThing(pThing, SITHTHING_SYNC_MOVEPOS);
            pThing->moveInfo.physics.angularVelocity.yaw = sithPlayerControls_CalculateAngularVelocity(&pThing->thingInfo.actorInfo, -1.0f, -1.0f, 1.0f);
        }
        else
        {
            sithPhysics_ResetThingMovement(pThing);
        }
    }
}

void J3DAPI sithWhip_WhipFirePuppetCallback(SithThing* pThing, int track, rdKeyMarkerType markerType)
{
    J3D_UNUSED(pThing);
    J3D_UNUSED(track);

    if ( markerType == RDKEYMARKER_ATTACK )
    {
        tSoundHandle hSnd = Sound_GetSoundHandle(SITHWORLD_STATICINDEX(3)); // gen_whip_fire.wav
        if ( hSnd )
        {
            sithSoundMixer_PlaySound(hSnd, 1.0f, 0.0f, (SoundPlayFlag)0);
        }
    }
}

void J3DAPI sithWhip_WhipClimbPuppetCallback(SithThing* pThing, int track, rdKeyMarkerType markerType)
{
    if ( markerType != 0 )
    {
        if ( markerType == RDKEYMARKER_CLIMB )
        {
            sithThing_RemoveSwapEntry(pThing, pThing->thingInfo.actorInfo.weaponInfo.swapRefNum);

            rdVector3 pos = sithWhip_pWhipThing->pos;

            sithWhip_RemoveWhip(); // Note, the function also frees sithWhip_pWhipThing

            sithWhip_pWhipThing = sithTemplate_GetTemplate("+whipc_actor");
            if ( sithWhip_pWhipThing )
            {
                pos.z += sithWhip_pWhipThing->renderData.data.pModel3->radius;
                sithWhip_pWhipThing = sithThing_CreateThingAtPos(sithWhip_pWhipThing, &pos, &rdroid_g_identMatrix34, pThing->pInSector, NULL);
                if ( sithWhip_pWhipThing )
                {
                    sithPuppet_PlayMode(sithWhip_pWhipThing, SITHPUPPETSUBMODE_WHIPCLIMBIDLE, NULL);
                }
            }

            sithWhip_WhipFirePuppetCallback(pThing, track, RDKEYMARKER_ATTACK);
        }
    }
    else
    {
        if ( pThing->thingInfo.actorInfo.bForceMovePlay == 1 )
        {
            sithPuppet_FinishForceMove(pThing, /*bStopTracks=*/0);
        }

        sithPuppet_FreeTrackByIndex(pThing, track);
    }
}

void J3DAPI sithWhip_ClimbDismountPuppetCallback(SithThing* pThing, int track, rdKeyMarkerType mt)
{
    if ( !mt )
    {
        sithWhip_bClimbDismountFinish = true;
        if ( pThing->thingInfo.actorInfo.bForceMovePlay == 1 )
        {
            sithPuppet_FinishForceMove(pThing, /*bStopTracks=*/0);
        }

        sithPuppet_FreeTrackByIndex(pThing, track);
    }
}

SithThing* sithWhip_GetWhipSwingThing(void)
{
    return sithWhip_pWhipSwingThing;
}

SithThing* sithWhip_GetWhipClimbThing(void)
{
    return sithWhip_pWhipClimbThing;
}

void sithWhip_Reset(void)
{
    sithWhip_pWhipSwingThing = NULL;
    sithWhip_pWhipClimbThing = NULL;

    if ( sithCamera_IsOpen() )
    { // Added Check for sithCamera system to be opened. Reason: JonesMain closes sithCamera before closing sith system resulting in sithCamera_g_pCurCamera = NULL here
        sithCamera_RestoreExtCamera();
    }
}

int J3DAPI sithWhip_Save(DPID idTo, uint32_t outstream)
{
    SITHDSS_STARTOUT(SITHDSS_WHIPSTATUS);

    SITHDSS_PUSHINT16(sithThing_GetThingIndex(sithWhip_pWhipThing));
    // *(uint16_t*)sithMulti_g_message.data = sithThing_GetThingIndex(sithWhip_pWhipThing);
    SITHDSS_PUSHUINT8(sithWhip_bClimbDismountFinish);
    //sithMulti_g_message.data[2] = sithWhip_bClimbDismountFinish;

    //sithMulti_g_message.type = SITHDSS_WHIPSTATUS;
    //sithMulti_g_message.length = 3;
    SITHDSS_ENDOUT;
    return sithMessage_SendMessage(&sithMulti_g_message, idTo, outstream, 0x1);
}

int J3DAPI sithWhip_Restore(const SithMessage* pMsg)
{
    SITH_ASSERTREL(pMsg);

    SITHDSS_STARTIN(pMsg);
    sithWhip_pWhipThing = sithThing_GetThingByIndex(SITHDSS_POPINT16());//sithThing_GetThingByIndex(*(int16_t*)pMsg->data);
    sithWhip_bClimbDismountFinish = SITHDSS_POPUINT8(); //pMsg->data[2];
    SITHDSS_ENDIN;

    sithWhip_pWhipSwingThing = NULL;
    sithWhip_pWhipClimbThing = NULL;

    return 1;
}

int J3DAPI sithWhip_SearchWhipSwingThing(SithThing* pThing)
{
    //moveNorm.x = 0.0f; // What's the point of this init?
    //moveNorm.y = 1.0f;
    //moveNorm.z = 1.0f;

    SITH_ASSERTREL((pThing != NULL) && (pThing->type == SITH_THING_PLAYER));
    sithWhip_pWhipSwingThing = NULL;

    // Search for whip swing thing in the forward,up direction at max distance 0.65 (6.5m)
    rdVector3 moveNorm = pThing->orient.lvec;
    moveNorm.z = sqrtf(moveNorm.x * moveNorm.x + moveNorm.y * moveNorm.y);
    rdVector_Normalize3Acc(&moveNorm);

    if ( sithCollision_SearchForCollisions(pThing->pInSector, pThing, &pThing->pos, &moveNorm, 0.65f, 0.075f, 0x122) > 0.65f )
    {
        return sithWhip_pWhipSwingThing != NULL;
    }

    while ( 1 )
    {
        SithCollision* pCollision = sithCollision_PopStack();
        if ( !pCollision )
        {
            break;
        }

        // TODO: Refactor and add else scope to null found swing thing and break out of scope in case any other collision is found in the path
        if ( (pCollision->type & SITHCOLLISION_THING) != 0 && pCollision->pThingCollided != pThing && (pCollision->pThingCollided->flags & SITH_TF_WHIPSWING) != 0 )
        {
            sithWhip_pWhipSwingThing = pCollision->pThingCollided;
            break;
        }
    }

    // TODO: Add collision search in forward direction from current position for 8m, to detect if there is anything blocking whip swing trajectory.

    sithCollision_DecreaseStackLevel();
    return sithWhip_pWhipSwingThing != NULL;
}

int J3DAPI sithWhip_SearchWhipClimbThing(SithThing* pThing)
{
    SITH_ASSERTREL((pThing != NULL) && (pThing->type == SITH_THING_PLAYER));

    sithWhip_pWhipClimbThing = NULL; // No need to free as it's reference to world thing

    // Search for whip climb thing in the up direction at max distance 0.6 (6m)
    rdVector3 moveNorm = rdroid_g_zVector3;
    if ( sithCollision_SearchForCollisions(pThing->pInSector, pThing, &pThing->pos, &moveNorm, 0.6f, 0.075f, 0x122) <= 0.6f )
    {
        while ( 1 )
        {
            SithCollision* pCollision = sithCollision_PopStack();
            if ( !pCollision )
            {
                break;
            }

            if ( (pCollision->type & SITHCOLLISION_THING) != 0 && pCollision->pThingCollided != pThing && (pCollision->pThingCollided->flags & SITH_TF_WHIPCLIMB) != 0 )
            {
                sithWhip_pWhipClimbThing = pCollision->pThingCollided;
                if ( sithWhip_pWhipClimbThing->pos.z - pThing->pos.z >= 0.30000001f )
                {
                    break;
                }

                sithWhip_pWhipClimbThing = NULL;
            }
        }

        sithCollision_DecreaseStackLevel();
    }

    // Note, the unused var sithWhip_bFoundWhipClimbThing
    if ( sithWhip_pWhipClimbThing )
    {
        sithWhip_bFoundWhipClimbThing = true;
        return 1;
    }
    else
    {
        sithWhip_bFoundWhipClimbThing = false;
        return 0;
    }
}

void J3DAPI sithWhip_CreatePlayerWhip(SithThing* pThing)
{
    SITH_ASSERTREL((pThing != NULL) && (pThing->type == SITH_THING_PLAYER) && (pThing->renderData.pPuppet != NULL));

    const SithThing* pTemplate = sithTemplate_GetTemplate("+whip_actor");
    if ( !pTemplate )
    {
        // TODO: maybe add debug log
        return;
    }

    int handMeshIdx = sithThing_GetThingMeshIndex(pThing, "inrhand"); // Altered: Moved down here from the start of the scope
    if ( handMeshIdx == -1 )
    {
        return;
    }

    sithWhip_pWhipThing = sithThing_CreateThingAtPos(pTemplate, &pThing->pos, &pThing->orient, pThing->pInSector, 0);
    if ( sithWhip_pWhipThing )
    {
        rdModel3* pWhipHandModel = sithModel_GetModelByIndex(SITHWORLD_STATICINDEX(31u)); // whip hand - weap_whip.3do
        if ( pWhipHandModel )
        {
            pThing->thingInfo.actorInfo.weaponInfo.swapRefNum = sithThing_AddSwapEntry(pThing, handMeshIdx, pWhipHandModel, /*meshNumSrc=*/0);
        }

        sithThing_AttachThingToThingMesh(sithWhip_pWhipThing, pThing, handMeshIdx);
    }
}

void sithWhip_RemoveWhip(void)
{
    if ( sithWhip_pWhipThing )
    {
        sithWhip_pWhipThing->thingInfo.actorInfo.pThingMeshAttached = NULL;
        sithWhip_pWhipThing->thingInfo.actorInfo.attachMeshNum      = 0;

        sithThing_RemoveThing(sithWorld_g_pCurrentWorld, sithWhip_pWhipThing);
        sithWhip_pWhipThing = NULL;
    }
}
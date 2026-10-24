#include "sithPlayerActions.h"

#include <j3dcore/j3dhook.h>

#include <rdroid/Math/rdMath.h>
#include <rdroid/Math/rdMatrix.h>
#include <rdroid/Math/rdVector.h>

#include <sith/Cog/sithCog.h>
#include <sith/Devices/sithComm.h>
#include <sith/Dss/sithDSSThing.h>
#include <sith/Engine/sithAnimate.h>
#include <sith/Engine/sithCamera.h>
#include <sith/Engine/sithCollision.h>
#include <sith/Engine/sithPhysics.h>
#include <sith/Engine/sithPuppet.h>
#include <sith/Gameplay/sithFX.h>
#include <sith/Gameplay/sithInventory.h>
#include <sith/Gameplay/sithPlayer.h>
#include <sith/Gameplay/sithTime.h>
#include <sith/Gameplay/sithPlayerControls.h>
#include <sith/Gameplay/sithVehicleControls.h>
#include <sith/RTI/symbols.h>
#include <sith/World/sithSoundClass.h>
#include <sith/World/sithSurface.h>
#include <sith/World/sithThing.h>
#include <sith/World/sithWeapon.h>
#include <sith/World/sithWorld.h>

#include <sound/Sound.h>

uint32_t sithPlayerActions_msecCurActivateTime = 0; // Altered: Init to 0
static const float sithPlayerActions_raftActivateSearchRadius = 0.07f;

float J3DAPI sithPlayerActions_GetLedgeSurfaceGrabPosZ(const SithSurface* pLedgeSurf); // Added
float J3DAPI sithPlayerActions_GetLedgeThingGrabPosZ(const SithThing* pThing, const rdFace* pFace, const rdModel3Mesh* pMesh);

void sithPlayerActions_InstallHooks(void)
{
    // Uncomment only lines for functions that have full definition and doesn't call original function (non-thunk functions)

    J3D_HOOKFUNC(sithPlayerActions_Activate);
    J3D_HOOKFUNC(sithPlayerActions_Jump);
    J3D_HOOKFUNC(sithPlayerActions_MoveToPlayerPosition);
    J3D_HOOKFUNC(sithPlayerActions_PushItem);
    J3D_HOOKFUNC(sithPlayerActions_PullItem);
    J3D_HOOKFUNC(sithPlayerActions_ClimbPullUp);
    J3D_HOOKFUNC(sithPlayerActions_ClimbMove);
    J3D_HOOKFUNC(sithPlayerActions_UnmountWall);
    J3D_HOOKFUNC(sithPlayerActions_ClimbOn1m);
    J3D_HOOKFUNC(sithPlayerActions_ClimbOn2m);
    J3D_HOOKFUNC(sithPlayerActions_LeapForward);
    J3D_HOOKFUNC(sithPlayerActions_JumpForward);
    J3D_HOOKFUNC(sithPlayerActions_HopLeft);
    J3D_HOOKFUNC(sithPlayerActions_HopRight);
    J3D_HOOKFUNC(sithPlayerActions_JumpRollBack);
    J3D_HOOKFUNC(sithPlayerActions_JumpRollForward);
    J3D_HOOKFUNC(sithPlayerActions_StrafeLeft);
    J3D_HOOKFUNC(sithPlayerActions_StrafeRight);
    J3D_HOOKFUNC(sithPlayerActions_HopBack);
    J3D_HOOKFUNC(sithPlayerActions_Stand2Crawl);
    J3D_HOOKFUNC(sithPlayerActions_Crawl2Stand);
    J3D_HOOKFUNC(sithPlayerActions_JumpStart);
    J3D_HOOKFUNC(sithPlayerActions_HasActiveWeapon);
    J3D_HOOKFUNC(sithPlayerActions_ClimbDownToClimb);
    J3D_HOOKFUNC(sithPlayerActions_ClimbDownToHang);
    J3D_HOOKFUNC(sithPlayerActions_CheckFloorAtPos);
    J3D_HOOKFUNC(sithPlayerActions_FindLedge);
    J3D_HOOKFUNC(sithPlayerActions_CanClimbOn1m);
    J3D_HOOKFUNC(sithPlayerActions_CanClimbOn2m);
    J3D_HOOKFUNC(sithPlayerActions_CheckClimbDownWall);
    J3D_HOOKFUNC(sithPlayerActions_GrabLedge);
    J3D_HOOKFUNC(sithPlayerActions_FindAndAttachToClimbWall);
    J3D_HOOKFUNC(sithPlayerActions_CanPullUp);
    J3D_HOOKFUNC(sithPlayerActions_GetLedgeThingGrabPosZ);
    J3D_HOOKFUNC(sithPlayerActions_StartInvisibility);
    J3D_HOOKFUNC(sithPlayerActions_EndInvisibility);
    J3D_HOOKFUNC(sithPlayerActions_IsInvisible);
    J3D_HOOKFUNC(sithPlayerActions_StartJewelFlying);
    J3D_HOOKFUNC(sithPlayerActions_EnableJewelFlying);
    J3D_HOOKFUNC(sithPlayerActions_DisableJewelFlying);
    J3D_HOOKFUNC(sithPlayerActions_CenterOnClimbSurface);
    J3D_HOOKFUNC(sithPlayerActions_MoveToCrawlPosition);
}

void sithPlayerActions_ResetGlobals(void)
{
    int sithPlayerActions_g_jewelFlyingPuppetTrackNum_tmp = -1;
    STD_COPYMEM(&sithPlayerActions_g_jewelFlyingPuppetTrackNum, &sithPlayerActions_g_jewelFlyingPuppetTrackNum_tmp, sizeof(sithPlayerActions_g_jewelFlyingPuppetTrackNum));

    STD_ZEROMEM(&sithPlayerActions_g_pCurLedgeSurface, sizeof(sithPlayerActions_g_pCurLedgeSurface));
    STD_ZEROMEM(&sithPlayerActions_g_pCurLedgeThingModelFace, sizeof(sithPlayerActions_g_pCurLedgeThingModelFace));
    STD_ZEROMEM(&sithPlayerActions_g_pCurLedgeThingModel, sizeof(sithPlayerActions_g_pCurLedgeThingModel));
    STD_ZEROMEM(&sithPlayerActions_g_pPlasma, sizeof(sithPlayerActions_g_pPlasma));
    STD_ZEROMEM(&sithPlayerActions_g_bJewelFlying, sizeof(sithPlayerActions_g_bJewelFlying));
    STD_ZEROMEM(&sithPlayerActions_g_bPlayerInvisible, sizeof(sithPlayerActions_g_bPlayerInvisible));
    STD_ZEROMEM(&sithPlayerActions_g_pChalkWriteSurf, sizeof(sithPlayerActions_g_pChalkWriteSurf));
    STD_ZEROMEM(&sithPlayerActions_g_chalkWritePos, sizeof(sithPlayerActions_g_chalkWritePos));
    STD_ZEROMEM(&sithPlayerActions_g_pChalkWriteSector, sizeof(sithPlayerActions_g_pChalkWriteSector));
}

int J3DAPI sithPlayerActions_Activate(SithThing* pThing)
{
    if ( sithPlayerActions_msecCurActivateTime + 500 > sithTime_g_clockTime )
    {
        return 0;
    }

    // Activate only if no weapon is currently selected
    int typeId = pThing->thingInfo.actorInfo.weaponInfo.curWeaponID;
    if ( typeId != SITHWEAPON_NO_WEAPON && typeId < (unsigned int)SITHWEAPON_ZIPPO )
    {
        return 0;
    }

    sithPlayerActions_msecCurActivateTime = sithTime_g_clockTime;

    rdMatrix34 eyeorient = pThing->orient;
    rdVector3 eyepoint   = pThing->pos;
    SithSector* pEyepointSec = sithCollision_FindSectorInRadius(pThing->pInSector, &pThing->pos, &eyepoint, 0.0f);
    if ( !pEyepointSec )
    {
        // Note: OG code was at line no.: 139
        SITHLOG_ERROR("Activate: eyepoint outside of sector?\n");
        return 0;
    }

    SithSurface* pSurfaceCollided = NULL;
    if ( pThing != sithPlayer_g_pLocalPlayerThing
        || sithInventory_GetCurrentItem(pThing) != SITHWEAPON_CHALK
        || !pThing->pInSector
        || (pThing->pInSector->flags & SITH_SECTOR_UNDERWATER) != 0
        || sithInventory_GetCurrentWeapon(pThing)
        || sithWeapon_IsMountingWeapon(pThing)
        || pThing->moveStatus )
    {
        // Try to activate surface or thing in forward direction
        int bObjActivated = 0;
        if ( sithInventory_GetCurrentItem(pThing) != 56 ) // 56 - raft patchkit
        {
            eyepoint         = pThing->pos;
            pEyepointSec     = sithCollision_FindSectorInRadius(pThing->pInSector, &pThing->pos, &eyepoint, 0.0f);
            pSurfaceCollided = NULL;

            float moveDist = pThing->collide.movesize + 0.039999999f;
            sithCollision_SearchForCollisions(pEyepointSec, pThing, &eyepoint, &eyeorient.lvec, moveDist, 0.050000001f, 0x02);

            SithCollision* pCollision;
            while ( (pCollision = sithCollision_PopStack()) != NULL )
            {
                if ( (pCollision->type & SITHCOLLISION_WORLD) != 0 )
                {
                    if ( (sithMain_g_sith_mode.debugModeFlags & SITHDEBUG_TRACKSHOTS) != 0 )
                    {
                        // Note: OG code was at line no.: 230
                        int surfIdx = sithSurface_GetSurfaceIndex(pCollision->pSurfaceCollided);
                        SITHLOG_STATUS("Activate hit surface %d.\n", surfIdx);
                    }

                    // Send activate message to collided surface if cur selected item ID is zippo or less
                    if ( (pCollision->pSurfaceCollided->flags & SITH_SURFACE_COGLINKED) != 0 && typeId < (unsigned int)SITHWEAPON_IMP1 )
                    {
                        sithCog_SurfaceSendMessage(pCollision->pSurfaceCollided, pThing, SITHCOG_MSG_ACTIVATE);
                        bObjActivated = 1;
                        break;
                    }

                    if ( !pSurfaceCollided )
                    {
                        pSurfaceCollided = pCollision->pSurfaceCollided;
                        float distance = pCollision->distance;
                    }
                }
                else if ( (pCollision->type & SITHCOLLISION_THING) != 0 )
                {
                    SithThing* pThingCollided = pCollision->pThingCollided;

                    // Activate collided/activated thing if collided thing is not item, weapon and player and activation thing is not AI
                    if ( pThingCollided->type != SITH_THING_ITEM
                        && pThingCollided->type != SITH_THING_WEAPON
                        && (pThing->controlType != SITH_CT_AI || pThingCollided->type != SITH_THING_PLAYER) )
                    {
                        if ( (sithMain_g_sith_mode.debugModeFlags & SITHDEBUG_TRACKSHOTS) != 0 )
                        {
                            // Note: OG code was at line no.: 264
                            SITHLOG_STATUS("Activate hit thing %s.\n", pThingCollided->aName);
                        }

                        // Send activate message to collided thing if collided thing is not movable
                        if ( (pThingCollided->flags & SITH_TF_COGLINKED) != 0 && (pThingCollided->flags & SITH_TF_MOVABLE) == 0 )
                        {
                            sithCog_ThingSendMessage(pThingCollided, pThing, SITHCOG_MSG_ACTIVATE);
                            bObjActivated = 1;

                            if ( typeId <= (unsigned int)SITHWEAPON_ZIPPO )
                            {
                                break;
                            }

                            // If cur weapon is IMP1 and imp1 stand is activated, COG script will disable fire
                            if ( pThingCollided->renderData.type == RD_THING_MODEL3 && pThingCollided->renderData.data.pModel3 )
                            {
                                if ( streqi(pThingCollided->renderData.data.pModel3->aName, "imp_stands.3do") )
                                {
                                    SithInventoryType* pInventoryType = sithInventory_GetType(typeId);
                                    if ( pInventoryType->pCog )
                                    {
                                        sithCog_SendMessage(pInventoryType->pCog, SITHCOG_MSG_CHANGED, SITHCOG_SYM_REF_THING, pThing->idx, SITHCOG_SYM_REF_THING, pThingCollided->idx, 0);
                                    }
                                }

                                break;
                            }
                        }
                    }
                }
            }

            sithCollision_DecreaseStackLevel();

            // If nothing was activated try to activate surface/thing in the down direction
            if ( !bObjActivated )
            {
                rdVector3 endPos;
                rdVector_ScaleAdd3(&endPos, &pThing->orient.lvec, 0.035f, &pThing->pos);

                SithSector* pStartSector = sithCollision_FindSectorInRadius(pThing->pInSector, &pThing->pos, &endPos, 0.0f);
                if ( pStartSector )
                {
                    rdVector3 downVector;
                    rdVector_Neg3(&downVector, &rdroid_g_zVector3);

                    sithCollision_SearchForCollisions(pStartSector, pThing, &endPos, &downVector, 0.11f, 0.039999999f, 0x02);

                    while ( (pCollision = sithCollision_PopStack()) != NULL )
                    {
                        if ( (pCollision->type & SITHCOLLISION_WORLD) != 0 )
                        {
                            if ( (sithMain_g_sith_mode.debugModeFlags & SITHDEBUG_TRACKSHOTS) != 0 )
                            {
                                // Note: OG code was at line no.: 322
                                int surfIdx = sithSurface_GetSurfaceIndex(pCollision->pSurfaceCollided);
                                SITHLOG_STATUS("Activate down hit surface %d.\n", surfIdx);
                            }

                            if ( (pCollision->pSurfaceCollided->flags & SITH_SURFACE_COGLINKED) != 0 )
                            {
                                sithCog_SurfaceSendMessage(pCollision->pSurfaceCollided, pThing, SITHCOG_MSG_ACTIVATE);
                                bObjActivated = 1;
                                break;
                            }
                        }
                        else if ( (pCollision->type & SITHCOLLISION_THING) != 0
                            && pCollision->pThingCollided->type != SITH_THING_ITEM
                            && pCollision->pThingCollided->type != SITH_THING_WEAPON
                            && (pThing->controlType != SITH_CT_AI || pCollision->pThingCollided->type != SITH_THING_PLAYER) )
                        {
                            if ( (sithMain_g_sith_mode.debugModeFlags & SITHDEBUG_TRACKSHOTS) != 0 )
                            {
                                // Note: OG code was at line no.: 344
                                SITHLOG_STATUS("Activate down hit thing %s.\n", pCollision->pThingCollided->aName);
                            }

                            if ( (pCollision->pThingCollided->flags & SITH_TF_COGLINKED) != 0 && (pCollision->pThingCollided->flags & SITH_TF_MOVABLE) == 0 )
                            {
                                sithCog_ThingSendMessage(pCollision->pThingCollided, pThing, SITHCOG_MSG_ACTIVATE);
                                bObjActivated = 1;
                                break;
                            }
                        }
                    }

                    sithCollision_DecreaseStackLevel();
                }
            }
        }

        if ( bObjActivated || pThing != sithPlayer_g_pLocalPlayerThing || !pThing->pInSector || (pThing->pInSector->flags & SITH_SECTOR_UNDERWATER) != 0 )
        {
            return bObjActivated;
        }

        // Mo object was activated, check If menu item was activated
        int curItem = sithInventory_GetCurrentItem(pThing);

        // Check if inflatable raft was activated in menu
        if ( curItem == 57  // limpraft
            && !sithInventory_GetCurrentWeapon(pThing)
            && !sithWeapon_IsMountingWeapon(pThing)
            && !sithPhysics_IsVehicleThing(pThing) )
        {
            if ( (pThing->attach.flags & SITH_ATTACH_SURFACE) != 0
                && (pThing->attach.attachedToStructure.pSurfaceAttached->flags & SITH_SURFACE_ISFLOOR) != 0
                && pThing->attach.attachedToStructure.pSurfaceAttached->face.normal.z > 0.99959999f )
            {
                rdVector3 startPos;
                rdVector_ScaleAdd3(&startPos, &pThing->orient.lvec, 0.1f, &pThing->pos);

                SithSector* pFoundSector = sithCollision_FindSectorInRadius(pThing->pInSector, &pThing->pos, &startPos, 0.001f);
                if ( !pFoundSector )
                {
                    return 0;
                }

                // Find surface to inflate raft
                rdVector3 moveNorm;
                rdVector_Neg3(&moveNorm, &rdroid_g_zVector3);
                float moveDist = sithPhysics_GetThingHeight(pThing) + 0.059999999f;

                sithCollision_SearchForCollisions(pFoundSector, NULL, &startPos, &moveNorm, moveDist, sithPlayerActions_raftActivateSearchRadius, 0x01);

                int bBoardRaft = 0;
                float raftInflateColDist = 0.0f;

                SithCollision* pCollision;
                while ( (pCollision = sithCollision_PopStack()) != NULL )
                {
                    // Check if "coldwater" like surface was hit
                    if ( (pCollision->type & SITHCOLLISION_ADJOINTOUCH) == 0 )
                    {
                        if ( (pCollision->type & SITHCOLLISION_WORLD) != 0
                            && (pCollision->type & SITHCOLLISION_FACE) != 0
                            && (pCollision->pSurfaceCollided->flags & SITH_SURFACE_WATER) != 0 )
                        {
                            // Check if water surface is at valid depth for raft boarding
                            // i.e.: If collision dist is >= 0.04f and <= 0.06f and collided surface is up
                            float waterDepthFromPlayer = (sithPlayerActions_raftActivateSearchRadius + pCollision->distance) - sithPhysics_GetThingHeight(pThing);
                            if ( waterDepthFromPlayer >= 0.04f && waterDepthFromPlayer <= 0.06f
                                && pCollision->pSurfaceCollided->face.normal.z > 0.99984002f )
                            {
                                raftInflateColDist = pCollision->distance + sithPlayerActions_raftActivateSearchRadius;
                                bBoardRaft = 1;
                            }
                        }

                        break;
                    }

                    // Check if normal water surface was hit where mirror adjoin is underwater surface
                    if ( pCollision->pSurfaceCollided
                        && pCollision->pSurfaceCollided->pAdjoin
                        && (pCollision->pSurfaceCollided->pAdjoin->pAdjoinSector->flags & SITH_SECTOR_UNDERWATER) != 0 )
                    {
                        raftInflateColDist = pCollision->distance;
                        bBoardRaft = 1;
                        break;
                    }
                }

                sithCollision_DecreaseStackLevel();

                if ( bBoardRaft )
                {
                    // Find raft player
                    size_t raftPlayerNum;
                    SithThing* pRaftPlayerThing = NULL;
                    for ( raftPlayerNum = 0; raftPlayerNum < sithPlayer_g_numPlayers; ++raftPlayerNum )
                    {
                        if ( streq(sithPlayer_g_aPlayers[raftPlayerNum].pThing->aName, "raftplayer")
                            || streq(sithPlayer_g_aPlayers[raftPlayerNum].pThing->aName, "shirtraftplayer") )
                        {
                            pRaftPlayerThing = sithPlayer_g_aPlayers[raftPlayerNum].pThing;
                            break;
                        }
                    }

                    if ( pRaftPlayerThing )
                    {
                        float raftPlayerHeight = sithPhysics_GetThingHeight(pRaftPlayerThing);
                        rdVector3 raftInflatePos = startPos;
                        raftInflatePos.z = raftInflatePos.z - (raftInflateColDist - raftPlayerHeight);

                        pFoundSector = sithCollision_FindSectorInRadius(pFoundSector, &startPos, &raftInflatePos, 0.0f);
                        if ( !pFoundSector )
                        {
                            // Note: OG code was at line no.: 473
                            SITHLOG_ERROR("This is really bad, trying to board raft in non-existent sector..\n");
                        }

                        rdMatrix34 raftOrient;
                        raftOrient.uvec = pThing->orient.uvec;
                        raftOrient.lvec = pThing->orient.rvec;
                        rdVector_Neg3(&raftOrient.rvec, &pThing->orient.lvec);

                    #ifdef J3D_DEBUG
                        // In debug version for some reason bPlayerInRaftSec is always 0
                        int bPlayerInRaftSec = 0;
                    #else
                        int bPlayerInRaftSec = (pFoundSector == pThing->pInSector) ? 1 : 0;
                    #endif

                        if ( bPlayerInRaftSec )
                        {
                            sithCog_SectorSendMessage(pThing->pInSector, pThing, SITHCOG_MSG_EXITED);
                        }

                        if ( sithPlayer_g_bGuybrush )
                        {
                            sithPlayer_ToggleGuybrush();
                        }

                        // Hide cur local player
                        sithPlayer_HidePlayer(sithPlayer_g_playerNum);

                        // Exit sector of raft player thing
                        sithThing_ExitSector(pRaftPlayerThing);

                        // Copy player state to raft player thing
                        pRaftPlayerThing->thingInfo.actorInfo.health          = pThing->thingInfo.actorInfo.health;
                        *pRaftPlayerThing->thingInfo.actorInfo.pPlayer        = *pThing->thingInfo.actorInfo.pPlayer;
                        pRaftPlayerThing->thingInfo.actorInfo.pPlayer->pThing = pRaftPlayerThing;

                        // Set raft player as local & show it
                        sithPlayer_SetLocalPlayer(raftPlayerNum);
                        sithPlayer_ShowPlayer(raftPlayerNum, 0);

                        // Set raft position
                        sithThing_SetPositionAndOrient(pRaftPlayerThing, &raftInflatePos, &raftOrient);
                        sithThing_EnterSector(pRaftPlayerThing, pFoundSector, 1, bPlayerInRaftSec == 0);
                        sithPuppet_SetMoveMode(pRaftPlayerThing, SITHPUPPET_MOVEMODE_NORMAL);

                        // Stick raft player to water & set camera focus to raft player
                        sithPhysics_FindFloor(pRaftPlayerThing, 1);
                        sithCamera_SetCameraFocus(&sithCamera_g_aCameras[SITHCAMERA_EXTCAMERANUM], pRaftPlayerThing, NULL);

                        // Play raft boarding animation
                        pRaftPlayerThing->moveStatus = SITHPLAYERMOVE_RAFT_BOARDING;
                        sithPuppet_PlayMode(pRaftPlayerThing, SITHPUPPETSUBMODE_HOPBACK, sithVehicleControls_PuppetCallback);
                        pRaftPlayerThing->thingInfo.actorInfo.bControlsDisabled = 1;

                        // Disable inventory
                        sithInventory_SetSwimmingInventory(pRaftPlayerThing, /*bItemsAvailable=*/0);
                        pRaftPlayerThing->alpha = 1.0f;

                        // Enable raft repair kit
                        sithInventory_SetInventoryDisabled(pRaftPlayerThing, 56u, 0);

                        sithVehicleControls_StartBoardCutscene(pRaftPlayerThing); // User0 - startCutscene
                        if ( pRaftPlayerThing->pCog )
                        {
                            sithCog_SendMessage(pRaftPlayerThing->pCog, SITHCOG_MSG_BOARDED, SITHCOG_SYM_REF_THING, pRaftPlayerThing->idx, SITHCOG_SYM_REF_THING, pThing->idx, 0);
                        }

                        if ( (pThing->thingInfo.actorInfo.flags & SITH_AF_INVULNERABLE) != 0 )
                        {
                            pRaftPlayerThing->thingInfo.actorInfo.flags |= SITH_AF_INVULNERABLE;
                        }

                        // Reset raft leak damage
                        if ( pRaftPlayerThing->thingInfo.actorInfo.endurance.raftLeakDamage )
                        {
                            pRaftPlayerThing->thingInfo.actorInfo.endurance.raftLeakDamage = 1;
                        }

                        sithCog_ThingSendMessageEx(pRaftPlayerThing, NULL, SITHCOG_MSG_CALLBACK, 0, 16, 0, 0); // 16 - inflate start
                        return 1;
                    }
                }
            }
        } // end of raft item activation

        // Patch raft (56 - raft patch)
        else if ( curItem == 56 && (pThing->moveInfo.physics.flags & SITH_PF_RAFT) != 0 && pThing->thingInfo.actorInfo.endurance.raftLeakDamage )
        {
            pThing->thingInfo.actorInfo.endurance.raftLeakDamage = 0;
            float amount = sithInventory_GetInventory(pThing, 56) - 1.0f;
            sithInventory_SetInventory(pThing, 56u, amount);

            sithSoundClass_PlayModeFirst(pThing, SITHSOUNDCLASS_ACTIVATE);
            sithSoundClass_StopMode(pThing, SITHSOUNDCLASS_JUMPWOOD); // patch sound
            return 1;
        }

        return bObjActivated;
    }

    // Player activated chalk
    eyepoint.z = eyepoint.z + 0.039999999f;
    pEyepointSec = sithCollision_FindSectorInRadius(pThing->pInSector, &pThing->pos, &eyepoint, 0.0f);
    if ( !pEyepointSec )
    {
        return 0;
    }

    float moveDist = pThing->collide.movesize + 0.039999999f + 0.039999999f;
    sithCollision_SearchForCollisions(pEyepointSec, pThing, &eyepoint, &pThing->orient.lvec, moveDist, 0.0099999998f, 0x03);

    float distance = 0.0f;
    SithCollision* pCollision;
    while ( (pCollision = sithCollision_PopStack()) != NULL )
    {
        if ( (pCollision->type & SITHCOLLISION_WORLD) != 0 )
        {
            if ( (pCollision->pSurfaceCollided->flags & SITH_SURFACE_COGLINKED) == 0
                && (!pCollision->pSurfaceCollided->pAdjoin || (pCollision->pSurfaceCollided->pAdjoin->flags & SITH_ADJOIN_NOPLAYERMOVE) == 0) )
            {
                pSurfaceCollided = pCollision->pSurfaceCollided;
                distance = pCollision->distance;
                break;
            }
        }
        else if ( (pCollision->type & SITHCOLLISION_THING) != 0 )
        {
            break;
        }
    }

    sithCollision_DecreaseStackLevel();

    if ( !pSurfaceCollided
        || (pSurfaceCollided->pAdjoin
            && ((pSurfaceCollided->pAdjoin->flags & SITH_ADJOIN_NOPLAYERMOVE) != 0 || (pSurfaceCollided->pAdjoin->flags & SITH_ADJOIN_MOVE) == 0))
        || (pSurfaceCollided->flags & SITH_SURFACE_WATER) != 0 )
    {
        sithSoundClass_PlayVoiceModeRandom(pThing, SITHSOUNDCLASS_SPLATTERED);
        return 0;
    }

    rdVector3 writePos;
    rdVector_ScaleAdd3(&writePos, &pThing->orient.lvec, distance + pThing->collide.movesize, &pThing->pos);
    writePos.z += pThing->collide.movesize; // Adjust Z position to be above the surface

    SithSector* pWriteSec = sithCollision_FindSectorInRadius(pThing->pInSector, &pThing->pos, &writePos, 0.0f);
    if ( !pWriteSec )
    {
        return 0;
    }

    // Set chalk writing parameters
    pThing->moveStatus = SITHPLAYERMOVE_ACTIVATING;
    pThing->thingInfo.actorInfo.bControlsDisabled = 1;

    sithPlayerActions_g_pChalkWriteSurf   = pSurfaceCollided;
    sithPlayerActions_g_chalkWritePos     = writePos;
    sithPlayerActions_g_pChalkWriteSector = pWriteSec;

    sithCog_ThingSendMessage(pThing, pThing, SITHCOG_MSG_USER6); // Sends start writing with chalk
    sithPhysics_ResetThingMovement(pThing);
    sithPuppet_PlayMode(pThing, SITHPUPPETSUBMODE_ACTIVATE, sithPlayerControls_PuppetCallback);
    return 1;
}

void J3DAPI sithPlayerActions_Jump(SithThing* pThing, float speedFactor, int jumpDirection)
{
    // Early exit if thing can't jump
    if ( (pThing->type != SITH_THING_ACTOR && pThing->type != SITH_THING_PLAYER)
        || (pThing->thingInfo.actorInfo.flags & SITH_AF_IMMOBILE) != 0 )
    {
        return;
    }

    SithPhysicsInfo* pPhysicsInfo = (SithPhysicsInfo*)&pThing->moveInfo;
    float jumpSpeed = pThing->thingInfo.actorInfo.jumpSpeed * speedFactor; // Found in debug version

    // Determine jump direction vector
    rdVector3 jumpDir = rdroid_g_zeroVector3;
    switch ( jumpDirection )
    {
        case 0:  // up - already zero vector
            break;
        case 1:  // forward
            jumpDir = rdroid_g_yVector3;
            break;
        case 2:  // backward
            jumpDir = RDVECTOR_NEG3(rdroid_g_yVector3);
            break;
        case 3:  // left
            jumpDir = RDVECTOR_NEG3(rdroid_g_xVector3);
            break;
        case 4:  // right
            jumpDir = rdroid_g_xVector3;
            break;
        case 99: // special forward
            jumpDir = rdroid_g_yVector3;
            break;
        case -1: // hop back
            jumpDir = RDVECTOR_NEG3(rdroid_g_yVector3); // TODO: This seems like leftover from original code?
            jumpDir = rdroid_g_zeroVector3; // This seems like a bug in original - should be negative Y? Or maybe again a leftover since hop back is played by puppet mode?
            break;
        default:
            break;
    }

    // Calculate jump newVel
    rdVector3 newJumpVelocity = { 0.0f, 0.0f, 2.4f }; // Default jump height
    if ( jumpDirection != 0 )
    {
        rdVector3 transformedDir;
        rdMatrix_TransformVector34(&transformedDir, &jumpDir, &pThing->orient);
        rdVector_Add3Acc(&newJumpVelocity, &transformedDir);
    }
    rdVector_Normalize3Acc(&newJumpVelocity);

    if ( pThing->attach.flags != 0 ) // Note, this if check could be optimized and moved to the top in if statement at the start of the function
    {
        // Determine jump sound based on surface/thing attached to
        SithSoundClassMode sndmode = SITHSOUNDCLASS_JUMP; // Default sound
        if ( pThing->attach.flags & (SITH_ATTACH_THING | SITH_ATTACH_THINGFACE) )
        {
            // Attached to thing
            SithThing* pThingAttached = pThing->attach.attachedToStructure.pThingAttached;
            if ( pThingAttached->flags & SITH_TF_METAL )
            {
                sndmode = SITHSOUNDCLASS_JUMPMETAL;
            }
            else if ( pThingAttached->flags & SITH_TF_EARTH )
            {
                sndmode = SITHSOUNDCLASS_JUMPEARTH;
            }
            else if ( pThingAttached->flags & SITH_TF_SNOW )
            {
                sndmode = SITHSOUNDCLASS_JUMPSNOW;
            }
            else if ( pThingAttached->flags & SITH_TF_WOOD )
            {
                sndmode = SITHSOUNDCLASS_JUMPWOOD;
            }
        }
        else
        {
            // Attached to surface
            SithSurface* pSurfaceAttached = pThing->attach.attachedToStructure.pSurfaceAttached;
            if ( pSurfaceAttached->flags & SITH_SURFACE_METAL )
            {
                sndmode = SITHSOUNDCLASS_JUMPMETAL;
            }
            else if ( pSurfaceAttached->flags & (SITH_SURFACE_WATER | SITH_SURFACE_SHALLOWWATER) )
            {
                sndmode = SITHSOUNDCLASS_JUMPWATER;
            }
            else if ( pSurfaceAttached->flags & SITH_SURFACE_EARTH )
            {
                sndmode = SITHSOUNDCLASS_JUMPEARTH;
            }
            else if ( pSurfaceAttached->flags & SITH_SURFACE_SNOW )
            {
                sndmode = SITHSOUNDCLASS_JUMPSNOW;
            }
            else if ( pSurfaceAttached->flags & SITH_SURFACE_WOOD )
            {
                sndmode = SITHSOUNDCLASS_JUMPWOOD;
            }
            else if ( pSurfaceAttached->flags & SITH_SURFACE_ECHO )
            {
                sndmode = SITHSOUNDCLASS_JUMPHARDECHO;
            }
            else if ( pSurfaceAttached->flags & SITH_SURFACE_WOODECHO )
            {
                sndmode = SITHSOUNDCLASS_JUMPWOODECHO;
            }
            else if ( pSurfaceAttached->flags & SITH_SURFACE_EARTHECHO )
            {
                sndmode = SITHSOUNDCLASS_JUMPEARTHECHO;
            }
            else if ( pSurfaceAttached->flags & SITH_SURFACE_AETHERIUM )
            {
                sndmode = SITHSOUNDCLASS_JUMPAET;
            }
        }

        // Play jump sound
        tSoundChannelHandle hSndChannel = sithSoundClass_PlayModeRandom(pThing, sndmode);
        if ( hSndChannel && sithMessage_g_outputstream )
        {
            int chguid = Sound_GetChannelGUID(hSndChannel);
            sithDSSThing_PlaySoundMode(pThing, sndmode, chguid, -1.0f);
        }

        // Detach thing from surface and setup jump
        sithThing_DetachThing(pThing);
        pThing->forceMoveStartPos       = pThing->pos;
        pPhysicsInfo->angularVelocity.y = 0.0f;

        // Set movement status and puppet mode based on jump direction
        switch ( jumpDirection )
        {
            case 0: // up
                pThing->moveStatus = SITHPLAYERMOVE_JUMPUP;
                sithPuppet_PlayForceMoveMode(pThing, SITHPUPPETSUBMODE_JUMPUP, NULL);
                break;
            case 1: // forward
                pThing->moveStatus = SITHPLAYERMOVE_JUMPFWD;
                pThing->collide.movesize = 0.059999999f;
                sithPuppet_PlayMode(pThing, SITHPUPPETSUBMODE_JUMPFWD, NULL);
                break;
            case -1: // hop back
                pThing->moveStatus = SITHPLAYERMOVE_JUMPBACK;
                sithPuppet_PlayMode(pThing, SITHPUPPETSUBMODE_HOPBACK, NULL);
                sithPlayerActions_HopBack(pThing);
                break;
            default: // other jump movement directions
                pPhysicsInfo->velocity = newJumpVelocity;
                break;
        }

        if ( sithMessage_g_outputstream )
        {
            sithThing_SyncThing(pThing, SITHTHING_SYNC_POS);
        }
    }
}

void J3DAPI sithPlayerActions_MoveToPlayerPosition(SithThing* pThing, size_t playerNum)
{
    if ( playerNum >= sithPlayer_g_numPlayers )
    {
        return;
    }

    SithPlayer* pPlayer = &sithPlayer_g_aPlayers[playerNum];
    if ( (pPlayer->flags & SITH_PLAYER_PLACED) != 0 )
    {
        pThing->orient      = pPlayer->orient;
        pThing->pos         = pThing->orient.dvec;
        pThing->orient.dvec = rdroid_g_zeroVector3;
        sithThing_SetSector(pThing, pPlayer->pInSector, /*bNotify=*/0);
    }

    if ( pThing->moveType == SITH_MT_PHYSICS )
    {
        sithPhysics_ResetThingMovement(pThing);
        pThing->moveInfo.physics.flags &= ~SITH_PF_ALIGNED;
        sithPhysics_FindFloor(pThing, 1);
    }
}

void J3DAPI sithPlayerActions_PushItem(SithThing* pThing, SithThing* pItem, const rdVector3* pDirection)
{
    pThing->thingInfo.actorInfo.bForceMovePlay = 1;
    pThing->moveStatus        = SITHPLAYERMOVE_PUSHING;
    pThing->forceMoveStartPos = pThing->pos;
    pThing->collide.movesize /= 2.0f;

    int trackNum = sithPuppet_PlayForceMoveMode(pThing, SITHPUPPETSUBMODE_PUSHITEM, NULL);
    if ( trackNum == -1 )
    {
        pThing->thingInfo.actorInfo.bForceMovePlay = 0;
        pThing->moveStatus        = SITHPLAYERMOVE_STILL;
        pThing->collide.movesize *= 2.0f;
    }
    else if ( trackNum >= 0 )
    {
        sithAnimate_PushItem(pThing, pItem, pDirection, trackNum);
        sithPlayerControls_g_bCutsceneMode = 1;
    }
}

void J3DAPI sithPlayerActions_PullItem(SithThing* pThing, SithThing* pItem, const rdVector3* pDirection)
{
    pThing->thingInfo.actorInfo.bForceMovePlay = 1;
    pThing->moveStatus        = SITHPLAYERMOVE_PULLING;
    pThing->forceMoveStartPos = pThing->pos;
    pThing->collide.movesize /= 2.0f;

    int trackNum = sithPuppet_PlayForceMoveMode(pThing, SITHPUPPETSUBMODE_PULLITEM, NULL);
    if ( trackNum == -1 )
    {
        pThing->thingInfo.actorInfo.bForceMovePlay = 0;
        pThing->moveStatus        = SITHPLAYERMOVE_STILL;
        pThing->collide.movesize *= 2.0f;
    }
    else if ( trackNum >= 0 )
    {
        sithPlayerControls_g_bCutsceneMode = 1;
        rdVector3 dir = RDVECTOR_NEG3(*pDirection);
        sithAnimate_PullItem(pThing, pItem, &dir, trackNum);
    }
}

void J3DAPI sithPlayerActions_ClimbPullUp(SithThing* pThing)
{
    pThing->thingInfo.actorInfo.bForceMovePlay = 1;
    pThing->collide.movesize /= 4.0f;
    pThing->forceMoveStartPos = pThing->pos;
    pThing->moveStatus        = SITHPLAYERMOVE_PULLINGUP;

    pThing->moveInfo.physics.flags &= ~SITH_PF_ALIGNED;
    pThing->moveInfo.physics.flags |= SITH_PF_FLOORSTICK;

    sithPuppet_PlayForceMoveMode(pThing, SITHPUPPETSUBMODE_CLIMBPULLINGUP, NULL);
    sithSoundClass_PlayModeFirst(pThing, SITHSOUNDCLASS_CLIMBONTO);
    sithInventory_SetSwimmingInventory(pThing, /*bItemsAvailable=*/0); // Fixed: Disable inventory when climbing. Re-enabled in sithPuppet_FinishForceMove
                                                                       //        Originally it was enabled.
}

void J3DAPI sithPlayerActions_ClimbMove(SithThing* pThing, SithSurface* pSurf, int direction)
{
    pThing->thingInfo.actorInfo.bForceMovePlay = 1;
    pThing->forceMoveStartPos = pThing->pos;
    pThing->collide.movesize /= 4.0f;

    switch ( direction )
    {
        case 1:
            pThing->moveStatus = SITHPLAYERMOVE_CLIMBING_UP;
            sithPuppet_PlayForceMoveMode(pThing, SITHPUPPETSUBMODE_CLIMBWALLUP, NULL);
            break;

        case 2:
            pThing->moveStatus = SITHPLAYERMOVE_CLIMBING_DOWN;
            sithPuppet_PlayForceMoveMode(pThing, SITHPUPPETSUBMODE_CLIMBWALLDOWN, NULL);
            break;

        case 3:
            pThing->moveStatus = SITHPLAYERMOVE_CLIMBING_RIGHT;
            sithPuppet_PlayForceMoveMode(pThing, SITHPUPPETSUBMODE_CLIMBWALLRIGHT, NULL);
            sithSoundClass_PlayModeFirst(pThing, SITHSOUNDCLASS_CLIMBHORIZ);
            break;

        case 4:
            pThing->moveStatus = SITHPLAYERMOVE_CLIMBING_LEFT;
            sithPuppet_PlayForceMoveMode(pThing, SITHPUPPETSUBMODE_CLIMBWALLLEFT, NULL);
            sithSoundClass_PlayModeFirst(pThing, SITHSOUNDCLASS_CLIMBHORIZ);
            break;

        default:
            break;
    }

    sithThing_AttachThingToClimbSurface(pThing, pSurf);
}

void J3DAPI sithPlayerActions_UnmountWall(SithThing* pThing, int trackNum)
{
    J3D_UNUSED(trackNum);

    pThing->moveStatus = SITHPLAYERMOVE_FALLING;

    pThing->moveInfo.physics.flags &= ~SITH_PF_ALIGNED;
    pThing->moveInfo.physics.flags |= SITH_PF_FLOORSTICK;
    pThing->thingInfo.actorInfo.bControlsDisabled = 0;
    pThing->thingInfo.actorInfo.bForceMovePlay    = 0;

    sithPhysics_ResetThingMovement(pThing);

    // Leftover from debug version
    rdVector3 newLVec = { -pThing->orient.lvec.x, -pThing->orient.lvec.y, -pThing->orient.lvec.z };

    sithPuppet_RemoveAllTracks(pThing);

    // Calculate dismount newVel
    rdVector3 localVelocity = { 0.0f, -0.075000003f, 0.0099999998f };
    rdVector3 newVelocity;
    rdMatrix_TransformVector34(&newVelocity, &localVelocity, &pThing->orient);
    pThing->moveInfo.physics.velocity = newVelocity;

    sithThing_DetachThing(pThing);
    sithPhysics_FindFloor(pThing, 0);

    if ( pThing->attach.flags != 0 )
    {
        pThing->moveStatus = SITHPLAYERMOVE_STILL;
        sithPuppet_PlayMode(pThing, SITHPUPPETSUBMODE_STAND, NULL);
    }

    sithPhysics_SetThingLook(pThing, &rdroid_g_zVector3, 0.0f);
    sithInventory_SetSwimmingInventory(pThing, /*bItemsAvailable=*/1);
}

void J3DAPI sithPlayerActions_ClimbOn1m(SithThing* pThing)
{
    if ( pThing->thingInfo.actorInfo.bForceMovePlay )
    {
        return;
    }

    pThing->thingInfo.actorInfo.bForceMovePlay = 1;
    pThing->moveStatus = SITHPLAYERMOVE_PULLINGUP_1M;
    pThing->moveInfo.physics.flags &= ~SITH_PF_ONWATERSURFACE; // In case player is on water surface
    pThing->collide.movesize /= 2.0f;
    pThing->forceMoveStartPos = pThing->pos;

    sithThing_DetachThing(pThing);
    sithPuppet_PlayForceMoveMode(pThing, SITHPUPPETSUBMODE_MOUNT1MSTEP, NULL);
    sithInventory_SetSwimmingInventory(pThing, /*bItemsAvailable=*/0);
    sithSoundClass_PlayModeFirst(pThing, SITHSOUNDCLASS_CLIMBONTO);
}

void J3DAPI sithPlayerActions_ClimbOn2m(SithThing* pThing)
{
    if ( pThing->thingInfo.actorInfo.bForceMovePlay )
    {
        return;
    }

    pThing->thingInfo.actorInfo.bForceMovePlay = 1;
    pThing->moveStatus        = SITHPLAYERMOVE_PULLINGUP;
    pThing->collide.movesize  = 0.01f;
    pThing->forceMoveStartPos = pThing->pos;

    sithThing_DetachThing(pThing);
    sithPuppet_PlayForceMoveMode(pThing, SITHPUPPETSUBMODE_MOUNT2MLEDGE, NULL);
    sithInventory_SetSwimmingInventory(pThing, /*bItemsAvailable=*/0);
    sithSoundClass_PlayModeFirst(pThing, SITHSOUNDCLASS_CLIMBONTO);
}

// Found in debug version, not used
void sithPlayerActions_sub_461444(void)
{}

void J3DAPI sithPlayerActions_LeapForward(SithThing* pThing)
{
    // Check if we're on a slope for leap validation
    // This was found in debug version
    bool bSlope = false;
    if ( pThing->attach.flags != 0 )
    {
        double dot = rdVector_Dot3(&pThing->attach.pFace->normal, &rdroid_g_zVector3);
        bSlope     = sithPhysics_CheckSlopeAngle(dot);
    }

    // Set leap newVel (forward and upward)
    rdVector3 newVel;
    rdVector3 localVel = { 0.0f, 0.76999998f, 0.75f };
    rdMatrix_TransformVector34(&newVel, &localVel, &pThing->orient);
    pThing->moveInfo.physics.velocity = newVel;

    // Determine and play jump sound based on surface
    if ( pThing->attach.flags != 0 )
    {
        SithSoundClassMode sndmode = SITHSOUNDCLASS_JUMP; // Default sound
        if ( pThing->attach.flags & (SITH_ATTACH_THING | SITH_ATTACH_THINGFACE) )
        {
            // Attached to thing
            SithThing* pThingAttached = pThing->attach.attachedToStructure.pThingAttached;
            if ( pThingAttached->flags & SITH_TF_METAL )
            {
                sndmode = SITHSOUNDCLASS_JUMPMETAL;
            }
            else if ( pThingAttached->flags & SITH_TF_EARTH )
            {
                sndmode = SITHSOUNDCLASS_JUMPEARTH;
            }
            else if ( pThingAttached->flags & SITH_TF_SNOW )
            {
                sndmode = SITHSOUNDCLASS_JUMPSNOW;
            }
            else if ( pThingAttached->flags & SITH_TF_WOOD )
            {
                sndmode = SITHSOUNDCLASS_JUMPWOOD;
            }
        }
        else
        {
            // Attached to surface
            SithSurface* pSurfaceAttached = pThing->attach.attachedToStructure.pSurfaceAttached;
            if ( pSurfaceAttached->flags & SITH_SURFACE_METAL )
            {
                sndmode = SITHSOUNDCLASS_JUMPMETAL;
            }
            else if ( pSurfaceAttached->flags & (SITH_SURFACE_WATER | SITH_SURFACE_SHALLOWWATER) )
            {
                sndmode = SITHSOUNDCLASS_JUMPWATER;
            }
            else if ( pSurfaceAttached->flags & SITH_SURFACE_EARTH )
            {
                sndmode = SITHSOUNDCLASS_JUMPEARTH;
            }
            else if ( pSurfaceAttached->flags & SITH_SURFACE_SNOW )
            {
                sndmode = SITHSOUNDCLASS_JUMPSNOW;
            }
            else if ( pSurfaceAttached->flags & SITH_SURFACE_WOOD )
            {
                sndmode = SITHSOUNDCLASS_JUMPWOOD;
            }
            else if ( pSurfaceAttached->flags & SITH_SURFACE_ECHO )
            {
                sndmode = SITHSOUNDCLASS_JUMPHARDECHO;
            }
            else if ( pSurfaceAttached->flags & SITH_SURFACE_WOODECHO )
            {
                sndmode = SITHSOUNDCLASS_JUMPWOODECHO;
            }
            else if ( pSurfaceAttached->flags & SITH_SURFACE_EARTHECHO )
            {
                sndmode = SITHSOUNDCLASS_JUMPEARTHECHO;
            }
            else if ( pSurfaceAttached->flags & SITH_SURFACE_AETHERIUM )
            {
                sndmode = SITHSOUNDCLASS_JUMPAET;
            }
        }

        sithSoundClass_PlayModeFirst(pThing, sndmode);
    }

    sithThing_DetachThing(pThing);
}

void J3DAPI sithPlayerActions_JumpForward(SithThing* pThing)
{
    rdVector3 newVel;
    rdVector3 localVel = { 0.0f, 0.40000001f, 0.75f };
    rdMatrix_TransformVector34(&newVel, &localVel, &pThing->orient);

    pThing->moveInfo.physics.velocity = newVel;
    pThing->collide.movesize          = 0.079999998f;

    // Determine and play jump sound based on surface
    if ( pThing->attach.flags != 0 )
    {
        SithSoundClassMode sndmode = SITHSOUNDCLASS_JUMP; // Default sound
        if ( pThing->attach.flags & (SITH_ATTACH_THING | SITH_ATTACH_THINGFACE) )
        {
            // Attached to thing
            SithThing* pThingAttached = pThing->attach.attachedToStructure.pThingAttached;
            if ( pThingAttached->flags & SITH_TF_METAL )
            {
                sndmode = SITHSOUNDCLASS_JUMPMETAL;
            }
            else if ( pThingAttached->flags & SITH_TF_EARTH )
            {
                sndmode = SITHSOUNDCLASS_JUMPEARTH;
            }
            else if ( pThingAttached->flags & SITH_TF_SNOW )
            {
                sndmode = SITHSOUNDCLASS_JUMPSNOW;
            }
            else if ( pThingAttached->flags & SITH_TF_WOOD )
            {
                sndmode = SITHSOUNDCLASS_JUMPWOOD;
            }
        }
        else
        {
            // Attached to surface
            SithSurface* pSurfaceAttached = pThing->attach.attachedToStructure.pSurfaceAttached;
            if ( pSurfaceAttached->flags & SITH_SURFACE_METAL )
            {
                sndmode = SITHSOUNDCLASS_JUMPMETAL;
            }
            else if ( pSurfaceAttached->flags & (SITH_SURFACE_WATER | SITH_SURFACE_SHALLOWWATER) )
            {
                sndmode = SITHSOUNDCLASS_JUMPWATER;
            }
            else if ( pSurfaceAttached->flags & SITH_SURFACE_EARTH )
            {
                sndmode = SITHSOUNDCLASS_JUMPEARTH;
            }
            else if ( pSurfaceAttached->flags & SITH_SURFACE_SNOW )
            {
                sndmode = SITHSOUNDCLASS_JUMPSNOW;
            }
            else if ( pSurfaceAttached->flags & SITH_SURFACE_WOOD )
            {
                sndmode = SITHSOUNDCLASS_JUMPWOOD;
            }
            else if ( pSurfaceAttached->flags & SITH_SURFACE_ECHO )
            {
                sndmode = SITHSOUNDCLASS_JUMPHARDECHO;
            }
            else if ( pSurfaceAttached->flags & SITH_SURFACE_WOODECHO )
            {
                sndmode = SITHSOUNDCLASS_JUMPWOODECHO;
            }
            else if ( pSurfaceAttached->flags & SITH_SURFACE_EARTHECHO )
            {
                sndmode = SITHSOUNDCLASS_JUMPEARTHECHO;
            }
            else if ( pSurfaceAttached->flags & SITH_SURFACE_AETHERIUM )
            {
                sndmode = SITHSOUNDCLASS_JUMPAET;
            }
        }

        sithSoundClass_PlayModeFirst(pThing, sndmode);
    }

    sithThing_DetachThing(pThing);
}

void J3DAPI sithPlayerActions_HopLeft(SithThing* pThing)
{
    rdVector3 newVel;
    rdVector3 localVel = { -0.75f, 0.0f, 0.40000001f }; // Hop left vector
    rdMatrix_TransformVector34(&newVel, &localVel, &pThing->orient);

    pThing->moveInfo.physics.velocity          = newVel;
    pThing->moveInfo.physics.angularVelocity.y = 0.0f;

    // Determine and play jump sound based on surface
    if ( pThing->attach.flags != 0 )
    {
        SithSoundClassMode sndmode = SITHSOUNDCLASS_JUMP; // Default sound
        if ( pThing->attach.flags & (SITH_ATTACH_THING | SITH_ATTACH_THINGFACE) )
        {
            // Attached to thing
            SithThing* pThingAttached = pThing->attach.attachedToStructure.pThingAttached;
            if ( pThingAttached->flags & SITH_TF_METAL )
            {
                sndmode = SITHSOUNDCLASS_JUMPMETAL;
            }
            else if ( pThingAttached->flags & SITH_TF_EARTH )
            {
                sndmode = SITHSOUNDCLASS_JUMPEARTH;
            }
            else if ( pThingAttached->flags & SITH_TF_SNOW )
            {
                sndmode = SITHSOUNDCLASS_JUMPSNOW;
            }
            else if ( pThingAttached->flags & SITH_TF_WOOD )
            {
                sndmode = SITHSOUNDCLASS_JUMPWOOD;
            }
        }
        else
        {
            // Attached to surface
            SithSurface* pSurfaceAttached = pThing->attach.attachedToStructure.pSurfaceAttached;
            if ( pSurfaceAttached->flags & SITH_SURFACE_METAL )
            {
                sndmode = SITHSOUNDCLASS_JUMPMETAL;
            }
            else if ( pSurfaceAttached->flags & (SITH_SURFACE_WATER | SITH_SURFACE_SHALLOWWATER) )
            {
                sndmode = SITHSOUNDCLASS_JUMPWATER;
            }
            else if ( pSurfaceAttached->flags & SITH_SURFACE_EARTH )
            {
                sndmode = SITHSOUNDCLASS_JUMPEARTH;
            }
            else if ( pSurfaceAttached->flags & SITH_SURFACE_SNOW )
            {
                sndmode = SITHSOUNDCLASS_JUMPSNOW;
            }
            else if ( pSurfaceAttached->flags & SITH_SURFACE_WOOD )
            {
                sndmode = SITHSOUNDCLASS_JUMPWOOD;
            }
            else if ( pSurfaceAttached->flags & SITH_SURFACE_ECHO )
            {
                sndmode = SITHSOUNDCLASS_JUMPHARDECHO;
            }
            else if ( pSurfaceAttached->flags & SITH_SURFACE_WOODECHO )
            {
                sndmode = SITHSOUNDCLASS_JUMPWOODECHO;
            }
            else if ( pSurfaceAttached->flags & SITH_SURFACE_EARTHECHO )
            {
                sndmode = SITHSOUNDCLASS_JUMPEARTHECHO;
            }
            else if ( pSurfaceAttached->flags & SITH_SURFACE_AETHERIUM )
            {
                sndmode = SITHSOUNDCLASS_JUMPAET;
            }
        }

        sithSoundClass_PlayModeFirst(pThing, sndmode);
    }

    sithThing_DetachThing(pThing);
}

void J3DAPI sithPlayerActions_HopRight(SithThing* pThing)
{
    rdVector3 newVel;
    rdVector3 localVel = { 0.75f, 0.0f, 0.40000001f }; // Hop right vector
    rdMatrix_TransformVector34(&newVel, &localVel, &pThing->orient);

    pThing->moveInfo.physics.velocity          = newVel;
    pThing->moveInfo.physics.angularVelocity.y = 0.0f;

    // Determine and play jump sound based on surface
    if ( pThing->attach.flags != 0 )
    {
        SithSoundClassMode sndmode = SITHSOUNDCLASS_JUMP; // Default sound
        if ( pThing->attach.flags & (SITH_ATTACH_THING | SITH_ATTACH_THINGFACE) )
        {
            // Attached to thing
            SithThing* pThingAttached = pThing->attach.attachedToStructure.pThingAttached;
            if ( pThingAttached->flags & SITH_TF_METAL )
            {
                sndmode = SITHSOUNDCLASS_JUMPMETAL;
            }
            else if ( pThingAttached->flags & SITH_TF_EARTH )
            {
                sndmode = SITHSOUNDCLASS_JUMPEARTH;
            }
            else if ( pThingAttached->flags & SITH_TF_SNOW )
            {
                sndmode = SITHSOUNDCLASS_JUMPSNOW;
            }
            else if ( pThingAttached->flags & SITH_TF_WOOD )
            {
                sndmode = SITHSOUNDCLASS_JUMPWOOD;
            }
        }
        else
        {
            // Attached to surface
            SithSurface* pSurfaceAttached = pThing->attach.attachedToStructure.pSurfaceAttached;
            if ( pSurfaceAttached->flags & SITH_SURFACE_METAL )
            {
                sndmode = SITHSOUNDCLASS_JUMPMETAL;
            }
            else if ( pSurfaceAttached->flags & (SITH_SURFACE_WATER | SITH_SURFACE_SHALLOWWATER) )
            {
                sndmode = SITHSOUNDCLASS_JUMPWATER;
            }
            else if ( pSurfaceAttached->flags & SITH_SURFACE_EARTH )
            {
                sndmode = SITHSOUNDCLASS_JUMPEARTH;
            }
            else if ( pSurfaceAttached->flags & SITH_SURFACE_SNOW )
            {
                sndmode = SITHSOUNDCLASS_JUMPSNOW;
            }
            else if ( pSurfaceAttached->flags & SITH_SURFACE_WOOD )
            {
                sndmode = SITHSOUNDCLASS_JUMPWOOD;
            }
            else if ( pSurfaceAttached->flags & SITH_SURFACE_ECHO )
            {
                sndmode = SITHSOUNDCLASS_JUMPHARDECHO;
            }
            else if ( pSurfaceAttached->flags & SITH_SURFACE_WOODECHO )
            {
                sndmode = SITHSOUNDCLASS_JUMPWOODECHO;
            }
            else if ( pSurfaceAttached->flags & SITH_SURFACE_EARTHECHO )
            {
                sndmode = SITHSOUNDCLASS_JUMPEARTHECHO;
            }
            else if ( pSurfaceAttached->flags & SITH_SURFACE_AETHERIUM )
            {
                sndmode = SITHSOUNDCLASS_JUMPAET;
            }
        }

        sithSoundClass_PlayModeFirst(pThing, sndmode);
    }

    sithThing_DetachThing(pThing);
}

void J3DAPI sithPlayerActions_JumpRollBack(SithThing* pThing)
{
    rdVector3 newVel;
    rdVector3 localVel = { 0.0f, -0.75f, 0.40000001f }; // Jump roll back vector
    rdMatrix_TransformVector34(&newVel, &localVel, &pThing->orient);

    pThing->moveInfo.physics.velocity          = newVel;
    pThing->moveInfo.physics.angularVelocity.y = 0.0f;

    // Determine and play jump sound based on surface
    if ( pThing->attach.flags != 0 )
    {
        SithSoundClassMode sndmode = SITHSOUNDCLASS_JUMP; // Default sound
        if ( pThing->attach.flags & (SITH_ATTACH_THING | SITH_ATTACH_THINGFACE) )
        {
            // Attached to thing
            SithThing* pThingAttached = pThing->attach.attachedToStructure.pThingAttached;
            if ( pThingAttached->flags & SITH_TF_METAL )
            {
                sndmode = SITHSOUNDCLASS_JUMPMETAL;
            }
            else if ( pThingAttached->flags & SITH_TF_EARTH )
            {
                sndmode = SITHSOUNDCLASS_JUMPEARTH;
            }
            else if ( pThingAttached->flags & SITH_TF_SNOW )
            {
                sndmode = SITHSOUNDCLASS_JUMPSNOW;
            }
            else if ( pThingAttached->flags & SITH_TF_WOOD )
            {
                sndmode = SITHSOUNDCLASS_JUMPWOOD;
            }
        }
        else
        {
            // Attached to surface
            SithSurface* pSurfaceAttached = pThing->attach.attachedToStructure.pSurfaceAttached;
            if ( pSurfaceAttached->flags & SITH_SURFACE_METAL )
            {
                sndmode = SITHSOUNDCLASS_JUMPMETAL;
            }
            else if ( pSurfaceAttached->flags & (SITH_SURFACE_WATER | SITH_SURFACE_SHALLOWWATER) )
            {
                sndmode = SITHSOUNDCLASS_JUMPWATER;
            }
            else if ( pSurfaceAttached->flags & SITH_SURFACE_EARTH )
            {
                sndmode = SITHSOUNDCLASS_JUMPEARTH;
            }
            else if ( pSurfaceAttached->flags & SITH_SURFACE_SNOW )
            {
                sndmode = SITHSOUNDCLASS_JUMPSNOW;
            }
            else if ( pSurfaceAttached->flags & SITH_SURFACE_WOOD )
            {
                sndmode = SITHSOUNDCLASS_JUMPWOOD;
            }
            else if ( pSurfaceAttached->flags & SITH_SURFACE_ECHO )
            {
                sndmode = SITHSOUNDCLASS_JUMPHARDECHO;
            }
            else if ( pSurfaceAttached->flags & SITH_SURFACE_WOODECHO )
            {
                sndmode = SITHSOUNDCLASS_JUMPWOODECHO;
            }
            else if ( pSurfaceAttached->flags & SITH_SURFACE_EARTHECHO )
            {
                sndmode = SITHSOUNDCLASS_JUMPEARTHECHO;
            }
            else if ( pSurfaceAttached->flags & SITH_SURFACE_AETHERIUM )
            {
                sndmode = SITHSOUNDCLASS_JUMPAET;
            }
        }

        sithSoundClass_PlayModeFirst(pThing, sndmode);
    }

    sithThing_DetachThing(pThing);
}

void J3DAPI sithPlayerActions_JumpRollForward(SithThing* pThing)
{
    rdVector3 newVel;
    rdVector3 localVel = { 0.0f, 0.75f, 0.40000001f }; // Jump forward back vector
    rdMatrix_TransformVector34(&newVel, &localVel, &pThing->orient);

    pThing->moveInfo.physics.velocity          = newVel;
    pThing->moveInfo.physics.angularVelocity.y = 0.0f;

    // Determine and play jump sound based on surface
    if ( pThing->attach.flags != 0 )
    {
        SithSoundClassMode sndmode = SITHSOUNDCLASS_JUMP; // Default sound
        if ( pThing->attach.flags & (SITH_ATTACH_THING | SITH_ATTACH_THINGFACE) )
        {
            // Attached to thing
            SithThing* pThingAttached = pThing->attach.attachedToStructure.pThingAttached;
            if ( pThingAttached->flags & SITH_TF_METAL )
            {
                sndmode = SITHSOUNDCLASS_JUMPMETAL;
            }
            else if ( pThingAttached->flags & SITH_TF_EARTH )
            {
                sndmode = SITHSOUNDCLASS_JUMPEARTH;
            }
            else if ( pThingAttached->flags & SITH_TF_SNOW )
            {
                sndmode = SITHSOUNDCLASS_JUMPSNOW;
            }
            else if ( pThingAttached->flags & SITH_TF_WOOD )
            {
                sndmode = SITHSOUNDCLASS_JUMPWOOD;
            }
        }
        else
        {
            // Attached to surface
            SithSurface* pSurfaceAttached = pThing->attach.attachedToStructure.pSurfaceAttached;
            if ( pSurfaceAttached->flags & SITH_SURFACE_METAL )
            {
                sndmode = SITHSOUNDCLASS_JUMPMETAL;
            }
            else if ( pSurfaceAttached->flags & (SITH_SURFACE_WATER | SITH_SURFACE_SHALLOWWATER) )
            {
                sndmode = SITHSOUNDCLASS_JUMPWATER;
            }
            else if ( pSurfaceAttached->flags & SITH_SURFACE_EARTH )
            {
                sndmode = SITHSOUNDCLASS_JUMPEARTH;
            }
            else if ( pSurfaceAttached->flags & SITH_SURFACE_SNOW )
            {
                sndmode = SITHSOUNDCLASS_JUMPSNOW;
            }
            else if ( pSurfaceAttached->flags & SITH_SURFACE_WOOD )
            {
                sndmode = SITHSOUNDCLASS_JUMPWOOD;
            }
            else if ( pSurfaceAttached->flags & SITH_SURFACE_ECHO )
            {
                sndmode = SITHSOUNDCLASS_JUMPHARDECHO;
            }
            else if ( pSurfaceAttached->flags & SITH_SURFACE_WOODECHO )
            {
                sndmode = SITHSOUNDCLASS_JUMPWOODECHO;
            }
            else if ( pSurfaceAttached->flags & SITH_SURFACE_EARTHECHO )
            {
                sndmode = SITHSOUNDCLASS_JUMPEARTHECHO;
            }
            else if ( pSurfaceAttached->flags & SITH_SURFACE_AETHERIUM )
            {
                sndmode = SITHSOUNDCLASS_JUMPAET;
            }
        }

        sithSoundClass_PlayModeFirst(pThing, sndmode);
    }

    sithThing_DetachThing(pThing);
}

void J3DAPI sithPlayerActions_StrafeLeft(SithThing* pThing)
{
    if ( !pThing->thingInfo.actorInfo.bForceMovePlay )
    {
        rdVector3 pos;
        rdVector_Neg3(&pos, &pThing->orient.rvec);
        rdVector_ScaleAdd3Acc(&pos, 0.1f, &pThing->pos); // Strafe left vector

        int bSurfChange;
        if ( sithPlayerActions_CheckFloorAtPos(pThing, &pos, &bSurfChange) == 1 )
        {
            pThing->thingInfo.actorInfo.bForceMovePlay = 1;
            pThing->moveStatus        = SITHPLAYERMOVE_STRAFING_LEFT;
            pThing->forceMoveStartPos = pThing->pos;
            sithPuppet_PlayForceMoveMode(pThing, SITHPUPPETSUBMODE_STRAFELEFT, NULL);
        }
    }
}

void J3DAPI sithPlayerActions_StrafeRight(SithThing* pThing)
{
    if ( !pThing->thingInfo.actorInfo.bForceMovePlay )
    {
        rdVector3 pos = pThing->orient.rvec;
        rdVector_ScaleAdd3Acc(&pos, 0.1f, &pThing->pos); // Strafe right vector

        int bSurfChange;
        if ( sithPlayerActions_CheckFloorAtPos(pThing, &pos, &bSurfChange) == 1 )
        {
            pThing->thingInfo.actorInfo.bForceMovePlay = 1;
            pThing->moveStatus        = SITHPLAYERMOVE_STRAFING_RIGHT;
            pThing->forceMoveStartPos = pThing->pos;
            sithPuppet_PlayForceMoveMode(pThing, SITHPUPPETSUBMODE_STRAFERIGHT, NULL);
        }
    }
}

void J3DAPI sithPlayerActions_HopBack(SithThing* pThing)
{
    rdVector3 newVel;
    rdVector3 localVel = { 0.0f, -0.34999999f, 0.60000002f };
    rdMatrix_TransformVector34(&newVel, &localVel, &pThing->orient);
    pThing->moveInfo.physics.velocity = newVel;

    // Determine and play jump sound based on surface
    if ( pThing->attach.flags != 0 )
    {
        SithSoundClassMode sndmode = SITHSOUNDCLASS_JUMP; // Default sound
        if ( pThing->attach.flags & (SITH_ATTACH_THING | SITH_ATTACH_THINGFACE) )
        {
            // Attached to thing
            SithThing* pThingAttached = pThing->attach.attachedToStructure.pThingAttached;
            if ( pThingAttached->flags & SITH_TF_METAL )
            {
                sndmode = SITHSOUNDCLASS_JUMPMETAL;
            }
            else if ( pThingAttached->flags & SITH_TF_EARTH )
            {
                sndmode = SITHSOUNDCLASS_JUMPEARTH;
            }
            else if ( pThingAttached->flags & SITH_TF_SNOW )
            {
                sndmode = SITHSOUNDCLASS_JUMPSNOW;
            }
            else if ( pThingAttached->flags & SITH_TF_WOOD )
            {
                sndmode = SITHSOUNDCLASS_JUMPWOOD;
            }
        }
        else
        {
            // Attached to surface
            SithSurface* pSurfaceAttached = pThing->attach.attachedToStructure.pSurfaceAttached;
            if ( pSurfaceAttached->flags & SITH_SURFACE_METAL )
            {
                sndmode = SITHSOUNDCLASS_JUMPMETAL;
            }
            else if ( pSurfaceAttached->flags & (SITH_SURFACE_WATER | SITH_SURFACE_SHALLOWWATER) )
            {
                sndmode = SITHSOUNDCLASS_JUMPWATER;
            }
            else if ( pSurfaceAttached->flags & SITH_SURFACE_EARTH )
            {
                sndmode = SITHSOUNDCLASS_JUMPEARTH;
            }
            else if ( pSurfaceAttached->flags & SITH_SURFACE_SNOW )
            {
                sndmode = SITHSOUNDCLASS_JUMPSNOW;
            }
            else if ( pSurfaceAttached->flags & SITH_SURFACE_WOOD )
            {
                sndmode = SITHSOUNDCLASS_JUMPWOOD;
            }
            else if ( pSurfaceAttached->flags & SITH_SURFACE_ECHO )
            {
                sndmode = SITHSOUNDCLASS_JUMPHARDECHO;
            }
            else if ( pSurfaceAttached->flags & SITH_SURFACE_WOODECHO )
            {
                sndmode = SITHSOUNDCLASS_JUMPWOODECHO;
            }
            else if ( pSurfaceAttached->flags & SITH_SURFACE_EARTHECHO )
            {
                sndmode = SITHSOUNDCLASS_JUMPEARTHECHO;
            }
            else if ( pSurfaceAttached->flags & SITH_SURFACE_AETHERIUM )
            {
                sndmode = SITHSOUNDCLASS_JUMPAET;
            }
        }

        sithSoundClass_PlayModeFirst(pThing, sndmode);
    }

    sithThing_DetachThing(pThing);
}

void J3DAPI sithPlayerActions_Stand2Crawl(SithThing* pThing)
{
    sithInventory_SetSwimmingInventory(pThing, /*bItemsAvailable=*/0);

    pThing->collide.movesize  = 0.02f; // crawl move size
    pThing->forceMoveStartPos = pThing->pos;
    pThing->moveStatus        = SITHPLAYERMOVE_STAND_TO_CRAWL;

    sithPuppet_SetMoveMode(pThing, SITHPUPPET_MOVEMODE_CRAWL);
    sithPuppet_PlayForceMoveMode(pThing, SITHPUPPETSUBMODE_STAND2CRAWL, NULL);
    sithSoundClass_PlayModeFirst(pThing, SITHSOUNDCLASS_CLIMBDOWNONTO);
}

void J3DAPI sithPlayerActions_Crawl2Stand(SithThing* pThing)
{
    rdVector3 moveNorm = rdroid_g_zVector3;
    sithCollision_SearchForCollisions(pThing->pInSector, pThing, &pThing->pos, &moveNorm, 0.115f, pThing->collide.movesize, 0x200);

    bool bCanMove = true;
    SithCollision* pCollision;
    while ( (pCollision = sithCollision_PopStack()) != NULL )
    {
        if ( (pCollision->type & SITHCOLLISION_WORLD) != 0 )
        {
            bCanMove = false;
            break;
        }

        if ( (pCollision->type & SITHCOLLISION_THING) != 0 )
        {
            bCanMove = false;
            break;
        }
    }

    sithCollision_DecreaseStackLevel();

    if ( bCanMove )
    {
        sithInventory_SetSwimmingInventory(pThing, /*bItemsAvailable=*/0); // Fixed: Disable inventory items when crawling to stand. Re-enabled in sithPuppet_FinishForceMove
        sithPuppet_ClearMode(pThing, SITHPUPPETSUBMODE_WALK);
        sithPuppet_ClearMode(pThing, SITHPUPPETSUBMODE_WALKBACK);

        pThing->collide.movesize  = 0.039999999f; // default move size
        pThing->forceMoveStartPos = pThing->pos;
        pThing->moveStatus        = SITHPLAYERMOVE_CRAWL_TO_STAND;

        sithPuppet_PlayForceMoveMode(pThing, SITHPUPPETSUBMODE_CRAWL2STAND, NULL);
        sithSoundClass_PlayModeFirst(pThing, SITHSOUNDCLASS_CLIMBONTO);
    }
}

void J3DAPI sithPlayerActions_JumpStart(SithThing* pThing)
{
    pThing->moveStatus = SITHPLAYERMOVE_JUMP_READY;
    sithPhysics_ResetThingMovement(pThing);
    sithPuppet_PlayMode(pThing, SITHPUPPETSUBMODE_JUMPREADY, sithPlayerControls_PuppetCallback);

    sithPuppet_g_bPlayerLeapForward = 0;
    pThing->thingInfo.actorInfo.bControlsDisabled = 1;
}

int J3DAPI sithPlayerActions_HasActiveWeapon(SithThing* pThing)
{
    if ( sithInventory_GetCurrentWeapon(pThing) )
    {
        return 1;
    }
    else
    {
        return sithWeapon_IsMountingWeapon(pThing);
    }
}

void J3DAPI sithPlayerActions_ClimbDownToClimb(SithThing* pThing, int bAngled)
{
    if ( !pThing->thingInfo.actorInfo.bForceMovePlay )
    {
        pThing->thingInfo.actorInfo.bForceMovePlay = 1;
        pThing->moveStatus        = SITHPLAYERMOVE_CLIMB_DOWN_TO_MOUNT;
        pThing->collide.movesize  = 0.001f;
        pThing->forceMoveStartPos = pThing->pos;

        sithThing_DetachThing(pThing);

        if ( bAngled )
        {
            sithPuppet_PlayForceMoveMode(pThing, SITHPUPPETSUBMODE_GRABLEDGE, NULL);// Climb down onto angled climb wall
        }
        else
        {
            sithPuppet_PlayForceMoveMode(pThing, SITHPUPPETSUBMODE_CLIMBTOCLIMB, NULL);
        }

        sithSoundClass_PlayModeFirst(pThing, SITHSOUNDCLASS_CLIMBDOWNONTO);
        sithInventory_SetSwimmingInventory(pThing, /*bItemsAvailable=*/0);
        sithSoundClass_PlayModeFirst(pThing, SITHSOUNDCLASS_CLIMBDOWNONTO);
    }
}

void J3DAPI sithPlayerActions_ClimbDownToHang(SithThing* pThing)
{
    if ( !pThing->thingInfo.actorInfo.bForceMovePlay )
    {
        pThing->thingInfo.actorInfo.bForceMovePlay = 1;
        pThing->moveStatus        = SITHPLAYERMOVE_CLIMB_TO_HANG;
        pThing->collide.movesize  = 0.001f;
        pThing->forceMoveStartPos = pThing->pos;

        sithThing_DetachThing(pThing);
        sithPuppet_PlayForceMoveMode(pThing, SITHPUPPETSUBMODE_CLIMBTOHANG, NULL);
        sithInventory_SetSwimmingInventory(pThing, /*bItemsAvailable=*/0);
        sithSoundClass_PlayModeFirst(pThing, SITHSOUNDCLASS_CLIMBDOWNONTO);
    }
}

int J3DAPI sithPlayerActions_CheckFloorAtPos(SithThing* pThing, rdVector3* pPos, int* pbSurfaceChange)
{
    SITH_ASSERTREL(pThing);

    SithSector* pStartSector = sithCollision_FindSectorInRadius(pThing->pInSector, &pThing->pos, pPos, 0.0f);
    if ( !pStartSector )
    {
        return 2;
    }

    rdVector3 moveNorm = { 0.0f, 0.0f, -1.0f }; // Down move
    float moveDist     = sithPhysics_GetThingHeight(pThing) + 0.115f; // Adding 0.115 allows stepping down from small inclined surfaces like large stairs
    float radius       = pThing->collide.movesize / 2.0f; // TODO: The radius might be too big in some cases and facing wall surface could be detected instead of ground/down floor surface
    sithCollision_SearchForCollisions(pStartSector, pThing, pPos, &moveNorm, moveDist, radius, 0xA00);

    int result = 0;
    SithCollision* pCollision;
    while ( (pCollision = sithCollision_PopStack()) != NULL )
    {
        if ( (pCollision->type & SITHCOLLISION_WORLD) != 0 )
        {
            SITH_ASSERTREL(pCollision->pSurfaceCollided);

            if ( (pCollision->pSurfaceCollided->flags & (SITH_SURFACE_ISFLOOR | SITH_SURFACE_LAVA)) != 0 )
            {
                // Handle water surfaces for specific movement states
                if ( (pThing->moveStatus == SITHPLAYERMOVE_UNKNOWN_4 || pThing->moveStatus == SITHPLAYERMOVE_UNKNOWN_5)
                    && (pCollision->pSurfaceCollided->flags & (SITH_SURFACE_SHALLOWWATER | SITH_SURFACE_WATER)) != 0 )
                {
                    result = 0;
                    break;
                }

                // Can't move onto lava surface
                if ( (pCollision->pSurfaceCollided->flags & SITH_SURFACE_LAVA) != 0 )
                {
                    result = 0;
                    break;
                }

                // Check if surface is not slope (slideable slope  36.87 - 46.49 degrees)
                float surfaceDot = rdVector_Dot3(&pCollision->pSurfaceCollided->face.normal, &rdroid_g_zVector3);
                if ( !sithPhysics_CheckSlopeAngle(surfaceDot) ) // surf angle <= 36.87 or angle >= 46.49
                {
                    result = (surfaceDot >= 0.69f) ? 1 : 0; // if surf is not too angled floor (slope) return 1 else 0 - too steep
                }
                else
                {
                    // Surface is a slope

                    // Walking or any other movement is invalid on slope surface
                    result = 0;

                    if ( (pThing->thingInfo.actorInfo.flags & SITH_AF_CONTROLSDISABLED) != 0 )
                    {
                        result = 1;
                    }
                    else if ( pThing->moveStatus == SITHPLAYERMOVE_RUNNING )
                    {
                        // Check if running into ascending slope surface
                        // Note, that this means that only for ascending slope 3 is returned.
                        // For descending slope 0 is returned since player would face away from surface (same direction as surface normal).
                        rdVector3 surfaceNormal = pCollision->pSurfaceCollided->face.normal;
                        surfaceNormal.z = 0.0f;
                        rdVector_Normalize3Acc(&surfaceNormal);

                        rdVector3 moveDirection = pThing->orient.lvec;
                        moveDirection.z = 0.0f;
                        rdVector_Normalize3Acc(&moveDirection);

                        float moveDot = rdVector_Dot3(&moveDirection, &surfaceNormal);
                        result = (moveDot <= 0.0f) ? 3 : 0; // dot == 0.0 - running perpendicular to the surface normal (sideways)
                                                            // dot < 0.0  - running into the surface (e.g., head-on into a wall)
                                                            // dot > 0.0  - running away from the surface (e.g. descending slope)
                    }
                }
            }
            else
            {
                // Non-floor surface, i.e. floor surface with no floor flag set.

                // Check if flat or small descending/ascending slope surface
                float surfaceDot = rdVector_Dot3(&pCollision->pSurfaceCollided->face.normal, &rdroid_g_zVector3);
                if ( surfaceDot >= 0.63f ) // ~50.80 degrees
                {
                    result = 2;
                }
                else if ( pThing->moveStatus != SITHPLAYERMOVE_RUNNING )
                {
                    result = 0;
                }
                else
                {
                    // Surface is a wall, cliff
                    // check if we're running into the surface or away (e.g.: standing on a top of a cliff returns 0, but walking into the cliff returns 3
                    rdVector3 surfaceNormal = pCollision->pSurfaceCollided->face.normal;
                    surfaceNormal.z = 0.0f;
                    rdVector_Normalize3Acc(&surfaceNormal);

                    rdVector3 moveDirection = pThing->orient.lvec;
                    moveDirection.z = 0.0f;
                    rdVector_Normalize3Acc(&moveDirection);

                    float moveDot = rdVector_Dot3(&moveDirection, &surfaceNormal);
                    result = (moveDot <= 0.0f) ? 3 : 0; // dot == 0.0 - running perpendicular to the surface normal (sideways)
                                                        // dot < 0.0  - running into the surface (e.g., head-on into a wall)
                                                        // dot > 0.0  - running away from the surface (forward movement)
                }
            }

            // Set surface changed flag if provided
            if ( pbSurfaceChange )
            {
                *pbSurfaceChange = ((pThing->attach.flags & SITH_ATTACH_SURFACE) == 0
                    || pThing->attach.attachedToStructure.pSurfaceAttached != pCollision->pSurfaceCollided);
            }

            break; // Jump out of loop after processing world surface
        } // pCollision->type & SITHCOLLISION_WORLD

        // Check if collision was with thing
        if ( (pCollision->type & SITHCOLLISION_THING) != 0 )
        {
            SITH_ASSERTREL(pCollision->pThingCollided);

            if ( (pCollision->pThingCollided->flags & SITH_TF_STANDON) != 0
                || pCollision->pThingCollided->type == SITH_THING_ITEM )
            {
                // TODO: To improve engine maybe test for slope should be performed
                result = 1;
                if ( pbSurfaceChange )
                {
                    *pbSurfaceChange = ((pThing->attach.flags & SITH_ATTACH_THINGFACE) == 0 ||
                        pThing->attach.attachedToStructure.pThingAttached != pCollision->pThingCollided);
                }
            }
            else
            {
                result = 0;
            }
            break;
        }

        // Check if collision was with non-walkable lava adjoin surface
        // TODO: Due to calling sithCollision_SearchForCollisions with 0x200 flag adjoin with SITH_ADJOIN_NOPLAYERMOVE flag will never be collected.
        //       This could probably be safely removed.
        if ( (pCollision->type & SITHCOLLISION_ADJOINTOUCH) != 0
            && pCollision->pSurfaceCollided
            && (pCollision->pSurfaceCollided->pAdjoin->flags & SITH_ADJOIN_NOPLAYERMOVE) != 0
            && (pCollision->pSurfaceCollided->flags & SITH_SURFACE_LAVA) != 0 )
        {
            if ( pbSurfaceChange )
            {
                *pbSurfaceChange = 0;
            }
            result = 0;
            break;
        }
    }

    sithCollision_DecreaseStackLevel();
    return result;
}

// Found in debug version
int sithPlayerActions_sub_462E59(void)
{
    return 0;
}

float J3DAPI sithPlayerActions_FindLedge(SithThing* pThing, const rdVector3* pPYR, SithSurface** ppLedgeSurf, rdFace** ppLedgeThingFace, rdModel3Mesh** ppLedgeThingMesh, SithThing** ppLedgeThing)
{
    J3D_UNUSED(pPYR);
    if ( sithPlayerActions_HasActiveWeapon(pThing) )
    {
        return -1.0f;
    }

    // Try to find ledge surface in player's direction but at position 0.9m higher
    // than current player position, i.e. approximate position of up outstretched hands
    rdVector3 startPos = pThing->pos;
    startPos.z += 0.090000004f;

    SithSector* pStartSector = sithCollision_FindSectorInRadius(pThing->pInSector, &pThing->pos, &startPos, 0.0f);
    if ( !pStartSector )
    {
        return -1.0f;
    }

    float radius = 0.0049999999f;
#ifdef J3D_DEBUG
    if ( pThing->moveStatus < (unsigned int)SITHPLAYERMOVE_CLIMB_TO_HANG ) // Leftover in debug
    {
        radius = 0.0049999999f;
    }
#endif

    sithCollision_SearchForCollisions(pStartSector, pThing, &startPos, &pThing->orient.lvec, 0.090000004f, 0.0049999999f, 0xA00);

    bool bSurfaceHit           = false;
    SithSurface* pHitLedgeSurf = NULL;

    bool bFaceHit         = false;
    rdFace* pHitThingFace = NULL;

    float hitDistance = 0.0f;

    SithCollision* pCollision;
    while ( (pCollision = sithCollision_PopStack()) != NULL )
    {
        if ( (pCollision->type & SITHCOLLISION_WORLD) != 0 )
        {
            if ( pCollision->pSurfaceCollided && (pCollision->pSurfaceCollided->flags & SITH_SURFACE_LEDGE) != 0 )
            {
                // Check if player is oriented almost towards the ledge surface
                rdVector3 hitNormal = RDVECTOR_NEG3(pCollision->pSurfaceCollided->face.normal); // Negate normal to get the hit surface normal
                if ( rdVector_Dot3(&pThing->orient.lvec, &hitNormal) >= 0.949f )
                {
                    if ( sithPlayerActions_g_pCurLedgeSurface != pCollision->pSurfaceCollided ) // TODO: This check should probably be done before the dot product check
                    {
                        sithPlayerActions_g_pCurLedgeSurface = pCollision->pSurfaceCollided;

                        pHitLedgeSurf = sithPlayerActions_g_pCurLedgeSurface;
                        bSurfaceHit   = true;
                        hitDistance   = pCollision->distance;
                        if ( hitDistance < 0.0f )
                        {
                            pHitLedgeSurf = NULL;
                            bSurfaceHit = false;
                        }
                    }
                    break;
                }
            }
        }
        else if ( (pCollision->type & SITHCOLLISION_THING) != 0 )
        {
            if ( pCollision->pFaceCollided
                && pCollision->pFaceCollided != sithPlayerActions_g_pCurLedgeThingModelFace
                && (pCollision->pFaceCollided->flags & RD_FF_3DO_LEDGE) != 0 )
            {
                sithPlayerActions_g_pCurLedgeThingModelFace = pCollision->pFaceCollided;
                pHitThingFace = sithPlayerActions_g_pCurLedgeThingModelFace;

                // Convert face normal to world space
                rdVector3 hitNormal;
                rdMatrix_TransformVector34(&hitNormal, &pCollision->pFaceCollided->normal, &pCollision->pThingCollided->orient);

                // Check if player is oriented almost towards the ledge surface
                rdVector3 negPlayerDir = RDVECTOR_NEG3(pThing->orient.lvec);
                if ( rdVector_Dot3(&negPlayerDir, &hitNormal) > 0.949f )
                {
                    bFaceHit    = true;
                    hitDistance = pCollision->distance;
                    if ( hitDistance < 0.0f )
                    {
                        hitDistance = 0.0f;
                    }

                    *ppLedgeThingMesh = pCollision->pMeshCollided;
                    *ppLedgeThing     = pCollision->pThingCollided;

                    sithPlayerActions_g_pCurLedgeThingModel     = pCollision->pThingCollided->renderData.data.pModel3;
                    sithPlayerActions_g_pCurLedgeThingModelFace = pCollision->pFaceCollided;
                    break;
                }
            }
        }
    }

    sithCollision_DecreaseStackLevel();

    if ( bSurfaceHit || bFaceHit )
    {
        // Now search if there is any solid surface or standon thing object 0.4m beneath current thing position
        // that thing could land on
        float curMovesize = pThing->collide.movesize;
        pThing->collide.movesize = 0.02f;

        sithPhysics_ResetThingMovement(pThing);

        // Fixed: Shift start position Z to ledge grab point Z position (height of ledge).
        //        Originally it was just pThing->pos, and the search for collision was done 0.13 + radius (i.e. 0.09-insert position + 0.04) down from that position.
        //        This caused the ledge grab to fail when the ledge start position was at 0.2 (2m) from the ground, but the ledge grab point was higher than 0.2.
        startPos = pThing->pos;
        startPos.z = bSurfaceHit
            ? sithPlayerActions_GetLedgeSurfaceGrabPosZ(pHitLedgeSurf)
            : (sithPlayerActions_GetLedgeThingGrabPosZ(*ppLedgeThing, pHitThingFace, *ppLedgeThingMesh)
                + (*ppLedgeThing)->pos.z); // shift returned Z in world space to ledge pos

        // Added: Find sector of new startPos
        pStartSector = sithCollision_FindSectorInRadius(pThing->pInSector, &pThing->pos, &startPos, 0.0f);
        if ( !pStartSector )
        {
            pThing->collide.movesize = curMovesize;
            return -1.0f;
        }

        rdVector3 moveNorm = RDVECTOR_NEG3(rdroid_g_zVector3); // Down direction
        sithCollision_SearchForCollisions(pStartSector, pThing, &startPos, &moveNorm, 0.21f, pThing->collide.movesize, 0xA00); // Total down move dist of sphere's center is 2.1m.
                                                                                                                               // Combined with 0.2m radius, the total distance is 2.3m.
                                                                                                                               // i.e.: indy should be at least 10 cm (OG was 40 cm) higher from the ground
                                                                                                                               //       when stretched out in hang position (2.2m).

        bool bSolidSurfFound = false;
        while ( (pCollision = sithCollision_PopStack()) != NULL )
        {
            if ( (pCollision->type & SITHCOLLISION_WORLD) != 0 )
            {
                if ( (pCollision->pSurfaceCollided->flags & SITH_SURFACE_LAVA) == 0 )
                {
                    bSolidSurfFound = true;
                    break;
                }
            }
            else if ( (pCollision->type & SITHCOLLISION_ADJOINTOUCH) != 0 )
            {
                if ( (pCollision->pSurfaceCollided->pAdjoin->flags & SITH_ADJOIN_NOPLAYERMOVE) != 0
                    && (pCollision->pSurfaceCollided->flags & SITH_SURFACE_LAVA) == 0 )
                {
                    bSolidSurfFound = true;
                    break;
                }
            }
            else if ( (pCollision->type & SITHCOLLISION_THING) != 0
                && (pCollision->pThingCollided->flags & SITH_TF_STANDON) != 0
                && pCollision->pThingCollided != *ppLedgeThing )
            {
                bSolidSurfFound = true;
                break;
            }
        }

        sithCollision_DecreaseStackLevel();

        // Restore original move size
        pThing->collide.movesize = curMovesize;

        if ( bSolidSurfFound )
        {
            return -1.0f;
        }
    }

    if ( bSurfaceHit )
    {
        *ppLedgeSurf = pHitLedgeSurf;
        return hitDistance;
    }
    else if ( bFaceHit )
    {
        *ppLedgeThingFace = pHitThingFace;
        return hitDistance;
    }
    else
    {
        return -1.0f;
    }
}

int J3DAPI sithPlayerActions_CanClimbOn1m(SithThing* pThing)
{
    // 1.) Find vertical surface at curPos - thingHeight + 0.97m in radius of players direction at move distance of player move size + 0.1m
    //
    //                      -----------------
    //                      |
    // start pos         -. |
    //    X--------------> )|
    //    | ms + 0.1m    -' |
    //    |                 |
    //    o                 |
    // p.pos                |
    //----------------------|

    float thingHeight  = sithPhysics_GetThingHeight(pThing);
    rdVector3 startPos = pThing->pos;
    startPos.z = (startPos.z - thingHeight) + 0.097999997f;

    SithSector* pStartSec = sithCollision_FindSectorInRadius(pThing->pInSector, &pThing->pos, &startPos, 0.0f);
    if ( !pStartSec )
    {
        return 0;
    }

    rdVector3 moveNorm = pThing->orient.lvec;
    float radius       = 0.0099999998f;
    float moveDist     = pThing->collide.movesize + 0.0099999998f;
    sithCollision_SearchForCollisions(pStartSec, pThing, &startPos, &moveNorm, moveDist, radius, 0xA00);

    bool bFoundVerticalSurf = false;
    SithCollision* pCollision;
    while ( (pCollision = sithCollision_PopStack()) != NULL )
    {
        if ( (pCollision->type & SITHCOLLISION_WORLD) != 0 )
        {
            if ( pCollision->pSurfaceCollided )
            {
                // Check if surface is vertical (face.normal dot rdroid_g_zVector)
                float surfDot = rdVector_Dot3(&pCollision->pSurfaceCollided->face.normal, &rdroid_g_zVector3);
                if ( surfDot < 0.1f && surfDot > -0.1f ) // almost vertical surface between 84 and 96 degrees
                {
                    bFoundVerticalSurf = true;
                    break;
                }
            }
        }
        else if ( (pCollision->type & SITHCOLLISION_THING) != 0 && pCollision->pFaceCollided )
        {
            if ( ((pCollision->pThingCollided->flags & SITH_TF_STANDON) != 0
                && ((pCollision->pThingCollided->flags & SITH_TF_MOUNTABLE) != 0)
                || (pCollision->pThingCollided->flags & SITH_TF_UNKNOWN_1000) != 0) )
            {
                if ( pCollision->pFaceCollided->normal.z == 0.0f ) // TODO: Maybe also do a dot product check here?
                {
                    // TODO: This hardcoded check for thing names should be replaced with a more generic check
                    bFoundVerticalSurf =
                        !streq(pCollision->pThingCollided->aName, "20mropebridge")
                        && !streq(pCollision->pThingCollided->aName, "10mbrokebridge")
                        && !streq(pCollision->pThingCollided->aName, "20mholesbridge")
                        && !streq(pCollision->pThingCollided->aName, "20mholebridge")
                        && !streq(pCollision->pThingCollided->aName, "25mholebridge");
                    break;
                }
            }
        }
    }

    sithCollision_DecreaseStackLevel();

    if ( !bFoundVerticalSurf )
    {
        return 0;
    }

    // 2.) Now check there is no obstacle at the top of the vertical surface to prevent climbing
    // i.e.: there should be no collision 1.15m up from current startPos
    //
    // start pos         -.
    //    X--------------> )
    //    |    0.7m      -'
    //    |            -----------------
    //    |            |
    //    |            |
    //    o            |
    // prev start pos  |
    //                 |
    //                 |
    //                 |
    //-----------------|

    startPos.z += 0.115f;
    pStartSec = sithCollision_FindSectorInRadius(pThing->pInSector, &pThing->pos, &startPos, 0.0f);
    if ( !pStartSec )
    {
        return 0;
    }

    radius   = 0.045000002f;
    moveDist = 0.071000002f;
    sithCollision_SearchForCollisions(pStartSec, pThing, &startPos, &moveNorm, moveDist, radius, 0xA00);

    bool bCanClimbOn = true;
    while ( (pCollision = sithCollision_PopStack()) != NULL )
    {
        if ( (pCollision->type & SITHCOLLISION_WORLD) != 0 )
        {
            if ( pCollision->pSurfaceCollided )
            {
                bCanClimbOn = false;
                break;
            }
        }
        else if ( (pCollision->type & SITHCOLLISION_THING) != 0 && pCollision->pFaceCollided )
        {
            bCanClimbOn = false;
            break;
        }
    }

    sithCollision_DecreaseStackLevel();
    return bFoundVerticalSurf && bCanClimbOn;
}

int J3DAPI sithPlayerActions_CanClimbOn2m(SithThing* pThing)
{
    // 1.) Find ledge surface or claimable crate at curPos - thingHeight + 1.5m in radius of players direction at move distance of player move size + 0.1m
    //
    //                      -----------------
    //                      |
    // start pos         -. |
    //    X--------------> )|
    //    | ms + 0.1m    -' |
    //    |                 |
    //    o                 |
    // player pos           |
    //----------------------|

    float thingHeight  = sithPhysics_GetThingHeight(pThing); // height is thing insert offset
    rdVector3 startPos = pThing->pos;
    startPos.z = startPos.z - thingHeight + 0.15000001f; // TODO: Should z be almost at thing height (e.g. 2 * thingHeight)?

    SithSector* pFoundSec = sithCollision_FindSectorInRadius(pThing->pInSector, &pThing->pos, &startPos, 0.0f);
    if ( !pFoundSec )
    {
        return 0;
    }

    rdVector3 moveNorm = pThing->orient.lvec;
    float radius       = 0.02f;
    float moveDist     = pThing->collide.movesize + 0.0099999998f;
    sithCollision_SearchForCollisions(pFoundSec, pThing, &startPos, &moveNorm, moveDist, radius, 0xA00);

    bool bFoundClimb   = false;
    SithCollision* pCollision;
    while ( (pCollision = sithCollision_PopStack()) != NULL )
    {
        if ( (pCollision->type & SITHCOLLISION_WORLD) != 0 )
        {
            if ( pCollision->pSurfaceCollided
                && (pCollision->pSurfaceCollided->flags & SITH_SURFACE_LEDGE) != 0 // TODO: The ledge check could possible be removed, since there is no need for this optimization anymore (Do check if there are any level cases where climb on 2m should be prevented)
                && pCollision->distance < 0.039999999f ) // 0.04 is default player collision size
            {
                bFoundClimb = true;
                break;
            }
        }
        // TODO: Check also for 3DO Model's face flag RD_FF_3DO_LEDGE
        else if ( (pCollision->type & SITHCOLLISION_THING) != 0
            && pCollision->pThingCollided
            && (pCollision->pThingCollided->flags & SITH_TF_CLIMBCRATE) != 0
            && pCollision->distance < 0.039999999f )
        {
            bFoundClimb = true;
            break;
        }
    }

    sithCollision_DecreaseStackLevel();

    bool bFoundObstacle = false;
    if ( bFoundClimb )
    {
        // 2.) Now check there is no obstacle at the top of the ledge surface or crate thing object to prevent climbing
        // i.e.: there should be no collision 1.15m up from current startPos
        //
        // start pos         -.
        //    X--------------> )
        //    | ms + 0.5m    -'
        //    |            -----------------
        //    |            |
        //    |            |
        //    o            |
        // prev start pos  |
        //                 |
        //                 |
        //                 |
        //-----------------|

        rdVector3 checkPos = startPos;
        checkPos.z += 0.15000001f;

        pFoundSec = sithCollision_FindSectorInRadius(pFoundSec, &startPos, &checkPos, 0.0f);
        if ( !pFoundSec )
        {
            return 0;
        }

        moveDist = pThing->collide.movesize + 0.050000001f;
        radius = 0.045000002f;
        sithCollision_SearchForCollisions(pFoundSec, pThing, &checkPos, &moveNorm, moveDist, radius, 0xA00);

        while ( (pCollision = sithCollision_PopStack()) != NULL )
        {
            if ( (pCollision->type & SITHCOLLISION_WORLD) != 0 )
            {
                if ( pCollision->pSurfaceCollided )
                {
                    bFoundObstacle = true;
                    break;
                }
            }
            else if ( (pCollision->type & SITHCOLLISION_THING) != 0 )
            {
                bFoundObstacle = true;
                break;
            }
        }

        sithCollision_DecreaseStackLevel();
    }

    // Check that the thing can climb on to found climb surf/thing from current thing position up 2m
    // 3.) Now check there is no obstacle from current player position upward 2m
        //
        //   .-.
        //  (   )
        //    |
        //    |        -----------------
        //    |        |
        //    |        |
        //    |2m      |
        //    |        |
        //    |        |
        //    X        |
        // player pos  |
        //-------------|
    bool bCanClimbOn = true;
    sithCollision_SearchForCollisions(pThing->pInSector, pThing, &pThing->pos, &rdroid_g_zVector3, 0.2f, 0.029999999f, 0xA00);

    while ( (pCollision = sithCollision_PopStack()) != NULL )
    {
        if ( (pCollision->type & SITHCOLLISION_WORLD) != 0 )
        {
            if ( pCollision->pSurfaceCollided )
            {
                bCanClimbOn = false;
                break;
            }
        }
        else if ( (pCollision->type & SITHCOLLISION_THING) != 0 && pCollision->pThingCollided != pThing )
        {
            bCanClimbOn = false;
            break;
        }
    }

    sithCollision_DecreaseStackLevel();

    // TODO: Simplify
    if ( !bCanClimbOn )
    {
        return 0;
    }

    if ( bFoundClimb && bFoundObstacle )
    {
        return 0;
    }

    if ( !bFoundClimb || bFoundObstacle )
    {
        return bFoundClimb;
    }

    return 1;
}

int J3DAPI sithPlayerActions_CheckClimbDownWall(SithThing* pThing)
{
    // Skip if player has weapon
    if ( sithInventory_GetCurrentWeapon(pThing) || sithWeapon_IsMountingWeapon(pThing) )
    {
        return 0;
    }

    // 1.) Check if player can move backwards for 0.7m
    rdVector3 backDir = RDVECTOR_NEG3(pThing->orient.lvec);
    sithCollision_SearchForCollisions(pThing->pInSector, pThing, &pThing->pos, &backDir, 0.07f, pThing->collide.movesize, 0xA00);

    bool bBlocked = false;
    SithCollision* pCollision;
    while ( (pCollision = sithCollision_PopStack()) != NULL )
    {
        if ( (pCollision->type & (SITHCOLLISION_WORLD | SITHCOLLISION_THING)) != 0 )
        {
            bBlocked = true;
            break;
        }
    }

    sithCollision_DecreaseStackLevel();

    if ( bBlocked )
    {
        return 0;
    }

    // 2.) Check if player can move downwards for 2.5m without hitting obstacle, i.e. thing object or non-climbable (angeled) surface.
    rdVector3 downStartPos;
    rdVector_ScaleAdd3(&downStartPos, &backDir, 0.07f, &pThing->pos);

    SithSector* pFoundSector = sithCollision_FindSectorInRadius(pThing->pInSector, &pThing->pos, &downStartPos, 0.0f);
    if ( !pFoundSector )
    {
        return 0;
    }

    rdVector3 downDir = RDVECTOR_NEG3(rdroid_g_zVector3);
    sithCollision_SearchForCollisions(pFoundSector, pThing, &downStartPos, &downDir, 0.25f, pThing->collide.movesize, 0xA00);
    while ( (pCollision = sithCollision_PopStack()) != NULL )
    {
        if ( (pCollision->type & SITHCOLLISION_WORLD) != 0 )
        {
            if ( (pCollision->pSurfaceCollided->flags & SITH_SURFACE_CLIMBABLE) == 0 ) // if surface is not climbable (angeled) then movement is blocked
            {
                bBlocked = true;
                break;
            }
        }
        else if ( (pCollision->type & SITHCOLLISION_THING) != 0 )
        {
            bBlocked = true;
            break;
        }
    }

    sithCollision_DecreaseStackLevel();
    if ( bBlocked )
    {
        return 0;
    }

    // 3.) Now find ledge or climbable surface by starting at 1m below in player direction for 0.2m
    rdVector3 climbStartPos;
    rdVector_ScaleAdd3(&climbStartPos, &downDir, 0.1f, &downStartPos); // 1m down from previous down start position

    pFoundSector = sithCollision_FindSectorInRadius(pFoundSector, &downStartPos, &climbStartPos, 0.0f);
    if ( !pFoundSector )
    {
        return 0;
    }

    sithCollision_SearchForCollisions(pFoundSector, NULL, &climbStartPos, &pThing->orient.lvec, 0.02f, 0.039999999f, 0xA00);

    bool bAngledWall = false;
    bool bLedge      = false;
    bool bClimbWall  = false;
    rdVector3 negPlayerDir = RDVECTOR_NEG3(pThing->orient.lvec);
    while ( (pCollision = sithCollision_PopStack()) != NULL )
    {
        if ( (pCollision->type & SITHCOLLISION_WORLD) != 0 && pCollision->pSurfaceCollided )
        {
            rdVector3 surfNormal = pCollision->pSurfaceCollided->face.normal;
            if ( surfNormal.z > 0.1f )
            {
                bAngledWall = true;
            }

            surfNormal.z = 0.0f; // Zero z to test player-surface alignment

            rdVector_Normalize3Acc(&surfNormal);
            if ( rdVector_Dot3(&negPlayerDir, &surfNormal) > 0.949f ) // collinear
            {
                if ( (pCollision->pSurfaceCollided->flags & SITH_SURFACE_LEDGE) != 0 )
                {
                    bLedge = true;
                    break;
                }
                if ( (pCollision->pSurfaceCollided->flags & SITH_SURFACE_CLIMBABLE) != 0 )
                {
                    bClimbWall = true;
                    break;
                }
            }
        }
        else if ( (pCollision->type & SITHCOLLISION_THING) != 0 && pCollision->pFaceCollided )
        {
            if ( (pCollision->pFaceCollided->flags & RD_FF_3DO_LEDGE) != 0 )
            {
                rdVector3 surfNormal;
                rdMatrix_TransformVector34(&surfNormal, &pCollision->pFaceCollided->normal, &pCollision->pThingCollided->orient);

                if ( rdVector_Dot3(&surfNormal, &negPlayerDir) > 0.949f ) // collinear
                {
                    bLedge = true;
                    break;
                }
            }
        }
    }

    sithCollision_DecreaseStackLevel();

    if ( bClimbWall )
    {
        return bAngledWall ? 3 : 1;
    }
    else if ( bLedge )
    {
        return 2;
    }
    else
    {
        return 0;
    }
}

int J3DAPI sithPlayerActions_GrabLedge(SithThing* pThing, float distance, SithSurface* pLedgeSurf, rdFace* pLedgeThingFace, const rdModel3Mesh* pLedgeThingMesh, SithThing* pLedgeThing)
{
    J3D_UNUSED(distance);

    // Calculate player new orientation based on ledge surface
    if ( pLedgeSurf )
    {
        // Set look vector opposite to surface normal
        pThing->orient.lvec = RDVECTOR_NEG3(pLedgeSurf->face.normal);

        // Calculate right vector (cross product of look and up vectors)
        rdVector_Cross3(&pThing->orient.rvec, &pThing->orient.lvec, &rdroid_g_zVector3);
        rdVector_Normalize3Acc(&pThing->orient.rvec);

        // Calculate up vector (cross product of right and look vectors)
        rdVector_Cross3(&pThing->orient.uvec, &pThing->orient.rvec, &pThing->orient.lvec);
    }
    else if ( pLedgeThingFace )
    {
        // Transform face normal to world space
        rdVector3 worldNormal;
        rdMatrix_TransformVector34(&worldNormal, &pLedgeThingFace->normal, &pLedgeThing->orient);

        // Set look vector opposite to face normal
        pThing->orient.lvec = RDVECTOR_NEG3(worldNormal);

        // Calculate right vector (cross product of look and up vectors)
        rdVector_Cross3(&pThing->orient.rvec, &pThing->orient.lvec, &rdroid_g_zVector3);
        rdVector_Normalize3Acc(&pThing->orient.rvec);

        // Calculate up vector (cross product of right and look vectors)
        rdVector_Cross3(&pThing->orient.uvec, &pThing->orient.rvec, &pThing->orient.lvec);
    }

    // Set player move status
    pThing->moveStatus = SITHPLAYERMOVE_HANGING;

    rdVector3 curPos = pThing->pos;
    if ( pLedgeSurf )
    {
        // Find highest Z coordinate of surface vertices
        // Note, OG in-place calculation was done
        float ledgeTopZ = sithPlayerActions_GetLedgeSurfaceGrabPosZ(pLedgeSurf);

        // Calculate distance from player to surface plane
        rdVector3 altPos  = pThing->pos;
        altPos.z = ledgeTopZ;

        float playerToLedgeDist  = rdMath_DistancePointToPlane(&altPos, &pLedgeSurf->face.normal, &sithWorld_g_pCurrentWorld->aVertices[pLedgeSurf->face.aVertices[0]]);

        // Move player to ledge surface
        rdVector_MultAcc3(&pThing->pos, &pThing->orient.lvec, playerToLedgeDist);

        // Move player back 0.24m and position 1.18m below ledge top
        rdVector_MultAcc3(&pThing->pos, &pThing->orient.lvec, -0.024f);
        pThing->pos.z = ledgeTopZ - 0.118f; // move to ledge hand grabbing position

        // Update sector if needed
        SithSector* pLedgeSector = sithCollision_FindSectorInRadius(pThing->pInSector, &curPos, &pThing->pos, 0.0f);
        if ( pLedgeSector != pThing->pInSector )
        {
            sithThing_ExitSector(pThing);
            sithThing_EnterSector(pThing, pLedgeSector, 1, 0);
        }

        sithThing_AttachThingToClimbSurface(pThing, pLedgeSurf);
        sithPlayerActions_g_pCurLedgeSurface = pLedgeSurf;
    }
    else if ( pLedgeThingFace )
    {
        // Get grab point Z coordinate for thing ledge
        // Note, the returned Z coordinate is in world space
        float ledgeTopZ = sithPlayerActions_GetLedgeThingGrabPosZ(pLedgeThing, pLedgeThingFace, pLedgeThingMesh);
        ledgeTopZ += pLedgeThing->pos.z; // Convert to world coordinate

        // Transform model face and vertex to world space
        rdVector3 worldFaceNormal;
        rdMatrix_TransformVector34(&worldFaceNormal, &pLedgeThingFace->normal, &pLedgeThing->orient);

        rdVector3 worldVertex;
        rdMatrix_TransformVector34(&worldVertex, &pLedgeThingMesh->apVertices[pLedgeThingFace->aVertices[0]], &pLedgeThing->orient);

        // Convert to world coordinates
        rdVector_Add3Acc(&worldVertex, &pLedgeThing->pos);

        // Calculate distance from player to face plane
        rdVector3 altPos = pThing->pos;
        altPos.z = ledgeTopZ;

        float playerToLedgeDist  = rdMath_DistancePointToPlane(&altPos, &worldFaceNormal, &worldVertex);

        // Move player to ledge face
        rdVector_MultAcc3(&pThing->pos, &pThing->orient.lvec, playerToLedgeDist);

        // Move player back 0.24m and position 1.18m below ledge top
        rdVector_MultAcc3(&pThing->pos, &pThing->orient.lvec, -0.024f);
        pThing->pos.z = ledgeTopZ - 0.118f;

        // Update sector if needed
        SithSector* pNewSector = sithCollision_FindSectorInRadius(pThing->pInSector, &curPos, &pThing->pos, 0.0f);
        if ( pNewSector != pThing->pInSector )
        {
            sithThing_ExitSector(pThing);
            sithThing_EnterSector(pThing, pNewSector, 1, 0);
        }

        sithThing_AttachThingToThingFace(pThing, pLedgeThing, pLedgeThingFace, pLedgeThingMesh->apVertices, 1);

        sithPlayerActions_g_pCurLedgeThingModelFace = pLedgeThingFace;
        sithPlayerActions_g_pCurLedgeThingModel     = pLedgeThing->renderData.data.pModel3;
    }

    sithPuppet_PlayMode(pThing, SITHPUPPETSUBMODE_HANGLEDGE, NULL);
    pThing->moveInfo.physics.flags &= ~SITH_PF_FLOORSTICK;
    sithInventory_SetSwimmingInventory(pThing, /*bItemsAvailable=*/0);
    return 1;
}

int J3DAPI sithPlayerActions_FindAndAttachToClimbWall(SithThing* pThing)
{
    sithCollision_SearchForCollisions(pThing->pInSector, pThing, &pThing->pos, &pThing->orient.lvec, pThing->collide.movesize, pThing->collide.movesize, 0xA00);

    SithCollision* pCollision;
    while ( (pCollision = sithCollision_PopStack()) != NULL )
    {
        const bool bSurfOrAdjoin = (pCollision->type & SITHCOLLISION_WORLD) != 0 || (pCollision->type & SITHCOLLISION_ADJOINCROSS) != 0;
        if ( bSurfOrAdjoin && pCollision->pSurfaceCollided && (pCollision->pSurfaceCollided->flags & SITH_SURFACE_CLIMBABLE) != 0 )
        {
            pThing->moveStatus = SITHPLAYERMOVE_CLIMBIDLE;

            sithPlayerActions_CenterOnClimbSurface(pThing, pCollision->pSurfaceCollided);
            sithThing_AttachThingToClimbSurface(pThing, pCollision->pSurfaceCollided);

            sithPuppet_PlayMode(pThing, SITHPUPPETSUBMODE_CLIMBWALLIDLE, NULL);

            pThing->moveInfo.physics.flags &= ~SITH_PF_FLOORSTICK;
            sithInventory_SetSwimmingInventory(pThing, /*bItemsAvailable=*/0); // Fixed: Disable inventory when climbing. Re-enabled in sithPuppet_FinishForceMove
                                                                               //        Originally it was bug and inventory was enabled when climbing on wall.
            break;
        }
    }

    sithCollision_DecreaseStackLevel();
    return 1;
}

int J3DAPI sithPlayerActions_CanPullUp(SithThing* pThing)
{
    // 1.) Check if player is attached to thing face and in this case it must not be moving
    if ( pThing->attach.flags == SITH_ATTACH_THINGFACE )
    {
        SithThing* pThingAttached = pThing->attach.attachedToStructure.pThingAttached;
        if ( pThingAttached )
        {
            if ( pThingAttached->moveType == SITH_MT_PHYSICS )
            {
                if ( !rdVector_IsZero3(&pThingAttached->moveInfo.physics.velocity) )
                {
                    return 0;
                }
            }
            else if ( pThingAttached->moveType == SITH_MT_PATH &&
                (pThingAttached->moveInfo.pathMovement.mode & (SITH_PATHMOVE_ROTATE | SITH_PATHMOVE_MOVE)) != 0 )
            {
                return 0;
            }
        }
    }

    // 2.) Search for any blocking thing surface in upward direction for 2.2m
    bool bCanMoveUp = true;
    sithCollision_SearchForCollisions(pThing->pInSector, pThing, &pThing->pos, &rdroid_g_zVector3, 0.22f, 0.015f, 0xA00);

    SithCollision* pCollision;
    while ( (pCollision = sithCollision_PopStack()) != NULL )
    {
        if ( (pCollision->type & SITHCOLLISION_WORLD) != 0 )
        {
            if ( pCollision->pSurfaceCollided )
            {
                bCanMoveUp = false;
                break;
            }
        }
        else if ( (pCollision->type & SITHCOLLISION_THING) != 0 )
        {
            bCanMoveUp = false;
            break;
        }
        else if ( (pCollision->type & SITHCOLLISION_ADJOINTOUCH) != 0 && pCollision->pSurfaceCollided &&
            (pCollision->pSurfaceCollided->pAdjoin->flags & SITH_ADJOIN_NOPLAYERMOVE) != 0 )
        {
            bCanMoveUp = false;
            break;
        }
    }

    sithCollision_DecreaseStackLevel();

    if ( !bCanMoveUp )
    {
        return 0;
    }

    // 3.) Search for any blocking thing or surface for 0.7m
    //     at the top position (1.78m from current position) in the direction of current player direction
    rdVector3 topPos = pThing->pos;
    topPos.z += 0.178f;

    SithSector* pStartSec = sithCollision_FindSectorInRadius(pThing->pInSector, &pThing->pos, &topPos, 0.0f);
    if ( !pStartSec )
    {
        return 0;
    }

    sithCollision_SearchForCollisions(pStartSec, NULL, &topPos, &pThing->orient.lvec, 0.07f, 0.0099999998f, 0xA00);

    while ( (pCollision = sithCollision_PopStack()) != NULL )
    {
        if ( (pCollision->type & SITHCOLLISION_WORLD) != 0 )
        {
            if ( pCollision->pSurfaceCollided )
            {
                bCanMoveUp = false;
                break;
            }
        }
        else if ( (pCollision->type & SITHCOLLISION_THING) != 0 )
        {
            bCanMoveUp = false;
            break;
        }
        else if ( (pCollision->type & SITHCOLLISION_ADJOINTOUCH) != 0 && pCollision->pSurfaceCollided
            && (pCollision->pSurfaceCollided->pAdjoin->flags & SITH_ADJOIN_NOPLAYERMOVE) != 0 )
        {
            bCanMoveUp = false;
            break;
        }
    }

    sithCollision_DecreaseStackLevel();

    if ( !bCanMoveUp )
    {
        return 0;
    }

    // 4.) Do another search for any blocking thing or surface for 0.7m
    //     at the top position (1.78m + 0.69m from current position) in the direction of current player direction
    topPos.z += 0.068999998f;
    pStartSec = sithCollision_FindSectorInRadius(pThing->pInSector, &pThing->pos, &topPos, 0.0f);
    if ( !pStartSec )
    {
        return 0;
    }

    sithCollision_SearchForCollisions(pStartSec, NULL, &topPos, &pThing->orient.lvec, 0.07f, 0.02f, 0xA00);

    while ( (pCollision = sithCollision_PopStack()) != NULL )
    {
        if ( (pCollision->type & SITHCOLLISION_WORLD) != 0 )
        {
            if ( pCollision->pSurfaceCollided )
            {
                bCanMoveUp = false;
                break;
            }
        }
        else if ( (pCollision->type & SITHCOLLISION_THING) != 0 )
        {
            bCanMoveUp = false;
            break;
        }
        else if ( (pCollision->type & SITHCOLLISION_ADJOINTOUCH) != 0 && pCollision->pSurfaceCollided &&
            (pCollision->pSurfaceCollided->pAdjoin->flags & SITH_ADJOIN_NOPLAYERMOVE) != 0 )
        {
            bCanMoveUp = false;
            break;
        }
    }

    sithCollision_DecreaseStackLevel();
    return bCanMoveUp ? 1 : 0;
}

float J3DAPI sithPlayerActions_GetLedgeSurfaceGrabPosZ(const SithSurface* pLedgeSurf)
{
    // Find highest Z coordinate of surface vertices

    const rdFace* pFace = &pLedgeSurf->face;
    if ( pFace->numVertices <= 0 ) // Added
    {
        return 0.0f;
    }

    float topZ  = sithWorld_g_pCurrentWorld->aVertices[pFace->aVertices[0]].z;
    for ( size_t i = 1; i < pFace->numVertices; ++i )
    {
        float vertexZ = sithWorld_g_pCurrentWorld->aVertices[pFace->aVertices[i]].z;
        if ( vertexZ > topZ )
        {
            topZ = vertexZ;
        }
    }
    return topZ;
}

float J3DAPI sithPlayerActions_GetLedgeThingGrabPosZ(const SithThing* pThing, const rdFace* pFace, const rdModel3Mesh* pMesh)
{
    if ( pFace->numVertices <= 0 )
    {
        return 0.0f;
    }

    // Transform first vertex to world space
    rdVector3 tvert;
    rdMatrix_TransformVector34(&tvert, &pMesh->apVertices[*pFace->aVertices], &pThing->orient);

    float topZ = tvert.z;
    for ( size_t i = 1; i < pFace->numVertices; ++i )
    {
        rdMatrix_TransformVector34(&tvert, &pMesh->apVertices[pFace->aVertices[i]], &pThing->orient);
        if ( tvert.z > topZ )
        {
            topZ = tvert.z;
        }
    }

    return topZ;
}

void sithPlayerActions_StartInvisibility(void)
{
    if ( sithPlayerActions_g_bPlayerInvisible != 1 )
    {
        sithPlayer_g_pLocalPlayerThing->flags &= ~SITH_TF_SHADOW;

        rdVector4 startColor = { 0.0f, 0.0f, 0.0f, 1.0f };
        rdVector4 endColor   = { 0.0f, 0.0f, 0.0f, 0.1f };
        sithAnimate_StartThingFadeAnim(sithPlayer_g_pLocalPlayerThing, &startColor, &endColor, 1.0f, (SithAnimateFlags)0);

        sithPlayerActions_g_bPlayerInvisible = 1;

        if ( sithPlayer_g_pLocalPlayerThing )
        {
            sithPlayer_g_pLocalPlayerThing->thingInfo.actorInfo.flags |= SITH_AF_INVISIBLE;
        }
    }
}

void sithPlayerActions_EndInvisibility(void)
{
    if ( sithPlayerActions_g_bPlayerInvisible )
    {
        rdVector4 startColor = { 0.0f, 0.0f, 0.0f, 0.1f };
        rdVector4 endColor   = { 0.0f, 0.0f, 0.0f, 1.0f };
        sithAnimate_StartThingFadeAnim(sithPlayer_g_pLocalPlayerThing, &startColor, &endColor, 1.0f, (SithAnimateFlags)0);

        sithPlayerActions_g_bPlayerInvisible = 0;
        sithFX_DestroyFairyDustDeluxe(sithPlayer_g_pLocalPlayerThing);

        if ( sithPlayer_g_pLocalPlayerThing )
        {
            sithPlayer_g_pLocalPlayerThing->thingInfo.actorInfo.flags &= ~SITH_AF_INVISIBLE;
        }

        sithPlayer_g_pLocalPlayerThing->flags |= SITH_TF_SHADOW;
    }
}

int sithPlayerActions_IsInvisible(void)
{
    return sithPlayerActions_g_bPlayerInvisible;
}

int sithPlayerActions_StartJewelFlying(void)
{
    SithThing* pPlayerThing = sithPlayer_g_pLocalPlayerThing;
    if ( !pPlayerThing )
    {
        return 0;
    }

    int curWeapon = sithInventory_GetCurrentWeapon(pPlayerThing);
    if ( curWeapon != SITHWEAPON_IMP3 )
    {
        return 0;
    }

    if ( !sithPlayerActions_g_bJewelFlying )
    {
        return 0;
    }

    sithPhysics_ResetThingMovement(pPlayerThing);

    pPlayerThing->moveStatus = SITHPLAYERMOVE_JEWELFLYING;
    sithThing_DetachThing(pPlayerThing);

    pPlayerThing->moveInfo.physics.flags |= SITH_PF_FLY;
    pPlayerThing->moveInfo.physics.flags &= ~SITH_PF_USEGRAVITY;

    if ( sithPlayerActions_g_pPlasma )
    {
        sithPlayerActions_g_pPlasma->flags &= ~SITH_TF_DISABLED;

        rdVector3 newPos = pPlayerThing->pos;
        newPos.x = sithPlayerActions_g_pPlasma->pos.x;
        newPos.y = sithPlayerActions_g_pPlasma->pos.y;
        newPos.z += 0.050000001f;

        pPlayerThing->collide.type = SITH_COLLIDE_NONE;
        pPlayerThing->thingInfo.actorInfo.bControlsDisabled = 1;
        sithAnimate_StartThingMoveToPos(pPlayerThing, &newPos, 1.0f); // 1 sec move
    }

    sithInventory_SetSwimmingInventory(pPlayerThing, /*bItemsAvailable=*/0);

    rdKeyframe* pKFTrack = sithPuppet_GetKeyframe("in_imp3_float.key");
    if ( !pKFTrack )
    {
        return 1;
    }

    sithPlayerActions_g_jewelFlyingPuppetTrackNum = sithPuppet_PlayKey(pPlayerThing->renderData.pPuppet, pKFTrack, 3, 4, (rdKeyframeFlags)0, NULL);
    sithPuppet_ClearMode(pPlayerThing, SITHPUPPETSUBMODE_STAND);
    return 1;
}

void J3DAPI sithPlayerActions_EnableJewelFlying(SithThing* pPlasma)
{
    sithPlayerActions_g_pPlasma      = pPlasma;
    sithPlayerActions_g_bJewelFlying = 1;

    if ( sithPlayer_g_impFireType == SITHWEAPON_IMP3 )
    {
        sithPlayerActions_StartJewelFlying();
    }
}

void sithPlayerActions_DisableJewelFlying(void)
{
    if ( sithPlayer_g_pLocalPlayerThing )
    {
        switch ( sithPlayer_g_pLocalPlayerThing->moveStatus )
        {
            case SITHPLAYERMOVE_CRAWLIDLE:
            case SITHPLAYERMOVE_UNKNOWN_4:
            case SITHPLAYERMOVE_UNKNOWN_5:
            case SITHPLAYERMOVE_PUSHING:
            case SITHPLAYERMOVE_PULLING:
            case SITHPLAYERMOVE_STAND_TO_CRAWL:
            case SITHPLAYERMOVE_CRAWL_TO_STAND:
                break;

            default:
                sithPlayer_g_pLocalPlayerThing->collide.movesize = 0.039999999f;
                break;
        }
    }

    if ( sithPlayerActions_g_pPlasma )
    {
        sithPlayerActions_g_pPlasma->flags |= SITH_TF_DISABLED;
    }

    sithPlayerActions_g_pPlasma = NULL;
    sithPlayerActions_g_bJewelFlying = 0;
}

void J3DAPI sithPlayerActions_QuickTurnLeft(SithThing* pThing)
{
    if ( !pThing )
    {
        SITHLOG_ERROR("Bad thing pointer to QuickTurnLeft()");
        return;
    }

    if ( !sithAnimate_StartThingQuickTurn(pThing, /*direction=*/0) )
    {
        SITHLOG_ERROR("Could not start quick-turn sith animation on %s!", pThing->aName);
        return;
    }

    pThing->thingInfo.actorInfo.flags |= SITH_AF_CONTROLSDISABLED;
    pThing->moveStatus = SITHPLAYERMOVE_TURNING_LEFT_90_DEGREES;

    rdKeyframe* pKfTrack = sithPuppet_GetKeyframe("in_rotate90_left.key");
    if ( pKfTrack )
    {
        sithPuppet_PlayKey(pThing->renderData.pPuppet, pKfTrack, 1, 2, RDKEYFRAME_DISABLE_FADEIN | RDKEYFRAME_NOLOOP, NULL);
    }
}

void J3DAPI sithPlayerActions_QuickTurnRight(SithThing* pThing)
{
    if ( !pThing )
    {
        SITHLOG_ERROR("Bad thing pointer to QuickTurnRight()");
        return;
    }

    if ( !sithAnimate_StartThingQuickTurn(pThing, /*direction=*/1) )
    {
        SITHLOG_ERROR("Could not start quick-turn sith animation on %s!");
        return;
    }

    pThing->thingInfo.actorInfo.flags |= SITH_AF_CONTROLSDISABLED;
    pThing->moveStatus = SITHPLAYERMOVE_TURNING_RIGHT_90_DEGREES;

    rdKeyframe* pKfTrack = sithPuppet_GetKeyframe("in_rotate90_right.key");
    if ( pKfTrack )
    {
        sithPuppet_PlayKey(pThing->renderData.pPuppet, pKfTrack, 1, 2, RDKEYFRAME_DISABLE_FADEIN | RDKEYFRAME_NOLOOP, NULL);
    }
}

void J3DAPI sithPlayerActions_CenterOnClimbSurface(SithThing* pThing, const SithSurface* pSurface)
{
    rdVector3 surfNormal = pSurface->face.normal;
    surfNormal.z = 0.0f; // Set Z to 0 to make surface vertical wall
    rdVector_Normalize3Acc(&surfNormal);

    // Calculate surface bounds
    const rdFace* pFace = &pSurface->face;

    float minX = sithWorld_g_pCurrentWorld->aVertices[pFace->aVertices[0]].x;
    float maxX = minX;

    float minY = sithWorld_g_pCurrentWorld->aVertices[pFace->aVertices[0]].y;
    float maxY = minY;

    for ( size_t i = 0; i < pFace->numVertices; ++i )
    {
        float vertX = sithWorld_g_pCurrentWorld->aVertices[pFace->aVertices[i]].x;
        float vertY = sithWorld_g_pCurrentWorld->aVertices[pFace->aVertices[i]].y;

        if ( vertX > maxX ) maxX = vertX;
        if ( vertX < minX ) minX = vertX;
        if ( vertY > maxY ) maxY = vertY;
        if ( vertY < minY ) minY = vertY;
    }

    // Calculate center of surface
    float centerX = (maxX + minX) / 2.0f;
    float centerY = (maxY + minY) / 2.0f;

    // Calculate new position offset from surface by 0.05 units
    rdVector3 newPos = {
        centerX + surfNormal.x * 0.050000001f,
        centerY + surfNormal.y * 0.050000001f,
        pThing->pos.z + surfNormal.z * 0.050000001f
    };

    SithSector* pNewSector = sithCollision_FindSectorInRadius(pThing->pInSector, &pThing->pos, &newPos, 0.0f);
    if ( !pNewSector )
    {
        SITHLOG_ERROR("can't move to valid sector centering on climb face!");
        return;
    }

    pThing->pos = newPos;

    if ( pNewSector != pThing->pInSector )
    {
        sithThing_ExitSector(pThing);
        sithThing_EnterSector(pThing, pNewSector, /*bNoWaterSplash=*/1, /*bNoNotify=*/1);
    }
}

void J3DAPI sithPlayerActions_MoveToCrawlPosition(SithThing* pThing)
{
    // Find distance to crawl surface in down direction for 2m
    rdVector3 moveNorm = RDVECTOR_NEG3(rdroid_g_zVector3);
    sithCollision_SearchForCollisions(pThing->pInSector, NULL, &pThing->pos, &moveNorm, 0.2f, 0.0f, 0xA00);

    float distance = 0.0f;
    SithCollision* pCollision;
    while ( (pCollision = sithCollision_PopStack()) != NULL )
    {
        if ( (pCollision->type & SITHCOLLISION_WORLD) != 0
            && pCollision->pSurfaceCollided && (pCollision->pSurfaceCollided->flags & SITH_SURFACE_ISFLOOR) != 0 )
        {
            distance = pCollision->distance;
            break;
        }
    }

    sithCollision_DecreaseStackLevel();

    // Set new player position & sector
    distance -= 0.045000002f; // 0.045f is height of crawl player collider
    if ( distance > 0.0f )
    {
        rdVector3 newPos = {
            pThing->pos.x + moveNorm.x * distance,
            pThing->pos.y + moveNorm.y * distance,
            pThing->pos.z + moveNorm.z * distance
        };

        SithSector* pNewSector = sithCollision_FindSectorInRadius(pThing->pInSector, &pThing->pos, &newPos, 0.0f);
        pThing->pos = newPos;

        if ( pNewSector != pThing->pInSector )
        {
            sithThing_ExitSector(pThing);
            sithThing_EnterSector(pThing, pNewSector, 0, 0);
        }
    }
}
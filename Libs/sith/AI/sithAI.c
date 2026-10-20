#include "sithAI.h"
#include <j3dcore/j3dhook.h>

#include <rdroid/Math/rdMatrix.h>
#include <rdroid/Math/rdVector.h>

#include <sith/AI/sithAIInstinct.h>
#include <sith/AI/sithAIMove.h>
#include <sith/AI/sithAIUtil.h>
#include <sith/Cog/sithCog.h>
#include <sith/Devices/sithConsole.h>
#include <sith/Engine/sithPuppet.h>
#include <sith/Gameplay/sithTime.h>
#include <sith/RTI/symbols.h>
#include <sith/World/sithActor.h>
#include <sith/World/sithSurface.h>
#include <sith/World/sithWeapon.h>

#include <std/General/stdHashtbl.h>
#include <std/General/stdMemory.h>
#include <std/General/stdUtil.h>
#include <stdlib.h>

static size_t numFreeAIs = 0; // Added: Init to 0
static size_t aFreeAIIndices[STD_ARRAYLEN(sithAI_g_aControlBlocks)];

static bool bStartup = false; // Added: Init to false
static SithAIRegisteredInstinct* aRegisteredInstincts;
static size_t numRegisteredInstincts;
static tHashTable* pRegisteredInstinctHashtbl;

void J3DAPI sithAI_ProcessAIStateChange(SithAIControlBlock* pLocal);
void J3DAPI sithAI_InstinctUpdate(SithAIControlBlock* pLocal);
int J3DAPI sithAI_ProcessBlockedEvent(SithAIControlBlock* pLocal, SithAIEventType aievent);
int J3DAPI sithAI_ProcessUnhandledEvent(SithAIControlBlock* pLocal, SithAIEventType event, void* pObject);
int J3DAPI sithAI_ProcessUnhandledWpntEvent(SithAIControlBlock* pLocal, SithAIEventType event);
int J3DAPI sithAI_CreateInstinctRegistry(size_t maxInstincts);
int J3DAPI sithAI_FreeAI(size_t aiNum);
int sithAI_CreateAI(void);
void sithAI_ResetAllAIs(void);

void sithAI_InstallHooks(void)
{
    J3D_HOOKFUNC(sithAI_Startup);
    J3D_HOOKFUNC(sithAI_Shutdown);
    J3D_HOOKFUNC(sithAI_Open);
    J3D_HOOKFUNC(sithAI_Close);
    J3D_HOOKFUNC(sithAI_Create);
    J3D_HOOKFUNC(sithAI_Free);
    J3D_HOOKFUNC(sithAI_Process);
    J3D_HOOKFUNC(sithAI_ProcessAIStateChange);
    J3D_HOOKFUNC(sithAI_InstinctUpdate);
    J3D_HOOKFUNC(sithAI_ForceInstinctUpdate);
    J3D_HOOKFUNC(sithAI_EmitEvent);
    J3D_HOOKFUNC(sithAI_ProcessBlockedEvent);
    J3D_HOOKFUNC(sithAI_ProcessUnhandledEvent);
    J3D_HOOKFUNC(sithAI_ProcessUnhandledWpntEvent);
    J3D_HOOKFUNC(sithAI_Stop);
    J3D_HOOKFUNC(sithAI_StopAllAttackingAIs);
    J3D_HOOKFUNC(sithAI_RegisterInstinct);
    J3D_HOOKFUNC(sithAI_FindInstinct);
    J3D_HOOKFUNC(sithAI_GetInstinctIndex);
    J3D_HOOKFUNC(sithAI_HasInstinct);
    J3D_HOOKFUNC(sithAI_EnableInstinct);
    J3D_HOOKFUNC(sithAI_ParseArg);
    J3D_HOOKFUNC(sithAI_AllocAIFrames);
    J3D_HOOKFUNC(sithAI_CreateInstinctRegistry);
    J3D_HOOKFUNC(sithAI_FreeAI);
    J3D_HOOKFUNC(sithAI_CreateAI);
    J3D_HOOKFUNC(sithAI_ResetAllAIs);
}

void sithAI_ResetGlobals(void)
{
    STD_ZEROMEM(&sithAI_g_bOpen, sizeof(sithAI_g_bOpen));
    STD_ZEROMEM(&sithAI_g_aControlBlocks, sizeof(sithAI_g_aControlBlocks));
    STD_ZEROMEM(&sithAI_g_lastUsedAIIndex, sizeof(sithAI_g_lastUsedAIIndex));
}

int sithAI_Startup(void)
{
    if ( bStartup )
    {
        SITHLOG_ERROR("Warning: System already initialized!\n");
        return 0; // Fixed: Return after the warning, the original does not initialize the system a second time (original Indy3D.exe v1.0 0x00429E90)
    }

    if ( sithAI_CreateInstinctRegistry(SITHAI_MAXREGISTEREDINSTINCTS) )
    {
        return 1;
    }

    sithAIInstinct_InitInstincts();
    sithAI_ResetAllAIs();
    bStartup = true;

    return 0;
}

void sithAI_Shutdown(void)
{
    if ( !bStartup )
    {
        SITHLOG_ERROR("Warning: System already shutdown!\n");
        return; // Fixed: Return after the warning, the original does not free the instinct registry a second time (original Indy3D.exe v1.0 0x00429EF0)
    }

    SITH_ASSERTREL(aRegisteredInstincts && pRegisteredInstinctHashtbl);
    STDFREE(aRegisteredInstincts);
    stdHashtbl_Free(pRegisteredInstinctHashtbl);
    bStartup = false;
}

void sithAI_Open(void)
{
    if ( !bStartup )
    {
        SITHLOG_ERROR("Error: Attempt to open uninitialized system!\n");
    }

    if ( sithAI_g_bOpen )
    {
        SITHLOG_ERROR("Warning: System already open!\n");
    }

    sithAI_g_bOpen = 1;
}

void sithAI_Close(void)
{
    if ( !sithAI_g_bOpen )
    {
        SITHLOG_ERROR("Warning: System already closed!\n");
    }

    sithAI_ResetAllAIs();
    sithAI_g_bOpen = 0;
}

void J3DAPI sithAI_Create(SithThing* pThing)
{
    SITH_ASSERTREL(pThing);
    SITH_ASSERTREL(pThing->controlType == SITH_CT_AI);

    SithAIClass* pClass = pThing->controlInfo.aiControl.pClass;
    if ( !pClass )
    {
        pThing->controlType = SITH_CT_PLOT;
    }
    else
    {
        int aiIndex = sithAI_CreateAI();
        if ( aiIndex < 0 )
        {
            SITHLOG_ERROR("Too many AI things, not enough local control blocks.\n");
            pThing->controlType = SITH_CT_PLOT;
            return;
        }

        pThing->controlInfo.aiControl.pLocal = &sithAI_g_aControlBlocks[aiIndex];
        SithAIControlBlock* pLocal = pThing->controlInfo.aiControl.pLocal;

        pLocal->homePos    = pThing->pos;
        pLocal->homeOrient = pThing->orient.lvec;

        pLocal->pClass       = pClass;
        pLocal->pOwner       = pThing;
        pLocal->numInstincts = pClass->numInstincts;
        pLocal->mode         = SITHAI_MODE_SLEEPING | SITHAI_MODE_SEARCHING;
        pLocal->moveSpeed    = 1.5f;

        if ( pThing->pPuppetClass )
        {
            if ( pThing->pPuppetState )
            {
                pThing->pPuppetState->armedMode = pClass->armedMode;
                pThing->pPuppetState->majorMode = SITH_PUPPET_GETARMEDMAJORMODE(pThing, pClass->armedMode);
            }
        }

        pLocal->allowedSurfaceTypes = 0;
    }
}

void J3DAPI sithAI_Free(SithThing* pThing)
{
    SITH_ASSERTREL(pThing->controlType == SITH_CT_AI);

    if ( pThing->controlInfo.aiControl.pLocal )
    {
        size_t index = pThing->controlInfo.aiControl.pLocal - sithAI_g_aControlBlocks;
        SITH_ASSERTREL(index < STD_ARRAYLEN(sithAI_g_aControlBlocks));

        if ( pThing->controlInfo.aiControl.pLocal->aFrames )
        {
            STDFREE(pThing->controlInfo.aiControl.pLocal->aFrames);
            pThing->controlInfo.aiControl.pLocal->aFrames = NULL;
        }

        sithAI_FreeAI(index);
        pThing->controlInfo.aiControl.pLocal = NULL;
    }
}

void sithAI_Process(void)
{
    for ( int index = 0; index <= sithAI_g_lastUsedAIIndex; ++index )
    {
        SithAIControlBlock* pLocal = &sithAI_g_aControlBlocks[index];
        if ( pLocal->pClass )
        {
            if ( (pLocal->pOwner->flags & (SITH_TF_DISABLED | SITH_TF_DYING | SITH_TF_DESTROYED)) == 0
                && pLocal->pOwner->thingInfo.actorInfo.health > 0.0f
                && pLocal->pOwner->thingInfo.actorInfo.bForceMovePlay != 1
                && (pLocal->mode & SITHAI_MODE_BLOCK) == 0
                && (pLocal->mode & (SITHAI_MODE_DISABLED | SITHAI_MODE_SLEEPING)) == 0 )
            {
                sithAI_ProcessAIStateChange(pLocal);
                sithWeapon_UpdateActorWeapon(pLocal->pOwner);
                if ( pLocal->msecNextUpdate <= sithTime_g_msecGameTime && sithAIMove_AIGetMoveState(pLocal) <= 0 )
                {
                    sithAI_InstinctUpdate(pLocal);
                }
            }
        }
    }
}

void J3DAPI sithAI_ProcessAIStateChange(SithAIControlBlock* pLocal)
{
    SithActorStateChange* pStateChange = &pLocal->pOwner->thingInfo.actorInfo.stateChange;
    if ( pStateChange->type == SITHACTORSTATECHANGE_ARMEDMODE )
    {
        if ( sithPuppet_IsAnyModeOnTrack(pLocal->pOwner, SITHPUPPETSUBMODE_STAND2WALK, SITHPUPPETSUBMODE_WALK2ATTACK) )
        {
            return;
        }

        sithPuppet_RemoveAllTracks(pLocal->pOwner);
        sithPuppet_SetArmedMode(pLocal->pOwner, pStateChange->params.armedMode);

        pLocal->pOwner->pPuppetState->submode = 0;
        pLocal->pOwner->moveStatus = SITHPLAYERMOVE_STILL;
        sithAIMove_UpdateAIMove(pLocal);

        sithCog_ThingSendMessageEx(pLocal->pOwner, 0, SITHCOG_MSG_STATECHANGE, pStateChange->type, pStateChange->params.armedMode, 0, 0);
        sithActor_ResetStateChange(pLocal->pOwner);
        return;
    }

    if ( pStateChange->type == SITHACTORSTATECHANGE_ANIMMOVE && sithTime_g_msecGameTime >= pLocal->msecPauseMoveUntil )
    {
        sithAIMove_AISpecialMove(pLocal, pStateChange->params.moveFlags);
        sithCog_ThingSendMessageEx(pLocal->pOwner, 0, SITHCOG_MSG_STATECHANGE, pStateChange->type, pStateChange->params.moveFlags, 0, 0);
        sithActor_ResetStateChange(pLocal->pOwner);
    }
}

void J3DAPI sithAI_InstinctUpdate(SithAIControlBlock* pLocal)
{
    uint32_t msecNextUpdate = sithTime_g_msecGameTime + 5000;
    SithAIMode curMode = pLocal->mode;

LABEL_2:
    for ( size_t i = 0; i < pLocal->numInstincts; ++i )
    {
        if ( (pLocal->aInstinctStates[i].flags & SITHAI_INSTINCT_DISABLED) == 0
            && (pLocal->pClass->aInstincts[i].updateModes & pLocal->mode) != 0
            && (pLocal->pClass->aInstincts[i].updateBlockModes & pLocal->mode) == 0 )
        {
            if ( pLocal->aInstinctStates[i].msecNextUpdate <= sithTime_g_msecGameTime )
            {
                pLocal->aInstinctStates[i].msecNextUpdate = sithTime_g_msecGameTime + 1000;
                int bHandled = pLocal->pClass->aInstincts[i].pfInstinct(pLocal, &pLocal->pClass->aInstincts[i], &pLocal->aInstinctStates[i], (SithAIEventType)0, 0);

                if ( curMode != pLocal->mode )
                {
                    sithAI_EmitEvent(pLocal, SITHAI_EVENT_MODECHANGED, (void*)curMode);
                }

                if ( bHandled )
                {
                    curMode = pLocal->mode;
                    goto LABEL_2;
                    // TODO: Instead of using goto, we could set i = -1 and continue the loop
                }
            }

            if ( pLocal->aInstinctStates[i].msecNextUpdate < msecNextUpdate )
            {
                msecNextUpdate = pLocal->aInstinctStates[i].msecNextUpdate;
            }
        }
    }

    pLocal->msecNextUpdate = msecNextUpdate;
}

int J3DAPI sithAI_ForceInstinctUpdate(SithAIControlBlock* pLocal, const char* pInstinctName, int bSendModeChangeEvent)
{
    SITH_ASSERTREL(pLocal);

    SithAIMode prevMode = pLocal->mode;
    SithAIRegisteredInstinct* pInstinct = sithAI_FindInstinct(pInstinctName);
    if ( !pInstinct )
    {
        SITHLOG_ERROR("Invalid instinct '%s' passed to sithAI_ForceInstinctUpdate().\n", pInstinctName);
        return 0;
    }

    int instIdx = sithAI_GetInstinctIndex(pLocal, pInstinct);
    if ( instIdx <= -1 )
    {
        return 0;
    }

    if ( (pLocal->aInstinctStates[instIdx].flags & SITHAI_INSTINCT_DISABLED) != 0 )
    {
        return 0;
    }

    if ( (pLocal->pClass->aInstincts[instIdx].updateModes & pLocal->mode) == 0
        || (pLocal->pClass->aInstincts[instIdx].updateBlockModes & pLocal->mode) != 0 )
    {
        return 0;
    }

    pLocal->aInstinctStates[instIdx].msecNextUpdate = sithTime_g_msecGameTime + 1000;
    pLocal->pClass->aInstincts[instIdx].pfInstinct(pLocal, &pLocal->pClass->aInstincts[instIdx], &pLocal->aInstinctStates[instIdx], (SithAIEventType)0, NULL);

    if ( !bSendModeChangeEvent )
    {
        return 1;
    }

    if ( prevMode != pLocal->mode )
    {
        sithAI_EmitEvent(pLocal, SITHAI_EVENT_MODECHANGED, (void*)prevMode);
    }

    return 1;
}

//void J3DAPI sithAI_EmitEvent(SithAIControlBlock* pLocal, SithAIEventType event, void* pObject)
//{
//    int surfIdx;
//    int cogRes = 0;
//    int cogParam = 0;
//    int instinctRes = 0;
//    bool bSendCogEvent = false;
//
//    if ( !pLocal->pClass || !pLocal->pOwner || (pLocal->pOwner->flags & (SITH_TF_DISABLED | SITH_TF_DYING | SITH_TF_DESTROYED)) != 0 || (sithMain_g_sith_mode.debugModeFlags & SITHDEBUG_AIEVENTS_DISABLED) != 0 || (pLocal->mode & SITHAI_MODE_DISABLED) != 0 || pLocal->pOwner->thingInfo.actorInfo.health <= 0.0f )
//    {
//        return;
//    }
//
//    SithAIMode prevMode = pLocal->mode;
//    if ( (prevMode & SITHAI_MODE_SLEEPING) != 0 )
//    {
//        if ( event != SITHAI_EVENT_SOUND )
//        {
//            return;
//        }
//        pLocal->mode &= ~SITHAI_MODE_SLEEPING; // 0x1000 - SITHAI_MODE_SLEEPING
//    }
//
//    if ( (size_t)event <= SITHAI_EVENT_HIT_WALL )
//    {
//        switch ( event )
//        {
//            case SITHAI_EVENT_HIT_WALL:
//            case SITHAI_EVENT_HIT_SECTOR:
//                if ( pObject )
//                {
//                    cogParam = *((uint32_t*)pObject + 1);
//                }
//                bSendCogEvent = true;
//                break;
//            case SITHAI_EVENT_TOUCHED:
//                if ( pObject )
//                {
//                    cogParam = *((uint32_t*)pObject + 1);
//                    bSendCogEvent = true;
//                }
//                break;
//            default:
//                cogParam = (int)pObject;
//                bSendCogEvent = true;
//                break;
//        }
//    }
//    else
//    {
//        switch ( event )
//        {
//            case SITHAI_EVENT_HIT_FLOOR:
//            case SITHAI_EVENT_LAND_FLOOR:
//                if ( pObject )
//                {
//                    surfIdx = sithSurface_GetSurfaceIndex((const SithSurface*)pObject);
//                    if ( surfIdx > -1 )
//                    {
//                        cogParam = surfIdx;
//                        bSendCogEvent = true;
//                    }
//                }
//                break;
//            case SITHAI_EVENT_FIRE:
//            case SITHAI_EVENT_TARGETED:
//                if ( pObject )
//                {
//                    cogParam = *((uint32_t*)pObject + 1);
//                    bSendCogEvent = true;
//                }
//                break;
//            default:
//                cogParam = (int)pObject;
//                bSendCogEvent = true;
//                break;
//        }
//    }
//
//    if ( bSendCogEvent )
//    {
//        cogRes = sithCog_ThingSendMessageEx(pLocal->pOwner, 0, SITHCOG_MSG_AIEVENT, event, pLocal->mode, cogParam, 0);
//    }
//
//    if ( !cogRes && (pLocal->mode & SITHAI_MODE_BLOCK) == 0 && !sithAIMove_AIGetMoveState(pLocal) )
//    {
//        for ( size_t i = 0; i < pLocal->numInstincts; ++i )
//        {
//            if ( (pLocal->aInstinctStates[i].flags & SITHAI_INSTINCT_DISABLED) == 0 && (event & pLocal->pClass->aInstincts[i].triggerEvents) != 0 )
//            {
//                instinctRes = pLocal->pClass->aInstincts[i].pfInstinct(pLocal, &pLocal->pClass->aInstincts[i], &pLocal->aInstinctStates[i], event, pObject);
//                if ( instinctRes )
//                {
//                    break;
//                }
//            }
//        }
//    }
//
//    if ( !cogRes && !instinctRes )
//    {
//        if ( (pLocal->mode & SITHAI_MODE_TRAVERSEWPNTS) != 0 )
//        {
//            instinctRes = sithAI_ProcessUnhandledWpntEvent(pLocal, event);
//        }
//
//        if ( !instinctRes )
//        {
//            if ( (pLocal->mode & SITHAI_MODE_BLOCK) != 0 || (pLocal->pOwner->thingInfo.actorInfo.flags & SITH_AF_NOSLOPEMOVE) != 0 )
//            {
//                sithAI_ProcessBlockedEvent(pLocal, event);
//            }
//            else
//            {
//                sithAI_ProcessUnhandledEvent(pLocal, event, pObject);
//            }
//        }
//    }
//
//    if ( pLocal->mode != prevMode )
//    {
//        sithAI_EmitEvent(pLocal, SITHAI_EVENT_MODECHANGED, (void*)prevMode);
//    }
//}

void J3DAPI sithAI_EmitEvent(SithAIControlBlock* pLocal, SithAIEventType event, void* pObject)
{
    if ( !pLocal->pClass
        || !pLocal->pOwner
        || pLocal->pOwner->type == SITH_THING_FREE // Fixed: Added check for free thing. TODO: Research why thiswould be the case
        || (pLocal->pOwner->flags & (SITH_TF_DISABLED | SITH_TF_DYING | SITH_TF_DESTROYED)) != 0
        || (sithMain_g_sith_mode.debugModeFlags & SITHDEBUG_AIEVENTS_DISABLED) != 0
        || (pLocal->mode & SITHAI_MODE_DISABLED) != 0
        || pLocal->pOwner->thingInfo.actorInfo.health <= 0.0f )
    {
        return;
    }

    SithAIMode prevMode = pLocal->mode;
    if ( (prevMode & SITHAI_MODE_SLEEPING) != 0 )
    {
        if ( event != SITHAI_EVENT_SOUND )
        {
            return;
        }

        pLocal->mode &= ~SITHAI_MODE_SLEEPING;
    }

    bool bSendCogEvent = false;
    int cogParam = 0;
    switch ( event )
    {
        case (SithAIEventType)0: break; // TODO: 0 could be event that just instinct update 
        case SITHAI_EVENT_TOUCHED:
        case SITHAI_EVENT_FIRE:
        case SITHAI_EVENT_TARGETED:
            if ( pObject )
            {
                cogParam = ((const SithThing*)pObject)->idx; // Was *((uint32_t*)pObject + 1)
                bSendCogEvent = true;
            } break;
        case SITHAI_EVENT_HIT_SECTOR:
            if ( pObject )
            {
                cogParam = ((const SithThing*)pObject)->idx; // Was *((uint32_t*)pObject + 1)
            }
            bSendCogEvent = true;
            break;
        case SITHAI_EVENT_HIT_WALL:
        case SITHAI_EVENT_HIT_FLOOR:
        case SITHAI_EVENT_LAND_FLOOR:
            if ( pObject )
            {
                int surfIdx = sithSurface_GetSurfaceIndex((const SithSurface*)pObject);
                if ( surfIdx > -1 )
                {
                    cogParam = surfIdx;
                    bSendCogEvent = true;
                }
            }
            break;
        default:
            cogParam = (int)pObject;
            bSendCogEvent = true;
            break;
    };

    int cogRes = 0;
    if ( bSendCogEvent )
    {
        cogRes = sithCog_ThingSendMessageEx(pLocal->pOwner, 0, SITHCOG_MSG_AIEVENT, event, pLocal->mode, cogParam, 0);
    }

    int instinctRes = 0;
    if ( !cogRes && (pLocal->mode & SITHAI_MODE_BLOCK) == 0 && !sithAIMove_AIGetMoveState(pLocal) )
    {
        for ( size_t i = 0; i < pLocal->numInstincts; ++i )
        {
            if ( (pLocal->aInstinctStates[i].flags & SITHAI_INSTINCT_DISABLED) == 0 && (event & pLocal->pClass->aInstincts[i].triggerEvents) != 0 )
            {
                instinctRes = pLocal->pClass->aInstincts[i].pfInstinct(pLocal, &pLocal->pClass->aInstincts[i], &pLocal->aInstinctStates[i], event, pObject);
                if ( instinctRes )
                {
                    break;
                }
            }
        }
    }

    if ( !cogRes && !instinctRes )
    {
        if ( (pLocal->mode & SITHAI_MODE_TRAVERSEWPNTS) != 0 )
        {
            instinctRes = sithAI_ProcessUnhandledWpntEvent(pLocal, event);
        }

        if ( !instinctRes )
        {
            if ( (pLocal->mode & SITHAI_MODE_BLOCK) != 0 || (pLocal->pOwner->thingInfo.actorInfo.flags & SITH_AF_NOSLOPEMOVE) != 0 )
            {
                sithAI_ProcessBlockedEvent(pLocal, event);
            }
            else
            {
                sithAI_ProcessUnhandledEvent(pLocal, event, pObject);
            }
        }
    }

    if ( pLocal->mode != prevMode )
    {
        sithAI_EmitEvent(pLocal, SITHAI_EVENT_MODECHANGED, (void*)prevMode);
    }
}

int J3DAPI sithAI_ProcessBlockedEvent(SithAIControlBlock* pLocal, SithAIEventType aievent)
{
    SITH_ASSERTREL(pLocal && pLocal->pOwner);

    if ( aievent <= SITHAI_EVENT_HIT_WALL )
    {
        if ( aievent == SITHAI_EVENT_HIT_WALL )
        {
            sithAIMove_Unreachable(pLocal);
            return 1;
        }

        if ( aievent != SITHAI_EVENT_TOUCHED )
        {
            return 0;
        }
    }
    else
    {
        if ( aievent == SITHAI_EVENT_HIT_CLIFF )
        {
            sithAIMove_Unreachable(pLocal);
            return 1;
        }

        if ( aievent != SITHAI_EVENT_HIT_THING )
        {
            return aievent == SITHAI_EVENT_HIT_FLOOR;
        }
    }

    if ( (pLocal->mode & SITHAI_MODE_MOVING) == 0 )
    {
        return 0;
    }

    sithAIMove_Unreachable(pLocal);
    return 1;
}

int J3DAPI sithAI_ProcessUnhandledEvent(SithAIControlBlock* pLocal, SithAIEventType event, void* pObject)
{
    SITH_ASSERTREL(pLocal);
    SITH_ASSERTREL(pLocal->pOwner && pLocal->pClass);

    switch ( event )
    {
        case SITHAI_EVENT_HIT_FLOOR:
            // Fixed: Notify underwater thing that path is not reachable due to hit floor surface
            //        This fixes e.g.: sharks getting head stuck in the floor surface in lagoon level
            if ( pLocal->pOwner->pInSector && (pLocal->pOwner->pInSector->flags & SITH_SECTOR_UNDERWATER) != 0 )
            {
                sithAIMove_Unreachable(pLocal);
                return 0;
            }

            return 1;
        case SITHAI_EVENT_HIT_CLIFF:
        {
            if ( SITH_RANDF() <= pLocal->pClass->rank )
            {
                sithAIMove_StopAIMovement(pLocal);

                rdVector3 heading;
                sithAIUtil_GetXYHeadingVector(pLocal->pOwner, &heading);

            #ifdef J3D_QOL_IMPROVEMENTS
                // Move in the same direction
                int pathFlags = 0xC002; // 0x2 - yaw | 0x4000 - random | 0x8000 - rand 180 deg dir flip
            #else
                // Move in the opposite direction
                rdVector_Neg3Acc(&heading); // TODO: Note, commenting out the this line will make AI not to move back and forth
                sithAIUtil_ApplyForce(pLocal, &heading, 0.05f);

                int pathFlags = 0xC002; // 0x2 - yaw | 0x4000 - random | 0x8000 - rand neg 
            #endif

                if ( (pLocal->mode & SITHAI_MODE_ACTIVE) != 0 )
                {
                    pathFlags |= 0x100;
                }

                float minDist = 0.089999996f;
                if ( pLocal->pOwner->collide.movesize > minDist )
                {
                    minDist = pLocal->pOwner->collide.movesize;
                }

                const float moveDist = (float)(SITH_RAND() * 0.2 + minDist);

                rdVector3 newPos;
                if ( sithAIUtil_MakePathPos(pLocal, &pLocal->pOwner->pos, minDist, moveDist, 60.0f, pathFlags, &heading, &newPos) )
                {
                    sithAIMove_AISetLookPosEyeLevel(pLocal, &newPos);
                    sithAIMove_AISetMoveTargetPos(pLocal, &newPos, 1.0f);
                    return 1;
                }
            }

            sithAIMove_Unreachable(pLocal);
            return 0;
        }
        case SITHAI_EVENT_GOAL_REACHED:
        {
            if ( (pLocal->mode & SITHAI_MODE_CHASE_GOAL) == 0 )
            {
                return 0;
            }

            rdVector3 heading;
            sithAIUtil_GetXYHeadingVector(pLocal->pOwner, &heading);

            float minDist  = pLocal->pOwner->collide.movesize <= 0.089999996f ? 0.089999996f : pLocal->pOwner->collide.movesize;
            float moveDist = minDist + 0.2f;

            rdVector3 newPos;
            if ( sithAIUtil_MakePathPos(pLocal, &pLocal->pOwner->pos, minDist, moveDist, 45.0f, 0xC102, &heading, &newPos) )
            {
                sithAIMove_AISetLookPosEyeLevel(pLocal, &newPos);
                sithAIMove_AISetMovePos(pLocal, &newPos, 1.7f);
                return 1;
            }

            return 0;
        }
        case SITHAI_EVENT_HIT_WALL:
        case SITHAI_EVENT_HIT_THING:
        {
            sithAIMove_Unreachable(pLocal);
            return 1;
        }
        case SITHAI_EVENT_MODECHANGED:
        {
            if ( (pLocal->mode & SITHAI_MODE_CHASE_GOAL) == 0 || ((SithAIMode)pObject & SITHAI_MODE_CHASE_GOAL) != 0 )
            {
                return 0;
            }

            if ( (pLocal->mode & SITHAI_MODE_NOCHASING) != 0 && pLocal->targetDistance > (double)pLocal->pClass->sightDistance )
            {
                sithAIMove_StopAIMovement(pLocal);
                sithAIUtil_AISetMode(pLocal, SITHAI_MODE_SEARCHING);
                return 1;
            }

            if ( (pLocal->submode & SITHAI_SUBMODE_CONTINUOUSWPNTMOTION) != 0 )
            {
                return 1;
            }

            sithAIMove_AISetMovePos(pLocal, &pLocal->vecUnknown5, 1.7f);
            sithAIMove_AISetLookPosEyeLevel(pLocal, &pLocal->vecUnknown5);
            return 0;
        }
        case SITHAI_EVENT_TOUCHED:
        {
            if ( (pLocal->mode & SITHAI_MODE_MOVING) == 0 )
            {
                return 0;
            }

            if ( pObject )
            {
                rdVector3 heading;
                sithAIUtil_GetXYHeadingVector(pLocal->pOwner, &heading);

                rdVector3 touchDir;
                touchDir.x = pLocal->pOwner->pos.x - ((const SithThing*)pObject)->pos.x; // heading.x = pLocal->pOwner->pos.x - *((float *)pObject + 0x34);
                touchDir.y = pLocal->pOwner->pos.y - ((const SithThing*)pObject)->pos.y; // heading.y = pLocal->pOwner->pos.y - *((float *)pObject + 0x35
                touchDir.z = 0.0f;
                rdVector_Normalize3Acc(&touchDir);
                if ( rdVector_Dot3(&heading, &touchDir) >= 0.0f )
                {
                    return 1;
                }
            }

            sithAIMove_Unreachable(pLocal);
            return 1;
        };
    };

    return 0;
}

int J3DAPI sithAI_ProcessUnhandledWpntEvent(SithAIControlBlock* pLocal, SithAIEventType event)
{
    if ( event != SITHAI_EVENT_TOUCHED && event != SITHAI_EVENT_HIT_WALL && event != SITHAI_EVENT_HIT_CLIFF )
    {
        return 0;
    }

    sithAI_EmitEvent(pLocal, SITHAI_EVENT_GOAL_UNREACHABLE, 0);
    if ( sithAIUtil_AIMoveToNextWpnt(pLocal, pLocal->moveSpeed, 0.0f, 0) )
    {
    #ifdef J3D_QOL_IMPROVEMENTS
        // The following code can resolves the NPC movement deadlock that occurs when they face each
        if ( event == SITHAI_EVENT_TOUCHED )
        {
            // TODO: Maybe add check in which direction the AI is facing touched thing
            rdVector3 heading;
            sithAIUtil_GetXYHeadingVector(pLocal->pOwner, &heading);

            float minDist  = pLocal->pOwner->collide.movesize <= 0.089999996f ? 0.089999996f : pLocal->pOwner->collide.movesize;
            float moveDist = minDist + 0.05f;

            rdVector3 newPos;
            if ( sithAIUtil_MakePathPos(pLocal, &pLocal->pOwner->pos, minDist, moveDist, 45.0f, 0xC102, &heading, &newPos) )
            {
                sithAIMove_AISetLookPosEyeLevel(pLocal, &newPos);
                sithAIMove_AISetMovePos(pLocal, &newPos, 1.7f);
                return 1;
            }
        }
    #endif

        return 1;
    }

    return 0;
}

void J3DAPI sithAI_Stop(SithThing* pThing)
{
    if ( pThing )
    {
        SithAIControlBlock* pLocal = pThing->controlInfo.aiControl.pLocal;
        if ( pLocal )
        {
            sithAIMove_StopAIMovement(pLocal);
            sithAIUtil_AISetMode(pLocal, SITHAI_MODE_SEARCHING);
            sithAIUtil_ClearAIWaypoint(pLocal);
        }
    }
}

void sithAI_StopAllAttackingAIs(void)
{
    for ( int aiIdx = 0; aiIdx <= sithAI_g_lastUsedAIIndex; ++aiIdx )
    {
        SithAIControlBlock* pLocal = &sithAI_g_aControlBlocks[aiIdx];
        if ( pLocal
            && pLocal->pOwner
            && pLocal->pOwner->controlType == SITH_CT_AI
            && (pLocal->pOwner->flags & (SITH_TF_DISABLED | SITH_TF_DYING | SITH_TF_DESTROYED)) == 0
            && (pLocal->mode & SITHAI_MODE_ATTACKING) != 0 )
        {
            sithAIMove_AIStop(pLocal);
            sithAIUtil_AISetMode(pLocal, SITHAI_MODE_SEARCHING);
        }
    }
}

int J3DAPI sithAI_RegisterInstinct(const char* pName, SithAIInstinctFunc pfInstinct, SithAIMode updateModes, SithAIMode updateBlockModes, SithAIEventType triggerEvents)
{
    SITH_ASSERTREL(pName);
    SITH_ASSERTREL(pfInstinct);

    if ( numRegisteredInstincts >= SITHAI_MAXREGISTEREDINSTINCTS )
    {
        return 0;
    }

    STD_STRCPY(aRegisteredInstincts[numRegisteredInstincts].aName, pName);
    aRegisteredInstincts[numRegisteredInstincts].pfInstinct       = pfInstinct;
    aRegisteredInstincts[numRegisteredInstincts].updateModes      = updateModes;// jones3D engine (ref dbg ver): mode
    aRegisteredInstincts[numRegisteredInstincts].updateBlockModes = updateBlockModes;
    aRegisteredInstincts[numRegisteredInstincts].triggerEvents    = triggerEvents;// jones3D engine (ref dbg ver): mask
    return ++numRegisteredInstincts;
}

SithAIRegisteredInstinct* J3DAPI sithAI_FindInstinct(const char* pInstinctName)
{
    if ( !pInstinctName )
    {
        SITHLOG_ERROR("Null parameter passed to FindInstinct().\n");
        return 0;
    }

    for ( size_t i = 0; i < numRegisteredInstincts; ++i )
    {
        if ( stdUtil_StrCmp(pInstinctName, aRegisteredInstincts[i].aName) == 0 )
        {
            return &aRegisteredInstincts[i];
        }
    }

    return 0;
}

int J3DAPI sithAI_GetInstinctIndex(const SithAIControlBlock* pLocal, const SithAIRegisteredInstinct* pRegInstinct)
{
    size_t i;
    for ( i = 0; i < pLocal->numInstincts && pLocal->pClass->aInstincts[i].pfInstinct != pRegInstinct->pfInstinct; ++i )
    {
        ;
    }

    return i < pLocal->numInstincts ? i : -1;
}

int J3DAPI sithAI_HasInstinct(const SithAIControlBlock* pLocal, const char* pName)
{

    SITH_ASSERTREL(pLocal);
    SithAIRegisteredInstinct* pInstinct = sithAI_FindInstinct(pName);
    return pInstinct && sithAI_GetInstinctIndex(pLocal, pInstinct) != -1;
}

int J3DAPI sithAI_EnableInstinct(SithAIControlBlock* pLocal, const char* pInstinctName, int bEnable)
{
    SITH_ASSERTREL(pLocal);

    SithAIRegisteredInstinct* pInstinct = sithAI_FindInstinct(pInstinctName);
    if ( !pInstinct )
    {
        SITHLOG_ERROR("Invalid instinct '%s' passed to EnableInstinct().\n", pInstinctName);
        return 0;
    }

    int instinctIndex = sithAI_GetInstinctIndex(pLocal, pInstinct);
    if ( instinctIndex == -1 )
    {
        if ( bEnable )
        {
            SITHLOG_ERROR("AI '%s' doesn't possess instinct '%s' for EnableInstinct() call.\n", pLocal->pOwner->aName, pInstinctName);
        }
        return 0;
    }

    if ( bEnable )
    {
        pLocal->aInstinctStates[instinctIndex].flags &= ~SITHAI_INSTINCT_DISABLED;
    }
    else
    {
        pLocal->aInstinctStates[instinctIndex].flags |= SITHAI_INSTINCT_DISABLED;
    }

    return 1;
}

int J3DAPI sithAI_AIList(const SithConsoleCommand* pFunc, const char* pArg)
{
    J3D_UNUSED(pFunc);
    J3D_UNUSED(pArg);

    if ( !sithAI_g_bOpen )
    {
        sithConsole_PrintString("AI system not open.\n");
        return 0;
    }

    sithConsole_PrintString("Active AI things:\n");

    for ( int i = 0; i <= sithAI_g_lastUsedAIIndex; ++i )
    {
        SithAIControlBlock* pLocal = &sithAI_g_aControlBlocks[i];
        if ( pLocal->pClass && pLocal->pOwner )
        {
            SITHCONSOLE_PRINTF(
                "Block %2d: Class '%s', Owner '%s' (%d), Flags 0x%x\n",
                i,
                pLocal->pClass->aName,
                pLocal->pOwner->aName,
                pLocal->pOwner->idx,
                pLocal->mode
            );
        }
    }

    return 1;
}

int J3DAPI sithAI_AIStatus(const SithConsoleCommand* pFunc, const char* pArg)
{
    J3D_UNUSED(pFunc);

    int idx;
    if ( !pArg || !sithAI_g_bOpen || sscanf_s(pArg, "%d", &idx) != 1 || idx > sithAI_g_lastUsedAIIndex )
    {
        sithConsole_PrintString("Cannot process AIStatus command.\n");
        return 0;
    }

    SithAIControlBlock* pLocal = &sithAI_g_aControlBlocks[idx];
    if ( !pLocal->pOwner )
    {
        sithConsole_PrintString("That AI block is not currently active.\n");
        return 1;
    }

    SITHCONSOLE_PRINTF("AI Status dump for thing %d (%s).\n", pLocal->pOwner->idx, pLocal->pOwner->aName);
    SITHCONSOLE_PRINTF(
        "Class '%s', Flags=0x%x, Moods %d/%d/%d, NextUpdate=%d\n",
        pLocal->pClass->aName,
        pLocal->mode,
        pLocal->aMoods[0],
        pLocal->aMoods[1],
        pLocal->aMoods[2],
        pLocal->msecNextUpdate
    );

    sithConsole_PrintString("Current instincts:\n");
    for ( size_t i = 0; i < pLocal->numInstincts; ++i )
    {
        SITHCONSOLE_PRINTF(
            "Instinct %d: Params: %f/%f/%f/%f, nextUpdate=%d, mask=0x%x, mode=0x%x.\n",
            i,
            pLocal->aInstinctStates[i].aParams[0],
            pLocal->aInstinctStates[i].aParams[1],
            pLocal->aInstinctStates[i].aParams[2],
            pLocal->aInstinctStates[i].aParams[3],
            pLocal->aInstinctStates[i].msecNextUpdate,
            pLocal->pClass->aInstincts[i].triggerEvents,
            pLocal->pClass->aInstincts[i].updateModes
        );
    }

    return 1;
}

int J3DAPI sithAI_ParseArg(const StdConffileArg* pArg, SithThing* pThing, int adjNum)
{
    SITH_ASSERTREL(pArg && pThing); // Moved here to the top

    SithAIControlBlock* pLocal = pThing->controlInfo.aiControl.pLocal;
    if ( !pLocal )
    {
        goto argument_error;
    }

    switch ( adjNum )
    {
        case SITHTHING_ARG_FRAME:
        {
            if ( pLocal->numFrames >= pLocal->sizeFrames )
            {
                SITHLOG_ERROR("Too many AI frames for thing %d.\n", pThing->idx);
            }
            else
            {
                rdVector3 framePos;
                if ( sscanf_s(pArg->argValue, "(%f/%f/%f)", &framePos.x, &framePos.y, &framePos.z) == 3 )
                {
                    rdVector_Copy3(&pLocal->aFrames[pLocal->numFrames], &framePos);
                    ++pLocal->numFrames;
                    return 1;
                }
            }

            goto argument_error;
        }
        case SITHTHING_ARG_NUMFRAMES:
        {
            if ( pLocal->sizeFrames )
            {
                goto argument_error;
            }

            SITH_ASSERTREL(!pLocal->aFrames);
            size_t size = atoi(pArg->argValue);
            if ( size == 0 )
            {
                goto argument_error;
            }

            if ( sithAI_AllocAIFrames(pLocal, size) )
            {
                SITHLOG_ERROR("Memory allocation error, line %d.\n", stdConffile_GetLineNumber());
                return 0;
            }

            // Success
            return 1;
        }
        default:
            return 0;
    };

argument_error:
    SITHLOG_ERROR("Bad argument %s=%s line %d.\n", pArg->argName, pArg->argValue, stdConffile_GetLineNumber());
    return 0;
}

int J3DAPI sithAI_AllocAIFrames(SithAIControlBlock* pLocal, size_t sizeFrames)
{
    pLocal->aFrames = (rdVector3*)STDMALLOC(sizeof(rdVector3) * sizeFrames);
    if ( !pLocal->aFrames )
    {
        return 1;
    }

    STD_ZEROMEM(pLocal->aFrames, sizeof(rdVector3) * sizeFrames);
    pLocal->sizeFrames = sizeFrames;
    pLocal->numFrames  = 0;
    return 0;
}

void J3DAPI sithAI_CreateAIFramesFomMarker(SithThing* pNewThing, const SithThing* pMarker, const rdVector3* pivot)
{
    SITH_ASSERTREL(pNewThing && pMarker);
    SITH_ASSERTREL((pNewThing->controlType == SITH_CT_AI) && (pMarker->moveType == SITH_MT_PATH));

    SithAIControlBlock* pLocal = pNewThing->controlInfo.aiControl.pLocal;
    SITH_ASSERTREL(pLocal);
    SITH_ASSERTREL(!pLocal->aFrames);

    pLocal->aFrames    = (rdVector3*)STDMALLOC(sizeof(rdVector3) * pMarker->moveInfo.pathMovement.sizeFrames);
    pLocal->sizeFrames = pMarker->moveInfo.pathMovement.sizeFrames;
    pLocal->numFrames  = pMarker->moveInfo.pathMovement.numFrames;

    for ( size_t i = 0; i < pLocal->numFrames; ++i )
    {
        SithPathFrame* pFrame= &pMarker->moveInfo.pathMovement.aFrames[i];

        rdVector3 vec;
        rdVector_Rotate3(&vec, pivot, &pFrame->pyr);

        pLocal->aFrames[i].x = pFrame->pos.x + vec.x;
        pLocal->aFrames[i].y = pFrame->pos.y + vec.y;
        pLocal->aFrames[i].z = pFrame->pos.z + vec.z;
    }
}

int J3DAPI sithAI_CreateInstinctRegistry(size_t maxInstincts)
{
    SITH_ASSERTREL(aRegisteredInstincts == NULL);
    aRegisteredInstincts = (SithAIRegisteredInstinct*)STDMALLOC(sizeof(SithAIRegisteredInstinct) * maxInstincts);
    if ( !aRegisteredInstincts )
    {
        SITHLOG_ERROR("Startup failure on AI - no mem for instinct registry.\n");
        return 1;
    }

    pRegisteredInstinctHashtbl = stdHashtbl_New(2 * maxInstincts);
    if ( !pRegisteredInstinctHashtbl )
    {
        STDFREE(aRegisteredInstincts);
        SITHLOG_ERROR("Startup failure on AI - no mem for hashtable.\n");
        return 1;
    }

    return 0;
}

int J3DAPI sithAI_FreeAI(size_t aiNum)
{
    SITH_ASSERTREL((aiNum < STD_ARRAYLEN(sithAI_g_aControlBlocks)));
    SITH_ASSERTREL(numFreeAIs < STD_ARRAYLEN(sithAI_g_aControlBlocks));

    STD_ZEROMEM(&sithAI_g_aControlBlocks[aiNum], sizeof(SithAIControlBlock));

    if ( aiNum == sithAI_g_lastUsedAIIndex )
    {
        int i;
        for ( i = (int)aiNum - 1; i >= 0 && !sithAI_g_aControlBlocks[i].pOwner; --i )
        {
            ;
        }

        sithAI_g_lastUsedAIIndex = i;
    }

    aFreeAIIndices[numFreeAIs] = aiNum;
    return ++numFreeAIs;
}

int sithAI_CreateAI(void)
{
    if ( !numFreeAIs )
    {
        SITHLOG_ERROR("Warning: out of object space - Create failure.\n");
        return -1;
    }

    size_t freeAIIndex = aFreeAIIndices[--numFreeAIs];
    SITH_ASSERTREL(freeAIIndex < STD_ARRAYLEN(sithAI_g_aControlBlocks));

    if ( (int)freeAIIndex > sithAI_g_lastUsedAIIndex )
    {
        sithAI_g_lastUsedAIIndex = freeAIIndex;
    }

    return freeAIIndex;
}

void sithAI_ResetAllAIs(void)
{
    STD_ZEROMEM(sithAI_g_aControlBlocks, sizeof(sithAI_g_aControlBlocks));
    numFreeAIs = 0;

    for ( int aiNum = STD_ARRAYLEN(sithAI_g_aControlBlocks) - 1; aiNum >= 0; --aiNum )
    {
        sithAI_FreeAI(aiNum);
    }
}

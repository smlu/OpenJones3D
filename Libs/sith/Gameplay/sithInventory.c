#include "sithInventory.h"
#include <j3dcore/j3dhook.h>

#include <sith/Cog/sithCog.h>
#include <sith/Devices/sithComm.h>
#include <sith/Dss/sithDSSThing.h>
#include <sith/RTI/symbols.h>
#include <sith/World/sithTemplate.h>
#include <sith/World/sithThing.h>

#include <std/General/stdMath.h>
#include <std/General/stdUtil.h>
#include <std/Win95/stdComm.h>

void sithInventory_InstallHooks(void)
{
    J3D_HOOKFUNC(sithInventory_RegisterType);
    J3D_HOOKFUNC(sithInventory_FindNextTypeID);
    J3D_HOOKFUNC(sithInventory_FindPreviousTypeID);
    J3D_HOOKFUNC(sithInventory_GetType);
    J3D_HOOKFUNC(sithInventory_GetCurrentWeapon);
    J3D_HOOKFUNC(sithInventory_SetCurrentWeapon);
    J3D_HOOKFUNC(sithInventory_GetCurrentItem);
    J3D_HOOKFUNC(sithInventory_SetCurrentItem);
    J3D_HOOKFUNC(sithInventory_ChangeInventory);
    J3D_HOOKFUNC(sithInventory_GetInventory);
    J3D_HOOKFUNC(sithInventory_SetInventory);
    J3D_HOOKFUNC(sithInventory_SetInventoryActivated);
    J3D_HOOKFUNC(sithInventory_IsInventoryActivated);
    J3D_HOOKFUNC(sithInventory_SetInventoryAvailable);
    J3D_HOOKFUNC(sithInventory_IsInventoryAvailable);
    J3D_HOOKFUNC(sithInventory_SetInventoryDisabled);
    J3D_HOOKFUNC(sithInventory_IsBackpackItem);
    J3D_HOOKFUNC(sithInventory_IsWeapon);
    J3D_HOOKFUNC(sithInventory_GetInventoryMinimum);
    J3D_HOOKFUNC(sithInventory_GetInventoryMaximum);
    J3D_HOOKFUNC(sithInventory_SetInventoryFlags);
    J3D_HOOKFUNC(sithInventory_BroadcastMessage);
    J3D_HOOKFUNC(sithInventory_ResetInventory);
    J3D_HOOKFUNC(sithInventory_CreateBackpack);
    J3D_HOOKFUNC(sithInventory_PickupBackpack);
    J3D_HOOKFUNC(sithInventory_GetBackpackItemID);
    J3D_HOOKFUNC(sithInventory_GetBackpackItemValue);
    J3D_HOOKFUNC(sithInventory_GetNumBackpackItems);
    J3D_HOOKFUNC(sithInventory_GetInventoryType);
    J3D_HOOKFUNC(sithInventory_InitInventory);
    J3D_HOOKFUNC(sithInventory_BroadcastKilledMessage);
    J3D_HOOKFUNC(sithInventory_SetSwimmingInventory);
}

void sithInventory_ResetGlobals(void)
{
    int sithInventory_g_bResetInventory_tmp = 1;
    STD_COPYMEM(&sithInventory_g_bInitInventory, &sithInventory_g_bResetInventory_tmp, sizeof(sithInventory_g_bInitInventory));

    STD_ZEROMEM(&sithInventory_g_bSendDeactivateMessage, sizeof(sithInventory_g_bSendDeactivateMessage));
    STD_ZEROMEM(&sithInventory_g_dword_56B750, sizeof(sithInventory_g_dword_56B750));
    STD_ZEROMEM(&sithInventory_g_dword_56B754, sizeof(sithInventory_g_dword_56B754));
    STD_ZEROMEM(&sithInventory_g_aTypes, sizeof(sithInventory_g_aTypes));
    STD_ZEROMEM(&sithInventory_g_aPowerKeybinds, sizeof(sithInventory_g_aPowerKeybinds));
}

void J3DAPI sithInventory_RegisterType(size_t typeId, const char* pName, float min, float max, SithInventoryTypeFlag flags, SithCog* pItemCog, rdModel3* pInvModel, rdModel3* pItemModel)
{
    SITH_ASSERTREL(typeId < STD_ARRAYLEN(sithInventory_g_aTypes));
    SITH_ASSERTREL(pName);

    SithInventoryType* pType = &sithInventory_g_aTypes[typeId];
    pType->flags = flags | SITHINVENTORY_TYPE_REGISTERED;

    STD_STRCPY(pType->aName, pName);
    pType->min  = min;
    pType->max  = max;
    pType->pCog = pItemCog;

    if ( pItemCog && ((pType->flags & SITHINVENTORY_TYPE_ITEM) != 0 || (pType->flags & SITHINVENTORY_TYPE_WEAPON) != 0) )
    {
        pItemCog->flags |= SITHCOG_LOCAL;
    }

    pType->pInvIconModel = pInvModel;
    pType->pModel        = pItemModel;
    pType->pHolsterModel = NULL;
}

int J3DAPI sithInventory_FindNextTypeID(SithThing* pThing, int startSearchId, SithInventoryTypeFlag flags)
{
    SITH_ASSERTREL(startSearchId >= 0);

    for ( int typeId = startSearchId + 1; typeId < STD_ARRAYLEN(sithInventory_g_aTypes); ++typeId )
    {
        if ( (flags & sithInventory_g_aTypes[typeId].flags) != 0 && sithInventory_IsInventoryAvailable(pThing, typeId) )
        {
            return typeId;
        }
    }

    for ( int typeId = 0; typeId < startSearchId; ++typeId )
    {
        if ( (flags & sithInventory_g_aTypes[typeId].flags) != 0 && sithInventory_IsInventoryAvailable(pThing, typeId) )
        {
            return typeId;
        }
    }

    return -1;
}

int J3DAPI sithInventory_FindPreviousTypeID(SithThing* pThing, int startSearchId, SithInventoryTypeFlag flags)
{
    SITH_ASSERTREL(startSearchId >= 0);

    for ( int typeId = startSearchId - 1; typeId >= 0; --typeId )
    {
        if ( (flags & sithInventory_g_aTypes[typeId].flags) != 0 && sithInventory_IsInventoryAvailable(pThing, typeId) )
        {
            return typeId;
        }
    }

    for ( int typeId = STD_ARRAYLEN(sithInventory_g_aTypes) - 1; typeId > startSearchId; --typeId )
    {
        if ( (flags & sithInventory_g_aTypes[typeId].flags) != 0 && sithInventory_IsInventoryAvailable(pThing, typeId) )
        {
            return typeId;
        }
    }

    return -1;
}

int J3DAPI sithInventory_FindNextItemID(SithThing* pThing, int itemId)
{
    SITH_ASSERTREL(pThing->type == SITH_THING_PLAYER);
    SITH_ASSERTREL(pThing->thingInfo.actorInfo.pPlayer);
    if ( itemId < 0 )
    {
        itemId = pThing->thingInfo.actorInfo.pPlayer->curItemID;
    }

    return sithInventory_FindNextTypeID(pThing, itemId, SITHINVENTORY_TYPE_ITEM);
}

int J3DAPI sithInventory_FindPreviousItemID(SithThing* pThing, int itemId)
{
    SITH_ASSERTREL(pThing->type == SITH_THING_PLAYER);
    SITH_ASSERTREL(pThing->thingInfo.actorInfo.pPlayer);
    if ( itemId < 0 )
    {
        itemId = pThing->thingInfo.actorInfo.pPlayer->curItemID;
    }

    return sithInventory_FindPreviousTypeID(pThing, itemId, SITHINVENTORY_TYPE_ITEM);
}

void J3DAPI sithInventory_SelectItem(SithThing* pThing, size_t typeId)
{
    SITH_ASSERTREL(pThing);

    if ( (typeId & 0x80000000) == 0 && sithInventory_IsInventoryAvailable(pThing, typeId) )
    {
        SithPlayer* pPlayer = pThing->thingInfo.actorInfo.pPlayer;
        int curItemId = pPlayer->curItemID;

        SithInventoryType* pType = sithInventory_GetType(curItemId);
        if ( pType->pCog )
        {
            sithCog_SendMessage(pType->pCog, SITHCOG_MSG_DESELECTED, SITHCOG_SYM_REF_INT, curItemId, SITHCOG_SYM_REF_THING, pThing->idx, 0);
            if ( sithInventory_g_bSendDeactivateMessage == 1 )
            {
                sithCog_SendMessage(pType->pCog, SITHCOG_MSG_DEACTIVATED, SITHCOG_SYM_REF_INT, curItemId, SITHCOG_SYM_REF_THING, pThing->idx, 0);
                sithInventory_g_bSendDeactivateMessage = 0;
            }
        }

        pType = sithInventory_GetType(typeId);
        if ( pType->pCog )
        {
            sithCog_SendMessage(pType->pCog, SITHCOG_MSG_SELECTED, SITHCOG_SYM_REF_INT, typeId, SITHCOG_SYM_REF_THING, pThing->idx, 0);
        }

        pPlayer->curItemID = typeId;
    }
}

void J3DAPI sithInventory_SelectNextItem(SithThing* pThing)
{
    SITH_ASSERTREL(pThing->type == SITH_THING_PLAYER);
    SITH_ASSERTREL(pThing->thingInfo.actorInfo.pPlayer);

    int itemId = sithInventory_FindNextItemID(pThing, -1);
    sithInventory_SelectItem(pThing, itemId);
}

void J3DAPI sithInventory_SelectPreviousItem(SithThing* pThing)
{
    SITH_ASSERTREL(pThing->type == SITH_THING_PLAYER);
    SITH_ASSERTREL(pThing->thingInfo.actorInfo.pPlayer);
    int itemId = sithInventory_FindPreviousItemID(pThing, -1);
    sithInventory_SelectItem(pThing, itemId);
}

SithInventoryType* J3DAPI sithInventory_GetType(size_t typeId)
{
    SITH_ASSERTREL(typeId < STD_ARRAYLEN(sithInventory_g_aTypes)); // Added: Add bounds check
    return &sithInventory_g_aTypes[typeId];
}

int J3DAPI sithInventory_GetCurrentWeapon(const SithThing* pThing)
{
    SITH_ASSERTREL((pThing->type == SITH_THING_PLAYER) || (pThing->type == SITH_THING_ACTOR));
    if ( pThing->type == SITH_THING_ACTOR && pThing->controlInfo.aiControl.pLocal )
    {
        return pThing->thingInfo.actorInfo.weaponInfo.curWeaponID;
    }

    if ( pThing->type != SITH_THING_PLAYER )
    {
        return SITHWEAPON_NO_WEAPON;
    }

    SITH_ASSERTREL(pThing->thingInfo.actorInfo.pPlayer);
    return pThing->thingInfo.actorInfo.pPlayer->curWeaponID;
}

void J3DAPI sithInventory_SetCurrentWeapon(SithThing* pThing, SithWeaponId weaponID)
{
    SITH_ASSERTREL(pThing);
    SITH_ASSERTREL(pThing->type == SITH_THING_PLAYER);
    SITH_ASSERTREL(pThing->thingInfo.actorInfo.pPlayer);

    SithPlayer* pPlayer = pThing->thingInfo.actorInfo.pPlayer;
    pThing->thingInfo.actorInfo.pPlayer->curWeaponID = weaponID;

    switch ( weaponID )
    {
        case SITHWEAPON_PISTOL:
        case SITHWEAPON_TOKAREV:
        case SITHWEAPON_MAUSER:
            pPlayer->lastPistolID = weaponID;
            break;

        case SITHWEAPON_SIMONOV:
        case SITHWEAPON_MACHETE:
        case SITHWEAPON_SUBMACHINE:
        case SITHWEAPON_SHOTGUN:
        case SITHWEAPON_BAZOOKA:
            pPlayer->lastRifleID = weaponID;
            break;

        default:
            return;
    }
}

int J3DAPI sithInventory_GetCurrentItem(SithThing* pThing)
{
    SITH_ASSERTREL(pThing);
    SITH_ASSERTREL(pThing->type == SITH_THING_PLAYER);
    SITH_ASSERTREL(pThing->thingInfo.actorInfo.pPlayer);
    return pThing->thingInfo.actorInfo.pPlayer->curItemID;
}

void J3DAPI sithInventory_SetCurrentItem(SithThing* pThing, int typeId)
{
    SITH_ASSERTREL(pThing->type == SITH_THING_PLAYER);
    SITH_ASSERTREL(pThing->thingInfo.actorInfo.pPlayer);
    pThing->thingInfo.actorInfo.pPlayer->curItemID = typeId;
}

float J3DAPI sithInventory_ChangeInventory(SithThing* pThing, size_t typeId, float amount)
{
    SITH_ASSERTREL(pThing->type == SITH_THING_PLAYER);
    SITH_ASSERTREL(typeId < STD_ARRAYLEN(sithInventory_g_aTypes));
    SITH_ASSERTREL(pThing->thingInfo.actorInfo.pPlayer);

    SithInventoryType* pType = &sithInventory_g_aTypes[typeId];
    if ( pThing->thingInfo.actorInfo.pPlayer->aItems == NULL || (pType->flags & SITHINVENTORY_TYPE_REGISTERED) == 0 ) // TODO: pThing->thingInfo.actorInfo.pPlayer->aItems == NULL ???
    {
        return 0.0f;
    }

    SithInventoryItem* pItem = &pThing->thingInfo.actorInfo.pPlayer->aItems[typeId];

    float prevAmount = pItem->amount;
    pItem->amount = STDMATH_CLAMP(pItem->amount + amount, pType->min, pType->max);

    if ( prevAmount == pItem->amount )
    {
        return pItem->amount;
    }

    if ( pType->pCog )
    {
        sithCog_SendMessage(pType->pCog, SITHCOG_MSG_CHANGED, SITHCOG_SYM_REF_NONE, 0, SITHCOG_SYM_REF_THING, pThing->idx, typeId);
    }

    pItem->status |= SITHINVENTORY_ITEM_CHANGED;
    if ( (pItem->status & SITHINVENTORY_ITEM_FOUND) == 0 )
    {
        pItem->status |= SITHINVENTORY_ITEM_FOUND;
    }

    if ( pItem->amount == 0.0f )
    {
        pItem->status &= ~SITHINVENTORY_ITEM_AVAILABLE;
    }

    return pItem->amount;
}

float J3DAPI sithInventory_GetInventory(SithThing* pThing, size_t typeId)
{
    SITH_ASSERTREL(pThing->type == SITH_THING_PLAYER);
    SITH_ASSERTREL(typeId < STD_ARRAYLEN(sithInventory_g_aTypes));
    SITH_ASSERTREL(pThing->thingInfo.actorInfo.pPlayer);

    if ( pThing->thingInfo.actorInfo.pPlayer->aItems != NULL && (sithInventory_g_aTypes[typeId].flags & SITHINVENTORY_TYPE_REGISTERED) != 0 ) // TODO: pThing->thingInfo.actorInfo.pPlayer->aItems != NULL ???
    {
        return pThing->thingInfo.actorInfo.pPlayer->aItems[typeId].amount;
    }

    return 0.0f;
}

float J3DAPI sithInventory_SetInventory(SithThing* pThing, size_t typeId, float amount)
{
    SITH_ASSERTREL(pThing->type == SITH_THING_PLAYER);
    SITH_ASSERTREL(typeId < STD_ARRAYLEN(sithInventory_g_aTypes));
    SITH_ASSERTREL(pThing->thingInfo.actorInfo.pPlayer);

    SithInventoryType* pType = &sithInventory_g_aTypes[typeId];
    if ( pThing->thingInfo.actorInfo.pPlayer->aItems == NULL || (pType->flags & SITHINVENTORY_TYPE_REGISTERED) == 0 )// TODO: pThing->thingInfo.actorInfo.pPlayer->aItems == NULL ???
    {
        return 0.0f;
    }

    SithInventoryItem* pItem = &pThing->thingInfo.actorInfo.pPlayer->aItems[typeId];

    float prevAmount = pItem->amount;
    pItem->amount = STDMATH_CLAMP(amount, pType->min, pType->max); ;

    if ( prevAmount == pItem->amount )
    {
        return pItem->amount;
    }

    if ( pType->pCog )
    {
        sithCog_SendMessage(pType->pCog, SITHCOG_MSG_CHANGED, SITHCOG_SYM_REF_NONE, 0, SITHCOG_SYM_REF_THING, pThing->idx, typeId);
    }

    pItem->status |= SITHINVENTORY_ITEM_CHANGED;
    if ( (pItem->status & SITHINVENTORY_ITEM_FOUND) == 0 )
    {
        pItem->status |= SITHINVENTORY_ITEM_FOUND;
    }

    if ( pItem->amount == 0.0f )
    {
        pItem->status &= ~SITHINVENTORY_ITEM_AVAILABLE;
    }

    return pItem->amount;
}

void J3DAPI sithInventory_SetInventoryActivated(SithThing* pThing, size_t typeId, int bActivated)
{
    SITH_ASSERTREL(pThing->type == SITH_THING_PLAYER);
    SITH_ASSERTREL(typeId < STD_ARRAYLEN(sithInventory_g_aTypes));
    SITH_ASSERTREL(pThing->thingInfo.actorInfo.pPlayer);

    if ( pThing->thingInfo.actorInfo.pPlayer->aItems != NULL && (sithInventory_g_aTypes[typeId].flags & SITHINVENTORY_TYPE_REGISTERED) != 0 ) // TODO: pThing->thingInfo.actorInfo.pPlayer->aItems != NULL ???
    {
        SithInventoryItem* pItem = &pThing->thingInfo.actorInfo.pPlayer->aItems[typeId];
        if ( bActivated )
        {
            pItem->status |= SITHINVENTORY_ITEM_ACTIVATED;
        }
        else
        {
            pItem->status &= ~SITHINVENTORY_ITEM_ACTIVATED;
        }
    }
}

int J3DAPI sithInventory_IsInventoryActivated(SithThing* pThing, size_t typeId)
{
    SITH_ASSERTREL(pThing->type == SITH_THING_PLAYER);
    SITH_ASSERTREL(typeId < STD_ARRAYLEN(sithInventory_g_aTypes));
    SITH_ASSERTREL(pThing->thingInfo.actorInfo.pPlayer);
    return pThing->thingInfo.actorInfo.pPlayer->aItems != NULL  // TODO: pThing->thingInfo.actorInfo.pPlayer->aItems != NULL ???
        && (sithInventory_g_aTypes[typeId].flags & SITHINVENTORY_TYPE_REGISTERED) != 0
        && (pThing->thingInfo.actorInfo.pPlayer->aItems[typeId].status & SITHINVENTORY_ITEM_ACTIVATED) != 0;
}

void J3DAPI sithInventory_SetInventoryAvailable(SithThing* pThing, size_t typeId, int bAvailable)
{
    SITH_ASSERTREL(pThing->type == SITH_THING_PLAYER);
    SITH_ASSERTREL(typeId < STD_ARRAYLEN(sithInventory_g_aTypes));
    SITH_ASSERTREL(pThing->thingInfo.actorInfo.pPlayer);
    if ( pThing->thingInfo.actorInfo.pPlayer->aItems != NULL && (sithInventory_g_aTypes[typeId].flags & SITHINVENTORY_TYPE_REGISTERED) != 0 ) // TODO: pThing->thingInfo.actorInfo.pPlayer->aItems != NULL ???
    {
        SithInventoryItem* pItem = &pThing->thingInfo.actorInfo.pPlayer->aItems[typeId];
        if ( bAvailable )
        {
            pItem->status |= SITHINVENTORY_ITEM_AVAILABLE;
        }
        else
        {
            pItem->status &= ~SITHINVENTORY_ITEM_AVAILABLE;
        }
    }
}

int J3DAPI sithInventory_IsInventoryAvailable(const SithThing* pThing, size_t typeId)
{
    SITH_ASSERTREL(pThing->type == SITH_THING_PLAYER);
    SITH_ASSERTREL(typeId < STD_ARRAYLEN(sithInventory_g_aTypes));
    SITH_ASSERTREL(pThing->thingInfo.actorInfo.pPlayer);

    if ( pThing->thingInfo.actorInfo.pPlayer->aItems == NULL || (sithInventory_g_aTypes[typeId].flags & SITHINVENTORY_TYPE_REGISTERED) == 0 ) // TODO: pThing->thingInfo.actorInfo.pPlayer->aItems == NULL ???
    {
        return 0;
    }

    SithInventoryItem* aItems = pThing->thingInfo.actorInfo.pPlayer->aItems;
    return (aItems[typeId].status & SITHINVENTORY_ITEM_AVAILABLE) != 0 && (aItems[typeId].status & SITHINVENTORY_ITEM_DISABLED) == 0;
}

void J3DAPI sithInventory_SetInventoryDisabled(SithThing* pThing, size_t typeId, int bDisabled)
{
    SITH_ASSERTREL(pThing->type == SITH_THING_PLAYER);
    SITH_ASSERTREL(typeId < STD_ARRAYLEN(sithInventory_g_aTypes));

    if ( pThing->thingInfo.actorInfo.pPlayer
        && pThing->thingInfo.actorInfo.pPlayer->aItems != NULL  // TODO: pThing->thingInfo.actorInfo.pPlayer->aItems != NUL ???
        && (sithInventory_g_aTypes[typeId].flags & SITHINVENTORY_TYPE_REGISTERED) != 0 )
    {
        SithInventoryItem* pItem = &pThing->thingInfo.actorInfo.pPlayer->aItems[typeId];
        if ( bDisabled )
        {
            pItem->status |= SITHINVENTORY_ITEM_DISABLED;
        }
        else
        {
            pItem->status &= ~SITHINVENTORY_ITEM_DISABLED;
        }
    }
}

int J3DAPI sithInventory_IsBackpackItem(SithThing* pThing, size_t typeId)
{
    SITH_ASSERTREL(pThing->type == SITH_THING_PLAYER);
    SITH_ASSERTREL(typeId < STD_ARRAYLEN(sithInventory_g_aTypes));
    SITH_ASSERTREL(pThing->thingInfo.actorInfo.pPlayer);

    return (sithInventory_g_aTypes[typeId].flags & SITHINVENTORY_TYPE_REGISTERED) != 0
        && (sithInventory_g_aTypes[typeId].flags & SITHINVENTORY_TYPE_BACKPACKITEM) != 0;
}

int J3DAPI sithInventory_IsWeapon(size_t typeID)
{
    SITH_ASSERTREL(typeID < STD_ARRAYLEN(sithInventory_g_aTypes));
    return (sithInventory_g_aTypes[typeID].flags & SITHINVENTORY_TYPE_WEAPON) != 0;
}

float J3DAPI sithInventory_GetInventoryMinimum(SithThing* pThing, size_t id)
{
    SITH_ASSERTREL(pThing->type == SITH_THING_PLAYER);
    SITH_ASSERTREL(id < STD_ARRAYLEN(sithInventory_g_aTypes));
    SITH_ASSERTREL(pThing->thingInfo.actorInfo.pPlayer);

    return sithInventory_g_aTypes[id].min;
}

float J3DAPI sithInventory_GetInventoryMaximum(SithThing* pThing, size_t id)
{
    SITH_ASSERTREL(pThing->type == SITH_THING_PLAYER);
    SITH_ASSERTREL(id < STD_ARRAYLEN(sithInventory_g_aTypes));
    SITH_ASSERTREL(pThing->thingInfo.actorInfo.pPlayer);

    return sithInventory_g_aTypes[id].max;
}

void J3DAPI sithInventory_SetInventoryFlags(SithThing* pThing, size_t id, SithInventoryTypeFlag flags)
{
    SITH_ASSERTREL(pThing->type == SITH_THING_PLAYER);
    SITH_ASSERTREL(id < STD_ARRAYLEN(sithInventory_g_aTypes));
    SITH_ASSERTREL(pThing->thingInfo.actorInfo.pPlayer);

    sithInventory_g_aTypes[id].flags |= flags;
}

SithInventoryTypeFlag J3DAPI sithInventory_GetInventoryFlags(const SithThing* pThing, size_t id)
{
    SITH_ASSERTREL(pThing->type == SITH_THING_PLAYER);
    SITH_ASSERTREL(id < STD_ARRAYLEN(sithInventory_g_aTypes));
    SITH_ASSERTREL(pThing->thingInfo.actorInfo.pPlayer);

    return sithInventory_g_aTypes[id].flags;
}

void J3DAPI sithInventory_ClearInventoryFlags(const SithThing* pThing, size_t id, SithInventoryTypeFlag flags)
{
    SITH_ASSERTREL(pThing->type == SITH_THING_PLAYER);
    SITH_ASSERTREL(id < STD_ARRAYLEN(sithInventory_g_aTypes));
    SITH_ASSERTREL(pThing->thingInfo.actorInfo.pPlayer);

    sithInventory_g_aTypes[id].flags &= ~flags;
}

int J3DAPI sithInventory_BroadcastInventoryMessage(SithThing* pThing, SithCogSymbolRefType srcType, int srcIdx, SithCogMsgType msg, SithInventoryItemStatus status, int param0, int param1, int param2, int param3)
{
    SITH_ASSERTREL(pThing->type == SITH_THING_PLAYER);
    SITH_ASSERTREL(pThing->thingInfo.actorInfo.pPlayer);

    SithInventoryItem* aItems = pThing->thingInfo.actorInfo.pPlayer->aItems;
    int result = 0;
    for ( size_t typeId = 0; typeId < STD_ARRAYLEN(sithInventory_g_aTypes); ++typeId )
    {
        if ( (status & aItems[typeId].status) != 0 && sithInventory_IsInventoryAvailable(pThing, typeId) )
        {
            if ( sithInventory_g_aTypes[typeId].pCog )
            {
                result += sithCog_SendMessageEx(sithInventory_g_aTypes[typeId].pCog, msg, SITHCOG_SYM_REF_THING, pThing->idx, srcType, srcIdx, -1, param0, param1, param2, param3);
            }
        }
    }

    return result;
}

float J3DAPI sithInventory_BroadcastMessage(SithThing* pThing, SithCogSymbolRefType srcType, int srcIdx, SithCogMsgType messageType, SithInventoryTypeFlag flags, float param0, int param1, int param2, int param3)
{
    SITH_ASSERTREL(pThing->type == SITH_THING_PLAYER);
    SITH_ASSERTREL(pThing->thingInfo.actorInfo.pPlayer);

    for ( size_t i = 0; i < STD_ARRAYLEN(sithInventory_g_aTypes); ++i )
    {
        if ( sithInventory_IsInventoryAvailable(pThing, i) )
        {
            SithInventoryType* pType = &sithInventory_g_aTypes[i];
            if ( (flags & pType->flags) != 0 )
            {
                if ( pType->pCog )
                {
                    param0 = (float)(unsigned int)sithCog_SendMessageEx(pType->pCog, messageType, SITHCOG_SYM_REF_THING, pThing->idx, srcType, srcIdx, -1, (int32_t)param0, param1, param2, param3);
                }
            }
        }
    }

    return param0;
}

void J3DAPI sithInventory_ResetInventory(SithThing* pThing)
{
    SITH_ASSERTREL(pThing->type == SITH_THING_PLAYER);
    SITH_ASSERTREL(pThing->thingInfo.actorInfo.pPlayer);

    if ( sithInventory_g_bInitInventory || stdComm_IsGameActive() )
    {
        sithInventory_InitInventory(pThing);
        sithInventory_g_bInitInventory = 0;
    }

    for ( size_t i = 0; i < STD_ARRAYLEN(sithInventory_g_aTypes); ++i )
    {
        if ( (sithInventory_g_aTypes[i].flags & SITHINVENTORY_TYPE_NOT_CARRIED_BETWEEN_LEVELS) != 0 )
        {
            sithInventory_SetInventoryAvailable(pThing, i, 0);
            sithInventory_SetInventory(pThing, i, 0.0f);
        }
    }

    SithPlayer* pPlayer = pThing->thingInfo.actorInfo.pPlayer;
    pPlayer->curItemID   = 0;
    pPlayer->curWeaponID = SITHWEAPON_NO_WEAPON;

    sithInventory_g_bSendDeactivateMessage = 0;
    sithInventory_g_dword_56B750 = 0;
    sithInventory_g_dword_56B754 = 0;
}

void J3DAPI sithInventory_ResetAllTypes(SithThing* pThing)
{
    SITH_ASSERTREL(pThing->type == SITH_THING_PLAYER);
    SITH_ASSERTREL(pThing->thingInfo.actorInfo.pPlayer);
    for ( size_t typeId = 0; typeId < STD_ARRAYLEN(sithInventory_g_aTypes); ++typeId )
    {
        if ( (sithInventory_g_aTypes[typeId].flags & SITHINVENTORY_TYPE_NOT_CARRIED_BETWEEN_LEVELS) != 0 )
        {
            sithInventory_SetInventory(pThing, typeId, 0.0f);
        }
    }
}

SithThing* J3DAPI sithInventory_CreateBackpack(SithThing* pThing)
{
    SITH_ASSERTREL(pThing->type == SITH_THING_PLAYER);
    SITH_ASSERTREL(pThing->thingInfo.actorInfo.pPlayer);

    if ( !stdComm_IsGameActive() )
    {
        return NULL;
    }

    SithThing* pTemplate = sithTemplate_GetTemplate("+backpack");
    if ( !pTemplate )
    {
        return NULL;
    }

    SithThing* pBackpack = sithThing_CreateThing(pTemplate, pThing);
    if ( !pBackpack )
    {
        return NULL;
    }

    pBackpack->thingInfo.itemInfo.flags |= SITH_ITEM_BACKPACK;

    SithItemInfo* pItemInfo = &pBackpack->thingInfo.itemInfo;

    pItemInfo->numItems = 0;

    for ( size_t typeId = 0; typeId < STD_ARRAYLEN(sithInventory_g_aTypes); ++typeId )
    {
        if ( sithInventory_IsBackpackItem(pThing, typeId) )
        {
            float amount = sithInventory_GetInventory(pThing, typeId);
            if ( pItemInfo->numItems < STD_ARRAYLEN(pItemInfo->aBackpackItems) && amount > 0.0f )
            {
                pItemInfo->aBackpackItems[pItemInfo->numItems].typeId = typeId;
                pItemInfo->aBackpackItems[pItemInfo->numItems++].pItem->amount = amount;
            }
        }
    }

    sithDSSThing_CreateThing(pTemplate, pBackpack, pThing, NULL, NULL, NULL, SITHMESSAGE_STREAM_ALL, DPSEND_GUARANTEED);
    sithDSSThing_UpdateState(pBackpack, SITHMESSAGE_SENDTOJOINEDPLAYERS, SITHMESSAGE_STREAM_ALL);
    return pBackpack;
}

void J3DAPI sithInventory_PickupBackpack(SithThing* pPlayerThing, SithThing* pBackpackThing)
{
    SITH_ASSERTREL(pPlayerThing->type == SITH_THING_PLAYER);
    SITH_ASSERTREL(pPlayerThing->thingInfo.actorInfo.pPlayer);
    SITH_ASSERTREL(pBackpackThing->type == SITH_THING_ITEM);
    SITH_ASSERTREL(((pBackpackThing->thingInfo.itemInfo.flags) & (SITH_ITEM_BACKPACK)));

    SithItemInfo* pItemInfo = &pBackpackThing->thingInfo.itemInfo;
    for ( size_t i = 0; i < pItemInfo->numItems; ++i )
    {
        if ( sithInventory_IsBackpackItem(pPlayerThing, pItemInfo->aBackpackItems[i].typeId) )
        {
            sithInventory_ChangeInventory(pPlayerThing, pItemInfo->aBackpackItems[i].typeId, pItemInfo->aBackpackItems[i].pItem->amount);
        }
    }
}

int J3DAPI sithInventory_GetBackpackItemID(SithThing* pBackpackThing, size_t itemNum)
{
    SITH_ASSERTREL(pBackpackThing->type == SITH_THING_ITEM);
    SITH_ASSERTREL(((pBackpackThing->thingInfo.itemInfo.flags) & (SITH_ITEM_BACKPACK)));
    if ( itemNum >= pBackpackThing->thingInfo.itemInfo.numItems )
    {
        return -1;
    }

    return pBackpackThing->thingInfo.itemInfo.aBackpackItems[itemNum].typeId;
}

float J3DAPI sithInventory_GetBackpackItemValue(SithThing* pBackpackThing, size_t itemNum)
{
    SITH_ASSERTREL(pBackpackThing->type == SITH_THING_ITEM);
    SITH_ASSERTREL(((pBackpackThing->thingInfo.itemInfo.flags) & (SITH_ITEM_BACKPACK)));
    if ( itemNum >= pBackpackThing->thingInfo.itemInfo.numItems )
    {
        return -1.0f;
    }

    return pBackpackThing->thingInfo.itemInfo.aBackpackItems[itemNum].pItem->amount;
}

int J3DAPI sithInventory_GetNumBackpackItems(SithThing* pBackpackThing)
{
    SITH_ASSERTREL(pBackpackThing->type == SITH_THING_ITEM);
    SITH_ASSERTREL(((pBackpackThing->thingInfo.itemInfo.flags) & (SITH_ITEM_BACKPACK)));
    return pBackpackThing->thingInfo.itemInfo.numItems;
}

void J3DAPI sithInventory_CheckInventory(SithThing* pThing, size_t typeId)
{
    SITH_ASSERTREL(pThing->type == SITH_THING_PLAYER);
    SITH_ASSERTREL(typeId < STD_ARRAYLEN(sithInventory_g_aTypes));
    SITH_ASSERTREL(pThing->thingInfo.actorInfo.pPlayer);
}

SithInventoryType* J3DAPI sithInventory_GetInventoryType(SithThing* pThing, size_t typeId)
{
    SITH_ASSERTREL(pThing->type == SITH_THING_PLAYER);
    SITH_ASSERTREL(typeId < STD_ARRAYLEN(sithInventory_g_aTypes));
    SITH_ASSERTREL(pThing->thingInfo.actorInfo.pPlayer);

    if ( pThing->thingInfo.actorInfo.pPlayer->aItems != NULL && (sithInventory_g_aTypes[typeId].flags & SITHINVENTORY_TYPE_REGISTERED) != 0 ) // TODO: pThing->thingInfo.actorInfo.pPlayer->aItems != NULL  ???
    {
        return &sithInventory_g_aTypes[typeId];
    }

    return NULL;
}

// Following 3 functions were found in debug version

int sithInventory_sub_45E072(void)
{
    for ( size_t i = 0; i < STD_ARRAYLEN(sithInventory_g_aPowerKeybinds); ++i )
    {
        sithInventory_g_aPowerKeybinds[i].bEnabled = 0;
    }

    return 0;
}

void J3DAPI sithInventory_sub_45E0B0(size_t index, int a2)
{
    sithInventory_g_aPowerKeybinds[index].bEnabled = 1;
    sithInventory_g_aPowerKeybinds[index].binding = a2;
    sithInventory_g_aPowerKeybinds[index].bActivated = 0;
}

int J3DAPI sithInventory_sub_45E0E4(size_t index)
{
    if ( sithInventory_g_aPowerKeybinds[index].bEnabled == 1 )
    {
        return sithInventory_g_aPowerKeybinds[index].binding;
    }

    return -1;
}

void J3DAPI sithInventory_InitInventory(SithThing* pThing)
{
    SITH_ASSERTREL(pThing->type == SITH_THING_PLAYER);
    SITH_ASSERTREL(pThing->thingInfo.actorInfo.pPlayer);

    for ( size_t i = 0; i < STD_ARRAYLEN(sithInventory_g_aTypes); ++i )
    {
        sithInventory_SetInventory(pThing, i, 0.0f);
        if ( (sithInventory_g_aTypes[i].flags & SITHINVENTORY_TYPE_DEFAULT) != 0 )
        {
            sithInventory_SetInventoryAvailable(pThing, i, 1);
        }
        else
        {
            sithInventory_SetInventoryAvailable(pThing, i, 0);
        }

        sithInventory_SetInventoryActivated(pThing, i, 0);
    }

    pThing->thingInfo.actorInfo.pPlayer->lastPistolID = SITHWEAPON_PISTOL;
    pThing->thingInfo.actorInfo.pPlayer->lastRifleID  = SITHWEAPON_NO_WEAPON;
}

void J3DAPI sithInventory_BroadcastKilledMessage(const SithThing* pSender, const SithThing* pKiller)
{
    for ( size_t i = 0; i < STD_ARRAYLEN(sithInventory_g_aTypes); ++i )
    {
        if ( sithInventory_IsInventoryAvailable(pSender, i) )
        {
            SithInventoryType* pType = &sithInventory_g_aTypes[i];
            if ( pType->pCog )
            {
                if ( pKiller )
                {
                    sithCog_SendMessage(pType->pCog, SITHCOG_MSG_KILLED, SITHCOG_SYM_REF_THING, pSender->idx, SITHCOG_SYM_REF_THING, pKiller->idx, 0);
                }
                else
                {
                    sithCog_SendMessage(pType->pCog, SITHCOG_MSG_KILLED, SITHCOG_SYM_REF_THING, pSender->idx, SITHCOG_SYM_REF_THING, -1, 0);
                }
            }
        }
    }
}

void J3DAPI sithInventory_SetSwimmingInventory(SithThing* pThing, int bItemsAvailable)
{
    for ( size_t typeId = SITHWEAPON_FISTS; typeId <= SITHWEAPON_IMP5; ++typeId )
    {
        sithInventory_SetInventoryDisabled(pThing, typeId, bItemsAvailable == 0);
    }

    sithInventory_SetInventoryDisabled(pThing, SITHWEAPON_CHALK, bItemsAvailable == 0);

    for ( size_t typeId = SITHWEAPON_MIRROR; typeId <= 118; ++typeId )
    {
        sithInventory_SetInventoryDisabled(pThing, typeId, bItemsAvailable == 0);
    }
}
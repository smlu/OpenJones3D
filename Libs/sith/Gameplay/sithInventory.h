#ifndef SITH_SITHINVENTORY_H
#define SITH_SITHINVENTORY_H
#include <j3dcore/j3d.h>
#include <rdroid/types.h>
#include <sith/types.h>
#include <sith/Main/sithMain.h>
#include <std/types.h>

J3D_EXTERN_C_START

extern int sithInventory_g_bInitInventory;
// extern int sithInventory_g_bInitInventory ;

extern int sithInventory_g_bSendDeactivateMessage;
// extern int sithInventory_g_bSendDeactivateMessage;

extern int sithInventory_g_dword_56B750;
// extern int sithInventory_g_dword_56B750;

extern int sithInventory_g_dword_56B754;
// extern int sithInventory_g_dword_56B754;

extern SithInventoryType sithInventory_g_aTypes[SITHINVENTORY_MAXTYPES];
// extern SithInventoryType sithInventory_g_aTypes[SITHINVENTORY_MAXTYPES];

extern SithPowerKeybind sithInventory_g_aPowerKeybinds[20];
// extern SithPowerKeybind sithInventory_g_aPowerKeybinds[20];

void J3DAPI sithInventory_RegisterType(size_t typeId, const char* pName, float min, float max, SithInventoryTypeFlag flags, SithCog* pItemCog, rdModel3* pInvModel, rdModel3* pItemModel);

int J3DAPI sithInventory_FindNextTypeID(SithThing* pThing, int startSearchId, SithInventoryTypeFlag flags);
int J3DAPI sithInventory_FindPreviousTypeID(SithThing* pThing, int startSearchId, SithInventoryTypeFlag flags);

int J3DAPI sithInventory_FindNextItemID(SithThing* pThing, int itemId); // Added
int J3DAPI sithInventory_FindPreviousItemID(SithThing* pThing, int itemId); // Added

void J3DAPI sithInventory_SelectItem(SithThing* pThing, size_t typeId); // Added
void J3DAPI sithInventory_SelectNextItem(SithThing* pThing); // Added
void J3DAPI sithInventory_SelectPreviousItem(SithThing* pThing); // Added

SithInventoryType* J3DAPI sithInventory_GetType(size_t typeId);

int J3DAPI sithInventory_GetCurrentWeapon(const SithThing* pThing);
void J3DAPI sithInventory_SetCurrentWeapon(SithThing* pThing, SithWeaponId weaponID);

int J3DAPI sithInventory_GetCurrentItem(SithThing* pThing);
void J3DAPI sithInventory_SetCurrentItem(SithThing* pThing, int typeId);
float J3DAPI sithInventory_ChangeInventory(SithThing* pThing, size_t typeId, float amount);

float J3DAPI sithInventory_GetInventory(SithThing* pThing, size_t typeId);
float J3DAPI sithInventory_SetInventory(SithThing* pThing, size_t typeId, float amount);
void J3DAPI sithInventory_SetInventoryActivated(SithThing* pThing, size_t typeId, int bActivated);
int J3DAPI sithInventory_IsInventoryActivated(SithThing* pThing, size_t typeId);
void J3DAPI sithInventory_SetInventoryAvailable(SithThing* pThing, size_t typeId, int bAvailable);
int J3DAPI sithInventory_IsInventoryAvailable(const SithThing* pThing, size_t typeId);
void J3DAPI sithInventory_SetInventoryDisabled(SithThing* pThing, size_t typeId, int bDisabled);

int J3DAPI sithInventory_IsBackpackItem(SithThing* pThing, size_t typeId);
int J3DAPI sithInventory_IsWeapon(size_t typeID);

float J3DAPI sithInventory_GetInventoryMinimum(SithThing* pThing, size_t id);
float J3DAPI sithInventory_GetInventoryMaximum(SithThing* pThing, size_t id);

void J3DAPI sithInventory_SetInventoryFlags(SithThing* pThing, size_t id, SithInventoryTypeFlag flags);
SithInventoryTypeFlag J3DAPI sithInventory_GetInventoryFlags(const SithThing* pThing, size_t id); // Added
void J3DAPI sithInventory_ClearInventoryFlags(const SithThing* pThing, size_t id, SithInventoryTypeFlag flags); // Added

int J3DAPI sithInventory_BroadcastInventoryMessage(SithThing* pThing, SithCogSymbolRefType srcType, int srcIdx, SithCogMsgType msg, SithInventoryItemStatus status, int param0, int param1, int param2, int param3);
float J3DAPI sithInventory_BroadcastMessage(SithThing* pThing, SithCogSymbolRefType srcType, int srcIdx, SithCogMsgType messageType, SithInventoryTypeFlag flags, float param0, int param1, int param2, int param3);

void J3DAPI sithInventory_ResetInventory(SithThing* pThing);

void J3DAPI sithInventory_ResetAllTypes(SithThing* pThing); // Added

SithThing* J3DAPI sithInventory_CreateBackpack(SithThing* pThing);
void J3DAPI sithInventory_PickupBackpack(SithThing* pPlayerThing, SithThing* pBackpackThing);
int J3DAPI sithInventory_GetBackpackItemID(SithThing* pBackpackThing, size_t itemNum);
float J3DAPI sithInventory_GetBackpackItemValue(SithThing* pBackpackThing, size_t itemNum);
int J3DAPI sithInventory_GetNumBackpackItems(SithThing* pBackpackThing);

void J3DAPI sithInventory_CheckInventory(SithThing* pThing, size_t typeId); // Added
SithInventoryType* J3DAPI sithInventory_GetInventoryType(SithThing* pThing, size_t typeId);
void J3DAPI sithInventory_InitInventory(SithThing* pThing);

void J3DAPI sithInventory_BroadcastKilledMessage(const SithThing* pSender, const SithThing* pKiller);
void J3DAPI sithInventory_SetSwimmingInventory(SithThing* pThing, int bItemsAvailable);

void sithInventory_ResetGlobals(void);

J3D_EXTERN_C_END
#endif // SITH_SITHINVENTORY_H

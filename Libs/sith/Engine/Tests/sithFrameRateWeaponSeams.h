#ifndef SITH_FRAMERATE_WEAPON_SEAMS_H
#define SITH_FRAMERATE_WEAPON_SEAMS_H

#include "sithEngineTestAddrs.h"
#include <sith/types.h>

// Seams of sithWeapon.c and sithItem.c in the frame-rate tests. The weapon refire case fires a weapon type with a script and
// the item case keeps the item in the player's sight; everything else runs unchanged. The stubs are in sithFrameRateVec.h.
typedef struct sSithInventoryType SithInventoryType;
SithInventoryType* J3DAPI sithFrameRateVec_GetInventoryType(size_t typeId);
#define sithInventory_GetType sithFrameRateVec_GetInventoryType

int J3DAPI sithFrameRateVec_CheckThingLOS(const SithThing* pThing, const SithThing* pTarget, int bFlag);
#define sithCollision_CheckThingLOS sithFrameRateVec_CheckThingLOS

// The weapon noise case counts the AI awareness events of sithWeapon_Update
typedef struct srdVector3 rdVector3;
int J3DAPI sithFrameRateVec_CreateTransmittingEvent(SithSector* pSector, const rdVector3* pPos, int type, float level, SithThing* pThing);
#define sithAIAwareness_CreateTransmittingEvent sithFrameRateVec_CreateTransmittingEvent

#endif

#ifndef SITH_PHYSICS_TEST_SEAMS_H
#define SITH_PHYSICS_TEST_SEAMS_H

#include "sithEngineTestAddrs.h"
#include <rdroid/Math/rdVector.h>

// Include the real inline helper before renaming its call sites. Observing the
// integration step bounds a regression to the old unsigned negative-step loop.
void J3DAPI sithPhysicsTest_MultAcc3(rdVector3* pAcc, const rdVector3* pVector, float scale);
#define rdVector_MultAcc3 sithPhysicsTest_MultAcc3

// Model only the obsolete setting; JSON persistence is covered by stdConfig's tests.
float J3DAPI sithPhysicsTest_ConfigGetFloat(const char* pKey, float defaultValue);
bool sithPhysicsTest_ConfigContains(const char* pKey);
bool J3DAPI sithPhysicsTest_ConfigSetFloat(const char* pKey, float value);
#define stdConfig_GetFloat sithPhysicsTest_ConfigGetFloat
#define stdConfig_Contains sithPhysicsTest_ConfigContains
#define stdConfig_SetFloat sithPhysicsTest_ConfigSetFloat
// The exhaust smoke of the frame-rate tests: the smoke template and the created smoke things are stubs (sithFrameRateVec.h).
typedef struct sSithThing SithThing;
typedef struct srdVector3 rdVector3;
typedef struct srdMatrix34 rdMatrix34;
typedef struct sSithSector SithSector;
typedef struct sSithAnimationSlot SithAnimationSlot;
SithThing* J3DAPI sithFrameRateVec_GetTemplate(const char* pName);
#define sithTemplate_GetTemplate sithFrameRateVec_GetTemplate
SithThing* J3DAPI sithFrameRateVec_CreateThingAtPos(const SithThing* pTemplate, const rdVector3* pPos, const rdMatrix34* pOrient, SithSector* pSector, SithThing* pParent);
#define sithThing_CreateThingAtPos sithFrameRateVec_CreateThingAtPos
SithAnimationSlot* J3DAPI sithFrameRateVec_StartAnimateSpriteSize(SithThing* pSprite, const rdVector3* pStart, const rdVector3* pEnd, float secDuration);
#define sithAnimate_StartAnimateSpriteSize sithFrameRateVec_StartAnimateSpriteSize

#endif

#ifndef SITH_FRAMERATE_TEST_SEAMS_H
#define SITH_FRAMERATE_TEST_SEAMS_H

#include "sithEngineTestAddrs.h"
#include <sith/types.h>

// The stubs are in sithFrameRateVec.h, shared with the harness that captured tv/sithFrameRate.

// Record the damage the real per-frame damage code deals, instead of applying it through COG and actor state.
float J3DAPI sithFrameRateVec_DamageThing(SithThing* pThing, const SithThing* pPurpetrator, float damage, SithDamageType hitType);
#define sithThing_DamageThing sithFrameRateVec_DamageThing

// Choose the difficulty and IMP damage scale without game mode globals.
int sithFrameRateVec_GetGameDifficulty(void);
#define sithGetGameDifficulty sithFrameRateVec_GetGameDifficulty

float sithFrameRateVec_GetIMPDamageScalar(void);
#define sithGetIMPDamageScalar sithFrameRateVec_GetIMPDamageScalar

// Count the actor sound class modes the real per-frame code plays.
tSoundChannelHandle J3DAPI sithFrameRateVec_PlayModeRandom(SithThing* pThing, SithSoundClassMode mode);
#define sithSoundClass_PlayModeRandom sithFrameRateVec_PlayModeRandom

// Lip sync and weapon updates are not part of the damage under test.
void J3DAPI sithFrameRateVec_UpdateLipSync(SithThing* pThing);
#define sithVoice_UpdateLipSync sithFrameRateVec_UpdateLipSync

void J3DAPI sithFrameRateVec_UpdateActorWeapon(SithThing* pThing);
#define sithWeapon_UpdateActorWeapon sithFrameRateVec_UpdateActorWeapon

#endif

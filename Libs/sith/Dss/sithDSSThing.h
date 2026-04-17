#ifndef SITH_SITHDSSTHING_H
#define SITH_SITHDSSTHING_H
#include <j3dcore/j3d.h>
#include <rdroid/types.h>
#include <sith/types.h>
#include <sith/Devices/sithComm.h>
#include <sith/Main/sithMain.h>
#include <sith/RTI/addresses.h>
#include <std/types.h>

J3D_EXTERN_C_START

int J3DAPI sithDSSThing_Pos(const SithThing* pThing, DPID toID, unsigned int dpFlags);
int J3DAPI sithDSSThing_ProcessPos(const SithMessage* pMsg);
int J3DAPI sithDSSThing_MovePos(const SithThing* pThing, DPID idTo, unsigned int sendFlags);
int J3DAPI sithDSSThing_ProcessMovePos(const SithMessage* pMsg);
int J3DAPI sithDSSThing_UpdateState(const SithThing* pThing, DPID toID, SithMessageStream outstream);
int J3DAPI sithDSSThing_ProcessStateUpdate(const SithMessage* pMsg);
int J3DAPI sithDSSThing_PlaySound(const SithThing* pThing, const rdVector3* pPos, tSoundHandle hSnd, float volume, float pan, SoundPlayFlag playflags, int guid, DPID idTo, SithMessageStream outstream);
int J3DAPI sithDSSThing_ProcessPlaySound(const SithMessage* pMsg);
int J3DAPI sithDSSThing_PlaySoundMode(const SithThing* pThing, int16_t mode, int channel, float a4);
int J3DAPI sithDSSThing_ProcessPlaySoundMode(const SithMessage* pMsg);
int J3DAPI sithDSSThing_PlayKey(const SithThing* pThing, const rdKeyframe* pKey, rdKeyframeFlags flags, int16_t lo, unsigned int trackGUID, DPID idTo, SithMessageStream outstream);
int J3DAPI sithDSSThing_ProcessPlayKey(const SithMessage* pMsg);
int J3DAPI sithDSSThing_PlayKeyMode(const SithThing* pThing, SithPuppetSubMode mode, unsigned int trackGUID, DPID idTo, SithMessageStream outstream);
int J3DAPI sithDSSThing_ProcessPlayKeyMode(const SithMessage* pMsg);
int J3DAPI sithDSSThing_SetModel(const SithThing* pThing, DPID idTo);
int J3DAPI sithDSSThing_ProcessSetModel(const SithMessage* pMsg);
int J3DAPI sithDSSThing_StopKey(const SithThing* pThing, int trackGUID, float fadeTime, DPID idTo, SithMessageStream outstream);
int J3DAPI sithDSSThing_ProcessStopKey(const SithMessage* pMsg);
int J3DAPI sithDSSThing_StopSound(tSoundChannelHandle hChannel, float secFadeTime, DPID idTo, SithMessageStream outstream);
int J3DAPI sithDSSThing_ProcessStopSound(const SithMessage* pMsg);
int J3DAPI sithDSSThing_Fire(const SithThing* pShooter, const SithThing* pProjectileTemplate, const rdVector3* fireDir, const rdVector3* firePos, tSoundHandle hFireSnd, uint16_t puppetSubmode, float extra, int16_t projectileFlags, float sedRapidFireTime, int projectileGUID, DPID idTo, SithMessageStream outstream);
int J3DAPI sithDSSThing_ProcessFire(const SithMessage* pMsg);
int J3DAPI sithDSSThing_Death(const SithThing* pThing, const SithThing* pKiller, char bKillPlayer, DPID to, SithMessageStream outstream);
int J3DAPI sithDSSThing_ProcessDeath(const SithMessage* pMsg);
int J3DAPI sithDSSThing_DamageThing(const SithThing* pVictim, const SithThing* pPurpetrator, float damage, SithDamageType hitType, DPID idTo, SithMessageStream outstream);
int J3DAPI sithDSSThing_ProcessDamage(const SithMessage* pMsg);
int J3DAPI sithDSSThing_ThingFullDescription(const SithThing* pThing, DPID idTo, SithMessageStream outstream);
int J3DAPI sithDSSThing_ProcessThingFullDescription(const SithMessage* pMsg);
int J3DAPI sithDSSThing_PathMove(const SithThing* pThing, int16_t frame, float speed, int moveType, DPID idTO, SithMessageStream outstream);
int J3DAPI sithDSSThing_ProcessPathMove(const SithMessage* pMsg);
int J3DAPI sithDSSThing_Attachment(const SithThing* pThing, DPID idTo, SithMessageStream outstream, unsigned int sendFlags);
int J3DAPI sithDSSThing_ProcessAttachment(const SithMessage* pMsg);
int J3DAPI sithDSSThing_Take(const SithThing* pItem, const SithThing* pThing, SithMessageStream outstream);
int J3DAPI sithDSSThing_ProcessTake(const SithMessage* pMsg);
int J3DAPI sithDSSThing_CreateThing(const SithThing* pTemplate, const SithThing* pNewThing, const SithThing* pMarker, const SithSector* pSector, const rdVector3* pos, const rdVector3* pyr, SithMessageStream outstream, unsigned int sendFlags);
int J3DAPI sithDSSThing_ProcessCreateThing(const SithMessage* pMsg);
int J3DAPI sithDSSThing_DestroyThing(int guid, DPID idTo);
int J3DAPI sithDSSThing_ProcessDestroyThing(const SithMessage* pMsg);
void J3DAPI sithDSSThing_MoveToPos(SithThing* pThing, const rdVector3* pPos, SithSector* pSector);

// Helper hooking functions
void sithDSSThing_InstallHooks(void);
void sithDSSThing_ResetGlobals(void);

J3D_EXTERN_C_END
#endif // SITH_SITHDSSTHING_H

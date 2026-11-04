#ifndef SITH_SITHPUPPET_H
#define SITH_SITHPUPPET_H
#include <j3dcore/j3d.h>
#include <rdroid/types.h>
#include <sith/types.h>
#include <sith/Main/sithMain.h>
#include <std/types.h>

J3D_EXTERN_C_START

#define SITHPUPPET_JOINTINDEX_HEAD     0
#define SITHPUPPET_JOINTINDEX_NECK     1
#define SITHPUPPET_JOINTINDEX_HIP      2

#define SITHPUPPET_JOINTINDEX_FIRE1    3
#define SITHPUPPET_JOINTINDEX_FIRE2    4

#define SITHPUPPET_JOINTINDEX_AIM1     5
#define SITHPUPPET_JOINTINDEX_AIM2     6

#define SITHPUPPET_JOINTINDEX_AIMPITCH 7
#define SITHPUPPET_JOINTINDEX_AIMYAW   8
#define SITHPUPPET_JOINTINDEX_AIMROLL  9

#define SITHPUPPET_PLAYERROR_ALREADYPLAYING -99

#define SITH_PUPPET_GETMOVEMAJORMODE(thing, moveMode)   thing->pPuppetState->armedMode + SITH_PUPPET_NUMARMEDMODES * moveMode;
#define SITH_PUPPET_GETARMEDMAJORMODE(thing, armedMode) armedMode + SITH_PUPPET_NUMARMEDMODES * thing->pPuppetState->moveMode;

#define SITHPUPPET_CFG_CNDWORLDKEYFRAMES_LOADEXTERNAL "engine.world.cnd.keyframes.loadExternal"

extern int sithPuppet_g_bPlayerLeapForward;
// extern int sithPuppet_g_bPlayerLeapForward;

int sithPuppet_Startup(void);
void sithPuppet_Shutdown(void);

void J3DAPI sithPuppet_New(SithThing* pThing);
void J3DAPI sithPuppet_Free(SithThing* pThing);

void J3DAPI sithPuppet_RemoveAllTracks(SithThing* pThing);
void J3DAPI sithPuppet_UpdatePuppet(SithThing* pThing, float secDeltaTime);

void J3DAPI sithPuppet_PlayFidgetMode(SithThing* pThing);
void J3DAPI sithPuppet_StopFridgetTrack(SithThing* pThing);

int J3DAPI sithPuppet_IsModeOnTrack(const SithThing* pThing, SithPuppetSubMode mode);
int J3DAPI sithPuppet_IsAnyModeOnTrack(const SithThing* pThing, SithPuppetSubMode firstMode, SithPuppetSubMode lastMode);
SithPuppetTrack* J3DAPI sithPuppet_FindActiveTrack(const SithThing* pThing, const rdKeyframe* pKfTrack);

void J3DAPI sithPuppet_UpdateThingMoveTracks(SithThing* pThing, float secDeltaTime);
float J3DAPI sithPuppet_UpdateThingMove(SithThing* pThing, rdPuppetTrackCallback pAxis);

int J3DAPI sithPuppet_SetSubMode(SithThing* pThing, SithPuppetSubMode newSubMode, rdPuppetTrackCallback pfCallback);

/**
 * Sets new move mode and removes any playing track.
 * @param pThing        - Puppet thing to set new move mode
 * @param newMode       - The new move mode
 */
void J3DAPI sithPuppet_SetMoveMode(SithThing* pThing, SithPuppetMoveMode newMode);

/**
 * Sets new move mode and removes tracks only if current mode is different that newMode or if `bRemoveTracks` == true.
 * @param pThing        - Puppet thing to set new move mode
 * @param newMode       - The new move mode
 * @param bRemoveTracks - If true all currently playing tracks will be removed  regardless if newMode is same as current move mode.
 */
void J3DAPI sithPuppet_SetMoveModeEx(SithThing* pThing, SithPuppetMoveMode newMode, bool bRemoveTracks); // New;

void J3DAPI sithPuppet_SetArmedMode(SithThing* pThing, unsigned int newMode);
unsigned int J3DAPI sithPuppet_GetArmedMode(SithThing* pThing); // New

// On success trackIdx is returned, 
// -99 is returned when submode already plays 
// and -1 is returned on error e.g. submode doesn't contain animation keyframe 
int J3DAPI sithPuppet_PlayMode(SithThing* pThing, SithPuppetSubMode submode, rdPuppetTrackCallback pfCallback);
int J3DAPI sithPuppet_PlayForceMoveMode(SithThing* pThing, SithPuppetSubMode submode, rdPuppetTrackCallback pfCallback);
int J3DAPI sithPuppet_PlayKey(rdPuppet* pPuppet, rdKeyframe* pTrack, int lowPriority, int highPriority, rdKeyframeFlags flags, rdPuppetTrackCallback pfCallback);
int J3DAPI sithPuppet_GetTrackNumForGUID(const SithThing* pThing, uint32_t guid);

int J3DAPI sithPuppet_StopKey(rdPuppet* pPuppet, int track, float fadeTime);
void J3DAPI sithPuppet_SwapSubMode(SithThing* pThing, SithPuppetSubMode newMode, SithPuppetSubMode oldMode); // Added: From debug version
void J3DAPI sithPuppet_FinishForceMove(SithThing* pThing, int bStopTracks);

void J3DAPI sithPuppet_DefaultCallback(SithThing* pThing, int track, rdKeyMarkerType markerType);

int J3DAPI sithPuppet_WriteStaticPuppetsListText(const SithWorld* pWorld); // Added: From debug
int J3DAPI sithPuppet_ReadStaticPuppetsListText(SithWorld* pWorld, int bSkip);

int J3DAPI sithPuppet_WriteStaticPuppetsListBinary(tFileHandle fh, const SithWorld* pWorld);
int J3DAPI sithPuppet_ReadStaticPuppetsListBinary(tFileHandle fh, SithWorld* pWorld);

SithPuppetClass* J3DAPI sithPuppet_LoadPuppetClass(const char* pFilename);
int J3DAPI sithPuppet_LoadPuppetClassEntry(SithPuppetClass* pClass, const char* pPath);

int J3DAPI sithPuppet_AllocWorldPuppets(SithWorld* pWorld, size_t size);
void J3DAPI sithPuppet_FreeWorldPuppets(SithWorld* pWorld);

int J3DAPI sithPuppet_WriteStaticKeyframesListText(const SithWorld* pWorld); // Added: From debug version
int J3DAPI sithPuppet_ReadStaticKeyframesListText(SithWorld* pWorld, int bSkip);

int J3DAPI sithPuppet_WriteStaticKeyframesListBinary(tFileHandle fh, const SithWorld* pWorld);
int J3DAPI sithPuppet_ReadStaticKeyframesListBinary(tFileHandle fh, SithWorld* pWorld);

rdKeyframe* J3DAPI sithPuppet_GetKeyframeByIndex(int index);
rdKeyframe* J3DAPI sithPuppet_LoadKeyframe(const char* pName);

int J3DAPI sithPuppet_AllocWorldKeyframes(SithWorld* pWorld, size_t size);
void J3DAPI sithPuppet_FreeWorldKeyframes(SithWorld* pWorld);

rdKeyframe* J3DAPI sithPuppet_GetKeyframe(const char* pName);

int J3DAPI sithPuppet_StopMode(SithThing* pThing, SithPuppetSubMode submode, float fadeOutTime);
int J3DAPI sithPuppet_SynchMode(SithThing* pThing, SithPuppetSubMode oldMode, SithPuppetSubMode newMode, float a4, int bReverse);
void J3DAPI sithPuppet_ClearMode(SithThing* pThing, SithPuppetSubMode mode);

/**
 * Sets playback speed for any currently playing track with mode equal to `submode`.
 * @param pThing        - Actor thing to set submode speed
 * @param submode       - The playing submode to set the playback speed fot
 * @param playbackSpeed - The new playback speed. Must be > 0.0. Normal play is at speed 1.0
 * @return on success returns true, otherwise false (invalid submode, thing has no puppet class, submode has no keyframe set).
 */
bool J3DAPI sithPuppet_SetModeSpeed(SithThing* pThing, SithPuppetSubMode submode, float playbackSpeed); // Added new

void J3DAPI sithPuppet_RemoveTrackByIndex(SithThing* pThing, int trackNum);
int J3DAPI sithPuppet_NewTrack(SithThing* pThing, SithPuppetClassSubmode* pSubmode, int trackNum, SithPuppetSubMode submode);
void J3DAPI sithPuppet_AddTrack(SithThing* pThing, SithPuppetTrack* pNewTrack);
void J3DAPI sithPuppet_RemoveTrack(SithThing* pThing, SithPuppetTrack* pTrack);
void J3DAPI sithPuppet_FreeTrack(SithThing* pThing, SithPuppetTrack* pTrack);
void J3DAPI sithPuppet_FreeTrackByIndex(SithThing* pThing, int trackNum);

SithPuppetTrack* J3DAPI sithPuppet_GetModeTrack(const SithThing* pThing, SithPuppetSubMode mode);

void J3DAPI sithPuppet_ClearTrackList(SithThing* pThing);

// Helper hooking functions
void sithPuppet_ResetGlobals(void);

J3D_EXTERN_C_END
#endif // SITH_SITHPUPPET_H

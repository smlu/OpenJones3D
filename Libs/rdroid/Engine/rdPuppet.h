#ifndef RDROID_RDPUPPET_H
#define RDROID_RDPUPPET_H
#include <j3dcore/j3d.h>
#include <rdroid/types.h>
#include <std/types.h>

J3D_EXTERN_C_START

rdPuppet* J3DAPI rdPuppet_New(rdThing* pThing);
void J3DAPI rdPuppet_NewEntry(rdPuppet* pPuppet, rdThing* pThing);

void J3DAPI rdPuppet_Free(rdPuppet* pPuppet);
void J3DAPI rdPuppet_FreeEntry(rdPuppet* pPuppet); // Added

void J3DAPI rdPuppet_SetPause(rdPuppet* pPuppet, bool bPaused);
int J3DAPI rdPuppet_AddTrack(rdPuppet* pPuppet, rdKeyframe* pKFTrack, int lowPriority, int highPriority);
void J3DAPI rdPuppet_RemoveTrack(rdPuppet* pPuppet, int32_t track);

int J3DAPI rdPuppet_SetStatus(rdPuppet* pPuppet, int32_t track, rdPuppetTrackStatus status);
void J3DAPI rdPuppet_SetCallback(rdPuppet* pPuppet, int32_t track, rdPuppetTrackCallback pfCallback);

int J3DAPI rdPuppet_PlayTrack(rdPuppet* pPuppet, int32_t track);
int J3DAPI rdPuppet_FadeInTrack(rdPuppet* pPuppet, int32_t track, float fadeDuration);
int J3DAPI rdPuppet_FadeOutTrack(rdPuppet* pPuppet, int32_t track, float fadeDuration);
void J3DAPI rdPuppet_SetTrackSpeed(rdPuppet* pPuppet, int32_t track, float fps);
void J3DAPI rdPuppet_SetPlaybackSpeed(rdPuppet* pPuppet, int32_t track, float speed); // Added new
void J3DAPI rdPuppet_SetTrackNoise(rdPuppet* pPuppet, int32_t track, float noise); // Added from debug
void J3DAPI rdPuppet_SetTrackPriority(rdPuppet* pPuppet, int32_t track, int lowPri, int heighPri); // Added from debug

void J3DAPI rdPuppet_AdvanceTrack(rdPuppet* pPuppet, int32_t track, float frames);
int J3DAPI rdPuppet_UpdateTracks(rdPuppet* pPuppet, float secDeltaTime);

void J3DAPI rdPuppet_ResetTrack(rdPuppet* pPuppet, int32_t track);
void J3DAPI rdPuppet_BuildJointMatrices(rdThing* prdThing, const rdMatrix34* pPlacement);

void rdPuppet_ResetGlobals(void);

J3D_EXTERN_C_END
#endif // RDROID_RDPUPPET_H

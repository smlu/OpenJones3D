#ifndef SITH_SITHSOUNDCLASS_H
#define SITH_SITHSOUNDCLASS_H
#include <j3dcore/j3d.h>
#include <rdroid/types.h>
#include <sith/types.h>
#include <sith/Main/sithMain.h>
#include <sith/RTI/addresses.h>
#include <std/types.h>

J3D_EXTERN_C_START

int sithSoundClass_Startup(void);
void sithSoundClass_Shutdown(void);

int J3DAPI sithSoundClass_WriteSoundClassesListText(const SithWorld* pWorld); // Added: From debug version
int J3DAPI sithSoundClass_ReadSoundClassesListText(SithWorld* pWorld, int bSkip);

int J3DAPI sithSoundClass_WriteSoundClassesListBinary(tFileHandle fh, const SithWorld* pWorld);
int J3DAPI sithSoundClass_ReadSoundClassesListBinary(tFileHandle fh, SithWorld* pWorld);

SithSoundClass* J3DAPI sithSoundClass_Load(SithWorld* pWorld, const char* pName);

int J3DAPI sithSoundClass_LoadEntry(SithWorld* pWorld, SithSoundClass* pClass, const char* pPath);
signed int J3DAPI sithSoundClass_AllocSoundClasses(SithWorld* pWorld, size_t size);
void J3DAPI sithSoundClass_FreeWorldSoundClasses(SithWorld* pWorld);

tSoundChannelHandle J3DAPI sithSoundClass_PlayModeFirstEx(SithThing* pThing, SithSoundClassMode mode, float volume);
tSoundChannelHandle J3DAPI sithSoundClass_PlayModeFirst(SithThing* pThing, SithSoundClassMode mode);
tSoundChannelHandle J3DAPI sithSoundClass_PlayModeRandom(SithThing* pThing, SithSoundClassMode mode);
tSoundChannelHandle J3DAPI sithSoundClass_PlayVoiceModeRandom(SithThing* pThing, SithSoundClassMode mode);
tSoundChannelHandle J3DAPI sithSoundClass_PlayPlayerVoiceModeRandom(SithThing* pThing, SithSoundClassMode mode);
tSoundChannelHandle J3DAPI sithSoundClass_PlayMode(SithThing* pThing, SithSoundClassMode mode, float selectionRatio);

void J3DAPI sithSoundClass_StopMode(SithThing* pThing, SithSoundClassMode mode);
void J3DAPI sithSoundClass_SetModePitch(SithThing* pThing, SithSoundClassMode mode, float pitch);
void J3DAPI sithSoundClass_SetModeVolume(SithThing* pThing, SithSoundClassMode mode, float volume);
void J3DAPI sithSoundClass_FadeModeVolume(SithThing* pThing, SithSoundClassMode mode, float startVolume, float endVolume);

int J3DAPI sithSoundClass_IsModeFadingVol(SithThing* pThing, SithSoundClassMode mode);
bool J3DAPI sithSoundClass_IsModePitchFading(SithThing* pThing, SithSoundClassMode mode); // Added: From debug version

int J3DAPI sithSoundClass_SetThingClass(SithThing* pThing, SithSoundClass* pSoundClass); // Added: From debug version

// Helper hooking functions
void sithSoundClass_ResetGlobals(void);

J3D_EXTERN_C_END
#endif // SITH_SITHSOUNDCLASS_H

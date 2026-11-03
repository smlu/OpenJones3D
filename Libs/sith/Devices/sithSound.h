#ifndef SITH_SITHSOUND_H
#define SITH_SITHSOUND_H
#include <j3dcore/j3d.h>
#include <rdroid/types.h>
#include <sith/types.h>
#include <sith/Main/sithMain.h>
#include <sith/RTI/addresses.h>
#include <std/types.h>

J3D_EXTERN_C_START

// Max number of simultaneous playing sounds
#define SITHSOUND_MAXCHANNELS J3D_QOL_VALUE( 512, 32 )


int J3DAPI sithSound_Initialize(tHostServices* pHS);
void sithSound_Uninitialize(void);

int J3DAPI sithSound_Startup(int bSound3D);
int sithSound_StartupSound(void);

void sithSound_Shutdown(void);
void sithSound_ShutdownSound(void);

int J3DAPI sithSound_WriteSoundsListText(const SithWorld* pWorld); //Added: From debug version
int J3DAPI sithSound_ReadSoundsListText(SithWorld* pWorld, int bSkip);

int J3DAPI sithSound_WriteSoundsListBinary(tFileHandle fh, const SithWorld* pWorld);
int J3DAPI sithSound_ReadSoundsListBinary(tFileHandle fh, SithWorld* pWorld);

void J3DAPI sithSound_FreeWorldSounds(SithWorld* pWorld);

tSoundHandle J3DAPI sithSound_Load(SithWorld* pWorld, const char* filename);
void J3DAPI sithSound_SetSkipRestoringSounds(int bSkip);

// Helper hooking functions
void sithSound_ResetGlobals(void);

J3D_EXTERN_C_END
#endif // SITH_SITHSOUND_H

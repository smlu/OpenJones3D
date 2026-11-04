#ifndef SITH_SITHGAMESAVE_H
#define SITH_SITHGAMESAVE_H
#include <j3dcore/j3d.h>

#include <rdroid/types.h>

#include <sith/types.h>
#include <sith/Main/sithMain.h>

#include <std/types.h>

J3D_EXTERN_C_START

#define SITHSAVEGAME_THUMB_WIDTH  128
#define SITHSAVEGAME_THUMB_HEIGHT 96

#define SITHSAVEGAME_CFG_GAMEPLAY_LASTSAVEGAME "gameplay.lastSaveGame"

void sithGamesave_Startup(void);
void sithGamesave_Shutdown(void);

void sithGamesave_Open(void);
void sithGamesave_CloseRestore(void);
void sithGamesave_Close(void);

SithGameStatistics* sithGamesave_GetGameStatistics(void);
int sithGamesave_LockGameStatistics(void);
int sithGamesave_UnlockGameStatistics(void);

int J3DAPI sithGamesave_Save(const char* pFilename, int bOverwrite);
int J3DAPI sithGamesave_Restore(const char* pFilename, int bNotifyCog);
int sithGamesave_Process(void);

void J3DAPI sithGamesave_NotifyRestored(const char* pFilePath); // Notify master cog the game has been restored from a save file

void J3DAPI sithGamesave_SetThumbnailImage(tVBuffer* pVBuffer);
tSysBitmap sithGamesave_CreateThumbnail(void);
tSysBitmap J3DAPI sithGamesave_LoadThumbnail(const char* pFilename); // Function reads the thumbnail from the nds save file

void J3DAPI sithGamesave_SetPreviousLevelFilename(const char* pFilename);
const char* sithGamesave_GetPreviousLevelFilename(void);

const char* sithGamesave_GetLastFilename(void);
int J3DAPI sithGamesave_LoadLevelFilename(const char* pNdsFilePath, char* pDestFilename); // Function reads the level filename from the nds save file

int J3DAPI sithGamesave_GetState(const char** ppCurFilename);
void J3DAPI sithGamesave_SetSaveGameCallback(SithGameSaveCallback pfCallback);

// Helper hooking functions
void sithGamesave_ResetGlobals(void);

J3D_EXTERN_C_END
#endif // SITH_SITHGAMESAVE_H

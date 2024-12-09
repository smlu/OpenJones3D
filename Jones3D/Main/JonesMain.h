#ifndef JONES3D_JONESMAIN_H
#define JONES3D_JONESMAIN_H
#include <Jones3D/types.h>
#include <Jones3D/RTI/addresses.h>
#include <j3dcore/j3d.h>
#include <rdroid/types.h>
#include <sith/types.h>
#include <std/types.h>

J3D_EXTERN_C_START

#define JonesMain_g_aErrorBuffer J3D_DECL_FAR_ARRAYVAR(JonesMain_g_aErrorBuffer, char(*)[1024])
// extern char JonesMain_g_aErrorBuffer[1024] ;

#define JonesMain_g_mainMutex J3D_DECL_FAR_VAR(JonesMain_g_mainMutex, HANDLE)
// extern HANDLE JonesMain_g_mainMutex;

int J3DAPI JonesMain_Startup(const char* lpCmdLine);
void JonesMain_Shutdown(void);

int JonesMain_Open(void);
int JonesMain_Close(void);
int JonesMain_HasStarted(void);

void J3DAPI JonesMain_PauseGame();
void J3DAPI JonesMain_ResumeGame();
int J3DAPI JonesMain_IsGamePaused();

int JonesMain_Process(void);
int JonesMain_ProcessGamesaveState(void);
void JonesMain_UpdateLevelNum(void);

void JonesMain_TogglePrintFramerate(void);

void JonesMain_NextLevel(void); // Added
void J3DAPI JonesMain_JumpLevel(size_t levelNum); // Added
void JonesMain_RestartGame(void); // Added; restarts the game from beginning (1st level)
void JonesMain_RestartLevel(void); // Added; restarts current level
size_t JonesMain_GetCurrentLevelNum(void);

void JonesMain_SetBonusLevel(void);
void JonesMain_ShowEndCredits(void);

StdDisplayEnvironment* JonesMain_GetDisplayEnvironment(void);
JonesDisplaySettings* JonesMain_GetDisplaySettings(void);

int JonesMain_CloseWindow(void);

void JonesMain_RefreshDisplayDevice(void);

int JonesMain_PlayIntroMovie(void);

void J3DAPI JonesMain_BindToggleMenuControlKeys(const int* paKeyIds, int numKeys);

int J3DAPI JonesMain_Log(const char* pFormat, ...);
void J3DAPI JonesMain_LogErrorToFile(const char* pErrorText);
J3DNORETURN void J3DAPI JonesMain_Assert(const char* pErrorText, const char* pSrcFile, int line);
J3DNORETURN void JonesMain_FatalErrorUnknown(void); // Added

int J3DAPI JonesMain_ShowDevDialog(HWND hWnd, JonesState* pConfig);
int J3DAPI JonesMain_ShowDisplayOptions(HWND hWnd, JonesState* pState); // Added

size_t J3DAPI JonesMain_FindClosestVideoMode(const StdDisplayEnvironment* pList, const StdVideoMode* pVideoMode, size_t deviceNum);

// Helper hooking functions
void JonesMain_InstallHooks(void);
void JonesMain_ResetGlobals(void);

J3D_EXTERN_C_END
#endif // JONES3D_JONESMAIN_H

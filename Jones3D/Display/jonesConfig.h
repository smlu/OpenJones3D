#ifndef JONES3D_JONESCONFIG_H
#define JONES3D_JONESCONFIG_H
#include <Jones3D/types.h>
#include <Jones3D/RTI/addresses.h>
#include <j3dcore/j3d.h>
#include <rdroid/types.h>
#include <sith/types.h>

#include <std/General/stdConffile.h>
#include <std/types.h>

J3D_EXTERN_C_START

// Required size of string that is passed to functions that assigns savegame nds file path to it 
// i.e.: jonesConfig_GetSaveGameFilePath,jonesConfig_GetLoadGameFilePath, jonesConfig_ShowGameOverDialog, jonesConfig_ShowExitGameDialog
// The length of passed string argument should be the same as this macro
#define  JONESCONFIG_GAMESAVE_FILEPATHSIZE 128

int jonesConfig_Startup(void);
void jonesConfig_Shutdown(void);

int J3DAPI jonesConfig_ShowMessageDialog(HWND hWnd, const char* pTitle, const char* pText, int iconID);

int J3DAPI jonesConfig_GetSaveGameFilePath(HWND hWnd, char* pOutFilePath); // save game dialog
int J3DAPI jonesConfig_GetLoadGameFilePath(HWND hWnd, char* pDestNdsPath); // load game dialog

int J3DAPI jonesConfig_ShowGamePlayOptions(HWND hWnd);
void J3DAPI jonesConfig_EnableMouseControl(int bEnable);
int J3DAPI jonesConfig_ShowControlOptions(HWND hWnd);
int J3DAPI jonesConfig_ShowDisplaySettingsDialog(HWND hWnd, StdDisplayEnvironment* pDisplayEnv, JonesDisplaySettings* pDSettings);
int J3DAPI jonesConfig_ShowSoundSettingsDialog(HWND hWnd, JonesSoundSettings* pData);

int J3DAPI jonesConfig_ShowGameOverDialog(HWND hWnd, char* pRestoreFilename, tSoundHandle hSndGameOVerMus, tSoundChannelHandle* pSndChnlMus);
int J3DAPI jonesConfig_ShowStatisticsDialog(HWND hWnd, SithGameStatistics* pStatistics);
int J3DAPI jonesConfig_ShowLevelCompletedDialog(HWND hWnd, int* pBalance, int* apItemsState, int a4, int elapsedTime, int qiPoints, int numFoundTrasures, int foundTrasureValue, int totalTreasureValue);
int J3DAPI jonesConfig_ShowExitGameDialog(HWND hWnd, char* pSaveGameFilePath);

int J3DAPI jonesConfig_ShowDialogInsertCD(HWND hWnd, LPARAM dwInitParam);

void jonesConfig_UpdateCurrentLevelNum(void); // Updates cached current level num from JonesMain module. 
                                              // Cached level num is used for statistics dialog

// Helper hooking functions
void jonesConfig_InstallHooks(void);
void jonesConfig_ResetGlobals(void);

J3D_EXTERN_C_END
#endif // JONES3D_JONESCONFIG_H

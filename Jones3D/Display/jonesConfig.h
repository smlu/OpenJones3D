#ifndef JONES3D_JONESCONFIG_H
#define JONES3D_JONESCONFIG_H
#include <Jones3D/types.h>
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

// Configuration keys
#define JONESCONFIG_CFG_SOURCEPATH          "sourcePath"
#define JONESCONFIG_CFG_DEVMODE             "devMode"
#define JONESCONFIG_CFG_STARTMODE           "startMode"

#define JONESCONFIG_CFG_CONTROLS_CONFIGFILE "controls.configFile"

#define JONESCONFIG_CFG_GAMEPLAY_DEFAULTRUN "gameplay.defaultRun"
#define JONESCONFIG_CFG_GAMEPLAY_STARTLEVEL "gameplay.startLevel"

#define JONESCONFIG_CFG_LOG_MODE            "log.mode"
#define JONESCONFIG_CFG_LOG_LEVEL           "log.level"

#define JONESCONFIG_CFG_SOUND_HW            "sound.hw"
#define JONESCONFIG_CFG_SOUND_REVERSE       "sound.reverse"
#define JONESCONFIG_CFG_SOUND_VOLUME        "sound.volume"

int jonesConfig_Startup(void);
void jonesConfig_Shutdown(void);

int J3DAPI jonesConfig_ShowMessageDialog(tSysWindow hWnd, const char* pTitle, const char* pText, int iconID);

int J3DAPI jonesConfig_GetSaveGameFilePath(tSysWindow hWnd, char* pOutFilePath); // save game dialog
int J3DAPI jonesConfig_GetLoadGameFilePath(tSysWindow hWnd, char* pDestNdsPath); // load game dialog

int J3DAPI jonesConfig_ShowGamePlayOptions(tSysWindow hWnd);
void J3DAPI jonesConfig_EnableMouseControl(int bEnable);
int J3DAPI jonesConfig_ShowControlOptions(tSysWindow hWnd);
int J3DAPI jonesConfig_ShowDisplaySettingsDialog(tSysWindow hWnd, StdDisplayEnvironment* pDisplayEnv, JonesDisplaySettings* pDSettings);
int J3DAPI jonesConfig_ShowSoundSettingsDialog(tSysWindow hWnd, JonesSoundSettings* pData);

int J3DAPI jonesConfig_ShowGameOverDialog(tSysWindow hWnd, char* pRestoreFilename, tSoundHandle hSndGameOVerMus, tSoundChannelHandle* pSndChnlMus);
int J3DAPI jonesConfig_ShowStatisticsDialog(tSysWindow hWnd, SithGameStatistics* pStatistics);
int J3DAPI jonesConfig_ShowLevelCompletedDialog(tSysWindow hWnd, int* pBalance, int* apItemsState, int a4, int elapsedTime, int qiPoints, int numFoundTrasures, int foundTrasureValue, int totalTreasureValue);
int J3DAPI jonesConfig_ShowExitGameDialog(tSysWindow hWnd, char* pSaveGameFilePath);

int J3DAPI jonesConfig_ShowDialogInsertCD(tSysWindow hWnd, intptr_t dwInitParam);

void jonesConfig_UpdateCurrentLevelNum(void); // Updates cached current level num from JonesMain module. 
                                              // Cached level num is used for statistics dialog

// Helper hooking functions
void jonesConfig_ResetGlobals(void);

J3D_EXTERN_C_END
#endif // JONES3D_JONESCONFIG_H

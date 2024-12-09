#include "jonesConfig.h"
#include <j3dcore/j3dhook.h>

#include <Jones3D/Gui/JonesDialog.h>
#include <Jones3D/Main/JonesMain.h>
#include <Jones3D/Main/jonesString.h>
#include <Jones3D/Play/jonesInventory.h>
#include <Jones3D/RTI/symbols.h>

#include <rdroid/Main/rdroid.h>

#include <sith/Devices/sithControl.h>
#include <sith/DSS/sithGamesave.h>
#include <sith/Gameplay/sithOverlayMap.h>
#include <sith/Gameplay/sithPlayer.h>
#include <sith/World/sithModel.h>
#include <sith/World/sithVoice.h>

#include <std/General/std.h>
#include <std/General/stdFnames.h>
#include <std/General/stdUtil.h>
#include <std/Win95/stdDisplay.h>
#include <std/Win95/stdControl.h>

#include <w32util/wuRegistry.h>

#include <math.h>

#define JONESCONFIG_NOFONTSCALEMASK 1119
#define JONESCONFIG_NOFONTSCALE(dlgID) MAKELONG(dlgID, JONESCONFIG_NOFONTSCALEMASK)

typedef struct sLoadGameDialogData
{
    HDC hdcThumbnail;
    char aFilePath[128];
    HBITMAP hThumbnail;
    int pfThubnailProc;
    int bFolderSel;
    int unknown36;
    int unknown37;
} LoadGameDialogData;

typedef struct sSaveGameDialogData
{
    HDC hdcThumbnail;
    char aFilePath[128];
    HBITMAP hThumbnail;
    int pfThumbnailProc;
    int bFolderSel;
} SaveGameDialogData;

typedef struct sJonesDialogFontScaleMetrics
{
    int dialogID;
    int refWidth;
    int refHeight;
} JonesDialogSize;
static_assert(sizeof(JonesDialogSize) == 12, "sizeof(JonesDialogSize) == 12");

static const JonesDialogSize jonesConfig_aDialogSizes[21] = {
   { 164, 368, 232 },
   { 112, 260, 252 },
   { 111, 408, 218 },
   { 116, 297, 95 },
   { 115, 549, 367 },
   { 117, 309, 103 },
   { 120, 282, 152 },
   { 114, 238, 285 },
   { 148, 440, 393 },
   { 113, 351, 206 },
   { 211, 266, 103 },
   { 163, 282, 90 },
   { 214, 282, 90 },
   { 121, 282, 90 },
   { 150, 249, 100 },
   { 233, 344, 180 },
   { 190, 538, 297 },
   { 212, 280, 74 },
   { 159, 583, 250 },
   { 154, 579, 252 },
   { 167, 279, 75 }
};

HFONT jonesConfig_hFontGamePlayOptionsDlg = NULL;

#define jonesConfig_aStoreItems J3D_DECL_FAR_ARRAYVAR(jonesConfig_aStoreItems, tStoreItem(*)[14])
#define jonesConfig_aNumberGlyphMetrics J3D_DECL_FAR_ARRAYVAR(jonesConfig_aNumberGlyphMetrics, StdRect(*)[10])
#define jonesConfig_apDialogIconFiles J3D_DECL_FAR_ARRAYVAR(jonesConfig_apDialogIconFiles, char*(*)[6])
#define jonesConfig_aLevelNames J3D_DECL_FAR_ARRAYVAR(jonesConfig_aLevelNames, const char*(*)[17])
#define jonesConfig_JONES_STR_SUMMERY J3D_DECL_FAR_VAR(jonesConfig_JONES_STR_SUMMERY, const char*)
#define jonesConfig_aJonesControlActionNames J3D_DECL_FAR_ARRAYVAR(jonesConfig_aJonesControlActionNames, const char*(*)[37])
#define jonesConfig_aJonesCapControlActionNames J3D_DECL_FAR_ARRAYVAR(jonesConfig_aJonesCapControlActionNames, const char*(*)[37])
#define jonesConfig_aDfltKeySets J3D_DECL_FAR_ARRAYVAR(jonesConfig_aDfltKeySets, int*(*)[3])
#define jonesConfig_aDfltKeySetNames J3D_DECL_FAR_ARRAYVAR(jonesConfig_aDfltKeySetNames, const char*(*)[3])
#define jonesConfig_aControlKeyStrings J3D_DECL_FAR_ARRAYVAR(jonesConfig_aControlKeyStrings, const char*(*)[223])
// #define jonesConfig_aDialogSizes J3D_DECL_FAR_ARRAYVAR(jonesConfig_aDialogSizes, JonesDialogSize(*)[21])
#define jonesConfig_dword_511954 J3D_DECL_FAR_VAR(jonesConfig_dword_511954, int)
#define jonesConfig_dword_511958 J3D_DECL_FAR_VAR(jonesConfig_dword_511958, int)
#define jonesConfig_perfLevel J3D_DECL_FAR_VAR(jonesConfig_perfLevel, int)
#define jonesConfig_prevLevelNum J3D_DECL_FAR_VAR(jonesConfig_prevLevelNum, int)
#define jonesConfig_dword_5510B8 J3D_DECL_FAR_VAR(jonesConfig_dword_5510B8, int)
#define jonesConfig_dword_5510BC J3D_DECL_FAR_VAR(jonesConfig_dword_5510BC, int)
#define jonesConfig_dword_5510C0 J3D_DECL_FAR_VAR(jonesConfig_dword_5510C0, int)
#define jonesConfig_dword_5510C4 J3D_DECL_FAR_VAR(jonesConfig_dword_5510C4, int)
#define jonesConfig_curControlConfig J3D_DECL_FAR_VAR(jonesConfig_curControlConfig, JonesControlsConfig)
#define jonesConfig_dword_5510E0 J3D_DECL_FAR_VAR(jonesConfig_dword_5510E0, int)
#define jonesConfig_aGlyphMetrics J3D_DECL_FAR_ARRAYVAR(jonesConfig_aGlyphMetrics, ABC(*)[256])
#define jonesConfig_textMetric J3D_DECL_FAR_VAR(jonesConfig_textMetric, TEXTMETRICA)
#define jonesConfig_dword_551D20 J3D_DECL_FAR_VAR(jonesConfig_dword_551D20, unsigned int)
#define jonesConfig_apDialogIcons J3D_DECL_FAR_ARRAYVAR(jonesConfig_apDialogIcons, HBITMAP(*)[6])
#define jonesConfig_pKeySetsDirPath J3D_DECL_FAR_ARRAYVAR(jonesConfig_pKeySetsDirPath, char(*)[128])
#define jonesConfig_bDefaultRun J3D_DECL_FAR_VAR(jonesConfig_bDefaultRun, UINT)
#define jonesConfig_curLevelNum J3D_DECL_FAR_VAR(jonesConfig_curLevelNum, int)
#define jonesConfig_bStartup J3D_DECL_FAR_VAR(jonesConfig_bStartup, int)
#define jonesConfig_dword_551DCC J3D_DECL_FAR_VAR(jonesConfig_dword_551DCC, int)
#define jonesConfig_hdo_551DD0 J3D_DECL_FAR_VAR(jonesConfig_hdo_551DD0, HGDIOBJ)
#define jonesConfig_hFontSaveGameDlg J3D_DECL_FAR_VAR(jonesConfig_hFontSaveGameDlg, HFONT)
#define jonesConfig_hFontExitDlg J3D_DECL_FAR_VAR(jonesConfig_hFontExitDlg, HFONT)
#define jonesConfig_hFontGameSaveMsgBox J3D_DECL_FAR_VAR(jonesConfig_hFontGameSaveMsgBox, HFONT)
#define jonesConfig_hFontLoadGameDlg J3D_DECL_FAR_VAR(jonesConfig_hFontLoadGameDlg, HFONT)
//#define jonesConfig_hFontGamePlayOptionsDlg J3D_DECL_FAR_VAR(jonesConfig_hFontGamePlayOptionsDlg, HFONT)
#define jonesConfig_hFontControlOptions J3D_DECL_FAR_VAR(jonesConfig_hFontControlOptions, HFONT)
#define jonesConfig_dword_551DEC J3D_DECL_FAR_VAR(jonesConfig_dword_551DEC, void*)
#define jonesConfig_hFontCreateControlSchemeDlg J3D_DECL_FAR_VAR(jonesConfig_hFontCreateControlSchemeDlg, HFONT)
#define jonesConfig_hFontEditControlShceme J3D_DECL_FAR_VAR(jonesConfig_hFontEditControlShceme, HFONT)
#define jonesConfig_hFontAssignKeyDlg J3D_DECL_FAR_VAR(jonesConfig_hFontAssignKeyDlg, HFONT)
#define jonesConfig_bControlsActive J3D_DECL_FAR_VAR(jonesConfig_bControlsActive, int)
#define jonesConfig_hFontReassignKeyDlg J3D_DECL_FAR_VAR(jonesConfig_hFontReassignKeyDlg, HFONT)
#define jonesConfig_hFontDisplaySettingsDlg J3D_DECL_FAR_VAR(jonesConfig_hFontDisplaySettingsDlg, HFONT)
#define jonesConfig_hFontAdvanceDisplaySettingsDialog J3D_DECL_FAR_VAR(jonesConfig_hFontAdvanceDisplaySettingsDialog, HFONT)
#define jonesConfig_hFontSoundSettings J3D_DECL_FAR_VAR(jonesConfig_hFontSoundSettings, HFONT)
#define jonesConfig_hSndChannel J3D_DECL_FAR_VAR(jonesConfig_hSndChannel, tSoundChannelHandle)
#define jonesConfig_dword_551E14 J3D_DECL_FAR_VAR(jonesConfig_dword_551E14, int)
#define jonesConfig_hFontGameOverDialog J3D_DECL_FAR_VAR(jonesConfig_hFontGameOverDialog, HFONT)
#define jonesConfig_dword_551E1C J3D_DECL_FAR_VAR(jonesConfig_dword_551E1C, HGDIOBJ)
#define jonesConfig_hFontLevelCopletedDialog J3D_DECL_FAR_VAR(jonesConfig_hFontLevelCopletedDialog, HFONT)
#define jonesConfig_hFontStoreDialog J3D_DECL_FAR_VAR(jonesConfig_hFontStoreDialog, HFONT)
#define jonesConfig_hFontPurchaseMessageBox J3D_DECL_FAR_VAR(jonesConfig_hFontPurchaseMessageBox, HFONT)
#define jonesConfig_hFontDialogInsertCD J3D_DECL_FAR_VAR(jonesConfig_hFontDialogInsertCD, HFONT)

void jonesConfig_InstallHooks(void)
{
    // Uncomment only lines for functions that have full definition and doesn't call original function (non-thunk functions)

    // J3D_HOOKFUNC(jonesConfig_Startup);
    // J3D_HOOKFUNC(jonesConfig_InitKeySetsPath);
    // J3D_HOOKFUNC(jonesConfig_InitKeyActions);
    // J3D_HOOKFUNC(jonesConfig_Shutdown);
    // J3D_HOOKFUNC(jonesConfig_ControlToString);
    // J3D_HOOKFUNC(jonesConfig_ShowMessageDialog);
    // J3D_HOOKFUNC(jonesConfig_MessageDialogProc);
    // J3D_HOOKFUNC(jonesConfig_sub_4025F0);
    // J3D_HOOKFUNC(jonesConfig_sub_402670);
    // J3D_HOOKFUNC(jonesConfig_SetTextControl);
    // J3D_HOOKFUNC(jonesConfig_sub_402A90);
    // J3D_HOOKFUNC(jonesConfig_NewControlScheme);
    // J3D_HOOKFUNC(jonesConfig_LoadConstrolsScheme);
    // J3D_HOOKFUNC(jonesConfig_GetEntryIndex);
    // J3D_HOOKFUNC(jonesConfig_GetValueIndex);
    // J3D_HOOKFUNC(jonesConfig_GetControllerKeySchemeIndex);
    // J3D_HOOKFUNC(jonesConfig_SetDefaultControlScheme);
    // J3D_HOOKFUNC(jonesConfig_BindControls);
    // J3D_HOOKFUNC(jonesConfig_BindJoystickControl);
    J3D_HOOKFUNC(jonesConfig_InitDialog);
    J3D_HOOKFUNC(jonesConfig_CreateDialogFont);
    J3D_HOOKFUNC(jonesConfig_ResetDialogFont);
    J3D_HOOKFUNC(jonesConfig_ResetWindowFontCallback);
    J3D_HOOKFUNC(jonesConfig_SetPositionAndTextCallback);
    J3D_HOOKFUNC(jonesConfig_SetWindowFontAndPosition);
    J3D_HOOKFUNC(jonesConfig_GetWindowScreenRect);
    J3D_HOOKFUNC(jonesConfig_SetDialogTitleAndPosition);
    // J3D_HOOKFUNC(jonesConfig_GetSaveGameFilePath);
    J3D_HOOKFUNC(jonesConfig_SaveGameDialogHookProc);
    J3D_HOOKFUNC(jonesConfig_SaveGameDialogInit);
    J3D_HOOKFUNC(jonesConfig_SaveGameThumbnailPaintProc);
    // J3D_HOOKFUNC(jonesConfig_ShowOverwriteSaveGameDlg);
    // J3D_HOOKFUNC(jonesConfig_ShowExitGameDialog);
    // J3D_HOOKFUNC(jonesConfig_ExitGameDlgProc);
    // J3D_HOOKFUNC(jonesConfig_SaveGameMsgBoxProc);
    // J3D_HOOKFUNC(jonesConfig_GameSaveSetData);
    // J3D_HOOKFUNC(jonesConfig_MsgBoxDlg_HandleWM_COMMAND);
    J3D_HOOKFUNC(jonesConfig_GetLoadGameFilePath);
    // J3D_HOOKFUNC(jonesConfig_sub_405F60);
    J3D_HOOKFUNC(jonesConfig_LoadGameDialogHookProc);
    // J3D_HOOKFUNC(jonesConfig_ShowLoadGameWarningMsgBox);
    J3D_HOOKFUNC(jonesConfig_LoadGameDialogInit);
    J3D_HOOKFUNC(jonesConfig_LoadGameThumbnailPaintProc);
    J3D_HOOKFUNC(jonesConfig_ShowGamePlayOptions);
    J3D_HOOKFUNC(jonesConfig_GamePlayOptionsProc);
    J3D_HOOKFUNC(jonesConfig_HandleWM_HSCROLL);
    J3D_HOOKFUNC(jonesConfig_GamePlayOptionsInitDlg);
    J3D_HOOKFUNC(jonesConfig_GamePlayOptions_HandleWM_COMMAND);
    J3D_HOOKFUNC(jonesConfig_EnableMouseControl);
    // J3D_HOOKFUNC(jonesConfig_FreeControlScheme);
    // J3D_HOOKFUNC(jonesConfig_FreeControlConfigEntry);
    // J3D_HOOKFUNC(jonesConfig_ShowControlOptions);
    // J3D_HOOKFUNC(jonesConfig_CopyControlConfig);
    // J3D_HOOKFUNC(jonesConfig_CopyControlSchemes);
    // J3D_HOOKFUNC(jonesConfig_InitControlsConfig);
    // J3D_HOOKFUNC(jonesConfig_LoadControlsSchemesFromSystem);
    // J3D_HOOKFUNC(jonesConfig_SetDefaultControlSchemes);
    // J3D_HOOKFUNC(jonesConfig_ControlOptionsProc);
    // J3D_HOOKFUNC(jonesConfig_InitControlOptionsDlg);
    // J3D_HOOKFUNC(jonesConfig_ControlOptions_HandleWM_COMMAND);
    // J3D_HOOKFUNC(jonesConfig_WriteScheme);
    // J3D_HOOKFUNC(jonesConfig_sub_407F60);
    // J3D_HOOKFUNC(jonesConfig_sub_408000);
    // J3D_HOOKFUNC(jonesConfig_sub_408260);
    // J3D_HOOKFUNC(jonesConfig_sub_408400);
    // J3D_HOOKFUNC(jonesConfig_CreateNewSchemeAction);
    // J3D_HOOKFUNC(jonesConfig_ShowCreateControlSchemeDialog);
    // J3D_HOOKFUNC(jonesConfig_CreateControlSchemeProc);
    // J3D_HOOKFUNC(jonesConfig_InitCreateControlScheme);
    // J3D_HOOKFUNC(jonesConfig_CreateControlScheme_Handle_WM_COMMAND);
    // J3D_HOOKFUNC(jonesConfig_ShowEditControlShemeDialog);
    // J3D_HOOKFUNC(jonesConfig_EditControlSchemProc);
    // J3D_HOOKFUNC(jonesConfig_InitEditControlSchemeDlg);
    // J3D_HOOKFUNC(jonesConfig_sub_408ED0);
    // J3D_HOOKFUNC(jonesConfig_sub_408FC0);
    // J3D_HOOKFUNC(jonesConfig_sub_409200);
    // J3D_HOOKFUNC(jonesConfig_sub_409530);
    // J3D_HOOKFUNC(jonesConfig_sub_4096F0);
    // J3D_HOOKFUNC(jonesConfig_sub_4097D0);
    // J3D_HOOKFUNC(jonesConfig_sub_409BC0);
    // J3D_HOOKFUNC(jonesConfig_AssignKeyAction);
    // J3D_HOOKFUNC(jonesConfig_sub_409F70);
    // J3D_HOOKFUNC(jonesConfig_AssignKeyDlgProc);
    // J3D_HOOKFUNC(jonesConfig_sub_40A1A0);
    // J3D_HOOKFUNC(jonesConfig_AssignControlKey);
    // J3D_HOOKFUNC(jonesConfig_sub_40A500);
    // J3D_HOOKFUNC(jonesConfig_AssignKeyDlg_HandleWM_COMMAND);
    // J3D_HOOKFUNC(jonesConfig_ShowReassignKeyMsgBox);
    // J3D_HOOKFUNC(jonesConfig_ReassignKeyDialogProc);
    // J3D_HOOKFUNC(jonesConfig_SetReassignKeyDialogText);
    // J3D_HOOKFUNC(jonesConfig_sub_40AA10);
    // J3D_HOOKFUNC(jonesConfig_ShowDisplaySettingsDialog);
    // J3D_HOOKFUNC(jonesConfig_DisplaySettingsDialogProc);
    // J3D_HOOKFUNC(jonesConfig_sub_40AE90);
    // J3D_HOOKFUNC(jonesConfig_sub_40B530);
    // J3D_HOOKFUNC(jonesConfig_sub_40B5A0);
    // J3D_HOOKFUNC(jonesConfig_ShowAdvanceDisplaySettings);
    // J3D_HOOKFUNC(jonesConfig_AdvanceDisplaySettingsDialog);
    // J3D_HOOKFUNC(jonesConfig_sub_40BC40);
    // J3D_HOOKFUNC(jonesConfig_sub_40BCD0);
    // J3D_HOOKFUNC(jonesConfig_sub_40C090);
    // J3D_HOOKFUNC(jonesConfig_ShowSoundSettingsDialog);
    // J3D_HOOKFUNC(jonesConfig_SoundSettingsDialogProc);
    // J3D_HOOKFUNC(jonesConfig_sub_40C650);
    // J3D_HOOKFUNC(jonesConfig_SoundSettings_HandleWM_COMMAND);
    // J3D_HOOKFUNC(jonesConfig_ShowGameOverDialog);
    // J3D_HOOKFUNC(jonesConfig_GameOverDialogProc);
    // J3D_HOOKFUNC(jonesConfig_LoadGameGetLastSavedGamePath);
    // J3D_HOOKFUNC(jonesConfig_GameOverDialogInit);
    // J3D_HOOKFUNC(jonesConfig_GameOverDialog_HandleWM_COMMAND);
    // J3D_HOOKFUNC(jonesConfig_ShowStatisticsDialog);
    // J3D_HOOKFUNC(jonesConfig_StatisticsDialogProc);
    // J3D_HOOKFUNC(jonesConfig_sub_40D100);
    // J3D_HOOKFUNC(jonesConfig_UpdateCurrentLevelNum);
    // J3D_HOOKFUNC(jonesConfig_DrawImageOnDialogItem);
    // J3D_HOOKFUNC(jonesConfig_SetStatisticsDialogForLevel);
    // J3D_HOOKFUNC(jonesConfig_DrawStatisticDialogIQPoints);
    // J3D_HOOKFUNC(jonesConfig_InitStatisticDialog);
    // J3D_HOOKFUNC(jonesConfig_StatisticProc_HandleWM_COMMAND);
    // J3D_HOOKFUNC(jonesConfig_ShowLevelCompletedDialog);
    // J3D_HOOKFUNC(jonesConfig_LevelCompletedDialogProc);
    // J3D_HOOKFUNC(jonesConfig_InitLevelCompletedDialog);
    // J3D_HOOKFUNC(jonesConfig_ChapterCompleteDialog_HandleWM_COMMAND);
    // J3D_HOOKFUNC(jonesConfig_ShowStoreDialog);
    // J3D_HOOKFUNC(jonesConfig_StoreDialogProc);
    // J3D_HOOKFUNC(jonesConfig_StoreHandleDragEvent);
    // J3D_HOOKFUNC(jonesConfig_StoreDialog_HandleWM_MBUTTONUP);
    // J3D_HOOKFUNC(jonesConfig_StoreDialog_HandleWM_MOUSEFIRST);
    // J3D_HOOKFUNC(jonesConfig_InitStoreDialog);
    // J3D_HOOKFUNC(jonesConfig_StoreInitItemIcons);
    // J3D_HOOKFUNC(jonesConfig_StoreSetListColumns);
    // J3D_HOOKFUNC(jonesConfig_StoreInitItemList);
    // J3D_HOOKFUNC(jonesConfig_UpdateBalances);
    // J3D_HOOKFUNC(jonesConfig_AddStoreCartItem);
    // J3D_HOOKFUNC(jonesConfig_RemoveStoreCartItem);
    // J3D_HOOKFUNC(jonesConfig_StoreDialog_HandleWM_COMMAND);
    // J3D_HOOKFUNC(jonesConfig_ClearStoreCart);
    // J3D_HOOKFUNC(jonesConfig_ShowPurchaseMessageBox);
    // J3D_HOOKFUNC(jonesConfig_PurchaseMessageBoxProc);
    // J3D_HOOKFUNC(jonesConfig_InitPurchaseMessageBox);
    // J3D_HOOKFUNC(jonesConfig_ShowDialogInsertCD);
    // J3D_HOOKFUNC(jonesConfig_DialogInsertCDProc);
    // J3D_HOOKFUNC(jonesConfig_InitDialogInsertCD);
    // J3D_HOOKFUNC(jonesConfig_InsertCD_HandleWM_COMMAND);
}

void jonesConfig_ResetGlobals(void)
{
    StdRect jonesConfig_aNumberGlyphMetrics_tmp[10] = {
      { 11, 2, 44, 45 },
      { 49, 2, 69, 45 },
      { 78, 2, 106, 45 },
      { 110, 2, 138, 45 },
      { 11, 76, 44, 120 },
      { 45, 76, 75, 120 },
      { 77, 76, 107, 120 },
      { 11, 150, 45, 194 },
      { 45, 150, 75, 194 },
      { 76, 150, 107, 194 }
    };
    memcpy(&jonesConfig_aNumberGlyphMetrics, &jonesConfig_aNumberGlyphMetrics_tmp, sizeof(jonesConfig_aNumberGlyphMetrics));

    char* jonesConfig_apDialogIconFiles_tmp[6] = {
      "iq.bmp",
      "iqMask.bmp",
      "iqOverlay.bmp",
      "numbers.bmp",
      "exit.bmp",
      "exitmask.bmp"
    };
    memcpy(&jonesConfig_apDialogIconFiles, &jonesConfig_apDialogIconFiles_tmp, sizeof(jonesConfig_apDialogIconFiles));

    const char* jonesConfig_aLevelNames_tmp[17] = {
      "JONES_STR_CYN",
      "JONES_STR_BAB",
      "JONES_STR_RIV",
      "JONES_STR_SHS",
      "JONES_STR_LAG",
      "JONES_STR_VOL",
      "JONES_STR_TEM",
      "JONES_STR_JEP",
      "JONES_STR_TEO",
      "JONES_STR_OLV",
      "JONES_STR_SEA",
      "JONES_STR_PYR",
      "JONES_STR_SOL",
      "JONES_STR_NUB",
      "JONES_STR_INF",
      "JONES_STR_AET",
      "JONES_STR_PRU"
    };
    memcpy((char**)&jonesConfig_aLevelNames, &jonesConfig_aLevelNames_tmp, sizeof(jonesConfig_aLevelNames));

    const char* jonesConfig_JONES_STR_SUMMERY_tmp = "JONES_STR_SUMMARY";
    memcpy((char**)&jonesConfig_JONES_STR_SUMMERY, &jonesConfig_JONES_STR_SUMMERY_tmp, sizeof(jonesConfig_JONES_STR_SUMMERY));

    const char* jonesConfig_aJonesControlActionNames_tmp[37] = {
      "JONES_STR_WLKFWD",
      "JONES_STR_WLKBK",
      "JONES_STR_TRNLFT",
      "JONES_STR_TRNRGHT",
      "JONES_STR_STPLFT",
      "JONES_STR_STPRGHT",
      "JONES_STR_CRAWL",
      "JONES_STR_RUN",
      "JONES_STR_ROLL",
      "JONES_STR_JUMP",
      "JONES_STR_LOOK",
      "JONES_STR_ACT",
      "JONES_STR_TGLWEAP",
      "JONES_STR_PREVWEAP",
      "JONES_STR_NEXTWEAP",
      "JONES_STR_TGLLGHT",
      "JONES_STR_FISTS",
      "JONES_STR_WHIP",
      "JONES_STR_MAUSER",
      "JONES_STR_PPSH41",
      "JONES_STR_PISTOL",
      "JONES_STR_SIMONOV",
      "JONES_STR_TOKEREV",
      "JONES_STR_TOZ34",
      "JONES_STR_BAZOOKA",
      "JONES_STR_MACHETE",
      "JONES_STR_SATCHEL",
      "JONES_STR_GRENADE",
      "JONES_STR_MAP",
      "JONES_STR_INTERFACE",
      "JONES_STR_HEALTH",
      "JONES_STR_IMP1",
      "JONES_STR_IMP2",
      "JONES_STR_IMP3",
      "JONES_STR_IMP4",
      "JONES_STR_IMP5",
      "JONES_STR_CHALK"
    };
    memcpy((char**)&jonesConfig_aJonesControlActionNames, &jonesConfig_aJonesControlActionNames_tmp, sizeof(jonesConfig_aJonesControlActionNames));

    const char* jonesConfig_aJonesCapControlActionNames_tmp[37] = {
      "JONES_STR_CAPS_WLKFWD",
      "JONES_STR_CAPS_WLKBK",
      "JONES_STR_CAPS_TRNLFT",
      "JONES_STR_CAPS_TRNRGHT",
      "JONES_STR_CAPS_STPLFT",
      "JONES_STR_CAPS_STPRGHT",
      "JONES_STR_CAPS_CRAWL",
      "JONES_STR_CAPS_RUN",
      "JONES_STR_CAPS_ROLL",
      "JONES_STR_CAPS_JUMP",
      "JONES_STR_CAPS_LOOK",
      "JONES_STR_CAPS_ACT",
      "JONES_STR_CAPS_TGLWEAP",
      "JONES_STR_CAPS_PREVWEAP",
      "JONES_STR_CAPS_NEXTWEAP",
      "JONES_STR_CAPS_TGLLGHT",
      "JONES_STR_CAPS_FISTS",
      "JONES_STR_CAPS_WHIP",
      "JONES_STR_CAPS_MAUSER",
      "JONES_STR_CAPS_PPSH41",
      "JONES_STR_CAPS_PISTOL",
      "JONES_STR_CAPS_SIMONOV",
      "JONES_STR_CAPS_TOKEREV",
      "JONES_STR_CAPS_TOZ34",
      "JONES_STR_CAPS_BAZOOKA",
      "JONES_STR_CAPS_MACHETE",
      "JONES_STR_CAPS_SATCHEL",
      "JONES_STR_CAPS_GRENADE",
      "JONES_STR_CAPS_MAP",
      "JONES_STR_CAPS_INTERFACE",
      "JONES_STR_CAPS_HEALTH",
      "JONES_STR_CAPS_IMP1",
      "JONES_STR_CAPS_IMP2",
      "JONES_STR_CAPS_IMP3",
      "JONES_STR_CAPS_IMP4",
      "JONES_STR_CAPS_IMP5",
      "JONES_STR_CAPS_CHALK"
    };
    memcpy((char**)&jonesConfig_aJonesCapControlActionNames, &jonesConfig_aJonesCapControlActionNames_tmp, sizeof(jonesConfig_aJonesCapControlActionNames));

    // TODO: define keysets and uncomment
    /*int *jonesConfig_aDfltKeySets_tmp[3] = {
      &jonesConfig_aDfltKeyboardSets,
      &jonesConfig_aDflt2a4b,
      &jonesConfig_aDflt2a8b
    };
    memcpy(&jonesConfig_aDfltKeySets, &jonesConfig_aDfltKeySets_tmp, sizeof(jonesConfig_aDfltKeySets));*/

    const char* jonesConfig_aDfltKeySetNames_tmp[3] = { "JONES_STR_DFLTKEY", "JONES_STR_DFLT2A4B", "JONES_STR_DFLT2A8B" };
    memcpy((char**)&jonesConfig_aDfltKeySetNames, &jonesConfig_aDfltKeySetNames_tmp, sizeof(jonesConfig_aDfltKeySetNames));

    const char* jonesConfig_aControlKeyStrings_tmp[223] = {
      NULL,
      "JONES_STR_ESCAPE",
      "1",
      "2",
      "3",
      "4",
      "5",
      "6",
      "7",
      "8",
      "9",
      "0",
      "JONES_STR_MINUS",
      "JONES_STR_EQUAL",
      "JONES_STR_BACK",
      "JONES_STR_TAB",
      "Q",
      "W",
      "E",
      "R",
      "T",
      "posY",
      "U",
      "I",
      "O",
      "P",
      "JONES_STR_LEFT_BRCKT",
      "JONES_STR_RIGHT_BRCKT",
      "JONES_STR_ENTER",
      "JONES_STR_LEFT_CTRL",
      "A",
      "S",
      "D",
      "F",
      "G",
      "H",
      "J",
      "K",
      "L",
      ";",
      "'",
      "`",
      "JONES_STR_LEFT_SHIFT",
      "\\",
      "Z",
      "X",
      "C",
      "V",
      "B",
      "N",
      "M",
      "JONES_STR_COMMA",
      ".",
      "/",
      "JONES_STR_RIGHT_SHIFT",
      "JONES_STR_NUMPAD_MULT",
      "JONES_STR_LEFT_ALT",
      "JONES_STR_SPACE",
      "JONES_STR_CAPS",
      "JONES_STR_F1",
      "JONES_STR_F2",
      "JONES_STR_F3",
      "JONES_STR_F4",
      "JONES_STR_F5",
      "JONES_STR_F6",
      "JONES_STR_F7",
      "JONES_STR_F8",
      "JONES_STR_F9",
      "JONES_STR_F10",
      "JONES_STR_NUMLOCK",
      "JONES_STR_SCROLL",
      "JONES_STR_NUMPAD_7",
      "JONES_STR_NUMPAD_8",
      "JONES_STR_NUMPAD_9",
      "JONES_STR_NUMPAD_MINUS",
      "JONES_STR_NUMPAD_4",
      "JONES_STR_NUMPAD_5",
      "JONES_STR_NUMPAD_6",
      "JONES_STR_NUMPAD_PLUS",
      "JONES_STR_NUMPAD_1",
      "JONES_STR_NUMPAD_2",
      "JONES_STR_NUMPAD_3",
      "JONES_STR_NUMPAD_0",
      "JONES_STR_NUMPAD_DOT",
      NULL,
      NULL,
      NULL,
      "JONES_STR_F11",
      "JONES_STR_F12",
      NULL,
      NULL,
      NULL,
      NULL,
      NULL,
      NULL,
      NULL,
      NULL,
      NULL,
      NULL,
      NULL,
      "JONES_STR_F13",
      "JONES_STR_F14",
      "JONES_STR_F15",
      NULL,
      NULL,
      NULL,
      NULL,
      NULL,
      NULL,
      NULL,
      NULL,
      NULL,
      NULL,
      NULL,
      NULL,
      NULL,
      NULL,
      NULL,
      NULL,
      NULL,
      NULL,
      NULL,
      NULL,
      NULL,
      NULL,
      NULL,
      NULL,
      NULL,
      NULL,
      NULL,
      NULL,
      NULL,
      NULL,
      NULL,
      NULL,
      NULL,
      NULL,
      NULL,
      NULL,
      NULL,
      NULL,
      "JONES_STR_NUMPAD_EQL",
      NULL,
      NULL,
      NULL,
      "@",
      ":",
      "__",
      NULL,
      "STOP",
      NULL,
      NULL,
      NULL,
      NULL,
      NULL,
      NULL,
      "JONES_STR_NUMPAD_ENTER",
      "JONES_STR_RIGHT_CTRL",
      NULL,
      NULL,
      NULL,
      NULL,
      NULL,
      NULL,
      NULL,
      NULL,
      NULL,
      NULL,
      NULL,
      NULL,
      NULL,
      NULL,
      NULL,
      NULL,
      NULL,
      NULL,
      NULL,
      NULL,
      NULL,
      "JONES_STR_NUMPAD_COMMA",
      NULL,
      "JONES_STR_NUMPAD_DIV",
      NULL,
      "JONES_STR_SYSRQ",
      "JONES_STR_RIGHT_ALT",
      NULL,
      NULL,
      NULL,
      NULL,
      NULL,
      NULL,
      NULL,
      NULL,
      NULL,
      NULL,
      NULL,
      NULL,
      NULL,
      NULL,
      "JONES_STR_HOME",
      "JONES_STR_UPARRW",
      "JONES_STR_PAGEUP",
      NULL,
      "JONES_STR_LEFTARRW",
      NULL,
      "JONES_STR_RGHTARRW",
      NULL,
      "JONES_STR_END",
      "JONES_STR_DNARRW",
      "JONES_STR_PGDN",
      "JONES_STR_INSERT",
      "JONES_STR_DELETE",
      NULL,
      NULL,
      NULL,
      NULL,
      NULL,
      NULL,
      NULL,
      "JONES_STR_LEFT_WND",
      "JONES_STR_RIGHT_WND",
      "JONES_STR_APPS",
      NULL
    };
    memcpy((char**)&jonesConfig_aControlKeyStrings, &jonesConfig_aControlKeyStrings_tmp, sizeof(jonesConfig_aControlKeyStrings));

    /*   JonesDialogSize jonesConfig_aFontScaleFactors_tmp[21] = {
         { 164, 368, 232 },
         { 112, 260, 224 },
         { 111, 408, 218 },
         { 116, 297, 95 },
         { 115, 549, 367 },
         { 117, 309, 103 },
         { 120, 282, 152 },
         { 114, 238, 285 },
         { 148, 440, 393 },
         { 113, 351, 206 },
         { 211, 266, 103 },
         { 163, 282, 90 },
         { 214, 282, 90 },
         { 121, 282, 90 },
         { 150, 249, 100 },
         { 233, 344, 180 },
         { 190, 538, 297 },
         { 212, 280, 74 },
         { 159, 583, 250 },
         { 154, 579, 252 },
         { 167, 279, 75 }
       };
       memcpy(&jonesConfig_aDialogSizes, &jonesConfig_aFontScaleFactors_tmp, sizeof(jonesConfig_aDialogSizes));*/

    int jonesConfig_dword_511954_tmp = 1;
    memcpy(&jonesConfig_dword_511954, &jonesConfig_dword_511954_tmp, sizeof(jonesConfig_dword_511954));

    int jonesConfig_dword_511958_tmp = 1;
    memcpy(&jonesConfig_dword_511958, &jonesConfig_dword_511958_tmp, sizeof(jonesConfig_dword_511958));

    int jonesConfig_perfLevel_tmp = 4;
    memcpy(&jonesConfig_perfLevel, &jonesConfig_perfLevel_tmp, sizeof(jonesConfig_perfLevel));

    int jonesConfig_prevLevelNum_tmp = -1;
    memcpy(&jonesConfig_prevLevelNum, &jonesConfig_prevLevelNum_tmp, sizeof(jonesConfig_prevLevelNum));


    jonesConfig_dword_5510B8 = 0;
    jonesConfig_dword_5510BC = 0;
    jonesConfig_dword_5510C0 = 0;
    jonesConfig_dword_5510C4 = 0;
    JonesControlsConfig jonesConfig_curControlConfig_tmp = { NULL, 0, 0, 0, 0, NULL };
    memcpy(&jonesConfig_curControlConfig, &jonesConfig_curControlConfig_tmp, sizeof(jonesConfig_curControlConfig));

    jonesConfig_dword_5510E0 = 0;
    ABC jonesConfig_aGlyphMetrics_tmp[256] = {
      { 0, 0u, 0 },
      { 0, 0u, 0 },
      { 0, 0u, 0 },
      { 0, 0u, 0 },
      { 0, 0u, 0 },
      { 0, 0u, 0 },
      { 0, 0u, 0 },
      { 0, 0u, 0 },
      { 0, 0u, 0 },
      { 0, 0u, 0 },
      { 0, 0u, 0 },
      { 0, 0u, 0 },
      { 0, 0u, 0 },
      { 0, 0u, 0 },
      { 0, 0u, 0 },
      { 0, 0u, 0 },
      { 0, 0u, 0 },
      { 0, 0u, 0 },
      { 0, 0u, 0 },
      { 0, 0u, 0 },
      { 0, 0u, 0 },
      { 0, 0u, 0 },
      { 0, 0u, 0 },
      { 0,  }
    };
    memcpy(&jonesConfig_aGlyphMetrics, &jonesConfig_aGlyphMetrics_tmp, sizeof(jonesConfig_aGlyphMetrics));

    memset(&jonesConfig_textMetric, 0, sizeof(jonesConfig_textMetric));
    memset(&jonesConfig_dword_551D20, 0, sizeof(jonesConfig_dword_551D20));
    memset(&jonesConfig_apDialogIcons, 0, sizeof(jonesConfig_apDialogIcons));
    memset(&jonesConfig_pKeySetsDirPath, 0, sizeof(jonesConfig_pKeySetsDirPath));
    memset(&jonesConfig_bDefaultRun, 0, sizeof(jonesConfig_bDefaultRun));
    memset(&jonesConfig_curLevelNum, 0, sizeof(jonesConfig_curLevelNum));
    memset(&jonesConfig_bStartup, 0, sizeof(jonesConfig_bStartup));
    memset(&jonesConfig_dword_551DCC, 0, sizeof(jonesConfig_dword_551DCC));
    memset(&jonesConfig_hdo_551DD0, 0, sizeof(jonesConfig_hdo_551DD0));
    memset(&jonesConfig_hFontSaveGameDlg, 0, sizeof(jonesConfig_hFontSaveGameDlg));
    memset(&jonesConfig_hFontExitDlg, 0, sizeof(jonesConfig_hFontExitDlg));
    memset(&jonesConfig_hFontGameSaveMsgBox, 0, sizeof(jonesConfig_hFontGameSaveMsgBox));
    memset(&jonesConfig_hFontLoadGameDlg, 0, sizeof(jonesConfig_hFontLoadGameDlg));
    //memset(&jonesConfig_hFontGamePlayOptionsDlg, 0, sizeof(jonesConfig_hFontGamePlayOptionsDlg));
    memset(&jonesConfig_hFontControlOptions, 0, sizeof(jonesConfig_hFontControlOptions));
    memset(&jonesConfig_dword_551DEC, 0, sizeof(jonesConfig_dword_551DEC));
    memset(&jonesConfig_hFontCreateControlSchemeDlg, 0, sizeof(jonesConfig_hFontCreateControlSchemeDlg));
    memset(&jonesConfig_hFontEditControlShceme, 0, sizeof(jonesConfig_hFontEditControlShceme));
    memset(&jonesConfig_hFontAssignKeyDlg, 0, sizeof(jonesConfig_hFontAssignKeyDlg));
    memset(&jonesConfig_bControlsActive, 0, sizeof(jonesConfig_bControlsActive));
    memset(&jonesConfig_hFontReassignKeyDlg, 0, sizeof(jonesConfig_hFontReassignKeyDlg));
    memset(&jonesConfig_hFontDisplaySettingsDlg, 0, sizeof(jonesConfig_hFontDisplaySettingsDlg));
    memset(&jonesConfig_hFontAdvanceDisplaySettingsDialog, 0, sizeof(jonesConfig_hFontAdvanceDisplaySettingsDialog));
    memset(&jonesConfig_hFontSoundSettings, 0, sizeof(jonesConfig_hFontSoundSettings));
    memset(&jonesConfig_hSndChannel, 0, sizeof(jonesConfig_hSndChannel));
    memset(&jonesConfig_dword_551E14, 0, sizeof(jonesConfig_dword_551E14));
    memset(&jonesConfig_hFontGameOverDialog, 0, sizeof(jonesConfig_hFontGameOverDialog));
    memset(&jonesConfig_dword_551E1C, 0, sizeof(jonesConfig_dword_551E1C));
    memset(&jonesConfig_hFontLevelCopletedDialog, 0, sizeof(jonesConfig_hFontLevelCopletedDialog));
    memset(&jonesConfig_hFontStoreDialog, 0, sizeof(jonesConfig_hFontStoreDialog));
    memset(&jonesConfig_hFontPurchaseMessageBox, 0, sizeof(jonesConfig_hFontPurchaseMessageBox));
    memset(&jonesConfig_hFontDialogInsertCD, 0, sizeof(jonesConfig_hFontDialogInsertCD));

}

int jonesConfig_Startup(void)
{
    return J3D_TRAMPOLINE_CALL(jonesConfig_Startup);
}

int J3DAPI jonesConfig_InitKeySetsPath()
{
    return J3D_TRAMPOLINE_CALL(jonesConfig_InitKeySetsPath);
}

int J3DAPI jonesConfig_InitKeyActions()
{
    return J3D_TRAMPOLINE_CALL(jonesConfig_InitKeyActions);
}

void jonesConfig_Shutdown(void)
{
    J3D_TRAMPOLINE_CALL(jonesConfig_Shutdown);
}

void J3DAPI jonesConfig_ControlToString(unsigned int controlId, char* pDest)
{
    J3D_TRAMPOLINE_CALL(jonesConfig_ControlToString, controlId, pDest);
}

int J3DAPI jonesConfig_ShowMessageDialog(HWND hWnd, const char* pTitle, const char* pText, int iconID)
{
    return J3D_TRAMPOLINE_CALL(jonesConfig_ShowMessageDialog, hWnd, pTitle, pText, iconID);
}

BOOL CALLBACK jonesConfig_MessageDialogProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM a4)
{
    return J3D_TRAMPOLINE_CALL(jonesConfig_MessageDialogProc, hwnd, uMsg, wParam, a4);
}

void J3DAPI jonesConfig_sub_4025F0(HWND hWnd, uint16_t* a2)
{
    J3D_TRAMPOLINE_CALL(jonesConfig_sub_4025F0, hWnd, a2);
}

int J3DAPI jonesConfig_sub_402670(HWND hDlg, int a2, uint16_t* dwNewLong)
{
    return J3D_TRAMPOLINE_CALL(jonesConfig_sub_402670, hDlg, a2, dwNewLong);
}

int J3DAPI jonesConfig_SetTextControl(HWND hDlg, HWND hWnd, const char* aText)
{
    return J3D_TRAMPOLINE_CALL(jonesConfig_SetTextControl, hDlg, hWnd, aText);
}

BOOL J3DAPI jonesConfig_sub_402A90(HWND hDlg, int nResult)
{
    return J3D_TRAMPOLINE_CALL(jonesConfig_sub_402A90, hDlg, nResult);
}

JonesControlsScheme* J3DAPI jonesConfig_NewControlScheme()
{
    return J3D_TRAMPOLINE_CALL(jonesConfig_NewControlScheme);
}

int J3DAPI jonesConfig_LoadConstrolsScheme(const char* pFilePath, JonesControlsScheme* pConfig)
{
    return J3D_TRAMPOLINE_CALL(jonesConfig_LoadConstrolsScheme, pFilePath, pConfig);
}

int J3DAPI jonesConfig_GetEntryIndex(int* argNum, StdConffileEntry* pEntry, int a3, const char** apStringList, int size)
{
    return J3D_TRAMPOLINE_CALL(jonesConfig_GetEntryIndex, argNum, pEntry, a3, apStringList, size);
}

int J3DAPI jonesConfig_GetValueIndex(char* pValue, const char** apStringList, int size)
{
    return J3D_TRAMPOLINE_CALL(jonesConfig_GetValueIndex, pValue, apStringList, size);
}

int J3DAPI jonesConfig_GetControllerKeySchemeIndex(int* pArgNum, StdConffileEntry* pEntry)
{
    return J3D_TRAMPOLINE_CALL(jonesConfig_GetControllerKeySchemeIndex, pArgNum, pEntry);
}

int J3DAPI jonesConfig_SetDefaultControlScheme(JonesControlsScheme* pScheme, int num)
{
    return J3D_TRAMPOLINE_CALL(jonesConfig_SetDefaultControlScheme, pScheme, num);
}

void J3DAPI jonesConfig_BindControls(JonesControlsScheme* pConfig)
{
    J3D_TRAMPOLINE_CALL(jonesConfig_BindControls, pConfig);
}

void J3DAPI jonesConfig_BindJoystickControl(SithControlFunction functionId, int controlId)
{
    J3D_TRAMPOLINE_CALL(jonesConfig_BindJoystickControl, functionId, controlId);
}

//HFONT J3DAPI jonesConfig_InitDialog(HWND hWnd, HFONT hFont, int dlgID)
//{
//    return J3D_TRAMPOLINE_CALL(jonesConfig_InitDialog, hWnd, hFont, dlgID);
//}

//HFONT J3DAPI jonesConfig_CreateDialogFont(HWND hWnd, int bWindowMode, int dlgResNum, float* pFontScale)
//{
//    return J3D_TRAMPOLINE_CALL(jonesConfig_CreateDialogFont, hWnd, bWindowMode, dlgResNum, pFontScale);
//}
//
//void J3DAPI jonesConfig_ResetDialogFont(HWND hWndParent, HFONT hFont)
//{
//    J3D_TRAMPOLINE_CALL(jonesConfig_ResetDialogFont, hWndParent, hFont);
//}
//
//int __stdcall jonesConfig_SetWindowFontCallback(HWND hWnd, LPARAM lparam)
//{
//    return J3D_TRAMPOLINE_CALL(jonesConfig_SetWindowFontCallback, hWnd, lparam);
//}
//
//BOOL __stdcall jonesConfig_SetPositionAndTextCallback(HWND hwnd, JonesDialogFontInfo* lparam)
//{
//    return J3D_TRAMPOLINE_CALL(jonesConfig_SetPositionAndTextCallback, hwnd, lparam);
//}
//
//void J3DAPI jonesConfig_SetWindowFontAndPosition(HWND hwnd, JonesDialogFontInfo* pFontInfo)
//{
//    J3D_TRAMPOLINE_CALL(jonesConfig_SetWindowFontAndPosition, hwnd, pFontInfo);
//}
//
//void J3DAPI jonesConfig_GetWindowScreenRect(HWND hWnd, LPRECT lpRect)
//{
//    J3D_TRAMPOLINE_CALL(jonesConfig_GetWindowScreenRect, hWnd, lpRect);
//}
//
//void J3DAPI jonesConfig_SetDialogTitleAndPosition(HWND hWnd, JonesDialogFontInfo* pDlgFontInfo)
//{
//    J3D_TRAMPOLINE_CALL(jonesConfig_SetDialogTitleAndPosition, hWnd, pDlgFontInfo);
//}

int J3DAPI jonesConfig_GetSaveGameFilePath(HWND hWnd, char* pOutFilePath)
{
    return J3D_TRAMPOLINE_CALL(jonesConfig_GetSaveGameFilePath, hWnd, pOutFilePath);
}

//UINT_PTR CALLBACK jonesConfig_SaveGameDialogHookProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam)
//{
//    return J3D_TRAMPOLINE_CALL(jonesConfig_SaveGameDialogHookProc, hDlg, msg, wParam, lParam);
//}
//
//int J3DAPI jonesConfig_SaveGameDialogInit(HWND hDlg, int a2, LPOPENFILENAMEA lpOfn)
//{
//    return J3D_TRAMPOLINE_CALL(jonesConfig_SaveGameDialogInit, hDlg, a2, lpOfn);
//}

//LRESULT CALLBACK jonesConfig_SaveGameThumbnailPaintProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
//{
//    return J3D_TRAMPOLINE_CALL(jonesConfig_SaveGameThumbnailPaintProc, hWnd, uMsg, wParam, lParam);
//}

int J3DAPI jonesConfig_ShowOverwriteSaveGameDlg(HWND hWnd, const char* aFilePath)
{
    return J3D_TRAMPOLINE_CALL(jonesConfig_ShowOverwriteSaveGameDlg, hWnd, aFilePath);
}

int J3DAPI jonesConfig_ShowExitGameDialog(HWND hWnd, char* pSaveGameFilePath)
{
    return J3D_TRAMPOLINE_CALL(jonesConfig_ShowExitGameDialog, hWnd, pSaveGameFilePath);
}

INT_PTR CALLBACK jonesConfig_ExitGameDlgProc(HWND hWnd, UINT uMsg, WPARAM wparam, LPARAM lparam)
{
    return J3D_TRAMPOLINE_CALL(jonesConfig_ExitGameDlgProc, hWnd, uMsg, wparam, lparam);
}

INT_PTR CALLBACK jonesConfig_SaveGameMsgBoxProc(HWND hWnd, UINT umsg, WPARAM wParam, LPARAM lParam)
{
    return J3D_TRAMPOLINE_CALL(jonesConfig_SaveGameMsgBoxProc, hWnd, umsg, wParam, lParam);
}

int J3DAPI jonesConfig_GameSaveSetData(HWND hDlg, int a2, GameSaveMsgBoxData* pData)
{
    return J3D_TRAMPOLINE_CALL(jonesConfig_GameSaveSetData, hDlg, a2, pData);
}

void J3DAPI jonesConfig_MsgBoxDlg_HandleWM_COMMAND(HWND hWnd, int nResult)
{
    J3D_TRAMPOLINE_CALL(jonesConfig_MsgBoxDlg_HandleWM_COMMAND, hWnd, nResult);
}

//int J3DAPI jonesConfig_GetLoadGameFilePath(HWND hWnd, char* pDestNdsPath)
//{
//    return J3D_TRAMPOLINE_CALL(jonesConfig_GetLoadGameFilePath, hWnd, pDestNdsPath);
//}

void* J3DAPI jonesConfig_sub_405F60(HWND hWnd)
{
    return J3D_TRAMPOLINE_CALL(jonesConfig_sub_405F60, hWnd);
}

//UINT_PTR __stdcall jonesConfig_LoadGameDialogHookProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lparam)
//{
//    return J3D_TRAMPOLINE_CALL(jonesConfig_LoadGameDialogHookProc, hDlg, uMsg, wParam, lparam);
//}

int J3DAPI jonesConfig_ShowLoadGameWarningMsgBox(HWND hWnd)
{
    return J3D_TRAMPOLINE_CALL(jonesConfig_ShowLoadGameWarningMsgBox, hWnd);
}

//int J3DAPI jonesConfig_LoadGameDialogInit(HWND hDlg, int a2, LPOPENFILENAMEA pofn)
//{
//    return J3D_TRAMPOLINE_CALL(jonesConfig_LoadGameDialogInit, hDlg, a2, pofn);
//}

//LRESULT CALLBACK jonesConfig_LoadGameThumbnailPaintProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
//{
//    return J3D_TRAMPOLINE_CALL(jonesConfig_LoadGameThumbnailPaintProc, hWnd, uMsg, wParam, lParam);
//}

//int J3DAPI jonesConfig_ShowGamePlayOptions(HWND hWnd)
//{
//    return J3D_TRAMPOLINE_CALL(jonesConfig_ShowGamePlayOptions, hWnd);
//}
//
//INT_PTR CALLBACK jonesConfig_GamePlayOptionsProc(HWND hWnd, UINT msg, WPARAM wparam, LPARAM lparam)
//{
//    return J3D_TRAMPOLINE_CALL(jonesConfig_GamePlayOptionsProc, hWnd, msg, wparam, lparam);
//}
//
//LRESULT J3DAPI jonesConfig_HandleWM_HSCROLL(HWND hDlg, HWND hWnd, int a3)
//{
//    return J3D_TRAMPOLINE_CALL(jonesConfig_HandleWM_HSCROLL, hDlg, hWnd, a3);
//}
//
//int J3DAPI jonesConfig_GamePlayOptionsInitDlg(HWND hDlg)
//{
//    return J3D_TRAMPOLINE_CALL(jonesConfig_GamePlayOptionsInitDlg, hDlg);
//}
//
//int J3DAPI jonesConfig_GamePlayOptions_HandleWM_COMMAND(HWND hDlg, int nResult)
//{
//    return J3D_TRAMPOLINE_CALL(jonesConfig_GamePlayOptions_HandleWM_COMMAND, hDlg, nResult);
//}

//void J3DAPI jonesConfig_EnableMouseControl(int bEnable)
//{
//    J3D_TRAMPOLINE_CALL(jonesConfig_EnableMouseControl, bEnable);
//}

void J3DAPI jonesConfig_FreeControlScheme(JonesControlsScheme* pConfig)
{
    J3D_TRAMPOLINE_CALL(jonesConfig_FreeControlScheme, pConfig);
}

void J3DAPI jonesConfig_FreeControlConfigEntry(JonesControlsConfig* pConfig)
{
    J3D_TRAMPOLINE_CALL(jonesConfig_FreeControlConfigEntry, pConfig);
}

int J3DAPI jonesConfig_ShowControlOptions(HWND hWnd)
{
    return J3D_TRAMPOLINE_CALL(jonesConfig_ShowControlOptions, hWnd);
}

int J3DAPI jonesConfig_CopyControlConfig(JonesControlsConfig* pSrc, JonesControlsConfig* pDst)
{
    return J3D_TRAMPOLINE_CALL(jonesConfig_CopyControlConfig, pSrc, pDst);
}

JonesControlsScheme* J3DAPI jonesConfig_CopyControlSchemes(JonesControlsScheme* aSchemes, int start, int sizeScheme)
{
    return J3D_TRAMPOLINE_CALL(jonesConfig_CopyControlSchemes, aSchemes, start, sizeScheme);
}

int J3DAPI jonesConfig_InitControlsConfig()
{
    return J3D_TRAMPOLINE_CALL(jonesConfig_InitControlsConfig);
}

int J3DAPI jonesConfig_LoadControlsSchemesFromSystem(JonesControlsConfig* pConfig)
{
    return J3D_TRAMPOLINE_CALL(jonesConfig_LoadControlsSchemesFromSystem, pConfig);
}

int J3DAPI jonesConfig_SetDefaultControlSchemes(JonesControlsConfig* pConfig)
{
    return J3D_TRAMPOLINE_CALL(jonesConfig_SetDefaultControlSchemes, pConfig);
}

INT_PTR __stdcall jonesConfig_ControlOptionsProc(HWND hwnd, UINT umsg, WPARAM wparam, LPARAM lparam)
{
    return J3D_TRAMPOLINE_CALL(jonesConfig_ControlOptionsProc, hwnd, umsg, wparam, lparam);
}

int J3DAPI jonesConfig_InitControlOptionsDlg(HWND hDlg, int a2, JonesControlsConfig* pConfig)
{
    return J3D_TRAMPOLINE_CALL(jonesConfig_InitControlOptionsDlg, hDlg, a2, pConfig);
}

void J3DAPI jonesConfig_ControlOptions_HandleWM_COMMAND(HWND hWnd, int ctrlID, int a3, int16_t a4)
{
    J3D_TRAMPOLINE_CALL(jonesConfig_ControlOptions_HandleWM_COMMAND, hWnd, ctrlID, a3, a4);
}

int J3DAPI jonesConfig_WriteScheme(JonesControlsScheme* pScheme)
{
    return J3D_TRAMPOLINE_CALL(jonesConfig_WriteScheme, pScheme);
}

int J3DAPI jonesConfig_sub_407F60(HWND hDlg)
{
    return J3D_TRAMPOLINE_CALL(jonesConfig_sub_407F60, hDlg);
}

LRESULT J3DAPI jonesConfig_sub_408000(HWND hDlg, int a2)
{
    return J3D_TRAMPOLINE_CALL(jonesConfig_sub_408000, hDlg, a2);
}

void J3DAPI jonesConfig_sub_408260(HWND hDlg, JonesControlsConfig* a2)
{
    J3D_TRAMPOLINE_CALL(jonesConfig_sub_408260, hDlg, a2);
}

JonesControlsScheme* J3DAPI jonesConfig_sub_408400(JonesControlsConfig* pConfig)
{
    return J3D_TRAMPOLINE_CALL(jonesConfig_sub_408400, pConfig);
}

void J3DAPI jonesConfig_CreateNewSchemeAction(HWND hDlg, JonesControlsConfig* pConfig)
{
    J3D_TRAMPOLINE_CALL(jonesConfig_CreateNewSchemeAction, hDlg, pConfig);
}

int J3DAPI jonesConfig_ShowCreateControlSchemeDialog(HWND hWnd, char* pDstSchemeName)
{
    return J3D_TRAMPOLINE_CALL(jonesConfig_ShowCreateControlSchemeDialog, hWnd, pDstSchemeName);
}

INT_PTR __stdcall jonesConfig_CreateControlSchemeProc(HWND hWnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    return J3D_TRAMPOLINE_CALL(jonesConfig_CreateControlSchemeProc, hWnd, msg, wparam, lparam);
}

int J3DAPI jonesConfig_InitCreateControlScheme(HWND hDlg, int a2, char* pSchemeName)
{
    return J3D_TRAMPOLINE_CALL(jonesConfig_InitCreateControlScheme, hDlg, a2, pSchemeName);
}

void J3DAPI jonesConfig_CreateControlScheme_Handle_WM_COMMAND(HWND hWnd, int16_t nResult)
{
    J3D_TRAMPOLINE_CALL(jonesConfig_CreateControlScheme_Handle_WM_COMMAND, hWnd, nResult);
}

int J3DAPI jonesConfig_ShowEditControlShemeDialog(HWND hWnd, void* dwInitParam)
{
    return J3D_TRAMPOLINE_CALL(jonesConfig_ShowEditControlShemeDialog, hWnd, dwInitParam);
}

INT_PTR __stdcall jonesConfig_EditControlSchemProc(HWND hWnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    return J3D_TRAMPOLINE_CALL(jonesConfig_EditControlSchemProc, hWnd, msg, wparam, lparam);
}

int J3DAPI jonesConfig_InitEditControlSchemeDlg(HWND hDlg, int a2, void* dwNewLong)
{
    return J3D_TRAMPOLINE_CALL(jonesConfig_InitEditControlSchemeDlg, hDlg, a2, dwNewLong);
}

LRESULT J3DAPI jonesConfig_sub_408ED0(HWND hWnd, char* pKey)
{
    return J3D_TRAMPOLINE_CALL(jonesConfig_sub_408ED0, hWnd, pKey);
}

const char** J3DAPI jonesConfig_sub_408FC0(HWND hWnd, int a2, JonesControlsScheme* a3)
{
    return J3D_TRAMPOLINE_CALL(jonesConfig_sub_408FC0, hWnd, a2, a3);
}

char J3DAPI jonesConfig_sub_409200(HWND hWnd, int nResult)
{
    return J3D_TRAMPOLINE_CALL(jonesConfig_sub_409200, hWnd, nResult);
}

LPARAM J3DAPI jonesConfig_sub_409530(HWND hWnd, HWND a2, int a3, int a4)
{
    return J3D_TRAMPOLINE_CALL(jonesConfig_sub_409530, hWnd, a2, a3, a4);
}

int J3DAPI jonesConfig_sub_4096F0(int a1, HWND hWnd, int a3, HWND* a4, uint32_t* a5)
{
    return J3D_TRAMPOLINE_CALL(jonesConfig_sub_4096F0, a1, hWnd, a3, a4, a5);
}

char J3DAPI jonesConfig_sub_4097D0(HWND a1, HWND hWnd, int a3, int a4)
{
    return J3D_TRAMPOLINE_CALL(jonesConfig_sub_4097D0, a1, hWnd, a3, a4);
}

LPARAM J3DAPI jonesConfig_sub_409BC0(int a1, HWND hWnd, int a3, int a4)
{
    return J3D_TRAMPOLINE_CALL(jonesConfig_sub_409BC0, a1, hWnd, a3, a4);
}

int J3DAPI jonesConfig_AssignKeyAction(HWND hWnd, uint32_t* dwInitParam)
{
    return J3D_TRAMPOLINE_CALL(jonesConfig_AssignKeyAction, hWnd, dwInitParam);
}

int J3DAPI jonesConfig_sub_409F70(int a1, int a2, int* a3, int a4, int* a5)
{
    return J3D_TRAMPOLINE_CALL(jonesConfig_sub_409F70, a1, a2, a3, a4, a5);
}

INT_PTR __stdcall jonesConfig_AssignKeyDlgProc(HWND hWnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    return J3D_TRAMPOLINE_CALL(jonesConfig_AssignKeyDlgProc, hWnd, msg, wparam, lparam);
}

int J3DAPI jonesConfig_sub_40A1A0(HWND hDlg, int a2, uint32_t* dwNewLong)
{
    return J3D_TRAMPOLINE_CALL(jonesConfig_sub_40A1A0, hDlg, a2, dwNewLong);
}

void J3DAPI jonesConfig_AssignControlKey(HWND hWnd)
{
    J3D_TRAMPOLINE_CALL(jonesConfig_AssignControlKey, hWnd);
}

int J3DAPI jonesConfig_sub_40A500(int a1)
{
    return J3D_TRAMPOLINE_CALL(jonesConfig_sub_40A500, a1);
}

void J3DAPI jonesConfig_AssignKeyDlg_HandleWM_COMMAND(HWND hWnd, int a2)
{
    J3D_TRAMPOLINE_CALL(jonesConfig_AssignKeyDlg_HandleWM_COMMAND, hWnd, a2);
}

int J3DAPI jonesConfig_ShowReassignKeyMsgBox(HWND hWnd, LPARAM dwInitParam)
{
    return J3D_TRAMPOLINE_CALL(jonesConfig_ShowReassignKeyMsgBox, hWnd, dwInitParam);
}

INT_PTR __stdcall jonesConfig_ReassignKeyDialogProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    return J3D_TRAMPOLINE_CALL(jonesConfig_ReassignKeyDialogProc, hWnd, uMsg, wParam, lParam);
}

int J3DAPI jonesConfig_SetReassignKeyDialogText(HWND hDlg, int a2, void* dwNewLong)
{
    return J3D_TRAMPOLINE_CALL(jonesConfig_SetReassignKeyDialogText, hDlg, a2, dwNewLong);
}

int J3DAPI jonesConfig_sub_40AA10(HWND hWnd, int nResult)
{
    return J3D_TRAMPOLINE_CALL(jonesConfig_sub_40AA10, hWnd, nResult);
}

int J3DAPI jonesConfig_ShowDisplaySettingsDialog(HWND hWnd, StdDisplayEnvironment* pDisplayEnv, JonesDisplaySettings* pDSettings)
{
    return J3D_TRAMPOLINE_CALL(jonesConfig_ShowDisplaySettingsDialog, hWnd, pDisplayEnv, pDSettings);
}

INT_PTR __stdcall jonesConfig_DisplaySettingsDialogProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    return J3D_TRAMPOLINE_CALL(jonesConfig_DisplaySettingsDialogProc, hWnd, uMsg, wParam, lParam);
}

int J3DAPI jonesConfig_sub_40AE90(HWND hDlg, int a2, int* dwNewLong)
{
    return J3D_TRAMPOLINE_CALL(jonesConfig_sub_40AE90, hDlg, a2, dwNewLong);
}

int J3DAPI jonesConfig_sub_40B530(const char* a1, const char* a2, int a3)
{
    return J3D_TRAMPOLINE_CALL(jonesConfig_sub_40B530, a1, a2, a3);
}

int J3DAPI jonesConfig_sub_40B5A0(HWND hWnd, int nResult, int a3, int a4)
{
    return J3D_TRAMPOLINE_CALL(jonesConfig_sub_40B5A0, hWnd, nResult, a3, a4);
}

uint32_t J3DAPI jonesConfig_ShowAdvanceDisplaySettings(HWND hWnd, LPARAM dwInitParam)
{
    return J3D_TRAMPOLINE_CALL(jonesConfig_ShowAdvanceDisplaySettings, hWnd, dwInitParam);
}

INT_PTR __stdcall jonesConfig_AdvanceDisplaySettingsDialog(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    return J3D_TRAMPOLINE_CALL(jonesConfig_AdvanceDisplaySettingsDialog, hWnd, uMsg, wParam, lParam);
}

int J3DAPI jonesConfig_sub_40BC40(int a1, int a2, int a3)
{
    return J3D_TRAMPOLINE_CALL(jonesConfig_sub_40BC40, a1, a2, a3);
}

int J3DAPI jonesConfig_sub_40BCD0(HWND hDlg, int a2, uint32_t* dwNewLong)
{
    return J3D_TRAMPOLINE_CALL(jonesConfig_sub_40BCD0, hDlg, a2, dwNewLong);
}

int J3DAPI jonesConfig_sub_40C090(HWND hWnd, int nResult, int a3, int a4)
{
    return J3D_TRAMPOLINE_CALL(jonesConfig_sub_40C090, hWnd, nResult, a3, a4);
}

int J3DAPI jonesConfig_ShowSoundSettingsDialog(HWND hWnd, float* dwInitParam)
{
    return J3D_TRAMPOLINE_CALL(jonesConfig_ShowSoundSettingsDialog, hWnd, dwInitParam);
}

INT_PTR __stdcall jonesConfig_SoundSettingsDialogProc(HWND hWnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    return J3D_TRAMPOLINE_CALL(jonesConfig_SoundSettingsDialogProc, hWnd, msg, wparam, lparam);
}

int J3DAPI jonesConfig_sub_40C650(HWND hDlg, int a2, LONG dwNewLong)
{
    return J3D_TRAMPOLINE_CALL(jonesConfig_sub_40C650, hDlg, a2, dwNewLong);
}

int J3DAPI jonesConfig_SoundSettings_HandleWM_COMMAND(HWND hWnd, int nResult)
{
    return J3D_TRAMPOLINE_CALL(jonesConfig_SoundSettings_HandleWM_COMMAND, hWnd, nResult);
}

int J3DAPI jonesConfig_ShowGameOverDialog(HWND hWnd, char* pRestoreFilename, tSoundHandle hSndGameOVerMus, tSoundChannelHandle* pSndChnlMus)
{
    return J3D_TRAMPOLINE_CALL(jonesConfig_ShowGameOverDialog, hWnd, pRestoreFilename, hSndGameOVerMus, pSndChnlMus);
}

INT_PTR __stdcall jonesConfig_GameOverDialogProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    return J3D_TRAMPOLINE_CALL(jonesConfig_GameOverDialogProc, hWnd, uMsg, wParam, lParam);
}

void J3DAPI jonesConfig_LoadGameGetLastSavedGamePath(char* pPath, unsigned int size)
{
    J3D_TRAMPOLINE_CALL(jonesConfig_LoadGameGetLastSavedGamePath, pPath, size);
}

int J3DAPI jonesConfig_GameOverDialogInit(HWND hDlg, int a2, GameOverDialogData* pData)
{
    return J3D_TRAMPOLINE_CALL(jonesConfig_GameOverDialogInit, hDlg, a2, pData);
}

void J3DAPI jonesConfig_GameOverDialog_HandleWM_COMMAND(HWND hWnd, uint16_t wParam)
{
    J3D_TRAMPOLINE_CALL(jonesConfig_GameOverDialog_HandleWM_COMMAND, hWnd, wParam);
}

int J3DAPI jonesConfig_ShowStatisticsDialog(HWND hWnd, SithGameStatistics* pStatistics)
{
    return J3D_TRAMPOLINE_CALL(jonesConfig_ShowStatisticsDialog, hWnd, pStatistics);
}

INT_PTR __stdcall jonesConfig_StatisticsDialogProc(HWND hwnd, UINT uMsg, WPARAM wPAram, LPARAM lParam)
{
    return J3D_TRAMPOLINE_CALL(jonesConfig_StatisticsDialogProc, hwnd, uMsg, wPAram, lParam);
}

int J3DAPI jonesConfig_sub_40D100(int a1, HWND hWnd, int a3, int a4)
{
    return J3D_TRAMPOLINE_CALL(jonesConfig_sub_40D100, a1, hWnd, a3, a4);
}

void J3DAPI jonesConfig_UpdateCurrentLevelNum()
{
    J3D_TRAMPOLINE_CALL(jonesConfig_UpdateCurrentLevelNum);
}

int J3DAPI jonesConfig_DrawImageOnDialogItem(HWND hDlg, HDC hdcWnd, HDC hdcCtrl, int nIDDlgItem, HBITMAP hImage, HBITMAP hMask)
{
    return J3D_TRAMPOLINE_CALL(jonesConfig_DrawImageOnDialogItem, hDlg, hdcWnd, hdcCtrl, nIDDlgItem, hImage, hMask);
}

int J3DAPI jonesConfig_SetStatisticsDialogForLevel(HWND hDlg, int levelNum, int* a3)
{
    return J3D_TRAMPOLINE_CALL(jonesConfig_SetStatisticsDialogForLevel, hDlg, levelNum, a3);
}

void J3DAPI jonesConfig_DrawStatisticDialogIQPoints(HWND hwnd, JonesDialogImageInfo** ppImageInfo, int dlgID, int iqpoints)
{
    J3D_TRAMPOLINE_CALL(jonesConfig_DrawStatisticDialogIQPoints, hwnd, ppImageInfo, dlgID, iqpoints);
}

int J3DAPI jonesConfig_InitStatisticDialog(HWND hDlg, int a2, int* pData)
{
    return J3D_TRAMPOLINE_CALL(jonesConfig_InitStatisticDialog, hDlg, a2, pData);
}

void J3DAPI jonesConfig_StatisticProc_HandleWM_COMMAND(HWND hWnd, int16_t wParam)
{
    J3D_TRAMPOLINE_CALL(jonesConfig_StatisticProc_HandleWM_COMMAND, hWnd, wParam);
}

int J3DAPI jonesConfig_ShowLevelCompletedDialog(HWND hWnd, int* pBalance, int* apItemsState, int a4, int elapsedTime, int qiPoints, int numFoundTrasures, int foundTrasureValue, int totalTreasureValue)
{
    return J3D_TRAMPOLINE_CALL(jonesConfig_ShowLevelCompletedDialog, hWnd, pBalance, apItemsState, a4, elapsedTime, qiPoints, numFoundTrasures, foundTrasureValue, totalTreasureValue);
}

int __stdcall jonesConfig_LevelCompletedDialogProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    return J3D_TRAMPOLINE_CALL(jonesConfig_LevelCompletedDialogProc, hWnd, uMsg, wParam, lParam);
}

int J3DAPI jonesConfig_InitLevelCompletedDialog(HWND hDlg, int wParam, tLevelCompletedDialogState* pState)
{
    return J3D_TRAMPOLINE_CALL(jonesConfig_InitLevelCompletedDialog, hDlg, wParam, pState);
}

void J3DAPI jonesConfig_ChapterCompleteDialog_HandleWM_COMMAND(HWND hWnd, int wParam)
{
    J3D_TRAMPOLINE_CALL(jonesConfig_ChapterCompleteDialog_HandleWM_COMMAND, hWnd, wParam);
}

int J3DAPI jonesConfig_ShowStoreDialog(HWND hWnd, int* pBalance, int* pItemsState, int a4)
{
    return J3D_TRAMPOLINE_CALL(jonesConfig_ShowStoreDialog, hWnd, pBalance, pItemsState, a4);
}

int __stdcall jonesConfig_StoreDialogProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    return J3D_TRAMPOLINE_CALL(jonesConfig_StoreDialogProc, hwnd, msg, wparam, lparam);
}

int J3DAPI jonesConfig_StoreHandleDragEvent(HWND hwnd, HWND hDlgCtrl)
{
    return J3D_TRAMPOLINE_CALL(jonesConfig_StoreHandleDragEvent, hwnd, hDlgCtrl);
}

void J3DAPI jonesConfig_StoreDialog_HandleWM_MBUTTONUP(HWND hWnd)
{
    J3D_TRAMPOLINE_CALL(jonesConfig_StoreDialog_HandleWM_MBUTTONUP, hWnd);
}

BOOL J3DAPI jonesConfig_StoreDialog_HandleWM_MOUSEFIRST(HWND hWnd)
{
    return J3D_TRAMPOLINE_CALL(jonesConfig_StoreDialog_HandleWM_MOUSEFIRST, hWnd);
}

int J3DAPI jonesConfig_InitStoreDialog(HWND hDlg, int a2, tStoreCartState* pCart)
{
    return J3D_TRAMPOLINE_CALL(jonesConfig_InitStoreDialog, hDlg, a2, pCart);
}

int J3DAPI jonesConfig_StoreInitItemIcons(HWND hWnd, tStoreCartState* pCart)
{
    return J3D_TRAMPOLINE_CALL(jonesConfig_StoreInitItemIcons, hWnd, pCart);
}

LRESULT J3DAPI jonesConfig_StoreSetListColumns(HWND hList, const char* pColumnName)
{
    return J3D_TRAMPOLINE_CALL(jonesConfig_StoreSetListColumns, hList, pColumnName);
}

void J3DAPI jonesConfig_StoreInitItemList(HWND hWnd, int* apItemsState, int listID)
{
    J3D_TRAMPOLINE_CALL(jonesConfig_StoreInitItemList, hWnd, apItemsState, listID);
}

void J3DAPI jonesConfig_UpdateBalances(HWND hDlg, tStoreCartState* pCart)
{
    J3D_TRAMPOLINE_CALL(jonesConfig_UpdateBalances, hDlg, pCart);
}

void J3DAPI jonesConfig_AddStoreCartItem(HWND hDlg, tStoreCartState* pCart)
{
    J3D_TRAMPOLINE_CALL(jonesConfig_AddStoreCartItem, hDlg, pCart);
}

void J3DAPI jonesConfig_RemoveStoreCartItem(HWND hDlg, tStoreCartState* pCart)
{
    J3D_TRAMPOLINE_CALL(jonesConfig_RemoveStoreCartItem, hDlg, pCart);
}

void J3DAPI jonesConfig_StoreDialog_HandleWM_COMMAND(HWND hWnd, WPARAM wParam)
{
    J3D_TRAMPOLINE_CALL(jonesConfig_StoreDialog_HandleWM_COMMAND, hWnd, wParam);
}

void J3DAPI jonesConfig_ClearStoreCart(HWND hDlg, tStoreCartState* pCart)
{
    J3D_TRAMPOLINE_CALL(jonesConfig_ClearStoreCart, hDlg, pCart);
}

int J3DAPI jonesConfig_ShowPurchaseMessageBox(HWND hWnd, tStoreCartState* dwInitParam)
{
    return J3D_TRAMPOLINE_CALL(jonesConfig_ShowPurchaseMessageBox, hWnd, dwInitParam);
}

INT_PTR __stdcall jonesConfig_PurchaseMessageBoxProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    return J3D_TRAMPOLINE_CALL(jonesConfig_PurchaseMessageBoxProc, hWnd, uMsg, wParam, lParam);
}

int J3DAPI jonesConfig_InitPurchaseMessageBox(HWND hDlg, int a2, tStoreCartState* pCart)
{
    return J3D_TRAMPOLINE_CALL(jonesConfig_InitPurchaseMessageBox, hDlg, a2, pCart);
}

int J3DAPI jonesConfig_ShowDialogInsertCD(HWND hWnd, LPARAM dwInitParam)
{
    return J3D_TRAMPOLINE_CALL(jonesConfig_ShowDialogInsertCD, hWnd, dwInitParam);
}

INT_PTR __stdcall jonesConfig_DialogInsertCDProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    return J3D_TRAMPOLINE_CALL(jonesConfig_DialogInsertCDProc, hWnd, uMsg, wParam, lParam);
}

int J3DAPI jonesConfig_InitDialogInsertCD(HWND hDlg, int a2, int cdNum)
{
    return J3D_TRAMPOLINE_CALL(jonesConfig_InitDialogInsertCD, hDlg, a2, cdNum);
}

int J3DAPI jonesConfig_InsertCD_HandleWM_COMMAND(HWND hWnd, int nResult)
{
    return J3D_TRAMPOLINE_CALL(jonesConfig_InsertCD_HandleWM_COMMAND, hWnd, nResult);
}

HFONT J3DAPI jonesConfig_InitDialog(HWND hWnd, HFONT hFont, int dlgID)
{
    JonesDialogFontInfo fontInfo;
    fontInfo.bWindowMode = 0;
    fontInfo.hFont       = hFont;

    JonesDisplaySettings* pSettings = JonesMain_GetDisplaySettings();
    if ( pSettings )
    {
        fontInfo.bWindowMode = pSettings->bWindowMode;
    }

    static float curFontScale = 0; // Changed: Made variable static to preserve state for cases when hFont != NULL. (See jonesConfig_LoadGameDialogHookProc)
                                   // Note, original code somehow managed to preserve the state of this variable on the stack and retrieve the value from it on another call
    if ( !hFont )
    {
        fontInfo.hFont = jonesConfig_CreateDialogFont(hWnd, fontInfo.bWindowMode, (uint16_t)dlgID, &curFontScale);
    }

    fontInfo.dialogID = HIWORD(dlgID);
    if ( !fontInfo.dialogID )
    {
        fontInfo.dialogID = (uint16_t)dlgID;
    }

    fontInfo.hControlFont = (HFONT)SendMessage(hWnd, WM_GETFONT, 0, 0);
    fontInfo.fontScaleX = curFontScale;
    fontInfo.fontScaleY = curFontScale;
    jonesConfig_SetDialogTitleAndPosition(hWnd, &fontInfo);

    fontInfo.dialogID = dlgID; // Use the original dlgID with potential NOFONTSCALE mask
    EnumChildWindows(hWnd, jonesConfig_SetPositionAndTextCallback, (LPARAM)&fontInfo);
    return fontInfo.hFont;
}

HFONT J3DAPI jonesConfig_CreateDialogFont(HWND hWnd, int bWindowMode, int dlgID, float* pFontScale)
{
    // Function creates dialog font and calculates scale
    // Note the function was adjusted to account for screen DPI

    if ( !hWnd )
    {
        return NULL;
    }

    RECT rect;
    GetClientRect(hWnd, &rect);

    LPCSTR pFontName = jonesString_GetString("JONES_STR_FONT");
    if ( !pFontName )
    {
        STDLOG_ERROR("Error: Could not get font for dlg id:%d\n", dlgID); // Added log
        return NULL;
    }

    LOGFONT lf;
    HFONT hfont = (HFONT)SendMessage(hWnd, WM_GETFONT, 0, 0);
    GetObject(hfont, sizeof(LOGFONT), &lf);

    // Get system DPI first
    // Added: This was added
    UINT systemDPI = GetDpiForWindow(hWnd);
    float dpiScale = (float)systemDPI / USER_DEFAULT_SCREEN_DPI;

    STDLOG_STATUS("height: %i, width:%i, dpi:%d dpiScale:%f\n", rect.bottom, rect.right, systemDPI, dpiScale);

    double dialogRefHeight = 0;
    for ( size_t i = 0; i < STD_ARRAYLEN(jonesConfig_aDialogSizes); ++i )
    {
        if ( jonesConfig_aDialogSizes[i].dialogID == dlgID )
        {
            dialogRefHeight = jonesConfig_aDialogSizes[i].refHeight * dpiScale;
            break;
        }
    }

    double scale = 1.0; // Added: Init to 1.0
    if ( dialogRefHeight > 0 )
    {
        scale = (dialogRefHeight / (double)rect.bottom) * dpiScale;
    }

    int cHeight;
    float cHeightf;
    uint32_t scrwidth, scrheight;

    // Adjust reference heights and widths for DPI
    int scaledRefHeight = (int)(RD_REF_HEIGHT * dpiScale);
    int scaledRefWidth  = (int)(RD_REF_WIDTH * dpiScale);

    if ( bWindowMode || !JonesMain_HasStarted() )
    {
        cHeight = lf.lfHeight;
        RECT rectDskt;
        GetWindowRect(GetDesktopWindow(), &rectDskt);
        scrwidth = rectDskt.right - rectDskt.left;
        scrheight = rectDskt.bottom - rectDskt.top;

        if ( (size_t)(rectDskt.bottom - rectDskt.top) < (size_t)scaledRefHeight )
        {
            SystemParametersInfo(SPI_GETWORKAREA, 0, &rectDskt, 0);
            scrheight = rectDskt.bottom;
        }

        if ( scale > 1.0f )
        {
            cHeightf = (float)lf.lfHeight;
            float heightScale = (float)((double)scrheight / scaledRefHeight);
            if ( lf.lfHeight <= (int)(int32_t)((double)lf.lfHeight * heightScale) )
            {
                cHeightf = (float)((double)lf.lfHeight * heightScale);
            }
        }
        else
        {
            if ( scrheight < scaledRefHeight && scrwidth < scaledRefWidth )
            {
                scale = (float)((double)scrheight / scaledRefHeight * scale);
            }
            cHeightf = (float)((double)lf.lfHeight * scale);
        }
    }
    else
    {
        stdDisplay_GetBackBufferSize(&scrwidth, &scrheight);

        // Calculate base font size considering DPI
        cHeight = -MulDiv(8, systemDPI, USER_DEFAULT_SCREEN_DPI);

        if ( scrheight >= (size_t)scaledRefHeight || scrwidth >= (size_t)scaledRefWidth )
        {
            cHeightf = (float)((double)cHeight * scale);
        }
        else
        {
            if ( scale > 1.0f )
            {
                cHeightf = (float)((double)cHeight * ((double)scrheight / scaledRefHeight));
            }
            else
            {
                scale = (float)((double)scrheight / scaledRefHeight * scale);
                cHeightf = (float)((double)cHeight * scale);
            }
        }
    }

    float cHeighRnd = floorf(cHeightf);
    if ( fabsf(cHeighRnd - cHeightf) < 0.5f )
    {
        cHeightf = cHeighRnd;
    }

    if ( cHeight <= (int)cHeightf )
    {
        cHeight = (int)cHeightf;
    }

    // Adjust minimum height based on DPI
    if ( cHeight > -MulDiv(10, systemDPI, USER_DEFAULT_SCREEN_DPI) ) // Changed to 10 from 9
    {
        cHeight = -MulDiv(10, systemDPI, USER_DEFAULT_SCREEN_DPI);
    }

    if ( bWindowMode || !JonesMain_HasStarted() )
    {
        *pFontScale = (float)scale;
    }
    else if ( scale > 1.0f
        && (scrheight <= (size_t)(scaledRefHeight) || scrwidth <= (size_t)(scaledRefWidth)) )
    {
        *pFontScale = (float)((double)scrheight / (scaledRefHeight));
    }
    else
    {
        *pFontScale = (float)scale;
    }

    // Create DPI-aware font
    LOGFONT lf2 = {
        .lfHeight         = cHeight,
        .lfWidth          = 0,
        .lfEscapement     = 0,
        .lfOrientation    = 0,
        .lfWeight         = FW_REGULAR,
        .lfItalic         = FALSE,
        .lfUnderline      = FALSE,
        .lfStrikeOut      = FALSE,
        .lfCharSet        = DEFAULT_CHARSET,
        .lfOutPrecision   = OUT_DEFAULT_PRECIS,
        .lfClipPrecision  = CLIP_CHARACTER_PRECIS,
        .lfQuality        = PROOF_QUALITY,
        .lfPitchAndFamily = FF_DONTCARE
    };
    STD_STRCPY(lf2.lfFaceName, pFontName);

    return CreateFontIndirect(&lf2);
}

void J3DAPI jonesConfig_ResetDialogFont(HWND hWndParent, HFONT hFont)
{
    JonesDisplaySettings* pSettings = JonesMain_GetDisplaySettings();
    if ( hFont && (!pSettings || !pSettings->bWindowMode) )
    {
        EnumChildWindows(hWndParent, jonesConfig_ResetWindowFontCallback, 0);
        DeleteObject((HGDIOBJ)hFont);
    }
}

BOOL CALLBACK jonesConfig_ResetWindowFontCallback(HWND hWnd, LPARAM lparam)
{
    J3D_UNUSED(lparam);
    HFONT hfont = (HFONT)GetWindowLong(hWnd, GWL_USERDATA);
    SendMessage(hWnd, WM_SETFONT, (WPARAM)hfont, 0);  // 0 - don't repaint
    return TRUE;
}

BOOL CALLBACK jonesConfig_SetPositionAndTextCallback(HWND hwnd, LPARAM lparam)
{
    // Function changes all text of controls from JONES_STR_* to corresponding text

    if ( !lparam ) { // Added: Added check for null
        return TRUE;
    }
    JonesDialogFontInfo* pFontInfo = (JonesDialogFontInfo*)lparam;
    if ( pFontInfo->hFont )
    {
        // Position hwnd based on font
        jonesConfig_SetWindowFontAndPosition(hwnd, pFontInfo);
    }

    if ( pFontInfo->dialogID == 154 || pFontInfo->dialogID == 159 ) // Load/Save dialogs
    {
        return TRUE;
    }

    CHAR aClassName[256] = { 0 };
    GetClassName(hwnd, aClassName, 256);

    CHAR aText[256] = { 0 };
    if ( strncmpi(aClassName, "BUTTON", STD_ARRAYLEN(aClassName)) == 0 )
    {
        GetWindowText(hwnd, aText, STD_ARRAYLEN(aText));
        LPCSTR pText = jonesString_GetString(aText);
        if ( pText )
        {
            SetWindowText(hwnd, pText);
        }

        return TRUE;
    }
    else if ( strncmpi(aClassName, "STATIC", STD_ARRAYLEN(aClassName)) == 0 )
    {
        GetWindowText(hwnd, aText, STD_ARRAYLEN(aText));
        if ( strchr(aText, '%') )
        {
            return TRUE;
        }

        LPCSTR pText = jonesString_GetString(aText);
        if ( pText )
        {
            SetWindowText(hwnd, pText);
        }

        return TRUE;

    }

    return TRUE;
}

void J3DAPI jonesConfig_SetWindowFontAndPosition(HWND hwnd, JonesDialogFontInfo* pFontInfo)
{
    HWND hDlg = GetParent(hwnd);
    // Set font
    if ( pFontInfo->hFont )
    {
        SendMessage(hwnd, WM_SETFONT, (WPARAM)pFontInfo->hFont, 1); // 1 - repaint
        if ( HIWORD(pFontInfo->dialogID) != JONESCONFIG_NOFONTSCALEMASK )
        {
            SetWindowLong(hwnd, GWL_USERDATA, (WPARAM)pFontInfo->hControlFont);
        }
    }

    RECT rectDesktop;
    HWND hwndDesktop  = GetDesktopWindow();
    GetWindowRect(hwndDesktop, &rectDesktop);

    int heightDesktop = rectDesktop.bottom - rectDesktop.top;
    int widthDesktop  = rectDesktop.right - rectDesktop.left;

    RECT rectWindow;
    GetWindowRect(hwnd, &rectWindow);

    RECT rectClient;
    GetClientRect(hwnd, &rectClient);

    RECT rectDlgWindow;
    GetWindowRect(hDlg, &rectDlgWindow);

    if ( !pFontInfo->bWindowMode || pFontInfo->fontScaleX < 1.0f || pFontInfo->fontScaleY < 1.0f )
    {
        int offsetY = 0;
        if ( HIWORD(pFontInfo->dialogID) != JONESCONFIG_NOFONTSCALEMASK )
        {
        LABEL_17:
            HICON hIcon = (HICON)SendMessage(hwnd, STM_GETICON, 0, 0);
            HWND dlgItem = GetDlgItem(hDlg, 1182);
            if ( dlgItem == hwnd )
            {
                hIcon = (HICON)dlgItem;
            }

            HWND hwndResumeBtn = NULL;
            HWND hwndThumbnail = NULL;

            if ( pFontInfo->dialogID == 159 || pFontInfo->dialogID == 154 )// Load/Save dialog
            {
                LONG dlgID = GetWindowLong(hwnd, GWL_USERDATA); // TODO: UNUSED
                J3D_UNUSED(dlgID);

                HWND hwndParent = GetParent(hDlg);
                if ( hwndParent )
                {
                    HWND dlgItem1137 = GetDlgItem(hwndParent, 1137);
                    if ( dlgItem1137 )
                    {
                        if ( dlgItem1137 != hwnd )
                        {
                            RECT rectDlgItem1137Window;
                            GetWindowRect(dlgItem1137, &rectDlgItem1137Window);
                            offsetY = (int)((double)(rectDlgItem1137Window.bottom - rectDlgItem1137Window.top) * (1.0f - pFontInfo->fontScaleX));
                        }
                    }
                }

                if ( hwnd == GetDlgItem(hDlg, 1120) )
                {
                    hwndResumeBtn = hwnd;
                }
                if ( hwnd == GetDlgItem(hDlg, 1163) || hwnd == GetDlgItem(hDlg, 1125) )
                {
                    hwndThumbnail = hwnd; // Load / Save thumbnail
                    offsetY = 0;
                }
            }

            if ( (!hIcon || heightDesktop < (size_t)RD_REF_HEIGHT && widthDesktop < (size_t)RD_REF_WIDTH) && !hwndThumbnail )
            {
                double fontScaleX = pFontInfo->fontScaleX;
                if ( hIcon && heightDesktop < (size_t)RD_REF_HEIGHT && widthDesktop < (size_t)RD_REF_WIDTH )
                {
                    fontScaleX = (double)heightDesktop / RD_REF_HEIGHT;
                }

                rectWindow.bottom = (int32_t)((double)(rectWindow.bottom - rectWindow.top) * fontScaleX) + rectWindow.top;
            }

            if ( (!hIcon || heightDesktop < (size_t)RD_REF_HEIGHT && widthDesktop < (size_t)RD_REF_WIDTH) && !hwndThumbnail && !hwndResumeBtn )
            {
                double fontScaleY = pFontInfo->fontScaleY;
                if ( hIcon && heightDesktop < (size_t)RD_REF_HEIGHT && widthDesktop < (size_t)RD_REF_WIDTH )
                {
                    fontScaleY = (double)widthDesktop / RD_REF_WIDTH;
                }

                rectWindow.right = (int32_t)((double)(rectWindow.right - rectWindow.left) * fontScaleY) + rectWindow.left;
            }

            if ( !hDlg )
            {
                return;
            }

            GetWindowRect(hDlg, &rectDlgWindow);
            jonesConfig_GetWindowScreenRect(hDlg, &rectClient);

            int posX = (int)((double)(rectWindow.left - rectClient.left) * pFontInfo->fontScaleY);
            int posY = (int)((double)(rectWindow.top - rectClient.top) * pFontInfo->fontScaleX);
            if ( offsetY > 0 && !hwndThumbnail )
            {
                posY += offsetY;
            }

            // Set position
            MoveWindow(hwnd, posX, posY, rectWindow.right - rectWindow.left, rectWindow.bottom - rectWindow.top, 1); // 1 - repaint

            GetWindowRect(hwnd, &rectWindow);
            GetClientRect(hwnd, &rectClient);
            if ( rectWindow.bottom <= rectDlgWindow.bottom && rectWindow.right <= rectDlgWindow.right )
            {
                return;
            }

            int dDlgWinX = rectWindow.right - rectDlgWindow.right;
            int dDlgWinY = rectWindow.bottom - rectDlgWindow.bottom;

            int dlgWidth  = rectDlgWindow.right - rectDlgWindow.left;
            int dlgHeight = rectDlgWindow.bottom - rectDlgWindow.top;

            if ( rectWindow.bottom - rectDlgWindow.bottom <= 0 || dDlgWinX <= 0 )
            {
                if ( dDlgWinY <= 0 )
                {
                    MoveWindow(hDlg, rectDlgWindow.left, rectDlgWindow.top, dDlgWinX + dlgWidth, dlgHeight, 1);
                }
                else
                {
                    MoveWindow(hDlg, rectDlgWindow.left, rectDlgWindow.top, dlgWidth, dDlgWinY + dlgHeight, 1);
                }
            }
            else
            {
                MoveWindow(hDlg, rectDlgWindow.left, rectDlgWindow.top, dDlgWinX + dlgWidth, dDlgWinY + dlgHeight, 1);
            }

            return;
        }

        LONG dlgID = GetWindowLong(hwnd, GWL_USERDATA);
        if ( dlgID == 159 || dlgID == 154 )     // load/save dialogs
        {
            SendMessage(hwnd, WM_SETFONT, (WPARAM)pFontInfo->hControlFont, 1);// 1 - marks redraw
            return;
        }

        HWND dlgItem1137  = GetDlgItem(hDlg, 1137);
        if ( dlgItem1137 ) // note dlg item with this ID in resources
        {
            CHAR aClassName[256] = { 0 };
            memset(aClassName, 0, sizeof(aClassName));
            GetClassName(hwnd, aClassName, 256);
            if ( dlgItem1137 != hwnd && strcmp(aClassName, "ToolbarWindow32") )
            {
                RECT rectDlgItem1137Window;
                GetWindowRect(dlgItem1137, &rectDlgItem1137Window);
                offsetY = (int32_t)((double)(rectDlgItem1137Window.bottom - rectDlgItem1137Window.top) * (1.0f - pFontInfo->fontScaleX));
            }

            goto LABEL_17;
        }
    }
}

void J3DAPI jonesConfig_GetWindowScreenRect(HWND hWnd, LPRECT lpRect)
{
    GetClientRect(hWnd, lpRect);

    POINT point = { 0 };
    ClientToScreen(hWnd, &point);
    lpRect->top  = point.y;
    lpRect->left = point.x;

    point.x = lpRect->right;
    point.y = lpRect->bottom;
    ClientToScreen(hWnd, &point);

    lpRect->right  = point.x;
    lpRect->bottom = point.y;
}

void J3DAPI jonesConfig_SetDialogTitleAndPosition(HWND hWnd, JonesDialogFontInfo* pDlgFontInfo)
{
    if ( pDlgFontInfo->dialogID != 154 && pDlgFontInfo->dialogID != 159 && pDlgFontInfo->dialogID != JONESCONFIG_NOFONTSCALEMASK )// if not load/save dialogs
    {
        CHAR aTitleJS[256] = { 0 };
        GetWindowText(hWnd, aTitleJS, STD_ARRAYLEN(aTitleJS));
        if ( strlen(aTitleJS) )
        {
            LPCSTR pTitle = jonesString_GetString(aTitleJS);
            if ( pTitle )
            {
                SetWindowText(hWnd, pTitle);
            }
        }
    }

    RECT rectDeskto;
    HWND hswnDesktop = GetDesktopWindow();
    GetWindowRect(hswnDesktop, &rectDeskto);

    uint32_t width, height;
    height = rectDeskto.bottom - rectDeskto.top;
    width  = rectDeskto.right - rectDeskto.left;

    if ( !pDlgFontInfo->bWindowMode || pDlgFontInfo->fontScaleX < 1.0f || pDlgFontInfo->fontScaleY < 1.0f )
    {
        RECT rectWindow;
        GetWindowRect(hWnd, &rectWindow);

        RECT rectClient;
        GetClientRect(hWnd, &rectClient);

        if ( !pDlgFontInfo->bWindowMode && JonesMain_HasStarted() )
        {
            stdDisplay_GetBackBufferSize(&width, &height);
        }

        if ( pDlgFontInfo->dialogID == 154 || pDlgFontInfo->dialogID == 159 )// load/save dialogs
        {
            int32_t scaledClientWidth = (int32_t)((1.0f - pDlgFontInfo->fontScaleY) * (float)SITHSAVEGAME_THUMB_WIDTH) + rectClient.right;
            int32_t offsetY = (int32_t)((1.0f - pDlgFontInfo->fontScaleX) * (float)SITHSAVEGAME_THUMB_HEIGHT);
            //int32_t scaledClientHeight = offsetY + rectClient.bottom; // TODO: Unused

            LONG winWidth  = rectWindow.right - rectWindow.left - (rectClient.right - scaledClientWidth);
            LONG winHeight = rectWindow.bottom - rectWindow.top + offsetY;

            RECT rectParent;
            jonesConfig_GetWindowScreenRect(GetParent(hWnd), &rectParent);

            RECT rectClientScreen;
            jonesConfig_GetWindowScreenRect(hWnd, &rectClientScreen);

            rectWindow.top    = rectClientScreen.top - rectParent.top;
            rectWindow.left   = rectClientScreen.left - rectParent.left;
            rectWindow.bottom = winHeight + rectClientScreen.top - rectParent.top;
            rectWindow.right  = winWidth + rectClientScreen.left - rectParent.left;
        }
        else
        {
            int32_t scaledClientWidth  = (int32_t)((double)rectClient.right * pDlgFontInfo->fontScaleY);
            int32_t scaledClientHeight = (int32_t)((double)rectClient.bottom * pDlgFontInfo->fontScaleX);
            if ( pDlgFontInfo->dialogID == JONESCONFIG_NOFONTSCALEMASK )
            {
                scaledClientWidth += (int32_t)((1.0f - pDlgFontInfo->fontScaleY) * (float)SITHSAVEGAME_THUMB_WIDTH);
                scaledClientHeight += (int32_t)((1.0f - pDlgFontInfo->fontScaleX) * (float)SITHSAVEGAME_THUMB_HEIGHT);
            }

            LONG winWidth  = rectWindow.right - rectWindow.left - (rectClient.right - scaledClientWidth);
            LONG winHeight = rectWindow.bottom - rectWindow.top - (rectClient.bottom - scaledClientHeight);

            rectWindow.top    = (LONG)(height - winHeight) / 2;
            rectWindow.left   = (LONG)(width - winWidth) / 2;
            rectWindow.bottom = winHeight + rectWindow.top;
            rectWindow.right  = winWidth + rectWindow.left;
        }

        MoveWindow(hWnd, rectWindow.left, rectWindow.top, rectWindow.right - rectWindow.left, rectWindow.bottom - rectWindow.top, 1);
    }
}

UINT_PTR CALLBACK jonesConfig_SaveGameDialogHookProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch ( uMsg )
    {
        case WM_INITDIALOG:
        {
            return jonesConfig_SaveGameDialogInit(hDlg, wParam, (LPOPENFILENAMEA)lParam);
        }
        case WM_COMMAND:
        {
            jonesConfig_sub_405F60(hDlg);
            return 0;
        }
        case WM_CLOSE:
        {
            DeleteObject((HGDIOBJ)jonesConfig_hFontSaveGameDlg);
            jonesConfig_hFontSaveGameDlg = NULL;
            return 0;
        }
        case WM_DESTROY:
        {
            HDC* hdc = (HDC*)GetWindowLong(hDlg, DWL_USER);
            DeleteDC(*hdc);
            *hdc = NULL;
            return 1;
        }
        case WM_PAINT:
        {
            // Fixed: Added full paint logic.
            //        This should fixed painting the file list on windows 11

            PAINTSTRUCT ps;
            /*HDC hdcThumbnail =*/ BeginPaint(hDlg, &ps);

            //LoadGameDialogData* pData = (LoadGameDialogData*)GetWindowLongPtr(hDlg, DWL_USER);
            HWND hThumbnail = GetDlgItem(hDlg, 1125);
            InvalidateRect(hThumbnail, NULL, TRUE);  // Mark for repaint
            UpdateWindow(hThumbnail);

            HWND hIcon = GetDlgItem(hDlg, 1115);
            InvalidateRect(hIcon, NULL, TRUE);  // Mark for repaint
            UpdateWindow(hIcon);

            // End painting
            EndPaint(hDlg, &ps);
            return 1;
        }
        case WM_NOTIFY:
        {
            SaveGameDialogData* pData  = (SaveGameDialogData*)GetWindowLong(hDlg, DWL_USER);
            NMHDR* pNmHdr = (NMHDR*)lParam;

            switch ( pNmHdr->code )
            {
                case CDN_FILEOK:
                {
                    OFNOTIFY* pOfNotify = (OFNOTIFY*)lParam;
                    bool bValidFile     = false;
                    CHAR aPath[128]     = { 0 };
                    char aFilename[128] = { 0 };

                    if ( pOfNotify->lpOFN->nFileOffset < 0x8000 )
                    {
                        CHAR aExtension[128] = { 0 };
                        STD_FORMAT(aExtension, ".%s", pOfNotify->lpOFN->lpstrDefExt);

                        CHAR aSearchPath[128] = { 0 };
                        CommDlg_OpenSave_GetSpec(GetParent(hDlg), aFilename, STD_ARRAYLEN(aFilename));

                        size_t nFileOffset = 0;
                        char* pFilename = strstr(pOfNotify->lpOFN->lpstrFile, aFilename);
                        if ( pFilename )
                        {
                            nFileOffset = pFilename - pOfNotify->lpOFN->lpstrFile;
                        }
                        else
                        {
                            nFileOffset = strlen(pOfNotify->lpOFN->lpstrFile);
                        }

                        STD_STRNCPY(aSearchPath, pOfNotify->lpOFN->lpstrFile, nFileOffset);

                        LPCSTR pFilePart; // TODO: this might be used for file name instead of searching for it below?
                        SearchPath(aSearchPath, aFilename, aExtension, STD_ARRAYLEN(aPath), aPath, &pFilePart);

                        CHAR aCurDir[128] = { 0 };
                        STD_STRCPY(aCurDir, aSearchPath);

                        pFilename = strrchr(pOfNotify->lpOFN->lpstrFile, '\\');
                        if ( pFilename )
                        {
                            nFileOffset = pFilename - pOfNotify->lpOFN->lpstrFile;
                        }
                        else
                        {
                            nFileOffset = strlen(pOfNotify->lpOFN->lpstrFile);
                        }

                        STD_STRNCPY(aSearchPath, pOfNotify->lpOFN->lpstrFile, nFileOffset);

                        if ( pOfNotify->lpOFN->nFileExtension )
                        {
                            pFilename = &pOfNotify->lpOFN->lpstrFile[pOfNotify->lpOFN->nFileExtension];
                            if ( !*pFilename )
                            {
                                pFilename = NULL;
                            }
                        }
                        else
                        {
                            pFilename = NULL;
                        }

                        if ( (SetCurrentDirectory(aSearchPath) || strlen(aPath)) && (!pFilename || strcmp(pFilename, pOfNotify->lpOFN->lpstrDefExt) == 0) )
                        {
                            SetCurrentDirectory(aCurDir);
                            bValidFile = true;
                        }

                        size_t nOccurrence = strcspn(aFilename, "?*");
                        if ( nOccurrence != strlen(aFilename) )
                        {
                            bValidFile = false;
                        }

                        // TODO: Replace const 17 with macro or with some STD_ARRAYLEN(aLevels)
                        for ( size_t levelNum = 0; levelNum < 17u; ++levelNum )
                        {
                            const char* pSaveName = sithGetLevelSaveFilename(levelNum);
                            if ( pSaveName )
                            {
                                char aAutoSaveFilename[128] = { 0 };
                                const char* pAutoSavePrefix = sithGetAutoSaveFilePrefix();
                                STD_FORMAT(aAutoSaveFilename, "%s%s", pAutoSavePrefix, pSaveName);
                                if ( !strcmpi(aFilename, aAutoSaveFilename) )
                                {
                                    bValidFile = false;
                                    break;
                                }
                            }
                        }

                        char aQuckSaveFilename[128] = { 0 };
                        const char* pQuickSavePrefix = sithGetQuickSaveFilePrefix();
                        STD_FORMAT(aQuckSaveFilename, "%s.%s", pQuickSavePrefix, pOfNotify->lpOFN->lpstrDefExt);

                        stdFnames_ChangeExt(aFilename, pOfNotify->lpOFN->lpstrDefExt); // Fixed: Added .nds extension to filename here to make sure filename "QUICKSAVE" without ".nds" extension doesn't slip over
                        if ( strcmpi(aFilename, aQuckSaveFilename) == 0 )
                        {
                            bValidFile = false;
                        }
                    }

                    if ( pOfNotify->lpOFN->nFileOffset < 0x8000 && bValidFile )
                    {
                        if ( strlen(aPath) )
                        {
                            STD_STRCPY(pData->aFilePath, aFilename);
                            stdFnames_ChangeExt(pData->aFilePath, pOfNotify->lpOFN->lpstrDefExt);

                            if ( jonesConfig_ShowOverwriteSaveGameDlg(hDlg, pData->aFilePath) == 1 )
                            {
                                return 0;
                            }
                            else
                            {
                                SetWindowLong(hDlg, DWL_MSGRESULT, 1);
                                return 1;
                            }
                        }
                    }
                    else
                    {
                        char* pFormat = (char*)jonesString_GetString("JONES_STR_INVALIDFILE");
                        if ( pFormat )
                        {
                            memset(aFilename, 0, sizeof(aFilename));
                            CommDlg_OpenSave_GetSpec(GetParent(hDlg), aFilename, STD_ARRAYLEN(aFilename));

                            char aErrorText[256] = { 0 };
                            STD_FORMAT(aErrorText, pFormat, aFilename);

                            jonesConfig_ShowMessageDialog(hDlg, "JONES_STR_SAVEGM", aErrorText, 141);
                        }

                        SetWindowLong(hDlg, DWL_MSGRESULT, 1);
                        return 1;
                    }
                }
                case CDN_INITDONE:
                {
                    HWND hThumbnail = GetDlgItem(hDlg, 1125);
                    SetWindowLong(hThumbnail, GWL_USERDATA, 154);

                    HWND hIcon = GetDlgItem(hDlg, 1115);
                    SetWindowLong(hIcon, GWL_USERDATA, 154);

                    jonesConfig_hFontSaveGameDlg = jonesConfig_InitDialog(GetParent(hDlg), NULL, JONESCONFIG_NOFONTSCALE(154));
                    jonesConfig_hFontSaveGameDlg = jonesConfig_InitDialog(hDlg, jonesConfig_hFontSaveGameDlg, 154);
                    SetWindowPos(hDlg, HWND_TOPMOST, 0, 0, 0, 0, SWP_SHOWWINDOW | SWP_NOMOVE | SWP_NOSIZE);
                    return 1;
                }
            };
            return 0;
        }
    }

    return 0;
}

int J3DAPI jonesConfig_SaveGameDialogInit(HWND hDlg, int a2, LPOPENFILENAMEA lpOfn)
{
    J3D_UNUSED(a2);

    SaveGameDialogData* pData = (SaveGameDialogData*)lpOfn->lCustData;
    pData->hThumbnail = sithGamesave_GetScreenShotBitmap();

    HWND hThumbnail = GetDlgItem(hDlg, 1125);
    pData->hdcThumbnail = CreateCompatibleDC(GetDC(hThumbnail));

    hThumbnail = GetDlgItem(hDlg, 1125); // ??
    pData->pfThumbnailProc = GetWindowLong(hThumbnail, GWL_WNDPROC);
    SetWindowLongPtr(hThumbnail, GWL_WNDPROC, (LONG)jonesConfig_SaveGameThumbnailPaintProc);

    SetWindowLongPtr(hDlg, DWL_USER, (LONG)pData);

    RECT dlgRect;
    GetWindowRect(hDlg, &dlgRect);

    RECT thumbWndRect, thumbRect;;
    GetWindowRect(hThumbnail, &thumbWndRect);
    GetClientRect(hThumbnail, &thumbRect);

    // Changed: Added scale the thumb size to DPI
    UINT systemDPI = GetDpiForWindow(hDlg);
    double dpiScale = (double)systemDPI / USER_DEFAULT_SCREEN_DPI;
    LONG tbImageWidth  = (LONG)ceil((double)SITHSAVEGAME_THUMB_WIDTH * dpiScale);
    LONG tbImageHeight = (LONG)ceil((double)SITHSAVEGAME_THUMB_HEIGHT * dpiScale);

    LONG thumbWidth  = tbImageWidth - (thumbRect.right - thumbRect.left);
    LONG thumbHeight = tbImageHeight - (thumbRect.bottom - thumbRect.top);

    if ( thumbWidth != 0 || thumbHeight != 0 )
    {
        MoveWindow(hDlg, dlgRect.left, dlgRect.top, thumbWidth + dlgRect.right - dlgRect.left, dlgRect.bottom - dlgRect.top, 1);
        MoveWindow(
            hThumbnail,
            thumbWndRect.left - dlgRect.left,
            thumbWndRect.top - dlgRect.top,
            thumbWidth + thumbWndRect.right - thumbWndRect.left,
            thumbHeight + thumbWndRect.bottom - thumbWndRect.top,
            1
        );
    }

    HWND h1152 = GetDlgItem(GetParent(hDlg), 1152);  // some other dlgItem
    SendMessage(h1152, EM_LIMITTEXT, 60u, 0);
    return 1;
}

LRESULT CALLBACK jonesConfig_SaveGameThumbnailPaintProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    HWND hwndParent = (HWND)GetWindowLong(hWnd, GWL_HWNDPARENT);
    SaveGameDialogData* pData = (SaveGameDialogData*)GetWindowLong(hwndParent, DWL_USER);
    if ( uMsg != WM_PAINT || !pData || !pData->hThumbnail )
    {
        if ( !pData ) {
            return 1;
        }
        return CallWindowProc((WNDPROC)pData->pfThumbnailProc, hWnd, uMsg, wParam, lParam);
    }

    PAINTSTRUCT ps;
    HDC hdc = BeginPaint(hWnd, (LPPAINTSTRUCT)&ps);
    SetStretchBltMode(hdc, STRETCH_HALFTONE);
    jonesConfig_DrawImageOnDialogItem(hwndParent, pData->hdcThumbnail, hdc, 1125, pData->hThumbnail, NULL);
    EndPaint(hWnd, &ps);
    return 0;
}

int J3DAPI jonesConfig_GetLoadGameFilePath(HWND hWnd, char* pDestNdsPath)
{
    char aFilterFormatStr[512] = { 0 };
    const char* pFilterStr = jonesString_GetString("JONES_STR_FILTER");
    if ( pFilterStr )
    {
        STD_STRCPY(aFilterFormatStr, pFilterStr);
    }

    char aLoadGameStr[512] = { 0 };
    const char* pLoadGameStr = jonesString_GetString("JONES_STR_LOADGM");
    if ( pLoadGameStr )
    {
        STD_STRCPY(aLoadGameStr, pLoadGameStr);
    }

    if ( !aFilterFormatStr[0] || !aLoadGameStr[0] )
    {
        return 2;
    }

    char aCwd[128] = { 0 };
    GetCurrentDirectory(STD_ARRAYLEN(aCwd), aCwd);

    LoadGameDialogData data = { 0 };
    data.hThumbnail = 0;

    char aFilterStr[128] = { 0 };
    STD_FORMAT(aFilterStr, aFilterFormatStr, " (.nds)");
    STD_FORMAT(aFilterStr, "%s%c%s%c", aFilterStr, '\0', "*.nds", '\0'); // Fixed: filtering only by .nds files. Note, 2 terminal nulls are required, that's way null at the end

    OPENFILENAME ofn = { 0 };
    ofn.lStructSize = sizeof(OPENFILENAME);
    ofn.hwndOwner   = hWnd;
    ofn.hInstance   = (HINSTANCE)GetWindowLong(hWnd, GWL_HINSTANCE);
    ofn.lpstrFilter = aFilterStr;
    //memset(&ofn.lpstrCustomFilter, 0, 12); // ??
    ofn.lpstrCustomFilter = NULL;

    char aLastFile[128] = { 0 };
    const char* pLastFile = sithGamesave_GetLastFilename();
    if ( pLastFile )
    {
        STD_STRCPY(aLastFile, pLastFile);
    }

    ofn.nMaxFile  = STD_ARRAYLEN(aLastFile);
    ofn.lpstrFile = aLastFile;
    if ( strlen(aLastFile) == 0 )
    {
        aLastFile[0] = 0;
    }

    char szFileTitle[128] = { 0 };
    ofn.lpstrFileTitle  = szFileTitle;
    ofn.nMaxFileTitle   = STD_ARRAYLEN(szFileTitle);

    ofn.lpstrInitialDir = sithGetSaveGamesDir();
    ofn.lpstrTitle      = aLoadGameStr;
    ofn.Flags           = OFN_EXPLORER | OFN_NONETWORKBUTTON | OFN_NOVALIDATE | OFN_ENABLETEMPLATE | OFN_ENABLEHOOK | OFN_HIDEREADONLY | OFN_READONLY | OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;
    ofn.nFileOffset     = 0;
    ofn.lpstrDefExt     = "nds";
    ofn.lCustData       = (LPARAM)&data;
    ofn.lpfnHook        = jonesConfig_LoadGameDialogHookProc;
    ofn.lpTemplateName  = MAKEINTRESOURCE(159);

    uint32_t width, height;
    JonesDisplaySettings* pSettings = JonesMain_GetDisplaySettings();
    if ( pSettings && pSettings->bWindowMode || !JonesMain_HasStarted() )
    {
        HWND hwndDesktop = GetDesktopWindow();
        RECT rect;
        GetWindowRect(hwndDesktop, &rect);
        height = rect.bottom - rect.top;
        width = rect.right - rect.left;
    }
    else
    {
        stdDisplay_GetBackBufferSize(&width, &height);
    }

    BOOL bHasFilePath = JonesDialog_ShowFileSelectDialog((LPOPENFILENAMEA)&ofn, 1);
    SetCurrentDirectory(aCwd);
    if ( !bHasFilePath && !data.unknown36 || !pDestNdsPath )
    {
        return 2;
    }

    if ( bHasFilePath )
    {
        stdFnames_ChangeExt(ofn.lpstrFile, "nds");
        stdUtil_StringCopy(pDestNdsPath, 128, ofn.lpstrFile);
    }
    else
    {
        stdFnames_ChangeExt(data.aFilePath, "nds");
        stdUtil_StringCopy(pDestNdsPath, 128, data.aFilePath);
    }

    return 1;
}

UINT_PTR CALLBACK jonesConfig_LoadGameDialogHookProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch ( uMsg )
    {
        case WM_INITDIALOG:
        {
            return jonesConfig_LoadGameDialogInit(hDlg, wParam, (LPOPENFILENAMEA)lParam);
        }
        case WM_COMMAND:
        {
            jonesConfig_sub_405F60(hDlg);
            return FALSE;
        }
        case WM_CLOSE:
        {
            DeleteObject(jonesConfig_hFontLoadGameDlg);
            jonesConfig_hFontLoadGameDlg = NULL;
            return FALSE;
        }
        case WM_DESTROY:
        {
            LoadGameDialogData* pData = (LoadGameDialogData*)GetWindowLongPtr(hDlg, DWL_USER);
            DeleteDC(pData->hdcThumbnail);
            DeleteObject(pData->hThumbnail);
            pData->hThumbnail   = NULL;
            pData->hdcThumbnail = NULL;
            return FALSE;
        }
        case WM_PAINT:
        {
            // Fixed: Added full paint logic.
            //       This should fixed painting the file list on windows 11

            PAINTSTRUCT ps;
            /*HDC hdcThumbnail =*/ BeginPaint(hDlg, &ps);

            LoadGameDialogData* pData = (LoadGameDialogData*)GetWindowLongPtr(hDlg, DWL_USER);

            if ( pData && !pData->bFolderSel )
            {
                HWND hThumbnail = GetDlgItem(hDlg, 1163);
                if ( hThumbnail )
                {
                    InvalidateRect(hThumbnail, NULL, TRUE);  // Mark for repaint
                    UpdateWindow(hThumbnail);  // Force immediate repaint
                }
            }

            HWND hIcon = GetDlgItem(hDlg, 1117);
            if ( hIcon )
            {
                InvalidateRect(hIcon, NULL, TRUE);  // Mark for repaint
                UpdateWindow(hIcon);  // Force immediate repaint
            }

            // End painting
            EndPaint(hDlg, &ps);
            return TRUE;
        }
        case WM_NOTIFY:
        {
            LoadGameDialogData* pData = (LoadGameDialogData*)GetWindowLongPtr(hDlg, DWL_USER);
            NMHDR* pNmHdr = (NMHDR*)lParam;
            STDLOG_STATUS("code: %i\n", pNmHdr->code);

            switch ( pNmHdr->code )
            {
                case CDN_FILEOK:
                {
                    OFNOTIFY* pOfNotify = (OFNOTIFY*)lParam;

                    bool bValidFile = false;
                    CHAR aPath[128] = { 0 };
                    LPOPENFILENAMEA pofn = pOfNotify->lpOFN;
                    if ( pofn->nFileOffset < 0x8000 )
                    {

                        char aExtension[128] = { 0 };
                        STD_FORMAT(aExtension, ".%s", pofn->lpstrDefExt);

                        char aFilename[128] = { 0 };
                        SendMessage(GetParent(hDlg), CDM_GETSPEC, STD_ARRAYLEN(aFilename), (LPARAM)aFilename); // get selected filename

                        char* pStr = strstr(pofn->lpstrFile, aFilename);
                        size_t strLen;
                        if ( pStr )
                        {
                            strLen = pStr - pofn->lpstrFile;
                        }
                        else
                        {
                            strLen = strlen(pofn->lpstrFile);
                        }

                        CHAR aSearchPath[128] = { 0 };
                        STD_STRNCPY(aSearchPath, pofn->lpstrFile, strLen);

                        LPSTR lpFilePart;
                        SearchPath(aSearchPath, aFilename, aExtension, STD_ARRAYLEN(aPath), aPath, &lpFilePart);
                        if ( strlen(aPath) )
                        {
                            bValidFile = true;
                        }

                        if ( strcspn(aFilename, "?*") != strlen(aFilename) )
                        {
                            bValidFile = false;
                        }
                    }

                    const char* pErrorString = NULL;
                    if ( (pofn->nFileOffset < 0x8000) && bValidFile )
                    {
                        if ( strlen(aPath) )
                        {
                            if ( !JonesMain_HasStarted()
                              || (sithPlayer_g_pLocalPlayerThing->flags & SITH_TF_DYING) != 0
                              || (sithPlayer_g_pLocalPlayerThing->flags & SITH_TF_DESTROYED) != 0
                              || jonesConfig_ShowLoadGameWarningMsgBox(hDlg) == 1 )
                            {
                                return FALSE;
                            }

                            SetWindowLong(hDlg, DWL_MSGRESULT, 1);
                            return TRUE;
                        }

                        pErrorString = jonesString_GetString("JONES_STR_NOFILE");
                        if ( !pErrorString )
                        {
                            SetWindowLong(hDlg, DWL_MSGRESULT, 1);
                            return TRUE;
                        }
                    }
                    else
                    {
                        pErrorString = jonesString_GetString("JONES_STR_INVALIDFILE");
                        if ( !pErrorString )
                        {
                            SetWindowLong(hDlg, DWL_MSGRESULT, 1);
                            return TRUE;
                        }
                    }

                    // Get selected filename
                    CHAR aMessage[256]  = { 0 };
                    char aFilename[128] = { 0 };
                    SendMessage(GetParent(hDlg), CDM_GETSPEC, STD_ARRAYLEN(aFilename), (LPARAM)aFilename);
                    STD_FORMAT(aMessage, pErrorString, aFilename);

                    jonesConfig_ShowMessageDialog(hDlg, "JONES_STR_LOADGM", aMessage, 139);
                    SetWindowLong(hDlg, DWL_MSGRESULT, 1);
                    return TRUE;
                }
                case CDN_FOLDERCHANGE:
                {
                    if ( !pData->hThumbnail )
                    {
                        return TRUE;
                    }

                    HWND hDlgItem = GetDlgItem(hDlg, 1163);
                    pData->bFolderSel = 1;

                    InvalidateRect(hDlgItem, NULL, TRUE);
                    UpdateWindow(hDlg);
                    return TRUE;
                }
                case CDN_SELCHANGE:
                {
                    HWND hThumbnail = GetDlgItem(hDlg, 1163);
                    if ( !pData->bFolderSel )
                    {
                        const char* pExt = NULL;
                        SendMessage(GetParent(hDlg), CDM_GETFILEPATH, STD_ARRAYLEN(pData->aFilePath), (LPARAM)pData->aFilePath);
                        if ( strlen(pData->aFilePath) )
                        {
                            pExt = pData->aFilePath + strlen(pData->aFilePath) - 3;
                        }

                        if ( pExt && strcmp(pExt, "nds") == 0 )
                        {
                            // Load thumbnail of newly selected nds file
                            if ( pData->hThumbnail )
                            {
                                DeleteObject(pData->hThumbnail);
                                pData->hThumbnail = NULL;
                            }

                            pData->hThumbnail = sithGamesave_LoadThumbnail(pData->aFilePath);
                        }
                    }

                    InvalidateRect(hThumbnail, NULL, TRUE);
                    UpdateWindow(hDlg);
                    return TRUE;
                }
                case CDN_INITDONE:
                {
                    HWND hBtn = GetDlgItem(hDlg, 1120);
                    SetWindowLong(hBtn, GWL_USERDATA, 159);

                    HWND hIcon = GetDlgItem(hDlg, 1117);
                    SetWindowLong(hIcon, GWL_USERDATA, 159);

                    HWND hThumbnail = GetDlgItem(hDlg, 1163);
                    SetWindowLong(hThumbnail, GWL_USERDATA, 159);
                    InvalidateRect(hThumbnail, NULL, TRUE);

                    static_assert(JONESCONFIG_NOFONTSCALE(159) == 0x45F009F, "");
                    jonesConfig_hFontLoadGameDlg = jonesConfig_InitDialog(GetParent(hDlg), NULL, JONESCONFIG_NOFONTSCALE(159));
                    jonesConfig_hFontLoadGameDlg = jonesConfig_InitDialog(hDlg, jonesConfig_hFontLoadGameDlg, 159);
                    SetWindowPos(hDlg, HWND_TOPMOST, 0, 0, 0, 0, SWP_SHOWWINDOW | SWP_NOMOVE | SWP_NOSIZE);
                    return TRUE;
                }

                default:
                    return FALSE;
            }
        }
    };

    return FALSE;
}

LRESULT CALLBACK jonesConfig_LoadGameThumbnailPaintProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    HWND hwndParent = (HWND)GetWindowLong(hWnd, GWL_HWNDPARENT);
    LoadGameDialogData* pData = (LoadGameDialogData*)GetWindowLongPtr(hwndParent, DWL_USER);
    if ( !pData ) // Fixed: Moved null check to the top
    {
        return 1;
    }

    if ( uMsg != WM_PAINT )
    {
        return CallWindowProc((WNDPROC)pData->pfThubnailProc, hWnd, uMsg, wParam, lParam);
    }

    if ( !pData->hThumbnail )
    {
        return CallWindowProc((WNDPROC)pData->pfThubnailProc, hWnd, uMsg, wParam, lParam); // Added: Should fix rendering of file list on Windows 11
    }

    if ( !pData->bFolderSel )
    {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hWnd, (LPPAINTSTRUCT)&ps);
        SetStretchBltMode(hdc, STRETCH_HALFTONE);
        jonesConfig_DrawImageOnDialogItem(hwndParent, pData->hdcThumbnail, hdc, 1163, pData->hThumbnail, NULL);// 1163 - thumbnail picture edit control
        EndPaint(hWnd, &ps);
        return 0;
    }

    pData->bFolderSel = 0;

    // TODO: useless scope, found only in debug version
#ifdef J3D_DEBUG
    if ( !pData->hThumbnail )
    {
        return CallWindowProc((WNDPROC)pData->pfThubnailProc, hWnd, uMsg, wParam, lParam); // Added: Should fix rendering of file list on Windows 11
    }
#endif

    DeleteObject((HGDIOBJ)pData->hThumbnail);
    pData->hThumbnail = NULL;
    return CallWindowProc((WNDPROC)pData->pfThubnailProc, hWnd, uMsg, wParam, lParam); // Added: Should fix rendering of file list on Windows 11
}

BOOL J3DAPI jonesConfig_LoadGameDialogInit(HWND hDlg, int a2, LPOPENFILENAMEA pofn)
{
    J3D_UNUSED(a2);

    LoadGameDialogData* pData = (LoadGameDialogData*)pofn->lCustData;

    RECT btnRect;
    HWND hButton = GetDlgItem(hDlg, 1120); // load last saved game button 
    GetWindowRect(hButton, &btnRect);

    HWND hThumbnail = GetDlgItem(hDlg, 1163);
    HDC dc = GetDC(hThumbnail);
    pData->hdcThumbnail = CreateCompatibleDC(dc);

    hThumbnail = GetDlgItem(hDlg, 1163); // ??
    pData->pfThubnailProc = GetWindowLongPtr(hThumbnail, GWL_WNDPROC);

    HWND hIcon = GetDlgItem(hDlg, 1117);

    SetWindowLongPtr(hThumbnail, GWL_WNDPROC, (LONG_PTR)jonesConfig_LoadGameThumbnailPaintProc);
    SetWindowLongPtr(hDlg, DWL_USER, (LONG_PTR)pData);

    RECT dlgWinRect;
    GetWindowRect(hDlg, &dlgWinRect);

    RECT thumbWinRect;
    GetWindowRect(hThumbnail, &thumbWinRect);

    RECT thumbRect;
    GetClientRect(hThumbnail, &thumbRect);

    RECT iconWinRect;
    GetWindowRect(hIcon, &iconWinRect);

    RECT iconRect;
    GetClientRect(hIcon, &iconRect);

    // LONG dlgWidth  = dlgWinRect.right - dlgWinRect.left;
    // LONG dlgHeight = dlgWinRect.bottom - dlgWinRect.top;

     // Changed: Scale the thumb size to DPI
    UINT systemDPI = GetDpiForWindow(hDlg);
    double dpiScale = (double)systemDPI / USER_DEFAULT_SCREEN_DPI;
    LONG tbImageWidth  = (LONG)ceil((double)SITHSAVEGAME_THUMB_WIDTH * dpiScale);
    LONG tbImageHeight = (LONG)ceil((double)SITHSAVEGAME_THUMB_HEIGHT * dpiScale);

    LONG thmbWidth  = thumbRect.left - thumbRect.right + tbImageWidth;
    LONG thmbHeight = thumbRect.top - thumbRect.bottom + tbImageHeight;

    //LONG thumbPosX = thumbWinRect.left - dlgWinRect.left;
    //LONG thumbPosY = thumbWinRect.top - dlgWinRect.top;

    //LONG thumbWinWidth  = thumbWinRect.right - thumbWinRect.left;
    //LONG thumbWinHeight = thumbWinRect.bottom - thumbWinRect.top;

    //LONG thumbTotalWidth = thmbWidth + thumbWinWidth;
    //LONG thumbTotalHeight = thmbHeight + thumbWinHeight;

    if ( thumbRect.left - thumbRect.right == -tbImageWidth && thumbRect.top - thumbRect.bottom == -tbImageHeight )
    {
        return TRUE;
    }

    if ( thumbRect.left - thumbRect.right == -SITHSAVEGAME_THUMB_WIDTH && thumbRect.top - thumbRect.bottom == -SITHSAVEGAME_THUMB_HEIGHT )
    {
        return 1;
    }

    MoveWindow(
        hDlg,
        dlgWinRect.left,
        dlgWinRect.top,
        thmbWidth + dlgWinRect.right - dlgWinRect.left,
        dlgWinRect.bottom - dlgWinRect.top,
        1
    );

    MoveWindow(
        hThumbnail,
        thumbWinRect.left - dlgWinRect.left,
        thumbWinRect.top - dlgWinRect.top,
        thmbWidth + thumbWinRect.right - thumbWinRect.left,
        thmbHeight + thumbWinRect.bottom - thumbWinRect.top,
        1
    );

    LONG width = thumbWinRect.right + thmbWidth - thumbWinRect.left;
    MoveWindow(
        hButton,
        thumbWinRect.left - dlgWinRect.left,
        btnRect.top + thmbHeight - dlgWinRect.top,
        width,
        btnRect.bottom - btnRect.top,
        1
    );

    // TODO: dead code as the button is not hooked to any code
    char aPath[128] = { 0 };
    jonesConfig_LoadGameGetLastSavedGamePath(aPath, sizeof(aPath));
    if ( !strlen(aPath) )
    {
        EnableWindow(hButton, 0); // close window if no game was saved yet
    }

    return TRUE;
}

int J3DAPI jonesConfig_ShowGamePlayOptions(HWND hWnd)
{
    GetWindowLongA(hWnd, GWL_HINSTANCE); // TODO: useless
    return JonesDialog_ShowDialog((LPCSTR)112, hWnd, jonesConfig_GamePlayOptionsProc, 0);
}

INT_PTR CALLBACK jonesConfig_GamePlayOptionsProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    int bProcessed = 1;
    if ( uMsg > WM_INITDIALOG )
    {
        if ( uMsg == WM_COMMAND )
        {
            jonesConfig_GamePlayOptions_HandleWM_COMMAND(hWnd, LOWORD(wParam));
            return 0;
        }

        if ( uMsg != WM_HSCROLL )
        {
            return 0;
        }


        jonesConfig_HandleWM_HSCROLL(hWnd, (HWND)lParam, LOWORD(wParam));

        HWND hDifSlideer = GetDlgItem(hWnd, 1050);   // Difficulty slider control

        const char* pDifficultyStr = NULL;
        int difficulty = SendMessageA(hDifSlideer, TBM_GETPOS, 0, 0);
        switch ( difficulty )
        {
            case 0:
                pDifficultyStr = jonesString_GetString("JONES_STR_HARD0");
                break;

            case 1:
                pDifficultyStr = jonesString_GetString("JONES_STR_HARD1");
                break;

            case 2:
                pDifficultyStr = jonesString_GetString("JONES_STR_HARD2");
                break;

            case 3:
                pDifficultyStr = jonesString_GetString("JONES_STR_HARD3");
                break;

            case 4:
                pDifficultyStr = jonesString_GetString("JONES_STR_HARD4");
                break;

            case 5:
                pDifficultyStr = jonesString_GetString("JONES_STR_HARD5");
                break;

            default:
                break;
        }

        if ( pDifficultyStr )
        {
            HWND hDifText = GetDlgItem(hWnd, 1215); // Difficulty text control
            SetWindowTextA(hDifText, pDifficultyStr);
        }

        return 1;
    }

    if ( uMsg == WM_INITDIALOG )
    {
        // Changed: Change the order of initialization, to first init dialog 
        //          and then do general dialog init which scales the fonts and inits dialog text
        bProcessed = jonesConfig_GamePlayOptionsInitDlg(hWnd);
        jonesConfig_hFontGamePlayOptionsDlg = jonesConfig_InitDialog(hWnd, NULL, 112);
        SetWindowPos(hWnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_SHOWWINDOW | SWP_NOMOVE | SWP_NOSIZE);
    }
    else
    {
        if ( uMsg != WM_DESTROY )
        {
            return 0;
        }

        jonesConfig_ResetDialogFont(hWnd, jonesConfig_hFontGamePlayOptionsDlg);
    }

    return bProcessed;
}

void J3DAPI jonesConfig_HandleWM_HSCROLL(HWND hDlg, HWND hWnd, uint16_t sbValue)
{
    J3D_UNUSED(hDlg);

    const int maxPos = SendMessageA(hWnd, TBM_GETRANGEMAX, 0, 0);
    const int minPos = SendMessageA(hWnd, TBM_GETRANGEMIN, 0, 0);
    const int curPos = SendMessageA(hWnd, TBM_GETPOS, 0, 0);
    SendMessageA(hWnd, TBM_GETPAGESIZE, 0, 0);  // ???

    int newPos = -1;
    switch ( sbValue )
    {
        case SB_THUMBPOSITION:
        case SB_THUMBTRACK:
            newPos = curPos;
            break;

        case SB_LEFT:
            newPos = minPos;
            break;

        case SB_RIGHT:
            newPos = maxPos;
            break;

        default:
            break;
    }

    if ( newPos >= 0 )
    {
        SendMessageA(hWnd, TBM_SETPOS, /*redraw=*/1u, newPos);
    }
}

int J3DAPI jonesConfig_GamePlayOptionsInitDlg(HWND hDlg)
{
    // Shot Text option
    HWND hCBShowText = GetDlgItem(hDlg, 1052);
    int bShowText = sithVoice_GetShowText();
    SendMessageA(hCBShowText, BM_SETCHECK, bShowText, 0);

    // Show Hints option
    HWND hCBShowHints = GetDlgItem(hDlg, 1051);
    int bShowHints = sithOverlayMap_GetShowHints();
    SendMessageA(hCBShowHints, BM_SETCHECK, bShowHints, 0);

    // RotateMap option
    HWND hCBRotateMap = GetDlgItem(hDlg, 1204);
    int  bRotateMap = sithOverlayMap_GetMapRotation();
    SendMessageA(hCBRotateMap, BM_SETCHECK, bRotateMap, 0);

    // Default to Run option
    CheckDlgButton(hDlg, 1202, jonesConfig_bDefaultRun);// CB default run

    // Added: Check box "High Poly Objects"
    // Get the position of the base checkbox
    RECT rectCBShowText;
    GetWindowRect(hCBShowText, &rectCBShowText);

    // Convert screen coordinates to client coordinates
    POINT pt = { rectCBShowText.left, rectCBShowText.top };
    ScreenToClient(hDlg, &pt);

    // Calculate the position for the new checkbox
    int x = pt.x;
    int y = pt.y + (rectCBShowText.bottom - rectCBShowText.top) + 14;

    // Create the checkbox
    HWND hCBHiPoly = CreateWindow(
        "BUTTON",               // Class name for button/checkbox
        "JONES_STR_HIPOLY",     // Text displayed on the checkbox
        WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
        x,
        y,
        150,                   // Width
        20,                    // Height
        hDlg,                  // Parent window handle (the dialog)
        (HMENU)1053,           // Control ID
        GetModuleHandle(NULL), // Instance handle
        NULL                   // Additional creation data
    );
    J3D_UNUSED(hCBHiPoly);

    CheckDlgButton(hDlg, 1053, sithModel_IsHiPolyEnabled());

    // Difficulty slider and text
    HWND hDifSlider = GetDlgItem(hDlg, 1050); // Difficulty slider control

    size_t maxDifficulty = jonesInventory_GetMaxDifficultyLevel();
    SendMessageA(hDifSlider, TBM_SETRANGE, 1u, (uint16_t)maxDifficulty << 16);// LWORD is min and HIWORD is max range

    int curDifficulty = sithGetGameDifficulty();
    SendMessageA(hDifSlider, TBM_SETPOS, 1u, curDifficulty);

    SendMessageA(hDifSlider, TBM_SETTICFREQ, 1u, 0);
    SendMessageA(hDifSlider, TBM_SETPAGESIZE, 0, 1);


    const char* pDifficultyStr = NULL;
    switch ( sithGetGameDifficulty() )
    {
        case 0:
            pDifficultyStr = jonesString_GetString("JONES_STR_HARD0");
            break;

        case 1:
            pDifficultyStr = jonesString_GetString("JONES_STR_HARD1");
            break;

        case 2:
            pDifficultyStr = jonesString_GetString("JONES_STR_HARD2");
            break;

        case 3:
            pDifficultyStr = jonesString_GetString("JONES_STR_HARD3");
            break;

        case 4:
            pDifficultyStr = jonesString_GetString("JONES_STR_HARD4");
            break;

        case 5:
            pDifficultyStr = jonesString_GetString("JONES_STR_HARD5");
            break;

        default:
            break;
    }

    if ( pDifficultyStr )
    {
        HWND  hDifText = GetDlgItem(hDlg, 1215); // Difficulty text control
        SetWindowTextA(hDifText, pDifficultyStr);
    }


    // Added: Resize dialog to fit in new check box
    RECT rectDlg;
    GetWindowRect(hDlg, &rectDlg);
    SetWindowPos(hDlg, NULL, 0, 0, rectDlg.right - rectDlg.left, (rectDlg.bottom - rectDlg.top) + 28, SWP_NOZORDER | SWP_NOMOVE | SWP_NOACTIVATE);

    // Added: Move OK & Cancel buttons down
    HWND hBtnOk     = GetDlgItem(hDlg, 1);
    RECT rectBtnOk;
    GetWindowRect(hBtnOk, &rectBtnOk);
    POINT ptBtnOk = { rectBtnOk.left, rectBtnOk.top + (rectBtnOk.bottom - rectBtnOk.top) / 2 + 14 };
    ScreenToClient(hDlg, &ptBtnOk);
    SetWindowPos(hBtnOk, NULL, ptBtnOk.x, ptBtnOk.y, 0, 0, SWP_NOZORDER | SWP_NOSIZE | SWP_NOACTIVATE);

    HWND hBtnCancel = GetDlgItem(hDlg, 2);
    RECT rectBtnCancel;
    GetWindowRect(hBtnCancel, &rectBtnCancel);
    POINT ptBtnCancel = { rectBtnCancel.left, rectBtnCancel.top + (rectBtnCancel.bottom - rectBtnCancel.top) / 2 + 14 };
    ScreenToClient(hDlg, &ptBtnCancel);
    SetWindowPos(hBtnCancel, NULL, ptBtnCancel.x, ptBtnCancel.y, 0, 0, SWP_NOZORDER | SWP_NOSIZE | SWP_NOACTIVATE);

    return 1;
}

void J3DAPI jonesConfig_GamePlayOptions_HandleWM_COMMAND(HWND hDlg, uint16_t controlID)
{
    if ( controlID == 1 ) // OK
    {
        // Get & save show text option
        HWND hCBShowText = GetDlgItem(hDlg, 1052);
        int bShowText = SendMessageA(hCBShowText, BM_GETSTATE, 0, 0);
        sithVoice_ShowText(bShowText);

        bShowText = sithVoice_GetShowText();
        wuRegistry_SaveIntEx("Show Text", bShowText);

        // Get & save map rotation option
        HWND hCBMapRotate = GetDlgItem(hDlg, 1204);
        int bRotateMap = SendMessageA(hCBMapRotate, BM_GETSTATE, 0, 0);
        sithOverlayMap_EnableMapRotation(bRotateMap);

        bRotateMap = sithOverlayMap_GetMapRotation();
        wuRegistry_SaveIntEx("Map Rotation", bRotateMap);
        // Get & save show hint option
        HWND hCBShowHints = GetDlgItem(hDlg, 1051);
        int bShowHints = SendMessageA(hCBShowHints, BM_GETSTATE, 0, 0);
        sithOverlayMap_SetShowHints(bShowHints);

        bShowHints = sithOverlayMap_GetShowHints();
        wuRegistry_SaveIntEx("Show Hints", bShowHints);
        // Get & save difficulty
        HWND hDifSlideer = GetDlgItem(hDlg, 1050);
        int difficulty = SendMessageA(hDifSlideer, TBM_GETPOS, 0, 0);
        sithSetGameDifficulty(difficulty);

        difficulty = sithGetGameDifficulty();
        wuRegistry_SaveInt("Difficulty", difficulty);

        // Get & save default to run option
        HWND hCBRun = GetDlgItem(hDlg, 1202);
        jonesConfig_bDefaultRun = SendMessageA(hCBRun, BM_GETCHECK, 0, 0);
        wuRegistry_SaveIntEx("Default Run", jonesConfig_bDefaultRun);

        if ( jonesConfig_bDefaultRun )
        {
            sithControl_g_controlOptions |= 0x02;
        }
        else
        {
            sithControl_g_controlOptions &= ~0x02;
        }

        // Added
        // Get & save HiPoly option
        int bCurBHiPoly = sithModel_IsHiPolyEnabled();

        int bHiPoly = IsDlgButtonChecked(hDlg, 1053);
        sithModel_EnableHiPoly(bHiPoly);
        wuRegistry_SaveIntEx("HiPoly", bHiPoly);

        if ( (bHiPoly != 0) != (bCurBHiPoly != 0) )
        {
            const char* pNoteStr = jonesString_GetString("JONES_STR_HIPOLYGO");
            if ( pNoteStr ) {
                pNoteStr = "You must quit and restart the game for the high poly option to take effect.";
            }
            jonesConfig_ShowMessageDialog(hDlg, "JONES_STR_GMPLY_OPTS", pNoteStr, 137);
        }

        // Close dialog
        EndDialog(hDlg, controlID);
    }

    if ( controlID == 2 ) //  CANCEL
    {
        EndDialog(hDlg, controlID);
    }
}

void J3DAPI jonesConfig_EnableMouseControl(int bEnable)
{
    JonesDisplaySettings* pSettings = JonesMain_GetDisplaySettings();
    if ( bEnable )
    {
        sithControl_UnbindMouseAxes();
        sithControl_RegisterMouseBindings();
    }

    else if ( pSettings->bWindowMode )
    {
        stdControl_EnableMouse(0);
    }
    else
    {
        sithControl_UnbindMouseAxes();
        stdControl_EnableMouse(1);
    }
}
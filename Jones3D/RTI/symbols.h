#ifndef JONES3D_RTI_SYMBOLS_H
#define JONES3D_RTI_SYMBOLS_H
#include <Jones3D/types.h>
#include <j3dcore/j3d.h>
#include <rdroid/types.h>
#include <sith/types.h>
#include <std/types.h>

#define JonesConsole_Startup_TYPE int (J3DAPI*)()
#define JonesConsole_Shutdown_TYPE void (*)(void)
#define JonesConsole_Open_TYPE int (*)(void)
#define JonesConsole_Close_TYPE void (*)(void)
#define JonesConsole_FlushToDisplay_TYPE void (*)(void)
#define JonesConsole_PrintText_TYPE size_t (J3DAPI*)(const char*)
#define JonesConsole_PrintTextWithID_TYPE void (J3DAPI*)(int, const char*)
#define JonesConsole_ClearText_TYPE void (J3DAPI*)(int)
#define JonesConsole_ExeCommand_TYPE int (J3DAPI*)(const char*)
#define JonesConsole_ShowConsole_TYPE void (J3DAPI*)()
#define JonesConsole_HandelChar_TYPE void (J3DAPI*)(char)
#define JonesConsole_FlushCommandLine_TYPE void (*)(void)
#define JonesConsole_HideConsole_TYPE void (*)(void)
#define JonesConsole_PrintVersion_TYPE int (J3DAPI*)(const SithConsoleCommand*, const char*)
#define JonesConsole_PrintFramerate_TYPE int (J3DAPI*)(const SithConsoleCommand*, const char*)
#define JonesConsole_PrintPolys_TYPE int (J3DAPI*)(const SithConsoleCommand*, const char*)
#define JonesConsole_ShowEndCredits_TYPE int (J3DAPI*)(const SithConsoleCommand*, const char*)
#define JonesConsole_RemoveExpired_TYPE int (J3DAPI*)()
#define JonesHud_Startup_TYPE int (*)(void)
#define JonesHud_Shutdown_TYPE void (*)(void)
#define JonesHud_Open_TYPE int (*)(void)
#define JonesHud_Close_TYPE void (*)(void)
#define JonesHud_ToggleMenu_TYPE void (*)(void)
#define JonesHud_EnableMenu_TYPE void (J3DAPI*)(int)
#define JonesHud_IsMenuEnabled_TYPE int (*)(void)
#define JonesHud_Render_TYPE void (*)(void)
#define JonesHud_Update_TYPE void (J3DAPI*)(const SithWorld*)
#define JonesHud_SetCanvasSize_TYPE int (J3DAPI*)(int, int)
#define JonesHud_UpdateSinCosTable_TYPE void (*)(void)
#define JonesHud_UpdateHUDLayout_TYPE void (J3DAPI*)(uint32_t, uint32_t)
#define JonesHud_MenuOpen_TYPE void (*)(void)
#define JonesHud_MenuClose_TYPE void (*)(void)
#define JonesHud_StartItemTranslation_TYPE void (J3DAPI*)(JonesHudMenuItem*, uint32_t, float, int)
#define JonesHud_HasTimeElapsed_TYPE int (J3DAPI*)(unsigned int, unsigned int, unsigned int)
#define JonesHud_RenderEnduranceIndicator_TYPE void (J3DAPI*)(float)
#define JonesHud_DrawEnduranceIndicator_TYPE void (J3DAPI*)(float, float)
#define JonesHud_SetFadeHealthHUD_TYPE void (J3DAPI*)(int, int)
#define JonesHud_RenderFadeHealthIndicator_TYPE int (J3DAPI*)(int)
#define JonesHud_DrawHealthIndicator_TYPE void (J3DAPI*)(float, float, float)
#define JonesHud_DrawHealthHitOverlay_TYPE void (J3DAPI*)(float, float, float)
#define JonesHud_RenderHealthIndicator_TYPE void (J3DAPI*)(float)
#define JonesHud_DrawHealthIndicatorBase_TYPE void (J3DAPI*)(float, float, float, float)
#define JonesHud_DrawHealthIndicatorBar_TYPE void (J3DAPI*)(float, float, float, float)
#define JonesHud_DrawIndicatorBar_TYPE void (J3DAPI*)(const rdVector4*, float, const rdVector4*, float, int)
#define JonesHud_Draw_TYPE void (J3DAPI*)(const rdMaterial*, const JonesHudRect*, float, float, const rdVector4*, int, int)
#define JonesHud_ProcessMenuItems_TYPE void (*)(void)
#define JonesHud_MenuMoveLeft_TYPE void (*)(void)
#define JonesHud_MenuMoveRight_TYPE void (*)(void)
#define JonesHud_MenuMoveDown_TYPE void (*)(void)
#define JonesHud_SetSelectedMenuItem_TYPE void (J3DAPI*)(int, JonesHudMenuItem*)
#define JonesHud_MenuMoveUp_TYPE void (*)(void)
#define JonesHud_BindActivateControlKeys_TYPE void (J3DAPI*)(int*, int)
#define JonesHud_GetKey_TYPE int (J3DAPI*)(unsigned int)
#define JonesHud_InitializeMenu_TYPE int (*)(void)
#define JonesHud_InitializeMenuSounds_TYPE void (*)(void)
#define JonesHud_NewMenuItem_TYPE JonesHudMenuItem* (J3DAPI*)(rdModel3*)
#define JonesHud_FreeMenuItem_TYPE void (J3DAPI*)(JonesHudMenuItem*)
#define JonesHud_UpdateItem_TYPE void (J3DAPI*)(JonesHudMenuItem*)
#define JonesHud_RenderMenuItems_TYPE void (J3DAPI*)(JonesHudMenuItem*)
#define JonesHud_RenderMenuItem_TYPE void (J3DAPI*)(JonesHudMenuItem*)
#define JonesHud_MenuActivateItem_TYPE void (*)(void)
#define JonesHud_ResetMenuItems_TYPE void (*)(void)
#define JonesHud_SetLeftRightItems_TYPE void (J3DAPI*)(JonesHudMenuItem*, JonesHudMenuItem*)
#define JonesHud_SetDownUpItems_TYPE void (J3DAPI*)(JonesHudMenuItem*, JonesHudMenuItem*)
#define JonesHud_sub_4198E0_TYPE void (J3DAPI*)(JonesHudMenuItem*)
#define JonesHud_sub_419B50_TYPE void (J3DAPI*)(JonesHudMenuItem*)
#define JonesHud_InventoryItemChanged_TYPE void (J3DAPI*)(int)
#define JonesHud_RenderInventoryItemChange_TYPE void (*)(void)
#define JonesHud_RenderChangedItem_TYPE void (J3DAPI*)(const JonesHudMenuItem*, float)
#define JonesHud_ShowLevelCompleted_TYPE int (*)(void)
#define JonesHud_OpenHelp_TYPE HANDLE (J3DAPI*)(HANDLE)
#define JonesHud_ShowGameOverDialog_TYPE void (J3DAPI*)(int)
#define JonesHud_GetHealthBarAlpha_TYPE float (*)(void)
#define JonesHud_SetHealthBarAlpha_TYPE void (J3DAPI*)(float)
#define JonesHud_CutsceneStart_TYPE void (J3DAPI*)(int)
#define JonesHud_EnableInterface_TYPE int (J3DAPI*)(int)
#define JonesHud_DrawCredits_TYPE int (J3DAPI*)(int, tSoundChannelHandle)
#define JonesHud_InitializeGameStatistics_TYPE void (*)(void)
#define JonesHud_RestoreGameStatistics_TYPE void (*)(void)
#define JonesHud_RestoreTreasuresStatistics_TYPE void (*)(void)
#define JonesMain_Startup_TYPE int (J3DAPI*)(const char*)
#define JonesMain_GameWndProc_TYPE int (J3DAPI*)(HWND, UINT, WPARAM, LPARAM, int*)
#define JonesMain_HandleWMGetMinMaxInfo_TYPE void (J3DAPI*)(HWND, LPMINMAXINFO)
#define JonesMain_HandleWMPaint_TYPE void (J3DAPI*)(HWND)
#define JonesMain_HandleWMTimer_TYPE void (J3DAPI*)(HWND, WPARAM)
#define JonesMain_HandleWMKeydown_TYPE void (J3DAPI*)(HWND, WPARAM, int, uint16_t, uint16_t)
#define JonesMain_PrintQuickSave_TYPE void (*)(void)
#define JonesMain_HandleWMActivateApp_TYPE int (J3DAPI*)(HWND, WPARAM, LPARAM)
#define JonesMain_HandleWMActivate_TYPE LRESULT (J3DAPI*)(HWND, uint16_t, LPARAM, uint16_t)
#define JonesMain_OnAppActivate_TYPE void (J3DAPI*)(HWND, int)
#define JonesMain_HandleWMChar_TYPE void (J3DAPI*)(HWND, char, uint16_t)
#define JonesMain_HandleWMSysCommand_TYPE void (J3DAPI*)(HWND, WPARAM, uint16_t, uint16_t)
#define JonesMain_HandleWMPowerBroadcast_TYPE void (J3DAPI*)(HWND, WPARAM)
#define JonesMain_Shutdown_TYPE void (*)(void)
#define JonesMain_PauseGame_TYPE void (J3DAPI*)()
#define JonesMain_ResumeGame_TYPE void (J3DAPI*)()
#define JonesMain_IsGamePaused_TYPE int (J3DAPI*)()
#define JonesMain_Process_TYPE int (*)(void)
#define JonesMain_ProcessWindowEvents_TYPE int (*)(void)
#define JonesMain_ProcessGame_TYPE int (*)(void)
#define JonesMain_ProcessMenu_TYPE void (*)(void)
#define JonesMain_PrintFramerate_TYPE void (J3DAPI*)()
#define JonesMain_TogglePrintFramerate_TYPE void (*)(void)
#define JonesMain_Open_TYPE int (*)(void)
#define JonesMain_EnsureFile_TYPE int (J3DAPI*)(const char*)
#define JonesMain_Close_TYPE int (*)(void)
#define JonesMain_Restore_TYPE int (J3DAPI*)(const char*)
#define JonesMain_ProcessGamesaveState_TYPE int (*)(void)
#define JonesMain_UpdateLevelNum_TYPE void (*)(void)
#define JonesMain_ProcessStartGame_TYPE int (*)(void)
#define JonesMain_SetBonusLevel_TYPE void (*)(void)
#define JonesMain_ShowEndCredits_TYPE void (*)(void)
#define JonesMain_ProcessCredits_TYPE int (*)(void)
#define JonesMain_HasStarted_TYPE int (*)(void)
#define JonesMain_GetDisplayEnvironment_TYPE StdDisplayEnvironment* (*)(void)
#define JonesMain_GetDisplaySettings_TYPE JonesDisplaySettings* (*)(void)
#define JonesMain_CloseWindow_TYPE int (*)(void)
#define JonesMain_FilePrintf_TYPE int (J3DAPI*)(const char*, ...)
#define JonesMain_Log_TYPE int (J3DAPI*)(const char*, ...)
#define JonesMain_RefreshDisplayDevice_TYPE void (*)(void)
#define JonesMain_PlayIntroMovie_TYPE int (*)(void)
#define JonesMain_IntroWndProc_TYPE int (J3DAPI*)(HWND, UINT, WPARAM, LPARAM, int*)
#define JonesMain_IntroHandleWMLButtonUp_TYPE void (J3DAPI*)(HWND, uint16_t, uint16_t, WPARAM)
#define JonesMain_IntroHandleWMKeydown_TYPE void (J3DAPI*)(HWND, WPARAM, int, uint16_t, uint16_t)
#define JonesMain_IntroMovieBlt565_TYPE int (J3DAPI*)(const SmushBitmap*, int)
#define JonesMain_IntroMovieBlt555_TYPE int (J3DAPI*)(const SmushBitmap*, int)
#define JonesMain_IntroMovieBlt32_TYPE int (J3DAPI*)(const SmushBitmap*, int)
#define JonesMain_Assert_TYPE void (J3DAPI*)(const char*, const char*, int)
#define JonesMain_BindToggleMenuControlKeys_TYPE void (J3DAPI*)(const int*, int)
#define JonesMain_InitializeHUD_TYPE void (*)(void)
#define JonesMain_SaveHUD_TYPE int (J3DAPI*)(DPID, unsigned int)
#define JonesMain_RestoreHUD_TYPE int (J3DAPI*)(const SithMessage*)
#define JonesMain_GetCurrentLevelNum_TYPE size_t (*)(void)
#define JonesMain_LogErrorToFile_TYPE void (J3DAPI*)(const char*)
#define JonesMain_LoadSettings_TYPE void (J3DAPI*)(StdDisplayEnvironment*, JonesState*)
#define JonesMain_ShowDevDialog_TYPE int (J3DAPI*)(HWND, JonesState*)
#define JonesMain_DevDialogProc_TYPE INT_PTR (__stdcall*)(HWND, UINT, WPARAM, LPARAM)
#define JonesMain_InitDevDialog_TYPE int (J3DAPI*)(HWND, WPARAM, JonesState*)
#define JonesMain_DevDialogHandleCommand_TYPE void (J3DAPI*)(HWND, int, LPARAM, int)
#define JonesMain_DevDialogInitDisplayDevices_TYPE void (J3DAPI*)(HWND, JonesState*)
#define JonesMain_DevDialogUpdateRadioButtons_TYPE void (J3DAPI*)(HWND, const JonesState*)
#define JonesMain_FindClosestVideoMode_TYPE size_t (J3DAPI*)(const StdDisplayEnvironment*, StdVideoMode*, size_t)
#define JonesMain_CurDisplaySupportsBPP_TYPE bool (J3DAPI*)(const JonesDisplaySettings*, size_t)
#define jonesCog_Startup_TYPE int (*)(void)
#define jonesCog_Shutdown_TYPE void (*)(void)
#define jonesCog_RegisterFunctions_TYPE int (J3DAPI*)()
#define jonesCog_HideHealthDisplay_TYPE void (J3DAPI*)(SithCog*)
#define jonesCog_ShowHealthDisplay_TYPE void (J3DAPI*)(SithCog*)
#define jonesCog_InventoryItemChanged_TYPE void (J3DAPI*)(SithCog*)
#define jonesCog_EndLevelFunc_TYPE void (J3DAPI*)(SithCog*)
#define jonesCog_EndLevel_TYPE void (*)(void)
#define jonesCog_ExitToShell_TYPE void (J3DAPI*)(SithCog*)
#define jonesCog_StartCutsceneFunc_TYPE void (J3DAPI*)(SithCog*)
#define jonesCog_EndCutscene_TYPE void (J3DAPI*)(SithCog*)
#define jonesCog_UpdateDifficulty_TYPE void (J3DAPI*)(SithCog*)
#define jonesCog_EnableInterfaceFunc_TYPE void (J3DAPI*)(SithCog*)
#define jonesCog_StartCutscene_TYPE void (J3DAPI*)(SithCog*)
#define jonesCog_EnableInterface_TYPE void (J3DAPI*)(SithCog*)
#define jonesConfig_Startup_TYPE int (*)(void)
#define jonesConfig_InitKeySetsPath_TYPE int (J3DAPI*)()
#define jonesConfig_InitKeyActions_TYPE int (J3DAPI*)()
#define jonesConfig_Shutdown_TYPE void (*)(void)
#define jonesConfig_ControlToString_TYPE void (J3DAPI*)(unsigned int, char*)
#define jonesConfig_ShowMessageDialog_TYPE int (J3DAPI*)(HWND, const char*, const char*, int)
#define jonesConfig_MessageDialogProc_TYPE BOOL (__stdcall*)(HWND, UINT, WPARAM, LPARAM)
#define jonesConfig_sub_4025F0_TYPE void (J3DAPI*)(HWND, uint16_t*)
#define jonesConfig_sub_402670_TYPE int (J3DAPI*)(HWND, int, uint16_t*)
#define jonesConfig_SetTextControl_TYPE int (J3DAPI*)(HWND, HWND, const char*)
#define jonesConfig_sub_402A90_TYPE BOOL (J3DAPI*)(HWND, int)
#define jonesConfig_NewControlScheme_TYPE JonesControlsScheme* (J3DAPI*)()
#define jonesConfig_LoadConstrolsScheme_TYPE int (J3DAPI*)(const char*, JonesControlsScheme*)
#define jonesConfig_GetEntryIndex_TYPE int (J3DAPI*)(int*, StdConffileEntry*, int, const char**, int)
#define jonesConfig_GetValueIndex_TYPE int (J3DAPI*)(char*, const char**, int)
#define jonesConfig_GetControllerKeySchemeIndex_TYPE int (J3DAPI*)(int*, StdConffileEntry*)
#define jonesConfig_SetDefaultControlScheme_TYPE int (J3DAPI*)(JonesControlsScheme*, int)
#define jonesConfig_BindControls_TYPE void (J3DAPI*)(JonesControlsScheme*)
#define jonesConfig_BindJoystickControl_TYPE void (J3DAPI*)(SithControlFunction, int)
#define jonesConfig_InitDialog_TYPE HFONT (J3DAPI*)(HWND, HFONT, int)
#define jonesConfig_CreateDialogFont_TYPE HFONT (J3DAPI*)(HWND, int, int, float*)
#define jonesConfig_ResetDialogFont_TYPE void (J3DAPI*)(HWND, HFONT)
#define jonesConfig_ResetWindowFontCallback_TYPE int (__stdcall*)(HWND, LPARAM)
#define jonesConfig_SetPositionAndTextCallback_TYPE BOOL (__stdcall*)(HWND, LPARAM)
#define jonesConfig_SetWindowFontAndPosition_TYPE void (J3DAPI*)(HWND, JonesDialogFontInfo*)
#define jonesConfig_GetWindowScreenRect_TYPE void (J3DAPI*)(HWND, LPRECT)
#define jonesConfig_SetDialogTitleAndPosition_TYPE void (J3DAPI*)(HWND, JonesDialogFontInfo*)
#define jonesConfig_GetSaveGameFilePath_TYPE int (J3DAPI*)(HWND, char*)
#define jonesConfig_SaveGameDialogHookProc_TYPE UINT_PTR (__stdcall*)(HWND, UINT, WPARAM, LPARAM)
#define jonesConfig_SaveGameDialogInit_TYPE int (J3DAPI*)(HWND, int, LPOPENFILENAMEA)
#define jonesConfig_SaveGameThumbnailPaintProc_TYPE LRESULT (__stdcall*)(HWND, UINT, WPARAM, LPARAM)
#define jonesConfig_ShowOverwriteSaveGameDlg_TYPE int (J3DAPI*)(HWND, const char*)
#define jonesConfig_ShowExitGameDialog_TYPE int (J3DAPI*)(HWND, char*)
#define jonesConfig_ExitGameDlgProc_TYPE INT_PTR (__stdcall*)(HWND, UINT, WPARAM, LPARAM)
#define jonesConfig_SaveGameMsgBoxProc_TYPE INT_PTR (__stdcall*)(HWND, UINT, WPARAM, LPARAM)
#define jonesConfig_GameSaveSetData_TYPE int (J3DAPI*)(HWND, int, GameSaveMsgBoxData*)
#define jonesConfig_MsgBoxDlg_HandleWM_COMMAND_TYPE void (J3DAPI*)(HWND, int)
#define jonesConfig_GetLoadGameFilePath_TYPE int (J3DAPI*)(HWND, char*)
#define jonesConfig_sub_405F60_TYPE void* (J3DAPI*)(HWND)
#define jonesConfig_LoadGameDialogHookProc_TYPE UINT_PTR (__stdcall*)(HWND, UINT, WPARAM, LPARAM)
#define jonesConfig_ShowLoadGameWarningMsgBox_TYPE int (J3DAPI*)(HWND)
#define jonesConfig_LoadGameDialogInit_TYPE int (J3DAPI*)(HWND, int, LPOPENFILENAMEA)
#define jonesConfig_LoadGameThumbnailPaintProc_TYPE LRESULT (__stdcall*)(HWND, UINT, WPARAM, LPARAM)
#define jonesConfig_ShowGamePlayOptions_TYPE int (J3DAPI*)(HWND)
#define jonesConfig_GamePlayOptionsProc_TYPE INT_PTR (__stdcall*)(HWND, UINT, WPARAM, LPARAM)
#define jonesConfig_HandleWM_HSCROLL_TYPE void (J3DAPI*)(HWND, HWND, uint16_t)
#define jonesConfig_GamePlayOptionsInitDlg_TYPE int (J3DAPI*)(HWND)
#define jonesConfig_GamePlayOptions_HandleWM_COMMAND_TYPE void (J3DAPI*)(HWND, uint16_t)
#define jonesConfig_EnableMouseControl_TYPE void (J3DAPI*)(int)
#define jonesConfig_FreeControlScheme_TYPE void (J3DAPI*)(JonesControlsScheme*)
#define jonesConfig_FreeControlConfigEntry_TYPE void (J3DAPI*)(JonesControlsConfig*)
#define jonesConfig_ShowControlOptions_TYPE int (J3DAPI*)(HWND)
#define jonesConfig_CopyControlConfig_TYPE int (J3DAPI*)(JonesControlsConfig*, JonesControlsConfig*)
#define jonesConfig_CopyControlSchemes_TYPE JonesControlsScheme* (J3DAPI*)(JonesControlsScheme*, int, int)
#define jonesConfig_InitControlsConfig_TYPE int (J3DAPI*)()
#define jonesConfig_LoadControlsSchemesFromSystem_TYPE int (J3DAPI*)(JonesControlsConfig*)
#define jonesConfig_SetDefaultControlSchemes_TYPE int (J3DAPI*)(JonesControlsConfig*)
#define jonesConfig_ControlOptionsProc_TYPE INT_PTR (__stdcall*)(HWND, UINT, WPARAM, LPARAM)
#define jonesConfig_InitControlOptionsDlg_TYPE int (J3DAPI*)(HWND, int, JonesControlsConfig*)
#define jonesConfig_ControlOptions_HandleWM_COMMAND_TYPE void (J3DAPI*)(HWND, int, int, int16_t)
#define jonesConfig_WriteScheme_TYPE int (J3DAPI*)(JonesControlsScheme*)
#define jonesConfig_sub_407F60_TYPE int (J3DAPI*)(HWND)
#define jonesConfig_sub_408000_TYPE LRESULT (J3DAPI*)(HWND, int)
#define jonesConfig_sub_408260_TYPE void (J3DAPI*)(HWND, JonesControlsConfig*)
#define jonesConfig_sub_408400_TYPE JonesControlsScheme* (J3DAPI*)(JonesControlsConfig*)
#define jonesConfig_CreateNewSchemeAction_TYPE void (J3DAPI*)(HWND, JonesControlsConfig*)
#define jonesConfig_ShowCreateControlSchemeDialog_TYPE int (J3DAPI*)(HWND, char*)
#define jonesConfig_CreateControlSchemeProc_TYPE INT_PTR (__stdcall*)(HWND, UINT, WPARAM, LPARAM)
#define jonesConfig_InitCreateControlScheme_TYPE int (J3DAPI*)(HWND, int, char*)
#define jonesConfig_CreateControlScheme_Handle_WM_COMMAND_TYPE void (J3DAPI*)(HWND, int16_t)
#define jonesConfig_ShowEditControlShemeDialog_TYPE int (J3DAPI*)(HWND, void*)
#define jonesConfig_EditControlSchemProc_TYPE INT_PTR (__stdcall*)(HWND, UINT, WPARAM, LPARAM)
#define jonesConfig_InitEditControlSchemeDlg_TYPE int (J3DAPI*)(HWND, int, void*)
#define jonesConfig_sub_408ED0_TYPE LRESULT (J3DAPI*)(HWND, char*)
#define jonesConfig_sub_408FC0_TYPE const char** (J3DAPI*)(HWND, int, JonesControlsScheme*)
#define jonesConfig_sub_409200_TYPE char (J3DAPI*)(HWND, int)
#define jonesConfig_sub_409530_TYPE LPARAM (J3DAPI*)(HWND, HWND, int, int)
#define jonesConfig_sub_4096F0_TYPE int (J3DAPI*)(int, HWND, int, HWND*, uint32_t*)
#define jonesConfig_sub_4097D0_TYPE char (J3DAPI*)(HWND, HWND, int, int)
#define jonesConfig_sub_409BC0_TYPE LPARAM (J3DAPI*)(int, HWND, int, int)
#define jonesConfig_AssignKeyAction_TYPE int (J3DAPI*)(HWND, uint32_t*)
#define jonesConfig_sub_409F70_TYPE int (J3DAPI*)(int, int, int*, int, int*)
#define jonesConfig_AssignKeyDlgProc_TYPE INT_PTR (__stdcall*)(HWND, UINT, WPARAM, LPARAM)
#define jonesConfig_sub_40A1A0_TYPE int (J3DAPI*)(HWND, int, uint32_t*)
#define jonesConfig_AssignControlKey_TYPE void (J3DAPI*)(HWND)
#define jonesConfig_sub_40A500_TYPE int (J3DAPI*)(int)
#define jonesConfig_AssignKeyDlg_HandleWM_COMMAND_TYPE void (J3DAPI*)(HWND, int)
#define jonesConfig_ShowReassignKeyMsgBox_TYPE int (J3DAPI*)(HWND, LPARAM)
#define jonesConfig_ReassignKeyDialogProc_TYPE INT_PTR (__stdcall*)(HWND, UINT, WPARAM, LPARAM)
#define jonesConfig_SetReassignKeyDialogText_TYPE int (J3DAPI*)(HWND, int, void*)
#define jonesConfig_sub_40AA10_TYPE int (J3DAPI*)(HWND, int)
#define jonesConfig_ShowDisplaySettingsDialog_TYPE int (J3DAPI*)(HWND, StdDisplayEnvironment*, JonesDisplaySettings*)
#define jonesConfig_DisplaySettingsDialogProc_TYPE INT_PTR (__stdcall*)(HWND, UINT, WPARAM, LPARAM)
#define jonesConfig_sub_40AE90_TYPE int (J3DAPI*)(HWND, int, int*)
#define jonesConfig_sub_40B530_TYPE int (J3DAPI*)(const char*, const char*, int)
#define jonesConfig_sub_40B5A0_TYPE int (J3DAPI*)(HWND, int, int, int)
#define jonesConfig_ShowAdvanceDisplaySettings_TYPE uint32_t (J3DAPI*)(HWND, LPARAM)
#define jonesConfig_AdvanceDisplaySettingsDialog_TYPE INT_PTR (__stdcall*)(HWND, UINT, WPARAM, LPARAM)
#define jonesConfig_sub_40BC40_TYPE int (J3DAPI*)(int, int, int)
#define jonesConfig_sub_40BCD0_TYPE int (J3DAPI*)(HWND, int, uint32_t*)
#define jonesConfig_sub_40C090_TYPE int (J3DAPI*)(HWND, int, int, int)
#define jonesConfig_ShowSoundSettingsDialog_TYPE int (J3DAPI*)(HWND, float*)
#define jonesConfig_SoundSettingsDialogProc_TYPE INT_PTR (__stdcall*)(HWND, UINT, WPARAM, LPARAM)
#define jonesConfig_sub_40C650_TYPE int (J3DAPI*)(HWND, int, LONG)
#define jonesConfig_SoundSettings_HandleWM_COMMAND_TYPE int (J3DAPI*)(HWND, int)
#define jonesConfig_ShowGameOverDialog_TYPE int (J3DAPI*)(HWND, char*, tSoundHandle, tSoundChannelHandle*)
#define jonesConfig_GameOverDialogProc_TYPE INT_PTR (__stdcall*)(HWND, UINT, WPARAM, LPARAM)
#define jonesConfig_LoadGameGetLastSavedGamePath_TYPE void (J3DAPI*)(char*, unsigned int)
#define jonesConfig_GameOverDialogInit_TYPE int (J3DAPI*)(HWND, int, GameOverDialogData*)
#define jonesConfig_GameOverDialog_HandleWM_COMMAND_TYPE void (J3DAPI*)(HWND, uint16_t)
#define jonesConfig_ShowStatisticsDialog_TYPE int (J3DAPI*)(HWND, SithGameStatistics*)
#define jonesConfig_StatisticsDialogProc_TYPE INT_PTR (__stdcall*)(HWND, UINT, WPARAM, LPARAM)
#define jonesConfig_sub_40D100_TYPE int (J3DAPI*)(int, HWND, int, int)
#define jonesConfig_UpdateCurrentLevelNum_TYPE void (J3DAPI*)()
#define jonesConfig_DrawImageOnDialogItem_TYPE int (J3DAPI*)(HWND, HDC, HDC, int, HBITMAP, HBITMAP)
#define jonesConfig_SetStatisticsDialogForLevel_TYPE int (J3DAPI*)(HWND, int, int*)
#define jonesConfig_DrawStatisticDialogIQPoints_TYPE void (J3DAPI*)(HWND, JonesDialogImageInfo**, int, int)
#define jonesConfig_InitStatisticDialog_TYPE int (J3DAPI*)(HWND, int, int*)
#define jonesConfig_StatisticProc_HandleWM_COMMAND_TYPE void (J3DAPI*)(HWND, int16_t)
#define jonesConfig_ShowLevelCompletedDialog_TYPE int (J3DAPI*)(HWND, int*, int*, int, int, int, int, int, int)
#define jonesConfig_LevelCompletedDialogProc_TYPE int (__stdcall*)(HWND, UINT, WPARAM, LPARAM)
#define jonesConfig_InitLevelCompletedDialog_TYPE int (J3DAPI*)(HWND, int, tLevelCompletedDialogState*)
#define jonesConfig_ChapterCompleteDialog_HandleWM_COMMAND_TYPE void (J3DAPI*)(HWND, int)
#define jonesConfig_ShowStoreDialog_TYPE int (J3DAPI*)(HWND, int*, int*, int)
#define jonesConfig_StoreDialogProc_TYPE int (__stdcall*)(HWND, UINT, WPARAM, LPARAM)
#define jonesConfig_StoreHandleDragEvent_TYPE int (J3DAPI*)(HWND, HWND)
#define jonesConfig_StoreDialog_HandleWM_MBUTTONUP_TYPE void (J3DAPI*)(HWND)
#define jonesConfig_StoreDialog_HandleWM_MOUSEFIRST_TYPE BOOL (J3DAPI*)(HWND)
#define jonesConfig_InitStoreDialog_TYPE int (J3DAPI*)(HWND, int, tStoreCartState*)
#define jonesConfig_StoreInitItemIcons_TYPE int (J3DAPI*)(HWND, tStoreCartState*)
#define jonesConfig_StoreSetListColumns_TYPE LRESULT (J3DAPI*)(HWND, const char*)
#define jonesConfig_StoreInitItemList_TYPE void (J3DAPI*)(HWND, int*, int)
#define jonesConfig_UpdateBalances_TYPE void (J3DAPI*)(HWND, tStoreCartState*)
#define jonesConfig_AddStoreCartItem_TYPE void (J3DAPI*)(HWND, tStoreCartState*)
#define jonesConfig_RemoveStoreCartItem_TYPE void (J3DAPI*)(HWND, tStoreCartState*)
#define jonesConfig_StoreDialog_HandleWM_COMMAND_TYPE void (J3DAPI*)(HWND, WPARAM)
#define jonesConfig_ClearStoreCart_TYPE void (J3DAPI*)(HWND, tStoreCartState*)
#define jonesConfig_ShowPurchaseMessageBox_TYPE int (J3DAPI*)(HWND, tStoreCartState*)
#define jonesConfig_PurchaseMessageBoxProc_TYPE INT_PTR (__stdcall*)(HWND, UINT, WPARAM, LPARAM)
#define jonesConfig_InitPurchaseMessageBox_TYPE int (J3DAPI*)(HWND, int, tStoreCartState*)
#define jonesConfig_ShowDialogInsertCD_TYPE int (J3DAPI*)(HWND, LPARAM)
#define jonesConfig_DialogInsertCDProc_TYPE INT_PTR (__stdcall*)(HWND, UINT, WPARAM, LPARAM)
#define jonesConfig_InitDialogInsertCD_TYPE int (J3DAPI*)(HWND, int, int)
#define jonesConfig_InsertCD_HandleWM_COMMAND_TYPE int (J3DAPI*)(HWND, int)
#define JonesDialog_InitCursorTracking_TYPE int (J3DAPI*)(int*)
#define JonesDialog_TrackCursor_TYPE int (J3DAPI*)(int)
#define JonesDialog_ResetCursorTracking_TYPE int (J3DAPI*)()
#define JonesDialog_CopyRectToDrawBuffer_TYPE int (J3DAPI*)(GDIDIBSectionInfo*, const PPOINT, LPRECT)
#define JonesDialog_RestoreBackground_TYPE int (J3DAPI*)(HDC, HWND, LPPOINT, LPRECT)
#define JonesDialog_ShowDialog_TYPE int (J3DAPI*)(LPCSTR, HWND, DLGPROC, LPARAM)
#define JonesDialog_SetGameState_TYPE void (J3DAPI*)(JonesDialogGameState*, int)
#define JonesDialog_ResetGameState_TYPE void (J3DAPI*)(JonesDialogGameState*, int)
#define JonesDialog_DummyFlipGDI_TYPE int (*)(void)
#define JonesDialog_SubclassDialogProc_TYPE INT_PTR (__stdcall*)(HWND, UINT, WPARAM, LPARAM)
#define JonesDialog_SubclassDialogWindowProc_TYPE LRESULT (__stdcall*)(HWND, UINT, WPARAM, LPARAM)
#define JonesDialog_HandleWM_ERASEBKGND_TYPE void (J3DAPI*)(HWND, WPARAM, int, JonesDialogData*)
#define JonesDialog_HandleWM_PAINT_TYPE void (J3DAPI*)(HWND, JonesDialogData*)
#define JonesDialog_HandleWM_NCPAINT_TYPE void (J3DAPI*)(HWND, WPARAM, JonesDialogData*)
#define JonesDialog_AllocScreenDBISection_TYPE int (J3DAPI*)(int*)
#define JonesDialog_AllocOffScreenGDISection_TYPE int (J3DAPI*)(GDIDIBSectionInfo*, HDC, int, int*)
#define JonesDialog_FreeScreenDIBSection_TYPE void (J3DAPI*)()
#define JonesDialog_AllocOffScreenDIBSection_TYPE int (J3DAPI*)(int*)
#define JonesDialog_FreeOffScreenDIBSection_TYPE int (J3DAPI*)()
#define JonesDialog_ShowFileSelectDialog_TYPE BOOL (J3DAPI*)(LPOPENFILENAMEA, int)
#define JonesDialog_SubclassFileDialogProc_TYPE UINT_PTR (__stdcall*)(HWND, UINT, WPARAM, LPARAM)
#define JonesDisplay_Startup_TYPE int (J3DAPI*)(JonesDisplaySettings*)
#define JonesDisplay_Shutdown_TYPE void (*)(void)
#define JonesDisplay_Restart_TYPE int (J3DAPI*)(JonesDisplaySettings*)
#define JonesDisplay_SetDefaultVideoMode_TYPE void (J3DAPI*)(const StdDisplayEnvironment*, JonesDisplaySettings*)
#define JonesDisplay_Open_TYPE int (J3DAPI*)(JonesDisplaySettings*)
#define JonesDisplay_Close_TYPE void (*)(void)
#define JonesDisplay_EnableDualMonitor_TYPE void (J3DAPI*)(int)
#define JonesDisplay_UpdateDualScreenWindowSize_TYPE int (J3DAPI*)(const JonesDisplaySettings*)
#define JonesDisplay_OpenLoadScreen_TYPE void (J3DAPI*)(const char*, float, float, float, float, int)
#define JonesDisplay_CloseLoadScreen_TYPE void (*)(void)
#define JonesDisplay_UpdateLoadProgress_TYPE void (J3DAPI*)(float)
#define JonesControl_Startup_TYPE int (*)(void)
#define JonesControl_Shutdown_TYPE void (*)(void)
#define JonesControl_ProcessControls_TYPE int (J3DAPI*)(SithThing*, float)
#define JonesControl_EnableJoystickAxes_TYPE void (*)(void)
#define JonesFile_Startup_TYPE void (J3DAPI*)(tHostServices*)
#define JonesFile_Shutdown_TYPE void (*)(void)
#define JonesFile_Open_TYPE int (J3DAPI*)(tHostServices*, const char*, const char*)
#define JonesFile_Close_TYPE void (*)(void)
#define JonesFile_GetCurrentCDNum_TYPE int (*)(void)
#define JonesFile_FileExists_TYPE int (J3DAPI*)(const char*)
#define JonesFile_SetInstallPath_TYPE void (J3DAPI*)(const char*)
#define JonesFile_SetCDPath_TYPE void (J3DAPI*)(const char*)
#define JonesFile_InitServices_TYPE void (*)(void)
#define JonesFile_ResetServices_TYPE void (*)(void)
#define JonesFile_OpenResource_TYPE void (J3DAPI*)(JonesResource*, const char*)
#define JonesFile_CloseResource_TYPE void (J3DAPI*)(JonesResource*)
#define JonesFile_OpenFile_TYPE tFileHandle (J3DAPI*)(const char*, const char*)
#define JonesFile_CloseFile_TYPE int (J3DAPI*)(tFileHandle)
#define JonesFile_FileRead_TYPE size_t (J3DAPI*)(tFileHandle, void*, size_t)
#define JonesFile_FileWrite_TYPE size_t (J3DAPI*)(tFileHandle, const void*, size_t)
#define JonesFile_FileGets_TYPE char* (J3DAPI*)(tFileHandle, char*, size_t)
#define JonesFile_FileEOF_TYPE int (J3DAPI*)(tFileHandle)
#define JonesFile_FileTell_TYPE int (J3DAPI*)(tFileHandle)
#define JonesFile_FileSeek_TYPE int (J3DAPI*)(tFileHandle, int, int)
#define JonesFile_FileSize_TYPE size_t (J3DAPI*)(const char*)
#define JonesFile_FilePrintf_TYPE int (*)(tFileHandle, const char*, ...)
#define jonesInventory_Open_TYPE int (*)(void)
#define jonesInventory_Load_TYPE int (J3DAPI*)(const char*)
#define jonesInventory_Close_TYPE void (J3DAPI*)()
#define jonesInventory_ActivateItem_TYPE int (J3DAPI*)(SithThing*, int)
#define jonesInventory_UseItem_TYPE int (J3DAPI*)(SithThing*, unsigned int)
#define jonesInventory_ResetGameStatistics_TYPE int (*)(void)
#define jonesInventory_GetMaxDifficultyLevel_TYPE size_t (*)(void)
#define jonesInventory_UpdateSolvedHintsStatistics_TYPE void (*)(void)
#define jonesInventory_UpdateIQPoints_TYPE void (J3DAPI*)(size_t)
#define jonesInventory_GetTotalIQPoints_TYPE int (*)(void)
#define jonesInventory_AdvanceStatistics_TYPE void (J3DAPI*)()
#define jonesInventory_AdvanceFoundTreasuresStatistics_TYPE void (*)()
#define jonesInventory_UpdateGameTimeStatistics_TYPE void (J3DAPI*)(unsigned int)
#define jonesInventory_UpdateLevelStatisticTime_TYPE void (J3DAPI*)(int, SithLevelStatistic*, unsigned int, int)
#define jonesInventory_ResetStatisticsGameTime_TYPE void (*)()
#define jonesString_GetString_TYPE const char* (J3DAPI*)(const char*)
#define jonesString_Startup_TYPE int (J3DAPI*)()
#define jonesString_Shutdown_TYPE void (*)(void)
#define Indy3D_WinMain_TYPE int (__stdcall*)(HINSTANCE, HINSTANCE, LPSTR, int)

#endif // JONES3D_RTI_SYMBOLS_H

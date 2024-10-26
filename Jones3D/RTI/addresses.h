#ifndef JONES3D_RTI_ADDRESSES_H
#define JONES3D_RTI_ADDRESSES_H
#include <Jones3D/types.h>
#include <j3dcore/j3d.h>
#include <rdroid/types.h>
#include <sith/types.h>
#include <std/types.h>

// Exe secion addresses

#define EXE_TEXT_START_ADDR 0x00401000
#define EXE_TEXT_END_ADDR   0x00505000

#define EXE_RDATA_START_ADDR 0x00506324
#define EXE_RDATA_END_ADDR   0x0050D000

// Function addresses

#define JonesConsole_Startup_ADDR 0x0040F290
#define JonesConsole_Shutdown_ADDR 0x0040F310
#define JonesConsole_Open_ADDR 0x0040F340
#define JonesConsole_Close_ADDR 0x0040F430
#define JonesConsole_FlushToDisplay_ADDR 0x0040F4A0
#define JonesConsole_PrintText_ADDR 0x0040F6A0
#define JonesConsole_PrintTextWithID_ADDR 0x0040F7B0
#define JonesConsole_ClearText_ADDR 0x0040F8E0
#define JonesConsole_ExeCommand_ADDR 0x0040F960
#define JonesConsole_ShowConsole_ADDR 0x0040FA60
#define JonesConsole_HandelChar_ADDR 0x0040FAA0
#define JonesConsole_FlushCommandLine_ADDR 0x0040FB50
#define JonesConsole_HideConsole_ADDR 0x0040FB70
#define JonesConsole_PrintVersion_ADDR 0x0040FBA0
#define JonesConsole_PrintFramerate_ADDR 0x0040FBC0
#define JonesConsole_PrintPolys_ADDR 0x0040FBE0
#define JonesConsole_ShowEndCredits_ADDR 0x0040FC80
#define JonesConsole_RemoveExpired_ADDR 0x0040FC90
#define JonesHud_Startup_ADDR 0x00413B00
#define JonesHud_Shutdown_ADDR 0x00413C30
#define JonesHud_Open_ADDR 0x00413DA0
#define JonesHud_Close_ADDR 0x00414240
#define JonesHud_ToggleMenu_ADDR 0x004143E0
#define JonesHud_EnableMenu_ADDR 0x004144E0
#define JonesHud_IsMenuEnabled_ADDR 0x004144F0
#define JonesHud_Render_ADDR 0x00414500
#define JonesHud_Update_ADDR 0x004149F0
#define JonesHud_SetCanvasSize_ADDR 0x00414B80
#define JonesHud_UpdateSinCosTable_ADDR 0x00414C60
#define JonesHud_UpdateHUDLayout_ADDR 0x00414CC0
#define JonesHud_MenuOpen_ADDR 0x00414E90
#define JonesHud_MenuClose_ADDR 0x00414FF0
#define JonesHud_StartItemTranslation_ADDR 0x004152B0
#define JonesHud_HasTimeElapsed_ADDR 0x00415330
#define JonesHud_RenderEnduranceIndicator_ADDR 0x00415370
#define JonesHud_DrawEnduranceIndicator_ADDR 0x00415550
#define JonesHud_SetFadeHealthHUD_ADDR 0x00415750
#define JonesHud_RenderFadeHealthIndicator_ADDR 0x00415780
#define JonesHud_DrawHealthIndicator_ADDR 0x00415980
#define JonesHud_DrawHealthHitOverlay_ADDR 0x00415A80
#define JonesHud_RenderHealthIndicator_ADDR 0x00415BE0
#define JonesHud_DrawHealthIndicatorBase_ADDR 0x00415D50
#define JonesHud_DrawHealthIndicatorBar_ADDR 0x00415E80
#define JonesHud_DrawIndicatorBar_ADDR 0x00415FB0
#define JonesHud_Draw_ADDR 0x00416380
#define JonesHud_ProcessMenuItems_ADDR 0x00416570
#define JonesHud_MenuMoveLeft_ADDR 0x004166A0
#define JonesHud_MenuMoveRight_ADDR 0x00416790
#define JonesHud_MenuMoveDown_ADDR 0x00416900
#define JonesHud_SetSelectedMenuItem_ADDR 0x00416C60
#define JonesHud_MenuMoveUp_ADDR 0x00416CF0
#define JonesHud_BindActivateControlKeys_ADDR 0x00417100
#define JonesHud_GetKey_ADDR 0x00417150
#define JonesHud_InitializeMenu_ADDR 0x00417330
#define JonesHud_InitializeMenuSounds_ADDR 0x00417510
#define JonesHud_NewMenuItem_ADDR 0x00417560
#define JonesHud_FreeMenuItem_ADDR 0x00417660
#define JonesHud_UpdateItem_ADDR 0x00417680
#define JonesHud_RenderMenuItems_ADDR 0x00417F50
#define JonesHud_RenderMenuItem_ADDR 0x00418050
#define JonesHud_MenuActivateItem_ADDR 0x00418B80
#define JonesHud_ResetMenuItems_ADDR 0x00419200
#define JonesHud_SetLeftRightItems_ADDR 0x004198A0
#define JonesHud_SetDownUpItems_ADDR 0x004198C0
#define JonesHud_sub_4198E0_ADDR 0x004198E0
#define JonesHud_sub_419B50_ADDR 0x00419B50
#define JonesHud_InventoryItemChanged_ADDR 0x00419DE0
#define JonesHud_RenderInventoryItemChange_ADDR 0x0041A190
#define JonesHud_DrawMenuItemIcon_ADDR 0x0041A630
#define JonesHud_ShowLevelCompleted_ADDR 0x0041A810
#define JonesHud_OpenHelp_ADDR 0x0041AD20
#define JonesHud_ShowGameOverDialog_ADDR 0x0041AE30
#define JonesHud_GetHealthBarAlpha_ADDR 0x0041AFC0
#define JonesHud_SetHealthBarAlpha_ADDR 0x0041AFD0
#define JonesHud_CutsceneStart_ADDR 0x0041AFE0
#define JonesHud_EnableInterface_ADDR 0x0041AFF0
#define JonesHud_DrawCredits_ADDR 0x0041B000
#define JonesHud_InitializeGameStatistics_ADDR 0x0041C1A0
#define JonesHud_RestoreGameStatistics_ADDR 0x0041C2A0
#define JonesHud_RestoreTreasuresStatistics_ADDR 0x0041C2B0
#define JonesMain_Startup_ADDR 0x0041CC70
#define JonesMain_GameWndProc_ADDR 0x0041D3D0
#define JonesMain_HandleWMGetMinMaxInfo_ADDR 0x0041D520
#define JonesMain_HandleWMPaint_ADDR 0x0041D5C0
#define JonesMain_HandleWMTimer_ADDR 0x0041D630
#define JonesMain_HandleWMKeydown_ADDR 0x0041D650
#define JonesMain_PrintQuickSave_ADDR 0x0041D990
#define JonesMain_HandleWMActivateApp_ADDR 0x0041DAE0
#define JonesMain_HandleWMActivate_ADDR 0x0041DB10
#define JonesMain_OnAppActivate_ADDR 0x0041DB60
#define JonesMain_HandleWMChar_ADDR 0x0041DC40
#define JonesMain_HandleWMSysCommand_ADDR 0x0041DC60
#define JonesMain_HandleWMPowerBroadcast_ADDR 0x0041DC80
#define JonesMain_Shutdown_ADDR 0x0041DD50
#define JonesMain_PauseGame_ADDR 0x0041DE60
#define JonesMain_ResumeGame_ADDR 0x0041DE80
#define JonesMain_IsGamePaused_ADDR 0x0041DEA0
#define JonesMain_Process_ADDR 0x0041DEB0
#define JonesMain_ProcessWindowEvents_ADDR 0x0041DEC0
#define JonesMain_ProcessGame_ADDR 0x0041DED0
#define JonesMain_ProcessMenu_ADDR 0x0041E120
#define JonesMain_PrintFramerate_ADDR 0x0041E1E0
#define JonesMain_TogglePrintFramerate_ADDR 0x0041E300
#define JonesMain_Open_ADDR 0x0041E320
#define JonesMain_EnsureFile_ADDR 0x0041E520
#define JonesMain_Close_ADDR 0x0041E5B0
#define JonesMain_Restore_ADDR 0x0041E5C0
#define JonesMain_ProcessGamesaveState_ADDR 0x0041E730
#define JonesMain_UpdateLevelNum_ADDR 0x0041E8B0
#define JonesMain_ProcessEndLevel_ADDR 0x0041E9C0
#define JonesMain_SetBonusLevel_ADDR 0x0041EA90
#define JonesMain_ShowEndCredits_ADDR 0x0041EAD0
#define JonesMain_Credits_ADDR 0x0041EAE0
#define JonesMain_IsOpen_ADDR 0x0041EC10
#define JonesMain_GetDisplayEnvironment_ADDR 0x0041EC20
#define JonesMain_GetDisplaySettings_ADDR 0x0041EC30
#define JonesMain_CloseWindow_ADDR 0x0041EC40
#define JonesMain_FilePrintf_ADDR 0x0041EC60
#define JonesMain_Printf_ADDR 0x0041ECB0
#define JonesMain_RefreshDisplayDevice_ADDR 0x0041ED50
#define JonesMain_PlayIntro_ADDR 0x0041ED60
#define JonesMain_IntroWndProc_ADDR 0x0041F4D0
#define JonesMain_HandleWMLButtonUp_ADDR 0x0041F5D0
#define JonesMain_IntroHandleKeydown_ADDR 0x0041F5E0
#define JonesMain_IntroMovieBll565_ADDR 0x0041F6A0
#define JonesMain_IntroMovieBlt555_ADDR 0x0041F7C0
#define JonesMain_IntroMovieBlt32_ADDR 0x0041F900
#define JonesMain_Assert_ADDR 0x0041FA30
#define JonesMain_BindInventoryControlKeys_ADDR 0x0041FAF0
#define JonesMain_ResetHUD_ADDR 0x0041FB20
#define JonesMain_SaveHUD_ADDR 0x0041FB50
#define JonesMain_RestoreHUD_ADDR 0x0041FBA0
#define JonesMain_GetCurrentLevelNum_ADDR 0x0041FC00
#define JonesMain_LogErrorToFile_ADDR 0x0041FC10
#define JonesMain_LoadSettings_ADDR 0x0041FD50
#define JonesMain_ShowDevDialog_ADDR 0x004203A0
#define JonesMain_DevDialogProc_ADDR 0x004203E0
#define JonesMain_InitDevDialog_ADDR 0x00420460
#define JonesMain_DevDialogHandleCommand_ADDR 0x00420B90
#define JonesMain_DevDialogInitDisplayDevices_ADDR 0x004213D0
#define JonesMain_DevDialogUpdateRadioButtons_ADDR 0x004218F0
#define JonesMain_FindClosestVideoMode_ADDR 0x00421A90
#define JonesMain_CurDisplaySupportsBPP_ADDR 0x00421B50
#define jonesCog_Startup_ADDR 0x00401000
#define jonesCog_Shutdown_ADDR 0x00401050
#define jonesCog_RegisterFunctions_ADDR 0x00401060
#define jonesCog_HideHealthDisplay_ADDR 0x00401150
#define jonesCog_ShowHealthDisplay_ADDR 0x00401170
#define jonesCog_InventoryItemChanged_ADDR 0x00401190
#define jonesCog_EndLevelFunc_ADDR 0x004011B0
#define jonesCog_EndLevel_ADDR 0x004011C0
#define jonesCog_ExitToShell_ADDR 0x00401250
#define jonesCog_StartCutsceneFunc_ADDR 0x00401260
#define jonesCog_EndCutscene_ADDR 0x004013C0
#define jonesCog_UpdateDifficulty_ADDR 0x00401450
#define jonesCog_EnableInterfaceFunc_ADDR 0x00401480
#define jonesCog_StartCutscene_ADDR 0x004014E0
#define jonesCog_EnableInterface_ADDR 0x004014F0
#define jonesConfig_Startup_ADDR 0x00401500
#define jonesConfig_InitKeySetsPath_ADDR 0x00401660
#define jonesConfig_InitKeyActions_ADDR 0x00401780
#define jonesConfig_Shutdown_ADDR 0x004019A0
#define jonesConfig_ControlToString_ADDR 0x004019F0
#define jonesConfig_ShowMessageDialog_ADDR 0x00402400
#define jonesConfig_MessageDialogProc_ADDR 0x004024B0
#define jonesConfig_sub_4025F0_ADDR 0x004025F0
#define jonesConfig_sub_402670_ADDR 0x00402670
#define jonesConfig_SetTextControl_ADDR 0x004026D0
#define jonesConfig_sub_402A90_ADDR 0x00402A90
#define jonesConfig_NewControlScheme_ADDR 0x00402AB0
#define jonesConfig_LoadConstrolsScheme_ADDR 0x00402D70
#define jonesConfig_GetEntryIndex_ADDR 0x004030B0
#define jonesConfig_GetValueIndex_ADDR 0x00403110
#define jonesConfig_GetControllerKeySchemeIndex_ADDR 0x00403210
#define jonesConfig_SetDefaultControlScheme_ADDR 0x00403900
#define jonesConfig_BindControls_ADDR 0x00403A00
#define jonesConfig_BindJoystickControl_ADDR 0x00403F00
#define jonesConfig_InitDialogText_ADDR 0x00404040
#define jonesConfig_CreateDialogFont_ADDR 0x004040F0
#define jonesConfig_ResetDialogFont_ADDR 0x00404450
#define jonesConfig_SetWindowFontCallback_ADDR 0x00404490
#define jonesConfig_PositionAndSetTextCallback_ADDR 0x004044C0
#define jonesConfig_sub_4045E0_ADDR 0x004045E0
#define jonesConfig_GetWindowScreenRect_ADDR 0x00404A80
#define jonesConfig_SetDialogTitleAndPosition_ADDR 0x00404AF0
#define jonesConfig_GetSaveGameFilePath_ADDR 0x00404D60
#define jonesConfig_SaveGameDialogHookProc_ADDR 0x00405010
#define jonesConfig_SaveGameDialogInit_ADDR 0x00405660
#define jonesConfig_SaveGameThumbnailPaintProc_ADDR 0x004057A0
#define jonesConfig_ShowOverwriteSaveGameDlg_ADDR 0x00405850
#define jonesConfig_ShowExitGameDialog_ADDR 0x00405890
#define jonesConfig_ExitGameDlgProc_ADDR 0x004058D0
#define jonesConfig_SaveGameMsgBoxProc_ADDR 0x00405A80
#define jonesConfig_GameSaveSetData_ADDR 0x00405B30
#define jonesConfig_MsgBoxDlg_HandleWM_COMMAND_ADDR 0x00405BF0
#define jonesConfig_GetLoadGameFilePath_ADDR 0x00405C50
#define jonesConfig_sub_405F60_ADDR 0x00405F60
#define jonesConfig_LoadGameDialogHookProc_ADDR 0x00405F70
#define jonesConfig_ShowLoadGameDialog_ADDR 0x00406580
#define jonesConfig_LoadGameDialogInit_ADDR 0x004065C0
#define jonesConfig_LoadGameThumbnailPaintProc_ADDR 0x00406760
#define jonesConfig_ShowGamePlayOptions_ADDR 0x00406840
#define jonesConfig_GamePlayOptionsProc_ADDR 0x00406870
#define jonesConfig_sub_406A00_ADDR 0x00406A00
#define jonesConfig_GamePlayOptionsInitDlg_ADDR 0x00406AA0
#define jonesConfig_GamePlayOptions_HandleWM_COMMAND_ADDR 0x00406BF0
#define jonesConfig_EnableMouseControl_ADDR 0x00406D40
#define jonesConfig_FreeControlScheme_ADDR 0x00406D80
#define jonesConfig_FreeControlConfigEntry_ADDR 0x00406DB0
#define jonesConfig_ShowControlOptions_ADDR 0x00406E30
#define jonesConfig_CopyControlConfig_ADDR 0x00406F50
#define jonesConfig_CopyControlSchemes_ADDR 0x00407060
#define jonesConfig_InitControlsConfig_ADDR 0x004071C0
#define jonesConfig_LoadControlsSchemesFromSystem_ADDR 0x00407260
#define jonesConfig_SetDefaultControlSchemes_ADDR 0x00407380
#define jonesConfig_ControlOptionsProc_ADDR 0x00407410
#define jonesConfig_InitControlOptionsDlg_ADDR 0x004074C0
#define jonesConfig_ControlOptions_HandleWM_COMMAND_ADDR 0x00407780
#define jonesConfig_WriteScheme_ADDR 0x00407A10
#define jonesConfig_sub_407F60_ADDR 0x00407F60
#define jonesConfig_sub_408000_ADDR 0x00408000
#define jonesConfig_sub_408260_ADDR 0x00408260
#define jonesConfig_sub_408400_ADDR 0x00408400
#define jonesConfig_CreateNewSchemeAction_ADDR 0x004084B0
#define jonesConfig_ShowCreateControlSchemeDialog_ADDR 0x004088B0
#define jonesConfig_CreateControlSchemeProc_ADDR 0x004088E0
#define jonesConfig_InitCreateControlScheme_ADDR 0x00408990
#define jonesConfig_CreateControlScheme_Handle_WM_COMMAND_ADDR 0x004089D0
#define jonesConfig_ShowEditControlShemeDialog_ADDR 0x00408C40
#define jonesConfig_EditControlSchemProc_ADDR 0x00408C70
#define jonesConfig_InitEditControlSchemeDlg_ADDR 0x00408DA0
#define jonesConfig_sub_408ED0_ADDR 0x00408ED0
#define jonesConfig_sub_408FC0_ADDR 0x00408FC0
#define jonesConfig_sub_409200_ADDR 0x00409200
#define jonesConfig_sub_409530_ADDR 0x00409530
#define jonesConfig_sub_4096F0_ADDR 0x004096F0
#define jonesConfig_sub_4097D0_ADDR 0x004097D0
#define jonesConfig_sub_409BC0_ADDR 0x00409BC0
#define jonesConfig_AssignKeyAction_ADDR 0x00409D20
#define jonesConfig_sub_409F70_ADDR 0x00409F70
#define jonesConfig_AssignKeyDlgProc_ADDR 0x0040A0A0
#define jonesConfig_sub_40A1A0_ADDR 0x0040A1A0
#define jonesConfig_AssignControlKey_ADDR 0x0040A230
#define jonesConfig_sub_40A500_ADDR 0x0040A500
#define jonesConfig_AssignKeyDlg_HandleWM_COMMAND_ADDR 0x0040A650
#define jonesConfig_ShowReassignKeyMsgBox_ADDR 0x0040A670
#define jonesConfig_ReassignKeyDialogProc_ADDR 0x0040A6A0
#define jonesConfig_SetReassignKeyDialogText_ADDR 0x0040A750
#define jonesConfig_sub_40AA10_ADDR 0x0040AA10
#define jonesConfig_ShowDisplaySettingsDialog_ADDR 0x0040ABB0
#define jonesConfig_DisplaySettingsDialogProc_ADDR 0x0040AD40
#define jonesConfig_sub_40AE90_ADDR 0x0040AE90
#define jonesConfig_sub_40B530_ADDR 0x0040B530
#define jonesConfig_sub_40B5A0_ADDR 0x0040B5A0
#define jonesConfig_ShowAdvanceDisplaySettings_ADDR 0x0040BB60
#define jonesConfig_AdvanceDisplaySettingsDialog_ADDR 0x0040BB80
#define jonesConfig_sub_40BC40_ADDR 0x0040BC40
#define jonesConfig_sub_40BCD0_ADDR 0x0040BCD0
#define jonesConfig_sub_40C090_ADDR 0x0040C090
#define jonesConfig_ShowSoundSettingsDialog_ADDR 0x0040C480
#define jonesConfig_SoundSettingsDialogProc_ADDR 0x0040C4B0
#define jonesConfig_sub_40C650_ADDR 0x0040C650
#define jonesConfig_SoundSettings_HandleWM_COMMAND_ADDR 0x0040C730
#define jonesConfig_ShowGameOverDialog_ADDR 0x0040C800
#define jonesConfig_GameOverDialogProc_ADDR 0x0040C860
#define jonesConfig_LoadGameGetLastSavedGamePath_ADDR 0x0040C920
#define jonesConfig_GameOverDialogInit_ADDR 0x0040CA90
#define jonesConfig_GameOverDialog_HandleWM_COMMAND_ADDR 0x0040CBB0
#define jonesConfig_ShowStatisticsDialog_ADDR 0x0040CC20
#define jonesConfig_StatisticsDialogProc_ADDR 0x0040CC80
#define jonesConfig_sub_40D100_ADDR 0x0040D100
#define jonesConfig_UpdateCurrentLevelNum_ADDR 0x0040D1D0
#define jonesConfig_DrawImageOnDialogItem_ADDR 0x0040D1E0
#define jonesConfig_SetStatisticsDialogForLevel_ADDR 0x0040D300
#define jonesConfig_DrawStatisticDialogIQPoints_ADDR 0x0040D480
#define jonesConfig_InitStatisticDialog_ADDR 0x0040D720
#define jonesConfig_StatisticProc_HandleWM_COMMAND_ADDR 0x0040D820
#define jonesConfig_ShowLevelCompletedDialog_ADDR 0x0040D870
#define jonesConfig_LevelCompletedDialogProc_ADDR 0x0040D950
#define jonesConfig_InitLevelCompletedDialog_ADDR 0x0040DAA0
#define jonesConfig_ChapterCompleteDialog_HandleWM_COMMAND_ADDR 0x0040DC80
#define jonesConfig_ShowStoreDialog_ADDR 0x0040DD10
#define jonesConfig_StoreDialogProc_ADDR 0x0040DD70
#define jonesConfig_StoreHandleDragEvent_ADDR 0x0040E010
#define jonesConfig_StoreDialog_HandleWM_MBUTTONUP_ADDR 0x0040E1B0
#define jonesConfig_StoreDialog_HandleWM_MOUSEFIRST_ADDR 0x0040E290
#define jonesConfig_InitStoreDialog_ADDR 0x0040E2D0
#define jonesConfig_StoreInitItemIcons_ADDR 0x0040E480
#define jonesConfig_StoreSetListColumns_ADDR 0x0040E4F0
#define jonesConfig_StoreInitItemList_ADDR 0x0040E5D0
#define jonesConfig_UpdateBalances_ADDR 0x0040E780
#define jonesConfig_AddStoreCartItem_ADDR 0x0040E820
#define jonesConfig_RemoveStoreCartItem_ADDR 0x0040EB70
#define jonesConfig_StoreDialog_HandleWM_COMMAND_ADDR 0x0040ECA0
#define jonesConfig_ClearStoreCart_ADDR 0x0040ED30
#define jonesConfig_ShowPurchaseMessageBox_ADDR 0x0040EDF0
#define jonesConfig_PurchaseMessageBoxProc_ADDR 0x0040EE10
#define jonesConfig_InitPurchaseMessageBox_ADDR 0x0040EED0
#define jonesConfig_ShowDialogInsertCD_ADDR 0x0040F120
#define jonesConfig_DialogInsertCDProc_ADDR 0x0040F160
#define jonesConfig_InitDialogInsertCD_ADDR 0x0040F1F0
#define jonesConfig_InsertCD_HandleWM_COMMAND_ADDR 0x0040F250
#define JonesDialog_InitCursorTracking_ADDR 0x0040FF30
#define JonesDialog_TrackCursor_ADDR 0x004102F0
#define JonesDialog_ResetCursorTracking_ADDR 0x004106D0
#define JonesDialog_CopyRectToDrawBuffer_ADDR 0x00410760
#define JonesDialog_RestoreBackground_ADDR 0x00410990
#define JonesDialog_ShowDialog_ADDR 0x00410A70
#define JonesDialog_SetGameState_ADDR 0x00410B50
#define JonesDialog_ResetGameState_ADDR 0x00410E40
#define JonesDialog_DummyFlipGDI_ADDR 0x00410F00
#define JonesDialog_FullscreenDlgProc_ADDR 0x00410F10
#define JonesDialog_SubclassDialogWindowProc_ADDR 0x00411200
#define JonesDialog_HandleWM_ERASEBKGND_ADDR 0x00411A30
#define JonesDialog_HandleWM_PAINT_ADDR 0x00411AF0
#define JonesDialog_HandleWM_NCPAINT_ADDR 0x00411BA0
#define JonesDialog_AllocScreenDBISection_ADDR 0x00411C80
#define JonesDialog_AllocOffScreenGDISection_ADDR 0x00411D90
#define JonesDialog_FreeScreenDIBSection_ADDR 0x00411F10
#define JonesDialog_AllocOffScreenDIBSection_ADDR 0x00411F80
#define JonesDialog_FreeOffScreenDIBSection_ADDR 0x00412080
#define JonesDialog_ShowFileSelectDialog_ADDR 0x004120D0
#define JonesDialog_SubclassFileDialogProc_ADDR 0x004121C0
#define JonesDisplay_Startup_ADDR 0x00412490
#define JonesDisplay_Shutdown_ADDR 0x004126F0
#define JonesDisplay_Restart_ADDR 0x00412720
#define JonesDisplay_UpdateCur3DDevice_ADDR 0x00412750
#define JonesDisplay_Open_ADDR 0x004129C0
#define JonesDisplay_Close_ADDR 0x00412A80
#define JonesDisplay_EnableDualMonitor_ADDR 0x00412AB0
#define JonesDisplay_UpdateDualScreenWindowSize_ADDR 0x00412AC0
#define JonesDisplay_OpenLoadScreen_ADDR 0x00412B00
#define JonesDisplay_CloseLoadScreen_ADDR 0x00412B80
#define JonesDisplay_UpdateLoadProgress_ADDR 0x00412BC0
#define JonesControl_Startup_ADDR 0x0040FD80
#define JonesControl_Shutdown_ADDR 0x0040FE90
#define JonesControl_ProcessControls_ADDR 0x0040FEA0
#define JonesControl_EnableJoystickAxes_ADDR 0x0040FEE0
#define JonesFile_Startup_ADDR 0x00412E70
#define JonesFile_Shutdown_ADDR 0x00412EA0
#define JonesFile_Open_ADDR 0x00412ED0
#define JonesFile_Close_ADDR 0x00412FA0
#define JonesFile_GetCurrentCDNum_ADDR 0x00413060
#define JonesFile_FileExists_ADDR 0x004130E0
#define JonesFile_AddInstallPath_ADDR 0x00413110
#define JonesFile_AddCDPath_ADDR 0x00413140
#define JonesFile_InitServices_ADDR 0x004131C0
#define JonesFile_ResetServices_ADDR 0x00413250
#define JonesFile_AddResourcePath_ADDR 0x00413300
#define JonesFile_CloseResource_ADDR 0x00413410
#define JonesFile_OpenFile_ADDR 0x00413450
#define JonesFile_CloseFile_ADDR 0x00413660
#define JonesFile_FileRead_ADDR 0x00413700
#define JonesFile_FileWrite_ADDR 0x00413790
#define JonesFile_FileGets_ADDR 0x00413830
#define JonesFile_FileEOF_ADDR 0x004138C0
#define JonesFile_FileTell_ADDR 0x00413930
#define JonesFile_FileSeek_ADDR 0x004139A0
#define JonesFile_FileSize_ADDR 0x00413A30
#define JonesFile_FilePrintf_ADDR 0x00413A50
#define jonesInventory_Open_ADDR 0x0041C2F0
#define jonesInventory_Load_ADDR 0x0041C320
#define jonesInventory_Close_ADDR 0x0041C670
#define jonesInventory_ActivateItem_ADDR 0x0041C680
#define jonesInventory_UseItem_ADDR 0x0041C6A0
#define jonesInventory_ResetGameStatistics_ADDR 0x0041C740
#define jonesInventory_GetMaxDifficultyLevel_ADDR 0x0041C7B0
#define jonesInventory_UpdateSolvedHintsStatistics_ADDR 0x0041C7C0
#define jonesInventory_UpdateIQPoints_ADDR 0x0041C830
#define jonesInventory_GetTotalIQPoints_ADDR 0x0041C8E0
#define jonesInventory_AdvanceStatistics_ADDR 0x0041C980
#define jonesInventory_AdvanceFoundTreasuresStatistics_ADDR 0x0041CAB0
#define jonesInventory_UpdateGameTimeStatistics_ADDR 0x0041CAE0
#define jonesInventory_UpdateLevelStatisticTime_ADDR 0x0041CB50
#define jonesInventory_ResetStatisticsGameTime_ADDR 0x0041CC00
#define jonesString_GetString_ADDR 0x00421C60
#define jonesString_Startup_ADDR 0x00421D00
#define jonesString_Shutdown_ADDR 0x00421D60
#define Indy3D_WinMain_ADDR 0x0041CC10

// Variable addresses

#define JonesConsole_aVersion_ADDR 0x00506398
#define JonesDisplay_wallLineColor_ADDR 0x00506428
#define JonesConsole_aCmdLine_ADDR 0x00551E38
#define JonesConsole_aBuffers_ADDR 0x00551EB8
#define JonesConsole_nextIndex_ADDR 0x00552764
#define JonesConsole_aUnused_552770_ADDR 0x00552770
#define JonesConsole_cursorPos_ADDR 0x00553070
#define JonesConsole_pFont_ADDR 0x00553078
#define JonesConsole_g_bVisible_ADDR 0x0055307C
#define JonesConsole_g_bStarted_ADDR 0x00553080
#define JonesConsole_bOpened_ADDR 0x00553084
#define JonesConsole_pFuncHashtbl_ADDR 0x00553088
#define JonesHud_colorGreen_ADDR 0x00506450
#define JonesHud_colorYellow_ADDR 0x00506460
#define JonesHud_colorRed_ADDR 0x00506470
#define JonesHud_colorBlack_ADDR 0x00506480
#define JonesHud_overlayColor_ADDR 0x00506490
#define JonesHud_breathIndicatorColor_ADDR 0x005064A0
#define JonesHud_colorPink_ADDR 0x005064B0
#define JonesHud_camViewPos_ADDR 0x005064C0
#define JonesHud_pyrInventoryOrient_ADDR 0x005064D0
#define JonesHud_vecSelectedMenuItemScale_ADDR 0x005064F0
#define JonesHud_aStoreItems_ADDR 0x0050EDF0
#define JonesHud_aDfltMenuItemsOrients_ADDR 0x005156F8
#define JonesHud_apMenuIconModelNames_ADDR 0x00515F58
#define JonesHud_apItemNames_ADDR 0x00516170
#define JonesHud_aSoundFxIdxs_ADDR 0x00516388
#define JonesHud_aCreditFontColors_ADDR 0x005163C0
#define JonesHud_aCredits_ADDR 0x00516480
#define JonesHud_msecMenuItemMoveDuration_ADDR 0x00521128
#define JonesHud_selectedWeaponID_ADDR 0x0052112C
#define JonesHud_curEnduranceIndAlpha_ADDR 0x00521130
#define JonesHud_curEnduranceState_ADDR 0x00521134
#define JonesHud_curKeyId_ADDR 0x00521138
#define JonesHud_curWhoopsSndFxIdx_ADDR 0x0052113C
#define JonesHud_bCutsceneStart_ADDR 0x00554148
#define JonesHud_healthIndBarPos_ADDR 0x00554150
#define JonesHud_msecDeltaTime_ADDR 0x00554160
#define JonesHud_apMenuItems_ADDR 0x00554168
#define JonesHud_hudState_ADDR 0x00554380
#define JonesHud_msecGameOverWaitDeltaTime_ADDR 0x00554384
#define JonesHud_enduranceIndBarPos_ADDR 0x00554388
#define JonesHud_creditsSomeIdx_ADDR 0x00554398
#define JonesHud_screenHeightScalar_ADDR 0x0055439C
#define JonesHud_screenWidthScalar_ADDR 0x005543A0
#define JonesHud_selectedTreasuresMenuItemID_ADDR 0x005543A4
#define JonesHud_healthIndRect_ADDR 0x005543A8
#define JonesHud_indScale_ADDR 0x005543B8
#define JonesHud_curCelNum_ADDR 0x005543BC
#define JonesHud_msecTime_ADDR 0x005543C0
#define JonesHud_camViewMatrix_ADDR 0x005543C8
#define JonesHud_pCreditsFont2_ADDR 0x005543F8
#define JonesHud_aSlectedNdsFilePath_ADDR 0x00554400
#define JonesHud_rootMenuItemId_ADDR 0x00554480
#define JonesHud_maxItemZ_ADDR 0x00554484
#define JonesHud_bIMPState_ADDR 0x00554488
#define JonesHud_bBonusMapBought_ADDR 0x0055448C
#define JonesHud_apCreditsMats_ADDR 0x00554490
#define JonesHud_pCreditsFont1_ADDR 0x005549A4
#define JonesHud_msecLastKeyPressTime_ADDR 0x005549A8
#define JonesHud_selectedWeaponMenuItemID_ADDR 0x005549AC
#define JonesHud_selectedOptionsMenuItemID_ADDR 0x005549B0
#define JonesHud_msecRaftIndicatorFadeTime_ADDR 0x005549B4
#define JonesHud_foundTreasureValue_ADDR 0x005549B8
#define JonesHud_curHealth_ADDR 0x005549BC
#define JonesHud_creditTextHeight_ADDR 0x005549C0
#define JonesHud_nearClipPlane_ADDR 0x005549C4
#define JonesHud_aActivateKeyIds_ADDR 0x005549C8
#define JonesHud_aCosSinTableScaled_ADDR 0x005549E8
#define JonesHud_bInterfaceEnabled_ADDR 0x00554AA8
#define JonesHud_bufferHeight_ADDR 0x00554AAC
#define JonesHud_aCreditsTextHeights_ADDR 0x00554AB0
#define JonesHud_levelStartIQPoints_ADDR 0x00554FC4
#define JonesHud_enduranceRect_ADDR 0x00554FC8
#define JonesHud_dword_554FDC_ADDR 0x00554FDC
#define JonesHud_flt_554FE0_ADDR 0x00554FE0
#define JonesHud_flt_554FE4_ADDR 0x00554FE4
#define JonesHud_creditsSomeIdx_0_ADDR 0x00554FE8
#define JonesHud_selectedItemsMenuItemID_ADDR 0x00554FEC
#define JonesHud_numFoundTreasures_ADDR 0x00554FF0
#define JonesHud_creditsSomeIdx_1_ADDR 0x00554FF4
#define JonesHud_aSoundFxHandles_ADDR 0x00554FF8
#define JonesHud_flt_555034_ADDR 0x00555034
#define JonesHud_flt_555038_ADDR 0x00555038
#define JonesHud_flt_55503C_ADDR 0x0055503C
#define JonesHud_bufferWidth_ADDR 0x00555040
#define JonesHud_healthIndAlpha_ADDR 0x00555044
#define JonesHud_bFadeHealthHUD_ADDR 0x00555048
#define JonesHud_aCosSinTable_ADDR 0x00555050
#define JonesHud_msecHealthIndLastFadeUpdate_ADDR 0x00555110
#define JonesHud_playerHealthFadeStart_ADDR 0x00555114
#define JonesHud_bStartup_ADDR 0x00555118
#define JonesHud_bOpen_ADDR 0x0055511C
#define JonesHud_pArrowUp_ADDR 0x00555120
#define JonesHud_pArrowDown_ADDR 0x00555124
#define JonesHud_bHealthIndFadeSet_ADDR 0x00555128
#define JonesHud_pHudBaseMat_ADDR 0x0055512C
#define JonesHud_pHealthOverlay_ADDR 0x00555130
#define JonesHud_pHealthOverlayHitMat_ADDR 0x00555134
#define JonesHud_pPoisonedOverlayMat_ADDR 0x00555138
#define JonesHud_pEnduranceOverlayMat_ADDR 0x0055513C
#define JonesHud_pMenuFont_ADDR 0x00555140
#define JonesHud_pMenuItemLinkedList_ADDR 0x00555144
#define JonesHud_pCloseMenuItem_ADDR 0x00555148
#define JonesHud_item_flag_state_55514C_ADDR 0x0055514C
#define JonesHud_pCurSelectedMenuItem_ADDR 0x00555150
#define JonesHud_pCurInvChangedItem_ADDR 0x00555154
#define JonesHud_bExitActivated_ADDR 0x00555158
#define JonesHud_dword_55515C_ADDR 0x0055515C
#define JonesHud_bRestoreActivated_ADDR 0x00555160
#define JonesHud_bRestoreGameStatistics_ADDR 0x00555164
#define JonesHud_bItemActivated_ADDR 0x00555168
#define JonesHud_bMenuEnabled_ADDR 0x0055516C
#define JonesHud_pHudCamera_ADDR 0x00555170
#define JonesHud_pHudCanvas_ADDR 0x00555174
#define JonesHud_bMapOpen_ADDR 0x00555178
#define JonesHud_bShowMenu_ADDR 0x0055517C
#define JonesHud_hCurSndChannel_ADDR 0x00555184
#define JonesHud_bFadingHealthHUD_ADDR 0x00555188
#define JonesHud_hitOverlayAlphaState_ADDR 0x0055518C
#define JonesHud_hitOverlayAlpha_ADDR 0x00555190
#define JonesHud_bKeyStateUpdated_ADDR 0x00555194
#define JonesHud_hProcessHelp_ADDR 0x00555198
#define JonesHud_msecCreditsElapsedTime_ADDR 0x0055519C
#define JonesHud_msecCreditsFadeStart_ADDR 0x005551A0
#define JonesHud_creditsSomeHeightRatio_ADDR 0x005551A4
#define JonesHud_bFinishingCredits_ADDR 0x005551A8
#define JonesHud_bSomeCreditsBoolean_ADDR 0x005551AC
#define JonesMain_aCndLevelLoadInfos_ADDR 0x00506668
#define JonesMain_aNdyLevelLoadInfos_ADDR 0x005068A8
#define JonesMain_pfProcess_ADDR 0x00524568
#define JonesMain_g_aErrorBuffer_ADDR 0x00524570
#define JonesMain_curLevelNum_ADDR 0x005551B8
#define JonesMain_curGamesaveState_ADDR 0x005551BC
#define JonesMain_aOpenMenuKeyIds_ADDR 0x005551C0
#define JonesMain_introVideoMode_ADDR 0x005551E0
#define JonesMain_state_ADDR 0x005551E8
#define JonesMain_hs_ADDR 0x005553F0
#define JonesMain_bNoProcessWndEvents_ADDR 0x00555460
#define JonesMain_circBuf_ADDR 0x00555468
#define JonesMain_pLogFile_ADDR 0x0055547C
#define JonesMain_g_mainMutex_ADDR 0x00555480
#define JonesMain_bPrintFramerate_ADDR 0x00555484
#define JonesMain_dword_555488_ADDR 0x00555488
#define JonesMain_bGamePaused_ADDR 0x0055548C
#define JonesMain_bOpen_ADDR 0x00555490
#define JonesMain_pNdsFileName_ADDR 0x00555498
#define JonesMain_pDisplayEnv_ADDR 0x00555518
#define JonesMain_bRefreshDisplayDevice_ADDR 0x0055551C
#define JonesMain_bSystemSuspended_ADDR 0x00555520
#define JonesMain_bMouseLButtomUp_ADDR 0x00555524
#define JonesMain_pIntroVideoBuf_ADDR 0x00555528
#define JonesMain_bPrintQuickSave_ADDR 0x0055552C
#define JonesMain_bAppActivated_ADDR 0x00555530
#define JonesMain_pfCurProcess_ADDR 0x00555534
#define JonesMain_bEndCredits_ADDR 0x00555538
#define JonesMain_hSndCredits_ADDR 0x0055553C
#define JonesMain_hCreditsMusic_ADDR 0x00555540
#define JonesMain_bMenuVisible_ADDR 0x00555544
#define JonesMain_bDisplayError_ADDR 0x00555548
#define JonesMain_frameRate_ADDR 0x0055554C
#define JonesMain_frameCount_ADDR 0x00555550
#define JonesMain_frameCount0_ADDR 0x00555554
#define JonesMain_frameTime_ADDR 0x00555558
#define JonesMain_frameTime0_ADDR 0x0055555C
#define JonesMain_bEnteredOnce_ADDR 0x00555564
#define JonesMain_curVideoMode_ADDR 0x00555568
#define JonesMain_pStartupDisplayEnv_ADDR 0x005555B8
#define jonesCog_curCutsceneType_ADDR 0x0050ED10
#define jonesCog_bInterfaceEnabled_ADDR 0x0050ED14
#define jonesCog_bOverlayMapVisible_ADDR 0x005510B0
#define jonesCog_bStartup_ADDR 0x005510B4
#define jonesCog_g_bShowHealthHUD_ADDR 0x017EFFC0
#define jonesCog_g_bMenuVisible_ADDR 0x017EFFC4
#define jonesCog_g_bEnableGamesave_ADDR 0x017EFFC8
#define jonesCog_g_dword_17EFFCC_ADDR 0x017EFFCC
#define jonesConfig_aNumberGlyphMetrics_ADDR 0x00510AD0
#define jonesConfig_apDialogIconFiles_ADDR 0x00510B70
#define jonesConfig_aLevelNames_ADDR 0x00510B88
#define jonesConfig_JONES_STR_SUMMERY_ADDR 0x00510BCC
#define jonesConfig_aJonesControlActionNames_ADDR 0x00510BD0
#define jonesConfig_aJonesCapControlActionNames_ADDR 0x00510C68
#define jonesConfig_aDfltKeySets_ADDR 0x005114B8
#define jonesConfig_aDfltKeySetNames_ADDR 0x005114C8
#define jonesConfig_aControlKeyStrings_ADDR 0x005114D8
#define jonesConfig_aFontScaleFactors_ADDR 0x00511858
#define jonesConfig_dword_511954_ADDR 0x00511954
#define jonesConfig_dword_511958_ADDR 0x00511958
#define jonesConfig_perfLevel_ADDR 0x0051195C
#define jonesConfig_prevLevelNum_ADDR 0x00511960
#define jonesConfig_aStoreItemsInfo_ADDR 0x00513A18
#define jonesConfig_dword_5510B8_ADDR 0x005510B8
#define jonesConfig_dword_5510BC_ADDR 0x005510BC
#define jonesConfig_dword_5510C0_ADDR 0x005510C0
#define jonesConfig_dword_5510C4_ADDR 0x005510C4
#define jonesConfig_curControlConfig_ADDR 0x005510C8
#define jonesConfig_dword_5510E0_ADDR 0x005510E0
#define jonesConfig_aGlyphMetrics_ADDR 0x005510E8
#define jonesConfig_textMetric_ADDR 0x00551CE8
#define jonesConfig_dword_551D20_ADDR 0x00551D20
#define jonesConfig_apDialogIcons_ADDR 0x00551D28
#define jonesConfig_pKeySetsDirPath_ADDR 0x00551D40
#define jonesConfig_bDefaultRun_ADDR 0x00551DC0
#define jonesConfig_curLevelNum_ADDR 0x00551DC4
#define jonesConfig_bStartup_ADDR 0x00551DC8
#define jonesConfig_dword_551DCC_ADDR 0x00551DCC
#define jonesConfig_hdo_551DD0_ADDR 0x00551DD0
#define jonesConfig_hFontSaveGameDlg_ADDR 0x00551DD4
#define jonesConfig_hFontExitDlg_ADDR 0x00551DD8
#define jonesConfig_hFontGameSaveMsgBox_ADDR 0x00551DDC
#define jonesConfig_hFontLoadGameDlg_ADDR 0x00551DE0
#define jonesConfig_hFontGamePlayOptionsDlg_ADDR 0x00551DE4
#define jonesConfig_hFontControlOptions_ADDR 0x00551DE8
#define jonesConfig_dword_551DEC_ADDR 0x00551DEC
#define jonesConfig_hFontCreateControlSchemeDlg_ADDR 0x00551DF0
#define jonesConfig_hFontEditControlShceme_ADDR 0x00551DF4
#define jonesConfig_hFontAssignKeyDlg_ADDR 0x00551DF8
#define jonesConfig_bControlsActive_ADDR 0x00551DFC
#define jonesConfig_hFontReassignKeyDlg_ADDR 0x00551E00
#define jonesConfig_hFontDisplaySettingsDlg_ADDR 0x00551E04
#define jonesConfig_hFontAdvanceDisplaySettingsDialog_ADDR 0x00551E08
#define jonesConfig_hFontSoundSettings_ADDR 0x00551E0C
#define jonesConfig_hSndChannel_ADDR 0x00551E10
#define jonesConfig_dword_551E14_ADDR 0x00551E14
#define jonesConfig_hFontGameOverDialog_ADDR 0x00551E18
#define jonesConfig_dword_551E1C_ADDR 0x00551E1C
#define jonesConfig_hFontLevelCopletedDialog_ADDR 0x00551E20
#define jonesConfig_hFontStoreDialog_ADDR 0x00551E24
#define jonesConfig_hFontPurchaseMessageBox_ADDR 0x00551E28
#define jonesConfig_hFontDialogInsertCD_ADDR 0x00551E2C
#define JonesDialog_bWindowResized_ADDR 0x00513084
#define JonesDialog_cursorPointIdx_ADDR 0x00513088
#define JonesDialog_mousePos_ADDR 0x00553090
#define JonesDialog_pfGetDrawBufferDC_ADDR 0x00553098
#define JonesDialog_szErrorBuffer_ADDR 0x005530A0
#define JonesDialog_GDIDIBSectionInfoOffScreen_ADDR 0x005531A0
#define JonesDialog_pfReleaseDC_ADDR 0x005531DC
#define JonesDialog_windowRect_ADDR 0x005531E0
#define JonesDialog_windowPos_ADDR 0x005531F0
#define JonesDialog_stru_553210_ADDR 0x00553210
#define JonesDialog_cursorInfo_ADDR 0x00553220
#define JonesDialog_GDIDIBSectionInfo_ADDR 0x00553290
#define JonesDialog_stru_5532D0_ADDR 0x005532D0
#define JonesDialog_pfFlipPage_ADDR 0x005532E0
#define JonesDialog_dword_5532E4_ADDR 0x005532E4
#define JonesDialog_bCursorTrackingInited_ADDR 0x005532E8
#define JonesDialog_dword_5532EC_ADDR 0x005532EC
#define JonesDisplay_primaryDisplayNum_ADDR 0x0051378C
#define JonesDisplay_backBufferWidth_ADDR 0x005532F0
#define JonesDisplay_backBufferHeight_ADDR 0x005532F4
#define JonesDisplay_pCanvas_ADDR 0x005532F8
#define JonesDisplay_bDualMonitor_ADDR 0x005532FC
#define JonesDisplay_pWallpaper_ADDR 0x00553300
#define JonesDisplay_pWallLine_ADDR 0x00553304
#define JonesDisplay_hSndLoadMusic_ADDR 0x00553308
#define JonesDisplay_hSndChannelLoadMusic_ADDR 0x0055330C
#define JonesDisplay_openLoadScreenCounter_ADDR 0x00553310
#define JonesDisplay_bUpdateLoadScreen_ADDR 0x00553318
#define JonesDisplay_loadScreenState_ADDR 0x0055331C
#define JonesDisplay_bPrimaryLoadMusic_ADDR 0x00553324
#define JonesDisplay_dword_553328_ADDR 0x00553328
#define JonesDisplay_dword_55332C_ADDR 0x0055332C
#define JonesControl_bStartup_ADDR 0x0055308C
#define JonesFile_aResources_ADDR 0x00553330
#define JonesFile_sysHS_ADDR 0x00553640
#define JonesFile_aPathBuf_ADDR 0x005536B0
#define JonesFile_aFileHandles_ADDR 0x005536E0
#define JonesFile_bStartup_ADDR 0x00554130
#define JonesFile_bOpened_ADDR 0x00554134
#define JonesFile_pSysEnv_ADDR 0x00554138
#define JonesFile_pHS_ADDR 0x0055413C
#define JonesFile_hsInitCount_ADDR 0x00554140
#define jonesInventory_pGameStatistics_ADDR 0x005551B0
#define jonesInventory_msecStatisticsGameTime_ADDR 0x005551B4
#define jonesString_jonesStringsTbl_ADDR 0x005555C0
#define jonesString_aBuffer_ADDR 0x005555D0
#define jonesString_bStarted_ADDR 0x005557D0

#endif // JONES3D_RTI_ADDRESSES_H

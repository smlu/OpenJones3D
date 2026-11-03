// Added: Null version of Jones3D/Platform/Win32 (JonesMainWin32.c, JonesHudWin32.c,
// JonesDevDialogWin32.c) and the Win32 side of jonesConfig.c. There is no window, no
// message pump and no modal dialog box on this platform: the window-proc switch is a
// no-op, every dialog answers the DialogBoxParam value of the Windows user pressing
// Cancel (EndDialog(hwnd, IDCANCEL) == 2), and the timer/mutex/ComputerName services
// degrade to safe stubs. Same header contracts.
#include <unistd.h>
#include <stdio.h>

#include <j3dcore/j3d.h>

#include <Jones3D/Main/JonesMain.h>
#include <Jones3D/Main/JonesMainSys.h>
#include <Jones3D/Display/JonesHudSys.h>
#include <Jones3D/Display/jonesConfig.h>
#include <Jones3D/Platform/Null/JonesPlatformNull.h>
#include <Jones3D/types.h>
#include <sith/Dss/sithGamesave.h>

#include <std/General/stdFnames.h>
#include <std/General/stdUtil.h>

// Altered: Answers IDCANCEL (2), the value Windows dialogs hand back when the user
// cancels (WM_CLOSE/Cancel all call EndDialog(hwnd, 2) in jonesConfig.c). Callers treat
// everything but their success codes as cancel, so the Null dialogs always cancel.
#define JONESPLATFORM_NULL_IDCANCEL 2

// JonesMainSys_*: the Win32 message services moved out of JonesMain.c.

void JonesMainSys_StartQuickSaveTimer(tSysWindow hWnd)
{
    J3D_UNUSED(hWnd);
    // Added: The Windows SetTimer quicksave-text timer; a headless render keeps nothing
    // that shows on-screen, so there is nothing to schedule.
}

void JonesMainSys_ShowWindow(tSysWindow hWnd)
{
    J3D_UNUSED(hWnd);
}

int JonesMainSys_PostCloseMessage(void)
{
    // Returns the PostMessage success value; with no window the close request is
    // "delivered" so callers observe the Windows success path.
    return 1;
}

void JonesMainSys_UseGameWindowProc(void)
{
    // Added: Switches wkernel to JonesMain_GameWndProc on Windows; there is no window or
    // message queue on the Null platform, and the wkernel proc hook is Win32-only, so
    // nothing to switch.
}

void JonesMainSys_UseIntroWindowProc(void)
{
    // Added: See JonesMainSys_UseGameWindowProc.
}

void JonesMainSys_ClearWindowProc(void)
{
    // Added: See JonesMainSys_UseGameWindowProc.
}

void JonesMainSys_InstallHooks(void)
{
    // Added: No RTI hooks in the standalone Null build: nothing to hook.
}

bool JonesMainSys_GetComputerName(char* pDst, size_t size)
{
    if ( !pDst || !size )
    {
        return false;
    }

    // Added: gethostname fills the same host-name contract as the Windows
    // GetComputerName call (POSIX name, truncated to the buffer).
    char aName[128] = { 0 };
    if ( gethostname(aName, STD_ARRAYLEN(aName) - 1) != 0 || !aName[0] )
    {
        aName[0] = 'J';
    }

    size_t nameLen = strlen(aName);
    if ( nameLen >= size )
    {
        return false;
    }

    memcpy(pDst, aName, nameLen + 1);
    return true;
}

bool JonesMainSys_CloseHandle(tSysHandle handle)
{
    J3D_UNUSED(handle);
    // Added: No kernel handle table on this platform; nothing owns a Null handle yet, so
    // closing stays a no-op that keeps the true-on-success convention.
    return true;
}

int JonesMainSys_ShowFatalMessage(const char* pText, const char* pTitle, bool bIconStop)
{
    // Added: No message box: the text goes to stderr. Returns IDOK (1), the value the
    // Windows MessageBox returns when its default OK button closes the box.
    fprintf(stderr, "%s: %s%s\n", pTitle ? pTitle : "", pText ? pText : "",
        bIconStop ? " [Fatal]" : "");
    return 1;
}

// JonesMain_ShowDevDialog: developer startup dialog (devMode only).

int J3DAPI JonesMain_ShowDevDialog(tSysWindow hWnd, JonesState* pConfig)
{
    J3D_UNUSED(hWnd);
    J3D_UNUSED(pConfig);
    // Altered: The Windows dialog returns what DialogBoxParam hands back; on cancel that
    // is IDCANCEL (2), and JonesMain treats everything but 1 as "exit", so the Null
    // startup dialog always cancels.
    return JONESPLATFORM_NULL_IDCANCEL;
}

// JonesHudSys_*: the Windows help-viewer launcher and process check.

// Altered: No help viewer can be launched on this platform, so no viewer process handle
// is ever created (the Windows version returns NULL the same way on a bad help string).
tSysHandle J3DAPI JonesHud_OpenHelp(tSysHandle process)
{
    J3D_UNUSED(process);
    return NULL;
}

bool JonesHudSys_IsProcessRunning(tSysHandle hProcess)
{
    J3D_UNUSED(hProcess);
    // Added: No process was ever launched, so no process is running.
    return false;
}

// jonesConfig_*: the Win32 side of Jones3D/Display/jonesConfig.c.

static int jonesConfig_bStartup = 0;

// Added: --load path handed to the Null load-game dialog by JonesPlatformNull_SetLoadPath.
static const char* jonesPlatformNull_pLoadPath = NULL;

void JonesPlatformNull_SetLoadPath(const char* pPath)
{
    jonesPlatformNull_pLoadPath = pPath;
}

int jonesConfig_Startup(void)
{
    if ( jonesConfig_bStartup )
    {
        return 1;
    }

    // Added: The Windows Startup loads the dialog icon bitmaps through stdBmp_Load and
    // the key-set/key-action tables of the settings dialogs. None of those exist on the
    // Null platform, so the startup keeps the same "started" answer (returns 1) with no
    // dialog resources behind it.
    jonesConfig_bStartup = 1;
    return 1;
}

void jonesConfig_Shutdown(void)
{
    jonesConfig_bStartup = 0;
}

int J3DAPI jonesConfig_GetLoadGameFilePath(tSysWindow hWnd, char* pDestNdsPath)
{
    J3D_UNUSED(hWnd);

    // Added: JonesPlatformNull_SetLoadPath lets the headless runner answer the dialog
    // the way a chosen file does; without one it still cancels.
    if ( !jonesPlatformNull_pLoadPath || !jonesPlatformNull_pLoadPath[0] || !pDestNdsPath )
    {
        // Altered: The Windows OpenFileName dialog returns 1 with a chosen path and 2 when
        // the user cancels; there is no game list on this platform, so the dialog cancels.
        return JONESPLATFORM_NULL_IDCANCEL;
    }

    // Added: Fill pDestNdsPath the way the Windows dialog does on success (the OpenFileName
    // result forced to the .nds extension, then copied) and return its success value.
    char aPath[JONESCONFIG_GAMESAVE_FILEPATHSIZE] = { 0 };
    stdUtil_StringCopy(aPath, STD_ARRAYLEN(aPath), jonesPlatformNull_pLoadPath);
    stdFnames_ChangeExtEx(aPath, STD_ARRAYLEN(aPath), "nds");
    stdUtil_StringCopy(pDestNdsPath, JONESCONFIG_GAMESAVE_FILEPATHSIZE, aPath);

    // Added: One-shot, so a later load-game request (from a menu) cancels again.
    jonesPlatformNull_pLoadPath = NULL;
    return 1;
}

int J3DAPI jonesConfig_GetSaveGameFilePath(tSysWindow hWnd, char* pOutFilePath)
{
    J3D_UNUSED(hWnd);
    J3D_UNUSED(pOutFilePath);
    // Altered: Same DialogBoxParam cancel answer as GetLoadGameFilePath (1 = saved).
    return JONESPLATFORM_NULL_IDCANCEL;
}

int J3DAPI jonesConfig_ShowControlOptions(tSysWindow hWnd)
{
    J3D_UNUSED(hWnd);
    // Altered: DialogBoxParam with the user cancelling (EndDialog(hWnd, 2) trackers in
    // jonesConfig.c) hands back IDCANCEL; the control dialog has no Null substitute.
    return JONESPLATFORM_NULL_IDCANCEL;
}

int J3DAPI jonesConfig_ShowDialogInsertCD(tSysWindow hWnd, intptr_t dwInitParam)
{
    J3D_UNUSED(hWnd);
    J3D_UNUSED(dwInitParam);
    // Altered: The Windows "insert CD" prompt cancels with EndDialog(hwnd, 2); callers
    // treat the cancel answer as "do not retry, exit".
    return JONESPLATFORM_NULL_IDCANCEL;
}

int J3DAPI jonesConfig_ShowDisplaySettingsDialog(tSysWindow hWnd, StdDisplayEnvironment* pDisplayEnv, JonesDisplaySettings* pDSettings)
{
    J3D_UNUSED(hWnd);
    J3D_UNUSED(pDisplayEnv);
    J3D_UNUSED(pDSettings);
    // Altered: 1 applies changed display settings; the Null dialog cancels like the
    // Windows WM_CLOSE path (EndDialog(hWnd, 2)), so no settings are applied.
    return JONESPLATFORM_NULL_IDCANCEL;
}

int J3DAPI jonesConfig_ShowExitGameDialog(tSysWindow hWnd, char* pSaveGameFilePath)
{
    J3D_UNUSED(hWnd);
    J3D_UNUSED(pSaveGameFilePath);
    // Altered: The Windows exit dialog answers 1 ("exit"), 1187 ("exit and save") or 2
    // (cancel); the Null dialog always answers the cancel value, so the game does not
    // exit through this path.
    return JONESPLATFORM_NULL_IDCANCEL;
}

int J3DAPI jonesConfig_ShowGameOverDialog(tSysWindow hWnd, char* pRestoreFilename, tSoundHandle hSndGameOVerMus, tSoundChannelHandle* pSndChnlMus)
{
    J3D_UNUSED(hWnd);
    J3D_UNUSED(pRestoreFilename);
    J3D_UNUSED(hSndGameOVerMus);
    J3D_UNUSED(pSndChnlMus);
    // Altered: Buttons are 1177 ("restart") and 1178 ("load game"); anything else
    // (including the EndDialog(hwnd, 2) cancel of the Windows dialog) just stops the
    // died-music channel, which is the natural Null answer.
    return JONESPLATFORM_NULL_IDCANCEL;
}

int J3DAPI jonesConfig_ShowGamePlayOptions(tSysWindow hWnd)
{
    J3D_UNUSED(hWnd);
    return JONESPLATFORM_NULL_IDCANCEL;
}

int J3DAPI jonesConfig_ShowLevelCompletedDialog(tSysWindow hWnd, int* pBalance, int* apItemsState, int a4, int elapsedTime, int qiPoints, int numFoundTrasures, int foundTrasureValue, int totalTreasureValue)
{
    J3D_UNUSED(hWnd);
    J3D_UNUSED(pBalance);
    J3D_UNUSED(apItemsState);
    J3D_UNUSED(a4);
    J3D_UNUSED(elapsedTime);
    J3D_UNUSED(qiPoints);
    J3D_UNUSED(numFoundTrasures);
    J3D_UNUSED(foundTrasureValue);
    J3D_UNUSED(totalTreasureValue);
    // Altered: The Windows store dialog answers 1 when the bonus map was bought; the
    // Null dialog cancels, so no store purchases and no bonus map happen.
    return JONESPLATFORM_NULL_IDCANCEL;
}

int J3DAPI jonesConfig_ShowMessageDialog(tSysWindow hWnd, const char* pTitle, const char* pText, int iconID)
{
    J3D_UNUSED(hWnd);

    // Added: The Windows message dialog shows an icon, title and text; the text is
    // mirrored to stderr, and the cancel answer (EndDialog(hwnd, 2) of the WM_CLOSE
    // handler in jonesConfig.c MessageDialogProc) is returned.
    fprintf(stderr, "%s: %s\n", pTitle ? pTitle : "", pText ? pText : "");
    return JONESPLATFORM_NULL_IDCANCEL;
}

int J3DAPI jonesConfig_ShowSoundSettingsDialog(tSysWindow hWnd, JonesSoundSettings* pData)
{
    J3D_UNUSED(hWnd);
    J3D_UNUSED(pData);
    // Altered: 1 applies changed sound settings; the Null dialog cancels like the
    // Windows cancel paths.
    return JONESPLATFORM_NULL_IDCANCEL;
}

int J3DAPI jonesConfig_ShowStatisticsDialog(tSysWindow hWnd, SithGameStatistics* pStatistics)
{
    J3D_UNUSED(hWnd);
    J3D_UNUSED(pStatistics);
    // Altered: The statistics dialog's result is only consumed as a modal wait; cancel
    // is the Windows EndDialog(hwnd, 2) answer.
    return JONESPLATFORM_NULL_IDCANCEL;
}

void jonesConfig_UpdateCurrentLevelNum(void)
{
    // Added: No statistics dialog dialog on this platform; nothing to track.
}

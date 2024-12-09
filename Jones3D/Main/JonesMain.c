#include "JonesMain.h"
#include <j3dcore/j3dhook.h>

#include <Jones3D/Gui/JonesDialog.h>
#include <Jones3D/Display/jonesConfig.h>
#include <Jones3D/Display/JonesConsole.h>
#include <Jones3D/Display/JonesDisplay.h>
#include <Jones3D/Display/JonesHud.h>
#include <Jones3D/Main/JonesFile.h>
#include <Jones3D/Main/jonesString.h>
#include <Jones3D/Play/jonesCog.h>
#include <Jones3D/Play/JonesControl.h>
#include <Jones3D/RTI/symbols.h>

#include <rdroid/Engine/rdCamera.h>
#include <rdroid/Main/rdroid.h>
#include <rdroid/Primitives/rdFont.h>
#include <rdroid/Raster/rdCache.h>

#include <sith/DSS/sithDSS.h>
#include <sith/DSS/sithGamesave.h>
#include <sith/Devices/sithComm.h>
#include <sith/Devices/sithConsole.h>
#include <sith/Devices/sithSound.h>
#include <sith/Devices/sithSoundMixer.h>
#include <sith/Engine/sithCamera.h>
#include <sith/Engine/sithRender.h>
#include <sith/Gameplay/sithOverlayMap.h>
#include <sith/Gameplay/sithTime.h>
#include <sith/Main/sithString.h>
#include <sith/World/sithModel.h>
#include <sith/World/sithVoice.h>
#include <sith/World/sithWorld.h>

#include <smush/SmushPlay.h>

#include <sound/AudioLib.h>
#include <sound/Sound.h>
#include <sound/Driver.h>

#include <std/General/std.h>
#include <std/General/stdCircBuf.h>
#include <std/General/stdColor.h>
#include <std/General/stdEffect.h>
#include <std/General/stdFileUtil.h>
#include <std/General/stdFnames.h>
#include <std/General/stdMath.h>
#include <std/General/stdMemory.h>
#include <std/General/stdPlatform.h>
#include <std/General/stdUtil.h>
#include <std/Win95/std3D.h>
#include <std/Win95/stdConsole.h>
#include <std/Win95/stdControl.h>
#include <std/Win95/stdDisplay.h>
#include <std/Win95/stdWin95.h>

#include <wkernel/wkernel.h>
#include <w32util/wuRegistry.h>

#include <stdint.h>

#define JONES_QUICKSAVE_TEXTSHOWTIME    1000u // 1 sec
#define JONES_QUICKSAVE_TEXTSHOWTIMERID 1u 

#define JONES_FPSPRINT_CONSOLEID 102u
#define JONES_FPSPRINT_INTERVAL  2000u // 2 sec

static bool JonesMain_bStartup = false; // Added: Init to false

static JonesMainProcessFunc JonesMain_pfCurProcess = NULL; // Added: Init. to NULL
static JonesMainProcessFunc JonesMain_pfProcess    = NULL; // Changed: Set to NULL, OG JonesMain_ProcessGame was assigned
static size_t JonesMain_curLevelNum;

static tHostServices JonesMain_hs = { 0 }; // Added: Init to 0
static JonesState JonesMain_state = { 0 }; // Added: Init to 0

static bool JonesMain_bRefreshDisplayDevice = false; // Added: Init to false
static bool JonesMain_bVideoModeError       = false; // Added: Init to false
static StdVideoMode JonesMain_curVideoMode;
static StdDisplayEnvironment* JonesMain_pDisplayEnv        = NULL; // Added: Init. to NULL
static StdDisplayEnvironment* JonesMain_pStartupDisplayEnv = NULL; // Added: Init. to NULL

static tCircularBuffer JonesMain_circBuf;
static FILE* JonesMain_pLogFile = NULL; // Added: Init. to NULL

static bool JonesMain_bGamePaused      = false; // Added: Init to false
static bool JonesMain_bAssertTriggered = false; // Added: Init to false

static bool JonesMain_bWndMsgProcessed;
static bool JonesMain_bAppActivated    = false;
static bool JonesMain_bSystemSuspended = false; // Added: Init to false

static bool JonesMain_bPrintFramerate = false; // Added: Init to false
static float JonesMain_frameRate;
static size_t JonesMain_frameCount;
static size_t JonesMain_prevFrameCount;
static uint32_t JonesMain_frameTime;
static uint32_t JonesMain_prevFrameTime;

static bool JonesMain_bPrintQuickSave;
static int JonesMain_curGamesaveState = 0; // Added: Init to 0
static char JonesMain_pNdsFileName[128];

static bool JonesMain_bMenuToggled = false; // Added: Init to false
static int JonesMain_aToggleMenuKeyIds[8];

static bool JonesMain_bEndCredits = false; // Added: Init to false
static tSoundHandle JonesMain_hSndCredits;
static tSoundChannelHandle JonesMain_hCreditsMusic = 0; // Added: Init to 0

static bool JonesMain_bSkipIntro;
static int JonesMain_introVideoMode;
static bool JonesMain_bNoProcessWndEvents;
static uint8_t* JonesMain_aIntroMovieColorTable;

static const JonesLevelInfo JonesMain_aCndLevelLoadInfos[18] =
{
    {
        NULL,
        "mat\\teo_amap",
        { -5.0f, 358.0f },
        { 366.0f, 221.0f },
        0,
        NULL
    },
    {
        "00_CYN.cnd",
        "mat\\cyn_amap",
        { -5.0f, 187.0f },
        { 345.0f, 344.0f },
        1,
        "CANYONLANDS"
    },
    {
        "01_BAB.cnd",
        "mat\\bab_amap",
        { -5.0f, 163.0f },
        { 419.0f, 251.0f },
        1,
        "BABYLON"
    },
    {
        "02_RIV.cnd",
        "mat\\riv_amap",
        { -5.0f, 344.0f },
        { 423.0f, 130.0f },
        0,
        "TIAN SHAN RIVER"
    },
    {
        "03_SHS.cnd",
        "mat\\shs_amap",
        { 150.0f, 512.0f },
        { 397.0f, 274.0f },
        0,
        "SHAMBALA SANCTUARY"
    },
    {
        "05_LAG.cnd",
        "mat\\lag_amap",
        { -5.0f, 147.0f },
        { 435.0f, 380.0f },
        1,
        "PALAWAN LAGOON"
    },
    {
        "06_VOL.cnd",
        "mat\\vol_amap",
        { -5.0f, 184.0f },
        { 330.0f, 269.0f },
        0,
        "PALAWAN VOLCANO"
    },
    {
        "07_TEM.cnd",
        "mat\\tem_amap",
        { -5.0f, 127.0f },
        { 349.0f, 312.0f },
        0,
        "PALAWAN TEMPLE"
    },
    {
        "16_JEP.cnd",
        "mat\\jep_amap",
        { -5.0f, 223.0f },
        { 370.0f, 309.0f },
        1,
        "JEEP"
    },
    {
        "08_TEO.cnd",
        "mat\\teo_amap",
        { -5.0f, 358.0f },
        { 365.0f, 221.0f },
        1,
        "TEOTIAACAN"
    },
    {
        "09_OLV.cnd",
        "mat\\olv_amap",
        { -5.0f, 277.0f },
        { 413.0f, 257.0f },
        0,
        "OLMEC VALLEY"
    },
    {
        "10_SEA.cnd",
        "mat\\sea_amap",
        { -5.0f, 252.0f },
        { 380.0f, 199.0f },
        0,
        "PUDOVKIN SHIP"
    },
    {
        "11_PYR.cnd",
        "mat\\pyr_amap",
        { -5.0f, 243.0f },
        { 421.0f, 391.0f },
        1,
        "MEROE PYRAMIDS"
    },
    {
        "12_SOL.cnd",
        "mat\\sol_amap",
        { -5.0f, 208.0f },
        { 348.0f, 266.0f },
        1,
        "SOLOMON'S MINES"
    },
    {
        "13_NUB.cnd",
        "mat\\nub_amap",
        { -5.0f, 237.0f },
        { 383.0f, 245.0f },
        0,
        "NUB'S TOMB"
    },
    {
        "14_INF.cnd",
        "mat\\inf_amap",
        { 85.0f, 485.0f },
        { 395.0f, 224.0f },
        0,
        "INFERNAL MACHINE"
    },
    {
        "15_AET.cnd",
        "mat\\aet_amap",
        { 329.0f, -5.0f },
        { 329.0f, 316.0f },
        1,
        "AETHERIUM"
    },
    {
        "17_PRU.cnd",
        "mat\\pru_amap",
        { 565.0f, -5.0f },
        { 176.0f, 341.0f },
        1,
        "RETURN TO PERU"
    }
};

static const JonesLevelInfo JonesMain_aNdyLevelLoadInfos[18] =
{
    {
        NULL,
        "mat\\teo_amap",
        { -5.0f, 358.0f },
        { 366.0f, 221.0f },
        1,
        NULL
    },
    {
        "00_CYN.ndy",
        "mat\\cyn_amap",
        { -5.0f, 187.0f },
        { 345.0f, 344.0f },
        1,
        "CANYONLANDS"
    },
    {
        "01_BAB.ndy",
        "mat\\bab_amap",
        { -5.0f, 163.0f },
        { 419.0f, 251.0f },
        1,
        "BABYLON"
    },
    {
        "02_RIV.ndy",
        "mat\\riv_amap",
        { -5.0f, 344.0f },
        { 423.0f, 130.0f },
        1,
        "TIAN SHAN RIVER"
    },
    {
        "03_SHS.ndy",
        "mat\\shs_amap",
        { 150.0f, 512.0f },
        { 397.0f, 274.0f },
        1,
        "SHAMBALA SANCTUARY"
    },
    {
        "05_LAG.ndy",
        "mat\\lag_amap",
        { -5.0f, 147.0f },
        { 435.0f, 380.0f },
        1,
        "PALAWAN LAGOON"
    },
    {
        "06_VOL.ndy",
        "mat\\vol_amap",
        { -5.0f, 184.0f },
        { 330.0f, 267.0f },
        1,
        "PALAWAN VOLCANO"
    },
    {
        "07_TEM.ndy",
        "mat\\tem_amap",
        { -5.0f, 127.0f },
        { 349.0f, 312.0f },
        1,
        "PALAWAN TEMPLE"
    },
    {
        "16_JEP.ndy",
        "mat\\jep_amap",
        { -5.0f, 223.0f },
        { 370.0f, 308.0f },
        1,
        "JEEP"
    },
    {
        "08_TEO.ndy",
        "mat\\teo_amap",
        { -5.0f, 358.0f },
        { 365.0f, 221.0f },
        1,
        "TEOTIAACAN"
    },
    {
        "09_OLV.ndy",
        "mat\\olv_amap",
        { -5.0f, 277.0f },
        { 413.0f, 257.0f },
        1,
        "OLMEC VALLEY"
    },
    {
        "10_SEA.ndy",
        "mat\\sea_amap",
        { -5.0f, 252.0f },
        { 380.0f, 199.0f },
        1,
        "PUDOVKIN SHIP"
    },
    {
        "11_PYR.ndy",
        "mat\\pyr_amap",
        { -5.0f, 243.0f },
        { 421.0f, 391.0f },
        1,
        "MEROE PYRAMIDS"
    },
    {
        "12_SOL.ndy",
        "mat\\sol_amap",
        { -5.0f, 208.0f },
        { 348.0f, 266.0f },
        1,
        "SOLOMON'S MINES"
    },
    {
        "13_NUB.ndy",
        "mat\\nub_amap",
        { -5.0f, 237.0f },
        { 383.0f, 245.0f },
        1,
        "NUB'S TOMB"
    },
    {
        "14_INF.ndy",
        "mat\\inf_amap",
        { 85.0f, 485.0f },
        { 395.0f, 224.0f },
        1,
        "INFERNAL MACHINE"
    },
    {
        "15_AET.ndy",
        "mat\\aet_amap",
        { 327.0f, -5.0f },
        { 327.0f, 316.0f },
        1,
        "AETHERIUM"
    },
    {
        "17_PRU.ndy",
        "mat\\pru_amap",
        { 565.0f, -5.0f },
        { 176.0f, 340.0f },
        1,
        "RETURN TO PERU"
    }
};

int J3DAPI JonesMain_GameWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, int* pReturnVal);
void J3DAPI JonesMain_HandleWMGetMinMaxInfo(HWND hwnd, LPMINMAXINFO pMinMaxInfo);
void J3DAPI JonesMain_HandleWMPaint(HWND hWnd);
int J3DAPI JonesMain_HandleWMTimer(HWND hWnd, WPARAM timerID);
void J3DAPI JonesMain_HandleWMKeydown(HWND hWnd, WPARAM vk, int a3, uint16_t repreatCount, uint16_t exkeyflags);
void JonesMain_PrintQuickSave(void);
int J3DAPI JonesMain_HandleWMActivateApp(HWND hWnd, WPARAM wParam, LPARAM lParam);
LRESULT J3DAPI JonesMain_HandleWMActivate(HWND hWnd, uint16_t activateState, LPARAM hwndActivate, uint16_t minimizedState);
void J3DAPI JonesMain_OnAppActivate(HWND hWnd, int bActivated);
void J3DAPI JonesMain_HandleWMChar(HWND hwnd, char chr, uint16_t repeatCount);
void J3DAPI JonesMain_HandleWMSysCommand(HWND hWnd, WPARAM wParam, uint16_t curPosX, uint16_t curPosY);
void J3DAPI JonesMain_HandleWMPowerBroadcast(HWND hWnd, WPARAM wParam);

int JonesMain_ProcessWindowEvents(void);
int JonesMain_ProcessGame(void);
void JonesMain_ProcessMenu(void);

void J3DAPI JonesMain_PrintFramerate();

int J3DAPI JonesMain_EnsureFile(const char* pFilename);
int J3DAPI JonesMain_Restore(const char* pNdsFilePath);

int JonesMain_ProcessStartGame(void);
int JonesMain_ProcessCredits(void);

int J3DAPI JonesMain_FilePrintf(const char* pFormat, ...);
int J3DAPI JonesMain_NoLog(const char* pFormat, ...); // Added

int J3DAPI JonesMain_IntroWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, int* pRetValue);
void J3DAPI JonesMain_IntroHandleWMLButtonUp(HWND hwnd, uint16_t curPosX, uint16_t curPosY, WPARAM mvk);
void J3DAPI JonesMain_IntroHandleWMKeydown(HWND hwnd, WPARAM vk, int a3, uint16_t repreatCount, uint16_t exkeyflags);
int J3DAPI JonesMain_IntroMovieBlt565(const SmushBitmap* pBitmap, int a2);
int J3DAPI JonesMain_IntroMovieBlt555(const SmushBitmap* pBitmap, int a2);
int J3DAPI JonesMain_IntroMovieBlt32(const SmushBitmap* pBitmap, int a2);

void JonesMain_InitializeHUD(void);
int J3DAPI JonesMain_SaveHUD(DPID idTo, unsigned int outstream);
int J3DAPI JonesMain_RestoreHUD(const SithMessage* pMsg);

void J3DAPI JonesMain_LoadSettings(StdDisplayEnvironment* pDisplayEnv, JonesState* pConfig);

INT_PTR CALLBACK JonesMain_DevDialogProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
int J3DAPI JonesMain_InitDevDialog(HWND hDlg, WPARAM wParam, JonesState* pConfig);
void J3DAPI JonesMain_DevDialogHandleCommand(HWND hWnd, int controlId, LPARAM lParam, int hiWParam);
void J3DAPI JonesMain_DevDialogInitDisplayDevices(HWND hDlg, JonesState* pConfig);
void J3DAPI JonesMain_DevDialogUpdateRadioButtons(HWND hDlg, const JonesState* pState);
bool J3DAPI JonesMain_CurDisplaySupportsBPP(const JonesDisplaySettings* pSettings, size_t bpp);


void JonesMain_InstallHooks(void)
{
    J3D_HOOKFUNC(JonesMain_Startup);
    J3D_HOOKFUNC(JonesMain_GameWndProc);
    J3D_HOOKFUNC(JonesMain_HandleWMGetMinMaxInfo);
    J3D_HOOKFUNC(JonesMain_HandleWMPaint);
    J3D_HOOKFUNC(JonesMain_HandleWMTimer);
    J3D_HOOKFUNC(JonesMain_HandleWMKeydown);
    J3D_HOOKFUNC(JonesMain_PrintQuickSave);
    J3D_HOOKFUNC(JonesMain_HandleWMActivateApp);
    J3D_HOOKFUNC(JonesMain_HandleWMActivate);
    J3D_HOOKFUNC(JonesMain_OnAppActivate);
    J3D_HOOKFUNC(JonesMain_HandleWMChar);
    J3D_HOOKFUNC(JonesMain_HandleWMSysCommand);
    J3D_HOOKFUNC(JonesMain_HandleWMPowerBroadcast);
    J3D_HOOKFUNC(JonesMain_Shutdown);
    J3D_HOOKFUNC(JonesMain_PauseGame);
    J3D_HOOKFUNC(JonesMain_ResumeGame);
    J3D_HOOKFUNC(JonesMain_IsGamePaused);
    J3D_HOOKFUNC(JonesMain_Process);
    J3D_HOOKFUNC(JonesMain_ProcessWindowEvents);
    J3D_HOOKFUNC(JonesMain_ProcessGame);
    J3D_HOOKFUNC(JonesMain_ProcessMenu);
    J3D_HOOKFUNC(JonesMain_PrintFramerate);
    J3D_HOOKFUNC(JonesMain_TogglePrintFramerate);
    J3D_HOOKFUNC(JonesMain_Open);
    J3D_HOOKFUNC(JonesMain_EnsureFile);
    J3D_HOOKFUNC(JonesMain_Close);
    J3D_HOOKFUNC(JonesMain_Restore);
    J3D_HOOKFUNC(JonesMain_ProcessGamesaveState);
    J3D_HOOKFUNC(JonesMain_UpdateLevelNum);
    J3D_HOOKFUNC(JonesMain_ProcessStartGame);
    J3D_HOOKFUNC(JonesMain_SetBonusLevel);
    J3D_HOOKFUNC(JonesMain_ShowEndCredits);
    J3D_HOOKFUNC(JonesMain_ProcessCredits);
    J3D_HOOKFUNC(JonesMain_HasStarted);
    J3D_HOOKFUNC(JonesMain_GetDisplayEnvironment);
    J3D_HOOKFUNC(JonesMain_GetDisplaySettings);
    J3D_HOOKFUNC(JonesMain_CloseWindow);
    J3D_HOOKFUNC(JonesMain_FilePrintf);
    J3D_HOOKFUNC(JonesMain_Log);
    J3D_HOOKFUNC(JonesMain_RefreshDisplayDevice);
    J3D_HOOKFUNC(JonesMain_PlayIntroMovie);
    J3D_HOOKFUNC(JonesMain_IntroWndProc);
    J3D_HOOKFUNC(JonesMain_IntroHandleWMLButtonUp);
    J3D_HOOKFUNC(JonesMain_IntroHandleWMKeydown);
    J3D_HOOKFUNC(JonesMain_IntroMovieBlt565);
    J3D_HOOKFUNC(JonesMain_IntroMovieBlt555);
    J3D_HOOKFUNC(JonesMain_IntroMovieBlt32);
    J3D_HOOKFUNC(JonesMain_Assert);
    J3D_HOOKFUNC(JonesMain_BindToggleMenuControlKeys);
    J3D_HOOKFUNC(JonesMain_InitializeHUD);
    J3D_HOOKFUNC(JonesMain_SaveHUD);
    J3D_HOOKFUNC(JonesMain_RestoreHUD);
    J3D_HOOKFUNC(JonesMain_GetCurrentLevelNum);
    J3D_HOOKFUNC(JonesMain_LogErrorToFile);
    J3D_HOOKFUNC(JonesMain_LoadSettings);
    J3D_HOOKFUNC(JonesMain_ShowDevDialog);
    J3D_HOOKFUNC(JonesMain_DevDialogProc);
    J3D_HOOKFUNC(JonesMain_InitDevDialog);
    J3D_HOOKFUNC(JonesMain_DevDialogHandleCommand);
    J3D_HOOKFUNC(JonesMain_DevDialogInitDisplayDevices);
    J3D_HOOKFUNC(JonesMain_DevDialogUpdateRadioButtons);
    J3D_HOOKFUNC(JonesMain_FindClosestVideoMode);
    J3D_HOOKFUNC(JonesMain_CurDisplaySupportsBPP);
}

void JonesMain_ResetGlobals(void)
{
    char JonesMain_g_aErrorBuffer_tmp[1024] = "Unknown error";
    memcpy(&JonesMain_g_aErrorBuffer, &JonesMain_g_aErrorBuffer_tmp, sizeof(JonesMain_g_aErrorBuffer));

    memset(&JonesMain_g_mainMutex, 0, sizeof(JonesMain_g_mainMutex));
}

int J3DAPI JonesMain_Startup(const char* lpCmdLine)
{
    JonesMain_bPrintQuickSave = false;
    JonesMain_curLevelNum     = 0;

    memset(&JonesMain_circBuf, 0, sizeof(JonesMain_circBuf));
    memset(JonesMain_aToggleMenuKeyIds, 0, 8u); // TODO: [BUG]

    // Setup process routines
    JonesMain_pfProcess = JonesMain_ProcessGame;
    wkernel_SetWindowProc(JonesMain_GameWndProc);

    // Initialize Host services
    stdPlatform_InitServices(&JonesMain_hs);

    stdStartup(&JonesMain_hs);
    rdSetServices(&JonesMain_hs);
    sithSetServices(&JonesMain_hs);
    sithSound_InitializeSound(&JonesMain_hs);

    JonesDisplay_UpdateDualScreenWindowSize(&JonesMain_state.displaySettings);
    JonesFile_Startup(&JonesMain_hs);

    if ( wuRegistry_Startup(HKEY_LOCAL_MACHINE, "Software\\LucasArts Entertainment Company LLC\\Indiana Jones and the Infernal Machine\\v1.0") )
    {
        JonesMain_LogErrorToFile("Couldn't open the registry.");
        JonesMain_CloseWindow();
        return 1;
    }

    wuRegistry_GetStr("User Path", JonesMain_state.aInstallPath, STD_ARRAYLEN(JonesMain_state.aInstallPath), "");
    wuRegistry_GetStr("Source Dir", JonesMain_state.aCDPath, STD_ARRAYLEN(JonesMain_state.aCDPath), "");

    std3D_SetFindAllDevices(wuRegistry_GetIntEx("AllDevices", 0));

    JonesFile_Open(&JonesMain_hs, JonesMain_state.aInstallPath, JonesMain_state.aCDPath);
    if ( jonesString_Startup() )
    {
        JonesMain_LogErrorToFile("Could not open game text.");
        JonesMain_CloseWindow();
        return 1;
    }

    if ( sithString_Startup() )
    {
        const char* pErrorText = jonesString_GetString("JONES_STR_SITHERROR");
        if ( pErrorText )
        {
            JonesMain_LogErrorToFile(pErrorText);
        }

        JonesMain_CloseWindow();
        return 1;
    }

    JonesMain_pDisplayEnv = std3D_BuildDisplayEnvironment();
    if ( !JonesMain_pDisplayEnv )
    {
        const char* pErrorText = jonesString_GetString("JONES_STR_DISPLAYERROR");
        if ( pErrorText )
        {
            JonesMain_LogErrorToFile(pErrorText);
        }

        JonesMain_CloseWindow();
        return 1;
    }

    JonesDisplay_EnableDualMonitor(1);
    JonesDisplay_UpdateDualScreenWindowSize(&JonesMain_state.displaySettings);

    JonesMain_LoadSettings(JonesMain_pDisplayEnv, &JonesMain_state);

    sithSound_StartupSound();
    if ( sithSound_Startup(JonesMain_state.bSound3D) )
    {
        const char* pErrorText = jonesString_GetString("JONES_STR_SOUNDERROR");
        if ( pErrorText )
        {
            JonesMain_LogErrorToFile(pErrorText);
        }
    }

    float defultVol = Sound_GetMaxVolume();
    JonesMain_state.soundVolume = wuRegistry_GetFloat("Sound Volume", defultVol);
    wuRegistry_SaveFloat("Sound Volume", JonesMain_state.soundVolume);

    Sound_SetMaxVolume(JonesMain_state.soundVolume);
    SmushPlay_SetGlobalVolume((size_t)(JonesMain_state.soundVolume * 127.0f)); // TODO: Can be removed as it's being set in PlayIntroMovie

    // Handle start mode
    int bSuccess    = 0;
    bool bPlayIntro = true;
    switch ( JonesMain_state.startMode )
    {
        case JONES_STARTMODE_STARTGAME:
        {
            sscanf_s(lpCmdLine, "%d", &JonesMain_curLevelNum);
            if ( JonesMain_curLevelNum == 0 || JonesMain_curLevelNum > STD_ARRAYLEN(JonesMain_aCndLevelLoadInfos) )
            {
                JonesMain_curLevelNum = 1;
            }

            STD_STRCPY(JonesMain_state.aCurLevelFilename, JonesMain_aCndLevelLoadInfos[JonesMain_curLevelNum].pFilename);
            bSuccess = 1;

        } break; // start game

        case JONES_STARTMODE_LOADGAME:
        {
            if ( jonesConfig_GetLoadGameFilePath(stdWin95_GetWindow(), JonesMain_pNdsFileName) != 1 )
            {
                return 1;
            }

            // Skip intro video
            bPlayIntro = false;

        } break; // Start game

        case JONES_STARTMODE_SOUNDSETTINGS:
        {
            bSuccess = jonesConfig_ShowSoundSettingsDialog(stdWin95_GetWindow(), &JonesMain_state.soundVolume);
            return 1; // exit
        }

        case JONES_STARTMODE_DISPLAYSETTINGS:
        {
            bSuccess = jonesConfig_ShowDisplaySettingsDialog(stdWin95_GetWindow(), JonesMain_pDisplayEnv, &JonesMain_state.displaySettings);
            return 1; // exit
        }

        default: // Developer dialog
        {
            if ( JonesMain_ShowDevDialog(stdWin95_GetWindow(), &JonesMain_state) != 1 ) {
                return 1; // exit
            }
        } break;
    };

    // Start game

    // Init jones vars

    JonesDisplay_EnableDualMonitor(JonesMain_state.displaySettings.bDualMonitor);

    // Init output mode
    switch ( JonesMain_state.outputMode )
    {
        case JONES_OUTPUTMODE_CONSOLE:
        {
            switch ( JonesMain_state.logLevel )
            {
                case JONES_LOGLEVEL_ERROR:
                    JonesMain_hs.pErrorPrint = JonesMain_Log;
                    break;
                case JONES_LOGLEVEL_NORMAL:
                    JonesMain_hs.pErrorPrint  = JonesMain_Log;
                    JonesMain_hs.pStatusPrint = stdConsolePrintf;
                    break;
                case JONES_LOGLEVEL_VERBOSE:
                    JonesMain_hs.pErrorPrint  = JonesMain_Log;
                    JonesMain_hs.pStatusPrint = stdConsolePrintf;
                    JonesMain_hs.pDebugPrint  = stdConsolePrintf;
                    break;
                default:
                    break; // No logging
            };

            stdConsole_Startup("Debug", FOREGROUND_RED | FOREGROUND_BLUE | FOREGROUND_GREEN, /*bShowMinimized=*/0); // White text
        } break;

        case JONES_OUTPUTMODE_LOGFILE:
        {
            fopen_s(&JonesMain_pLogFile, "JonesLog.txt", "w+"); // TODO: Add check for case whe fopen fails
            switch ( JonesMain_state.logLevel )
            {
                case JONES_LOGLEVEL_ERROR:
                    JonesMain_hs.pErrorPrint  = JonesMain_Log;
                    break;
                case JONES_LOGLEVEL_NORMAL:
                    JonesMain_hs.pErrorPrint  = JonesMain_Log;
                    JonesMain_hs.pStatusPrint = JonesMain_FilePrintf;
                    break;
                case JONES_LOGLEVEL_VERBOSE:
                    JonesMain_hs.pErrorPrint  = JonesMain_Log;
                    JonesMain_hs.pStatusPrint = JonesMain_FilePrintf;
                    JonesMain_hs.pDebugPrint  = JonesMain_FilePrintf;
                    break;
                default:
                    break; // No logging
            };
        } break;

        default: // JONES_OUTPUTMODE_NONE
        {
            switch ( JonesMain_state.logLevel )
            {
                // TODO: why nothing is set for normal mode??
                //       Note in this case the default log is used that was set in stdPlatform_InitServices
                case JONES_LOGLEVEL_VERBOSE:
                    JonesMain_hs.pDebugPrint = JonesMain_hs.pStatusPrint; // ???
                    // Fallthrough
                case JONES_LOGLEVEL_ERROR:
                    JonesMain_hs.pMessagePrint = JonesMain_NoLog;
                    JonesMain_hs.pStatusPrint  = JonesMain_NoLog;
                    JonesMain_hs.pWarningPrint = JonesMain_NoLog;
                    JonesMain_hs.pDebugPrint   = JonesMain_NoLog;
                    JonesMain_hs.pErrorPrint   = JonesMain_Log;
                    break;
                default:
                    break; // No logging
            };
        } break;
    };

    // Set assert handler
    JonesMain_hs.pAssert = JonesMain_Assert;

    stdCircBuf_New(&JonesMain_circBuf, 4, 128);

    // Startup modules and load level

    STDLOG_DEBUG("Starting up Indiana Jones and the Infernal Machine.\n");

    stdEffect_Startup();
    rdStartup();

    if ( sithStartup() )
    {
        const char* pErrorText = jonesString_GetString("JONES_STR_SITHSTARTERROR");
        if ( pErrorText )
        {
            JonesMain_LogErrorToFile(pErrorText);
        }

        JonesMain_CloseWindow();
        return 1;
    }


    // Init save game callbacks
    sithSetOpenCallback(JonesMain_InitializeHUD);
    sithGamesave_SetSaveGameCallback(JonesMain_SaveHUD);
    sithMessage_RegisterFunction(SITHDSS_HUDSTATE, JonesMain_RestoreHUD);

    if ( JonesConsole_Startup() )
    {
        const char* pErrorText = jonesString_GetString("JONES_STR_CONSOLEERROR");
        if ( pErrorText )
        {
            JonesMain_LogErrorToFile(pErrorText);
        }

        JonesMain_CloseWindow();
        return 1;
    }

    if ( JonesHud_Startup() )
    {
        const char* pErrorText = jonesString_GetString("JONES_STR_CTRLERROR");
        if ( pErrorText )
        {
            JonesMain_LogErrorToFile(pErrorText);
        }

        JonesMain_CloseWindow();
        return 1;
    }

    // Set performance level
    sithSetPerformanceLevel(JonesMain_state.performanceLevel);

    // Init debug flags
    sithMain_g_sith_mode.debugModeFlags = 0;
    if ( JonesMain_state.bDevMode )
    {
        sithMain_g_sith_mode.debugModeFlags |= SITHDEBUG_INEDITOR;
    }

    if ( JonesDisplay_Startup(&JonesMain_state.displaySettings) )
    {
        // Error starting display module with set video mode, try setting default video mode

        JonesDisplay_SetDefaultVideoMode(JonesMain_pDisplayEnv, &JonesMain_state.displaySettings);
        if ( JonesMain_state.displaySettings.displayDeviceNum == -1 )
        {
            // Error setting default video mode
            char aErrorText[128] = { 0 };
            const char* pErrorText = jonesString_GetString("JONES_STR_NO3DCARD");
            STD_FORMAT(aErrorText, "%s", pErrorText);

            jonesConfig_ShowMessageDialog(stdWin95_GetWindow(), "JONES_STR_3D_DEVICE", aErrorText, 136);
            JonesMain_CloseWindow();
            return 1;
        }

        // There was no error in setting default mode, lets try starting default module
        if ( JonesDisplay_Startup(&JonesMain_state.displaySettings) )
        {
            // Nope it won't budge, we have no option but to quit the game
            const char* pErrorText = jonesString_GetString("JONES_STR_DISPLAYERROR");
            if ( pErrorText )
            {
                JonesMain_LogErrorToFile(pErrorText);
            }

            JonesMain_CloseWindow(); // Quit game
            return 1;
        }
    }

    if ( JonesControl_Startup() )
    {
        const char* pErrorText = jonesString_GetString("JONES_STR_CTRLERROR");
        if ( pErrorText )
        {
            JonesMain_LogErrorToFile(pErrorText);
        }

        JonesMain_CloseWindow();
        return 1;
    }

    if ( JonesConsole_Open() )
    {
        const char* pErrorText = jonesString_GetString("JONES_STR_CONSOLEERROR");
        if ( pErrorText )
        {
            JonesMain_LogErrorToFile(pErrorText);
        }

        JonesMain_CloseWindow();
        return 1;
    }

    if ( jonesCog_Startup() )
    {
        const char* pErrorText = jonesString_GetString("JONES_STR_COGERROR");
        if ( pErrorText )
        {
            JonesMain_LogErrorToFile(pErrorText);
        }

        JonesMain_CloseWindow();
        return 1;
    }

    // Play intro video
    if ( bPlayIntro && JonesMain_PlayIntroMovie() )
    {
        // Error playing intro movie
        return 1;
    }

    // Finish playing intro video, now load static resource level

    // Set load screen for loading static world
    if ( JonesMain_state.startMode == JONES_STARTMODE_LOADGAME )
    {
        JonesMain_Restore(JonesMain_pNdsFileName);
    }
    else
    {
        JonesMain_UpdateLevelNum();
        JonesDisplay_OpenLoadScreen( // TODO: what if ndy file?
            JonesMain_aCndLevelLoadInfos[JonesMain_curLevelNum].pMatFilename,
            JonesMain_aCndLevelLoadInfos[JonesMain_curLevelNum].wallineStart.x,
            JonesMain_aCndLevelLoadInfos[JonesMain_curLevelNum].wallineStart.y,
            JonesMain_aCndLevelLoadInfos[JonesMain_curLevelNum].wallineEnd.x,
            JonesMain_aCndLevelLoadInfos[JonesMain_curLevelNum].wallineEnd.y,
            JonesMain_aCndLevelLoadInfos[JonesMain_curLevelNum].bPrimaryMusicTheme
        );
    }

    // Load Static level
    char aStaticFilename[64];
    STD_STRCPY(aStaticFilename, "Jones3DSTATIC");
    stdFnames_ChangeExt(aStaticFilename, "cnd");

    if ( sithOpenStatic(aStaticFilename) )
    {
        stdFnames_ChangeExt(aStaticFilename, "ndy");
        if ( sithOpenStatic(aStaticFilename) )
        {
            // Error loading static level
            const char* pErrorText = jonesString_GetString("JONES_STR_STATICERROR");
            if ( pErrorText )
            {
                JonesMain_LogErrorToFile(pErrorText);
            }

            JonesMain_CloseWindow(); // quit game
            return 1;
        }
    }

    // Finished loading static resource level
    JonesDisplay_CloseLoadScreen();

    // Open HUD
    if ( JonesHud_Open() )
    {
        // Error opening HUD
        const char* pErrorText = jonesString_GetString("JONES_STR_HUDERROR");
        if ( pErrorText )
        {
            JonesMain_LogErrorToFile(pErrorText);
        }

        JonesMain_CloseWindow();
        return 1;
    }

    // Load level
    if ( JonesMain_Open() )
    {
        // Error opening level
        const char* pFormat = jonesString_GetString("JONES_STR_OPENERROR");
        if ( pFormat )
        {
            char aErrorText[128] = { 0 };
            STD_FORMAT(aErrorText, pFormat, JonesMain_aCndLevelLoadInfos[JonesMain_curLevelNum].pName);
            JonesMain_LogErrorToFile(aErrorText);
        }

        JonesMain_CloseWindow();
        return 1;
    }

    // Success starting game
    JonesMain_bStartup = true;
    return 0;
}

int J3DAPI JonesMain_GameWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, int* pReturnVal)
{
    JonesMain_bWndMsgProcessed = false;
    switch ( uMsg )
    {
        case WM_TIMER:
        {
            JonesMain_HandleWMTimer(hWnd, wParam);
            return 0;
        }
        case WM_POWERBROADCAST:
        {
            JonesMain_HandleWMPowerBroadcast(hWnd, wParam);
            return lParam;
        }
        case WM_SYSCOMMAND:
        {
            JonesMain_HandleWMSysCommand(hWnd, wParam, LOWORD(lParam), HIWORD(lParam));
            return 0;
        }
        case WM_KEYDOWN:
        {
            JonesMain_HandleWMKeydown(hWnd, wParam, 1, LOWORD(lParam), HIWORD(lParam));
            return 0; // TODO: should break instead of return and let the JonesMain_bWndMsgProcessed scope do it's thing
        }
        case WM_CHAR:
        {
            JonesMain_HandleWMChar(hWnd, (char)wParam, LOWORD(lParam));
            return 0;
        }
        case WM_GETMINMAXINFO:
        {
            JonesMain_HandleWMGetMinMaxInfo(hWnd, (LPMINMAXINFO)lParam);
            return 0; // TODO: should break instead of return and let the JonesMain_bWndMsgProcessed scope do it's thing
        }
        case  WM_ACTIVATEAPP:
        {
            JonesMain_HandleWMActivateApp(hWnd, wParam, lParam);
            return 0;
        }
        case WM_PAINT:
        {
            JonesMain_HandleWMPaint(hWnd);
            return 0;
        }
        case  WM_ACTIVATE:
        {
            JonesMain_HandleWMActivate(hWnd, LOWORD(wParam), lParam, HIWORD(wParam));
            return 0;
        }
    }

    // TODO: Maybe release version should be used as this code is never reached when msg is handled
#ifdef J3D_DEBUG
    if ( JonesMain_bWndMsgProcessed )
    {
        *pReturnVal = 0;
    }

    return JonesMain_bWndMsgProcessed;
#else
    return 0
    #endif
}

void J3DAPI JonesMain_HandleWMGetMinMaxInfo(HWND hwnd, LPMINMAXINFO pMinMaxInfo)
{
    RECT rectWnd;
    GetWindowRect(hwnd, &rectWnd);

    int frameWidth  = 2 * GetSystemMetrics(SM_CXFRAME);
    int frameSize   = GetSystemMetrics(SM_CXFRAME);
    int frameHeight = GetSystemMetrics(SM_CYMENU) + 2 * frameSize;

    uint32_t width, height;
    stdDisplay_GetBackBufferSize(&width, &height);
    if ( width && height )
    {
        rectWnd.left   = 0;
        rectWnd.top    = 0;
        rectWnd.right  = frameWidth + width;
        rectWnd.bottom = frameHeight + height;
    }

    pMinMaxInfo->ptMaxSize.x = rectWnd.right - rectWnd.left;
    pMinMaxInfo->ptMaxSize.y = rectWnd.bottom - rectWnd.top;

    pMinMaxInfo->ptMinTrackSize.x = rectWnd.right - rectWnd.left;
    pMinMaxInfo->ptMinTrackSize.y = rectWnd.bottom - rectWnd.top;
    pMinMaxInfo->ptMaxTrackSize.x = rectWnd.right - rectWnd.left;
    pMinMaxInfo->ptMaxTrackSize.y = rectWnd.bottom - rectWnd.top;

    JonesMain_bWndMsgProcessed = true;
}

void J3DAPI JonesMain_HandleWMPaint(HWND hWnd)
{
    if ( JonesMain_pfProcess == JonesMain_ProcessWindowEvents )
    {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hWnd, (LPPAINTSTRUCT)&ps);
        if ( hdc )
        {
            POINT pt = { .x=0, .y=0 };
            ClientToScreen(hWnd, &pt);
            JonesDialog_RestoreBackground(hdc, hWnd, &pt, &ps.rcPaint);
            EndPaint(hWnd, &ps);
        }
    }
}

int J3DAPI JonesMain_HandleWMTimer(HWND hWnd, WPARAM timerID)
{
    J3D_UNUSED(timerID);

    // TODO: Check if timerID is same as JONES_QUICKSAVE_TEXTSHOWTIMERID
    JonesMain_bPrintQuickSave = false;
    return KillTimer(hWnd, JONES_QUICKSAVE_TEXTSHOWTIMERID);
}

void J3DAPI JonesMain_HandleWMKeydown(HWND hWnd, WPARAM vk, int a3, uint16_t repreatCount, uint16_t exkeyflags)
{
    J3D_UNUSED(a3);

    switch ( vk )
    {
        case VK_F5:// Quick save
        {
            if ( repreatCount <= 1 && (exkeyflags & KF_REPEAT) == 0 )
            {
                if ( !jonesCog_g_bEnableGamesave || jonesCog_g_bMenuVisible || sithWorld_g_bLoading )
                {
                    return;
                }

                char aFilename[128];
                const char* pQSPrefix = sithGetQuickSaveFilePrefix();
                STD_FORMAT(aFilename, "%s.%s", pQSPrefix, "nds");

                char aPath[128];
                const char* pSaveGamesDir = sithGetSaveGamesDir();
                STD_MAKEPATH(aPath, pSaveGamesDir, aFilename);

                JonesHud_RestoreTreasuresStatistics();

                // Note, JonesMain_ProcessGamesaveState shows error message box in case of game save error
                if ( !sithGamesave_Save(aPath, 1) )
                {
                    // Success

                    //tSoundHandle hSnd = Sound_GetSoundHandle(0x804F);
                    tSoundHandle hSnd = Sound_GetSoundHandle(SITHWORLD_STATICINDEX(79));  // 79 - inv_quicksave.wav
                    sithSoundMixer_PlaySound(hSnd, 1.0f, 0.0f, (SoundPlayFlag)0);

                    // Print quicksave text to screen and set text hide timer
                    JonesMain_bPrintQuickSave = true;
                    JonesMain_PrintQuickSave();
                    SetTimer(hWnd, JONES_QUICKSAVE_TEXTSHOWTIMERID, JONES_QUICKSAVE_TEXTSHOWTIME, NULL); // 1 sec
                }
            }
        } break;
        case VK_F8: // Quick load
        {
            if ( repreatCount <= 1 && (exkeyflags & KF_REPEAT) == 0 )
            {
                if ( !jonesCog_g_bEnableGamesave || jonesCog_g_bMenuVisible || sithWorld_g_bLoading )
                {
                    return;
                }

                jonesCog_g_bEnableGamesave = 0;

                char aFilename[128];
                const char* pQSPrefix = sithGetQuickSaveFilePrefix();
                STD_FORMAT(aFilename, "%s.%s", pQSPrefix, "nds");

                char aPath[128];
                const char* pSaveGamesDir = sithGetSaveGamesDir();
                STD_MAKEPATH(aPath, pSaveGamesDir, aFilename);

                FILE* pFile = fopen(aPath, "r");
                if ( pFile )
                {
                    fclose(pFile); // Looks like the file exists, so we can close it here

                    if ( sithGamesave_LoadLevelFilename(aPath, NULL) != 0 )
                    {
                        // Error loading, try load last save file
                        STD_STRCPY(aPath, sithGamesave_GetLastFilename());

                        if ( sithGamesave_LoadLevelFilename(aPath, NULL) != 0 )
                        {
                            const char* pErrorFormat = jonesString_GetString("JONES_STR_LOADERROR");
                            if ( pErrorFormat )
                            {
                                char aErrorText[128] = { 0 };
                                STD_FORMAT(aErrorText, pErrorFormat, aPath);
                                JonesMain_LogErrorToFile(aErrorText);
                            }
                            return;
                        }
                    }

                    if ( sithCamera_g_pCurCamera )
                    {
                        sithCamera_ResetAllCameras();
                    }

                    if ( sithGamesave_Restore(aPath, /*bNotifyCog*/1) )
                    {
                        // Error restoring
                        const char* pErrorFormat = jonesString_GetString("JONES_STR_LOADERROR");
                        if ( pErrorFormat )
                        {
                            char aErrorText[128] = { 0 };
                            STD_FORMAT(aErrorText, pErrorFormat, aPath);
                            JonesMain_LogErrorToFile(aErrorText);
                        }

                        JonesMain_CloseWindow();
                        return;
                    }

                    // Success
                    JonesHud_RestoreGameStatistics();
                }
            }
        } break;
        case VK_F12: // screenshot
        {
            if ( repreatCount <= 1 && (exkeyflags & KF_REPEAT) == 0 )
            {
                sithMakeDirs();
                sithRender_MakeScreenShot();
            }
        } break;
    }
}

void JonesMain_PrintQuickSave(void)
{
    const rdFont* pFont = sithVoice_GetTextFont();
    if ( JonesMain_bPrintQuickSave && pFont )
    {
        const char* pText = jonesString_GetString("JONES_STR_QUICKSAVE");
        if ( pText )
        {
            float posY = 0.5f - rdFont_GetNormY((float)pFont->fontSize);

            rdFontColor fontColor;
            rdVector_Set4(&fontColor[0], 1.0f, 0.0f, 0.0f, 1.0f);
            rdVector_Set4(&fontColor[1], 1.0f, 0.0f, 0.0f, 1.0f);
            rdVector_Set4(&fontColor[2], 1.0f, 1.0f, 0.0f, 1.0f);
            rdVector_Set4(&fontColor[3], 1.0f, 1.0f, 0.0f, 1.0f);

            rdFont_SetFontColor(fontColor);
            rdFont_DrawTextLine(pText, 0.5f, posY, rdCamera_g_pCurCamera->pFrustum->nearPlane, pFont, 1);
        }
    }
}

int J3DAPI JonesMain_HandleWMActivateApp(HWND hWnd, WPARAM wParam, LPARAM lParam)
{
    // TODO: If JonesMain_bWndMsgProcessed will be used by main wnd proc than it should be set to true here
    JonesMain_OnAppActivate(hWnd, wParam);
    return DefWindowProc(hWnd, WM_ACTIVATEAPP, wParam, lParam);
}

LRESULT J3DAPI JonesMain_HandleWMActivate(HWND hWnd, uint16_t activateState, LPARAM hwndActivate, uint16_t minimizedState)
{
    // TODO: If JonesMain_bWndMsgProcessed will be used by main wnd proc than it should be set to true here
    JonesMain_OnAppActivate(hWnd, activateState != WA_INACTIVE);
    return DefWindowProc(hWnd, WM_ACTIVATE, (minimizedState << 16) | activateState, hwndActivate);
}

void J3DAPI JonesMain_OnAppActivate(HWND hWnd, int bActivated)
{
    if ( !JonesMain_pfCurProcess )
    {
        JonesMain_pfCurProcess = JonesMain_pfProcess;
    }

    if ( bActivated )
    {
        if ( !JonesMain_bAppActivated )
        {
            if ( !JonesMain_state.displaySettings.bWindowMode )
            {
                ShowWindow(hWnd, SW_SHOW);
            }

            JonesDisplay_UpdateDualScreenWindowSize(&JonesMain_state.displaySettings);
            stdDisplay_Refresh(1);
            std3D_ResetTextureCache();

            if ( sithWorld_g_pCurrentWorld )
            {
                sithWorld_g_pCurrentWorld->state |= SITH_WORLD_STATE_UPDATE_FOG;
            }

            JonesDisplay_UpdateDualScreenWindowSize(&JonesMain_state.displaySettings);
        }

        if ( JonesMain_pfCurProcess )
        {
            JonesMain_pfProcess = JonesMain_pfCurProcess;
            JonesMain_pfCurProcess = false;
        }

        JonesMain_bAppActivated = true;
    }
    else
    {
        JonesMain_pfProcess = JonesMain_ProcessWindowEvents;
        stdDisplay_Refresh(0);
        JonesMain_bAppActivated = false;
    }

    if ( !stdControl_SetActivation(bActivated) )
    {
        if ( bActivated )
        {
            stdControl_ShowMouseCursor(0);
        }
        else
        {
            stdControl_ShowMouseCursor(1);
        }
    }
}

void J3DAPI JonesMain_HandleWMChar(HWND hwnd, char chr, uint16_t repeatCount)
{
    J3D_UNUSED(hwnd);
    J3D_UNUSED(repeatCount);

    if ( JonesConsole_g_bVisible )
    {
        JonesConsole_HandelChar(chr);
    }
}

void J3DAPI JonesMain_HandleWMSysCommand(HWND hWnd, WPARAM wParam, uint16_t curPosX, uint16_t curPosY)
{
    J3D_UNUSED(hWnd);
    J3D_UNUSED(curPosX);
    J3D_UNUSED(curPosY);

    if ( wParam == SC_KEYMENU || wParam == SC_SCREENSAVE )
    {
        JonesMain_bWndMsgProcessed = true;
    }
}

void J3DAPI JonesMain_HandleWMPowerBroadcast(HWND hWnd, WPARAM wParam)
{
    switch ( wParam )
    {
        case PBT_APMQUERYSUSPEND:
            STDLOG_DEBUG("WM_POWERBROADCAST Query Suspend.\n");
            break;

        case PBT_APMSUSPEND:
            JonesMain_bSystemSuspended = true;
            JonesMain_bSkipIntro       = true;
            JonesMain_OnAppActivate(hWnd, 0);
            STDLOG_DEBUG("WM_POWERBROADCAST Suspend.\n");
            break;

        case PBT_APMRESUMECRITICAL:
        case PBT_APMRESUMESUSPEND:
            JonesMain_bSystemSuspended = false;
            JonesMain_bSkipIntro       = true;
            JonesMain_OnAppActivate(hWnd, 1);
            Sleep(100u);

            STDLOG_DEBUG("WM_POWERBROADCAST Resume.\n");
            break;
    }
}

void JonesMain_Shutdown(void)
{
    if ( JonesMain_aIntroMovieColorTable )
    {
        stdMemory_Free(JonesMain_aIntroMovieColorTable);
    }

    JonesMain_aIntroMovieColorTable = NULL;

    wkernel_SetWindowProc(NULL);

    if ( JonesMain_pDisplayEnv )
    {
        std3D_FreeDisplayEnvironment(JonesMain_pDisplayEnv);
        JonesMain_pDisplayEnv = NULL;
    }

    JonesMain_Close();
    JonesHud_Close();
    sithCloseStatic();

    jonesCog_Shutdown();
    JonesControl_Shutdown();
    JonesConsole_Close();

    JonesDisplay_Shutdown();
    JonesDisplay_CloseLoadScreen();

    JonesHud_Shutdown();
    JonesConsole_Shutdown();

    sithString_Shutdown();
    jonesString_Shutdown();

    sithShutdown();
    rdShutdown();

    stdEffect_Shutdown();
    stdCircBuf_Free(&JonesMain_circBuf);

    if ( JonesMain_state.outputMode == JONES_OUTPUTMODE_CONSOLE )
    {
        stdConsole_Shutdown();
    }

    else if ( JonesMain_state.outputMode == JONES_OUTPUTMODE_LOGFILE )
    {
        fclose(JonesMain_pLogFile); // TODO: BUG JonesMain_pLogFile could be NULL
    }

    sithSound_Shutdown();
    sithSound_ShutdownSound();

    JonesFile_Close();
    wuRegistry_Shutdown();
    JonesFile_Shutdown();

    stdShutdown();
    sithSound_UninitializeSound();
    sithClearServices();
    rdClearServices();
    stdPlatform_ClearServices(&JonesMain_hs);

    JonesMain_bStartup = false;

    if ( JonesMain_g_mainMutex )
    {
        CloseHandle(JonesMain_g_mainMutex);
    }
}

void JonesMain_PauseGame(void)
{
    if ( !JonesMain_bGamePaused )
    {
        sithTime_Pause();
        Sound_Pause();
        JonesMain_bGamePaused = true;
    }
}

void JonesMain_ResumeGame(void)
{
    if ( JonesMain_bGamePaused )
    {
        sithTime_Resume();
        Sound_Resume();
        JonesMain_bGamePaused = false;
    }
}

int JonesMain_IsGamePaused(void)
{
    return JonesMain_bGamePaused != 0;
}

int JonesMain_Process(void)
{
    return JonesMain_pfProcess();
}

int JonesMain_ProcessWindowEvents(void)
{
    return wkernel_ProcessEvents();
}

int JonesMain_ProcessGame(void)
{
    if ( JonesMain_bSystemSuspended )
    {
        return 0;
    }

    if ( sithMain_g_sith_mode.masterMode == SITH_MODE_OPENED )
    {
        JonesMain_ProcessMenu();
        sithUpdate();

        if ( JonesMain_ProcessGamesaveState() )
        {
            return -1;
        }

        // Note functions from ClearZBuffer to and including std3D_StartScene could be part of rdAdvanceFrame
        std3D_ClearZBuffer();
        rdCache_AdvanceFrame();

        // TODO: Refactor and enable viewport clearing in std3D instead
        if ( JonesMain_state.displaySettings.bClearBackBuffer )
        {
            stdDisplay_BackBufferFill(0, 0);
        }

        if ( !std3D_StartScene() )
        {
            sithDrawScene();
            sithOverlayMap_Draw();
            JonesMain_PrintQuickSave();
            JonesHud_Render();

            // below 3 function calls could be part of rdFinishFrame
            rdCache_Flush();
            rdCache_FlushAlpha();
            std3D_EndScene();
        }

        JonesMain_PrintFramerate();
        stdDisplay_Update();
    }

    if ( JonesMain_bRefreshDisplayDevice )
    {
        if ( !JonesDisplay_Restart(&JonesMain_state.displaySettings) )
        {
            // Success
            JonesMain_bRefreshDisplayDevice = false;
            return wkernel_PeekProcessEvents();
        }

        JonesDisplay_SetDefaultVideoMode(JonesMain_pDisplayEnv, &JonesMain_state.displaySettings);
        if ( !JonesDisplay_Restart(&JonesMain_state.displaySettings) )
        {
            // Success with default video mode
            JonesMain_bVideoModeError       = true;
            JonesMain_bRefreshDisplayDevice = false;
            return wkernel_PeekProcessEvents();
        }

        // All attempt to set video mode failed, exit game
        const char* pFormat = jonesString_GetString("JONES_STR_VIDEOERROR");
        if ( pFormat )
        {
            JonesMain_LogErrorToFile(pFormat);
        }

        JonesMain_CloseWindow(); // exits game
        return -1;
    }

    if ( JonesMain_bVideoModeError )
    {
        // Shows message box that there was an error setting video mode in previous frame and
        // was set to default video mode (640 x 480)

        char aNoDisplyError[512] = { 0 };
        const char* pErrorStr = jonesString_GetString("JONES_STR_NODISPLAY");
        if ( pErrorStr )
        {
            STD_STRCPY(aNoDisplyError, pErrorStr);
        }

        char aNoDisply2Error[512] = { 0 };
        pErrorStr = jonesString_GetString("JONES_STR_NODISPLAY2");
        if ( pErrorStr )
        {
            STD_STRCPY(aNoDisply2Error, pErrorStr);
        }

        if ( aNoDisplyError[0] && aNoDisply2Error[0] )
        {
            char aErrorText[128] = { 0 };
            STD_FORMAT(aErrorText, "%s \n %s", aNoDisplyError, aNoDisply2Error);

            HWND hWnd = stdWin95_GetWindow();
            jonesConfig_ShowMessageDialog(hWnd, "JONES_STR_DPLY_OPTS", aErrorText, 136);
        }

        JonesMain_bVideoModeError = false;
    }

    return wkernel_PeekProcessEvents();
}

void JonesMain_ProcessMenu(void)
{
    int bMenuKeyPressed = 0;
    for ( size_t i = 0; i < STD_ARRAYLEN(JonesMain_aToggleMenuKeyIds) && JonesMain_aToggleMenuKeyIds[i]; ++i ) // Fixed: Added bounds check for i < STD_ARRAYLEN(JonesMain_aToggleMenuKeyIds)
    {
        int numPressed;
        if ( stdControl_ReadKey(JonesMain_aToggleMenuKeyIds[i], &numPressed) )
        {
            bMenuKeyPressed = 1;
            break;
        }
    }

    int numPressed;
    bMenuKeyPressed |= stdControl_ReadKey(DIK_ESCAPE, &numPressed);
    if ( bMenuKeyPressed && !JonesMain_bMenuToggled && !stdControl_ReadKey(DIK_RCONTROL, &numPressed) && !stdControl_ReadKey(DIK_LCONTROL, &numPressed) )
    {
        if ( JonesConsole_g_bVisible )
        {
            JonesConsole_HideConsole();
        }
        else
        {
            JonesHud_ToggleMenu();
            JonesMain_bMenuToggled = true;
        }
    }
    else if ( !bMenuKeyPressed )
    {
        JonesMain_bMenuToggled = false;
    }
}

void JonesMain_PrintFramerate(void)
{
    ++JonesMain_frameCount;
    JonesMain_frameTime = stdPlatform_GetTimeMsec();

    if ( JonesMain_bPrintFramerate )
    {
        if ( JonesMain_frameTime - JonesMain_prevFrameTime > JONES_FPSPRINT_INTERVAL ) // 2 secs
        {
            JonesMain_frameRate      = (float)(JonesMain_frameCount - JonesMain_prevFrameCount) * 1000.0f / (float)(JonesMain_frameTime - JonesMain_prevFrameTime);
            JonesMain_prevFrameTime  = JonesMain_frameTime;
            JonesMain_prevFrameCount = JonesMain_frameCount;

            STD_FORMAT(
                std_g_genBuffer,
                "%02.2fHz A:%d S:%d P:%d Z:%d", //TODO: 'Z' in format is probably typo and instead 'T' letter should be there
                JonesMain_frameRate,
                sithRender_g_numVisibleAdjoins,
                sithRender_g_numVisibleSectors,
                sithRender_g_numAlphaThingPoly + sithRender_g_numThingPolys + sithRender_g_numAlphaArchPolys + sithRender_g_numArchPolys,
                sithRender_g_numDrawnThings
            );

            JonesConsole_PrintTextWithID(JONES_FPSPRINT_CONSOLEID, std_g_genBuffer);
            STDLOG_STATUS("%s\n", std_g_genBuffer);
        }
    }
}

J3DNORETURN void JonesMain_FatalErrorUnknown(void)
{
    MessageBox(NULL, "Unknown error", "Unable to continue Error", MB_TASKMODAL | MB_ICONSTOP);
    JonesMain_Shutdown();
    exit(1);
}

void JonesMain_TogglePrintFramerate(void)
{
    JonesMain_bPrintFramerate = JonesMain_bPrintFramerate == 0;
}

// Note: Found in debug version
//int J3DAPI JonesMain_sub_422443(SithMessage* pMsg)
//{
//    STD_ASSERTREL(pMsg);
//    if ( (sithMain_g_sith_mode.subModeFlags & 8) != 0 )
//    {
//        JonesMain_pMessageData = *(uint8_t**)pMsg->data;
//    }
//
//    return 1;
//}

int JonesMain_Open(void)
{
    if ( JonesMain_EnsureFile(JonesMain_state.aCurLevelFilename) )
    {
        return 1;
    }

    // Open load screen
    // Note that internal state of loadscreen should be here > 1, meaning the static world has been loaded
    // so the progress bar starts at 50%
    if ( JonesMain_state.startMode == JONES_STARTMODE_LOADGAME )
    {
        JonesMain_Restore(JonesMain_pNdsFileName);
    }
    else
    {
        JonesMain_UpdateLevelNum();
        JonesDisplay_OpenLoadScreen(
            JonesMain_aCndLevelLoadInfos[JonesMain_curLevelNum].pMatFilename,
            JonesMain_aCndLevelLoadInfos[JonesMain_curLevelNum].wallineStart.x,
            JonesMain_aCndLevelLoadInfos[JonesMain_curLevelNum].wallineStart.y,
            JonesMain_aCndLevelLoadInfos[JonesMain_curLevelNum].wallineEnd.x,
            JonesMain_aCndLevelLoadInfos[JonesMain_curLevelNum].wallineEnd.y,
            JonesMain_aCndLevelLoadInfos[JonesMain_curLevelNum].bPrimaryMusicTheme
        );
    }

    JonesDisplay_UpdateDualScreenWindowSize(&JonesMain_state.displaySettings);

    // Load savegame
    if ( strlen(JonesMain_pNdsFileName) )
    {
        int v5 = strncmp("start", JonesMain_pNdsFileName, 5u) != 0; // TODO: ??
        J3D_UNUSED(v5);

        if ( sithGamesave_Restore(JonesMain_pNdsFileName, 0) )
        {
            // Error
            const  char* pFormat = jonesString_GetString("JONES_STR_LOADERROR");
            if ( pFormat )
            {
                char aErrorText[128] = { 0 };
                STD_FORMAT(aErrorText, pFormat, JonesMain_pNdsFileName);
                JonesMain_LogErrorToFile(aErrorText);
            }

            JonesMain_CloseWindow(); // Note this will exit process
            // TODO: should we still return error here?
        }
        else
        {
            JonesHud_RestoreGameStatistics();
        }

        if ( JonesDisplay_Open(&JonesMain_state.displaySettings) )
        {
            return 1;
        }

        if ( JonesMain_state.startMode == JONES_STARTMODE_LOADGAME )
        {
            sithGamesave_NotifyRestored(JonesMain_pNdsFileName);
        }

        memset(JonesMain_pNdsFileName, 0, sizeof(JonesMain_pNdsFileName));
    }
    else
    {
        // Load level & open level
        if ( sithOpenNormal(JonesMain_state.aCurLevelFilename, JonesMain_state.waPlayerName) )
        {
            // Error opening level
            const  char* pFormat = jonesString_GetString("JONES_STR_OPENERROR");
            if ( pFormat )
            {
                char aErrorText[128] = { 0 };
                STD_FORMAT(aErrorText, pFormat, JonesMain_aCndLevelLoadInfos[JonesMain_curLevelNum].pName);
                JonesMain_LogErrorToFile(aErrorText);
            }

            JonesMain_CloseWindow(); // Note this will exit process
            // TODO: should we still return error here?
        }

        if ( JonesDisplay_Open(&JonesMain_state.displaySettings) )
        {
            return 1;
        }

        sithOpenPostProcess();
    }

    JonesDisplay_CloseLoadScreen();
    return 0;
}

int J3DAPI JonesMain_EnsureFile(const char* pFilename)
{
    // TODO: there is an issue when loading level from different CD*.gob file than current
    //       the insert CD dialog can be shown even if file exists on disk

    char aPath[128];
    STD_MAKEPATH(aPath, "ndy", pFilename);
    if ( JonesFile_FileExists(aPath) )
    {
        // File found
        return 0;
    }

    JonesFile_Close();

    size_t cndNum = 2 - (JonesFile_GetCurrentCDNum() != 1);
    if ( jonesConfig_ShowDialogInsertCD(stdWin95_GetWindow(), cndNum) == 1 )
    {
        return JonesFile_Open(&JonesMain_hs, JonesMain_state.aInstallPath, JonesMain_state.aCDPath);
    }

    return 1; // exit
}

int JonesMain_Close(void)
{
    JonesDisplay_Close();
    sithClose();
    return 0;
}

int J3DAPI JonesMain_Restore(const char* pNdsFilePath)
{
    if ( !pNdsFilePath )
    {
        return 1;
    }

    if ( sithGamesave_LoadLevelFilename(pNdsFilePath, JonesMain_state.aCurLevelFilename) )
    {
        return 1;
    }

    if ( JonesMain_EnsureFile(JonesMain_state.aCurLevelFilename) )
    {
        return 1;
    }

    const char* pNdsFilename = strrchr(pNdsFilePath, '\\');
    if ( pNdsFilename )
    {
        ++pNdsFilename;
    }
    else
    {
        pNdsFilename = pNdsFilePath;
    }

    JonesMain_UpdateLevelNum();

    const char* pSaveName    = sithGetCurrentWorldSaveName();
    const char* pFilePrefix  = sithGetAutoSaveFilePrefix();
    char aQSaveFilename[128] = { 0 };
    STD_FORMAT(aQSaveFilename, "%s%s", pFilePrefix, pSaveName);

    stdFnames_ChangeExt(aQSaveFilename, "nds");

    // Open load screen
    if ( JonesMain_curLevelNum != 1 || strcmp(pNdsFilename, aQSaveFilename) == 0 )
    {
        JonesDisplay_OpenLoadScreen(
            JonesMain_aCndLevelLoadInfos[JonesMain_curLevelNum].pMatFilename,
            JonesMain_aCndLevelLoadInfos[JonesMain_curLevelNum].wallineStart.x,
            JonesMain_aCndLevelLoadInfos[JonesMain_curLevelNum].wallineStart.y,
            JonesMain_aCndLevelLoadInfos[JonesMain_curLevelNum].wallineEnd.x,
            JonesMain_aCndLevelLoadInfos[JonesMain_curLevelNum].wallineEnd.y,
            JonesMain_aCndLevelLoadInfos[JonesMain_curLevelNum].bPrimaryMusicTheme
        );
    }
    else
    {
        // Note, using cyb_amap instead of cyn_amap
        JonesDisplay_OpenLoadScreen("mat\\cyb_amap", -5.0f, 187.0f, 315.0f, 297.0f, JonesMain_aCndLevelLoadInfos[1].bPrimaryMusicTheme);
    }

    return 0;
}

int JonesMain_ProcessGamesaveState(void)
{
    // TODO: this part should be moved in the load game scope
    char aLoadLerrorStr[512] = { 0 };
    const char* pLoadErrStr = jonesString_GetString("JONES_STR_LOADERROR");
    if ( pLoadErrStr )
    {
        STD_STRCPY(aLoadLerrorStr, pLoadErrStr);
    }

    char* pNdsFilename;
    JonesMain_curGamesaveState = sithGamesave_GetState(&pNdsFilename);

    switch ( JonesMain_curGamesaveState )
    {
        case SITHGAMESAVE_SAVE:
        {
            if ( sithGamesave_Process() )
            {
                // Error
                const char* pFormat = jonesString_GetString("JONES_STR_SAVEERROR");
                if ( !pFormat )
                {
                    return 1;
                }

                char aErrorText[128] = { 0 };
                STD_FORMAT(aErrorText, pFormat, pNdsFilename);
                JonesMain_LogErrorToFile(aErrorText);
                return 1;
            }

            // Success
            return 0;
        }

        case SITHGAMESAVE_RESTORE:
        {
            if ( JonesMain_Restore(pNdsFilename) || sithGamesave_Process() )
            {
                // Error
                if ( aLoadLerrorStr[0] )
                {
                    char aErrorText[128] = { 0 };
                    STD_FORMAT(aErrorText, aLoadLerrorStr, pNdsFilename);
                    JonesMain_LogErrorToFile(aErrorText);
                }

                JonesMain_CloseWindow();
                STDLOG_ERROR("Error in Restore: %s\n", pNdsFilename);
                return 1;
            }

            // Success
            JonesDisplay_CloseLoadScreen();
            return 0;
        }
    };

    return 0;
}

void JonesMain_UpdateLevelNum(void)
{
    JonesMain_curLevelNum = 0;
    for ( size_t i = 1; i < STD_ARRAYLEN(JonesMain_aCndLevelLoadInfos); ++i )
    {
        if ( strcmpi(JonesMain_state.aCurLevelFilename, JonesMain_aCndLevelLoadInfos[i].pFilename) == 0 )
        {
            JonesMain_curLevelNum = i;
            break;
        }
    }

    if ( JonesMain_curLevelNum == 0 )
    {
        for ( size_t i = 1; i < STD_ARRAYLEN(JonesMain_aCndLevelLoadInfos); ++i )
        {
            if ( strcmpi(JonesMain_state.aCurLevelFilename, JonesMain_aNdyLevelLoadInfos[i].pFilename) == 0 )
            {
                JonesMain_curLevelNum = i;
                return;
            }
        }
    }
}

void JonesMain_NextLevel(void)
{
    if ( strcmpi(sithWorld_g_pCurrentWorld->aName, "17_PRU.cnd") )
    {
        STD_STRCPY(JonesMain_state.aCurLevelFilename, sithWorld_g_pCurrentWorld->aName);
    }
    else
    {
        const char* pPreviousLevelName = sithGamesave_GetPreviousLevelFilename();
        STD_STRCPY(JonesMain_state.aCurLevelFilename, pPreviousLevelName);
    }

    JonesMain_state.startMode = JONES_STARTMODE_STARTGAME;

    // Advance level
    JonesMain_UpdateLevelNum();
    STD_STRCPY(JonesMain_state.aCurLevelFilename, JonesMain_aCndLevelLoadInfos[++JonesMain_curLevelNum].pFilename); // Note, the JonesMain_curLevelNum gets incremented here to get the next level

    // Set process
    JonesMain_pfProcess = JonesMain_ProcessStartGame;
}

int JonesMain_ProcessStartGame(void)
{
    if ( JonesMain_Close() )
    {
        const char* pErrorStr = jonesString_GetString("JONES_STR_CLOSEERROR");
        if ( pErrorStr )
        {
            char aErrorText[128] = { 0 };
            STD_FORMAT(aErrorText, pErrorStr, JonesMain_aCndLevelLoadInfos[JonesMain_curLevelNum].pName);
            JonesMain_LogErrorToFile(aErrorText);
        }

        JonesMain_CloseWindow();
    }

    if ( JonesMain_Open() )
    {
        const char* pErrorStr = jonesString_GetString("JONES_STR_OPENERROR");
        if ( pErrorStr )
        {
            char aErrorText[128] = { 0 };
            STD_FORMAT(aErrorText, pErrorStr, JonesMain_aCndLevelLoadInfos[JonesMain_curLevelNum].pName);
            JonesMain_LogErrorToFile(aErrorText);
        }

        JonesMain_CloseWindow();
    }

    JonesMain_pfProcess = JonesMain_ProcessGame;
    return 0;
}

void JonesMain_SetBonusLevel(void)
{
    JonesMain_state.startMode = JONES_STARTMODE_STARTGAME;

    sithGamesave_SetPreviousLevelFilename(JonesMain_state.aCurLevelFilename);
    STD_STRCPY(JonesMain_state.aCurLevelFilename, "17_PRU.cnd");

    JonesMain_pfProcess = JonesMain_ProcessStartGame;
}

void J3DAPI JonesMain_JumpLevel(size_t levelNum)
{
    if ( levelNum >= STD_ARRAYLEN(JonesMain_aCndLevelLoadInfos) || levelNum < 1 )
    {
        sithConsole_PrintString("Invalid Level number!");
        return;
    }

    JonesMain_state.startMode = JONES_STARTMODE_STARTGAME;
    STD_STRCPY(JonesMain_state.aCurLevelFilename, JonesMain_aCndLevelLoadInfos[levelNum].pFilename);

    JonesMain_pfProcess = JonesMain_ProcessStartGame;
}

void JonesMain_RestartGame(void)
{
    JonesMain_curLevelNum = 1;
    STD_STRCPY(JonesMain_state.aCurLevelFilename, JonesMain_aCndLevelLoadInfos[1].pFilename);

    JonesMain_pfProcess = JonesMain_ProcessStartGame;
}

void JonesMain_RestartLevel(void)
{
    JonesMain_pfProcess = JonesMain_ProcessStartGame;
}

void JonesMain_ShowEndCredits(void)
{
    Sound_StopAllSounds(); // Added: Stop all sounds before showing end credits. Note, don't call sithSoundMixer_StopAll(), as it clears cur camera sec, which is re-assigned when sithSoundMixer_Update() is called
    JonesMain_pfProcess = JonesMain_ProcessCredits;
}

int JonesMain_ProcessCredits(void)
{
    bool bFinished = false; // Added: Init. to false

    if ( !JonesMain_bEndCredits )
    {
        JonesMain_hSndCredits = Sound_GetSoundHandle(SITHWORLD_STATICINDEX(2)); // mus_enddemo.wav
        if ( JonesMain_hSndCredits && !JonesMain_hCreditsMusic )
        {
            JonesMain_hCreditsMusic = sithSoundMixer_PlaySound(JonesMain_hSndCredits, 1.0, 0.0, SOUNDPLAY_PLAYONCE | SOUNDPLAY_LOOP);
        }

        stdControl_ReadControls();
        int numEscPressed;
        if ( !JonesMain_bEndCredits && stdControl_ReadKey(DIK_ESCAPE, &numEscPressed) )
        {
            JonesMain_bEndCredits = true;
        }
    }

    std3D_ClearZBuffer();
    rdCache_AdvanceFrame();
    sithSoundMixer_Update(); // Fixed: Added missing call to sithSoundMixer_Update which updates sound fadeout at the end of credits

    // TODO: Refactor and enable viewport clearing in std3D instead
    if ( JonesMain_state.displaySettings.bClearBackBuffer )
    {
        stdDisplay_BackBufferFill(0, NULL);
    }

    if ( !std3D_StartScene() )
    {
        sithDrawScene();
        if ( JonesMain_bEndCredits )
        {
            bFinished = JonesHud_DrawCredits(1, JonesMain_hCreditsMusic);
        }
        else
        {
            bFinished = JonesHud_DrawCredits(0, JonesMain_hCreditsMusic);
        }

        rdCache_Flush();
        rdCache_FlushAlpha();
        std3D_EndScene();
        stdDisplay_Update();
    }

    if ( !bFinished )
    {
        return wkernel_PeekProcessEvents();
    }

    if ( JonesMain_hCreditsMusic )
    {
        sithSoundMixer_StopSound(JonesMain_hCreditsMusic);
    }

    JonesMain_pfProcess     = JonesMain_ProcessGame;
    JonesMain_hSndCredits   = 0;
    JonesMain_hCreditsMusic = 0;
    JonesMain_bEndCredits   = false;
    JonesHud_ShowGameOverDialog(0);
    return wkernel_PeekProcessEvents();
}

int JonesMain_HasStarted(void)
{
    return JonesMain_bStartup;
}

StdDisplayEnvironment* JonesMain_GetDisplayEnvironment(void)
{
    return JonesMain_pDisplayEnv;
}

JonesDisplaySettings* JonesMain_GetDisplaySettings(void)
{
    return &JonesMain_state.displaySettings;
}

int JonesMain_CloseWindow(void)
{
    return PostMessage(stdWin95_GetWindow(), WM_CLOSE, 0, 0);
}

int JonesMain_FilePrintf(const char* pFormat, ...)
{
    // TODO: Check if file is open? Tho, the startup should take care of that
    va_list args;
    va_start(args, pFormat);
    vsnprintf_s(std_g_genBuffer, STD_ARRAYLEN(std_g_genBuffer), STD_ARRAYLEN(std_g_genBuffer) - 1, pFormat, args);
    va_end(args); // Fixed: Add missing call to va_end

    fprintf(JonesMain_pLogFile, std_g_genBuffer);
    fflush(JonesMain_pLogFile);
    return STD_ARRAYLEN(std_g_genBuffer);
}

int JonesMain_Log(const char* pFormat, ...)
{
    va_list args;
    va_start(args, pFormat);
    vsnprintf_s(std_g_genBuffer, STD_ARRAYLEN(std_g_genBuffer), STD_ARRAYLEN(std_g_genBuffer) - 1, pFormat, args);
    va_end(args); // Fixed: Add missing call to va_end

    // TODO: What's the point of using circbuf??
    char* pString = (char*)stdCircBuf_GetNextElement(&JonesMain_circBuf);
    if ( pString )
    {
        stdUtil_StringCopy(pString, 128, std_g_genBuffer);
    }

    if ( JonesMain_state.outputMode == JONES_OUTPUTMODE_CONSOLE )
    {
        stdConsole_WriteConsole(std_g_genBuffer, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
    }

    else if ( JonesMain_state.outputMode == JONES_OUTPUTMODE_LOGFILE )
    {
        JonesMain_FilePrintf(std_g_genBuffer);
    }

    return strlen(std_g_genBuffer);
}

int JonesMain_NoLog(const char* pFormat, ...)
{
    J3D_UNUSED(pFormat);
    return 0;
}

void JonesMain_RefreshDisplayDevice(void)
{
    JonesMain_bRefreshDisplayDevice = true;
}

int JonesMain_PlayIntroMovie(void)
{
    wkernel_SetWindowProc(JonesMain_IntroWndProc);
    JonesMain_bNoProcessWndEvents = 0;

    int result = wkernel_PeekProcessEvents();
    if ( result )
    {
        return result;
    }

    LPDIRECTSOUND pDSound = SoundDriver_GetDSound();
    HWND hwnd = stdWin95_GetWindow();
    SmushPlay_SysStartup(hwnd, pDSound);
    SmushPlay_SetGlobalVolume((size_t)(JonesMain_state.soundVolume * 127.0f));

    StdVideoMode videoMode;
    if ( stdDisplay_GetCurrentVideoMode(&videoMode) )
    {
        return result;
    }

    // Set video filename base on screen resolution
    const char aJonesopn_640x480[16] = "jonesopn.snm";
    const char aJonesopn_320x240[16] = "jonesopn_3.snm";
    const char aJonesopn_400x300[16] = "jonesopn_4.snm";
    const char aJonesopn_512x384[16] = "jonesopn_5.snm";
    const char aJonesopn_800x600[16] = "jonesopn_8.snm";

    char aFilename[256] = { 0 };
    if ( videoMode.rasterInfo.width == 512 )
    {
        STD_STRCPY(aFilename, aJonesopn_512x384);
    }
    else if ( videoMode.rasterInfo.width != 640 )
    {
        STD_STRCPY(aFilename, aJonesopn_640x480); // TODO: use aJonesopn_800x600 by default
        if ( videoMode.rasterInfo.width == 800 )
        {
            if ( JonesMain_state.performanceLevel < 3 )
            {
                STD_STRCPY(aFilename, aJonesopn_400x300);
            }
            else
            {
                STD_STRCPY(aFilename, aJonesopn_800x600);
            }

        }
    }
    else // 640 x 480
    {
        STD_STRCPY(aFilename, aJonesopn_320x240);
        if ( JonesMain_state.performanceLevel >= 3 )
        {
            STD_STRCPY(aFilename, aJonesopn_640x480);
        }
    }

    char aPath[128] = { 0 };
    if ( stdFileUtil_FileExists(aFilename) )
    {
        STD_STRCPY(aPath, aFilename);
    }
    else
    {
        char aResDir[128];
        wuRegistry_GetStr("Source Dir", aPath, STD_ARRAYLEN(aPath), "");
        STD_FORMAT(aResDir, "%sresource", aPath);
        STD_MAKEPATH(aPath, aResDir, aFilename);
    }

    if ( !stdFileUtil_FileExists(aPath) )
    {
        wkernel_SetWindowProc(JonesMain_GameWndProc);
        return wkernel_PeekProcessEvents();
    }

    Sleep(100u);

    if ( memcmp(&stdColor_cfRGB555, &videoMode.rasterInfo.colorInfo, 44) == 0 )
    {
        JonesMain_introVideoMode = 2;
        JonesMain_aIntroMovieColorTable = (uint8_t*)STDMALLOC(((int)UINT16_MAX + 1) * sizeof(int16_t));
        if ( JonesMain_aIntroMovieColorTable )
        {
            // Init pixel conversion table
            for ( int pixel = 0; pixel < (int)UINT16_MAX + 1; ++pixel )
            {
                *(uint16_t*)&JonesMain_aIntroMovieColorTable[2 * pixel] = stdDisplay_EncodeFromRGB565((uint16_t)pixel);
            }

            // Play intro movie
            SmushPlay_PlayMovie(aPath, 15, 0, 0, 0, 640, 480, -1, 0, JonesMain_IntroMovieBlt555, 1, 1000000, 1000000);

            if ( JonesMain_aIntroMovieColorTable )
            {
                stdMemory_Free(JonesMain_aIntroMovieColorTable);
            }

            JonesMain_aIntroMovieColorTable = NULL;
        }
    }
    else if ( memcmp(&stdColor_cfRGB565, &videoMode.rasterInfo.colorInfo, 44) == 0 )
    {
        // Play intro movie
        JonesMain_introVideoMode = 1;
        SmushPlay_PlayMovie(aPath, 15, 0, 0, 0, 640, 480, -1, 0, JonesMain_IntroMovieBlt565, 1, 1000000, 1000000);
    }
    else if ( memcmp(&stdColor_cfBGR888, &videoMode.rasterInfo.colorInfo, 44) == 0 || memcmp(&stdColor_cfRGB888, &videoMode.rasterInfo.colorInfo, 44) == 0 )
    {
        JonesMain_introVideoMode = 3;
    }
    else if ( memcmp(&stdColor_cfBGR8888, &videoMode.rasterInfo.colorInfo, 44) == 0 || memcmp(&stdColor_cfRGB8888, &videoMode.rasterInfo.colorInfo, 44) == 0 )
    {
        JonesMain_introVideoMode = 4;
        JonesMain_aIntroMovieColorTable = (uint8_t*)STDMALLOC(((int)UINT16_MAX + 1) * sizeof(uint32_t));
        if ( JonesMain_aIntroMovieColorTable )
        {
            // Init pixel conversion table
            for ( int pixel = 0; pixel < (int)UINT16_MAX + 1; ++pixel )
            {
                *(uint32_t*)&JonesMain_aIntroMovieColorTable[4 * pixel] = stdDisplay_EncodeFromRGB565((uint16_t)pixel);
            }

            // Play intro movie
            SmushPlay_PlayMovie(aPath, 15, 0, 0, 0, 640, 480, -1, 0, JonesMain_IntroMovieBlt32, 1, 1000000, 1000000);

            if ( JonesMain_aIntroMovieColorTable )
            {
                stdMemory_Free(JonesMain_aIntroMovieColorTable);
            }

            JonesMain_aIntroMovieColorTable = NULL;
        }
    }
    else
    {
        JonesMain_introVideoMode = 0;
    }

    SmushPlay_SysShutdown();
    JonesMain_bSkipIntro = false;

    // Set window process to game process
    wkernel_SetWindowProc(JonesMain_GameWndProc);

    if ( JonesMain_bNoProcessWndEvents == 1 )
    {
        return 1;
    }

    return wkernel_PeekProcessEvents();
}

int J3DAPI JonesMain_IntroWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, int* pRetValue)
{
    JonesMain_bWndMsgProcessed = false;

    switch ( uMsg )
    {
        case WM_POWERBROADCAST:
        {
            JonesMain_HandleWMPowerBroadcast(hWnd, wParam);
            return lParam;
        }
        case WM_KEYDOWN:
        {
            JonesMain_IntroHandleWMKeydown(hWnd, wParam, 1, LOWORD(lParam), HIWORD(lParam));
            return 0;
        }
        case  WM_LBUTTONUP:
        {
            JonesMain_IntroHandleWMLButtonUp(hWnd, LOWORD(lParam), HIWORD(lParam), wParam);
            return 0;
        }
        case WM_GETMINMAXINFO:
        {
            JonesMain_HandleWMGetMinMaxInfo(hWnd, (LPMINMAXINFO)lParam);
            return 0;
        }
        case  WM_ACTIVATEAPP:
        {
            JonesMain_HandleWMActivateApp(hWnd, wParam, lParam);
            return 0;
        }
        case  WM_ACTIVATE:
        {
            JonesMain_HandleWMActivate(hWnd, LOWORD(wParam), lParam, HIWORD(wParam));
            return 0;
        }
    }

    // TODO: Maybe release version should be used as this code is never reached when msg is handled
#ifdef J3D_DEBUG
    if ( JonesMain_bWndMsgProcessed )
    {
        *pRetValue = 0;
    }

    return JonesMain_bWndMsgProcessed;
#else
    return 0
    #endif
}

void J3DAPI JonesMain_IntroHandleWMLButtonUp(HWND hwnd, uint16_t curPosX, uint16_t curPosY, WPARAM mvk)
{
    J3D_UNUSED(hwnd);
    J3D_UNUSED(curPosX);
    J3D_UNUSED(curPosY);
    J3D_UNUSED(mvk);
    JonesMain_bSkipIntro = true;
}

void J3DAPI JonesMain_IntroHandleWMKeydown(HWND hwnd, WPARAM vk, int a3, uint16_t repreatCount, uint16_t exkeyflags)
{
    J3D_UNUSED(hwnd);
    J3D_UNUSED(a3);
    J3D_UNUSED(repreatCount);
    J3D_UNUSED(exkeyflags);

    switch ( vk )
    {
        case VK_RETURN:
        case VK_ESCAPE:
            JonesMain_bSkipIntro = true;
            break;

        case VK_F4:
            JonesMain_CloseWindow();
            break;

        case VK_F12:
            sithMakeDirs();
            sithRender_MakeScreenShot();
            break;

        default:
            return;
    }
}

int J3DAPI JonesMain_IntroMovieBlt565(const SmushBitmap* pBitmap, int a2)
{
    J3D_UNUSED(a2);

    uint32_t i;
    uint32_t j;
    size_t curHeight;
    size_t curWidth;
    uint8_t* pPixels;
    uint32_t height;
    uint32_t width;
    void* pSurface;
    uint8_t* pCurPixelIn;
    uint16_t* pCurPixelOut;
    uint8_t* pRow;
    int pitch;

    stdDisplay_LockBackBuffer(&pSurface, &width, &height, &pitch);

    pRow = (uint8_t*)pSurface;
    pPixels = (uint8_t*)pBitmap->pPixels;
    curHeight = 0;

    for ( i = 0; i < height; ++i )
    {
        curWidth = 0;
        pCurPixelIn = pPixels;
        pCurPixelOut = (uint16_t*)pRow;

        for ( j = 0; j < width; ++j )
        {
            *pCurPixelOut++ = *(uint16_t*)pCurPixelIn;

            curWidth += pBitmap->width;
            while ( (int)curWidth >= (int)width )
            {
                curWidth -= width;
                pCurPixelIn += pBitmap->pixelSize;
            }
        }

        pRow += pitch;

        curHeight += pBitmap->height;
        while ( (int)curHeight >= (int)height )
        {
            curHeight -= height;
            pPixels += pBitmap->pitch;
        }
    }

    stdDisplay_UnlockBackBuffer();

    stdDisplay_Update();

    if ( !JonesMain_bSkipIntro )
    {
        if ( !wkernel_PeekProcessEvents() )
        {
            return 0;
        }

        JonesMain_bNoProcessWndEvents = 1;
    }

    if ( JonesMain_aIntroMovieColorTable )
    {
        stdMemory_Free(JonesMain_aIntroMovieColorTable);
    }

    JonesMain_aIntroMovieColorTable = 0;
    return 1;
}

int J3DAPI JonesMain_IntroMovieBlt555(const SmushBitmap* pBitmap, int a2)
{
    J3D_UNUSED(a2);

    uint32_t i;
    uint32_t j;
    size_t curHeight;
    size_t curWidth;
    uint8_t* pPixels;
    uint32_t height;
    uint32_t width;
    void* pSurface;
    uint8_t* pCurPixelIn;
    uint16_t* pCurPixelOut;
    uint8_t* pRow;
    int pitch;

    stdDisplay_LockBackBuffer(&pSurface, &width, &height, &pitch);

    pRow = (uint8_t*)pSurface;
    pPixels = (uint8_t*)pBitmap->pPixels;
    curHeight = 0;

    for ( i = 0; i < height; ++i )
    {
        curWidth = 0;
        pCurPixelIn = pPixels;
        pCurPixelOut = (uint16_t*)pRow;

        for ( j = 0; j < width; ++j )
        {
            *pCurPixelOut++ = *(uint16_t*)&JonesMain_aIntroMovieColorTable[2 * *(uint16_t*)pCurPixelIn];

            curWidth += pBitmap->width;
            while ( (int)curWidth >= (int)width )
            {
                curWidth -= width;
                pCurPixelIn += pBitmap->pixelSize;
            }
        }

        pRow += pitch;

        curHeight += pBitmap->height;
        while ( (int)curHeight >= (int)height )
        {
            curHeight -= height;
            pPixels += pBitmap->pitch;
        }
    }

    stdDisplay_UnlockBackBuffer();

    stdDisplay_Update();

    if ( !JonesMain_bSkipIntro )
    {
        if ( !wkernel_PeekProcessEvents() )
        {
            return 0;
        }

        JonesMain_bNoProcessWndEvents = 1;
    }

    if ( JonesMain_aIntroMovieColorTable )
    {
        stdMemory_Free(JonesMain_aIntroMovieColorTable);
    }

    JonesMain_aIntroMovieColorTable = 0;
    return 1;
}

int J3DAPI JonesMain_IntroMovieBlt32(const SmushBitmap* pBitmap, int a2)
{
    J3D_UNUSED(a2);

    uint32_t i;
    uint32_t j;
    size_t curHeight;
    size_t curWidth;
    uint8_t* pPixels;
    uint32_t height;
    uint32_t width;
    void* pSurface;
    uint8_t* pCurPixelIn;
    uint32_t* pCurPixelOut;
    uint8_t* pRow;
    int pitch;

    stdDisplay_LockBackBuffer(&pSurface, &width, &height, &pitch);

    pRow = (uint8_t*)pSurface;
    pPixels = (uint8_t*)pBitmap->pPixels;
    curHeight = 0;

    for ( i = 0; i < height; ++i )
    {
        curWidth = 0;
        pCurPixelIn = pPixels;
        pCurPixelOut = (uint32_t*)pRow;

        for ( j = 0; j < width; ++j )
        {
            *pCurPixelOut++ = *(uint32_t*)&JonesMain_aIntroMovieColorTable[4 * *(uint16_t*)pCurPixelIn];

            curWidth += pBitmap->width;
            while ( (int)curWidth >= (int)width )
            {
                curWidth -= width;
                pCurPixelIn += pBitmap->pixelSize;
            }
        }

        pRow += pitch;

        curHeight += pBitmap->height;
        while ( (int)curHeight >= (int)height )
        {
            curHeight -= height;
            pPixels += pBitmap->pitch;
        }
    }

    stdDisplay_UnlockBackBuffer();

    stdDisplay_Update();

    if ( !JonesMain_bSkipIntro )
    {
        if ( !wkernel_PeekProcessEvents() )
        {
            return 0;
        }

        JonesMain_bNoProcessWndEvents = 1;
    }

    if ( JonesMain_aIntroMovieColorTable )
    {
        stdMemory_Free(JonesMain_aIntroMovieColorTable);
    }

    JonesMain_aIntroMovieColorTable = NULL;
    return 1;
}

J3DNORETURN void J3DAPI JonesMain_Assert(const char* pErrorText, const char* pSrcFile, int line)
{
    if ( JonesMain_bAssertTriggered )
    {
        DebugBreak();
        exit(1);
    }
    JonesMain_bAssertTriggered = true;

    size_t filenamePos   = 0;
    bool bFoundFilename = false;
    for ( size_t i = 0; pSrcFile[i]; ++i )
    {
        if ( pSrcFile[i] == '\\' )
        {
            bFoundFilename = true;
            filenamePos    = i;
        }
    }

    if ( bFoundFilename ) {
        ++filenamePos;
    }

    STD_FORMAT(JonesMain_g_aErrorBuffer, "%s(%d):  %s\n", &pSrcFile[filenamePos], line, pErrorText);
    std_g_pHS->pErrorPrint("ASSERT: %s", JonesMain_g_aErrorBuffer);

    MessageBox(NULL, JonesMain_g_aErrorBuffer, "Assert Handler", MB_TASKMODAL);

    DebugBreak();
    JonesMain_Shutdown();
    exit(1);
}

void J3DAPI JonesMain_BindToggleMenuControlKeys(const int* paKeyIds, int numKeys)
{
    memset(JonesMain_aToggleMenuKeyIds, 0, 8u); // TODO: bug zero all elements
    for ( size_t i = 0; i < numKeys; ++i )
    {
        JonesMain_aToggleMenuKeyIds[i] = paKeyIds[i];
    }
}

void JonesMain_InitializeHUD(void)
{
    JonesHud_EnableMenu(0);
    JonesHud_SetHealthBarAlpha(0.0f);
    jonesCog_g_bShowHealthHUD  = 0;
    jonesCog_g_bEnableGamesave = 1;
}

int J3DAPI JonesMain_SaveHUD(DPID idTo, unsigned int outstream)
{
    SITHDSS_STARTOUT(SITHDSS_HUDSTATE);
    SITHDSS_PUSHUINT8(JonesHud_IsMenuEnabled());
    SITHDSS_PUSHUINT8(jonesCog_g_bShowHealthHUD);
    SITHDSS_PUSHFLOAT(JonesHud_GetHealthBarAlpha());
    SITHDSS_ENDOUT;
    STD_ASSERT(sithMulti_g_message.length == 6);

    //sithMulti_g_message.data[0] = JonesHud_IsMenuEnabled();
    //sithMulti_g_message.data[1] = jonesCog_g_bShowHealthHUD;
    //*(float*)&sithMulti_g_message.data[2] = JonesHud_GetHealthBarAlpha();
    //sithMulti_g_message.type = 47;
    //sithMulti_g_message.length = 6;

    return sithMessage_SendMessage(&sithMulti_g_message, idTo, outstream, 0x01);
}

int J3DAPI JonesMain_RestoreHUD(const SithMessage* pMsg)
{
    STD_ASSERTREL(pMsg);
    SITHDSS_STARTIN(pMsg);

    JonesHud_EnableMenu(SITHDSS_POPUINT8());
    jonesCog_g_bShowHealthHUD = SITHDSS_POPUINT8();
    JonesHud_SetHealthBarAlpha(SITHDSS_POPFLOAT());
    SITHDSS_ENDIN;

    /*  JonesHud_EnableMenu(pMsg->data[0]);
      jonesCog_g_bShowHealthHUD = pMsg->data[1];
      JonesHud_SetHealthBarAlpha(*(float*)&pMsg->data[2]);*/
    return 1;
}

size_t JonesMain_GetCurrentLevelNum(void)
{
    return JonesMain_curLevelNum;
}

void J3DAPI JonesMain_LogErrorToFile(const char* pErrorText)
{
    char aFilePath[128] = { 0 };

    const char* pFilename = jonesString_GetString("JONES_STR_ERRORFILE"); // TODO: pErrorText could be internally cached injonesString which will get overridden here. Best solution would be to use circ buffer by jonesString_GetString 
    if ( pFilename )
    {
        char aInstallPath[128] ={ 0 };
        wuRegistry_GetStr("Install Path", aInstallPath, STD_ARRAYLEN(aInstallPath), "");
        size_t pathLen = strlen(aInstallPath);
        if ( pathLen )
        {
            if ( aInstallPath[pathLen - 1] == '\\' ) // Fixed: before: aFilePath[pathLen + 127] == '\\'
            {
                STD_FORMAT(aFilePath, "%s%s", aInstallPath, pFilename);
            }
            else
            {
                STD_FORMAT(aFilePath, "%s\\%s", aInstallPath, pFilename);
            }
        }
        else
        {
            STD_FORMAT(aFilePath, ".\\\\%s", pFilename);
        }
    }
    else
    {
        STD_FORMAT(aFilePath, ".\\\\%s", "JonesError.txt");
    }

    if ( pErrorText && strlen(pErrorText) )
    {
        FILE* fp = fopen(aFilePath, "wt+");
        if ( fp )
        {
            fprintf(fp, pErrorText);
            fprintf(fp, "\n");
            fclose(fp);
        }
    }
}

void J3DAPI JonesMain_LoadSettings(StdDisplayEnvironment* pDisplayEnv, JonesState* pConfig)
{
    DWORD nSize = STD_ARRAYLEN(pConfig->waPlayerName);
    CHAR aText[128] = { 0 }; // Added: Init to 0
    if ( GetComputerName(aText, &nSize) )
    {
        stdUtil_ToWStringEx(pConfig->waPlayerName, aText, STD_ARRAYLEN(pConfig->waPlayerName) - 1);
    }
    else
    {
        stdUtil_ToWStringEx(pConfig->waPlayerName, "NoName", STD_ARRAYLEN(pConfig->waPlayerName) - 1);
    }

    pConfig->waPlayerName[STD_ARRAYLEN(pConfig->waPlayerName) - 1] = 0;

    // Removed: rdModel3K module not supported 
    //if ( wuRegistry_GetIntEx("Katmai", 1) && rdModel3K_sub_4E2ED0() )
    //{
    //    rdModel3K_sub_4E2F00(1);
    //    rdModel3K_sub_4E1DA0();
    //    rdModel3K_sub_4E2F00(0);
    //}

    // Make sure sith has all required dirs set
    sithMakeDirs();

    wuRegistry_GetStr("StartLevel", pConfig->aCurLevelFilename, STD_ARRAYLEN(pConfig->aCurLevelFilename), "");

    pConfig->displaySettings.bWindowMode  = wuRegistry_GetIntEx("InWindow", 0);
    pConfig->displaySettings.bDualMonitor = wuRegistry_GetIntEx("Dual Monitor", 0);
    pConfig->displaySettings.bBuffering   = wuRegistry_GetIntEx("Buffering", 0);
    pConfig->displaySettings.filter       = wuRegistry_GetInt("Filter", 1); // bilinear

    pConfig->displaySettings.bFog = wuRegistry_GetIntEx("Fog", 1);
    pConfig->fogDensity           = wuRegistry_GetFloat("Fog Density", 1.0f);
    std3D_EnableFog(pConfig->displaySettings.bFog, pConfig->fogDensity);

    sithRender_g_fogDensity = pConfig->fogDensity * 100.0f;

    pConfig->bDevMode   = wuRegistry_GetIntEx("DevMode", 0);
    pConfig->startMode = wuRegistry_GetInt("Start Mode", 2);
    pConfig->startMode = STDMATH_CLAMP(pConfig->startMode, 0, 4);

    pConfig->outputMode = wuRegistry_GetInt("Debug Mode", 0);
    pConfig->logLevel   = wuRegistry_GetInt("Verbosity", 1);
    pConfig->performanceLevel = wuRegistry_GetInt("Performance Level", 4);

    pConfig->displaySettings.geoMode   = wuRegistry_GetInt("Geometry Mode", RD_GEOMETRY_FULL);
    pConfig->displaySettings.lightMode = wuRegistry_GetInt("Lighting Mode", RD_LIGHTING_GOURAUD);

    int bHiPoly = wuRegistry_GetIntEx("HiPoly", 1); // Changed: Enable by default, was disabled
    sithModel_EnableHiPoly(bHiPoly);

    JonesMain_pStartupDisplayEnv = pDisplayEnv;

    wuRegistry_GetStr("Display", aText, STD_ARRAYLEN(aText), "");

    for ( size_t i = 0; i < JonesMain_pStartupDisplayEnv->numInfos; ++i )
    {
        if ( JonesMain_pStartupDisplayEnv->aDisplayInfos[i].displayDevice.bHAL )
        {
            pConfig->displaySettings.displayDeviceNum = i;
        }

        if ( strcmp(JonesMain_pStartupDisplayEnv->aDisplayInfos[i].displayDevice.aDriverName, aText) == 0 )
        {
            pConfig->displaySettings.displayDeviceNum = i;
            break;
        }
    }

    StdDisplayInfo* pDisplay = &JonesMain_pStartupDisplayEnv->aDisplayInfos[pConfig->displaySettings.displayDeviceNum];
    for ( size_t i = 0; i < pDisplay->numDevices; ++i )
    {
        if ( strstr(pDisplay->aDevices[i].deviceDescription, "HAL") ) // contains "HAL" word
        {
            pConfig->displaySettings.device3DNum = i;
            break;
        }
    }

    wuRegistry_GetStr("3D Device", aText, STD_ARRAYLEN(aText), "");
    for ( size_t i = 0; i < pDisplay->numDevices; ++i )
    {
        if ( !strcmp(pDisplay->aDevices[i].deviceDescription, aText) )
        {
            pConfig->displaySettings.device3DNum = i;
            break;
        }
    }

    JonesMain_curVideoMode.aspectRatio                    = 1.0f;
    JonesMain_curVideoMode.rasterInfo.width               = wuRegistry_GetInt("Width", 640);
    JonesMain_curVideoMode.rasterInfo.height              = wuRegistry_GetInt("Height", 480);
    JonesMain_curVideoMode.rasterInfo.colorInfo.bpp       = wuRegistry_GetInt("BPP", 32);
    JonesMain_curVideoMode.rasterInfo.colorInfo.colorMode = STDCOLOR_RGB;

    pConfig->displaySettings.videoModeNum = JonesMain_FindClosestVideoMode(JonesMain_pStartupDisplayEnv, &JonesMain_curVideoMode, pConfig->displaySettings.displayDeviceNum);

    memcpy(&JonesMain_curVideoMode, &pDisplay->aModes[pConfig->displaySettings.videoModeNum], sizeof(JonesMain_curVideoMode));

    pConfig->displaySettings.width  = JonesMain_curVideoMode.rasterInfo.width;
    pConfig->displaySettings.height = JonesMain_curVideoMode.rasterInfo.height;
    pConfig->displaySettings.bClearBackBuffer = 0;

    pConfig->bSound3D      = wuRegistry_GetIntEx("Sound 3D", 0);
    pConfig->bReverseSound = wuRegistry_GetIntEx("ReverseSound", 0);
}


int J3DAPI JonesMain_ShowDevDialog(HWND hWnd, JonesState* pConfig)
{
    HINSTANCE hInstance = (HINSTANCE)GetWindowLongPtr(hWnd, GWL_HINSTANCE);
    return DialogBoxParam(hInstance, (LPCSTR)101, hWnd, JonesMain_DevDialogProc, (LPARAM)pConfig);
}

int J3DAPI JonesMain_ShowDisplayOptions(HWND hWnd, JonesState* pState)
{
    HINSTANCE hInstance = (HINSTANCE)GetWindowLongPtr(hWnd, GWL_HINSTANCE);
    return DialogBoxParam(hInstance, (LPCSTR)106, hWnd, JonesMain_DevDialogProc, (LPARAM)pState);
}

INT_PTR CALLBACK JonesMain_DevDialogProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch ( uMsg )
    {
        case WM_INITDIALOG:
            return JonesMain_InitDevDialog(hDlg, wParam, (JonesState*)lParam);

        case WM_COMMAND:
            JonesMain_DevDialogHandleCommand(hDlg, (uint16_t)wParam, lParam, HIWORD(wParam));
            return 1;
    };

    return 0;
}

int J3DAPI JonesMain_InitDevDialog(HWND hDlg, WPARAM wParam, JonesState* pConfig)
{
    J3D_UNUSED(wParam);

    // Populate display driver combo box list
    HWND hDlgItem = GetDlgItem(hDlg, 1030); // Display driver
    for ( size_t i = 0; i < JonesMain_pStartupDisplayEnv->numInfos; ++i )
    {
        if ( JonesMain_pStartupDisplayEnv->aDisplayInfos[i].displayDevice.bHAL == 1 )
        {
            STD_FORMAT(std_g_genBuffer, "%s", JonesMain_pStartupDisplayEnv->aDisplayInfos[i].displayDevice.aDriverName);
            int itemIdx = SendMessage(hDlgItem, CB_ADDSTRING, 0, (LPARAM)std_g_genBuffer);
            SendMessage(hDlgItem, CB_SETITEMDATA, itemIdx, i);

            // Select diver if matches the one in settings
            if ( i == pConfig->displaySettings.displayDeviceNum )
            {
                SendMessage(hDlgItem, CB_SETCURSEL, itemIdx, 0);
            }
        }
    }

    // Init CB GeometryMOde
    hDlgItem = GetDlgItem(hDlg, 1008);
    int itemIdx = SendMessage(hDlgItem, CB_ADDSTRING, 0, (LPARAM)"Vertex Only");
    SendMessage(hDlgItem, CB_SETITEMDATA, itemIdx, 1);

    int selectedItemIdx = itemIdx;
    itemIdx = SendMessage(hDlgItem, CB_ADDSTRING, 0, (LPARAM)"Wire Frame");
    SendMessage(hDlgItem, CB_SETITEMDATA, itemIdx, 2);
    if ( pConfig->displaySettings.geoMode == RD_GEOMETRY_WIREFRAME )
    {
        selectedItemIdx = itemIdx;
    }

    itemIdx = SendMessage(hDlgItem, CB_ADDSTRING, 0, (LPARAM)"Solid");
    SendMessage(hDlgItem, CB_SETITEMDATA, itemIdx, 3);
    if ( pConfig->displaySettings.geoMode == RD_GEOMETRY_SOLID )
    {
        selectedItemIdx = itemIdx;
    }

    itemIdx = SendMessage(hDlgItem, CB_ADDSTRING, 0, (LPARAM)"Texture");
    SendMessage(hDlgItem, CB_SETITEMDATA, itemIdx, 4);
    if ( pConfig->displaySettings.geoMode == RD_GEOMETRY_FULL )
    {
        selectedItemIdx = itemIdx;
    }

    SendMessage(hDlgItem, CB_SETCURSEL, selectedItemIdx, 0);

    // Init CB Lighting Mode
    hDlgItem = GetDlgItem(hDlg, 1009);
    itemIdx = SendMessage(hDlgItem, CB_ADDSTRING, 0, (LPARAM)"None");
    SendMessage(hDlgItem, CB_SETITEMDATA, itemIdx, 1);

    selectedItemIdx = itemIdx;
    itemIdx = SendMessage(hDlgItem, CB_ADDSTRING, 0, (LPARAM)"Lit");
    SendMessage(hDlgItem, CB_SETITEMDATA, itemIdx, 0);
    if ( pConfig->displaySettings.lightMode == RD_LIGHTING_NONE )
    {
        selectedItemIdx = itemIdx;
    }

    itemIdx = SendMessage(hDlgItem, CB_ADDSTRING, 0, (LPARAM)"Diffuse");
    SendMessage(hDlgItem, CB_SETITEMDATA, itemIdx, 2);
    if ( pConfig->displaySettings.lightMode == RD_LIGHTING_DIFFUSE )
    {
        selectedItemIdx = itemIdx;
    }

    itemIdx = SendMessage(hDlgItem, CB_ADDSTRING, 0, (LPARAM)"Gouraud");
    SendMessage(hDlgItem, CB_SETITEMDATA, itemIdx, 3);
    if ( pConfig->displaySettings.lightMode == RD_LIGHTING_GOURAUD )
    {
        selectedItemIdx = itemIdx;
    }

    SendMessage(hDlgItem, CB_SETCURSEL, selectedItemIdx, 0);

    // Init CB MimpMap Filter
    hDlgItem = GetDlgItem(hDlg, 1012);
    itemIdx = SendMessage(hDlgItem, CB_ADDSTRING, 0, (LPARAM)"None");
    SendMessage(hDlgItem, CB_SETITEMDATA, itemIdx, 0);

    itemIdx = SendMessage(hDlgItem, CB_ADDSTRING, 0, (LPARAM)"Bilinear");
    SendMessage(hDlgItem, CB_SETCURSEL, itemIdx, 0);// Select bilinear as defult
    SendMessage(hDlgItem, CB_SETITEMDATA, itemIdx, 1);

    itemIdx = SendMessage(hDlgItem, CB_ADDSTRING, 0, (LPARAM)"Trilinear");
    SendMessage(hDlgItem, CB_SETITEMDATA, itemIdx, 2);

    // Added
    // Enable and init  HiPoly check button
    hDlgItem = GetDlgItem(hDlg, 1051);
    EnableWindow(hDlgItem, 1);
    ShowWindow(hDlgItem, 1);
    CheckDlgButton(hDlg, 1051, sithModel_IsHiPolyEnabled());

    // Setup Display related controls
    JonesMain_DevDialogInitDisplayDevices(hDlg, pConfig);

    // Populate level List box
    hDlgItem = GetDlgItem(hDlg, 1001);
    if ( hDlgItem )
    {
        tFoundFileInfo fileInfo;
        FindFileData* pFileData = stdFileUtil_NewFind("ndy", 3, "ndy");
        while ( stdFileUtil_FindNext(pFileData, &fileInfo) )
        {
            itemIdx = SendMessage(hDlgItem, LB_ADDSTRING, 0, (LPARAM)&fileInfo);
        }

        stdFileUtil_DisposeFind(pFileData);
        pFileData = stdFileUtil_NewFind("ndy", 3, "cnd");
        while ( stdFileUtil_FindNext(pFileData, &fileInfo) )
        {
            itemIdx = SendMessage(hDlgItem, LB_ADDSTRING, 0, (LPARAM)&fileInfo);
        }

        stdFileUtil_DisposeFind(pFileData);

        // Select cur level from config
        int numLevels = SendMessage(hDlgItem, LB_GETCOUNT, 0, 0);
        for ( itemIdx = 0; itemIdx < numLevels; ++itemIdx )
        {
            char aLevelName[128] = { 0 }; // Fixed: Increased string len to 128 from 64
            SendMessage(hDlgItem, LB_GETTEXT, itemIdx, (LPARAM)aLevelName);
            if ( strcmp(aLevelName, pConfig->aCurLevelFilename) == 0 )
            {
                SendMessage(hDlgItem, LB_SETCURSEL, itemIdx, 0);
                break;
            }
        }

        if ( itemIdx == numLevels )
        {
            // Non selected, select level in the middle of the list
            SendMessage(hDlgItem, LB_SETCURSEL, numLevels / 2, 0);
        }
    }

    SetWindowLongPtr(hDlg, DWL_USER, (LONG)pConfig); // Set config to dialog handle

    CheckDlgButton(hDlg, 1007, pConfig->bDevMode);// Dev mode

    JonesMain_DevDialogUpdateRadioButtons(hDlg, pConfig);
    return 1;
}

void J3DAPI JonesMain_DevDialogHandleCommand(HWND hWnd, int controlId, LPARAM lParam, int hiWParam)
{
    J3D_UNUSED(lParam);

    JonesState* pState = (JonesState*)GetWindowLongPtr(hWnd, DWL_USER);
    if ( controlId > 1001 )
    {
        switch ( controlId )
        {
            case 1013: // error output normal
                pState->outputMode = JONES_OUTPUTMODE_NONE;
                JonesMain_DevDialogUpdateRadioButtons(hWnd, pState);
                break;

            case 1014: // error output console
                pState->outputMode = JONES_OUTPUTMODE_CONSOLE;
                JonesMain_DevDialogUpdateRadioButtons(hWnd, pState);
                break;

            case 1015: // error output file
                pState->outputMode = JONES_OUTPUTMODE_LOGFILE;
                JonesMain_DevDialogUpdateRadioButtons(hWnd, pState);
                break;

            case 1016: // debug output normal
                pState->logLevel = JONES_LOGLEVEL_NORMAL;
                JonesMain_DevDialogUpdateRadioButtons(hWnd, pState);
                break;

            case 1017: // debug output verbose
                pState->logLevel = JONES_LOGLEVEL_VERBOSE;
                JonesMain_DevDialogUpdateRadioButtons(hWnd, pState);
                break;

            case 1018: // debug output quiet
                pState->logLevel = JONES_LOGLEVEL_ERROR;
                JonesMain_DevDialogUpdateRadioButtons(hWnd, pState);
                break;

            case 1029: // display mode
                if ( hiWParam == 1 )
                {
                    HWND hCBDisplayMode = GetDlgItem(hWnd, 1029);
                    int curSelIdx = SendMessage(hCBDisplayMode, CB_GETCURSEL, 0, 0);
                    pState->displaySettings.videoModeNum = SendMessage(hCBDisplayMode, CB_GETITEMDATA, curSelIdx, 0);
                }
                break;

            case 1030: // Display settings
                if ( hiWParam == 1 )
                {
                    HWND CBDisplayDriver = GetDlgItem(hWnd, 1030);
                    int curSelIdx = SendMessage(CBDisplayDriver, CB_GETCURSEL, 0, 0);
                    pState->displaySettings.displayDeviceNum = SendMessage(CBDisplayDriver, CB_GETITEMDATA, curSelIdx, 0);
                    JonesMain_DevDialogInitDisplayDevices(hWnd, pState);
                }
                break;

            case 1031: // 3D Driver
                if ( hiWParam == 1 )
                {
                    HWND hCB3DDriver = GetDlgItem(hWnd, 1031);
                    int curSelIdx = SendMessage(hCB3DDriver, CB_GETCURSEL, 0, 0);
                    pState->displaySettings.device3DNum = SendMessage(hCB3DDriver, CB_GETITEMDATA, curSelIdx, 0);
                    JonesMain_DevDialogInitDisplayDevices(hWnd, pState);
                }
                break;

            case 1176: // performance level 0
                pState->performanceLevel = 0;
                JonesMain_DevDialogUpdateRadioButtons(hWnd, pState);
                break;

            case 1177: // performance level 1
                pState->performanceLevel = 1;
                JonesMain_DevDialogUpdateRadioButtons(hWnd, pState);
                break;

            case 1178: // performance level 2
                pState->performanceLevel = 2;
                JonesMain_DevDialogUpdateRadioButtons(hWnd, pState);
                break;

            case 1179: // performance level 3
                pState->performanceLevel = 3;
                JonesMain_DevDialogUpdateRadioButtons(hWnd, pState);
                break;

            case 1180: // performance level 4
                pState->performanceLevel = 4;
                JonesMain_DevDialogUpdateRadioButtons(hWnd, pState);
                break;

            default:
                return;
        }
    }
    else
    {
        switch ( controlId )
        {
            case 1001: // select level list
                if ( hiWParam != 2 )
                {
                    return;
                }

                controlId = 1; // start game
                break;

            case 1: // start the game
                break;

            case 2: // exit
                EndDialog(hWnd, controlId);
                return;

            default:
                return;
        }

        // Start game logic

        // Get 3D driver
        HWND hCB3DDriver = GetDlgItem(hWnd, 1031);
        int curSelIdx = SendMessage(hCB3DDriver, CB_GETCURSEL, 0, 0);
        pState->displaySettings.device3DNum = SendMessage(hCB3DDriver, CB_GETITEMDATA, curSelIdx, 0);

        // Get Display mode
        HWND hCBDisplayMode = GetDlgItem(hWnd, 1029);
        curSelIdx = SendMessage(hCBDisplayMode, CB_GETCURSEL, 0, 0);
        pState->displaySettings.videoModeNum = SendMessage(hCBDisplayMode, CB_GETITEMDATA, curSelIdx, 0);

        // Geometry mode
        HWND hCBGeometryMode = GetDlgItem(hWnd, 1008);
        curSelIdx = SendMessage(hCBGeometryMode, CB_GETCURSEL, 0, 0);
        pState->displaySettings.geoMode = SendMessage(hCBGeometryMode, CB_GETITEMDATA, curSelIdx, 0);

        if ( pState->displaySettings.geoMode == RD_GEOMETRY_WIREFRAME || pState->displaySettings.geoMode == RD_GEOMETRY_VERTEX )
        {
            pState->displaySettings.bClearBackBuffer = 1;
        }

        // Light mode
        HWND hCBLightMode = GetDlgItem(hWnd, 1009);
        curSelIdx = SendMessage(hCBLightMode, CB_GETCURSEL, 0, 0);
        pState->displaySettings.lightMode = SendMessage(hCBLightMode, CB_GETITEMDATA, curSelIdx, 0);

        // Filter mode
        HWND hCBFilterMode = GetDlgItem(hWnd, 1012);
        curSelIdx = SendMessage(hCBFilterMode, CB_GETCURSEL, 0, 0);
        pState->displaySettings.filter = SendMessage(hCBFilterMode, CB_GETITEMDATA, curSelIdx, 0);

        // Added: Enable/Disable HiPoly
        sithModel_EnableHiPoly(IsDlgButtonChecked(hWnd, 1051) == 1);

        pState->displaySettings.bWindowMode = IsDlgButtonChecked(hWnd, 1002) == 1;// window mode
        pState->bDevMode = IsDlgButtonChecked(hWnd, 1007) == 1;// devmode

        pState->displaySettings.width = JonesMain_pStartupDisplayEnv->aDisplayInfos[pState->displaySettings.displayDeviceNum].aModes[pState->displaySettings.videoModeNum].rasterInfo.width;
        pState->displaySettings.height = JonesMain_pStartupDisplayEnv->aDisplayInfos[pState->displaySettings.displayDeviceNum].aModes[pState->displaySettings.videoModeNum].rasterInfo.height;

        // Get selected level & Save settings
        HWND hCBLevelList = GetDlgItem(hWnd, 1001);
        curSelIdx = SendMessage(hCBLevelList, LB_GETCURSEL, 0, 0);
        if ( SendMessage(hCBLevelList, LB_GETTEXT, curSelIdx, (LPARAM)pState->aCurLevelFilename) != -1 )
        {
            wuRegistry_SaveStr("Display", JonesMain_pStartupDisplayEnv->aDisplayInfos[pState->displaySettings.displayDeviceNum].displayDevice.aDriverName);
            wuRegistry_SaveStr("3D Device", JonesMain_pStartupDisplayEnv->aDisplayInfos[pState->displaySettings.displayDeviceNum].aDevices[pState->displaySettings.device3DNum].deviceDescription);
            wuRegistry_SaveInt("Width", pState->displaySettings.width);
            wuRegistry_SaveInt("Height", pState->displaySettings.height);
            wuRegistry_SaveInt("BPP", JonesMain_pStartupDisplayEnv->aDisplayInfos[pState->displaySettings.displayDeviceNum].aModes[pState->displaySettings.videoModeNum].rasterInfo.colorInfo.bpp);
            wuRegistry_SaveInt("Filter", pState->displaySettings.filter);

            wuRegistry_SaveStr("StartLevel", pState->aCurLevelFilename);

            wuRegistry_SaveIntEx("InWindow", pState->displaySettings.bWindowMode);
            wuRegistry_SaveIntEx("Dual Monitor", pState->displaySettings.bDualMonitor);

            wuRegistry_SaveIntEx("DevMode", pState->bDevMode);

            wuRegistry_SaveIntEx("Sound 3D", pState->bSound3D);

            wuRegistry_SaveInt("Debug Mode", pState->outputMode);
            wuRegistry_SaveInt("Verbosity", pState->logLevel);

            wuRegistry_SaveStr("User Path", pState->aInstallPath);
            wuRegistry_SaveInt("Performance Level", pState->performanceLevel);
            wuRegistry_SaveInt("HiPoly", sithModel_IsHiPolyEnabled()); // Added

            wuRegistry_SaveInt("Geometry Mode", pState->displaySettings.geoMode);
            wuRegistry_SaveInt("Lighting Mode", pState->displaySettings.lightMode);

            EndDialog(hWnd, controlId); // Close dialog
        }
    }
}

void J3DAPI JonesMain_DevDialogInitDisplayDevices(HWND hDlg, JonesState* pConfig)
{
    HWND hBTNStart      = GetDlgItem(hDlg, 1);
    HWND hCBWindowMode  = GetDlgItem(hDlg, 1002);
    HWND hCB3DDriver    = GetDlgItem(hDlg, 1031);

    HWND hCBDisplayMode = GetDlgItem(hDlg, 1029);
    SendMessage(hCBDisplayMode, CB_RESETCONTENT, 0, 0);

    StdDisplayInfo* pDisplay = &JonesMain_pStartupDisplayEnv->aDisplayInfos[pConfig->displaySettings.displayDeviceNum];
    if ( !pDisplay->displayDevice.bHAL )
    {
        EnableWindow(hBTNStart, 0);
        EnableWindow(hCBWindowMode, 0);
        EnableWindow(hCBDisplayMode, 0);
        EnableWindow(hCB3DDriver, 0);
        return;
    }

    EnableWindow(hBTNStart, 1);
    EnableWindow(hCBWindowMode, pDisplay->displayDevice.bWindowRenderNotSupported == 0);

    if ( pDisplay->displayDevice.bWindowRenderNotSupported )
    {
        pConfig->displaySettings.bWindowMode = 0;
    }

    EnableWindow(hCBDisplayMode, 1);
    EnableWindow(hCB3DDriver, 1);
    CheckDlgButton(hDlg, 1002, pConfig->displaySettings.bWindowMode);// CB window mode

    // Init 3D driver combo box list
    bool bDriverSet = false;

    CHAR aDriverName[128] = { 0 };
    GetWindowText(hCB3DDriver, aDriverName, STD_ARRAYLEN(aDriverName));

    SendMessage(hCB3DDriver, CB_RESETCONTENT, 0, 0);
    for ( size_t deviceNum = 0; deviceNum < pDisplay->numDevices; ++deviceNum )
    {
        STD_FORMAT(std_g_genBuffer, "%s", pDisplay->aDevices[deviceNum].deviceDescription);

        int itemIdx = SendMessage(hCB3DDriver, CB_ADDSTRING, 0, (LPARAM)std_g_genBuffer);
        SendMessage(hCB3DDriver, CB_SETITEMDATA, itemIdx, deviceNum);

        // Select item in cb list
        if ( strcmp(aDriverName, std_g_genBuffer) == 0 || pConfig->displaySettings.device3DNum == deviceNum )
        {
            SendMessage(hCB3DDriver, CB_SETCURSEL, itemIdx, 0);
            pConfig->displaySettings.device3DNum = deviceNum;
            bDriverSet = true;
        }
    }

    if ( !bDriverSet )
    {
        pConfig->displaySettings.device3DNum = 0;
        SendMessage(hCB3DDriver, CB_SETCURSEL, 0, 0);
    }

    // Populate display mode combo box list (resolutions) and select 

    // JonesMain_curVideoMode.rasterInfo.colorInfo.bpp = 16; // Removed: This prevents getting stored 32 BPP resolution from config

    bDriverSet = false;
    for ( size_t modeNum = 0; modeNum < JonesMain_pStartupDisplayEnv->aDisplayInfos[pConfig->displaySettings.displayDeviceNum].numModes; ++modeNum )
    {
        if ( pDisplay->aModes[modeNum].aspectRatio == 1.0f
          && pDisplay->aModes[modeNum].rasterInfo.width >= 512
          && pDisplay->aModes[modeNum].rasterInfo.height >= 384 )
        {
            if ( JonesMain_CurDisplaySupportsBPP(&pConfig->displaySettings, pDisplay->aModes[modeNum].rasterInfo.colorInfo.bpp) )
            {
                STD_FORMAT(std_g_genBuffer, "%dx%d %dbpp", pDisplay->aModes[modeNum].rasterInfo.width, pDisplay->aModes[modeNum].rasterInfo.height, pDisplay->aModes[modeNum].rasterInfo.colorInfo.bpp);  // Changed: Moved in this scope
                int itemIdx = SendMessage(hCBDisplayMode, CB_ADDSTRING, 0, (LPARAM)std_g_genBuffer);
                SendMessage(hCBDisplayMode, CB_SETITEMDATA, itemIdx, modeNum);

                // Select mode
                if ( pDisplay->aModes[modeNum].rasterInfo.width == JonesMain_curVideoMode.rasterInfo.width
                  && pDisplay->aModes[modeNum].rasterInfo.height == JonesMain_curVideoMode.rasterInfo.height
                  && pDisplay->aModes[modeNum].rasterInfo.colorInfo.bpp == JonesMain_curVideoMode.rasterInfo.colorInfo.bpp )
                {
                    SendMessage(hCBDisplayMode, CB_SETCURSEL, itemIdx, 0);
                    bDriverSet = true;
                }
            }
        }
    }

    if ( !bDriverSet )
    {
        SendMessage(hCBDisplayMode, CB_SETCURSEL, 0, 0);
    }
}

void J3DAPI JonesMain_DevDialogUpdateRadioButtons(HWND hDlg, const JonesState* pState)
{
    switch ( pState->outputMode )
    {
        case JONES_OUTPUTMODE_NONE:
            CheckRadioButton(hDlg, 1013, 1015, 1013); // Normal
            break;
        case JONES_OUTPUTMODE_CONSOLE:
            CheckRadioButton(hDlg, 1013, 1015, 1014); // Console
            break;
        case JONES_OUTPUTMODE_LOGFILE:
            CheckRadioButton(hDlg, 1013, 1015, 1015); // Log file
            break;
    };

    switch ( pState->logLevel )
    {
        case JONES_LOGLEVEL_ERROR:
            CheckRadioButton(hDlg, 1016, 1018, 1018); // Quite
            break;
        case JONES_LOGLEVEL_NORMAL:
            CheckRadioButton(hDlg, 1016, 1018, 1016); // Normal
            break;
        case JONES_LOGLEVEL_VERBOSE:
            CheckRadioButton(hDlg, 1016, 1018, 1017); // Verbose
            break;
    };

    switch ( pState->performanceLevel )
    {
        case 0:
            CheckRadioButton(hDlg, 1176, 1180, 1176);
            break;

        case 1:
            CheckRadioButton(hDlg, 1176, 1180, 1177);
            break;

        case 2:
            CheckRadioButton(hDlg, 1176, 1180, 1178);
            break;

        case 3:
            CheckRadioButton(hDlg, 1176, 1180, 1179);
            break;

        case 4:
            CheckRadioButton(hDlg, 1176, 1180, 1180);
            break;

        default:
            return;
    }
}

size_t J3DAPI JonesMain_FindClosestVideoMode(const StdDisplayEnvironment* pList, const StdVideoMode* pVideoMode, size_t deviceNum)
{
    size_t videoMode = 0;
    StdDisplayInfo* pDisplay = &pList->aDisplayInfos[deviceNum];
    for ( size_t i = 0; i < pList->aDisplayInfos[deviceNum].numModes; ++i )
    {
        if ( pDisplay->aModes[i].rasterInfo.colorInfo.bpp == pVideoMode->rasterInfo.colorInfo.bpp ) // Fixed: Changed hardcoded 16 BPP check to pVideoMode BPP compare
        {
            if ( pDisplay->aModes[i].rasterInfo.width == pVideoMode->rasterInfo.width && pDisplay->aModes[i].rasterInfo.height == pVideoMode->rasterInfo.height )
            {
                return i; // exact
            }

            videoMode = i; // closest
        }
    }

    return videoMode;
}

bool J3DAPI JonesMain_CurDisplaySupportsBPP(const JonesDisplaySettings* pSettings, size_t bpp)
{
    StdDisplayInfo* pDisplay = &JonesMain_pStartupDisplayEnv->aDisplayInfos[pSettings->displayDeviceNum];
    switch ( bpp )
    {
        case 8:
            return(pDisplay->aDevices[pSettings->device3DNum].d3dDesc.dwDeviceRenderBitDepth & DDBD_8) != 0;
        case 16:
            return (pDisplay->aDevices[pSettings->device3DNum].d3dDesc.dwDeviceRenderBitDepth & DDBD_16) != 0;
        case 24:
            return (pDisplay->aDevices[pSettings->device3DNum].d3dDesc.dwDeviceRenderBitDepth & DDBD_24) != 0;
        case 32:
            return (pDisplay->aDevices[pSettings->device3DNum].d3dDesc.dwDeviceRenderBitDepth & DDBD_32) != 0;
    }

    return false;
}
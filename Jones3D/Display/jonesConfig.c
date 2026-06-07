#include "jonesConfig.h"
#include <j3dcore/j3dhook.h>

#include <Jones3D/Display/JonesDisplay.h>
#include <Jones3D/Display/JonesHud.h>
#include <Jones3D/Display/JonesHudConstants.h>
#include <Jones3D/Display/JonesReticle.h>
#include <Jones3D/Gui/JonesDialog.h>
#include <Jones3D/Main/JonesFile.h>
#include <Jones3D/Main/JonesLevel.h>
#include <Jones3D/Main/JonesMain.h>
#include <Jones3D/Main/jonesString.h>
#include <Jones3D/Play/JonesControl.h>
#include <Jones3D/Play/jonesInventory.h>
#include <Jones3D/RTI/symbols.h>

#include <rdroid/Main/rdroid.h>
#include <rdroid/Raster/rdCache.h>

#include <sith/Devices/sithControl.h>
#include <sith/Devices/sithSoundMixer.h>
#include <sith/DSS/sithGamesave.h>
#include <sith/Engine/sithRender.h>
#include <sith/Gameplay/sithOverlayMap.h>
#include <sith/Gameplay/sithPlayer.h>
#include <sith/Main/sithString.h>
#include <sith/World/sithModel.h>
#include <sith/World/sithVoice.h>
#include <sith/World/sithWeapon.h>
#include <sith/World/sithWorld.h>

#include <std/General/std.h>
#include <std/General/stdBmp.h>
#include <std/General/stdConfig.h>
#include <std/General/stdFnames.h>
#include <std/General/stdMemory.h>
#include <std/General/stdUtil.h>
#include <std/Win95/std3D.h>
#include <std/Win95/stdControl.h>
#include <std/Win95/stdDisplay.h>
#include <std/Win95/stdWin95.h>

#include <sound/Sound.h>

#include <math.h>
#include <windowsx.h>

// Joystick AXIS key IDs
#define JONESCONFIG_JOYAXISKID_UP       0x8000u
#define JONESCONFIG_JOYAXISKID_RIGHT    0x8001u
#define JONESCONFIG_JOYAXISKID_DOWN     0x8002u
#define JONESCONFIG_JOYAXISKID_LEFT     0x8003u
#define JONESCONFIG_JOYAXISKID_TWISTL   0x8004u
#define JONESCONFIG_JOYAXISKID_TWISTR   0x8005u
#define JONESCONFIG_JOYAXISKID_AIR      0x8006u
#define JONESCONFIG_JOYAXISKID_GROUND   0x8007u
#define JONESCONFIG_NUM_JOYAXISKIDS     8u

#define JONESCONFIG_CONTROLSTRING_MAXLEN 256u

#define JONESCONFIG_NOFONTSCALEMASK 1119
#define JONESCONFIG_NOFONTSCALE(dlgID) MAKELONG(dlgID, JONESCONFIG_NOFONTSCALEMASK)

typedef struct sJonesConfigControlAction
{
    const size_t numBinds;
    const size_t* aBindings;
} JonesConfigControlAction;

typedef struct sJonesMessageDialogData
{
    int iconID;
    char aTitle[256];
    char aText[256];
    HICON hPrevIcon;
} JonesMessageDialogData;

typedef struct sJonesCreateControlSchemeDialogData
{
    char aNewSchemeName[128];
    HWND hSchemeList;
} JonesCreateControlSchemeDialogData;

typedef struct sJonesEditControlSchemeDialogData
{
    JonesControlsScheme* aSchemes;
    int numSchemes;
    HWND hCurList;
    int curListID;
    JonesControlsScheme* pDefaultShceme;
} JonesEditControlSchemeDialogData;

typedef struct sJonesAssignKeyDialogData
{
    size_t actionId;
    JonesControlsScheme* pScheme;
    char aControlName[256];
    size_t controlId;
    int listID;
    int bLocked;
} JonesAssignKeyDialogData;
static_assert(sizeof(JonesAssignKeyDialogData) == 276, "sizeof(JonesAssignKeyDialogData) == 276");

typedef struct sJonesReAssignKeyDialogData
{
    JonesAssignKeyDialogData* pAssignKeyData;
    int index;
    size_t offset;
    size_t actionId;
    HWND hAssignKeyDlg;
} JonesReAssignKeyDialogData;

typedef struct sJonesDisplaySettingsVideoMode
{
    char aResolutionText[256];
    int colorDepthFlags;
    int aColorDepthVideoModes[5];
} JonesDisplaySettingsVideoMode;

typedef struct sJonesDisplaySettingsDialogData
{
    JonesDisplaySettings* pSettings;
    StdDisplayEnvironment* pDisplayEnv;
    JonesDisplaySettingsVideoMode* aVideoModes;
    const JonesDisplaySettingsVideoMode** aCurColorDepthResolutions;
    size_t numCurColorDepthResolutions;
    size_t numVideoModes;
} JonesDisplaySettingsDialogData;

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

// Module vars
static int jonesConfig_bStartup = 0; // Fixed: Added init. to 0

static HBITMAP jonesConfig_apDialogIcons[6];
static char jonesConfig_aKeySetsDirPath[128] = { 0 }; // Fixed: Added init. to 0

static int jonesConfig_bTextMetricInited;
static ABC jonesConfig_aGlyphMetrics[256] = { 0 };
static TEXTMETRICA jonesConfig_textMetric;

// Message dialog
HFONT jonesConfig_hFontMessageDialog = NULL; // Fixed: Added init. to NULL

// Game save dialog
static HFONT jonesConfig_hFontSaveGameDlg = NULL;

// Override game save/load message box dialog
HFONT jonesConfig_hFontGameSaveMsgBox = NULL; // Fixed: Added init. to NULL

// Game load dialog
static HFONT jonesConfig_hFontLoadGameDlg = NULL;

// Game play options dialog
static HFONT jonesConfig_hFontGamePlayOptionsDlg = NULL;
static int jonesConfig_gamePlayOptions_bDefaultRun;

// Exit game dialog
static HFONT jonesConfig_hFontExitDlg = NULL;

// Control options dialog
static HFONT jonesConfig_hFontControlOptions = NULL; // Fixed: Added init. to NULL

static JonesControlsConfig jonesConfig_controlOptions_curControlConfig = { 0 };
static JonesControlsScheme* jonesConfig_controlOptions_pCurNewScheme;

// Create control scheme
HFONT jonesConfig_hFontCreateControlSchemeDlg = NULL; // Fixed: Added init. to NULL

// Edit control scheme dialog
HFONT jonesConfig_hFontEditControlShceme = NULL; // Fixed: Added init. to NULL

// Assign key control options dialog
HFONT jonesConfig_hFontAssignKeyDlg = NULL; // Fixed: Added init. to NULL
int jonesConfig_assignKey_bControlsActive;

// Re-assign key control options dialog
HFONT jonesConfig_hFontReassignKeyDlg = NULL; // Fixed: Added init. to NULL

// Display settings dialog
static HFONT jonesConfig_hFontDisplaySettingsDlg = NULL; // Fixed: Added init. to NULL
static int jonesConfig_displaySettings_bUpdateSliderText = 1;

// Advance display settings dialog
static HFONT jonesConfig_hFontAdvanceDisplaySettingsDialog = NULL; // Fixed: Added init. to NULL
static Std3DMipmapFilterType jonesConfig_advanceDisplaySettings_curFilterMode = STD3D_MIPMAPFILTER_TRILINEAR; // Altered: Changed to Trilinear from Bilinear
static int jonesConfig_advanceDisplaySettings_perfLevel = 4;

// Sound settings dialog
static HFONT jonesConfig_hFontSoundSettings = NULL; // Fixed: Added init. to NULL
static tSoundChannelHandle jonesConfig_soundSettings_hSndChannel;
static int jonesConfig_soundSettings_curSoundVolume;

// Game overr dialog
static HFONT jonesConfig_hFontGameOverDialog = NULL; // Fixed: Added init. to NULL

// Game statistics dialog
static HFONT jonesConfig_hFontGameStatisticsDialog = NULL; // Fixed: Added init. to NULL
int jonesConfig_gameStatistics_curLevelNum;

static const char* jonesConfig_gameStatistics__aLevelNames[JONESLEVEL_NUMLEVELS - 1] = // not including the general load level
{
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

// Chapter completed dialog
static int jonesConfig_prevLevelNum = -1;
static HFONT jonesConfig_hFontLevelCopletedDialog;

// Store dialog
static HFONT jonesConfig_hFontStoreDialog;
static HFONT jonesConfig_hFontPurchaseMessageBox;

// CD dialog
static HFONT jonesConfig_hFontDialogInsertCD;

// Control keyset scheme serialization vars
static size_t jonesConfig_maxControlStrLen       = 0u;
static size_t jonesConfig_maxControlWords        = 0u;
static size_t jonesConfig_maxControlPaddedStrLen = 0u;
static size_t jonesConfig_maxActionStrLen        = 0u;
static size_t jonesConfig_maxActionWords         = 0u;
static size_t jonesConfig_maxActionPaddedStrLen  = 0u; // Fixed: Init to 0

static const JonesDialogSize jonesConfig_aDialogSizes[21] =
{
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
    { 159, 583, 330 }, // Changed: height to 330 from 250
    { 154, 579, 332 }, // Changed: height to 332 from 252
    { 167, 279, 75 }
};

// jonesConfig_DrawStatisticDialogIQPoints
static const StdRect jonesConfig_aNumberGlyphMetrics[10] =
{
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

static const char* jonesConfig_apDialogIconFiles[6] =
{
    "iq.bmp",
    "iqMask.bmp",
    "iqOverlay.bmp",
    "numbers.bmp",
    "exit.bmp",
    "exitmask.bmp"
};

// Keyboard default keysets

static const size_t aActionWalkFwdKeys[2]     = { DIK_UP   , JONESCONFIG_JOYAXISKID_UP };
static const size_t aActionWalkBackKeys[2]    = { DIK_DOWN , JONESCONFIG_JOYAXISKID_DOWN };
static const size_t aActionTurnLeftKeys[2]    = { DIK_LEFT , JONESCONFIG_JOYAXISKID_LEFT };
static const size_t aActionTurnRightKeys[2]   = { DIK_RIGHT, JONESCONFIG_JOYAXISKID_RIGHT };
static const size_t aActionStepLeftKeys[1]    = { DIK_A };
static const size_t aActionStepRightKeys[1]   = { DIK_S };
static const size_t aActionCrawlKeys[1]       = { DIK_C };
static const size_t aActionRunKeys[2]         = { DIK_LSHIFT, DIK_RSHIFT };
static const size_t aActionRollKeys[1]        = { DIK_Z };
static const size_t aActionJumpKeys[4]        = { DIK_X, DIK_LMENU, DIK_RMENU, STDCONTROL_JOYSTICK_GETBUTTON(0, 1) };
static const size_t aActionLookKeys[2]        = { DIK_INSERT, DIK_NUMPAD0 };
static const size_t aActionActivateKeys[3]    = { DIK_LCONTROL, DIK_RCONTROL, STDCONTROL_JOYSTICK_GETBUTTON(0, 0) };
static const size_t aActionToggleMapKeys[1]   = { DIK_SPACE };
static const size_t aActionPrevWeaponKeys[1]  = { DIK_COMMA };
static const size_t aActionNextWeaponKeys[1]  = { DIK_PERIOD };
static const size_t aActionToggleLightKeys[1] = { DIK_L };
static const size_t aActionFistsKeys[1]       = { DIK_0 };
static const size_t aActionWhipKeys[1]        = { DIK_1 };
static const size_t aActionMauserKeys[1]      = { DIK_4 };
static const size_t aActionPPSH41Keys[1]      = { DIK_7 };
static const size_t aActionPistolKeys[1]      = { DIK_2 };
static const size_t aActionSimonovKeys[1]     = { DIK_5 };
static const size_t aActionTokarevKeys[1]     = { DIK_3 };
static const size_t aActionToz34Keys[1]       = { DIK_8 };
static const size_t aActionBazookaKeys[1]     = { DIK_9 };
static const size_t aActionMacheteKeys[1]     = { DIK_6 };
static const size_t aActionSatchelKeys[1]     = { DIK_P };
static const size_t aActionGrenadeKeys[1]     = { DIK_G };
static const size_t aActionMapKeys[1]         = { DIK_M };
static const size_t aActionInterfaceKeys[1]   = { DIK_I };
static const size_t aActionHealthKeys[1]      = { DIK_H };
static const size_t aActionIMP1Keys[1]        = { DIK_Q };
static const size_t aActionIMP2Keys[1]        = { DIK_W };
static const size_t aActionIMP3Keys[1]        = { DIK_E };
static const size_t aActionIMP4Keys[1]        = { DIK_R };
static const size_t aActionIMP5Keys[1]        = { DIK_T };
static const size_t aActionChalk[1]           = { DIK_K };

static const JonesConfigControlAction jonesConfig_aDefaultControlSet[JONESCONTROL_ACTION_NUMACTIONS] =
{
    { STD_ARRAYLEN(aActionWalkFwdKeys)    , aActionWalkFwdKeys     },
    { STD_ARRAYLEN(aActionWalkBackKeys)   , aActionWalkBackKeys    },
    { STD_ARRAYLEN(aActionTurnLeftKeys)   , aActionTurnLeftKeys    },
    { STD_ARRAYLEN(aActionTurnRightKeys)  , aActionTurnRightKeys   },
    { STD_ARRAYLEN(aActionStepLeftKeys)   , aActionStepLeftKeys    },
    { STD_ARRAYLEN(aActionStepRightKeys)  , aActionStepRightKeys   },
    { STD_ARRAYLEN(aActionCrawlKeys)      , aActionCrawlKeys       },
    { STD_ARRAYLEN(aActionRunKeys)        , aActionRunKeys         },
    { STD_ARRAYLEN(aActionRollKeys)       , aActionRollKeys        },
    { STD_ARRAYLEN(aActionJumpKeys)       , aActionJumpKeys        },
    { STD_ARRAYLEN(aActionLookKeys)       , aActionLookKeys        },
    { STD_ARRAYLEN(aActionActivateKeys)   , aActionActivateKeys    },
    { STD_ARRAYLEN(aActionToggleMapKeys)  , aActionToggleMapKeys   },
    { STD_ARRAYLEN(aActionPrevWeaponKeys) , aActionPrevWeaponKeys  },
    { STD_ARRAYLEN(aActionNextWeaponKeys) , aActionNextWeaponKeys  },
    { STD_ARRAYLEN(aActionToggleLightKeys), aActionToggleLightKeys },
    { STD_ARRAYLEN(aActionFistsKeys)      , aActionFistsKeys       },
    { STD_ARRAYLEN(aActionWhipKeys)       , aActionWhipKeys        },
    { STD_ARRAYLEN(aActionMauserKeys)     , aActionMauserKeys      },
    { STD_ARRAYLEN(aActionPPSH41Keys)     , aActionPPSH41Keys      },
    { STD_ARRAYLEN(aActionPistolKeys)     , aActionPistolKeys      },
    { STD_ARRAYLEN(aActionSimonovKeys)    , aActionSimonovKeys     },
    { STD_ARRAYLEN(aActionTokarevKeys)    , aActionTokarevKeys     },
    { STD_ARRAYLEN(aActionToz34Keys)      , aActionToz34Keys       },
    { STD_ARRAYLEN(aActionBazookaKeys)    , aActionBazookaKeys     },
    { STD_ARRAYLEN(aActionMacheteKeys)    , aActionMacheteKeys     },
    { STD_ARRAYLEN(aActionSatchelKeys)    , aActionSatchelKeys     },
    { STD_ARRAYLEN(aActionGrenadeKeys)    , aActionGrenadeKeys     },
    { STD_ARRAYLEN(aActionMapKeys)        , aActionMapKeys         },
    { STD_ARRAYLEN(aActionInterfaceKeys)  , aActionInterfaceKeys   },
    { STD_ARRAYLEN(aActionHealthKeys)     , aActionHealthKeys      },
    { STD_ARRAYLEN(aActionIMP1Keys)       , aActionIMP1Keys        },
    { STD_ARRAYLEN(aActionIMP2Keys)       , aActionIMP2Keys        },
    { STD_ARRAYLEN(aActionIMP3Keys)       , aActionIMP3Keys        },
    { STD_ARRAYLEN(aActionIMP4Keys)       , aActionIMP4Keys        },
    { STD_ARRAYLEN(aActionIMP5Keys)       , aActionIMP5Keys        },
    { STD_ARRAYLEN(aActionChalk)          , aActionChalk           }
};

// Joystick 2 Axis 4 buttons default keysets

static const size_t aActionCrawlJoyButtons[2]     = { DIK_C, STDCONTROL_JOYSTICK_GETBUTTON(0, 2) };
static const size_t aActionInterfaceJoyButtons[2] = { DIK_I, STDCONTROL_JOYSTICK_GETBUTTON(0, 3) };

static const JonesConfigControlAction jonesConfig_aDfault2Axis4Button[JONESCONTROL_ACTION_NUMACTIONS] =
{
    { STD_ARRAYLEN(aActionWalkFwdKeys)        , aActionWalkFwdKeys },
    { STD_ARRAYLEN(aActionWalkBackKeys)       , aActionWalkBackKeys },
    { STD_ARRAYLEN(aActionTurnLeftKeys)       , aActionTurnLeftKeys },
    { STD_ARRAYLEN(aActionTurnRightKeys)      , aActionTurnRightKeys },
    { STD_ARRAYLEN(aActionStepLeftKeys)       , aActionStepLeftKeys },
    { STD_ARRAYLEN(aActionStepRightKeys)      , aActionStepRightKeys },
    { STD_ARRAYLEN(aActionCrawlJoyButtons)    , aActionCrawlJoyButtons },
    { STD_ARRAYLEN(aActionRunKeys)            , aActionRunKeys },
    { STD_ARRAYLEN(aActionRollKeys)           , aActionRollKeys },
    { STD_ARRAYLEN(aActionJumpKeys)           , aActionJumpKeys },
    { STD_ARRAYLEN(aActionLookKeys)           , aActionLookKeys },
    { STD_ARRAYLEN(aActionActivateKeys)       , aActionActivateKeys },
    { STD_ARRAYLEN(aActionToggleMapKeys)      , aActionToggleMapKeys },
    { STD_ARRAYLEN(aActionPrevWeaponKeys)     , aActionPrevWeaponKeys },
    { STD_ARRAYLEN(aActionNextWeaponKeys)     , aActionNextWeaponKeys },
    { STD_ARRAYLEN(aActionToggleLightKeys)    , aActionToggleLightKeys },
    { STD_ARRAYLEN(aActionFistsKeys)          , aActionFistsKeys },
    { STD_ARRAYLEN(aActionWhipKeys)           , aActionWhipKeys },
    { STD_ARRAYLEN(aActionMauserKeys)         , aActionMauserKeys },
    { STD_ARRAYLEN(aActionPPSH41Keys)         , aActionPPSH41Keys },
    { STD_ARRAYLEN(aActionPistolKeys)         , aActionPistolKeys },
    { STD_ARRAYLEN(aActionSimonovKeys)        , aActionSimonovKeys },
    { STD_ARRAYLEN(aActionTokarevKeys)        , aActionTokarevKeys },
    { STD_ARRAYLEN(aActionToz34Keys)          , aActionToz34Keys },
    { STD_ARRAYLEN(aActionBazookaKeys)        , aActionBazookaKeys },
    { STD_ARRAYLEN(aActionMacheteKeys)        , aActionMacheteKeys },
    { STD_ARRAYLEN(aActionSatchelKeys)        , aActionSatchelKeys },
    { STD_ARRAYLEN(aActionGrenadeKeys)        , aActionGrenadeKeys },
    { STD_ARRAYLEN(aActionMapKeys)            , aActionMapKeys },
    { STD_ARRAYLEN(aActionInterfaceJoyButtons), aActionInterfaceJoyButtons },
    { STD_ARRAYLEN(aActionHealthKeys)         , aActionHealthKeys },
    { STD_ARRAYLEN(aActionIMP1Keys)           , aActionIMP1Keys },
    { STD_ARRAYLEN(aActionIMP2Keys)           , aActionIMP2Keys },
    { STD_ARRAYLEN(aActionIMP3Keys)           , aActionIMP3Keys },
    { STD_ARRAYLEN(aActionIMP4Keys)           , aActionIMP4Keys },
    { STD_ARRAYLEN(aActionIMP5Keys)           , aActionIMP5Keys },
    { STD_ARRAYLEN(aActionChalk)              , aActionChalk }
};

// Joystick 2 Axis 8 buttons default keysets

static const size_t aActionRollJoyButtons[2]         = { DIK_CAPITAL, STDCONTROL_JOYSTICK_GETBUTTON(0, 4) };
static const size_t aActionNextWeaponJoyButtons[2]   = { DIK_PERIOD, STDCONTROL_JOYSTICK_GETBUTTON(0, 5) };
static const size_t aActionToggleWeaponJoyButtons[2] = { DIK_SPACE, STDCONTROL_JOYSTICK_GETBUTTON(0, 6) };
static const size_t aActionLookJoyButtons[3]         = { DIK_INSERT, DIK_NUMPAD0, STDCONTROL_JOYSTICK_GETBUTTON(0, 7) };

static const JonesConfigControlAction jonesConfig_aDefault2Axis8Button[JONESCONTROL_ACTION_NUMACTIONS] =
{
    { STD_ARRAYLEN(aActionWalkFwdKeys)           , aActionWalkFwdKeys            },
    { STD_ARRAYLEN(aActionWalkBackKeys)          , aActionWalkBackKeys           },
    { STD_ARRAYLEN(aActionTurnLeftKeys)          , aActionTurnLeftKeys           },
    { STD_ARRAYLEN(aActionTurnRightKeys)         , aActionTurnRightKeys          },
    { STD_ARRAYLEN(aActionStepLeftKeys)          , aActionStepLeftKeys           },
    { STD_ARRAYLEN(aActionStepRightKeys)         , aActionStepRightKeys          },
    { STD_ARRAYLEN(aActionCrawlJoyButtons)       , aActionCrawlJoyButtons        },
    { STD_ARRAYLEN(aActionRunKeys)               , aActionRunKeys                },
    { STD_ARRAYLEN(aActionRollJoyButtons)        , aActionRollJoyButtons         },
    { STD_ARRAYLEN(aActionJumpKeys)              , aActionJumpKeys               },
    { STD_ARRAYLEN(aActionLookJoyButtons)        , aActionLookJoyButtons         },
    { STD_ARRAYLEN(aActionActivateKeys)          , aActionActivateKeys           },
    { STD_ARRAYLEN(aActionToggleWeaponJoyButtons), aActionToggleWeaponJoyButtons },
    { STD_ARRAYLEN(aActionPrevWeaponKeys)        , aActionPrevWeaponKeys         },
    { STD_ARRAYLEN(aActionNextWeaponJoyButtons)  , aActionNextWeaponJoyButtons   },
    { STD_ARRAYLEN(aActionToggleLightKeys)       , aActionToggleLightKeys        },
    { STD_ARRAYLEN(aActionFistsKeys)             , aActionFistsKeys              },
    { STD_ARRAYLEN(aActionWhipKeys)              , aActionWhipKeys               },
    { STD_ARRAYLEN(aActionMauserKeys)            , aActionMauserKeys             },
    { STD_ARRAYLEN(aActionPPSH41Keys)            , aActionPPSH41Keys             },
    { STD_ARRAYLEN(aActionPistolKeys)            , aActionPistolKeys             },
    { STD_ARRAYLEN(aActionSimonovKeys)           , aActionSimonovKeys            },
    { STD_ARRAYLEN(aActionTokarevKeys)           , aActionTokarevKeys            },
    { STD_ARRAYLEN(aActionToz34Keys)             , aActionToz34Keys              },
    { STD_ARRAYLEN(aActionBazookaKeys)           , aActionBazookaKeys            },
    { STD_ARRAYLEN(aActionMacheteKeys)           , aActionMacheteKeys            },
    { STD_ARRAYLEN(aActionSatchelKeys)           , aActionSatchelKeys            },
    { STD_ARRAYLEN(aActionGrenadeKeys)           , aActionGrenadeKeys            },
    { STD_ARRAYLEN(aActionMapKeys)               , aActionMapKeys                },
    { STD_ARRAYLEN(aActionInterfaceJoyButtons)   , aActionInterfaceJoyButtons    },
    { STD_ARRAYLEN(aActionHealthKeys)            , aActionHealthKeys             },
    { STD_ARRAYLEN(aActionIMP1Keys)              , aActionIMP1Keys               },
    { STD_ARRAYLEN(aActionIMP2Keys)              , aActionIMP2Keys               },
    { STD_ARRAYLEN(aActionIMP3Keys)              , aActionIMP3Keys               },
    { STD_ARRAYLEN(aActionIMP4Keys)              , aActionIMP4Keys               },
    { STD_ARRAYLEN(aActionIMP5Keys)              , aActionIMP5Keys               },
    { STD_ARRAYLEN(aActionChalk)                 , aActionChalk                  }
};

// Side Winder Gamepad default keysets found in debug version

static const size_t aActionToggleStepLeftSideWinderGamePadButtons[2]  = { DIK_LBRACKET, STDCONTROL_JOYSTICK_GETBUTTON(0, 6) };
static const size_t aActionToggleStepRightSideWinderGamePadButtons[2] = { DIK_RBRACKET, STDCONTROL_JOYSTICK_GETBUTTON(0, 7) };
static const size_t aActionToggleCrawlSideWinderGamePadButtons[2]     = { DIK_C       , STDCONTROL_JOYSTICK_GETBUTTON(0, 2) };
static const size_t aActionToggleRunSideWinderGamePadButtons[2]       = { DIK_CAPITAL , STDCONTROL_JOYSTICK_GETBUTTON(0, 3) };
static const size_t aActionLookSideWinderGamePadButtons[3]            = { DIK_INSERT  , DIK_NUMPAD0, STDCONTROL_JOYSTICK_GETBUTTON(0, 5) };
static const size_t aActionToggleWeaponSideWinderGamePadButtons[2]    = { DIK_SPACE   , STDCONTROL_JOYSTICK_GETBUTTON(0, 9) };
static const size_t aActionNextWeaponSideWinderGamePad[2]             = { DIK_PERIOD  , STDCONTROL_JOYSTICK_GETBUTTON(0, 4) };
static const size_t aActionToggleInterfaceSideWinderGamePadButtons[2] = { DIK_I       , STDCONTROL_JOYSTICK_GETBUTTON(0, 8) };

static const JonesConfigControlAction jonesConfig_aDefaultSideWinderGamepad[JONESCONTROL_ACTION_NUMACTIONS] =
{
    { STD_ARRAYLEN(aActionWalkFwdKeys)                            , aActionWalkFwdKeys    },
    { STD_ARRAYLEN(aActionWalkBackKeys)                           , aActionWalkBackKeys  },
    { STD_ARRAYLEN(aActionTurnLeftKeys)                           , aActionTurnLeftKeys  },
    { STD_ARRAYLEN(aActionTurnRightKeys)                          , aActionTurnRightKeys },
    { STD_ARRAYLEN(aActionToggleStepLeftSideWinderGamePadButtons) , aActionToggleStepLeftSideWinderGamePadButtons},
    { STD_ARRAYLEN(aActionToggleStepRightSideWinderGamePadButtons), aActionToggleStepRightSideWinderGamePadButtons},
    { STD_ARRAYLEN(aActionToggleCrawlSideWinderGamePadButtons)    , aActionToggleCrawlSideWinderGamePadButtons},
    { STD_ARRAYLEN(aActionToggleRunSideWinderGamePadButtons)      , aActionToggleRunSideWinderGamePadButtons},
    { STD_ARRAYLEN(aActionRollKeys)                               , aActionRollKeys      },
    { STD_ARRAYLEN(aActionJumpKeys)                               , aActionJumpKeys      },
    { STD_ARRAYLEN(aActionLookSideWinderGamePadButtons)           , aActionLookSideWinderGamePadButtons},
    { STD_ARRAYLEN(aActionActivateKeys)                           , aActionActivateKeys  },
    { STD_ARRAYLEN(aActionToggleWeaponSideWinderGamePadButtons)   , aActionToggleWeaponSideWinderGamePadButtons},
    { STD_ARRAYLEN(aActionPrevWeaponKeys)                         , aActionPrevWeaponKeys},
    { STD_ARRAYLEN(aActionNextWeaponSideWinderGamePad)            , aActionNextWeaponSideWinderGamePad},
    { STD_ARRAYLEN(aActionToggleLightKeys)                        , aActionToggleLightKeys},
    { STD_ARRAYLEN(aActionFistsKeys)                              , aActionFistsKeys    },
    { STD_ARRAYLEN(aActionWhipKeys)                               , aActionWhipKeys     },
    { STD_ARRAYLEN(aActionMauserKeys)                             , aActionMauserKeys   },
    { STD_ARRAYLEN(aActionPPSH41Keys)                             , aActionPPSH41Keys   },
    { STD_ARRAYLEN(aActionPistolKeys)                             , aActionPistolKeys   },
    { STD_ARRAYLEN(aActionSimonovKeys)                            , aActionSimonovKeys  },
    { STD_ARRAYLEN(aActionTokarevKeys)                            , aActionTokarevKeys  },
    { STD_ARRAYLEN(aActionToz34Keys)                              , aActionToz34Keys    },
    { STD_ARRAYLEN(aActionBazookaKeys)                            , aActionBazookaKeys  },
    { STD_ARRAYLEN(aActionMacheteKeys)                            , aActionMacheteKeys  },
    { STD_ARRAYLEN(aActionSatchelKeys)                            , aActionSatchelKeys  },
    { STD_ARRAYLEN(aActionGrenadeKeys)                            , aActionGrenadeKeys  },
    { STD_ARRAYLEN(aActionMapKeys)                                , aActionMapKeys      },
    { STD_ARRAYLEN(aActionToggleInterfaceSideWinderGamePadButtons), aActionToggleInterfaceSideWinderGamePadButtons},
    { STD_ARRAYLEN(aActionHealthKeys)                             , aActionHealthKeys   },
    { STD_ARRAYLEN(aActionIMP1Keys)                               , aActionIMP1Keys     },
    { STD_ARRAYLEN(aActionIMP2Keys)                               , aActionIMP2Keys     },
    { STD_ARRAYLEN(aActionIMP3Keys)                               , aActionIMP3Keys     },
    { STD_ARRAYLEN(aActionIMP4Keys)                               , aActionIMP4Keys     },
    { STD_ARRAYLEN(aActionIMP5Keys)                               , aActionIMP5Keys     },
    { STD_ARRAYLEN(aActionChalk)                                  , aActionChalk        }
};

// Default key sets
static const JonesConfigControlAction* jonesConfig_aDfltKeySets[] = {
    jonesConfig_aDefaultControlSet,
    jonesConfig_aDfault2Axis4Button,
    jonesConfig_aDefault2Axis8Button
};

#define JONESCONFIG_NUMDEFAULTCONTROLSCHEMES STD_ARRAYLEN(jonesConfig_aDfltKeySets)

static const char* jonesConfig_aDfltKeySetNames[JONESCONFIG_NUMDEFAULTCONTROLSCHEMES] = {
    "JONES_STR_DFLTKEY",
    "JONES_STR_DFLT2A4B",
    "JONES_STR_DFLT2A8B"
};

// The action string names
static const char* jonesConfig_aJonesControlActionNames[JONESCONTROL_ACTION_NUMACTIONS] =
{
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

static const char* jonesConfig_aJonesCapControlActionNames[JONESCONTROL_ACTION_NUMACTIONS] =
{
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

// Key control sting names
static const char* jonesConfig_aControlKeyStrings[223] =
{
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
    "Y",
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

int jonesConfig_InitKeySetsPath(void);
int jonesConfig_InitKeyActions(void);

void J3DAPI jonesConfig_ControlToString(size_t controlId, char* pDest);

JonesControlsScheme* jonesConfig_LoadActiveControlScheme(void);
int J3DAPI jonesConfig_LoadControlScheme(const char* pFilePath, JonesControlsScheme* pConfig);
int J3DAPI jonesConfig_ParseArgumentIndex(size_t* argNum, const StdConffileEntry* pEntry, size_t a3, const char** apStringList, size_t size);
int J3DAPI jonesConfig_GetStringListIndex(char* pValue, const char** apStringList, size_t size);
int J3DAPI jonesConfig_ParseJoystickControlId(size_t* pArgNum, const StdConffileEntry* pEntry);
int J3DAPI jonesConfig_SetDefaultControlScheme(JonesControlsScheme* pScheme, size_t num);
void J3DAPI jonesConfig_BindControls(JonesControlsScheme* pConfig);
void J3DAPI jonesConfig_BindJoystickControl(SithControlFunction functionId, size_t controlId);
void J3DAPI jonesConfig_FreeControlScheme(JonesControlsScheme* pScheme);
void J3DAPI jonesConfig_FreeControlConfigEntry(JonesControlsConfig* pConfig);

HFONT J3DAPI jonesConfig_InitDialog(HWND hWnd, HFONT hFont, int dlgID);
HFONT J3DAPI jonesConfig_CreateDialogFont(HWND hWnd, int bWindowMode, int dlgResNum, float* pFontScale);
void J3DAPI jonesConfig_ResetDialogFont(HWND hWndParent, HFONT hFont);
BOOL CALLBACK jonesConfig_ResetWindowFontCallback(HWND hWnd, LPARAM lparam);
BOOL CALLBACK jonesConfig_SetPositionAndTextCallback(HWND hCtrl, LPARAM lparam);
void J3DAPI jonesConfig_SetWindowFontAndPosition(HWND hCtrl, JonesDialogFontInfo* pFontInfo);
void J3DAPI jonesConfig_GetWindowScreenRect(HWND hWnd, LPRECT lpRect);
void J3DAPI jonesConfig_SetDialogTitleAndPosition(HWND hWnd, JonesDialogFontInfo* pDlgFontInfo);

UINT_PTR CALLBACK jonesConfig_SaveGameDialogHookProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam);
int J3DAPI jonesConfig_SaveGameDialogInit(HWND hDlg, int a2, LPOPENFILENAME lpOfn);
LRESULT CALLBACK jonesConfig_SaveGameThumbnailPaintProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
int J3DAPI jonesConfig_ShowOverwriteSaveGameDlg(HWND hWnd, const char* aFilePath);
INT_PTR CALLBACK jonesConfig_SaveGameMsgBoxProc(HWND hWnd, UINT umsg, WPARAM wParam, LPARAM lParam);
int J3DAPI jonesConfig_GameSaveSetData(HWND hDlg, int a2, GameSaveMsgBoxData* pData);

INT_PTR CALLBACK jonesConfig_ExitGameDlgProc(HWND hWnd, UINT uMsg, WPARAM wparam, LPARAM lparam);

void* J3DAPI jonesConfig_sub_405F60(HWND hWnd);
UINT_PTR CALLBACK jonesConfig_LoadGameDialogHookProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
int J3DAPI jonesConfig_ShowLoadGameWarningMsgBox(HWND hWnd);
BOOL J3DAPI jonesConfig_LoadGameDialogInit(HWND hDlg, int a2, LPOPENFILENAME pofn);
LRESULT CALLBACK jonesConfig_LoadGameThumbnailPaintProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

int J3DAPI jonesConfig_ShowCreateControlSchemeDialog(HWND hWnd, JonesCreateControlSchemeDialogData* pData);
INT_PTR CALLBACK jonesConfig_CreateControlSchemeProc(HWND hWnd, UINT msg, WPARAM wparam, LPARAM lparam);
int J3DAPI jonesConfig_InitCreateControlScheme(HWND hDlg, int a2, JonesCreateControlSchemeDialogData* pData);
void J3DAPI jonesConfig_CreateControlScheme_Handle_WM_COMMAND(HWND hWnd, int nResult);

int J3DAPI jonesConfig_ShowEditControlShemeDialog(HWND hWnd, JonesEditControlSchemeDialogData* pData);
INT_PTR CALLBACK jonesConfig_EditControlSchemeProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
int J3DAPI jonesConfig_InitEditControlSchemeDlg(HWND hDlg, int a2, JonesEditControlSchemeDialogData* pData);
void J3DAPI jonesConfig_EditControlScheme_SetListColumn(HWND hWnd, const char* pJSText);
void J3DAPI jonesConfig_EditControlScheme_SetListScheme(HWND hWnd, int a2, JonesControlsScheme* pScheme);
void J3DAPI jonesConfig_EditControlScheme_Handle_WM_COMMAND(HWND hWnd, int ctrlID);
void J3DAPI jonesConfig_EditControlScheme_EditAssignment(HWND hDlg, HWND hListCtrl, int listID, JonesControlsScheme* pScheme);
int J3DAPI jonesConfig_EditControlScheme_GetSelectedListItemBinding(HWND hDlg, HWND hListCtrl, JonesControlsScheme* pScheme, size_t* pItemIdx, size_t* pBindIdx);
void J3DAPI jonesConfig_EditControlScheme_AddAssignment(HWND hDlg, HWND hListCtrl, int listID, JonesControlsScheme* pScheme);
void J3DAPI jonesConfig_EditControlScheme_DeleteAssignment(HWND hDlg, HWND hListCtrl, int listID, JonesControlsScheme* pScheme);

int J3DAPI jonesConfig_ShowAssignKeyDialog(HWND hWnd, JonesAssignKeyDialogData* pData);
int J3DAPI jonesConfig_AssignKey_CheckBindForKey(JonesControlsScheme* pScheme, size_t keyControlID, size_t* pFunctionId, int listID, size_t* pOffset);
INT_PTR CALLBACK jonesConfig_AssignKeyDlgProc(HWND hWnd, UINT msg, WPARAM wparam, LPARAM lparam);
int J3DAPI jonesConfig_InitAssignKeyDlg(HWND hDlg, int a2, JonesAssignKeyDialogData* pData);
void J3DAPI jonesConfig_AssignControlKey_ReadKey(HWND hWnd);
int J3DAPI jonesConfig_AssignControlKey_ReadJoyAxisKey(JonesAssignKeyDialogData* pData);
void J3DAPI jonesConfig_AssignKeyDlg_HandleWM_COMMAND(HWND hWnd, int ctrlID);

int J3DAPI jonesConfig_ShowReassignKeyMsgBox(HWND hWnd, JonesReAssignKeyDialogData* pData);
INT_PTR CALLBACK jonesConfig_ReassignKeyDialogProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
int J3DAPI jonesConfig_InitReassignKeyDialog(HWND hDlg, int a2, JonesReAssignKeyDialogData* pData);
void J3DAPI jonesConfig_ReassignKeyDlg_HandleWM_COMMAND(HWND hWnd, unsigned int cntrlId);

BOOL CALLBACK jonesConfig_MessageDialogProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
void J3DAPI jonesConfig_InitMessageDialogIcon(HWND hWnd, JonesMessageDialogData* pData);
int J3DAPI jonesConfig_InitMessageDialogData(HWND hDlg, int a2, JonesMessageDialogData* pData);
int J3DAPI jonesConfig_SetTextControl(HWND hDlg, HWND hWnd, const char* aText);
void J3DAPI jonesConfig_MessageDialog_HandleWM_COMMAND(HWND hDlg, int nResult);

void J3DAPI jonesConfig_MsgBoxDlg_HandleWM_COMMAND(HWND hWnd, int nResult);

INT_PTR CALLBACK jonesConfig_GamePlayOptionsProc(HWND hWnd, UINT msg, WPARAM wparam, LPARAM lparam);
void J3DAPI jonesConfig_HandleWM_HSCROLL(HWND hDlg, HWND hWnd, uint16_t sbValue);
int J3DAPI jonesConfig_GamePlayOptionsInitDlg(HWND hDlg);
void J3DAPI jonesConfig_GamePlayOptions_HandleWM_COMMAND(HWND hDlg, uint16_t controlID);

int J3DAPI jonesConfig_CopyControlConfig(JonesControlsConfig* pSrc, JonesControlsConfig* pDst);
JonesControlsScheme* J3DAPI jonesConfig_CloneControlSchemesList(JonesControlsScheme* aSchemes, size_t start, size_t numSchemes);

int jonesConfig_ControlOptions_InitControlsConfig(void);
int J3DAPI jonesConfig_ControlOptions_LoadAllControlSchemes(JonesControlsConfig* pConfig);
int J3DAPI jonesConfig_ControlOptions_SetAllDefaultControlSchemes(JonesControlsConfig* pConfig);
INT_PTR CALLBACK jonesConfig_ControlOptionsDialogProc(HWND hwnd, UINT umsg, WPARAM wparam, LPARAM lparam);
int J3DAPI jonesConfig_InitControlOptionsDialog(HWND hDlg, int a2, JonesControlsConfig* pConfig);
void J3DAPI jonesConfig_ControlOptions_HandleWM_COMMAND(HWND hWnd, int ctrlID, LPARAM lParam, unsigned int notificationCode);
int J3DAPI jonesConfig_ControlOptions_WriteScheme(JonesControlsScheme* pScheme);
int J3DAPI jonesConfig_ControlOptions_UpdateButtons(HWND hDlg);
void J3DAPI jonesConfig_ControlOptions_DeleteScheme(HWND hDlg, JonesControlsConfig* pConfig);
void J3DAPI jonesConfig_ControlOptions_EditSelectedScheme(HWND hDlg, JonesControlsConfig* a2);
JonesControlsScheme* J3DAPI jonesConfig_GetDefaultControlScheme(JonesControlsConfig* pConfig);
void J3DAPI jonesConfig_ControlOptions_CreateNewScheme(HWND hDlg, JonesControlsConfig* pConfig, JonesControlsScheme* pNewScheme);

INT_PTR CALLBACK jonesConfig_DisplaySettingsDialogProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
int J3DAPI jonesConfig_InitDisplaySettingsDialog(HWND hDlg, int a2, JonesDisplaySettingsDialogData* pData);
int J3DAPI jonesConfig_DisplaySettings_GetVideoModeNum(const JonesDisplaySettingsVideoMode* pVideoMode, const char* pText, size_t numVideoModes);
void J3DAPI jonesConfig_DisplaySettings_HandleWM_COMMAND(HWND hWnd, int ctrlID, LPARAM a3, int notifyCode);

int J3DAPI jonesConfig_ShowAdvanceDisplaySettings(HWND hWnd, JonesDisplaySettingsDialogData* pData);
INT_PTR CALLBACK jonesConfig_AdvanceDisplaySettingsDialogProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
int J3DAPI jonesConfig_DisplaySettings_Get3DDeviceSupportsBPP(const StdDisplayInfo* pDisplayInfo, const JonesDisplaySettings* pSettings, int bpp);
int J3DAPI jonesConfig_InitAdvanceDisplaySettingsDialog(HWND hDlg, int a2, JonesDisplaySettingsDialogData* pData);
void J3DAPI jonesConfig_AdvanceDisplaySettings_HandleWM_COMMAND(HWND hDlg, int ctrlID, int a3, int notifyCode);

INT_PTR CALLBACK jonesConfig_SoundSettingsDialogProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
int J3DAPI jonesConfig_InitSoundSettingsDialog(HWND hDlg, int a2, JonesSoundSettings* pData);
void J3DAPI jonesConfig_SoundSettings_HandleWM_COMMAND(HWND hWnd, int ctrlID, int notifyCode);

INT_PTR CALLBACK jonesConfig_GameOverDialogProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
void J3DAPI jonesConfig_LoadGameGetLastSavedGamePath(char* pPath, unsigned int size);
int J3DAPI jonesConfig_GameOverDialogInit(HWND hDlg, int a2, GameOverDialogData* pData);
void J3DAPI jonesConfig_GameOverDialog_HandleWM_COMMAND(HWND hWnd, uint16_t ctrlID);

INT_PTR CALLBACK jonesConfig_StatisticsDialogProc(HWND hwnd, UINT uMsg, WPARAM wPAram, LPARAM lParam);
int J3DAPI jonesConfig_sub_40D100(int a1, HWND hWnd, int a3, int a4);
int J3DAPI jonesConfig_DrawImageOnDialogItem(HWND hDlg, HDC hdcWnd, HDC hdcCtrl, int nIDDlgItem, HBITMAP hImage, HBITMAP hMask);
int J3DAPI jonesConfig_SetStatisticsDialogForLevel(HWND hDlg, int levelNum, int* a3);
void J3DAPI jonesConfig_DrawStatisticDialogIQPoints(HWND hwnd, JonesDialogImageInfo** ppImageInfo, int dlgID, int iqpoints);
int J3DAPI jonesConfig_InitStatisticDialog(HWND hDlg, int a2, int* pData);
void J3DAPI jonesConfig_StatisticProc_HandleWM_COMMAND(HWND hWnd, int16_t wParam);

int CALLBACK jonesConfig_LevelCompletedDialogProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
int J3DAPI jonesConfig_InitLevelCompletedDialog(HWND hDlg, int wParam, tLevelCompletedDialogState* pState);
void J3DAPI jonesConfig_ChapterCompleteDialog_HandleWM_COMMAND(HWND hWnd, int wParam);

int J3DAPI jonesConfig_ShowStoreDialog(HWND hWnd, int* pBalance, int* pItemsState, int a4);
int CALLBACK jonesConfig_StoreDialogProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);
int J3DAPI jonesConfig_StoreHandleDragEvent(HWND hwnd, HWND hDlgCtrl);
void J3DAPI jonesConfig_StoreDialog_HandleWM_MBUTTONUP(HWND hWnd);
BOOL J3DAPI jonesConfig_StoreDialog_HandleWM_MOUSEFIRST(HWND hWnd);
int J3DAPI jonesConfig_InitStoreDialog(HWND hDlg, int a2, tStoreCartState* pCart);
int J3DAPI jonesConfig_StoreInitItemIcons(HWND hWnd, tStoreCartState* pCart);
void J3DAPI jonesConfig_StoreSetListColumns(HWND hList, const char* pColumnName);
void J3DAPI jonesConfig_StoreInitItemList(HWND hWnd, int* apItemsState, int listID);
void J3DAPI jonesConfig_UpdateBalances(HWND hDlg, tStoreCartState* pCart);
void J3DAPI jonesConfig_AddStoreCartItem(HWND hDlg, tStoreCartState* pCart);
void J3DAPI jonesConfig_RemoveStoreCartItem(HWND hDlg, tStoreCartState* pCart);
void J3DAPI jonesConfig_StoreDialog_HandleWM_COMMAND(HWND hWnd, WPARAM wParam);
void J3DAPI jonesConfig_ClearStoreCart(HWND hDlg, tStoreCartState* pCart);

int J3DAPI jonesConfig_ShowPurchaseMessageBox(HWND hWnd, tStoreCartState* dwInitParam);
INT_PTR CALLBACK jonesConfig_PurchaseMessageBoxProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
int J3DAPI jonesConfig_InitPurchaseMessageBox(HWND hDlg, int a2, tStoreCartState* pCart);

INT_PTR CALLBACK jonesConfig_DialogInsertCDProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
int J3DAPI jonesConfig_InitDialogInsertCD(HWND hDlg, int a2, int cdNum);
int J3DAPI jonesConfig_InsertCD_HandleWM_COMMAND(HWND hWnd, int nResult);

void jonesConfig_InstallHooks(void)
{
    J3D_HOOKFUNC(jonesConfig_Startup);
    J3D_HOOKFUNC(jonesConfig_InitKeySetsPath);
    J3D_HOOKFUNC(jonesConfig_InitKeyActions);
    J3D_HOOKFUNC(jonesConfig_Shutdown);
    J3D_HOOKFUNC(jonesConfig_ControlToString);
    J3D_HOOKFUNC(jonesConfig_ShowMessageDialog);
    J3D_HOOKFUNC(jonesConfig_MessageDialogProc);
    J3D_HOOKFUNC(jonesConfig_InitMessageDialogIcon);
    J3D_HOOKFUNC(jonesConfig_InitMessageDialogData);
    J3D_HOOKFUNC(jonesConfig_SetTextControl);
    J3D_HOOKFUNC(jonesConfig_MessageDialog_HandleWM_COMMAND);
    J3D_HOOKFUNC(jonesConfig_LoadActiveControlScheme);
    J3D_HOOKFUNC(jonesConfig_LoadControlScheme);
    J3D_HOOKFUNC(jonesConfig_ParseArgumentIndex);
    J3D_HOOKFUNC(jonesConfig_GetStringListIndex);
    J3D_HOOKFUNC(jonesConfig_ParseJoystickControlId);
    J3D_HOOKFUNC(jonesConfig_SetDefaultControlScheme);
    J3D_HOOKFUNC(jonesConfig_BindControls);
    J3D_HOOKFUNC(jonesConfig_BindJoystickControl);
    J3D_HOOKFUNC(jonesConfig_InitDialog);
    J3D_HOOKFUNC(jonesConfig_CreateDialogFont);
    J3D_HOOKFUNC(jonesConfig_ResetDialogFont);
    J3D_HOOKFUNC(jonesConfig_ResetWindowFontCallback);
    J3D_HOOKFUNC(jonesConfig_SetPositionAndTextCallback);
    J3D_HOOKFUNC(jonesConfig_SetWindowFontAndPosition);
    J3D_HOOKFUNC(jonesConfig_GetWindowScreenRect);
    J3D_HOOKFUNC(jonesConfig_SetDialogTitleAndPosition);
    J3D_HOOKFUNC(jonesConfig_GetSaveGameFilePath);
    J3D_HOOKFUNC(jonesConfig_SaveGameDialogHookProc);
    J3D_HOOKFUNC(jonesConfig_SaveGameDialogInit);
    J3D_HOOKFUNC(jonesConfig_SaveGameThumbnailPaintProc);
    J3D_HOOKFUNC(jonesConfig_ShowOverwriteSaveGameDlg);
    J3D_HOOKFUNC(jonesConfig_ShowExitGameDialog);
    J3D_HOOKFUNC(jonesConfig_ExitGameDlgProc);
    J3D_HOOKFUNC(jonesConfig_SaveGameMsgBoxProc);
    J3D_HOOKFUNC(jonesConfig_GameSaveSetData);
    J3D_HOOKFUNC(jonesConfig_MsgBoxDlg_HandleWM_COMMAND);
    J3D_HOOKFUNC(jonesConfig_GetLoadGameFilePath);
    J3D_HOOKFUNC(jonesConfig_sub_405F60);
    J3D_HOOKFUNC(jonesConfig_LoadGameDialogHookProc);
    J3D_HOOKFUNC(jonesConfig_ShowLoadGameWarningMsgBox);
    J3D_HOOKFUNC(jonesConfig_LoadGameDialogInit);
    J3D_HOOKFUNC(jonesConfig_LoadGameThumbnailPaintProc);
    J3D_HOOKFUNC(jonesConfig_ShowGamePlayOptions);
    J3D_HOOKFUNC(jonesConfig_GamePlayOptionsProc);
    J3D_HOOKFUNC(jonesConfig_HandleWM_HSCROLL);
    J3D_HOOKFUNC(jonesConfig_GamePlayOptionsInitDlg);
    J3D_HOOKFUNC(jonesConfig_GamePlayOptions_HandleWM_COMMAND);
    J3D_HOOKFUNC(jonesConfig_EnableMouseControl);
    J3D_HOOKFUNC(jonesConfig_FreeControlScheme);
    J3D_HOOKFUNC(jonesConfig_FreeControlConfigEntry);
    J3D_HOOKFUNC(jonesConfig_ShowControlOptions);
    J3D_HOOKFUNC(jonesConfig_CopyControlConfig);
    J3D_HOOKFUNC(jonesConfig_CloneControlSchemesList);
    J3D_HOOKFUNC(jonesConfig_ControlOptions_InitControlsConfig);
    J3D_HOOKFUNC(jonesConfig_ControlOptions_LoadAllControlSchemes);
    J3D_HOOKFUNC(jonesConfig_ControlOptions_SetAllDefaultControlSchemes);
    J3D_HOOKFUNC(jonesConfig_ControlOptionsDialogProc);
    J3D_HOOKFUNC(jonesConfig_InitControlOptionsDialog);
    J3D_HOOKFUNC(jonesConfig_ControlOptions_HandleWM_COMMAND);
    J3D_HOOKFUNC(jonesConfig_ControlOptions_WriteScheme);
    J3D_HOOKFUNC(jonesConfig_ControlOptions_UpdateButtons);
    J3D_HOOKFUNC(jonesConfig_ControlOptions_DeleteScheme);
    J3D_HOOKFUNC(jonesConfig_ControlOptions_EditSelectedScheme);
    J3D_HOOKFUNC(jonesConfig_GetDefaultControlScheme);
    J3D_HOOKFUNC(jonesConfig_ControlOptions_CreateNewScheme);
    J3D_HOOKFUNC(jonesConfig_ShowCreateControlSchemeDialog);
    J3D_HOOKFUNC(jonesConfig_CreateControlSchemeProc);
    J3D_HOOKFUNC(jonesConfig_InitCreateControlScheme);
    J3D_HOOKFUNC(jonesConfig_CreateControlScheme_Handle_WM_COMMAND);
    J3D_HOOKFUNC(jonesConfig_ShowEditControlShemeDialog);
    J3D_HOOKFUNC(jonesConfig_EditControlSchemeProc);
    J3D_HOOKFUNC(jonesConfig_InitEditControlSchemeDlg);
    J3D_HOOKFUNC(jonesConfig_EditControlScheme_SetListColumn);
    J3D_HOOKFUNC(jonesConfig_EditControlScheme_SetListScheme);
    J3D_HOOKFUNC(jonesConfig_EditControlScheme_Handle_WM_COMMAND);
    J3D_HOOKFUNC(jonesConfig_EditControlScheme_EditAssignment);
    J3D_HOOKFUNC(jonesConfig_EditControlScheme_GetSelectedListItemBinding);
    J3D_HOOKFUNC(jonesConfig_EditControlScheme_AddAssignment);
    J3D_HOOKFUNC(jonesConfig_EditControlScheme_DeleteAssignment);
    J3D_HOOKFUNC(jonesConfig_ShowAssignKeyDialog);
    J3D_HOOKFUNC(jonesConfig_AssignKey_CheckBindForKey);
    J3D_HOOKFUNC(jonesConfig_AssignKeyDlgProc);
    J3D_HOOKFUNC(jonesConfig_InitAssignKeyDlg);
    J3D_HOOKFUNC(jonesConfig_AssignControlKey_ReadKey);
    J3D_HOOKFUNC(jonesConfig_AssignControlKey_ReadJoyAxisKey);
    J3D_HOOKFUNC(jonesConfig_AssignKeyDlg_HandleWM_COMMAND);
    J3D_HOOKFUNC(jonesConfig_ShowReassignKeyMsgBox);
    J3D_HOOKFUNC(jonesConfig_ReassignKeyDialogProc);
    J3D_HOOKFUNC(jonesConfig_InitReassignKeyDialog);
    J3D_HOOKFUNC(jonesConfig_ReassignKeyDlg_HandleWM_COMMAND);
    J3D_HOOKFUNC(jonesConfig_ShowDisplaySettingsDialog);
    J3D_HOOKFUNC(jonesConfig_DisplaySettingsDialogProc);
    J3D_HOOKFUNC(jonesConfig_InitDisplaySettingsDialog);
    J3D_HOOKFUNC(jonesConfig_DisplaySettings_GetVideoModeNum);
    J3D_HOOKFUNC(jonesConfig_DisplaySettings_HandleWM_COMMAND);
    J3D_HOOKFUNC(jonesConfig_ShowAdvanceDisplaySettings);
    J3D_HOOKFUNC(jonesConfig_AdvanceDisplaySettingsDialogProc);
    J3D_HOOKFUNC(jonesConfig_DisplaySettings_Get3DDeviceSupportsBPP);
    J3D_HOOKFUNC(jonesConfig_InitAdvanceDisplaySettingsDialog);
    J3D_HOOKFUNC(jonesConfig_AdvanceDisplaySettings_HandleWM_COMMAND);
    J3D_HOOKFUNC(jonesConfig_ShowSoundSettingsDialog);
    J3D_HOOKFUNC(jonesConfig_SoundSettingsDialogProc);
    J3D_HOOKFUNC(jonesConfig_InitSoundSettingsDialog);
    J3D_HOOKFUNC(jonesConfig_SoundSettings_HandleWM_COMMAND);
    J3D_HOOKFUNC(jonesConfig_ShowGameOverDialog);
    J3D_HOOKFUNC(jonesConfig_GameOverDialogProc);
    J3D_HOOKFUNC(jonesConfig_LoadGameGetLastSavedGamePath);
    J3D_HOOKFUNC(jonesConfig_GameOverDialogInit);
    J3D_HOOKFUNC(jonesConfig_GameOverDialog_HandleWM_COMMAND);
    J3D_HOOKFUNC(jonesConfig_ShowStatisticsDialog);
    J3D_HOOKFUNC(jonesConfig_StatisticsDialogProc);
    J3D_HOOKFUNC(jonesConfig_sub_40D100);
    J3D_HOOKFUNC(jonesConfig_UpdateCurrentLevelNum);
    J3D_HOOKFUNC(jonesConfig_DrawImageOnDialogItem);
    J3D_HOOKFUNC(jonesConfig_SetStatisticsDialogForLevel);
    J3D_HOOKFUNC(jonesConfig_DrawStatisticDialogIQPoints);
    J3D_HOOKFUNC(jonesConfig_InitStatisticDialog);
    J3D_HOOKFUNC(jonesConfig_StatisticProc_HandleWM_COMMAND);
    J3D_HOOKFUNC(jonesConfig_ShowLevelCompletedDialog);
    J3D_HOOKFUNC(jonesConfig_LevelCompletedDialogProc);
    J3D_HOOKFUNC(jonesConfig_InitLevelCompletedDialog);
    J3D_HOOKFUNC(jonesConfig_ChapterCompleteDialog_HandleWM_COMMAND);
    J3D_HOOKFUNC(jonesConfig_ShowStoreDialog);
    J3D_HOOKFUNC(jonesConfig_StoreDialogProc);
    J3D_HOOKFUNC(jonesConfig_StoreHandleDragEvent);
    J3D_HOOKFUNC(jonesConfig_StoreDialog_HandleWM_MBUTTONUP);
    J3D_HOOKFUNC(jonesConfig_StoreDialog_HandleWM_MOUSEFIRST);
    J3D_HOOKFUNC(jonesConfig_InitStoreDialog);
    J3D_HOOKFUNC(jonesConfig_StoreInitItemIcons);
    J3D_HOOKFUNC(jonesConfig_StoreSetListColumns);
    J3D_HOOKFUNC(jonesConfig_StoreInitItemList);
    J3D_HOOKFUNC(jonesConfig_UpdateBalances);
    J3D_HOOKFUNC(jonesConfig_AddStoreCartItem);
    J3D_HOOKFUNC(jonesConfig_RemoveStoreCartItem);
    J3D_HOOKFUNC(jonesConfig_StoreDialog_HandleWM_COMMAND);
    J3D_HOOKFUNC(jonesConfig_ClearStoreCart);
    J3D_HOOKFUNC(jonesConfig_ShowPurchaseMessageBox);
    J3D_HOOKFUNC(jonesConfig_PurchaseMessageBoxProc);
    J3D_HOOKFUNC(jonesConfig_InitPurchaseMessageBox);
    J3D_HOOKFUNC(jonesConfig_ShowDialogInsertCD);
    J3D_HOOKFUNC(jonesConfig_DialogInsertCDProc);
    J3D_HOOKFUNC(jonesConfig_InitDialogInsertCD);
    J3D_HOOKFUNC(jonesConfig_InsertCD_HandleWM_COMMAND);
}

void jonesConfig_ResetGlobals(void)
{}

int jonesConfig_Startup(void)
{
    if ( jonesConfig_bStartup )
    {
        return 1;
    }

    for ( size_t i = 0; i < STD_ARRAYLEN(jonesConfig_apDialogIcons); ++i )
    {
        char aIconPath[128];
        STD_MAKEPATH(aIconPath, "misc", jonesConfig_apDialogIconFiles[i]);

        jonesConfig_apDialogIcons[i] = stdBmp_Load(aIconPath);
        if ( !jonesConfig_apDialogIcons[i] )
        {
            for ( size_t j = 0; j < i; ++j )
            {
                DeleteObject(jonesConfig_apDialogIcons[j]);
            }

            STDLOG_ERROR("Couldn't load bitmap %s.\n", aIconPath);
            return 0;
        }
    }

    if ( !jonesConfig_InitKeySetsPath() )
    {
        return 0;
    }

    if ( !jonesConfig_InitKeyActions() )
    {
        return 0;
    }

    memset(jonesConfig_aGlyphMetrics, 0, sizeof(jonesConfig_aGlyphMetrics));
    memset(&jonesConfig_textMetric, 0, sizeof(jonesConfig_textMetric));

    jonesConfig_bTextMetricInited = 0;

    jonesConfig_gamePlayOptions_bDefaultRun = stdConfig_GetBool(JONESCONFIG_CFG_GAMEPLAY_DEFAULTRUN, false);

    if ( jonesConfig_gamePlayOptions_bDefaultRun )
    {
        sithControl_g_controlOptions |= 2u; // run flag
    }
    else
    {
        sithControl_g_controlOptions &= ~2;
    }

    // Init cur level num
    jonesConfig_gameStatistics_curLevelNum = 0;

    JonesControlsScheme* pConfig = jonesConfig_LoadActiveControlScheme();
    if ( pConfig )
    {
        jonesConfig_BindControls(pConfig);
        jonesConfig_FreeControlScheme(pConfig);
    }

    jonesConfig_bStartup = 1;
    return 1;
}

int jonesConfig_InitKeySetsPath(void)
{
    wchar_t* pwKeySets = sithString_GetString("SITHSTRING_KEYSETS");
    if ( !pwKeySets )
    {
        return 0;
    }

    char* pKeySets = stdUtil_ToAString(pwKeySets);

    char aInstallPath[128] = { 0 };
    stdConfig_GetString(SITH_CFG_INSTALLPATH, aInstallPath, STD_ARRAYLEN(aInstallPath), "");

    if ( pKeySets )
    {
        size_t len = strlen(aInstallPath) + 1;
        if ( len == 1 )
        {
            STD_FORMAT(jonesConfig_aKeySetsDirPath, "..\\\\%s", pKeySets);
            stdMemory_Free(pKeySets);
            return 1;
        }
        else
        {
            if ( aInstallPath[len - 1] == '\\' )
            {
                STD_FORMAT(jonesConfig_aKeySetsDirPath, "%s%s", aInstallPath, pKeySets);
            }
            else
            {
                STD_FORMAT(jonesConfig_aKeySetsDirPath, "%s\\%s", aInstallPath, pKeySets);
            }

            stdMemory_Free(pKeySets);
            return 1;
        }
    }
    else
    {
        STDLOG_ERROR("Couldn't find string %s in lookup table.\n", 0); // TODO: [BUG] zero passed as %s
        stdMemory_Free(pwKeySets); // pwKeySets is null value but is allocated on heap in sithString_GetString
        return 0;
    }
}

int jonesConfig_InitKeyActions(void)
{
    jonesConfig_maxActionWords  = 0;
    jonesConfig_maxActionStrLen = 0;

    jonesConfig_maxControlWords  = 0; // Fixed: Assign 0
    jonesConfig_maxControlStrLen = 0; // Fixed: Assign 0

    for ( size_t i = 0; i < STD_ARRAYLEN(jonesConfig_aJonesCapControlActionNames); ++i )
    {
        const char* pActionName = jonesString_GetString(jonesConfig_aJonesCapControlActionNames[i]);
        if ( !pActionName )
        {
            return 0;
        }

        char aActionName[512] = { 0 }; // Fixed: Init. to 0
        STD_STRCPY(aActionName, pActionName);

        size_t len = 0;
        char* pCurActionWord = aActionName;
        while ( pCurActionWord )
        {
            pCurActionWord = strchr(pCurActionWord, ' ');
            if ( pCurActionWord )
            {
                ++pCurActionWord;
            }

            ++len;
        }

        if ( len > jonesConfig_maxActionWords )
        {
            jonesConfig_maxActionWords = len;
        }

        len = strlen(aActionName);
        if ( len > jonesConfig_maxActionStrLen )
        {
            jonesConfig_maxActionStrLen = len;
        }
    }

    for ( size_t i = 0; i < STDCONTROL_MAX_KEYID + JONESCONFIG_NUM_JOYAXISKIDS + 4; ++i ) // 656 =  STDCONTROL_MAX_KEYID + JONESCONFIG_NUM_JOYAXISKIDS + 4
    {
        char aControlStr[JONESCONFIG_CONTROLSTRING_MAXLEN] = { 0 };
        switch ( i )
        {
            case 644:
                jonesConfig_ControlToString(JONESCONFIG_JOYAXISKID_UP, aControlStr);
                break;

            case 645:
                jonesConfig_ControlToString(JONESCONFIG_JOYAXISKID_DOWN, aControlStr);
                break;

            case 646:
                jonesConfig_ControlToString(JONESCONFIG_JOYAXISKID_LEFT, aControlStr);
                break;

            case 647:
                jonesConfig_ControlToString(JONESCONFIG_JOYAXISKID_RIGHT, aControlStr);
                break;

            case 648:
                jonesConfig_ControlToString(JONESCONFIG_JOYAXISKID_AIR, aControlStr);
                break;

            case 649:
                jonesConfig_ControlToString(JONESCONFIG_JOYAXISKID_GROUND, aControlStr);
                break;

            case 650:
                jonesConfig_ControlToString(JONESCONFIG_JOYAXISKID_TWISTL, aControlStr);
                break;

            case 651:
                jonesConfig_ControlToString(JONESCONFIG_JOYAXISKID_TWISTR, aControlStr);
                break;

            default:
                jonesConfig_ControlToString(i, aControlStr);
                break;
        }

        size_t len = 0;
        char* pCurControlWord = aControlStr;
        while ( pCurControlWord )
        {
            pCurControlWord = strchr(pCurControlWord, ' ');
            if ( pCurControlWord )
            {
                ++pCurControlWord;
            }

            ++len;
        }

        if ( len > jonesConfig_maxControlWords )
        {
            jonesConfig_maxControlWords = len;
        }

        len = strlen(aControlStr);
        if ( len > jonesConfig_maxControlStrLen )
        {
            jonesConfig_maxControlStrLen = len;
        }
    }

    jonesConfig_maxActionPaddedStrLen  = 5 - jonesConfig_maxActionStrLen % 5 + jonesConfig_maxActionStrLen;
    jonesConfig_maxControlPaddedStrLen = 5 - jonesConfig_maxControlStrLen % 5 + jonesConfig_maxControlStrLen;
    return 1;
}

void jonesConfig_Shutdown(void)
{
    jonesConfig_FreeControlConfigEntry(&jonesConfig_controlOptions_curControlConfig);

    for ( size_t i = 0; i < 2; i++ ) // TODO: Bug? Why only 2?
    {
        DeleteObject(jonesConfig_apDialogIcons[i]);
    }

    if ( *jonesConfig_aKeySetsDirPath )
    {
        memset(jonesConfig_aKeySetsDirPath, 0, sizeof(jonesConfig_aKeySetsDirPath));
    }
}

void J3DAPI jonesConfig_ControlToString(size_t controlId, char* pDest)
{
    if ( !pDest )
    {
        // TODO: maybe log?
        return;
    }

    memset(pDest, 0, JONESCONFIG_CONTROLSTRING_MAXLEN);

    char* pJonesString = NULL;
    if ( controlId < JONESCONFIG_JOYAXISKID_UP )
    {
        switch ( controlId )
        {
            case DIK_ESCAPE:
                pJonesString = "JONES_STR_ESCAPE";
                break;

            case DIK_1:
                stdUtil_StringCopy(pDest, JONESCONFIG_CONTROLSTRING_MAXLEN, "1");
                break;

            case DIK_2:
                stdUtil_StringCopy(pDest, JONESCONFIG_CONTROLSTRING_MAXLEN, "2");
                break;

            case DIK_3:
                stdUtil_StringCopy(pDest, JONESCONFIG_CONTROLSTRING_MAXLEN, "3");
                break;

            case DIK_4:
                stdUtil_StringCopy(pDest, JONESCONFIG_CONTROLSTRING_MAXLEN, "4");
                break;

            case DIK_5:
                stdUtil_StringCopy(pDest, JONESCONFIG_CONTROLSTRING_MAXLEN, "5");
                break;

            case DIK_6:
                stdUtil_StringCopy(pDest, JONESCONFIG_CONTROLSTRING_MAXLEN, "6");
                break;

            case DIK_7:
                stdUtil_StringCopy(pDest, JONESCONFIG_CONTROLSTRING_MAXLEN, "7");
                break;

            case DIK_8:
                stdUtil_StringCopy(pDest, JONESCONFIG_CONTROLSTRING_MAXLEN, "8");
                break;

            case DIK_9:
                stdUtil_StringCopy(pDest, JONESCONFIG_CONTROLSTRING_MAXLEN, "9");
                break;

            case DIK_0:
                stdUtil_StringCopy(pDest, JONESCONFIG_CONTROLSTRING_MAXLEN, "0");
                break;

            case DIK_MINUS:
                pJonesString = "JONES_STR_MINUS";
                break;

            case DIK_EQUALS:
                pJonesString = "JONES_STR_EQUAL";
                break;

            case DIK_BACK:
                pJonesString = "JONES_STR_BACK";
                break;

            case DIK_TAB:
                pJonesString = "JONES_STR_TAB";
                break;

            case DIK_Q:
                stdUtil_StringCopy(pDest, JONESCONFIG_CONTROLSTRING_MAXLEN, "Q");
                break;

            case DIK_W:
                stdUtil_StringCopy(pDest, JONESCONFIG_CONTROLSTRING_MAXLEN, "W");
                break;

            case DIK_E:
                stdUtil_StringCopy(pDest, JONESCONFIG_CONTROLSTRING_MAXLEN, "E");
                break;

            case DIK_R:
                stdUtil_StringCopy(pDest, JONESCONFIG_CONTROLSTRING_MAXLEN, "R");
                break;

            case DIK_T:
                stdUtil_StringCopy(pDest, JONESCONFIG_CONTROLSTRING_MAXLEN, "T");
                break;

            case DIK_Y:
                stdUtil_StringCopy(pDest, JONESCONFIG_CONTROLSTRING_MAXLEN, "Y");
                break;

            case DIK_U:
                stdUtil_StringCopy(pDest, JONESCONFIG_CONTROLSTRING_MAXLEN, "U");
                break;

            case DIK_I:
                stdUtil_StringCopy(pDest, JONESCONFIG_CONTROLSTRING_MAXLEN, "I");
                break;

            case DIK_O:
                stdUtil_StringCopy(pDest, JONESCONFIG_CONTROLSTRING_MAXLEN, "O");
                break;

            case DIK_P:
                stdUtil_StringCopy(pDest, JONESCONFIG_CONTROLSTRING_MAXLEN, "P");
                break;

            case DIK_LBRACKET:
                pJonesString = "JONES_STR_LEFT_BRCKT";
                break;

            case DIK_RBRACKET:
                pJonesString = "JONES_STR_RIGHT_BRCKT";
                break;

            case DIK_RETURN:
                pJonesString = "JONES_STR_ENTER";
                break;

            case DIK_LCONTROL:
                pJonesString = "JONES_STR_LEFT_CTRL";
                break;

            case DIK_A:
                stdUtil_StringCopy(pDest, JONESCONFIG_CONTROLSTRING_MAXLEN, "A");
                break;

            case DIK_S:
                stdUtil_StringCopy(pDest, JONESCONFIG_CONTROLSTRING_MAXLEN, "S");
                break;

            case DIK_D:
                stdUtil_StringCopy(pDest, JONESCONFIG_CONTROLSTRING_MAXLEN, "D");
                break;

            case DIK_F:
                stdUtil_StringCopy(pDest, JONESCONFIG_CONTROLSTRING_MAXLEN, "F");
                break;

            case DIK_G:
                stdUtil_StringCopy(pDest, JONESCONFIG_CONTROLSTRING_MAXLEN, "G");
                break;

            case DIK_H:
                stdUtil_StringCopy(pDest, JONESCONFIG_CONTROLSTRING_MAXLEN, "H");
                break;

            case DIK_J:
                stdUtil_StringCopy(pDest, JONESCONFIG_CONTROLSTRING_MAXLEN, "J");
                break;

            case DIK_K:
                stdUtil_StringCopy(pDest, JONESCONFIG_CONTROLSTRING_MAXLEN, "K");
                break;

            case DIK_L:
                stdUtil_StringCopy(pDest, JONESCONFIG_CONTROLSTRING_MAXLEN, "L");
                break;

            case DIK_SEMICOLON:
                stdUtil_StringCopy(pDest, JONESCONFIG_CONTROLSTRING_MAXLEN, ";");
                break;

            case DIK_APOSTROPHE:
                stdUtil_StringCopy(pDest, JONESCONFIG_CONTROLSTRING_MAXLEN, "'");
                break;

            case DIK_GRAVE:
                stdUtil_StringCopy(pDest, JONESCONFIG_CONTROLSTRING_MAXLEN, "`");
                break;

            case DIK_LSHIFT:
                pJonesString = "JONES_STR_LEFT_SHIFT";
                break;

            case DIK_BACKSLASH:
                stdUtil_StringCopy(pDest, JONESCONFIG_CONTROLSTRING_MAXLEN, "\\");
                break;

            case DIK_Z:
                stdUtil_StringCopy(pDest, JONESCONFIG_CONTROLSTRING_MAXLEN, "Z");
                break;

            case DIK_X:
                stdUtil_StringCopy(pDest, JONESCONFIG_CONTROLSTRING_MAXLEN, "X");
                break;

            case DIK_C:
                stdUtil_StringCopy(pDest, JONESCONFIG_CONTROLSTRING_MAXLEN, "C");
                break;

            case DIK_V:
                stdUtil_StringCopy(pDest, JONESCONFIG_CONTROLSTRING_MAXLEN, "V");
                break;

            case DIK_B:
                stdUtil_StringCopy(pDest, JONESCONFIG_CONTROLSTRING_MAXLEN, "B");
                break;

            case DIK_N:
                stdUtil_StringCopy(pDest, JONESCONFIG_CONTROLSTRING_MAXLEN, "N");
                break;

            case DIK_M:
                stdUtil_StringCopy(pDest, JONESCONFIG_CONTROLSTRING_MAXLEN, "M");
                break;

            case DIK_COMMA:
                pJonesString = "JONES_STR_COMMA";
                break;

            case DIK_PERIOD:
                stdUtil_StringCopy(pDest, JONESCONFIG_CONTROLSTRING_MAXLEN, ".");
                break;

            case DIK_SLASH:
                stdUtil_StringCopy(pDest, JONESCONFIG_CONTROLSTRING_MAXLEN, "/");
                break;

            case DIK_RSHIFT:
                pJonesString = "JONES_STR_RIGHT_SHIFT";
                break;

            case DIK_MULTIPLY:
                pJonesString = "JONES_STR_NUMPAD_MULT";
                break;

            case DIK_LMENU:
                pJonesString = "JONES_STR_LEFT_ALT";
                break;

            case DIK_SPACE:
                pJonesString = "JONES_STR_SPACE";
                break;

            case DIK_CAPITAL:
                pJonesString = "JONES_STR_CAPS";
                break;

            case DIK_F1:
                pJonesString = "JONES_STR_F1";
                break;

            case DIK_F2:
                pJonesString = "JONES_STR_F2";
                break;

            case DIK_F3:
                pJonesString = "JONES_STR_F3";
                break;

            case DIK_F4:
                pJonesString = "JONES_STR_F4";
                break;

            case DIK_F5:
                pJonesString = "JONES_STR_F5";
                break;

            case DIK_F6:
                pJonesString = "JONES_STR_F6";
                break;

            case DIK_F7:
                pJonesString = "JONES_STR_F7";
                break;

            case DIK_F8:
                pJonesString = "JONES_STR_F8";
                break;

            case DIK_F9:
                pJonesString = "JONES_STR_F9";
                break;

            case DIK_F10:
                pJonesString = "JONES_STR_F10";
                break;

            case DIK_NUMLOCK:
                pJonesString = "JONES_STR_NUMLOCK";
                break;

            case DIK_NUMPAD7:
                pJonesString = "JONES_STR_NUMPAD_7";
                break;

            case DIK_NUMPAD8:
                pJonesString = "JONES_STR_NUMPAD_8";
                break;

            case DIK_NUMPAD9:
                pJonesString = "JONES_STR_NUMPAD_9";
                break;

            case DIK_SUBTRACT:
                pJonesString = "JONES_STR_NUMPAD_MINUS";
                break;

            case DIK_NUMPAD4:
                pJonesString = "JONES_STR_NUMPAD_4";
                break;

            case DIK_NUMPAD5:
                pJonesString = "JONES_STR_NUMPAD_5";
                break;

            case DIK_NUMPAD6:
                pJonesString = "JONES_STR_NUMPAD_6";
                break;

            case DIK_ADD:
                pJonesString = "JONES_STR_NUMPAD_PLUS";
                break;

            case DIK_NUMPAD1:
                pJonesString = "JONES_STR_NUMPAD_1";
                break;

            case DIK_NUMPAD2:
                pJonesString = "JONES_STR_NUMPAD_2";
                break;

            case DIK_NUMPAD3:
                pJonesString = "JONES_STR_NUMPAD_3";
                break;

            case DIK_NUMPAD0:
                pJonesString = "JONES_STR_NUMPAD_0";
                break;

            case DIK_DECIMAL:
                pJonesString = "JONES_STR_NUMPAD_DOT";
                break;

            case DIK_F11:
                pJonesString = "JONES_STR_F11";
                break;

            case DIK_F12:
                pJonesString = "JONES_STR_F12";
                break;

            case DIK_F13:
                pJonesString = "JONES_STR_F13";
                break;

            case DIK_F14:
                pJonesString = "JONES_STR_F14";
                break;

            case DIK_F15:
                pJonesString = "JONES_STR_F15";
                break;

            case DIK_NUMPADEQUALS:
                pJonesString = "JONES_STR_NUMPAD_EQL";
                break;

            case DIK_NUMPADENTER:
                pJonesString = "JONES_STR_NUMPAD_ENTER";
                break;

            case DIK_RCONTROL:
                pJonesString = "JONES_STR_RIGHT_CTRL";
                break;

            case DIK_NUMPADCOMMA:
                pJonesString = "JONES_STR_NUMPAD_COMMA";
                break;

            case DIK_DIVIDE:
                pJonesString = "JONES_STR_NUMPAD_DIV";
                break;

            case DIK_RMENU:
                pJonesString = "JONES_STR_RIGHT_ALT";
                break;

            case DIK_HOME:
                pJonesString = "JONES_STR_HOME";
                break;

            case DIK_UP:
                pJonesString = "JONES_STR_UPARRW";
                break;

            case DIK_PRIOR:
                pJonesString = "JONES_STR_PAGEUP";
                break;

            case DIK_LEFT:
                pJonesString = "JONES_STR_LEFTARRW";
                break;

            case DIK_RIGHT:
                pJonesString = "JONES_STR_RGHTARRW";
                break;

            case DIK_END:
                pJonesString = "JONES_STR_END";
                break;

            case DIK_DOWN:
                pJonesString = "JONES_STR_DNARRW";
                break;

            case DIK_NEXT:
                pJonesString = "JONES_STR_PGDN";
                break;

            case DIK_INSERT:
                pJonesString = "JONES_STR_INSERT";
                break;

            case DIK_DELETE:
                pJonesString = "JONES_STR_DELETE";
                break;

            case DIK_LWIN:
                pJonesString = "JONES_STR_LEFT_WND";
                break;

            case DIK_RWIN:
                pJonesString = "JONES_STR_RIGHT_WND";
                break;

            default:
            {
            joystick_controls:
                if ( controlId < STDCONTROL_JOYSTICK_FIRTPOVCID || controlId > STDCONTROL_JOYSTICK_LASTPOVCID || STDCONTROL_JOYSTICK_GETPOVDIRECTIONINDEX(controlId) != 0 )
                {
                    if ( controlId >= STDCONTROL_JOYSTICK_FIRTPOVCID && controlId <= STDCONTROL_JOYSTICK_LASTPOVCID && STDCONTROL_JOYSTICK_GETPOVDIRECTIONINDEX(controlId) == 2 )
                    {
                        pJonesString = "JONES_STR_HATRIGHT";
                    }
                    else if ( controlId >= STDCONTROL_JOYSTICK_FIRTPOVCID && controlId <= STDCONTROL_JOYSTICK_LASTPOVCID && STDCONTROL_JOYSTICK_GETPOVDIRECTIONINDEX(controlId) == 3 )
                    {
                        pJonesString = "JONES_STR_HATDOWN";
                    }
                    else if ( controlId >= STDCONTROL_JOYSTICK_FIRTPOVCID && controlId <= STDCONTROL_JOYSTICK_LASTPOVCID && STDCONTROL_JOYSTICK_GETPOVDIRECTIONINDEX(controlId) == 1 )
                    {
                        pJonesString = "JONES_STR_HATUP";
                    }
                    else if ( controlId >= STDCONTROL_JOYSTICK_FIRSTKID && controlId <= STDCONTROL_JOYSTICK_LASTPOVCID )
                    {
                        pJonesString = "JONES_STR_JOYSTICK";
                    }
                }
                else
                {
                    // Index 0
                    pJonesString = "JONES_STR_HATLEFT";
                }
            } break;
        }
    }
    else
    {
        switch ( controlId )
        {
            case JONESCONFIG_JOYAXISKID_UP:
                pJonesString = "JONES_STR_JOYUP";
                break;

            case JONESCONFIG_JOYAXISKID_RIGHT:
                pJonesString = "JONES_STR_JOYRIGHT";
                break;

            case JONESCONFIG_JOYAXISKID_DOWN:
                pJonesString = "JONES_STR_JOYDOWN";
                break;

            case JONESCONFIG_JOYAXISKID_LEFT:
                pJonesString = "JONES_STR_JOYLEFT";
                break;

            case JONESCONFIG_JOYAXISKID_TWISTL:
                pJonesString = "JONES_STR_JOYTWISTL";
                break;

            case JONESCONFIG_JOYAXISKID_TWISTR:
                pJonesString = "JONES_STR_JOYTWISTR";
                break;

            // Note missing handling of JONESCONFIG_JOYAXISKID_AIR and JONESCONFIG_JOYAXISKID_GROUND

            default:
                goto joystick_controls;
        }
    }

    if ( pJonesString )
    {
        const char* pControlStr = jonesString_GetString(pJonesString);
        if ( pControlStr )
        {
            if ( controlId < STDCONTROL_MAX_KEYBOARD_BUTTONS
                || controlId > STDCONTROL_JOYSTICK_LASTPOVCID
                || controlId >= STDCONTROL_JOYSTICK_FIRTPOVCID && STDCONTROL_JOYSTICK_GETPOVDIRECTIONINDEX(controlId) == STDCONTROL_JOYSTICK_POVDIRLEFT
                || controlId >= STDCONTROL_JOYSTICK_FIRTPOVCID && STDCONTROL_JOYSTICK_GETPOVDIRECTIONINDEX(controlId) == STDCONTROL_JOYSTICK_POVDIRUP
                || controlId >= STDCONTROL_JOYSTICK_FIRTPOVCID && STDCONTROL_JOYSTICK_GETPOVDIRECTIONINDEX(controlId) == STDCONTROL_JOYSTICK_POVDIRRIGHT
                || controlId >= STDCONTROL_JOYSTICK_FIRTPOVCID && STDCONTROL_JOYSTICK_GETPOVDIRECTIONINDEX(controlId) == STDCONTROL_JOYSTICK_POVDIRDOWN )
            {
                if ( controlId >= STDCONTROL_JOYSTICK_FIRTPOVCID && controlId <= STDCONTROL_JOYSTICK_LASTPOVCID && STDCONTROL_JOYSTICK_GETPOVDIRECTIONINDEX(controlId) == STDCONTROL_JOYSTICK_POVDIRLEFT
                    || controlId >= STDCONTROL_JOYSTICK_FIRTPOVCID && controlId <= STDCONTROL_JOYSTICK_LASTPOVCID && STDCONTROL_JOYSTICK_GETPOVDIRECTIONINDEX(controlId) == STDCONTROL_JOYSTICK_POVDIRUP
                    || controlId >= STDCONTROL_JOYSTICK_FIRTPOVCID && controlId <= STDCONTROL_JOYSTICK_LASTPOVCID && STDCONTROL_JOYSTICK_GETPOVDIRECTIONINDEX(controlId) == STDCONTROL_JOYSTICK_POVDIRRIGHT
                    || controlId >= STDCONTROL_JOYSTICK_FIRTPOVCID && controlId <= STDCONTROL_JOYSTICK_LASTPOVCID && STDCONTROL_JOYSTICK_GETPOVDIRECTIONINDEX(controlId) == STDCONTROL_JOYSTICK_POVDIRDOWN )
                {
                    // Joystick POV
                    stdUtil_Format(pDest, JONESCONFIG_CONTROLSTRING_MAXLEN, pControlStr, STDCONTROL_JOYSTICK_GETPOVINDEX(controlId) + 1);
                }
                else
                {
                    // Regular key or joystick stick move
                    stdUtil_StringCopy(pDest, JONESCONFIG_CONTROLSTRING_MAXLEN, pControlStr);
                }
            }
            else
            {
                // Joystick button
                stdUtil_Format(pDest, JONESCONFIG_CONTROLSTRING_MAXLEN, pControlStr, STDCONTROL_JOYSTICK_GETBUTTONINDEX(controlId) + 1);
            }
        }
    }
}

int J3DAPI jonesConfig_ShowMessageDialog(HWND hWnd, const char* pTitle, const char* pText, int iconID)
{
    GetWindowLongPtr(hWnd, GWL_HINSTANCE); // ???

    JonesMessageDialogData data = { 0 };
    data.iconID = iconID;
    STD_STRCPY(data.aTitle, pTitle);
    STD_STRCPY(data.aText, pText);
    return JonesDialog_ShowDialog(MAKEINTRESOURCE(121), hWnd, jonesConfig_MessageDialogProc, (LPARAM)&data);
}

BOOL CALLBACK jonesConfig_MessageDialogProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch ( uMsg )
    {
        case WM_DESTROY:
        {
            JonesMessageDialogData* pData = (JonesMessageDialogData*)GetWindowLongPtr(hwnd, DWL_USER);
            HWND hIconCtrl = GetDlgItem(hwnd, 1201);
            jonesConfig_ResetDialogFont(hwnd, jonesConfig_hFontMessageDialog);

            if ( pData )
            {
                if ( hIconCtrl )
                {
                    HICON hCurIcon = Static_GetIcon(hIconCtrl, NULL);
                    Static_SetIcon(hIconCtrl, pData->hPrevIcon);
                    DeleteObject(hCurIcon);
                }
            }

            return TRUE;
        }
        case WM_INITDIALOG:
        {
            JonesMessageDialogData* pData = (JonesMessageDialogData*)lParam;
            SetWindowText(hwnd, pData->aTitle);
            jonesConfig_InitMessageDialogIcon(hwnd, pData);
            jonesConfig_hFontMessageDialog = (HGDIOBJ)jonesConfig_InitDialog(hwnd, 0, 121);
            int inited = jonesConfig_InitMessageDialogData(hwnd, wParam, pData);

            SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_SHOWWINDOW | SWP_NOMOVE | SWP_NOSIZE);
            return inited;
        }
        case WM_COMMAND:
            jonesConfig_MessageDialog_HandleWM_COMMAND(hwnd, LOWORD(wParam));
            return FALSE;

        default:
            return FALSE;
    }
}

void J3DAPI jonesConfig_InitMessageDialogIcon(HWND hWnd, JonesMessageDialogData* pData)
{
    HINSTANCE hInstance = (HINSTANCE)GetWindowLongPtr(hWnd, GWL_HINSTANCE);
    if ( pData )
    {
        HANDLE hImage = LoadImage(hInstance, MAKEINTRESOURCE(pData->iconID), IMAGE_ICON, 64, 64, LR_DEFAULTCOLOR);
        HWND hImageCtrl = GetDlgItem(hWnd, 1201);
        if ( hImageCtrl )
        {
            if ( hImage )
            {
                HICON hIcon = Static_GetIcon(hImageCtrl, 0);
                Static_SetIcon(hImageCtrl, hImage);
                pData->hPrevIcon = hIcon;
            }
        }
    }
}

int J3DAPI jonesConfig_InitMessageDialogData(HWND hDlg, int a2, JonesMessageDialogData* pData)
{
    J3D_UNUSED(a2);

    HWND hTextCtrl = GetDlgItem(hDlg, 1075);
    GetDC(hTextCtrl); // TODO: ??
    if ( !pData )
    {
        return 0;
    }

    jonesConfig_InitMessageDialogIcon(hDlg, pData);
    jonesConfig_SetTextControl(hDlg, hTextCtrl, pData->aText);
    SetWindowLongPtr(hDlg, DWL_USER, (LONG_PTR)pData);
    return 1;
}

int J3DAPI jonesConfig_SetTextControl(HWND hDlg, HWND hTextCtrl, const char* aText)
{
    HDC hdc = GetDC(hTextCtrl);
    HFONT hFont = (HFONT)SendMessage(hTextCtrl, WM_GETFONT, 0, 0);
    SelectObject(hdc, hFont);

    if ( !jonesConfig_bTextMetricInited )
    {
        memset(jonesConfig_aGlyphMetrics, 0, sizeof(jonesConfig_aGlyphMetrics));
        memset(&jonesConfig_textMetric, 0, sizeof(jonesConfig_textMetric));
        GetTextMetrics(hdc, (LPTEXTMETRICA)&jonesConfig_textMetric);
        GetCharABCWidths(hdc, jonesConfig_textMetric.tmFirstChar, jonesConfig_textMetric.tmLastChar, jonesConfig_aGlyphMetrics);
        jonesConfig_bTextMetricInited = 1;
    }

    RECT rectWnd;
    GetWindowRect(hDlg, &rectWnd);
    LONG dlgX = rectWnd.left;
    LONG dlgY = rectWnd.top;

    GetWindowRect(hTextCtrl, &rectWnd);
    LONG maxWidth = rectWnd.right - rectWnd.left;

    size_t numLines = 0;
    const char* pCurPos = aText;
    do
    {
        const char* pStart = pCurPos;
        pCurPos = strchr(pCurPos, '\n');

        char aLine[256] = { 0 };
        if ( pCurPos )
        {
            STD_STRNCPY(aLine, pStart, pCurPos - pStart);
        }
        else
        {
            STD_STRCPY(aLine, pStart);
        }

        size_t lineLen = strlen(aLine);
        size_t lineWidth = 0;
        for ( size_t i = 0; i < lineLen; ++i )
        {
            const ABC* pCharMetric = &jonesConfig_aGlyphMetrics[(uint8_t)aLine[i] - jonesConfig_textMetric.tmFirstChar];
            lineWidth += abs(pCharMetric->abcB)
                + abs(pCharMetric->abcA)
                + abs(pCharMetric->abcC);
        }

        if ( lineLen || !pCurPos )
        {
            numLines += (int32_t)ceil((double)lineWidth / (double)maxWidth);
        }
        else
        {
            ++numLines;
        }

        if ( pCurPos )
        {
            if ( strlen(++pCurPos) == 0 )
            {
                pCurPos = NULL;
            }
        }
    } while ( pCurPos );

    LONG yOffset = numLines * (jonesConfig_textMetric.tmExternalLeading + jonesConfig_textMetric.tmHeight) - (rectWnd.bottom - rectWnd.top);
    if ( yOffset > 0 )
    {
        int heightCaption = GetSystemMetrics(SM_CYCAPTION);
        int frameSize = 2 * GetSystemMetrics(SM_CYDLGFRAME);

        HWND hOkBtn = GetDlgItem(hDlg, 1);
        RECT rectBtn;
        GetWindowRect(hOkBtn, &rectBtn);
        rectBtn.top += yOffset;

        RECT rectDlg;
        GetWindowRect(hDlg, &rectDlg);

        int btnX = rectBtn.left - rectDlg.left;
        int btnY = rectBtn.top - rectDlg.top - (frameSize + heightCaption);

        int textX = rectWnd.left - rectDlg.left;
        int textY = rectWnd.top - rectDlg.top - (frameSize + heightCaption);

        rectDlg.bottom += yOffset;
        rectBtn.bottom += yOffset;
        rectWnd.bottom += yOffset;

        MoveWindow(hDlg, dlgX, dlgY, rectDlg.right - rectDlg.left, rectDlg.bottom - rectDlg.top, 1);
        MoveWindow(hOkBtn, btnX, btnY, rectBtn.right - rectBtn.left, rectBtn.bottom - rectBtn.top, 1);
        MoveWindow(hTextCtrl, textX, textY, rectWnd.right - rectWnd.left, rectWnd.bottom - rectWnd.top, 1);

        HWND hNoBtn = GetDlgItem(hDlg, 2);
        if ( hNoBtn )
        {
            RECT rectNoBtn;
            GetWindowRect(hNoBtn, &rectNoBtn);
            rectNoBtn.top += yOffset;

            int btnNoY = rectNoBtn.top - rectDlg.top - (frameSize + heightCaption);

            rectNoBtn.bottom += yOffset;
            MoveWindow(hNoBtn, rectNoBtn.left - rectDlg.left, btnNoY, rectNoBtn.right - rectNoBtn.left, rectNoBtn.bottom - rectNoBtn.top, 1);
        }
    }

    SetWindowText(hTextCtrl, aText);
    return ShowWindow(hDlg, 1);
}

void J3DAPI jonesConfig_MessageDialog_HandleWM_COMMAND(HWND hDlg, int nResult)
{
    if ( nResult > 0 && nResult <= 2 )
    {
        EndDialog(hDlg, nResult);
    }
}

JonesControlsScheme* jonesConfig_LoadActiveControlScheme(void)
{
    JonesControlsScheme* pScheme = (JonesControlsScheme*)STDMALLOC(sizeof(JonesControlsScheme));
    if ( !pScheme )
    {
        return NULL;
    }

    memset(pScheme, 0, sizeof(JonesControlsScheme));

    char aPath[128] = { 0 };

    // Find active control scheme in system
    char aFilename[128] = { 0 };
    stdConfig_GetString(JONESCONFIG_CFG_CONTROLS_CONFIGFILE, aFilename, STD_ARRAYLEN(aFilename), "");
    if ( strlen(aFilename) )
    {
        // Load config scheme to new scheme
        char aFilePath[128] = { 0 };
        STD_FORMAT(aFilePath, "%s\\%s%s", jonesConfig_aKeySetsDirPath, aFilename, ".kfg");

        LPSTR pFilePart;
        SearchPath(jonesConfig_aKeySetsDirPath, aFilename, ".kfg", STD_ARRAYLEN(aPath), aPath, &pFilePart);
        if ( strlen(aPath) && jonesConfig_LoadControlScheme(aFilePath, pScheme) )
        {
            return pScheme;
        }

        // Load failed, try to make new scheme to default scheme based on config scheme name
        for ( size_t i = 0; i < STD_ARRAYLEN(jonesConfig_aDfltKeySetNames); i++ )
        {
            const char* pDfltKeyset = jonesString_GetString(jonesConfig_aDfltKeySetNames[i]);
            if ( !pDfltKeyset )
            {
                goto error;
            }

            STD_STRCPY(pScheme->aName, pDfltKeyset);

            if ( streq(aFilename, pDfltKeyset) )
            {
                if ( strlen(aPath) == 0 && jonesConfig_SetDefaultControlScheme(pScheme, i) ) // TODO: why len of aPath has to be 0?
                {
                    return pScheme;
                }
            }
        }

        // Clear keyset config
        // TODO: Should load the default scheme, right?
        STDLOG_WARNING("Failed to load control scheme '%s', reverting to default.\n", aFilePath); // Added: Added log
        stdConfig_SetString(JONESCONFIG_CFG_CONTROLS_CONFIGFILE, "");
        // Removed: goto error;
        //         Fall through to load default scheme.
        //         This fixes bug when custom scheme fails to load and default scheme is not loaded either.
    }

    // TODO: Hmm why is this check here? Should be already handled in above scope.
    if ( strlen(aFilename) && strlen(aPath) )
    {
        return pScheme;
    }

    // Ok we have no scheme configured, let's load default
    size_t schemeIdx = 0;
    size_t maxJoyButtons = stdControl_GetMaxJoystickButtons();
    if ( maxJoyButtons == 4 )
    {
        schemeIdx = 1;
    }
    else if ( maxJoyButtons == 8 )
    {
        schemeIdx = 2;
    }

    const char* pDfltKeyset = jonesString_GetString(jonesConfig_aDfltKeySetNames[schemeIdx]);
    if ( pDfltKeyset )
    {
        STD_STRCPY(pScheme->aName, pDfltKeyset);
        if ( jonesConfig_SetDefaultControlScheme(pScheme, schemeIdx) )
        {
            stdConfig_SetString(JONESCONFIG_CFG_CONTROLS_CONFIGFILE, pDfltKeyset);
            return pScheme;
        }
    }

error:
    jonesConfig_FreeControlScheme(pScheme);
    return NULL;
}

int J3DAPI jonesConfig_LoadControlScheme(const char* pFilePath, JonesControlsScheme* pConfig)
{
     // Function loads kfg file to pData

     // Altered: Moved this check before stdConffile_Open
    if ( !pConfig || !pFilePath )
    {
        return 0;
    }

    if ( !stdConffile_Open(pFilePath) )
    {
        return 0;
    }

    memset(pConfig->aName, 0, sizeof(pConfig->aName));
    STD_STRNCPY(pConfig->aName, pFilePath, strlen(pFilePath) - 4);// -4 removes .kfg extension

    size_t bindNum = 0;

    stdConffile_ReadArgs(); // Skip first comment line
    stdConffile_ReadArgs();

    if ( stdConffile_g_entry.numArgs && strneqi(stdConffile_g_entry.aArgs[0].argName, "1", 128u) )// 1 mark scheme is enabled
    {
        stdConffile_ReadArgs(); // Skip header line

        size_t actionCount = 0;

        for ( size_t i = 0; i < STD_ARRAYLEN(pConfig->aActions); ++i )
        {
            size_t argNum = 0;
            if ( !stdConffile_ReadArgs() )
            {
                break;
            }

            int actionIdx = jonesConfig_ParseArgumentIndex(&argNum, &stdConffile_g_entry, jonesConfig_maxActionWords, jonesConfig_aJonesControlActionNames, STD_ARRAYLEN(jonesConfig_aJonesControlActionNames));
            if ( actionIdx == -1 )
            {
                break;
            }

            bindNum = 1; // Note, at index 0 the num of bindings state is stored
            pConfig->aActions[actionIdx][0] = 0; // Reset number of bindings state

            // Removed: As paActionBindings was pointer to fixed size array
            //int (*paActionBindings)[9] = &pScheme->aActions[actionIdx];
            //if ( paActionBindings )

            while ( 1 )
            {
                if ( argNum >= stdConffile_g_entry.numArgs )
                {
                    break;
                }

                uint8_t bController = 0;
                uint8_t bKeyboard = 0;

                int keyIdx = jonesConfig_ParseArgumentIndex(&argNum, &stdConffile_g_entry, jonesConfig_maxControlWords, jonesConfig_aControlKeyStrings, STD_ARRAYLEN(jonesConfig_aControlKeyStrings));
                pConfig->aActions[actionIdx][bindNum] = keyIdx;

                if ( keyIdx != -1 )
                {
                    bKeyboard = 1;
                }
                else
                {
                    // We couldn't find control key num, try searching by control mouse button num, gamepad & joystick button num
                    --argNum; // Has to be decremented as jonesConfig_ParseArgumentIndex increments it
                    int controllerKey = jonesConfig_ParseJoystickControlId(&argNum, &stdConffile_g_entry);
                    pConfig->aActions[actionIdx][bindNum] = controllerKey;

                    if ( controllerKey == -1 )
                    {
                        goto error;
                    }

                    bController = 1;
                    ++argNum;
                }

                bindNum++;
                JONESCONTROL_ACTION_UPDATENUMBINDINGS(pConfig->aActions[actionIdx], bKeyboard, bController);

                if ( JONESCONTROL_ACTION_GETNUMBINDS(pConfig->aActions[actionIdx]) > JONESCONTROL_ACTION_MAXBINDS ) // TODO: is this a bug? Shouldn't it be GT or EQ?
                {
                    static_assert(STD_ARRAYLEN(pConfig->aActions[actionIdx]) == 9, "STD_ARRAYLEN(pConfig->aActions[actionIdx]) == 9");
                    // We've exceeded the number of key mappings

                    char aErrorActionName[512] = { 0 };
                    const char* pFaultyActionName = jonesString_GetString(jonesConfig_aJonesCapControlActionNames[actionIdx]);
                    if ( pFaultyActionName )
                    {
                        STD_STRCPY(aErrorActionName, pFaultyActionName);
                    }

                    char aErrorFormat[512] = { 0 };
                    const char* pErrorText = jonesString_GetString("JONES_STR_EXCEED");
                    if ( pErrorText )
                    {
                        STD_STRCPY(aErrorFormat, pErrorText);
                    }

                    if ( aErrorFormat[0] && aErrorActionName[0] )
                    {
                        char aErrorStr[256] = { 0 };
                        STD_FORMAT(aErrorStr, aErrorFormat, aErrorActionName);
                        jonesConfig_ShowMessageDialog(stdWin95_GetWindow(), "JONES_STR_CTRL_OPTS", aErrorStr, 145);
                    }

                    goto error;
                }
            }
        }

        // Success
        stdConffile_Close();
        return 1;
    }

error:
    memset(pConfig->aName, 0, sizeof(pConfig->aName));
    for ( size_t i = 0; i < bindNum; ++i ) // TODO: Hmm is it ok using binNum here? Shouldn't it use actionCount, or just memset the whole array?
    {
        if ( pConfig->aActions[i][0] )
        {
            memset(pConfig->aActions[i], 0, STD_ARRAYLEN(pConfig->aActions[i]));
        }
    }
    // Old code
    /*if ( bindNum > 0 )
    {
        pAction = pData->aActions;
        do
        {
            if ( pAction )
            {
                (*pAction)[0] = 0;
                (*pAction)[1] = 0;
                (*pAction)[2] &= 0xFF00;
            }

            ++pAction;
            --bindNum;
        } while ( bindNum );
    }*/

    stdConffile_Close();
    return 0;
}

int J3DAPI jonesConfig_ParseArgumentIndex(size_t* argNum, const StdConffileEntry* pEntry, size_t a3, const char** apStringList, size_t size)
{
    J3D_UNUSED(a3);

    char aValue[256] = { 0 }; // Fixed: Init to 0
    STD_STRCPY(aValue, pEntry->aArgs[*argNum].argName);
    int result = jonesConfig_GetStringListIndex(aValue, apStringList, size);

    ++*argNum;
    return result;
}

int J3DAPI jonesConfig_GetStringListIndex(char* pValue, const char** aStringList, size_t size)
{
    if ( size == STD_ARRAYLEN(jonesConfig_aControlKeyStrings) ) // if the list is list of keys, TODO: kill this nonsense and make case insensitive comparison
    {
        stdUtil_ToUpper(pValue);
    }

    for ( size_t index = 0; index < size; index++ )
    {
        if ( aStringList[index] )
        {
            const char* pStr = jonesString_GetString(aStringList[index]);
            if ( pStr )
            {
                if ( strneqi(pValue, pStr, strlen(pStr)) && strlen(pStr) == strlen(pValue) )
                {
                    return index;
                }
            }
            else if ( strneqi(pValue, aStringList[index], strlen(pValue)) && strlen(pValue) == strlen(aStringList[index]) )
            {
                return index;
            }
        }
    }

    return -1;
}

int J3DAPI jonesConfig_ParseJoystickControlId(size_t* pArgNum, const StdConffileEntry* pEntry)
{
    int controlId = -1;

    const char* pButtonFormat = jonesString_GetString("JONES_STR_CAPS_BUTTON");// "BUTTON"
    if ( pButtonFormat )
    {
        size_t formatLen = strlen(pButtonFormat);
        if ( strneqi(pEntry->aArgs[*pArgNum].argName, pButtonFormat, formatLen) )
        {
            char* pArg2 = strpbrk(pEntry->aArgs[*pArgNum].argName, "0123456789");
            if ( pArg2 )
            {
                int btnNum = atoi(pArg2) - 1;
                controlId = STDCONTROL_JOYSTICK_GETBUTTON(0, btnNum);
            }
        }
    }

    if ( controlId == -1 )
    {
        char aArgText[256] = { 0 };
        STD_STRCPY(aArgText, pEntry->aArgs[*pArgNum].argName);

        size_t btnNum = strcspn(aArgText, "0123456789");
        aArgText[btnNum] = '\0';

        if ( btnNum > 0 )
        {
            btnNum = atoi(&pEntry->aArgs[*pArgNum].argName[btnNum]) - 1;
        }

        char aHatFormat[256] = { 0 };
        char* pArg2 = strchr(pEntry->aArgs[*pArgNum].argName, ' ');
        if ( pArg2 )
        {
            STD_FORMAT(aHatFormat, "%s%%i %s", aArgText, pArg2 + 1);
        }

        char aForward[512] = { 0 };
        pButtonFormat = jonesString_GetString("JONES_STR_HATUP");// "HAT%i UP"
        if ( pButtonFormat )
        {
            STD_STRCPY(aForward, pButtonFormat);
        }

        char aBackward[512] = { 0 };
        pButtonFormat = jonesString_GetString("JONES_STR_HATDOWN");// "HAT%i DOWN"
        if ( pButtonFormat )
        {
            STD_STRCPY(aBackward, pButtonFormat);
        }

        char aLeft[512] = { 0 };
        pButtonFormat = jonesString_GetString("JONES_STR_HATLEFT");// "HAT%i LEFT"
        if ( pButtonFormat )
        {
            STD_STRCPY(aLeft, pButtonFormat);
        }

        char aRight[512] = { 0 };
        pButtonFormat = jonesString_GetString("JONES_STR_HATRIGHT");// "HAT%i RIGHT"
        if ( pButtonFormat )
        {
            STD_STRCPY(aRight, pButtonFormat);
        }

        if ( aForward[0] && aBackward[0] && aLeft[0] && aRight[0] && strlen(aHatFormat) )
        {
            size_t formatLen = strlen(aForward);
            if ( strneqi(aHatFormat, aForward, formatLen) )
            {
                controlId = STDCONTROL_JOYSTICK_GETPOVUP(0, btnNum);
            }
            else
            {
                formatLen = strlen(aBackward);
                if ( strneqi(aHatFormat, aBackward, formatLen) )
                {
                    controlId = STDCONTROL_JOYSTICK_GETPOVDOWN(0, btnNum);
                }
                else
                {
                    formatLen = strlen(aLeft);
                    if ( strneqi(aHatFormat, aLeft, formatLen) )
                    {
                        controlId = STDCONTROL_JOYSTICK_GETPOVLEFT(0, btnNum);
                    }
                    else
                    {
                        formatLen = strlen(aRight);
                        if ( strneqi(aHatFormat, aRight, formatLen) )
                        {
                            controlId = STDCONTROL_JOYSTICK_GETPOVRIGHT(0, btnNum);
                        }
                    }
                }
            }
        }
    }

    if ( controlId == -1 )
    {
        char aTwistList[512] = { 0 };
        pButtonFormat = jonesString_GetString("JONES_STR_JOYTWISTL");// "TWISTL"
        if ( pButtonFormat )
        {
            STD_STRCPY(aTwistList, pButtonFormat);
        }

        char aTwistRight[512] = { 0 };
        pButtonFormat = jonesString_GetString("JONES_STR_JOYTWISTR");// "TWISTR"
        if ( pButtonFormat )
        {
            STD_STRCPY(aTwistRight, pButtonFormat);
        }

        char aArgText[256] = { 0 };
        STD_FORMAT(aArgText, "%s", pEntry->aArgs[*pArgNum].argName);

        size_t formatLen = strlen(aTwistList);
        if ( strneqi(aArgText, aTwistList, formatLen) )
        {
            controlId = JONESCONFIG_JOYAXISKID_TWISTL;
        }
        else
        {
            formatLen = strlen(aTwistRight);
            if ( strneqi(aArgText, aTwistRight, formatLen) )
            {
                controlId = JONESCONFIG_JOYAXISKID_TWISTR;
            }
        }
    }

    if ( controlId == -1 )
    {
        char aForward[512] = { 0 };
        pButtonFormat = jonesString_GetString("JONES_STR_JOYUP");// "FORWARD"
        if ( pButtonFormat )
        {
            STD_STRCPY(aForward, pButtonFormat);
        }

        char aBackward[512] = { 0 };
        pButtonFormat = jonesString_GetString("JONES_STR_JOYDOWN");// "BACKWARD"
        if ( pButtonFormat )
        {
            STD_STRCPY(aBackward, pButtonFormat);
        }

        char aLeft[512] = { 0 };
        pButtonFormat = jonesString_GetString("JONES_STR_JOYLEFT");// "LEFT"
        if ( pButtonFormat )
        {
            STD_STRCPY(aLeft, pButtonFormat);
        }

        char aRight[512] = { 0 };
        pButtonFormat = jonesString_GetString("JONES_STR_JOYRIGHT");// "RIGHT"
        if ( pButtonFormat )
        {
            STD_STRCPY(aRight, pButtonFormat);
        }

        size_t formatLen = strlen(aForward);
        if ( strneqi(pEntry->aArgs[*pArgNum].argName, aForward, formatLen) )
        {
            controlId = JONESCONFIG_JOYAXISKID_UP;
        }
        else
        {
            formatLen = strlen(aBackward);
            if ( strneqi(pEntry->aArgs[*pArgNum].argName, aBackward, formatLen) )
            {
                controlId = JONESCONFIG_JOYAXISKID_DOWN;
            }
            else
            {
                formatLen = strlen(aLeft);
                if ( strneqi(pEntry->aArgs[*pArgNum].argName, aLeft, formatLen) )
                {
                    controlId = JONESCONFIG_JOYAXISKID_LEFT;
                }
                else
                {
                    formatLen = strlen(aRight);
                    if ( strneqi(pEntry->aArgs[*pArgNum].argName, aRight, formatLen) )
                    {
                        controlId = JONESCONFIG_JOYAXISKID_RIGHT;
                    }
                }
            }
        }
    }

    if ( controlId > -1 )
    {
        ++*pArgNum;
    }

    return controlId;
}

int J3DAPI jonesConfig_SetDefaultControlScheme(JonesControlsScheme* pScheme, size_t num)
{
    const char* pDefaultKeySetName = jonesString_GetString(jonesConfig_aDfltKeySetNames[num]);
    if ( !pDefaultKeySetName )
    {
        return 0;
    }

    STD_STRCPY(pScheme->aName, pDefaultKeySetName);

    for ( size_t i = 0; i < JONESCONTROL_ACTION_NUMACTIONS; ++i )
    {
        size_t* pAction = pScheme->aActions[i];
        *pAction = 0;

        for ( size_t j = 1; j <= jonesConfig_aDfltKeySets[num][i].numBinds; ++j )
        {
            pAction[j] = jonesConfig_aDfltKeySets[num][i].aBindings[j - 1];
            if ( (pAction[j] < STDCONTROL_MAX_KEYBOARD_BUTTONS || pAction[j] > STDCONTROL_JOYSTICK_LASTPOVCID)
                && (pAction[j] < JONESCONFIG_JOYAXISKID_UP || pAction[j] > JONESCONFIG_JOYAXISKID_GROUND) )
            {
                // keyboard
                JONESCONTROL_ACTION_UPDATENUMBINDINGS_KEYBOARD(pScheme->aActions[i], +1);
            }
            else
            {
                // joystick controller
                JONESCONTROL_ACTION_UPDATENUMBINDINGS_CONTROLLER(pScheme->aActions[i], +1);
            }
        }
    }

    return 1;
}

void J3DAPI jonesConfig_BindControls(JonesControlsScheme* pConfig)
{
    JonesControlActions actionId;

    int bMouse    = stdConfig_GetBool(JONESCONTROL_CFG_CONTROLS_MOUSE, false);
    int bJoystick = stdConfig_GetBool(JONESCONTROL_CFG_CONTROLS_CONTROLLER, false);

    if ( pConfig )
    {
        // Bind toggle menu keys
        JonesMain_BindToggleMenuControlKeys(&pConfig->aActions[JONESCONTROL_ACTION_INTERFACE][1],
            JONESCONTROL_ACTION_GETNUMBINDS(pConfig->aActions[JONESCONTROL_ACTION_INTERFACE])
        );

        // Bind toggle menu keys
        JonesHud_BindActivateControlKeys(&pConfig->aActions[JONESCONTROL_ACTION_ACTIVATE][1],
            JONESCONTROL_ACTION_GETNUMBINDS(pConfig->aActions[JONESCONTROL_ACTION_ACTIVATE])
        );

        for ( SithControlFunction fctnID = SITHCONTROL_WSELECT0; fctnID <= SITHCONTROL_WSELECT16; ++fctnID )
        {
            sithControl_UnbindFunction(fctnID);
        }

        for ( actionId = JONESCONTROL_ACTION_WALKFORWARD; actionId < JONESCONTROL_ACTION_NUMACTIONS; ++actionId )
        {
            SithControlFunction functionId = -1;

            if ( actionId == JONESCONTROL_ACTION_ACTIVATE || actionId == JONESCONTROL_ACTION_TURNLEFT || actionId == JONESCONTROL_ACTION_TURNRIGHT )
            {
                size_t curBindNum = 1;
                size_t numActionBinds = 3;
                if ( actionId <= JONESCONTROL_ACTION_TURNRIGHT )
                {
                    numActionBinds = 2;
                }

                while ( curBindNum <= numActionBinds )
                {
                    if ( actionId == JONESCONTROL_ACTION_TURNLEFT )
                    {
                        if ( curBindNum == 1 )
                        {
                            functionId = SITHCONTROL_LEFT;
                        }
                        else if ( curBindNum == 2 )
                        {
                            functionId = SITHCONTROL_TURNLEFT;
                        }
                    }
                    else if ( actionId == JONESCONTROL_ACTION_TURNRIGHT )
                    {
                        if ( curBindNum == 1 )
                        {
                            functionId = SITHCONTROL_RIGHT;
                        }

                        else if ( curBindNum == 2 )
                        {
                            functionId = SITHCONTROL_TURNRIGHT;
                        }
                    }
                    else
                    {
                        switch ( curBindNum )
                        {
                            case 1:
                                functionId = SITHCONTROL_ACT2;
                                break;

                            case 2:
                                functionId = SITHCONTROL_FIRE1;
                                break;

                            case 3:
                                functionId = SITHCONTROL_ACTIVATE;
                                break;
                        }
                    }

                    if ( functionId != -1 )
                    {
                        sithControl_UnbindFunction(functionId);

                        // TODO: This must be a bug checking i is LT or EQ, most likely should be LT
                        for ( size_t i = 1; i <= JONESCONTROL_ACTION_GETNUMBINDS(pConfig->aActions[actionId]); ++i )
                        {
                            if ( pConfig->aActions[actionId][i] >= STDCONTROL_JOYSTICK_FIRSTKID && pConfig->aActions[actionId][i] <= STDCONTROL_JOYSTICK_LASTPOVCID
                                || pConfig->aActions[actionId][i] >= STDCONTROL_JOYSTICK_FIRSTKID )
                            {
                                // We have joystick key bind
                                if ( bJoystick )
                                {
                                    jonesConfig_BindJoystickControl(functionId, pConfig->aActions[actionId][i]);
                                }
                            }
                            else
                            {
                                // keyboard key bind
                                sithControl_BindControl(functionId, pConfig->aActions[actionId][i], (SithControlBindFlag)0);
                            }
                        }
                    }

                    ++curBindNum;
                }
            }

            else if ( actionId >= JONESCONTROL_ACTION_FISTS && actionId != JONESCONTROL_ACTION_MAP && actionId != JONESCONTROL_ACTION_INTERFACE )
            {
                if ( JONESCONTROL_ACTION_GETNUMBINDS(pConfig->aActions[actionId]) ) // TODO: this is kinda useless
                {
                    // TODO: This must be a bug checking i is LT or EQ, most likely should be LT
                    for ( size_t i = 1; i <= JONESCONTROL_ACTION_GETNUMBINDS(pConfig->aActions[actionId]); ++i )
                    {
                        switch ( actionId )
                        {
                            case JONESCONTROL_ACTION_FISTS:
                                functionId = SITHCONTROL_WSELECT0;
                                break;

                            case JONESCONTROL_ACTION_WHIP:
                                functionId = SITHCONTROL_WSELECT1;
                                break;

                            case JONESCONTROL_ACTION_MAUSER:
                                functionId = SITHCONTROL_WSELECT4;
                                break;

                            case JONESCONTROL_ACTION_PPSH41:
                                functionId = SITHCONTROL_WSELECT7;
                                break;

                            case JONESCONTROL_ACTION_PISTOL:
                                functionId = SITHCONTROL_WSELECT2;
                                break;

                            case JONESCONTROL_ACTION_SIMONOV:
                                functionId = SITHCONTROL_WSELECT5;
                                break;

                            case JONESCONTROL_ACTION_TOKEREV:
                                functionId = SITHCONTROL_WSELECT3;
                                break;

                            case JONESCONTROL_ACTION_TOZ34:
                                functionId = SITHCONTROL_WSELECT8;
                                break;

                            case JONESCONTROL_ACTION_BAZOOKA:
                                functionId = SITHCONTROL_WSELECT9;
                                break;

                            case JONESCONTROL_ACTION_MACHETE:
                                functionId = SITHCONTROL_WSELECT6;
                                break;

                            case JONESCONTROL_ACTION_SATCHEL:
                                functionId = SITHCONTROL_WSELECT11;
                                break;

                            case JONESCONTROL_ACTION_GRENADE:
                                functionId = SITHCONTROL_WSELECT10;
                                break;

                            case JONESCONTROL_ACTION_HEALTH:
                                functionId = SITHCONTROL_HEALTH;
                                break;

                            case JONESCONTROL_ACTION_IMP1:
                                functionId = SITHCONTROL_WSELECT12;
                                break;

                            case JONESCONTROL_ACTION_IMP2:
                                functionId = SITHCONTROL_WSELECT13;
                                break;

                            case JONESCONTROL_ACTION_IMP3:
                                functionId = SITHCONTROL_WSELECT14;
                                break;

                            case JONESCONTROL_ACTION_IMP4:
                                functionId = SITHCONTROL_WSELECT15;
                                break;

                            case JONESCONTROL_ACTION_IMP5:
                                functionId = SITHCONTROL_WSELECT16;
                                break;

                            case JONESCONTROL_ACTION_CHALK:
                                functionId = SITHCONTROL_CHALK;
                                break;

                            default:
                                break;
                        }

                        if ( functionId != -1 )
                        {
                            if ( pConfig->aActions[actionId][i] >= STDCONTROL_JOYSTICK_FIRSTKID && pConfig->aActions[actionId][i] <= STDCONTROL_JOYSTICK_LASTPOVCID
                                || pConfig->aActions[actionId][i] >= STDCONTROL_JOYSTICK_FIRSTKID )
                            {
                                // We have joystick key bind
                                if ( bJoystick )
                                {
                                    jonesConfig_BindJoystickControl(functionId, pConfig->aActions[actionId][i]);
                                }
                            }
                            else
                            {
                                // We have keyboard key bind
                                sithControl_BindControl(functionId, pConfig->aActions[actionId][i], (SithControlBindFlag)0);
                            }
                        }
                    }
                }
            }
            else
            {
                switch ( actionId )
                {
                    case JONESCONTROL_ACTION_WALKFORWARD:
                        functionId = SITHCONTROL_FORWARD;
                        break;

                    case JONESCONTROL_ACTION_WALKBACK:
                        functionId = SITHCONTROL_BACK;
                        break;

                    case JONESCONTROL_ACTION_STEPLEFT:
                        functionId = SITHCONTROL_STPLEFT;
                        break;

                    case JONESCONTROL_ACTION_STEPRIGHT:
                        functionId = SITHCONTROL_STPRIGHT;
                        break;

                    case JONESCONTROL_ACTION_CRAWL:
                        functionId = SITHCONTROL_CRAWLTOGGLE;
                        break;

                    case JONESCONTROL_ACTION_RUN:
                        functionId = SITHCONTROL_ACT1;
                        break;

                    case JONESCONTROL_ACTION_ROLL:
                        functionId = SITHCONTROL_ACT3;
                        break;

                    case JONESCONTROL_ACTION_JUMP:
                        functionId = SITHCONTROL_JUMP;
                        break;

                    case JONESCONTROL_ACTION_LOOK:
                        functionId = SITHCONTROL_LOOK;
                        break;

                    case JONESCONTROL_ACTION_TOGGLEWEAPON:
                        functionId = SITHCONTROL_WEAPONTOGGLE;
                        break;

                    case JONESCONTROL_ACTION_PREVWEAPON:
                        functionId = SITHCONTROL_PREVWEAPON;
                        break;

                    case JONESCONTROL_ACTION_NEXTWEAPON:
                        functionId = SITHCONTROL_NEXTWEAPON;
                        break;

                    case JONESCONTROL_ACTION_TOGGLELIGHT:
                        functionId = SITHCONTROL_LIGHTERTOGGLE;
                        break;

                    case JONESCONTROL_ACTION_MAP:
                        functionId = SITHCONTROL_MAP;
                        break;

                    default:
                        break;
                }

                if ( functionId != -1 )
                {
                    sithControl_UnbindFunction(functionId);
                    if ( functionId == SITHCONTROL_FORWARD )
                    {
                        sithControl_UnbindFunction(SITHCONTROL_RUNFWD);
                    }

                    // TODO: This must be a bug checking i is LT or EQ, most likely should be LT
                    for ( size_t i = 1; i <= JONESCONTROL_ACTION_GETNUMBINDS(pConfig->aActions[actionId]); ++i )
                    {
                        if ( pConfig->aActions[actionId][i] >= STDCONTROL_JOYSTICK_FIRSTKID && pConfig->aActions[actionId][i] <= STDCONTROL_JOYSTICK_LASTPOVCID
                            || pConfig->aActions[actionId][i] >= STDCONTROL_JOYSTICK_FIRSTKID )
                        {
                            if ( bJoystick )
                            {
                                jonesConfig_BindJoystickControl(functionId, pConfig->aActions[actionId][i]);
                            }
                        }
                        else
                        {
                            sithControl_BindControl(functionId, pConfig->aActions[actionId][i], (SithControlBindFlag)0);
                        }
                    }
                }
            }
        }

        jonesConfig_EnableMouseControl(bMouse);
        if ( !bJoystick )
        {
            sithControl_UnbindJoystickAxes();
        }
    }
}

void J3DAPI jonesConfig_BindJoystickControl(SithControlFunction functionId, size_t controlId)
{
    int axis = -1;
    int flags = 0; // Fixed: Init to 0

    switch ( controlId )
    {
        case JONESCONFIG_JOYAXISKID_UP:
            axis  = STDCONTROL_JOYSTICK_AXIS_Y;
            flags = STDCONTROL_AID_NEGATIVE_AXIS;
            break;

        case JONESCONFIG_JOYAXISKID_RIGHT:
            axis  = STDCONTROL_JOYSTICK_AXIS_X;
            flags = STDCONTROL_AID_POSITIVE_AXIS;
            break;

        case JONESCONFIG_JOYAXISKID_DOWN:
            axis  = STDCONTROL_JOYSTICK_AXIS_Y;
            flags = STDCONTROL_AID_POSITIVE_AXIS;
            break;

        case JONESCONFIG_JOYAXISKID_LEFT:
            axis  = STDCONTROL_JOYSTICK_AXIS_X;
            flags = STDCONTROL_AID_NEGATIVE_AXIS;
            break;

        case JONESCONFIG_JOYAXISKID_TWISTL:
            axis  = STDCONTROL_JOYSTICK_AXIS_RZ;
            flags = STDCONTROL_AID_NEGATIVE_AXIS;
            break;

        case JONESCONFIG_JOYAXISKID_TWISTR:
            axis  = STDCONTROL_JOYSTICK_AXIS_RZ;
            flags = STDCONTROL_AID_POSITIVE_AXIS;
            break;

        case JONESCONFIG_JOYAXISKID_AIR:
            axis  = STDCONTROL_JOYSTICK_AXIS_Z;
            flags = STDCONTROL_AID_NEGATIVE_AXIS;
            break;

        case JONESCONFIG_JOYAXISKID_GROUND:
            axis  = STDCONTROL_JOYSTICK_AXIS_Z;
            flags = STDCONTROL_AID_POSITIVE_AXIS;
            break;
        default:
            axis = -1;
            break;
    }

    const size_t numJoysticks = stdControl_GetNumJoysticks();
    if ( axis == -1 )
    {
        // Bind joystick button
        for ( size_t joyNum = 0; joyNum < numJoysticks; ++joyNum )
        {
            // Note, controlID has to be unpacked to button number due to binding cid to specific joystick
            sithControl_BindControl(functionId, STDCONTROL_JOYSTICK_GETBUTTON(joyNum, STDCONTROL_JOYSTICK_GETBUTTONINDEX(controlId)), (SithControlBindFlag)0);
        }
    }
    else
    {
        for ( size_t joyNum = 0; (int)joyNum < numJoysticks; ++joyNum )
        {
            sithControl_BindAxis(functionId, STDCONTROL_GET_JOYSTICK_AXIS(joyNum, axis) | flags, (SithControlBindFlag)0);
            if ( functionId == SITHCONTROL_FORWARD && !stdControl_IsGamePad(joyNum) )
            {
                flags |= STDCONTROL_AID_LOW_SENSITIVITY;
                functionId = SITHCONTROL_RUNFWD;
                sithControl_BindAxis(SITHCONTROL_RUNFWD, STDCONTROL_GET_JOYSTICK_AXIS(joyNum, axis) | flags, (SithControlBindFlag)0);
            }
        }
    }
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
            dialogRefHeight = jonesConfig_aDialogSizes[i].refHeight * dpiScale; // Added: scale by dpi
            break;
        }
    }

    double scale = 1.0f; // Added: Init to 1.0f
    if ( dialogRefHeight > 0 )
    {
        scale = (dialogRefHeight / (double)rect.bottom) * dpiScale; // Added: scale by dpi
    }

    int cHeight;
    float cHeightf;
    uint32_t scrwidth, scrheight;

    int scaledRefHeight = (int)(RD_REF_HEIGHT * scale);
    int scaledRefWidth  = (int)(RD_REF_WIDTH * scale);

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
        cHeight = -MulDiv(10, systemDPI, USER_DEFAULT_SCREEN_DPI); // Changed: to 10 from 8
        cHeightf = (float)cHeight;

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

    if ( cHeight > (int)cHeightf ) // Changed: Use cHeightf if smaller than cHeight; i.e.: bigger font size
    {
        cHeight = (int)cHeightf;
    }

    // Adjust minimum height based on DPI
    if ( cHeight > -MulDiv(10, systemDPI, USER_DEFAULT_SCREEN_DPI) ) // Changed: to 10 from 9 and dpi scale check
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
    HFONT hfont = (HFONT)GetWindowLongPtr(hWnd, GWL_USERDATA);
    SendMessage(hWnd, WM_SETFONT, (WPARAM)hfont, 0);  // 0 - don't repaint
    return TRUE;
}

BOOL CALLBACK jonesConfig_SetPositionAndTextCallback(HWND hCtrl, LPARAM lparam)
{
    // Function resize control by scale, sets font and
    // replaces all text of control from JONES_STR_* to corresponding text

    // Fixed: Check if hCtrl parent is top dialog, otherwise skip control.
    //        This fixes positioning and scaling of controls with children
    //        as they also get enumerated. So we skip here any child window of the control.
    if ( GetDlgCtrlID(GetParent(hCtrl)) != 0 )
    {
        return TRUE;
    }

    if ( !lparam )
    { // Added: Added check for null
        return TRUE;
    }
    JonesDialogFontInfo* pFontInfo = (JonesDialogFontInfo*)lparam;

    if ( pFontInfo->hFont )
    {
        // Scale and set font
        jonesConfig_SetWindowFontAndPosition(hCtrl, pFontInfo);
    }

    if ( pFontInfo->dialogID == 154 || pFontInfo->dialogID == 159 ) // Load/Save dialogs
    {
        return TRUE;
    }

    // Replace JONES_STR_* strings to corresponding text

    CHAR aClassName[256] = { 0 };
    GetClassName(hCtrl, aClassName, 256);

    CHAR aText[256] = { 0 };
    if ( strneqi(aClassName, "BUTTON", STD_ARRAYLEN(aClassName)) )
    {
        GetWindowText(hCtrl, aText, STD_ARRAYLEN(aText));
        LPCSTR pText = jonesString_GetString(aText);
        if ( pText )
        {
            SetWindowText(hCtrl, pText);
        }

        return TRUE;
    }
    else if ( strneqi(aClassName, "STATIC", STD_ARRAYLEN(aClassName)) )
    {
        GetWindowText(hCtrl, aText, STD_ARRAYLEN(aText));
        if ( strchr(aText, '%') )
        {
            return TRUE;
        }

        LPCSTR pText = jonesString_GetString(aText);
        if ( pText )
        {
            SetWindowText(hCtrl, pText);
        }

        return TRUE;
    }

    return TRUE;
}

void J3DAPI jonesConfig_SetWindowFontAndPosition(HWND hCtrl, JonesDialogFontInfo* pFontInfo)
{
    HWND hParent = GetParent(hCtrl);

    // Set font
    if ( pFontInfo->hFont )
    {
        SendMessage(hCtrl, WM_SETFONT, (WPARAM)pFontInfo->hFont, 1); // 1 - repaint
        if ( HIWORD(pFontInfo->dialogID) != JONESCONFIG_NOFONTSCALEMASK )
        {
            SetWindowLongPtr(hCtrl, GWL_USERDATA, (LONG_PTR)pFontInfo->hControlFont);
        }
    }

    RECT rectWindow;
    GetWindowRect(hCtrl, &rectWindow);

    RECT rectClient;
    GetClientRect(hCtrl, &rectClient);

    RECT rectDlgWindow;
    GetWindowRect(hParent, &rectDlgWindow);

    if ( !pFontInfo->bWindowMode || pFontInfo->fontScaleX < 1.0f || pFontInfo->fontScaleY < 1.0f )
    {
        int offsetY = 0;
        if ( HIWORD(pFontInfo->dialogID) != JONESCONFIG_NOFONTSCALEMASK )
        {
        LABEL_17:
            HICON hIcon = (HICON)SendMessage(hCtrl, STM_GETICON, 0, 0);
            HWND dlgItem = GetDlgItem(hParent, 1182);
            if ( dlgItem == hCtrl )
            {
                hIcon = (HICON)dlgItem;
            }

            HWND hwndResumeBtn = NULL;
            HWND hwndThumbnail = NULL;

            if ( pFontInfo->dialogID == 159 || pFontInfo->dialogID == 154 )// Load/Save dialog
            {
                LONG dlgID =(LONG)GetWindowLongPtr(hCtrl, GWL_USERDATA); // TODO: UNUSED
                J3D_UNUSED(dlgID);

                HWND hwndParent = GetParent(hParent);
                if ( hwndParent )
                {
                    HWND dlgItem1137 = GetDlgItem(hwndParent, 1137);
                    if ( dlgItem1137 )
                    {
                        if ( dlgItem1137 != hCtrl )
                        {
                            RECT rectDlgItem1137Window;
                            GetWindowRect(dlgItem1137, &rectDlgItem1137Window);
                            offsetY = (int)((double)(rectDlgItem1137Window.bottom - rectDlgItem1137Window.top) * (1.0f - pFontInfo->fontScaleX));
                        }
                    }
                }

                if ( hCtrl == GetDlgItem(hParent, 1120) )
                {
                    hwndResumeBtn = hCtrl;
                }
                if ( hCtrl == GetDlgItem(hParent, 1163) || hCtrl == GetDlgItem(hParent, 1125) )
                {
                    hwndThumbnail = hCtrl; // Load / Save thumbnail
                    offsetY = 0;
                }
            }

            //if ( (!hPrevIcon || heightDesktop < (size_t)RD_REF_HEIGHT && widthDesktop < (size_t)RD_REF_WIDTH) && !hwndThumbnail )
            {
                double fontScaleX = pFontInfo->fontScaleX;
             /*   if ( hPrevIcon && heightDesktop < (size_t)RD_REF_HEIGHT && widthDesktop < (size_t)RD_REF_WIDTH )
                {
                    fontScaleX = (double)heightDesktop / RD_REF_HEIGHT;
                }*/

                rectWindow.bottom = (LONG)((double)(rectWindow.bottom - rectWindow.top) * fontScaleX) + rectWindow.top;
            }

            //if ( (!hPrevIcon || heightDesktop < (size_t)RD_REF_HEIGHT && widthDesktop < (size_t)RD_REF_WIDTH) && !hwndThumbnail && !hwndResumeBtn )
            {
                double fontScaleY = pFontInfo->fontScaleY;
               /* if ( hPrevIcon && heightDesktop < (size_t)RD_REF_HEIGHT && widthDesktop < (size_t)RD_REF_WIDTH )
                {
                    fontScaleY = (double)widthDesktop / RD_REF_WIDTH;
                }*/

                rectWindow.right = (LONG)((double)(rectWindow.right - rectWindow.left) * fontScaleY) + rectWindow.left;
            }

            if ( !hParent )
            {
                return;
            }

            GetWindowRect(hParent, &rectDlgWindow);
            jonesConfig_GetWindowScreenRect(hParent, &rectClient);

            int posX = (int)((double)(rectWindow.left - rectClient.left) * pFontInfo->fontScaleY);
            int posY = (int)((double)(rectWindow.top - rectClient.top) * pFontInfo->fontScaleX);
            if ( offsetY > 0 && !hwndThumbnail )
            {
                posY += offsetY;
            }

            // Resize and reposition control
            MoveWindow(hCtrl, posX, posY, rectWindow.right - rectWindow.left, rectWindow.bottom - rectWindow.top, /*bRepaint=*/TRUE);

            GetWindowRect(hCtrl, &rectWindow);
            GetClientRect(hCtrl, &rectClient);
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
                    MoveWindow(hParent, rectDlgWindow.left, rectDlgWindow.top, dDlgWinX + dlgWidth, dlgHeight, 1);
                }
                else
                {
                    MoveWindow(hParent, rectDlgWindow.left, rectDlgWindow.top, dlgWidth, dDlgWinY + dlgHeight, 1);
                }
            }
            else
            {
                MoveWindow(hParent, rectDlgWindow.left, rectDlgWindow.top, dDlgWinX + dlgWidth, dDlgWinY + dlgHeight, 1);
            }

            return;
        }

        LONG dlgID = (LONG)GetWindowLongPtr(hCtrl, GWL_USERDATA);
        if ( dlgID == 159 || dlgID == 154 )     // load/save dialogs
        {
            SendMessage(hCtrl, WM_SETFONT, (WPARAM)pFontInfo->hControlFont, 1);// 1 - marks redraw
            return;
        }

        HWND dlgItem1137  = GetDlgItem(hParent, 1137);
        if ( dlgItem1137 ) // note, no dlg item with this ID in resources
        {
            CHAR aClassName[256] = { 0 };
            GetClassName(hCtrl, aClassName, 256);
            if ( dlgItem1137 != hCtrl && strcmp(aClassName, "ToolbarWindow32") )
            {
                RECT rectDlgItem1137Window;
                GetWindowRect(dlgItem1137, &rectDlgItem1137Window);
                offsetY = (int)((double)(rectDlgItem1137Window.bottom - rectDlgItem1137Window.top) * (1.0f - pFontInfo->fontScaleX));
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

int J3DAPI jonesConfig_GetSaveGameFilePath(HWND hWnd, char* pOutFilePath)
{
    char aFileterStr[512] = { 0 };
    const char* pFilterStr = jonesString_GetString("JONES_STR_FILTER");
    if ( pFilterStr )
    {
        STD_STRCPY(aFileterStr, pFilterStr);
    }

    char aSaveGameStr[512] = { 0 };
    const char* pSaveGMStr = jonesString_GetString("JONES_STR_SAVEGM");
    if ( pSaveGMStr )
    {
        STD_STRCPY(aSaveGameStr, pSaveGMStr);
    }

    if ( !aFileterStr[0] || !aSaveGameStr[0] )
    {
        return 2;
    }

    GetWindowLongPtr(hWnd, GWL_HINSTANCE); // ??

    CHAR aCurDir[128] = { 0 };
    GetCurrentDirectory(STD_ARRAYLEN(aCurDir), aCurDir);

    OPENFILENAME ofn = { 0 };
    ofn.lStructSize   = sizeof(OPENFILENAME);
    ofn.hwndOwner     = hWnd;
    ofn.hInstance     = (HINSTANCE)GetWindowLongPtr(hWnd, GWL_HINSTANCE);

    char aFilterStr[128] = { 0 };
    STD_FORMAT(aFilterStr, aFileterStr, "(.nds)");
    STD_FORMAT(aFilterStr, "%s%c%s%c", aFilterStr, '\0', "*.nds", '\0'); // Fixed: filtering only by .nds files. Note, 2 terminal nulls are required, that's way null at the end

    ofn.lpstrFilter = aFilterStr;

    memset(&ofn.lpstrCustomFilter, 0, 12);

    char aFilename[JONESCONFIG_GAMESAVE_FILEPATHSIZE] = { 0 };
    aFilename[0]        = 0;
    ofn.lpstrFile       = aFilename;
    ofn.nMaxFile        = STD_ARRAYLEN(aFilename);

    char aFileTitle[JONESCONFIG_GAMESAVE_FILEPATHSIZE] = { 0 };
    ofn.lpstrFileTitle   = aFileTitle;
    ofn.nMaxFileTitle    = STD_ARRAYLEN(aFileTitle);

    SaveGameDialogData dialogData = { 0 };
    dialogData.hThumbnail         = NULL;
    ofn.lCustData                 = (LPARAM)&dialogData;

    ofn.lpstrInitialDir = sithGetSaveGamesDir();
    ofn.lpstrTitle      = aSaveGameStr;
    ofn.Flags           = OFN_EXPLORER | OFN_NONETWORKBUTTON | OFN_NOVALIDATE | OFN_ENABLETEMPLATE | OFN_ENABLEHOOK | OFN_HIDEREADONLY;
    ofn.nFileOffset     = 0;
    ofn.nFileExtension  = 0;
    ofn.lpstrDefExt     = "nds";
    ofn.lpfnHook        = jonesConfig_SaveGameDialogHookProc;
    ofn.lpTemplateName  = MAKEINTRESOURCE(154);

    uint32_t width, height;
    JonesDisplaySettings* pSettings = JonesMain_GetDisplaySettings();
    if ( pSettings && pSettings->bWindowMode )
    {
        RECT rect;
        GetWindowRect(GetDesktopWindow(), &rect);
        height = rect.bottom - rect.top;
        width  = rect.right - rect.left;
    }
    else
    {
        stdDisplay_GetBackBufferSize(&width, &height);
    }

    BOOL res = JonesDialog_ShowFileSelectDialog(&ofn,/*bOpen=*/0);
    SetCurrentDirectory(aCurDir);

    if ( !res || !pOutFilePath )
    {
        return 2;
    }

    // Fixed: Use the OPENFILENAME buffer size when adding the savegame extension.
    stdFnames_ChangeExtEx(ofn.lpstrFile, ofn.nMaxFile, "nds");
    stdUtil_StringCopy(pOutFilePath, JONESCONFIG_GAMESAVE_FILEPATHSIZE, ofn.lpstrFile);
    return 1;
}

UINT_PTR CALLBACK jonesConfig_SaveGameDialogHookProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch ( uMsg )
    {
        case WM_INITDIALOG:
        {
            return jonesConfig_SaveGameDialogInit(hDlg, wParam, (LPOPENFILENAME)lParam);
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
            HDC* hdc = (HDC*)GetWindowLongPtr(hDlg, DWL_USER);
            DeleteDC(*hdc);
            *hdc = NULL;
            return 1;
        }
        case WM_NOTIFY:
        {
            SaveGameDialogData* pData  = (SaveGameDialogData*)GetWindowLongPtr(hDlg, DWL_USER);
            switch ( ((LPNMHDR)lParam)->code )
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

                        if ( (SetCurrentDirectory(aSearchPath) || strlen(aPath)) && (!pFilename || streq(pFilename, pOfNotify->lpOFN->lpstrDefExt)) )
                        {
                            SetCurrentDirectory(aCurDir);
                            bValidFile = true;
                        }

                        size_t nOccurrence = strcspn(aFilename, "?*");
                        if ( nOccurrence != strlen(aFilename) )
                        {
                            bValidFile = false;
                        }

                        for ( size_t levelNum = 0; levelNum < JONESLEVEL_BONUSLEVELNUM; ++levelNum )
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

                        // Fixed: Use bounded extension replacement for the fixed-size path buffer.
                        stdFnames_ChangeExtEx(aFilename, STD_ARRAYLEN(aFilename), pOfNotify->lpOFN->lpstrDefExt); // Fixed: Added .nds extension to filename here to make sure filename "QUICKSAVE" without ".nds" extension doesn't slip over
                        if ( streqi(aFilename, aQuckSaveFilename) )
                        {
                            bValidFile = false;
                        }
                    }

                    if ( pOfNotify->lpOFN->nFileOffset < 0x8000 && bValidFile )
                    {
                        if ( strlen(aPath) )
                        {
                            STD_STRCPY(pData->aFilePath, aFilename);
                            // Fixed: Use bounded extension replacement for the fixed-size path buffer.
                            stdFnames_ChangeExtEx(pData->aFilePath, STD_ARRAYLEN(pData->aFilePath), pOfNotify->lpOFN->lpstrDefExt);

                            if ( jonesConfig_ShowOverwriteSaveGameDlg(hDlg, pData->aFilePath) == 1 )
                            {
                                return 0;
                            }
                            else
                            {
                                SetWindowLongPtr(hDlg, DWL_MSGRESULT, 1);
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

                        SetWindowLongPtr(hDlg, DWL_MSGRESULT, 1);
                        return 1;
                    }
                }
                case CDN_INITDONE:
                {
                    HWND hThumbnail = GetDlgItem(hDlg, 1125);
                    SetWindowLongPtr(hThumbnail, GWL_USERDATA, 154);

                    HWND hIcon = GetDlgItem(hDlg, 1115);
                    SetWindowLongPtr(hIcon, GWL_USERDATA, 154);

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

int J3DAPI jonesConfig_SaveGameDialogInit(HWND hDlg, int a2, LPOPENFILENAME lpOfn)
{
    J3D_UNUSED(a2);

    SaveGameDialogData* pData = (SaveGameDialogData*)lpOfn->lCustData;
    pData->hThumbnail = sithGamesave_CreateThumbnail();

    HWND hThumbnail = GetDlgItem(hDlg, 1125);
    pData->hdcThumbnail = CreateCompatibleDC(GetDC(hThumbnail));

    hThumbnail = GetDlgItem(hDlg, 1125); // ??
    pData->pfThumbnailProc = GetWindowLongPtr(hThumbnail, GWL_WNDPROC);
    SetWindowLongPtr(hThumbnail, GWL_WNDPROC, (LONG_PTR)jonesConfig_SaveGameThumbnailPaintProc);

    SetWindowLongPtr(hDlg, DWL_USER, (LONG_PTR)pData);

    RECT thumbRect;
    GetClientRect(hThumbnail, &thumbRect);

    // Added: Calculate thumbRect to screen
    POINT pt = { thumbRect.left, thumbRect.top };
    ClientToScreen(hThumbnail, &pt);
    OffsetRect(&thumbRect, pt.x - thumbRect.left, pt.y - thumbRect.top);

    // Changed: Added scale the thumb size to DPI
    UINT systemDPI = GetDpiForWindow(hDlg);
    double dpiScale = (double)systemDPI / USER_DEFAULT_SCREEN_DPI;
    LONG tbImageWidth  = (LONG)ceil((double)SITHSAVEGAME_THUMB_WIDTH * dpiScale);
    LONG tbImageHeight = (LONG)ceil((double)SITHSAVEGAME_THUMB_HEIGHT * dpiScale);

    LONG thumbWidth  = tbImageWidth - (thumbRect.right - thumbRect.left);
    LONG thumbHeight = tbImageHeight - (thumbRect.bottom - thumbRect.top);

    if ( thumbWidth != 0 || thumbHeight != 0 )
    {
        RECT dlgWndRect;
        GetWindowRect(hDlg, &dlgWndRect);

        RECT thumbWndRect;
        GetWindowRect(hThumbnail, &thumbWndRect);

        // Added: Added padding. This should fix drawing all components right of file list
        const int pad = (int)ceil(10.0f * dpiScale);
        MoveWindow(hDlg,
            dlgWndRect.left,
            dlgWndRect.top,
            thumbWidth + dlgWndRect.right - dlgWndRect.left + pad,
            dlgWndRect.bottom - dlgWndRect.top + pad,
            TRUE
        );

        MoveWindow(
            hThumbnail,
            (thumbWndRect.left - dlgWndRect.left) + (thumbRect.left - thumbWndRect.left), // Added: dlgX + borderSize (thumbRect.left - thumbWndRect.left)... This should fix rendering image on different DPIs
            thumbWndRect.top - dlgWndRect.top,
            thumbWidth + thumbWndRect.right - thumbWndRect.left,
            thumbHeight + thumbWndRect.bottom - thumbWndRect.top,
            TRUE
        );
    }

    HWND h1152 = GetDlgItem(GetParent(hDlg), 1152);  // some other dlgItem
    SendMessage(h1152, EM_LIMITTEXT, 60u, 0);
    return 1;
}

LRESULT CALLBACK jonesConfig_SaveGameThumbnailPaintProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    HWND hwndParent = (HWND)GetWindowLongPtr(hWnd, GWL_HWNDPARENT);
    SaveGameDialogData* pData = (SaveGameDialogData*)GetWindowLongPtr(hwndParent, DWL_USER);
    if ( uMsg != WM_PAINT || !pData || !pData->hThumbnail )
    {
        if ( !pData )
        {
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

int J3DAPI jonesConfig_ShowOverwriteSaveGameDlg(HWND hWnd, const char* aFilePath)
{
    GameSaveMsgBoxData data;
    data.dialogID     = 214;
    data.pNdsFilePath = aFilePath;
    return JonesDialog_ShowDialog(MAKEINTRESOURCE(214), hWnd, jonesConfig_SaveGameMsgBoxProc, (LPARAM)&data);
}

INT_PTR CALLBACK jonesConfig_SaveGameMsgBoxProc(HWND hWnd, UINT umsg, WPARAM wParam, LPARAM lParam)
{
    int bSuccess;

    if ( umsg == WM_DESTROY )
    {
        jonesConfig_ResetDialogFont(hWnd, jonesConfig_hFontGameSaveMsgBox);
        return 1;
    }

    else if ( umsg == WM_INITDIALOG )
    {
        jonesConfig_hFontGameSaveMsgBox = jonesConfig_InitDialog(hWnd, 0, ((GameSaveMsgBoxData*)lParam)->dialogID);
        bSuccess = jonesConfig_GameSaveSetData(hWnd, wParam, (GameSaveMsgBoxData*)lParam);
        SetWindowPos(hWnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_SHOWWINDOW | SWP_NOMOVE | SWP_NOSIZE);
        return bSuccess;
    }
    else
    {
        if ( umsg == WM_COMMAND )
        {
            jonesConfig_MsgBoxDlg_HandleWM_COMMAND(hWnd, (uint16_t)wParam);
        }

        return 0;
    }
}

int J3DAPI jonesConfig_GameSaveSetData(HWND hDlg, int a2, GameSaveMsgBoxData* pData)
{
    J3D_UNUSED(a2);

    const char* pNdsFilePath;
    const char* pOverwriteText;
    const char* pProgressText;
    HWND hMsgTextCntrl;
    char aText[256];

    if ( !pData )
    {
        return 0;
    }

    if ( pData->dialogID == 163 ) // if  load game message box
    {
        pProgressText = jonesString_GetString("JONES_STR_PROGRESS1");
        if ( pProgressText )
        {
            hMsgTextCntrl = GetDlgItem(hDlg, 1126);
            jonesConfig_SetTextControl(hDlg, hMsgTextCntrl, pProgressText);
        }
    }
    else if ( pData->dialogID == 214 ) // if save game overwrite message box
    {
        pNdsFilePath = pData->pNdsFilePath;
        pOverwriteText = jonesString_GetString("JONES_STR_OVERWRITE");
        if ( pOverwriteText )
        {
            memset(aText, 0, sizeof(aText));
            STD_FORMAT(aText, pOverwriteText, pNdsFilePath + 4);
            hMsgTextCntrl = GetDlgItem(hDlg, 1197);
            jonesConfig_SetTextControl(hDlg, hMsgTextCntrl, aText);
        }
    }

    SetWindowLongPtr(hDlg, DWL_USER, (LONG_PTR)pData);
    return 1;
}

void J3DAPI jonesConfig_MsgBoxDlg_HandleWM_COMMAND(HWND hWnd, int nResult)
{
    GameSaveMsgBoxData* pData = (GameSaveMsgBoxData*)GetWindowLongPtr(hWnd, DWL_USER);
    if ( nResult > 0 )
    {
        if ( nResult <= 2 )
        {
            EndDialog(hWnd, nResult);
            return;
        }

        if ( nResult != 1187 )
        {
            return;
        }

        if ( pData->dialogID == 211 && jonesConfig_GetSaveGameFilePath(hWnd, (char*)pData->pNdsFilePath) != 1 )// in exit dialoge save game button was clicked
        {
            EndDialog(hWnd, 2);
        }
        else
        {
            EndDialog(hWnd, nResult);
        }
    }
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

    OPENFILENAME ofn      = { 0 };
    ofn.lStructSize       = sizeof(OPENFILENAME);
    ofn.hwndOwner         = hWnd;
    ofn.hInstance         = (HINSTANCE)GetWindowLongPtr(hWnd, GWL_HINSTANCE);
    ofn.lpstrFilter       = aFilterStr;
    ofn.lpstrCustomFilter = NULL;

    char aLastFile[JONESCONFIG_GAMESAVE_FILEPATHSIZE] = { 0 };
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

    char aFileTitle[JONESCONFIG_GAMESAVE_FILEPATHSIZE] = { 0 };
    ofn.lpstrFileTitle  = aFileTitle;
    ofn.nMaxFileTitle   = STD_ARRAYLEN(aFileTitle);

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

    BOOL bHasFilePath = JonesDialog_ShowFileSelectDialog((LPOPENFILENAME)&ofn, 1);
    SetCurrentDirectory(aCwd);
    if ( !bHasFilePath && !data.unknown36 || !pDestNdsPath )
    {
        return 2;
    }

    if ( bHasFilePath )
    {
        // Fixed: Use the OPENFILENAME buffer size when adding the savegame extension.
        stdFnames_ChangeExtEx(ofn.lpstrFile, ofn.nMaxFile, "nds");
        stdUtil_StringCopy(pDestNdsPath, JONESCONFIG_GAMESAVE_FILEPATHSIZE, ofn.lpstrFile);
    }
    else
    {
        // Fixed: Use bounded extension replacement for the fixed-size path buffer.
        stdFnames_ChangeExtEx(data.aFilePath, STD_ARRAYLEN(data.aFilePath), "nds");
        stdUtil_StringCopy(pDestNdsPath, JONESCONFIG_GAMESAVE_FILEPATHSIZE, data.aFilePath);
    }

    return 1;
}

void* J3DAPI jonesConfig_sub_405F60(HWND hWnd)
{
    return (void*)GetWindowLongPtr(hWnd, DWL_USER);
}

UINT_PTR CALLBACK jonesConfig_LoadGameDialogHookProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch ( uMsg )
    {
        case WM_INITDIALOG:
        {
            return jonesConfig_LoadGameDialogInit(hDlg, wParam, (LPOPENFILENAME)lParam);
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
                    LPOPENFILENAME pofn = pOfNotify->lpOFN;
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

                            SetWindowLongPtr(hDlg, DWL_MSGRESULT, 1);
                            return TRUE;
                        }

                        pErrorString = jonesString_GetString("JONES_STR_NOFILE");
                        if ( !pErrorString )
                        {
                            SetWindowLongPtr(hDlg, DWL_MSGRESULT, 1);
                            return TRUE;
                        }
                    }
                    else
                    {
                        pErrorString = jonesString_GetString("JONES_STR_INVALIDFILE");
                        if ( !pErrorString )
                        {
                            SetWindowLongPtr(hDlg, DWL_MSGRESULT, 1);
                            return TRUE;
                        }
                    }

                    // Get selected filename
                    CHAR aMessage[256]  = { 0 };
                    char aFilename[128] = { 0 };
                    SendMessage(GetParent(hDlg), CDM_GETSPEC, STD_ARRAYLEN(aFilename), (LPARAM)aFilename);
                    STD_FORMAT(aMessage, pErrorString, aFilename);

                    jonesConfig_ShowMessageDialog(hDlg, "JONES_STR_LOADGM", aMessage, 139);
                    SetWindowLongPtr(hDlg, DWL_MSGRESULT, 1);
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

                        if ( pExt && streq(pExt, "nds") )
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
                    SetWindowLongPtr(hBtn, GWL_USERDATA, 159);

                    HWND hIcon = GetDlgItem(hDlg, 1117);
                    SetWindowLongPtr(hIcon, GWL_USERDATA, 159);

                    HWND hThumbnail = GetDlgItem(hDlg, 1163);
                    SetWindowLongPtr(hThumbnail, GWL_USERDATA, 159);
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

int J3DAPI jonesConfig_ShowLoadGameWarningMsgBox(HWND hWnd)
{
    GameSaveMsgBoxData data;

    data.pNdsFilePath = 0;
    data.dialogID = 163;
    return JonesDialog_ShowDialog(MAKEINTRESOURCE(163), hWnd, jonesConfig_SaveGameMsgBoxProc, (LPARAM)&data);
}

BOOL J3DAPI jonesConfig_LoadGameDialogInit(HWND hDlg, int a2, LPOPENFILENAME pofn)
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
        (LONG)ceil(11.0f * dpiScale) + dlgWinRect.bottom - dlgWinRect.top, // Added: Added padding  11 * dpiScale. This should fix drawing all components right of file list
        TRUE
    );

    MoveWindow(
        hThumbnail,
        (thumbWinRect.left - dlgWinRect.left),
        thumbWinRect.top - dlgWinRect.top,
        thmbWidth + thumbWinRect.right - thumbWinRect.left,
        thmbHeight + thumbWinRect.bottom - thumbWinRect.top,
        TRUE
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

LRESULT CALLBACK jonesConfig_LoadGameThumbnailPaintProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    HWND hwndParent = (HWND)GetWindowLongPtr(hWnd, GWL_HWNDPARENT);
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

int J3DAPI jonesConfig_ShowGamePlayOptions(HWND hWnd)
{
    GetWindowLongPtr(hWnd, GWL_HINSTANCE); // TODO: useless
    return JonesDialog_ShowDialog(MAKEINTRESOURCE(112), hWnd, jonesConfig_GamePlayOptionsProc, 0);
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
        int difficulty = SendMessage(hDifSlideer, TBM_GETPOS, 0, 0);
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
            SetWindowText(hDifText, pDifficultyStr);
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

    const int maxPos = SendMessage(hWnd, TBM_GETRANGEMAX, 0, 0);
    const int minPos = SendMessage(hWnd, TBM_GETRANGEMIN, 0, 0);
    const int curPos = SendMessage(hWnd, TBM_GETPOS, 0, 0);
    SendMessage(hWnd, TBM_GETPAGESIZE, 0, 0);  // ???

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
        SendMessage(hWnd, TBM_SETPOS, /*redraw=*/1u, newPos);
    }
}

int J3DAPI jonesConfig_GamePlayOptionsInitDlg(HWND hDlg)
{
    // Shot Text option
    HWND hCBShowText = GetDlgItem(hDlg, 1052);
    int bShowText = sithVoice_GetShowText();
    Button_SetCheck(hCBShowText, bShowText);

    // Show Hints option
    HWND hCBShowHints = GetDlgItem(hDlg, 1051);
    int bShowHints = sithOverlayMap_GetShowHints();
    Button_SetCheck(hCBShowHints, bShowHints);

    // RotateMap option
    HWND hCBRotateMap = GetDlgItem(hDlg, 1204);
    int  bRotateMap = sithOverlayMap_GetMapRotation();
    Button_SetCheck(hCBRotateMap, bRotateMap);

    // Default to Run option
    CheckDlgButton(hDlg, 1202, jonesConfig_gamePlayOptions_bDefaultRun);// CB default run

    // Added: Check box "Aim Reticle"
    // Get the position of the base checkbox
    RECT rectCBShowText;
    GetWindowRect(hCBShowText, &rectCBShowText);

    // Convert screen coordinates to client coordinates
    POINT pt = { rectCBShowText.left, rectCBShowText.top };
    ScreenToClient(hDlg, &pt);

    // Calculate the position for the new checkbox
    UINT dpi = GetDpiForWindow(hDlg);
    int spacing = MulDiv(8, dpi, USER_DEFAULT_SCREEN_DPI);

    int x = pt.x;
    int y = pt.y + (rectCBShowText.bottom - rectCBShowText.top) + spacing;

    // Create the reticle checkbox
    HWND hCBReticle = CreateWindow(
        "BUTTON",               // Class name for button/checkbox
        "Aim Reticle",          // Text displayed on the checkbox
        WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
        x,
        y,
        150,                   // Width
        20,                    // Height
        hDlg,                  // Parent window handle (the dialog)
        (HMENU)1053,           // Control ID
        GetModuleHandle(NULL), // Instance handle
        NULL                   // Additional creation config
    );
    J3D_UNUSED(hCBReticle);

    CheckDlgButton(hDlg, 1053, JonesReticle_IsEnabled());

    // Added: Check box "Blood Splatter Effect"
    CreateWindow(
        "BUTTON",
        "Blood Splatter Effect",
        WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
        x,
        y + 20 + spacing,
        200,                   // Width
        20,                    // Height
        hDlg,                  // Parent window handle (the dialog)
        (HMENU)1054,           // Control ID
        GetModuleHandle(NULL), // Instance handle
        NULL                   // Additional creation config
    );

    CheckDlgButton(hDlg, 1054, sithWeapon_IsBloodSplatterEnabled());

    // Difficulty slider and text
    HWND hDifSlider = GetDlgItem(hDlg, 1050); // Difficulty slider control

    size_t maxDifficulty = jonesInventory_GetMaxDifficultyLevel();
    SendMessage(hDifSlider, TBM_SETRANGE, 1u, (uint16_t)maxDifficulty << 16);// LWORD is min and HIWORD is max range

    int curDifficulty = sithGetGameDifficulty();
    SendMessage(hDifSlider, TBM_SETPOS, 1u, curDifficulty);

    SendMessage(hDifSlider, TBM_SETTICFREQ, 1u, 0);
    SendMessage(hDifSlider, TBM_SETPAGESIZE, 0, 1);

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
        SetWindowText(hDifText, pDifficultyStr);
    }

    // Added: Resize dialog to fit in new check boxes
    RECT rectDlg;
    GetWindowRect(hDlg, &rectDlg);
    SetWindowPos(hDlg, NULL, 0, 0, rectDlg.right - rectDlg.left, (rectDlg.bottom - rectDlg.top) + 66, SWP_NOZORDER | SWP_NOMOVE | SWP_NOACTIVATE);

    // Added: Move OK & Cancel buttons down
    HWND hBtnOk     = GetDlgItem(hDlg, 1);
    RECT rectBtnOk;
    GetWindowRect(hBtnOk, &rectBtnOk);
    POINT ptBtnOk = { rectBtnOk.left, rectBtnOk.top + (rectBtnOk.bottom - rectBtnOk.top) / 2 + 48 };
    ScreenToClient(hDlg, &ptBtnOk);
    SetWindowPos(hBtnOk, NULL, ptBtnOk.x, ptBtnOk.y, 0, 0, SWP_NOZORDER | SWP_NOSIZE | SWP_NOACTIVATE);

    HWND hBtnCancel = GetDlgItem(hDlg, 2);
    RECT rectBtnCancel;
    GetWindowRect(hBtnCancel, &rectBtnCancel);
    POINT ptBtnCancel = { rectBtnCancel.left, rectBtnCancel.top + (rectBtnCancel.bottom - rectBtnCancel.top) / 2 + 48 };
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
        int bShowText = Button_GetState(hCBShowText);
        sithVoice_ShowText(bShowText);

        bShowText = sithVoice_GetShowText();
        stdConfig_SetBool(SITHVOICE_CFG_GAMEPLAY_SHOWTEXT, bShowText);

        // Get & save map rotation option
        HWND hCBMapRotate = GetDlgItem(hDlg, 1204);
        int bRotateMap = Button_GetState(hCBMapRotate);
        sithOverlayMap_EnableMapRotation(bRotateMap);

        bRotateMap = sithOverlayMap_GetMapRotation();
        stdConfig_SetBool(SITHOVERLAYMAP_CFG_GAMEPLAY_MAPROTATION, bRotateMap);

        // Get & save show hint option
        HWND hCBShowHints = GetDlgItem(hDlg, 1051);
        int bShowHints = Button_GetState(hCBShowHints);
        sithOverlayMap_SetShowHints(bShowHints);

        bShowHints = sithOverlayMap_GetShowHints();
        stdConfig_SetBool(SITHOVERLAYMAP_CFG_GAMEPLAY_SHOWHINTS, bShowHints);
        // Get & save difficulty
        HWND hDifSlideer = GetDlgItem(hDlg, 1050);
        int difficulty = SendMessage(hDifSlideer, TBM_GETPOS, 0, 0);
        sithSetGameDifficulty(difficulty);

        difficulty = sithGetGameDifficulty();
        stdConfig_SetInt(SITH_CFG_GAMEPLAY_DIFFICULTY, difficulty);

        // Get & save default to run option
        HWND hCBRun = GetDlgItem(hDlg, 1202);
        jonesConfig_gamePlayOptions_bDefaultRun = Button_GetCheck(hCBRun);
        stdConfig_SetBool(JONESCONFIG_CFG_GAMEPLAY_DEFAULTRUN, jonesConfig_gamePlayOptions_bDefaultRun);

        if ( jonesConfig_gamePlayOptions_bDefaultRun )
        {
            sithControl_g_controlOptions |= 0x02;
        }
        else
        {
            sithControl_g_controlOptions &= ~0x02;
        }

        // Added
        // Enable/Disable aim reticle
        int bReticleEnabled = IsDlgButtonChecked(hDlg, 1053);
        JonesReticle_Enable(bReticleEnabled);

        // Added
        // Enable/Disable blood splatter
        int bSplatter = IsDlgButtonChecked(hDlg, 1054);
        sithWeapon_EnableBloodSplatter(bSplatter);

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
        sithControl_RebindMouse();
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

void J3DAPI jonesConfig_FreeControlScheme(JonesControlsScheme* pScheme)
{
    for ( size_t i = 0; i < STD_ARRAYLEN(pScheme->aActions); ++i )
    {
        //if ( (JonesControlsScheme *)((char *)pScheme + 36 * i) != (JonesControlsScheme *)-4 )
        {
            memset(pScheme->aActions[i], 0, STD_ARRAYLEN(pScheme->aActions[i]));
        }
    }

    stdMemory_Free(pScheme);
}

void J3DAPI jonesConfig_FreeControlConfigEntry(JonesControlsConfig* pConfig)
{
    for ( size_t i = 0; i < pConfig->numSchemes; ++i )
    {
        JonesControlsScheme* pScheme = &pConfig->aSchemes[i];
        if ( pScheme )
        {
            for ( size_t j = 0; j < STD_ARRAYLEN(pScheme->aActions); ++j )
            {
                //if ( (JonesControlsScheme*)((char*)pScheme + 36 * j) != (JonesControlsScheme*)-4 )
                {
                    memset(pScheme->aActions[j], 0, STD_ARRAYLEN(pScheme->aActions[j]));
                }
            }
        }
    }

    if ( pConfig->paDeleteSchemes )
    {
        stdMemory_Free(pConfig->paDeleteSchemes);
        pConfig->paDeleteSchemes = NULL;
    }

    if ( pConfig->aSchemes )
    {
        stdMemory_Free(pConfig->aSchemes);
        pConfig->aSchemes = NULL;
    }
}

int J3DAPI jonesConfig_CopyControlConfig(JonesControlsConfig* pSrc, JonesControlsConfig* pDst)
{
    if ( !pSrc )
    {
        return 0;
    }

    if ( !pDst )
    {
        pDst = (JonesControlsConfig*)STDMALLOC(sizeof(JonesControlsConfig));
        if ( !pDst )
        {
            return 0;
        }
    }

    pDst->numSchemes       = pSrc->numSchemes;
    pDst->selectedShemeIdx = pSrc->selectedShemeIdx;

    JonesControlsScheme* apSchemes = jonesConfig_CloneControlSchemesList(pSrc->aSchemes, 0, pSrc->numSchemes);
    if ( !apSchemes && pSrc->aSchemes )
    {
        return 0;
    }

    if ( pDst->aSchemes )
    {
        jonesConfig_FreeControlConfigEntry(pDst);
        pDst->aSchemes = NULL;
    }

    pDst->aSchemes = apSchemes;

    size_t bufSize = 10 - pSrc->numSchemes % 10;
    if ( bufSize <= 0 || bufSize >= 10 )
    {
        return 1;
    }

    pDst->aSchemes = (JonesControlsScheme*)STDREALLOC(pDst->aSchemes, sizeof(JonesControlsScheme) * (bufSize + pSrc->numSchemes));
    memset(&pDst->aSchemes[pSrc->numSchemes], 0, sizeof(JonesControlsScheme) * bufSize);
    return 1;
}

JonesControlsScheme* J3DAPI jonesConfig_CloneControlSchemesList(JonesControlsScheme* aSchemes, size_t start, size_t numSchemes)
{
    JonesControlsScheme* aNewSchemes = (JonesControlsScheme*)STDMALLOC(sizeof(JonesControlsScheme) * numSchemes);
    if ( !aNewSchemes || !aSchemes )
    {
        return 0;
    }

    memset(aNewSchemes, 0, sizeof(JonesControlsScheme) * numSchemes);

    for ( size_t i = start; i < numSchemes + start; ++i )
    {
        STD_STRCPY(aNewSchemes[i - start].aName, aSchemes[i].aName);

        for ( size_t j = 0; j < STD_ARRAYLEN(aSchemes[i].aActions); ++j )
        {
            memset(aNewSchemes[i - start].aActions[j], 0, STD_ARRAYLEN(aNewSchemes[i - start].aActions[j]));
            for ( size_t k = 0; k <= JONESCONTROL_ACTION_GETNUMBINDS(aSchemes[i].aActions[j]); ++k )
            {
                aNewSchemes[i - start].aActions[j][k] = aSchemes[i].aActions[j][k];
            }
        }
    }

    return aNewSchemes;
}

int J3DAPI jonesConfig_ShowControlOptions(HWND hWnd)
{
    if ( !jonesConfig_ControlOptions_InitControlsConfig() )
    {
        return 2;
    }

    JonesControlsConfig config = { 0 };
    if ( !jonesConfig_CopyControlConfig(&jonesConfig_controlOptions_curControlConfig, &config) )
    {
        return 2;
    }

    GetWindowLongPtr(hWnd, GWL_HINSTANCE); // ???

    int result = JonesDialog_ShowDialog(MAKEINTRESOURCE(111), hWnd, jonesConfig_ControlOptionsDialogProc, (LPARAM)&config);
    if ( result == 1 )
    {
        // Added: Make sure we have valid index
        if ( config.numSchemes <= config.selectedShemeIdx )
        {
            // Maybe add log
            return 2; // cancel
        }

        // Set and bind selected scheme as active game control set
        stdConfig_SetString(JONESCONFIG_CFG_CONTROLS_CONFIGFILE, config.aSchemes[config.selectedShemeIdx].aName);
        jonesConfig_BindControls(&config.aSchemes[config.selectedShemeIdx]);

        jonesConfig_FreeControlConfigEntry(&config);
        jonesConfig_FreeControlConfigEntry(&jonesConfig_controlOptions_curControlConfig);
        jonesConfig_controlOptions_curControlConfig.numSchemes = 0;

        // Enable disable mouse
        int bMouseEnabled = stdConfig_GetBool(JONESCONTROL_CFG_CONTROLS_MOUSE, false);
        jonesConfig_EnableMouseControl(bMouseEnabled);
    }
    else
    {
        jonesConfig_FreeControlConfigEntry(&config);
    }

    return result;
}

int jonesConfig_ControlOptions_InitControlsConfig(void)
{
    JonesControlsConfig config = { 0 };
    config.selectedShemeIdx = -1;

    if ( !jonesConfig_ControlOptions_SetAllDefaultControlSchemes(&config) || !jonesConfig_ControlOptions_LoadAllControlSchemes(&config) )
    {
        jonesConfig_FreeControlConfigEntry(&config);
        return 0;
    }
    else if ( !jonesConfig_CopyControlConfig(&config, &jonesConfig_controlOptions_curControlConfig) )
    {
        jonesConfig_FreeControlConfigEntry(&jonesConfig_controlOptions_curControlConfig);
        jonesConfig_FreeControlConfigEntry(&config);
        return 0;
    }
    else
    {
        jonesConfig_FreeControlConfigEntry(&config);
        return 1;
    }
}

int J3DAPI jonesConfig_ControlOptions_LoadAllControlSchemes(JonesControlsConfig* pConfig)
{
    char aCurDir[128] = { 0 };
    GetCurrentDirectory(STD_ARRAYLEN(aCurDir), aCurDir);
    if ( !SetCurrentDirectory(jonesConfig_aKeySetsDirPath) )
    {
        return 1;
    }

    WIN32_FIND_DATAA fileData;
    HANDLE hFile = FindFirstFile("*.kfg", &fileData);
    if ( hFile == INVALID_HANDLE_VALUE )
    {
        SetCurrentDirectory(aCurDir);
        return 1;
    }

    while ( 1 )
    {
        if ( (pConfig->numSchemes % 10) == 0 )
        {
            pConfig->aSchemes = (JonesControlsScheme*)STDREALLOC(pConfig->aSchemes, sizeof(JonesControlsScheme) * (pConfig->numSchemes + 10));
        }

        if ( !pConfig->aSchemes )
        {
            break;
        }

        if ( jonesConfig_LoadControlScheme(fileData.cFileName, &pConfig->aSchemes[pConfig->numSchemes]) )
        {
            ++pConfig->numSchemes;
        }

        if ( !FindNextFile(hFile, &fileData) )
        {
            FindClose(hFile);
            SetCurrentDirectory(aCurDir);
            return 1;
        }
    }

    SetCurrentDirectory(aCurDir);
    return 0;
}

int J3DAPI jonesConfig_ControlOptions_SetAllDefaultControlSchemes(JonesControlsConfig* pConfig)
{
    for ( size_t i = 0; i < JONESCONFIG_NUMDEFAULTCONTROLSCHEMES; i++ )
    {
        if ( !(pConfig->numSchemes % 10) )
        {
            pConfig->aSchemes = (JonesControlsScheme*)STDREALLOC(pConfig->aSchemes, sizeof(JonesControlsScheme) * (pConfig->numSchemes + 10));
        }

        if ( !pConfig->aSchemes )
        {
            return 0;
        }

        JonesControlsScheme* pScheme = &pConfig->aSchemes[pConfig->numSchemes];
        if ( !pScheme || !jonesConfig_SetDefaultControlScheme(pScheme, i) )
        {
            return 0;
        }

        ++pConfig->numSchemes;
    }

    return 1;
}

INT_PTR CALLBACK jonesConfig_ControlOptionsDialogProc(HWND hwnd, UINT umsg, WPARAM wparam, LPARAM lparam)
{
    if ( umsg == WM_DESTROY )
    {
        jonesConfig_ResetDialogFont(hwnd, jonesConfig_hFontControlOptions);
        return 1;
    }
    else if ( umsg == WM_INITDIALOG )
    {
        jonesConfig_hFontControlOptions = jonesConfig_InitDialog(hwnd, 0, 111);
        int res = jonesConfig_InitControlOptionsDialog(hwnd, wparam, (JonesControlsConfig*)lparam);
        SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_SHOWWINDOW | SWP_NOMOVE | SWP_NOSIZE);
        return res;
    }
    else
    {
        if ( umsg == WM_COMMAND )
        {
            jonesConfig_ControlOptions_HandleWM_COMMAND(hwnd, LOWORD(wparam), lparam, HIWORD(wparam));
        }

        return 0;
    }
}

int J3DAPI jonesConfig_InitControlOptionsDialog(HWND hDlg, int a2, JonesControlsConfig* pConfig)
{
    J3D_UNUSED(a2);

    char aSelectedCfgName[128] = { 0 };
    if ( pConfig->selectedShemeIdx == -1 )
    {
        stdConfig_GetString(JONESCONFIG_CFG_CONTROLS_CONFIGFILE, aSelectedCfgName, STD_ARRAYLEN(aSelectedCfgName), "");
        if ( strlen(aSelectedCfgName) == 0 )
        {
            // Load default config
            const char* pDfltKeyset = NULL;
            size_t maxJoyButtons = stdControl_GetMaxJoystickButtons();
            if ( maxJoyButtons == 4 )
            {
                pDfltKeyset = jonesString_GetString(jonesConfig_aDfltKeySetNames[1]);
            }
            else
            {
                pDfltKeyset = maxJoyButtons == 8
                    ? jonesString_GetString(jonesConfig_aDfltKeySetNames[2])
                    : jonesString_GetString(jonesConfig_aDfltKeySetNames[0]);
            }

            if ( pDfltKeyset )
            {
                STD_STRCPY(aSelectedCfgName, pDfltKeyset);
            }
        }
    }

    pConfig->selectedShemeIdx = 0;

    HWND hDlgItem = GetDlgItem(hDlg, 1047);
    HDC hCfgListDC = GetDC(hDlgItem);

    for ( size_t i = 0; i < pConfig->numSchemes; ++i )
    {
        ListBox_AddString(hDlgItem, pConfig->aSchemes[i].aName);

        pConfig->aSchemes[i].bModified = 0;
        if ( streq(aSelectedCfgName, pConfig->aSchemes[i].aName) )
        {
            pConfig->selectedShemeIdx = i;
        }

        SIZE sz;
        size_t nameLen = strlen(pConfig->aSchemes[i].aName);
        GetTextExtentPoint(hCfgListDC, pConfig->aSchemes[i].aName, nameLen, &sz);
        if ( sz.cx > pConfig->maxNameLen )
        {
            pConfig->maxNameLen = sz.cx;
        }
    }

    RECT rectList;
    GetClientRect(hDlgItem, &rectList);
    if ( rectList.right - rectList.left < pConfig->maxNameLen )
    {
        ListBox_SetHorizontalExtent(hDlgItem, pConfig->maxNameLen);
    }

    ListBox_SetCurSel(hDlgItem, pConfig->selectedShemeIdx);

    if ( pConfig->selectedShemeIdx < JONESCONFIG_NUMDEFAULTCONTROLSCHEMES )
    {
        // We have only default scheme, disable view scheme button
        const char* pButtonText = jonesString_GetString("JONES_STR_VIEW_SCHEME");
        hDlgItem = GetDlgItem(hDlg, 1022);
        EnableWindow(hDlgItem, 0);
        if ( pButtonText )
        {
            hDlgItem = GetDlgItem(hDlg, 1028);
            SetWindowText(hDlgItem, pButtonText);
        }
    }

    // Mouse checkbox
    hDlgItem = GetDlgItem(hDlg, 1053);
    int bMouseEnabled = stdConfig_GetBool(JONESCONTROL_CFG_CONTROLS_MOUSE, false);
    Button_SetCheck(hDlgItem, bMouseEnabled);

    // Joystick checkbox
    hDlgItem = GetDlgItem(hDlg, 1054);
    if ( stdControl_GetNumJoysticks() )
    {
        int bJoystickEnabled = stdConfig_GetBool(JONESCONTROL_CFG_CONTROLS_CONTROLLER, false);
        Button_SetCheck(hDlgItem, bJoystickEnabled);
    }
    else
    {
        EnableWindow(hDlgItem, 0);
    }

    SetWindowLongPtr(hDlg, DWL_USER, (LONG_PTR)pConfig);
    return 1;
}

void J3DAPI jonesConfig_ControlOptions_HandleWM_COMMAND(HWND hWnd, int ctrlID, LPARAM lParam, unsigned int notificationCode)
{
    J3D_UNUSED(lParam);

    JonesControlsConfig* pConfig = (JonesControlsConfig*)GetWindowLongA(hWnd, DWL_USER);

    if ( ctrlID > 1022 ) // delete button
    {
        switch ( ctrlID )
        {
            case 1028: // Edit Scheme button clicked
                jonesConfig_ControlOptions_EditSelectedScheme(hWnd, pConfig);
                break;

            case 1029: // New Scheme button clicked
                jonesConfig_ControlOptions_CreateNewScheme(hWnd, pConfig, jonesConfig_controlOptions_pCurNewScheme);
                return;

            case 1047: // list control notification
            {
                HWND hDlgList = GetDlgItem(hWnd, 1047);

                pConfig->selectedShemeIdx = ListBox_GetCurSel(hDlgList);
                jonesConfig_ControlOptions_UpdateButtons(hWnd);

                if ( notificationCode == LBN_DBLCLK )
                {
                    jonesConfig_ControlOptions_EditSelectedScheme(hWnd, pConfig);
                }
                break;
            }
        }
    }
    else
    {
        switch ( ctrlID )
        {
            case 1022: // delete scheme button
                jonesConfig_ControlOptions_DeleteScheme(hWnd, pConfig);
                return;

            case 1: // OK button
            {
                HWND hCBMouseCtrl = GetDlgItem(hWnd, 1053);
                int bMouseEnabled = Button_GetCheck(hCBMouseCtrl);

                HWND hCBKoyCtrl = GetDlgItem(hWnd, 1054);
                int bJoyEnabled = Button_GetCheck(hCBKoyCtrl);

                // Delete pending scheme files to be removed
                for ( size_t i = 0; i < pConfig->numDeleteSchemes; ++i )
                {
                    remove(pConfig->paDeleteSchemes[i]);
                }

                pConfig->numDeleteSchemes = 0;
                if ( pConfig->paDeleteSchemes )
                {
                    stdMemory_Free(pConfig->paDeleteSchemes);
                    pConfig->paDeleteSchemes = NULL;
                }

                // Set current selected scheme as current game control set
                if ( !jonesConfig_CopyControlConfig(pConfig, &jonesConfig_controlOptions_curControlConfig) )
                {
                    const char* pErrorText = jonesString_GetString("JONES_STR_NO_MEM_SCHEME");
                    if ( pErrorText )
                    {
                        jonesConfig_ShowMessageDialog(hWnd, "JONES_STR_CTRL_OPTS", pErrorText, 145);
                        InvalidateRect(hWnd, 0, 1);
                    }

                    // Due to error cancel set to cancel action
                    ctrlID = 2; // cancel
                }
                else
                {
                    // Write modified schemes to file
                    for ( size_t i = JONESCONFIG_NUMDEFAULTCONTROLSCHEMES; i < pConfig->numSchemes; ++i )
                    {
                        if ( pConfig->aSchemes[i].bModified == 1 && !jonesConfig_ControlOptions_WriteScheme(&pConfig->aSchemes[i]) )
                        {
                            const char* pErrorText = jonesString_GetString("JONES_STR_NO_MEM_SCHEME");
                            if ( pErrorText )
                            {
                                jonesConfig_ShowMessageDialog(hWnd, "JONES_STR_CTRL_OPTS", pErrorText, 145); // 145 - typewriter icon
                                InvalidateRect(hWnd, 0, 1);
                            }
                        }
                    }
                }

                // Save mouse/joystick settings
                stdConfig_SetBool(JONESCONTROL_CFG_CONTROLS_MOUSE, bMouseEnabled);
                stdConfig_SetBool(JONESCONTROL_CFG_CONTROLS_CONTROLLER, bJoyEnabled);

                if ( bJoyEnabled )
                {
                    // Load current active scheme from file
                    JonesControlsScheme* pActiveScheme = jonesConfig_LoadActiveControlScheme();
                    if ( pActiveScheme )
                    {
                        JonesControl_EnableJoystickAxes();
                        jonesConfig_BindControls(pActiveScheme);// TODO: why this has to be done here? Note, same is done when this dialog closes
                        jonesConfig_FreeControlScheme(pActiveScheme);
                    }
                }
                else
                {
                    sithControl_UnbindJoystickAxes();
                }
            } break;

            case 2: // Cancel button
                break;

            default:
                return;
        }

        // Delete any new scheme
        if ( jonesConfig_controlOptions_pCurNewScheme )
        {
            stdMemory_Free(jonesConfig_controlOptions_pCurNewScheme);
            jonesConfig_controlOptions_pCurNewScheme = NULL;
        }

        EndDialog(hWnd, ctrlID);
    }
}

int J3DAPI jonesConfig_ControlOptions_WriteScheme(JonesControlsScheme* pScheme)
{
    // Change cur dir to control config dir
    char aCurDir[128] = { 0 };
    if ( !SetCurrentDirectory(jonesConfig_aKeySetsDirPath) )
    {
        SECURITY_ATTRIBUTES secattr = { 0 };
        secattr.nLength              = sizeof(SECURITY_ATTRIBUTES);
        secattr.lpSecurityDescriptor = NULL;
        secattr.bInheritHandle       = 1;
        CreateDirectory(jonesConfig_aKeySetsDirPath, &secattr);

        GetCurrentDirectory(STD_ARRAYLEN(aCurDir), aCurDir);
        SetCurrentDirectory(jonesConfig_aKeySetsDirPath);
    }

    char* pPadding;
    if ( jonesConfig_maxActionPaddedStrLen <= jonesConfig_maxControlPaddedStrLen )
    {
        pPadding = (char*)STDMALLOC(jonesConfig_maxControlPaddedStrLen);
    }
    else
    {
        pPadding = (char*)STDMALLOC(jonesConfig_maxActionPaddedStrLen);
    }

    if ( !pPadding )
    {
        return 0;
    }

    if ( jonesConfig_maxActionPaddedStrLen <= jonesConfig_maxControlPaddedStrLen )
    {
        memset(pPadding, ' ', jonesConfig_maxControlPaddedStrLen);
    }
    else
    {
        memset(pPadding, ' ', jonesConfig_maxActionPaddedStrLen);
    }

    const size_t maxColumnTextLen = jonesConfig_maxControlStrLen + jonesConfig_maxActionStrLen;
    char* pColumnTitleText = (char*)STDMALLOC(maxColumnTextLen);
    if ( !pColumnTitleText )
    {
        stdMemory_Free(pPadding);
        return 0;
    }

    memset(pColumnTitleText, 0, maxColumnTextLen);

    char aKfgParh[128] = { 0 };
    STD_FORMAT(aKfgParh, "%s%s", pScheme->aName, ".kfg");

    FILE* pFile = fopen(aKfgParh, "wt+");
    if ( !pFile )
    {
        SetCurrentDirectory(aCurDir);
        stdMemory_Free(pColumnTitleText);
        stdMemory_Free(pPadding);
        return 0;
    }

    // Write heading text
    const char* pJonesString = jonesString_GetString("JONES_STR_HEADING");
    if ( !pJonesString )
    {
        goto error;
    }

    fprintf(pFile, pJonesString);
    fprintf(pFile, "\n");

    // Write enabled line
    char aEnabledLine[12] = { 0 }; // Added_ Init to 0
    STD_FORMAT(aEnabledLine, "%s\n\n", "1");
    fprintf(pFile, aEnabledLine);

    // Write column text "Actions :"
    memset(pColumnTitleText, 0, maxColumnTextLen);

    pJonesString = jonesString_GetString("JONES_STR_ACTIONS");
    if ( !pJonesString )
    {
        goto error;
    }

    stdUtil_StringCopy(pColumnTitleText, maxColumnTextLen, pJonesString);

    size_t numPadding = jonesConfig_maxActionPaddedStrLen - strlen(pColumnTitleText);
    if ( jonesConfig_maxActionPaddedStrLen <= jonesConfig_maxControlPaddedStrLen )
    {
        memset(pPadding, 0, jonesConfig_maxControlPaddedStrLen);
    }
    else
    {
        memset(pPadding, 0, jonesConfig_maxActionPaddedStrLen);
    }

    memset(pPadding, ' ', numPadding);
    stdUtil_StringCat(pColumnTitleText, maxColumnTextLen, pPadding);

    // Write column text "Keys:"
    pJonesString = jonesString_GetString("JONES_STR_KEYS");
    if ( !pJonesString )
    {
        goto error;
    }

    // Write column header line
    stdUtil_StringCat(pColumnTitleText, maxColumnTextLen, pJonesString);
    fprintf(pFile, pColumnTitleText);
    fprintf(pFile, "\n\n");

    // Now go over actions and write the key config per action
    for ( size_t i = 0; i < STD_ARRAYLEN(pScheme->aActions); ++i )
    {
        int (*aBindings)[9] = (int (*)[9])pScheme->aActions[i];

      // Write action name
        pJonesString = jonesString_GetString(jonesConfig_aJonesControlActionNames[i]);
        if ( !aBindings || !pJonesString )
        {
            goto error;
        }

        char aActionControlLine[256] = { 0 };
        STD_FORMAT(aActionControlLine, "\"%s\"", pJonesString);
        fprintf(pFile, aActionControlLine);

        // Write padding
        numPadding = jonesConfig_maxActionPaddedStrLen - strlen(aActionControlLine);
        if ( jonesConfig_maxActionPaddedStrLen <= jonesConfig_maxControlPaddedStrLen )
        {
            memset(pPadding, 0, jonesConfig_maxControlPaddedStrLen);
        }
        else
        {
            memset(pPadding, 0, jonesConfig_maxActionPaddedStrLen);
        }

        memset(pPadding, ' ', numPadding);
        fprintf(pFile, pPadding);

        // Now go over action's bindings and write them to file
        for ( size_t j = 1; j <= (uint8_t)(*aBindings)[0]; ++j )
        {
            char aControlText[JONESCONFIG_CONTROLSTRING_MAXLEN] = { 0 };
            jonesConfig_ControlToString((*aBindings)[j], aControlText);

            STD_FORMAT(aActionControlLine, "\"%s\"", aControlText);
            fprintf(pFile, aActionControlLine);

            // Write padding
            numPadding = jonesConfig_maxControlPaddedStrLen - strlen(aControlText);
            if ( jonesConfig_maxActionPaddedStrLen <= jonesConfig_maxControlPaddedStrLen )
            {
                memset(pPadding, 0, jonesConfig_maxControlPaddedStrLen);
            }
            else
            {
                memset(pPadding, 0, jonesConfig_maxActionPaddedStrLen);
            }

            memset(pPadding, ' ', numPadding);
            fprintf(pFile, pPadding);
        }

        fprintf(pFile, "\n");
    }

    // Close file and set cur dir to previous dir
    fclose(pFile);
    SetCurrentDirectory(aCurDir);

    stdMemory_Free(pPadding);
    stdMemory_Free(pColumnTitleText);
    return 1;

error:
    if ( pFile )
    {
        fclose(pFile);
    }

    SetCurrentDirectory(aCurDir);
    stdMemory_Free(pPadding);
    stdMemory_Free(pColumnTitleText);
    return 0;
}

int J3DAPI jonesConfig_ControlOptions_UpdateButtons(HWND hDlg)
{
    HWND hListCtrl = GetDlgItem(hDlg, 1047);
    int selIdx = ListBox_GetCurSel(hListCtrl);
    if ( selIdx < JONESCONFIG_NUMDEFAULTCONTROLSCHEMES )
    {
        // Only default schemes are available,
        // set edit button to view and disable delete button
        const char* pText = jonesString_GetString("JONES_STR_VIEW_SCHEME");
        if ( pText )
        {
            HWND hEditBtnCtrl = GetDlgItem(hDlg, 1028);
            SetWindowText(hEditBtnCtrl, pText);
        }

        HWND hDelBtnCtrl = GetDlgItem(hDlg, 1022);
        return EnableWindow(hDelBtnCtrl, 0);
    }
    else
    {
        // Ok we have custom schemes enable delete button
        const char* pText = jonesString_GetString("JONES_STR_EDIT_SCHEME");
        if ( pText )
        {
            HWND hEditBtnCtrl = GetDlgItem(hDlg, 1028);
            SetWindowText(hEditBtnCtrl, pText);
        }

        HWND hDelBtnCtrl = GetDlgItem(hDlg, 1022);
        return EnableWindow(hDelBtnCtrl, 1);
    }
}

void J3DAPI jonesConfig_ControlOptions_DeleteScheme(HWND hDlg, JonesControlsConfig* pConfig)
{
    HWND hListCtrl = GetDlgItem(hDlg, 1047); // list of schemes in control options dialog

    char aSchemeName[128] = { 0 };
    int selIdx = ListBox_GetCurSel(hListCtrl);
    ListBox_GetText(hListCtrl, selIdx, aSchemeName);
    ListBox_DeleteString(hListCtrl, selIdx);

    size_t schemeNum = 0;
    while ( schemeNum < pConfig->numSchemes )
    {
        if ( streq(pConfig->aSchemes[schemeNum].aName, aSchemeName) )
        {
            if ( (pConfig->numDeleteSchemes % 10) == 0 )
            {
                pConfig->paDeleteSchemes = (char (*)[128])STDREALLOC(
                    pConfig->paDeleteSchemes,
                    (pConfig->numDeleteSchemes + 10) * 128
                );
            }

            if ( pConfig->aSchemes[schemeNum].bModified != 1 )
            {
                char aPath[128] = { 0 };
                STD_FORMAT(aPath, "%s\\%s%s", jonesConfig_aKeySetsDirPath, aSchemeName, ".kfg");
                STD_STRCPY(pConfig->paDeleteSchemes[pConfig->numDeleteSchemes], aPath);
                ++pConfig->numDeleteSchemes;
            }

            for ( size_t i = 0; i < STD_ARRAYLEN(pConfig->aSchemes[schemeNum].aActions); ++i )
            {
                memset(pConfig->aSchemes[schemeNum].aActions[i], 0, STD_ARRAYLEN(pConfig->aSchemes[schemeNum].aActions[i]));
            }

            for ( size_t j = schemeNum; j < (signed int)pConfig->numSchemes; ++j )
            {
                if ( j == pConfig->numSchemes - 1 )
                {
                    memset(&pConfig->aSchemes[j], 0, sizeof(pConfig->aSchemes[j]));
                }
                else
                {
                    memcpy(&pConfig->aSchemes[j], &pConfig->aSchemes[j + 1], sizeof(pConfig->aSchemes[j]));
                }
            }

            if ( schemeNum == pConfig->selectedShemeIdx )
            {
                size_t newSelIdx = pConfig->selectedShemeIdx - 1 >= 0 ? pConfig->selectedShemeIdx - 1 : 0;
                pConfig->selectedShemeIdx = newSelIdx;

                ListBox_SetCurSel(hListCtrl, pConfig->selectedShemeIdx);

                if ( pConfig->selectedShemeIdx < JONESCONFIG_NUMDEFAULTCONTROLSCHEMES )// Are we left only with default schemes?
                {
                    // Disable delete button
                    HWND hBtnDelCtrl = GetDlgItem(hDlg, 1022);
                    EnableWindow(hBtnDelCtrl, 0);
                }
            }

            --pConfig->numSchemes;
            return;
        }

        ++schemeNum;
    }
}

void J3DAPI jonesConfig_ControlOptions_EditSelectedScheme(HWND hDlg, JonesControlsConfig* pConfig)
{
    HWND hListCtrl = GetDlgItem(hDlg, 1047); // list of scheme names in control options

    // Get selected scheme name
    char aSchemeName[128] = { 0 };
    size_t selIdx = ListBox_GetCurSel(hListCtrl);
    ListBox_GetText(hListCtrl, selIdx, aSchemeName);

    size_t start = 0;
    JonesEditControlSchemeDialogData editdata = { 0 };
    while ( start < pConfig->numSchemes )
    {
        if ( streq(pConfig->aSchemes[start].aName, aSchemeName) )
        {
            memset(&editdata, 0, sizeof(editdata));
            editdata.aSchemes       = jonesConfig_CloneControlSchemesList(pConfig->aSchemes, start, 1);
            editdata.numSchemes     = start;
            editdata.pDefaultShceme = jonesConfig_GetDefaultControlScheme(pConfig);
            break;
        }

        ++start;
    }

    if ( start != pConfig->numSchemes )
    {
        if ( jonesConfig_ShowEditControlShemeDialog(hDlg, &editdata) == 1 )
        {
            InvalidateRect(hDlg, 0, 1);
            pConfig->aSchemes[start].bModified = editdata.aSchemes->bModified;
            if ( pConfig->aSchemes[start].bModified )
            {
                for ( size_t i = 0; i < STD_ARRAYLEN(pConfig->aSchemes[start].aActions); ++i )
                {
                    for ( size_t j = 0; j < STD_ARRAYLEN(pConfig->aSchemes[start].aActions[i]); ++j )
                    {
                        pConfig->aSchemes[start].aActions[i][j] = editdata.aSchemes->aActions[i][j];
                    }
                }
            }

            stdMemory_Free(editdata.aSchemes);
            editdata.aSchemes = NULL;
        }
        else
        {
            jonesConfig_FreeControlScheme(editdata.aSchemes);
        }
    }
}

JonesControlsScheme* J3DAPI jonesConfig_GetDefaultControlScheme(JonesControlsConfig* pConfig)
{
    size_t numButtons = stdControl_GetMaxJoystickButtons();

    const char* pSchemeName;
    if ( numButtons == 4 )
    {
        pSchemeName = jonesString_GetString(jonesConfig_aDfltKeySetNames[1]);
    }
    else if ( numButtons == 8 )
    {
        pSchemeName = jonesString_GetString(jonesConfig_aDfltKeySetNames[2]);
    }
    else
    {
        // Keyboard
        pSchemeName = jonesString_GetString(jonesConfig_aDfltKeySetNames[0]);
    }

    if ( pSchemeName )
    {
        for ( size_t i = 0; i < JONESCONFIG_NUMDEFAULTCONTROLSCHEMES; ++i ) // 3 default schemes
        {
            if ( streq(pConfig->aSchemes[i].aName, pSchemeName) )
            {
                return &pConfig->aSchemes[i];
            }
        }
    }

    return NULL;
}

void J3DAPI jonesConfig_ControlOptions_CreateNewScheme(HWND hDlg, JonesControlsConfig* pConfig, JonesControlsScheme* pNewScheme)
{
    // Show dialog for user to enter new scheme filename
    JonesCreateControlSchemeDialogData newSchemeDlgData = { 0 };
    newSchemeDlgData.hSchemeList = GetDlgItem(hDlg, 1047);
    if ( jonesConfig_ShowCreateControlSchemeDialog(hDlg, &newSchemeDlgData) != 1 )
    {
        return;
    }

    InvalidateRect(hDlg, 0, 1);

    pNewScheme = jonesConfig_CloneControlSchemesList(pConfig->aSchemes, 0, 1);
    if ( !pNewScheme )
    {
        const char* pErrorText = jonesString_GetString("JONES_STR_NO_MEM_SCHEME");
        if ( pErrorText )
        {
            jonesConfig_ShowMessageDialog(hDlg, "JONES_STR_CTRL_OPTS", pErrorText, 145); // 145 - typewriter icon
            InvalidateRect(hDlg, 0, 1);
        }
        return;
    }

    STD_STRCPY(pNewScheme->aName, newSchemeDlgData.aNewSchemeName);

    JonesEditControlSchemeDialogData data = { 0 };
    data.aSchemes       = pNewScheme;
    data.numSchemes     = pConfig->numSchemes;
    data.pDefaultShceme = jonesConfig_GetDefaultControlScheme(pConfig);

    if ( jonesConfig_ShowEditControlShemeDialog(hDlg, &data) != 1 )
    {
        stdMemory_Free(pNewScheme);
        return;
    }

    InvalidateRect(hDlg, 0, 1);

    // Get list position for the new scheme based on alphabetical order
    size_t schemeIdx;
    for ( schemeIdx = JONESCONFIG_NUMDEFAULTCONTROLSCHEMES; // 3 - first index after default schemes
        schemeIdx < pConfig->numSchemes
        && strcmp(newSchemeDlgData.aNewSchemeName, pConfig->aSchemes[schemeIdx].aName) >= 0;
        ++schemeIdx )
    {
        ;
    }

    // Allocate additional scheme list buffer
    if ( (pConfig->numSchemes % 10) == 0 )
    {
        pConfig->aSchemes = (JonesControlsScheme*)STDREALLOC(pConfig->aSchemes, sizeof(JonesControlsScheme) * (pConfig->numSchemes + 10));
    }

    if ( !pConfig->aSchemes )
    {
        const char* pErrorText = jonesString_GetString("JONES_STR_NO_MEM_SCHEME");
        if ( pErrorText )
        {
            jonesConfig_ShowMessageDialog(hDlg, "JONES_STR_CTRL_OPTS", pErrorText, 145); // 145 - typewriter icon
            InvalidateRect(hDlg, 0, 1);
        }
        return;
    }

    // Make space in config list to insert new scheme by shifting existing schemes for 1 place
    for ( size_t i = pConfig->numSchemes - 1; i >= schemeIdx; --i )
    {
        STD_STRCPY(pConfig->aSchemes[i + 1].aName, pConfig->aSchemes[i].aName);
        for ( size_t j = 0; j < STD_ARRAYLEN(pConfig->aSchemes[i + 1].aActions); ++j )
        {
            for ( size_t k = 0; k < STD_ARRAYLEN(pConfig->aSchemes[i + 1].aActions[j]); ++k )
            {
                pConfig->aSchemes[i + 1].aActions[j][k] = pConfig->aSchemes[i].aActions[j][k];
            }
        }
    }

    // Insert new scheme name in list control
    HWND hSchemeList = GetDlgItem(hDlg, 1047);
    ListBox_InsertString(hSchemeList, schemeIdx, newSchemeDlgData.aNewSchemeName);
    ListBox_SetCurSel(hSchemeList, schemeIdx);

    pConfig->selectedShemeIdx = schemeIdx;

    // Resize increase width of list for the new scheme name to fit in row
    SIZE sz;
    HDC hdc = GetDC(hSchemeList);
    size_t nameLen = strlen(newSchemeDlgData.aNewSchemeName);
    GetTextExtentPoint(hdc, newSchemeDlgData.aNewSchemeName, nameLen, &sz);

    if ( sz.cx > pConfig->maxNameLen )
    {
        pConfig->maxNameLen = sz.cx;

        RECT rectSchemeList;
        GetClientRect(hSchemeList, &rectSchemeList);
        if ( rectSchemeList.right - rectSchemeList.left < sz.cx )
        {
            ListBox_SetHorizontalExtent(hSchemeList, sz.cx);
        }
    }

    // Insert new scheme in config list
    pConfig->aSchemes[schemeIdx].bModified = data.aSchemes->bModified;
    STD_STRCPY(pConfig->aSchemes[schemeIdx].aName, data.aSchemes->aName);

    for ( size_t i = 0; i < STD_ARRAYLEN(pConfig->aSchemes[schemeIdx].aActions); ++i )
    {
        for ( size_t j = 0; j < STD_ARRAYLEN(pConfig->aSchemes[schemeIdx].aActions[i]); ++j )
        {
            pConfig->aSchemes[schemeIdx].aActions[i][j] = data.aSchemes->aActions[i][j];
        }
    }

    stdMemory_Free(data.aSchemes);
    data.aSchemes = NULL;
    ++pConfig->numSchemes;
}

int J3DAPI jonesConfig_ShowCreateControlSchemeDialog(HWND hWnd, JonesCreateControlSchemeDialogData* pData)
{
    GetWindowLongPtr(hWnd, GWL_HINSTANCE);
    return JonesDialog_ShowDialog(MAKEINTRESOURCE(116), hWnd, jonesConfig_CreateControlSchemeProc, (LPARAM)pData);
}

INT_PTR CALLBACK jonesConfig_CreateControlSchemeProc(HWND hWnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    switch ( msg )
    {
        case WM_DESTROY:
            jonesConfig_ResetDialogFont(hWnd, jonesConfig_hFontCreateControlSchemeDlg);
            break;

        case WM_INITDIALOG:
        {
            jonesConfig_hFontCreateControlSchemeDlg = jonesConfig_InitDialog(hWnd, 0, 116);
            int res = jonesConfig_InitCreateControlScheme(hWnd, wparam, (JonesCreateControlSchemeDialogData*)lparam);
            SetWindowPos(hWnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_SHOWWINDOW | SWP_NOMOVE | SWP_NOSIZE);
            return res;
        }

        case WM_COMMAND:
            jonesConfig_CreateControlScheme_Handle_WM_COMMAND(hWnd, LOWORD(wparam));
            return 0;

        default:
            return 0;
    }

    return 1;
}

int J3DAPI jonesConfig_InitCreateControlScheme(HWND hDlg, int a2, JonesCreateControlSchemeDialogData* pData)
{
    J3D_UNUSED(a2);

    HWND hEditCtrl = GetDlgItem(hDlg, 1052);
    SetFocus(hEditCtrl);
    Edit_LimitText(hEditCtrl, 60);

    SetWindowLongPtr(hDlg, DWL_USER, (LONG)pData);
    return 0; // should it return 1 ?
}

void J3DAPI jonesConfig_CreateControlScheme_Handle_WM_COMMAND(HWND hDlg, int ctrlID)
{
    JonesCreateControlSchemeDialogData* pData = (JonesCreateControlSchemeDialogData*)GetWindowLongA(hDlg, DWL_USER);

    if ( ctrlID != 1 ) // ok button
    {
        if ( ctrlID == 2 ) // cancel button
        {
            EndDialog(hDlg, ctrlID);
            return;
        }
        return;
    }

    HWND hEditCtrl = GetDlgItem(hDlg, 1052);

    CHAR aName[128] = { 0 };
    Edit_GetText(hEditCtrl, aName, STD_ARRAYLEN(aName));

    size_t nameLen = strlen(aName);

    // Check if name is empty
    // Altered: This check was done within while loop
    if ( nameLen == 0 )
    {
        const char* pErrorText = jonesString_GetString("JONES_STR_NO_SCHEME");
        if ( !pErrorText )
        {
            return;
        }

        jonesConfig_ShowMessageDialog(hDlg, "JONES_STR_CTRL_OPTS", pErrorText, 145); // 145 - typewriter icon
        InvalidateRect(hDlg, 0, 1);
        return;
    }

    if ( strcspn(aName, "/:\\\"?<>*|+") != nameLen )
    {
        // Bad file name format
        const char* pErrorText = jonesString_GetString("JONES_STR_INVALIDFILE");
        if ( pErrorText )
        {
            char aErrorText[256] = { 0 };
            STD_FORMAT(aErrorText, pErrorText, aName);
            jonesConfig_ShowMessageDialog(hDlg, "JONES_STR_CTRL_OPTS", aErrorText, 145); // 145 - typewriter icon
            InvalidateRect(hDlg, 0, 1);
        }

        Edit_SetText(hEditCtrl, ""); // empty string
        return;
    }

    int itemIdx = 0;
    int numSchemes = ListBox_GetCount(pData->hSchemeList);
    while ( itemIdx < numSchemes )
    {
        // Check if scheme with same name already exists
        char aSchemeName[128] = { 0 };
        ListBox_GetText(pData->hSchemeList, itemIdx, aSchemeName);
        if ( streq(aName, aSchemeName) )
        {
            const char* pErrorText = jonesString_GetString("JONES_STR_DUP_SCHEME");
            if ( pErrorText )
            {
                jonesConfig_ShowMessageDialog(hDlg, "JONES_STR_CTRL_OPTS", pErrorText, 145); // 145 - typewriter icon
                InvalidateRect(hDlg, 0, 1);
            }

            Edit_SetText(hEditCtrl, ""); // empty string
            return;
        }

        ++itemIdx;
    }

    // Ok name check has passed, assign new name to dlg data and close dialog
    if ( itemIdx == numSchemes )
    {
        Edit_GetText(hEditCtrl, pData->aNewSchemeName, STD_ARRAYLEN(pData->aNewSchemeName));
    }

    EndDialog(hDlg, ctrlID);
    return;
}

int J3DAPI jonesConfig_ShowEditControlShemeDialog(HWND hWnd, JonesEditControlSchemeDialogData* pData)
{
    GetWindowLongA(hWnd, GWL_HINSTANCE);
    return JonesDialog_ShowDialog(MAKEINTRESOURCE(115), hWnd, jonesConfig_EditControlSchemeProc, (LPARAM)pData);
}

INT_PTR CALLBACK jonesConfig_EditControlSchemeProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    if ( uMsg > WM_NOTIFY )
    {
        if ( uMsg != WM_INITDIALOG )
        {
            if ( uMsg != WM_COMMAND )
            {
                return 0;
            }

            jonesConfig_EditControlScheme_Handle_WM_COMMAND(hWnd, (uint16_t)wParam);
            return 0;
        }

        jonesConfig_hFontEditControlShceme = jonesConfig_InitDialog(hWnd, 0, 115);
        int result = jonesConfig_InitEditControlSchemeDlg(hWnd, wParam, (JonesEditControlSchemeDialogData*)lParam);

        SetWindowPos(hWnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_SHOWWINDOW | SWP_NOMOVE | SWP_NOSIZE);
        return result;
    }
    else
    {
        if ( uMsg == WM_NOTIFY )
        {
            NMHDR* pNM = (NMHDR*)lParam;
            if ( pNM->code != NM_CLICK )// -2 NM_CLICK
            {
                return 1;
            }

            // Update cur selected list state
            JonesEditControlSchemeDialogData* pData = (JonesEditControlSchemeDialogData*)GetWindowLongPtr(hWnd, DWL_USER);
            if ( pNM->idFrom == 1005 )
            {
                pData->hCurList = GetDlgItem(hWnd, 1005);
                pData->curListID = 1005;
            }
            else if ( pNM->idFrom == 1006 )
            {
                pData->hCurList = GetDlgItem(hWnd, 1006);
                pData->curListID = 1006;
            }

            return 1;
        }

        if ( uMsg != WM_DESTROY )
        {
            return 0;
        }

        jonesConfig_ResetDialogFont(hWnd, jonesConfig_hFontEditControlShceme);
    }

    return 1;
}

int J3DAPI jonesConfig_InitEditControlSchemeDlg(HWND hDlg, int a2, JonesEditControlSchemeDialogData* pData)
{
    J3D_UNUSED(a2);

    // Set keyboard binding list
    HWND hKeyListCtrl = GetDlgItem(hDlg, 1005);
    jonesConfig_EditControlScheme_SetListColumn(hKeyListCtrl, "JONES_STR_MAPKEY");
    jonesConfig_EditControlScheme_SetListScheme(hKeyListCtrl, 1005, pData->aSchemes);

    // Set controller (joystick/gamepad) binding list
    HWND hControlListCtrl = GetDlgItem(hDlg, 1006);
    jonesConfig_EditControlScheme_SetListColumn(hControlListCtrl, "JONES_STR_MAPBUTTON");
    jonesConfig_EditControlScheme_SetListScheme(hControlListCtrl, 1006, pData->aSchemes);

    char aGBText[256] = { 0 };
    const char* pGBText = jonesString_GetString("JONES_STR_SETTINGS");
    if ( pGBText )
    {
        STD_FORMAT(aGBText, pGBText, pData->aSchemes->aName);
        HWND hGBCtrl = GetDlgItem(hDlg, 1049); // settings group box
        SetWindowText(hGBCtrl, aGBText);
    }

    if ( pData->numSchemes < JONESCONFIG_NUMDEFAULTCONTROLSCHEMES )
    {
        // We have only default schemes, disable modify buttons

        HWND hEditBtn = GetDlgItem(hDlg, 1056);
        EnableWindow(hEditBtn, 0);

        HWND hAddBtn = GetDlgItem(hDlg, 1046);
        EnableWindow(hAddBtn, 0);

        HWND hDelBtn = GetDlgItem(hDlg, 1038);
        EnableWindow(hDelBtn, 0);

        HWND hOkBtn = GetDlgItem(hDlg, 1);
        EnableWindow(hOkBtn, 0);

        HWND hDefltBtn = GetDlgItem(hDlg, 1099);
        EnableWindow(hDefltBtn, 0);
    }

    SetWindowLongPtr(hDlg, DWL_USER, (LONG_PTR)pData);
    return 1;
}

void J3DAPI jonesConfig_EditControlScheme_SetListColumn(HWND hCtrl, const char* pJSText)
{
    RECT rect;
    GetClientRect(hCtrl, &rect);
    int ctrlWidth = rect.right - rect.left;

    int width = ctrlWidth - GetSystemMetrics(SM_CXVSCROLL);

    uint32_t gameWidth, gameHeight;
    stdDisplay_GetBackBufferSize(&gameWidth, &gameHeight);

    float ratio = 1.0f;
    if ( (float)gameHeight / RD_REF_HEIGHT <= 1.0f ) // TODO: maybe remove this check to also upscale
    {
        ratio = (float)gameHeight / RD_REF_HEIGHT;
    }

    int spacing = (int32_t)(105.0f * ratio);

    LVCOLUMNA column;
    column.mask       = LVCF_SUBITEM | LVCF_TEXT | LVCF_WIDTH | LVCF_FMT;
    column.cchTextMax = 256;
    column.fmt        = LVCFMT_LEFT;
    column.pszText    = jonesString_GetString("JONES_STR_FCTN");
    column.iSubItem   = 0;
    column.cx         = width - spacing;
    ListView_InsertColumn(hCtrl, 0, &column);

    if ( pJSText )
    {
        column.pszText = jonesString_GetString(pJSText);
    }

    column.iSubItem = 1;
    column.cx       = spacing;
    ListView_InsertColumn(hCtrl, 1, &column);
}

void J3DAPI jonesConfig_EditControlScheme_SetListScheme(HWND hCtrl, int ctrlID, JonesControlsScheme* pScheme)
{
    LVITEM item;
    item.mask       = LVIF_STATE | LVIF_PARAM | LVIF_TEXT;
    item.cchTextMax = 256;
    item.state      = 0;
    item.stateMask  = 0;

    size_t itemNum = 0;
    for ( size_t i = 0; i < STD_ARRAYLEN(jonesConfig_aJonesControlActionNames); ++i )
    {
        size_t numControllers = 0;
        const char* pActionName = jonesString_GetString(jonesConfig_aJonesControlActionNames[i]);

        for ( size_t bindNum = 1; bindNum <= (uint8_t)pScheme->aActions[i][0]; ++bindNum ) // Note, bindNum must start at 1 as 0 indes ix num bindings state
        {
            int keyid = pScheme->aActions[i][bindNum];
            item.iItem    = itemNum;
            item.iSubItem = 0;
            item.pszText  = (LPSTR)pActionName;

            if ( ctrlID == 1005 // 1005 - keyboard list
                && (keyid < STDCONTROL_MAX_KEYBOARD_BUTTONS || keyid > STDCONTROL_JOYSTICK_LASTPOVCID) && (keyid < JONESCONFIG_JOYAXISKID_UP || keyid > JONESCONFIG_JOYAXISKID_GROUND)
                || ctrlID == 1006 // controller list
                && (keyid >= STDCONTROL_JOYSTICK_FIRSTKID && keyid <= STDCONTROL_JOYSTICK_LASTPOVCID || keyid >= JONESCONFIG_JOYAXISKID_UP && keyid <= JONESCONFIG_JOYAXISKID_GROUND) )
            {
                if ( ctrlID == 1006 ) // Control List
                {
                    ++numControllers;
                }

                item.lParam = i | (keyid << 16);
                ListView_InsertItem(hCtrl, &item);

                char aControlText[256] = { 0 };
                jonesConfig_ControlToString(keyid, aControlText);
                ListView_SetItemText(hCtrl, itemNum++, 1, aControlText);
            }
        }

        if ( !numControllers && ctrlID == 1006 )   // Control List
        {
            item.lParam = i;
            ListView_InsertItem(hCtrl, &item);

            char aControlText[256] = { 0 };
            ListView_SetItemText(hCtrl, itemNum++, 1, aControlText);
        }
    }
}
void J3DAPI jonesConfig_EditControlScheme_Handle_WM_COMMAND(HWND hDlg, int ctrlID)
{
    JonesEditControlSchemeDialogData* pData = (JonesEditControlSchemeDialogData*)GetWindowLongPtr(hDlg, DWL_USER);

    switch ( ctrlID )
    {
        case 1046: // Add button
            if ( pData->numSchemes >= JONESCONFIG_NUMDEFAULTCONTROLSCHEMES )
            {
                jonesConfig_EditControlScheme_AddAssignment(hDlg, pData->hCurList, pData->curListID, pData->aSchemes);
            }
            break;

        case 1056: // Edit button
            if ( pData->numSchemes >= JONESCONFIG_NUMDEFAULTCONTROLSCHEMES )
            {
                jonesConfig_EditControlScheme_EditAssignment(hDlg, pData->hCurList, pData->curListID, pData->aSchemes);
            }
            break;

        case 1099: // Default scheme button
        {
            HWND hKeyListCtrl     = GetDlgItem(hDlg, 1005);
            HWND hControlListCtrl = GetDlgItem(hDlg, 1006);

            // Copy default scheme
            for ( size_t i = 0; i < STD_ARRAYLEN(pData->aSchemes->aActions); ++i )
            {
                memset(pData->aSchemes->aActions[i], 0, STD_ARRAYLEN(pData->aSchemes->aActions[i])); // Clear existing bindings
                for ( size_t j = 0; j <= JONESCONTROL_ACTION_GETNUMBINDS(pData->pDefaultShceme->aActions[i]); ++j )
                {
                    pData->aSchemes->aActions[i][j] = pData->pDefaultShceme->aActions[i][j];
                }
            }

            // Reset list controls
            ListView_DeleteAllItems(hKeyListCtrl);
            ListView_DeleteAllItems(hControlListCtrl);

            // Init list controls with changed scheme
            jonesConfig_EditControlScheme_SetListScheme(hKeyListCtrl, 1005, pData->aSchemes); // Key list
            jonesConfig_EditControlScheme_SetListScheme(hControlListCtrl, 1006, pData->aSchemes); // Controller  list
        } break;

        case 1038: // ctrlID == 1038 - Delete scheme button
            if ( pData->numSchemes >= JONESCONFIG_NUMDEFAULTCONTROLSCHEMES )
            {
                jonesConfig_EditControlScheme_DeleteAssignment(hDlg, pData->hCurList, pData->curListID, pData->aSchemes);
            }
            break;

        case 2: // Cancel button
            EndDialog(hDlg, ctrlID);
            return;

        case 1: // Save button
        {
            // Check that all required actions have key bindings
            char aActionName[512] = { 0 };
            for ( size_t i = 0; i < STD_ARRAYLEN(pData->aSchemes->aActions); ++i )
            {
                if ( JONESCONTROL_ACTION_GETNUMBINDS(pData->aSchemes->aActions[i]) == 0 && (i < JONESCONTROL_ACTION_FISTS || i > JONESCONTROL_ACTION_BAZOOKA) ) // 16 - 24 are weapons keys
                {
                    const char* pActionName = jonesString_GetString(jonesConfig_aJonesCapControlActionNames[i]);
                    STD_STRCAT(aActionName, pActionName);
                    STD_STRCAT(aActionName, "\n");
                }
            }

            // If there was an error aActionName len != 0
            if ( strlen(aActionName) )
            {
                char aErrorText[512] = { 0 };
                const char* pErrorText = jonesString_GetString("JONES_STR_NO_MAPNG");
                STD_FORMAT(aErrorText, "%s\n", pErrorText);

                STD_STRCAT(aErrorText, aActionName);
                jonesConfig_ShowMessageDialog(hDlg, "JONES_STR_CTRL_OPTS", aErrorText, 145);// 145 - typewrite icon
                InvalidateRect(hDlg, 0, 1);
                return;
            }

            pData->aSchemes->bModified = 1;
            EndDialog(hDlg, ctrlID);
            return;
        }
    };
}

void J3DAPI jonesConfig_EditControlScheme_EditAssignment(HWND hDlg, HWND hListCtrl, int listID, JonesControlsScheme* pScheme)
{
    size_t bindIdx, itemIdx;
    JonesAssignKeyDialogData assignkeyData = { 0 };
    assignkeyData.pScheme  = pScheme;
    assignkeyData.actionId = jonesConfig_EditControlScheme_GetSelectedListItemBinding(hDlg, hListCtrl, pScheme, &itemIdx, &bindIdx);
    assignkeyData.listID   = listID;

    if ( assignkeyData.actionId < 0 )
    {
        const char* pErrorText = jonesString_GetString("JONES_STR_NOKEYACTION");
        if ( pErrorText )
        {
            jonesConfig_ShowMessageDialog(hDlg, "JONES_STR_CTRL_OPTS", pErrorText, 145);// 145 - typewriter icon
            InvalidateRect(hDlg, 0, 1);
        }
        return;
    }

    if ( jonesConfig_ShowAssignKeyDialog(hDlg, &assignkeyData) == 1 )
    {
        InvalidateRect(hDlg, 0, 1);

        if ( JONESCONTROL_ACTION_GETNUMBINDS(pScheme->aActions[assignkeyData.actionId]) )
        {
            itemIdx = ListView_GetNextItem(hListCtrl, -1, LVNI_SELECTED); // TODO: why do another call, and not use value of itemIdx returned by above func?
            ListView_SetItemText(hListCtrl, itemIdx, 1, assignkeyData.aControlName);

            LVITEM item;
            item.mask     = LVIF_PARAM;
            item.iItem    = itemIdx;
            item.iSubItem = 0;
            item.lParam   = assignkeyData.actionId | (assignkeyData.controlId << 16);
            ListView_SetItem(hListCtrl, &item);

            // Assign new key biding
            pScheme->aActions[assignkeyData.actionId][bindIdx + 1] = assignkeyData.controlId; // Note, bindIdx must be incremented as it starts with 0, and index 0 points to num binds state

            // Update num key binds only if 0
            if ( assignkeyData.listID == 1005 && !JONESCONTROL_ACTION_GETNUMKEYBOARDBINDS(pScheme->aActions[assignkeyData.actionId]) )
            {
                JONESCONTROL_ACTION_UPDATENUMBINDINGS_KEYBOARD(pScheme->aActions[assignkeyData.actionId], +1);
            }
            else if ( assignkeyData.listID == 1006 && !JONESCONTROL_ACTION_GETNUMCONTROLLERBINDS(pScheme->aActions[assignkeyData.actionId]) )
            {
                JONESCONTROL_ACTION_UPDATENUMBINDINGS_CONTROLLER(pScheme->aActions[assignkeyData.actionId], +1);
            }
        }
    }
}

int J3DAPI jonesConfig_EditControlScheme_GetSelectedListItemBinding(HWND hDlg, HWND hListCtrl, JonesControlsScheme* pScheme, size_t* pItemIdx, size_t* pBindIdx)
{
    if ( !hListCtrl || !hDlg || !pScheme )
    {
        return -1;
    }

    int itemIdx = ListView_GetNextItem(hListCtrl, -1, LVNI_SELECTED);
    if ( itemIdx < 0 )
    {
        return -1;
    }

    LVITEM item;
    item.mask     = LVIF_PARAM;
    item.iItem    = itemIdx;
    item.iSubItem = 0;
    ListView_GetItem(hListCtrl, &item);

    size_t actionId  = LOWORD(item.lParam);
    size_t controlId = HIWORD(item.lParam);// key binding controlID (kid or cid)
    size_t bindIdx = 0;

    STD_ASSERT(actionId < JONESCONTROL_ACTION_NUMACTIONS); // Added

    size_t i;
    for ( i = 1; i <= JONESCONTROL_ACTION_GETNUMBINDS(pScheme->aActions[actionId]) // TODO, should check be < rather than <=
        && pScheme->aActions[actionId][i] != controlId; ++i )
    {
        ++bindIdx;
    }

    if ( controlId
        && JONESCONTROL_ACTION_GETNUMBINDS(pScheme->aActions[actionId])
        && i > JONESCONTROL_ACTION_GETNUMBINDS(pScheme->aActions[actionId]) )
    {
        return -1;
    }

    if ( pItemIdx )
    {
        *pItemIdx = itemIdx;
    }

    if ( pBindIdx )
    {
        *pBindIdx = bindIdx;
    }

    return actionId;
}

void J3DAPI jonesConfig_EditControlScheme_AddAssignment(HWND hDlg, HWND hListCtrl, int listID, JonesControlsScheme* pScheme)
{
    LVITEM item;
    item.mask     = LVIF_PARAM;
    item.iSubItem = 0;
    item.iItem    = ListView_GetNextItem(hListCtrl, -1, LVNI_SELECTED);
    if ( item.iItem < 0 )
    {
        const char* pErrorText = (char*)jonesString_GetString("JONES_STR_NOKEYACTION");
        if ( pErrorText )
        {
            jonesConfig_ShowMessageDialog(hDlg, "JONES_STR_CTRL_OPTS", pErrorText, 145); // 145 - typwrite icon
            InvalidateRect(hDlg, 0, 1);
        }
        return;
    }

    ListView_GetItem(hListCtrl, &item);

    JonesAssignKeyDialogData data = { 0 };
    data.actionId = LOWORD(item.lParam);
    data.pScheme  = pScheme;
    data.listID   = listID;
    if ( jonesConfig_ShowAssignKeyDialog(hDlg, &data) == 1 )
    {
        InvalidateRect(hDlg, 0, 1);

        STD_ASSERT(data.actionId < JONESCONTROL_ACTION_NUMACTIONS); // Added
        if ( JONESCONTROL_ACTION_HASMAXBINDS(pScheme->aActions[data.actionId]) ) // Fixed: Changed to GT or EQ was EQ
        {
            // We exceed max number of action key bindings

            char aErrorText[512] = { 0 };
            const char* pErrorText = jonesString_GetString("JONES_STR_EXCEED");
            if ( pErrorText )
            {
                STD_STRCPY(aErrorText, pErrorText);
            }

            char aActioName[512] = { 0 };
            const char* pActionName = jonesString_GetString(jonesConfig_aJonesCapControlActionNames[data.actionId]);
            if ( pActionName )
            {
                STD_STRCPY(aActioName, pActionName);
            }

            if ( aErrorText[0] && aActioName[0] )
            {
                char aMsgText[256] = { 0 };
                STD_FORMAT(aMsgText, aErrorText, aActioName);
                jonesConfig_ShowMessageDialog(hDlg, "JONES_STR_CTRL_OPTS", aMsgText, 145);
                InvalidateRect(hDlg, 0, 1);
            }
            return;
        }

        // Removed: Useless check, as &pScheme->aActions[data.actionId] (was pBinds) is pointer to fixed array
        //else if ( !&pScheme->aActions[data.actionId] )
        //{
        //    const char* pErrorText = jonesString_GetString("JONES_STR_NO_MEM_ASGNMNT");
        //    if ( pErrorText )
        //    {
        //        jonesConfig_ShowMessageDialog(hDlg, "JONES_STR_CTRL_OPTS", pErrorText, 145);
        //        InvalidateRect(hDlg, 0, 1);
        //    }
        //}

        int numBinds = (listID == 1005)
            ? JONESCONTROL_ACTION_GETNUMKEYBOARDBINDS(pScheme->aActions[data.actionId])
            : JONESCONTROL_ACTION_GETNUMCONTROLLERBINDS(pScheme->aActions[data.actionId]);
        if ( numBinds <= 0 )
        {
            item.mask   = LVIF_PARAM;
            item.lParam = MAKELONG(data.actionId, data.controlId);
            ListView_GetItem(hListCtrl, &item);
        }
        else
        {
            int numItems = ListView_GetItemCount(hListCtrl);
            while ( data.actionId == LOWORD(item.lParam) && item.iItem < numItems )
            {
                ++item.iItem;
                ListView_GetItem(hListCtrl, &item);
            }

            const char* pActionName = jonesString_GetString(jonesConfig_aJonesControlActionNames[data.actionId]);
            if ( pActionName )
            {
                item.mask       = LVIF_STATE | LVIF_PARAM | LVIF_TEXT;
                item.state      = 0;
                item.stateMask  = 0;
                item.pszText    = (LPSTR)pActionName;
                item.cchTextMax = 256;
                item.lParam     = MAKELONG(data.actionId, data.controlId);
                ListView_InsertItem(hListCtrl, &item);
            }
        }

        ListView_SetItemState(hListCtrl, item.iItem, 3, 3);
        ListView_SetItemText(hListCtrl, item.iItem, 1, data.aControlName);

       // Assign nex key binding
        STD_ASSERT(data.actionId < JONESCONTROL_ACTION_NUMACTIONS); // Added
        JONES_CONTROLACTION_ADDNEXTKEYBIND(pScheme->aActions[data.actionId], data.controlId);
         //pBinds[(uint8_t)*pBinds + 1] = data.controlId;

        // Update num bindings state
        if ( listID == 1005 ) // key list
        {
            JONESCONTROL_ACTION_UPDATENUMBINDINGS_KEYBOARD(pScheme->aActions[data.actionId], +1);
        }
        else // controller list
        {
            JONESCONTROL_ACTION_UPDATENUMBINDINGS_CONTROLLER(pScheme->aActions[data.actionId], +1);
        }

       /* if ( listID == 1005 )
        {
            v4 = ((uint8_t)*pBinds + 1) | ((((*pBinds & 0xFF00u) >> 8) + 1) << 8) | ((*pBinds & 0xFF0000u) >> 16 << 16);
        }
        else
        {
            v4 = ((uint8_t)*pBinds + 1) | ((*pBinds & 0xFF00u) >> 8 << 8) | ((((*pBinds & 0xFF0000u) >> 16) + 1) << 16);
        }*/
    }
}

void jonesConfig_EditControlScheme_DeleteAssignment(HWND hDlg, HWND hListCtrl, int listID, JonesControlsScheme* pScheme)
{
    size_t itemIdx, bndIdx;
    int actionIdx = jonesConfig_EditControlScheme_GetSelectedListItemBinding(hDlg, hListCtrl, pScheme, &itemIdx, &bndIdx);
    if ( actionIdx >= 0 )
    {
        size_t numBinds = listID == 1005 // if keyboard list
            ? JONESCONTROL_ACTION_GETNUMKEYBOARDBINDS(pScheme->aActions[actionIdx])
            : JONESCONTROL_ACTION_GETNUMCONTROLLERBINDS(pScheme->aActions[actionIdx]);
        if ( numBinds )
        {
            if ( numBinds <= 1 )
            {
                ListView_SetItemText(hListCtrl, itemIdx, 1, "");

                LVITEM item;
                item.mask     = LVIF_PARAM;
                item.iItem    = itemIdx;
                item.iSubItem = 0;
                item.lParam   = actionIdx;
                ListView_SetItem(hListCtrl, &item);
            }
            else
            {
                ListView_DeleteItem(hListCtrl, itemIdx);
            }

            STD_ASSERT(actionIdx < JONESCONTROL_ACTION_NUMACTIONS); // Added
            for ( size_t i = bndIdx + 1; i < JONESCONTROL_ACTION_GETNUMBINDS(pScheme->aActions[actionIdx]); ++i )
            {
                pScheme->aActions[actionIdx][i] = pScheme->aActions[actionIdx][i + 1];
            }

            if ( listID == 1005 ) // Keyboard list
            {
                JONESCONTROL_ACTION_UPDATENUMBINDINGS_KEYBOARD(pScheme->aActions[actionIdx], -1);
            }
            else // Controller list
            {
                JONESCONTROL_ACTION_UPDATENUMBINDINGS_CONTROLLER(pScheme->aActions[actionIdx], -1);
            }
        }
    }
}
int J3DAPI jonesConfig_ShowAssignKeyDialog(HWND hWnd, JonesAssignKeyDialogData* pData)
{
    GetWindowLongPtr(hWnd, GWL_HINSTANCE); // ??

    int remapDlgResult = 1; // OK button
    int mapKeyDlgResult, index;
    do
    {
        memset(pData->aControlName, 0, sizeof(pData->aControlName));

        pData->controlId = 0;
        mapKeyDlgResult = JonesDialog_ShowDialog(MAKEINTRESOURCE(117), hWnd, jonesConfig_AssignKeyDlgProc, (LPARAM)pData);
        if ( mapKeyDlgResult == 2 ) // Cancel
        {
            break;
        }

        size_t actionID, offset;
        index = jonesConfig_AssignKey_CheckBindForKey(pData->pScheme, pData->controlId, &actionID, pData->listID, &offset);
        if ( actionID == pData->actionId )
        {
            char aErrorText[512] = { 0 };
            const char* pErrorText = jonesString_GetString("JONES_STR_SAMEMAP");
            if ( pErrorText )
            {
                STD_STRCPY(aErrorText, pErrorText);
            }
            else
            {
                memset(aErrorText, 0, sizeof(aErrorText));
            }

            char aActionName[512] = { 0 };
            const char* pActionName = jonesString_GetString(jonesConfig_aJonesCapControlActionNames[pData->actionId]);
            if ( pActionName )
            {
                STD_STRCPY(aActionName, pActionName);
            }

            if ( !aErrorText[0] || !aActionName[0] )
            {
                return 2;
            }

            char aControlName[256] = { 0 };
            jonesConfig_ControlToString(pData->controlId, aControlName);

            char aMsgText[256] = { 0 };
            STD_FORMAT(aMsgText, "\"%s\" %s %s.", aControlName, aErrorText, aActionName);
            jonesConfig_ShowMessageDialog(hWnd, "JONES_STR_CTRL_OPTS", aMsgText, 145);
            return 2;
        }

        // If index > -1 then there is already key assigned for some action.
        // Show reassign msg box
        if ( index > -1 )
        {
            JonesReAssignKeyDialogData redata;
            redata.pAssignKeyData = pData;
            redata.index          = index;
            redata.actionId       = actionID;
            redata.offset         = offset;
            redata.hAssignKeyDlg  = hWnd;
            remapDlgResult = jonesConfig_ShowReassignKeyMsgBox(hWnd, &redata);
        }
    } while ( mapKeyDlgResult == 1 && index > -1 && remapDlgResult != 1 );

    return mapKeyDlgResult;
}

int J3DAPI jonesConfig_AssignKey_CheckBindForKey(JonesControlsScheme* pScheme, size_t keyControlID, size_t* pFunctionId, int listID, size_t* pOffset)
{
    int index = -1;
    for ( size_t function = 0; function < STD_ARRAYLEN(pScheme->aActions); ++function )
    {
        size_t offset = 0;
        for ( size_t bindIndx = 1; bindIndx <= JONESCONTROL_ACTION_GETNUMBINDS(pScheme->aActions[function]); ++bindIndx )
        {
            if ( listID == 1006 // controller list
                && ((unsigned int)pScheme->aActions[function][bindIndx] >= STDCONTROL_JOYSTICK_FIRSTKID && (unsigned int)pScheme->aActions[function][bindIndx] <= STDCONTROL_JOYSTICK_LASTPOVCID
                    || (unsigned int)pScheme->aActions[function][bindIndx] >= JONESCONFIG_JOYAXISKID_UP && (unsigned int)pScheme->aActions[function][bindIndx] <= JONESCONFIG_JOYAXISKID_GROUND) )
            {
                ++index;
            }
            else if ( listID == 1005 // key list
                && ((unsigned int)pScheme->aActions[function][bindIndx] < STDCONTROL_MAX_KEYBOARD_BUTTONS || (unsigned int)pScheme->aActions[function][bindIndx] > STDCONTROL_JOYSTICK_LASTPOVCID)
                && ((unsigned int)pScheme->aActions[function][bindIndx] < JONESCONFIG_JOYAXISKID_UP || (unsigned int)pScheme->aActions[function][bindIndx] > JONESCONFIG_JOYAXISKID_GROUND) )
            {
                ++index;
            }

            if ( keyControlID == pScheme->aActions[function][bindIndx] )
            {
                // We found existing binding for keyID
                *pFunctionId = function;
                *pOffset = offset;
                STDLOG_STATUS("index: %i function: %i offset: %i\n", index, function, offset);
                return index;
            }

            ++offset;
        }

        if ( !JONESCONTROL_ACTION_GETNUMKEYBOARDBINDS(pScheme->aActions[function]) && listID == 1005 // key list
            || !JONESCONTROL_ACTION_GETNUMCONTROLLERBINDS(pScheme->aActions[function]) && listID == 1006 ) // controller list
        {
            ++index;
        }
    }

    return -1;
}

INT_PTR CALLBACK jonesConfig_AssignKeyDlgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    if ( uMsg > WM_INITDIALOG )
    {
        if ( uMsg == WM_COMMAND )
        {
            jonesConfig_AssignKeyDlg_HandleWM_COMMAND(hWnd, (uint16_t)wParam);
            return 0;
        }

        if ( uMsg != WM_TIMER )
        {
            return 0;
        }

        // Timer
        JonesAssignKeyDialogData* pData = (JonesAssignKeyDialogData*)GetWindowLongPtr(hWnd, DWL_USER);
        if ( !pData || pData->bLocked )
        {
            return 1;
        }

        jonesConfig_AssignControlKey_ReadKey(hWnd);
        return 0;
    }

    if ( uMsg == WM_INITDIALOG )
    {
        jonesConfig_hFontAssignKeyDlg = jonesConfig_InitDialog(hWnd, 0, 117);
        int inited = jonesConfig_InitAssignKeyDlg(hWnd, wParam, (JonesAssignKeyDialogData*)lParam);

        SetWindowPos(hWnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_SHOWWINDOW | SWP_NOMOVE | SWP_NOSIZE);
        return inited;
    }
    else
    {
        if ( uMsg != WM_DESTROY )
        {
            return 0;
        }

        jonesConfig_ResetDialogFont(hWnd, jonesConfig_hFontAssignKeyDlg);
    }

    return 1;
}

int J3DAPI jonesConfig_InitAssignKeyDlg(HWND hDlg, int a2, JonesAssignKeyDialogData* pData)
{
    J3D_UNUSED(a2);

    if ( pData->listID == 1006 )
    {
        const char* pText = jonesString_GetString("JONES_STR_NEXTBUTTON");
        if ( pText )
        {
            HWND hNextKeyCaptionCtrl = GetDlgItem(hDlg, 1054);
            SetWindowText(hNextKeyCaptionCtrl, pText);
        }
    }

    const char* pActionName = jonesString_GetString(jonesConfig_aJonesCapControlActionNames[pData->actionId]);
    if ( pActionName )
    {
        HWND hActionTextCtrl = GetDlgItem(hDlg, 1055);
        SetWindowText(hActionTextCtrl, pActionName);
    }

    SetTimer(hDlg, 1u, 100u, 0); // Set timer to periodically check for pressed key
    SetWindowLongPtr(hDlg, DWL_USER, (LONG_PTR)pData);
    return 1;
}

void J3DAPI jonesConfig_AssignControlKey_ReadKey(HWND hWnd)
{
    // Function checks if any of the key was pressed, and that case assings kid to pData->mapKeyDlgResult  and ends dialog

    JonesAssignKeyDialogData* pData = (JonesAssignKeyDialogData*)GetWindowLongPtr(hWnd, DWL_USER);

    int bJoyEnabled = stdConfig_GetBool(JONESCONTROL_CFG_CONTROLS_CONTROLLER, false);
    int bGotKy = 0;

    // Read controls
    jonesConfig_assignKey_bControlsActive = stdControl_ControlsActive();
    stdControl_SetActivation(1);
    stdControl_ReadControls();

    // Check if any of the axis were moved
    if ( bJoyEnabled && pData->listID == 1006 && jonesConfig_AssignControlKey_ReadJoyAxisKey(pData) ) // 1006 - controller list
    {
        stdControl_SetActivation(jonesConfig_assignKey_bControlsActive);
        bGotKy = 1;
        KillTimer(hWnd, 1u);
        EndDialog(hWnd, 1);
        return;
    }

    // Ok let's check if button was pressed
    for ( size_t keyNum = 0; keyNum < STDCONTROL_MAX_KEYID; ++keyNum )
    {
        int bJustPressd;
        if ( stdControl_ReadKey(keyNum, &bJustPressd) )
        {
            // 1005 - key list
            if ( pData->listID == 1005
                && (keyNum >= STDCONTROL_JOYSTICK_FIRSTKID
                    && keyNum <= STDCONTROL_JOYSTICK_LASTPOVCID
                    || keyNum >= JONESCONFIG_JOYAXISKID_UP && keyNum <= JONESCONFIG_JOYAXISKID_GROUND) )
            {
                const char* pMsgText = jonesString_GetString("JONES_STR_KEYONLY");
                if ( pMsgText )
                {
                    pData->bLocked = 1;
                    jonesConfig_ShowMessageDialog(hWnd, "JONES_STR_CTRL_OPTS", pMsgText, 145); // 145 - typewriter icon
                    pData->bLocked = 0;
                    InvalidateRect(hWnd, 0, 1);
                }
                return;
            }
            else if ( pData->listID == 1006 // Controller list
                && (keyNum < STDCONTROL_JOYSTICK_FIRSTKID || keyNum > STDCONTROL_JOYSTICK_LASTPOVCID)
                && (keyNum < JONESCONFIG_JOYAXISKID_UP || keyNum > JONESCONFIG_JOYAXISKID_GROUND) )
            {
                const char* pMsgText = jonesString_GetString("JONES_STR_BUTTONONLY");
                if ( pMsgText )
                {
                    pData->bLocked = 1;
                    jonesConfig_ShowMessageDialog(hWnd, "JONES_STR_CTRL_OPTS", pMsgText, 145); // 145 - typewriter icon
                    pData->bLocked = 0;
                    InvalidateRect(hWnd, 0, 1);
                }
                return;
            }
            else
            {
                jonesConfig_ControlToString(keyNum, pData->aControlName);
                pData->controlId = keyNum;
                switch ( keyNum )
                {
                    case DIK_ESCAPE:
                    case DIK_MINUS:
                    case DIK_EQUALS:
                    case DIK_F4:
                    {
                        // Non mappable key, skip
                        const char* pMsgText = jonesString_GetString("JONES_STR_NO_MAP");
                        if ( pMsgText )
                        {
                            char aMsgText[256] = { 0 }; // Added: Init to 0
                            STD_FORMAT(aMsgText, pMsgText, pData->aControlName);

                            pData->bLocked = 1;
                            jonesConfig_ShowMessageDialog(hWnd, "JONES_STR_CTRL_OPTS", aMsgText, 145); // 145 - typewriter icon
                            pData->bLocked = 0;
                        }

                        memset(pData->aControlName, 0, sizeof(pData->aControlName));
                        return;
                    }
                    case DIK_1:
                    case DIK_2:
                    case DIK_3:
                    case DIK_4:
                    case DIK_5:
                    case DIK_6:
                    case DIK_7:
                    case DIK_8:
                    case DIK_9:
                    case DIK_0:
                    {
                        if ( pData->actionId >= JONESCONTROL_ACTION_BAZOOKA && pData->actionId <= JONESCONTROL_ACTION_GRENADE ) // 16 - fist; 27 - grenade
                        {
                           // Got Key close dialog
                            stdControl_SetActivation(jonesConfig_assignKey_bControlsActive);
                            bGotKy = 1;
                            KillTimer(hWnd, 1u);
                            EndDialog(hWnd, 1);
                        }

                        // Only weapon key and we have non-weapon action
                        const char* pMsgText = jonesString_GetString("JONES_STR_NO_KEYMAP_NUM");
                        if ( pMsgText )
                        {
                            char aMsgText[256] = { 0 }; // Added: Init to 0
                            STD_FORMAT(aMsgText, pMsgText, pData->aControlName);

                            pData->bLocked = 1;
                            jonesConfig_ShowMessageDialog(hWnd, "JONES_STR_CTRL_OPTS", aMsgText, 145); // 145 - typewriter icon
                            pData->bLocked = 0;
                            InvalidateRect(hWnd, 0, 1);
                        }

                        memset(pData->aControlName, 0, sizeof(pData->aControlName));
                        return;
                    }
                    default:
                    {
                       // Got Key close dialog
                        stdControl_SetActivation(jonesConfig_assignKey_bControlsActive);
                        bGotKy = 1;
                        KillTimer(hWnd, 1u);
                        EndDialog(hWnd, 1);
                        return;
                    }
                }
            }

            return;
        }
    }
}

int J3DAPI jonesConfig_AssignControlKey_ReadJoyAxisKey(JonesAssignKeyDialogData* pData)
{
    size_t numJoysticks = stdControl_GetNumJoysticks();
    if ( !numJoysticks )
    {
        return 0;
    }

    for ( size_t joyNum = 0; joyNum < numJoysticks; ++joyNum )
    {
        for ( size_t axisNum = 0; axisNum < STDCONTROL_JOYSTICK_NUMAXES; ++axisNum )
        {
            int bJustPressd;
            if ( stdControl_ReadAxisAsKey(STDCONTROL_GET_JOYSTICK_AXIS(joyNum, axisNum) | STDCONTROL_AID_POSITIVE_AXIS, &bJustPressd) )
            {
                if ( axisNum )
                {
                    if ( axisNum == STDCONTROL_JOYSTICK_AXIS_Y )
                    {
                        pData->controlId = JONESCONFIG_JOYAXISKID_DOWN;
                    }

                    else if ( axisNum == STDCONTROL_JOYSTICK_AXIS_RZ )
                    {
                        pData->controlId = JONESCONFIG_JOYAXISKID_TWISTR;
                    }
                }
                else
                {
                    // axisNum == STDCONTROL_JOYSTICK_AXIS_X
                    pData->controlId = JONESCONFIG_JOYAXISKID_RIGHT;
                }

                if ( pData->controlId )
                {
                    jonesConfig_ControlToString(pData->controlId, pData->aControlName);
                    return 1;
                }
            }
            else if ( stdControl_ReadAxisAsKey(STDCONTROL_GET_JOYSTICK_AXIS(joyNum, axisNum) | STDCONTROL_AID_NEGATIVE_AXIS, &bJustPressd) )
            {
                if ( axisNum )
                {
                    if ( axisNum == STDCONTROL_JOYSTICK_AXIS_Y )
                    {
                        pData->controlId = JONESCONFIG_JOYAXISKID_UP;
                    }

                    else if ( axisNum == STDCONTROL_JOYSTICK_AXIS_RZ )
                    {
                        pData->controlId = JONESCONFIG_JOYAXISKID_TWISTL;
                    }
                }
                else
                {
                    // axisNum == STDCONTROL_JOYSTICK_AXIS_X
                    pData->controlId = JONESCONFIG_JOYAXISKID_LEFT;
                }

                if ( pData->controlId )
                {
                    jonesConfig_ControlToString(pData->controlId, pData->aControlName);
                    return 1;
                }
            }
        }
    }

    return 0;
}

void J3DAPI jonesConfig_AssignKeyDlg_HandleWM_COMMAND(HWND hWnd, int ctrlID)
{
    if ( ctrlID == 2 )
    {
        KillTimer(hWnd, 1u);
        EndDialog(hWnd, 2);
    }
}

int J3DAPI jonesConfig_ShowReassignKeyMsgBox(HWND hWnd, JonesReAssignKeyDialogData* pData)
{
    GetWindowLongPtr(hWnd, GWL_HINSTANCE);
    return JonesDialog_ShowDialog(MAKEINTRESOURCE(120), hWnd, jonesConfig_ReassignKeyDialogProc, (LPARAM)pData);
}

INT_PTR CALLBACK jonesConfig_ReassignKeyDialogProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch ( uMsg )
    {
        case WM_DESTROY:
            jonesConfig_ResetDialogFont(hWnd, jonesConfig_hFontReassignKeyDlg);
            return 1;
        case WM_INITDIALOG:
        {
            jonesConfig_hFontReassignKeyDlg = jonesConfig_InitDialog(hWnd, 0, 120);
            int result  = jonesConfig_InitReassignKeyDialog(hWnd, wParam, (JonesReAssignKeyDialogData*)lParam);

            SetWindowPos(hWnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_SHOWWINDOW | SWP_NOMOVE | SWP_NOSIZE);
            return result;
        }
        case WM_COMMAND:
            jonesConfig_ReassignKeyDlg_HandleWM_COMMAND(hWnd, (uint16_t)wParam);
            return 0;
        default:
            return 0;
    }
}

int J3DAPI jonesConfig_InitReassignKeyDialog(HWND hDlg, int a2, JonesReAssignKeyDialogData* pData)
{
    J3D_UNUSED(a2);

    char aControlName[256] = { 0 };
    jonesConfig_ControlToString(pData->pAssignKeyData->controlId, aControlName);

    char aText[256] = { 0 };
    STD_FORMAT(aText, "\"%s\" ", aControlName);

    const char* pText = jonesString_GetString("JONES_STR_NEWMAP");
    if ( pText )
    {
        char aNewMapText[256] = { 0 };
        STD_FORMAT(aNewMapText, pText, aText);

        HWND hTextCtrl = GetDlgItem(hDlg, 1066);
        SetWindowText(hTextCtrl, aNewMapText);
    }

    char aFormat[512] = { 0 };
    pText = jonesString_GetString("JONES_STR_FROM");
    if ( pText )
    {
        STD_STRCPY(aFormat, pText);
    }
    else
    {
        memset(aFormat, 0, sizeof(aFormat));
    }

    char aActionName[512] = { 0 };
    const char* pActionName = jonesString_GetString(jonesConfig_aJonesCapControlActionNames[pData->actionId]);
    if ( pActionName )
    {
        STD_STRCPY(aActionName, pActionName);
    }
    else
    {
        memset(aActionName, 0, sizeof(aActionName));
    }

    if ( aFormat[0] && aActionName[0] )
    {
        memset(aText, 0, sizeof(aText));
        STD_FORMAT(aText, aFormat, aActionName);

        HWND hTextCtrl = GetDlgItem(hDlg, 1073);
        SetWindowText(hTextCtrl, aText);
    }

    memset(aControlName, 0, sizeof(aControlName));
    pText = jonesString_GetString("JONES_STR_TO");
    if ( pText )
    {
        STD_STRCPY(aFormat, pText);
    }
    else
    {
        memset(aFormat, 0, sizeof(aFormat));
    }

    pActionName = jonesString_GetString(jonesConfig_aJonesCapControlActionNames[pData->pAssignKeyData->actionId]);
    if ( pActionName )
    {
        STD_STRCPY(aActionName, pActionName);
    }
    else
    {
        memset(aActionName, 0, sizeof(aActionName));
    }

    if ( aFormat[0] && aActionName[0] )
    {
        HWND hTextCtrl = GetDlgItem(hDlg, 1072);
        STD_FORMAT(aControlName, aFormat, aActionName);
        SetWindowText(hTextCtrl, aControlName);
    }

    SetWindowLongPtr(hDlg, DWL_USER, (LONG)pData);
    return 1;
}

void J3DAPI jonesConfig_ReassignKeyDlg_HandleWM_COMMAND(HWND hWnd, unsigned int cntrlId)
{
    JonesReAssignKeyDialogData* pData = (JonesReAssignKeyDialogData*)GetWindowLongPtr(hWnd, DWL_USER);
    JonesControlsScheme* pScheme = pData->pAssignKeyData->pScheme;

    if ( cntrlId == 1 ) // Yes button
    {
        HWND hList = GetDlgItem(pData->hAssignKeyDlg, pData->pAssignKeyData->listID);

        int numBinds;
        if ( pData->pAssignKeyData->listID == 1005 )// Key list
        {
            numBinds = JONESCONTROL_ACTION_GETNUMKEYBOARDBINDS(pScheme->aActions[pData->actionId]);
        }
        else
        {
            numBinds = JONESCONTROL_ACTION_GETNUMCONTROLLERBINDS(pScheme->aActions[pData->actionId]);
        }

        if ( numBinds <= 1 )
        {
            if ( numBinds == 1 )
            {
                LVITEM item;
                item.mask     = LVIF_PARAM;
                item.iItem    = pData->index;
                item.iSubItem = 0;
                item.lParam   = pData->actionId;
                ListView_SetItem(hList, &item);

                // Set Empty text
                ListView_SetItemText(hList, pData->index, 1, "");
            }
        }
        else
        {
            ListView_DeleteItem(hList, pData->index);
        }

        for ( size_t i = pData->offset + 1; i < JONESCONTROL_ACTION_GETNUMBINDS(pScheme->aActions[pData->actionId]); ++i )
        {
            if ( i < STD_ARRAYLEN(pScheme->aActions[pData->actionId]) - 1 ) // Added: Added check that we don't go OOB
            {
                pScheme->aActions[pData->actionId][i] = pScheme->aActions[pData->actionId][i + 1];
            }
        }

        if ( pData->pAssignKeyData->listID == 1005 ) // key list
        {
            JONESCONTROL_ACTION_UPDATENUMBINDINGS_KEYBOARD(pScheme->aActions[pData->actionId], -1);
           //pScheme->aActions[pData->actionId][0] = ((uint8_t)pScheme->aActions[pData->actionId][0] - 1) | ((((pScheme->aActions[pData->actionId][0] & 0xFF00u) >> 8) - 1) << 8) | ((pScheme->aActions[pData->actionId][0] & 0xFF0000u) >> 16 << 16);
        }
        else // controller list
        {
            JONESCONTROL_ACTION_UPDATENUMBINDINGS_CONTROLLER(pScheme->aActions[pData->actionId], -1);
            //pScheme->aActions[pData->actionId][0] = ((uint8_t)pScheme->aActions[pData->actionId][0] - 1) | ((pScheme->aActions[pData->actionId][0] & 0xFF00u) >> 8 << 8) | ((((pScheme->aActions[pData->actionId][0] & 0xFF0000u) >> 16) - 1) << 16);
        }

        EndDialog(hWnd, cntrlId);
    }

    if ( cntrlId == 2 ) // No button
    {
        EndDialog(hWnd, cntrlId);
    }
}
int J3DAPI jonesConfig_ShowDisplaySettingsDialog(HWND hWnd, StdDisplayEnvironment* pDisplayEnv, JonesDisplaySettings* pDSettings)
{
    GetWindowLongPtr(hWnd, GWL_HINSTANCE); // ??

    JonesDisplaySettingsDialogData dlgData = { 0 };
    dlgData.pSettings   = pDSettings;
    dlgData.pDisplayEnv = pDisplayEnv;

    // Save current settings, to restore in case of canceling
    float curFogDensity = sithRender_g_fogDensity;
    int curPerLevel     = stdConfig_GetInt(JONESDISPLAY_CFG_GRAPHICS_PERFORMANCELEVEL, 4);

    JonesDisplaySettings curSettings = *pDSettings; // Changed: Copy fogDensity field

    int res = JonesDialog_ShowDialog(MAKEINTRESOURCE(114), hWnd, jonesConfig_DisplaySettingsDialogProc, (LPARAM)&dlgData);
    if ( dlgData.aVideoModes )
    {
        stdMemory_Free(dlgData.aVideoModes);
    }

    if ( dlgData.aCurColorDepthResolutions )
    {
        stdMemory_Free(dlgData.aCurColorDepthResolutions);
    }

    if ( res == 1 ) // OK btn clicked
    {
        int bMouseEnabled = stdConfig_GetBool(JONESCONTROL_CFG_CONTROLS_MOUSE, false);
        jonesConfig_EnableMouseControl(bMouseEnabled);
    }
    else // Cancel clicked
    {
        //  Restore to previous settings
        sithRender_g_fogDensity = curFogDensity;
        stdConfig_SetInt(JONESDISPLAY_CFG_GRAPHICS_PERFORMANCELEVEL, curPerLevel);

        *pDSettings = curSettings; // Changed: Copy fogDensity field
    }

    return res;
}
INT_PTR CALLBACK jonesConfig_DisplaySettingsDialogProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    int inited = 1;
    if ( uMsg > WM_INITDIALOG )
    {
        if ( uMsg == WM_COMMAND )
        {
            jonesConfig_DisplaySettings_HandleWM_COMMAND(hWnd, LOWORD(wParam), lParam, HIWORD(wParam));
            return 0;
        }

        if ( uMsg != WM_HSCROLL )
        {
            return 0;
        }

        if ( !jonesConfig_displaySettings_bUpdateSliderText )
        {
            return 1;
        }

        JonesDisplaySettingsDialogData* pData = (JonesDisplaySettingsDialogData*)GetWindowLongPtr(hWnd, DWL_USER);
        jonesConfig_HandleWM_HSCROLL(hWnd, (HWND)lParam, LOWORD(wParam));

        HWND hResolutionSliderCtrl = GetDlgItem(hWnd, 1050);
        int sliderPos = (int)SendMessage(hResolutionSliderCtrl, TBM_GETPOS, 0, 0);

        HWND hResText = GetDlgItem(hWnd, 1097);
        SetWindowText(hResText, pData->aVideoModes[sliderPos].aResolutionText);
        return 1;
    }

    if ( uMsg == WM_INITDIALOG )
    {
        jonesConfig_hFontDisplaySettingsDlg = jonesConfig_InitDialog(hWnd, 0, 114);
        inited = jonesConfig_InitDisplaySettingsDialog(hWnd, wParam, (JonesDisplaySettingsDialogData*)lParam);
        SetWindowPos(hWnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_SHOWWINDOW | SWP_NOMOVE | SWP_NOSIZE);
    }
    else
    {
        if ( uMsg != WM_DESTROY )
        {
            return 0;
        }

        jonesConfig_ResetDialogFont(hWnd, jonesConfig_hFontDisplaySettingsDlg);
    }

    return inited;
}

int J3DAPI jonesConfig_InitDisplaySettingsDialog(HWND hDlg, int a2, JonesDisplaySettingsDialogData* pData)
{
    J3D_UNUSED(a2);

   // Init color depth combo-box
    HWND hCBColorDepth = GetDlgItem(hDlg, 1094);
    ComboBox_ResetContent(hCBColorDepth);

    if ( pData->aVideoModes )
    {
        stdMemory_Free(pData->aVideoModes);
        pData->aVideoModes   = NULL;
        pData->numVideoModes = 0;
    }

    pData->aVideoModes = (JonesDisplaySettingsVideoMode*)STDMALLOC(
        sizeof(JonesDisplaySettingsVideoMode) * pData->pDisplayEnv->aDisplayInfos[pData->pSettings->displayDeviceNum].numModes
    );
    if ( !pData->aVideoModes )
    {
        return 0;
    }

    memset(pData->aVideoModes, 0, sizeof(JonesDisplaySettingsVideoMode) * pData->pDisplayEnv->aDisplayInfos[pData->pSettings->displayDeviceNum].numModes);

    if ( pData->aCurColorDepthResolutions )
    {
        stdMemory_Free(pData->aCurColorDepthResolutions);
        pData->aCurColorDepthResolutions   = NULL;
        pData->numCurColorDepthResolutions = 0;
    }

    StdDisplayInfo* pDisplayInfo = &pData->pDisplayEnv->aDisplayInfos[pData->pSettings->displayDeviceNum];

    HWND hBtnOk = GetDlgItem(hDlg, 1);
    if ( pDisplayInfo->displayDevice.bHAL )
    {
        EnableWindow(hBtnOk, 1);
        EnableWindow(hCBColorDepth, 1);
    }
    else
    {
        EnableWindow(hBtnOk, 0);
        EnableWindow(hCBColorDepth, 0);
    }

    StdVideoMode curVideoMode = pDisplayInfo->aModes[pData->pSettings->videoModeNum];
    bool bHasCurVideoMode     = false;
    size_t selVideoModeNum    = 0;
    int selCBColorDepthData   = 0; // Fixed: Init to 0

    for ( size_t i = 0; i < pData->pDisplayEnv->aDisplayInfos[pData->pSettings->displayDeviceNum].numModes; ++i )
    {
        const char* pFormat = jonesString_GetString("JONES_STR_BYPIXELS");
        char aResolutionText[256] = { 0 };
        if ( pFormat )
        {
            STD_FORMAT(aResolutionText, pFormat, pDisplayInfo->aModes[i].rasterInfo.width, pDisplayInfo->aModes[i].rasterInfo.height);
            if ( pDisplayInfo->aModes[i].refreshRate > 0 )
            {
                size_t pos = strlen(aResolutionText);
                stdUtil_Format(&aResolutionText[pos], STD_ARRAYLEN(aResolutionText) - pos, " (%d Hz)", pDisplayInfo->aModes[i].refreshRate);
            }
        }

        if ( pDisplayInfo->aModes[i].aspectRatio == 1.0f && pDisplayInfo->aModes[i].rasterInfo.width >= 512 && pDisplayInfo->aModes[i].rasterInfo.height >= 384 )
        {
            if ( jonesConfig_DisplaySettings_Get3DDeviceSupportsBPP(pDisplayInfo, pData->pSettings, pDisplayInfo->aModes[i].rasterInfo.colorInfo.bpp) )
            {
                int videomodeNum = jonesConfig_DisplaySettings_GetVideoModeNum(pData->aVideoModes, aResolutionText, pData->numVideoModes);
                if ( videomodeNum != -1 )
                {
                    // Check if mode for this resolution is not assigned yet
                    if ( (size_t)videomodeNum == pData->numVideoModes )
                    {
                        STD_STRCPY(pData->aVideoModes[videomodeNum].aResolutionText, aResolutionText);
                        pData->numVideoModes++;
                    }

                    // Fill color depth combo box

                    int colorDepthIdx = 0; // Fixed: Init to 0
                    int colorDepthMask = 0; // Fixed: Init to 0
                    switch ( pDisplayInfo->aModes[i].rasterInfo.colorInfo.bpp )
                    {
                        case 16:
                            colorDepthIdx = 2;
                            colorDepthMask = 4;
                            pFormat = jonesString_GetString("JONES_STR_16BBP");
                            if ( pFormat )
                            {
                                STD_FORMAT(aResolutionText, pFormat);
                            }

                            break;

                        case 24:
                            colorDepthIdx = 3;
                            colorDepthMask = 1 << colorDepthIdx;
                            pFormat = jonesString_GetString("JONES_STR_24BBP");
                            if ( pFormat )
                            {
                                STD_FORMAT(aResolutionText, pFormat);
                            }

                            break;

                        case 32:
                            colorDepthIdx   = 4;
                            colorDepthMask = 1 << colorDepthIdx;
                            pFormat = jonesString_GetString("JONES_STR_32BBP");
                            if ( pFormat )
                            {
                                STD_FORMAT(aResolutionText, pFormat);
                            }
                            break;

                        default:
                            // TODO: Shall we continue loop or break here?
                            break;
                    }

                    pData->aVideoModes[videomodeNum].colorDepthFlags |= colorDepthMask;
                    pData->aVideoModes[videomodeNum].aColorDepthVideoModes[colorDepthIdx] = i;

                    if ( ComboBox_FindString(hCBColorDepth, 0, aResolutionText) == -1 )
                    {
                        selCBColorDepthData = MAKELONG(colorDepthIdx, colorDepthMask);// Packs pixel byte size and
                        int index  = ComboBox_AddString(hCBColorDepth, aResolutionText);
                        ComboBox_SetItemData(hCBColorDepth, index, selCBColorDepthData);
                    }

                    if ( pDisplayInfo->aModes[i].rasterInfo.width == curVideoMode.rasterInfo.width
                        && pDisplayInfo->aModes[i].rasterInfo.height == curVideoMode.rasterInfo.height
                        && pDisplayInfo->aModes[i].rasterInfo.colorInfo.bpp == curVideoMode.rasterInfo.colorInfo.bpp
                        && (pDisplayInfo->aModes[i].refreshRate == 0 || pDisplayInfo->aModes[i].refreshRate == curVideoMode.refreshRate) ) // Added: Add refresh rate check
                    {
                        selVideoModeNum  = (size_t)videomodeNum;
                        bHasCurVideoMode = true;
                        ComboBox_SetCurSel(hCBColorDepth, ComboBox_FindString(hCBColorDepth, 0, aResolutionText));
                    }
                }
            }
        }
    }

    // Init screen resolutions for selected color depth

    pData->aCurColorDepthResolutions = (const JonesDisplaySettingsVideoMode**)STDMALLOC(sizeof(const JonesDisplaySettingsVideoMode**) * pData->numVideoModes);
    if ( !pData->aCurColorDepthResolutions )
    {
        return 0;
    }

    // Altered: Moved memset here from below
    memset(pData->aCurColorDepthResolutions, 0, sizeof(const JonesDisplaySettingsVideoMode**) * pData->numVideoModes);

    if ( !bHasCurVideoMode )
    {
        // No color depth selected, so we select first video mode
        selVideoModeNum     = 0;
        selCBColorDepthData = MAKELONG(0, pData->aVideoModes[0].colorDepthFlags);
    }

    HWND hResTextCtrl = GetDlgItem(hDlg, 1097);
    char aResText[256] = { 0 }; // Added: Init to 0
    GetWindowText(hResTextCtrl, aResText, STD_ARRAYLEN(aResText));

    for ( size_t i = 0; i < pData->numVideoModes; ++i )
    {
        if ( (HIWORD(selCBColorDepthData) & pData->aVideoModes[i].colorDepthFlags) != 0 )
        {
            if ( strlen(aResText) && streq(aResText, pData->aVideoModes[i].aResolutionText) )
            {
                selVideoModeNum = pData->numCurColorDepthResolutions;
            }
            else if ( selVideoModeNum == i )
            {
                selVideoModeNum = pData->numCurColorDepthResolutions;
            }

            pData->aCurColorDepthResolutions[pData->numCurColorDepthResolutions++] = &pData->aVideoModes[i];
        }
    }

    // Init resolution slider
    HWND hCtrl = GetDlgItem(hDlg, 1050);
    SendMessage(hCtrl, TBM_SETRANGE, /*re-draw=*/TRUE, MAKELONG(0, pData->numCurColorDepthResolutions - 1));
    SendMessage(hCtrl, TBM_SETPOS, /*re-draw=*/TRUE, selVideoModeNum);
    SendMessage(hCtrl, TBM_SETTICFREQ, 1u, 0);
    SendMessage(hCtrl, TBM_SETPAGESIZE, 0, 1); // Fixed: Was sending TBM_SETPAGESIZE message to text control below

  // Init resolution text control
    hCtrl = GetDlgItem(hDlg, 1097);
    SetWindowText(hCtrl, pData->aCurColorDepthResolutions[selVideoModeNum]->aResolutionText);

    if ( !bHasCurVideoMode )
    {
        ComboBox_SetCurSel(hCBColorDepth, 0);
    }

    SetWindowLongPtr(hDlg, DWL_USER, (LONG_PTR)pData);
    return 1;
}

int J3DAPI jonesConfig_DisplaySettings_GetVideoModeNum(const JonesDisplaySettingsVideoMode* pVideoMode, const char* pText, size_t numVideoModes)
{
    if ( !pText )
    {
        return -1;
    }

    size_t i = 0;
    for ( ; i < numVideoModes; ++i )
    {
        if ( streq(pVideoMode[i].aResolutionText, pText) )
        {
            return i;
        }
    }

    return i;
}

void J3DAPI jonesConfig_DisplaySettings_HandleWM_COMMAND(HWND hWnd, int ctrlID, LPARAM a3, int notifyCode)
{
    J3D_UNUSED(a3);

    JonesDisplaySettingsDialogData* pData = (JonesDisplaySettingsDialogData*)GetWindowLongPtr(hWnd, DWL_USER);

    JonesDisplaySettings* pSettings    = pData->pSettings;
    StdDisplayEnvironment* pDisplayEnv = pData->pDisplayEnv;

    if ( ctrlID <= 1093 ) // // Advance settings clicked
    {
        switch ( ctrlID )
        {
            case 1093: // Advance settings clicked
                jonesConfig_ShowAdvanceDisplaySettings(hWnd, pData);
                InvalidateRect(hWnd, 0, 1);
                return;

            case 1: // OK btn
            {
                // Save new screen resolution data

                HWND hResSliderCtrl = GetDlgItem(hWnd, 1050);// resolution slider control
                int curSliderPos = (int)SendMessage(hResSliderCtrl, TBM_GETPOS, 0, 0);

                HWND hCBColorDepth = GetDlgItem(hWnd, 1094);// color depth slider control
                int curSelColorDepthIdx = ComboBox_GetCurSel(hCBColorDepth);

                // Fixed: Added null check.
                pSettings->videoModeNum = 0;
                if ( *pData->aCurColorDepthResolutions )
                {
                    pSettings->videoModeNum = pData->aCurColorDepthResolutions[curSliderPos]->aColorDepthVideoModes[LOWORD(ComboBox_GetItemData(hCBColorDepth, curSelColorDepthIdx))];
                }

                pSettings->width = pDisplayEnv->aDisplayInfos[pSettings->displayDeviceNum].aModes[pSettings->videoModeNum].rasterInfo.width;
                pSettings->height = pDisplayEnv->aDisplayInfos[pSettings->displayDeviceNum].aModes[pSettings->videoModeNum].rasterInfo.height;

                stdConfig_SetString(JONESDISPLAY_CFG_GRAPHICS_DISPLAY, pDisplayEnv->aDisplayInfos[pSettings->displayDeviceNum].displayDevice.aDriverName);
                stdConfig_SetString(JONESDISPLAY_CFG_GRAPHICS_DEVICE, pDisplayEnv->aDisplayInfos[pSettings->displayDeviceNum].aDevices[pSettings->device3DNum].deviceDescription);
                stdConfig_SetInt(JONESDISPLAY_CFG_GRAPHICS_WIDTH, pSettings->width);
                stdConfig_SetInt(JONESDISPLAY_CFG_GRAPHICS_HEIGHT, pSettings->height);
                stdConfig_SetInt(JONESDISPLAY_CFG_GRAPHICS_BPP, pDisplayEnv->aDisplayInfos[pSettings->displayDeviceNum].aModes[pSettings->videoModeNum].rasterInfo.colorInfo.bpp);
                stdConfig_SetInt(JONESDISPLAY_CFG_GRAPHICS_REFRESHRATE, pDisplayEnv->aDisplayInfos[pSettings->displayDeviceNum].aModes[pSettings->videoModeNum].refreshRate); // Added

                // Set new fog config

                float fogDensity = sithRender_g_fogDensity / 100.0f;
                stdConfig_SetFloat(JONESDISPLAY_CFG_ENGINE_RENDER_FOGDENSITY, fogDensity);

                stdConfig_SetBool(JONESDISPLAY_CFG_ENGINE_RENDER_FOGENABLED, pSettings->bFog);
                std3D_EnableFog(pSettings->bFog, sithRender_g_fogDensity);

                if ( sithWorld_g_pCurrentWorld )
                {
                    sithWorld_g_pCurrentWorld->state |= SITH_WORLD_STATE_UPDATE_FOG;
                }

                // Save buffering and tex filtering mode
                stdConfig_SetBool(JONESDISPLAY_CFG_GRAPHICS_BUFFERING, pSettings->bBuffering);
                stdConfig_SetInt(JONESDISPLAY_CFG_GRAPHICS_MIPMAPFILTER, pSettings->filter);

                EndDialog(hWnd, ctrlID);
                return;
            }

            case 2: // Cancel btn
                EndDialog(hWnd, ctrlID);
                return;

            default:
                return;
        }

        return;
    }

    if ( ctrlID == 1094 ) // Color depth combo box
    {
        if ( notifyCode == CBN_SELCHANGE )
        {
            // Selection of color depth combo box changed,
            // we update aCurColorDepthResolutions for new the color depth

            HWND hCBColorDepth = GetDlgItem(hWnd, 1094);

            HWND hResSliderCtrl = GetDlgItem(hWnd, 1050);
            HWND hResTextCtrl = GetDlgItem(hWnd, 1097);// resolution text control

            int curSelColorDepthIdx = ComboBox_GetCurSel(hCBColorDepth);
            int colorDepthMask = ComboBox_GetItemData(hCBColorDepth, curSelColorDepthIdx);

            // Clear cur color depth res array
            memset(pData->aCurColorDepthResolutions, 0, sizeof(pData->aCurColorDepthResolutions) * pData->numCurColorDepthResolutions);

            char aResText[256] = { 0 };
            GetWindowText(hResTextCtrl, aResText, STD_ARRAYLEN(aResText));
            pData->numCurColorDepthResolutions = 0;

            size_t newItemIdx = 0; // Fixed: Init to 0
            for ( size_t i = 0; i < pData->numVideoModes; ++i )
            {
                if ( (HIWORD(colorDepthMask) & pData->aVideoModes[i].colorDepthFlags) != 0 )
                {
                    if ( streq(aResText, pData->aVideoModes[i].aResolutionText) )
                    {
                        newItemIdx = pData->numCurColorDepthResolutions;
                    }

                    pData->aCurColorDepthResolutions[pData->numCurColorDepthResolutions++] = &pData->aVideoModes[i];
                }
            }

            SendMessage(hResSliderCtrl, TBM_SETRANGE, /*re-draw=*/TRUE, MAKELONG(0, pData->numCurColorDepthResolutions - 1));
            SendMessage(hResSliderCtrl, TBM_SETPOS, /*re-draw=*/TRUE, newItemIdx);
            SendMessage(hResSliderCtrl, TBM_SETTICFREQ, 1u, 0);
            SetWindowText(hResTextCtrl, pData->aCurColorDepthResolutions[newItemIdx]->aResolutionText);
        }
    }
    else if ( ctrlID == 1099 ) // Default button
    {
        HWND hCBColorDepth = GetDlgItem(hWnd, 1094);
        size_t numColorDepths = ComboBox_GetCount(hCBColorDepth);

        for ( size_t i = 0; i < numColorDepths; i++ )
        {
            int colorDepthMask = ComboBox_GetItemData(hCBColorDepth, i);
            if ( HIWORD(colorDepthMask) == 0x04 )  // 16 bpp
            {
                ComboBox_SetCurSel(hCBColorDepth, i);
                break;
            }
        }

        CheckDlgButton(hWnd, 1095, 0); // ??? unknown ctrl, in advance display settings this is very fast performance radio button

        const char* pResFormat = jonesString_GetString("JONES_STR_BYPIXELS");
        if ( pResFormat )
        {
            HWND hResSliderCtrl = GetDlgItem(hWnd, 1050);
            HWND hResTextCtrl   = GetDlgItem(hWnd, 1097);

            char aResText[256] = { 0 };
            STD_FORMAT(aResText, pResFormat, 640, 480);
            SetWindowText(hResTextCtrl, aResText);

            // Clear cur color depth res array
            memset(pData->aCurColorDepthResolutions, 0, sizeof(pData->aCurColorDepthResolutions) * pData->numCurColorDepthResolutions);
            pData->numCurColorDepthResolutions = 0;

            size_t selDefaultIdx = 0; // Fixed: Init to 0
            for ( size_t i = 0; i < pData->numVideoModes; ++i )
            {
                if ( (pData->aVideoModes[i].colorDepthFlags & 0x04) != 0 )// 16 bpp ie 1 << 2 (bytes)
                {
                    if ( streq(aResText, pData->aVideoModes[i].aResolutionText) )
                    {
                        selDefaultIdx = pData->numCurColorDepthResolutions;
                    }

                    pData->aCurColorDepthResolutions[pData->numCurColorDepthResolutions++] = &pData->aVideoModes[i];
                }
            }

            SendMessage(hResSliderCtrl, TBM_SETRANGE, /*re-draw=*/TRUE, MAKELONG(0, pData->numCurColorDepthResolutions - 1));
            SendMessage(hResSliderCtrl, TBM_SETPOS, /*re-draw=*/TRUE, selDefaultIdx);
            SendMessage(hResSliderCtrl, TBM_SETTICFREQ, 1u, 0);
            SetWindowText(hResTextCtrl, pData->aCurColorDepthResolutions[selDefaultIdx]->aResolutionText);
        }
    }
}
int J3DAPI jonesConfig_ShowAdvanceDisplaySettings(HWND hWnd, JonesDisplaySettingsDialogData* pData)
{
    return JonesDialog_ShowDialog(MAKEINTRESOURCE(148), hWnd, jonesConfig_AdvanceDisplaySettingsDialogProc, (LPARAM)pData);
}

INT_PTR CALLBACK jonesConfig_AdvanceDisplaySettingsDialogProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch ( uMsg )
    {
        case WM_DESTROY:
            jonesConfig_ResetDialogFont(hWnd, jonesConfig_hFontAdvanceDisplaySettingsDialog);
            return 1;

        case WM_INITDIALOG:
        {
            // Altered: Switched order so InitDialog affects any new items added to dialog
            int inited = jonesConfig_InitAdvanceDisplaySettingsDialog(hWnd, wParam, (JonesDisplaySettingsDialogData*)lParam);
            jonesConfig_hFontAdvanceDisplaySettingsDialog = jonesConfig_InitDialog(hWnd, NULL, 148);

            SetWindowPos(hWnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_SHOWWINDOW | SWP_NOMOVE | SWP_NOSIZE);
            return inited;
        }

        case WM_COMMAND:
            jonesConfig_AdvanceDisplaySettings_HandleWM_COMMAND(hWnd, LOWORD(wParam), lParam, HIWORD(wParam));
            return 0;

        default:
            return 0;
    }
}

int J3DAPI jonesConfig_DisplaySettings_Get3DDeviceSupportsBPP(const StdDisplayInfo* pDisplayInfo, const JonesDisplaySettings* pSettings, int bpp)
{
    if ( !pDisplayInfo )
    {
        return 0;
    }

#if defined(J3D_DIRECTX6)
    switch ( bpp )
    {
        case 16:
            return pDisplayInfo->aDevices[pSettings->device3DNum].d3dDesc.dwDeviceRenderBitDepth & DDBD_16;

        case 24:
            return pDisplayInfo->aDevices[pSettings->device3DNum].d3dDesc.dwDeviceRenderBitDepth & DDBD_24;

        case 32:
            return pDisplayInfo->aDevices[pSettings->device3DNum].d3dDesc.dwDeviceRenderBitDepth & DDBD_32;
    }
#elif defined(J3D_DIRECTX9)
    J3D_UNUSED(pDisplayInfo);
    J3D_UNUSED(pSettings);
    if ( bpp == 24 || bpp == 32 ) return 1;
#else
#error "Unsupported 3D API"
#endif

    return 0;
}

int J3DAPI jonesConfig_InitAdvanceDisplaySettingsDialog(HWND hDlg, int a2, JonesDisplaySettingsDialogData* pData)
{
    J3D_UNUSED(a2);

    // Init 3D dwevice combo box
    HWND hCB3DeviceCtrl = GetDlgItem(hDlg, 1084);    // 3D Devices combo box
    ComboBox_ResetContent(hCB3DeviceCtrl);

    for ( size_t i = 0; i < pData->pDisplayEnv->numInfos; ++i )
    {
        char aDBEntryText[256] = { 0 };
        if ( pData->pDisplayEnv->aDisplayInfos[i].displayDevice.bHAL == 1 )
        {
            STD_FORMAT(aDBEntryText, "%s", pData->pDisplayEnv->aDisplayInfos[i].displayDevice.aDriverName);
            int newItemIdx = ComboBox_AddString(hCB3DeviceCtrl, aDBEntryText);

            ComboBox_SetItemData(hCB3DeviceCtrl, newItemIdx, i); // i is index in aDisplayInfos

            if ( i == (size_t)pData->pSettings->displayDeviceNum )
            {
                ComboBox_SetCurSel(hCB3DeviceCtrl, newItemIdx);
            }
        }
    }

    // Init 3D Raster combo box
    HWND hCB3DRasterCtrl = GetDlgItem(hDlg, 1085);

    char aSelDeviceDesc[256] = { 0 };
    GetWindowText(hCB3DRasterCtrl, aSelDeviceDesc, STD_ARRAYLEN(aSelDeviceDesc));
    ComboBox_ResetContent(hCB3DRasterCtrl);

    StdDisplayInfo* pDisplayInfo = &pData->pDisplayEnv->aDisplayInfos[pData->pSettings->displayDeviceNum];
    if ( !pDisplayInfo->displayDevice.bHAL )
    {
        EnableWindow(hCB3DRasterCtrl, 0);
    }
    else
    {
        EnableWindow(hCB3DRasterCtrl, 1);

        bool bCurSel = false;
        for ( size_t i = 0; i < pDisplayInfo->numDevices; ++i )
        {
            char aDBEntryText[256] = { 0 };
            STD_FORMAT(aDBEntryText, "%s", pDisplayInfo->aDevices[i].deviceDescription);
            int newItemIdx = ComboBox_AddString(hCB3DRasterCtrl, aDBEntryText);

            ComboBox_SetItemData(hCB3DRasterCtrl, newItemIdx, i); // i is index in aDevices

            if ( streq(aSelDeviceDesc, aDBEntryText) || pData->pSettings->device3DNum == i )
            {
                ComboBox_SetCurSel(hCB3DRasterCtrl, newItemIdx);
                pData->pSettings->device3DNum = i;
                bCurSel = true;
            }
        }

        if ( !bCurSel )
        {
            // Set first device as selected
            ComboBox_SetCurSel(hCB3DRasterCtrl, 0);
            pData->pSettings->device3DNum = 0;
        }
    }

    // Init dialog radio buttons

    if ( pData->pSettings->bBuffering )
    {
        // Tripple buffering
        CheckRadioButton(hDlg, 1091, 1092, 1092);
    }
    else
    {
        // Double buffering
        CheckRadioButton(hDlg, 1091, 1092, 1091);
    }

    // Set checked mipmap radio button
    switch ( pData->pSettings->filter )
    {
        case STD3D_MIPMAPFILTER_NONE:
            CheckRadioButton(hDlg, 1088, 1090, 1088);
            jonesConfig_advanceDisplaySettings_curFilterMode = STD3D_MIPMAPFILTER_NONE; // Fixed: assign config filtermode to cache var, OG was missing this assignment
            break;

        case STD3D_MIPMAPFILTER_BILINEAR:
            CheckRadioButton(hDlg, 1088, 1090, 1089);
            jonesConfig_advanceDisplaySettings_curFilterMode = STD3D_MIPMAPFILTER_BILINEAR; // Fixed: assign config filtermode to cache var, OG was missing this assignment
            break;

        case STD3D_MIPMAPFILTER_TRILINEAR:
            CheckRadioButton(hDlg, 1088, 1090, 1090);
            jonesConfig_advanceDisplaySettings_curFilterMode = STD3D_MIPMAPFILTER_TRILINEAR; // Fixed: assign config filtermode to cache var, OG was missing this assignment
            break;
    }

    int perfLevel = stdConfig_GetInt(JONESDISPLAY_CFG_GRAPHICS_PERFORMANCELEVEL, 4);
    switch ( perfLevel )
    {
        case 0:
            CheckRadioButton(hDlg, 1093, 1095, 1093); // average
            break;

        case 2:
            CheckRadioButton(hDlg, 1093, 1095, 1094); // fast
            break;

        case 4:
            CheckRadioButton(hDlg, 1093, 1095, 1095); // very fast
            break;
    }

    // Set fog check box
    CheckDlgButton(hDlg, 1051, pData->pSettings->bFog);

    // Set fog slider
    HWND hFogSliderCtrl = GetDlgItem(hDlg, 1216);
    SendMessage(hFogSliderCtrl, TBM_SETRANGE, /*re-draw=*/TRUE, MAKELONG(0, 100));
    SendMessage(hFogSliderCtrl, TBM_SETTICFREQ, 5u, 0);
    SendMessage(hFogSliderCtrl, TBM_SETPOS, /*re-draw=*/TRUE, (LPARAM)sithRender_g_fogDensity);
    SendMessage(hFogSliderCtrl, TBM_SETPAGESIZE, 0, 5u);

    // New option items
    const int spacing = 15; // TODO: make it static
    UINT dpi = GetDpiForWindow(hDlg);
    int x = MulDiv(20, dpi, USER_DEFAULT_SCREEN_DPI);
    int y = MulDiv(190, dpi, USER_DEFAULT_SCREEN_DPI);
    int h = MulDiv(14, dpi, USER_DEFAULT_SCREEN_DPI);

    // Added: Adds check box for Anti-Aliasing (MSAA) option
    HWND hCbMSAA = GetDlgItem(hDlg, 1053);
    if ( std3D_IsMSAASupported() )
    {
        int w = MulDiv(120, dpi, USER_DEFAULT_SCREEN_DPI);

        // Fixed: Reuse the dynamically-created checkbox when the dialog is reinitialized.
        if ( !hCbMSAA )
        {
            hCbMSAA = CreateWindow(
                "Button",
                "Anti-Aliasing (MSAA)",
                WS_CHILD | BS_AUTOCHECKBOX | BS_VCENTER | WS_VISIBLE | WS_GROUP | WS_TABSTOP,
                x,
                y,
                w,
                h,
                hDlg,                  // Parent window handle
                (HMENU)1053,           // Control ID
                GetModuleHandle(NULL),
                NULL                   // Additional creation config
            );
        }

        ShowWindow(hCbMSAA, SW_SHOW);
        EnableWindow(hCbMSAA, TRUE);
        SetWindowPos(hCbMSAA, NULL, x, y, w, h, SWP_NOZORDER | SWP_NOREDRAW);
        CheckDlgButton(hDlg, 1053, stdConfig_GetBool(STD3D_CFG_MSAAENABLED, true));
    }
    else if ( hCbMSAA )
    {
        ShowWindow(hCbMSAA, SW_HIDE);
        EnableWindow(hCbMSAA, FALSE);
    }

    // Added: Adds check box for mipmap auto-gen option
    HWND hCbMipmapAutoGen = GetDlgItem(hDlg, 1054);
    if ( std3D_IsMipmapAutoGenSupported() )
    {
        y += MulDiv(spacing, dpi, USER_DEFAULT_SCREEN_DPI);
        int w = MulDiv(120, dpi, USER_DEFAULT_SCREEN_DPI);

        // Fixed: Reuse the dynamically-created checkbox when the dialog is reinitialized.
        if ( !hCbMipmapAutoGen )
        {
            hCbMipmapAutoGen = CreateWindow(
                "Button",
                "Auto Gen. Mipmaps",
                WS_CHILD | BS_AUTOCHECKBOX | BS_VCENTER | WS_VISIBLE | WS_GROUP | WS_TABSTOP,
                x,
                y,
                w,
                h,
                hDlg,                  // Parent window handle
                (HMENU)1054,           // Control ID
                GetModuleHandle(NULL),
                NULL                   // Additional creation config
            );
        }

        ShowWindow(hCbMipmapAutoGen, SW_SHOW);
        EnableWindow(hCbMipmapAutoGen, TRUE);
        SetWindowPos(hCbMipmapAutoGen, NULL, x, y, w, h, SWP_NOZORDER | SWP_NOREDRAW);
        CheckDlgButton(hDlg, 1054, stdConfig_GetBool(STD3D_CFG_MIPMAPAUTOGEN, true));
    }
    else if ( hCbMipmapAutoGen )
    {
        ShowWindow(hCbMipmapAutoGen, SW_HIDE);
        EnableWindow(hCbMipmapAutoGen, FALSE);
    }

    // Added: Adds check box for anisotropic filtering option
    HWND hCbAnisotropic = GetDlgItem(hDlg, 1055);
    if ( std3D_IsAnisotropicFilteringSupported() )
    {
        y += MulDiv(spacing, dpi, USER_DEFAULT_SCREEN_DPI);
        int w = MulDiv(120, dpi, USER_DEFAULT_SCREEN_DPI);

        // Fixed: Reuse the dynamically-created checkbox when the dialog is reinitialized.
        if ( !hCbAnisotropic )
        {
            hCbAnisotropic = CreateWindow(
                "Button",
                "Anisotropic Filtering",
                WS_CHILD | BS_AUTOCHECKBOX | BS_VCENTER | WS_VISIBLE | WS_GROUP | WS_TABSTOP,
                x,
                y,
                w,
                h,
                hDlg,                  // Parent window handle
                (HMENU)1055,           // Control ID
                GetModuleHandle(NULL),
                NULL                   // Additional creation config
            );
        }

        ShowWindow(hCbAnisotropic, SW_SHOW);
        EnableWindow(hCbAnisotropic, TRUE);
        SetWindowPos(hCbAnisotropic, NULL, x, y, w, h, SWP_NOZORDER | SWP_NOREDRAW);
        CheckDlgButton(hDlg, 1055, stdConfig_GetBool(STD3D_CFG_ANISOTROPICFILTER, true));
    }
    else if ( hCbAnisotropic )
    {
        ShowWindow(hCbAnisotropic, SW_HIDE);
        EnableWindow(hCbAnisotropic, FALSE);
    }

    // Added: Enable and init HiPoly check button
    HWND hHiPolyBtn = GetDlgItem(hDlg, 1052);
    EnableWindow(hHiPolyBtn, 1);
    ShowWindow(hHiPolyBtn, 1);
    CheckDlgButton(hDlg, 1052, sithModel_IsHiPolyEnabled());

    // Move HiPoly checkbox to new position
    y += MulDiv(spacing, dpi, USER_DEFAULT_SCREEN_DPI);
    SetWindowPos(hHiPolyBtn, NULL, x, y, MulDiv(105, dpi, USER_DEFAULT_SCREEN_DPI), h, SWP_NOZORDER | SWP_NOREDRAW);

    // Added: Move TV icon down
    x = MulDiv(35, dpi, USER_DEFAULT_SCREEN_DPI);
    y += MulDiv(spacing * 2, dpi, USER_DEFAULT_SCREEN_DPI);
    SetWindowPos(GetDlgItem(hDlg, 1098), NULL, x, y, 0, 0, SWP_NOZORDER | SWP_NOSIZE | SWP_NOREDRAW);

    SetWindowLongPtr(hDlg, DWL_USER, (LONG_PTR)pData);
    return 1;
}

void J3DAPI jonesConfig_AdvanceDisplaySettings_HandleWM_COMMAND(HWND hDlg, int ctrlID, int a3, int notifyCode)
{
    J3D_UNUSED(a3);

    JonesDisplaySettingsDialogData* pData = (JonesDisplaySettingsDialogData*)GetWindowLongPtr(hDlg, DWL_USER);
    JonesDisplaySettings* pSettings = pData->pSettings;

    switch ( ctrlID )
    {
        case 1084: // 3D Devices combo box
        {
            if ( notifyCode == CBN_SELCHANGE )
            {
                HWND hCB3DeviceCtrl = GetDlgItem(hDlg, 1084);
                HWND hDisplaySettingsDlg = GetParent(hDlg);

                int curSelIdx = ComboBox_GetCurSel(hCB3DeviceCtrl);
                pSettings->displayDeviceNum = ComboBox_GetItemData(hCB3DeviceCtrl, curSelIdx);

                jonesConfig_InitAdvanceDisplaySettingsDialog(hDlg, 0, pData);
                jonesConfig_InitDisplaySettingsDialog(hDisplaySettingsDlg, 0, pData);
            }
        } break;

        case 1085: // 3D Rasterizers combo box
        {
            if ( notifyCode == CBN_SELCHANGE )
            {
                // Fixed: Read the selected rasterizer from the rasterizer combo, not the 3D device combo. OG retrieved ctrlID 1084 which is 3D device combo
                HWND hCB3DRasterCtrl = GetDlgItem(hDlg, 1085);
                HWND hDisplaySettingsDlg = GetParent(hDlg);

                int curSelIdx = ComboBox_GetCurSel(hCB3DRasterCtrl);
                pSettings->device3DNum = ComboBox_GetItemData(hCB3DRasterCtrl, curSelIdx);

                jonesConfig_InitAdvanceDisplaySettingsDialog(hDlg, 0, pData);
                jonesConfig_InitDisplaySettingsDialog(hDisplaySettingsDlg, 0, pData);
            }
        } break;

        case 1088: // No Mipmap radio-button
            CheckRadioButton(hDlg, 1088, 1090, 1088);
            jonesConfig_advanceDisplaySettings_curFilterMode = STD3D_MIPMAPFILTER_NONE;
            break;

        case 1089: // Bilinear radio-button
            CheckRadioButton(hDlg, 1088, 1090, 1089);
            jonesConfig_advanceDisplaySettings_curFilterMode = STD3D_MIPMAPFILTER_BILINEAR;
            break;

        case 1090: // Trilinear radio-button
            CheckRadioButton(hDlg, 1088, 1090, 1090);
            jonesConfig_advanceDisplaySettings_curFilterMode = STD3D_MIPMAPFILTER_TRILINEAR;
            break;

        case 1091: // Dbl buffer radio-button
            CheckRadioButton(hDlg, 1091, 1092, 1091);
            pSettings->bBuffering = 0;
            break;

        case 1092: // Tripple buffer
            CheckRadioButton(hDlg, 1091, 1092, 1092);
            pSettings->bBuffering = 1;
            break;

        case 1093: // Average perf. level radio button
            CheckRadioButton(hDlg, 1093, 1095, 1093);
            jonesConfig_advanceDisplaySettings_perfLevel = 0;
            break;

        case 1094: // Fast perf. level radio button
            CheckRadioButton(hDlg, 1093, 1095, 1094);
            jonesConfig_advanceDisplaySettings_perfLevel = 2;
            break;

        case 1095: // Very fast perf. level radio button
            CheckRadioButton(hDlg, 1093, 1095, 1095);
            jonesConfig_advanceDisplaySettings_perfLevel = 4;
            break;

        case 1099: // Default button
        {
            CheckDlgButton(hDlg, 1051, 1u); // Fog check box
            pSettings->bFog = 1;
            sithRender_g_fogDensity = 100.0f;

            // Fog slider
            HWND hFogSliderCtrl = GetDlgItem(hDlg, 1216);
            SendMessage(hFogSliderCtrl, TBM_SETPOS, /*re-draw=*/TRUE, (LPARAM)100.0f);

            // Perf. level set to very fast
            CheckRadioButton(hDlg, 1093, 1095, 1095);
            jonesConfig_advanceDisplaySettings_perfLevel = 4;

            // Set buffering
            // Altered: Changed to triple buffering from OG double buffering
            CheckRadioButton(hDlg, 1091, 1092, 1092);
            pSettings->bBuffering = 1;

            // Set default to trilinear
            // Altered: Original was set to bilinear
            CheckRadioButton(hDlg, 1088, 1090, 1090);
            jonesConfig_advanceDisplaySettings_curFilterMode = STD3D_MIPMAPFILTER_TRILINEAR;
            break;
        }

        case 1: // OK btn
        {
            HWND hCBFogCtrl = GetDlgItem(hDlg, 1051);
            pSettings->bFog =  Button_GetCheck(hCBFogCtrl);

            HWND hFogSliderCtrl = GetDlgItem(hDlg, 1216);

            sithRender_g_fogDensity = (float)SendMessage(hFogSliderCtrl, TBM_GETPOS, 0, 0);
            pSettings->fogDensity = sithRender_g_fogDensity / 100.0f;

            // Fixed: Compare the selected performance level against the previously saved value before writing it.
            int curPerfLevel = stdConfig_GetInt(JONESDISPLAY_CFG_GRAPHICS_PERFORMANCELEVEL, 4);
            int newPerfLevel = jonesConfig_advanceDisplaySettings_perfLevel;
            stdConfig_SetInt(JONESDISPLAY_CFG_GRAPHICS_PERFORMANCELEVEL, newPerfLevel);

            if ( JonesMain_HasStarted() && newPerfLevel != curPerfLevel )
            {
                // Performance changed, show msg box informing that change will take into affect after game restart
                const char* pMsgText = jonesString_GetString("JONES_STR_PERFORMANCE");
                if ( pMsgText )
                {
                    jonesConfig_ShowMessageDialog(hDlg, "JONES_STR_DPLY_OPTS", pMsgText, 136); // 136 - tv icon
                    InvalidateRect(hDlg, 0, 1);
                }
            }

            pSettings->filter = jonesConfig_advanceDisplaySettings_curFilterMode;
            //jonesConfig_advanceDisplaySettings_curFilterMode = STD3D_MIPMAPFILTER_TRILINEAR; // Fixed: Removed, as cached filtermode is now set in init function,
            //                                                                                           as the mipmap filter radio boxes might not be changed and then the wrong filter mode would be set here

            // Added
            // Get & Save HiPoly option
            int bCurBHiPoly = sithModel_IsHiPolyEnabled();

            int bHiPoly = IsDlgButtonChecked(hDlg, 1052);
            sithModel_EnableHiPoly(bHiPoly);
            stdConfig_SetBool(JONESDISPLAY_CFG_ENGINE_RENDER_HIPOLY, bHiPoly);

            if ( (bHiPoly != 0) != (bCurBHiPoly != 0) )
            {
                const char* pNoteStr = jonesString_GetString("JONES_STR_HIPOLYGO");
                if ( !pNoteStr )
                {
                    pNoteStr = "You must quit and restart the game for the high poly option to take effect.";
                }
                jonesConfig_ShowMessageDialog(hDlg, "JONES_STR_GMPLY_OPTS", pNoteStr, 136); // 136 - TV icon
            }

            // Added: Grab MSAA option state
            if ( std3D_IsMSAASupported() )
            {
                int bMSAA= IsDlgButtonChecked(hDlg, 1053);
                stdConfig_SetBool(STD3D_CFG_MSAAENABLED, !!bMSAA);
            }

            // Added: Grab mipmap auto gen option
            // Fixed: Gate mipmap auto-gen saving on mipmap auto-gen support, not MSAA support.
            if ( std3D_IsMipmapAutoGenSupported() )
            {
                int bAutoGen = IsDlgButtonChecked(hDlg, 1054);
                stdConfig_SetBool(STD3D_CFG_MIPMAPAUTOGEN, !!bAutoGen);
            }

            // Added: Grab Mipmap auto gen option
            if ( std3D_IsAnisotropicFilteringSupported() )
            {
                int bAniso = IsDlgButtonChecked(hDlg, 1055);
                stdConfig_SetBool(STD3D_CFG_ANISOTROPICFILTER, !!bAniso);
            }

            // Close dialog
            EndDialog(hDlg, ctrlID);
        } break;

        case 2: // Cancel btn
            EndDialog(hDlg, ctrlID);
            break;

        default:
            return;
    }
}

int J3DAPI jonesConfig_ShowSoundSettingsDialog(HWND hWnd, JonesSoundSettings* pData)
{
    GetWindowLongA(hWnd, GWL_HINSTANCE);
    return JonesDialog_ShowDialog(MAKEINTRESOURCE(113), hWnd, jonesConfig_SoundSettingsDialogProc, (LPARAM)pData);
}

INT_PTR CALLBACK jonesConfig_SoundSettingsDialogProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    int res = 1;
    if ( uMsg > WM_INITDIALOG )
    {
        if ( uMsg == WM_COMMAND )
        {
            jonesConfig_SoundSettings_HandleWM_COMMAND(hWnd, LOWORD(wParam), lParam);
            return 0;
        }

        if ( uMsg != WM_HSCROLL )
        {
            return 0;
        }

        jonesConfig_HandleWM_HSCROLL(hWnd, (HWND)lParam, LOWORD(wParam));
        if ( LOWORD(wParam) != SB_ENDSCROLL )
        {
            return res;
        }

        // Play test sound with new volume
        tSoundHandle hSnd = Sound_GetSoundHandle(SITHWORLD_STATICINDEX(57)); // 57 - mus_find.wav
        if ( jonesConfig_soundSettings_hSndChannel && (Sound_GetChannelFlags(jonesConfig_soundSettings_hSndChannel) & SOUND_CHANNEL_PLAYING) != 0 )
        {
            sithSoundMixer_StopSound(jonesConfig_soundSettings_hSndChannel);
        }

        jonesConfig_soundSettings_hSndChannel = 0;
        if ( !hSnd )
        {
            return res;
        }

        HWND hVolSliderCtrl = GetDlgItem(hWnd, 1050);
        int curPos = SendMessage(hVolSliderCtrl, TBM_GETPOS, 0, 0);
        if ( jonesConfig_soundSettings_curSoundVolume == curPos )
        {
            return res;
        }

        float volume = (float)curPos / 100.0f;
        jonesConfig_soundSettings_hSndChannel = sithSoundMixer_PlaySound(hSnd, volume, 0.0f, (SoundPlayFlag)0);
        jonesConfig_soundSettings_curSoundVolume = curPos;
        return res;
    }

    if ( uMsg == WM_INITDIALOG )
    {
        jonesConfig_hFontSoundSettings = jonesConfig_InitDialog(hWnd, 0, 113);
        res = jonesConfig_InitSoundSettingsDialog(hWnd, wParam, (JonesSoundSettings*)lParam);
        SetWindowPos(hWnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_SHOWWINDOW | SWP_NOMOVE | SWP_NOSIZE);
    }
    else
    {
        if ( uMsg != WM_DESTROY )
        {
            return 0;
        }

        jonesConfig_ResetDialogFont(hWnd, jonesConfig_hFontSoundSettings);
    }

    return res;
}

int J3DAPI jonesConfig_InitSoundSettingsDialog(HWND hDlg, int a2, JonesSoundSettings* pData)
{
    J3D_UNUSED(a2);

    HWND hVolSliderCtrl = GetDlgItem(hDlg, 1050);
    SendMessage(hVolSliderCtrl, TBM_SETRANGE, /*re-draw=*/TRUE, MAKELONG(0, 100));
    SendMessage(hVolSliderCtrl, TBM_SETTICFREQ, 17u, 0);
    SendMessage(hVolSliderCtrl, TBM_SETPOS, /*re-draw=*/TRUE, (LPARAM)(pData->maxSoundVolume * 100.0f));
    SendMessage(hVolSliderCtrl, TBM_SETPAGESIZE, 0, 5);

    // Set 3D sound check box
    HWND hBtn3DSound = GetDlgItem(hDlg, 1118);
    if ( Sound_Has3DHW() )
    {
        if ( pData->b3DHWSupport )
        {
            CheckDlgButton(hDlg, 1118, 1u);
        }
        else
        {
            CheckDlgButton(hDlg, 1118, 0);
        }
    }
    else
    {
        CheckDlgButton(hDlg, 1118, 0);
        EnableWindow(hBtn3DSound, 0);
    }

    // Set reverse sound check box
    GetDlgItem(hDlg, 1051);
    CheckDlgButton(hDlg, 1051, pData->bReverseSound);

    SetWindowLongPtr(hDlg, DWL_USER, (LONG_PTR)pData);
    return 1;
}

void J3DAPI jonesConfig_SoundSettings_HandleWM_COMMAND(HWND hWnd, int ctrlID, int notifyCode)
{
    J3D_UNUSED(notifyCode);

    JonesSoundSettings* pSettings = (JonesSoundSettings*)GetWindowLongPtr(hWnd, DWL_USER);

    if ( ctrlID == 1 ) // OK
    {
        HWND hVolSliderCtrl = GetDlgItem(hWnd, 1050);
        int sliderPos = SendMessage(hVolSliderCtrl, TBM_GETPOS, 0, 0);
        pSettings->maxSoundVolume = (float)sliderPos / 100.0f;
        stdConfig_SetFloat(JONESCONFIG_CFG_SOUND_VOLUME, pSettings->maxSoundVolume);

        HWND hBtn3DSound = GetDlgItem(hWnd, 1118);
        pSettings->b3DHWSupport = Button_GetState(hBtn3DSound);
        stdConfig_SetBool(JONESCONFIG_CFG_SOUND_HW, pSettings->b3DHWSupport);

        HWND hBtnReverseSound = GetDlgItem(hWnd, 1051);
        pSettings->bReverseSound  = Button_GetState(hBtnReverseSound);
        stdConfig_SetBool(JONESCONFIG_CFG_SOUND_REVERSE, pSettings->bReverseSound);

        EndDialog(hWnd, ctrlID);
    }
    else if ( ctrlID == 2 ) // Cancel btn
    {
        EndDialog(hWnd, ctrlID);
    }
}

int J3DAPI jonesConfig_ShowGameOverDialog(HWND hWnd, char* pRestoreFilename, tSoundHandle hSndGameOVerMus, tSoundChannelHandle* pSndChnlMus)
{
    GameOverDialogData data;
    data.result           = 0;
    data.pRestoreFilename = pRestoreFilename;
    data.sndChnlMusic     = 0;
    data.hSndMusic        = 0;
    data.hSndMusic        = hSndGameOVerMus;
    JonesDialog_ShowDialog(MAKEINTRESOURCE(150), hWnd, jonesConfig_GameOverDialogProc, (LPARAM)&data);

    if ( pSndChnlMus )
    {
        *pSndChnlMus = data.sndChnlMusic;
    }
    return data.result;
}

INT_PTR CALLBACK jonesConfig_GameOverDialogProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    int bSuccess;

    if ( uMsg == WM_DESTROY )
    {
        jonesConfig_ResetDialogFont(hWnd, jonesConfig_hFontGameOverDialog);
        return 1;
    }

    else if ( uMsg == WM_INITDIALOG )
    {
        jonesConfig_hFontGameOverDialog = jonesConfig_InitDialog(hWnd, 0, 150);
        bSuccess = jonesConfig_GameOverDialogInit(hWnd, wParam, (GameOverDialogData*)lParam);
        SetWindowPos(hWnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_SHOWWINDOW | SWP_NOMOVE | SWP_NOSIZE);
        return bSuccess;
    }
    else
    {
        if ( uMsg == WM_COMMAND )
        {
            jonesConfig_GameOverDialog_HandleWM_COMMAND(hWnd, LOWORD(wParam));
        }

        return 0;
    }
}

void J3DAPI jonesConfig_LoadGameGetLastSavedGamePath(char* pPath, unsigned int size)
{
    const char* pLastFilename;
    int filenameSize;
    const char* pCurText;
    const char* pFilePrefix;
    const char* pSaveGamesDir;
    const char* lpszSaveGamesDir;
    const char* pNdsLevenFilename;
    char* pFilePart;
    char aPath[128];
    char aLastFilePath[128];

    pLastFilename = sithGamesave_GetLastFilename();
    if ( pLastFilename && strlen(pLastFilename) )
    {
        filenameSize = strlen(pLastFilename);
        pCurText = &pLastFilename[filenameSize - 1];
        if ( filenameSize >= 0 )
        {
            while ( *pCurText != '\\' )
            {
                --pCurText;
                if ( --filenameSize < 0 )
                {
                    goto LABEL_8;
                }
            }

            ++pCurText;
        }

    LABEL_8:
        if ( pCurText )
        {
            memset(aLastFilePath, 0, sizeof(aLastFilePath));
            memset(aPath, 0, sizeof(aPath));
            STD_STRNCPY(aLastFilePath, pLastFilename, pCurText - pLastFilename);
            STD_STRNCPY(aPath, pCurText, strlen(pCurText));
            SearchPath(aLastFilePath, aPath, 0, 128u, pPath, &pFilePart);
        }
        else
        {
            lpszSaveGamesDir = sithGetSaveGamesDir();
            SearchPath(lpszSaveGamesDir, pLastFilename, 0, 128u, pPath, &pFilePart);
        }
    }

    if ( strlen(pPath) == 0 )
    {
        memset(aPath, 0, sizeof(aPath));
        memset(pPath, 0, size);
        if ( sithWorld_g_pCurrentWorld )
        {
            pNdsLevenFilename = sithGetCurrentWorldSaveName();
            pFilePrefix = sithGetAutoSaveFilePrefix();
            STD_FORMAT(aPath, "%s%s", pFilePrefix, pNdsLevenFilename);
            // Fixed: Use bounded extension replacement for the fixed-size path buffer.
            stdFnames_ChangeExtEx(aPath, STD_ARRAYLEN(aPath), "nds");
            pSaveGamesDir = sithGetSaveGamesDir();
            SearchPath(pSaveGamesDir, aPath, 0, 128u, pPath, &pFilePart);
        }
    }
}

int J3DAPI jonesConfig_GameOverDialogInit(HWND hDlg, int a2, GameOverDialogData* pData)
{
    J3D_UNUSED(a2);

    char aFilePath[128] = { 0 };
    if ( sithWorld_g_pCurrentWorld )
    {
        const char* pNdsFilename = sithGetCurrentWorldSaveName();
        const char* pFilePrefix = sithGetAutoSaveFilePrefix();

        char aFilename[128] = { 0 };
        STD_FORMAT(aFilename, "%s%s", pFilePrefix, pNdsFilename);
        // Fixed: Use bounded extension replacement for the fixed-size path buffer.
        stdFnames_ChangeExtEx(aFilename, STD_ARRAYLEN(aFilename), "nds");

        const char* pDirPath = sithGetSaveGamesDir();
        LPSTR pFilePart;
        SearchPath(pDirPath, aFilename, NULL, STD_ARRAYLEN(aFilePath), aFilePath, &pFilePart);
    }

    if ( strlen(aFilePath) == 0 )
    {
        // Disable restart btn
        HWND hBtn = GetDlgItem(hDlg, 1177); // Restart
        EnableWindow(hBtn, 0);
    }

    tSoundHandle hSnd = pData->hSndMusic;
    if ( hSnd )
    {
        pData->sndChnlMusic = sithSoundMixer_PlaySound(hSnd, 1.0f, 0.0f, (SoundPlayFlag)0);
    }

    stdUtil_StringCopy(pData->pRestoreFilename, JONESCONFIG_GAMESAVE_FILEPATHSIZE, aFilePath);
    SetWindowLongPtr(hDlg, DWL_USER, (LONG_PTR)pData);
    return 1;
}

void J3DAPI jonesConfig_GameOverDialog_HandleWM_COMMAND(HWND hWnd, uint16_t ctrlID)
{
    GameOverDialogData* pData = (GameOverDialogData*)GetWindowLongPtr(hWnd, DWL_USER);
    switch ( ctrlID )
    {
        case 1177u:
            pData->result = 1177; // restart
            EndDialog(hWnd, 1);
            break;

        case 1178u:
            pData->result = 1178; // load game
            if ( jonesConfig_GetLoadGameFilePath(hWnd, pData->pRestoreFilename) == 1 )
            {
                InvalidateRect(hWnd, 0, 1);
                EndDialog(hWnd, 1);
            }

            break;

        case 1179u:
            pData->result = 1179;
            EndDialog(hWnd, 1);
            break;
    }
}

int J3DAPI jonesConfig_ShowStatisticsDialog(HWND hWnd, SithGameStatistics* pStatistics)
{
    int dwInitParam[3];
    JonesDialogImageInfo imageInfo;

    if ( !pStatistics )
    {
        return 2;
    }

    memset(&imageInfo, 0, sizeof(imageInfo));
    dwInitParam[1] = (int)pStatistics;
    dwInitParam[0] = (int)&imageInfo;
    return JonesDialog_ShowDialog(MAKEINTRESOURCE(164), hWnd, jonesConfig_StatisticsDialogProc, (LPARAM)dwInitParam);
}

INT_PTR CALLBACK jonesConfig_StatisticsDialogProc(HWND hwnd, UINT uMsg, WPARAM wPAram, LPARAM lParam)
{
    INT_PTR result;
    HWND textStatistict;
    const char* pTitleText;
    int inited;
    void* pData_1;
    HDC* v9;
    HBRUSH SolidBrush;
    int* pData;
    HWND hScroll;
    int scrollPos;
    int levelNum;
    int MaxPos;
    int MinPos;
    struct tagRECT Rect;
    DRAWITEMSTRUCT drawitemData;
    const char* pSummeryText;
    HWND hTextStatTitle;
    WPARAM iqpoints;

    if ( uMsg > WM_CLOSE )
    {
        switch ( uMsg )
        {
            case WM_DRAWITEM:
                pData_1 = (void*)GetWindowLongPtr(hwnd, DWL_USER);
                v9 = *(HDC**)pData_1;
                if ( !*(uint32_t*)(*(uint32_t*)pData_1 + 16) )
                {
                    v9[4] = (HDC)GetBkColor(*(HDC*)(lParam + 24));
                }

                GetClientRect(*(HWND*)(lParam + 20), &Rect);
                SolidBrush = CreateSolidBrush((COLORREF)v9[4]);
                FillRect(*(HDC*)(lParam + 24), &Rect, SolidBrush);
                SetStretchBltMode(*(HDC*)(lParam + 24), STRETCH_HALFTONE);
                jonesConfig_DrawImageOnDialogItem(
                    hwnd,
                    *v9,
                    *(HDC*)(lParam + 24),
                    wPAram,
                    jonesConfig_apDialogIcons[0],// iq.bmp
                    jonesConfig_apDialogIcons[1]);// iq_mask.bmp
                result = 1;
                break;

            case WM_INITDIALOG:
                textStatistict = GetDlgItem(hwnd, 1158);
                jonesConfig_hFontGameStatisticsDialog = jonesConfig_InitDialog(hwnd, 0, 164);
                pTitleText = jonesString_GetString("JONES_STR_SUMMARY");
                if ( pTitleText )
                {
                    SetWindowText(textStatistict, pTitleText);
                }

                inited = jonesConfig_InitStatisticDialog(hwnd, wPAram, (int*)lParam);
                SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_SHOWWINDOW | SWP_NOMOVE | SWP_NOSIZE);// TODO: 2nd is HWND_TOPMOST
                result = inited;
                break;

            case WM_COMMAND:
                jonesConfig_StatisticProc_HandleWM_COMMAND(hwnd, LOWORD(wPAram));
                result = 0;
                break;

            case WM_HSCROLL:
                pData = (int*)GetWindowLongPtr(hwnd, DWL_USER);
                hScroll = GetDlgItem(hwnd, 1162);
                hTextStatTitle = GetDlgItem(hwnd, 1158);
                jonesConfig_sub_40D100((int)hwnd, (HWND)lParam, (uint16_t)wPAram, HIWORD(wPAram));
                scrollPos = GetScrollPos(hScroll, 2);
                if ( scrollPos != pData[2] )
                {
                    GetScrollRange(hScroll, SB_CTL, &MinPos, &MaxPos);
                    if ( scrollPos == MaxPos )
                    {
                        pSummeryText = jonesString_GetString("JONES_STR_SUMMARY");// "JONES_STR_SUMMARY"
                        levelNum = 17;
                        iqpoints = jonesInventory_GetTotalIQPoints();
                        pTitleText = pSummeryText;
                    }
                    else
                    {
                        if ( scrollPos == jonesConfig_gameStatistics_curLevelNum && jonesConfig_gameStatistics_curLevelNum > 0 )
                        {
                            pTitleText = jonesString_GetString(jonesConfig_gameStatistics__aLevelNames[JONESLEVEL_LASTLEVELNUM]);
                        }
                        else
                        {
                            pTitleText = jonesString_GetString(jonesConfig_gameStatistics__aLevelNames[scrollPos]);
                        }

                        levelNum = scrollPos;
                        iqpoints = *(uint32_t*)(32 * scrollPos + pData[1] + 44);// pData[1] -> SithGameStatistics*
                    }

                    if ( pTitleText )
                    {
                        SetWindowText(hTextStatTitle, pTitleText);
                    }

                    else if ( scrollPos == jonesConfig_gameStatistics_curLevelNum && jonesConfig_gameStatistics_curLevelNum > 0 )
                    {
                        SetWindowText(hTextStatTitle, jonesConfig_gameStatistics__aLevelNames[JONESLEVEL_LASTLEVELNUM]);
                    }
                    else
                    {
                        SetWindowText(hTextStatTitle, jonesConfig_gameStatistics__aLevelNames[levelNum]);
                    }

                    jonesConfig_SetStatisticsDialogForLevel(hwnd, levelNum, pData);
                    jonesConfig_DrawStatisticDialogIQPoints(hwnd, (JonesDialogImageInfo**)pData, 164, iqpoints);

                    drawitemData.CtlType = ODT_BUTTON;
                    drawitemData.CtlID = 1220;
                    drawitemData.itemID = 0;
                    drawitemData.itemAction = 1;
                    drawitemData.itemState = 0;
                    drawitemData.hwndItem = GetDlgItem(hwnd, 1220);
                    drawitemData.hDC = GetDC(drawitemData.hwndItem);

                    GetClientRect(drawitemData.hwndItem, &drawitemData.rcItem);

                    drawitemData.itemData = 0;

                    SendMessage(hwnd, WM_DRAWITEM, 1220u, (LPARAM)&drawitemData);
                    ReleaseDC(drawitemData.hwndItem, drawitemData.hDC);
                    pData[2] = scrollPos;
                }

                result = 1;
                break;

            default:
                return 0;
        }
    }

    else if ( uMsg == WM_CLOSE )
    {
        EndDialog(hwnd, 2);
        return 0;
    }

    else if ( uMsg == WM_DESTROY )
    {
        jonesConfig_ResetDialogFont(hwnd, (HFONT)jonesConfig_hFontGameStatisticsDialog);
        return 1;
    }
    else
    {
        return 0;
    }

    return result;
}

int J3DAPI jonesConfig_sub_40D100(int a1, HWND hWnd, int a3, int a4)
{
    J3D_UNUSED(a1);

    int result;
    int v5;
    int v6;
    int v7;
    int v8;
    LPARAM lParam[7];
    int v10;
    LPARAM v11;
    int v12;
    LPARAM v13;
    int nPos;

    nPos = -1;
    lParam[1] = 23;
    SendMessage(hWnd, 0xEAu, 0, (LPARAM)lParam);
    v10 = lParam[2];
    v12 = lParam[3];
    v13 = lParam[5];
    v11 = lParam[4];
    result = a3;
    switch ( a3 )
    {
        case 0:
            if ( v10 <= v13 - 1 )
            {
                v8 = v13 - 1;
            }
            else
            {
                v8 = v10;
            }

            result = v8;
            nPos = v8;
            break;

        case 1:
            if ( v12 >= v13 + 1 )
            {
                result = v13 + 1;
                v7 = v13 + 1;
            }
            else
            {
                v7 = v12;
            }

            nPos = v7;
            break;

        case 2:
            if ( v10 <= v13 - v11 )
            {
                v6 = v13 - v11;
            }
            else
            {
                result = v10;
                v6 = v10;
            }

            nPos = v6;
            break;

        case 3:
            if ( v12 >= v11 + v13 )
            {
                v5 = v11 + v13;
            }
            else
            {
                v5 = v12;
            }

            result = v5;
            nPos = v5;
            break;

        case 4:
        case 5:
            nPos = a4;
            break;

        case 6:
            nPos = v10;
            break;

        case 7:
            nPos = v12;
            break;

        default:
            break;
    }

    if ( nPos >= 0 )
    {
        return SetScrollPos(hWnd, 2, nPos, 1);
    }

    return result;
}

void jonesConfig_UpdateCurrentLevelNum(void)
{
    jonesConfig_gameStatistics_curLevelNum = JonesMain_GetCurrentLevelNum();
}

int J3DAPI jonesConfig_DrawImageOnDialogItem(HWND hDlg, HDC hdcWnd, HDC hdcCtrl, int nIDDlgItem, HBITMAP hImage, HBITMAP hMask)
{
    RECT rect;
    HWND hDlgItem = GetDlgItem(hDlg, nIDDlgItem);
    GetClientRect(hDlgItem, &rect);

    BITMAP bmp;
    GetObject(hImage, sizeof(BITMAP), &bmp);

    if ( hMask )
    {
        SelectObject(hdcWnd, (HGDIOBJ)hMask);
        StretchBlt(hdcCtrl, 0, 0, rect.right - rect.left, rect.bottom - rect.top, hdcWnd, 0, 0, bmp.bmWidth, bmp.bmHeight, SRCAND);

        SelectObject(hdcWnd, (HGDIOBJ)hImage);
        return StretchBlt(hdcCtrl, 0, 0, rect.right - rect.left, rect.bottom - rect.top, hdcWnd, 0, 0, bmp.bmWidth, bmp.bmHeight, SRCPAINT);
    }
    else
    {
        SelectObject(hdcWnd, (HGDIOBJ)hImage);
        return StretchBlt(hdcCtrl, 0, 0, rect.right - rect.left, rect.bottom - rect.top, hdcWnd, 0, 0, bmp.bmWidth, bmp.bmHeight, SRCCOPY);
    }
}

int J3DAPI jonesConfig_SetStatisticsDialogForLevel(HWND hDlg, int levelNum, int* a3)
{
    int v3;
    int v4;
    HWND hTextTimePassed;
    HWND hTextLvlFinished;
    HWND hTextTreasuresFound;
    HWND hTextHintsSeen;
    HWND hTextDifficultyDrop;
    SithLevelStatistic* pLevelStatistics;
    SithGameStatistics* pStatistics;
    char aText[256];

    pStatistics = (SithGameStatistics*)a3[1];
    pLevelStatistics = &pStatistics->aLevelStatistic[levelNum];
    v3 = pLevelStatistics->elapsedTime >> 8;
    v4 = pLevelStatistics->elapsedTime ^ (v3 << 8);
    memset(aText, 0, sizeof(aText));
    if ( v4 >= 10 )
    {
        STD_FORMAT(aText, "%i : %i ", v3, v4);
    }
    else
    {
        STD_FORMAT(aText, "%i : 0%i ", v3, v4);
    }

    hTextTimePassed = GetDlgItem(hDlg, 1153);
    SetWindowText(hTextTimePassed, aText);

    memset(aText, 0, sizeof(aText));
    STD_FORMAT(aText, "%i ", pStatistics->curLevelNum);
    hTextLvlFinished = GetDlgItem(hDlg, 1152);
    SetWindowText(hTextLvlFinished, aText);

    memset(aText, 0, sizeof(aText));
    STD_FORMAT(aText, "%i ", pStatistics->aLevelStatistic[levelNum].numFoundTreasures);
    hTextTreasuresFound = GetDlgItem(hDlg, 1154);
    SetWindowText(hTextTreasuresFound, aText);

    memset(aText, 0, sizeof(aText));
    STD_FORMAT(aText, "%i ", pStatistics->aLevelStatistic[levelNum].numSeenHints);
    hTextHintsSeen = GetDlgItem(hDlg, 1155);
    SetWindowText(hTextHintsSeen, aText);

    memset(aText, 0, sizeof(aText));
    STD_FORMAT(aText, "%i ", pStatistics->aLevelStatistic[levelNum].difficultyPenalty);
    hTextDifficultyDrop = GetDlgItem(hDlg, 1156);
    return SetWindowText(hTextDifficultyDrop, aText);
}

void J3DAPI jonesConfig_DrawStatisticDialogIQPoints(HWND hwnd, JonesDialogImageInfo** ppImageInfo, int dlgID, int iqpoints)
{
    J3D_UNUSED(hwnd);
    J3D_UNUSED(dlgID);

    JonesDialogImageInfo* pImageInfo = *ppImageInfo;
    if ( pImageInfo->hBmp )
    {
        DeleteObject((HGDIOBJ)pImageInfo->hBmp);
        pImageInfo->hBmp = 0;
    }

    SelectObject(pImageInfo->hdcFront, (HGDIOBJ)jonesConfig_apDialogIcons[0]);
    BitBlt(pImageInfo->hdcFront, 39, 207, 127, 46, NULL, 0, 0, BLACKNESS);

    // TODO: add iqoverlay.bmp (icons[2])

    SelectObject(pImageInfo->hdcFront, (HGDIOBJ)jonesConfig_apDialogIcons[1]);
    BitBlt(pImageInfo->hdcFront, 39, 207, 127, 46, NULL, 0, 0, WHITENESS);

    if ( iqpoints < 0 )
    {
        iqpoints = 0;
    }

    int points = iqpoints;

    // Calculate required width/height
    int width = 0, height = 0;
    while ( iqpoints >= 0 )
    {
        int digit = iqpoints % 10;
        iqpoints /= 10;
        if ( !iqpoints )
        {
            iqpoints = -1;
        }

        width += jonesConfig_aNumberGlyphMetrics[digit].right - jonesConfig_aNumberGlyphMetrics[digit].left + 1;
        if ( jonesConfig_aNumberGlyphMetrics[digit].bottom - jonesConfig_aNumberGlyphMetrics[digit].top + 1 > height )
        {
            height = jonesConfig_aNumberGlyphMetrics[digit].bottom - jonesConfig_aNumberGlyphMetrics[digit].top + 1;
        }
    }

    BITMAPINFO bmi              = { 0 };
    bmi.bmiHeader.biSize        = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth       = width;
    bmi.bmiHeader.biHeight      = -height;
    bmi.bmiHeader.biPlanes      = 1;
    bmi.bmiHeader.biBitCount    = 24;
    bmi.bmiHeader.biCompression = BI_RGB;
    bmi.bmiHeader.biSizeImage   = 3 * height * width;

    void* ppvBits;
    HBITMAP hBmp = CreateDIBSection(pImageInfo->hdcBack, &bmi, DIB_RGB_COLORS, &ppvBits, NULL, 0);
    pImageInfo->hBmp = hBmp;

    if ( pImageInfo->hBmp )
    { // Fixed: Added no null check
        SelectObject(pImageInfo->hdcBack, pImageInfo->hBmp);
    }
    SelectObject(pImageInfo->hdcFront, jonesConfig_apDialogIcons[3]); // 3 - numbers bmp

    RECT rc = { 0 };
    rc.bottom = height;
    rc.right  = width;

    HBRUSH hBrush = CreateSolidBrush(RGB(255, 255, 255));
    FillRect(pImageInfo->hdcBack, &rc, hBrush);

    int x = width;
    while ( points >= 0 )
    {
        int digit = points % 10;
        points /= 10;
        if ( !points )
        {
            points = -1;
        }

        x -= jonesConfig_aNumberGlyphMetrics[digit].right - jonesConfig_aNumberGlyphMetrics[digit].left + 1;
        BitBlt(
            pImageInfo->hdcBack,
            x,
            0,
            jonesConfig_aNumberGlyphMetrics[digit].right - jonesConfig_aNumberGlyphMetrics[digit].left + 1,
            jonesConfig_aNumberGlyphMetrics[digit].bottom - jonesConfig_aNumberGlyphMetrics[digit].top + 1,
            pImageInfo->hdcFront,
            jonesConfig_aNumberGlyphMetrics[digit].left,
            jonesConfig_aNumberGlyphMetrics[digit].top,
            SRCCOPY
        );
    }

    x = (127 - width) / 2 + 39;
    int y = (46 - height) / 2 + 207;

    // Draw IQ bmp
    SelectObject(pImageInfo->hdcFront, (HGDIOBJ)jonesConfig_apDialogIcons[0]);
    BitBlt(pImageInfo->hdcFront, x, y, width, height, pImageInfo->hdcBack, 0, 0, SRCAND);

    // Draw IQ mask bmp
    SelectObject(pImageInfo->hdcFront, (HGDIOBJ)jonesConfig_apDialogIcons[1]);
    BitBlt(pImageInfo->hdcFront, x, y, width, height, pImageInfo->hdcBack, 0, 0, SRCCOPY);
}

int J3DAPI jonesConfig_InitStatisticDialog(HWND hDlg, int a2, int* pData)
{
    J3D_UNUSED(a2);
    HDC* v4;
    HWND DlgItem;
    HDC DC;
    HWND v7;
    HDC v8;
    int iqPoints;
    HWND v10;
    HWND hScrl;
    SCROLLINFO scrlInfo;
    SithGameStatistics* pDataa;

    v4 = (HDC*)*pData;
    pDataa = (SithGameStatistics*)pData[1];
    DlgItem = GetDlgItem(hDlg, 1135);
    DC = GetDC(DlgItem);
    *v4 = CreateCompatibleDC(DC);
    v7 = GetDlgItem(hDlg, 1135);
    v8 = GetDC(v7);
    v4[1] = CreateCompatibleDC(v8);
    iqPoints = jonesInventory_GetTotalIQPoints();
    jonesConfig_DrawStatisticDialogIQPoints(hDlg, (JonesDialogImageInfo**)pData, 164, iqPoints);
    if ( !pDataa->curLevelNum )
    {
        v10 = GetDlgItem(hDlg, 1133);
        EnableWindow(v10, 0);
    }

    jonesConfig_SetStatisticsDialogForLevel(hDlg, JONESLEVEL_BONUSLEVELNUM, pData);

    scrlInfo.cbSize = sizeof(SCROLLINFO);
    scrlInfo.fMask = SIF_ALL | SIF_DISABLENOSCROLL;
    scrlInfo.nMin = 0;
    scrlInfo.nMax = pDataa->curLevelNum + 1;
    scrlInfo.nPage = 1;
    scrlInfo.nPos = scrlInfo.nMax;
    hScrl = GetDlgItem(hDlg, 1162);
    SendMessage(hScrl, SBM_SETSCROLLINFO, TRUE, (LPARAM)&scrlInfo);

    pData[2] = scrlInfo.nMax;
    SetWindowLongPtr(hDlg, DWL_USER, (LONG_PTR)pData);
    return 1;
}

void J3DAPI jonesConfig_StatisticProc_HandleWM_COMMAND(HWND hWnd, int16_t wParam)
{
    int v2;
    HDC v3;

    v2 = *(uint32_t*)GetWindowLongPtr(hWnd, DWL_USER);
    if ( wParam > 0 && wParam <= 2 )
    {
        DeleteObject(*(HGDIOBJ*)(v2 + 8));
        v3 = *(HDC*)v2;
        *(uint32_t*)(v2 + 8) = 0;
        DeleteDC(v3);
        DeleteDC(*(HDC*)(v2 + 4));
        EndDialog(hWnd, wParam);
    }
}

int J3DAPI jonesConfig_ShowExitGameDialog(HWND hWnd, char* pSaveGameFilePath)
{
    GameSaveMsgBoxData data;
    data.dialogID = 211;
    data.pNdsFilePath = pSaveGameFilePath;
    return JonesDialog_ShowDialog(MAKEINTRESOURCE(211), hWnd, jonesConfig_ExitGameDlgProc, (LPARAM)&data);
}

INT_PTR CALLBACK jonesConfig_ExitGameDlgProc(HWND hWnd, UINT uMsg, WPARAM wparam, LPARAM lparam)
{
    switch ( uMsg )
    {
        case WM_INITDIALOG:
        {
            RECT rectWnd;
            GetClientRect(hWnd, &rectWnd);
            jonesConfig_GetWindowScreenRect(hWnd, &rectWnd);

            RECT rectWndIcon;
            HWND hIcon = GetDlgItem(hWnd, 1182);

            // Added: Make hPrevIcon to be drawn by hWnd -> WM_DRAWITEM
            SetWindowLongPtr(hIcon, GWL_STYLE, GetWindowLongPtr(hIcon, GWL_STYLE) | SS_OWNERDRAW);

            GetWindowRect(hIcon, &rectWndIcon);
            MoveWindow(hIcon, rectWndIcon.top - rectWnd.top, rectWndIcon.left - rectWnd.left, 96, 96, TRUE); // Changed: Shange icon size to 96 from 64

            GameSaveMsgBoxData* pGSData = (GameSaveMsgBoxData*)lparam;
            jonesConfig_hFontExitDlg = jonesConfig_InitDialog(hWnd, 0, pGSData->dialogID);

            int result = jonesConfig_GameSaveSetData(hWnd, wparam, pGSData);
            SetWindowPos(hWnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_SHOWWINDOW | SWP_NOMOVE | SWP_NOSIZE);
            return result;
        }
        case WM_COMMAND:
            jonesConfig_MsgBoxDlg_HandleWM_COMMAND(hWnd, (uint16_t)wparam);
            return 0;
        case WM_DESTROY:
            jonesConfig_ResetDialogFont(hWnd, jonesConfig_hFontExitDlg);
            return 1;
        case WM_DRAWITEM: // Added: Fixes icon drawing
        {
            LPDRAWITEMSTRUCT pdis = (LPDRAWITEMSTRUCT)lparam;
            if ( pdis->CtlID == 1182 )
            {
                HDC hmemdc = CreateCompatibleDC(pdis->hDC);
                SetStretchBltMode(pdis->hDC, STRETCH_HALFTONE);
                jonesConfig_DrawImageOnDialogItem(hWnd, hmemdc, pdis->hDC, 1182, jonesConfig_apDialogIcons[4], jonesConfig_apDialogIcons[5]); // exit and exitmask bmps
                DeleteDC(hmemdc);
                return TRUE;
            }
            return FALSE;
        }

        default:
            return 0;
    }
}

int J3DAPI jonesConfig_ShowLevelCompletedDialog(HWND hWnd, int* pBalance, int* apItemsState, int a4, int elapsedTime, int qiPoints, int numFoundTrasures, int foundTrasureValue, int totalTreasureValue)
{
    tLevelCompletedDialogState state = { 0 };
    state.balance            = pBalance;
    state.apItemsState       = apItemsState;
    state.unknown2           = a4;
    state.elapsedTime        = elapsedTime;
    state.iqPoints           = qiPoints;
    state.numFoundTrasures   = numFoundTrasures;
    state.foundTreasureValue = foundTrasureValue;
    state.totalTreasureValue = totalTreasureValue;
    state.pStatistics        = sithGamesave_GetGameStatistics();

    if ( !state.pStatistics || !sithGamesave_LockGameStatistics() )
    {
        return jonesConfig_ShowStoreDialog(hWnd, pBalance, apItemsState, a4);
    }

    JonesDialogImageInfo imageInfo = { 0 };
    state.pIQImageInfo = &imageInfo;

    int result = JonesDialog_ShowDialog(MAKEINTRESOURCE(233), hWnd, jonesConfig_LevelCompletedDialogProc, (LPARAM)&state);
    *pBalance  = *state.balance;
    sithGamesave_UnlockGameStatistics();
    return result;
}

int CALLBACK jonesConfig_LevelCompletedDialogProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch ( uMsg )
    {
        case WM_INITDIALOG:
        {
            jonesConfig_hFontLevelCopletedDialog = jonesConfig_InitDialog(hWnd, 0, 233);
            int result  = jonesConfig_InitLevelCompletedDialog(hWnd, wParam, (tLevelCompletedDialogState*)lParam);
            SetWindowPos(hWnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_SHOWWINDOW | SWP_NOMOVE | SWP_NOSIZE);
            return result;
        }
        case WM_COMMAND:
            jonesConfig_ChapterCompleteDialog_HandleWM_COMMAND(hWnd, (uint16_t)wParam);
            return 0;

        case WM_DESTROY:
            jonesConfig_ResetDialogFont(hWnd, jonesConfig_hFontLevelCopletedDialog);
            return 1;

        // Removed: The IQ bmp and qi points are drawn in WM_DRAWITEM; fixes drawing the bitmaps
        //case WM_PAINT:
        //{
        //    tLevelCompletedDialogState* pState = (tLevelCompletedDialogState*)GetWindowLongPtr(hWnd, DWL_USER);
        //    HWND hImgCtrl = GetDlgItem(hWnd, 1135);
        //    PAINTSTRUCT paint = { 0 }; // Added: Init to 0
        //    HDC hdcImg = BeginPaint(hWnd, (LPPAINTSTRUCT)&paint);
        //
        //    SetStretchBltMode(hdcImg, STRETCH_HALFTONE);
        //    jonesConfig_DrawImageOnDialogItem(hWnd, pState->pIQImageInfo->hdcFront, hdcImg, 1135, jonesConfig_apDialogIcons[0], jonesConfig_apDialogIcons[1]);//   "iq.bmp" "iqMask.bmp"
        //
        //    EndPaint(hWnd, &paint);
        //    return 0;
        //}

        // Added: Fixes drawing of IQ bmp and iq points. Note, jonesConfig_InitLevelCompletedDialog sets picture control 1135 to be drawn by dialog
        case WM_DRAWITEM:
        {
            LPDRAWITEMSTRUCT pdis = (LPDRAWITEMSTRUCT)lParam;
            if ( pdis->CtlID == 1135 )
            {
                tLevelCompletedDialogState* pState = (tLevelCompletedDialogState*)GetWindowLongPtr(hWnd, DWL_USER);
                SetStretchBltMode(pdis->hDC, STRETCH_HALFTONE);
                jonesConfig_DrawImageOnDialogItem(hWnd, pState->pIQImageInfo->hdcFront, pdis->hDC, 1135, jonesConfig_apDialogIcons[0], jonesConfig_apDialogIcons[1]); //  "iq.bmp" "iqMask.bmp"
                return TRUE;
            }
            return FALSE;
        }

        default:
            return 0;
    };
}

int J3DAPI jonesConfig_InitLevelCompletedDialog(HWND hDlg, int wParam, tLevelCompletedDialogState* pState)
{
    J3D_UNUSED(wParam);

    HWND hImgCtrl = GetDlgItem(hDlg, 1135);

    // Added: Make hImgCtrl to be drawn by hDlg -> WM_DRAWITEM
    LONG style = GetWindowLongPtr(hImgCtrl, GWL_STYLE);
    SetWindowLongPtr(hImgCtrl, GWL_STYLE, style | SS_OWNERDRAW);

    pState->pIQImageInfo->hdcFront = CreateCompatibleDC(GetDC(hImgCtrl));

    HWND hIQImageCtrl = GetDlgItem(hDlg, 1135); // ?? Why again retrieving of image ctrl
    pState->pIQImageInfo->hdcBack = CreateCompatibleDC(GetDC(hIQImageCtrl));

    jonesConfig_prevLevelNum = pState->pStatistics->curLevelNum - 1;
    jonesConfig_DrawStatisticDialogIQPoints(hDlg, &pState->pIQImageInfo, 233, pState->pStatistics->aLevelStatistic[jonesConfig_prevLevelNum].iqPoints);

    // Format game elapsed time
    int hours = pState->elapsedTime >> 8;
    int minutes = (hours << 8) ^ pState->elapsedTime;

    char aText[256] = { 0 };
    if ( minutes >= 10 )
    {
        STD_FORMAT(aText, "%i : %i ", hours, minutes);
    }
    else
    {
        STD_FORMAT(aText, "%i : 0%i ", hours, minutes);
    }

    SetWindowText(GetDlgItem(hDlg, 1153), aText);

    memset(aText, 0, sizeof(aText));
    STD_FORMAT(aText, "%i ", pState->iqPoints);
    SetWindowText(GetDlgItem(hDlg, 1154), aText);

    memset(aText, 0, sizeof(aText));
    STD_FORMAT(aText, "%i ", pState->numFoundTrasures);
    SetWindowText(GetDlgItem(hDlg, 1155), aText);

    memset(aText, 0, sizeof(aText));
    STD_FORMAT(aText, "%i ", pState->foundTreasureValue);
    SetWindowText(GetDlgItem(hDlg, 1157), aText);

    memset(aText, 0, sizeof(aText));
    STD_FORMAT(aText, "%i ", pState->totalTreasureValue);
    SetWindowText(GetDlgItem(hDlg, 1156), aText);

    SetWindowLongPtr(hDlg, DWL_USER, (LONG_PTR)pState);
    return 1;
}

void J3DAPI jonesConfig_ChapterCompleteDialog_HandleWM_COMMAND(HWND hWnd, int wParam)
{
    tLevelCompletedDialogState* pState = (tLevelCompletedDialogState*)GetWindowLongPtr(hWnd, DWL_USER);
    if ( wParam > 0 && wParam <= 2 ) // 1 or 2 - done
    {
        DeleteObject((HGDIOBJ)pState->pIQImageInfo->hBmp);
        pState->pIQImageInfo->hBmp = NULL;

        DeleteDC(pState->pIQImageInfo->hdcFront);
        DeleteDC(pState->pIQImageInfo->hdcBack);

        // Show store dialog if not bonus level
        if ( pState->pStatistics->curLevelNum + 1 < JONESLEVEL_BONUSLEVELNUM )
        {
            jonesConfig_ShowStoreDialog(hWnd, pState->balance, pState->apItemsState, pState->unknown2);
        }

        InvalidateRect(hWnd, NULL, TRUE);
        EndDialog(hWnd, wParam);
    }
}

int J3DAPI jonesConfig_ShowStoreDialog(HWND hWnd, int* pBalance, int* pItemsState, int a4)
{
    tStoreCartState cart = { 0 };
    cart.balance         = *pBalance;
    cart.apItemsState    = pItemsState;
    cart.unknown20       = a4;

    int result = JonesDialog_ShowDialog(MAKEINTRESOURCE(190), hWnd, jonesConfig_StoreDialogProc, (LPARAM)&cart);
    *pBalance  = cart.balance;
    return result;
}

int CALLBACK jonesConfig_StoreDialogProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    switch ( msg )
    {
        case WM_INITDIALOG:
            jonesConfig_hFontStoreDialog = jonesConfig_InitDialog(hwnd, 0, 190);
            int result = jonesConfig_InitStoreDialog(hwnd, wparam, (tStoreCartState*)lparam);
            SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_SHOWWINDOW | SWP_NOMOVE | SWP_NOSIZE);
            return result;

        case WM_COMMAND:
            jonesConfig_StoreDialog_HandleWM_COMMAND(hwnd, (uint16_t)wparam);
            return 0;

        case WM_MOUSEFIRST:
            jonesConfig_StoreDialog_HandleWM_MOUSEFIRST(hwnd);
            return 1;

        case WM_LBUTTONUP:
        case WM_RBUTTONUP:
        case WM_MBUTTONUP:
            jonesConfig_StoreDialog_HandleWM_MBUTTONUP(hwnd);
            return 1;

        case WM_NOTIFY:
        {
            tStoreCartState* pCart = (tStoreCartState*)GetWindowLongPtr(hwnd, DWL_USER);
            switch ( ((LPNMHDR)lparam)->code )
            {
                case LVN_BEGINDRAG:
                case LVN_BEGINRDRAG:
                {
                    // Note, lparam is NMLISTVIEW struct here
                    pCart->dragListId = ((LPNMHDR)lparam)->idFrom;
                    jonesConfig_StoreHandleDragEvent(hwnd, GetDlgItem(hwnd, pCart->dragListId));
                } break;
                case NM_DBLCLK:
                {
                    // If double mouse button click, add or remove item to cart based on which list item was clicked
                    if ( ((LPNMHDR)lparam)->idFrom == 1005 ) // available list editor
                    {
                        jonesConfig_AddStoreCartItem(hwnd, pCart);
                    }
                    else if ( ((LPNMHDR)lparam)->idFrom == 1006 ) // cart items list editor
                    {
                        jonesConfig_RemoveStoreCartItem(hwnd, pCart);
                    }
                }
            };
        } return 1;

        case WM_DESTROY:
            jonesConfig_ResetDialogFont(hwnd, jonesConfig_hFontStoreDialog);
            return 1;

        default:
            return 0;
    }
}

int J3DAPI jonesConfig_StoreHandleDragEvent(HWND hwnd, HWND hDlgCtrl)
{
    tStoreCartState* pCart = (tStoreCartState*)GetWindowLongPtr(hwnd, DWL_USER);
    UINT numSelected = ListView_GetSelectedCount(hDlgCtrl);

    SetCapture(hwnd);

    POINT cursorPos;
    GetCursorPos(&cursorPos);

    RECT dlgRect, wndRect;
    GetWindowRect(hDlgCtrl, &dlgRect);
    GetWindowRect(hwnd, &wndRect);

    int itemIdx   = -1;
    RECT itemRect = { 0 }; // Added: init to 0
    for ( UINT i = 0; i < numSelected; ++i )
    {
        itemIdx = ListView_GetNextItem(hDlgCtrl, itemIdx, LVNI_SELECTED);
        ListView_GetItemRect(hDlgCtrl, itemIdx, &itemRect, LVIR_SELECTBOUNDS);

        LONG dragX = cursorPos.x - (dlgRect.left + itemRect.left);
        LONG dragY = cursorPos.y - (dlgRect.top + itemRect.top);

        LVITEMA item;
        item.mask     = LVIF_PARAM;
        item.iSubItem = 0;
        item.iItem    = itemIdx;
        ListView_GetItem(hDlgCtrl, &item);

        int imageIdx = HIWORD(item.lParam);

        HIMAGELIST hList = ListView_GetImageList(hDlgCtrl, LVSIL_SMALL);
        if ( !ImageList_BeginDrag(hList, imageIdx, dragX, dragY) )
        {
            ImageList_EndDrag();
            ImageList_DragLeave(hwnd);
            int result = ReleaseCapture();
            pCart->bDragging = 0;
            return result;
        }
    }

    int result = ImageList_DragEnter(hwnd, cursorPos.x - wndRect.left, cursorPos.y - wndRect.top);
    pCart->bDragging = 1;
    return result;
}

void J3DAPI jonesConfig_StoreDialog_HandleWM_MBUTTONUP(HWND hWnd)
{
    tStoreCartState* pCart = (tStoreCartState*)GetWindowLongPtr(hWnd, DWL_USER);
    if ( pCart->bDragging )
    {
        ImageList_EndDrag();
        ImageList_DragLeave(hWnd);

        POINT cursorPos;
        GetCursorPos(&cursorPos);

        RECT rect;
        HWND dlgItem = GetDlgItem(hWnd, 1006);
        GetWindowRect(dlgItem, &rect);

        if ( PtInRect(&rect, cursorPos) && pCart->dragListId == 1005 )// available items list
        {
            jonesConfig_AddStoreCartItem(hWnd, pCart);
        }
        else
        {
            HWND hItemsDialog = GetDlgItem(hWnd, 1005);
            GetWindowRect(hItemsDialog, &rect);
            if ( PtInRect(&rect, cursorPos) )
            {
                if ( pCart->dragListId == 1006 )// cart list
                {
                    jonesConfig_RemoveStoreCartItem(hWnd, pCart);
                }
            }
        }

        pCart->bDragging = 0;
        ReleaseCapture();
    }
}

BOOL J3DAPI jonesConfig_StoreDialog_HandleWM_MOUSEFIRST(HWND hWnd)
{
    POINT cursorPos;
    GetCursorPos(&cursorPos);

    RECT rect;
    GetWindowRect(hWnd, &rect);
    return ImageList_DragMove(cursorPos.x - rect.left, cursorPos.y - rect.top);
}

int J3DAPI jonesConfig_InitStoreDialog(HWND hDlg, int a2, tStoreCartState* pCart)
{
    J3D_UNUSED(a2);

    HWND hAvailableList = GetDlgItem(hDlg, 1005);
    HWND hCartList      = GetDlgItem(hDlg, 1006);
    HWND hCurrentList   = GetDlgItem(hDlg, 1199);
    if ( !jonesConfig_StoreInitItemIcons(hDlg, pCart) )
    {
        return 0;
    }

    ListView_SetImageList(hAvailableList, pCart->hList, LVSIL_SMALL);
    ListView_SetImageList(hCartList, pCart->hList, LVSIL_SMALL);
    ListView_SetImageList(hCurrentList, pCart->hList, LVSIL_SMALL);

    jonesConfig_StoreSetListColumns(hAvailableList, "JONES_STR_COST");
    jonesConfig_StoreSetListColumns(hCartList, "JONES_STR_COST");
    jonesConfig_StoreSetListColumns(hCurrentList, "JONES_STR_COUNT");

    jonesConfig_StoreInitItemList(hCurrentList, pCart->apItemsState, 1199);
    jonesConfig_StoreInitItemList(hAvailableList, pCart->apItemsState, 1005);

    if ( ListView_GetItemCount(hCurrentList) > ListView_GetCountPerPage(hCurrentList) )
    {
        int colWidth = ListView_GetColumnWidth(hCurrentList, 0);
        int vscrlWidth = GetSystemMetrics(SM_CXVSCROLL);
        ListView_SetColumnWidth(hCurrentList, 0, colWidth - vscrlWidth);
    }

    if ( ListView_GetItemCount(hAvailableList) > ListView_GetCountPerPage(hAvailableList) )
    {
        int colWidth   = ListView_GetColumnWidth(hAvailableList, 0);
        int vscrlWidth = GetSystemMetrics(SM_CXVSCROLL);
        ListView_SetColumnWidth(hAvailableList, 0, colWidth - vscrlWidth);
    }

    pCart->total         = 0;
    pCart->cartListWidth = ListView_GetColumnWidth(hCartList, 0);
    jonesConfig_UpdateBalances(hDlg, pCart);

    SetWindowLongPtr(hDlg, DWL_USER, (LONG_PTR)pCart);
    return 1;
}

int J3DAPI jonesConfig_StoreInitItemIcons(HWND hWnd, tStoreCartState* pCart)
{
    pCart->hList = ImageList_Create(48, 48, ILC_MASK | ILC_COLOR8, STD_ARRAYLEN(JonesHud_aStoreItems), 0); // Changed icon size to 48 from 16
    if ( !pCart->hList )
    {
        return 0;
    }

    HINSTANCE hInstance = (HINSTANCE)GetWindowLongPtr(hWnd, GWL_HINSTANCE);
    int result = hInstance != NULL;
    for ( size_t i = 0; i < STD_ARRAYLEN(JonesHud_aStoreItems); ++i )
    {
        pCart->aItemIcons[i] = LoadIconA(hInstance, MAKEINTRESOURCE(JonesHud_aStoreItems[i].iconID));
        result = ImageList_ReplaceIcon(pCart->hList, -1, pCart->aItemIcons[i]);
    }

    return result;
}

void J3DAPI jonesConfig_StoreSetListColumns(HWND hList, const char* pColumnName)
{
    LVCOLUMNA col = { 0 }; // Added: Init to 0
    col.mask       = LVCF_SUBITEM | LVCF_TEXT | LVCF_WIDTH | LVCF_FMT;
    col.cchTextMax = 256;

    uint32_t  height, width;
    stdDisplay_GetBackBufferSize(&width, &height);

    // Removed:
  /*float scale = 1.0f;
    if ( (float)height / RD_REF_HEIGHT <= 1.0f )
    {
        scale = (float)height / RD_REF_HEIGHT;
    }*/
    float scale = RD_REF_APECTRATIO / (width / height); // Added: Fixes scaling issue on 16:9 ...
    int pad = (int)(50.0f * scale); // Changed: Change padding to 50 from 40 due to bigger font size
    int colNum = 0;

    RECT rect;
    GetClientRect(hList, &rect);

    col.fmt      = LVCFMT_LEFT;
    col.cx       = rect.right - rect.left - pad;
    col.pszText  = (LPSTR)jonesString_GetString("JONES_STR_ITEMNAME");
    col.iSubItem = colNum;
    ListView_InsertColumn(hList, colNum++, &col);

    col.fmt      = LVCFMT_RIGHT;
    col.cx       = pad;
    col.pszText  = (LPSTR)jonesString_GetString(pColumnName);
    col.iSubItem = colNum;
    ListView_InsertColumn(hList, colNum, &col);
}

void J3DAPI jonesConfig_StoreInitItemList(HWND hWnd, int* apItemsState, int listID)
{
    LVITEM newItem;
    newItem.mask       = LVIF_STATE | LVIF_PARAM | LVIF_IMAGE | LVIF_TEXT;
    newItem.cchTextMax = 256; // must match aCost
    newItem.state      = 0;
    newItem.stateMask  = 0;

    int imageIdx = 0;
    for ( size_t i = 0; i < STD_ARRAYLEN(JonesHud_aStoreItems); ++i )
    {
        if ( listID == 1005 ) //  available items list
        {
            if ( (apItemsState[i] & 0xF) != 0 ) // if item is available
            {
                newItem.iItem    = imageIdx;
                newItem.iSubItem = 0;
                newItem.pszText  = (LPSTR)jonesString_GetString(JonesHud_aStoreItems[i].aClipName);
                newItem.iImage   = i;
                newItem.lParam   = JonesHud_aStoreItems[i].price | (i << 16);
                ListView_InsertItem(hWnd, &newItem);

                char aCost[256];
                STD_FORMAT(aCost, "%d", JonesHud_aStoreItems[i].price);
                ListView_SetItemText(hWnd, imageIdx++, 1, aCost);
            }

            apItemsState[i] &= ~0xFFFFu;
        }
        else if ( listID == 1199 ) // current inventory list
        {
            if ( (apItemsState[i] & 0xFFF0) >> 4 )
            {
                newItem.iItem    = imageIdx;
                newItem.iSubItem = 0;
                newItem.pszText  = (LPSTR)jonesString_GetString(JonesHud_aStoreItems[i].aName);
                newItem.iImage   = i;
                ListView_InsertItem(hWnd, &newItem);

                char aAmount[256];
                STD_FORMAT(aAmount, "%d", (apItemsState[i] & 0xFFF0) >> 4);// extracts amount
                ListView_SetItemText(hWnd, imageIdx++, 1, aAmount);
            }
        }
    }
}

void J3DAPI jonesConfig_UpdateBalances(HWND hDlg, tStoreCartState* pCart)
{
    CHAR aText[256] = { 0 };
    STD_FORMAT(aText, "%i ", pCart->total);

    HWND hTotalCtrl = GetDlgItem(hDlg, 1173); // total cost
    SetWindowText(hTotalCtrl, aText);

    memset(aText, 0, sizeof(aText));
    STD_FORMAT(aText, "%i ", pCart->balance);

    HWND hBalanceCtrl = GetDlgItem(hDlg, 1174); // balance
    SetWindowText(hBalanceCtrl, aText);
}

void J3DAPI jonesConfig_AddStoreCartItem(HWND hDlg, tStoreCartState* pCart)
{
    HWND hItemsList = GetDlgItem(hDlg, 1005);        // available items list
    UINT selectedCount = ListView_GetSelectedCount(hItemsList);
    if ( selectedCount )
    {
        int total = 0;
        int itemIdx = -1;
        int bonusMapItemIdx = -1;
        for ( UINT i = 0; i < selectedCount; ++i )
        {
            itemIdx = ListView_GetNextItem(hItemsList, itemIdx, LVNI_SELECTED);

            LVITEM lvitem   = { 0 }; // Added: init to 0
            lvitem.mask     = LVIF_PARAM;
            lvitem.iSubItem = 0;
            lvitem.iItem    = itemIdx;
            ListView_GetItem(hItemsList, &lvitem);

            int storeItemIdx = HIWORD(lvitem.lParam);//  HIWORD
            if ( storeItemIdx > -1 ) // Fixed: Added check for bonusMapItemIdx > -1
            {
                if ( JonesHud_aStoreItems[storeItemIdx].menuID == JONESHUD_MENU_INVITEM_BONUSMAP )
                {
                    bonusMapItemIdx = storeItemIdx;
                }

                total += JonesHud_aStoreItems[storeItemIdx].price;
            }
        }

        if ( pCart->balance >= total )
        {
            if ( bonusMapItemIdx > -1 && JonesHud_aStoreItems[bonusMapItemIdx].menuID == JONESHUD_MENU_INVITEM_BONUSMAP ) // Fixed: Added check for bonusMapItemIdx > -1
            {
                const char* pDlgText = jonesString_GetString("JONES_STR_PERU");
                if ( pDlgText )
                {
                    jonesConfig_ShowMessageDialog(hDlg, "JONES_STR_STORE", pDlgText, 191);
                    InvalidateRect(hDlg, 0, 1);
                }
            }

            itemIdx = -1;
            for ( UINT i = 0; i < selectedCount; ++i )
            {
                // Get item from available item list
                itemIdx = ListView_GetNextItem(hItemsList, itemIdx, LVNI_SELECTED);

                char aItemText[256] = { 0 };
                LVITEM item     = { 0 }; // Added: Init to 0
                item.mask       = LVIF_PARAM | LVIF_TEXT;
                item.iSubItem   = 0;
                item.iItem      = itemIdx;
                item.pszText    = aItemText;
                item.cchTextMax = STD_ARRAYLEN(aItemText);
                ListView_GetItem(hItemsList, &item);

                // Add item to purchase list
                int itemStateIdx = HIWORD(item.lParam); // See jonesConfig_StoreInitItemList

                HWND hCartList  = GetDlgItem(hDlg, 1006);// cart list
                item.mask      = LVIF_STATE | LVIF_PARAM | LVIF_IMAGE | LVIF_TEXT;
                item.state     = 0;
                item.stateMask = 0;
                item.iImage    = itemStateIdx;
                item.iItem     = ListView_GetItemCount(hCartList);
                ListView_InsertItem(hCartList, &item);

                // Set item in cart
                memset(aItemText, 0, sizeof(aItemText));
                ListView_GetItemText(hItemsList, itemIdx, 1, aItemText, STD_ARRAYLEN(aItemText));
                ListView_SetItemText(hCartList, item.iItem, 1, aItemText);

                int numItems = pCart->apItemsState[itemStateIdx];
                pCart->apItemsState[itemStateIdx] = ((uint16_t)numItems + 1) | numItems & 0xFFFF0000;

                if ( ListView_GetItemCount(hCartList) > ListView_GetCountPerPage(hCartList) )
                {
                    int vscrollWidth = GetSystemMetrics(SM_CXVSCROLL);
                    ListView_SetColumnWidth(hCartList, 0, pCart->cartListWidth - vscrollWidth);
                }
            }

            pCart->total   += total;
            pCart->balance -= total;
            jonesConfig_UpdateBalances(hDlg, pCart);
        }
        else
        {
            const char* pDlgText = NULL;
            if ( bonusMapItemIdx > -1 && JonesHud_aStoreItems[bonusMapItemIdx].menuID == JONESHUD_MENU_INVITEM_BONUSMAP )
            { // Fixed: Added check for bonusMapItemIdx > -1
                pDlgText = jonesString_GetString("JONES_STR_NOPERU");
            }
            else if ( selectedCount == 1 )
            {
                pDlgText = jonesString_GetString("JONES_STR_CANTBUY1");
            }
            else
            {
                pDlgText = jonesString_GetString("JONES_STR_CANTBUY");
            }

            if ( pDlgText )
            {
                jonesConfig_ShowMessageDialog(hDlg, "JONES_STR_STORE", pDlgText, 191);
                InvalidateRect(hDlg, 0, 1);
            }
        }
    }
}

void J3DAPI jonesConfig_RemoveStoreCartItem(HWND hDlg, tStoreCartState* pCart)
{
    int total = 0;
    HWND hCartList = GetDlgItem(hDlg, 1006);
    UINT selectedCount = ListView_GetSelectedCount(hCartList);
    if ( selectedCount )
    {
        int idx = -1;
        for ( UINT i = 0; i < selectedCount; ++i )
        {
            idx = ListView_GetNextItem(hCartList, idx, LVNI_SELECTED);

            LVITEM lvitem;
            lvitem.mask     = LVIF_PARAM;
            lvitem.iSubItem = 0;
            lvitem.iItem    = idx;
            ListView_GetItem(hCartList, &lvitem);

            total += LOWORD(lvitem.lParam);

            ListView_DeleteItem(hCartList, idx);

            int numItems = pCart->apItemsState[HIWORD(lvitem.lParam)];
            pCart->apItemsState[HIWORD(lvitem.lParam)] = ((uint16_t)numItems - 1) | numItems & 0xFFFF0000;

            int itemsCount = ListView_GetItemCount(hCartList);
            if ( itemsCount <= ListView_GetCountPerPage(hCartList) )
            {
                ListView_SetColumnWidth(hCartList, 0, LOWORD(pCart->cartListWidth));
            }
        }

        pCart->total   -= total;
        pCart->balance += total;
        jonesConfig_UpdateBalances(hDlg, pCart);
    }
}

void J3DAPI jonesConfig_StoreDialog_HandleWM_COMMAND(HWND hWnd, WPARAM wParam)
{
    tStoreCartState* pCart = (tStoreCartState*)GetWindowLongPtr(hWnd, DWL_USER);
    switch ( (int)wParam )
    {
        case 1:
        case 2:
        {
            if ( pCart->balance <= 0 )
            {
                EndDialog(hWnd, wParam);
            }
            else if ( jonesConfig_ShowPurchaseMessageBox(hWnd, pCart) == 1 )
            {
                InvalidateRect(hWnd, 0, 1);
                EndDialog(hWnd, wParam);
            }
        } return;
        case 1168: // Add item
            jonesConfig_AddStoreCartItem(hWnd, pCart);
            return;
        case 1169: // Return item
            jonesConfig_RemoveStoreCartItem(hWnd, pCart);
            return;
        case 1175: // Clear items
            jonesConfig_ClearStoreCart(hWnd, pCart);
            return;
    }
}

void J3DAPI jonesConfig_ClearStoreCart(HWND hDlg, tStoreCartState* pCart)
{
    HWND hWnd = GetDlgItem(hDlg, 1006);
    int count = ListView_GetItemCount(hWnd);
    for ( int i = 0; i < count; ++i )
    {
        LVITEM lvitem;
        lvitem.mask = LVIF_PARAM;
        lvitem.iSubItem = 0;
        lvitem.iItem = i;
        ListView_GetItem(hWnd, &lvitem);

        unsigned int idx = HIWORD(lvitem.lParam);
        pCart->apItemsState[idx] = ((uint16_t)pCart->apItemsState[idx] - 1) | pCart->apItemsState[idx] & 0xFFFF0000;
    }

    pCart->balance += pCart->total;
    pCart->total = 0;
    ListView_DeleteAllItems(hWnd);
    jonesConfig_UpdateBalances(hDlg, pCart);
}

int J3DAPI jonesConfig_ShowPurchaseMessageBox(HWND hWnd, tStoreCartState* dwInitParam)
{
    return JonesDialog_ShowDialog(MAKEINTRESOURCE(212), hWnd, jonesConfig_PurchaseMessageBoxProc, (LPARAM)dwInitParam);
}

INT_PTR CALLBACK jonesConfig_PurchaseMessageBoxProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch ( uMsg )
    {
        case WM_DESTROY:
            jonesConfig_ResetDialogFont(hWnd, jonesConfig_hFontPurchaseMessageBox);
            return 1;

        case WM_INITDIALOG:
            jonesConfig_hFontPurchaseMessageBox = jonesConfig_InitDialog(hWnd, 0, 212);
            int bSuccess = jonesConfig_InitPurchaseMessageBox(hWnd, wParam, (tStoreCartState*)lParam);
            SetWindowPos(hWnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_SHOWWINDOW | SWP_NOMOVE | SWP_NOSIZE);
            return bSuccess;

        case WM_COMMAND:
            jonesConfig_MsgBoxDlg_HandleWM_COMMAND(hWnd, (uint16_t)wParam);
            return 0;

        default:
            return 0;
    }
}

int J3DAPI jonesConfig_InitPurchaseMessageBox(HWND hDlg, int a2, tStoreCartState* pCart)
{
    J3D_UNUSED(a2);

    char aText[512] = { 0 };

    if ( pCart->total )
    {
        STD_STRCAT(aText, "\n");
        const char* pFormat = jonesString_GetString("JONES_STR_ADDINV");
        if ( pFormat )
        {
            STD_STRCAT(aText, pFormat);
            STD_STRCAT(aText, " ");
        }

        pFormat = jonesString_GetString("JONES_STR_SUBCHEST");
        if ( pFormat )
        {
            char aTotalText[256] = { 0 };
            STD_FORMAT(aTotalText, pFormat, pCart->total);
            STD_STRCAT(aText, aTotalText);
            STD_STRCAT(aText, "  ");
        }

        pFormat = jonesString_GetString("JONES_STR_YESNOBUY");
        if ( pFormat )
        {
            STD_STRCAT(aText, pFormat);
        }
    }
    else
    {
        const char* pFormat = jonesString_GetString("JONES_STR_NOCHANGE");
        if ( pFormat )
        {
            STD_STRCAT(aText, pFormat);
            STD_STRCAT(aText, "  ");
        }

        pFormat = jonesString_GetString("JONES_STR_ABANDON");
        if ( pFormat )
        {
            STD_STRCAT(aText, pFormat);
        }
    }

    HWND hMsgControl = GetDlgItem(hDlg, 1191);
    jonesConfig_SetTextControl(hDlg, hMsgControl, aText);
    return 1;
}

int J3DAPI jonesConfig_ShowDialogInsertCD(HWND hWnd, LPARAM dwInitParam)
{
    ShowCursor(1);
    GetWindowLongPtr(hWnd, GWL_HINSTANCE); // TODO: ??
    int btnNum = JonesDialog_ShowDialog(MAKEINTRESOURCE(167), hWnd, jonesConfig_DialogInsertCDProc, dwInitParam);
    ShowCursor(0);
    return btnNum; // 1 - ok or 2 - quit
}

INT_PTR CALLBACK jonesConfig_DialogInsertCDProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch ( uMsg )
    {
        case WM_DESTROY:
            jonesConfig_ResetDialogFont(hWnd, jonesConfig_hFontDialogInsertCD);
            return 1;

        case WM_INITDIALOG:
            jonesConfig_hFontDialogInsertCD = jonesConfig_InitDialog(hWnd, 0, 167);
            return jonesConfig_InitDialogInsertCD(hWnd, wParam, lParam);

        case WM_COMMAND:
            jonesConfig_InsertCD_HandleWM_COMMAND(hWnd, (uint16_t)wParam);
            return 1;

        default:
            return 0;
    }
}

int J3DAPI jonesConfig_InitDialogInsertCD(HWND hDlg, int a2, int cdNum)
{
    J3D_UNUSED(a2);

    HWND hWnd = GetDlgItem(hDlg, 1134);
    if ( cdNum == 2 )
    {
        const char* pText = jonesString_GetString("JONES_STR_INSERT2");
        if ( pText )
        {
            SetWindowText(hWnd, pText);
        }
    }
    else
    {
        const char* pText = jonesString_GetString("JONES_STR_INSERT1");
        if ( pText )
        {
            SetWindowText(hWnd, pText);
        }
    }

    SetWindowLongPtr(hDlg, DWL_USER, (LONG_PTR)cdNum);
    return 1;
}

int J3DAPI jonesConfig_InsertCD_HandleWM_COMMAND(HWND hWnd, int nResult)
{
    int cdNum = (int)GetWindowLongPtr(hWnd, DWL_USER);
    int prevCdNum = cdNum;
    if ( nResult == 1 ) // ok
    {
        cdNum = JonesFile_GetCurrentCDNum();
        if ( prevCdNum != cdNum )
        {
            return cdNum;
        }

        return EndDialog(hWnd, nResult);
    }

    if ( nResult == 2 ) // quit
    {
        return EndDialog(hWnd, nResult);
    }

    return cdNum;
}

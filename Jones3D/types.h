#ifndef JONES3D_TYPES_H
#define JONES3D_TYPES_H
#include <stdint.h>
#include <Windows.h>
#include <CommCtrl.h>

#include <j3dcore/j3d.h>
#include <rdroid/types.h>
#include <sith/types.h>
#include <std/types.h>

J3D_EXTERN_C_START

typedef enum eJonesHudMenuType
{
    JONESHUD_MENU_WEAP_GLOVES               = 0,
    JONESHUD_MENU_WEAP_WHIP                 = 1,
    JONESHUD_MENU_WEAP_PISTOL               = 2,
    JONESHUD_MENU_WEAP_TOKAREV              = 3,
    JONESHUD_MENU_WEAP_MAUSER               = 4,
    JONESHUD_MENU_WEAP_SIMONOV              = 5,
    JONESHUD_MENU_WEAP_MACHETE              = 6,
    JONESHUD_MENU_WEAP_SUBMACHINE           = 7,
    JONESHUD_MENU_WEAP_SHOTGUN              = 8,
    JONESHUD_MENU_WEAP_GRENADE              = 9,
    JONESHUD_MENU_WEAP_SATCHEL              = 10,
    JONESHUD_MENU_WEAP_BAZOOKA              = 11,
    JONESHUD_MENU_AMMO_MAUSER               = 12,
    JONESHUD_MENU_AMMO_SUBMACHINE           = 13,
    JONESHUD_MENU_AMMO_SIMONOV              = 14,
    JONESHUD_MENU_AMMO_TOKAREV              = 15,
    JONESHUD_MENU_AMMO_SHOTGUN              = 16,
    JONESHUD_MENU_AMMO_BAZOOKA              = 17,
    JONESHUD_MENU_AMMO_ROCKET_PACK          = 18,
    JONESHUD_MENU_AMMO_GRENADE_PACK         = 19,
    JONESHUD_MENU_AMMO_SATCHEL_PACK         = 20,
    JONESHUD_MENU_HEALTH_SMALL              = 21,
    JONESHUD_MENU_HEALTH_BIG                = 22,
    JONESHUD_MENU_HEALTH_HERBS_SMALL        = 23,
    JONESHUD_MENU_HEALTH_HERBS_BIG          = 24,
    JONESHUD_MENU_HEALTH_POISONKIT          = 25,
    JONESHUD_MENU_UNKNOWN_26                = 26,
    JONESHUD_MENU_TREASURE_COINS_GOLD       = 27,
    JONESHUD_MENU_TREASURE_COINS_SILVER     = 28,
    JONESHUD_MENU_TREASURE_IDOL_GOLD        = 29,
    JONESHUD_MENU_TREASURE_IDOL_SILVER      = 30,
    JONESHUD_MENU_TREASURE_IDOL_RAID        = 31,
    JONESHUD_MENU_TREASURE_INGOT_GOLD       = 32,
    JONESHUD_MENU_TREASURE_INGOT_SILVER     = 33,
    JONESHUD_MENU_TREASURE_GEM_RED          = 34,
    JONESHUD_MENU_TREASURE_GEM_GREEN        = 35,
    JONESHUD_MENU_TREASURE_GEM_BLUE         = 36,
    JONESHUD_MENU_TREASURE_GEM_AETHERIAL    = 37,
    JONESHUD_MENU_TREASURE_CASHBOX          = 38,
    JONESHUD_MENU_TREASURE_CHEST            = 39,
    JONESHUD_MENU_INVITEM_ZIPPO             = 40,
    JONESHUD_MENU_INVITEM_CHALK             = 41,
    JONESHUD_MENU_INVITEM_IMP1              = 42,
    JONESHUD_MENU_INVITEM_IMP2              = 43,
    JONESHUD_MENU_INVITEM_IMP3              = 44,
    JONESHUD_MENU_INVITEM_IMP4              = 45,
    JONESHUD_MENU_INVITEM_IMP5              = 46,
    JONESHUD_MENU_INVITEM_POTSHARD          = 47,
    JONESHUD_MENU_INVITEM_ELEVATOR_GEAR     = 48,
    JONESHUD_MENU_INVITEM_TURNER_KEY        = 49,
    JONESHUD_MENU_INVITEM_TABLET_A          = 50,
    JONESHUD_MENU_INVITEM_TABLET_B          = 51,
    JONESHUD_MENU_INVITEM_TABLET_C          = 52,
    JONESHUD_MENU_INVITEM_IDOL_MARDUK       = 53,
    JONESHUD_MENU_INVITEM_LIMP_RAFT         = 54,
    JONESHUD_MENU_INVITEM_PATCHKIT          = 55,
    JONESHUD_MENU_INVITEM_CANDLE_A          = 56,
    JONESHUD_MENU_INVITEM_CANDLE_B          = 57,
    JONESHUD_MENU_INVITEM_CANDLE_C          = 58,
    JONESHUD_MENU_INVITEM_CANDLE_D          = 59,
    JONESHUD_MENU_INVITEM_COIN              = 60,
    JONESHUD_MENU_INVITEM_SEAL_KEY          = 61,
    JONESHUD_MENU_INVITEM_BULB              = 62,
    JONESHUD_MENU_INVITEM_PLANT             = 63,
    JONESHUD_MENU_INVITEM_BUD               = 64,
    JONESHUD_MENU_INVITEM_BRONZE_KEY        = 65,
    JONESHUD_MENU_INVITEM_OLDLADY_KEY       = 66,
    JONESHUD_MENU_INVITEM_OILJAR            = 67,
    JONESHUD_MENU_INVITEM_DIVER_ROOM_1_KEY  = 68,
    JONESHUD_MENU_INVITEM_DIVER_ROOM_2_KEY  = 69,
    JONESHUD_MENU_INVITEM_DIVER_ROOM_3_KEY  = 70,
    JONESHUD_MENU_INVITEM_PAD_KEY           = 71,
    JONESHUD_MENU_INVITEM_FIELD_SHOVEL      = 72,
    JONESHUD_MENU_INVITEM_PLANE_PROPELLOR   = 73,
    JONESHUD_MENU_INVITEM_HAMMER            = 74,
    JONESHUD_MENU_INVITEM_ARM_DEVICE        = 75,
    JONESHUD_MENU_INVITEM_CRANK_RUSTY       = 76,
    JONESHUD_MENU_INVITEM_VOL_KEY_SHARKGATE = 77,
    JONESHUD_MENU_UNKNOWN_78                = 78,
    JONESHUD_MENU_UNKNOWN_79                = 79,
    JONESHUD_MENU_INVITEM_TRAM_WHEEL        = 80,
    JONESHUD_MENU_INVITEM_CUFFS_KEY         = 81,
    JONESHUD_MENU_INVITEM_TIKI_KEY          = 82,
    JONESHUD_MENU_INVITEM_TIKI_KEY_2        = 83,
    JONESHUD_MENU_INVITEM_MONKEY_KEY        = 84,
    JONESHUD_MENU_INVITEM_SHARK_KEY         = 85,
    JONESHUD_MENU_INVITEM_H2O_KEY           = 86,
    JONESHUD_MENU_INVITEM_IDOL_BIRD         = 87,
    JONESHUD_MENU_INVITEM_IDOL_FISH         = 88,
    JONESHUD_MENU_INVITEM_IDOL_JAG          = 89,
    JONESHUD_MENU_INVITEM_MIRROR            = 90,
    JONESHUD_MENU_INVITEM_UNKNOWN_91        = 91,
    JONESHUD_MENU_INVITEM_CRANK             = 92,
    JONESHUD_MENU_INVITEM_MINECAR_WHEEL     = 93,
    JONESHUD_MENU_INVITEM_PYR_GEM_EYE       = 94,
    JONESHUD_MENU_INVITEM_WATCH             = 95,
    JONESHUD_MENU_UNKNOWN_96                = 96,
    JONESHUD_MENU_INVITEM_ENGIN_CHAIN       = 97,
    JONESHUD_MENU_INVITEM_GASCAN            = 98,
    JONESHUD_MENU_INVITEM_KINDLING_CONE     = 99,
    JONESHUD_MENU_INVITEM_BUCKET_WOOD       = 100,
    JONESHUD_MENU_INVITEM_OILCAN            = 101,
    JONESHUD_MENU_UNKNOWN_102               = 102,
    JONESHUD_MENU_UNKNOWN_103               = 103,
    JONESHUD_MENU_INVITEM_FUSE              = 104,
    JONESHUD_MENU_UNKNOWN_105               = 105,
    JONESHUD_MENU_INVITEM_SOL_GEM_A         = 106,
    JONESHUD_MENU_INVITEM_SOL_GEM_B         = 107,
    JONESHUD_MENU_INVITEM_SOL_GEM_C         = 108,
    JONESHUD_MENU_UNKNOWN_109               = 109,
    JONESHUD_MENU_INVITEM_PYRAMID_KEY       = 110,
    JONESHUD_MENU_INVITEM_OIL_KEY           = 111,
    JONESHUD_MENU_INVITEM_LEVER_STICK       = 112,
    JONESHUD_MENU_INVITEM_ANUBIS_ARM        = 113,
    JONESHUD_MENU_INVITEM_TURTLE_GEAR       = 114,
    JONESHUD_MENU_INVITEM_ELEVATOR_KEY      = 115,
    JONESHUD_MENU_INVITEM_ROBO_EYE          = 116,
    JONESHUD_MENU_INVITEM_ROBO_HEAD         = 117,
    JONESHUD_MENU_INVITEM_MARDUK_KEY        = 118,
    JONESHUD_MENU_INVITEM_WEAP_MIRROR       = 119,
    JONESHUD_MENU_INVITEM_PLANK_2M          = 120,
    JONESHUD_MENU_INVITEM_WHIP_SPIKE        = 121,
    JONESHUD_MENU_INVITEM_SQUARE_KEY        = 122,
    JONESHUD_MENU_INVITEM_BONUSMAP          = 123,
    JONESHUD_MENU_UNKNOWN_124               = 124,
    JONESHUD_MENU_IQ                        = 125,
    JONESHUD_MENU_HELP                      = 126,
    JONESHUD_MENU_SAVE_GAME                 = 127,
    JONESHUD_MENU_LOAD_GAME                 = 128,
    JONESHUD_MENU_GAME_SETTINGS             = 129,
    JONESHUD_MENU_CONTROLS_SETTINGS         = 130,
    JONESHUD_MENU_DISPLAY_SETTINGS          = 131,
    JONESHUD_MENU_SOUND_SETTINGS            = 132,
    JONESHUD_MENU_EXIT                      = 133,
} JonesHudMenuType;

typedef enum eJonesStartMode
{
    JONES_STARTMODE_STARTGAME       = 0,
    JONES_STARTMODE_LOADGAME        = 1,
    JONES_STARTMODE_DEVELOPERDIALOG = 2,
    JONES_STARTMODE_SOUNDSETTINGS   = 3,
    JONES_STARTMODE_DISPLAYSETTINGS = 4,
} JonesStartMode;

typedef enum eJonesOuputMode
{
    JONES_OUTPUTMODE_NONE    = 0,
    JONES_OUTPUTMODE_CONSOLE = 1,
    JONES_OUTPUTMODE_LOGFILE = 2,
} JonesOuputMode;

typedef enum eJonesLogLevel
{
    JONES_LOGLEVEL_ERROR   = 0,
    JONES_LOGLEVEL_NORMAL  = 1,
    JONES_LOGLEVEL_VERBOSE = 2,
} JonesLogLevel;

typedef struct sJonesDialogData JonesDialogData;

typedef int (J3DAPI* JonesMainProcessFunc)();
typedef void (J3DAPI* JonesDialogReleaseDCFunc)(HDC);
typedef HDC(*JonesDialogGetDrawBufferDCFunc)(void);
typedef int (*JonesDialogFlipPageFunc)(void);

typedef struct sJonesResource
{
    char dirPath[128];
    size_t numGobFiles;
    Gob* aGobFiles[32];
} JonesResource;
static_assert(sizeof(JonesResource) == 260, "sizeof(JonesResource) == 164");

typedef struct sJonesFileHandle
{
    int bUsed;
    char aName[63];
    int bFileHandle;
    tFileHandle hFile;
    GobFileHandle* pGobFileHandle;
} JonesFileHandle;

typedef struct sJonesLevelInfo
{
    char* pFilename;
    char* pMatFilename;
    rdVector2 wallineStart;
    rdVector2 wallineEnd;
    int bPrimaryMusicTheme;
    char* pName;
} JonesLevelInfo;

typedef struct sJonesHudMenuItem
{
    rdThing* prdIcon;
    int id;
    int inventoryID;
    int flags;
    float alpha;
    unsigned int msecMoveDuration;
    unsigned int msecMoveEndTime;
    unsigned int msecMoveDelta;
    rdVector3 startMovePos;
    rdVector3 endMovePos;
    rdVector3 pos;
    rdVector3 pyr;
    int nextDownItemId;
    int nextRightItemId;
    int nextUpItemId;
    int nextLeftItemId;
} JonesHudMenuItem;

typedef struct sJonesDisplaySettings
{
    int bWindowMode;
    int bDualMonitor;
    int displayDeviceNum; // Can be -1
    size_t videoModeNum;
    size_t device3DNum;
    Std3DMipmapFilterType filter;
    uint32_t width;
    uint32_t height;
    int bBuffering;
    int bFog;
    rdGeometryMode geoMode;
    rdLightMode lightMode;
    int bClearBackBuffer;
} JonesDisplaySettings;
static_assert(sizeof(JonesDisplaySettings) == 52, "sizeof(JonesDisplaySettings) == 52");


typedef struct sJonesState
{
    int bDevMode;
    int startMode;
    int outputMode;
    int logLevel;
    int performanceLevel;
    char aCurLevelFilename[128];
    char aInstallPath[128];
    char aCDPath[128];
    wchar_t waPlayerName[20];
    float soundVolume;
    int bSound3D;
    int bReverseSound;
    JonesDisplaySettings displaySettings;
    float fogDensity;
} JonesState;

typedef struct sJonesResourcesOld
{
    JonesResource installed;
    JonesResource cwd;
    JonesResource cd;
} JonesResourcesOld;

typedef struct sJonesDialogGameState
{
    int bFrontBufferClipper;
    int bBackBufferClipper;
    int bOffScreenDBIAllocSkipped;
    int bScreenDBIAllocSkipped;
    int unknown4;
    int bControlsActive;
    int bGamePaused;
    int unknown7;
} JonesDialogGameState;

typedef struct sJonesConsoleTextLine
{
    char aLine[128];
    size_t msecExpireTime;
    int ID;
    int unknown2;
    int unknown3;
    int unknown4;
} JonesConsoleTextLine;

typedef struct sJonesDialogCursorInfo
{
    POINT point[2];
    HCURSOR hCursor;
    ICONINFO info;
    SIZE size;
    HDC hDrawDC;
    RECT rect;
    HWND aHwnd[10];
    int numHandels;
} JonesDialogCursorInfo;

struct sJonesDialogData
{
    uint32_t dwSize;
    LPOFNHOOKPROC lpfnPrevHook;
    WNDPROC lpfnWndProc;
    LPARAM lPrevCustData;
    LPVOID lpUserData;
    RECT rcWindow;
    int unknown9;
    JonesDialogData* pExtraData;
};

typedef struct sJonesControlsScheme
{
    int unknown0;
    int aActions[37][9];
    char aName[128];
} JonesControlsScheme;

typedef struct sJonesControlAction
{
    int unknown0;
    int aUnknown1[8];
} JonesControlAction;

typedef struct sJonesControlsConfig
{
    void* unknown0;
    int unknown1;
    int unknown2;
    int numSchemes;
    int selectedShemeIdx;
    JonesControlsScheme* aSchemes;
} JonesControlsConfig;

typedef struct sStoreItem
{
    int iconID;
    const char aInventoryName[256];
    const char aStoreName[256];
    int menuID;
    int cost;
    int unknown132;
} tStoreItem;

typedef struct sStoreCartState
{
    int* apItemsState;
    HICON aItemIcons[14];
    HIMAGELIST hList;
    int total;
    int balance;
    int bDragging;
    int dragListId;
    int unknown20;
    int cartListWidth;
} tStoreCartState;

typedef struct sJonesDialogImageInfo
{
    HDC hdcFront;
    HDC hdcBack;
    HBITMAP hBmp;
    int unknown3;
    int unknown4;
} JonesDialogImageInfo;

typedef struct sLevelCompletedDialogState
{
    int* balance;
    int* apItemsState;
    int unknown2;
    int elapsedTime;
    int iqPoints;
    int numFoundTrasures;
    int foundTreasureValue;
    int totalTreasureValue;
    JonesDialogImageInfo* pIQImageInfo;
    SithGameStatistics* pStatistics;
    int unknown10;
} tLevelCompletedDialogState;

typedef struct sMenuItemOrientInfo
{
    float scale;
    rdVector3 pyr;
} tMenuItemOrientInfo;

typedef struct sJonesCreditEntry
{
    const char aText[128];
    int flags;
    int fontColorNum;
} tJonesCreditEntry;

typedef struct sJonesHudRect
{
    float x;
    float y;
    float width;
    float height;
} JonesHudRect;

typedef struct sJonesDialogFontInfo
{
    HFONT hFont;
    HFONT hControlFont;
    int bWindowMode;
    float fontScaleX;
    float fontScaleY;
    int dialogID;
} JonesDialogFontInfo;
static_assert(sizeof(JonesDialogFontInfo) == 24, "sizeof(JonesDialogFontInfo) == 24");

typedef struct sGameSaveMsgBoxData
{
    int dialogID;
    char* pNdsFilePath;
} GameSaveMsgBoxData;

typedef struct sGameOverDialogData
{
    int result;
    char* pRestoreFilename;
    tSoundChannelHandle sndChnlMusic;
    tSoundHandle hSndMusic;
} GameOverDialogData;

typedef struct sGDIDIBSectionInfo
{
    HBITMAP hBitmap;
    HDC hScreenDC;
    PBYTE pPixels;
    BITMAPINFO bmpInfo;
    int bHasDC;
} GDIDIBSectionInfo;

J3D_EXTERN_C_END
#endif //JONES3D_TYPES_H

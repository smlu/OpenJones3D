#ifndef JONES3D_JONESDIALOGTYPES_H
#define JONES3D_JONESDIALOGTYPES_H
#define STRICT
#include <windows.h>
#include <commctrl.h>
#include <commdlg.h> // LPOPENFILENAMEA, LPOFNHOOKPROC used by dialog types
#include <Jones3D/types.h>

J3D_EXTERN_C_START

typedef struct sJonesDialogData JonesDialogData;

typedef void (J3DAPI* JonesDialogReleaseDCFunc)(HDC);
typedef HDC(*JonesDialogGetDrawBufferDCFunc)(void);
typedef int (*JonesDialogFlipPageFunc)(void);

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

typedef struct sJonesControlsConfig
{
    LONG maxNameLen;
    size_t numDeleteSchemes;
    char (*paDeleteSchemes)[128];
    size_t numSchemes;
    int selectedShemeIdx; // cane be -1
    JonesControlsScheme* aSchemes;
} JonesControlsConfig;
static_assert(sizeof(JonesControlsConfig) == 24, "sizeof(JonesControlsConfig) == 24");

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

typedef struct sGDIDIBSectionInfo
{
    HBITMAP hBitmap;
    HDC hScreenDC;
    PBYTE pPixels;
    BITMAPINFO bmpInfo;
    int bHasDC;
} GDIDIBSectionInfo;

J3D_EXTERN_C_END
#endif //JONES3D_JONESDIALOGTYPES_H

// Added: Null version of std/Win95/stdWin95.c. There is no window on a headless Null
// platform: the window, instance and GUID sets are mutable state, but nothing sets a
// window unless a future Null windowing layer calls the setters. Same header contracts.
#include <j3dcore/j3d.h>

#include <std/Win95/stdWin95.h>

#include <std/General/std.h>

static tSysWindow stdWin95_hwnd        = NULL;
static tSysInstance stdWin95_hInstance = NULL;
static bool stdWin95_bGuidSet          = false;
static char stdWin95_aGuid[STD_SYS_GUID_SIZE] = { 0 };

void J3DAPI stdWin95_SetWindow(tSysWindow hwnd)
{
    stdWin95_hwnd = hwnd;
}

tSysWindow J3DAPI stdWin95_GetWindow()
{
    return stdWin95_hwnd;
}

void J3DAPI stdWin95_SetInstance(tSysInstance hinstance)
{
    stdWin95_hInstance = hinstance;
}

tSysInstance stdWin95_GetInstance(void)
{
    return stdWin95_hInstance;
}

void J3DAPI stdWin95_SetGuid(const tSysGuid* pGuid)
{
    STD_ASSERTREL(pGuid);
    memcpy(stdWin95_aGuid, pGuid, sizeof(stdWin95_aGuid));
    stdWin95_bGuidSet = true;
}

const tSysGuid* J3DAPI stdWin95_GetGuid(void)
{
    return stdWin95_bGuidSet ? (const tSysGuid*)stdWin95_aGuid : NULL;
}

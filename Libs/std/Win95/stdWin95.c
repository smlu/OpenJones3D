
#include "stdWin95.h"

#include <j3dcore/j3dhook.h>
#include <std/General/std.h>
#include <std/RTI/symbols.h>


static GUID stdWin95_guid;
static HWND stdWin95_hwnd           = NULL;
static HINSTANCE stdWin95_hInstance = NULL;


void stdWin95_InstallHooks(void)
{
    J3D_HOOKFUNC(stdWin95_SetWindow);
    J3D_HOOKFUNC(stdWin95_GetWindow);
    J3D_HOOKFUNC(stdWin95_SetInstance);
    J3D_HOOKFUNC(stdWin95_GetInstance);
    J3D_HOOKFUNC(stdWin95_SetGuid);
}

void stdWin95_ResetGlobals(void)
{
    /*memset(&stdWin95_guid, 0, sizeof(stdWin95_guid));
    memset(&stdWin95_hwnd, 0, sizeof(stdWin95_hwnd));
    memset(&stdWin95_hInstance, 0, sizeof(stdWin95_hInstance));*/
}

void J3DAPI stdWin95_SetWindow(HWND hwnd)
{
    stdWin95_hwnd = hwnd;
}

HWND J3DAPI stdWin95_GetWindow()
{
    return stdWin95_hwnd;
}

void J3DAPI stdWin95_SetInstance(HINSTANCE hinstance)
{
    stdWin95_hInstance = hinstance;
}

HINSTANCE stdWin95_GetInstance(void)
{
    return stdWin95_hInstance;
}

void J3DAPI stdWin95_SetGuid(const GUID* pGuid)
{
    STD_ASSERTREL(pGuid);
    stdWin95_guid = *pGuid;
}

const GUID* J3DAPI stdWin95_GetGuid(void)
{
    return &stdWin95_guid;
}


#include "wkernel.h"
#include "wkernel/RTI/symbols.h"

#include <j3dcore/j3dhook.h>
#include <std/Win95/stdWin95.h>

#include <CommCtrl.h> // InitCommonControls
#include <initguid.h> // DEFINE_GUID


#define IDI_APPICON 108
DEFINE_GUID(wkernel_guid, 0x82CE4DA0, 0x9CBF, 0x11D1, 0x90, 0x85, 0x00, 0x60, 0x97, 0x76, 0x0EA, 0x02);
static const TCHAR wkernel_aClassName[] = "wKernelJones3D";

static HWND wkernel_hwnd       = 0;
static int wkernel_nShowCmd    = 0;
static MSG wkernel_msg_1       = { 0 };
static MSG wkernel_msg_2       = { 0 };

static WKERNELPROC wkernel_pfProcess            = NULL;
static WKERNELSTARTUPPROC wkernel_pfOnStartup   = NULL;
static WKERNELSHUTDOWNPROC wkernel_pfOnShutdown = NULL;
static WKERNELWNDPROC wkernel_pfWndProc         = NULL;

static int wkernel_wndBorderWidth   = 0;
static int wkernel_wndMenuBarHeight = 0;

static inline int J3DAPI wkernel_CreateWindow(HINSTANCE hInstance, int nShowCmd, LPCSTR lpWindowName);
static LRESULT CALLBACK wkernel_MainWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

void wkernel_InstallHooks(void)
{
    J3D_HOOKFUNC(wkernel_Run);
    J3D_HOOKFUNC(wkernel_PeekProcessEvents);
    J3D_HOOKFUNC(wkernel_ProcessEvents);
    J3D_HOOKFUNC(wkernel_SetWindowStyle);
    J3D_HOOKFUNC(wkernel_SetWindowSize);
    J3D_HOOKFUNC(wkernel_SetWindowProc);
    J3D_HOOKFUNC(wkernel_CreateWindow);
    J3D_HOOKFUNC(wkernel_MainWndProc);
}

void wkernel_ResetGlobals(void)
{}

int J3DAPI wkernel_Run(HINSTANCE hinstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd, LPCSTR lpWindowName)
{
    J3D_UNUSED(hPrevInstance);

    // Added: Refactored to run main proc via callback
    if ( wkernel_pfProcess == NULL )
    {
        // TODO: [LOG] make log entry
        fprintf(stderr, "ERROR: wkernel_Run: No main process set!\n");
        return -1;
    }

    wkernel_nShowCmd = nShowCmd;
    if ( wkernel_CreateWindow(hinstance, nShowCmd, lpWindowName) ) {
        return -1;
    }

    stdWin95_SetWindow(wkernel_hwnd);
    stdWin95_SetInstance(hinstance);
    stdWin95_SetGuid(&wkernel_guid);

    InitCommonControls();

    wkernel_wndBorderWidth = 2 * GetSystemMetrics(SM_CXFRAME);
    int wndBorderSize = GetSystemMetrics(SM_CXFRAME);
    wkernel_wndMenuBarHeight = GetSystemMetrics(SM_CYMENU) + 2 * wndBorderSize;

    // Added: Refactored to run main proc via callback
    if ( wkernel_pfOnStartup ) {
        if ( wkernel_pfOnStartup(lpCmdLine) ) {
            return -1;
        }
    }

    int result = 0;
    while ( result != -1 )
    {
        if ( result )
        {
            if ( result == 1 ) // Finish
            {
                return 0; // Fixed: Changed return code to 0
            }
            else
            {
                // TODO: [LOG] Add log entry
                // Added: Log
                fprintf(stdout, "WARNING: wkernel_Run: Unhandled result code: %d, skipping...\n", result);
            }
        }

        // Fixed: Removed else statement to fix bug when result != 1 || result != -1
        result = wkernel_pfProcess();
    }

    return result;
}

WKERNELPROC J3DAPI wkernel_SetProcessProc(WKERNELPROC pfProc)
{
    WKERNELPROC pfCurProc = wkernel_pfProcess;
    wkernel_pfProcess = pfProc;
    return pfCurProc;
}

WKERNELSTARTUPPROC J3DAPI wkernel_SetStartupCallback(WKERNELSTARTUPPROC pfOnClose)
{
    WKERNELSTARTUPPROC pfCurProc = wkernel_pfOnStartup;
    wkernel_pfOnStartup = pfOnClose;
    return pfCurProc;
}

WKERNELSHUTDOWNPROC J3DAPI wkernel_SetShutdownCallback(WKERNELSHUTDOWNPROC pfOnClose)
{
    WKERNELSHUTDOWNPROC pfCurProc = wkernel_pfOnShutdown;
    wkernel_pfOnShutdown = pfOnClose;
    return pfCurProc;
}

int wkernel_PeekProcessEvents(void)
{
    int bGotMsg;
    if ( !PeekMessage(&wkernel_msg_1, NULL, 0, 0, PM_NOREMOVE) )
    {
        return 0;
    }

    while ( 1 )
    {
        bGotMsg = GetMessage(&wkernel_msg_1, NULL, 0, 0);
        if ( bGotMsg == -1 ) // Error?
        {
            break;
        }

        if ( !bGotMsg ) // WM_QUIT retrieved
        {
            return 1;
        }

        TranslateMessage(&wkernel_msg_1);
        DispatchMessage(&wkernel_msg_1);

        if ( !PeekMessage(&wkernel_msg_1, NULL, 0, 0, PM_NOREMOVE) )
        {
            return 0;
        }
    }

    return -1;
}

int wkernel_ProcessEvents(void)
{
    int bGotMsg;
    while ( 1 )
    {
        bGotMsg = GetMessage(&wkernel_msg_2, NULL, 0, 0);
        if ( bGotMsg == -1 ) // Error?
        {
            break;
        }

        if ( !bGotMsg ) // WM_QUIT retrieved
        {
            return 1;
        }

        TranslateMessage(&wkernel_msg_2);
        DispatchMessage(&wkernel_msg_2);
        if ( !PeekMessage(&wkernel_msg_2, NULL, 0, 0, PM_NOREMOVE) )
        {
            return 0;
        }
    }

    return -1;
}

void J3DAPI wkernel_SetWindowStyle(LONG dwNewLong)
{
    SetWindowLong(wkernel_hwnd, GWL_STYLE, dwNewLong);
    if ( (dwNewLong & WS_BORDER) != 0 )
    {
        wkernel_wndBorderWidth = 2 * GetSystemMetrics(SM_CXFRAME);
        wkernel_wndMenuBarHeight = 2 * GetSystemMetrics(SM_CXFRAME);
    }

    if ( (dwNewLong & WS_CAPTION) != 0 )
    {
        wkernel_wndMenuBarHeight += GetSystemMetrics(SM_CYMENU);
    }
}

BOOL J3DAPI wkernel_SetWindowSize(int width, int height)
{
    return SetWindowPos(
        wkernel_hwnd,
        NULL,
        0,
        0,
        wkernel_wndBorderWidth + width,
        wkernel_wndMenuBarHeight + height,
        SWP_NOZORDER | SWP_NOMOVE
    );
}

void J3DAPI wkernel_SetWindowProc(WKERNELWNDPROC pfProc)
{
    wkernel_pfWndProc = pfProc;
}

int J3DAPI wkernel_CreateWindow(HINSTANCE hInstance, int nShowCmd, LPCSTR lpWindowName)
{
    J3D_UNUSED(nShowCmd);
    int width;
    int height;
    WNDCLASSEXA wndcls = { 0 };

    wndcls.cbSize        = sizeof(WNDCLASSEXA);
    wndcls.hInstance     = hInstance;
    wndcls.lpszClassName = wkernel_aClassName;
    wndcls.lpszMenuName  = NULL;
    wndcls.lpfnWndProc   = wkernel_MainWndProc;
    wndcls.style         = 0;

    // Added: Load app icon from resource
    wndcls.hIcon = LoadIcon(hInstance, MAKEINTRESOURCEA(IDI_APPICON));
    if ( !wndcls.hIcon )
    {
        wndcls.hIcon = LoadIcon(hInstance, "APPICON");
        if ( !wndcls.hIcon ) {
            wndcls.hIcon = LoadIcon(NULL, (LPCSTR)IDI_APPLICATION);
        }
    }
    wndcls.hIconSm = LoadIcon(hInstance, MAKEINTRESOURCEA(IDI_APPICON));
    if ( !wndcls.hIconSm )
    {
        wndcls.hIconSm = LoadIcon(hInstance, "APPICON");
        if ( !wndcls.hIconSm ) {
            wndcls.hIconSm = LoadIcon(NULL, (LPCSTR)IDI_APPLICATION);
        }
    }

    wndcls.hCursor    = LoadCursor(NULL, (LPCSTR)IDC_ARROW);
    wndcls.cbClsExtra = 0;
    wndcls.cbWndExtra = 0;
    wndcls.hbrBackground = (HBRUSH)GetStockObject(4);
    if ( !RegisterClassEx(&wndcls) )
    {
        // TODO: Add log 
        return 1;
    }

    // Make sure only one game window is live
    if ( FindWindow(wkernel_aClassName, lpWindowName) )
    {
        // Added: log print
        // TODO: [LOG] make log entry
        fprintf(stdout, "INFO: wkernel_CreateWindow: An existing instance of window already running!\n");
        return 1;
    }

    height = GetSystemMetrics(1);
    width = GetSystemMetrics(0);
    wkernel_hwnd = CreateWindowEx(
        WS_EX_TOPMOST,
        wkernel_aClassName,
        lpWindowName,
        WS_POPUP | WS_VISIBLE,
        0,
        0,
        width,
        height,
        NULL,
        NULL,
        hInstance,
        NULL
    );

    wkernel_SetWindowSize(100, 25);
    if ( !wkernel_hwnd )
    {
        return 1;
    }

    ShowWindow(wkernel_hwnd, 1);
    UpdateWindow(wkernel_hwnd);
    return 0;
}

LRESULT CALLBACK wkernel_MainWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    bool bProcess = true;

    if ( uMsg == WM_DESTROY )
    {
        PostQuitMessage(0);
        bProcess = false;
    }

    if ( uMsg == WM_CLOSE )
    {
        if ( wkernel_pfOnShutdown ) {
            wkernel_pfOnShutdown();
        }
        bProcess = false;
    }

    int retValue;
    if ( bProcess && wkernel_pfWndProc && wkernel_pfWndProc(hWnd, uMsg, wParam, lParam, &retValue) )
    {
        return retValue;
    }

    // Fallback to default window process
    return DefWindowProc(hWnd, uMsg, wParam, lParam);
}
#include <std/Win95/stdSysTagsWin32.h>
#include <Jones3D/Main/JonesMain.h>
#include <Jones3D/Main/JonesMainSys.h>

#include <Jones3D/Platform/Win32/JonesDialog.h>
#include <Jones3D/Display/JonesConsole.h>
#include <Jones3D/Display/JonesHud.h>
#include <Jones3D/Main/jonesString.h>
#include <Jones3D/Play/jonesCog.h>

#include <sith/Dss/sithGamesave.h>
#include <sith/Devices/sithSoundMixer.h>
#include <sith/Engine/sithCamera.h>
#include <sith/Main/sithMain.h>
#include <sith/World/sithWorld.h>

#include <sound/Sound.h>

#include <std/General/stdFnames.h>
#include <std/General/stdPlatform.h>
#include <std/General/stdUtil.h>
#include <std/Win95/stdDisplay.h>
#include <std/Win95/stdWin95.h>

#include <wkernel/wkernel.h>
#include <wkernel/wkernelWin32.h>

#include <stdint.h>

// Win32 message handling for the game and intro windows, moved out of Jones3D/Main/JonesMain.c.
// The game logic of these handlers lives in the JonesMain_On* functions in JonesMain.c.

#define JONES_QUICKSAVE_TEXTSHOWTIME    1000u // 1 sec
#define JONES_QUICKSAVE_TEXTSHOWTIMERID 1u

static bool JonesMain_bWndMsgProcessed;

// The dev dialog functions live in JonesDevDialogWin32.c.
INT_PTR CALLBACK JonesMain_DevDialogProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
void J3DAPI JonesMain_DevDialogUpdateRadioButtons(HWND hDlg, const JonesState* pState);
int J3DAPI JonesMain_InitDevDialog(HWND hDlg, WPARAM wParam, JonesState* pConfig);
void J3DAPI JonesMain_DevDialogHandleCommand(HWND hWnd, int controlId, LPARAM lParam, int notifyCode);
void J3DAPI JonesMain_DevDialogInitDisplayDevices(HWND hDlg, JonesState* pConfig);

int J3DAPI JonesMain_GameWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, int* pReturnVal);
void J3DAPI JonesMain_HandleWMGetMinMaxInfo(HWND hwnd, LPMINMAXINFO pMinMaxInfo);


void J3DAPI JonesMain_HandleWMPaint(HWND hWnd);
void J3DAPI JonesMain_HandleWMTimer(HWND hWnd, WPARAM timerID);
void J3DAPI JonesMain_HandleWMKeydown(HWND hWnd, WPARAM vk, int a3, uint16_t repreatCount, uint16_t exkeyflags);
int J3DAPI JonesMain_HandleWMActivateApp(HWND hWnd, WPARAM wParam, LPARAM lParam);
LRESULT J3DAPI JonesMain_HandleWMActivate(HWND hWnd, uint16_t activateState, LPARAM hwndActivate, uint16_t minimizedState);
void J3DAPI JonesMain_HandleWMChar(HWND hwnd, char chr, uint16_t repeatCount);
void J3DAPI JonesMain_HandleWMSysCommand(HWND hWnd, WPARAM wParam, uint16_t curPosX, uint16_t curPosY);
void J3DAPI JonesMain_HandleWMPowerBroadcast(HWND hWnd, WPARAM wParam);

int J3DAPI JonesMain_IntroWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, int* pRetValue);
void J3DAPI JonesMain_IntroHandleWMLButtonUp(HWND hwnd, uint16_t curPosX, uint16_t curPosY, WPARAM mvk);
void J3DAPI JonesMain_IntroHandleWMKeydown(HWND hwnd, WPARAM vk, int a3, uint16_t repreatCount, uint16_t exkeyflags);

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
            // Fixed: Return non-zero if message was SC_KEYMENU to avoid passing wParam
            // to DefWindowProc which would attempt to open the (non-existent) window menu
            // on F10 and ALT press and eat player's input.
            if ( JonesMain_bWndMsgProcessed )
            {
                return wParam;
            }
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
    J3D_UNUSED(pReturnVal);
    return 0;
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
    if ( JonesMain_IsProcessingWindowEvents() )
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

void J3DAPI JonesMain_HandleWMTimer(HWND hWnd, WPARAM timerID)
{
    if ( timerID == JONES_QUICKSAVE_TEXTSHOWTIMERID ) // Added: Added timedID check
    {
        // TODO: If JonesMain_bWndMsgProcessed will be used by main wnd proc than it should be set to true here
        JonesMain_OnQuickSaveTextTimer();
        KillTimer(hWnd, JONES_QUICKSAVE_TEXTSHOWTIMERID);
    }
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
                JonesMain_OnQuickSaveKey(hWnd);
            }
        } break;
        case VK_F8: // Quick load
        {
            if ( repreatCount <= 1 && (exkeyflags & KF_REPEAT) == 0 )
            {
                JonesMain_OnQuickLoadKey();
            }
        } break;
        case VK_F12: // screenshot
        {
            if ( repreatCount <= 1 && (exkeyflags & KF_REPEAT) == 0 )
            {
                JonesMain_OnScreenshotKey();
            }
        } break;
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


void J3DAPI JonesMain_HandleWMChar(HWND hwnd, char chr, uint16_t repeatCount)
{
    J3D_UNUSED(hwnd);
    J3D_UNUSED(repeatCount);

    JonesMain_OnChar(chr);
}


void J3DAPI JonesMain_HandleWMSysCommand(HWND hWnd, WPARAM wParam, uint16_t curPosX, uint16_t curPosY)
{
    J3D_UNUSED(hWnd);
    J3D_UNUSED(curPosX);
    J3D_UNUSED(curPosY);

    // In WM_SYSCOMMAND messages, the four low-order bits of the wParam parameter
    // are used internally by the system. To obtain the correct result when testing
    // the value of wParam, an application must combine its value with 0xFFF0
    // by using the bitwise AND operator.
    wParam &= 0xFFF0;
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
            JonesMain_OnSystemSuspend(hWnd);
            STDLOG_DEBUG("WM_POWERBROADCAST Suspend.\n");
            break;

        case PBT_APMRESUMECRITICAL:
        case PBT_APMRESUMESUSPEND:
            JonesMain_OnSystemResume(hWnd);
            Sleep(100u);

            STDLOG_DEBUG("WM_POWERBROADCAST Resume.\n");
            break;
    }
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
    J3D_UNUSED(pRetValue);
    return 0;
#endif
}


void J3DAPI JonesMain_IntroHandleWMLButtonUp(HWND hwnd, uint16_t curPosX, uint16_t curPosY, WPARAM mvk)
{
    J3D_UNUSED(hwnd);
    J3D_UNUSED(curPosX);
    J3D_UNUSED(curPosY);
    J3D_UNUSED(mvk);
    JonesMain_OnSkipIntro();
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
            JonesMain_OnSkipIntro();
            break;

        case VK_F4:
            JonesMain_CloseWindow();
            break;

        case VK_F12:
            JonesMain_OnScreenshotKey();
            break;

        default:
            return;
    }
}


void JonesMainSys_StartQuickSaveTimer(tSysWindow hWnd)
{
    SetTimer(hWnd, JONES_QUICKSAVE_TEXTSHOWTIMERID, JONES_QUICKSAVE_TEXTSHOWTIME, NULL); // 1 sec
}

void JonesMainSys_ShowWindow(tSysWindow hWnd)
{
    ShowWindow(hWnd, SW_SHOW);
}

int JonesMainSys_PostCloseMessage(void)
{
    return PostMessage(stdWin95_GetWindow(), WM_CLOSE, 0, 0);
}

void JonesMainSys_UseGameWindowProc(void)
{
    wkernel_SetWindowProc(JonesMain_GameWndProc);
}

void JonesMainSys_UseIntroWindowProc(void)
{
    wkernel_SetWindowProc(JonesMain_IntroWndProc);
}

void JonesMainSys_ClearWindowProc(void)
{
    wkernel_SetWindowProc(NULL);
}

// Copies the computer name of the host into pDst of at most size characters, matching GetComputerName.
// Returns true on success, false when the name does not fit or the call failed.
bool JonesMainSys_GetComputerName(char* pDst, size_t size)
{
    DWORD nSize = (DWORD)size;
    return GetComputerName(pDst, &nSize) != 0;
}

// Closes a platform handle such as the one CreateMutex returned, matching CloseHandle.
// Returns true on success, false when the handle is invalid or already closed.
bool JonesMainSys_CloseHandle(tSysHandle handle)
{
    return CloseHandle(handle) != 0;
}

// Shows a modal message box with no owner window, matching the original MessageBox calls:
// always MB_TASKMODAL, plus MB_ICONSTOP when bIconStop. Returns the MessageBox result.
int JonesMainSys_ShowFatalMessage(const char* pText, const char* pTitle, bool bIconStop)
{
    UINT uType = MB_TASKMODAL | (bIconStop ? MB_ICONSTOP : 0);
    return MessageBox(NULL, pText, pTitle, uType);
}

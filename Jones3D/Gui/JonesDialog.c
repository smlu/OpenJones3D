#include "JonesDialog.h"
#include <j3dcore/j3dhook.h>

#include <Jones3D/Main/JonesMain.h>
#include <Jones3D/RTI/symbols.h>

#include <std/General/std.h>
#include <std/General/stdUtil.h>
#include <std/Win95/stdDisplay.h>
#include <std/Win95/stdControl.h>
#include <std/Win95/stdWin95.h>

static int JonesDialog_bWindowResized = 1;
static int JonesDialog_cursorPointIdx = 1;

static POINT JonesDialog_mousePos;
static JonesDialogGetDrawBufferDCFunc JonesDialog_pfGetDrawBufferDC;
static CHAR JonesDialog_szErrorBuffer[256];
static GDIDIBSectionInfo JonesDialog_GDIDIBSectionInfoOffScreen;
static JonesDialogReleaseDCFunc JonesDialog_pfReleaseDC;
static RECT JonesDialog_windowRect;
static RECT JonesDialog_windowPos[2];
static RECT JonesDialog_stru_553210;
static JonesDialogCursorInfo JonesDialog_cursorInfo;
static GDIDIBSectionInfo JonesDialog_GDIDIBSectionInfo;
static RECT JonesDialog_stru_5532D0;
static JonesDialogFlipPageFunc JonesDialog_pfFlipPage;
static int JonesDialog_dword_5532E4;
static int JonesDialog_bCursorTrackingInited;
static int JonesDialog_dword_5532EC;

void JonesDialog_InstallHooks(void)
{
    J3D_HOOKFUNC(JonesDialog_InitCursorTracking);
    J3D_HOOKFUNC(JonesDialog_TrackCursor);
    J3D_HOOKFUNC(JonesDialog_ResetCursorTracking);
    J3D_HOOKFUNC(JonesDialog_CopyRectToDrawBuffer);
    J3D_HOOKFUNC(JonesDialog_RestoreBackground);
    J3D_HOOKFUNC(JonesDialog_ShowDialog);
    J3D_HOOKFUNC(JonesDialog_SetGameState);
    J3D_HOOKFUNC(JonesDialog_ResetGameState);
    J3D_HOOKFUNC(JonesDialog_DummyFlipGDI);
    J3D_HOOKFUNC(JonesDialog_SubclassDialogProc);
    J3D_HOOKFUNC(JonesDialog_SubclassDialogWindowProc);
    J3D_HOOKFUNC(JonesDialog_HandleWM_ERASEBKGND);
    J3D_HOOKFUNC(JonesDialog_HandleWM_PAINT);
    J3D_HOOKFUNC(JonesDialog_HandleWM_NCPAINT);
    J3D_HOOKFUNC(JonesDialog_AllocScreenDBISection);
    J3D_HOOKFUNC(JonesDialog_AllocOffScreenGDISection);
    J3D_HOOKFUNC(JonesDialog_FreeScreenDIBSection);
    J3D_HOOKFUNC(JonesDialog_AllocOffScreenDIBSection);
    J3D_HOOKFUNC(JonesDialog_FreeOffScreenDIBSection);
    J3D_HOOKFUNC(JonesDialog_ShowFileSelectDialog);
    J3D_HOOKFUNC(JonesDialog_SubclassFileDialogProc);
}

void JonesDialog_ResetGlobals(void)
{

}

int J3DAPI JonesDialog_InitCursorTracking(int* pAlreadyInited)
{
    uint32_t dwError;
    HDC hDrawBufferDC;
    RECT recz;
    int height;
    int width;
    int canRenderWindowed;

    if ( JonesDialog_bCursorTrackingInited )
    {
        *pAlreadyInited = 1;
        return 0;
    }

    *pAlreadyInited = 0;
    memset(&JonesDialog_cursorInfo, 0, sizeof(JonesDialog_cursorInfo));

    canRenderWindowed = stdDisplay_CanRenderWindowed();
    if ( canRenderWindowed == -1 )
    {
        return 1;
    }

    if ( canRenderWindowed )
    {
        return 0;
    }

    JonesDialog_cursorInfo.hCursor = LoadCursor(NULL, (LPCSTR)IDC_ARROW);
    if ( !GetIconInfo(JonesDialog_cursorInfo.hCursor, &JonesDialog_cursorInfo.info) )// TODO: 2nd param should be made
    {
        STDLOG_ERROR("JonesDialog_InitCursorTracking: Error calling GetIconInfo().\n");
        JonesDialog_ResetCursorTracking();
        return 1;
    }

    hDrawBufferDC = JonesDialog_pfGetDrawBufferDC();
    JonesDialog_cursorInfo.hDrawDC = CreateCompatibleDC(hDrawBufferDC);
    JonesDialog_pfReleaseDC(hDrawBufferDC);

    if ( !JonesDialog_cursorInfo.hDrawDC )
    {
        STDLOG_ERROR("JonesDialog_InitCursorTracking: Error creating memory DC.\n");
        JonesDialog_ResetCursorTracking();
        return 1;
    }

    if ( JonesDialog_cursorInfo.info.hbmColor )
    {
        if ( !GetBitmapDimensionEx(JonesDialog_cursorInfo.info.hbmColor, &JonesDialog_cursorInfo.size) )
        {
            STDLOG_ERROR("JonesDialog_InitCursorTracking: Error calling GetBitmapDimensionEx() for color bitmap.\n");
            JonesDialog_ResetCursorTracking();
            return 1;
        }

        if ( SelectObject(JonesDialog_cursorInfo.hDrawDC, JonesDialog_cursorInfo.info.hbmColor) == (HGDIOBJ)HGDI_ERROR )
        {
            STDLOG_ERROR("JonesDialog_InitCursorTracking: Error calling SelectObject().\n");
            JonesDialog_ResetCursorTracking();
            return 1;
        }
    }
    else
    {
        if ( !JonesDialog_cursorInfo.info.hbmMask )
        {
            STDLOG_ERROR("JonesDialog_InitCursorTracking: What the heck kinda cursor is this?.\n");
            JonesDialog_ResetCursorTracking();
            return 1;
        }

        if ( !GetBitmapDimensionEx(JonesDialog_cursorInfo.info.hbmMask, &JonesDialog_cursorInfo.size) )
        {
            STDLOG_ERROR("JonesDialog_InitCursorTracking: Error calling GetBitmapDimensionEx() for color bitmap.\n");
            JonesDialog_ResetCursorTracking();
            return 1;
        }

        if ( JonesDialog_cursorInfo.size.cy )
        {
            JonesDialog_cursorInfo.size.cy /= 2;
        }

        if ( SelectObject(JonesDialog_cursorInfo.hDrawDC, JonesDialog_cursorInfo.info.hbmMask) == (HGDIOBJ)HGDI_ERROR )
        {
            STDLOG_ERROR("JonesDialog_InitCursorTracking: Error calling SelectObject().\n");
            JonesDialog_ResetCursorTracking();
            return 1;
        }
    }

    GetClipCursor(&JonesDialog_cursorInfo.rect);

    recz.left = 0;
    recz.top = 0;

    stdDisplay_GetBackBufferSize((unsigned int*)&width, (unsigned int*)&height);

    recz.right = width - 1;
    recz.bottom = height - 1;

    if ( !ClipCursor(&recz) )
    {
        dwError = GetLastError();
        FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, 0, dwError, 0, JonesDialog_szErrorBuffer, 255u, 0);
        STDLOG_ERROR("JonesDialog_InitCursorTracking: Error calling ClipCursor().  Reason: %s", JonesDialog_szErrorBuffer);
        JonesDialog_ResetCursorTracking();
        return 1;
    }

    JonesDialog_cursorInfo.size.cx = 32;
    JonesDialog_cursorInfo.size.cy = 32;

    JonesDialog_cursorInfo.point[0].x = -1;
    JonesDialog_cursorInfo.point[0].y = -1;

    JonesDialog_cursorInfo.point[1].x = -1;
    JonesDialog_cursorInfo.point[1].y = -1;

    JonesDialog_bCursorTrackingInited = 1;
    return 0;
}

int J3DAPI JonesDialog_TrackCursor(int bReDraw)
{
    HWND hwnd;
    uint32_t dwError;
    uint32_t LastError;
    uint32_t dwError_1;
    int i;
    RECT Rect;
    HDC hDrawBufferDC;
    RECT rect;
    POINT curPos;
    POINT screenPos;
    int v12;
    int CanRenderWindowed;

    if ( !JonesDialog_bCursorTrackingInited )
    {
        return 1;
    }

    if ( !JonesDialog_bWindowResized )
    {
        return 0;
    }

    CanRenderWindowed = stdDisplay_CanRenderWindowed();
    if ( CanRenderWindowed == -1 )
    {
        return 1;
    }

    if ( CanRenderWindowed )
    {
        return 0;
    }

    if ( !JonesDialog_cursorInfo.hDrawDC )
    {
        return 0;
    }

    screenPos.x = 0;
    screenPos.y = 0;
    v12 = 1;                                    // TODO: remove decomp. leftover

    GetCursorPos(&curPos);
    if ( curPos.x == JonesDialog_cursorInfo.point[1].x && curPos.y == JonesDialog_cursorInfo.point[1].y && !bReDraw )
    {
        return 0;
    }

    rect.left = JonesDialog_cursorInfo.point[JonesDialog_cursorPointIdx].x;
    rect.top = JonesDialog_cursorInfo.point[JonesDialog_cursorPointIdx].y;
    rect.right = JonesDialog_cursorInfo.size.cx + JonesDialog_cursorInfo.point[JonesDialog_cursorPointIdx].x;
    rect.bottom = JonesDialog_cursorInfo.size.cy + JonesDialog_cursorInfo.point[JonesDialog_cursorPointIdx].y;

    JonesDialog_CopyRectToDrawBuffer(
        &JonesDialog_GDIDIBSectionInfoOffScreen,
        &JonesDialog_cursorInfo.point[JonesDialog_cursorPointIdx],
        &rect);
    if ( JonesDialog_cursorInfo.numHandels )
    {
        for ( i = 0; i < JonesDialog_cursorInfo.numHandels; ++i )
        {
            GetWindowRect(JonesDialog_cursorInfo.aHwnd[i], &Rect);
            screenPos.x = Rect.left;
            screenPos.y = Rect.top;
            JonesDialog_CopyRectToDrawBuffer(&JonesDialog_GDIDIBSectionInfo, &screenPos, &Rect);
        }
    }

    JonesDialog_cursorInfo.point[0] = JonesDialog_cursorInfo.point[1];
    JonesDialog_cursorInfo.point[1] = curPos;
    JonesDialog_cursorInfo.hCursor = (HCURSOR)screenPos.x;// TODO: wtf???
    JonesDialog_cursorInfo.info.fIcon = screenPos.y;

    rect.left = curPos.x;
    rect.top = curPos.y;
    rect.right = JonesDialog_cursorInfo.size.cx + curPos.x;
    rect.bottom = JonesDialog_cursorInfo.size.cy + curPos.y;
    screenPos.x = 0;
    screenPos.y = 0;

    hwnd = stdWin95_GetWindow();
    ClientToScreen(hwnd, &screenPos);

    hDrawBufferDC = JonesDialog_pfGetDrawBufferDC();
    if ( !BitBlt(
        hDrawBufferDC,
        screenPos.x + rect.left,
        screenPos.y + rect.top,
        JonesDialog_cursorInfo.size.cx,
        JonesDialog_cursorInfo.size.cy,
        JonesDialog_cursorInfo.hDrawDC,
        0,
        0,
        SRCAND) )
    {
        dwError = GetLastError();
        FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, 0, dwError, 0, JonesDialog_szErrorBuffer, 255u, 0);
        STDLOG_ERROR("JonesDialog_TrackCursor: BitBlt of cursor, SRCAND failed.  Reason: %s", JonesDialog_szErrorBuffer);
    }

    if ( JonesDialog_cursorInfo.info.hbmColor )
    {
        SelectObject(JonesDialog_cursorInfo.hDrawDC, JonesDialog_cursorInfo.info.hbmColor);
        if ( !BitBlt(
            hDrawBufferDC,
            screenPos.x + rect.left,
            screenPos.y + rect.top,
            JonesDialog_cursorInfo.size.cx,
            JonesDialog_cursorInfo.size.cy,
            JonesDialog_cursorInfo.hDrawDC,
            0,
            0,
            SRCPAINT) )
        {
            LastError = GetLastError();
            FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, 0, LastError, 0, JonesDialog_szErrorBuffer, 0xFFu, 0);
            STDLOG_ERROR("JonesDialog_TrackCursor: BitBlt of cursor, SRCPAINT failed.  Reason: %s", JonesDialog_szErrorBuffer);
        }

        SelectObject(JonesDialog_cursorInfo.hDrawDC, JonesDialog_cursorInfo.info.hbmMask);
    }
    else if ( !BitBlt(
        hDrawBufferDC,
        screenPos.x + rect.left,
        screenPos.y + rect.top,
        JonesDialog_cursorInfo.size.cx,
        JonesDialog_cursorInfo.size.cy,
        JonesDialog_cursorInfo.hDrawDC,
        0,
        JonesDialog_cursorInfo.size.cy,
        SRCPAINT) )
    {
        dwError_1 = GetLastError();
        FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, 0, dwError_1, 0, JonesDialog_szErrorBuffer, 255u, 0);
        STDLOG_ERROR("JonesDialog_TrackCursor: BitBlt of cursor, SRCPAINT failed.  Reason: %s", JonesDialog_szErrorBuffer);
    }

    JonesDialog_pfReleaseDC(hDrawBufferDC);
    JonesDialog_pfFlipPage();
    return 0;
}

int J3DAPI JonesDialog_ResetCursorTracking()
{
    int bCanRenderWindowed;

    bCanRenderWindowed = stdDisplay_CanRenderWindowed();
    if ( bCanRenderWindowed == -1 )
    {
        return 1;
    }

    if ( !bCanRenderWindowed )
    {
        if ( JonesDialog_cursorInfo.info.hbmColor )
        {
            DeleteObject(JonesDialog_cursorInfo.info.hbmColor);
        }


        if ( JonesDialog_cursorInfo.info.hbmMask )
        {
            DeleteObject(JonesDialog_cursorInfo.info.hbmMask);
        }


        if ( JonesDialog_cursorInfo.hDrawDC )
        {
            DeleteDC(JonesDialog_cursorInfo.hDrawDC);
        }


        ClipCursor(&JonesDialog_cursorInfo.rect);
    }

    memset(&JonesDialog_cursorInfo, 0, sizeof(JonesDialog_cursorInfo));
    JonesDialog_bCursorTrackingInited = 0;
    return 0;
}

int J3DAPI JonesDialog_CopyRectToDrawBuffer(GDIDIBSectionInfo* pInfo, const PPOINT lpSourceOrigin, LPRECT pDstRect)
{
    uint32_t dwError;
    uint32_t dwError_1;
    HDC hDC;
    HDC hDrawBufferDC;

    if ( lpSourceOrigin )
    {
        hDrawBufferDC = JonesDialog_pfGetDrawBufferDC();
        if ( hDrawBufferDC )
        {
            if ( pDstRect->left < pDstRect->right && pDstRect->top < pDstRect->bottom )
            {
                if ( pInfo == &JonesDialog_GDIDIBSectionInfo && pInfo->bHasDC )
                {
                    hDC = GetDC(0);
                    if ( !BitBlt(
                        JonesDialog_GDIDIBSectionInfo.hScreenDC,
                        pDstRect->left,
                        pDstRect->top,
                        pDstRect->right - pDstRect->left,
                        pDstRect->bottom - pDstRect->top,
                        hDC,
                        lpSourceOrigin->x,
                        lpSourceOrigin->y,
                        SRCCOPY) )
                    {
                        dwError = GetLastError();
                        FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, 0, dwError, 0, JonesDialog_szErrorBuffer, 255u, 0);
                        STDLOG_ERROR("JonesDialog_CopyRectToDrawBuffer: BitBlt to virtual desktop failed.  Reason: %s", JonesDialog_szErrorBuffer);
                    }

                    ReleaseDC(0, hDC);
                }

                if ( !BitBlt(
                    hDrawBufferDC,
                    pDstRect->left,
                    pDstRect->top,
                    pDstRect->right - pDstRect->left,
                    pDstRect->bottom - pDstRect->top,
                    pInfo->hScreenDC,
                    lpSourceOrigin->x,
                    lpSourceOrigin->y,
                    SRCCOPY) )
                {
                    dwError_1 = GetLastError();
                    FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, 0, dwError_1, 0, JonesDialog_szErrorBuffer, 255u, 0);
                    STDLOG_ERROR("JonesDialog_CopyRectToDrawBuffer: BitBlt failed.  Reason: %s", JonesDialog_szErrorBuffer);
                }

                JonesDialog_pfReleaseDC(hDrawBufferDC);
                JonesDialog_pfFlipPage(); // Added
                return 1;
            }
            else
            {
                JonesDialog_pfReleaseDC(hDrawBufferDC);
                return 0;
            }
        }
        else
        {
            STDLOG_ERROR("JonesDialog_CopyRectToDrawBuffer: hDrawBufferDC is NULL.\n");
            return 0;
        }
    }
    else
    {
        STDLOG_ERROR("JonesDialog_CopyRectToDrawBuffer: lpSourceOrigin is NULL.\n");
        return 0;
    }
}

int J3DAPI JonesDialog_RestoreBackground(HDC hdc, HWND hwnd, LPPOINT a3, LPRECT a4)
{
    J3D_UNUSED(hwnd);
    uint32_t dwError;

    if ( !JonesDialog_GDIDIBSectionInfoOffScreen.hScreenDC || !a3 || !a4 )
    {
        return 0;
    }

    if ( BitBlt(
        hdc,
        a4->left,
        a4->top,
        a4->right - a4->left + 1,
        a4->bottom - a4->top + 1,
        JonesDialog_GDIDIBSectionInfoOffScreen.hScreenDC,
        a3->x + a4->left,
        a3->y + a4->top,
        SRCCOPY) )
    {
        JonesDialog_pfFlipPage(); // Added
        return 1;
    }

    dwError = GetLastError();
    FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, 0, dwError, 0, JonesDialog_szErrorBuffer, 255u, 0);
    STDLOG_ERROR("JonesDialog_RestoreBackground: BitBlt failed.  Reason: %s", JonesDialog_szErrorBuffer);
    return 1;
}

int J3DAPI JonesDialog_ShowDialog(LPCSTR lpTemplateName, HWND hWnd, DLGPROC lpDialogFunc, LPARAM dwInitParam)
{
    int bWindowModeSupported;
    JonesDialogData data;
    HINSTANCE hInstance;
    JonesDialogGameState state;
    int v9;

    hInstance = (HINSTANCE)GetWindowLong(hWnd, GWL_HINSTANCE);
    if ( !JonesMain_HasStarted() )
    {
        return DialogBoxParam(hInstance, lpTemplateName, hWnd, lpDialogFunc, dwInitParam);
    }

    memset(&data, 0, sizeof(data));
    data.dwSize = sizeof(JonesDialogData);
    data.lpfnPrevHook = (LPOFNHOOKPROC)lpDialogFunc;
    data.lPrevCustData = dwInitParam;

    bWindowModeSupported = stdDisplay_CanRenderWindowed();
    if ( bWindowModeSupported == -1 )
    {
        return -1;
    }

    JonesDialog_SetGameState(&state, bWindowModeSupported);
    if ( bWindowModeSupported )
    {
        v9 = DialogBoxParam(hInstance, lpTemplateName, hWnd, lpDialogFunc, dwInitParam);
    }
    else
    {
        v9 = DialogBoxParam(hInstance, lpTemplateName, hWnd, JonesDialog_SubclassDialogProc, (LPARAM)&data);
    }

    JonesDialog_ResetGameState(&state, bWindowModeSupported);
    return v9;
}

void J3DAPI JonesDialog_SetGameState(JonesDialogGameState* pState, int bWindowModeSupported)
{
    uint32_t dwError;
    uint32_t dwError_1;
    HDC hDC;
    RECT pDstRect;
    unsigned int height;
    POINT SourceOrigin;
    unsigned int width;

    SourceOrigin.x = 0;
    SourceOrigin.y = 0;
    pDstRect.left = 0;
    pDstRect.top = 0;

    stdDisplay_GetBackBufferSize(&width, &height);

    pDstRect.right = width;
    pDstRect.bottom = height;

    if ( bWindowModeSupported )
    {
        JonesDialog_pfGetDrawBufferDC = stdDisplay_GetFrontBufferDC;
        JonesDialog_pfReleaseDC = stdDisplay_ReleaseFrontBufferDC;
        JonesDialog_cursorPointIdx = 1;
        if ( stdDisplay_IsFullscreen() )
        {
            JonesDialog_pfFlipPage = stdDisplay_FlipToGDISurface;
            stdDisplay_FlipToGDISurface();
        }
        else
        {
            //JonesDialog_pfFlipPage = JonesDialog_DummyFlipGDI;
            JonesDialog_pfFlipPage = stdDisplay_Update; // Changed: Fixes game background rendering when dialog is open
        }
    }
    else
    {
        JonesDialog_pfGetDrawBufferDC = stdDisplay_GetBackBufferDC;
        JonesDialog_pfReleaseDC = stdDisplay_ReleaseBackBufferDC;
        JonesDialog_cursorPointIdx = 0;
        JonesDialog_pfFlipPage = stdDisplay_Update;
    }

    if ( JonesDialog_AllocOffScreenDIBSection(&pState->bOffScreenDBIAllocSkipped) )
    {
        JonesDialog_FreeOffScreenDIBSection();
    }

    else if ( JonesDialog_AllocScreenDBISection(&pState->bScreenDBIAllocSkipped) )
    {
        JonesDialog_FreeScreenDIBSection();
    }
    else
    {
        pState->unknown7 = JonesDialog_dword_5532E4;

        if ( !JonesDialog_dword_5532E4 && !bWindowModeSupported )
        {
            JonesDialog_CopyRectToDrawBuffer(&JonesDialog_GDIDIBSectionInfoOffScreen, &SourceOrigin, &pDstRect);
            JonesDialog_pfFlipPage();
            JonesDialog_CopyRectToDrawBuffer(&JonesDialog_GDIDIBSectionInfoOffScreen, &SourceOrigin, &pDstRect);
            JonesDialog_dword_5532E4 = 1;
        }

        if ( !pState->bScreenDBIAllocSkipped )
        {
            if ( !BitBlt(
                JonesDialog_GDIDIBSectionInfo.hScreenDC,
                0,
                0,
                width,
                height,
                JonesDialog_GDIDIBSectionInfoOffScreen.hScreenDC,
                0,
                0,
                SRCCOPY) )
            {
                dwError = GetLastError();
                FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, 0, dwError, 0, JonesDialog_szErrorBuffer, 255u, 0);
                STDLOG_ERROR("JonesDialog_SetGameState: BitBlt of background to virtual desktop failed.  Reason: %s", JonesDialog_szErrorBuffer);
            }

            hDC = GetDC(0);
            if ( hDC != JonesDialog_GDIDIBSectionInfo.hScreenDC
              && !BitBlt(hDC, 0, 0, width, height, JonesDialog_GDIDIBSectionInfoOffScreen.hScreenDC, 0, 0, SRCCOPY) )
            {
                dwError_1 = GetLastError();
                FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, 0, dwError_1, 0, JonesDialog_szErrorBuffer, 255u, 0);
                STDLOG_ERROR("JonesDialog_SetGameState: BitBlt of background to actual desktop failed.  Reason: %s", JonesDialog_szErrorBuffer);
            }

            ReleaseDC(0, hDC);
        }

        JonesDialog_pfFlipPage();

        pState->bGamePaused = JonesMain_IsGamePaused();
        if ( !pState->bGamePaused )
        {
            JonesMain_PauseGame();
        }

        pState->bControlsActive = stdControl_ControlsActive();
        if ( pState->bControlsActive && !stdControl_SetActivation(0) )
        {
            stdControl_ShowMouseCursor(1);
        }

        JonesDialog_InitCursorTracking(&pState->unknown4);
        pState->bFrontBufferClipper = stdDisplay_SetBufferClipper(1);
        if ( !bWindowModeSupported )
        {
            pState->bBackBufferClipper = stdDisplay_SetBufferClipper(0);
        }
    }
}

void J3DAPI JonesDialog_ResetGameState(JonesDialogGameState* pState, int bWindowModeSupported)
{
    if ( !pState->bFrontBufferClipper )
    {
        stdDisplay_RemoveBufferClipper(1);
    }

    if ( !bWindowModeSupported && !pState->bBackBufferClipper )
    {
        stdDisplay_RemoveBufferClipper(0);
    }

    if ( !pState->unknown4 )
    {
        JonesDialog_ResetCursorTracking();
    }

    if ( !pState->bOffScreenDBIAllocSkipped )
    {
        JonesDialog_FreeOffScreenDIBSection();
    }

    if ( !pState->bScreenDBIAllocSkipped )
    {
        JonesDialog_FreeScreenDIBSection();
    }

    stdControl_SetActivation(pState->bControlsActive);
    if ( !stdControl_SetActivation(pState->bControlsActive) )
    {
        if ( pState->bControlsActive )
        {
            stdControl_ShowMouseCursor(0);
        }
        else
        {
            stdControl_ShowMouseCursor(1);
        }
    }

    JonesDialog_dword_5532E4 = pState->unknown7;
    if ( !pState->bGamePaused )
    {
        JonesMain_ResumeGame();
    }
}

int JonesDialog_DummyFlipGDI(void)
{
    return 1;
}

INT_PTR CALLBACK JonesDialog_SubclassDialogProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    uint32_t height;
    int y;
    POINT SourceOrigin;
    int x;
    JonesDialogData* lpData;
    WNDPROC lpPrevWndFunc;
    RECT rect;
    uint32_t width;

    lpPrevWndFunc = 0;
    lpData = (JonesDialogData*)GetWindowLong(hwnd, GWL_USERDATA);
    if ( lpData && lpData->dwSize == sizeof(JonesDialogData) )
    {
        lpPrevWndFunc = (WNDPROC)lpData->lpfnPrevHook;
    }

    if ( uMsg > WM_INITDIALOG )
    {
        if ( uMsg == WM_TIMER )
        {
            JonesDialog_TrackCursor(1);
        }
    }
    else if ( uMsg == WM_INITDIALOG )
    {
        SetWindowLong(hwnd, GWL_USERDATA, lParam);// TODO: cast lParam which is JonesDialogData  here
        lpData = (JonesDialogData*)lParam;
        if ( lParam && lpData->dwSize == sizeof(JonesDialogData) )
        {
            lpPrevWndFunc = (WNDPROC)lpData->lpfnPrevHook;
            lpData->lpfnWndProc = (WNDPROC)GetWindowLong(hwnd, GWL_WNDPROC);
            lpData->lpUserData = (LPVOID)GetWindowLong(hwnd, GWL_USERDATA);
            SetWindowLong(hwnd, GWL_WNDPROC, (LONG)JonesDialog_SubclassDialogWindowProc);
            lParam = lpData->lPrevCustData;
        }

        GetWindowRect(hwnd, &rect);
        stdDisplay_GetBackBufferSize(&width, &height);
        x = (width >> 1) - (rect.right - rect.left + 1) / 2;
        y = (height >> 1) - (rect.bottom - rect.top + 1) / 2;
        MoveWindow(hwnd, x, y, rect.right - rect.left + 1, rect.bottom - rect.top + 1, 1);

        if ( lpData && lpData->dwSize == sizeof(JonesDialogData) )
        {
            GetWindowRect(hwnd, &lpData->rcWindow);
        }

        if ( JonesDialog_cursorInfo.numHandels >= STD_ARRAYLEN(JonesDialog_cursorInfo.aHwnd) )
        {
            STDLOG_ERROR("JonesDialog_SubclassDialogProc: Too many dialogs displayed!\n");
        }
        else
        {
            JonesDialog_cursorInfo.aHwnd[JonesDialog_cursorInfo.numHandels++] = hwnd;
        }

        SetTimer(hwnd, 1u, 33u, 0);
    }

    else if ( uMsg == WM_DESTROY )
    {
        if ( !stdDisplay_CanRenderWindowed() )
        {
            GetWindowRect(hwnd, &rect);
            SourceOrigin.x = rect.left;
            SourceOrigin.y = rect.top;
            JonesDialog_CopyRectToDrawBuffer(&JonesDialog_GDIDIBSectionInfoOffScreen, &SourceOrigin, &rect);
            JonesDialog_pfFlipPage();
            JonesDialog_CopyRectToDrawBuffer(&JonesDialog_GDIDIBSectionInfoOffScreen, &SourceOrigin, &rect);
        }

        if ( JonesDialog_cursorInfo.numHandels )
        {
            JonesDialog_cursorInfo.aHwnd[--JonesDialog_cursorInfo.numHandels] = 0;
        }

        lpData = 0;
        JonesDialog_TrackCursor(1);
        if ( !JonesDialog_cursorInfo.numHandels )
        {
            KillTimer(hwnd, 1u);
        }
    }
    else if ( uMsg == WM_MOVE && lpData && lpData->dwSize == sizeof(JonesDialogData) )
    {
        GetWindowRect(hwnd, &lpData->rcWindow);
    }

    if ( lpPrevWndFunc )
    {
        return CallWindowProc(lpPrevWndFunc, hwnd, uMsg, wParam, lParam);
    }
    else
    {
        return 0;
    }
}

LRESULT CALLBACK JonesDialog_SubclassDialogWindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    LRESULT result;
    uint32_t dwError;
    HWND Parent;
    LONG top;
    LONG v8;
    LONG left;
    LONG v10;
    int v11;
    int v12;
    int i;
    HDC hDrawBufferDC;
    LONG pHeight;
    struct tagPOINT screenPos;
    JonesDialogData* lpSubclassData;
    WNDPROC lpPrevWndFunc;
    LONG pWidth;
    int bCanRenderWindowed;

    lpPrevWndFunc = 0;
    lpSubclassData = (JonesDialogData*)GetWindowLong(hwnd, GWL_USERDATA);
    if ( lpSubclassData && lpSubclassData->dwSize == sizeof(JonesDialogData) )
    {
        lpPrevWndFunc = lpSubclassData->lpfnWndProc;
        if ( uMsg <= WM_PAINT )
        {
            switch ( uMsg )
            {
                case WM_PAINT:
                    JonesDialog_HandleWM_PAINT(hwnd, lpSubclassData);
                    return 0;

                case WM_CREATE:
                    GetCursorPos(&JonesDialog_mousePos);
                    break;

                case WM_DESTROY:
                    lpSubclassData = 0;
                    JonesDialog_TrackCursor(1);
                    break;

                case WM_MOVE:
                    if ( lpSubclassData->pExtraData )
                    {
                        Parent = GetParent(hwnd);
                        GetWindowRect(Parent, &lpSubclassData->rcWindow);
                    }
                    else
                    {
                        GetWindowRect(hwnd, &lpSubclassData->rcWindow);
                    }

                    break;

                default:
                    goto LABEL_64;
            }

            return CallWindowProc(lpPrevWndFunc, hwnd, uMsg, wParam, lParam);
        }

        if ( uMsg <= WM_ICONERASEBKGND )
        {
            if ( uMsg == WM_ICONERASEBKGND )
            {
                JonesDialog_HandleWM_ERASEBKGND(hwnd, wParam, 1, lpSubclassData);
                return 0;
            }


            if ( uMsg == WM_ERASEBKGND )
            {
                JonesDialog_HandleWM_ERASEBKGND(hwnd, wParam, 0, lpSubclassData);
                return 0;
            }

            goto LABEL_64;
        }

        if ( uMsg <= WM_NCLBUTTONDOWN )
        {
            if ( uMsg >= WM_NCMOUSEMOVE )
            {
            LABEL_23:
                JonesDialog_mousePos.x = (uint16_t)lParam;
                JonesDialog_mousePos.y = HIWORD(lParam);
                return CallWindowProc(lpPrevWndFunc, hwnd, uMsg, wParam, lParam);
            }

            if ( uMsg == WM_NCPAINT )
            {
                JonesDialog_HandleWM_NCPAINT(hwnd, wParam, lpSubclassData);
                return 0;
            }

        LABEL_64:
            if ( lpPrevWndFunc )
            {
                return CallWindowProc(lpPrevWndFunc, hwnd, uMsg, wParam, lParam);
            }

            return CallWindowProc(lpPrevWndFunc, hwnd, uMsg, wParam, lParam); // TODO: this stinks, lpPrevWndFunc is null here
        }

        switch ( uMsg )
        {
            case WM_MOUSEFIRST:
            case WM_LBUTTONDOWN:
                goto LABEL_23;

            case WM_MOVING:
                bCanRenderWindowed = stdDisplay_CanRenderWindowed();
                if ( bCanRenderWindowed == -1 || bCanRenderWindowed || !JonesDialog_bCursorTrackingInited )
                {
                    return CallWindowProc(lpPrevWndFunc, hwnd, uMsg, wParam, lParam);
                }

                screenPos.x = JonesDialog_windowPos[JonesDialog_cursorPointIdx].left;
                screenPos.y = JonesDialog_windowPos[JonesDialog_cursorPointIdx].top;

                JonesDialog_CopyRectToDrawBuffer(
                    &JonesDialog_GDIDIBSectionInfoOffScreen,
                    &screenPos,
                    &JonesDialog_windowPos[JonesDialog_cursorPointIdx]);

                if ( JonesDialog_cursorInfo.numHandels )
                {
                    for ( i = 0; i < JonesDialog_cursorInfo.numHandels - 1; ++i )
                    {
                        GetWindowRect(JonesDialog_cursorInfo.aHwnd[i], &JonesDialog_windowRect);
                        screenPos.x = JonesDialog_windowRect.left;
                        screenPos.y = JonesDialog_windowRect.top;
                        JonesDialog_CopyRectToDrawBuffer(
                            &JonesDialog_GDIDIBSectionInfo,
                            &screenPos,
                            &JonesDialog_windowRect);
                    }
                }

                lpSubclassData->rcWindow = *(RECT*)lParam;
                screenPos.x = 0;
                screenPos.y = 0;

                ClientToScreen(hwnd, &screenPos);
                GetWindowRect(hwnd, &JonesDialog_windowRect);

                screenPos.x = JonesDialog_windowRect.left;
                screenPos.y = JonesDialog_windowRect.top;

                JonesDialog_CopyRectToDrawBuffer(&JonesDialog_GDIDIBSectionInfo, &screenPos, (LPRECT)lParam);
                GetCursorPos(&screenPos);

                hDrawBufferDC = JonesDialog_pfGetDrawBufferDC();
                BitBlt(
                    hDrawBufferDC,
                    screenPos.x,
                    screenPos.y,
                    JonesDialog_cursorInfo.size.cx,
                    JonesDialog_cursorInfo.size.cy,
                    JonesDialog_cursorInfo.hDrawDC,
                    0,
                    0,
                    SRCAND);

                if ( JonesDialog_cursorInfo.info.hbmColor )
                {
                    SelectObject(JonesDialog_cursorInfo.hDrawDC, JonesDialog_cursorInfo.info.hbmColor);

                    BitBlt(
                        hDrawBufferDC,
                        screenPos.x,
                        screenPos.y,
                        JonesDialog_cursorInfo.size.cx,
                        JonesDialog_cursorInfo.size.cy,
                        JonesDialog_cursorInfo.hDrawDC,
                        0,
                        0,
                        SRCPAINT);

                    SelectObject(JonesDialog_cursorInfo.hDrawDC, JonesDialog_cursorInfo.info.hbmMask);
                }
                else
                {
                    BitBlt(
                        hDrawBufferDC,
                        screenPos.x,
                        screenPos.y,
                        JonesDialog_cursorInfo.size.cx,
                        JonesDialog_cursorInfo.size.cy,
                        JonesDialog_cursorInfo.hDrawDC,
                        0,
                        JonesDialog_cursorInfo.size.cy,
                        SRCPAINT);
                }

                JonesDialog_pfReleaseDC(hDrawBufferDC);

                JonesDialog_windowPos[0] = JonesDialog_windowPos[1];
                JonesDialog_windowPos[1].left = *(uint32_t*)lParam;// lParam is RECT here
                JonesDialog_windowPos[1].top = *(uint32_t*)(lParam + 4);
                JonesDialog_windowPos[1].right = *(uint32_t*)(lParam + 8);
                JonesDialog_windowPos[1].bottom = *(uint32_t*)(lParam + 12);

                JonesDialog_pfFlipPage();
                result = 1;
                break;

            case WM_ENTERSIZEMOVE:
                bCanRenderWindowed = stdDisplay_CanRenderWindowed();
                if ( bCanRenderWindowed == -1 || bCanRenderWindowed )
                {
                    return CallWindowProc(lpPrevWndFunc, hwnd, uMsg, wParam, lParam);
                }

                JonesDialog_bWindowResized = 0;
                GetWindowRect(hwnd, &JonesDialog_stru_553210);
                GetWindowRect(hwnd, JonesDialog_windowPos);
                GetWindowRect(hwnd, &JonesDialog_windowPos[1]);

                JonesDialog_stru_553210.right -= JonesDialog_stru_553210.left;
                JonesDialog_stru_553210.left = 0;
                JonesDialog_stru_553210.bottom -= JonesDialog_stru_553210.top;
                JonesDialog_stru_553210.top = 0;
                JonesDialog_stru_5532D0.left = 0;
                JonesDialog_stru_5532D0.top = 0;

                stdDisplay_GetBackBufferSize((unsigned int*)&pWidth, (unsigned int*)&pHeight);
                JonesDialog_stru_5532D0.right = pWidth;
                JonesDialog_stru_5532D0.bottom = pHeight;
                v12 = JonesDialog_stru_5532D0.right >= GetDeviceCaps(JonesDialog_GDIDIBSectionInfo.hScreenDC, HORZRES)
                    ? GetDeviceCaps(JonesDialog_GDIDIBSectionInfo.hScreenDC, 8)
                    : JonesDialog_stru_5532D0.right;

                JonesDialog_stru_5532D0.right = v12 - 1;
                v11 = JonesDialog_stru_5532D0.bottom >= GetDeviceCaps(JonesDialog_GDIDIBSectionInfo.hScreenDC, 10)
                    ? GetDeviceCaps(JonesDialog_GDIDIBSectionInfo.hScreenDC, 10)
                    : JonesDialog_stru_5532D0.bottom;

                JonesDialog_stru_5532D0.left = JonesDialog_mousePos.x - JonesDialog_windowPos[0].left + 1;
                JonesDialog_stru_5532D0.top = JonesDialog_mousePos.y - JonesDialog_windowPos[0].top + 1;
                JonesDialog_stru_5532D0.right -= JonesDialog_windowPos[0].right - JonesDialog_mousePos.x - 1;
                JonesDialog_stru_5532D0.bottom = v11 - (JonesDialog_windowPos[0].bottom - JonesDialog_mousePos.y);

                if ( JonesDialog_mousePos.x < JonesDialog_stru_5532D0.left )
                {
                    left = JonesDialog_stru_5532D0.left;
                }
                else
                {
                    v10 = JonesDialog_mousePos.x > JonesDialog_stru_5532D0.right
                        ? JonesDialog_stru_5532D0.right
                        : JonesDialog_mousePos.x;
                    left = v10;
                }

                JonesDialog_mousePos.x = left;
                if ( JonesDialog_mousePos.y < JonesDialog_stru_5532D0.top )
                {
                    top = JonesDialog_stru_5532D0.top;
                }
                else
                {
                    v8 = JonesDialog_mousePos.y > JonesDialog_stru_5532D0.bottom
                        ? JonesDialog_stru_5532D0.bottom
                        : JonesDialog_mousePos.y;
                    top = v8;
                }

                JonesDialog_mousePos.y = top;
                SetCursorPos(JonesDialog_mousePos.x, top);
                if ( ClipCursor(&JonesDialog_stru_5532D0) )
                {
                    return CallWindowProc(lpPrevWndFunc, hwnd, uMsg, wParam, lParam);
                }

                dwError = GetLastError();
                FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, 0, dwError, 0, JonesDialog_szErrorBuffer, 255u, 0);
                STDLOG_ERROR("JonesDialog_SubclassDialogWindowProc: Error calling ClipCursor().  Reason: %s", JonesDialog_szErrorBuffer);

                result = 1;
                break;

            case WM_EXITSIZEMOVE:
                GetWindowRect(hwnd, &lpSubclassData->rcWindow);
                JonesDialog_bWindowResized = 1;
                result = 1;
                break;

            default:
                goto LABEL_64;
        }
    }
    else
    {
        STDLOG_ERROR("JonesDialog_SubclassDialogWindowProc: Invalid lpSubclassData.\n");
        return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }

    return result;
}

void J3DAPI JonesDialog_HandleWM_ERASEBKGND(HWND hwnd, WPARAM wParam, int bIconEraseBkgnd, JonesDialogData* pData)
{
    struct tagPOINT SourceOrigin;
    WNDPROC lpfnWndProc;

    lpfnWndProc = 0;
    if ( pData && pData->dwSize == sizeof(JonesDialogData) )
    {
        lpfnWndProc = pData->lpfnWndProc;
        if ( lpfnWndProc )
        {
            if ( bIconEraseBkgnd )
            {
                lpfnWndProc(hwnd, WM_ICONERASEBKGND, wParam, 0);
            }
            else
            {
                lpfnWndProc(hwnd, WM_ERASEBKGND, wParam, 0);
            }
        }

        SourceOrigin.x = pData->rcWindow.left;
        SourceOrigin.y = pData->rcWindow.top;

        if ( JonesDialog_GDIDIBSectionInfo.hScreenDC )
        {
            JonesDialog_CopyRectToDrawBuffer(&JonesDialog_GDIDIBSectionInfo, &SourceOrigin, &pData->rcWindow);
        }
        else
        {
            STDLOG_ERROR("JonesDialog_HandleWM_ERASEBKGND: GDIDIBSectionInfo.hDC is NULL.\n");
        }
    }
}

void J3DAPI JonesDialog_HandleWM_PAINT(HWND hWnd, JonesDialogData* lpData)
{
    POINT origin;
    WNDPROC lpfnWndProc;

    lpfnWndProc = 0;
    if ( lpData && lpData->dwSize == sizeof(JonesDialogData) )
    {
        lpfnWndProc = lpData->lpfnWndProc;
        if ( lpfnWndProc )
        {
            lpfnWndProc(hWnd, WM_PAINT, 0, 0);
        }

        if ( JonesDialog_GDIDIBSectionInfo.hScreenDC )
        {
            origin.x = lpData->rcWindow.left;
            origin.y = lpData->rcWindow.top;
            JonesDialog_CopyRectToDrawBuffer(&JonesDialog_GDIDIBSectionInfo, &origin, &lpData->rcWindow);
            JonesDialog_TrackCursor(1);
        }
        else
        {
            STDLOG_ERROR("JonesDialog_HandleWM_PAINT: hScreenDC is NULL.\n");
        }
    }
}

void J3DAPI JonesDialog_HandleWM_NCPAINT(HWND hwnd, WPARAM wParam, JonesDialogData* lpData)
{
    uint32_t dwError;
    struct tagPOINT SourceOrigin;
    WNDPROC lpfnWndProc;

    lpfnWndProc = 0;
    if ( lpData && lpData->dwSize == sizeof(JonesDialogData) )
    {
        lpfnWndProc = lpData->lpfnWndProc;
        if ( lpfnWndProc )
        {
            lpfnWndProc(hwnd, WM_NCPAINT, wParam, 0);
        }

        if ( JonesDialog_GDIDIBSectionInfo.hScreenDC )
        {
            SourceOrigin.x = lpData->rcWindow.left;
            SourceOrigin.y = lpData->rcWindow.top;
            JonesDialog_CopyRectToDrawBuffer(&JonesDialog_GDIDIBSectionInfo, &SourceOrigin, &lpData->rcWindow);
            JonesDialog_TrackCursor(1);
        }
        else
        {
            dwError = GetLastError();
            FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, 0, dwError, 0, JonesDialog_szErrorBuffer, 0xFFu, 0);
            STDLOG_ERROR("JonesDialog_HandleWM_NCPAINT: GDIDIBSectionInfo.hDC is NULL.  Reason: %s\n", JonesDialog_szErrorBuffer);
        }
    }
}

int J3DAPI JonesDialog_AllocScreenDBISection(int* pbSkippedAllocation)
{
    int bOffScreen;
    HDC hdcFront;
    HDC hdc;
    HDC hDC;
    StdVideoMode pDisplayMode;

    if ( stdDisplay_GetCurrentVideoMode(&pDisplayMode) )
    {
        return 1;
    }

    hdc = GetDC(0);
    bOffScreen = !stdDisplay_CanRenderWindowed()
        && pDisplayMode.rasterInfo.colorInfo.bpp == 16
        && GetDeviceCaps(hdc, BITSPIXEL) == 16;

    if ( bOffScreen )
    {
        hdcFront = stdDisplay_GetFrontBufferDC();
        ReleaseDC(0, hdc);
        hDC = CreateCompatibleDC(hdcFront);
        stdDisplay_ReleaseFrontBufferDC(hdcFront);
        if ( JonesDialog_AllocOffScreenGDISection(&JonesDialog_GDIDIBSectionInfo, hDC, bOffScreen, pbSkippedAllocation) )
        {
            return 1;
        }
    }
    else
    {
        memset(&JonesDialog_GDIDIBSectionInfo, 0, sizeof(JonesDialog_GDIDIBSectionInfo));
        JonesDialog_GDIDIBSectionInfo.hScreenDC = hdc;
    }

    JonesDialog_GDIDIBSectionInfo.bHasDC = bOffScreen;
    if ( bOffScreen )
    {
        return JonesDialog_GDIDIBSectionInfo.hBitmap == 0;
    }
    else
    {
        return JonesDialog_GDIDIBSectionInfo.hScreenDC == 0;
    }
}

int J3DAPI JonesDialog_AllocOffScreenGDISection(GDIDIBSectionInfo* pInfo, HDC hdc, int b24bpp, int* pBSkipedAllocation)
{
    uint32_t dwError;
    StdVideoMode pDisplayMode;

    stdDisplay_GetCurrentVideoMode(&pDisplayMode);
    if ( pInfo->hBitmap )
    {
        if ( pBSkipedAllocation )
        {
            *pBSkipedAllocation = 1;
        }

        return 0;
    }
    else
    {
        *pBSkipedAllocation = 0;
        if ( stdDisplay_GetCurrentVideoMode(&pDisplayMode) )
        {
            return 1;
        }
        else
        {
            memset(&pInfo->bmpInfo, 0, sizeof(pInfo->bmpInfo));

            static_assert(sizeof(pInfo->bmpInfo.bmiHeader) == 40, "sizeof(pInfo->bmpInfo.bmiHeader) == 40");
            pInfo->bmpInfo.bmiHeader.biSize   = sizeof(pInfo->bmpInfo.bmiHeader);
            pInfo->bmpInfo.bmiHeader.biWidth  = pDisplayMode.rasterInfo.width;
            pInfo->bmpInfo.bmiHeader.biHeight = pDisplayMode.rasterInfo.height;
            pInfo->bmpInfo.bmiHeader.biPlanes = 1;
            if ( b24bpp )
            {
                pInfo->bmpInfo.bmiHeader.biBitCount = 24;
            }
            else
            {
                pInfo->bmpInfo.bmiHeader.biBitCount = pDisplayMode.rasterInfo.colorInfo.bpp;
            }

            pInfo->bmpInfo.bmiHeader.biCompression = 0;
            pInfo->bmpInfo.bmiHeader.biSizeImage = ((unsigned int)pDisplayMode.rasterInfo.colorInfo.bpp >> 3)
                * 4
                * ((pDisplayMode.rasterInfo.width + 3) >> 2)
                * pDisplayMode.rasterInfo.height;

            pInfo->hScreenDC = hdc;
            pInfo->hBitmap   = CreateDIBSection(pInfo->hScreenDC, &pInfo->bmpInfo, 0, (void**)&pInfo->pPixels, 0, 0);
            if ( !pInfo->hBitmap )
            {
                dwError = GetLastError();
                FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, 0, dwError, 0, JonesDialog_szErrorBuffer, STD_ARRAYLEN(JonesDialog_szErrorBuffer), 0);
                STDLOG_ERROR("JonesDialog_AllocOffScreenGDISection: CreateDIBSection failed.  Reason: %s", JonesDialog_szErrorBuffer);
            }

            SelectObject(pInfo->hScreenDC, pInfo->hBitmap);
            return pInfo->hBitmap == 0;
        }
    }
}

void J3DAPI JonesDialog_FreeScreenDIBSection()
{
    if ( JonesDialog_GDIDIBSectionInfo.hScreenDC )
    {
        if ( JonesDialog_GDIDIBSectionInfo.bHasDC )
        {
            DeleteDC(JonesDialog_GDIDIBSectionInfo.hScreenDC);
        }
        else
        {
            ReleaseDC(0, JonesDialog_GDIDIBSectionInfo.hScreenDC);
        }
    }

    if ( JonesDialog_GDIDIBSectionInfo.hBitmap )
    {
        DeleteObject(JonesDialog_GDIDIBSectionInfo.hBitmap);
    }

    JonesDialog_GDIDIBSectionInfo.hScreenDC = NULL;
    JonesDialog_GDIDIBSectionInfo.hBitmap   = NULL;
    JonesDialog_GDIDIBSectionInfo.pPixels   = NULL;
}

int J3DAPI JonesDialog_AllocOffScreenDIBSection(int* pBSkipedAllocation)
{
    uint32_t LastError;
    HDC drawDC;
    StdVideoMode pDisplayMode;
    HDC screenDC;

    if ( stdDisplay_GetCurrentVideoMode(&pDisplayMode) )
    {
        return 1;
    }

    drawDC = stdDisplay_GetFrontBufferDC();
    screenDC = CreateCompatibleDC(drawDC);
    if ( JonesDialog_AllocOffScreenGDISection(&JonesDialog_GDIDIBSectionInfoOffScreen, screenDC, 0, pBSkipedAllocation) )
    {
        return 1;
    }

    JonesDialog_GDIDIBSectionInfoOffScreen.bHasDC = 1;
    if ( !*pBSkipedAllocation
      && !BitBlt(JonesDialog_GDIDIBSectionInfoOffScreen.hScreenDC, 0, 0, pDisplayMode.rasterInfo.width, pDisplayMode.rasterInfo.height, drawDC, 0, 0, SRCCOPY) )
    {
        LastError = GetLastError();
        FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, 0, LastError, 0, JonesDialog_szErrorBuffer, STD_ARRAYLEN(JonesDialog_szErrorBuffer), 0);
        STDLOG_ERROR("JonesDialog_AllocOffScreenDIBSection: BitBlt failed.  Reason: %s", JonesDialog_szErrorBuffer);
    }

    stdDisplay_ReleaseFrontBufferDC(drawDC);
    JonesDialog_pfFlipPage(); // Added: Fixes rendering of game background when dialog is open.
    return JonesDialog_GDIDIBSectionInfoOffScreen.hBitmap == 0;
}

int J3DAPI JonesDialog_FreeOffScreenDIBSection()
{
    if ( JonesDialog_GDIDIBSectionInfoOffScreen.hScreenDC )
    {
        DeleteDC(JonesDialog_GDIDIBSectionInfoOffScreen.hScreenDC);
    }

    if ( JonesDialog_GDIDIBSectionInfoOffScreen.hBitmap )
    {
        DeleteObject(JonesDialog_GDIDIBSectionInfoOffScreen.hBitmap);
    }

    JonesDialog_GDIDIBSectionInfoOffScreen.hScreenDC = 0;
    JonesDialog_GDIDIBSectionInfoOffScreen.hBitmap = 0;
    JonesDialog_GDIDIBSectionInfoOffScreen.pPixels = 0;
    return 0;
}

BOOL J3DAPI JonesDialog_ShowFileSelectDialog(LPOPENFILENAMEA pofn, int bOpen)
{
    if ( !JonesMain_HasStarted() )
    {
        return GetOpenFileName(pofn);
    }

    if ( !pofn )
    {
        return 0;
    }

    int bWindowModeSupported = stdDisplay_CanRenderWindowed();
    if ( bWindowModeSupported == -1 )
    {
        return -1;
    }

    if ( !bWindowModeSupported )
    {
        JonesDialogData dlgData = { 0 };
        dlgData.dwSize        = sizeof(JonesDialogData);
        dlgData.lpfnPrevHook  = pofn->lpfnHook;
        dlgData.lPrevCustData = pofn->lCustData;

        pofn->lCustData = (LPARAM)&dlgData;
        pofn->lpfnHook  = JonesDialog_SubclassFileDialogProc;

        JonesDialogData extraData = { 0 };
        extraData.dwSize   = sizeof(JonesDialogData);
        dlgData.pExtraData = &extraData;
    }

    JonesDialogGameState state;
    JonesDialog_SetGameState(&state, bWindowModeSupported);

    BOOL bRes;
    if ( bOpen )
    {
        bRes = GetOpenFileName(pofn);
    }
    else
    {
        bRes = GetSaveFileName(pofn);
    }

    JonesDialog_ResetGameState(&state, bWindowModeSupported);
    return bRes;
}

UINT_PTR CALLBACK JonesDialog_SubclassFileDialogProc(HWND hDlg, UINT Msg, WPARAM wParam, LPARAM lParam)
{
    HWND hwnd;
    HWND hParent;
    JonesDialogData* lpData_1;
    JonesDialogData* lpData;
    LPOFNHOOKPROC lpPrevWndFunc;
    LPOFNHOOKPROC lpPrevWndFunca;
    UINT_PTR result;

    lpPrevWndFunc = 0;
    hParent = GetParent(hDlg);
    if ( hParent == stdWin95_GetWindow() || (hwnd = stdWin95_GetWindow(), hParent == GetParent(hwnd)) )
    {
        hParent = hDlg;
    }

    lpData_1 = (JonesDialogData*)GetWindowLong(hDlg, GWL_USERDATA);
    if ( lpData_1 && lpData_1->dwSize == sizeof(JonesDialogData) )
    {
        lpPrevWndFunc = lpData_1->lpfnPrevHook;
    }

    if ( Msg == WM_DESTROY )
    {
        if ( JonesDialog_cursorInfo.numHandels )
        {
            JonesDialog_cursorInfo.aHwnd[--JonesDialog_cursorInfo.numHandels] = 0;
        }

        KillTimer(hDlg, 1u);
        goto LABEL_27;
    }

    if ( Msg != WM_INITDIALOG )
    {
        if ( Msg == WM_TIMER )
        {
            JonesDialog_TrackCursor(1);
        }

        goto LABEL_27;
    }

    lpData = *(JonesDialogData**)(lParam + 64);// TODO: cast lParam which is LPOPENFILENAMEA  here
    SetWindowLong(hDlg, GWL_USERDATA, (LONG)lpData);
    JonesDialog_dword_5532EC = 0;

    if ( !lpData || lpData->dwSize != sizeof(JonesDialogData) )
    {
    LABEL_27:
        if ( lpPrevWndFunc )
        {
            return CallWindowProc((WNDPROC)lpPrevWndFunc, hDlg, Msg, wParam, lParam);
        }
        else
        {
            return 0;
        }
    }

    STDLOG_ERROR("JonesDialog_SubclassFileDialogProc: hDlg == %08lx, hParent == %08lx\n", hDlg, hParent);

    *(uint32_t*)(lParam + 64) = lpData->lPrevCustData;
    lpPrevWndFunca = lpData->lpfnPrevHook;
    lpData->lpfnWndProc = (WNDPROC)GetWindowLong(hDlg, GWL_WNDPROC);
    lpData->lpUserData = (LPVOID)GetWindowLong(hDlg, GWL_USERDATA);
    SetWindowLong(hDlg, GWL_WNDPROC, (LONG)JonesDialog_SubclassDialogWindowProc);

    if ( JonesDialog_cursorInfo.numHandels >= 10 )
    {
        STDLOG_ERROR("JonesDialog_SubclassFileDialogProc: Too many dialogs displayed!\n");
    }
    else
    {
        JonesDialog_cursorInfo.aHwnd[JonesDialog_cursorInfo.numHandels++] = hParent;
    }

    if ( lpData->pExtraData )
    {
        lpData = lpData->pExtraData;
        lpData->lpfnWndProc = (WNDPROC)GetWindowLong(hParent, GWL_WNDPROC);
        lpData->lpUserData = (LPVOID)GetWindowLong(hParent, GWL_USERDATA);
        GetWindowRect(hParent, &lpData->rcWindow);
        SetWindowLong(hParent, GWL_WNDPROC, (LONG)JonesDialog_SubclassDialogWindowProc);
        SetWindowLong(hParent, GWL_USERDATA, (LONG)lpData);
    }

    result = CallWindowProc((WNDPROC)lpPrevWndFunca, hDlg, Msg, wParam, lParam);
    if ( lpData && lpData->dwSize == sizeof(JonesDialogData) )
    {
        GetWindowRect(hParent, &lpData->rcWindow);
    }

    SetTimer(hDlg, 1u, 33u, 0);
    return result;
}


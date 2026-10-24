#include "stdWin95SystemTestSupport.h"

#include <unity_fixture.h>

#include <stdlib.h>

#include <std/General/stdColor.h>
#include <std/General/stdConfig.h>
#include <std/General/stdJSON.h>
#include <std/General/stdUtil.h>
#include <std/Win95/std3D.h>
#include <std/Win95/stdControl.h>
#include <std/Win95/stdDisplay.h>
#include <std/Win95/stdWin95.h>

#include "stdGeneralTest.h"

#define STDWIN95SYSTEMTEST_WINDOW_CLASS_NAME "OpenJones3DStdWin95SystemTestWindow"
#define STDWIN95SYSTEMTEST_WINDOW_TITLE      "OpenJones3D std system test"
#define STDWIN95SYSTEMTEST_CONFIG_FILE       "stdWin95SystemTest.cfg"
#define STDWIN95SYSTEMTEST_HIDE_WINDOW_ENV   "JONES3D_SYSTEM_TEST_HIDE_WINDOW"
#define STDWIN95SYSTEMTEST_FRAME_DELAY_ENV   "JONES3D_SYSTEM_TEST_FRAME_DELAY_MS"
#define STDWIN95SYSTEMTEST_VISUAL_SPEED_ENV  "JONES3D_SYSTEM_TEST_VISUAL_SPEED"
#define STDWIN95SYSTEMTEST_FRAME_DELAY_MS    100u
#define STDWIN95SYSTEMTEST_WINDOW_WIDTH      320u
#define STDWIN95SYSTEMTEST_WINDOW_HEIGHT     240u

static HWND stdWin95SystemTest_hwnd;
static HWND stdWin95SystemTest_previousForegroundWindow;
static bool stdWin95SystemTest_bForegroundWindowRequested;
static HINSTANCE stdWin95SystemTest_hInstance;
static bool stdWin95SystemTest_bVisualWindowShown;
static DWORD stdWin95SystemTest_frameDelayMs;
static DWORD stdWin95SystemTest_lastVisualFrameStartTimeMs;

static bool stdWin95SystemTest_bDisplayStarted;
static bool stdWin95SystemTest_bDisplayOpen;
static bool stdWin95SystemTest_bControlStarted;
static bool stdWin95SystemTest_bControlOpen;
static bool stdWin95SystemTest_bStd3DStarted;
static bool stdWin95SystemTest_bJsonStarted;
static bool stdWin95SystemTest_bConfigStarted;

static void StdWin95SystemTest_BlitBackBufferToHdc(HDC hdc);

static LRESULT CALLBACK StdWin95SystemTest_WindowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    if ( message == WM_CLOSE )
    {
        ShowWindow(hwnd, SW_HIDE);
        return 0;
    }

    if ( message == WM_PAINT )
    {
        // Exclusive DirectDraw owns the display; acknowledge paint without opening a GDI DC.
        if ( stdDisplay_IsFullscreen() )
        {
            ValidateRect(hwnd, NULL);
            return 0;
        }

        PAINTSTRUCT paint;

        HDC hdc = BeginPaint(hwnd, &paint);
        if ( hdc )
        {
            StdWin95SystemTest_BlitBackBufferToHdc(hdc);
            EndPaint(hwnd, &paint);
        }

        return 0;
    }

    return DefWindowProcA(hwnd, message, wParam, lParam);
}

static bool StdWin95SystemTest_IsEnvEnabled(const char* pName)
{
    const char* pValue = getenv(pName);

    if ( !pValue || !pValue[0] )
    {
        return false;
    }

    return pValue[0] == '1'
        || pValue[0] == 't'
        || pValue[0] == 'T'
        || pValue[0] == 'y'
        || pValue[0] == 'Y'
        || pValue[0] == 'o'
        || pValue[0] == 'O';
}

static DWORD StdWin95SystemTest_GetVisualFrameDelayMs(void)
{
    const char* pValue = getenv(STDWIN95SYSTEMTEST_FRAME_DELAY_ENV);

    if ( pValue && pValue[0] )
    {
        unsigned long delayMs = strtoul(pValue, NULL, 10);
        if ( delayMs > 1000ul )
        {
            delayMs = 1000ul;
        }

        return (DWORD)delayMs;
    }

    const char* pSpeed = getenv(STDWIN95SYSTEMTEST_VISUAL_SPEED_ENV);
    if ( !pSpeed || !pSpeed[0] )
    {
        return STDWIN95SYSTEMTEST_FRAME_DELAY_MS;
    }

    double speed = strtod(pSpeed, NULL);
    if ( speed <= 0.0 )
    {
        return 0u;
    }

    if ( speed > 10.0 )
    {
        speed = 10.0;
    }

    return (DWORD)((double)STDWIN95SYSTEMTEST_FRAME_DELAY_MS / speed);
}

void StdWin95SystemTest_PumpMessages(void)
{
    MSG msg;

    while ( PeekMessageA(&msg, NULL, 0u, 0u, PM_REMOVE) )
    {
        TranslateMessage(&msg);
        DispatchMessageA(&msg);
    }
}

void StdWin95SystemTest_SizeWindowClient(uint32_t width, uint32_t height)
{
    RECT rect = { 0, 0, (LONG)width, (LONG)height };

    if ( !stdWin95SystemTest_hwnd )
    {
        return;
    }

    RECT clientRect;
    if ( GetClientRect(stdWin95SystemTest_hwnd, &clientRect) )
    {
        uint32_t clientWidth  = (uint32_t)(clientRect.right - clientRect.left);
        uint32_t clientHeight = (uint32_t)(clientRect.bottom - clientRect.top);
        if ( clientWidth == width && clientHeight == height )
        {
            return;
        }
    }

    DWORD style   = (DWORD)GetWindowLongPtrA(stdWin95SystemTest_hwnd, GWL_STYLE);
    DWORD exStyle = (DWORD)GetWindowLongPtrA(stdWin95SystemTest_hwnd, GWL_EXSTYLE);
    if ( !AdjustWindowRectEx(&rect, style, FALSE, exStyle) )
    {
        return;
    }

    SetWindowPos(
        stdWin95SystemTest_hwnd,
        NULL,
        0,
        0,
        rect.right - rect.left,
        rect.bottom - rect.top,
        SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE
    );
}

static void StdWin95SystemTest_ReadBackBufferPixel(const uint8_t* pSurface, uint32_t surfaceHeight, int32_t pitch, uint32_t x, uint32_t y, const ColorInfo* pColorInfo, uint8_t* pRed, uint8_t* pGreen, uint8_t* pBlue)
{
    uint32_t encoded     = 0u;
    size_t bytesPerPixel = pColorInfo->bpp / 8u;

    if ( bytesPerPixel == 0u || bytesPerPixel > sizeof(encoded) )
    {
        *pRed   = 0u;
        *pGreen = 0u;
        *pBlue  = 0u;
        return;
    }

    const uint8_t* pRow = pitch >= 0
        ? pSurface + (size_t)y * (size_t)pitch
        : pSurface + (size_t)(surfaceHeight - 1u - y) * (size_t)-pitch;
    const uint8_t* pPixel = pRow + (size_t)x * bytesPerPixel;

    STD_COPYMEM(&encoded, pPixel, bytesPerPixel);
    stdColor_DecodeRGB(encoded, pColorInfo, pRed, pGreen, pBlue);
}

static void StdWin95SystemTest_BlitBackBufferToHdc(HDC hdc)
{
    void* pSurface;
    uint32_t width;
    uint32_t height;
    int32_t pitch;
    StdVideoMode mode;

    // Exclusive fullscreen owns the primary surface; keep the GDI preview in windowed tests.
    if ( !hdc || !stdWin95SystemTest_hwnd || !stdDisplay_IsOpen() || stdDisplay_IsFullscreen() )
    {
        return;
    }

    STD_ZEROMEM(&mode, sizeof(mode));
    if ( stdDisplay_GetCurrentVideoMode(&mode) )
    {
        return;
    }

    if ( stdDisplay_LockBackBufferReadOnly(&pSurface, &width, &height, &pitch) )
    {
        return;
    }

    size_t rowBytes = (size_t)width * 4u;
    uint8_t* pBits  = (uint8_t*)malloc(rowBytes * height);
    if ( !pBits )
    {
        stdDisplay_UnlockBackBuffer();
        return;
    }

    for ( uint32_t y = 0u; y < height; ++y )
    {
        uint8_t* pDstRow = pBits + (size_t)y * rowBytes;

        for ( uint32_t x = 0u; x < width; ++x )
        {
            uint8_t red;
            uint8_t green;
            uint8_t blue;
            uint8_t* pDst = pDstRow + (size_t)x * 4u;

            StdWin95SystemTest_ReadBackBufferPixel(
                (const uint8_t*)pSurface,
                height,
                pitch,
                x,
                y,
                &mode.rasterInfo.colorInfo,
                &red,
                &green,
                &blue
            );
            pDst[0] = blue;
            pDst[1] = green;
            pDst[2] = red;
            pDst[3] = 0u;
        }
    }

    stdDisplay_UnlockBackBuffer();
    StdWin95SystemTest_SizeWindowClient(width, height);

    BITMAPINFO bitmapInfo;
    STD_ZEROMEM(&bitmapInfo, sizeof(bitmapInfo));
    bitmapInfo.bmiHeader.biSize        = sizeof(bitmapInfo.bmiHeader);
    bitmapInfo.bmiHeader.biWidth       = (LONG)width;
    bitmapInfo.bmiHeader.biHeight      = -(LONG)height;
    bitmapInfo.bmiHeader.biPlanes      = 1u;
    bitmapInfo.bmiHeader.biBitCount    = 32u;
    bitmapInfo.bmiHeader.biCompression = BI_RGB;

    StretchDIBits(
        hdc,
        0,
        0,
        (int)width,
        (int)height,
        0,
        0,
        (int)width,
        (int)height,
        pBits,
        &bitmapInfo,
        DIB_RGB_COLORS,
        SRCCOPY
    );
    GdiFlush();
    free(pBits);
}

static void StdWin95SystemTest_BlitBackBufferToWindow(void)
{
    if ( stdDisplay_IsFullscreen() )
    {
        return;
    }

    HDC hdc = GetDC(stdWin95SystemTest_hwnd);
    if ( hdc )
    {
        StdWin95SystemTest_BlitBackBufferToHdc(hdc);
        ReleaseDC(stdWin95SystemTest_hwnd, hdc);
    }
}

static bool StdWin95SystemTest_RegisterWindowClass(void)
{
    WNDCLASSEXA windowClass;

    STD_ZEROMEM(&windowClass, sizeof(windowClass));
    windowClass.cbSize        = sizeof(windowClass);
    windowClass.style         = CS_HREDRAW | CS_VREDRAW;
    windowClass.lpfnWndProc   = StdWin95SystemTest_WindowProc;
    windowClass.hInstance     = stdWin95SystemTest_hInstance;
    windowClass.hCursor       = LoadCursorA(NULL, IDC_ARROW);
    windowClass.lpszClassName = STDWIN95SYSTEMTEST_WINDOW_CLASS_NAME;

    if ( RegisterClassExA(&windowClass) )
    {
        return true;
    }

    return GetLastError() == ERROR_CLASS_ALREADY_EXISTS;
}

static HWND StdWin95SystemTest_CreateHiddenWindow(void)
{
    stdWin95SystemTest_hInstance = GetModuleHandleA(NULL);
    if ( !stdWin95SystemTest_hInstance )
    {
        return NULL;
    }

    if ( !StdWin95SystemTest_RegisterWindowClass() )
    {
        return NULL;
    }

    RECT windowRect = { 0, 0, STDWIN95SYSTEMTEST_WINDOW_WIDTH, STDWIN95SYSTEMTEST_WINDOW_HEIGHT };
    if ( !AdjustWindowRectEx(&windowRect, WS_OVERLAPPEDWINDOW, FALSE, 0) )
    {
        return NULL;
    }

    HWND hwnd = CreateWindowExA(
        0,
        STDWIN95SYSTEMTEST_WINDOW_CLASS_NAME,
        STDWIN95SYSTEMTEST_WINDOW_TITLE,
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        windowRect.right - windowRect.left,
        windowRect.bottom - windowRect.top,
        NULL,
        NULL,
        stdWin95SystemTest_hInstance,
        NULL
    );

    if ( hwnd )
    {
        stdWin95SystemTest_bVisualWindowShown         = !StdWin95SystemTest_IsEnvEnabled(STDWIN95SYSTEMTEST_HIDE_WINDOW_ENV);
        stdWin95SystemTest_frameDelayMs               = stdWin95SystemTest_bVisualWindowShown
            ? StdWin95SystemTest_GetVisualFrameDelayMs()
            : 0u;
        stdWin95SystemTest_lastVisualFrameStartTimeMs = 0u;

        ShowWindow(hwnd, stdWin95SystemTest_bVisualWindowShown ? SW_SHOWNORMAL : SW_HIDE);
        if ( stdWin95SystemTest_bVisualWindowShown )
        {
            UpdateWindow(hwnd);
            StdWin95SystemTest_PumpMessages();
        }
    }

    return hwnd;
}

static void StdWin95SystemTest_DestroyHiddenWindow(void)
{
    stdWin95_SetWindow(NULL);
    stdWin95_SetInstance(NULL);

    if ( stdWin95SystemTest_hwnd )
    {
        DestroyWindow(stdWin95SystemTest_hwnd);
        stdWin95SystemTest_hwnd = NULL;
    }

    stdWin95SystemTest_hInstance                  = NULL;
    stdWin95SystemTest_bVisualWindowShown         = false;
    stdWin95SystemTest_frameDelayMs               = 0u;
    stdWin95SystemTest_lastVisualFrameStartTimeMs = 0u;
}

static void StdWin95SystemTest_ShutdownSubsystems(void)
{
    if ( stdWin95SystemTest_bStd3DStarted )
    {
        std3D_Shutdown();
        stdWin95SystemTest_bStd3DStarted = false;
    }

    if ( stdWin95SystemTest_bControlOpen )
    {
        stdControl_Close();
        stdWin95SystemTest_bControlOpen = false;
    }

    if ( stdWin95SystemTest_bControlStarted )
    {
        stdControl_Shutdown();
        stdWin95SystemTest_bControlStarted = false;
    }

    if ( stdWin95SystemTest_bDisplayOpen )
    {
        stdDisplay_Close();
        stdWin95SystemTest_bDisplayOpen = false;
    }

    if ( stdWin95SystemTest_bDisplayStarted )
    {
        stdDisplay_Shutdown();
        stdWin95SystemTest_bDisplayStarted = false;
    }

    if ( stdWin95SystemTest_bConfigStarted )
    {
        stdConfig_Shutdown();
        stdWin95SystemTest_bConfigStarted = false;
    }

    if ( stdWin95SystemTest_bJsonStarted )
    {
        stdJSON_Shutdown();
        stdWin95SystemTest_bJsonStarted = false;
    }

    StdGeneralTest_DeleteFile(STDWIN95SYSTEMTEST_CONFIG_FILE);
}

void StdWin95SystemTest_Startup(void)
{
    StdGeneralTest_Startup();

    stdWin95SystemTest_previousForegroundWindow = GetForegroundWindow();
    stdWin95SystemTest_bForegroundWindowRequested = false;

    stdWin95SystemTest_bDisplayStarted = false;
    stdWin95SystemTest_bDisplayOpen    = false;
    stdWin95SystemTest_bControlStarted = false;
    stdWin95SystemTest_bControlOpen    = false;
    stdWin95SystemTest_bStd3DStarted   = false;
    stdWin95SystemTest_bJsonStarted    = false;
    stdWin95SystemTest_bConfigStarted  = false;

    stdWin95_SetWindow(NULL);
    stdWin95_SetInstance(NULL);

    stdWin95SystemTest_hwnd = StdWin95SystemTest_CreateHiddenWindow();
    if ( stdWin95SystemTest_hwnd )
    {
        stdWin95_SetInstance(stdWin95SystemTest_hInstance);
        stdWin95_SetWindow(stdWin95SystemTest_hwnd);
    }
}

void StdWin95SystemTest_Shutdown(void)
{
    StdWin95SystemTest_ShutdownSubsystems();
    StdWin95SystemTest_DestroyHiddenWindow();
    if ( stdWin95SystemTest_bForegroundWindowRequested
        && IsWindow(stdWin95SystemTest_previousForegroundWindow) )
    {
        SetForegroundWindow(stdWin95SystemTest_previousForegroundWindow);
    }
    stdWin95SystemTest_previousForegroundWindow = NULL;
    stdWin95SystemTest_bForegroundWindowRequested = false;
    StdGeneralTest_Shutdown();
}

HWND StdWin95SystemTest_GetWindow(void)
{
    return stdWin95SystemTest_hwnd;
}

HINSTANCE StdWin95SystemTest_GetInstance(void)
{
    return stdWin95SystemTest_hInstance;
}

void StdWin95SystemTest_PresentVisualFrame(void)
{
    if ( !stdWin95SystemTest_bVisualWindowShown )
    {
        return;
    }

    StdWin95SystemTest_PumpMessages();

    // Count presentation, comparison, render, and readback work toward the visual frame interval.
    if ( stdWin95SystemTest_frameDelayMs && stdWin95SystemTest_lastVisualFrameStartTimeMs )
    {
        DWORD elapsedMs = GetTickCount() - stdWin95SystemTest_lastVisualFrameStartTimeMs;
        if ( elapsedMs < stdWin95SystemTest_frameDelayMs )
        {
            Sleep(stdWin95SystemTest_frameDelayMs - elapsedMs);
        }
    }

    stdWin95SystemTest_lastVisualFrameStartTimeMs = GetTickCount();
    StdWin95SystemTest_BlitBackBufferToWindow();

    StdWin95SystemTest_PumpMessages();
}

void StdWin95SystemTest_RequireHiddenWindow(void)
{
    if ( !stdWin95SystemTest_hwnd )
    {
        TEST_IGNORE_MESSAGE("Hidden Win32 window unavailable.");
    }
}

void StdWin95SystemTest_RequireForegroundWindow(void)
{
    StdWin95SystemTest_RequireHiddenWindow();
    if ( !stdWin95SystemTest_bVisualWindowShown )
    {
        TEST_IGNORE_MESSAGE("Injected input requires a visible foreground window; hidden-window mode is enabled.");
    }

    stdWin95SystemTest_bForegroundWindowRequested = true;
    SetForegroundWindow(stdWin95SystemTest_hwnd);
    StdWin95SystemTest_PumpMessages();
    Sleep(25u);
    StdWin95SystemTest_PumpMessages();
    if ( GetForegroundWindow() != stdWin95SystemTest_hwnd )
    {
        TEST_IGNORE_MESSAGE("The host desktop denied foreground focus to the input test window.");
    }
}

void StdWin95SystemTest_RequireDisplayConfig(void)
{
    if ( !stdJSON_HasStarted() )
    {
        if ( !stdJSON_Startup() )
        {
            TEST_IGNORE_MESSAGE("stdJSON startup unavailable for system display config.");
        }

        stdWin95SystemTest_bJsonStarted = true;
    }

    if ( !stdConfig_HasStarted() )
    {
        StdGeneralTest_DeleteFile(STDWIN95SYSTEMTEST_CONFIG_FILE);
        if ( !stdConfig_Startup(STDWIN95SYSTEMTEST_CONFIG_FILE) )
        {
            TEST_IGNORE_MESSAGE("stdConfig startup unavailable for system display config.");
        }

        stdWin95SystemTest_bConfigStarted = true;
        TEST_ASSERT_TRUE(stdConfig_SetBool(STD3D_CFG_MSAAENABLED, false));
        TEST_ASSERT_TRUE(stdConfig_SetInt(STD3D_CFG_MSAASAMPLES, 2));
        TEST_ASSERT_TRUE(stdConfig_SetBool(STD3D_CFG_MIPMAPAUTOGEN, false));
        TEST_ASSERT_TRUE(stdConfig_SetBool(STD3D_CFG_ANISOTROPICFILTER, false));
    }
}

void StdWin95SystemTest_RequireControlStartup(void)
{
    if ( stdControl_Startup(0) )
    {
        TEST_IGNORE_MESSAGE("DirectInput startup unavailable on this host.");
    }

    stdWin95SystemTest_bControlStarted = true;
}

void StdWin95SystemTest_RequireDisplayStartup(void)
{
    StdWin95SystemTest_RequireDisplayConfig();

    if ( stdDisplay_Startup() )
    {
        stdWin95SystemTest_bDisplayStarted = true;
        return;
    }

    stdDisplay_Shutdown();
    TEST_IGNORE_MESSAGE("DirectX display startup unavailable on this host.");
}

void StdWin95SystemTest_RequireDisplayDeviceOpen(void)
{
    StdWin95SystemTest_RequireDisplayStartup();

    if ( stdDisplay_GetNumDevices() == 0u )
    {
        TEST_IGNORE_MESSAGE("No DirectX display devices were enumerated on this host.");
    }

    if ( !stdDisplay_Open(0u) )
    {
        TEST_IGNORE_MESSAGE("DirectX display device 0 could not be opened on this host.");
    }

    stdWin95SystemTest_bDisplayOpen = true;
}

void StdWin95SystemTest_RequireWindowedDisplayModeAtResolution(uint32_t width, uint32_t height)
{
    StdWin95SystemTest_RequireDisplayDeviceOpen();

    stdDisplay_SetDefaultResolution(width, height);
    if ( stdDisplay_SetMode(0u, 0, 1u) )
    {
        TEST_IGNORE_MESSAGE("Windowed DirectX display mode unavailable on this host.");
    }

    StdWin95SystemTest_SizeWindowClient(width, height);
}

void StdWin95SystemTest_RequireWindowedDisplayMode(void)
{
    StdWin95SystemTest_RequireWindowedDisplayModeAtResolution(320u, 240u);
}

void StdWin95SystemTest_SetControlStarted(bool bStarted)
{
    stdWin95SystemTest_bControlStarted = bStarted;
}

void StdWin95SystemTest_SetControlOpen(bool bOpen)
{
    stdWin95SystemTest_bControlOpen = bOpen;
}

void StdWin95SystemTest_SetDisplayStarted(bool bStarted)
{
    stdWin95SystemTest_bDisplayStarted = bStarted;
}

void StdWin95SystemTest_SetDisplayOpen(bool bOpen)
{
    stdWin95SystemTest_bDisplayOpen = bOpen;
}

void StdWin95SystemTest_SetStd3DStarted(bool bStarted)
{
    stdWin95SystemTest_bStd3DStarted = bStarted;
}

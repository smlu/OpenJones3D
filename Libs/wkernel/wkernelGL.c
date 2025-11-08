#include "wkernel.h"
#include "wkernel/RTI/symbols.h"

#include <j3dcore/j3dhook.h>
#include <std/Win95/stdWin95.h>
#include <CommCtrl.h> // InitCommonControls
#include <initguid.h>// DEFINE_GUID
#include <glad/glad.h>
#include <SDL3/SDL.h>

#include "std/General/std.h"


#define IDI_APPICON 108
DEFINE_GUID(wkernel_guid, 0x82CE4DA0, 0x9CBF, 0x1D1, 0x90, 0x85, 0x00, 0x60, 0x97, 0x76, 0x0EA, 0x02);

static SDL_Window *wkernel_sdl_window = NULL;
static SDL_GLContext wkernel_gl_context = NULL;

static HWND wkernel_hwnd       = 0;

static WKERNELPROC wkernel_pfProcess            = NULL;
static WKERNELSTARTUPPROC wkernel_pfOnStartup   = NULL;
static WKERNELSHUTDOWNPROC wkernel_pfOnShutdown = NULL;
static WKERNELWNDPROC wkernel_pfWndProc         = NULL;

static inline int J3DAPI wkernel_CreateWindow(HINSTANCE hInstance, int nShowCmd, LPCSTR lpWindowName);
static LRESULT CALLBACK wkernel_MainWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
static void wkernel_DispatchSdlEvent(const SDL_Event* ev);

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
    J3D_UNUSED(hinstance); // old DX6/9 path still passes it, ignore for SDL/OpenGL
    J3D_UNUSED(hPrevInstance);

    //SDL_Delay(10000);


    // Added: Refactored to run main proc via callback
    if ( wkernel_pfProcess == NULL )
    {
        // TODO: [LOG] make log entry
        fprintf(stderr, "ERROR: wkernel_Run: No main process set!\n");
        return -1;
    }

    if ( wkernel_CreateWindow(hinstance, nShowCmd, lpWindowName) ) {
        return -1;
    }

    HINSTANCE glInstance = GetModuleHandle(NULL);
    stdWin95_SetWindow(wkernel_hwnd);
    stdWin95_SetInstance(glInstance);
    stdWin95_SetGuid(&wkernel_guid);
    stdWin95_SetGLContext(wkernel_gl_context);
    stdWin95_SetSDLWindow(wkernel_sdl_window);

    InitCommonControls();

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
            // TODO: [LOG] Add log entry
            // Added: Log
            fprintf(stdout, "WARNING: wkernel_Run: Unhandled result code: %d, skipping...\n", result);
        }

        int quit = wkernel_ProcessEvents();
        if (quit == 1) return 0;

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
    SDL_Event e;
    if (!SDL_PeepEvents(&e, 1, SDL_PEEKEVENT, SDL_EVENT_FIRST, SDL_EVENT_LAST))
        return 0; // nichts da
    // einmal abarbeiten (nur „einen Schwung“)
    while (SDL_PollEvent(&e)) {
        if (e.type == SDL_EVENT_QUIT) return 1;
        wkernel_DispatchSdlEvent(&e); // siehe unten
    }
    return 0;
}

int wkernel_ProcessEvents(void)
{
    SDL_Event e;
    while (SDL_PollEvent(&e)) {
        if (e.type == SDL_EVENT_QUIT) return 1;
        wkernel_DispatchSdlEvent(&e);
        int pending = SDL_PeepEvents(NULL, 0, SDL_PEEKEVENT, SDL_EVENT_FIRST, SDL_EVENT_LAST);
        if (pending == 0) return 0;
    }
    return -1;
}

static void wkernel_DispatchSdlEvent(const SDL_Event* ev)
{
    if (!wkernel_pfWndProc) return;
    int ret = 0;

    switch (ev->type) {
    case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
        if (wkernel_pfOnShutdown) wkernel_pfOnShutdown();
        wkernel_pfWndProc(wkernel_hwnd, WM_CLOSE, 0, 0, &ret);
        SDL_PushEvent(&(SDL_Event){ .type = SDL_EVENT_QUIT });
        break;
    case SDL_EVENT_WINDOW_DESTROYED:
        wkernel_pfWndProc(wkernel_hwnd, WM_DESTROY, 0, 0, &ret);
        break;
    case SDL_EVENT_KEY_DOWN:
        wkernel_pfWndProc(wkernel_hwnd, WM_KEYDOWN, (WPARAM)ev->key.key, 0, &ret);
        break;
    case SDL_EVENT_KEY_UP:
        wkernel_pfWndProc(wkernel_hwnd, WM_KEYUP, (WPARAM)ev->key.key, 0, &ret);
        break;
    case SDL_EVENT_MOUSE_MOTION: {
            LPARAM lp = ((int)ev->motion.y << 16) | ((int)ev->motion.x & 0xFFFF);
            wkernel_pfWndProc(wkernel_hwnd, WM_MOUSEMOVE, 0, lp, &ret);
            break;
    }
    case SDL_EVENT_MOUSE_BUTTON_DOWN:
    case SDL_EVENT_MOUSE_BUTTON_UP: {
            UINT msg =
                (ev->type == SDL_EVENT_MOUSE_BUTTON_DOWN) ? WM_LBUTTONDOWN : WM_LBUTTONUP;
            LPARAM lp = ((int)ev->button.y << 16) | ((int)ev->button.x & 0xFFFF);
            wkernel_pfWndProc(wkernel_hwnd, msg, 0, lp, &ret);
            break;
    }
    default:
        break;
    }
}


void J3DAPI wkernel_SetWindowStyle(LONG dwNewLong)
{
    if (!wkernel_sdl_window) return;

    bool bordered = (dwNewLong & (WS_BORDER | WS_CAPTION)) != 0;
    SDL_SetWindowBordered(wkernel_sdl_window, bordered);
}


BOOL J3DAPI wkernel_SetWindowSize(int width, int height)
{
    SDL_SetWindowPosition(wkernel_sdl_window, 0, 0);
    return SDL_SetWindowSize(wkernel_sdl_window,width, height);
    // return SetWindowPos(
    //     wkernel_hwnd,
    //     NULL,
    //     0,
    //     0,
    //     wkernel_wndBorderWidth + width,
    //     wkernel_wndMenuBarHeight + height,
    //     SWP_NOZORDER | SWP_NOMOVE
    // );
}

void J3DAPI wkernel_SetWindowProc(WKERNELWNDPROC pfProc)
{
    wkernel_pfWndProc = pfProc;
}

int J3DAPI wkernel_CreateWindow(HINSTANCE hInstance, int nShowCmd, LPCSTR lpWindowName)
{
    J3D_UNUSED(hInstance);
    J3D_UNUSED(nShowCmd);
    int width, height;

    // --- optional: Single-Instance wie bisher (Windows) ---
    // SDL-Fenster haben standardmäßig die Klasse "SDL_app"
    if (FindWindow("SDL_app", lpWindowName))
    {
        STDLOG_STATUS("INFO: wkernel_CreateWindow: An existing instance of window already running!\n");
        return 1;
    }

    // --- SDL Video init ---
    if (!SDL_WasInit(SDL_INIT_VIDEO))
    {
        if (SDL_InitSubSystem(SDL_INIT_VIDEO) < 0)
        {
            STDLOG_ERROR("SDL video init failed: %s\n", SDL_GetError());
            return 1;
        }
    }


    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_COMPATIBILITY);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetSwapInterval(1);

    Uint32 flags = SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE;

    height = GetSystemMetrics(1);
    width = GetSystemMetrics(0);

    wkernel_sdl_window = SDL_CreateWindow(lpWindowName, width, height, flags);
    if (!wkernel_sdl_window)
    {
        STDLOG_ERROR("SDL_CreateWindow failed: %s\n", SDL_GetError());
        return 1;
    }



    wkernel_gl_context = SDL_GL_CreateContext(wkernel_sdl_window);
    if (!wkernel_gl_context)
    {
        STDLOG_ERROR("SDL_GL_CreateContext failed: %s\n", SDL_GetError());
        return 1;
    }



    if (!gladLoadGLLoader((GLADloadproc)SDL_GL_GetProcAddress))
    {
        STDLOG_ERROR("Failed to initialize GLAD\n");
        return 1;
    }

    SDL_PropertiesID props = SDL_GetWindowProperties(wkernel_sdl_window);
    wkernel_hwnd = (HWND)SDL_GetPointerProperty(props, SDL_PROP_WINDOW_WIN32_HWND_POINTER, NULL);
    if (!wkernel_hwnd)
    {
        return 1;
    }

    wkernel_SetWindowSize(100, 25);


    return 0;
}

LRESULT CALLBACK wkernel_MainWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    return 0;
}
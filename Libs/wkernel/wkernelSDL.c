#include "wkernel.h"
#include "wkernel/RTI/symbols.h"

#include <j3dcore/j3dhook.h>
#include <std/Win95/stdWin95.h>

#include <SDL3/SDL.h>
#include <glad/glad.h>

#include <stdio.h>
#include <stdbool.h>
#include <string.h>

static const SDL_GUID wkernel_guid = {
    0xA0, 0x4D, 0xCE, 0x82,  // Data1: 0x82CE4DA0
    0xBF, 0x9C,              // Data2: 0x9CBF
    0xD1, 0x01,              // Data3: 0x1D1
    0x90, 0x85, 0x00, 0x60, 0x97, 0x76, 0xEA, 0x02
};
static const char wkernel_aClassName[] = "wKernelJones3D";

static SDL_Window* wkernel_sdlWindow   = NULL;
static SDL_GLContext wkernel_glContext = NULL;
static HWND wkernel_hwnd               = NULL;

static bool wkernel_quit_requested = false;

static WKERNELPROC wkernel_pfProcess            = NULL;
static WKERNELSTARTUPPROC wkernel_pfOnStartup   = NULL;
static WKERNELSHUTDOWNPROC wkernel_pfOnShutdown = NULL;
static WKERNELWNDPROC wkernel_pfWndProc         = NULL;
static WNDPROC wkernel_sdlWndProc               = NULL; // Store the original SDL WndProc

// Window stats
static LONG wkernel_currentStyle = 0;

// SDL key scancode to VK translation table
static const WPARAM wkernel_sdlScancodeToVK[SDL_SCANCODE_COUNT] =
{
    [SDL_SCANCODE_UNKNOWN] = 0,

    // Letters A-Z
    [SDL_SCANCODE_A] = 'A',
    [SDL_SCANCODE_B] = 'B',
    [SDL_SCANCODE_C] = 'C',
    [SDL_SCANCODE_D] = 'D',
    [SDL_SCANCODE_E] = 'E',
    [SDL_SCANCODE_F] = 'F',
    [SDL_SCANCODE_G] = 'G',
    [SDL_SCANCODE_H] = 'H',
    [SDL_SCANCODE_I] = 'I',
    [SDL_SCANCODE_J] = 'J',
    [SDL_SCANCODE_K] = 'K',
    [SDL_SCANCODE_L] = 'L',
    [SDL_SCANCODE_M] = 'M',
    [SDL_SCANCODE_N] = 'N',
    [SDL_SCANCODE_O] = 'O',
    [SDL_SCANCODE_P] = 'P',
    [SDL_SCANCODE_Q] = 'Q',
    [SDL_SCANCODE_R] = 'R',
    [SDL_SCANCODE_S] = 'S',
    [SDL_SCANCODE_T] = 'T',
    [SDL_SCANCODE_U] = 'U',
    [SDL_SCANCODE_V] = 'V',
    [SDL_SCANCODE_W] = 'W',
    [SDL_SCANCODE_X] = 'X',
    [SDL_SCANCODE_Y] = 'Y',
    [SDL_SCANCODE_Z] = 'Z',

    // Numbers 1-0
    [SDL_SCANCODE_1] = '1',
    [SDL_SCANCODE_2] = '2',
    [SDL_SCANCODE_3] = '3',
    [SDL_SCANCODE_4] = '4',
    [SDL_SCANCODE_5] = '5',
    [SDL_SCANCODE_6] = '6',
    [SDL_SCANCODE_7] = '7',
    [SDL_SCANCODE_8] = '8',
    [SDL_SCANCODE_9] = '9',
    [SDL_SCANCODE_0] = '0',

    // Function keys
    [SDL_SCANCODE_F1]  = VK_F1,
    [SDL_SCANCODE_F2]  = VK_F2,
    [SDL_SCANCODE_F3]  = VK_F3,
    [SDL_SCANCODE_F4]  = VK_F4,
    [SDL_SCANCODE_F5]  = VK_F5,
    [SDL_SCANCODE_F6]  = VK_F6,
    [SDL_SCANCODE_F7]  = VK_F7,
    [SDL_SCANCODE_F8]  = VK_F8,
    [SDL_SCANCODE_F9]  = VK_F9,
    [SDL_SCANCODE_F10] = VK_F10,
    [SDL_SCANCODE_F11] = VK_F11,
    [SDL_SCANCODE_F12] = VK_F12,
    [SDL_SCANCODE_F13] = VK_F13,
    [SDL_SCANCODE_F14] = VK_F14,
    [SDL_SCANCODE_F15] = VK_F15,
    [SDL_SCANCODE_F16] = VK_F16,
    [SDL_SCANCODE_F17] = VK_F17,
    [SDL_SCANCODE_F18] = VK_F18,
    [SDL_SCANCODE_F19] = VK_F19,
    [SDL_SCANCODE_F20] = VK_F20,
    [SDL_SCANCODE_F21] = VK_F21,
    [SDL_SCANCODE_F22] = VK_F22,
    [SDL_SCANCODE_F23] = VK_F23,
    [SDL_SCANCODE_F24] = VK_F24,

    // Arrow keys
    [SDL_SCANCODE_UP]    = VK_UP,
    [SDL_SCANCODE_DOWN]  = VK_DOWN,
    [SDL_SCANCODE_LEFT]  = VK_LEFT,
    [SDL_SCANCODE_RIGHT] = VK_RIGHT,

    // Special keys
    [SDL_SCANCODE_RETURN]       = VK_RETURN,
    [SDL_SCANCODE_ESCAPE]       = VK_ESCAPE,
    [SDL_SCANCODE_BACKSPACE]    = VK_BACK,
    [SDL_SCANCODE_TAB]          = VK_TAB,
    [SDL_SCANCODE_SPACE]        = VK_SPACE,
    [SDL_SCANCODE_MINUS]        = VK_OEM_MINUS,
    [SDL_SCANCODE_EQUALS]       = VK_OEM_PLUS,
    [SDL_SCANCODE_LEFTBRACKET]  = VK_OEM_4,
    [SDL_SCANCODE_RIGHTBRACKET] = VK_OEM_6,
    [SDL_SCANCODE_BACKSLASH]    = VK_OEM_5,
    [SDL_SCANCODE_SEMICOLON]    = VK_OEM_1,
    [SDL_SCANCODE_APOSTROPHE]   = VK_OEM_7,
    [SDL_SCANCODE_GRAVE]        = VK_OEM_3,
    [SDL_SCANCODE_COMMA]        = VK_OEM_COMMA,
    [SDL_SCANCODE_PERIOD]       = VK_OEM_PERIOD,
    [SDL_SCANCODE_SLASH]        = VK_OEM_2,

    // Modifier keys
    [SDL_SCANCODE_LSHIFT] = VK_LSHIFT,
    [SDL_SCANCODE_RSHIFT] = VK_RSHIFT,
    [SDL_SCANCODE_LCTRL]  = VK_LCONTROL,
    [SDL_SCANCODE_RCTRL]  = VK_RCONTROL,
    [SDL_SCANCODE_LALT]   = VK_LMENU,
    [SDL_SCANCODE_RALT]   = VK_RMENU,
    [SDL_SCANCODE_LGUI]   = VK_LWIN,
    [SDL_SCANCODE_RGUI]   = VK_RWIN,

    // Editing keys
    [SDL_SCANCODE_INSERT]   = VK_INSERT,
    [SDL_SCANCODE_DELETE]   = VK_DELETE,
    [SDL_SCANCODE_HOME]     = VK_HOME,
    [SDL_SCANCODE_END]      = VK_END,
    [SDL_SCANCODE_PAGEUP]   = VK_PRIOR,
    [SDL_SCANCODE_PAGEDOWN] = VK_NEXT,

    // Lock keys
    [SDL_SCANCODE_CAPSLOCK]     = VK_CAPITAL,
    [SDL_SCANCODE_NUMLOCKCLEAR] = VK_NUMLOCK,
    [SDL_SCANCODE_SCROLLLOCK]   = VK_SCROLL,

    // Numpad
    [SDL_SCANCODE_KP_0]        = VK_NUMPAD0,
    [SDL_SCANCODE_KP_1]        = VK_NUMPAD1,
    [SDL_SCANCODE_KP_2]        = VK_NUMPAD2,
    [SDL_SCANCODE_KP_3]        = VK_NUMPAD3,
    [SDL_SCANCODE_KP_4]        = VK_NUMPAD4,
    [SDL_SCANCODE_KP_5]        = VK_NUMPAD5,
    [SDL_SCANCODE_KP_6]        = VK_NUMPAD6,
    [SDL_SCANCODE_KP_7]        = VK_NUMPAD7,
    [SDL_SCANCODE_KP_8]        = VK_NUMPAD8,
    [SDL_SCANCODE_KP_9]        = VK_NUMPAD9,
    [SDL_SCANCODE_KP_DIVIDE]   = VK_DIVIDE,
    [SDL_SCANCODE_KP_MULTIPLY] = VK_MULTIPLY,
    [SDL_SCANCODE_KP_MINUS]    = VK_SUBTRACT,
    [SDL_SCANCODE_KP_PLUS]     = VK_ADD,
    [SDL_SCANCODE_KP_ENTER]    = VK_RETURN,
    [SDL_SCANCODE_KP_PERIOD]   = VK_DECIMAL,

    // System keys
    [SDL_SCANCODE_PRINTSCREEN] = VK_SNAPSHOT,
    [SDL_SCANCODE_PAUSE]       = VK_PAUSE,
    [SDL_SCANCODE_APPLICATION] = VK_APPS,
    [SDL_SCANCODE_MENU]        = VK_MENU,
    [SDL_SCANCODE_SELECT]      = VK_SELECT,
    [SDL_SCANCODE_EXECUTE]     = VK_EXECUTE,
    [SDL_SCANCODE_HELP]        = VK_HELP,

    // Media keys
    [SDL_SCANCODE_MUTE]       = VK_VOLUME_MUTE,
    [SDL_SCANCODE_VOLUMEUP]   = VK_VOLUME_UP,
    [SDL_SCANCODE_VOLUMEDOWN] = VK_VOLUME_DOWN,

    // Browser keys
    [SDL_SCANCODE_AC_BACK]      = VK_BROWSER_BACK,
    [SDL_SCANCODE_AC_FORWARD]   = VK_BROWSER_FORWARD,
    [SDL_SCANCODE_AC_REFRESH]   = VK_BROWSER_REFRESH,
    [SDL_SCANCODE_AC_STOP]      = VK_BROWSER_STOP,
    [SDL_SCANCODE_AC_SEARCH]    = VK_BROWSER_SEARCH,
    [SDL_SCANCODE_AC_BOOKMARKS] = VK_BROWSER_FAVORITES,
    [SDL_SCANCODE_AC_HOME]      = VK_BROWSER_HOME,

    [SDL_SCANCODE_MINUS]          = VK_OEM_MINUS,   // -_
    [SDL_SCANCODE_EQUALS]         = VK_OEM_PLUS,    // =+
    [SDL_SCANCODE_LEFTBRACKET]    = VK_OEM_4,       // [{
    [SDL_SCANCODE_RIGHTBRACKET]   = VK_OEM_6,       // ]}
    [SDL_SCANCODE_BACKSLASH]      = VK_OEM_5,       // \|
    [SDL_SCANCODE_SEMICOLON]      = VK_OEM_1,       // ;:
    [SDL_SCANCODE_APOSTROPHE]     = VK_OEM_7,       // '"
    [SDL_SCANCODE_GRAVE]          = VK_OEM_3,       // `~
    [SDL_SCANCODE_COMMA]          = VK_OEM_COMMA,   // ,
    [SDL_SCANCODE_PERIOD]         = VK_OEM_PERIOD,  // .>
    [SDL_SCANCODE_SLASH]          = VK_OEM_2,       // /?
    [SDL_SCANCODE_NONUSBACKSLASH] = VK_OEM_102,
};

static inline bool wkernel_IsExtendedKey(SDL_Scancode scancode)
{
    switch ( scancode )
    {
        case SDL_SCANCODE_RALT:
        case SDL_SCANCODE_RCTRL:
        case SDL_SCANCODE_INSERT:
        case SDL_SCANCODE_DELETE:
        case SDL_SCANCODE_HOME:
        case SDL_SCANCODE_END:
        case SDL_SCANCODE_PAGEUP:
        case SDL_SCANCODE_PAGEDOWN:
        case SDL_SCANCODE_UP:
        case SDL_SCANCODE_DOWN:
        case SDL_SCANCODE_LEFT:
        case SDL_SCANCODE_RIGHT:
        case SDL_SCANCODE_NUMLOCKCLEAR:
        case SDL_SCANCODE_KP_DIVIDE:
        case SDL_SCANCODE_KP_ENTER:
        case SDL_SCANCODE_PRINTSCREEN:
        case SDL_SCANCODE_PAUSE:
            return true;
        default:
            return false;
    }
}

static inline WPARAM wkernel_TranslateToVK(SDL_Scancode scancode)
{
    if ( scancode >= 0 && scancode < SDL_SCANCODE_COUNT )
    {
        WPARAM vk = wkernel_sdlScancodeToVK[scancode];
        if ( vk != 0 )
        {
            return vk;
        }
    }
    return 0;
}

LPARAM wkernel_GetVKeyLParam(const SDL_KeyboardEvent* e, int vk)
{
    // Repeat count
    unsigned int repeatCount = (e->repeat) ? (e->repeat + 1) : 1;

    // Get the scan code
    unsigned int scanCode = 0;
    if ( vk != 0 )
    {
        scanCode = MapVirtualKey(vk, MAPVK_VK_TO_VSC);
    }

    // Extended key flag: typical for RALT, RCTRL, etc.
    unsigned int extended = wkernel_IsExtendedKey(e->scancode) ? 1 : 0;


    // Previous key state if repeat, assume was down
    unsigned int prevState  = (e->repeat) ? 1 : 0;

    // Transition state  0 for WM_KEYDOWN, 1 for WM_KEYUP
    unsigned int transition = (e->down) ? 0 : 1;

    LPARAM lParam = 0;
    lParam |= (repeatCount & 0xFFFF);           // Bits 0-15
    lParam |= ((scanCode & 0xFF) << 16);         // Bits 16-23
    lParam |= (extended << 24);                 // Bit 24 (KF_EXTENDED in HIWORD)
    // Bits 25-28 reserved (KF_DLGMODE, KF_MENUMODE would go here but we don't track dialog/menu state)
    lParam |= (prevState << 30);                // Bit 30 (KF_REPEAT)
    lParam |= (transition << 31);               // Bit 31 (KF_UP)

    return lParam;
}

static inline int J3DAPI wkernel_CreateWindow(HINSTANCE hInstance, int nShowCmd, LPCSTR lpWindowName);
static LRESULT CALLBACK wkernel_SubclassWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
static LRESULT CALLBACK wkernel_MainWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
static LRESULT wkernel_ProcessEvent(SDL_Event* event);

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
{
}

int J3DAPI wkernel_Run(HINSTANCE hinstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd, LPCSTR lpWindowName)
{
    J3D_UNUSED(hPrevInstance);

    // Added: Refactored to run main proc via callback
    if ( wkernel_pfProcess == NULL )
    {
        fprintf(stderr, "ERROR: wkernel_Run: No main process set!\n");
        return -1;
    }

    if ( wkernel_CreateWindow(hinstance, nShowCmd, lpWindowName) )
    {
        SDL_Quit();
        return -1;
    }

    // Set stdWin95 globals (will be adapted for SDL types later)
    stdWin95_SetWindow(wkernel_hwnd);
    stdWin95_SetInstance(hinstance);
    stdWin95_SetGuid((const GUID*)&wkernel_guid);
    stdWin95_SetGLContext(wkernel_glContext);
    stdWin95_SetSDLWindow(wkernel_sdlWindow);

    // Added: Refactored to run main proc via callback
    if ( wkernel_pfOnStartup )
    {
        if ( wkernel_pfOnStartup(lpCmdLine) )
        {
            SDL_GL_DestroyContext(wkernel_glContext);
            SDL_DestroyWindow(wkernel_sdlWindow);
            SDL_Quit();
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
                SDL_GL_DestroyContext(wkernel_glContext);
                SDL_DestroyWindow(wkernel_sdlWindow);
                SDL_Quit();
                return 0; // Fixed: Changed return code to 0
            }
            else
            {
                // Added: Log
                fprintf(stdout, "WARNING: wkernel_Run: Unhandled result code: %d, skipping...\n", result);
            }
        }

        // Fixed: Removed else statement to fix bug when result != 1 || result != -1
        result = wkernel_pfProcess();
    }

    SDL_GL_DestroyContext(wkernel_glContext);
    SDL_DestroyWindow(wkernel_sdlWindow);
    SDL_Quit();
    return result;
}

WKERNELPROC J3DAPI wkernel_SetProcessProc(WKERNELPROC pfProc)
{
    WKERNELPROC pfCurProc = wkernel_pfProcess;
    wkernel_pfProcess     = pfProc;
    return pfCurProc;
}

WKERNELSTARTUPPROC J3DAPI wkernel_SetStartupCallback(WKERNELSTARTUPPROC pfOnClose)
{
    WKERNELSTARTUPPROC pfCurProc = wkernel_pfOnStartup;
    wkernel_pfOnStartup          = pfOnClose;
    return pfCurProc;
}

WKERNELSHUTDOWNPROC J3DAPI wkernel_SetShutdownCallback(WKERNELSHUTDOWNPROC pfOnClose)
{
    WKERNELSHUTDOWNPROC pfCurProc = wkernel_pfOnShutdown;
    wkernel_pfOnShutdown          = pfOnClose;
    return pfCurProc;
}

// Non-blocking, TODO: rename to wkernel_ProcessEvents or wkernel_ProcessPendingEvents
int wkernel_PeekProcessEvents(void)
{
    // Check if there are any events wainting to be processed
    if ( !SDL_PollEvent(NULL) )
    {
        return 0; // No events
    }

    // Wait for event - should not block and immediately return
    SDL_Event event;
    while ( SDL_WaitEvent(&event) )
    {
        // TODO: Reserved for future multi-platform handling
    #ifndef _WIN32
        wkernel_ProcessEvent(&event);
    #endif

        // Check for quit (GetMessage returns 0 for WM_QUIT)
        if ( wkernel_quit_requested )
        {
            wkernel_quit_requested = false;
            return 1;
        }

        // Check if there are any more pending events
        if ( !SDL_PollEvent(NULL) )
        {
            return 0; // No more events
        }
    }

    fprintf(stderr, "ERROR: wkernel_PeekProcessEvents: Failed to peep events: %s\n", SDL_GetError());
    return -1;
}

// Blocking -> TODO: rename to wkernel_ProcessEventsWait
int wkernel_ProcessEvents(void)
{
    // Wait for an event
    SDL_Event event;
    while ( SDL_WaitEvent(&event) )
    {
        // TODO: Reserved for future multi-platform handling
    #ifndef _WIN32
        wkernel_ProcessEvent(&event);
    #endif

        // Exit if quit message received
        if ( wkernel_quit_requested )
        {
            wkernel_quit_requested = false;
            return 1;
        }

        // Check if there are any more pending events
        if ( !SDL_PollEvent(NULL) )
        {
            return 0; // No more events
        }
    }

    fprintf(stderr, "ERROR: wkernel_ProcessEvents: Failed to peep events: %s\n", SDL_GetError());
    return -1;
}

static LRESULT wkernel_ProcessEvent(SDL_Event* pEvent)
{
    // Note: Function reserved for future use
    if ( !pEvent )
    {
        return 0;
    }

    switch ( pEvent->type )
    {
        case SDL_EVENT_QUIT:
        {
            wkernel_quit_requested = true;
            return 1; // handled?
        }
        case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
        {
            if ( wkernel_pfOnShutdown )
            {
                wkernel_pfOnShutdown();
            }
            wkernel_quit_requested = true;
            return wkernel_MainWndProc(wkernel_hwnd, WM_CLOSE, 0, 0);
        }
        case SDL_EVENT_WINDOW_DESTROYED:
        {
            wkernel_quit_requested = true;
            return wkernel_MainWndProc(wkernel_hwnd, WM_DESTROY, 0, 0);
        }
        case SDL_EVENT_TEXT_INPUT:
        {
            for ( size_t i = 0; i < strlen(pEvent->text.text); i++ )
            {
                wkernel_MainWndProc(wkernel_hwnd, WM_CHAR, pEvent->text.text[i], 0);
            }
            return 1;
        }
        case SDL_EVENT_KEY_DOWN:
        {
            WPARAM vkCode = wkernel_TranslateToVK(pEvent->key.scancode);
            LPARAM lParam = wkernel_GetVKeyLParam(&pEvent->key, vkCode);

            // Dispatch WM_KEYDOWN
            bool handled = wkernel_MainWndProc(wkernel_hwnd, WM_KEYDOWN, vkCode, lParam);
            switch ( pEvent->key.scancode )
            {
                case SDL_SCANCODE_RETURN:
                case SDL_SCANCODE_ESCAPE:
                case SDL_SCANCODE_BACKSPACE:
                case SDL_SCANCODE_INSERT:
                case SDL_SCANCODE_TAB:
                case SDL_SCANCODE_END:
                case SDL_SCANCODE_HOME:
                    handled |= wkernel_MainWndProc(wkernel_hwnd, WM_CHAR, vkCode, lParam);
            }

            return handled;
        }
        case SDL_EVENT_KEY_UP:
        {
            WPARAM vkCode = wkernel_TranslateToVK(pEvent->key.scancode);
            LPARAM lParam = wkernel_GetVKeyLParam(&pEvent->key, vkCode);
            return wkernel_MainWndProc(wkernel_hwnd, WM_KEYUP, vkCode, lParam);
        }
        case SDL_EVENT_MOUSE_BUTTON_DOWN:
        {
            UINT uMsg = 0;
            if ( pEvent->button.button == SDL_BUTTON_LEFT )
            {
                uMsg = WM_LBUTTONDOWN;
            }
            else if ( pEvent->button.button == SDL_BUTTON_RIGHT )
            {
                uMsg = WM_RBUTTONDOWN;
            }
            else if ( pEvent->button.button == SDL_BUTTON_MIDDLE )
            {
                uMsg = WM_MBUTTONDOWN;
            }

            if ( uMsg )
            {
                LPARAM lParam = MAKELPARAM((int)pEvent->button.x, (int)pEvent->button.y);
                WPARAM wParam = 0;
                if ( SDL_GetModState() & SDL_KMOD_CTRL ) wParam |= MK_CONTROL;
                if ( SDL_GetModState() & SDL_KMOD_SHIFT ) wParam |= MK_SHIFT;
                return wkernel_MainWndProc(wkernel_hwnd, uMsg, wParam, lParam);
            }
        }
        break;
        case SDL_EVENT_MOUSE_BUTTON_UP:
        {
            UINT uMsg = 0;
            if ( pEvent->button.button == SDL_BUTTON_LEFT )
            {
                uMsg = WM_LBUTTONUP;
            }
            else if ( pEvent->button.button == SDL_BUTTON_RIGHT )
            {
                uMsg = WM_RBUTTONUP;
            }
            else if ( pEvent->button.button == SDL_BUTTON_MIDDLE )
            {
                uMsg = WM_MBUTTONUP;
            }

            if ( uMsg )
            {
                LPARAM lParam = MAKELPARAM((int)pEvent->button.x, (int)pEvent->button.y);
                WPARAM wParam = 0;
                if ( SDL_GetModState() & SDL_KMOD_CTRL ) wParam |= MK_CONTROL;
                if ( SDL_GetModState() & SDL_KMOD_SHIFT ) wParam |= MK_SHIFT;

                return wkernel_MainWndProc(wkernel_hwnd, uMsg, wParam, lParam);
            }
        }
        break;
        case SDL_EVENT_MOUSE_MOTION:
        {
            LPARAM lParam = MAKELPARAM((int)pEvent->motion.x, (int)pEvent->motion.y);
            WPARAM wParam = 0;
            if ( pEvent->motion.state & SDL_BUTTON_LMASK ) wParam |= MK_LBUTTON;
            if ( pEvent->motion.state & SDL_BUTTON_RMASK ) wParam |= MK_RBUTTON;
            if ( pEvent->motion.state & SDL_BUTTON_MMASK ) wParam |= MK_MBUTTON;
            if ( SDL_GetModState() & SDL_KMOD_CTRL ) wParam |= MK_CONTROL;
            if ( SDL_GetModState() & SDL_KMOD_SHIFT ) wParam |= MK_SHIFT;

            return wkernel_MainWndProc(wkernel_hwnd, WM_MOUSEMOVE, wParam, lParam);
        }
        case SDL_EVENT_MOUSE_WHEEL:
        {
            WPARAM wParam = MAKEWPARAM(0, (int)(pEvent->wheel.y * 120));
            LPARAM lParam = 0; // Would need mouse position
            return wkernel_MainWndProc(wkernel_hwnd, WM_MOUSEWHEEL, wParam, lParam);
        }
        case SDL_EVENT_WINDOW_RESIZED:
        {
            WPARAM wParam = SIZE_RESTORED;
            LPARAM lParam = MAKELPARAM(pEvent->window.data1, pEvent->window.data2);
            return wkernel_MainWndProc(wkernel_hwnd, WM_SIZE, wParam, lParam);
        }
        case SDL_EVENT_WINDOW_MINIMIZED:
            return wkernel_MainWndProc(wkernel_hwnd, WM_SIZE, SIZE_MINIMIZED, 0);
        case SDL_EVENT_WINDOW_MAXIMIZED:
            return wkernel_MainWndProc(wkernel_hwnd, WM_SIZE, SIZE_MAXIMIZED, 0);
        case SDL_EVENT_WINDOW_EXPOSED:
            return wkernel_MainWndProc(wkernel_hwnd, WM_PAINT, 0, 0);
        case SDL_EVENT_WINDOW_FOCUS_GAINED:
        {
            int bHandled = wkernel_MainWndProc(wkernel_hwnd, WM_ACTIVATEAPP, TRUE, 0);
            bHandled     = bHandled || wkernel_MainWndProc(wkernel_hwnd, WM_ACTIVATE, WA_ACTIVE, 0);
            return bHandled || wkernel_MainWndProc(wkernel_hwnd, WM_SETFOCUS, 0, 0);
        }

        case SDL_EVENT_WINDOW_FOCUS_LOST:
        {
            int bHandled = wkernel_MainWndProc(wkernel_hwnd, WM_ACTIVATEAPP, FALSE, 0);
            bHandled     = bHandled || wkernel_MainWndProc(wkernel_hwnd, WM_ACTIVATE, WA_INACTIVE, 0);
            return bHandled || wkernel_MainWndProc(wkernel_hwnd, WM_KILLFOCUS, 0, 0);
        }
        case SDL_EVENT_WINDOW_SHOWN:
            return wkernel_MainWndProc(wkernel_hwnd, WM_SHOWWINDOW, TRUE, 0);
        case SDL_EVENT_WINDOW_HIDDEN:
            return wkernel_MainWndProc(wkernel_hwnd, WM_SHOWWINDOW, FALSE, 0);
        default:
            break;
    }

    return 0;
}

void J3DAPI wkernel_SetWindowStyle(LONG dwNewLong)
{
    if ( !wkernel_sdlWindow )
    {
        return;
    }

    wkernel_currentStyle = dwNewLong;

    // Handle WS_POPUP
    if ( dwNewLong & WS_POPUP )
    {
        SDL_SetWindowBordered(wkernel_sdlWindow, true);
    }
    // Handle WS_OVERLAPPEDWINDOW (normal window with border, title, etc)
    else if ( (dwNewLong & WS_OVERLAPPEDWINDOW) == WS_OVERLAPPEDWINDOW )
    {
        SDL_SetWindowBordered(wkernel_sdlWindow, true);
        SDL_SetWindowResizable(wkernel_sdlWindow, true);
    }
    else
    {
        // Handle individual flags
        if ( dwNewLong & WS_BORDER )
        {
            SDL_SetWindowBordered(wkernel_sdlWindow, true);
        }

        if ( dwNewLong & WS_CAPTION )
        {
        }

        if ( dwNewLong & WS_THICKFRAME )
        {
            SDL_SetWindowResizable(wkernel_sdlWindow, false);
        }
    }

    // Handle WS_VISIBLE
    if ( dwNewLong & WS_VISIBLE )
    {
        SDL_ShowWindow(wkernel_sdlWindow);
    }
}

BOOL J3DAPI wkernel_SetWindowSize(int width, int height)
{
    if ( !wkernel_sdlWindow )
    {
        return FALSE;
    }

    return SDL_SetWindowSize(wkernel_sdlWindow, width, height) ? TRUE : FALSE;
}

void J3DAPI wkernel_SetWindowProc(WKERNELWNDPROC pfProc)
{
    wkernel_pfWndProc = pfProc;
}

int J3DAPI wkernel_CreateWindow(HINSTANCE hInstance, int nShowCmd, LPCSTR lpWindowName)
{
    J3D_UNUSED(nShowCmd);
    J3D_UNUSED(hInstance);

    // Initialize SDL
    if ( !SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS) )
    {
        fprintf(stderr, "ERROR: wkernel_Run: Failed to initialize SDL: %s\n", SDL_GetError());
        return -1;
    }

    // Set OpenGL attributes
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_COMPATIBILITY); // TODO: test if SDL_GL_CONTEXT_PROFILE_CORE works (OpenGL core profile - deprecated functions are disabled)
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_SHARE_WITH_CURRENT_CONTEXT, 1);

    // TODO: following only for windows and linux only
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG);

    // Get display mode for initial size
    SDL_DisplayID displayID      = SDL_GetPrimaryDisplay();
    const SDL_DisplayMode* pMode = SDL_GetCurrentDisplayMode(displayID);
    if ( !pMode )
    {
        fprintf(stderr, "ERROR: wkernel_CreateWindow: Failed to get display mode: %s\n", SDL_GetError());
        return 1;
    }

    // Check if window already exists
    if ( wkernel_sdlWindow )
    {
        fprintf(stdout, "INFO: wkernel_CreateWindow: An existing instance of window already running!\n");
        return 1;
    }

    // Create fullscreen borderless window (WS_POPUP | WS_VISIBLE equivalent)
    wkernel_sdlWindow = SDL_CreateWindow(
        lpWindowName,
        pMode->w,
        pMode->h,
        SDL_WINDOW_OPENGL | SDL_WINDOW_HIGH_PIXEL_DENSITY
    );

    if ( !wkernel_sdlWindow )
    {
        fprintf(stderr, "ERROR: wkernel_CreateWindow: Failed to create window: %s\n", SDL_GetError());
        return 1;
    }

    // Set initial smaller size
    wkernel_SetWindowSize(100, 25);

    // Show and raise window
    SDL_ShowWindow(wkernel_sdlWindow);
    SDL_RaiseWindow(wkernel_sdlWindow);
    SDL_SetWindowFocusable(wkernel_sdlWindow, true);

    // Create OpenGL context
    wkernel_glContext = SDL_GL_CreateContext(wkernel_sdlWindow);
    if ( !wkernel_glContext )
    {
        fprintf(stderr, "ERROR: wkernel_CreateWindow: Failed to create OpenGL context: %s\n", SDL_GetError());
        SDL_DestroyWindow(wkernel_sdlWindow);
        return 1;
    }

    // Initialize GLAD
    if ( !gladLoadGLLoader((GLADloadproc)SDL_GL_GetProcAddress) )
    {
        fprintf(stderr, "ERROR: wkernel_CreateWindow: Failed to initialize GLAD\n");
        SDL_GL_DestroyContext(wkernel_glContext);
        SDL_DestroyWindow(wkernel_sdlWindow);
        return 1;
    }

    SDL_GL_MakeCurrent(wkernel_sdlWindow, wkernel_glContext);

    // Get HWND
    SDL_PropertiesID props = SDL_GetWindowProperties(wkernel_sdlWindow);
    wkernel_hwnd           = (HWND)SDL_GetPointerProperty(props, SDL_PROP_WINDOW_WIN32_HWND_POINTER, NULL);
    if ( !wkernel_hwnd )
    {
        fprintf(stderr, "ERROR: wkernel_CreateWindow: Failed to retrieve SDL window HWND\n");
        SDL_GL_DestroyContext(wkernel_glContext);
        SDL_DestroyWindow(wkernel_sdlWindow);
        return 1;
    }

    // Hook on windows VM event stream (win only)
    wkernel_sdlWndProc = (WNDPROC)SetWindowLongPtr(wkernel_hwnd, GWLP_WNDPROC, (LONG_PTR)wkernel_SubclassWndProc);

    // Enable receiving SDL_EVENT_TEXT_INPUT
    // TODO: Reserved for future use
#ifndef _WIN32
    SDL_StartTextInput(wkernel_sdlWindow);
#endif

    return 0;
}

static LRESULT CALLBACK wkernel_SubclassWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    if ( wkernel_MainWndProc(hwnd, uMsg, wParam, lParam) )
    {
        return 0;
    }

    // Not handled by us, pass to SDL's original WndProc
    if ( wkernel_sdlWndProc )
    {
        return CallWindowProc(wkernel_sdlWndProc, hwnd, uMsg, wParam, lParam);
    }

    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

LRESULT CALLBACK wkernel_MainWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    bool bProcess = true;
    if ( uMsg == WM_QUIT )
    {
        wkernel_quit_requested = true;
    }

    if ( uMsg == WM_SYSCOMMAND )
    {
        if ( (wParam & 0xFFF0) == SC_CLOSE ) // Added
        {
            wkernel_quit_requested = true;
            bProcess               = false;
        }
    }

    if ( uMsg == WM_DESTROY )
    {
        wkernel_quit_requested = true;
        bProcess               = false;
    }

    if ( uMsg == WM_CLOSE )
    {
        if ( wkernel_pfOnShutdown )
        {
            wkernel_pfOnShutdown();
        }
        bProcess               = false;
        wkernel_quit_requested = true;
    }

    int retValue;
    if ( bProcess && wkernel_pfWndProc && wkernel_pfWndProc(hWnd, uMsg, wParam, lParam, &retValue) )
    {
        return retValue;
    }

    // Mark that event should be further processed
    return 0;
}
#ifndef JONES3D_JONESMAINSYS_H
#define JONES3D_JONESMAINSYS_H

#include <stdbool.h>
#include <std/types.h>

J3D_EXTERN_C_START

// Returns the display environment built at startup; NULL before that. Read-only.
StdDisplayEnvironment* JonesMain_GetStartupDisplayEnv(void); // Implemented in Jones3D/Main/JonesMain.c

// Returns the current video mode built from Jones.cfg at startup. Read-only.
StdVideoMode* JonesMain_GetCurVideoMode(void); // Implemented in Jones3D/Main/JonesMain.c

// Starts the timer that hides the quicksave text.
void JonesMainSys_StartQuickSaveTimer(tSysWindow hWnd); // Implemented in Jones3D/Platform/Win32/JonesMainWin32.c

// Shows the game window after activation or resume.
void JonesMainSys_ShowWindow(tSysWindow hWnd); // Implemented in Jones3D/Platform/Win32/JonesMainWin32.c

// Posts the close message to the main game window; returns the PostMessage result.
int JonesMainSys_PostCloseMessage(void); // Implemented in Jones3D/Platform/Win32/JonesMainWin32.c

// Installs the game window message handler for the wkernel event loop.
void JonesMainSys_UseGameWindowProc(void); // Implemented in Jones3D/Platform/Win32/JonesMainWin32.c

// Installs the intro window message handler for the wkernel event loop.
void JonesMainSys_UseIntroWindowProc(void); // Implemented in Jones3D/Platform/Win32/JonesMainWin32.c

// Removes the window message handler so DefWindowProc handles events.
void JonesMainSys_ClearWindowProc(void); // Implemented in Jones3D/Platform/Win32/JonesMainWin32.c

// Copies the computer name of the host into pDst of at most size characters. Returns true on
// success; on failure it follows the GetComputerName semantics and may leave pDst untouched.
bool JonesMainSys_GetComputerName(char* pDst, size_t size); // Implemented in Jones3D/Platform/Win32/JonesMainWin32.c

// Registers the RTI hook targets that now live in the Win32 platform files.
void JonesMainSys_InstallHooks(void); // Implemented in Jones3D/Platform/Win32/JonesMainWin32.c

// Quick save (F5): saves the game into the player's quicksave slot.
void JonesMain_OnQuickSaveKey(tSysWindow hWnd); // Implemented in Jones3D/Main/JonesMain.c

// Quick load (F8): loads the player's quicksave or last gamesave.
void JonesMain_OnQuickLoadKey(void); // Implemented in Jones3D/Main/JonesMain.c

// Screenshot (F12): makes a screen shot of the current frame.
void JonesMain_OnScreenshotKey(void); // Implemented in Jones3D/Main/JonesMain.c

// Hides the quicksave on-screen text when its show timer expires.
void JonesMain_OnQuickSaveTextTimer(void); // Implemented in Jones3D/Main/JonesMain.c

// Feeds one character of console input into the console.
void JonesMain_OnChar(char chr); // Implemented in Jones3D/Main/JonesMain.c

// True while the process loop is only pumping window events.
bool JonesMain_IsProcessingWindowEvents(void); // Implemented in Jones3D/Main/JonesMain.c

// System suspend: marks the system state and deactivates the game.
void JonesMain_OnSystemSuspend(tSysWindow hWnd); // Implemented in Jones3D/Main/JonesMain.c

// System resume: clears the suspend flag and reactivates the game.
void JonesMain_OnSystemResume(tSysWindow hWnd); // Implemented in Jones3D/Main/JonesMain.c

// Handles the game gaining or losing focus, refreshing display state.
void J3DAPI JonesMain_OnAppActivate(tSysWindow hWnd, int bActivated); // Implemented in Jones3D/Main/JonesMain.c

// Skips the currently playing intro movie.
void JonesMain_OnSkipIntro(void); // Implemented in Jones3D/Main/JonesMain.c

J3D_EXTERN_C_END

#endif // JONES3D_JONESMAINSYS_H

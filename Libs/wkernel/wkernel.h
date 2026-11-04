#ifndef WKERNEL_WKERNEL_H
#define WKERNEL_WKERNEL_H
#include <j3dcore/j3d.h>
#include <wkernel/types.h>

// TODO: move module to std

J3D_EXTERN_C_START

// Altered: wkernel_Run, wkernel_SetWindowStyle and wkernel_SetWindowProc moved to
// wkernel/wkernelWin32.h with the WKERNELWNDPROC typedef; they use Win32 types.
// Added: wkernel_SetWindowSize returns int instead of BOOL, matching the int
// error convention the other wkernel functions use.
int J3DAPI wkernel_SetWindowSize(int width, int height);
int wkernel_PeekProcessEvents(void);
int wkernel_ProcessEvents(void);

// Added: Window styles for windowed and fullscreen display
void wkernel_SetWindowedStyle(void);   // wkernel_SetWindowStyle(WS_VISIBLE | WS_OVERLAPPEDWINDOW)
void wkernel_SetFullscreenStyle(void); // wkernel_SetWindowStyle(WS_POPUP | WS_VISIBLE)

WKERNELPROC J3DAPI wkernel_SetProcessProc(WKERNELPROC pfProc);
WKERNELSTARTUPPROC J3DAPI wkernel_SetStartupCallback(WKERNELSTARTUPPROC pfOnStartup);
WKERNELSHUTDOWNPROC J3DAPI wkernel_SetShutdownCallback(WKERNELSHUTDOWNPROC pfOnShutdown);

// Added: for the Babylon column's wkernel (wkernelBabylon.cpp): it hands the game window's
// focus changes, taken from the Babylon platform event queue, to this callback; that is how
// JonesMain_OnAppActivate runs there without Win32 messages. The Win32 wkernel.c does not need
// it (its window procedure calls JonesMain_OnAppActivate directly); wkernelNull.c stores it and
// never calls it. Returns the previous callback, like wkernel_SetProcessProc.
typedef void (J3DAPI* WKERNELACTIVATEPROC)(int bActive);
WKERNELACTIVATEPROC J3DAPI wkernel_SetActivateCallback(WKERNELACTIVATEPROC pfProc);

void wkernel_ResetGlobals(void);

J3D_EXTERN_C_END
#endif // WKERNEL_WKERNEL_H

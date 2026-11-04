// Added: Null version of the platform side of Libs/wkernel/wkernel.c (wkernelRun, window
// creation and window_message dispatch, plus wkernel_SetWindowProc, stay Win32-only).
// There is no window and no message queue on this platform: both event loop entry points
// report "no message waiting" while WM_QUIT never occurs.
#include <j3dcore/j3d.h>

#include <wkernel/wkernel.h>

// Added: 0 is PeekProcessEvents' "no message is waiting" answer; the process
// never exits on this platform.
int wkernel_PeekProcessEvents(void)
{
    return 0;
}

int wkernel_ProcessEvents(void)
{
    return 0;
}

void wkernel_SetWindowedStyle(void)
{
}

void wkernel_SetFullscreenStyle(void)
{
}

// Fixed: Returns 1, the SetWindowPos BOOL success value the Win32 version returns;
// callers do not check the result, but the contract keeps the success answer.
int J3DAPI wkernel_SetWindowSize(int width, int height)
{
    J3D_UNUSED(width);
    J3D_UNUSED(height);
    return 1;
}

// Added: the activation callback exists for the Babylon column's wkernel (wkernelBabylon.cpp).
// It is stored here so the Null column links; there is no focus event on this platform, so it
// is never called.
static WKERNELACTIVATEPROC wkernel_pfActivate = NULL;

WKERNELACTIVATEPROC J3DAPI wkernel_SetActivateCallback(WKERNELACTIVATEPROC pfProc)
{
    WKERNELACTIVATEPROC pfCurProc = wkernel_pfActivate;
    wkernel_pfActivate = pfProc;
    return pfCurProc;
}

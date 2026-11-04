// Added: the Babylon column's platform side of Libs/wkernel/wkernel.c (wkernel_Run, window
// creation and the window-proc dispatch stay Win32-only; wkernelNull.c is the Null column's
// answer to the same header). There is no Win32 message queue here: the events come from the
// Babylon platform of the std adapter context, drained through its one pump stdBabylonEvents_Pump.
// A recorded quit or game-window close is answered like the WM_QUIT of the Win32 loop, and the
// game window's recorded focus changes go to the activation callback wkernel_SetActivateCallback
// installs, the way the Win32 window procedure calls JonesMain_OnAppActivate.
#include <chrono>
#include <cstddef>
#include <optional>

#include <babylon/platform/Event.h>
#include <babylon/platform/Window.h>
#include <babylon/time/Clock.h>

#include <std/Babylon/stdBabylon.h>
#include <std/Babylon/stdBabylonEvents.h>

#include <wkernel/wkernel.h>

namespace
{
    WKERNELACTIVATEPROC wkernelBabylon_pfActivate = nullptr;

    // Hands every recorded focus change, oldest first, to the activation callback.
    void wkernelBabylon_DeliverFocusChanges()
    {
        while ( const std::optional<bool> bActive = stdBabylonEvents_TakeFocusChange() )
        {
            if ( wkernelBabylon_pfActivate )
            {
                wkernelBabylon_pfActivate(*bActive ? 1 : 0);
            }
        }
    }
}

extern "C" {

int wkernel_PeekProcessEvents(void)
{
    stdBabylonEvents_Pump();
    wkernelBabylon_DeliverFocusChanges();

    // 1 is the WM_QUIT answer of the Win32 version; the game ends its loop on it.
    return stdBabylonEvents_TakeQuitRequest() ? 1 : 0;
}

int wkernel_ProcessEvents(void)
{
    StdBabylonContext& rContext = stdBabylon_RequireContext("wkernel_ProcessEvents");

    // The Win32 version blocks in GetMessage until a message arrives; the game uses this entry
    // while it is deactivated, so waiting for the next event is the contract here too.
    for ( ;; )
    {
        if ( stdBabylonEvents_Pump() != 0 )
        {
            wkernelBabylon_DeliverFocusChanges();
            return stdBabylonEvents_TakeQuitRequest() ? 1 : 0;
        }

        rContext.pClock->sleepFor(std::chrono::milliseconds(10));
    }
}

// Returns 1, the SetWindowPos BOOL success value the Win32 version returns; callers do not check
// the result, but the contract keeps the success answer.
int J3DAPI wkernel_SetWindowSize(int width, int height)
{
    StdBabylonContext* pContext = stdBabylon_GetContext();
    if ( pContext && pContext->pWindow )
    {
        // The resize answer is ignored: the Win32 callers do not check it either.
        static_cast<void>(pContext->pWindow->resize(babylon::platform::Size{ width, height }));
    }

    return 1;
}

void wkernel_SetWindowedStyle(void)
{
    // TODO: fullscreen comes with the presentation work (M9); Babylon's Window has no fullscreen yet.
}

void wkernel_SetFullscreenStyle(void)
{
    // TODO: fullscreen comes with the presentation work (M9); Babylon's Window has no fullscreen yet.
}

WKERNELACTIVATEPROC J3DAPI wkernel_SetActivateCallback(WKERNELACTIVATEPROC pfProc)
{
    WKERNELACTIVATEPROC pfCurProc = wkernelBabylon_pfActivate;
    wkernelBabylon_pfActivate = pfProc;
    return pfCurProc;
}

} // extern "C"

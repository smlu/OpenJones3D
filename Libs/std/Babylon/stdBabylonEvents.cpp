// Added: the Babylon column's one event pump, the owner of the shared babylon::input::InputState.
// See stdBabylonEvents.h. 0 is never a valid window id, so a context without a game window (the
// headless run) matches no window event and records no quit from one; a QuitRequested carries no
// window and is recorded either way.
// Adapter code: no Fixed/Added markers inside.
#include "stdBabylonEvents.h"

#include <babylon/platform/Event.h>
#include <babylon/platform/Platform.h>
#include <babylon/platform/Window.h>

#include <cstddef>
#include <deque>
#include <optional>

#include "stdBabylon.h"

namespace
{
    // The state every pumped event goes into; owned here, read through stdBabylonEvents_InputState().
    babylon::input::InputState inputState;

    // True while a recorded quit or game-window close waits to be taken by the window adapter.
    bool quitRequested = false;

    // The recorded game-window focus changes, oldest first; true = the window gained focus.
    std::deque<bool> focusChanges;
}

std::size_t stdBabylonEvents_Pump()
{
    StdBabylonContext& rContext = stdBabylon_RequireContext("stdBabylonEvents_Pump");

    const babylon::platform::WindowId windowId = rContext.pWindow ? rContext.pWindow->id() : 0;

    std::size_t numEvents = 0;
    for ( ;; )
    {
        const std::optional<babylon::platform::Event> pEvent = rContext.pPlatform->pollEvent();
        if ( !pEvent )
        {
            break;
        }

        ++numEvents;

        // Events that carry no input (window, quit) are ignored by InputState, except the focus
        // loss, which releases every held key and button.
        inputState.apply(*pEvent);

        if ( std::holds_alternative<babylon::platform::QuitRequested>(*pEvent) )
        {
            quitRequested = true;
            continue;
        }

        if ( windowId == 0 )
        {
            continue; // no game window: its close and focus events are ignored
        }

        const babylon::platform::WindowCloseRequested* pClose =
            std::get_if<babylon::platform::WindowCloseRequested>(&*pEvent);
        if ( pClose && pClose->window == windowId )
        {
            quitRequested = true;
            continue;
        }

        const babylon::platform::WindowFocusChanged* pFocus =
            std::get_if<babylon::platform::WindowFocusChanged>(&*pEvent);
        if ( pFocus && pFocus->window == windowId )
        {
            focusChanges.push_back(pFocus->focused);
        }
    }

    return numEvents;
}

babylon::input::InputState& stdBabylonEvents_InputState()
{
    return inputState;
}

bool stdBabylonEvents_TakeQuitRequest()
{
    const bool bQuit = quitRequested;
    quitRequested = false;
    return bQuit;
}

std::optional<bool> stdBabylonEvents_TakeFocusChange()
{
    if ( focusChanges.empty() )
    {
        return std::nullopt;
    }

    const bool bGained = focusChanges.front();
    focusChanges.pop_front();
    return bGained;
}

#ifdef J3D_TEST
void stdBabylonEvents_ResetForTest()
{
    inputState = babylon::input::InputState();
    quitRequested = false;
    focusChanges.clear();
}
#endif

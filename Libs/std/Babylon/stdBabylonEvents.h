// Added: the Babylon column's one event pump. The composition root owns the platform whose queue
// this drains; the stdControl device read and the wkernel window adapter both call stdBabylonEvents_Pump
// instead of owning a pump of their own, so one event is taken exactly once and every consumer sees
// the same records. Input events go into the shared babylon::input::InputState; a QuitRequested, and
// a WindowCloseRequested or WindowFocusChanged for the context's game window (StdBabylonContext::pWindow),
// are recorded here for the window adapter to take.
// Adapter code: no Fixed/Added markers inside.
#ifndef STD_STDBABYLONEVENTS_H
#define STD_STDBABYLONEVENTS_H
#ifndef __cplusplus
#error "stdBabylonEvents.h is C++ only: it names babylon::input::InputState and std::optional."
#endif

#include <babylon/input/InputState.h>

#include <cstddef>
#include <optional>

// Drains the context platform's event queue once: input events go into the shared InputState; a
// QuitRequested, and a WindowCloseRequested or WindowFocusChanged for the context's game window
// (pWindow; ignored when there is none), are recorded for the window adapter. Returns the number
// of events taken from the queue. Safe to call any number of times per frame.
std::size_t stdBabylonEvents_Pump();

// The input state every pumped event went into (owned here, file-static).
babylon::input::InputState& stdBabylonEvents_InputState();

// True once for each recorded quit or close request (the WM_QUIT the window adapter reports).
bool stdBabylonEvents_TakeQuitRequest();

// The oldest recorded focus change of the game window (true = gained), or nullopt.
std::optional<bool> stdBabylonEvents_TakeFocusChange();

#ifdef J3D_TEST
void stdBabylonEvents_ResetForTest(); // clears the state, the quit flag and the focus queue
#endif

#endif // STD_STDBABYLONEVENTS_H

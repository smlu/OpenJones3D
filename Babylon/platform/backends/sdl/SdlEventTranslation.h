// SPDX-License-Identifier: MPL-2.0
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

// Private to the SDL platform backend: the event mapping of SdlPlatform,
// split out as one pure function so it can be tested without a device. This
// header is not installed and not part of the module's public interface.

#pragma once

#include <babylon/platform/Event.h>
#include <babylon/platform/Keys.h>

#include <SDL3/SDL_events.h>
#include <SDL3/SDL_scancode.h>

#include <optional>

namespace babylon::platform::sdl
{

    // Translates one SDL event into the Babylon event the queue carries.
    // Returns nullopt for the events Babylon does not deliver: SDL's own
    // display, sensor, clipboard and audio events, mouse buttons beyond X2
    // and gamepad buttons beyond DpadRight. Pure: it calls no SDL function
    // with side effects, so tests can feed it hand-made SDL_Event values.
    std::optional<Event> translateEvent(const SDL_Event& event);

    // The physical key of an SDL scancode, independent of the keyboard
    // layout. A scancode the Key table does not name gives Key::Unknown, so
    // every value in SDL's range translates.
    Key translateScancode(SDL_Scancode scancode);

}

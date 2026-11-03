// SPDX-License-Identifier: MPL-2.0
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include <babylon/platform/Platform.h>

#include <memory>

namespace babylon::platform
{

    // Platform on SDL3's window system (ADR-0009): windows over
    // SDL_CreateWindow and the event queue over SDL_PollEvent. It
    // initializes the SDL video and gamepad subsystems (SDL_InitSubSystem)
    // in the factory and quits them again, gamepad first, in the destructor,
    // opening each gamepad as it is connected and closing it again when it
    // goes, so the platform must outlive its windows and gamepads. SDL's
    // video subsystem belongs to the whole process, so a second platform
    // while one exists is refused: nullptr and one warning. Both cases need
    // no display: with SDL_VIDEO_DRIVER=offscreen the backend runs headless.
    // The returned platform is owned by the caller.
    std::unique_ptr<Platform> createSdlPlatform();

}

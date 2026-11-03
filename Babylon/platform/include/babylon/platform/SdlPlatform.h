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
    // initializes the SDL video subsystem (SDL_InitSubSystem) in the factory
    // and releases it (SDL_QuitSubSystem) in the destructor, so the platform
    // must outlive its windows. SDL's video subsystem belongs to the whole
    // process, so a second platform while one exists is refused: nullptr and
    // one warning. Both cases need no display: with SDL_VIDEO_DRIVER=offscreen
    // the backend runs headless. The returned platform is owned by the
    // caller.
    std::unique_ptr<Platform> createSdlPlatform();

}

// SPDX-License-Identifier: MPL-2.0
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include <babylon/time/Clock.h>

#include <memory>

namespace babylon::time
{

    // A Clock on SDL's monotonic nanosecond counter (SDL_GetTicksNS) and
    // SDL_DelayNS. Needs no SDL_Init. The returned clock is owned by the
    // caller.
    std::unique_ptr<Clock> createSdlClock();

}

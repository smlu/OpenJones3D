// SPDX-License-Identifier: MPL-2.0
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <babylon/time/SdlClock.h>

#include <SDL3/SDL_timer.h>

namespace babylon::time
{

namespace
{

    // Clock on SDL's monotonic nanosecond counter. Both calls work without
    // SDL_Init (ADR-0017); the counter starts at an unspecified origin, so
    // only differences of now() mean anything, as Clock requires.
    class SdlClock final : public Clock
    {
    public:
        Duration now() const override
        {
            // The counter has run for far less than 2^63 ns on any real
            // machine, so this keeps every value the platform can produce.
            return Duration(static_cast<Duration::rep>(SDL_GetTicksNS()));
        }

        void sleepFor(Duration duration) override
        {
            // Zero or negative durations return at once, as Clock requires;
            // SDL_DelayNS waits for at least the duration it is given.
            if ( duration > Duration::zero() )
            {
                SDL_DelayNS(static_cast<Uint64>(duration.count()));
            }
        }
    };

}

    std::unique_ptr<Clock> createSdlClock()
    {
        return std::make_unique<SdlClock>();
    }

}

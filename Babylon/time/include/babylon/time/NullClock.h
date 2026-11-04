// SPDX-License-Identifier: MPL-2.0
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include <babylon/time/Clock.h>

namespace babylon::time
{

    // Clock that reads nothing but its own counter: time moves only through
    // advance() and sleepFor(); nothing reads a real clock. Starts at the given
    // origin offset (default zero). sleepFor(d) advances by d without waiting,
    // so a frame limiter running on a NullClock is deterministic. Public
    // because headless runs and tests drive it.
    class NullClock final : public Clock
    {
    public:
        explicit NullClock(Duration start = Duration::zero());

        Duration now() const override;
        void sleepFor(Duration duration) override;

        // Moves time forward. Returns false and logs nothing for negative
        // steps, leaving time unchanged.
        [[nodiscard]] bool advance(Duration step);

    private:
        Duration m_now;
    };

}

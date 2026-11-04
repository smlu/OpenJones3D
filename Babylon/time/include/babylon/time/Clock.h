// SPDX-License-Identifier: MPL-2.0
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include <chrono>

namespace babylon::time
{

    using Duration = std::chrono::nanoseconds;

    // A monotonic clock. now() never decreases. The origin is unspecified: only
    // differences between two now() values of the same clock mean anything. Not
    // thread-safe unless a backend says so.
    class Clock
    {
    public:
        virtual ~Clock() = default;

        // Time since the clock's origin.
        virtual Duration now() const = 0;

        // Waits for at least the given duration, then returns. Zero or negative
        // durations return at once.
        virtual void sleepFor(Duration duration) = 0;
    };

}

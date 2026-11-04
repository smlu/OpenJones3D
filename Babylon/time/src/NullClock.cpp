// SPDX-License-Identifier: MPL-2.0
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <babylon/time/NullClock.h>

namespace babylon::time
{

    NullClock::NullClock(Duration start) :
        m_now(start)
    {
    }

    Duration NullClock::now() const
    {
        return m_now;
    }

    void NullClock::sleepFor(Duration duration)
    {
        // Deterministic stand-in for waiting: zero or negative durations return
        // at once, a positive one moves time by exactly the duration.
        if ( duration > Duration::zero() )
        {
            m_now += duration;
        }
    }

    bool NullClock::advance(Duration step)
    {
        if ( step < Duration::zero() )
        {
            return false;
        }
        m_now += step;
        return true;
    }

}

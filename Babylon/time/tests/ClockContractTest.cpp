// SPDX-License-Identifier: MPL-2.0
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <babylon/time/Clock.h>
#include <babylon/time/NullClock.h>
#include <babylon/time/SdlClock.h>

#include <doctest.h>

#include <memory>

namespace
{

    // The Clock contract, run against one backend. movesOnlyWhenTold is true
    // for clocks that read nothing but their own counter (NullClock): there a
    // zero or negative sleepFor must leave now() unchanged. A clock on a real
    // time source (SDL) only has to return from those calls.
    void checkClockContract(babylon::time::Clock& clock, bool movesOnlyWhenTold)
    {
        // now() never decreases over 1,000 consecutive calls.
        babylon::time::Duration previous = clock.now();
        for ( int i = 0; i < 1000; ++i )
        {
            const babylon::time::Duration current = clock.now();
            CHECK(current >= previous);
            previous = current;
        }

        // sleepFor(2 ms) returns no earlier than 2 ms after it was called.
        const babylon::time::Duration before = clock.now();
        clock.sleepFor(babylon::time::Duration{2'000'000});
        CHECK(clock.now() - before >= babylon::time::Duration{2'000'000});

        // Zero and negative durations return at once; only a clock that moves
        // when told can promise that time did not change.
        const babylon::time::Duration beforeNoop = clock.now();
        clock.sleepFor(babylon::time::Duration::zero());
        clock.sleepFor(babylon::time::Duration{-1});
        if ( movesOnlyWhenTold )
        {
            CHECK(clock.now() == beforeNoop);
        }
    }

}

TEST_CASE("The Null clock honors the Clock contract")
{
    babylon::time::NullClock clock;
    checkClockContract(clock, true);
}

TEST_CASE("The SDL clock honors the Clock contract")
{
    const std::unique_ptr<babylon::time::Clock> clock = babylon::time::createSdlClock();
    REQUIRE(clock != nullptr);
    checkClockContract(*clock, false);
}

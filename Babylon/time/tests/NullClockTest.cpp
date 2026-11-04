// SPDX-License-Identifier: MPL-2.0
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <babylon/time/NullClock.h>

#include <doctest.h>

TEST_CASE("A NullClock starts at zero or at the given origin offset")
{
    CHECK(babylon::time::NullClock().now() == babylon::time::Duration::zero());
    CHECK(babylon::time::NullClock(babylon::time::Duration{1'000'000}).now()
        == babylon::time::Duration{1'000'000});
}

TEST_CASE("advance moves the NullClock forward by exactly the step")
{
    babylon::time::NullClock clock;

    // Three frames at 60 Hz rounded up to nanoseconds.
    CHECK(clock.advance(babylon::time::Duration{16'666'667}));
    CHECK(clock.advance(babylon::time::Duration{16'666'667}));
    CHECK(clock.advance(babylon::time::Duration{16'666'667}));
    CHECK(clock.now() == babylon::time::Duration{50'000'001});
}

TEST_CASE("advance rejects a negative step and leaves the NullClock unchanged")
{
    babylon::time::NullClock clock(babylon::time::Duration{10'000});

    CHECK_FALSE(clock.advance(babylon::time::Duration{-1}));
    CHECK(clock.now() == babylon::time::Duration{10'000});
}

TEST_CASE("sleepFor advances the NullClock by exactly the duration without waiting")
{
    babylon::time::NullClock clock;

    clock.sleepFor(babylon::time::Duration{1'000'000});
    CHECK(clock.now() == babylon::time::Duration{1'000'000});

    // The zero and negative cases of the Clock contract, exact on a NullClock.
    clock.sleepFor(babylon::time::Duration::zero());
    clock.sleepFor(babylon::time::Duration{-1});
    CHECK(clock.now() == babylon::time::Duration{1'000'000});
}

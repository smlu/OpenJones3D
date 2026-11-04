// SPDX-License-Identifier: MPL-2.0
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <babylon/log/CaptureSink.h>
#include <babylon/log/Log.h>

#include <doctest.h>

#include <string>

TEST_CASE("CaptureSink captures records in order and owns a copy of the message")
{
    babylon::log::CaptureSink capture;

    std::string text = "first";
    babylon::log::write(babylon::log::Level::Info, text);
    text = "changed";
    babylon::log::write(babylon::log::Level::Error, "error");

    REQUIRE(capture.records().size() == 2);
    CHECK(capture.records()[0].level == babylon::log::Level::Info);
    CHECK(capture.records()[0].message == "first");
    CHECK(capture.records()[1].level == babylon::log::Level::Error);
    CHECK(capture.records()[1].message == "error");
}

TEST_CASE("CaptureSink clear empties the records and the sink keeps capturing")
{
    babylon::log::CaptureSink capture;

    babylon::log::error("one");
    babylon::log::error("two");
    REQUIRE(capture.records().size() == 2);

    capture.clear();
    CHECK(capture.records().empty());

    babylon::log::error("three");
    REQUIRE(capture.records().size() == 1);
    CHECK(capture.records()[0].message == "three");
}

TEST_CASE("A destroyed CaptureSink is detached and only the outer sink remains")
{
    babylon::log::CaptureSink outer;
    {
        babylon::log::CaptureSink inner;
        babylon::log::error("inside");
        CHECK(inner.records().size() == 1);
        CHECK(outer.records().size() == 1);
    }

    babylon::log::error("outside");
    REQUIRE(outer.records().size() == 2);
    CHECK(outer.records()[0].message == "inside");
    CHECK(outer.records()[1].message == "outside");
}

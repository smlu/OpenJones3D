// SPDX-License-Identifier: MPL-2.0
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <babylon/core/Parse.h>

#include <doctest.h>

#include <clocale>
#include <cmath>
#include <limits>
#include <optional>
#include <string_view>

namespace
{

    // parseDouble must accept the text and return exactly this value.
    void checkParses(std::string_view text, double expected)
    {
        const std::optional<double> value = babylon::core::parseDouble(text);
        REQUIRE_MESSAGE(value.has_value(), "text '" << text << "' must parse");
        CHECK(*value == expected);
    }

    // parseDouble must refuse the text.
    void checkRefuses(std::string_view text)
    {
        CHECK_MESSAGE(!babylon::core::parseDouble(text).has_value(),
            "text '" << text << "' must be refused");
    }

}

TEST_CASE("parseDouble accepts the decimal forms of its grammar")
{
    checkParses("0", 0.0);
    checkParses("1.5", 1.5);
    checkParses(".5", 0.5);
    checkParses("5.", 5.0);
    checkParses("1e300", 1e300);
    checkParses("-2.5E-3", -2.5e-3);
    // The smallest denormal is underflow, not an error, and correct
    // rounding lands this text on it exactly.
    checkParses("4.9e-324", std::numeric_limits<double>::denorm_min());
    // Correct rounding: this equals the literal the compiler produced,
    // not just the closest double the text could name.
    checkParses("0.1", 0.1);
}

TEST_CASE("parseDouble keeps the sign of negative zero")
{
    const std::optional<double> value = babylon::core::parseDouble("-0");
    REQUIRE(value.has_value());
    CHECK(*value == 0.0);
    CHECK(std::signbit(*value));
}

TEST_CASE("parseDouble refuses everything outside the grammar")
{
    checkRefuses("");
    checkRefuses("-");
    checkRefuses(".");
    checkRefuses("+1");      // no leading '+'
    checkRefuses(" 1");      // no whitespace
    checkRefuses("1 ");
    checkRefuses("1e");      // an exponent needs at least one digit
    checkRefuses("1e+");
    checkRefuses("0x10");    // no hex
    checkRefuses("inf");     // no named constants
    checkRefuses("nan");
    checkRefuses("1.2.3");   // no trailing characters
    checkRefuses("1,5");
}

TEST_CASE("parseDouble refuses results that overflow to infinity")
{
    checkRefuses("1e400");
}

TEST_CASE("parseDouble ignores the C locale's decimal point")
{
    // The substitution only shows with a comma locale installed; on a
    // machine without de_DE.UTF-8 this part is skipped.
    const char* installed = std::setlocale(LC_NUMERIC, "de_DE.UTF-8");
    if ( installed == nullptr )
    {
        MESSAGE("de_DE.UTF-8 is not installed here; the comma decimal point is not exercised");
        return;
    }
    const std::optional<double> value = babylon::core::parseDouble("1.5");
    std::setlocale(LC_NUMERIC, "C");
    REQUIRE(value.has_value());
    CHECK(*value == 1.5);
}

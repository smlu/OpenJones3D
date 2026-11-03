// SPDX-License-Identifier: MPL-2.0
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include <string_view>

namespace babylon::core
{

    // Everything a failed assertion can report. All views only have to stay
    // valid for the duration of the handler call.
    struct AssertInfo
    {
        std::string_view expression;
        std::string_view message;   // empty when no message was given
        std::string_view file;
        int line;
    };

    // Handler signature for assertion failures.
    using AssertHandler = void (*)(const AssertInfo& info);

    // Installs the handler called when an assertion fails and returns the
    // previous one. nullptr restores the default handler. Not thread-safe;
    // call during startup or in tests.
    AssertHandler setAssertHandler(AssertHandler handler);

    // Called by BABYLON_ASSERT. Calls the installed handler. If the handler
    // returns, prints "<file>:<line>: assertion failed: <expression>", plus
    // ": <message>" when a message was given, to stderr and calls std::abort().
    // The default handler does exactly that. A custom handler either doesn't
    // return (tests throw a test-local exception) or the program aborts.
    [[noreturn]] void assertFailed(const AssertInfo& info);

}

// Assertions are active in every build type; there is no NDEBUG switch. The
// expression is evaluated exactly once. msg must be a string literal.
#define BABYLON_ASSERT(expr) \
    do { if ( !(expr) ) { ::babylon::core::assertFailed({ #expr, "", __FILE__, __LINE__ }); } } while ( false )

#define BABYLON_ASSERT_MSG(expr, msg) \
    do { if ( !(expr) ) { ::babylon::core::assertFailed({ #expr, msg, __FILE__, __LINE__ }); } } while ( false )

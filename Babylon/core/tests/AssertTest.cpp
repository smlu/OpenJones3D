// SPDX-License-Identifier: MPL-2.0
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <babylon/core/Assert.h>

#include <doctest.h>

#include <string>

namespace
{

    // Test-local exception thrown by the installed handler to unwind a failed
    // assertion back into the test instead of aborting the process.
    struct AssertFailure
    {
    };

    // Test-local storage the handler copies the AssertInfo into; the views only
    // have to stay valid for the duration of the handler call.
    struct Captured
    {
        std::string expression;
        std::string message;
        std::string file;
        int line = 0;
        int calls = 0;
    };

    Captured captured;
    int evalCount = 0;

    void testAssertHandler(const babylon::core::AssertInfo& info)
    {
        captured.expression = std::string(info.expression);
        captured.message = std::string(info.message);
        captured.file = std::string(info.file);
        captured.line = info.line;
        ++captured.calls;
        throw AssertFailure();
    }

    // Installs the test handler for one test case and restores the previous
    // handler on scope exit, so each test passes alone and in any order.
    class ScopedAssertHandler
    {
    public:
        ScopedAssertHandler() :
            m_previous(babylon::core::setAssertHandler(testAssertHandler))
        {
            captured = Captured();
        }

        ~ScopedAssertHandler()
        {
            babylon::core::setAssertHandler(m_previous);
        }

        ScopedAssertHandler(const ScopedAssertHandler&) = delete;
        ScopedAssertHandler& operator=(const ScopedAssertHandler&) = delete;

    private:
        babylon::core::AssertHandler m_previous;
    };

    bool bumpAndTrue()
    {
        ++evalCount;
        return true;
    }

    bool bumpAndFalse()
    {
        ++evalCount;
        return false;
    }

}

TEST_CASE("A passing BABYLON_ASSERT does not call the handler")
{
    ScopedAssertHandler guard;
    int value = 1;
    BABYLON_ASSERT(value == 1);
    CHECK(captured.calls == 0);
}

TEST_CASE("A failing BABYLON_ASSERT calls the handler once with expression, file and line")
{
    ScopedAssertHandler guard;
    int value = 1;
    int expectedLine = 0;
    try
    {
        expectedLine = __LINE__ + 1;
        BABYLON_ASSERT(value == 2);
        CHECK_FALSE("BABYLON_ASSERT must not return after a failed assertion");
    }
    catch (const AssertFailure&)
    {
    }
    CHECK(captured.calls == 1);
    CHECK(captured.expression == "value == 2");
    CHECK(captured.message.empty());
    CHECK(captured.file.ends_with("AssertTest.cpp"));
    CHECK(captured.line == expectedLine);
}

TEST_CASE("BABYLON_ASSERT_MSG passes the message to the handler")
{
    ScopedAssertHandler guard;
    int value = 1;
    try
    {
        BABYLON_ASSERT_MSG(value == 2, "value must be two");
        CHECK_FALSE("BABYLON_ASSERT_MSG must not return after a failed assertion");
    }
    catch (const AssertFailure&)
    {
    }
    CHECK(captured.calls == 1);
    CHECK(captured.expression == "value == 2");
    CHECK(captured.message == "value must be two");
}

TEST_CASE("The assertion expression is evaluated exactly once")
{
    ScopedAssertHandler guard;

    evalCount = 0;
    BABYLON_ASSERT(bumpAndTrue());
    CHECK(evalCount == 1);

    evalCount = 0;
    try
    {
        BABYLON_ASSERT(bumpAndFalse());
        CHECK_FALSE("BABYLON_ASSERT must not return after a failed assertion");
    }
    catch (const AssertFailure&)
    {
    }
    CHECK(evalCount == 1);
}

TEST_CASE("setAssertHandler returns the previous handler and nullptr restores the default")
{
    // The handler is the default (nullptr) whenever a test starts: every test
    // case restores it, so this holds alone and in any order.
    const babylon::core::AssertHandler previous = babylon::core::setAssertHandler(testAssertHandler);
    CHECK(previous == nullptr);
    const babylon::core::AssertHandler installed = babylon::core::setAssertHandler(nullptr);
    CHECK(installed == testAssertHandler);
}

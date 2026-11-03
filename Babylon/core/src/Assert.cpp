// SPDX-License-Identifier: MPL-2.0
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <babylon/core/Assert.h>

#include <cstdio>
#include <cstdlib>

namespace babylon::core
{
namespace
{

    // The installed handler; nullptr selects the default reporting below.
    AssertHandler installedHandler = nullptr;

    // The default handler's job: report to stderr and abort. A custom handler
    // that returns ends up here too, which is what assertFailed promises.
    [[noreturn]] void reportAndAbort(const AssertInfo& info)
    {
        // string_view data is not guaranteed null-terminated, so every text is
        // written through %.*s with its length.
        std::fprintf(stderr, "%.*s:%d: assertion failed: %.*s",
            static_cast<int>(info.file.size()), info.file.data(), info.line,
            static_cast<int>(info.expression.size()), info.expression.data());
        if ( !info.message.empty() )
        {
            std::fprintf(stderr, ": %.*s",
                static_cast<int>(info.message.size()), info.message.data());
        }
        std::fputc('\n', stderr);
        std::abort();
    }

}

    AssertHandler setAssertHandler(AssertHandler handler)
    {
        const AssertHandler previous = installedHandler;
        installedHandler = handler;
        return previous;
    }

    [[noreturn]] void assertFailed(const AssertInfo& info)
    {
        if ( installedHandler != nullptr )
        {
            installedHandler(info);
        }
        reportAndAbort(info);
    }

}

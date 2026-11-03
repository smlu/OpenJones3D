// SPDX-License-Identifier: MPL-2.0
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include <babylon/log/Sink.h>

#include <format>
#include <string_view>
#include <utility>

namespace babylon::log
{

    // Attaches a sink. The caller keeps ownership and must detach it before
    // destroying it. Attaching a sink that is already attached does nothing.
    void attachSink(Sink& sink);

    // Detaches a sink. Detaching a sink that isn't attached does nothing.
    void detachSink(Sink& sink);

    // Records below this level are dropped before formatting. Default:
    // Level::Info.
    void setMinimumLevel(Level level);
    Level minimumLevel();

    // True when a record at this level would reach the sinks: some sink is
    // attached and level >= minimumLevel().
    bool isEnabled(Level level);

    // Sends the message to every attached sink, in attach order. Records
    // below the minimum level are dropped here too, so the Sink contract
    // holds for direct calls as well. May be called from any thread; the log
    // serializes calls. Calling the log from inside a sink's write() would
    // deadlock, so sinks must not do it.
    void write(Level level, std::string_view message);

namespace detail
{

    // Internal to the five formatting front ends below, not part of the
    // module's API: it decides on isEnabled before formatting, so a dropped
    // record never touches its arguments.
    template <class... Args>
    void formatAndWrite(Level level, std::format_string<Args...> format, Args&&... args)
    {
        if ( !isEnabled(level) )
        {
            return;
        }
        write(level, std::format(format, std::forward<Args>(args)...));
    }

}

    // Formatting front ends in std::format syntax. They format only when
    // isEnabled(level) is true; otherwise the arguments are not evaluated
    // into a string.
    template <class... Args>
    void trace(std::format_string<Args...> format, Args&&... args)
    {
        detail::formatAndWrite(Level::Trace, format, std::forward<Args>(args)...);
    }

    template <class... Args>
    void debug(std::format_string<Args...> format, Args&&... args)
    {
        detail::formatAndWrite(Level::Debug, format, std::forward<Args>(args)...);
    }

    template <class... Args>
    void info(std::format_string<Args...> format, Args&&... args)
    {
        detail::formatAndWrite(Level::Info, format, std::forward<Args>(args)...);
    }

    template <class... Args>
    void warning(std::format_string<Args...> format, Args&&... args)
    {
        detail::formatAndWrite(Level::Warning, format, std::forward<Args>(args)...);
    }

    template <class... Args>
    void error(std::format_string<Args...> format, Args&&... args)
    {
        detail::formatAndWrite(Level::Error, format, std::forward<Args>(args)...);
    }

}

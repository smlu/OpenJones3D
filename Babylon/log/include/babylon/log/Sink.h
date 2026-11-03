// SPDX-License-Identifier: MPL-2.0
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include <string_view>

namespace babylon::log
{

    // Severity of a log record, ordered from the lowest to the highest.
    enum class Level
    {
        Trace,
        Debug,
        Info,
        Warning,
        Error
    };

    // Lowercase name: "trace", "debug", "info", "warning", "error".
    std::string_view levelName(Level level);

    // One record on its way to the sinks.
    struct Record
    {
        Level level;
        std::string_view message;   // valid only during Sink::write
    };

    // Receives every record at or above the minimum level. write() may be
    // called from any thread, but never concurrently: the log serializes
    // calls. A sink must not call the log from inside write(); the log holds
    // its mutex across the call, so a nested call would deadlock.
    class Sink
    {
    public:
        virtual ~Sink() = default;
        virtual void write(const Record& record) = 0;
    };

}

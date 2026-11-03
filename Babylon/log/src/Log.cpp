// SPDX-License-Identifier: MPL-2.0
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <babylon/log/Log.h>

#include <algorithm>
#include <mutex>
#include <vector>

namespace babylon::log
{
namespace
{

    // Guards the two globals below and is held while the sinks run. A sink
    // that called the log again from write() would deadlock on it; Sink.h
    // documents that a sink must not do so.
    std::mutex logMutex;

    // The attached sinks in attach order, never with duplicates. The log
    // points at them and never owns them: the caller must detach a sink
    // before destroying it.
    std::vector<Sink*> attachedSinks;

    // Records below this level are dropped.
    Level minLevel = Level::Info;

}

    std::string_view levelName(Level level)
    {
        switch ( level )
        {
            case Level::Trace: return "trace";
            case Level::Debug: return "debug";
            case Level::Info: return "info";
            case Level::Warning: return "warning";
            case Level::Error: return "error";
        }
        // Only an out-of-range cast of Level reaches this; the five declared
        // levels all return above.
        return "unknown";
    }

    void attachSink(Sink& sink)
    {
        const std::lock_guard<std::mutex> lock(logMutex);
        if ( std::find(attachedSinks.begin(), attachedSinks.end(), &sink) == attachedSinks.end() )
        {
            attachedSinks.push_back(&sink);
        }
    }

    void detachSink(Sink& sink)
    {
        const std::lock_guard<std::mutex> lock(logMutex);
        attachedSinks.erase(
            std::remove(attachedSinks.begin(), attachedSinks.end(), &sink),
            attachedSinks.end());
    }

    void setMinimumLevel(Level level)
    {
        const std::lock_guard<std::mutex> lock(logMutex);
        minLevel = level;
    }

    Level minimumLevel()
    {
        const std::lock_guard<std::mutex> lock(logMutex);
        return minLevel;
    }

    bool isEnabled(Level level)
    {
        const std::lock_guard<std::mutex> lock(logMutex);
        return !attachedSinks.empty() && level >= minLevel;
    }

    void write(Level level, std::string_view message)
    {
        const std::lock_guard<std::mutex> lock(logMutex);
        if ( level < minLevel )
        {
            return;
        }
        const Record record{ level, message };
        for ( Sink* sink : attachedSinks )
        {
            sink->write(record);
        }
    }

}

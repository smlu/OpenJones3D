// SPDX-License-Identifier: MPL-2.0
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include <babylon/log/Sink.h>

#include <string>
#include <vector>

namespace babylon::log
{

    // One record captured by CaptureSink. The message is a copy, so it stays
    // valid as long as the CapturedRecord itself.
    struct CapturedRecord
    {
        Level level;
        std::string message;
    };

    // Sink for tests in any module: copies every record it receives. Attaches
    // itself on construction and detaches on destruction, so a test can't
    // forget to detach. The copy happens under the log's mutex; a capture
    // sink must not call the log from inside write().
    class CaptureSink final : public Sink
    {
    public:
        CaptureSink();
        ~CaptureSink() override;
        CaptureSink(const CaptureSink&) = delete;
        CaptureSink& operator=(const CaptureSink&) = delete;

        void write(const Record& record) override;

        // The captured records in arrival order. The reference stays valid
        // while the sink lives, but later records move the vector's contents,
        // so re-fetch elements after every write.
        const std::vector<CapturedRecord>& records() const;

        // Forgets all captured records.
        void clear();

    private:
        std::vector<CapturedRecord> m_records;
    };

}

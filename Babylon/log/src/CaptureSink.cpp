// SPDX-License-Identifier: MPL-2.0
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <babylon/log/CaptureSink.h>

#include <babylon/log/Log.h>

#include <string>

namespace babylon::log
{

    CaptureSink::CaptureSink()
    {
        attachSink(*this);
    }

    CaptureSink::~CaptureSink()
    {
        detachSink(*this);
    }

    void CaptureSink::write(const Record& record)
    {
        m_records.push_back({ record.level, std::string(record.message) });
    }

    const std::vector<CapturedRecord>& CaptureSink::records() const
    {
        return m_records;
    }

    void CaptureSink::clear()
    {
        m_records.clear();
    }

}

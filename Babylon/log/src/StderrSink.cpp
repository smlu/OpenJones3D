// SPDX-License-Identifier: MPL-2.0
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <babylon/log/StderrSink.h>

#include <cstdio>
#include <string>

namespace babylon::log
{

    void StderrSink::write(const Record& record)
    {
        // One fwrite of a complete line: record.message is a string_view, so
        // the line is assembled in a buffer first instead of written piece
        // by piece, which another stderr writer could interleave with.
        std::string line;
        line.reserve(record.message.size() + levelName(record.level).size() + 4);
        line += '[';
        line += levelName(record.level);
        line += "] ";
        line += record.message;
        line += '\n';
        std::fwrite(line.data(), 1, line.size(), stderr);
    }

}

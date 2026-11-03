// SPDX-License-Identifier: MPL-2.0
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include <babylon/log/Sink.h>

namespace babylon::log
{

    // Sink that writes "[<level>] <message>\n" to stderr with one fwrite per
    // record, so two log records never interleave mid-line. No state: an
    // instance can be attached at any time and detached again. Interleaving
    // with other writers to stderr is not prevented.
    class StderrSink final : public Sink
    {
    public:
        void write(const Record& record) override;
    };

}

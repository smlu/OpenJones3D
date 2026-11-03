// SPDX-License-Identifier: MPL-2.0
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include <babylon/platform/Platform.h>

#include <deque>
#include <memory>
#include <optional>

namespace babylon::platform
{

    // Platform that touches no operating system at all, for tests and
    // headless runs. Windows are records in memory: ids start at 1 and count
    // up, resize() always succeeds once the size passed the shared check, and
    // the title and size are exactly what was asked for.
    //
    // Nothing ever arrives in the event queue by itself: only pushEvent(),
    // from a test or from the headless runner, puts events in it. They come
    // out again in that order.
    class NullPlatform final : public Platform
    {
    public:
        std::unique_ptr<Window> createWindow(const WindowDesc& desc) override;
        std::optional<Event> pollEvent() override;

        // Appends an event to the queue (FIFO). pushEvent() followed by
        // pollEvent() hands back a copy of the event.
        void pushEvent(const Event& event);

    private:
        // The id the next created window gets. 0 is never handed out, as the
        // interface requires.
        WindowId m_nextId = 1;
        std::deque<Event> m_events;
    };

}

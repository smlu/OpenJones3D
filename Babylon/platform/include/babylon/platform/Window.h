// SPDX-License-Identifier: MPL-2.0
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include <babylon/platform/Event.h>

#include <string>
#include <string_view>

namespace babylon::platform
{

    // The largest width or height a window may have, in pixels. Both the
    // creation of a window and Window::resize() refuse anything above it,
    // which is also beyond what any backend's window system can give.
    inline constexpr int kMaxWindowDimension = 16384;

    // How a window is created. The title is shown as given; an empty title
    // is allowed and means the window system's own default.
    struct WindowDesc
    {
        std::string title;
        Size size{ 640, 480 };
        bool resizable = false;
    };

    // One OS window, created by Platform::createWindow and never directly.
    // Destroy every window before the Platform that made it. Failures log
    // one babylon::log::warning line and change nothing; nothing throws.
    // Not thread-safe: call from the thread that created the window.
    class Window
    {
    public:
        virtual ~Window() = default;

        // The shared size check of every backend: 1 <= width and height <=
        // kMaxWindowDimension. On failure this logs one warning and returns
        // false with the size unchanged. Otherwise it asks the backend and
        // returns its answer; a backend that refuses logs its own warning.
        [[nodiscard]] bool resize(Size size);

        // Sets the title shown by the window system. This cannot fail in a
        // way the caller can act on: a backend that rejects the text logs
        // one warning and leaves the title as it was.
        virtual void setTitle(std::string_view title) = 0;

        // The title as the backend reports it now, which is what
        // setTitle() set last unless the backend refused it.
        virtual std::string title() const = 0;

        // The current client size in pixels, excluding decorations.
        virtual Size size() const = 0;

        // The id this window was created with, stable for its lifetime and
        // never 0. Events about this window carry it.
        virtual WindowId id() const = 0;

    private:
        // Called only with a size that passed the check in resize(). Returns
        // false and logs one warning if the backend refused it.
        virtual bool doResize(Size size) = 0;
    };

    // True when both dimensions are within 1..kMaxWindowDimension. Shared by
    // Window::resize() and by every Platform::createWindow(), so the same
    // size is accepted at creation and in a resize.
    bool isValidWindowSize(Size size);

}

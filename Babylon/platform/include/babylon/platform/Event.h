// SPDX-License-Identifier: MPL-2.0
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include <cstdint>
#include <variant>

namespace babylon::platform
{

    // Identifies one window created by a Platform. 0 is never a valid
    // window, so it means "no window" in an event.
    using WindowId = std::uint32_t;

    // A size in pixels. A window only ever has positive dimensions; see
    // isValidWindowSize().
    struct Size
    {
        int width = 0;
        int height = 0;

        bool operator==(const Size&) const = default;
    };

    // The user or the operating system asked the application to quit. The
    // game decides: it may keep running for a while, but nothing forces the
    // quit.
    struct QuitRequested {};

    // The window manager asked one window to close; the window is still
    // there until its owner destroys it.
    struct WindowCloseRequested
    {
        WindowId window;
    };

    // One window gained or lost the keyboard focus.
    struct WindowFocusChanged
    {
        WindowId window;
        bool focused;
    };

    // One window now has a different client size in pixels.
    struct WindowResized
    {
        WindowId window;
        Size size;
    };

    // One OS event. Every backend delivers these in the order the OS
    // produced them. New kinds are added as further alternatives, which is
    // why they are plain records in one variant instead of a class per kind:
    // keys, mouse and gamepad events join this queue in the next slice.
    using Event = std::variant<QuitRequested, WindowCloseRequested,
        WindowFocusChanged, WindowResized>;

}

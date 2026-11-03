// SPDX-License-Identifier: MPL-2.0
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include <babylon/platform/Keys.h>

#include <cstdint>
#include <variant>

namespace babylon::platform
{

    // Identifies one window created by a Platform. 0 is never a valid
    // window, so it means "no window" in an event.
    using WindowId = std::uint32_t;

    // Identifies one gamepad a backend reported. 0 is never a valid
    // gamepad, so it means "no gamepad" in an event.
    using GamepadId = std::uint32_t;

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

    // A physical key went down or came up in one window. Keys outside the
    // Key table arrive as Key::Unknown and are still delivered, so a game
    // can log them. repeat is true only while down, for the operating
    // system's key repeat.
    struct KeyChanged
    {
        WindowId window;
        Key key;
        bool down;
        bool repeat;
    };

    // The pointer moved in one window. x and y are window pixels, dx and dy
    // the relative motion since the previous event; in relative mouse mode
    // only dx and dy carry information.
    struct MouseMoved
    {
        WindowId window;
        float x;
        float y;
        float dx;
        float dy;
    };

    // One of the five mouse buttons went down or came up in one window.
    // Buttons beyond X2 are dropped by the backend and never arrive.
    struct MouseButtonChanged
    {
        WindowId window;
        MouseButton button;
        bool down;
    };

    // The wheel scrolled in one window. positive dy = away from the user,
    // positive dx = to the right, both already corrected for a flipped
    // (natural) scroll direction.
    struct MouseWheelMoved
    {
        WindowId window;
        float dx;
        float dy;
    };

    // A gamepad was plugged in; its id is valid from this event on, and
    // button and axis events carry it.
    struct GamepadConnected
    {
        GamepadId gamepad;
    };

    // A gamepad was unplugged; its id is not reused.
    struct GamepadDisconnected
    {
        GamepadId gamepad;
    };

    // A gamepad button went down or came up.
    struct GamepadButtonChanged
    {
        GamepadId gamepad;
        GamepadButton button;
        bool down;
    };

    // A gamepad axis moved. Sticks report -1..1, triggers 0..1.
    struct GamepadAxisMoved
    {
        GamepadId gamepad;
        GamepadAxis axis;
        float value;
    };

    // One OS event. Every backend delivers these in the order the OS
    // produced them. New kinds are added as further alternatives, which is
    // why they are plain records in one variant instead of a class per kind:
    // window, key, mouse and gamepad events all share this one queue.
    using Event = std::variant<QuitRequested, WindowCloseRequested,
        WindowFocusChanged, WindowResized,
        KeyChanged, MouseMoved, MouseButtonChanged, MouseWheelMoved,
        GamepadConnected, GamepadDisconnected, GamepadButtonChanged,
        GamepadAxisMoved>;

}

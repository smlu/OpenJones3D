// SPDX-License-Identifier: MPL-2.0
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include <cstdint>
#include <optional>
#include <string_view>

namespace babylon::platform
{

    // Physical keys, named by the enumerator and independent of the keyboard
    // layout, like USB HID usages. KeyChanged events carry one. A key the
    // table does not name arrives as Key::Unknown and is still delivered.
    enum class Key : std::uint16_t
    {
        Unknown = 0,
        A, B, C, D, E, F, G, H, I, J, K, L, M,
        N, O, P, Q, R, S, T, U, V, W, X, Y, Z,
        Num0, Num1, Num2, Num3, Num4,         // the row above the letters
        Num5, Num6, Num7, Num8, Num9,
        F1, F2, F3, F4, F5, F6,
        F7, F8, F9, F10, F11, F12,
        Escape, Enter, Tab, Backspace, Space,
        Minus, Equals, LeftBracket, RightBracket, Backslash, Semicolon,
        Apostrophe, Grave, Comma, Period, Slash,
        CapsLock, ScrollLock, NumLock, PrintScreen, Pause,
        Insert, Delete, Home, End, PageUp, PageDown,
        Left, Right, Up, Down,
        Keypad0, Keypad1, Keypad2, Keypad3, Keypad4,
        Keypad5, Keypad6, Keypad7, Keypad8, Keypad9,
        KeypadDivide, KeypadMultiply, KeypadMinus, KeypadPlus, KeypadEnter,
        KeypadPeriod,
        LeftShift, RightShift, LeftCtrl, RightCtrl, LeftAlt, RightAlt,
        LeftMeta, RightMeta,
        Count                                 // number of keys, not a key
    };

    // Mouse buttons as Babylon names them; the backend maps its own button
    // numbers onto these. Buttons beyond X2 are dropped, not delivered.
    enum class MouseButton : std::uint8_t { Left, Middle, Right, X1, X2 };

    // Gamepad buttons by position on the face, not by the label one
    // manufacturer prints there.
    enum class GamepadButton : std::uint8_t
    {
        South, East, West, North,
        Back, Guide, Start,
        LeftStick, RightStick,
        LeftShoulder, RightShoulder,
        DpadUp, DpadDown, DpadLeft, DpadRight
    };

    // Gamepad axes. The sticks report -1..1, the triggers 0..1.
    enum class GamepadAxis : std::uint8_t { LeftX, LeftY, RightX, RightY, LeftTrigger, RightTrigger };

    // The stable name of a key, the enumerator spelled exactly ("A", "Num0",
    // "F1", "LeftShift", "KeypadEnter", ...). keyName(Key::Unknown) and any
    // value outside the table give "Unknown". The names are what scripts and
    // configuration use, so they never change once given.
    std::string_view keyName(Key key);

    // The exact, case-sensitive inverse of keyName(). nullopt for a name the
    // table does not hold; "Unknown" and "Count" are not key names.
    std::optional<Key> keyFromName(std::string_view name);

    // The same for the other input enums. An out-of-range value gives the
    // empty name, and a name outside the table gives nullopt.
    std::string_view mouseButtonName(MouseButton button);
    std::optional<MouseButton> mouseButtonFromName(std::string_view name);
    std::string_view gamepadButtonName(GamepadButton button);
    std::optional<GamepadButton> gamepadButtonFromName(std::string_view name);
    std::string_view gamepadAxisName(GamepadAxis axis);
    std::optional<GamepadAxis> gamepadAxisFromName(std::string_view name);

}

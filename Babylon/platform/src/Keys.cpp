// SPDX-License-Identifier: MPL-2.0
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <babylon/platform/Keys.h>

#include <array>
#include <cstddef>
#include <optional>
#include <string_view>

namespace babylon::platform
{
namespace
{

    // One name per Key enumerator, in value order, so the name of a key is
    // kKeyNames[static_cast<std::size_t>(key)]. Entry 0 is "Unknown", which
    // keyFromName() never accepts; adding a key without its name, or a name
    // without its key, fails the static_assert below.
    constexpr std::array<std::string_view, static_cast<std::size_t>(Key::Count)> kKeyNames = {
        "Unknown",
        "A", "B", "C", "D", "E", "F", "G", "H", "I", "J", "K", "L",
        "M", "N", "O", "P", "Q", "R", "S", "T", "U", "V", "W", "X", "Y", "Z",
        "Num0", "Num1", "Num2", "Num3", "Num4", "Num5", "Num6", "Num7", "Num8", "Num9",
        "F1", "F2", "F3", "F4", "F5", "F6", "F7", "F8", "F9", "F10", "F11", "F12",
        "Escape", "Enter", "Tab", "Backspace", "Space",
        "Minus", "Equals", "LeftBracket", "RightBracket", "Backslash", "Semicolon",
        "Apostrophe", "Grave", "Comma", "Period", "Slash",
        "CapsLock", "ScrollLock", "NumLock", "PrintScreen", "Pause",
        "Insert", "Delete", "Home", "End", "PageUp", "PageDown",
        "Left", "Right", "Up", "Down",
        "Keypad0", "Keypad1", "Keypad2", "Keypad3", "Keypad4",
        "Keypad5", "Keypad6", "Keypad7", "Keypad8", "Keypad9",
        "KeypadDivide", "KeypadMultiply", "KeypadMinus", "KeypadPlus", "KeypadEnter",
        "KeypadPeriod",
        "LeftShift", "RightShift", "LeftCtrl", "RightCtrl", "LeftAlt", "RightAlt",
        "LeftMeta", "RightMeta",
    };
    static_assert( kKeyNames.size() == static_cast<std::size_t>(Key::Count) );

    // One name per enumerator, in value order, X2 last.
    constexpr std::array<std::string_view, static_cast<std::size_t>(MouseButton::X2) + 1>
        kMouseButtonNames = { "Left", "Middle", "Right", "X1", "X2" };
    static_assert( kMouseButtonNames.size() == static_cast<std::size_t>(MouseButton::X2) + 1 );

    // One name per enumerator, in value order, DpadRight last.
    constexpr std::array<std::string_view, static_cast<std::size_t>(GamepadButton::DpadRight) + 1>
        kGamepadButtonNames = {
            "South", "East", "West", "North",
            "Back", "Guide", "Start",
            "LeftStick", "RightStick",
            "LeftShoulder", "RightShoulder",
            "DpadUp", "DpadDown", "DpadLeft", "DpadRight",
        };
    static_assert( kGamepadButtonNames.size() == static_cast<std::size_t>(GamepadButton::DpadRight) + 1 );

    // One name per enumerator, in value order, RightTrigger last.
    constexpr std::array<std::string_view, static_cast<std::size_t>(GamepadAxis::RightTrigger) + 1>
        kGamepadAxisNames = { "LeftX", "LeftY", "RightX", "RightY", "LeftTrigger", "RightTrigger" };
    static_assert( kGamepadAxisNames.size() == static_cast<std::size_t>(GamepadAxis::RightTrigger) + 1 );

    // Searches a name table for the exact name, from entry `first` up: used
    // with first == 1 for the keys, whose entry 0 "Unknown" is no name, and
    // first == 0 for the enums without an Unknown.
    template <typename Table>
    std::optional<std::size_t> indexOf(const Table& table, std::string_view name, std::size_t first)
    {
        for ( std::size_t i = first; i < table.size(); ++i )
        {
            if ( table[i] == name )
            {
                return i;
            }
        }
        return std::nullopt;
    }

}

    std::string_view keyName(Key key)
    {
        const std::size_t index = static_cast<std::size_t>(key);
        if ( index >= kKeyNames.size() )
        {
            return "Unknown";   // values outside the table have no name of their own
        }
        return kKeyNames[index];
    }

    std::optional<Key> keyFromName(std::string_view name)
    {
        const std::optional<std::size_t> index = indexOf(kKeyNames, name, 1);
        if ( !index.has_value() )
        {
            return std::nullopt;
        }
        return static_cast<Key>(*index);
    }

    std::string_view mouseButtonName(MouseButton button)
    {
        const std::size_t index = static_cast<std::size_t>(button);
        if ( index >= kMouseButtonNames.size() )
        {
            return {};
        }
        return kMouseButtonNames[index];
    }

    std::optional<MouseButton> mouseButtonFromName(std::string_view name)
    {
        const std::optional<std::size_t> index = indexOf(kMouseButtonNames, name, 0);
        if ( !index.has_value() )
        {
            return std::nullopt;
        }
        return static_cast<MouseButton>(*index);
    }

    std::string_view gamepadButtonName(GamepadButton button)
    {
        const std::size_t index = static_cast<std::size_t>(button);
        if ( index >= kGamepadButtonNames.size() )
        {
            return {};
        }
        return kGamepadButtonNames[index];
    }

    std::optional<GamepadButton> gamepadButtonFromName(std::string_view name)
    {
        const std::optional<std::size_t> index = indexOf(kGamepadButtonNames, name, 0);
        if ( !index.has_value() )
        {
            return std::nullopt;
        }
        return static_cast<GamepadButton>(*index);
    }

    std::string_view gamepadAxisName(GamepadAxis axis)
    {
        const std::size_t index = static_cast<std::size_t>(axis);
        if ( index >= kGamepadAxisNames.size() )
        {
            return {};
        }
        return kGamepadAxisNames[index];
    }

    std::optional<GamepadAxis> gamepadAxisFromName(std::string_view name)
    {
        const std::optional<std::size_t> index = indexOf(kGamepadAxisNames, name, 0);
        if ( !index.has_value() )
        {
            return std::nullopt;
        }
        return static_cast<GamepadAxis>(*index);
    }

}

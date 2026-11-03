// SPDX-License-Identifier: MPL-2.0
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <babylon/input/InputState.h>

#include <algorithm>
#include <cstddef>
#include <variant>

namespace babylon::input
{
namespace
{

    // std::visit helper: combines lambdas into one overload set, one per
    // event kind of platform::Event.
    template <typename... Functions>
    struct Overloaded : Functions...
    {
        using Functions::operator()...;
    };

    template <typename... Functions>
    Overloaded(Functions...) -> Overloaded<Functions...>;

}

    void InputState::beginFrame()
    {
        for ( ButtonState& state : m_keys )
        {
            state.pressed = false;
            state.released = false;
        }
        for ( ButtonState& state : m_mouseButtons )
        {
            state.pressed = false;
            state.released = false;
        }
        m_mouseDeltaX = 0.0f;
        m_mouseDeltaY = 0.0f;
        m_wheelX = 0.0f;
        m_wheelY = 0.0f;
    }

    void InputState::apply(const platform::Event& event)
    {
        std::visit(Overloaded {
            [this](const platform::KeyChanged& changed)
            {
                // Unknown carries no state, and repeats never change it.
                const std::size_t index = static_cast<std::size_t>(changed.key);
                if ( changed.repeat || changed.key == platform::Key::Unknown
                    || index >= m_keys.size() )
                {
                    return;
                }
                applyPress(m_keys[index], changed.down);
            },
            [this](const platform::MouseButtonChanged& changed)
            {
                const std::size_t index = static_cast<std::size_t>(changed.button);
                if ( index >= m_mouseButtons.size() )
                {
                    return;
                }
                applyPress(m_mouseButtons[index], changed.down);
            },
            [this](const platform::MouseMoved& moved)
            {
                m_mouseX = moved.x;
                m_mouseY = moved.y;
                m_mouseDeltaX += moved.dx;
                m_mouseDeltaY += moved.dy;
            },
            [this](const platform::MouseWheelMoved& moved)
            {
                m_wheelX += moved.dx;
                m_wheelY += moved.dy;
            },
            [this](const platform::GamepadConnected& connected)
            {
                // Id 0 means "no gamepad"; a repeat connection is ignored.
                if ( connected.gamepad == 0 || indexOfGamepad(connected.gamepad) != kNoGamepad )
                {
                    return;
                }
                m_gamepads.push_back(connected.gamepad);
                m_gamepadStates.push_back(GamepadState{});
            },
            [this](const platform::GamepadDisconnected& disconnected)
            {
                const std::size_t index = indexOfGamepad(disconnected.gamepad);
                if ( index == kNoGamepad )
                {
                    return;
                }
                m_gamepads.erase(m_gamepads.begin() + static_cast<std::ptrdiff_t>(index));
                m_gamepadStates.erase(m_gamepadStates.begin() + static_cast<std::ptrdiff_t>(index));
            },
            [this](const platform::GamepadButtonChanged& changed)
            {
                const std::size_t pad = indexOfGamepad(changed.gamepad);
                const std::size_t button = static_cast<std::size_t>(changed.button);
                if ( pad == kNoGamepad || button >= GamepadState::kButtonCount )
                {
                    return;
                }
                m_gamepadStates[pad].buttons[button] = changed.down;
            },
            [this](const platform::GamepadAxisMoved& moved)
            {
                const std::size_t pad = indexOfGamepad(moved.gamepad);
                const std::size_t axis = static_cast<std::size_t>(moved.axis);
                if ( pad == kNoGamepad || axis >= GamepadState::kAxisCount )
                {
                    return;
                }
                m_gamepadStates[pad].axes[axis] = moved.value;
            },
            // Losing the focus releases what is held, so nothing sticks
            // while another window has the keyboard.
            [this](const platform::WindowFocusChanged& changed)
            {
                if ( !changed.focused )
                {
                    releaseAll();
                }
            },
            // The remaining events carry no input.
            [](const platform::QuitRequested&) {},
            [](const platform::WindowCloseRequested&) {},
            [](const platform::WindowResized&) {},
        }, event);
    }

    bool InputState::keyDown(platform::Key key) const
    {
        const std::size_t index = static_cast<std::size_t>(key);
        return index < m_keys.size() && m_keys[index].down;
    }

    bool InputState::keyPressed(platform::Key key) const
    {
        const std::size_t index = static_cast<std::size_t>(key);
        return index < m_keys.size() && m_keys[index].pressed;
    }

    bool InputState::keyReleased(platform::Key key) const
    {
        const std::size_t index = static_cast<std::size_t>(key);
        return index < m_keys.size() && m_keys[index].released;
    }

    bool InputState::mouseButtonDown(platform::MouseButton button) const
    {
        const std::size_t index = static_cast<std::size_t>(button);
        return index < m_mouseButtons.size() && m_mouseButtons[index].down;
    }

    bool InputState::mouseButtonPressed(platform::MouseButton button) const
    {
        const std::size_t index = static_cast<std::size_t>(button);
        return index < m_mouseButtons.size() && m_mouseButtons[index].pressed;
    }

    bool InputState::mouseButtonReleased(platform::MouseButton button) const
    {
        const std::size_t index = static_cast<std::size_t>(button);
        return index < m_mouseButtons.size() && m_mouseButtons[index].released;
    }

    float InputState::mouseX() const
    {
        return m_mouseX;
    }

    float InputState::mouseY() const
    {
        return m_mouseY;
    }

    float InputState::mouseDeltaX() const
    {
        return m_mouseDeltaX;
    }

    float InputState::mouseDeltaY() const
    {
        return m_mouseDeltaY;
    }

    float InputState::wheelX() const
    {
        return m_wheelX;
    }

    float InputState::wheelY() const
    {
        return m_wheelY;
    }

    const std::vector<platform::GamepadId>& InputState::gamepads() const
    {
        return m_gamepads;
    }

    bool InputState::gamepadButtonDown(platform::GamepadId gamepad,
        platform::GamepadButton button) const
    {
        const std::size_t pad = indexOfGamepad(gamepad);
        const std::size_t index = static_cast<std::size_t>(button);
        if ( pad == kNoGamepad || index >= GamepadState::kButtonCount )
        {
            return false;
        }
        return m_gamepadStates[pad].buttons[index];
    }

    float InputState::gamepadAxis(platform::GamepadId gamepad, platform::GamepadAxis axis) const
    {
        const std::size_t pad = indexOfGamepad(gamepad);
        const std::size_t index = static_cast<std::size_t>(axis);
        if ( pad == kNoGamepad || index >= GamepadState::kAxisCount )
        {
            return 0.0f;
        }
        return m_gamepadStates[pad].axes[index];
    }

    void InputState::applyPress(ButtonState& state, bool down)
    {
        if ( down )
        {
            if ( !state.down )
            {
                state.down = true;
                state.pressed = true;
            }
        }
        else if ( state.down )
        {
            state.down = false;
            state.released = true;
        }
    }

    void InputState::releaseAll()
    {
        for ( ButtonState& state : m_keys )
        {
            if ( state.down )
            {
                state.down = false;
                state.released = true;
            }
        }
        for ( ButtonState& state : m_mouseButtons )
        {
            if ( state.down )
            {
                state.down = false;
                state.released = true;
            }
        }
    }

    std::size_t InputState::indexOfGamepad(platform::GamepadId gamepad) const
    {
        const auto found = std::find(m_gamepads.begin(), m_gamepads.end(), gamepad);
        if ( found == m_gamepads.end() )
        {
            return kNoGamepad;
        }
        return static_cast<std::size_t>(found - m_gamepads.begin());
    }

}

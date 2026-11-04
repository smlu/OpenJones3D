// SPDX-License-Identifier: MPL-2.0
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include <babylon/platform/Event.h>

#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <vector>

namespace babylon::input
{

    // The input a game reads during one frame. Feed every platform event
    // through apply(), and call beginFrame() once at the start of each
    // frame, before applying that frame's events.
    //
    // Deterministic: the same events in the same order give the same state.
    // Not thread-safe.
    class InputState
    {
    public:
        // Clears what is per frame: pressed and released flags, mouse
        // motion and wheel sums. Positions, held keys and buttons, and
        // gamepad state carry over.
        void beginFrame();

        // Applies one event. Events that aren't input (window, quit) are
        // ignored, except WindowFocusChanged with focused == false, which
        // releases every held key and mouse button (each counts as released
        // this frame) so nothing sticks while another window has focus.
        // Key repeats don't change the state, and events for gamepads that
        // aren't connected are ignored.
        void apply(const platform::Event& event);

        bool keyDown(platform::Key key) const;
        bool keyPressed(platform::Key key) const;     // went down during this frame
        bool keyReleased(platform::Key key) const;    // went up during this frame

        bool mouseButtonDown(platform::MouseButton button) const;
        bool mouseButtonPressed(platform::MouseButton button) const;
        bool mouseButtonReleased(platform::MouseButton button) const;

        float mouseX() const;       // last reported position, 0 before any motion
        float mouseY() const;
        float mouseDeltaX() const;  // sum of relative motion this frame
        float mouseDeltaY() const;
        float wheelX() const;       // sum of wheel motion this frame
        float wheelY() const;

        // Connected gamepads in the order they connected.
        const std::vector<platform::GamepadId>& gamepads() const;
        bool gamepadButtonDown(platform::GamepadId gamepad, platform::GamepadButton button) const;
        float gamepadAxis(platform::GamepadId gamepad, platform::GamepadAxis axis) const;   // 0 for unknown pads

    private:
        // One key or mouse button: the current state plus the transitions
        // of this frame. A key pressed and released within one frame ends up
        // pressed and released but not down.
        struct ButtonState
        {
            bool down = false;
            bool pressed = false;
            bool released = false;
        };

        // One connected gamepad's current state, indexed by the enums.
        // Buttons have no per-frame flags; disconnecting forgets the state.
        struct GamepadState
        {
            static constexpr std::size_t kButtonCount =
                static_cast<std::size_t>(platform::GamepadButton::DpadRight) + 1;
            static constexpr std::size_t kAxisCount =
                static_cast<std::size_t>(platform::GamepadAxis::RightTrigger) + 1;

            std::array<bool, kButtonCount> buttons = {};
            std::array<float, kAxisCount> axes = {};
        };

        // indexOfGamepad()'s answer for a pad that is not connected.
        static constexpr std::size_t kNoGamepad =
            std::numeric_limits<std::size_t>::max();

        // Applies one press or release edge. Only real transitions change
        // the state: a duplicate down or up leaves the flags as they are.
        static void applyPress(ButtonState& state, bool down);

        // Releases every held key and mouse button, counting each as
        // released this frame. Called when the focus is lost.
        void releaseAll();

        // Position of the pad in m_gamepads, or kNoGamepad.
        std::size_t indexOfGamepad(platform::GamepadId gamepad) const;

        // Indexed by the enums; entry 0 of m_keys is Key::Unknown, which
        // is never set. Keys and buttons outside their tables are ignored.
        std::array<ButtonState, static_cast<std::size_t>(platform::Key::Count)> m_keys = {};
        std::array<ButtonState, static_cast<std::size_t>(platform::MouseButton::X2) + 1>
            m_mouseButtons = {};

        float m_mouseX = 0.0f;
        float m_mouseY = 0.0f;
        float m_mouseDeltaX = 0.0f;
        float m_mouseDeltaY = 0.0f;
        float m_wheelX = 0.0f;
        float m_wheelY = 0.0f;

        // m_gamepadStates mirrors m_gamepads entry for entry.
        std::vector<platform::GamepadId> m_gamepads;
        std::vector<GamepadState> m_gamepadStates;
    };

}

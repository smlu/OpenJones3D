// SPDX-License-Identifier: MPL-2.0
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "SdlEventTranslation.h"

#include <babylon/platform/Event.h>
#include <babylon/platform/Keys.h>

#include <SDL3/SDL_events.h>
#include <SDL3/SDL_gamepad.h>
#include <SDL3/SDL_mouse.h>
#include <SDL3/SDL_scancode.h>

#include <algorithm>
#include <cstdint>
#include <optional>

namespace babylon::platform::sdl
{
namespace
{

    // The raw SDL button number, 1..5, onto the MouseButton enumerators.
    // SDL buttons beyond X2 are dropped rather than delivered as X2, so a
    // wheel button or a sixth key on a mouse never pretends to be a side
    // button.
    std::optional<MouseButton> mouseButtonFromSdl(std::uint8_t sdlButton)
    {
        switch ( sdlButton )
        {
            case SDL_BUTTON_LEFT:   return MouseButton::Left;
            case SDL_BUTTON_MIDDLE: return MouseButton::Middle;
            case SDL_BUTTON_RIGHT:  return MouseButton::Right;
            case SDL_BUTTON_X1:     return MouseButton::X1;
            case SDL_BUTTON_X2:     return MouseButton::X2;
            default:                return std::nullopt;
        }
    }

    // The raw SDL button index onto the GamepadButton enumerators; SDL's
    // extra buttons (paddles, touchpad, misc) are dropped: Babylon names
    // the fifteen positionally identifiable ones.
    std::optional<GamepadButton> gamepadButtonFromSdl(std::uint8_t sdlButton)
    {
        switch ( sdlButton )
        {
            case SDL_GAMEPAD_BUTTON_SOUTH:          return GamepadButton::South;
            case SDL_GAMEPAD_BUTTON_EAST:           return GamepadButton::East;
            case SDL_GAMEPAD_BUTTON_WEST:           return GamepadButton::West;
            case SDL_GAMEPAD_BUTTON_NORTH:          return GamepadButton::North;
            case SDL_GAMEPAD_BUTTON_BACK:           return GamepadButton::Back;
            case SDL_GAMEPAD_BUTTON_GUIDE:          return GamepadButton::Guide;
            case SDL_GAMEPAD_BUTTON_START:          return GamepadButton::Start;
            case SDL_GAMEPAD_BUTTON_LEFT_STICK:     return GamepadButton::LeftStick;
            case SDL_GAMEPAD_BUTTON_RIGHT_STICK:    return GamepadButton::RightStick;
            case SDL_GAMEPAD_BUTTON_LEFT_SHOULDER:  return GamepadButton::LeftShoulder;
            case SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER: return GamepadButton::RightShoulder;
            case SDL_GAMEPAD_BUTTON_DPAD_UP:        return GamepadButton::DpadUp;
            case SDL_GAMEPAD_BUTTON_DPAD_DOWN:      return GamepadButton::DpadDown;
            case SDL_GAMEPAD_BUTTON_DPAD_LEFT:      return GamepadButton::DpadLeft;
            case SDL_GAMEPAD_BUTTON_DPAD_RIGHT:     return GamepadButton::DpadRight;
            default:                                return std::nullopt;
        }
    }

    // The raw SDL axis index onto the GamepadAxis enumerators.
    std::optional<GamepadAxis> gamepadAxisFromSdl(std::uint8_t sdlAxis)
    {
        switch ( sdlAxis )
        {
            case SDL_GAMEPAD_AXIS_LEFTX:         return GamepadAxis::LeftX;
            case SDL_GAMEPAD_AXIS_LEFTY:         return GamepadAxis::LeftY;
            case SDL_GAMEPAD_AXIS_RIGHTX:        return GamepadAxis::RightX;
            case SDL_GAMEPAD_AXIS_RIGHTY:        return GamepadAxis::RightY;
            case SDL_GAMEPAD_AXIS_LEFT_TRIGGER:  return GamepadAxis::LeftTrigger;
            case SDL_GAMEPAD_AXIS_RIGHT_TRIGGER: return GamepadAxis::RightTrigger;
            default:                             return std::nullopt;
        }
    }

}

    Key translateScancode(SDL_Scancode scancode)
    {
        switch ( scancode )
        {
            case SDL_SCANCODE_A: return Key::A;
            case SDL_SCANCODE_B: return Key::B;
            case SDL_SCANCODE_C: return Key::C;
            case SDL_SCANCODE_D: return Key::D;
            case SDL_SCANCODE_E: return Key::E;
            case SDL_SCANCODE_F: return Key::F;
            case SDL_SCANCODE_G: return Key::G;
            case SDL_SCANCODE_H: return Key::H;
            case SDL_SCANCODE_I: return Key::I;
            case SDL_SCANCODE_J: return Key::J;
            case SDL_SCANCODE_K: return Key::K;
            case SDL_SCANCODE_L: return Key::L;
            case SDL_SCANCODE_M: return Key::M;
            case SDL_SCANCODE_N: return Key::N;
            case SDL_SCANCODE_O: return Key::O;
            case SDL_SCANCODE_P: return Key::P;
            case SDL_SCANCODE_Q: return Key::Q;
            case SDL_SCANCODE_R: return Key::R;
            case SDL_SCANCODE_S: return Key::S;
            case SDL_SCANCODE_T: return Key::T;
            case SDL_SCANCODE_U: return Key::U;
            case SDL_SCANCODE_V: return Key::V;
            case SDL_SCANCODE_W: return Key::W;
            case SDL_SCANCODE_X: return Key::X;
            case SDL_SCANCODE_Y: return Key::Y;
            case SDL_SCANCODE_Z: return Key::Z;

            case SDL_SCANCODE_0: return Key::Num0;
            case SDL_SCANCODE_1: return Key::Num1;
            case SDL_SCANCODE_2: return Key::Num2;
            case SDL_SCANCODE_3: return Key::Num3;
            case SDL_SCANCODE_4: return Key::Num4;
            case SDL_SCANCODE_5: return Key::Num5;
            case SDL_SCANCODE_6: return Key::Num6;
            case SDL_SCANCODE_7: return Key::Num7;
            case SDL_SCANCODE_8: return Key::Num8;
            case SDL_SCANCODE_9: return Key::Num9;

            case SDL_SCANCODE_F1: return Key::F1;
            case SDL_SCANCODE_F2: return Key::F2;
            case SDL_SCANCODE_F3: return Key::F3;
            case SDL_SCANCODE_F4: return Key::F4;
            case SDL_SCANCODE_F5: return Key::F5;
            case SDL_SCANCODE_F6: return Key::F6;
            case SDL_SCANCODE_F7: return Key::F7;
            case SDL_SCANCODE_F8: return Key::F8;
            case SDL_SCANCODE_F9: return Key::F9;
            case SDL_SCANCODE_F10: return Key::F10;
            case SDL_SCANCODE_F11: return Key::F11;
            case SDL_SCANCODE_F12: return Key::F12;

            case SDL_SCANCODE_ESCAPE: return Key::Escape;
            case SDL_SCANCODE_RETURN: return Key::Enter;
            case SDL_SCANCODE_TAB: return Key::Tab;
            case SDL_SCANCODE_BACKSPACE: return Key::Backspace;
            case SDL_SCANCODE_SPACE: return Key::Space;

            case SDL_SCANCODE_MINUS: return Key::Minus;
            case SDL_SCANCODE_EQUALS: return Key::Equals;
            case SDL_SCANCODE_LEFTBRACKET: return Key::LeftBracket;
            case SDL_SCANCODE_RIGHTBRACKET: return Key::RightBracket;
            case SDL_SCANCODE_BACKSLASH: return Key::Backslash;
            case SDL_SCANCODE_SEMICOLON: return Key::Semicolon;
            case SDL_SCANCODE_APOSTROPHE: return Key::Apostrophe;
            case SDL_SCANCODE_GRAVE: return Key::Grave;
            case SDL_SCANCODE_COMMA: return Key::Comma;
            case SDL_SCANCODE_PERIOD: return Key::Period;
            case SDL_SCANCODE_SLASH: return Key::Slash;

            case SDL_SCANCODE_CAPSLOCK: return Key::CapsLock;
            case SDL_SCANCODE_SCROLLLOCK: return Key::ScrollLock;
            case SDL_SCANCODE_NUMLOCKCLEAR: return Key::NumLock;
            case SDL_SCANCODE_PRINTSCREEN: return Key::PrintScreen;
            case SDL_SCANCODE_PAUSE: return Key::Pause;

            case SDL_SCANCODE_INSERT: return Key::Insert;
            case SDL_SCANCODE_DELETE: return Key::Delete;
            case SDL_SCANCODE_HOME: return Key::Home;
            case SDL_SCANCODE_END: return Key::End;
            case SDL_SCANCODE_PAGEUP: return Key::PageUp;
            case SDL_SCANCODE_PAGEDOWN: return Key::PageDown;

            case SDL_SCANCODE_LEFT: return Key::Left;
            case SDL_SCANCODE_RIGHT: return Key::Right;
            case SDL_SCANCODE_UP: return Key::Up;
            case SDL_SCANCODE_DOWN: return Key::Down;

            case SDL_SCANCODE_KP_0: return Key::Keypad0;
            case SDL_SCANCODE_KP_1: return Key::Keypad1;
            case SDL_SCANCODE_KP_2: return Key::Keypad2;
            case SDL_SCANCODE_KP_3: return Key::Keypad3;
            case SDL_SCANCODE_KP_4: return Key::Keypad4;
            case SDL_SCANCODE_KP_5: return Key::Keypad5;
            case SDL_SCANCODE_KP_6: return Key::Keypad6;
            case SDL_SCANCODE_KP_7: return Key::Keypad7;
            case SDL_SCANCODE_KP_8: return Key::Keypad8;
            case SDL_SCANCODE_KP_9: return Key::Keypad9;
            case SDL_SCANCODE_KP_DIVIDE: return Key::KeypadDivide;
            case SDL_SCANCODE_KP_MULTIPLY: return Key::KeypadMultiply;
            case SDL_SCANCODE_KP_MINUS: return Key::KeypadMinus;
            case SDL_SCANCODE_KP_PLUS: return Key::KeypadPlus;
            case SDL_SCANCODE_KP_ENTER: return Key::KeypadEnter;
            case SDL_SCANCODE_KP_PERIOD: return Key::KeypadPeriod;

            case SDL_SCANCODE_LSHIFT: return Key::LeftShift;
            case SDL_SCANCODE_RSHIFT: return Key::RightShift;
            case SDL_SCANCODE_LCTRL: return Key::LeftCtrl;
            case SDL_SCANCODE_RCTRL: return Key::RightCtrl;
            case SDL_SCANCODE_LALT: return Key::LeftAlt;
            case SDL_SCANCODE_RALT: return Key::RightAlt;
            case SDL_SCANCODE_LGUI: return Key::LeftMeta;
            case SDL_SCANCODE_RGUI: return Key::RightMeta;

            default:
                // F13 and up, the international and consumer keys and
                // anything newer than this table: the key stays Unknown, but
                // the event is still delivered so the game can log it.
                return Key::Unknown;
        }
    }

    std::optional<Event> translateEvent(const SDL_Event& event)
    {
        switch ( event.type )
        {
            case SDL_EVENT_QUIT:
                return QuitRequested{};

            case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
                return WindowCloseRequested{ static_cast<WindowId>(event.window.windowID) };

            case SDL_EVENT_WINDOW_FOCUS_GAINED:
                return WindowFocusChanged{ static_cast<WindowId>(event.window.windowID), true };

            case SDL_EVENT_WINDOW_FOCUS_LOST:
                return WindowFocusChanged{ static_cast<WindowId>(event.window.windowID), false };

            case SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED:
                return WindowResized{ static_cast<WindowId>(event.window.windowID),
                    Size{ event.window.data1, event.window.data2 } };

            case SDL_EVENT_KEY_DOWN:
            case SDL_EVENT_KEY_UP:
                return KeyChanged{ static_cast<WindowId>(event.key.windowID),
                    translateScancode(event.key.scancode),
                    event.type == SDL_EVENT_KEY_DOWN, event.key.repeat };

            case SDL_EVENT_MOUSE_MOTION:
                return MouseMoved{ static_cast<WindowId>(event.motion.windowID),
                    event.motion.x, event.motion.y, event.motion.xrel, event.motion.yrel };

            case SDL_EVENT_MOUSE_BUTTON_DOWN:
            case SDL_EVENT_MOUSE_BUTTON_UP:
            {
                const std::optional<MouseButton> button = mouseButtonFromSdl(event.button.button);
                if ( !button.has_value() )
                {
                    return std::nullopt;   // beyond X2: dropped
                }
                return MouseButtonChanged{ static_cast<WindowId>(event.button.windowID), *button,
                    event.type == SDL_EVENT_MOUSE_BUTTON_DOWN };
            }

            case SDL_EVENT_MOUSE_WHEEL:
            {
                // SDL reports a flipped wheel with the values upside down
                // and only the direction marking what they meant; Babylon
                // always means "positive dy away from the user".
                float dx = event.wheel.x;
                float dy = event.wheel.y;
                if ( event.wheel.direction == SDL_MOUSEWHEEL_FLIPPED )
                {
                    dx = -dx;
                    dy = -dy;
                }
                return MouseWheelMoved{ static_cast<WindowId>(event.wheel.windowID), dx, dy };
            }

            case SDL_EVENT_GAMEPAD_ADDED:
                return GamepadConnected{ static_cast<GamepadId>(event.gdevice.which) };

            case SDL_EVENT_GAMEPAD_REMOVED:
                return GamepadDisconnected{ static_cast<GamepadId>(event.gdevice.which) };

            case SDL_EVENT_GAMEPAD_BUTTON_DOWN:
            case SDL_EVENT_GAMEPAD_BUTTON_UP:
            {
                const std::optional<GamepadButton> button =
                    gamepadButtonFromSdl(event.gbutton.button);
                if ( !button.has_value() )
                {
                    return std::nullopt;   // SDL's paddles and extra buttons: dropped
                }
                return GamepadButtonChanged{ static_cast<GamepadId>(event.gbutton.which), *button,
                    event.type == SDL_EVENT_GAMEPAD_BUTTON_DOWN };
            }

            case SDL_EVENT_GAMEPAD_AXIS_MOTION:
            {
                const std::optional<GamepadAxis> axis = gamepadAxisFromSdl(event.gaxis.axis);
                if ( !axis.has_value() )
                {
                    return std::nullopt;   // an axis Babylon does not name
                }
                // SDL's raw range is -32768..32767; 32767 is one full
                // step, and the clamp makes -32768 exactly -1. Triggers
                // rest at -32768, so their low half is not wanted.
                const float raw = event.gaxis.value / 32767.0f;
                const bool trigger = *axis == GamepadAxis::LeftTrigger
                    || *axis == GamepadAxis::RightTrigger;
                const float value = trigger ? std::clamp(raw, 0.0f, 1.0f)
                                            : std::clamp(raw, -1.0f, 1.0f);
                return GamepadAxisMoved{ static_cast<GamepadId>(event.gaxis.which), *axis, value };
            }

            default:
                // Every other SDL event (display changes, sensors, audio
                // devices, the clipboard, unmapped buttons) is not part of
                // this queue.
                return std::nullopt;
        }
    }

}

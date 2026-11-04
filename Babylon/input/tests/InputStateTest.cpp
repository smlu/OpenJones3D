// SPDX-License-Identifier: MPL-2.0
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <babylon/input/InputState.h>

#include <babylon/platform/Event.h>
#include <babylon/platform/Keys.h>

#include <doctest.h>

#include <cstddef>
#include <cstdint>
#include <vector>

namespace
{
    using babylon::input::InputState;
    using babylon::platform::Event;
    using babylon::platform::GamepadAxis;
    using babylon::platform::GamepadAxisMoved;
    using babylon::platform::GamepadButton;
    using babylon::platform::GamepadButtonChanged;
    using babylon::platform::GamepadConnected;
    using babylon::platform::GamepadDisconnected;
    using babylon::platform::GamepadId;
    using babylon::platform::Key;
    using babylon::platform::KeyChanged;
    using babylon::platform::MouseButton;
    using babylon::platform::MouseButtonChanged;
    using babylon::platform::MouseMoved;
    using babylon::platform::MouseWheelMoved;
    using babylon::platform::QuitRequested;
    using babylon::platform::Size;
    using babylon::platform::WindowCloseRequested;
    using babylon::platform::WindowFocusChanged;
    using babylon::platform::WindowId;
    using babylon::platform::WindowResized;

    // The window id the synthetic events of these tests carry. InputState
    // does not filter on it; the events are just built as a device would.
    constexpr WindowId kWindow = 1;

    Event keyEvent(const Key key, const bool down, const bool repeat = false)
    {
        return KeyChanged{kWindow, key, down, repeat};
    }

    Event mouseButtonEvent(const MouseButton button, const bool down)
    {
        return MouseButtonChanged{kWindow, button, down};
    }

    // A deterministic 32-bit xorshift generator, so an event sequence never
    // depends on the wall clock or the C library's rand.
    struct Random
    {
        std::uint32_t value = 0x9E37'79B9u;

        std::uint32_t next()
        {
            value ^= value << 13;
            value ^= value >> 17;
            value ^= value << 5;
            return value;
        }
    };
}

TEST_CASE("A press holds across beginFrame until the release")
{
    InputState state;

    state.beginFrame();
    state.apply(keyEvent(Key::W, true));
    CHECK(state.keyDown(Key::W));
    CHECK(state.keyPressed(Key::W));
    CHECK_FALSE(state.keyReleased(Key::W));

    state.beginFrame();
    CHECK(state.keyDown(Key::W));           // held keys carry across frames
    CHECK_FALSE(state.keyPressed(Key::W));  // pressed is per frame
    CHECK_FALSE(state.keyReleased(Key::W));

    state.apply(keyEvent(Key::W, false));
    CHECK_FALSE(state.keyDown(Key::W));
    CHECK_FALSE(state.keyPressed(Key::W));
    CHECK(state.keyReleased(Key::W));

    state.beginFrame();
    CHECK_FALSE(state.keyReleased(Key::W));
    CHECK_FALSE(state.keyDown(Key::A));     // an untouched key reports nothing
}

TEST_CASE("A key pressed and released within one frame reports both edges, not down")
{
    InputState state;

    state.beginFrame();
    state.apply(keyEvent(Key::Space, true));
    state.apply(keyEvent(Key::Space, false));
    CHECK(state.keyPressed(Key::Space));
    CHECK(state.keyReleased(Key::Space));
    CHECK_FALSE(state.keyDown(Key::Space));
}

TEST_CASE("Key repeats and duplicate edges do not change the state")
{
    InputState state;

    state.beginFrame();
    state.apply(keyEvent(Key::Enter, true));
    state.beginFrame();

    // The operating system's repeat while the key is held.
    state.apply(keyEvent(Key::Enter, true, true));
    CHECK(state.keyDown(Key::Enter));
    CHECK_FALSE(state.keyPressed(Key::Enter));

    // A second down without the repeat flag is still not a new edge.
    state.apply(keyEvent(Key::Enter, true));
    CHECK(state.keyDown(Key::Enter));
    CHECK_FALSE(state.keyPressed(Key::Enter));

    // A stray up for a key that is not down changes nothing.
    state.apply(keyEvent(Key::Escape, false));
    CHECK_FALSE(state.keyReleased(Key::Escape));
}

TEST_CASE("Key::Unknown is never reported down")
{
    InputState state;

    state.beginFrame();
    state.apply(keyEvent(Key::Unknown, true));
    CHECK_FALSE(state.keyDown(Key::Unknown));
    CHECK_FALSE(state.keyPressed(Key::Unknown));
    state.apply(keyEvent(Key::Unknown, false));
    CHECK_FALSE(state.keyReleased(Key::Unknown));
}

TEST_CASE("Losing the focus releases held keys and mouse buttons")
{
    InputState state;

    state.beginFrame();
    state.apply(keyEvent(Key::W, true));
    state.apply(mouseButtonEvent(MouseButton::Left, true));
    state.apply(mouseButtonEvent(MouseButton::Right, true));
    state.beginFrame();
    state.apply(keyEvent(Key::W, true));   // a repeat does not re-press it
    state.apply(WindowFocusChanged{kWindow, false});

    // Everything held is down no longer and counts as released this frame.
    CHECK_FALSE(state.keyDown(Key::W));
    CHECK(state.keyReleased(Key::W));
    CHECK_FALSE(state.keyPressed(Key::W));
    CHECK_FALSE(state.mouseButtonDown(MouseButton::Left));
    CHECK(state.mouseButtonReleased(MouseButton::Left));
    CHECK_FALSE(state.mouseButtonDown(MouseButton::Right));
    CHECK(state.mouseButtonReleased(MouseButton::Right));

    // Buttons and keys that were not held are not marked released.
    CHECK_FALSE(state.keyReleased(Key::A));
    CHECK_FALSE(state.mouseButtonReleased(MouseButton::Middle));

    // The up that arrives later, after the focus loss, is a no-op.
    state.beginFrame();
    state.apply(keyEvent(Key::W, false));
    CHECK_FALSE(state.keyReleased(Key::W));
}

TEST_CASE("Mouse motion and wheel sum per frame; the position carries over")
{
    // Nothing reported before any motion.
    InputState fresh;
    CHECK(fresh.mouseX() == 0.0f);
    CHECK(fresh.mouseY() == 0.0f);
    CHECK(fresh.mouseDeltaX() == 0.0f);
    CHECK(fresh.mouseDeltaY() == 0.0f);
    CHECK(fresh.wheelX() == 0.0f);
    CHECK(fresh.wheelY() == 0.0f);

    InputState state;
    state.beginFrame();
    state.apply(MouseMoved{kWindow, 10.0f, 20.0f, 3.0f, -4.0f});
    state.apply(MouseMoved{kWindow, 11.5f, 20.0f, 0.5f, 1.0f});
    state.apply(MouseWheelMoved{kWindow, 0.0f, 1.0f});
    state.apply(MouseWheelMoved{kWindow, 1.0f, 0.0f});
    CHECK(state.mouseX() == 11.5f);
    CHECK(state.mouseY() == 20.0f);
    CHECK(state.mouseDeltaX() == 3.5f);
    CHECK(state.mouseDeltaY() == -3.0f);
    CHECK(state.wheelX() == 1.0f);
    CHECK(state.wheelY() == 1.0f);

    state.beginFrame();
    CHECK(state.mouseX() == 11.5f);    // position is not per frame
    CHECK(state.mouseY() == 20.0f);
    CHECK(state.mouseDeltaX() == 0.0f);
    CHECK(state.mouseDeltaY() == 0.0f);
    CHECK(state.wheelX() == 0.0f);
    CHECK(state.wheelY() == 0.0f);
}

TEST_CASE("Gamepad connect, state, disconnect and the connection order")
{
    InputState state;

    // Events for a pad that is not connected are ignored.
    state.apply(GamepadButtonChanged{7, GamepadButton::South, true});
    state.apply(GamepadAxisMoved{7, GamepadAxis::LeftX, 0.25f});
    CHECK_FALSE(state.gamepadButtonDown(7, GamepadButton::South));
    CHECK(state.gamepadAxis(7, GamepadAxis::LeftX) == 0.0f);

    state.apply(GamepadConnected{7});
    state.apply(GamepadConnected{2});
    state.apply(GamepadConnected{0});   // 0 means "no gamepad"
    CHECK(state.gamepads() == std::vector<GamepadId>{7, 2});   // order kept

    // A second connect of a known id is ignored and keeps its state.
    state.apply(GamepadButtonChanged{7, GamepadButton::South, true});
    state.apply(GamepadConnected{7});
    CHECK(state.gamepads() == std::vector<GamepadId>{7, 2});
    CHECK(state.gamepadButtonDown(7, GamepadButton::South));

    state.apply(GamepadAxisMoved{2, GamepadAxis::RightTrigger, 0.5f});
    CHECK(state.gamepadButtonDown(7, GamepadButton::South));
    CHECK_FALSE(state.gamepadButtonDown(7, GamepadButton::North));
    CHECK_FALSE(state.gamepadButtonDown(2, GamepadButton::South));
    CHECK(state.gamepadAxis(2, GamepadAxis::RightTrigger) == 0.5f);
    CHECK(state.gamepadAxis(2, GamepadAxis::LeftY) == 0.0f);    // untouched axis

    // Gamepad state is not per frame.
    state.beginFrame();
    CHECK(state.gamepadButtonDown(7, GamepadButton::South));
    CHECK(state.gamepadAxis(2, GamepadAxis::RightTrigger) == 0.5f);

    state.apply(GamepadDisconnected{7});
    CHECK(state.gamepads() == std::vector<GamepadId>{2});
    CHECK_FALSE(state.gamepadButtonDown(7, GamepadButton::South));   // forgotten
    CHECK(state.gamepadAxis(7, GamepadAxis::LeftX) == 0.0f);
    state.apply(GamepadDisconnected{7});    // disconnecting twice is a no-op
    CHECK(state.gamepads() == std::vector<GamepadId>{2});

    // Reconnecting starts from a clean state, at the end of the order.
    state.apply(GamepadConnected{7});
    CHECK(state.gamepads() == std::vector<GamepadId>{2, 7});
    CHECK_FALSE(state.gamepadButtonDown(7, GamepadButton::South));
}

TEST_CASE("Window and quit events are ignored, and focus gained changes nothing")
{
    InputState state;

    state.beginFrame();
    state.apply(keyEvent(Key::W, true));
    state.apply(QuitRequested{});
    state.apply(WindowCloseRequested{kWindow});
    state.apply(WindowResized{kWindow, Size{800, 600}});
    state.apply(WindowFocusChanged{kWindow, true});

    CHECK(state.keyDown(Key::W));
    CHECK(state.keyPressed(Key::W));
    CHECK_FALSE(state.keyReleased(Key::W));
    CHECK(state.mouseX() == 0.0f);
    CHECK(state.gamepads().empty());
}

TEST_CASE("The same 200-event sequence gives the same state twice")
{
    // One sequence of 200 mixed events, applied to two fresh states with
    // identical frame boundaries. Determinism means every answer must match.
    const Key keys[] = {Key::A, Key::W, Key::Space, Key::Enter, Key::F1, Key::LeftShift};
    const MouseButton buttons[] = {MouseButton::Left, MouseButton::Right, MouseButton::Middle};
    const GamepadButton padButtons[] = {GamepadButton::South, GamepadButton::Start, GamepadButton::DpadUp};
    const GamepadAxis padAxes[] = {GamepadAxis::LeftX, GamepadAxis::RightTrigger};

    Random random;
    std::vector<Event> events;
    events.push_back(GamepadConnected{1});
    while ( events.size() < 200 )
    {
        const std::uint32_t roll = random.next();
        switch ( roll % 7 )
        {
        case 0:
            events.push_back(KeyChanged{kWindow, keys[roll % 6], (roll >> 3) % 2 != 0,
                (roll >> 6) % 4 == 0});
            break;
        case 1:
            events.push_back(MouseButtonChanged{kWindow, buttons[roll % 3], (roll >> 3) % 2 != 0});
            break;
        case 2:
            events.push_back(MouseMoved{kWindow, static_cast<float>(roll % 100),
                static_cast<float>(roll % 60), static_cast<float>(roll % 9) - 4.0f,
                static_cast<float>((roll >> 4) % 7) - 3.0f});
            break;
        case 3:
            events.push_back(MouseWheelMoved{kWindow, static_cast<float>(roll % 3) - 1.0f,
                static_cast<float>((roll >> 5) % 5) - 2.0f});
            break;
        case 4:
            events.push_back(GamepadButtonChanged{1 + roll % 2, padButtons[roll % 3],
                (roll >> 3) % 2 != 0});
            break;
        case 5:
            events.push_back(GamepadAxisMoved{1 + roll % 2, padAxes[roll % 2],
                static_cast<float>(roll % 21) / 10.0f - 1.0f});
            break;
        default:
            // Mostly focus gained, sometimes a focus loss that releases all.
            events.push_back(WindowFocusChanged{kWindow, roll % 3 != 0});
            break;
        }
    }
    REQUIRE(events.size() == 200);

    InputState first;
    InputState second;
    for ( InputState* state : {&first, &second} )
    {
        for ( std::size_t i = 0; i < events.size(); ++i )
        {
            if ( i % 10 == 0 )
            {
                state->beginFrame();
            }
            state->apply(events[i]);
        }
    }

    for ( int value = 0; value < static_cast<int>(Key::Count); ++value )
    {
        const Key key = static_cast<Key>(value);
        INFO("key " << babylon::platform::keyName(key) << " value " << value);
        CHECK(first.keyDown(key) == second.keyDown(key));
        CHECK(first.keyPressed(key) == second.keyPressed(key));
        CHECK(first.keyReleased(key) == second.keyReleased(key));
    }
    for ( const MouseButton button : buttons )
    {
        CHECK(first.mouseButtonDown(button) == second.mouseButtonDown(button));
        CHECK(first.mouseButtonPressed(button) == second.mouseButtonPressed(button));
        CHECK(first.mouseButtonReleased(button) == second.mouseButtonReleased(button));
    }
    CHECK(first.mouseX() == second.mouseX());
    CHECK(first.mouseY() == second.mouseY());
    CHECK(first.mouseDeltaX() == second.mouseDeltaX());
    CHECK(first.mouseDeltaY() == second.mouseDeltaY());
    CHECK(first.wheelX() == second.wheelX());
    CHECK(first.wheelY() == second.wheelY());
    CHECK(first.gamepads() == second.gamepads());
    for ( const GamepadId gamepad : first.gamepads() )
    {
        for ( const GamepadButton button : padButtons )
        {
            CHECK(first.gamepadButtonDown(gamepad, button) == second.gamepadButtonDown(gamepad, button));
        }
        for ( const GamepadAxis axis : padAxes )
        {
            CHECK(first.gamepadAxis(gamepad, axis) == second.gamepadAxis(gamepad, axis));
        }
    }
}

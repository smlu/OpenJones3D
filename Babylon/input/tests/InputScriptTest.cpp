// SPDX-License-Identifier: MPL-2.0
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <babylon/input/InputScript.h>

#include <babylon/input/InputState.h>

#include <babylon/platform/Event.h>
#include <babylon/platform/Keys.h>

#include <doctest.h>

#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

namespace
{
    using babylon::input::InputScript;
    using babylon::input::ScriptError;
    using babylon::platform::Event;
    using babylon::platform::GamepadAxis;
    using babylon::platform::GamepadAxisMoved;
    using babylon::platform::GamepadButton;
    using babylon::platform::GamepadButtonChanged;
    using babylon::platform::GamepadConnected;
    using babylon::platform::GamepadDisconnected;
    using babylon::platform::Key;
    using babylon::platform::KeyChanged;
    using babylon::platform::MouseButton;
    using babylon::platform::MouseButtonChanged;
    using babylon::platform::MouseMoved;
    using babylon::platform::MouseWheelMoved;
    using babylon::platform::WindowId;

    // The window id parse() is called with in these tests; key and mouse
    // events must carry it back.
    constexpr WindowId kWindow = 7;

    // Parses text that must succeed and hands back the script.
    InputScript parseOk(std::string_view text, WindowId window = kWindow)
    {
        const std::variant<InputScript, ScriptError> result = InputScript::parse(text, window);
        const InputScript* script = std::get_if<InputScript>(&result);
        REQUIRE_MESSAGE(script != nullptr,
            "unexpected error: " << std::get<ScriptError>(result).message);
        return *script;
    }

    // Parses a one-event script and hands back that event.
    Event parseSingleEvent(std::string_view line, WindowId window = kWindow)
    {
        const InputScript script = parseOk(line, window);
        REQUIRE(script.eventCount() == 1);
        const std::optional<std::uint64_t> frame = script.lastFrame();
        REQUIRE(frame.has_value());
        const std::span<const Event> events = script.eventsForFrame(*frame);
        REQUIRE(events.size() == 1);
        return events[0];
    }

    // Parses text that must fail on exactly this line with exactly this message.
    void parseFails(std::string_view text, std::size_t line, std::string_view message)
    {
        const std::variant<InputScript, ScriptError> result = InputScript::parse(text, 1);
        const ScriptError* error = std::get_if<ScriptError>(&result);
        REQUIRE_MESSAGE(error != nullptr, "parse unexpectedly succeeded");
        INFO("expected line " << line << " message " << message);
        CHECK(error->line == line);
        CHECK(error->message == message);
    }
}

TEST_CASE("A key line gives the exact KeyChanged event")
{
    const Event down = parseSingleEvent("frame 0: key Enter down");
    const KeyChanged* key = std::get_if<KeyChanged>(&down);
    REQUIRE(key != nullptr);
    CHECK(key->window == kWindow);
    CHECK(key->key == Key::Enter);
    CHECK(key->down);
    CHECK_FALSE(key->repeat);

    const Event up = parseSingleEvent("frame 12: key LeftShift up");
    const KeyChanged* released = std::get_if<KeyChanged>(&up);
    REQUIRE(released != nullptr);
    CHECK(released->window == kWindow);
    CHECK(released->key == Key::LeftShift);
    CHECK_FALSE(released->down);
    CHECK_FALSE(released->repeat);
}

TEST_CASE("A mouse move line zeroes the position and keeps the motion")
{
    const Event event = parseSingleEvent("frame 3: mouse move -2.5 4");
    const MouseMoved* moved = std::get_if<MouseMoved>(&event);
    REQUIRE(moved != nullptr);
    CHECK(moved->window == kWindow);
    CHECK(moved->x == 0.0f);
    CHECK(moved->y == 0.0f);
    CHECK(moved->dx == -2.5f);
    CHECK(moved->dy == 4.0f);
}

TEST_CASE("A mouse button line gives the exact MouseButtonChanged event")
{
    const Event event = parseSingleEvent("frame 0: mouse button Right up");
    const MouseButtonChanged* button = std::get_if<MouseButtonChanged>(&event);
    REQUIRE(button != nullptr);
    CHECK(button->window == kWindow);
    CHECK(button->button == MouseButton::Right);
    CHECK_FALSE(button->down);
}

TEST_CASE("A mouse wheel line gives the exact MouseWheelMoved event")
{
    const Event event = parseSingleEvent("frame 10000000: mouse wheel 0 -1");
    const MouseWheelMoved* wheel = std::get_if<MouseWheelMoved>(&event);
    REQUIRE(wheel != nullptr);
    CHECK(wheel->window == kWindow);
    CHECK(wheel->dx == 0.0f);
    CHECK(wheel->dy == -1.0f);
}

TEST_CASE("Gamepad connect and disconnect turn the index into id index + 1")
{
    const Event connect = parseSingleEvent("frame 0: gamepad 0 connect");
    const GamepadConnected* connected = std::get_if<GamepadConnected>(&connect);
    REQUIRE(connected != nullptr);
    CHECK(connected->gamepad == 1);

    const Event disconnect = parseSingleEvent("frame 1: gamepad 15 disconnect");
    const GamepadDisconnected* disconnected = std::get_if<GamepadDisconnected>(&disconnect);
    REQUIRE(disconnected != nullptr);
    CHECK(disconnected->gamepad == 16);
}

TEST_CASE("A gamepad button line gives the exact GamepadButtonChanged event")
{
    const Event event = parseSingleEvent("frame 0: gamepad 2 button South down");
    const GamepadButtonChanged* button = std::get_if<GamepadButtonChanged>(&event);
    REQUIRE(button != nullptr);
    CHECK(button->gamepad == 3);
    CHECK(button->button == GamepadButton::South);
    CHECK(button->down);
}

TEST_CASE("A gamepad axis line gives the exact GamepadAxisMoved event")
{
    const Event stick = parseSingleEvent("frame 0: gamepad 0 axis RightX -1");
    const GamepadAxisMoved* left = std::get_if<GamepadAxisMoved>(&stick);
    REQUIRE(left != nullptr);
    CHECK(left->gamepad == 1);
    CHECK(left->axis == GamepadAxis::RightX);
    CHECK(left->value == -1.0f);

    const Event trigger = parseSingleEvent("frame 0: gamepad 2 axis LeftTrigger 0.5");
    const GamepadAxisMoved* right = std::get_if<GamepadAxisMoved>(&trigger);
    REQUIRE(right != nullptr);
    CHECK(right->axis == GamepadAxis::LeftTrigger);
    CHECK(right->value == 0.5f);
}

TEST_CASE("Several lines of one frame keep their file order")
{
    const InputScript script = parseOk(
        "frame 2: key A down\n"
        "frame 2: mouse move 1 2\n"
        "frame 2: key A up\n");
    CHECK(script.eventCount() == 3);
    CHECK(script.lastFrame() == std::optional<std::uint64_t>{2});

    const std::span<const Event> events = script.eventsForFrame(2);
    REQUIRE(events.size() == 3);
    CHECK(std::holds_alternative<KeyChanged>(events[0]));
    CHECK(std::holds_alternative<MouseMoved>(events[1]));
    const KeyChanged* up = std::get_if<KeyChanged>(&events[2]);
    REQUIRE(up != nullptr);
    CHECK_FALSE(up->down);
}

TEST_CASE("Comments, blank lines and CRLF endings are accepted")
{
    const InputScript script = parseOk(
        "\n"
        "# a leading comment\n"
        "   \t \n"
        "frame 0: key A down # trailing comment\n"
        "\n"
        "frame 1: key A up\r\n"
        "# the end");
    CHECK(script.eventCount() == 2);
    CHECK(script.lastFrame() == std::optional<std::uint64_t>{1});
}

TEST_CASE("An empty script holds no events")
{
    const InputScript script = parseOk("");
    CHECK(script.eventCount() == 0);
    CHECK(script.lastFrame() == std::nullopt);
    CHECK(script.eventsForFrame(0).empty());
}

TEST_CASE("A frame without events gives an empty span")
{
    const InputScript script = parseOk(
        "frame 3: key A down\n"
        "frame 9: key A up\n");
    CHECK(script.eventsForFrame(0).empty());
    CHECK(script.eventsForFrame(4).empty());
    CHECK(script.eventsForFrame(10).empty());
    CHECK(script.eventsForFrame(3).size() == 1);
    CHECK(script.eventsForFrame(9).size() == 1);
}

TEST_CASE("Errors name the problem on the exact line")
{
    parseFails("frame 0: touch A down", 1, "unknown verb 'touch'");
    parseFails("frame 0: key Foo down", 1, "unknown key name 'Foo'");
    parseFails("frame 0: key A", 1, "missing 'down' or 'up'");
    parseFails("frame 0: key A middle", 1, "expected 'down' or 'up', found 'middle'");
    parseFails("frame 0: key A down\nframe 5: key A up\nframe 4: key B down", 3,
        "frame number 4 is below previous frame 5");
    parseFails("frame 10000001: key A down", 1, "frame number 10000001 is above 10000000");
    parseFails("frame -1: key A down", 1, "frame number must not be negative");
    parseFails("frame 0 key A down", 1, "missing ':' after the frame number");
    parseFails("frame 0: key A down b", 1, "unexpected extra token 'b'");
    parseFails("frame 0: gamepad 16 connect", 1, "gamepad index 16 is above 15");
    parseFails("frame 0: gamepad 0 axis LeftX 1.5", 1, "axis value 1.5 is outside -1..1");
    parseFails("frame 0: gamepad 0 axis LeftTrigger -0.5", 1, "axis value -0.5 is outside 0..1");
    parseFails("frame 0: gamepad 0 axis LeftX nan", 1, "invalid axis value 'nan'");
    parseFails("frame 0: mouse move inf 0", 1, "invalid mouse motion 'inf'");
}

TEST_CASE("A line of 256 bytes passes the length check, a longer one is refused")
{
    const std::string text = "frame 0: key A down\n" + std::string(300, 'z') + "\n";
    parseFails(text, 2, "line is longer than 256 bytes");

    // A 256-byte line is still checked for its content, not refused for its
    // length; a 257-byte line is refused before anything else.
    parseFails(std::string(256, 'k'), 1, "missing ':' after the frame number");
    parseFails(std::string(257, 'k'), 1, "line is longer than 256 bytes");
}

TEST_CASE("Text larger than 4 MiB is refused before parsing")
{
    const std::string text(4u * 1024u * 1024u + 1u, '\n');
    parseFails(text, 1, "input script is larger than 4194304 bytes");
}

TEST_CASE("formatScriptError joins source, line and message")
{
    CHECK(babylon::input::formatScriptError("in.txt", ScriptError{3, "x"}) == "in.txt:3: x");
    CHECK(babylon::input::formatScriptError("", ScriptError{1, "unknown verb 'x'"})
        == ":1: unknown verb 'x'");
}

TEST_CASE("Feeding a parsed script to an InputState drives the expected state")
{
    const InputScript script = parseOk(
        "frame 0: gamepad 2 connect\n"
        "frame 0: key W down\n"
        "frame 1: mouse move 3 4\n"
        "frame 2: key W up\n"
        "frame 2: gamepad 2 button South down\n"
        "frame 4: mouse wheel 0 1");

    babylon::input::InputState state;
    const std::optional<std::uint64_t> last = script.lastFrame();
    REQUIRE(last.has_value());
    for ( std::uint64_t frame = 0; frame <= *last; ++frame )
    {
        state.beginFrame();
        for ( const Event& event : script.eventsForFrame(frame) )
        {
            state.apply(event);
        }

        if ( frame == 0 )
        {
            CHECK(state.keyDown(Key::W));
            CHECK(state.keyPressed(Key::W));
        }
        if ( frame == 1 )
        {
            CHECK(state.mouseDeltaX() == 3.0f);   // "3" is exact in a float
            CHECK(state.mouseDeltaY() == 4.0f);
            CHECK(state.mouseX() == 0.0f);        // the script carries no position
            CHECK_FALSE(state.keyPressed(Key::W));
        }
        if ( frame == 2 )
        {
            CHECK(state.keyReleased(Key::W));
            CHECK_FALSE(state.keyDown(Key::W));
        }
        if ( frame == 3 )
        {
            CHECK_FALSE(state.keyReleased(Key::W));   // empty frame clears the edge
        }
    }

    CHECK(state.gamepads() == std::vector<babylon::platform::GamepadId>{3});  // index 2 + 1
    CHECK(state.gamepadButtonDown(3, GamepadButton::South));
    CHECK(state.wheelY() == 1.0f);
    CHECK(state.mouseDeltaX() == 0.0f);           // motion belonged to frame 1
}

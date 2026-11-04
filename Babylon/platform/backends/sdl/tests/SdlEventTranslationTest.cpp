// SPDX-License-Identifier: MPL-2.0
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "../SdlEventTranslation.h"

#include <babylon/platform/Event.h>
#include <babylon/platform/Keys.h>

#include <SDL3/SDL_events.h>
#include <SDL3/SDL_gamepad.h>
#include <SDL3/SDL_mouse.h>
#include <SDL3/SDL_scancode.h>

#include <doctest.h>

#include <initializer_list>
#include <map>
#include <optional>
#include <utility>
#include <variant>

namespace
{
    using babylon::platform::Event;
    using babylon::platform::GamepadAxis;
    using babylon::platform::GamepadButton;
    using babylon::platform::GamepadId;
    using babylon::platform::Key;
    using babylon::platform::MouseButton;
    using babylon::platform::Size;
    using babylon::platform::WindowId;
    using babylon::platform::sdl::translateEvent;
    using babylon::platform::sdl::translateScancode;

    // The delivered event of one kind, or nullptr when translateEvent()
    // gave back nothing, or something else.
    template <typename Kind>
    const Kind* delivered(const std::optional<Event>& event)
    {
        if ( !event.has_value() )
        {
            return nullptr;
        }
        return std::get_if<Kind>(&*event);
    }

    // A zeroed SDL_Event with one type, so each case only sets the fields
    // the translation reads.
    SDL_Event sdlEvent(const SDL_EventType type)
    {
        SDL_Event event = {};
        event.type = type;
        return event;
    }
}

TEST_CASE("The window and quit events keep translating")
{
    const std::optional<Event> quit = translateEvent(sdlEvent(SDL_EVENT_QUIT));
    CHECK(std::holds_alternative<babylon::platform::QuitRequested>(*quit));

    SDL_Event event = sdlEvent(SDL_EVENT_WINDOW_CLOSE_REQUESTED);
    event.window.windowID = 12;
    const std::optional<Event> closed = translateEvent(event);
    const auto* const close = delivered<babylon::platform::WindowCloseRequested>(closed);
    REQUIRE(close != nullptr);
    CHECK(close->window == 12);

    event = sdlEvent(SDL_EVENT_WINDOW_FOCUS_GAINED);
    event.window.windowID = 4;
    const std::optional<Event> gainedOpt = translateEvent(event);
    const auto* const gained = delivered<babylon::platform::WindowFocusChanged>(gainedOpt);
    REQUIRE(gained != nullptr);
    CHECK(gained->window == 4);
    CHECK(gained->focused);

    event = sdlEvent(SDL_EVENT_WINDOW_FOCUS_LOST);
    event.window.windowID = 4;
    const std::optional<Event> lostOpt = translateEvent(event);
    const auto* const lost = delivered<babylon::platform::WindowFocusChanged>(lostOpt);
    REQUIRE(lost != nullptr);
    CHECK(lost->window == 4);
    CHECK_FALSE(lost->focused);

    event = sdlEvent(SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED);
    event.window.windowID = 9;
    event.window.data1 = 1024;
    event.window.data2 = 768;
    const std::optional<Event> resizedOpt = translateEvent(event);
    const auto* const resized = delivered<babylon::platform::WindowResized>(resizedOpt);
    REQUIRE(resized != nullptr);
    CHECK(resized->window == 9);
    CHECK(resized->size == Size{ 1024, 768 });
}

TEST_CASE("Key events carry the physical key, the window and the repeat")
{
    SDL_Event event = sdlEvent(SDL_EVENT_KEY_DOWN);
    event.key.windowID = 7;
    event.key.scancode = SDL_SCANCODE_A;
    event.key.repeat = false;
    const std::optional<Event> pressedEvent = translateEvent(event);
    const auto* const pressed = delivered<babylon::platform::KeyChanged>(pressedEvent);
    REQUIRE(pressed != nullptr);
    CHECK(pressed->window == 7);
    CHECK(pressed->key == Key::A);
    CHECK(pressed->down);
    CHECK_FALSE(pressed->repeat);

    event = sdlEvent(SDL_EVENT_KEY_UP);
    event.key.windowID = 7;
    event.key.scancode = SDL_SCANCODE_LSHIFT;
    const std::optional<Event> releasedEvent = translateEvent(event);
    const auto* const released = delivered<babylon::platform::KeyChanged>(releasedEvent);
    REQUIRE(released != nullptr);
    CHECK(released->key == Key::LeftShift);
    CHECK_FALSE(released->down);

    event = sdlEvent(SDL_EVENT_KEY_DOWN);
    event.key.windowID = 1;
    event.key.scancode = SDL_SCANCODE_KP_ENTER;
    const std::optional<Event> entered = translateEvent(event);
    const auto* const enter = delivered<babylon::platform::KeyChanged>(entered);
    REQUIRE(enter != nullptr);
    CHECK(enter->key == Key::KeypadEnter);

    // The operating system repeats a held key: the flag comes through.
    event = sdlEvent(SDL_EVENT_KEY_DOWN);
    event.key.windowID = 1;
    event.key.scancode = SDL_SCANCODE_SPACE;
    event.key.repeat = true;
    const std::optional<Event> repeatedEvent = translateEvent(event);
    const auto* const repeated = delivered<babylon::platform::KeyChanged>(repeatedEvent);
    REQUIRE(repeated != nullptr);
    CHECK(repeated->key == Key::Space);
    CHECK(repeated->repeat);

    // A key outside the table stays Unknown but is still delivered, so a
    // game can log it.
    event = sdlEvent(SDL_EVENT_KEY_DOWN);
    event.key.windowID = 1;
    event.key.scancode = SDL_SCANCODE_F13;
    const std::optional<Event> unmappedEvent = translateEvent(event);
    const auto* const unmapped = delivered<babylon::platform::KeyChanged>(unmappedEvent);
    REQUIRE(unmapped != nullptr);
    CHECK(unmapped->key == Key::Unknown);
    CHECK(unmapped->down);
}

TEST_CASE("Mouse motion, buttons and the wheel translate")
{
    SDL_Event event = sdlEvent(SDL_EVENT_MOUSE_MOTION);
    event.motion.windowID = 5;
    event.motion.x = 100.5f;
    event.motion.y = 200.25f;
    event.motion.xrel = -4.f;
    event.motion.yrel = 8.f;
    const std::optional<Event> movedEvent = translateEvent(event);
    const auto* const moved = delivered<babylon::platform::MouseMoved>(movedEvent);
    REQUIRE(moved != nullptr);
    CHECK(moved->window == 5);
    CHECK(moved->x == 100.5f);
    CHECK(moved->y == 200.25f);
    CHECK(moved->dx == -4.f);
    CHECK(moved->dy == 8.f);

    event = sdlEvent(SDL_EVENT_MOUSE_BUTTON_DOWN);
    event.button.windowID = 5;
    event.button.button = SDL_BUTTON_X2;
    const std::optional<Event> pressedButton = translateEvent(event);
    const auto* const pressed = delivered<babylon::platform::MouseButtonChanged>(pressedButton);
    REQUIRE(pressed != nullptr);
    CHECK(pressed->window == 5);
    CHECK(pressed->button == MouseButton::X2);
    CHECK(pressed->down);

    event = sdlEvent(SDL_EVENT_MOUSE_BUTTON_UP);
    event.button.windowID = 5;
    event.button.button = SDL_BUTTON_LEFT;
    const std::optional<Event> releasedButton = translateEvent(event);
    const auto* const released = delivered<babylon::platform::MouseButtonChanged>(releasedButton);
    REQUIRE(released != nullptr);
    CHECK(released->button == MouseButton::Left);
    CHECK_FALSE(released->down);

    // A button beyond X2 is dropped, not delivered.
    event = sdlEvent(SDL_EVENT_MOUSE_BUTTON_DOWN);
    event.button.windowID = 5;
    event.button.button = 7;
    CHECK(translateEvent(event) == std::nullopt);

    // The wheel: positive dy is away from the user, which is what SDL
    // already reports for a normal wheel.
    event = sdlEvent(SDL_EVENT_MOUSE_WHEEL);
    event.wheel.windowID = 5;
    event.wheel.x = 1.5f;
    event.wheel.y = -3.f;
    event.wheel.direction = SDL_MOUSEWHEEL_NORMAL;
    const std::optional<Event> wheelEvent = translateEvent(event);
    const auto* const wheel = delivered<babylon::platform::MouseWheelMoved>(wheelEvent);
    REQUIRE(wheel != nullptr);
    CHECK(wheel->window == 5);
    CHECK(wheel->dx == 1.5f);
    CHECK(wheel->dy == -3.f);

    // A flipped (natural) wheel reports its values upside down; both are
    // negated back.
    event = sdlEvent(SDL_EVENT_MOUSE_WHEEL);
    event.wheel.windowID = 5;
    event.wheel.x = 1.5f;
    event.wheel.y = -3.f;
    event.wheel.direction = SDL_MOUSEWHEEL_FLIPPED;
    const std::optional<Event> flippedEvent = translateEvent(event);
    const auto* const flipped = delivered<babylon::platform::MouseWheelMoved>(flippedEvent);
    REQUIRE(flipped != nullptr);
    CHECK(flipped->dx == -1.5f);
    CHECK(flipped->dy == 3.f);
}

TEST_CASE("Gamepad connect, disconnect, buttons and axes translate")
{
    SDL_Event event = sdlEvent(SDL_EVENT_GAMEPAD_ADDED);
    event.gdevice.which = 42;
    const std::optional<Event> addedEvent = translateEvent(event);
    const auto* const added = delivered<babylon::platform::GamepadConnected>(addedEvent);
    REQUIRE(added != nullptr);
    CHECK(added->gamepad == static_cast<GamepadId>(42));

    event = sdlEvent(SDL_EVENT_GAMEPAD_REMOVED);
    event.gdevice.which = 42;
    const std::optional<Event> removedEvent = translateEvent(event);
    const auto* const removed = delivered<babylon::platform::GamepadDisconnected>(removedEvent);
    REQUIRE(removed != nullptr);
    CHECK(removed->gamepad == static_cast<GamepadId>(42));

    event = sdlEvent(SDL_EVENT_GAMEPAD_BUTTON_DOWN);
    event.gbutton.which = 7;
    event.gbutton.button = SDL_GAMEPAD_BUTTON_SOUTH;
    const std::optional<Event> pressedPad = translateEvent(event);
    const auto* const pressed = delivered<babylon::platform::GamepadButtonChanged>(pressedPad);
    REQUIRE(pressed != nullptr);
    CHECK(pressed->gamepad == 7);
    CHECK(pressed->button == GamepadButton::South);
    CHECK(pressed->down);

    event = sdlEvent(SDL_EVENT_GAMEPAD_BUTTON_UP);
    event.gbutton.which = 7;
    event.gbutton.button = SDL_GAMEPAD_BUTTON_DPAD_RIGHT;
    const std::optional<Event> releasedPad = translateEvent(event);
    const auto* const released = delivered<babylon::platform::GamepadButtonChanged>(releasedPad);
    REQUIRE(released != nullptr);
    CHECK(released->button == GamepadButton::DpadRight);
    CHECK_FALSE(released->down);

    // SDL's extra buttons (paddles, touchpad, misc) are dropped.
    event = sdlEvent(SDL_EVENT_GAMEPAD_BUTTON_DOWN);
    event.gbutton.which = 7;
    event.gbutton.button = SDL_GAMEPAD_BUTTON_MISC1;
    CHECK(translateEvent(event) == std::nullopt);

    // A stick: the raw range maps onto -1..1, and one past the negative
    // extreme clamps to exactly -1.
    for ( const auto& [raw, expected] : std::initializer_list<std::pair<Sint16, float>> {
             { -32768, -1.f }, { 0, 0.f }, { 32767, 1.f } } )
    {
        INFO("stick value " << raw);
        event = sdlEvent(SDL_EVENT_GAMEPAD_AXIS_MOTION);
        event.gaxis.which = 7;
        event.gaxis.axis = SDL_GAMEPAD_AXIS_LEFTX;
        event.gaxis.value = raw;
        const std::optional<Event> axisEvent = translateEvent(event);
        const auto* const moved = delivered<babylon::platform::GamepadAxisMoved>(axisEvent);
        REQUIRE(moved != nullptr);
        CHECK(moved->gamepad == 7);
        CHECK(moved->axis == GamepadAxis::LeftX);
        CHECK(moved->value == expected);
    }

    // A trigger: the resting half of the range clamps to 0.
    for ( const auto& [raw, expected] : std::initializer_list<std::pair<Sint16, float>> {
             { -32768, 0.f }, { 0, 0.f }, { 32767, 1.f } } )
    {
        INFO("trigger value " << raw);
        event = sdlEvent(SDL_EVENT_GAMEPAD_AXIS_MOTION);
        event.gaxis.which = 7;
        event.gaxis.axis = SDL_GAMEPAD_AXIS_RIGHT_TRIGGER;
        event.gaxis.value = raw;
        const std::optional<Event> axisEvent = translateEvent(event);
        const auto* const moved = delivered<babylon::platform::GamepadAxisMoved>(axisEvent);
        REQUIRE(moved != nullptr);
        CHECK(moved->axis == GamepadAxis::RightTrigger);
        CHECK(moved->value == expected);
    }
}

TEST_CASE("Events Babylon does not deliver give nullopt")
{
    CHECK(translateEvent(sdlEvent(SDL_EVENT_CLIPBOARD_UPDATE)) == std::nullopt);
    CHECK(translateEvent(sdlEvent(SDL_EVENT_DISPLAY_ORIENTATION)) == std::nullopt);
    CHECK(translateEvent(sdlEvent(SDL_EVENT_GAMEPAD_REMAPPED)) == std::nullopt);
    CHECK(translateEvent(sdlEvent(SDL_EVENT_JOYSTICK_AXIS_MOTION)) == std::nullopt);
}

TEST_CASE("Every SDL scancode translates, and every key exactly once")
{
    // The whole scancode range, including the values SDL never sends,
    // translates without crashing and stays inside the Key enum.
    std::map<int, int> producers;   // Key value -> number of scancodes
    for ( int raw = 0; raw < SDL_SCANCODE_COUNT; ++raw )
    {
        const Key key = translateScancode(static_cast<SDL_Scancode>(raw));
        INFO("scancode " << raw);
        const int value = static_cast<int>(key);
        CHECK(value >= 0);
        CHECK(value < static_cast<int>(Key::Count));
        if ( key != Key::Unknown )
        {
            ++producers[value];
        }
    }

    // Every named key is produced by exactly one scancode, so the table is
    // complete and no two scancodes claim the same key.
    for ( int value = 1; value < static_cast<int>(Key::Count); ++value )
    {
        INFO("key " << babylon::platform::keyName(static_cast<Key>(value)) << " (value " << value
            << ")");
        CHECK(producers.count(value) == 1);
    }
    CHECK(producers.size() == static_cast<std::size_t>(static_cast<int>(Key::Count)) - 1);
}

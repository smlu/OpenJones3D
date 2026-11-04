// SPDX-License-Identifier: MPL-2.0
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <babylon/platform/Event.h>
#include <babylon/platform/Keys.h>
#include <babylon/platform/NullPlatform.h>
#include <babylon/platform/Platform.h>
#include <babylon/platform/Window.h>

#include <babylon/log/CaptureSink.h>
#include <babylon/log/Log.h>
#include <babylon/log/Sink.h>

#include <doctest.h>

#include <memory>
#include <optional>
#include <variant>

namespace
{
    using babylon::platform::kMaxWindowDimension;
    using babylon::platform::NullPlatform;
    using babylon::platform::Size;
    using babylon::platform::Window;
    using babylon::platform::WindowDesc;
}

TEST_CASE("isValidWindowSize accepts the bounds and refuses outside them")
{
    using babylon::platform::isValidWindowSize;

    // The two bounds themselves are legal in both dimensions.
    CHECK(isValidWindowSize(Size{ 1, 1 }));
    CHECK(isValidWindowSize(Size{ kMaxWindowDimension, kMaxWindowDimension }));
    CHECK(isValidWindowSize(Size{ 1, kMaxWindowDimension }));

    // Zero and one past the maximum are refused in either dimension.
    CHECK_FALSE(isValidWindowSize(Size{ 0, 1 }));
    CHECK_FALSE(isValidWindowSize(Size{ 1, 0 }));
    CHECK_FALSE(isValidWindowSize(Size{ 0, 0 }));
    CHECK_FALSE(isValidWindowSize(Size{ kMaxWindowDimension + 1, kMaxWindowDimension }));
    CHECK_FALSE(isValidWindowSize(Size{ kMaxWindowDimension, kMaxWindowDimension + 1 }));
    CHECK_FALSE(isValidWindowSize(Size{ -1, -1 }));
}

TEST_CASE("Null windows get ids counting up from 1")
{
    NullPlatform platform;
    babylon::log::CaptureSink sink;

    const std::unique_ptr<Window> first =
        platform.createWindow(WindowDesc{ "one", Size{ 640, 480 } });
    REQUIRE(first != nullptr);
    CHECK(first->id() == 1);

    const std::unique_ptr<Window> second =
        platform.createWindow(WindowDesc{ "two", Size{ 320, 200 } });
    REQUIRE(second != nullptr);
    CHECK(second->id() == 2);

    // An invalid size is refused with one warning and takes no id: the next
    // window keeps counting from where the accepted ones left off.
    CHECK(platform.createWindow(WindowDesc{ "zero", Size{ 0, 480 } }) == nullptr);
    REQUIRE(sink.records().size() == 1);
    CHECK(sink.records()[0].level == babylon::log::Level::Warning);
    sink.clear();

    const std::unique_ptr<Window> third =
        platform.createWindow(WindowDesc{ "three", Size{ 100, 100 } });
    REQUIRE(third != nullptr);
    CHECK(third->id() == 3);

    // A Null window holds exactly what it was given, and resize() always
    // takes a size the shared check allowed.
    CHECK(second->title() == "two");
    CHECK(second->size() == Size{ 320, 200 });
    CHECK(second->resize(Size{ 1, kMaxWindowDimension }));
    CHECK(second->size() == Size{ 1, kMaxWindowDimension });
    sink.clear();
    CHECK(platform.pollEvent() == std::nullopt);   // window creation queues nothing
    CHECK(sink.records().empty());
}

TEST_CASE("The Null queue hands back what was pushed, first in first out")
{
    NullPlatform platform;

    // An empty queue says so and does not wait for anything.
    CHECK(platform.pollEvent() == std::nullopt);

    platform.pushEvent(babylon::platform::QuitRequested{});
    platform.pushEvent(babylon::platform::WindowCloseRequested{ 7 });
    platform.pushEvent(babylon::platform::WindowFocusChanged{ 2, true });
    platform.pushEvent(babylon::platform::WindowResized{ 5, Size{ 1024, 768 } });
    platform.pushEvent(babylon::platform::WindowFocusChanged{ 2, false });

    const std::optional<babylon::platform::Event> quit = platform.pollEvent();
    REQUIRE(quit.has_value());
    CHECK(std::holds_alternative<babylon::platform::QuitRequested>(*quit));

    const std::optional<babylon::platform::Event> close = platform.pollEvent();
    REQUIRE(close.has_value());
    const auto* const closeEvent = std::get_if<babylon::platform::WindowCloseRequested>(&*close);
    REQUIRE(closeEvent != nullptr);
    CHECK(closeEvent->window == 7);

    const std::optional<babylon::platform::Event> gained = platform.pollEvent();
    REQUIRE(gained.has_value());
    const auto* const gainedEvent = std::get_if<babylon::platform::WindowFocusChanged>(&*gained);
    REQUIRE(gainedEvent != nullptr);
    CHECK(gainedEvent->window == 2);
    CHECK(gainedEvent->focused);

    const std::optional<babylon::platform::Event> resized = platform.pollEvent();
    REQUIRE(resized.has_value());
    const auto* const resizedEvent = std::get_if<babylon::platform::WindowResized>(&*resized);
    REQUIRE(resizedEvent != nullptr);
    CHECK(resizedEvent->window == 5);
    CHECK(resizedEvent->size == Size{ 1024, 768 });

    const std::optional<babylon::platform::Event> lost = platform.pollEvent();
    REQUIRE(lost.has_value());
    const auto* const lostEvent = std::get_if<babylon::platform::WindowFocusChanged>(&*lost);
    REQUIRE(lostEvent != nullptr);
    CHECK_FALSE(lostEvent->focused);

    // The queue is empty again, and a push after draining starts over.
    CHECK(platform.pollEvent() == std::nullopt);
    platform.pushEvent(babylon::platform::QuitRequested{});
    CHECK(platform.pollEvent() != std::nullopt);
    CHECK(platform.pollEvent() == std::nullopt);
}

TEST_CASE("The Null queue carries key, mouse and gamepad events unchanged")
{
    NullPlatform platform;

    platform.pushEvent(babylon::platform::KeyChanged{ 3, babylon::platform::Key::F5, true, false });
    platform.pushEvent(babylon::platform::MouseMoved{ 3, 120.5f, 64.25f, -8.f, 2.5f });
    platform.pushEvent(
        babylon::platform::MouseButtonChanged{ 4, babylon::platform::MouseButton::X2, true });
    platform.pushEvent(babylon::platform::MouseWheelMoved{ 4, 0.f, -1.5f });
    platform.pushEvent(babylon::platform::GamepadConnected{ 9 });
    platform.pushEvent(babylon::platform::GamepadDisconnected{ 9 });
    platform.pushEvent(babylon::platform::GamepadButtonChanged{ 9,
        babylon::platform::GamepadButton::North, true });
    platform.pushEvent(babylon::platform::GamepadAxisMoved{ 9,
        babylon::platform::GamepadAxis::LeftTrigger, 0.5f });

    const std::optional<babylon::platform::Event> key = platform.pollEvent();
    REQUIRE(key.has_value());
    const auto* const keyEvent = std::get_if<babylon::platform::KeyChanged>(&*key);
    REQUIRE(keyEvent != nullptr);
    CHECK(keyEvent->window == 3);
    CHECK(keyEvent->key == babylon::platform::Key::F5);
    CHECK(keyEvent->down);
    CHECK_FALSE(keyEvent->repeat);

    const std::optional<babylon::platform::Event> moved = platform.pollEvent();
    REQUIRE(moved.has_value());
    const auto* const movedEvent = std::get_if<babylon::platform::MouseMoved>(&*moved);
    REQUIRE(movedEvent != nullptr);
    CHECK(movedEvent->window == 3);
    CHECK(movedEvent->x == 120.5f);
    CHECK(movedEvent->y == 64.25f);
    CHECK(movedEvent->dx == -8.f);
    CHECK(movedEvent->dy == 2.5f);

    const std::optional<babylon::platform::Event> button = platform.pollEvent();
    REQUIRE(button.has_value());
    const auto* const buttonEvent = std::get_if<babylon::platform::MouseButtonChanged>(&*button);
    REQUIRE(buttonEvent != nullptr);
    CHECK(buttonEvent->window == 4);
    CHECK(buttonEvent->button == babylon::platform::MouseButton::X2);
    CHECK(buttonEvent->down);

    const std::optional<babylon::platform::Event> wheel = platform.pollEvent();
    REQUIRE(wheel.has_value());
    const auto* const wheelEvent = std::get_if<babylon::platform::MouseWheelMoved>(&*wheel);
    REQUIRE(wheelEvent != nullptr);
    CHECK(wheelEvent->window == 4);
    CHECK(wheelEvent->dx == 0.f);
    CHECK(wheelEvent->dy == -1.5f);

    const std::optional<babylon::platform::Event> added = platform.pollEvent();
    REQUIRE(added.has_value());
    const auto* const addedEvent = std::get_if<babylon::platform::GamepadConnected>(&*added);
    REQUIRE(addedEvent != nullptr);
    CHECK(addedEvent->gamepad == 9);

    const std::optional<babylon::platform::Event> removed = platform.pollEvent();
    REQUIRE(removed.has_value());
    const auto* const removedEvent =
        std::get_if<babylon::platform::GamepadDisconnected>(&*removed);
    REQUIRE(removedEvent != nullptr);
    CHECK(removedEvent->gamepad == 9);

    const std::optional<babylon::platform::Event> pressed = platform.pollEvent();
    REQUIRE(pressed.has_value());
    const auto* const pressedEvent =
        std::get_if<babylon::platform::GamepadButtonChanged>(&*pressed);
    REQUIRE(pressedEvent != nullptr);
    CHECK(pressedEvent->gamepad == 9);
    CHECK(pressedEvent->button == babylon::platform::GamepadButton::North);
    CHECK(pressedEvent->down);

    const std::optional<babylon::platform::Event> axis = platform.pollEvent();
    REQUIRE(axis.has_value());
    const auto* const axisEvent = std::get_if<babylon::platform::GamepadAxisMoved>(&*axis);
    REQUIRE(axisEvent != nullptr);
    CHECK(axisEvent->gamepad == 9);
    CHECK(axisEvent->axis == babylon::platform::GamepadAxis::LeftTrigger);
    CHECK(axisEvent->value == 0.5f);

    CHECK(platform.pollEvent() == std::nullopt);
}

TEST_CASE("The Null window records relative mouse mode")
{
    NullPlatform platform;
    babylon::log::CaptureSink sink;

    const std::unique_ptr<Window> window =
        platform.createWindow(WindowDesc{ "one", Size{ 64, 64 } });
    REQUIRE(window != nullptr);
    const std::unique_ptr<Window> other =
        platform.createWindow(WindowDesc{ "two", Size{ 64, 64 } });
    REQUIRE(other != nullptr);

    // Off as long as nobody asked, and the Null backend never refuses.
    CHECK_FALSE(window->relativeMouse());
    CHECK(window->setRelativeMouse(true));
    CHECK(window->relativeMouse());

    // The mode is recorded per window.
    CHECK_FALSE(other->relativeMouse());

    CHECK(window->setRelativeMouse(false));
    CHECK_FALSE(window->relativeMouse());

    // A mode that cannot fail never logs.
    CHECK(sink.records().empty());
}

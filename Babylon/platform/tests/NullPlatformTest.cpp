// SPDX-License-Identifier: MPL-2.0
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <babylon/platform/Event.h>
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

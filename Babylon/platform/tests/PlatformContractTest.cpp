// SPDX-License-Identifier: MPL-2.0
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <babylon/platform/Event.h>
#include <babylon/platform/NullPlatform.h>
#include <babylon/platform/Platform.h>
#include <babylon/platform/SdlPlatform.h>
#include <babylon/platform/Window.h>

#include <babylon/log/CaptureSink.h>
#include <babylon/log/Log.h>
#include <babylon/log/Sink.h>

#include <doctest.h>

#include <initializer_list>
#include <memory>
#include <optional>
#include <variant>

namespace
{
    using babylon::log::CaptureSink;
    using babylon::log::Level;
    using babylon::platform::kMaxWindowDimension;
    using babylon::platform::NullPlatform;
    using babylon::platform::Platform;
    using babylon::platform::Size;
    using babylon::platform::Window;
    using babylon::platform::WindowDesc;
    using babylon::platform::WindowId;

    // The Platform and Window contract, run through the interfaces against
    // one backend. Every failure the headers promise is exercised: exactly
    // one babylon::log::warning line per refused call, nothing logged on the
    // success paths, and the queue drained by polling only. Windows are
    // destroyed here, before the caller destroys the platform, as Platform.h
    // requires.
    void checkPlatformContract(Platform& platform)
    {
        CaptureSink sink;

        // SDL's event queue belongs to the process, not to one platform, so
        // a previous run's events can still be in it. They are drained with
        // the same cap as below before anything is checked.
        int leftovers = 0;
        for ( ; leftovers < 10000 && platform.pollEvent().has_value(); ++leftovers )
        {
        }
        CHECK(leftovers < 10000);

        const auto noWarning = [&sink]()
        {
            CHECK(sink.records().empty());
            sink.clear();
        };
        const auto oneWarning = [&sink]()
        {
            REQUIRE(sink.records().size() == 1);
            CHECK(sink.records()[0].level == Level::Warning);
            sink.clear();
        };

        // createWindow() gives back the title and size it was asked for and
        // a nonzero id; two windows have different ids.
        const WindowDesc desc{ "Babylon test", Size{ 320, 240 } };
        const std::unique_ptr<Window> window = platform.createWindow(desc);
        REQUIRE(window != nullptr);
        CHECK(window->title() == "Babylon test");
        CHECK(window->size() == Size{ 320, 240 });
        CHECK(window->id() != 0);
        noWarning();

        const std::unique_ptr<Window> second = platform.createWindow(desc);
        REQUIRE(second != nullptr);
        CHECK(second->id() != 0);
        CHECK(second->id() != window->id());
        noWarning();

        // setTitle() round-trips through title().
        window->setTitle("Babylon test renamed");
        CHECK(window->title() == "Babylon test renamed");
        noWarning();

        // A legal resize is applied and seen by size() before resize()
        // returns.
        CHECK(window->resize(Size{ 800, 600 }));
        noWarning();
        CHECK(window->size() == Size{ 800, 600 });

        // The shared size check refuses 0, -1 and one past the maximum in
        // either dimension: false, one warning, and the size unchanged.
        const Size before = window->size();
        const std::initializer_list<Size> invalidSizes = {
            Size{ 0, 600 }, Size{ 800, 0 }, Size{ -1, 600 }, Size{ 800, -1 },
            Size{ kMaxWindowDimension + 1, 600 },
            Size{ 800, kMaxWindowDimension + 1 },
        };
        for ( const Size bad : invalidSizes )
        {
            CHECK_FALSE(window->resize(bad));
            oneWarning();
            CHECK(window->size() == before);
        }

        // The same sizes are refused at creation: nullptr and one warning.
        CHECK(platform.createWindow(WindowDesc{ "Babylon test", Size{ 0, 240 } }) == nullptr);
        oneWarning();
        CHECK(platform.createWindow(
            WindowDesc{ "Babylon test", Size{ 320, kMaxWindowDimension + 1 } }) == nullptr);
        oneWarning();

        // A size at both bounds is accepted, so the shared check really is
        // 1..kMaxWindowDimension, and a resizable window is created too.
        const std::unique_ptr<Window> biggest = platform.createWindow(
            WindowDesc{ "Babylon test", Size{ 1, kMaxWindowDimension } });
        REQUIRE(biggest != nullptr);
        const std::unique_ptr<Window> resizable = platform.createWindow(
            WindowDesc{ "Babylon test", Size{ 200, 150 }, true });
        REQUIRE(resizable != nullptr);
        CHECK(resizable->id() != biggest->id());
        noWarning();

        // Draining the queue terminates: the backend does not make events
        // faster than they are taken out. 10,000 calls is far past what
        // four windows and one resize can produce.
        int polls = 0;
        bool drained = false;
        for ( ; polls < 10000; ++polls )
        {
            const std::optional<babylon::platform::Event> event = platform.pollEvent();
            if ( !event.has_value() )
            {
                drained = true;
                break;
            }
            // A window event names a window this platform made; a resized
            // event names a size one of them had.
            WindowId named = 0;
            bool isWindowEvent = false;
            bool isResize = false;
            Size reported{};
            if ( const auto* const close =
                    std::get_if<babylon::platform::WindowCloseRequested>(&*event) )
            {
                named = close->window;
                isWindowEvent = true;
            }
            else if ( const auto* const focus =
                    std::get_if<babylon::platform::WindowFocusChanged>(&*event) )
            {
                named = focus->window;
                isWindowEvent = true;
            }
            else if ( const auto* const resized =
                    std::get_if<babylon::platform::WindowResized>(&*event) )
            {
                named = resized->window;
                reported = resized->size;
                isWindowEvent = true;
                isResize = true;
            }
            if ( isWindowEvent )
            {
                const bool knownWindow = named == window->id()
                    || named == second->id() || named == biggest->id()
                    || named == resizable->id();
                CHECK(knownWindow);
            }
            if ( isResize )
            {
                const bool knownSize = reported == Size{ 320, 240 }
                    || reported == Size{ 800, 600 }
                    || reported == Size{ 1, kMaxWindowDimension }
                    || reported == Size{ 200, 150 };
                CHECK(knownSize);
            }
        }
        INFO("pollEvent() calls until the queue was empty: " << polls);
        CHECK(drained);
        noWarning();
    }
}

TEST_CASE("The Null platform honors the Platform contract")
{
    NullPlatform platform;
    checkPlatformContract(platform);
}

TEST_CASE("The SDL platform honors the Platform contract")
{
    // SDL's offscreen video driver, set by the test properties, needs no
    // display. Without it the platform is not usable at all, which is a
    // failure to fix, not a reason to skip.
    const std::unique_ptr<Platform> platform = babylon::platform::createSdlPlatform();
    if ( platform == nullptr )
    {
        FAIL("createSdlPlatform() returned nullptr: SDL could not initialize its "
            "video subsystem here; run this test with SDL_VIDEO_DRIVER=offscreen");
    }
    checkPlatformContract(*platform);

    // A second platform would need a second video subsystem, so it is
    // refused with one warning while this one lives.
    babylon::log::CaptureSink sink;
    CHECK(babylon::platform::createSdlPlatform() == nullptr);
    REQUIRE(sink.records().size() == 1);
    CHECK(sink.records()[0].level == Level::Warning);
}

TEST_CASE("The SDL platform puts a window resize it applied into the queue")
{
    // The event mapping of this backend cannot be reached through the
    // contract helper alone: only the window system produces the events.
    const std::unique_ptr<Platform> platform = babylon::platform::createSdlPlatform();
    if ( platform == nullptr )
    {
        FAIL("createSdlPlatform() returned nullptr: run this test with "
            "SDL_VIDEO_DRIVER=offscreen");
    }
    babylon::log::CaptureSink sink;

    const std::unique_ptr<Window> window =
        platform->createWindow(WindowDesc{ "Babylon test", Size{ 320, 240 } });
    REQUIRE(window != nullptr);
    REQUIRE(window->resize(Size{ 800, 600 }));
    CHECK(window->size() == Size{ 800, 600 });

    bool sawAppliedSize = false;
    for ( int polls = 0; polls < 10000; ++polls )
    {
        const std::optional<babylon::platform::Event> event = platform->pollEvent();
        if ( !event.has_value() )
        {
            break;
        }
        if ( const auto* const resized =
                std::get_if<babylon::platform::WindowResized>(&*event) )
        {
            sawAppliedSize = sawAppliedSize || (resized->window == window->id()
                && resized->size == Size{ 800, 600 });
        }
    }
    CHECK(sawAppliedSize);
    CHECK(sink.records().empty());
}

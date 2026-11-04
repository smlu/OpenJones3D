// Added: Unity Fixture suite for the Babylon column's one event pump. It drives stdBabylonEvents.cpp
// through a fresh StdBabylonContext whose platform is a babylon::platform::NullPlatform with one
// created window as pWindow (and once without a window at all), so every recorded quit, close and
// focus answer and every pumped key is exact and deterministic.
#include <unity_fixture.h>

#include <memory>
#include <optional>

#include <babylon/platform/Event.h>
#include <babylon/platform/Keys.h>
#include <babylon/platform/NullPlatform.h>
#include <babylon/platform/Platform.h>
#include <babylon/platform/Window.h>
#include <babylon/time/NullClock.h>

#include <std/Babylon/stdBabylon.h>
#include <std/Babylon/stdBabylonEvents.h>

TEST_GROUP(stdBabylonEvents);

namespace
{
    babylon::time::NullClock*         pNullClock     = nullptr;
    babylon::time::NullClock*         pVirtualClock  = nullptr;
    babylon::platform::NullPlatform*  pPlatform      = nullptr;
    std::unique_ptr<babylon::platform::Window> pWindow;
    StdBabylonContext                 context;

    void PushEvent(const babylon::platform::Event& event)
    {
        pPlatform->pushEvent(event);
    }
}

TEST_SETUP(stdBabylonEvents)
{
    pNullClock    = new babylon::time::NullClock();
    pVirtualClock = new babylon::time::NullClock();
    pPlatform     = new babylon::platform::NullPlatform();
    pWindow       = pPlatform->createWindow({ "stdBabylonEvents test", { 640, 480 }, false });
    TEST_ASSERT_NOT_NULL(pWindow.get());

    context.pClock          = pNullClock;
    context.pVirtualClock   = pVirtualClock;
    context.pFileSystem     = nullptr;
    context.pPlatform       = pPlatform;
    context.pScriptPlatform = nullptr;
    context.pWindow         = pWindow.get();
    stdBabylon_SetContext(&context);

    stdBabylonEvents_ResetForTest();
}

TEST_TEAR_DOWN(stdBabylonEvents)
{
    stdBabylonEvents_ResetForTest();
    stdBabylon_SetContext(nullptr);

    // The window dies before the platform that made it, like the interface requires.
    pWindow.reset();
    delete pPlatform;
    delete pVirtualClock;
    delete pNullClock;
    pPlatform     = nullptr;
    pVirtualClock = nullptr;
    pNullClock    = nullptr;
}

TEST(stdBabylonEvents, PumpReturnsTheNumberOfEventsTakenAndDrainsTheQueue)
{
    PushEvent(babylon::platform::KeyChanged{ pWindow->id(), babylon::platform::Key::W, true, false });
    PushEvent(babylon::platform::MouseMoved{ pWindow->id(), 10.0f, 20.0f, 1.0f, 2.0f });
    PushEvent(babylon::platform::QuitRequested{});

    TEST_ASSERT_EQUAL_UINT(3u, (unsigned)stdBabylonEvents_Pump());

    // The queue is drained: a second pump in the same frame takes nothing.
    TEST_ASSERT_EQUAL_UINT(0u, (unsigned)stdBabylonEvents_Pump());
}

TEST(stdBabylonEvents, QuitRequestedIsTakenOnce)
{
    TEST_ASSERT_FALSE(stdBabylonEvents_TakeQuitRequest());

    PushEvent(babylon::platform::QuitRequested{});
    TEST_ASSERT_EQUAL_UINT(1u, (unsigned)stdBabylonEvents_Pump());

    TEST_ASSERT_TRUE(stdBabylonEvents_TakeQuitRequest());
    TEST_ASSERT_FALSE(stdBabylonEvents_TakeQuitRequest());
}

TEST(stdBabylonEvents, GameWindowCloseIsTakenOnceLikeAQuit)
{
    PushEvent(babylon::platform::WindowCloseRequested{ pWindow->id() });
    TEST_ASSERT_EQUAL_UINT(1u, (unsigned)stdBabylonEvents_Pump());

    TEST_ASSERT_TRUE(stdBabylonEvents_TakeQuitRequest());
    TEST_ASSERT_FALSE(stdBabylonEvents_TakeQuitRequest());
}

TEST(stdBabylonEvents, OtherWindowCloseIsIgnored)
{
    PushEvent(babylon::platform::WindowCloseRequested{ pWindow->id() + 1 });
    TEST_ASSERT_EQUAL_UINT(1u, (unsigned)stdBabylonEvents_Pump());
    TEST_ASSERT_EQUAL_UINT(0u, (unsigned)stdBabylonEvents_Pump());

    TEST_ASSERT_FALSE(stdBabylonEvents_TakeQuitRequest());
    TEST_ASSERT_FALSE(stdBabylonEvents_TakeFocusChange().has_value());
}

TEST(stdBabylonEvents, GameWindowFocusChangesComeOutOldestFirst)
{
    PushEvent(babylon::platform::WindowFocusChanged{ pWindow->id(), true });
    PushEvent(babylon::platform::WindowFocusChanged{ pWindow->id() + 1, false });
    PushEvent(babylon::platform::WindowFocusChanged{ pWindow->id(), false });
    TEST_ASSERT_EQUAL_UINT(3u, (unsigned)stdBabylonEvents_Pump());

    const std::optional<bool> first = stdBabylonEvents_TakeFocusChange();
    TEST_ASSERT_TRUE(first.has_value());
    TEST_ASSERT_TRUE(*first);

    const std::optional<bool> second = stdBabylonEvents_TakeFocusChange();
    TEST_ASSERT_TRUE(second.has_value());
    TEST_ASSERT_FALSE(*second);

    // The other window's focus change was never recorded.
    TEST_ASSERT_FALSE(stdBabylonEvents_TakeFocusChange().has_value());
}

TEST(stdBabylonEvents, PumpedKeyEventsReachTheInputState)
{
    PushEvent(babylon::platform::KeyChanged{ pWindow->id(), babylon::platform::Key::W, true, false });
    TEST_ASSERT_EQUAL_UINT(1u, (unsigned)stdBabylonEvents_Pump());

    TEST_ASSERT_TRUE(stdBabylonEvents_InputState().keyDown(babylon::platform::Key::W));

    PushEvent(babylon::platform::KeyChanged{ pWindow->id(), babylon::platform::Key::W, false, false });
    TEST_ASSERT_EQUAL_UINT(1u, (unsigned)stdBabylonEvents_Pump());

    TEST_ASSERT_FALSE(stdBabylonEvents_InputState().keyDown(babylon::platform::Key::W));
}

TEST(stdBabylonEvents, WithoutAGameWindowCloseAndFocusAreIgnoredButQuitIsRecorded)
{
    context.pWindow = nullptr;

    PushEvent(babylon::platform::WindowCloseRequested{ 1 });
    PushEvent(babylon::platform::WindowFocusChanged{ 1, false });
    TEST_ASSERT_EQUAL_UINT(2u, (unsigned)stdBabylonEvents_Pump());

    TEST_ASSERT_FALSE(stdBabylonEvents_TakeQuitRequest());
    TEST_ASSERT_FALSE(stdBabylonEvents_TakeFocusChange().has_value());

    // A QuitRequested carries no window: the window adapter still gets its WM_QUIT answer.
    PushEvent(babylon::platform::QuitRequested{});
    TEST_ASSERT_EQUAL_UINT(1u, (unsigned)stdBabylonEvents_Pump());
    TEST_ASSERT_TRUE(stdBabylonEvents_TakeQuitRequest());
}

TEST_GROUP_RUNNER(stdBabylonEvents)
{
    RUN_TEST_CASE(stdBabylonEvents, PumpReturnsTheNumberOfEventsTakenAndDrainsTheQueue);
    RUN_TEST_CASE(stdBabylonEvents, QuitRequestedIsTakenOnce);
    RUN_TEST_CASE(stdBabylonEvents, GameWindowCloseIsTakenOnceLikeAQuit);
    RUN_TEST_CASE(stdBabylonEvents, OtherWindowCloseIsIgnored);
    RUN_TEST_CASE(stdBabylonEvents, GameWindowFocusChangesComeOutOldestFirst);
    RUN_TEST_CASE(stdBabylonEvents, PumpedKeyEventsReachTheInputState);
    RUN_TEST_CASE(stdBabylonEvents, WithoutAGameWindowCloseAndFocusAreIgnoredButQuitIsRecorded);
}

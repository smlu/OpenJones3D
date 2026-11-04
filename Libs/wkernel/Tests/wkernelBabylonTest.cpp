// Added: Unity Fixture suite for the Babylon column's platform side of wkernel
// (Libs/wkernel/wkernelBabylon.cpp). There is no test executable of the Babylon column that
// already links wkernel (Jones3D/Main/Tests is not built in the native columns), so this small
// suite covers wkernel_PeekProcessEvents' activation-callback order and its WM_QUIT answer,
// wkernel_ProcessEvents' first-event wake-up and wkernel_SetWindowSize through a fresh
// StdBabylonContext whose platform is a babylon::platform::NullPlatform with one created window
// as pWindow. The pump behind it is the real stdBabylonEvents.cpp, compiled into this executable
// with J3D_TEST so its ResetForTest is available.
// Adapter test written in C++; the C runner reaches the group through the extern "C" entry.
#include <unity_fixture.h>

#include <memory>

#include <babylon/platform/Event.h>
#include <babylon/platform/Keys.h>
#include <babylon/platform/NullPlatform.h>
#include <babylon/platform/Platform.h>
#include <babylon/platform/Window.h>
#include <babylon/time/NullClock.h>

#include <std/Babylon/stdBabylon.h>
#include <std/Babylon/stdBabylonEvents.h>

#include <wkernel/wkernel.h>

TEST_GROUP(wkernelBabylon);

// The linkage the C runner's declaration expects; the TEST_GROUP_RUNNER definition below
// inherits it from this declaration.
extern "C" void TEST_wkernelBabylon_GROUP_RUNNER(void);

namespace
{
    babylon::time::NullClock*        pClock        = nullptr;
    babylon::time::NullClock*        pVirtualClock = nullptr;
    babylon::platform::NullPlatform* pPlatform     = nullptr;
    std::unique_ptr<babylon::platform::Window> pWindow;
    StdBabylonContext                context;
    StdBabylonContext*               pSavedContext = nullptr;

    // The activations wkernel handed to the callback, oldest first.
    constexpr int kActivateLogMax = 16;
    int aActivateLog[kActivateLogMax];
    int numActivateLog = 0;
}

// The activation callback under test: it records what the game window's focus changes arrive as.
extern "C" void J3DAPI RecordActivate(int bActive)
{
    if ( numActivateLog < kActivateLogMax )
    {
        aActivateLog[numActivateLog] = bActive;
    }

    ++numActivateLog;
}

TEST_SETUP(wkernelBabylon)
{
    pSavedContext = stdBabylon_GetContext();

    pClock        = new babylon::time::NullClock();
    pVirtualClock = new babylon::time::NullClock();
    pPlatform     = new babylon::platform::NullPlatform();
    pWindow       = pPlatform->createWindow({ "wkernel test", { 640, 480 }, false });
    TEST_ASSERT_NOT_NULL(pWindow.get());

    context.pClock          = pClock;
    context.pVirtualClock   = pVirtualClock;
    context.pFileSystem     = nullptr;
    context.pPlatform       = pPlatform;
    context.pScriptPlatform = nullptr;
    context.pWindow         = pWindow.get();
    stdBabylon_SetContext(&context);

    numActivateLog = 0;
    stdBabylonEvents_ResetForTest();
    TEST_ASSERT_TRUE(wkernel_SetActivateCallback(RecordActivate) == nullptr);
}

TEST_TEAR_DOWN(wkernelBabylon)
{
    wkernel_SetActivateCallback(nullptr);
    stdBabylonEvents_ResetForTest();
    stdBabylon_SetContext(pSavedContext);

    // The window dies before the platform that made it, like the interface requires.
    pWindow.reset();
    delete pPlatform;
    delete pVirtualClock;
    delete pClock;
    pPlatform     = nullptr;
    pVirtualClock = nullptr;
    pClock        = nullptr;
    pSavedContext = nullptr;
}

TEST(wkernelBabylon, PeekProcessEventsDeliversFocusChangesOldestFirstAndAnswersQuit)
{
    pPlatform->pushEvent(babylon::platform::WindowFocusChanged{ pWindow->id(), true });
    pPlatform->pushEvent(babylon::platform::WindowFocusChanged{ pWindow->id() + 1, false });
    pPlatform->pushEvent(babylon::platform::KeyChanged{ pWindow->id(), babylon::platform::Key::W, true, false });
    pPlatform->pushEvent(babylon::platform::WindowFocusChanged{ pWindow->id(), false });
    pPlatform->pushEvent(babylon::platform::QuitRequested{});

    TEST_ASSERT_EQUAL_INT(1, wkernel_PeekProcessEvents());
    TEST_ASSERT_EQUAL_INT(2, numActivateLog);
    TEST_ASSERT_EQUAL_INT(1, aActivateLog[0]);
    TEST_ASSERT_EQUAL_INT(0, aActivateLog[1]);

    // The quit answer is the WM_QUIT taken from the queue: the next peek says no, and the
    // focus changes were delivered once.
    TEST_ASSERT_EQUAL_INT(0, wkernel_PeekProcessEvents());
    TEST_ASSERT_EQUAL_INT(2, numActivateLog);
}

TEST(wkernelBabylon, GameWindowCloseAnswersQuitAndDeliversNoFocus)
{
    pPlatform->pushEvent(babylon::platform::WindowCloseRequested{ pWindow->id() });

    TEST_ASSERT_EQUAL_INT(1, wkernel_PeekProcessEvents());
    TEST_ASSERT_EQUAL_INT(0, numActivateLog);
    TEST_ASSERT_EQUAL_INT(0, wkernel_PeekProcessEvents());
}

TEST(wkernelBabylon, OtherWindowCloseAndInputAnswerNoQuit)
{
    pPlatform->pushEvent(babylon::platform::WindowCloseRequested{ pWindow->id() + 1 });
    pPlatform->pushEvent(babylon::platform::KeyChanged{ pWindow->id(), babylon::platform::Key::Escape, true, false });

    TEST_ASSERT_EQUAL_INT(0, wkernel_PeekProcessEvents());
    TEST_ASSERT_EQUAL_INT(0, numActivateLog);
}

TEST(wkernelBabylon, ProcessEventsWakesOnTheFirstEvent)
{
    // The deactivated game's blocking pump: it answers on the first event taken, delivers its
    // focus change and keeps the quit answer for the event that carries one.
    pPlatform->pushEvent(babylon::platform::WindowFocusChanged{ pWindow->id(), false });

    TEST_ASSERT_EQUAL_INT(0, wkernel_ProcessEvents());
    TEST_ASSERT_EQUAL_INT(1, numActivateLog);
    TEST_ASSERT_EQUAL_INT(0, aActivateLog[0]);

    pPlatform->pushEvent(babylon::platform::QuitRequested{});
    TEST_ASSERT_EQUAL_INT(1, wkernel_ProcessEvents());
}

TEST(wkernelBabylon, SetActivateCallbackReturnsThePreviousOne)
{
    // The same answer pattern as wkernel_SetProcessProc: the setter hands back what it replaced.
    TEST_ASSERT_TRUE(wkernel_SetActivateCallback(nullptr) == RecordActivate);
    TEST_ASSERT_TRUE(wkernel_SetActivateCallback(RecordActivate) == nullptr);
}

TEST(wkernelBabylon, SetWindowSizeResizesTheGameWindowAndAnswersSuccess)
{
    TEST_ASSERT_EQUAL_INT(1, wkernel_SetWindowSize(800, 600));

    const babylon::platform::Size size = pWindow->size();
    TEST_ASSERT_EQUAL_INT(800, size.width);
    TEST_ASSERT_EQUAL_INT(600, size.height);
}

TEST_GROUP_RUNNER(wkernelBabylon)
{
    RUN_TEST_CASE(wkernelBabylon, PeekProcessEventsDeliversFocusChangesOldestFirstAndAnswersQuit);
    RUN_TEST_CASE(wkernelBabylon, GameWindowCloseAnswersQuitAndDeliversNoFocus);
    RUN_TEST_CASE(wkernelBabylon, OtherWindowCloseAndInputAnswerNoQuit);
    RUN_TEST_CASE(wkernelBabylon, ProcessEventsWakesOnTheFirstEvent);
    RUN_TEST_CASE(wkernelBabylon, SetActivateCallbackReturnsThePreviousOne);
    RUN_TEST_CASE(wkernelBabylon, SetWindowSizeResizesTheGameWindowAndAnswersSuccess);
}

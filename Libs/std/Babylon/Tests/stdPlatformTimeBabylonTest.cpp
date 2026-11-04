// Added: Unity Fixture suite for the Babylon column's stdPlatform time adapter. It compiles
// stdBabylon.cpp and stdPlatformTimeBabylon.cpp directly and drives them through a fresh
// StdBabylonContext whose both clocks are NullClocks and whose platform is a NullPlatform, so
// every value is exact and deterministic.
#include <unity_fixture.h>

#include <chrono>

#include <babylon/platform/NullPlatform.h>
#include <babylon/time/Clock.h>
#include <babylon/time/NullClock.h>

#include <std/Babylon/stdBabylon.h>
#include <std/General/stdPlatform.h>
#include <std/Null/stdPlatformNull.h>

TEST_GROUP(stdPlatformTimeBabylon);

namespace
{
    babylon::time::NullClock* pRealClock    = nullptr;
    babylon::time::NullClock* pVirtualClock = nullptr;
    babylon::platform::NullPlatform* pPlatform = nullptr;
    StdBabylonContext         context;

    uint64_t ClockMs(const babylon::time::Clock* pClock)
    {
        return static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(pClock->now()).count());
    }
}

TEST_SETUP(stdPlatformTimeBabylon)
{
    pRealClock    = new babylon::time::NullClock();
    pVirtualClock = new babylon::time::NullClock();
    pPlatform     = new babylon::platform::NullPlatform();
    context.pClock        = pRealClock;
    context.pVirtualClock = pVirtualClock;

    // The time adapter never touches files, so this suite leaves pFileSystem null. The context
    // contract wants a platform, and a NullPlatform is the one with no operating system.
    context.pFileSystem      = nullptr;
    context.pPlatform        = pPlatform;
    context.pScriptPlatform  = pPlatform;
    context.pWindow          = nullptr;
    stdBabylon_SetContext(&context);

    // The latch is file-static and never resets on its own; clear it so each test starts unlatched.
    stdPlatformTimeBabylon_ResetForTest();
}

TEST_TEAR_DOWN(stdPlatformTimeBabylon)
{
    stdBabylon_SetContext(nullptr);
    delete pPlatform;
    delete pRealClock;
    delete pVirtualClock;
    pPlatform     = nullptr;
    pRealClock    = nullptr;
    pVirtualClock = nullptr;
}

TEST(stdPlatformTimeBabylon, UnlatchedReadsAdvancesAndIgnoresTheRealClock)
{
    static_cast<void>(pRealClock->advance(std::chrono::milliseconds(1234)));
    TEST_ASSERT_EQUAL_UINT32(1234u, stdPlatform_GetTimeMsec());

    stdPlatform_Sleep(5u);
    TEST_ASSERT_EQUAL_UINT64(1239u, ClockMs(pRealClock));
    TEST_ASSERT_EQUAL_UINT32(1239u, stdPlatform_GetTimeMsec());

    // AdvanceClock is a no-op while the virtual clock is off.
    stdPlatformNull_AdvanceClock(7u);
    TEST_ASSERT_EQUAL_UINT64(1239u, ClockMs(pRealClock));
    TEST_ASSERT_EQUAL_UINT32(1239u, stdPlatform_GetTimeMsec());
}

TEST(stdPlatformTimeBabylon, LatchedUsesTheVirtualClockOnly)
{
    stdPlatformNull_UseVirtualClock(0x00100000u);
    TEST_ASSERT_EQUAL_UINT32(0x00100000u, stdPlatform_GetTimeMsec());

    stdPlatformNull_AdvanceClock(33u);
    TEST_ASSERT_EQUAL_UINT32(0x00100021u, stdPlatform_GetTimeMsec());

    stdPlatform_Sleep(10u);
    TEST_ASSERT_EQUAL_UINT32(0x0010002Bu, stdPlatform_GetTimeMsec());

    // Sleep on the virtual clock never touches the real clock.
    TEST_ASSERT_EQUAL_UINT64(0u, ClockMs(pRealClock));
}

TEST(stdPlatformTimeBabylon, VirtualClockWrapsAt2Pow32)
{
    stdPlatformNull_UseVirtualClock(0xFFFFFFF0u);
    stdPlatformNull_AdvanceClock(0x20u);
    TEST_ASSERT_EQUAL_UINT32(0x10u, stdPlatform_GetTimeMsec());
}

TEST(stdPlatformTimeBabylon, SubMillisecondElapsedIsTruncated)
{
    stdPlatformNull_UseVirtualClock(0x00100000u);
    static_cast<void>(pVirtualClock->advance(std::chrono::microseconds(999)));
    TEST_ASSERT_EQUAL_UINT32(0x00100000u, stdPlatform_GetTimeMsec());
}

TEST_GROUP_RUNNER(stdPlatformTimeBabylon)
{
    RUN_TEST_CASE(stdPlatformTimeBabylon, UnlatchedReadsAdvancesAndIgnoresTheRealClock);
    RUN_TEST_CASE(stdPlatformTimeBabylon, LatchedUsesTheVirtualClockOnly);
    RUN_TEST_CASE(stdPlatformTimeBabylon, VirtualClockWrapsAt2Pow32);
    RUN_TEST_CASE(stdPlatformTimeBabylon, SubMillisecondElapsedIsTruncated);
}

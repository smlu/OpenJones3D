// Added: the Babylon column's stdPlatform time services, running on the babylon::time::Clock pair
// the composition root installs. Same four functions, signatures and 2^32 wrap as the Null file;
// a missing context is a programming error (abort). Adapter code: no Fixed/Added markers inside.
#include <babylon/time/Clock.h>
#include <babylon/time/NullClock.h>

#include <chrono>
#include <cstdint>

#include <std/General/stdPlatform.h>
#include <std/Null/stdPlatformNull.h>

#include "stdBabylon.h"

static bool stdPlatformTimeBabylon_bVirtualClock = false;

extern "C" {

void stdPlatformNull_UseVirtualClock(tStdTime startMsec)
{
    StdBabylonContext& rContext = stdBabylon_RequireContext("stdPlatformNull_UseVirtualClock");

    // The virtual clock starts at zero, so latching at startMsec means advancing by startMsec.
    static_cast<void>(rContext.pVirtualClock->advance(std::chrono::milliseconds(static_cast<uint64_t>(startMsec))));
    stdPlatformTimeBabylon_bVirtualClock = true;
}

void stdPlatformNull_AdvanceClock(uint32_t msec)
{
    if ( !stdPlatformTimeBabylon_bVirtualClock )
    {
        return;
    }

    StdBabylonContext& rContext = stdBabylon_RequireContext("stdPlatformNull_AdvanceClock");
    static_cast<void>(rContext.pVirtualClock->advance(std::chrono::milliseconds(msec)));
}

tStdTime stdPlatform_GetTimeMsec(void)
{
    StdBabylonContext& rContext = stdBabylon_RequireContext("stdPlatform_GetTimeMsec");

    const babylon::time::Clock* pClock = stdPlatformTimeBabylon_bVirtualClock
        ? static_cast<const babylon::time::Clock*>(rContext.pVirtualClock)
        : rContext.pClock;

    const uint64_t ms = static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(pClock->now()).count());
    return static_cast<tStdTime>(ms);
}

void J3DAPI stdPlatform_Sleep(uint32_t msec)
{
    if ( stdPlatformTimeBabylon_bVirtualClock )
    {
        stdPlatformNull_AdvanceClock(msec);
        return;
    }

    StdBabylonContext& rContext = stdBabylon_RequireContext("stdPlatform_Sleep");
    rContext.pClock->sleepFor(std::chrono::milliseconds(msec));
}

} // extern "C"

#ifdef J3D_TEST
void stdPlatformTimeBabylon_ResetForTest()
{
    stdPlatformTimeBabylon_bVirtualClock = false;
}
#endif

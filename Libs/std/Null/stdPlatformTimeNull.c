// Added: the stdPlatform time services split out of stdPlatformNull.c so the Babylon column can
// replace just these four functions. Same Null behavior, same contracts: the virtual-clock state
// and the four entry points moved verbatim.
#include <time.h>

#include <j3dcore/j3d.h>

#include <std/General/stdPlatform.h>
#include <std/Null/stdPlatformNull.h>

// Added: Virtual clock state for the headless runner. Once stdPlatformNull_UseVirtualClock latches
// the clock on, stdPlatform_GetTimeMsec never reads the monotonic clock again, so nothing can slip
// between the times the engine samples; only the two entry points below move the clock.
static bool     stdPlatformNull_bVirtualClock = false;
static tStdTime stdPlatformNull_virtualTime   = 0;

void stdPlatformNull_UseVirtualClock(tStdTime startMsec)
{
    stdPlatformNull_virtualTime   = startMsec;
    stdPlatformNull_bVirtualClock = true;
}

void stdPlatformNull_AdvanceClock(uint32_t msec)
{
    if ( stdPlatformNull_bVirtualClock )
    {
        // Added: tStdTime is unsigned int, so the addition already wraps at 2^32 like timeGetTime.
        stdPlatformNull_virtualTime = (tStdTime)(stdPlatformNull_virtualTime + msec);
    }
}

// Added: POSIX clock is used to honor the tStdTime wrap contract. The Windows timeGetTime
// wraps at 2^32 (~49.7 days), so mask the nanosecond-converted milliseconds the same way.
tStdTime stdPlatform_GetTimeMsec(void)
{
    if ( stdPlatformNull_bVirtualClock )
    {
        return stdPlatformNull_virtualTime;
    }

    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);

    uint64_t msec = (uint64_t)ts.tv_sec * 1000u + (uint64_t)ts.tv_nsec / 1000000u;
    return (tStdTime)(msec & 0xFFFFFFFFu);
}

void J3DAPI stdPlatform_Sleep(uint32_t msec)
{
    // Added: On the virtual clock, sleeping means advancing it by the requested time: the caller
    // sees the same time delta as a real sleep without ever blocking the (single) thread.
    if ( stdPlatformNull_bVirtualClock )
    {
        stdPlatformNull_AdvanceClock(msec);
        return;
    }

    struct timespec req;
    uint64_t remaining = msec;

    while ( remaining )
    {
        req.tv_sec  = (time_t)(remaining / 1000u);
        req.tv_nsec = (long)((remaining % 1000u) * 1000000u);
        remaining -= (uint64_t)req.tv_sec * 1000u;
        if ( nanosleep(&req, &req) == 0 )
        {
            break;
        }
    }
}

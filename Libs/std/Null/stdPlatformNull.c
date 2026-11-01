// Added: Null platform layer of Libs/std/General/stdPlatform (the Windows one lives in
// std/Win95/stdPlatformWin32.c). There is no debugger, COM, minidump or console window on
// this platform: time comes from the monotonic clock, sleep and local time from POSIX, and
// the debugger-facing services degrade to abort/ stderr. Same header contracts.
#include <time.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <j3dcore/j3d.h>

#include <std/General/std.h>
#include <std/General/stdPlatform.h>
#include <std/General/stdPlatformSys.h>

// Added: POSIX clock is used to honor the tStdTime wrap contract. The Windows timeGetTime
// wraps at 2^32 (~49.7 days), so mask the nanosecond-converted milliseconds the same way.
tStdTime stdPlatform_GetTimeMsec(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);

    uint64_t msec = (uint64_t)ts.tv_sec * 1000u + (uint64_t)ts.tv_nsec / 1000000u;
    return (tStdTime)(msec & 0xFFFFFFFFu);
}

void J3DAPI stdPlatform_Sleep(uint32_t msec)
{
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

void J3DAPI stdPlatform_GetLocalTime(StdLocalTime* pTime)
{
    time_t now;
    struct tm localTime;

    time(&now);
    localtime_r(&now, &localTime);

    pTime->year   = (uint16_t)(localTime.tm_year + 1900);
    pTime->month  = (uint16_t)(localTime.tm_mon + 1);
    pTime->day    = (uint16_t)localTime.tm_mday;
    pTime->hour   = (uint16_t)localTime.tm_hour;
    pTime->minute = (uint16_t)localTime.tm_min;
    pTime->second = (uint16_t)localTime.tm_sec;
}

bool J3DAPI stdPlatform_DirExists(const char* pPath)
{
    struct stat st;
    return stat(pPath, &st) == 0 && S_ISDIR(st.st_mode);
}

// Added: No unwinding debugger support on the Null platform; the Windows version prints a
// mini-dump style trace. Nothing is printed here, so the call stays observable-log free.
void stdPlatform_PrintStackTrace(tStackTracePrintFunc pfPrintFunc, size_t numFrames)
{
    J3D_UNUSED(pfPrintFunc);
    J3D_UNUSED(numFrames);
}

// Added: No JIT debugger: break into abort, matching the DebugBreak contract (never returns).
void stdPlatformSys_DebugBreak(void)
{
    abort();
}

// Added: No assert message box: write to stderr like the debug-output service.
void stdPlatformSys_ShowAssertMessage(const char* pText)
{
    fprintf(stderr, "%s\n", pText);
}

// Added: No debug-output channel (OutputDebugString): write to stderr so logs stay visible.
void stdPlatformSys_DebugOutput(const char* pText)
{
    fprintf(stderr, "%s", pText);
}

// Added: No COM: return 0, the value of the successful S_OK path the Windows version returns,
// so callers with the if(expected)/error convention treat it as success.
int stdPlatformSys_CoInitialize(void)
{
    return 0;
}

// Added: No COM to balance; a safe no-op, as closing something that is not open.
void stdPlatformSys_CoUninitialize(void)
{
}

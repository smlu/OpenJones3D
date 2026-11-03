// TimerWin32.c: the Windows tick thread of the timer service (`TIMER=insane`) (part of original 0x004EA7D0-0x004EAE10).
#include <windows.h> // thread and event API
#include <mmsystem.h> // timeGetTime
#include <stdlib.h> // ldiv

#include <smush/insane/Timer/Timer.h>
#include <smush/insane/Timer/TimerSys.h>
#include <j3dcore/j3d.h>
#include <j3dcore/j3dhook.h>

DWORD WINAPI timer_Thread(LPVOID pArg);

static HANDLE timer_hThread;
static HANDLE timer_hEvent;

void timer_SysStartThread(void)
{
    timer_hEvent  = CreateEvent(NULL, FALSE, FALSE, NULL);

    DWORD id;
    timer_hThread = CreateThread(NULL, 0, timer_Thread, NULL, 0, &id);
    SetThreadPriority(timer_hThread, THREAD_PRIORITY_TIME_CRITICAL);
}

DWORD WINAPI timer_Thread(LPVOID pArg)
{
    (void)pArg;

    CRITICAL_SECTION cs;
    InitializeCriticalSection(&cs);

    ldiv_t step = ldiv(1000, TIMER_THREAD_RATE);
    int frac    = 0;
    int timeout = 0;
    int next    = timeGetTime();
    while ( WaitForSingleObject(timer_hEvent, timeout) != 0 )
    {
        EnterCriticalSection(&cs);
        timer_Tick();
        frac += step.rem;
        next += step.quot;
        if ( frac >= TIMER_THREAD_RATE )
        {
            frac -= TIMER_THREAD_RATE;
            next++;
        }

        LeaveCriticalSection(&cs);

        int delta = next - timeGetTime();
        timeout   = delta > 0 ? delta : 0;
    }

    DeleteCriticalSection(&cs);
    return 0;
}

void timer_SysStopThread(void)
{
    SetEvent(timer_hEvent);
    WaitForSingleObject(timer_hThread, INFINITE);
    CloseHandle(timer_hThread);
    CloseHandle(timer_hEvent);
}

void timer_SysResetGlobals(void)
{
    timer_hThread = NULL;
    timer_hEvent  = NULL;
}

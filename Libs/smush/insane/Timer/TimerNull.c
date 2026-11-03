// Added: Null version of smush/insane/Timer/TIMEWIN32.c: same header contract, a POSIX
// thread instead of CreateThread. The tick thread sleeps 1000/TIMER_THREAD_RATE ms each
// loop and calls timer_Tick, exactly like the Win32 tick thread's ldiv scheduling.
#include <pthread.h>
#include <time.h>

#include <j3dcore/j3d.h>

#include <smush/insane/Timer/Timer.h>
#include <smush/insane/Timer/TimerSys.h>

#include <std/General/std.h>

static pthread_t timer_thread;
static volatile int timer_bThreadRunning = false;

static void* timer_ThreadProc(void* pArg)
{
    J3D_UNUSED(pArg);

    const long msecStep = 1000 / TIMER_THREAD_RATE;
    struct timespec ts = { 0, msecStep * 1000000L };

    while ( timer_bThreadRunning )
    {
        nanosleep(&ts, NULL);
        timer_Tick();
    }

    return NULL;
}

void timer_SysStartThread(void)
{
    if ( timer_bThreadRunning )
    {
        return;
    }

    timer_bThreadRunning = true;
    if ( pthread_create(&timer_thread, NULL, timer_ThreadProc, NULL) != 0 )
    {
        timer_bThreadRunning = false;
        STDLOG_ERROR("timer_SysStartThread: Failed to create the tick thread.\n");
        return;
    }
}

void timer_SysStopThread(void)
{
    if ( !timer_bThreadRunning )
    {
        return;
    }

    timer_bThreadRunning = false;
    pthread_join(timer_thread, NULL);
}

void timer_SysInstallHooks(void)
{
    // Added: No RTI hooks in the standalone Null build: nothing to hook.
}

void timer_SysResetGlobals(void)
{
}

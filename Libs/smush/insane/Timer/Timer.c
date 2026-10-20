// Timer.c: the timer service (`TIMER=insane`) and its TIME_CRITICAL tick thread (original 0x004EA7D0-0x004EAE10).
#include <smush/insane/Timer/Timer.h>
#include <j3dcore/j3d.h>
#include <j3dcore/j3dhook.h>
#include <smush/RTI/symbols.h>
#include <string.h>
#include <setjmp.h>
#include <stdlib.h>
#include <windows.h>
#include <mmsystem.h> // timeGetTime

static LecModule timer_module = { timer_Register, timer_Unregister, timer_Startup, timer_Shutdown };
static TimerInterface timer_interface = {
    timer_Alloc,
    timer_Free,
    timer_Pause,
    timer_Resume,
    timer_SetInterval,
    timer_Wait,
    timer_GetTicks,
    timer_GetTickLen,
    timer_Sync
};

static int timer_field54D84C = -1;

static void* timer_pServices;
static int timer_rate;
static jmp_buf timer_jmpA;
static jmp_buf timer_jmpB;
static volatile int timer_ticks; // Volatile: timer_Wait reloads it on each iteration while the timer thread increments it (the original reloads it across the opaque timer_GetTicks call; not a behavior change)
static LecShare* timer_pShare;
static int timer_x0B8;
static int timer_x0BC;
static TimerEntry timer_entries[TIMER_COUNT + 1];
static HANDLE timer_hThread;
static void* timer_pParam;
static int timer_tickLen;
static HANDLE timer_hEvent;
static int timer_reent;
static int timer_inTick;
static int timer_savedReent;
static int timer_refs;
static int timer_paused;
static int timer_x364;
static volatile int timer_x368;
static int timer_x36C;
static int timer_x370;
static int timer_x374;

int J3DAPI timer_Register(LecShare* pShare, void* pParam)
{
    timer_pShare = pShare;
    timer_pParam = pParam;
    pShare->Publish("LECMODULE=insane_timer", &timer_module);
    timer_pShare->Publish("TIMER=insane", &timer_interface);
    timer_rate = 300;
    return 0;
}

void timer_Unregister(void)
{
    timer_pShare->Unpublish("TIMER=insane");
    timer_pShare->Unpublish("LECMODULE=insane_timer");
}

int timer_Startup(void)
{
    timer_pServices = timer_pShare->Subscribe("MEMORY.workspace", timer_pParam);
    if ( timer_pServices == NULL )
    {
        timer_pShare->ReportError("Timer Module: Cannot allocate memory services.");
        return -1;
    }

    return timer_Start(1193180 / timer_rate, 0x10000);
}

void timer_Shutdown(void)
{
    timer_pShare->Unsubscribe(timer_pServices);
    timer_Stop();
}

int J3DAPI timer_Start(int a, int b)
{
    (void)a;
    (void)b;

    if ( timer_refs++ == 0 )
    {
        for ( int i = 1; i < TIMER_COUNT + 1; i++ )
        {
            timer_entries[i].pfn = NULL;
        }

        timer_tickLen = 0x4DAE;
        timer_hEvent  = CreateEvent(NULL, FALSE, FALSE, NULL);

        DWORD id;
        timer_hThread = CreateThread(NULL, 0, timer_Thread, NULL, 0, &id);
        SetThreadPriority(timer_hThread, THREAD_PRIORITY_TIME_CRITICAL);
        timer_field54D84C = -1;
    }

    return 0;
}

DWORD WINAPI timer_Thread(LPVOID pArg)
{
    (void)pArg;

    CRITICAL_SECTION cs;
    InitializeCriticalSection(&cs);

    ldiv_t step = ldiv(1000, 60);
    int frac    = 0;
    int timeout = 0;
    int next    = timeGetTime();
    while ( WaitForSingleObject(timer_hEvent, timeout) != 0 )
    {
        EnterCriticalSection(&cs);
        timer_Tick();
        frac += step.rem;
        next += step.quot;
        if ( frac >= 60 )
        {
            frac -= 60;
            next++;
        }

        LeaveCriticalSection(&cs);

        int delta = next - timeGetTime();
        timeout   = delta > 0 ? delta : 0;
    }

    DeleteCriticalSection(&cs);
    return 0;
}

void timer_Stop(void)
{
    if ( --timer_refs == 0 )
    {
        SetEvent(timer_hEvent);
        WaitForSingleObject(timer_hThread, INFINITE);
        CloseHandle(timer_hThread);
        CloseHandle(timer_hEvent);
    }
}

void timer_Pause(void)
{
    timer_paused++;
}

void timer_Resume(void)
{
    timer_paused--;
}

int J3DAPI timer_Alloc(TimerCallback pfn, int interval, uint32_t flags, int param)
{
    for ( int i = 1; i < TIMER_COUNT + 1; i++ )
    {
        if ( timer_entries[i].pfn == NULL )
        {
            timer_entries[i].busy     = 0;
            timer_entries[i].elapsed  = 0;
            timer_entries[i].param    = param;
            timer_entries[i].flags    = flags;
            timer_entries[i].interval = interval;
            timer_entries[i].pfn      = pfn;
            timer_entries[i].running  = 0;
            timer_entries[i].total    = 0;
            return i;
        }
    }

    return 0;
}

void J3DAPI timer_Free(int index)
{
    if ( index > 0 && index < TIMER_COUNT + 1 )
    {
        timer_entries[index].pfn = NULL;
    }
}

void J3DAPI timer_SetInterval(int index, int interval)
{
    if ( index > 0 && index < TIMER_COUNT + 1 )
    {
        timer_entries[index].interval = interval;
        timer_entries[index].elapsed  = 0;
    }
}

void J3DAPI timer_Wait(int ticks)
{
    int start = timer_ticks;
    int end   = start + ticks;
    int cur   = start;

    while ( cur >= start && cur < end )
    {
        timer_GetTicks();
        cur = timer_ticks;
    }
}

int timer_GetTicks(void)
{
    return timer_ticks;
}

int timer_GetTickLen(void)
{
    return timer_tickLen;
}

void J3DAPI timer_Sync(int ticks)
{
    for ( int i = 1; i < TIMER_COUNT + 1; i++ )
    {
        TimerEntry* pEntry = &timer_entries[i];
        if ( pEntry->pfn && (pEntry->flags & 8) )
        {
            if ( ticks > 0 )
            {
                pEntry->elapsed += ticks - pEntry->total;
            }

            pEntry->total = 0;
        }
    }
}

void timer_Tick(void)
{
    timer_ticks += timer_tickLen;
    if ( timer_paused == 0 )
    {
        timer_savedReent = timer_reent;
        timer_inTick     = 1;
        timer_reent      = 0;

        for ( int i = 1; i < TIMER_COUNT + 1; i++ )
        {
            int step = timer_tickLen;
            if ( timer_entries[i].pfn )
            {
                timer_entries[i].elapsed += step;
                timer_entries[i].total += step;
                if ( (timer_entries[i].flags & 5) == 1 && timer_entries[i].interval != 0 )
                {
                    while ( timer_entries[i].elapsed >= timer_entries[i].interval )
                    {
                        timer_entries[i].elapsed -= timer_entries[i].interval;
                        if ( timer_entries[i].flags & 2 )
                        {
                            timer_entries[i].interval = 0;
                            timer_entries[i].pfn(i, timer_entries[i].param);
                            break;
                        }

                        timer_entries[i].pfn(i, timer_entries[i].param);
                    }
                }
            }
        }

        for ( int i = 1; i < TIMER_COUNT + 1; i++ )
        {
            if ( timer_entries[i].pfn && !(timer_entries[i].flags & 5) && timer_entries[i].interval != 0 && timer_entries[i].busy == 0 )
            {
                timer_entries[i].busy = 1;
                while ( timer_entries[i].elapsed >= timer_entries[i].interval )
                {
                    timer_entries[i].elapsed -= timer_entries[i].interval;
                    if ( timer_entries[i].flags & 2 )
                    {
                        timer_entries[i].interval = 0;
                        timer_entries[i].pfn(i, timer_entries[i].param);
                        break;
                    }

                    timer_entries[i].pfn(i, timer_entries[i].param);
                }

                timer_entries[i].busy = 0;
            }
        }

        timer_inTick = 0;
        timer_reent  = timer_savedReent;
        if ( timer_x368 == 0 && timer_x364 == 0 )
        {
            timer_x368 = 1;
            timer_Poll();
            timer_x368 = 0;
        }
    }
}

void timer_Poll(void)
{
    if ( timer_x374 == 0 || --timer_x374 == 0 )
    {
        if ( setjmp(timer_jmpA) == 0 )
        {
            if ( timer_x36C )
            {
                // TODO: [BUG] timer_jmpB is never set with setjmp, so this path would jump through an empty buffer (timer_x36C is never set).
                timer_x36C = 0;
                timer_x370 = 1;
                longjmp(timer_jmpB, 1);
            }
            else
            {
                timer_x36C = 0;
                timer_x370 = 1;
            }

            timer_RunDeferred();
            timer_x370 = 0;
            timer_x0B8 = timer_x0BC;
            longjmp(timer_jmpA, 2);
        }

        timer_reent = 0;
    }
}

void timer_RunDeferred(void)
{
    for ( int i = 1; i < TIMER_COUNT + 1; i++ )
    {
        timer_entries[i].running = 1;
        if ( !timer_entries[i].pfn || (timer_entries[i].flags & 5) != 4 || timer_entries[i].interval == 0 )
        {
            timer_entries[i].running = 0;
            continue;
        }

        while ( timer_entries[i].elapsed >= timer_entries[i].interval )
        {
            timer_entries[i].elapsed -= timer_entries[i].interval;
            if ( timer_entries[i].flags & 2 )
            {
                timer_entries[i].interval = 0;
                timer_entries[i].pfn(i, timer_entries[i].param);
                break;
            }

            timer_entries[i].pfn(i, timer_entries[i].param);
        }

        timer_entries[i].running = 0;
    }
}

void Timer_InstallHooks(void)
{
    // Uncomment only lines for functions that have full definition and doesn't call original function (non-thunk functions)

    J3D_HOOKFUNC(timer_Register);
    J3D_HOOKFUNC(timer_Unregister);
    J3D_HOOKFUNC(timer_Startup);
    J3D_HOOKFUNC(timer_Shutdown);
    J3D_HOOKFUNC(timer_Start);
    J3D_HOOKFUNC(timer_Thread);
    J3D_HOOKFUNC(timer_Stop);
    J3D_HOOKFUNC(timer_Pause);
    J3D_HOOKFUNC(timer_Resume);
    J3D_HOOKFUNC(timer_Alloc);
    J3D_HOOKFUNC(timer_Free);
    J3D_HOOKFUNC(timer_SetInterval);
    J3D_HOOKFUNC(timer_Wait);
    J3D_HOOKFUNC(timer_GetTicks);
    J3D_HOOKFUNC(timer_GetTickLen);
    J3D_HOOKFUNC(timer_Sync);
    J3D_HOOKFUNC(timer_Tick);
    J3D_HOOKFUNC(timer_Poll);
    J3D_HOOKFUNC(timer_RunDeferred);
}

void Timer_ResetGlobals(void)
{
    timer_field54D84C = -1;
    timer_pServices   = NULL;
    timer_rate        = 0;
    memset(&timer_jmpA, 0, sizeof(timer_jmpA));
    memset(&timer_jmpB, 0, sizeof(timer_jmpB));
    timer_ticks = 0;
    timer_pShare = NULL;
    timer_x0B8 = 0;
    timer_x0BC = 0;
    memset(timer_entries, 0, sizeof(timer_entries));
    timer_hThread    = NULL;
    timer_pParam     = NULL;
    timer_tickLen    = 0;
    timer_hEvent     = NULL;
    timer_reent      = 0;
    timer_inTick     = 0;
    timer_savedReent = 0;
    timer_refs       = 0;
    timer_paused     = 0;
    timer_x364       = 0;
    timer_x368       = 0;
    timer_x36C       = 0;
    timer_x370       = 0;
    timer_x374       = 0;
}

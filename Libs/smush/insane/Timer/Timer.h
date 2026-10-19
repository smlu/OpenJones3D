#ifndef SMUSH_INSANE_TIMER_TIMER_H
#define SMUSH_INSANE_TIMER_TIMER_H
#include <assert.h>
#include <j3dcore/j3d.h>
#include <smush/insane/lec.h>

/*
 * The `TIMER=insane` service Timer.c publishes (`timer_interface`), subscribed by the animation player (Pcplay.c), the
 * DirectSound driver (Dsnd.c) and the sound engine (Engine.c, as `TIMER.external_sync`). The tick runs on a
 * TIME_CRITICAL thread (timer_Thread); `busy` is shared with it.
 */

J3D_EXTERN_C_START

#define TIMER_COUNT 19

typedef void (*TimerCallback)(int index, int param);

typedef struct sTimerInterface
{
    int  (*Alloc)(TimerCallback pfn, int interval, uint32_t flags, int param);
    void (*Free)(int index);
    void (*Pause)(void);
    void (*Resume)(void);
    void (*SetInterval)(int index, int interval);
    void (*Wait)(int ticks);
    int  (*GetTicks)(void);
    int  (*GetTickLen)(void);
    void (*Sync)(int ticks);
} TimerInterface;
static_assert(sizeof(TimerInterface) == 0x24, "sizeof(TimerInterface) == 0x24");

typedef struct sTimerEntry
{
    TimerCallback pfn;  /* 0x00 (NULL: the slot is free) */
    int interval;       /* 0x04 (ticks) */
    uint32_t flags;     /* 0x08 (bit 1: periodic, bit 2: one-shot, bit 3 (4): deferred, bit 4 (8): resync) */
    int param;          /* 0x0C */
    volatile int busy;  /* 0x10 (reentrancy flag shared by the timer thread and the main thread) */
    int elapsed;        /* 0x14 */
    int running;        /* 0x18 */
    int total;          /* 0x1C */
} TimerEntry;
static_assert(sizeof(TimerEntry) == 0x20, "sizeof(TimerEntry) == 0x20");

int J3DAPI timer_Register(LecShare* pShare, void* pParam);
void timer_Unregister(void);
int timer_Startup(void);
void timer_Shutdown(void);
int J3DAPI timer_Start(int a, int b);
DWORD WINAPI timer_Thread(LPVOID pArg);
void timer_Stop(void);
void timer_Pause(void);
void timer_Resume(void);
int J3DAPI timer_Alloc(TimerCallback pfn, int interval, uint32_t flags, int param);
void J3DAPI timer_Free(int index);
void J3DAPI timer_SetInterval(int index, int interval);
void J3DAPI timer_Wait(int ticks);
int timer_GetTicks(void);
int timer_GetTickLen(void);
void J3DAPI timer_Sync(int ticks);
void timer_Tick(void);
void timer_Poll(void);
void timer_RunDeferred(void);

void Timer_InstallHooks(void);
void Timer_ResetGlobals(void);

J3D_EXTERN_C_END

#endif // SMUSH_INSANE_TIMER_TIMER_H

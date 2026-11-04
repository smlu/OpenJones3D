#ifndef SMUSH_INSANE_TIMER_TIMERSYS_H
#define SMUSH_INSANE_TIMER_TIMERSYS_H

/*
 * Platform layer of the timer service (TimerWin32.c), private to the Timer module: the tick thread, its event handle
 * and the Windows calls that drive them stay behind these calls. No platform types cross this boundary.
 */

#ifdef __cplusplus
extern "C" {
#endif

// Starts the tick thread, which calls timer_Tick at TIMER_THREAD_RATE until timer_SysStopThread
void timer_SysStartThread(void);

// Signals the tick thread, waits for it to exit and releases its resources
void timer_SysStopThread(void);

// Clears the thread and event handles
void timer_SysResetGlobals(void);

#ifdef __cplusplus
}
#endif

#endif // SMUSH_INSANE_TIMER_TIMERSYS_H

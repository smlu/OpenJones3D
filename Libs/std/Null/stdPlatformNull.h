// Added: Null-platform-only control for the stdPlatform time services. A headless run needs the
// engine clock to follow a fixed schedule instead of wall time to be reproducible, so the virtual
// clock lives with the Null backend that implements the time services.
#ifndef STD_STDPLATFORMNULL_H
#define STD_STDPLATFORMNULL_H
#include <stdint.h>
#include <stdbool.h>
#include <std/types.h>

J3D_EXTERN_C_START

// Switches stdPlatform_GetTimeMsec to a virtual clock that starts at startMsec and only moves when
// stdPlatformNull_AdvanceClock or stdPlatform_Sleep is called. The Null stdDisplay_Update also
// advances it by 1 ms per present, so frame loops that wait on the clock around a present finish.
// Call before anything reads the time. Cannot be switched off.
void stdPlatformNull_UseVirtualClock(tStdTime startMsec);

// Moves the virtual clock forward by msec, wrapping at 2^32 like the real clock. No-op when the virtual clock is off.
void stdPlatformNull_AdvanceClock(uint32_t msec);

J3D_EXTERN_C_END
#endif // STD_STDPLATFORMNULL_H

// Added: the Babylon column's adapter context. The composition root (Jones3D/Platform/Babylon/main.cpp)
// owns every object it points to, fills it in and installs it with stdBabylon_SetContext before any
// engine code runs; adapter files read it through stdBabylon_GetContext.
#ifndef STD_STDBABYLON_H
#define STD_STDBABYLON_H
#ifndef __cplusplus
#error "stdBabylon.h is C++ only: it names babylon::time types and holds the adapter's one context pointer."
#endif

#include <babylon/files/FileSystem.h>
#include <babylon/time/Clock.h>
#include <babylon/time/NullClock.h>

struct StdBabylonContext
{
    babylon::time::Clock*     pClock;        // real clock; never null while installed
    babylon::time::NullClock* pVirtualClock; // the headless run's virtual clock; never null while installed
    babylon::files::FileSystem* pFileSystem; // never null while installed
};

// Installs the context (nullptr uninstalls). The pointer must stay valid until it is uninstalled.
void stdBabylon_SetContext(StdBabylonContext* pContext);

// The installed context, or nullptr.
StdBabylonContext* stdBabylon_GetContext();

// The installed context; with none, prints "ERROR: <pFunction>: no Babylon platform context
// installed!" to stderr and aborts: every caller needs the context and cannot recover.
StdBabylonContext& stdBabylon_RequireContext(const char* pFunction);

#ifdef J3D_TEST
// Test-only: clears the virtual-clock latch of stdPlatformTimeBabylon.cpp, which is a
// file-static that a test cannot otherwise reset. Defined only when J3D_TEST is set.
void stdPlatformTimeBabylon_ResetForTest();
#endif

#endif // STD_STDBABYLON_H

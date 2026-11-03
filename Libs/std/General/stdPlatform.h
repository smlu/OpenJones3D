#ifndef STD_STDPLATFORM_H
#define STD_STDPLATFORM_H
#include <j3dcore/j3d.h>
#include <std/types.h>
#include <std/RTI/addresses.h>

J3D_EXTERN_C_START

typedef void (J3DAPI* tStackTracePrintFunc)(const char*, ...);

// Local wall-clock time, broken into the fields the engine reads from the system clock.
typedef struct sStdLocalTime
{
    uint16_t year, month, day, hour, minute, second;
} StdLocalTime;

int J3DAPI stdPlatform_InitServices(tHostServices* pHS);
void J3DAPI stdPlatform_ClearServices(tHostServices* pHS);
tStdTime stdPlatform_GetTimeMsec(void);

// Suspends the calling thread for the given number of milliseconds.
void J3DAPI stdPlatform_Sleep(uint32_t msec);

// Fills the given struct with the current local date and time.
void J3DAPI stdPlatform_GetLocalTime(StdLocalTime* pTime);

J3DNORETURN void J3DAPI stdPlatform_Assert(const char* pErrorStr, const char* pFilename, int linenum);
int stdPlatform_Printf(const char* format, ...);

void stdPlatform_PrintStackTrace(tStackTracePrintFunc pfPrintFunc, size_t numFrames);

void* J3DAPI stdPlatform_AllocHandle(size_t size);
void J3DAPI stdPlatform_FreeHandle(void* pData);
void* J3DAPI stdPlatform_ReallocHandle(void* pMemory, size_t newSize);

int J3DAPI stdPlatform_LockHandle(int a1);
void J3DAPI stdPlatform_UnlockHandle();

bool J3DAPI stdPlatform_DirExists(const char* pPath);

// Installs signal handler for unhandled exceptions and signals
// On windows this will generate a minidump
void J3DAPI stdPlatform_InstallSignalHandler(void);

// Helper hooking functions
void stdPlatform_ResetGlobals(void);

J3D_EXTERN_C_END
#endif // STD_STDPLATFORM_H

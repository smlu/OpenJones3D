#ifndef STD_STDPLATFORMSYS_H
#define STD_STDPLATFORMSYS_H

#ifdef __cplusplus
extern "C" {
#endif

// Breaks into the platform debugger
void stdPlatformSys_DebugBreak(void);

// Shows the given text in the platform assert message box, with the original title and flags
void stdPlatformSys_ShowAssertMessage(const char* pText);

// Writes the given text to the platform debug output
void stdPlatformSys_DebugOutput(const char* pText);

// Starts COM on the calling thread, returns the underlying CoInitialize result value as int
int stdPlatformSys_CoInitialize(void);

// Releases COM on the calling thread, must balance a previous stdPlatformSys_CoInitialize
void stdPlatformSys_CoUninitialize(void);

#ifdef __cplusplus
}
#endif

#endif // STD_STDPLATFORMSYS_H

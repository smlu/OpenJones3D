#ifndef STD_STDCONSOLE_H
#define STD_STDCONSOLE_H
#include <j3dcore/j3d.h>
#include <std/types.h>
#include <std/RTI/addresses.h>

J3D_EXTERN_C_START

int J3DAPI stdConsole_Startup(const char* pTitleText, int attributes, int bShowMinimized);
void stdConsole_Shutdown(void);

// Altered: WORD means uint16_t so the shared header stays free of Windows types.
int J3DAPI stdConsole_SetAttributes(uint16_t wAttributes);
int J3DAPI stdConsole_SetConsoleTextAttribute(uint16_t wAttributes);
int stdConsole_InitOutputConsole(void);
int J3DAPI stdConsole_WriteConsole(const char* pText, uint32_t textAttribute);

// Helper hooking functions
void stdConsole_InstallHooks(void);
void stdConsole_ResetGlobals(void);

J3D_EXTERN_C_END
#endif // STD_STDCONSOLE_H

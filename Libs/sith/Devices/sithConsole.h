#ifndef SITH_SITHCONSOLE_H
#define SITH_SITHCONSOLE_H
#include <j3dcore/j3d.h>
#include <rdroid/types.h>
#include <sith/types.h>
#include <sith/Main/sithMain.h>
#include <sith/RTI/addresses.h>

#include <std/General/std.h>
#include <std/General/stdUtil.h>

#include <wchar.h>

J3D_EXTERN_C_START

typedef enum eSithConsoleFlags // Added
{
    SITHCONSOLE_DEVMODE = 0x100 // enables dev commands
} SithConsoleFlags;

#define SITHCONSOLE_PRINTF(format, ...) \
    do { \
        STD_FORMAT(std_g_genBuffer, format, __VA_ARGS__); \
        sithConsole_PrintString(std_g_genBuffer); \
    } while (0)

int sithConsole_Startup(void);
void sithConsole_Shutdown(void);

int J3DAPI sithConsole_Open(size_t numLines, size_t numCommands);
void sithConsole_Close(void);

// Added
SithConsoleFlags sithConsole_GetConsoleFlags(void);
void J3DAPI sithConsole_SetConsoleFlags(SithConsoleFlags flags);
void J3DAPI sithConsole_ClearConsoleFlags(SithConsoleFlags flags);

void J3DAPI sithConsole_PrintString(const char* pString);
void J3DAPI sithConsole_PrintWString(const wchar_t* pwString);
int J3DAPI sithConsole_ExeCommand(const char* pLine);
void sithConsole_Flush(void);

int J3DAPI sithConsole_RegisterCommand(SithConsoleFunction pfFunc, const char* pName, int flags);
void J3DAPI sithConsole_RegisterPrintFunctions(SithConsoleWriteTextFunc pfWriteText, SithConsoleWriteWideTextFunc pfWriteWText, SithConsoleFlushFunc pfFlush);

int J3DAPI sithConsole_Help(const SithConsoleCommand* pFunc, const char* pArg); // Added

// Helper hooking functions
void sithConsole_ResetGlobals(void);

J3D_EXTERN_C_END
#endif // SITH_SITHCONSOLE_H

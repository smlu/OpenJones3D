#ifndef STD_STDCONFFILE_H
#define STD_STDCONFFILE_H
#include <j3dcore/j3d.h>
#include <j3dcore/j3dhook.h>

#include <std/types.h>
#include <std/RTI/addresses.h>

#include <stdio.h>

J3D_EXTERN_C_START

#define STDCONFFILE_LINESIZE 4096u
#define STDCONFILE_MAXARGS   512u

typedef struct sStdConffileArg
{
    const char* argName;
    const char* argValue;
} StdConffileArg;
static_assert(sizeof(StdConffileArg) == 8, "sizeof(StdConffileArg) == 8");

typedef struct sStdConffileEntry
{
    size_t numArgs;
    StdConffileArg aArgs[STDCONFILE_MAXARGS];
} StdConffileEntry;
static_assert(sizeof(StdConffileEntry) == 4100, "sizeof(StdConffileEntry) == 4100");

#ifdef J3D_LOCAL_RTI
extern StdConffileEntry stdConffile_g_entry;
extern char* stdConffile_g_aLine;
#else
#   define stdConffile_g_entry J3D_DECL_FAR_VAR(stdConffile_g_entry, StdConffileEntry)
#   define stdConffile_g_aLine J3D_DECL_FAR_VAR(stdConffile_g_aLine, char*)
#endif

int J3DAPI stdConffile_Open(const char* pFilename);
int J3DAPI stdConffile_OpenWrite(const char* pFilename);
int J3DAPI stdConffile_OpenMode(const char* pFilename, const char* openMode);

void stdConffile_Close(void);
void stdConffile_CloseWrite(void);

int J3DAPI stdConffile_WriteLine(const char* pLine); // Added
int J3DAPI stdConffile_Write(const void* pData, size_t size);
int stdConffile_Printf(const char* pFormat, ...); // Added

int J3DAPI stdConffile_Read(void* pData, size_t size);
int J3DAPI stdConffile_ReadArgsFromStr(char* pStr);
int stdConffile_ReadArgs(void);
int stdConffile_ReadLine(void);
int stdConffile_ScanLine(const char* pFormat, ...); // Added

tFileHandle stdConffile_GetFileHandle(void);
tFileHandle stdConffile_GetWriteFileHandle(void); // Added
size_t stdConffile_GetLineNumber(void);

const char* stdConffile_GetFilename(void);
const char* stdConffile_GetWriteFilename(void); // Added

// Helper hooking functions
void stdConffile_InstallHooks(void);
void stdConffile_ResetGlobals(void);

J3D_EXTERN_C_END
#endif // STD_STDCONFFILE_H

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


#define stdConffile_g_entry J3D_DECL_FAR_VAR(stdConffile_g_entry, StdConffileEntry)
// extern StdConffileEntry stdConffile_g_entry;

#define stdConffile_g_aLine J3D_DECL_FAR_VAR(stdConffile_g_aLine, char*)
// extern char *stdConffile_g_aLine;

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

/**
 * @brief Reads a line and parses data based on the provided format.
 *
 * This function attempts to scan a line from a string and extract data according to the format specified.
 * If an error occurs while reading the line, it returns -1; otherwise, it returns the number of successfully
 * read arguments.
 *
 * @param pFormat Format string specifying how to parse the input data.
 * @param ... Additional arguments specifying where to store parsed data.
 *
 * @return -1 if a read error occurs, or the number of arguments successfully parsed.
 *
 * @note For the `%c`, `%s`, and `%[` conversion specifiers, two arguments are expected:
 *       a pointer to the data and the size of the receiving buffer (as an `rsize_t` type),
 *       where the size may be 1 when using `%c` to read a single character.
 */
inline int stdConffile_ScanLine(const char* pFormat, ...)
{
    if ( !stdConffile_ReadLine() )
    {
        return -1;
    }

    va_list args;
    va_start(args, pFormat);
    int result = vsscanf_s(stdConffile_g_aLine, pFormat, args);
    va_end(args);
    return result;
}

J3D_EXTERN_C_END
#endif // STD_STDCONFFILE_H

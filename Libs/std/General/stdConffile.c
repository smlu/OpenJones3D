#include "std.h"
#include "stdConffile.h"
#include "stdMemory.h"
#include "stdUtil.h"

#include <j3dcore/j3dhook.h>
#include <std/RTI/symbols.h>

#define STDCONFFILE_STACKSIZE 20
#define STDCONFFILE_FILENAMESIZE 128

static bool stdConffile_bOpen = false;

static tFileHandle openFile  = 0;
static tFileHandle writeFile = 0;

static size_t stackLevel = 0;
static char* apBufferStack[STDCONFFILE_STACKSIZE]          = { 0 };
static tFileHandle openFileStack[STDCONFFILE_STACKSIZE]    = { 0 };
static StdConffileEntry aEntryStack[STDCONFFILE_STACKSIZE] = { 0 };
static char aFilenameStack[STDCONFFILE_STACKSIZE][STDCONFFILE_FILENAMESIZE] = { 0 };

static char stdConffile_pFilename[STDCONFFILE_FILENAMESIZE]      = { 0 };
static char stdConffile_aWriteFilename[STDCONFFILE_FILENAMESIZE] = { 0 };

static size_t stdConffile_linenum = 0;
static size_t linenumStack[STDCONFFILE_STACKSIZE] = { 0 };

static char printBuffer[4096] = { 0 };

void stdConffile_PushStack(void);
void stdConffile_PopStack(void);

void stdConffile_InstallHooks(void)
{
    J3D_HOOKFUNC(stdConffile_Open);
    J3D_HOOKFUNC(stdConffile_OpenWrite);
    J3D_HOOKFUNC(stdConffile_OpenMode);
    J3D_HOOKFUNC(stdConffile_Close);
    J3D_HOOKFUNC(stdConffile_CloseWrite);
    J3D_HOOKFUNC(stdConffile_Write);
    J3D_HOOKFUNC(stdConffile_Read);
    J3D_HOOKFUNC(stdConffile_ReadArgsFromStr);
    J3D_HOOKFUNC(stdConffile_ReadArgs);
    J3D_HOOKFUNC(stdConffile_ReadLine);
    J3D_HOOKFUNC(stdConffile_GetFileHandle);
    J3D_HOOKFUNC(stdConffile_GetLineNumber);
    J3D_HOOKFUNC(stdConffile_GetFilename);
    J3D_HOOKFUNC(stdConffile_PushStack);
    J3D_HOOKFUNC(stdConffile_PopStack);
}

void stdConffile_ResetGlobals(void)
{
    STD_ZEROMEM(&stdConffile_g_entry, sizeof(stdConffile_g_entry));
    STD_ZEROMEM(&stdConffile_g_aLine, sizeof(stdConffile_g_aLine));
}

int J3DAPI stdConffile_Open(const char* pFilename)
{
    return stdConffile_OpenMode(pFilename, "r");
}

int J3DAPI stdConffile_OpenWrite(const char* pFilename)
{
    if ( writeFile )
    {
        return 0;
    }

    writeFile = std_g_pHS->pFileOpen(pFilename, "wb");
    if ( writeFile )
    {
        STD_STRCPY(stdConffile_aWriteFilename, pFilename);
        return 1;
    }
    else
    {
        writeFile = 0;
        return 0;
    }
}

int J3DAPI stdConffile_OpenMode(const char* pFilename, const char* openMode)
{
    // Added: Release-build guard for invalid open requests.
    STD_GUARD(pFilename && openMode, 0);

    bool bWasOpen = stdConffile_bOpen;
    if ( bWasOpen )
    {
        // Fixed: Prevent include stack overflow before pushing the current file state.
        if ( stackLevel >= STDCONFFILE_STACKSIZE )
        {
            STDLOG_ERROR("Conffile include stack overflow while opening '%s'.\n", pFilename);
            return 0;
        }

        stdConffile_PushStack();
    }

    STD_ASSERTREL(openFile == 0);
    if ( streq(pFilename, "none") )
    {
        openFile = 0;
    }
    else
    {
        openFile = std_g_pHS->pFileOpen(pFilename, openMode);
        if ( !openFile )
        {
        #ifdef J3D_DEBUG
            STDLOG_DEBUG("Conffile '%s' could not be opened (handle=%d)\n", pFilename, openFile);
        #endif

            openFile = 0;
            if ( bWasOpen )
            {
                stdConffile_PopStack();
            }

            return 0;
        }
    }

    stdConffile_g_aLine = (char*)STDMALLOC(STDCONFFILE_LINESIZE);

    // Fixed: Handle line-buffer allocation failure and restore the previous file state.
    if ( !stdConffile_g_aLine )
    {
        if ( openFile )
        {
            std_g_pHS->pFileClose(openFile);
            openFile = 0;
        }

        if ( bWasOpen )
        {
            stdConffile_PopStack();
        }

        STDLOG_ERROR("Conffile '%s' could not allocate line buffer.\n", pFilename);
        return 0;
    }

    STD_STRCPY(stdConffile_pFilename, pFilename);

    stdConffile_linenum = 0;
    stdConffile_bOpen   = true;
    return 1;
}

void stdConffile_Close(void)
{
    if ( !stdConffile_bOpen )
    {
        STDLOG_ERROR("Warning: System already closed!\n");
        return;
    }

    STD_ASSERTREL(stdConffile_g_aLine != NULL);
    if ( openFile )
    {
        std_g_pHS->pFileClose(openFile);
    }

    openFile = 0;
    stdMemory_Free(stdConffile_g_aLine);

    if ( stackLevel )
    {
        stdConffile_PopStack();
    }
    else
    {
        stdConffile_bOpen = false;
    }
}

void stdConffile_CloseWrite(void)
{
    if ( writeFile )
    {
        std_g_pHS->pFileClose(writeFile);
        writeFile = 0;
        STD_STRCPY(stdConffile_aWriteFilename, "NOT_OPEN");
    }
}

int J3DAPI stdConffile_WriteLine(const char* pLine)
{
    if ( !writeFile || !pLine )
    {
        return 1;
    }

    size_t len = strlen(pLine);
    return len != std_g_pHS->pFileWrite(writeFile, pLine, len);
}

int J3DAPI stdConffile_Write(const void* pData, size_t size)
{
    return !writeFile || !pData || size != std_g_pHS->pFileWrite(writeFile, pData, size);
}

int stdConffile_Printf(const char* pFormat, ...)
{
    if ( !writeFile || !pFormat )
    {
        return 1;
    }

    va_list args;
    va_start(args, pFormat);
    int nWrite = vsnprintf(printBuffer, sizeof(printBuffer), pFormat, args);
    va_end(args);

    // Fixed: vsnprintf returns a negative int on formatting errors.
    if ( nWrite < 0 )
    {
        return 1;
    }

    size_t size = (size_t)nWrite;
    // Fixed: Do not pass a formatted size larger than the fixed print buffer to pFileWrite.
    if ( size >= sizeof(printBuffer) )
    {
        STDLOG_WARNING(
            "stdConffile_Printf: Truncated formatted output for '%s' from %zu to %zu bytes.\n",
            stdConffile_aWriteFilename,
            size,
            sizeof(printBuffer) - 1u
        );
        size = sizeof(printBuffer) - 1;
    }

    return std_g_pHS->pFileWrite(writeFile, printBuffer, size) != size;
}

int J3DAPI stdConffile_Read(void* pData, size_t size)
{
    STD_ASSERTREL(pData && (size > 0));
    return stdConffile_bOpen && openFile && std_g_pHS->pFileRead(openFile, pData, size) == size;
}

int J3DAPI stdConffile_ReadArgsFromStr(char* pStr)
{
    size_t argNum = 0;
    stdConffile_g_entry.numArgs = 0;

    char* pntok = NULL;
    for ( char* ptok = strtok_r(pStr, ", \t\n\r", &pntok); ptok; ptok = strtok_r(NULL, ", \t\n\r", &pntok) )
    {
        if ( argNum >= STDCONFILE_MAXARGS ) // Fixed; original: argNum > STDCONFILE_MAXARGS
        {
            STDLOG_ERROR("conffile  %s line %d too many args.\n", stdConffile_pFilename, stdConffile_linenum);
            return 1;
        }

        char* pqstr = strchr(ptok, '"');
        if ( pqstr )
        {
            stdConffile_g_entry.aArgs[argNum].argName  = pqstr + 1;
            stdConffile_g_entry.aArgs[argNum].argValue = pqstr + 1;
            if ( ptok[strlen(ptok) - 1] == '"' )
            {
                ptok[strlen(ptok) - 1] = '\0';
            }
            else
            {
                ptok[strlen(ptok)] = ' ';
                strtok_r(NULL, ",\t\n\r\"", &pntok);
            }
        }
        else
        {
            char* pval = strchr(ptok, '=');
            if ( pval )
            {
                *pval = '\0';
                stdConffile_g_entry.aArgs[argNum].argName = ptok;
                stdConffile_g_entry.aArgs[argNum].argValue = pval + 1;
            }
            else
            {
                stdConffile_g_entry.aArgs[argNum].argName = ptok;
                stdConffile_g_entry.aArgs[argNum].argValue = ptok;
            }
        }

        ++argNum;
    }

    stdConffile_g_entry.numArgs = argNum;
    return 0;
}

int stdConffile_ReadArgs(void)
{
    STD_ASSERTREL((stdConffile_bOpen) && (openFile != 0));
    do
    {
        if ( !stdConffile_ReadLine() )
        {
            return 0;
        }

        if ( stdConffile_ReadArgsFromStr(stdConffile_g_aLine) )
        {
            return 0;
        }
    } while ( !stdConffile_g_entry.numArgs );

    return 1;
}

int stdConffile_ReadLine(void)
{
    int bEnd = 0;
    char* pLine  = stdConffile_g_aLine;
    size_t nRead = STDCONFFILE_LINESIZE - 1;

    while ( !bEnd && nRead )
    {
        if ( !std_g_pHS->pFileGets(openFile, pLine, nRead) )
        {
            return 0;
        }

        ++stdConffile_linenum;
        if ( *pLine != ';' && *pLine != '#' && *pLine != '\n' && *pLine != '\r' )
        {
            char* pCommentLine = strchr(pLine, '#');
            if ( pCommentLine )
            {
                *pCommentLine = '\0';
            }

            // Convert line to lower case
            stdUtil_ToLower(pLine);

            size_t len = strlen(stdConffile_g_aLine);
            // Fixed: Short non-comment lines can be empty after comment stripping; check length before len - 2/len - 1.
            if ( len >= 2 && stdConffile_g_aLine[len - 2] == '\\' )
            {
                nRead = STDCONFFILE_LINESIZE - len;
                if ( len == STDCONFFILE_LINESIZE )
                {
                    STDLOG_ERROR("conffile line %d too long.  Truncated.\n", stdConffile_linenum);
                }

                pLine = &stdConffile_g_aLine[len - 2];
            }
            else
            {
                bEnd = 1;
                if ( len && (stdConffile_g_aLine[len - 1] == '\r' || stdConffile_g_aLine[len - 1] == '\n') )
                {
                    stdConffile_g_aLine[len - 1] = '\0';
                }
            }
        }
    }

    return 1;
}

tFileHandle stdConffile_GetFileHandle(void)
{
    STD_ASSERTREL(openFile != 0);
    return openFile;
}

tFileHandle stdConffile_GetWriteFileHandle(void)
{
    STD_ASSERTREL(writeFile != 0);
    return writeFile;
}

size_t stdConffile_GetLineNumber(void)
{
    return stdConffile_linenum;
}

const char* stdConffile_GetFilename(void)
{
    return stdConffile_pFilename;
}

const char* stdConffile_GetWriteFilename(void)
{
    return stdConffile_aWriteFilename;
}

// TODO: refactor to return error code instead of void
void stdConffile_PushStack(void)
{
    STD_ASSERTREL(stackLevel < STDCONFFILE_STACKSIZE);

    // Added: Guard against stack overflow before pushing the current file state.
    STD_GUARDEX(stackLevel < STDCONFFILE_STACKSIZE, {
         STDLOG_ERROR(
            "stdConffile_PushStack: cannot save current file '%s' at line %zu; include depth %zu reached max stack size %d.\n",
            stdConffile_pFilename,
            stdConffile_linenum,
            stackLevel,
            STDCONFFILE_STACKSIZE
        );
        return;
        });

    STD_ASSERTREL(strlen(stdConffile_pFilename) < STDCONFFILE_FILENAMESIZE); // TODO: What's up with this check? The len should be guarantied when copying to it.
    stdUtil_StringCopy(aFilenameStack[stackLevel], sizeof(stdConffile_pFilename), stdConffile_pFilename);

    linenumStack[stackLevel] = stdConffile_linenum;
    stdConffile_linenum = 0;

    openFileStack[stackLevel] = openFile;
    openFile = 0;

    apBufferStack[stackLevel] = stdConffile_g_aLine;

    STD_COPYMEM(&aEntryStack[stackLevel], &stdConffile_g_entry, sizeof(StdConffileEntry));
    ++stackLevel;
}

void stdConffile_PopStack(void)
{
    if ( stackLevel )
    {
        STD_STRCPY(stdConffile_pFilename, aFilenameStack[--stackLevel]);

        stdConffile_linenum = linenumStack[stackLevel];
        openFile = openFileStack[stackLevel];

        stdConffile_g_aLine = apBufferStack[stackLevel];
        STD_COPYMEM(&stdConffile_g_entry, &aEntryStack[stackLevel], sizeof(stdConffile_g_entry));
    }
}
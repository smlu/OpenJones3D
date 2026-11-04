#include "sithConsole.h"

#include <sith/Main/sithCommand.h>

#include <std/General/stdHashtbl.h>
#include <std/General/stdMemory.h>
#include <std/General/stdUtil.h>

#include <stdio.h>
#include <string.h>
#include <wchar.h>

#define SITHCONSOLE_MAXLINES 32
#define SITHCONSOLE_LINELEN  128

static bool bOpen;
static bool bStarted;

static SithConsoleFlags consoleflags = 0x0; // Added

static char aBuffers[SITHCONSOLE_MAXLINES][SITHCONSOLE_LINELEN];
static size_t bufferSize;
static size_t curFlushIndex;
static size_t endPrintIndex;
static size_t curPrintIndex;

static tHashTable* pCommandTable;
static SithConsoleCommand* apCommands;
static size_t maxRegisteredCommands;
static size_t numRegistredCommands;

static SithConsoleWriteTextFunc pfPrintString;
static SithConsoleWriteWideTextFunc pfPrintWString;
static SithConsoleFlushFunc pfFlushFunc;

int J3DAPI sithConsole_CreateCommandTable(size_t size);
void sithConsole_FreeCommandTable(void);

void sithConsole_ResetGlobals(void)
{}

int sithConsole_Startup(void)
{
    SITH_ASSERTREL(!bStarted);

    bufferSize            = 0;
    maxRegisteredCommands = 0;
    numRegistredCommands  = 0;

    pfPrintString  = NULL;
    pfPrintWString = NULL;
    pfFlushFunc    = NULL;

    bStarted = true;
    return 0;
}

void sithConsole_Shutdown(void)
{
    bufferSize            = 0;
    maxRegisteredCommands = 0;
    numRegistredCommands  = 0;

    pfPrintString  = NULL;
    pfPrintWString = NULL;
    pfFlushFunc    = NULL;

    if ( bOpen ) {
        sithConsole_Close();
    }

    bStarted = false;
}

int J3DAPI sithConsole_Open(size_t numLines, size_t numCommands)
{
    if ( !bStarted ) {
        return 1;
    }

    if ( bOpen ) {
        return 1;
    }

    // Added: bound check
    if ( numLines > SITHCONSOLE_MAXLINES )
    {
        SITHLOG_ERROR("Failed to create console. Requested too big print buffer size! max=%d\n", SITHCONSOLE_MAXLINES);
        return 1;
    }

    memset(aBuffers, 0, numLines * SITHCONSOLE_LINELEN);
    bufferSize = numLines;

    endPrintIndex = 0;
    curPrintIndex = 0;
    curFlushIndex = 0;

    if ( !sithConsole_CreateCommandTable(numCommands) )
    {
        SITHLOG_ERROR("Failed to create console.\n");
        return 1;
    }

    sithCommand_RegisterCommands();

    bOpen = true;
    return 0;
}

void sithConsole_Close(void)
{
    if ( !bOpen ) {
        SITHLOG_STATUS("sithConsole closed when not opened.\n");
    }

    bufferSize            = 0;
    maxRegisteredCommands = 0;
    numRegistredCommands  = 0;

    sithConsole_FreeCommandTable();
    bOpen = false;
}

SithConsoleFlags sithConsole_GetConsoleFlags(void)
{
    return consoleflags;
}

void J3DAPI sithConsole_SetConsoleFlags(SithConsoleFlags flags)
{
    consoleflags |= flags;
}

void J3DAPI sithConsole_ClearConsoleFlags(SithConsoleFlags flags)
{
    consoleflags &= ~flags;
}

void J3DAPI sithConsole_PrintString(const char* pString)
{
    SITH_ASSERTREL(pString);

    if ( bOpen )
    {
        if ( pfPrintString )
        {
            pfPrintString(pString);
        }
        else
        {
            const size_t len = strlen(pString);
            if ( len > SITHCONSOLE_LINELEN - 1 )
            {
                SITHLOG_ERROR("Console string too long (%d).  Truncated.\n", len);
            }

            curPrintIndex = (curPrintIndex + 1) % bufferSize;
            if ( curPrintIndex == endPrintIndex )
            {
                endPrintIndex = (endPrintIndex + 1) % bufferSize;
            }

            STD_STRCPY(aBuffers[curPrintIndex], pString);
        }
    }
}

void J3DAPI sithConsole_PrintWString(const wchar_t* pwString)
{
    SITH_ASSERTREL(pwString && bOpen);
    if ( pfPrintWString )
    {
        pfPrintWString(pwString);
    }
}

int J3DAPI sithConsole_ExeCommand(const char* pLine)
{
    SITH_ASSERTREL(bOpen);
    SITH_ASSERTREL(pLine);

    // Fixed: Copy line to buffer before convert to lower str
    char aLine[128];
    STD_STRCPY(aLine, pLine);
    stdUtil_ToLower(aLine);// Note, was calling _strlwr((char*)pLine);

    const char* pCmdName = strtok(aLine, ", \t\n\r");
    if ( !pCmdName ) {
        return 0;
    }

    SithConsoleCommand* pFunc = (SithConsoleCommand*)stdHashtbl_Find(pCommandTable, pCmdName);
    if ( !pFunc || (pFunc->flags & SITHCONSOLE_DEVMODE) != 0 && (consoleflags & SITHCONSOLE_DEVMODE) == 0 )
    {
        SITHCONSOLE_PRINTF("Console command %s not recognized.", pCmdName);
        return 0;
    }

    SITH_ASSERTREL(pFunc->pfFunc);
    const char* pArg = strtok(NULL, "\n\r");
    pFunc->pfFunc(pFunc, pArg);
    return 1;
}

void sithConsole_Flush(void)
{
    if ( pfFlushFunc )
    {
        pfFlushFunc();
    }
    else
    {
        while ( curFlushIndex != curPrintIndex )
        {
            curFlushIndex = (curFlushIndex + 1) % bufferSize;
            SITHLOG_STATUS("%s", aBuffers[curFlushIndex]);
        }
    }
}

int J3DAPI sithConsole_RegisterCommand(SithConsoleFunction pfFunc, const char* pName, int flags)
{
    SithConsoleCommand* pCommand;

    SITH_ASSERTREL(pfFunc && pName);
    if ( numRegistredCommands == maxRegisteredCommands )
    {
        SITHLOG_ERROR("Tried to register > %d console funcs.\n", maxRegisteredCommands);
        return 0;
    }

    pCommand = (SithConsoleCommand*)stdHashtbl_Find(pCommandTable, pName);
    if ( pCommand )
    {
        // TODO: Add log for func getting overridden
        pCommand->pfFunc = pfFunc;
        pCommand->flags  = flags;
    }
    else
    {
        STD_STRCPY(apCommands[numRegistredCommands].aName, pName);

        apCommands[numRegistredCommands].pfFunc = pfFunc;
        apCommands[numRegistredCommands].flags  = flags;

        stdHashtbl_Add(
            pCommandTable,
            apCommands[numRegistredCommands].aName,
            &apCommands[numRegistredCommands]
        );
        ++numRegistredCommands;
    }

    return 1;
}

void J3DAPI sithConsole_RegisterPrintFunctions(SithConsoleWriteTextFunc pfWriteText, SithConsoleWriteWideTextFunc pfWriteWText, SithConsoleFlushFunc pfFlush)
{
    pfPrintString  = pfWriteText;
    pfPrintWString = pfWriteWText;
    pfFlushFunc    = pfFlush;
}

int J3DAPI sithConsole_Help(const SithConsoleCommand* pFunc, const char* pArg)
{
    J3D_UNUSED(pFunc);
    J3D_UNUSED(pArg);

    char aLine[56] = { 0 }; // Added: Init to 0

    // Removed
    //*(uint16_t*)aLine = word_5FA420; // ???
    //memset(&aLine[2], 0, 54);

    size_t curPos = 0;
    sithConsole_PrintString("The following commands are available:");
    for ( size_t i = 0; i < numRegistredCommands; ++i )
    {
        const SithConsoleCommand* pCommand = &apCommands[i];
        if ( (pCommand->flags & SITHCONSOLE_DEVMODE) == 0 || (consoleflags & SITHCONSOLE_DEVMODE) != 0 ) // Added
        {
            if ( curPos + strlen(pCommand->aName) + 5 >= 56 )
            {
                sithConsole_PrintString(aLine);
                curPos = 0;
            }

            curPos += sprintf_s(&aLine[curPos], STD_ARRAYLEN(aLine) - curPos, "    %s", pCommand->aName);
        }
    }

    sithConsole_PrintString(aLine);
    return 1;
}

int J3DAPI sithConsole_CreateCommandTable(size_t size)
{
    SITH_ASSERTREL(size > 0);
    apCommands = (SithConsoleCommand*)STDMALLOC(sizeof(SithConsoleCommand) * size);
    pCommandTable = stdHashtbl_New(2 * size);

    if ( !apCommands || !pCommandTable )
    {
        if ( apCommands )
        {
            stdMemory_Free(apCommands);
            apCommands = NULL;
        }

        if ( !pCommandTable )
        {
            return 0;
        }

        stdHashtbl_Free(pCommandTable);
        pCommandTable = NULL;
        return 0;
    }
    else
    {
        memset(apCommands, 0, sizeof(SithConsoleCommand) * size);
        maxRegisteredCommands = size;
        return 1;
    }
}

void sithConsole_FreeCommandTable(void)
{
    if ( apCommands )
    {
        stdMemory_Free(apCommands);
        apCommands = NULL;
    }

    if ( pCommandTable )
    {
        stdHashtbl_Free(pCommandTable);
        pCommandTable = NULL;
    }
}

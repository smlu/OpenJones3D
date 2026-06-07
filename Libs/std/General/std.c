#include <stdio.h>

#include "std.h"
#include "stdMemory.h"
#include "stdJSON.h"
#include "stdUtil.h"

#include <j3dcore/j3dhook.h>
#include <std/RTI/symbols.h>
#include <std/Win95/stdConsole.h>

static char aPrintBuffer[2048]     = { 0 };
static char aFilePrintBuffer[2048] = { 0 };
static bool bStdStartup = false;

#define word_183962C J3D_DECL_FAR_VAR(word_183962C, int16_t)
#define word_1839630 J3D_DECL_FAR_VAR(word_1839630, int16_t)
#define word_183962E J3D_DECL_FAR_VAR(word_183962E, int16_t)

void std_InstallHooks(void)
{
    J3D_HOOKFUNC(stdStartup);
    J3D_HOOKFUNC(stdShutdown);
    J3D_HOOKFUNC(stdPrintf);
    J3D_HOOKFUNC(stdConsolePrintf);
    J3D_HOOKFUNC(stdFileFromPath);
    J3D_HOOKFUNC(stdCalcBitPos);
    J3D_HOOKFUNC(stdFileOpen);
    J3D_HOOKFUNC(stdFileClose);
    J3D_HOOKFUNC(stdFileRead);
    J3D_HOOKFUNC(stdFileWrite);
    J3D_HOOKFUNC(stdFileGets);
    J3D_HOOKFUNC(stdFileGetws);
    J3D_HOOKFUNC(stdFileEof);
    J3D_HOOKFUNC(stdFileTell);
    J3D_HOOKFUNC(stdFileSize);
    J3D_HOOKFUNC(stdFilePrintf);
    J3D_HOOKFUNC(stdFileSeek);

    // Hook on stdio_open, stdio_close & stdio_flush due to stdio_services using stdFile* functions which might have conflicting FILE* object
    // Note on debug version this should be skipped
    J3DHookFunction(stdio_open_ADDR, (void*)stdFileOpen);
    J3DHookFunction(stdio_close_ADDR, (void*)stdFileClose);
    J3DHookFunction(stdio_flush_ADDR, (void*)stdFileFlush);
}

void std_ResetGlobals(void)
{
    memset((char*)&std_g_aEmptyString, 0, sizeof(std_g_aEmptyString));
    //memset(&aPrintBuffer, 0, sizeof(aPrintBuffer));
    //memset(&bStdStartup, 0, sizeof(bStdStartup));
    memset(&std_g_genBuffer, 0, sizeof(std_g_genBuffer));
    memset(&std_g_pHS, 0, sizeof(std_g_pHS));
}

void J3DAPI stdStartup(tHostServices* pHS)
{
    int16_t v1;

    std_g_pHS = pHS; // TODO: Should probably be moved within if scope

    if ( !bStdStartup )
    {
        // TODO: Should probably do success check
        stdMemory_Startup();
        stdMemory_Open();
        stdJSON_Startup(); // Added in 1.4

        // TODO: Remove when all round and rand float functions are replaced && updated
        // TODO: consider setting FE_DOWNWARD for round functions (lrintf); i.e.: fesetround(FE_DOWNWARD) to mimic same behavior as OG
        short  tmp = 0;
        __asm  fnstcw word ptr[tmp];
        word_183962C = tmp;

        // The following flags control in binary (Indy3D.exe) FPU rounding control flags for frndint (floorf, ceilf) and fistp (lrintf)
        // and should not have any effect in the reimplementation since these vars aren't used.
        v1 = (0xB00 | word_183962C & 0xFF); // round up CF for in binary ceilf function
        word_1839630 = v1;
        v1 = (0x700 | word_183962C & 0xFF); // Round down CF for in binary floorf & lrintf
        word_183962E = v1;
        bStdStartup = true;
    }
}

void stdShutdown(void)
{
    if ( bStdStartup )
    {
        stdJSON_Shutdown();
        stdMemory_Close();
        stdMemory_Shutdown();
        bStdStartup = false;
    }
}

void J3DAPI stdPrintf(tPrintfFunc pfPrint, const char* pFilePath, unsigned int linenum, const char* format, ...)
{
    if ( pfPrint )
    {
        va_list args;
        va_start(args, format);

        // Fixed: Build the prefix with a literal format instead of using the file path as a format string.
        const char* pFilename = pFilePath ? stdFileFromPath(pFilePath) : "";
        int prefixSize = _snprintf_s(aPrintBuffer, sizeof(aPrintBuffer), sizeof(aPrintBuffer) - 1, "%s(%u): ", pFilename, linenum);
        if ( prefixSize < 0 )
        {
            prefixSize = (int)strnlen_s(aPrintBuffer, sizeof(aPrintBuffer));
        }

        size_t offset = (size_t)prefixSize;
        if ( offset >= sizeof(aPrintBuffer) )
        {
            offset = sizeof(aPrintBuffer) - 1;
            aPrintBuffer[offset] = '\0';
        }

        // Added: Keep release builds from formatting a NULL message or writing past the buffer.
        if ( format && offset < sizeof(aPrintBuffer) - 1 )
        {
            int msgSize = vsnprintf_s(&aPrintBuffer[offset], sizeof(aPrintBuffer) - offset, sizeof(aPrintBuffer) - offset - 1, format, args);
            if ( msgSize < 0 )
            {
                aPrintBuffer[offset] = '\0';
            }
            aPrintBuffer[sizeof(aPrintBuffer) - 1] = '\0';
        }
        va_end(args);

        // Write to output function
        pfPrint(aPrintBuffer);
    }
}

int stdConsolePrintf(const char* pFormat, ...)
{
    va_list args;
    va_start(args, pFormat);
    vsnprintf_s(std_g_genBuffer, STD_ARRAYLEN(std_g_genBuffer), STD_ARRAYLEN(std_g_genBuffer) - 1, pFormat, args);
    va_end(args);

    stdConsole_WriteConsole(std_g_genBuffer, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
    return sizeof(std_g_genBuffer);
}

const char* J3DAPI stdFileFromPath(const char* pPath)
{
    char* pFilename = strrchr(pPath, '\\');
    if ( pFilename )
    {
        return pFilename + 1;
    }

    return pPath;
}

int J3DAPI stdCalcBitPos(int bit)
{
    int pos = 0;
    while ( bit > 1 )
    {
        bit >>= 1;
        pos++;
    }

    return pos;
}

// TODO: Functions below might all needed to be move to stdPlatform

tFileHandle J3DAPI stdFileOpen(const char* pFilename, const char* mode)
{
    FILE* fh;
    if ( fopen_s(&fh, pFilename, mode) != 0 )
    {
        fh = NULL; // Make sure it's null
    }
    return (tFileHandle)fh;
}

int J3DAPI stdFileClose(tFileHandle fh)
{
    fclose((FILE*)fh);
    return 0;
}

size_t J3DAPI stdFileRead(tFileHandle fh, void* pOutData, size_t size)
{
    return fread(pOutData, 1u, size, (FILE*)fh);
}

size_t J3DAPI stdFileWrite(tFileHandle fh, const void* pData, size_t size)
{
    return fwrite(pData, 1u, size, (FILE*)fh);
}

int J3DAPI stdFileFlush(tFileHandle fh)
{
    return fflush((FILE*)fh);
}

char* J3DAPI stdFileGets(tFileHandle fh, char* pStr, size_t size)
{
    return fgets(pStr, size, (FILE*)fh);
}

wchar_t* J3DAPI stdFileGetws(tFileHandle fh, wchar_t* pOutStr, size_t size)
{
    return fgetws(pOutStr, size, (FILE*)fh);
}

int J3DAPI stdFileEof(tFileHandle fh)
{
    return feof((FILE*)fh);
}

int J3DAPI stdFileTell(tFileHandle fh)
{
    return ftell((FILE*)fh);
}

size_t J3DAPI stdFileSize(const char* pFilePath)
{
    tFileHandle fh = stdFileOpen(pFilePath, "rb");
    if ( !fh )
    {
        return 0;
    }

    stdFileSeek(fh, 0, 2);
    int size = stdFileTell(fh);
    stdFileClose(fh);
    return size;
}

int stdFilePrintf(tFileHandle fh, const char* pFormat, ...)
{
    STD_GUARD(fh && pFormat, 1);

    va_list args;
    va_start(args, pFormat);
    int size = vsnprintf_s(aFilePrintBuffer, sizeof(aFilePrintBuffer), sizeof(aFilePrintBuffer) - 1, pFormat, args);
    va_end(args);

    // Fixed: vsnprintf_s returns negative on formatting errors/truncation; never pass that as a byte count.
    if ( size < 0 )
    {
        size_t nWrite = strnlen_s(aFilePrintBuffer, sizeof(aFilePrintBuffer));
        return !nWrite || stdFileWrite(fh, aFilePrintBuffer, nWrite) != nWrite;
    }

    return stdFileWrite(fh, aFilePrintBuffer, (size_t)size) != (size_t)size;
}

int J3DAPI stdFileSeek(tFileHandle fh, int offset, int origin)
{
    return fseek((FILE*)fh, offset, origin);
}

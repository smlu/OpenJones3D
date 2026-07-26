#include "stdGeneralTest.h"

#include <unity.h>

#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <j3dcore/Tests/j3dTest.h>
#include <std/General/stdConffile.h>
#include <std/General/std.h>
#include <std/General/stdMemory.h>
#include <std/General/stdUtil.h>
#include <std/RTI/addresses.h>
#include <std/types.h>

#define STDGENERALTEST_FAKE_FILE_HANDLE ((tFileHandle)(uintptr_t)1u)

#ifndef STD_GENERAL_TEST_TV_DIR
#define STD_GENERAL_TEST_TV_DIR "."
#endif

static char stdGeneralTest_aDebugOutput[2048];
static uint32_t stdGeneralTest_consoleAttributes;
static size_t stdGeneralTest_consoleWriteCount;
static char stdGeneralTest_aTvPath[512];
static const char* stdGeneralTest_pExistingFileName;
static int stdGeneralTest_numSuccessfulAllocationsBeforeFailure = -1;
static int stdGeneralTest_numSuccessfulFileOpensBeforeFailure   = -1;
static int stdGeneralTest_numSuccessfulFileReadsBeforeFailure   = -1;
static int stdGeneralTest_numSuccessfulFileWritesBeforeFailure  = -1;
static int stdGeneralTest_numSuccessfulFileGetsBeforeFailure    = -1;
static int stdGeneralTest_numSuccessfulFileSeeksBeforeFailure   = -1;
static bool stdGeneralTest_bStarted;
static bool stdGeneralTest_bMemoryMapped;

static bool stdGeneralTest_bRegistryStarted = true;
static bool stdGeneralTest_bHasRegistryInt;
static bool stdGeneralTest_bHasRegistryFloat;
static bool stdGeneralTest_bHasRegistryString;
static char stdGeneralTest_aRegistryIntKey[128];
static char stdGeneralTest_aRegistryFloatKey[128];
static char stdGeneralTest_aRegistryStringKey[128];
static char stdGeneralTest_aRegistryStringValue[256];
static int stdGeneralTest_registryIntValue;
static float stdGeneralTest_registryFloatValue;

static void StdGeneralTest_CopyString(char* pDst, size_t dstSize, const char* pSrc)
{
    if ( !dstSize )
    {
        return;
    }

    snprintf(pDst, dstSize, "%s", pSrc ? pSrc : "");
    pDst[dstSize - 1] = '\0';
}

static bool StdGeneralTest_ShouldFailOperation(int* pNumSuccessfulOperationsBeforeFailure)
{
    if ( *pNumSuccessfulOperationsBeforeFailure == 0 )
    {
        return true;
    }

    if ( *pNumSuccessfulOperationsBeforeFailure > 0 )
    {
        --(*pNumSuccessfulOperationsBeforeFailure);
    }

    return false;
}

static void* J3DAPI StdGeneralTest_Malloc(size_t size)
{
    if ( StdGeneralTest_ShouldFailOperation(&stdGeneralTest_numSuccessfulAllocationsBeforeFailure) )
    {
        return NULL;
    }

    return malloc(size);
}

static void J3DAPI StdGeneralTest_Free(void* pData)
{
    free(pData);
}

static void* J3DAPI StdGeneralTest_Realloc(void* pData, size_t size)
{
    if ( StdGeneralTest_ShouldFailOperation(&stdGeneralTest_numSuccessfulAllocationsBeforeFailure) )
    {
        return NULL;
    }

    return realloc(pData, size);
}

static int J3DAPI StdGeneralTest_Printf(const char* pFormat, ...)
{
    size_t len = strlen(stdGeneralTest_aDebugOutput);
    va_list args;

    va_start(args, pFormat);
    int result = vsnprintf(
        &stdGeneralTest_aDebugOutput[len],
        sizeof(stdGeneralTest_aDebugOutput) - len,
        pFormat,
        args
    );
    va_end(args);

    stdGeneralTest_aDebugOutput[sizeof(stdGeneralTest_aDebugOutput) - 1] = '\0';
    return result;
}

static int J3DAPI StdGeneralTest_DebugPrint(const char* pFormat, ...)
{
    size_t len = strlen(stdGeneralTest_aDebugOutput);
    va_list args;
    va_start(args, pFormat);

    int result = vsnprintf(
        &stdGeneralTest_aDebugOutput[len],
        sizeof(stdGeneralTest_aDebugOutput) - len,
        pFormat,
        args
    );
    va_end(args);

    stdGeneralTest_aDebugOutput[sizeof(stdGeneralTest_aDebugOutput) - 1] = '\0';
    return result;
}

static tFileHandle J3DAPI StdGeneralTest_FileOpen(const char* pFilename, const char* mode)
{
    if ( StdGeneralTest_ShouldFailOperation(&stdGeneralTest_numSuccessfulFileOpensBeforeFailure) )
    {
        return 0;
    }

    if ( stdGeneralTest_pExistingFileName && pFilename && strcmp(pFilename, stdGeneralTest_pExistingFileName) == 0 )
    {
        return STDGENERALTEST_FAKE_FILE_HANDLE;
    }

    return stdFileOpen(pFilename, mode);
}

static int J3DAPI StdGeneralTest_FileClose(tFileHandle fh)
{
    if ( fh == STDGENERALTEST_FAKE_FILE_HANDLE )
    {
        return 0;
    }

    return fh ? stdFileClose(fh) : -1;
}

static size_t J3DAPI StdGeneralTest_FileRead(tFileHandle fh, void* pOutData, size_t size)
{
    if ( StdGeneralTest_ShouldFailOperation(&stdGeneralTest_numSuccessfulFileReadsBeforeFailure) )
    {
        return 0;
    }

    if ( fh == STDGENERALTEST_FAKE_FILE_HANDLE )
    {
        return 0;
    }

    return stdFileRead(fh, pOutData, size);
}

static char* J3DAPI StdGeneralTest_FileGets(tFileHandle fh, char* pStr, size_t size)
{
    if ( StdGeneralTest_ShouldFailOperation(&stdGeneralTest_numSuccessfulFileGetsBeforeFailure) )
    {
        return NULL;
    }

    return fh == STDGENERALTEST_FAKE_FILE_HANDLE ? NULL : stdFileGets(fh, pStr, size);
}

static size_t J3DAPI StdGeneralTest_FileWrite(tFileHandle fh, const void* pData, size_t size)
{
    if ( StdGeneralTest_ShouldFailOperation(&stdGeneralTest_numSuccessfulFileWritesBeforeFailure) )
    {
        return 0;
    }

    return fh == STDGENERALTEST_FAKE_FILE_HANDLE ? 0 : stdFileWrite(fh, pData, size);
}

static int J3DAPI StdGeneralTest_FileEof(tFileHandle fh)
{
    return fh == STDGENERALTEST_FAKE_FILE_HANDLE ? 1 : stdFileEof(fh);
}

static int J3DAPI StdGeneralTest_FileTell(tFileHandle fh)
{
    return fh == STDGENERALTEST_FAKE_FILE_HANDLE ? 0 : stdFileTell(fh);
}

static int J3DAPI StdGeneralTest_FileSeek(tFileHandle fh, int offset, int origin)
{
    if ( StdGeneralTest_ShouldFailOperation(&stdGeneralTest_numSuccessfulFileSeeksBeforeFailure) )
    {
        return -1;
    }

    return fh == STDGENERALTEST_FAKE_FILE_HANDLE ? -1 : stdFileSeek(fh, offset, origin);
}

static size_t J3DAPI StdGeneralTest_FileSize(const char* pFilename)
{
    return stdFileSize(pFilename);
}

static int StdGeneralTest_FilePrintf(tFileHandle fh, const char* pFormat, ...)
{
    if ( fh == STDGENERALTEST_FAKE_FILE_HANDLE || !pFormat )
    {
        return 1;
    }

    va_list args;
    va_start(args, pFormat);
    int result = vfprintf((FILE*)fh, pFormat, args);
    va_end(args);
    return result < 0;
}

static wchar_t* J3DAPI StdGeneralTest_FileGetws(tFileHandle fh, wchar_t* pOutStr, size_t size)
{
    return fh == STDGENERALTEST_FAKE_FILE_HANDLE ? NULL : stdFileGetws(fh, pOutStr, size);
}

int J3DAPI stdConsole_WriteConsole(const char* pText, uint32_t textAttribute)
{
    stdGeneralTest_consoleAttributes = textAttribute;
    ++stdGeneralTest_consoleWriteCount;
    return StdGeneralTest_DebugPrint("%s", pText ? pText : "");
}

int J3DAPI wuRegistry_Startup(HKEY hKey, LPCSTR lpSubKey)
{
    J3D_UNUSED(hKey);
    J3D_UNUSED(lpSubKey);
    stdGeneralTest_bRegistryStarted = true;
    return 0;
}

bool wuRegistry_HasStarted(void)
{
    return stdGeneralTest_bRegistryStarted;
}

void J3DAPI wuRegistry_Shutdown(void)
{
    stdGeneralTest_bRegistryStarted = false;
}

int J3DAPI wuRegistry_SaveInt(const char* pKey, int value)
{
    StdGeneralTest_SetRegistryInt(pKey, value);
    return 0;
}

int J3DAPI wuRegistry_GetInt(const char* pKey, int defaultValue)
{
    return stdGeneralTest_bHasRegistryInt && pKey && strcmp(pKey, stdGeneralTest_aRegistryIntKey) == 0
        ? stdGeneralTest_registryIntValue
        : defaultValue;
}

int J3DAPI wuRegistry_SaveFloat(const char* pKey, float value)
{
    StdGeneralTest_SetRegistryFloat(pKey, value);
    return 0;
}

float J3DAPI wuRegistry_GetFloat(const char* pKey, float defaultValue)
{
    return stdGeneralTest_bHasRegistryFloat && pKey && strcmp(pKey, stdGeneralTest_aRegistryFloatKey) == 0
        ? stdGeneralTest_registryFloatValue
        : defaultValue;
}

int J3DAPI wuRegistry_SaveBool(const char* pKey, int defaultValue)
{
    return wuRegistry_SaveInt(pKey, defaultValue);
}

int J3DAPI wuRegistry_GetBool(const char* pKey, int defaultValue)
{
    return wuRegistry_GetInt(pKey, defaultValue);
}

bool J3DAPI wuRegistry_SaveBinary(const char* pKey, const uint8_t* pData, size_t size)
{
    J3D_UNUSED(pKey);
    J3D_UNUSED(pData);
    J3D_UNUSED(size);
    return false;
}

bool J3DAPI wuRegistry_GetBinary(const char* pKey, uint8_t* pData, size_t size)
{
    J3D_UNUSED(pKey);
    J3D_UNUSED(pData);
    J3D_UNUSED(size);
    return false;
}

int J3DAPI wuRegistry_SaveStr(const char* pKey, const char* pStr)
{
    StdGeneralTest_SetRegistryString(pKey, pStr);
    return 0;
}

int J3DAPI wuRegistry_GetStr(const char* pKey, char* pDstStr, size_t size, const char* pDefaultValue)
{
    const char* pValue = pDefaultValue;
    if ( stdGeneralTest_bHasRegistryString && pKey && strcmp(pKey, stdGeneralTest_aRegistryStringKey) == 0 )
    {
        pValue = stdGeneralTest_aRegistryStringValue;
    }

    if ( !pDstStr || !size )
    {
        return 1;
    }

    StdGeneralTest_CopyString(pDstStr, size, pValue ? pValue : "");
    return 0;
}

void StdGeneralTest_Startup(void)
{
    tHostServices* pHS;

    J3DTest_Startup();
    stdGeneralTest_bStarted = true;

    memset(&stdConffile_g_entry, 0, sizeof(stdConffile_g_entry));
    memset(&stdConffile_g_aLine, 0, sizeof(stdConffile_g_aLine));

    memset(&stdMemory_g_curState, 0, sizeof(stdMemory_g_curState));
    std_g_pHS = J3DTest_GetHostServices();
    stdGeneralTest_bMemoryMapped = true;

    pHS                 = J3DTest_GetHostServices();
    pHS->pMalloc       = StdGeneralTest_Malloc;
    pHS->pFree         = StdGeneralTest_Free;
    pHS->pRealloc      = StdGeneralTest_Realloc;
    pHS->pStatusPrint  = StdGeneralTest_Printf;
    pHS->pMessagePrint = StdGeneralTest_Printf;
    pHS->pWarningPrint = StdGeneralTest_Printf;
    pHS->pErrorPrint   = StdGeneralTest_Printf;
    pHS->pDebugPrint   = StdGeneralTest_DebugPrint;
    pHS->pFileOpen     = StdGeneralTest_FileOpen;
    pHS->pFileClose    = StdGeneralTest_FileClose;
    pHS->pFileRead     = StdGeneralTest_FileRead;
    pHS->pFileGets     = StdGeneralTest_FileGets;
    pHS->pFileWrite    = StdGeneralTest_FileWrite;
    pHS->pFileEOF      = StdGeneralTest_FileEof;
    pHS->pFileTell     = StdGeneralTest_FileTell;
    pHS->pFileSeek     = StdGeneralTest_FileSeek;
    pHS->pFileSize     = StdGeneralTest_FileSize;
    pHS->pFilePrintf   = StdGeneralTest_FilePrintf;
    pHS->pFileGetws    = StdGeneralTest_FileGetws;

    StdGeneralTest_ResetDebugOutput();
    stdGeneralTest_consoleAttributes = 0u;
    stdGeneralTest_consoleWriteCount = 0u;
    StdGeneralTest_SetExistingFileName(NULL);
    StdGeneralTest_ClearAllocationFailures();
    StdGeneralTest_ClearFileFailures();
    StdGeneralTest_ClearRegistryValues();
}

void StdGeneralTest_Shutdown(void)
{
    StdGeneralTest_ClearAllocationFailures();
    StdGeneralTest_ClearFileFailures();

    memset(&stdMemory_g_curState, 0, sizeof(stdMemory_g_curState));
    std_g_pHS = NULL;

    if ( stdGeneralTest_bMemoryMapped )
    {
        memset(&stdConffile_g_entry, 0, sizeof(stdConffile_g_entry));
        memset(&stdConffile_g_aLine, 0, sizeof(stdConffile_g_aLine));
        stdGeneralTest_bMemoryMapped = false;
    }

    if ( stdGeneralTest_bStarted )
    {
        J3DTest_Shutdown();
        stdGeneralTest_bStarted = false;
    }
}

tHostServices* StdGeneralTest_GetHostServices(void)
{
    return J3DTest_GetHostServices();
}

void StdGeneralTest_FailNextAllocations(int numSuccessfulAllocationsBeforeFailure)
{
    stdGeneralTest_numSuccessfulAllocationsBeforeFailure = numSuccessfulAllocationsBeforeFailure;
}

void StdGeneralTest_ClearAllocationFailures(void)
{
    stdGeneralTest_numSuccessfulAllocationsBeforeFailure = -1;
}

void StdGeneralTest_FailNextFileOpens(int numSuccessfulOpensBeforeFailure)
{
    stdGeneralTest_numSuccessfulFileOpensBeforeFailure = numSuccessfulOpensBeforeFailure;
}

void StdGeneralTest_FailNextFileReads(int numSuccessfulReadsBeforeFailure)
{
    stdGeneralTest_numSuccessfulFileReadsBeforeFailure = numSuccessfulReadsBeforeFailure;
}

void StdGeneralTest_FailNextFileWrites(int numSuccessfulWritesBeforeFailure)
{
    stdGeneralTest_numSuccessfulFileWritesBeforeFailure = numSuccessfulWritesBeforeFailure;
}

void StdGeneralTest_FailNextFileGets(int numSuccessfulGetsBeforeFailure)
{
    stdGeneralTest_numSuccessfulFileGetsBeforeFailure = numSuccessfulGetsBeforeFailure;
}

void StdGeneralTest_FailNextFileSeeks(int numSuccessfulSeeksBeforeFailure)
{
    stdGeneralTest_numSuccessfulFileSeeksBeforeFailure = numSuccessfulSeeksBeforeFailure;
}

void StdGeneralTest_ClearFileFailures(void)
{
    stdGeneralTest_numSuccessfulFileOpensBeforeFailure  = -1;
    stdGeneralTest_numSuccessfulFileReadsBeforeFailure  = -1;
    stdGeneralTest_numSuccessfulFileWritesBeforeFailure = -1;
    stdGeneralTest_numSuccessfulFileGetsBeforeFailure   = -1;
    stdGeneralTest_numSuccessfulFileSeeksBeforeFailure  = -1;
}

void StdGeneralTest_ResetDebugOutput(void)
{
    stdGeneralTest_aDebugOutput[0] = '\0';
}

// Compare the complete assertion condition using NULL spelling, independent of MSVC's expansion.
void StdGeneralTest_AssertLastAssert(const char* pExpectedText, const char* pExpectedFile)
{
    static const char aExpandedNull[] = "((void *)0)";
    const char* pCaptured = J3DTest_GetLastAssertText();
    char aNormalized[2048];
    TEST_ASSERT_TRUE(strlen(pCaptured) < sizeof(aNormalized));
    char* pOutput = aNormalized;
    while ( *pCaptured )
    {
        if ( strncmp(pCaptured, aExpandedNull, sizeof(aExpandedNull) - 1u) == 0 )
        {
            memcpy(pOutput, "NULL", 4u);
            pOutput += 4u;
            pCaptured += sizeof(aExpandedNull) - 1u;
        }
        else
        {
            *pOutput++ = *pCaptured++;
        }
    }
    *pOutput = '\0';
    TEST_ASSERT_EQUAL_STRING(pExpectedText, aNormalized);
    TEST_ASSERT_EQUAL_STRING(pExpectedFile, J3DTest_GetLastAssertFile());
    TEST_ASSERT_TRUE(J3DTest_GetLastAssertLine() > 0);
}

const char* StdGeneralTest_GetDebugOutput(void)
{
    return stdGeneralTest_aDebugOutput;
}

uint32_t StdGeneralTest_GetConsoleAttributes(void)
{
    return stdGeneralTest_consoleAttributes;
}

size_t StdGeneralTest_GetConsoleWriteCount(void)
{
    return stdGeneralTest_consoleWriteCount;
}

void StdGeneralTest_SetExistingFileName(const char* pFilename)
{
    stdGeneralTest_pExistingFileName = pFilename;
}

void StdGeneralTest_SetRegistryInt(const char* pKey, int value)
{
    StdGeneralTest_CopyString(stdGeneralTest_aRegistryIntKey, sizeof(stdGeneralTest_aRegistryIntKey), pKey);
    stdGeneralTest_registryIntValue = value;
    stdGeneralTest_bHasRegistryInt = true;
}

void StdGeneralTest_SetRegistryFloat(const char* pKey, float value)
{
    StdGeneralTest_CopyString(stdGeneralTest_aRegistryFloatKey, sizeof(stdGeneralTest_aRegistryFloatKey), pKey);
    stdGeneralTest_registryFloatValue = value;
    stdGeneralTest_bHasRegistryFloat = true;
}

void StdGeneralTest_SetRegistryString(const char* pKey, const char* pValue)
{
    StdGeneralTest_CopyString(stdGeneralTest_aRegistryStringKey, sizeof(stdGeneralTest_aRegistryStringKey), pKey);
    StdGeneralTest_CopyString(stdGeneralTest_aRegistryStringValue, sizeof(stdGeneralTest_aRegistryStringValue), pValue);
    stdGeneralTest_bHasRegistryString = true;
}

void StdGeneralTest_ClearRegistryValues(void)
{
    stdGeneralTest_bRegistryStarted        = true;
    stdGeneralTest_bHasRegistryInt         = false;
    stdGeneralTest_bHasRegistryFloat       = false;
    stdGeneralTest_bHasRegistryString      = false;
    stdGeneralTest_aRegistryIntKey[0]      = '\0';
    stdGeneralTest_aRegistryFloatKey[0]    = '\0';
    stdGeneralTest_aRegistryStringKey[0]   = '\0';
    stdGeneralTest_aRegistryStringValue[0] = '\0';
    stdGeneralTest_registryIntValue        = 0;
    stdGeneralTest_registryFloatValue      = 0.0f;
}

const char* StdGeneralTest_GetTestVectorPath(const char* pRelativePath)
{
    STD_FORMAT(stdGeneralTest_aTvPath, "%s/%s", STD_GENERAL_TEST_TV_DIR, pRelativePath ? pRelativePath : "");
    stdGeneralTest_aTvPath[sizeof(stdGeneralTest_aTvPath) - 1] = '\0';
    return stdGeneralTest_aTvPath;
}

void StdGeneralTest_DeleteFile(const char* pPath)
{
    if ( pPath )
    {
        errno = 0;
        if ( remove(pPath) != 0 && errno != ENOENT )
        {
            fprintf(stderr, "Warning: Failed to delete test file '%s' (errno=%d).\n", pPath, errno);
        }
    }
}

int StdGeneralTest_CopyFile(const char* pSrcPath, const char* pDstPath)
{
    FILE* pSrcFile;
    FILE* pDstFile;

    if ( fopen_s(&pSrcFile, pSrcPath, "rb") != 0 )
    {
        return 0;
    }

    if ( fopen_s(&pDstFile, pDstPath, "wb") != 0 )
    {
        fclose(pSrcFile);
        return 0;
    }

    char aBuffer[4096];
    size_t nRead;
    while ( (nRead = fread(aBuffer, 1u, sizeof(aBuffer), pSrcFile)) > 0 )
    {
        if ( fwrite(aBuffer, 1u, nRead, pDstFile) != nRead )
        {
            fclose(pDstFile);
            fclose(pSrcFile);
            return 0;
        }
    }

    int bSuccess = !ferror(pSrcFile) && !ferror(pDstFile);
    fclose(pDstFile);
    fclose(pSrcFile);
    return bSuccess;
}

int StdGeneralTest_CopyTestVectorFile(const char* pRelativePath, const char* pDstPath)
{
    return StdGeneralTest_CopyFile(StdGeneralTest_GetTestVectorPath(pRelativePath), pDstPath);
}
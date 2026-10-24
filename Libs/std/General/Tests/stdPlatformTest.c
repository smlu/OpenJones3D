#include <Windows.h>
#include <DbgHelp.h>
#include <unity_fixture.h>

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>

#include <std/General/std.h>
#include <std/General/stdFileUtil.h>
#include <std/General/stdMemory.h>
#include <std/General/stdPlatform.h>
#include <std/General/stdUtil.h>

#include "stdGeneralTest.h"

bool CheckStackFrameStringArg(uintptr_t ptr, bool* bWide, char* outAnsiBuf, size_t ansiBufSize, wchar_t* outWideBuf, size_t wideBufSize, size_t* readLen);
void stdPlatform_InitHighResTimer(void);
extern LARGE_INTEGER stdPlatform_perfCounterFreq;
extern LARGE_INTEGER stdPlatform_perfCounterStart;

static char stdPlatformTest_aStackTrace[2048];
static char stdPlatformTest_aDeathTestDir[MAX_PATH];

static void stdPlatformTest_RemoveDeathTestFiles(void)
{
    if ( stdPlatformTest_aDeathTestDir[0] )
    {
        char aDumpPath[MAX_PATH + sizeof("\\core.dmp")];
        STD_FORMAT(aDumpPath, "%s\\core.dmp", stdPlatformTest_aDeathTestDir);
        DeleteFileA(aDumpPath);
        RemoveDirectoryA(stdPlatformTest_aDeathTestDir);
        stdPlatformTest_aDeathTestDir[0] = '\0';
    }
}

int StdPlatformTest_RunDeathChild(void)
{
    SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX | SEM_NOOPENFILEERRORBOX);
    stdPlatform_InstallSignalHandler();
    RaiseException(EXCEPTION_ACCESS_VIOLATION, EXCEPTION_NONCONTINUABLE, 0u, NULL);
    return EXIT_FAILURE;
}

static void J3DAPI stdPlatformTest_CaptureStackTrace(const char* pText, ...)
{
    if ( pText )
    {
        stdUtil_StringCat(stdPlatformTest_aStackTrace, sizeof(stdPlatformTest_aStackTrace), pText);
    }
}

TEST_GROUP(stdPlatform);

TEST_SETUP(stdPlatform)
{
    StdGeneralTest_Startup();
    stdPlatformTest_aStackTrace[0] = '\0';
    stdPlatformTest_aDeathTestDir[0] = '\0';
}

TEST_TEAR_DOWN(stdPlatform)
{
    SetUnhandledExceptionFilter(NULL);
    stdPlatformTest_RemoveDeathTestFiles();
    stdFileUtil_RmDir("stdPlatformTest_dir");
    StdGeneralTest_Shutdown();
}

TEST(stdPlatform, TestInitServicesPopulatesExpectedCallbacksAndClearZerosThem)
{
    tHostServices services;
    tHostServices zeroServices;
    HRESULT hr;

    STD_ZEROMEM(&services, sizeof(services));
    STD_ZEROMEM(&zeroServices, sizeof(zeroServices));
    hr = stdPlatform_InitServices(&services);
    TEST_ASSERT_TRUE(SUCCEEDED(hr) || hr == S_FALSE);
    TEST_ASSERT_EQUAL_FLOAT(1000.0f, services.unknown1);
    TEST_ASSERT_EQUAL_PTR(stdPlatform_Printf, services.pMessagePrint);
    TEST_ASSERT_EQUAL_PTR(stdPlatform_Printf, services.pStatusPrint);
    TEST_ASSERT_EQUAL_PTR(stdPlatform_Printf, services.pWarningPrint);
    TEST_ASSERT_EQUAL_PTR(stdPlatform_Printf, services.pErrorPrint);
    TEST_ASSERT_EQUAL_PTR(stdPlatform_Printf, services.pDebugPrint);
    TEST_ASSERT_EQUAL_PTR(stdPlatform_Assert, services.pAssert);
    TEST_ASSERT_EQUAL_PTR(stdMemory_BlockMalloc, services.pMalloc);
    TEST_ASSERT_EQUAL_PTR(stdMemory_BlockFree, services.pFree);
    TEST_ASSERT_EQUAL_PTR(stdMemory_BlockRealloc, services.pRealloc);
    TEST_ASSERT_EQUAL_PTR(stdPlatform_GetTimeMsec, services.pGetTimeMsec);
    TEST_ASSERT_EQUAL_PTR(stdFileOpen, services.pFileOpen);
    TEST_ASSERT_EQUAL_PTR(stdFileClose, services.pFileClose);
    TEST_ASSERT_EQUAL_PTR(stdFileRead, services.pFileRead);
    TEST_ASSERT_EQUAL_PTR(stdFileGets, services.pFileGets);
    TEST_ASSERT_EQUAL_PTR(stdFileWrite, services.pFileWrite);
    TEST_ASSERT_EQUAL_PTR(stdFileEof, services.pFileEOF);
    TEST_ASSERT_EQUAL_PTR(stdFileTell, services.pFileTell);
    TEST_ASSERT_EQUAL_PTR(stdFileSeek, services.pFileSeek);
    TEST_ASSERT_EQUAL_PTR(stdFileSize, services.pFileSize);
    TEST_ASSERT_EQUAL_PTR(stdFilePrintf, services.pFilePrintf);
    TEST_ASSERT_EQUAL_PTR(stdFileGetws, services.pFileGetws);
    TEST_ASSERT_EQUAL_PTR(stdPlatform_AllocHandle, services.pAllocHandle);
    TEST_ASSERT_EQUAL_PTR(stdPlatform_FreeHandle, services.pFreeHandle);
    TEST_ASSERT_EQUAL_PTR(stdPlatform_ReallocHandle, services.pReallocHandle);
    TEST_ASSERT_EQUAL_PTR(stdPlatform_LockHandle, services.pLockHandle);
    TEST_ASSERT_EQUAL_PTR(stdPlatform_UnlockHandle, services.pUnlockHandle);

    stdPlatform_ClearServices(&services);
    TEST_ASSERT_EQUAL_MEMORY(&zeroServices, &services, sizeof(services));
}

TEST(stdPlatform, TestHandleAllocationReallocationFreeAndLock)
{
    uint8_t* pData = (uint8_t*)stdPlatform_AllocHandle(8u);

    TEST_ASSERT_NOT_NULL(pData);
    pData[0] = 0x12u;
    pData[7] = 0x34u;

    pData = (uint8_t*)stdPlatform_ReallocHandle(pData, 16u);
    TEST_ASSERT_NOT_NULL(pData);
    TEST_ASSERT_EQUAL_UINT8(0x12u, pData[0]);
    TEST_ASSERT_EQUAL_UINT8(0x34u, pData[7]);
    stdPlatform_FreeHandle(pData);

    TEST_ASSERT_EQUAL_INT(123, stdPlatform_LockHandle(123));
    stdPlatform_UnlockHandle();
}

TEST(stdPlatform, TestTimerPrintfAndDirExists)
{
    tStdTime first = stdPlatform_GetTimeMsec();
    tStdTime second = stdPlatform_GetTimeMsec();

    TEST_ASSERT_TRUE(second >= first);
    TEST_ASSERT_EQUAL_INT(1, stdPlatform_Printf("%s %d", "platform", 7));
    TEST_ASSERT_EQUAL_STRING("platform 7", std_g_genBuffer);

    TEST_ASSERT_TRUE(stdFileUtil_MkDir("stdPlatformTest_dir"));
    TEST_ASSERT_TRUE(stdPlatform_DirExists("stdPlatformTest_dir"));
    TEST_ASSERT_FALSE(stdPlatform_DirExists("stdPlatformTest_missing_dir"));
}

TEST(stdPlatform, TestTimeMsecUsesActiveTimerBranch)
{
#ifdef J3D_QOL_IMPROVEMENTS
    stdPlatform_InitHighResTimer();
    TEST_ASSERT_TRUE(stdPlatform_perfCounterFreq.QuadPart > 0);
    TEST_ASSERT_TRUE(stdPlatform_perfCounterStart.QuadPart > 0);
#endif

    tStdTime first = stdPlatform_GetTimeMsec();
    Sleep(1);
    TEST_ASSERT_TRUE(stdPlatform_GetTimeMsec() >= first);
}

TEST(stdPlatform, TestCheckStackFrameStringArgRejectsInvalidAndReadsStrings)
{
    char aAnsiText[32];
    const wchar_t awWideText[] = L"Sanctuary";
    char aAnsiBuf[32] = { 0 };
    wchar_t awWideBuf[32] = { 0 };
    bool bWide = true;
    size_t readLen = 99u;

    // Keep the byte after the ANSI terminator non-zero so the wide-string probe cannot accept it first.
    STD_FILLMEM(aAnsiText, 0xFF, sizeof(aAnsiText));
    STD_STRCPY(aAnsiText, "Temple");

    TEST_ASSERT_FALSE(CheckStackFrameStringArg(0u, &bWide, aAnsiBuf, STD_ARRAYLEN(aAnsiBuf), awWideBuf, STD_ARRAYLEN(awWideBuf), &readLen));
    TEST_ASSERT_FALSE(bWide);
    TEST_ASSERT_EQUAL_size_t(0u, readLen);

    TEST_ASSERT_FALSE(CheckStackFrameStringArg(1u, &bWide, aAnsiBuf, STD_ARRAYLEN(aAnsiBuf), awWideBuf, STD_ARRAYLEN(awWideBuf), &readLen));

    TEST_ASSERT_TRUE(CheckStackFrameStringArg((uintptr_t)aAnsiText, &bWide, aAnsiBuf, STD_ARRAYLEN(aAnsiBuf), awWideBuf, STD_ARRAYLEN(awWideBuf), &readLen));
    TEST_ASSERT_FALSE(bWide);
    TEST_ASSERT_EQUAL_STRING("Temple", aAnsiBuf);

    TEST_ASSERT_TRUE(CheckStackFrameStringArg((uintptr_t)awWideText, &bWide, aAnsiBuf, STD_ARRAYLEN(aAnsiBuf), awWideBuf, STD_ARRAYLEN(awWideBuf), &readLen));
    TEST_ASSERT_TRUE(bWide);
    TEST_ASSERT_EQUAL(0, wcscmp(L"Sanctuary", awWideBuf));
}

TEST(stdPlatform, TestCheckStackFrameStringArgRejectsNonPrintableAndUnterminatedStrings)
{
    char aNonPrintable[] = { 'O', 'k', '\x01', '\0' };
    char aUnterminated[32];
    char aAnsiBuf[32] = { 0 };
    wchar_t awWideBuf[32] = { 0 };
    bool bWide = false;
    size_t readLen = 0;

    STD_FILLMEM(aUnterminated, 'A', sizeof(aUnterminated));

    TEST_ASSERT_FALSE(CheckStackFrameStringArg(
        (uintptr_t)aNonPrintable,
        &bWide,
        aAnsiBuf,
        STD_ARRAYLEN(aAnsiBuf),
        awWideBuf,
        STD_ARRAYLEN(awWideBuf),
        &readLen
    ));
    TEST_ASSERT_FALSE(bWide);

    TEST_ASSERT_FALSE(CheckStackFrameStringArg(
        (uintptr_t)aUnterminated,
        &bWide,
        aAnsiBuf,
        STD_ARRAYLEN(aAnsiBuf),
        awWideBuf,
        STD_ARRAYLEN(awWideBuf),
        &readLen
    ));
}

TEST(stdPlatform, TestInstallSignalHandlerAndZeroFrameStackTrace)
{
    stdPlatform_InstallSignalHandler();
    stdPlatform_PrintStackTrace(stdPlatformTest_CaptureStackTrace, 0u);

    TEST_ASSERT_EQUAL_STRING(
        "\n==================== STACK TRACE ====================\n"
        "=========================================================\n",
        stdPlatformTest_aStackTrace
    );
}

TEST(stdPlatform, TestUnhandledExceptionWritesMiniDump)
{
    char aTempPath[MAX_PATH];
    DWORD tempPathLen = GetTempPathA(STD_ARRAYLEN(aTempPath), aTempPath);
    TEST_ASSERT_TRUE(tempPathLen > 0u && tempPathLen < STD_ARRAYLEN(aTempPath));
    TEST_ASSERT_NOT_EQUAL_UINT(0u, GetTempFileNameA(aTempPath, "OJD", 0u, stdPlatformTest_aDeathTestDir));
    TEST_ASSERT_TRUE(DeleteFileA(stdPlatformTest_aDeathTestDir));
    TEST_ASSERT_TRUE(CreateDirectoryA(stdPlatformTest_aDeathTestDir, NULL));

    char aExecutablePath[MAX_PATH];
    DWORD executablePathLen = GetModuleFileNameA(NULL, aExecutablePath, STD_ARRAYLEN(aExecutablePath));
    TEST_ASSERT_TRUE(executablePathLen > 0u && executablePathLen < STD_ARRAYLEN(aExecutablePath));

    char aCommandLine[MAX_PATH * 2u];
    STD_FORMAT(aCommandLine, "\"%s\" --std-platform-death-child", aExecutablePath);

    STARTUPINFOA startupInfo;
    PROCESS_INFORMATION processInfo;
    STD_ZEROMEM(&startupInfo, sizeof(startupInfo));
    STD_ZEROMEM(&processInfo, sizeof(processInfo));
    startupInfo.cb = sizeof(startupInfo);

    BOOL bCreated = CreateProcessA(
        aExecutablePath,
        aCommandLine,
        NULL,
        NULL,
        FALSE,
        CREATE_NO_WINDOW,
        NULL,
        stdPlatformTest_aDeathTestDir,
        &startupInfo,
        &processInfo
    );
    TEST_ASSERT_TRUE(bCreated);

    DWORD waitResult = WaitForSingleObject(processInfo.hProcess, 30000u);
    if ( waitResult == WAIT_TIMEOUT )
    {
        TerminateProcess(processInfo.hProcess, ERROR_TIMEOUT);
        WaitForSingleObject(processInfo.hProcess, 5000u);
    }

    DWORD exitCode    = 0u;
    BOOL bGotExitCode = GetExitCodeProcess(processInfo.hProcess, &exitCode);
    CloseHandle(processInfo.hThread);
    CloseHandle(processInfo.hProcess);
    TEST_ASSERT_TRUE(bGotExitCode);
    TEST_ASSERT_EQUAL_UINT32(WAIT_OBJECT_0, waitResult);
    TEST_ASSERT_EQUAL_HEX32(EXCEPTION_ACCESS_VIOLATION, exitCode);

    char aDumpPath[MAX_PATH + sizeof("\\core.dmp")];
    STD_FORMAT(aDumpPath, "%s\\core.dmp", stdPlatformTest_aDeathTestDir);
    HANDLE hDump = CreateFileA(aDumpPath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    TEST_ASSERT_TRUE(hDump != INVALID_HANDLE_VALUE);

    LARGE_INTEGER dumpSize;
    MINIDUMP_HEADER header;
    DWORD numRead    = 0u;
    BOOL bGotSize    = GetFileSizeEx(hDump, &dumpSize);
    BOOL bReadHeader = ReadFile(hDump, &header, sizeof(header), &numRead, NULL);
    CloseHandle(hDump);

    TEST_ASSERT_TRUE(bGotSize);
    TEST_ASSERT_TRUE(dumpSize.QuadPart > (LONGLONG)sizeof(MINIDUMP_HEADER));
    TEST_ASSERT_TRUE(bReadHeader);
    TEST_ASSERT_EQUAL_UINT32(sizeof(header), numRead);
    TEST_ASSERT_EQUAL_HEX32(MINIDUMP_SIGNATURE, header.Signature);
    TEST_ASSERT_EQUAL_UINT16((uint16_t)MINIDUMP_VERSION, (uint16_t)(header.Version & 0xFFFFu));
    TEST_ASSERT_TRUE(header.NumberOfStreams > 0u);
    TEST_ASSERT_TRUE(header.StreamDirectoryRva >= sizeof(header));
    TEST_ASSERT_TRUE(
        (uint64_t)header.StreamDirectoryRva + (uint64_t)header.NumberOfStreams * sizeof(MINIDUMP_DIRECTORY)
        <= (uint64_t)dumpSize.QuadPart
    );
}

TEST(stdPlatform, TestOneFrameStackTraceProducesFrameOutput)
{
    stdPlatform_PrintStackTrace(stdPlatformTest_CaptureStackTrace, 1u);

    TEST_ASSERT_NOT_NULL(strstr(stdPlatformTest_aStackTrace, "STACK TRACE"));
    TEST_ASSERT_NOT_NULL(strstr(stdPlatformTest_aStackTrace, "#00"));
}

TEST_GROUP_RUNNER(stdPlatform)
{
    RUN_TEST_CASE(stdPlatform, TestInitServicesPopulatesExpectedCallbacksAndClearZerosThem);
    RUN_TEST_CASE(stdPlatform, TestHandleAllocationReallocationFreeAndLock);
    RUN_TEST_CASE(stdPlatform, TestTimerPrintfAndDirExists);
    RUN_TEST_CASE(stdPlatform, TestTimeMsecUsesActiveTimerBranch);
    RUN_TEST_CASE(stdPlatform, TestCheckStackFrameStringArgRejectsInvalidAndReadsStrings);
    RUN_TEST_CASE(stdPlatform, TestCheckStackFrameStringArgRejectsNonPrintableAndUnterminatedStrings);
    RUN_TEST_CASE(stdPlatform, TestInstallSignalHandlerAndZeroFrameStackTrace);
    RUN_TEST_CASE(stdPlatform, TestUnhandledExceptionWritesMiniDump);
    RUN_TEST_CASE(stdPlatform, TestOneFrameStackTraceProducesFrameOutput);
}

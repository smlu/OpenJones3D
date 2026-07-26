#include <unity_fixture.h>

#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <wchar.h>

#include <j3dcore/Tests/j3dTest.h>
#include <std/General/std.h>
#include <std/General/stdJSON.h>
#include <std/General/stdMemory.h>
#include <std/General/stdUtil.h>

#include "stdGeneralTest.h"

static char stdTest_aPrintedText[4096];
static const char* stdTest_pCallbackFormat;
static size_t stdTest_callbackCount;

static int J3DAPI stdTest_PrintCapture(const char* pFormat, ...)
{
    va_list args;

    stdTest_pCallbackFormat = pFormat;
    ++stdTest_callbackCount;
    va_start(args, pFormat);
    vsnprintf(stdTest_aPrintedText, sizeof(stdTest_aPrintedText), pFormat, args);
    va_end(args);

    stdTest_aPrintedText[sizeof(stdTest_aPrintedText) - 1] = '\0';
    return (int)strlen(stdTest_aPrintedText);
}

static size_t stdTest_logChannel;

#define STDTEST_PRINT_CALLBACK(name, channel) \
    static int J3DAPI name(const char* pFormat, ...) \
    { \
        stdTest_logChannel = channel; \
        stdTest_pCallbackFormat = pFormat; \
        ++stdTest_callbackCount; \
        va_list args; \
        va_start(args, pFormat); \
        vsnprintf(stdTest_aPrintedText, sizeof(stdTest_aPrintedText), pFormat, args); \
        va_end(args); \
        return -17; \
    }

STDTEST_PRINT_CALLBACK(stdTest_DebugCallback, 0u)
STDTEST_PRINT_CALLBACK(stdTest_StatusCallback, 1u)
STDTEST_PRINT_CALLBACK(stdTest_MessageCallback, 2u)
STDTEST_PRINT_CALLBACK(stdTest_WarningCallback, 3u)
STDTEST_PRINT_CALLBACK(stdTest_ErrorCallback, 4u)

static void stdTest_DeleteFile(const char* pPath)
{
    StdGeneralTest_DeleteFile(pPath);
}

TEST_GROUP(std);

TEST_SETUP(std)
{
    StdGeneralTest_Startup();
    stdTest_aPrintedText[0] = '\0';
    stdTest_pCallbackFormat = NULL;
    stdTest_callbackCount = 0u;
}

TEST_TEAR_DOWN(std)
{
    stdTest_DeleteFile("stdTest_fileWrappers.tmp");
    stdTest_DeleteFile("stdTest_filePrintf.tmp");
    stdTest_DeleteFile("stdTest_gets.tmp");
    stdTest_DeleteFile("stdTest_getws.tmp");
    StdGeneralTest_Shutdown();
}

TEST(std, TestStartupAndShutdownStartOwnedModules)
{
    TEST_ASSERT_FALSE(stdJSON_HasStarted());

    stdStartup(StdGeneralTest_GetHostServices());
    TEST_ASSERT_EQUAL_PTR(StdGeneralTest_GetHostServices(), std_g_pHS);
    TEST_ASSERT_TRUE(stdJSON_HasStarted());

    stdShutdown();
    TEST_ASSERT_FALSE(stdJSON_HasStarted());
}

TEST(std, TestPrintfPrefixesMessagesAndHandlesNullFormat)
{
    stdPrintf(stdTest_PrintCapture, "C:\\games\\Jones3D\\source.c", 77, "Value %d", 42);
    TEST_ASSERT_EQUAL_STRING("source.c(77): Value 42", stdTest_aPrintedText);
    TEST_ASSERT_EQUAL_STRING("%s", stdTest_pCallbackFormat);
    TEST_ASSERT_EQUAL_size_t(1u, stdTest_callbackCount);

    stdPrintf(stdTest_PrintCapture, NULL, 0u, "%s", "no file");
    TEST_ASSERT_EQUAL_STRING("(0): no file", stdTest_aPrintedText);

    stdPrintf(stdTest_PrintCapture, "source.c", 8, NULL);
    TEST_ASSERT_EQUAL_STRING("source.c(8): ", stdTest_aPrintedText);

    stdTest_aPrintedText[0] = '\0';
    stdPrintf(NULL, "source.c", 1, "ignored");
    TEST_ASSERT_EQUAL_STRING("", stdTest_aPrintedText);

    stdPrintf(stdTest_PrintCapture, "C:\\100%s%n.c", 9, "Progress: 100%%; literal %%s and %%n");
    TEST_ASSERT_EQUAL_STRING("100%s%n.c(9): Progress: 100%; literal %s and %n", stdTest_aPrintedText);
}

TEST(std, TestPrintfTruncatesWithoutLosingFormattedPrefix)
{
    char aLongMessage[4096];
    memset(aLongMessage, 'x', sizeof(aLongMessage));
    aLongMessage[STD_ARRAYLEN(aLongMessage) - 1u] = '\0';

    stdPrintf(stdTest_PrintCapture, "long.c", 10, "%s", aLongMessage);
    TEST_ASSERT_EQUAL_size_t(2047u, strlen(stdTest_aPrintedText));
    char aExpected[2048];
    memcpy(aExpected, "long.c(10): ", 12u);
    memset(aExpected + 12u, 'x', sizeof(aExpected) - 13u);
    aExpected[sizeof(aExpected) - 1u] = '\0';
    TEST_ASSERT_EQUAL_STRING(aExpected, stdTest_aPrintedText);
}

TEST(std, TestConsolePrintfWritesThroughConsoleStub)
{
    TEST_ASSERT_EQUAL_INT((int)sizeof(std_g_genBuffer), stdConsolePrintf("%s %d", "Temple", 12));
    TEST_ASSERT_EQUAL_STRING("Temple 12", StdGeneralTest_GetDebugOutput());
    TEST_ASSERT_EQUAL_STRING("Temple 12", std_g_genBuffer);
    TEST_ASSERT_EQUAL_UINT32(FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE, StdGeneralTest_GetConsoleAttributes());
    TEST_ASSERT_EQUAL_size_t(1u, StdGeneralTest_GetConsoleWriteCount());
}

TEST(std, TestFileFromPathAndBitPosition)
{
    TEST_ASSERT_EQUAL_STRING("indy3d.exe", stdFileFromPath("C:\\Games\\indy3d.exe"));
    TEST_ASSERT_EQUAL_STRING("plain.txt", stdFileFromPath("plain.txt"));
    TEST_ASSERT_EQUAL_STRING("/tmp/plain.txt", stdFileFromPath("/tmp/plain.txt"));

    TEST_ASSERT_EQUAL_INT(0, stdCalcBitPos(0));
    TEST_ASSERT_EQUAL_INT(0, stdCalcBitPos(1));
    TEST_ASSERT_EQUAL_INT(1, stdCalcBitPos(2));
    TEST_ASSERT_EQUAL_INT(1, stdCalcBitPos(3));
    TEST_ASSERT_EQUAL_INT(7, stdCalcBitPos(255));
    TEST_ASSERT_EQUAL_INT(8, stdCalcBitPos(256));
}

TEST(std, TestFileWrappersReadWriteSeekTellSizeAndPrintf)
{
    const char* pPath = "stdTest_fileWrappers.tmp";
    char aReadBuffer[32];
    tFileHandle fh;

    stdTest_DeleteFile(pPath);

    fh = stdFileOpen(pPath, "wb");
    TEST_ASSERT_NOT_EQUAL(0u, fh);
    TEST_ASSERT_EQUAL_size_t(5u, stdFileWrite(fh, "Alpha", 5u));
    TEST_ASSERT_EQUAL_INT(0, stdFilePrintf(fh, "-%d", 17));
    TEST_ASSERT_EQUAL_INT(0, stdFileFlush(fh));
    TEST_ASSERT_EQUAL_INT(0, stdFileClose(fh));

    TEST_ASSERT_EQUAL_size_t(8u, stdFileSize(pPath));

    fh = stdFileOpen(pPath, "rb");
    TEST_ASSERT_NOT_EQUAL(0u, fh);
    TEST_ASSERT_EQUAL_INT(0, stdFileSeek(fh, 5, SEEK_SET));
    TEST_ASSERT_EQUAL_INT(5, stdFileTell(fh));
    TEST_ASSERT_EQUAL_size_t(3u, stdFileRead(fh, aReadBuffer, 3u));
    aReadBuffer[3] = '\0';
    TEST_ASSERT_EQUAL_STRING("-17", aReadBuffer);
    TEST_ASSERT_EQUAL_INT(0, stdFileClose(fh));

    stdTest_DeleteFile(pPath);
}

TEST(std, TestFileGetsAndGetws)
{
    const char* pTextPath = "stdTest_gets.tmp";
    const char* pWidePath = "stdTest_getws.tmp";
    wchar_t aWideText[16];
    char aText[16];
    tFileHandle fh;

    stdTest_DeleteFile(pTextPath);
    stdTest_DeleteFile(pWidePath);

    fh = stdFileOpen(pTextPath, "w");
    TEST_ASSERT_NOT_EQUAL(0u, fh);
    TEST_ASSERT_EQUAL_size_t(5u, stdFileWrite(fh, "line\n", 5u));
    TEST_ASSERT_EQUAL_INT(0, stdFileClose(fh));

    fh = stdFileOpen(pTextPath, "r");
    TEST_ASSERT_NOT_EQUAL(0u, fh);
    TEST_ASSERT_EQUAL_PTR(aText, stdFileGets(fh, aText, sizeof(aText)));
    TEST_ASSERT_EQUAL_STRING("line\n", aText);
    TEST_ASSERT_FALSE(stdFileEof(fh));
    TEST_ASSERT_NULL(stdFileGets(fh, aText, sizeof(aText)));
    TEST_ASSERT_TRUE(stdFileEof(fh));
    TEST_ASSERT_EQUAL_INT(0, stdFileClose(fh));

    fh = stdFileOpen(pWidePath, "w");
    TEST_ASSERT_NOT_EQUAL(0u, fh);
    TEST_ASSERT_EQUAL_size_t(5u, stdFileWrite(fh, "wide\n", 5u));
    TEST_ASSERT_EQUAL_INT(0, stdFileClose(fh));

    fh = stdFileOpen(pWidePath, "r");
    TEST_ASSERT_NOT_EQUAL(0u, fh);
    TEST_ASSERT_EQUAL_PTR(aWideText, stdFileGetws(fh, aWideText, STD_ARRAYLEN(aWideText)));
    TEST_ASSERT_EQUAL(0, wcscmp(L"wide\n", aWideText));
    TEST_ASSERT_EQUAL_INT(0, stdFileClose(fh));

    stdTest_DeleteFile(pTextPath);
    stdTest_DeleteFile(pWidePath);
}

TEST(std, TestFilePrintfCapacityAndWriteFailures)
{
    const char* pPath = "stdTest_filePrintf.tmp";
    char aPayload[2049];
    char aReadBuffer[2048];
    tFileHandle fh;

    memset(aPayload, 'q', sizeof(aPayload));
    aPayload[2047] = '\0';

    fh = stdFileOpen(pPath, "wb");
    TEST_ASSERT_NOT_EQUAL(0u, fh);
    TEST_ASSERT_EQUAL_INT(0, stdFilePrintf(fh, "%s", aPayload));
    TEST_ASSERT_EQUAL_INT(0, stdFileClose(fh));
    TEST_ASSERT_EQUAL_size_t(2047u, stdFileSize(pPath));

    fh = stdFileOpen(pPath, "rb");
    TEST_ASSERT_NOT_EQUAL(0u, fh);
    TEST_ASSERT_EQUAL_size_t(2047u, stdFileRead(fh, aReadBuffer, sizeof(aReadBuffer)));
    TEST_ASSERT_EACH_EQUAL_UINT8('q', aReadBuffer, 2047u);
    TEST_ASSERT_EQUAL_INT(1, stdFilePrintf(fh, "cannot write"));
    TEST_ASSERT_EQUAL_INT(0, stdFileClose(fh));

    aPayload[2047] = 'q';
    aPayload[2048] = '\0';
    fh = stdFileOpen(pPath, "wb");
    TEST_ASSERT_NOT_EQUAL(0u, fh);
    TEST_ASSERT_EQUAL_INT(0, stdFilePrintf(fh, "%s", aPayload));
    TEST_ASSERT_EQUAL_INT(0, stdFileClose(fh));
    TEST_ASSERT_EQUAL_size_t(2047u, stdFileSize(pPath));

#ifdef J3D_RUNTIME_GUARDS
    TEST_ASSERT_EQUAL_INT(1, stdFilePrintf(0, "%s", "bad"));
    fh = stdFileOpen(pPath, "rb");
    TEST_ASSERT_NOT_EQUAL(0u, fh);
    TEST_ASSERT_EQUAL_INT(1, stdFilePrintf(fh, NULL));
    TEST_ASSERT_EQUAL_INT(0, stdFileClose(fh));
#endif

    TEST_ASSERT_EQUAL_size_t(0u, stdFileSize("stdTest_missing_file.tmp"));
    TEST_ASSERT_EQUAL_INT(0, stdFileOpen("stdTest_missing_file.tmp", "rb"));
}

TEST(std, TestPrintfPrefixFillsCapacityAndEmptyMessages)
{
    // A prefix that fills the output buffer must be terminated and delivered exactly once.
    char aLongPath[4096];
    char aExpected[2048];
    memset(aLongPath, 'p', sizeof(aLongPath) - 1u);
    aLongPath[sizeof(aLongPath) - 1u] = '\0';
    memset(aExpected, 'p', sizeof(aExpected) - 1u);
    aExpected[sizeof(aExpected) - 1u] = '\0';
    stdPrintf(stdTest_PrintCapture, aLongPath, 1u, "must not be appended");
    TEST_ASSERT_EQUAL_STRING(aExpected, stdTest_aPrintedText);
    TEST_ASSERT_EQUAL_STRING("%s", stdTest_pCallbackFormat);
    TEST_ASSERT_EQUAL_size_t(1u, stdTest_callbackCount);

    // An empty message still delivers the exact file and line prefix.
    stdPrintf(stdTest_PrintCapture, "empty.c", 0u, "");
    TEST_ASSERT_EQUAL_STRING("empty.c(0): ", stdTest_aPrintedText);
    TEST_ASSERT_EQUAL_size_t(2u, stdTest_callbackCount);
}

TEST(std, TestLoggingAndAssertMacrosPreserveCallbackOutputs)
{
    // Use a distinct callback per log level to verify routing, exact text, and callback arguments.
    tHostServices* pHS = StdGeneralTest_GetHostServices();
    pHS->pDebugPrint   = stdTest_DebugCallback;
    pHS->pStatusPrint  = stdTest_StatusCallback;
    pHS->pMessagePrint = stdTest_MessageCallback;
    pHS->pWarningPrint = stdTest_WarningCallback;
    pHS->pErrorPrint   = stdTest_ErrorCallback;
    char aExpected[128];

    // Each logging macro must preserve its source line and formatted message.
    unsigned int line = __LINE__ + 1u;
    STDLOG_DEBUG("debug %d", 17);
    STD_FORMAT(aExpected, "stdTest.c(%u): debug 17", line);
    TEST_ASSERT_EQUAL_STRING(aExpected, stdTest_aPrintedText);
    TEST_ASSERT_EQUAL_size_t(0u, stdTest_logChannel);
    line = __LINE__ + 1u;
    STDLOG_STATUS("status %d", 18);
    STD_FORMAT(aExpected, "stdTest.c(%u): status 18", line);
    TEST_ASSERT_EQUAL_STRING(aExpected, stdTest_aPrintedText);
    TEST_ASSERT_EQUAL_size_t(1u, stdTest_logChannel);
    line = __LINE__ + 1u;
    STDLOG_MESSAGE("message %d", 19);
    STD_FORMAT(aExpected, "stdTest.c(%u): message 19", line);
    TEST_ASSERT_EQUAL_STRING(aExpected, stdTest_aPrintedText);
    TEST_ASSERT_EQUAL_size_t(2u, stdTest_logChannel);
    line = __LINE__ + 1u;
    STDLOG_WARNING("warning %d", 20);
    STD_FORMAT(aExpected, "stdTest.c(%u): warning 20", line);
    TEST_ASSERT_EQUAL_STRING(aExpected, stdTest_aPrintedText);
    TEST_ASSERT_EQUAL_size_t(3u, stdTest_logChannel);
    line = __LINE__ + 1u;
    STDLOG_ERROR("error %d", 21);
    STD_FORMAT(aExpected, "stdTest.c(%u): error 21", line);
    TEST_ASSERT_EQUAL_STRING(aExpected, stdTest_aPrintedText);
    TEST_ASSERT_EQUAL_size_t(4u, stdTest_logChannel);
    TEST_ASSERT_EQUAL_size_t(5u, stdTest_callbackCount);
    TEST_ASSERT_EQUAL_STRING("%s", stdTest_pCallbackFormat);

    // Assert and fatal macros must preserve their callback payloads and Debug/Release behavior.
    J3DTest_ResetAssertCapture();
    STD_ASSERT(1);
    STD_ASSERTREL(1);
    TEST_ASSERT_EQUAL_INT(0, J3DTest_GetAssertCount());
    STD_ASSERT(0);
#ifdef J3D_DEBUG
    TEST_ASSERT_EQUAL_INT(1, J3DTest_GetAssertCount());
    J3DTest_AssertLastAssert("0", "stdTest.c");
#else
    TEST_ASSERT_EQUAL_INT(0, J3DTest_GetAssertCount());
#endif
    J3DTest_ResetAssertCapture();
    STD_ASSERTREL(0);
    TEST_ASSERT_EQUAL_INT(1, J3DTest_GetAssertCount());
    J3DTest_AssertLastAssert("0", "stdTest.c");
    J3DTest_ResetAssertCapture();
    STDLOG_FATAL("fatal literal %s");
    TEST_ASSERT_EQUAL_INT(1, J3DTest_GetAssertCount());
    J3DTest_AssertLastAssert("fatal literal %s", "stdTest.c");
}

TEST_GROUP_RUNNER(std)
{
    RUN_TEST_CASE(std, TestLoggingAndAssertMacrosPreserveCallbackOutputs);
    RUN_TEST_CASE(std, TestPrintfPrefixFillsCapacityAndEmptyMessages);
    RUN_TEST_CASE(std, TestStartupAndShutdownStartOwnedModules);
    RUN_TEST_CASE(std, TestPrintfPrefixesMessagesAndHandlesNullFormat);
    RUN_TEST_CASE(std, TestPrintfTruncatesWithoutLosingFormattedPrefix);
    RUN_TEST_CASE(std, TestConsolePrintfWritesThroughConsoleStub);
    RUN_TEST_CASE(std, TestFileFromPathAndBitPosition);
    RUN_TEST_CASE(std, TestFileWrappersReadWriteSeekTellSizeAndPrintf);
    RUN_TEST_CASE(std, TestFileGetsAndGetws);
    RUN_TEST_CASE(std, TestFilePrintfCapacityAndWriteFailures);
}
